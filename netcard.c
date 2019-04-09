#include "defs.h"
#include "types.h"
#include "netcard.h"
#include "util.h"
#include "virtio.h"

struct net_card netcards[NCARDS] = {0};

int alloc_netcard()
{
  struct net_card card;
  int index = -1;

  for(int i = 0; i < NCARDS; i++) {
      index += 1;
      card = netcards[i];
      if (card.state == FREE) {
        goto found;
      }
  }

  return -1;

found:
  card.state = USED;
  return index;
}

void net_init()
{
  int fd = alloc_netcard();
  if (fd == -1) {
      panic("Unable to allocate netcard\n");
  }

  struct net_card card = netcards[fd];
  // Device class number for a network card
  int pci_fd = get_pci_dev(0x02);
  int virt_fd = virtio_init(pci_fd);

  card.device = &virtdevs[virt_fd];
}
