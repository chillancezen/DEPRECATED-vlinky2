#include <message.h>
#include <errorcode.h>
#include <string.h>

int message_builder_init(struct message_builder * builder,char * buffer,int buffer_length)
{
	if(buffer_length<sizeof(struct message_header))
		return -1;
	if(!buffer)
		return -2;
	memset(builder,0x0,sizeof(struct message_builder));
	builder->buffer=(uint8_t*)buffer;
	builder->buffer_length=buffer_length;
	builder->message_header_ptr=(struct message_header*)builder->buffer;

	/*update the message_header fields*/
	builder->message_header_ptr->reserved=0;
	builder->message_header_ptr->version_number=MESSAGE_VERSION;
	builder->message_header_ptr->total_length=sizeof(struct message_header);
	builder->message_header_ptr->tlv_number=0;
	/*update meta data*/
	builder->buffer_iptr=sizeof(struct message_header);
	builder->initialized=TRUE;
	return 0;
}
int message_builder_add_tlv(struct message_builder *builder,struct tlv_header* tlv,void * value)
{
	if(!builder->initialized)
		return -1;
	int needed_room=sizeof(struct tlv_header)+tlv->length;
	int available_room=builder->buffer_length-builder->buffer_iptr;
	if(needed_room>available_room)
		return -2;
	struct tlv_header * tlv_tmp;
	/*update TLV room*/
	tlv_tmp=(struct tlv_header*)(builder->buffer+builder->buffer_iptr);
	tlv_tmp->type=tlv->type;
	tlv_tmp->length=tlv->length;
	builder->buffer_iptr+=sizeof(struct tlv_header);
	/*update value part*/
	memcpy(builder->buffer_iptr+(char*)builder->buffer,value,tlv->length);
	builder->buffer_iptr+=tlv->length;
	/*update message_header part*/
	builder->message_header_ptr->total_length+=needed_room;
	builder->message_header_ptr->tlv_number++;
	
	return 0;
	
}
int message_iterate(struct message_builder *builder,void * argument,void (*callback)(struct tlv_header* _tlv,void *_value,void *_arg))
{
	int idx=0;
	int iptr=sizeof(struct message_header);
	struct tlv_header * tlv_tmp;
	void * value;
	for(idx=0;idx<builder->message_header_ptr->tlv_number;idx++){
		tlv_tmp=(struct tlv_header*)(builder->buffer+iptr);
		value=(void*)(builder->buffer+iptr+sizeof(struct tlv_header));
		if(callback)
			(*callback)(tlv_tmp,value,argument);
		iptr+=sizeof(struct tlv_header)+tlv_tmp->length;
	}
	return 0;
}
int message_parse_raw(struct message_header *header,void* tlv_start,struct tlv_header * tlv,void**value)
{/*tlv->type must be set before calling this function*/
	int idx=0;
	int iptr=0;
	struct tlv_header * tlv_tmp;
	for(idx=0;idx<header->tlv_number;idx++){
		tlv_tmp=(struct tlv_header*)(iptr+(char*)tlv_start);
		if(tlv_tmp->type==tlv->type){
			tlv->length=tlv_tmp->length;
			*value=(void*)(sizeof(struct tlv_header)+iptr+(char*)tlv_start);
			return 0;
		}
		iptr+=sizeof(struct tlv_header)+tlv_tmp->length;
	}
	return -1;
}
/*this does not requires header and tlv_start must be consecutive*/
int message_iterate_raw(struct message_header *header,void *tlv_start,void * argument,void (*callback)(struct tlv_header* _tlv,void *_value,void *_arg))
{
	int idx=0;
	int iptr=0;
	struct tlv_header *tlv_tmp;
	void *value;
	for(idx=0;idx<header->tlv_number;idx++){
		tlv_tmp=(struct tlv_header*)(iptr+(char*)tlv_start);
		value=(void*)(sizeof(struct tlv_header)+iptr+(char*)tlv_start);
		if(callback)
			(*callback)(tlv_tmp,value,argument);
		iptr+=sizeof(struct tlv_header)+tlv_tmp->length;
	}
	return 0;
}
void def_callback(struct tlv_header*tlv,void * value,void * arg)
{
	printf("%x %d %02x:%02x\n",tlv->type,tlv->length,*(char*)value,*(1+(char*)value));
}

int send_data_with_quantum(int fd,char * buffer, int length)
{
	int rc;
	int pending=length;
	int iptr=0;
	while(pending>0){
		rc=send(fd,buffer+iptr,pending,0);
		if(!rc)
			break;
		pending-=rc;
		iptr+=rc;
	}
	return iptr;
}

int recv_data_with_quantum(int fd,char *buffer,int length)
{
	int rc;
	int pending = length;
	int iptr = 0;
	while(pending >0){
		rc=recv(fd,buffer+iptr,pending,0);
		if(!rc)
			break;
		pending-=rc;
		iptr+=rc;
	}
	return iptr;
}