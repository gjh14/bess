#include "maglev.h"

#include <algorithm>

#include "../utils/ether.h"
#include "../utils/ip.h"
#include "../utils/udp.h"

const Commands Maglev::cmds = {
    {"modify", "MaglevCommandArg", MODULE_CMD_FUNC(&Maglev::CommandModify),
     Command::Command::THREAD_UNSAFE},
    {"clear", "EmptyArg", MODULE_CMD_FUNC(&Maglev::CommandClear),
     Command::Command::THREAD_UNSAFE}};
     
CommandResponse Maglev::Init(const bess::pb::MaglevArg &arg){
  size = arg.size();
  for (const auto &dst : arg.dsts()) {
    be32_t addr(0);
    if(ParseIpv4Address(dst.dst_ip(), &addr)){
      MaglevDst new_dst = {
        .dst_ip = addr.raw_value(),
        .dst_port = (uint16_t)dst.dst_port()};
      dsts.push_back(new_dst);
    }
  }
  ndsts = dsts.size();
  
  for(uint32_t i = 0; i < ndsts; ++i){
    is_valid.push_back(true);
    shuffle_list.push_back(std::vector<uint32_t>());
    for(uint32_t j = 0; j < size; ++j)
      shuffle_list[i].push_back(j);
    std::random_shuffle(shuffle_list[i].begin(), shuffle_list[i].end());
  }
  for(uint32_t i = 0; i < size; ++i)
    hash_table.push_back(ndsts);
  build();
  
  return CommandSuccess();
}

CommandResponse Maglev::CommandModify(const bess::pb::MaglevCommandArg &arg){
  for(const auto &mod : arg.mods())
    is_valid[mod.gate()] = mod.state();
  build();
  return CommandSuccess();
}

CommandResponse Maglev::CommandClear(const bess::pb::EmptyArg &){
  for(uint32_t i = 0; i < ndsts; ++i)
    is_valid[i] = false;
  build();
  return CommandSuccess();
}

void Maglev::build(){
  for(uint32_t i = 0; i < size; ++i)
    hash_table[i] = ndsts;
  std::vector<uint32_t> gates;
  std::vector<uint32_t> point;
  for(uint32_t i = 0; i < ndsts; ++i)
    if(is_valid[i]){
      gates.push_back(i);
      point.push_back(0);
    }
  if(gates.size() == 0)
    return; 

  for(uint32_t linked = 0; linked < size;){
    for(unsigned i = 0; i < gates.size() && linked < size; ++i){
      for(; hash_table[shuffle_list[gates[i]][point[i]]] < ndsts; ++point[i]);
      ++linked;
      hash_table[shuffle_list[gates[i]][point[i]]] = gates[i];
    }
  }
}

uint32_t Maglev::hash(uint8_t protocol, be32_t src_ip, be16_t src_port, be32_t dst_ip, be16_t dst_port){
  uint64_t value = protocol;
  value = ((value << 32) | src_ip.raw_value()) % size;
  value = ((value << 16) | src_port.raw_value()) % size;
  value = ((value << 32) | dst_ip.raw_value()) % size;
  value = ((value << 16) | dst_port.raw_value()) % size;
  return (uint32_t)value;
}

void Maglev::ProcessBatch(bess::PacketBatch *batch) {
  using bess::utils::Ethernet;
  using bess::utils::Ipv4;
  using bess::utils::Udp;
  
  bess::PacketBatch out_batch;
  out_batch.clear();
  bess::PacketBatch free_batch;
  free_batch.clear();
  
  int cnt = batch->cnt();
  for (int i = 0; i < cnt; i++) {
    bess::Packet *pkt = batch->pkts()[i];

    Ethernet *eth = pkt->head_data<Ethernet *>();
    Ipv4 *ip = reinterpret_cast<Ipv4 *>(eth + 1);
    size_t ip_bytes = ip->header_length << 2;
    Udp *udp = reinterpret_cast<Udp *>(reinterpret_cast<uint8_t *>(ip) + ip_bytes);

    uint32_t value = hash(ip->protocol, ip->src, udp->src_port, ip->dst, udp->dst_port);
    uint32_t gate = hash_table[value];
      
    HeadAction head;
    if(gate == ndsts){
      head.type = HeadAction::DROP;
      free_batch.add(pkt);
    }else{
      ip -> dst = be32_t(dsts[gate].dst_ip);
      udp -> dst_port = be16_t(dsts[gate].dst_port);
      head.modify(HeadAction::DST_IP, dsts[gate].dst_ip);
      head.modify(HeadAction::DST_PORT, dsts[gate].dst_port);
      out_batch.add(pkt);
    }
    
    StateAction state;
    state.type = StateAction::UNRELATE;
    state.action = 
      [=](bess::Packet *pkt[[maybe_unused]]) ->bool {
        if(hash_table[value] != gate)
          LOG(INFO) << "CHANGE " << value << " " <<  gate << " " << hash_table[value];
        return hash_table[value] != gate;
      };
    auto update = 
      [&](bess::PacketBatch *unit) {
        ProcessBatch(unit);
      };
    batch->path()->appendRule(head, state, update);
  }

  bess::Packet::Free(&free_batch);

  out_batch.set_path(batch->path());
  RunNextModule(&out_batch);
}

ADD_MODULE(Maglev, "maglev", "Load Balance System from Google")

