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
	task->flags = 2; /* ������ */
	return;
}

void task_remove(struct TASK *task){
	int i;
	struct TASKLEVEL *tl = &taskctl->level[task->level];

	/* task���ɤ��ˤ��뤫��̽�� */
	for (i = 0; i < tl->running; i++) {
		if (tl->tasks[i] == task) {
			/* �����ˤ��� */
			break;
		}
	}

	tl->running--;
	if (i < tl->now) {
		tl->now--; /* �����Τǡ�����⤢�碌�Ƥ��� */
	}
	if (tl->now >= tl->running) {
		/* now���������ʂ��ˤʤäƤ����顢�������� */
		tl->now = 0;
	}
	task->flags = 1; /* ����`���� */

	/* ���餷 */
	for (; i < tl->running; i++) {
		tl->tasks[i] = tl->tasks[i + 1];
	}

	return;
}

void task_switchsub(void){
	int i;
	/* һ���ϤΥ�٥��̽�� */
	for (i = 0; i < MAX_TASKLEVELS; i++) {
		if (taskctl->level[i].running > 0) {
			break; /* Ҋ�Ĥ��ä� */
		}
	}
	taskctl->now_lv = i;
	taskctl->lv_change = 0;
	return;
}

struct TASK *task_init(struct MEMMAN *memman){
	int i;
	struct TASK *task;
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
	task->flags = 2;	//���ڻ�б�־ 
	task->priority = 2; //0.02��
	task->level = 0;	//��ߵȼ� 
	task_add(task);
	task_switchsub();	/* ��٥��O�� */
	load_tr(task->sel);
	//���������л���ʱ�� 
	task_timer = timer_insert(0,0,task->priority);
	return task;
}

struct TASK *task_alloc(void){
	int i;
	struct TASK *task;
	for (i = 0; i < MAX_TASKS; i++) {
		if (taskctl->tasks0[i].flags == 0) {
			task = &taskctl->tasks0[i];
			task->flags = 1; //����ʹ���б�־ 
			task->tss.eflags = 0x00000202; // IF = 1;
			task->tss.eax = 0; //��������Ϊ0 
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
	return 0; //ȫ����ʹ���� 
}

void task_run(struct TASK *task, int level, int priority){
	if (level < 0) {
		level = task->level; /* ��٥�������ʤ� */
	}
	if (priority > 0) {
		task->priority = priority;
	}

	if (task->flags == 2 && task->level != level) { /* �����ФΥ�٥�Ή�� */
		task_remove(task); /* �����g�Ф����flags��1�ˤʤ�Τ��¤�if��g�Ф���� */
	}
	if (task->flags != 2) {
		/* ����`�פ����𤳤������� */
		task->level = level;
		task_add(task);
	}

	taskctl->lv_change = 1; /* �λإ����������å��ΤȤ��˥�٥��Ҋֱ�� */
	return;
}

void task_sleep(struct TASK *task){
	struct TASK *now_task;
	if (task->flags == 2) {
		/* �����Ф��ä��� */
		now_task = task_now();
		task_remove(task); /* �����g�Ф����flags��1�ˤʤ� */
		if (task == now_task) {
			/* �Է�����Υ���`�פ��ä��Τǡ������������å�����Ҫ */
			task_switchsub();
			now_task = task_now(); /* �O����ǤΡ����F�ڤΥ���������̤��Ƥ�餦 */
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
	//���������л���ʱ�� 
	task_timer = timer_insert(0,0,new_task->priority);
	if (new_task != now_task) {
		farjmp(0, new_task->sel);
	}
	return;
}
