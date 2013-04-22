/* LZ77 is a compression technique that used to compress text
 */

#ifndef _LZ_H_
#define _LZ_H_

typedef unsigned char byte;

typedef struct lz_ctxt{
	byte* scroll_window;
	int   scroll_window_size;
	int   max_scroll_window_size;
	int   min_scroll_window_size; 

	byte* forward_window;
	int   forward_window_size;
	int   max_forward_window_size;
	int   min_forward_window_size;

	byte* plain_data;
	int   plain_data_size;
	int   max_plain_data_size;
	int   min_plain_data_size;

	byte* compressed_data;
	int   compressed_data_size;

	int   byte_count;
}lz_ctxt;

enum {
	LZ_LITERAL = 0,
	LZ_LEN_DIS_PAIR,
	LZ_BYTE_PAIR,
	LZ_LITERAL_COUNT 
};
byte* decompress(byte* compressed_data, int len);

int compress(lz_ctxt* lz);

lz_ctxt* lz_ctxt_new(byte* plain_data);
lz_ctxt* lz_ctxt_new_full(byte* plain_data, int plain_data_size);

void lz_ctxt_delete(lz_ctxt* lz);
#endif
