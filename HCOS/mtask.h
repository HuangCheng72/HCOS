//������
#define AR_TSS32		0x0089
#define MAX_TASKS		1000	//���������
#define TASK_GDT0		3		//�����GDT�ļ��ſ�ʼ�����TSS
#define MAX_TASKS_LV	100     //��һ���������������
#define MAX_TASKLEVELS	10      //������񼶱�
struct TSS32 {
	int backlink, esp0, ss0, esp1, ss1, esp2, ss2, cr3;
	int eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
	int es, cs, ss, ds, fs, gs;
	int ldtr, iomap;
};
struct TASK {
	int sel, flags; //SEL���������GDT�ı��
	int level, priority; //�ڵڼ����������ȼ�
	struct FIFO32 fifo;//������ʹ�õĻ����� 
	struct TSS32 tss; //TSS
    struct SEGMENT_DESCRIPTOR ldt[2];
	struct CONSOLE *cons;
	int ds_base, cons_stack;
	struct FILEHANDLE *fhandle;
	int *fat;
	char *cmdline;
	unsigned char langmode, langbyte1;
};
struct TASKLEVEL {
	int running; //��������������
	int now; //��¼��ǰ���е����ĸ�����
	struct TASK *tasks[MAX_TASKS_LV]; //��ŵ�����ָ��
};
struct TASKCTL {
	int now_lv; //��ǰ���е�����
	char lv_change; //�´�����䶯
	struct TASKLEVEL level[MAX_TASKLEVELS];
	struct TASK tasks0[MAX_TASKS];
};
extern struct TIMER *task_timer;
struct TASK *task_now(void);
struct TASK *task_init(struct MEMMAN *memman);
struct TASK *task_alloc(void);
void task_run(struct TASK *task, int level, int priority);
void task_switch(void);
void task_sleep(struct TASK *task);
