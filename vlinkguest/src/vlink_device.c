#include <vlink_device.h>
#include <uio_helper.h>
#include <sys/io.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
int vlink_device_initialize(struct vlink_device * device)
{
	int rc;
	int bar_num;
	memset(device,0x0,sizeof(struct vlink_device));
	
	device->uio_device_num=find_uio_device_number_by_vendor_and_device_id(VLINK_VENDOR_ID,VLINK_DEVICE_ID);
	if(device->uio_device_num<0)
		return -1;
	rc=get_uio_port_info(device->uio_device_num,0,(int*)&device->bar0_base,(int*)&device->bar0_size);
	if(rc<0)
		return -2;
	rc=iopl(3);/*grant all rights to access io ports*/
	/*rc=ioperm(device->bar0_base,device->bar0_size,1);*/
	device->nr_host_hugepages=inl(device->bar0_base+BAR0_OFFSET_COMMAND_NR_PAGES_REG);
	
	/*retrive anything about BAR2*/
	rc=get_uio_memory_info(device->uio_device_num,0,&bar_num,(int*)&device->host_mapping_size);
	if(rc<0||bar_num!=2)
		return -3;
	device->host_hugepage_base=(uint64_t)map_uio_region_memory_from_char_device_alligned(device->uio_device_num,0,device->host_mapping_size);
	if(device->host_hugepage_base==-1)
		return -4;

	/*retrive anything about BAR3*/
	rc=get_uio_memory_info(device->uio_device_num,1,&bar_num,(int*)&device->ctrl_channel_size);
	if(rc<0 || bar_num!=3)
		return -5;
	device->ctrl_channel_base=(uint64_t)map_uio_region_memory_from_char_device_alligned(device->uio_device_num,1,device->ctrl_channel_size);
	if(device->ctrl_channel_base==-1)
		return -6;
	
	return 0;
}

int establish_translation_tbl(struct vlink_device * device)
{
	int idx=0;
	struct dpdk_mmap_record *record=malloc(device->nr_host_hugepages*sizeof(struct dpdk_mmap_record));
	if(record)
		memset(record,0x0,device->nr_host_hugepages*sizeof(struct dpdk_mmap_record));
	device->p2v_hashtbl=alloc_physeg_hash_table();
	device->v2p_hashtbl=alloc_physeg_hash_table();
	if(!record||!device->p2v_hashtbl || !device->v2p_hashtbl)
		goto fails;
	for(idx=0;idx<device->nr_host_hugepages;idx++){
		uint32_t addr_lo;
		uint32_t addr_hi;
		uint64_t phy_addr;
		outl_p(idx,device->bar0_base+BAR0_OFFSET_COMMON_0_REG);
		addr_lo=inl_p(device->bar0_base+BAR0_OFFSET_COMMAND_HUGEPAGE_LO_ADDRESS_REG);
		addr_hi=inl_p(device->bar0_base+BAR0_OFFSET_COMMAND_HUGEPAGE_HI_ADDRESS_REG);
		phy_addr=((uint64_t)addr_hi)<<32|addr_lo;

		record[idx].phy_address=phy_addr;
		record[idx].vir_address=device->host_hugepage_base+idx*_HUGEPAGE_2M;
	}
	setup_phy2virt_hash_tbl(device->p2v_hashtbl,record,device->nr_host_hugepages);
	setup_virt2phy_hash_tbl(device->v2p_hashtbl,record,device->nr_host_hugepages);

	free(record);
	
	return 0;
	fails:
		if(record)
			free(record);
		if(device->p2v_hashtbl)
			free(device->p2v_hashtbl);
		if(device->v2p_hashtbl)
			free(device->v2p_hashtbl);
		return -1;
		
}
int scan_virtual_link(struct vlink_device * device)
{
	int idx=0;
	int is_link_enabled ;
	int ctrl_channel_idx;
	int iptr=0;
	printf("[x]scan virtual links... ...\n");
	for(;idx<MAX_LINKS;idx++){
		device->us_link_desc[idx].ctrl_channel_index=0xffffffff;
		outl_p(idx,device->bar0_base+BAR0_OFFSET_COMMON_0_REG);
		/*1.check whether the link is available*/
		is_link_enabled= inl_p(device->bar0_base+BAR0_OFFSET_COMMAND_LINK_ENABLED_REG);
		if(!is_link_enabled)
			continue;
		/*2.try to obtain the controll channel index*/
		ctrl_channel_idx=inl_p(device->bar0_base+BAR0_OFFSET_COMMAMD_LINK_CTRL_CHANNEL_IDX_REG);
		device->us_link_desc[idx].ctrl_channel_index=ctrl_channel_idx;

		/*3.try to initialize ctrl and data channels*/
		device->us_link_desc[idx].ctrl=PTR_OFFSET_BY(struct ctrl_channel*,device->ctrl_channel_base,channel_base_offset(device->us_link_desc[idx].ctrl_channel_index));
		printf("[x]link-id: %d  ctrl-channel-index:%d data-channels:%d mac:%02x:%02x:%02x:%02x:%02x:%02x\n",
			idx,
			device->us_link_desc[idx].ctrl->index_in_vm_domain,
			device->us_link_desc[idx].ctrl->nr_data_channels,
			device->us_link_desc[idx].ctrl->mac_address[0],
			device->us_link_desc[idx].ctrl->mac_address[1],
			device->us_link_desc[idx].ctrl->mac_address[2],
			device->us_link_desc[idx].ctrl->mac_address[3],
			device->us_link_desc[idx].ctrl->mac_address[4],
			device->us_link_desc[idx].ctrl->mac_address[5]);
		
		for(iptr=0;iptr<device->us_link_desc[idx].ctrl->nr_data_channels;iptr++){
			device->us_link_desc[idx].data[iptr].rx_stub=PTR_OFFSET_BY(struct queue_stub*,device->ctrl_channel_base,device->us_link_desc[idx].ctrl->channel_records[iptr].offset_to_vm_shm_base+rx_sub_channel_offset());
			device->us_link_desc[idx].data[iptr].free_stub=PTR_OFFSET_BY(struct queue_stub*,device->ctrl_channel_base,device->us_link_desc[idx].ctrl->channel_records[iptr].offset_to_vm_shm_base+free_sub_channel_offset());
			device->us_link_desc[idx].data[iptr].alloc_stub=PTR_OFFSET_BY(struct queue_stub*,device->ctrl_channel_base,device->us_link_desc[idx].ctrl->channel_records[iptr].offset_to_vm_shm_base+alloc_sub_channel_offset());
			device->us_link_desc[idx].data[iptr].tx_stub=PTR_OFFSET_BY(struct queue_stub*,device->ctrl_channel_base,device->us_link_desc[idx].ctrl->channel_records[iptr].offset_to_vm_shm_base+tx_sub_channel_offset());
			printf("\t[x]data-channel-index:%d  (rx:%d) (free:%d) (alloc:%d) (tx:%d)\n",
				device->us_link_desc[idx].ctrl->channel_records[iptr].index_in_m_domain,
				(int)(device->us_link_desc[idx].ctrl->channel_records[iptr].offset_to_vm_shm_base+rx_sub_channel_offset()),
				(int)(device->us_link_desc[idx].ctrl->channel_records[iptr].offset_to_vm_shm_base+free_sub_channel_offset()),
				(int)(device->us_link_desc[idx].ctrl->channel_records[iptr].offset_to_vm_shm_base+alloc_sub_channel_offset()),
				(int)(device->us_link_desc[idx].ctrl->channel_records[iptr].offset_to_vm_shm_base+tx_sub_channel_offset()));
		}
	}
	return 0;
}
