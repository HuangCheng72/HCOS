//这些都是naskfunc.nas给出的函数
void io_hlt(void); //停止，就是汇编的HLT
void io_cli(void); //禁止CPU级别的中断，就是汇编的CLI
void io_sti(void);
void io_stihlt(void);
int io_in8(int port);  //IO输入八位数据
void io_out8(int port, int data);   //IO输出八位数据
int io_load_eflags(void);   //IO载入eflag
void io_store_eflags(int eflags);   //IO存储eflag
void load_gdtr(int limit, int addr);    //把信息存到gdtr寄存器
void load_idtr(int limit, int addr);    //把信息存到idtr寄存器
int load_cr0(void);
void store_cr0(int cr0);
void load_tr(int tr);//载入多任务信息 
void asm_inthandler0c(void);
void asm_inthandler0d(void);
void asm_inthandler20(void);
void asm_inthandler21(void);
void asm_inthandler2c(void);
unsigned int memtest_sub(unsigned int start, unsigned int end);
void farjmp(int eip, int cs);//这个的详细解释在书P287页 
void farcall(int eip, int cs);
void asm_hrb_api(void);
void start_app(int eip, int cs, int esp, int ds, int *tss_esp0);
void asm_end_app(void);
