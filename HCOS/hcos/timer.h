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
