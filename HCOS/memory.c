#include "bootpack.h" 

//内存容量检查 
unsigned int memtest(unsigned int start, unsigned int end){
	char flg486 = 0;
	unsigned int eflg, cr0, i;

	//确定CPU是386还是486及以上的 
	eflg = io_load_eflags();
	eflg |= EFLAGS_AC_BIT; /* AC-bit = 1 */
	io_store_eflags(eflg);
	eflg = io_load_eflags();
	if ((eflg & EFLAGS_AC_BIT) != 0) { /* 如果是386，即使我们如此设定，AC还是会回到0 */
		flg486 = 1;
	}
	eflg &= ~EFLAGS_AC_BIT; /* AC-bit = 0 */
	io_store_eflags(eflg);

	if (flg486 != 0) {
		cr0 = load_cr0();
		cr0 |= CR0_CACHE_DISABLE; /* 暂时禁止缓存（这是为了计算内存的需要） */
		store_cr0(cr0);
	}
	
	//这段计算可用内存长度必须使用汇编编写，具体查看naskfunc.nas，书中P166页有C语言的代码（可惜不能编译到我们想要的结果） 
	i = memtest_sub(start, end);

	if (flg486 != 0) {
		cr0 = load_cr0();
		cr0 &= ~CR0_CACHE_DISABLE; /* 允许缓存 */
		store_cr0(cr0);
	}

	return i;
}
//初始化内存管理 
void memman_init(struct MEMMAN *man){
	man->frees = 0;			/* 可用信息数 */
	man->maxfrees = 0;		/* 用于观察可用状况：frees的最大值 */
	man->lostsize = 0;		/* 释放失败的内存大小总和，也就是丢失的碎片 */
	man->losts = 0;			/* 释放失败的次数 */
	return;
}
//空余内存合计是多少 
unsigned int memman_total(struct MEMMAN *man){
	//因为是基于数组的，所以计算倒是简单。。直接循环加起来就行了 
	unsigned int i, t = 0;
	for (i = 0; i < man->frees; i++) {
		t += man->free[i].size;
	}
	return t;
}
//申请一块合适的内存空间 
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size){
	//这一段的核心思路大概就是，寻找合适内存空间（第一个找到能用的就用，这叫首次适应）
	//然后这一段打上已经使用的标签或者减去这一部分的剩余内存空间，可用内存地址偏移 
	//如果减去之后可用内存空间完了
	//那就直接循环左移
	unsigned int i, a;
	for (i = 0; i < man->frees; i++) {
		if (man->free[i].size >= size) {
			/* 找到了足够大的内存 */
			a = man->free[i].addr;
			man->free[i].addr += size;
			man->free[i].size -= size;
			if (man->free[i].size == 0) {
				/* 如果free[i]变成了0，就减去一条可用信息*/
				man->frees--;
				for (; i < man->frees; i++) {
					man->free[i] = man->free[i + 1]; /* 结构体代入 */
				}
			}
			return a;
		}
	}
	return 0; /* 无可用空间 */
}
//内存释放 
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size){
	//就是申请的反操作
	//free内存应当按照addr顺序排列
	//然后归还到合适的位置
	//相当于在这个有序数组某个位置的元素改变属性
	//有序数组，找到符合条件的元素，然后更改其属性，是否与前面与后面可以合并 
	int i, j;
	/* 为了便于合并内存，将free[]按照addr的顺序排列 */
	/* 所以，先决定应该放在哪里 */
	for (i = 0; i < man->frees; i++) {
		if (man->free[i].addr > addr) {
			break;
		}
	}
	/* free[i - 1].addr < addr < free[i].addr */
	if (i > 0) {
		/* 前面有可用内存 */
		if (man->free[i - 1].addr + man->free[i - 1].size == addr) {
			/* 向前合并 */
			man->free[i - 1].size += size;
			if (i < man->frees) {
				/* 后面也有 */
				if (addr + size == man->free[i].addr) {
					/* 向后合并 */
					man->free[i - 1].size += man->free[i].size;
					/* man->free[i]删除 */
					/* free[i]变成0之后合并到前面去 */
					man->frees--;
					for (; i < man->frees; i++) {
						man->free[i] = man->free[i + 1]; /* 结构体赋值 */
					}
				}
			}
			return 0; /* 成功完成 */
		}
	}
	/* 不能与前面的可用空间合并到一起 */
	if (i < man->frees) {
		/* 后面还有 */
		if (addr + size == man->free[i].addr) {
			/* 可以和后面的空间合并 */
			man->free[i].addr = addr;
			man->free[i].size += size;
			return 0; /* 成功完成 */
		}
	}
	/* 既不能与前面合并，又不能与后面合并 */
	if (man->frees < MEMMAN_FREES) {
		/* free[i]之后的，向后移动，腾出可用空间 */
		for (j = man->frees; j > i; j--) {
			man->free[j] = man->free[j - 1];
		}
		man->frees++;
		if (man->maxfrees < man->frees) {
			man->maxfrees = man->frees; /* 更新最大值 */
		}
		man->free[i].addr = addr;
		man->free[i].size = size;
		return 0; /* 成功完成 */
	}
	/* 不能往后移动 */
	man->losts++;
	man->lostsize += size;
	return -1; /* 失败 */
}

unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size)
{
	unsigned int a;
	size = (size + 0xfff) & 0xfffff000;
	a = memman_alloc(man, size);
	return a;
}

int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size)
{
	int i;
	size = (size + 0xfff) & 0xfffff000;
	i = memman_free(man, addr, size);
	return i;
}

