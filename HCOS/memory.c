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


//递归法查找"RBT"中addr值大于等于指定addr值的节点。没找到的话，返回0。
struct FREEINFO* my_search_addr(struct rb_node* rbnode, unsigned int addr){
    if(rbnode == 0){
        return 0;
    }
    struct FREEINFO* mynode = container_of(rbnode, struct FREEINFO, rb_node_addr);
    if(mynode->addr < addr){
        return my_search_addr(mynode->rb_node_addr.rb_right , addr);
    }else{
        //找到一个大于等于的结点
        if(mynode->addr == addr){
            //等于就直接返回
            return mynode;
        }
        if(mynode->rb_node_addr.rb_left == 0 ||  container_of(mynode->rb_node_addr.rb_left, struct FREEINFO, rb_node_addr)->addr < addr ){
            //左孩子不存在，或者， 左孩子小于，本结点大于，那就只能是这个了
            return mynode;
        }
        return my_search_addr(mynode->rb_node_addr.rb_left , addr);
    }
}
struct FREEINFO* my_findNode_addr(struct rb_root *root, unsigned int addr){
    return my_search_addr(root->rb_node, addr);
}
//将结点插入RBT中
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

//递归法查找"RBT"中size值大于等于指定size值的节点。没找到的话，返回0。
struct FREEINFO* my_search_size(struct rb_node* rbnode, unsigned int size){
    if(rbnode == 0){
        return 0;
    }
    struct FREEINFO* mynode = container_of(rbnode, struct FREEINFO, rb_node_size);
    if(mynode->size < size){
        return my_search_size(mynode->rb_node_size.rb_right , size);
    }else{
        //找到一个大于等于的结点
        if(mynode->size == size){
            //等于就直接返回
            if(mynode->next2){
                //优先链表里面的，不要过多变动红黑树
                return mynode->next2;
            }
            return mynode;
        }
        if(mynode->rb_node_size.rb_left == 0 ||  container_of(mynode->rb_node_size.rb_left, struct FREEINFO, rb_node_size)->size < size ){
            //左孩子不存在，或者， 左孩子小于，本结点大于，那就只能是这个了
            if(mynode->next2){
                //优先链表里面的，不要过多变动红黑树
                return mynode->next2;
            }
            return mynode;
        }
        return my_search_size(mynode->rb_node_size.rb_left , size);
    }
}
struct FREEINFO* my_findNode_size(struct rb_root *root, unsigned int size){
    return my_search_size(root->rb_node, size);
}
//将结点插入RBT中
void insert_size(struct rb_root *root, struct FREEINFO* node){
    if(node->pre2 || node->next2){
        //防止重复插入
        return;
    }
    struct rb_node **tmp = &(root->rb_node), *parent = 0;
    //寻找插入位置
    while (*tmp){
        struct FREEINFO* my = container_of(*tmp, struct FREEINFO, rb_node_size);
        parent = *tmp;
        if (node->size < my->size)
            tmp = &((*tmp)->rb_left);
        else if (node->size > my->size)
            tmp = &((*tmp)->rb_right);
        else{
            //已经有结点了就链接到链表上去
            node->next2 = my->next2;
            if(node->next2){
                //如果后面还有结点的话，连接前驱
                node->next2->pre2 = node;
            }
            node->pre2 = my;
            my->next2 = node;
            return;
        }
    }
    //连接要插入的结点和父结点
    rb_link_node(&node->rb_node_size, parent, tmp);
    //插入该结点，并进行调整
    rb_insert_color(&node->rb_node_size, root);
    return;
}

//RBT删除某个结点
void delete_size(struct rb_root *root, struct FREEINFO* node){
    //防止出现删除不存在结点的问题
    //先查找
    struct rb_node *rbnode = root->rb_node;
    while (rbnode != 0){
        struct FREEINFO* mynode = container_of(rbnode, struct FREEINFO, rb_node_size);
        if (node->size < mynode->size)
            rbnode = rbnode->rb_left;
        else if (node->size > mynode->size)
            rbnode = rbnode->rb_right;
        else if(node->size == mynode->size){
            if(mynode->next2 == 0 && mynode->pre2 == 0){
                //如果是在红黑树中的结点，此时只有这个结点，直接从红黑树中删除就行
                rb_erase(&node->rb_node_size, root);
                return;
            }else{
                //如果不是在红黑树中的那个结点，直接删除就是了
                node->pre2->next2 = node->next2;
                if(node->next2){
                    node->next2->pre2 = node->pre2;
                }
                node->next2 = 0;
                node->pre2 = 0;
                return;
            }
        }
    }
    return;
}

//初始化内存管理，我的双向链表+双RBT实现（RBT初始化）
void memman_init(struct MEMMAN *man){
    man->lostsize = 0;		// 释放失败的内存大小总和，也就是丢失的碎片
    man->losts = 0;			// 释放失败的次数
    man->root_addr = RB_ROOT; //初始化根结点
    man->root_size = RB_ROOT; //初始化根结点
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
//内存申请，我的双向链表+双RBT实现版本
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size){
    struct FREEINFO* ptr =  my_findNode_size(&man->root_size, size);
    if(ptr == 0){
        return 0;
    }
    unsigned int a;
    a = ptr->addr;
    ptr->addr += size;
    delete_size(&man->root_size, ptr);
    ptr->size -= size;
    if(ptr->size == 0){
        //把这个结点从RBT中删除
        delete_addr(&man->root_addr , ptr);
        //断开本结点和前后的联系
        if(ptr->pre && ptr->next){
            //不是第一个结点或者最后一个结点
            ptr->pre->next = ptr->next;
            ptr->next->pre = ptr->pre;
        }else{
            if(ptr->pre == 0 && ptr->next){
                //第一个结点
                man->free[MEMMAN_FREES].next = ptr->next;
                ptr->next->pre = 0;
            }else if(ptr->pre && ptr->next == 0){
                //最后一个结点
                ptr->pre->next = 0;
                man->free[MEMMAN_FREES+1].pre = ptr->pre;
            }else{
                //ptr的pre和next都是0，就是说要删除这个唯一的结点，那就将其加入空余链表中
                man->free[MEMMAN_FREES+1].pre = &man->free[MEMMAN_FREES];
                ptr->next2 = man->free[MEMMAN_FREES + 1].next;
                man->free[MEMMAN_FREES + 1].next = ptr;
            }
        }
        //然后当前结点加入空余链表存着
        ptr->next = man->free[MEMMAN_FREES+1].next;
        man->free[MEMMAN_FREES+1].next = ptr;
        return a;
    }
    insert_size(&man->root_size, ptr);
    return a;
}
//我的双向链表+双RBT实现版本
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size){
    //这一步是找到正确的归还位置
    struct FREEINFO* ptr =  my_findNode_addr(&man->root_addr , addr);
    if(ptr == 0){
        //如果找到的结果ptr == 0
        //那么说明需要新插入结点到最末尾
        //空余链表的pre指向可用链表的末位置（作为尾指针来用）
        ptr = man->free[MEMMAN_FREES+1].pre;
        //先确定尾指针指向的不是头结点
        if(ptr != &man->free[MEMMAN_FREES] && addr == ptr->addr + ptr->size){
            //如果新插入的结点可以直接合并到最末尾结点就不要插入了
            //修改之后RBT依然有序，无需重新插入RBT
            //地址变动就修改
			delete_size(&man->root_size, ptr); 
            ptr->size += size;
            insert_size(&man->root_size, ptr);
            return 0;
        }
        ptr->next = man->free[MEMMAN_FREES+1].next;
        //把新结点从空余链表中分离出来
        man->free[MEMMAN_FREES+1].next = man->free[MEMMAN_FREES+1].next->next;
        //对新结点写入数据
        ptr->next->addr = addr;
        //地址变动就修改
        delete_size(&man->root_size, ptr->next); 
        ptr->next->size = size;
        insert_size(&man->root_size, ptr->next);
        ptr->next->next = 0;
        if(ptr == &man->free[MEMMAN_FREES]){
            //如果是头结点的话，新结点pre不要指向头结点，这是为了合并的时候不要合并到头结点
            ptr->next->pre = 0;
        }else{
            ptr->next->pre = ptr;
        }
        //更新尾指针
        man->free[MEMMAN_FREES+1].pre = ptr->next;
        insert_addr(&man->root_addr,ptr->next);//RBT插入
        return 0;
    } else {
        //有结果就尝试先往后合并（合并到ptr结点）
        if(addr + size == ptr->addr){
            //修改之后RBT依然有序，无需重新插入RBT
            ptr->addr = addr;
            //地址变动就修改
            delete_size(&man->root_size, ptr); 
            ptr->size += size;
        }
        //再尝试往前合并
        if(ptr->pre){
            //如果可以往前合并
            if(ptr->pre->addr + ptr->pre->size == addr){
                //是否已经合并到ptr结点
                if(addr == ptr->addr){
                	//地址变动就修改
                	delete_size(&man->root_size, ptr->pre); 
                    ptr->pre->size += ptr->size;
                    insert_size(&man->root_size, ptr->pre);
                    //已经合并到ptr结点，就要把ptr结点从RBT中删去
                    delete_addr(&man->root_addr,ptr);
                    //把ptr结点回收到空余链表中去
                    ptr->pre->next = ptr->next;
                    ptr->next->pre = ptr->pre;
                    ptr->next = man->free[MEMMAN_FREES+1].next;
                    man->free[MEMMAN_FREES+1].next = ptr;
                }else{
                    //没有合并到ptr结点，就合并到ptr->pre结点就行
                    
                    //地址变动就修改
                	delete_size(&man->root_size, ptr->pre); 
                    ptr->pre->size += ptr->size;
                    insert_size(&man->root_size, ptr->pre);
                }
                return 0;
            } else {
                //是否已经合并到ptr结点
                if(addr == ptr->addr){
                    //已经合并到ptr结点，不用再新增结点 
                    insert_size(&man->root_size, ptr);
                    return 0;
                }
                //如果不可以往前合并，又没有合并到ptr结点，那就只能插入新结点了
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
                    insert_size(&man->root_size, ptr->pre);
                    insert_addr(&man->root_addr,ptr->pre);//RBT插入
                    return 0;
                }
            }
        }
        //位于第一个结点，没办法往前合并
        //判断是否已经合并到ptr结点
        if(addr == ptr->addr){
            //已经合并到ptr结点，不用再新增
            insert_size(&man->root_size, ptr);
            return 0;
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
            insert_size(&man->root_size, ptr->pre);
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

