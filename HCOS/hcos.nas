; 这个系统的名字是hcos
; TAB宽度为4

; 下面这段是标准的FAT12格式软盘专用的代码

		DB		0xeb, 0x4e, 0x90
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

; 程序本体

		DB		0xb8, 0x00, 0x00, 0x8e, 0xd0, 0xbc, 0x00, 0x7c
		DB		0x8e, 0xd8, 0x8e, 0xc0, 0xbe, 0x74, 0x7c, 0x8a
		DB		0x04, 0x83, 0xc6, 0x01, 0x3c, 0x00, 0x74, 0x09
		DB		0xb4, 0x0e, 0xbb, 0x0f, 0x00, 0xcd, 0x10, 0xeb
		DB		0xee, 0xf4, 0xeb, 0xfd

; 信息显示部分

		DB		0x0a, 0x0a		; 两个换行
		DB		"hello, this is hcos!"
		DB		0x0a			; 换行
		DB		0

		RESB	0x1fe-$			; 一直填写0x00这个值，直到地址达到0x001fe为止

		DB		0x55, 0xaa

; 以下是启动区以外部分的输出，目前不知道是用来干什么的

		DB		0xf0, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00
		RESB	4600
		DB		0xf0, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00
		RESB	1469432
