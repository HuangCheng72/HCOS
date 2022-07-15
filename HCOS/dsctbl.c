#include "bootpack.h"
void init_gdtidt(void){
	//为什么设置这两个地址，其实是作者随便选的，因为我们现在对内存分布不是很熟悉，不要乱动，保持原状 
	//其实只要是没使用过的地址，都可以用 
	//这两个地址已经设为常量放在头文件中 
	struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;
	struct GATE_DESCRIPTOR    *idt = (struct GATE_DESCRIPTOR    *) ADR_IDT;
	int i;
	// GDT的初始化，直接把段号为0到8191的段信息，统统置0 
	for (i = 0; i <= LIMIT_GDT / 8; i++) {
		set_segmdesc(gdt + i, 0, 0, 0);
	}
	//单独设置段号为1和2的两个段
	//段号为1的段，其上限值0xffffffff大小正好是4GB，正好是32位可以管理的内存上限，其基地址位0，表示整个CPU能管理的全部内存 
//	set_segmdesc(gdt + 1, 0xffffffff, 0x00000000, 0x4092);
	//段号为2的段，其大小为512KB，这是给bootpack.hrb准备的 
//	set_segmdesc(gdt + 2, 0x0007ffff, 0x00280000, 0x409a);
	//将GDT信息写入gdtr寄存器，也就是我们设置的GDT信息存放的地址0x00270000 
//	load_gdtr(0xffff, 0x00270000);

	//常量写法 
	set_segmdesc(gdt + 1, 0xffffffff,   0x00000000, AR_DATA32_RW);
	set_segmdesc(gdt + 2, LIMIT_BOTPAK, ADR_BOTPAK, AR_CODE32_ER);
	load_gdtr(LIMIT_GDT, ADR_GDT);
	
	// IDT的初始化，同GDT 
	for (i = 0; i <= LIMIT_IDT / 8; i++) {
		set_gatedesc(idt + i, 0, 0, 0);
	}
	//将IDT信息写入专用于IDT的寄存器。 
	load_idtr(LIMIT_IDT, ADR_IDT);
	
	// IDT的设定
	//以第一个为例子，意思就是asm_inthandler21函数注册在IDT的第0x21号位置，如果发生相应中断，就自动调用指定函数
	//这里的2*8表示asm_inthandler21属于哪一个段，这里意思是段号为2，乘以8之前也说了，每个段的信息只有八个字节
	//上面也说了段号为2的段是给bootpack.hrb准备的
	//最后的 AR_INTGATE32的值说明把IDT的属性设定为0x008e，这是表示中断处理的有效设定。 
	set_gatedesc(idt + 0x20, (int) asm_inthandler20, 2 * 8, AR_INTGATE32);
	set_gatedesc(idt + 0x21, (int) asm_inthandler21, 2 * 8, AR_INTGATE32);
	set_gatedesc(idt + 0x27, (int) asm_inthandler27, 2 * 8, AR_INTGATE32);
	set_gatedesc(idt + 0x2c, (int) asm_inthandler2c, 2 * 8, AR_INTGATE32);
 
	return;
}
//这个函数本质上来说就是把段的全部信息归纳为八个字节（64位）CPU可以理解的约定俗成的形式，写入内存中
//理解需要位运算的基础，这里因为我还不会位运算，暂时先跳过。 
//这里的参数分别是，本段信息存放的地址，段的大小，段的基址，段的管理属性 
//段的基址占用32位，在32位CPU的世界里面所有的地址都是这么定的，没办法改
//段的大小占用20位，只能表示最多1MB，而4GB这个数值就已经是32位的二进制数了，不能直接占满，这里用了一种迂回的办法，在段的属性里面设置了一个标志位
//在标志位Gbit位为1的时候，limit的单位不是字节（byte），而是页（page），一页就是4KB（这里涉及到内存分页的问题），如此可以完成4GB的内存管理 
//段的管理属性占据12位，这部分只简单介绍几个常用的
//0x00，未使用的记录表
//0x92，系统专用，可读写不可执行。 
//0x9a，系统专用，可执行可读不可写。
//0xf2，应用程序用，可读写不可执行
//0xfa，应用程序用，可执行可读不可写。 
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar){
	
	if (limit > 0x000fffff) {
		ar |= 0x8000;
		limit /= 0x1000;
	}
	sd->limit_low 	= limit & 0xffff;
	sd->base_low	= base & 0xffff;
	sd->base_mid	= (base >> 16) & 0xff;
	sd->access_right	= ar & 0xff;
	sd->limit_high   = ((limit >> 16) & 0x0f) | ((ar >> 8) & 0xf0);
	sd->base_high    = (base >> 24) & 0xff;
	return;
}
//与上面的类似，不加赘述 
void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar){
	gd->offset_low   = offset & 0xffff;
	gd->selector     = selector;
	gd->dw_count     = (ar >> 8) & 0xff;
	gd->access_right = ar & 0xff;
	gd->offset_high  = (offset >> 16) & 0xffff;
	return;
}
