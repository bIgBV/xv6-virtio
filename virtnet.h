#ifndef __XV6_NETSTACK_VIRTNET_H__
#define __XV6_NETSTACK_VIRTNET_H__

#include "types.h"

#define FRAME_SIZE 1526 // including the net_header

struct virtio_net_hdr {
#define VIRTIO_NET_HDR_F_NEEDS_CSUM    1
    uint8 flags;
#define VIRTIO_NET_HDR_GSO_NONE        0
#define VIRTIO_NET_HDR_GSO_TCPV4       1
#define VIRTIO_NET_HDR_GSO_UDP         3
#define VIRTIO_NET_HDR_GSO_TCPV6       4
#define VIRTIO_NET_HDR_GSO_ECN      0x80
    uint8 gso_type;
    uint16 hdr_len;
    uint16 gso_size;
    uint16 csum_start;
    uint16 csum_offset;
    uint16 num_buffers;
};

#endif
