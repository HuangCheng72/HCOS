; hcos
; TAB=4
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
fin:
		HLT
		JMP		fin
