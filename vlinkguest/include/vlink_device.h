#ifndef _VLINK_DEVICE
#define _VLINK_DEVICE

#include <inttypes.h>
#include <front-config.h>
#include <dpdk-mapping.h>
#include <my_config.h>
#include <vlink_channel.h>
#include <vlink_queue.h>
#include <translation_tbl.h>

#define MAX_CHANNEL_PER_LINK 16 /*should not be lower than MAX_LINK_CHANNEL in agent.h*/

struct data_channel_desc{
	struct queue_stub * rx_stub;
	struct queue_stub * free_stub;
	struct queue_stub * alloc_stub;
	struct queue_stub * tx_stub;

	uint64_t rx_pkts;
	uint64_t rx_bytes;
	uint64_t tx_pkts;
	uint64_t tx_bytes;
	uint64_t rx_errors;
	uint64_t tx_errors;
};
struct vlink_desc{
	int ctrl_channel_index;	
	struct ctrl_channel *ctrl;
	struct data_channel_desc data[MAX_CHANNEL_PER_LINK];
};
struct vlink_device{
	int uio_device_num;
	/*region 0*/
	/*should be port0 in UIO*/
	uint32_t bar0_base;
	uint32_t bar0_size;
	struct vlink_desc us_link_desc[MAX_LINKS]; /*store controller channel index of each link,-1 indicates non-existence*/
	
	/*region 2:dpdk hugepage mmaping*/
	/*should be map0 in UIO*/
	uint32_t nr_host_hugepages;
	uint32_t host_mapping_size;
	uint64_t host_hugepage_base;/*this is mapped and aligned*/

	/*region 3:channel memory mmaping,alignment is not essential*/
	/*should be map1 in UIO*/
	uint32_t ctrl_channel_size;
	uint64_t ctrl_channel_base;

	/*addresss translation table*/
	#if 0 /*deprecated*/
	struct dpdk_physeg_str * p2v_hashtbl;
	struct dpdk_physeg_str * v2p_hashtbl;
	#endif
	struct phy2virt_table p2v_tbl;
	struct virt2phy_table v2p_tbl;
	
	int fd_dummy;
};

int vlink_device_initialize(struct vlink_device * device);
int establish_translation_tbl(struct vlink_device * device,mem_alloc m_alloc,mem_dealloc m_free);
int scan_virtual_link(struct vlink_device * device);

#endif