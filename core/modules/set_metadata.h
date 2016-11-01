#ifndef __SET_METADATA_H__
#define __SET_METADATA_H__

#include "../module.h"

typedef struct { char bytes[bess::metadata::kMetadataAttrMaxSize]; } value_t;

struct Attr {
  std::string name;
  value_t value;
  int offset;
  int size;
};

class SetMetadata : public Module {
 public:
  SetMetadata() : Module(), attrs_() {}

  struct snobj *Init(struct snobj *arg);
  pb_error_t Init(const bess::protobuf::SetMetadataArg &arg);

  void ProcessBatch(struct pkt_batch *batch);

  static const gate_idx_t kNumIGates = 1;
  static const gate_idx_t kNumOGates = 1;

  static const Commands<Module> cmds;

 private:
  struct snobj *AddAttrOne(struct snobj *attr);
  pb_error_t AddAttrOne(const bess::protobuf::SetMetadataArg_Attribute &attr);

  std::vector<struct Attr> attrs_;
};

#endif