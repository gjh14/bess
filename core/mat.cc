#include "mat.h"
#include "packet.h"

#include "utils/ether.h"
#include "utils/ip.h"
#include "utils/tcp.h"
#include "utils/udp.h"

void MAT::appendData(std::string &fid, uint64_t &hash, uint32_t num, int len){
  hash = ((hash << (len << 3)) | num) % MAX_PATHS;
  for (int i = 0; i < len; ++i){
    fid.push_back(num & 255);
    num >>= 8;
  }
}

void MAT::getFID(bess::Packet *pkt, std::string &fid, uint64_t &hash) {
  uint8_t *protocol = (uint8_t *)pkt + 535;
  uint32_t *src_ip = (uint32_t *)((uint8_t *)pkt + 538);
  uint16_t *src_port = (uint16_t *)((uint8_t *) pkt + 546);
  uint32_t *dst_ip = (uint32_t *)((uint8_t *)pkt + 542);
  uint16_t *dst_port =  (uint16_t *)((uint8_t *) pkt + 548);
  
  appendData(fid, hash, *protocol, 1);
  appendData(fid, hash, *src_ip, 4);
  appendData(fid, hash, *src_port, 2);
  appendData(fid, hash, *dst_ip, 4);
  appendData(fid, hash, *dst_port, 2);
}

bool MAT::checkMAT(bess::Packet *pkt, Path *&path){
  std::string *fid = new std::string();
  uint64_t hash = 0;
  getFID(pkt, *fid, hash);
  path = paths + hash;
  if(paths[hash].fid() != nullptr && *paths[hash].fid() == *fid){
    delete fid;
    return true;
  }
  paths[hash].set_fid(fid);
  return false;
}

void MAT::runMAT(bess::PacketBatch *batch){
  int cnt = batch->cnt();
  for(int i = 0; i < cnt; ++i)
    batch->path(i)->handlePkt(batch->pkts()[i]);
}

