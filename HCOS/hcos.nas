; hcos
; TAB=4

		ORG		0xc200			; 这个程序将被放到内存中的0xc200位置，怎么算出来这个位置就看书P55
fin:
		HLT
		JMP		fin
