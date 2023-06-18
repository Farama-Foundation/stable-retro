#include "driver.h"
#include "state.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/taitoic.h"

#define TC0100SCN_GFX_NUM 1
#define TC0480SCP_GFX_NUM 1

static int sci_spriteframe;
extern data16_t *taitoz_sharedram;



/**********************************************************/

static int taitoz_core_vh_start (int x_offs)
{
	if (has_TC0480SCP())	/* it's Dblaxle, a tc0480scp game */
	{
		if (TC0480SCP_vh_start(TC0480SCP_GFX_NUM,x_offs,0x21,0x08,4,0,0,0,0))
			return 1;
	}
	else	/* it's a tc0100scn game */
	{
		if (TC0100SCN_vh_start(1,TC0100SCN_GFX_NUM,x_offs,0,0,0,0,0,0))
			return 1;
	}

	if (has_TC0150ROD())
		if (TC0150ROD_vh_start())
			return 1;

	if (has_TC0110PCR())
		if (TC0110PCR_vh_start())
			return 1;

	return 0;
}

VIDEO_START( taitoz )
{
	return (taitoz_core_vh_start(0));
}

VIDEO_START( spacegun )
{
	return (taitoz_core_vh_start(4));
}

/********************************************************
            SPRITE READ AND WRITE HANDLERS
********************************************************/


READ16_HANDLER( sci_spriteframe_r )
{
	return (sci_spriteframe << 8);
}

WRITE16_HANDLER( sci_spriteframe_w )
{
	sci_spriteframe = (data >> 8) &0xff;
}


/************************************************************
                   SPRITE DRAW ROUTINES

These draw a series of small tiles ("chunks") together to
create each big sprite. The spritemap rom provides the lookup
table for this. E.g. Spacegun looks up 16x8 sprite chunks
from the spritemap rom, creating this 64x64 sprite (numbers
are the word offset into the spritemap rom):

	 0  1  2  3
	 4  5  6  7
	 8  9 10 11
	12 13 14 15
	16 17 18 19
	20 21 22 23
	24 25 26 27
	28 29 30 31

Chasehq/Nightstr are the only games to build from 16x16
tiles. They are also more complicated to draw, as they have
3 different aggregation formats [32/64/128 x 128]
whereas the other games stick to one, typically 64x64.

All the games make heavy use of sprite zooming.

I'm 99% sure there are probably just two levels of sprite
priority - under and over the road - but this isn't certain.

		***

[The routines for the 16x8 tile games seem to have large
common elements that could be extracted as subroutines.]

NB: unused portions of the spritemap rom contain hex FF's.
It is a useful coding check to warn in the log if these
are being accessed. [They can be inadvertently while
spriteram is being tested, take no notice of that.]

BUT: Nightstr uses code 0x3ff as a mask sprite. This
references 0x1ff00-ff in spritemap rom, which is an 0xff fill.
This must be accessing sprite chunk 0x3fff: in other words the
top bits are thrown away and so accessing chunk 0xfe45 would
display chunk 0x3e45 etc.


		Aquajack/Spacegun (modified table from Raine; size of bit
		masks verified in Spacegun code)

		Byte | Bit(s) | Description
		-----+76543210+-------------------------------------
		  0  |xxxxxxx.| ZoomY (0 min, 63 max - msb unused as sprites are 64x64)
		  0  |.......x| Y position (High)
		  1  |xxxxxxxx| Y position (Low)
		  2  |x.......| Priority (0=sprites high)
		  2  |.x......| Flip X
		  2  |..?????.| unknown/unused ?
		  2  |.......x| X position (High)
		  3  |xxxxxxxx| X position (Low)
		  4  |xxxxxxxx| Palette bank
		  5  |?.......| unknown/unused ?
		  5  |.xxxxxxx| ZoomX (0 min, 63 max - msb unused as sprites are 64x64)
		  6  |x.......| Flip Y ?
		  6  |.??.....| unknown/unused ?
		  6  |...xxxxx| Sprite Tile high (msb unused - half of spritemap rom empty)
		  7  |xxxxxxxx| Sprite Tile low

		Continental circus (modified Raine table): note similar format.
		The zoom msb is actually used, as sprites are 128x128.

		---+-------------------+--------------
		 0 | xxxxxxx. ........ | ZoomY
		 0 | .......x xxxxxxxx | Y position
						// unsure about Flip Y
		 2 | .....xxx xxxxxxxx | Sprite Tile
		 4 | x....... ........ | Priority (0=sprites high)
		 4 | .x...... ........ | Flip X
		 4 | .......x xxxxxxxx | X position
		 6 | xxxxxxxx ........ | Palette bank
		 6 | ........ .xxxxxxx | ZoomX
		---+-------------------+--------------

		 Bshark/Chasehq/Nightstr/SCI (modified Raine table): similar format.
		 The zoom msb is only used for 128x128 sprites.

		-----+--------+------------------------
		Byte | Bit(s) | Description
		-----+76543210+------------------------
		  0  |xxxxxxx.| ZoomY
		  0  |.......x| Y position (High)
		  1  |xxxxxxxx| Y position (Low)
		  2  |x.......| Priority (0=sprites high)
		  2  |.xxxxxxx| Palette bank (High)
		  3  |x.......| Palette bank (Low)
		  3  |.xxxxxxx| ZoomX
		  4  |x.......| Flip Y
		  4  |.x......| Flip X
		  4  |.......x| X position (High)
		  5  |xxxxxxxx| X position (Low)
		  6  |...xxxxx| Sprite Tile (High)
		  7  |xxxxxxxx| Sprite Tile (Low)
		-----+--------+------------------------

 [Raine Chasehq sprite plotting is peculiar. It determines
 the type of big sprite by reference to the zoomx and y.
 Therefore it says that the big sprite number is 0-0x7ff
 across ALL three sizes and you can't distinguish them by
 tilenum. FWIW I seem to be ok just using zoomx.]

TODO: Contcirc, Aquajack, Spacegun need flip y bit to be
confirmed

********************************************************/


static void contcirc_draw_sprites_16x8(struct mame_bitmap *bitmap,const struct rectangle *cliprect,int y_offs)
{
	data16_t *spritemap = (data16_t *)memory_region(REGION_USER1);
	int offs, data, tilenum, color, flipx, flipy;
	int x, y, priority, curx, cury;
	int sprites_flipscreen = 0;
	int zoomx, zoomy, zx, zy;
	int sprite_chunk,map_offset,code,j,k,px,py;
	int bad_chunks;
	int primasks[2] = {0xf0,0xfc};

	for (offs = 0;offs < spriteram_size/2;offs += 4)
	{
		data = spriteram16[offs+0];
		zoomy = (data & 0xfe00) >> 9;
		y = data & 0x1ff;

		data = spriteram16[offs+1];
		tilenum = data & 0x7ff;		/* $80000 spritemap rom maps up to $7ff 128x128 sprites */

		data = spriteram16[offs+2];
		priority = (data & 0x8000) >> 15;
		flipx = (data & 0x4000) >> 14;
		flipy = (data & 0x2000) >> 13;	/* ???*/
		x = data & 0x1ff;   /* correct mask?*/

		data = spriteram16[offs+3];
		color = (data & 0xff00) >> 8;
		zoomx = (data & 0x7f);

		if (!tilenum) continue;

		map_offset = tilenum << 7;

		zoomx += 1;
		zoomy += 1;

		y += y_offs;
		y += (128-zoomy);

		/* treat coords as signed */
		if (x>0x140) x -= 0x200;
		if (y>0x140) y -= 0x200;

		bad_chunks = 0;

		for (sprite_chunk=0;sprite_chunk<128;sprite_chunk++)
		{
			k = sprite_chunk % 8;   /* 8 sprite chunks per row */
			j = sprite_chunk / 8;   /* 16 rows */

			px = flipx ? (7-k) : k;	/* pick tiles back to front for x and y flips */
			py = flipy ? (15-j) : j;

			code = spritemap[map_offset + px + (py<<3)];

			if (code == 0xffff)	bad_chunks++;

			curx = x + ((k*zoomx)/8);
			cury = y + ((j*zoomy)/16);

			zx = x + (((k+1)*zoomx)/8) - curx;
			zy = y + (((j+1)*zoomy)/16) - cury;

			if (sprites_flipscreen)
			{
				/* -zx/y is there to fix zoomed sprite coords in screenflip.
				   drawgfxzoom does not know to draw from flip-side of sprites when
				   screen is flipped; so we must correct the coords ourselves. */

				curx = 320 - curx - zx;
				cury = 256 - cury - zy;
				flipx = !flipx;
				flipy = !flipy;
			}

			pdrawgfxzoom(bitmap,Machine->gfx[0],
					code,
					color,
					flipx,flipy,
					curx,cury,
					cliprect,TRANSPARENCY_PEN,0,
					zx<<12,zy<<13,primasks[priority]);
		}

		if (bad_chunks)
log_cb(RETRO_LOG_DEBUG, LOGPRE "Sprite number %04x had %02x invalid chunks\n",tilenum,bad_chunks);
	}
}



static void chasehq_draw_sprites_16x16(struct mame_bitmap *bitmap,const struct rectangle *cliprect,int y_offs)
{
	data16_t *spritemap = (data16_t *)memory_region(REGION_USER1);
	int offs, data, tilenum, color, flipx, flipy;
	int x, y, priority, curx, cury;
	int sprites_flipscreen = 0;
	int zoomx, zoomy, zx, zy;
	int sprite_chunk,map_offset,code,j,k,px,py;
	int bad_chunks;
	int primasks[2] = {0xf0,0xfc};

	for (offs = spriteram_size/2-4;offs >=0;offs -= 4)
	{
		data = spriteram16[offs+0];
		zoomy = (data & 0xfe00) >> 9;
		y = data & 0x1ff;

		data = spriteram16[offs+1];
		priority = (data & 0x8000) >> 15;
		color = (data & 0x7f80) >> 7;
		zoomx = (data & 0x7f);

		data = spriteram16[offs+2];
		flipy = (data & 0x8000) >> 15;
		flipx = (data & 0x4000) >> 14;
		x = data & 0x1ff;

		data = spriteram16[offs+3];
		/* higher bits are sometimes used... e.g. sign over flashing enemy car...! */
		tilenum = data & 0x7ff;

		if (!tilenum) continue;

		zoomx += 1;
		zoomy += 1;

		y += y_offs;
		y += (128-zoomy);

		/* treat coords as signed */
		if (x>0x140) x -= 0x200;
		if (y>0x140) y -= 0x200;

		bad_chunks = 0;

		if ((zoomx-1) & 0x40)	/* 128x128 sprites, $0-$3ffff in spritemap rom, OBJA */
		{
			map_offset = tilenum << 6;

			for (sprite_chunk=0;sprite_chunk<64;sprite_chunk++)
			{
				j = sprite_chunk / 8;   /* 8 rows */
				k = sprite_chunk % 8;   /* 8 sprite chunks per row */

				px = flipx ? (7-k) : k;	/* pick tiles back to front for x and y flips */
				py = flipy ? (7-j) : j;

				code = spritemap[map_offset + px + (py<<3)];

				if (code == 0xffff)	bad_chunks++;

				curx = x + ((k*zoomx)/8);
				cury = y + ((j*zoomy)/8);

				zx = x + (((k+1)*zoomx)/8) - curx;
				zy = y + (((j+1)*zoomy)/8) - cury;

				if (sprites_flipscreen)
				{
					/* -zx/y is there to fix zoomed sprite coords in screenflip.
					   drawgfxzoom does not know to draw from flip-side of sprites when
					   screen is flipped; so we must correct the coords ourselves. */

					curx = 320 - curx - zx;
					cury = 256 - cury - zy;
					flipx = !flipx;
					flipy = !flipy;
				}

				pdrawgfxzoom(bitmap,Machine->gfx[0],
						code,
						color,
						flipx,flipy,
						curx,cury,
						cliprect,TRANSPARENCY_PEN,0,
						zx<<12,zy<<12,
						primasks[priority]);
			}
		}
		else if ((zoomx-1) & 0x20)	/* 64x128 sprites, $40000-$5ffff in spritemap rom, OBJB */
		{
			map_offset = (tilenum << 5) + 0x20000;

			for (sprite_chunk=0;sprite_chunk<32;sprite_chunk++)
			{
				j = sprite_chunk / 4;   /* 8 rows */
				k = sprite_chunk % 4;   /* 4 sprite chunks per row */

				px = flipx ? (3-k) : k;	/* pick tiles back to front for x and y flips */
				py = flipy ? (7-j) : j;

				code = spritemap[map_offset + px + (py<<2)];

				if (code == 0xffff)	bad_chunks++;

				curx = x + ((k*zoomx)/4);
				cury = y + ((j*zoomy)/8);

				zx = x + (((k+1)*zoomx)/4) - curx;
				zy = y + (((j+1)*zoomy)/8) - cury;

				if (sprites_flipscreen)
				{
					/* -zx/y is there to fix zoomed sprite coords in screenflip.
					   drawgfxzoom does not know to draw from flip-side of sprites when
					   screen is flipped; so we must correct the coords ourselves. */

					curx = 320 - curx - zx;
					cury = 256 - cury - zy;
					flipx = !flipx;
					flipy = !flipy;
				}

				pdrawgfxzoom(bitmap,Machine->gfx[2],
						code,
						color,
						flipx,flipy,
						curx,cury,
						cliprect,TRANSPARENCY_PEN,0,
						zx<<12,zy<<12,
						primasks[priority]);
			}
		}
		else if (!((zoomx-1) & 0x60))	/* 32x128 sprites, $60000-$7ffff in spritemap rom, OBJB */
		{
			map_offset = (tilenum << 4) + 0x30000;

			for (sprite_chunk=0;sprite_chunk<16;sprite_chunk++)
			{
				j = sprite_chunk / 2;   /* 8 rows */
				k = sprite_chunk % 2;   /* 2 sprite chunks per row */

				px = flipx ? (1-k) : k;	/* pick tiles back to front for x and y flips */
				py = flipy ? (7-j) : j;

				code = spritemap[map_offset + px + (py<<1)];

				if (code == 0xffff)	bad_chunks ++;

				curx = x + ((k*zoomx)/2);
				cury = y + ((j*zoomy)/8);

				zx = x + (((k+1)*zoomx)/2) - curx;
				zy = y + (((j+1)*zoomy)/8) - cury;

				if (sprites_flipscreen)
				{
					/* -zx/y is there to fix zoomed sprite coords in screenflip.
					   drawgfxzoom does not know to draw from flip-side of sprites when
					   screen is flipped; so we must correct the coords ourselves. */

					curx = 320 - curx - zx;
					cury = 256 - cury - zy;
					flipx = !flipx;
					flipy = !flipy;
				}

				pdrawgfxzoom(bitmap,Machine->gfx[2],
						code,
						color,
						flipx,flipy,
						curx,cury,
						cliprect,TRANSPARENCY_PEN,0,
						zx<<12,zy<<12,
						primasks[priority]);
			}
		}

		if (bad_chunks)
log_cb(RETRO_LOG_DEBUG, LOGPRE "Sprite number %04x had %02x invalid chunks\n",tilenum,bad_chunks);
	}
}



static void bshark_draw_sprites_16x8(struct mame_bitmap *bitmap,const struct rectangle *cliprect,int y_offs)
{
	data16_t *spritemap = (data16_t *)memory_region(REGION_USER1);
	int offs, data, tilenum, color, flipx, flipy;
	int x, y, priority, curx, cury;
	int sprites_flipscreen = 0;
	int zoomx, zoomy, zx, zy;
	int sprite_chunk,map_offset,code,j,k,px,py;
	int bad_chunks;
	int primasks[2] = {0xf0,0xfc};

	for (offs = spriteram_size/2-4;offs >= 0;offs -= 4)
	{
		data = spriteram16[offs+0];
		zoomy = (data & 0x7e00) >> 9;
		y = data & 0x1ff;

		data = spriteram16[offs+1];
		priority = (data & 0x8000) >> 15;
		color = (data & 0x7f80) >> 7;
		zoomx = (data & 0x3f);

		data = spriteram16[offs+2];
		flipy = (data & 0x8000) >> 15;
		flipx = (data & 0x4000) >> 14;
		x = data & 0x1ff;

		data = spriteram16[offs+3];
		tilenum = data & 0x1fff;	/* $80000 spritemap rom maps up to $2000 64x64 sprites */

		if (!tilenum) continue;

		map_offset = tilenum << 5;

		zoomx += 1;
		zoomy += 1;

		y += y_offs;
		y += (64-zoomy);

		/* treat coords as signed */
		if (x>0x140) x -= 0x200;
		if (y>0x140) y -= 0x200;

		bad_chunks = 0;

		for (sprite_chunk=0;sprite_chunk<32;sprite_chunk++)
		{
			k = sprite_chunk % 4;   /* 4 sprite chunks per row */
			j = sprite_chunk / 4;   /* 8 rows */

			px = flipx ? (3-k) : k;	/* pick tiles back to front for x and y flips */
			py = flipy ? (7-j) : j;

			code = spritemap[map_offset + px + (py<<2)];

			if (code == 0xffff)	bad_chunks++;

			curx = x + ((k*zoomx)/4);
			cury = y + ((j*zoomy)/8);

			zx = x + (((k+1)*zoomx)/4) - curx;
			zy = y + (((j+1)*zoomy)/8) - cury;

			if (sprites_flipscreen)
			{
				/* -zx/y is there to fix zoomed sprite coords in screenflip.
				   drawgfxzoom does not know to draw from flip-side of sprites when
				   screen is flipped; so we must correct the coords ourselves. */

				curx = 320 - curx - zx;
				cury = 256 - cury - zy;
				flipx = !flipx;
				flipy = !flipy;
			}

			pdrawgfxzoom(bitmap,Machine->gfx[0],
					code,
					color,
					flipx,flipy,
					curx,cury,
					cliprect,TRANSPARENCY_PEN,0,
					zx<<12,zy<<13,
					primasks[priority]);
		}

		if (bad_chunks)
log_cb(RETRO_LOG_DEBUG, LOGPRE "Sprite number %04x had %02x invalid chunks\n",tilenum,bad_chunks);
	}
}



static void sci_draw_sprites_16x8(struct mame_bitmap *bitmap,const struct rectangle *cliprect,int y_offs)
{
	data16_t *spritemap = (data16_t *)memory_region(REGION_USER1);
	int offs, start_offs, data, tilenum, color, flipx, flipy;
	int x, y, priority, curx, cury;
	int sprites_flipscreen = 0;
	int zoomx, zoomy, zx, zy;
	int sprite_chunk,map_offset,code,j,k,px,py;
	int bad_chunks;
	int primasks[2] = {0xf0,0xfc};

	/* SCI alternates between two areas of its spriteram */

	/* This gave back to front frames causing bad flicker... but*/
	/* reversing it now only gives us sprite updates on alternate*/
	/* frames. So we probably have to partly buffer spriteram?*/

	start_offs = (sci_spriteframe &1) * 0x800;
	start_offs = 0x800 - start_offs;

	for (offs = (start_offs + 0x800 - 4);offs >= start_offs;offs -= 4)
	{
		data = spriteram16[offs+0];
		zoomy = (data & 0x7e00) >> 9;
		y = data & 0x1ff;

		data = spriteram16[offs+1];
		priority = (data & 0x8000) >> 15;
		color = (data & 0x7f80) >> 7;
		zoomx = (data & 0x3f);

		data = spriteram16[offs+2];
		flipy = (data & 0x8000) >> 15;
		flipx = (data & 0x4000) >> 14;
		x = data & 0x1ff;

		data = spriteram16[offs+3];
		tilenum = data & 0x1fff;	/* $80000 spritemap rom maps up to $2000 64x64 sprites */

		if (!tilenum) continue;

		map_offset = tilenum << 5;

		zoomx += 1;
		zoomy += 1;

		y += y_offs;
		y += (64-zoomy);

		/* treat coords as signed */
		if (x>0x140) x -= 0x200;
		if (y>0x140) y -= 0x200;

		bad_chunks = 0;

		for (sprite_chunk=0;sprite_chunk<32;sprite_chunk++)
		{
			j = sprite_chunk / 4;   /* 8 rows */
			k = sprite_chunk % 4;   /* 4 sprite chunks per row */

			px = flipx ? (3-k) : k;	/* pick tiles back to front for x and y flips */
			py = flipy ? (7-j) : j;

			code = spritemap[map_offset + px + (py<<2)];

			if (code == 0xffff)	bad_chunks++;

			curx = x + ((k*zoomx)/4);
			cury = y + ((j*zoomy)/8);

			zx = x + (((k+1)*zoomx)/4) - curx;
			zy = y + (((j+1)*zoomy)/8) - cury;

			if (sprites_flipscreen)
			{
				/* -zx/y is there to fix zoomed sprite coords in screenflip.
				   drawgfxzoom does not know to draw from flip-side of sprites when
				   screen is flipped; so we must correct the coords ourselves. */

				curx = 320 - curx - zx;
				cury = 256 - cury - zy;
				flipx = !flipx;
				flipy = !flipy;
			}

			pdrawgfxzoom(bitmap,Machine->gfx[0],
					code,
					color,
					flipx,flipy,
					curx,cury,
					cliprect,TRANSPARENCY_PEN,0,
					zx<<12,zy<<13,
					primasks[priority]);
		}

		if (bad_chunks)
log_cb(RETRO_LOG_DEBUG, LOGPRE "Sprite number %04x had %02x invalid chunks\n",tilenum,bad_chunks);
	}
}



static void aquajack_draw_sprites_16x8(struct mame_bitmap *bitmap,const struct rectangle *cliprect,int y_offs)
{
	data16_t *spritemap = (data16_t *)memory_region(REGION_USER1);
	int offs, data, tilenum, color, flipx, flipy;
	int x, y, priority, curx, cury;
	int sprites_flipscreen = 0;
	int zoomx, zoomy, zx, zy;
	int sprite_chunk,map_offset,code,j,k,px,py;
	int bad_chunks;
	int primasks[2] = {0xf0,0xfc};

	for (offs = 0;offs < spriteram_size/2;offs += 4)
	{
		data = spriteram16[offs+0];
		zoomy = (data & 0x7e00) >> 9;
		y = data & 0x1ff;

		data = spriteram16[offs+1];
		priority = (data & 0x8000) >> 15;
		flipx = (data & 0x4000) >> 14;
		x = data & 0x1ff;   /* correct mask?*/

		data = spriteram16[offs+2];
		color = (data & 0xff00) >> 8;
		zoomx = (data & 0x3f);

		data = spriteram16[offs+3];
		flipy = (data & 0x8000) >> 15;	/* ???*/
		tilenum = data & 0x1fff;	/* $80000 spritemap rom maps up to $2000 64x64 sprites */

		if (!tilenum) continue;

		map_offset = tilenum << 5;

		zoomx += 1;
		zoomy += 1;

		y += y_offs;

		/* treat coords as signed */
		if (x>0x140) x -= 0x200;
		if (y>0x140) y -= 0x200;

		bad_chunks = 0;

		for (sprite_chunk=0;sprite_chunk<32;sprite_chunk++)
		{
			k = sprite_chunk % 4;   /* 4 sprite chunks per row */
			j = sprite_chunk / 4;   /* 8 rows */

			px = flipx ? (3-k) : k;	/* pick tiles back to front for x and y flips */
			py = flipy ? (7-j) : j;

			code = spritemap[map_offset + px + (py<<2)];

			if (code == 0xffff)	bad_chunks++;

			curx = x + ((k*zoomx)/4);
			cury = y + ((j*zoomy)/8);

			zx = x + (((k+1)*zoomx)/4) - curx;
			zy = y + (((j+1)*zoomy)/8) - cury;

			if (sprites_flipscreen)
			{
				/* -zx/y is there to fix zoomed sprite coords in screenflip.
				   drawgfxzoom does not know to draw from flip-side of sprites when
				   screen is flipped; so we must correct the coords ourselves. */

				curx = 320 - curx - zx;
				cury = 256 - cury - zy;
				flipx = !flipx;
				flipy = !flipy;
			}

			pdrawgfxzoom(bitmap,Machine->gfx[0],
					code,
					color,
					flipx,flipy,
					curx,cury,
					cliprect,TRANSPARENCY_PEN,0,
					zx<<12,zy<<13,
					primasks[priority]);
		}

		if (bad_chunks)
log_cb(RETRO_LOG_DEBUG, LOGPRE "Sprite number %04x had %02x invalid chunks\n",tilenum,bad_chunks);
	}
}



static void spacegun_draw_sprites_16x8(struct mame_bitmap *bitmap,const struct rectangle *cliprect,int y_offs)
{
	data16_t *spritemap = (data16_t *)memory_region(REGION_USER1);
	int offs, data, tilenum, color, flipx, flipy;
	int x, y, priority, curx, cury;
	int sprites_flipscreen = 0;
	int zoomx, zoomy, zx, zy;
	int sprite_chunk,map_offset,code,j,k,px,py;
	int bad_chunks;
	int primasks[2] = {0xf0,0xfc};

	for (offs = 0; offs < spriteram_size/2-4;offs += 4)
	{
		data = spriteram16[offs+0];
		zoomy = (data & 0xfe00) >> 9;
		y = data & 0x1ff;

		data = spriteram16[offs+1];
		priority = (data & 0x8000) >> 15;
		flipx = (data & 0x4000) >> 14;
		x = data & 0x1ff;   /* correct mask?*/

		data = spriteram16[offs+2];
		color = (data & 0xff00) >> 8;
		zoomx = (data & 0x7f);

		data = spriteram16[offs+3];
		flipy = (data & 0x8000) >> 15;	/* ???*/
		tilenum = data & 0x1fff;	/* $80000 spritemap rom maps up to $2000 64x64 sprites */

		if (!tilenum) continue;

		map_offset = tilenum << 5;

		zoomx += 1;
		zoomy += 1;

		y += y_offs;

		/* treat coords as signed */
		if (x>0x140) x -= 0x200;
		if (y>0x140) y -= 0x200;

		bad_chunks = 0;

		for (sprite_chunk=0;sprite_chunk<32;sprite_chunk++)
		{
			k = sprite_chunk % 4;   /* 4 sprite chunks per row */
			j = sprite_chunk / 4;   /* 8 rows */

			px = flipx ? (3-k) : k;	/* pick tiles back to front for x and y flips */
			py = flipy ? (7-j) : j;

			code = spritemap[map_offset + px + (py<<2)];

			if (code == 0xffff)	bad_chunks++;

			curx = x + ((k*zoomx)/4);
			cury = y + ((j*zoomy)/8);

			zx = x + (((k+1)*zoomx)/4) - curx;
			zy = y + (((j+1)*zoomy)/8) - cury;

			if (sprites_flipscreen)
			{
				/* -zx/y is there to fix zoomed sprite coords in screenflip.
				   drawgfxzoom does not know to draw from flip-side of sprites when
				   screen is flipped; so we must correct the coords ourselves. */

				curx = 320 - curx - zx;
				cury = 256 - cury - zy;
				flipx = !flipx;
				flipy = !flipy;
			}

			pdrawgfxzoom(bitmap,Machine->gfx[0],
					code,
					color,
					flipx,flipy,
					curx,cury,
					cliprect,TRANSPARENCY_PEN,0,
					zx<<12,zy<<13,
					primasks[priority]);
		}

		if (bad_chunks)
log_cb(RETRO_LOG_DEBUG, LOGPRE "Sprite number %04x had %02x invalid chunks\n",tilenum,bad_chunks);
	}
}


/**************************************************************
                        SCREEN REFRESH
**************************************************************/

VIDEO_UPDATE( contcirc )
{
	UINT8 layer[3];

	TC0100SCN_tilemap_update();

	layer[0] = TC0100SCN_bottomlayer(0);
	layer[1] = layer[0]^1;
	layer[2] = 2;

	fillbitmap(priority_bitmap,0,cliprect);

	/* Ensure screen blanked even when bottom layer not drawn due to disable bit */
	fillbitmap(bitmap, Machine->pens[0], cliprect);

	TC0100SCN_tilemap_draw(bitmap,cliprect,0,layer[0],TILEMAP_IGNORE_TRANSPARENCY,0);
	TC0100SCN_tilemap_draw(bitmap,cliprect,0,layer[1],0,1);
	TC0150ROD_draw(bitmap,cliprect,-3,0xc0,1,0,2);	/* -6*/
	TC0100SCN_tilemap_draw(bitmap,cliprect,0,layer[2],0,4);

	contcirc_draw_sprites_16x8(bitmap,cliprect,5);	/* 7*/
}


/* Nightstr and ChaseHQ */

VIDEO_UPDATE( chasehq )
{
	UINT8 layer[3];

	TC0100SCN_tilemap_update();

	layer[0] = TC0100SCN_bottomlayer(0);
	layer[1] = layer[0]^1;
	layer[2] = 2;

	fillbitmap(priority_bitmap,0,cliprect);

	/* Ensure screen blanked even when bottom layer not drawn due to disable bit */
	fillbitmap(bitmap, Machine->pens[0], cliprect);

	TC0100SCN_tilemap_draw(bitmap,cliprect,0,layer[0],TILEMAP_IGNORE_TRANSPARENCY,0);
	TC0100SCN_tilemap_draw(bitmap,cliprect,0,layer[1],0,1);
	TC0150ROD_draw(bitmap,cliprect,-1,0xc0,0,0,2);
	TC0100SCN_tilemap_draw(bitmap,cliprect,0,layer[2],0,4);

	chasehq_draw_sprites_16x16(bitmap,cliprect,7);
}


VIDEO_UPDATE( bshark )
{
	UINT8 layer[3];

	TC0100SCN_tilemap_update();

	layer[0] = TC0100SCN_bottomlayer(0);
	layer[1] = layer[0]^1;
	layer[2] = 2;

	fillbitmap(priority_bitmap,0,cliprect);

	/* Ensure screen blanked even when bottom layer not drawn due to disable bit */
	fillbitmap(bitmap, Machine->pens[0],cliprect);

	TC0100SCN_tilemap_draw(bitmap,cliprect,0,layer[0],TILEMAP_IGNORE_TRANSPARENCY,0);
	TC0100SCN_tilemap_draw(bitmap,cliprect,0,layer[1],0,1);
	TC0150ROD_draw(bitmap,cliprect,-1,0xc0,0,1,2);
	TC0100SCN_tilemap_draw(bitmap,cliprect,0,layer[2],0,4);

	bshark_draw_sprites_16x8(bitmap,cliprect,8);
}


VIDEO_UPDATE( sci )
{
	UINT8 layer[3];

	TC0100SCN_tilemap_update();

	layer[0] = TC0100SCN_bottomlayer(0);
	layer[1] = layer[0]^1;
	layer[2] = 2;

	fillbitmap(priority_bitmap,0,cliprect);

	/* Ensure screen blanked even when bottom layer not drawn due to disable bit */
	fillbitmap(bitmap, Machine->pens[0], cliprect);

	TC0100SCN_tilemap_draw(bitmap,cliprect,0,layer[0],TILEMAP_IGNORE_TRANSPARENCY,0);
	TC0100SCN_tilemap_draw(bitmap,cliprect,0,layer[1],0,1);
	TC0150ROD_draw(bitmap,cliprect,-1,0xc0,0,0,2);
	TC0100SCN_tilemap_draw(bitmap,cliprect,0,layer[2],0,4);

	sci_draw_sprites_16x8(bitmap,cliprect,6);
}


VIDEO_UPDATE( aquajack )
{
	UINT8 layer[3];

	TC0100SCN_tilemap_update();

	layer[0] = TC0100SCN_bottomlayer(0);
	layer[1] = layer[0]^1;
	layer[2] = 2;

	fillbitmap(priority_bitmap,0,cliprect);

	/* Ensure screen blanked even when bottom layer not drawn due to disable bit */
	fillbitmap(bitmap, Machine->pens[0], cliprect);

	TC0100SCN_tilemap_draw(bitmap,cliprect,0,layer[0],TILEMAP_IGNORE_TRANSPARENCY,0);
	TC0100SCN_tilemap_draw(bitmap,cliprect,0,layer[1],0,1);
	TC0150ROD_draw(bitmap,cliprect,-1,0,2,1,2);
	TC0100SCN_tilemap_draw(bitmap,cliprect,0,layer[2],0,4);

	aquajack_draw_sprites_16x8(bitmap,cliprect,3);
}


VIDEO_UPDATE( spacegun )
{
	UINT8 layer[3];

	TC0100SCN_tilemap_update();

	layer[0] = TC0100SCN_bottomlayer(0);
	layer[1] = layer[0]^1;
	layer[2] = 2;

	fillbitmap(priority_bitmap,0,cliprect);

	/* Ensure screen blanked even when bottom layer not drawn due to disable bit */
	fillbitmap(bitmap, Machine->pens[0], cliprect);

	TC0100SCN_tilemap_draw(bitmap,cliprect,0,layer[0],TILEMAP_IGNORE_TRANSPARENCY,1);
	TC0100SCN_tilemap_draw(bitmap,cliprect,0,layer[1],0,2);
	TC0100SCN_tilemap_draw(bitmap,cliprect,0,layer[2],0,4);

	spacegun_draw_sprites_16x8(bitmap,cliprect,4);

	/* Draw artificial gun targets */
	{
		int rawx, rawy, centrex, centrey, screenx, screeny;

		/* A lag of one frame can be seen with the scope pickup and on the test
		   screen, however there is conflicting evidence so this isn't emulated
		   During high score entry the scope is displayed slightly offset
		   from the crosshair, close inspection shows the crosshair location
		   being used to target letters. */

		/* calculate p1 screen co-ords by matching routine at $195D4 */
		rawx = taitoz_sharedram[0xd94/2];
		centrex = taitoz_sharedram[0x26/2];
		if (rawx <= centrex)
		{
			rawx = centrex - rawx;
			screenx = rawx * taitoz_sharedram[0x2e/2] +
				(((rawx * taitoz_sharedram[0x30/2]) &0xffff0000) >> 16);
			screenx = 0xa0 - screenx;
			if (screenx < 0) screenx = 0;
		}
		else
		{
			if (rawx > taitoz_sharedram[0x8/2]) rawx = taitoz_sharedram[0x8/2];
			rawx -= centrex;
			screenx = rawx * taitoz_sharedram[0x36/2] +
				(((rawx * taitoz_sharedram[0x38/2]) &0xffff0000) >> 16);
			screenx += 0xa0;
			if (screenx > 0x140) screenx = 0x140;
		}
		rawy = taitoz_sharedram[0xd96/2];
		centrey = taitoz_sharedram[0x28/2];
		if (rawy <= centrey)
		{
			rawy = centrey - rawy;
			screeny = rawy * taitoz_sharedram[0x32/2] +
				(((rawy * taitoz_sharedram[0x34/2]) &0xffff0000) >> 16);
			screeny = 0x78 - screeny;
			if (screeny < 0) screeny = 0;
		}
		else
		{
			if (rawy > taitoz_sharedram[0x10/2]) rawy = taitoz_sharedram[0x10/2];
			rawy -= centrey;
			screeny = rawy * taitoz_sharedram[0x3a/2] +
				(((rawy * taitoz_sharedram[0x3c/2]) &0xffff0000) >> 16);
			screeny += 0x78;
			if (screeny > 0xf0) screeny = 0xf0;
		}

		/* fudge x and y to show in centre of scope, note that screenx, screeny
		   are confirmed to match those stored by the game at $317540, $317542 */
		--screenx;
		screeny += 15;

		draw_crosshair(1, bitmap,screenx,screeny,cliprect);

		/* calculate p2 screen co-ords by matching routine at $196EA */
		rawx = taitoz_sharedram[0xd98/2];
		centrex = taitoz_sharedram[0x2a/2];
		if (rawx <= centrex)
		{
			rawx = centrex - rawx;
			screenx = rawx * taitoz_sharedram[0x3e/2] +
				(((rawx * taitoz_sharedram[0x40/2]) &0xffff0000) >> 16);
			screenx = 0xa0 - screenx;
			if (screenx < 0) screenx = 0;
		}
		else
		{
			if (rawx > taitoz_sharedram[0x18/2]) rawx = taitoz_sharedram[0x18/2];
			rawx -= centrex;
			screenx = rawx * taitoz_sharedram[0x46/2] +
				(((rawx * taitoz_sharedram[0x48/2]) &0xffff0000) >> 16);
			screenx += 0xa0;
			if (screenx > 0x140) screenx = 0x140;
		}
		rawy = taitoz_sharedram[0xd9a/2];
		centrey = taitoz_sharedram[0x2c/2];
		if (rawy <= centrey)
		{
			rawy = centrey - rawy;
			screeny = rawy * taitoz_sharedram[0x42/2] +
				(((rawy * taitoz_sharedram[0x44/2]) &0xffff0000) >> 16);
			screeny = 0x78 - screeny;
			if (screeny < 0) screeny = 0;
		}
		else
		{
			if (rawy > taitoz_sharedram[0x20/2]) rawy = taitoz_sharedram[0x20/2];
			rawy -= centrey;
			screeny = rawy * taitoz_sharedram[0x4a/2] +
				(((rawy * taitoz_sharedram[0x4c/2]) &0xffff0000) >> 16);
			screeny += 0x78;
			if (screeny > 0xf0) screeny = 0xf0;
		}

		/* fudge x and y to show in centre of scope, note that screenx, screeny
		   are confirmed to match those stored by the game at $317544, $317546 */
		--screenx;
		screeny += 15;

		draw_crosshair(2, bitmap,screenx,screeny,cliprect);
	}
}


VIDEO_UPDATE( dblaxle )
{
	UINT8 layer[5];
	UINT16 priority;

	TC0480SCP_tilemap_update();

	priority = TC0480SCP_get_bg_priority();

	layer[0] = (priority &0xf000) >> 12;	/* tells us which bg layer is bottom */
	layer[1] = (priority &0x0f00) >>  8;
	layer[2] = (priority &0x00f0) >>  4;
	layer[3] = (priority &0x000f) >>  0;	/* tells us which is top */
	layer[4] = 4;   /* text layer always over bg layers */

	fillbitmap(priority_bitmap,0,cliprect);

	/* Ensure screen blanked - this shouldn't be necessary! */
	fillbitmap(bitmap, Machine->pens[0], cliprect);

	TC0480SCP_tilemap_draw(bitmap,cliprect,layer[0],TILEMAP_IGNORE_TRANSPARENCY,0);
	TC0480SCP_tilemap_draw(bitmap,cliprect,layer[1],0,0);
	TC0480SCP_tilemap_draw(bitmap,cliprect,layer[2],0,1);

	TC0150ROD_draw(bitmap,cliprect,-1,0xc0,0,0,2);
	bshark_draw_sprites_16x8(bitmap,cliprect,7);

	/* This layer used for the big numeric displays */
	TC0480SCP_tilemap_draw(bitmap,cliprect,layer[3],0,4);

	TC0480SCP_tilemap_draw(bitmap,cliprect,layer[4],0,0);	/* Text layer */
}

VIDEO_UPDATE( racingb )
{
	UINT8 layer[5];
	UINT16 priority;

	TC0480SCP_tilemap_update();

	priority = TC0480SCP_get_bg_priority();

	layer[0] = (priority & 0xf000) >> 12;	/* tells us which bg layer is bottom */
	layer[1] = (priority & 0x0f00) >>  8;
	layer[2] = (priority & 0x00f0) >>  4;
	layer[3] = (priority & 0x000f) >>  0;	/* tells us which is top */
	layer[4] = 4;   /* text layer always over bg layers */

	fillbitmap(priority_bitmap,0,cliprect);

	/* Ensure screen blanked - this shouldn't be necessary! */
	fillbitmap(bitmap, Machine->pens[0], cliprect);

	TC0480SCP_tilemap_draw(bitmap,cliprect,layer[0],TILEMAP_IGNORE_TRANSPARENCY,0);
	TC0480SCP_tilemap_draw(bitmap,cliprect,layer[1],0,0);
	TC0480SCP_tilemap_draw(bitmap,cliprect,layer[2],0,2);
	TC0480SCP_tilemap_draw(bitmap,cliprect,layer[3],0,2);

	TC0150ROD_draw(bitmap,cliprect,-1,0xc0,0,0,2);
	sci_draw_sprites_16x8(bitmap, cliprect, 7);

	TC0480SCP_tilemap_draw(bitmap,cliprect,layer[4],0,4);
}
