#include "rbtree.h" 

#define MEMMAN_FREES		360	/* 大约32KB */
#define MEMMAN_ADDR			0x003c0000    //这里指定的是MEMMAN这个结构体放置的内存空间开始地址 
#define EFLAGS_AC_BIT		0x00040000
#define CR0_CACHE_DISABLE	0x60000000

//尝试用双向链表思想修改内存管理 
struct FREEINFO {	// 可用信息
	unsigned int addr, size;
    struct rb_node rb_node_addr;    // 地址红黑树结点
    struct rb_node rb_node_size;    // 大小红黑树结点
    struct FREEINFO* pre;
    struct FREEINFO* next; //可用信息结点的pre指针和next指针
    struct FREEINFO* pre2;
    struct FREEINFO* next2; //用于大小红黑树的pre指针和next指针
};

struct MEMMAN {		// 内存管理
	int lostsize, losts;
	struct rb_root root_addr; //地址红黑树根结点结构体 
    struct rb_root root_size; //大小红黑树根结点结构体
	struct FREEINFO free[MEMMAN_FREES+2]; //下标为MEMMAN_FREES的结点，是可用链表的头结点，下标为MEMMAN_FREES+1的结点，是空余链表的头结点 
};

unsigned int memtest(unsigned int start, unsigned int end);//内存容量检查
void memman_init(struct MEMMAN *man);//内存管理初始化 
unsigned int memman_total(struct MEMMAN *man);//计算可用内存总量 
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size);//申请内存 
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size);//释放内存 
unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size);
int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size);
