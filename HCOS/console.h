//控制台信息结构体 
struct CONSOLE {
	//图层 
	struct SHEET *sht;
	//这三个分别就是之前的cursor_x，cursor_y，cursor_c 
	int cur_x, cur_y, cur_c;
	//定时器 
	struct TIMER *timer;
};

//控制台任务 
void console_task(struct SHEET *sheet, int memtotal);
//控制台新开一行 
void cons_newline(struct CONSOLE *cons);
//控制台打印字符串 
void cons_putchar(struct CONSOLE *cons, int chr, char move);
void cons_putstr0(struct CONSOLE *cons, char *s);
void cons_putstr1(struct CONSOLE *cons, char *s, int l);
void cons_runcmd(char *cmdline, struct CONSOLE *cons, int *fat, int memtotal);
//控制台命令行的命令 
void cmd_mem(struct CONSOLE *cons, int memtotal);
void cmd_cls(struct CONSOLE *cons);
void cmd_dir(struct CONSOLE *cons);
void cmd_type(struct CONSOLE *cons, int *fat, char *cmdline);
int cmd_app(struct CONSOLE *cons, int *fat, char *cmdline);
//API
int *hrb_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax);
//中断处理 
int *inthandler0d(int *esp);
int *inthandler0c(int *esp);
void hrb_api_linewin(struct SHEET *sht, int x0, int y0, int x1, int y1, int col);
