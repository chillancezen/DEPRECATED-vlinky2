#ifndef _VLINK_CHANNEL_H
#define _VLINK_CHANNEL_H
#include <stdint.h>

struct data_channel_record{
	uint32_t  index_in_m_domain;
	uint32_t  offset_to_vm_shm_base;
	uint32_t  data_channel_enabled;
	uint64_t  rx_address_to_translate;
	uint64_t  tx_address_to_translate;
	uint32_t  interrupt_enabled;/*default is false*/
}__attribute__((packed));


struct ctrl_channel{
	uint32_t index_in_vm_domain;
	uint32_t offset_to_vm_shm_base;
	uint32_t dpdk_connected;
	uint32_t qemu_connected;
	uint32_t nr_data_channels;
	union{
		uint32_t dummy;
		uint8_t mac_address[6];
	};
	struct data_channel_record channel_records[0];
}__attribute__((packed));

#define PTR(type_ptr,ptr) ((type_ptr)(ptr))
#define PTR_OFFSET_BY(type_ptr,ptr,offset) PTR(type_ptr,((uint64_t)(offset)+((char*)(ptr))))


#endif
