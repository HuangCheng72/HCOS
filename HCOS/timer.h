#define PIT_CTRL 0x0043
#define PIT_CNT0 0x0040
void init_pit(void);
void inthandler20(int *esp);
struct TIMERCTL{
	unsigned int count;
}; 
