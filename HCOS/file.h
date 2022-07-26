#define ADR_DISKIMG		0x00100000
struct FILEINFO {
	unsigned char name[8], ext[3], type;
	char reserve[10];
	unsigned short time, date, clustno;
	unsigned int size;
};

//�������е�FAT��ѹ�� 
void file_readfat(int *fat, unsigned char *img);
void file_loadfile(int clustno, int size, char *buf, int *fat, char *img);
//���Ҵ����е��ļ� 
struct FILEINFO *file_search(char *name, struct FILEINFO *finfo, int max);
