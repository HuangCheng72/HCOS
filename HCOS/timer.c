#include "bootpack.h"

struct TIMERCTL timerctl;
//��������Ԫ�أ��ú��������Ⱪ¶
void exchange(int i,int j){
    if(i == j){
        return;
    }
    //����ָ��Ԫ��
    struct TIMER* temp = timerctl.ptr_timer[i];
    timerctl.ptr_timer[i] = timerctl.ptr_timer[j];
    timerctl.ptr_timer[j] = temp;
    //�����±�
    timerctl.ptr_timer[i]->index = i;
    timerctl.ptr_timer[j]->index = j;
    return;
}
//Ԫ���ϸ����������յ�λ�ã��ú��������Ⱪ¶
void swim(int i){
    //����±�Ϊi�Ķ�ʱ���趨ʱ��С���±�Ϊi/2�Ķ�ʱ���趨ʱ�̣��ͽ�����ǰ��ȥ
    while(timerctl.ptr_timer[i]->timeout < timerctl.ptr_timer[i/2]->timeout){//��Ϊtimer[0]�����ڱ���������timer[1]���ǶѶ��ˣ��������ϸ�
        exchange(i,i/2);
        i /= 2;
    }
    return;
}
//Ԫ���³����ú��������Ⱪ¶
void sink(int i){
    //����i����ʱ���趨ʱ�̴���i*2����i*2+1����ʱ���趨ʱ�̣��ͽ�������С���Ǹ�λ��
    //ע��ѵĹ�ģ
    while(2 * i <= timerctl.size){
        //�Ƚ�i*2��i*2+1�ĸ���С
        int j = 2 * i;
        if(j < timerctl.size && timerctl.ptr_timer[j+1]->timeout < timerctl.ptr_timer[j]->timeout){
            j++;
        }
        if(timerctl.ptr_timer[i]->timeout < timerctl.ptr_timer[j]->timeout){
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
//��ʼ��PIT
void init_pit(void){
	io_out8(PIT_CTRL, 0x34);
	io_out8(PIT_CNT0, 0x9c);
	io_out8(PIT_CNT0, 0x2e);
	timerctl.count = 0;
    timerctl.size = 0;
    timerctl.total = 0;
    timerctl.timer[0].timeout = 0;//����timer[0]�����ڱ�
    //ָ��ָ�������ڴ�ռ�
    //�±�Ҳ����Ϊ��ָ�������е�λ��
    int i;
    for(i = 0; i <= MAX_TIMER; i++){
        timerctl.ptr_timer[i] = &timerctl.timer[i];
        timerctl.ptr_timer[i]->index = i;
    }
    return;
}
//����һ����ʱ��
struct TIMER *timer_alloc(void){
    if(timerctl.total == MAX_TIMER){
        //�����ʱ�������Ѿ��ﵽ����
        return 0; //����ʧ�ܣ�����nullָ��
    }
    //û�е������޾Ͳ��뵽total+1λ�ã����Է������ָ��
    return timerctl.ptr_timer[++timerctl.total];
}
//�ͷ�һ����ʱ��
void timer_free(struct TIMER *timer){
    //���ж��ܲ����ͷ�
    //��û���������еĶ�ʱ�������ǲ��������ж�ʱ��֮��
    if(timerctl.size == 0 || timerctl.total < timer->index){
        return;
    }
    //�����Ķ�ʱ���Ƿ��Ѿ��ڶ���
    if(timerctl.size < timer->index){
        //���ڶ��еĻ�����Ҫ���ǲ���
        return;
    } else {
        //�ڶ���
        int e;
		e = io_load_eflags();
		io_cli(); //�������ã���ֹ�ж� 
        //���Ȱ����ָ�뽻������ĩβ��Ȼ������С�ѵĹ�ģ
        int index = timer->index;
        exchange(index,timerctl.size);
        timerctl.size--;
        //Ȼ�����˳���³���
        sink(1);
        //�ϸ�����
        swim(timerctl.size);
        io_store_eflags(e);
    }
    return;
}
//��ʼ��һ����ʱ��������
void timer_init(struct TIMER *timer, struct FIFO32 *fifo, int data){
    timer->fifo = fifo;
    timer->data = data;
    return;
}
//���ö�ʱ�����趨ʱ��
void timer_settime(struct TIMER *timer, unsigned int timeout){
    //��������ж�ʱ��֮�⣬�˳�
    if(timer->index > timerctl.total){
        return;
    }
    int e;
	e = io_load_eflags();
	io_cli(); //�������ã���ֹ�ж� 
    //����ڶ��⣬��Ҫ���Ƚ���
    if(timer->index > timerctl.size){
        //����ѵķ�Χ
        timerctl.size++;
        //��λ�ý������ѵ���ĩβ
        exchange(timer->index , timerctl.size);
    }
    //Ȼ��������ʱ��
    timer->timeout = timeout;
    //�ϸ��³�������
    swim(1);
    sink(timerctl.size);
    io_store_eflags(e);
    return;
}
//ɾ����һ����ʱ�� 
int timer_cancel(struct TIMER *timer){
	if(timerctl.total < timer->index){
		//��ʱ���������Ѿ�����Ķ�ʱ��֮һ���ͷ���
		return 1; 
	}
    int e;
	e = io_load_eflags();
	io_cli(); //�������ã���ֹ�ж� 
	//�����Ķ�ʱ���Ƿ��Ѿ��ڶ���
    if(timerctl.size >= timer->index){
        //�ڶ��У������ͷ� 
        timer_free(timer); 
    }
    //�ٰ�ָ��ֱ�ӽ��������ж�ʱ�����Ȼ���������ж�ʱ���Ĺ�ģ 
    exchange(timer->index,timerctl.total);
    timerctl.total--;
    io_store_eflags(e);
    return 1;
}
void inthandler20(int *esp){
	char ts = 0;//�л������ʶ�� 
	io_out8(PIC0_OCW2, 0x60);	//��IRQ-00�źŽ��ܽ�������Ϣ֪ͨ��PIC
	timerctl.count++;//��ʱ����
	//�ж���û�ж�ʱ��������еĻ����Ѷ���ʱ���Ƿ��Ѿ���ʱ 
	while(timerctl.size > 0 && timerctl.ptr_timer[1]->timeout <= timerctl.count){
		//����ж�ʱ���ж�һ�¶Ѷ��ǲ��Ƕ�����Ķ�ʱ��
		if (timerctl.ptr_timer[1] == task_timer) {
			//����ǣ����޸ı�ʶ��
			ts = 1;
		}else{
			//�������
			//�����������
        	fifo32_put(timerctl.ptr_timer[1]->fifo, timerctl.ptr_timer[1]->data);
		}
		//�ͷŶѶ���ʱ��
        timer_free(timerctl.ptr_timer[1]);
	}
	if(ts != 0){
		//��ʶ��Ϊ�棬�л����� 
		task_switch();
	}
	return;
}

void timer_cancelall(struct FIFO32 *fifo){
	int e, i;
	struct TIMER *t;
	e = io_load_eflags();
	io_cli();	//��ֹ�ж� 
	for (i = 1; i <= timerctl.total; i++) {
		t = timerctl.ptr_timer[i];
		if (t->fifo == fifo) {
			timer_cancel(t);
		}
	}
	io_store_eflags(e);
	return;
}
