#include <nametree.h>
#include <dsm.h>
#include <virtbus.h>
#include <stdio.h>
#include <linkedhash.h>
#include <message.h>
#include <virtbus_logic.h>


int main()
{

#if 0

	//char dst[10][64];
	struct nametree_node * root_node=alloc_nametree_node();
	struct nametree_node * node;
	//initalize_nametree_node(root_node,"meeeow",NULL);
	

	struct nametree_node * sub_node=alloc_nametree_node();
	initalize_nametree_node(sub_node,"cute",NULL);
	add_node_to_parent(root_node,sub_node);

	#if 1

	struct nametree_node * sub_node1=alloc_nametree_node();
	initalize_nametree_node(sub_node1,"cute2",NULL);
	add_node_to_parent(root_node,sub_node1);

	//int rc=delete_node_from_parent(root_node,sub_node);
	
//	int rc=nametree_son_node_lookup(root_node,"cute1",&node);

	

	add_key_to_name_tree(root_node,"/meeeowworld",NULL,&node);
	
	add_key_to_name_tree(root_node,"/hello11/hhh",NULL,&node);
	add_key_to_name_tree(root_node,"/hello11/hhh1/hello",NULL,&node);
	add_key_to_name_tree(root_node,"/hello22/hoo",NULL,&node);
	add_key_to_name_tree(root_node,"/hello22/hoo1",NULL,&node);
	
	add_key_to_name_tree(root_node,"/helloworld/hoo1/cute/heloworld",NULL,&node);
	#endif
	add_key_to_name_tree(root_node,"/helloworld/hoo1/cute2/missant1",NULL,&node);
	//int rc=lookup_key_in_name_tree(root_node,"/hello22/hoo",&node);
	//printf("%d:%s\n",rc,node->node_key);
	int rc=delete_key_from_name_tree(root_node,"/helloworld/hoo1/cute2/missant1");
	
	dump_nametree(root_node,0);
	printf("%d\n",rc);


	printf("dsm cache lien:%d\n",1<<CACHE_LINE_BIT);
	

	struct dsm_item* arr=alloc_dsm_item_array(256);
	dealloc_dsm_item_array(arr);
	printf("%d\n",arr[12].block_address);
	
	char buffer[128];
	struct dsm_memory * dsm=alloc_dsm_memory("/cute/meeeow",64);
	
	write_dsm_memory_raw(dsm,3,2,"Hello ,virtual isa");
	update_dsm_memory_version_with_max_number(dsm,0,4);
	update_dsm_memory_version_with_max_number(dsm,3,4);
	update_dsm_memory_version_with_max_number(dsm,0,6);
	update_dsm_memory_version_with_specific_value(dsm,0,1,5);
	//updtae_dsm_memory_version_with_max_number(dsm,4,3);
//	updtae_dsm_memory_version_with_max_number(dsm,1,9);
	//int rc=read_dsm_memory_raw(dsm,3,2,buffer);
	
	printf("%d\n",match_version_number(dsm,0,1,5));
	//printf("%s\n",buffer);
	
	char buffer[128];
	uint64_t version=0;
	int rc;
	struct virtual_bus * bus=alloc_virtual_bus("/cute/meeeow",16);
	//issue_bus_write_raw(bus,1,1,"hello ,virtual bus");
	//write_dsm_memory_raw( bus->dsm,0,2,"hello world,meeeow");
	//update_dsm_memory_version_with_max_number(bus->dsm,0,2);
	rc=issue_bus_write_generic(bus,0,16,"heelo world");
	printf("rc:%d\n",rc);
	rc=issue_bus_write_matched(bus,1,1,"heelo mee",1);
	printf("rc:%d\n",rc);
	rc=issue_bus_read_matched(bus,0,1,buffer,&version);
	
	//dealloc_virtual_bus(bus);
	printf("%d %d :%s\n",rc,version,buffer);
	
	hashtable_t * ht=hashtable_create(1024);
	hashtable_set_key_value(ht,18921,(void*)12121);
	hashtable_set_key_value(ht,18923,(void*)12123);
	/*
	hashtable_set_key_value(ht,3,(void*)12);
	rc=hashtable_delete_key(ht,3);
	hashtable_set_key_value(ht,3,(void*)121);
	hashtable_set_key_value(ht,18922,(void*)1212);
	//rc=hashtable_delete_key(ht,3);
	=hashtable_get_value(ht,18922);
	*/
	//hashtable_delete_key(ht,18921);
	//hashtable_delete_key(ht,18923);
	int val;
	val=hashtable_get_value(ht,18921);
	printf(" %d\n",val);
	val=hashtable_get_value(ht,18923);
	printf(" %d\n",val);
	
	struct message_builder builder;
	char buffer[21];
	int rc1=message_builder_init(&builder, buffer,sizeof(buffer));
	struct tlv_header tlv;
	tlv.type=1;
	tlv.length=2;
	rc1=message_builder_add_tlv(&builder,&tlv,"helloworld");

	tlv.type=2;
	tlv.length=3;
	rc1=message_builder_add_tlv(&builder,&tlv,"helloworld");
	message_iterate(&builder,NULL,def_callback);
	printf("%d\n",builder.message_header_ptr->total_length);
	
	printf("%d\n",ENDPOINT_BUFFER_LENGTH);
	#endif

	int rc=start_virtbus_logic();
	return 0;
}
