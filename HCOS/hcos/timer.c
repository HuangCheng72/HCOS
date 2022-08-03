#include "bootpack.h"

struct TIMERCTL timerctl;
//交换两个元素，该函数不对外暴露
void exchange(int i,int j){
    if(i == j){
        return;
    }
    //交换指针元素
    struct TIMER* temp = timerctl.ptr_timer[i];
    timerctl.ptr_timer[i] = timerctl.ptr_timer[j];
    timerctl.ptr_timer[j] = temp;
    //更新下标
    timerctl.ptr_timer[i]->index = i;
    timerctl.ptr_timer[j]->index = j;
    return;
}
//元素上浮，返回最终的位置，该函数不对外暴露
void swim(int i){
    //如果下标为i的定时器予定时刻小于下标为i/2的定时器予定时刻，就交换到前面去
    while(timerctl.ptr_timer[i]->timeout < timerctl.ptr_timer[i/2]->timeout){//因为timer[0]处是哨兵，交换到timer[1]就是堆顶了，不会再上浮
        exchange(i,i/2);
        i /= 2;
    }
    return;
}
//元素下沉，该函数不对外暴露
void sink(int i){
    //若是i处定时器予定时刻大于i*2或者i*2+1处定时器予定时刻，就交换到较小的那个位置
    //注意堆的规模
    while(2 * i <= timerctl.size){
        //比较i*2和i*2+1哪个更小
        int j = 2 * i;
        if(j < timerctl.size && timerctl.ptr_timer[j+1]->timeout < timerctl.ptr_timer[j]->timeout){
            j++;
        }
        if(timerctl.ptr_timer[i]->timeout < timerctl.ptr_timer[j]->timeout){
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
//初始化PIT
void init_pit(void){
	io_out8(PIT_CTRL, 0x34);
	io_out8(PIT_CNT0, 0x9c);
	io_out8(PIT_CNT0, 0x2e);
	timerctl.count = 0;
    timerctl.size = 0;
    timerctl.total = 0;
    timerctl.timer[0].timeout = 0;//设置timer[0]处是哨兵
    //指针指向具体的内存空间
    //下标也更新为在指针数组中的位置
    int i;
    for(i = 0; i <= MAX_TIMER; i++){
        timerctl.ptr_timer[i] = &timerctl.timer[i];
        timerctl.ptr_timer[i]->index = i;
    }
    return;
}
//申请一个定时器
struct TIMER *timer_alloc(void){
    if(timerctl.total == MAX_TIMER){
        //如果定时器个数已经达到上限
        return 0; //插入失败，返回null指针
    }
    //没有到达上限就插入到total+1位置，所以返回这个指针
    return timerctl.ptr_timer[++timerctl.total];
}
//释放一个定时器
void timer_free(struct TIMER *timer){
    //先判断能不能释放
    //有没有正在运行的定时器或者是不是在所有定时器之外
    if(timerctl.size == 0 || timerctl.total < timer->index){
        return;
    }
    //给定的定时器是否已经在堆中
    if(timerctl.size < timer->index){
        //不在堆中的话不需要我们操心
        return;
    } else {
        //在堆中
        int e;
		e = io_load_eflags();
		io_cli(); //正在设置，禁止中断 
        //首先把这个指针交换到堆末尾，然后再缩小堆的规模
        int index = timer->index;
        exchange(index,timerctl.size);
        timerctl.size--;
        //然后调整顺序（下沉）
        sink(1);
        //上浮调整
        swim(timerctl.size);
        io_store_eflags(e);
    }
    return;
}
//初始化一个定时器的属性
void timer_init(struct TIMER *timer, struct FIFO32 *fifo, int data){
    timer->fifo = fifo;
    timer->data = data;
    return;
}
//设置定时器的予定时刻
void timer_settime(struct TIMER *timer, unsigned int timeout){
    //如果在所有定时器之外，退出
    if(timer->index > timerctl.total){
        return;
    }
    int e;
	e = io_load_eflags();
	io_cli(); //正在设置，禁止中断 
    //如果在堆外，就要首先进堆
    if(timer->index > timerctl.size){
        //扩大堆的范围
        timerctl.size++;
        //把位置交换到堆的最末尾
        exchange(timer->index , timerctl.size);
    }
    //然后再设置时刻
    timer->timeout = timeout;
    //上浮下沉调整堆
    swim(1);
    sink(timerctl.size);
    io_store_eflags(e);
    return;
}
//删除掉一个定时器 
int timer_cancel(struct TIMER *timer){
	if(timerctl.total < timer->index){
		//定时器不属于已经申请的定时器之一，就返回
		return 1; 
	}
    int e;
	e = io_load_eflags();
	io_cli(); //正在设置，禁止中断 
	//给定的定时器是否已经在堆中
    if(timerctl.size >= timer->index){
        //在堆中，就先释放 
        timer_free(timer); 
    }
    //再把指针直接交换到所有定时器最后，然后缩减所有定时器的规模 
    exchange(timer->index,timerctl.total);
    timerctl.total--;
    io_store_eflags(e);
    return 1;
}
void inthandler20(int *esp){
	char ts = 0;//切换任务标识符 
	io_out8(PIC0_OCW2, 0x60);	//把IRQ-00信号接受结束的信息通知给PIC
	timerctl.count++;//计时增加
	//判断有没有定时器，如果有的话，堆顶定时器是否已经超时 
	while(timerctl.size > 0 && timerctl.ptr_timer[1]->timeout <= timerctl.count){
		//如果有定时器判断一下堆顶是不是多任务的定时器
		if (timerctl.ptr_timer[1] == task_timer) {
			//如果是，就修改标识符
			ts = 1;
		}else{
			//如果不是
			//输出到缓冲区
        	fifo32_put(timerctl.ptr_timer[1]->fifo, timerctl.ptr_timer[1]->data);
		}
		//释放堆顶定时器
        timer_free(timerctl.ptr_timer[1]);
	}
	if(ts != 0){
		//标识符为真，切换任务 
		task_switch();
	}
	return;
}

void timer_cancelall(struct FIFO32 *fifo){
	int e, i;
	struct TIMER *t;
	e = io_load_eflags();
	io_cli();	//禁止中断 
	for (i = 1; i <= timerctl.total; i++) {
		t = timerctl.ptr_timer[i];
		if (t->fifo == fifo) {
			timer_cancel(t);
		}
	}
	io_store_eflags(e);
	return;
}
