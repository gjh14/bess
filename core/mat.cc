#include "mat.h"

#include <vector>

#include "module.h"
#include "packet.h"
#include "utils/ether.h"
#include "utils/ip.h"
#include "utils/tcp.h"
#include "utils/udp.h"

MAT::MAT(){
  for (uint32_t i = 0; i < MAX_PATHS; ++i)
    paths[i].mat = this;
}

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
  std::string fid;
  uint64_t hash = 0;
  getFID(pkt, fid, hash);
  path = paths + hash;
  if (paths[hash].fid() == fid)
    return true;
  paths[hash].set_fid(fid);
  return false;
}

void MAT::runMAT(bess::PacketBatch *batch) {
  int cnt  = batch->cnt();
  // LOG(INFO) << "Start: " << cnt << " " << modules.size() << " " << ports.size();

  for(int i = 0; i < cnt; ++i)
    batch->path(i)->handlePkt(batch->pkts()[i]);
/*
  int first[bess::PacketBatch::kMaxBurst];
  for(int i = 0; i < cnt; ++i){
    bess::Packet *pkt = batch->pkts()[i];
    Path *path = batch->path(i);
    first[i] = -1;
    for (unsigned j = 0; j < path->modules.size(); ++j)
      path->modules[j]->parallel()->append(i, j, pkt, path->states[j]);
  }
 
  for (Module *module : modules)
    module->parallel()->start();
  
  for (Module *module : modules){
    Parallel *parallel = module->parallel();
    parallel->join();
    for(int i = 0; i < parallel->cnt(); ++i)
      if(parallel->result(i))
        first[parallel->rank(i)] = parallel->pos(i);
  }
  
  bess::PacketBatch left;
  left.clear();
  bess::PacketBatch unit;
  for(int i = 0; i < cnt; ++i){
    bess::Packet *pkt = batch->pkts()[i];
    Path *path = batch->path(i);
    if(first[i] >= 0){
      unit.clear();
      unit.add(pkt, path);
      path->rehandle(i, &unit);
    }
    else
      left.add(pkt, path);
  }

  cnt = left.cnt();
  bess::PacketBatch send;
  for (Module *port : ports) {
    send.clear();
    for(int i = 0; i < cnt; ++i)
      if (left.path(i)->port() == port)
        send.add(left.pkts()[i], nullptr);
    port->ProcessBatch(&send);
  }
  */
  // LOG(INFO) << "End: " << cnt;
}

void MAT::add_module(Module *module){
  for (Module* module_ : modules)
    if (module_ == module)
      return;
  LOG(INFO) << "Module " << module;
  modules.push_back(module);
}

void MAT::add_port(Module *port){
  for (Module* port_ : ports)
    if (port_ == port)
      return;
  LOG(INFO) << "Port " << port;
  ports.push_back(port);
}
