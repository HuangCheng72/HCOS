#include "bootpack.h"

/*
 * 查找"RBT"中addr值大于等于指定addr值的节点。没找到的话，返回0。
 */
struct FREEINFO* my_findNode(struct rb_root *root, unsigned int addr){
    struct rb_node* rbnode = root->rb_node;
    while (rbnode != 0){
        struct FREEINFO* mynode = container_of(rbnode, struct FREEINFO, rb_node_addr);
        if (mynode->addr < addr){
            //出现这种情况就找右子树有没有符合条件的结果
            struct rb_node* temp = &mynode->rb_node_addr;
            //如果右子树为0，无法再往下找，说明不存在
            //如果右子树不为0，就往右子树走
            //若是当前结点不为0且结点addr大于等于指定addr，则此处即是结果
            while(temp->rb_right != 0){
                temp = temp->rb_right;
                //如果找到结果就直接返回
                if(temp != 0 && container_of(temp, struct FREEINFO, rb_node_addr)->addr >= addr){
                    return container_of(temp, struct FREEINFO, rb_node_addr);
                }
            }
            //这都找不到，说明是真的找不到
            if(temp->rb_right == 0){
                return 0;
            }
        } else if (mynode->addr > addr){
            //出现这种情况就找左子树有没有符合条件的结果
            struct rb_node* temp = &mynode->rb_node_addr;
            //如果左子树为零，无法再往下找，必须终止，此处是结果
            //如果左子树不为零，若是左孩子的addr小于指定addr，此处就是结果
            while(temp->rb_left != 0 && container_of(temp->rb_left, struct FREEINFO, rb_node_addr)->addr >= addr){
                temp = temp->rb_left;
                //如果左子树不为0且左子树的addr小于指定的addr，则找到结果，自动退出循环
                //如果左子树为0，则找到结果，自动退出循环
            }
            //找不到结果，此处修改
            return container_of(temp, struct FREEINFO, rb_node_addr);
        } else {
            //等于，那就是mynode了
            return mynode;
        }
    }
    return 0;
}
//将地址插入RBT中
void insert_addr(struct rb_root *root, struct FREEINFO* node){
    struct rb_node **tmp = &(root->rb_node), *parent = 0;
    //寻找插入位置
    while (*tmp){
        struct FREEINFO* my = container_of(*tmp, struct FREEINFO, rb_node_addr);
        parent = *tmp;
        if (node->addr < my->addr)
            tmp = &((*tmp)->rb_left);
        else if (node->addr > my->addr)
            tmp = &((*tmp)->rb_right);
        else
            return;
    }
    //连接要插入的结点和父结点
    rb_link_node(&node->rb_node_addr, parent, tmp);
    //插入该结点，并进行调整
    rb_insert_color(&node->rb_node_addr, root);
    return;
}

//RBT删除某个结点
void delete_addr(struct rb_root *root, struct FREEINFO* node){
    //防止出现删除不存在结点的问题
    //先查找
    struct rb_node *rbnode = root->rb_node;
    char flag = 0;
    while (rbnode != 0){
        struct FREEINFO* mynode = container_of(rbnode, struct FREEINFO, rb_node_addr);

        if (node->addr < mynode->addr)
            rbnode = rbnode->rb_left;
        else if (node->addr > mynode->addr)
            rbnode = rbnode->rb_right;
        else if(node->addr == mynode->addr){
            flag = 1;
            break;
        }
    }
    if(!flag){
        return;
    }
    //从红黑树中删除节点mynode
    rb_erase(&node->rb_node_addr, root);
    return;
}

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


//初始化内存管理，我的双向链表+RBT实现（RBT初始化） 
void memman_init(struct MEMMAN *man){
    man->frees = 0;			//可用信息数
    man->maxfrees = 0;		// 用于观察可用状况：frees的最大值
    man->lostsize = 0;		// 释放失败的内存大小总和，也就是丢失的碎片
    man->losts = 0;			// 释放失败的次数
    man->root_addr = RB_ROOT; //初始化根结点
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
//我的双向链表+RBT实现版本
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size){
    //这一步是找到正确的归还位置
    struct FREEINFO* ptr =  my_findNode(&man->root_addr , addr);
    if(ptr == 0){
        //如果找到的结果ptr == 0
        //那么说明需要新插入结点到最末尾
        //空余链表的pre指向可用链表的末位置（作为尾指针来用）
        ptr = man->free[MEMMAN_FREES+1].pre;
        if(addr == ptr->addr + ptr->size){
            //如果新插入的结点可以直接合并到尾指针就不要插入了
            //修改之后RBT依然有序，无需重新插入RBT
            ptr->size += size;
            return 0;
        }
        ptr->next = man->free[MEMMAN_FREES+1].next;
        //把新结点从空余链表中分离出来
        man->free[MEMMAN_FREES+1].next = man->free[MEMMAN_FREES+1].next->next;
        //对新结点写入数据
        ptr->next->addr = addr;
        ptr->next->size = size;
        ptr->next->next = 0;
        if(ptr == &man->free[MEMMAN_FREES]){
            //如果是头结点的话，新结点pre不要指向头结点，这是为了合并的时候不要合并到头结点
            ptr->next->pre = 0;
        }else{
            ptr->next->pre = ptr;
        }
        //更新尾指针
        man->free[MEMMAN_FREES+1].pre = ptr->next;
        man->frees++;//统计信息更新
        insert_addr(&man->root_addr,ptr->next);//RBT插入
        return 0;
    } else {
        //有结果就尝试先往后合并（合并到ptr结点）
        if(addr + size == ptr->addr){
            //修改之后RBT依然有序，无需重新插入RBT
            ptr->addr = addr;
            ptr->size += size;
        }
        //再尝试往前合并
        if(ptr->pre){
            //如果可以往前合并
            if(ptr->pre->addr + ptr->pre->size == addr){
                //是否已经合并到ptr结点
                if(addr == ptr->addr){
                    //已经合并到ptr结点，就要把ptr结点从RBT中删去
                    ptr->pre->size += ptr->size;
                    delete_addr(&man->root_addr,ptr);
                    //把ptr结点回收到空余链表中去
                    ptr->pre->next = ptr->next;
                    ptr->next->pre = ptr->pre;
                    ptr->next = man->free[MEMMAN_FREES+1].next;
                    man->free[MEMMAN_FREES+1].next = ptr;
                }else{
                    //没有合并到ptr结点，就合并到ptr->pre结点就行
                    ptr->pre->size += size;
                }
                return 0;
            } else {
                //如果不可以往前合并，那就只能插入新结点了
                //直接从空余链表里面要一个结点插入
                if(man->free[MEMMAN_FREES+1].next){
                    //还有结点我们就用吧
                    //把pre的next指针指向我们要使用的新节点
                    //不过要防止ptr是第一个结点的情况（其pre为0）
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
                    insert_addr(&man->root_addr,ptr->pre);//RBT插入
                    return 0;
                }
            }
        }
        //到这里，那就只能是在头结点的后面插入一个新结点了
        if(man->free[MEMMAN_FREES+1].next){
            //还有结点我们就用吧
            //把pre的next指针指向我们要使用的新节点
            //本结点pre指向新结点
            ptr->pre = man->free[MEMMAN_FREES+1].next;
            //把新节点从空余链表中分离出来
            man->free[MEMMAN_FREES+1].next = man->free[MEMMAN_FREES+1].next->next;
            //新结点的next指向本结点，头结点指向新结点，第一个结点的pre为0
            man->free[MEMMAN_FREES].next = ptr->pre;
            ptr->pre->next = ptr;
            ptr->pre->pre = 0;
            //写入数据，返回结果
            ptr->pre->addr = addr;
            ptr->pre->size = size;
            man->frees++;//统计信息更新
            insert_addr(&man->root_addr,ptr->pre);//RBT插入
            return 0;
        }
    }
    //失败了内存当然丢了
    man->losts++;
    man->lostsize += size;
    return -1;
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

