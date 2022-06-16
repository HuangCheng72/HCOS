MAKE     = make.exe -r
NASK     = nask.exe
EDIMG    = edimg.exe
COPY     = copy
DEL      = del

#默认动作

default :
	$(MAKE) img

#文件生成规则

ipl.bin : ipl.nas Makefile
	$(NASK) ipl.nas ipl.bin ipl.lst

hcos.sys : hcos.nas Makefile
	$(NASK) hcos.nas hcos.sys hcos.lst

hcos.img : ipl.bin hcos.sys Makefile
	$(EDIMG)   imgin:fdimg0at.tek \
		wbinimg src:ipl.bin len:512 from:0 to:0  copy from:hcos.sys to:@: imgout:hcos.img

# 常用命令行

img :
	$(MAKE) hcos.img

clean :
	-$(DEL) ipl.bin
	-$(DEL) ipl.lst
	-$(DEL) hcos.sys
	-$(DEL) hcos.lst

src_only :
	$(MAKE) clean
	-$(DEL) hcos.img
