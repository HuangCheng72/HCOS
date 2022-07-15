#include "bootpack.h" 

struct TIMER *mt_timer;
int mt_tr;
//多任务初始化（也就是设定一个计时器进行任务切换） 
void mt_init(void){	
	//插入一个不绑定缓冲区无写入数据的计时器，只单纯用于多任务切换的计时 
	mt_timer = timer_insert(0 , 0, 2); 
	mt_tr = 3 * 8;
	return;
}
//多任务切换
void mt_taskswitch(void){
	if (mt_tr == 3 * 8) {
		mt_tr = 4 * 8;
	} else {
		mt_tr = 3 * 8;
	}
	//插入一个不绑定缓冲区无写入数据的计时器，只单纯用于多任务切换的计时 
	mt_timer = timer_insert(0 , 0, 2); 
	farjmp(0, mt_tr);
	return;
}
