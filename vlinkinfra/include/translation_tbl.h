#ifndef _TRANSLATION_TBL
#define _TRANSLATION_TBL
/*this is optimized for QEMU/CONTAINER address translation where physical pages are mapped into contiguous one*/

#include <inttypes.h>
#include <dpdk-mapping.h>
typedef void*(*mem_alloc)(int length);/*allignment is not the concern,after allocation ,please handle it ourselves once you reserve enough space*/
typedef void (*mem_dealloc)(void * mem);

#define MAX_ELE_IN_ENTRY 16
#define INVALID_INDEX 0xffff
#define HUGEPAGE_2M_SHIFT 21 

struct paddr_offset_index{
	uint16_t high_addr;
	uint16_t index;
}__attribute__((packed));


struct paddr_trans_entry{
	struct paddr_offset_index ele[MAX_ELE_IN_ENTRY];
}__attribute__((aligned(64)));

#define LEFT_EXPAND_MASK(m) (((m)<<1)|(m))
#define LEFT_SHRINK_MASK(m) (((m)<<1)&(m))
#define MASK_TRIM_RIGHT_ZERO_BIT(m) ({ \
	int idx=0; \
	uint64_t v=(m); \
	for(idx=0;idx<64;idx++,v=v>>1) \
		if (v&0x1) \
			break; \
	v; \
})

#define RIGHT_MOST_BIT(m) ({ \
	int idx=0; \
	for(idx=0;idx<64;idx++) \
		if((m)&(((uint64_t)1)<<idx)) \
			break; \
	idx; \
})
#define MAKE_PADDR_BY_HIGH_ADDR_AND_TBL_IDX(tbl,hi,tbl_idx) ({\
	uint64_t addr; \
	addr=((uint64_t)(hi))<<(tbl)->high_addr_mask_shift; \
	addr|=((uint64_t)(tbl_idx))<<(tbl)->tbl_mask_shift; \
	addr; \
})
	
	
#define INITIAL_TBL_MASK ((uint64_t)0xFFE00000)
#define INITIAL_HIGH_ADDR_MASK ((uint64_t)0xffff00000000)
#define ALIGN_ADDR(addr,alignment) (((uint64_t)(addr))&~((alignment)-1)) /*alignment must be power of TWO,
usually CACHE LINE ALLIGNMENT of x86_64 platform is 64*/


struct phy2virt_table{
	uint64_t virt_base;
	
	union{
		void * aligned_base;
		struct paddr_trans_entry* entries;
	};
	
	uint64_t tbl_mask;
	uint64_t high_addr_mask;
	
	int tbl_mask_shift;
	int high_addr_mask_shift;
	
};

void dump_phy2virt_table(struct phy2virt_table *p2v_tbl);
void uninitialize_phy2virt_table(struct phy2virt_table *p2v_tbl,mem_dealloc m_free);
int initialize_phy2virt_table(struct phy2virt_table *p2v_tbl,
	uint64_t virt_base,
	uint64_t tbl_mask,
	uint64_t hi_addr_mask,
	mem_alloc m_alloc,
	mem_dealloc m_free);
int setup_phy2virt_translation_table(struct phy2virt_table *p2v_tbl,
	uint64_t virt_base,
	struct dpdk_mmap_record *records,
	int nr_records,
	mem_alloc m_alloc,
	mem_dealloc m_free);
uint64_t translation_phy2virt(struct phy2virt_table *p2v_tbl,uint64_t paddr);


struct vaddr_trans_entry{
	uint64_t is_valid:1;
	uint64_t paddr:56;
}__attribute__((aligned(8)));

struct virt2phy_table{
	uint64_t virt_base;
	int nr_real_pages;
	union{
		void * aligned_base;
		struct vaddr_trans_entry * entries;
	};
};
int setup_virt2phy_translation_table(struct virt2phy_table *v2p_tbl,
	uint64_t virt_base,
	struct dpdk_mmap_record *records,
	int nr_records,
	mem_alloc m_alloc,
	mem_dealloc m_free);
void uninitialize_virt2phy_table(struct virt2phy_table * v2p_tbl,
	mem_dealloc m_free);

uint64_t translation_virt2phy(struct virt2phy_table *v2p_tbl,uint64_t vaddr);
void dump_virt2phy_table(struct virt2phy_table* v2p_tbl);

#endif
