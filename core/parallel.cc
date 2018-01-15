#include "paralell.h"

#include <rte_ring.h>

#include "packet.h"


Parallel::Parallel() : cnt_(0), start(false), end(false) {
  for(lcore_id = 0; core[lcore_id]; ++lcore_id);
  core[lcore_id] = 1;
  rte_eal_remote_launch(act, this, lcore_id);
}

~Parallel() {
 end = true;
 rte_eal_wait_lcore(lcore_id);
 core[lcore_id] = 0;
}

void Parallel::append(int rank, int pos, bess::Packet *pkt, StateAction state){
  ranks_[cnt_] = rank;
  poss_[cnt_] = pos;
  pkts_[cnt_] = pkt;
  states_[++cnt_] = state;
}

void Parallel::start(){
  finish = false;
  start = true;
}

bool Paralell:finish(){
  return finish;
}

static int Parallel::act(void *arg){
  Parallel *run = (Parallel *)arg;
  int lcore_id = rte_lcore_id();
  while(1){
    if(run->start){
      int cnt = run->cnt_;
      for(int i = 0; i < cnt; ++i)
        run->result[i] = run->states_[i].action(run->pkts_[i], run->states_[i].arg);
      run->finish = true;
    }
    if(run->end)
      break;
  }
  return 0;
}

static Parallel::init(){
  memset(core, 0, sizeof(core));
  core[rte_lcore_id()] = 1; 
}

