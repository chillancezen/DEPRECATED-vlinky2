#include<hash.h>
#define jhash_size(n)   ((uint32_t)1<<(n))
#define jhash_mask(n)   (jhash_size(n)-1)

#define rol32(a,s) (((a)>>(s))|((a)<<(32-(s))))

#define __get_unaligned_cpuint32_t(a) (*(uint32_t*)(a))


#define __jhash_mix(a, b, c)			\
{						\
	a -= c;  a ^= rol32(c, 4);  c += b;	\
	b -= a;  b ^= rol32(a, 6);  a += c;	\
	c -= b;  c ^= rol32(b, 8);  b += a;	\
	a -= c;  a ^= rol32(c, 16); c += b;	\
	b -= a;  b ^= rol32(a, 19); a += c;	\
	c -= b;  c ^= rol32(b, 4);  b += a;	\
}
#define __jhash_final(a, b, c)			\
{						\
	c ^= b; c -= rol32(b, 14);		\
	a ^= c; a -= rol32(c, 11);		\
	b ^= a; b -= rol32(a, 25);		\
	c ^= b; c -= rol32(b, 16);		\
	a ^= c; a -= rol32(c, 4);		\
	b ^= a; b -= rol32(a, 14);		\
	c ^= b; c -= rol32(b, 24);		\
}

#define JHASH_INITVAL		0xdeadbeef



uint32_t jhash(void *key, uint32_t length, uint32_t initval)
{
	uint32_t a, b, c;
	uint8_t *k = (uint8_t*)key;

	/* Set up the internal state */
	a = b = c = JHASH_INITVAL + length + initval;

	/* All but the last block: affect some 32 bits of (a,b,c) */
	while (length > 12) {
		a += __get_unaligned_cpuint32_t(k);
		b += __get_unaligned_cpuint32_t(k + 4);
		c += __get_unaligned_cpuint32_t(k + 8);
		__jhash_mix(a, b, c);
		length -= 12;
		k += 12;
	}
	/* Last block: affect all 32 bits of (c) */
	/* All the case statements fall through */
	switch (length) {
	case 12: c += (uint32_t)k[11]<<24;
	case 11: c += (uint32_t)k[10]<<16;
	case 10: c += (uint32_t)k[9]<<8;
	case 9:  c += k[8];
	case 8:  b += (uint32_t)k[7]<<24;
	case 7:  b += (uint32_t)k[6]<<16;
	case 6:  b += (uint32_t)k[5]<<8;
	case 5:  b += k[4];
	case 4:  a += (uint32_t)k[3]<<24;
	case 3:  a += (uint32_t)k[2]<<16;
	case 2:  a += (uint32_t)k[1]<<8;
	case 1:  a += k[0];
		 __jhash_final(a, b, c);
	case 0: /* Nothing left to add */
		break;
	}

	return c;
}


//struct dpdk_physeg_str address_table_base[MEMORY_SEGMENT_TABLE_HASH_SIZE];
void address_table_init(struct dpdk_physeg_str* address_table_base);

struct dpdk_physeg_str* alloc_physeg_hash_table(void)
{
	struct dpdk_physeg_str* ret=NULL;
	ret=malloc(sizeof(struct dpdk_physeg_str)*MEMORY_SEGMENT_TABLE_HASH_SIZE);
	if(ret)
		address_table_init(ret);
	return ret;
}


void address_table_init(struct dpdk_physeg_str* address_table_base)
{
	int idx=0;
	//printf("[x] size of metadata:%d\n",sizeof(struct dpdk_physeg_str));
	for(;idx<MEMORY_SEGMENT_TABLE_HASH_SIZE;idx++){
		address_table_base[idx].key=0;
		address_table_base[idx].value=0;
	}
}

int add_key_value(struct dpdk_physeg_str* address_table_base,uint64_t key,uint64_t value)
{
	uint32_t hash_value;
	uint32_t hash_index;
	hash_value=jhash(&key,sizeof(key),JHASH_INITVAL);
	/*hash_value=rte_jhash(&key,sizeof(key),RTE_JHASH_GOLDEN_RATIO);*/
	hash_value&=(MEMORY_SEGMENT_TABLE_HASH_SIZE-1);
	hash_index=hash_value;
	/*printf("[x] %llx -->%llx %d\n",key,value,hash_index);*/
	do{
		if(address_table_base[hash_index].key==0){
			address_table_base[hash_index].key=key;
			address_table_base[hash_index].value=value;
			return 0;
		}
		hash_index=(hash_index+1)%MEMORY_SEGMENT_TABLE_HASH_SIZE;
	}while(hash_index!=hash_value);
	return -1;
}

uint64_t lookup_key(struct dpdk_physeg_str* address_table_base,uint64_t key)
{
	uint32_t hash_value;
	uint32_t hash_index;
	uint64_t ret=0;
	hash_value=jhash(&key,sizeof(key),JHASH_INITVAL);
	/*hash_value=rte_jhash(&key,sizeof(key),RTE_JHASH_GOLDEN_RATIO);*/
	hash_value&=(MEMORY_SEGMENT_TABLE_HASH_SIZE-1);
	hash_index=hash_value;
	do{
		if(address_table_base[hash_index].key==key){
			ret=address_table_base[hash_index].value;
			break;
		}
		hash_index=(hash_index+1)%MEMORY_SEGMENT_TABLE_HASH_SIZE;
	}while(hash_index!=hash_value);
	return ret;
}




