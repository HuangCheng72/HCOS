#include "bootpack.h"

struct TIMERCTL timerctl;

//初始化PIT
void init_pit(void){
	io_out8(PIT_CTRL, 0x34);
	io_out8(PIT_CNT0, 0x9c);
	io_out8(PIT_CNT0, 0x2e);
	timerctl.count = 0;
	timerctl.size = 0;
	timerctl.timer[0].timeout = 0;//设置timer[0]处是哨兵
	return;
}
//交换两个元素，该函数不对外暴露
void exchange(int i,int j){
    if(i == j){
        return;
    }
    struct TIMER temp = timerctl.timer[i];
    timerctl.timer[i] = timerctl.timer[j];
    timerctl.timer[j] = temp;
    return;
}
//元素上浮，该函数不对外暴露
void swim(int i){
//    int i = k;
    //如果下标为i的定时器予定时刻小于下标为i/2的定时器予定时刻，就交换到前面去
    while(timerctl.timer[i].timeout < timerctl.timer[i/2].timeout){//因为timer[0]处是哨兵，交换到timer[1]就是堆顶了，不会再上浮
        exchange(i,i/2);
        i /= 2;
    }
    return;
}
//元素下沉，该函数不对外暴露
void sink(int i){
//    int i = k;
    //若是i处定时器予定时刻大于i*2或者i*2+1处定时器予定时刻，就交换到较小的那个位置
    //注意堆的规模
    while(2 * i <= timerctl.size){
        //比较i*2和i*2+1哪个更小
        int j = 2 * i;
        if(j < timerctl.size && timerctl.timer[j+1].timeout < timerctl.timer[j].timeout){
            j++;
        }
        if(timerctl.timer[i].timeout < timerctl.timer[j].timeout){
            //i处定时器予定时刻已经比两个当中最小的那一个都要小了，说明符合小顶堆性质，退出
            return;
        }
        //不符合小顶堆性质，就交换
        exchange(i,j);
        //更新j的值
        i = j;
    }
    return;
}
//插入一个定时器，设置各项属性，插入成功返回下标，
int timer_insert(unsigned int timeout, unsigned char data, struct FIFO8 *fifo){
	if(timerctl.size == MAX_TIMER){
	    //如果定时器个数已经达到上限
        return -1; //插入失败
	}
	//没有到达上限就插入到size+1位置
	timerctl.size++;
	timerctl.timer[timerctl.size].data = data;
	timerctl.timer[timerctl.size].fifo = fifo;
	timerctl.timer[timerctl.size].timeout = timeout;
	//上浮调整堆
	swim(timerctl.size);
	//返回下标
	return timerctl.size;
}
//释放堆顶的定时器
void timer_free(){
    //先判断能不能释放
    if(timerctl.size == 0){
        return;
    }
    //可以释放，将其与堆底元素交换，再缩减堆规模
    exchange(1,timerctl.size);
    timerctl.size--;
    //下沉调整堆
    if(timerctl.size > 1){
        sink(1);
    }
	return;
}

void inthandler20(int *esp){
	io_out8(PIC0_OCW2, 0x60);	/* 把IRQ-00信号接受结束的信息通知给PIC */
	timerctl.count++;//计时增加
	//判断有没有定时器，如果有的话，堆顶定时器是否已经超时 
	while(timerctl.size > 0 && timerctl.timer[1].timeout <= timerctl.count){
	    //输出到缓冲区
        fifo8_put(timerctl.timer[1].fifo, timerctl.timer[1].data);
        //释放堆顶定时器
        timer_free();
	}
	return;
}
