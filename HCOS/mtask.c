#include "bootpack.h" 

struct TIMER *mt_timer;
int mt_tr;
//�������ʼ����Ҳ�����趨һ����ʱ�����������л��� 
void mt_init(void){	
	//����һ�����󶨻�������д�����ݵļ�ʱ����ֻ�������ڶ������л��ļ�ʱ 
	mt_timer = timer_insert(0 , 0, 2); 
	mt_tr = 3 * 8;
	return;
}
//�������л�
void mt_taskswitch(void){
	if (mt_tr == 3 * 8) {
		mt_tr = 4 * 8;
	} else {
		mt_tr = 3 * 8;
	}
	//����һ�����󶨻�������д�����ݵļ�ʱ����ֻ�������ڶ������л��ļ�ʱ 
	mt_timer = timer_insert(0 , 0, 2); 
	farjmp(0, mt_tr);
	return;
}
