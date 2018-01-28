#include "monitor.h" 

#include <algorithm>
#include <rte_cycles.h>


const Commands Monitor::cmds = {
    {"clear", "EmptyArg", MODULE_CMD_FUNC(&Monitor::CommandClear),
     Command::Command::THREAD_UNSAFE}};

Monitor::Monitor() : Module() {
  sfunc = [&](bess::Packet *pkt[[maybe_unused]], void *arg[[maybe_unused]]) ->bool {
      int delay = 1, p = *(int*)pkt; 
      for(int i = 0; i < 1125; ++i)
        delay *= p ^ (p & 1);
      return delay;
    }; 
}

CommandResponse Monitor::Init(const bess::pb::EmptyArg &){
  return CommandSuccess();
}

CommandResponse Monitor::CommandClear(const bess::pb::EmptyArg &){
  return CommandSuccess();
}

void Monitor::ProcessBatch(bess::PacketBatch *batch) {
  /* static uint64_t a = 0, b = 0;
  uint64_t c = rte_get_timer_cycles(); */

  int cnt = batch->cnt();

  for (int i = 0; i < cnt; i++) {
    // bess::Packet *pkt = batch->pkts()[i];
    // MAT::mark(pkt);
    
    Path *path = batch->path(i);

    int delay = 1;
    for(int j = 0; j < 1125; ++j) // 1850
      delay *= i ^ (i & 1);
    i += delay;

    HeadAction *head = nullptr;
    StateAction *state = nullptr;
    if (path != nullptr) {
      path->appendRule(this, head, state);
      state->action = sfunc; // nullptr;
      state->arg = nullptr;
    }
    
    // MAT::stat(pkt);
  }

  /* int64_t d = rte_get_timer_cycles();
  a += cnt;
  b += d - c;
  LOG(INFO) << cnt << " " << d - c << " " << a << " " << b; */

  RunNextModule(batch);
}

ADD_MODULE(Monitor, "monitor", "Empty")
