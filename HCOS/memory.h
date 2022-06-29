#define MEMMAN_FREES		4090	/* ��Լ32KB */
#define MEMMAN_ADDR			0x003c0000
#define EFLAGS_AC_BIT		0x00040000
#define CR0_CACHE_DISABLE	0x60000000

struct FREEINFO {	/* ������Ϣ */
	unsigned int addr, size;
};

struct MEMMAN {		/* �ڴ���� */
	int frees, maxfrees, lostsize, losts;
	struct FREEINFO free[MEMMAN_FREES];
};

unsigned int memtest(unsigned int start, unsigned int end);//�ڴ��������
void memman_init(struct MEMMAN *man);//�ڴ�����ʼ�� 
unsigned int memman_total(struct MEMMAN *man);//��������ڴ����� 
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size);//�����ڴ� 
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size);//�ͷ��ڴ� 
