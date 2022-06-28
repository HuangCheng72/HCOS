#include "bootpack.h"

#define MEMMAN_FREES		4090	/* ��Լ32KB */
#define MEMMAN_ADDR			0x003c0000

struct FREEINFO {	/* ������Ϣ */
	unsigned int addr, size;
};

struct MEMMAN {		/* �ڴ���� */
	int frees, maxfrees, lostsize, losts;
	struct FREEINFO free[MEMMAN_FREES];
};

unsigned int memtest(unsigned int start, unsigned int end);
void memman_init(struct MEMMAN *man);
unsigned int memman_total(struct MEMMAN *man);
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size);
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size);

extern struct FIFO8 keyfifo, mousefifo;//���������� 

void HariMain(void){
	//Ⱦɫ����ָ�� ��ֱ��ָ��ָ������ָ����ʾ��Ϣ��ŵĵ�ַ 
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO; 
	//�����Ϣ�ṹ�壬�Ժ������Ϣ�Ϳ��������� 
	struct MOUSE_DEC mdec;
	//�ڴ����ṹ��
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR; 
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
	enable_mouse(&mdec);//ʹ�����ã������ṹ��ָ�봫��ȥ 
	
	unsigned int memtotal = memtest(0x00400000, 0xbfffffff);
	memman_init(memman);
	memman_free(memman, 0x00001000, 0x0009e000); /* 0x00001000 - 0x0009efff */
	memman_free(memman, 0x00400000, memtotal - 0x00400000);
	
	
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
	
	sprintf(s, "memory %dMB   free : %dKB",
			memtotal / (1024 * 1024), memman_total(memman) / 1024);
	putfonts8_asc(binfo->vram, binfo->scrnx, 0, 32, COL8_FFFFFF, s);
	
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

#define EFLAGS_AC_BIT		0x00040000
#define CR0_CACHE_DISABLE	0x60000000

unsigned int memtest(unsigned int start, unsigned int end){
	char flg486 = 0;
	unsigned int eflg, cr0, i;

	//ȷ��CPU��386����486�����ϵ� 
	eflg = io_load_eflags();
	eflg |= EFLAGS_AC_BIT; /* AC-bit = 1 */
	io_store_eflags(eflg);
	eflg = io_load_eflags();
	if ((eflg & EFLAGS_AC_BIT) != 0) { /* �����386����ʹ��������趨��AC���ǻ�ص�0 */
		flg486 = 1;
	}
	eflg &= ~EFLAGS_AC_BIT; /* AC-bit = 0 */
	io_store_eflags(eflg);

	if (flg486 != 0) {
		cr0 = load_cr0();
		cr0 |= CR0_CACHE_DISABLE; /* ��ʱ��ֹ���棨����Ϊ�˼����ڴ����Ҫ�� */
		store_cr0(cr0);
	}

	i = memtest_sub(start, end);

	if (flg486 != 0) {
		cr0 = load_cr0();
		cr0 &= ~CR0_CACHE_DISABLE; /* ������ */
		store_cr0(cr0);
	}

	return i;
}

void memman_init(struct MEMMAN *man){
	man->frees = 0;			/* ������Ϣ�� */
	man->maxfrees = 0;		/* ���ڹ۲����״����frees�����ֵ */
	man->lostsize = 0;		/* �ͷ�ʧ�ܵ��ڴ��С�ܺͣ�Ҳ���Ƕ�ʧ����Ƭ */
	man->losts = 0;			/* �ͷ�ʧ�ܵĴ��� */
	return;
}
//�����ڴ�ϼ��Ƕ��� 
unsigned int memman_total(struct MEMMAN *man){
	unsigned int i, t = 0;
	for (i = 0; i < man->frees; i++) {
		t += man->free[i].size;
	}
	return t;
}
//�ڴ���� 
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size){
	unsigned int i, a;
	for (i = 0; i < man->frees; i++) {
		if (man->free[i].size >= size) {
			/* �ҵ����㹻����ڴ� */
			a = man->free[i].addr;
			man->free[i].addr += size;
			man->free[i].size -= size;
			if (man->free[i].size == 0) {
				/* ���free[i]�����0���ͼ�ȥһ��������Ϣ*/
				man->frees--;
				for (; i < man->frees; i++) {
					man->free[i] = man->free[i + 1]; /* �ṹ����� */
				}
			}
			return a;
		}
	}
	return 0; /* �޿��ÿռ� */
}
//�ڴ��ͷ� 
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size){
	int i, j;
	/* Ϊ�˱��ںϲ��ڴ棬��free[]����addr��˳������ */
	/* ���ԣ��Ⱦ���Ӧ�÷������� */
	for (i = 0; i < man->frees; i++) {
		if (man->free[i].addr > addr) {
			break;
		}
	}
	/* free[i - 1].addr < addr < free[i].addr */
	if (i > 0) {
		/* ǰ���п����ڴ� */
		if (man->free[i - 1].addr + man->free[i - 1].size == addr) {
			/* ��ǰ�ϲ� */
			man->free[i - 1].size += size;
			if (i < man->frees) {
				/* ����Ҳ�� */
				if (addr + size == man->free[i].addr) {
					/* ���ϲ� */
					man->free[i - 1].size += man->free[i].size;
					/* man->free[i]ɾ�� */
					/* free[i]���0֮��ϲ���ǰ��ȥ */
					man->frees--;
					for (; i < man->frees; i++) {
						man->free[i] = man->free[i + 1]; /* �ṹ�帳ֵ */
					}
				}
			}
			return 0; /* �ɹ���� */
		}
	}
	/* ������ǰ��Ŀ��ÿռ�ϲ���һ�� */
	if (i < man->frees) {
		/* ���滹�� */
		if (addr + size == man->free[i].addr) {
			/* ���Ժͺ���Ŀռ�ϲ� */
			man->free[i].addr = addr;
			man->free[i].size += size;
			return 0; /* �ɹ���� */
		}
	}
	/* �Ȳ�����ǰ��ϲ����ֲ��������ϲ� */
	if (man->frees < MEMMAN_FREES) {
		/* free[i]֮��ģ�����ƶ����ڳ����ÿռ� */
		for (j = man->frees; j > i; j--) {
			man->free[j] = man->free[j - 1];
		}
		man->frees++;
		if (man->maxfrees < man->frees) {
			man->maxfrees = man->frees; /* �������ֵ */
		}
		man->free[i].addr = addr;
		man->free[i].size = size;
		return 0; /* �ɹ���� */
	}
	/* ���������ƶ� */
	man->losts++;
	man->lostsize += size;
	return -1; /* ʧ�� */
}


