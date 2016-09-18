#ifndef __MESSAGE
#define __MESSAGE
#include<stdint.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>

struct message_header {
	uint8_t version_number;
	uint8_t reserved;
	uint16_t tlv_number;
	uint32_t total_length;
}__attribute__((packed));


struct tlv_header{
	uint16_t type;
	uint16_t length;
}__attribute__((packed));

struct message_builder{
	uint8_t initialized;
	uint8_t * buffer;
	uint32_t buffer_length;
	uint32_t buffer_iptr;
	struct message_header * message_header_ptr;
	
};
#define MESSAGE_TLV_TYPE_BUS_BASE 0x100

#define _(code)  VLINK_STATUS_CODE_##code,
enum VLINK_STATUS_CODE{
_(NONE)
_(OK)
_(NOT_FOUND)
_(EXIST)
_(COMMON_ERROR)
_(INVALID_PARAMETER)
};
#undef _

#define _(role) VLINK_ROLE_##role,
enum VLINK_ROLE{
_(NONE)
_(DPDK)
_(QEMU)
};
#undef _

	
#define TO_INT32(value) \
	(*(uint32_t*)(value))


enum MESSAGE_TLV_TYPE
{
	/*NULL*/MESSAGE_TLV_VLINK_INIT_START=MESSAGE_TLV_TYPE_BUS_BASE,
	/*NULL*/MESSAGE_TLV_VLINK_INIT_END,
	
	/*NULL*/MESSAGE_TLV_VLINK_REQUEST_START,
	/*NULL*/MESSAGE_TLV_VLINK_REQUEST_END,

	/*uint32_t*/MESSAGE_TLV_VLINK_COMMON_STATUS_CODE,
	
	/*string*/MESSAGE_TLV_VLINK_COMMON_VM_DOMAIN,
	/*uint32_t*/MESSAGE_TLV_VLINK_COMMON_VM_MAX_CHANNELS,

	/*string*/MESSAGE_TLV_VLINK_COMMON_VLINK_NAME,
	/*uint32_t*/MESSAGE_TLV_VLINK_COMMON_VLINK_NR_CHANNELS,
	/*uint32_t array*/MESSAGE_TLV_VLINK_COMMON_VLINK_CHANNELS,
	/*uint32_t*/MESSAGE_TLV_VLINK_COMMON_VLINK_ROLE,
	
	#if 0
	/*string*/MESSAGE_TLV_TYPE_JOIN_BUS=MESSAGE_TLV_TYPE_BUS_BASE,/*before anything ,endpoint must join the bus*/
	/*int32*/MESSAGE_TLV_TYPE_JOIN_BUS_QUANTUM,	
	/*NULL*/MESSAGE_TLV_TYPE_JOIN_BUS_END,
	
	/*NULL*/MESSAGE_TLV_TYPE_READ_BUS_START,
	/*NULL*/MESSAGE_TLV_TYPE_READ_BUS_END,/*it must be */

	/*NULL*/MESSAGE_TLV_TYPE_WRITE_BUS_START,/*must be set before common entries*/
	/*NULL*/MESSAGE_TLV_TYPE_WRITE_BUS_END,/*must be set after common entries*/
	
	/*int32*/MESSAGE_TLV_TYPE_START_BLOCK_INDEX,
	/*int32*/MESSAGE_TLV_TYPE_NB_OF_BLOCKS,
	/*int64*/MESSAGE_TLV_TYPE_TARGET_VERSION,
	/*NULL*/MESSAGE_TLV_TYPE_LOCK_BUS,
	
	/*NULL*/MESSAGE_TLV_TYPE_ACK_BUS,/*ack endpoit the result of the issued bus operation*/
	/*NULL*/MESSAGE_TLV_TYPE_ACK_BUS_OPS_SUCCEED,
	/*NULL*/MESSAGE_TLV_TYPE_ACK_BUS_OPS_FAIL,
	/*NULL*/MESSAGE_TLV_TYPE_DATA,
	/*NULL*/MESSAGE_TLV_TYPE_DATA_NOT_MODIFIED,
	/*NULL*/MESSAGE_TLV_TYPE_DATA_NOT_MATCHED,
	#endif
	
	MESSAGE_TLV_TYPE_END/*indicator of ending*/
};
#define MESSAGE_VERSION 0x1

/*messager_builder must be:mb and tlv_header must tlv*/

#define TLV_ADD_UINT32(_type,_value_ptr) {\
	tlv.type=(_type);\
	tlv.length=4;\
	message_builder_add_tlv(&mb,&tlv,(_value_ptr));\
}

#define TLV_ADD_STRING(_type,_string) {\
	tlv.type=(_type);\
	tlv.length=strlen(_string);\
	message_builder_add_tlv(&mb,&tlv,(_string));\
}

#define TLV_ADD_MEM(_type,_mem,_len) {\
	tlv.type=(_type);\
	tlv.length=(_len);\
	message_builder_add_tlv(&mb,&tlv,(_mem));\
}

int message_builder_init(struct message_builder * builder,char * buffer,int buffer_length);
int message_builder_add_tlv(struct message_builder *builder,struct tlv_header* tlv,void * value);
int message_iterate(struct message_builder *builder,void * argument,void (*callback)(struct tlv_header*,void *,void *));
int message_parse_raw(struct message_header *header,void* tlv_start,struct tlv_header * tlv,void**value);
int message_iterate_raw(struct message_header *header,void *tlv_start,void * argument,void (*callback)(struct tlv_header* _tlv,void *_value,void *_arg));
void def_callback(struct tlv_header*tlv,void * value,void * arg);
int send_data_with_quantum(int fd,char * buffer, int length);
int recv_data_with_quantum(int fd,char *buffer,int length);

#endif


