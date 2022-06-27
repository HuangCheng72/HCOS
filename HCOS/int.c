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
//����PS/2���̵��ж�
void inthandler21(int *esp){
    //һ�����յ����������ַ���ϵͳ�����жϣ�Ȼ������Ļ�ϴ�ӡ�ַ���
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	boxfill8(binfo->vram, binfo->scrnx, COL8_000000, 0, 0, 32 * 8 - 1, 15);
	putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, "INT 21 (IRQ-1) : PS/2 keyboard");
    //���֮���������
	while(1) {
		io_hlt();
	}
}

//����PS/2�����ж�
void inthandler2c(int *esp){
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	boxfill8(binfo->vram, binfo->scrnx, COL8_000000, 0, 0, 32 * 8 - 1, 15);
	putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, "INT 2C (IRQ-12) : PS/2 mouse");
	while(1) {
		io_hlt();
	}
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
