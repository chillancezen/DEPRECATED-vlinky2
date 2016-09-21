#ifndef _VLINK_CHANNEL_H
#define _VLINK_CHANNEL_H
#include <stdint.h>

struct data_channel_record{
	uint32_t  index_in_m_domain;
	uint32_t  offset_to_vm_shm_base;
	uint32_t  data_channel_enabled;
	uint64_t  rx_address_to_translate;
	uint64_t  tx_address_to_translate;
	uint64_t  interrupt_enabled;/*default is false*/
};


struct ctrl_channel{
	uint32_t index_in_vm_domain;
	uint32_t offset_to_vm_shm_base;
	uint32_t dpdk_connected;
	uint32_t qemu_connected;
	uint32_t nr_data_channels;
	struct data_channel_record channel_records[0];
};

#define PTR(type_ptr,ptr) ((type_ptr)(ptr))
#define PTR_OFFSET_BY(type_ptr,ptr,offset) PTR(type_ptr,((uint64_t)(offset)+((void*)(ptr))))


#endif
