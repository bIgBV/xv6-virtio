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

void virtio_enable_intr(struct virt_queue* vq)
{
    vq->used->flags |= ~VIRTQ_USED_F_NO_NOTIFY;
}

void virtio_disable_intr(struct virt_queue* vq)
{
    vq->used->flags |= VIRTQ_USED_F_NO_NOTIFY;
}


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
    virtq->next_buffer = 0;

    dev->cfg->queue_desc = V2P(&virtq->buffers);
    dev->cfg->queue_avail = V2P(&virtq->available);
    dev->cfg->queue_used = V2P(&virtq->used);

    cprintf("descriptors: %d available: %d used: %d\n", dev->cfg->queue_desc, dev->cfg->queue_avail, dev->cfg->queue_used);
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

void virtio_fill_buffer(struct virtio_device* dev, uint16 queue, struct virtq_desc* desc_chain, uint32 count)
{
    struct virt_queue* vq = &dev->queues[queue];
    // Do we need to lock this queue?

    uint16 idx = vq->available->idx % vq->queue_size;
    uint16 buf_idx = vq->next_buffer;
    uint16 next_buf;

    uint8* buf = (uint8 *)(&vq->arena[vq->chunk_size * buf_idx]);

    vq->available->ring[idx] = buf_idx;
    for (int i = 0; i < count; i++) {
        cprintf("Filling buffer %d\n", buf_idx);

        next_buf = (buf_idx + 1) % vq->queue_size;

        vq->buffers[buf_idx].flags = desc_chain[i].flags;

        // If this isn't the last buffer, add the chaining flag
        if (i != count -1) {
            vq->buffers[buf_idx].flags |= VIRTQ_DESC_F_NEXT;
        }

        vq->buffers[buf_idx].next = next_buf;
        vq->buffers[buf_idx].len = desc_chain[i].len;
        vq->buffers[buf_idx].addr = V2P(buf);

        if (desc_chain[i].addr != 0) {
            // Only copy if a valid address is present
            memmove(buf, &desc_chain[i].addr, desc_chain[i].len);
        }

        buf += desc_chain[i].len;

        buf_idx = next_buf;
    }

    vq->next_buffer = next_buf;

    // Do we need an mfence here?
    vq->available->idx++;
}
