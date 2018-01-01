#include "mat.h"

#include "utils/ether.h"
#include "utils/ip.h"
#include "utils/udp.h"

Mat:Mat(){
  mat.clear();
}

~Mat::Mat() {
  for(auto path : mat)
    delete path;
}

void Mat::appendData(std::tring *fid, uint32_t num, uint32_t len){
  for (int i = 0; i < len; ++i){
    fid->push_back(num & 255);
      num >>= 8;
  }
}

std::string* Mat::getFID(bess:Packet pkt) {
  std::string* fid = new std::string();
  Ethernet *eth = pkt->head_data<Ethernet *>();
  Ipv4 *ip = reinterpret_cast<Ipv4 *>(eth + 1);
  size_t ip_bytes = ip->header_length << 2;
  Udp *udp = reinterpret_cast<Udp *>(reinterpret_cast<uint8_t *>(ip) + ip_bytes);
  appendData(fid, ip->protocol, 1);
  appendData(fid, ip->src, 4);
  appendData(fid, ip->dst, 4);
  appendData(fid, udp->src_port, 2);
  appendData(fid, udp->dst_port, 2);
  return fid;
}

void Mat::checkMat(bess::Packet *pkt){
  std::string *fid = getFID(pkt);
  if (mat.count(*fid)) {
    Path *path = mat[*fid];
    path->handlePkt(pkt);
  } else {
    Path *path = new Path();
    mat[*fid] = path;
    pkt->set_path(path);
  }
  delete fid;
}

