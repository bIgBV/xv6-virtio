#include "pci.h"
#include "types.h"


struct net_conf
{
  uint32 ip;
  uint32 subnetmask;
  uint32 gateway;
  uint32 vlan;
};

struct net_card
{
  enum { NET_FREE, NET_USED } state;
  struct net_conf conf;
  void* device;
  int (*write)(char*, int);
  int (*read)(char*, int);
};

#define NCARDS                         10

// Array of virtio devices
extern struct net_card netcards[NCARDS];

