#include "bootpack.h"

extern struct FIFO8 keyfifo, mousefifo;//���������� 

void HariMain(void){
	//Ⱦɫ����ָ�� ��ֱ��ָ��ָ������ָ����ʾ��Ϣ��ŵĵ�ַ 
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO; 
	//�����Ϣ�ṹ�壬�Ժ������Ϣ�Ϳ��������� 
	struct MOUSE_DEC mdec;
	char s[40], mcursor[256]; //�ַ�������� 
	int mx, my;// ��굱ǰλ�� 
	
	//��ʼ��GDT��IDT 
	init_gdtidt();
	//��ʼ��PIC 
	init_pic();
	//ִ��STIָ��֮���ж���ɱ�־λ���1��CPU���Խ��������ⲿ�豸���жϣ�����CLI����ָ�� 
	io_sti();
	//��ʼ��������
	char keybuf[32], mousebuf[128];
	fifo8_init(&keyfifo, 32, keybuf);
	fifo8_init(&mousefifo, 128, mousebuf);
	//�޸�PIC��IMR�����Խ������Լ��̺������ж� 
	io_out8(PIC0_IMR, 0xf9); //����PIC1�ͼ����жϣ�11111001��Ȩ�� 
	io_out8(PIC1_IMR, 0xef); //��������ж�(11101111) Ȩ�� 
	
	init_keyboard();//��ʼ������ 
	
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
	
	enable_mouse(&mdec);//ʹ�����ã������ṹ��ָ�봫��ȥ 
	
	int i;//���ڴ洢��ȡ������������
	//ѭ����ֹ����ֹ�˳� 
	while(1) {
		io_cli(); //��ֹ�жϣ���ӡ��ʱ�����ж� 
		if (fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0) {
			io_stihlt();
		} else {
			if (fifo8_status(&keyfifo) != 0) {
				i = fifo8_get(&keyfifo);
				io_sti();
				sprintf(s, "%02X", i);
				boxfill8(binfo->vram, binfo->scrnx, COL8_008484,  0, 16, 15, 31);
				putfonts8_asc(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
			} else if (fifo8_status(&mousefifo) != 0) {
				i = fifo8_get(&mousefifo);
				io_sti();
				if (mouse_decode(&mdec, i) != 0) {//����ɹ���û����;���ϵ���� 
					//�ɹ�����������ֽڵ�������ǵ�ȻҪ��� 
					sprintf(s, "[lcr %4d %4d]", mdec.x, mdec.y);
					if ((mdec.btn & 0x01) != 0) {
						//��������������� 
						s[1] = 'L';
					}
					if ((mdec.btn & 0x02) != 0) {
						//�������Ҽ������� 
						s[3] = 'R';
					}
					if ((mdec.btn & 0x04) != 0) {
						//�����껬�ּ������� 
						s[2] = 'C';
					}
					boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 32, 16, 32 + 15 * 8 - 1, 31);
					putfonts8_asc(binfo->vram, binfo->scrnx, 32, 16, COL8_FFFFFF, s);
					//��ʼ�ƶ����ָ��
					//������һ�����ΰ���������ָ���˿�ʼ�������µ�16*16����������ˣ�ע�ⱳ��ɫ
					boxfill8(binfo->vram, binfo->scrnx, COL8_008484, mx, my, mx + 15, my + 15);
					//Ȼ��������ָ��λ��
					//��λ��+ƫ���� 
					mx += mdec.x;
					my += mdec.y;
					//��ֹԽ�� 
					if (mx < 0) {
						mx = 0;
					}
					if (my < 0) {
						my = 0;
					}
					//�Ƿ����������� 
					if (mx > binfo->scrnx - 16) {
						mx = binfo->scrnx - 16;
					}
					if (my > binfo->scrny - 16) {
						my = binfo->scrny - 16;
					}
					//�����������ƣ��ظ�֮ǰ�����̣������������ 
					putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
					//��ӡ���굽�ַ��� 
					sprintf(s, "(%3d, %3d)", mx, my);
					//��ӡ֮ǰ�Ȱ�ԭ���������ס
					boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 0, 79, 15);
					//��ӡ�µ��ַ�������Ļ�� 
					putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
				}
			}
		}
	}
}
