; 这个系统的名字是hcos
; TAB宽度为4

		ORG		0x7c00			; 指明程序的装载地址

; 以下用于标准FAT12格式的软盘

		JMP		entry           ; 就是之前的DB 0xeb, 0x4e
		DB		0x90
		DB		"HCIPL   "		; 启动区名称可以是任意八字节的字符串
		DW		512				; 每个扇区（sector）的大小必须为512字节
		DB		1				; 每个簇（cluster）的大小必须为一个扇区
		DW		1				; FAT的起始位置一般从第一个扇区开始
		DB		2				; FAT的个数必须为2
		DW		224				; 根目录的大小一般设为22项
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

entry:
		MOV		AX,0			; 初始化各个寄存器
		MOV		SS,AX
		MOV		SP,0x7c00
		MOV		DS,AX
		MOV		ES,AX

		MOV		SI,msg          ; 把msg的地址放到SI寄存器
putloop:
		MOV		AL,[SI]
		ADD		SI,1			; 给SI寄存器加1
		CMP		AL,0
		JE		fin
		MOV		AH,0x0e			; 显示一个文字
		MOV		BX,15			; 指定字符的颜色
		INT		0x10			; 调用显卡的BIOS
		JMP		putloop
fin:
		HLT						; 让CPU停止
		JMP		fin				; 再跳回去，死循环，这样实现系统一直运行又不会把CPU烧了

msg:
		DB		0x0a, 0x0a		; 换行两次
		DB		"hello, world"
		DB		0x0a			; 换行
		DB		0

		RESB	0x7dfe-$		; 一直填写0x00这个值，直到地址达到0x001fe为止

		DB		0x55, 0xaa
