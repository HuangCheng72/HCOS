//#include "int.h" 
#include "bootpack.h"

//��ʼ��PIC 
void init_pic(void){
	
	io_out8(PIC0_IMR, 0xff); //��ֹ�����ж�
	io_out8(PIC1_IMR, 0xff); //��ֹ�����ж� 
	
	io_out8(PIC0_ICW1, 0x11  ); //���ش���ģʽ��edge trigger mode�� 
	io_out8(PIC0_ICW2, 0x20  ); //IRQ0-7��INT20-27���� 
	io_out8(PIC0_ICW3, 1 << 2); //PIC1��IRQ2���� 
	io_out8(PIC0_ICW4, 0x01  ); //�޻�����ģʽ 

	io_out8(PIC1_ICW1, 0x11  ); //���ش���ģʽ 
	io_out8(PIC1_ICW2, 0x28  ); //IRQ8-15��INT28-2f���� 
	io_out8(PIC1_ICW3, 2     ); //PIC1��IRQ2���� 
	io_out8(PIC1_ICW4, 0x01  ); //�޻�����ģʽ 

	io_out8(PIC0_IMR,  0xfb  ); //11111011 PIC1����ȫ����ֹ/
	io_out8(PIC1_IMR,  0xff  ); //11111111 ��ֹ�����ж� 

	return;
}
#define PORT_KEYDAT		0x0060
//���̻�����
struct FIFO8 keyfifo; 
//����PS/2���̵��ж�
void inthandler21(int *esp){
	io_out8(PIC0_OCW2, 0x61); //֪ͨPIC��IRQ-01�����Ѿ���ɣ��ⲽ����ʡ�� 
	unsigned char data = io_in8(PORT_KEYDAT); //�������ȡ�������
	fifo8_put(&keyfifo, data); //����������
	return; 
}
//��껺���� 
struct FIFO8 mousefifo;
//����PS/2�����ж�
void inthandler2c(int *esp){
	io_out8(PIC1_OCW2, 0x64); //֪ͨPIC��IRQ-12�����Ѿ���ɣ��ⲽ����ʡ�� 
	io_out8(PIC0_OCW2, 0x62); //֪ͨPIC��IRQ-02�����Ѿ���ɣ��ⲽ����ʡ�� 
	unsigned char data = io_in8(PORT_KEYDAT); //�������ȡ�������
	fifo8_put(&mousefifo, data); //����������
	return; 
}

void inthandler27(int *esp)
/* PIC0����β���ȫ����z�ߌ��� */
/* Athlon64X2�C�ʤɤǤϥ��åץ��åȤζ��Ϥˤ��PIC�γ��ڻ��r�ˤ��θ���z�ߤ�1�Ȥ��������� */
/* ���θ���z�߄I���v���ϡ����θ���z�ߤˌ����ƺΤ⤷�ʤ��Ǥ���^���� */
/* �ʤ��Τ⤷�ʤ��Ƥ����Σ�
	��  ���θ���z�ߤ�PIC���ڻ��r��늚ݵĤʥΥ����ˤ�äưk��������ΤʤΤǡ�
		�ޤ���˺Τ��I���Ƥ���Ҫ���ʤ���									*/
{
	io_out8(PIC0_OCW2, 0x67); /* IRQ-07�ܸ����ˤ�PIC��֪ͨ(7-1����) */
	return;
}
