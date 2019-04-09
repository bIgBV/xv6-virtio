#include <stddef.h>
#include "types.h"
#include "defs.h"
#include "pci.h"
#include "virtio.h"
#include "util.h"


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
  struct virtio_device vdev;
  struct pci_device* dev = &pcidevs[pci_fd];
  int index = -1;

  for (int i = 0; i < NVIRTIO; i++) {
    index += 1;
    vdev = virtdevs[i];
    if (vdev.state == FREE) {
      goto found;
    }
  }

  return index;

found:

  vdev.state = USED;
  vdev.base = dev->membase;
  vdev.size = dev->reg_size[4];
  vdev.irq = dev->irq_line;
  vdev.iobase = dev->iobase;
  vdev.pci = dev;
  vdev.cfg = (struct virtio_pci_common_cfg*)vdev.base;

  cprintf("Membase is: %x\n", vdev.base);

  return index;
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

    dev->cfg->queue_select = 0;
    uint16 size0 = dev->cfg->queue_size;

    dev->cfg->queue_select = 1;
    uint16 size1 = dev->cfg->queue_size;

    cprintf("Queue 0 size: %d, queue 1 size: %d\n", size0, size1);

    return 0;
}
