/***************************************************************************

							-= Kaneko 16 Bit Games =-

					driver by	Luca Elia (l.elia@tin.it)


Note:	if MAME_DEBUG is defined, pressing:

		Q  with  X/C/V/B/ Z   shows Layer 0 (tiles with priority 0/1/2/3/ All)
		W  with  X/C/V/B/ Z   shows Layer 1 (tiles with priority 0/1/2/3/ All)
		E  with  X/C/V/B/ Z   shows Layer 2 (tiles with priority 0/1/2/3/ All)
		R  with  X/C/V/B/ Z   shows Layer 3 (tiles with priority 0/1/2/3/ All)
		A  with  X/C/V/B/ Z   shows Sprites (tiles with priority 0/1/2/3/ All)

		Keys can be used together!

	[ 1 High Color Layer ]

		In ROM	(Optional)

	[ Scrolling Layers ]

		Each VIEW2 chip generates 2 layers. Up to 2 chips are used
		(4 layers)

		Layer Size:				512 x 512
		Tiles:					16 x 16 x 4

		Line scroll is supported by the chip: each layer has RAM
		for 512 horizontal scroll offsets (one per tilemap line)
		that are added to the global scroll values.
		See e.g. blazeon (2nd demo level), mgcrystl, sandscrp.

	[ 1024 Sprites ]

        Sprites are 16 x 16 x 4 in the older games, 16 x 16 x 8 in
        gtmr & gtmr2.
        Sprites types 0 and 2 can also have a simple effect keeping
        sprites on the screen


**************************************************************************/

#include "vidhrdw/generic.h"
#include "kaneko16.h"

struct tilemap *kaneko16_tmap_0, *kaneko16_tmap_1;
struct tilemap *kaneko16_tmap_2, *kaneko16_tmap_3;
data16_t *kaneko16_vram_0,    *kaneko16_vram_1,    *kaneko16_layers_0_regs;
data16_t *kaneko16_vscroll_0, *kaneko16_vscroll_1;
data16_t *kaneko16_vram_2,    *kaneko16_vram_3,    *kaneko16_layers_1_regs;
data16_t *kaneko16_vscroll_2, *kaneko16_vscroll_3;
int kaneko16_sprite_fliptype = 0;

int kaneko16_sprite_type;
int kaneko16_keep_sprites = 0; /* default disabled for games not using it */
data16_t kaneko16_sprite_xoffs, kaneko16_sprite_flipx;
data16_t kaneko16_sprite_yoffs, kaneko16_sprite_flipy;
data16_t *kaneko16_sprites_regs;


data16_t *kaneko16_bg15_select, *kaneko16_bg15_reg;
static struct mame_bitmap *kaneko16_bg15_bitmap;
static struct mame_bitmap *sprites_bitmap; /* bitmap used for to keep sprites on screen (mgcrystl 1st boss)*/

struct tempsprite
{
	int code,color;
	int x,y;
	int xoffs,yoffs;
	int flipx,flipy;
	int priority;
};

struct
{
	struct tempsprite *first_sprite;
}	spritelist;


kaneko16_priority_t kaneko16_priority;


/***************************************************************************

						Callbacks for the TileMap code

							  [ Tiles Format ]

Offset:

0000.w			fedc ba-- ---- ----		unused?
				---- --9- ---- ----		High Priority (vs Sprites)
				---- ---8 ---- ----		High Priority (vs Tiles)
				---- ---- 7654 32--		Color
				---- ---- ---- --1-		Flip X
				---- ---- ---- ---0		Flip Y

0002.w									Code

***************************************************************************/


#define KANEKO16_LAYER(_N_) \
static void get_tile_info_##_N_(int tile_index) \
{ \
	data16_t code_hi = kaneko16_vram_##_N_[ 2 * tile_index + 0]; \
	data16_t code_lo = kaneko16_vram_##_N_[ 2 * tile_index + 1]; \
	SET_TILE_INFO(1 + _N_/2, code_lo, (code_hi >> 2) & 0x3f, TILE_FLIPXY( code_hi & 3 )); \
	tile_info.priority	=	(code_hi >> 8) & 3; \
} \
\
WRITE16_HANDLER( kaneko16_vram_##_N_##_w ) \
{ \
	data16_t old_data	=	kaneko16_vram_##_N_[offset]; \
	data16_t new_data	=	COMBINE_DATA(&kaneko16_vram_##_N_[offset]); \
	if (old_data != new_data)	tilemap_mark_tile_dirty(kaneko16_tmap_##_N_, offset/2); \
}


KANEKO16_LAYER(0)
KANEKO16_LAYER(1)
KANEKO16_LAYER(2)
KANEKO16_LAYER(3)

VIDEO_START( kaneko16_sprites )
{
	/* 0x400 sprites max */
	spritelist.first_sprite = (struct tempsprite *)auto_malloc(0x400 * sizeof(spritelist.first_sprite[0]));

	if (	!spritelist.first_sprite	)
		return 1;

	return 0;
}

VIDEO_START( kaneko16_1xVIEW2 )
{
	if (	video_start_kaneko16_sprites()	)
		return 1;

	kaneko16_tmap_0 = tilemap_create(	get_tile_info_0, tilemap_scan_rows,
										TILEMAP_TRANSPARENT, 16,16, 0x20,0x20	);
	kaneko16_tmap_1 = tilemap_create(	get_tile_info_1, tilemap_scan_rows,
										TILEMAP_TRANSPARENT, 16,16, 0x20,0x20	);

	kaneko16_tmap_2 = 0;

	kaneko16_tmap_3 = 0;

	if ((sprites_bitmap = auto_bitmap_alloc(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
		return 1;

	if (	!kaneko16_tmap_0 || !kaneko16_tmap_1	)
		return 1;

	{
		int dx, xdim = Machine->drv->screen_width;
		int dy, ydim = Machine->drv->screen_height;

		switch (xdim)
		{
			case 320:	dx = 0x33;	break;
			case 256:	dx = 0x5b;	break;
			default:	dx = 0;
		}
		switch (Machine->visible_area.max_y - Machine->visible_area.min_y + 1)
		{
			case 240- 8:	dy = +0x08;	break;	/* blazeon */
			case 240-16:	dy = -0x08;	break;	/* berlwall, bakubrk */
			default:		dy = 0;
		}

		tilemap_set_scrolldx( kaneko16_tmap_0, -dx,		xdim + dx -1        );
		tilemap_set_scrolldx( kaneko16_tmap_1, -(dx+2),	xdim + (dx + 2) - 1 );

		tilemap_set_scrolldy( kaneko16_tmap_0, -dy,		ydim + dy -1 );
		tilemap_set_scrolldy( kaneko16_tmap_1, -dy,		ydim + dy -1 );

		tilemap_set_transparent_pen(kaneko16_tmap_0, 0);
		tilemap_set_transparent_pen(kaneko16_tmap_1, 0);

		tilemap_set_scroll_rows(kaneko16_tmap_0, 0x200);	/* Line Scroll*/
		tilemap_set_scroll_rows(kaneko16_tmap_1, 0x200);

		return 0;
	}
}

VIDEO_START( kaneko16_2xVIEW2 )
{
	if (	video_start_kaneko16_1xVIEW2()	)
		return 1;

	kaneko16_tmap_2 = tilemap_create(	get_tile_info_2, tilemap_scan_rows,
										TILEMAP_TRANSPARENT, 16,16, 0x20,0x20	);
	kaneko16_tmap_3 = tilemap_create(	get_tile_info_3, tilemap_scan_rows,
										TILEMAP_TRANSPARENT, 16,16, 0x20,0x20	);

	if (	!kaneko16_tmap_2 || !kaneko16_tmap_3	)
		return 1;
	{
		int dx, xdim = Machine->drv->screen_width;
		int dy, ydim = Machine->drv->screen_height;

		switch (xdim)
		{
			case 320:	dx = 0x33;	break;
			case 256:	dx = 0x5b;	break;
			default:	dx = 0;
		}
		switch (Machine->visible_area.max_y - Machine->visible_area.min_y + 1)
		{
			case 240- 8:	dy = +0x08;	break;
			case 240-16:	dy = -0x08;	break;
			default:		dy = 0;
		}

		tilemap_set_scrolldx( kaneko16_tmap_2, -dx,		xdim + dx -1        );
		tilemap_set_scrolldx( kaneko16_tmap_3, -(dx+2),	xdim + (dx + 2) - 1 );

		tilemap_set_scrolldy( kaneko16_tmap_2, -dy,		ydim + dy -1 );
		tilemap_set_scrolldy( kaneko16_tmap_3, -dy,		ydim + dy -1 );

		tilemap_set_transparent_pen(kaneko16_tmap_2, 0);
		tilemap_set_transparent_pen(kaneko16_tmap_3, 0);

		tilemap_set_scroll_rows(kaneko16_tmap_2, 0x200);	/* Line Scroll*/
		tilemap_set_scroll_rows(kaneko16_tmap_3, 0x200);

		return 0;
	}
}

VIDEO_START( sandscrp_1xVIEW2 )
{
	if (	video_start_kaneko16_1xVIEW2()	)
		return 1;

	tilemap_set_scrolldy( kaneko16_tmap_0, 0, 256 - 1 );
	tilemap_set_scrolldy( kaneko16_tmap_1, 0, 256 - 1 );
	return 0;
}

VIDEO_START( wingforce_1xVIEW2 )
{
	if (	video_start_kaneko16_sprites()	)
		return 1;

	kaneko16_tmap_0 = tilemap_create(	get_tile_info_0, tilemap_scan_rows,
										TILEMAP_TRANSPARENT, 16,16, 0x20,0x20	);
	kaneko16_tmap_1 = tilemap_create(	get_tile_info_1, tilemap_scan_rows,
										TILEMAP_TRANSPARENT, 16,16, 0x20,0x20	);

	kaneko16_tmap_2 = 0;

	kaneko16_tmap_3 = 0;
	
	if ((sprites_bitmap = auto_bitmap_alloc(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
		return 1;

	if (	!kaneko16_tmap_0 || !kaneko16_tmap_1	)
		return 1;

	{
		int dx, xdim = Machine->drv->screen_width;
		int dy, ydim = Machine->drv->screen_height;

		switch (xdim)
		{
			case 320:	dx = 0x33;	break;
			case 256:	dx = 0x5b;	break;
			default:	dx = 0;
		}
		switch (Machine->visible_area.max_y - Machine->visible_area.min_y + 1)
		{
			case 240- 8:	dy = +0x08;	break;	/* blazeon */
			case 240-16:	dy = +0x08+1;	break;	/* wing force */
			default:		dy = 0;
		}

		tilemap_set_scrolldx( kaneko16_tmap_0, -dx,		xdim + dx -1        );
		tilemap_set_scrolldx( kaneko16_tmap_1, -(dx+2),	xdim + (dx + 2) - 1 );

		tilemap_set_scrolldy( kaneko16_tmap_0, -dy,		ydim + dy -1 );
		tilemap_set_scrolldy( kaneko16_tmap_1, -dy,		ydim + dy -1 );

		tilemap_set_transparent_pen(kaneko16_tmap_0, 0);
		tilemap_set_transparent_pen(kaneko16_tmap_1, 0);

		tilemap_set_scroll_rows(kaneko16_tmap_0, 0x200);	/* Line Scroll*/
		tilemap_set_scroll_rows(kaneko16_tmap_1, 0x200);

		return 0;
	}
}

/* Berlwall has an additional hi-color background */

PALETTE_INIT( berlwall )
{
	int i;

	/* first 2048 colors are dynamic */

	/* initialize 555 RGB lookup */
	for (i = 0; i < 32768; i++)
	{
		int r,g,b;

		r = (i >>  5) & 0x1f;
		g = (i >> 10) & 0x1f;
		b = (i >>  0) & 0x1f;

		r = (r << 3) | (r >> 2);
		g = (g << 3) | (g >> 2);
		b = (b << 3) | (b >> 2);

		palette_set_color(2048 + i,r,g,b);
	}
}

VIDEO_START( berlwall )
{
	int sx, x,y;
	unsigned char *RAM	=	memory_region(REGION_GFX3);

	/* Render the hi-color static backgrounds held in the ROMs */

	if ((kaneko16_bg15_bitmap = auto_bitmap_alloc_depth(256 * 32, 256 * 1, 16)) == 0)
		return 1;

/*
	8aba is used as background color
	8aba/2 = 455d = 10001 01010 11101 = $11 $0a $1d
*/

	for (sx = 0 ; sx < 32 ; sx++)	/* horizontal screens*/
	 for (x = 0 ; x < 256 ; x++)	/* horizontal pixels*/
	  for (y = 0 ; y < 256 ; y++)	/* vertical pixels*/
	  {
			int addr  = sx * (256 * 256) + x + y * 256;
			int data = RAM[addr * 2 + 0] * 256 + RAM[addr * 2 + 1];
			int r,g,b;

			r = (data & 0x07c0) >>  6;
			g = (data & 0xf800) >> 11;
			b = (data & 0x003e) >>  1;

			/* apply a simple decryption */
			r ^= 0x09;

			if (~g & 0x08) g ^= 0x10;
			g = (g - 1) & 0x1f;		/* decrease with wraparound */

			b ^= 0x03;
			if (~b & 0x08) b ^= 0x10;
			b = (b + 2) & 0x1f;		/* increase with wraparound */

			/* kludge to fix the rollercoaster picture */
			if ((r & 0x10) && (b & 0x10))
				g = (g - 1) & 0x1f;		/* decrease with wraparound */

			plot_pixel( kaneko16_bg15_bitmap,
						sx * 256 + x, y,
						Machine->pens[2048 + ((g << 10) | (r << 5) | b)] );
	  }

	return video_start_kaneko16_1xVIEW2();
}


/***************************************************************************

								Sprites Drawing

	Sprite data is layed out in RAM in different ways for different games
	(type 0,1,2,etc.). This basically involves the bits in the attribute
	word to be shuffled around and/or the words being in different order.

	Each sprite is always stuffed in 4 words. There may be some extra
	padding words though (e.g. type 2 sprites are like type 0 but the
	data is held in the last 8 bytes of every 16). Examples are:

	Type 0: shogwarr, blazeon, bakubrkr.
	Type 1: gtmr.
	Type 2: berlwall

Offset:			Format:						Value:

0000.w			Attribute (type 0 & 2)

					f--- ---- ---- ----		Multisprite: Use Latched Code + 1
					-e-- ---- ---- ----		Multisprite: Use Latched Color (And Flip?)
					--d- ---- ---- ----		Multisprite: Use Latched X,Y As Offsets
					---c ba-- ---- ----
					---- --9- ---- ----		High Priority (vs FG Tiles Of High Priority)
					---- ---8 ---- ----		High Priority (vs BG Tiles Of High Priority)
					---- ---- 7654 32--		Color
					---- ---- ---- --1-		X Flip
					---- ---- ---- ---0		Y Flip

				Attribute (type 1)

					f--- ---- ---- ----		Multisprite: Use Latched Code + 1
					-e-- ---- ---- ----		Multisprite: Use Latched Color (And Flip?)
					--d- ---- ---- ----		Multisprite: Use Latched X,Y As Offsets
					---c ba-- ---- ----
					---- --9- ---- ----		X Flip
					---- ---8 ---- ----		Y Flip
					---- ---- 7--- ----		High Priority (vs FG Tiles Of High Priority)
					---- ---- -6-- ----		High Priority (vs BG Tiles Of High Priority)
					---- ---- --54 3210		Color

0002.w										Code
0004.w										X Position << 6
0006.w										Y Position << 6

	Type 3: sandscrp

Offset:			Format:				Value:

07.b			7654 ----			Color
				---- 3---
				---- -2--			Multi Sprite
				---- --1-			Y (High Bit)
				---- ---0			X (High Bit)

09.b								X (Low Bits)

0B.b								Y (Low Bits)

0D.b								Code (Low Bits)

0F.b			7--- ----			Flip X
				-6-- ----			Flip Y
				--54 3210			Code (High Bits)

***************************************************************************/

#define USE_LATCHED_XY		1
#define USE_LATCHED_CODE	2
#define USE_LATCHED_COLOR	4

int kaneko16_parse_sprite_type012(int i, struct tempsprite *s)
{
	int attr, xoffs, offs;

	if (kaneko16_sprite_type == 2)	offs = i * 16/2 + 0x8/2;
	else							offs = i * 8/2;

	if (offs >= (spriteram_size/2))	return -1;

	attr			=		spriteram16[offs + 0];
	s->code			=		spriteram16[offs + 1];
	s->x			=		spriteram16[offs + 2];
	s->y			=		spriteram16[offs + 3];

	if (kaneko16_sprite_type == 1)
	{
	s->color		=		(attr & 0x003f);
	s->priority		=		(attr & 0x00c0) >> 6;
	s->flipy		=		(attr & 0x0100);
	s->flipx		=		(attr & 0x0200);
	s->code			+=		(s->y & 1) << 16;	/* bloodwar*/
	}
	else
	{
	s->flipy		=		(attr & 0x0001);
	s->flipx		=		(attr & 0x0002);
	s->color		=		(attr & 0x00fc) >> 2;
	s->priority		=		(attr & 0x0300) >> 8;
	}
	xoffs			=		(attr & 0x1800) >> 11;
	s->yoffs		=		kaneko16_sprites_regs[0x10/2 + xoffs*2 + 1];
	s->xoffs		=		kaneko16_sprites_regs[0x10/2 + xoffs*2 + 0];

if (kaneko16_sprite_flipy)
{
	s->yoffs		-=		kaneko16_sprites_regs[0x2/2];
	s->yoffs		-=		Machine->visible_area.min_y<<6;
}
else
{
	s->yoffs		-=		kaneko16_sprites_regs[0x2/2];
	s->yoffs		+=		Machine->visible_area.min_y<<6;
}

	return 					( (attr & 0x2000) ? USE_LATCHED_XY    : 0 ) |
							( (attr & 0x4000) ? USE_LATCHED_COLOR : 0 ) |
							( (attr & 0x8000) ? USE_LATCHED_CODE  : 0 ) ;
}

int kaneko16_parse_sprite_type3(int i, struct tempsprite *s)
{
	int attr;

	int offs = i * 16/2;

	if (offs >= (spriteram_size/2))	return -1;

	attr			=		(spriteram16[offs + 0x6/2] & 0xff);
	s->x			=		(spriteram16[offs + 0x8/2] & 0xff);
	s->y			=		(spriteram16[offs + 0xa/2] & 0xff);
	s->code			=		(spriteram16[offs + 0xc/2] & 0xff) +
							(spriteram16[offs + 0xe/2] & 0xff) * 256;

	s->flipy		=		s->code & 0x4000;
	s->flipx		=		s->code & 0x8000;

	s->priority		=		3;	/* ?*/
	s->xoffs		=		0;	/* ?*/
	s->yoffs		=		0;	/* ?*/

	s->x			|=		(attr & 0x01) ? 0xff00 : 0;
	s->y			|=		(attr & 0x02) ? 0xff00 : 0;
	s->x			<<=		6;
	s->y			<<=		6;
	s->color		=		(attr & 0xf0) >> 4;

	return					(attr & 0x04) ? USE_LATCHED_XY : 0;
}

/* Build a list of sprites to display & draw them */

void kaneko16_draw_sprites(struct mame_bitmap *bitmap, const struct rectangle *cliprect, int pri)
{
	/* Sprites *must* be parsed from the first in RAM to the last,
	   because of the multisprite feature. But they *must* be drawn
	   from the last in RAM (frontmost) to the firtst in order to
	   cope with priorities using pdrawgfx.

	   Hence we parse them from first to last and put the result
	   in a temp buffer, then draw the buffer's contents from last
	   to first. */

	int max	=	(Machine->drv->screen_width > 0x100) ? (0x200<<6) : (0x100<<6);

	int i = 0;
	struct tempsprite *s = spritelist.first_sprite;

	/* These values are latched from the last sprite. */
	int x			=	0;
	int y			=	0;
	int code		=	0;
	int color		=	0;
	int priority	=	0;
	int xoffs		=	0;
	int yoffs		=	0;
	int flipx		=	0;
	int flipy		=	0;

	while (1)
	{
		int flags;

		switch( kaneko16_sprite_type )
		{
			case 0:
			case 1:
			case 2:		flags = kaneko16_parse_sprite_type012(i,s);	break;
			case 3:		flags = kaneko16_parse_sprite_type3(i,s);	break;
			default:	flags = -1;
		}

		if (flags == -1)	/* End of Sprites*/
			break;

		if (flags & USE_LATCHED_CODE)
			s->code = ++code;	/* Use the latched code + 1 ..*/
		else
			code = s->code;		/* .. or latch this value*/

		if (flags & USE_LATCHED_COLOR)
		{
			s->color		=	color;
			s->priority		=	priority;
			s->xoffs		=	xoffs;
			s->yoffs		=	yoffs;

			if (kaneko16_sprite_fliptype==0)
			{
				s->flipx		=	flipx;
				s->flipy		=	flipy;
			}
		}
		else
		{
			color		=	s->color;
			priority	=	s->priority;
			xoffs		=	s->xoffs;
			yoffs		=	s->yoffs;

			if (kaneko16_sprite_fliptype==0)
			{
				flipx = s->flipx;
				flipy = s->flipy;
			}
		}

		/* brap boys explicitly doesn't want the flip to be latched, maybe there is a different bit to enable that behavior?*/
		if (kaneko16_sprite_fliptype==1)
		{
			flipx		=	s->flipx;
			flipy		=	s->flipy;
		}

		if (flags & USE_LATCHED_XY)
		{
			s->x += x;
			s->y += y;
		}
		/* Always latch the latest result*/
		x	=	s->x;
		y	=	s->y;

		/* We can now buffer this sprite */

		s->x	=	s->xoffs + s->x;
		s->y	=	s->yoffs + s->y;

		s->x	+=	kaneko16_sprite_xoffs;
		s->y	+=	kaneko16_sprite_yoffs;

		if (kaneko16_sprite_flipx)	{ s->x = max - s->x - (16<<6);	s->flipx = !s->flipx;	}
		if (kaneko16_sprite_flipy)	{ s->y = max - s->y - (16<<6);	s->flipy = !s->flipy;	}

		s->x		=		( (s->x & 0x7fc0) - (s->x & 0x8000) ) / 0x40;
		s->y		=		( (s->y & 0x7fc0) - (s->y & 0x8000) ) / 0x40;

		i++;
		s++;
	}


	/* Let's finally draw the sprites we buffered, in reverse order
	   (for pdrawgfx) */

	for (s--; s >= spritelist.first_sprite; s--)
	{
		int curr_pri = s->priority;

		UINT32 primask = kaneko16_priority.sprite[curr_pri];

		/* You can choose which sprite priorities get displayed (for debug) */
		if ( ((1 << curr_pri) & pri) == 0 )	continue;

		pdrawgfx(	bitmap,Machine->gfx[0],
					s->code,
					s->color,
					s->flipx, s->flipy,
					s->x, s->y,
					cliprect,TRANSPARENCY_PEN,0,
					primask );
#ifdef MAME_DEBUG
#if 0
if (keyboard_pressed(KEYCODE_Z))
{	/* Display some info on each sprite */
	struct DisplayText dt[2];	char buf[10];
	sprintf(buf, "%X",s->priority);
	dt[0].text = buf;	dt[0].color = UI_COLOR_NORMAL;
	dt[0].x = s->x;		dt[0].y = s->y;
	dt[1].text = 0;	/* terminate array */
	displaytext(Machine->scrbitmap,dt);		}
#endif
#endif
	}
}



/***************************************************************************


							Sprites Registers

	Offset:			Format:						Value:

	0000.w			f--- ---- ---- ----			Sprites Disable?? (see blazeon)
					-edc ba98 7654 32--
					---- ---- ---- --1-			Flip X
					---- ---- ---- ---0			Flip Y

	0002.w										Y Offset << 6 (Global)


	0004..000e.w								?


	0010.w										X Offset << 6 #0
	0012.w										Y Offset << 6 #0

	0014.w										X Offset << 6 #1
	0016.w										Y Offset << 6 #1

	0018.w										X Offset << 6 #2
	001a.w										Y Offset << 6 #2

	001c.w										X Offset << 6 #3
	001e.w										Y Offset << 6 #3

***************************************************************************/

/*
[gtmr]

Initial self test:
600000: 4BC0 94C0 4C40 94C0-0404 0002 0000 0000		(Layers 1 regs)
680000: 4BC0 94C0 4C40 94C0-1C1C 0002 0000 0000		(Layers 2 regs)
Race start:
600000: DC00 7D00 DC80 7D00-0404 0002 0000 0000		(Layers 1 regs)
680000: DC00 7D00 DC80 7D00-1C1C 0002 0000 0000		(Layers 2 regs)

[gtmr]
700000: 0040 0000 0001 0180-0000 0000 0000 0000		(Sprites  regs)
700010: 0040 0000 0040 0000-0040 0000 2840 1E00		; 1,0 .. a1,78
													; a0*2=screenx/2
													; 78*2=screeny/2
FLIP ON:
700000: 0043 FFC0 0001 0180-0000 0000 0000 0000		(Sprites  regs)
700010: 2FC0 4400 2FC0 4400-2FC0 4400 57C0 6200		; bf,110 .. 15f,188
													; 15f-bf=a0! 188-110=78!

[berlwall]
600000: 48CC 03C0 0001 0100-0000 0000 0000 0000		(Sprites  regs)
600010: 0000 0000 0000 0000-0000 0000 0000 0000
FLIP ON:
600000: 48CF FC00 0001 0100-0000 0000 0000 0000		(Sprites  regs)
600010: 0000 0000 0000 0000-0000 0000 0000 0000

[mgcrystl]
900000: 4FCC 0000 0040 00C0-xxxx 0001 0001 0001		(Sprites  regs)
900010: 0000 FC40 A000 9C40-1E00 1A40 0000 FC40
FLIP ON:
900000: 4FCF 0000 0040 00C0-xxxx 0001 0001 0001		(Sprites  regs)
900010: 0000 0400 A000 A400-1E00 2200 0000 0400		; +1f<<6 on y
*/

READ16_HANDLER( kaneko16_sprites_regs_r )
{
	return kaneko16_sprites_regs[offset];
}

WRITE16_HANDLER( kaneko16_sprites_regs_w )
{
	data16_t new_data;

	COMBINE_DATA(&kaneko16_sprites_regs[offset]);
	new_data  = kaneko16_sprites_regs[offset];

	switch (offset)
	{
		case 0:
			if (ACCESSING_LSB)
			{
				kaneko16_sprite_flipx = new_data & 2;
				kaneko16_sprite_flipy = new_data & 1;

				if(kaneko16_sprite_type == 0 || kaneko16_sprite_type == 2)
					kaneko16_keep_sprites = ~new_data & 4;
			}

			break;
	}

/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #0 PC %06X : Warning, sprites reg %04X <- %04X\n",activecpu_get_pc(),offset*2,data);*/
}


/***************************************************************************

							Layers Registers


	Offset:			Format:						Value:

	0000.w										FG Scroll X
	0002.w										FG Scroll Y

	0004.w										BG Scroll X
	0006.w										BG Scroll Y

	0008.w			Layers Control

					fed- ---- ---- ----
					---c ---- ---- ----		BG Disable
					---- b--- ---- ----		Line Scroll (Always 1 in berlwall & bakubrkr)
					---- -a-- ---- ----		? Always 1 in gtmr     & bakubrkr ?
					---- --9- ---- ----		BG Flip X
					---- ---8 ---- ----		BG Flip Y

					---- ---- 765- ----
					---- ---- ---4 ----		FG Disable
					---- ---- ---- 3---		Line Scroll (Always 1 in berlwall & bakubrkr)
					---- ---- ---- -2--		? Always 1 in gtmr     & bakubrkr ?
					---- ---- ---- --1-		FG Flip X
					---- ---- ---- ---0		FG Flip Y

	000a.w										? always 0x0002 ?

There are more!

***************************************************************************/

/*	[gtmr]

	car select screen scroll values:
	Flipscreen off:
		$6x0000: $72c0 ; $fbc0 ; 7340 ; 0
		$72c0/$40 = $1cb = $200-$35	/	$7340/$40 = $1cd = $1cb+2

		$fbc0/$40 = -$11

	Flipscreen on:
		$6x0000: $5d00 ; $3780 ; $5c80 ; $3bc0
		$5d00/$40 = $174 = $200-$8c	/	$5c80/$40 = $172 = $174-2

		$3780/$40 = $de	/	$3bc0/$40 = $ef

*/

WRITE16_HANDLER( kaneko16_layers_0_regs_w )
{
	COMBINE_DATA(&kaneko16_layers_0_regs[offset]);
}

WRITE16_HANDLER( kaneko16_layers_1_regs_w )
{
	COMBINE_DATA(&kaneko16_layers_1_regs[offset]);
}




/* Select the high color background image (out of 32 in the ROMs) */
READ16_HANDLER( kaneko16_bg15_select_r )
{
	return kaneko16_bg15_select[0];
}
WRITE16_HANDLER( kaneko16_bg15_select_w )
{
	COMBINE_DATA(&kaneko16_bg15_select[0]);
}

/* ? */
READ16_HANDLER( kaneko16_bg15_reg_r )
{
	return kaneko16_bg15_reg[0];
}
WRITE16_HANDLER( kaneko16_bg15_reg_w )
{
	COMBINE_DATA(&kaneko16_bg15_reg[0]);
}




/***************************************************************************


								Screen Drawing


***************************************************************************/

VIDEO_UPDATE( kaneko16 )
{
	int layers_flip_0, layers_flip_1 = 0;
	int layers_ctrl = -1;
	int i,flag;

	data16_t layer0_scrollx, layer0_scrolly;
	data16_t layer1_scrollx, layer1_scrolly;

	layers_flip_0 = kaneko16_layers_0_regs[ 4 ];
	if (kaneko16_tmap_2)
	{
	layers_flip_1 = kaneko16_layers_1_regs[ 4 ];
	}

	/* Enable layers */
	tilemap_set_enable(kaneko16_tmap_0, ~layers_flip_0 & 0x1000);
	tilemap_set_enable(kaneko16_tmap_1, ~layers_flip_0 & 0x0010);
	if (kaneko16_tmap_2)
	{
	tilemap_set_enable(kaneko16_tmap_2, ~layers_flip_1 & 0x1000);
	tilemap_set_enable(kaneko16_tmap_3, ~layers_flip_1 & 0x0010);
	}

	/* Flip layers */
	tilemap_set_flip(kaneko16_tmap_0,	((layers_flip_0 & 0x0100) ? TILEMAP_FLIPY : 0) |
								 		((layers_flip_0 & 0x0200) ? TILEMAP_FLIPX : 0) );
	tilemap_set_flip(kaneko16_tmap_1,	((layers_flip_0 & 0x0100) ? TILEMAP_FLIPY : 0) |
								 		((layers_flip_0 & 0x0200) ? TILEMAP_FLIPX : 0) );
	if (kaneko16_tmap_2)
	{
	tilemap_set_flip(kaneko16_tmap_2,	((layers_flip_1 & 0x0100) ? TILEMAP_FLIPY : 0) |
								 		((layers_flip_1 & 0x0200) ? TILEMAP_FLIPX : 0) );
	tilemap_set_flip(kaneko16_tmap_3,	((layers_flip_1 & 0x0100) ? TILEMAP_FLIPY : 0) |
								 		((layers_flip_1 & 0x0200) ? TILEMAP_FLIPX : 0) );
	}

	/* Scroll layers */
	layer0_scrollx		=	kaneko16_layers_0_regs[ 2 ];
	layer0_scrolly		=	kaneko16_layers_0_regs[ 3 ] >> 6;
	layer1_scrollx		=	kaneko16_layers_0_regs[ 0 ];
	layer1_scrolly		=	kaneko16_layers_0_regs[ 1 ] >> 6;

	tilemap_set_scrolly(kaneko16_tmap_0,0,layer0_scrolly);
	tilemap_set_scrolly(kaneko16_tmap_1,0,layer1_scrolly);

	for (i=0; i<0x200; i++)
	{
		data16_t scroll;
		scroll = (layers_flip_0 & 0x0800) ? kaneko16_vscroll_0[i] : 0;
		tilemap_set_scrollx(kaneko16_tmap_0,i,(layer0_scrollx + scroll) >> 6 );
		scroll = (layers_flip_0 & 0x0008) ? kaneko16_vscroll_1[i] : 0;
		tilemap_set_scrollx(kaneko16_tmap_1,i,(layer1_scrollx + scroll) >> 6 );
	}

	if (kaneko16_tmap_2)
	{
	layer0_scrollx		=	kaneko16_layers_1_regs[ 2 ];
	layer0_scrolly		=	kaneko16_layers_1_regs[ 3 ] >> 6;
	layer1_scrollx		=	kaneko16_layers_1_regs[ 0 ];
	layer1_scrolly		=	kaneko16_layers_1_regs[ 1 ] >> 6;

	tilemap_set_scrolly(kaneko16_tmap_2,0,layer0_scrolly);
	tilemap_set_scrolly(kaneko16_tmap_3,0,layer1_scrolly);

	for (i=0; i<0x200; i++)
	{
		data16_t scroll;
		scroll = (layers_flip_1 & 0x0800) ? kaneko16_vscroll_2[i] : 0;
		tilemap_set_scrollx(kaneko16_tmap_2,i,(layer0_scrollx + scroll) >> 6 );
		scroll = (layers_flip_1 & 0x0008) ? kaneko16_vscroll_3[i] : 0;
		tilemap_set_scrollx(kaneko16_tmap_3,i,(layer1_scrollx + scroll) >> 6 );
	}
	}

#ifdef MAME_DEBUG
if ( keyboard_pressed(KEYCODE_Z) ||
	 keyboard_pressed(KEYCODE_X) || keyboard_pressed(KEYCODE_C) ||
     keyboard_pressed(KEYCODE_V) || keyboard_pressed(KEYCODE_B) )
{	int msk = 0, val = 0;

	if (keyboard_pressed(KEYCODE_X))	val = 1;	/* priority 0 only*/
	if (keyboard_pressed(KEYCODE_C))	val = 2;	/* ""       1*/
	if (keyboard_pressed(KEYCODE_V))	val = 4;	/* ""       2*/
	if (keyboard_pressed(KEYCODE_B))	val = 8;	/* ""       3*/

	if (keyboard_pressed(KEYCODE_Z))	val = 1|2|4|8;	/* All of the above priorities*/

	if (keyboard_pressed(KEYCODE_Q))	msk |= val << 0;	/* for tmap 0*/
	if (keyboard_pressed(KEYCODE_W))	msk |= val << 4;	/* ""       1*/
	if (keyboard_pressed(KEYCODE_E))	msk |= val << 8;	/* ""       2*/
	if (keyboard_pressed(KEYCODE_R))	msk |= val << 12;	/* ""       3*/
	if (keyboard_pressed(KEYCODE_A))	msk |= val << 16;	/* for sprites*/
	if (msk != 0) layers_ctrl &= msk;

#if 0
	usrintf_showmessage(
		"%04X %04X %04X %04X %04X %04X %04X %04X - %04X %04X %04X %04X %04X %04X %04X %04X",
		kaneko16_layers_0_regs[0x0],kaneko16_layers_0_regs[0x1],
		kaneko16_layers_0_regs[0x2],kaneko16_layers_0_regs[0x3],
		kaneko16_layers_0_regs[0x4],kaneko16_layers_0_regs[0x5],
		kaneko16_layers_0_regs[0x6],kaneko16_layers_0_regs[0x7],

		kaneko16_layers_0_regs[0x8],kaneko16_layers_0_regs[0x9],
		kaneko16_layers_0_regs[0xa],kaneko16_layers_0_regs[0xb],
		kaneko16_layers_0_regs[0xc],kaneko16_layers_0_regs[0xd],
		kaneko16_layers_0_regs[0xe],kaneko16_layers_0_regs[0xf]	);
#endif
}
#endif

	flag = TILEMAP_IGNORE_TRANSPARENCY;

	/* Draw the high colour bg layer first, if any */

	if (kaneko16_bg15_bitmap)
	{
		int select	=	kaneko16_bg15_select[ 0 ];
/*		int reg		=	kaneko16_bg15_reg[ 0 ];*/
		int flip	=	select & 0x20;
		int sx, sy;

		if (flip)	select ^= 0x1f;

		sx		=	(select & 0x1f) * 256;
		sy		=	0;

		copybitmap(
			bitmap, kaneko16_bg15_bitmap,
			flip, flip,
			-sx, -sy,
			cliprect, TRANSPARENCY_NONE,0 );

		flag = 0;
	}

	/* Fill the bitmap with pen 0. This is wrong, but will work most of
	   the times. To do it right, each pixel should be drawn with pen 0
	   of the bottomost tile that covers it (which is pretty tricky to do) */

	if (flag!=0)	fillbitmap(bitmap,Machine->pens[0],cliprect);

	fillbitmap(priority_bitmap,0,cliprect);

	if (kaneko16_tmap_2)
	{
	/*
		The only working game using a 2nd VIEW2 chip is mgcrystl, where
		its tilemaps seem to always be below every sprite. Hence we can
		draw them with priority 0. To treat more complex cases, however,
		we need tilemap.c to handle 8 "layers" (4 priorities x 2 chips)
	*/
		for ( i = 0; i < 4; i++ )	if (layers_ctrl&(1<<(i+ 8)))	tilemap_draw(bitmap,cliprect, kaneko16_tmap_2, i, 0);
		for ( i = 0; i < 4; i++ )	if (layers_ctrl&(1<<(i+12)))	tilemap_draw(bitmap,cliprect, kaneko16_tmap_3, i, 0);
	}

	for ( i = 0; i < 4; i++ )
	{
		int tile = kaneko16_priority.tile[i];
		if (layers_ctrl&(1<<(tile+0)))	tilemap_draw(bitmap,cliprect, kaneko16_tmap_0, tile, 1<<i );
		if (layers_ctrl&(1<<(tile+4)))	tilemap_draw(bitmap,cliprect, kaneko16_tmap_1, tile, 1<<i );
	}

	/* Sprites last (rendered with pdrawgfx, so they can slip
	   in between the layers) */

	if (layers_ctrl & (0xf<<16))
	{
		if(kaneko16_keep_sprites)
		{
			/* keep sprites on screen */
			kaneko16_draw_sprites(sprites_bitmap,cliprect, (layers_ctrl >> 16) & 0xf);
			copybitmap(bitmap,sprites_bitmap,0,0,0,0,cliprect,TRANSPARENCY_PEN,0);
		}
		else
		{
			fillbitmap(sprites_bitmap,Machine->pens[0],cliprect);
			kaneko16_draw_sprites(bitmap,cliprect, (layers_ctrl >> 16) & 0xf);
		}
	}
}
