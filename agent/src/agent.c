#include<agent.h>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <inttypes.h>
uint64_t map_shared_memory(char * name,int length)
{
	void* base=NULL;
	int fd;
	char filename[256];
	sprintf(filename,"/dev/shm/%s",name);
	fd=open(filename,O_RDWR|O_CREAT,0);
	if(fd<0)
		return 0;
	ftruncate(fd,length);
	base=mmap(NULL,length,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
	close(fd);
	/*if(base)
		memset(base,0x0,length);*/
	return (uint64_t)base;
}
void unmap_shared_memory(uint64_t base,int length)
{
	munmap((void*)base,length);
}
struct vm_domain* alloc_vm_domain(int max_channels,char*name)
{
	struct vm_domain * domain=NULL;
	
	domain=(struct vm_domain*)malloc(sizeof(struct vm_domain));
	if(domain){
		memset(domain,0x0,sizeof(struct vm_domain));
		strcpy((char*)domain->domain_name,name);
		domain->nr_links=0;
		domain->max_channels=max_channels;
		domain->head=NULL;
		domain->shm_length=domain->max_channels*4*
			(DEFAULT_CHANNEL_QUEUE_ELEMENT_SIZE*DEFAULT_CHANNEL_QUEUE_LENGTH+
			DEFAULT_CHANNEL_QUEUE_HEADER_ROOM);
		domain->shm_base=map_shared_memory((char*)domain->domain_name,domain->shm_length);
		if(!domain->shm_base)
			goto fails;
		
		/*channels track array allocation*/
		domain->channel_track=(uint8_t*)malloc(sizeof(uint8_t)*max_channels);
		{
			int idx;
			for(idx=0;idx<max_channels;idx++)
				domain->channel_track[idx]=0;
		}
		printf("[x]allocate vm_domain:%s\n",name);
	}
	normal:
	return domain;
	fails:
		if(domain){
			if(domain->channel_track)
				free(domain->channel_track);
			free(domain);
		}
		goto normal;
		return domain;
}
void free_vm_domain(struct vm_domain*domain)
{
	if(domain->shm_base)
		unmap_shared_memory(domain->shm_base,domain->shm_length);
	if(domain->channel_track)
		free(domain->channel_track);
	if(domain->head)
		printf("[x]:vm_domain %s virtual link is not freed completely\n",domain->domain_name);
	printf("[x]free vm domain:%s\n",domain->domain_name);
	
	free(domain);
}
void attach_virtual_link_to_vm_domain(struct vm_domain*domain,struct virtual_link* link)
{
	link->domain=domain;
	link->next=domain->head;
	domain->head=link;
	domain->nr_links++;
}
void detach_virtual_link_from_vm_domain(struct virtual_link* link)
{
	if(!link->domain)/*already detached from domain or not yet attached to a vm_domain*/
		return ;
	if(link->nr_channels_allocated)
		dealloc_channel_to_vm_domain(link);
	
	struct vm_domain * domain=link->domain;
	if(domain->head==link)
		domain->head=link->next;
	else{
		struct virtual_link * prev_link=domain->head;
		while(prev_link->next&&(prev_link->next!=link))
			prev_link=prev_link->next;
		if(prev_link->next)
			prev_link->next=prev_link->next->next;
	}
	domain->nr_links--;
	link->domain=NULL;
	link->next=NULL;
}
void dump_virtual_link_in_vm_domain(struct vm_domain* domain)
{
	struct virtual_link* link=domain->head;
	while(link){
		printf("[x] link:0x%"PRIx64"\n",(uint64_t)link);
		link=link->next;
	}
}
void _initialize_virtual_link_channel(struct virtual_link * link,void*channel_base)
{

	/*printf("[x]channel addr:%"PRIx64"\n",channel_base);	*/
}
struct virtual_link * alloc_virtual_link(char* name)
{
	struct virtual_link * link=NULL;
	link=(struct virtual_link*)malloc(sizeof(struct virtual_link));
	if(link){
		memset(link,0x0,sizeof(struct virtual_link));
		strcpy((char*)link->link_name,name);
		printf("[x]allocate virtual link:%s\n",link->link_name);
	}
	return link;
}
int allocate_channel_from_vm_domain(struct virtual_link* link,int channel_quantum)
{
	int idx;
	if(!link->domain){
		printf("[x]virtual link is not yet attached to vm domain\n");
		return -1;
	}
	if(link->nr_channels_allocated){
		printf("[x]already allocated channels resouce to vlink,please release them first\n");
		return -2;
	}
	for(idx=0;idx<link->domain->max_channels;idx++){
		if(!link->domain->channel_track[idx]){
			link->domain->channel_track[idx]=1;
			link->channels[link->nr_channels_allocated]=idx;
			link->nr_channels_allocated++;
			if(link->nr_channels_allocated>=channel_quantum)
				break;
		}
	}
	return 0;
}

int dealloc_channel_to_vm_domain(struct virtual_link * link)
{
	int idx;
	if(!link->domain){
		printf("[x]virtual link is not yet attached to vm domain\n");
		return -1;
	}
	for(idx=0;idx<link->nr_channels_allocated;idx++)
		link->domain->channel_track[link->channels[idx]]=0;
	link->nr_channels_allocated=0;	
	return 0;
}
void free_virtual_link(struct virtual_link* link)
{
	/*release channels resource first*/
	if(link->nr_channels_allocated)
		dealloc_channel_to_vm_domain(link);
	if(link->domain)
		detach_virtual_link_from_vm_domain(link);
	printf("[x]free virtual link:%s\n",link->link_name);
	
	free(link);

}