#ifndef BESS_MAT_H_
#define BESS_MAT_H_

#include <map>
#include <string>
//#include <unordered_map>

#include "path.h"

class MAT {
 public:
  MAT();
  ~MAT();
  bool checkMAT(bess::Packet *pkt);

 private:
//  std::unordered_map<std:string, Path *> mat;
  std::map<std::string, Path *> mat;
  
  void appendData(std::string *fid, uint32_t num, int len);
  std::string* getFID(bess::Packet *pkt);
};

#endif

