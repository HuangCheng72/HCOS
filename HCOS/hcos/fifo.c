#include "bootpack.h"

#define FLAGS_OVERRUN		0x0001

void fifo32_init(struct FIFO32 *fifo, int size, int* buf, struct TASK *task){
    fifo->size = size;
    fifo->buf = buf;
    fifo->free = size; //缓冲区大小
    fifo->flags = 0;
    fifo->p = 0; //下一个数据写入位置
    fifo->q = 0; //下一个数据读出位置
    fifo->task = task; //设置任务 
	return;
}

int fifo32_put(struct FIFO32 *fifo, int data){
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
    if (fifo->task != 0) {
		if (fifo->task->flags != 2) { //如果任务处于休眠状态 
			task_run(fifo->task, -1, 0); //将任务唤醒 
		}
	}
    return 0;
}

int fifo32_get(struct FIFO32 *fifo){
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

int fifo32_status(struct FIFO32 *fifo){
    //多少已经用了
    return fifo->size - fifo->free;
}

