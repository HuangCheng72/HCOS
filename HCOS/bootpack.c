//函数声明
void io_hlt(void);

void HariMain(void){
fin:
	/* 这里本来要写HLT，但是现在C语言里面还没有HLT*/
    io_hlt(); //这里就是执行naskfunc.nas里面的io_hlt函数
	goto fin;
}
