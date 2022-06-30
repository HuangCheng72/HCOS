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


//初始化内存管理，我的双向链表实现 
void memman_init(struct MEMMAN *man){
	man->frees = 0;			//可用信息数
	man->maxfrees = 0;		// 用于观察可用状况：frees的最大值
	man->lostsize = 0;		// 释放失败的内存大小总和，也就是丢失的碎片
	man->losts = 0;			// 释放失败的次数
	//构建链表关系，空余链表可以头插法插入结点，以头删法删除结点
	//先初始化两个头结点 
	man->free[MEMMAN_FREES].next = 0;
	man->free[MEMMAN_FREES].pre = 0;
	man->free[MEMMAN_FREES+1].next = &man->free[0];
	//为了实现有序插入，用空余链表的pre存储可用链表的最后一个结点指针，实现尾插法 
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
//内存申请，我的双向链表实现版本  
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size){
	struct FREEINFO* ptr =  man->free[MEMMAN_FREES].next;
	unsigned int a;
	while(ptr){
		//找到一块足够大的内存 
		if (ptr->size >= size) { 
			a = ptr->addr;
			ptr->addr += size;
			ptr->size -= size;
			//如果ptr的size都变成0了，那结点也没有存在的必要了，加入空余链表中
			if(ptr->size == 0){
				man->frees--;
				//断开本结点和前后的联系 
				ptr->pre->next = ptr->next;
				ptr->next->pre = ptr->pre;
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
//内存释放，我的双向链表实现版本 
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size){
	//这一步是通过循环找到正确的归还位置，所以一开始就必须按照addr顺序排序，现在我们没有红黑树只能用循环找正确的位置 
	struct FREEINFO* ptr =  man->free[MEMMAN_FREES].next;
	if(ptr == 0){
		//特殊情况特殊处理
		//1.第一次释放内存，可用链表中一个结点都无
		//2.释放的结点大于结点中最末尾的地址  
		//把可用链表头结点的next指针指向我们要使用的新节点 
		man->free[MEMMAN_FREES].next = man->free[MEMMAN_FREES+1].next;
		//把新节点从空余链表中分离出来
		man->free[MEMMAN_FREES+1].next = man->free[MEMMAN_FREES+1].next->next;
		ptr = man->free[MEMMAN_FREES].next;
		//写入数据，返回结果 
		ptr->addr = addr;
		ptr->size = size;
		ptr->next = 0;
		ptr->pre = 0;//这么做是到时候不要把头结点纳入范围去合并
		//更新尾指针 
		man->free[MEMMAN_FREES+1].pre = ptr;
		man->frees++;//统计信息更新 
		return 0;
	}
	while(ptr){
		//因为是按顺序排序，所以一旦ptr->addr大于addr，说明这个结点的pre就是我们这个内存的归还位置 
		if (ptr->addr > addr) {
			break;
		}
		ptr = ptr->next;
	}
	if(ptr == 0){
		//特殊情况特殊处理
		//2.释放的结点大于结点中最末尾的地址，要更新尾指针 
		//把可ptr指向我们要使用的新节点
		ptr = man->free[MEMMAN_FREES+1].next;
		//把新节点从空余链表中分离出来
		man->free[MEMMAN_FREES+1].next = man->free[MEMMAN_FREES+1].next->next;
		//写入数据，返回结果 
		ptr->addr = addr;
		ptr->size = size;
		//链接到尾结点后面 
		ptr->next = 0;
		ptr->pre = man->free[MEMMAN_FREES+1].pre;
		ptr->pre->next = ptr; 
		//更新尾指针 
		man->free[MEMMAN_FREES+1].pre = ptr;
		man->frees++;//统计信息更新 
		return 0;
	}
	//找到正确的归还位置之后
	//归还之前能否向后合并（合并到当前结点）呢？ 
	if(ptr->next){
		//如果可以向后合并就先合并到后一个结点 
		if(addr + size == ptr->addr){
			ptr->addr = addr;
			ptr->size += size; 
		}
	}
	if(ptr->pre){
		//如果已经合并到当前结点了，能否向前合并，然后把当前结点加入空余链表
		if(ptr->pre->addr + ptr->pre->size == ptr->addr){
			ptr->pre->size += ptr->size;
			//统计信息更新 
			man->frees--;
			//首先断开本结点和前后的联系 
			ptr->pre->next = ptr->next;
			ptr->next->pre = ptr->pre;
			//然后当前结点加入空余链表存着
			ptr->next = man->free[MEMMAN_FREES+1].next; 
			man->free[MEMMAN_FREES+1].next = ptr;
			//返回值 
			return 0;
		}else{
			//不能向前合并，就看看是不是已经归还到本结点了
			if(addr == ptr->addr){
				//归还完毕，返回
				return 0; 
			}
			//到了这个那就是又不能向前归还又不能向后归还了
			//直接从空余链表里面索要一个结点插入
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
				man->frees++;//统计信息更新 
				return 0;
			}
		}
	}else{
		//到了这一步也只有一种情况
		//找到的可以归还到的内存块在第一个结点
		//直接从空余链表里面索要一个结点插入
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
			//写入数据，返回结果
			ptr->pre->addr = addr;
			ptr->pre->size = size;
			man->frees++;//统计信息更新 
			return 0;
		}
	}
	//失败了内存当然丢了 
	man->losts++;
	man->lostsize += size;
	return -1;
}


//作者的基于数组实现，我的基于双向链表实现 
/* 
//初始化内存管理 
void memman_init(struct MEMMAN *man){
	man->frees = 0;			//可用信息数
	man->maxfrees = 0;		// 用于观察可用状况：frees的最大值
	man->lostsize = 0;		// 释放失败的内存大小总和，也就是丢失的碎片
	man->losts = 0;			// 释放失败的次数
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
			// 找到了足够大的内存
			a = man->free[i].addr;
			man->free[i].addr += size;
			man->free[i].size -= size;
			if (man->free[i].size == 0) {
				// 如果free[i]变成了0，就减去一条可用信息
				man->frees--;
				for (; i < man->frees; i++) {
					man->free[i] = man->free[i + 1]; // 结构体代入
				}
			}
			return a;
		}
	}
	return 0; // 无可用空间
}
//内存释放作者的版本  
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size){
	//就是申请的反操作
	//free内存应当按照addr顺序排列
	//然后归还到合适的位置
	//相当于在这个有序数组某个位置的元素改变属性
	//有序数组，找到符合条件的元素，然后更改其属性，是否与前面与后面可以合并 
	int i, j;
	//为了便于合并内存，将free[]按照addr的顺序排列
	// 所以，先决定应该放在哪里
	for (i = 0; i < man->frees; i++) {
		if (man->free[i].addr > addr) {
			break;
		}
	}
	// free[i - 1].addr < addr < free[i].addr
	if (i > 0) {
		// 前面有可用内存
		if (man->free[i - 1].addr + man->free[i - 1].size == addr) {
			// 向前合并
			man->free[i - 1].size += size;
			if (i < man->frees) {
				// 后面也有
				if (addr + size == man->free[i].addr) {
					// 向后合并
					man->free[i - 1].size += man->free[i].size;
					//man->free[i]删除
					// free[i]变成0之后合并到前面去
					man->frees--;
					for (; i < man->frees; i++) {
						man->free[i] = man->free[i + 1]; //结构体赋值
					}
				}
			}
			return 0; // 成功完成
		}
	}
	//不能与前面的可用空间合并到一起
	if (i < man->frees) {
		// 后面还有
		if (addr + size == man->free[i].addr) {
			// 可以和后面的空间合并
			man->free[i].addr = addr;
			man->free[i].size += size;
			return 0; // 成功完成
		}
	}
	// 既不能与前面合并，又不能与后面合并
	if (man->frees < MEMMAN_FREES) {
		//free[i]之后的，向后移动，腾出可用空间
		for (j = man->frees; j > i; j--) {
			man->free[j] = man->free[j - 1];
		}
		man->frees++;
		if (man->maxfrees < man->frees) {
			man->maxfrees = man->frees; // 更新最大值
		}
		man->free[i].addr = addr;
		man->free[i].size = size;
		return 0; // 成功完成
	}
	// 不能往后移动
	man->losts++;
	man->lostsize += size;
	return -1; //失败
}
*/

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

