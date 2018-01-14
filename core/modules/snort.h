#ifndef BESS_MODULES_SNORT_H_
#define BESS_MODULES_SNORT_H_

#include <arpa/inet.h>
#include <ctype.h>
#include <math.h>
#include <netdb.h>
#include <sys/socket.h>

#include "../module.h"
#include "../pb/module_msg.pb.h"

class Snort final : public Module {
 public:
  struct Rule {
    int rule_num;        /* for debug purposes */
    int type;            /* alert, log, or pass */
    u_long sip;          /* src IP */
    u_long smask;        /* src netmask */
    u_long dip;          /* dest IP */
    u_long dmask;        /* dest netmask */
    int not_sp_flag;     /* not implemented yet... */
     u_short hsp;         /* hi src port */
    u_short lsp;         /* lo src port */
    int not_dp_flag;     /* not implemented yet... */
    u_short hdp;         /* hi dest port */
    u_short ldp;         /* lo dest port */
    u_char tcp_flags;    /* self explainatory */
    int check_tcp_flags; /* program flag */
    int proto;           /* protocol */
    int pattern_match_flag; /* program flag */
    int icmp_type;       /* ICMP type */
    int use_icmp_type;
    int icmp_code;       /* ICMP code */
    int use_icmp_code;
    int ttl;             /* TTL value */ 
    u_char flags;        /* control flags */
    char *message;       /* alert message */
    u_int pattern_size;  /* size of app layer pattern */
    char *pattern_buf;   /* app layer pattern to match on */
    Rule *next;  /* ptr to the next rule */
  };

  struct PrintIP {
    u_char timestamp[64];   /* packet timestamp */
    u_char eth_src[6];
    u_char eth_dst[6];
    u_short eth_type;
    u_int eth_len;
    u_char saddr[16];       /* src ip addr */
    u_char daddr[16];       /* dst ip addr */
    u_short sport;          /* src port */
    u_short dport;          /* dst port */
    char sport_name[16];    /* src port name */
    char dport_name[16];    /* dst port name */
    u_long seq;             /* TCP sequence number */
    u_long ack;             /* TCP acknowledge number */
    u_char flags;           /* TCP flags */
    char proto[5];          /* datagram protocol */
    u_long win;             /* IP window size */
    u_char ttl;             /* IP time to live */
    u_char tos;             /* IP type of service */
    u_char ip_df;           /* IP don't fragment flag */
    u_char ip_mf;           /* IP more fragments flag */
    u_short frag_id;        /* IP fragment ID field */
    u_short frag_off;       /* IP fragment offset field */
    u_short udp_len;        /* UDP app layer length field */
    u_short icmp_type;
    u_short icmp_code;
    u_char icmp_str[64];    /* ICMP type/code string */
    u_short icmp_id;        /* ICMP (ping) id number */
    u_short icmp_seqno;     /* ICMP (ping) sequence number */
    u_int dsize;            /* packet data size */
    int IPO_flag;           /* IP Options are present */
    u_char IPO_Str[160];     /* IP Options string */
    int TO_flag;            /* TCP Options flag */
    char TO_Str[160];    /* TCP option string */
    char *data;             /* ptr to the packet data (app layer) */
  }; 

  struct NetData {
    unsigned long sip;
    unsigned long dip;
    unsigned short sp;
    unsigned short dp;
    unsigned int proto;
    unsigned char tcp_flags;
  };
 
  Snort();
  ~Snort();
  
  static const Commands cmds;
  
  CommandResponse Init(const bess::pb::SnortArg &arg);
  CommandResponse CommandAdd(const bess::pb::SnortArg &arg);
  CommandResponse CommandClear(const bess::pb::EmptyArg &arg);
  
  void ProcessBatch(bess::PacketBatch *batch) override;
  
 private:
 
  int file_line;      /* current line being processed in the rules file */
  int rule_count;
  Rule *current;      /* util ptr for the current rule */
  Rule *PassList;     /* List of Pass Rules */
  Rule *LogList;      /* List of Log Rules */
  Rule *AlertList;    /* List of Alert Rules */
  PrintIP pip;

  void ParsePattern(char *rule);
  void ParseRule(char *rule);
  void ParseRulesFile(const char *file);
  void ParseRuleOptions(char *rule);
  int ParsePort(char *rule_port, u_short *hi_port, u_short *lo_port, char *proto, int *not_flag);
  int ParseIP(char *addr, u_long *ip_addr, u_long *netmask);
  int WhichProto(char *proto_str);
  void CreateRuleNode(int type);
  int RuleType(char *func);
  int ConvPort(char *port, char *proto);
  char **mSplit(char *str, char *sep, int max_strs, int *toks, char meta);
  void ParseMessage(char *msg);
  void ParseFlags(char *rule);
  void ParseItype(char *number);
  void ParseIcode(char *type);
  int strip(char *data);
  int CheckRules(Rule *list, NetData net);
  int mSearch( char *buf, int blen, char *ptrn, int plen);

  void clear();
  bool snort_pktcon(struct bess::Packet *pkt, NetData& net);

  StateAction::FUNC sfunc;
};

#endif

