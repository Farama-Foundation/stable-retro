/***************************************************************************

Sega System 16 Video Hardware

Known issues:
- better abstraction of tilemap hardware is needed; a lot of this mess could and should
	be consolidated further
- many games have ROM patches - why?  let's emulate protection when possible
- several games fail their RAM self-test because of hacks in the drivers
- many registers are suspiciously plucked from working RAM
- several games have nvram according to the self test, but we aren't yet saving it
- many games suffer from sys16_refreshenable register not being mapped
- road-rendering routines need to be cleaned up or at least better described
- logical sprite height computation isn't quite right - garbage pixels are drawn
- screen orientation support for sprite drawing
- sprite drawing is unoptimized
- end-of-sprite marker support will fix some glitches
- shadow and partial shadow sprite support
- to achieve sprite-tilemap orthogonality we must draw sprites from front to back;
	this will also allow us to avoid processing the same shadowed pixel twice.

The System16 video hardware consists of:

- Road Layer (present only for certain games)

- Two scrolling tilemap layers

- Two alternate scrolling tilemap layers, normally invisible.

- A fixed tilemap layer, normally used for score, lives display

Each scrolling layer (foreground, background) is an arrangement
of 4 pages selected from 16 available pages, laid out as follows:

	Page0  Page1
	Page2  Page3

Each page is an arrangement of 8x8 tiles, 64 tiles wide, and 32 tiles high.

Layers normally scroll as a whole, with xscroll and yscroll.

However, layers can also be configured with screenwise rowscroll, if the most significant
bit of xscroll is set.  When a layer is in this mode, the splittable is used.

When rowscroll is in effect, the most significant bit of rowscroll selects between the
default layer and an alternate layer associated with it.

The foreground layer's tiles may be flagged as high-priority; this is used to mask
sprites, i.e. the grass in Altered Beast.

The background layer's tiles may also be flagged as high-priority.  In this case,
it's really a transparency_pen effect rather.  Aurail uses this.

Most games map Video Registers in textram as follows:

type0:
most games

type1:
alexkidd,fantzone,shinobl,hangon
mjleague

others:
	shangon, shdancbl,
	dduxbl,eswat,
	passsht,passht4b
	quartet,quartet2,
	tetris,tturfbl,wb3bl

sys16_textram:
type1		type0			function
---------------------------------------
0x74f		0x740			sys16_fg_page
0x74e		0x741			sys16_bg_page
			0x742			sys16_fg2_page
			0x743			sys16_bg2_page

0x792		0x748			sys16_fg_scrolly
0x793		0x749			sys16_bg_scrolly
			0x74a			sys16_fg2_scrolly
			0x74b			sys16_bg2_scrolly

0x7fc		0x74c			sys16_fg_scrollx
0x7fd		0x74d			sys16_bg_scrollx
			0x74e			sys16_fg2_scrollx
			0x74f			sys16_bg2_scrollx

			0x7c0..0x7df	sys18_splittab_fg_x
			0x7e0..0x7ff	sys18_splittab_bg_x

***************************************************************************/
#include "driver.h"
#include "system16.h"
#include "vidhrdw/res_net.h"

/*
static void debug_draw( struct mame_bitmap *bitmap, int x, int y, unsigned int data ){
	int digit;
	for( digit=0; digit<4; digit++ ){
		drawgfx( bitmap, Machine->uifont,
			"0123456789abcdef"[data>>12],
			0,
			0,0,
			x+digit*6,y,
			&Machine->visible_area,TRANSPARENCY_NONE,0);
		data = (data<<4)&0xffff;
	}
}

static void debug_vreg( struct mame_bitmap *bitmap ){
	int g = 0x740;
	int i;

	if( keyboard_pressed( KEYCODE_Q ) ) g+=0x10;
	if( keyboard_pressed( KEYCODE_W ) ) g+=0x20;
	if( keyboard_pressed( KEYCODE_E ) ) g+=0x40;
	if( keyboard_pressed( KEYCODE_R ) ) g+=0x80;

	for( i=0; i<16; i++ ){
		debug_draw( bitmap, 8,8*i,sys16_textram[g+i] );
	}
}
*/

/* callback to poll video registers */
void (* sys16_update_proc)( void );

data16_t *sys16_tileram;
data16_t *sys16_textram;
data16_t *sys16_spriteram;
data16_t *sys16_roadram;

static int num_sprites;

#define MAXCOLOURS 0x2000 /* 8192 */
int sys16_sprite_draw;
int sys16_wwfix;
int sys16_sh_shadowpal;
int sys16_MaxShadowColors;

/* video driver constants (potentially different for each game) */
int sys16_gr_bitmap_width;
int (*sys16_spritesystem)( struct sys16_sprite_attributes *sprite, const UINT16 *source, int bJustGetColor );
int *sys16_obj_bank;
int sys16_sprxoffset;
int sys16_bgxoffset;
int sys16_fgxoffset;
int sys16_textmode;
int sys16_textlayer_lo_min;
int sys16_textlayer_lo_max;
int sys16_textlayer_hi_min;
int sys16_textlayer_hi_max;
int sys16_bg1_trans; /* alien syn + sys18 */
int sys16_bg_priority_mode;
int sys16_fg_priority_mode;
int sys16_bg_priority_value;
int sys16_fg_priority_value;
int sys16_18_mode;
int sys16_tilebank_switch;
int sys16_rowscroll_scroll;
int sys16_quartet_title_kludge;

/* video registers */
int sys16_tile_bank1;
int sys16_tile_bank0;
int sys16_refreshenable;

int sys16_bg_scrollx, sys16_bg_scrolly;
int sys16_bg2_scrollx, sys16_bg2_scrolly;
int sys16_fg_scrollx, sys16_fg_scrolly;
int sys16_fg2_scrollx, sys16_fg2_scrolly;

int sys16_bg_page[4];
int sys16_bg2_page[4];
int sys16_fg_page[4];
int sys16_fg2_page[4];

int sys18_bg2_active;
int sys18_fg2_active;
data16_t *sys18_splittab_bg_x;
data16_t *sys18_splittab_bg_y;
data16_t *sys18_splittab_fg_x;
data16_t *sys18_splittab_fg_y;

data16_t *sys16_gr_ver;
data16_t *sys16_gr_hor;
data16_t *sys16_gr_pal;
data16_t *sys16_gr_flip;
int sys16_gr_palette;
int sys16_gr_palette_default;
unsigned char sys16_gr_colorflip[2][4];
data16_t *sys16_gr_second_road;

static struct tilemap *background, *foreground, *text_layer;
static struct tilemap *background2, *foreground2;
static int old_bg_page[4],old_fg_page[4], old_tile_bank1, old_tile_bank0;
static int old_bg2_page[4],old_fg2_page[4];

/***************************************************************************/

READ16_HANDLER( sys16_textram_r ){
	return sys16_textram[offset];
}

READ16_HANDLER( sys16_tileram_r ){
	return sys16_tileram[offset];
}

/***************************************************************************/

/*
	We mark the priority buffer as follows:
		text	(0xf)
		fg (hi) (0x7)
		fg (lo) (0x3)
		bg (hi) (0x1)
		bg (lo) (0x0)

	Each sprite has 4 levels of priority, specifying where they are placed between bg(lo) and text.
*/


#define draw_pixel() 														\
	/* only draw if onscreen, not 0 or 15, and high enough priority */		\
	if (x >= cliprect->min_x && pix != 0 && pix != 15 && sprpri > pri[x])	\
	{																		\
		/* shadow/hilight mode? */											\
		if (color == 1024 + (0x3f << 4))									\
			dest[x] += (paletteram16[dest[x]] & 0x8000) ? 4096 : 2048;		\
																			\
		/* regular draw */													\
		else																\
			dest[x] = pix | color;											\
																			\
		/* always mark priority so no one else draws here */				\
		pri[x] = 0xff;														\
	}																		\


static void draw_one_sprite_new(struct mame_bitmap *bitmap, const struct rectangle *cliprect, UINT16 *data)
{
	int bottom  = data[0] >> 8;
	int top     = data[0] & 0xff;
	int xpos    = (data[1] & 0x1ff) + sys16_sprxoffset;
	int hide    = data[2] & 0x4000;
	int flip    = data[2] & 0x100;
	int pitch   = (INT8)(data[2] & 0xff);
	UINT16 addr = data[3];
	int bank    = sys16_obj_bank[(data[4] >> 8) & 0xf];
	int sprpri  = 1 << ((data[4] >> 6) & 0x3);
	int color   = 1024 + ((data[4] & 0x3f) << 4);
	int vzoom   = (data[5] >> 5) & 0x1f;
	int hzoom   = data[5] & 0x1f;
	int x, y, pix, numbanks;
	UINT16 *spritedata;

	/* initialize the end address to the start address */
	data[7] = addr;

	/* if hidden, or top greater than/equal to bottom, or invalid bank, punt */
	if (hide || (top >= bottom) || bank == 255)
		return;

	/* clamp to within the memory region size */
	numbanks = memory_region_length(REGION_GFX2) / 0x20000;
	if (numbanks)
		bank %= numbanks;
	spritedata = (UINT16 *)memory_region(REGION_GFX2) + 0x10000 * bank;

	/* reset the yzoom counter */
	data[5] &= 0x03ff;

	/* for the non-flipped case, we start one row ahead */
	if (!flip)
		addr += pitch;

	/* loop from top to bottom */
	for (y = top; y < bottom; y++)
	{
		/* skip drawing if not within the cliprect */
		if (y >= cliprect->min_y && y <= cliprect->max_y)
		{
			UINT16 *dest = (UINT16 *)bitmap->line[y];
			UINT8 *pri = (UINT8 *)priority_bitmap->line[y];
			int xacc = 0x20;

			/* non-flipped case */
			if (!flip)
			{
				/* start at the word before because we preincrement below */
				data[7] = addr - 1;
				for (x = xpos; x <= cliprect->max_x; )
				{
					UINT16 pixels = spritedata[++data[7]];

					/* draw four pixels */
					pix = (pixels >> 12) & 0xf; if (xacc < 0x40) { draw_pixel(); x++; } else xacc -= 0x40; xacc += hzoom;
					pix = (pixels >>  8) & 0xf; if (xacc < 0x40) { draw_pixel(); x++; } else xacc -= 0x40; xacc += hzoom;
					pix = (pixels >>  4) & 0xf; if (xacc < 0x40) { draw_pixel(); x++; } else xacc -= 0x40; xacc += hzoom;
					pix = (pixels >>  0) & 0xf; if (xacc < 0x40) { draw_pixel(); x++; } else xacc -= 0x40; xacc += hzoom;

					/* stop if the last pixel in the group was 0xf */
					if (pix == 15)
						break;
				}
			}

			/* flipped case */
			else
			{
				/* start at the word after because we predecrement below */
				data[7] = addr + pitch + 1;
				for (x = xpos; x <= cliprect->max_x; )
				{
					UINT16 pixels = spritedata[--data[7]];

					/* draw four pixels */
					pix = (pixels >>  0) & 0xf; if (xacc < 0x40) { draw_pixel(); x++; } else xacc -= 0x40; xacc += hzoom;
					pix = (pixels >>  4) & 0xf; if (xacc < 0x40) { draw_pixel(); x++; } else xacc -= 0x40; xacc += hzoom;
					pix = (pixels >>  8) & 0xf; if (xacc < 0x40) { draw_pixel(); x++; } else xacc -= 0x40; xacc += hzoom;
					pix = (pixels >> 12) & 0xf; if (xacc < 0x40) { draw_pixel(); x++; } else xacc -= 0x40; xacc += hzoom;

					/* stop if the last pixel in the group was 0xf */
					if (pix == 15)
						break;
				}
			}
		}

		/* advance a row */
		addr += pitch;

		/* accumulate zoom factors; if we carry into the high bit, skip an extra row */
		data[5] += vzoom << 10;
		if (data[5] & 0x8000)
		{
			addr += pitch;
			data[5] &= ~0x8000;
		}
	}
}



/*************************************
 *
 *	Sprite drawing
 *
 *************************************/

static void draw_sprites_new(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	UINT16 *cursprite;

	/* first scan forward to find the end of the list */
	for (cursprite = sys16_spriteram; cursprite < sys16_spriteram + 0x7ff/2; cursprite += 8)
		if (cursprite[2] & 0x8000)
			break;

	/* now scan backwards and render the sprites in order */
	for (cursprite -= 8; cursprite >= sys16_spriteram; cursprite -= 8)
		draw_one_sprite_new(bitmap, cliprect, cursprite);
}



static void draw_sprite( 
	struct mame_bitmap *bitmap,
	const struct rectangle *cliprect,
	const unsigned char *addr, int pitch,
	const pen_t *paldata,
	int x0, int y0, int screen_width, int screen_height,
	int width, int height,
	int flipx, int flipy,
	int priority,
	int shadow,
	int shadow_pen, int eos )
{
	const pen_t *shadow_base = Machine->gfx[0]->colortable + (Machine->drv->total_colors/2);
	const UINT8 *source;
	int full_shadow=shadow&SYS16_SPR_SHADOW;
	int partial_shadow=shadow&SYS16_SPR_PARTIAL_SHADOW;
	int shadow_mask=(Machine->drv->total_colors/2)-1;
	int sx, x, xcount;
	int sy, y, ycount = 0;
	int dx,dy;
	UINT16 *dest;
	UINT8 *pri;
	unsigned pen, data;

	priority = 1<<priority;
	if (!strcmp(Machine->gamedrv->name,"sonicbom")) flipy^=0x80; /* temp hack until we fix drawing */

	if( flipy ){
		dy = -1;
		y0 += screen_height-1;
	}
	else {
		dy = 1;
	}

	if( flipx ){
		dx = -1;
		x0 += screen_width-1;
	}
	else {
		dx = 1;
	}

	if (!eos)
	{
		sy = y0;
		for( y=height; y; y-- ){
			ycount += screen_height;
			while( ycount>=height ){
				if( sy>=cliprect->min_y && sy<=cliprect->max_y ){
					source = addr;
					dest = (UINT16 *)bitmap->line[sy];
					pri = priority_bitmap->line[sy];
					sx = x0;
					xcount = 0;
					for( x=width; x; x-=2 ){
						data = (unsigned)*source++; /* next 2 pixels */
						pen = data>>4;
						xcount += screen_width;
						while( xcount>=width )
						{
							if( pen && pen!=0xf && sx>=cliprect->min_x && sx<=cliprect->max_x ){
								if(!(pri[sx]&priority)){
									if (full_shadow)
										dest[sx] = shadow_base[dest[sx]&shadow_mask];
									else if (partial_shadow && pen==shadow_pen)
										dest[sx] = shadow_base[dest[sx]&shadow_mask];
									else
										dest[sx] = paldata[pen];
								}
							}
							xcount -= width;
							sx+=dx;
						}
						pen = data&0xf;
						xcount += screen_width;
						while( xcount>=width )
						{
							if( pen && pen!=0xf && sx>=cliprect->min_x && sx<=cliprect->max_x ){
								if(!(pri[sx]&priority)){
									if (full_shadow)
										dest[sx] = shadow_base[dest[sx]&shadow_mask];
									else if (partial_shadow && pen==shadow_pen)
										dest[sx] = shadow_base[dest[sx]&shadow_mask];
									else
										dest[sx] = paldata[pen];
								}
							}
							xcount -= width;
							sx+=dx;
						}
					}
				}
				ycount -= height;
				sy+=dy;
			}
			addr += pitch;
		}
	}
	else
	{
		sy = y0;
		for( y=height; y; y-- ){
			ycount += screen_height;
			while( ycount>=height ){
				if( sy>=cliprect->min_y && sy<=cliprect->max_y ){
					source = addr;
					dest = (UINT16 *)bitmap->line[sy];
					pri = priority_bitmap->line[sy];
					sx = x0;
					xcount = 0;
					for( x=width; x; x-=2 ){
						data = (unsigned)*source++; /* next 2 pixels */
						pen = data>>4;
						if (pen==0xf) break;
						xcount += screen_width;
						while( xcount>=width )
						{
							if( pen && pen!=0xf && sx>=cliprect->min_x && sx<=cliprect->max_x )
								if(!(pri[sx]&priority)) dest[sx] = paldata[pen];
							xcount -= width;
							sx+=dx;
						}
						pen = data&0xf;
						xcount += screen_width;
						while( xcount>=width )
						{
							if( pen && pen!=0xf && sx>=cliprect->min_x && sx<=cliprect->max_x )
								if(!(pri[sx]&priority)) dest[sx] = paldata[pen];
							xcount -= width;
							sx+=dx;
						}
					}
				}
				ycount -= height;
				sy+=dy;
			}
			addr += pitch;
		}
	}
}

static void draw_sprites( struct mame_bitmap *bitmap, const struct rectangle *cliprect, int b3d ) 
{
	const pen_t *base_pal = Machine->gfx[0]->colortable;
	const unsigned char *base_gfx = memory_region(REGION_GFX2);
	const int gfx_rom_size = memory_region_length(REGION_GFX2);
	const data16_t *source = sys16_spriteram;
	struct sys16_sprite_attributes sprite;
	int xpos, ypos, screen_width, width, logical_height, pitch, flipy, flipx;
	int i, mod_h, mod_x, eos;
	unsigned gfx;

	memset(&sprite, 0x00, sizeof(sprite));

	for(i=0; i<num_sprites; i++)
	{
		sprite.flags = 0;
		if (sys16_spritesystem(&sprite, source, 0)) return; /* end-of-spritelist */
		source += 8;

		if( sprite.flags & SYS16_SPR_VISIBLE )
		{
			xpos = sprite.x;
			flipx = sprite.flags & SYS16_SPR_FLIPX;
			ypos = sprite.y;
			pitch = sprite.pitch;
			flipy = pitch & 0x80;
			width = pitch & 0x7f;
			if (pitch & 0x80) width = 0x80 - width;
			pitch = width << 1;
			width <<= 2;
			eos = 0;

			if( b3d ) /* outrun/aburner */
			{
				if (b3d == 2) eos = 1;
				if (xpos < 0 && flipx) continue;
				if (ypos >= 240) ypos -= 256;
				sprite.screen_height++;
				logical_height = (sprite.screen_height<<4)*sprite.zoomy/0x2000;
				screen_width = width*0x200/sprite.zoomx;

				if (flipx && flipy) { mod_h = -logical_height;   mod_x = 4; }
				else if     (flipx) { mod_h = -1; xpos++;        mod_x = 4; }
				else if     (flipy) { mod_h = -logical_height;   mod_x = 0; }
				else                { mod_h = 0;                 mod_x = 0; }

				if( sprite.flags & SYS16_SPR_DRAW_TO_TOP )
				{
					ypos -= sprite.screen_height;
					flipy = !flipy;
				}

				if( sprite.flags & SYS16_SPR_DRAW_TO_LEFT )
				{
					xpos -= screen_width;
					flipx = !flipx;
				}
			}
			else if( sys16_spritesystem==sys16_sprite_sharrier )
			{
				logical_height = (sprite.screen_height<<4)*(0x400+sprite.zoomy)/0x4000;
				screen_width = width*(0x800-sprite.zoomx)/0x800;

				if (flipx && flipy) { mod_h = -logical_height-1; mod_x = 4; }
				else if     (flipx) { mod_h = 0;                 mod_x = 4; }
				else if     (flipy) { mod_h = -logical_height;   mod_x = 0; }
				else                { mod_h = 1;                 mod_x = 0; }
			}
			else
			{
				if (!width) { width = 512; eos = 1; } /* used by fantasy zone for laser */
				screen_width = width;
				logical_height = sprite.screen_height;

				if (sprite.zoomy) logical_height = logical_height*(0x400 + sprite.zoomy)/0x400 - 1;
				if (sprite.zoomx) screen_width = screen_width*(0x800 - sprite.zoomx)/0x800 + 2;

/* fix, 5-bit zoom field */
/*				if (sprite.zoomy) logical_height = logical_height*(0x20 + sprite.zoomy)/0x20 - 1; */
/*				if (sprite.zoomx) screen_width = screen_width*(0x40 - sprite.zoomx)/0x40 + 2; */

				if (flipx && flipy) { mod_h = -logical_height-1; mod_x = 2; }
				else if     (flipx) { mod_h = 0;                 mod_x = 2; }
				else if     (flipy) { mod_h = -logical_height;   mod_x = 0; }
				else                { mod_h = 1;                 mod_x = 0; }
			}

			gfx = sprite.gfx + pitch * mod_h + mod_x;
			if (gfx >= gfx_rom_size) gfx %= gfx_rom_size;

			draw_sprite(
				bitmap,cliprect,
				base_gfx + gfx, pitch,
				base_pal + (sprite.color<<4),
				xpos, ypos, screen_width, sprite.screen_height,
				width, logical_height,
				flipx, flipy,
				sprite.priority,
				sprite.flags,
				sprite.shadow_pen, eos);
		}
	}
}

/***************************************************************************/

UINT32 sys16_bg_map( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows ){
	int page = 0;
	if( row<32 ){ /* top */
		if( col<64 ) page = 0; else page = 1;
	}
	else { /* bottom */
		if( col<64 ) page = 2; else page = 3;
	}
	row = row%32;
	col = col%64;
	return page*64*32+row*64+col;
}

UINT32 sys16_text_map( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows ){
	return row*64+col+(64-40);
}

/***************************************************************************/

/*
	Color generation details

	Each color is made up of 5 bits, connected through one or more resistors like so:

	Bit 0 = 1 x 3.9K ohm
	Bit 1 = 1 x 2.0K ohm
	Bit 2 = 1 x 1.0K ohm
	Bit 3 = 2 x 1.0K ohm
	Bit 4 = 4 x 1.0K ohm

	Another data bit is connected by a tristate buffer to the color output through a 470 ohm resistor.
	The buffer allows the resistor to have no effect (tristate), halve brightness (pull-down) or double brightness (pull-up).
	The data bit source is a PPI pin in some of the earlier hardware (Hang-On, Pre-System 16) or bit 15 of each
	color RAM entry (Space Harrier, System 16B and most later boards).
*/

const int resistances_normal[6] = {3900, 2000, 1000, 1000/2, 1000/4, 0};
const int resistances_sh[6] = {3900, 2000, 1000, 1000/2, 1000/4, 470};
static double weights[2][3][6];

WRITE16_HANDLER( sys16_paletteram_w )
{
	data16_t oldword = paletteram16[offset];
	data16_t newword;
	COMBINE_DATA( &paletteram16[offset] );
	newword = paletteram16[offset];

	if( oldword!=newword )
	{
		/* we can do this, because we initialize palette RAM to all black in vh_start */
		/*	   byte 0    byte 1 */
		/*	sBGR BBBB GGGG RRRR */
		/*	x000 4321 4321 4321 */

		int r, g, b, rs, gs, bs, rh, gh, bh;
		int r0 = (newword >> 12) & 1;
		int r1 = (newword >>  0) & 1;
		int r2 = (newword >>  1) & 1;
		int r3 = (newword >>  2) & 1;
		int r4 = (newword >>  3) & 1;
		int g0 = (newword >> 13) & 1;
		int g1 = (newword >>  4) & 1;
		int g2 = (newword >>  5) & 1;
		int g3 = (newword >>  6) & 1;
		int g4 = (newword >>  7) & 1;
		int b0 = (newword >> 14) & 1;
		int b1 = (newword >>  8) & 1;
		int b2 = (newword >>  9) & 1;
		int b3 = (newword >> 10) & 1;
		int b4 = (newword >> 11) & 1;

		/* Normal colors */
		r = combine_6_weights(weights[0][0], r0, r1, r2, r3, r4, 0);
		g = combine_6_weights(weights[0][1], g0, g1, g2, g3, g4, 0);
		b = combine_6_weights(weights[0][2], b0, b1, b2, b3, b4, 0);

		/* Shadow colors */
		rs = combine_6_weights(weights[1][0], r0, r1, r2, r3, r4, 0);
		gs = combine_6_weights(weights[1][1], g0, g1, g2, g3, g4, 0);
		bs = combine_6_weights(weights[1][2], b0, b1, b2, b3, b4, 0);

		/* Highlight colors */
		rh = combine_6_weights(weights[1][0], r0, r1, r2, r3, r4, 1);
		gh = combine_6_weights(weights[1][1], g0, g1, g2, g3, g4, 1);
		bh = combine_6_weights(weights[1][2], b0, b1, b2, b3, b4, 1);

		palette_set_color( offset, r, g, b );

#ifdef TRANSPARENT_SHADOWS
		palette_set_color( offset+Machine->drv->total_colors/2,rs,gs,bs);
#endif

	}
}


static void update_page( void ){
	int all_dirty = 0;
	int i,offset;
	if( old_tile_bank1 != sys16_tile_bank1 ){
		all_dirty = 1;
		old_tile_bank1 = sys16_tile_bank1;
	}
	if( old_tile_bank0 != sys16_tile_bank0 ){
		all_dirty = 1;
		old_tile_bank0 = sys16_tile_bank0;
		tilemap_mark_all_tiles_dirty( text_layer );
	}
	if( all_dirty ){
		tilemap_mark_all_tiles_dirty( background );
		tilemap_mark_all_tiles_dirty( foreground );
		if( sys16_18_mode ){
			tilemap_mark_all_tiles_dirty( background2 );
			tilemap_mark_all_tiles_dirty( foreground2 );
		}
	}
	else {
		for(i=0;i<4;i++){
			int page0 = 64*32*i;
			if( old_bg_page[i]!=sys16_bg_page[i] ){
				old_bg_page[i] = sys16_bg_page[i];
				for( offset = page0; offset<page0+64*32; offset++ ){
					tilemap_mark_tile_dirty( background, offset );
				}
			}
			if( old_fg_page[i]!=sys16_fg_page[i] ){
				old_fg_page[i] = sys16_fg_page[i];
				for( offset = page0; offset<page0+64*32; offset++ ){
					tilemap_mark_tile_dirty( foreground, offset );
				}
			}
			if( sys16_18_mode ){
				if( old_bg2_page[i]!=sys16_bg2_page[i] ){
					old_bg2_page[i] = sys16_bg2_page[i];
					for( offset = page0; offset<page0+64*32; offset++ ){
						tilemap_mark_tile_dirty( background2, offset );
					}
				}
				if( old_fg2_page[i]!=sys16_fg2_page[i] ){
					old_fg2_page[i] = sys16_fg2_page[i];
					for( offset = page0; offset<page0+64*32; offset++ ){
						tilemap_mark_tile_dirty( foreground2, offset );
					}
				}
			}
		}
	}
}

static void get_bg_tile_info( int offset ){
	const UINT16 *source = 64*32*sys16_bg_page[offset/(64*32)] + sys16_tileram;
	int data = source[offset%(64*32)];
	int tile_number = (data&0xfff) + 0x1000*((data&sys16_tilebank_switch)?sys16_tile_bank1:sys16_tile_bank0);

	if( sys16_textmode==2 ){ /* afterburner: ?---CCCT TTTTTTTT */
		SET_TILE_INFO(
				0,
				tile_number,
				512+384+((data>>6)&0x7f),
				0)
	}
	else if(sys16_textmode==0){
		SET_TILE_INFO(
				0,
				tile_number,
				(data>>6)&0x7f,
				0)
	}
	else{
		SET_TILE_INFO(
				0,
				tile_number,
				(data>>5)&0x7f,
				0)
	}

	switch(sys16_bg_priority_mode) {
	case 1: /* Alien Syndrome */
		tile_info.priority = (data&0x8000)?1:0;
		break;
	case 2: /* Body Slam / wrestwar */
		tile_info.priority = ((data&0xff00) >= sys16_bg_priority_value)?1:0;
		break;
	case 3: /* sys18 games */
		if( data&0x8000 ){
			tile_info.priority = 2;
		}
		else {
			tile_info.priority = ((data&0xff00) >= sys16_bg_priority_value)?1:0;
		}
		break;
	}
}

static void get_fg_tile_info( int offset ){
	const UINT16 *source = 64*32*sys16_fg_page[offset/(64*32)] + sys16_tileram;
	int data = source[offset%(64*32)];
	int tile_number = (data&0xfff) + 0x1000*((data&sys16_tilebank_switch)?sys16_tile_bank1:sys16_tile_bank0);

	if( sys16_textmode==2 ){ /* afterburner: ?---CCCT TTTTTTTT */
		SET_TILE_INFO(
				0,
				tile_number,
				512+384+((data>>6)&0x7f),
				0)
	}
	else if(sys16_textmode==0){
		SET_TILE_INFO(
				0,
				tile_number,
				(data>>6)&0x7f,
				0)
	}
	else{
		SET_TILE_INFO(
				0,
				tile_number,
				(data>>5)&0x7f,
				0)
	}
	switch(sys16_fg_priority_mode){
	case 1: /* alien syndrome */
		tile_info.priority = (data&0x8000)?1:0;
		break;

	case 3:
		tile_info.priority = ((data&0xff00) >= sys16_fg_priority_value)?1:0;
		break;

	case 4:
		tile_info.priority = ( (data/0x1000) % 2 != 0)?1:0;
		break;

	default:
		if( sys16_fg_priority_mode>=0 ){
			tile_info.priority = (data&0x8000)?1:0;
		}
		break;
	}
}

static void get_bg2_tile_info( int offset ){
	const UINT16 *source = 64*32*sys16_bg2_page[offset/(64*32)] + sys16_tileram;
	int data = source[offset%(64*32)];
	int tile_number = (data&0xfff) + 0x1000*((data&0x1000)?sys16_tile_bank1:sys16_tile_bank0);

	if( sys16_textmode==2 ){ /* afterburner: ?---CCCT TTTTTTTT */
		SET_TILE_INFO(
				0,
				tile_number,
				512+384+((data>>6)&0x7f),
				0)
	}
	else if(sys16_textmode==0){
		SET_TILE_INFO(
				0,
				tile_number,
				(data>>6)&0x7f,
				0)
	}
	else{
		SET_TILE_INFO(
				0,
				tile_number,
				(data>>5)&0x7f,
				0)
	}
	tile_info.priority = 0;
}

static void get_fg2_tile_info( int offset ){
	const UINT16 *source = 64*32*sys16_fg2_page[offset/(64*32)] + sys16_tileram;
	int data = source[offset%(64*32)];
	int tile_number = (data&0xfff) + 0x1000*((data&0x1000)?sys16_tile_bank1:sys16_tile_bank0);

	if( sys16_textmode==2 ){ /* afterburner: ?---CCCT TTTTTTTT */
		SET_TILE_INFO(
				0,
				tile_number,
				512+384+((data>>6)&0x7f),
				0)
	}
	else if(sys16_textmode==0){
		SET_TILE_INFO(
				0,
				tile_number,
				(data>>6)&0x7f,
				0)
	}
	else{
		SET_TILE_INFO(
				0,
				tile_number,
				(data>>5)&0x7f,
				0)
	}
	if((data&0xff00) >= sys16_fg_priority_value) tile_info.priority = 1;
	else tile_info.priority = 0;
}

WRITE16_HANDLER( sys16_tileram_w ){
	data16_t oldword = sys16_tileram[offset];
	COMBINE_DATA( &sys16_tileram[offset] );
	if( oldword != sys16_tileram[offset] ){
		int page = offset/(64*32);
		offset = offset%(64*32);

		if( sys16_bg_page[0]==page ) tilemap_mark_tile_dirty( background, offset+64*32*0 );
		if( sys16_bg_page[1]==page ) tilemap_mark_tile_dirty( background, offset+64*32*1 );
		if( sys16_bg_page[2]==page ) tilemap_mark_tile_dirty( background, offset+64*32*2 );
		if( sys16_bg_page[3]==page ) tilemap_mark_tile_dirty( background, offset+64*32*3 );

		if( sys16_fg_page[0]==page ) tilemap_mark_tile_dirty( foreground, offset+64*32*0 );
		if( sys16_fg_page[1]==page ) tilemap_mark_tile_dirty( foreground, offset+64*32*1 );
		if( sys16_fg_page[2]==page ) tilemap_mark_tile_dirty( foreground, offset+64*32*2 );
		if( sys16_fg_page[3]==page ) tilemap_mark_tile_dirty( foreground, offset+64*32*3 );

		if( sys16_18_mode ){
			if( sys16_bg2_page[0]==page ) tilemap_mark_tile_dirty( background2, offset+64*32*0 );
			if( sys16_bg2_page[1]==page ) tilemap_mark_tile_dirty( background2, offset+64*32*1 );
			if( sys16_bg2_page[2]==page ) tilemap_mark_tile_dirty( background2, offset+64*32*2 );
			if( sys16_bg2_page[3]==page ) tilemap_mark_tile_dirty( background2, offset+64*32*3 );

			if( sys16_fg2_page[0]==page ) tilemap_mark_tile_dirty( foreground2, offset+64*32*0 );
			if( sys16_fg2_page[1]==page ) tilemap_mark_tile_dirty( foreground2, offset+64*32*1 );
			if( sys16_fg2_page[2]==page ) tilemap_mark_tile_dirty( foreground2, offset+64*32*2 );
			if( sys16_fg2_page[3]==page ) tilemap_mark_tile_dirty( foreground2, offset+64*32*3 );
		}
	}
}

/***************************************************************************/

static void get_text_tile_info( int offset ){
	const data16_t *source = sys16_textram;
	int tile_number = source[offset];
	int pri = tile_number >> 8;
	if( sys16_textmode==2 ){ /* afterburner: ?---CCCT TTTTTTTT */
		SET_TILE_INFO(
				0,
				(tile_number&0x1ff) + sys16_tile_bank0 * 0x1000,
				512+384+((tile_number>>9)&0x7),
				0)
	}
	else if(sys16_textmode==0){
		SET_TILE_INFO(
				0,
				(tile_number&0x1ff) + sys16_tile_bank0 * 0x1000,
				(tile_number>>9)%8,
				0)
	}
	else{
		SET_TILE_INFO(
				0,
				(tile_number&0xff)  + sys16_tile_bank0 * 0x1000,
				(tile_number>>8)%8,
				0)
	}
	if(pri>=sys16_textlayer_lo_min && pri<=sys16_textlayer_lo_max)
		tile_info.priority = 1;
	if(pri>=sys16_textlayer_hi_min && pri<=sys16_textlayer_hi_max)
		tile_info.priority = 0;
}

WRITE16_HANDLER( sys16_textram_w ){
	int oldword = sys16_textram[offset];
	COMBINE_DATA( &sys16_textram[offset] );
	if( oldword!=sys16_textram[offset] ){
		tilemap_mark_tile_dirty( text_layer, offset );
	}
}

/***************************************************************************/

VIDEO_START( system16 ){
	static int bank_default[16] = {
		0x0,0x1,0x2,0x3,
		0x4,0x5,0x6,0x7,
		0x8,0x9,0xa,0xb,
		0xc,0xd,0xe,0xf
	};
	if (!sys16_obj_bank)
		sys16_obj_bank = bank_default;

	/* Normal colors */
	compute_resistor_weights(0, 255, -1.0,
		6, resistances_normal, weights[0][0], 0, 0,
		6, resistances_normal, weights[0][1], 0, 0,
		6, resistances_normal, weights[0][2], 0, 0
		);

	/* Shadow/Highlight colors */
	compute_resistor_weights(0, 255, -1.0,
		6, resistances_sh, weights[1][0], 0, 0,
		6, resistances_sh, weights[1][1], 0, 0,
		6, resistances_sh, weights[1][2], 0, 0
		);

	if( !sys16_bg1_trans )
		background = tilemap_create(
			get_bg_tile_info,
			sys16_bg_map,
			TILEMAP_OPAQUE,
			8,8,
			64*2,32*2 );
	else
		background = tilemap_create(
			get_bg_tile_info,
			sys16_bg_map,
			TILEMAP_TRANSPARENT,
			8,8,
			64*2,32*2 );

	foreground = tilemap_create(
		get_fg_tile_info,
		sys16_bg_map,
		TILEMAP_TRANSPARENT,
		8,8,
		64*2,32*2 );

	text_layer = tilemap_create(
		get_text_tile_info,
		sys16_text_map,
		TILEMAP_TRANSPARENT,
		8,8,
		40,28 );

	num_sprites = 128*2; /* only 128 for most games; aburner uses 256 */

	if(!strcmp(Machine->gamedrv->name, "hangon"))
		num_sprites = 128;

	if( background && foreground && text_layer ){
		/* initialize all entries to black - needed for Golden Axe*/
		int i;
		for( i=0; i<Machine->drv->total_colors; i++ ){
			palette_set_color( i, 0,0,0 );
		}

		if(sys16_bg1_trans) tilemap_set_transparent_pen( background, 0 );
		tilemap_set_transparent_pen( foreground, 0 );
		tilemap_set_transparent_pen( text_layer, 0 );

		sys16_tile_bank0 = 0;
		sys16_tile_bank1 = 1;

		sys16_fg_scrollx = 0;
		sys16_fg_scrolly = 0;

		sys16_bg_scrollx = 0;
		sys16_bg_scrolly = 0;

		sys16_refreshenable = 1;

		/* common defaults */
		sys16_spritesystem = sys16_sprite_shinobi;
		sys16_textmode = 0;
		sys16_bgxoffset = 0;
		sys16_bg_priority_mode=0;
		sys16_fg_priority_mode=0;
		sys16_tilebank_switch=0x1000;

		/* Defaults for sys16 games */
		sys16_textlayer_lo_min=0;
		sys16_textlayer_lo_max=0x7f;
		sys16_textlayer_hi_min=0x80;
		sys16_textlayer_hi_max=0xff;

		sys16_18_mode=0;

#ifdef GAMMA_ADJUST
		{
			static float sys16_orig_gamma=0;
			static float sys16_set_gamma=0;
			float cur_gamma=osd_get_gamma();

			if(sys16_orig_gamma == 0)
			{
				sys16_orig_gamma = cur_gamma;
				sys16_set_gamma = cur_gamma - 0.35;
				if (sys16_set_gamma < 0.5) sys16_set_gamma = 0.5;
				if (sys16_set_gamma > 2.0) sys16_set_gamma = 2.0;
				osd_set_gamma(sys16_set_gamma);
			}
			else
			{
				if(sys16_orig_gamma == cur_gamma)
				{
					osd_set_gamma(sys16_set_gamma);
				}
			}
		}
#endif
		return 0;
	}
	return 1;
}

VIDEO_START( hangon ){
	int ret;
	sys16_bg1_trans=1;
	ret = video_start_system16();
	if(ret) return 1;

	sys16_textlayer_lo_min=0;
	sys16_textlayer_lo_max=0;
	sys16_textlayer_hi_min=0;
	sys16_textlayer_hi_max=0xff;

	sys16_bg_priority_mode=-1;
	sys16_bg_priority_value=0x1800;
	sys16_fg_priority_value=0x2000;
	return 0;
}

VIDEO_START( system18 ){
	sys16_bg1_trans=1;

	background2 = tilemap_create(
		get_bg2_tile_info,
		sys16_bg_map,
		TILEMAP_OPAQUE,
		8,8,
		64*2,32*2 );

	foreground2 = tilemap_create(
		get_fg2_tile_info,
		sys16_bg_map,
		TILEMAP_TRANSPARENT,
		8,8,
		64*2,32*2 );

	if( background2 && foreground2 ){
		if( video_start_system16()==0 ){
			tilemap_set_transparent_pen( foreground2, 0 );

			if(sys18_splittab_fg_x){
				tilemap_set_scroll_rows( foreground , 64 );
				tilemap_set_scroll_rows( foreground2 , 64 );
			}
			if(sys18_splittab_bg_x){
				tilemap_set_scroll_rows( background , 64 );
				tilemap_set_scroll_rows( background2 , 64 );
			}

			sys16_textlayer_lo_min=0;
			sys16_textlayer_lo_max=0x1f;
			sys16_textlayer_hi_min=0x20;
			sys16_textlayer_hi_max=0xff;

			sys16_18_mode=1;
			sys16_bg_priority_mode=3;
			sys16_fg_priority_mode=3;
			sys16_bg_priority_value=0x1800;
			sys16_fg_priority_value=0x2000;

			return 0;
		}
	}
	return 1;
}

/***************************************************************************/

static void sys16_vh_refresh_helper( void ){
	if( sys18_splittab_bg_x ){
		if( (sys16_bg_scrollx&0xff00)  != sys16_rowscroll_scroll ){
			tilemap_set_scroll_rows( background , 1 );
			tilemap_set_scrollx( background, 0, -320-sys16_bg_scrollx+sys16_bgxoffset );
		}
		else {
			int offset, scroll,i;

			tilemap_set_scroll_rows( background , 64 );
			offset = 32+((sys16_bg_scrolly&0x1f8) >> 3);

			for( i=0; i<29; i++ ){
				scroll = sys18_splittab_bg_x[i];
				tilemap_set_scrollx( background , (i+offset)&0x3f, -320-(scroll&0x3ff)+sys16_bgxoffset );
			}
		}
	}
	else {
		tilemap_set_scrollx( background, 0, -320-sys16_bg_scrollx+sys16_bgxoffset );
	}

	if( sys18_splittab_bg_y ){
		if( (sys16_bg_scrolly&0xff00)  != sys16_rowscroll_scroll ){
			tilemap_set_scroll_cols( background , 1 );
			tilemap_set_scrolly( background, 0, -256+sys16_bg_scrolly );
		}
		else {
			int offset, scroll,i;

			tilemap_set_scroll_cols( background , 128 );
			offset = 127-((sys16_bg_scrollx&0x3f8) >> 3)-40+2;

			for( i=0;i<41;i++ ){
				scroll = sys18_splittab_bg_y[(i+24)>>1];
				tilemap_set_scrolly( background , (i+offset)&0x7f, -256+(scroll&0x3ff) );
			}
		}
	}
	else {
		tilemap_set_scrolly( background, 0, -256+sys16_bg_scrolly );
	}

	if( sys18_splittab_fg_x ){
		if( (sys16_fg_scrollx&0xff00)  != sys16_rowscroll_scroll ){
			tilemap_set_scroll_rows( foreground , 1 );
			tilemap_set_scrollx( foreground, 0, -320-sys16_fg_scrollx+sys16_fgxoffset );
		}
		else {
			int offset, scroll,i;

			tilemap_set_scroll_rows( foreground , 64 );
			offset = 32+((sys16_fg_scrolly&0x1f8) >> 3);

			for(i=0;i<29;i++){
				scroll = sys18_splittab_fg_x[i];
				tilemap_set_scrollx( foreground , (i+offset)&0x3f, -320-(scroll&0x3ff)+sys16_fgxoffset );
			}
		}
	}
	else {
		tilemap_set_scrollx( foreground, 0, -320-sys16_fg_scrollx+sys16_fgxoffset );
	}

	if( sys18_splittab_fg_y ){
		if( (sys16_fg_scrolly&0xff00)  != sys16_rowscroll_scroll ){
			tilemap_set_scroll_cols( foreground , 1 );
			tilemap_set_scrolly( foreground, 0, -256+sys16_fg_scrolly );
		}
		else {
			int offset, scroll,i;

			tilemap_set_scroll_cols( foreground , 128 );
			offset = 127-((sys16_fg_scrollx&0x3f8) >> 3)-40+2;

			for( i=0; i<41; i++ ){
				scroll = sys18_splittab_fg_y[(i+24)>>1];
				tilemap_set_scrolly( foreground , (i+offset)&0x7f, -256+(scroll&0x3ff) );
			}
		}
	}
	else {
		tilemap_set_scrolly( foreground, 0, -256+sys16_fg_scrolly );
	}
}

static void sys18_vh_screenrefresh_helper( void ){
	int i;
	if( sys18_splittab_bg_x ){ /* screenwise rowscroll? */
		int offset,offset2, scroll,scroll2,orig_scroll;

		offset = 32+((sys16_bg_scrolly&0x1f8) >> 3); /* 0x00..0x3f */
		offset2 = 32+((sys16_bg2_scrolly&0x1f8) >> 3); /* 0x00..0x3f */

		for( i=0;i<29;i++ ){
			orig_scroll = scroll2 = scroll = sys18_splittab_bg_x[i];
			if((sys16_bg_scrollx  &0xff00) != 0x8000) scroll = sys16_bg_scrollx;
			if((sys16_bg2_scrollx &0xff00) != 0x8000) scroll2 = sys16_bg2_scrollx;

			if(orig_scroll&0x8000){ /* background2 */
				tilemap_set_scrollx( background , (i+offset)&0x3f, TILE_LINE_DISABLED );
				tilemap_set_scrollx( background2, (i+offset2)&0x3f, -320-(scroll2&0x3ff)+sys16_bgxoffset );
			}
			else{ /* background */
				tilemap_set_scrollx( background , (i+offset)&0x3f, -320-(scroll&0x3ff)+sys16_bgxoffset );
				tilemap_set_scrollx( background2, (i+offset2)&0x3f, TILE_LINE_DISABLED );
			}
		}
	}
	else {
		tilemap_set_scrollx( background , 0, -320-(sys16_bg_scrollx&0x3ff)+sys16_bgxoffset );
		tilemap_set_scrollx( background2, 0, -320-(sys16_bg2_scrollx&0x3ff)+sys16_bgxoffset );
	}

	tilemap_set_scrolly( background , 0, -256+sys16_bg_scrolly );
	tilemap_set_scrolly( background2, 0, -256+sys16_bg2_scrolly );

	if( sys18_splittab_fg_x ){
		int offset,offset2, scroll,scroll2,orig_scroll;

		offset = 32+((sys16_fg_scrolly&0x1f8) >> 3);
		offset2 = 32+((sys16_fg2_scrolly&0x1f8) >> 3);

		for( i=0;i<29;i++ ){
			orig_scroll = scroll2 = scroll = sys18_splittab_fg_x[i];
			if( (sys16_fg_scrollx &0xff00) != 0x8000 ) scroll = sys16_fg_scrollx;

			if( (sys16_fg2_scrollx &0xff00) != 0x8000 ) scroll2 = sys16_fg2_scrollx;

			if( orig_scroll&0x8000 ){
				tilemap_set_scrollx( foreground , (i+offset)&0x3f, TILE_LINE_DISABLED );
				tilemap_set_scrollx( foreground2, (i+offset2)&0x3f, -320-(scroll2&0x3ff)+sys16_fgxoffset );
			}
			else {
				tilemap_set_scrollx( foreground , (i+offset)&0x3f, -320-(scroll&0x3ff)+sys16_fgxoffset );
				tilemap_set_scrollx( foreground2 , (i+offset2)&0x3f, TILE_LINE_DISABLED );
			}
		}
	}
	else {
		tilemap_set_scrollx( foreground , 0, -320-(sys16_fg_scrollx&0x3ff)+sys16_fgxoffset );
		tilemap_set_scrollx( foreground2, 0, -320-(sys16_fg2_scrollx&0x3ff)+sys16_fgxoffset );
	}


	tilemap_set_scrolly( foreground , 0, -256+sys16_fg_scrolly );
	tilemap_set_scrolly( foreground2, 0, -256+sys16_fg2_scrolly );

	tilemap_set_enable( background2, sys18_bg2_active );
	tilemap_set_enable( foreground2, sys18_fg2_active );
}

VIDEO_UPDATE( system16 ){
	if (!sys16_refreshenable) 
	{
		fillbitmap(bitmap, get_black_pen(), cliprect);
		return;
	}

	if( sys16_update_proc ) sys16_update_proc();
	update_page();
	sys16_vh_refresh_helper(); /* set scroll registers */

	fillbitmap(priority_bitmap,0,cliprect);

	tilemap_draw( bitmap,cliprect, background, TILEMAP_IGNORE_TRANSPARENCY, 0x00 );
	if(sys16_bg_priority_mode) tilemap_draw( bitmap,cliprect, background, TILEMAP_IGNORE_TRANSPARENCY | 1, 0x00 );
/*	sprite_draw(sprite_list,3);*/ /* needed for Aurail */
	if( sys16_bg_priority_mode==2 ) tilemap_draw( bitmap,cliprect, background, 1, 0x01 );/* body slam (& wrestwar??) */
/*	sprite_draw(sprite_list,2); */
	else if( sys16_bg_priority_mode==1 ) tilemap_draw( bitmap,cliprect, background, 1, 0x03 );/* alien syndrome / aurail */
	tilemap_draw( bitmap,cliprect, foreground, 0, 0x03 );
/*	sprite_draw(sprite_list,1); */
	tilemap_draw( bitmap,cliprect, foreground, 1, 0x07 );
	if( sys16_textlayer_lo_max!=0 ) tilemap_draw( bitmap,cliprect, text_layer, 1, 7 );/* needed for Body Slam */
/*	sprite_draw(sprite_list,0); */
	tilemap_draw( bitmap,cliprect, text_layer, 0, 0xf );

	if (!sys16_sprite_draw)
	{
		draw_sprites( bitmap,cliprect,0 );
	}
	else
		draw_sprites_new( bitmap,cliprect);
}

VIDEO_UPDATE( system18 ){
	if (!sys16_refreshenable) 
	{
		fillbitmap(bitmap, get_black_pen(), cliprect);
		return;
	}
	if( sys16_update_proc ) sys16_update_proc();
	update_page();
	sys18_vh_screenrefresh_helper(); /* set scroll registers */

	fillbitmap(priority_bitmap,0,NULL);
	if(sys18_bg2_active)
		tilemap_draw( bitmap,cliprect, background2, 0, 0 );
	else
		fillbitmap(bitmap,Machine->pens[0],cliprect);

	tilemap_draw( bitmap,cliprect, background, TILEMAP_IGNORE_TRANSPARENCY, 0 );
	tilemap_draw( bitmap,cliprect, background, TILEMAP_IGNORE_TRANSPARENCY | 1, 0 );
	tilemap_draw( bitmap,cliprect, background, TILEMAP_IGNORE_TRANSPARENCY | 2, 0 );

/*	sprite_draw(sprite_list,3); */
	tilemap_draw( bitmap,cliprect, background, 1, 0x1 );
/*	sprite_draw(sprite_list,2); */
	tilemap_draw( bitmap,cliprect, background, 2, 0x3 );

	if(sys18_fg2_active) tilemap_draw( bitmap,cliprect, foreground2, 0, 0x3 );
	tilemap_draw( bitmap,cliprect, foreground, 0, 0x3 );
/*	sprite_draw(sprite_list,1); */
	if(sys18_fg2_active) tilemap_draw( bitmap,cliprect, foreground2, 1, 0x7 );
	tilemap_draw( bitmap,cliprect, foreground, 1, 0x7 );

	tilemap_draw( bitmap,cliprect, text_layer, 1, 0x7 );
/*	sprite_draw(sprite_list,0); */
	tilemap_draw( bitmap,cliprect, text_layer, 0, 0xf );

	if (!sys16_sprite_draw)
	{
		draw_sprites( bitmap,cliprect,0 );
	}
	else
		draw_sprites_new( bitmap,cliprect);
}


static void render_gr(struct mame_bitmap *bitmap,const struct rectangle *cliprect,int priority){
	/* the road is a 4 color bitmap */
	int i,j;
	UINT8 *data = memory_region(REGION_GFX3);
	UINT8 *source;
	UINT16 *line16;
	UINT16 *data_ver=sys16_gr_ver;
	UINT32 ver_data,hor_pos;
	UINT16 colors[5];
	int colorflip;
	int yflip=0, ypos;
	int dx=1,xoff=0;

	pen_t *paldata1 = Machine->gfx[0]->colortable + sys16_gr_palette;
	pen_t *paldata2 = Machine->gfx[0]->colortable + sys16_gr_palette_default;

#if 0
if( keyboard_pressed( KEYCODE_S ) ){
	FILE *f;
	int i;
	char fname[64];
	static int fcount = 0;
	while( keyboard_pressed( KEYCODE_S ) ){}
	sprintf( fname, "road%d.txt",fcount );
	fcount++;
	f = fopen( fname,"w" );
	if( f ){
		const UINT16 *source = sys16_gr_ver;
		for( i=0; i<0x1000; i++ ){
			if( (i&0x1f)==0 ) fprintf( f, "\n %04x: ", i );
			fprintf( f, "%04x ", source[i] );
		}
		fclose( f );
	}
}
#endif

	priority=priority << 10;

	if (Machine->scrbitmap->depth == 16) /* 16 bit */
	{
		if( Machine->orientation & ORIENTATION_SWAP_XY ){
			if( Machine->orientation & ORIENTATION_FLIP_Y ){
				dx=-1;
				xoff=319;
			}
			if( Machine->orientation & ORIENTATION_FLIP_X ){
				yflip=1;
			}

			for(i=cliprect->min_y;i<=cliprect->max_y;i++){
				if(yflip) ypos=223-i;
				else ypos=i;
				ver_data=*data_ver;
				if((ver_data & 0x400) == priority)
				{
					colors[0] = paldata1[ sys16_gr_pal[(ver_data)&0xff]&0xff ];

					if((ver_data & 0x500) == 0x100 || (ver_data & 0x300) == 0x200)
					{
						/* fill line */
						for(j=cliprect->min_x;j<=cliprect->max_x;j++)
						{
							line16=(UINT16 *)bitmap->line[j]+ypos;
							*line16=colors[0];
						}
					}
					else
					{
						/* copy line */
						ver_data=ver_data & 0x00ff;
						colorflip = (sys16_gr_flip[ver_data] >> 3) & 1;

						colors[1] = paldata2[ sys16_gr_colorflip[colorflip][0] ];
						colors[2] = paldata2[ sys16_gr_colorflip[colorflip][1] ];
						colors[3] = paldata2[ sys16_gr_colorflip[colorflip][2] ];
						colors[4] = paldata2[ sys16_gr_colorflip[colorflip][3] ];

						hor_pos = sys16_gr_hor[ver_data];
						ver_data = ver_data << sys16_gr_bitmap_width;

						if(hor_pos & 0xf000)
						{
							/* reverse */
							hor_pos=((0-((hor_pos&0x7ff)^7))+0x9f8)&0x3ff;
						}
						else
						{
							/* normal */
							hor_pos=(hor_pos+0x200) & 0x3ff;
						}

						source = data + hor_pos + ver_data + 18 + 8;

						for(j=cliprect->min_x;j<cliprect->max_x;j++)
						{
							line16=(UINT16 *)bitmap->line[xoff+j*dx]+ypos;
							*line16 = colors[*source++];
						}
					}
				}
				data_ver++;
			}
		}
		else
		{ /* 16 bpp, normal screen orientation */
			if( Machine->orientation & ORIENTATION_FLIP_X ){
				dx=-1;
				xoff=319;
			}
			if( Machine->orientation & ORIENTATION_FLIP_Y ){
				yflip=1;
			}

			for(i=cliprect->min_y;i<=cliprect->max_y;i++){ /* with each scanline */
				if( yflip ) ypos=223-i; else ypos=i;
				ver_data= *data_ver; /* scanline parameters */
				/*
					gr_ver:
						---- -x-- ---- ----	priority
						---- --x- ---- ---- ?
						---- ---x ---- ---- ?
						---- ---- xxxx xxxx ypos (source bitmap)

					gr_flip:
						---- ---- ---- x--- flip colors

					gr_hor:
						xxxx xxxx xxxx xxxx	xscroll

					gr_pal:
						---- ---- xxxx xxxx palette
				*/
				if( (ver_data & 0x400) == priority ){
					colors[0] = paldata1[ sys16_gr_pal[ver_data&0xff]&0xff ];

					if((ver_data & 0x500) == 0x100 || (ver_data & 0x300) == 0x200){
						line16 = (UINT16 *)bitmap->line[ypos]; /* dest for drawing */
						for(j=cliprect->min_x;j<=cliprect->max_x;j++){
							*line16++=colors[0]; /* opaque fill with background color */
						}
					}
					else {
						/* copy line */
						line16 = (UINT16 *)bitmap->line[ypos]+xoff; /* dest for drawing */
						ver_data &= 0xff;

						colorflip = (sys16_gr_flip[ver_data] >> 3) & 1;
						colors[1] = paldata2[ sys16_gr_colorflip[colorflip][0] ];
						colors[2] = paldata2[ sys16_gr_colorflip[colorflip][1] ];
						colors[3] = paldata2[ sys16_gr_colorflip[colorflip][2] ];
						colors[4] = paldata2[ sys16_gr_colorflip[colorflip][3] ];

						hor_pos = sys16_gr_hor[ver_data];
						if( hor_pos & 0xf000 ){ /* reverse (precalculated) */
							hor_pos=((0-((hor_pos&0x7ff)^7))+0x9f8)&0x3ff;
						}
						else { /* normal */
							hor_pos=(hor_pos+0x200) & 0x3ff;
						}

						ver_data <<= sys16_gr_bitmap_width;
						source = data + hor_pos + ver_data + 18 + 8;

						for(j=cliprect->min_x;j<=cliprect->max_x;j++){
							*line16 = colors[*source++];
							line16+=dx;
						}
					}
				}
				data_ver++;
			}
		}
	}
}

VIDEO_UPDATE( hangon ){
	if (!sys16_refreshenable) 
	{
		fillbitmap(bitmap, get_black_pen(), cliprect);
		return;
	}
	if( sys16_update_proc ) sys16_update_proc();
	update_page();

	tilemap_set_scrollx( background, 0, -320-sys16_bg_scrollx+sys16_bgxoffset );
	tilemap_set_scrollx( foreground, 0, -320-sys16_fg_scrollx+sys16_fgxoffset );
	tilemap_set_scrolly( background, 0, -256+sys16_bg_scrolly );
	tilemap_set_scrolly( foreground, 0, -256+sys16_fg_scrolly );

	fillbitmap(priority_bitmap,0,cliprect);

	render_gr(bitmap,cliprect,0); /* sky */
	tilemap_draw( bitmap,cliprect, background, 0, 0 );
	tilemap_draw( bitmap,cliprect, foreground, 0, 0 );
	render_gr(bitmap,cliprect,1); /* floor */
	tilemap_draw( bitmap,cliprect, text_layer, 0, 0xf );

	if (!sys16_sprite_draw)
	{
		draw_sprites( bitmap,cliprect,0 );
	}
	else
		draw_sprites_new( bitmap,cliprect);
}

static void render_grv2(struct mame_bitmap *bitmap,const struct rectangle *cliprect,int priority)
{
	int i,j;
	UINT8 *data = memory_region(REGION_GFX3);
	UINT8 *source,*source2,*temp;
	UINT16 *line16;
	UINT16 *data_ver=sys16_gr_ver;
	UINT32 ver_data,hor_pos,hor_pos2;
	UINT16 colors[5];
	int colorflip,colorflip_info;
	int yflip=0,ypos;
	int dx=1,xoff=0;

	int second_road = sys16_gr_second_road[0];

	pen_t *paldata1 = Machine->gfx[0]->colortable + sys16_gr_palette;
	pen_t *paldata2 = Machine->gfx[0]->colortable + sys16_gr_palette_default;

	priority=priority << 11;

	if (Machine->scrbitmap->depth == 16) /* 16 bit */
	{
		if( Machine->orientation & ORIENTATION_SWAP_XY )
		{
			if( Machine->orientation & ORIENTATION_FLIP_Y ){
				dx=-1;
				xoff=319;
			}
			if( Machine->orientation & ORIENTATION_FLIP_X ){
				yflip=1;
			}

			for(i=cliprect->min_y;i<=cliprect->max_y;i++)
			{
				if(yflip) ypos=223-i;
				else ypos=i;
				ver_data = *data_ver;
				if((ver_data & 0x800) == priority)
				{

					if(ver_data & 0x800) /* disable */
					{
						colors[0] = paldata1[ ver_data&0x3f ];
						/* fill line */
						for(j=cliprect->min_x;j<=cliprect->max_x;j++)
						{
							line16=(UINT16 *)bitmap->line[j]+ypos;
							*line16=colors[0];
						}
					}
					else
					{
						/* copy line */
						ver_data=ver_data & 0x01ff;		/*??? */
						colorflip_info = sys16_gr_flip[ver_data];

						colors[0] = paldata2[ ((colorflip_info >> 8) & 0x1f) + 0x20 ];

						colorflip = (colorflip_info >> 3) & 1;

						colors[1] = paldata2[ sys16_gr_colorflip[colorflip][0] ];
						colors[2] = paldata2[ sys16_gr_colorflip[colorflip][1] ];
						colors[3] = paldata2[ sys16_gr_colorflip[colorflip][2] ];

						hor_pos = sys16_gr_hor[ver_data];
						hor_pos2= sys16_gr_hor[ver_data+0x200];

						ver_data=ver_data>>1;
						if( ver_data != 0 )
						{
							ver_data = (ver_data-1) << sys16_gr_bitmap_width;
						}
						source  = data + ((hor_pos +0x200) & 0x7ff) + 0x300 + ver_data + 8;
						source2 = data + ((hor_pos2+0x200) & 0x7ff) + 0x300 + ver_data + 8;

						switch(second_road)
						{
							case 0:	source2=source;	break;
							case 2:	temp=source;source=source2;source2=temp; break;
							case 3:	source=source2;	break;
						}

						source2++;

						for(j=cliprect->min_x;j<=cliprect->max_x;j++)
						{
							line16=(UINT16 *)bitmap->line[xoff+j*dx]+ypos;
							if(*source2 <= *source)
								*line16 = colors[*source];
							else
								*line16 = colors[*source2];
							source++;
							source2++;
						}
					}
				}
				data_ver++;
			}
		}
		else
		{
			if( Machine->orientation & ORIENTATION_FLIP_X ){
				dx=-1;
				xoff=319;
			}
			if( Machine->orientation & ORIENTATION_FLIP_Y ){
				yflip=1;
			}

			for(i=cliprect->min_y;i<=cliprect->max_y;i++){
				if(yflip) ypos=223-i;
				else ypos=i;
				ver_data= *data_ver;
				if((ver_data & 0x800) == priority){
					if(ver_data & 0x800){
						colors[0] = paldata1[ ver_data&0x3f ];
						/* fill line */
						line16 = (UINT16 *)bitmap->line[ypos];
						for(j=cliprect->min_x;j<=cliprect->max_x;j++){
							*line16++ = colors[0];
						}
					}
					else {
						/* copy line */
						line16 = (UINT16 *)bitmap->line[ypos]+xoff;
						ver_data &= 0x01ff;		/*??? */
						colorflip_info = sys16_gr_flip[ver_data];
						colors[0] = paldata2[ ((colorflip_info >> 8) & 0x1f) + 0x20 ];		/*?? */
						colorflip = (colorflip_info >> 3) & 1;
						colors[1] = paldata2[ sys16_gr_colorflip[colorflip][0] ];
						colors[2] = paldata2[ sys16_gr_colorflip[colorflip][1] ];
						colors[3] = paldata2[ sys16_gr_colorflip[colorflip][2] ];
						hor_pos = sys16_gr_hor[ver_data];
						hor_pos2= sys16_gr_hor[ver_data+0x200];
						ver_data=ver_data>>1;
						if( ver_data != 0 ){
							ver_data = (ver_data-1) << sys16_gr_bitmap_width;
						}
						source  = data + ((hor_pos +0x200) & 0x7ff) + 768 + ver_data + 8;
						source2 = data + ((hor_pos2+0x200) & 0x7ff) + 768 + ver_data + 8;
						switch(second_road){
							case 0:	source2=source;	break;
							case 2:	temp=source;source=source2;source2=temp; break;
							case 3:	source=source2;	break;
						}
						source2++;
						for(j=cliprect->min_x;j<=cliprect->max_x;j++){
							if(*source2 <= *source) *line16 = colors[*source]; else *line16 = colors[*source2];
							source++;
							source2++;
							line16+=dx;
						}
					}
				}
				data_ver++;
			}
		}
	}
}


VIDEO_START( outrun_old ){
	int ret;
	sys16_bg1_trans=1;
	ret = video_start_system16();
	if(ret) return 1;

	sys16_textlayer_lo_min=0;
	sys16_textlayer_lo_max=0;
	sys16_textlayer_hi_min=0;
	sys16_textlayer_hi_max=0xff;

	sys16_bg_priority_mode=-1;
	sys16_bg_priority_value=0x1800;
	sys16_fg_priority_value=0x2000;
	return 0;
}

VIDEO_UPDATE( outrun_old )
{
	if (!sys16_refreshenable) 
	{
		fillbitmap(bitmap, get_black_pen(), cliprect);
		return;
	}
	if( sys16_update_proc ) sys16_update_proc();
	update_page();

	tilemap_set_scrollx( background, 0, -320-sys16_bg_scrollx+sys16_bgxoffset );
	tilemap_set_scrollx( foreground, 0, -320-sys16_fg_scrollx+sys16_fgxoffset );

	tilemap_set_scrolly( background, 0, -256+sys16_bg_scrolly );
	tilemap_set_scrolly( foreground, 0, -256+sys16_fg_scrolly );

	render_grv2(bitmap,cliprect,1);
	tilemap_draw( bitmap,cliprect, background, 0, 0 );
	tilemap_draw( bitmap,cliprect, foreground, 0, 0 );
	render_grv2(bitmap,cliprect,0);

	if (!sys16_sprite_draw)
		draw_sprites( bitmap,cliprect,1 );
	else
		draw_sprites_new( bitmap,cliprect);

	tilemap_draw( bitmap,cliprect, text_layer, 0, 0 );
}


/***************************************************************************/

static UINT8 *aburner_backdrop;

UINT8 *aburner_unpack_backdrop( const UINT8 *baseaddr ){
	UINT8 *result = auto_malloc(512*256*2);
	if( result ){
		int page;
		for( page=0; page<2; page++ ){
			UINT8 *dest = result + 512*256*page;
			const UINT8 *source = baseaddr + 0x8000*page;
			int y;
			for( y=0; y<256; y++ ){
				int x;
				for( x=0; x<512; x++ ){
					int data0 = source[x/8];
					int data1 = source[x/8 + 0x4000];
					int bit = 0x80>>(x&7);
					int pen = 0;
					if( data0 & bit ) pen+=1;
					if( data1 & bit ) pen+=2;
					dest[x] = pen;
				}

				{
					int edge_color = dest[0];
					for( x=0; x<512; x++ ){
						if( dest[x]==edge_color ) dest[x] = 4; else break;
					}
					edge_color = dest[511];
					for( x=511; x>=0; x-- ){
						if( dest[x]==edge_color ) dest[x] = 4; else break;
					}
				}

				source += 0x40;
				dest += 512;
			}
		}
	}
	return result;
}

VIDEO_START( aburner ){
	int ret;

	aburner_backdrop = aburner_unpack_backdrop( memory_region(REGION_GFX3) );

	sys16_bg1_trans=1;
	ret = video_start_system16();
	if(ret) return 1;

	foreground2 = tilemap_create(
		get_fg2_tile_info,
		sys16_bg_map,
		TILEMAP_TRANSPARENT,
		8,8,
		64*2,32*2 );

	background2 = tilemap_create(
		get_bg2_tile_info,
		sys16_bg_map,
		TILEMAP_TRANSPARENT,
		8,8,
		64*2,32*2 );

	if( foreground2 && background2 ){
		ret = video_start_system16();
		if(ret) return 1;
		tilemap_set_transparent_pen( foreground2, 0 );
		sys16_18_mode = 1;

		tilemap_set_scroll_rows( foreground , 64 );
		tilemap_set_scroll_rows( foreground2 , 64 );
		tilemap_set_scroll_rows( background , 64 );
		tilemap_set_scroll_rows( background2 , 64 );

		return 0;
	}
	return 1;
}

static void aburner_draw_road( struct mame_bitmap *bitmap, const struct rectangle *cliprect ){
	/*
		sys16_roadram[0x1000]:
			0x04: flying (sky/horizon)
			0x43: (flying->landing)
			0xc3: runway landing
			0xe3: (landing -> flying)
			0x03: rocky canyon

			Thunderblade: 0x04, 0xfe
	*/

	/*	Palette:
	**		0x1700	ground(8), road(4), road(4)
	**		0x1720	sky(16)
	**		0x1730	road edge(2)
	**		0x1780	background color(16)
	*/

	const UINT16 *vreg = sys16_roadram;
	/*	0x000..0x0ff: 0x800: disable; 0x100: enable
		0x100..0x1ff: color/line_select
		0x200..0x2ff: xscroll
		0x400..0x4ff: 0x5b0?
		0x600..0x6ff: flip colors
	*/
	int page = sys16_roadram[0x1000];
	int sy;

	for( sy=cliprect->min_y; sy<=cliprect->max_y; sy++ ){
		UINT16 *dest = (UINT16 *)bitmap->line[sy] + cliprect->min_x; /* assume 16bpp */
		int sx;
		UINT16 line = vreg[0x100+sy];

		if( page&4 ){ /* flying */
			int xscroll = vreg[0x200+sy] - 0x552;
			UINT16 sky = Machine->pens[0x1720];
			UINT16 ground = Machine->pens[0x1700];
			for( sx=cliprect->min_x; sx<=cliprect->max_x; sx++ ){
				int temp = xscroll+sx;
				if( temp<0 ){
					*dest++ = sky;
				}
				else if( temp < 0x200 ){
					*dest++ = ground;
				}
				else {
					*dest++ = sky;
				}
			}
		}
		else if( line&0x800 ){
			/* opaque fill; the least significant nibble selects color */
			unsigned short color = Machine->pens[0x1780+(line&0xf)];
			for( sx=cliprect->min_x; sx<=cliprect->max_x; sx++ ){
				*dest++ = color;
			}
		}
		else if( page&0xc0 ){ /* road */
			const UINT8 *source = aburner_backdrop+(line&0xff)*512 + 512*256*(page&1);
			UINT16 xscroll = (512-320)/2;
			/* 040d 04b0 0552: normal: sky,horizon,sea */

			UINT16 flip = vreg[0x600+sy];
			int clut[5];
			{
				int road_color = 0x1708+(flip&0x1);
				clut[0] = Machine->pens[road_color];
				clut[1] = Machine->pens[road_color+2];
				clut[2] = Machine->pens[road_color+4];
				clut[3] = Machine->pens[road_color+6];
				clut[4] = Machine->pens[(flip&0x100)?0x1730:0x1731]; /* edge of road */
			}
			for( sx=cliprect->min_x; sx<=cliprect->max_x; sx++ ){
				int xpos = (sx + xscroll)&0x1ff;
				*dest++ = clut[source[xpos]];
			}
		}
		else { /* rocky canyon */
			UINT16 flip = vreg[0x600+sy];
			unsigned short color = Machine->pens[(flip&0x100)?0x1730:0x1731];
			for( sx=cliprect->min_x; sx<=cliprect->max_x; sx++ ){
				*dest++ = color;
			}
		}
	}
#if 0
	if( keyboard_pressed( KEYCODE_S ) ){ /* debug */
		FILE *f;
		int i;
		char fname[64];
		static int fcount = 0;
		while( keyboard_pressed( KEYCODE_S ) ){}
		sprintf( fname, "road%d.txt",fcount );
		fcount++;
		f = fopen( fname,"w" );
		if( f ){
			const UINT16 *source = sys16_roadram;
			for( i=0; i<0x1000; i++ ){
				if( (i&0x1f)==0 ) fprintf( f, "\n %04x: ", i );
				fprintf( f, "%04x ", source[i] );
			}
			fclose( f );
		}
	}
#endif
}

static void sys16_aburner_vh_screenrefresh_helper( void ){
	const data16_t *vreg = &sys16_textram[0x740];
	int i;

	{
		UINT16 data = vreg[0];
		sys16_fg_page[0] = data>>12;
		sys16_fg_page[1] = (data>>8)&0xf;
		sys16_fg_page[2] = (data>>4)&0xf;
		sys16_fg_page[3] = data&0xf;
		sys16_fg_scrolly = vreg[8];
		sys16_fg_scrollx = vreg[12];
	}

	{
		UINT16 data = vreg[0+1];
		sys16_bg_page[0] = data>>12;
		sys16_bg_page[1] = (data>>8)&0xf;
		sys16_bg_page[2] = (data>>4)&0xf;
		sys16_bg_page[3] = data&0xf;
		sys16_bg_scrolly = vreg[8+1];
		sys16_bg_scrollx = vreg[12+1];
	}

	{
		UINT16 data = vreg[0+2];
		sys16_fg2_page[0] = data>>12;
		sys16_fg2_page[1] = (data>>8)&0xf;
		sys16_fg2_page[2] = (data>>4)&0xf;
		sys16_fg2_page[3] = data&0xf;
		sys16_fg2_scrolly = vreg[8+2];
		sys16_fg2_scrollx = vreg[12+2];
	}

	{
		UINT16 data = vreg[0+3];
		sys16_bg2_page[0] = data>>12;
		sys16_bg2_page[1] = (data>>8)&0xf;
		sys16_bg2_page[2] = (data>>4)&0xf;
		sys16_bg2_page[3] = data&0xf;
		sys16_bg2_scrolly = vreg[8+3];
		sys16_bg2_scrollx = vreg[12+3];
	}

	sys18_splittab_fg_x = &sys16_textram[0x7c0];
	sys18_splittab_bg_x = &sys16_textram[0x7e0];

	{
		int offset,offset2, scroll,scroll2,orig_scroll;
		offset  = 32+((sys16_bg_scrolly >>3)&0x3f ); /* screenwise rowscroll */
		offset2 = 32+((sys16_bg2_scrolly>>3)&0x3f ); /* screenwise rowscroll */

		for( i=0;i<29;i++ ){
			orig_scroll = scroll2 = scroll = sys18_splittab_bg_x[i];
			if((sys16_bg_scrollx  &0xff00) != 0x8000) scroll = sys16_bg_scrollx;
			if((sys16_bg2_scrollx &0xff00) != 0x8000) scroll2 = sys16_bg2_scrollx;

			if( orig_scroll&0x8000 ){ /* background2 */
				tilemap_set_scrollx( background , (i+offset)&0x3f, TILE_LINE_DISABLED );
				tilemap_set_scrollx( background2, (i+offset2)&0x3f, -320-(scroll2&0x3ff)+sys16_bgxoffset );
			}
			else{ /* background1 */
				tilemap_set_scrollx( background , (i+offset)&0x3f, -320-(scroll&0x3ff)+sys16_bgxoffset );
				tilemap_set_scrollx( background2, (i+offset2)&0x3f, TILE_LINE_DISABLED );
			}
		}
	}

	tilemap_set_scrolly( background , 0, -256+sys16_bg_scrolly );
	tilemap_set_scrolly( background2, 0, -256+sys16_bg2_scrolly );

	{
		int offset,offset2, scroll,scroll2,orig_scroll;
		offset  = 32+((sys16_fg_scrolly >>3)&0x3f ); /* screenwise rowscroll */
		offset2 = 32+((sys16_fg2_scrolly>>3)&0x3f ); /* screenwise rowscroll */

		for( i=0;i<29;i++ ){
			orig_scroll = scroll2 = scroll = sys18_splittab_fg_x[i];
			if( (sys16_fg_scrollx &0xff00) != 0x8000 ) scroll = sys16_fg_scrollx;

			if( (sys16_fg2_scrollx &0xff00) != 0x8000 ) scroll2 = sys16_fg2_scrollx;

			if( orig_scroll&0x8000 ){ /* foreground2 */
				tilemap_set_scrollx( foreground , (i+offset)&0x3f, TILE_LINE_DISABLED );
				tilemap_set_scrollx( foreground2, (i+offset2)&0x3f, -320-(scroll2&0x3ff)+sys16_fgxoffset );
			}
			else { /* foreground */
				tilemap_set_scrollx( foreground , (i+offset)&0x3f, -320-(scroll&0x3ff)+sys16_fgxoffset );
				tilemap_set_scrollx( foreground2 , (i+offset2)&0x3f, TILE_LINE_DISABLED );
			}
		}
	}

	tilemap_set_scrolly( foreground , 0, -256+sys16_fg_scrolly );
	tilemap_set_scrolly( foreground2, 0, -256+sys16_fg2_scrolly );
}

VIDEO_UPDATE( aburner ){
	sys16_aburner_vh_screenrefresh_helper();
	update_page();

	fillbitmap(priority_bitmap,0,cliprect);

	aburner_draw_road( bitmap,cliprect );

/*	tilemap_draw( bitmap,cliprect, background2, 0, 7 ); */
/*	tilemap_draw( bitmap,cliprect, background2, 1, 7 ); */

	/* speed indicator, high score header */
	tilemap_draw( bitmap,cliprect, background, 0, 7 );
	tilemap_draw( bitmap,cliprect, background, 1, 7 );

	/* radar view */
	tilemap_draw( bitmap,cliprect, foreground2, 0, 7 );
	tilemap_draw( bitmap,cliprect, foreground2, 1, 7 );

	/* hand, scores */
	tilemap_draw( bitmap,cliprect, foreground, 0, 7 );
	tilemap_draw( bitmap,cliprect, foreground, 1, 7 );

	tilemap_draw( bitmap,cliprect, text_layer, 0, 7 );

	if (!sys16_sprite_draw)
	{
		draw_sprites( bitmap,cliprect,2 );
	}
	else
		draw_sprites_new( bitmap,cliprect);

/*	debug_draw( bitmap,cliprect, 8,8,sys16_roadram[0x1000] ); */
}
