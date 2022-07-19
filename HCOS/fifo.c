#include "bootpack.h"

void fifo32_init(struct FIFO32 *fifo, int size, int* buf, struct TASK *task){
    fifo->size = size;
    fifo->buf = buf;
    fifo->free = size; //��������С
    fifo->flags = 0;
    fifo->p = 0; //��һ������д��λ��
    fifo->q = 0; //��һ�����ݶ���λ��
    fifo->task = task; //�������� 
	return;
}

int fifo32_put(struct FIFO32 *fifo, int data){
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
    if (fifo->task != 0) {
		if (fifo->task->flags != 2) { //�������������״̬ 
			task_run(fifo->task, -1, 0); //�������� 
		}
	}
    return 0;
}

int fifo32_get(struct FIFO32 *fifo){
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

int fifo32_status(struct FIFO32 *fifo){
    //�����Ѿ�����
    return fifo->size - fifo->free;
}

