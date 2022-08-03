//多任务
#define AR_TSS32		0x0089
#define MAX_TASKS		1000	//最大任务数
#define TASK_GDT0		3		//定义从GDT的几号开始分配给TSS
#define MAX_TASKS_LV	100     //单一级别最多任务数量
#define MAX_TASKLEVELS	10      //最多任务级别
struct TSS32 {
	int backlink, esp0, ss0, esp1, ss1, esp2, ss2, cr3;
	int eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
	int es, cs, ss, ds, fs, gs;
	int ldtr, iomap;
};
struct TASK {
	int sel, flags; //SEL是用来存放GDT的编号
	int level, priority; //在第几个级别，优先级
	struct FIFO32 fifo;//任务所使用的缓冲区 
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
	int running; //正在运行任务数
	int now; //记录当前运行的是哪个任务
	struct TASK *tasks[MAX_TASKS_LV]; //存放的任务指针
};
struct TASKCTL {
	int now_lv; //当前运行的任务
	char lv_change; //下次任务变动
	struct TASKLEVEL level[MAX_TASKLEVELS];
	struct TASK tasks0[MAX_TASKS];
};
extern struct TASKCTL *taskctl;
extern struct TIMER *task_timer;
struct TASK *task_now(void);
struct TASK *task_init(struct MEMMAN *memman);
struct TASK *task_alloc(void);
void task_run(struct TASK *task, int level, int priority);
void task_switch(void);
void task_sleep(struct TASK *task);
