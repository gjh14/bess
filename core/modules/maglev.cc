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
  ngates = arg.ngates();
  for(uint32_t i = 0; i < ngates; ++i){
    is_valid.push_back(true);
    shuffle_list.push_back(std::vector<uint32_t>());
    for(uint32_t j = 0; j < size; ++j)
      shuffle_list[i].push_back(j);
    std::random_shuffle(shuffle_list[i].begin(), shuffle_list[i].end());
  }
  for(uint32_t i = 0; i < size; ++i)
    hash_table.push_back(DROP_GATE);
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
  for(uint32_t i = 0; i < ngates; ++i)
    is_valid[i] = false;
  build();
  return CommandSuccess();
}

void Maglev::build(){
  for(uint32_t i = 0; i < size; ++i)
    hash_table[i] = DROP_GATE;
  std::vector<uint32_t> gates;
  std::vector<uint32_t> point;
  for(uint32_t i = 0; i < ngates; ++i)
    if(is_valid[i]){
      gates.push_back(i);
      point.push_back(0);
    }
  for(uint32_t linked = 0; linked < size;){
    for(unsigned i = 0; i < gates.size(); ++i){
      for(; hash_table[shuffle_list[gates[i]][point[i]]] < ngates; ++point[i]);
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

  gate_idx_t out_gates[bess::PacketBatch::kMaxBurst];

  int cnt = batch->cnt();
  for (int i = 0; i < cnt; i++) {
    bess::Packet *pkt = batch->pkts()[i];

    Ethernet *eth = pkt->head_data<Ethernet *>();
    Ipv4 *ip = reinterpret_cast<Ipv4 *>(eth + 1);
    size_t ip_bytes = ip->header_length << 2;
    Udp *udp = reinterpret_cast<Udp *>(reinterpret_cast<uint8_t *>(ip) + ip_bytes);

    uint32_t value = hash(ip->protocol, ip->src, udp->src_port, ip->dst, udp->dst_port);
    uint32_t gate = hash_table[value];
    out_gates[i] = gate;
      
    HeadAction head;
    if(gate == DROP_GATE)
      head.type = HeadAction::DROP;
    StateAction state;
    state.type = StateAction::UNRELATE;
    state.action = 
      [=](bess::Packet *pkt[[maybe_unused]]) ->bool {
        return hash_table[value] != gate;
      };
    auto update = 
      [&](bess::Packet *pkt) {
        bess::PacketBatch batch;
        batch.clear();
        batch.add(pkt);
        ProcessBatch(&batch);
      };
    batch->path()->appendRule(head, state, update);
  }
  RunSplit(out_gates, batch);
}

ADD_MODULE(Maglev, "maglev", "Load Balance System from Google")

