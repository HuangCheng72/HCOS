//下面是绘制GUI界面需要用到的函数 
void init_palette(void); //初始化调色板 
void set_palette(int start, int end, unsigned char *rgb); //设置调色板颜色 
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1); //对某一区域染色也就是绘制矩形，xsize是屏幕横轴的大小 
void init_screen8(char *vram, int x, int y); //初始化所有矩形，也就是绘图过程 
void putfont8(char *vram, int xsize, int x, int y, char c, char *font); //绘制 8*16的点阵图，这里用来显示字符 
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
