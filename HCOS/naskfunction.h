//��Щ����naskfunc.nas�����ĺ���
void io_hlt(void); //ֹͣ�����ǻ���HLT
void io_cli(void); //��ֹCPU������жϣ����ǻ���CLI
void io_sti(void);
void io_stihlt(void);
int io_in8(int port);  //IO�����λ����
void io_out8(int port, int data);   //IO�����λ����
int io_load_eflags(void);   //IO����eflag
void io_store_eflags(int eflags);   //IO�洢eflag
void load_gdtr(int limit, int addr);    //����Ϣ�浽gdtr�Ĵ���
void load_idtr(int limit, int addr);    //����Ϣ�浽idtr�Ĵ���
int load_cr0(void);
void store_cr0(int cr0);
void asm_inthandler20(void);
void asm_inthandler21(void);
void asm_inthandler27(void);
void asm_inthandler2c(void);
unsigned int memtest_sub(unsigned int start, unsigned int end);
