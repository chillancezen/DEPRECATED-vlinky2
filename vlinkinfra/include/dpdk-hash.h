#ifndef __DPDK_MAPPING_HASH
#define __DPDK_MAPPING_HASH
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#define MEMORY_SEGMENT_TABLE_HASH_SIZE (1024*64)/*must be power of 2*/

struct dpdk_physeg_str{	
	union{
		uint64_t key;
		uint64_t physical_address;
	};
	union{
		uint64_t value;
		uint64_t virtual_address;
	};
};

struct dpdk_physeg_str* alloc_physeg_hash_table(void);
int add_key_value(struct dpdk_physeg_str* address_table_base,uint64_t key,uint64_t value);
uint64_t lookup_key(struct dpdk_physeg_str* address_table_base,uint64_t key);

#endif
