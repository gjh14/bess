#include "parallel.h"

#include <cstring>
#include <rte_eal.h>
#include <rte_launch.h>
#include <rte_lcore.h>
#include <rte_memory.h>
#include <rte_per_lcore.h>

#include "packet.h"

Parallel::Parallel() : remote_(false), run_(false),  cnt_(0) {}

Parallel::~Parallel() {
  if(remote_) {
    remote_ = false;
    rte_eal_wait_lcore(lcore_id);
    core[lcore_id] = 0;
  }
}

void Parallel::remote(){
  RTE_LCORE_FOREACH_SLAVE(lcore_id){
    LOG(INFO) << "lcore " << lcore_id;
  }
  
  remote_ = true;
  for (lcore_id = 0; core[lcore_id]; ++lcore_id);
  core[lcore_id] = 1;
  LOG(INFO) << "used " << lcore_id;
  rte_eal_remote_launch(act, this, lcore_id);
} 

void Parallel::append(int rank, int pos, bess::Packet *pkt, StateAction *state){
  ranks_[cnt_] = rank;
  poss_[cnt_] = pos;
  pkts_[cnt_] = pkt;
  states_[++cnt_] = state;
}

void Parallel::start(){
  if(remote_){
     // std::atomic_thread_fence(std::memory_order_acquire);
     run_ = true;
     // std::atomic_thread_fence(std::memory_order_release);
     return;
  }

  for(int i = 0; i < cnt_; ++i)
    if(states_[i]->action != nullptr)
      results_[i] = states_[i]->action(pkts_[i], states_[i]->arg);
}

void Parallel::join(){
  while(run_);
  cnt_ = 0;
}

int Parallel::act(void *arg){
  Parallel *master = (Parallel *)arg;
  // int lcore_id = rte_lcore_id();
  while(1){
    if(master->run_) {
      int cnt = master->cnt_;
      LOG(INFO) << "Run " << cnt;
      for(int i = 0; i < cnt; ++i)
        if(master->states_[i] != nullptr)
          master->results_[i] = master->states_[i]->action(master->pkts_[i], master->states_[i]->arg);
      // std::atomic_thread_fence(std::memory_order_acquire);
      master->run_ = false;
      // std::atomic_thread_fence(std::memory_order_release);
    }
    if(!master->remote_)
      break;
  }
  return 0;
}

bool Parallel::core[RTE_MAX_LCORE + 1];

void Parallel::init(){
  memset(core, 0, sizeof(core));
  core[rte_lcore_id()] = 1; 
}
