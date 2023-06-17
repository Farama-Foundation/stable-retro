/*

STV - VDP1

crude drawing code to support some of the basic vdp1 features
based on what hanagumi columns needs
-- to be expanded

the vdp1 draws to the FRAMEBUFFER which is mapped in memory

*/


#include "driver.h"

data32_t *stv_vdp1_vram;
data32_t *stv_vdp1_regs;
extern data32_t *stv_scu;
char shienryu_sprite_kludge;
/*
Registers:
00
02
04
06
08 EWLR
0a EWRR
0c
0e
10 EDSR
12
*/
/*Erase/Write Upper-Left register*/
/*
15|14|13|12|11|10|09|08|07|06|05|04|03|02|01|00|
 0|X1 register      |Y1 register               |
*/
#define STV_VDP1_EWLR ((stv_vdp1_regs[0x008/4] >> 16)&0x0000ffff)
#define STV_VDP1_EWLR_X1 ((STV_VDP1_EWLR & 0x7e00) >> 9)
#define STV_VDP1_EWLR_Y1 ((STV_VDP1_EWLR & 0x01ff) >> 0)
/*Erase/Write Lower-Right register*/
/*
15|14|13|12|11|10|09|08|07|06|05|04|03|02|01|00|
X1 register         |Y1 register               |
*/
#define STV_VDP1_EWRR ((stv_vdp1_regs[0x008/4] >> 16)&0x0000ffff)
#define STV_VDP1_EWRR_X3 ((STV_VDP1_EWLR & 0xfe00) >> 9)
#define STV_VDP1_EWRR_Y3 ((STV_VDP1_EWLR & 0x01ff) >> 0)
/*Transfer End Status Register*/
/*
15|14|13|12|11|10|09|08|07|06|05|04|03|02| 01| 00|
0                                        |CEF|BEF|
*/
#define STV_VDP1_EDSR ((stv_vdp1_regs[0x010/4] >> 16)&0x0000ffff)
#define STV_VDP1_CEF  (STV_VDP1_EDSR & 2)
#define STV_VDP1_BEF  (STV_VDP1_EDSR & 1)
#define SET_CEF_FROM_1_TO_0 	if(STV_VDP1_CEF)	 stv_vdp1_regs[0x010/4]^=0x00020000
#define SET_CEF_FROM_0_TO_1     if(!(STV_VDP1_CEF))	 stv_vdp1_regs[0x010/4]^=0x00020000
/**/

#include "machine/random.h"

READ32_HANDLER( stv_vdp1_regs_r )
{
/*	static int x;*/

/*	x ^= 0x00020000;*/

	logerror ("cpu #%d (PC=%08X) VDP1: Read from Registers, Offset %04x\n",cpu_getactivecpu(), activecpu_get_pc(), offset);
/*	if (offset == 0x04) return x;*/

	return stv_vdp1_regs[offset];
}


int stv_vdp1_start ( void )
{
	stv_vdp1_regs = auto_malloc ( 0x040000 );
	stv_vdp1_vram = auto_malloc ( 0x100000 );

	memset(stv_vdp1_regs, 0, 0x040000);
	memset(stv_vdp1_vram, 0, 0x100000);

	/* our colour calculation is broken .. must fix it */
	shienryu_sprite_kludge = 0;
	if (!strcmp(Machine->gamedrv->name,"shienryu"))	shienryu_sprite_kludge = 1;

	return 0;
}

WRITE32_HANDLER( stv_vdp1_regs_w )
{
	COMBINE_DATA(&stv_vdp1_regs[offset]);
}

READ32_HANDLER ( stv_vdp1_vram_r )
{
	return stv_vdp1_vram[offset];
}


WRITE32_HANDLER ( stv_vdp1_vram_w )
{
	data8_t *vdp1 = memory_region(REGION_GFX2);

	COMBINE_DATA (&stv_vdp1_vram[offset]);

/*	if (((offset * 4) > 0xdf) && ((offset * 4) < 0x140))*/
/*	{*/
/*		log_cb(RETRO_LOG_DEBUG, LOGPRE "cpu #%d (PC=%08X): VRAM dword write to %08X = %08X & %08X\n", cpu_getactivecpu(), activecpu_get_pc(), offset*4, data, mem_mask ^ 0xffffffff);*/
/*	}*/

	data = stv_vdp1_vram[offset];
	/* put in gfx region for easy decoding */
	vdp1[offset*4+0] = (data & 0xff000000) >> 24;
	vdp1[offset*4+1] = (data & 0x00ff0000) >> 16;
	vdp1[offset*4+2] = (data & 0x0000ff00) >> 8;
	vdp1[offset*4+3] = (data & 0x000000ff) >> 0;
}

/*

there is a command every 0x20 bytes
the first word is the control word
the rest are data used by it

---
00 CMDCTRL
   e--- ---- ---- ---- | end bit (15)
   -jjj ---- ---- ---- | jump select bits (12-14)
   ---- zzzz ---- ---- | zoom point / hotspot (8-11)
   ---- ---- 00-- ---- | UNUSED
   ---- ---- --dd ---- | character read direction (4,5)
   ---- ---- ---- cccc | command bits (0-3)

02 CMDLINK
   llll llll llll ll-- | link
   ---- ---- ---- --00 | UNUSED

04 CMDPMOD
   m--- ---- ---- ---- | MON (looks at MSB and apply shadows etc.)
   -00- ---- ---- ---- | UNUSED
   ---h ---- ---- ---- | HSS (High Speed Shrink)
   ---- p--- ---- ---- | PCLIP (Pre Clipping Disable)
   ---- -c-- ---- ---- | CLIP (Clipping Mode Bit)
   ---- --m- ---- ---- | CMOD (User Clipping Enable Bit)
   ---- ---M ---- ---- | MESH (Mesh Enable Bit)
   ---- ---- e--- ---- | ECD (End Code Disable)
   ---- ---- -S-- ---- | SPD (Transparent Pixel Disable)
   ---- ---- --cc c--- | Colour Mode
   ---- ---- ---- -CCC | Colour Calculation bits

06 CMDCOLR
   mmmm mmmm mmmm mmmm | Colour Bank, Colour Lookup /8

08 CMDSRCA (Character Address)
   aaaa aaaa aaaa aa-- | Character Address
   ---- ---- ---- --00 | UNUSED

0a CMDSIZE (Character Size)
   00-- ---- ---- ---- | UNUSED
   --xx xxxx ---- ---- | Character Size (X)
   ---- ---- yyyy yyyy | Character Size (Y)

0c CMDXA (used for normal sprite)
   eeee ee-- ---- ---- | extension bits
   ---- --xx xxxx xxxx | x position

0e CMDYA (used for normal sprite)
   eeee ee-- ---- ---- | extension bits
   ---- --yy yyyy yyyy | y position

10 CMDXB
12 CMDYB
14 CMDXC
16 CMDYC
18 CMDXD
1a CMDYD
1c CMDGRDA (Gouraud Shading Table)
1e UNUSED
---


*/

int vdp1_sprite_log = 0;

static struct stv_vdp2_sprite_list
{

	int CMDCTRL, CMDLINK, CMDPMOD, CMDCOLR, CMDSRCA, CMDSIZE, CMDGRDA;
	int CMDXA, CMDYA;
	int CMDXB, CMDYB;
	int CMDXC, CMDYC;
	int CMDXD, CMDYD;

	int ispoly;

} stv2_current_sprite;

int stvvdp1_local_x;
int stvvdp1_local_y;

/* we should actually draw to the framebuffer then process that with vdp.. note that if we're drawing
to the framebuffer we CAN'T frameskip the vdp1 drawing as the hardware can READ the framebuffer
and if we skip the drawing the content could be incorrect when it reads it, although i have no idea
why they would want to */

extern data32_t* stv_vdp2_cram;

static INLINE void drawpixel(UINT16 *dest, int patterndata, int offsetcnt)
{
	int pix,mode,transmask;
	data8_t* gfxdata = memory_region(REGION_GFX2);
	int pix2;

	switch (stv2_current_sprite.CMDPMOD&0x0038)
	{
		case 0x0000: /* mode 0 16 colour bank mode (4bits) (hanagumi blocks)*/
			/* most of the shienryu sprites use this mode*/
			pix = gfxdata[patterndata+offsetcnt/2];
			pix = offsetcnt&1 ? (pix & 0x0f):((pix & 0xf0)>>4) ;
			pix = pix+((stv2_current_sprite.CMDCOLR&0x0ff0));
			mode = 0;
			transmask = 0xf;

			if (shienryu_sprite_kludge)
			{
				pix += 0x400;
				pix &= 0x7ff;
			}

			break;
		case 0x0008: /* mode 1 16 colour lookup table mode (4bits)*/
			/* shienryu explosisons (and some enemies) use this mode*/
			pix2 = gfxdata[patterndata+offsetcnt/2];
			pix2 = offsetcnt&1 ?  (pix2 & 0x0f):((pix2 & 0xf0)>>4);
			pix = pix2&1 ?
			((((stv_vdp1_vram[(((stv2_current_sprite.CMDCOLR&0xffff)*8)>>2)+((pix2&0xfffe)/2)])) & 0x0000ffff) >> 0):
			((((stv_vdp1_vram[(((stv2_current_sprite.CMDCOLR&0xffff)*8)>>2)+((pix2&0xfffe)/2)])) & 0xffff0000) >> 16);


			mode = 1;
			transmask = 0xf;

			if (pix2 & 0xf)
			{
				if (pix & 0x8000)
				{
					mode = 5;
					transmask = 0x7fff;

				}


			}
			else
			{
				pix=pix2; /* this is messy .. but just ensures that pen 0 isn't drawn*/
			}
			if (shienryu_sprite_kludge)
			{
				pix &= 0x1ff;
				pix += 0x400;
				pix &= 0x7ff;
			}


			break;
		case 0x0010: /* mode 2 64 colour bank mode (8bits) (character select portraits on hanagumi)*/
			pix = gfxdata[patterndata+offsetcnt];
			mode = 2;
			pix = pix+(stv2_current_sprite.CMDCOLR&0x0fc0);
			transmask = 0x3f;
			break;
		case 0x0018: /* mode 3 128 colour bank mode (8bits) (little characters on hanagumi use this mode)*/
			pix = gfxdata[patterndata+offsetcnt];
			pix = pix+(stv2_current_sprite.CMDCOLR&0x0f80);
			transmask = 0x7f;
			mode = 3;
		/*	pix = rand();*/
			break;
		case 0x0020: /* mode 4 256 colour bank mode (8bits) (hanagumi title)*/
			pix = gfxdata[patterndata+offsetcnt];
			pix = pix+(stv2_current_sprite.CMDCOLR&0x0f00);
			transmask = 0xff;
			mode = 4;
			break;
		case 0x0028: /* mode 5 32,768 colour RGB mode (16bits)*/
			pix = gfxdata[patterndata+offsetcnt*2+1] | (gfxdata[patterndata+offsetcnt*2]<<8) ;
			mode = 5;
			transmask = 0x7fff;
			break;
		default: /* other settings illegal*/
			pix = rand();
			mode = 0;
			transmask = 0xff;
	}

	if (stv2_current_sprite.ispoly)
	{
		pix = stv2_current_sprite.CMDCOLR&0xffff;
			mode = 1;
			transmask = 0xf;

			if (pix & 0x8000)
			{
				mode = 5;
				transmask = 0x7fff;
			}
	}


	if (mode != 5) /* mode 0-4 are 'normal'*/
	{
		if (pix & transmask)
		{
			/* there is probably a better way to do this .. it will probably have to change anyway because we'll be writing to the framebuffer instead */
			int col;
			col = (pix&1)? ((stv_vdp2_cram[(pix&0xfffe)/2] & 0x00007fff) >>0): ((stv_vdp2_cram[(pix&0xfffe)/2] & 0x7fff0000) >>16);
			col = ((col & 0x001f)*0x400) + (col & 0x03e0) + ((col & 0x7c00)/0x400);
			*dest = col;
		}
	}
	else /* mode 5 is rgb mode*/
	{
		int col;
		if (pix & 0x8000)
		{
			col = pix;
			col = ((col & 0x001f)*0x400) + (col & 0x03e0) + ((col & 0x7c00)/0x400);
			*dest = col & 0x7fff;
		}
	}
}

enum { FRAC_SHIFT = 16 };

struct spoint {
	INT32 x, y;
	INT32 u, v;
};

static void vdp1_fill_slope(struct mame_bitmap *bitmap, const struct rectangle *cliprect, int patterndata, int xsize,
							INT32 x1, INT32 x2, INT32 sl1, INT32 sl2, INT32 *nx1, INT32 *nx2,
							INT32 u1, INT32 u2, INT32 slu1, INT32 slu2, INT32 *nu1, INT32 *nu2,
							INT32 v1, INT32 v2, INT32 slv1, INT32 slv2, INT32 *nv1, INT32 *nv2,
							INT32 _y1, INT32 y2)
{
	if(_y1 > cliprect->max_y)
		return;

	if(y2 <= cliprect->min_y) {
		int delta = y2-_y1;
		*nx1 = x1+delta*sl1;
		*nu1 = u1+delta*slu1;
		*nv1 = v1+delta*slv1;
		*nx2 = x2+delta*sl2;
		*nu2 = u2+delta*slu2;
		*nv2 = v2+delta*slv2;
		return;
	}

	if(y2 > cliprect->max_y)
		y2 = cliprect->max_y+1;

	if(_y1 < cliprect->min_y) {
		int delta = cliprect->min_y - _y1;
		x1 += delta*sl1;
		u1 += delta*slu1;
		v1 += delta*slv1;
		x2 += delta*sl2;
		u2 += delta*slu2;
		v2 += delta*slv2;
		_y1 = cliprect->min_y;
	}

	if(x1 > x2 || (x1==x2 && sl1 > sl2)) {
		INT32 t, *tp;
		t = x1;
		x1 = x2;
		x2 = t;
		t = sl1;
		sl1 = sl2;
		sl2 = t;
		tp = nx1;
		nx1 = nx2;
		nx2 = tp;

		t = u1;
		u1 = u2;
		u2 = t;
		t = slu1;
		slu1 = slu2;
		slu2 = t;
		tp = nu1;
		nu1 = nu2;
		nu2 = tp;

		t = v1;
		v1 = v2;
		v2 = t;
		t = slv1;
		slv1 = slv2;
		slv2 = t;
		tp = nv1;
		nv1 = nv2;
		nv2 = tp;
	}

	while(_y1 < y2) {
		if(_y1 >= cliprect->min_y) {
			INT32 slux = 0, slvx = 0;
			int xx1 = x1>>FRAC_SHIFT;
			int xx2 = x2>>FRAC_SHIFT;
			INT32 u = u1;
			INT32 v = v1;
			if(xx1 != xx2) {
				int delta = xx2-xx1;
				slux = (u2-u1)/delta;
				slvx = (v2-v1)/delta;
			}
			if(xx1 <= cliprect->max_x || xx2 >= cliprect->min_x) {
				if(xx1 < cliprect->min_x) {
					int delta = cliprect->min_x-xx1;
					u += slux*delta;
					v += slvx*delta;
					xx1 = cliprect->min_x;
				}
				if(xx2 > cliprect->max_x)
					xx2 = cliprect->max_x;

				while(xx1 <= xx2) {
					drawpixel(((UINT16 *)(bitmap->line[_y1]))+xx1,
							  patterndata,
							  (v>>FRAC_SHIFT)*xsize+(u>>FRAC_SHIFT));
					xx1++;
					u += slux;
					v += slvx;
				}
			}
		}

		x1 += sl1;
		u1 += slu1;
		v1 += slv1;
		x2 += sl2;
		u2 += slu2;
		v2 += slv2;
		_y1++;
	}
	*nx1 = x1;
	*nu1 = u1;
	*nv1 = v1;
	*nx2 = x2;
	*nu2 = u2;
	*nv2 = v2;
}

static void vdp1_fill_line(struct mame_bitmap *bitmap, const struct rectangle *cliprect, int patterndata, int xsize, INT32 y,
						   INT32 x1, INT32 x2, INT32 u1, INT32 u2, INT32 v1, INT32 v2)
{
	int xx1 = x1>>FRAC_SHIFT;
	int xx2 = x2>>FRAC_SHIFT;

	if(y > cliprect->max_y || y < cliprect->min_y)
		return;

	if(xx1 <= cliprect->max_x || xx2 >= cliprect->min_x) {
		INT32 slux = 0, slvx = 0;
		INT32 u = u1;
		INT32 v = v1;
		if(xx1 != xx2) {
			int delta = xx2-xx1;
			slux = (u2-u1)/delta;
			slvx = (v2-v1)/delta;
		}
		if(xx1 < cliprect->min_x) {
			int delta = cliprect->min_x-xx1;
			u += slux*delta;
			v += slvx*delta;
			xx1 = cliprect->min_x;
		}
		if(xx2 > cliprect->max_x)
			xx2 = cliprect->max_x;

		while(xx1 <= xx2) {
			drawpixel(((UINT16 *)(bitmap->line[y]))+xx1,
					  patterndata,
					  (v>>FRAC_SHIFT)*xsize+(u>>FRAC_SHIFT));
			xx1++;
			u += slux;
			v += slvx;
		}
	}
}

static void vdp1_fill_quad(struct mame_bitmap *bitmap, const struct rectangle *cliprect, int patterndata, int xsize, const struct spoint *q)
{
	INT32 sl1, sl2, slu1, slu2, slv1, slv2, cury, limy, x1, x2, u1, u2, v1, v2, delta;
	int pmin, pmax, i, ps1, ps2;
	struct spoint p[8];

	for(i=0; i<4; i++) {
		p[i].x = p[i+4].x = q[i].x << FRAC_SHIFT;
		p[i].y = p[i+4].y = q[i].y;
		p[i].u = p[i+4].u = q[i].u << FRAC_SHIFT;
		p[i].v = p[i+4].v = q[i].v << FRAC_SHIFT;
	}

	pmin = pmax = 0;
	for(i=1; i<4; i++) {
		if(p[i].y < p[pmin].y)
			pmin = i;
		if(p[i].y > p[pmax].y)
			pmax = i;
	}

	cury = p[pmin].y;
	limy = p[pmax].y;

	if(cury == limy) {
		x1 = x2 = p[0].x;
		u1 = u2 = p[0].u;
		v1 = v2 = p[0].v;
		for(i=1; i<4; i++) {
			if(p[i].x < x1) {
				x1 = p[i].x;
				u1 = p[i].u;
				v1 = p[i].v;
			}
			if(p[i].x > x2) {
				x2 = p[i].x;
				u2 = p[i].u;
				v2 = p[i].v;
			}
		}
		vdp1_fill_line(bitmap, cliprect, patterndata, xsize, cury, x1, x2, u1, u2, v1, v2);
		return;
	}

	if(cury > cliprect->max_y)
		return;
	if(limy <= cliprect->min_y)
		return;

	if(limy > cliprect->max_y)
		limy = cliprect->max_y;

	ps1 = pmin+4;
	ps2 = pmin;

	goto startup;

	for(;;) {
		if(p[ps1-1].y == p[ps2+1].y) {
			vdp1_fill_slope(bitmap, cliprect, patterndata, xsize,
							x1, x2, sl1, sl2, &x1, &x2,
							u1, u2, slu1, slu2, &u1, &u2,
							v1, v2, slv1, slv2, &v1, &v2,
							cury, p[ps1-1].y);
			cury = p[ps1-1].y;
			if(cury >= limy)
				break;
			ps1--;
			ps2++;

		startup:
			while(p[ps1-1].y == cury)
				ps1--;
			while(p[ps2+1].y == cury)
				ps2++;
			x1 = p[ps1].x;
			u1 = p[ps1].u;
			v1 = p[ps1].v;
			x2 = p[ps2].x;
			u2 = p[ps2].u;
			v2 = p[ps2].v;

			delta = cury-p[ps1-1].y;
			sl1 = (x1-p[ps1-1].x)/delta;
			slu1 = (u1-p[ps1-1].u)/delta;
			slv1 = (v1-p[ps1-1].v)/delta;

			delta = cury-p[ps2+1].y;
			sl2 = (x2-p[ps2+1].x)/delta;
			slu2 = (u2-p[ps2+1].u)/delta;
			slv2 = (v2-p[ps2+1].v)/delta;
		} else if(p[ps1-1].y < p[ps2+1].y) {
			vdp1_fill_slope(bitmap, cliprect, patterndata, xsize,
							x1, x2, sl1, sl2, &x1, &x2,
							u1, u2, slu1, slu2, &u1, &u2,
							v1, v2, slv1, slv2, &v1, &v2,
							cury, p[ps1-1].y);
			cury = p[ps1-1].y;
			if(cury >= limy)
				break;
			ps1--;
			while(p[ps1-1].y == cury)
				ps1--;
			x1 = p[ps1].x;
			u1 = p[ps1].u;
			v1 = p[ps1].v;

			delta = cury-p[ps1-1].y;
			sl1 = (x1-p[ps1-1].x)/delta;
			slu1 = (u1-p[ps1-1].u)/delta;
			slv1 = (v1-p[ps1-1].v)/delta;
		} else {
			vdp1_fill_slope(bitmap, cliprect, patterndata, xsize,
							x1, x2, sl1, sl2, &x1, &x2,
							u1, u2, slu1, slu2, &u1, &u2,
							v1, v2, slv1, slv2, &v1, &v2,
							cury, p[ps2+1].y);
			cury = p[ps2+1].y;
			if(cury >= limy)
				break;
			ps2++;
			while(p[ps2+1].y == cury)
				ps2++;
			x2 = p[ps2].x;
			u2 = p[ps2].u;
			v2 = p[ps2].v;

			delta = cury-p[ps2+1].y;
			sl2 = (x2-p[ps2+1].x)/delta;
			slu2 = (u2-p[ps2+1].u)/delta;
			slv2 = (v2-p[ps2+1].v)/delta;
		}
	}
	if(cury == limy)
		vdp1_fill_line(bitmap, cliprect, patterndata, xsize, cury, x1, x2, u1, u2, v1, v2);
}

static int x2s(int v)
{
	int r = v & 0x7ff;
	if (r & 0x400)
		r -= 0x800;
	return r + stvvdp1_local_x;
}

static int y2s(int v)
{
	int r = v & 0x7ff;
	if (r & 0x400)
		r -= 0x800;
	return r + stvvdp1_local_y;
}

void stv_vpd1_draw_distorded_sprite(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	struct spoint q[4];

	int xsize, ysize;
	int direction;
	int patterndata;

	direction = (stv2_current_sprite.CMDCTRL & 0x0030)>>4;

	xsize = (stv2_current_sprite.CMDSIZE & 0x3f00) >> 8;
	xsize = xsize * 8;

	ysize = (stv2_current_sprite.CMDSIZE & 0x00ff);

	patterndata = (stv2_current_sprite.CMDSRCA) & 0xffff;
	patterndata = patterndata * 0x8;

	q[0].x = x2s(stv2_current_sprite.CMDXA);
	q[0].y = y2s(stv2_current_sprite.CMDYA);
	q[1].x = x2s(stv2_current_sprite.CMDXB);
	q[1].y = y2s(stv2_current_sprite.CMDYB);
	q[2].x = x2s(stv2_current_sprite.CMDXC);
	q[2].y = y2s(stv2_current_sprite.CMDYC);
	q[3].x = x2s(stv2_current_sprite.CMDXD);
	q[3].y = y2s(stv2_current_sprite.CMDYD);

	if(direction & 1) { /* xflip*/
		q[0].u = q[3].u = xsize-1;
		q[1].u = q[2].u = 0;
	} else {
		q[0].u = q[3].u = 0;
		q[1].u = q[2].u = xsize-1;
	}
	if(direction & 2) { /* yflip*/
		q[0].v = q[1].v = ysize-1;
		q[2].v = q[3].v = 0;
	} else {
		q[0].v = q[1].v = 0;
		q[2].v = q[3].v = ysize-1;
	}

	vdp1_fill_quad(bitmap, cliprect, patterndata, xsize, q);
}

void stv_vpd1_draw_scaled_sprite(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	struct spoint q[4];

	int xsize, ysize;
	int direction;
	int patterndata;
	int zoompoint;
	int x,y;
	int x2,y2;
	int screen_width,screen_height;

	direction = (stv2_current_sprite.CMDCTRL & 0x0030)>>4;

	xsize = (stv2_current_sprite.CMDSIZE & 0x3f00) >> 8;
	xsize = xsize * 8;

	ysize = (stv2_current_sprite.CMDSIZE & 0x00ff);

	patterndata = (stv2_current_sprite.CMDSRCA) & 0xffff;
	patterndata = patterndata * 0x8;

	zoompoint = (stv2_current_sprite.CMDCTRL & 0x0f00)>>8;

	x = stv2_current_sprite.CMDXA;
	y = stv2_current_sprite.CMDYA;

	screen_width = stv2_current_sprite.CMDXB;
	screen_height = stv2_current_sprite.CMDYB;

	x2 = stv2_current_sprite.CMDXC; /* second co-ordinate set x*/
	y2 = stv2_current_sprite.CMDYC; /* second co-ordinate set y*/

	switch (zoompoint)
	{
		case 0x0: /* specified co-ordinates*/
			break;
		case 0x5: /* up left*/
			break;
		case 0x6: /* up center*/
			x -= screen_width/2 ;
			break;
		case 0x7: /* up right*/
			x -= screen_width;
			break;

		case 0x9: /* center left*/
			y -= screen_height/2 ;
			break;
		case 0xa: /* center center*/
			y -= screen_height/2 ;
			x -= screen_width/2 ;

			break;

		case 0xb: /* center right*/
			y -= screen_height/2 ;
			x -= screen_width;
			break;

		case 0xd: /* center left*/
			y -= screen_height;
			break;

		case 0xe: /* center center*/
			y -= screen_height;
			x -= screen_width/2 ;
			break;

		case 0xf: /* center right*/
			y -= screen_height;
			x -= screen_width;
			break;

		default: /* illegal*/
			break;

	}

	/*  0----1
	    |    |
	    |    |
	    3----2   */

	if (zoompoint)
	{
		q[0].x = x2s(x);
		q[0].y = y2s(y);
		q[1].x = x2s(x)+screen_width;
		q[1].y = y2s(y);
		q[2].x = x2s(x)+screen_width;
		q[2].y = y2s(y)+screen_height;
		q[3].x = x2s(x);
		q[3].y = y2s(y)+screen_height;
	}
	else
	{
		q[0].x = x2s(x);
		q[0].y = y2s(y);
		q[1].x = x2s(x2);
		q[1].y = y2s(y);
		q[2].x = x2s(x2);
		q[2].y = y2s(y2);
		q[3].x = x2s(x);
		q[3].y = y2s(y2);
	}


	if(direction & 1) { /* xflip*/
		q[0].u = q[3].u = xsize-1;
		q[1].u = q[2].u = 0;
	} else {
		q[0].u = q[3].u = 0;
		q[1].u = q[2].u = xsize-1;
	}
	if(direction & 2) { /* yflip*/
		q[0].v = q[1].v = ysize-1;
		q[2].v = q[3].v = 0;
	} else {
		q[0].v = q[1].v = 0;
		q[2].v = q[3].v = ysize-1;
	}

	vdp1_fill_quad(bitmap, cliprect, patterndata, xsize, q);
}


void stv_vpd1_draw_normal_sprite(struct mame_bitmap *bitmap, const struct rectangle *cliprect, int sprite_type)
{
	UINT16 *destline;

	int y, ysize, ycnt, drawypos;
	int x, xsize, xcnt, drawxpos;
	int direction;
	int patterndata;

	x = x2s(stv2_current_sprite.CMDXA);
	y = y2s(stv2_current_sprite.CMDYA);

	direction = (stv2_current_sprite.CMDCTRL & 0x0030)>>4;

	xsize = (stv2_current_sprite.CMDSIZE & 0x3f00) >> 8;
	xsize = xsize * 8;

	ysize = (stv2_current_sprite.CMDSIZE & 0x00ff);


	patterndata = (stv2_current_sprite.CMDSRCA) & 0xffff;
	patterndata = patterndata * 0x8;




	if (vdp1_sprite_log) logerror ("Drawing Normal Sprite x %04x y %04x xsize %04x ysize %04x patterndata %06x\n",x,y,xsize,ysize,patterndata);

	for (ycnt = 0; ycnt != ysize; ycnt++) {


		if (direction & 0x2) /* 'yflip' (reverse direction)*/
		{
			drawypos = y+((ysize-1)-ycnt);
		}
		else
		{
			drawypos = y+ycnt;
		}

		if ((drawypos >= cliprect->min_y) && (drawypos <= cliprect->max_y))
		{
			destline = (UINT16 *)(bitmap->line[drawypos]);

			for (xcnt = 0; xcnt != xsize; xcnt ++)
			{
				if (direction & 0x1) /* 'xflip' (reverse direction)*/
				{
					drawxpos = x+((xsize-1)-xcnt);
				}
				else
				{
					drawxpos = x+xcnt;
				}
				if ((drawxpos >= cliprect->min_x) && (drawxpos <= cliprect->max_x))
				{
					int offsetcnt;

					offsetcnt = ycnt*xsize+xcnt;

					drawpixel(destline+drawxpos, patterndata, offsetcnt);
				} /* drawxpos*/

			} /* xcnt*/

		} /* if drawypos*/

	} /* ycny*/
}

void stv_vdp1_process_list(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	int position;
	int spritecount;
	int vdp1_nest;

	spritecount = 0;
	position = 0;

	if (vdp1_sprite_log) logerror ("Sprite List Process START\n");

	vdp1_nest = -1;

	/*Set CEF bit to 0*/
	SET_CEF_FROM_1_TO_0;

	while (spritecount<10000) /* if its drawn this many sprites something is probably wrong or sega were crazy ;-)*/
	{
		int draw_this_sprite;

		draw_this_sprite = 1;

	/*	if (position >= ((0x80000/0x20)/4)) // safety check*/
	/*	{*/
	/*		if (vdp1_sprite_log) logerror ("Sprite List Position Too High!\n");*/
	/*		position = 0;*/
	/*	}*/

		stv2_current_sprite.CMDCTRL = (stv_vdp1_vram[position * (0x20/4)+0] & 0xffff0000) >> 16;

		if (stv2_current_sprite.CMDCTRL == 0x8000)
		{
			if (vdp1_sprite_log) logerror ("List Terminator (0x8000) Encountered, Sprite List Process END\n");
			goto end; /* end of list*/
		}

		stv2_current_sprite.CMDLINK = (stv_vdp1_vram[position * (0x20/4)+0] & 0x0000ffff) >> 0;
		stv2_current_sprite.CMDPMOD = (stv_vdp1_vram[position * (0x20/4)+1] & 0xffff0000) >> 16;
		stv2_current_sprite.CMDCOLR = (stv_vdp1_vram[position * (0x20/4)+1] & 0x0000ffff) >> 0;
		stv2_current_sprite.CMDSRCA = (stv_vdp1_vram[position * (0x20/4)+2] & 0xffff0000) >> 16;
		stv2_current_sprite.CMDSIZE = (stv_vdp1_vram[position * (0x20/4)+2] & 0x0000ffff) >> 0;
		stv2_current_sprite.CMDXA   = (stv_vdp1_vram[position * (0x20/4)+3] & 0xffff0000) >> 16;
		stv2_current_sprite.CMDYA   = (stv_vdp1_vram[position * (0x20/4)+3] & 0x0000ffff) >> 0;
		stv2_current_sprite.CMDXB   = (stv_vdp1_vram[position * (0x20/4)+4] & 0xffff0000) >> 16;
		stv2_current_sprite.CMDYB   = (stv_vdp1_vram[position * (0x20/4)+4] & 0x0000ffff) >> 0;
		stv2_current_sprite.CMDXC   = (stv_vdp1_vram[position * (0x20/4)+5] & 0xffff0000) >> 16;
		stv2_current_sprite.CMDYC   = (stv_vdp1_vram[position * (0x20/4)+5] & 0x0000ffff) >> 0;
		stv2_current_sprite.CMDXD   = (stv_vdp1_vram[position * (0x20/4)+6] & 0xffff0000) >> 16;
		stv2_current_sprite.CMDYD   = (stv_vdp1_vram[position * (0x20/4)+6] & 0x0000ffff) >> 0;
		stv2_current_sprite.CMDGRDA = (stv_vdp1_vram[position * (0x20/4)+7] & 0xffff0000) >> 16;
/*		stv2_current_sprite.UNUSED  = (stv_vdp1_vram[position * (0x20/4)+7] & 0x0000ffff) >> 0;*/

		/* proecess jump / skip commands, set position for next sprite */
		switch (stv2_current_sprite.CMDCTRL & 0x7000)
		{
			case 0x0000: /* jump next*/
				if (vdp1_sprite_log) logerror ("Sprite List Process + Next (Normal)\n");
				position++;
				break;
			case 0x1000: /* jump assign*/
				if (vdp1_sprite_log) logerror ("Sprite List Process + Jump Old %06x New %06x\n", position, (stv2_current_sprite.CMDLINK>>2));
				position= (stv2_current_sprite.CMDLINK>>2);
				break;
			case 0x2000: /* jump call*/
				if (vdp1_nest == -1)
				{
					if (vdp1_sprite_log) logerror ("Sprite List Process + Call Old %06x New %06x\n",position, (stv2_current_sprite.CMDLINK>>2));
					vdp1_nest = position+1;
					position = (stv2_current_sprite.CMDLINK>>2);
				}
				else
				{
					if (vdp1_sprite_log) logerror ("Sprite List Nested Call, ignoring\n");
					position++;
				}
				break;
			case 0x3000:
				if (vdp1_nest != -1)
				{
					if (vdp1_sprite_log) logerror ("Sprite List Process + Return\n");
					position = vdp1_nest;
					vdp1_nest = -1;
				}
				else
				{
					if (vdp1_sprite_log) logerror ("Attempted return from no subroutine, aborting\n");
					position++;
					goto end; /* end of list*/
				}
				break;
			case 0x4000:
				draw_this_sprite = 0;
				position++;
				break;
			case 0x5000:
				if (vdp1_sprite_log) logerror ("Sprite List Skip + Jump Old %06x New %06x\n", position, (stv2_current_sprite.CMDLINK>>2));
				draw_this_sprite = 0;
				position= (stv2_current_sprite.CMDLINK>>2);

				break;
			case 0x6000:
				draw_this_sprite = 0;
				if (vdp1_nest == -1)
				{
					if (vdp1_sprite_log) logerror ("Sprite List Skip + Call To Subroutine Old %06x New %06x\n",position, (stv2_current_sprite.CMDLINK>>2));

					vdp1_nest = position+1;
					position = (stv2_current_sprite.CMDLINK>>2);
				}
				else
				{
					if (vdp1_sprite_log) logerror ("Sprite List Nested Call, ignoring\n");
					position++;
				}
				break;
			case 0x7000:
				draw_this_sprite = 0;
				if (vdp1_nest != -1)
				{
					if (vdp1_sprite_log) logerror ("Sprite List Skip + Return from Subroutine\n");

					position = vdp1_nest;
					vdp1_nest = -1;
				}
				else
				{
					if (vdp1_sprite_log) logerror ("Attempted return from no subroutine, aborting\n");
					position++;
					goto end; /* end of list*/
				}
				break;
		}

		/* continue to draw this sprite only if the command wasn't to skip it */
		if (draw_this_sprite ==1)
		{
			switch (stv2_current_sprite.CMDCTRL & 0x000f)
			{
				case 0x0000:
					if (vdp1_sprite_log) logerror ("Sprite List Normal Sprite\n");
					stv2_current_sprite.ispoly = 0;
					stv_vpd1_draw_normal_sprite(bitmap,cliprect, 0);
					break;

				case 0x0001:
					if (vdp1_sprite_log) logerror ("Sprite List Scaled Sprite\n");
					stv2_current_sprite.ispoly = 0;
					stv_vpd1_draw_scaled_sprite(bitmap,cliprect);
					break;

				case 0x0002:
					if (vdp1_sprite_log) logerror ("Sprite List Distorted Sprite\n");
					stv2_current_sprite.ispoly = 0;
					stv_vpd1_draw_distorded_sprite(bitmap,cliprect);
					break;

				case 0x0004:
					if (vdp1_sprite_log) logerror ("Sprite List Polygon\n");
					stv2_current_sprite.ispoly = 1;
					stv_vpd1_draw_distorded_sprite(bitmap,cliprect);
					break;

				case 0x0005:
					if (vdp1_sprite_log) logerror ("Sprite List Polyline\n");
					break;

				case 0x0006:
					if (vdp1_sprite_log) logerror ("Sprite List Line\n");
					break;

				case 0x0008:
					if (vdp1_sprite_log) logerror ("Sprite List Set Command for User Clipping\n");
					break;

				case 0x0009:
					if (vdp1_sprite_log) logerror ("Sprite List Set Command for System Clipping\n");
					break;

				case 0x000a:
					if (vdp1_sprite_log) logerror ("Sprite List Local Co-Ordinate Set\n");
					stvvdp1_local_x = stv2_current_sprite.CMDXA;
					stvvdp1_local_y = stv2_current_sprite.CMDYA;
					break;

				default:
					if (vdp1_sprite_log) logerror ("Sprite List Illegal!\n");
					break;


			}


		}

		spritecount++;

	}


	end:
	/*set CEF to 1*/
	SET_CEF_FROM_0_TO_1;

	/* not here! this is done every frame drawn even if the cpu isn't running eg in the debugger */
/*	if(!(stv_scu[40] & 0x2000)) //Sprite draw end irq*/
/*		cpu_set_irq_line_and_vector(0, 2, HOLD_LINE , 0x4d);*/

	if (vdp1_sprite_log) logerror ("End of list processing!\n");
}

void video_update_vdp1(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
/*	int enable;*/
/*	if (keyboard_pressed (KEYCODE_R)) vdp1_sprite_log = 1;*/
/*	if (keyboard_pressed (KEYCODE_T)) vdp1_sprite_log = 0;*/

/*	if (keyboard_pressed (KEYCODE_Y)) vdp1_sprite_log = 0;*/
/*	{*/
/*		FILE *fp;*/
/**/
/*		fp=fopen("vdp1_ram.dmp", "w+b");*/
/*		if (fp)*/
/*		{*/
/*			fwrite(stv_vdp1, 0x00100000, 1, fp);*/
/*			fclose(fp);*/
/*		}*/
/*	}*/
	stv_vdp1_process_list(bitmap,cliprect);
}
