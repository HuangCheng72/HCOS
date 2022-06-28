; hcos
; TAB=4

BOTPAK	EQU		0x00280000		; bootpack的存放位置
DSKCAC	EQU		0x00100000		; ディスクキャッシュの場所
DSKCAC0	EQU		0x00008000		; ディスクキャッシュの場所（リアルモード）

; BOOT_INFO，这部分是我们之前所作的部分
CYLS    EQU     0X0ff0          ; 设定启动区
LEDS    EQU     0x0ff1
VMODE   EQU     0x0ff2          ; 关于颜色数目的信息，颜色的位数
SCRNX   EQU     0x0ff4          ; 分辨率的x（screen x）
SCRNY   EQU     0x0ff6          ; 分辨率的y
VRAM    EQU     0x0ff8          ; 图像缓冲区的开始地址

		ORG		0xc200			; 这个程序将被放到内存中的0xc200位置，怎么算出来这个位置就看书P55
        MOV     AL,0x13         ; VGA显卡，320×200×8位色彩
        MOV     AH,0x00
        INT     0x10
        MOV     BYTE [VMODE],8  ; 记录画面模式
        MOV     WORD [SCRNX],320; 设置分辨率
        MOV     WORD [SCRNY],200
        MOV     DWORD [VRAM],0x000a0000
        
; 用BIOS取得键盘上各种LED指示灯的状态
        MOV     AH,0x02
        INT     0x16            ;键盘BIOS
        MOV     [LEDS],AL

; PIC关闭一切中断
;	根据AT兼容机的规格，如果要初始化PIC，必须在CLT之前进行，否则有时会挂起。这段之后随后进行PIC的初始化
;	这段代码等效于C语言的
;	io_out(PIC0_IMR, 0xff); //禁止主PIC的全部中断
;	io_out(PIC1_IMR, 0xff); //禁止从PIC的全部中断
;	io_cli(); //禁止CPU级别的中断

		MOV		AL,0xff
		OUT		0x21,AL
		NOP						; NOP就是让CPU休眠一个时钟，如果连续执行OUT指令，有些计算机无法正常运行，因此需休眠
		OUT		0xa1,AL

		CLI						; 禁止CPU级别的中断

; 这是为了让CPU能够访问1MB以上的内存空间，设定A20GATE
; 这段程序与init_keyboard完全相同，其功能仅仅是往键盘控制电路附属端口发送指令，令其输出0xdf。
; 输出这个所要完成的功能，是让A20GATE信号线变成ON状态，才可以使用1MB以上的内存
; 相当于以下的C语言程序
; 	//A20GATE的设定
;	wait_KBC_sendready();
;	io_out8(PORT_KEYCMD, 0xd1);
;	wait_KBC_sendready();
;	io_out8(PORT_KEYDAT, 0xdf);
;	wait_KBC_sendready(); //这是为了等待指令完成

		CALL	waitkbdout		; 等同于wait_KBC_sendready
		MOV		AL,0xd1
		OUT		0x64,AL
		CALL	waitkbdout
		MOV		AL,0xdf			; enable A20
		OUT		0x60,AL
		CALL	waitkbdout

; 切换到保护模式

[INSTRSET "i486p"]				; “需要使用486指令”的意思，是为了能够使用386以后的LGDT，EAX，CR0等关键字

		LGDT	[GDTR0]			; 设定临时GDT（因为以后要重新设置）
		MOV		EAX,CR0			; 将CRO（control register 0）寄存器的值带入EAX，这个寄存器只有操作系统才能使用
		AND		EAX,0x7fffffff	; 将bit31设置为0（为了禁止什么东西“原话是：颁”没搞明白）
		OR		EAX,0x00000001	; 将bit0设置为1（为了切换到保护模式）
		MOV		CR0,EAX
		JMP		pipelineflush	; 带入CR0切换到保护模式的时候要马上执行JMP指令（这是因为模式变了需要重新解释）
pipelineflush:
		MOV		AX,1*8			; 可读写的段 32bit
		MOV		DS,AX
		MOV		ES,AX
		MOV		FS,AX
		MOV		GS,AX
		MOV		SS,AX

; bootpack的转送

		MOV		ESI,bootpack	; 转送源（从哪开始）
		MOV		EDI,BOTPAK		; 转送目的地（送到哪里）
		MOV		ECX,512*1024/4
		CALL	memcpy

; 磁盘数据最终转送到它本来的地方去

; 首先从启动扇区开始

		MOV		ESI,0x7c00		; 转送源（从哪开始）
		MOV		EDI,DSKCAC		; 转送目的地（送到哪里）
		MOV		ECX,512/4
		CALL	memcpy

; 所有剩下的

		MOV		ESI,DSKCAC0+512	; 转送源（从哪开始）
		MOV		EDI,DSKCAC+512	; 转送目的地（送到哪里）
		MOV		ECX,0
		MOV		CL,BYTE [CYLS]
		IMUL	ECX,512*18*2/4	; 从柱面数变换字节数/4
		SUB		ECX,512/4		; 减去IPL
		CALL	memcpy

; 上面一段传输，写成C语言大意就是这样
; 这里复制数据的大小是以柱面数来计算的，所以需要减去启动区那一部分的长度
; memcpy(bootpack, BOTPAK, 512*1024/4); //把bootpack开始的内存空间的数据复制到BOTPAK这个空间，复制长度为512*1024/4
; memcpy(0x7c00, DSKCAC, 512/4);
; memcpy(DSKCAC0+512, DSKCAC+512, 512*18*2/4 - 512/4);

; 必须要有asmhead完成的工作到此完毕
;	此后就交给bootpack来完成

; bootpack启动

		MOV		EBX,BOTPAK
		MOV		ECX,[EBX+16]
		ADD		ECX,3			; ECX += 3;
		SHR		ECX,2			; ECX /= 4;
		JZ		skip			; 没有要转送的数据，就跳到skip
		MOV		ESI,[EBX+20]	; 转送源（从哪开始）
		ADD		ESI,EBX
		MOV		EDI,[EBX+12]	; 转送目的地（送到哪里）
		CALL	memcpy
skip:
		MOV		ESP,[EBX+12]	; 栈初始值
		JMP		DWORD 2*8:0x0000001b

waitkbdout:
		IN		 AL,0x64
		AND		 AL,0x02
		JNZ		waitkbdout		; AND的结果如果不是0就跳到waitkbdout
		RET

memcpy:
		MOV		EAX,[ESI]
		ADD		ESI,4
		MOV		[EDI],EAX
		ADD		EDI,4
		SUB		ECX,1
		JNZ		memcpy			; 减法运算的结果如果不是0，就跳到memcpy继续复制
		RET
; memcpyはアドレスサイズプリフィクスを入れ忘れなければ、ストリング命令でも書ける

		ALIGNB	16				; 一直添加DBO，直到地址能被16整除（估计就是十六进制地址操作的要求）
		; 如果GDT0的地址不是8的整数倍，向段寄存器赋值的MOV指令就会变慢（需要大量计算），所以以上操作就是为了提高速度
GDT0:							; GDT0是一种特殊的GDT，0是空区域不能在那里定义段，其实就是因为已经把这个作为临时段了
		RESB	8				; NULL selector
		DW		0xffff,0x0000,0x9200,0x00cf	; 可以读写的段（segment）32bit，这些地址都是算好的，P160页
		DW		0xffff,0x0000,0x9a28,0x0047	; 可以执行的段（segment）32bit（bootpack用）

		DW		0
GDTR0:	; LGDT指令，意思是通知GDT0中已存在GDT
		DW		8*3-1
		DD		GDT0

		ALIGNB	16
bootpack:
