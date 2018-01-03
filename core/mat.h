#ifndef BESS_MAT_H_
#define BESS_MAT_H_
s
#include <string>
#include <unordered_map>

#include "path.h"

class MAT {
 public:
  MAT();
  ~MAT();
  bool checkMAT(bess::Packet *pkt);

 private:
  std::unordered_map<std:string, Path *, std::string::Hash, std::string::EqualTo> mat;
  
  void appendData(std::string *fid, uint32_t num, int len);
  std::string* getFID(bess::Packet *pkt);
};

#endif

