#ifndef BESS_MAT_H_
#define BESS_MAT_H_

#include <string>
#include <unordered_map>

#include "path.h"

class MAT {
 public:
  MAT();
  ~MAT();
  bool checkMAT(bess::PacketBatch *unit);

 private:
  std::unordered_map<std::string, Path *, std::hash<std::string>, std::equal_to<std::string>> mat;
  
  void appendData(std::string &fid, uint32_t num, int len);
  void getFID(bess::Packet *pkt, std::string &fid);
};

#endif

