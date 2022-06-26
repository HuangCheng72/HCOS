#include<stdio.h> 
void io_hlt(void);
void io_cli(void);
void io_out8(int port, int data);
int io_load_eflags(void);
void io_store_eflags(int eflags);

//下面是我们这次需要用到的函数 

void init_palette(void); //初始化调色板 
void set_palette(int start, int end, unsigned char *rgb); //设置调色板颜色 
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1); //对某一区域染色也就是绘制矩形，xsize是屏幕横轴的大小 
void init_screen(char *vram, int x, int y); //初始化所有矩形，也就是绘图过程 
void putfont8(char *vram, int xsize, int x, int y, char c, char *font); //绘制 8*16的点阵图，这里用来显示字符 
void putfont16(char *vram, int xsize, int x, int y, char c, char *font); //绘制 16*16的点阵图，这里用来显示汉字 
void putfonts8_asc(char *vram, int xsize, int x, int y, char c, unsigned char *s);// 打印半角字符串 
void init_mouse_cursor8(char *mouse, char bc); //初始化鼠标图片，bc是背景颜色 
void putblock8_8(char *vram, int vxsize, int pxsize, int pysize, int px0, int py0, char *buf, int bxsize); //把buf中的数组复制到vram中去 

//下面是颜色常量 

#define COL8_000000		0
#define COL8_FF0000		1
#define COL8_00FF00		2
#define COL8_FFFF00		3
#define COL8_0000FF		4
#define COL8_FF00FF		5
#define COL8_00FFFF		6
#define COL8_FFFFFF		7
#define COL8_C6C6C6		8
#define COL8_840000		9
#define COL8_008400		10
#define COL8_848400		11
#define COL8_000084		12
#define COL8_840084		13
#define COL8_008484		14
#define COL8_848484		15


//BOOTINFO结构体，其结构看asmhead.nas中已经定下来了
//其实vram应该是一个二维数组，但是在内存中是一维数组的形式，因此书写程序的时候我们必须手动计算 
//vram就是存储整个屏幕上显示的所有色素信息！！！！！ 
struct BOOTINFO {
	char cyls, leds, vmode, reserve;
	short scrnx, scrny;
	char *vram;
};

void HariMain(void){
	//染色区域指针 ，直接指针指向我们指定显示信息存放的地址 
	struct BOOTINFO *binfo = (struct BOOTINFO *) 0x0ff0;
	char s[40], mcursor[256]; //字符串和鼠标 
	int mx, my;// 鼠标当前位置 
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
	
	//循环中断，防止退出 

	while(1) {
		io_hlt();
	}
}

void init_palette(void){
	static unsigned char table_rgb[16 * 3] = {
		0x00, 0x00, 0x00,	/*  0:黑 */
		0xff, 0x00, 0x00,	/*  1:亮红 */
		0x00, 0xff, 0x00,	/*  2:亮绿 */
		0xff, 0xff, 0x00,	/*  3:亮黄 */
		0x00, 0x00, 0xff,	/*  4:亮蓝 */
		0xff, 0x00, 0xff,	/*  5:亮紫 */
		0x00, 0xff, 0xff,	/*  6:浅亮蓝 */
		0xff, 0xff, 0xff,	/*  7:白 */
		0xc6, 0xc6, 0xc6,	/*  8:亮灰 */
		0x84, 0x00, 0x00,	/*  9:暗红 */
		0x00, 0x84, 0x00,	/* 10:暗绿 */
		0x84, 0x84, 0x00,	/* 11:暗黄 */
		0x00, 0x00, 0x84,	/* 12:暗青 */
		0x84, 0x00, 0x84,	/* 13:暗紫色 */
		0x00, 0x84, 0x84,	/* 14:浅暗蓝 */
		0x84, 0x84, 0x84	/* 15:暗灰 */
	};
	set_palette(0, 15, table_rgb);
	return;

	/* C语言中的static char只能用于数据，相当于汇编中的DB */
}

void set_palette(int start, int end, unsigned char *rgb){
	int i, eflags;
	eflags = io_load_eflags();	/* 记录中断许可标志的值 */
	io_cli(); 					/* 将许可标志设置为0，也就是禁止中断 */
	io_out8(0x03c8, start);
	for (i = start; i <= end; i++) {
		io_out8(0x03c9, rgb[0] / 4);
		io_out8(0x03c9, rgb[1] / 4);
		io_out8(0x03c9, rgb[2] / 4);
		rgb += 3;
	}
	io_store_eflags(eflags);	/* 恢复中断许可标志的值 */
	return;
}

void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1){
	int x, y;
	for (y = y0; y <= y1; y++) {
		for (x = x0; x <= x1; x++){
			vram[y * xsize + x] = c;
		}
	}
	return;
}

void init_screen(char *vram, int x, int y){
	boxfill8(vram, x, COL8_008484,  0,     0,      x -  1, y - 29);
	boxfill8(vram, x, COL8_C6C6C6,  0,     y - 28, x -  1, y - 28);
	boxfill8(vram, x, COL8_FFFFFF,  0,     y - 27, x -  1, y - 27);
	boxfill8(vram, x, COL8_C6C6C6,  0,     y - 26, x -  1, y -  1);

	boxfill8(vram, x, COL8_FFFFFF,  3,     y - 24, 59,     y - 24);
	boxfill8(vram, x, COL8_FFFFFF,  2,     y - 24,  2,     y -  4);
	boxfill8(vram, x, COL8_848484,  3,     y -  4, 59,     y -  4);
	boxfill8(vram, x, COL8_848484, 59,     y - 23, 59,     y -  5);
	boxfill8(vram, x, COL8_000000,  2,     y -  3, 59,     y -  3);
	boxfill8(vram, x, COL8_000000, 60,     y - 24, 60,     y -  3);

	boxfill8(vram, x, COL8_848484, x - 47, y - 24, x -  4, y - 24);
	boxfill8(vram, x, COL8_848484, x - 47, y - 23, x - 47, y -  4);
	boxfill8(vram, x, COL8_FFFFFF, x - 47, y -  3, x -  4, y -  3);
	boxfill8(vram, x, COL8_FFFFFF, x -  3, y - 24, x -  3, y -  3);
	return;
}

void putfont8(char *vram, int xsize, int x, int y, char c, char *font){
	int i;
	char *p, d /* data */;
	for (i = 0; i < 16; i++) {
		p = vram + (y + i) * xsize + x;
		d = font[i];
		if ((d & 0x80) != 0) { p[0] = c; }
		if ((d & 0x40) != 0) { p[1] = c; }
		if ((d & 0x20) != 0) { p[2] = c; }
		if ((d & 0x10) != 0) { p[3] = c; }
		if ((d & 0x08) != 0) { p[4] = c; }
		if ((d & 0x04) != 0) { p[5] = c; }
		if ((d & 0x02) != 0) { p[6] = c; }
		if ((d & 0x01) != 0) { p[7] = c; }
	}
	return;
}
//绘制16*16字符的函数，由绘制8*16字符的函数修改而来 
void putfont16(char *vram, int xsize, int x, int y, char c, char *font){
	int i;
	char *p, d /* data */;
	for (i = 0; i < 32; i++) {
		//主要变动在这里，内存地址的计算公式，这在书后面有提到，日语全角字符的显示那里。 
		p = vram + (y + i / 2) * xsize + x + 8*(i % 2);
		d = font[i];
		if ((d & 0x80) != 0) { p[0] = c; }
		if ((d & 0x40) != 0) { p[1] = c; }
		if ((d & 0x20) != 0) { p[2] = c; }
		if ((d & 0x10) != 0) { p[3] = c; }
		if ((d & 0x08) != 0) { p[4] = c; }
		if ((d & 0x04) != 0) { p[5] = c; }
		if ((d & 0x02) != 0) { p[6] = c; }
		if ((d & 0x01) != 0) { p[7] = c; }
	}
	return;
}
void putfonts8_asc(char *vram, int xsize, int x, int y, char c, unsigned char *s){
    //hankaku是作者提供的256个字符的点阵字库文件
	extern char hankaku[4096];
	for (; *s != 0x00; s++) {
		putfont8(vram, xsize, x, y, c, hankaku + *s * 16);
		x += 8;
	}
	return;
}
void init_mouse_cursor8(char *mouse, char bc){
     /* 打印鼠标图标（16x16） */
	static char cursor[16][16] = {
         "*...............",
         "**..............",
         "*O*.............",
         "*OO*............",
         "*OOO*...........",
         "*OOOO*..........",
         "*OOOOO*.........",
         "*OOOOOO*........",
         "*OOOOOOO*.......",
         "*OOOO*****......",
         "*OO*O*..........",
         "*O*.*O*.........",
         "**..*O*.........",
         "*....*O*........",
         ".....*O*........",
         "......*........."
    };
	int x, y;
 
	for (y = 0; y < 16; y++) {
		for (x = 0; x < 16; x++) {
			if (cursor[y][x] == '*') {
                 //*所在就打印黑色，作为边框
				mouse[y * 16 + x] = COL8_000000;
			}
			if (cursor[y][x] == 'O') {
                 //0所在就打印白色
				mouse[y * 16 + x] = COL8_FFFFFF;
			}
			if (cursor[y][x] == '.') {
                 //背景颜色是什么就打印什么
				mouse[y * 16 + x] = bc;
			}
		}
	}
	return;
}
void putblock8_8(char *vram, int vxsize, int pxsize, int pysize, int px0, int py0, char *buf, int bxsize){
	//vxsize就是binfo->scrnx，y同理，pxsize和pysize则是想要显示的图片的大小
	//这个的意思就是把一个图片直接打印到vram上去 
	int x, y;
	for (y = 0; y < pysize; y++) {
		for (x = 0; x < pxsize; x++) {
			vram[(py0 + y) * vxsize + (px0 + x)] = buf[y * bxsize + x];
		}
	}
	return;
}
