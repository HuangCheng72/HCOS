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


//��ʼ���ڴ�����ҵ�˫������ʵ�� 
void memman_init(struct MEMMAN *man){
	man->frees = 0;			//������Ϣ��
	man->maxfrees = 0;		// ���ڹ۲����״����frees�����ֵ
	man->lostsize = 0;		// �ͷ�ʧ�ܵ��ڴ��С�ܺͣ�Ҳ���Ƕ�ʧ����Ƭ
	man->losts = 0;			// �ͷ�ʧ�ܵĴ���
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
//�ڴ��ͷţ��ҵ�˫������ʵ�ְ汾 
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size){
	//��һ����ͨ��ѭ���ҵ���ȷ�Ĺ黹λ�ã�����һ��ʼ�ͱ��밴��addr˳��������������û�к����ֻ����ѭ������ȷ��λ�� 
	struct FREEINFO* ptr =  man->free[MEMMAN_FREES].next;
	if(ptr == 0){
		//����������⴦��
		//1.��һ���ͷ��ڴ棬����������һ����㶼��
		//2.�ͷŵĽ����ڽ������ĩβ�ĵ�ַ  
		//�ѿ�������ͷ����nextָ��ָ������Ҫʹ�õ��½ڵ� 
		man->free[MEMMAN_FREES].next = man->free[MEMMAN_FREES+1].next;
		//���½ڵ�ӿ��������з������
		man->free[MEMMAN_FREES+1].next = man->free[MEMMAN_FREES+1].next->next;
		ptr = man->free[MEMMAN_FREES].next;
		//д�����ݣ����ؽ�� 
		ptr->addr = addr;
		ptr->size = size;
		ptr->next = 0;
		ptr->pre = 0;//��ô���ǵ�ʱ��Ҫ��ͷ������뷶Χȥ�ϲ�
		//����βָ�� 
		man->free[MEMMAN_FREES+1].pre = ptr;
		man->frees++;//ͳ����Ϣ���� 
		return 0;
	}
	while(ptr){
		//��Ϊ�ǰ�˳����������һ��ptr->addr����addr��˵���������pre������������ڴ�Ĺ黹λ�� 
		if (ptr->addr > addr) {
			break;
		}
		ptr = ptr->next;
	}
	if(ptr == 0){
		//����������⴦��
		//2.�ͷŵĽ����ڽ������ĩβ�ĵ�ַ��Ҫ����βָ�� 
		//�ѿ�ptrָ������Ҫʹ�õ��½ڵ�
		ptr = man->free[MEMMAN_FREES+1].next;
		//���½ڵ�ӿ��������з������
		man->free[MEMMAN_FREES+1].next = man->free[MEMMAN_FREES+1].next->next;
		//д�����ݣ����ؽ�� 
		ptr->addr = addr;
		ptr->size = size;
		//���ӵ�β������ 
		ptr->next = 0;
		ptr->pre = man->free[MEMMAN_FREES+1].pre;
		ptr->pre->next = ptr; 
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

