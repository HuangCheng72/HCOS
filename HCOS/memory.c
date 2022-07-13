#include "bootpack.h"

/*
 * ����"RBT"��addrֵ���ڵ���ָ��addrֵ�Ľڵ㡣û�ҵ��Ļ�������0��
 */
struct FREEINFO* my_findNode(struct rb_root *root, unsigned int addr){
    struct rb_node* rbnode = root->rb_node;
    while (rbnode != 0){
        struct FREEINFO* mynode = container_of(rbnode, struct FREEINFO, rb_node_addr);
        if (mynode->addr < addr){
            //�����������������������û�з��������Ľ��
            struct rb_node* temp = &mynode->rb_node_addr;
            //���������Ϊ0���޷��������ң�˵��������
            //�����������Ϊ0��������������
            //���ǵ�ǰ��㲻Ϊ0�ҽ��addr���ڵ���ָ��addr����˴����ǽ��
            while(temp->rb_right != 0){
                temp = temp->rb_right;
                //����ҵ������ֱ�ӷ���
                if(temp != 0 && container_of(temp, struct FREEINFO, rb_node_addr)->addr >= addr){
                    return container_of(temp, struct FREEINFO, rb_node_addr);
                }
            }
            //�ⶼ�Ҳ�����˵��������Ҳ���
            if(temp->rb_right == 0){
                return 0;
            }
        } else if (mynode->addr > addr){
            //�����������������������û�з��������Ľ��
            struct rb_node* temp = &mynode->rb_node_addr;
            //���������Ϊ�㣬�޷��������ң�������ֹ���˴��ǽ��
            //�����������Ϊ�㣬�������ӵ�addrС��ָ��addr���˴����ǽ��
            while(temp->rb_left != 0 && container_of(temp->rb_left, struct FREEINFO, rb_node_addr)->addr >= addr){
                temp = temp->rb_left;
                //�����������Ϊ0����������addrС��ָ����addr�����ҵ�������Զ��˳�ѭ��
                //���������Ϊ0�����ҵ�������Զ��˳�ѭ��
            }
            //�Ҳ���������˴��޸�
            return container_of(temp, struct FREEINFO, rb_node_addr);
        } else {
            //���ڣ��Ǿ���mynode��
            return mynode;
        }
    }
    return 0;
}
//����ַ����RBT��
void insert_addr(struct rb_root *root, struct FREEINFO* node){
    struct rb_node **tmp = &(root->rb_node), *parent = NULL;
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


//��ʼ���ڴ�����ҵ�˫������+RBTʵ�֣�RBT��ʼ���� 
void memman_init(struct MEMMAN *man){
    man->frees = 0;			//������Ϣ��
    man->maxfrees = 0;		// ���ڹ۲����״����frees�����ֵ
    man->lostsize = 0;		// �ͷ�ʧ�ܵ��ڴ��С�ܺͣ�Ҳ���Ƕ�ʧ����Ƭ
    man->losts = 0;			// �ͷ�ʧ�ܵĴ���
    man->root_addr = RB_ROOT; //��ʼ�������
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
//�ڴ����룬�ҵ�˫������ʵ�ְ汾  
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size){
	struct FREEINFO* ptr =  man->free[MEMMAN_FREES].next;
	unsigned int a;
	while(ptr){
		//�ҵ�һ���㹻����ڴ� 
		if (ptr->size >= size) { 
			a = ptr->addr;
			ptr->addr += size;
			ptr->size -= size;
			//���ptr��size�����0�ˣ��ǽ��Ҳû�д��ڵı�Ҫ�ˣ��������������
			if(ptr->size == 0){
				man->frees--;
				//�Ͽ�������ǰ�����ϵ 
				ptr->pre->next = ptr->next;
				ptr->next->pre = ptr->pre;
				//Ȼ��ǰ����������������
				ptr->next = man->free[MEMMAN_FREES+1].next; 
				man->free[MEMMAN_FREES+1].next = ptr;
			}
			return a; 
		}
		ptr = ptr->next;
	}
	//û���ÿռ�ֱ�ӷ���0������ 
	return 0;
}
//�ҵ�˫������+RBTʵ�ְ汾
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size){
    //��һ�����ҵ���ȷ�Ĺ黹λ��
    struct FREEINFO* ptr =  my_findNode(&man->root_addr , addr);
    if(ptr == 0){
        //����ҵ��Ľ��ptr == 0
        //��ô˵����Ҫ�²����㵽��ĩβ
        //���������preָ����������ĩλ�ã���Ϊβָ�����ã�
        ptr = man->free[MEMMAN_FREES+1].pre;
        if(addr == ptr->addr + ptr->size){
            //����²���Ľ�����ֱ�Ӻϲ���βָ��Ͳ�Ҫ������
            //�޸�֮��RBT��Ȼ�����������²���RBT
            ptr->size += size;
            return 0;
        }
        ptr->next = man->free[MEMMAN_FREES+1].next;
        //���½��ӿ��������з������
        man->free[MEMMAN_FREES+1].next = man->free[MEMMAN_FREES+1].next->next;
        //���½��д������
        ptr->next->addr = addr;
        ptr->next->size = size;
        ptr->next->next = 0;
        if(ptr == &man->free[MEMMAN_FREES]){
            //�����ͷ���Ļ����½��pre��Ҫָ��ͷ��㣬����Ϊ�˺ϲ���ʱ��Ҫ�ϲ���ͷ���
            ptr->next->pre = 0;
        }else{
            ptr->next->pre = ptr;
        }
        //����βָ��
        man->free[MEMMAN_FREES+1].pre = ptr->next;
        man->frees++;//ͳ����Ϣ����
        insert_addr(&man->root_addr,ptr->next);//RBT����
        return 0;
    } else {
        //�н���ͳ���������ϲ����ϲ���ptr��㣩
        if(addr + size == ptr->addr){
            //�޸�֮��RBT��Ȼ�����������²���RBT
            ptr->addr = addr;
            ptr->size += size;
        }
        //�ٳ�����ǰ�ϲ�
        if(ptr->pre){
            //���������ǰ�ϲ�
            if(ptr->pre->addr + ptr->pre->size == addr){
                //�Ƿ��Ѿ��ϲ���ptr���
                if(addr == ptr->addr){
                    //�Ѿ��ϲ���ptr��㣬��Ҫ��ptr����RBT��ɾȥ
                    ptr->pre->size += ptr->size;
                    delete_addr(&man->root_addr,ptr);
                    //��ptr�����յ�����������ȥ
                    ptr->pre->next = ptr->next;
                    ptr->next->pre = ptr->pre;
                    ptr->next = man->free[MEMMAN_FREES+1].next;
                    man->free[MEMMAN_FREES+1].next = ptr;
                }else{
                    //û�кϲ���ptr��㣬�ͺϲ���ptr->pre������
                    ptr->pre->size += size;
                }
                return 0;
            } else {
                //�����������ǰ�ϲ����Ǿ�ֻ�ܲ����½����
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
                    man->frees++;//ͳ����Ϣ����
                    insert_addr(&man->root_addr,ptr->pre);//RBT����
                    return 0;
                }
            }
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
            man->frees++;//ͳ����Ϣ����
            insert_addr(&man->root_addr,ptr->pre);//RBT����
            return 0;
        }
    }
    //ʧ�����ڴ浱Ȼ����
    man->losts++;
    man->lostsize += size;
    return -1;
}

/* 
//�ڴ��ͷţ��ҵ�˫������ʵ�ְ汾 
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size){
	//��һ����ͨ��ѭ���ҵ���ȷ�Ĺ黹λ�ã�����һ��ʼ�ͱ��밴��addr˳��������������û�к����ֻ����ѭ������ȷ��λ�� 
	struct FREEINFO* ptr =  man->free[MEMMAN_FREES].next;
	if(ptr == 0){
		//����������⴦��
		//1.��һ���ͷ��ڴ棬����������һ����㶼��
		//�ѿ�������ͷ����nextָ��ָ������Ҫʹ�õ��½ڵ�
		man->free[MEMMAN_FREES].next = man->free[MEMMAN_FREES+1].next;
		//���½ڵ�ӿ��������з������
		man->free[MEMMAN_FREES+1].next = man->free[MEMMAN_FREES+1].next->next;
		ptr = man->free[MEMMAN_FREES].next;
		//д�����ݣ����ؽ�� 
		ptr->addr = addr;
		ptr->size = size;
		ptr->next = 0;
		ptr->pre = 0;//��ô���ǵ�ʱ��Ҫ��ͷ������뷶Χȥ�ϲ���ֻ�������ͬ 
		//����βָ�� 
		man->free[MEMMAN_FREES+1].pre = ptr;
		man->frees++;//ͳ����Ϣ���� 
		return 0;
	}
	//������Ӧ�ĵ�ַλ�ã�RBT����Ӧ�������
	while(ptr){
		//��Ϊ�ǰ�˳����������һ��ptr->addr����addr��˵���������pre������������ڴ�Ĺ黹λ�� 
		if (ptr->addr > addr) {
			break;
		}
		ptr = ptr->next;
	}
	//�ö��ֲ��ҽ��иĽ�����Ҫ��ʼʱ��������Ӧ��ָ�����飬������û�����ֱ����RBT�Ľ� 
	if(ptr == 0){
		//����������⴦��
		//2.�ͷŵĽ����ڽ������ĩβ�ĵ�ַ��Ҫ����βָ�� 
		//��ptrָ������Ҫʹ�õ��½ڵ�
		ptr = man->free[MEMMAN_FREES+1].next;
		//���½ڵ�ӿ��������з������
		man->free[MEMMAN_FREES+1].next = man->free[MEMMAN_FREES+1].next->next;
		//д�����ݣ����ؽ�� 
		ptr->addr = addr;
		ptr->size = size;
		//���ӵ�β������ 
		ptr->next = 0;
		ptr->pre = man->free[MEMMAN_FREES+1].pre;//���ӵ�֮ǰ��β��� 
		ptr->pre->next = ptr; //֮ǰ��β���ָ������ 
		//����βָ�� 
		man->free[MEMMAN_FREES+1].pre = ptr;
		man->frees++;//ͳ����Ϣ���� 
		return 0;
	}
	//�ҵ���ȷ�Ĺ黹λ��֮��
	//�黹֮ǰ�ܷ����ϲ����ϲ�����ǰ��㣩�أ� 
	if(ptr->next){
		//����������ϲ����Ⱥϲ�����һ����� 
		if(addr + size == ptr->addr){
			ptr->addr = addr;
			ptr->size += size; 
		}
	}
	if(ptr->pre){
		//����Ѿ��ϲ�����ǰ����ˣ��ܷ���ǰ�ϲ���Ȼ��ѵ�ǰ�������������
		if(ptr->pre->addr + ptr->pre->size == ptr->addr){
			ptr->pre->size += ptr->size;
			//ͳ����Ϣ���� 
			man->frees--;
			//���ȶϿ�������ǰ�����ϵ 
			ptr->pre->next = ptr->next;
			ptr->next->pre = ptr->pre;
			//Ȼ��ǰ����������������
			ptr->next = man->free[MEMMAN_FREES+1].next; 
			man->free[MEMMAN_FREES+1].next = ptr;
			//����ֵ 
			return 0;
		}else{
			//������ǰ�ϲ����Ϳ����ǲ����Ѿ��黹���������
			if(addr == ptr->addr){
				//�黹��ϣ�����
				return 0; 
			}
			//��������Ǿ����ֲ�����ǰ�黹�ֲ������黹��
			//ֱ�Ӵӿ�������������Ҫһ��������
			if(man->free[MEMMAN_FREES+1].next){
				//���н�����Ǿ��ð�
				//��pre��nextָ��ָ������Ҫʹ�õ��½ڵ�
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
				man->frees++;//ͳ����Ϣ���� 
				return 0;
			}
		}
	}else{
		//������һ��Ҳֻ��һ�����
		//�ҵ��Ŀ��Թ黹�����ڴ���ڵ�һ�����
		//ֱ�Ӵӿ�������������Ҫһ��������
		if(man->free[MEMMAN_FREES+1].next){
			//���н�����Ǿ��ð�
			//��pre��nextָ��ָ������Ҫʹ�õ��½ڵ� 
			//�����preָ���½�� 
			ptr->pre = man->free[MEMMAN_FREES+1].next;
			//���½ڵ�ӿ��������з������ 
			man->free[MEMMAN_FREES+1].next = man->free[MEMMAN_FREES+1].next->next;
			//�½���nextָ�򱾽�㣬ͷ���ָ���½��
			man->free[MEMMAN_FREES].next = ptr->pre;
			ptr->pre->next = ptr;
			//д�����ݣ����ؽ��
			ptr->pre->addr = addr;
			ptr->pre->size = size;
			man->frees++;//ͳ����Ϣ���� 
			return 0;
		}
	}
	//ʧ�����ڴ浱Ȼ���� 
	man->losts++;
	man->lostsize += size;
	return -1;
}
*/


//���ߵĻ�������ʵ�֣��ҵĻ���˫������ʵ�� 
/* 
//��ʼ���ڴ���� 
void memman_init(struct MEMMAN *man){
	man->frees = 0;			//������Ϣ��
	man->maxfrees = 0;		// ���ڹ۲����״����frees�����ֵ
	man->lostsize = 0;		// �ͷ�ʧ�ܵ��ڴ��С�ܺͣ�Ҳ���Ƕ�ʧ����Ƭ
	man->losts = 0;			// �ͷ�ʧ�ܵĴ���
	return;
}
//�����ڴ�ϼ��Ƕ��� 
unsigned int memman_total(struct MEMMAN *man){
	//��Ϊ�ǻ�������ģ����Լ��㵹�Ǽ򵥡���ֱ��ѭ�������������� 
	unsigned int i, t = 0;
	for (i = 0; i < man->frees; i++) {
		t += man->free[i].size;
	}
	return t;
}
//����һ����ʵ��ڴ�ռ� 
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size){
	//��һ�εĺ���˼·��ž��ǣ�Ѱ�Һ����ڴ�ռ䣨��һ���ҵ����õľ��ã�����״���Ӧ��
	//Ȼ����һ�δ����Ѿ�ʹ�õı�ǩ���߼�ȥ��һ���ֵ�ʣ���ڴ�ռ䣬�����ڴ��ַƫ�� 
	//�����ȥ֮������ڴ�ռ�����
	//�Ǿ�ֱ��ѭ������
	unsigned int i, a;
	for (i = 0; i < man->frees; i++) {
		if (man->free[i].size >= size) {
			// �ҵ����㹻����ڴ�
			a = man->free[i].addr;
			man->free[i].addr += size;
			man->free[i].size -= size;
			if (man->free[i].size == 0) {
				// ���free[i]�����0���ͼ�ȥһ��������Ϣ
				man->frees--;
				for (; i < man->frees; i++) {
					man->free[i] = man->free[i + 1]; // �ṹ�����
				}
			}
			return a;
		}
	}
	return 0; // �޿��ÿռ�
}
//�ڴ��ͷ����ߵİ汾  
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size){
	//��������ķ�����
	//free�ڴ�Ӧ������addr˳������
	//Ȼ��黹�����ʵ�λ��
	//�൱���������������ĳ��λ�õ�Ԫ�ظı�����
	//�������飬�ҵ�����������Ԫ�أ�Ȼ����������ԣ��Ƿ���ǰ���������Ժϲ� 
	int i, j;
	//Ϊ�˱��ںϲ��ڴ棬��free[]����addr��˳������
	// ���ԣ��Ⱦ���Ӧ�÷�������
	for (i = 0; i < man->frees; i++) {
		if (man->free[i].addr > addr) {
			break;
		}
	}
	// free[i - 1].addr < addr < free[i].addr
	if (i > 0) {
		// ǰ���п����ڴ�
		if (man->free[i - 1].addr + man->free[i - 1].size == addr) {
			// ��ǰ�ϲ�
			man->free[i - 1].size += size;
			if (i < man->frees) {
				// ����Ҳ��
				if (addr + size == man->free[i].addr) {
					// ���ϲ�
					man->free[i - 1].size += man->free[i].size;
					//man->free[i]ɾ��
					// free[i]���0֮��ϲ���ǰ��ȥ
					man->frees--;
					for (; i < man->frees; i++) {
						man->free[i] = man->free[i + 1]; //�ṹ�帳ֵ
					}
				}
			}
			return 0; // �ɹ����
		}
	}
	//������ǰ��Ŀ��ÿռ�ϲ���һ��
	if (i < man->frees) {
		// ���滹��
		if (addr + size == man->free[i].addr) {
			// ���Ժͺ���Ŀռ�ϲ�
			man->free[i].addr = addr;
			man->free[i].size += size;
			return 0; // �ɹ����
		}
	}
	// �Ȳ�����ǰ��ϲ����ֲ��������ϲ�
	if (man->frees < MEMMAN_FREES) {
		//free[i]֮��ģ�����ƶ����ڳ����ÿռ�
		for (j = man->frees; j > i; j--) {
			man->free[j] = man->free[j - 1];
		}
		man->frees++;
		if (man->maxfrees < man->frees) {
			man->maxfrees = man->frees; // �������ֵ
		}
		man->free[i].addr = addr;
		man->free[i].size = size;
		return 0; // �ɹ����
	}
	// ���������ƶ�
	man->losts++;
	man->lostsize += size;
	return -1; //ʧ��
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

