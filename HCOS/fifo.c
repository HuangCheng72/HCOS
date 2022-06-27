#include "fifo.h"

//溢出标识符 
#define FLAGS_OVERRUN		0x0001

void fifo8_init(struct FIFO8 *fifo, int size, unsigned char *buf){
	fifo->size = size;
	fifo->buf = buf;
	fifo->free = size; //缓冲区大小 
	fifo->flags = 0;
	fifo->p = 0; //下一个数据写入位置 
	fifo->q = 0; //下一个数据读出位置 
	return;
}

int fifo8_put(struct FIFO8 *fifo, unsigned char data){
	if (fifo->free == 0) {
		//缓冲区没容量了，必定溢出，修改标识符 
		fifo->flags |= FLAGS_OVERRUN;
		return -1;
	}
	//读入数据 
	fifo->buf[fifo->p] = data;
	//移动指针 
	fifo->p++;
	//如果移动到了末尾越界了，回到最前面 
	if (fifo->p == fifo->size) {
		fifo->p = 0;
	}
	//空余自减 
	fifo->free--;
	return 0;
}

int fifo8_get(struct FIFO8 *fifo){
	int data;
	if (fifo->free == fifo->size) {
		//没数据 
		return -1;
	}
	//不用多解释 
	data = fifo->buf[fifo->q];
	fifo->q++;
	if (fifo->q == fifo->size) {
		fifo->q = 0;
	}
	fifo->free++;
	return data;
}

int fifo8_status(struct FIFO8 *fifo){
	//多少已经用了 
	return fifo->size - fifo->free;
}

