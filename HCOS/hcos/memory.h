#include "rbtree.h" 

#define MEMMAN_FREES		360	/* ��Լ32KB */
#define MEMMAN_ADDR			0x003c0000    //����ָ������MEMMAN����ṹ����õ��ڴ�ռ俪ʼ��ַ 
#define EFLAGS_AC_BIT		0x00040000
#define CR0_CACHE_DISABLE	0x60000000

//������˫������˼���޸��ڴ���� 
struct FREEINFO {	// ������Ϣ
	unsigned int addr, size;
    struct rb_node rb_node_addr;    // ��ַ��������
    struct rb_node rb_node_size;    // ��С��������
    struct FREEINFO* pre;
    struct FREEINFO* next; //������Ϣ����preָ���nextָ��
    struct FREEINFO* pre2;
    struct FREEINFO* next2; //���ڴ�С�������preָ���nextָ��
};

struct MEMMAN {		// �ڴ����
	int lostsize, losts;
	struct rb_root root_addr; //��ַ����������ṹ�� 
    struct rb_root root_size; //��С����������ṹ��
	struct FREEINFO free[MEMMAN_FREES+2]; //�±�ΪMEMMAN_FREES�Ľ�㣬�ǿ��������ͷ��㣬�±�ΪMEMMAN_FREES+1�Ľ�㣬�ǿ��������ͷ��� 
};

unsigned int memtest(unsigned int start, unsigned int end);//�ڴ��������
void memman_init(struct MEMMAN *man);//�ڴ�����ʼ�� 
unsigned int memman_total(struct MEMMAN *man);//��������ڴ����� 
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size);//�����ڴ� 
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size);//�ͷ��ڴ� 
unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size);
int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size);
