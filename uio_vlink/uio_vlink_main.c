#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/uio_driver.h>
#include "front-config.h"

#define VENDOR_ID 0xcccc
#define DEVICE_ID 0x2222

struct pci_vlink_private{
	struct pci_dev * pdev;
	struct uio_info uio;
	
	int bar0_start;
	int bar0_size;
	int bar0_flag;
	int is_initialized;
};

struct pci_device_id vlink_device_id_tbl[]={
	{VENDOR_ID,DEVICE_ID,PCI_ANY_ID,PCI_ANY_ID,0,0,0},
	{0}
};

/*the flollowing three helper functons are imported from DPDK iigb_uio*/
static int vlink_uio_pci_setup_iomem(struct pci_dev *dev, struct uio_info *info, int n, int pci_bar, const char *name)
{
	unsigned long addr, len;
	void *internal_addr;

	if (n >= ARRAY_SIZE(info->mem))
		return -EINVAL;

	addr = pci_resource_start(dev, pci_bar);
	len = pci_resource_len(dev, pci_bar);
	if (addr == 0 || len == 0)
		return -1;
	internal_addr = ioremap(addr, len);
	if (internal_addr == NULL)
		return -1;
	info->mem[n].name = name;
	info->mem[n].addr = addr;
	info->mem[n].internal_addr = internal_addr;
	info->mem[n].size = len;
	info->mem[n].memtype = UIO_MEM_PHYS;
	return 0;
}

static int vlink_uio_pci_setup_ioport(struct pci_dev *dev, struct uio_info *info,int n, int pci_bar, const char *name)
{
	unsigned long addr, len;

	if (n >= ARRAY_SIZE(info->port))
		return -EINVAL;

	addr = pci_resource_start(dev, pci_bar);
	len = pci_resource_len(dev, pci_bar);
	if (addr == 0 || len == 0)
		return -EINVAL;

	info->port[n].name = name;
	info->port[n].start = addr;
	info->port[n].size = len;
	info->port[n].porttype = UIO_PORT_X86;

	return 0;
}
static int vlink_uio_setup_bars(struct pci_dev *dev, struct uio_info *info)
{
	int i, iom, iop, ret;
	unsigned long flags;
	static const char *bar_names[PCI_STD_RESOURCE_END + 1]  = {
		"BAR0",
		"BAR1",
		"BAR2",
		"BAR3",
		"BAR4",
		"BAR5",
	};

	iom = 0;
	iop = 0;

	for (i = 0; i < ARRAY_SIZE(bar_names); i++) {
		if (pci_resource_len(dev, i) != 0 && pci_resource_start(dev, i) != 0) {
			flags = pci_resource_flags(dev, i);
			if (flags & IORESOURCE_MEM) {
				ret = vlink_uio_pci_setup_iomem(dev, info, iom,
							     i, bar_names[i]);
				if (ret != 0)
					return ret;
				iom++;
			} else if (flags & IORESOURCE_IO) {
				ret = vlink_uio_pci_setup_ioport(dev, info, iop,
							      i, bar_names[i]);
				if (ret != 0)
					return ret;
				iop++;
			}
		}
	}
	
	return (iom != 0) ? ret : -ENOENT;
}



static int vlink_pci_probe(struct pci_dev*dev,const struct pci_device_id * id)
{
	struct pci_vlink_private *private= kzalloc(sizeof(struct pci_vlink_private),GFP_KERNEL);
	if(!private)
		return -ENOMEM;
	
	
	
	if(pci_enable_device(dev))
		goto out_free;
	if(pci_request_regions(dev,"uio_vlink"))
		goto out_disable;
	
	private->pdev=dev;
	pci_set_drvdata(dev,private);

	private->bar0_start=pci_resource_start(dev,0);
	private->bar0_size=pci_resource_len(dev,0);
	private->bar0_flag=pci_resource_flags(dev,0);

	outl(0,private->bar0_start+BAR0_OFFSET_INTERRUPT_ENABLE_REG);/*disbale interrupt first*/
	
	
	private->uio.name="uio_vlink";
	private->uio.version="0.1";
	private->uio.priv=private;
	private->uio.irq=/*dev->irq*/UIO_IRQ_NONE;
	/*todo:interrupt features*/
	/*todo:open/release features*/
	
	vlink_uio_setup_bars(dev,&private->uio);
	if(uio_register_device(&dev->dev,&private->uio))
		goto out_release;
	
	private->is_initialized=1;
	return 0;
	
	out_release:
		pci_release_regions(dev);
	out_disable:
		pci_disable_device(dev);
	out_free:
		private->is_initialized=0;
		return -ENODEV;
}
static void vlink_pci_remove(struct pci_dev *dev)
{
	struct pci_vlink_private *private =pci_get_drvdata(dev);
	
	uio_unregister_device(&private->uio);
	pci_release_regions(dev);
	pci_disable_device(dev);
	kfree(private);
}
struct pci_driver vlink_device_pci_driver={
	.name="uio_vlink",
	.id_table=vlink_device_id_tbl,
	.probe=vlink_pci_probe,
	.remove=vlink_pci_remove
};
MODULE_DEVICE_TABLE(pci,vlink_device_id_tbl);
module_pci_driver(vlink_device_pci_driver);

#if 0
int uio_vlink_mod_init(void)
{
	
	return 0;
}

void uio_vlink_mod_exit(void)
{
	
}
module_init(uio_vlink_mod_init);
module_exit(uio_vlink_mod_exit);
#endif

MODULE_AUTHOR("Linc@unitedstack");
MODULE_DESCRIPTION("UIO driver for virtual link vNIC");
MODULE_LICENSE("GPL");



