#include "bootpack.h"

int keywin_off(struct SHEET *key_win, struct SHEET *sht_win, int cur_c, int cur_x);
int keywin_on(struct SHEET *key_win, struct SHEET *sht_win, int cur_c);

void HariMain(void){
	//染色区域指针 ，直接指针指向我们指定显示信息存放的地址 
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	//FIFO缓冲区公用内存空间
	int fifobuf[128], keycmd_buf[32];
	//缓冲区
	struct FIFO32 fifo, keycmd;
	//字符串，各个缓冲区所用的数组 
	char s[40];
	//鼠标的x，y坐标，以及变量i存放从缓冲区中读取的数据 ，追记添加位置的光标，以及光标颜色
	int mx, my, i, cursor_x, cursor_c;
	unsigned int memtotal;//内存大小
	//鼠标信息结构体，以后鼠标信息就靠它来描述 
	struct MOUSE_DEC mdec;
	//内存管理结构体
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	//图层管理结构体 
	struct SHTCTL *shtctl;
	//几个图层的结构体 
	struct SHEET *sht_back, *sht_mouse, *sht_win, *sht_cons;
	//任务的指针 
	struct TASK *task_a, *task_cons;
	//定时器指针 
	struct TIMER *timer;
	//buf_mouse就是之前的mcursor，背景，鼠标，窗口的绘制数据 
	unsigned char *buf_back, buf_mouse[256], *buf_win, *buf_cons;
	//十六进制的键盘代码 
	static char keytable0[0x80] = {
		0,   0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '^', 0,   0,
		'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '@', '[', 0,   0,   'A', 'S',
		'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', ':', 0,   0,   ']', 'Z', 'X', 'C', 'V',
		'B', 'N', 'M', ',', '.', '/', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
		'2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0x5c, 0,  0,   0,   0,   0,   0,   0,   0,   0,   0x5c, 0,  0
	};
	static char keytable1[0x80] = {
		0,   0,   '!', 0x22, '#', '$', '%', '&', 0x27, '(', ')', '~', '=', '~', 0,   0,
		'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '`', '{', 0,   0,   'A', 'S',
		'D', 'F', 'G', 'H', 'J', 'K', 'L', '+', '*', 0,   0,   '}', 'Z', 'X', 'C', 'V',
		'B', 'N', 'M', '<', '>', '?', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
		'2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   '_', 0,   0,   0,   0,   0,   0,   0,   0,   0,   '|', 0,   0
	};
	int key_shift = 0, key_leds = (binfo->leds >> 4) & 7, keycmd_wait = -1;
	//命令行窗口结构体指针 
	struct CONSOLE *cons;
    //记录鼠标点击到的信息（点到哪个图层等）
    int j, x, y, mmx = -1, mmy = -1;
	struct SHEET *sht = 0, *key_win;
	
	//初始化GDT和IDT 
	init_gdtidt();
	//初始化PIC 
	init_pic();
	//执行STI指令之后，中断许可标志位变成1，CPU可以接受来自外部设备的中断，它是CLI的逆指令 
	io_sti();
	//初始化缓冲区 
	fifo32_init(&fifo, 128, fifobuf,0);
	//初始化定时器 
	init_pit();
	//修改PIC的IMR，可以接受来自键盘和鼠标的中断 
	io_out8(PIC0_IMR, 0xf8); //开放PIC1和键盘中断（11111000）权限 （定时器进来要开放新权限） 
	io_out8(PIC1_IMR, 0xef); //开放鼠标中断(11101111) 权限
	
	init_keyboard(&fifo, 256);//初始化键盘
	enable_mouse(&fifo,512,&mdec);//使鼠标可用，把鼠标结构体指针传进去
	
	fifo32_init(&keycmd, 32, keycmd_buf, 0);
	
    //初始化内存管理 
	memtotal = memtest(0x00400000, 0xbfffffff);
	memman_init(memman);
	memman_free(memman, 0x00001000, 0x0009e000); /* 0x00001000 - 0x0009efff */
	memman_free(memman, 0x00400000, memtotal - 0x00400000);
	
	//初始化调色板 
	init_palette();
	//以下就是绘图过程 
	shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
	//初始化任务
	task_a = task_init(memman);
	fifo.task = task_a;
	task_run(task_a, 1, 2);
	*((int *) 0x0fe4) = (int) shtctl;
	//初始化背景
	sht_back  = sheet_alloc(shtctl);
	buf_back  = (unsigned char *) memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
	sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1); //没有透明色
	init_screen8(buf_back, binfo->scrnx, binfo->scrny); //初始化
	
	//控制台图层 
	sht_cons = sheet_alloc(shtctl);
	buf_cons = (unsigned char *) memman_alloc_4k(memman, 256 * 165);
	sheet_setbuf(sht_cons, buf_cons, 256, 165, -1); //没有透明色 
	make_window8(buf_cons, 256, 165, "console", 0);
	make_textbox8(sht_cons, 8, 28, 240, 128, COL8_000000);
	task_cons = task_alloc();
	task_cons->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024 - 12;
	task_cons->tss.eip = (int) &console_task;
	task_cons->tss.es = 1 * 8;
	task_cons->tss.cs = 2 * 8;
	task_cons->tss.ss = 1 * 8;
	task_cons->tss.ds = 1 * 8;
	task_cons->tss.fs = 1 * 8;
	task_cons->tss.gs = 1 * 8;
	*((int *) (task_cons->tss.esp + 4)) = (int) sht_cons;
	*((int *) (task_cons->tss.esp + 8)) = memtotal;
	task_run(task_cons, 2, 2); // level=2, priority=2
	 
	//任务窗口A图层和绘制 
	sht_win   = sheet_alloc(shtctl);
	buf_win   = (unsigned char *) memman_alloc_4k(memman, 160 * 52);
	sheet_setbuf(sht_win, buf_win, 144, 52, -1); // 没有透明色
	make_window8(buf_win, 144, 52, "task_a",1);
	make_textbox8(sht_win, 8, 28, 128, 16, COL8_FFFFFF); //绘制文本框
	cursor_x = 8;
	cursor_c = COL8_FFFFFF;
	//申请、设置定时器，用于光标闪烁 
	timer = timer_alloc();
	timer_init(timer, &fifo, 1);
	timer_settime(timer, 50);
	
	//鼠标图层和绘制 
	sht_mouse = sheet_alloc(shtctl);
	sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99); //透明色号99 
	init_mouse_cursor8(buf_mouse, 99);//背景色号99 
	//打印鼠标并显示尖端所指向的位置坐标 
	mx = (binfo->scrnx - 16) / 2; //屏幕中心位置计算，鼠标的打印起点要在屏幕中心位置往左上一点
	my = (binfo->scrny - 28 - 16) / 2;
	//各种图层刷新相关 
	sheet_slide(sht_back,  0,  0);
	sheet_slide(sht_cons, 32,  4);
	sheet_slide(sht_win,  64, 56);
	sheet_slide(sht_mouse, mx, my);
	sheet_updown(sht_back,  0);
	sheet_updown(sht_cons,  1);
	sheet_updown(sht_win,   2);
	sheet_updown(sht_mouse, 3);
    key_win = sht_win;
	sht_cons->task = task_cons;
	sht_cons->flags |= 0x20;	//有光标
    
	//为了避免和键盘当前状态冲突，在一开始先进行设置 
	fifo32_put(&keycmd, KEYCMD_LED);
	fifo32_put(&keycmd, key_leds);
	
	
	for (;;) {
		if (fifo32_status(&keycmd) > 0 && keycmd_wait < 0) {
			//若存在向键盘控制器发送的数据就发送它 
			keycmd_wait = fifo32_get(&keycmd);
			wait_KBC_sendready();
			io_out8(PORT_KEYDAT, keycmd_wait);
		}
		io_cli(); //禁止中断
		if (fifo32_status(&fifo) == 0) {
			task_sleep(task_a);
			io_sti();
		} else {
			i = fifo32_get(&fifo);
			io_sti();
            if (key_win->flags == 0) {	//输入窗口被关闭
				key_win = shtctl->sheets[shtctl->top - 1];
				cursor_c = keywin_on(key_win, sht_win, cursor_c);
			}
			if (256 <= i && i <= 511) { //键盘数据 
				if (i < 0x80 + 256) { //将按键编码转化为字符编码 
					if (key_shift == 0) {
						s[0] = keytable0[i - 256];
					} else {
						s[0] = keytable1[i - 256];
					}
				} else {
					s[0] = 0;
				}
				if ('A' <= s[0] && s[0] <= 'Z') {	//输入字母为英文字母的时候 
					if (((key_leds & 4) == 0 && key_shift == 0) ||
							((key_leds & 4) != 0 && key_shift != 0)) {
						s[0] += 0x20;	//切换大小写 
					}
				}
				if (s[0] != 0) { //普通字符
                    if (key_win == sht_win) {	//发送给任务A
						if (cursor_x < 128) {
							//用字符填充光标位置，光标位置后移一位 
							s[1] = 0;
							putfonts8_asc_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, s, 1);
							cursor_x += 8;
						}
					} else {	//发送给命令行窗口 
						fifo32_put(&key_win->task->fifo, s[0] + 256);
					}
				}
				if (i == 256 + 0x0e) {	//退格键 
					if (key_win == sht_win) {//发送给任务A 
						if (cursor_x > 8) {
							//用空白擦除光标后把光标前移一位 
							putfonts8_asc_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, " ", 1);
							cursor_x -= 8;
						}
					} else {	//发送给命令行窗口 
						fifo32_put(&key_win->task->fifo, 8 + 256);
					}
				}
				if (i == 256 + 0x1c) {	//回车键 
					if (key_win != sht_win) {	//发送到命令行窗口 
						fifo32_put(&key_win->task->fifo, 10 + 256);
					}
				}
				if (i == 256 + 0x0f) {	//tab键 
                    //修改了这里
					cursor_c = keywin_off(key_win, sht_win, cursor_c, cursor_x);
					j = key_win->height - 1;
					if (j == 0) {
						j = shtctl->top - 1;
					}
					key_win = shtctl->sheets[j];
					cursor_c = keywin_on(key_win, sht_win, cursor_c);
				}
				if (i == 256 + 0x2a) {	//左shift打开 
					key_shift |= 1;
				}
				if (i == 256 + 0x36) {	//右shift打开 
					key_shift |= 2;
				}
				if (i == 256 + 0xaa) {	//左shift关闭 
					key_shift &= ~1;
				}
				if (i == 256 + 0xb6) {	//右shift关闭 
					key_shift &= ~2;
				}
				if (i == 256 + 0x3a) {	//CapsLock
					key_leds ^= 4;
					fifo32_put(&keycmd, KEYCMD_LED);
					fifo32_put(&keycmd, key_leds);
				}
				if (i == 256 + 0x45) {	//NumLock
					key_leds ^= 2;
					fifo32_put(&keycmd, KEYCMD_LED);
					fifo32_put(&keycmd, key_leds);
				}
				if (i == 256 + 0x46) {	//ScrollLock
					key_leds ^= 1;
					fifo32_put(&keycmd, KEYCMD_LED);
					fifo32_put(&keycmd, key_leds);
				}
				if (i == 256 + 0x3b && key_shift != 0 && task_cons->tss.ss0 != 0) {	//Shift+F1
					cons = (struct CONSOLE *) *((int *) 0x0fec);
					cons_putstr0(cons, "\nBreak(key) :\n");
					io_cli();	//不能在改变寄存器值的时候切换到其他任务 
					task_cons->tss.eax = (int) &(task_cons->tss.esp0);
					task_cons->tss.eip = (int) asm_end_app;
					io_sti();
				}
				if (i == 256 + 0xfa) {	//键盘成功接收到数据 
					keycmd_wait = -1;
				}
				if (i == 256 + 0xfe) {	//键盘没有成功接收到数据 
					wait_KBC_sendready();
					io_out8(PORT_KEYDAT, keycmd_wait);
				}
				//光标重新显示
				if (cursor_c >= 0) {
					boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
				}
				sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
			} else if (512 <= i && i <= 767) { //鼠标数据
				if (mouse_decode(&mdec, i - 512) != 0) {//解码成功，没有中途而废的情况
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
					sheet_slide(sht_mouse, mx, my);
					if ((mdec.btn & 0x01) != 0) {
						//按下左键
						if (mmx < 0) {
							//如果出于通常模式
							//按照才能过上到下的顺序寻找鼠标所指向的图层
							for (j = shtctl->top - 1; j > 0; j--) {
								sht = shtctl->sheets[j];
								x = mx - sht->vx0;
								y = my - sht->vy0;
								if (0 <= x && x < sht->bxsize && 0 <= y && y < sht->bysize) {
									if (sht->buf[y * sht->bxsize + x] != sht->col_inv) {
										sheet_updown(sht, shtctl->top - 1);
										if (sht != key_win) {
											cursor_c = keywin_off(key_win, sht_win, cursor_c, cursor_x);
											key_win = sht;
											cursor_c = keywin_on(key_win, sht_win, cursor_c);
										}
										if (3 <= x && x < sht->bxsize - 3 && 3 <= y && y < 21) {
											mmx = mx;	//进入窗口移动模式
											mmy = my;
										}
										if (sht->bxsize - 21 <= x && x < sht->bxsize - 5 && 5 <= y && y < 19) {
											//点击“X”按钮
											if ((sht->flags & 0x10) != 0) {		//该窗口是否为应用程序窗口
												cons = (struct CONSOLE *) *((int *) 0x0fec);
												cons_putstr0(cons, "\nBreak(mouse) :\n");
												io_cli();	//强制结束处理中禁止切换任务
												task_cons->tss.eax = (int) &(task_cons->tss.esp0);
												task_cons->tss.eip = (int) asm_end_app;
												io_sti();
											}
										}
										break;
									}
								}
							}
						} else {
							//如果出于窗口移动模式
							x = mx - mmx;	//计算鼠标的移动距离
							y = my - mmy;
							sheet_slide(sht, sht->vx0 + x, sht->vy0 + y);
							mmx = mx;	//更新为移动后的坐标
							mmy = my;
						}
					} else {
						//没有按下左键
						mmx = -1;	//返回通常模式
					}
				}
			} else if (i <= 1) { //光标用数据
				if (i != 0) {
                    timer_init(timer, &fifo, 0); //插入新的定时器，数据改为0 
					if (cursor_c >= 0) {
						cursor_c = COL8_000000;
					}
				} else {
					timer_init(timer, &fifo, 1); //插入新的定时器，数据改为1 
					if (cursor_c >= 0) {
						cursor_c = COL8_FFFFFF;
					}
				}
				timer_settime(timer, 50); //设置时间为50 
				if (cursor_c >= 0) {
					//覆盖光标位置 
					boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
					//刷新图层，才有一闪一闪的效果 
					sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
				}
			}
		}
	}
}

int keywin_off(struct SHEET *key_win, struct SHEET *sht_win, int cur_c, int cur_x){
	change_wtitle8(key_win, 0);
	if (key_win == sht_win) {
		cur_c = -1; //删除光标
		boxfill8(sht_win->buf, sht_win->bxsize, COL8_FFFFFF, cur_x, 28, cur_x + 7, 43);
	} else {
		if ((key_win->flags & 0x20) != 0) {
			fifo32_put(&key_win->task->fifo, 3); //命令行窗口光标关闭
		}
	}
	return cur_c;
}

int keywin_on(struct SHEET *key_win, struct SHEET *sht_win, int cur_c){
	change_wtitle8(key_win, 1);
	if (key_win == sht_win) {
		cur_c = COL8_000000; //显示光标
	} else {
		if ((key_win->flags & 0x20) != 0) {
			fifo32_put(&key_win->task->fifo, 2); //命令行窗口光标打开
		}
	}
	return cur_c;
}

