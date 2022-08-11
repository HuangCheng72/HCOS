## 双向链表与RBT，优化内存管理功能（下）

author: HC        2022/08/12

篇前说明：本篇其实是我的脑洞之作，改进的时候只是想到这么个思路就去做了。虽说最后理论上的时间复杂度是O(log n)，实际上因为频繁变动红黑树，插入删除红黑树结点又要着色、删改等，最后的实际效果也许可能尚不如时间复杂度O(n)的原方法。但是我认为在这一改进过程中，我对经典数据结构的修改以适应特殊场景需要的思考和实践，是我最大的收获，也是我所想呈现给读者的东西。

在上一篇也就是中篇中，我们使用双向链表+红黑树所实现的内存管理，申请内存的时间复杂度为O(n)，释放内存的时间复杂度为O(log n)。

我们能不能更近一步，将申请内存的时间复杂度也降低到O(log n)呢？

与释放内存类似的，申请内存也主要有两个步骤，一是寻找到合适的位置（时间复杂度O(n)），二是删除或者修改原有的结点（时间复杂度O(1)）。

降低申请内存总的时间复杂度，也应当在寻找到合适的位置这一个步骤上优化。

上一篇中的申请内存的代码：

```c
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
                //把这个结点从RBT中删除
                delete_addr(&man->root_addr , ptr);
                //断开本结点和前后的联系
                if(ptr->pre && ptr->next){
                    //不是第一个结点或者最后一个结点
                    ptr->pre->next = ptr->next;
                    ptr->next->pre = ptr->pre;
                }else{
                    if(ptr->pre == 0){
                        //第一个结点
                        man->free[MEMMAN_FREES].next = ptr->next;
                        ptr->next->pre = 0;
                    }else{
                        //最后一个结点
                        ptr->pre->next = 0;
                        man->free[MEMMAN_FREES+1].pre = ptr->pre;
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

分析代码思路，我们可以知道，申请内存在寻找到合适的位置这一个步骤上所要寻找的是第一个size > 指定size值的结点，这个过程也称为首次适应查找。这种做法也有造成浪费内存的问题，可能会制造出较多的内存碎片，无法达到内存利用率最大化。但是若要达到内存利用率最大化，必须每次都遍历整个链表，寻找到**“size值大于指定size值的所有结点中size值最小的那个结点”**，这个做法也叫做最适应查找。

似乎我们完全可以参考释放内存，再构建一棵以size为key的红黑树用来优化这个过程。而且，若在红黑树上查找，则最适应查找的问题也可以转化为**“在二叉搜索树中找出一个大于指定键值的最小键值”**，又可以套用我们在上一篇中实现的查找函数了。

但是，我们应当注意到，与释放内存所不同的是，**addr在数据中是唯一的，size在数据中是不唯一的**，而在二叉搜索树中，**key必须是唯一的**，在红黑树中自然也一样。

所以，我们不能直接简单套用红黑树，要**针对这一特殊的应用场景进行改写和优化**。

首先我想到的是，我们可以借鉴哈希表的分离链接法，可以将key（在这棵红黑树中是size）相同的结点直接用链表链起来。

我们在系列的上篇，也就是只使用了双向链表实现了内存管理功能的那一篇中提到。

> 双向链表则因为同时具有pre指针和next指针，不管是插入结点（插入在结点之前或者插入在结点之后，也即头插法和尾插法）还是删除结点，其时间复杂度均是O(1)。

所以，在这里我们依然使用双向链表。

首先，要修改memory.h，增加第二条双向链表的两个指针pre2和next2，并且在内存管理结构体中加入第二棵红黑树的根结点。

```c
#include "rbtree.h" 

#define MEMMAN_FREES		370	/* 大约32KB */
#define MEMMAN_ADDR			0x003c0000    //这里指定的是MEMMAN这个结构体放置的内存空间开始地址 
#define EFLAGS_AC_BIT		0x00040000
#define CR0_CACHE_DISABLE	0x60000000

//尝试用双向链表思想修改内存管理 
struct FREEINFO {	// 可用信息
	unsigned int addr, size;
    struct rb_node rb_node_addr;    // 地址红黑树结点
    struct rb_node rb_node_size;    // 大小红黑树结点
    struct FREEINFO* pre;
    struct FREEINFO* next; //可用信息结点的pre指针和next指针
    struct FREEINFO* pre2;
    struct FREEINFO* next2; //用于大小红黑树的pre指针和next指针
};

struct MEMMAN {		// 内存管理
	int lostsize, losts;
	struct rb_root root_addr; //地址红黑树根结点结构体 
    struct rb_root root_size; //大小红黑树根结点结构体
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

内存管理初始化修改，增加初始化大小红黑树根结点。

memory.c

```c
//初始化内存管理，我的双向链表+双RBT实现（RBT初始化）
void memman_init(struct MEMMAN *man){
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
    man->root_addr = RB_ROOT; //初始化根结点
    man->root_size = RB_ROOT; //初始化根结点
    return;
}

```

插入结点到size红黑树中，所有的情形：

1. 查找插入位置（时间复杂度为O(log n)），以该size为key的结点不存在于树中，则直接插入结点即可（时间复杂度为O(1)）。则时间复杂度为O(log n)。
2. 查找插入位置（时间复杂度为O(log n)，以该size为key的结点已存在于树中，且已存在的结点不是根结点，则将已存在结点的父结点、颜色、左右孩子等信息全部赋给要插入的结点，而后将已存在的结点的父结点、左右孩子等连接到要插入的结点。插入的结点变成新的在红黑树中的结点，原先已存在的结点变成新结点的后继，并且清空原先已存在的结点在红黑树中的连接信息（时间复杂度为O(1)）。则时间复杂度为O(log n)。
3. 查找插入位置（时间复杂度为O(log n)，以该size为key的结点已存在于树中，且已存在的结点是根结点，则在2的基础上，将红黑树的根结点修改指向该新插入的结点（时间复杂度为O(1)）。则时间复杂度为O(log n)。

因此，插入结点实现如下：

memory.c

```c
//将结点插入大小RBT中
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
            //已经有结点了就头插法链接到现有结点第二条链表上
            node->next2 = my;
            my->pre2 = node;
            //设置父结点和颜色0
            node->rb_node_size.rb_parent_color = my->rb_node_size.rb_parent_color;
            //设置孩子结点
            node->rb_node_size.rb_left = my->rb_node_size.rb_left;
            if(node->rb_node_size.rb_left){
                rb_set_parent(node->rb_node_size.rb_left, &node->rb_node_size);
            }
            node->rb_node_size.rb_right = my->rb_node_size.rb_right;
            if(node->rb_node_size.rb_right){
                rb_set_parent(node->rb_node_size.rb_right, &node->rb_node_size);
            }
            //如果父结点存在
            if(rb_parent(&node->rb_node_size)){
                //确定其系父结点的左孩子还是右孩子
                if(rb_parent(&node->rb_node_size)->rb_left == &my->rb_node_size){
                    rb_parent(&node->rb_node_size)->rb_left = &node->rb_node_size;
                } else {
                    rb_parent(&node->rb_node_size)->rb_right = &node->rb_node_size;
                }
            }else{
                //不存在父结点说明是根结点，那么就要更新根结点指针指向的结点
                struct MEMMAN *memman = (struct MEMMAN *)MEMMAN_ADDR;
                memman->root_size.rb_node = &node->rb_node_size;
            }
            //清除my结点在二叉树中的连接信息
            my->rb_node_size.rb_parent_color = 0;
            my->rb_node_size.rb_left = 0;
            my->rb_node_size.rb_right = 0;
            return;
        }
    }
    //连接要插入的结点和父结点
    rb_link_node(&node->rb_node_size, parent, tmp);
    //插入该结点，并进行调整
    rb_insert_color(&node->rb_node_size, root);
    return;
}

```

删除size红黑树中的结点，所有的情形：

1. 判断以该size为key的结点存在于树中（时间复杂度为O(1)），在红黑树中连接的的结点不是这个结点，这个结点在第二条链表中，则直接在链表中删除该结点即可（时间复杂度为O(1)）。则时间复杂度为O(1)。
2. 判断以该size为key的结点存在于树中（时间复杂度为O(1)），且在红黑树中连接的的结点就是这个结点，这个结点无后继，则直接红黑树中删除该结点即可（时间复杂度为O(1)）。则时间复杂度为O(1)。
3. 判断以该size为key的结点存在于树中（时间复杂度为O(1)），且在红黑树中连接的的结点就是这个结点，这个结点有后继，且不为根结点，则将该结点的父结点、颜色、左右孩子等信息全部赋给其后继的结点，并且将该结点的父结点、左右孩子等连接到替代该结点位置的后继结点，然后清空该结点在红黑树中的连接信息（时间复杂度为O(1)）。则时间复杂度为O(1)。
4. 判断以该size为key的结点存在于树中（时间复杂度为O(1)），且在红黑树中连接的的结点就是这个结点，这个结点有后继，且为根结点，则在3的基础上更新根结点指向即可（时间复杂度为O(1)）。则时间复杂度为O(1)。

因此，删除size红黑树中的结点实现如下：

memory.c

```c
//删除大小红黑树中的结点
void delete_size(struct rb_root *root, struct FREEINFO* node){
    //防止出现删除不存在结点的问题
    //先确定结点是否存在二叉树中
    if(node->pre2 == 0 && node->next2 == 0){
        //判断是否有父结点子结点
        if(rb_parent(&node->rb_node_size) == 0 && node->rb_node_size.rb_left == 0 && node->rb_node_size.rb_right == 0){
            //没有父结点子结点就判断是否为根结点
            if(root->rb_node != &node->rb_node_size){
                //不是根结点，说明根本不在树中
                return;
            }
        }
    }
    //判断是否为第二条链表中的元素（不在树中，在链表中）
    if(node->pre2){
        //pre2不为空，说明其是第二条链表中的元素
        //删除链表元素即可
        node->pre2->next2 = node->next2;
        if(node->next2){
            node->next2->pre2 = node->pre2;
        }
        //删除之后置空两个指针
        node->pre2 = 0;
        node->next2 = 0;
        return;
    }
    //在树中的元素
    if(node->next2){
        //链表后面还有元素，就替换
        //设置父结点和颜色
        node->next2->rb_node_size.rb_parent_color = node->rb_node_size.rb_parent_color;
        //设置孩子结点
        node->next2->rb_node_size.rb_left = node->rb_node_size.rb_left;
        if(node->next2->rb_node_size.rb_left){
            rb_set_parent(node->next2->rb_node_size.rb_left, &node->next2->rb_node_size);
        }
        node->next2->rb_node_size.rb_right = node->rb_node_size.rb_right;
        if(node->next2->rb_node_size.rb_right){
            rb_set_parent(node->next2->rb_node_size.rb_right, &node->next2->rb_node_size);
        }
        //如果父结点存在
        if(rb_parent(&node->rb_node_size) != 0){
            //确定其系父结点的左孩子还是右孩子
            if(rb_parent(&node->rb_node_size)->rb_left == &node->rb_node_size){
                rb_parent(&node->rb_node_size)->rb_left = &node->next2->rb_node_size;
            } else {
                rb_parent(&node->rb_node_size)->rb_right = &node->next2->rb_node_size;
            }
        }else{
            //不存在父结点说明是根结点，那么就要更新根结点指针指向的结点
            struct MEMMAN *memman = (struct MEMMAN *)MEMMAN_ADDR;
            memman->root_size.rb_node = &node->rb_node_size;
        }
        //前驱置空
        node->next2->pre2 = 0;
        //清除该结点全部连接信息
        node->rb_node_size.rb_parent_color = 0;
        node->rb_node_size.rb_left = 0;
        node->rb_node_size.rb_right = 0;
        node->pre2 = 0;
        node->next2 = 0;
    }else{
        rb_erase(&node->rb_node_size, root);
    }
    return;
}

```

而套用之前的查找函数，所实现的最适应查找如下：

memory.c

```c
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
            return mynode;
        }
        if(mynode->rb_node_size.rb_left == 0 ||  container_of(mynode->rb_node_size.rb_left, struct FREEINFO, rb_node_size)->size < size ){
            //左孩子不存在，或者， 左孩子小于，本结点大于，那就只能是这个了
            return mynode;
        }
        return my_search_size(mynode->rb_node_size.rb_left , size);
    }
}
struct FREEINFO* my_findNode_size(struct rb_root *root, unsigned int size){
    return my_search_size(root->rb_node, size);
}

```

然后就到了申请内存的优化了，将遍历查找替换为我们的查找函数即可，但是要注意，若结点的size将发生改变，必须先删除红黑树中的结点（时间复杂度为O(1)），再更改结点size（时间复杂度为O(1)），再重新插入该结点（时间复杂度为O(log n)），在释放内存中的修改同理。

修改申请内存如下，则时间复杂度优化为O(log n)：

memory.c

```c
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
                //ptr的pre和next都是0，就是说要删除这个唯一的结点
                man->free[MEMMAN_FREES+1].pre = &man->free[MEMMAN_FREES];
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

```

修改释放内存如下，时间复杂度不变：

memory.c

```c
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
            //大小变动就修改
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
        //大小变动就修改
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
            //大小变动就修改
            delete_size(&man->root_size, ptr); 
            ptr->size += size;
        }
        //再尝试往前合并
        if(ptr->pre){
            //如果可以往前合并
            if(ptr->pre->addr + ptr->pre->size == addr){
                //是否已经合并到ptr结点
                if(addr == ptr->addr){
                	//大小变动就修改
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
                    //大小变动就修改
                	delete_size(&man->root_size, ptr->pre); 
                    ptr->pre->size += size;
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

```

修改完毕，编译运行即可。

欢迎访问我的GitHub仓库，如果对您有帮助，麻烦给一个star，谢谢！

[GitHub - HuangCheng72/HCOS](https://github.com/HuangCheng72/HCOS)