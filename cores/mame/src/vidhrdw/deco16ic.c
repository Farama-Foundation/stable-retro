/********************************************************************************

    Data East video emulation & information by Bryan McPhail, mish@tendril.co.uk (c) 2000-2005 Bryan McPhail
    Please send me any additions to the table below.


    Game                    Mask Rom    CPU     Sprites         Playfields  IO/Protection   Other
    ==============================================================================================
    Vapor Trail/Kuhga           MAA     59      MXC-06          55, 55
    Crude Buster/Two Crude      MAB     59      52              55, 55
    Dark Seal/Gate Of Doom      MAC     59      52              55, 55
    Edward Randy                MAD     59      52              55, 55          60
    Super Burger Time           MAE     59      52              55
    Mutant Fighter/Death Brade  MAF     59      52              55, 56          66
    Caveman Ninja/Joe & Mac     MAG     59      52              55, 55          104
    Robocop 2                   MAH     59      52              55, 55          75
    Desert Assault/Thunderzone  MAJ     59,59   52,52           55, 55
    China Town                  MAK     59      52              55
    Rogha/Wolf Fang             MAM     59      52,52,71,71     55, 56          104         113
    Captain America             MAN     101     52,71           56, 56          75
    Tumblepop                   MAP     59      52              56
    Dragon Gun                  MAR     101     ?               74, 74          146         113,186,187
    Wizard Fire/Dark Seal 2     MAS     59      52,52,71,71     74, 74          104         113
    Funky Jet                   MAT     59      52              74              146
    Nitro Ball                  MAV     59      52,52,71,71     56, 74          146         113
    Diet GoGo                   MAY     102     52,71           141             104         113
    Pocket Gal DX               MAZ     102     52,71           56              104         153
    Boogie Wings                MBD     102     52,52,71,71     141, 141        104         113,99,200
1   Double Wings                MBE     102     52              141             104
    Fighter's History           MBF     101     52,153          56,74           [Scratched] 200, 153, 170
    Heavy Smash                 MBG     156     52              141                         153*3
    Night Slashers              MBH     156     52,52,52,153,153,153    74, 141 104         99,200
    Locked N Loaded             MBM     101     ?               74,74           146         113,186,187
    Joe & Mac Return            MBN     156     52              141                         223,223
2   Charlie Ninja               MBR     156     52              141                         223,223
    World Cup Volleyball 95     MBX     156     52              141             ?
    Backfire!                   MBZ     156     52,52,153,153   141,141         ?           223
2*  Ganbare Gonta               MCB     156     52              141                         223,223
    Chain Reaction/Magical Drop MCC     156     52              141                         223,223
    Dunk Dream 95               MCE     156     [MLC]           [MLC]
2   Osman/Cannon Dancer         MCF     156     52              141                         223,223
    Avengers In Galactic Storm  MCG     SH2     [MLC]           [MLC]
    Stadium Hero 96             MCM     156     [MLC]           [MLC]           146

    Sotsugyo Shousho                    59      52              74              146?
    Lemmings                    ---     59      52,52,71,71     None            75
    Tattoo Assassins            ---     101     52,52,71,71     141, 141?       ?           99, ?

Note 1: Mitchell game on DECO PCB board number DEC-22V0 (S-NK-3220)
Note 2: Mitchell games on DECO PCB board number MT5601-0
Note *: Ganbare! Gonta!! 2 / Lady Killer Part 2 - Party Time

    Custom chip 59  = 68000 cpu
    Custom chip 101 = Arm6 cpu
    Custom chip 113 = Alpha blending
    Custom chip 99  = 'Ace' chip (Special alpha blending?)
    Custom chip 156 = Encrypted ARM cpu
    Custom chip 102 = Encrypted 68000 cpu

    Custom chip 55 provides two playfields of 4bpp tiles, with optional
    rowscroll and column scroll.  Some games use two of these to give
    4 playfields.  Palette banking, tile banking and priority are outside
    the scope of this chip, and usually differ between games.  Some games
    combine the 4bpp output of each playfield to give an effective 8bpp
    display.

    Custom chip 56 is the same as 55 but with on-chip decryption, so
    encrypted roms can be used.

    Custom chip 74 is the same as 56 but with different decryption tables.

    Custom chip 141 is same as 56, but can output up to 8BPP per layer.

    Custom chip 55/56/74/141 control register layout:

    Word 0:
        Mask 0x0080: Flip screen
        Mask 0x007f: ?  Possibly a bit to set transparent pen 0 on/off
    Word 2:
        Mask 0xffff: Playfield 2 X scroll
    Word 4:
        Mask 0xffff: Playfield 2 Y scroll
    Word 6:
        Mask 0xffff: Playfield 1 X scroll
    Word 8:
        Mask 0xffff: Playfield 1 Y scroll
    Word 0xa:
        Mask 0x8000: Playfield 1 enable
        Mask 0x7800: Playfield 1 rowscroll style
        Mask 0x0700: Playfield 1 colscroll style

        Mask 0x0080: Playfield 2 enable
        Mask 0x0078: Playfield 2 rowscroll style
        Mask 0x0007: Playfield 2 colscroll style
    Word 0xc:
        Mask 0x8000: Playfield 1 is 8*8 tiles else 16*16
        Mask 0x4000: Playfield 1 rowscroll enabled
        Mask 0x2000: Playfield 1 colscroll enabled
        Mask 0x1800: Seemingly unused
        Mask 0x0400: Use alternate palette bank (unimplemented/unused by any game)
        Mask 0x0200: Y flip tiles with top bit of palette code set (palette becomes 3 bits for this tile)
        Mask 0x0100: X flip tiles with top bit of palette code set (palette becomes 3 bits for this tile)

        Mask 0x0080: Playfield 2 is 8*8 tiles else 16*16
        Mask 0x0040: Playfield 2 rowscroll enabled
        Mask 0x0020: Playfield 2 colscroll enabled
        Mask 0x0018: Seemingly unused
        Mask 0x0004: Use alternate palette bank (unimplemented/unused by any game)
        Mask 0x0002: Y flip tiles with top bit of palette code set (palette becomes 3 bits for this tile)
        Mask 0x0001: X flip tiles with top bit of palette code set (palette becomes 3 bits for this tile)

    Word 0xe:
        Mask 0xff00: Playfield 1 gfx rom banking (Usually different hookup per game)
        Mask 0x00ff: Playfield 2 gfx rom banking

Colscroll style:
    0   8 pixel columns across bitmap
    1   16 pixel columns across bitmap
    2   32
    3   64
    4   128
    5   256
    6   512
    7   1024  (Effectively 0 as 1024 is the width of the bitmap)

Rowscroll style:
    0   512 rows down bitmap in 16x16 tile mode, 256 rows in 8x8 mode
    1   256 rows in 16x16, 128 in 8x8
    2   128 / 64
    3   64 / 32
    4   32 / 16
    5   16 / 8
    6   8 / 4
    7   4 / 2
    8   2 / 1
    9-15 Untested on hardware, no known games set this anyway.  Assumed invalid (no rowscroll)

    Column and rowscroll can both be applied at once.  (Not implemented yet)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

data16_t *deco16_pf1_data,*deco16_pf2_data;
data16_t *deco16_pf3_data,*deco16_pf4_data;
data16_t *deco16_pf1_rowscroll,*deco16_pf2_rowscroll;
data16_t *deco16_pf3_rowscroll,*deco16_pf4_rowscroll;

static const data16_t *pf1_rowscroll_ptr, *pf2_rowscroll_ptr;
static const data16_t *pf3_rowscroll_ptr, *pf4_rowscroll_ptr;

data16_t *deco16_pf12_control,*deco16_pf34_control;
data16_t deco16_priority;

data16_t *deco16_raster_display_list;
int deco16_raster_display_position;

static int use_custom_pf1, use_custom_pf2, use_custom_pf3, use_custom_pf4;

static struct tilemap *pf1_tilemap_16x16,*pf2_tilemap_16x16,*pf3_tilemap_16x16,*pf4_tilemap_16x16;
static struct tilemap *pf1_tilemap_8x8,*pf2_tilemap_8x8;

static struct mame_bitmap *sprite_priority_bitmap;

static data8_t *dirty_palette;
static int deco16_pf1_bank,deco16_pf2_bank,deco16_pf3_bank,deco16_pf4_bank;
static int deco16_pf12_16x16_gfx_bank,deco16_pf34_16x16_gfx_bank,deco16_pf12_8x8_gfx_bank;
static int deco16_pf1_colourmask,deco16_pf2_colourmask,deco16_pf3_colourmask,deco16_pf4_colourmask;
int deco16_pf1_colour_bank,deco16_pf2_colour_bank,deco16_pf3_colour_bank,deco16_pf4_colour_bank;
static int deco16_pf1_trans_mask,deco16_pf2_trans_mask,deco16_pf3_trans_mask,deco16_pf4_trans_mask;
static int (*deco16_bank_callback_1)(const int bank);
static int (*deco16_bank_callback_2)(const int bank);
static int (*deco16_bank_callback_3)(const int bank);
static int (*deco16_bank_callback_4)(const int bank);
static void custom_tilemap_draw(struct mame_bitmap *bitmap,struct tilemap *tilemap0_8x8,struct tilemap *tilemap0_16x16,
	struct tilemap *tilemap1_8x8,struct tilemap *tilemap1_16x16, const data16_t *rowscroll_ptr,const data16_t scrollx,
	const data16_t scrolly,const data16_t control0, const data16_t control1,int combine_mask,int combine_shift,int trans_mask,int flags,UINT32 priority);

/******************************************************************************/

/* Later games have double buffered paletteram - the real palette ram is
only updated on a DMA call */

WRITE16_HANDLER( deco16_nonbuffered_palette_w )
{
	int r,g,b;

	COMBINE_DATA(&paletteram16[offset]);
	if (offset&1) offset--;

	b = (paletteram16[offset] >> 0) & 0xff;
	g = (paletteram16[offset+1] >> 8) & 0xff;
	r = (paletteram16[offset+1] >> 0) & 0xff;

	palette_set_color(offset/2,r,g,b);
}

WRITE16_HANDLER( deco16_buffered_palette_w )
{
	COMBINE_DATA(&paletteram16[offset]);
	dirty_palette[offset/2]=1;
}

WRITE16_HANDLER( deco16_palette_dma_w )
{
	const int m=Machine->drv->total_colors;
	int r,g,b,i;

	for (i=0; i<m; i++) {
		if (dirty_palette[i]) {
			dirty_palette[i]=0;

			b = (paletteram16[i*2] >> 0) & 0xff;
			g = (paletteram16[i*2+1] >> 8) & 0xff;
			r = (paletteram16[i*2+1] >> 0) & 0xff;

			palette_set_color(i,r,g,b);
		}
	}
}

/*****************************************************************************************/

/* */
READ16_HANDLER( deco16_71_r )
{
	return 0xffff;
}

WRITE16_HANDLER( deco16_priority_w )
{
	deco16_priority=data;
}

/*****************************************************************************************/

static UINT32 deco16_scan_rows(UINT32 col,UINT32 row,UINT32 num_cols,UINT32 num_rows)
{
	/* logical (col,row) -> memory offset */
	return (col & 0x1f) + ((row & 0x1f) << 5) + ((col & 0x20) << 5) + ((row & 0x20) << 6);
}

static void get_pf4_tile_info(int tile_index)
{
	data16_t tile=deco16_pf4_data[tile_index];
	data8_t colour=(tile>>12)&0xf;
	data8_t flags=0;

	if (tile&0x8000) {
		if ((deco16_pf34_control[6]>>8)&0x01) {
			flags|=TILE_FLIPX;
			colour&=0x7;
		}
		if ((deco16_pf34_control[6]>>8)&0x02) {
			flags|=TILE_FLIPY;
			colour&=0x7;
		}
	}

	SET_TILE_INFO(
			deco16_pf34_16x16_gfx_bank,
			(tile&0xfff)|deco16_pf4_bank,
			(colour&deco16_pf4_colourmask)+deco16_pf4_colour_bank,
			flags)
}

static void get_pf3_tile_info(int tile_index)
{
	data16_t tile=deco16_pf3_data[tile_index];
	data8_t colour=(tile>>12)&0xf;
	data8_t flags=0;

	if (tile&0x8000) {
		if ((deco16_pf34_control[6]>>0)&0x01) {
			flags|=TILE_FLIPX;
			colour&=0x7;
		}
		if ((deco16_pf34_control[6]>>0)&0x02) {
			flags|=TILE_FLIPY;
			colour&=0x7;
		}
	}

	SET_TILE_INFO(
			deco16_pf34_16x16_gfx_bank,
			(tile&0xfff)|deco16_pf3_bank,
			(colour&deco16_pf3_colourmask)+deco16_pf3_colour_bank,
			flags)
}

static void get_pf2_tile_info(int tile_index)
{
	data16_t tile=deco16_pf2_data[tile_index];
	data8_t colour=(tile>>12)&0xf;
	data8_t flags=0;

	if (tile&0x8000) {
		if ((deco16_pf12_control[6]>>8)&0x01) {
			flags|=TILE_FLIPX;
			colour&=0x7;
		}
		if ((deco16_pf12_control[6]>>8)&0x02) {
			flags|=TILE_FLIPY;
			colour&=0x7;
		}
	}

	SET_TILE_INFO(
			deco16_pf12_16x16_gfx_bank,
			(tile&0xfff)|deco16_pf2_bank,
			(colour&deco16_pf2_colourmask)+deco16_pf2_colour_bank,
			flags)
}

static void get_pf1_tile_info(int tile_index)
{
	data16_t tile=deco16_pf1_data[tile_index];
	data8_t colour=(tile>>12)&0xf;
	data8_t flags=0;

	if (tile&0x8000) {
		if ((deco16_pf12_control[6]>>0)&0x01) {
			flags|=TILE_FLIPX;
			colour&=0x7;
		}
		if ((deco16_pf12_control[6]>>0)&0x02) {
			flags|=TILE_FLIPY;
			colour&=0x7;
		}
	}

	SET_TILE_INFO(
			deco16_pf12_16x16_gfx_bank,
			(tile&0xfff)|deco16_pf1_bank,
			(colour&deco16_pf1_colourmask)+deco16_pf1_colour_bank,
			flags)
}

static void get_pf2_tile_info_b(int tile_index)
{
	data16_t tile=deco16_pf2_data[tile_index];
	data8_t colour=(tile>>12)&0xf;
	data8_t flags=0;

	if (tile&0x8000) {
		if ((deco16_pf12_control[6]>>8)&0x01) {
			flags|=TILE_FLIPX;
			colour&=0x7;
		}
		if ((deco16_pf12_control[6]>>8)&0x02) {
			flags|=TILE_FLIPY;
			colour&=0x7;
		}
	}

	SET_TILE_INFO(
			deco16_pf12_8x8_gfx_bank,
			(tile&0xfff)|deco16_pf2_bank,
			(colour&deco16_pf2_colourmask)+deco16_pf2_colour_bank,
			flags)
}

static void get_pf1_tile_info_b(int tile_index)
{
	data16_t tile=deco16_pf1_data[tile_index];
	data8_t colour=(tile>>12)&0xf;
	data8_t flags=0;

	if (tile&0x8000) {
		if ((deco16_pf12_control[6]>>0)&0x01) {
			flags|=TILE_FLIPX;
			colour&=0x7;
		}
		if ((deco16_pf12_control[6]>>0)&0x02) {
			flags|=TILE_FLIPY;
			colour&=0x7;
		}
	}

	SET_TILE_INFO(
			deco16_pf12_8x8_gfx_bank,
			(tile&0xfff)|deco16_pf1_bank,
			(colour&deco16_pf1_colourmask)+deco16_pf1_colour_bank,
			flags)
}

/******************************************************************************/

/* Each game can have banking set up differently depending on how the roms are
connected, and what rom slots are used */

void deco16_set_tilemap_bank_callback(int tilemap, int (*callback)(const int bank))
{
	switch (tilemap) {
	case 0: deco16_bank_callback_1=callback; break;
	case 1: deco16_bank_callback_2=callback; break;
	case 2: deco16_bank_callback_3=callback; break;
	case 3: deco16_bank_callback_4=callback; break;
	}
}

/* Each game can have colours set up differently depending on how the playfield
generator is connected to paletteram */

void deco16_set_tilemap_colour_base(int tilemap, int base)
{
	switch (tilemap) {
	case 0: deco16_pf1_colour_bank=base; break;
	case 1: deco16_pf2_colour_bank=base; break;
	case 2: deco16_pf3_colour_bank=base; break;
	case 3: deco16_pf4_colour_bank=base; break;
	}
}

void deco16_set_tilemap_colour_mask(int tilemap, int mask)
{
	switch (tilemap) {
	case 0: deco16_pf1_colourmask=mask; break;
	case 1: deco16_pf2_colourmask=mask; break;
	case 2: deco16_pf3_colourmask=mask; break;
	case 3: deco16_pf4_colourmask=mask; break;
	}
}

void deco16_set_tilemap_transparency_mask(int tilemap, int mask)
{
	switch (tilemap) {
	case 0: deco16_pf1_trans_mask=mask; break;
	case 1: deco16_pf2_trans_mask=mask; break;
	case 2: deco16_pf3_trans_mask=mask; break;
	case 3: deco16_pf4_trans_mask=mask; break;
	}
}

void deco16_pf12_set_gfxbank(int small, int big)
{
	static int last_small=-1, last_big=-1;

	if (last_small!=small) {
		if (pf1_tilemap_8x8)
			tilemap_mark_all_tiles_dirty(pf1_tilemap_8x8);
		if (pf2_tilemap_8x8)
			tilemap_mark_all_tiles_dirty(pf2_tilemap_8x8);
		last_small=small;
	}
	deco16_pf12_8x8_gfx_bank=small;

	if (last_big!=big) {
		if (pf1_tilemap_16x16)
			tilemap_mark_all_tiles_dirty(pf1_tilemap_16x16);
		if (pf2_tilemap_16x16)
			tilemap_mark_all_tiles_dirty(pf2_tilemap_16x16);
		last_big=big;
	}
	deco16_pf12_16x16_gfx_bank=big;

}

void deco16_pf34_set_gfxbank(int small, int big)
{
	static int last_big=-1;

	if (last_big!=big) {
		if (pf3_tilemap_16x16)
			tilemap_mark_all_tiles_dirty(pf3_tilemap_16x16);
		if (pf4_tilemap_16x16)
			tilemap_mark_all_tiles_dirty(pf4_tilemap_16x16);
		last_big=big;
	}
	deco16_pf34_16x16_gfx_bank=big;
}

struct tilemap *deco16_get_tilemap(int pf, int size)
{
	switch (pf) {
	case 0: if (size) return pf1_tilemap_8x8; return pf1_tilemap_16x16;
	case 1: if (size) return pf2_tilemap_8x8; return pf2_tilemap_16x16;
	case 2: if (size) return 0; return pf3_tilemap_16x16;
	case 3: if (size) return 0; return pf4_tilemap_16x16;
	}
	return 0;
}

/******************************************************************************/

WRITE16_HANDLER( deco16_pf1_data_w )
{
	data16_t oldword=deco16_pf1_data[offset];
	COMBINE_DATA(&deco16_pf1_data[offset]);
	if (oldword!=deco16_pf1_data[offset]) {
		tilemap_mark_tile_dirty(pf1_tilemap_8x8,offset);
		if (offset<0x800)
			tilemap_mark_tile_dirty(pf1_tilemap_16x16,offset);
	}
}

WRITE16_HANDLER( deco16_pf2_data_w )
{
	data16_t oldword=deco16_pf2_data[offset];
	COMBINE_DATA(&deco16_pf2_data[offset]);
	if (oldword!=deco16_pf2_data[offset]) {
		tilemap_mark_tile_dirty(pf2_tilemap_8x8,offset);
		if (offset<0x800)
			tilemap_mark_tile_dirty(pf2_tilemap_16x16,offset);
	}
}

WRITE16_HANDLER( deco16_pf3_data_w )
{
	data16_t oldword=deco16_pf3_data[offset];
	COMBINE_DATA(&deco16_pf3_data[offset]);
	if (oldword!=deco16_pf3_data[offset]) {
		tilemap_mark_tile_dirty(pf3_tilemap_16x16,offset);
	}
}

WRITE16_HANDLER( deco16_pf4_data_w )
{
	data16_t oldword=deco16_pf4_data[offset];
	COMBINE_DATA(&deco16_pf4_data[offset]);
	if (oldword!=deco16_pf4_data[offset])
		tilemap_mark_tile_dirty(pf4_tilemap_16x16,offset);
}

/*****************************************************************************************/

int deco16_1_video_init(void) /* 1 times playfield generator chip */
{
	pf2_tilemap_16x16 =	tilemap_create(get_pf2_tile_info,   deco16_scan_rows, TILEMAP_TRANSPARENT,16,16,64,32);
	pf1_tilemap_16x16 =	tilemap_create(get_pf1_tile_info,   deco16_scan_rows, TILEMAP_TRANSPARENT,16,16,64,32);
	pf2_tilemap_8x8 =	tilemap_create(get_pf2_tile_info_b, tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,32);
	pf1_tilemap_8x8 =	tilemap_create(get_pf1_tile_info_b, tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,32);
	sprite_priority_bitmap = auto_bitmap_alloc_depth( Machine->scrbitmap->width, Machine->scrbitmap->height, -8 );

	dirty_palette = auto_malloc(4096);
	deco16_raster_display_list=auto_malloc(20 * 256);

	if (!dirty_palette || !pf1_tilemap_8x8 || !pf2_tilemap_8x8 || !pf1_tilemap_16x16 || !pf2_tilemap_16x16 || !deco16_raster_display_list || !sprite_priority_bitmap)
		return 1;

	memset(dirty_palette,0,4096);
	tilemap_set_transparent_pen(pf1_tilemap_8x8,0);
	tilemap_set_transparent_pen(pf2_tilemap_8x8,0);
	tilemap_set_transparent_pen(pf1_tilemap_16x16,0);
	tilemap_set_transparent_pen(pf2_tilemap_16x16,0);

	pf3_tilemap_16x16=0;
	pf4_tilemap_16x16=0;

	deco16_bank_callback_1=0;
	deco16_bank_callback_2=0;
	deco16_bank_callback_3=0;
	deco16_bank_callback_4=0;

	deco16_pf1_trans_mask=0xf;
	deco16_pf2_trans_mask=0xf;
	deco16_pf3_trans_mask=0xf;
	deco16_pf4_trans_mask=0xf;

	deco16_pf1_colourmask=deco16_pf2_colourmask=0xf;
	deco16_pf1_bank=deco16_pf2_bank=deco16_pf3_bank=deco16_pf4_bank=0;
	deco16_pf4_colour_bank=deco16_pf2_colour_bank=16;
	deco16_pf3_colour_bank=deco16_pf1_colour_bank=0;

	deco16_pf12_8x8_gfx_bank=0;
	deco16_pf12_16x16_gfx_bank=1;
	deco16_pf34_16x16_gfx_bank=2;

	deco16_raster_display_position=0;

	return 0;
}

int deco16_2_video_init(int split) /* 2 times playfield generator chips */
{
	pf4_tilemap_16x16 =	tilemap_create(get_pf4_tile_info,   deco16_scan_rows, TILEMAP_TRANSPARENT,16,16,64,32);
	pf3_tilemap_16x16 =	tilemap_create(get_pf3_tile_info,   deco16_scan_rows, TILEMAP_TRANSPARENT,16,16,64,32);
	if (split)
		pf2_tilemap_16x16 =	tilemap_create(get_pf2_tile_info,   deco16_scan_rows, TILEMAP_SPLIT,16,16,64,32);
	else
		pf2_tilemap_16x16 =	tilemap_create(get_pf2_tile_info,   deco16_scan_rows, TILEMAP_TRANSPARENT,16,16,64,32);
	pf1_tilemap_16x16 =	tilemap_create(get_pf1_tile_info,   deco16_scan_rows, TILEMAP_TRANSPARENT,16,16,64,32);
	pf2_tilemap_8x8 =	tilemap_create(get_pf2_tile_info_b, tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,32);
	pf1_tilemap_8x8 =	tilemap_create(get_pf1_tile_info_b, tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,32);
	sprite_priority_bitmap = auto_bitmap_alloc_depth( Machine->scrbitmap->width, Machine->scrbitmap->height, -8 );

	dirty_palette = auto_malloc(4096);
	deco16_raster_display_list=auto_malloc(20 * 256);

	if (!dirty_palette || !pf1_tilemap_8x8 || !pf2_tilemap_8x8 || !pf1_tilemap_16x16 || !pf2_tilemap_16x16 || !pf3_tilemap_16x16 || !pf4_tilemap_16x16 || !deco16_raster_display_list || !sprite_priority_bitmap)
		return 1;

	memset(dirty_palette,0,4096);
	tilemap_set_transparent_pen(pf1_tilemap_8x8,0);
	tilemap_set_transparent_pen(pf2_tilemap_8x8,0);
	tilemap_set_transparent_pen(pf1_tilemap_16x16,0);
	tilemap_set_transparent_pen(pf2_tilemap_16x16,0);
	tilemap_set_transparent_pen(pf3_tilemap_16x16,0);
	tilemap_set_transparent_pen(pf4_tilemap_16x16,0);
	if (split) /* Caveman Ninja only */
		tilemap_set_transmask(pf2_tilemap_16x16,0,0x00ff,0xff01);

	deco16_bank_callback_1=0;
	deco16_bank_callback_2=0;
	deco16_bank_callback_3=0;
	deco16_bank_callback_4=0;

	deco16_pf1_trans_mask=0xf;
	deco16_pf2_trans_mask=0xf;
	deco16_pf3_trans_mask=0xf;
	deco16_pf4_trans_mask=0xf;

	deco16_pf1_colourmask=deco16_pf2_colourmask=0xf;
	deco16_pf3_colourmask=deco16_pf4_colourmask=0xf;
	deco16_pf1_bank=deco16_pf2_bank=deco16_pf3_bank=deco16_pf4_bank=0;
	deco16_pf4_colour_bank=deco16_pf2_colour_bank=16;
	deco16_pf3_colour_bank=deco16_pf1_colour_bank=0;

	deco16_pf12_8x8_gfx_bank=0;
	deco16_pf12_16x16_gfx_bank=1;
	deco16_pf34_16x16_gfx_bank=2;

	deco16_raster_display_position=0;

	return 0;
}

int deco_allocate_sprite_bitmap(void)
{
	/* Allow sprite bitmap to be used by Deco32 games as well */
	sprite_priority_bitmap = auto_bitmap_alloc_depth( Machine->scrbitmap->width, Machine->scrbitmap->height, -8 );

	return (sprite_priority_bitmap!=0);
}

/*****************************************************************************************/

static int deco16_pf_update(
	struct tilemap *tilemap_8x8,
	struct tilemap *tilemap_16x16,
	const data16_t *rowscroll_ptr,
	const data16_t scrollx,
	const data16_t scrolly,
	const data16_t control0,
	const data16_t control1)
{
	int rows, cols, offs, use_custom=0;

	/* Toggle between 8x8 and 16x16 modes (and master enable bit) */
	if (control1&0x80) {
		if (!tilemap_8x8) usrintf_showmessage("Deco16: Playfield switched into 8x8 mode but no tilemap defined");

		if (tilemap_8x8) tilemap_set_enable(tilemap_8x8,control0&0x80);
		if (tilemap_16x16) tilemap_set_enable(tilemap_16x16,0);
	} else {
		if (!tilemap_16x16) usrintf_showmessage("Deco16: Playfield switched into 16x16 mode but no tilemap defined");

		if (tilemap_8x8) tilemap_set_enable(tilemap_8x8,0);
		if (tilemap_16x16) tilemap_set_enable(tilemap_16x16,control0&0x80);
	}

	/* Rowscroll enable */
	if (rowscroll_ptr && (control1&0x60)==0x40) {

		/* Several different rowscroll styles */
		switch ((control0>>3)&0xf) {
			case 0: rows=512; break;/* Every line of 512 height bitmap */
			case 1: rows=256; break;
			case 2: rows=128; break;
			case 3: rows=64; break;
			case 4: rows=32; break;
			case 5: rows=16; break;
			case 6: rows=8; break;
			case 7: rows=4; break;
			case 8: rows=2; break;
			default: rows=1; break;
		}

		if (tilemap_16x16) {
			tilemap_set_scroll_cols(tilemap_16x16,1);
			tilemap_set_scroll_rows(tilemap_16x16,rows);
			tilemap_set_scrolly(tilemap_16x16,0,scrolly);

			for (offs = 0;offs < rows;offs++)
				tilemap_set_scrollx( tilemap_16x16, offs, scrollx + rowscroll_ptr[offs] );
		}

		if (tilemap_8x8) {
			tilemap_set_scroll_cols(tilemap_8x8,1);
			tilemap_set_scroll_rows(tilemap_8x8,rows/2);
			tilemap_set_scrolly(tilemap_8x8,0,scrolly);

			for (offs = 0;offs < rows/2;offs++)
				tilemap_set_scrollx( tilemap_8x8, offs, scrollx + rowscroll_ptr[offs] );
		}
	}
	else if (rowscroll_ptr && (control1&0x60)==0x20) { /* Column scroll */

		/* Column scroll ranges from 8 pixel columns to 512 pixel columns */
		int mask=(0x40 >> (control0&7))-1;
		if (mask==-1) mask=0;
		cols=(8<<(control0&7))&0x3ff;
		if (!cols) cols=1024;
		cols=1024 / cols;

		if (tilemap_16x16) {
			tilemap_set_scroll_cols(tilemap_16x16,cols);
			tilemap_set_scroll_rows(tilemap_16x16,1);
			tilemap_set_scrollx(tilemap_16x16,0,scrollx );

			for (offs=0 ; offs < cols;offs++)
				tilemap_set_scrolly( tilemap_16x16, offs, scrolly + rowscroll_ptr[(offs&mask)+0x200] );
		}

		if (tilemap_8x8) {
			tilemap_set_scroll_cols(tilemap_8x8,cols);
			tilemap_set_scroll_rows(tilemap_8x8,1);
			tilemap_set_scrollx(tilemap_8x8,0,scrollx );

			for (offs=0 ; offs < cols;offs++)
				tilemap_set_scrolly( tilemap_8x8,offs, scrolly + rowscroll_ptr[(offs&mask)+0x200] );
		}
	}
	else if (control1&0x60) {
		/* Simultaneous row & column scroll requested - use custom renderer */
		use_custom=1;

		if (tilemap_16x16) {
			tilemap_set_scroll_rows(tilemap_16x16,1);
			tilemap_set_scroll_cols(tilemap_16x16,1);
			tilemap_set_scrollx( tilemap_16x16,0, scrollx );
			tilemap_set_scrolly( tilemap_16x16,0, scrolly );
		}

		if (tilemap_8x8) {
			tilemap_set_scroll_rows(tilemap_8x8,1);
			tilemap_set_scroll_cols(tilemap_8x8,1);
			tilemap_set_scrollx( tilemap_8x8,0, scrollx );
			tilemap_set_scrolly( tilemap_8x8,0, scrolly );

		}

	} else {
		if (tilemap_16x16) {
			tilemap_set_scroll_rows(tilemap_16x16,1);
			tilemap_set_scroll_cols(tilemap_16x16,1);
			tilemap_set_scrollx( tilemap_16x16,0, scrollx );
			tilemap_set_scrolly( tilemap_16x16,0, scrolly );
		}

		if (tilemap_8x8) {
			tilemap_set_scroll_rows(tilemap_8x8,1);
			tilemap_set_scroll_cols(tilemap_8x8,1);
			tilemap_set_scrollx( tilemap_8x8,0, scrollx );
			tilemap_set_scrolly( tilemap_8x8,0, scrolly );
		}
	}

	return use_custom;
}

void deco16_pf12_update(const data16_t *rowscroll_1_ptr, const data16_t *rowscroll_2_ptr)
{
	int bank1, bank2;

	/* Update scrolling and tilemap enable */
	pf1_rowscroll_ptr=rowscroll_1_ptr;
	pf2_rowscroll_ptr=rowscroll_2_ptr;
	use_custom_pf2=deco16_pf_update(pf2_tilemap_8x8,pf2_tilemap_16x16,rowscroll_2_ptr,deco16_pf12_control[3], deco16_pf12_control[4], deco16_pf12_control[5]>>8, deco16_pf12_control[6]>>8);
	use_custom_pf1=deco16_pf_update(pf1_tilemap_8x8,pf1_tilemap_16x16,rowscroll_1_ptr,deco16_pf12_control[1], deco16_pf12_control[2], deco16_pf12_control[5]&0xff, deco16_pf12_control[6]&0xff);

	/* Update banking and global flip state */
	if (deco16_bank_callback_1) {
		bank1=deco16_bank_callback_1(deco16_pf12_control[7]&0xff);
		if (bank1!=deco16_pf1_bank) {
			if (pf1_tilemap_8x8) tilemap_mark_all_tiles_dirty(pf1_tilemap_8x8);
			if (pf1_tilemap_16x16) tilemap_mark_all_tiles_dirty(pf1_tilemap_16x16);
		}

		deco16_pf1_bank=bank1;
	}

	if (deco16_bank_callback_2) {
		bank2=deco16_bank_callback_2(deco16_pf12_control[7]>>8);
		if (bank2!=deco16_pf2_bank) {
			if (pf2_tilemap_8x8) tilemap_mark_all_tiles_dirty(pf2_tilemap_8x8);
			if (pf2_tilemap_16x16) tilemap_mark_all_tiles_dirty(pf2_tilemap_16x16);
		}

		deco16_pf2_bank=bank2;
	}
}

void deco16_pf34_update(const data16_t *rowscroll_1_ptr, const data16_t *rowscroll_2_ptr)
{
	int bank1, bank2;

	/* Update scrolling and tilemap enable */
	pf3_rowscroll_ptr=rowscroll_1_ptr;
	pf4_rowscroll_ptr=rowscroll_2_ptr;
	use_custom_pf4=deco16_pf_update(0,pf4_tilemap_16x16,rowscroll_2_ptr,deco16_pf34_control[3], deco16_pf34_control[4], deco16_pf34_control[5]>>8, deco16_pf34_control[6]>>8);
	use_custom_pf3=deco16_pf_update(0,pf3_tilemap_16x16,rowscroll_1_ptr,deco16_pf34_control[1], deco16_pf34_control[2], deco16_pf34_control[5]&0xff, deco16_pf34_control[6]&0xff);


	/* Update banking and global flip state */
	if (deco16_bank_callback_3) {
		bank1=deco16_bank_callback_3(deco16_pf34_control[7]&0xff);
		if (bank1!=deco16_pf3_bank) {
			/*if (pf3_tilemap_8x8) tilemap_mark_all_tiles_dirty(pf3_tilemap_8x8);*/
			if (pf3_tilemap_16x16) tilemap_mark_all_tiles_dirty(pf3_tilemap_16x16);
		}

		deco16_pf3_bank=bank1;
	}

	if (deco16_bank_callback_4) {
		bank2=deco16_bank_callback_4(deco16_pf34_control[7]>>8);
		if (bank2!=deco16_pf4_bank) {
			/*if (pf4_tilemap_8x8) tilemap_mark_all_tiles_dirty(pf4_tilemap_8x8);*/
			if (pf4_tilemap_16x16) tilemap_mark_all_tiles_dirty(pf4_tilemap_16x16);
		}

		deco16_pf4_bank=bank2;
	}
}

/*****************************************************************************************/

void deco16_print_debug_info(void)
{
	struct mame_bitmap *bitmap = Machine->scrbitmap;
	char buf[64];
	int j;

	if (keyboard_pressed(KEYCODE_O))
		return;

	if (deco16_pf12_control) {
		sprintf(buf,"%04X %04X %04X %04X",deco16_pf12_control[0],deco16_pf12_control[1],deco16_pf12_control[2],deco16_pf12_control[3]);
		for (j = 0;j< 16+3;j++)
			drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,40,0,TRANSPARENCY_NONE,0);
		sprintf(buf,"%04X %04X %04X %04X",deco16_pf12_control[4],deco16_pf12_control[5],deco16_pf12_control[6],deco16_pf12_control[7]);
		for (j = 0;j< 16+3;j++)
			drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,48,0,TRANSPARENCY_NONE,0);
	}
	if (deco16_pf34_control) {
		sprintf(buf,"%04X %04X %04X %04X",deco16_pf34_control[0],deco16_pf34_control[1],deco16_pf34_control[2],deco16_pf34_control[3]);
		for (j = 0;j< 16+3;j++)
			drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,60,0,TRANSPARENCY_NONE,0);
		sprintf(buf,"%04X %04X %04X %04X",deco16_pf34_control[4],deco16_pf34_control[5],deco16_pf34_control[6],deco16_pf34_control[7]);
		for (j = 0;j< 16+3;j++)
			drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,68,0,TRANSPARENCY_NONE,0);
	}
	sprintf(buf,"%04X",deco16_priority);
	for (j = 0;j< 4;j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,80,0,TRANSPARENCY_NONE,0);
}

/*****************************************************************************************/

void deco16_clear_sprite_priority_bitmap(void)
{
	if (sprite_priority_bitmap)
		fillbitmap(sprite_priority_bitmap,0,NULL);
}

/* A special pdrawgfx z-buffered sprite renderer that is needed to properly draw multiple sprite sources with alpha */
void deco16_pdrawgfx(struct mame_bitmap *dest,const struct GfxElement *gfx,
		unsigned int code,unsigned int color,int flipx,int flipy,int sx,int sy,
		const struct rectangle *clip,int transparency,int transparent_color,UINT32 pri_mask,UINT32 sprite_mask,UINT8 write_pri)
{
	int ox,oy,cx,cy;
	int x_index,y_index,x,y;

	const pen_t *pal = &gfx->colortable[gfx->color_granularity * (color % gfx->total_colors)];
	int source_base = (code % gfx->total_elements) * gfx->height;

	/* check bounds */
	ox = sx;
	oy = sy;

	if (sx>319 || sy>247 || sx<-15 || sy<-7)
		return;

	if (sy<0) sy=0;
	if (sx<0) sx=0;
	if (sx>319) cx=319;
	else cx=ox+16;

	cy=(sy-oy);

	if (flipy) y_index=15-cy; else y_index=cy;

	for( y=0; y<16-cy; y++ )
	{
		UINT8 *source = gfx->gfxdata + ((source_base+y_index) * gfx->line_modulo);
		UINT32 *destb = (UINT32 *)dest->line[sy];
		UINT8 *pri = priority_bitmap->line[sy];
		UINT8 *spri = sprite_priority_bitmap->line[sy];

		if (flipx) { source+=15-(sx-ox); x_index=-1; } else { x_index=1; source+=(sx-ox); }

		for (x=sx; x<cx; x++)
		{
			int c = *source;
			if( c != transparent_color )
			{
				if (pri_mask>pri[x] && sprite_mask>spri[x]) {
					if (transparency == TRANSPARENCY_ALPHA)
						destb[x] = alpha_blend32(destb[x], pal[c]);
					else
						destb[x] = pal[c];
					if (write_pri)
						pri[x] |= pri_mask;
				}
				spri[x]|=sprite_mask;
			}
			source+=x_index;
		}

		sy++;
		if (sy>247)
			return;
		if (flipy) y_index--; else y_index++;
	}
}

/*****************************************************************************************/

void deco16_tilemap_1_draw(struct mame_bitmap *bitmap, const struct rectangle *cliprect, int flags, UINT32 priority)
{
	if (pf1_tilemap_8x8) tilemap_draw(bitmap,cliprect,pf1_tilemap_8x8,flags,priority);
	if (pf1_tilemap_16x16) tilemap_draw(bitmap,cliprect,pf1_tilemap_16x16,flags,priority);
}

void deco16_tilemap_2_draw(struct mame_bitmap *bitmap, const struct rectangle *cliprect, int flags, UINT32 priority)
{
	if (use_custom_pf2)
	{
		custom_tilemap_draw(bitmap,pf2_tilemap_8x8,pf2_tilemap_16x16,pf2_tilemap_8x8,pf2_tilemap_16x16,pf2_rowscroll_ptr,deco16_pf12_control[3], deco16_pf12_control[4], deco16_pf12_control[5]>>8, deco16_pf12_control[6]>>8, 0, 0, deco16_pf2_trans_mask, flags, priority);
	}
	else
	{
		if (pf2_tilemap_8x8) tilemap_draw(bitmap,cliprect,pf2_tilemap_8x8,flags,priority);
		if (pf2_tilemap_16x16) tilemap_draw(bitmap,cliprect,pf2_tilemap_16x16,flags,priority);
	}
}

void deco16_tilemap_3_draw(struct mame_bitmap *bitmap, const struct rectangle *cliprect, int flags, UINT32 priority)
{
	if (use_custom_pf3) custom_tilemap_draw(bitmap,0,pf3_tilemap_16x16,0,0,pf3_rowscroll_ptr,deco16_pf34_control[1], deco16_pf34_control[2], deco16_pf34_control[5]&0xff, deco16_pf34_control[6]&0xff, 0, 0, deco16_pf3_trans_mask, flags, priority);
	else if (pf3_tilemap_16x16) tilemap_draw(bitmap,cliprect,pf3_tilemap_16x16,flags,priority);
}

void deco16_tilemap_4_draw(struct mame_bitmap *bitmap, const struct rectangle *cliprect, int flags, UINT32 priority)
{
	if (use_custom_pf4) custom_tilemap_draw(bitmap,0,pf4_tilemap_16x16,0,0,pf4_rowscroll_ptr,deco16_pf34_control[3], deco16_pf34_control[4], deco16_pf34_control[5]>>8, deco16_pf34_control[6]>>8, 0, 0, deco16_pf4_trans_mask, flags, priority);
	else if (pf4_tilemap_16x16) tilemap_draw(bitmap,cliprect,pf4_tilemap_16x16,flags,priority);
}

/*****************************************************************************************/

/* Combines the output of two 4BPP tilemaps into an 8BPP tilemap*/
void deco16_tilemap_34_combine_draw(struct mame_bitmap *bitmap, const struct rectangle *cliprect, int flags, UINT32 priority)
{
	custom_tilemap_draw(bitmap,0,pf3_tilemap_16x16,0,pf4_tilemap_16x16,pf3_rowscroll_ptr,deco16_pf34_control[1], deco16_pf34_control[2], deco16_pf34_control[5]&0xff, deco16_pf34_control[6]&0xff, 0xf, 4, 0xff, flags, priority);
}

/*****************************************************************************************/

/*
    Consider this the 'reference rasterizer' for the 56/74/141 tilemap chips - it implements
    simultaneous row & column scroll which the Mame tilemap core cannot do.  It also
    implements combining the 4BPP output of two tilemaps into 8BPP output.  This function
    is automatically called when the tilemap is in a state the cannot be properly rendered
    by the Mame core.
*/

static void custom_tilemap_draw(
	struct mame_bitmap *bitmap,
	struct tilemap *tilemap0_8x8,
	struct tilemap *tilemap0_16x16,
	struct tilemap *tilemap1_8x8,
	struct tilemap *tilemap1_16x16,
	const data16_t *rowscroll_ptr,
	const data16_t scrollx,
	const data16_t scrolly,
	const data16_t control0,
	const data16_t control1,
	int combine_mask,
	int combine_shift,
	int trans_mask,
	int flags,
	UINT32 priority)
{
	const struct mame_bitmap *src_bitmap0 = (control1&0x80) ? tilemap_get_pixmap(tilemap0_8x8) : tilemap_get_pixmap(tilemap0_16x16);
	const struct mame_bitmap *src_bitmap1 = (control1&0x80) ? tilemap_get_pixmap(tilemap1_8x8) : tilemap_get_pixmap(tilemap1_16x16);

	int width_mask, height_mask, x, y, p;
	int column_offset, src_x=0, src_y=0;
	int	row_type=1 << ((control0>>3)&0xf);
	int col_type=8 << (control0&7);

	if (!src_bitmap0)
		return;

	/* Playfield disable*/
	if (!(control0&0x80))
		return;

	width_mask=src_bitmap0->width - 1;
	height_mask=src_bitmap0->height - 1;
	src_y=scrolly + 8;

	for (y=8; y<248; y++) {
		if (rowscroll_ptr && (control1&0x40))
			src_x=scrollx + rowscroll_ptr[src_y / row_type];
		else
			src_x=scrollx;

		for (x=0; x<320; x++) {
			if (rowscroll_ptr && (control1&0x20))
				column_offset=rowscroll_ptr[0x200 + ((src_x&0x1ff) / col_type)];
			else
				column_offset=0;

			p=(((UINT16*)src_bitmap0->line[(src_y + column_offset)&height_mask])[src_x]);
			if (src_bitmap1)
				p|=(((((UINT16*)src_bitmap1->line[(src_y + column_offset)&height_mask])[src_x]))&combine_mask)<<combine_shift;

			src_x=(src_x+1)&width_mask;
			if ((flags&TILEMAP_IGNORE_TRANSPARENCY) || (p&trans_mask))
			{
				plot_pixel(bitmap, x, y, Machine->pens[p]);
				if (priority_bitmap)
				{
					UINT8 *pri = priority_bitmap->line[y];
					pri[x]|=priority;
				}
			}
		}
		src_y=(src_y+1)&height_mask;
	}
}
