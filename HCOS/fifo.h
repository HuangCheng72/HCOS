//�������ṹ 
struct FIFO8 {
	unsigned char *buf;
	//p��һ������д��λ�ã�q��һ�����ݶ���λ�ã�size��������С��freeʣ��������flags״̬��ʶ�� 
	int p, q, size, free, flags;
};
//��������ʼ�� 
void fifo8_init(struct FIFO8 *fifo, int size, unsigned char *buf);
//��������д������ ��1�ֽڣ� 
int fifo8_put(struct FIFO8 *fifo, unsigned char data);
//�ӻ������������� ��1�ֽڣ� 
int fifo8_get(struct FIFO8 *fifo);
//��ȡ�����������ݳ��� 
int fifo8_status(struct FIFO8 *fifo);

//�����ʶ�� 
#define FLAGS_OVERRUN		0x0001
