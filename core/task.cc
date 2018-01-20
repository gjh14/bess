// Copyright (c) 2014-2017, The Regents of the University of California.
// Copyright (c) 2016-2017, Nefeli Networks, Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// * Neither the names of the copyright holders nor the names of their
// contributors may be used to endorse or promote products derived from this
// software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include "task.h"

#include <rte_cycles.h>
#include <unordered_set>

#include "module.h"

// Called when the leaf that owns this task is destroyed.
void Task::Detach() {
  c_ = nullptr;
}

// Called when the leaf that owns this task is created.
void Task::Attach(bess::LeafTrafficClass *c) {
  c_ = c;
}

struct task_result Task::operator()(void) {
  return module_->RunTask(this, arg_);
}

/*!
 * Compute constraints for the pipeline starting at this task.
 */
placement_constraint Task::GetSocketConstraints() const {
  if (module_) {
    std::unordered_set<const Module *> visited;
    return module_->ComputePlacementConstraints(&visited);
  } else {
    return UNCONSTRAINED_SOCKET;
  }
}

/*!
 * Add a worker to the set of workers that call this task.
 */
void Task::AddActiveWorker(int wid) const {
  if (module_) {
    module_->AddActiveWorker(wid, c_->task());
  }
}

/*!
 * GMAT
 */
void Task::collect(bess::PacketBatch *batch, Module *module) {
  module->RunNextModule(batch);

  // static uint64_t tot = 0, sum = 0;
  // uint64_t start = rte_get_timer_cycles();

  /* bess::PacketBatch hits;
  hits.clear();
  bess::PacketBatch unhits;
  unhits.clear();
  
  int cnt = batch->cnt();
  for (int i = 0; i < cnt; ++i) {
    bess::Packet *pkt = batch->pkts()[i];
    Path *path = nullptr;
    bool flag = gmat.checkMAT(pkt, path);

    for (int j = 0; j < hits.cnt(); ++j)
      if (path == hits.path(j)){
        gmat.runMAT(&hits);
        hits.clear();
      }
    for (int j = 0; j < unhits.cnt(); ++j)
      if (path == unhits.path(j)) {
        module->RunNextModule(&unhits);
        unhits.clear();
      }
    if(flag)
      hits.add(pkt, path);
    else{
      unhits.add(pkt, path);
      path->clear();
    }
  }

  // uint64_t end = rte_get_timer_cycles();
  // tot += cnt;
  // sum += end - start;
  // LOG(INFO) << cnt << " " << end - start << " " << tot << " " << sum;

  if(hits.cnt())
    gmat.runMAT(&hits);
  if(unhits.cnt())
    module->RunNextModule(&unhits); */
}

