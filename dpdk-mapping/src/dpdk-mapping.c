#include <dpdk-mapping.h>

struct dpdk_mmap_record * alloc_dpdk_mmap_record_array(void)
{
	struct dpdk_mmap_record * ret=NULL;
	ret=malloc(sizeof(struct dpdk_mmap_record)*MAX_DPDK_HUGEPAGES);
	
	return  ret;
}

void dealloc_dpdk_mmap_record_array(struct dpdk_mmap_record*array)
{
	if(array)
		free(array);
}

int load_dpdk_memory_metadata(struct dpdk_mmap_record*array)
{
	
	int nr_pages=0;
	int idx=0;
	FILE* fp=fopen(DPDK_METADATA_FILE,"r");
	if(!fp)
		return 0;
	for(idx=0;idx<MAX_DPDK_HUGEPAGES;idx++){
		if(feof(fp))
			break;
		fscanf(fp,"%s%"PRIx64,array[idx].hugepage_file_path,&array[idx].phy_address);
		if(!array[idx].phy_address)
			break;
		/*printf("[x] %s %"PRIx64"\n",array[idx].hugepage_file_path,array[idx].phy_address);*/
	}
	nr_pages=idx;
	printf("[x]%d hugepage file detected\n",nr_pages);
	fclose(fp);
	return nr_pages;
}
uint64_t preserve_vm_area(int nr_pages,void **raw_addr)
{
	void* addr=NULL;
	int fd;
	fd=open("/dev/zero",O_RDONLY);
	if(fd<0)
		goto ret;
	addr=mmap(0,(nr_pages+2)*HUGEPAGE_2M, PROT_READ, MAP_PRIVATE, fd, 0);
	if(!addr)
		goto fails;
	printf("[x]unaligned preserved memory start address:%"PRIx64"\n",(uint64_t)addr);
	if(raw_addr)
		*raw_addr=(void*)addr;
	munmap(addr,(nr_pages+2)*HUGEPAGE_2M);
	
	addr=(void*)((HUGEPAGE_2M+(uint64_t)addr)&(uint64_t)(~HUGEPAGE_2M_MASK));
	printf("[x]aligned preserved memory start address:%"PRIx64"\n",(uint64_t)addr);
	ret:
	return (uint64_t)addr;

	fails:
		close(fd);
		goto ret;
}
int map_vma_to_hugepages(uint64_t addr,struct dpdk_mmap_record*array,int nr_pages)
{
	int fd;
	int idx=0;
	uint64_t addr_map;
	assert(addr);
	assert(array);
	
	for(idx=0;idx<nr_pages;idx++){
		addr_map=addr+HUGEPAGE_2M*idx;
		fd=open(array[idx].hugepage_file_path,O_RDWR,0);
		assert(fd>=0);
		array[idx].vir_address=(uint64_t)mmap((void*)addr_map,HUGEPAGE_2M,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
		assert(array[idx].vir_address==addr_map);
		printf("[x]%s 0x%"PRIx64"  0x%"PRIx64"\n",array[idx].hugepage_file_path,array[idx].vir_address,array[idx].phy_address);
		close(fd);
	}
	return 0;
}
void setup_phy2virt_hash_tbl(struct dpdk_physeg_str * hashtbl,struct dpdk_mmap_record*array,int nr_pages)
{
	int idx;
	for(idx=0;idx<nr_pages;idx++){
		assert(!add_key_value(hashtbl,array[idx].phy_address,array[idx].vir_address));
	}
}

void setup_virt2phy_hash_tbl(struct dpdk_physeg_str*hashtbl,struct dpdk_mmap_record*array,int nr_pages)
{
	int idx;
	for(idx=0;idx<nr_pages;idx++){
		assert(!add_key_value(hashtbl,array[idx].vir_address,array[idx].phy_address));
	}
}
uint64_t translate_phy_address(struct dpdk_physeg_str*hashtbl,uint64_t phy_addr)
{
	uint64_t page_address=phy_addr&(~HUGEPAGE_2M_MASK);
	uint64_t address_offset=phy_addr&HUGEPAGE_2M_MASK;
	uint64_t page_virt_address=lookup_key(hashtbl,page_address);
	if(!page_virt_address)//loopup failure
		return 0;
		
	return page_virt_address|address_offset;
}
uint64_t translate_virt_address(struct dpdk_physeg_str*hashtbl,uint64_t virt_addr)
{
	uint64_t page_address=virt_addr&(~HUGEPAGE_2M_MASK);
	uint64_t address_offset=virt_addr&HUGEPAGE_2M_MASK;
	uint64_t page_phy_address=lookup_key(hashtbl,page_address);
	if(!page_phy_address)//loopup failure
		return 0;
		
	return page_phy_address|address_offset;
}

