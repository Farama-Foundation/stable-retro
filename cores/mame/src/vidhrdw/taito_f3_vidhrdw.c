/***************************************************************************

   Taito F3 Video emulation - Bryan McPhail, mish@tendril.co.uk

****************************************************************************

Brief overview:

	4 scrolling layers (512x512 or 1024x512) of 4/5/6 bpp tiles.
	1 scrolling text layer (512x512, characters generated in vram), 4bpp chars.
	1 scrolling pixel layer (512x256 pixels generated in pivot ram), 4bpp pixels.
	2 sprite banks (for double buffering of sprites)
	Sprites can be 4, 5 or 6 bpp
	Sprite scaling.
	Rowscroll on all playfields
	Line by line zoom on all playfields
	Column scroll on all playfields
	Line by line sprite and playfield priority mixing

Notes:
	All special effects are controlled by an area in 'line ram'.  Typically
	there are 256 values, one for each line of the screen (including clipped
	lines at the top of the screen).  For example, at 0x8000 in lineram,
	there are 4 sets of 256 values (one for each playfield) and each value
	is the scale control for that line in the destination bitmap (screen).
	Therefore each line can have a different zoom value for special effects.

	This also applies to playfield priority, rowscroll, column scroll, sprite
	priority and VRAM/pivot layers.

	However - at the start of line ram there are also sets of 256 values
	controlling each effect - effects can be selectively applied to individual
	playfields or only certain lines out of the 256 can be active - in which
	case the last allowed value can be latched (typically used so a game can
	use one zoom or row value over the whole playfield).

	The programmers of some of these games made strange use of flipscreen -
	some games have all their graphics flipped in ROM, and use the flipscreen
	bit to display them correctly!.

	Most games display 232 scanlines, but some use lineram effects to clip
	themselves to 224 or less.

****************************************************************************

Line ram memory map:

	Here 'playfield 1' refers to the first playfield in memory, etc

	0x0000: Column line control ram (256 lines)
		100x:	Where bit 0 of x enables effect on playfield 1
				Where bit 1 of x enables effect on playfield 2
				Where bit 2 of x enables effect on playfield 3
				Where bit 3 of x enables effect on playfield 4
	0x0200: Line control ram for 0x5000 section.
	0x0400: Line control ram for 0x6000 section.
		(Alpha control)
	0x0600: Sprite control ram
		1c0x:	Where x enables sprite control word for that line
	0x0800: Zoom line control ram (256 lines)
		200x:	Where bit 0 of x enables effect on playfield 1
				Where bit 1 of x enables effect on playfield 2
				Where bit 2 of x enables effect on playfield 3
				Where bit 3 of x enables effect on playfield 4
	0x0a00: Assumed unused.
	0x0c00: Rowscroll line control ram (256 lines)
		280x:	Where bit 0 of x enables effect on playfield 1
				Where bit 1 of x enables effect on playfield 2
				Where bit 2 of x enables effect on playfield 3
				Where bit 3 of x enables effect on playfield 4
	0x0e00: Priority line control ram (256 lines)
		2c0x:	Where bit 0 of x enables effect on playfield 1
				Where bit 1 of x enables effect on playfield 2
				Where bit 2 of x enables effect on playfield 3
				Where bit 3 of x enables effect on playfield 4

	0x4000: Playfield 1 column scroll (on source bitmap, not destination)
	0x4200: Playfield 2 column scroll (on source bitmap, not destination)
	0x4400: Playfield 3 column scroll (on source bitmap, not destination)
	0x4600: Playfield 4 column scroll (on source bitmap, not destination)

	0x5000: ?????

	0x6000:	Pivot layer control
		Cupfinal a255 display, 0255 don't
		Bubsymph a255 display  0255 don't
		Pbobbl4u a2ff display (colour 0xa) 02ff don't
		Landmakr 00df dont display
		qtheater 01ff dont display
		0x00p0 - p = priority? (See BubSymph continue screen)
	0x6200: Alpha blending control
		Cupfinal 3000
		Landmakr bbbb
		qtheater bbbb
		bubsymph 3000
	0x6400: Pivot layer control?
		Cupfinal 7000
		landmakr 7000
		qtheater 7000
		bubsymph 7000
		pbobbl4u 7000
	0x6600: Always zero?

	0x7000: ?
	0x7200: ?
	0x7400: ?
	0x7600: Sprite priority values
		0xf000:	Relative priority for sprites with pri value 0xc0
		0x0f00:	Relative priority for sprites with pri value 0x80
		0x00f0:	Relative priority for sprites with pri value 0x40
		0x000f:	Relative priority for sprites with pri value 0x00

	0x8000: Playfield 1 scale (1 word per line, 256 lines, 0x80 = no scale)
	0x8200: Playfield 2 scale
	0x8400: Playfield 3 scale
	0x8600: Playfield 4 scale
		0x0080 = No scale
		< 0x80 = Zoom Out
		> 0x80 = Zoom in

	0xa000: Playfield 1 rowscroll (1 word per line, 256 lines)
	0xa200: Playfield 2 rowscroll
	0xa400: Playfield 3 rowscroll
	0xa600: Playfield 4 rowscroll

	0xb000: Playfield 1 priority (1 word per line, 256 lines)
	0xb200: Playfield 2 priority
	0xb400: Playfield 3 priority
	0xb600: Playfield 4 priority
		0xf000 = Disable playfield (ElvAct2 second level)
		0x8000 = Enable alpha-blending for this line
		0x4000 = Playfield can be alpha-blended against?  (Otherwise, playfield shouldn't be in blend calculation?)
		0x2000 = Enable line? (Darius Gaiden 0x5000 = disable, 0x3000, 0xb000 & 0x7000 = display)
		0x1000 = ?
		0x0800 = Disable line (Used by KTiger2 to clip screen)
		0x07f0 = ?
		0x000f = Playfield priority

	0xc000 - 0xffff: Unused.

	When sprite priority==playfield priority sprite takes precedence (Bubble Symph title)

****************************************************************************

	F3 sprite format:

	Word 0:	0xffff		Tile number (LSB)
	Word 1:	0xff00		X zoom
			0x00ff		Y zoom
	Word 2:	0x03ff		X position
	Word 3:	0x03ff		Y position
	Word 4:	0xf000		Sprite block controls
			0x0800		Sprite block start
			0x0400		Use same colour on this sprite as block start
			0x0200		Y flip
			0x0100		X flip
			0x00ff		Colour
	Word 5: 0xffff		Tile number (MSB), probably only low bits used
	Word 6:	0x8000		If set, jump to sprite location in low bits
			0x03ff		Location to jump to.
	Word 7: 0xffff		Unused?  Always zero?

****************************************************************************

	Playfield control information (0x660000-1f):

	Word 0- 3: X scroll values for the 4 playfields.
	Word 4- 7: Y scroll values for the 4 playfields.
	Word 8-11: Unused.  Always zero.
	Word   12: Pixel + VRAM playfields X scroll
	Word   13: Pixel + VRAM playfields Y scroll
	Word   14: Unused. Always zero.
	Word   15: If set to 0x80, then 1024x512 playfields are used, else 512x512

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "taito_f3.h"
#include "state.h"

#define DARIUSG_KLUDGE
#define DEBUG_F3 0

static struct tilemap *pf1_tilemap,*pf2_tilemap,*pf3_tilemap,*pf4_tilemap;
static struct tilemap *pixel_layer;
static data32_t *spriteram32_buffered;
static int vram_dirty[256];
static int pivot_changed,vram_changed,scroll_kludge_y,scroll_kludge_x;
static data32_t f3_control_0[8];
static data32_t f3_control_1[8];
static int flipscreen;

static UINT8 *pivot_dirty;
static int pf23_y_kludge;
static struct rectangle pixel_layer_clip;

static data32_t *f3_pf_data_1,*f3_pf_data_2,*f3_pf_data_3,*f3_pf_data_4;

data32_t *f3_vram,*f3_line_ram;
data32_t *f3_pf_data,*f3_pivot_ram;

extern int f3_game;
#if DEBUG_F3
static int sprite_pri_word;
#endif	/*DEBUG_F3*/
static int scroll_dirty,skip_this_frame;

/* Game specific data, some of this can be
removed when the software values are figured out */
struct F3config
{
	int name;
	int extend;
	int sx;
	int sy;
	int pivot;
	int sprite_lag;
};

const struct F3config *f3_game_config;

static const struct F3config f3_config_table[] =
{
	/* Name    Extend  X Offset, Y Offset  Flip Pivot Sprite Lag */  /* Uses 5000, uses control bits,works with line X zoom */
/**/{ RINGRAGE,  0,      0,        -30,         0,          2    }, /*no,no,no*/
	{ ARABIANM,  0,      0,        -30,         0,          2    }, /*ff00 in 5000, no,yes*/
/**/{ RIDINGF,   1,      0,        -30,         0,          1    }, /*yes,yes,yes*/
	{ GSEEKER,   0,      1,        -30,         0,          1    }, /*yes,yes,yes*/
	{ TRSTAR,    1,      0,          0,         0,          0    }, /**/
	{ GUNLOCK,   1,      1,        -30,         0,          2    }, /*yes,yes,partial*/
/**/{ TWINQIX,   1,      0,        -30,         1,          1    },
	{ SCFINALS,  0,      0,        -30,         1,          1/**/},
	{ LIGHTBR,   1,      0,        -30,         0,          2    }, /*yes,?,no*/
	{ KAISERKN,  0,      0,        -30,         1,          2    },
	{ DARIUSG,   0,      0,        -23,         0,          2    },
	{ BUBSYMPH,  1,      0,        -30,         0,          1/**/}, /*yes,yes,?*/
	{ SPCINVDX,  1,      0,        -30,         1,          1/**/}, /*yes,yes,?*/
	{ QTHEATER,  1,      0,          0,         0,          1/**/},
	{ HTHERO95,  0,      0,        -30,         1,          1/**/},
	{ SPCINV95,  0,      0,        -30,         0,          1/**/},
	{ EACTION2,  1,      0,        -23,         0,          2    }, /*no,yes,?*/
	{ QUIZHUHU,  1,      0,        -23,         0,          1/**/},
	{ PBOBBLE2,  0,      0,          0,         1,          1/**/}, /*no,no,?*/
	{ GEKIRIDO,  0,      0,        -23,         0,          1    },
	{ KTIGER2,   0,      0,        -23,         0,          0    },/*no,yes,partial*/
	{ BUBBLEM,   1,      0,        -30,         0,          1/**/},
	{ CLEOPATR,  0,      0,        -30,         0,          1/**/},
	{ PBOBBLE3,  0,      0,          0,         0,          1/**/},
	{ ARKRETRN,  1,      0,        -23,         0,          1/**/},
	{ KIRAMEKI,  0,      0,        -30,         0,          1/**/},
	{ PUCHICAR,  1,      0,        -23,         0,          1/**/},
	{ PBOBBLE4,  0,      0,          0,         1,          1/**/},
	{ POPNPOP,   1,      0,        -23,         0,          1/**/},
	{ LANDMAKR,  1,      0,        -23,         0,          1/**/},
	{ RECALH,    1,      1,        -23,         0,          1/**/}, /* not verified */
	{ COMMANDW,  1,      0,        -23,         0,          1/**/}, /* not verified */
	{0}
};


struct tempsprite
{
	int code,color;
	int flipx,flipy;
	int x,y;
	int zoomx,zoomy;
	int pri;
};
static struct tempsprite *spritelist;

static int alpha_disable=0;
#if DEBUG_F3
static char deb_buf[10][80];
static int deb_enable=0;
static int deb_tileflag=0;
static int deb_tile_code=0;
#endif	/*DEBUG_F3*/

static const struct tempsprite *sprite_end;
static void get_sprite_info(const data32_t *spriteram32_ptr);
static int sprite_lag=1;
static UINT8 sprite_pri_usage=0;

struct f3_line_inf
{
	int alpha_mode[256];
	int alpha_level[256];
	int pri[256];
	int spri[256];
	UINT16 sprite_alpha[256];

	/* use for draw_scanlines */
	UINT16 *src[256],*src_s[256],*src_e[256];
	UINT8 *tsrc[256],*tsrc_s[256];
	int x_count[256];
	UINT32 x_zoom[256];
};
/*
alpha_mode
---- --xx    0:disable 1:nomal 2:alpha 7000 3:alpha b000
---1 ----    alpha level a
--1- ----    alpha level b
1--------    opaque line
*/

static struct f3_line_inf *line_inf;
static struct mame_bitmap *pri_alp_bitmap;
/*
pri_alp_bitmap
---- ---1    sprite priority 0
---- --1-    sprite priority 1
---- -1--    sprite priority 2
---- 1---    sprite priority 3
---1 ----    alpha level a 7000
--1- ----    alpha level b 7000
-1-- ----    alpha level a b000
1--- ----    alpha level b b000
1111 1111    opaque pixcel
*/
static int f3_alpha_level_2as=127;
static int f3_alpha_level_2ad=127;
static int f3_alpha_level_3as=127;
static int f3_alpha_level_3ad=127;
static int f3_alpha_level_2bs=127;
static int f3_alpha_level_2bd=127;
static int f3_alpha_level_3bs=127;
static int f3_alpha_level_3bd=127;

static void init_alpha_blend_func(void);

static int width_mask=0x1ff;
static int twidth_mask=0x1f,twidth_mask_bit=5;
static UINT8 *tile_opaque_sp;
static UINT8 *tile_opaque_pf;


/******************************************************************************/

#if DEBUG_F3
static void print_debug_info(int t0, int t1, int t2, int t3, int c0, int c1, int c2, int c3)
{
	struct mame_bitmap *bitmap = Machine->scrbitmap;
	int j,trueorientation,l[16];
	char buf[64];

	trueorientation = Machine->orientation;
	Machine->orientation = ROT0;

	sprintf(buf,"%04X %04X %04X %04X",f3_control_0[0]>>22,(f3_control_0[0]&0xffff)>>6,f3_control_0[1]>>22,(f3_control_0[1]&0xffff)>>6);
	for (j = 0;j< 16+3;j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,40,0,TRANSPARENCY_NONE,0);
	sprintf(buf,"%04X %04X %04X %04X",f3_control_0[2]>>23,(f3_control_0[2]&0xffff)>>7,f3_control_0[3]>>23,(f3_control_0[3]&0xffff)>>7);
	for (j = 0;j< 16+3;j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,48,0,TRANSPARENCY_NONE,0);
	sprintf(buf,"%04X %04X %04X %04X",f3_control_1[0]>>16,f3_control_1[0]&0xffff,f3_control_1[1]>>16,f3_control_1[1]&0xffff);
	for (j = 0;j< 16+3;j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,58,0,TRANSPARENCY_NONE,0);
	sprintf(buf,"%04X %04X %04X %04X",f3_control_1[2]>>16,f3_control_1[2]&0xffff,f3_control_1[3]>>16,f3_control_1[3]&0xffff);
	for (j = 0;j< 16+3;j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,66,0,TRANSPARENCY_NONE,0);

	sprintf(buf,"%04X %04X %04X %04X %04X %04X %04X %04X",spriteram32_buffered[0]>>16,spriteram32_buffered[0]&0xffff,spriteram32_buffered[1]>>16,spriteram32_buffered[1]&0xffff,spriteram32_buffered[2]>>16,spriteram32_buffered[2]&0xffff,spriteram32_buffered[3]>>16,spriteram32_buffered[3]&0xffff);
	for (j = 0;j< 32+7;j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,76,0,TRANSPARENCY_NONE,0);
	sprintf(buf,"%04X %04X %04X %04X %04X %04X %04X %04X",spriteram32_buffered[4]>>16,spriteram32_buffered[4]&0xffff,spriteram32_buffered[5]>>16,spriteram32_buffered[5]&0xffff,spriteram32_buffered[6]>>16,spriteram32_buffered[6]&0xffff,spriteram32_buffered[7]>>16,spriteram32_buffered[7]&0xffff);
	for (j = 0;j< 32+7;j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,84,0,TRANSPARENCY_NONE,0);
	sprintf(buf,"%04X %04X %04X %04X %04X %04X %04X %04X",spriteram32_buffered[8]>>16,spriteram32_buffered[8]&0xffff,spriteram32_buffered[9]>>16,spriteram32_buffered[9]&0xffff,spriteram32_buffered[10]>>16,spriteram32_buffered[10]&0xffff,spriteram32_buffered[11]>>16,spriteram32_buffered[11]&0xffff);
	for (j = 0;j< 32+7;j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,92,0,TRANSPARENCY_NONE,0);

	l[0]=f3_line_ram[0x0040]&0xffff;
	l[1]=f3_line_ram[0x00c0]&0xffff;
	l[2]=f3_line_ram[0x0140]&0xffff;
	l[3]=f3_line_ram[0x01c0]&0xffff;
	sprintf(buf,"Ctr1: %04x %04x %04x %04x",l[0],l[1],l[2],l[3]);
	for (j = 0;j < 16+9; j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,8*13,0,TRANSPARENCY_NONE,0);

	l[0]=f3_line_ram[0x0240]&0xffff;
	l[1]=f3_line_ram[0x02c0]&0xffff;
	l[2]=f3_line_ram[0x0340]&0xffff;
	l[3]=f3_line_ram[0x03c0]&0xffff;
	sprintf(buf,"Ctr2: %04x %04x %04x %04x",l[0],l[1],l[2],l[3]);
	for (j = 0;j < 16+9; j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,8*14,0,TRANSPARENCY_NONE,0);

	l[0]=f3_line_ram[0x2c60]&0xffff;
	l[1]=f3_line_ram[0x2ce0]&0xffff;
	l[2]=f3_line_ram[0x2d60]&0xffff;
	l[3]=f3_line_ram[0x2de0]&0xffff;
	sprintf(buf,"Pri : %04x %04x %04x %04x",l[0],l[1],l[2],l[3]);
	for (j = 0;j < 16+9; j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,8*15,0,TRANSPARENCY_NONE,0);

	l[0]=f3_line_ram[0x2060]&0xffff;
	l[1]=f3_line_ram[0x20e0]&0xffff;
	l[2]=f3_line_ram[0x2160]&0xffff;
	l[3]=f3_line_ram[0x21e0]&0xffff;
	sprintf(buf,"Zoom: %04x %04x %04x %04x",l[0],l[1],l[2],l[3]);
	for (j = 0;j < 16+9; j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,8*16,0,TRANSPARENCY_NONE,0);

	l[0]=f3_line_ram[0x2860]&0xffff;
	l[1]=f3_line_ram[0x28e0]&0xffff;
	l[2]=f3_line_ram[0x2960]&0xffff;
	l[3]=f3_line_ram[0x29e0]&0xffff;
	sprintf(buf,"Line: %04x %04x %04x %04x",l[0],l[1],l[2],l[3]);
	for (j = 0;j < 16+9; j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,8*17,0,TRANSPARENCY_NONE,0);

	l[0]=f3_line_ram[0x1c60]&0xffff;
	l[1]=f3_line_ram[0x1ce0]&0xffff;
	l[2]=f3_line_ram[0x1d60]&0xffff;
	l[3]=f3_line_ram[0x1de0]&0xffff;
	sprintf(buf,"Sprt: %04x %04x %04x %04x",l[0],l[1],l[2],l[3]);
	for (j = 0;j < 16+9; j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,8*18,0,TRANSPARENCY_NONE,0);

	l[0]=f3_line_ram[0x1860]&0xffff;
	l[1]=f3_line_ram[0x18e0]&0xffff;
	l[2]=f3_line_ram[0x1960]&0xffff;
	l[3]=f3_line_ram[0x19e0]&0xffff;
	sprintf(buf,"Pivt: %04x %04x %04x %04x",l[0],l[1],l[2],l[3]);
	for (j = 0;j < 16+9; j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,8*19,0,TRANSPARENCY_NONE,0);

	l[0]=f3_line_ram[0x1060]&0xffff;
	l[1]=f3_line_ram[0x10e0]&0xffff;
	l[2]=f3_line_ram[0x1160]&0xffff;
	l[3]=f3_line_ram[0x11e0]&0xffff;
	sprintf(buf,"Colm: %04x %04x %04x %04x",l[0],l[1],l[2],l[3]);
	for (j = 0;j < 16+9; j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,8*20,0,TRANSPARENCY_NONE,0);

	l[0]=f3_line_ram[0x1460]&0xffff;
	l[1]=f3_line_ram[0x14e0]&0xffff;
	l[2]=f3_line_ram[0x1560]&0xffff;
	l[3]=f3_line_ram[0x15e0]&0xffff;
	sprintf(buf,"5000: %04x %04x %04x %04x",l[0],l[1],l[2],l[3]);
	for (j = 0;j < 16+9; j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,8*21,0,TRANSPARENCY_NONE,0);

	sprintf(buf,"SPri: %04x",sprite_pri_word);
	for (j = 0;j < 10; j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,8*23,0,TRANSPARENCY_NONE,0);
	sprintf(buf,"TPri: %04x %04x %04x %04x",t0,t1,t2,t3);
	for (j = 0;j < 16+9; j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,8*24,0,TRANSPARENCY_NONE,0);
	sprintf(buf,"Cstm: %04x %04x %04x %04x",c0,c1,c2,c3);
	for (j = 0;j < 16+9; j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,8*25,0,TRANSPARENCY_NONE,0);
	sprintf(buf,"6000: %08x %08x %08x",f3_line_ram[0x1800],f3_line_ram[0x1890],f3_line_ram[0x1910]);
	for (j = 0;j < 16+9; j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,8*27,0,TRANSPARENCY_NONE,0);

	Machine->orientation = trueorientation;
}
#endif	/*DEBUG_F3*/

/******************************************************************************/

static INLINE void get_tile_info(int tile_index, data32_t *gfx_base)
{
	data32_t tile=gfx_base[tile_index];
	UINT8 abtype=(tile>>(16+9))&0x1f;

#if DEBUG_F3
/*if((tile&0xffff)>Machine->gfx[1]->total_elements) log_cb(RETRO_LOG_DEBUG, LOGPRE "tile code:%04x\n",tile&0xffff);*/
	if(deb_tileflag)
	{
		int c=tile&0xffff;
		if(abtype==deb_tile_code) c=0;

		SET_TILE_INFO(
				1,
				c,
				(tile>>16)&0x1ff,
				TILE_FLIPYX( tile >> 30 ))
		tile_info.priority = abtype&1;		/* alpha blending type? */
	}
	else
#endif	/*DEBUG_F3*/
	{
		SET_TILE_INFO(
				1,
				tile&0xffff,
				(tile>>16)&0x1ff,
				TILE_FLIPYX( tile >> 30 ))
		tile_info.priority =  abtype&1;		/* alpha blending type? */
	}
}

static void get_tile_info1(int tile_index)
{
	get_tile_info(tile_index,f3_pf_data_1);
}

static void get_tile_info2(int tile_index)
{
	get_tile_info(tile_index,f3_pf_data_2);
}

static void get_tile_info3(int tile_index)
{
	get_tile_info(tile_index,f3_pf_data_3);
}

static void get_tile_info4(int tile_index)
{
	get_tile_info(tile_index,f3_pf_data_4);
}

static void get_tile_info_pixel(int tile_index)
{
	int color,col_off;
	int y_offs=(f3_control_1[2]&0x1ff)+scroll_kludge_y;

	if (flipscreen) y_offs+=0x100;

	/* Colour is shared with VRAM layer */
	if ((((tile_index%32)*8 + y_offs)&0x1ff)>0xff)
		col_off=0x800+((tile_index%32)*0x40)+((tile_index&0xfe0)>>5);
	else
		col_off=((tile_index%32)*0x40)+((tile_index&0xfe0)>>5);

	if (col_off&1)
	   	color = ((videoram32[col_off>>1]&0xffff)>>9)&0x3f;
	else
		color = ((videoram32[col_off>>1]>>16)>>9)&0x3f;

	SET_TILE_INFO(
			3,
			tile_index,
			color&0x3f,
			0)
	tile_info.flags = f3_game_config->pivot ? TILE_FLIPX : 0;
}

/******************************************************************************/

VIDEO_EOF( f3 )
{
	if (sprite_lag==2)
	{
		if (osd_skip_this_frame() == 0)
		{
			get_sprite_info(spriteram32_buffered);
		}
		memcpy(spriteram32_buffered,spriteram32,spriteram_size);
	}
	else if (sprite_lag==1)
	{
		if (osd_skip_this_frame() == 0)
		{
			get_sprite_info(spriteram32);
		}
	}
}

VIDEO_STOP( f3 )
{
#if DEBUG_F3
#define FWRITE32(pRAM,len,file)	\
{								\
	int i;						\
	for(i=0;i<len;i++)			\
	{							\
		unsigned char c;		\
		UINT32 d=*pRAM;			\
		c=(d&0xff000000)>>24;	\
		fwrite(&c,1, 1 ,file);	\
		d<<=8;					\
		c=(d&0xff000000)>>24;	\
		fwrite(&c,1, 1 ,file);	\
		d<<=8;					\
		c=(d&0xff000000)>>24;	\
		fwrite(&c,1, 1 ,file);	\
		d<<=8;					\
		c=(d&0xff000000)>>24;	\
		fwrite(&c,1, 1 ,file);	\
		pRAM++;					\
	}							\
}

	if (deb_enable) {
		FILE *fp;

		fp=fopen("line.dmp","wb");
		if (fp) {
			FWRITE32(f3_line_ram,0x10000/4,fp);
			fclose(fp);
		}
		fp=fopen("sprite.dmp","wb");
		if (fp) {
			FWRITE32(spriteram32,0x10000/4,fp);
			fclose(fp);
		}
		fp=fopen("vram.dmp","wb");
		if (fp) {
			FWRITE32(f3_pf_data,0xc000/4,fp);
			fclose(fp);
		}
	}
#undef FWRITE32
#endif	/*DEBUG_F3*/
}

VIDEO_START( f3 )
{
	const struct F3config *pCFG=&f3_config_table[0];
	int tile;

	spritelist=0;
	spriteram32_buffered=0;
	pivot_dirty=0;
	pixel_layer_clip=Machine->visible_area;
	line_inf=0;
	pri_alp_bitmap=0;
	tile_opaque_sp=0;
	tile_opaque_pf=0;

	/* Setup individual game */
	do {
		if (pCFG->name==f3_game)
		{
			break;
		}
		pCFG++;
	} while(pCFG->name);

	f3_game_config=pCFG;

	if (f3_game_config->extend) {
		pf1_tilemap = tilemap_create(get_tile_info1,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,64,32);
		pf2_tilemap = tilemap_create(get_tile_info2,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,64,32);
		pf3_tilemap = tilemap_create(get_tile_info3,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,64,32);
		pf4_tilemap = tilemap_create(get_tile_info4,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,64,32);

		f3_pf_data_1=f3_pf_data+0x0000;
		f3_pf_data_2=f3_pf_data+0x0800;
		f3_pf_data_3=f3_pf_data+0x1000;
		f3_pf_data_4=f3_pf_data+0x1800;

		width_mask=0x3ff;
		twidth_mask=0x3f;
		twidth_mask_bit=6;

	} else {
		pf1_tilemap = tilemap_create(get_tile_info1,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
		pf2_tilemap = tilemap_create(get_tile_info2,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
		pf3_tilemap = tilemap_create(get_tile_info3,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
		pf4_tilemap = tilemap_create(get_tile_info4,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);

		f3_pf_data_1=f3_pf_data+0x0000;
		f3_pf_data_2=f3_pf_data+0x0400;
		f3_pf_data_3=f3_pf_data+0x0800;
		f3_pf_data_4=f3_pf_data+0x0c00;

		width_mask=0x1ff;
		twidth_mask=0x1f;
		twidth_mask_bit=5;
	}

	spriteram32_buffered = (UINT32 *)auto_malloc(0x10000);
	spritelist = auto_malloc(0x400 * sizeof(*spritelist));
	sprite_end = spritelist;
	pixel_layer = tilemap_create(get_tile_info_pixel,tilemap_scan_cols,TILEMAP_TRANSPARENT,8,8,64,32);
	pivot_dirty = (UINT8 *)auto_malloc(2048);
	line_inf = auto_malloc(4 * sizeof(struct f3_line_inf));
	pri_alp_bitmap = auto_bitmap_alloc_depth( Machine->scrbitmap->width, Machine->scrbitmap->height, -8 );
	tile_opaque_sp = (UINT8 *)auto_malloc(Machine->gfx[2]->total_elements);
	tile_opaque_pf = (UINT8 *)auto_malloc(Machine->gfx[1]->total_elements);

	if (!pf1_tilemap || !pf2_tilemap || !pf3_tilemap || !pf4_tilemap || !line_inf || !pri_alp_bitmap
		 || !spritelist || !pixel_layer || !spriteram32_buffered || !pivot_dirty || !tile_opaque_sp || !tile_opaque_pf)
		return 1;

	tilemap_set_transparent_pen(pf1_tilemap,0);
	tilemap_set_transparent_pen(pf2_tilemap,0);
	tilemap_set_transparent_pen(pf3_tilemap,0);
	tilemap_set_transparent_pen(pf4_tilemap,0);
	tilemap_set_transparent_pen(pixel_layer,0);

	tilemap_set_scroll_rows(pf1_tilemap,512);
	tilemap_set_scroll_rows(pf2_tilemap,512);
	tilemap_set_scroll_rows(pf3_tilemap,512);
	tilemap_set_scroll_rows(pf4_tilemap,512);

	/* Y Offset is related to the first visible line in line ram in some way */
	scroll_kludge_y=f3_game_config->sy;
	scroll_kludge_x=f3_game_config->sx;

	/* Palettes have 4 bpp indexes despite up to 6 bpp data */
	Machine->gfx[1]->color_granularity=16;
	Machine->gfx[2]->color_granularity=16;

	flipscreen = 0;
	memset(spriteram32_buffered,0,spriteram_size);
	memset(spriteram32,0,spriteram_size);

	state_save_register_UINT32("f3", 0, "vcontrol0", f3_control_0, 8);
	state_save_register_UINT32("f3", 0, "vcontrol1", f3_control_1, 8);

	/* Why?!??  These games have different offsets for the two middle playfields only */
	if (f3_game==LANDMAKR || f3_game==EACTION2 || f3_game==DARIUSG || f3_game==GEKIRIDO || f3_game==RECALH)
		pf23_y_kludge=23;
	else
		pf23_y_kludge=0;

	for (tile = 0;tile < 256;tile++)
		vram_dirty[tile]=1;
	for (tile = 0;tile < 2048;tile++)
		pivot_dirty[tile]=1;

	scroll_dirty=1;
	skip_this_frame=0;

	sprite_lag=f3_game_config->sprite_lag;

	init_alpha_blend_func();

	{
		const struct GfxElement *sprite_gfx = Machine->gfx[2];
		int c;
#if DEBUG_F3
		int deb_count_opa=0,deb_count_tra=0,deb_tra_code=-1;
#endif	/*DEBUG_F3*/

		for (c = 0;c < sprite_gfx->total_elements;c++)
		{
			int x,y;
			int chk_trans_or_opa=0;
			UINT8 *dp = sprite_gfx->gfxdata + c * sprite_gfx->char_modulo;
			for (y = 0;y < sprite_gfx->height;y++)
			{
				for (x = 0;x < sprite_gfx->width;x++)
				{
					if(!dp[x]) chk_trans_or_opa|=2;
					else	   chk_trans_or_opa|=1;
				}
				dp += sprite_gfx->line_modulo;
			}
			if(chk_trans_or_opa==1) tile_opaque_sp[c]=1;
			else					tile_opaque_sp[c]=0;
#if DEBUG_F3
			if     (chk_trans_or_opa==2){deb_count_tra++;deb_tra_code=c;}
			else if(chk_trans_or_opa==1) deb_count_opa++;
#endif	/*DEBUG_F3*/
		}
#if DEBUG_F3
		printf("tile_opaque_sp: t=%d o=%d total=%d tra_code=%d\n",deb_count_tra,deb_count_opa,
		sprite_gfx->total_elements,deb_tra_code);
#endif	/*DEBUG_F3*/
	}


	{
		const struct GfxElement *pf_gfx = Machine->gfx[1];
		int c;
#if DEBUG_F3
		int deb_count_opa=0,deb_count_tra=0,deb_tra_code=-1;
#endif	/*DEBUG_F3*/

		for (c = 0;c < pf_gfx->total_elements;c++)
		{
			int x,y;
			int chk_trans_or_opa=0;
			UINT8 *dp = pf_gfx->gfxdata + c * pf_gfx->char_modulo;
			for (y = 0;y < pf_gfx->height;y++)
			{
				for (x = 0;x < pf_gfx->width;x++)
				{
					if(!dp[x]) chk_trans_or_opa|=2;
					else	   chk_trans_or_opa|=1;
				}
				dp += pf_gfx->line_modulo;
			}
			tile_opaque_pf[c]=chk_trans_or_opa;
#if DEBUG_F3
			if     (chk_trans_or_opa==2){deb_count_tra++;deb_tra_code=c;}
			else if(chk_trans_or_opa==1) deb_count_opa++;
#endif	/*DEBUG_F3*/
		}
#if DEBUG_F3
		printf("tile_opaque_pf: t=%d o=%d total=%d tra_code=%d\n",deb_count_tra,deb_count_opa,
		pf_gfx->total_elements,deb_tra_code);
#endif	/*DEBUG_F3*/
	}

	return 0;
}

/******************************************************************************/

WRITE32_HANDLER( f3_pf_data_w )
{
	COMBINE_DATA(&f3_pf_data[offset]);

	if (f3_game_config->extend) {
		if (offset<0x800) tilemap_mark_tile_dirty(pf1_tilemap,offset-0x0000);
		else if (offset<0x1000) tilemap_mark_tile_dirty(pf2_tilemap,offset-0x0800);
		else if (offset<0x1800) tilemap_mark_tile_dirty(pf3_tilemap,offset-0x1000);
		else if (offset<0x2000) tilemap_mark_tile_dirty(pf4_tilemap,offset-0x1800);
	} else {
		if (offset<0x400) tilemap_mark_tile_dirty(pf1_tilemap,offset-0x0000);
		else if (offset<0x800) tilemap_mark_tile_dirty(pf2_tilemap,offset-0x0400);
		else if (offset<0xc00) tilemap_mark_tile_dirty(pf3_tilemap,offset-0x0800);
		else if (offset<0x1000) tilemap_mark_tile_dirty(pf4_tilemap,offset-0xc00);
	}
}

WRITE32_HANDLER( f3_control_0_w )
{
	COMBINE_DATA(&f3_control_0[offset]);
}

WRITE32_HANDLER( f3_control_1_w )
{
	COMBINE_DATA(&f3_control_1[offset]);
}

WRITE32_HANDLER( f3_videoram_w )
{
	int tile,col_off;
	COMBINE_DATA(&videoram32[offset]);

	if (offset>0x3ff) offset-=0x400;

	tile=offset<<1;
	col_off=((tile&0x3f)*32)+((tile&0xfc0)>>6);

	tilemap_mark_tile_dirty(pixel_layer,col_off);
	tilemap_mark_tile_dirty(pixel_layer,col_off+32);
}

WRITE32_HANDLER( f3_vram_w )
{
	COMBINE_DATA(&f3_vram[offset]);
	vram_dirty[offset/8]=1;
	vram_changed=1;
}

WRITE32_HANDLER( f3_pivot_w )
{
	COMBINE_DATA(&f3_pivot_ram[offset]);
	pivot_dirty[offset/8]=1;
	pivot_changed=1;
}

WRITE32_HANDLER( f3_lineram_w )
{
	/* DariusGX has an interesting bug at the start of Round D - the clearing of lineram
	(0xa000->0x0xa7ff) overflows into priority RAM (0xb000) and creates garbage priority
	values.  I'm not sure what the real machine would do with these values, and this
	emulation certainly doesn't like it, so I've chosen to catch the bug here, and prevent
	the trashing of priority ram.  If anyone has information on what the real machine does,
	please let me know! */
	if (f3_game==DARIUSG) {
		if (skip_this_frame)
			return;
		if (offset==0xb000/4 && data==0x003f0000) {
			skip_this_frame=1;
			return;
		}
	}

	COMBINE_DATA(&f3_line_ram[offset]);

/*	if ((offset&0xfe00)==0x2800)*/
/*		scroll_dirty=1;*/

/*	if (offset>=0x6000/4 && offset<0x7000/4)*/
/*	if (offset==0x18c0)*/
/*		log_cb(RETRO_LOG_DEBUG, LOGPRE "%08x:  Write 6000 %08x, %08x\n",activecpu_get_pc(),offset,data);*/
/*	if (offset>=0xa000/4 && offset<0xb000/4)*/
/*		log_cb(RETRO_LOG_DEBUG, LOGPRE "%08x:  Write a000 %08x, %08x\n",activecpu_get_pc(),offset,data);*/
/*	if (offset>=0xb000/4 && offset<0xc000/4)*/
/*		log_cb(RETRO_LOG_DEBUG, LOGPRE "%08x:  Write b000 %08x, %08x\n",activecpu_get_pc(),offset,data);*/

}

WRITE32_HANDLER( f3_palette_24bit_w )
{
	int r,g,b;

	COMBINE_DATA(&paletteram32[offset]);

	/* 12 bit palette games - there has to be a palette select bit somewhere */
	if (f3_game==SPCINVDX || f3_game==RIDINGF || f3_game==ARABIANM || f3_game==RINGRAGE) {
		b = 15 * ((paletteram32[offset] >> 4) & 0xf);
		g = 15 * ((paletteram32[offset] >> 8) & 0xf);
		r = 15 * ((paletteram32[offset] >> 12) & 0xf);
	}

	/* This is weird - why are only the sprites and VRAM palettes 21 bit? */
	else if (f3_game==CLEOPATR) {
		if (offset<0x100 || offset>0x1000) {
		 	r = ((paletteram32[offset] >>16) & 0x7f)<<1;
			g = ((paletteram32[offset] >> 8) & 0x7f)<<1;
			b = ((paletteram32[offset] >> 0) & 0x7f)<<1;
		} else {
		 	r = (paletteram32[offset] >>16) & 0xff;
			g = (paletteram32[offset] >> 8) & 0xff;
			b = (paletteram32[offset] >> 0) & 0xff;
		}
	}

	/* Another weird one */
	else if (f3_game==TWINQIX || f3_game==RECALH) {
		if (offset>0x1c00) {
		 	r = ((paletteram32[offset] >>16) & 0x7f)<<1;
			g = ((paletteram32[offset] >> 8) & 0x7f)<<1;
			b = ((paletteram32[offset] >> 0) & 0x7f)<<1;
		} else {
		 	r = (paletteram32[offset] >>16) & 0xff;
			g = (paletteram32[offset] >> 8) & 0xff;
			b = (paletteram32[offset] >> 0) & 0xff;
		}
	}

	/* All other games - standard 24 bit palette */
	else {
	 	r = (paletteram32[offset] >>16) & 0xff;
		g = (paletteram32[offset] >> 8) & 0xff;
		b = (paletteram32[offset] >> 0) & 0xff;
	}

	palette_set_color(offset,r,g,b);
}

/******************************************************************************/

#if DEBUG_F3
static int deb_alpha_level_a=0;
static int deb_alpha_level_b=0;
static int deb_alp_mode=0;
static int deb_loop=0;
static int deb_alpha_cnt=0;
#endif	/*DEBUG_F3*/

static UINT8 add_sat[256][256];

static const UINT8 *alpha_s_1_1;
static const UINT8 *alpha_s_1_2;
static const UINT8 *alpha_s_1_4;
static const UINT8 *alpha_s_1_5;
static const UINT8 *alpha_s_1_6;
static const UINT8 *alpha_s_1_8;
static const UINT8 *alpha_s_1_9;
static const UINT8 *alpha_s_1_a;

static const UINT8 *alpha_s_2a_0;
static const UINT8 *alpha_s_2a_4;
static const UINT8 *alpha_s_2a_8;

static const UINT8 *alpha_s_2b_0;
static const UINT8 *alpha_s_2b_4;
static const UINT8 *alpha_s_2b_8;

static const UINT8 *alpha_s_3a_0;
static const UINT8 *alpha_s_3a_1;
static const UINT8 *alpha_s_3a_2;

static const UINT8 *alpha_s_3b_0;
static const UINT8 *alpha_s_3b_1;
static const UINT8 *alpha_s_3b_2;

static UINT32 dval;
static UINT8 pval;
static UINT8 tval;
static UINT8 pdest_2a = 0x10;
static UINT8 pdest_2b = 0x20;
static int tr_2a = 0;
static int tr_2b = 1;
static UINT8 pdest_3a = 0x40;
static UINT8 pdest_3b = 0x80;
static int tr_3a = 0;
static int tr_3b = 1;

static int (*dpix_n[8][16])(UINT32 s_pix);
static int (**dpix_lp[4])(UINT32 s_pix);
static int (**dpix_sp[9])(UINT32 s_pix);

/*============================================================================*/

#define SET_ALPHA_LEVEL(d,s)			\
{										\
	int level = s;						\
	if(level == 0) level = -1;			\
	d = alpha_cache.alpha[level+1];		\
}

static INLINE void f3_alpha_set_level(void)
{
/*	SET_ALPHA_LEVEL(alpha_s_1_1, f3_alpha_level_2ad)*/
	SET_ALPHA_LEVEL(alpha_s_1_1, 255-f3_alpha_level_2as)
/*	SET_ALPHA_LEVEL(alpha_s_1_2, f3_alpha_level_2bd)*/
	SET_ALPHA_LEVEL(alpha_s_1_2, 255-f3_alpha_level_2bs)
	SET_ALPHA_LEVEL(alpha_s_1_4, f3_alpha_level_3ad)
/*	SET_ALPHA_LEVEL(alpha_s_1_5, f3_alpha_level_3ad*f3_alpha_level_2ad/255)*/
	SET_ALPHA_LEVEL(alpha_s_1_5, f3_alpha_level_3ad*(255-f3_alpha_level_2as)/255)
/*	SET_ALPHA_LEVEL(alpha_s_1_6, f3_alpha_level_3ad*f3_alpha_level_2bd/255)*/
	SET_ALPHA_LEVEL(alpha_s_1_6, f3_alpha_level_3ad*(255-f3_alpha_level_2bs)/255)
	SET_ALPHA_LEVEL(alpha_s_1_8, f3_alpha_level_3bd)
/*	SET_ALPHA_LEVEL(alpha_s_1_9, f3_alpha_level_3bd*f3_alpha_level_2ad/255)*/
	SET_ALPHA_LEVEL(alpha_s_1_9, f3_alpha_level_3bd*(255-f3_alpha_level_2as)/255)
/*	SET_ALPHA_LEVEL(alpha_s_1_a, f3_alpha_level_3bd*f3_alpha_level_2bd/255)*/
	SET_ALPHA_LEVEL(alpha_s_1_a, f3_alpha_level_3bd*(255-f3_alpha_level_2bs)/255)

	SET_ALPHA_LEVEL(alpha_s_2a_0, f3_alpha_level_2as)
	SET_ALPHA_LEVEL(alpha_s_2a_4, f3_alpha_level_2as*f3_alpha_level_3ad/255)
	SET_ALPHA_LEVEL(alpha_s_2a_8, f3_alpha_level_2as*f3_alpha_level_3bd/255)

	SET_ALPHA_LEVEL(alpha_s_2b_0, f3_alpha_level_2bs)
	SET_ALPHA_LEVEL(alpha_s_2b_4, f3_alpha_level_2bs*f3_alpha_level_3ad/255)
	SET_ALPHA_LEVEL(alpha_s_2b_8, f3_alpha_level_2bs*f3_alpha_level_3bd/255)

	SET_ALPHA_LEVEL(alpha_s_3a_0, f3_alpha_level_3as)
	SET_ALPHA_LEVEL(alpha_s_3a_1, f3_alpha_level_3as*f3_alpha_level_2ad/255)
	SET_ALPHA_LEVEL(alpha_s_3a_2, f3_alpha_level_3as*f3_alpha_level_2bd/255)

	SET_ALPHA_LEVEL(alpha_s_3b_0, f3_alpha_level_3bs)
	SET_ALPHA_LEVEL(alpha_s_3b_1, f3_alpha_level_3bs*f3_alpha_level_2ad/255)
	SET_ALPHA_LEVEL(alpha_s_3b_2, f3_alpha_level_3bs*f3_alpha_level_2bd/255)
}
#undef SET_ALPHA_LEVEL

/*============================================================================*/

#ifdef MSB_FIRST
#define COLOR1 3
#define COLOR2 2
#define COLOR3 1
#else
#define COLOR1 0
#define COLOR2 1
#define COLOR3 2
#endif

static INLINE void f3_alpha_blend32_s( const UINT8 *alphas, UINT32 s )
{
	UINT8 *sc = (UINT8 *)&s;
	UINT8 *dc = (UINT8 *)&dval;
	dc[COLOR1] = alphas[sc[COLOR1]];
	dc[COLOR2] = alphas[sc[COLOR2]];
	dc[COLOR3] = alphas[sc[COLOR3]];
}

static INLINE void f3_alpha_blend32_d( const UINT8 *alphas, UINT32 s )
{
	UINT8 *sc = (UINT8 *)&s;
	UINT8 *dc = (UINT8 *)&dval;
	dc[COLOR1] = add_sat[dc[COLOR1]][alphas[sc[COLOR1]]];
	dc[COLOR2] = add_sat[dc[COLOR2]][alphas[sc[COLOR2]]];
	dc[COLOR3] = add_sat[dc[COLOR3]][alphas[sc[COLOR3]]];
}

/*============================================================================*/

static INLINE void f3_alpha_blend_1_1( UINT32 s ){f3_alpha_blend32_d(alpha_s_1_1,s);}
static INLINE void f3_alpha_blend_1_2( UINT32 s ){f3_alpha_blend32_d(alpha_s_1_2,s);}
static INLINE void f3_alpha_blend_1_4( UINT32 s ){f3_alpha_blend32_d(alpha_s_1_4,s);}
static INLINE void f3_alpha_blend_1_5( UINT32 s ){f3_alpha_blend32_d(alpha_s_1_5,s);}
static INLINE void f3_alpha_blend_1_6( UINT32 s ){f3_alpha_blend32_d(alpha_s_1_6,s);}
static INLINE void f3_alpha_blend_1_8( UINT32 s ){f3_alpha_blend32_d(alpha_s_1_8,s);}
static INLINE void f3_alpha_blend_1_9( UINT32 s ){f3_alpha_blend32_d(alpha_s_1_9,s);}
static INLINE void f3_alpha_blend_1_a( UINT32 s ){f3_alpha_blend32_d(alpha_s_1_a,s);}

static INLINE void f3_alpha_blend_2a_0( UINT32 s ){f3_alpha_blend32_s(alpha_s_2a_0,s);}
static INLINE void f3_alpha_blend_2a_4( UINT32 s ){f3_alpha_blend32_d(alpha_s_2a_4,s);}
static INLINE void f3_alpha_blend_2a_8( UINT32 s ){f3_alpha_blend32_d(alpha_s_2a_8,s);}

static INLINE void f3_alpha_blend_2b_0( UINT32 s ){f3_alpha_blend32_s(alpha_s_2b_0,s);}
static INLINE void f3_alpha_blend_2b_4( UINT32 s ){f3_alpha_blend32_d(alpha_s_2b_4,s);}
static INLINE void f3_alpha_blend_2b_8( UINT32 s ){f3_alpha_blend32_d(alpha_s_2b_8,s);}

static INLINE void f3_alpha_blend_3a_0( UINT32 s ){f3_alpha_blend32_s(alpha_s_3a_0,s);}
static INLINE void f3_alpha_blend_3a_1( UINT32 s ){f3_alpha_blend32_d(alpha_s_3a_1,s);}
static INLINE void f3_alpha_blend_3a_2( UINT32 s ){f3_alpha_blend32_d(alpha_s_3a_2,s);}

static INLINE void f3_alpha_blend_3b_0( UINT32 s ){f3_alpha_blend32_s(alpha_s_3b_0,s);}
static INLINE void f3_alpha_blend_3b_1( UINT32 s ){f3_alpha_blend32_d(alpha_s_3b_1,s);}
static INLINE void f3_alpha_blend_3b_2( UINT32 s ){f3_alpha_blend32_d(alpha_s_3b_2,s);}

/*============================================================================*/

static int dpix_1_noalpha(UINT32 s_pix) {dval = s_pix; return 1;}
static int dpix_ret1(UINT32 s_pix) {return 1;}
static int dpix_ret0(UINT32 s_pix) {return 0;}
static int dpix_1_1(UINT32 s_pix) {if(s_pix) f3_alpha_blend_1_1(s_pix); return 1;}
static int dpix_1_2(UINT32 s_pix) {if(s_pix) f3_alpha_blend_1_2(s_pix); return 1;}
static int dpix_1_4(UINT32 s_pix) {if(s_pix) f3_alpha_blend_1_4(s_pix); return 1;}
static int dpix_1_5(UINT32 s_pix) {if(s_pix) f3_alpha_blend_1_5(s_pix); return 1;}
static int dpix_1_6(UINT32 s_pix) {if(s_pix) f3_alpha_blend_1_6(s_pix); return 1;}
static int dpix_1_8(UINT32 s_pix) {if(s_pix) f3_alpha_blend_1_8(s_pix); return 1;}
static int dpix_1_9(UINT32 s_pix) {if(s_pix) f3_alpha_blend_1_9(s_pix); return 1;}
static int dpix_1_a(UINT32 s_pix) {if(s_pix) f3_alpha_blend_1_a(s_pix); return 1;}

static int dpix_2a_0(UINT32 s_pix)
{
	if(s_pix) f3_alpha_blend_2a_0(s_pix);
	else	  dval = 0;
	if(pdest_2a) {pval |= pdest_2a;return 0;}
	return 1;
}
static int dpix_2a_4(UINT32 s_pix)
{
	if(s_pix) f3_alpha_blend_2a_4(s_pix);
	if(pdest_2a) {pval |= pdest_2a;return 0;}
	return 1;
}
static int dpix_2a_8(UINT32 s_pix)
{
	if(s_pix) f3_alpha_blend_2a_8(s_pix);
	if(pdest_2a) {pval |= pdest_2a;return 0;}
	return 1;
}

static int dpix_3a_0(UINT32 s_pix)
{
	if(s_pix) f3_alpha_blend_3a_0(s_pix);
	else	  dval = 0;
	if(pdest_3a) {pval |= pdest_3a;return 0;}
	return 1;
}
static int dpix_3a_1(UINT32 s_pix)
{
	if(s_pix) f3_alpha_blend_3a_1(s_pix);
	if(pdest_3a) {pval |= pdest_3a;return 0;}
	return 1;
}
static int dpix_3a_2(UINT32 s_pix)
{
	if(s_pix) f3_alpha_blend_3a_2(s_pix);
	if(pdest_3a) {pval |= pdest_3a;return 0;}
	return 1;
}

static int dpix_2b_0(UINT32 s_pix)
{
	if(s_pix) f3_alpha_blend_2b_0(s_pix);
	else	  dval = 0;
	if(pdest_2b) {pval |= pdest_2b;return 0;}
	return 1;
}
static int dpix_2b_4(UINT32 s_pix)
{
	if(s_pix) f3_alpha_blend_2b_4(s_pix);
	if(pdest_2b) {pval |= pdest_2b;return 0;}
	return 1;
}
static int dpix_2b_8(UINT32 s_pix)
{
	if(s_pix) f3_alpha_blend_2b_8(s_pix);
	if(pdest_2b) {pval |= pdest_2b;return 0;}
	return 1;
}

static int dpix_3b_0(UINT32 s_pix)
{
	if(s_pix) f3_alpha_blend_3b_0(s_pix);
	else	  dval = 0;
	if(pdest_3b) {pval |= pdest_3b;return 0;}
	return 1;
}
static int dpix_3b_1(UINT32 s_pix)
{
	if(s_pix) f3_alpha_blend_3b_1(s_pix);
	if(pdest_3b) {pval |= pdest_3b;return 0;}
	return 1;
}
static int dpix_3b_2(UINT32 s_pix)
{
	if(s_pix) f3_alpha_blend_3b_2(s_pix);
	if(pdest_3b) {pval |= pdest_3b;return 0;}
	return 1;
}

static int dpix_2_0(UINT32 s_pix)
{
	UINT8 tr2=tval&1;
	if(s_pix)
	{
		if(tr2==tr_2b)		{f3_alpha_blend_2b_0(s_pix);if(pdest_2b) pval |= pdest_2b;else return 1;}
		else if(tr2==tr_2a)	{f3_alpha_blend_2a_0(s_pix);if(pdest_2a) pval |= pdest_2a;else return 1;}
	}
	else
	{
		if(tr2==tr_2b)		{dval = 0;if(pdest_2b) pval |= pdest_2b;else return 1;}
		else if(tr2==tr_2a)	{dval = 0;if(pdest_2a) pval |= pdest_2a;else return 1;}
	}
	return 0;
}
static int dpix_2_4(UINT32 s_pix)
{
	UINT8 tr2=tval&1;
	if(s_pix)
	{
		if(tr2==tr_2b)		{f3_alpha_blend_2b_4(s_pix);if(pdest_2b) pval |= pdest_2b;else return 1;}
		else if(tr2==tr_2a)	{f3_alpha_blend_2a_4(s_pix);if(pdest_2a) pval |= pdest_2a;else return 1;}
	}
	else
	{
		if(tr2==tr_2b)		{if(pdest_2b) pval |= pdest_2b;else return 1;}
		else if(tr2==tr_2a)	{if(pdest_2a) pval |= pdest_2a;else return 1;}
	}
	return 0;
}
static int dpix_2_8(UINT32 s_pix)
{
	UINT8 tr2=tval&1;
	if(s_pix)
	{
		if(tr2==tr_2b)		{f3_alpha_blend_2b_8(s_pix);if(pdest_2b) pval |= pdest_2b;else return 1;}
		else if(tr2==tr_2a)	{f3_alpha_blend_2a_8(s_pix);if(pdest_2a) pval |= pdest_2a;else return 1;}
	}
	else
	{
		if(tr2==tr_2b)		{if(pdest_2b) pval |= pdest_2b;else return 1;}
		else if(tr2==tr_2a)	{if(pdest_2a) pval |= pdest_2a;else return 1;}
	}
	return 0;
}

static int dpix_3_0(UINT32 s_pix)
{
	UINT8 tr2=tval&1;
	if(s_pix)
	{
		if(tr2==tr_3b)		{f3_alpha_blend_3b_0(s_pix);if(pdest_3b) pval |= pdest_3b;else return 1;}
		else if(tr2==tr_3a)	{f3_alpha_blend_3a_0(s_pix);if(pdest_3a) pval |= pdest_3a;else return 1;}
	}
	else
	{
		if(tr2==tr_3b)		{dval = 0;if(pdest_3b) pval |= pdest_3b;else return 1;}
		else if(tr2==tr_3a)	{dval = 0;if(pdest_3a) pval |= pdest_3a;else return 1;}
	}
	return 0;
}
static int dpix_3_1(UINT32 s_pix)
{
	UINT8 tr2=tval&1;
	if(s_pix)
	{
		if(tr2==tr_3b)		{f3_alpha_blend_3b_1(s_pix);if(pdest_3b) pval |= pdest_3b;else return 1;}
		else if(tr2==tr_3a)	{f3_alpha_blend_3a_1(s_pix);if(pdest_3a) pval |= pdest_3a;else return 1;}
	}
	else
	{
		if(tr2==tr_3b)		{if(pdest_3b) pval |= pdest_3b;else return 1;}
		else if(tr2==tr_3a)	{if(pdest_3a) pval |= pdest_3a;else return 1;}
	}
	return 0;
}
static int dpix_3_2(UINT32 s_pix)
{
	UINT8 tr2=tval&1;
	if(s_pix)
	{
		if(tr2==tr_3b)		{f3_alpha_blend_3b_2(s_pix);if(pdest_3b) pval |= pdest_3b;else return 1;}
		else if(tr2==tr_3a)	{f3_alpha_blend_3a_2(s_pix);if(pdest_3a) pval |= pdest_3a;else return 1;}
	}
	else
	{
		if(tr2==tr_3b)		{if(pdest_3b) pval |= pdest_3b;else return 1;}
		else if(tr2==tr_3a)	{if(pdest_3a) pval |= pdest_3a;else return 1;}
	}
	return 0;
}

static INLINE void dpix_1_sprite(UINT32 s_pix)
{
	if(s_pix)
	{
		UINT8 p1 = pval&0xf0;
		if     (p1==0x10)	f3_alpha_blend_1_1(s_pix);
		else if(p1==0x20)	f3_alpha_blend_1_2(s_pix);
		else if(p1==0x40)	f3_alpha_blend_1_4(s_pix);
		else if(p1==0x50)	f3_alpha_blend_1_5(s_pix);
		else if(p1==0x60)	f3_alpha_blend_1_6(s_pix);
		else if(p1==0x80)	f3_alpha_blend_1_8(s_pix);
		else if(p1==0x90)	f3_alpha_blend_1_9(s_pix);
		else if(p1==0xa0)	f3_alpha_blend_1_a(s_pix);
	}
}

static INLINE void dpix_bg(UINT32 bgcolor)
{
	UINT8 p1 = pval&0xf0;
	if(!p1)			dval = bgcolor;
	else if(p1==0x10)	f3_alpha_blend_1_1(bgcolor);
	else if(p1==0x20)	f3_alpha_blend_1_2(bgcolor);
	else if(p1==0x40)	f3_alpha_blend_1_4(bgcolor);
	else if(p1==0x50)	f3_alpha_blend_1_5(bgcolor);
	else if(p1==0x60)	f3_alpha_blend_1_6(bgcolor);
	else if(p1==0x80)	f3_alpha_blend_1_8(bgcolor);
	else if(p1==0x90)	f3_alpha_blend_1_9(bgcolor);
	else if(p1==0xa0)	f3_alpha_blend_1_a(bgcolor);
}

/******************************************************************************/

static void init_alpha_blend_func(void)
{
	int i,j;

	dpix_n[0][0x0]=dpix_1_noalpha;
	dpix_n[0][0x1]=dpix_1_noalpha;
	dpix_n[0][0x2]=dpix_1_noalpha;
	dpix_n[0][0x3]=dpix_1_noalpha;
	dpix_n[0][0x4]=dpix_1_noalpha;
	dpix_n[0][0x5]=dpix_1_noalpha;
	dpix_n[0][0x6]=dpix_1_noalpha;
	dpix_n[0][0x7]=dpix_1_noalpha;
	dpix_n[0][0x8]=dpix_1_noalpha;
	dpix_n[0][0x9]=dpix_1_noalpha;
	dpix_n[0][0xa]=dpix_1_noalpha;
	dpix_n[0][0xb]=dpix_1_noalpha;
	dpix_n[0][0xc]=dpix_1_noalpha;
	dpix_n[0][0xd]=dpix_1_noalpha;
	dpix_n[0][0xe]=dpix_1_noalpha;
	dpix_n[0][0xf]=dpix_1_noalpha;

	dpix_n[1][0x0]=dpix_1_noalpha;
	dpix_n[1][0x1]=dpix_1_1;
	dpix_n[1][0x2]=dpix_1_2;
	dpix_n[1][0x3]=dpix_ret1;
	dpix_n[1][0x4]=dpix_1_4;
	dpix_n[1][0x5]=dpix_1_5;
	dpix_n[1][0x6]=dpix_1_6;
	dpix_n[1][0x7]=dpix_ret1;
	dpix_n[1][0x8]=dpix_1_8;
	dpix_n[1][0x9]=dpix_1_9;
	dpix_n[1][0xa]=dpix_1_a;
	dpix_n[1][0xb]=dpix_ret1;
	dpix_n[1][0xc]=dpix_ret1;
	dpix_n[1][0xd]=dpix_ret1;
	dpix_n[1][0xe]=dpix_ret1;
	dpix_n[1][0xf]=dpix_ret1;

	dpix_n[2][0x0]=dpix_2a_0;
	dpix_n[2][0x1]=dpix_ret0;
	dpix_n[2][0x2]=dpix_ret0;
	dpix_n[2][0x3]=dpix_ret0;
	dpix_n[2][0x4]=dpix_2a_4;
	dpix_n[2][0x5]=dpix_ret0;
	dpix_n[2][0x6]=dpix_ret0;
	dpix_n[2][0x7]=dpix_ret0;
	dpix_n[2][0x8]=dpix_2a_8;
	dpix_n[2][0x9]=dpix_ret0;
	dpix_n[2][0xa]=dpix_ret0;
	dpix_n[2][0xb]=dpix_ret0;
	dpix_n[2][0xc]=dpix_ret0;
	dpix_n[2][0xd]=dpix_ret0;
	dpix_n[2][0xe]=dpix_ret0;
	dpix_n[2][0xf]=dpix_ret0;

	dpix_n[3][0x0]=dpix_3a_0;
	dpix_n[3][0x1]=dpix_3a_1;
	dpix_n[3][0x2]=dpix_3a_2;
	dpix_n[3][0x3]=dpix_ret0;
	dpix_n[3][0x4]=dpix_ret0;
	dpix_n[3][0x5]=dpix_ret0;
	dpix_n[3][0x6]=dpix_ret0;
	dpix_n[3][0x7]=dpix_ret0;
	dpix_n[3][0x8]=dpix_ret0;
	dpix_n[3][0x9]=dpix_ret0;
	dpix_n[3][0xa]=dpix_ret0;
	dpix_n[3][0xb]=dpix_ret0;
	dpix_n[3][0xc]=dpix_ret0;
	dpix_n[3][0xd]=dpix_ret0;
	dpix_n[3][0xe]=dpix_ret0;
	dpix_n[3][0xf]=dpix_ret0;

	dpix_n[4][0x0]=dpix_2b_0;
	dpix_n[4][0x1]=dpix_ret0;
	dpix_n[4][0x2]=dpix_ret0;
	dpix_n[4][0x3]=dpix_ret0;
	dpix_n[4][0x4]=dpix_2b_4;
	dpix_n[4][0x5]=dpix_ret0;
	dpix_n[4][0x6]=dpix_ret0;
	dpix_n[4][0x7]=dpix_ret0;
	dpix_n[4][0x8]=dpix_2b_8;
	dpix_n[4][0x9]=dpix_ret0;
	dpix_n[4][0xa]=dpix_ret0;
	dpix_n[4][0xb]=dpix_ret0;
	dpix_n[4][0xc]=dpix_ret0;
	dpix_n[4][0xd]=dpix_ret0;
	dpix_n[4][0xe]=dpix_ret0;
	dpix_n[4][0xf]=dpix_ret0;

	dpix_n[5][0x0]=dpix_3b_0;
	dpix_n[5][0x1]=dpix_3b_1;
	dpix_n[5][0x2]=dpix_3b_2;
	dpix_n[5][0x3]=dpix_ret0;
	dpix_n[5][0x4]=dpix_ret0;
	dpix_n[5][0x5]=dpix_ret0;
	dpix_n[5][0x6]=dpix_ret0;
	dpix_n[5][0x7]=dpix_ret0;
	dpix_n[5][0x8]=dpix_ret0;
	dpix_n[5][0x9]=dpix_ret0;
	dpix_n[5][0xa]=dpix_ret0;
	dpix_n[5][0xb]=dpix_ret0;
	dpix_n[5][0xc]=dpix_ret0;
	dpix_n[5][0xd]=dpix_ret0;
	dpix_n[5][0xe]=dpix_ret0;
	dpix_n[5][0xf]=dpix_ret0;

	dpix_n[6][0x0]=dpix_2_0;
	dpix_n[6][0x1]=dpix_ret0;
	dpix_n[6][0x2]=dpix_ret0;
	dpix_n[6][0x3]=dpix_ret0;
	dpix_n[6][0x4]=dpix_2_4;
	dpix_n[6][0x5]=dpix_ret0;
	dpix_n[6][0x6]=dpix_ret0;
	dpix_n[6][0x7]=dpix_ret0;
	dpix_n[6][0x8]=dpix_2_8;
	dpix_n[6][0x9]=dpix_ret0;
	dpix_n[6][0xa]=dpix_ret0;
	dpix_n[6][0xb]=dpix_ret0;
	dpix_n[6][0xc]=dpix_ret0;
	dpix_n[6][0xd]=dpix_ret0;
	dpix_n[6][0xe]=dpix_ret0;
	dpix_n[6][0xf]=dpix_ret0;

	dpix_n[7][0x0]=dpix_3_0;
	dpix_n[7][0x1]=dpix_3_1;
	dpix_n[7][0x2]=dpix_3_2;
	dpix_n[7][0x3]=dpix_ret0;
	dpix_n[7][0x4]=dpix_ret0;
	dpix_n[7][0x5]=dpix_ret0;
	dpix_n[7][0x6]=dpix_ret0;
	dpix_n[7][0x7]=dpix_ret0;
	dpix_n[7][0x8]=dpix_ret0;
	dpix_n[7][0x9]=dpix_ret0;
	dpix_n[7][0xa]=dpix_ret0;
	dpix_n[7][0xb]=dpix_ret0;
	dpix_n[7][0xc]=dpix_ret0;
	dpix_n[7][0xd]=dpix_ret0;
	dpix_n[7][0xe]=dpix_ret0;
	dpix_n[7][0xf]=dpix_ret0;

	for(i=0;i<256;i++)
		for(j=0;j<256;j++)
			add_sat[i][j] = (i + j < 256) ? i + j : 255;
}

/******************************************************************************/

#define GET_PIXMAP_POINTER(pf_num) \
{ \
	const struct f3_line_inf *line_tmp=line_t[pf_num]; \
	src##pf_num=line_tmp->src[y]; \
	src_s##pf_num=line_tmp->src_s[y]; \
	src_e##pf_num=line_tmp->src_e[y]; \
	tsrc##pf_num=line_tmp->tsrc[y]; \
	tsrc_s##pf_num=line_tmp->tsrc_s[y]; \
	x_count##pf_num=line_tmp->x_count[y]; \
	x_zoom##pf_num=line_tmp->x_zoom[y]; \
}

#define CULC_PIXMAP_POINTER(pf_num) \
{ \
	x_count##pf_num += x_zoom##pf_num; \
	if(x_count##pf_num>>16) \
	{ \
		x_count##pf_num &= 0xffff; \
		src##pf_num++; \
		tsrc##pf_num++; \
		if(src##pf_num==src_e##pf_num) {src##pf_num=src_s##pf_num; tsrc##pf_num=tsrc_s##pf_num;} \
	} \
}

/*============================================================================*/

static INLINE void f3_drawscanlines(
		struct mame_bitmap *bitmap,int x,int xsize,INT16 *draw_line_num,
		const struct f3_line_inf **line_t,
		const int *sprite,
		UINT32 orient,
		int skip_layer_num)
{
	pen_t *clut = &Machine->remapped_colortable[0];
	UINT32 bgcolor=clut[0];
	int length;

	UINT32 sprite_noalp_0=sprite[0]&0x100;
	UINT32 sprite_noalp_1=sprite[1]&0x100;
	UINT32 sprite_noalp_2=sprite[2]&0x100;
	UINT32 sprite_noalp_3=sprite[3]&0x100;
	UINT32 sprite_noalp_4=sprite[4]&0x100;

	static UINT16 *src0=0,*src_s0=0,*src_e0=0;
	static UINT8 *tsrc0=0,*tsrc_s0=0;
	static UINT32 x_count0=0,x_zoom0=0;

	static UINT16 *src1=0,*src_s1=0,*src_e1=0;
	static UINT8 *tsrc1=0,*tsrc_s1=0;
	static UINT32 x_count1=0,x_zoom1=0;

	static UINT16 *src2=0,*src_s2=0,*src_e2=0;
	static UINT8 *tsrc2=0,*tsrc_s2=0;
	static UINT32 x_count2=0,x_zoom2=0;

	static UINT16 *src3=0,*src_s3=0,*src_e3=0;
	static UINT8 *tsrc3=0,*tsrc_s3=0;
	static UINT32 x_count3=0,x_zoom3=0;

	UINT8 *dstp0,*dstp;

	int yadv = bitmap->rowpixels;
	int i=0,y=draw_line_num[0];
	int ty = y;

	if (orient & ORIENTATION_FLIP_Y)
	{
		ty = bitmap->height - 1 - ty;
		yadv = -yadv;
	}

	dstp0 = (UINT8 *)pri_alp_bitmap->line[ty] + x;

	pdest_2a = f3_alpha_level_2ad ? 0x10 : 0;
	pdest_2b = f3_alpha_level_2bd ? 0x20 : 0;
	tr_2a =(f3_alpha_level_2as==0 && f3_alpha_level_2ad==255) ? -1 : 0;
	tr_2b =(f3_alpha_level_2bs==0 && f3_alpha_level_2bd==255) ? -1 : 1;
	pdest_3a = f3_alpha_level_3ad ? 0x40 : 0;
	pdest_3b = f3_alpha_level_3bd ? 0x80 : 0;
	tr_3a =(f3_alpha_level_3as==0 && f3_alpha_level_3ad==255) ? -1 : 0;
	tr_3b =(f3_alpha_level_3bs==0 && f3_alpha_level_3bd==255) ? -1 : 1;


/*	if (bitmap->depth == 32)*/
	{
		UINT32 *dsti0,*dsti;
		dsti0 = (UINT32 *)bitmap->line[ty] + x;
		while(1)
		{
			length=xsize;
			dsti = dsti0;
			dstp = dstp0;

			switch(skip_layer_num)
			{
				case 0: GET_PIXMAP_POINTER(0)
				case 1: GET_PIXMAP_POINTER(1)
				case 2: GET_PIXMAP_POINTER(2)
				case 3: GET_PIXMAP_POINTER(3)
			}

			while (1)
			{
				pval=*dstp;
				if (pval!=0xff)
				{
					UINT8 sprite_pri;
					switch(skip_layer_num)
					{
						case 0: if((sprite_pri=sprite[0]&pval))
								{
									if(sprite_noalp_0) break;
									if(!dpix_sp[sprite_pri]) break;
									if(dpix_sp[sprite_pri][pval>>4](*dsti)) {*dsti=dval;break;}
								}
								{tval=*tsrc0;if(tval&0xf0) if(dpix_lp[0][pval>>4](clut[*src0])) {*dsti=dval;break;}}
						case 1: if((sprite_pri=sprite[1]&pval))
								{
									if(sprite_noalp_1) break;
									if(!dpix_sp[sprite_pri])
									{
										if(!(pval&0xf0)) break;
										else {dpix_1_sprite(*dsti);*dsti=dval;break;}
									}
									if(dpix_sp[sprite_pri][pval>>4](*dsti)) {*dsti=dval;break;}
								}
								{tval=*tsrc1;if(tval&0xf0) if(dpix_lp[1][pval>>4](clut[*src1])) {*dsti=dval;break;}}
						case 2: if((sprite_pri=sprite[2]&pval))
								{
									if(sprite_noalp_2) break;
									if(!dpix_sp[sprite_pri])
									{
										if(!(pval&0xf0)) break;
										else {dpix_1_sprite(*dsti);*dsti=dval;break;}
									}
									if(dpix_sp[sprite_pri][pval>>4](*dsti)) {*dsti=dval;break;}
								}
								{tval=*tsrc2;if(tval&0xf0) if(dpix_lp[2][pval>>4](clut[*src2])) {*dsti=dval;break;}}
						case 3: if((sprite_pri=sprite[3]&pval))
								{
									if(sprite_noalp_3) break;
									if(!dpix_sp[sprite_pri])
									{
										if(!(pval&0xf0)) break;
										else {dpix_1_sprite(*dsti);*dsti=dval;break;}
									}
									if(dpix_sp[sprite_pri][pval>>4](*dsti)) {*dsti=dval;break;}
								}
								{tval=*tsrc3;if(tval&0xf0) if(dpix_lp[3][pval>>4](clut[*src3])) {*dsti=dval;break;}}
						case 4: if((sprite_pri=sprite[4]&pval))
								{
									if(sprite_noalp_4) break;
									if(!dpix_sp[sprite_pri])
									{
										if(!(pval&0xf0)) break;
										else {dpix_1_sprite(*dsti);*dsti=dval;break;}
									}
									if(dpix_sp[sprite_pri][pval>>4](*dsti)) {*dsti=dval;break;}
								}
								if(!bgcolor) {if(!(pval&0xf0)) {*dsti=0;break;}}
								else dpix_bg(bgcolor);
								*dsti=dval;
					}
				}

				if(!(--length)) break;
				dsti++;
				dstp++;

				switch(skip_layer_num)
				{
					case 0: CULC_PIXMAP_POINTER(0)
					case 1: CULC_PIXMAP_POINTER(1)
					case 2: CULC_PIXMAP_POINTER(2)
					case 3: CULC_PIXMAP_POINTER(3)
				}
			}

			i++;
			if(draw_line_num[i]<0) break;
			if(draw_line_num[i]==y+1)
			{
				dsti0 += yadv;
				dstp0 += yadv;
				y++;
				continue;
			}
			else
			{
				int dy=(draw_line_num[i]-y)*yadv;
				dsti0 += dy;
				dstp0 += dy;
				y=draw_line_num[i];
			}
		}
	}
}
#undef GET_PIXMAP_POINTER
#undef CULC_PIXMAP_POINTER

/******************************************************************************/

static INLINE void clear_scanlines(struct mame_bitmap *bitmap,int x,int xsize,INT16 *draw_line_num,UINT32 orient)
{
	int length;

	int yadv = bitmap->rowpixels;
	int i=0,y=draw_line_num[0];
	int ty = y;

	if (orient & ORIENTATION_FLIP_Y)
	{
		ty = bitmap->height - 1 - ty;
		yadv = -yadv;
	}

/*	if (bitmap->depth == 32)*/
	{
		UINT32 *dsti0,*dsti;
		dsti0 = (UINT32 *)bitmap->line[ty] + x;

		while (1)
		{
			length=xsize;
			dsti = dsti0;
			while (length--)
			{
				*dsti = 0;
				dsti++;
			}

			i++;
			if(draw_line_num[i]<0) break;
			if(draw_line_num[i]==y+1)
			{
				dsti0 += yadv;
				y++;
				continue;
			}
			else
			{
				dsti0 += (draw_line_num[i]-y)*yadv;
				y=draw_line_num[i];
			}
		}
	}
}

/******************************************************************************/

static void visible_tile_check(struct f3_line_inf *line_t,
								int line,
								UINT32 x_index_fx,UINT32 y_index,
								data32_t *f3_pf_data_n)
{
	data32_t *pf_base;
	int i,trans_all,tile_index,tile_num;
	int alpha_type,alpha_mode;
	int opaque_all;
	int total_elements;

	if(!(alpha_mode=line_t->alpha_mode[line])) return;

	total_elements=Machine->gfx[1]->total_elements;

	tile_index=x_index_fx>>16;
	tile_num=(((line_t->x_zoom[line]*320+(x_index_fx & 0xffff)+0xffff)>>16)+(tile_index%16)+15)/16;
	tile_index/=16;

	if (flipscreen)
	{
		pf_base=f3_pf_data_n+((31-(y_index/16))<<twidth_mask_bit);
		tile_index=(twidth_mask-tile_index)-tile_num+1;
	}
	else pf_base=f3_pf_data_n+((y_index/16)<<twidth_mask_bit);


	trans_all=1;
	opaque_all=1;
	alpha_type=0;
	for(i=0;i<tile_num;i++)
	{
		UINT32 tile=pf_base[(tile_index)&twidth_mask];
		if(tile&0xffff)
		{
			trans_all=0;
			if(opaque_all)
			{
				if(tile_opaque_pf[(tile&0xffff)%total_elements]!=1) opaque_all=0;
			}

			if(alpha_mode==1)
			{
				if(!opaque_all) return;
			}
			else
			{
				if(alpha_type!=3)
				{
					if((tile>>(16+9))&1) alpha_type|=2;
					else				 alpha_type|=1;
				}
				else if(!opaque_all) break;
			}
		}
		else if(opaque_all) opaque_all=0;

		tile_index++;
	}

	if(trans_all)	{line_t->alpha_mode[line]=0;return;}

	if(alpha_mode>1)
	{
		line_t->alpha_mode[line]|=alpha_type<<4;
	}

	if(opaque_all)
		line_t->alpha_mode[line]|=0x80;
}

/******************************************************************************/

/* sx and sy are 16.16 fixed point numbers */
static void get_line_ram_info(struct tilemap *tilemap,int sx,int sy,int pos,data32_t *f3_pf_data_n)
{
	struct f3_line_inf *line_t=&line_inf[pos];
	const struct mame_bitmap *srcbitmap;
	const struct mame_bitmap *transbitmap;

	int y,y_start,y_end,y_inc;
	int line_base,zoom_base,col_base,pri_base,spri_base,inc;

	int line_enable;
	int colscroll=0,x_offset=0;
	UINT32 x_zoom=0x10000;
	UINT32 y_zoom=0;
	UINT16 pri=0;
	UINT16 spri=0;
	int alpha_level=0;
	int bit_select0=0x10000<<pos;
	int bit_select1=1<<pos;
	UINT16 sprite_alpha=0;

	int _colscroll[256];
	UINT32 _x_offset[256];
	int y_index_fx;

	sx+=(46<<16);/*+scroll_kludge_x;*/
	if (flipscreen)
	{
		line_base=0xa1fe + (pos*0x200);
		zoom_base=0x81fe + (pos*0x200);
		col_base =0x41fe + (pos*0x200);
		pri_base =0xb1fe + (pos*0x200);
		spri_base =0x77fe;
		inc=-2;
		y_start=255;
		y_end=-1;
		y_inc=-1;

		if (f3_game_config->extend)	sx=-sx+((188-512)<<16); else sx=-sx+(188<<16); /* Adjust for flipped scroll position */
		y_index_fx=-sy-(256<<16); /* Adjust for flipped scroll position */
	}
	else
	{
		line_base=0xa000 + (pos*0x200);
		zoom_base=0x8000 + (pos*0x200);
		col_base =0x4000 + (pos*0x200);
		pri_base =0xb000 + (pos*0x200);
		spri_base =0x7600;
		inc=2;
		y_start=0;
		y_end=256;
		y_inc=1;

		y_index_fx=sy;
	}

	y=y_start;
	while(y!=y_end)
	{
		/* The zoom, column and row values can latch according to control ram */
		if (y&1)
		{
			if (f3_line_ram[0x300+(y>>1)]&bit_select1)
				x_offset=(f3_line_ram[line_base/4]&0xffff)<<10;
			if (f3_line_ram[0x380+(y>>1)]&bit_select1)
				pri=f3_line_ram[pri_base/4]&0xffff;
			if (pri && !(pri&0x800) ) line_enable=1; else line_enable=0;
			if (f3_line_ram[0x200+(y>>1)]&bit_select1)
			{
				int line_ram_zoom=f3_line_ram[zoom_base/4]&0xffff;
				if (line_ram_zoom!=0)
				{
					x_zoom=0x10080 - line_ram_zoom;
					if (y_zoom==0 && line_enable) y_zoom=x_zoom;
				}
			}
			if (f3_line_ram[0x000+(y>>1)]&bit_select1)
				colscroll=(f3_line_ram[col_base/4]>> 0)&0x1ff;
		}
		else
		{
			if (f3_line_ram[0x300+(y>>1)]&bit_select0)
				x_offset=(f3_line_ram[line_base/4]&0xffff0000)>>6;
			if (f3_line_ram[0x380+(y>>1)]&bit_select0)
				pri=(f3_line_ram[pri_base/4]>>16)&0xffff;
			if (pri && !(pri&0x800) ) line_enable=1; else line_enable=0;
			if (f3_line_ram[0x200+(y>>1)]&bit_select0)
			{
				int line_ram_zoom=f3_line_ram[zoom_base/4]>>16;
				if (line_ram_zoom!=0)
				{
					x_zoom=0x10080 - line_ram_zoom;
					if (y_zoom==0 && line_enable) y_zoom=x_zoom;
				}
			}
			if (f3_line_ram[0x000+(y>>1)]&bit_select0)
				colscroll=(f3_line_ram[col_base/4]>>16)&0x1ff;
		}

		/* XYZoom? */
		if(y_zoom && line_enable && (colscroll!=0 || x_zoom!=y_zoom)) y_zoom=0x10000;


/*		if (line_enable)*/
		{
			if (!pri || (!flipscreen && y<24) || (flipscreen && y>231) ||
				(pri&0xc000)==0xc000 || !(pri&0x2000)/**/)
 				line_enable=0;
			else if(pri&0x4000)	/*alpha1*/
				line_enable=2;
			else if(pri&0x8000)	/*alpha2*/
				line_enable=3;
		  /*special case when the blend mode is "normal" but the 6200 area is used, Might be missing a flag*/
		  else if ((pri & 0x3000) && (f3_line_ram[0x6230/4] != 0) && (pos == 2) &&
				(((f3_line_ram[(0x6200/4) + (y>>1)] >> 4) & 0xf) != 0xb) &&
				(f3_line_ram[(0x6200/4) + (y>>1)] != 0x7777) && (f3_game == EACTION2))
		    {
			  line_enable = 0x22;
		    }
			else
				line_enable=1;
		}

		_colscroll[y]=colscroll;
		_x_offset[y]=(x_offset&0xffff0000) - (x_offset&0x0000ffff);

		line_t->x_zoom[y]=x_zoom;
		line_t->alpha_mode[y]=line_enable;
		line_t->pri[y]=pri;

		if(!pos)
		{
			if (y&1)
			{
				if (f3_line_ram[(0x0600/4)+(y>>1)]&0x8)
					spri=f3_line_ram[spri_base/4]&0xffff;
				if (f3_line_ram[(0x0400/4)+(y>>1)]&0x1)
					sprite_alpha=f3_line_ram[(spri_base-0x1600)/4]&0xffff;
				if (f3_line_ram[(0x0400/4)+(y>>1)]&0x2)
					alpha_level=f3_line_ram[(spri_base-0x1400)/4]&0xffff;
			}
			else
			{
				if (f3_line_ram[(0x0600/4)+(y>>1)]&0x80000)
					spri=f3_line_ram[spri_base/4]>>16;
				if (f3_line_ram[(0x0400/4)+(y>>1)]&0x10000)
					sprite_alpha=f3_line_ram[(spri_base-0x1600)/4]>>16;
				if (f3_line_ram[(0x0400/4)+(y>>1)]&0x20000)
					alpha_level=f3_line_ram[(spri_base-0x1400)/4]>>16;
			}

			line_t->alpha_level[y]=alpha_level;
			line_t->spri[y]=spri;
			line_t->sprite_alpha[y]=sprite_alpha;
		}

		zoom_base+=inc;
		line_base+=inc;
		col_base +=inc;
		pri_base +=inc;
		spri_base+=inc;
		y +=y_inc;
	}
	if(!y_zoom) y_zoom=0x10000;



	/* set pixmap pointer */

	srcbitmap = tilemap_get_pixmap(tilemap);
	transbitmap = tilemap_get_transparency_bitmap(tilemap);

	y=y_start;
	while(y!=y_end)
	{
		UINT32 x_index_fx;
		UINT32 y_index;

		if(line_t->alpha_mode[y]!=0)
		{
			UINT16 *src_s;
			UINT8 *tsrc_s;

			x_index_fx = (sx+_x_offset[y]-(10*0x10000)+10*line_t->x_zoom[y])&((width_mask<<16)|0xffff);
			y_index = ((y_index_fx>>16)+_colscroll[y])&0x1ff;

			/* check tile status */
			visible_tile_check(line_t,y,x_index_fx,y_index,f3_pf_data_n);
      
      if ((pos == 1) && (((f3_line_ram[(0x6200/4) + (y>>1)] >> 4) & 0xf) > 0xb) && (f3_game == EACTION2)) line_t->alpha_mode[y] = 0x22;  /* from shmupmame */

			/* set pixmap index */
			line_t->x_count[y]=x_index_fx & 0xffff;
			line_t->src_s[y]=src_s=(unsigned short *)srcbitmap->line[y_index];
			line_t->src_e[y]=&src_s[width_mask+1];
			line_t->src[y]=&src_s[x_index_fx>>16];

			line_t->tsrc_s[y]=tsrc_s=(unsigned char *)transbitmap->line[y_index];
			line_t->tsrc[y]=&tsrc_s[x_index_fx>>16];
		}

		if(y_zoom==line_t->x_zoom[y])
			y_index_fx += y_zoom;
		else
			y_index_fx += 0x10000;

		y +=y_inc;
	}
}

/******************************************************************************/

static void f3_tilemap_draw(struct mame_bitmap *bitmap,
							const struct rectangle *cliprect)
{
	int i,j,y,ys,ye;
	int y_start,y_end,y_start_next,y_end_next;
	UINT8 draw_line[256];
	INT16 draw_line_num[256];

	UINT32 rot=0;
#if DEBUG_F3
	int enable[4]={~0,~0,~0,~0};

	if(deb_enable)
	{
		if (keyboard_pressed(KEYCODE_Z)) enable[0]=0;
		if (keyboard_pressed(KEYCODE_X)) enable[1]=0;
		if (keyboard_pressed(KEYCODE_C)) enable[2]=0;
		if (keyboard_pressed(KEYCODE_V)) enable[3]=0;

		if (keyboard_pressed(KEYCODE_A)) sprite_pri_usage &= ~(1<<3);
		if (keyboard_pressed(KEYCODE_S)) sprite_pri_usage &= ~(1<<2);
		if (keyboard_pressed(KEYCODE_D)) sprite_pri_usage &= ~(1<<1);
		if (keyboard_pressed(KEYCODE_F)) sprite_pri_usage &= ~(1<<0);
	}
#endif	/*DEBUG_F3*/

	if (flipscreen)
	{
		rot=ORIENTATION_FLIP_Y;
		ys=0;
		ye=232;
	}
	else
	{
		ys=24;
		ye=256;
	}
/*	ys=0;*/
/*	ye=256;*/

#if DEBUG_F3
deb_loop=0;
deb_alpha_cnt=0;
#endif	/*DEBUG_F3*/

	y_start=ys;
	y_end=ye;
	memset(draw_line,0,256);

	while(1)
	{
		static int alpha_level_last=-1;
		int pos;
		int pri[4],alpha_mode[4],alpha_mode_flag[4],alpha_level;
		UINT16 sprite_alpha;
		UINT8 sprite_alpha_check;
		UINT8 sprite_alpha_all_2a;
		int spri;
		int alpha;
		int layer_tmp[4];

		int count_skip_layer=0;
		int sprite[5]={0,0,0,0,0};
		const struct f3_line_inf *line_t[4];


		/* find same status of scanlines */
		pri[0]=line_inf[0].pri[y_start];
		pri[1]=line_inf[1].pri[y_start];
		pri[2]=line_inf[2].pri[y_start];
		pri[3]=line_inf[3].pri[y_start];
		alpha_mode[0]=line_inf[0].alpha_mode[y_start];
		alpha_mode[1]=line_inf[1].alpha_mode[y_start];
		alpha_mode[2]=line_inf[2].alpha_mode[y_start];
		alpha_mode[3]=line_inf[3].alpha_mode[y_start];
		alpha_level=line_inf[0].alpha_level[y_start];
		spri=line_inf[0].spri[y_start];
		sprite_alpha=line_inf[0].sprite_alpha[y_start];

		draw_line[y_start]=1;
		draw_line_num[i=0]=y_start;
		y_start_next=-1;
		y_end_next=-1;
		for(y=y_start+1;y<y_end;y++)
		{
			if(!draw_line[y])
			{
				if(pri[0]!=line_inf[0].pri[y]) y_end_next=y+1;
				else if(pri[1]!=line_inf[1].pri[y]) y_end_next=y+1;
				else if(pri[2]!=line_inf[2].pri[y]) y_end_next=y+1;
				else if(pri[3]!=line_inf[3].pri[y]) y_end_next=y+1;
				else if(alpha_mode[0]!=line_inf[0].alpha_mode[y]) y_end_next=y+1;
				else if(alpha_mode[1]!=line_inf[1].alpha_mode[y]) y_end_next=y+1;
				else if(alpha_mode[2]!=line_inf[2].alpha_mode[y]) y_end_next=y+1;
				else if(alpha_mode[3]!=line_inf[3].alpha_mode[y]) y_end_next=y+1;
				else if(alpha_level!=line_inf[0].alpha_level[y]) y_end_next=y+1;
				else if(spri!=line_inf[0].spri[y]) y_end_next=y+1;
				else if(sprite_alpha!=line_inf[0].sprite_alpha[y]) y_end_next=y+1;
				else
				{
					draw_line[y]=1;
					draw_line_num[++i]=y;
					continue;
				}

				if(y_start_next<0) y_start_next=y;
			}
		}
		y_end=y_end_next;
		y_start=y_start_next;
		draw_line_num[++i]=-1;


		/* for clear top & bottom border */
		if(
			(!(pri[0]&0xf000) && !(pri[1]&0xf000) && !(pri[2]&0xf000) && !(pri[3]&0xf000)) ||
			((pri[0]&0x800) && (pri[1]&0x800) && (pri[2]&0x800) && (pri[3]&0x800))		/*KTIGER2*/
		  )
		{
			clear_scanlines(bitmap,46,320,draw_line_num,rot);
			if(y_start<0) break;
			continue;
		}

#if DEBUG_F3
deb_loop++;
#endif	/*DEBUG_F3*/


		/* alpha blend */
		alpha_mode_flag[0]=alpha_mode[0]&~3;
		alpha_mode_flag[1]=alpha_mode[1]&~3;
		alpha_mode_flag[2]=alpha_mode[2]&~3;
		alpha_mode_flag[3]=alpha_mode[3]&~3;
		alpha_mode[0]&=3;
		alpha_mode[1]&=3;
		alpha_mode[2]&=3;
		alpha_mode[3]&=3;
		if( alpha_mode[0]>1 ||
			alpha_mode[1]>1 ||
			alpha_mode[2]>1 ||
			alpha_mode[3]>1 ||
			(sprite_alpha&0xff) != 0xff  )
		{
			/* set alpha level */
			if(alpha_level!=alpha_level_last)
			{
				int al_s,al_d;
				int a=alpha_level;
				int b=(a>>8)&0xf;
				int c=(a>>4)&0xf;
				int d=(a>>0)&0xf;
				a>>=12;

				/* b000 7000 */
				al_s = ( (15-d)*256) / 8;
				al_d = ( (15-b)*256) / 8;
				if(al_s>255) al_s = 255;
				if(al_d>255) al_d = 255;
				f3_alpha_level_3as = al_s;
				f3_alpha_level_3ad = al_d;
				f3_alpha_level_2as = al_d;
				f3_alpha_level_2ad = al_s;

				al_s = ( (15-c)*256) / 8;
				al_d = ( (15-a)*256) / 8;
				if(al_s>255) al_s = 255;
				if(al_d>255) al_d = 255;
				f3_alpha_level_3bs = al_s;
				f3_alpha_level_3bd = al_d;
				f3_alpha_level_2bs = al_d;
				f3_alpha_level_2bd = al_s;

				f3_alpha_set_level();
				alpha_level_last=alpha_level;
			}


			/* set sprite alpha mode */
			sprite_alpha_check=0;
			sprite_alpha_all_2a=1;
			dpix_sp[1]=0;
			dpix_sp[2]=0;
			dpix_sp[4]=0;
			dpix_sp[8]=0;
			for(i=0;i<4;i++)	/* i = sprite priority offset */
			{
				UINT8 sprite_alpha_mode=(sprite_alpha>>(i*2))&3;
				UINT8 sftbit=1<<i;
				if(sprite_pri_usage&sftbit)
				{
					if(sprite_alpha_mode==1)
					{
						if(f3_alpha_level_2as==0 && f3_alpha_level_2ad==255) sprite_pri_usage&=~sftbit;
						else
						{
							dpix_sp[1<<i]=dpix_n[2];
							sprite_alpha_check|=sftbit;
						}
					}
					else if(sprite_alpha_mode==2)
					{
						if(sprite_alpha&0xff00)
						{
							if(f3_alpha_level_3as==0 && f3_alpha_level_3ad==255) sprite_pri_usage&=~sftbit;
							else
							{
								dpix_sp[1<<i]=dpix_n[3];
								sprite_alpha_check|=sftbit;
								sprite_alpha_all_2a=0;
							}
						}
						else
						{
							if(f3_alpha_level_3bs==0 && f3_alpha_level_3bd==255) sprite_pri_usage&=~sftbit;
							else
							{
								dpix_sp[1<<i]=dpix_n[5];
								sprite_alpha_check|=sftbit;
								sprite_alpha_all_2a=0;
							}
						}
					}
				}
			}


			/* check alpha level */
			for(i=0;i<4;i++)	/* i = playfield num (pos) */
			{
				int alpha_type = (alpha_mode_flag[i]>>4)&3;

				if(alpha_mode[i]==2)
				{
					if(alpha_type==1)
					{
						if     (f3_alpha_level_2as==0   && f3_alpha_level_2ad==255) alpha_mode[i]=0;
						else if(f3_alpha_level_2as==255 && f3_alpha_level_2ad==0  ) alpha_mode[i]=1;
					}
					else if(alpha_type==2)
					{
						if     (f3_alpha_level_2bs==0   && f3_alpha_level_2bd==255) alpha_mode[i]=0;
						else if(f3_alpha_level_2as==255 && f3_alpha_level_2ad==0 &&
								f3_alpha_level_2bs==255 && f3_alpha_level_2bd==0  ) alpha_mode[i]=1;
					}
					else if(alpha_type==3)
					{
						if     (f3_alpha_level_2as==0   && f3_alpha_level_2ad==255 &&
								f3_alpha_level_2bs==0   && f3_alpha_level_2bd==255) alpha_mode[i]=0;
						else if(f3_alpha_level_2as==255 && f3_alpha_level_2ad==0   &&
								f3_alpha_level_2bs==255 && f3_alpha_level_2bd==0  ) alpha_mode[i]=1;
					}
				}
				else if(alpha_mode[i]==3)
				{
					if(alpha_type==1)
					{
						if     (f3_alpha_level_3as==0   && f3_alpha_level_3ad==255) alpha_mode[i]=0;
						else if(f3_alpha_level_3as==255 && f3_alpha_level_3ad==0  ) alpha_mode[i]=1;
					}
					else if(alpha_type==2)
					{
						if     (f3_alpha_level_3bs==0   && f3_alpha_level_3bd==255) alpha_mode[i]=0;
						else if(f3_alpha_level_3as==255 && f3_alpha_level_3ad==0 &&
								f3_alpha_level_3bs==255 && f3_alpha_level_3bd==0  ) alpha_mode[i]=1;
					}
					else if(alpha_type==3)
					{
						if     (f3_alpha_level_3as==0   && f3_alpha_level_3ad==255 &&
								f3_alpha_level_3bs==0   && f3_alpha_level_3bd==255) alpha_mode[i]=0;
						else if(f3_alpha_level_3as==255 && f3_alpha_level_3ad==0   &&
								f3_alpha_level_3bs==255 && f3_alpha_level_3bd==0  ) alpha_mode[i]=1;
					}
				}
			}


			/* enable/disable alpha blend */
			if (alpha_disable)
			{
				if(alpha_mode[0]>1) alpha_mode[0]=1;
				if(alpha_mode[1]>1) alpha_mode[1]=1;
				if(alpha_mode[2]>1) alpha_mode[2]=1;
				if(alpha_mode[3]>1) alpha_mode[3]=1;
				sprite_alpha_check=0;
				dpix_sp[1]=0;
				dpix_sp[2]=0;
				dpix_sp[4]=0;
				dpix_sp[8]=0;
			}
			else
			{
				if (	(alpha_mode[0]==1 || alpha_mode[0]==2 || !alpha_mode[0]) &&
						(alpha_mode[1]==1 || alpha_mode[1]==2 || !alpha_mode[1]) &&
						(alpha_mode[2]==1 || alpha_mode[2]==2 || !alpha_mode[2]) &&
						(alpha_mode[3]==1 || alpha_mode[3]==2 || !alpha_mode[3]) &&
						sprite_alpha_all_2a						)
				{
					int alpha_type = (alpha_mode_flag[0] | alpha_mode_flag[1] | alpha_mode_flag[2] | alpha_mode_flag[3])&0x30;
					if(		(alpha_type==0x10 && f3_alpha_level_2as==255) ||
							(alpha_type==0x20 && f3_alpha_level_2as==255 && f3_alpha_level_2bs==255) ||
							(alpha_type==0x30 && f3_alpha_level_2as==255 && f3_alpha_level_2bs==255)	)
					{
						if(alpha_mode[0]>1) alpha_mode[0]=1;
						if(alpha_mode[1]>1) alpha_mode[1]=1;
						if(alpha_mode[2]>1) alpha_mode[2]=1;
						if(alpha_mode[3]>1) alpha_mode[3]=1;
						sprite_alpha_check=0;
						dpix_sp[1]=0;
						dpix_sp[2]=0;
						dpix_sp[4]=0;
						dpix_sp[8]=0;
					}
				}
			}
		}
		else
		{
			sprite_alpha_check=0;
			dpix_sp[1]=0;
			dpix_sp[2]=0;
			dpix_sp[4]=0;
			dpix_sp[8]=0;
		}



		/* set scanline priority */
		{
			int pri_max_opa=-1;
			for(i=0;i<4;i++)	/* i = playfield num (pos) */
			{
				int p0=pri[i];
				int pri_sl1=p0&0x0f;
				int pri_sl2=(p0&0xf0)>>4;

				if (f3_game == ARKRETRN && (p0&0xf00)==0x300)			/*???*/
					pri_sl1=pri_sl2<pri_sl1 ? pri_sl2 : pri_sl1;

				layer_tmp[i]=i + (pri_sl1<<2);

				if(!alpha_mode[i]
#if DEBUG_F3
					|| !enable[i]
#endif	/*DEBUG_F3*/
					/*|| !(pri[i]&0x2000)*/)
				{
					layer_tmp[i]|=0x40;
					count_skip_layer++;
				}
				else if(alpha_mode[i]==1 && (alpha_mode_flag[i]&0x80))
				{
					if(layer_tmp[i]>pri_max_opa) pri_max_opa=layer_tmp[i];
				}
			}

			if(pri_max_opa!=-1)
			{
				if(pri_max_opa>layer_tmp[0]) {layer_tmp[0]|=0x40;count_skip_layer++;}
				if(pri_max_opa>layer_tmp[1]) {layer_tmp[1]|=0x40;count_skip_layer++;}
				if(pri_max_opa>layer_tmp[2]) {layer_tmp[2]|=0x40;count_skip_layer++;}
				if(pri_max_opa>layer_tmp[3]) {layer_tmp[3]|=0x40;count_skip_layer++;}
			}
		}


		/* sort layer_tmp */
		for(i=0;i<3;i++)
		{
			for(j=i+1;j<4;j++)
			{
				if(layer_tmp[i]<layer_tmp[j])
				{
					int temp = layer_tmp[i];
					layer_tmp[i] = layer_tmp[j];
					layer_tmp[j] = temp;
				}
			}
		}


		/* check sprite & layer priority */
		{
			int l0,l1,l2,l3;
			int pri_sp[4];

			l0=layer_tmp[0]>>2;
			l1=layer_tmp[1]>>2;
			l2=layer_tmp[2]>>2;
			l3=layer_tmp[3]>>2;

			pri_sp[0]=spri&0xf;
			pri_sp[1]=(spri>>4)&0xf;
			pri_sp[2]=(spri>>8)&0xf;
			pri_sp[3]=spri>>12;

			for(i=0;i<4;i++)	/* i = sprite priority offset */
			{
				int sp,sflg=1<<i;
				if(!(sprite_pri_usage & sflg)) continue;
				sp=pri_sp[i];

				/*
					sprite priority==playfield priority
						BUBSYMPH (title)       ---> sprite
						DARIUSG (ZONE V' BOSS) ---> playfield
				*/

				if (f3_game == BUBSYMPH ) sp++;		/*BUBSYMPH (title)*/

					 if(		  sp>l0) sprite[0]|=sflg;
				else if(sp<=l0 && sp>l1) sprite[1]|=sflg;
				else if(sp<=l1 && sp>l2) sprite[2]|=sflg;
				else if(sp<=l2 && sp>l3) sprite[3]|=sflg;
				else if(sp<=l3		   ) sprite[4]|=sflg;
			}
		}


		/* draw scanlines */
		alpha=0;
		for(i=count_skip_layer;i<4;i++)
		{
			pos=layer_tmp[i]&3;
			line_t[i]=&line_inf[pos];

			if(sprite[i]&sprite_alpha_check) alpha=1;
			else if(!alpha) sprite[i]|=0x100;

			if(alpha_mode[pos]>1)
			{
				int alpha_type=(((alpha_mode_flag[pos]>>4)&3)-1)*2;
				dpix_lp[i]=dpix_n[alpha_mode[pos]+alpha_type];
				alpha=1;
			}
			else
			{
#if DEBUG_F3
				if(!alpha_mode[pos]) usrintf_showmessage("layer priority error");
#endif	/*DEBUG_F3*/
				if(alpha) dpix_lp[i]=dpix_n[1];
				else	  dpix_lp[i]=dpix_n[0];
			}
		}
		if(sprite[4]&sprite_alpha_check) alpha=1;
		else if(!alpha) sprite[4]|=0x100;

#if DEBUG_F3
		if(alpha) deb_alpha_cnt++;
		if(deb_enable)
			sprintf(deb_buf[8],"spr:%3x %3x %3x %3x %3x",sprite[0],
														sprite[1],
														sprite[2],
														sprite[3],
														sprite[4]);
#endif	/*DEBUG_F3*/

		f3_drawscanlines(bitmap,46,320,draw_line_num,line_t,sprite,rot,count_skip_layer);
		if(y_start<0) break;
	}
}

/******************************************************************************/

static void f3_update_pivot_layer(void)
{
	struct rectangle pivot_clip;
	int tile,i,pivot_base;

	/* A poor way to guess if the pivot layer is enabled, but quicker than
		parsing control ram. */
	int ctrl  = f3_line_ram[0x180f]&0xa000; /* SpcInvDX sets only this address */
	int ctrl2 = f3_line_ram[0x1870]&0xa0000000; /* SpcInvDX flipscreen */
	int ctrl3 = f3_line_ram[0x1820]&0xa000; /* Other games set the whole range 0x6000-0x61ff */
	int ctrl4 = f3_line_ram[0x1840]&0xa000; /* ScFinals only sets a small range over the screen area */

	/* Quickly decide whether to process the rest of the pivot layer */
	if (!(ctrl || ctrl2 || ctrl3 || ctrl4)) {
		tilemap_set_enable(pixel_layer,0);
		return;
	}
	tilemap_set_enable(pixel_layer,1);

	if (flipscreen) {
		tilemap_set_scrollx( pixel_layer,0,(f3_control_1[2]>>16)-(512-320)-16);
		tilemap_set_scrolly( pixel_layer,0,-(f3_control_1[2]&0xff));
	} else {
		tilemap_set_scrollx( pixel_layer,0,-(f3_control_1[2]>>16)-5);
		tilemap_set_scrolly( pixel_layer,0,-(f3_control_1[2]&0xff));
	}

	/* Clip top scanlines according to line ram - Bubble Memories makes use of this */
	pivot_clip.min_x=0;/*Machine->visible_area.min_x;*/
	pivot_clip.max_x=512;/*Machine->visible_area.max_x;*/
	pivot_clip.min_y=Machine->visible_area.min_y;
	pivot_clip.max_y=Machine->visible_area.max_y;
	if (flipscreen)
		pivot_base=0x61fe;
	else
		pivot_base=0x6000;

	for (i=0; i<256; i++) {
		/* Loop through until first visible line */
		if (pivot_base&2) {
			if ((f3_line_ram[pivot_base/4]&0xa000)==0xa000) {
				pivot_clip.min_y=i;
				i=256;
			}
		} else {
			if ((f3_line_ram[pivot_base/4]&0xa0000000)==0xa0000000) {
				pivot_clip.min_y=i;
				i=256;
			}
		}
		if (flipscreen) pivot_base-=2; else pivot_base+=2;
	}

	if (!flipscreen)
		pixel_layer_clip = pivot_clip;

	/* Decode chars & mark tilemap dirty */
	if (pivot_changed)
		for (tile = 0;tile < 2048;tile++)
			if (pivot_dirty[tile]) {
				decodechar(Machine->gfx[3],tile,(UINT8 *)f3_pivot_ram,Machine->drv->gfxdecodeinfo[3].gfxlayout);
				tilemap_mark_tile_dirty(pixel_layer,tile);
				pivot_dirty[tile]=0;
			}
	pivot_changed=0;
}

/******************************************************************************/

static void f3_draw_vram_layer(struct mame_bitmap *bitmap,const struct rectangle *cliprect)
{
	int offs,mx,my,tile,color,fx,fy,sx,sy;

	sx=(f3_control_1[2]>>16)+5;
	sy=f3_control_1[2]&0xffff;

   	for (offs = 0; offs < 0x2000 ;offs += 2)
	{
		mx = (offs%128)/2;
		my = offs/128;

		if (offs&2)
        	tile = videoram32[offs>>2]&0xffff;
		else
			tile = videoram32[offs>>2]>>16;

        /* Transparency hack, 6010 for PB2, 1205 for PB3 */
		if (tile==0x6010 || tile==0x1205 || tile==0x12be) continue;
		if (f3_game==RECALH && tile==0x12be) continue;

        color = (tile>>9) &0x3f;
		fx = tile&0x0100;
		fy = tile&0x8000;

        tile&=0xff;
        if (!tile) continue;

		/* Graphics flip */
		if (flipscreen) {
			if (fx) fx=0; else fx=1;
			if (fy) fy=0; else fy=1;
			drawgfx(bitmap,Machine->gfx[0],
					tile,
					color,
					fx,fy,
					504+17-(((8*mx)+sx)&0x1ff),504-(((8*my)+sy)&0x1ff),
					cliprect,TRANSPARENCY_PEN,0);
		}
		else
	        drawgfx(bitmap,Machine->gfx[0],
					tile,
					color,
					fx,fy,
					((8*mx)+sx)&0x1ff,((8*my)+sy)&0x1ff,
					cliprect,TRANSPARENCY_PEN,0);
	}
}

/******************************************************************************/

#define PSET_T					\
	c = *source;				\
	if(c)						\
	{							\
		p=*pri;					\
		if(!p || p==0xff)		\
		{						\
			*dest = pal[c];		\
			*pri = pri_dst;		\
		}						\
	}

#define PSET_O					\
	p=*pri;						\
	if(!p || p==0xff)			\
	{							\
		*dest = pal[*source];	\
		*pri = pri_dst;			\
	}

#define NEXT_P					\
	source += dx;				\
	dest++;						\
	pri++;

static INLINE void f3_drawgfx( struct mame_bitmap *dest_bmp,const struct GfxElement *gfx,
		unsigned int code,
		unsigned int color,
		int flipx,int flipy,
		int sx,int sy,
		const struct rectangle *clip,
		UINT8 pri_dst)
{
	struct rectangle myclip;

	pri_dst=1<<pri_dst;

	/* KW 991012 -- Added code to force clip to bitmap boundary */
	if(clip)
	{
		myclip.min_x = clip->min_x;
		myclip.max_x = clip->max_x;
		myclip.min_y = clip->min_y;
		myclip.max_y = clip->max_y;

		if (myclip.min_x < 0) myclip.min_x = 0;
		if (myclip.max_x >= dest_bmp->width) myclip.max_x = dest_bmp->width-1;
		if (myclip.min_y < 0) myclip.min_y = 0;
		if (myclip.max_y >= dest_bmp->height) myclip.max_y = dest_bmp->height-1;

		clip=&myclip;
	}


	if( gfx && gfx->colortable )
	{
		const pen_t *pal = &gfx->colortable[gfx->color_granularity * (color % gfx->total_colors)]; /* ASG 980209 */
/*		int palBase = &gfx->colortable[gfx->color_granularity * (color % gfx->total_colors)] - Machine->remapped_colortable;*/
		int source_base = (code % gfx->total_elements) * 16;

		{
			/* compute sprite increment per screen pixel */
			int dx = 1;
			int dy = 1;

			int ex = sx+16;
			int ey = sy+16;

			int x_index_base;
			int y_index;

			if( flipx )
			{
				x_index_base = 15;
				dx = -1;
			}
			else
			{
				x_index_base = 0;
			}

			if( flipy )
			{
				y_index = 15;
				dy = -1;
			}
			else
			{
				y_index = 0;
			}

			if( clip )
			{
				if( sx < clip->min_x)
				{ /* clip left */
					int pixels = clip->min_x-sx;
					sx += pixels;
					x_index_base += pixels*dx;
				}
				if( sy < clip->min_y )
				{ /* clip top */
					int pixels = clip->min_y-sy;
					sy += pixels;
					y_index += pixels*dy;
				}
				/* NS 980211 - fixed incorrect clipping */
				if( ex > clip->max_x+1 )
				{ /* clip right */
					int pixels = ex-clip->max_x-1;
					ex -= pixels;
				}
				if( ey > clip->max_y+1 )
				{ /* clip bottom */
					int pixels = ey-clip->max_y-1;
					ey -= pixels;
				}
			}

			if( ex>sx && ey>sy)
			{ /* skip if inner loop doesn't draw anything */
/*				if (dest_bmp->depth == 32)*/
				{
					int y=ey-sy;
					int x=(ex-sx-1)|(tile_opaque_sp[code]<<4);
					UINT8 *source0 = gfx->gfxdata + (source_base+y_index) * 16 + x_index_base;
					UINT32 *dest0 = (UINT32 *)dest_bmp->line[sy]+sx;
					UINT8 *pri0 = (UINT8 *)pri_alp_bitmap->line[sy]+sx;
					int yadv = dest_bmp->rowpixels;
					dy=dy*16;
					while(1)
					{
						UINT8 *source = source0;
						UINT32 *dest = dest0;
						UINT8 *pri = pri0;

						switch(x)
						{
							int c;
							UINT8 p;
							case 31: PSET_O NEXT_P
							case 30: PSET_O NEXT_P
							case 29: PSET_O NEXT_P
							case 28: PSET_O NEXT_P
							case 27: PSET_O NEXT_P
							case 26: PSET_O NEXT_P
							case 25: PSET_O NEXT_P
							case 24: PSET_O NEXT_P
							case 23: PSET_O NEXT_P
							case 22: PSET_O NEXT_P
							case 21: PSET_O NEXT_P
							case 20: PSET_O NEXT_P
							case 19: PSET_O NEXT_P
							case 18: PSET_O NEXT_P
							case 17: PSET_O NEXT_P
							case 16: PSET_O break;

							case 15: PSET_T NEXT_P
							case 14: PSET_T NEXT_P
							case 13: PSET_T NEXT_P
							case 12: PSET_T NEXT_P
							case 11: PSET_T NEXT_P
							case 10: PSET_T NEXT_P
							case  9: PSET_T NEXT_P
							case  8: PSET_T NEXT_P
							case  7: PSET_T NEXT_P
							case  6: PSET_T NEXT_P
							case  5: PSET_T NEXT_P
							case  4: PSET_T NEXT_P
							case  3: PSET_T NEXT_P
							case  2: PSET_T NEXT_P
							case  1: PSET_T NEXT_P
							case  0: PSET_T
						}

						if(!(--y)) break;
						source0 += dy;
						dest0+=yadv;
						pri0+=yadv;
					}
				}
			}
		}
	}
}
#undef PSET_T
#undef PSET_O
#undef NEXT_P


static INLINE void f3_drawgfxzoom( struct mame_bitmap *dest_bmp,const struct GfxElement *gfx,
		unsigned int code,
		unsigned int color,
		int flipx,int flipy,
		int sx,int sy,
		const struct rectangle *clip,
		int scalex, int scaley,
		UINT8 pri_dst)
{
	struct rectangle myclip;

	pri_dst=1<<pri_dst;

	/* KW 991012 -- Added code to force clip to bitmap boundary */
	if(clip)
	{
		myclip.min_x = clip->min_x;
		myclip.max_x = clip->max_x;
		myclip.min_y = clip->min_y;
		myclip.max_y = clip->max_y;

		if (myclip.min_x < 0) myclip.min_x = 0;
		if (myclip.max_x >= dest_bmp->width) myclip.max_x = dest_bmp->width-1;
		if (myclip.min_y < 0) myclip.min_y = 0;
		if (myclip.max_y >= dest_bmp->height) myclip.max_y = dest_bmp->height-1;

		clip=&myclip;
	}


	if( gfx && gfx->colortable )
	{
		const pen_t *pal = &gfx->colortable[gfx->color_granularity * (color % gfx->total_colors)]; /* ASG 980209 */
/*		int palBase = &gfx->colortable[gfx->color_granularity * (color % gfx->total_colors)] - Machine->remapped_colortable;*/
		int source_base = (code % gfx->total_elements) * 16;

		{
			/* compute sprite increment per screen pixel */
			int dx = (16<<16)/scalex;
			int dy = (16<<16)/scaley;

			int ex = sx+scalex;
			int ey = sy+scaley;

			int x_index_base;
			int y_index;

			if( flipx )
			{
				x_index_base = (scalex-1)*dx;
				dx = -dx;
			}
			else
			{
				x_index_base = 0;
			}

			if( flipy )
			{
				y_index = (scaley-1)*dy;
				dy = -dy;
			}
			else
			{
				y_index = 0;
			}

			if( clip )
			{
				if( sx < clip->min_x)
				{ /* clip left */
					int pixels = clip->min_x-sx;
					sx += pixels;
					x_index_base += pixels*dx;
				}
				if( sy < clip->min_y )
				{ /* clip top */
					int pixels = clip->min_y-sy;
					sy += pixels;
					y_index += pixels*dy;
				}
				/* NS 980211 - fixed incorrect clipping */
				if( ex > clip->max_x+1 )
				{ /* clip right */
					int pixels = ex-clip->max_x-1;
					ex -= pixels;
				}
				if( ey > clip->max_y+1 )
				{ /* clip bottom */
					int pixels = ey-clip->max_y-1;
					ey -= pixels;
				}
			}

			if( ex>sx )
			{ /* skip if inner loop doesn't draw anything */
/*				if (dest_bmp->depth == 32)*/
				{
					int y;
					for( y=sy; y<ey; y++ )
					{
						UINT8 *source = gfx->gfxdata + (source_base+(y_index>>16)) * 16;
						UINT32 *dest = (UINT32 *)dest_bmp->line[y];
						UINT8 *pri = pri_alp_bitmap->line[y];

						int x, x_index = x_index_base;
						for( x=sx; x<ex; x++ )
						{
							int c = source[x_index>>16];
							if(c)
							{
								UINT8 p=pri[x];
								if (p == 0 || p == 0xff)
								{
									dest[x] = pal[c];
									pri[x] = pri_dst;
								}
							}
							x_index += dx;
						}
						y_index += dy;
					}
				}
			}
		}
	}
}


#define CALC_ZOOM(p)	{										\
	p##_addition = 0x100 - block_zoom_##p + p##_addition_left;	\
	p##_addition_left = p##_addition & 0xf;						\
	p##_addition = p##_addition >> 4;							\
	/*zoom##p = p##_addition << 12;*/							\
}

static void get_sprite_info(const data32_t *spriteram32_ptr)
{
	const int min_x=Machine->visible_area.min_x,max_x=Machine->visible_area.max_x;
	const int min_y=Machine->visible_area.min_y,max_y=Machine->visible_area.max_y;
	int offs,spritecont,flipx,flipy,old_x,old_y,color,x,y;
	int sprite,global_x=0,global_y=0,subglobal_x=0,subglobal_y=0;
	int block_x=0, block_y=0;
	int last_color=0,last_x=0,last_y=0,block_zoom_x=0,block_zoom_y=0;
	int this_x,this_y;
	int y_addition=16, x_addition=16;
	int multi=0;
	int sprite_top;

	int x_addition_left = 8, y_addition_left = 8;

	struct tempsprite *sprite_ptr = spritelist;

	int total_sprites=0;

	color=0;
    flipx=flipy=0;
    old_y=old_x=0;
    y=x=0;

	sprite_top=0x1000;
	for (offs = 0; offs < sprite_top && (total_sprites < 0x400); offs += 4)
	{
		const int current_offs=offs; /* Offs can change during loop, current_offs cannot */

		/* Check if the sprite list jump command bit is set */
		if ((spriteram32_ptr[current_offs+3]>>16) & 0x8000) {
			data32_t jump = (spriteram32_ptr[current_offs+3]>>16)&0x3ff;

			data32_t new_offs=((offs&0x2000)|((jump<<4)/4));
			if (new_offs==offs)
				break;
			offs=new_offs - 4;
		}


		/* Check if special command bit is set */
		if (spriteram32_ptr[current_offs+1] & 0x8000) {
			data32_t cntrl=(spriteram32_ptr[current_offs+2])&0xffff;
			flipscreen=cntrl&0x2000;

			/*	cntrl&0x1000 = disabled?  (From F2 driver, doesn't seem used anywhere)
				cntrl&0x0010 = ???
				cntrl&0x0020 = ???
			*/

			/* Sprite bank select */
			if (cntrl&1) {
				offs=offs|0x2000;
				sprite_top=sprite_top|0x2000;
			}
		}

		/* Set global sprite scroll */
		if (((spriteram32_ptr[current_offs+1]>>16) & 0xf000) == 0xa000) {
			global_x = (spriteram32_ptr[current_offs+1]>>16) & 0xfff;
			if (global_x >= 0x800) global_x -= 0x1000;
			global_y = spriteram32_ptr[current_offs+1] & 0xfff;
			if (global_y >= 0x800) global_y -= 0x1000;
		}

		/* And sub-global sprite scroll */
		if (((spriteram32_ptr[current_offs+1]>>16) & 0xf000) == 0x5000) {
			subglobal_x = (spriteram32_ptr[current_offs+1]>>16) & 0xfff;
			if (subglobal_x >= 0x800) subglobal_x -= 0x1000;
			subglobal_y = spriteram32_ptr[current_offs+1] & 0xfff;
			if (subglobal_y >= 0x800) subglobal_y -= 0x1000;
		}

		if (((spriteram32_ptr[current_offs+1]>>16) & 0xf000) == 0xb000) {
			subglobal_x = (spriteram32_ptr[current_offs+1]>>16) & 0xfff;
			if (subglobal_x >= 0x800) subglobal_x -= 0x1000;
			subglobal_y = spriteram32_ptr[current_offs+1] & 0xfff;
			if (subglobal_y >= 0x800) subglobal_y -= 0x1000;
			global_y=subglobal_y;
			global_x=subglobal_x;
		}

		/* A real sprite to process! */
		sprite = (spriteram32_ptr[current_offs]>>16) | ((spriteram32_ptr[current_offs+2]&1)<<16);
		spritecont = spriteram32_ptr[current_offs+2]>>24;

/* These games either don't set the XY control bits properly (68020 bug?), or
	have some different mode from the others */
#ifdef DARIUSG_KLUDGE
		if (f3_game==DARIUSG || f3_game==GEKIRIDO || f3_game==CLEOPATR || f3_game==RECALH)
			multi=spritecont&0xf0;
#endif

		/* Check if this sprite is part of a continued block */
		if (multi) {
			/* Bit 0x4 is 'use previous colour' for this block part */
			if (spritecont&0x4) color=last_color;
			else color=(spriteram32_ptr[current_offs+2]>>16)&0xff;

#ifdef DARIUSG_KLUDGE
			if (f3_game==DARIUSG || f3_game==GEKIRIDO || f3_game==CLEOPATR || f3_game==RECALH) {
				/* Adjust X Position */
				if ((spritecont & 0x40) == 0) {
					if (spritecont & 0x4) {
						x = block_x;
					} else {
						this_x = spriteram32_ptr[current_offs+1]>>16;
						if (this_x&0x800) this_x= 0 - (0x800 - (this_x&0x7ff)); else this_x&=0x7ff;

						if ((spriteram32_ptr[current_offs+1]>>16)&0x8000) {
							this_x+=0;
						} else if ((spriteram32_ptr[current_offs+1]>>16)&0x4000) {
							/* Ignore subglobal (but apply global) */
							this_x+=global_x;
						} else { /* Apply both scroll offsets */
							this_x+=global_x+subglobal_x;
						}

						x = block_x = this_x;
					}
					x_addition_left = 8;
					CALC_ZOOM(x)
				}
				else if ((spritecont & 0x80) != 0) {
					x = last_x+x_addition;
					CALC_ZOOM(x)
				}

				/* Adjust Y Position */
				if ((spritecont & 0x10) == 0) {
					if (spritecont & 0x4) {
						y = block_y;
					} else {
						this_y = spriteram32_ptr[current_offs+1]&0xffff;
						if (this_y&0x800) this_y= 0 - (0x800 - (this_y&0x7ff)); else this_y&=0x7ff;

						if ((spriteram32_ptr[current_offs+1]>>16)&0x8000) {
							this_y+=0;
						} else if ((spriteram32_ptr[current_offs+1]>>16)&0x4000) {
							/* Ignore subglobal (but apply global) */
							this_y+=global_y;
						} else { /* Apply both scroll offsets */
							this_y+=global_y+subglobal_y;
						}

						y = block_y = this_y;
					}
					y_addition_left = 8;
					CALC_ZOOM(y)
				}
				else if ((spritecont & 0x20) != 0) {
					y = last_y+y_addition;
					CALC_ZOOM(y)
				}
			} else
#endif
			{
				/* Adjust X Position */
				if ((spritecont & 0x40) == 0) {
					x = block_x;
					x_addition_left = 8;
					CALC_ZOOM(x)
				}
				else if ((spritecont & 0x80) != 0) {
					x = last_x+x_addition;
					CALC_ZOOM(x)
				}
				/* Adjust Y Position */
				if ((spritecont & 0x10) == 0) {
					y = block_y;
					y_addition_left = 8;
					CALC_ZOOM(y)
				}
				else if ((spritecont & 0x20) != 0) {
					y = last_y+y_addition;
					CALC_ZOOM(y)
				}
				/* Both zero = reread block latch? */
			}
		}
		/* Else this sprite is the possible start of a block */
		else {
			color = (spriteram32_ptr[current_offs+2]>>16)&0xff;
			last_color=color;

			/* Sprite positioning */
			this_y = spriteram32_ptr[current_offs+1]&0xffff;
			this_x = spriteram32_ptr[current_offs+1]>>16;
			if (this_y&0x800) this_y= 0 - (0x800 - (this_y&0x7ff)); else this_y&=0x7ff;
			if (this_x&0x800) this_x= 0 - (0x800 - (this_x&0x7ff)); else this_x&=0x7ff;

			/* Ignore both scroll offsets for this block */
			if ((spriteram32_ptr[current_offs+1]>>16)&0x8000) {
				this_x+=0;
				this_y+=0;
			} else if ((spriteram32_ptr[current_offs+1]>>16)&0x4000) {
				/* Ignore subglobal (but apply global) */
				this_x+=global_x;
				this_y+=global_y;
			} else { /* Apply both scroll offsets */
				this_x+=global_x+subglobal_x;
				this_y+=global_y+subglobal_y;
			}

	        block_y = y = this_y;
            block_x = x = this_x;

			block_zoom_x=spriteram32_ptr[current_offs];
			block_zoom_y=(block_zoom_x>>8)&0xff;
			block_zoom_x&=0xff;

			x_addition_left = 8;
			CALC_ZOOM(x)

			y_addition_left = 8;
			CALC_ZOOM(y)
		}

		/* These features are common to sprite and block parts */
  		flipx = spritecont&0x1;
		flipy = spritecont&0x2;
		multi = spritecont&0x8;
		last_x=x;
		last_y=y;

		if (!sprite) continue;
		if (!x_addition || !y_addition) continue;

		if (flipscreen)
		{
			int tx,ty;
			if (f3_game == GSEEKER )
			{
				tx = 512-x_addition-x-44;
				ty = 256-y_addition-y+17;
			}
			else
			{
				tx = 512-x_addition-x;
				ty = 256-y_addition-y;
			}
			if (tx+x_addition<=min_x || tx>max_x || ty+y_addition<=min_y || ty>max_y) continue;
			sprite_ptr->x = tx;
			sprite_ptr->y = ty;
			sprite_ptr->flipx = !flipx;
			sprite_ptr->flipy = !flipy;
		}
		else
		{
			if (x+x_addition<=min_x || x>max_x || y+y_addition<=min_y || y>max_y) continue;
			sprite_ptr->x = x;
			sprite_ptr->y = y;
			sprite_ptr->flipx = flipx;
			sprite_ptr->flipy = flipy;
		}


		sprite_ptr->code = sprite;
		sprite_ptr->color = color;
		sprite_ptr->zoomx = x_addition;
		sprite_ptr->zoomy = y_addition;
		sprite_ptr->pri = (color & 0xc0) >> 6;
		sprite_ptr++;
		total_sprites++;
	}
	sprite_end = sprite_ptr;
}
#undef CALC_ZOOM


static void f3_drawsprites(struct mame_bitmap *bitmap,const struct rectangle *cliprect)
{
	const struct tempsprite *sprite_ptr;
	const struct GfxElement *sprite_gfx = Machine->gfx[2];

	sprite_ptr = sprite_end;
	sprite_pri_usage=0;
	while (sprite_ptr != spritelist)
	{
		int pri;
		sprite_ptr--;

		pri=sprite_ptr->pri;
		sprite_pri_usage|=1<<pri;

		if(sprite_ptr->zoomx==16 && sprite_ptr->zoomy==16)
			f3_drawgfx(bitmap,sprite_gfx,
					sprite_ptr->code,
					sprite_ptr->color,
					sprite_ptr->flipx,sprite_ptr->flipy,
					sprite_ptr->x,sprite_ptr->y,
					cliprect,
					pri);
		else
			f3_drawgfxzoom(bitmap,sprite_gfx,
					sprite_ptr->code,
					sprite_ptr->color,
					sprite_ptr->flipx,sprite_ptr->flipy,
					sprite_ptr->x,sprite_ptr->y,
					cliprect,
					sprite_ptr->zoomx,sprite_ptr->zoomy,
					pri);
	}
}

/******************************************************************************/

VIDEO_UPDATE( f3 )
{
	struct rectangle tempclip;
	unsigned int sy_fix[4],sx_fix[4];
	int tile;
#if DEBUG_F3
	static int deb_sc_x=0,deb_sc_y=0;
#endif	/*DEBUG_F3*/
	struct mame_bitmap *priority_bitmap_bak;

	skip_this_frame=0;
	tilemap_set_flip(ALL_TILEMAPS,flipscreen ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);

	/* Dynamically decode VRAM chars if dirty */
	if (vram_changed)
		for (tile = 0;tile < 256;tile++)
			if (vram_dirty[tile]) {
				decodechar(Machine->gfx[0],tile,(UINT8 *)f3_vram,Machine->drv->gfxdecodeinfo[0].gfxlayout);
				vram_dirty[tile]=0;
			}
	vram_changed=0;

/*	if (scroll_dirty)*/
	{
		sy_fix[0]=((f3_control_0[2]&0xffff0000)>> 7)+(scroll_kludge_y<<16);
		sy_fix[1]=((f3_control_0[2]&0x0000ffff)<< 9)+((scroll_kludge_y+pf23_y_kludge)<<16);
		sy_fix[2]=((f3_control_0[3]&0xffff0000)>> 7)+((scroll_kludge_y+pf23_y_kludge)<<16);
		sy_fix[3]=((f3_control_0[3]&0x0000ffff)<< 9)+(scroll_kludge_y<<16);
		sx_fix[0]=((f3_control_0[0]&0xffc00000)>> 6)-((6+scroll_kludge_x)<<16);
		sx_fix[1]=((f3_control_0[0]&0x0000ffc0)<<10)-((10+scroll_kludge_x)<<16);
		sx_fix[2]=((f3_control_0[1]&0xffc00000)>> 6)-((14+scroll_kludge_x)<<16);
		sx_fix[3]=((f3_control_0[1]&0x0000ffc0)<<10)-((18+scroll_kludge_x)<<16);

		sx_fix[0]-=((f3_control_0[0]&0x003f0000)>> 6)+0x0400-0x10000;
		sx_fix[1]-=((f3_control_0[0]&0x0000003f)<<10)+0x0400-0x10000;
		sx_fix[2]-=((f3_control_0[1]&0x003f0000)>> 6)+0x0400-0x10000;
		sx_fix[3]-=((f3_control_0[1]&0x0000003f)<<10)+0x0400-0x10000;

#if DEBUG_F3
		sy_fix[0]+=deb_sc_y;
		sy_fix[1]+=deb_sc_y;
		sy_fix[2]+=deb_sc_y;
		sy_fix[3]+=deb_sc_y;
		sx_fix[0]+=deb_sc_x;
		sx_fix[1]+=deb_sc_x;
		sx_fix[2]+=deb_sc_x;
		sx_fix[3]+=deb_sc_x;
#endif	/*DEBUG_F3*/

		if (flipscreen)
		{
			sy_fix[0]= 0x3000000-sy_fix[0];
			sy_fix[1]= 0x3000000-sy_fix[1];
			sy_fix[2]= 0x3000000-sy_fix[2];
			sy_fix[3]= 0x3000000-sy_fix[3];
			sx_fix[0]=-0x1a00000-sx_fix[0];
			sx_fix[1]=-0x1a00000-sx_fix[1];
			sx_fix[2]=-0x1a00000-sx_fix[2];
			sx_fix[3]=-0x1a00000-sx_fix[3];
		}
	}
/*	scroll_dirty=1;// 0*/

	/* Update pivot layer */
	f3_update_pivot_layer();


	fillbitmap(pri_alp_bitmap,0,cliprect);
#if DEBUG_F3
if (deb_enable) fillbitmap(bitmap,255,cliprect);
#endif	/*DEBUG_F3*/

	/* Pixel layer */
	tempclip = pixel_layer_clip;
	sect_rect(&tempclip,cliprect);
#if DEBUG_F3
if (!deb_enable || !keyboard_pressed(KEYCODE_N))
#endif	/*DEBUG_F3*/
{
	priority_bitmap_bak=priority_bitmap;
	priority_bitmap=pri_alp_bitmap;
	tilemap_draw(bitmap,&tempclip,pixel_layer,0,0xff);
	priority_bitmap=priority_bitmap_bak;
}


	/* sprites */
	if (sprite_lag==0)
	{
		get_sprite_info(spriteram32);
	}

#if DEBUG_F3
if (!deb_enable || !keyboard_pressed(KEYCODE_B))
#endif	/*DEBUG_F3*/
{
	f3_drawsprites(bitmap,cliprect);
}

#if DEBUG_F3
deb_tileflag=0;
if (deb_enable && keyboard_pressed(KEYCODE_G))
{
	deb_tileflag=1;
	tilemap_mark_all_tiles_dirty( pf1_tilemap );
	tilemap_mark_all_tiles_dirty( pf2_tilemap );
	tilemap_mark_all_tiles_dirty( pf3_tilemap );
	tilemap_mark_all_tiles_dirty( pf4_tilemap );
}
#endif	/*DEBUG_F3*/

	/* Playfield */
	get_line_ram_info(pf1_tilemap,sx_fix[0],sy_fix[0],0,f3_pf_data_1);
	get_line_ram_info(pf2_tilemap,sx_fix[1],sy_fix[1],1,f3_pf_data_2);
	get_line_ram_info(pf3_tilemap,sx_fix[2],sy_fix[2],2,f3_pf_data_3);
	get_line_ram_info(pf4_tilemap,sx_fix[3],sy_fix[3],3,f3_pf_data_4);

	f3_tilemap_draw(bitmap,cliprect);


	/* vram layer */
#if DEBUG_F3
if (!deb_enable || !keyboard_pressed(KEYCODE_M))
#endif	/*DEBUG_F3*/
	f3_draw_vram_layer(bitmap,cliprect);


#if DEBUG_F3
	if (0 && keyboard_pressed(KEYCODE_O))
		print_debug_info(0,0,0,0,0,0,0,0);
#endif	/*DEBUG_F3*/


	if (!keyboard_pressed(KEYCODE_LSHIFT) && keyboard_pressed_memory(KEYCODE_F1))
	{
		alpha_disable=!alpha_disable;
		if(alpha_disable)
			usrintf_showmessage("alpha blending:off");
		else usrintf_showmessage("alpha blending:on");
	}

#if DEBUG_F3
	{/******************************************************************************/
		static int debdisp = 0;
		static int cz_pos=0,cz_line=24;

		if (keyboard_pressed(KEYCODE_LSHIFT) && keyboard_pressed_memory(KEYCODE_F1))
		{
			deb_enable=!deb_enable;
			if(!deb_enable)
			{
				debdisp = 0;
				usrintf_showmessage("debug mode:off");
			}
			else usrintf_showmessage("debug mode:on");

		}

		if (deb_enable && keyboard_pressed_memory(KEYCODE_Q))
		{
			debdisp++;
			if(debdisp==4) debdisp = 0;
		}

		if(debdisp)
		{
			int sft;
			if (keyboard_pressed(KEYCODE_K))	cz_line--;
			if (keyboard_pressed(KEYCODE_L))	cz_line++;
			if (keyboard_pressed_memory(KEYCODE_I))	cz_line--;
			if (keyboard_pressed_memory(KEYCODE_O))	cz_line++;
			cz_line=cz_line & 0xff;
			sft=16*~(cz_line & 1);

			if(debdisp==2)
			{
				sprintf(deb_buf[0],"LINE:%3d Z:%04x R:%04x C:%04x",cz_line,
									(f3_line_ram[(0x0800+cz_line*2)/4]>>sft)&0xffff,
									(f3_line_ram[(0x0c00+cz_line*2)/4]>>sft)&0xffff,
									(f3_line_ram[(0x0000+cz_line*2)/4]>>sft)&0xffff
						);

				for(cz_pos=0;cz_pos<4;cz_pos++)
					sprintf(deb_buf[1+cz_pos],"Layer:%2d z:%04x r:%04x c:%04x",cz_pos,
									(f3_line_ram[(0x8000+cz_pos*256*2+cz_line*2)/4]>>sft)&0xffff,
									(f3_line_ram[(0xa000+cz_pos*256*2+cz_line*2)/4]>>sft)&0xffff,
									(f3_line_ram[(0x4000+cz_pos*256*2+cz_line*2)/4]>>sft)&0xffff
							);
			}
			else if(debdisp==3)
			{
				int deb_sx[4],deb_sx_fix[4];
				int deb_sy[4],deb_sy_fix[4];

				deb_sx[0]=(f3_control_0[0]&0xffc00000)>>22;
				deb_sx[1]=(f3_control_0[0]&0x0000ffc0)>>6;
				deb_sx[2]=(f3_control_0[1]&0xffc00000)>>22;
				deb_sx[3]=(f3_control_0[1]&0x0000ffc0)>>6;

				deb_sx_fix[0]=(f3_control_0[0]&0x003f0000)>>14;
				deb_sx_fix[1]=(f3_control_0[0]&0x0000003f)<<2;
				deb_sx_fix[2]=(f3_control_0[1]&0x003f0000)>>14;
				deb_sx_fix[3]=(f3_control_0[1]&0x0000003f)<<2;

				deb_sy[0]=(f3_control_0[2]&0xffff0000)>>23;
				deb_sy[1]=(f3_control_0[2]&0x0000ffff)>>7;
				deb_sy[2]=(f3_control_0[3]&0xffff0000)>>23;
				deb_sy[3]=(f3_control_0[3]&0x0000ffff)>>7;

				deb_sy_fix[0]=(f3_control_0[2]&0x007f0000)>>15;
				deb_sy_fix[1]=(f3_control_0[2]&0x0000007f)<<1;
				deb_sy_fix[2]=(f3_control_0[3]&0x007f0000)>>15;
				deb_sy_fix[3]=(f3_control_0[3]&0x0000007f)<<1;

				for(cz_pos=0;cz_pos<4;cz_pos++)
					sprintf(deb_buf[1+cz_pos],"Layer:%2d x:%03x.%02x y:%03x.%02x",cz_pos,
									deb_sx[cz_pos],deb_sx_fix[cz_pos],
									deb_sy[cz_pos],deb_sy_fix[cz_pos]
							);
			}
			else if(debdisp==1)
			{
				sprintf(deb_buf[0],"LINE:%3d S:%04x T:%04x A:%04x ?:%04x",cz_line,
									(f3_line_ram[(0x0600+cz_line*2)/4]>>sft)&0xffff,
									(f3_line_ram[(0x0e00+cz_line*2)/4]>>sft)&0xffff,
									(f3_line_ram[(0x0400+cz_line*2)/4]>>sft)&0xffff,
									(f3_line_ram[(0x0200+cz_line*2)/4]>>sft)&0xffff
						);

				for(cz_pos=0;cz_pos<4;cz_pos++)
					sprintf(deb_buf[1+cz_pos],"Layer:%2d s:%04x t:%04x a:%04x ?:%04x",cz_pos,
										(f3_line_ram[(0x7000+cz_pos*256*2+cz_line*2)/4]>>sft)&0xffff,
										(f3_line_ram[(0xb000+cz_pos*256*2+cz_line*2)/4]>>sft)&0xffff,
										(f3_line_ram[(0x6000+cz_pos*256*2+cz_line*2)/4]>>sft)&0xffff,
										(f3_line_ram[(0x5000+cz_pos*256*2+cz_line*2)/4]>>sft)&0xffff
							);
			}

			if(1)
			{
				if (keyboard_pressed_memory(KEYCODE_0_PAD))	deb_sc_x-=0x0400;
				if (keyboard_pressed_memory(KEYCODE_DEL_PAD))	deb_sc_x+=0x0400;
				if (keyboard_pressed_memory(KEYCODE_1_PAD))	deb_sc_x-=0x10000;
				if (keyboard_pressed_memory(KEYCODE_2_PAD))	deb_sc_x+=0x10000;

				if (keyboard_pressed_memory(KEYCODE_4_PAD))	deb_sc_y-=0x0200;
				if (keyboard_pressed_memory(KEYCODE_5_PAD))	deb_sc_y+=0x0200;
				if (keyboard_pressed_memory(KEYCODE_7_PAD))	deb_sc_y-=0x10000;
				if (keyboard_pressed_memory(KEYCODE_8_PAD))	deb_sc_y+=0x10000;
				sprintf(deb_buf[5],"sc offset x:%8x y:%8x",deb_sc_x,deb_sc_y);

				sprintf(deb_buf[6],"flip:%d alp/loop:%2d/%2d",!(!flipscreen),deb_alpha_cnt,deb_loop);

				if (keyboard_pressed_memory(KEYCODE_H))	deb_tile_code--;
				if (keyboard_pressed_memory(KEYCODE_J))	deb_tile_code++;
				deb_tile_code&=0x1f;
				sprintf(deb_buf[7],"tile code flg:%02x",deb_tile_code);
			}
			else
			{
				if (keyboard_pressed(KEYCODE_0_PAD))	deb_alpha_level_a-=1;
				if (keyboard_pressed(KEYCODE_DEL_PAD))	deb_alpha_level_a+=1;
				if (keyboard_pressed_memory(KEYCODE_1_PAD))	deb_alpha_level_a-=1;
				if (keyboard_pressed_memory(KEYCODE_2_PAD))	deb_alpha_level_a+=1;

				if (keyboard_pressed(KEYCODE_4_PAD))	deb_alpha_level_b-=1;
				if (keyboard_pressed(KEYCODE_5_PAD))	deb_alpha_level_b+=1;
				if (keyboard_pressed_memory(KEYCODE_7_PAD))	deb_alpha_level_b-=1;
				if (keyboard_pressed_memory(KEYCODE_8_PAD))	deb_alpha_level_b+=1;

				deb_alpha_level_a &= 0xff;
				deb_alpha_level_b &= 0xff;

				if (keyboard_pressed_memory(KEYCODE_6_PAD)) deb_alp_mode++;
				if (deb_alp_mode>2 ) deb_alp_mode=0;

				sprintf(deb_buf[5],"mode:%d alpha_a:%2x alpha_b:%2x",
									deb_alp_mode,deb_alpha_level_a,deb_alpha_level_b );

				{
					int al_s,al_d;
					int deb_alpha_level_s0,deb_alpha_level_d0;
					int deb_alpha_level_s1,deb_alpha_level_d1;

					int a=(f3_line_ram[(0x6000+1*256*2+cz_line*2)/4]>>sft)&0xffff;
					int b=(a>>8)&0xf;
/*					int c=(a>>4)&0xf;*/
					int d=(a>>0)&0xf;
					a>>=12;

					/* b000 */
					al_s = ( (15-d)*256) / 8;
					al_d = ( (15-b)*256) / 8;
					if(al_s>255) al_s = 255;
					if(al_d>255) al_d = 255;
					deb_alpha_level_s0 = al_s;
					deb_alpha_level_d0 = al_d;

					/* 7000 */
					if(a==11 || a>=b)	al_s = ((15-b)*256) / 8;
					else 				al_s = ((15-a)*256) / 8;
					if(al_s>255) al_s = 255;
					al_d = 255-al_s;
					deb_alpha_level_s1 = al_s;
					deb_alpha_level_d1 = al_d;

					sprintf(deb_buf[6],"alpb src:%4d dst:%4d",deb_alpha_level_s0,deb_alpha_level_d0);
					sprintf(deb_buf[7],"alp7 src:%4d dst:%4d",deb_alpha_level_s1,deb_alpha_level_d1);
				}
			}

			ui_text(bitmap,deb_buf[0],0,Machine->uifontheight*0);
			ui_text(bitmap,deb_buf[1],0,Machine->uifontheight*1);
			ui_text(bitmap,deb_buf[2],0,Machine->uifontheight*2);
			ui_text(bitmap,deb_buf[3],0,Machine->uifontheight*3);
			ui_text(bitmap,deb_buf[4],0,Machine->uifontheight*4);
			ui_text(bitmap,deb_buf[5],0,Machine->uifontheight*5);
			ui_text(bitmap,deb_buf[6],0,Machine->uifontheight*6);
			ui_text(bitmap,deb_buf[7],0,Machine->uifontheight*7);
			ui_text(bitmap,deb_buf[8],0,Machine->uifontheight*8);
			memset(deb_buf, 0x00, sizeof(deb_buf));
		}
	}
#endif	/*DEBUG_F3*/
}
