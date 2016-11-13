#include <translation_tbl.h>
#include <stdio.h>
#include <stdlib.h>

/*add a new physical entries into p2v table,non-zero indicate failure*/
static int  _add_phyaddr_entry(struct phy2virt_table *p2v_tbl,uint64_t paddr,uint16_t index)
{
	int entry_index=(paddr&p2v_tbl->tbl_mask)>>HUGEPAGE_2M_SHIFT;
	int idx=0;
	for(idx=0;idx<MAX_ELE_IN_ENTRY;idx++){
		if(p2v_tbl->entries[entry_index].ele[idx].index==INVALID_INDEX){
			p2v_tbl->entries[entry_index].ele[idx].index=index;
			p2v_tbl->entries[entry_index].ele[idx].high_addr=(paddr&p2v_tbl->high_addr_mask)>>p2v_tbl->high_addr_mask_shift;
			break;
		}
	}
	return !(idx<MAX_ELE_IN_ENTRY);
}
static int add_physical_entries(struct phy2virt_table *p2v_tbl,struct dpdk_mmap_record *records,int nr_records)
{
	int idx=0;
	int rc;
	for(idx=0;idx<nr_records;idx++){
		rc=_add_phyaddr_entry(p2v_tbl,records[idx].phy_address,records[idx].vir_index);
		if(rc)
			return rc;
	}
	return 0;
}
int setup_phy2virt_translation_table(struct phy2virt_table *p2v_tbl,
	uint64_t virt_base,
	struct dpdk_mmap_record *records,
	int nr_records,
	mem_alloc m_alloc,
	mem_dealloc m_free)
{
	int rc=0;
	int retries;
	uint64_t tbl_mask=INITIAL_TBL_MASK;
	uint64_t high_addr_mask=INITIAL_HIGH_ADDR_MASK;
	
	for(retries=0;retries<4;retries++,tbl_mask=LEFT_EXPAND_MASK(tbl_mask),high_addr_mask=LEFT_SHRINK_MASK(high_addr_mask)){
		rc=initialize_phy2virt_table(p2v_tbl,virt_base,tbl_mask,high_addr_mask,m_alloc,m_free);
		if(rc)
			return rc;
		rc=add_physical_entries(p2v_tbl,records,nr_records);
		if(!rc)
			return 0;
		uninitialize_phy2virt_table(p2v_tbl,m_free);
	}
	return -1;
}
int initialize_phy2virt_table(struct phy2virt_table *p2v_tbl,
	uint64_t virt_base,
	uint64_t tbl_mask,
	uint64_t hi_addr_mask,
	mem_alloc m_alloc,
	mem_dealloc m_free)
{
	int idx=0;
	int iptr=0;
	int length=0;
	int nr_entries;
	
	p2v_tbl->virt_base=virt_base;
	p2v_tbl->tbl_mask=tbl_mask;
	p2v_tbl->tbl_mask_shift=RIGHT_MOST_BIT(tbl_mask);
	p2v_tbl->high_addr_mask=hi_addr_mask;
	p2v_tbl->high_addr_mask_shift=RIGHT_MOST_BIT(hi_addr_mask);
	
	
	nr_entries=MASK_TRIM_RIGHT_ZERO_BIT(p2v_tbl->tbl_mask)+1;
	length=(nr_entries+1)*sizeof(struct paddr_trans_entry);
	p2v_tbl->aligned_base=(!m_alloc)?malloc(length):m_alloc(length);
	if(!p2v_tbl->aligned_base)
		goto fails;
	printf("[p2v_tbl]:number of entries:%d\n",nr_entries);
	printf("[p2v_tbl]:aligned(original) table base:%p\n",p2v_tbl->aligned_base);
	
	for(idx=0;idx<nr_entries;idx++){
		for(iptr=0;iptr<MAX_ELE_IN_ENTRY;iptr++){
			p2v_tbl->entries[idx].ele[iptr].high_addr=0;
			p2v_tbl->entries[idx].ele[iptr].index=INVALID_INDEX;
		}
	}

	return 0;
	fails:
			return -1;
}
uint64_t translation_phy2virt(struct phy2virt_table *p2v_tbl,uint64_t paddr)
{
	uint64_t vaddr=0;
	int entries_index=(paddr&p2v_tbl->tbl_mask)>>p2v_tbl->tbl_mask_shift;
	uint16_t high_addr=(uint16_t)((paddr&p2v_tbl->high_addr_mask)>>p2v_tbl->high_addr_mask_shift);
	int virt_index=INVALID_INDEX;
	int idx=0;
	
	for(idx=0;idx<MAX_ELE_IN_ENTRY;idx++){
		if(p2v_tbl->entries[entries_index].ele[idx].index==INVALID_INDEX)
			break;
		if(p2v_tbl->entries[entries_index].ele[idx].high_addr==high_addr){
			virt_index=p2v_tbl->entries[entries_index].ele[idx].index;
			break;
		}
	}
	if(virt_index!=INVALID_INDEX)
		vaddr=p2v_tbl->virt_base+virt_index*HUGEPAGE_2M+(uint64_t)(paddr&HUGEPAGE_2M_MASK);
	return vaddr;
	
}
void dump_phy2virt_table(struct phy2virt_table *p2v_tbl)
{
	int nr_entries=MASK_TRIM_RIGHT_ZERO_BIT(p2v_tbl->tbl_mask)+1;
	int max_depth=0;
	int idx=0;
	int iptr=0;
	for(idx=0;idx<nr_entries;idx++){
		for(iptr=0;iptr<MAX_ELE_IN_ENTRY;iptr++){
			if(p2v_tbl->entries[idx].ele[iptr].index==INVALID_INDEX)
				break;	
			printf("%p %d\n",(void*)MAKE_PADDR_BY_HIGH_ADDR_AND_TBL_IDX(p2v_tbl,p2v_tbl->entries[idx].ele[iptr].high_addr,idx),(int)p2v_tbl->entries[idx].ele[iptr].index);
		}
		max_depth=max_depth<iptr?iptr:max_depth;
	}
	printf("max p2v translation depth:%d\n",max_depth);
}
void uninitialize_phy2virt_table(struct phy2virt_table *p2v_tbl,
	mem_dealloc m_free)
{
	if(p2v_tbl->aligned_base)
		m_free?m_free(p2v_tbl->aligned_base):free(p2v_tbl->aligned_base);
	p2v_tbl->aligned_base=NULL;
}

int setup_virt2phy_translation_table(struct virt2phy_table *v2p_tbl,
	uint64_t virt_base,
	struct dpdk_mmap_record *records,
	int nr_records,
	mem_alloc m_alloc,
	mem_dealloc m_free)
{
	int idx;
	int entries_index;
	uint64_t max_virt_addr=0;
	
	for(idx=0;idx<nr_records;idx++)
		max_virt_addr=max_virt_addr>records[idx].vir_address?max_virt_addr:records[idx].vir_address;
	
	v2p_tbl->virt_base=virt_base;
	v2p_tbl->nr_real_pages=1+(int)((max_virt_addr-virt_base)/HUGEPAGE_2M);
	
	if(0>=v2p_tbl->nr_real_pages)
		return -1;
	v2p_tbl->aligned_base=m_alloc?m_alloc(v2p_tbl->nr_real_pages*sizeof(struct vaddr_trans_entry)):
		malloc(v2p_tbl->nr_real_pages*sizeof(struct vaddr_trans_entry));
	if(!v2p_tbl->aligned_base)
		return -2;

	printf("[v2p_tbl]nr_pages:%d(real_entries:%d)\n",nr_records,v2p_tbl->nr_real_pages);
	printf("[v2p_tbl]allocated aligned address:%p\n",(void*)v2p_tbl->aligned_base);
	for(idx=0;idx<v2p_tbl->nr_real_pages;idx++){
		v2p_tbl->entries[idx].is_valid=0;
		v2p_tbl->entries[idx].paddr=0x0;
	}

	for(idx=0;idx<nr_records;idx++){
		entries_index=(int)((records[idx].vir_address-v2p_tbl->virt_base)/HUGEPAGE_2M);
		if(entries_index>=v2p_tbl->nr_real_pages)
			return -3;
		v2p_tbl->entries[entries_index].is_valid=1;
		v2p_tbl->entries[entries_index].paddr=records[idx].phy_address;
	}
	return 0;
}
void uninitialize_virt2phy_table(struct virt2phy_table * v2p_tbl,
	mem_dealloc m_free)
{
	if(v2p_tbl->aligned_base){
		m_free?m_free(v2p_tbl->aligned_base):free(v2p_tbl->aligned_base);
		v2p_tbl->aligned_base=NULL;
	}
}
uint64_t translation_virt2phy(struct virt2phy_table *v2p_tbl,uint64_t vaddr)
{
	int entries_index=(uint64_t)((vaddr-v2p_tbl->virt_base)/HUGEPAGE_2M);
	if(entries_index>=v2p_tbl->nr_real_pages||entries_index<0)
		return 0;
	if(v2p_tbl->entries[entries_index].is_valid==0)
		return 0;
	return v2p_tbl->entries[entries_index].paddr+(uint64_t)(vaddr&HUGEPAGE_2M_MASK);
}
void dump_virt2phy_table(struct virt2phy_table* v2p_tbl)
{
	int idx;
	for(idx=0;idx<v2p_tbl->nr_real_pages;idx++){
		if(v2p_tbl->entries[idx].is_valid==0)
			continue;
		printf("%p %p\n",(void*)(v2p_tbl->virt_base+idx*HUGEPAGE_2M),(void*)v2p_tbl->entries[idx].paddr);
	}
}