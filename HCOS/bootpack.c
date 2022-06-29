#include "bootpack.h"

extern struct FIFO8 keyfifo, mousefifo;//两个缓冲区 

void HariMain(void){
	//染色区域指针 ，直接指针指向我们指定显示信息存放的地址 
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO; 
	//鼠标信息结构体，以后鼠标信息就靠它来描述 
	struct MOUSE_DEC mdec;
	//内存管理结构体
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR; 
	//图层管理结构体 
	struct SHTCTL *shtctl;
	struct SHEET *sht_back, *sht_mouse;
	
	char s[40],keybuf[32], mousebuf[128]; //字符串和键盘鼠标缓冲区
	//buf_mouse就是之前的mcursor
	unsigned char *buf_back, buf_mouse[256];
	int mx, my, i;// 鼠标当前位置，用于存储获取缓冲区的数据的变量 
	
	//初始化GDT和IDT 
	init_gdtidt();
	//初始化PIC 
	init_pic();
	//执行STI指令之后，中断许可标志位变成1，CPU可以接受来自外部设备的中断，它是CLI的逆指令 
	io_sti();
	//初始化缓冲区 
	fifo8_init(&keyfifo, 32, keybuf);
	fifo8_init(&mousefifo, 128, mousebuf);
	//修改PIC的IMR，可以接受来自键盘和鼠标的中断 
	io_out8(PIC0_IMR, 0xf9); //开放PIC1和键盘中断（11111001）权限 
	io_out8(PIC1_IMR, 0xef); //开放鼠标中断(11101111) 权限 
	
	init_keyboard();//初始化键盘 
	enable_mouse(&mdec);//使鼠标可用，把鼠标结构体指针传进去 
	
	unsigned int memtotal = memtest(0x00400000, 0xbfffffff);
	memman_init(memman);
	memman_free(memman, 0x00001000, 0x0009e000); /* 0x00001000 - 0x0009efff */
	memman_free(memman, 0x00400000, memtotal - 0x00400000);
	
	
	//初始化调色板 
	init_palette();
	//以下就是绘图过程 
	shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
	sht_back  = sheet_alloc(shtctl);
	sht_mouse = sheet_alloc(shtctl);
	buf_back  = (unsigned char *) memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
	sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1); /* 没有透明色 */
	sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);//透明色号99 
	init_screen8(buf_back, binfo->scrnx, binfo->scrny);
	init_mouse_cursor8(buf_mouse, 99);//背景色号99 
	sheet_slide(shtctl, sht_back, 0, 0);
	//打印鼠标并显示尖端所指向的位置坐标 
	mx = (binfo->scrnx - 16) / 2; /* 屏幕中心位置计算，鼠标的打印起点要在屏幕中心位置往左上一点 */
	my = (binfo->scrny - 28 - 16) / 2;
	sheet_slide(shtctl, sht_mouse, mx, my);
	sheet_updown(shtctl, sht_back,  0);
	sheet_updown(shtctl, sht_mouse, 1);
	//打印坐标到字符串 
	sprintf(s, "(%d, %d)", mx, my);
	putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
	
	sprintf(s, "memory %dMB   free : %dKB",
			memtotal / (1024 * 1024), memman_total(memman) / 1024);
	putfonts8_asc(binfo->vram, binfo->scrnx, 0, 32, COL8_FFFFFF, s);
	sheet_refresh(shtctl, sht_back, 0, 0, binfo->scrnx, 48);
	
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
				sheet_refresh(shtctl, sht_back, 0, 16, 16, 32);
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
					boxfill8(buf_back, binfo->scrnx, COL8_008484, 32, 16, 32 + 15 * 8 - 1, 31);
					putfonts8_asc(buf_back, binfo->scrnx, 32, 16, COL8_FFFFFF, s);
					sheet_refresh(shtctl, sht_back, 32, 16, 32 + 15 * 8, 32);
					//开始移动鼠标指针
					//计算鼠标指针位置
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
					//可以完整绘制，重复之前的流程，打印坐标到字符串 
					sprintf(s, "(%3d, %3d)", mx, my);
					boxfill8(buf_back, binfo->scrnx, COL8_008484, 0, 0, 79, 15); //打印之前先把原来的坐标盖住
					putfonts8_asc(buf_back, binfo->scrnx, 0, 0, COL8_FFFFFF, s); //打印新的字符串到屏幕上 
					sheet_refresh(shtctl, sht_back, 0, 0, 80, 16);
					sheet_slide(shtctl, sht_mouse, mx, my);
				}
			}
		}
	}
}
