/* �����v�S */

#include "bootpack.h"

#define EFLAGS_AC_BIT		0x00040000
#define CR0_CACHE_DISABLE	0x60000000

unsigned int memtest(unsigned int start, unsigned int end)
{
	char flg486 = 0;
	unsigned int eflg, cr0, i;

	/* 386����486�Խ��ʤΤ��δ_�J */
	eflg = io_load_eflags();
	eflg |= EFLAGS_AC_BIT; /* AC-bit = 1 */
	io_store_eflags(eflg);
	eflg = io_load_eflags();
	if ((eflg & EFLAGS_AC_BIT) != 0) { /* 386�Ǥ�AC=1�ˤ��Ƥ��ԄӤ�0�ˑ��äƤ��ޤ� */
		flg486 = 1;
	}
	eflg &= ~EFLAGS_AC_BIT; /* AC-bit = 0 */
	io_store_eflags(eflg);

	if (flg486 != 0) {
		cr0 = load_cr0();
		cr0 |= CR0_CACHE_DISABLE; /* ����å����ֹ */
		store_cr0(cr0);
	}

	i = memtest_sub(start, end);

	if (flg486 != 0) {
		cr0 = load_cr0();
		cr0 &= ~CR0_CACHE_DISABLE; /* ����å����S�� */
		store_cr0(cr0);
	}

	return i;
}

void memman_init(struct MEMMAN *man)
{
	man->frees = 0;			/* �������΂��� */
	man->maxfrees = 0;		/* ״�r�Q���ã�frees����� */
	man->lostsize = 0;		/* ��Ť�ʧ��������Ӌ������ */
	man->losts = 0;			/* ��Ť�ʧ���������� */
	return;
}

unsigned int memman_total(struct MEMMAN *man)
/* �����������κ�Ӌ���� */
{
	unsigned int i, t = 0;
	for (i = 0; i < man->frees; i++) {
		t += man->free[i].size;
	}
	return t;
}

unsigned int memman_alloc(struct MEMMAN *man, unsigned int size)
/* �_�� */
{
	unsigned int i, a;
	for (i = 0; i < man->frees; i++) {
		if (man->free[i].size >= size) {
			/* ʮ�֤ʎڤ��Τ�����kҊ */
			a = man->free[i].addr;
			man->free[i].addr += size;
			man->free[i].size -= size;
			if (man->free[i].size == 0) {
				/* free[i]���ʤ��ʤä��Τ�ǰ�ؤĤ�� */
				man->frees--;
				for (; i < man->frees; i++) {
					man->free[i] = man->free[i + 1]; /* ������δ��� */
				}
			}
			return a;
		}
	}
	return 0; /* �������ʤ� */
}

int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size)
/* ��� */
{
	int i, j;
	/* �ޤȤ�䤹���򿼤���ȡ�free[]��addr혤ˁK��Ǥ���ۤ������� */
	/* ������ޤ����ɤ�������٤�����Q��� */
	for (i = 0; i < man->frees; i++) {
		if (man->free[i].addr > addr) {
			break;
		}
	}
	/* free[i - 1].addr < addr < free[i].addr */
	if (i > 0) {
		/* ǰ������ */
		if (man->free[i - 1].addr + man->free[i - 1].size == addr) {
			/* ǰ�Τ����I��ˤޤȤ���� */
			man->free[i - 1].size += size;
			if (i < man->frees) {
				/* ���⤢�� */
				if (addr + size == man->free[i].addr) {
					/* �ʤ�����Ȥ�ޤȤ���� */
					man->free[i - 1].size += man->free[i].size;
					/* man->free[i]������ */
					/* free[i]���ʤ��ʤä��Τ�ǰ�ؤĤ�� */
					man->frees--;
					for (; i < man->frees; i++) {
						man->free[i] = man->free[i + 1]; /* ������δ��� */
					}
				}
			}
			return 0; /* �ɹ��K�� */
		}
	}
	/* ǰ�ȤϤޤȤ���ʤ��ä� */
	if (i < man->frees) {
		/* ������� */
		if (addr + size == man->free[i].addr) {
			/* ���ȤϤޤȤ���� */
			man->free[i].addr = addr;
			man->free[i].size += size;
			return 0; /* �ɹ��K�� */
		}
	}
	/* ǰ�ˤ����ˤ�ޤȤ���ʤ� */
	if (man->frees < MEMMAN_FREES) {
		/* free[i]����������ؤ��餷�ơ������ޤ����� */
		for (j = man->frees; j > i; j--) {
			man->free[j] = man->free[j - 1];
		}
		man->frees++;
		if (man->maxfrees < man->frees) {
			man->maxfrees = man->frees; /* ��󂎤���� */
		}
		man->free[i].addr = addr;
		man->free[i].size = size;
		return 0; /* �ɹ��K�� */
	}
	/* ���ˤ��餻�ʤ��ä� */
	man->losts++;
	man->lostsize += size;
	return -1; /* ʧ���K�� */
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
