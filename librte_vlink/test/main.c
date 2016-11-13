/*-
 *   BSD LICENSE
 *
 *   Copyright(c) 2010-2014 Intel Corporation. All rights reserved.
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <sys/queue.h>

#include <rte_memory.h>
#include <rte_memzone.h>
#include <rte_launch.h>
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_lcore.h>
#include <rte_debug.h>
#include <rte_vlink.h>
#include <rte_prefetch.h>
#include <rte_cycles.h>
static int dummy_packets_handle(struct rte_vlink_mbuf * vbuf,int nr_buf)
{
	int idx=0;
	unsigned char *ptr;
	int iptr;
	unsigned char cute;
	int cnt=0;
	
	for(idx=0;idx<nr_buf;idx++){
		ptr=vbuf[idx].data_ptr;
		for(iptr=0;iptr<(int)vbuf[idx].dpdk_mbuf->pkt_len;iptr++){
			cute=ptr[iptr];
			cute++;
			cute--;
			ptr[iptr]=cute;
			cnt++;
		}
	}
	return cnt;
}
void dummy_task(int link_num,int channel_num)
{
	int idx=0;
	struct rte_vlink_mbuf vbuf[DEFAULT_RXTX_BURST_SIZE];
	int nr_rx,nr_tx;
	int rc;
	while(1){
		nr_rx=rte_vlink_rx_burst(link_num,channel_num,vbuf,DEFAULT_RXTX_BURST_SIZE);
		for(idx=0;idx<nr_rx;idx++){
			
			rte_prefetch0(vbuf[idx].data_ptr);
			
			/*printf("%02x:%02x:%02x:%02x:%02x:%02x %d\n",((char*)vbuf[idx].data_ptr)[0]
				,((char*)vbuf[idx].data_ptr)[1]
				,((char*)vbuf[idx].data_ptr)[2]
				,((char*)vbuf[idx].data_ptr)[3]
				,((char*)vbuf[idx].data_ptr)[4]
				,((char*)vbuf[idx].data_ptr)[5],
				vbuf[idx].dpdk_mbuf->pkt_len);*/
		}
		rc=0;
		for(idx=0;idx<1000000000;idx++)
			rc+=dummy_packets_handle(vbuf,nr_rx);
		
		if(nr_rx){
			nr_tx=rte_vlink_tx_burst(link_num,channel_num,vbuf, nr_rx);
			if(nr_tx!=nr_rx)
				printf("remain:%d\n",nr_rx-nr_tx);
		}
	}
}

static int
lcore_hello(__attribute__((unused)) void *arg)
{
	uint64_t rc;
	uint64_t start,end,start1,end1;
	unsigned lcore_id;
	int idx=0;
	lcore_id = rte_lcore_id();
	struct rte_vlink_mbuf vbuf[DEFAULT_RXTX_BURST_SIZE];
	int nr_rx,nr_tx;
	int link_num=-1;
	int channel_num;
	if(lcore_id==1){
		link_num=0;
		channel_num=0;
	}else if(lcore_id==2){
		link_num=0;
		channel_num=1;
	}
	/*else if(lcore_id==3){
		link_num=0;
		channel_num=2;
	}else if(lcore_id==4){
		link_num=0;
		channel_num=3;
	}else if(lcore_id==5){
		link_num=0;
		channel_num=4;
	}*/
	if(link_num==-1){
		printf("%d exit\n",lcore_id);
		return 0;
	}
	while(1){
		start=rte_get_tsc_cycles();
		nr_rx=rte_vlink_rx_burst(link_num,channel_num,vbuf,DEFAULT_RXTX_BURST_SIZE);
		end=rte_get_tsc_cycles();
	//	printf("1.%"PRIu64" %d\n",end-start,nr_rx);
		for(idx=0;idx<nr_rx;idx++){
			
			//rte_prefetch0(vbuf[idx].data_ptr);
			
			/*printf("%02x:%02x:%02x:%02x:%02x:%02x %d\n",((char*)vbuf[idx].data_ptr)[0]
				,((char*)vbuf[idx].data_ptr)[1]
				,((char*)vbuf[idx].data_ptr)[2]
				,((char*)vbuf[idx].data_ptr)[3]
				,((char*)vbuf[idx].data_ptr)[4]
				,((char*)vbuf[idx].data_ptr)[5],
				vbuf[idx].dpdk_mbuf->pkt_len);*/
		}
		rc=0;
		start1=rte_get_tsc_cycles();
		for(idx=0;idx<1;idx++)
			rc+=dummy_packets_handle(vbuf,nr_rx);
		end1=rte_get_tsc_cycles();
		//printf("2.%"PRIu64" %"PRIu64" %"PRIu64"\n",end-start,end1-start1,rc);
		if(nr_rx){
			nr_tx=rte_vlink_tx_burst(link_num,channel_num,vbuf, nr_rx);
			if(nr_tx!=nr_rx)
				printf("remain:%d\n",nr_rx-nr_tx);
		}
	}
	
	return 0;
}

int
main(int argc, char **argv)
{
	int ret;
	unsigned lcore_id;
	

	#if 0
	int  link_num=atoi(argv[1]);
	int  channel_num=atoi(argv[2]);
	dummy_task(link_num,channel_num);
	
	#else
	ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_panic("Cannot init EAL\n");

	ret=rte_vlink_init();
		if(ret<0)
			rte_panic("can not init vlink\n");

	/* call lcore_hello() on every slave lcore */
	RTE_LCORE_FOREACH_SLAVE(lcore_id) {
		rte_eal_remote_launch(lcore_hello, NULL, lcore_id);
	}

	/* call it on master lcore too */
	lcore_hello(NULL);

	getchar();
	#endif
	//rte_eal_mp_wait_lcore();
	return 0;
}
