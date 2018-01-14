#include "path.h"

#include <rte_cycles.h>

#include "module.h"
#include "packet.h"
#include "utils/ether.h"
#include "utils/ip.h"
#include "utils/udp.h"

Path::Path(){
  fid_ = nullptr;
  port_ = nullptr;
}

Path::~Path(){
  delete fid_;
}

void Path::set_fid(std::string *fid){
  delete fid_;
  fid_ = fid;
  heads.clear();
  states.clear();
  updates.clear();
  total.clear();
}

const std::string *Path::fid(){
  return fid_;
}

void Path::set_port(Module* port){
  port_ = port;
}

void Path::appendRule(Module *module, HeadAction head, StateAction state, UpdateAction update){
  modules.push_back(module);
  heads.push_back(head);
  states.push_back(state);
  updates.push_back(update);
  total.merge(head);
}

void Path::handlePkt(bess::PacketBatch *unit){
  bess::Packet *pkt = unit->pkts()[0];
  
  static uint64_t tot = 0, sum = 0;
  uint64_t start = rte_get_timer_cycles();
  start = rte_get_timer_cycles();

  for(unsigned i = 0; i < modules.size(); ++i)
    modules[i]->start(states[i].action, pkt);
    
  for(unsigned i = 0; i < modules.size(); ++i)
    if(modules[i]->result()){
      for(; i < modules.size(); ++i)
        modules[i]->result();
  // for(unsigned i = 0; i < modules.size(); ++i)
    // if(states[i].action(pkt)){
      UpdateAction next = updates[i];
      
      total.clear();  
      for(unsigned j = 0; j < i; ++j)
        total.merge(heads[j]);
      handleHead(pkt);
      
      modules.erase(modules.begin() + i, modules.end());
      heads.erase(heads.begin() + i, heads.end());
      states.erase(states.begin() + i, states.end());
      updates.erase(updates.begin() + i, updates.end());
      
      next(unit);  
      return;
    }
  
  uint64_t end = rte_get_timer_cycles();
  sum += end - start;
  LOG(INFO) << end - start << " " << ++tot << " " << sum;


  handleHead(pkt);
  
  if(port_ != nullptr)
    port_->ProcessBatch(unit);
  else
    bess::Packet::Free(pkt);
}

void Path::handleHead(bess::Packet *pkt){
  // static uint64_t tot = 0, sum = 0;
  // uint64_t start = rte_get_timer_cycles();
  // start = rte_get_timer_cycles();

  if(total.type & HeadAction::DROP){
    port_ = nullptr;
    return;
  }

  uint8_t *protocol = (uint8_t *)pkt + 535;
  uint32_t *src_ip = (uint32_t *)((uint8_t *)pkt + 538);
  uint16_t *src_port = (uint16_t *)((uint8_t *) pkt + 546);
  uint32_t *dst_ip = (uint32_t *)((uint8_t *)pkt + 542);
  uint16_t *dst_port =  (uint16_t *)((uint8_t *) pkt + 548);
  
  *protocol = (*protocol & total.mask[0]) | total.value[0];
  *src_ip = (*src_ip & total.mask[1]) | total.value[1];
  *src_port = (*src_port & total.mask[2]) | total.value[2];
  *dst_ip = (*dst_ip & total.mask[3]) | total.value[1];
  *dst_port = (*dst_port & total.mask[4]) | total.value[4];

  // TODO: correct checksum
  
  // uint64_t end = rte_get_timer_cycles();
  // if(end - start < 60){
    // sum += end - start;
    // LOG(INFO) << end - start << " " << ++tot << " " << sum;
  // }
}

