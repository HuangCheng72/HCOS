#include "bootpack.h"

struct TIMERCTL timerctl; 

void init_pit(void){
	//这一段的作用就是，中断周期11932，终端频率好像就是100Hz，一秒钟就会发生一百次中断 
	io_out8(PIT_CTRL,0x34);
	io_out8(PIT_CNT0,0x9c);
	io_out8(PIT_CNT0,0x2e);
	timerctl.count = 0;//计次数据初始化 
	return;
} 
void inthandler20(int *esp){
	io_out8(PIC0_OCW2, 0x60); /* 把IRQ-00信号接收完了的信息通知给PIC */
    timerctl.count++;//收到一次就自增1
	return;
}
