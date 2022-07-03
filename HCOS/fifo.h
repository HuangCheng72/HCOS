//缓冲区结构

struct FIFO32 {
    int *buf;
    //p下一个数据写入位置，q下一个数据读出位置，size缓冲区大小，free剩余容量，flags状态标识符
    int p, q, size, free, flags;
};
//缓冲区初始化
void fifo32_init(struct FIFO32 *fifo, int size, int *buf);
//往缓冲区写入数据 （1字节）
int fifo32_put(struct FIFO32 *fifo, int data);
//从缓冲区读出数据 （1字节）
int fifo32_get(struct FIFO32 *fifo);
//获取缓冲区中数据长度
int fifo32_status(struct FIFO32 *fifo);

//溢出标识符 
#define FLAGS_OVERRUN		0x0001
