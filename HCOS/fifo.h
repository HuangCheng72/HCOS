//缓冲区结构 
struct FIFO8 {
	unsigned char *buf;
	//p下一个数据写入位置，q下一个数据读出位置，size缓冲区大小，free剩余容量，flags状态标识符 
	int p, q, size, free, flags;
};
//缓冲区初始化 
void fifo8_init(struct FIFO8 *fifo, int size, unsigned char *buf);
//往缓冲区写入数据 （1字节） 
int fifo8_put(struct FIFO8 *fifo, unsigned char data);
//从缓冲区读出数据 （1字节） 
int fifo8_get(struct FIFO8 *fifo);
//获取缓冲区中数据长度 
int fifo8_status(struct FIFO8 *fifo);

//溢出标识符 
#define FLAGS_OVERRUN		0x0001
