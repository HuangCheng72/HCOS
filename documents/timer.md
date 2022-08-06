## 运用小根堆思想重写定时器功能

author: HC        2022/08/07

在书中，定时器部分的内容位于第十二天和第十三天，在书的第220页到第261页。

在书中的第十三天结束时，作者给出的定时器代码，是我们改进的出发点，现将其粘贴如下（已将其中注释替换为中文）：

bootpack.h

```c
/* timer.c */
#define MAX_TIMER		500
struct TIMER {
	struct TIMER *next;
	unsigned int timeout, flags;
	struct FIFO32 *fifo;
	int data;
};
struct TIMERCTL {
	unsigned int count, next;
	struct TIMER *t0;
	struct TIMER timers0[MAX_TIMER];
};
extern struct TIMERCTL timerctl;
void init_pit(void);
struct TIMER *timer_alloc(void);
void timer_free(struct TIMER *timer);
void timer_init(struct TIMER *timer, struct FIFO32 *fifo, int data);
void timer_settime(struct TIMER *timer, unsigned int timeout);
void inthandler20(int *esp);
```

timer.c

```c
#include "bootpack.h"

#define PIT_CTRL	0x0043
#define PIT_CNT0	0x0040

struct TIMERCTL timerctl;

#define TIMER_FLAGS_ALLOC		1	/* 已配置状态 */
#define TIMER_FLAGS_USING		2	/* 定时器运行中 */

void init_pit(void)
{
	int i;
	struct TIMER *t;
	io_out8(PIT_CTRL, 0x34);
	io_out8(PIT_CNT0, 0x9c);
	io_out8(PIT_CNT0, 0x2e);
	timerctl.count = 0;
	for (i = 0; i < MAX_TIMER; i++) {
		timerctl.timers0[i].flags = 0; /* 没有使用 */
	}
	t = timer_alloc(); /* 取得一个定时器作为哨兵 */
	t->timeout = 0xffffffff;
	t->flags = TIMER_FLAGS_USING;
	t->next = 0; /* 末尾 */
	timerctl.t0 = t; /* 这个定时器作为哨兵，此时只有一个定时器，故位于最前 */
	timerctl.next = 0xffffffff; /* 只有哨兵，所以下一个超时时刻就是哨兵的时刻 */
	return;
}

struct TIMER *timer_alloc(void)
{
	int i;
	for (i = 0; i < MAX_TIMER; i++) {
		if (timerctl.timers0[i].flags == 0) {
			timerctl.timers0[i].flags = TIMER_FLAGS_ALLOC;
			return &timerctl.timers0[i];
		}
	}
	return 0; /* 定时器已满，不能再申请到了 */
}

void timer_free(struct TIMER *timer)
{
	timer->flags = 0; /* 没有使用 */
	return;
}

void timer_init(struct TIMER *timer, struct FIFO32 *fifo, int data)
{
	timer->fifo = fifo;
	timer->data = data;
	return;
}

void timer_settime(struct TIMER *timer, unsigned int timeout)
{
	int e;
	struct TIMER *t, *s;
	timer->timeout = timeout + timerctl.count;
	timer->flags = TIMER_FLAGS_USING;
	e = io_load_eflags();
	io_cli();
	t = timerctl.t0;
	if (timer->timeout <= t->timeout) {
		/* 插入最前面的情况下 */
		timerctl.t0 = timer;
		timer->next = t; /* 下一个是t */
		timerctl.next = timer->timeout;
		io_store_eflags(e);
		return;
	}
	/* 搜寻插入位置 */
	for (;;) {
		s = t;
		t = t->next;
		if (timer->timeout <= t->timeout) {
			/* 插入到s和t之间的时候 */
			s->next = timer; /* s的下一个是timer */
			timer->next = t; /* timer的下一个是t */
			io_store_eflags(e);
			return;
		}
	}
}

void inthandler20(int *esp)
{
	struct TIMER *timer;
	io_out8(PIC0_OCW2, 0x60);	/* 将IRQ-00接收信号结束的信息通知给PIC */
	timerctl.count++;
	if (timerctl.next > timerctl.count) {
		return;
	}
	timer = timerctl.t0; /* 首先把最前面的地址赋给timer */
	for (;;) {
		/* 因为timers的定时器都处于运行状态，因而不确认flags */
		if (timer->timeout > timerctl.count) {
			break;
		}
		/* 超时 */
		timer->flags = TIMER_FLAGS_ALLOC;
		fifo32_put(timer->fifo, timer->data);
		timer = timer->next; /* 下一定时器的地址赋给timer */
	}
	timerctl.t0 = timer;
	timerctl.next = timer->timeout;
	return;
}

```

作者给出的定时器，其实现是以链表作为基础，是一个按升序排列的有序链表。申请定时器即是申请一个结点的指针，结点先不插入链表中。在设置定时器的予定时刻即timeout时，再通过循环查找适当的位置并插入结点到该位置。在定时器控制结构体timerctl中，用一个指针t0存储链表的第一个结点，用一个整型变量next储存第一个定时器超时的时刻，便于判断是否超时。在中断函数inthandler20中，当发现第一个定时器已经超时的时候，就将已超时的定时器执行其功能并释放（free），并更新timerctl中的t0和next，直到第一个定时器不超时为止。

分析作者的实现思路，我们发现，其核心在于要找出那个timeout最小的定时器，然后将该定时器执行其功能并释放（free），作者虽然是按照升序排列整条链表，但是自始至终只针对链表的第一个结点也即timeout最小的定时器进行操作。

核心问题就是找出那个timeout最小的定时器，该问题可以转化为如何在大量数据中迅速找出最小值。

因此，运用我们所学过的数据结构与算法的相关知识，我们可以选择二叉搜索树、小根堆，可用于解决这一问题。

本人采用了小根堆这一数据结构，来解决这一问题。如果不知道小根堆这一数据结构，请移步https://blog.csdn.net/weixin_29648175/article/details/117209249，了解小根堆的原理。

在作者的版本中：

1. 当需要插入新的定时器的时候，需要遍历查找合适的位置（时间复杂度O(n)），然后再将结点插入该位置（时间复杂度O(1)）。插入操作的时间复杂度为O(n)。
2. 当需要释放一个定时器时，timeout最小的的定时器必定位于所维护的升序链表头部，因此可以直接获取timeout最小的定时器（时间复杂度O(1)），判断该定时器的timeout是否超时（时间复杂度O(1)），如果超时则直接删除该结点（该操作时间复杂度O(1)）。释放操作的时间复杂度为O(1)。

若使用小根堆进行改进：

1. 当插入新的定时器的时候，只需要插入堆底（时间复杂度O(1)），然后再上浮调整堆（时间复杂度O(log n)）。则插入操作的时间复杂度为O(log n)。
2. 当需要释放定时器的时候，timeout最小的定时器必定位于所维护的堆的堆顶，可以直接获取timeout最小的定时器（时间复杂度O(1)），判断该定时器的timeout是否超时（时间复杂度O(1)），如果超时则释放，交换到堆底再缩小堆的规模（时间复杂度O(1)），再下沉调整堆（时间复杂度O(log n)）。则释放操作的时间复杂度为O(log n)。

理论上来说，小根堆版本释放定时器会比链表版本释放定时器慢一些，但是并不会落后很多，而且插入操作的时间复杂度降低到了O(log n)，相比较于链表版本的O(n)提升明显，因此我决定应用小根堆这种数据结构进行改进。

以下所使用的小根堆及其改版均是本人所码出的，并未借鉴其他人。

version1代码如下：

timer.h（从bootpack.h中分离出来timer.c部分的声明，在bootpack.h中include进去即可）

```c
#define PIT_CTRL 0x0043
#define PIT_CNT0 0x0040

#define MAX_TIMER		500
struct TIMER {
	unsigned int timeout;
	struct FIFO32 *fifo;
	int data;
};
struct TIMERCTL {
    //count为计时器，size是现有计时器数目
	unsigned int count, size;
	//运用小顶堆思想改写（下标从1开始比较好计算），可以在timer[0]处放置哨兵
	//以timeout为排序键值
	struct TIMER timer[MAX_TIMER + 1];
};
extern struct TIMERCTL timerctl;
void init_pit(void);//初始化PIT
void timer_insert(struct FIFO32 *fifo , int data, unsigned int timeout);//插入一个指定属性的定时器
void timer_free();//释放已经到达的定时器
void inthandler20(int *esp);//处理函数 

```

timer.c

```c
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
//元素上浮，返回最终的位置，该函数不对外暴露
void swim(int i){
    //如果下标为i的定时器予定时刻小于下标为i/2的定时器予定时刻，就交换到前面去
    while(timerctl.timer[i].timeout < timerctl.timer[i/2].timeout){//因为timer[0]处是哨兵，交换到timer[1]就是堆顶了，不会再上浮
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
//插入一个定时器，设置各项属性，插入成功返回指针
void timer_insert(struct FIFO32 *fifo , int data, unsigned int timeout){
	if(timerctl.size == MAX_TIMER){
	    //如果定时器个数已经达到上限
        return; //插入失败
	}
	//没有到达上限就插入到size+1位置
	timerctl.size++;
	timerctl.timer[timerctl.size].data = data;
	timerctl.timer[timerctl.size].fifo = fifo;
	timerctl.timer[timerctl.size].timeout = timeout + timerctl.count;
	//上浮调整堆
    swim(timerctl.size)
	return;
}
//释放堆顶的定时器，因为此后数日的内容只用到了释放最小定时器的功能，故version1中简化如此。
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
        fifo32_put(timerctl.timer[1].fifo, timerctl.timer[1].data);
        //释放堆顶定时器
        timer_free();
	}
	return;
}

```

这个版本就是很正常的运用小根堆思想重写，并且将timer_alloc、timer_init、timer_settime函数合并为一个函数timer_insert，因此需要修改其他部分运用到定时器的代码，将timer_alloc、timer_init、timer_settime删去并代之以timer_insert，举例如下：

bootpack.c（13_day/harib10i）

```c
	//略
	struct TIMER *timer, *timer2, *timer3;
	//略
	timer = timer_alloc();
	timer_init(timer, &fifo, 10);
	timer_settime(timer, 1000);
	timer2 = timer_alloc();
	timer_init(timer2, &fifo, 3);
	timer_settime(timer2, 300);
	timer3 = timer_alloc();
	timer_init(timer3, &fifo, 1);
	timer_settime(timer3, 50);
```

修改为

```c
	timer_insert(&fifo,1,1000);
	timer_insert(&fifo,1,300);
	timer_insert(&fifo,1,50);
```

其他不加赘述，编译运行，效果良好。

后来，在知乎上看到Ron Tang前辈的这篇文章：https://zhuanlan.zhihu.com/p/70280608

发现Ron Tang前辈与我思路相同，十分惊喜，并且前辈亲自测试，其结果如文中所示，十分感谢前辈！

相对于作者所给的版本，基于小根堆的数据结构所实现的版本性能有了较大的提高，符合我们之前所分析的结果。

但是我在往下学习的过程中，发现在多任务部分需要用到定时器指针进行判断是否需要进行任务切换，同时在第二十二天的学习过程中，作者再次对定时器进行了修改，加入了取消定时器（也就是回收被释放的结点，之前被释放的结点是不能申请得到的，取消之后就可以申请得到了）的函数timer_cancel和timer_cancelall，并且后续的应用程序中大量运用了作者的给出的函数，工作量较大而我由于缺少时间以至于难以进行全面的替换。

因此我决定重新进行改写，恢复作者原有的函数声明，这样子能够大大减少替换的工作量，只需要替换实现文件即可。

因此在version1的基础上，我参考小根堆的一种实现形式最小索引优先队列的思路，针对当前问题，设计了这种数据结构，我称之为“改版指针优先队列”。

图示示例如下：

![](pic\modifiedminheap.jpg)

该数据结构是在小根堆的基础上，新增了一个total变量，记录已申请的定时器总数，下标为1到size的元素则是小根堆的范围。

当使用timer_alloc函数申请一个定时器的时候，则total自增，而size不变。

过程图示如下（**图示有误，应为一共申请了7个定时器，哨兵不算**）：

![](pic\alloctimer.jpg)

当使用timer_settime函数设定某个位于size和total之间的定时器的timeout值时，首先修改定时器的timeout值，然后将被设置的定时器与下标为size+1处元素交换位置，size自增，即扩大堆的规模，并上浮调整堆。

过程图示如下：

![](pic\settimeouttimer.jpg)

结果：

![](pic\settimeouttimer2.jpg)

当使用timer_free函数释放定时器的时候，则将该定时器与堆末尾（size位置）的定时器进行交换，而后size自减缩小堆的范围。

过程图示如下：

![](pic\freetimer1.jpg)

结果：

![](pic\freetimer2.jpg)

当使用timer_cancel删除某个定时器的时候，首先判断其是否在堆中，如果在堆中则要先调用timer_free函数释放，当定时器在size和total之间，则将定时器交换到已申请定时器的最末尾，也就是total位置，而后total自减，缩减已申请定时器的总数。

过程如图所示：

![](pic\timercancel1.jpg)

结果：

![](pic\timercancel2.jpg)

同时，因为交换结构体元素的速度较慢，远不如交换指针方便，因此我借鉴了索引优先队列的思路，将小根堆建立在指针数组上，结构体数组自始至终只用于存放数据，交换的元素从结构体改为结构体指针，如此，即可兼容作者所给出的函数声明。

我的完整实现如下：

timer.h

```c
#define PIT_CTRL 0x0043
#define PIT_CNT0 0x0040

#define MAX_TIMER		500
struct TIMER {
    //超时时间
    unsigned int timeout;
    //所在堆中下标（这里的下标是在指针数组ptr_timer里面的下标）
    int index;
    //绑定的缓冲区和数据
    struct FIFO32 *fifo;
    int data;
};
struct TIMERCTL {
    //count为计时器，size是现有已设定时刻的计时器数目（即堆规模），total是当前已经被申请出去的定时器数目
    unsigned int count, size, total;
    //运用小顶堆思想改写（下标从1开始比较好计算），可以在timer[0]处放置哨兵
    //以timeout为排序键值
    struct TIMER timer[MAX_TIMER + 1];
    //指针数组
    struct TIMER* ptr_timer[MAX_TIMER + 1];
};
extern struct TIMERCTL timerctl;
void init_pit(void);//初始化PIT
struct TIMER *timer_alloc(void); //申请一个定时器
void timer_free(struct TIMER *timer); //释放一个定时器
void timer_init(struct TIMER *timer, struct FIFO32 *fifo, int data); //初始化一个定时器的属性
void timer_settime(struct TIMER *timer, unsigned int timeout); //设置定时器的予定时刻
int timer_cancel(struct TIMER *timer); //删除掉alloc出来的一个定时器
void inthandler20(int *esp);//处理函数
void timer_cancelall(struct FIFO32 *fifo); //删除掉绑定了某个缓冲区的全部定时器

```

timer.c

```c
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
        sink(index);
        //交换过来的堆底元素所指向的定时器的timeout值无论如何都是不小于原来元素所指向的timeout值，因此只需要下沉调整即可
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
    //然后再设置予定时刻（从现在开始计算，何时超时）
    timer->timeout = timeout + timerctl.count;
    //因为元素位于堆底，只需要上浮调整堆即可
    swim(timerctl.size);
    io_store_eflags(e);
    return;
}
//删除掉一个定时器 
int timer_cancel(struct TIMER *timer){
	if(timerctl.total < timer->index){
		//定时器不属于已经申请的定时器之一，就返回
		return 1; 
	}
	//给定的定时器是否已经在堆中
    if(timerctl.size >= timer->index){
        //在堆中，就先释放 
        timer_free(timer); 
    }
    //再把指针直接交换到所有定时器最后，然后缩减所有定时器的规模 
    exchange(timer->index,timerctl.total);
    timerctl.total--;
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
            i--;//如此是为了防止交换过来的定时器绑定的缓冲区也是fifo
		}
	}
	io_store_eflags(e);
	return;
}

```

我的这一实现版本可以直接替换作者的timer，直接在30_day的harib27f替换：

将我的timer.h和timer.c保存为相同的文件名，而后直接复制粘贴到harib27f文件夹下面的haribote文件夹中。

修改bootpack.h，注释掉原先的timer.c的声明，并且include我的timer.h，保存即可。

```c
/* timer.c */
/*
#define MAX_TIMER		500
struct TIMER {
	struct TIMER *next;
	unsigned int timeout;
	char flags, flags2;
	struct FIFO32 *fifo;
	int data;
};
struct TIMERCTL {
	unsigned int count, next;
	struct TIMER *t0;
	struct TIMER timers0[MAX_TIMER];
};
extern struct TIMERCTL timerctl;
void init_pit(void);
struct TIMER *timer_alloc(void);
void timer_free(struct TIMER *timer);
void timer_init(struct TIMER *timer, struct FIFO32 *fifo, int data);
void timer_settime(struct TIMER *timer, unsigned int timeout);
void inthandler20(int *esp);
int timer_cancel(struct TIMER *timer);
void timer_cancelall(struct FIFO32 *fifo);
*/
#include "timer.h"
```

然后修改console.c中的hrb_api中的这段代码（第543行），并保存

```c
	} else if (edx == 16) {
		reg[7] = (int) timer_alloc();
		//((struct TIMER *) reg[7])->flags2 = 1;	/* 自動キャンセル有効 */ //注释掉这一句
	} else if (edx == 17) {
```

编译运行，即完成替换。

如出现乱码或者多于字符问题，请移步我的GitHub仓库

[GitHub - HuangCheng72/HCOS](https://github.com/HuangCheng72/HCOS)

进入HCOS目录下的hcos文件夹中可获取timer.h和timer.c的完整文件。
