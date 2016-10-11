#include <virtbus_logic.h>
#include <linkedhash.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>


int global_epfd;
hashtable_t *socket_hash_tbl;
hashtable_t * g_vlink_hash_tbl;
hashtable_t * g_vdomain_hash_tbl;
#define DEBUG_FLAG 1
#define LOG(...) fprintf(stdout,__VA_ARGS__)

/*static struct  nametree_node* virtbus_rootnode;*/
enum read_return_status{
	read_return_pass,
	read_return_close,
};
struct endpoint * alloc_endpoint()
{
	struct endpoint * ep=malloc(sizeof(struct endpoint));
	if(ep){
		memset(ep,0x0,sizeof(struct endpoint));
	}
	return ep;
}
void schedule_epoll_out(struct endpoint*point)
{
	if(point->is_pollout_scheduled==TRUE)
		return;
	struct epoll_event event;
	event.data.fd=point->fd_client;
	event.events=EPOLLOUT|EPOLLIN;
	int res=epoll_ctl(global_epfd,EPOLL_CTL_MOD,point->fd_client,&event);
	assert(!res);
	point->is_pollout_scheduled=TRUE;
}
void close_epoll_out(struct endpoint*point)
{
	if(point->is_pollout_scheduled==FALSE)
		return ;
	struct epoll_event event;
	event.data.fd=point->fd_client;
	event.events=EPOLLIN;
	int res=epoll_ctl(global_epfd,EPOLL_CTL_MOD,point->fd_client,&event);
	assert(!res);
	point->is_pollout_scheduled=FALSE;
	
}

void reset_state_endpoint(struct endpoint *point)
{
	point->state=endpoint_state_init;
	point->msg_buffer_iptr=0;
	point->msg_header_iptr=0;
	point->msg_buffer_pending=0;
	point->msg_header_pending=sizeof(struct message_header);
	//point->msg_send_buffer_iptr=0;
	//point->msg_send_buffer_pending=0;
}

struct message_callback_entry{
	uint16_t message_type;
	void (*callback_entry)(struct tlv_header*,void *,void *);	
};


#if 0
void message_join_bus_entry(struct tlv_header*tlv,void * value,void * arg)
{
	struct endpoint * point=(struct endpoint *)arg;
	struct virtual_bus *vbus=NULL;
	int rc;
	struct nametree_node *found_node;
	struct message_builder builder;
	struct tlv_header tlv_tmp;
	int dummy;
	
	switch(tlv->type)
	{
		case MESSAGE_TLV_TYPE_JOIN_BUS:
			memcpy(point->bus_name,value,tlv->length);
			point->bus_name[tlv->length]='\0';
			break;
		case MESSAGE_TLV_TYPE_JOIN_BUS_QUANTUM:
			point->bus_quantum=*(int32_t*)value;
			break;
		case MESSAGE_TLV_TYPE_JOIN_BUS_END:/*check whether the bus exists,if not ,create one*/
			rc=lookup_key_in_name_tree(virtbus_rootnode,point->bus_name,&found_node);
			if(rc){/*virtual created by previous node*/
				vbus=(struct virtual_bus*)found_node->node_value;
				printf("[x]virtual bus found:%s\n",point->bus_name);
			}else{
				vbus=alloc_virtual_bus(point->bus_name,point->bus_quantum);
				printf("[x]virtual bus newly created:%s\n",point->bus_name);
				if(vbus)
					add_key_to_name_tree(virtbus_rootnode,point->bus_name,vbus,&found_node);
			}
			if(!point->bus_ptr&&vbus)/*this is newly added bus node,increase the reference count*/
				vbus->ref_counnt++;
			
			point->bus_ptr=vbus;
			/*ack*/
			message_builder_init(&builder,point->msg_send_buffer,sizeof(point->msg_send_buffer));
			tlv_tmp.type=point->bus_ptr?MESSAGE_TLV_TYPE_ACK_BUS_OPS_SUCCEED:MESSAGE_TLV_TYPE_ACK_BUS_OPS_FAIL;
			tlv_tmp.length=0;
			//printf("result %d\n",tlv_tmp.type==MESSAGE_TLV_TYPE_ACK_BUS_OPS_SUCCEED);
			message_builder_add_tlv(&builder,&tlv_tmp,&dummy);
			if(point->bus_ptr){
				tlv_tmp.type=MESSAGE_TLV_TYPE_NB_OF_BLOCKS;
				tlv_tmp.length=4;
				message_builder_add_tlv(&builder,&tlv_tmp,&point->bus_ptr->dsm->nr_of_items);
			}
			
			point->msg_send_buffer_pending=builder.message_header_ptr->total_length;
			point->msg_send_buffer_iptr=0;
			schedule_epoll_out(point);
			break;
		
	}
}
#endif
#if 0
void message_common_options_entry(struct tlv_header*tlv,void * value,void * arg)
{
	struct endpoint * point=(struct endpoint *)arg;
	switch(tlv->type)
	{

		case MESSAGE_TLV_TYPE_START_BLOCK_INDEX:
			point->start_block_index=*(uint32_t*)value;
			break;
		case MESSAGE_TLV_TYPE_NB_OF_BLOCKS:
			point->nr_of_blocks=*(uint32_t*)value;
			break;
		case MESSAGE_TLV_TYPE_TARGET_VERSION:
			point->target_version=*(uint64_t*)value;
			point->is_gona_be_matched=TRUE;
			break;
		case MESSAGE_TLV_TYPE_LOCK_BUS:
			/*because it's scheduled only on single CPU/thread,no lock needed */
			point->is_gona_lock_bus=TRUE;
			break;
		case MESSAGE_TLV_TYPE_DATA:
			point->data_length=tlv->length;
			point->data_ptr=value;
			puts("data init");
			break;
		default:

			break;
	}

}
void message_read_bus_entry(struct tlv_header*tlv,void * value,void * arg)
{
	struct endpoint * point=(struct endpoint *)arg;
	struct message_builder builder;
	struct tlv_header tlv_tmp;
	char msg_temp_buffer[ENDPOINT_BUFFER_LENGTH];
	int rc;
	int dummy=0;
	switch(tlv->type)
	{
		case MESSAGE_TLV_TYPE_READ_BUS_START:
			/*reset status fields*/
			point->start_block_index=0;
			point->nr_of_blocks=0;
			point->target_version=0;
			point->is_gona_be_matched=FALSE;
			point->is_gona_lock_bus=FALSE;
			break;
		case MESSAGE_TLV_TYPE_READ_BUS_END:
			message_builder_init(&builder,point->msg_send_buffer,sizeof(point->msg_send_buffer));
			if(!point->bus_ptr){/*bus error:MESSAGE_TLV_TYPE_ACK_BUS_OPS_FAIL*/
				tlv_tmp.type=MESSAGE_TLV_TYPE_ACK_BUS_OPS_FAIL;
				tlv_tmp.length=0;
				message_builder_add_tlv(&builder,&tlv_tmp,&dummy);
			} else if(point->is_gona_be_matched){/*need to match the version number*/
				rc=issue_bus_read_matched(point->bus_ptr,point->start_block_index,point->nr_of_blocks,msg_temp_buffer,&point->target_version);
				if(rc==-VIRTUAL_BUS_MATCHED_VERSION){
					tlv_tmp.type=MESSAGE_TLV_TYPE_DATA_NOT_MODIFIED;
					tlv_tmp.length=0;
					message_builder_add_tlv(&builder,&tlv_tmp,&dummy);
				}else if(rc==VIRTUAL_BUS_OK){
					tlv_tmp.type=MESSAGE_TLV_TYPE_DATA;
					tlv_tmp.length=point->nr_of_blocks*CACHE_LINE_SIZE;
					message_builder_add_tlv(&builder,&tlv_tmp,msg_temp_buffer);
					tlv_tmp.type=MESSAGE_TLV_TYPE_TARGET_VERSION;
					tlv_tmp.length=sizeof(uint64_t);
					message_builder_add_tlv(&builder,&tlv_tmp,&point->target_version);
				}else{/*this is never expected*/
					tlv_tmp.type=MESSAGE_TLV_TYPE_ACK_BUS_OPS_FAIL;
					tlv_tmp.length=0;
					message_builder_add_tlv(&builder,&tlv_tmp,&dummy);
				}
			}else{/*read cache line blocks raw*/
				rc=issue_bus_read_raw(point->bus_ptr,point->start_block_index,point->nr_of_blocks,msg_temp_buffer);
				if(rc==VIRTUAL_BUS_OK){
					tlv_tmp.type=MESSAGE_TLV_TYPE_DATA;
					tlv_tmp.length=point->nr_of_blocks*CACHE_LINE_SIZE;
					message_builder_add_tlv(&builder,&tlv_tmp,msg_temp_buffer);
				}else{
					tlv_tmp.type=MESSAGE_TLV_TYPE_ACK_BUS_OPS_FAIL;
					tlv_tmp.length=0;
					message_builder_add_tlv(&builder,&tlv_tmp,&dummy);
				}
			}
			
			/*schedule to send the data back to client*/
			point->msg_send_buffer_pending=builder.message_header_ptr->total_length;
			point->msg_send_buffer_iptr=0;
			schedule_epoll_out(point);
			break;
		default:
			break;
	}
}
void message_write_bus_entry(struct tlv_header*tlv,void * value,void * arg)
{
	struct endpoint * point=(struct endpoint *)arg;
	struct message_builder builder;
	struct tlv_header tlv_tmp;
	int dummy;
	int rc;

	switch(tlv->type)
	{
		case MESSAGE_TLV_TYPE_WRITE_BUS_START:
			point->start_block_index=0;
			point->nr_of_blocks=0;
			point->target_version=0;
			point->is_gona_be_matched=FALSE;
			point->is_gona_lock_bus=FALSE;
			point->data_length=0;
			point->data_ptr=NULL;
			puts("write_start");
			break;
		case MESSAGE_TLV_TYPE_WRITE_BUS_END:
			message_builder_init(&builder,point->msg_send_buffer,sizeof(point->msg_send_buffer));
			if(!point->bus_ptr||!point->data_ptr){
				puts("write init eerror");
				tlv_tmp.type=MESSAGE_TLV_TYPE_ACK_BUS_OPS_FAIL;
				tlv_tmp.length=0;
				message_builder_add_tlv(&builder,&tlv_tmp,&dummy);
			}else if(point->is_gona_be_matched){
				puts("write match");
				rc=issue_bus_write_matched(point->bus_ptr,point->start_block_index,point->nr_of_blocks,point->data_ptr, &point->target_version);
				if(rc==-VIRTUAL_BUS_VERSION_NOT_MATCH){/*version number checking fails*/
					tlv_tmp.type=MESSAGE_TLV_TYPE_DATA_NOT_MATCHED;
					tlv_tmp.length=0;
					message_builder_add_tlv(&builder,&tlv_tmp,&dummy);
				}else if(rc==VIRTUAL_BUS_OK){/*write bus OK,return the updated version*/
					tlv_tmp.type=MESSAGE_TLV_TYPE_ACK_BUS_OPS_SUCCEED;
					tlv_tmp.length=0;
					message_builder_add_tlv(&builder,&tlv_tmp,&dummy);

					tlv_tmp.type=MESSAGE_TLV_TYPE_TARGET_VERSION;
					tlv_tmp.length=8;
					message_builder_add_tlv(&builder,&tlv_tmp,&point->target_version);
				}else{/*error happens*/
					tlv_tmp.type=MESSAGE_TLV_TYPE_ACK_BUS_OPS_FAIL;
					tlv_tmp.length=0;
					message_builder_add_tlv(&builder,&tlv_tmp,&dummy);
				}
			}else{ 
				puts("write generic");
				rc=issue_bus_write_generic(point->bus_ptr,point->start_block_index,point->nr_of_blocks,point->data_ptr,&point->target_version);
				if(rc==VIRTUAL_BUS_OK){
					tlv_tmp.type=MESSAGE_TLV_TYPE_ACK_BUS_OPS_SUCCEED;
					tlv_tmp.length=0;
					message_builder_add_tlv(&builder,&tlv_tmp,&dummy);
					tlv_tmp.type=MESSAGE_TLV_TYPE_TARGET_VERSION;
					tlv_tmp.length=8;
					message_builder_add_tlv(&builder,&tlv_tmp,&point->target_version);
				}else{
					tlv_tmp.type=MESSAGE_TLV_TYPE_ACK_BUS_OPS_FAIL;
					tlv_tmp.length=0;
					message_builder_add_tlv(&builder,&tlv_tmp,&dummy);
				}
			}
			
			point->msg_send_buffer_pending=builder.message_header_ptr->total_length;
			point->msg_send_buffer_iptr=0;
			schedule_epoll_out(point);
			break;
		default:
			break;
	}
}
#endif
void message_common_entry(struct tlv_header*tlv,void * value,void * arg)
{
	struct endpoint * point=(struct endpoint *)arg;
	switch(tlv->type)
	{
		case MESSAGE_TLV_VLINK_COMMON_VM_DOMAIN:
			memcpy(point->vm_domain_name,value,tlv->length);
			point->vm_domain_name[tlv->length]='\0';
			
			/*TO_STRING(point->vm_domain_name,value,sizeof(point->vm_domain_name));*/
			/*strncpy(point->vm_domain_name,(char*)value,sizeof(point->vm_domain_name));*/
			break;
		case MESSAGE_TLV_VLINK_COMMON_VM_MAX_CHANNELS:
			point->vm_max_channel=TO_INT32(value);
			break;
		case MESSAGE_TLV_VLINK_COMMON_VLINK_NAME:
			memcpy(point->virt_link,value,tlv->length);
			point->virt_link[tlv->length]='\0';
			/*TO_STRING(point->virt_link,value,sizeof(point->vm_domain_name));*/
			break;
		case MESSAGE_TLV_VLINK_COMMON_VLINK_NR_CHANNELS:
			point->nr_channels=TO_INT32(value);
			break;
		case MESSAGE_TLV_VLINK_COMMON_VLINK_ROLE:
			point->role=TO_INT32(value);
			break;
		case MESSAGE_TLV_VLINK_COMMON_VLINK_MAC:
			ASSERT(tlv->length==6);
			memcpy(point->mac_address,value,6);
			break;
			
		default:
			break;
	}
	
}


void initialize_virtual_link_channels(struct endpoint * point)
{
	void * vm_shm_base;
	int ctrl_channel_index;
	int ctrl_channel_offset;
	struct ctrl_channel *ctrl;
	void * data_channel_base;
	void * rx_sub_channel_base;
	void * free_sub_channel_base;
	void * alloc_sub_channel_base;
	void * tx_sub_channel_base;
	
	int idx;
	if(!point->vlink||!point->vlink->domain)
		return;

	vm_shm_base=PTR(void*,point->vlink->domain->shm_base);
	if(point->vlink->nr_channels_allocated<2)
		return;/*at least TWO channels ,one for control channel,others for data channels*/
	ctrl_channel_index=point->vlink->channels[0];
	ctrl_channel_offset=channel_base_offset(ctrl_channel_index);
	
	ctrl=PTR_OFFSET_BY(struct ctrl_channel*,vm_shm_base,ctrl_channel_offset);
	ctrl->index_in_vm_domain=ctrl_channel_index;
	ctrl->offset_to_vm_shm_base=ctrl_channel_offset;
	ctrl->nr_data_channels=(point->vlink->nr_channels_allocated-1);
	memcpy(ctrl->mac_address,point->vlink->mac_address,6);
	#if DEBUG_FLAG
	LOG("[x]%s(%s)\n",point->vlink->domain->domain_name,point->vlink->link_name);
	LOG("\t[x]ctrl channel:%d(offset:%d),(nr-of-data-channels:%d)\n",ctrl->index_in_vm_domain,ctrl->offset_to_vm_shm_base,ctrl->nr_data_channels);
	#endif
	
	for(idx=0;idx<ctrl->nr_data_channels;idx++){
		/*1.initialize data channels metadata*/
		ctrl->channel_records[idx].index_in_m_domain=point->vlink->channels[idx+1];
		ctrl->channel_records[idx].offset_to_vm_shm_base=channel_base_offset(point->vlink->channels[idx+1]);
		
		ctrl->channel_records[idx].data_channel_enabled=0;/*DPDK lane rx-queue setup will enable this*/
		ctrl->channel_records[idx].rx_address_to_translate=0;
		ctrl->channel_records[idx].tx_address_to_translate=0;
		ctrl->channel_records[idx].interrupt_enabled=0;/*qemu lane will enable this*/
		/*2.initialize every data channel(subchannels)*/
		
		data_channel_base=PTR_OFFSET_BY(void*,vm_shm_base,ctrl->channel_records[idx].offset_to_vm_shm_base);
		rx_sub_channel_base=PTR_OFFSET_BY(void*,data_channel_base,rx_sub_channel_offset());
		free_sub_channel_base=PTR_OFFSET_BY(void*,data_channel_base,free_sub_channel_offset());
		alloc_sub_channel_base=PTR_OFFSET_BY(void*,data_channel_base,alloc_sub_channel_offset());
		tx_sub_channel_base=PTR_OFFSET_BY(void*,data_channel_base,tx_sub_channel_offset());
		
		ASSERT(!initialize_queue(rx_sub_channel_base,SUB_CHANNEL_SIZE,DEFAULT_CHANNEL_QUEUE_LENGTH));
		ASSERT(!initialize_queue(free_sub_channel_base,SUB_CHANNEL_SIZE,DEFAULT_CHANNEL_QUEUE_LENGTH));
		ASSERT(!initialize_queue(alloc_sub_channel_base,SUB_CHANNEL_SIZE,DEFAULT_CHANNEL_QUEUE_LENGTH));
		ASSERT(!initialize_queue(tx_sub_channel_base,SUB_CHANNEL_SIZE,DEFAULT_CHANNEL_QUEUE_LENGTH));
		#if DEBUG_FLAG
		LOG("\t[x]data-channel:%d (offset:%d) (rx:%d) (free:%d) (alloc:%d) (tx:%d)\n",ctrl->channel_records[idx].index_in_m_domain,
		(int)(ctrl->channel_records[idx].offset_to_vm_shm_base),
		(int)(ctrl->channel_records[idx].offset_to_vm_shm_base+rx_sub_channel_offset()),
		(int)(ctrl->channel_records[idx].offset_to_vm_shm_base+free_sub_channel_offset()),
		(int)(ctrl->channel_records[idx].offset_to_vm_shm_base+alloc_sub_channel_offset()),
		(int)(ctrl->channel_records[idx].offset_to_vm_shm_base+tx_sub_channel_offset())
		);
		#endif
	}
}
void change_lane_connection_status(struct endpoint *point,int is_dpdk,int is_connected)
{
	void * vm_shm_base;
	int ctrl_channel_index;
	int ctrl_channel_offset;
	struct ctrl_channel *ctrl;
	if(!point->vlink||!point->vlink->domain)
		return;
	vm_shm_base=PTR(void*,point->vlink->domain->shm_base);
	if(point->vlink->nr_channels_allocated<2)
		return;
	ctrl_channel_index=point->vlink->channels[0];
	ctrl_channel_offset=channel_base_offset(ctrl_channel_index);
	ctrl=PTR_OFFSET_BY(struct ctrl_channel*,vm_shm_base,ctrl_channel_offset);
	*(is_dpdk?&ctrl->dpdk_connected:&ctrl->qemu_connected)=is_connected?1:0;
}
void message_vlink_request_entry(struct tlv_header * tlv,void * value,void*arg)
{
	struct endpoint * point=(struct endpoint *)arg;
	struct virtual_link * tmp_link;
	
	switch(tlv->type)
	{
		case MESSAGE_TLV_VLINK_REQUEST_START:
			memset(point->vm_domain_name,0x0,sizeof(point->vm_domain_name));
			memset(point->virt_link,0x0,sizeof(point->virt_link));
			point->vm_max_channel=0;
			point->nr_channels=0;
			point->role=VLINK_ROLE_NONE;
			
			break;
		case MESSAGE_TLV_VLINK_REQUEST_END:
			
			if(point->vlink)
				goto normal_tag;
			
			if((!point->virt_link[0])||(point->role==VLINK_ROLE_NONE))
				goto init_exception;
			
			tmp_link=hashtable_get_value(g_vlink_hash_tbl,(uint8_t*)point->virt_link,strlen(point->virt_link));
			
			if(tmp_link==(void*)-1)
				goto not_found_exception;
			if(!point->vlink)
				point->vlink=tmp_link;
			
			normal_tag:
				
				switch(point->role)
				{
					case VLINK_ROLE_DPDK:
						{/*clear previous vlink pointer in case the virtual_link is released ,so homeless link's vlink ptr will be poisoned,thus crashing down the whole system*/
							struct endpoint *temp_point=hashtable_get_value(socket_hash_tbl,(uint8_t *)&point->vlink->fd_dpdk,sizeof(int));
							if(temp_point!=(void*)-1){
								temp_point->vlink=NULL;
							}
						}
						point->vlink->fd_dpdk=point->fd_client;
						initialize_virtual_link_channels(point);/*whenever dpdk lane is connected ,initialize vlink channels*/
						change_lane_connection_status(point,1,1);
						break;
					case VLINK_ROLE_QEMU:
						{
							struct endpoint *temp_point=hashtable_get_value(socket_hash_tbl,(uint8_t *)&point->vlink->fd_qemu,sizeof(int));
							if(temp_point!=(void*)-1){
								temp_point->vlink=NULL;
							}
						}
						point->vlink->fd_qemu=point->fd_client;
						change_lane_connection_status(point,0,1);
						break;
					default:
						/*ASSERT(({!"unknow vlink role found!";}));*/
						goto init_exception;
						break;
				}
				_generate_vlink_info(point);
				schedule_epoll_out(point);
			break;
			not_found_exception:
				_generate_vlink_error_info(point,VLINK_STATUS_CODE_NOT_FOUND);
				schedule_epoll_out(point);
				break;
			init_exception:
				_generate_vlink_error_info(point,VLINK_STATUS_CODE_INVALID_PARAMETER);
				schedule_epoll_out(point);
				break;
		default:

			break;
	}
}

	
	
void _generate_vlink_info(struct endpoint *point)
{
	struct message_builder mb;
	struct tlv_header tlv;
	uint32_t status_code=VLINK_STATUS_CODE_OK;
	message_builder_init(&mb,point->msg_send_buffer,sizeof(point->msg_send_buffer));
	if(!(point->vlink)||!(point->vlink->domain)){
		status_code=VLINK_STATUS_CODE_COMMON_ERROR;
		TLV_ADD_UINT32(MESSAGE_TLV_VLINK_COMMON_STATUS_CODE,&status_code);
		goto normal;
	}
	
	TLV_ADD_UINT32(MESSAGE_TLV_VLINK_COMMON_STATUS_CODE,&status_code);
	TLV_ADD_STRING(MESSAGE_TLV_VLINK_COMMON_VM_DOMAIN,(char*)point->vlink->domain->domain_name);
	TLV_ADD_UINT32(MESSAGE_TLV_VLINK_COMMON_VM_MAX_CHANNELS,&point->vlink->domain->max_channels);
	TLV_ADD_STRING(MESSAGE_TLV_VLINK_COMMON_VLINK_NAME,(char*)point->vlink->link_name);
	TLV_ADD_UINT32(MESSAGE_TLV_VLINK_COMMON_VLINK_NR_CHANNELS,&point->vlink->nr_channels_allocated);
	TLV_ADD_MEM(MESSAGE_TLV_VLINK_COMMON_VLINK_MAC,point->vlink->mac_address,6);
	{
		int idx=0;
		for(;idx<point->vlink->nr_channels_allocated;idx++){
			point->channels[idx]=point->vlink->channels[idx];
		}
	}
	TLV_ADD_MEM(MESSAGE_TLV_VLINK_COMMON_VLINK_CHANNELS,point->channels,sizeof(uint32_t)*point->vlink->nr_channels_allocated);
	
	normal:
	point->msg_send_buffer_pending=mb.message_header_ptr->total_length;
	point->msg_send_buffer_iptr=0;
}
void _generate_vlink_error_info(struct endpoint* point,uint32_t status_code)
{
	struct message_builder mb;
	struct tlv_header tlv;
	message_builder_init(&mb,point->msg_send_buffer,sizeof(point->msg_send_buffer));
	TLV_ADD_UINT32(MESSAGE_TLV_VLINK_COMMON_STATUS_CODE,&status_code);

	point->msg_send_buffer_pending=mb.message_header_ptr->total_length;
	point->msg_send_buffer_iptr=0;
}
void message_vlink_init_entry(struct tlv_header * tlv,void * value,void*arg)
{
	struct endpoint * point=(struct endpoint *)arg;
	struct virtual_link * tmp_link;
	struct vm_domain* tmp_domain;
	switch(tlv->type)
	{
		case MESSAGE_TLV_VLINK_INIT_START:
			memset(point->vm_domain_name,0x0,sizeof(point->vm_domain_name));
			memset(point->virt_link,0x0,sizeof(point->virt_link));
			point->vm_max_channel=0;
			point->nr_channels=0;
			point->role=VLINK_ROLE_NONE;
			
			break;
		case MESSAGE_TLV_VLINK_INIT_END:
			/*1.check parameter*/
			if(point->vlink)/*already allocated,so directly to respond*/
				goto normal_tag;
			
			if((!point->vm_domain_name[0])||(!point->virt_link[0])||(!point->nr_channels)||(!point->vm_max_channel))
				goto init_except;
			
			/*2 search whether vlink exists,if not ,create one,otherwise inform client with _EXIST*/
			tmp_link=hashtable_get_value(g_vlink_hash_tbl,(uint8_t*)point->virt_link,strlen(point->virt_link));
			if(tmp_link==(void*)-1){
				
				ASSERT((tmp_link=alloc_virtual_link(point->virt_link)));
				memcpy(tmp_link->mac_address,point->mac_address,6);/*it's really ugly to put it here*/
				hashtable_set_key_value(g_vlink_hash_tbl,(uint8_t*)point->virt_link,strlen(point->virt_link),tmp_link);
			}else{ 
				point->vlink=tmp_link;
				goto normal_tag;
			}
			
			tmp_domain=hashtable_get_value(g_vdomain_hash_tbl,(uint8_t*)point->vm_domain_name,strlen(point->vm_domain_name));
			if(tmp_domain==(void*)-1){
				ASSERT((tmp_domain=alloc_vm_domain(point->vm_max_channel,point->vm_domain_name)));
				hashtable_set_key_value(g_vdomain_hash_tbl,(uint8_t*)point->vm_domain_name,strlen(point->vm_domain_name),tmp_domain);
			}
			
			/*3 make sure virtual link and vm domain are relevant*/
			if(!tmp_link->domain)
				ASSERT(({attach_virtual_link_to_vm_domain(tmp_domain,tmp_link);tmp_link->domain==tmp_domain;}));

			/*4 allocate channels from vm_domains*/
			if(!tmp_link->nr_channels_allocated)
				allocate_channel_from_vm_domain(tmp_link,point->nr_channels);/*tmp_link->nr_channels_allocated is the real nr of channels allocated*/

			/*5.assign role for this connection*/
			/*mybe its an update behavior*/
			
			if(!point->vlink)
				point->vlink=tmp_link;

			normal_tag:
			/*by assigning with last requested role,we can easily mitigrate role.*/
			switch(point->role)
			{
				case VLINK_ROLE_DPDK:
					{/*clear previous vlink pointer in case the virtual_link is released ,so homeless link's vlink ptr will be poisoned,thus crashing down the whole system*/
						struct endpoint *temp_point=hashtable_get_value(socket_hash_tbl,(uint8_t *)&point->vlink->fd_dpdk,sizeof(int));
						if(temp_point!=(void*)-1){
							temp_point->vlink=NULL;
						}
					}
					point->vlink->fd_dpdk=point->fd_client;
					initialize_virtual_link_channels(point);/*whenever dpdk lane is connected ,initialize vlink channels*/
					change_lane_connection_status(point,1,1);
					break;
				case VLINK_ROLE_QEMU:
					{
						struct endpoint *temp_point=hashtable_get_value(socket_hash_tbl,(uint8_t *)&point->vlink->fd_qemu,sizeof(int));
						if(temp_point!=(void*)-1){
							temp_point->vlink=NULL;
						}
					}
					point->vlink->fd_qemu=point->fd_client;
					change_lane_connection_status(point,0,1);
					break;
				default:
					/*ASSERT(({!"unknow vlink role found!";}));*/
					goto init_except;
					break;
			}
			
			/*6.then,acknoledge client*/
			_generate_vlink_info(point);
			schedule_epoll_out(point);
			
			break;
			init_except:
				_generate_vlink_error_info(point,VLINK_STATUS_CODE_INVALID_PARAMETER);
				schedule_epoll_out(point);
				break;
		default:

			break;
	}
}


struct message_callback_entry cb_entries[]={
		/*{MESSAGE_TLV_TYPE_JOIN_BUS,message_join_bus_entry},
		{MESSAGE_TLV_TYPE_JOIN_BUS_QUANTUM,message_join_bus_entry},
		{MESSAGE_TLV_TYPE_JOIN_BUS_END,message_join_bus_entry},
		
		{MESSAGE_TLV_TYPE_START_BLOCK_INDEX,message_common_options_entry},
		{MESSAGE_TLV_TYPE_NB_OF_BLOCKS,message_common_options_entry},
		{MESSAGE_TLV_TYPE_TARGET_VERSION,message_common_options_entry},
		{MESSAGE_TLV_TYPE_LOCK_BUS,message_common_options_entry},
		{MESSAGE_TLV_TYPE_DATA,message_common_options_entry},
		
		{MESSAGE_TLV_TYPE_READ_BUS_START,message_read_bus_entry},
		{MESSAGE_TLV_TYPE_READ_BUS_END,message_read_bus_entry},

		{MESSAGE_TLV_TYPE_WRITE_BUS_START,message_write_bus_entry},
		{MESSAGE_TLV_TYPE_WRITE_BUS_END,message_write_bus_entry},
		*/
		{MESSAGE_TLV_VLINK_COMMON_VM_DOMAIN,message_common_entry},
		{MESSAGE_TLV_VLINK_COMMON_VM_MAX_CHANNELS,message_common_entry},
		{MESSAGE_TLV_VLINK_COMMON_VLINK_NAME,message_common_entry},
		{MESSAGE_TLV_VLINK_COMMON_VLINK_NR_CHANNELS,message_common_entry},
		{MESSAGE_TLV_VLINK_COMMON_VLINK_CHANNELS,message_common_entry},
		{MESSAGE_TLV_VLINK_COMMON_VLINK_ROLE,message_common_entry},
		{MESSAGE_TLV_VLINK_COMMON_VLINK_MAC,message_common_entry},
		
		{MESSAGE_TLV_VLINK_REQUEST_START,message_vlink_request_entry},
		{MESSAGE_TLV_VLINK_REQUEST_END,message_vlink_request_entry},
		
		{MESSAGE_TLV_VLINK_INIT_START,message_vlink_init_entry},
		{MESSAGE_TLV_VLINK_INIT_END,message_vlink_init_entry},
		
		{MESSAGE_TLV_TYPE_END,NULL}
};
void endpoint_message_callback(struct tlv_header*tlv,void * value,void * arg)
{
	
	
	int idx=0;
	/*struct endpoint * point=(struct endpoint *)arg;*/
	for(idx=0;cb_entries[idx].message_type!=MESSAGE_TLV_TYPE_END;idx++){
		if(cb_entries[idx].message_type==tlv->type){
			if(cb_entries[idx].callback_entry)
				cb_entries[idx].callback_entry(tlv,value,arg);
			break;
		}
	}
}

void endpoint_alloc_callback(struct endpoint * point)
{
}
void endpoint_dealloc_callback(struct endpoint* point)
{
	struct vm_domain*tmp_domain;
	int fd_peer;
	
	if(!point->vlink)
		return ;
	
	tmp_domain=point->vlink->domain;
	/*1.test whether this endpoint is homeless in case of mitigration*/
	if((point->vlink->fd_dpdk!=point->fd_client)&&(point->vlink->fd_qemu!=point->fd_client))
		return;
	fd_peer=(point->vlink->fd_dpdk==point->fd_client)?point->vlink->fd_qemu:point->vlink->fd_dpdk;
	/*2.notify other side,this half is going down*/
	/*2.1 change connection in ctrl channel*/
	change_lane_connection_status(point,point->fd_client==point->vlink->fd_dpdk,0);
	/*3.detach fd from vlink*/
	point->vlink->fd_dpdk=(point->vlink->fd_dpdk==point->fd_client)?0:point->vlink->fd_dpdk;
	point->vlink->fd_qemu=(point->vlink->fd_qemu==point->fd_client)?0:point->vlink->fd_qemu;

	
	/*4.test whether to release vlink ,even vm_domain*/
	if(point->vlink->fd_dpdk || point->vlink->fd_qemu)
		return;
	#if 0
	dealloc_channel_to_vm_domain(point->vlink);
	detach_virtual_link_from_vm_domain(point->vlink);
	#endif
	hashtable_delete_key(g_vlink_hash_tbl,(uint8_t*)point->vlink->link_name,strlen((char*)point->vlink->link_name));
	free_virtual_link(point->vlink);
	if(!tmp_domain->nr_links){
		hashtable_delete_key(g_vdomain_hash_tbl,tmp_domain->domain_name,strlen((char*)tmp_domain->domain_name));
		free_vm_domain(tmp_domain);
	}
	
}
enum read_return_status virtbus_logic_read_data(struct endpoint* point)
{
	int rc;
	switch(point->state)
	{
		case endpoint_state_init:
			rc=read(point->fd_client,point->msg_header_iptr+(char*)&point->msg_header,point->msg_header_pending);
			//printf("recv1:%d\n",rc);
			if(!rc)
				goto error_ret;
			point->msg_header_iptr+=rc;
			point->msg_header_pending-=rc;
			if(!point->msg_header_pending){/*header reception  completed*/
				point->state=endpoint_state_tlv;
				point->msg_buffer_iptr=0;
				point->msg_buffer_pending=point->msg_header.total_length-sizeof(struct message_header);
				//printf("buffer pending:%d\n",point->msg_buffer_pending);
			}
			break;
		case endpoint_state_tlv:
			rc=read(point->fd_client,point->msg_buffer+point->msg_buffer_iptr,point->msg_buffer_pending);
			//printf("recv2:%d\n",rc);
			if(!rc)
				goto error_ret;
			point->msg_buffer_iptr+=rc;
			point->msg_buffer_pending-=rc;
			if(!point->msg_buffer_pending){
				/*here we enter TLV loop logic*/
				message_iterate_raw(&point->msg_header,point->msg_buffer,point,endpoint_message_callback);
				/*prepare for another round TLV parse*/
				reset_state_endpoint(point);
				/*test sending data*/
				#if 0
				strcpy(point->msg_send_buffer,"hello world");
				point->msg_send_buffer_pending=11;
				schedule_epoll_out(point);
				#endif
			}
			break;
		default:
			goto error_ret;
			break;
	}
	return read_return_pass;
	error_ret:
		
		return read_return_close;
}
#include <agent.h>
#include <inttypes.h>
int start_virtbus_logic()
{
	#if 0
	struct vm_domain* domain=alloc_vm_domain(12,"testvm1-channels");
	struct vm_domain* domain1=alloc_vm_domain(12,"testvm1-channels");

	struct virtual_link *link1=alloc_virtual_link("tap-12ef983483");
	struct virtual_link *link2=alloc_virtual_link("tap-12ef983484");
	struct virtual_link *link3=alloc_virtual_link("tap-12ef983485");

	attach_virtual_link_to_vm_domain(domain1,link1);
	allocate_channel_from_vm_domain(link1,11);
	
	printf("[x]shm base:%"PRIx64"\n",domain->shm_base);
	//printf("[x]offset:%d\n",channel_offset(link1,1));
	initialize_virtual_link_channel(link1,0);

	

	attach_virtual_link_to_vm_domain(domain,link2);
	allocate_channel_from_vm_domain(link2,11);
	
	printf("[x] 1 allocated channels:%d\n",link1->nr_channels_allocated);
	printf("[x] 2 allocated channels:%d\n",link2->nr_channels_allocated);

	dealloc_channel_to_vm_domain(link1);
	//dealloc_channel_to_vm_domain(link1);
	attach_virtual_link_to_vm_domain(domain,link3);
	allocate_channel_from_vm_domain(link3,5);
	printf("[x] 3 allocated channels:%d\n",link3->nr_channels_allocated);

	free_virtual_link(link1);
	
	
	struct virtual_link link1,link2;

	struct vm_domain* domain=alloc_vm_domain(40,"testvm1-channels");
	
	add_virtual_link_to_vm_domain(domain,&link1);
	add_virtual_link_to_vm_domain(domain,&link2);
	//del_virtual_link_from_vm_domain(&link1);
	del_virtual_link_from_vm_domain(&link2);
	dump_virtual_link_in_vm_domain(domain);
	printf("[x]link nrs:%d\n",domain->nr_links);
	
	
	uint64_t base=map_shared_memory("cute",252);
	strcpy((void*)base,"hello world");
	printf("[base]:%"PRIx64"\n",base);


	
	int key=0;
	
	hashtable_t *htbl=hashtable_create(1024);
	hashtable_set_key_value(htbl,&key,4,(void*)1223);
	
	key=1;
	hashtable_set_key_value(htbl,&key,4,(void*)996);


	key=123;
	hashtable_delete_key(htbl,&key,4);


	key=0;

	printf("[x]%d\n",hashtable_get_value(htbl,&key,4));
	
	#endif 

	
	#if 0
	/*0.allocate gloval resource*/
	virtbus_rootnode=alloc_nametree_node();
	assert(virtbus_rootnode);
	initalize_nametree_node(virtbus_rootnode,"meeeow",NULL);
	
	/*take care of following code clip, adding these avoids bugs which is brought by deleting node*/
	struct nametree_node * sub_node=alloc_nametree_node();
	initalize_nametree_node(sub_node,"cute",NULL);
	add_node_to_parent(virtbus_rootnode,sub_node);
	#endif
	
	socket_hash_tbl=hashtable_create(4096);
	g_vlink_hash_tbl=hashtable_create(512);
	g_vdomain_hash_tbl=hashtable_create(256);
	ASSERT(g_vlink_hash_tbl);
	ASSERT(g_vdomain_hash_tbl);
	ASSERT(socket_hash_tbl);
	/*1.setup socket framework*/
	int fd_accept;
	/*struct sockaddr_in server_addr;*/
	struct sockaddr_un addr;
	fd_accept=socket(PF_UNIX,SOCK_STREAM,0);
	if(fd_accept<0)
		return FALSE;
	memset(&addr, 0, sizeof(addr));
	unlink(SERVER_UNIX_SOCKET_PATH);/*unlink previous socket file first*/
	addr.sun_family=AF_UNIX;
	strcpy(addr.sun_path,SERVER_UNIX_SOCKET_PATH);
	
	/*
	server_addr.sin_family=AF_INET;
	server_addr.sin_addr.s_addr=INADDR_ANY;
	server_addr.sin_port=htons(VIRTIO_BUS_PORT);
	*/
	/*if(bind(fd_accept,(struct sockaddr*)&server_addr,sizeof(struct sockaddr))<0)*/
	if(bind(fd_accept,(struct sockaddr*)&addr,sizeof(struct sockaddr_un))<0)
		return FALSE;
	if(listen(fd_accept,64)<0)
		return FALSE;
	/*2.setup epoll framework*/
	int epfd=epoll_create1(EPOLL_CLOEXEC);/*always be the epoll instance*/
	struct epoll_event event;
	struct epoll_event events[MAX_EVENT_NR];
	int idx;
	assert(epfd>=0);
	global_epfd=epfd;/*we make this global*/
	event.data.fd=fd_accept;
	event.events=EPOLLIN;
	epoll_ctl(epfd,EPOLL_CTL_ADD,fd_accept,&event);
	/*3.enter event loop*/
	int  nfds;
	int fd_client;
	struct sockaddr_in client_addr;
	int client_addr_len;
	int res;
	struct endpoint *tmp_endpoint;
	enum read_return_status ret_status;
	
	while(1){
		nfds=epoll_wait(epfd,events,MAX_EVENT_NR,100);
		if(nfds<0)
			goto fails;
		for(idx=0;idx<nfds;idx++){
			if(events[idx].data.fd==fd_accept){//new connection arrives
				memset(&client_addr,0x0,sizeof(struct sockaddr_in));
				client_addr_len=sizeof(struct sockaddr_in);
				fd_client=accept(fd_accept,(struct sockaddr*)&client_addr,(socklen_t *)&client_addr_len);
				printf("new connection:%d\n",fd_client);

				/*add the newly arrived connection into epoll set*/
				event.data.fd=fd_client;
				event.events=EPOLLIN;//|EPOLLET;
				res=epoll_ctl(epfd,EPOLL_CTL_ADD,fd_client,&event);
				if(!res){/*add socket handle into hash table for fast index*/
					/*allocate tmp_endpoint*/
					tmp_endpoint=alloc_endpoint();
					assert(tmp_endpoint);
					endpoint_alloc_callback(tmp_endpoint);
					tmp_endpoint->fd_client=fd_client;
					reset_state_endpoint(tmp_endpoint);
					
					res=hashtable_set_key_value(socket_hash_tbl,(uint8_t*)&fd_client,sizeof(int),tmp_endpoint);
					assert(!res);
					
				}
				
			}else if(events[idx].events&EPOLLIN){
				fd_client=events[idx].data.fd;
				tmp_endpoint=hashtable_get_value(socket_hash_tbl,(uint8_t *)&fd_client,sizeof(int));
				if(tmp_endpoint==(void*)-1){/*if no endpoint found,this socket must be fake*/
					close(fd_client);
					continue;
				}
				ret_status=virtbus_logic_read_data(tmp_endpoint);
				switch(ret_status)
				{
					case read_return_close:
						/*1.delete fd form epoll set*/
						close_epoll_out(tmp_endpoint);//out first removed
						event.data.fd=tmp_endpoint->fd_client;
						event.events=EPOLLIN|EPOLLOUT;
						epoll_ctl(epfd,EPOLL_CTL_DEL,tmp_endpoint->fd_client,&event);
						
						/*2.close socket*/
						close(tmp_endpoint->fd_client);
						/*3 release fd from hash table*/
						//assert(hashtable_get_value(socket_hash_tbl,fd_client)==tmp_endpoint);
						
						res=hashtable_delete_key(socket_hash_tbl,(uint8_t *)&(tmp_endpoint->fd_client),sizeof(int));
						assert(!res);
						/*4.deallocate whatever is allocated*/
						endpoint_dealloc_callback(tmp_endpoint);
						/*5.release virtual_bus*/
						#if 0
						if(tmp_endpoint->bus_ptr){
							tmp_endpoint->bus_ptr->ref_counnt--;
							if(tmp_endpoint->bus_ptr->ref_counnt<=0){
								res=delete_key_from_name_tree(virtbus_rootnode,tmp_endpoint->bus_name);
								printf("[x]release virtual bus:%s\n",tmp_endpoint->bus_name);
								dealloc_virtual_bus(tmp_endpoint->bus_ptr);
							}
							
						}
						#endif
						
						free(tmp_endpoint);
						break;
					default:
						
						break;
				}
				
				
			}else if(events[idx].events&EPOLLOUT){ 
				fd_client=events[idx].data.fd;
				tmp_endpoint=hashtable_get_value(socket_hash_tbl,(uint8_t *)&fd_client,sizeof(int));
				if(tmp_endpoint==(void*)-1){
					close(fd_client);
					continue;
				}
				/*if there are some data pending,ok,we just send as possible as we can ,but once all data is sent,
				we should close the EPOLLOUT as soon as possible */
				if(tmp_endpoint->msg_send_buffer_pending){
					res=write(tmp_endpoint->fd_client,tmp_endpoint->msg_send_buffer+tmp_endpoint->msg_send_buffer_iptr,tmp_endpoint->msg_send_buffer_pending);
					tmp_endpoint->msg_send_buffer_iptr+=res;
					tmp_endpoint->msg_send_buffer_pending-=res;
				}
				if(!tmp_endpoint->msg_send_buffer_pending){
					//printf("no data sent,close epoll out\n");
					close_epoll_out(tmp_endpoint);
				}
			}
		}
	}	
	
	return TRUE;
	fails:
		return FALSE;
}
