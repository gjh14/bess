#include "mat.h"

#include <cstring>
#include <rte_cycles.h>
#include <vector>

#include "module.h"
#include "packet.h"

MAT::MAT(){
  for (uint32_t i = 0; i < MAX_PATHS; ++i)
    paths[i].mat = this;
}

void MAT::getFID(bess::Packet *pkt, uint64_t &hash, uint8_t *fid){
  uint8_t *protocol = (uint8_t *)pkt + 535;
  uint32_t *src_ip = (uint32_t *)((uint8_t *)pkt + 538);
  uint16_t *src_port = (uint16_t *)((uint8_t *) pkt + 546);
  uint32_t *dst_ip = (uint32_t *)((uint8_t *)pkt + 542);
  uint16_t *dst_port =  (uint16_t *)((uint8_t *) pkt + 548);
    
  append(hash, fid, *protocol, 1, 0); 
  append(hash, fid, *src_ip, 4, 1); 
  append(hash, fid, *src_port, 2, 5); 
  append(hash, fid, *dst_ip, 4, 7); 
  append(hash, fid, *dst_port, 2, 11);

  hash += delay(hash);
}

void MAT::append(uint64_t &hash, uint8_t *fid, uint32_t num, int len, int pos) {
  hash = ((hash << (len << 3)) | num) % MAX_PATHS;
  for (int i = 0; i < len; ++i) {
    fid[pos + i] = num & 255;
    num >>= 8;
  }
}

int MAT::delay(int x){
  int res = 1;
  for(int i = 0; i < 500; ++i)
   res *= x ^ (x & 1);
  return res;
}

bool MAT::checkMAT(bess::Packet *pkt, Path *&path) {
  /*static uint64_t tot = 0, sum = 0;
  uint64_t start = rte_get_timer_cycles();*/
 
  uint64_t hash = 0;
  uint8_t fid[Path::FIDLEN];

  getFID(pkt, hash, fid);
  path = paths + hash;

  if (!memcmp(fid, path->fid(), Path::FIDLEN))
    return true;

  path->set_fid(fid);
  /*uint64_t end = rte_get_timer_cycles();
  sum += end - start;
  LOG(INFO) << end - start << " " << ++tot << " " << sum;*/
  return false;
}

void MAT::runMAT(bess::PacketBatch *batch) {
  // static uint64_t tot = 0, sum = 0;
  // uint64_t start = rte_get_timer_cycles();

  int cnt  = batch->cnt();
  bess::PacketBatch out_batch; 
  bess::PacketBatch free_batch;
  out_batch.clear();
  free_batch.clear();
  bess::PacketBatch unit;

  for (int i = 0; i < cnt; ++i) {
    bess::Packet *pkt = batch->pkts()[i];
    Path *path = batch->path(i);
    if (path->port() == nullptr) {
      free_batch.add(pkt, nullptr);
      continue;
    }
    
    unit.clear();
    for(int j = 0; j < path->cnt_; ++j) {
      StateAction &state = path->states[j];
      if (state.action!= nullptr && state.action(pkt, state.arg)){
        unit.add(pkt, path);        
        path->rehandle(j, &unit);
        break;
      }
    }
    
    if (!unit.cnt()) {
      // path->handleHead(pkt);
      out_batch.add(pkt, path);
    }
  }


  /* int first[bess::PacketBatch::kMaxBurst];
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
  } */

  // uint64_t end = rte_get_timer_cycles();
  // tot += cnt; 
  // sum += end - start;
  // LOG(INFO) << cnt << " " << end - start << " " << tot << " " << sum;

  bess::PacketBatch send;
  for (Module *port : ports) {
    send.clear();
    for(int i = 0; i < out_batch.cnt(); ++i)
      if (out_batch.path(i)->port() == port)
        send.add(out_batch.pkts()[i], nullptr);
    port->ProcessBatch(&send);
  }
  bess::Packet::Free(&free_batch);
}

void MAT::add_module(Module *module){
  for (Module* module_ : modules)
    if (module_ == module)
      return;
  // LOG(INFO) << "Module " << module;
  modules.push_back(module);
}

void MAT::add_port(Module *port){
  for (Module* port_ : ports)
    if (port_ == port)
      return;
  // LOG(INFO) << "Port " << port;
  ports.push_back(port);
}

