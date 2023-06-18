/***************************************************************************

  vidhrdw.c

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include <math.h>

data16_t *nemesis_videoram1b;
data16_t *nemesis_videoram2b;
data16_t *nemesis_videoram1f;
data16_t *nemesis_videoram2f;

data16_t *nemesis_characterram;
size_t nemesis_characterram_size;
data16_t *nemesis_xscroll1,*nemesis_xscroll2,*nemesis_yscroll;
data16_t *nemesis_yscroll1,*nemesis_yscroll2;

static int spriteram_words;
static int tilemap_flip;
static int flipscreen;

static struct tilemap *background, *foreground;

static unsigned char *blank_characterdata; /* pseudo character */

/* gfxram dirty flags */
static unsigned char *char_dirty;	/* 2048 chars */

/* we should be able to draw sprite gfx directly without caching to GfxElements */
static unsigned char *sprite_dirty;	/* 512 sprites */
static unsigned char *sprite3216_dirty;	/* 256 sprites */
static unsigned char *sprite816_dirty;	/* 1024 sprites */
static unsigned char *sprite1632_dirty;	/* 256 sprites */
static unsigned char *sprite3232_dirty;	/* 128 sprites */
static unsigned char *sprite168_dirty;	/* 1024 sprites */
static unsigned char *sprite6464_dirty;	/* 32 sprites */

static void get_bg_tile_info( int offs )
{
	int code,color,flags;
	code = nemesis_videoram1f[offs];
	color = nemesis_videoram2f[offs];
	flags = 0;
	if ( color & 0x80)  flags |= TILE_FLIPX;
	if ( code & 0x0800) flags |= TILE_FLIPY;
	if ((~code & 0x2000) || ((code & 0xc000) == 0x4000))
		 flags |= TILE_IGNORE_TRANSPARENCY;
	if (code & 0xf800) {
		SET_TILE_INFO( 0, code&0x7ff, color&0x7f, flags );
	} else {
		SET_TILE_INFO( 0, 0x800, 0x00, 0 );
	}
	tile_info.priority = (code & 0x1000)>>12;
}

static void get_fg_tile_info( int offs )
{
	int code,color,flags;
	code = nemesis_videoram1b[offs];
	color = nemesis_videoram2b[offs];
	flags = 0;
	if ( color & 0x80)  flags |= TILE_FLIPX;
	if ( code & 0x0800) flags |= TILE_FLIPY;
	if ((~code & 0x2000) || ((code & 0xc000) == 0x4000))
		 flags |= TILE_IGNORE_TRANSPARENCY;
	if (code & 0xf800) {
		SET_TILE_INFO( 0, code&0x7ff, color&0x7f, flags );
	} else {
		SET_TILE_INFO( 0, 0x800, 0x00, 0 );
	}
	tile_info.priority = (code & 0x1000)>>12;
}

WRITE16_HANDLER( nemesis_gfx_flipx_w )
{
	if (ACCESSING_LSB)
	{
		flipscreen = data & 0x01;
		if (flipscreen)
			tilemap_flip |= TILEMAP_FLIPX;
		else
			tilemap_flip &= ~TILEMAP_FLIPX;

		tilemap_set_flip(ALL_TILEMAPS, tilemap_flip);
	}
}

WRITE16_HANDLER( nemesis_gfx_flipy_w )
{
	if (ACCESSING_LSB)
	{
		if (data & 0x01)
			tilemap_flip |= TILEMAP_FLIPY;
		else
			tilemap_flip &= ~TILEMAP_FLIPY;

		tilemap_set_flip(ALL_TILEMAPS, tilemap_flip);
	}
}

WRITE16_HANDLER( nemesis_palette_word_w )
{
	int r,g,b,bit1,bit2,bit3,bit4,bit5;

	COMBINE_DATA(paletteram16 + offset);
	data = paletteram16[offset];

	/* Mish, 30/11/99 - Schematics show the resistor values are:
		300 Ohms
		620 Ohms
		1200 Ohms
		2400 Ohms
		4700 Ohms

		So the correct weights per bit are 8, 17, 33, 67, 130
	*/

	#define MULTIPLIER 8 * bit1 + 17 * bit2 + 33 * bit3 + 67 * bit4 + 130 * bit5

	bit1=(data >>  0)&1;
	bit2=(data >>  1)&1;
	bit3=(data >>  2)&1;
	bit4=(data >>  3)&1;
	bit5=(data >>  4)&1;
	r = MULTIPLIER;
	r = pow (r/255.0, 2)*255;
	bit1=(data >>  5)&1;
	bit2=(data >>  6)&1;
	bit3=(data >>  7)&1;
	bit4=(data >>  8)&1;
	bit5=(data >>  9)&1;
	g = MULTIPLIER;
	g = pow (g/255.0, 2)*255;
	bit1=(data >>  10)&1;
	bit2=(data >>  11)&1;
	bit3=(data >>  12)&1;
	bit4=(data >>  13)&1;
	bit5=(data >>  14)&1;
	b = MULTIPLIER;
	b = pow (b/255.0, 2)*255;

	palette_set_color(offset,r,g,b);
}

WRITE16_HANDLER( salamander_palette_word_w )
{
	int r,g,b;

	COMBINE_DATA(paletteram16 + offset);
	offset &= ~1;

	data = ((paletteram16[offset] << 8) & 0xff00) | (paletteram16[offset+1] & 0xff);

	r = (data >>  0) & 0x1f;
	g = (data >>  5) & 0x1f;
	b = (data >> 10) & 0x1f;

	r = (r << 3) | (r >> 2);
	g = (g << 3) | (g >> 2);
	b = (b << 3) | (b >> 2);

	palette_set_color(offset / 2,r,g,b);
}

READ16_HANDLER( nemesis_videoram1b_word_r )
{
	return nemesis_videoram1b[offset];
}
READ16_HANDLER( nemesis_videoram1f_word_r )
{
	return nemesis_videoram1f[offset];
}

WRITE16_HANDLER( nemesis_videoram1b_word_w )
{
	COMBINE_DATA(nemesis_videoram1b + offset);
	tilemap_mark_tile_dirty( foreground,offset );
}
WRITE16_HANDLER( nemesis_videoram1f_word_w )
{
	COMBINE_DATA(nemesis_videoram1f + offset);
	tilemap_mark_tile_dirty( background,offset );
}

READ16_HANDLER( nemesis_videoram2b_word_r )
{
	return nemesis_videoram2b[offset];
}
READ16_HANDLER( nemesis_videoram2f_word_r )
{
	return nemesis_videoram2f[offset];
}

WRITE16_HANDLER( nemesis_videoram2b_word_w )
{
	COMBINE_DATA(nemesis_videoram2b + offset);
	tilemap_mark_tile_dirty( foreground,offset );
}
WRITE16_HANDLER( nemesis_videoram2f_word_w )
{
	COMBINE_DATA(nemesis_videoram2f + offset);
	tilemap_mark_tile_dirty( background,offset );
}


READ16_HANDLER( gx400_xscroll1_word_r ) { return nemesis_xscroll1[offset];}
READ16_HANDLER( gx400_xscroll2_word_r ) { return nemesis_xscroll2[offset];}
READ16_HANDLER( gx400_yscroll_word_r ) { return nemesis_yscroll[offset];}
READ16_HANDLER( gx400_yscroll1_word_r ) { return nemesis_yscroll1[offset];}
READ16_HANDLER( gx400_yscroll2_word_r ) { return nemesis_yscroll2[offset];}

WRITE16_HANDLER( gx400_xscroll1_word_w ) { COMBINE_DATA(nemesis_xscroll1 + offset);}
WRITE16_HANDLER( gx400_xscroll2_word_w ) { COMBINE_DATA(nemesis_xscroll2 + offset);}
WRITE16_HANDLER( gx400_yscroll_word_w ) { COMBINE_DATA(nemesis_yscroll + offset);}
WRITE16_HANDLER( gx400_yscroll1_word_w ) { COMBINE_DATA(nemesis_yscroll1 + offset);}
WRITE16_HANDLER( gx400_yscroll2_word_w ) { COMBINE_DATA(nemesis_yscroll2 + offset);}


/* we have to straighten out the 16-bit word into bytes for gfxdecode() to work */
READ16_HANDLER( nemesis_characterram_word_r )
{
	return nemesis_characterram[offset];
}

WRITE16_HANDLER( nemesis_characterram_word_w )
{
	data16_t oldword = nemesis_characterram[offset];
	COMBINE_DATA(nemesis_characterram + offset);
	data = nemesis_characterram[offset];

	if (oldword != data)
	{
		char_dirty[offset / 16] = 1;
		sprite_dirty[offset / 64] = 1;
		sprite3216_dirty[offset / 128] = 1;
		sprite1632_dirty[offset / 128] = 1;
		sprite3232_dirty[offset / 256] = 1;
		sprite168_dirty[offset / 32] = 1;
		sprite816_dirty[offset / 32] = 1;
		sprite6464_dirty[offset / 1024] = 1;
	}
}


/* Fix the gdx decode info for lsb first systems */
static void nemesis_lsbify_gfx(void)
{
	int i, j;
	for(i=0; i<8; i++)
		for(j=0; j<Machine->drv->gfxdecodeinfo[i].gfxlayout->width; j++)
			Machine->drv->gfxdecodeinfo[i].gfxlayout->xoffset[j] ^= 8;
}


/* claim a palette dirty array */
VIDEO_START( nemesis )
{
#ifndef MSB_FIRST
	nemesis_lsbify_gfx();
#endif

	spriteram_words = spriteram_size / 2;

	background = tilemap_create(
		get_bg_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8,8, 64,32 );

	foreground = tilemap_create(
		get_fg_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8,8, 64,32 );

	if( !(background && foreground) )
		return 1;

	tilemap_set_transparent_pen( background, 0 );
	tilemap_set_transparent_pen( foreground, 0 );
	tilemap_set_scroll_rows( background, 256 );
	tilemap_set_scroll_rows( foreground, 256 );

	char_dirty = auto_malloc(2048);
	if (!char_dirty)
		return 1;
	memset(char_dirty,1,2048);

	sprite_dirty = auto_malloc(512);
	if (!sprite_dirty)
		return 1;
	memset(sprite_dirty,1,512);

	sprite3216_dirty = auto_malloc(256);
	if (!sprite3216_dirty)
		return 1;
	memset(sprite3216_dirty,1,256);

	sprite1632_dirty = auto_malloc(256);
	if (!sprite1632_dirty)
		return 1;
	memset(sprite1632_dirty,1,256);

	sprite3232_dirty = auto_malloc(128);
	if (!sprite3232_dirty)
		return 1;
	memset(sprite3232_dirty,1,128);

	sprite168_dirty = auto_malloc(1024);
	if (!sprite168_dirty)
		return 1;
	memset(sprite168_dirty,1,1024);

	sprite816_dirty = auto_malloc(1024);
	if (!sprite816_dirty)
		return 1;
	memset(sprite816_dirty,1,1024);

	sprite6464_dirty = auto_malloc(32);
	if (!sprite6464_dirty)
		return 1;
	memset(sprite6464_dirty,1,32);

	memset(nemesis_characterram,0,nemesis_characterram_size);


	blank_characterdata = auto_malloc(32*8/8*(2048+1));
	if (!blank_characterdata)
		return 1;
	memset(blank_characterdata,0x00,32*8/8*(2048+1));
	decodechar(Machine->gfx[0],0x800,(unsigned char *)blank_characterdata,
					Machine->drv->gfxdecodeinfo[0].gfxlayout);

	flipscreen = 0;
	tilemap_flip = 0;

	return 0;
}

static void draw_sprites(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	/*
	 *	16 bytes per sprite, in memory from 56000-56fff
	 *
	 *	byte	0 :	relative priority.
	 *	byte	2 :	size (?) value #E0 means not used., bit 0x01 is flipx
	                0xc0 is upper 2 bits of zoom.
					0x38 is size.
	 * 	byte	4 :	zoom = 0xff
	 *	byte	6 :	low bits sprite code.
	 *	byte	8 :	color + hi bits sprite code., bit 0x20 is flipy bit. bit 0x01 is high bit of X pos.
	 *	byte	A :	X position.
	 *	byte	C :	Y position.
	 * 	byte	E :	not used.
	 */

	int adress;	/* start of sprite in spriteram */
	int sx;	/* sprite X-pos */
	int sy;	/* sprite Y-pos */
	int code;	/* start of sprite in obj RAM */
	int color;	/* color of the sprite */
	int flipx,flipy;
	int zoom;
	int char_type;
	int priority;
	int size;
	int w,h;

	for (priority=256-1; priority>=0; priority--)
	{
		for (adress = spriteram_words-8; adress >= 0; adress -= 8)
		{
			if((spriteram16[adress] & 0xff)!=priority) continue;

			zoom = spriteram16[adress+2] & 0xff;
			if (!(spriteram16[adress+2] & 0xff00) && ((spriteram16[adress+3] & 0xff00) != 0xff00))
				code = spriteram16[adress+3] + ((spriteram16[adress+4] & 0xc0) << 2);
			else
				code = (spriteram16[adress+3] & 0xff) + ((spriteram16[adress+4] & 0xc0) << 2);

			if (zoom != 0xFF || code!=0)
			{
				size=spriteram16[adress+1];
				zoom+=(size&0xc0)<<2;

				sx = spriteram16[adress+5]&0xff;
				sy = spriteram16[adress+6]&0xff;
				if(spriteram16[adress+4]&1)
					sx-=0x100;	/* fixes left side clip */
				color = (spriteram16[adress+4] & 0x1e) >> 1;
				flipx = spriteram16[adress+1] & 0x01;
				flipy = spriteram16[adress+4] & 0x20;

				switch(size&0x38)
				{
					case 0x00:	/* sprite 32x32*/
						char_type=4;
						code/=8;
						w=32;
						h=32;
						break;
					case 0x08:	/* sprite 16x32 */
						char_type=5;
						code/=4;
						w=16;
						h=32;
						break;
					case 0x10:	/* sprite 32x16 */
						char_type=2;
						code/=4;
						w=32;
						h=16;
						break;
					case 0x18:		/* sprite 64x64 */
						char_type=7;
						code/=32;
						w=64;
						h=64;
						break;
					case 0x20:	/* char 8x8 */
						char_type=0;
						code*=2;
						w=8;
						h=8;
						break;
					case 0x28:		/* sprite 16x8 */
						char_type=6;
						w=16;
						h=8;
						break;
					case 0x30:	/* sprite 8x16 */
						char_type=3;
						w=8;
						h=16;
						break;
					case 0x38:
					default:	/* sprite 16x16 */
						char_type=1;
						code/=2;
						w=16;
						h=16;
						break;
				}

				if( zoom )
				{
					zoom = ((1<<16)*0x80/zoom) + 0x02ab;
					if (flipscreen)
					{
						sx = 256 - ((zoom * w) >> 16) - sx;
						sy = 256 - ((zoom * h) >> 16) - sy;
						flipx = !flipx;
						flipy = !flipy;
					}
					pdrawgfxzoom(bitmap,Machine->gfx[char_type],
						code,
						color,
						flipx,flipy,
						sx,sy,
						cliprect,TRANSPARENCY_PEN,0,
						zoom,zoom,
						0xfff0 );
				}
			} /* if sprite */
		} /* for loop */
	} /* priority */
}

/******************************************************************************/

static void update_gfx(void)
{
	int offs,code;
	int bAnyDirty;

	bAnyDirty = 0;
	for (offs = 0x800 - 1;offs >= 0;offs --)
	{
		if (char_dirty[offs] )
		{
			decodechar(Machine->gfx[0],offs,(unsigned char *)nemesis_characterram,
					Machine->drv->gfxdecodeinfo[0].gfxlayout);
			bAnyDirty = 1;
			char_dirty[offs] = 0;
		}
	}
	if( bAnyDirty )
	{
		tilemap_mark_all_tiles_dirty( background );
		tilemap_mark_all_tiles_dirty( foreground );
	}

	for (offs = 0;offs < spriteram_words;offs += 8)
	{
		int char_type;
		int zoom;

		zoom = spriteram16[offs+2] & 0xff;
		if (!(spriteram16[offs+2] & 0xff00) && ((spriteram16[offs+3] & 0xff00) != 0xff00))
			code = spriteram16[offs+3] + ((spriteram16[offs+4] & 0xc0) << 2);
		else
			code = (spriteram16[offs+3] & 0xff) + ((spriteram16[offs+4] & 0xc0) << 2);

		if (zoom != 0xFF || code!=0)
		{
			int size = spriteram16[offs+1];
			switch(size&0x38)
			{
				case 0x00:
					/* sprite 32x32*/
					char_type=4;
					code/=8;
					if (sprite3232_dirty[code] == 1)
					{
						decodechar(Machine->gfx[char_type],code,(unsigned char *)nemesis_characterram,
								Machine->drv->gfxdecodeinfo[char_type].gfxlayout);
						sprite3232_dirty[code] = 0;
					}
					break;
				case 0x08:
					/* sprite 16x32 */
					char_type=5;
					code/=4;
					if (sprite1632_dirty[code] == 1)
					{
						decodechar(Machine->gfx[char_type],code,(unsigned char *)nemesis_characterram,
								Machine->drv->gfxdecodeinfo[char_type].gfxlayout);
						sprite1632_dirty[code] = 0;

					}
					break;
				case 0x10:
					/* sprite 32x16 */
					char_type=2;
					code/=4;
					if (sprite3216_dirty[code] == 1)
					{
						decodechar(Machine->gfx[char_type],code,(unsigned char *)nemesis_characterram,
								Machine->drv->gfxdecodeinfo[char_type].gfxlayout);
						sprite3216_dirty[code] = 0;
					}
					break;
				case 0x18:
					/* sprite 64x64 */
					char_type=7;
					code/=32;
					if (sprite6464_dirty[code] == 1)
					{
						decodechar(Machine->gfx[char_type],code,(unsigned char *)nemesis_characterram,
								Machine->drv->gfxdecodeinfo[char_type].gfxlayout);
						sprite6464_dirty[code] = 0;
					}
					break;
				case 0x20:
					/* char 8x8 */
					char_type=0;
					code*=2;
					if (char_dirty[code] == 1)
					{
						decodechar(Machine->gfx[char_type],code,(unsigned char *)nemesis_characterram,
						Machine->drv->gfxdecodeinfo[char_type].gfxlayout);
						char_dirty[code] = 0;
					}
					break;
				case 0x28:
					/* sprite 16x8 */
					char_type=6;
					if (sprite168_dirty[code] == 1)
					{
						decodechar(Machine->gfx[char_type],code,(unsigned char *)nemesis_characterram,
								Machine->drv->gfxdecodeinfo[char_type].gfxlayout);
						sprite168_dirty[code] = 0;
					}
					break;
				case 0x30:
					/* sprite 8x16 */
					char_type=3;
					if (sprite816_dirty[code] == 1)
					{
						decodechar(Machine->gfx[char_type],code,(unsigned char *)nemesis_characterram,
								Machine->drv->gfxdecodeinfo[char_type].gfxlayout);
						sprite816_dirty[code] = 0;
					}
					break;
				default:
					log_cb(RETRO_LOG_DEBUG, LOGPRE "UN-SUPPORTED SPRITE SIZE %-4x\n",size&0x38);
				case 0x38:
					/* sprite 16x16 */
					char_type=1;
					code/=2;
					if (sprite_dirty[code] == 1)
					{
						decodechar(Machine->gfx[char_type],code,(unsigned char *)nemesis_characterram,
								Machine->drv->gfxdecodeinfo[char_type].gfxlayout);
						sprite_dirty[code] = 2;

					}
					break;
			}
		}
	}
}

/******************************************************************************/

VIDEO_UPDATE( nemesis )
{
	int offs;

	update_gfx();

	fillbitmap(priority_bitmap,0,cliprect);
	fillbitmap(bitmap,Machine->pens[0],cliprect);

	tilemap_set_scrolly( background, 0, (nemesis_yscroll[0x180] & 0xff) );

	for (offs = 0;offs < 256;offs++)
	{
		tilemap_set_scrollx( background, offs,
			(nemesis_xscroll2[offs] & 0xff) + ((nemesis_xscroll2[0x100 + offs] & 0x01) << 8) - (flipscreen ? 0x107 : 0) );
		tilemap_set_scrollx( foreground, offs,
			(nemesis_xscroll1[offs] & 0xff) + ((nemesis_xscroll1[0x100 + offs] & 0x01) << 8) - (flipscreen ? 0x107 : 0) );
	}

	tilemap_draw(bitmap,cliprect,background,0,1);
	tilemap_draw(bitmap,cliprect,foreground,0,2);
	tilemap_draw(bitmap,cliprect,background,1,4);
	tilemap_draw(bitmap,cliprect,foreground,1,8);

	draw_sprites(bitmap,cliprect);
}

VIDEO_UPDATE( salamand )
{
	int offs;
	struct rectangle clip;

	update_gfx();

	fillbitmap(priority_bitmap,0,cliprect);
	fillbitmap(bitmap,Machine->pens[0],cliprect);

	clip.min_x = 0;
	clip.max_x = 255;

	tilemap_set_scroll_cols( background, 64 );
	tilemap_set_scroll_cols( foreground, 64 );
	tilemap_set_scroll_rows( background, 1 );
	tilemap_set_scroll_rows( foreground, 1 );

	for (offs = 0; offs < 64; offs++)
	{
		tilemap_set_scrolly( background, offs, nemesis_yscroll1[offs] );
		tilemap_set_scrolly( foreground, offs, nemesis_yscroll2[offs] );
	}

	for (offs = cliprect->min_y; offs <= cliprect->max_y; offs++)
	{
		clip.min_y = offs;
		clip.max_y = offs;

		tilemap_set_scrollx( background, 0,
			((nemesis_xscroll2[offs] & 0xff) + ((nemesis_xscroll2[0x100 + offs] & 1) << 8)) );
		tilemap_set_scrollx( foreground, 0,
			((nemesis_xscroll1[offs] & 0xff) + ((nemesis_xscroll1[0x100 + offs] & 1) << 8)) );

		tilemap_draw(bitmap,&clip,foreground,0,1);
		tilemap_draw(bitmap,&clip,background,0,2);
		tilemap_draw(bitmap,&clip,foreground,1,4);
		tilemap_draw(bitmap,&clip,background,1,8);
	}

	draw_sprites(bitmap,cliprect);
}
