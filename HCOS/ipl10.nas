; hcos-ipl
; TAB=4

CYLS	EQU		10				; 这个IPL只能读入十个柱面

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
; 重置读取失败的寄存器的值为0
readloop:
		MOV		SI,0			; 记录失败次数的寄存器为SI，赋值为0
; 读入硬盘内容
retry:
		MOV		AH,0x02			; AH=0x02 : BIOS的2号函数，读盘
		MOV		AL,1			; 1个扇区
		MOV		BX,0
		MOV		DL,0x00			; A驱动器
		INT		0x13			; 调用磁盘BIOS
		JNC		next			; 没出错的话就跳到next
		ADD		SI,1			; 出错了就SI+1
		CMP		SI,5			; 比较SI和5
		JAE		error			; SI >= 5就直接跳到error
		MOV		AH,0x00
		MOV		DL,0x00			; A驱动器
		INT		0x13			; 调用BIOS，用来重置驱动器
		JMP		retry
next:
		MOV		AX,ES			; 将内存地址后移0x200
		ADD		AX,0x0020
		MOV		ES,AX			; 这里没有ADD ES,0x020这个指令，所以要稍微绕一下弯才能完成后移动作
		ADD		CL,1			; CL+1
		CMP		CL,18			; 比较CL和18
		JBE		readloop		; 若CL <= 18则跳转到readloop
		MOV		CL,1
		ADD		DH,1
		CMP		DH,2
		JB		readloop		; 若DH < 2则跳转到readloop
		MOV		DH,0
		ADD		CH,1
		CMP		CH,CYLS         ; 比较当前柱面
		JB		readloop		; 若CH < CYLS，也就是还没到达指定的柱面10，则跳转到readloop，继续往下读

; 进入我们的系统，跳转到hcos.sys所在位置
        MOV		[0x0ff0],CH     ; 记录IPL读取的进度
		JMP		0xc200

; 把报错信息放到寄存器
error:
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
; 输出各个字节的信息
msg:
		DB		0x0a, 0x0a		; 换行两次
		DB		"load error"    ; 输出的信息
		DB		0x0a			; 换行
		DB		0

		RESB	0x7dfe-$		; 一直填写0x00这个值，直到地址达到0x001fe为止

		DB		0x55, 0xaa
