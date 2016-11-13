#ifndef _LIBRTE_VLINK
#define _LIBRTE_VLINK
#include <inttypes.h>

#define DEFAULT_RXTX_BURST_SIZE 64


/*same offset with struct rte_mbuf,but do not touch any other field*/

struct dpdk_native_mbuf{
        union{
                unsigned char dummy0[128];
                struct {
                        unsigned char dummy1[0];
                        void * buf_addr;
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
				struct {
						unsigned char dummy4[64];
						uint64_t udata64;
				};
        };
}__attribute__((packed));

struct rte_vlink_mbuf {
	uint8_t link_num;
	uint8_t channel_num;
	uint8_t version;
	struct dpdk_native_mbuf * dpdk_mbuf;
	void * 					  data_ptr;
};


/*extern struct vlink_device gdevice;*//*reference to the only one global device*/

int rte_vlink_init(void);
int rte_vlink_is_enabled(int link_num);
int rte_vlink_get_mac(unsigned char * dst,int link_num);
int rte_vlink_get_data_channel_nr(int link_num);
inline int rte_vlink_ready(int link_num);
int rte_vlink_rx_burst(int link_num,int channel_num, struct rte_vlink_mbuf * pbuf,int nr_mbuf);
int rte_vlink_tx_burst(int link_num,int channel_num,struct rte_vlink_mbuf *pbuf,int nr_mbuf);



#endif
