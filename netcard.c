#include "defs.h"
#include "types.h"
#include "netcard.h"
#include "virtio.h"

struct net_card netcards[NCARDS] = {0};

int alloc_netcard()
{
  struct net_card* card;
  int index = -1;

  for (card = netcards; card < &netcards[NCARDS]; card++) {
    index += 1;
    if (card->state == NET_FREE) {
      goto found;
    }
  }

  return -1;

found:
  card->state = NET_USED;
  return index;
}

void net_init()
{
  int fd = alloc_netcard();
  if (fd == -1) {
      panic("Unable to allocate netcard\n");
  }

  struct net_card card = netcards[fd];
  // Device class number for a network card is 0x02
  int pci_fd = get_pci_dev(0x02);
  int virt_fd = virtio_init(pci_fd);

  card.device = &virtdevs[virt_fd];
}
