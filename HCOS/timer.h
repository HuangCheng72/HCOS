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
struct TIMER* timer_insert(struct FIFO32 *fifo , int data, unsigned int timeout);//插入一个指定属性的定时器
void timer_free();//释放已经到达的定时器
void inthandler20(int *esp);//处理函数 
