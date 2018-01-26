#ifndef BESS_MODULES_MAGLEV_H_
#define BESS_MODULES_MAGLEV_H_

#include <vector>

#include "../module.h"
#include "../pb/module_msg.pb.h"
#include "../utils/endian.h"

class Maglev final: public Module {
 using be32_t = bess::utils::be32_t;
 using be16_t = bess::utils::be16_t;
 
 public:
   struct MaglevDst {
    uint32_t dst_ip;
    uint16_t dst_port;
  };
  
  static const Commands cmds;
  
  Maglev();
 
  CommandResponse Init(const bess::pb::MaglevArg &arg);
  CommandResponse CommandModify(const bess::pb::MaglevCommandArg &arg);
  CommandResponse CommandClear(const bess::pb::EmptyArg &arg);
  
  void ProcessBatch(bess::PacketBatch *batch) override;
  
 private:
  struct MaglevArg{
    uint32_t value;
    uint32_t gate;
  };
  StateAction::FUNC sfunc;
 
  uint32_t size;
  uint32_t ndsts;
  std::vector<MaglevDst> dsts;
 
  std::vector<bool> is_valid;
  std::vector<std::vector<uint32_t>> shuffle_list;
  std::vector<uint32_t> hash_table;

  uint32_t hash(uint8_t protocol, uint32_t src_ip, uint16_t src_port, uint32_t dst_ip, uint16_t dst_port);
  void build();

  uint8_t cache[MAT::MAX_PATHS][Path::FIDLEN];
};

#endif
