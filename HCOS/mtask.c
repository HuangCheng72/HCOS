#include "bootpack.h" 

struct TASKCTL *taskctl;
struct TIMER *task_timer;

struct TASK *task_now(void){
	struct TASKLEVEL *tl = &taskctl->level[taskctl->now_lv];
	return tl->tasks[tl->now];
}

void task_add(struct TASK *task){
	struct TASKLEVEL *tl = &taskctl->level[task->level];
	tl->tasks[tl->running] = task;
	tl->running++;
	task->flags = 2; //活跃状态
	return;
}

void task_remove(struct TASK *task){
	int i;
	struct TASKLEVEL *tl = &taskctl->level[task->level];

	//寻找待移除任务的位置
	for (i = 0; i < tl->running; i++) {
		if (tl->tasks[i] == task) {
			//找到了退出
			break;
		}
	}

	tl->running--;
	if (i < tl->now) {
		tl->now--; //当前任务的下标
	}
	if (tl->now >= tl->running) {
		//越界修正
		tl->now = 0;
	}
	task->flags = 1; //确定为静止状态

	//前移其他任务
	for (; i < tl->running; i++) {
		tl->tasks[i] = tl->tasks[i + 1];
	}

	return;
}

void task_switchsub(void){
	int i;
	//寻找下一个有任务的level
	for (i = 0; i < MAX_TASKLEVELS; i++) {
		if (taskctl->level[i].running > 0) {
			break; //找到了就退出
		}
	}
	taskctl->now_lv = i;
	taskctl->lv_change = 0;
	return;
}
//闲置任务
void task_idle(void){
	for (;;) {
		io_hlt();
	}
}

struct TASK *task_init(struct MEMMAN *memman){
	int i;
	struct TASK *task , *idle;
	struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;
	taskctl = (struct TASKCTL *) memman_alloc_4k(memman, sizeof (struct TASKCTL));
	for (i = 0; i < MAX_TASKS; i++) {
		taskctl->tasks0[i].flags = 0;
		taskctl->tasks0[i].sel = (TASK_GDT0 + i) * 8;
		set_segmdesc(gdt + TASK_GDT0 + i, 103, (int) &taskctl->tasks0[i].tss, AR_TSS32);
	}
	for (i = 0; i < MAX_TASKLEVELS; i++) {
		taskctl->level[i].running = 0;
		taskctl->level[i].now = 0;
	}
	task = task_alloc();
	task->flags = 2;	//正在活动中标志 
	task->priority = 2; //0.02秒
	task->level = 0;	//最高等级 
	task_add(task);
	task_switchsub();	//修改设置
	load_tr(task->sel);
	//申请任务切换定时器 
	task_timer = timer_alloc();
	timer_settime(task_timer, task->priority);
    
    //插入闲置任务
    idle = task_alloc();
	idle->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024;
	idle->tss.eip = (int) &task_idle;
	idle->tss.es = 1 * 8;
	idle->tss.cs = 2 * 8;
	idle->tss.ss = 1 * 8;
	idle->tss.ds = 1 * 8;
	idle->tss.fs = 1 * 8;
	idle->tss.gs = 1 * 8;
	task_run(idle, MAX_TASKLEVELS - 1, 1);
    
	return task;
}

struct TASK *task_alloc(void){
	int i;
	struct TASK *task;
	for (i = 0; i < MAX_TASKS; i++) {
		if (taskctl->tasks0[i].flags == 0) {
			task = &taskctl->tasks0[i];
			task->flags = 1; //正在使用中标志 
			task->tss.eflags = 0x00000202; // IF = 1;
			task->tss.eax = 0; //这里先置为0 
			task->tss.ecx = 0;
			task->tss.edx = 0;
			task->tss.ebx = 0;
			task->tss.ebp = 0;
			task->tss.esi = 0;
			task->tss.edi = 0;
			task->tss.es = 0;
			task->tss.ds = 0;
			task->tss.fs = 0;
			task->tss.gs = 0;
			task->tss.ldtr = 0;
			task->tss.iomap = 0x40000000;
			return task;
		}
	}
	return 0; //全部在使用中 
}

void task_run(struct TASK *task, int level, int priority){
	if (level < 0) {
		level = task->level; //任务等级修正
	}
	if (priority > 0) {
		task->priority = priority;
	}

	if (task->flags == 2 && task->level != level) { //正在运行中且任务等级不一致，那就只能删除了
		task_remove(task); //删除任务
	}
	if (task->flags != 2) {
		//这个任务本来不在运行中，就运行，并且加入到合理的任务等级
		task->level = level;
		task_add(task);
	}

	taskctl->lv_change = 1; //下次变更等级的值
	return;
}

void task_sleep(struct TASK *task){
	struct TASK *now_task;
	if (task->flags == 2) {
		//运行中
		now_task = task_now();
		task_remove(task); //移除任务
		if (task == now_task) {
			//寻找下一个任务level
			task_switchsub();
			now_task = task_now(); //更新现行任务
			farjmp(0, now_task->sel);
		}
	}
	return;
}

void task_switch(void){
	struct TASKLEVEL *tl = &taskctl->level[taskctl->now_lv];
	struct TASK *new_task, *now_task = tl->tasks[tl->now];
	tl->now++;
	if (tl->now == tl->running) {
		tl->now = 0;
	}
	if (taskctl->lv_change != 0) {
		task_switchsub();
		tl = &taskctl->level[taskctl->now_lv];
	}
	new_task = tl->tasks[tl->now];
	//设置任务切换定时器 
	timer_settime(task_timer, new_task->priority);
	if (new_task != now_task) {
		farjmp(0, new_task->sel);
	}
	return;
}
