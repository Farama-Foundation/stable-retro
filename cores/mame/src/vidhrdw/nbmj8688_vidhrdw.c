/******************************************************************************

	Video Hardware for Nichibutsu Mahjong series.

	Driver by Takahiro Nogi <nogi@kt.rim.or.jp> 1999/11/05 -

******************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "nb1413m3.h"


static int mjsikaku_scrolly;
static int blitter_destx, blitter_desty;
static int blitter_sizex, blitter_sizey;
static int blitter_src_addr;
static int mjsikaku_gfxrom;
static int mjsikaku_dispflag;
static int mjsikaku_gfxflag1;
static int mjsikaku_gfxflag2;
static int mjsikaku_gfxflag3;
static int mjsikaku_flipscreen;
static int blitter_direction_x, blitter_direction_y;
static int mjsikaku_screen_refresh;
static int mjsikaku_gfxmode;

static struct mame_bitmap *mjsikaku_tmpbitmap;
static data16_t *mjsikaku_videoram;
static data8_t *nbmj8688_color_lookup;

static data8_t *HD61830B_ram[2];
static int HD61830B_instr[2];
static int HD61830B_addr[2];


static void mjsikaku_vramflip(void);
static void mbmj8688_gfxdraw(int gfxtype);


/* the blitter can copy data both in "direct" mode, where every byte of the source
   data is copied verbatim to video RAM *twice* (thus doubling the pixel width),
   and in "lookup" mode, where the source byte is taken 4 bits at a time and indexed
   though a lookup table.
   Video RAM directly maps to a RGB output. In the first version of the hardware
   the palette was 8-bit, then they added more video RAM to have better color
   reproduction in photos. This was done in different ways, which differ for the
   implementation and the control over pixel color in the two drawing modes.
 */
enum
{
	GFXTYPE_8BIT,			/* direct mode:  8-bit; lookup table:  8-bit*/
	GFXTYPE_HYBRID_12BIT,	/* direct mode: 12-bit; lookup table:  8-bit*/
	GFXTYPE_HYBRID_16BIT,	/* direct mode: 16-bit; lookup table: 12-bit*/
	GFXTYPE_PURE_16BIT,		/* direct mode: 16-bit; lookup table: 16-bit*/
	GFXTYPE_PURE_12BIT		/* direct mode:    n/a; lookup table: 12-bit*/
};


/******************************************************************************


******************************************************************************/

PALETTE_INIT( mbmj8688_8bit )
{
	int i;

	/* initialize 332 RGB lookup */
	for (i = 0; i < 0x100; i++)
	{
		int bit0, bit1, bit2, r, g, b;

		/* xxxxxxxx_bbgggrrr*/
		/* red component */
		bit0 = ((i >> 0) & 0x01);
		bit1 = ((i >> 1) & 0x01);
		bit2 = ((i >> 2) & 0x01);
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = ((i >> 3) & 0x01);
		bit1 = ((i >> 4) & 0x01);
		bit2 = ((i >> 5) & 0x01);
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = 0;
		bit1 = ((i >> 6) & 0x01);
		bit2 = ((i >> 7) & 0x01);
		b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		palette_set_color(i, r, g, b);
	}
}

PALETTE_INIT( mbmj8688_12bit )
{
	int i;

	/* initialize 444 RGB lookup */
	for (i = 0; i < 0x1000; i++)
	{
		int r, g, b;

		/* high and low bytes swapped for convenience*/
		r = ((i & 0x07) << 1) | (((i >> 8) & 0x01) >> 0);
		g = ((i & 0x38) >> 2) | (((i >> 8) & 0x02) >> 1);
		b = ((i & 0xc0) >> 4) | (((i >> 8) & 0x0c) >> 2);

		r = ((r << 4) | r);
		g = ((g << 4) | g);
		b = ((b << 4) | b);

		palette_set_color(i, r, g, b);
	}
}

PALETTE_INIT( mbmj8688_16bit )
{
	int i;

	/* initialize 655 RGB lookup */
	for (i = 0; i < 0x10000; i++)
	{
		int r, g, b;

		r = (((i & 0x0700) >>  5) | ((i & 0x0007) >>  0));	/* R 6bit*/
		g = (((i & 0x3800) >>  9) | ((i & 0x0018) >>  3));	/* G 5bit*/
		b = (((i & 0xc000) >> 11) | ((i & 0x00e0) >>  5));	/* B 5bit*/

		r = ((r << 2) | (r >> 4));
		g = ((g << 3) | (g >> 2));
		b = ((b << 3) | (b >> 2));

		palette_set_color(i, r, g, b);
	}
}



WRITE_HANDLER( nbmj8688_color_lookup_w )
{
	nbmj8688_color_lookup[offset] = (data ^ 0xff);
}

/******************************************************************************


******************************************************************************/

WRITE_HANDLER( nbmj8688_blitter_w )
{
	switch (offset)
	{
		case 0: blitter_src_addr = (blitter_src_addr & 0xff00) | data; break;
		case 1: blitter_src_addr = (blitter_src_addr & 0x00ff) | (data << 8); break;
		case 2: blitter_destx = data; break;
		case 3: blitter_desty = data; break;
		case 4: blitter_sizex = data; break;
		case 5: blitter_sizey = data;
				/* writing here also starts the blit */
			    mbmj8688_gfxdraw(mjsikaku_gfxmode);
				break;
	}
}

WRITE_HANDLER( mjsikaku_gfxflag1_w )
{
	static int mjsikaku_flipscreen_old = -1;

	mjsikaku_gfxflag1 = data;

	blitter_direction_x = (data & 0x01) ? 1 : 0;
	blitter_direction_y = (data & 0x02) ? 1 : 0;
	mjsikaku_flipscreen = (data & 0x04) ? 0 : 1;
	mjsikaku_dispflag = (data & 0x08) ? 0 : 1;

	if (mjsikaku_flipscreen != mjsikaku_flipscreen_old)
	{
		mjsikaku_vramflip();
		mjsikaku_screen_refresh = 1;
		mjsikaku_flipscreen_old = mjsikaku_flipscreen;
	}
}

WRITE_HANDLER( mjsikaku_gfxflag2_w )
{
	mjsikaku_gfxflag2 = data;

	if (nb1413m3_type == NB1413M3_SEIHAM
			|| nb1413m3_type == NB1413M3_KORINAI)
		mjsikaku_gfxflag2 ^= 0x20;
}

WRITE_HANDLER( mjsikaku_gfxflag3_w )
{
	mjsikaku_gfxflag3 = (data & 0xe0);
}

WRITE_HANDLER( mjsikaku_scrolly_w )
{
	mjsikaku_scrolly = data;
}

WRITE_HANDLER( mjsikaku_romsel_w )
{
	mjsikaku_gfxrom = (data & 0x07);

	if ((mjsikaku_gfxrom << 17) > (memory_region_length(REGION_GFX1) - 1))
	{
#ifdef MAME_DEBUG
		usrintf_showmessage("GFXROM BANK OVER!!");
#endif
		mjsikaku_gfxrom = 0;
	}
}

WRITE_HANDLER( secolove_romsel_w )
{
	mjsikaku_gfxrom = ((data & 0xc0) >> 4) + (data & 0x03);
	mjsikaku_gfxflag2_w(0,data);

	if ((mjsikaku_gfxrom << 17) > (memory_region_length(REGION_GFX1) - 1))
	{
#ifdef MAME_DEBUG
		usrintf_showmessage("GFXROM BANK OVER!!");
#endif
		mjsikaku_gfxrom = 0;
	}
}

WRITE_HANDLER( crystal2_romsel_w )
{
	mjsikaku_gfxrom = (data & 0x03);
	mjsikaku_gfxflag2_w(0,data);

	if ((mjsikaku_gfxrom << 17) > (memory_region_length(REGION_GFX1) - 1))
	{
#ifdef MAME_DEBUG
		usrintf_showmessage("GFXROM BANK OVER!!");
#endif
		mjsikaku_gfxrom = 0;
	}
}

WRITE_HANDLER( seiha_romsel_w )
{
	mjsikaku_gfxrom = (data & 0x1f);
	mjsikaku_gfxflag3_w(0,data);

	if ((mjsikaku_gfxrom << 17) > (memory_region_length(REGION_GFX1) - 1))
	{
#ifdef MAME_DEBUG
		usrintf_showmessage("GFXROM BANK OVER!!");
#endif
		mjsikaku_gfxrom = 0;
	}
}

/******************************************************************************


******************************************************************************/
void mjsikaku_vramflip(void)
{
	int x, y;
	unsigned short color1, color2;

	for (y = 0; y < (256 / 2); y++)
	{
		for (x = 0; x < 512; x++)
		{
			color1 = mjsikaku_videoram[(y * 512) + x];
			color2 = mjsikaku_videoram[((y ^ 0xff) * 512) + (x ^ 0x1ff)];
			mjsikaku_videoram[(y * 512) + x] = color2;
			mjsikaku_videoram[((y ^ 0xff) * 512) + (x ^ 0x1ff)] = color1;
		}
	}
}


static void update_pixel(int x,int y)
{
	int color = mjsikaku_videoram[(y * 512) + x];
	plot_pixel(mjsikaku_tmpbitmap, x, y, Machine->pens[color]);
}

static void writeram_low(int x,int y,int color)
{
	mjsikaku_videoram[(y * 512) + x] &= 0xff00;
	mjsikaku_videoram[(y * 512) + x] |= color;
	update_pixel(x,y);
}

static void writeram_high(int x,int y,int color)
{
	mjsikaku_videoram[(y * 512) + x] &= 0x00ff;
	mjsikaku_videoram[(y * 512) + x] |= color << 8;
	update_pixel(x,y);
}


static void mbmj8688_gfxdraw(int gfxtype)
{
	unsigned char *GFX = memory_region(REGION_GFX1);

	int x, y;
	int dx1, dx2, dy;
	int startx, starty;
	int sizex, sizey;
	int skipx, skipy;
	int ctrx, ctry;
	int gfxaddr;
	unsigned short color, color1, color2;

	if (gfxtype == GFXTYPE_PURE_12BIT)
	{
		if (mjsikaku_gfxflag2 & 0x20) return;
	}

	startx = blitter_destx + blitter_sizex;
	starty = blitter_desty + blitter_sizey;

	if (blitter_direction_x)
	{
		sizex = blitter_sizex ^ 0xff;
		skipx = 1;
	}
	else
	{
		sizex = blitter_sizex;
		skipx = -1;
	}

	if (blitter_direction_y)
	{
		sizey = blitter_sizey ^ 0xff;
		skipy = 1;
	}
	else
	{
		sizey = blitter_sizey;
		skipy = -1;
	}

	gfxaddr = (mjsikaku_gfxrom << 17) + (blitter_src_addr << 1);

	for (y = starty, ctry = sizey; ctry >= 0; y += skipy, ctry--)
	{
		for (x = startx, ctrx = sizex; ctrx >= 0; x += skipx, ctrx--)
		{
			if ((gfxaddr > (memory_region_length(REGION_GFX1) - 1)))
			{
#ifdef MAME_DEBUG
				usrintf_showmessage("GFXROM ADDRESS OVER!!");
#endif
				gfxaddr = 0;
			}

			color = GFX[gfxaddr++];

			dx1 = (2 * x + 0) & 0x1ff;
			dx2 = (2 * x + 1) & 0x1ff;
			dy = (y + mjsikaku_scrolly) & 0xff;

			if (mjsikaku_flipscreen)
			{
				dx1 ^= 0x1ff;
				dx2 ^= 0x1ff;
				dy ^= 0xff;
			}

			if (gfxtype == GFXTYPE_HYBRID_16BIT)
			{
				if (mjsikaku_gfxflag3 & 0x40)
				{
					/* direct mode*/

					if (mjsikaku_gfxflag3 & 0x80)
					{
						/* least significant bits */
						if (color != 0xff)
						{
							writeram_low(dx1,dy,color);
							writeram_low(dx2,dy,color);
						}
					}
					else
					{
						/* most significant bits */
						if (color != 0xff)
						{
							writeram_high(dx1,dy,color);
							writeram_high(dx2,dy,color);
						}
					}
				}
				else
				{
					/* 16-bit palette with 4-to-12 bit lookup (!) */
					/* lookup table mode*/

					/* unknown flag (seiha, seiham)*/
				/*	if (mjsikaku_gfxflag3 & 0x80) return;*/

					/* unknown (seiha, seiham, iemoto, ojousan)*/
					if (!(mjsikaku_gfxflag2 & 0x20)) return;

					if (blitter_direction_x)
					{
						/* flip*/
						color1 = (color & 0x0f) >> 0;
						color2 = (color & 0xf0) >> 4;
					}
					else
					{
						/* normal*/
						color1 = (color & 0xf0) >> 4;
						color2 = (color & 0x0f) >> 0;
					}

					color1 = (nbmj8688_color_lookup[color1] << 8) | ((nbmj8688_color_lookup[color1 | 0x10] & 0x0f) << 4);
					color2 = (nbmj8688_color_lookup[color2] << 8) | ((nbmj8688_color_lookup[color2 | 0x10] & 0x0f) << 4);

					if (color1 != 0xfff0)
					{
						/* extend color from 12-bit to 16-bit */
						color1 = (color1 & 0xffc0) | ((color1 & 0x20) >> 1) | ((color1 & 0x10) >> 2);
						mjsikaku_videoram[(dy * 512) + dx1] = color1;
						update_pixel(dx1,dy);
					}

					if (color2 != 0xfff0)
					{
						/* extend color from 12-bit to 16-bit */
						color2 = (color2 & 0xffc0) | ((color2 & 0x20) >> 1) | ((color2 & 0x10) >> 2);
						mjsikaku_videoram[(dy * 512) + dx2] = color2;
						update_pixel(dx2,dy);
					}
				}
			}
			else if (gfxtype == GFXTYPE_PURE_12BIT)
			{
				/* 12-bit palette with 4-to-12 bit lookup table */

				if (blitter_direction_x)
				{
					/* flip*/
					color1 = (color & 0x0f) >> 0;
					color2 = (color & 0xf0) >> 4;
				}
				else
				{
					/* normal*/
					color1 = (color & 0xf0) >> 4;
					color2 = (color & 0x0f) >> 0;
				}

				color1 = nbmj8688_color_lookup[color1] | ((nbmj8688_color_lookup[color1 | 0x10] & 0x0f) << 8);
				color2 = nbmj8688_color_lookup[color2] | ((nbmj8688_color_lookup[color2 | 0x10] & 0x0f) << 8);

				if (color1 != 0x0fff)
				{
					mjsikaku_videoram[(dy * 512) + dx1] = color1;
					update_pixel(dx1, dy);
				}
				if (color2 != 0x0fff)
				{
					mjsikaku_videoram[(dy * 512) + dx2] = color2;
					update_pixel(dx2, dy);
				}
			}
			else
			{
				if (gfxtype == GFXTYPE_HYBRID_12BIT && (mjsikaku_gfxflag2 & 0x20))
				{
					/* 4096 colors mode, wedged in on top of normal mode
					   Here we affect only the 4 least significant bits, the others are
					   changed as usual.
					 */

					if (mjsikaku_gfxflag2 & 0x10)
					{
						/* 4096 colors low mode (2nd draw upper)*/
						color = nbmj8688_color_lookup[((color & 0xf0) >> 4)];
					}
					else
					{
						/* 4096 colors low mode (1st draw lower)*/
						color = nbmj8688_color_lookup[((color & 0x0f) >> 0)];
					}

					if (color != 0xff)
					{
						color &= 0x0f;
						writeram_high(dx1,dy,color);
						writeram_high(dx2,dy,color);
					}
				}
				else
				{
					if (mjsikaku_gfxflag2 & 0x04)
					{
						/* direct mode*/

						color1 = color2 = color;
					}
					else
					{
						/* lookup table mode*/

						if (blitter_direction_x)
						{
							/* flip*/
							color1 = (color & 0x0f) >> 0;
							color2 = (color & 0xf0) >> 4;
						}
						else
						{
							/* normal*/
							color1 = (color & 0xf0) >> 4;
							color2 = (color & 0x0f) >> 0;
						}

						color1 = nbmj8688_color_lookup[color1];
						color2 = nbmj8688_color_lookup[color2];
					}

					if (gfxtype == GFXTYPE_PURE_16BIT && !(mjsikaku_gfxflag2 & 0x20))
					{
						/* 16-bit palette most significant bits */
						if (color1 != 0xff) writeram_high(dx1,dy,color1);
						if (color2 != 0xff) writeram_high(dx2,dy,color2);
					}
					else
					{
						/* 8-bit palette or 16-bit palette least significant bits */
						if (color1 != 0xff) writeram_low(dx1,dy,color1);
						if (color2 != 0xff) writeram_low(dx2,dy,color2);
					}
				}
			}

			nb1413m3_busyctr++;
		}
	}

	if (gfxtype == GFXTYPE_8BIT)
		nb1413m3_busyflag = (nb1413m3_busyctr > 600) ? 0 : 1;
	else
		nb1413m3_busyflag = (nb1413m3_busyctr > 4000) ? 0 : 1;
}


/******************************************************************************


******************************************************************************/

static int common_video_start(void)
{
	if ((mjsikaku_tmpbitmap = auto_bitmap_alloc(512, 256)) == 0) return 1;
	if ((mjsikaku_videoram = auto_malloc(512 * 256 * sizeof(data16_t))) == 0) return 1;
	if ((nbmj8688_color_lookup = auto_malloc(0x20 * sizeof(data8_t))) == 0) return 1;
	memset(mjsikaku_videoram, 0, (512 * 256 * sizeof(data16_t)));

	mjsikaku_scrolly = 0;	/* reset because crystalg/crystal2 don't write to this register*/

	return 0;
}

VIDEO_START( mbmj8688_8bit )
{
	mjsikaku_gfxmode = GFXTYPE_8BIT;
	return common_video_start();
}

VIDEO_START( mbmj8688_hybrid_12bit )
{
	mjsikaku_gfxmode = GFXTYPE_HYBRID_12BIT;
	return common_video_start();
}

VIDEO_START( mbmj8688_pure_12bit )
{
	mjsikaku_gfxmode = GFXTYPE_PURE_12BIT;
	return common_video_start();
}

VIDEO_START( mbmj8688_hybrid_16bit )
{
	mjsikaku_gfxmode = GFXTYPE_HYBRID_16BIT;
	return common_video_start();
}

VIDEO_START( mbmj8688_pure_16bit )
{
	mjsikaku_gfxmode = GFXTYPE_PURE_16BIT;
	return common_video_start();
}

VIDEO_START( mbmj8688_pure_16bit_LCD )
{
	mjsikaku_gfxmode = GFXTYPE_PURE_16BIT;

	if ((HD61830B_ram[0] = auto_malloc(0x10000)) == 0) return 1;
	if ((HD61830B_ram[1] = auto_malloc(0x10000)) == 0) return 1;

	return common_video_start();
}


/******************************************************************************

Quick and dirty implementation of the bare minimum required to elmulate the
Hitachi HD61830B LCD controller.

******************************************************************************/

static void nbmj8688_HD61830B_instr_w(int chip,int offset,int data)
{
	HD61830B_instr[chip] = data;
}

static void nbmj8688_HD61830B_data_w(int chip,int offset,int data)
{
	switch (HD61830B_instr[chip])
	{
		case 0x0a:	/* set cursor address (low order)*/
			HD61830B_addr[chip] = (HD61830B_addr[chip] & 0xff00) | data;
			break;
		case 0x0b:	/* set cursor address (high order)*/
			HD61830B_addr[chip] = (HD61830B_addr[chip] & 0x00ff) | (data << 8);
			break;
		case 0x0c:	/* write display data*/
			HD61830B_ram[chip][HD61830B_addr[chip]++] = data;
			break;
		default:
log_cb(RETRO_LOG_DEBUG, LOGPRE "HD61830B unsupported instruction %02x %02x\n",HD61830B_instr[chip],data);
			break;
	}
}

WRITE_HANDLER( nbmj8688_HD61830B_0_instr_w )
{
	nbmj8688_HD61830B_instr_w(0,offset,data);
}

WRITE_HANDLER( nbmj8688_HD61830B_1_instr_w )
{
	nbmj8688_HD61830B_instr_w(1,offset,data);
}

WRITE_HANDLER( nbmj8688_HD61830B_both_instr_w )
{
	nbmj8688_HD61830B_instr_w(0,offset,data);
	nbmj8688_HD61830B_instr_w(1,offset,data);
}

WRITE_HANDLER( nbmj8688_HD61830B_0_data_w )
{
	nbmj8688_HD61830B_data_w(0,offset,data);
}

WRITE_HANDLER( nbmj8688_HD61830B_1_data_w )
{
	nbmj8688_HD61830B_data_w(1,offset,data);
}

WRITE_HANDLER( nbmj8688_HD61830B_both_data_w )
{
	nbmj8688_HD61830B_data_w(0,offset,data);
	nbmj8688_HD61830B_data_w(1,offset,data);
}



/******************************************************************************


******************************************************************************/


VIDEO_UPDATE( mbmj8688 )
{
	int x, y;

	if (get_vh_global_attribute_changed() || mjsikaku_screen_refresh)
	{
		mjsikaku_screen_refresh = 0;
		for (y = 0; y < 256; y++)
		{
			for (x = 0; x < 512; x++)
			{
				update_pixel(x,y);
			}
		}
	}

	if (mjsikaku_dispflag)
	{
		int scrolly;
		if (mjsikaku_flipscreen) scrolly =   mjsikaku_scrolly;
		else                     scrolly = (-mjsikaku_scrolly) & 0xff;

		if (cliprect->min_y > 64)	/* kludge to compensate for LCD on top of screen*/
			scrolly += 64;
		copybitmap(bitmap, mjsikaku_tmpbitmap, 0, 0, 0, scrolly,       cliprect, TRANSPARENCY_NONE, 0);
		copybitmap(bitmap, mjsikaku_tmpbitmap, 0, 0, 0, scrolly - 256, cliprect, TRANSPARENCY_NONE, 0);
	}
	else
	{
		fillbitmap(bitmap, Machine->pens[0], 0);
	}
}



VIDEO_UPDATE( mbmj8688_LCD )
{
	int x, y, b;
	struct rectangle clip = *cliprect;

	clip.min_y += 64;
	clip.max_y -= 64;
	video_update_mbmj8688(bitmap,&clip);
	clip.min_y -= 64;
	clip.max_y += 64;

	for (y = 0;y < 64;y++)
	{
		for (x = 0;x < 60;x++)
		{
			int data = HD61830B_ram[0][y * 60 + x];

			for (b = 0;b < 8;b++)
				plot_pixel(bitmap,16 + 480-1-(8*x+b),224+16 + 64*2-1-y,(data & (1<<b)) ? 0x0000 : 0x18ff);
		}
	}

	for (y = 0;y < 64;y++)
	{
		for (x = 0;x < 60;x++)
		{
			int data = HD61830B_ram[1][y * 60 + x];

			for (b = 0;b < 8;b++)
				plot_pixel(bitmap,16 + (8*x+b),16+y,(data & (1<<b)) ? 0x0000 : 0x18ff);
		}
	}
}
