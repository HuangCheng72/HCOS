#include "bootpack.h"

struct TIMERCTL timerctl; 

void init_pit(void){
	//��һ�ε����þ��ǣ��ж�����11932���ն�Ƶ�ʺ������100Hz��һ���Ӿͻᷢ��һ�ٴ��ж� 
	io_out8(PIT_CTRL,0x34);
	io_out8(PIT_CNT0,0x9c);
	io_out8(PIT_CNT0,0x2e);
	timerctl.count = 0;//�ƴ����ݳ�ʼ�� 
	return;
} 
void inthandler20(int *esp){
	io_out8(PIC0_OCW2, 0x60); /* ��IRQ-00�źŽ������˵���Ϣ֪ͨ��PIC */
    timerctl.count++;//�յ�һ�ξ�����1
	return;
}
