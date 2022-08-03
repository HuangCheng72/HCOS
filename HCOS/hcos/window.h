
//绘制窗口
void make_window8(unsigned char *buf, int xsize, int ysize, char *title, char act);
//整合了重新上色部分区域，打印字符串，刷新图层三个函数
void putfonts8_asc_sht(struct SHEET *sht, int x, int y, int c, int b, char *s, int l);
//绘制文本框 
void make_textbox8(struct SHEET *sht, int x0, int y0, int sx, int sy, int c);
//绘制窗口部分工作 
void make_wtitle8(unsigned char *buf, int xsize, char *title, char act);
//改变窗口标题
void change_wtitle8(struct SHEET *sht, char act);
