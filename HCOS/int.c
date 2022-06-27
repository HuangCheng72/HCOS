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
//来自PS/2键盘的中断
void inthandler21(int *esp){
    //一旦接收到键盘输入字符，系统即刻中断，然后在屏幕上打印字符串
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	boxfill8(binfo->vram, binfo->scrnx, COL8_000000, 0, 0, 32 * 8 - 1, 15);
	putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, "INT 21 (IRQ-1) : PS/2 keyboard");
    //完成之后继续待机
	while(1) {
		io_hlt();
	}
}

//来自PS/2鼠标的中断
void inthandler2c(int *esp){
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	boxfill8(binfo->vram, binfo->scrnx, COL8_000000, 0, 0, 32 * 8 - 1, 15);
	putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, "INT 2C (IRQ-12) : PS/2 mouse");
	while(1) {
		io_hlt();
	}
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
