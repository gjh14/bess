#ifndef BESS_MODULES_MONITOR_H_
#define BESS_MODULES_MONITOR_H_

#include <vector>

#include "../module.h"
#include "../pb/module_msg.pb.h"

class Monitor final: public Module {
public:
  static const Commands cmds;
  
  Monitor();
 
  CommandResponse Init(const bess::pb::EmptyArg &arg);
  CommandResponse CommandClear(const bess::pb::EmptyArg &arg);
  
  void ProcessBatch(bess::PacketBatch *batch) override;

 private:
   StateAction::FUNC sfunc;

};

#endif
