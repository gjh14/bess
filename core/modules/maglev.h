#ifndef BESS_MODULES_ACL_H_
#define BESS_MODULES_ACL_H_

#include <vector>

#include "../module.h"
#include "../pb/module_msg.pb.h"
#include "../utils/ip.h"

class Maglev final: public Module{
 public:  
  static const Commands cmds;
  Maglev() : Module() { max_allowed_workers_ = Worker::kMaxWorkers; }
  
  CommandResponse Init(const bess::pb::MaglevArg &arg);
  
  void ProcessBatch(bess::PacketBatch *batch) override;
  
  CommandResponse CommandModify(const bess::pb::MaglevCommandArg &arg);
  CommandResponse CommandClear(const bess::pb::EmptyArg &arg);
  
 private:
  uint32_t size;
  uint32_t ngates; 
 
  std::vector<bool> is_valid;
  std::vector<std::vector<uint32_t>> shuffle_list;
  std::vector<uint32_t> hash_table;
};
#endif

