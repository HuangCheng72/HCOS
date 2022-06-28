#include "bootpack.h" 

void wait_KBC_sendready(void){
	//�ȴ����̿��Ƶ�·׼����� 
	while(1){
		if ((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0) {
			break;
		}
	}
	return;
}

void init_keyboard(void){
	//��ʼ�����̿��Ƶ�· 
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, KBC_MODE);
	return;
}

void enable_mouse(struct MOUSE_DEC *mdec){
	//������� 
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
	mdec->phase = 0; //���ó�ʼ״̬Ϊ0���ȴ�0xfa 
	return; //˳���Ļ����̿��ƻ᷵��ACK(0xfa) 
}
//������ݽ��� 
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat){
	if (mdec->phase == 0) {
		//�ȴ�����0xfa�Ľ׶� 
		if (dat == 0xfa) {
			mdec->phase = 1;
		}
		return 0;
	}
	if (mdec->phase == 1) {
		//�ȴ����ĵ�һ�ֽڵĽ׶� 
		if ((dat & 0xc8) == 0x08) {
			//�����һ�ֽ���ȷ 
			mdec->buf[0] = dat;
			mdec->phase = 2;
		}
		return 0;
	}
	if (mdec->phase == 2) {
		//�ȴ����ĵڶ��ֽڵĽ׶� 
		mdec->buf[1] = dat;
		mdec->phase = 3;
		return 0;
	}
	if (mdec->phase == 3) {
		//�ȴ����ĵ����ֽڵĽ׶�
		mdec->buf[2] = dat;
		mdec->phase = 1;
		mdec->btn = mdec->buf[0] & 0x07;
		mdec->x = mdec->buf[1];
		mdec->y = mdec->buf[2];
		if ((mdec->buf[0] & 0x10) != 0) {
			mdec->x |= 0xffffff00;
		}
		if ((mdec->buf[0] & 0x20) != 0) {
			mdec->y |= 0xffffff00;
		}
		mdec->y = - mdec->y; //����y�᷽���뻭����ŷ����෴�������µ� ������Ҫȡ�� 
		return 1; //��ɽ��� 
	}
	return -1; //�����������Ӧ�ò��ᵽ����������������˵��������ݿ��ܳ������� 
}

