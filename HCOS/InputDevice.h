#define PORT_KEYDAT				0x0060
#define PORT_KEYSTA				0x0064
#define PORT_KEYCMD				0x0064
#define KEYSTA_SEND_NOTREADY	0x02
#define KEYCMD_WRITE_MODE		0x60
#define KBC_MODE				0x47
#define KEYCMD_SENDTO_MOUSE		0xd4
#define MOUSECMD_ENABLE			0xf4

//鼠标信息结构体 
struct MOUSE_DEC {
	unsigned char buf[3], phase;
	int x, y, btn;
};
//这两个原本是int.c里面的中断处理程序，但是放在这里比较合适
void inthandler21(int *esp); //键盘中断处理
void inthandler2c(int *esp); //鼠标中断处理
void enable_mouse(struct MOUSE_DEC *mdec);//设置鼠标可用，传入值是要设置鼠标的初始状态 
void init_keyboard(void);//键盘初始化 
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat); //鼠标数据解码 
