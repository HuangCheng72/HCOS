#include "bootpack.h"

extern struct FIFO8 keyfifo, mousefifo;//两个缓冲区 

void HariMain(void){
	//染色区域指针 ，直接指针指向我们指定显示信息存放的地址 
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO; 
	//鼠标信息结构体，以后鼠标信息就靠它来描述 
	struct MOUSE_DEC mdec;
	char s[40], mcursor[256]; //字符串和鼠标 
	int mx, my;// 鼠标当前位置 
	
	//初始化GDT和IDT 
	init_gdtidt();
	//初始化PIC 
	init_pic();
	//执行STI指令之后，中断许可标志位变成1，CPU可以接受来自外部设备的中断，它是CLI的逆指令 
	io_sti();
	//初始化缓冲区
	char keybuf[32], mousebuf[128];
	fifo8_init(&keyfifo, 32, keybuf);
	fifo8_init(&mousefifo, 128, mousebuf);
	//修改PIC的IMR，可以接受来自键盘和鼠标的中断 
	io_out8(PIC0_IMR, 0xf9); //开放PIC1和键盘中断（11111001）权限 
	io_out8(PIC1_IMR, 0xef); //开放鼠标中断(11101111) 权限 
	
	init_keyboard();//初始化键盘 
	
	//初始化调色板 
	init_palette();
	
	//以下就是绘图过程 
	init_screen(binfo->vram, binfo->scrnx, binfo->scrny);
	
	//打印鼠标并显示尖端所指向的位置坐标 
	mx = (binfo->scrnx - 16) / 2; /* 屏幕中心位置计算，鼠标的打印起点要在屏幕中心位置往左上一点 */
	my = (binfo->scrny - 28 - 16) / 2;
	init_mouse_cursor8(mcursor, COL8_008484);
	putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
	//打印坐标到字符串 
	sprintf(s, "(%d, %d)", mx, my);
	putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
	
	enable_mouse(&mdec);//使鼠标可用，把鼠标结构体指针传进去 
	
	int i;//用于存储获取缓冲区的数据
	//循环中止，防止退出 
	while(1) {
		io_cli(); //禁止中断，打印的时候不能中断 
		if (fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0) {
			io_stihlt();
		} else {
			if (fifo8_status(&keyfifo) != 0) {
				i = fifo8_get(&keyfifo);
				io_sti();
				sprintf(s, "%02X", i);
				boxfill8(binfo->vram, binfo->scrnx, COL8_008484,  0, 16, 15, 31);
				putfonts8_asc(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
			} else if (fifo8_status(&mousefifo) != 0) {
				i = fifo8_get(&mousefifo);
				io_sti();
				if (mouse_decode(&mdec, i) != 0) {//解码成功，没有中途而废的情况 
					//成功解读出三个字节的情况，那当然要输出 
					sprintf(s, "[lcr %4d %4d]", mdec.x, mdec.y);
					if ((mdec.btn & 0x01) != 0) {
						//如果鼠标左键被按下 
						s[1] = 'L';
					}
					if ((mdec.btn & 0x02) != 0) {
						//如果鼠标右键被按下 
						s[3] = 'R';
					}
					if ((mdec.btn & 0x04) != 0) {
						//如果鼠标滑轮键被按下 
						s[2] = 'C';
					}
					boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 32, 16, 32 + 15 * 8 - 1, 31);
					putfonts8_asc(binfo->vram, binfo->scrnx, 32, 16, COL8_FFFFFF, s);
					//开始移动鼠标指针
					//首先用一个矩形把这个从鼠标指针尖端开始往右往下的16*16区域给覆盖了，注意背景色
					boxfill8(binfo->vram, binfo->scrnx, COL8_008484, mx, my, mx + 15, my + 15);
					//然后计算鼠标指针位置
					//基位置+偏移量 
					mx += mdec.x;
					my += mdec.y;
					//防止越界 
					if (mx < 0) {
						mx = 0;
					}
					if (my < 0) {
						my = 0;
					}
					//是否能完整绘制 
					if (mx > binfo->scrnx - 16) {
						mx = binfo->scrnx - 16;
					}
					if (my > binfo->scrny - 16) {
						my = binfo->scrny - 16;
					}
					//可以完整绘制，重复之前的流程，绘制鼠标如下 
					putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
					//打印坐标到字符串 
					sprintf(s, "(%3d, %3d)", mx, my);
					//打印之前先把原来的坐标盖住
					boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 0, 79, 15);
					//打印新的字符串到屏幕上 
					putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
				}
			}
		}
	}
}
