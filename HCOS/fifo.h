//�������ṹ

struct FIFO32 {
    int *buf;
    //p��һ������д��λ�ã�q��һ�����ݶ���λ�ã�size��������С��freeʣ��������flags״̬��ʶ��
    int p, q, size, free, flags;
    struct TASK* task;	//�����Զ�����������������Զ�����������Ϊ0���� 
};
//��������ʼ��
void fifo32_init(struct FIFO32 *fifo, int size, int* buf, struct TASK *task);
//��������д������ ��4�ֽڣ�
int fifo32_put(struct FIFO32 *fifo, int data);
//�ӻ������������� ��4�ֽڣ�
int fifo32_get(struct FIFO32 *fifo);
//��ȡ�����������ݳ���
int fifo32_status(struct FIFO32 *fifo);

//�����ʶ�� 
#define FLAGS_OVERRUN		0x0001
