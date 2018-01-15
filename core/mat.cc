#include "mat.h"

#include <unordered_set>

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

bool MAT::checkMAT(bess::Packet *pkt, Path *&path) {
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

void MAT::runMAT(bess::PacketBatch *batch) {
  int first[bess::PacketBatch::kMaxBurst];
  int cnt  = batch->cnt();
  unordered_set<Port *> modules;
  for(int i = 0; i < cnt; ++i){
    bess::Packet *pkt = batch->pkts()[i];
    Path *path = batch->path(i);
    first[i] = -1;
    for(unsigned j = 0; j < path->modules.size(); ++j) {
      modules.insert(path->modules[j]);
      path->modules[j]->parallel.append(i, j, pkt, path->actions[j]);
    }
  }
  
  for(Modules* module : modules)
    module->parallel.start();
  
  for(Modules* module : modules){
    Parallel parallel = &module->parallel;
    while(parallel->finish());
    for(int i = 0; i < parallel->cnt(); ++i)
      if(parallel->result(i))
        first[parallel->rank(i)] = parallel->pos(i);
  }
  
  bess:PacketBatch left;
  left.clear();
  bess:PacketBatch unit;
  for(int i = 0; i < cnt; ++i){
    bess::Packet *pkt = batch->pkts()[i];
    Path *path = batch->path(i);
    if(first[i] >= 0){
      unit.clear();
      unit.add(pkt, path);
      path->rehandle(i, unit);
    }
    else
      left.add(pkt, path);
  }

  unordered_set<Port *> ports;
  cnt = left.cnt();
  for(int i = 0; i < cnt; ++i)
    ports.insert(left.path(i)->port());
  bess:PacketBatch* send;  
  for(Port *port : ports){
    send.clear();
    for(int i = 0; i < cnt; ++i)
      if(left.path(i) == port)
       send.add(left.pkts()[i], nullptr);
    port->ProcessBatch(send);
  }
}

