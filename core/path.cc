#include "path.h"

Path::Path(){
  heads.clear();
  states.clear();
  updates.clear();
}

void Path::appendRule(HeadAction head, StateAction state, UpdateAction update){
  heads.append(head);
  states.append(state);
  updates.append(update);
  total.merge(head);
}

void Path::handlePkt(bess:Packet* pkt){
  for(unsigned i = 0; i < states.size(); ++i){
    if((*states[i].action)(pkt)){
      UpdateAction next = updates[i];
      total.type = total.pos = 0;
      for(unsigned j = 0; j < i; ++j)
        total.merge(heads[j]);
      handleHead(pkt);
      heads.erase(heads.being() + i, heads.end());
      states.erase(states.being() + i, states.end());
      updates.erase(updates.being() + i, updates.end());
      next(pkt);
      return;
    }
  }
  handleHead(pkt);
  bess:Packet::Free(pkt);
}

