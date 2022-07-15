#include "bootpack.h"
void init_gdtidt(void){
	//Ϊʲô������������ַ����ʵ���������ѡ�ģ���Ϊ�������ڶ��ڴ�ֲ����Ǻ���Ϥ����Ҫ�Ҷ�������ԭ״ 
	//��ʵֻҪ��ûʹ�ù��ĵ�ַ���������� 
	//��������ַ�Ѿ���Ϊ��������ͷ�ļ��� 
	struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;
	struct GATE_DESCRIPTOR    *idt = (struct GATE_DESCRIPTOR    *) ADR_IDT;
	int i;
	// GDT�ĳ�ʼ����ֱ�ӰѶκ�Ϊ0��8191�Ķ���Ϣ��ͳͳ��0 
	for (i = 0; i <= LIMIT_GDT / 8; i++) {
		set_segmdesc(gdt + i, 0, 0, 0);
	}
	//�������öκ�Ϊ1��2��������
	//�κ�Ϊ1�ĶΣ�������ֵ0xffffffff��С������4GB��������32λ���Թ�����ڴ����ޣ������ַλ0����ʾ����CPU�ܹ����ȫ���ڴ� 
//	set_segmdesc(gdt + 1, 0xffffffff, 0x00000000, 0x4092);
	//�κ�Ϊ2�ĶΣ����СΪ512KB�����Ǹ�bootpack.hrb׼���� 
//	set_segmdesc(gdt + 2, 0x0007ffff, 0x00280000, 0x409a);
	//��GDT��Ϣд��gdtr�Ĵ�����Ҳ�����������õ�GDT��Ϣ��ŵĵ�ַ0x00270000 
//	load_gdtr(0xffff, 0x00270000);

	//����д�� 
	set_segmdesc(gdt + 1, 0xffffffff,   0x00000000, AR_DATA32_RW);
	set_segmdesc(gdt + 2, LIMIT_BOTPAK, ADR_BOTPAK, AR_CODE32_ER);
	load_gdtr(LIMIT_GDT, ADR_GDT);
	
	// IDT�ĳ�ʼ����ͬGDT 
	for (i = 0; i <= LIMIT_IDT / 8; i++) {
		set_gatedesc(idt + i, 0, 0, 0);
	}
	//��IDT��Ϣд��ר����IDT�ļĴ����� 
	load_idtr(LIMIT_IDT, ADR_IDT);
	
	// IDT���趨
	//�Ե�һ��Ϊ���ӣ���˼����asm_inthandler21����ע����IDT�ĵ�0x21��λ�ã����������Ӧ�жϣ����Զ�����ָ������
	//�����2*8��ʾasm_inthandler21������һ���Σ�������˼�Ƕκ�Ϊ2������8֮ǰҲ˵�ˣ�ÿ���ε���Ϣֻ�а˸��ֽ�
	//����Ҳ˵�˶κ�Ϊ2�Ķ��Ǹ�bootpack.hrb׼����
	//���� AR_INTGATE32��ֵ˵����IDT�������趨Ϊ0x008e�����Ǳ�ʾ�жϴ������Ч�趨�� 
	set_gatedesc(idt + 0x20, (int) asm_inthandler20, 2 * 8, AR_INTGATE32);
	set_gatedesc(idt + 0x21, (int) asm_inthandler21, 2 * 8, AR_INTGATE32);
	set_gatedesc(idt + 0x27, (int) asm_inthandler27, 2 * 8, AR_INTGATE32);
	set_gatedesc(idt + 0x2c, (int) asm_inthandler2c, 2 * 8, AR_INTGATE32);
 
	return;
}
//���������������˵���ǰѶε�ȫ����Ϣ����Ϊ�˸��ֽڣ�64λ��CPU��������Լ���׳ɵ���ʽ��д���ڴ���
//�����Ҫλ����Ļ�����������Ϊ�һ�����λ���㣬��ʱ�������� 
//����Ĳ����ֱ��ǣ�������Ϣ��ŵĵ�ַ���εĴ�С���εĻ�ַ���εĹ������� 
//�εĻ�ַռ��32λ����32λCPU�������������еĵ�ַ������ô���ģ�û�취��
//�εĴ�Сռ��20λ��ֻ�ܱ�ʾ���1MB����4GB�����ֵ���Ѿ���32λ�Ķ��������ˣ�����ֱ��ռ������������һ���ػصİ취���ڶε���������������һ����־λ
//�ڱ�־λGbitλΪ1��ʱ��limit�ĵ�λ�����ֽڣ�byte��������ҳ��page����һҳ����4KB�������漰���ڴ��ҳ�����⣩����˿������4GB���ڴ���� 
//�εĹ�������ռ��12λ���ⲿ��ֻ�򵥽��ܼ������õ�
//0x00��δʹ�õļ�¼��
//0x92��ϵͳר�ã��ɶ�д����ִ�С� 
//0x9a��ϵͳר�ã���ִ�пɶ�����д��
//0xf2��Ӧ�ó����ã��ɶ�д����ִ��
//0xfa��Ӧ�ó����ã���ִ�пɶ�����д�� 
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar){
	
	if (limit > 0x000fffff) {
		ar |= 0x8000;
		limit /= 0x1000;
	}
	sd->limit_low 	= limit & 0xffff;
	sd->base_low	= base & 0xffff;
	sd->base_mid	= (base >> 16) & 0xff;
	sd->access_right	= ar & 0xff;
	sd->limit_high   = ((limit >> 16) & 0x0f) | ((ar >> 8) & 0xf0);
	sd->base_high    = (base >> 24) & 0xff;
	return;
}
//����������ƣ�����׸�� 
void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar){
	gd->offset_low   = offset & 0xffff;
	gd->selector     = selector;
	gd->dw_count     = (ar >> 8) & 0xff;
	gd->access_right = ar & 0xff;
	gd->offset_high  = (offset >> 16) & 0xffff;
	return;
}
