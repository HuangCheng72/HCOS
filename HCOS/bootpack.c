#include "bootpack.h" 

void HariMain(void){
	//Ⱦɫ����ָ�� ��ֱ��ָ��ָ������ָ����ʾ��Ϣ��ŵĵ�ַ 
	struct BOOTINFO *binfo = (struct BOOTINFO *) 0x0ff0;
	char s[40], mcursor[256]; //�ַ�������� 
	int mx, my;// ��굱ǰλ�� 
	
	//��ʼ��GDT��IDT 
	init_gdtidt();
	//��ʼ��PIC 
	init_pic();
	//ִ��STIָ��֮���ж���ɱ�־λ���1��CPU���Խ��������ⲿ�豸���жϣ�����CLI����ָ�� 
	io_sti();
	//��ʼ����ɫ�� 
	init_palette();
	
	//���¾��ǻ�ͼ���� 
	init_screen(binfo->vram, binfo->scrnx, binfo->scrny);
	
	//��ӡ��겢��ʾ�����ָ���λ������ 
	mx = (binfo->scrnx - 16) / 2; /* ��Ļ����λ�ü��㣬���Ĵ�ӡ���Ҫ����Ļ����λ��������һ�� */
	my = (binfo->scrny - 28 - 16) / 2;
	init_mouse_cursor8(mcursor, COL8_008484);
	putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
	//��ӡ���굽�ַ��� 
	sprintf(s, "(%d, %d)", mx, my);
	putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
	
	//�޸�PIC��IMR�����Խ������Լ��̺������ж� 
	io_out8(PIC0_IMR, 0xf9); /* PIC1�ȥ��`�ܩ`�ɤ��S��(11111001) */
	io_out8(PIC1_IMR, 0xef); /* �ޥ������S��(11101111) */
	
	
	//ѭ����ֹ����ֹ�˳� 
	while(1) {
		io_hlt();
	}
}
