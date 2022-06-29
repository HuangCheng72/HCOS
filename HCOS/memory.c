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
//��ʼ���ڴ���� 
void memman_init(struct MEMMAN *man){
	man->frees = 0;			/* ������Ϣ�� */
	man->maxfrees = 0;		/* ���ڹ۲����״����frees�����ֵ */
	man->lostsize = 0;		/* �ͷ�ʧ�ܵ��ڴ��С�ܺͣ�Ҳ���Ƕ�ʧ����Ƭ */
	man->losts = 0;			/* �ͷ�ʧ�ܵĴ��� */
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
			/* �ҵ����㹻����ڴ� */
			a = man->free[i].addr;
			man->free[i].addr += size;
			man->free[i].size -= size;
			if (man->free[i].size == 0) {
				/* ���free[i]�����0���ͼ�ȥһ��������Ϣ*/
				man->frees--;
				for (; i < man->frees; i++) {
					man->free[i] = man->free[i + 1]; /* �ṹ����� */
				}
			}
			return a;
		}
	}
	return 0; /* �޿��ÿռ� */
}
//�ڴ��ͷ� 
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size){
	//��������ķ�����
	//free�ڴ�Ӧ������addr˳������
	//Ȼ��黹�����ʵ�λ��
	//�൱���������������ĳ��λ�õ�Ԫ�ظı�����
	//�������飬�ҵ�����������Ԫ�أ�Ȼ����������ԣ��Ƿ���ǰ���������Ժϲ� 
	int i, j;
	/* Ϊ�˱��ںϲ��ڴ棬��free[]����addr��˳������ */
	/* ���ԣ��Ⱦ���Ӧ�÷������� */
	for (i = 0; i < man->frees; i++) {
		if (man->free[i].addr > addr) {
			break;
		}
	}
	/* free[i - 1].addr < addr < free[i].addr */
	if (i > 0) {
		/* ǰ���п����ڴ� */
		if (man->free[i - 1].addr + man->free[i - 1].size == addr) {
			/* ��ǰ�ϲ� */
			man->free[i - 1].size += size;
			if (i < man->frees) {
				/* ����Ҳ�� */
				if (addr + size == man->free[i].addr) {
					/* ���ϲ� */
					man->free[i - 1].size += man->free[i].size;
					/* man->free[i]ɾ�� */
					/* free[i]���0֮��ϲ���ǰ��ȥ */
					man->frees--;
					for (; i < man->frees; i++) {
						man->free[i] = man->free[i + 1]; /* �ṹ�帳ֵ */
					}
				}
			}
			return 0; /* �ɹ���� */
		}
	}
	/* ������ǰ��Ŀ��ÿռ�ϲ���һ�� */
	if (i < man->frees) {
		/* ���滹�� */
		if (addr + size == man->free[i].addr) {
			/* ���Ժͺ���Ŀռ�ϲ� */
			man->free[i].addr = addr;
			man->free[i].size += size;
			return 0; /* �ɹ���� */
		}
	}
	/* �Ȳ�����ǰ��ϲ����ֲ��������ϲ� */
	if (man->frees < MEMMAN_FREES) {
		/* free[i]֮��ģ�����ƶ����ڳ����ÿռ� */
		for (j = man->frees; j > i; j--) {
			man->free[j] = man->free[j - 1];
		}
		man->frees++;
		if (man->maxfrees < man->frees) {
			man->maxfrees = man->frees; /* �������ֵ */
		}
		man->free[i].addr = addr;
		man->free[i].size = size;
		return 0; /* �ɹ���� */
	}
	/* ���������ƶ� */
	man->losts++;
	man->lostsize += size;
	return -1; /* ʧ�� */
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

