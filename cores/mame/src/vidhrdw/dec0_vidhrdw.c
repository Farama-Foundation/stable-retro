/***************************************************************************

  Dec0 Video emulation - Bryan McPhail, mish@tendril.co.uk

*********************************************************************

	Each game uses the MXC-06 chip to produce sprites.

	Sprite data:  The unknown bits seem to be unused.

	Byte 0:
		Bit 0 : Y co-ord hi bit
		Bit 1,2: ?
		Bit 3,4 : Sprite height (1x, 2x, 4x, 8x)
		Bit 5  - X flip
		Bit 6  - Y flip
		Bit 7  - Only display Sprite if set
	Byte 1: Y-coords
	Byte 2:
		Bit 0,1,2,3: Hi bits of sprite number
		Bit 4,5,6,7: (Probably unused MSB's of sprite)
	Byte 3: Low bits of sprite number
	Byte 4:
		Bit 0 : X co-ords hi bit
		Bit 1,2: ??
		Bit 3: Sprite flash (sprite is displayed every other frame)
		Bit 4,5,6,7:  - Colour
	Byte 5: X-coords

**********************************************************************

  Palette data

    0x000 - character palettes (Sprites on Midnight R)
    0x200 - sprite palettes (Characters on Midnight R)
    0x400 - tiles 1
  	0x600 - tiles 2

	Bad Dudes, Robocop, Heavy Barrel, Hippodrome - 24 bit rgb
	Sly Spy, Midnight Resistance - 12 bit rgb

  Tile data

  	4 bit palette select, 12 bit tile select

**********************************************************************

 All games contain three BAC06 background generator chips, usual (software)
configuration is 2 chips of 16*16 tiles, 1 of 8*8.

 Playfield control registers:
   bank 0:
   0:
		bit 0 (0x1) set = 8*8 tiles, else 16*16 tiles
		Bit 1 (0x2) unknown
		bit 2 (0x4) set enables rowscroll
		bit 3 (0x8) set enables colscroll
		bit 7 (0x80) set in playfield 1 is reverse screen (set via dip-switch)
		bit 7 (0x80) in other playfields unknown
   2: unknown (00 in bg, 03 in fg+text - maybe controls pf transparency?)
   4: unknown (always 00)
   6: playfield shape: 00 = 4x1, 01 = 2x2, 02 = 1x4 (low 4 bits only)

   bank 1:
   0: horizontal scroll
   2: vertical scroll
   4: Style of colscroll (low 4 bits, top 4 bits do nothing)
   6: Style of rowscroll (low 4 bits, top 4 bits do nothing)

Rowscroll/Colscroll styles:
	0: 256 scroll registers (Robocop)
	1: 128 scroll registers
	2:  64 scroll registers
	3:  32 scroll registers (Heavy Barrel, Midres)
	4:  16 scroll registers (Bad Dudes, Sly Spy)
	5:   8 scroll registers (Hippodrome)
	6:   4 scroll registers (Heavy Barrel)
	7:   2 scroll registers (Heavy Barrel, used on other games but registers kept at 0)
	8:   1 scroll register (ie, none)

	Values above are *multiplied* by playfield shape.

Playfield priority (Bad Dudes, etc):
	In the bottommost playfield, pens 8-15 can have priority over the next playfield.
	In that next playfield, pens 8-15 can have priority over sprites.

Bit 0:  Playfield inversion
Bit 1:  Enable playfield mixing (for palettes 8-15 only)
Bit 2:  Enable playfield/sprite mixing (for palettes 8-15 only)

Priority word (Midres):
	Bit 0 set = Playfield 3 drawn over Playfield 2
			~ = Playfield 2 drawn over Playfield 3
	Bit 1 set = Sprites are drawn inbetween playfields
			~ = Sprites are on top of playfields
	Bit 2
	Bit 3 set = ...

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

static struct tilemap *pf1_tilemap_0,*pf1_tilemap_1,*pf1_tilemap_2;
static struct tilemap *pf2_tilemap_0,*pf2_tilemap_1,*pf2_tilemap_2;
static struct tilemap *pf3_tilemap_0,*pf3_tilemap_1,*pf3_tilemap_2;

data16_t *dec0_pf1_data,*dec0_pf2_data,*dec0_pf3_data;
data16_t *dec0_pf1_rowscroll,*dec0_pf2_rowscroll,*dec0_pf3_rowscroll;
data16_t *dec0_pf1_colscroll,*dec0_pf2_colscroll,*dec0_pf3_colscroll;
static data16_t dec0_pf1_control_0[4];
static data16_t dec0_pf1_control_1[4];
static data16_t dec0_pf2_control_0[4];
static data16_t dec0_pf2_control_1[4];
static data16_t dec0_pf3_control_0[4];
static data16_t dec0_pf3_control_1[4];
static data16_t *dec0_spriteram;
static data16_t dec0_pri;

/******************************************************************************/

WRITE16_HANDLER( dec0_update_sprites_w )
{
	memcpy(dec0_spriteram,spriteram16,0x800);
}

/******************************************************************************/

static void update_24bitcol(int offset)
{
	int r,g,b;

	r = (paletteram16[offset] >> 0) & 0xff;
	g = (paletteram16[offset] >> 8) & 0xff;
	b = (paletteram16_2[offset] >> 0) & 0xff;

	palette_set_color(offset,r,g,b);
}

WRITE16_HANDLER( dec0_paletteram_rg_w )
{
	COMBINE_DATA(&paletteram16[offset]);
	update_24bitcol(offset);
}

WRITE16_HANDLER( dec0_paletteram_b_w )
{
	COMBINE_DATA(&paletteram16_2[offset]);
	update_24bitcol(offset);
}

/******************************************************************************/

static void dec0_drawsprites(struct mame_bitmap *bitmap,const struct rectangle *cliprect,int pri_mask,int pri_val)
{
	int offs;

	for (offs = 0;offs < 0x400;offs += 4)
	{
		int x,y,sprite,colour,multi,fx,fy,inc,flash,mult;

		y = dec0_spriteram[offs];
		if ((y&0x8000) == 0) continue;

		x = dec0_spriteram[offs+2];
		colour = x >> 12;
		if ((colour & pri_mask) != pri_val) continue;

		flash=x&0x800;
		if (flash && (cpu_getcurrentframe() & 1)) continue;

		fx = y & 0x2000;
		fy = y & 0x4000;
		multi = (1 << ((y & 0x1800) >> 11)) - 1;	/* 1x, 2x, 4x, 8x height */
											/* multi = 0   1   3   7 */

		sprite = dec0_spriteram[offs+1] & 0x0fff;

		x = x & 0x01ff;
		y = y & 0x01ff;
		if (x >= 256) x -= 512;
		if (y >= 256) y -= 512;
		x = 240 - x;
		y = 240 - y;

		if (x>256) continue; /* Speedup */

		sprite &= ~multi;
		if (fy)
			inc = -1;
		else
		{
			sprite += multi;
			inc = 1;
		}

		if (flip_screen) {
			y=240-y;
			x=240-x;
			if (fx) fx=0; else fx=1;
			if (fy) fy=0; else fy=1;
			mult=16;
		}
		else mult=-16;

		while (multi >= 0)
		{
			drawgfx(bitmap,Machine->gfx[3],
					sprite - multi * inc,
					colour,
					fx,fy,
					x,y + mult * multi,
					cliprect,TRANSPARENCY_PEN,0);

			multi--;
		}
	}
}

/******************************************************************************/

static void dec0_pf1_update(void)
{
	int offs,lines,height,scrolly,scrollx;
	struct tilemap *tilemap_ptr;

	/* Flipscreen */
	flip_screen_set(dec0_pf1_control_0[0]&0x80);

	/* Master scroll registers */
	scrollx = dec0_pf1_control_1[0];
	scrolly = dec0_pf1_control_1[1];

	/* Playfield shape */
	switch (dec0_pf1_control_0[3]&0x3) {
		case 0:	height=1; tilemap_ptr=pf1_tilemap_0; break; /* 4x1, 256 rows */
		case 1:	height=2; tilemap_ptr=pf1_tilemap_1; break; /* 2x2, 512 rows */
		case 2:	height=4; tilemap_ptr=pf1_tilemap_2; break; /* 1x4, 1024 rows */
		default: height=2; tilemap_ptr=pf1_tilemap_1; break; /* Never happens */
	}

	/* Column scroll - may be screen-wise rather than tilemap-wise, not confirmed yet */
	if (dec0_pf1_control_0[0]&0x8 && dec0_pf1_colscroll[0]) {
		tilemap_set_scroll_cols(tilemap_ptr,32);
		tilemap_set_scroll_rows(tilemap_ptr,1);
		tilemap_set_scrollx(tilemap_ptr,0,scrollx);

		for (offs = 0;offs < 32;offs++)
			tilemap_set_scrolly(tilemap_ptr, offs, scrolly + dec0_pf1_colscroll[offs]);
	}

	/* Row scroll enable bit */
	else if (dec0_pf1_control_0[0]&0x4) {
		/* Rowscroll style */
		switch (dec0_pf1_control_1[3]&0xf) {
			case 0: lines=256; break; /* 256 horizontal scroll registers (Robocop) */
			case 1: lines=128; break; /* 128 horizontal scroll registers (Not used?) */
			case 2: lines=64; break; /* 64 horizontal scroll registers (Not used?) */
			case 3: lines=32; break; /* 32 horizontal scroll registers (Heavy Barrel title screen) */
			case 4: lines=16; break; /* 16 horizontal scroll registers (Bad Dudes, Sly Spy) */
			case 5: lines=8; break; /* 8 horizontal scroll registers (Not used?) */
			case 6: lines=4; break; /* 4 horizontal scroll registers (Not used?) */
			case 7: lines=2; break; /* 2 horizontal scroll registers (Not used?) */
			default: lines=1;
		}

		tilemap_set_scroll_cols(tilemap_ptr,1);
		tilemap_set_scroll_rows(tilemap_ptr,lines*height);
		tilemap_set_scrolly(tilemap_ptr,0,scrolly);
		for (offs = 0; offs < lines*height; offs++)
			tilemap_set_scrollx(tilemap_ptr,offs,scrollx + dec0_pf1_rowscroll[offs]);
	}
	else { /* Scroll registers not enabled */
		tilemap_set_scroll_rows(tilemap_ptr,1);
		tilemap_set_scroll_cols(tilemap_ptr,1);
		tilemap_set_scrollx(tilemap_ptr,0,scrollx);
		tilemap_set_scrolly(tilemap_ptr,0,scrolly);
	}
}

static void dec0_pf2_update(void)
{
	int offs,lines,height,scrolly,scrollx;
	struct tilemap *tilemap_ptr;

	/* Master scroll registers */
	scrollx = dec0_pf2_control_1[0];
	scrolly = dec0_pf2_control_1[1];

	/* Playfield shape */
	switch (dec0_pf2_control_0[3]&0x3) {
		case 0:	height=1; tilemap_ptr=pf2_tilemap_0; break; /* 4x1, 256 rows */
		case 1:	height=2; tilemap_ptr=pf2_tilemap_1; break; /* 2x2, 512 rows */
		case 2:	height=4; tilemap_ptr=pf2_tilemap_2; break; /* 1x4, 1024 rows */
		default: height=2; tilemap_ptr=pf2_tilemap_1; break; /* Never happens */
	}

	/* Column scroll - may be screen-wise rather than tilemap-wise, not confirmed yet */
	if (dec0_pf2_control_0[0]&0x8 && (dec0_pf2_colscroll[0] || dec0_pf2_colscroll[1])) {
		switch (dec0_pf2_control_1[2]&0x7) {
			case 0: lines=1; break;
			case 1: lines=2; break;
			case 2: lines=4; break;
			case 3: lines=8; break;
			case 4: lines=16; break;
			case 5: lines=32; break;
			case 6: lines=64; break;
			case 7: lines=128; break;
			default: lines=1;
		}
		if (height==2) lines*=2; else if (height==1) lines*=4;

		tilemap_set_scroll_cols(tilemap_ptr,lines);
		tilemap_set_scroll_rows(tilemap_ptr,1);
		tilemap_set_scrollx(tilemap_ptr,0,scrollx);

		for (offs = 0;offs < lines;offs++)
			tilemap_set_scrolly(tilemap_ptr, offs, scrolly + dec0_pf2_colscroll[offs]);
	}

	/* Row scroll enable bit */
	else if (dec0_pf2_control_0[0]&0x4) {
		/* Rowscroll style */
		switch (dec0_pf2_control_1[3]&0xf) {
			case 0: lines=256; break; /* 256 horizontal scroll registers (Robocop) */
			case 1: lines=128; break; /* 128 horizontal scroll registers (Not used?) */
			case 2: lines=64; break; /* 64 horizontal scroll registers (Not used?) */
			case 3: lines=32; break; /* 32 horizontal scroll registers (Heavy Barrel title screen) */
			case 4: lines=16; break; /* 16 horizontal scroll registers (Bad Dudes, Sly Spy) */
			case 5: lines=8; break; /* 8 horizontal scroll registers (Not used?) */
			case 6: lines=4; break; /* 4 horizontal scroll registers (Not used?) */
			case 7: lines=2; break; /* 2 horizontal scroll registers (Not used?) */
			default: lines=1;
		}

		tilemap_set_scroll_cols(tilemap_ptr,1);
		tilemap_set_scroll_rows(tilemap_ptr,lines*height);
		tilemap_set_scrolly(tilemap_ptr,0,scrolly);
		for (offs = 0; offs < lines*height; offs++)
			tilemap_set_scrollx(tilemap_ptr,offs,scrollx + dec0_pf2_rowscroll[offs]);
	}
	else { /* Scroll registers not enabled */
		tilemap_set_scroll_rows(tilemap_ptr,1);
		tilemap_set_scroll_cols(tilemap_ptr,1);
		tilemap_set_scrollx(tilemap_ptr,0,scrollx);
		tilemap_set_scrolly(tilemap_ptr,0,scrolly);
	}
}

static void dec0_pf3_update(void)
{
	int offs,lines,height,scrolly,scrollx;
	struct tilemap *tilemap_ptr;

	/* Master scroll registers */
	scrollx = dec0_pf3_control_1[0];
	scrolly = dec0_pf3_control_1[1];

	/* Playfield shape */
	switch (dec0_pf3_control_0[3]&0x3) {
		case 0:	height=1; tilemap_ptr=pf3_tilemap_0; break; /* 4x1, 256 rows */
		case 1:	height=2; tilemap_ptr=pf3_tilemap_1; break; /* 2x2, 512 rows */
		case 2:	height=4; tilemap_ptr=pf3_tilemap_2; break; /* 1x4, 1024 rows */
		default: height=2; tilemap_ptr=pf3_tilemap_1; break; /* Never happens */
	}

	/* Column scroll - may be screen-wise rather than tilemap-wise, not confirmed yet */
	if (dec0_pf3_control_0[0]&0x8 && (dec0_pf3_colscroll[0] || dec0_pf3_colscroll[1])) {
		switch (dec0_pf3_control_1[2]&0x7) {
			case 0: lines=2; break;
			case 1: lines=2; break;
			case 2: lines=4; break;
			case 3: lines=8; break;
			case 4: lines=16; break;
			case 5: lines=32; break;
			case 6: lines=64; break;
			case 7: lines=128; break;
			default: lines=1;
		}
		if (height==2) lines*=2; else if (height==1) lines*=4;

		tilemap_set_scroll_cols(tilemap_ptr,lines);
		tilemap_set_scroll_rows(tilemap_ptr,1);
		tilemap_set_scrollx(tilemap_ptr,0,scrollx);

		for (offs = 0;offs < lines;offs++) {
			tilemap_set_scrolly(tilemap_ptr, offs, scrolly + dec0_pf3_colscroll[offs]);
		}
	}

	/* Row scroll enable bit */
	else if (dec0_pf3_control_0[0]&0x4) {
		/* Rowscroll style */
		switch (dec0_pf3_control_1[3]&0xf) {
			case 0: lines=256; break; /* 256 horizontal scroll registers (Robocop) */
			case 1: lines=128; break; /* 128 horizontal scroll registers (Not used?) */
			case 2: lines=64; break; /* 64 horizontal scroll registers (Not used?) */
			case 3: lines=32; break; /* 32 horizontal scroll registers (Heavy Barrel title screen) */
			case 4: lines=16; break; /* 16 horizontal scroll registers (Bad Dudes, Sly Spy) */
			case 5: lines=8; break; /* 8 horizontal scroll registers (Not used?) */
			case 6: lines=4; break; /* 4 horizontal scroll registers (Not used?) */
			case 7: lines=2; break; /* 2 horizontal scroll registers (Not used?) */
			default: lines=1;
		}

		tilemap_set_scroll_cols(tilemap_ptr,1);
		tilemap_set_scroll_rows(tilemap_ptr,lines*height);
		tilemap_set_scrolly(tilemap_ptr,0,scrolly);
		for (offs = 0; offs < lines*height; offs++)
			tilemap_set_scrollx(tilemap_ptr,offs,scrollx + dec0_pf3_rowscroll[offs]);
	}
	else { /* Scroll registers not enabled */
		tilemap_set_scroll_rows(tilemap_ptr,1);
		tilemap_set_scroll_cols(tilemap_ptr,1);
		tilemap_set_scrollx(tilemap_ptr,0,scrollx);
		tilemap_set_scrolly(tilemap_ptr,0,scrolly);
	}
}

/******************************************************************************/

static void dec0_pf1_draw(struct mame_bitmap *bitmap,const struct rectangle *cliprect,int flags,int pri)
{
	tilemap_set_enable(pf1_tilemap_0,0);
	tilemap_set_enable(pf1_tilemap_1,0);
	tilemap_set_enable(pf1_tilemap_2,0);

	switch (dec0_pf1_control_0[3]&0x3) {
		case 0:	/* 4x1 */
			tilemap_set_enable(pf1_tilemap_0,1);
			tilemap_draw(bitmap,cliprect,pf1_tilemap_0,flags,pri);
			break;
		case 1:	/* 2x2 */
		default:
			tilemap_set_enable(pf1_tilemap_1,1);
			tilemap_draw(bitmap,cliprect,pf1_tilemap_1,flags,pri);
			break;
		case 2:	/* 1x4 */
			tilemap_set_enable(pf1_tilemap_2,1);
			tilemap_draw(bitmap,cliprect,pf1_tilemap_2,flags,pri);
			break;
	}
}

static void dec0_pf2_draw(struct mame_bitmap *bitmap,const struct rectangle *cliprect,int flags,int pri)
{
	tilemap_set_enable(pf2_tilemap_0,0);
	tilemap_set_enable(pf2_tilemap_1,0);
	tilemap_set_enable(pf2_tilemap_2,0);

	switch (dec0_pf2_control_0[3]&0x3) {
		case 0:	/* 4x1 */
			tilemap_set_enable(pf2_tilemap_0,1);
			tilemap_draw(bitmap,cliprect,pf2_tilemap_0,flags,pri);
			break;
		case 1:	/* 2x2 */
		default:
			tilemap_set_enable(pf2_tilemap_1,1);
			tilemap_draw(bitmap,cliprect,pf2_tilemap_1,flags,pri);
			break;
		case 2:	/* 1x4 */
			tilemap_set_enable(pf2_tilemap_2,1);
			tilemap_draw(bitmap,cliprect,pf2_tilemap_2,flags,pri);
			break;
	}
}

static void dec0_pf3_draw(struct mame_bitmap *bitmap,const struct rectangle *cliprect,int flags,int pri)
{
	tilemap_set_enable(pf3_tilemap_0,0);
	tilemap_set_enable(pf3_tilemap_1,0);
	tilemap_set_enable(pf3_tilemap_2,0);

	switch (dec0_pf3_control_0[3]&0x3) {
		case 0:	/* 4x1 */
			tilemap_set_enable(pf3_tilemap_0,1);
			tilemap_draw(bitmap,cliprect,pf3_tilemap_0,flags,pri);
			break;
		case 1:	/* 2x2 */
		default:
			tilemap_set_enable(pf3_tilemap_1,1);
			tilemap_draw(bitmap,cliprect,pf3_tilemap_1,flags,pri);
			break;
		case 2:	/* 1x4 */
			tilemap_set_enable(pf3_tilemap_2,1);
			tilemap_draw(bitmap,cliprect,pf3_tilemap_2,flags,pri);
			break;
	}
}

/******************************************************************************/

VIDEO_UPDATE( hbarrel )
{
	dec0_pf1_update();
	dec0_pf2_update();
	dec0_pf3_update();

	dec0_pf3_draw(bitmap,cliprect,TILEMAP_BACK|TILEMAP_IGNORE_TRANSPARENCY,0);
	dec0_pf3_draw(bitmap,cliprect,TILEMAP_FRONT|TILEMAP_IGNORE_TRANSPARENCY,1);
	dec0_drawsprites(bitmap,cliprect,0x08,0x08);
	dec0_pf2_draw(bitmap,cliprect,TILEMAP_BACK,0);
	dec0_pf2_draw(bitmap,cliprect,TILEMAP_FRONT,1);

	/* HB always keeps pf2 on top of pf3, no need explicitly support priority register */

	dec0_drawsprites(bitmap,cliprect,0x08,0x00);
	dec0_pf1_draw(bitmap,cliprect,0,0);
}

/******************************************************************************/

VIDEO_UPDATE( baddudes )
{
	/* WARNING: priority inverted wrt all the other games */
	dec0_pf1_update();
	dec0_pf2_update();
	dec0_pf3_update();

	/* WARNING: inverted wrt Midnight Resistance */
	if ((dec0_pri & 0x01) == 0)
	{
		dec0_pf2_draw(bitmap,cliprect,TILEMAP_BACK|TILEMAP_IGNORE_TRANSPARENCY,0);
		if (!(dec0_pri & 2))
			dec0_pf2_draw(bitmap,cliprect,TILEMAP_FRONT|TILEMAP_IGNORE_TRANSPARENCY,1);

		dec0_pf3_draw(bitmap,cliprect,TILEMAP_BACK,0);
		if (!(dec0_pri & 4))
			dec0_pf3_draw(bitmap,cliprect,TILEMAP_FRONT,1);

		if (dec0_pri & 2)
			dec0_pf2_draw(bitmap,cliprect,TILEMAP_FRONT,1);

		dec0_drawsprites(bitmap,cliprect,0x00,0x00);

		if (dec0_pri & 4)
			dec0_pf3_draw(bitmap,cliprect,TILEMAP_FRONT,1); /* Foreground pens only */
	}
	else
	{
		dec0_pf3_draw(bitmap,cliprect,TILEMAP_BACK|TILEMAP_IGNORE_TRANSPARENCY,0);
		if (!(dec0_pri & 2))
			dec0_pf3_draw(bitmap,cliprect,TILEMAP_FRONT|TILEMAP_IGNORE_TRANSPARENCY,1);

		dec0_pf2_draw(bitmap,cliprect,TILEMAP_BACK,0);
		if (!(dec0_pri & 4))
			dec0_pf2_draw(bitmap,cliprect,TILEMAP_FRONT,1);

		if (dec0_pri & 2)
			dec0_pf3_draw(bitmap,cliprect,TILEMAP_FRONT,1);

		dec0_drawsprites(bitmap,cliprect,0x00,0x00);

		if (dec0_pri & 4)
			dec0_pf2_draw(bitmap,cliprect,TILEMAP_FRONT,1);
	}

	dec0_pf1_draw(bitmap,cliprect,0,0);
}

/******************************************************************************/

VIDEO_UPDATE( robocop )
{
	int trans;

	dec0_pf1_update();
	dec0_pf2_update();
	dec0_pf3_update();

	if (dec0_pri & 0x04)
		trans = 0x08;
	else
		trans = 0x00;

	if (dec0_pri & 0x01)
	{
		/* WARNING: inverted wrt Midnight Resistance */
		/* Robocop uses it only for the title screen, so this might be just */
		/* completely wrong. The top 8 bits of the register might mean */
		/* something (they are 0x80 in midres, 0x00 here) */
		dec0_pf2_draw(bitmap,cliprect,TILEMAP_BACK|TILEMAP_IGNORE_TRANSPARENCY,0);
		dec0_pf2_draw(bitmap,cliprect,TILEMAP_FRONT|TILEMAP_IGNORE_TRANSPARENCY,1);

		if (dec0_pri & 0x02)
			dec0_drawsprites(bitmap,cliprect,0x08,trans);

		dec0_pf3_draw(bitmap,cliprect,TILEMAP_BACK,0);
		dec0_pf3_draw(bitmap,cliprect,TILEMAP_FRONT,1);
	}
	else
	{
		dec0_pf3_draw(bitmap,cliprect,TILEMAP_BACK|TILEMAP_IGNORE_TRANSPARENCY,0);
		dec0_pf3_draw(bitmap,cliprect,TILEMAP_FRONT|TILEMAP_IGNORE_TRANSPARENCY,1);

		if (dec0_pri & 0x02)
			dec0_drawsprites(bitmap,cliprect,0x08,trans);

		dec0_pf2_draw(bitmap,cliprect,TILEMAP_BACK,0);
		dec0_pf2_draw(bitmap,cliprect,TILEMAP_FRONT,1);
	}

	if (dec0_pri & 0x02)
		dec0_drawsprites(bitmap,cliprect,0x08,trans ^ 0x08);
	else
		dec0_drawsprites(bitmap,cliprect,0x00,0x00);

	dec0_pf1_draw(bitmap,cliprect,0,0);
}

/******************************************************************************/

VIDEO_UPDATE( birdtry )
{
	/* This game doesn't have the extra playfield chip on the game board */
	dec0_pf1_update();
	dec0_pf2_update();
	dec0_pf2_draw(bitmap,cliprect,TILEMAP_BACK|TILEMAP_IGNORE_TRANSPARENCY,0);
	dec0_pf2_draw(bitmap,cliprect,TILEMAP_FRONT|TILEMAP_IGNORE_TRANSPARENCY,1);
	dec0_drawsprites(bitmap,cliprect,0x00,0x00);
	dec0_pf1_draw(bitmap,cliprect,0,0);
}

/******************************************************************************/

VIDEO_UPDATE( hippodrm )
{
	dec0_pf1_update();
	dec0_pf2_update();
	dec0_pf3_update();

	if (dec0_pri & 0x01)
	{
		dec0_pf2_draw(bitmap,cliprect,TILEMAP_BACK|TILEMAP_IGNORE_TRANSPARENCY,0);
		dec0_pf2_draw(bitmap,cliprect,TILEMAP_FRONT|TILEMAP_IGNORE_TRANSPARENCY,1);
		dec0_pf3_draw(bitmap,cliprect,TILEMAP_BACK,0);
		dec0_pf3_draw(bitmap,cliprect,TILEMAP_FRONT,1);
	}
	else
	{
		dec0_pf3_draw(bitmap,cliprect,TILEMAP_BACK|TILEMAP_IGNORE_TRANSPARENCY,0);
		dec0_pf3_draw(bitmap,cliprect,TILEMAP_FRONT|TILEMAP_IGNORE_TRANSPARENCY,1);
		dec0_pf2_draw(bitmap,cliprect,TILEMAP_BACK,0);
		dec0_pf2_draw(bitmap,cliprect,TILEMAP_FRONT,1);
	}

	dec0_drawsprites(bitmap,cliprect,0x00,0x00);
	dec0_pf1_draw(bitmap,cliprect,0,0);
}

/******************************************************************************/

VIDEO_UPDATE( slyspy )
{
	dec0_pf1_update();
	dec0_pf2_update();
	dec0_pf3_update();

	dec0_pf3_draw(bitmap,cliprect,TILEMAP_BACK|TILEMAP_IGNORE_TRANSPARENCY,0);
	dec0_pf3_draw(bitmap,cliprect,TILEMAP_FRONT|TILEMAP_IGNORE_TRANSPARENCY,1);
	dec0_pf2_draw(bitmap,cliprect,TILEMAP_BACK,0);
	if (!(dec0_pri&0x80))
		dec0_pf2_draw(bitmap,cliprect,TILEMAP_FRONT,1);

	dec0_drawsprites(bitmap,cliprect,0x00,0x00);

	if (dec0_pri&0x80)
		dec0_pf2_draw(bitmap,cliprect,TILEMAP_FRONT,1);

	dec0_pf1_draw(bitmap,cliprect,0,0);
}

/******************************************************************************/

VIDEO_UPDATE( midres )
{
	int trans;

	if (dec0_pri & 0x04)
		trans = 0x00;
	else trans = 0x08;

	dec0_pf1_update();
	dec0_pf2_update();
	dec0_pf3_update();

	if (dec0_pri & 0x01)
	{
		dec0_pf2_draw(bitmap,cliprect,TILEMAP_BACK|TILEMAP_IGNORE_TRANSPARENCY,0);
		dec0_pf2_draw(bitmap,cliprect,TILEMAP_FRONT|TILEMAP_IGNORE_TRANSPARENCY,1);

		if (dec0_pri & 0x02)
			dec0_drawsprites(bitmap,cliprect,0x08,trans);

		dec0_pf3_draw(bitmap,cliprect,TILEMAP_BACK,0);
		dec0_pf3_draw(bitmap,cliprect,TILEMAP_FRONT,1);
	}
	else
	{
		dec0_pf3_draw(bitmap,cliprect,TILEMAP_BACK|TILEMAP_IGNORE_TRANSPARENCY,0);
		dec0_pf3_draw(bitmap,cliprect,TILEMAP_FRONT|TILEMAP_IGNORE_TRANSPARENCY,1);

		if (dec0_pri & 0x02)
			dec0_drawsprites(bitmap,cliprect,0x08,trans);

		dec0_pf2_draw(bitmap,cliprect,TILEMAP_BACK,0);
		dec0_pf2_draw(bitmap,cliprect,TILEMAP_FRONT,1);
	}

	if (dec0_pri & 0x02)
		dec0_drawsprites(bitmap,cliprect,0x08,trans ^ 0x08);
	else
		dec0_drawsprites(bitmap,cliprect,0x00,0x00);

	dec0_pf1_draw(bitmap,cliprect,0,0);
}

/******************************************************************************/

WRITE16_HANDLER( dec0_pf1_control_0_w )
{
	COMBINE_DATA(&dec0_pf1_control_0[offset]);
}

WRITE16_HANDLER( dec0_pf1_control_1_w )
{
	COMBINE_DATA(&dec0_pf1_control_1[offset]);
}

WRITE16_HANDLER( dec0_pf1_data_w )
{
	data16_t oldword=dec0_pf1_data[offset];
	COMBINE_DATA(&dec0_pf1_data[offset]);
	if (oldword!=dec0_pf1_data[offset]) {
		tilemap_mark_tile_dirty(pf1_tilemap_0,offset);
		tilemap_mark_tile_dirty(pf1_tilemap_1,offset);
		tilemap_mark_tile_dirty(pf1_tilemap_2,offset);
	}
}

WRITE16_HANDLER( dec0_pf2_control_0_w )
{
	COMBINE_DATA(&dec0_pf2_control_0[offset]);
}

WRITE16_HANDLER( dec0_pf2_control_1_w )
{
	COMBINE_DATA(&dec0_pf2_control_1[offset]);
}

WRITE16_HANDLER( dec0_pf2_data_w )
{
	data16_t oldword=dec0_pf2_data[offset];
	COMBINE_DATA(&dec0_pf2_data[offset]);
	if (oldword!=dec0_pf2_data[offset]) {
		tilemap_mark_tile_dirty(pf2_tilemap_0,offset);
		tilemap_mark_tile_dirty(pf2_tilemap_1,offset);
		tilemap_mark_tile_dirty(pf2_tilemap_2,offset);
	}
}

WRITE16_HANDLER( dec0_pf3_control_0_w )
{
	COMBINE_DATA(&dec0_pf3_control_0[offset]);
}

WRITE16_HANDLER( dec0_pf3_control_1_w )
{
	COMBINE_DATA(&dec0_pf3_control_1[offset]);
}

WRITE16_HANDLER( dec0_pf3_data_w )
{
	data16_t oldword=dec0_pf3_data[offset];
	COMBINE_DATA(&dec0_pf3_data[offset]);
	if (oldword!=dec0_pf3_data[offset]) {
		tilemap_mark_tile_dirty(pf3_tilemap_0,offset);
		tilemap_mark_tile_dirty(pf3_tilemap_1,offset);
		tilemap_mark_tile_dirty(pf3_tilemap_2,offset);
	}
}

WRITE16_HANDLER( dec0_priority_w )
{
  	COMBINE_DATA(&dec0_pri);
}

WRITE_HANDLER( dec0_pf3_control_8bit_w )
{
	static int buffer[0x20];
	data16_t myword;

	buffer[offset]=data;

	/* Rearrange little endian bytes from H6280 into big endian words for 68k */
	offset&=0xffe;
	myword=buffer[offset] + (buffer[offset+1]<<8);

	if (offset<0x10) dec0_pf3_control_0_w(offset/2,myword,0);
	else dec0_pf3_control_1_w((offset-0x10)/2,myword,0);
}

WRITE_HANDLER( dec0_pf3_data_8bit_w )
{
	if (offset&1) { /* MSB has changed */
		data16_t lsb=dec0_pf3_data[offset>>1];
		data16_t newword=(lsb&0xff) | (data<<8);
		dec0_pf3_data[offset>>1]=newword;
	}
	else { /* LSB has changed */
		data16_t msb=dec0_pf3_data[offset>>1];
		data16_t newword=(msb&0xff00) | data;
		dec0_pf3_data[offset>>1]=newword;
	}
	tilemap_mark_tile_dirty(pf3_tilemap_0,offset>>1);
	tilemap_mark_tile_dirty(pf3_tilemap_1,offset>>1);
	tilemap_mark_tile_dirty(pf3_tilemap_2,offset>>1);
}

READ_HANDLER( dec0_pf3_data_8bit_r )
{
	if (offset&1) /* MSB */
		return dec0_pf3_data[offset>>1]>>8;

	return dec0_pf3_data[offset>>1]&0xff;
}

/******************************************************************************/

static UINT32 tile_shape0_scan(UINT32 col,UINT32 row,UINT32 num_cols,UINT32 num_rows)
{
	return (col & 0xf) + ((row & 0xf) << 4) + ((col & 0x30) << 4);
}

static UINT32 tile_shape1_scan(UINT32 col,UINT32 row,UINT32 num_cols,UINT32 num_rows)
{
	return (col & 0xf) + ((row & 0xf) << 4) + ((row & 0x10) << 4) + ((col & 0x10) << 5);
}

static UINT32 tile_shape2_scan(UINT32 col,UINT32 row,UINT32 num_cols,UINT32 num_rows)
{
	return (col & 0xf) + ((row & 0x3f) << 4);
}

static UINT32 tile_shape0_8x8_scan(UINT32 col,UINT32 row,UINT32 num_cols,UINT32 num_rows)
{
	return (col & 0x1f) + ((row & 0x1f) << 5) + ((col & 0x60) << 5);
}

static UINT32 tile_shape1_8x8_scan(UINT32 col,UINT32 row,UINT32 num_cols,UINT32 num_rows)
{
	return (col & 0x1f) + ((row & 0x1f) << 5) + ((row & 0x20) << 5) + ((col & 0x20) << 6);
}

static UINT32 tile_shape2_8x8_scan(UINT32 col,UINT32 row,UINT32 num_cols,UINT32 num_rows)
{
	return (col & 0x1f) + ((row & 0x7f) << 5);
}

static void get_pf1_tile_info(int tile_index)
{
	int tile=dec0_pf1_data[tile_index];
	SET_TILE_INFO(0,tile&0xfff,tile>>12,0)
}

static void get_pf2_tile_info(int tile_index)
{
	int tile=dec0_pf2_data[tile_index];
	int pri=((tile>>12)>7);
	SET_TILE_INFO(1,tile&0xfff,tile>>12,TILE_SPLIT(pri))
}

static void get_pf3_tile_info(int tile_index)
{
	int tile=dec0_pf3_data[tile_index];
	int pri=((tile>>12)>7);
	SET_TILE_INFO(2,tile&0xfff,tile>>12,TILE_SPLIT(pri))
}

VIDEO_START( dec0_nodma )
{
	pf1_tilemap_0 = tilemap_create(get_pf1_tile_info,tile_shape0_8x8_scan,TILEMAP_TRANSPARENT, 8, 8,128, 32);
	pf1_tilemap_1 = tilemap_create(get_pf1_tile_info,tile_shape1_8x8_scan,TILEMAP_TRANSPARENT, 8, 8, 64, 64);
	pf1_tilemap_2 = tilemap_create(get_pf1_tile_info,tile_shape2_8x8_scan,TILEMAP_TRANSPARENT, 8, 8, 32,128);
	pf2_tilemap_0 = tilemap_create(get_pf2_tile_info,tile_shape0_scan,    TILEMAP_SPLIT,      16,16, 64, 16);
	pf2_tilemap_1 = tilemap_create(get_pf2_tile_info,tile_shape1_scan,    TILEMAP_SPLIT,      16,16, 32, 32);
	pf2_tilemap_2 = tilemap_create(get_pf2_tile_info,tile_shape2_scan,    TILEMAP_SPLIT,      16,16, 16, 64);
	pf3_tilemap_0 = tilemap_create(get_pf3_tile_info,tile_shape0_scan,    TILEMAP_SPLIT,      16,16, 64, 16);
	pf3_tilemap_1 = tilemap_create(get_pf3_tile_info,tile_shape1_scan,    TILEMAP_SPLIT,      16,16, 32, 32);
	pf3_tilemap_2 = tilemap_create(get_pf3_tile_info,tile_shape2_scan,    TILEMAP_SPLIT,      16,16, 16, 64);

	if (!pf1_tilemap_0 || !pf1_tilemap_1 || !pf1_tilemap_2
		|| !pf2_tilemap_0 || !pf2_tilemap_1 || !pf2_tilemap_2
		|| !pf3_tilemap_0 || !pf3_tilemap_1 || !pf3_tilemap_2)
		return 1;

	tilemap_set_transparent_pen(pf1_tilemap_0,0);
	tilemap_set_transparent_pen(pf1_tilemap_1,0);
	tilemap_set_transparent_pen(pf1_tilemap_2,0);
	tilemap_set_transparent_pen(pf2_tilemap_0,0);
	tilemap_set_transparent_pen(pf2_tilemap_1,0);
	tilemap_set_transparent_pen(pf2_tilemap_2,0);
	tilemap_set_transparent_pen(pf3_tilemap_0,0);
	tilemap_set_transparent_pen(pf3_tilemap_1,0);
	tilemap_set_transparent_pen(pf3_tilemap_2,0);
	tilemap_set_transmask(pf2_tilemap_0,0,0xffff,0x0001); /* Transparent pen 1 only */
	tilemap_set_transmask(pf2_tilemap_1,0,0xffff,0x0001); /* Transparent pen 1 only */
	tilemap_set_transmask(pf2_tilemap_2,0,0xffff,0x0001); /* Transparent pen 1 only */
	tilemap_set_transmask(pf3_tilemap_0,0,0xffff,0x0001); /* Transparent pen 1 only */
	tilemap_set_transmask(pf3_tilemap_1,0,0xffff,0x0001); /* Transparent pen 1 only */
	tilemap_set_transmask(pf3_tilemap_2,0,0xffff,0x0001); /* Transparent pen 1 only */
	tilemap_set_transmask(pf2_tilemap_0,1,0x00ff,0xff01); /* Bottom/Top 8 pen split */
	tilemap_set_transmask(pf2_tilemap_1,1,0x00ff,0xff01); /* Bottom/Top 8 pen split */
	tilemap_set_transmask(pf2_tilemap_2,1,0x00ff,0xff01); /* Bottom/Top 8 pen split */
	tilemap_set_transmask(pf3_tilemap_0,1,0x00ff,0xff01); /* Bottom/Top 8 pen split */
	tilemap_set_transmask(pf3_tilemap_1,1,0x00ff,0xff01); /* Bottom/Top 8 pen split */
	tilemap_set_transmask(pf3_tilemap_2,1,0x00ff,0xff01); /* Bottom/Top 8 pen split */

	dec0_spriteram=spriteram16;

	return 0;
}

VIDEO_START( dec0 )
{
	video_start_dec0_nodma();
	dec0_spriteram=auto_malloc(0x800);

	return 0;
}

/******************************************************************************/
