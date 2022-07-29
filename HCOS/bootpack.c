#include "bootpack.h"

int keywin_off(struct SHEET *key_win, struct SHEET *sht_win, int cur_c, int cur_x);
int keywin_on(struct SHEET *key_win, struct SHEET *sht_win, int cur_c);

void HariMain(void){
	//Ⱦɫ����ָ�� ��ֱ��ָ��ָ������ָ����ʾ��Ϣ��ŵĵ�ַ 
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	//FIFO�����������ڴ�ռ�
	int fifobuf[128], keycmd_buf[32];
	//������
	struct FIFO32 fifo, keycmd;
	//�ַ������������������õ����� 
	char s[40];
	//����x��y���꣬�Լ�����i��Ŵӻ������ж�ȡ������ ��׷�����λ�õĹ�꣬�Լ������ɫ
	int mx, my, i, cursor_x, cursor_c;
	unsigned int memtotal;//�ڴ��С
	//�����Ϣ�ṹ�壬�Ժ������Ϣ�Ϳ��������� 
	struct MOUSE_DEC mdec;
	//�ڴ����ṹ��
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	//ͼ�����ṹ�� 
	struct SHTCTL *shtctl;
	//����ͼ��Ľṹ�� 
	struct SHEET *sht_back, *sht_mouse, *sht_win, *sht_cons;
	//�����ָ�� 
	struct TASK *task_a, *task_cons;
	//��ʱ��ָ�� 
	struct TIMER *timer;
	//buf_mouse����֮ǰ��mcursor����������꣬���ڵĻ������� 
	unsigned char *buf_back, buf_mouse[256], *buf_win, *buf_cons;
	//ʮ�����Ƶļ��̴��� 
	static char keytable0[0x80] = {
		0,   0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '^', 0,   0,
		'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '@', '[', 0,   0,   'A', 'S',
		'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', ':', 0,   0,   ']', 'Z', 'X', 'C', 'V',
		'B', 'N', 'M', ',', '.', '/', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
		'2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0x5c, 0,  0,   0,   0,   0,   0,   0,   0,   0,   0x5c, 0,  0
	};
	static char keytable1[0x80] = {
		0,   0,   '!', 0x22, '#', '$', '%', '&', 0x27, '(', ')', '~', '=', '~', 0,   0,
		'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '`', '{', 0,   0,   'A', 'S',
		'D', 'F', 'G', 'H', 'J', 'K', 'L', '+', '*', 0,   0,   '}', 'Z', 'X', 'C', 'V',
		'B', 'N', 'M', '<', '>', '?', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
		'2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   '_', 0,   0,   0,   0,   0,   0,   0,   0,   0,   '|', 0,   0
	};
	int key_shift = 0, key_leds = (binfo->leds >> 4) & 7, keycmd_wait = -1;
	//�����д��ڽṹ��ָ�� 
	struct CONSOLE *cons;
    //��¼�����������Ϣ���㵽�ĸ�ͼ��ȣ�
    int j, x, y, mmx = -1, mmy = -1;
	struct SHEET *sht = 0, *key_win;
	
	//��ʼ��GDT��IDT 
	init_gdtidt();
	//��ʼ��PIC 
	init_pic();
	//ִ��STIָ��֮���ж���ɱ�־λ���1��CPU���Խ��������ⲿ�豸���жϣ�����CLI����ָ�� 
	io_sti();
	//��ʼ�������� 
	fifo32_init(&fifo, 128, fifobuf,0);
	//��ʼ����ʱ�� 
	init_pit();
	//�޸�PIC��IMR�����Խ������Լ��̺������ж� 
	io_out8(PIC0_IMR, 0xf8); //����PIC1�ͼ����жϣ�11111000��Ȩ�� ����ʱ������Ҫ������Ȩ�ޣ� 
	io_out8(PIC1_IMR, 0xef); //��������ж�(11101111) Ȩ��
	
	init_keyboard(&fifo, 256);//��ʼ������
	enable_mouse(&fifo,512,&mdec);//ʹ�����ã������ṹ��ָ�봫��ȥ
	
	fifo32_init(&keycmd, 32, keycmd_buf, 0);
	
    //��ʼ���ڴ���� 
	memtotal = memtest(0x00400000, 0xbfffffff);
	memman_init(memman);
	memman_free(memman, 0x00001000, 0x0009e000); /* 0x00001000 - 0x0009efff */
	memman_free(memman, 0x00400000, memtotal - 0x00400000);
	
	//��ʼ����ɫ�� 
	init_palette();
	//���¾��ǻ�ͼ���� 
	shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
	//��ʼ������
	task_a = task_init(memman);
	fifo.task = task_a;
	task_run(task_a, 1, 2);
	*((int *) 0x0fe4) = (int) shtctl;
	//��ʼ������
	sht_back  = sheet_alloc(shtctl);
	buf_back  = (unsigned char *) memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
	sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1); //û��͸��ɫ
	init_screen8(buf_back, binfo->scrnx, binfo->scrny); //��ʼ��
	
	//����̨ͼ�� 
	sht_cons = sheet_alloc(shtctl);
	buf_cons = (unsigned char *) memman_alloc_4k(memman, 256 * 165);
	sheet_setbuf(sht_cons, buf_cons, 256, 165, -1); //û��͸��ɫ 
	make_window8(buf_cons, 256, 165, "console", 0);
	make_textbox8(sht_cons, 8, 28, 240, 128, COL8_000000);
	task_cons = task_alloc();
	task_cons->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024 - 12;
	task_cons->tss.eip = (int) &console_task;
	task_cons->tss.es = 1 * 8;
	task_cons->tss.cs = 2 * 8;
	task_cons->tss.ss = 1 * 8;
	task_cons->tss.ds = 1 * 8;
	task_cons->tss.fs = 1 * 8;
	task_cons->tss.gs = 1 * 8;
	*((int *) (task_cons->tss.esp + 4)) = (int) sht_cons;
	*((int *) (task_cons->tss.esp + 8)) = memtotal;
	task_run(task_cons, 2, 2); // level=2, priority=2
	 
	//���񴰿�Aͼ��ͻ��� 
	sht_win   = sheet_alloc(shtctl);
	buf_win   = (unsigned char *) memman_alloc_4k(memman, 160 * 52);
	sheet_setbuf(sht_win, buf_win, 144, 52, -1); // û��͸��ɫ
	make_window8(buf_win, 144, 52, "task_a",1);
	make_textbox8(sht_win, 8, 28, 128, 16, COL8_FFFFFF); //�����ı���
	cursor_x = 8;
	cursor_c = COL8_FFFFFF;
	//���롢���ö�ʱ�������ڹ����˸ 
	timer = timer_alloc();
	timer_init(timer, &fifo, 1);
	timer_settime(timer, 50);
	
	//���ͼ��ͻ��� 
	sht_mouse = sheet_alloc(shtctl);
	sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99); //͸��ɫ��99 
	init_mouse_cursor8(buf_mouse, 99);//����ɫ��99 
	//��ӡ��겢��ʾ�����ָ���λ������ 
	mx = (binfo->scrnx - 16) / 2; //��Ļ����λ�ü��㣬���Ĵ�ӡ���Ҫ����Ļ����λ��������һ��
	my = (binfo->scrny - 28 - 16) / 2;
	//����ͼ��ˢ����� 
	sheet_slide(sht_back,  0,  0);
	sheet_slide(sht_cons, 32,  4);
	sheet_slide(sht_win,  64, 56);
	sheet_slide(sht_mouse, mx, my);
	sheet_updown(sht_back,  0);
	sheet_updown(sht_cons,  1);
	sheet_updown(sht_win,   2);
	sheet_updown(sht_mouse, 3);
    key_win = sht_win;
	sht_cons->task = task_cons;
	sht_cons->flags |= 0x20;	//�й��
    
	//Ϊ�˱���ͼ��̵�ǰ״̬��ͻ����һ��ʼ�Ƚ������� 
	fifo32_put(&keycmd, KEYCMD_LED);
	fifo32_put(&keycmd, key_leds);
	
	
	for (;;) {
		if (fifo32_status(&keycmd) > 0 && keycmd_wait < 0) {
			//����������̿��������͵����ݾͷ����� 
			keycmd_wait = fifo32_get(&keycmd);
			wait_KBC_sendready();
			io_out8(PORT_KEYDAT, keycmd_wait);
		}
		io_cli(); //��ֹ�ж�
		if (fifo32_status(&fifo) == 0) {
			task_sleep(task_a);
			io_sti();
		} else {
			i = fifo32_get(&fifo);
			io_sti();
            if (key_win->flags == 0) {	//���봰�ڱ��ر�
				key_win = shtctl->sheets[shtctl->top - 1];
				cursor_c = keywin_on(key_win, sht_win, cursor_c);
			}
			if (256 <= i && i <= 511) { //�������� 
				if (i < 0x80 + 256) { //����������ת��Ϊ�ַ����� 
					if (key_shift == 0) {
						s[0] = keytable0[i - 256];
					} else {
						s[0] = keytable1[i - 256];
					}
				} else {
					s[0] = 0;
				}
				if ('A' <= s[0] && s[0] <= 'Z') {	//������ĸΪӢ����ĸ��ʱ�� 
					if (((key_leds & 4) == 0 && key_shift == 0) ||
							((key_leds & 4) != 0 && key_shift != 0)) {
						s[0] += 0x20;	//�л���Сд 
					}
				}
				if (s[0] != 0) { //��ͨ�ַ�
                    if (key_win == sht_win) {	//���͸�����A
						if (cursor_x < 128) {
							//���ַ������λ�ã����λ�ú���һλ 
							s[1] = 0;
							putfonts8_asc_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, s, 1);
							cursor_x += 8;
						}
					} else {	//���͸������д��� 
						fifo32_put(&key_win->task->fifo, s[0] + 256);
					}
				}
				if (i == 256 + 0x0e) {	//�˸�� 
					if (key_win == sht_win) {//���͸�����A 
						if (cursor_x > 8) {
							//�ÿհײ�������ѹ��ǰ��һλ 
							putfonts8_asc_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, " ", 1);
							cursor_x -= 8;
						}
					} else {	//���͸������д��� 
						fifo32_put(&key_win->task->fifo, 8 + 256);
					}
				}
				if (i == 256 + 0x1c) {	//�س��� 
					if (key_win != sht_win) {	//���͵������д��� 
						fifo32_put(&key_win->task->fifo, 10 + 256);
					}
				}
				if (i == 256 + 0x0f) {	//tab�� 
                    //�޸�������
					cursor_c = keywin_off(key_win, sht_win, cursor_c, cursor_x);
					j = key_win->height - 1;
					if (j == 0) {
						j = shtctl->top - 1;
					}
					key_win = shtctl->sheets[j];
					cursor_c = keywin_on(key_win, sht_win, cursor_c);
				}
				if (i == 256 + 0x2a) {	//��shift�� 
					key_shift |= 1;
				}
				if (i == 256 + 0x36) {	//��shift�� 
					key_shift |= 2;
				}
				if (i == 256 + 0xaa) {	//��shift�ر� 
					key_shift &= ~1;
				}
				if (i == 256 + 0xb6) {	//��shift�ر� 
					key_shift &= ~2;
				}
				if (i == 256 + 0x3a) {	//CapsLock
					key_leds ^= 4;
					fifo32_put(&keycmd, KEYCMD_LED);
					fifo32_put(&keycmd, key_leds);
				}
				if (i == 256 + 0x45) {	//NumLock
					key_leds ^= 2;
					fifo32_put(&keycmd, KEYCMD_LED);
					fifo32_put(&keycmd, key_leds);
				}
				if (i == 256 + 0x46) {	//ScrollLock
					key_leds ^= 1;
					fifo32_put(&keycmd, KEYCMD_LED);
					fifo32_put(&keycmd, key_leds);
				}
				if (i == 256 + 0x3b && key_shift != 0 && task_cons->tss.ss0 != 0) {	//Shift+F1
					cons = (struct CONSOLE *) *((int *) 0x0fec);
					cons_putstr0(cons, "\nBreak(key) :\n");
					io_cli();	//�����ڸı�Ĵ���ֵ��ʱ���л����������� 
					task_cons->tss.eax = (int) &(task_cons->tss.esp0);
					task_cons->tss.eip = (int) asm_end_app;
					io_sti();
				}
				if (i == 256 + 0xfa) {	//���̳ɹ����յ����� 
					keycmd_wait = -1;
				}
				if (i == 256 + 0xfe) {	//����û�гɹ����յ����� 
					wait_KBC_sendready();
					io_out8(PORT_KEYDAT, keycmd_wait);
				}
				//���������ʾ
				if (cursor_c >= 0) {
					boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
				}
				sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
			} else if (512 <= i && i <= 767) { //�������
				if (mouse_decode(&mdec, i - 512) != 0) {//����ɹ���û����;���ϵ����
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
					//Խ������ 
					if (mx > binfo->scrnx - 1) {
						mx = binfo->scrnx - 1;
					}
					if (my > binfo->scrny - 1) {
						my = binfo->scrny - 1;
					}
					sheet_slide(sht_mouse, mx, my);
					if ((mdec.btn & 0x01) != 0) {
						//�������
						if (mmx < 0) {
							//�������ͨ��ģʽ
							//���ղ��ܹ��ϵ��µ�˳��Ѱ�������ָ���ͼ��
							for (j = shtctl->top - 1; j > 0; j--) {
								sht = shtctl->sheets[j];
								x = mx - sht->vx0;
								y = my - sht->vy0;
								if (0 <= x && x < sht->bxsize && 0 <= y && y < sht->bysize) {
									if (sht->buf[y * sht->bxsize + x] != sht->col_inv) {
										sheet_updown(sht, shtctl->top - 1);
										if (sht != key_win) {
											cursor_c = keywin_off(key_win, sht_win, cursor_c, cursor_x);
											key_win = sht;
											cursor_c = keywin_on(key_win, sht_win, cursor_c);
										}
										if (3 <= x && x < sht->bxsize - 3 && 3 <= y && y < 21) {
											mmx = mx;	//���봰���ƶ�ģʽ
											mmy = my;
										}
										if (sht->bxsize - 21 <= x && x < sht->bxsize - 5 && 5 <= y && y < 19) {
											//�����X����ť
											if ((sht->flags & 0x10) != 0) {		//�ô����Ƿ�ΪӦ�ó��򴰿�
												cons = (struct CONSOLE *) *((int *) 0x0fec);
												cons_putstr0(cons, "\nBreak(mouse) :\n");
												io_cli();	//ǿ�ƽ��������н�ֹ�л�����
												task_cons->tss.eax = (int) &(task_cons->tss.esp0);
												task_cons->tss.eip = (int) asm_end_app;
												io_sti();
											}
										}
										break;
									}
								}
							}
						} else {
							//������ڴ����ƶ�ģʽ
							x = mx - mmx;	//���������ƶ�����
							y = my - mmy;
							sheet_slide(sht, sht->vx0 + x, sht->vy0 + y);
							mmx = mx;	//����Ϊ�ƶ��������
							mmy = my;
						}
					} else {
						//û�а������
						mmx = -1;	//����ͨ��ģʽ
					}
				}
			} else if (i <= 1) { //���������
				if (i != 0) {
                    timer_init(timer, &fifo, 0); //�����µĶ�ʱ�������ݸ�Ϊ0 
					if (cursor_c >= 0) {
						cursor_c = COL8_000000;
					}
				} else {
					timer_init(timer, &fifo, 1); //�����µĶ�ʱ�������ݸ�Ϊ1 
					if (cursor_c >= 0) {
						cursor_c = COL8_FFFFFF;
					}
				}
				timer_settime(timer, 50); //����ʱ��Ϊ50 
				if (cursor_c >= 0) {
					//���ǹ��λ�� 
					boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
					//ˢ��ͼ�㣬����һ��һ����Ч�� 
					sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
				}
			}
		}
	}
}

int keywin_off(struct SHEET *key_win, struct SHEET *sht_win, int cur_c, int cur_x){
	change_wtitle8(key_win, 0);
	if (key_win == sht_win) {
		cur_c = -1; //ɾ�����
		boxfill8(sht_win->buf, sht_win->bxsize, COL8_FFFFFF, cur_x, 28, cur_x + 7, 43);
	} else {
		if ((key_win->flags & 0x20) != 0) {
			fifo32_put(&key_win->task->fifo, 3); //�����д��ڹ��ر�
		}
	}
	return cur_c;
}

int keywin_on(struct SHEET *key_win, struct SHEET *sht_win, int cur_c){
	change_wtitle8(key_win, 1);
	if (key_win == sht_win) {
		cur_c = COL8_000000; //��ʾ���
	} else {
		if ((key_win->flags & 0x20) != 0) {
			fifo32_put(&key_win->task->fifo, 2); //�����д��ڹ���
		}
	}
	return cur_c;
}

