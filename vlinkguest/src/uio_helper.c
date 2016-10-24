/*Userspace I/O helper routine*/

#include <uio_helper.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>



int _read_1st_line_from_file(char*path,char * buffer,int max_length)
{
	FILE *fp=fopen(path,"r");
	if(!fp)
		return -1;
	
	if(!fgets(buffer,max_length,fp)){
		fclose(fp);
		return -1;
	}
	fclose(fp);
	return 0;
}
int find_uio_device_number_by_vendor_and_device_id(uint16_t vendor_id,uint16_t device_id)
{
	#define MAX_POSSIBLE_DEVICE_NR 64
	int idx,rc;
	char path[256];
	char buffer[256];
	int tmp_vendor_id;
	int tmp_device_id;
	
	for(idx=0;idx<MAX_POSSIBLE_DEVICE_NR;idx++){
		/*1.phaze :vendor*/
		memset(path,0x0,sizeof(path));
		memset(buffer,0x0,sizeof(buffer));
		sprintf(path,"/sys/class/uio/uio%d/device/vendor",idx);
		rc=_read_1st_line_from_file(path,buffer,sizeof(buffer));
		if(rc<0)
			continue;
		sscanf(buffer,"0x%x\n",&tmp_vendor_id);
		
		/*2.phaze :device*/
		memset(path,0x0,sizeof(path));
		memset(buffer,0x0,sizeof(buffer));
		sprintf(path,"/sys/class/uio/uio%d/device/device",idx);
		rc=_read_1st_line_from_file(path,buffer,sizeof(buffer));
		if(rc<0)
			continue;
		sscanf(buffer,"0x%x\n",&tmp_device_id);
		
		/*3.phaze :compare*/
		if((tmp_vendor_id==vendor_id) && (tmp_device_id==device_id))
			return idx;
		
	}
	return -1;
}
int get_uio_port_info(int dev_num,int port_num,int*start,int*size)
{
	char buffer[256];
	char path[256];
	int rc;
	memset(path,0x0,sizeof(path));
	memset(buffer,0x0,sizeof(buffer));
	sprintf(path,"/sys/class/uio/uio%d/portio/port%d/start",dev_num,port_num);
	rc=_read_1st_line_from_file(path,buffer,sizeof(buffer));
	if(rc<0)
		return -1;
	sscanf(buffer,"0x%x\n",start);

	memset(path,0x0,sizeof(path));
	memset(buffer,0x0,sizeof(buffer));
	sprintf(path,"/sys/class/uio/uio%d/portio/port%d/size",dev_num,port_num);
	rc=_read_1st_line_from_file(path,buffer,sizeof(buffer));
	if(rc<0)
		return -1;
	sscanf(buffer,"0x%x\n",size);
	
	return 0;
}

int get_uio_memory_info(int dev_num,int map_num,int *bar,int *size)
{
	char buffer[256];
	char path[256];
	int rc;
	memset(path,0x0,sizeof(path));
	memset(buffer,0x0,sizeof(buffer));
	sprintf(path,"/sys/class/uio/uio%d/maps/map%d/name",dev_num,map_num);
	rc=_read_1st_line_from_file(path,buffer,sizeof(buffer));
	if(rc<0)
		return -1;
	sscanf(buffer,"BAR%d\n",bar);

	memset(path,0x0,sizeof(path));
	memset(buffer,0x0,sizeof(buffer));
	sprintf(path,"/sys/class/uio/uio%d/maps/map%d/size",dev_num,map_num);
	rc=_read_1st_line_from_file(path,buffer,sizeof(buffer));
	if(rc<0)
		return -1;
	sscanf(buffer,"0x%x\n",size);
	
	return 0;
}
void * map_uio_region_memory(int dev_num,int bar_num,int is_prefetchable,int size)
{	
	void * mapped_address=NULL;
	char path[256];
	int fd;

	memset(path,0x0,sizeof(path));
	sprintf(path,"/sys/class/uio/uio%d/device/resource%d%s",dev_num,bar_num,is_prefetchable?"_wc":"");
	fd=open(path,O_RDWR);
	if(fd<0)
		return NULL;
	
	printf("%s %d\n",path,size);
	mapped_address=mmap(NULL,size,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
	close(fd);

	return mapped_address;
}
void * map_uio_region_memory_from_char_device(int dev_num,int map_num,int size)
{
	void * mapped_address=NULL;
	char path[256];
	int fd;

	memset(path,0x0,sizeof(path));
	sprintf(path,"/dev/uio%d",dev_num);
	fd=open(path,O_RDWR);
	if(fd<0)
		return NULL;
	mapped_address=mmap(NULL,size,PROT_READ|PROT_WRITE,MAP_SHARED,fd,map_num*getpagesize());
	
	close(fd);
	return mapped_address;
}


uint64_t _uio_preserve_vm_area(int nr_pages,void **raw_addr)
{
	void* addr=NULL;
	int fd;
	fd=open("/dev/zero",O_RDONLY);
	if(fd<0)
		goto ret;
	addr=mmap(0,(nr_pages+2)*_HUGEPAGE_2M, PROT_READ, MAP_PRIVATE, fd, 0);
	if(!addr)
		goto fails;
	/*printf("[x]unaligned preserved memory start address:%"PRIx64"\n",(uint64_t)addr);*/
	if(raw_addr)
		*raw_addr=(void*)addr;
	munmap(addr,(nr_pages+2)*_HUGEPAGE_2M);
	
	addr=(void*)((_HUGEPAGE_2M+(uint64_t)addr)&(uint64_t)(~_HUGEPAGE_2M_MASK));
	/*printf("[x]aligned preserved memory start address:%"PRIx64"\n",(uint64_t)addr);*/
	ret:
	return (uint64_t)addr;

	fails:
		close(fd);
		goto ret;
}

void * map_uio_region_memory_from_char_device_alligned(int dev_num,int map_num,int size)
{
	
	void * mapped_address=(void*)_uio_preserve_vm_area(size/_HUGEPAGE_2M+((size%_HUGEPAGE_2M)?1:0),NULL);
	char path[256];
	int fd;

	memset(path,0x0,sizeof(path));
	sprintf(path,"/dev/uio%d",dev_num);
	fd=open(path,O_RDWR);
	if(fd<0)
		return NULL;
	mapped_address=mmap(mapped_address,size,PROT_READ|PROT_WRITE,MAP_SHARED,fd,map_num*getpagesize());
	
	close(fd);
	return mapped_address;

}

