//下面是PIC各个管脚的地址
//PIC0为主PIC，PIC1为从PIC 
#define PIC0_ICW1		0x0020
#define PIC0_OCW2		0x0020
#define PIC0_IMR		0x0021
#define PIC0_ICW2		0x0021
#define PIC0_ICW3		0x0021
#define PIC0_ICW4		0x0021
#define PIC1_ICW1		0x00a0
#define PIC1_OCW2		0x00a0
#define PIC1_IMR		0x00a1
#define PIC1_ICW2		0x00a1
#define PIC1_ICW3		0x00a1
#define PIC1_ICW4		0x00a1
//naskfunc.nas中给出的函数
#include "naskfunction.h"

//键盘鼠标处理，此处涉及到图形化操作要导入graphic
#include "graphic.h"
 
void init_pic(void); //初始化PIC
//以下三个是中断处理程序 
void inthandler21(int *esp);
void inthandler2c(int *esp);
void inthandler27(int *esp);
