#ifndef BESS_MODULES_ACL_H_
#define BESS_MODULES_ACL_H_

#include <vector>

#include "../module.h"
#include "../pb/module_msg.pb.h"
#include "../utils/endian.h"

class Maglev final: public Module{
 public:
  static const gate_idx_t kNumOGates = MAX_GATES;

  static const Commands cmds;
  Maglev() : Module() { max_allowed_workers_ = Worker::kMaxWorkers; }
  
  CommandResponse Init(const bess::pb::MaglevArg &arg);
  
  void ProcessBatch(bess::PacketBatch *batch) override;
  
  CommandResponse CommandModify(const bess::pb::MaglevCommandArg &arg);
  CommandResponse CommandClear(const bess::pb::EmptyArg &arg);
  
 private:
  using be32_t = bess::utils::be32_t;
  using be16_t = bess::utils::be16_t;

  uint32_t size;
  uint32_t ngates; 
 
  std::vector<bool> is_valid;
  std::vector<std::vector<uint32_t>> shuffle_list;
  std::vector<uint32_t> hash_table;

  uint32_t hash(uint8_t protocol, be32_t src_ip, be16_t src_port, be32_t dst_ip, be16_t dst_port);
  void build();
};
#endif

