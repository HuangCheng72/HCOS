//函数声明
void io_hlt(void);
void write_mem8(int addr,int data);

void HariMain(void){
    
    int i;
    char *p; //因为在汇编里面我们指定的地址指向的内存空间是BYTE型，对应C语言中就是char型地址 
    //绘制条纹图案 
    for(i = 0xa0000; i <= 0xaffff; i++){
        //write_mem8(i, i & 0x0f);
        
        p = (char*)i; //直接将i转化为char型指针
		*p = i & 0x0f; //直接对这个地址进行写入数据 
    }
    
    //这个就是说一直死循环然后又中断，和之前中断之后又goto fin一样的效果
    while(1){
        io_hlt();
    }
}

