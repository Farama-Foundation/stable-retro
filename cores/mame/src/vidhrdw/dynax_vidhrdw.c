/***************************************************************************

						-= Dynax / Nakanihon Games =-

					driver by	Luca Elia (l.elia@tin.it)


	hanamai:
	There are four scrolling layers. Each layer consists of 2 frame buffers.
	The 2 images are interleaved to form the final picture sent to the screen.

	drgpunch:
	There are three scrolling layers. Each layer consists of 2 frame buffers.
	The 2 images are interleaved to form the final picture sent to the screen.

	mjdialq2:
	Two scrolling layers.

	The gfx roms do not contain tiles: the CPU controls a video blitter
	that can read data from them (instructions to draw pixel by pixel,
	in a compressed form) and write to up the 8 frame buffers.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "includes/dynax.h"

/* 0 B01234 G01234 R01234 */
PALETTE_INIT( sprtmtch )
{
	int i;

	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int x =	(color_prom[i]<<8) + color_prom[0x200+i];
		/* The bits are in reverse order! */
		int r = BITSWAP8((x >>  0) & 0x1f, 7,6,5, 0,1,2,3,4 );
		int g = BITSWAP8((x >>  5) & 0x1f, 7,6,5, 0,1,2,3,4 );
		int b = BITSWAP8((x >> 10) & 0x1f, 7,6,5, 0,1,2,3,4 );
		r =  (r << 3) | (r >> 2);
		g =  (g << 3) | (g >> 2);
		b =  (b << 3) | (b >> 2);
		palette_set_color(i,r,g,b);
	}
}

/***************************************************************************


								Video Blitter(s)


***************************************************************************/


static int dynax_blit_scroll_x, dynax_blit_scroll_y, dynax_blit_scroll_high;

static int blit_dest;
static int blit_src;
static int dynax_blit_dest;
static int hanamai_layer_half;
static int hnoridur_layer_half2;

static int dynax_blit_pen;
static int dynax_blit_backpen;
static int dynax_blit_palettes;
static int dynax_blit_palbank;

static int dynax_layer_enable;

static int extra_scroll_x,extra_scroll_y;
static int flipscreen;

#define LAYOUT_HANAMAI	0	/* 4 layers, interleaved*/
#define LAYOUT_HNORIDUR	1	/* same as hanamai but some bits are inverted and layer order is reversed*/
#define LAYOUT_DRGPUNCH	2	/* 3 couples of layers, interleaved*/
#define LAYOUT_MJDIALQ2	3	/* 2 layers*/

static int layer_layout;

static int trigger_irq;	/* some games trigger IRQ at blitter end, some don't*/

/* 4 layers, 2 images per layer (interleaved on screen)*/
static UINT8 *dynax_pixmap[4][2];



WRITE_HANDLER( dynax_extra_scrollx_w )
{
	extra_scroll_x = data;
}

WRITE_HANDLER( dynax_extra_scrolly_w )
{
	extra_scroll_y = data;
}


/* Destination Pen */
WRITE_HANDLER( dynax_blit_pen_w )
{
	dynax_blit_pen = data;
	log_cb(RETRO_LOG_DEBUG, LOGPRE "P=%02X ",data);
}

/* Background Color */
WRITE_HANDLER( dynax_blit_backpen_w )
{
	dynax_blit_backpen = data;
	log_cb(RETRO_LOG_DEBUG, LOGPRE "B=%02X ",data);
}

/* Layers 0&1 Palettes (Low Bits) */
WRITE_HANDLER( dynax_blit_palette01_w )
{
	if (layer_layout == LAYOUT_HNORIDUR)
		dynax_blit_palettes = (dynax_blit_palettes & 0x00ff) | ((data&0x0f)<<12) | ((data&0xf0)<<4);
	else
		dynax_blit_palettes = (dynax_blit_palettes & 0xff00) | data;
	log_cb(RETRO_LOG_DEBUG, LOGPRE "P1=%02X ",data);
}

/* Layer 2 Palette (Low Bits) */
WRITE_HANDLER( dynax_blit_palette2_w )
{
	if (layer_layout == LAYOUT_HNORIDUR)
		dynax_blit_palettes = (dynax_blit_palettes & 0xff00) | ((data&0x0f)<<4) | ((data&0xf0)>>4);
	else
		dynax_blit_palettes = (dynax_blit_palettes & 0x00ff) | (data<<8);
	log_cb(RETRO_LOG_DEBUG, LOGPRE "P2=%02X ",data);
}

/* Layers Palettes (High Bits) */
WRITE_HANDLER( dynax_blit_palbank_w )
{
	dynax_blit_palbank = data;
	log_cb(RETRO_LOG_DEBUG, LOGPRE "PB=%02X ",data);
}

/* Destination Layers */
WRITE_HANDLER( dynax_blit_dest_w )
{
	dynax_blit_dest = data;
	if (layer_layout == LAYOUT_HNORIDUR)
		dynax_blit_dest = BITSWAP8(dynax_blit_dest ^ 0x0f, 7,6,5,4, 0,1,2,3);

	log_cb(RETRO_LOG_DEBUG, LOGPRE "D=%02X ",data);
}

/* Which half of the layers to write two (interleaved games only) */
WRITE_HANDLER( hanamai_layer_half_w )
{
	hanamai_layer_half = data & 1;
	log_cb(RETRO_LOG_DEBUG, LOGPRE "H=%02X ",data);
}

/* Write to both halves of the layers (interleaved games only) */
WRITE_HANDLER( hnoridur_layer_half2_w )
{
	hnoridur_layer_half2 = (~data) & 1;
	log_cb(RETRO_LOG_DEBUG, LOGPRE "H2=%02X ",data);
}

WRITE_HANDLER( mjdialq2_blit_dest_w )
{
	int mask = (2 >> offset);	/* 1 or 2 */

	dynax_blit_dest &= ~mask;
	if (~data & 1) dynax_blit_dest |= mask;
}

/* Layers Enable */
WRITE_HANDLER( dynax_layer_enable_w )
{
	dynax_layer_enable = data;

	log_cb(RETRO_LOG_DEBUG, LOGPRE "E=%02X ",data);
}

WRITE_HANDLER( mjdialq2_layer_enable_w )
{
	int mask = (2 >> offset);	/* 1 or 2 */

	dynax_layer_enable &= ~mask;
	if (~data & 1) dynax_layer_enable |= mask;
}


WRITE_HANDLER( dynax_flipscreen_w )
{
	flipscreen = data & 1;
	if (data & ~1)
		log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU#0 PC %06X: Warning, flip screen <- %02X\n", activecpu_get_pc(), data);
	log_cb(RETRO_LOG_DEBUG, LOGPRE "F=%02X ",data);
}


/***************************************************************************

							Blitter Data Format

	The blitter reads its commands from the gfx ROMs. They are
	instructions to draw an image pixel by pixel (in a compressed
	form) in a frame buffer.

	Fetch 1 Byte from the ROM:

	7654 ----	Pen to draw with
	---- 3210	Command

	Other bytes may follow, depending on the command

	Commands:

	0		Stop.
	1-b		Draw 1-b pixels along X.
	c		Followed by 1 byte (N): draw N pixels along X.
	d		Followed by 2 bytes (X,N): skip X pixels, draw N pixels along X.
	e		? unused
	f		Increment Y

***************************************************************************/


/* Plot a pixel (in the pixmaps specified by dynax_blit_dest) */
static INLINE void sprtmtch_plot_pixel(int x, int y, int pen, int flags)
{
	x &= 0xff;	/* confirmed by some mjdialq2 gfx and especially by mjfriday, which*/
				/* uses the front layer to mask out the right side of the screen as*/
				/* it draws stuff on the left, when it shows the girls scrolling*/
				/* horizontally after you win.*/
	y &= 0xff;	/* seems confirmed by mjdialq2 last picture of gal 6, but it breaks*/
				/* mjdialq2 title screen so there's something we are missing.*/

	/* "Flip Screen" just means complement the coordinates to 255 */
	if (flipscreen)	{	x ^= 0xff;	y ^= 0xff;	}

	/* Rotate: rotation = SWAPXY + FLIPY */
	if (flags & 0x08)	{ int t = x; x = y; y = t;	}

	/* Ignore the pens specified in ROM, draw everything with the
	   supplied one instead */
	if (flags & 0x02)	{ pen = (dynax_blit_pen >> 4) & 0xf;	}

	if (dynax_blit_dest & 0x10)	pen |= dynax_blit_pen<<1;	/* e.g. yarunara*/

	if (	(x >= 0) && (x <= 0xff) &&
			(y >= 0) && (y <= 0xff)	)
	{
		switch (layer_layout)
		{
			case LAYOUT_HANAMAI:
				if (dynax_blit_dest & 0x01)	dynax_pixmap[0][hanamai_layer_half][256*y+x] = pen;
				if (dynax_blit_dest & 0x02)	dynax_pixmap[1][hanamai_layer_half][256*y+x] = pen;
				if (dynax_blit_dest & 0x04)	dynax_pixmap[2][hanamai_layer_half][256*y+x] = pen;
				if (dynax_blit_dest & 0x08)	dynax_pixmap[3][hanamai_layer_half][256*y+x] = pen;
				break;

			case LAYOUT_HNORIDUR:
				if (dynax_blit_dest & 0x01)	dynax_pixmap[0][hanamai_layer_half][256*y+x] = pen;
				if (dynax_blit_dest & 0x02)	dynax_pixmap[1][hanamai_layer_half][256*y+x] = pen;
				if (dynax_blit_dest & 0x04)	dynax_pixmap[2][hanamai_layer_half][256*y+x] = pen;
				if (dynax_blit_dest & 0x08)	dynax_pixmap[3][hanamai_layer_half][256*y+x] = pen;
				if (!hnoridur_layer_half2) break;
				if (dynax_blit_dest & 0x01)	dynax_pixmap[0][1-hanamai_layer_half][256*y+x] = pen;
				if (dynax_blit_dest & 0x02)	dynax_pixmap[1][1-hanamai_layer_half][256*y+x] = pen;
				if (dynax_blit_dest & 0x04)	dynax_pixmap[2][1-hanamai_layer_half][256*y+x] = pen;
				if (dynax_blit_dest & 0x08)	dynax_pixmap[3][1-hanamai_layer_half][256*y+x] = pen;
				break;

			case LAYOUT_DRGPUNCH:
				if (dynax_blit_dest & 0x01)	dynax_pixmap[0][0][256*y+x] = pen;
				if (dynax_blit_dest & 0x02)	dynax_pixmap[0][1][256*y+x] = pen;
				if (dynax_blit_dest & 0x04)	dynax_pixmap[1][0][256*y+x] = pen;
				if (dynax_blit_dest & 0x08)	dynax_pixmap[1][1][256*y+x] = pen;
				if (dynax_blit_dest & 0x10)	dynax_pixmap[2][0][256*y+x] = pen;
				if (dynax_blit_dest & 0x20)	dynax_pixmap[2][1][256*y+x] = pen;
				break;

			case LAYOUT_MJDIALQ2:
				if (dynax_blit_dest & 0x01)	dynax_pixmap[0][0][256*y+x] = pen;
				if (dynax_blit_dest & 0x02)	dynax_pixmap[1][0][256*y+x] = pen;
				break;
		}
	}
}


int sprtmtch_drawgfx( int i, int dest, int flags )
{
	data8_t cmd, pen;
	int x = dest & 0xff;
	int y = dest >> 8;

	data8_t *SRC		=	memory_region( REGION_GFX1 );
	size_t   size_src	=	memory_region_length( REGION_GFX1 );

	int sx;

if (flags & 0xf4) usrintf_showmessage("flags %02x",flags);
	if ( flags & 1 )
	{
		int start,len;
		pen = (dynax_blit_pen >> 4) & 0xf;

		/* Clear the buffer(s) starting from the given scanline and exit */

		if (flipscreen)
			start = 0;
		else
			start = dest;

		len = 0x10000 - dest;

		switch (layer_layout)
		{
			case LAYOUT_HANAMAI:
				if (dynax_blit_dest & 0x01)	memset(&dynax_pixmap[0][0][start],pen,len);
				if (dynax_blit_dest & 0x01)	memset(&dynax_pixmap[0][1][start],pen,len);
				if (dynax_blit_dest & 0x02)	memset(&dynax_pixmap[1][0][start],pen,len);
				if (dynax_blit_dest & 0x02)	memset(&dynax_pixmap[1][1][start],pen,len);
				if (dynax_blit_dest & 0x04)	memset(&dynax_pixmap[2][0][start],pen,len);
				if (dynax_blit_dest & 0x04)	memset(&dynax_pixmap[2][1][start],pen,len);
				if (dynax_blit_dest & 0x08)	memset(&dynax_pixmap[3][0][start],pen,len);
				if (dynax_blit_dest & 0x08)	memset(&dynax_pixmap[3][1][start],pen,len);
				break;

			case LAYOUT_HNORIDUR:
				if (dynax_blit_dest & 0x01)	memset(&dynax_pixmap[0][hanamai_layer_half][start],pen,len);
				if (dynax_blit_dest & 0x02)	memset(&dynax_pixmap[1][hanamai_layer_half][start],pen,len);
				if (dynax_blit_dest & 0x04)	memset(&dynax_pixmap[2][hanamai_layer_half][start],pen,len);
				if (dynax_blit_dest & 0x08)	memset(&dynax_pixmap[3][hanamai_layer_half][start],pen,len);
				if (!hnoridur_layer_half2) break;
				if (dynax_blit_dest & 0x01)	memset(&dynax_pixmap[0][1-hanamai_layer_half][start],pen,len);
				if (dynax_blit_dest & 0x02)	memset(&dynax_pixmap[1][1-hanamai_layer_half][start],pen,len);
				if (dynax_blit_dest & 0x04)	memset(&dynax_pixmap[2][1-hanamai_layer_half][start],pen,len);
				if (dynax_blit_dest & 0x08)	memset(&dynax_pixmap[3][1-hanamai_layer_half][start],pen,len);
				break;

			case LAYOUT_DRGPUNCH:
				if (dynax_blit_dest & 0x01)	memset(&dynax_pixmap[0][0][start],pen,len);
				if (dynax_blit_dest & 0x02)	memset(&dynax_pixmap[0][1][start],pen,len);
				if (dynax_blit_dest & 0x04)	memset(&dynax_pixmap[1][0][start],pen,len);
				if (dynax_blit_dest & 0x08)	memset(&dynax_pixmap[1][1][start],pen,len);
				if (dynax_blit_dest & 0x10)	memset(&dynax_pixmap[2][0][start],pen,len);
				if (dynax_blit_dest & 0x20)	memset(&dynax_pixmap[2][1][start],pen,len);
				break;

			case LAYOUT_MJDIALQ2:
				if (dynax_blit_dest & 0x01)	memset(&dynax_pixmap[0][0][start],pen,len);
				if (dynax_blit_dest & 0x02)	memset(&dynax_pixmap[1][0][start],pen,len);
				break;
		}

		return i;
	}

	sx = x;

	for ( ;; )
	{
		if (i >= size_src)
		{
usrintf_showmessage("GFXROM OVER %08x",i);
			return i;
		}
		cmd = SRC[i++];
		pen = (cmd & 0xf0)>>4;
		cmd = (cmd & 0x0f)>>0;

		switch (cmd)
		{
		case 0xf:	/* Increment Y*/
			/* Rotate: rotation = SWAPXY + FLIPY */
			if (flags & 0x08)
				y--;
			else
				y++;
			x = sx;
			break;

		case 0xe:	/* unused ? was "change dest mask" in the "rev1" blitter*/
			usrintf_showmessage("Blitter unknown command %06X: %02X\n", i-1, cmd);

		case 0xd:	/* Skip X pixels*/
			if (i >= size_src)
			{
usrintf_showmessage("GFXROM OVER %08x",i);
				return i;
			}
			x = sx + SRC[i++];
			/* fall through into next case */

		case 0xc:	/* Draw N pixels*/
			if (i >= size_src)
			{
usrintf_showmessage("GFXROM OVER %08x",i);
				return i;
			}
			cmd = SRC[i++];
			/* fall through into next case */

		case 0xb:
		case 0xa:
		case 0x9:
		case 0x8:
		case 0x7:
		case 0x6:
		case 0x5:
		case 0x4:
		case 0x3:
		case 0x2:
		case 0x1:	/* Draw N pixels*/
			while (cmd--)
				sprtmtch_plot_pixel(x++, y, pen, flags);
			break;

		case 0x0:	/* Stop*/
			return i;
		}
	}
}



static void dynax_blitter_start(int flags)
{
	int i =
	sprtmtch_drawgfx(
		blit_src & 0x3fffff,
		blit_dest,
		flags
	);

	log_cb(RETRO_LOG_DEBUG, LOGPRE "SRC=%X BLIT=%02X\n",blit_src,flags);

	blit_src	=	(blit_src	&	~0x3fffff) |
					(i			&	 0x3fffff) ;

	/* Generate an IRQ */
	if (trigger_irq)
	{
		dynax_blitter_irq = 1;
		sprtmtch_update_irq();
	}
}




WRITE_HANDLER( dynax_blit_scroll_w )
{
/*logerror("%04x blit_scroll_w data = %02x addr = %06x\n",activecpu_get_pc(),data,blit_src);*/
	/* 0x800000 also used!*/
	if (blit_src & 0x800000)
	{
    dynax_blit_scroll_high = data;	/* ?*/
    log_cb(RETRO_LOG_DEBUG, LOGPRE "SH=%02X ",data);
	}
	else
	{
		if (blit_src & 0x400000)
		{
			dynax_blit_scroll_y = data;
			log_cb(RETRO_LOG_DEBUG, LOGPRE "SY=%02X ",data);

		}
		else
		{
			dynax_blit_scroll_x = data;
      log_cb(RETRO_LOG_DEBUG, LOGPRE "SX=%02X ",data);
		}
	}
}


WRITE_HANDLER( dynax_blitter_rev2_w )
{
	switch (offset)
	{
		case 0: dynax_blitter_start(data); break;
		case 1:	blit_dest = (blit_dest & 0xff00) | (data << 0); break;
		case 2: blit_dest = (blit_dest & 0x00ff) | (data << 8); break;
		case 3:	blit_src = (blit_src & 0xffff00) | (data << 0); break;
		case 4: blit_src = (blit_src & 0xff00ff) | (data << 8); break;
		case 5: blit_src = (blit_src & 0x00ffff) | (data <<16); break;
		case 6: dynax_blit_scroll_w(0,data); break;
	}
}


/***************************************************************************


								Video Init


***************************************************************************/

int *priority_table;
/*                           0       1       2       3       4       5       6       7*/
int priority_hnoridur[8] = { 0x0231, 0x2103, 0x3102, 0x2031, 0x3021, 0x1302, 0x2310, 0x1023 };
int priority_mcnpshnt[8] = { 0x3210, 0x2103, 0x3102, 0x2031, 0x3021, 0x1302, 0x2310, 0x1023 };

static void Video_Reset(void)
{
	extra_scroll_x = 0;
	extra_scroll_y = 0;

	hnoridur_layer_half2 = 0;

	trigger_irq = 1;
}

VIDEO_START( hanamai )
{
	if (!(dynax_pixmap[0][0] = auto_malloc(256*256)))	return 1;
	if (!(dynax_pixmap[0][1] = auto_malloc(256*256)))	return 1;
	if (!(dynax_pixmap[1][0] = auto_malloc(256*256)))	return 1;
	if (!(dynax_pixmap[1][1] = auto_malloc(256*256)))	return 1;
	if (!(dynax_pixmap[2][0] = auto_malloc(256*256)))	return 1;
	if (!(dynax_pixmap[2][1] = auto_malloc(256*256)))	return 1;
	if (!(dynax_pixmap[3][0] = auto_malloc(256*256)))	return 1;
	if (!(dynax_pixmap[3][1] = auto_malloc(256*256)))	return 1;

	Video_Reset();
	layer_layout = LAYOUT_HANAMAI;

	return 0;
}

VIDEO_START( hnoridur )
{
	if (!(dynax_pixmap[0][0] = auto_malloc(256*256)))	return 1;
	if (!(dynax_pixmap[0][1] = auto_malloc(256*256)))	return 1;
	if (!(dynax_pixmap[1][0] = auto_malloc(256*256)))	return 1;
	if (!(dynax_pixmap[1][1] = auto_malloc(256*256)))	return 1;
	if (!(dynax_pixmap[2][0] = auto_malloc(256*256)))	return 1;
	if (!(dynax_pixmap[2][1] = auto_malloc(256*256)))	return 1;
	if (!(dynax_pixmap[3][0] = auto_malloc(256*256)))	return 1;
	if (!(dynax_pixmap[3][1] = auto_malloc(256*256)))	return 1;

	Video_Reset();
	layer_layout = LAYOUT_HNORIDUR;

	priority_table = priority_hnoridur;

	return 0;
}

VIDEO_START( mcnpshnt )
{
	if (video_start_hnoridur())	return 1;
	priority_table = priority_mcnpshnt;
	return 0;
}

VIDEO_START( sprtmtch )
{
	if (!(dynax_pixmap[0][0] = auto_malloc(256*256)))	return 1;
	if (!(dynax_pixmap[0][1] = auto_malloc(256*256)))	return 1;
	if (!(dynax_pixmap[1][0] = auto_malloc(256*256)))	return 1;
	if (!(dynax_pixmap[1][1] = auto_malloc(256*256)))	return 1;
	if (!(dynax_pixmap[2][0] = auto_malloc(256*256)))	return 1;
	if (!(dynax_pixmap[2][1] = auto_malloc(256*256)))	return 1;

	Video_Reset();
	layer_layout = LAYOUT_DRGPUNCH;

	return 0;
}

VIDEO_START( mjdialq2 )
{
	if (!(dynax_pixmap[0][0] = auto_malloc(256*256)))	return 1;
	if (!(dynax_pixmap[1][0] = auto_malloc(256*256)))	return 1;

	Video_Reset();
	layer_layout = LAYOUT_MJDIALQ2;

	trigger_irq = 0;

	return 0;
}


/***************************************************************************


								Screen Drawing


***************************************************************************/

void hanamai_copylayer(struct mame_bitmap *bitmap,const struct rectangle *cliprect,int i)
{
	int color;
	int scrollx,scrolly;

	switch ( i )
	{
		case 0:	color = (dynax_blit_palettes >>  0) & 0xf;	break;
		case 1:	color = (dynax_blit_palettes >>  4) & 0xf;	break;
		case 2:	color = (dynax_blit_palettes >>  8) & 0xf;	break;
		case 3:	color = (dynax_blit_palettes >> 12) & 0xf;	break;
		default:	return;
	}

	color += (dynax_blit_palbank & 0x0f) * 16;

	scrollx = dynax_blit_scroll_x;
	scrolly = dynax_blit_scroll_y;

	if (	(layer_layout == LAYOUT_HANAMAI		&&	i == 1)	||
			(layer_layout == LAYOUT_HNORIDUR	&&	i == 1)	)
	{
		scrollx = extra_scroll_x;
		scrolly = extra_scroll_y;
	}

	{
		int dy,length,pen,offs;
		UINT8 *src1 = dynax_pixmap[i][1];
		UINT8 *src2 = dynax_pixmap[i][0];

		int palbase = 16*color;
		offs = 0;

		for (dy = 0; dy < 256; dy++)
		{
			UINT16 *dst;
			UINT16 *dstbase = (UINT16 *)bitmap->base + ((dy - scrolly) & 0xff) * bitmap->rowpixels;

			length = scrollx;
			dst = dstbase + 2*(256 - length);
			while (length--)
			{
				pen = *(src1++);
				if (pen) *dst     = palbase + pen;
				pen = *(src2++);
				if (pen) *(dst+1) = palbase + pen;
				dst += 2;
			}

			length = 256 - scrollx;
			dst = dstbase;
			while (length--)
			{
				pen = *(src1++);
				if (pen) *dst     = palbase + pen;
				pen = *(src2++);
				if (pen) *(dst+1) = palbase + pen;
				dst += 2;
			}
		}
	}
}



void mjdialq2_copylayer(struct mame_bitmap *bitmap,const struct rectangle *cliprect,int i)
{
	int color;
	int scrollx,scrolly;

	switch ( i )
	{
		case 0:	color = (dynax_blit_palettes >>  4) & 0xf;	break;
		case 1:	color = (dynax_blit_palettes >>  0) & 0xf;	break;
		default:	return;
	}

	color += (dynax_blit_palbank & 1) * 16;

	scrollx = dynax_blit_scroll_x;
	scrolly = dynax_blit_scroll_y;

	{
		int dy,length,pen,offs;
		UINT8 *src = dynax_pixmap[i][0];

		int palbase = 16*color;
		offs = 0;

		for (dy = 0; dy < 256; dy++)
		{
			UINT16 *dst;
			UINT16 *dstbase = (UINT16 *)bitmap->base + ((dy - scrolly) & 0xff) * bitmap->rowpixels;

			length = scrollx;
			dst = dstbase + 256 - length;
			while (length--)
			{
				pen = *(src++);
				if (pen) *dst = palbase + pen;
				dst++;
			}

			length = 256 - scrollx;
			dst = dstbase;
			while (length--)
			{
				pen = *(src++);
				if (pen) *dst = palbase + pen;
				dst++;
			}
		}
	}
}



#if 0
/*	A primitive gfx viewer:

	T          -  Toggle viewer
	I,O        -  Change palette (-,+)
	J,K & N,M  -  Change "tile"  (-,+, slow & fast)
	R          -  "tile" = 0		*/

static int toggle;
if (keyboard_pressed_memory(KEYCODE_T))	toggle = 1-toggle;
if (toggle)	{
	data8_t *RAM	=	memory_region( REGION_GFX1 );
	size_t size		=	memory_region_length( REGION_GFX1 );
	static int i = 0, c = 0;

	if (keyboard_pressed_memory(KEYCODE_I))	c = (c-1) & 0x1f;
	if (keyboard_pressed_memory(KEYCODE_O))	c = (c+1) & 0x1f;
	if (keyboard_pressed_memory(KEYCODE_R))	i = 0;
	if (keyboard_pressed(KEYCODE_M) | keyboard_pressed_memory(KEYCODE_K))	{
		while( i < size && RAM[i] ) i++;		while( i < size && !RAM[i] ) i++;	}
	if (keyboard_pressed(KEYCODE_N) | keyboard_pressed_memory(KEYCODE_J))	{
		if (i >= 2) i-=2;	while( i > 0 && RAM[i] ) i--;	i++;	}

	dynax_blit_palettes = (c & 0xf) * 0x111;
	dynax_blit_palbank  = (c >>  4) & 1;
	dynax_blit_dest = 1;

	fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area);
	memset(dynax_pixmap[0][0],0,sizeof(UINT8)*0x100*0x100);
	memset(dynax_pixmap[0][1],0,sizeof(UINT8)*0x100*0x100);
	hanamai_layer_half = 0;
	sprtmtch_drawgfx(i, Machine->visible_area.min_x + Machine->visible_area.min_y*256, 0);
	hanamai_layer_half = 1;
	sprtmtch_drawgfx(i, Machine->visible_area.min_x + Machine->visible_area.min_y*256, 0);
	hanamai_copylayer(bitmap, cliprect, 0);
	usrintf_showmessage("%06X C%02X",i,c);
}
else
#endif


static int hanamai_priority;

WRITE_HANDLER( hanamai_priority_w )
{
	hanamai_priority = data;
}


static int debug_mask(void)
{
#ifdef MAME_DEBUG
	int msk = 0;
	if (keyboard_pressed(KEYCODE_Z))
	{
		if (keyboard_pressed(KEYCODE_Q))	msk |= 1;	/* layer 0*/
		if (keyboard_pressed(KEYCODE_W))	msk |= 2;	/* layer 1*/
		if (keyboard_pressed(KEYCODE_E))	msk |= 4;	/* layer 2*/
		if (keyboard_pressed(KEYCODE_R))	msk |= 8;	/* layer 3*/
		if (msk != 0)	return msk;
	}
#endif
	return -1;
}


VIDEO_UPDATE( hanamai )
{
	int layers_ctrl = ~dynax_layer_enable;
	int lay[4];

	layers_ctrl &= debug_mask();

	fillbitmap(
		bitmap,
		Machine->pens[(dynax_blit_backpen & 0xff) + (dynax_blit_palbank & 1) * 256],
		cliprect);

	/* bit 4 = display enable? */
	if (!(hanamai_priority & 0x10)) return;

	switch (hanamai_priority)
	{
		default:
			usrintf_showmessage("unknown priority %02x",hanamai_priority);
		case 0x10:
			lay[0] = 0; lay[1] = 1; lay[2] = 2; lay[3] = 3; break;
		case 0x11:
			lay[0] = 0; lay[1] = 3; lay[2] = 2; lay[3] = 1; break;
		case 0x12:
			lay[0] = 0; lay[1] = 1; lay[2] = 3; lay[3] = 2; break;
		case 0x13:
			lay[0] = 0; lay[1] = 3; lay[2] = 1; lay[3] = 2; break;
		case 0x14:
			lay[0] = 0; lay[1] = 2; lay[2] = 1; lay[3] = 3; break;
		case 0x15:
			lay[0] = 0; lay[1] = 2; lay[2] = 3; lay[3] = 1; break;
	}

	if (layers_ctrl & (1 << lay[0]))	hanamai_copylayer( bitmap, cliprect, lay[0] );
	if (layers_ctrl & (1 << lay[1]))	hanamai_copylayer( bitmap, cliprect, lay[1] );
	if (layers_ctrl & (1 << lay[2]))	hanamai_copylayer( bitmap, cliprect, lay[2] );
	if (layers_ctrl & (1 << lay[3]))	hanamai_copylayer( bitmap, cliprect, lay[3] );
}


VIDEO_UPDATE( hnoridur )
{
	int layers_ctrl = ~BITSWAP8(hanamai_priority, 7,6,5,4, 0,1,2,3);
	int lay[4], pri;

	layers_ctrl &= debug_mask();

	fillbitmap(
		bitmap,
		Machine->pens[(dynax_blit_backpen & 0xff) + (dynax_blit_palbank & 0x0f) * 256],
		cliprect);

	pri = hanamai_priority >> 4;

	if (pri > 7)
	{
		usrintf_showmessage("unknown priority %02x",hanamai_priority);
		pri = 0;
	}

	pri = priority_table[pri];
	lay[0] = (pri >> 12) & 3;
	lay[1] = (pri >>  8) & 3;
	lay[2] = (pri >>  4) & 3;
	lay[3] = (pri >>  0) & 3;

	if (layers_ctrl & (1 << lay[0]))	hanamai_copylayer( bitmap, cliprect, lay[0] );
	if (layers_ctrl & (1 << lay[1]))	hanamai_copylayer( bitmap, cliprect, lay[1] );
	if (layers_ctrl & (1 << lay[2]))	hanamai_copylayer( bitmap, cliprect, lay[2] );
	if (layers_ctrl & (1 << lay[3]))	hanamai_copylayer( bitmap, cliprect, lay[3] );

/*	usrintf_showmessage("(%04x %02x %02x)(%x %02x-%02x e %02x-%02x f%d)",dynax_blit_palettes,dynax_blit_palbank,hanamai_priority,dynax_blit_scroll_high,dynax_blit_scroll_x, dynax_blit_scroll_y,extra_scroll_x,extra_scroll_y,flipscreen);*/
}


VIDEO_UPDATE( sprtmtch )
{
	int layers_ctrl = ~dynax_layer_enable;

	layers_ctrl &= debug_mask();

	fillbitmap(
		bitmap,
		Machine->pens[(dynax_blit_backpen & 0xff) + (dynax_blit_palbank & 1) * 256],
		cliprect);

	if (layers_ctrl & 1)	hanamai_copylayer( bitmap, cliprect, 0 );
	if (layers_ctrl & 2)	hanamai_copylayer( bitmap, cliprect, 1 );
	if (layers_ctrl & 4)	hanamai_copylayer( bitmap, cliprect, 2 );
}


VIDEO_UPDATE( mjdialq2 )
{
	int layers_ctrl = ~dynax_layer_enable;

	layers_ctrl &= debug_mask();

	fillbitmap(
		bitmap,
		Machine->pens[(dynax_blit_backpen & 0xff) + (dynax_blit_palbank & 1) * 256],
		cliprect);

	if (layers_ctrl & 1)	mjdialq2_copylayer( bitmap, cliprect, 0 );
	if (layers_ctrl & 2)	mjdialq2_copylayer( bitmap, cliprect, 1 );
}
