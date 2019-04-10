#include <stddef.h>
#include "types.h"
#include "mmu.h"
#include "memlayout.h"
#include "defs.h"
#include "pci.h"
#include "virtio.h"


#define VIRTIO_F_VERSION_1		32

/*
* Table of all virtio devices in the machine
*
* This is populated by alloc_virt_dev
*/
struct virtio_device virtdevs[NVIRTIO] = {0};

/*
 * Allocates a virtio device.
 */
int alloc_virt_dev(int pci_fd)
{
  struct virtio_device* vdev;
  struct pci_device* dev = &pcidevs[pci_fd];
  int index = -1;

  for (vdev = virtdevs; vdev < &virtdevs[NVIRTIO]; vdev++) {
      index += 1;
      if (vdev->state == VIRT_FREE) {
          goto found;
      }
  }

  return -1;

found:

  vdev->state = VIRT_USED;
  vdev->base = dev->membase;
  vdev->size = dev->reg_size[4];
  vdev->irq = dev->irq_line;
  vdev->iobase = dev->iobase;
  vdev->pci = dev;
  vdev->cfg = (struct virtio_pci_common_cfg*)vdev->base;

  return index;
}

int setup_virtqueue(struct virtio_device* dev, uint16 queue)
{
    // We are configuring queue number `queue`
    dev->cfg->queue_select = queue;

    uint16 size = dev->cfg->queue_size;

    cprintf("Queue: %d size: %d\n", queue, size);

    // size 0 implies the queue doesn't exist.
    if (size == 0) {
        return -1;
    }

    struct virt_queue* virtq = &dev->queues[queue];
    virtq->queue_size = size;
    virtq->num = queue;

    // http://docs.oasis-open.org/virtio/virtio/v1.0/cs04/virtio-v1.0-cs04.html#x1-220004
    uint32 desc_ring_size = 16 * size;
    uint32 avail_ring_size = 6 + 2 * (size);
    uint32 used_ring_size = 6 + 8 * (size);
    uint32 total = desc_ring_size + avail_ring_size + used_ring_size;

    uint32 count = PGROUNDUP(total);

    void* buf = kalloc();

    // first call to kalloc() allocates a page.
    int i = PGSIZE;

    while (i < count) {
        kalloc();
        i += PGSIZE;
    }

    // Since buf is a pointer to the starting of a page, this will be
    // a multiple of two. Plus the sizes of the rings are themselves multiples
    // of two, so the macro invocation shouldn't be an issue.
    virtq->buffers = (struct virtq_desc*)ALIGN(&buf, 16);
    virtq->available = (struct virtq_avail*)(&buf[desc_ring_size], 2);
    virtq->used = (struct virtq_used*)ALIGN(&buf[desc_ring_size + avail_ring_size], 4);
    virtq->next_buffer = 0;
    virtq->lock = 0;

    dev->cfg->queue_desc = V2P(buf);
    dev->cfg->queue_avail = V2P(&virtq->available);
    dev->cfg->queue_used = V2P(&virtq->used);
}

/*
 * Configures a virtio device.
 *
 * http://docs.oasis-open.org/virtio/virtio/v1.0/cs04/virtio-v1.0-cs04.html#x1-490001
 *
 * This functions accepts a function pointer to a negotiate function. This
 * means that different virtio devices can customize feature negotiation.
 */
int conf_virtio_mem(int fd, void (*negotiate)(uint32 *features))
{
    struct virtio_device *dev = &virtdevs[fd];

    // First reset the device
    uint8 flag = VIRTIO_STATUS_RESET;
    dev->cfg->device_status = flag;

    // Then acknowledge the device
    flag |= VIRTIO_STATUS_ACKNOWLEDGE;
    dev->cfg->device_status = flag;

    // Let the device know that we can drive it
    flag |= VIRTIO_STATUS_DRIVER;
    dev->cfg->device_status = flag;

    uint32 features = dev->cfg->device_feature;

    negotiate(&features);

    dev->cfg->driver_feature = features;

    flag |= VIRTIO_STATUS_FEATURES_OK;
    dev->cfg->device_status = flag;

    uint8 val = dev->cfg->device_status;

    // cprintf("got val: %d\n", val);

    if ((val & VIRTIO_STATUS_FEATURES_OK) == 0) {
        return -1;
    }

    // Only support 4 virt queues.
    for (int i = 0; i < 4; i++) {
        setup_virtqueue(dev, i);
    }

    flag |= VIRTIO_STATUS_DRIVER_OK;
    dev->cfg->device_status = flag;

    val = dev->cfg->device_status;

    return 0;
}
