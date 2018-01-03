#ifndef BESS_PATH_H_
#define BESS_PATH_H_

#include <functional> 
#include <vector>

#include "port.h"
#include "utils/endian."

namespace bess{
class Packet;
}

struct HeadAction {
  static const uint32_t TYPENUM =  2;
  static const uint32_t POSNUM = 5;
  typedef enum {DROP = 1, MODIFY = 2} TYPE;
  typedef enum {PROTO = 0, SRC_IP, SRC_PORT, DST_IP, DST_PORT} POSITION;
	
  uint32_t type;
  uint32_t pos;
  uint32_t value[POSNUM];
  
  HeadAction(){
  	type = pos = 0;
  }
  
  void modify(uint32_t _pos, uint32_t _value){
  	if(type & DROP)
  	  return;
  	type |= MODIFY;
  	pos |= 1 << _pos;
  	value[_pos] = _value;
  }
    
  void merge(HeadAction action){
    if(type & DROP)
      return;
    if(action.type & DROP){
      type = DROP;
      pos = 0;
      return;
    }
    if(action.type & MODIFY)
        for(uint32_t i = 0; i < POSNUM; ++i)
          if(action.pos & (1 << i)){
            pos |= 1 << i;
            value[i] = action.value[i];
          }
  }
};

struct StateAction{
  typedef enum {READ, WRITE, UNRELATE} TYPE;
  
  TYPE type;
  std::function<bool(bess::Packet *pkt)> action;
};

typedef std::function<void(bess::Packet *pkt)> UpdateAction;

class Path {
 public:
  Path();
  void appendRule(HeadAction head, StateAction state, UpdateAction update);
  void handlePkt(bess::PacketBatch *unit);
  
  void set_port(Modlue *module);
  
 private:
  Modue *port;
  
  std::vector<HeadAction> heads;
  std::vector<StateAction> states;
  std::vector<UpdateAction> updates;
  HeadAction total;
  
  void handleHead(bess::Packet *pkt);
};

#endif

