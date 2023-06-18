/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.


Graphics information:

0x00000 - 0xdfff	: Blocks of sprite data, each 0x80 bytes:
	Each 0x80 block is made up of 0x20 double words, their format is:
	Word: Sprite number (16 bits)
	Byte: Palette number (8 bits)
	Byte: Bit 0: X flip
		  Bit 1: Y flip
		  Bit 2: Automatic animation flag (4 tiles?)
		  Bit 3: Automatic animation flag (8 tiles?)
		  Bit 4: MSB of sprite number (confirmed, Karnov_r, Mslug). See note.
		  Bit 5: MSB of sprite number (MSlug2)
		  Bit 6: MSB of sprite number (Kof97)
		  Bit 7: Unknown for now

	Each double word sprite is drawn directly underneath the previous one,
	based on the starting coordinates.

0x0e000 - 0x0ea00	: Front plane fix tiles (8*8), 2 bytes each

0x10000: Control for sprites banks, arranged in words

	Bit 0 to 3 - Y zoom LSB
	Bit 4 to 7 - Y zoom MSB (ie, 1 byte for Y zoom).
	Bit 8 to 11 - X zoom, 0xf is full size (no scale).
	Bit 12 to 15 - Unknown, probably unused

0x10400: Control for sprite banks, arranged in words

	Bit 0 to 5: Number of sprites in this bank (see note below).
	Bit 6 - If set, this bank is placed to right of previous bank (same Y-coord).
	Bit 7 to 15 - Y position for sprite bank.

0x10800: Control for sprite banks, arranged in words
	Bit 0 to 5: Unknown
	Bit 7 to 15 - X position for sprite bank.

Notes:

* If rom set has less than 0x10000 tiles then msb of tile must be ignored
(see Magician Lord).

***************************************************************************/

#include "driver.h"
#include "common.h"
#include "usrintrf.h"
#include "vidhrdw/generic.h"
#include "neogeo.h"
#include "state.h"

static data16_t *neogeo_vidram16;
static data16_t *neogeo_paletteram16;	/* pointer to 1 of the 2 palette banks */
static data16_t *neogeo_palettebank[2]; /* 0x100*16 2 byte palette entries */
static int neogeo_palette_index;
static data16_t neogeo_vidram16_modulo;
static data16_t neogeo_vidram16_offset;
static int high_tile;
static int vhigh_tile;
static int vvhigh_tile;
static int no_of_tiles;
static int palette_swap_pending;
static int fix_bank;


static UINT8 *memory_region_gfx4;
static UINT8 *memory_region_gfx3;



extern unsigned int neogeo_frame_counter;

int neogeo_fix_bank_type;


/*
	X zoom table - verified on real hardware
	        8         = 0
	    4   8         = 1
	    4   8  c      = 2
	  2 4   8  c      = 3
	  2 4   8  c e    = 4
	  2 4 6 8  c e    = 5
	  2 4 6 8 a c e   = 6
	0 2 4 6 8 a c e   = 7
	0 2 4 6 89a c e   = 8
	0 234 6 89a c e   = 9
	0 234 6 89a c ef  = A
	0 234 6789a c ef  = B
	0 234 6789a cdef  = C
	01234 6789a cdef  = D
	01234 6789abcdef  = E
	0123456789abcdef  = F
*/
static char zoomx_draw_tables[16][16] =
{
	{ 0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0 },
	{ 0,0,0,0,1,0,0,0,1,0,0,0,0,0,0,0 },
	{ 0,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0 },
	{ 0,0,1,0,1,0,0,0,1,0,0,0,1,0,0,0 },
	{ 0,0,1,0,1,0,0,0,1,0,0,0,1,0,1,0 },
	{ 0,0,1,0,1,0,1,0,1,0,0,0,1,0,1,0 },
	{ 0,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0 },
	{ 1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0 },
	{ 1,0,1,0,1,0,1,0,1,1,1,0,1,0,1,0 },
	{ 1,0,1,1,1,0,1,0,1,1,1,0,1,0,1,0 },
	{ 1,0,1,1,1,0,1,0,1,1,1,0,1,0,1,1 },
	{ 1,0,1,1,1,0,1,1,1,1,1,0,1,0,1,1 },
	{ 1,0,1,1,1,0,1,1,1,1,1,0,1,1,1,1 },
	{ 1,1,1,1,1,0,1,1,1,1,1,0,1,1,1,1 },
	{ 1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1 },
	{ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 }
};



/******************************************************************************/

static void set_palettebank_on_postload(void)
{
	int i;

	neogeo_paletteram16 = neogeo_palettebank[neogeo_palette_index];

	for (i = 0; i < 0x2000 >> 1; i++)
	{
		data16_t newword = neogeo_paletteram16[i];
		int r,g,b;

		r = ((newword >> 7) & 0x1e) | ((newword >> 14) & 0x01);
		g = ((newword >> 3) & 0x1e) | ((newword >> 13) & 0x01);
		b = ((newword << 1) & 0x1e) | ((newword >> 12) & 0x01);

		r = (r << 3) | (r >> 2);
		g = (g << 3) | (g >> 2);
		b = (b << 3) | (b >> 2);

		palette_set_color(i, r, g, b);
	}
}

static void register_savestate(void)
{
	state_save_register_int   ("video", 0, "neogeo_palette_index",    &neogeo_palette_index);
	state_save_register_int   ("video", 0, "palette_swap_pending",    &palette_swap_pending);
	state_save_register_UINT16("video", 0, "neogeo_palettebank[0]",   neogeo_palettebank[0],    0x2000/2);
	state_save_register_UINT16("video", 0, "neogeo_palettebank[1]",   neogeo_palettebank[1],    0x2000/2);
	state_save_register_UINT16("video", 0, "neogeo_vidram16",         neogeo_vidram16,          0x20000/2);
	state_save_register_UINT16("video", 0, "neogeo_vidram16_modulo",  &neogeo_vidram16_modulo,  1);
	state_save_register_UINT16("video", 0, "neogeo_vidram16_offset",  &neogeo_vidram16_offset,  1);
	state_save_register_int   ("video", 0, "fix_bank",                &fix_bank);

	state_save_register_func_postload(set_palettebank_on_postload);
}

/******************************************************************************/

VIDEO_START( neogeo_mvs )
{
	no_of_tiles=Machine->gfx[2]->total_elements;
	if (no_of_tiles>0x10000) high_tile=1; else high_tile=0;
	if (no_of_tiles>0x20000) vhigh_tile=1; else vhigh_tile=0;
	if (no_of_tiles>0x40000) vvhigh_tile=1; else vvhigh_tile=0;

	neogeo_palettebank[0] = NULL;
	neogeo_palettebank[1] = NULL;
	neogeo_vidram16 = NULL;

	neogeo_palettebank[0] = auto_malloc(0x2000);
	if (!neogeo_palettebank[0])
		return 1;

	neogeo_palettebank[1] = auto_malloc(0x2000);
	if (!neogeo_palettebank[1])
		return 1;

	/* 0x20000 bytes even though only 0x10c00 is used */
	neogeo_vidram16 = auto_malloc(0x20000);
	if (!neogeo_vidram16)
		return 1;
	memset(neogeo_vidram16,0,0x20000);

	neogeo_paletteram16 = neogeo_palettebank[0];
	neogeo_palette_index = 0;
	neogeo_vidram16_modulo = 1;
	neogeo_vidram16_offset = 0;
	fix_bank = 0;
	palette_swap_pending = 0;

	memory_region_gfx4  = memory_region(REGION_GFX4);
	memory_region_gfx3  = memory_region(REGION_GFX3);


	register_savestate();

	return 0;
}

/******************************************************************************/

static void swap_palettes(void)
{
	int i;

	for (i = 0; i < 0x2000 >> 1; i++)
	{
		data16_t newword = neogeo_paletteram16[i];
		int r,g,b;

		r = ((newword >> 7) & 0x1e) | ((newword >> 14) & 0x01);
		g = ((newword >> 3) & 0x1e) | ((newword >> 13) & 0x01);
		b = ((newword << 1) & 0x1e) | ((newword >> 12) & 0x01);

		r = (r << 3) | (r >> 2);
		g = (g << 3) | (g >> 2);
		b = (b << 3) | (b >> 2);

		palette_set_color(i, r, g, b);
	}

	palette_swap_pending = 0;
}

static void neogeo_setpalbank(int n)
{
	if (neogeo_palette_index != n)
	{
		neogeo_palette_index = n;
		neogeo_paletteram16 = neogeo_palettebank[n];
		palette_swap_pending = 1;
	}
}

WRITE16_HANDLER( neogeo_setpalbank0_16_w )
{
	neogeo_setpalbank(0);
}

WRITE16_HANDLER( neogeo_setpalbank1_16_w )
{
	neogeo_setpalbank(1);
}

READ16_HANDLER( neogeo_paletteram16_r )
{
	return neogeo_paletteram16[offset];
}

WRITE16_HANDLER( neogeo_paletteram16_w )
{
	data16_t oldword, newword;
	int r,g,b;

	oldword = newword = neogeo_paletteram16[offset];
	COMBINE_DATA(&newword);

	if (oldword == newword)
		return;

	neogeo_paletteram16[offset] = newword;

	r = ((newword >> 7) & 0x1e) | ((newword >> 14) & 0x01);
	g = ((newword >> 3) & 0x1e) | ((newword >> 13) & 0x01) ;
	b = ((newword << 1) & 0x1e) | ((newword >> 12) & 0x01) ;

	r = (r << 3) | (r >> 2);
	g = (g << 3) | (g >> 2);
	b = (b << 3) | (b >> 2);

	palette_set_color(offset, r, g, b);
}

/******************************************************************************/

WRITE16_HANDLER( neogeo_vidram16_offset_w )
{
	COMBINE_DATA(&neogeo_vidram16_offset);
}

READ16_HANDLER( neogeo_vidram16_data_r )
{
	return neogeo_vidram16[neogeo_vidram16_offset];
}

WRITE16_HANDLER( neogeo_vidram16_data_w )
{
	COMBINE_DATA(&neogeo_vidram16[neogeo_vidram16_offset]);
	neogeo_vidram16_offset = (neogeo_vidram16_offset & 0x8000)	/* gururin fix */
			| ((neogeo_vidram16_offset + neogeo_vidram16_modulo) & 0x7fff);
}

/* Modulo can become negative , Puzzle Bobble Super Sidekicks and a lot */
/* of other games use this */
WRITE16_HANDLER( neogeo_vidram16_modulo_w )
{
	COMBINE_DATA(&neogeo_vidram16_modulo);
}

READ16_HANDLER( neogeo_vidram16_modulo_r )
{
	return neogeo_vidram16_modulo;
}


WRITE16_HANDLER( neo_board_fix_16_w )
{
	fix_bank = 1;
}

WRITE16_HANDLER( neo_game_fix_16_w )
{
	fix_bank = 0;
}

/******************************************************************************/


static void NeoMVSDrawGfxLine(UINT16 **line,const struct GfxElement *gfx,
		unsigned int code,unsigned int color,int flipx,int sx,int sy,
		int zx,int yoffs,const struct rectangle *clip)
{
	UINT16 *bm = line[sy]+sx;
	int col;
	int mydword;

	UINT8 *fspr = memory_region_gfx3;
	const pen_t *paldata = &gfx->colortable[gfx->color_granularity * color];

	if (sx <= -16) return;

	/* Safety feature */
	code %= no_of_tiles;

	/* Check for total transparency, no need to draw */
	if ((gfx->pen_usage[code] & ~1) == 0)
		return;

	fspr += code*128 + 8*yoffs;

	if (flipx)	/* X flip */
	{
		if (zx == 0x0f)
		{
			mydword = (fspr[4]<<0)|(fspr[5]<<8)|(fspr[6]<<16)|(fspr[7]<<24);
			col = (mydword>>28)&0xf; if (col) bm[ 0] = paldata[col];
			col = (mydword>>24)&0xf; if (col) bm[ 1] = paldata[col];
			col = (mydword>>20)&0xf; if (col) bm[ 2] = paldata[col];
			col = (mydword>>16)&0xf; if (col) bm[ 3] = paldata[col];
			col = (mydword>>12)&0xf; if (col) bm[ 4] = paldata[col];
			col = (mydword>> 8)&0xf; if (col) bm[ 5] = paldata[col];
			col = (mydword>> 4)&0xf; if (col) bm[ 6] = paldata[col];
			col = (mydword>> 0)&0xf; if (col) bm[ 7] = paldata[col];

			mydword = (fspr[0]<<0)|(fspr[1]<<8)|(fspr[2]<<16)|(fspr[3]<<24);
			col = (mydword>>28)&0xf; if (col) bm[ 8] = paldata[col];
			col = (mydword>>24)&0xf; if (col) bm[ 9] = paldata[col];
			col = (mydword>>20)&0xf; if (col) bm[10] = paldata[col];
			col = (mydword>>16)&0xf; if (col) bm[11] = paldata[col];
			col = (mydword>>12)&0xf; if (col) bm[12] = paldata[col];
			col = (mydword>> 8)&0xf; if (col) bm[13] = paldata[col];
			col = (mydword>> 4)&0xf; if (col) bm[14] = paldata[col];
			col = (mydword>> 0)&0xf; if (col) bm[15] = paldata[col];
		}
		else
		{
			char *zoomx_draw = zoomx_draw_tables[zx];

			mydword = (fspr[4]<<0)|(fspr[5]<<8)|(fspr[6]<<16)|(fspr[7]<<24);
			if (zoomx_draw[ 0]) { col = (mydword>>28)&0xf; if (col) *bm = paldata[col]; bm++; }
			if (zoomx_draw[ 1]) { col = (mydword>>24)&0xf; if (col) *bm = paldata[col]; bm++; }
			if (zoomx_draw[ 2]) { col = (mydword>>20)&0xf; if (col) *bm = paldata[col]; bm++; }
			if (zoomx_draw[ 3]) { col = (mydword>>16)&0xf; if (col) *bm = paldata[col]; bm++; }
			if (zoomx_draw[ 4]) { col = (mydword>>12)&0xf; if (col) *bm = paldata[col]; bm++; }
			if (zoomx_draw[ 5]) { col = (mydword>> 8)&0xf; if (col) *bm = paldata[col]; bm++; }
			if (zoomx_draw[ 6]) { col = (mydword>> 4)&0xf; if (col) *bm = paldata[col]; bm++; }
			if (zoomx_draw[ 7]) { col = (mydword>> 0)&0xf; if (col) *bm = paldata[col]; bm++; }

			mydword = (fspr[0]<<0)|(fspr[1]<<8)|(fspr[2]<<16)|(fspr[3]<<24);
			if (zoomx_draw[ 8]) { col = (mydword>>28)&0xf; if (col) *bm = paldata[col]; bm++; }
			if (zoomx_draw[ 9]) { col = (mydword>>24)&0xf; if (col) *bm = paldata[col]; bm++; }
			if (zoomx_draw[10]) { col = (mydword>>20)&0xf; if (col) *bm = paldata[col]; bm++; }
			if (zoomx_draw[11]) { col = (mydword>>16)&0xf; if (col) *bm = paldata[col]; bm++; }
			if (zoomx_draw[12]) { col = (mydword>>12)&0xf; if (col) *bm = paldata[col]; bm++; }
			if (zoomx_draw[13]) { col = (mydword>> 8)&0xf; if (col) *bm = paldata[col]; bm++; }
			if (zoomx_draw[14]) { col = (mydword>> 4)&0xf; if (col) *bm = paldata[col]; bm++; }
			if (zoomx_draw[15]) { col = (mydword>> 0)&0xf; if (col) *bm = paldata[col]; bm++; }
		}
	}
	else		/* normal */
	{
		if (zx == 0x0f)		/* fixed */
		{
			mydword = (fspr[0]<<0)|(fspr[1]<<8)|(fspr[2]<<16)|(fspr[3]<<24);
			col = (mydword>> 0)&0xf; if (col) bm[ 0] = paldata[col];
			col = (mydword>> 4)&0xf; if (col) bm[ 1] = paldata[col];
			col = (mydword>> 8)&0xf; if (col) bm[ 2] = paldata[col];
			col = (mydword>>12)&0xf; if (col) bm[ 3] = paldata[col];
			col = (mydword>>16)&0xf; if (col) bm[ 4] = paldata[col];
			col = (mydword>>20)&0xf; if (col) bm[ 5] = paldata[col];
			col = (mydword>>24)&0xf; if (col) bm[ 6] = paldata[col];
			col = (mydword>>28)&0xf; if (col) bm[ 7] = paldata[col];

			mydword = (fspr[4]<<0)|(fspr[5]<<8)|(fspr[6]<<16)|(fspr[7]<<24);
			col = (mydword>> 0)&0xf; if (col) bm[ 8] = paldata[col];
			col = (mydword>> 4)&0xf; if (col) bm[ 9] = paldata[col];
			col = (mydword>> 8)&0xf; if (col) bm[10] = paldata[col];
			col = (mydword>>12)&0xf; if (col) bm[11] = paldata[col];
			col = (mydword>>16)&0xf; if (col) bm[12] = paldata[col];
			col = (mydword>>20)&0xf; if (col) bm[13] = paldata[col];
			col = (mydword>>24)&0xf; if (col) bm[14] = paldata[col];
			col = (mydword>>28)&0xf; if (col) bm[15] = paldata[col];
		}
		else
		{
			char *zoomx_draw = zoomx_draw_tables[zx];

			mydword = (fspr[0]<<0)|(fspr[1]<<8)|(fspr[2]<<16)|(fspr[3]<<24);
			if (zoomx_draw[ 0]) { col = (mydword>> 0)&0xf; if (col) *bm = paldata[col]; bm++; }
			if (zoomx_draw[ 1]) { col = (mydword>> 4)&0xf; if (col) *bm = paldata[col]; bm++; }
			if (zoomx_draw[ 2]) { col = (mydword>> 8)&0xf; if (col) *bm = paldata[col]; bm++; }
			if (zoomx_draw[ 3]) { col = (mydword>>12)&0xf; if (col) *bm = paldata[col]; bm++; }
			if (zoomx_draw[ 4]) { col = (mydword>>16)&0xf; if (col) *bm = paldata[col]; bm++; }
			if (zoomx_draw[ 5]) { col = (mydword>>20)&0xf; if (col) *bm = paldata[col]; bm++; }
			if (zoomx_draw[ 6]) { col = (mydword>>24)&0xf; if (col) *bm = paldata[col]; bm++; }
			if (zoomx_draw[ 7]) { col = (mydword>>28)&0xf; if (col) *bm = paldata[col]; bm++; }

			mydword = (fspr[4]<<0)|(fspr[5]<<8)|(fspr[6]<<16)|(fspr[7]<<24);
			if (zoomx_draw[ 8]) { col = (mydword>> 0)&0xf; if (col) *bm = paldata[col]; bm++; }
			if (zoomx_draw[ 9]) { col = (mydword>> 4)&0xf; if (col) *bm = paldata[col]; bm++; }
			if (zoomx_draw[10]) { col = (mydword>> 8)&0xf; if (col) *bm = paldata[col]; bm++; }
			if (zoomx_draw[11]) { col = (mydword>>12)&0xf; if (col) *bm = paldata[col]; bm++; }
			if (zoomx_draw[12]) { col = (mydword>>16)&0xf; if (col) *bm = paldata[col]; bm++; }
			if (zoomx_draw[13]) { col = (mydword>>20)&0xf; if (col) *bm = paldata[col]; bm++; }
			if (zoomx_draw[14]) { col = (mydword>>24)&0xf; if (col) *bm = paldata[col]; bm++; }
			if (zoomx_draw[15]) { col = (mydword>>28)&0xf; if (col) *bm = paldata[col]; bm++; }
		}
	}
}



/******************************************************************************/

VIDEO_UPDATE( neogeo )
{
	int sx =0,sy =0,my =0,zx = 0x0f, zy = 0xff;
	int offs,count;
	int tileno,tileatr,t1,t2,t3;
	char fullmode = 0;
	void **line=bitmap->line;
	unsigned int *pen_usage;
	struct GfxElement *gfx=Machine->gfx[2]; /* Save constant struct dereference */

profiler_mark(PROFILER_VIDEO);

	/* Palette swap occured after last frame but before this one */
	if (palette_swap_pending) swap_palettes();

	fillbitmap(bitmap,Machine->pens[4095],cliprect);

	/* Draw sprites */
	for (count = 0; count < 0x300 >> 1; count++)
	{
		UINT8 *zoomy_rom;
		int drawn_lines;

		t3 = neogeo_vidram16[(0x10000 >> 1) + count];
		t1 = neogeo_vidram16[(0x10400 >> 1) + count];


		/* If this bit is set this new column is placed next to last one */
		if (t1 & 0x40)
		{
			sx += zx+1;
			if ( sx >= 0x1F0 )
				sx -= 0x200;

			/* Get new zoom for this column */
			zx = (t3 >> 8) & 0x0f;
		}
		else /* nope it is a new block */
		{
			/* Sprite scaling */
			t2 = neogeo_vidram16[(0x10800 >> 1) + count];
			zx = (t3 >> 8) & 0x0f;
			zy = t3 & 0xff;

			sx = (t2 >> 7);
			if ( sx >= 0x1F0 )
				sx -= 0x200;

			/* Number of tiles in this strip */
			my = t1 & 0x3f;

			sy = 0x200 - (t1 >> 7);

			if (my > 0x20)
			{
				my = 0x20;

				/* if (sy < 248) */
					fullmode = 1;
				/* else */
					/* kludge to avoid a white line in KOF94 Japan stage... */
					/* probably indication of a subtle bug somewhere else */
				/*	fullmode = 0; */
			}
			else
				fullmode = 0;
		}

		/* No point doing anything if tile strip is 0 */
		if (my==0) continue;

		if (sx >= 320)
			continue;

		offs = count<<6;

		/* get pointer to table in zoom ROM (thanks to Miguel Angel Horna for the info) */
		zoomy_rom = memory_region_gfx4 + (zy << 8);
		drawn_lines = 0;

		/* my holds the number of tiles in each vertical multisprite block */
		while (drawn_lines < my*0x10)
		{
			int yy = (sy + drawn_lines) & 0x1ff;

			if (yy >= cliprect->min_y && yy <= cliprect->max_y)
			{
				int tile,yoffs;
				int zoom_line;
				int invert=0;

				zoom_line = drawn_lines & 0xff;
				if (drawn_lines & 0x100)
				{
					zoom_line ^= 0xff;
					invert = 1;
				}
				if (fullmode)	/* fix for joyjoy, trally... */
				{
					if (zy)
					{
						zoom_line %= 2*(zy+1);	/* fixed */
						if (zoom_line >= (zy+1))	/* fixed */
						{
							zoom_line = 2*(zy+1)-1 - zoom_line;	/* fixed */
							invert ^= 1;
						}
					}
				}

				yoffs = zoomy_rom[zoom_line] & 0x0f;

				tile = zoomy_rom[zoom_line] >> 4;

				if (invert)
				{
					tile ^= 0x1f;
					yoffs ^= 0x0f;
				}

				tileno	= neogeo_vidram16[offs+2*tile];
				tileatr = neogeo_vidram16[offs+2*tile+1];

				if (  high_tile && (tileatr & 0x10)) tileno+=0x10000;
				if ( vhigh_tile && (tileatr & 0x20)) tileno+=0x20000;
				if (vvhigh_tile && (tileatr & 0x40)) tileno+=0x40000;

				if (tileatr & 0x08) tileno=(tileno&~7)|(neogeo_frame_counter&7);	/* fixed */
				else if (tileatr & 0x04) tileno=(tileno&~3)|(neogeo_frame_counter&3);	/* fixed */

				if (tileatr & 0x02) yoffs ^= 0x0f;	/* flip y */

				NeoMVSDrawGfxLine((UINT16 **)line,
					gfx,
					tileno,
					tileatr >> 8,
					tileatr & 0x01,	/* flip x */
					sx,yy,zx,yoffs,
					cliprect
				);
			}

			drawn_lines++;
		}  /* for y */
	}  /* for count */



	/* Save some struct de-refs */
	gfx = Machine->gfx[fix_bank];
	pen_usage=gfx->pen_usage;

	/* Character foreground */
	/* thanks to Mr K for the garou & kof2000 banking info */
	{
		int y,x;
		int banked;
		int garouoffsets[32];

		banked = (fix_bank == 0 && Machine->gfx[0]->total_elements > 0x1000) ? 1 : 0;

		/* Build line banking table for Garou & MS3 before starting render */
		if (banked && neogeo_fix_bank_type == 1)
		{
			int garoubank = 0;
			int k = 0;
			y = 0;
			while (y < 32)
			{
				if (neogeo_vidram16[(0xea00>>1)+k] == 0x0200 && (neogeo_vidram16[(0xeb00>>1)+k] & 0xff00) == 0xff00)
				{
					garoubank = neogeo_vidram16[(0xeb00>>1)+k] & 3;
					garouoffsets[y++] = garoubank;
				}
				garouoffsets[y++] = garoubank;
				k += 2;
			}
		}

		if (banked)
		{
			for (y=cliprect->min_y / 8; y <= cliprect->max_y / 8; y++)
			{
				for (x = 0; x < 40; x++)
				{
					int byte1 = neogeo_vidram16[(0xe000 >> 1) + y + 32 * x];
					int byte2 = byte1 >> 12;
					byte1 = byte1 & 0xfff;

					switch (neogeo_fix_bank_type)
					{
						case 1:
							/* Garou, MSlug 3 */
							byte1 += 0x1000 * (garouoffsets[(y-2)&31] ^ 3);
							break;
						case 2:
							byte1 += 0x1000 * (((neogeo_vidram16[(0xea00 >> 1) + ((y-1)&31) + 32 * (x/6)] >> (5-(x%6))*2) & 3) ^ 3);
							break;
					}


					if ((pen_usage[byte1] & ~1) == 0) continue;

					drawgfx(bitmap,gfx,
							byte1,
							byte2,
							0,0,
							x*8,y*8,
							cliprect,TRANSPARENCY_PEN,0);
				}
			}
		} /* Banked	*/
		else
		{
			for (y=cliprect->min_y / 8; y <= cliprect->max_y / 8; y++)
			{
				for (x = 0; x < 40; x++)
				{
					int byte1 = neogeo_vidram16[(0xe000 >> 1) + y + 32 * x];
					int byte2 = byte1 >> 12;
					byte1 = byte1 & 0xfff;

					if ((pen_usage[byte1] & ~1) == 0) continue;

					drawgfx(bitmap,gfx,
							byte1,
							byte2,
							0,0,
							x*8,y*8,
							cliprect,TRANSPARENCY_PEN,0);
							/* x = x +8; */
				}
			}
		} /* Not banked */
	}


profiler_mark(PROFILER_END);
}
