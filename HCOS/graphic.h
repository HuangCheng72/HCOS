//naskfunc.nas�и����ĺ���
#include "naskfunction.h"

//�����ǻ���GUI������Ҫ�õ��ĺ��� 
void init_palette(void); //��ʼ����ɫ�� 
void set_palette(int start, int end, unsigned char *rgb); //���õ�ɫ����ɫ 
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1); //��ĳһ����ȾɫҲ���ǻ��ƾ��Σ�xsize����Ļ����Ĵ�С 
void init_screen(char *vram, int x, int y); //��ʼ�����о��Σ�Ҳ���ǻ�ͼ���� 
void putfont8(char *vram, int xsize, int x, int y, char c, char *font); //���� 8*16�ĵ���ͼ������������ʾ�ַ� 
void putfont16(char *vram, int xsize, int x, int y, char c, char *font); //���� 16*16�ĵ���ͼ������������ʾ���� 
void putfonts8_asc(char *vram, int xsize, int x, int y, char c, unsigned char *s);// ��ӡ����ַ��� 
void init_mouse_cursor8(char *mouse, char bc); //��ʼ�����ͼƬ��bc�Ǳ�����ɫ 
void putblock8_8(char *vram, int vxsize, int pxsize, int pysize, int px0, int py0, char *buf, int bxsize); //��buf�е����鸴�Ƶ�vram��ȥ 

//��������ɫ���� 

#define COL8_000000		0
#define COL8_FF0000		1
#define COL8_00FF00		2
#define COL8_FFFF00		3
#define COL8_0000FF		4
#define COL8_FF00FF		5
#define COL8_00FFFF		6
#define COL8_FFFFFF		7
#define COL8_C6C6C6		8
#define COL8_840000		9
#define COL8_008400		10
#define COL8_848400		11
#define COL8_000084		12
#define COL8_840084		13
#define COL8_008484		14
#define COL8_848484		15

#define ADR_BOOTINFO	0x00000ff0


//BOOTINFO�ṹ�壬��ṹ��asmhead.nas���Ѿ���������
//��ʵvramӦ����һ����ά���飬�������ڴ�����һά�������ʽ�������д�����ʱ�����Ǳ����ֶ����� 
//vram���Ǵ洢������Ļ����ʾ������ɫ����Ϣ���������� 
struct BOOTINFO {
	char cyls, leds, vmode, reserve;
	short scrnx, scrny;
	char *vram;
}; 
