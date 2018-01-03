#include "mat.h"
#include "packet.h"

#include "utils/ether.h"
#include "utils/ip.h"
#include "utils/udp.h"

MAT::MAT(){
  mat.clear();
}

MAT::~MAT() {
  for(auto it : mat)
    delete it.second;
}

void MAT::appendData(std::string *fid, uint32_t num, int len){
  for (int i = 0; i < len; ++i){
    fid->push_back(num & 255);
      num >>= 8;
  }
}

std::string* MAT::getFID(bess::Packet *pkt) {
  using bess::utils::Ethernet;
  using bess::utils::Ipv4;
  using bess::utils::Udp;
  std::string *fid = new std::string();
  Ethernet *eth = pkt->head_data<Ethernet *>();
  Ipv4 *ip = reinterpret_cast<Ipv4 *>(eth + 1);
  size_t ip_bytes = ip->header_length << 2;
  Udp *udp = reinterpret_cast<Udp *>(reinterpret_cast<uint8_t *>(ip) + ip_bytes);
  appendData(fid, ip->protocol, 1);
  appendData(fid, ip->src.raw_value(), 4);
  appendData(fid, ip->dst.raw_value(), 4);
  appendData(fid, udp->src_port.raw_value(), 2);
  appendData(fid, udp->dst_port.raw_value(), 2);
  return fid;
}

bool MAT::checkMAT(bess::Packet *pkt, Path *&path){
  std::string *fid = getFID(pkt);
  if (mat.count(*fid)) {
    path = mat[*fid];
    path->handlePkt(pkt);
    delete fid;
    return true;
  }
  mat[*fid] = path = new Path();
  delete fid;
  return false;
}

