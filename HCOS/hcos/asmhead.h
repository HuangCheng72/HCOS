//存放BOOTINFO的地址
#define ADR_BOOTINFO	0x00000ff0

//BOOTINFO结构体，其结构看asmhead.nas中已经定下来了
//其实vram应该是一个二维数组，但是在内存中是一维数组的形式，因此书写程序的时候我们必须手动计算 
//vram就是存储整个屏幕上显示的所有色素信息！！！！！ 
struct BOOTINFO {
	char cyls, leds, vmode, reserve;
	short scrnx, scrny;
	char *vram;
}; 
