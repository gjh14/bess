#ifndef BESS_MAT_H_
#define BESS_MAT_H_

#include <string>
#include <unordered_set>
#include <vector>

#include "path.h"

class MAT {
 public:
  static const uint32_t MAX_PATHS = 999983;
 
  MAT();

  bool checkMAT(bess::Packet *pkt, Path *&path);
  void runMAT(bess::PacketBatch *batch);

  void add_module(Module *module);
  void add_port(Module *port);

  static void getFID(bess::Packet *pkt, uint64_t &hash, uint8_t *fid);
  static int delay(int x);

 private: 
  Path paths[MAX_PATHS];
  std::vector<Module *> modules;
  std::vector<Module *> ports;

  static void append(uint64_t &hash, uint8_t *fid, uint32_t num, int len, int pos);
};

#endif

