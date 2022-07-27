OBJS_BOOTPACK = bootpack.obj naskfunc.obj hankaku.obj graphic.obj \
                dsctbl.obj int.obj fifo.obj InputDevice.obj memory.obj \
                sheet.obj timer.obj rbtree.obj mtask.obj file.obj window.obj \
                console.obj

TOOLPATH = tools/
INCPATH  = tools/hcos01/

MAKE     = $(TOOLPATH)make.exe -r
NASK     = $(TOOLPATH)nask.exe
CC1      = $(TOOLPATH)cc1.exe -I$(INCPATH) -Os -Wall -quiet
GAS2NASK = $(TOOLPATH)gas2nask.exe -a
OBJ2BIM  = $(TOOLPATH)obj2bim.exe
MAKEFONT = $(TOOLPATH)makefont.exe
BIN2OBJ  = $(TOOLPATH)bin2obj.exe
BIM2HRB  = $(TOOLPATH)bim2hrb.exe
RULEFILE = $(TOOLPATH)hcos01/hcos01.rul
EDIMG    = $(TOOLPATH)edimg.exe
IMGTOL   = $(TOOLPATH)imgtol.com
COPY     = copy
DEL      = del

#默认动作

default :
	$(MAKE) img

#文件生成特殊规则

ipl10.bin : ipl10.nas Makefile
	$(NASK) ipl10.nas ipl10.bin ipl10.lst

asmhead.bin : asmhead.nas Makefile
	$(NASK) asmhead.nas asmhead.bin asmhead.lst

hankaku.bin : hankaku.txt Makefile
	$(MAKEFONT) hankaku.txt hankaku.bin

hankaku.obj : hankaku.bin Makefile
	$(BIN2OBJ) hankaku.bin hankaku.obj _hankaku

bootpack.bim : $(OBJS_BOOTPACK) Makefile
	$(OBJ2BIM) @$(RULEFILE) out:bootpack.bim stack:3136k map:bootpack.map \
		$(OBJS_BOOTPACK)
# 3MB+64KB=3136KB

bootpack.hrb : bootpack.bim Makefile
	$(BIM2HRB) bootpack.bim bootpack.hrb 0

hcos.sys : asmhead.bin bootpack.hrb Makefile
	copy /B asmhead.bin+bootpack.hrb hcos.sys
    

hello.hrb : hello.nas Makefile
	$(NASK) hello.nas hello.hrb hello.lst

hello2.hrb : hello2.nas Makefile
	$(NASK) hello2.nas hello2.hrb hello2.lst

a.bim : a.obj a_nask.obj Makefile
	$(OBJ2BIM) @$(RULEFILE) out:a.bim map:a.map a.obj a_nask.obj

a.hrb : a.bim Makefile
	$(BIM2HRB) a.bim a.hrb 0

hello3.bim : hello3.obj a_nask.obj Makefile
	$(OBJ2BIM) @$(RULEFILE) out:hello3.bim map:hello3.map hello3.obj a_nask.obj

hello3.hrb : hello3.bim Makefile
	$(BIM2HRB) hello3.bim hello3.hrb 0

hello4.bim : hello4.obj a_nask.obj Makefile
	$(OBJ2BIM) @$(RULEFILE) out:hello4.bim stack:1k map:hello4.map \
		hello4.obj a_nask.obj

hello4.hrb : hello4.bim Makefile
	$(BIM2HRB) hello4.bim hello4.hrb 0

hello5.bim : hello5.obj Makefile
	$(OBJ2BIM) @$(RULEFILE) out:hello5.bim stack:1k map:hello5.map hello5.obj

hello5.hrb : hello5.bim Makefile
	$(BIM2HRB) hello5.bim hello5.hrb 0

winhelo.bim : winhelo.obj a_nask.obj Makefile
	$(OBJ2BIM) @$(RULEFILE) out:winhelo.bim stack:1k map:winhelo.map \
		winhelo.obj a_nask.obj

winhelo.hrb : winhelo.bim Makefile
	$(BIM2HRB) winhelo.bim winhelo.hrb 0

winhelo2.bim : winhelo2.obj a_nask.obj Makefile
	$(OBJ2BIM) @$(RULEFILE) out:winhelo2.bim stack:1k map:winhelo2.map \
		winhelo2.obj a_nask.obj

winhelo2.hrb : winhelo2.bim Makefile
	$(BIM2HRB) winhelo2.bim winhelo2.hrb 0

winhelo3.bim : winhelo3.obj a_nask.obj Makefile
	$(OBJ2BIM) @$(RULEFILE) out:winhelo3.bim stack:1k map:winhelo3.map \
		winhelo3.obj a_nask.obj

winhelo3.hrb : winhelo3.bim Makefile
	$(BIM2HRB) winhelo3.bim winhelo3.hrb 40k

star1.bim : star1.obj a_nask.obj Makefile
	$(OBJ2BIM) @$(RULEFILE) out:star1.bim stack:1k map:star1.map \
		star1.obj a_nask.obj

star1.hrb : star1.bim Makefile
	$(BIM2HRB) star1.bim star1.hrb 47k

stars.bim : stars.obj a_nask.obj Makefile
	$(OBJ2BIM) @$(RULEFILE) out:stars.bim stack:1k map:stars.map \
		stars.obj a_nask.obj

stars.hrb : stars.bim Makefile
	$(BIM2HRB) stars.bim stars.hrb 47k

stars2.bim : stars2.obj a_nask.obj Makefile
	$(OBJ2BIM) @$(RULEFILE) out:stars2.bim stack:1k map:stars2.map \
		stars2.obj a_nask.obj

stars2.hrb : stars2.bim Makefile
	$(BIM2HRB) stars2.bim stars2.hrb 47k

lines.bim : lines.obj a_nask.obj Makefile
	$(OBJ2BIM) @$(RULEFILE) out:lines.bim stack:1k map:lines.map \
		lines.obj a_nask.obj

lines.hrb : lines.bim Makefile
	$(BIM2HRB) lines.bim lines.hrb 48k

walk.bim : walk.obj a_nask.obj Makefile
	$(OBJ2BIM) @$(RULEFILE) out:walk.bim stack:1k map:walk.map \
		walk.obj a_nask.obj

walk.hrb : walk.bim Makefile
	$(BIM2HRB) walk.bim walk.hrb 48k

hcos.img : ipl10.bin hcos.sys Makefile \
		hello.hrb hello2.hrb a.hrb hello3.hrb hello4.hrb hello5.hrb \
		winhelo.hrb winhelo2.hrb winhelo3.hrb star1.hrb stars.hrb stars2.hrb \
		lines.hrb walk.hrb
	$(EDIMG)   imgin:tools/fdimg0at.tek \
		wbinimg src:ipl10.bin len:512 from:0 to:0 \
        copy from:hcos.sys to:@: \
		copy from:ipl10.nas to:@: \
		copy from:hello.hrb to:@: \
		copy from:hello2.hrb to:@: \
		copy from:a.hrb to:@: \
		copy from:hello3.hrb to:@: \
		copy from:hello4.hrb to:@: \
		copy from:hello5.hrb to:@: \
		copy from:winhelo.hrb to:@: \
		copy from:winhelo2.hrb to:@: \
		copy from:winhelo3.hrb to:@: \
		copy from:star1.hrb to:@: \
		copy from:stars.hrb to:@: \
		copy from:stars2.hrb to:@: \
		copy from:lines.hrb to:@: \
		copy from:walk.hrb to:@: \
		imgout:hcos.img

# 一般规则
%.gas : %.c Makefile
	$(CC1) -o $*.gas $*.c

%.nas : %.gas Makefile
	$(GAS2NASK) $*.gas $*.nas

%.obj : %.nas Makefile
	$(NASK) $*.nas $*.obj $*.lst

# 常用命令行

img :
	$(MAKE) hcos.img

clean :
	-$(DEL) *.bin
	-$(DEL) *.lst
	-$(DEL) *.obj
	-$(DEL) *.map
	-$(DEL) *.bim
	-$(DEL) *.hrb
	-$(DEL) hcos.sys

src_only :
	$(MAKE) clean
	-$(DEL) hcos.img
	-$(DEL) *.hrb
