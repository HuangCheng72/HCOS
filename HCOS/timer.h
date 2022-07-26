#define PIT_CTRL 0x0043
#define PIT_CNT0 0x0040

#define MAX_TIMER		500
struct TIMER {
    //��ʱʱ��
    unsigned int timeout;
    //���ڶ����±꣨������±�����ָ������ptr_timer������±꣩
    int index;
    //�󶨵Ļ�����������
    struct FIFO32 *fifo;
    int data;
};
struct TIMERCTL {
    //countΪ��ʱ����size���������趨ʱ�̵ļ�ʱ����Ŀ�����ѹ�ģ����total�ǵ�ǰ�Ѿ��������ȥ�Ķ�ʱ����Ŀ
    unsigned int count, size, total;
    //����С����˼���д���±��1��ʼ�ȽϺü��㣩��������timer[0]�������ڱ�
    //��timeoutΪ�����ֵ
    struct TIMER timer[MAX_TIMER + 1];
    //ָ������
    struct TIMER* ptr_timer[MAX_TIMER + 1];
};
extern struct TIMERCTL timerctl;
void init_pit(void);//��ʼ��PIT
struct TIMER *timer_alloc(void); //����һ����ʱ��
void timer_free(struct TIMER *timer); //�ͷ�һ����ʱ��
void timer_init(struct TIMER *timer, struct FIFO32 *fifo, int data); //��ʼ��һ����ʱ��������
void timer_settime(struct TIMER *timer, unsigned int timeout); //���ö�ʱ�����趨ʱ��
int timer_cancel(struct TIMER *timer); //ɾ����alloc������һ����ʱ�� 
void inthandler20(int *esp);//������
