#ifndef __VIRTBUS_CLIENT
#define __VIRTBUS_CLIENT
#include <stdio.h>
#include <stdlib.h>
#include <virtbus_logic.h>
#include <message.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <assert.h>
#include <errorcode.h>
#include <string.h>
#include <malloc.h>
#include <sys/un.h>

struct client_endpoint{

	int fd_sock;
	struct sockaddr_un  server_addr;
	
	int is_connected;
	/*int is_connected_to_virtual_bus;*/
	
	int is_joint;
	
	int send_buffer_length;
	char send_buffer[ENDPOINT_BUFFER_LENGTH];
	char recv_buffer[ENDPOINT_BUFFER_LENGTH];
	int recv_buffer_length;
	
	#if 0
	int nr_of_blocks;/*this will be updated to the real nr_of_blocks if the bus was registered before*/
	#endif
	char vm_domain[64];
	char vlink_name[64];
	int max_channels;
	int allocated_channels;
	uint32_t channels[16];
	uint8_t mac_address[6];
};
struct client_endpoint * client_endpoint_alloc_and_init(void);
void client_endpoint_uninit_and_dealloc(struct client_endpoint*cep);


int client_endpoint_init_virtual_link(struct client_endpoint*cep,enum VLINK_ROLE ,char* vdomain_name,int max_channels,char*vlink_name,char*mac,int nr_channels);
int client_endpoint_request_virtual_link(struct client_endpoint*cep,char * vlink_name,enum VLINK_ROLE _role);
void client_endpoint_uninit_and_dealloc(struct client_endpoint*cep);


int recv_tlv_message(struct  client_endpoint *cep);

#if 0
struct client_endpoint * client_endpoint_alloc_and_init(char * server_ip);
void client_endpoint_uninit_and_dealloc(struct client_endpoint*cep);

int client_endpoint_connect_to_virtual_bus(struct client_endpoint *cep,char *bus_name,int32_t cache_line_quantum);
int client_endpoint_read_virtual_bus_raw(struct client_endpoint*cep,int start_block_index,int nr_of_blocks,char*buffer);
int client_endpoint_read_virtual_bus_matched(struct client_endpoint *cep,int start_block_index,int nr_of_blocks,char*buffer,uint64_t* target_version,int *is_modified);
int client_endpoint_write_virtual_bus_generic(struct client_endpoint*cep,int start_block_index,int nr_of_blocks,char*buffer,uint64_t *updated_version_number);
int client_endpoint_write_virtual_bus_matched(struct client_endpoint*cep,
													int start_block_index,
													int nr_of_blocks,
													char*buffer,
													int *is_matched,
													uint64_t *target_version);



int recv_tlv_message(struct  client_endpoint *cep);
#endif

#endif
