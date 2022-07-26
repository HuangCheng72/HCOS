#define ADR_DISKIMG		0x00100000
struct FILEINFO {
	unsigned char name[8], ext[3], type;
	char reserve[10];
	unsigned short time, date, clustno;
	unsigned int size;
};

//将磁盘中的FAT解压缩 
void file_readfat(int *fat, unsigned char *img);
void file_loadfile(int clustno, int size, char *buf, int *fat, char *img);
//查找磁盘中的文件 
struct FILEINFO *file_search(char *name, struct FILEINFO *finfo, int max);
