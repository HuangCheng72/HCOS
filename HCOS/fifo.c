#include "fifo.h"

//�����ʶ�� 
#define FLAGS_OVERRUN		0x0001

void fifo8_init(struct FIFO8 *fifo, int size, unsigned char *buf){
	fifo->size = size;
	fifo->buf = buf;
	fifo->free = size; //��������С 
	fifo->flags = 0;
	fifo->p = 0; //��һ������д��λ�� 
	fifo->q = 0; //��һ�����ݶ���λ�� 
	return;
}

int fifo8_put(struct FIFO8 *fifo, unsigned char data){
	if (fifo->free == 0) {
		//������û�����ˣ��ض�������޸ı�ʶ�� 
		fifo->flags |= FLAGS_OVERRUN;
		return -1;
	}
	//�������� 
	fifo->buf[fifo->p] = data;
	//�ƶ�ָ�� 
	fifo->p++;
	//����ƶ�����ĩβԽ���ˣ��ص���ǰ�� 
	if (fifo->p == fifo->size) {
		fifo->p = 0;
	}
	//�����Լ� 
	fifo->free--;
	return 0;
}

int fifo8_get(struct FIFO8 *fifo){
	int data;
	if (fifo->free == fifo->size) {
		//û���� 
		return -1;
	}
	//���ö���� 
	data = fifo->buf[fifo->q];
	fifo->q++;
	if (fifo->q == fifo->size) {
		fifo->q = 0;
	}
	fifo->free++;
	return data;
}

int fifo8_status(struct FIFO8 *fifo){
	//�����Ѿ����� 
	return fifo->size - fifo->free;
}

