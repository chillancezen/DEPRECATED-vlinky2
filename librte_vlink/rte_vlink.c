#include <rte_vlink.h>
#include <vlink_device.h>
#include <stdio.h>
#include <string.h>
#include <rte_malloc.h>

struct vlink_device gdevice;/*there chould be only one devcie which chould contain 16 virtual links,make it globally*/

void * rte_allocator(int len);
void * rte_allocator(int len)
{
	return rte_malloc(NULL,len,64);/*CACHE LINE aligned*/
}
void rte_deallocator(void*ptr);
void rte_deallocator(void*ptr)
{
	rte_free(ptr);
}
int rte_vlink_init(void)
{
	int rc=vlink_device_initialize(&gdevice);
	if(rc){
		printf("[x]please make sure the device is taken over by uio_vlink driver\n");
		goto fails;
	}
	rc=establish_translation_tbl(&gdevice,rte_allocator,rte_deallocator);
	if(rc){
		printf("[x]something is wrong when setting up vlink translation table\n");
		goto fails;
	}
	rc=scan_virtual_link(&gdevice);
	return 0;
	fails:
		return -1;
}

int rte_vlink_is_enabled(int link_num)
{
	if(link_num>=MAX_LINKS)
		return -1;
	return ((uint32_t)gdevice.us_link_desc[link_num].ctrl_channel_index)!=0xffffffff
		/*(gdevice.us_link_desc[link_num].ctrl_channel_index>=0)&&(gdevice.us_link_desc[link_num].ctrl_channel_index<MAX_LINKS)*/;
}

int rte_vlink_get_mac(unsigned char * dst,int link_num)
{
	if(link_num>=MAX_LINKS)
			return -1;
	if(!((gdevice.us_link_desc[link_num].ctrl_channel_index>=0)&&(gdevice.us_link_desc[link_num].ctrl_channel_index<MAX_LINKS)))
		return -1;
	if(!gdevice.us_link_desc[link_num].ctrl)/*this should not happen*/
		return -1;
	memcpy(dst,gdevice.us_link_desc[link_num].ctrl->mac_address,6);
	return 0;
}
inline int rte_vlink_ready(int link_num)
{
	if((link_num<0) || link_num>=MAX_LINKS)
		return 0;
	return (((uint32_t)gdevice.us_link_desc[link_num].ctrl_channel_index)!=0xffffffff)&&
			(gdevice.us_link_desc[link_num].ctrl)&&
			(gdevice.us_link_desc[link_num].ctrl->dpdk_connected)&&
			(gdevice.us_link_desc[link_num].ctrl->dpdk_driver_state==DRIVER_STATUS_INITIALIZED);
}
int rte_vlink_get_data_channel_nr(int link_num)
{
	if(rte_vlink_is_enabled(link_num)!=1)
		return -1;
	return gdevice.us_link_desc[link_num].ctrl->nr_data_channels;
}


int rte_vlink_rx_burst(int link_num,int channel_num,struct rte_vlink_mbuf *pbuf,int nr_mbuf)
{
	int ready_to_rx=nr_mbuf<DEFAULT_RXTX_BURST_SIZE?nr_mbuf:DEFAULT_RXTX_BURST_SIZE;
	int nr_rx;
	int idx;
	int iptr=0;
	struct dpdk_native_mbuf *native_buf;
	void *					 data_ptr;
	struct queue_element qe_rx[DEFAULT_RXTX_BURST_SIZE];
	/*make sure the link is ready*/
	if(!rte_vlink_ready(link_num))
		return 0;
	
	nr_rx=dequeue_bulk(gdevice.us_link_desc[link_num].data[channel_num].rx_stub,qe_rx,ready_to_rx);
	for(idx=0;idx<nr_rx;idx++){
		native_buf=(struct dpdk_native_mbuf *)(void*)translation_phy2virt(&gdevice.p2v_tbl,qe_rx[idx].rte_pkt_offset);
		if(!native_buf){
			continue;
		}
		data_ptr=(void*)(qe_rx[idx].rte_data_offset+(char*)native_buf);
		
		pbuf[iptr].link_num=link_num;
		pbuf[iptr].channel_num=channel_num;
		pbuf[iptr].version=gdevice.us_link_desc[link_num].ctrl->version_number;
		pbuf[iptr].dpdk_mbuf=native_buf;
		pbuf[iptr].data_ptr=data_ptr;
		iptr++;
	}
	return iptr;
}

int rte_vlink_tx_burst(int link_num,int channel_num,struct rte_vlink_mbuf *pbuf,int nr_mbuf)
{
	int idx;
	int ready_to_xmit=nr_mbuf<DEFAULT_RXTX_BURST_SIZE?nr_mbuf:DEFAULT_RXTX_BURST_SIZE;
	int nr_to_xmit=0;
	int nr_queued=0;
	int nr_xmited=0;
	int xmit_cnt=0;
	struct queue_element qe_tx[DEFAULT_RXTX_BURST_SIZE];
	int 				 pkt_flag[DEFAULT_RXTX_BURST_SIZE];
	
	if(!rte_vlink_ready(link_num))
		return 0;
	
	for(idx=0;idx<ready_to_xmit;idx++){
		qe_tx[nr_to_xmit].rte_pkt_offset=translation_virt2phy(&gdevice.v2p_tbl,(uint64_t)pbuf[idx].dpdk_mbuf);
		//qe_tx[nr_to_xmit].rte_pkt_offset=(uint64_t)pbuf[idx].dpdk_mbuf;//translate_virt_address(gdevice.v2p_hashtbl,(uint64_t)pbuf[idx].dpdk_mbuf);
		pkt_flag[idx]=0;
		if(!qe_tx[nr_to_xmit].rte_pkt_offset){/*translation exception*/
			continue;
		}
		pkt_flag[idx]=1;
		nr_to_xmit++;
	}
	
	if(nr_to_xmit)
		nr_queued=enqueue_bulk(gdevice.us_link_desc[link_num].data[channel_num].tx_stub,qe_tx,nr_to_xmit);

	if(nr_queued)
		for(idx=0;idx<ready_to_xmit;idx++){
			if(pkt_flag==0)
				continue;
			xmit_cnt++;
			if(xmit_cnt==nr_queued){
				nr_xmited=(idx+1);
				break;
			}
		}
	return nr_xmited;
}

