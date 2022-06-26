#include<stdio.h> 
#include "graphic.h"
void io_hlt(void);
void io_cli(void);
void io_out8(int port, int data);
int io_load_eflags(void);
void io_store_eflags(int eflags);

//下面是我们这次需要用到的函数 

void init_palette(void); //初始化调色板 
void set_palette(int start, int end, unsigned char *rgb); //设置调色板颜色 
