/******************************************************************************

	Video Hardware for Nichibutsu Mahjong series.

	Driver by Takahiro Nogi <nogi@kt.rim.or.jp> 2000/01/28 -

******************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "nb1413m3.h"


static int hyhoo_scrolly;
static int hyhoo_drawx, hyhoo_drawy;
static int hyhoo_sizex, hyhoo_sizey;
static int hyhoo_radrx, hyhoo_radry;
static int hyhoo_gfxrom;
static int hyhoo_gfxflag1;
static int hyhoo_gfxflag2;
static int hyhoo_dispflag;
static int hyhoo_flipscreen;
static int hyhoo_flipx, hyhoo_flipy;
static int hyhoo_screen_refresh;

static struct mame_bitmap *hyhoo_tmpbitmap;
static unsigned short *hyhoo_videoram;
static unsigned short *hyhoo_videoworkram;
static unsigned char *hyhoo_palette;


static void hyhoo_vramflip(void);
static void hyhoo_gfxdraw(void);


/******************************************************************************


******************************************************************************/
PALETTE_INIT( hyhoo )
{
	int i;

	/* initialize 655 RGB lookup */
	for (i = 0; i < 65536; i++)
	{
		int r, g, b;

		/* bbbbbggg_ggrrrrrr*/
		r = ((i >>  0) & 0x3f);
		g = ((i >>  6) & 0x1f);
		b = ((i >> 11) & 0x1f);

		r = ((r << 2) | (r >> 3));
		g = ((g << 3) | (g >> 2));
		b = ((b << 3) | (b >> 2));
		palette_set_color(i,r,g,b);
	}
}

WRITE_HANDLER( hyhoo_palette_w )
{
	hyhoo_palette[offset & 0x0f] = (data ^ 0xff);
}

/******************************************************************************


******************************************************************************/
void hyhoo_radrx_w(int data)
{
	hyhoo_radrx = data;
}

void hyhoo_radry_w(int data)
{
	hyhoo_radry = data;
}

void hyhoo_sizex_w(int data)
{
	hyhoo_sizex = data;
}

void hyhoo_sizey_w(int data)
{
	hyhoo_sizey = data;

	hyhoo_gfxdraw();
}

void hyhoo_gfxflag1_w(int data)
{
	static int hyhoo_flipscreen_old = -1;

	hyhoo_gfxflag1 = data;

	hyhoo_flipx = (data & 0x01) ? 1 : 0;
	hyhoo_flipy = (data & 0x02) ? 1 : 0;
	hyhoo_flipscreen = (data & 0x04) ? 0 : 1;
	hyhoo_dispflag = (data & 0x08) ? 0 : 1;

	if ((nb1413m3_type == NB1413M3_HYHOO) ||
	    (nb1413m3_type == NB1413M3_HYHOO2))
	{
		hyhoo_flipscreen ^= 1;
	}

	if (hyhoo_flipscreen != hyhoo_flipscreen_old)
	{
		hyhoo_vramflip();
		hyhoo_screen_refresh = 1;
		hyhoo_flipscreen_old = hyhoo_flipscreen;
	}
}

void hyhoo_gfxflag2_w(int data)
{
	hyhoo_gfxflag2 = data;
}

void hyhoo_drawx_w(int data)
{
	hyhoo_drawx = (data ^ 0xff);
}

void hyhoo_drawy_w(int data)
{
	hyhoo_drawy = (data ^ 0xff);

	if (hyhoo_flipscreen) hyhoo_scrolly = -2;
	else hyhoo_scrolly = 0;
}

void hyhoo_romsel_w(int data)
{
	hyhoo_gfxrom = (((data & 0xc0) >> 4) + (data & 0x03));

	if ((hyhoo_gfxrom << 17) > (memory_region_length(REGION_GFX1) - 1))
	{
#ifdef MAME_DEBUG
		usrintf_showmessage("GFXROM BANK OVER!!");
#endif
		hyhoo_gfxrom = 0;
	}
}

/******************************************************************************


******************************************************************************/
void hyhoo_vramflip(void)
{
	int x, y;
	unsigned short color1, color2;

	for (y = 0; y < (Machine->drv->screen_height / 2); y++)
	{
		for (x = 0; x < Machine->drv->screen_width; x++)
		{
			color1 = hyhoo_videoram[(y * Machine->drv->screen_width) + x];
			color2 = hyhoo_videoram[((y ^ 0xff) * Machine->drv->screen_width) + (x ^ 0x1ff)];
			hyhoo_videoram[(y * Machine->drv->screen_width) + x] = color2;
			hyhoo_videoram[((y ^ 0xff) * Machine->drv->screen_width) + (x ^ 0x1ff)] = color1;

			color1 = hyhoo_videoworkram[(y * Machine->drv->screen_width) + x];
			color2 = hyhoo_videoworkram[((y ^ 0xff) * Machine->drv->screen_width) + (x ^ 0x1ff)];
			hyhoo_videoworkram[(y * Machine->drv->screen_width) + x] = color2;
			hyhoo_videoworkram[((y ^ 0xff) * Machine->drv->screen_width) + (x ^ 0x1ff)] = color1;
		}
	}
}

void hyhoo_gfxdraw(void)
{
	unsigned char *GFX = memory_region(REGION_GFX1);

	int x, y;
	int dx1, dx2, dy;
	int startx, starty;
	int sizex, sizey;
	int skipx, skipy;
	int ctrx, ctry;
	int tflag1, tflag2;
	int gfxaddr;
	unsigned short r, g, b;
	unsigned short color, color1, color2;
	unsigned short drawcolor1, drawcolor2;

	hyhoo_gfxrom |= ((nb1413m3_sndrombank1 & 0x02) << 3);

	if (hyhoo_flipx)
	{
		hyhoo_drawx -= (hyhoo_sizex << 1);
		startx = hyhoo_sizex;
		sizex = ((hyhoo_sizex ^ 0xff) + 1);
		skipx = -1;
	}
	else
	{
		hyhoo_drawx = (hyhoo_drawx - hyhoo_sizex);
		startx = 0;
		sizex = (hyhoo_sizex + 1);
		skipx = 1;
	}

	if (hyhoo_flipy)
	{
		hyhoo_drawy -= ((hyhoo_sizey << 1) + 1);
		starty = hyhoo_sizey;
		sizey = ((hyhoo_sizey ^ 0xff) + 1);
		skipy = -1;
	}
	else
	{
		hyhoo_drawy = (hyhoo_drawy - hyhoo_sizey - 1);
		starty = 0;
		sizey = (hyhoo_sizey + 1);
		skipy = 1;
	}

	gfxaddr = ((hyhoo_gfxrom << 17) + (hyhoo_radry << 9) + (hyhoo_radrx << 1));

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

			if (hyhoo_flipscreen)
			{
				dx1 = (((((hyhoo_drawx + x) * 2) + 0) ^ 0x1ff) & 0x1ff);
				dx2 = (((((hyhoo_drawx + x) * 2) + 1) ^ 0x1ff) & 0x1ff);
				dy = (((hyhoo_drawy + y) ^ 0xff) & 0xff);
			}
			else
			{
				dx1 = ((((hyhoo_drawx + x) * 2) + 0) & 0x1ff);
				dx2 = ((((hyhoo_drawx + x) * 2) + 1) & 0x1ff);
				dy = ((hyhoo_drawy + y) & 0xff);
			}

			if (hyhoo_gfxflag2 & 0x04)
			{
				/* 65536 colors mode*/

				if (hyhoo_gfxflag2 & 0x20)
				{
					/* 65536 colors (lower)*/

					/* src xxxxxxxx_bbbggrrr*/
					/* dst xxbbbxxx_ggxxxrrr*/
					r = (((color & 0x07) >> 0) & 0x07);
					g = (((color & 0x18) >> 3) & 0x03);
					b = (((color & 0xe0) >> 5) & 0x07);
					drawcolor1 = drawcolor2 = ((b << (11 + 0)) | (g << (6 + 0)) | (r << (0 + 0)));

					drawcolor1 |= hyhoo_videoworkram[(dy * Machine->drv->screen_width) + dx1];
					drawcolor2 |= hyhoo_videoworkram[(dy * Machine->drv->screen_width) + dx2];

					tflag1 = (drawcolor1 != 0xffff) ? 1 : 0;
					tflag2 = (drawcolor2 != 0xffff) ? 1 : 0;
				}
				else
				{
					/* 65536 colors (higher)*/

					tflag1 = tflag2 = 1;	/* dummy*/

					/* src xxxxxxxx_bbgggrrr*/
					/* dst bbxxxggg_xxrrrxxx*/
					r = (((color & 0x07) >> 0) & 0x07);
					g = (((color & 0x38) >> 3) & 0x07);
					b = (((color & 0xc0) >> 6) & 0x03);
					drawcolor1 = drawcolor2 = ((b << (11 + 3)) | (g << (6 + 2)) | (r << (0 + 3)));

					hyhoo_videoworkram[(dy * Machine->drv->screen_width) + dx1] = drawcolor1;
					hyhoo_videoworkram[(dy * Machine->drv->screen_width) + dx2] = drawcolor2;

					continue;
				}
			}
			else
			{
				/* Palettized picture mode*/

				if (hyhoo_flipx)
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

				tflag1 = (hyhoo_palette[color1] != 0xff) ? 1 : 0;
				tflag2 = (hyhoo_palette[color2] != 0xff) ? 1 : 0;

				/* src xxxxxxxx_bbgggrrr*/
				/* dst bbxxxggg_xxrrrxxx*/

				r = (hyhoo_palette[color1] & 0x07) >> 0;
				g = (hyhoo_palette[color1] & 0x38) >> 3;
				b = (hyhoo_palette[color1] & 0xc0) >> 6;

				drawcolor1 = ((b << (11 + 3)) | (g << (6 + 2)) | (r << (0 + 3)));

				/* src xxxxxxxx_bbgggrrr*/
				/* dst bbxxxggg_xxrrrxxx*/

				r = (hyhoo_palette[color2] & 0x07) >> 0;
				g = (hyhoo_palette[color2] & 0x38) >> 3;
				b = (hyhoo_palette[color2] & 0xc0) >> 6;

				drawcolor2 = ((b << (11 + 3)) | (g << (6 + 2)) | (r << (0 + 3)));
			}

			nb1413m3_busyctr++;

			if (tflag1)
			{
				hyhoo_videoram[(dy * Machine->drv->screen_width) + dx1] = drawcolor1;
				plot_pixel(hyhoo_tmpbitmap, dx1, dy, Machine->pens[drawcolor1]);
			}
			if (tflag2)
			{
				hyhoo_videoram[(dy * Machine->drv->screen_width) + dx2] = drawcolor2;
				plot_pixel(hyhoo_tmpbitmap, dx2, dy, Machine->pens[drawcolor2]);
			}

			nb1413m3_busyctr++;
		}
	}

	nb1413m3_busyflag = (nb1413m3_busyctr > 10000) ? 0 : 1;
}

/******************************************************************************


******************************************************************************/
VIDEO_START( hyhoo )
{
	if ((hyhoo_tmpbitmap = auto_bitmap_alloc(Machine->drv->screen_width, Machine->drv->screen_height)) == 0) return 1;
	if ((hyhoo_videoram = auto_malloc(Machine->drv->screen_width * Machine->drv->screen_height * sizeof(short))) == 0) return 1;
	if ((hyhoo_videoworkram = auto_malloc(Machine->drv->screen_width * Machine->drv->screen_height * sizeof(short))) == 0) return 1;
	if ((hyhoo_palette = auto_malloc(0x10 * sizeof(char))) == 0) return 1;
	memset(hyhoo_videoram, 0x0000, (Machine->drv->screen_width * Machine->drv->screen_height * sizeof(short)));
	return 0;
}

/******************************************************************************


******************************************************************************/
VIDEO_UPDATE( hyhoo )
{
	int x, y;
	unsigned short color;

	if (get_vh_global_attribute_changed() || hyhoo_screen_refresh)
	{
		hyhoo_screen_refresh = 0;
		for (y = 0; y < Machine->drv->screen_height; y++)
		{
			for (x = 0; x < Machine->drv->screen_width; x++)
			{
				color = hyhoo_videoram[(y * Machine->drv->screen_width) + x];
				plot_pixel(hyhoo_tmpbitmap, x, y, Machine->pens[color]);
			}
		}
	}

	if (hyhoo_dispflag)
	{
		copyscrollbitmap(bitmap, hyhoo_tmpbitmap, 0, 0, 1, &hyhoo_scrolly, &Machine->visible_area, TRANSPARENCY_NONE, 0);
	}
	else
	{
		fillbitmap(bitmap, Machine->pens[0x0000], 0);
	}
}
