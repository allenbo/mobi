#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "palm.h"



static char* author = "Allenbo";
static char* contributor = "[linjianfengqrh@gmail.com]";
static char* cdetype = "EBOK";

static int set_exth_record_from_str(exth_record* er, int type, char* data, int len);
static int set_exth_record_from_int(exth_record* er, int type, int data);

palm_database*
get_palm_database(char* filename) {
	if(filename == NULL) {
		fprintf(stderr, "The filename is empty!\n");
		return NULL;
	}

	FILE* fp = fopen(filename, "rb");
	if(NULL == fp) {
		fprintf(stderr, "Open file error!\n");
		return NULL;
	}

	palm_database* pdb = (palm_database*)malloc(sizeof(palm_database));
	if(NULL == pdb){
		fprintf(stderr, "Memory Error!\n");
		fclose(fp);
		return NULL;
	}

	//read the palm database from the file
	fread(pdb, PALM_WITHOUT_LIST, 1, fp);
	
	pdb->list = NULL;
	pdb->gap = 0;
	fclose(fp);

	return pdb;
}

palm_database*
palm_database_new_default(char* filename) {
	palm_database* pdb = (palm_database*)malloc(sizeof(palm_database));
	if(NULL == pdb) {
		fprintf(stderr, "Memory Error!\n");
		exit(1);
	}
	memset(pdb, 0 , sizeof(palm_database));
	memcpy(pdb, filename,strlen(filename)); 
	pdb->creation_date = time(NULL);
	pdb->modification_date = pdb->creation_date;
	
	memcpy(pdb->type, "BOOK", 4);
	memcpy(pdb->creator, "MOBI", 4);

	pdb->uniqueIDseed = 0xf;
	return pdb;
}


void 
palm_database_delete(palm_database* pdb) {
	if(NULL != pdb) {
		if(NULL  != pdb->list)
			free(pdb->list);
		free(pdb);
	}
}


int
palm_database_write_to_file(palm_database* pdb, FILE* fp) {
	if(NULL == pdb || NULL == pdb->list){
		fprintf(stderr, "palm_database is empty, cannot be written!\n");
		return 0;
	}
	fwrite(pdb->name, 32, 1, fp);
	short short_tmp = SWAB_SHORT(pdb->attributes);
	fwrite(&short_tmp, 2,1, fp);
	short_tmp = SWAB_SHORT(pdb->version);
	fwrite(&short_tmp, 2,1,fp);
	int int_tmp = SWAB_LONG(pdb->creation_date);
	fwrite(&int_tmp, 4, 1, fp);
	int_tmp = SWAB_LONG(pdb->modification_date);
	fwrite(&int_tmp, 4, 1, fp);
	int_tmp = SWAB_LONG(pdb->last_backup_date);
	fwrite(&int_tmp, 4, 1, fp);
	int_tmp =  SWAB_LONG(pdb->modification_number);
	fwrite(&int_tmp, 4, 1, fp);
	int_tmp = SWAB_LONG(pdb->appInfoID);
	fwrite(&int_tmp, 4, 1, fp);
	int_tmp = SWAB_LONG(pdb->sortInfoID);
	fwrite(&int_tmp, 4, 1, fp);

	fwrite(pdb->type, 4, 1, fp);
	fwrite(pdb->creator, 4, 1, fp);

	int_tmp = SWAB_LONG(pdb->uniqueIDseed);
	fwrite(&int_tmp, 4, 1, fp);
	int_tmp = SWAB_LONG(pdb->nextRecordListID);
	fwrite(&int_tmp, 4, 1, fp);

	short_tmp = SWAB_SHORT(pdb->number_of_records);
	fwrite(&short_tmp, 2, 1, fp);
	
	int i;
	for(i = 0; i < pdb->number_of_records; i ++) {
		int_tmp = SWAB_LONG(pdb->list[i].offset);
		fwrite(&int_tmp, 4,1, fp);
		int_tmp = SWAB_LONG(pdb->list[i].uniqueID);
		fwrite(&int_tmp, 4,1, fp);
	}

	/*fwrite(pdb, PALM_WITHOUT_LIST, 1, fp);
	fwrite(pdb->list, sizeof(record_info), pdb->number_of_records, fp);*/
	fwrite(&pdb->gap, 2,1,fp);
	return 1;
}


palmdoc_header*
palmdoc_header_new_default() {
	palmdoc_header* pdh = (palmdoc_header*)malloc(sizeof(palmdoc_header));
	if(NULL == pdh) {
		fprintf(stderr, "Memory Error!\n");
		exit(0);
	}

	memset(pdh, 0, sizeof(pdh));
	pdh->compression = 2;
	pdh->record_size = 4096;
	return pdh;
}


int
palmdoc_header_write_to_file(palmdoc_header* pdh, FILE* fp) {
	if(NULL == pdh) {
		fprintf(stderr,"palmdoc header is empty!\n");
		return 0;
	}

	short short_tmp;
	int   int_tmp;

	short_tmp = SWAB_SHORT(pdh->compression);
	fwrite(&short_tmp, 2,1, fp);
	short_tmp = SWAB_SHORT(pdh->unused);
	fwrite(&short_tmp, 2,1, fp);

	int_tmp = SWAB_LONG(pdh->text_length);
	fwrite(&int_tmp, 4, 1, fp);
	short_tmp = SWAB_SHORT(pdh->record_count);
	fwrite(&short_tmp, 2, 1, fp);

	short_tmp = SWAB_SHORT(pdh->record_size);
	fwrite(&short_tmp, 2,1, fp);

	int_tmp = SWAB_LONG(pdh->current_position);
	fwrite(&int_tmp, 4,1, fp);

	return 1;
}

void
palmdoc_header_delete(palmdoc_header* pdh) {
	if(NULL != pdh){
		free(pdh);
	}
	
}


mobi_header*
mobi_header_new_default() {
	mobi_header* mh = (mobi_header*)malloc(sizeof(mobi_header));
	if(NULL == mh) {
		fprintf(stderr, "Memory Error!\n");
		exit(1);
	}

	memcpy(mh->identifier, "MOBI", 4);
	mh->header_length = 0xe8;
	mh->mobi_type = 2;
	mh->text_encoding = 65001;

	mh->uniqueID = random();
	mh->file_version = 0x06;

	memset(mh+24, 0xff, 40); //set all indexs to ff

	mh->first_non_book_index = 0;
	mh->full_name_offset = 0;
	mh->full_name_length = 0;

	mh->locale = 2;
	mh->input_language = 0;
	mh->output_language = 0;

	mh->min_version = 6;

	memset(mh+92, 0, 20);

	mh->exth_flag = 0x50;

	memset(mh+116, 0,32);	//unknow

	mh->drm_offset = 0xffffffff;
	mh->drm_count = 0xffffffff;

	mh->drm_size = 0;
	mh->drm_flags = 0;

	memset(mh+164, 0, 62);

	mh->extra_data_flags = 01;
	mh->indx_record_offset = 0xffffffff;

	return mh;
}

int
mobi_header_write_to_file(mobi_header* mh, FILE* fp) {
	if(NULL == mh) {
		fprintf(stderr, "mobi header is empty!\n");
		return 0;
	}

	fwrite(&mh->identifier, 4, 1, fp);
	int int_tmp ;
	short short_tmp;

	int_tmp = SWAB_LONG(mh->header_length);
	fwrite(&int_tmp, 4, 1, fp);
	int_tmp = SWAB_LONG(mh->mobi_type);
	fwrite(&int_tmp, 4, 1, fp);
	int_tmp = SWAB_LONG(mh->text_encoding);
	fwrite(&int_tmp, 4, 1, fp);
	int_tmp = SWAB_LONG(mh->uniqueID);
	fwrite(&int_tmp, 4, 1, fp);
	int_tmp = SWAB_LONG(mh->file_version);
	fwrite(&int_tmp, 4, 1, fp);

	fwrite(mh+24, 40, 1, fp);

	int_tmp = SWAB_LONG(mh->first_non_book_index);
	fwrite(&int_tmp, 4, 1, fp);
	int_tmp = SWAB_LONG(mh->full_name_offset);
	fwrite(&int_tmp, 4, 1, fp);
	int_tmp = SWAB_LONG(mh->full_name_length);
	fwrite(&int_tmp, 4, 1, fp);
	int_tmp = SWAB_LONG(mh->locale);
	fwrite(&int_tmp, 4, 1, fp);
	int_tmp = SWAB_LONG(mh->input_language);
	fwrite(&int_tmp, 4, 1, fp);
	int_tmp = SWAB_LONG(mh->output_language);
	fwrite(&int_tmp, 4, 1, fp);
	int_tmp = SWAB_LONG(mh->min_version);
	fwrite(&int_tmp, 4, 1, fp);
	int_tmp = SWAB_LONG(mh->first_image_index);
	fwrite(&int_tmp, 4, 1, fp);

	fwrite(mh+96, 16, 1, fp);

	int_tmp = SWAB_LONG(mh->exth_flag);
	fwrite(&int_tmp, 4,1, fp);

	fwrite(mh->unknow, 32, 1, fp);

	fwrite(mh + 148, 8, 1, fp);

	int_tmp = SWAB_LONG(mh->drm_size);
	fwrite(&int_tmp, 4,1, fp);
	int_tmp = SWAB_LONG(mh->drm_flags);
	fwrite(&int_tmp, 4,1, fp);

	fwrite(mh + 164, 62, 1, fp);

	short_tmp = SWAB_SHORT(mh->extra_data_flags);
	fwrite(&short_tmp, 2, 1, fp);
	int_tmp = SWAB_LONG(mh->indx_record_offset);
	fwrite(&int_tmp, 4,1, fp);

	return 1;
}


void 
mobi_header_delete(mobi_header* mh) {
	if(NULL != mh)
		free(mh);
}


exth_header*
exth_header_new_default(char* filename) {
	int length = 0;
	exth_header* eh = (exth_header*)malloc(sizeof(exth_header));
	if(NULL == eh) {
		fprintf(stderr, "Memory Error!\n");
		exit(1);
	}

	memcpy(eh->identifier, "EXTH", 4);
	length += 4;
	length += 4;
	
	eh->record_count = 0x0a;
	length += 4;
	eh->list = (exth_record*)malloc(sizeof(exth_record)*(eh->record_count));
	length += set_exth_record_from_str(&eh->list[0], 0x64, author,strlen(author));
	length += set_exth_record_from_str(&eh->list[1], 0x6c, contributor, strlen(contributor));
	length += set_exth_record_from_str(&eh->list[2], 0x1f7, filename, strlen(filename));
	length += set_exth_record_from_str(&eh->list[3], 0x1f5, cdetype, strlen(cdetype));
	char str[33] = {0};
	struct tm* ptr;
	time_t timer = time(NULL);
	ptr = localtime(&timer);
	strftime(str,33,"%FT%T+00:00", ptr);
	length += set_exth_record_from_str(&eh->list[4], 0x6a, str, strlen(str));

	length += set_exth_record_from_int(&eh->list[5], 0xcc, 0xc9);
	length += set_exth_record_from_int(&eh->list[6], 0xcd, 0x01);
	length += set_exth_record_from_int(&eh->list[7], 0xce, 0x02);
	length += set_exth_record_from_int(&eh->list[8], 0xcf, 0x811b);
	length += set_exth_record_from_int(&eh->list[9], 0x74, 0x00);
	int pad = length % 4;
	if( pad != 0){
		eh->padding = (char*)malloc(sizeof(char)*(4-pad));
		memset(eh->padding, 0, 4-pad);
		eh->pad_len = 4 - pad;
		eh->header_length = length + 4 - pad;
	}
	eh->padding = NULL;
	eh->header_length = length;
	return eh;
}


int
exth_header_write_to_file(exth_header* eh, FILE* fp) {
	if(NULL == eh || NULL == eh->list){
		fprintf(stderr, "exth header is empty!\n");
		return 0;
	}
	
	fwrite(eh->identifier, 4, 1, fp);
	int tmp;
	tmp = SWAB_LONG(eh->header_length);
	fwrite(&tmp, 4, 1, fp);
	tmp = SWAB_LONG(eh->record_count);
	fwrite(&tmp, 4, 1, fp);

	int i;
	for(i = 0; i < eh->record_count; i ++) {
		tmp = SWAB_LONG(eh->list[i].type);
		fwrite(&tmp, 4, 1, fp);
		tmp = SWAB_LONG(eh->list[i].length);
		fwrite(&tmp, 4, 1, fp);
		fwrite(eh->list[i].data, eh->list[i].length-8, 1, fp);
	}

	if(eh->padding != NULL)
		fwrite(eh->padding, eh->pad_len, 1, fp);

	return 1;
}


void
exth_header_delete(exth_header* eh) {
	if(NULL != eh){
		if(NULL != eh->list)
			free(eh->list);
		if(NULL != eh->padding)
			free(eh->padding);
		free(eh);
	}
}

flis_record* 
flis_record_new_default() {
	flis_record* flis = (flis_record*)malloc(sizeof(flis_record));
	if (NULL == flis) {
		fprintf(stderr, "Memory Error!\n");
		exit(1);
	}

	memcpy(flis->identifier, "FLIS", 4);
	flis->unknow1 = 0x8000000;
	flis->unknow2 = 0x4100;
	flis->unknow3 = 0;
	flis->unknow4 = 0;
	flis->unknow5 = 0xffffffff;
	flis->unknow6 = 0x100;
	flis->unknow7 = 0x300;
	flis->unknow8 = 0x3000000;
	flis->unknow9 = 0x1000000;
	flis->unknow10 = 0xffffffff;

	return flis;
}

int
flis_record_write_to_file(flis_record* flis, FILE* fp) {
	if(NULL == flis) {
		fprintf(stderr, "flis record is empty!\n");
		return 0;
	}

	fwrite(flis, sizeof(flis_record), 1, fp);

	return 1;
}

void
flis_record_delete(flis_record* flis) {
	if(NULL != flis) 
		free(flis);
}


fcis_record*
fcis_record_new_default(int text_length) {
	fcis_record* fcis = (fcis_record*)malloc(sizeof(fcis_record));
	if(NULL == fcis) {
		fprintf(stderr, "Memory Error!\n");
		exit(1);
	}
	memcpy(fcis->identifier, "FCIS", 4);
	fcis->unknow1 = 0x14000000;	
	fcis->unknow2 = 0x10000000;	
	fcis->unknow3 = 0x1000000;	
	fcis->unknow4 = 0x0;
	fcis->unknow5 = SWAB_LONG(text_length);
	fcis->unknow6 = 0x0;
	fcis->unknow7 = 0x20000000;
	fcis->unknow8 = 0x8000000;
	fcis->unknow9 = 0x100;
	fcis->unknow10 = 0x100;
	fcis->unknow11 = 0x0;
	return fcis;
}

int
fcis_record_write_to_file(fcis_record* fcis, FILE* fp) {
	if(NULL == fcis) {
		fprintf(stderr, "fcis record is empty!\n");
		return 0;
	}
	fwrite(fcis, sizeof(fcis_record), 1 , fp);
	return 1;
}

void
fcis_record_delete(fcis_record* fcis) {
	if(NULL != fcis)
		free(fcis);
}

static int
set_exth_record_from_str(exth_record* er, int type, char* data, int len) {
	er->type = type;
	er->data = (char*)malloc(len);
	memcpy(er->data, data, len);
	er->length  = 8 + len;
	return er->length;
}
static int
set_exth_record_from_int(exth_record* er, int type,  int data) {
	er->type = type;
	er->length = 12;
	er->data = (unsigned char*)malloc(sizeof(int));
	er->data[0] = (unsigned char)((data&0xff000000)>>24);
	er->data[1] = (unsigned char)((data&0xff0000)>>16);
	er->data[2] = (unsigned char)((data&0xff00)>>8);
	er->data[3] = (unsigned char)(data&0xff);
	return er->length;
}
