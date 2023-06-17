/******************************************************************************

	Video Hardware for Nichibutsu Mahjong series.

	Driver by Takahiro Nogi <nogi@kt.rim.or.jp> 1999/11/05 -
	Special thanks to Tatsuyuki Satoh

******************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


#define	VRAM_MAX	2

#define	RASTER_SCROLL	1

#define	SCANLINE_MIN	0
#define	SCANLINE_MAX	512


static int sailorws_scrollx[VRAM_MAX], sailorws_scrolly[VRAM_MAX];
static unsigned char sailorws_scrollx_tmp[VRAM_MAX][2], sailorws_scrolly_tmp[VRAM_MAX][2];
#if RASTER_SCROLL
static int sailorws_scrollx_raster[VRAM_MAX][SCANLINE_MAX];
static int sailorws_scanline[VRAM_MAX];
#endif
static int sailorws_drawx[VRAM_MAX], sailorws_drawy[VRAM_MAX];
static unsigned char sailorws_drawx_tmp[VRAM_MAX][2], sailorws_drawy_tmp[VRAM_MAX][2];
static int sailorws_sizex[VRAM_MAX], sailorws_sizey[VRAM_MAX];
static int sailorws_radr[VRAM_MAX];
static unsigned char sailorws_radr_tmp[VRAM_MAX][3];
static int sailorws_gfxflag[VRAM_MAX];
static int sailorws_dispflag[VRAM_MAX];
static int sailorws_flipscreen[VRAM_MAX];
static int sailorws_highcolor[VRAM_MAX];
static int sailorws_transparency[VRAM_MAX];
static int sailorws_flipx[VRAM_MAX], sailorws_flipy[VRAM_MAX];
static int sailorws_paltblnum;
static int sailorws_screen_refresh;
static int sailorws_gfxflag2;
static int gfxdraw_mode;

static struct mame_bitmap *sailorws_tmpbitmap0, *sailorws_tmpbitmap1;
static unsigned short *sailorws_videoram0, *sailorws_videoram1;
static unsigned short *sailorws_videoworkram0, *sailorws_videoworkram1;
static unsigned char *sailorws_palette, *mscoutm_palette;
static unsigned char *sailorws_paltbl0, *sailorws_paltbl1;


static void sailorws_vramflip(int vram);
static void sailorws_gfxdraw(int vram);
static void mscoutm_gfxdraw(int vram);


/******************************************************************************


******************************************************************************/
READ_HANDLER( sailorws_palette_r )
{
	return sailorws_palette[offset];
}

WRITE_HANDLER( sailorws_palette_w )
{
	int r, g, b;

	sailorws_palette[offset] = data;

	if (offset & 1)
	{
		offset &= 0x1fe;

		r = ((sailorws_palette[offset + 0] & 0x0f) << 4);
		g = ((sailorws_palette[offset + 0] & 0xf0) << 0);
		b = ((sailorws_palette[offset + 1] & 0x0f) << 4);

		r = (r | (r >> 4));
		g = (g | (g >> 4));
		b = (b | (b >> 4));

		palette_set_color((offset >> 1), r, g, b);
	}
}

READ_HANDLER( mscoutm_palette_r )
{
	return mscoutm_palette[offset];
}

WRITE_HANDLER( mscoutm_palette_w )
{
	int r, g, b;
	int offs_h, offs_l;

	mscoutm_palette[offset] = data;

	offs_h = (offset / 0x0300);	/* 0x000, 0x300, 0x600, 0x900*/
	offs_l = (offset & 0x00ff);	/* 0x000 - 0x0ff*/

	r = mscoutm_palette[(0x000 + (offs_h * 0x300) + offs_l)];
	g = mscoutm_palette[(0x100 + (offs_h * 0x300) + offs_l)];
	b = mscoutm_palette[(0x200 + (offs_h * 0x300) + offs_l)];

	palette_set_color(((offs_h * 0x100) + offs_l), r, g, b);
}

/******************************************************************************


******************************************************************************/
int sailorws_gfxbusy_r(int vram, int offset)
{
	return 0xfe;
}

void sailorws_scrollx_w(int vram, int offset, int data)
{
#if RASTER_SCROLL
	int new_line;

	sailorws_scrollx_tmp[vram][offset] = data;

	if (offset)
	{
		sailorws_scrollx[vram] = -((((sailorws_scrollx_tmp[vram][0] + (sailorws_scrollx_tmp[vram][1] << 8)) & 0x1ff) + 0x4e) << 1);

		if (gfxdraw_mode != 2)
		{
			/* update line scroll position */

			new_line = cpu_getscanline();
			if (new_line > SCANLINE_MAX) new_line = SCANLINE_MAX;

			if (sailorws_flipscreen[vram])
			{
				for ( ; sailorws_scanline[vram] < new_line; sailorws_scanline[vram]++)
				{
					sailorws_scrollx_raster[vram][sailorws_scanline[vram]] = sailorws_scrollx[vram];
				}
			}
			else
			{
				for ( ; sailorws_scanline[vram] < new_line; sailorws_scanline[vram]++)
				{
					sailorws_scrollx_raster[vram][(sailorws_scanline[vram] ^ 0x1ff)] = sailorws_scrollx[vram];
				}
			}
		}
	}
#else
	sailorws_scrollx_tmp[vram][offset] = data;

	if (offset)
	{
		sailorws_scrollx[vram] = -((((sailorws_scrollx_tmp[vram][0] + (sailorws_scrollx_tmp[vram][1] << 8)) & 0x1ff) + 0x4e) << 1);
	}
#endif
}

void sailorws_scrolly_w(int vram, int offset, int data)
{
	sailorws_scrolly_tmp[vram][offset] = data;

	if (offset)
	{
		if (sailorws_flipscreen[vram]) sailorws_scrolly[vram] = ((sailorws_scrolly_tmp[vram][0] + (sailorws_scrolly_tmp[vram][1] << 8)) ^ 0x1ff) & 0x1ff;
		else sailorws_scrolly[vram] = (sailorws_scrolly_tmp[vram][0] + (sailorws_scrolly_tmp[vram][1] << 8) + 1) & 0x1ff;
	}
}

void sailorws_radr_w(int vram, int offset, int data)
{
	sailorws_radr_tmp[vram][offset] = data;

	if (!offset)
	{
		sailorws_radr[vram] = (sailorws_radr_tmp[vram][0] + (sailorws_radr_tmp[vram][1] << 8) + (sailorws_radr_tmp[vram][2] << 16));
	}
}

void sailorws_gfxflag_w(int vram, int offset, int data)
{
	static int sailorws_flipscreen_old[VRAM_MAX] = { -1, -1 };

	sailorws_gfxflag[vram] = data;

	sailorws_flipx[vram] = (data & 0x01) ? 1 : 0;
	sailorws_flipy[vram] = (data & 0x02) ? 1 : 0;
	sailorws_highcolor[vram] = (data & 0x04) ? 1 : 0;
/*	if (data & 0x08) usrintf_showmessage("Unknown GFX Flag!! (0x08)");*/
	sailorws_transparency[vram] = (data & 0x10) ? 1 : 0;
/*	if (data & 0x20) usrintf_showmessage("Unknown GFX Flag!! (0x20)");*/
	sailorws_flipscreen[vram] = (data & 0x40) ? 0 : 1;
	sailorws_dispflag[vram] = (data & 0x80) ? 1 : 0;

	if (sailorws_flipscreen[vram] != sailorws_flipscreen_old[vram])
	{
		sailorws_screen_refresh = 1;
		sailorws_flipscreen_old[vram] = sailorws_flipscreen[vram];
		sailorws_vramflip(vram);
	}
}

void sailorws_sizex_w(int vram, int offset, int data)
{
	sailorws_sizex[vram] = data;
}

void sailorws_sizey_w(int vram, int offset, int data)
{
	sailorws_sizey[vram] = data;
}

void sailorws_drawx_w(int vram, int offset, int data)
{
	sailorws_drawx_tmp[vram][offset] = data;

	if (offset)
	{
		sailorws_drawx[vram] = ((sailorws_drawx_tmp[vram][0] + (sailorws_drawx_tmp[vram][1] << 8)) ^ 0x3ff) & 0x3ff;
	}
}

void sailorws_drawy_w(int vram, int offset, int data)
{
	sailorws_drawy_tmp[vram][offset] = data;

	if (offset)
	{
		sailorws_drawy[vram] = ((sailorws_drawy_tmp[vram][0] + (sailorws_drawy_tmp[vram][1] << 8)) ^ 0x1ff) & 0x1ff;

		if (gfxdraw_mode == 2) mscoutm_gfxdraw(vram);
		else sailorws_gfxdraw(vram);
	}
}

void sailorws_paltblnum_w(int data)
{
	sailorws_paltblnum = data;
}

WRITE_HANDLER( sailorws_paltbl_0_w )
{
	sailorws_paltbl0[((sailorws_paltblnum & 0xff) * 0x10) + (offset & 0x0f)] = data;
}

WRITE_HANDLER( sailorws_paltbl_1_w )
{
	sailorws_paltbl1[((sailorws_paltblnum & 0xff) * 0x10) + (offset & 0x0f)] = data;
}

void sailorws_gfxflag2_w(int data)
{
	sailorws_gfxflag2 = data;
}

int sailorws_gfxrom_r(int vram, int offset)
{
	unsigned char *GFXROM = memory_region(REGION_GFX1);

	return GFXROM[sailorws_radr[vram]];
}

/******************************************************************************


******************************************************************************/
static void sailorws_vramflip(int vram)
{
	int x, y;
	unsigned short color1, color2;
	unsigned short *vidram;

	vidram = vram ? sailorws_videoram1 : sailorws_videoram0;

	for (y = 0; y < (Machine->drv->screen_height / 2); y++)
	{
		for (x = 0; x < Machine->drv->screen_width; x++)
		{
			color1 = vidram[(y * Machine->drv->screen_width) + x];
			color2 = vidram[((y ^ 0x1ff) * Machine->drv->screen_width) + (x ^ 0x3ff)];
			vidram[(y * Machine->drv->screen_width) + x] = color2;
			vidram[((y ^ 0x1ff) * Machine->drv->screen_width) + (x ^ 0x3ff)] = color1;
		}
	}

	if (gfxdraw_mode == 2)
	{
		vidram = vram ? sailorws_videoworkram1 : sailorws_videoworkram0;

		for (y = 0; y < (Machine->drv->screen_height / 2); y++)
		{
			for (x = 0; x < Machine->drv->screen_width; x++)
			{
				color1 = vidram[(y * Machine->drv->screen_width) + x];
				color2 = vidram[((y ^ 0x1ff) * Machine->drv->screen_width) + (x ^ 0x3ff)];
				vidram[(y * Machine->drv->screen_width) + x] = color2;
				vidram[((y ^ 0x1ff) * Machine->drv->screen_width) + (x ^ 0x3ff)] = color1;
			}
		}
	}
}

static void sailorws_gfxdraw(int vram)
{
	unsigned char *GFX = memory_region(REGION_GFX1);

	int x, y;
	int dx1, dx2, dy;
	int startx, starty;
	int sizex, sizey;
	int skipx, skipy;
	int ctrx, ctry;
	int tflag1, tflag2;
	unsigned char color, color1, color2;
	unsigned short drawcolor1, drawcolor2;
	int gfxaddr;

	if (sailorws_flipx[vram])
	{
		sailorws_drawx[vram] -= sailorws_sizex[vram];
		startx = sailorws_sizex[vram];
		sizex = (sailorws_sizex[vram] + 1);
		skipx = -1;
	}
	else
	{
		sailorws_drawx[vram] = (sailorws_drawx[vram] - sailorws_sizex[vram]);
		startx = 0;
		sizex = (sailorws_sizex[vram] + 1);
		skipx = 1;
	}

	if (sailorws_flipy[vram])
	{
		sailorws_drawy[vram] -= (sailorws_sizey[vram] + 1);
		starty = sailorws_sizey[vram];
		sizey = (sailorws_sizey[vram] + 1);
		skipy = -1;
	}
	else
	{
		sailorws_drawy[vram] = (sailorws_drawy[vram] - sailorws_sizey[vram] - 1);
		starty = 0;
		sizey = (sailorws_sizey[vram] + 1);
		skipy = 1;
	}

	gfxaddr = ((sailorws_radr[vram] + 2) & 0x00ffffff);

	Machine->pens[0xff] = 0;	/* palette_transparent_pen */

	for (y = starty, ctry = sizey; ctry > 0; y += skipy, ctry--)
	{
		for (x = startx, ctrx = sizex; ctrx > 0; x += skipx, ctrx--)
		{
			if ((gfxaddr > (memory_region_length(REGION_GFX1) - 1)))
			{
#ifdef MAME_DEBUG
				usrintf_showmessage("GFXROM ADDRESS OVER!!");
#endif
				gfxaddr = 0;
			}

			color = GFX[gfxaddr++];

			if (sailorws_flipscreen[vram])
			{
				dx1 = (((((sailorws_drawx[vram] + x) * 2) + 0) ^ 0x3ff) & 0x3ff);
				dx2 = (((((sailorws_drawx[vram] + x) * 2) + 1) ^ 0x3ff) & 0x3ff);
				dy = (((sailorws_drawy[vram] + y) ^ 0x1ff) & 0x1ff);
			}
			else
			{
				dx1 = ((((sailorws_drawx[vram] + x) * 2) + 0) & 0x3ff);
				dx2 = ((((sailorws_drawx[vram] + x) * 2) + 1) & 0x3ff);
				dy = ((sailorws_drawy[vram] + y) & 0x1ff);
			}

			if (sailorws_flipx[vram])
			{
				/* flip*/
				color1 = (color & 0xf0) >> 4;
				color2 = (color & 0x0f) >> 0;
			}
			else
			{
				/* normal*/
				color1 = (color & 0x0f) >> 0;
				color2 = (color & 0xf0) >> 4;
			}

			if (!vram)
			{
				drawcolor1 = sailorws_paltbl0[(sailorws_paltblnum * 0x10) + color1];
				drawcolor2 = sailorws_paltbl0[(sailorws_paltblnum * 0x10) + color2];
			}
			else
			{
				drawcolor1 = sailorws_paltbl1[(sailorws_paltblnum * 0x10) + color1];
				drawcolor2 = sailorws_paltbl1[(sailorws_paltblnum * 0x10) + color2];
			}

			if (sailorws_transparency[vram])
			{
				tflag1 = (drawcolor1 != 0xff) ? 1 : 0;
				tflag2 = (drawcolor2 != 0xff) ? 1 : 0;
			}
			else
			{
				tflag1 = 1;
				tflag2 = 1;
			}

			if (!vram)
			{
				if (tflag1)
				{
					sailorws_videoram0[(dy * Machine->drv->screen_width) + dx1] = drawcolor1;
					plot_pixel(sailorws_tmpbitmap0, dx1, dy, Machine->pens[drawcolor1]);
				}
				if (tflag2)
				{
					sailorws_videoram0[(dy * Machine->drv->screen_width) + dx2] = drawcolor2;
					plot_pixel(sailorws_tmpbitmap0, dx2, dy, Machine->pens[drawcolor2]);
				}
			}
			else
			{
				if (tflag1)
				{
					sailorws_videoram1[(dy * Machine->drv->screen_width) + dx1] = drawcolor1;
					plot_pixel(sailorws_tmpbitmap1, dx1, dy, Machine->pens[drawcolor1]);
				}
				if (tflag2)
				{
					sailorws_videoram1[(dy * Machine->drv->screen_width) + dx2] = drawcolor2;
					plot_pixel(sailorws_tmpbitmap1, dx2, dy, Machine->pens[drawcolor2]);
				}
			}
		}
	}
}

static void mscoutm_gfxdraw(int vram)
{
	unsigned char *GFX = memory_region(REGION_GFX1);

	int x, y;
	int dx1, dx2, dy;
	int startx, starty;
	int sizex, sizey;
	int skipx, skipy;
	int ctrx, ctry;
	int tflag1, tflag2;
	unsigned char color, color1, color2;
	unsigned short drawcolor1, drawcolor2;
	int gfxaddr;

	if (sailorws_highcolor[vram])
	{
		/* NB22090 high color mode*/
		sailorws_sizex[vram] = (GFX[((sailorws_radr[vram] + 0) & 0x00ffffff)] & 0xff);
		sailorws_sizey[vram] = (GFX[((sailorws_radr[vram] + 1) & 0x00ffffff)] & 0xff);
	}

	if (sailorws_flipx[vram])
	{
		sailorws_drawx[vram] -= sailorws_sizex[vram];
		startx = sailorws_sizex[vram];
		sizex = (sailorws_sizex[vram] + 1);
		skipx = -1;
	}
	else
	{
		sailorws_drawx[vram] = (sailorws_drawx[vram] - sailorws_sizex[vram]);
		startx = 0;
		sizex = (sailorws_sizex[vram] + 1);
		skipx = 1;
	}

	if (sailorws_flipy[vram])
	{
		sailorws_drawy[vram] -= (sailorws_sizey[vram] + 1);
		starty = sailorws_sizey[vram];
		sizey = (sailorws_sizey[vram] + 1);
		skipy = -1;
	}
	else
	{
		sailorws_drawy[vram] = (sailorws_drawy[vram] - sailorws_sizey[vram] - 1);
		starty = 0;
		sizey = (sailorws_sizey[vram] + 1);
		skipy = 1;
	}

	gfxaddr = ((sailorws_radr[vram] + 2) & 0x00ffffff);

	Machine->pens[0x0ff] = 0;	/* palette_transparent_pen */
	Machine->pens[0x1ff] = 0;	/* palette_transparent_pen */

	for (y = starty, ctry = sizey; ctry > 0; y += skipy, ctry--)
	{
		for (x = startx, ctrx = sizex; ctrx > 0; x += skipx, ctrx--)
		{
			if ((gfxaddr > (memory_region_length(REGION_GFX1) - 1)))
			{
#ifdef MAME_DEBUG
				usrintf_showmessage("GFXROM ADDRESS OVER!!");
#endif
				gfxaddr = 0;
			}

			color = GFX[gfxaddr++];

			if (sailorws_flipscreen[vram])
			{
				dx1 = (((((sailorws_drawx[vram] + x) * 2) + 0) ^ 0x3ff) & 0x3ff);
				dx2 = (((((sailorws_drawx[vram] + x) * 2) + 1) ^ 0x3ff) & 0x3ff);
				dy = (((sailorws_drawy[vram] + y) ^ 0x1ff) & 0x1ff);
			}
			else
			{
				dx1 = ((((sailorws_drawx[vram] + x) * 2) + 0) & 0x3ff);
				dx2 = ((((sailorws_drawx[vram] + x) * 2) + 1) & 0x3ff);
				dy = ((sailorws_drawy[vram] + y) & 0x1ff);
			}

			if (sailorws_flipx[vram])
			{
				/* flip*/
				color1 = (color & 0xf0) >> 4;
				color2 = (color & 0x0f) >> 0;
			}
			else
			{
				/* normal*/
				color1 = (color & 0x0f) >> 0;
				color2 = (color & 0xf0) >> 4;
			}

			if (sailorws_highcolor[vram])
			{
				/* high color mode*/

				if (sailorws_gfxflag2 & 0xc0)
				{
					/* high color mode 1st draw*/

					drawcolor1 = ((color1 & 0x0f) << 0);
					drawcolor2 = ((color2 & 0x0f) << 0);

					if (!vram)
					{
						sailorws_videoworkram0[(dy * Machine->drv->screen_width) + dx1] = drawcolor1;
						sailorws_videoworkram0[(dy * Machine->drv->screen_width) + dx2] = drawcolor2;
					}
					else
					{
						sailorws_videoworkram1[(dy * Machine->drv->screen_width) + dx1] = drawcolor1;
						sailorws_videoworkram1[(dy * Machine->drv->screen_width) + dx2] = drawcolor2;
					}
					continue;
				}
				else
				{
					/* high color mode 2nd draw*/

					drawcolor1 = ((color1 & 0x0f) << 4);
					drawcolor2 = ((color2 & 0x0f) << 4);

					if (!vram)
					{
						drawcolor1 |= sailorws_videoworkram0[(dy * Machine->drv->screen_width) + dx1];
						drawcolor2 |= sailorws_videoworkram0[(dy * Machine->drv->screen_width) + dx2];

						drawcolor1 += sailorws_paltbl0[(sailorws_paltblnum * 0x10)];
						drawcolor2 += sailorws_paltbl0[(sailorws_paltblnum * 0x10)];
					}
					else
					{
						drawcolor1 |= sailorws_videoworkram1[(dy * Machine->drv->screen_width) + dx1];
						drawcolor2 |= sailorws_videoworkram1[(dy * Machine->drv->screen_width) + dx2];

						drawcolor1 += sailorws_paltbl1[(sailorws_paltblnum * 0x10)];
						drawcolor2 += sailorws_paltbl1[(sailorws_paltblnum * 0x10)];
					}
				}
			}
			else
			{
				/* normal color mode*/

				if (!vram)
				{
					drawcolor1 = sailorws_paltbl0[(sailorws_paltblnum * 0x10) + color1];
					drawcolor2 = sailorws_paltbl0[(sailorws_paltblnum * 0x10) + color2];
				}
				else
				{
					drawcolor1 = sailorws_paltbl1[(sailorws_paltblnum * 0x10) + color1];
					drawcolor2 = sailorws_paltbl1[(sailorws_paltblnum * 0x10) + color2];
				}
			}

			if (sailorws_transparency[vram])
			{
				tflag1 = (drawcolor1 != 0xff) ? 1 : 0;
				tflag2 = (drawcolor2 != 0xff) ? 1 : 0;
			}
			else
			{
				tflag1 = 1;
				tflag2 = 1;
			}

			drawcolor1 |= (0x100 * vram);
			drawcolor2 |= (0x100 * vram);

			if (!vram)
			{
				if (tflag1)
				{
					sailorws_videoram0[(dy * Machine->drv->screen_width) + dx1] = drawcolor1;
					plot_pixel(sailorws_tmpbitmap0, dx1, dy, Machine->pens[drawcolor1]);
				}
				if (tflag2)
				{
					sailorws_videoram0[(dy * Machine->drv->screen_width) + dx2] = drawcolor2;
					plot_pixel(sailorws_tmpbitmap0, dx2, dy, Machine->pens[drawcolor2]);
				}
			}
			else
			{
				if (tflag1)
				{
					sailorws_videoram1[(dy * Machine->drv->screen_width) + dx1] = drawcolor1;
					plot_pixel(sailorws_tmpbitmap1, dx1, dy, Machine->pens[drawcolor1]);
				}
				if (tflag2)
				{
					sailorws_videoram1[(dy * Machine->drv->screen_width) + dx2] = drawcolor2;
					plot_pixel(sailorws_tmpbitmap1, dx2, dy, Machine->pens[drawcolor2]);
				}
			}
		}
	}

	if (sailorws_highcolor[vram])
	{
		/* NB22090 high color mode*/
		sailorws_radr[vram] = gfxaddr;
	}
}

/******************************************************************************


******************************************************************************/
WRITE_HANDLER( sailorws_gfxflag_0_w ) { sailorws_gfxflag_w(0, offset, data); }
WRITE_HANDLER( sailorws_scrollx_0_w ) { sailorws_scrollx_w(0, offset, data); }
WRITE_HANDLER( sailorws_scrolly_0_w ) { sailorws_scrolly_w(0, offset, data); }
WRITE_HANDLER( sailorws_radr_0_w ) { sailorws_radr_w(0, offset, data); }
WRITE_HANDLER( sailorws_sizex_0_w ) { sailorws_sizex_w(0, offset, data); }
WRITE_HANDLER( sailorws_sizey_0_w ) { sailorws_sizey_w(0, offset, data); }
WRITE_HANDLER( sailorws_drawx_0_w ) { sailorws_drawx_w(0, offset, data); }
WRITE_HANDLER( sailorws_drawy_0_w ) { sailorws_drawy_w(0, offset, data); }

WRITE_HANDLER( sailorws_gfxflag_1_w ) { sailorws_gfxflag_w(1, offset, data); }
WRITE_HANDLER( sailorws_scrollx_1_w ) { sailorws_scrollx_w(1, offset, data); }
WRITE_HANDLER( sailorws_scrolly_1_w ) { sailorws_scrolly_w(1, offset, data); }
WRITE_HANDLER( sailorws_radr_1_w ) { sailorws_radr_w(1, offset, data); }
WRITE_HANDLER( sailorws_sizex_1_w ) { sailorws_sizex_w(1, offset, data); }
WRITE_HANDLER( sailorws_sizey_1_w ) { sailorws_sizey_w(1, offset, data); }
WRITE_HANDLER( sailorws_drawx_1_w ) { sailorws_drawx_w(1, offset, data); }
WRITE_HANDLER( sailorws_drawy_1_w ) { sailorws_drawy_w(1, offset, data); }

READ_HANDLER( sailorws_gfxbusy_0_r ) { return sailorws_gfxbusy_r(0, offset); }
READ_HANDLER( sailorws_gfxbusy_1_r ) { return sailorws_gfxbusy_r(1, offset); }
READ_HANDLER( sailorws_gfxrom_0_r ) { return sailorws_gfxrom_r(0, offset); }
READ_HANDLER( sailorws_gfxrom_1_r ) { return sailorws_gfxrom_r(1, offset); }

/******************************************************************************


******************************************************************************/
VIDEO_START( sailorws )
{
	if ((sailorws_tmpbitmap0 = auto_bitmap_alloc(Machine->drv->screen_width, Machine->drv->screen_height)) == 0) return 1;
	if ((sailorws_tmpbitmap1 = auto_bitmap_alloc(Machine->drv->screen_width, Machine->drv->screen_height)) == 0) return 1;
	if ((sailorws_videoram0 = auto_malloc(Machine->drv->screen_width * Machine->drv->screen_height * sizeof(short))) == 0) return 1;
	if ((sailorws_videoram1 = auto_malloc(Machine->drv->screen_width * Machine->drv->screen_height * sizeof(short))) == 0) return 1;
	if ((sailorws_palette = auto_malloc(0x200 * sizeof(char))) == 0) return 1;
	if ((sailorws_paltbl0 = auto_malloc(0x1000 * sizeof(char))) == 0) return 1;
	if ((sailorws_paltbl1 = auto_malloc(0x1000 * sizeof(char))) == 0) return 1;
	memset(sailorws_videoram0, 0x0000, (Machine->drv->screen_width * Machine->drv->screen_height * sizeof(short)));
	memset(sailorws_videoram1, 0x0000, (Machine->drv->screen_width * Machine->drv->screen_height * sizeof(short)));
#if RASTER_SCROLL
	sailorws_scanline[0] = sailorws_scanline[1] = SCANLINE_MIN;
#endif
	gfxdraw_mode = 1;
	return 0;
}

VIDEO_START( mjkoiura )
{
	if ((sailorws_tmpbitmap0 = auto_bitmap_alloc(Machine->drv->screen_width, Machine->drv->screen_height)) == 0) return 1;
	if ((sailorws_videoram0 = auto_malloc(Machine->drv->screen_width * Machine->drv->screen_height * sizeof(short))) == 0) return 1;
	if ((sailorws_palette = auto_malloc(0x200 * sizeof(char))) == 0) return 1;
	if ((sailorws_paltbl0 = auto_malloc(0x1000 * sizeof(char))) == 0) return 1;
	memset(sailorws_videoram0, 0x0000, (Machine->drv->screen_width * Machine->drv->screen_height * sizeof(short)));
#if RASTER_SCROLL
	sailorws_scanline[0] = sailorws_scanline[1] = SCANLINE_MIN;
#endif
	gfxdraw_mode = 0;
	return 0;
}

VIDEO_START( mscoutm )
{
	if ((sailorws_tmpbitmap0 = auto_bitmap_alloc(Machine->drv->screen_width, Machine->drv->screen_height)) == 0) return 1;
	if ((sailorws_tmpbitmap1 = auto_bitmap_alloc(Machine->drv->screen_width, Machine->drv->screen_height)) == 0) return 1;
	if ((sailorws_videoram0 = auto_malloc(Machine->drv->screen_width * Machine->drv->screen_height * sizeof(short))) == 0) return 1;
	if ((sailorws_videoram1 = auto_malloc(Machine->drv->screen_width * Machine->drv->screen_height * sizeof(short))) == 0) return 1;
	if ((sailorws_videoworkram0 = auto_malloc(Machine->drv->screen_width * Machine->drv->screen_height * sizeof(short))) == 0) return 1;
	if ((sailorws_videoworkram1 = auto_malloc(Machine->drv->screen_width * Machine->drv->screen_height * sizeof(short))) == 0) return 1;
	if ((mscoutm_palette = auto_malloc(0xc00 * sizeof(char))) == 0) return 1;
	if ((sailorws_paltbl0 = auto_malloc(0x1000 * sizeof(char))) == 0) return 1;
	if ((sailorws_paltbl1 = auto_malloc(0x1000 * sizeof(char))) == 0) return 1;
	memset(sailorws_videoram0, 0x0000, (Machine->drv->screen_width * Machine->drv->screen_height * sizeof(short)));
	memset(sailorws_videoram1, 0x0000, (Machine->drv->screen_width * Machine->drv->screen_height * sizeof(short)));
	memset(sailorws_videoworkram0, 0x0000, (Machine->drv->screen_width * Machine->drv->screen_height * sizeof(short)));
	memset(sailorws_videoworkram1, 0x0000, (Machine->drv->screen_width * Machine->drv->screen_height * sizeof(short)));
	gfxdraw_mode = 2;
	return 0;
}

/******************************************************************************


******************************************************************************/
VIDEO_UPDATE( sailorws )
{
	int x, y;
	unsigned short color;

	if (get_vh_global_attribute_changed() || sailorws_screen_refresh)
	{
		sailorws_screen_refresh = 0;

		Machine->pens[0xff] = 0;	/* palette_transparent_pen */

		for (y = 0; y < Machine->drv->screen_height; y++)
		{
			for (x = 0; x < Machine->drv->screen_width; x++)
			{
				color = sailorws_videoram0[(y * Machine->drv->screen_width) + x];
				plot_pixel(sailorws_tmpbitmap0, x, y, Machine->pens[color]);
			}
		}
		if (gfxdraw_mode)
		{
			for (y = 0; y < Machine->drv->screen_height; y++)
			{
				for (x = 0; x < Machine->drv->screen_width; x++)
				{
					color = sailorws_videoram1[(y * Machine->drv->screen_width) + x];
					plot_pixel(sailorws_tmpbitmap1, x, y, Machine->pens[color]);
				}
			}
		}
	}

#if RASTER_SCROLL
	{
		int i;

		for (i = 0; i < 2; i++)
		{
			if (sailorws_flipscreen[i])
			{
				for ( ; sailorws_scanline[i] < SCANLINE_MAX; sailorws_scanline[i]++)
				{
					sailorws_scrollx_raster[i][sailorws_scanline[i]] = sailorws_scrollx[i];
				}
			}
			else
			{
				for ( ; sailorws_scanline[i] < SCANLINE_MAX; sailorws_scanline[i]++)
				{
					sailorws_scrollx_raster[i][(sailorws_scanline[i] ^ 0x1ff)] = sailorws_scrollx[i];
				}
			}

			sailorws_scanline[i] = SCANLINE_MIN;
		}
	}
#endif

	if (sailorws_dispflag[0])
	{
#if RASTER_SCROLL
		copyscrollbitmap(bitmap, sailorws_tmpbitmap0, SCANLINE_MAX, sailorws_scrollx_raster[0], 1, &sailorws_scrolly[0], &Machine->visible_area, TRANSPARENCY_NONE, 0);
#else
		copyscrollbitmap(bitmap, sailorws_tmpbitmap0, 1, &sailorws_scrollx[0], 1, &sailorws_scrolly[0], &Machine->visible_area, TRANSPARENCY_NONE, 0);
#endif
	}
	else
	{
		fillbitmap(bitmap, Machine->pens[0x0ff], 0);
	}

	if (gfxdraw_mode)
	{
		if (sailorws_dispflag[1])
		{
#if RASTER_SCROLL
			copyscrollbitmap(bitmap, sailorws_tmpbitmap1, SCANLINE_MAX, sailorws_scrollx_raster[1], 1, &sailorws_scrolly[1], &Machine->visible_area, TRANSPARENCY_PEN, Machine->pens[0x0ff]);
#else
			copyscrollbitmap(bitmap, sailorws_tmpbitmap1, 1, &sailorws_scrollx[1], 1, &sailorws_scrolly[1], &Machine->visible_area, TRANSPARENCY_PEN, Machine->pens[0x0ff]);
#endif
		}
	}
}

VIDEO_UPDATE( mscoutm )
{
	int x, y;
	unsigned short color;

	if (get_vh_global_attribute_changed() || sailorws_screen_refresh)
	{
		sailorws_screen_refresh = 0;

		Machine->pens[0x0ff] = 0;	/* palette_transparent_pen */
		Machine->pens[0x1ff] = 0;	/* palette_transparent_pen */

		for (y = 0; y < Machine->drv->screen_height; y++)
		{
			for (x = 0; x < Machine->drv->screen_width; x++)
			{
				color = sailorws_videoram0[(y * Machine->drv->screen_width) + x];
				plot_pixel(sailorws_tmpbitmap0, x, y, Machine->pens[color]);
			}
		}
		if (gfxdraw_mode)
		{
			for (y = 0; y < Machine->drv->screen_height; y++)
			{
				for (x = 0; x < Machine->drv->screen_width; x++)
				{
					color = sailorws_videoram1[(y * Machine->drv->screen_width) + x];
					plot_pixel(sailorws_tmpbitmap1, x, y, Machine->pens[color]);
				}
			}
		}
	}

	if (sailorws_dispflag[0])
	{
		copyscrollbitmap(bitmap, sailorws_tmpbitmap0, 1, &sailorws_scrollx[0], 1, &sailorws_scrolly[0], &Machine->visible_area, TRANSPARENCY_NONE, 0);
	}
	else
	{
		fillbitmap(bitmap, Machine->pens[0x0ff], 0);
	}

	if (gfxdraw_mode)
	{
		if (sailorws_dispflag[1])
		{
			copyscrollbitmap(bitmap, sailorws_tmpbitmap1, 1, &sailorws_scrollx[1], 1, &sailorws_scrolly[1], &Machine->visible_area, TRANSPARENCY_PEN, Machine->pens[0x1ff]);
		}
	}
}
