#include "bootpack.h"

//�ڴ�������� 
unsigned int memtest(unsigned int start, unsigned int end){
	char flg486 = 0;
	unsigned int eflg, cr0, i;

	//ȷ��CPU��386����486�����ϵ� 
	eflg = io_load_eflags();
	eflg |= EFLAGS_AC_BIT; /* AC-bit = 1 */
	io_store_eflags(eflg);
	eflg = io_load_eflags();
	if ((eflg & EFLAGS_AC_BIT) != 0) { /* �����386����ʹ��������趨��AC���ǻ�ص�0 */
		flg486 = 1;
	}
	eflg &= ~EFLAGS_AC_BIT; /* AC-bit = 0 */
	io_store_eflags(eflg);

	if (flg486 != 0) {
		cr0 = load_cr0();
		cr0 |= CR0_CACHE_DISABLE; /* ��ʱ��ֹ���棨����Ϊ�˼����ڴ����Ҫ�� */
		store_cr0(cr0);
	}
	
	//��μ�������ڴ泤�ȱ���ʹ�û���д������鿴naskfunc.nas������P166ҳ��C���ԵĴ��루��ϧ���ܱ��뵽������Ҫ�Ľ���� 
	i = memtest_sub(start, end);

	if (flg486 != 0) {
		cr0 = load_cr0();
		cr0 &= ~CR0_CACHE_DISABLE; /* ������ */
		store_cr0(cr0);
	}

	return i;
}


//�ݹ鷨����"RBT"��addrֵ���ڵ���ָ��addrֵ�Ľڵ㡣û�ҵ��Ļ�������0��
struct FREEINFO* my_search_addr(struct rb_node* rbnode, unsigned int addr){
    if(rbnode == 0){
        return 0;
    }
    struct FREEINFO* mynode = container_of(rbnode, struct FREEINFO, rb_node_addr);
    if(mynode->addr < addr){
        return my_search_addr(mynode->rb_node_addr.rb_right , addr);
    }else{
        //�ҵ�һ�����ڵ��ڵĽ��
        if(mynode->addr == addr){
            //���ھ�ֱ�ӷ���
            return mynode;
        }
        if(mynode->rb_node_addr.rb_left == 0 ||  container_of(mynode->rb_node_addr.rb_left, struct FREEINFO, rb_node_addr)->addr < addr ){
            //���Ӳ����ڣ����ߣ� ����С�ڣ��������ڣ��Ǿ�ֻ���������
            return mynode;
        }
        return my_search_addr(mynode->rb_node_addr.rb_left , addr);
    }
}
struct FREEINFO* my_findNode_addr(struct rb_root *root, unsigned int addr){
    return my_search_addr(root->rb_node, addr);
}
//��������RBT��
void insert_addr(struct rb_root *root, struct FREEINFO* node){
    struct rb_node **tmp = &(root->rb_node), *parent = 0;
    //Ѱ�Ҳ���λ��
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
    //����Ҫ����Ľ��͸����
    rb_link_node(&node->rb_node_addr, parent, tmp);
    //����ý�㣬�����е���
    rb_insert_color(&node->rb_node_addr, root);
    return;
}

//RBTɾ��ĳ�����
void delete_addr(struct rb_root *root, struct FREEINFO* node){
    //��ֹ����ɾ�������ڽ�������
    //�Ȳ���
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
    //�Ӻ������ɾ���ڵ�mynode
    rb_erase(&node->rb_node_addr, root);
    return;
}

//�ݹ鷨����"RBT"��sizeֵ���ڵ���ָ��sizeֵ�Ľڵ㡣û�ҵ��Ļ�������0��
struct FREEINFO* my_search_size(struct rb_node* rbnode, unsigned int size){
    if(rbnode == 0){
        return 0;
    }
    struct FREEINFO* mynode = container_of(rbnode, struct FREEINFO, rb_node_size);
    if(mynode->size < size){
        return my_search_size(mynode->rb_node_size.rb_right , size);
    }else{
        //�ҵ�һ�����ڵ��ڵĽ��
        if(mynode->size == size){
            //���ھ�ֱ�ӷ���
            if(mynode->next2){
                //������������ģ���Ҫ����䶯�����
                return mynode->next2;
            }
            return mynode;
        }
        if(mynode->rb_node_size.rb_left == 0 ||  container_of(mynode->rb_node_size.rb_left, struct FREEINFO, rb_node_size)->size < size ){
            //���Ӳ����ڣ����ߣ� ����С�ڣ��������ڣ��Ǿ�ֻ���������
            if(mynode->next2){
                //������������ģ���Ҫ����䶯�����
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
//��������RBT��
void insert_size(struct rb_root *root, struct FREEINFO* node){
    if(node->pre2 || node->next2){
        //��ֹ�ظ�����
        return;
    }
    struct rb_node **tmp = &(root->rb_node), *parent = 0;
    //Ѱ�Ҳ���λ��
    while (*tmp){
        struct FREEINFO* my = container_of(*tmp, struct FREEINFO, rb_node_size);
        parent = *tmp;
        if (node->size < my->size)
            tmp = &((*tmp)->rb_left);
        else if (node->size > my->size)
            tmp = &((*tmp)->rb_right);
        else{
            //�Ѿ��н���˾����ӵ�������ȥ
            node->next2 = my->next2;
            if(node->next2){
                //������滹�н��Ļ�������ǰ��
                node->next2->pre2 = node;
            }
            node->pre2 = my;
            my->next2 = node;
            return;
        }
    }
    //����Ҫ����Ľ��͸����
    rb_link_node(&node->rb_node_size, parent, tmp);
    //����ý�㣬�����е���
    rb_insert_color(&node->rb_node_size, root);
    return;
}

//RBTɾ��ĳ�����
void delete_size(struct rb_root *root, struct FREEINFO* node){
    //��ֹ����ɾ�������ڽ�������
    //�Ȳ���
    struct rb_node *rbnode = root->rb_node;
    while (rbnode != 0){
        struct FREEINFO* mynode = container_of(rbnode, struct FREEINFO, rb_node_size);
        if (node->size < mynode->size)
            rbnode = rbnode->rb_left;
        else if (node->size > mynode->size)
            rbnode = rbnode->rb_right;
        else if(node->size == mynode->size){
            if(mynode->next2 == 0 && mynode->pre2 == 0){
                //������ں�����еĽ�㣬��ʱֻ�������㣬ֱ�ӴӺ������ɾ������
                rb_erase(&node->rb_node_size, root);
                return;
            }else{
                //��������ں�����е��Ǹ���㣬ֱ��ɾ��������
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

//��ʼ���ڴ�����ҵ�˫������+˫RBTʵ�֣�RBT��ʼ����
void memman_init(struct MEMMAN *man){
    man->lostsize = 0;		// �ͷ�ʧ�ܵ��ڴ��С�ܺͣ�Ҳ���Ƕ�ʧ����Ƭ
    man->losts = 0;			// �ͷ�ʧ�ܵĴ���
    man->root_addr = RB_ROOT; //��ʼ�������
    man->root_size = RB_ROOT; //��ʼ�������
    //���������ϵ�������������ͷ�巨�����㣬��ͷɾ��ɾ�����
    //�ȳ�ʼ������ͷ���
    man->free[MEMMAN_FREES].next = 0;
    man->free[MEMMAN_FREES].pre = 0;
    man->free[MEMMAN_FREES+1].next = &man->free[0];
    //Ϊ��ʵ��������룬�ÿ��������pre�洢������������һ�����ָ�룬ʵ��β�巨
    man->free[MEMMAN_FREES+1].pre = &man->free[MEMMAN_FREES];
    //һ��ʼȫ�ǿ�����������ֱ�Ӵ�ͷ��ʼ���󹹽���ϵ��ֻ�����洢���������þ��У�
    int i;
    for(i = 1; i < MEMMAN_FREES; i++){
        man->free[i-1].next = &man->free[i];
    }
    man->free[MEMMAN_FREES-1].next = 0;
    return;
}
//�����ڴ�ϼ��Ƕ��� ���ҵ�˫������ʵ��
unsigned int memman_total(struct MEMMAN *man){
    //�ĳ�����汾Ҳһ��ֱ��ѭ��������������
    unsigned int t = 0;
    struct FREEINFO* ptr =  man->free[MEMMAN_FREES].next;
    while(ptr){
        t += ptr->size;
        ptr = ptr->next;
    }
    return t;
}
//�ڴ����룬�ҵ�˫������+˫RBTʵ�ְ汾
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
        //���������RBT��ɾ��
        delete_addr(&man->root_addr , ptr);
        //�Ͽ�������ǰ�����ϵ
        if(ptr->pre && ptr->next){
            //���ǵ�һ�����������һ�����
            ptr->pre->next = ptr->next;
            ptr->next->pre = ptr->pre;
        }else{
            if(ptr->pre == 0 && ptr->next){
                //��һ�����
                man->free[MEMMAN_FREES].next = ptr->next;
                ptr->next->pre = 0;
            }else if(ptr->pre && ptr->next == 0){
                //���һ�����
                ptr->pre->next = 0;
                man->free[MEMMAN_FREES+1].pre = ptr->pre;
            }else{
                //ptr��pre��next����0������˵Ҫɾ�����Ψһ�Ľ�㣬�Ǿͽ���������������
                man->free[MEMMAN_FREES+1].pre = &man->free[MEMMAN_FREES];
                ptr->next2 = man->free[MEMMAN_FREES + 1].next;
                man->free[MEMMAN_FREES + 1].next = ptr;
            }
        }
        //Ȼ��ǰ����������������
        ptr->next = man->free[MEMMAN_FREES+1].next;
        man->free[MEMMAN_FREES+1].next = ptr;
        return a;
    }
    insert_size(&man->root_size, ptr);
    return a;
}
//�ҵ�˫������+˫RBTʵ�ְ汾
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size){
    //��һ�����ҵ���ȷ�Ĺ黹λ��
    struct FREEINFO* ptr =  my_findNode_addr(&man->root_addr , addr);
    if(ptr == 0){
        //����ҵ��Ľ��ptr == 0
        //��ô˵����Ҫ�²����㵽��ĩβ
        //���������preָ����������ĩλ�ã���Ϊβָ�����ã�
        ptr = man->free[MEMMAN_FREES+1].pre;
        //��ȷ��βָ��ָ��Ĳ���ͷ���
        if(ptr != &man->free[MEMMAN_FREES] && addr == ptr->addr + ptr->size){
            //����²���Ľ�����ֱ�Ӻϲ�����ĩβ���Ͳ�Ҫ������
            //�޸�֮��RBT��Ȼ�����������²���RBT
            //��ַ�䶯���޸�
			delete_size(&man->root_size, ptr); 
            ptr->size += size;
            insert_size(&man->root_size, ptr);
            return 0;
        }
        ptr->next = man->free[MEMMAN_FREES+1].next;
        //���½��ӿ��������з������
        man->free[MEMMAN_FREES+1].next = man->free[MEMMAN_FREES+1].next->next;
        //���½��д������
        ptr->next->addr = addr;
        //��ַ�䶯���޸�
        delete_size(&man->root_size, ptr->next); 
        ptr->next->size = size;
        insert_size(&man->root_size, ptr->next);
        ptr->next->next = 0;
        if(ptr == &man->free[MEMMAN_FREES]){
            //�����ͷ���Ļ����½��pre��Ҫָ��ͷ��㣬����Ϊ�˺ϲ���ʱ��Ҫ�ϲ���ͷ���
            ptr->next->pre = 0;
        }else{
            ptr->next->pre = ptr;
        }
        //����βָ��
        man->free[MEMMAN_FREES+1].pre = ptr->next;
        insert_addr(&man->root_addr,ptr->next);//RBT����
        return 0;
    } else {
        //�н���ͳ���������ϲ����ϲ���ptr��㣩
        if(addr + size == ptr->addr){
            //�޸�֮��RBT��Ȼ�����������²���RBT
            ptr->addr = addr;
            //��ַ�䶯���޸�
            delete_size(&man->root_size, ptr); 
            ptr->size += size;
        }
        //�ٳ�����ǰ�ϲ�
        if(ptr->pre){
            //���������ǰ�ϲ�
            if(ptr->pre->addr + ptr->pre->size == addr){
                //�Ƿ��Ѿ��ϲ���ptr���
                if(addr == ptr->addr){
                	//��ַ�䶯���޸�
                	delete_size(&man->root_size, ptr->pre); 
                    ptr->pre->size += ptr->size;
                    insert_size(&man->root_size, ptr->pre);
                    //�Ѿ��ϲ���ptr��㣬��Ҫ��ptr����RBT��ɾȥ
                    delete_addr(&man->root_addr,ptr);
                    //��ptr�����յ�����������ȥ
                    ptr->pre->next = ptr->next;
                    ptr->next->pre = ptr->pre;
                    ptr->next = man->free[MEMMAN_FREES+1].next;
                    man->free[MEMMAN_FREES+1].next = ptr;
                }else{
                    //û�кϲ���ptr��㣬�ͺϲ���ptr->pre������
                    
                    //��ַ�䶯���޸�
                	delete_size(&man->root_size, ptr->pre); 
                    ptr->pre->size += ptr->size;
                    insert_size(&man->root_size, ptr->pre);
                }
                return 0;
            } else {
                //�Ƿ��Ѿ��ϲ���ptr���
                if(addr == ptr->addr){
                    //�Ѿ��ϲ���ptr��㣬������������� 
                    insert_size(&man->root_size, ptr);
                    return 0;
                }
                //�����������ǰ�ϲ�����û�кϲ���ptr��㣬�Ǿ�ֻ�ܲ����½����
                //ֱ�Ӵӿ�����������Ҫһ��������
                if(man->free[MEMMAN_FREES+1].next){
                    //���н�����Ǿ��ð�
                    //��pre��nextָ��ָ������Ҫʹ�õ��½ڵ�
                    //����Ҫ��ֹptr�ǵ�һ�������������preΪ0��
                    ptr->pre->next = man->free[MEMMAN_FREES+1].next;
                    //���½ڵ�ӿ��������з������
                    man->free[MEMMAN_FREES+1].next = man->free[MEMMAN_FREES+1].next->next;
                    //�½���nextָ�򱾽�㣬preָ�򱾽���pre
                    ptr->pre->next->next = ptr;
                    ptr->pre->next->pre = ptr->pre;
                    //������preָ���½�㣬�������Ĳ���
                    ptr->pre = ptr->pre->next;
                    //д�����ݣ����ؽ��
                    ptr->pre->addr = addr;
                    ptr->pre->size = size;
                    insert_size(&man->root_size, ptr->pre);
                    insert_addr(&man->root_addr,ptr->pre);//RBT����
                    return 0;
                }
            }
        }
        //λ�ڵ�һ����㣬û�취��ǰ�ϲ�
        //�ж��Ƿ��Ѿ��ϲ���ptr���
        if(addr == ptr->addr){
            //�Ѿ��ϲ���ptr��㣬����������
            insert_size(&man->root_size, ptr);
            return 0;
        }
        //������Ǿ�ֻ������ͷ���ĺ������һ���½����
        if(man->free[MEMMAN_FREES+1].next){
            //���н�����Ǿ��ð�
            //��pre��nextָ��ָ������Ҫʹ�õ��½ڵ�
            //�����preָ���½��
            ptr->pre = man->free[MEMMAN_FREES+1].next;
            //���½ڵ�ӿ��������з������
            man->free[MEMMAN_FREES+1].next = man->free[MEMMAN_FREES+1].next->next;
            //�½���nextָ�򱾽�㣬ͷ���ָ���½�㣬��һ������preΪ0
            man->free[MEMMAN_FREES].next = ptr->pre;
            ptr->pre->next = ptr;
            ptr->pre->pre = 0;
            //д�����ݣ����ؽ��
            ptr->pre->addr = addr;
            ptr->pre->size = size;
            insert_size(&man->root_size, ptr->pre);
            insert_addr(&man->root_addr,ptr->pre);//RBT����
            return 0;
        }
    }
    //ʧ�����ڴ浱Ȼ����
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

