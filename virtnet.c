#include "defs.h"
#include "types.h"
#include "pci.h"
#include "virtio.h"

/*
 * Read the network device MAC address from the device specific configuration
 * capabality.
 *
 */
void init_macaddr(struct virtio_device *dev)
{
    struct pci_device* pci = dev->pci;

    uint8 cap_pointer = pci->capabalities[VIRTIO_PCI_CAP_PCI_CFG];

    cprintf("Mac addr: ");

    for (int i = 0; i < 6; i++) {
        // setup_window(pci, 1, bar, offset + i);
        // dev->macaddr[i] = confread8(dev, window_off);
        dev->macaddr[i] = inb(dev->iobase+VIRTIO_DEV_SPECIFIC_OFF + i);
        cprintf("%x:", dev->macaddr[i]);
    }

    cprintf("\n");

}

/*
 * Feature negotiation for a network device
 *
 * We are offloading checksuming to the device
 */
void virtionet_negotiate(uint32 *features)
{
    // do not use control queue
    DISABLE_FEATURE(*features, VIRTIO_NET_F_CTRL_VQ);

    DISABLE_FEATURE(*features, VIRTIO_NET_F_GUEST_TSO4);
    DISABLE_FEATURE(*features, VIRTIO_NET_F_GUEST_TSO6);
    DISABLE_FEATURE(*features, VIRTIO_NET_F_GUEST_UFO);
    DISABLE_FEATURE(*features, VIRTIO_NET_F_MRG_RXBUF);
    DISABLE_FEATURE(*features, VIRTIO_F_EVENT_IDX);

    ENABLE_FEATURE(*features, VIRTIO_NET_F_CSUM);

  // Only enable MAC if it is offered by the device
  if (*features & VIRTIO_NET_F_MAC) {
      ENABLE_FEATURE(*features, VIRTIO_NET_F_MAC);
  }
}


int virtio_init(int pci_fd)
{
    int virt_fd = alloc_virt_dev(pci_fd);
    conf_virtio_mem(virt_fd, &virtionet_negotiate);
    init_macaddr(&virtdevs[virt_fd]);

    struct virtio_device* dev = &virtdevs[virt_fd];

    struct virt_queue* rx = dev->queues[0]; // Receive
    struct virt_queue* tx = dev->queues[1]; // Send

    if (rx->buffers == 0 || tx->buffers == 0) {
        cprintf("unable to initialize virtio device\n");
        return virt_fd;
    }

    void* buf = kalloc();

    uint32 count = 64 * PGSIZE;

    int i = PGSIZE;
    while (i < count) {
        kalloc();
        i += PGSIZE;
    }

    rx->buffer = (uint8*)buf;
    rx->chunk_size = FRAME_SIZE;
    rx->available->index = 0;
    virtio_enable_intr(rx);

    // Fill up receive queue so that we can receive data.

    return virt_fd;
}
