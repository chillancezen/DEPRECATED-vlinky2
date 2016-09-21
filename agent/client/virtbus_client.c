
#include<virtbus_client.h>

#define check_and_goto_tag(false_condition,tag) {\
if((false_condition)) \
	goto tag;\
}

struct client_endpoint * client_endpoint_alloc_and_init(void)
{
	struct client_endpoint * ep=malloc(sizeof(struct client_endpoint));
	int rc;
	if(ep){
		memset(ep,sizeof(struct client_endpoint),0x0);
		
		ep->fd_sock=socket(PF_UNIX,SOCK_STREAM,0);
		if(!ep->fd_sock)
			goto ret_normal;
		#if 0
		ep->server_addr.sin_family=AF_INET;
		ep->server_addr.sin_port=htons(418);
		ep->server_addr.sin_addr.s_addr=inet_addr(server_ip);
		#endif
		memset(&ep->server_addr, 0x0, sizeof(ep->server_addr));
		ep->server_addr.sun_family=AF_UNIX;
		strcpy(ep->server_addr.sun_path,SERVER_UNIX_SOCKET_PATH);
		
		rc=connect(ep->fd_sock,(struct sockaddr*)&ep->server_addr,sizeof(struct sockaddr_un));
		if(rc)
			goto ret_normal;
		ep->is_connected=TRUE;
		
	}
	ret_normal:
	return ep;
}
void client_endpoint_uninit_and_dealloc(struct client_endpoint*cep)
{
	if(cep->is_connected)
		close(cep->fd_sock);
	free(cep);
}


int client_endpoint_request_virtual_link(struct client_endpoint*cep,char * vlink_name,enum VLINK_ROLE _role)
{
	struct message_builder mb;
	struct tlv_header tlv;
	void*dummy;
	uint32_t role=(uint32_t)_role;
	check_and_goto_tag(message_builder_init(&mb,cep->send_buffer,sizeof(cep->send_buffer)),fails);
	
	tlv.type=MESSAGE_TLV_VLINK_REQUEST_START;
	tlv.length=0;
	check_and_goto_tag(message_builder_add_tlv(&mb,&tlv,&dummy),fails);

	tlv.type=MESSAGE_TLV_VLINK_COMMON_VLINK_NAME;
	tlv.length=strlen(vlink_name);
	check_and_goto_tag(message_builder_add_tlv(&mb,&tlv,vlink_name),fails);

	tlv.type=MESSAGE_TLV_VLINK_COMMON_VLINK_ROLE;
	tlv.length=4;
	check_and_goto_tag(message_builder_add_tlv(&mb,&tlv,&role),fails);

	tlv.type=MESSAGE_TLV_VLINK_REQUEST_END;
	tlv.length=0;
	check_and_goto_tag(message_builder_add_tlv(&mb,&tlv,&dummy),fails);

	check_and_goto_tag(send_data_with_quantum(cep->fd_sock,(char*)mb.buffer,mb.buffer_iptr)!=mb.buffer_iptr,fails);

	check_and_goto_tag(recv_tlv_message(cep),fails);

	tlv.type=MESSAGE_TLV_VLINK_COMMON_STATUS_CODE;
	check_and_goto_tag(message_parse_raw((struct message_header *)(cep->recv_buffer),cep->recv_buffer+sizeof(struct message_header),&tlv,&dummy),fails);
	switch(TO_INT32(dummy))
	{
		case VLINK_STATUS_CODE_OK:
			tlv.type=MESSAGE_TLV_VLINK_COMMON_VM_DOMAIN;
			check_and_goto_tag(message_parse_raw((struct message_header *)(cep->recv_buffer),cep->recv_buffer+sizeof(struct message_header),&tlv,&dummy),fails);
			/*strncpy(cep->vm_domain,(char*)dummy,sizeof(cep->vm_domain));*/
			memcpy(cep->vm_domain,dummy,tlv.length);
			cep->vm_domain[tlv.length]='\0';

			tlv.type=MESSAGE_TLV_VLINK_COMMON_VLINK_NAME;
			check_and_goto_tag(message_parse_raw((struct message_header *)(cep->recv_buffer),cep->recv_buffer+sizeof(struct message_header),&tlv,&dummy),fails);
			/*strncpy(cep->vlink_name,(char*)dummy,sizeof(cep->vlink_name));*/
			memcpy(cep->vlink_name,dummy,tlv.length);
			cep->vlink_name[tlv.length]='\0';
			
			tlv.type=MESSAGE_TLV_VLINK_COMMON_VM_MAX_CHANNELS;
			check_and_goto_tag(message_parse_raw((struct message_header *)(cep->recv_buffer),cep->recv_buffer+sizeof(struct message_header),&tlv,&dummy),fails);
			cep->max_channels=TO_INT32(dummy);

			tlv.type=MESSAGE_TLV_VLINK_COMMON_VLINK_NR_CHANNELS;
			check_and_goto_tag(message_parse_raw((struct message_header *)(cep->recv_buffer),cep->recv_buffer+sizeof(struct message_header),&tlv,&dummy),fails);
			cep->allocated_channels=TO_INT32(dummy);

			tlv.type=MESSAGE_TLV_VLINK_COMMON_VLINK_CHANNELS;
			check_and_goto_tag(message_parse_raw((struct message_header *)(cep->recv_buffer),cep->recv_buffer+sizeof(struct message_header),&tlv,&dummy),fails);
			ASSERT((tlv.length==(sizeof(uint32_t)*cep->allocated_channels)));
			memcpy(cep->channels,dummy,tlv.length);

			tlv.type=MESSAGE_TLV_VLINK_COMMON_VLINK_MAC;
			check_and_goto_tag(message_parse_raw((struct message_header *)(cep->recv_buffer),cep->recv_buffer+sizeof(struct message_header),&tlv,&dummy),fails);
			ASSERT(tlv.length==6);
			memcpy(cep->mac_address,dummy,6);
			
			cep->is_joint=!0;
			break;
		default:
			return -1;
			break;
	}
	
	return 0;
	fails:
		return -2;
}
int client_endpoint_init_virtual_link(struct client_endpoint*cep,enum VLINK_ROLE _role,char* vdomain_name,int max_channels,char*vlink_name,char*mac,int nr_channels)
{

	struct message_builder mb;
	struct tlv_header tlv;
	void*dummy;
	uint32_t role=(uint32_t)_role;
	
	check_and_goto_tag(message_builder_init(&mb,cep->send_buffer,sizeof(cep->send_buffer)),fails);
	
	tlv.type=MESSAGE_TLV_VLINK_INIT_START;
	tlv.length=0;
	check_and_goto_tag(message_builder_add_tlv(&mb,&tlv,&dummy),fails);
	
	tlv.type=MESSAGE_TLV_VLINK_COMMON_VM_DOMAIN;
	tlv.length=strlen(vdomain_name);
	check_and_goto_tag(message_builder_add_tlv(&mb,&tlv,vdomain_name),fails);

	tlv.type=MESSAGE_TLV_VLINK_COMMON_VM_MAX_CHANNELS;
	tlv.length=4;
	check_and_goto_tag(message_builder_add_tlv(&mb,&tlv,&max_channels),fails);

	tlv.type=MESSAGE_TLV_VLINK_COMMON_VLINK_NAME;
	tlv.length=strlen(vlink_name);
	check_and_goto_tag(message_builder_add_tlv(&mb,&tlv,vlink_name),fails);
	
	tlv.type=MESSAGE_TLV_VLINK_COMMON_VLINK_NR_CHANNELS;
	tlv.length=4;
	check_and_goto_tag(message_builder_add_tlv(&mb,&tlv,&nr_channels),fails);
	
	tlv.type=MESSAGE_TLV_VLINK_COMMON_VLINK_ROLE;
	tlv.length=4;
	check_and_goto_tag(message_builder_add_tlv(&mb,&tlv,&role),fails);

	tlv.type=MESSAGE_TLV_VLINK_COMMON_VLINK_MAC;
	tlv.length=6;
	check_and_goto_tag(message_builder_add_tlv(&mb,&tlv,mac),fails);

	tlv.type=MESSAGE_TLV_VLINK_INIT_END;
	tlv.length=0;
	check_and_goto_tag(message_builder_add_tlv(&mb,&tlv,&dummy),fails);

	check_and_goto_tag(send_data_with_quantum(cep->fd_sock,(char*)mb.buffer,mb.buffer_iptr)!=mb.buffer_iptr,fails);

	/*recv ACK tlv encapsulated message*/
	check_and_goto_tag(recv_tlv_message(cep),fails);
	
	tlv.type=MESSAGE_TLV_VLINK_COMMON_STATUS_CODE;
	check_and_goto_tag(message_parse_raw((struct message_header *)(cep->recv_buffer),cep->recv_buffer+sizeof(struct message_header),&tlv,&dummy),fails);
	
	switch(TO_INT32(dummy))
	{
		case VLINK_STATUS_CODE_OK:
			tlv.type=MESSAGE_TLV_VLINK_COMMON_VM_DOMAIN;
			check_and_goto_tag(message_parse_raw((struct message_header *)(cep->recv_buffer),cep->recv_buffer+sizeof(struct message_header),&tlv,&dummy),fails);
			/*strncpy(cep->vm_domain,(char*)dummy,sizeof(cep->vm_domain));*/
			memcpy(cep->vm_domain,dummy,tlv.length);
			cep->vm_domain[tlv.length]='\0';
			
			tlv.type=MESSAGE_TLV_VLINK_COMMON_VLINK_NAME;
			check_and_goto_tag(message_parse_raw((struct message_header *)(cep->recv_buffer),cep->recv_buffer+sizeof(struct message_header),&tlv,&dummy),fails);
			/*strncpy(cep->vlink_name,(char*)dummy,sizeof(cep->vlink_name));*/
			memcpy(cep->vlink_name,dummy,tlv.length);
			cep->vlink_name[tlv.length]='\0';
			
			tlv.type=MESSAGE_TLV_VLINK_COMMON_VM_MAX_CHANNELS;
			check_and_goto_tag(message_parse_raw((struct message_header *)(cep->recv_buffer),cep->recv_buffer+sizeof(struct message_header),&tlv,&dummy),fails);
			cep->max_channels=TO_INT32(dummy);

			tlv.type=MESSAGE_TLV_VLINK_COMMON_VLINK_NR_CHANNELS;
			check_and_goto_tag(message_parse_raw((struct message_header *)(cep->recv_buffer),cep->recv_buffer+sizeof(struct message_header),&tlv,&dummy),fails);
			cep->allocated_channels=TO_INT32(dummy);

			tlv.type=MESSAGE_TLV_VLINK_COMMON_VLINK_CHANNELS;
			check_and_goto_tag(message_parse_raw((struct message_header *)(cep->recv_buffer),cep->recv_buffer+sizeof(struct message_header),&tlv,&dummy),fails);
			ASSERT((tlv.length==(sizeof(uint32_t)*cep->allocated_channels)));
			memcpy(cep->channels,dummy,tlv.length);

			tlv.type=MESSAGE_TLV_VLINK_COMMON_VLINK_MAC;
			check_and_goto_tag(message_parse_raw((struct message_header *)(cep->recv_buffer),cep->recv_buffer+sizeof(struct message_header),&tlv,&dummy),fails);
			ASSERT(tlv.length==6);
			memcpy(cep->mac_address,dummy,6);
			
			cep->is_joint=!0;
			break;
		default:
			return -1;
			break;
	}
	return 0;
	
	fails:
		
		return -2;
}
void client_endpoint_uninit_and_dealloc(struct client_endpoint*cep)
{
	if(cep->is_connected)
		close(cep->fd_sock);
	free(cep);
}

#if 0

void client_endpoint_uninit_and_dealloc(struct client_endpoint*cep)
{
	if(cep->is_connected)
		close(cep->fd_sock);
	free(cep);
}
int client_endpoint_connect_to_virtual_bus(struct client_endpoint *cep,char *bus_name,int32_t cache_line_quantum)
{
	struct message_builder mb;
	struct tlv_header tlv;
	void* value;
	void*dummy;
	/*1.build message*/
	int rc=message_builder_init(&mb,cep->send_buffer,sizeof(cep->send_buffer));
	assert(!rc);
	tlv.type=MESSAGE_TLV_TYPE_JOIN_BUS;
	tlv.length=strlen(bus_name);
	rc=message_builder_add_tlv(&mb,&tlv,bus_name);
	assert(!rc);
	tlv.type=MESSAGE_TLV_TYPE_JOIN_BUS_QUANTUM;
	tlv.length=4;
	rc=message_builder_add_tlv(&mb,&tlv,&cache_line_quantum);
	assert(!rc);
	tlv.type=MESSAGE_TLV_TYPE_JOIN_BUS_END;
	tlv.length=0;
	rc=message_builder_add_tlv(&mb,&tlv,&cache_line_quantum);
	assert(!rc);
	/*2.send the message*/
	
	rc=send_data_with_quantum(cep->fd_sock,mb.buffer,mb.buffer_iptr);
	if(rc!=mb.buffer_iptr)
		return FALSE;
	/*3.recv ack message*/
	rc=recv_tlv_message(cep);
	if(rc)
		return -1;
	tlv.type=MESSAGE_TLV_TYPE_ACK_BUS_OPS_SUCCEED;
	rc=message_parse_raw((struct message_header *)(cep->recv_buffer),cep->recv_buffer+sizeof(struct message_header),&tlv,&dummy);
	if(rc)
		return -1;
	tlv.type=MESSAGE_TLV_TYPE_NB_OF_BLOCKS;
	rc=message_parse_raw((struct message_header *)(cep->recv_buffer),cep->recv_buffer+sizeof(struct message_header),&tlv,&value);
	if(rc)
		return -1;
	cep->nr_of_blocks=*(uint32_t*)value;
	cep->is_connected_to_virtual_bus=TRUE;
	return 0;
}
int client_endpoint_read_virtual_bus_raw(struct client_endpoint*cep,int start_block_index,int nr_of_blocks,char*buffer)
{
	struct message_builder builder;
	int dummy;
	void *value;
	struct tlv_header tlv;
	int rc;
	if(cep->is_connected==FALSE||cep->is_connected_to_virtual_bus==FALSE)
		return -1;
	message_builder_init(&builder,cep->send_buffer,sizeof(cep->send_buffer));
	/*1st,bus read started*/
	tlv.type=MESSAGE_TLV_TYPE_READ_BUS_START;
	tlv.length=0;
	message_builder_add_tlv(&builder,&tlv,&dummy);
	/*2nd,bus read common options*/
	tlv.type=MESSAGE_TLV_TYPE_START_BLOCK_INDEX;
	tlv.length=4;
	message_builder_add_tlv(&builder,&tlv,&start_block_index);
	tlv.type=MESSAGE_TLV_TYPE_NB_OF_BLOCKS;
	tlv.length=4;
	message_builder_add_tlv(&builder,&tlv,&nr_of_blocks);
	tlv.type=MESSAGE_TLV_TYPE_READ_BUS_END;
	tlv.length=0;
	message_builder_add_tlv(&builder,&tlv,&dummy);
	/*3rd send them into the network*/
	rc=send_data_with_quantum(cep->fd_sock,builder.buffer,builder.buffer_iptr);
	if(rc!=builder.buffer_iptr)
		return -1;
	/*4th recv the ack message*/
	rc=recv_tlv_message(cep);
	if(rc)
		return -1;
	tlv.type=MESSAGE_TLV_TYPE_DATA;
	rc=message_parse_raw((struct message_header *)(cep->recv_buffer),cep->recv_buffer+sizeof(struct message_header),&tlv,&value);
	if(rc)
		return -1;
	memcpy(buffer,value,tlv.length);
	return 0;
}
int client_endpoint_read_virtual_bus_matched(struct client_endpoint *cep,int start_block_index,int nr_of_blocks,char*buffer,uint64_t* target_version,int *is_modified)
{
	struct message_builder builder;
	int dummy;
	void *value;
	struct tlv_header tlv;
	int rc;
	if(cep->is_connected==FALSE||cep->is_connected_to_virtual_bus==FALSE)
		return -1;
	message_builder_init(&builder,cep->send_buffer,sizeof(cep->send_buffer));
	tlv.type=MESSAGE_TLV_TYPE_READ_BUS_START;
	tlv.length=0;
	message_builder_add_tlv(&builder,&tlv,&dummy);

	tlv.type=MESSAGE_TLV_TYPE_START_BLOCK_INDEX;
	tlv.length=4;
	message_builder_add_tlv(&builder,&tlv,&start_block_index);

	tlv.type=MESSAGE_TLV_TYPE_NB_OF_BLOCKS;
	tlv.length=4;
	message_builder_add_tlv(&builder,&tlv,&nr_of_blocks);

	tlv.type=MESSAGE_TLV_TYPE_TARGET_VERSION;
	tlv.length=8;
	message_builder_add_tlv(&builder,&tlv,target_version);

	tlv.type=MESSAGE_TLV_TYPE_READ_BUS_END;
	tlv.length=0;
	message_builder_add_tlv(&builder,&tlv,&dummy);

	rc=send_data_with_quantum(cep->fd_sock,builder.buffer,builder.buffer_iptr);
	if(rc!=builder.buffer_iptr)
		return -1;

	rc=recv_tlv_message(cep);
	if(rc)
		return -1;
	/*1. data not modified*/
	tlv.type=MESSAGE_TLV_TYPE_DATA_NOT_MODIFIED;
	rc=message_parse_raw((struct message_header *)(cep->recv_buffer),cep->recv_buffer+sizeof(struct message_header),&tlv,&value);
	if(!rc){
		*is_modified=FALSE;
		return 0;
	}
	/*2.data modified*/
	tlv.type=MESSAGE_TLV_TYPE_DATA;
	rc=message_parse_raw((struct message_header *)(cep->recv_buffer),cep->recv_buffer+sizeof(struct message_header),&tlv,&value);
	if(rc)
		return -1;
	memcpy(buffer,value,tlv.length);

	tlv.type=MESSAGE_TLV_TYPE_TARGET_VERSION;
	rc=message_parse_raw((struct message_header *)(cep->recv_buffer),cep->recv_buffer+sizeof(struct message_header),&tlv,&value);
	if(rc)
		return -1;
	*target_version=*(uint64_t*)value;
	*is_modified=TRUE;
		
	//printf(" version length:%d\n",tlv.length);
	return 0;
	

}
int client_endpoint_write_virtual_bus_generic(struct client_endpoint*cep,int start_block_index,int nr_of_blocks,char*buffer,uint64_t *updated_version_number)
{
	struct message_builder builder;
	int dummy;
	void *value;
	struct tlv_header tlv;
	int rc;
	if(cep->is_connected==FALSE||cep->is_connected_to_virtual_bus==FALSE)
		return -1;
	message_builder_init(&builder,cep->send_buffer,sizeof(cep->send_buffer));

	tlv.type=MESSAGE_TLV_TYPE_WRITE_BUS_START;
	tlv.length=0;
	message_builder_add_tlv(&builder,&tlv,&dummy);

	tlv.type=MESSAGE_TLV_TYPE_START_BLOCK_INDEX;
	tlv.length=4;
	message_builder_add_tlv(&builder,&tlv,&start_block_index);

	tlv.type=MESSAGE_TLV_TYPE_NB_OF_BLOCKS;
	tlv.length=4;
	message_builder_add_tlv(&builder,&tlv,&nr_of_blocks);

	tlv.type=MESSAGE_TLV_TYPE_DATA;
	tlv.length=CACHE_LINE_SIZE*nr_of_blocks;
	message_builder_add_tlv(&builder,&tlv,buffer);
	
	/*no MESSAGE_TLV_TYPE_TARGET_VERSION section*/

	tlv.type=MESSAGE_TLV_TYPE_WRITE_BUS_END;
	tlv.length=0;
	message_builder_add_tlv(&builder,&tlv,&dummy);

	rc=send_data_with_quantum(cep->fd_sock,builder.buffer,builder.buffer_iptr);
	if(rc!=builder.buffer_iptr)
		return -1;
	
	rc=recv_tlv_message(cep);
		if(rc)
			return -1;

	tlv.type=MESSAGE_TLV_TYPE_ACK_BUS_OPS_SUCCEED;
	tlv.length=0;
	rc=message_parse_raw((struct message_header *)(cep->recv_buffer),cep->recv_buffer+sizeof(struct message_header),&tlv,&value);
	if(rc)
		return -1;
	
	tlv.type=MESSAGE_TLV_TYPE_TARGET_VERSION;
	tlv.length=0;
	rc=message_parse_raw((struct message_header *)(cep->recv_buffer),cep->recv_buffer+sizeof(struct message_header),&tlv,&value);
	if(rc)
		return -1;
	*updated_version_number=*(uint64_t*)value;
	return 0;
}
int client_endpoint_write_virtual_bus_matched(struct client_endpoint*cep,
													int start_block_index,
													int nr_of_blocks,
													char*buffer,
													int *is_matched,
													uint64_t *target_version)
{
	struct message_builder builder;
	int dummy;
	void *value;
	struct tlv_header tlv;
	int rc;
	if(cep->is_connected==FALSE||cep->is_connected_to_virtual_bus==FALSE)
		return -1;
	message_builder_init(&builder,cep->send_buffer,sizeof(cep->send_buffer));
	
	tlv.type=MESSAGE_TLV_TYPE_WRITE_BUS_START;
	tlv.length=0;
	message_builder_add_tlv(&builder,&tlv,&dummy);

	tlv.type=MESSAGE_TLV_TYPE_START_BLOCK_INDEX;
	tlv.length=4;
	message_builder_add_tlv(&builder,&tlv,&start_block_index);

	tlv.type=MESSAGE_TLV_TYPE_NB_OF_BLOCKS;
	tlv.length=4;
	message_builder_add_tlv(&builder,&tlv,&nr_of_blocks);

	tlv.type=MESSAGE_TLV_TYPE_DATA;
	tlv.length=CACHE_LINE_SIZE*nr_of_blocks;
	message_builder_add_tlv(&builder,&tlv,buffer);

	/*add MESSAGE_TLV_TYPE_TARGET_VERSION section*/
	tlv.type=MESSAGE_TLV_TYPE_TARGET_VERSION;
	tlv.length=8;
	message_builder_add_tlv(&builder,&tlv,target_version);

	tlv.type=MESSAGE_TLV_TYPE_WRITE_BUS_END;
	tlv.length=0;
	message_builder_add_tlv(&builder,&tlv,&dummy);

	rc=send_data_with_quantum(cep->fd_sock,builder.buffer,builder.buffer_iptr);
	if(rc!=builder.buffer_iptr)
		return -1;
	
	rc=recv_tlv_message(cep);
		if(rc)
			return -1;
	tlv.type=MESSAGE_TLV_TYPE_DATA_NOT_MATCHED;
	tlv.length=0;
	rc=message_parse_raw((struct message_header *)(cep->recv_buffer),cep->recv_buffer+sizeof(struct message_header),&tlv,&value);
	if(!rc){
		*is_matched=FALSE;
		return 0;
	}

	tlv.type=MESSAGE_TLV_TYPE_ACK_BUS_OPS_SUCCEED;
	tlv.length=0;
	rc=message_parse_raw((struct message_header *)(cep->recv_buffer),cep->recv_buffer+sizeof(struct message_header),&tlv,&value);
	if(rc)
		return -1;
	
	tlv.type=MESSAGE_TLV_TYPE_TARGET_VERSION;
	tlv.length=0;
	rc=message_parse_raw((struct message_header *)(cep->recv_buffer),cep->recv_buffer+sizeof(struct message_header),&tlv,&value);
	if(rc)
		return -1;
	*target_version=*(uint64_t*)value;
	*is_matched=TRUE;
	
	return 0;
}
int recv_tlv_message(struct  client_endpoint *cep)
{	
	int rc;
	int tmp;
	cep->recv_buffer_length=0;
	rc=recv_data_with_quantum(cep->fd_sock,cep->recv_buffer,sizeof(struct message_header));
	if(rc!=sizeof(struct message_header))
		return -1;
	cep->recv_buffer_length=rc;
	
	tmp=((struct message_header*)cep->recv_buffer)->total_length-sizeof(struct message_header);
	rc=recv_data_with_quantum(cep->fd_sock,cep->recv_buffer+cep->recv_buffer_length,tmp);
	if(rc!=tmp)
		return -1;
	cep->recv_buffer_length+=rc;
	return 0;
}
#endif
int recv_tlv_message(struct  client_endpoint *cep)
{	
	int rc;
	int tmp;
	cep->recv_buffer_length=0;
	rc=recv_data_with_quantum(cep->fd_sock,cep->recv_buffer,sizeof(struct message_header));
	if(rc!=sizeof(struct message_header))
		return -1;
	cep->recv_buffer_length=rc;
	
	tmp=(int)(((struct message_header*)cep->recv_buffer)->total_length)-(int)sizeof(struct message_header);
	rc=recv_data_with_quantum(cep->fd_sock,cep->recv_buffer+cep->recv_buffer_length,tmp);
	if(rc!=tmp)
		return -1;
	cep->recv_buffer_length+=rc;
	return 0;
}

