#include "bootpack.h" 

void HariMain(void){
	//染色区域指针 ，直接指针指向我们指定显示信息存放的地址 
	struct BOOTINFO *binfo = (struct BOOTINFO *) 0x0ff0;
	char s[40], mcursor[256]; //字符串和鼠标 
	int mx, my;// 鼠标当前位置 
	
	//初始化GDT和IDT 
	init_gdtidt();
	//初始化PIC 
	init_pic();
	//执行STI指令之后，中断许可标志位变成1，CPU可以接受来自外部设备的中断，它是CLI的逆指令 
	io_sti();
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
	
	//修改PIC的IMR，可以接受来自键盘和鼠标的中断 
	io_out8(PIC0_IMR, 0xf9); /* PIC1とキーボードを許可(11111001) */
	io_out8(PIC1_IMR, 0xef); /* マウスを許可(11101111) */
	
	
	//循环中止，防止退出 
	while(1) {
		io_hlt();
	}
}
