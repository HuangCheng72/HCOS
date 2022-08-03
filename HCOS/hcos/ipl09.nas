; haribote-ipl
; TAB=4

CYLS	EQU		9				; 这个IPL读入十个柱面

		ORG		0x7c00			; 指明程序的装载地址

; 以下用于标准FAT12格式的软盘

		JMP		entry           ; 就是之前的DB 0xeb, 0x4e
		DB		0x90
		DB		"HCIPL   "		; 启动区名称可以是任意八字节的字符串
		DW		512				; 每个扇区（sector）的大小必须为512字节
		DB		1				; 每个簇（cluster）的大小必须为一个扇区
		DW		1				; FAT的起始位置一般从第一个扇区开始
		DB		2				; FAT的个数必须为2
		DW		224				; 根目录的大小一般设为224
		DW		2880			; 磁盘大小（必须是2880扇区，软盘都是这个数字）
		DB		0xf0			; 硬盘的种类必须是0xf0
		DW		9				; FAT的长度必须是9个扇区
		DW		18				; 一个磁道（track）必须有18个扇区
		DW		2				; 磁头数量必须是2
		DD		0				; 不使用分区的话必须是0
		DD		2880			; 重写一次的磁盘大小，也就是上面的磁盘大小
		DB		0,0,0x29		; 意义不明，但是必须这么写，固定值
		DD		0xffffffff		; 似乎是卷标号码
		DB		"HCOS       "	; 磁盘名称，11个字节就行
		DB		"FAT12   "		; 磁盘格式名称，八个字节
		RESB	18				; 先空出18个字节

; 程序核心

; IPL入口
entry:
		MOV		AX,0			; 初始化各个寄存器
		MOV		SS,AX
		MOV		SP,0x7c00
		MOV		DS,AX

; 读硬盘

		MOV		AX,0x0820
		MOV		ES,AX
		MOV		CH,0			; 柱面0，这部分在书P47，里面有向下介绍
		MOV		DH,0			; 磁头0
		MOV		CL,2			; 扇区2
		MOV		BX,18*2*CYLS-1	; 要读取的合计扇区数
		CALL	readfast		; 高速读取

; //读取结束，运行hcos.sys

		MOV		BYTE [0x0ff0],CYLS	; 记录IPL实际读取了多少内容
		JMP		0xc200

error:
		MOV		AX,0
		MOV		ES,AX
		MOV		SI,msg
; 用循环打印信息
putloop:
		MOV		AL,[SI]
		ADD		SI,1			; 给SI寄存器加1
		CMP		AL,0
		JE		fin
		MOV		AH,0x0e			; 显示一个文字
		MOV		BX,15			; 指定字符的颜色
		INT		0x10			; 调用显卡的BIOS
		JMP		putloop
; 程序结束标签，这个必须要有，没有无法编译
fin:
		HLT						; 进去就退出
		JMP		fin				; 无限循环
msg:
		DB		0x0a, 0x0a		; 换行两次
		DB		"load error"    ; 输出的信息
		DB		0x0a			; 换行
		DB		0

readfast:	; 使用AL尽量一次性读取数据
;	ES:读取地址, CH:柱面, DH:磁头, CL:扇区, BX:读取扇区数

		MOV		AX,ES			; 通过ES计算AL的最大值
		SHL		AX,3			; AX除以32，将结果存入AH （SHL是左移位命令）
		AND		AH,0x7f			; AH是AH除以128所得的余数（512*128=64K）
		MOV		AL,128			; AL = 128 - AH;
		SUB		AL,AH

		MOV		AH,BL			; 通过BX计算AL的最大值并存入AH
		CMP		BH,0			; if (BH != 0) { AH = 18; }
		JE		.skip1
		MOV		AH,18
.skip1:
		CMP		AL,AH			; if (AL > AH) { AL = AH; }
		JBE		.skip2
		MOV		AL,AH
.skip2:

		MOV		AH,19			; 通过CL计算AL的最大值并存入AH
		SUB		AH,CL			; AH = 19 - CL;
		CMP		AL,AH			; if (AL > AH) { AL = AH; }
		JBE		.skip3
		MOV		AL,AH
.skip3:

		PUSH	BX
		MOV		SI,0			; 计算失败次数的寄存器
retry:
		MOV		AH,0x02			; AH=0x02 : 读取硬盘
		MOV		BX,0
		MOV		DL,0x00			; A盘
		PUSH	ES
		PUSH	DX
		PUSH	CX
		PUSH	AX
		INT		0x13			; 调用磁盘BIOS
		JNC		next			; 没有出错的话跳转到next
		ADD		SI,1			; 将SI加1
		CMP		SI,5			; 将SI与5比较
		JAE		error			; SI >= 5则跳转到error
		MOV		AH,0x00
		MOV		DL,0x00			; A盘
		INT		0x13			; 驱动器重置
		POP		AX
		POP		CX
		POP		DX
		POP		ES
		JMP		retry
next:
		POP		AX
		POP		CX
		POP		DX
		POP		BX				; 将ES的内容存入BX
		SHR		BX,5			; 将BX由16字节为单位转换为512字节为单位
		MOV		AH,0
		ADD		BX,AX			; BX += AL;
		SHL		BX,5			; 将BX由512字节为单位转换为16字节为单位
		MOV		ES,BX			; 相当于ES += AL * 0x20;
		POP		BX
		SUB		BX,AX
		JZ		.ret
		ADD		CL,AL			; 将CL加上AL
		CMP		CL,18			; 将CL与18比较
		JBE		readfast		; CL <= 18则跳转到readfast
		MOV		CL,1
		ADD		DH,1
		CMP		DH,2
		JB		readfast		; DH < 2则跳转到readfast
		MOV		DH,0
		ADD		CH,1
		JMP		readfast
.ret:
		RET

		RESB	0x7dfe-$		; 一直填写0x00这个值，直到地址达到0x7dfe为止

		DB		0x55, 0xaa
