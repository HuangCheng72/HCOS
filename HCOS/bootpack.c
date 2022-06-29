#include "bootpack.h"

extern struct FIFO8 keyfifo, mousefifo;//���������� 

void HariMain(void){
	//Ⱦɫ����ָ�� ��ֱ��ָ��ָ������ָ����ʾ��Ϣ��ŵĵ�ַ 
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO; 
	//�����Ϣ�ṹ�壬�Ժ������Ϣ�Ϳ��������� 
	struct MOUSE_DEC mdec;
	//�ڴ����ṹ��
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR; 
	//ͼ�����ṹ�� 
	struct SHTCTL *shtctl;
	struct SHEET *sht_back, *sht_mouse;
	
	char s[40],keybuf[32], mousebuf[128]; //�ַ����ͼ�����껺����
	//buf_mouse����֮ǰ��mcursor
	unsigned char *buf_back, buf_mouse[256];
	int mx, my, i;// ��굱ǰλ�ã����ڴ洢��ȡ�����������ݵı��� 
	
	//��ʼ��GDT��IDT 
	init_gdtidt();
	//��ʼ��PIC 
	init_pic();
	//ִ��STIָ��֮���ж���ɱ�־λ���1��CPU���Խ��������ⲿ�豸���жϣ�����CLI����ָ�� 
	io_sti();
	//��ʼ�������� 
	fifo8_init(&keyfifo, 32, keybuf);
	fifo8_init(&mousefifo, 128, mousebuf);
	//�޸�PIC��IMR�����Խ������Լ��̺������ж� 
	io_out8(PIC0_IMR, 0xf9); //����PIC1�ͼ����жϣ�11111001��Ȩ�� 
	io_out8(PIC1_IMR, 0xef); //��������ж�(11101111) Ȩ�� 
	
	init_keyboard();//��ʼ������ 
	enable_mouse(&mdec);//ʹ�����ã������ṹ��ָ�봫��ȥ 
	
	unsigned int memtotal = memtest(0x00400000, 0xbfffffff);
	memman_init(memman);
	memman_free(memman, 0x00001000, 0x0009e000); /* 0x00001000 - 0x0009efff */
	memman_free(memman, 0x00400000, memtotal - 0x00400000);
	
	
	//��ʼ����ɫ�� 
	init_palette();
	//���¾��ǻ�ͼ���� 
	shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
	sht_back  = sheet_alloc(shtctl);
	sht_mouse = sheet_alloc(shtctl);
	buf_back  = (unsigned char *) memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
	sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1); /* û��͸��ɫ */
	sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);//͸��ɫ��99 
	init_screen8(buf_back, binfo->scrnx, binfo->scrny);
	init_mouse_cursor8(buf_mouse, 99);//����ɫ��99 
	sheet_slide(shtctl, sht_back, 0, 0);
	//��ӡ��겢��ʾ�����ָ���λ������ 
	mx = (binfo->scrnx - 16) / 2; /* ��Ļ����λ�ü��㣬���Ĵ�ӡ���Ҫ����Ļ����λ��������һ�� */
	my = (binfo->scrny - 28 - 16) / 2;
	sheet_slide(shtctl, sht_mouse, mx, my);
	sheet_updown(shtctl, sht_back,  0);
	sheet_updown(shtctl, sht_mouse, 1);
	//��ӡ���굽�ַ��� 
	sprintf(s, "(%d, %d)", mx, my);
	putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
	
	sprintf(s, "memory %dMB   free : %dKB",
			memtotal / (1024 * 1024), memman_total(memman) / 1024);
	putfonts8_asc(binfo->vram, binfo->scrnx, 0, 32, COL8_FFFFFF, s);
	sheet_refresh(shtctl, sht_back, 0, 0, binfo->scrnx, 48);
	
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
				sheet_refresh(shtctl, sht_back, 0, 16, 16, 32);
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
					boxfill8(buf_back, binfo->scrnx, COL8_008484, 32, 16, 32 + 15 * 8 - 1, 31);
					putfonts8_asc(buf_back, binfo->scrnx, 32, 16, COL8_FFFFFF, s);
					sheet_refresh(shtctl, sht_back, 32, 16, 32 + 15 * 8, 32);
					//��ʼ�ƶ����ָ��
					//�������ָ��λ��
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
					//�����������ƣ��ظ�֮ǰ�����̣���ӡ���굽�ַ��� 
					sprintf(s, "(%3d, %3d)", mx, my);
					boxfill8(buf_back, binfo->scrnx, COL8_008484, 0, 0, 79, 15); //��ӡ֮ǰ�Ȱ�ԭ���������ס
					putfonts8_asc(buf_back, binfo->scrnx, 0, 0, COL8_FFFFFF, s); //��ӡ�µ��ַ�������Ļ�� 
					sheet_refresh(shtctl, sht_back, 0, 0, 80, 16);
					sheet_slide(shtctl, sht_mouse, mx, my);
				}
			}
		}
	}
}
