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

bool MAT::checkMAT(bess::Packet *pkt, Path *&path) {
  /*static uint64_t tot = 0, sum = 0;
  uint64_t start = rte_get_timer_cycles();*/

  FID fid(pkt);
  path = paths + fid.hash;

  if (fid == path->fid())
    return true;

  path->set_fid(fid);
  /*uint64_t end = rte_get_timer_cycles();
  sum += end - start;
  LOG(INFO) << end - start << " " << ++tot << " " << sum;*/
  return false;
}

void MAT::runMAT(bess::PacketBatch *batch) {
  int cnt  = batch->cnt();

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

