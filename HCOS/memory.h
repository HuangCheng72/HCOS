#define MEMMAN_FREES		4090	/* 大约32KB */
#define MEMMAN_ADDR			0x003c0000
#define EFLAGS_AC_BIT		0x00040000
#define CR0_CACHE_DISABLE	0x60000000

struct FREEINFO {	/* 可用信息 */
	unsigned int addr, size;
};

struct MEMMAN {		/* 内存管理 */
	int frees, maxfrees, lostsize, losts;
	struct FREEINFO free[MEMMAN_FREES];
};

unsigned int memtest(unsigned int start, unsigned int end);//内存容量检查
void memman_init(struct MEMMAN *man);//内存管理初始化 
unsigned int memman_total(struct MEMMAN *man);//计算可用内存总量 
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size);//申请内存 
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size);//释放内存 
