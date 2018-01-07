#include "mat.h"
#include "packet.h"

#include "utils/ether.h"
#include "utils/ip.h"
#include "utils/tcp.h"
#include "utils/udp.h"

MAT::MAT(){
  // mat.clear();
}

MAT::~MAT() {
  // for(auto it : mat)
    // delete it.second;
}

void MAT::appendData(std::string &fid, uint64_t &hash, uint32_t num, int len){
  hash = ((hash << len) | num) % MAX_PATHS;
  for (int i = 0; i < len; ++i){
    fid.push_back(num & 255);
    num >>= 8;
  }
}

void MAT::getFID(bess::Packet *pkt, std::string &fid, uint64_t &hash) {
  using bess::utils::Ethernet;
  using bess::utils::Ipv4;
  using bess::utils::Udp;
  Ethernet *eth = pkt->head_data<Ethernet *>();
  Ipv4 *ip = reinterpret_cast<Ipv4 *>(eth + 1);
  size_t ip_bytes = ip->header_length << 2;
  Udp *udp = reinterpret_cast<Udp *>(reinterpret_cast<uint8_t *>(ip) + ip_bytes);
  appendData(fid, hash, ip->protocol, 1);
  appendData(fid, hash, ip->src.raw_value(), 4);
  appendData(fid, hash, udp->src_port.raw_value(), 2);
  appendData(fid, hash, ip->dst.raw_value(), 4);
  appendData(fid, hash, udp->dst_port.raw_value(), 2);
  
/*  
  using bess::utils::Tcp;
  Tcp* tcp = reinterpret_cast<Tcp *>(reinterpret_cast<uint8_t *>(ip) + ip_bytes);
  const char *datastart = ((const char *)tcp) + (tcp->offset * 4);
  uint32_t datalen = ip->length.value() - (tcp->offset * 4) - (ip_bytes);
  if(datalen > 0 && datalen < 20 && datastart + datalen <= (char*)pkt + sizeof(bess::Packet)){
    std::string payload(datastart, datalen);
    LOG(INFO) << fid << " " << datalen << " " << payload;
  }
*/
}

bool MAT::checkMAT(bess::PacketBatch *unit){
  bess::Packet *pkt = unit->pkts()[0];
  std::string *fid = new std::string();
  uint64_t hash = 0;
  getFID(pkt, *fid, hash);
  unit->set_path(paths + hash);
  if(paths[hash].fid() !=nullptr && *paths[hash].fid() == *fid){
    delete fid;
    paths[hash].handlePkt(unit);
    LOG(INFO) << hash << " HIT";
    return true;
  }
  paths[hash].set_fid(fid);
  LOG(INFO) << hash << " UNHIT";
  return false;
/*  
  if (mat.count(fid)) {
    Path *path = mat[fid];
    unit->set_path(path);
    path->handlePkt(unit);
    return true;
  }
  Path *path = new Path();
  mat[fid] = path;
  unit->set_path(path);
  return false;
*/  
}

