#ifndef __DPDK_MAPPING
#define __DPDK_MAPPING
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <sys/mman.h>
#include <stdint.h>
#include <inttypes.h>
#include <unistd.h>
#include <hash.h>


#define HUGEPAGE_2M (1<<21)
#define HUGEPAGE_2M_MASK ((1<<21)-1)


#define DPDK_METADATA_FILE "/tmp/dpdk-memory-metadata"
#define MAX_DPDK_HUGEPAGES 2048 /*for most application,4GB is enough*/
struct dpdk_mmap_record{
	char hugepage_file_path[64];
	uint64_t phy_address;
	uint64_t vir_address;
};

struct dpdk_mmap_record * alloc_dpdk_mmap_record_array(void);
void dealloc_dpdk_mmap_record_array(struct dpdk_mmap_record*array);
int load_dpdk_memory_metadata(struct dpdk_mmap_record*array);
/*uint64_t preserve_vm_area(int nr_pages);*/
uint64_t preserve_vm_area(int nr_pages,void **raw_addr);

int map_vma_to_hugepages(uint64_t addr,struct dpdk_mmap_record*array,int nr_pages);

void setup_phy2virt_hash_tbl(struct dpdk_physeg_str * hashtbl,struct dpdk_mmap_record*array,int nr_pages);
void setup_virt2phy_hash_tbl(struct dpdk_physeg_str*hashtbl,struct dpdk_mmap_record*array,int nr_pages);

uint64_t translate_phy_address(struct dpdk_physeg_str*hashtbl,uint64_t phy_addr);
uint64_t translate_virt_address(struct dpdk_physeg_str*hashtbl,uint64_t virt_addr);

#endif
