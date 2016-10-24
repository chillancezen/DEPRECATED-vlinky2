/*
 * QEMU virtual link PCI device
 *
 * Copyright (c) 2016 zheng jie
 * Author: Linc <jzheng.bjtu@hotmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <http://www.gnu.org/licenses/>.
 */
#include "hw/hw.h"
#include "hw/pci/pci.h"
#include "qemu/event_notifier.h"
#include "qemu/osdep.h"
#include "front-config.h"
#include <agent.h>
#include <dpdk-mapping.h>
#include <vlink_channel.h>
#include <virtbus_client.h>

#define VLINK_DEVICE_NAME "pci-vlink"
#define MAX_DPDK_MM_SEG 8


typedef struct _vlink_descriptor{
	int is_enabled;
	struct client_endpoint *ce;
}vlink_descriptor;
typedef struct PCIVlinkprivate{
	PCIDevice parent_obj;

	/*misc fields*/
	int driver_in_control;

	
	/*channel-shared-memory*/
	uint64_t channel_vm_base;
	char* vm_domain_name;
	int max_channels;


	
	/*DPDK-hugepage-mapping*/
	struct dpdk_mmap_record *dpdk_mm_record;
	int nr_pages;
	uint64_t dpdk_hm_base;/*every 512 pages will be grouped into one seg*/
	struct dpdk_physeg_str  *p2v_hashtbl;
	struct dpdk_physeg_str * v2p_hashtbl;



	
	/*Base Address Region*/
	MemoryRegion misc_region;
	MemoryRegion dpdk_mapping_region;
	MemoryRegion channel_region;

	/*virtual links*/
	char * initial_link_config_path;
	vlink_descriptor link_desc[MAX_LINKS];

	/*common REGs*/
	uint32_t reg0;
	uint32_t reg1;
	uint32_t reg2;
	uint32_t reg3;

}PCIVlinkprivate;

uint64_t ctrl_bar_io_read(void * opaque,hwaddr addr,unsigned size)
{
	PCIVlinkprivate * private=(PCIVlinkprivate*)opaque;
	uint64_t ret_val=0;
	switch(addr)
	{
		case BAR0_OFFSET_COMMON_0_REG:
			ret_val=private->reg0;
			break;
		case BAR0_OFFSET_COMMON_1_REG:
			ret_val=private->reg1;
			break;
		case BAR0_OFFSET_COMMON_2_REG:
			ret_val=private->reg2;
			break;
		case BAR0_OFFSET_COMMON_3_REG:
			ret_val=private->reg3;
			break;
		case BAR0_OFFSET_COMMAND_NR_PAGES_REG:
			ret_val=private->nr_pages;
			break;
		case BAR0_OFFSET_COMMAND_HUGEPAGE_LO_ADDRESS_REG:
			/*
				reg0 must be set as the hugepage index before this reg is read,
				and reg0 must lower than private->nr_pages,otherwise,0 is returned
			*/
			{
				int index=(int)private->reg0;
				if(index>=private->nr_pages)
					break;
				ret_val=(private->dpdk_mm_record[index].phy_address&0xffffffff);
			}
			break;
		case BAR0_OFFSET_COMMAND_HUGEPAGE_HI_ADDRESS_REG:
			{
				int index=(int)private->reg0;
				if(index>=private->nr_pages)
					break;
				ret_val=((private->dpdk_mm_record[index].phy_address>>32)&0xffffffff);
			}
			break;
		case BAR0_OFFSET_COMMAND_LINK_ENABLED_REG:
			/*
				reg0 must be set as the link index before this reg is read,
				and reg0 must lower than MAX_LINKS,otherwise,0 is returned,
				a non-zero inicates "enabled"
			*/
			if(private->reg0>=MAX_LINKS)
				break;
			
			ret_val=private->link_desc[(int)private->reg0].is_enabled;
			break;
		case BAR0_OFFSET_COMMAMD_LINK_CTRL_CHANNEL_IDX_REG:
			/*
				reg0 must be set as the link index before this reg is read,
				and reg0 must lower than MAX_LINKS,otherwise,0xffffffff is returned,
			*/
			ret_val=0xffffffff;
			if(private->reg0>=MAX_LINKS || !private->link_desc[(int)private->reg0].is_enabled ||!private->link_desc[(int)private->reg0].ce/*in case it's bug on this*/)
				break;
			ret_val=private->link_desc[(int)private->reg0].ce->channels[0];/*1st channel is ctrl channel*/
			break;

		
	}
	return ret_val;

}
void ctrl_bar_io_write(void * opaque,hwaddr addr,uint64_t val,unsigned size)
{
	PCIVlinkprivate * private=(PCIVlinkprivate*)opaque;
	switch(addr)
	{
		case BAR0_OFFSET_COMMON_0_REG:
			private->reg0=(uint32_t)val;
			break;
		case BAR0_OFFSET_COMMON_1_REG:
			private->reg1=(uint32_t)val;
			break;
		case BAR0_OFFSET_COMMON_2_REG:
			private->reg2=(uint32_t)val;
			break;
		case BAR0_OFFSET_COMMON_3_REG:
			private->reg3=(uint32_t)val;
			break;
	}
	
}


struct MemoryRegionOps ctrl_bar_io_ops={
	.read=ctrl_bar_io_read,
	.write=ctrl_bar_io_write,
	.endianness=DEVICE_NATIVE_ENDIAN,
	.impl={
		.min_access_size=4,
		.max_access_size=4/*even with x86_64,io port bandwidth remains 32-bit*/
	}
};
void _initialize_links_from_config(PCIVlinkprivate*private)
{
	FILE *fp;
	int iptr=0;
	int rc;
	char  link_name[64];
	char mac_address[6];
	int nr_channel=0;
	if(!private->initial_link_config_path)
		return;
	fp=fopen(private->initial_link_config_path,"r");
	if(!fp)
		return;
	while(!feof(fp)){
		memset(link_name,0x0,sizeof(link_name));
		memset(mac_address,0x0,sizeof(mac_address));
		nr_channel=0;
		fscanf(fp,"%s %x:%x:%x:%x:%x:%x %d\n",link_name,(unsigned int*)&mac_address[0]
			,(unsigned int*)&mac_address[1]
			,(unsigned int*)&mac_address[2]
			,(unsigned int*)&mac_address[3]
			,(unsigned int*)&mac_address[4]
			,(unsigned int*)&mac_address[5],&nr_channel);
		private->link_desc[iptr].ce=client_endpoint_alloc_and_init();
		if(!private->link_desc[iptr].ce)
			continue;
		
		if(!private->link_desc[iptr].ce->is_connected){
			client_endpoint_uninit_and_dealloc(private->link_desc[iptr].ce);
			private->link_desc[iptr].ce=NULL;
			continue;
		}
		
		rc=client_endpoint_init_virtual_link(private->link_desc[iptr].ce,
			VLINK_ROLE_QEMU,
			private->vm_domain_name,
			private->max_channels,
			link_name,
			mac_address,
			MIN(nr_channel,MAX_LINK_CHANNEL));
		if(rc<0){
			client_endpoint_uninit_and_dealloc(private->link_desc[iptr].ce);
			private->link_desc[iptr].ce=NULL;
			continue;
		}
		private->link_desc[iptr].is_enabled=!0;
		printf("\n[x]initialized virtual link:%s with %d channels with mac: %02x:%02x:%02x:%02x:%02x:%02x",
			private->link_desc[iptr].ce->vlink_name,
			private->link_desc[iptr].ce->allocated_channels,
			private->link_desc[iptr].ce->mac_address[0],
			private->link_desc[iptr].ce->mac_address[1],
			private->link_desc[iptr].ce->mac_address[2],
			private->link_desc[iptr].ce->mac_address[3],
			private->link_desc[iptr].ce->mac_address[4],
			private->link_desc[iptr].ce->mac_address[5]);
		iptr++;
	}
	puts("\n");
	fclose(fp);
}
static void pci_vlinkdev_realize(PCIDevice *pci_dev,Error**errp)
{
	int bar_size;
	PCIVlinkprivate * private =OBJECT_CHECK(PCIVlinkprivate,pci_dev,VLINK_DEVICE_NAME);
	/*1.initialize dpdk-memory mapping*/
	
	private->dpdk_mm_record=alloc_dpdk_mmap_record_array();
	private->nr_pages=load_dpdk_memory_metadata(private->dpdk_mm_record);
	private->dpdk_hm_base=preserve_vm_area(private->nr_pages,NULL);
	map_vma_to_hugepages(private->dpdk_hm_base,private->dpdk_mm_record,private->nr_pages);

	private->p2v_hashtbl=alloc_physeg_hash_table();
	private->v2p_hashtbl=alloc_physeg_hash_table();

	setup_phy2virt_hash_tbl(private->p2v_hashtbl,private->dpdk_mm_record,private->nr_pages);
	setup_virt2phy_hash_tbl(private->v2p_hashtbl,private->dpdk_mm_record,private->nr_pages);
	
	/*2.register bra2:dpdk-memory-mapping-region*/
	bar_size=HUGEPAGE_2M*private->nr_pages;
	bar_size=POWER_ROUND(bar_size);
	memory_region_init_ram_ptr(&private->dpdk_mapping_region,OBJECT(private),"dpdk-memory-mapping-region",bar_size,(void*)private->dpdk_hm_base);
	pci_register_bar(pci_dev,2,PCI_BASE_ADDRESS_SPACE_MEMORY|PCI_BASE_ADDRESS_MEM_PREFETCH,&private->dpdk_mapping_region);
	
	/*3 register bar3:channels-region*/
	
	private->channel_vm_base=map_shared_memory(private->vm_domain_name,CHANNEL_SIZE*private->max_channels);
	bar_size=CHANNEL_SIZE*private->max_channels;
	bar_size=POWER_ROUND(bar_size);
	memory_region_init_ram_ptr(&private->channel_region,OBJECT(private),"channel-region",bar_size,(void*)private->channel_vm_base);
	pci_register_bar(pci_dev,3,PCI_BASE_ADDRESS_SPACE_MEMORY|PCI_BASE_ADDRESS_MEM_PREFETCH,&private->channel_region);

	/*4 register bar0:ctrl-region*/
	memory_region_init_io(&private->misc_region,OBJECT(private),&ctrl_bar_io_ops,private,"controll-region",1024);
	pci_register_bar(pci_dev,0,PCI_BASE_ADDRESS_SPACE_IO,&private->misc_region);

	/*5 additional setup*/
	{
		uint8_t * conf=pci_dev->config;
		conf[PCI_INTERRUPT_PIN]=0x2;
		conf[PCI_COMMAND]=PCI_COMMAND_IO|PCI_COMMAND_MEMORY;
		
	}
	/*6 initialize virtual link descriptor and link initial config*/
	private->driver_in_control=0;
	{
		int idx=0;
		for(idx=0;idx<MAX_LINKS;idx++){
			private->link_desc[idx].is_enabled=FALSE;
			private->link_desc[idx].ce=NULL;
		}
	}
	_initialize_links_from_config(private);
	
}
static void pci_vlinkdev_exit(PCIDevice *pci_dev)
{
	
}
static void pci_vlinkdev_reset(DeviceState*state)
{

}
static Property vlink_property[]={
	DEFINE_PROP_STRING("vm_domain",PCIVlinkprivate,vm_domain_name),
	DEFINE_PROP_INT32("max_channel",PCIVlinkprivate,max_channels,DEFAULT_MAX_CHANNEL),
	DEFINE_PROP_STRING("init_config",PCIVlinkprivate,initial_link_config_path),
	DEFINE_PROP_END_OF_LIST(),
};
void pci_vlink_class_init(ObjectClass * kclass,void*data)
{
	DeviceClass *dc=DEVICE_CLASS(kclass);
	PCIDeviceClass *pdc=PCI_DEVICE_CLASS(kclass);

	pdc->realize=pci_vlinkdev_realize;
	pdc->exit=pci_vlinkdev_exit;
	pdc->vendor_id=VLINK_VENDOR_ID;
	pdc->device_id=VLINK_DEVICE_ID;
	pdc->revision=0;
	pdc->class_id=PCI_CLASS_NETWORK_ETHERNET;
	dc->desc="virtual link PCI Ethernet device";
	dc->props=vlink_property;
	set_bit(DEVICE_CATEGORY_NETWORK,dc->categories);
	dc->reset=pci_vlinkdev_reset;
	
}
TypeInfo pci_vlink_info={
	.name=VLINK_DEVICE_NAME,
	.parent=TYPE_PCI_DEVICE,
	.instance_size=sizeof(PCIVlinkprivate),
	.class_init=pci_vlink_class_init
};

void pci_vlink_register_type(void)
{
	type_register_static(&pci_vlink_info);
}

type_init(pci_vlink_register_type);


