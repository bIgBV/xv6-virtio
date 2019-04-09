#ifndef __XV6_NETSTACK_PCI_REGISTERS_H__
#define __XV6_NETSTACK_PCI_REGISTERS_H__

unsigned int create_mask(int start, int offset) {

    return ((1 << offset) - 1) << start;
}


#define PCI_CAP_REG                 0x34
// The bottom two bits are reserved and have to be masked off
#define PCI_CAP_MASK                0xfc

#define PCI_CAP_TYPE                0
#define PCI_CAP_NEXT                1
#define PCI_CAP_CFG_TYPE            3
#define PCI_CAP_BAR                 4
#define PCI_CAP_OFF                 8 // 05 byte is padding
#define PCI_CAP_POINTER(reg) \
    (reg & create_mask(0, 8))

// The vendor ID is the last 16 bits of the ID register
#define PCI_VENDOR_ID(value) \
    (value & create_mask(0, 16))

// The device ID if the high 16 bits of the ID register
#define PCI_DEVICE_ID(value) \
    ((value & create_mask(16, 16)) >> 16)
/*
 * PCI config space BAR offset
 */
#define PCI_CFG_BAR_OFF             0x10
#define	PCI_CFG_BAR_END             0x28

/**
 * author: Anmol Vatsa <anvatsa@cs.utah.edu>
 *
 * PCI Register definitions and convenience macros to access them
 * http://wiki.osdev.org/PCI#PCI_Device_Structure
 */

/*
 * Device identification register; contains a vendor ID and a device ID.
 */
#define	PCI_ID_REG			0x00

#define	PCI_VENDOR_SHIFT			0
#define	PCI_VENDOR_MASK				0xffff
#define	PCI_VENDOR(id) \
	    (((id) >> PCI_VENDOR_SHIFT) & PCI_VENDOR_MASK)

#define	PCI_PRODUCT_SHIFT			16
#define	PCI_PRODUCT_MASK			0xffff
#define	PCI_PRODUCT(id) \
	    (((id) >> PCI_PRODUCT_SHIFT) & PCI_PRODUCT_MASK)

#define PCI_ID_CODE(vid,pid)					\
	((((vid) & PCI_VENDOR_MASK) << PCI_VENDOR_SHIFT) |	\
	 (((pid) & PCI_PRODUCT_MASK) << PCI_PRODUCT_SHIFT))	\

/*
 * PCI BHLC = BIST/Header Type/Latency Timer/Cache Line Size Register.
 */
#define	PCI_BHLC_REG			0x0c

#define	PCI_BIST_SHIFT				24
#define	PCI_BIST_MASK				0xff
#define	PCI_BIST(bhlcr) \
	    (((bhlcr) >> PCI_BIST_SHIFT) & PCI_BIST_MASK)

#define	PCI_HDRTYPE_SHIFT			16
#define	PCI_HDRTYPE_MASK			0xff
#define	PCI_HDRTYPE(bhlcr) \
	    (((bhlcr) >> PCI_HDRTYPE_SHIFT) & PCI_HDRTYPE_MASK)

#define	PCI_HDRTYPE_TYPE(bhlcr) \
	    (PCI_HDRTYPE(bhlcr) & 0x7f)

#define IS_PCI_HDRTYPE_PPB(bhlcr) \
      (PCI_HDRTYPE_TYPE(bhlcr) == PCI_HDRTYPE_PPB)

#define	PCI_HDRTYPE_MULTIFN(bhlcr) \
	    ((PCI_HDRTYPE(bhlcr) & 0x80) != 0)

#define	PCI_LATTIMER_SHIFT			8
#define	PCI_LATTIMER_MASK			0xff
#define	PCI_LATTIMER(bhlcr) \
	    (((bhlcr) >> PCI_LATTIMER_SHIFT) & PCI_LATTIMER_MASK)

#define	PCI_CACHELINE_SHIFT			0
#define	PCI_CACHELINE_MASK			0xff
#define	PCI_CACHELINE(bhlcr) \
	    (((bhlcr) >> PCI_CACHELINE_SHIFT) & PCI_CACHELINE_MASK)

/*
 * Interrupt Configuration Register; contains interrupt pin and line.
 */
#define	PCI_INTERRUPT_REG		0x3c

#define	PCI_INTERRUPT_PIN_SHIFT			8
#define	PCI_INTERRUPT_PIN_MASK			0xff
#define	PCI_INTERRUPT_PIN(icr) \
	    (((icr) >> PCI_INTERRUPT_PIN_SHIFT) & PCI_INTERRUPT_PIN_MASK)

#define	PCI_INTERRUPT_LINE_SHIFT		0
#define	PCI_INTERRUPT_LINE_MASK			0xff
#define	PCI_INTERRUPT_LINE(icr) \
	    (((icr) >> PCI_INTERRUPT_LINE_SHIFT) & PCI_INTERRUPT_LINE_MASK)


/*
 * PCI Class and Revision Register; defines type and revision of device.
 */
#define	PCI_CLASS_REG			0x08

#define	PCI_CLASS_SHIFT				24
#define	PCI_CLASS_MASK				0xff
#define	PCI_CLASS(cr) \
	    (((cr) >> PCI_CLASS_SHIFT) & PCI_CLASS_MASK)

#define	PCI_SUBCLASS_SHIFT			16
#define	PCI_SUBCLASS_MASK			0xff
#define	PCI_SUBCLASS(cr) \
	    (((cr) >> PCI_SUBCLASS_SHIFT) & PCI_SUBCLASS_MASK)

#define PCI_DEVICE_CLASS_NETWORK_CONTROLLER 2

/*
 * Command and status register.
 */
#define	PCI_COMMAND_STATUS_REG			0x04

#define	PCI_COMMAND_IO_ENABLE			0x00000001
#define	PCI_COMMAND_MEM_ENABLE			0x00000002
#define	PCI_COMMAND_MASTER_ENABLE		0x00000004
#define PCI_COMMAND_CAPABALITES_LIST    0x00000010


/*
 * Mapping registers
 */
#define	PCI_MAPREG_START		0x10
#define	PCI_MAPREG_END			0x28

#define PCI_MAPREG_NUM(offset)						\
(((unsigned)(offset)-PCI_MAPREG_START)/4)

#define	PCI_MAPREG_TYPE_MASK			0x00000001
#define	PCI_MAPREG_TYPE(mr)						\
	    ((mr) & PCI_MAPREG_TYPE_MASK)

#define	PCI_MAPREG_TYPE_MEM			0x00000000
#define	PCI_MAPREG_TYPE_IO			0x00000001

#define	PCI_MAPREG_MEM_TYPE_MASK		0x00000006
#define	PCI_MAPREG_MEM_TYPE(mr)						\
	    ((mr) & PCI_MAPREG_MEM_TYPE_MASK)

#define	PCI_MAPREG_MEM_TYPE_32BIT		0x00000000
#define	PCI_MAPREG_MEM_TYPE_64BIT		0x00000004

#define	PCI_MAPREG_MEM_ADDR_MASK		0xfffffff0
#define	PCI_MAPREG_MEM_ADDR(mr)						\
	    ((mr) & PCI_MAPREG_MEM_ADDR_MASK)

#define	PCI_MAPREG_MEM_SIZE(mr)						\
	    (PCI_MAPREG_MEM_ADDR(mr) & -PCI_MAPREG_MEM_ADDR(mr))

#define	PCI_MAPREG_IO_ADDR_MASK			0xfffffffc
#define	PCI_MAPREG_IO_ADDR(mr)						\
	    ((mr) & PCI_MAPREG_IO_ADDR_MASK)

#define	PCI_MAPREG_IO_SIZE(mr)						\
	    (PCI_MAPREG_IO_ADDR(mr) & -PCI_MAPREG_IO_ADDR(mr))


#endif
