#include <stdio.h>
#include <dpdk-mapping.h>
#include <virtbus_client.h>

#include <agent.h>
#include <assert.h>

struct dpdk_physeg_str * p2v_hashtbl;
struct dpdk_physeg_str * v2p_hashtbl;
uint64_t shm_base;

int main()
{
	struct dpdk_mmap_record*record=alloc_dpdk_mmap_record_array();
	assert(record);
	int nr_pages=load_dpdk_memory_metadata(record);
	shm_base=preserve_vm_area(nr_pages,NULL);
	map_vma_to_hugepages(shm_base,record,nr_pages);
	
	/*setup index table*/
	p2v_hashtbl=alloc_physeg_hash_table();
	v2p_hashtbl=alloc_physeg_hash_table();
	assert(p2v_hashtbl);
	assert(v2p_hashtbl);
	setup_phy2virt_hash_tbl(p2v_hashtbl,record,nr_pages);
	setup_virt2phy_hash_tbl(v2p_hashtbl,record,nr_pages);
	
	uint64_t target_phy,target_virt;
	scanf("%"PRIx64"",&target_phy);
	target_virt=translate_phy_address(p2v_hashtbl,target_phy);
	printf("%"PRIx64":%s",target_virt,target_virt);
	return 0;
}