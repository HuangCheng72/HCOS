## 双向链表与RBT，优化内存管理功能（中）

author: HC        2022/08/09

本篇基于上篇内容所实现的双向链表版本的内存管理功能，进行第二次修改，在这次修改中，我们引入了红黑树这种数据结构，实现将内存释放功能的时间复杂度降低到O(log n)。

在上一篇内容中，我们基于双向链表这种数据结构所实现的释放内存，主要有两个步骤，一是寻找到合适的位置（时间复杂度O(n)），二是插入新的结点或者归纳入原有的结点（时间复杂度O(1)）。释放内存操作总的时间复杂度为O(n)。

如果我们想要降低总的时间复杂度，应当优化寻找到合适的位置这一步骤。原先我们采用作者的思路，即将链表结点以addr为键值按升序排序，而后直接遍历，找到**第一个**满足**“结点的addr大于指定的addr”**这一条件的结点，其位置便是内存信息要插入的位置（插入新结点时，这个结点就是新结点的后继，可以归纳入原有结点时，先考虑归纳入这个结点）。所以，优化寻找到合适的位置这个步骤，本质上是改进线性查找算法的问题。

我们都学过经典的二分查找算法（如果没学过可以看看百度百科了解一下）。在一个有序数组中，二分查找可以在O(log n)的时间复杂度内找到指定键值的元素。数组和链表同属于线性表，理论上链表自然也可以用二分查找。但是我们要注意到，数组访问易而增删难，链表则是增删易访问难。查找算法只使用了访问，而没有增删，因此对链表直接套用二分查找并不合理。

因此，我们可以考虑引入一种运用了二分查找思想的数据结构来帮助我们实现这一目标。二叉搜索树便是一种运用了二分查找思想的数据结构，在二叉搜索树中查找一个符合条件的结点，时间复杂度为O(log n)。我们可以将在链表结点中串入二叉搜索树的结点，在访问时通过强转指针类型，达到在O(1)的时间复杂度内访问结点数据，因此便可实现了对链表运用二分查找。

普通的二叉搜索树BST，有因为失衡而造成查找效率低下的问题，甚至可能退化为链表而使得查找的时间复杂度退化为O(n)。因此我们需要考虑平衡二叉树，而平衡二叉树中最常见的便是AVL树和红黑树了。考虑到效率，我最终决定采用红黑树这种数据结构。

如果是对红黑树不了解的朋友，请移步[skywang12345](https://www.cnblogs.com/skywang12345/)前辈的博客，本文所使用的红黑树代码便是改自skywang12345前辈的红黑树代码，十分感谢skywang12345前辈！

[红黑树(一)之 原理和算法详细介绍 ](https://www.cnblogs.com/skywang12345/p/3245399.html)

skywang12345前辈的这一博文中有这样一句话**“红黑树的时间复杂度为: O(lgn)”**。用红黑树结合链表，理论上来说寻找到满足条件的结点可以在O(log n)的时间复杂度内完成，而且红黑树本身就是一种二叉搜索树，完全可以套用二叉搜索树的查找代码。

我们完全可以建立一棵以addr为键（key）的红黑树，在红黑树上查找，而不在链表上查找！

首先，我们先导入红黑树。

[红黑树(三)之 Linux内核中红黑树的经典实现](https://www.cnblogs.com/skywang12345/p/3624202.html)

将skywang12345前辈在上文中所提供rbtree.h和rbtree.c保存下来，复制粘贴到系统文件夹中（以30_day的harib27f为例，则是其中haribote文件夹），然后修改Makefile。

![](pic\memory2.png)

以30_day的harib27f为例，将其中haribote文件夹中的Makefile文件修改，添加rbtree.obj。

```makefile
OBJS_BOOTPACK = bootpack.obj naskfunc.obj hankaku.obj graphic.obj dsctbl.obj \
		int.obj fifo.obj keyboard.obj mouse.obj memory.obj sheet.obj timer.obj \
		mtask.obj window.obj console.obj file.obj tek.obj \
		rbtree.obj

```

如上所示，添加了一个换行符和一个rbtree.obj。

我们将红黑树结点串入链表结点中，并在内存管理结构体中加入一个红黑树的根结点结构体。

将memory.h修改如下：

```c
#include "rbtree.h" 	//导入红黑树

#define MEMMAN_FREES		673	/* 大约32KB */
#define MEMMAN_ADDR			0x003c0000    //这里指定的是MEMMAN这个结构体放置的内存空间开始地址 
#define EFLAGS_AC_BIT		0x00040000
#define CR0_CACHE_DISABLE	0x60000000

//尝试用双向链表思想修改内存管理 
struct FREEINFO {	/* 可用信息 */
	unsigned int addr, size;
	struct rb_node rb_node_addr;    // 地址红黑树结点
	struct FREEINFO* pre;
	struct FREEINFO* next; //可用信息结点的pre指针和next指针 
};

struct MEMMAN {		/* 内存管理 */
	int lostsize, losts;
	struct rb_root root_addr; //地址红黑树根结点结构体 
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

接下来我们要利用红黑树结合双向链表，对释放内存这一操作进行改写，降低其时间复杂度到O(log n)。

前文已述，寻找到合适的位置，核心问题是如何找到升序链表中的**第一个**满足**“结点的addr大于指定的addr”**这一条件的结点。而结合了红黑树之后，这个问题其实可以转化为，**“在二叉搜索树中找出一个大于指定键值的最小键值”**，也就是在红黑树中查找到addr大于指定addr的所有结点中addr最小的那一个结点。

二叉搜索树的性质是左孩子键值小于自身键值，右孩子键值大于自身键值。若是在二叉搜索树中找出一个大于指定键值的最小键值，可以采用递归或者迭代的方法。其思路如下：

1. 当前结点的键值若小于指定键值，则转到右孩子结点。
2. 当前结点的键值若大于指定键值，则判断有没有左孩子结点。
   1. 如果没有左孩子结点，那么要找的结点就是当前结点。
   2. 如果左孩子结点键值小于指定键值，那么要找的结点就是当前结点。

红黑树也是二叉搜索树，这一思路也完全通用，**我们建立的红黑树的键值是结点的addr值**。

因此查找符合条件的代码如下：

memory.c

```c
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

```

而后，便要考虑红黑树结点的插入和删除了。

这里红黑树结点的插入和删除，代码改自skywang12345前辈的红黑树示例，但是本人经过思考，将删除结点的功能的时间复杂度从O(log n)降低到了O(1)。

memory.c

```c
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
    //先判断node是否在树中
    //判断node是否有父结点或者子结点
    if(rb_parent(&node->rb_node_addr) == 0 && node->rb_node_addr.rb_left == 0 && node->rb_node_addr.rb_right == 0){
        //三个都没有，那么就判断是否为根结点
        if(root->rb_node != &node->rb_node_addr){
            //不是根结点，说明根本不在树中
            return;
        }
    }
    //否则就在树中，从红黑树中删除节点mynode
    rb_erase(&node->rb_node_addr, root);
    return;
}

```

在skywang12345前辈的示例代码中，删除结点之前需要判断结点是否在树中，用的是和插入结点相同的寻找插入位置的方法，如果位置上有该结点，则说明在树中可以删除。而我考虑到skywang12345前辈给出的Linux内核红黑树结点中保存了如下信息：

rbtree.h

```c
struct rb_node
{
    unsigned long  rb_parent_color;
#define    RB_RED        0
#define    RB_BLACK    1
    struct rb_node *rb_right;
    struct rb_node *rb_left;
};
```

在一个结点中保存了父结点，左右孩子结点的信息。

那么，一个结点若在树中，有这几种情况（确定内存中只维护一棵树，也即只有一个根结点）。

1. 结点的父结点不为空。
2. 结点的左孩子不为空。
3. 结点的右孩子不为空。
4. 结点的父结点，左孩子，右孩子均为空，但是结点就是根结点。

符合以上条件之一则说明结点在树中，如果均不符合说明不在树中。

因此，运用以上思路改写，可以将删除红黑树结点的时间复杂度降低到O(1)。

红黑树的作用和建立方法已经分析完毕，我们要在内存管理中使用，应当先初始化，在memory.c中添加一句代码，建立红黑树的根结点。

```c
//初始化内存管理，我的双向链表+RBT实现（RBT初始化）
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
    man->root_addr = RB_ROOT; //初始化根结点，添加了这句
    return;
}
```

建立了红黑树之后，也要对申请内存和释放内存两个函数进行修改。

申请内存部分修改比较简单，只需要在ptr->size清零，需要删除链表结点时删除红黑树结点即可。修改后，申请内存的时间复杂度为O(n)。

memory.c

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

有些读者可能会疑惑，为什么在申请内存时，除了需要删除结点的情形，其他情形下作为键值的addr已经发生了改变，红黑树却不需要调整或者重新插入呢？

这里我用图示回答：

![](pic\memory3.png)

若我们对addr为100的结点增加addr，减少size，假设申请size为50的内存，那么addr = addr + size = 150。

此时情况为：

![](pic\memory4.png)

可以看到，红黑树结构完全没变动，依然维持其性质。

这一个问题可能需要思考，这里我给读者一点提示：

**我们在上篇实现的双向链表内存管理中已经保证了某一结点的addr + size < 后继结点的addr绝对成立。**

释放内存部分修改较多，我们引用一下上篇我们分析基于链表释放内存的8种情形：

> 而基于链表实现的释放内存分情形如下：
>
> 1. 释放的内存数据不可与前后合并，也就是直接插入一个元素。
>    1. 释放的内存数据应位于可用数据的中部。
>    2. 释放的内存数据应位于可用数据的头部（也就是可用链表头结点的next所指向的结点）。
>    3. 释放的内存数据应位于可用数据的尾部（也就是空余链表头结点的pre所指向的结点）。
> 2. 释放的内存数据可以与前后合并。
>    1. 释放的内存数据应为与可用数据的中部。
>       1. 释放的内存数据可以与后合并，但是不可与前合并，则直接归纳到后面结点即可。
>       2. 释放的内存数据可以与前合并，但是不可与后合并，则直接归纳到前面结点即可。
>       3. 释放的内存数据可以与前后合并，那么就先与后合并，归纳到后面结点，然后再一同归纳到前面结点，然后再删除后面结点。
>    2. 释放的内存数据应为与可用数据的头部（第一个结点，是可用链表头结点的后继，且其pre为0）。
>       1. 释放的内存数据只能与后合并，直接归纳到后面结点即可。
>    3. 释放的内存数据应为与可用数据的尾部（最后一个结点，是空余链表头结点的pre所指向的结点，其next为0）。
>       1. 释放的内存数据只能与前合并，直接归纳到前面结点即可。

在这8种情况中。只有1.的三种情形和2.1.3的情形需要变动红黑树，别的情形并不需要变动红黑树。

原因如申请内存中我所给的图示一般，无需多加赘述。

因此修改释放内存的代码如下：

memory.c

```c
//我的双向链表+RBT实现版本
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
                    ptr->pre->size += ptr->size;
                    //已经合并到ptr结点，就要把ptr结点从RBT中删去
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
                //是否已经合并到ptr结点
                if(addr == ptr->addr){
                    //已经合并到ptr结点，不用再新增
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
                    insert_addr(&man->root_addr,ptr->pre);//RBT插入
                    return 0;
                }
            }
        }
        //位于第一个结点，没办法往前合并
        //判断是否已经合并到ptr结点
        if(addr == ptr->addr){
            //已经合并到ptr结点，不用再新增
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

在本次修改中，本文开头所述的释放内存的两个步骤：

> 一是寻找到合适的位置（时间复杂度O(n)），二是插入新的结点或者归纳入原有的结点（时间复杂度O(1)）。

中的第一个步骤，也就是寻找到合适的位置，由于我们是在红黑树上查找的，因此时间复杂度为O(log n)。而在第二个步骤中，只有变动结点的情形才需要变动红黑树，如果插入红黑树结点则时间复杂度为O(log n)，如果删除红黑树结点则时间复杂度为O(1)。因此第二个步骤最坏时间复杂度为O(log n)。因此总的来说，释放内存的时间复杂度已经降低为O(log n)。

修改完毕，编译运行即可。

欢迎访问我的GitHub仓库，如果对您有帮助，麻烦给一个star，谢谢！

[GitHub - HuangCheng72/HCOS](https://github.com/HuangCheng72/HCOS)