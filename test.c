#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "lz.h"
#include "palm.h"


void process(char* buffer, int len, char* filename);
int preprocess(char* filename, int* p_count);
void tmp_file_write_to_file(char* fileaname, FILE* fp);

void usage();
int main(int argc, char** argv) {

	char* filename = strdup(argv[1]);
	char* p = strrchr(filename, '.');
	*p = 0;

	if(argc != 2) {
		usage();
		return 0;
	}
	int count = 0;
	int text_length = 0;
	text_length = preprocess(argv[1], &count);

	// create the mobi file
	palmdoc_header* pdh = palmdoc_header_new_default();
	mobi_header* mh = mobi_header_new_default();
	exth_header* eh = exth_header_new_default(filename);
	
	pdh->text_length = text_length;
	pdh->record_count = count;
	printf("palmdoc header set done!\n");

	mh->first_non_book_index = count + 1;
	mh->first_image_index = count + 5;
	int offset = sizeof(palmdoc_header)+ mh->header_length + eh->header_length;
	mh->full_name_offset = offset; 
	mh->full_name_length = strlen(filename);
	printf("mobi header set done!\n");


	palm_database* pdb = palm_database_new_default(filename);
	pdb->number_of_records = count + 5;
	printf("palm_database set done!\n");

	pdb->list=(record_info*)malloc(sizeof(record_info)*pdb->number_of_records);

	int list_offset = PALM_WITHOUT_LIST + pdb->number_of_records * 8 + 2; 
	int id = 0;
	pdb->list[0].offset =  list_offset;
	pdb->list[0].uniqueID = id;

	list_offset += offset + 20; 
	id += 2;

	int i = 0;
	long size = 0;
	char  tmpname[20] = {0};
	FILE* fp = NULL;
	for(i = 0; i < count; i++) {
		memset(tmpname, 0, 20);
		sprintf(tmpname, "%d.tmp", i);
		if((fp = fopen(tmpname, "rb")) == NULL) {
			fprintf(stderr, "Cannot open this file ##%s\n", tmpname);
			exit(1);
		}
		fseek(fp, 0, SEEK_END);
		size = ftell(fp);
		fclose(fp);
		
		pdb->list[i + 1].offset = list_offset;
		pdb->list[i + 1].uniqueID = id;
		list_offset +=  size;
		id += 2;
	}
	
	pdb->list[count + 1].offset = list_offset;
	pdb->list[count + 1].uniqueID = id;
	list_offset += 3;
	id += 2;
	
	pdb->list[count + 2].offset = list_offset;
	pdb->list[count + 2].uniqueID = id;
	list_offset += sizeof(flis_record);
	id += 2;

	pdb->list[count + 3].offset = list_offset;
	pdb->list[count + 3].uniqueID = id;
	list_offset += sizeof(fcis_record);
	id += 2;

	pdb->list[count + 4].offset = list_offset;
	pdb->list[count + 4].uniqueID = id;
	printf("palm_database list set done!\n");

	char output_filename[32];
	strcpy(output_filename, filename);
	strcat(output_filename, ".mobi");

	FILE* fout = NULL;
	if(NULL == (fp = fopen(output_filename, "wb"))) {
		fprintf(stderr, "Cannot open the output file!##%s\n",output_filename);
		exit(1);
	}
	int flag = palm_database_write_to_file(pdb,fp);

	if(!flag) {
		fprintf(stderr, "Cannot write palm database!\n");
	}else{
		printf("palm database write done!\n");
		flag = palmdoc_header_write_to_file(pdh, fp);
		if(!flag) {
			fprintf(stderr, "Cannot write palm doc header!\n");
		}else{
			printf("palmdoc header write done!\n");
			flag = mobi_header_write_to_file(mh, fp);
			if(!flag) 
				fprintf(stderr, "Cannot write mobi header!\n");
			else{
				printf("mobi header write done!\n");
				flag = exth_header_write_to_file(eh, fp);
				if(!flag)
					fprintf(stderr, "Cannot write exth header!\n");
				else
					printf("exth header write done!\n");
			}
		}
	}
	fwrite(filename, 20, 1, fp);

	for(i = 0; i < count; i ++) {
		memset(tmpname, 0, 20);
		sprintf(tmpname, "%d.tmp", i);
		
		tmp_file_write_to_file(tmpname, fp);
		remove(tmpname);
	}
	printf("tmp file write done!\n");
	
	byte pad[3] = {0};
	fwrite(pad, 3,1, fp);
	printf("magic record write done!\n");
	
	flis_record* flis = flis_record_new_default();
	flag = flis_record_write_to_file(flis,fp);
	flis_record_delete(flis);

	if(!flag) 
		fprintf(stderr, "Cannot write flis record!\n");
	else  {
		printf("flis record write done!\n");
		fcis_record* fcis = fcis_record_new_default(text_length);
		flag = fcis_record_write_to_file(fcis, fp);
		fcis_record_delete(fcis);
		if(!flag)
			fprintf(stderr, "Cannot write fcis record!\n");
		else
			printf("fcis record write done!\n");
	}
	
	palm_database_delete(pdb);
	palmdoc_header_delete(pdh);
	mobi_header_delete(mh);
	exth_header_delete(eh);
	free(filename);
	printf("Convert successfully!\n");
	return 0;
}

int preprocess(char* filename, int* p_count) {
	char output_filename[20] = {0};	
	char result[4096 + 31] = {0};
	char buffer[4096] = {0};

	char* html = "<html>";
	char* end_html = "</html>";

	char* header = "<head>";
	char* end_header = "</head>";

	char* body = "<body>";
	char* end_body = "</body>";

	char* p = "<p height=\"1em\" width=\"0pt\">";
	char* end_p = "</p>";

	FILE* fp = fopen(filename, "rb");
	if(NULL == fp){
		fprintf(stderr, "Cannot open file ##%s\n", filename);
		exit(1);
	}
	
	int start = 0;
	int count = 0;
	int number = 0;
	int i = 0;
	int end_text = 0;
	int complete = 1;
	int file_count = 0;
	int text_length = 0;
	lz_ctxt* lz = NULL;


	strcpy(result, html);
	i += strlen(html);
	strcpy(result + i , header);
	i += strlen(header);
	strcpy(result + i , end_header);
	i += strlen(end_header);
	strcpy(result + i, body);
	i += strlen(body);
	
	int len = fread(buffer, 1, 4096, fp);
	text_length += len;
	while(len != 0){
		while(count  < len && i < 4096) {
			if(buffer[count] == '\r'){
				count ++;
				continue;
			}
			if(buffer[count] == '\n') {
				if(buffer[count - 1] == '\n') {
					count ++;
					continue;
				}
				memcpy(result + i - number, buffer + start, number);
				start = 0;
				number = 0;
			
				strcpy(result + i ,end_p);
				i += strlen(end_p);
				count ++;
				complete = 1;

			}else{
				if(complete && 0 == number) {
					start = count;
					strcpy(result + i, p);
					i += strlen(p);
				}
				number ++;
				count ++;
				i ++;
			}
		}
		if(i >= 4096){
			if(number != 0){
				memcpy(result + i - number, buffer + start , number);
				start = 0;
				number = 0;
				complete = 0;
			}
			lz = lz_ctxt_new(result);
			compress(lz);
			memset(output_filename, 0, sizeof(output_filename));
			sprintf(output_filename, "%d.tmp", file_count++);
			process(lz->compressed_data, lz->compressed_data_size,\
					output_filename);
			lz_ctxt_delete(lz);


			int left = len - count;
			memcpy(buffer, buffer + count, left);
			if(!end_text) {
				int read_number = fread(buffer + left, 1, count, fp);
				text_length += read_number;
				if(read_number != count){
					end_text = 1;
				}
				len = left + read_number;
			}else
				len = left;
				
			count = 0;
			
			left = i - 4096;
			if(left != 0)
				memcpy(result, result + 4096, left);
			i = left;
			memset(result + i, 0 ,4096 + 31 - i);
		}else{
			if(buffer[count-1] != '\n'){
				memcpy(result + i - number, buffer + start, number);
				start = 0;
				number = 0;
				count = 0;

				strcpy(result + i ,end_p);
				i += strlen(end_p);
			}
			strcpy(result + i, end_body);
			i += strlen(end_body);
			strcpy(result + i, end_html);
			i += strlen(end_html);

			lz = lz_ctxt_new_full(result, i);
			compress(lz);

			memset(output_filename, 0, sizeof(output_filename));
			sprintf(output_filename, "%d.tmp", file_count++);
			process(lz->compressed_data, lz->compressed_data_size,\
					output_filename);
			lz_ctxt_delete(lz);
			len = 0;
		}
	}
	fclose(fp);
	*p_count = file_count;
	return text_length;
}


void tmp_file_write_to_file(char* filename, FILE* fp) {
	FILE* fin = fopen(filename, "rb");
	if(NULL == fin) {
		fprintf(stderr, "Cannot open tmp file %s\n", filename);
		exit(1);
	}
	byte data[4096];
	int len = fread(data, 1, 4096, fin);
	fwrite(data, 1, len, fp);
	fclose(fin);
}
void process(char* buffer, int len, char* filename) {
	FILE* fp = fopen(filename, "wb");
	if(NULL == fp) {
		fprintf(stderr, "Cannot open file %s\n", filename);
		return;
	}

	fwrite(buffer, len, 1, fp);
	fclose(fp);
}

void usage() {
	printf("Wrong Input!");
	printf("Please input the file's name that you want to convert!");
}
