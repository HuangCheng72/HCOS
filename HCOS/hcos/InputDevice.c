#include "bootpack.h" 

//键盘缓冲区
struct FIFO32 *keyfifo;
int keydata0;
//来自PS/2键盘的中断
void inthandler21(int *esp){
	io_out8(PIC0_OCW2, 0x61); //通知PIC，IRQ-01受理已经完成，这步不可省略 
	int data = io_in8(PORT_KEYDAT); //从输入获取相关数据
	fifo32_put(keyfifo, data + keydata0); //塞到缓冲区
	return; 
}
//鼠标缓冲区 
struct FIFO32 *mousefifo;
int mousedata0;
//来自PS/2鼠标的中断
void inthandler2c(int *esp){
	io_out8(PIC1_OCW2, 0x64); //通知PIC，IRQ-12受理已经完成，这步不可省略 
	io_out8(PIC0_OCW2, 0x62); //通知PIC，IRQ-02受理已经完成，这步不可省略 
	int data = io_in8(PORT_KEYDAT); //从输入获取相关数据
	fifo32_put(mousefifo, data + mousedata0); //塞到缓冲区
	return; 
}

void wait_KBC_sendready(void){
	//等待键盘控制电路准备完毕 
	for(;;){
		if ((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0) {
			break;
		}
	}
	return;
}

void init_keyboard(struct FIFO32 *fifo, int data0){
	//初始化键盘控制电路，把缓冲区信息写入到全局变量
	keyfifo = fifo;
    keydata0 = data0;
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, KBC_MODE);
	return;
}

void enable_mouse(struct FIFO32 *fifo, int data0, struct MOUSE_DEC *mdec){
	//激活鼠标，把缓冲区信息写入到全局变量
	mousefifo = fifo;
    mousedata0 = data0;

	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
	mdec->phase = 0; //设置初始状态为0，等待0xfa 
	return; //顺利的话键盘控制会返回ACK(0xfa) 
}
//鼠标数据解码 
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat){
	if (mdec->phase == 0) {
		//等待鼠标的0xfa的阶段 
		if (dat == 0xfa) {
			mdec->phase = 1;
		}
		return 0;
	}
	if (mdec->phase == 1) {
		//等待鼠标的第一字节的阶段 
		if ((dat & 0xc8) == 0x08) {
			//如果第一字节正确 
			mdec->buf[0] = dat;
			mdec->phase = 2;
		}
		return 0;
	}
	if (mdec->phase == 2) {
		//等待鼠标的第二字节的阶段 
		mdec->buf[1] = dat;
		mdec->phase = 3;
		return 0;
	}
	if (mdec->phase == 3) {
		//等待鼠标的第三字节的阶段
		mdec->buf[2] = dat;
		mdec->phase = 1;
		mdec->btn = mdec->buf[0] & 0x07;
		mdec->x = mdec->buf[1];
		mdec->y = mdec->buf[2];
		if ((mdec->buf[0] & 0x10) != 0) {
			mdec->x |= 0xffffff00;
		}
		if ((mdec->buf[0] & 0x20) != 0) {
			mdec->y |= 0xffffff00;
		}
		mdec->y = - mdec->y; //鼠标的y轴方向与画面符号方向相反，是往下的 ，所以要取负 
		return 1; //完成解码 
	}
	return -1; //如果不出意外应该不会到这里来，到这里来说明鼠标数据可能出问题了 
}

