//#include "int.h" 
#include "bootpack.h"

//初始化PIC 
void init_pic(void){
	
	io_out8(PIC0_IMR, 0xff); //禁止所有中断
	io_out8(PIC1_IMR, 0xff); //禁止所有中断 
	
	io_out8(PIC0_ICW1, 0x11  ); //边沿触发模式（edge trigger mode） 
	io_out8(PIC0_ICW2, 0x20  ); //IRQ0-7由INT20-27接收 
	io_out8(PIC0_ICW3, 1 << 2); //PIC1由IRQ2连接 
	io_out8(PIC0_ICW4, 0x01  ); //无缓冲区模式 

	io_out8(PIC1_ICW1, 0x11  ); //边沿触发模式 
	io_out8(PIC1_ICW2, 0x28  ); //IRQ8-15由INT28-2f接收 
	io_out8(PIC1_ICW3, 2     ); //PIC1由IRQ2连接 
	io_out8(PIC1_ICW4, 0x01  ); //无缓冲区模式 

	io_out8(PIC0_IMR,  0xfb  ); //11111011 PIC1以外全部禁止/
	io_out8(PIC1_IMR,  0xff  ); //11111111 禁止所有中断 

	return;
}
#define PORT_KEYDAT		0x0060
//键盘缓冲区
struct FIFO8 keyfifo; 
//来自PS/2键盘的中断
void inthandler21(int *esp){
	io_out8(PIC0_OCW2, 0x61); //通知PIC，IRQ-01受理已经完成，这步不可省略 
	unsigned char data = io_in8(PORT_KEYDAT); //从输入获取相关数据
	fifo8_put(&keyfifo, data); //塞到缓冲区
	return; 
}
//鼠标缓冲区 
struct FIFO8 mousefifo;
//来自PS/2鼠标的中断
void inthandler2c(int *esp){
	io_out8(PIC1_OCW2, 0x64); //通知PIC，IRQ-12受理已经完成，这步不可省略 
	io_out8(PIC0_OCW2, 0x62); //通知PIC，IRQ-02受理已经完成，这步不可省略 
	unsigned char data = io_in8(PORT_KEYDAT); //从输入获取相关数据
	fifo8_put(&mousefifo, data); //塞到缓冲区
	return; 
}

void inthandler27(int *esp)
/* PIC0からの不完全割り込み対策 */
/* Athlon64X2機などではチップセットの都合によりPICの初期化時にこの割り込みが1度だけおこる */
/* この割り込み処理関数は、その割り込みに対して何もしないでやり過ごす */
/* なぜ何もしなくていいの？
	→  この割り込みはPIC初期化時の電気的なノイズによって発生したものなので、
		まじめに何か処理してやる必要がない。									*/
{
	io_out8(PIC0_OCW2, 0x67); /* IRQ-07受付完了をPICに通知(7-1参照) */
	return;
}
