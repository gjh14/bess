#ifndef BESS_PARALLEL_H_
#define BESS_PARALLEL_H_

class Parallel {
 public:
  static const size_t kMaxBurst = 32;
 
  Parallel();
  ~Parallel();
  
  int cnt() { return cnt_; }
  int rank(int x) { return ranks_[x]; }
  int pos(int x) { return poss_[x]; }
  bool result(int x) { return results_[x]; }
  
  void append();
  void start();
  
  static void init();
  
 private:
  int cnt_;
  int ranks_[kMaxBurst];
  int poss_[kMaxBurst];
  bess:Packet *pkts_[kMaxBurst];
  StateAction states_[kMaxBurst];
  bool results_[kMaxBurst];
  
  int lcore_id;
  static bool core[RTE_MAX_LCORE];
  static int act(void *arg);
};

