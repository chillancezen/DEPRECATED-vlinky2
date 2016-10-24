
#ifndef __AGENT_H
#define __AGENT_H
#include <stdint.h>
#include <my_config.h>

#define ASSERT(condition)  \
do{ \
	if(!(condition)){\
		printf("[assert]%s in file:%s line:%d %s\n",__FUNCTION__,__FILE__,__LINE__,#condition); \
		exit(-1); \
	}\
}while(0)


#define PAGE_ALIGN(pg_size,val) \
		((val)&((pg_size)-1))?(((~((pg_size)-1))&(val))+pg_size):(val)
		
#define POWER_ROUND(val)({ \
	uint64_t shadow;\
	int idx; \
	int left_most_index=-1; \
	int right_most_index=0; \
	for(shadow=0x1,idx=0;idx<64;idx++,shadow=shadow<<1){\
		if(((uint64_t)val)&shadow){\
			left_most_index=(left_most_index==-1)?idx:left_most_index;\
			right_most_index=idx;\
			}\
		}\
	(left_most_index==right_most_index)?(val):(((uint64_t)0x1)<<(right_most_index+1));\
	})



#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif

#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif




struct virtual_link;
struct  vm_domain{
	uint8_t  domain_name[64];
	uint32_t max_channels;
	uint64_t shm_base;
	uint32_t shm_length;
	
	uint32_t nr_links;

	/*channels resource track*/
	uint8_t * channel_track;
	struct virtual_link * head;
}__attribute__((packed));

#define MAX_LINK_CHANNEL 16

struct virtual_link {
	uint8_t link_name[64];/*this value will be hashed into global table*/
	int nr_channels_allocated;
	uint8_t channels[MAX_LINK_CHANNEL];
	
	struct vm_domain * domain;
	struct virtual_link *next;
	int fd_dpdk;
	int fd_qemu;
	uint8_t mac_address[6];
}__attribute__((packed));

uint64_t map_shared_memory_from_sockets(char * name,int length, unsigned long *nodemask);
uint64_t map_shared_memory(char * name,int length);
void unmap_shared_memory(uint64_t base,int length);
struct vm_domain* alloc_vm_domain(int max_channels,char*name);
void free_vm_domain(struct vm_domain*domain);
void attach_virtual_link_to_vm_domain(struct vm_domain*domain,struct virtual_link* link);
void detach_virtual_link_from_vm_domain(struct virtual_link* link);
void dump_virtual_link_in_vm_domain(struct vm_domain* domain);


#define channel_offset(link,local_index) \
({\
	ASSERT(((link)->domain)&&((local_index)<(link)->nr_channels_allocated));\
	ASSERT((link)->domain->channel_track[(link)->channels[local_index]]==1);\
	((link)->channels[local_index]*CHANNEL_SIZE);\
})



struct virtual_link * alloc_virtual_link(char* name);
int allocate_channel_from_vm_domain(struct virtual_link* link,int channel_quantum);
int dealloc_channel_to_vm_domain(struct virtual_link * link);
void free_virtual_link(struct virtual_link* link);

void _initialize_virtual_link_channel(struct virtual_link * link,void*channel_base);
#define initialize_virtual_link_channel(link,local_index) \
do{\
	ASSERT(link->domain);\
	ASSERT(local_index<link->nr_channels_allocated);\
	_initialize_virtual_link_channel(link,channel_offset(link,local_index)+(char*)link->domain->shm_base);\
}while(0)



#endif
