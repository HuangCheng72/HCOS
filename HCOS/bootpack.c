//��������
void io_hlt(void);
void write_mem8(int addr,int data);

void HariMain(void){
    
    int i;
    char *p; //��Ϊ�ڻ����������ָ���ĵ�ַָ����ڴ�ռ���BYTE�ͣ���ӦC�����о���char�͵�ַ 
    //��������ͼ�� 
    for(i = 0xa0000; i <= 0xaffff; i++){
        //write_mem8(i, i & 0x0f);
        
        p = (char*)i; //ֱ�ӽ�iת��Ϊchar��ָ��
		*p = i & 0x0f; //ֱ�Ӷ������ַ����д������ 
    }
    
    //�������˵һֱ��ѭ��Ȼ�����жϣ���֮ǰ�ж�֮����goto finһ����Ч��
    while(1){
        io_hlt();
    }
}

