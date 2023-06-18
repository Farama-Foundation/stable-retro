/******************************************************************************

	Video Hardware for Nichibutsu Mahjong series.

	Driver by Takahiro Nogi <nogi@kt.rim.or.jp> 1999/11/05 -

******************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "nb1413m3.h"


static int pstadium_scrollx, pstadium_scrollx1, pstadium_scrollx2;
static int pstadium_scrolly, pstadium_scrolly1, pstadium_scrolly2;
static int pstadium_drawx, pstadium_drawx1, pstadium_drawx2;
static int pstadium_drawy, pstadium_drawy1, pstadium_drawy2;
static int pstadium_sizex, pstadium_sizey;
static int pstadium_radrx, pstadium_radry;
static int pstadium_gfxrom;
static int pstadium_dispflag;
static int pstadium_flipscreen;
static int pstadium_flipx, pstadium_flipy;
static int pstadium_paltblnum;
static int pstadium_screen_refresh;

static struct mame_bitmap *pstadium_tmpbitmap;
static unsigned char *pstadium_videoram;
static unsigned char *pstadium_paltbl;


static void pstadium_vramflip(void);
static void pstadium_gfxdraw(void);


/******************************************************************************


******************************************************************************/

WRITE_HANDLER( pstadium_palette_w )
{
	int r, g, b;

	paletteram[offset] = data;

	if (!(offset & 1)) return;

	offset &= 0x1fe;

	r = ((paletteram[offset + 1] & 0x0f) << 4);
	g = ((paletteram[offset + 0] & 0xf0) << 0);
	b = ((paletteram[offset + 0] & 0x0f) << 4);

	r = (r | (r >> 4));
	g = (g | (g >> 4));
	b = (b | (b >> 4));

	palette_set_color((offset >> 1), r, g, b);
}

WRITE_HANDLER( galkoku_palette_w )
{
	int r, g, b;

	paletteram[offset] = data;

	if (!(offset & 1)) return;

	offset &= 0x1fe;

	r = ((paletteram[offset + 0] & 0x0f) << 4);
	g = ((paletteram[offset + 1] & 0xf0) << 0);
	b = ((paletteram[offset + 1] & 0x0f) << 4);

	r = (r | (r >> 4));
	g = (g | (g >> 4));
	b = (b | (b >> 4));

	palette_set_color((offset >> 1), r, g, b);
}

WRITE_HANDLER( galkaika_palette_w )
{
	int r, g, b;

	paletteram[offset] = data;

	if (!(offset & 1)) return;

	offset &= 0x1fe;

	r = ((paletteram[offset + 0] & 0x7c) >> 2);
	g = (((paletteram[offset + 0] & 0x03) << 3) | ((paletteram[offset + 1] & 0xe0) >> 5));
	b = ((paletteram[offset + 1] & 0x1f) >> 0);

	r = ((r << 3) | (r >> 2));
	g = ((g << 3) | (g >> 2));
	b = ((b << 3) | (b >> 2));

	palette_set_color((offset / 2), r, g, b);
}

/******************************************************************************


******************************************************************************/
static void pstadium_calc_scrollx(void)
{
	pstadium_scrollx = ((((pstadium_scrollx2 + pstadium_scrollx1) ^ 0x1ff) & 0x1ff) << 1);
}

static void pstadium_calc_scrolly(void)
{
	if (pstadium_flipscreen) pstadium_scrolly = (((pstadium_scrolly2 + pstadium_scrolly1 - 0xf0) ^ 0x1ff) & 0x1ff);
	else pstadium_scrolly = (((pstadium_scrolly2 + pstadium_scrolly1 + 1) - 0x10) & 0x1ff);
}

static void pstadium_calc_drawx(void)
{
	pstadium_drawx = ((pstadium_drawx2 + pstadium_drawx1) ^ 0x1ff) & 0x1ff;
}

static void pstadium_calc_drawy(void)
{
	pstadium_drawy = ((pstadium_drawy2 + pstadium_drawy1) ^ 0x1ff) & 0x1ff;
}

void pstadium_radrx_w(int data)
{
	pstadium_radrx = data;
}

void pstadium_radry_w(int data)
{
	pstadium_radry = data;
}

void pstadium_sizex_w(int data)
{
	pstadium_sizex = data;
}

void pstadium_sizey_w(int data)
{
	pstadium_sizey = data;

	pstadium_gfxdraw();
}

void pstadium_gfxflag_w(int data)
{
	static int pstadium_flipscreen_old = -1;

	pstadium_flipx = (data & 0x01) ? 1 : 0;
	pstadium_flipy = (data & 0x02) ? 1 : 0;
	pstadium_flipscreen = (data & 0x04) ? 0 : 1;
	pstadium_dispflag = (data & 0x10) ? 0 : 1;

	if (pstadium_flipscreen != pstadium_flipscreen_old)
	{
		pstadium_vramflip();
		pstadium_screen_refresh = 1;
		pstadium_flipscreen_old = pstadium_flipscreen;
	}
}

void pstadium_gfxflag2_w(int data)
{
	pstadium_drawx2 = (((data & 0x01) >> 0) << 8);
	pstadium_drawy2 = (((data & 0x02) >> 1) << 8);
	pstadium_scrollx2 = (((data & 0x04) >> 2) << 8);
	pstadium_scrolly2 = (((data & 0x08) >> 3) << 8);
}

void pstadium_drawx_w(int data)
{
	pstadium_drawx1 = data;
}

void pstadium_drawy_w(int data)
{
	pstadium_drawy1 = data;
}

void pstadium_scrollx_w(int data)
{
	pstadium_scrollx1 = data;
}

void pstadium_scrolly_w(int data)
{
	pstadium_scrolly1 = data;
}

void pstadium_romsel_w(int data)
{
	pstadium_gfxrom = data;

	if ((0x20000 * pstadium_gfxrom) > (memory_region_length(REGION_GFX1) - 1))
	{
#ifdef MAME_DEBUG
		usrintf_showmessage("GFXROM BANK OVER!!");
#endif
		pstadium_gfxrom = 0;
	}
}

void pstadium_paltblnum_w(int data)
{
	pstadium_paltblnum = data;
}

READ_HANDLER( pstadium_paltbl_r )
{
	return pstadium_paltbl[offset];
}

WRITE_HANDLER( pstadium_paltbl_w )
{
	pstadium_paltbl[((pstadium_paltblnum & 0x7f) * 0x10) + (offset & 0x0f)] = data;
}

/******************************************************************************


******************************************************************************/
static void pstadium_vramflip(void)
{
	int x, y;
	unsigned char color1, color2;

	for (y = 0; y < (Machine->drv->screen_height / 2); y++)
	{
		for (x = 0; x < Machine->drv->screen_width; x++)
		{
			color1 = pstadium_videoram[(y * Machine->drv->screen_width) + x];
			color2 = pstadium_videoram[((y ^ 0x1ff) * Machine->drv->screen_width) + (x ^ 0x3ff)];

			pstadium_videoram[(y * Machine->drv->screen_width) + x] = color2;
			pstadium_videoram[((y ^ 0x1ff) * Machine->drv->screen_width) + (x ^ 0x3ff)] = color1;
		}
	}
}

static void pstadium_gfxdraw(void)
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
	unsigned char drawcolor1, drawcolor2;
	int gfxaddr;

	pstadium_calc_drawx();
	pstadium_calc_drawy();

	if (pstadium_flipx)
	{
		pstadium_drawx -= pstadium_sizex;
		startx = pstadium_sizex;
		sizex = ((pstadium_sizex ^ 0xff) + 1);
		skipx = -1;
	}
	else
	{
		pstadium_drawx = (pstadium_drawx - pstadium_sizex);
		startx = 0;
		sizex = (pstadium_sizex + 1);
		skipx = 1;
	}

	if (pstadium_flipy)
	{
		pstadium_drawy -= (pstadium_sizey + 1);
		starty = pstadium_sizey;
		sizey = ((pstadium_sizey ^ 0xff) + 1);
		skipy = -1;
	}
	else
	{
		pstadium_drawy = (pstadium_drawy - pstadium_sizey - 1);
		starty = 0;
		sizey = (pstadium_sizey + 1);
		skipy = 1;
	}

	gfxaddr = ((pstadium_gfxrom << 17) + (pstadium_radry << 9) + (pstadium_radrx << 1));

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

			if (pstadium_flipscreen)
			{
				dx1 = (((((pstadium_drawx + x) * 2) + 0) ^ 0x3ff) & 0x3ff);
				dx2 = (((((pstadium_drawx + x) * 2) + 1) ^ 0x3ff) & 0x3ff);
				dy = (((pstadium_drawy + y) ^ 0x1ff) & 0x1ff);
			}
			else
			{
				dx1 = ((((pstadium_drawx + x) * 2) + 0) & 0x3ff);
				dx2 = ((((pstadium_drawx + x) * 2) + 1) & 0x3ff);
				dy = ((pstadium_drawy + y) & 0x1ff);
			}

			if (pstadium_flipx)
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

			drawcolor1 = pstadium_paltbl[((pstadium_paltblnum & 0x7f) * 0x10) + color1];
			drawcolor2 = pstadium_paltbl[((pstadium_paltblnum & 0x7f) * 0x10) + color2];

			tflag1 = (drawcolor1 != 0xff) ? 1 : 0;
			tflag2 = (drawcolor2 != 0xff) ? 1 : 0;

			nb1413m3_busyctr++;

			if (tflag1)
			{
				pstadium_videoram[(dy * Machine->drv->screen_width) + dx1] = drawcolor1;
				plot_pixel(pstadium_tmpbitmap, dx1, dy, Machine->pens[drawcolor1]);
			}
			if (tflag2)
			{
				pstadium_videoram[(dy * Machine->drv->screen_width) + dx2] = drawcolor2;
				plot_pixel(pstadium_tmpbitmap, dx2, dy, Machine->pens[drawcolor2]);
			}
		}
	}

	nb1413m3_busyflag = (nb1413m3_busyctr > 7500) ? 0 : 1;

}

/******************************************************************************


******************************************************************************/
VIDEO_START( pstadium )
{
	if ((pstadium_tmpbitmap = auto_bitmap_alloc(Machine->drv->screen_width, Machine->drv->screen_height)) == 0) return 1;
	if ((pstadium_videoram = auto_malloc(Machine->drv->screen_width * Machine->drv->screen_height * sizeof(char))) == 0) return 1;
	if ((pstadium_paltbl = auto_malloc(0x800 * sizeof(char))) == 0) return 1;
	memset(pstadium_videoram, 0x00, (Machine->drv->screen_width * Machine->drv->screen_height * sizeof(char)));
	return 0;
}

VIDEO_UPDATE( pstadium )
{
	int x, y;
	int color;

	if (get_vh_global_attribute_changed() || pstadium_screen_refresh)
	{
		pstadium_screen_refresh = 0;

		for (y = 0; y < Machine->drv->screen_height; y++)
		{
			for (x = 0; x < Machine->drv->screen_width; x++)
			{
				color = pstadium_videoram[(y * Machine->drv->screen_width) + x];
				plot_pixel(pstadium_tmpbitmap, x, y, Machine->pens[color]);
			}
		}
	}

	pstadium_calc_scrollx();
	pstadium_calc_scrolly();

	if (nb1413m3_inputport & 0x20)
	{
		copyscrollbitmap(bitmap, pstadium_tmpbitmap, 1, &pstadium_scrollx, 1, &pstadium_scrolly, &Machine->visible_area, TRANSPARENCY_NONE, 0);
	}
	else
	{
		fillbitmap(bitmap, Machine->pens[0x00], 0);
	}
}

VIDEO_UPDATE( galkoku )
{
	int x, y;
	int color;

	if (get_vh_global_attribute_changed() || pstadium_screen_refresh)
	{
		pstadium_screen_refresh = 0;

		for (y = 0; y < Machine->drv->screen_height; y++)
		{
			for (x = 0; x < Machine->drv->screen_width; x++)
			{
				color = pstadium_videoram[(y * Machine->drv->screen_width) + x];
				plot_pixel(pstadium_tmpbitmap, x, y, Machine->pens[color]);
			}
		}
	}

	pstadium_calc_scrollx();
	pstadium_calc_scrolly();

	if (pstadium_dispflag)
	{
		copyscrollbitmap(bitmap, pstadium_tmpbitmap, 1, &pstadium_scrollx, 1, &pstadium_scrolly, &Machine->visible_area, TRANSPARENCY_NONE, 0);
	}
	else
	{
		fillbitmap(bitmap, Machine->pens[0x00], 0);
	}
}
