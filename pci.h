#include "types.h"
#include "x86.h"

/*
 * PCI software configuration space access mechanism 1
 *
 * https://wiki.osdev.org/PCI#Configuration_Space_Access_Mechanism_.231
 */
#define PCI_CONFIG_ADDR             0xCF8
#define PCI_CONFIG_DATA             0xCFC

// Forward declaration
struct pci_device;

/*
 * The PCI specification specifies 8 bits for the bus identifier, 5 bits for
 * the device and 3 bits for selecting a particular function. This is the BDF
 * (Bus Device Function) address of a PCI device
 */
#define PCI_MAX_DEVICES 32


/*
 * Defines a PCI bus. Only considering it's parent bridge and the bus number
 * for now
 */
struct pci_bus {
    // A bridge is a type of PCI device which just forwards all transactions if
    // they are destined for any device behind it. The PCI host bridge is the
    // common interface for all other PCI devices/buses on a system
    struct pci_device *parent_bridge;
    uint32 bus_num;
};

struct pci_device {
    int state;
    // The bus this device is on
    struct pci_bus *bus;

    // The device number of the device on the bus
    uint32 dev;
    // The function represented by this struct
    uint32 func;

    uint32 dev_id;
    uint32 dev_class;

    uint32 reg_base[6];
    uint32 reg_size[6];
    uint32 capabalities[6]; // Virtio spec v1.0 only defines 5 types of capabilites.
    uint8 cap_map[6]; // Maps capabalities to their BAR number
    uint32 cap_offset[6]; // Map the offset into a BAR for the given capability

    uint8 irq_line;
    uint8 irq_pin;
    uint32 membase;
    uint32 iobase;
};

#define NPCI                        10

extern struct pci_device pcidevs[NPCI];
extern int pcikeys[NPCI];



/*
 * Macro to create the 32 bit register values for a PCI transaction. `off` here
 * is the register value in the PCI config space of the device specified by
 * `dev` on the bus specificed by `bus` and the function specified by `fn`.
 *
 * The MSB of the 32 bit value needs to be 1 in order to mark it as
 * a configuration transaction
 */
#define PCI_FORMAT(bus, dev, fn, off) ({0x80000000 | bus << 16 | dev << 11 | fn << 8 | off;})

#define PCI_CONF_READ8(bus, dev, func, reg) \
	(outl(PCI_CONFIG_ADDR, PCI_FORMAT(bus, dev, func, reg)), \
	inb(PCI_CONFIG_DATA+((reg)&3)))
#define PCI_CONF_READ16(bus, dev, func, reg) \
	(outl(PCI_CONFIG_ADDR, PCI_FORMAT(bus, dev, func, reg)), \
	inw(PCI_CONFIG_DATA+((reg)&2)))
#define PCI_CONF_READ32(bus, dev, func, reg) \
	(outl(PCI_CONFIG_ADDR, PCI_FORMAT(bus, dev, func, reg)), \
	inl(PCI_CONFIG_DATA))

#define PCI_CONF_WRITE8(bus, dev, func, reg, val) \
	(outl(PCI_CONFIG_ADDR, PCI_FORMAT(bus, dev, func, reg)), \
	outb(PCI_CONFIG_DATA+((reg)&3), (val)))
#define PCI_CONF_WRITE16(bus, dev, func, reg, val) \
	(outl(PCI_CONFIG_ADDR, PCI_FORMAT(bus, dev, func, reg)), \
	outw(PCI_CONFIG_DATA+((reg)&2), (val)))
#define PCI_CONF_WRITE32(bus, dev, func, reg, val) \
	(outl(PCI_CONFIG_ADDR, PCI_FORMAT(bus, dev, func, reg)), \
	outl(PCI_CONFIG_DATA, (val)))


/*
 * Class codes of PCI devices at their offsets
 */
extern char* PCI_CLASSES[18];



/*
 * Reads the register at offset `off` in the PCI config space of the device
 * `dev`
 */
static inline uint32 confread32(struct pci_device *dev, uint32 off)
{
    return PCI_CONF_READ32(dev->bus->bus_num, dev->dev, dev->func, off);
}

static inline uint32 confread16(struct pci_device *dev, uint32 off)
{
    return PCI_CONF_READ16(dev->bus->bus_num, dev->dev, dev->func, off);
}

static inline uint32 confread8(struct pci_device *dev, uint32 off) {
    return PCI_CONF_READ8(dev->bus->bus_num, dev->dev, dev->func, off);
}

/*
 * Writes the provided value `value` at the register `off` for the device `dev`
 */
static inline void conf_write32(struct pci_device *dev, uint32 off, uint32 value)
{
    return PCI_CONF_WRITE32(dev->bus->bus_num, dev->dev, dev->func, off, value);
}

/*
 * Writes the provided value `value` at the register `off` for the device `dev`
 */
static inline void conf_write16(struct pci_device *dev, uint32 off, uint16 value)
{
    return PCI_CONF_WRITE16(dev->bus->bus_num, dev->dev, dev->func, off, value);
}

/*
 * Writes the provided value `value` at the register `off` for the device `dev`
 */
static inline void conf_write8(struct pci_device *dev, uint32 off, uint8 value)
{
    return PCI_CONF_WRITE8(dev->bus->bus_num, dev->dev, dev->func, off, value);
}

