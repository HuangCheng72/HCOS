/* 夕蚊嶷栽侃尖 */

#include "bootpack.h"

#define SHEET_USE		1

//賦萩喘噐芝吮夕蚊陣崙延楚議坪贋腎寂��腎寂寄弌頁潤更悶議寄弌
//隼朔譜崔光嶽奉來��宸祥頁兜兵晒痕方 
struct SHTCTL *shtctl_init(struct MEMMAN *memman, unsigned char *vram, int xsize, int ysize){
	struct SHTCTL *ctl;
	int i;
	ctl = (struct SHTCTL *) memman_alloc_4k(memman, sizeof (struct SHTCTL));
	if (ctl == 0) {
		goto err;
	}
	ctl->vram = vram;
	ctl->xsize = xsize;
	ctl->ysize = ysize;
	ctl->top = -1; /* 匯倖sheet脅短嗤 */
	for (i = 0; i < MAX_SHEETS; i++) {
		ctl->sheets0[i].flags = 0; /* 炎芝葎隆聞喘 */
	}
err:
	return ctl;
}
//壓sheets0方怏戦中儖孀隆聞喘議夕蚊��泌惚孀欺阻祥繍凪炎芝葎屎壓聞喘��隼朔卦指仇峽 
struct SHEET *sheet_alloc(struct SHTCTL *ctl){
	struct SHEET *sht;
	int i;
	for (i = 0; i < MAX_SHEETS; i++) {
		if (ctl->sheets0[i].flags == 0) {
			sht = &ctl->sheets0[i];
			sht->flags = SHEET_USE; /* 炎芝葎屎壓聞喘*/
			sht->height = -1; /* 咨茄 */
			return sht;
		}
	}
	return 0;	/* 侭嗤議SHEET脅侃噐屎壓聞喘彜蓑 */
}
//譜崔夕蚊議産喝曝寄弌才邑苧弼議痕方 
void sheet_setbuf(struct SHEET *sht, unsigned char *buf, int xsize, int ysize, int col_inv){
	sht->buf = buf;
	sht->bxsize = xsize;
	sht->bysize = ysize;
	sht->col_inv = col_inv;
	return;
}

void sheet_refreshsub(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1){
	int h, bx, by, vx, vy, bx0, by0, bx1, by1;
	unsigned char *buf, c, *vram = ctl->vram;
	struct SHEET *sht;
	for (h = 0; h <= ctl->top; h++) {
		sht = ctl->sheets[h];
		buf = sht->buf;
		/* 聞喘vx0-vy1��斤曳bx0-by1序佩宜容 */
		bx0 = vx0 - sht->vx0;
		by0 = vy0 - sht->vy0;
		bx1 = vx1 - sht->vx0;
		by1 = vy1 - sht->vy0;
		if (bx0 < 0) { bx0 = 0; }
		if (by0 < 0) { by0 = 0; }
		if (bx1 > sht->bxsize) { bx1 = sht->bxsize; }
		if (by1 > sht->bysize) { by1 = sht->bysize; }
		for (by = by0; by < by1; by++) {
			vy = sht->vy0 + by;
			for (bx = bx0; bx < bx1; bx++) {
				vx = sht->vx0 + bx;
				c = buf[by * sht->bxsize + bx];
				if (c != sht->col_inv) {
					vram[vy * ctl->xsize + vx] = c;
				}
			}
		}
	}
	return;
}

void sheet_updown(struct SHTCTL *ctl, struct SHEET *sht, int height){
	int h, old = sht->height; /* 贋刈譜崔念議互業佚連 */

	/* 泌惚峪吸忽議互業狛互賜狛詰祥勣俐屎*/
	if (height > ctl->top + 1) {
		height = ctl->top + 1;
	}
	if (height < -1) {
		height = -1;
	}
	sht->height = height; /* 譜協互業 */

	/* 參和麼勣頁序佩sheets方怏議嶷仟電双 */
	if (old > height) {	/* 曳參念詰 */
		if (height >= 0) {
			/* 委嶄寂議吏貧性 */
			for (h = old; h > height; h--) {
				ctl->sheets[h] = ctl->sheets[h - 1];
				ctl->sheets[h]->height = h;
			}
			ctl->sheets[height] = sht;
		} else {	/* 咨茄 */
			if (ctl->top > old) {
				/* 委貧中議週和栖 */
				for (h = old; h < ctl->top; h++) {
					ctl->sheets[h] = ctl->sheets[h + 1];
					ctl->sheets[h]->height = h;
				}
			}
			ctl->top--; /* 喇噐�塋蕉亠塚鴫禺�富阻匯倖��侭參恷貧中議夕蚊互業和週 */
		}
		sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize); //梓孚仟夕蚊議佚連嶷仟紙崙鮫中 
	} else if (old < height) {	/* 曳參念互 */
		if (old >= 0) {
			/* 委嶄寂議性和肇 */
			for (h = old; h < height; h++) {
				ctl->sheets[h] = ctl->sheets[h + 1];
				ctl->sheets[h]->height = h;
			}
			ctl->sheets[height] = sht;
		} else {	/* 喇咨茄彜蓑廬葎�塋衝缶� */
			/* 繍厮壓貧中議戻貧栖 */
			for (h = ctl->top; h >= height; h--) {
				ctl->sheets[h + 1] = ctl->sheets[h];
				ctl->sheets[h + 1]->height = h + 1;
			}
			ctl->sheets[height] = sht;
			ctl->top++; /* 喇噐厮�塋承塚鴫龝�紗阻匯倖��侭參恷貧中議夕蚊互業奐紗 */
		}
		sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize);//梓孚仟夕蚊議佚連嶷仟紙崙鮫中 
	}
	return;
}

void sheet_refresh(struct SHTCTL *ctl, struct SHEET *sht, int bx0, int by0, int bx1, int by1){
	if (sht->height >= 0) { /* もしも燕幣嶄なら、仟しい和じきの秤�鵑朴悗辰道�中を宙き岷す */
		sheet_refreshsub(ctl, sht->vx0 + bx0, sht->vy0 + by0, sht->vx0 + bx1, sht->vy0 + by1);
	}
	return;
}

void sheet_slide(struct SHTCTL *ctl, struct SHEET *sht, int vx0, int vy0)
{
	int old_vx0 = sht->vx0, old_vy0 = sht->vy0;
	sht->vx0 = vx0;
	sht->vy0 = vy0;
	if (sht->height >= 0) { /* もしも燕幣嶄なら、仟しい和じきの秤�鵑朴悗辰道�中を宙き岷す */
		sheet_refreshsub(ctl, old_vx0, old_vy0, old_vx0 + sht->bxsize, old_vy0 + sht->bysize);
		sheet_refreshsub(ctl, vx0, vy0, vx0 + sht->bxsize, vy0 + sht->bysize);
	}
	return;
}

void sheet_free(struct SHTCTL *ctl, struct SHEET *sht)
{
	if (sht->height >= 0) {
		sheet_updown(ctl, sht, -1); /* 燕幣泌惚侃噐�塋衝缶���夸枠譜協葎咨茄 */
	}
	sht->flags = 0; /* 隆聞喘議炎崗 */
	return;
}
 
