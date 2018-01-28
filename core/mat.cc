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
  uint64_t hash = 0;
  uint8_t fid[Path::FIDLEN];

  getFID(pkt, hash, fid);
  path = paths + hash;

  if (!memcmp(fid, path->fid(), Path::FIDLEN))
    return true;

  path->set_fid(fid);
  return false;
}

void MAT::runMAT(bess::PacketBatch *batch) {
  /* static uint64_t a = 0, b = 0;
  uint64_t c = rte_get_timer_cycles(); */
  
  int cnt  = batch->cnt();
  bess::PacketBatch out_batch; 
  bess::PacketBatch free_batch;
  out_batch.clear();
  free_batch.clear();
  bess::PacketBatch unit;

  for (int i = 0; i < cnt; ++i) {
    bess::Packet *pkt = batch->pkts()[i];
    // mark(pkt);
     
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
      path->handleHead(pkt);
      if (path->port() == nullptr)
        free_batch.add(pkt, nullptr);
      else
        out_batch.add(pkt, path);
    }
    
    // stat(pkt);
  }
 
  /* uint64_t d = rte_get_timer_cycles();
  a += cnt;
  b += d - c;
  LOG(INFO) << cnt << " " << d - c << " " << a << " " << b; */
   
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

uint64_t MAT::tot[65536];
uint64_t MAT::sum[65536];
uint64_t MAT::start[65536];
  
void MAT::init(){
  memset(tot, 0, sizeof(tot));
  memset(sum, 0, sizeof(sum));
  memset(start, 0, sizeof(start));
}

void MAT::mark(bess::Packet *pkt) {
  uint16_t dport = *(uint16_t *)((uint8_t *) pkt + 548);
  start[dport] = rte_get_timer_cycles();
}

void MAT::stat(bess::Packet *pkt) {
  uint64_t end = rte_get_timer_cycles();
  uint16_t dport = *(uint16_t *)((uint8_t *)pkt + 548);
  ++tot[dport];
  sum[dport] += end - start[dport];
  // LOG(INFO) << dport << " " << tot[dport] << " " << end - start[dport];
}

void MAT::output() {
  for (uint32_t i = 0; i < 5750; ++i) {
    uint32_t j = ((i & 255) << 8 ) | (i >> 8); 
    LOG(INFO) << i  << " " << tot[j] << " " << sum[j];
  }
}

