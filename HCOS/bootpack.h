#include<stdio.h> 
#include "graphic.h"
void io_hlt(void);
void io_cli(void);
void io_out8(int port, int data);
int io_load_eflags(void);
void io_store_eflags(int eflags);

//���������������Ҫ�õ��ĺ��� 

void init_palette(void); //��ʼ����ɫ�� 
void set_palette(int start, int end, unsigned char *rgb); //���õ�ɫ����ɫ 
