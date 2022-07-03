#define PIT_CTRL 0x0043
#define PIT_CNT0 0x0040

#define MAX_TIMER		500
struct TIMER {
	unsigned int timeout;
	struct FIFO8 *fifo;
	unsigned char data;
};
struct TIMERCTL {
    //countΪ��ʱ����size�����м�ʱ����Ŀ
	unsigned int count, size;
	//����С����˼���д���±��1��ʼ�ȽϺü��㣩��������timer[0]�������ڱ�
	//��timeoutΪ�����ֵ
	struct TIMER timer[MAX_TIMER + 1];
};
extern struct TIMERCTL timerctl;
void init_pit(void);//��ʼ��PIT
int timer_insert(unsigned int timeout, unsigned char data, struct FIFO8 *fifo);//����һ��ָ�����ԵĶ�ʱ��
void timer_free();//�ͷ��Ѿ�����Ķ�ʱ��
void inthandler20(int *esp);//������ 
