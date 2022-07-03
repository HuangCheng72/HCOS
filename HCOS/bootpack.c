#include "bootpack.h"

extern struct FIFO32 keyfifo, mousefifo;//两个缓冲区
//绘制窗口
void make_window8(unsigned char *buf, int xsize, int ysize, char *title);
//整合了重新上色部分区域，打印字符串，刷新图层三个函数
void putfonts8_asc_sht(struct SHEET *sht, int x, int y, int c, int b, char *s, int l);

void HariMain(void){
	//染色区域指针 ，直接指针指向我们指定显示信息存放的地址 
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	//FIFO缓冲区公用内存空间
	int fifobuf[128];
	//缓冲区
	struct FIFO32 fifo;
	//字符串，各个缓冲区所用的数组 
	char s[40];
	//鼠标的x，y坐标，以及变量i存放从缓冲区中读取的数据 
	int mx, my, i, count = 0;
	unsigned int memtotal;//内存大小
	//鼠标信息结构体，以后鼠标信息就靠它来描述 
	struct MOUSE_DEC mdec;
	//内存管理结构体
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	//图层管理结构体 
	struct SHTCTL *shtctl;
	//三个图层的结构体 
	struct SHEET *sht_back, *sht_mouse, *sht_win;
	
	//buf_mouse就是之前的mcursor，背景，鼠标，窗口的绘制数据 
	unsigned char *buf_back, buf_mouse[256], *buf_win;
	
	//初始化GDT和IDT 
	init_gdtidt();
	//初始化PIC 
	init_pic();
	//执行STI指令之后，中断许可标志位变成1，CPU可以接受来自外部设备的中断，它是CLI的逆指令 
	io_sti();
	//初始化缓冲区 
	fifo32_init(&fifo, 128, fifobuf);
	//初始化定时器 
	init_pit(); 
	//修改PIC的IMR，可以接受来自键盘和鼠标的中断 
	io_out8(PIC0_IMR, 0xf8); //开放PIC1和键盘中断（11111000）权限 （定时器进来要开放新权限） 
	io_out8(PIC1_IMR, 0xef); //开放鼠标中断(11101111) 权限
	
	init_keyboard(&fifo, 256);//初始化键盘
	enable_mouse(&fifo,512,&mdec);//使鼠标可用，把鼠标结构体指针传进去
	memtotal = memtest(0x00400000, 0xbfffffff);
	memman_init(memman);
	memman_free(memman, 0x00001000, 0x0009e000); /* 0x00001000 - 0x0009efff */
	memman_free(memman, 0x00400000, memtotal - 0x00400000);

    //插入定时器
	timer_insert(&fifo,10,1000);
	timer_insert(&fifo,3,300);
	timer_insert(&fifo,1,50);
	
	//初始化调色板 
	init_palette();
	//以下就是绘图过程 
	shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
	sht_back  = sheet_alloc(shtctl);
	sht_mouse = sheet_alloc(shtctl);
	sht_win   = sheet_alloc(shtctl);
	buf_back  = (unsigned char *) memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
	buf_win   = (unsigned char *) memman_alloc_4k(memman, 160 * 52);
	sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1);/* 没有透明色 */
	sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99); //透明色号99 
	sheet_setbuf(sht_win, buf_win, 160, 52, -1); /* 没有透明色 */
	init_screen8(buf_back, binfo->scrnx, binfo->scrny);
	init_mouse_cursor8(buf_mouse, 99);//背景色号99 
	//绘制窗口 
	make_window8(buf_win, 160, 52, "counter");
	sheet_slide(sht_back, 0, 0);
	//打印鼠标并显示尖端所指向的位置坐标 
	mx = (binfo->scrnx - 16) / 2; //屏幕中心位置计算，鼠标的打印起点要在屏幕中心位置往左上一点
	my = (binfo->scrny - 28 - 16) / 2;
	sheet_slide(sht_mouse, mx, my);
	sheet_slide(sht_win, 80, 72);
	sheet_updown(sht_back,  0);
	sheet_updown(sht_win,   1);
	sheet_updown(sht_mouse, 2);
	//打印坐标到字符串 
	sprintf(s, "(%3d, %3d)", mx, my);
	putfonts8_asc_sht(sht_back, 0, 0, COL8_FFFFFF, COL8_008484, s, 10);
	//打印内存信息 
	sprintf(s, "memory %dMB   free : %dKB", memtotal / (1024 * 1024), memman_total(memman) / 1024);
	putfonts8_asc_sht(sht_back, 0, 32, COL8_FFFFFF, COL8_008484, s, 40);

	for (;;) {
	    count++;
		io_cli(); //禁止中断，打印的时候不能中断 
		if (fifo32_status(&fifo) == 0) {
			io_sti();
		} else {
		    i = fifo32_get(&fifo);
		    io_sti();
			if (256 <= i && i <= 511) {//键盘数据
				sprintf(s, "%02X", i - 256);
                putfonts8_asc_sht(sht_back,0,16,COL8_FFFFFF,COL8_008484,s,2);
			} else if (512 <= i && i <= 767) {//鼠标数据
				if (mouse_decode(&mdec, i - 512) != 0) {//解码成功，没有中途而废的情况
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
						//如果鼠标中键被按下 
						s[2] = 'C';
					}
					putfonts8_asc_sht(sht_back, 32, 16, COL8_FFFFFF, COL8_008484, s, 15);
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
					//越界条件 
					if (mx > binfo->scrnx - 1) {
						mx = binfo->scrnx - 1;
					}
					if (my > binfo->scrny - 1) {
						my = binfo->scrny - 1;
					}
					sprintf(s, "(%3d, %3d)", mx, my);
					putfonts8_asc_sht(sht_back, 0, 0, COL8_FFFFFF, COL8_008484, s, 10);
					sheet_slide(sht_mouse, mx, my);
				}
			}else if(i == 10) {//10秒定时器
				putfonts8_asc_sht(sht_back, 0, 64, COL8_FFFFFF, COL8_008484, "10[sec]", 7);
				sprintf(s, "%010d", timerctl.count);//输出计次器计时数据 
				putfonts8_asc_sht(sht_win, 40, 28, COL8_000000, COL8_C6C6C6, s, 10);//打印到窗口上 
			} else if (i == 3) {//3秒定时器
				putfonts8_asc_sht(sht_back, 0, 80, COL8_FFFFFF, COL8_008484, "3[sec]", 6);
				count = 0;//3秒后才开始计时 
			} else if ( i == 1 ) {//光标用定时器
			    //再次插入定时器，设置为data为0
			    timer_insert(&fifo,0,50);
			    boxfill8(buf_back, binfo->scrnx, COL8_FFFFFF, 8, 96, 15, 111);
				sheet_refresh(sht_back, 8, 96, 16, 112);
			} else if ( i == 0 ) {//光标用定时器
                //再次插入定时器，设置为data为1
                timer_insert(&fifo,1,50);
                boxfill8(buf_back, binfo->scrnx, COL8_008484, 8, 96, 15, 111);
                sheet_refresh(sht_back, 8, 96, 16, 112);
			}
		}
	}
}

void make_window8(unsigned char *buf, int xsize, int ysize, char *title){
	static char closebtn[14][16] = {
		"OOOOOOOOOOOOOOO@",
		"OQQQQQQQQQQQQQ$@",
		"OQQQQQQQQQQQQQ$@",
		"OQQQ@@QQQQ@@QQ$@",
		"OQQQQ@@QQ@@QQQ$@",
		"OQQQQQ@@@@QQQQ$@",
		"OQQQQQQ@@QQQQQ$@",
		"OQQQQQ@@@@QQQQ$@",
		"OQQQQ@@QQ@@QQQ$@",
		"OQQQ@@QQQQ@@QQ$@",
		"OQQQQQQQQQQQQQ$@",
		"OQQQQQQQQQQQQQ$@",
		"O$$$$$$$$$$$$$$@",
		"@@@@@@@@@@@@@@@@"
	};
	int x, y;
	char c;
	boxfill8(buf, xsize, COL8_C6C6C6, 0,         0,         xsize - 1, 0        );
	boxfill8(buf, xsize, COL8_FFFFFF, 1,         1,         xsize - 2, 1        );
	boxfill8(buf, xsize, COL8_C6C6C6, 0,         0,         0,         ysize - 1);
	boxfill8(buf, xsize, COL8_FFFFFF, 1,         1,         1,         ysize - 2);
	boxfill8(buf, xsize, COL8_848484, xsize - 2, 1,         xsize - 2, ysize - 2);
	boxfill8(buf, xsize, COL8_000000, xsize - 1, 0,         xsize - 1, ysize - 1);
	boxfill8(buf, xsize, COL8_C6C6C6, 2,         2,         xsize - 3, ysize - 3);
	boxfill8(buf, xsize, COL8_000084, 3,         3,         xsize - 4, 20       );
	boxfill8(buf, xsize, COL8_848484, 1,         ysize - 2, xsize - 2, ysize - 2);
	boxfill8(buf, xsize, COL8_000000, 0,         ysize - 1, xsize - 1, ysize - 1);
	putfonts8_asc(buf, xsize, 24, 4, COL8_FFFFFF, title);
	for (y = 0; y < 14; y++) {
		for (x = 0; x < 16; x++) {
			c = closebtn[y][x];
			if (c == '@') {
				c = COL8_000000;
			} else if (c == '$') {
				c = COL8_848484;
			} else if (c == 'Q') {
				c = COL8_C6C6C6;
			} else {
				c = COL8_FFFFFF;
			}
			buf[(5 + y) * xsize + (xsize - 21 + x)] = c;
		}
	}
	return;
}

//整合了以颜色覆盖部分区域，打印字符串，刷新图层三个函数
//参数分别是，图层，打印的x，y位置，c是字符串颜色，b是背景颜色，s是字符串，l是字符串长度
void putfonts8_asc_sht(struct SHEET *sht, int x, int y, int c, int b, char *s, int l){
	boxfill8(sht->buf, sht->bxsize, b, x, y, x + l * 8 - 1, y + 15);
	putfonts8_asc(sht->buf, sht->bxsize, x, y, c, s);
	sheet_refresh(sht, x, y, x + l * 8, y + 16);
	return;
}
