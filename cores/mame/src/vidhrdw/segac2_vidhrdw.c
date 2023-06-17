/***********************************************************************************************

	Sega System C/C2 Driver
	-----------------------
	Version 0.54 - 02 Feb 2003
	(for changes see drivers\segac2.c)

***********************************************************************************************/

#include "driver.h"
#include "state.h"
#include "segac2.h"

/* in vidhrdw/segasyse.c */
int start_megatech_video_normal(void);
void update_megatech_video_normal(struct mame_bitmap *bitmap, const struct rectangle *cliprect );
void update_megaplay_video_normal(struct mame_bitmap *bitmap, const struct rectangle *cliprect );

/******************************************************************************
	Macros
******************************************************************************/

#define BITMAP_WIDTH		320
#define BITMAP_HEIGHT		240

#define VRAM_SIZE			0x10000
#define VRAM_MASK			(VRAM_SIZE - 1)
#define VSRAM_SIZE			0x80
#define VSRAM_MASK			(VSRAM_SIZE - 1)

#define VDP_VRAM_BYTE(x)	(vdp_vram[(x) & VRAM_MASK])
#define VDP_VSRAM_BYTE(x)	(vdp_vsram[(x) & VSRAM_MASK])
#define VDP_VRAM_WORD(x)	((VDP_VRAM_BYTE(x) << 8) | VDP_VRAM_BYTE((x) + 1))
#define VDP_VSRAM_WORD(x)	((VDP_VSRAM_BYTE(x) << 8) | VDP_VSRAM_BYTE((x) + 1))

#ifdef MSB_FIRST
#define EXTRACT_PIXEL(x,i)	(((x) >> (((i) ^ 7) * 4)) & 0x0f)
#else
#define EXTRACT_PIXEL(x,i)	(((x) >> (((i) ^ 1) * 4)) & 0x0f)
#endif



/******************************************************************************
	Function Prototypes
******************************************************************************/

static int  vdp_data_r(void);
static void vdp_data_w(int data);
static int  vdp_control_r(void);
static void vdp_control_w(int data);
static void vdp_register_w(int data);
static void vdp_control_dma(int data);
static void vdp_dma_68k(void);
static void vdp_dma_fill(int);
static void vdp_dma_copy(void);

static void drawline(UINT16 *bitmap, int line);
static void get_scroll_tiles(int line, int scrollnum, UINT32 scrollbase, UINT32 *tiles, int *offset);
static void get_window_tiles(int line, UINT32 scrollbase, UINT32 *tiles);
static void drawline_tiles(UINT32 *tiles, UINT16 *bmap, int pri, int offset, int lclip, int rclip);
static void drawline_sprite(int line, UINT16 *bmap, int priority, UINT8 *spritebase);



/******************************************************************************
	Global variables
******************************************************************************/

/* EXTERNALLY ACCESSIBLE */
       int			segac2_bg_palbase;			/* base of background palette */
       int			segac2_sp_palbase;			/* base of sprite palette */
       int			segac2_palbank;				/* global palette bank */
       UINT8		segac2_vdp_regs[32];		/* VDP registers */
	   UINT16		scanbase;

/* LOCAL */
static UINT8 *		vdp_vram;					/* VDP video RAM */
static UINT8 *		vdp_vsram;					/* VDP vertical scroll RAM */
static UINT8		display_enable;				/* is the display enabled? */

/* updates */
static UINT8		internal_vblank;			/* state of the VBLANK line */
static UINT16 *		transparent_lookup;			/* fast transparent mapping table */

/* vram bases */
static UINT32		vdp_scrollabase;			/* base address of scroll A tiles */
static UINT32		vdp_scrollbbase;			/* base address of scroll B tiles */
static UINT32		vdp_windowbase;				/* base address of window tiles */
static UINT32		vdp_spritebase;				/* base address of sprite data */
static UINT32		vdp_hscrollbase;			/* base address of H scroll values */

/* other vdp variables */
static int			vdp_hscrollmask;			/* mask for H scrolling */
static UINT32		vdp_hscrollsize;			/* size of active H scroll values */
static UINT8		vdp_vscrollmode;			/* current V scrolling mode */

static UINT8		vdp_cmdpart;				/* partial command flag */
static UINT8		vdp_code;					/* command code value */
static UINT32		vdp_address;				/* current I/O address */
static UINT8		vdp_dmafill;				/* DMA filling flag */
static UINT8		scrollheight;				/* height of the scroll area in tiles */
static UINT8		scrollwidth;				/* width of the scroll area in tiles */
static UINT8		bgcol;						/* current background color */
static UINT8		window_down;				/* window Y direction */
static UINT32		window_vpos;				/* window Y position */
static UINT8		window_right;				/* window X direction */
static UINT32		window_hpos;				/* window X position */


#define GEN_TILEMAP_WIP			0


#if GEN_TILEMAP_WIP
/* tilemaps are annoying, they can change size, use ram based tiles, base address changes etc. */
static struct tilemap *scrolla_tilemap, *scrollb_tilemap;

static void get_scrolla_tile_info(int tile_index)
{
/*
Scroll Name Table
16-bits are used to Define a Tile in the Scroll Plane
|  PRI   |  CP1   |  CP0   |  VF    |  HF    |  PT10  |  PT9   |  PT8   |
|  PT7   |  PT6   |  PT5   |  PT4   |  PT3   |  PT2   |  PT1   |  PT0   |
PRI = Priority, CP = Colour Palette, VF = VFlip, HF = HFlip, PT0-9 = Tile # in VRAM
*/

	int tileno = vdp_vram[(vdp_scrollabase+tile_index*2+1)&0xffff];
	int attr   = vdp_vram[(vdp_scrollabase+tile_index*2)&0xffff];
	int colour = ((attr & 0x60) >> 5)+ segac2_bg_palbase + segac2_palbank;
/*	int priority = (attr & 0x80);*/
	int flipyx = (attr & 0x18) >> 3;

	tileno = tileno + ((attr & 7) << 8) ;

	SET_TILE_INFO(
			0,
			tileno,
			colour,
			TILE_FLIPYX(flipyx))
}

static void get_scrollb_tile_info(int tile_index)
{
/*
Scroll Name Table
16-bits are used to Define a Tile in the Scroll Plane
|  PRI   |  CP1   |  CP0   |  VF    |  HF    |  PT10  |  PT9   |  PT8   |
|  PT7   |  PT6   |  PT5   |  PT4   |  PT3   |  PT2   |  PT1   |  PT0   |
PRI = Priority, CP = Colour Palette, VF = VFlip, HF = HFlip, PT0-9 = Tile # in VRAM
*/

	int tileno = vdp_vram[(vdp_scrollbbase+tile_index*2+1)&0xffff];
	int attr   = vdp_vram[(vdp_scrollbbase+tile_index*2)&0xffff];
	int colour = ((attr & 0x60) >> 5) + segac2_bg_palbase + segac2_palbank;
/*	int priority = (attr & 0x80);*/
	int flipyx = (attr & 0x18) >> 3;

	tileno = tileno + ((attr & 7) << 8);


	SET_TILE_INFO(
			0,
			tileno,
			colour,
			TILE_FLIPYX(flipyx))
}
#endif

/******************************************************************************
	Video Start / Stop Functions
*******************************************************************************

	Here we allocate memory used by the VDP and various other video related parts
	of the System C/C2 hardware such as the Palette RAM. Here is what is needed

	64kb of VRAM (multi-purpose, storing tiles, tilemaps, hscroll data,
					spritelist etc.)

	80bytes of VSRAM (used exclusively for storing Vertical Scroll values)

******************************************************************************/

#if GEN_TILEMAP_WIP
static struct GfxLayout genvdp_charlayout =
{
	8, 8,	/* 8x8 pixels */
	2048,	/* 2048 chars */
	4,		/* 3 bits per pixel */
	{ 0,1,2,3 },
	{ 0, 4, 8, 12, 16, 20, 24, 28 },
	{ 0*32,1*32,2*32,3*32,4*32,5*32,6*32,7*32 },
	8*32
};
#endif

VIDEO_START( segac2 )
{
	static const UINT8 vdp_init[24] =
	{
		0x04, 0x44, 0x30, 0x3C, 0x07, 0x6C, 0x00, 0x00,
		0x00, 0x00, 0xFF, 0x00, 0x01, 0x37, 0x00, 0x02,
		0x01, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x80,
	};
	int i;

	/* allocate memory for the VDP, the lookup table, and the buffer bitmap */
	vdp_vram			= auto_malloc(VRAM_SIZE);
	vdp_vsram			= auto_malloc(VSRAM_SIZE);
	transparent_lookup	= auto_malloc(0x1000 * sizeof(UINT16));

	/* check for errors */
	if (!vdp_vram || !vdp_vsram || !transparent_lookup)
		return 1;

	/* clear the VDP memory, prevents corrupt tile in Puyo Puyo 2 */
	memset(vdp_vram, 0, VRAM_SIZE);
	memset(vdp_vsram, 0, VSRAM_SIZE);

	/* init transparency table */
	for (i = 0; i < 0x1000; i++)
	{
		int orig_color = i & 0x7ff;
		int half_bright = i & 0x800;

		if (orig_color & 0x100)
			transparent_lookup[i] = orig_color;
		else if (half_bright)
			transparent_lookup[i] = orig_color | 0x800;
		else
			transparent_lookup[i] = orig_color | 0x1000;
	}

	/* reset the palettes */
	memset(paletteram16, 0, 0x800 * sizeof(data16_t));
	segac2_bg_palbase = 0x000;
	segac2_sp_palbase = 0x100;
	segac2_palbank    = 0x000;
	
	scanbase = 0;

	/* reset VDP */
	internal_vblank = 1;
    for (i = 0; i < 24; i++)
        vdp_register_w(0x8000 | (i << 8) | vdp_init[i]);
	vdp_cmdpart = 0;
	vdp_code    = 0;
	vdp_address = 0;

	/* Save State Stuff we could probably do with an init values from vdp registers function or something (todo) */

	state_save_register_UINT8 ("C2_VDP", 0, "VDP Registers", segac2_vdp_regs, 32);
	state_save_register_UINT8 ("C2_VDP", 0, "VDP VRam", vdp_vram, 0x10000);
	state_save_register_UINT8 ("C2_VDP", 0, "VDP VSRam", vdp_vsram, 0x80);
	state_save_register_int("C2_Video", 0, "Palette Bank", &segac2_palbank);
	state_save_register_int("C2_Video", 0, "Background Pal Base",  &segac2_bg_palbase);
	state_save_register_int("C2_Video", 0, "Sprite Pal Base",  &segac2_sp_palbase);
	state_save_register_UINT8("C2_Video", 0, "Display Enabled",  &display_enable, 1);
	state_save_register_UINT32("C2_Video", 0, "Scroll A Base in VRAM",  &vdp_scrollabase, 1);
	state_save_register_UINT32("C2_Video", 0, "Scroll B Base in VRAM",  &vdp_scrollbbase, 1);
	state_save_register_UINT32("C2_Video", 0, "Window Base in VRAM",  &vdp_windowbase, 1);
	state_save_register_UINT32("C2_Video", 0, "Sprite Table Base in VRAM",  &vdp_spritebase, 1);
	state_save_register_UINT32("C2_Video", 0, "HScroll Data Base in VRAM",  &vdp_hscrollbase, 1);
	state_save_register_int("C2_Video", 0, "vdp_hscrollmask",  &vdp_hscrollmask);
	state_save_register_UINT32("C2_Video", 0, "vdp_hscrollsize",  &vdp_hscrollsize, 1);
	state_save_register_UINT8("C2_Video", 0, "vdp_vscrollmode",  &vdp_vscrollmode, 1);
	state_save_register_UINT8("C2_VDP", 0, "VDP Command Part",  &vdp_cmdpart, 1);
	state_save_register_UINT8("C2_VDP", 0, "VDP Current Code",  &vdp_code, 1);
	state_save_register_UINT32("C2_VDP", 0, "VDP Address",  &vdp_address, 1);
	state_save_register_UINT8("C2_VDP", 0, "VDP DMA Mode",  &vdp_dmafill, 1);
	state_save_register_UINT8("C2_Video", 0, "scrollheight",  &scrollheight, 1);
	state_save_register_UINT8("C2_Video", 0, "scrollwidth",  &scrollwidth, 1);
	state_save_register_UINT8("C2_Video", 0, "Background Colour",  &bgcol, 1);
	state_save_register_UINT8("C2_Video", 0, "Window Horz",  &window_down, 1);
	state_save_register_UINT32("C2_Video", 0, "Window Vert",  &window_vpos, 1);


#if GEN_TILEMAP_WIP
	{

		int gfx_index=0;

		 	/* find first empty slot to decode gfx */
		for (gfx_index = 0; gfx_index < MAX_GFX_ELEMENTS; gfx_index++)
			if (Machine->gfx[gfx_index] == 0)
				break;
		if (gfx_index == MAX_GFX_ELEMENTS)
			return 1;

			/* create the char set (gfx will then be updated dynamically from RAM) */
			Machine->gfx[gfx_index] = decodegfx((UINT8 *)vdp_vram,&genvdp_charlayout);
			if (!Machine->gfx[gfx_index])
				return 1;

			/* set the color information */
			Machine->gfx[gfx_index]->colortable = Machine->remapped_colortable;
			Machine->gfx[gfx_index]->total_colors = 64;

		scrolla_tilemap = tilemap_create(get_scrolla_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT, 8, 8,64,64);
		scrollb_tilemap = tilemap_create(get_scrollb_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT, 8, 8,64,64);
	}
#endif

	return 0;

}

VIDEO_START( puckpkmn )
{
	paletteram16 = auto_malloc(0x800 * sizeof(data16_t));

	if (video_start_segac2())
		return 1;

	segac2_sp_palbase = 0x000;	/* same palettes for sprites and bg*/
	display_enable = 1;

	scanbase = 0;

	return 0;
}



VIDEO_START( megatech )
{
	paletteram16 = auto_malloc(0x800 * sizeof(data16_t));

	if (video_start_segac2())
		return 1;

	segac2_sp_palbase = 0x000;	/* same palettes for sprites and bg*/
	display_enable = 1;


	if (start_megatech_video_normal())
		return 1;
	scanbase = 192;

	return 0;
}

VIDEO_START( megaplay )
{
	paletteram16 = auto_malloc(0x800 * sizeof(data16_t));

	if (video_start_segac2())
		return 1;

	segac2_sp_palbase = 0x000;	/* same palettes for sprites and bg*/
	display_enable = 1;

	scanbase = 0;

	if (start_megatech_video_normal())
		return 1;

	return 0;
}

/******************************************************************************
	VBLANK routines
*******************************************************************************

	These callbacks are used to track the state of VBLANK. At the end of
	VBLANK, all the palette information is reset and updates to the cache
	bitmap are enabled.

******************************************************************************/

/* timer callback for the end of VBLANK */
static void vblank_end(int param)
{
	/* reset VBLANK flag */
	internal_vblank = 0;
}


/* end-of-frame callback to mark the start of VBLANK */
VIDEO_EOF( segac2 )
{
	/* set VBLANK flag */
	internal_vblank = 1;

	/* set a timer for VBLANK off */
	timer_set(cpu_getscanlinetime(0), 0, vblank_end);

#if GEN_TILEMAP_WIP
	{
		int tile;

		for (tile = 0;tile < 2048;tile++)
			decodechar(Machine->gfx[0],tile,(UINT8 *)vdp_vram,&genvdp_charlayout);
	}
#endif

}



/******************************************************************************
	Screen Refresh Functions
*******************************************************************************

	These are responsible for the core drawing. The update_display function
	can be called under several circumstances to cache all the currently
	displayed lines before a significant palette or scrolling change is
	set to occur. The actual refresh routine marks the accumulated palette
	entries and then converts the raw pens in the cache bitmap to their
	final remapped values.

******************************************************************************/

/* set the display enable bit */
void segac2_enable_display(int enable)
{
	if (!internal_vblank)
		force_partial_update((cpu_getscanline()) + scanbase);
	display_enable = enable;
}


/* core refresh: computes the final screen */
VIDEO_UPDATE( segac2 )
{
	int old_bg = segac2_bg_palbase, old_sp = segac2_sp_palbase;
	int y;

#ifdef MAME_DEBUG
if (keyboard_pressed(KEYCODE_Z)) segac2_bg_palbase ^= 0x40;
if (keyboard_pressed(KEYCODE_X)) segac2_bg_palbase ^= 0x80;
if (keyboard_pressed(KEYCODE_C)) segac2_bg_palbase ^= 0x100;

if (keyboard_pressed(KEYCODE_A)) segac2_sp_palbase ^= 0x40;
if (keyboard_pressed(KEYCODE_S)) segac2_sp_palbase ^= 0x80;
if (keyboard_pressed(KEYCODE_D)) segac2_sp_palbase ^= 0x100;
#endif

#if GEN_TILEMAP_WIP
	{
		tilemap_mark_all_tiles_dirty (scrolla_tilemap);
		tilemap_mark_all_tiles_dirty (scrollb_tilemap);
		/* just so they get updated .. */
		tilemap_draw(bitmap,cliprect,scrolla_tilemap,0,0);
		tilemap_draw(bitmap,cliprect,scrollb_tilemap,0,0);
	}
#endif



	/* generate the final screen */
	for (y = cliprect->min_y; y <= cliprect->max_y; y++)
		drawline((UINT16 *)bitmap->line[y], y);

	segac2_bg_palbase = old_bg;
	segac2_sp_palbase = old_sp;
}

/* megatech, same but drawing the sms display too */

/* core refresh: computes the final screen */
VIDEO_UPDATE( megatech )
{
	int old_bg = segac2_bg_palbase, old_sp = segac2_sp_palbase;
	int y;

#if 0
if (keyboard_pressed(KEYCODE_Z)) segac2_bg_palbase ^= 0x40;
if (keyboard_pressed(KEYCODE_X)) segac2_bg_palbase ^= 0x80;
if (keyboard_pressed(KEYCODE_C)) segac2_bg_palbase ^= 0x100;

if (keyboard_pressed(KEYCODE_A)) segac2_sp_palbase ^= 0x40;
if (keyboard_pressed(KEYCODE_S)) segac2_sp_palbase ^= 0x80;
if (keyboard_pressed(KEYCODE_D)) segac2_sp_palbase ^= 0x100;
#endif

#if GEN_TILEMAP_WIP
	{
		tilemap_mark_all_tiles_dirty (scrolla_tilemap);
		tilemap_mark_all_tiles_dirty (scrollb_tilemap);
		/* just so they get updated .. */
		tilemap_draw(bitmap,cliprect,scrolla_tilemap,0,0);
		tilemap_draw(bitmap,cliprect,scrollb_tilemap,0,0);
	}
#endif



	/* generate the final screen */
	for (y = cliprect->min_y+192; y <= cliprect->max_y; y++)
		drawline((UINT16 *)bitmap->line[y], y-192);

	segac2_bg_palbase = old_bg;
	segac2_sp_palbase = old_sp;

	/* sms display should be on second monitor, for now we control it with a fake dipswitch while
	   the driver is in development */
	/*if (readinputport(5)&0x01)*/	
		update_megatech_video_normal(bitmap, cliprect);

}

/* megaplay, draws either Genesis or SMS (single screen display) */

/* core refresh: computes the final screen */
VIDEO_UPDATE( megaplay )
{
	int old_bg = segac2_bg_palbase, old_sp = segac2_sp_palbase;
	int y;

#if 0
if (keyboard_pressed(KEYCODE_Z)) segac2_bg_palbase ^= 0x40;
if (keyboard_pressed(KEYCODE_X)) segac2_bg_palbase ^= 0x80;
if (keyboard_pressed(KEYCODE_C)) segac2_bg_palbase ^= 0x100;

if (keyboard_pressed(KEYCODE_A)) segac2_sp_palbase ^= 0x40;
if (keyboard_pressed(KEYCODE_S)) segac2_sp_palbase ^= 0x80;
if (keyboard_pressed(KEYCODE_D)) segac2_sp_palbase ^= 0x100;
#endif

#if GEN_TILEMAP_WIP
	{
		tilemap_mark_all_tiles_dirty (scrolla_tilemap);
		tilemap_mark_all_tiles_dirty (scrollb_tilemap);
		/* just so they get updated .. */
		tilemap_draw(bitmap,cliprect,scrolla_tilemap,0,0);
		tilemap_draw(bitmap,cliprect,scrollb_tilemap,0,0);
	}
#endif



	/* generate the final screen - control which screen is 
	   shown by a keystroke for now */
	for (y = cliprect->min_y; y <= cliprect->max_y; y++)
		drawline((UINT16 *)bitmap->line[y], y);

	update_megaplay_video_normal(bitmap, cliprect);

	segac2_bg_palbase = old_bg;
	segac2_sp_palbase = old_sp;

}

/******************************************************************************
	VDP Read & Write Handlers
*******************************************************************************

	The VDP is accessed via 8 & 16 bit reads / writes at the addresses 0xC00000 -
	0xC0001F.  Different locations have different functions in the following
	layout.  (based on Information in Charles MacDonalds Document)

	0xC00000 -   DATA port (8 or 16 bit, Read or Write)
	0xC00002 -   mirror of above
	0xC00004 -   CTRL port (8 or 16 bit, Read or Write)
	0xC00006 -   mirror of above
	0xC00008 -   HV counter (8 or 16 bit Read Only)
	0xC0000A -   mirror of above
	0xC0000C -   mirror of above
	0xC0000E -   mirror of above
	0xC00010 -   SN76489 (8 bit Write Only)
	0xC00012 -   mirror of above
	0xC00014 -   mirror of above
	0xC00016 -   mirror of above

	The SN76489 PSG Writes are intercepted before we get here and handled
	directly.

******************************************************************************/

READ16_HANDLER( segac2_vdp_r )
{
	switch (offset)
	{
		case 0x00:	/* Read Data */
		case 0x01:
			return vdp_data_r();

		case 0x02:	/* Status Register */
		case 0x03:
			return vdp_control_r();

		case 0x04:	/* HV counter */
		case 0x05:
		case 0x06:
		case 0x07:
		{
			int xpos = cpu_gethorzbeampos();
			int ypos = cpu_getscanline();

			/* adjust for the weird counting rules */
			if (xpos > 0xe9) xpos -= (342 - 0x100);
			if (ypos > 0xea) ypos -= (262 - 0x100);

			/* kludge for ichidant: it sets the H counter to 160, then */
			/* expects that a read from here will return 159 */
			if (ypos > 0) ypos -= 2;
			return (ypos << 8) | xpos;
		}
	}
	return 0;
}


WRITE16_HANDLER( segac2_vdp_w )
{
	switch (offset)
	{
		case 0x00:	/* Write data */
		case 0x01:
			if (mem_mask)
			{
				data &= ~mem_mask;
				 if (ACCESSING_MSB)
				 	data |= data >> 8;
				 else
				 	data |= data << 8;
			}
			vdp_data_w(data);
			break;

		case 0x02:	/* Control Write */
		case 0x03:
			if (mem_mask)
			{
				data &= ~mem_mask;
				 if (ACCESSING_MSB)
				 	data |= data >> 8;
				 else
				 	data |= data << 8;
			}
			vdp_control_w(data);
			break;
	}
}



/******************************************************************************
	VDP DATA Reads / Writes (Accesses to 0xC00000 - 0xC00003)
*******************************************************************************

	The functions here are called by the read / write handlers for the VDP.
	They deal with Data Reads and Writes to and from the VDP Chip.

	Read / write mode is set by Writes to VDP Control, Can't Read when in
	write mode or vice versa.

******************************************************************************/

/* Games needing Read to Work .. bloxeed (attract) .. puyo puyo .. probably more */
static int vdp_data_r(void)
{
	int read = 0;

	/* kill 2nd write pending flag */
	vdp_cmdpart = 0;

	/* which memory is based on the code */
	switch (vdp_code & 0x0f)
	{
		case 0x00:		/* VRAM read */
			read = VDP_VRAM_WORD(vdp_address & ~1);
			break;

		case 0x04:		/* VSRAM read */
			read = VDP_VSRAM_WORD(vdp_address & ~1);
			break;

		default:		/* Illegal read attempt */
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%06x: VDP illegal read type %02x\n", activecpu_get_previouspc(), vdp_code);
			read = 0x00;
			break;
	}

	/* advance the address */
	vdp_address += segac2_vdp_regs[15];
	return read;
}


static void vdp_data_w(int data)
{
	/* kill 2nd write pending flag */
	vdp_cmdpart = 0;

	/* handle the fill case */
	if (vdp_dmafill)
	{
		vdp_dma_fill(data);
		vdp_dmafill = 0;
		return;
	}

	/* which memory is based on the code */
	switch (vdp_code & 0x0f)
	{
		case 0x01:		/* VRAM write */

			/* if the hscroll RAM is changing during screen refresh, force an update */
			if (!internal_vblank &&
				vdp_address >= vdp_hscrollbase &&
				vdp_address < vdp_hscrollbase + vdp_hscrollsize)
				force_partial_update((cpu_getscanline()) + scanbase);

			/* write to VRAM */
			if (vdp_address & 1)
				data = ((data & 0xff) << 8) | ((data >> 8) & 0xff);
			VDP_VRAM_BYTE(vdp_address & ~1) = data >> 8;
			VDP_VRAM_BYTE(vdp_address |  1) = data;
			break;


		case 0x03:		/* Palette write - puckpkmn */
			paletteram16_xxxxBBBBGGGGRRRR_word_w(vdp_address/2, data, 0);
			break;


		case 0x05:		/* VSRAM write */

			/* if the vscroll RAM is changing during screen refresh, force an update */
			if (!internal_vblank)
				force_partial_update((cpu_getscanline()) + scanbase);

			/* write to VSRAM */
			if (vdp_address & 1)
				data = ((data & 0xff) << 8) | ((data >> 8) & 0xff);
			VDP_VSRAM_BYTE(vdp_address & ~1) = data >> 8;
			VDP_VSRAM_BYTE(vdp_address |  1) = data;
			break;

		default:		/* Illegal write attempt */
			log_cb(RETRO_LOG_DEBUG, LOGPRE "PC:%06x: VDP illegal write type %02x data %04x\n", activecpu_get_previouspc(), vdp_code, data);
			break;
	}

	/* advance the address */
	vdp_address += segac2_vdp_regs[15];
}



/******************************************************************************
	VDP CTRL Reads / Writes (Accesses to 0xC00004 - 0xC00007)
*******************************************************************************

	A Read from the Control Port will return the Status Register Value.
	16-bits are used to report the VDP status
	|  0     |  0     |  1     |  1     |  0     |  1     |  FIFE  |  FIFF  |
	|  VIP   |  SOF   |  SCL   |  ODD   |  VBLK  |  HBLK  |  DMA   |  PAL   |

		0,1 = Set Values
		FIFE = FIFO Empty
		FIFF = FIFO Full
		VIP  = Vertical Interrupt Pending
		SOF  = Sprite Overflow  (Not used in C2 afaik)
		SCL  = Sprite Collision (Not used in C2 afaik)
		ODD  = Odd Frame (Interlace Mode) (Not used in C2 afaik)
		VBLK = In Vertical Blank
		HBLK = In Horizontal Blank
		DMA  = DMA In Progress
		PAL  = Pal Mode Flag

	Control Writes are used for setting VDP Registers, setting up DMA Transfers
	etc.

	A Write to the Control port can consist of 2 16-bit Words.
	When the VDP _isn't_ expecting the 2nd part of a command the highest 2 bits
	of the data written will determine what it is we want to do.
	10xxxxxx xxxxxxxx will cause a register to be set
	anything else will trigger the 1st half of a 2 Part Mode Setting Command
	If the VDP is already expecting the 2nd half of a command the data written
	will be the 2nd half of the Mode setup.

******************************************************************************/

static int vdp_control_r(void)
{
	int beampos = cpu_gethorzbeampos();
	int status = 0x3400;

	/* kill 2nd write pending flag */
	vdp_cmdpart = 0;

	/* set the VBLANK bit */
	if (internal_vblank)
		status |= 0x0008;

	/* set the HBLANK bit */
	if (beampos < Machine->visible_area.min_x || beampos > Machine->visible_area.max_x)
		status |= 0x0004;

	return (status);
}


static void vdp_control_w(int data)
{
	/* case 1: we're not expecting the 2nd half of a command */
	if (!vdp_cmdpart)
	{
		/* if 10xxxxxx xxxxxxxx this is a register setting command */
		if ((data & 0xc000) == 0x8000)
			vdp_register_w(data);

		/* otherwise this is the First part of a mode setting command */
		else
		{
			vdp_code    = (vdp_code & 0x3c) | ((data >> 14) & 0x03);
			vdp_address = (vdp_address & 0xc000) | (data & 0x3fff);
			vdp_cmdpart = 1;
		}
	}

	/* case 2: this is the 2nd part of a mode setting command */
	else
	{
		vdp_code    = (vdp_code & 0x03) | ((data >> 2) & 0x3c);
		vdp_address = (vdp_address & 0x3fff) | ((data << 14) & 0xc000);
		vdp_cmdpart = 0;
		vdp_control_dma(data);
	}
}


static void vdp_register_w(int data)
{
	static const UINT8 is_important[32] = { 0,0,1,1,1,1,0,1,0,0,0,1,0,1,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0 };

	UINT8 regnum = (data & 0x1f00) >> 8; /* ---R RRRR ---- ---- */
	UINT8 regdat = (data & 0x00ff);      /* ---- ---- DDDD DDDD */

	segac2_vdp_regs[regnum] = regdat;

	/* these are mostly important writes; force an update if they */
	/* are written during a screen refresh */
	if (!internal_vblank && is_important[regnum])
		force_partial_update((cpu_getscanline())+ scanbase);

	/* For quite a few of the registers its a good idea to set a couple of variable based
	   upon the writes here */
	switch (regnum)
	{
		case 0x01: /* video modes */
			if (regdat & 8)
				usrintf_showmessage("Video height = 240!");
			break;

		case 0x02: /* Scroll A Name Table Base */
			vdp_scrollabase = (regdat & 0x38) << 10;
			break;

		case 0x03: /* Window Name Table Base */
			vdp_windowbase = (regdat & 0x3e) << 10;
			break;

		case 0x04: /* Scroll B Name Table Base */
			vdp_scrollbbase = (regdat & 0x07) << 13;
			break;

		case 0x05: /* Sprite Table Base */
			vdp_spritebase = (regdat & 0x7e) << 9;
			break;

		case 0x07: /* BG Colour */
			bgcol = regdat & 0x3f;
			break;

		case 0x0b: /* Scroll Modes */
		{
			static const UINT16 mask_table[4] = { 0x000, 0x007, 0xff8, 0xfff };
			vdp_vscrollmode = (regdat & 0x04) >> 2;

			vdp_hscrollmask = mask_table[regdat & 3];
			vdp_hscrollsize = 4 * ((vdp_hscrollmask < 224) ? (vdp_hscrollmask + 1) : 224);
			break;
		}

		case 0x0c: /* video modes */
/*			if (!(regdat & 1))
				usrintf_showmessage("Video width = 256!"); */
			break;

		case 0x0d: /* HScroll Base */
			vdp_hscrollbase = (regdat & 0x3f) << 10;
			break;

		case 0x10: /* Scroll Size */
		{
			static const UINT8 size_table[4] = { 32, 64, 128, 128 };
			scrollwidth = size_table[regdat & 0x03];
			scrollheight = size_table[(regdat & 0x30) >> 4];
			break;
		}

		case 0x11: /* Window H Position .. Doesn't Matter for any of the C2 Games */
			window_right = regdat & 0x80;
			window_hpos = (regdat & 0x1f) << 4;
			break;

		case 0x12: /* Window V Position */
			window_down = regdat & 0x80;
			window_vpos = (regdat & 0x1f) << 3;
			break;
	}
}


static void vdp_control_dma(int data)
{
	if ((vdp_code & 0x20) && (segac2_vdp_regs[1] & 0x10))
	{
		switch(segac2_vdp_regs[23] & 0xc0)
		{
			case 0x00:
			case 0x40:		/* 68k -> VRAM Tranfser */
				vdp_dma_68k();
				break;

			case 0x80:		/* VRAM fill, can't be done here, requires data write */
				vdp_dmafill = 1;
				break;

			case 0xC0:		/* VRAM copy */
				vdp_dma_copy();
				break;
		}
	}
}



/******************************************************************************
	DMA handling
*******************************************************************************

	These are currently Pretty much directly from the C2 emu

******************************************************************************/

static void vdp_dma_68k(void)
{
	int length = segac2_vdp_regs[19] | (segac2_vdp_regs[20] << 8);
	int source = (segac2_vdp_regs[21] << 1) | (segac2_vdp_regs[22] << 9) | ((segac2_vdp_regs[23] & 0x7f) << 17);
	int count;

	/* length of 0 means 64k likely */
	if (!length)
		length = 0xffff;

	/* handle the DMA */
	for (count = 0; count < length; count++)
	{
		vdp_data_w(cpu_readmem24bew_word(source));
		source += 2;
	}
}


static void vdp_dma_copy(void)
{
	int length = segac2_vdp_regs[19] | (segac2_vdp_regs[20] << 8);
	int source = segac2_vdp_regs[21] | (segac2_vdp_regs[22] << 8);
	int count;

	/* length of 0 means 64k likely */
	if (!length)
		length = 0xffff;

	/* handle the DMA */
	for (count = 0; count < length; count++)
	{
		VDP_VRAM_BYTE(vdp_address) = VDP_VRAM_BYTE(source++);
		vdp_address += segac2_vdp_regs[15];
	}
}


static void vdp_dma_fill(int data)
{
	int length = segac2_vdp_regs[19] | (segac2_vdp_regs[20] << 8);
	int count;

	/* length of 0 means 64k likely */
	if (!length)
		length = 0xffff;

	/* handle the fill */
	VDP_VRAM_BYTE(vdp_address) = data;
	data >>= 8;
	for(count = 0; count < length; count++)
	{
		VDP_VRAM_BYTE(vdp_address ^ 1) = data;
		vdp_address += segac2_vdp_regs[15];
	}
}



/******************************************************************************
	Scroll calculations
*******************************************************************************

	These special routines handle the transparency effects in sprites.

******************************************************************************/

/*** Useful Little Functions **************************************************/

/* Note: We Expect plane = 0 for Scroll A, plane = 2 for Scroll B */
static INLINE int vdp_gethscroll(int plane, int line)
{
	line &= vdp_hscrollmask;
	return 0x400 - (VDP_VRAM_WORD(vdp_hscrollbase + (4 * line) + plane) & 0x3ff);
}


/* Note: We expect plane = 0 for Scroll A, plane = 2 for Scroll B
   A Column is 8 Pixels Wide                                     */
static int vdp_getvscroll(int plane, int column)
{
	UINT32 vsramoffset;

	switch (vdp_vscrollmode)
	{
		case 0x00: /* Overall Scroll */
			return VDP_VSRAM_WORD(plane) & 0x7ff;

		case 0x01: /* Column Scroll */
			if (column == 40) column = 39; /* Fix Minor Innacuracy Only affects PotoPoto */
			vsramoffset = (4 * (column >> 1)) + plane;
			return VDP_VSRAM_WORD(vsramoffset) & 0x7ff;
	}
	return 0;
}



/******************************************************************************
	Drawing Functions
*******************************************************************************

	These are used by the Screen Refresh functions to do the actual rendering
	of a screenline to Machine->scrbitmap.

	Draw Planes in Order
		Scroll B Low
		Scroll A Low / Window Low
		Sprites Low
		Scroll B High
		Scroll A High / Window High
		Sprites High

	NOTE: Low Sprites _can_ overlap High sprites, however none of the C2
			games do this ever so its safe to draw them in this order.

******************************************************************************/

static void drawline(UINT16 *bitmap, int line)
{
	int lowsprites, highsprites, link;
	UINT32 scrolla_tiles[41], scrollb_tiles[41], window_tiles[41];
	int scrolla_offset, scrollb_offset;
	UINT8 *lowlist[81], *highlist[81];
	int bgcolor = bgcol + segac2_palbank;
	int window_lclip, window_rclip;
	int scrolla_lclip, scrolla_rclip;
	int column, sprite;

	/* clear to the background color */
	for (column = 0; column < BITMAP_WIDTH; column++)
		bitmap[column] = bgcolor;

	/* if display is disabled, stop */
	if (!(segac2_vdp_regs[1] & 0x40) || !display_enable)
		return;

	/* Sprites need to be Drawn in Reverse order .. may as well sort them here */
	link = lowsprites = highsprites = 0;
	for (sprite = 0; sprite < 80; sprite++)
	{
		UINT8 *spritebase = &VDP_VRAM_BYTE(vdp_spritebase + 8 * link);

		/* sort into high/low priorities */
		if (spritebase[4] & 0x0080)
			highlist[++highsprites] = spritebase;
		else
			lowlist[++lowsprites] = spritebase;

		/* get the link; if 0, stop processing */
		link = spritebase[3] & 0x7F;
		if (!link)
			break;
	}

	/* get tiles for the B scroll layer */
	get_scroll_tiles(line, 2, vdp_scrollbbase, scrollb_tiles, &scrollb_offset);

	/* get tiles for the A scroll layer */
	get_scroll_tiles(line, 0, vdp_scrollabase, scrolla_tiles, &scrolla_offset);

	/* get tiles for the window layer */
	get_window_tiles(line, vdp_windowbase, window_tiles);

	/* compute the windowing for this line */
	if ((window_down && line >= window_vpos) || (!window_down && line < window_vpos))
		window_lclip = 0, window_rclip = BITMAP_WIDTH - 1;
	else if (window_right)
		window_lclip = window_hpos, window_rclip = BITMAP_WIDTH - 1;
	else
		window_lclip = 0, window_rclip = window_hpos - 1;

	/* compute the clipping of the scroll A layer */
	if (window_lclip == 0)
	{
		scrolla_lclip = window_rclip + 1;
		scrolla_rclip = BITMAP_WIDTH - 1;
	}
	else
	{
		scrolla_lclip = 0;
		scrolla_rclip = window_lclip - 1;
	}

	/* Scroll B Low */
	drawline_tiles(scrollb_tiles, bitmap, 0, scrollb_offset, 0, BITMAP_WIDTH - 1);

	/* Scroll A Low */
	drawline_tiles(scrolla_tiles, bitmap, 0, scrolla_offset, scrolla_lclip, scrolla_rclip);

	/* Window Low */
	drawline_tiles(window_tiles, bitmap, 0, 0, window_lclip, window_rclip);

	/* Sprites Low */
	for (sprite = lowsprites; sprite > 0; sprite--)
		drawline_sprite(line, bitmap, 0, lowlist[sprite]);

	/* Scroll B High */
	drawline_tiles(scrollb_tiles, bitmap, 1, scrollb_offset, 0, BITMAP_WIDTH - 1);

	/* Scroll A High */
	drawline_tiles(scrolla_tiles, bitmap, 1, scrolla_offset, scrolla_lclip, scrolla_rclip);

	/* Window High */
	drawline_tiles(window_tiles, bitmap, 1, 0, window_lclip, window_rclip);

	/* Sprites High */
	for (sprite = highsprites; sprite > 0; sprite--)
		drawline_sprite(line, bitmap, 1, highlist[sprite]);
}



/******************************************************************************
	Background rendering
*******************************************************************************

	This code handles all the scroll calculations and rendering of the
	background layers.

******************************************************************************/

/* determine the tiles we will draw on a scrolling layer */
static void get_scroll_tiles(int line, int scrollnum, UINT32 scrollbase, UINT32 *tiles, int *offset)
{
	int linehscroll = vdp_gethscroll(scrollnum, line);
	int column;

	/* adjust for partial tiles and then pre-divide hscroll to get the tile offset */
	*offset = -(linehscroll % 8);
	linehscroll /= 8;

	/* loop over columns */
	for (column = 0; column < 41; column++)
	{
		int columnvscroll = vdp_getvscroll(scrollnum, column) + line;

		/* determine the base of the tilemap row */
		int temp = ((columnvscroll / 8) & (scrollheight - 1)) * scrollwidth;
		int tilebase = scrollbase + 2 * temp;

		/* offset into the tilemap based on the column */
		temp = (linehscroll + column) & (scrollwidth - 1);
		tilebase += 2 * temp;

		/* get the tile info */
		*tiles++ = ((columnvscroll % 8) << 16) | VDP_VRAM_WORD(tilebase);
	}
}


/* determine the tiles we will draw on a non-scrolling window layer */
static void get_window_tiles(int line, UINT32 scrollbase, UINT32 *tiles)
{
	int column;

	/* loop over columns */
	for (column = 0; column < 40; column++)
	{
		/* determine the base of the tilemap row */
		int temp = (line / 8) * 64 + column;
		int tilebase = scrollbase + 2 * temp;

		/* get the tile info */
		*tiles++ = ((line % 8) << 16) | VDP_VRAM_WORD(tilebase);
	}
}


/* draw a line of tiles */
static void drawline_tiles(UINT32 *tiles, UINT16 *bmap, int pri, int offset, int lclip, int rclip)
{
	/* adjust for the 8-pixel slop */
	bmap += offset;
	if (lclip > rclip)
		return;

	/* loop over columns */
	for ( ; offset < BITMAP_WIDTH; offset += 8, bmap += 8)
	{
		UINT32 tile = *tiles++;

		/* if the tile is the correct priority, draw it */
		if (((tile >> 15) & 1) == pri && offset < BITMAP_WIDTH)
		{
			int colbase = 16 * ((tile & 0x6000) >> 13) + segac2_bg_palbase + segac2_palbank;
			UINT32 *tp = (UINT32 *)&VDP_VRAM_BYTE((tile & 0x7ff) * 32);
			UINT32 mytile;
			int col;

			/* vertical flipping */
			if (!(tile & 0x1000))
				mytile = tp[tile >> 16];
			else
				mytile = tp[(tile >> 16) ^ 7];

			/* skip if all-transparent */
			if (!mytile)
				continue;

			/* non-clipped */
			if (offset >= lclip && offset <= rclip - 7)
			{
				/* non-flipped */
				if (!(tile & 0x0800))
				{
					col = EXTRACT_PIXEL(mytile, 0); if (col) bmap[0] = colbase + col;
					col = EXTRACT_PIXEL(mytile, 1); if (col) bmap[1] = colbase + col;
					col = EXTRACT_PIXEL(mytile, 2); if (col) bmap[2] = colbase + col;
					col = EXTRACT_PIXEL(mytile, 3); if (col) bmap[3] = colbase + col;
					col = EXTRACT_PIXEL(mytile, 4); if (col) bmap[4] = colbase + col;
					col = EXTRACT_PIXEL(mytile, 5); if (col) bmap[5] = colbase + col;
					col = EXTRACT_PIXEL(mytile, 6); if (col) bmap[6] = colbase + col;
					col = EXTRACT_PIXEL(mytile, 7); if (col) bmap[7] = colbase + col;
				}

				/* horizontal flip */
				else
				{
					col = EXTRACT_PIXEL(mytile, 7); if (col) bmap[0] = colbase + col;
					col = EXTRACT_PIXEL(mytile, 6); if (col) bmap[1] = colbase + col;
					col = EXTRACT_PIXEL(mytile, 5); if (col) bmap[2] = colbase + col;
					col = EXTRACT_PIXEL(mytile, 4); if (col) bmap[3] = colbase + col;
					col = EXTRACT_PIXEL(mytile, 3); if (col) bmap[4] = colbase + col;
					col = EXTRACT_PIXEL(mytile, 2); if (col) bmap[5] = colbase + col;
					col = EXTRACT_PIXEL(mytile, 1); if (col) bmap[6] = colbase + col;
					col = EXTRACT_PIXEL(mytile, 0); if (col) bmap[7] = colbase + col;
				}
			}

			/* clipped */
			else if (offset >= lclip - 8 && offset <= rclip)
			{
				/* non-flipped */
				if (!(tile & 0x0800))
				{
					col = EXTRACT_PIXEL(mytile, 0); if (col && (offset + 0) >= lclip && (offset + 0) <= rclip) bmap[0] = colbase + col;
					col = EXTRACT_PIXEL(mytile, 1); if (col && (offset + 1) >= lclip && (offset + 1) <= rclip) bmap[1] = colbase + col;
					col = EXTRACT_PIXEL(mytile, 2); if (col && (offset + 2) >= lclip && (offset + 2) <= rclip) bmap[2] = colbase + col;
					col = EXTRACT_PIXEL(mytile, 3); if (col && (offset + 3) >= lclip && (offset + 3) <= rclip) bmap[3] = colbase + col;
					col = EXTRACT_PIXEL(mytile, 4); if (col && (offset + 4) >= lclip && (offset + 4) <= rclip) bmap[4] = colbase + col;
					col = EXTRACT_PIXEL(mytile, 5); if (col && (offset + 5) >= lclip && (offset + 5) <= rclip) bmap[5] = colbase + col;
					col = EXTRACT_PIXEL(mytile, 6); if (col && (offset + 6) >= lclip && (offset + 6) <= rclip) bmap[6] = colbase + col;
					col = EXTRACT_PIXEL(mytile, 7); if (col && (offset + 7) >= lclip && (offset + 7) <= rclip) bmap[7] = colbase + col;
				}

				/* horizontal flip */
				else
				{
					col = EXTRACT_PIXEL(mytile, 7); if (col && (offset + 0) >= lclip && (offset + 0) <= rclip) bmap[0] = colbase + col;
					col = EXTRACT_PIXEL(mytile, 6); if (col && (offset + 1) >= lclip && (offset + 1) <= rclip) bmap[1] = colbase + col;
					col = EXTRACT_PIXEL(mytile, 5); if (col && (offset + 2) >= lclip && (offset + 2) <= rclip) bmap[2] = colbase + col;
					col = EXTRACT_PIXEL(mytile, 4); if (col && (offset + 3) >= lclip && (offset + 3) <= rclip) bmap[3] = colbase + col;
					col = EXTRACT_PIXEL(mytile, 3); if (col && (offset + 4) >= lclip && (offset + 4) <= rclip) bmap[4] = colbase + col;
					col = EXTRACT_PIXEL(mytile, 2); if (col && (offset + 5) >= lclip && (offset + 5) <= rclip) bmap[5] = colbase + col;
					col = EXTRACT_PIXEL(mytile, 1); if (col && (offset + 6) >= lclip && (offset + 6) <= rclip) bmap[6] = colbase + col;
					col = EXTRACT_PIXEL(mytile, 0); if (col && (offset + 7) >= lclip && (offset + 7) <= rclip) bmap[7] = colbase + col;
				}
			}
		}
	}
}



/******************************************************************************
	Sprite drawing
*******************************************************************************

	These special routines handle the transparency effects in sprites.

******************************************************************************/

/* draw a non-horizontally-flipped section of a sprite */
static INLINE void draw8pixs(UINT16 *bmap, int patno, int priority, int colbase, int patline)
{
	UINT32 tile = *(UINT32 *)&VDP_VRAM_BYTE(patno * 32 + 4 * patline);
	int col;

	/* skip if all-transparent */
	if (!tile)
		return;

	/* non-transparent */
	if ((colbase & 0x30) != 0x30 || !(segac2_vdp_regs[12] & 0x08))
	{
		col = EXTRACT_PIXEL(tile, 0); if (col) bmap[0] = colbase + col;
		col = EXTRACT_PIXEL(tile, 1); if (col) bmap[1] = colbase + col;
		col = EXTRACT_PIXEL(tile, 2); if (col) bmap[2] = colbase + col;
		col = EXTRACT_PIXEL(tile, 3); if (col) bmap[3] = colbase + col;
		col = EXTRACT_PIXEL(tile, 4); if (col) bmap[4] = colbase + col;
		col = EXTRACT_PIXEL(tile, 5); if (col) bmap[5] = colbase + col;
		col = EXTRACT_PIXEL(tile, 6); if (col) bmap[6] = colbase + col;
		col = EXTRACT_PIXEL(tile, 7); if (col) bmap[7] = colbase + col;
	}

	/* transparent */
	else
	{
		col = EXTRACT_PIXEL(tile, 0);
		if (col)
		{
			if (col < 0x0e) bmap[0] = colbase + col;
			else bmap[0] = transparent_lookup[((col & 1) << 11) | (bmap[0] & 0x7ff)];
		}
		col = EXTRACT_PIXEL(tile, 1);
		if (col)
		{
			if (col < 0x0e) bmap[1] = colbase + col;
			else bmap[1] = transparent_lookup[((col & 1) << 11) | (bmap[1] & 0x7ff)];
		}
		col = EXTRACT_PIXEL(tile, 2);
		if (col)
		{
			if (col < 0x0e) bmap[2] = colbase + col;
			else bmap[2] = transparent_lookup[((col & 1) << 11) | (bmap[2] & 0x7ff)];
		}
		col = EXTRACT_PIXEL(tile, 3);
		if (col)
		{
			if (col < 0x0e) bmap[3] = colbase + col;
			else bmap[3] = transparent_lookup[((col & 1) << 11) | (bmap[3] & 0x7ff)];
		}
		col = EXTRACT_PIXEL(tile, 4);
		if (col)
		{
			if (col < 0x0e) bmap[4] = colbase + col;
			else bmap[4] = transparent_lookup[((col & 1) << 11) | (bmap[4] & 0x7ff)];
		}
		col = EXTRACT_PIXEL(tile, 5);
		if (col)
		{
			if (col < 0x0e) bmap[5] = colbase + col;
			else bmap[5] = transparent_lookup[((col & 1) << 11) | (bmap[5] & 0x7ff)];
		}
		col = EXTRACT_PIXEL(tile, 6);
		if (col)
		{
			if (col < 0x0e) bmap[6] = colbase + col;
			else bmap[6] = transparent_lookup[((col & 1) << 11) | (bmap[6] & 0x7ff)];
		}
		col = EXTRACT_PIXEL(tile, 7);
		if (col)
		{
			if (col < 0x0e) bmap[7] = colbase + col;
			else bmap[7] = transparent_lookup[((col & 1) << 11) | (bmap[7] & 0x7ff)];
		}
	}
}


/* draw a horizontally-flipped section of a sprite */
static INLINE void draw8pixs_hflip(UINT16 *bmap, int patno, int priority, int colbase, int patline)
{
	UINT32 tile = *(UINT32 *)&VDP_VRAM_BYTE(patno * 32 + 4 * patline);
	int col;

	/* skip if all-transparent */
	if (!tile)
		return;

	/* non-transparent */
	if ((colbase & 0x30) != 0x30 || !(segac2_vdp_regs[12] & 0x08))
	{
		col = EXTRACT_PIXEL(tile, 7); if (col) bmap[0] = colbase + col;
		col = EXTRACT_PIXEL(tile, 6); if (col) bmap[1] = colbase + col;
		col = EXTRACT_PIXEL(tile, 5); if (col) bmap[2] = colbase + col;
		col = EXTRACT_PIXEL(tile, 4); if (col) bmap[3] = colbase + col;
		col = EXTRACT_PIXEL(tile, 3); if (col) bmap[4] = colbase + col;
		col = EXTRACT_PIXEL(tile, 2); if (col) bmap[5] = colbase + col;
		col = EXTRACT_PIXEL(tile, 1); if (col) bmap[6] = colbase + col;
		col = EXTRACT_PIXEL(tile, 0); if (col) bmap[7] = colbase + col;
	}

	/* transparent */
	else
	{
		col = EXTRACT_PIXEL(tile, 7);
		if (col)
		{
			if (col < 0x0e) bmap[0] = colbase + col;
			else bmap[0] = transparent_lookup[((col & 1) << 11) | (bmap[0] & 0x7ff)];
		}
		col = EXTRACT_PIXEL(tile, 6);
		if (col)
		{
			if (col < 0x0e) bmap[1] = colbase + col;
			else bmap[1] = transparent_lookup[((col & 1) << 11) | (bmap[1] & 0x7ff)];
		}
		col = EXTRACT_PIXEL(tile, 5);
		if (col)
		{
			if (col < 0x0e) bmap[2] = colbase + col;
			else bmap[2] = transparent_lookup[((col & 1) << 11) | (bmap[2] & 0x7ff)];
		}
		col = EXTRACT_PIXEL(tile, 4);
		if (col)
		{
			if (col < 0x0e) bmap[3] = colbase + col;
			else bmap[3] = transparent_lookup[((col & 1) << 11) | (bmap[3] & 0x7ff)];
		}
		col = EXTRACT_PIXEL(tile, 3);
		if (col)
		{
			if (col < 0x0e) bmap[4] = colbase + col;
			else bmap[4] = transparent_lookup[((col & 1) << 11) | (bmap[4] & 0x7ff)];
		}
		col = EXTRACT_PIXEL(tile, 2);
		if (col)
		{
			if (col < 0x0e) bmap[5] = colbase + col;
			else bmap[5] = transparent_lookup[((col & 1) << 11) | (bmap[5] & 0x7ff)];
		}
		col = EXTRACT_PIXEL(tile, 1);
		if (col)
		{
			if (col < 0x0e) bmap[6] = colbase + col;
			else bmap[6] = transparent_lookup[((col & 1) << 11) | (bmap[6] & 0x7ff)];
		}
		col = EXTRACT_PIXEL(tile, 0);
		if (col)
		{
			if (col < 0x0e) bmap[7] = colbase + col;
			else bmap[7] = transparent_lookup[((col & 1) << 11) | (bmap[7] & 0x7ff)];
		}
	}
}


static void drawline_sprite(int line, UINT16 *bmap, int priority, UINT8 *spritebase)
{
	int spriteypos   = (((spritebase[0] & 0x01) << 8) | spritebase[1]) - 0x80;
	int spritexpos   = (((spritebase[6] & 0x01) << 8) | spritebase[7]) - 0x80;
	int spriteheight = ((spritebase[2] & 0x03) + 1) * 8;
	int spritewidth  = (((spritebase[2] & 0x0c) >> 2) + 1) * 8;
	int spriteattr, patno, patflip, patline, colbase, x;

	/* skip if out of range */
	if (line < spriteypos || line >= spriteypos + spriteheight)
		return;
	if (spritexpos + spritewidth < 0 || spritexpos >= BITMAP_WIDTH)
		return;

	/* extract the remaining data */
	spriteattr = (spritebase[4] << 8) | spritebase[5];
	patno      = spriteattr & 0x07FF;
	patflip    = (spriteattr & 0x1800) >> 11;
	patline    = line - spriteypos;

	/* determine the color base */
	colbase = 16 * ((spriteattr & 0x6000) >> 13) + segac2_sp_palbase + segac2_palbank;

	/* adjust for the X position */
	spritewidth >>= 3;
	spriteheight >>= 3;

	/* switch off the flip mode */
	bmap += spritexpos;
	switch (patflip)
	{
		case 0x00: /* No Flip */
			for (x = 0; x < spritewidth; x++, bmap += 8)
			{
				if (spritexpos >= -7 && spritexpos < BITMAP_WIDTH)
					draw8pixs(bmap, patno, priority, colbase, patline);
				spritexpos += 8;
				patno += spriteheight;
			}
			break;

		case 0x01: /* Horizontal Flip */
			patno += spriteheight * (spritewidth - 1);
			for (x = 0; x < spritewidth; x++, bmap += 8)
			{
				if (spritexpos >= -7 && spritexpos < BITMAP_WIDTH)
					draw8pixs_hflip(bmap, patno, priority, colbase, patline);
				spritexpos += 8;
				patno -= spriteheight;
			}
			break;

		case 0x02: /* Vertical Flip */
			patline = 8 * spriteheight - patline - 1;
			for (x = 0; x < spritewidth; x++, bmap += 8)
			{
				if (spritexpos >= -7 && spritexpos < BITMAP_WIDTH)
					draw8pixs(bmap, patno, priority, colbase, patline);
				spritexpos += 8;
				patno += spriteheight;
			}
			break;

		case 0x03: /* Both Flip */
			patno += spriteheight * (spritewidth - 1);
			patline = 8 * spriteheight - patline - 1;
			for (x = 0; x < spritewidth; x++, bmap += 8)
			{
				if (spritexpos >= -7 && spritexpos < BITMAP_WIDTH)
					draw8pixs_hflip(bmap, patno, priority, colbase, patline);
				spritexpos += 8;
				patno -= spriteheight;
			}
			break;
	}
}



/******************************************************************************/
/* General Information (Mainly Genesis Related                                */
/******************************************************************************/
/* Genesis VDP Registers (from sega2.doc)

Reg# : |  Bit7  |  Bit6  |  Bit5  |  Bit4  |  Bit3  |  Bit2  |  Bit1  |  Bit0  |    General Function
--------------------------------------------------------------------------------
0x00 : |  0     |  0     |  0     |  IE1   |  0     |  1     |  M3    |  0     |    Mode Set Register #1
IE = Enabled H Interrupt (Lev 4), M3 = Enable HV Counter Read / Stopped
0x01 : |  0     |  DISP  |  IE0   |  M1    |  M2    |  1     |  0     |  0     |    Mode Set Register #2
DISP = Display Enable, IE0 = Enabled V Interrupt (Lev 6), M1 = DMA Enabled, M2 = 30 Cell Mode
0x02 : |  0     |  0     |  SA15  |  SA14  |  SA13  |  0     |  0     |  0     |    Scroll A Base Address
SA13-15 = Bits 13-15 of the Scroll A Name Table Base Address in VRAM
0x03 : |  0     |  0     |  WD15  |  WD14  |  WD13  |  WD12  |  WD11  |  0     |    Window Base Address
WD11-15 = Bits 11-15 of the Window Name Table Base Address in VRAM
0x04 : |  0     |  0     |  0     |   0    |  0     |  SB15  |  SB14  |  SB13  |    Scroll B Base Address
SB13-15 = Bits 13-15 of the Scroll B Name Table Base Address in VRAM
0x05 : |  0     |  AT15  |  AT14  |  AT13  |  AT12  |  AT11  |  AT10  |  AT9   |    Sprite Table Base Address
AT9=15 = Bits 9-15 of the Sprite Name Table Base Address in VRAM
0x06 : |  0     |  0     |  0     |  0     |  0     |  0     |  0     |  0     |    Unused
0x07 : |  0     |  0     |  CPT1  |  CPT0  |  COL3  |  COL2  |  COL1  |  COL0  |    Background Colour Select
CPT0-1 = Palette Number, COL = Colour in Palette
0x08 : |  0     |  0     |  0     |  0     |  0     |  0     |  0     |  0     |    Unused
0x09 : |  0     |  0     |  0     |  0     |  0     |  0     |  0     |  0     |    Unused
0x0A : |  BIT7  |  BIT6  |  BIT5  |  BIT4  |  BIT3  |  BIT2  |  BIT1  |  BIT0  |    H-Interrupt Register
BIT0-7 = Controls Level 4 Interrupt Timing
0x0B : |  0     |  0     |  0     |  0     |  IE2   |  VSCR  |  HSCR  |  LSCR  |    Mode Set Register #3
IE2 = Enable E Interrupt (Lev 2), VSCR = Vertical Scroll Mode, HSCR / LSCR = Horizontal Scroll Mode
0x0C : |  RS0   |  0     |  0     |  0     |  S/TE  |  LSM1  |  LSM0  |  RS1   |    Mode Set Register #4
RS0 / RS1 = Cell Mode, S/TE = Shadow/Hilight Enable, LSM0 / LSM1 = Interlace Mode Setting
0x0D : |  0     |  0     |  HS15  |  HS14  |  HS13  |  HS12  |  HS11  |  HS10  |    HScroll Base Address
HS10-15 = Bits 10-15 of the HScroll Name Table Base Address in VRAM
0x0E : |  0     |  0     |  0     |  0     |  0     |  0     |  0     |  0     |    Unused
0x0F : |  INC7  |  INC6  |  INC5  |  INC4  |  INC3  |  INC2  |  INC1  |  INC0  |    Auto-Increment
INC0-7 = Auto Increment Value (after VRam Access)
0x10 : |  0     |  0     |  VSZ1  |  VSZ0  |  0     |  0     |  HSZ1  |  HSZ0  |    Scroll Size
VSZ0-1 = Vertical Plane Size, HSZ0-1 = Horizontal Plane Size
0x11 : |  RIGT  |  0     |  0     |  WHP5  |  WHP4  |  WHP3  |  WHP2  |  WHP1  |    Window H Position
RIGT = Window Right Side of Base Point, WHP1-5 = Bits1-5 of Window H Point
0x12 : |  DOWN  |  0     |  0     |  WVP4  |  WVP3  |  WVP2  |  WVP1  |  WVP0  |    Window V Position
DOWN = Window Below Base Point, WVP0-4 = Bits0-4 of Window V Point
0x13 : |  LG7   |  LG6   |  LG5   |  LG4   |  LG3   |  LG2   |  LG1   |  LG0   |    DMA Length Counter LOW
0x14 : |  LG15  |  LG14  |  LG13  |  LG12  |  LG11  |  LG10  |  LG9   |  LG8   |    DMA Length Counter HIGH
LG0-15 = Bits 0-15 of DMA Length Counter
0x15 : |  SA8   |  SA7   |  SA6   |  SA5   |  SA4   |  SA3   |  SA2   |  SA1   |    DMA Source Address LOW
0x16 : |  SA16  |  SA15  |  SA14  |  SA13  |  SA12  |  SA11  |  SA10  |  SA9   |    DMA Source Address MID
0x17 : |  DMD1  |  DMD0  |  SA22  |  SA21  |  SA20  |  SA19  |  SA18  |  S17   |    DMA Source Address HIGH
LG0-15 = Bits 1-22 of DMA Source Address
DMD0-1 = DMA Mode

Memory Layouts ...

Scroll Name Table
16-bits are used to Define a Tile in the Scroll Plane
|  PRI   |  CP1   |  CP0   |  VF    |  HF    |  PT10  |  PT9   |  PT8   |
|  PT7   |  PT6   |  PT5   |  PT4   |  PT3   |  PT2   |  PT1   |  PT0   |
PRI = Priority, CP = Colour Palette, VF = VFlip, HF = HFlip, PT0-9 = Tile # in VRAM

HScroll Data  (in VRAM)
0x00 |
0x01 / H Scroll of Plane A (Used in Overall, Cell & Line Modes)
0x02 |
0x03 / H Scroll of Plane B (Used in Overall, Cell & Line Modes)
0x04 |
0x05 / H Scroll of Plane A (Used in Line Mode)
0x06 |
0x07 / H Scroll of Plane B (Used in Line Mode)
...
0x20 |
0x21 / H Scroll of Plane A (Used in Cell & Line Mode)
0x22 |
0x23 / H Scroll of Plane B (Used in Cell & Line Mode)
etc.. That kinda thing :)
Data is in Format ..
|  x     |  x     |  x     |  x     |  x     |  x     |  HS9   |  HS8   |
|  HS7   |  HS6   |  HS5   |  HS4   |  HS3   |  HS2   |  HS1   |  HS0   |
HS = HScroll Amount for Overall / Cell / Line depending on Mode.

VScroll Data (in VSRAM)
0x00 |
0x01 / V Scroll of Plane A (Used in Overall & Cell Modes)
0x02 |
0x03 / V Scroll of Plane B (Used in Overall & Cell Modes)
0x04 |
0x05 / V Scroll of Plane A (Used in Cell Mode)
0x06 |
0x07 / V Scroll of Plane B (Used in Cell Modes)
etc..
Data is in Format ..
|  x     |  x     |  x     |  x     |  x     |  VS10  |  VS9   |  VS8   |
|  VS7   |  VS6   |  VS5   |  VS4   |  VS3   |  VS2   |  VS1   |  VS0   |
VS = HScroll Amount for Overall / Cell / Line depending on Mode.

Sprite Attributes Table (in VRAM)
Each Sprite uses 64-bits of Information (8 bytes)
|  x     |  x     |  x     |  x     |  x     |  x     |  YP9   |  YP8   |
|  YP7   |  YP6   |  YP5   |  YP4   |  YP3   |  YP2   |  YP1   |  YP0   |
|  x     |  x     |  x     |  x     |  HS1   |  HS0   |  VS1   |  VS0   |
|  x     |  LN6   |  LN5   |  LN4   |  LN3   |  LN2   |  LN1   |  LN0   |
|  PRI   |  CP1   |  CP0   |  VF    |  HF    |  PT10  |  PT9   |  PT8   |
|  PT7   |  PT6   |  PT5   |  PT4   |  PT3   |  PT2   |  PT1   |  PT0   |
|  x     |  x     |  x     |  x     |  x     |  x     |  XP9   |  XP8   |
|  XP7   |  XP6   |  XP5   |  XP4   |  XP3   |  XP2   |  XP1   |  XP0   |
YP = Y Position of Sprite
HS = Horizontal Size (Blocks)
VS = Vertical Size (Blocks)
LN = Link Field
PRI = Priority
CP = Colour Palette
VF = VFlip
HF = HFlip
PT = Pattern Number
XP = X Position of Sprite



*/
