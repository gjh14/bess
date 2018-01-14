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
    
  void merge(HeadAction action) {
    if(type == DROP)
      return;
  
    if(action.type & DROP) {
      type = DROP;
      return;
    }
    
    if(action.type & MODIFY) {
      type |= MODIFY;
      for(uint32_t i = 0; i < POSNUM; ++i) {
        mask[i] &= action.mask[i];
        value[i] &= (value[i] & action.mask[i]) | action.value[i];
      }
    }
  }
};

struct StateAction{
  typedef enum {READ, WRITE, UNRELATE} TYPE;
  
  TYPE type;
  std::function<bool(bess::Packet *pkt)> action;
};

typedef std::function<void(bess::PacketBatch *unit)> UpdateAction;

class Path {
 public:
  Path();
  ~Path();

  void appendRule(Module *module, HeadAction head, StateAction state, UpdateAction update);
  void handlePkt(bess::PacketBatch *unit);
  
  void set_port(Module *port);
  void set_fid(std::string *fid);
  const std::string *fid();
  
 private:
  std::string *fid_;
  Module *port_;
  
  std::vector<HeadAction> heads;
  HeadAction total;
  
  std::vector<Module *> modules;
  std::vector<StateAction> states;
  std::vector<UpdateAction> updates;
  
  void handleHead(bess::Packet *pkt);
};

#endif

