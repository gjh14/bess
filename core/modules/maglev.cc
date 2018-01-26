#include "maglev.h" 
#include <algorithm>
#include <rte_cycles.h>

#include "../utils/ether.h"
#include "../utils/ip.h"
#include "../utils/udp.h"

const Commands Maglev::cmds = {
    {"modify", "MaglevCommandArg", MODULE_CMD_FUNC(&Maglev::CommandModify),
     Command::Command::THREAD_UNSAFE},
    {"clear", "EmptyArg", MODULE_CMD_FUNC(&Maglev::CommandClear),
     Command::Command::THREAD_UNSAFE}};

Maglev::Maglev() : Module() {
  sfunc = [&](bess::Packet *pkt[[maybe_unused]], void *arg) ->bool {
      MaglevArg *check = (MaglevArg*) arg;
      return hash_table[check->value] != check->gate;
    };
}

CommandResponse Maglev::Init(const bess::pb::MaglevArg &arg){
  size = arg.size();
  for (const auto &dst : arg.dsts()) {
    be32_t addr(0);
    if(ParseIpv4Address(dst.dst_ip(), &addr)){
      MaglevDst new_dst = {
        .dst_ip = addr.raw_value(),
        .dst_port = be16_t(dst.dst_port()).raw_value()
      };
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

uint32_t Maglev::hash(uint8_t protocol, uint32_t src_ip, uint16_t src_port, uint32_t dst_ip, uint16_t dst_port){
  uint64_t value = protocol;
  value = ((value << 32) | src_ip) % size;
  value = ((value << 16) | src_port) % size;
  value = ((value << 32) | dst_ip) % size;
  value = ((value << 16) | dst_port) % size;
  return (uint32_t)value;
}

void Maglev::ProcessBatch(bess::PacketBatch *batch) {
  bess::PacketBatch out_batch;
  out_batch.clear();
  bess::PacketBatch free_batch;
  free_batch.clear();
  
  int cnt = batch->cnt();
  for (int i = 0; i < cnt; i++) {
    bess::Packet *pkt = batch->pkts()[i];
    MAT::mark(pkt);
    Path *path = batch->path(i);

    uint64_t hval = 0;
    uint8_t fid[Path::FIDLEN];
    MAT::getFID(pkt, hval, fid);
    if (memcmp(fid, cache[hval], Path::FIDLEN)) {
      int delay = 1;
      for(int j = 0; j < 650; ++j)
        delay *= i ^ (i & 1);
      i += delay; 
    }
    memcpy(cache[hval], fid, Path::FIDLEN);

    HeadAction *head = nullptr;
    StateAction *state = nullptr;
    if (path != nullptr)
      path->appendRule(this, head, state);

    uint8_t *protocol = (uint8_t *)pkt + 535;
    uint32_t *src_ip = (uint32_t *)((uint8_t *)pkt + 538);
    uint16_t *src_port = (uint16_t *)((uint8_t *) pkt + 546);
    uint32_t *dst_ip = (uint32_t *)((uint8_t *)pkt + 542);
    uint16_t *dst_port =  (uint16_t *)((uint8_t *) pkt + 548);
    
    uint32_t value = hash(*protocol, *src_ip, *src_port, *dst_ip, *dst_port);
    uint32_t gate = hash_table[value];
      
    if (gate == ndsts) {
      if (head != nullptr)
        head->type = HeadAction::DROP;
      free_batch.add(pkt, nullptr);
    } else {
      *dst_ip = dsts[gate].dst_ip;
      *dst_port = *dst_port; // dsts[gate].dst_port;
      if (head != nullptr) {
        head->modify(HeadAction::DST_IP, dsts[gate].dst_ip);
        head->modify(HeadAction::DST_PORT, *dst_port); // dsts[gate].dst_port); 
      }
      out_batch.add(pkt, path);
    }
    
    if (state != nullptr) {
      state->type = StateAction::UNRELATE;
      state->action = nullptr;
      /* MaglevArg *arg = (MaglevArg *)malloc(sizeof(MaglevArg));    
      state->action = sfunc;
      arg->value = value;
      arg->gate = gate;
      state->arg = (void*)arg; */
    }
    
    MAT::stat(pkt);
  }

  bess::Packet::Free(&free_batch);
  
  RunNextModule(&out_batch);
}

ADD_MODULE(Maglev, "maglev", "Load Balance System from Google")
