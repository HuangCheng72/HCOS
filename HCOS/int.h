//������PIC�����ܽŵĵ�ַ
//PIC0Ϊ��PIC��PIC1Ϊ��PIC 
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
//naskfunc.nas�и����ĺ���
#include "naskfunction.h"

//������괦���˴��漰��ͼ�λ�����Ҫ����graphic
#include "graphic.h"
 
void init_pic(void); //��ʼ��PIC
//�����������жϴ������ 
void inthandler21(int *esp);
void inthandler2c(int *esp);
void inthandler27(int *esp);
