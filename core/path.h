#ifndef BESS_PATH_H_
#define BESS_PATH_H_

#include <cstring>
#include <functional> 
#include <vector>

#include "utils/endian.h"

namespace bess{
class Packet;
class PacketBatch;
}

class MAT;
class Module;

struct HeadAction {
  static const uint32_t TYPENUM =  2;
  static const uint32_t POSNUM = 5;
  typedef enum {DROP = 1, MODIFY = 2} TYPE;
  typedef enum {PROTO = 0, SRC_IP, SRC_PORT, DST_IP, DST_PORT} POSITION;
	
  uint32_t type;
  uint32_t mask[POSNUM];
  uint32_t value[POSNUM];
  
  HeadAction() { clear(); }
  
  void clear() {
    type = 0;
    memset(mask, -1, sizeof(mask));
    memset(value, 0, sizeof(value)); 
  }
  
  void modify(uint32_t _pos, uint32_t _value);
  void merge(HeadAction *action);
};

struct StateAction{
  typedef enum {READ, WRITE, UNRELATE} TYPE;
  typedef std::function<bool(bess::Packet *pkt, void *arg)> FUNC;
  
  StateAction() : action(nullptr), arg(nullptr) {}
  ~StateAction() { free(arg); }
 
  TYPE type;
  FUNC action;
  void *arg;
};

class Path {
 public:
  friend class MAT;
   
  static const int MAXLEN = 10;
  static const int FIDLEN = 13;
  
  Path() : port_(nullptr), cnt_(0) { memset(fid_, 0, FIDLEN); }
  ~Path();

  void appendRule(Module *module, HeadAction *&head, StateAction *&state);
  void clear();
  void rehandle(int pos, bess::PacketBatch *unit);
  
  void set_port(Module *port);
  Module* port() const { return port_; }
  void set_fid(const uint8_t *fid);
  const uint8_t *fid() const { return fid_; }
  
 private:
  MAT *mat;
  uint8_t fid_[FIDLEN];
  Module *port_;
  
  int cnt_;
  Module *modules[MAXLEN];
  HeadAction heads[MAXLEN];
  StateAction states[MAXLEN];
  HeadAction total;

  void handleHead(bess::Packet *pkt);
};

#endif

