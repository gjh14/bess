#ifndef BESS_PARALLEL_H_
#define BESS_PARALLEL_H_

#include <rte_config.h>

#include "path.h"

class Parallel {
 public:
  static const size_t kMaxBurst = 32;
 
  Parallel();
  ~Parallel();
 
  void remote();
 
  int cnt() { return cnt_; }
  int rank(int x) { return ranks_[x]; }
  int pos(int x) { return poss_[x]; }
  bool result(int x) { return results_[x]; }

  void append(int rank, int pos, bess::Packet *pkt, StateAction *state); 
  void start();
  void join();
  
  static void init();
  
 private:
  bool remote_;
  bool run_;

  int cnt_;
  int ranks_[kMaxBurst];
  int poss_[kMaxBurst];
  bess::Packet *pkts_[kMaxBurst];
  StateAction *states_[kMaxBurst];
  bool results_[kMaxBurst];
  
  int lcore_id;
  static bool core[RTE_MAX_LCORE + 1];
  static int act(void *arg);
};

#endif
