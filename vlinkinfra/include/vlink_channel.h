#ifndef _VLINK_CHANNEL_H
#define _VLINK_CHANNEL_H

#ifndef _VLINK_IN_KERNEL
#include <stdint.h>
#endif

#define CONNECT_STATUS_DISCONNECTED 0x0
#define CONNECT_STATUS_CONNECTED 0x1

#define DRIVER_STATUS_NOT_INITIALIZED 0x0
#define DRIVER_STATUS_INITIALIZING 0x1
#define DRIVER_STATUS_INITIALIZED 0x2

#define INITIAL_VERSION_NUMBER 0x1


struct data_channel_record{
	uint32_t  index_in_m_domain;
	uint32_t  offset_to_vm_shm_base;
	uint32_t  data_channel_enabled;
	uint64_t  rx_address_to_translate;
	uint64_t  tx_address_to_translate;
	uint32_t  interrupt_enabled;/*default is false*/
}__attribute__((packed));


struct ctrl_channel{
	uint32_t dpdk_connected:1;
	uint32_t qemu_connected:1;
	uint32_t dpdk_driver_state:3;
	uint32_t qemu_driver_state:3;
	uint32_t version_number:8;
	uint32_t index_in_vm_domain:8;
	uint32_t offset_to_vm_shm_base;
	uint32_t nr_data_channels;
	union{
		uint64_t dummy;
		uint8_t mac_address[6];
	};
	
	struct data_channel_record channel_records[0];
}__attribute__((packed));

#define PTR(type_ptr,ptr) ((type_ptr)(ptr))
#define PTR_OFFSET_BY(type_ptr,ptr,offset) PTR(type_ptr,((uint64_t)(offset)+((char*)(ptr))))


#endif
