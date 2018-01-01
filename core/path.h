#ifndef BESS_PATH_H_
#define BESS_PATH_H_

#include "packet.h"

struct HeadAction {
  const TYPENUM =  2;
  const POSNUM = 5;
  typedef enum {DROP = 0, MODIFY = 1} TYPE;
  typedef enum {PROTO = 0, SRC_IP, SRC_PORT, DST_IP, DST_PORT} POSITION;
	
  uint32_t type;
  uint32_t pos;
  uint32_t value[POSNUM];
  
  HeadAction(){
  	type = pos = 0;
  }
  
  void merge(HeadAction action){
    if(type & DROP)
      return;
    if(action.type & DROP){
      type = DROP;
      return;
    }
    if(action.type & MODIFY)
        for(int i = 0; i < POSNUM; ++i)
          if(action.pos & (1 << i)){
            pos |= 1 << i;
            value[i] = action.value[i];
          }
  }
};

struct StateAction{
  typedef enum {READ, WRITE, UNRELATED} TYPE;
  
  TYPE type;
  bool (*action)(bess::Packet *pkt);
}

typedef void (*UpdateAction)(HeadAction *, StateAction *);

class Path {
 public:
  Path();
  void appendRule(HeadAction head, StateAction state, UpdateAction update);
  void handlePkt(bess:Packet *pkt);
  
 private:
  std::vector<HeadAction> heads;
  std::vector<StateAction> states;
  std::vector<UpdateAction> updates;
  HeadAction totalAction;
  
  void handleHead(bess:Packet *pkt);
};

#endif

