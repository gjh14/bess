#ifndef BESS_MAT_H_
#define BESS_MAT_H_

#include <string>
#include <unordered_map>

#include "path.h"

class MAT {
 public:
  MAT();
  ~MAT();
  void checkMat(bess::Packet *pkt)

 private:
  std::unordered_map<std:string, Path*> mat;
  
  void appendData(std::tring *fid, uint32_t num, uint32_t len);
  std::string* getFID(bess:Packet pkt)
};

#endif

