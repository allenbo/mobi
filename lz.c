#include "lz.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>


#define DATA_LENGTH 4096 
#define MAX_SCROLL_WINDOW_SIZE 2047
#define MIN_SCROLL_WINDOW_SIZE 3

#define MAX_FORWARD_WINDOW_SIZE 10
#define MIN_FORWARD_WINDOW_SIZE 3

#define MAX_PLAIN_DATA_SIZE 4096
#define MIN_PLAIN_DATA_SIZE 0

#define MAX_COUNT 0x08

static int get_byte_type(byte b);
static int get_distance(byte* b);
static int get_length(byte* b);
static int get_count(byte* b);
static int get_distance_length(lz_ctxt* lz, int* p_dis, int* p_len);


lz_ctxt*
lz_ctxt_new_full(byte* plain_data, int plain_data_size) {
	if(NULL == plain_data || 0 == plain_data_size \
			|| plain_data_size > MAX_PLAIN_DATA_SIZE){
		fprintf(stderr, "plain data error!\n");
		return NULL;
	}

	lz_ctxt* lz = (lz_ctxt*)malloc(sizeof(lz_ctxt));
	memset(lz,0, sizeof(lz_ctxt));
	lz->compressed_data = (byte*)malloc(sizeof(byte)*MAX_PLAIN_DATA_SIZE);
	memset(lz->compressed_data, 0, MAX_PLAIN_DATA_SIZE);

	lz->max_scroll_window_size =  MAX_SCROLL_WINDOW_SIZE;
	lz->min_scroll_window_size = MIN_SCROLL_WINDOW_SIZE;
	lz->max_forward_window_size = MAX_FORWARD_WINDOW_SIZE;
	lz->min_forward_window_size = MIN_FORWARD_WINDOW_SIZE;

	lz->max_plain_data_size = MAX_PLAIN_DATA_SIZE;
	lz->min_plain_data_size = MIN_PLAIN_DATA_SIZE;

	lz->plain_data = plain_data;
	lz->plain_data_size = plain_data_size;

	lz->scroll_window = plain_data;
	lz->forward_window = plain_data;

	return lz;
}

lz_ctxt*
lz_ctxt_new(byte* plain_data) {
	return lz_ctxt_new_full(plain_data, MAX_PLAIN_DATA_SIZE);
}

int
compress(lz_ctxt* lz) {
	if(NULL == lz) {
		fprintf(stderr, "Lz context is null!\n");
		return 0;
	}
	int i = 0;
	byte b;

	while(lz->byte_count < lz->plain_data_size) {
		if(lz->scroll_window_size < 3){
			if(*(lz->forward_window) < 0x80){
				lz->compressed_data[i++] = *(lz->forward_window);
				lz->scroll_window_size ++;
				lz->forward_window ++;
				lz->byte_count ++;
			}else{
				int count= get_count(lz->forward_window);
				lz->compressed_data[i++] = count;
				memcpy(lz->compressed_data+i,\
					lz->forward_window, count);
				lz->byte_count += count;
				i += count;
				lz->scroll_window_size += count;
				lz->forward_window +=count;
			}
		}else{
			int distance = 0;
			int length = 0;
			if(get_distance_length(lz, &distance, &length)) {
				byte pair0 = 0x80;
				pair0 |= (distance >> 5);
				byte pair1 = 0;
				pair1 = length-MIN_FORWARD_WINDOW_SIZE;
				pair1 = ((distance & 0x1f) << 3) | pair1;
				lz->compressed_data[i++] = pair0;
				lz->compressed_data[i++] = pair1;

			}else{
				if(*(lz->forward_window) < 0x80 && *(lz->forward_window) != 0x20) {
					lz->compressed_data[i++] = *(lz->forward_window);
					length = 1;
				}else if(*(lz->forward_window) == 0x20){
					if(*(lz->forward_window+1) >= 0x80 || *(lz->forward_window+1) < 0x40) {
						lz->compressed_data[i++] = *(lz->forward_window);
						length = 1;
					}else{
						byte b = *(lz->forward_window+1)^0x80;
						lz->compressed_data[i++] = b;
						length = 2;
					}
				}else{
					length = get_count(lz->forward_window);
					lz->compressed_data[i++] = length;
					memcpy(lz->compressed_data+i, lz->forward_window, length);
					i += length;
				}
			}
			lz->byte_count += length;
			if(lz->scroll_window_size == lz->max_scroll_window_size){
				lz->scroll_window += length;
			}else if(lz->scroll_window_size + length <= lz->max_scroll_window_size){
				lz->scroll_window_size += length;
			}else{
				lz->scroll_window += lz->scroll_window_size + length - lz->max_scroll_window_size;
				lz->scroll_window_size = lz->max_scroll_window_size;
			}
			lz->forward_window += length;
			lz->forward_window_size =\
				lz->byte_count + lz->max_forward_window_size < lz->plain_data_size?\
				lz->max_forward_window_size:lz->plain_data_size - lz->byte_count;
		}
	}
	lz->compressed_data_size = i;
	return 1;
}

byte*
decompress(byte* compressed_data, int len){
	byte *uncompressed_data = (byte*)malloc(sizeof(byte)*DATA_LENGTH);
	memset(uncompressed_data, DATA_LENGTH, 0);
	
	assert(len>0);

	int i,j;
	int distance;
	int length;
	int start;
	for (i = 0, j = 0; i < len; i ++, j ++) {
		switch(get_byte_type(compressed_data[i])){
			case LZ_LITERAL:
				uncompressed_data[j] = compressed_data[i];
				break;

			case LZ_LEN_DIS_PAIR:
				distance = get_distance(compressed_data+i);
				length = get_length(compressed_data+i+1);
				
				start = j - distance;
				memcpy(uncompressed_data+j,\
					uncompressed_data+start, length);
				j += length - 1;
				i++;
				break;

			case LZ_BYTE_PAIR:
				uncompressed_data[j] = ' ';
				j ++;
				uncompressed_data[j] = compressed_data[i]^0x80;
				break;
			case LZ_LITERAL_COUNT:
				length = compressed_data[i];
				memcpy(uncompressed_data+j, \
					compressed_data+i+1, length);
				i += length;
				j += length-1;
				break;
			default:
				break;
		}
	}
	return uncompressed_data;
}

void 
lz_ctxt_delete(lz_ctxt* lz) {
	if(NULL != lz) {
		if(NULL != lz->compressed_data)
			free(lz->compressed_data);
		free(lz);
	}
}

static int
get_byte_type(byte b) {
	if(b <= 0xbf && b >= 0x80){
		return LZ_LEN_DIS_PAIR;
	}else if(b >= 0xc0){
		return LZ_BYTE_PAIR;
	}else if(b >= 0x01 && b <= 0x08) {
		return LZ_LITERAL_COUNT;
	}else{
		return LZ_LITERAL;
	}
}

static int
get_distance(byte* b) {
	int distance = 0;
	distance = b[0] & 0x3f;
	distance = (distance << 5) + ((b[1] & 0xf8) >> 3);
	return distance;
}

static int
get_length(byte* b) {
	int length =0;
	length = ((*b) & 0x07) + 3;
	return length;
}

static int
get_count(byte* b) {
	int i;
	for(i = 0; i < MAX_COUNT && b[i] >= 0x80; i ++)
		;
	return i;
}

static int
get_distance_length(lz_ctxt* lz, int* p_dis, int* p_len) {
	int max_len = lz->min_forward_window_size - 1;
	int index = 0;
	byte* p = lz->scroll_window;
	while(index++ < lz->scroll_window_size) {
		if(*p != *(lz->forward_window)) {
			p ++;
			continue;
		}

		int len;
		for (len = 0; len < lz->forward_window_size && p[len] == lz->forward_window[len];\
				len ++)
			;
		if(len == lz->forward_window_size){
			max_len = len;
			*p_len = len;
			*p_dis = lz->scroll_window_size - index + 1;
			break;
		}else if (len >= lz->min_forward_window_size && len >= max_len){
			max_len = len;
			*p_len = len;
			*p_dis = lz->scroll_window_size - index + 1;
		}
		p ++;
	}
	if(max_len >= lz->min_forward_window_size)
		return 1;
	return 0;
}
