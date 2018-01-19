#include "path.h"

#include <cstring>
#include <rte_cycles.h>

#include "mat.h"
#include "module.h"
#include "packet.h"

void HeadAction::modify(uint32_t _pos, uint32_t _value) {
  if(type == DROP)
    return;
  type |= MODIFY;
  mask[_pos] = 0;
  value[_pos] = _value;
}
  
void HeadAction::merge(HeadAction *action) {
  if(action == nullptr || type == DROP)
    return;

   if(action->type & DROP) {
    type = DROP;
    return;
  }

  if(action->type & MODIFY) {
    type |= MODIFY;
    for(uint32_t i = 0; i < POSNUM; ++i) {
      mask[i] &= action->mask[i];
      value[i] &= (value[i] & action->mask[i]) | action->value[i];
    }
  }
}

Path::~Path(){
  for(int i = 0; i < cnt_; ++i)
    free(states[i].arg);
}

void Path::set_fid(const FID &fid) {
  fid_ = fid;
  clear();
}

void Path::appendRule(Module *module, HeadAction *&head, StateAction *&state){
  if(cnt_ >= MAXLEN)
    LOG(ERROR) << "Exceed Max Length";
  head = heads + cnt_;
  state = states + cnt_;
  modules[++cnt_] = module;
  total.merge(head);
  mat->add_module(module);
}

void Path::clear() {
  for(int i = 0; i < cnt_; ++i){
    heads[i].clear();
    free(states[i].arg);
  }
  cnt_ = 0;
  total.clear();
}

void Path::set_port(Module *port) {
  port_ = port;
  mat->add_port(port);
}

void Path::handlePkt(bess::Packet *pkt){
  bess::PacketBatch unit;
  unit.clear();
  unit.add(pkt, this);
 
  static uint64_t tot = 0, sum = 0;
  uint64_t start = rte_get_timer_cycles();
  /* 
  for(int i = 0; i < cnt_; ++i)
    if(states[i].action!= nullptr && states[i].action(pkt, states[i].arg))
      rehandle(i, &unit);
  */
  handleHead(pkt);

  uint64_t end = rte_get_timer_cycles();
  sum += end - start;
  LOG(INFO) << end - start << " " << ++tot << " " << sum;

  if(port_ != nullptr)
    port_->ProcessBatch(&unit);
  else
    bess::Packet::Free(pkt);
}

void Path::rehandle(int pos, bess::PacketBatch *unit) {
  Module *trigger = modules[pos];
  
  total.clear();
  for(int i = 0; i < pos; ++i)
    total.merge(&heads[i]);
  handleHead(unit->pkts()[0]);

  for(int i = pos; i < cnt_; ++i){
    heads[i].clear();
    free(states[i].arg);
  }
  cnt_ = pos;

  trigger->ProcessBatch(unit);
}

void Path::handleHead(bess::Packet *pkt){
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
}
