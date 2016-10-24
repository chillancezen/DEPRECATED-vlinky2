#include <stdio.h>
#include <uio_helper.h>
#include <vlink_device.h>
#define TEST_BURST_SIZE 64

struct dpdk_buf{
        union{
                unsigned char dummy0[128];
                struct {
                        unsigned char dummy1[0];
                        void * buf_addr;/*offset is 0*/
                };
                struct{
                        unsigned char dummy2[16];
                        uint16_t buf_len;
                        uint16_t data_off;
                };
                struct {
                        unsigned char dummy3[36];
                        uint32_t pkt_len;
                        uint16_t data_len;
                };
        };
}__attribute__((packed));


int main(int argc,char**argv)
{
	struct vlink_device device;
	vlink_device_initialize(&device);
	establish_translation_tbl(&device);
	scan_virtual_link(&device);
	int link_num=(argc>1)?atoi(argv[1]):0;
	int channel_num=(argc>2)?atoi(argv[2]):0;
	printf("pmd test on link:%d channel:%d\n",link_num, channel_num);
	
	struct queue_element qe_rx[TEST_BURST_SIZE];
    int nr_dequeued_rx;
    int nr_enqueued_tx;
	int cnt_total_rx=0;
	struct dpdk_buf *buf;
	int idx;
	int ipt_idx;
	unsigned char * pkt;
	
	while(1){
		nr_dequeued_rx=dequeue_bulk(device.us_link_desc[link_num].data[channel_num].rx_stub,qe_rx,TEST_BURST_SIZE);

		for(idx=0;idx<nr_dequeued_rx;idx++){
			buf=PTR(struct dpdk_buf*,translate_phy_address(device.p2v_hashtbl,qe_rx[idx].rte_pkt_offset));
			pkt=PTR_OFFSET_BY(unsigned char*,translate_phy_address(device.p2v_hashtbl,qe_rx[idx].rte_data_offset),buf->data_off);
			//printf("rx %02x:%02x:%02x:%02x:%02x:%02x %d\n",pkt[0+6-6],pkt[1+6-6],pkt[2+6-6],pkt[3+6-6],pkt[4+6-6],pkt[5+6-6],buf->data_len);
#if 0 
		for(ipt_idx=0;ipt_idx<14/*buf->pkt_len*/;ipt_idx++)
		    pkt[ipt_idx]=pkt[12+ipt_idx-12];
#endif
		}
		if(nr_dequeued_rx){
        	nr_enqueued_tx=enqueue_bulk(device.us_link_desc[link_num].data[channel_num].tx_stub,qe_rx,nr_dequeued_rx);
                        /*if there still are some pkts who remain unsent,send them into free*/
			if((nr_dequeued_rx-nr_enqueued_tx)>0)
				enqueue_bulk(device.us_link_desc[link_num].data[channel_num].free_stub,&qe_rx[nr_enqueued_tx],nr_dequeued_rx-nr_enqueued_tx);
            }
	}
	
	#if 0
	int nr=find_uio_device_number_by_vendor_and_device_id(0xcccc,0x2222);
	int bar;
	int size;
	int rc;
	int map_num=1;
	rc=get_uio_memory_info(nr,map_num,&bar,&size);

	printf("[x]dev number:%d\n",nr);
	printf("[map0]:bar:%d   size:%"PRIx64"\n",bar,size);

	uint64_t mapped_address=map_uio_region_memory_from_char_device_alligned(nr,map_num,size);
	printf("[preserved address]:0x%"PRIx64"(0x%"PRIx64")\n",mapped_address,0);
	
	getchar();
	#endif
	#if 0
							  
	int nr=find_uio_device_number_by_vendor_and_device_id(0x1af4,0x100);
	int start;
	int size;
	nr=get_uio_port_info(0,0,&start,&size);

	int bar;
	int size1;
	nr=get_uio_memory_info(0,0,&bar,&size1);

	uint64_t addr=map_uio_region_memory(0,bar,0,size1);
	 printf("%s\n",addr);
	printf("[x]mapped address:0x%"PRIx64"\n",addr);
	strcpy((char*)addr,"hello vlink\n");
	printf("%s\n",addr);	

	getchar();
	addr=map_uio_region_memory_from_char_device(0,0,size1);
	printf("[x]mapped address:0x%"PRIx64"\n",addr);
	
	strcpy((char*)addr,"hello,cute vlink");	
	printf("%s\n",addr);
	getchar();
	#endif
	
	return 0;
}
