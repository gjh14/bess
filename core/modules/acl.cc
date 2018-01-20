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

#include "acl.h"

#include <rte_cycles.h>

#include "../utils/ether.h"
#include "../utils/ip.h"
#include "../utils/udp.h"

const Commands ACL::cmds = {
    {"add", "ACLArg", MODULE_CMD_FUNC(&ACL::CommandAdd),
     Command::Command::THREAD_UNSAFE},
    {"clear", "EmptyArg", MODULE_CMD_FUNC(&ACL::CommandClear),
     Command::Command::THREAD_UNSAFE}};

ACL::ACL() : Module() {
  max_allowed_workers_ = Worker::kMaxWorkers;
  time = 0;
  sfunc = [&](bess::Packet *pkt[[maybe_unused]], void *arg) ->bool {
      ACLArg *check = (ACLArg*)arg;
      return *check != time;
    };

  memset(cache, 0, sizeof(cache));
  memset(result, 0, sizeof(result));
}

CommandResponse ACL::Init(const bess::pb::ACLArg &arg) {
  for (const auto &rule : arg.rules()) {
    ACLRule new_rule = {
        .src_ip = Ipv4Prefix(rule.src_ip()),
        .dst_ip = Ipv4Prefix(rule.dst_ip()),
        .src_port = be16_t(static_cast<uint16_t>(rule.src_port())),
        .dst_port = be16_t(static_cast<uint16_t>(rule.dst_port())),
        .drop = rule.drop()};
    rules_.push_back(new_rule);
  }
  ++time;
  return CommandSuccess();
}

CommandResponse ACL::CommandAdd(const bess::pb::ACLArg &arg) {
  Init(arg);
  return CommandSuccess();
}

CommandResponse ACL::CommandClear(const bess::pb::EmptyArg &) {
  rules_.clear();
  ++time;
  return CommandSuccess();
}

void ACL::ProcessBatch(bess::PacketBatch *batch) {
  static uint64_t to = 0, so = 0, tc = 0, sc = 0;

  using bess::utils::Ethernet;
  using bess::utils::Ipv4;
  using bess::utils::Udp;

  gate_idx_t out_gates[bess::PacketBatch::kMaxBurst];
  gate_idx_t incoming_gate = get_igate();

  int cnt = batch->cnt();
  for (int i = 0; i < cnt; i++) {
    uint64_t start = rte_get_timer_cycles();

    bess::Packet *pkt = batch->pkts()[i];
    Path *path = batch->path(i);
    
    Ethernet *eth = pkt->head_data<Ethernet *>();
    Ipv4 *ip = reinterpret_cast<Ipv4 *>(eth + 1);
    size_t ip_bytes = ip->header_length << 2;
    Udp *udp =
        reinterpret_cast<Udp *>(reinterpret_cast<uint8_t *>(ip) + ip_bytes);

    uint64_t hash = 0;
    uint8_t fid[Path::FIDLEN];
    MAT::getFID(pkt, hash, fid);
    if (!memcmp(fid, cache[hash], Path::FIDLEN)) {
      out_gates[i] = result[hash] ? DROP_GATE : incoming_gate;

      uint64_t end = rte_get_timer_cycles();
      tc += 1;
      sc += end - start;
      continue;
    }

    out_gates[i] = DROP_GATE;  // By default, drop unmatched packets

    for (const auto &rule : rules_) {
      if (rule.Match(ip->src, ip->dst, udp->src_port, udp->dst_port)) {
        if (!rule.drop) {
          out_gates[i] = incoming_gate;
        }
        break;  // Stop matching other rules
      }
    }

    memcpy(cache[hash], fid, Path::FIDLEN);
    result[hash] = out_gates[i] == DROP_GATE;

    if (path != nullptr) {
      HeadAction *head = nullptr;
      StateAction *state = nullptr;
      path->appendRule(this, head, state);
      if (out_gates[i] == DROP_GATE)
        head->type = HeadAction::DROP;
      state->type = StateAction::UNRELATE;
      state->action = sfunc;
      ACLArg *arg = (ACLArg *)malloc(sizeof(ACLArg));
      *arg = time;
      state->arg = (void *)arg;
    }

    uint64_t end = rte_get_timer_cycles();
    to += 1;
    so += end - start;
  }

  LOG(INFO) << to << " " << so << " " << tc << " " << sc;

  RunSplit(out_gates, batch);
}

ADD_MODULE(ACL, "acl", "ACL module from NetBricks")

