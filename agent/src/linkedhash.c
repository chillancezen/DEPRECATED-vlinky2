//modified by jzheng:
//1.substitute the hash algorithm with JHASH
//2.change the key,value type
// Linked Hash Table implementation.
// v1.0

// The MIT License (MIT)
//
// Copyright (c) 2015 Adrian Bueno Jimenez  (adrian.buenoj@gmail.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <linkedhash.h>
#include <inttypes.h>

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


hashnode_t *hashnode_create( /*int*/uint8_t* key, int key_len,void *value, hashnode_t *next){

	hashnode_t *node = NULL;

	node = (hashnode_t *) malloc (sizeof(hashnode_t));
	if(node == NULL) return NULL;

	memcpy(node->key,key,key_len);
	/*strcpy((char*)node->key,(char*)key);*/
	/*node->key = strdup((char*)key);*/
	//node->key = (key);
	#if 0
	if(node->key == NULL){
		free(node);
		return NULL;
	}
	#endif
	//node->value = strdup(value);
	node->value = (value);
	#if 0
	if(node->value == NULL){
		free(node->key);
		free(node);
		return NULL;
	}
	#endif

	node->next = next;

	return node;
}


/*char **//*int*/uint8_t* hashnode_get_key(hashnode_t *node){
	if (node == NULL) return 0;
	return (uint8_t*)node->key;
}

/*
 *
char*/void *hashnode_get_value(hashnode_t *node){
	if (node == NULL) return NULL;
	return node->value;
}

/*
 *
 */
hashnode_t *hashnode_get_next(hashnode_t *node){
	if (node == NULL) return NULL;
	return node->next;
}

/*
 * Free allocated memory from one node
 */
void hashnode_delete(hashnode_t *node){

	if(node == NULL) return;
	#if 0
	free(node->key);
	free(node->value);
	#endif
	free(node);
	return;
}

/*
 * Free allocated memory from one node and the next ones in the list.
 */
void hashnode_delete_all(hashnode_t *node){

	if(node == NULL)
		return;

	hashnode_delete_all(node->next);
	node->next = NULL;

	hashnode_delete(node);

	return;
}

/*
 * CHange the value of a node.
 */
int hashnode_set_value(hashnode_t *node, /*char*/void *value){

	if(node == NULL) return -1;
	#if 0
	free(node->value);
	
	if(value != NULL){
		node->value = strdup(value);
		if(node->value == NULL)
			return -1;
	}
	#endif
	node->value=value;
	return 0;
}

/*
 * Searh in the list of nodes and returns the value if the node with that key exits.
 */
void *hashnode_search_and_get(hashnode_t *node, /*char **/ /*int*/uint8_t* key,int key_len){

	if(node == NULL) return (void*)-1;

	#if 1
	if(memcmp(node->key,key,key_len)==0)
	/*if(strcmp((char*)node->key, (char*)key) == 0)*/
	#else
	if(node->key==key)
	#endif
		return node->value;

	return hashnode_search_and_get(node->next, key,key_len);
}

/*
 * Search if the key exits, if not, it creates one node with that key and value.
 */
hashnode_t *hashnode_search_and_set(hashnode_t *node, /*char **/ /*int*/uint8_t* key,int key_len, /*char*/void *value){

	#if 0
	if(key == NULL) return NULL;
	#endif 
	
	if(node == NULL)
		return hashnode_create(key,key_len, value, NULL);

	if(memcmp(node->key,key,key_len)==0/*strcmp((char*)node->key, (char*)key) == 0*//*node->key==key*/){
		hashnode_set_value(node, value);
		return node;
	}

	node->next = hashnode_search_and_set(node->next, key,key_len,value);
	return node;
}

/*
 *
 */
hashtable_t *hashtable_create(int size){

    hashtable_t *hashtable = NULL;
    int i;

    if(size < 1) return NULL;

    hashtable = (hashtable_t *) malloc (sizeof(hashtable_t));
    if(hashtable == NULL) return NULL;

    hashtable->size = size;
    hashtable->table = (hashnode_t **) malloc (size * sizeof(hashnode_t*));
    if(hashtable->table == NULL) return NULL;

    for(i = 0; i < size; i++)
        hashtable->table[i] = NULL;

    return hashtable;
}

/*
 * Using djb2 algorithm by Dan Bernstein
 */
unsigned long get_string_hash_value(char *str){

	unsigned long hash = 5381;
	int c;

	while ((c = *str++))
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

	return hash;
}

/*
 *
 */
unsigned long hashtable_calculate_key_position(hashtable_t *hashtable, /*char **//*int*/uint8_t* key,int key_len){

	return (jhash(key,/*(int)strlen((char*)key)*/key_len,0x12345678)&0x7fffffff)%hashtable->size;
	#if 0
    return get_string_hash_value(key) % hashtable->size;
	#endif
}

/*
 *
 */
int hashtable_set_key_value(hashtable_t *hashtable, /*char **//*int*/uint8_t* key,int key_len, void *value){

    unsigned long key_pos = 0;

    if(hashtable == NULL/* || key == NULL*/) return -1;

    key_pos = hashtable_calculate_key_position(hashtable, key,key_len);
	//printf("key pos:%d->%d\n",*key,key_pos);

	hashtable->table[key_pos] = hashnode_search_and_set(hashtable->table[key_pos], key,key_len,value);
	if(hashtable->table[key_pos] == NULL)
		return -1;

	return 0;
}

/*
 *
 
char*/void *hashtable_get_value(hashtable_t *hashtable, /*char **//*int*/uint8_t* key,int key_len){

    unsigned long key_pos = 0;

	if(hashtable == NULL /*|| key == NULL*/) return (void*)-1;

	key_pos = hashtable_calculate_key_position(hashtable, key,key_len);
	//printf("key pos:%d->%d\n",key,key_pos);
	return hashnode_search_and_get(hashtable->table[key_pos], key,key_len);
}

/*
 *
 */
void hashtable_delete(hashtable_t *hashtable){

    int i;

    if(hashtable == NULL) return;

	//delete all possible lists
    for(i = 0; i < hashtable->size; i++)
		hashnode_delete_all(hashtable->table[i]);

	free(hashtable->table);
	free(hashtable);

    return;
}
int hashtable_delete_key(hashtable_t * hashtable,/*int*/ uint8_t* key,int key_len)
{
	hashnode_t *node;
	hashnode_t * pre_node=NULL;
	if(hashtable==NULL)
		return -1;
	unsigned long key_pos=hashtable_calculate_key_position(hashtable,key,key_len);
	//printf("del key pos:%s->%d\n",key,key_pos);
	for(node=hashtable->table[key_pos];node;node=node->next){
		if(/*node->key==key*//*!strcmp((char*)node->key,(char*)key)*/!memcmp(node->key,key,key_len))
			break;
		pre_node=node;
	}
	
	if(!node)
		return -1;
	
	if(node==hashtable->table[key_pos]){
		hashtable->table[key_pos]=node->next;
	}else{
		pre_node->next=node->next;
	}
	
	hashnode_delete(node);
	
	return 0;
}
