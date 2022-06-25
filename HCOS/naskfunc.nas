; naskfunc
; TAB=4

[FORMAT "WCOFF"]				; 制作目标文件的模式
[INSTRSET "i486p"]				; 说明这个程序是给486处理器用的（32位）
[BITS 32]						; 制作32位模式用的机器语言


; 制作目标文件的信息

[FILE "naskfunc.nas"]			; 源文件名信息

		GLOBAL	_io_hlt,_write_mem8			; 暴露程序中包含的函数名


; 以下是实际的函数

[SECTION .text]		; 目标文件中写了这些之后再写程序

_io_hlt:	; void io_hlt(void);
		HLT
		RET
_write_mem8:	; void write_mem8(int addr, int data); 这里的8不知道是什么意思，一个int的长度是4，这里大概就是直接在函数的内存空间里面取参数，然后进行操作，以此类推，第一个参数是[ESP + 4]，第二个参数是[ESP + 8]，第三个参数是[ESP + 12]，如此下去具体的值自己算
        MOV		ECX,[ESP+4]     ; [ESP + 4]中存放的是地址，将这个地址读入ECX寄存器
        MOV		AL,[ESP+8]      ; [ESP + 8]中存放的是数据，将其读入AL寄存器
        MOV     [ECX],AL        ; 将AL的数据写入到ECX所在的内存空间
        RET
