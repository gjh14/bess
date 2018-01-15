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

class Module;

struct HeadAction {
  static const uint32_t TYPENUM =  2;
  static const uint32_t POSNUM = 5;
  typedef enum {DROP = 1, MODIFY = 2} TYPE;
  typedef enum {PROTO = 0, SRC_IP, SRC_PORT, DST_IP, DST_PORT} POSITION;
	
  uint32_t type;
  uint32_t mask[POSNUM];
  uint32_t value[POSNUM];
  
  HeadAction() {
    clear();
  }
  
  void clear() {
    type = 0;
    memset(mask, -1, sizeof(mask));
    memset(value, 0, sizeof(value)); 
  }
  
  void modify(uint32_t _pos, uint32_t _value) {
   if(type == DROP)
      return;
    type |= MODIFY;
    mask[_pos] = 0;
    value[_pos] = _value;
  }
  
  void merge(HeadAction *action);
};

struct StateAction{
  typedef enum {READ, WRITE, UNRELATE} TYPE;
  typedef std::function<bool(bess::Packet *pkt, void *arg)> FUNC;
  
  StateAction() : arg(nullptr) {}
  ~StateAction() { free(arg); }
 
  TYPE type;
  FUNC action;
  void *arg;
};

class Path {
 public:
  friend class MAT;
  
  Path() : fid_(nullptr), port_(nullptr) {}
  ~Path();

  void appendRule(Module *module, HeadAction *head, StateAction state);
  void handlePkt(bess::Packet *pkt);
  void rehandle(int pos, bess::PacketBatch *unit);
  
  void set_port(Module *port) { port_ = port; }
  Module* port() { return port_; }
  void set_fid(std::string *fid);
  const std::string *fid() { return fid_; }
  
 private:
  std::string *fid_;
  Module *port_;
  
  std::vector<HeadAction *> heads;
  HeadAction total;
  
  std::vector<Module *> modules;
  std::vector<StateAction> states;
  
  void handleHead(bess::Packet *pkt);
};

#endif

