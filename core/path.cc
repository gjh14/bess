#include "path.h"
#include "packet.h"
#include "utils/ether.h"
#include "utils/ip.h"
#include "utils/udp.h"

Path::Path(){
  heads.clear();
  states.clear();
  updates.clear();
}

void Path::appendRule(HeadAction head, StateAction state, UpdateAction update){
  heads.push_back(head);
  states.push_back(state);
  updates.push_back(update);
  total.merge(head);
}

void Path::handlePkt(bess::Packet* pkt){
  for(unsigned i = 0; i < states.size(); ++i){
    if(states[i].action(pkt)){
      UpdateAction next = updates[i];
      total.type = total.pos = 0;
      for(unsigned j = 0; j < i; ++j)
        total.merge(heads[j]);
      handleHead(pkt);
      heads.erase(heads.begin() + i, heads.end());
      states.erase(states.begin() + i, states.end());
      updates.erase(updates.begin() + i, updates.end());
      next(pkt);
      return;
    }
  }
  handleHead(pkt);
  bess::Packet::Free(pkt);
}

void Path::handleHead(bess::Packet* pkt){
  using bess::utils::Ethernet;
  using bess::utils::Ipv4;
  using bess::utils::Udp;
  using bess::utils::be32_t;
  using bess::utils::be16_t;

  Ethernet *eth = pkt->head_data<Ethernet *>();
  Ipv4 *ip = reinterpret_cast<Ipv4 *>(eth + 1);
  size_t ip_bytes = ip->header_length << 2;
  Udp *udp = reinterpret_cast<Udp *>(reinterpret_cast<uint8_t *>(ip) + ip_bytes);
  if(total.type == HeadAction::MODIFY){
    for(unsigned i = 0; i < HeadAction::POSNUM; ++i)
      if(total.pos & (1 << i))
        switch(i){
          case HeadAction::PROTO:
          ip->protocol = total.value[i];
          break;
          case HeadAction::SRC_IP:
          ip->src = be32_t(total.value[i]);
          break;
          case HeadAction::SRC_PORT:
          udp->src_port = be16_t(total.value[i]);
          case HeadAction::DST_IP:
          ip->dst = be32_t(total.value[i]);
          case HeadAction::DST_PORT:
          udp->dst_port = be16_t(total.value[i]);
          break;
        }
    // TODO: correct checksum
  }
}

