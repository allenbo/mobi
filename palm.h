/**
 * The header of some solid format
 */
#ifndef _PALM_H_
#define _PALM_H_


#define PALM_WITHOUT_LIST 78

#define SWAB_SHORT(n)\
	(short) (((n)&(0x00ff))<<8 | ((n)&(0xff00))>>8)

#define SWAB_LONG(n) \
	(int) (((n)&(0x000000ff))<<24 | \
		   ((n)&(0x0000ff00))<<8 |\
		   ((n)&(0x00ff0000))>>8 |\
		   ((n)&(0xff000000))>>24)
	
/* the struct of record info*/
typedef struct record_info{
	int offset;
	int uniqueID;
}record_info;


/**
 * The header of palm database format
 */
typedef struct palm_database{
	char	name[32]; /*<-- the name of the book */
	unsigned short attributes;
		/* 0x0002 Read-Only
		 * 0x0004 Dirty AppInfoArea
		 * 0x0008 Backup this database
		 * 0x0010 (16 decimal) ok to install newer over existing copy,
		 *	if present on PalmPilot
		 * 0x0020 (32 decimal) Force the PalmPilot to reset after this
		 *	database installed
		 * 0x0040 (64 decimal) don't allow copy of file to be beamed
		 *	to other Pilot
		 */
	unsigned short version;
	int creation_date;
	int modification_date;
	int last_backup_date;

	int modification_number;
	int appInfoID;
	int sortInfoID;

	char type[4];
	char creator[4]; /*<-- the mobi file will be BOOKMOBI */

	int uniqueIDseed;
	int nextRecordListID; /*<-- Always set to zero in stored file */

	short number_of_records;
	
	record_info* list;
	short gap;
}palm_database;




/* the header of palmdoc_header */
typedef struct palmdoc_header{
	unsigned short compression; /*<-- 1==no compression 
				      2==palmdoc compression
				      17480==HUFF/CDIC compression*/
	short unused; /*<-- Always zero */
	int text_length; /*<-- uncompressed length of the entire book */
	short record_count; /*<--number of pdb records used for the text */
	short record_size; /*<--always be 4096 */
	int current_position;
}palmdoc_header;


/* the header of mobi */
typedef struct mobi_header{
	char identifier[4]; /*<-- the characters MOBI */
	int header_length; /*<-- te length of the mobi header, including mobi */
	int mobi_type; /* 2 mobipocket book
			  3 palmdoc book
			  ......*/
	int text_encoding; /* 1252 = winlatin1 65001=utf-8 */

	int uniqueID; /* random ?*/
	int file_version;

	int ortographic_index; //0xffffffff if index is not available
	int inflection_index; //0xffffffff if index is not available
	int index_names; //0xffffffff if index is not available
	int index_keys; //0xffffffff if index is not available

	int extra_index0; //0xffffffff if index is not available
	int extra_index1; //0xffffffff if index is not available
	int extra_index2; //0xffffffff if index is not available
	int extra_index3; //0xffffffff if index is not available
	int extra_index4; //0xffffffff if index is not available
	int extra_index5; //0xffffffff if index is not available

	int first_non_book_index;

	int full_name_offset; //offet in record 0 of the full name of the book
	int full_name_length; //length in bytes of the full name of the book

	unsigned int locale;
	int input_language; // input language for a dictionary
	int output_language;

	int min_version; //min_version to support the mobi file

	int first_image_index;
	int huffman_record_offset;//the first number of huffman compre record
	int huffman_record_count;//the number of huffman compre records

	int huffman_table_offset;
	int huffman_table_length;

	unsigned int exth_flag; //bitfield, if bit 6(0x40) is set, exth exists
	
	char unknow[32]; 

	int drm_offset; //0xffffffff if no DRM
	int drm_count; //0xffffffff if no DRM

	int drm_size;
	int drm_flags;

	char random[62];

	short extra_data_flags;
	int indx_record_offset;//0xffffffff if no indx record
}mobi_header;


typedef struct exth_record{
	int type;	//record type, can found in the table
	int length;	// the length of the record
	unsigned char* data;	//l-8
}exth_record;

typedef struct exth_header{
	char identifier[4]; // the characters EXTH
	int header_length;
	int record_count; // the number of records in exth header
	exth_record* list;
	char* padding;	//null bytes to pas the header to a multiple of four bytes
	int pad_len;
}exth_header;

typedef struct flis_record{
	char identifier[4];
	int unknow1;
	short unknow2;
	short unknow3;
	int unknow4;
	int unknow5;
	short unknow6;
	short unknow7;
	int unknow8;
	int unknow9;
	int unknow10;
}flis_record;

typedef struct fcis_record {
	char identifier[4];
	int  unknow1;
	int  unknow2;
	int  unknow3;
	int  unknow4;
	int  unknow5;
	int  unknow6;
	int  unknow7;
	int  unknow8;
	short unknow9;
	short unknow10;
	int  unknow11;
}fcis_record;

palm_database* palm_database_new_default(char* filename);
void palm_database_delete(palm_database* pdb);
palm_database* get_palm_database(char* filename);
int palm_database_write_to_file(palm_database* pdb, FILE* fp);

palmdoc_header* palmdoc_header_new_default();
void palmdoc_header_delete(palmdoc_header* pdh);
int palmdoc_header_write_to_file(palmdoc_header* pdh, FILE* fp);


mobi_header* mobi_header_new_default();
void mobi_header_delete(mobi_header* mh);
int mobi_header_write_to_file(mobi_header* mh, FILE* fp);

exth_header* exth_header_new_default(char* filename);
void exth_header_delete(exth_header* eh);
int exth_header_write_to_file(exth_header* eh, FILE* fp);


flis_record* flis_record_new_default();
int flis_record_write_to_file(flis_record* flis, FILE* fp);
void flis_record_delete(flis_record* flis);

fcis_record* fcis_record_new_default(int text_length);
int fcis_record_write_to_file(fcis_record* fcis, FILE* fp);
void fcis_record_delete(fcis_record* fcis);
#endif
