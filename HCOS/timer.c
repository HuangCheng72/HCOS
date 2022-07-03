#include "bootpack.h"

struct TIMERCTL timerctl;

//��ʼ��PIT
void init_pit(void){
	io_out8(PIT_CTRL, 0x34);
	io_out8(PIT_CNT0, 0x9c);
	io_out8(PIT_CNT0, 0x2e);
	timerctl.count = 0;
	timerctl.size = 0;
	timerctl.timer[0].timeout = 0;//����timer[0]�����ڱ�
	return;
}
//��������Ԫ�أ��ú��������Ⱪ¶
void exchange(int i,int j){
    if(i == j){
        return;
    }
    struct TIMER temp = timerctl.timer[i];
    timerctl.timer[i] = timerctl.timer[j];
    timerctl.timer[j] = temp;
    return;
}
//Ԫ���ϸ����ú��������Ⱪ¶
void swim(int i){
//    int i = k;
    //����±�Ϊi�Ķ�ʱ���趨ʱ��С���±�Ϊi/2�Ķ�ʱ���趨ʱ�̣��ͽ�����ǰ��ȥ
    while(timerctl.timer[i].timeout < timerctl.timer[i/2].timeout){//��Ϊtimer[0]�����ڱ���������timer[1]���ǶѶ��ˣ��������ϸ�
        exchange(i,i/2);
        i /= 2;
    }
    return;
}
//Ԫ���³����ú��������Ⱪ¶
void sink(int i){
//    int i = k;
    //����i����ʱ���趨ʱ�̴���i*2����i*2+1����ʱ���趨ʱ�̣��ͽ�������С���Ǹ�λ��
    //ע��ѵĹ�ģ
    while(2 * i <= timerctl.size){
        //�Ƚ�i*2��i*2+1�ĸ���С
        int j = 2 * i;
        if(j < timerctl.size && timerctl.timer[j+1].timeout < timerctl.timer[j].timeout){
            j++;
        }
        if(timerctl.timer[i].timeout < timerctl.timer[j].timeout){
            //i����ʱ���趨ʱ���Ѿ�������������С����һ����ҪС�ˣ�˵������С�������ʣ��˳�
            return;
        }
        //������С�������ʣ��ͽ���
        exchange(i,j);
        //����j��ֵ
        i = j;
    }
    return;
}
//����һ����ʱ�������ø������ԣ�����ɹ������±꣬
int timer_insert(unsigned int timeout, unsigned char data, struct FIFO8 *fifo){
	if(timerctl.size == MAX_TIMER){
	    //�����ʱ�������Ѿ��ﵽ����
        return -1; //����ʧ��
	}
	//û�е������޾Ͳ��뵽size+1λ��
	timerctl.size++;
	timerctl.timer[timerctl.size].data = data;
	timerctl.timer[timerctl.size].fifo = fifo;
	timerctl.timer[timerctl.size].timeout = timeout;
	//�ϸ�������
	swim(timerctl.size);
	//�����±�
	return timerctl.size;
}
//�ͷŶѶ��Ķ�ʱ��
void timer_free(){
    //���ж��ܲ����ͷ�
    if(timerctl.size == 0){
        return;
    }
    //�����ͷţ�������ѵ�Ԫ�ؽ������������ѹ�ģ
    exchange(1,timerctl.size);
    timerctl.size--;
    //�³�������
    if(timerctl.size > 1){
        sink(1);
    }
	return;
}

void inthandler20(int *esp){
	io_out8(PIC0_OCW2, 0x60);	/* ��IRQ-00�źŽ��ܽ�������Ϣ֪ͨ��PIC */
	timerctl.count++;//��ʱ����
	//�ж���û�ж�ʱ��������еĻ����Ѷ���ʱ���Ƿ��Ѿ���ʱ 
	while(timerctl.size > 0 && timerctl.timer[1].timeout <= timerctl.count){
	    //�����������
        fifo8_put(timerctl.timer[1].fifo, timerctl.timer[1].data);
        //�ͷŶѶ���ʱ��
        timer_free();
	}
	return;
}
