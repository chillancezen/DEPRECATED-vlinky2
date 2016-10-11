#ifndef _VLINK_QUEUE_H
#define _VLINK_QUEUE_H
#include <stdint.h>
#include <my_config.h>

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))


#if defined(NATIVE_LINUX)
typedef uint64_t offset_t;
typedef uint32_t my_int_32;
typedef uint8_t my_int_8;

#define	WRITE_MEM_WB() do{\
	asm volatile ("" : : : "memory");\
} while(0)
#endif

#pragma pack(1)

/*here sub-channel means queue*/

struct queue_element{
	union{
		offset_t  rte_pkt_offset;
		offset_t  level1_offset;
	};
	union {
		offset_t  rte_data_offset;
		offset_t  level2_offset;
	};
}__attribute__((packed));
struct queue_stub{
	union{

		 my_int_8 stuff[128];/*preserve enough room in case new features per sub-channel(queue) is needed*/
	};
	my_int_32 ele_num;
	my_int_32 ele_size;
	my_int_32 front_ptr;/*update by consumer,read by both*/
	my_int_32 rear_ptr;/*updtae by produccer,read by both*/
	struct queue_element  records[0];/*this must be last field*/
}__attribute__((packed));

int initialize_queue(void *buffer,int size,int queue_ele_nr);
int queue_available_quantum(struct queue_stub *stub);
int queue_empty(struct queue_stub *stub);
int queue_full(struct queue_stub *stub);
int enqueue_single(struct queue_stub* stub,struct queue_element *ele);
int dequeue_single(struct queue_stub *stub,struct queue_element*ele);
int enqueue_bulk(struct queue_stub *stub,struct queue_element *ele_arr,int length);
int dequeue_bulk(struct queue_stub * stub,struct queue_element *ele_arr,int max_length);

#endif
