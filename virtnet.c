#include "defs.h"
#include "mmu.h"
#include "types.h"
#include "pci.h"
#include "virtio.h"
#include "virtnet.h"
#include "nic.h"

/*
 * Read the network device MAC address from the device specific configuration
 * capabality.
 *
 */
void init_macaddr(struct virtio_device *dev)
{
    struct pci_device* pci = dev->pci;

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

void virtionet_send(void* driver, uint8_t *packet, uint16_t length)
{
    struct virtio_device* dev = (struct virtio_device*)driver;
    struct virt_queue* vq = &dev->queues[1]; // Tx queue

    uint32 virt_size = length + sizeof(struct virtio_net_hdr);

    struct virtq_desc desc[2];
    struct virtio_net_hdr net;

    net.flags = VIRTIO_NET_HDR_F_NEEDS_CSUM;
    net.gso_type = VIRTIO_NET_HDR_GSO_NONE;
    net.csum_start = 0;
    net.csum_offset = virt_size;

    desc[0].len = sizeof(struct virtio_net_hdr);
    desc[0].flags = 0;
    desc[0].addr = &net;
    desc[1].addr = packet;
    desc[1].len = virt_size;
    desc[1].flags = 0;

    virtio_fill_buffer(dev, 1, &desc, 2);
}

void virtionet_recv(void* driver, uint8_t* packet, uint16_t length)
{}

int virtio_init(int pci_fd)
{
    int virt_fd = alloc_virt_dev(pci_fd);
    conf_virtio_mem(virt_fd, &virtionet_negotiate);
    init_macaddr(&virtdevs[virt_fd]);

    struct virtio_device* dev = &virtdevs[virt_fd];

    struct virt_queue* rx = &dev->queues[0]; // Receive
    struct virt_queue* tx = &dev->queues[1]; // Send

    if (rx->buffers == 0 || tx->buffers == 0) {
        cprintf("unable to initialize virtio device\n");
        return virt_fd;
    }

    rx->chunk_size = FRAME_SIZE;
    rx->available->idx = 0;
    virtio_enable_intr(rx);

    // Fill up receive queue so that we can receive data.
    struct virtq_desc buffer;
    buffer.len = FRAME_SIZE;
    buffer.flags = VIRTQ_DESC_F_WRITE;
    buffer.addr = 0; // This should be the physical address of the buffer.

    cprintf("Sending buffers to device\n");
    for (int i = 0; i < 10; i++) {
        virtio_fill_buffer(dev, 0, &buffer, 1);
    }

    tx->chunk_size = FRAME_SIZE;
    tx->available->idx = 0;
    notify_queue(dev, tx->num);

    picenable(dev->irq);
    ioapicenable(dev->irq, 0);
    ioapicenable(dev->irq, 1);

    struct nic_device nic = { .driver = dev, .mac_addr = dev->macaddr, .send_packet = &virtionet_send, .recv_packet = &virtionet_recv };

    register_device(nic);

    return virt_fd;
}

