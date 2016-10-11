/*shared config between AGENT and dpdk,even qemu*/
#ifndef _MY_CONFIG_H
#define _MY_CONFIG_H

#define NATIVE_LINUX 

#define DEFAULT_CHANNEL_QUEUE_LENGTH (4096)
#define DEFAULT_CHANNEL_QUEUE_ELEMENT_SIZE (sizeof(uint64_t)*2)
#define DEFAULT_CHANNEL_QUEUE_HEADER_ROOM 512/*queue stub included*/


#define CHANNEL_SIZE (4*(DEFAULT_CHANNEL_QUEUE_ELEMENT_SIZE*DEFAULT_CHANNEL_QUEUE_LENGTH+DEFAULT_CHANNEL_QUEUE_HEADER_ROOM))
#define SUB_CHANNEL_SIZE ((DEFAULT_CHANNEL_QUEUE_ELEMENT_SIZE*DEFAULT_CHANNEL_QUEUE_LENGTH+DEFAULT_CHANNEL_QUEUE_HEADER_ROOM))

#define RX_IN_DATA_CHANNEL 0
#define FREE_IN_DATA_CHANNEL 1
#define ALLOC_IN_DATA_CHANNEL 2
#define TX_IN_DATA_CHANNEL 3


#define _sub_channel_offset(sub_channel)  ((sub_channel)*SUB_CHANNEL_SIZE)

#define rx_sub_channel_offset() _sub_channel_offset(RX_IN_DATA_CHANNEL)
#define free_sub_channel_offset() _sub_channel_offset(FREE_IN_DATA_CHANNEL)
#define alloc_sub_channel_offset() _sub_channel_offset(ALLOC_IN_DATA_CHANNEL)
#define tx_sub_channel_offset() _sub_channel_offset(TX_IN_DATA_CHANNEL)

#define channel_base_offset(channel_nr)  (CHANNEL_SIZE*(channel_nr))
#endif



