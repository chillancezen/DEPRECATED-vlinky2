#ifndef _LIB_VLINK_LIB_US
#define _LIB_VLINK_LIB_US
#include<inttypes.h> 

#define _HUGEPAGE_2M (1<<21)
#define _HUGEPAGE_2M_MASK ((1<<21)-1)

int find_uio_device_number_by_vendor_and_device_id(uint16_t vendor_id,uint16_t device_id);
int get_uio_port_info(int dev_num,int port_num,int*start,int*size);
int get_uio_memory_info(int dev_num,int map_num,int *bar,int *size);
void * map_uio_region_memory(int dev_num,int bar_num,int is_prefetchable,int size);
void * map_uio_region_memory_from_char_device(int dev_num,int map_num,int size);
void * map_uio_region_memory_from_char_device_alligned(int dev_num,int map_num,int size);

#endif