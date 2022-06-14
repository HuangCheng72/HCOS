
# 默认动作

default :
	make.exe img

# 文件生成规则

ipl.bin : ipl.nas Makefile
	nask.exe ipl.nas ipl.bin ipl.lst

hcos.img : ipl.bin Makefile
	edimg.exe   imgin:fdimg0at.tek \
		wbinimg src:ipl.bin len:512 from:0 to:0   imgout:hcos.img

# 常用命令行，我们用的是虚拟机，不能直接run

asm :
	make.exe -r ipl.bin

img :
	make.exe -r hcos.img

#run :
#	../z_tools/make.exe img
#	copy helloos.img ..\z_tools\qemu\fdimage0.bin
#	../z_tools/make.exe -C ../z_tools/qemu

#install :
#	../z_tools/make.exe img
#	../z_tools/imgtol.com w a: helloos.img

clean :
	-del ipl.bin
	-del ipl.lst

src_only :
	make.exe clean
	-del hcos.img
