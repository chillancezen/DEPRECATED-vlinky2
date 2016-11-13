#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/uio_driver.h>
#include "front-config.h"

#define _VLINK_IN_KERNEL
#include "vlink_channel.h"
#include "my_config.h"
#include <asm/io.h>


#define VENDOR_ID 0xcccc
#define DEVICE_ID 0x2222

struct pci_vlink_private{
	struct pci_dev * pdev;
	struct uio_info uio;
	
	int bar0_start;
	int bar0_size;
	int bar0_flag;

	int bar3_start;
	int bar3_size;
	int bar3_flag;
	void * __iomem bar3_mapped_addr;
	
	int is_initialized;
	int open_ref_cnt;
};

struct pci_device_id vlink_device_id_tbl[]={
	{VENDOR_ID,DEVICE_ID,PCI_ANY_ID,PCI_ANY_ID,0,0,0},
	{0}
};

/*the flollowing three helper functons are imported from DPDK igb_uio*/
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


int vlink_uio_device_open(struct uio_info *info,struct  inode* inode)
{
	struct pci_vlink_private * private=(struct pci_vlink_private*)info->priv;
	int idx=0;
	struct ctrl_channel * ctrl;
	uint32_t ctrl_index;
	
	if(!private->open_ref_cnt)/*open for 1st time*/
	for(idx=0;idx<MAX_LINKS;idx++){
		outl_p(idx,private->bar0_start+BAR0_OFFSET_COMMON_0_REG);
		ctrl_index=inl_p(private->bar0_start+BAR0_OFFSET_COMMAMD_LINK_CTRL_CHANNEL_IDX_REG);
		if(ctrl_index==0xffffffff)
			continue;
		ctrl=PTR_OFFSET_BY(struct ctrl_channel*,private->bar3_mapped_addr,channel_base_offset(ctrl_index));
		ctrl->qemu_driver_state=DRIVER_STATUS_INITIALIZING;/*set driver state to INITIALIZING*/
	}
	if(!private->open_ref_cnt)
	printk("uio_vlink:set driver state to DRIVER_STATUS_INITIALIZING\n");
	private->open_ref_cnt++;
	return 0;
}
int vlink_uio_device_release(struct uio_info *info,struct inode* inode)
{
	struct pci_vlink_private * private=(struct pci_vlink_private*)info->priv;
	int idx=0;
	struct ctrl_channel * ctrl;
	uint32_t ctrl_index;
	
	private->open_ref_cnt--;
	if(!private->open_ref_cnt)
	for(idx=0;idx<MAX_LINKS;idx++){
		outl_p(idx,private->bar0_start+BAR0_OFFSET_COMMON_0_REG);
		ctrl_index=inl_p(private->bar0_start+BAR0_OFFSET_COMMAMD_LINK_CTRL_CHANNEL_IDX_REG);
		if(ctrl_index==0xffffffff)
			continue;
		ctrl=PTR_OFFSET_BY(struct ctrl_channel*,private->bar3_mapped_addr,channel_base_offset(ctrl_index));
		ctrl->qemu_driver_state=DRIVER_STATUS_NOT_INITIALIZED;/*reset driver state*/
	}
	if(!private->open_ref_cnt)
	printk("uio_vlink:set driver state to DRIVER_STATUS_NOT_INITIALIZED\n");
	return 0;
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
	private->open_ref_cnt=0;
	
	pci_set_drvdata(dev,private);

	private->bar0_start=pci_resource_start(dev,0);
	private->bar0_size=pci_resource_len(dev,0);
	private->bar0_flag=pci_resource_flags(dev,0);

	private->bar3_start=pci_resource_start(dev,3);
	private->bar3_size=pci_resource_len(dev,3);
	private->bar3_flag=pci_resource_flags(dev,3);
	private->bar3_mapped_addr=pci_ioremap_bar(dev,3);

	printk("bar3 mapped address:0x%llx\n",(long long unsigned int)private->bar3_mapped_addr);
	outl(0,private->bar0_start+BAR0_OFFSET_INTERRUPT_ENABLE_REG);/*disbale interrupt first*/
	
	
	private->uio.name="uio_vlink";
	private->uio.version="0.1";
	private->uio.priv=private;
	private->uio.irq=/*dev->irq*/UIO_IRQ_NONE;
	private->uio.open=vlink_uio_device_open;
	private->uio.release=vlink_uio_device_release;
	/*todo:interrupt features*/
	/*todo:open/release features*/
	
	vlink_uio_setup_bars(dev,&private->uio);
	if(uio_register_device(&dev->dev,&private->uio))
		goto out_release;
	
	private->is_initialized=1;
	return 0;
	
	out_release:
		if(private->bar3_mapped_addr)
			pci_iounmap(dev,private->bar3_mapped_addr);
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
	if(private->bar3_mapped_addr)
		pci_iounmap(dev,private->bar3_mapped_addr);
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



