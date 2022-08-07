## 双向链表与RBT，优化内存管理功能（上）

author: HC        2022/08/08

首先说明，上篇并未应用到红黑树（RBT），想看红黑树如何应用的朋友，请移步中篇和下篇，谢谢。

本篇所针对的内容是书中第九天内存管理部分，也即书的第162页到180页，具体一点其实只有第四小节挑战内存管理，也即从172页到180页这部分内容。

这部分内容虽然只有短短几页，但是却占据了如此篇幅，是因为我经历了三次几乎完全推倒重来，其中的思考（开脑洞）过程，代码的具体修改过程更是令我多次欲哭无泪（总是有各种各样的点没有考虑到导致出现了很多奇奇怪怪的错误）。虽然过程十分艰辛，但是结果是我比较满意的，其中收获也是十分巨大的。

先上作者的代码，并附上中文注释：

bootpack.h（来源于30_day/harib27f）

```c
/* memory.c */
#define MEMMAN_FREES		4090	/* 大小约32KB */
#define MEMMAN_ADDR			0x003c0000
struct FREEINFO {	/* 可用信息 */
	unsigned int addr, size;
};
struct MEMMAN {		/* 内存管理结构体 */
	int frees, maxfrees, lostsize, losts;
	struct FREEINFO free[MEMMAN_FREES];
};
unsigned int memtest(unsigned int start, unsigned int end);
void memman_init(struct MEMMAN *man);
unsigned int memman_total(struct MEMMAN *man);
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size);
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size);
unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size);
int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size);

```

memory.c

```c
#include "bootpack.h"

#define EFLAGS_AC_BIT		0x00040000
#define CR0_CACHE_DISABLE	0x60000000

unsigned int memtest(unsigned int start, unsigned int end)
{
	char flg486 = 0;
	unsigned int eflg, cr0, i;

	/* 确定CPU是386还是486及以上的 */
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

void memman_init(struct MEMMAN *man)
{
	man->frees = 0;			/* 可用信息数 */
	man->maxfrees = 0;		/* 用于观察可用状况：frees的最大値 */
	man->lostsize = 0;		/* 释放失败的内存大小总和 */
	man->losts = 0;			/* 释放失败的次数 */
	return;
}

unsigned int memman_total(struct MEMMAN *man)
/* 报告空余内存大小总和 */
{
	unsigned int i, t = 0;
	for (i = 0; i < man->frees; i++) {
		t += man->free[i].size;
	}
	return t;
}

unsigned int memman_alloc(struct MEMMAN *man, unsigned int size)
/* 分配 */
{
	unsigned int i, a;
	for (i = 0; i < man->frees; i++) {
		if (man->free[i].size >= size) {
			/* 找到了足够大的内存 */
			a = man->free[i].addr;
			man->free[i].addr += size;
			man->free[i].size -= size;
			if (man->free[i].size == 0) {
				/* 如果free[i].size变成了0，就减去这个结点 */
				man->frees--;
				for (; i < man->frees; i++) {
					man->free[i] = man->free[i + 1]; /* 代入结构体 */
				}
			}
			return a;
		}
	}
	return 0; /* 没有可用空间 */
}

int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size)
/* 释放 */
{
	int i, j;
	/* 为便于归纳内存，将free[]按照addr顺序排列 */
	/* 所以，先决定放在哪里 */
	for (i = 0; i < man->frees; i++) {
		if (man->free[i].addr > addr) {
			break;
		}
	}
	/* free[i - 1].addr < addr < free[i].addr */
	if (i > 0) {
		/* 前面有可用内存 */
		if (man->free[i - 1].addr + man->free[i - 1].size == addr) {
			/* 可以和前面的可用内存归纳到一起 */
			man->free[i - 1].size += size;
			if (i < man->frees) {
				/* 后面也有 */
				if (addr + size == man->free[i].addr) {
					/* 也可以与后面的可用内存归纳到一起 */
					man->free[i - 1].size += man->free[i].size;
					/* man->free[i]删除 */
					/* free[i]变成0以后归纳到前面去 */
					man->frees--;
					for (; i < man->frees; i++) {
						man->free[i] = man->free[i + 1]; /* 代入结构体 */
					}
				}
			}
			return 0; /* 成功完成 */
		}
	}
	/* 不能与前面的可用空间归纳到一起 */
	if (i < man->frees) {
		/* 后面还有 */
		if (addr + size == man->free[i].addr) {
			/* 可以和后面的内容归纳到一块 */
			man->free[i].addr = addr;
			man->free[i].size += size;
			return 0; /* 成功完成 */
		}
	}
	/* 既不能和前面归纳到一块，也不能和后面归纳到一块 */
	if (man->frees < MEMMAN_FREES) {
		/* free[i]之后的，向后移动，腾出一个可用的位置 */
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
	/* 不能向后移动 */
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

```

作者的内存管理，是基于升序排列的数组（排列的键值为addr）从而实现的，实现的思路相当简单。

1. 当申请内存的时候，遍历结构体数组（时间复杂度O(n)），找到第一个size >= 指定size的元素，而后再记录下该元素的addr，并更新addr为原addr+指定值，size为原size-指定值（时间复杂度O(1)），如果size更新为0，则循环将该位置以后的元素全部前移（时间复杂度O(n)）。则申请内存操作时间复杂度为O(n)。
2. 当释放内存的时候，遍历结构体数组（时间复杂度O(n)），因为是数组是升序排列的，找到第一个addr>=指定addr的元素即找到了合适的插入位置，如果要释放的内存块可以向前向后合并，则直接合并（时间复杂度O(1)），若不可以向前向后合并，则循环将该位置开始元素全部后移（时间复杂度O(n)），空出一个位置，将释放内存块的信息写入该位置元素中（时间复杂度O(1)），若同时可以向前向后合并，合并完成之后就要循环整体前移，覆盖掉一个被合并的元素（时间复杂度O(n)）。则释放内存操作时间复杂度为O(n)。

改进是循序渐进的，不可能一蹴而就，因此我们先考虑对其中一部分进行修改。

我首先注意到的是申请内存时的特殊情况，即申请内存中，当元素的size更新为0时，要将数组中从该位置以后的元素全部前移，这一操作时间复杂度是O(n)。而在释放内存时，插入一个元素也需要将数组从该位置开始的元素全部后移，同时向前向后合并时也会发生整体前移，这两个操作的时间复杂度都是O(n)。这些操作都是可以通过使用链表这种数据结构来避免的。

常用的链表有单链表和双向链表两种，双向链表相比较于单链表多了一个前驱指针pre指向前驱结点。在链表删除某个结点的时候，只需要将该结点的前驱的next指针指向该结点的next指针，即可从链表中删除该结点。单链表中获取指定结点的前驱结点则需要遍历（时间复杂度O(n)）或者提前存储，而双向链表则因为同时具有pre指针和next指针，可以避免这一操作，不管是插入结点（插入在结点之前或者插入在结点之后，也即头插法和尾插法）还是删除结点，其时间复杂度均是O(1)。

两相比较，我决定选择双向链表这种数据结构对HCOS的内存管理功能进行重写。

但是这又涉及到一个问题。即如何区分已使用和未使用的结点？

在作者版本的内存管理结构体中，使用了一个整型变量frees来记录d组中有多少个元素，这样就可以区分已使用和未使用的元素。

双向链表和数组都是线性表，自然也可以如法炮制。然而，数组是插入和删除元素困难（需要循环后移前移元素），访问元素容易（可以通过数组名[下标]直接访问）；双向链表是插入和删除结点容易（原因在上面），访问结点困难（需要遍历到这个结点）。如果我们需要对双向链表插入一个新结点，就要获取一个未使用的结点，如果以遍历的方式访问获取，那么时间复杂度就是O(n)，之后对结点写入相关信息并且插入链表时间复杂度为O(1)，与改进之前似乎也没什么区别。如果我们需要对双向链表删除其中一个结点，那么删除后的结点变为未使用状态，如果忽视则会造成内存空间的浪费，那么内存管理所管理的内存信息就越来越少了，若是要回收到未使用的部分，则又要以遍历的方式访问，时间复杂度为O(n)，与改进之前似乎区别真的不大。

所以，我经过考虑决定以空间换时间，直接增加两个特殊的头结点，分别存储可用链表（即存储已使用的结点）和空余链表（即存储未使用的结点），其中可用链表中的结点的addr应以升序排列，也就是一个升序双向链表。

因此建立的数据结构的图示如下：

![](pic\LinkedList1.jpg)

当双向链表插入结点的时候，直接从空余链表中获取第一个结点，作为新结点插入可用链表中。则时间复杂度为O(1)。

当双向链表删除结点的时候，直接将被删除的结点以头插法插入为空余链表的第一个结点。则时间复杂度为O(1)。

以双向链表这种数据结构重写内存管理，思路先继续沿用作者的，先不做过多变动。**以下数组和链表均按照addr为键值升序排序**。

memory.h

```c
#define MEMMAN_FREES		1340	/* 大约32KB，这里维持作者的这个32KB大小，是怕访问到可能存放其他信息的内存空间 */
#define MEMMAN_ADDR			0x003c0000    //这里指定的是MEMMAN这个结构体放置的内存空间开始地址 
#define EFLAGS_AC_BIT		0x00040000
#define CR0_CACHE_DISABLE	0x60000000

//尝试用双向链表思想修改内存管理 
struct FREEINFO {	/* 可用信息 */
	unsigned int addr, size;
	struct FREEINFO* pre;
	struct FREEINFO* next; //可用信息结点的pre指针和next指针 
};

struct MEMMAN {		/* 内存管理 */
	int lostsize, losts;
	struct FREEINFO free[MEMMAN_FREES+2]; //下标为MEMMAN_FREES的结点，是可用链表的头结点，下标为MEMMAN_FREES+1的结点，是空余链表的头结点 
};

unsigned int memtest(unsigned int start, unsigned int end);//内存容量检查
void memman_init(struct MEMMAN *man);//内存管理初始化 
unsigned int memman_total(struct MEMMAN *man);//计算可用内存总量 
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size);//申请内存 
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size);//释放内存 
unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size);
int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size);

```

memory.c

```c
#include "bootpack.h" 

//内存容量检查不受影响，不变
unsigned int memtest(unsigned int start, unsigned int end){
	//略
}
//一次性申请4KB的倍数的内存，不变
unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size){
	//略
}
//一次性释放4KB的倍数的内存，不变
int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size){
	//略
}

//初始化内存管理，我的双向链表实现 
void memman_init(struct MEMMAN *man){
	man->lostsize = 0;		// 释放失败的内存大小总和，也就是丢失的碎片
	man->losts = 0;			// 释放失败的次数
	//构建链表关系，空余链表可以头插法插入结点，删除结点同理
	//先初始化两个头结点，其中man->free[MEMMAN_FREES]是可用链表的头结点，man->free[MEMMAN_FREES+1]是空余链表头结点
	man->free[MEMMAN_FREES].next = 0;
	man->free[MEMMAN_FREES].pre = 0;
	man->free[MEMMAN_FREES+1].next = &man->free[0];
	//为了实现有序插入，用空余链表的pre存储可用链表的最后一个结点的指针（尾指针），实现尾插法，最开始可用链表没有结点，所以尾指针指向可用链表的头结点（置空其实也可以）
	man->free[MEMMAN_FREES+1].pre = &man->free[MEMMAN_FREES];
	//一开始全是空余链表，所以直接从头开始往后构建关系（只用来存储当单链表用就行）
	int i; 
	for(i = 1; i < MEMMAN_FREES; i++){
		man->free[i-1].next = &man->free[i];
	}
	man->free[MEMMAN_FREES-1].next = 0;
	return;
}
//空余内存合计是多少 ，我的双向链表实现 
unsigned int memman_total(struct MEMMAN *man){
	//改成链表版本也一样直接循环加起来就行了 
	unsigned int t = 0;
	struct FREEINFO* ptr =  man->free[MEMMAN_FREES].next;
	while(ptr){
		t += ptr->size;
		ptr = ptr->next;
	}
	return t;
}

```

申请内存的部分比较好改，与作者思路基本一致，遍历链表，找到第一个size>=指定值的结点，然后更新结点addr和size值（变动后依然有序，因此不需要重新排序），如果更新后size值为0，则将该结点从可用链表中删除，并用头插法插入到空余链表的第一个结点（头结点之后）。

不过申请内存部分需要注意的是，若要删除的结点是第一个结点、最后一个结点，或者唯一的结点的情形，需要判断和处理。

memory.c

```c
//内存申请，我的双向链表实现版本  
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size){
    struct FREEINFO* ptr =  man->free[MEMMAN_FREES].next;
    if(ptr == 0){
        return 0;
    }
    unsigned int a;
    while(ptr){
		//找到一块足够大的内存 
		if (ptr->size >= size) { 
			a = ptr->addr;
			ptr->addr += size;
			ptr->size -= size;
			//如果ptr的size都变成0了，那结点也没有存在的必要了，加入空余链表中
			if(ptr->size == 0){
				//断开本结点和前后的联系 
				if(ptr->pre && ptr->next){
                    //不是第一个结点或者最后一个结点
                    ptr->pre->next = ptr->next;
                    ptr->next->pre = ptr->pre;
                }else{
                    if(ptr->pre == 0 && ptr->next){
                        //第一个结点，要把后继的pre置0
                        man->free[MEMMAN_FREES].next = ptr->next;
                        ptr->next->pre = 0;
                    }else if(ptr->pre && ptr->next == 0){
                        //最后一个结点，要把前驱的后继置0
                        ptr->pre->next = 0;
                        man->free[MEMMAN_FREES+1].pre = ptr->pre;
                    }else{
                        //ptr的pre和next都是0，就是说要删除这个唯一的结点，直接把空余链表头结点的pre指向可用链表头结点
                        man->free[MEMMAN_FREES+1].pre = &man->free[MEMMAN_FREES];
                    }
                }
                //然后当前结点加入空余链表存着
                ptr->next = man->free[MEMMAN_FREES+1].next;
                man->free[MEMMAN_FREES+1].next = ptr;
			}
			return a; 
		}
		ptr = ptr->next;
	}
	//没可用空间直接返回0就是了 
    return 0;
}

```

释放内存部分，则要找出基于链表实现和基于数组实现的释放内存的相同和不同之处。

根据作者的思路，基于数组实现的释放内存分情形如下：

1. 释放的内存数据不可与前后合并，也就是直接插入一个元素。

   1. 释放的内存数据应位于可用数据的尾部（插入位置为frees），则对该位置元素直接写入信息即可。

   2. 释放的内存数据应位于可用数据的中部（插入位置在0到frees-1之间），则要先整体后移，再对该位置元素写入信息。

2. 释放的内存数据可以与前后合并。

   1. 释放的内存数据可以与后合并，但是不可与前合并，则直接归纳到后面元素即可。
   2. 释放的内存数据可以与前合并，但是不可与后合并，则直接归纳到前面元素即可。
   3. 释放的内存数据可以与前后合并，那么就先与后合并，归纳到后面元素，然后再一同归纳到前面元素，然后再整体前移，覆盖掉原有的后面元素。

而基于链表实现的释放内存分情形如下：

1. 释放的内存数据不可与前后合并，也就是直接插入一个元素。
   1. 释放的内存数据应位于可用数据的中部。
   2. 释放的内存数据应位于可用数据的头部（也就是可用链表头结点的next所指向的结点）。
   3. 释放的内存数据应位于可用数据的尾部（也就是空余链表头结点的pre所指向的结点）。
2. 释放的内存数据可以与前后合并。
   1. 释放的内存数据应为与可用数据的中部。
      1. 释放的内存数据可以与后合并，但是不可与前合并，则直接归纳到后面结点即可。
      2. 释放的内存数据可以与前合并，但是不可与后合并，则直接归纳到前面结点即可。
      3. 释放的内存数据可以与前后合并，那么就先与后合并，归纳到后面结点，然后再一同归纳到前面结点，然后再删除后面结点。
   2. 释放的内存数据应为与可用数据的头部（第一个结点，是可用链表头结点的后继，且其pre为0）。
      1. 释放的内存数据只能与后合并，直接归纳到后面结点即可。
   3. 释放的内存数据应为与可用数据的尾部（最后一个结点，是空余链表头结点的pre所指向的结点，其next为0）。
      1. 释放的内存数据只能与前合并，直接归纳到前面结点即可。

基于升序双向链表实现释放内存，要遍历找到第一个大于所要释放的内存信息的addr的结点，获取其指针。

若可用链表为空或数据应位于可用数据的尾部，则查询的结果均为0（指针的值，也就是NULL）。

若数据应位于可用数据的头部，则查询的结果必然是第一个结点的指针，其pre为0（插入新结点的时候就有点麻烦了，需要额外的判断处理）。

因此，我基于升序双向链表所实现的释放内存如下：

memory.c

```c
//内存释放，我的双向链表实现版本 
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size){
	//这一步是通过循环找到正确的归还位置，所以一开始就必须按照addr顺序排序，现在我们没有红黑树只能用循环找正确的位置 
	struct FREEINFO* ptr =  man->free[MEMMAN_FREES].next;
	//查找相应的地址位置
	while(ptr){
		//因为是按顺序排序，所以一旦ptr->addr大于addr，说明这个结点的pre就是我们这个内存的归还位置 
		if (ptr->addr > addr) {
			break;
		}
		ptr = ptr->next;
	}
	if(ptr == 0){
		//特殊情况特殊处理
        //1.第一次释放内存，可用链表中一个结点都无
		//2.释放的结点大于结点中最末尾的地址，要更新尾指针 
		
        //这两种情况其实差不多，其实都是插入数据到最末尾（第一种情况，唯一一个结点自然是第一个也是最后一个结点）
        
        //空余链表的pre指向可用链表的末位置（作为尾指针来用）
        ptr = man->free[MEMMAN_FREES+1].pre;
        //先确定尾指针指向的不是头结点
        if(ptr != &man->free[MEMMAN_FREES] && addr == ptr->addr + ptr->size){
            //如果新插入的结点可以直接合并到最末尾结点就不要插入了
            ptr->size += size;
            return 0;
        }
        ptr->next = man->free[MEMMAN_FREES+1].next;
        //把新结点从空余链表中分离出来
        man->free[MEMMAN_FREES+1].next = man->free[MEMMAN_FREES+1].next->next;
        //对新结点写入数据
        ptr->next->addr = addr;
        ptr->next->size = size;
        //最后一个结点的后继以定时0
        ptr->next->next = 0;
        if(ptr == &man->free[MEMMAN_FREES]){
            //如果是头结点的话，新结点pre不要指向头结点，这是为了合并的时候不要合并到头结点
            ptr->next->pre = 0;
        }else{
            ptr->next->pre = ptr;
        }
        //更新尾指针
        man->free[MEMMAN_FREES+1].pre = ptr->next;
        return 0;
	}
	//找到正确的归还位置之后
	//如果可以向后合并就先合并到后一个结点 
	if(addr + size == ptr->addr){
        ptr->addr = addr;
		ptr->size += size; 
	}
	if(ptr->pre){
		//能否向前合并
		if(ptr->pre->addr + ptr->pre->size == addr){
            //既然已经往后合并（到当前结点）了，又可以往前合并，那就往前合并，然后把当前结点加入空余链表
            if(addr == ptr->addr){
                ptr->pre->size += ptr->size;
                //首先断开本结点和前后的联系 
                ptr->pre->next = ptr->next;
                ptr->next->pre = ptr->pre;
                //然后当前结点加入空余链表存着
                ptr->next = man->free[MEMMAN_FREES+1].next; 
                man->free[MEMMAN_FREES+1].next = ptr;
            }else{
                //没有往后合并，那就往前合并即可
                ptr->pre->size += size;
            }
            return 0;
		}else{
			//不能向前合并，就看看是不是已经归还到本结点了
			if(addr == ptr->addr){
				//归还完毕，返回
				return 0; 
			}
			//到了这个那就是又不能向前归还又不能向后归还了
			//直接从空余链表里面要一个结点插入
			if(man->free[MEMMAN_FREES+1].next){
				//还有结点我们就用吧
				//把pre的next指针指向我们要使用的新节点
				ptr->pre->next = man->free[MEMMAN_FREES+1].next;
				//把新节点从空余链表中分离出来 
				man->free[MEMMAN_FREES+1].next = man->free[MEMMAN_FREES+1].next->next;
				//新结点的next指向本结点，pre指向本结点的pre 
				ptr->pre->next->next = ptr;
				ptr->pre->next->pre = ptr->pre;
				//本结点的pre指向新结点，完成链表的插入 
				ptr->pre = ptr->pre->next;
				//写入数据，返回结果 
				ptr->pre->addr = addr;
				ptr->pre->size = size;
				return 0;
			}
		}
	}else{
        //不能向前合并，就看看是不是已经归还到本结点了
		if(addr == ptr->addr){
			//归还完毕，返回
			return 0; 
		}
		//到了这一步也只有一种情况
		//找到的可以归还到的内存块应该在第一个结点
		//直接从空余链表里面要一个结点插入
		if(man->free[MEMMAN_FREES+1].next){
			//还有结点我们就用吧
			//把pre的next指针指向我们要使用的新节点 
			//本结点pre指向新结点 
			ptr->pre = man->free[MEMMAN_FREES+1].next;
			//把新节点从空余链表中分离出来 
			man->free[MEMMAN_FREES+1].next = man->free[MEMMAN_FREES+1].next->next;
			//新结点的next指向本结点，头结点指向新结点
			man->free[MEMMAN_FREES].next = ptr->pre;
			ptr->pre->next = ptr;
            ptr->pre->pre = 0;
			//写入数据，返回结果
			ptr->pre->addr = addr;
			ptr->pre->size = size;
			return 0;
		}
	}
	//失败了内存当然丢了 
	man->losts++;
	man->lostsize += size;
	return -1;
}

```

整合为memory.h和memory.c文件

并且将bootpack.h中的相关声明注释掉，include我的memory.h，如下所示：

```c
/* memory.c */
//#define MEMMAN_FREES		4090	/* これで約32KB */
//#define MEMMAN_ADDR			0x003c0000
//struct FREEINFO {	/* あき情報 */
//	unsigned int addr, size;
//};
//struct MEMMAN {		/* メモリ管理 */
//	int frees, maxfrees, lostsize, losts;
//	struct FREEINFO free[MEMMAN_FREES];
//};
//unsigned int memtest(unsigned int start, unsigned int end);
//void memman_init(struct MEMMAN *man);
//unsigned int memman_total(struct MEMMAN *man);
//unsigned int memman_alloc(struct MEMMAN *man, unsigned int size);
//int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size);
//unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size);
//int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size);
#include "memory.h"

```

而后正常编译运行即可。

欢迎访问我的GitHub仓库，如果对您有帮助，麻烦给一个star，谢谢！

[GitHub - HuangCheng72/HCOS](https://github.com/HuangCheng72/HCOS)
