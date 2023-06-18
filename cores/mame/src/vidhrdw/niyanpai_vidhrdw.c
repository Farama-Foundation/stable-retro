/******************************************************************************

	Video Hardware for Nichibutsu Mahjong series.

	Driver by Takahiro Nogi <nogi@kt.rim.or.jp> 2000/12/23 -

******************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


#define	VRAM_MAX	3


static int niyanpai_scrollx[VRAM_MAX], niyanpai_scrolly[VRAM_MAX];
static unsigned char niyanpai_scrollx_tmp[VRAM_MAX][2], niyanpai_scrolly_tmp[VRAM_MAX][2];
static int niyanpai_drawx[VRAM_MAX], niyanpai_drawy[VRAM_MAX];
static unsigned char niyanpai_drawx_tmp[VRAM_MAX][2], niyanpai_drawy_tmp[VRAM_MAX][2];
static int niyanpai_sizex[VRAM_MAX], niyanpai_sizey[VRAM_MAX];
static int niyanpai_radr[VRAM_MAX];
static unsigned char niyanpai_radr_tmp[VRAM_MAX][3];
static int niyanpai_gfxflag[VRAM_MAX];
static int niyanpai_dispflag[VRAM_MAX];
static int niyanpai_flipscreen[VRAM_MAX];
static int niyanpai_highcolor[VRAM_MAX];
static int niyanpai_transparency[VRAM_MAX];
static int niyanpai_flipx[VRAM_MAX], niyanpai_flipy[VRAM_MAX];
static int niyanpai_paltblnum[VRAM_MAX];
static int niyanpai_screen_refresh;

static struct mame_bitmap *niyanpai_tmpbitmap0, *niyanpai_tmpbitmap1, *niyanpai_tmpbitmap2;
static unsigned short *niyanpai_videoram0, *niyanpai_videoram1, *niyanpai_videoram2;
static unsigned short *niyanpai_palette;
static unsigned char *niyanpai_paltbl0, *niyanpai_paltbl1, *niyanpai_paltbl2;


static void niyanpai_vramflip(int vram);
static void niyanpai_gfxdraw(int vram);


/******************************************************************************


******************************************************************************/
READ16_HANDLER( niyanpai_palette_r )
{
	return niyanpai_palette[offset];
}

WRITE16_HANDLER( niyanpai_palette_w )
{
	int r, g, b;
	int offs_h, offs_l;
	data16_t oldword = niyanpai_palette[offset];
	data16_t newword;

	COMBINE_DATA(&niyanpai_palette[offset]);
	newword = niyanpai_palette[offset];

	if (oldword != newword)
	{
		offs_h = (offset / 0x180);
		offs_l = (offset & 0x7f);

		if (ACCESSING_MSB16)
		{
			r  = ((niyanpai_palette[(0x000 + (offs_h * 0x180) + offs_l)] & 0xff00) >> 8);
			g  = ((niyanpai_palette[(0x080 + (offs_h * 0x180) + offs_l)] & 0xff00) >> 8);
			b  = ((niyanpai_palette[(0x100 + (offs_h * 0x180) + offs_l)] & 0xff00) >> 8);

			palette_set_color(((offs_h << 8) + (offs_l << 1) + 0), r, g, b);
		}

		if (ACCESSING_LSB16)
		{
			r  = ((niyanpai_palette[(0x000 + (offs_h * 0x180) + offs_l)] & 0x00ff) >> 0);
			g  = ((niyanpai_palette[(0x080 + (offs_h * 0x180) + offs_l)] & 0x00ff) >> 0);
			b  = ((niyanpai_palette[(0x100 + (offs_h * 0x180) + offs_l)] & 0x00ff) >> 0);

			palette_set_color(((offs_h << 8) + (offs_l << 1) + 1), r, g, b);
		}
	}
}

/******************************************************************************


******************************************************************************/
int niyanpai_gfxbusy_r(int vram, int offset)
{
	return 0xfffe;
}

void niyanpai_scrollx_w(int vram, int offset, int data)
{
	niyanpai_scrollx_tmp[vram][offset] = data;

	if (offset)
	{
		niyanpai_scrollx[vram] = -((((niyanpai_scrollx_tmp[vram][0] + (niyanpai_scrollx_tmp[vram][1] << 8)) & 0x1ff) + 0x4e) << 1);
	}
}

void niyanpai_scrolly_w(int vram, int offset, int data)
{
	niyanpai_scrolly_tmp[vram][offset] = data;

	if (offset)
	{
		if (niyanpai_flipscreen[vram]) niyanpai_scrolly[vram] = ((niyanpai_scrolly_tmp[vram][0] + (niyanpai_scrolly_tmp[vram][1] << 8)) ^ 0x1ff) & 0x1ff;
		else niyanpai_scrolly[vram] = (niyanpai_scrolly_tmp[vram][0] + (niyanpai_scrolly_tmp[vram][1] << 8) + 1) & 0x1ff;
	}
}

void niyanpai_radr_w(int vram, int offset, int data)
{
	niyanpai_radr_tmp[vram][offset] = data;

	if (!offset)
	{
		niyanpai_radr[vram] = (niyanpai_radr_tmp[vram][0] + (niyanpai_radr_tmp[vram][1] << 8) + (niyanpai_radr_tmp[vram][2] << 16));
	}
}

void niyanpai_gfxflag_w(int vram, int offset, int data)
{
	static int niyanpai_flipscreen_old[VRAM_MAX] = { -1, -1, -1 };

	niyanpai_gfxflag[vram] = data;

	niyanpai_flipx[vram] = (data & 0x01) ? 1 : 0;
	niyanpai_flipy[vram] = (data & 0x02) ? 1 : 0;
	niyanpai_highcolor[vram] = (data & 0x04) ? 1 : 0;
/*	if (data & 0x08) usrintf_showmessage("Unknown GFX Flag!! (0x08)");*/
	niyanpai_transparency[vram] = (data & 0x10) ? 1 : 0;
/*	if (data & 0x20) usrintf_showmessage("Unknown GFX Flag!! (0x20)");*/
	niyanpai_flipscreen[vram] = (data & 0x40) ? 0 : 1;
	niyanpai_dispflag[vram] = (data & 0x80) ? 1 : 0;

	if (niyanpai_flipscreen[vram] != niyanpai_flipscreen_old[vram])
	{
		niyanpai_screen_refresh = 1;
		niyanpai_flipscreen_old[vram] = niyanpai_flipscreen[vram];
		niyanpai_vramflip(vram);
	}
}

void niyanpai_sizex_w(int vram, int offset, int data)
{
	niyanpai_sizex[vram] = data;
}

void niyanpai_sizey_w(int vram, int offset, int data)
{
	niyanpai_sizey[vram] = data;
}

void niyanpai_drawx_w(int vram, int offset, int data)
{
	niyanpai_drawx_tmp[vram][offset] = data;

	if (offset)
	{
		niyanpai_drawx[vram] = ((niyanpai_drawx_tmp[vram][0] + (niyanpai_drawx_tmp[vram][1] << 8)) ^ 0x3ff) & 0x3ff;
	}
}

void niyanpai_drawy_w(int vram, int offset, int data)
{
	niyanpai_drawy_tmp[vram][offset] = data;

	if (offset)
	{
		niyanpai_drawy[vram] = ((niyanpai_drawy_tmp[vram][0] + (niyanpai_drawy_tmp[vram][1] << 8)) ^ 0x1ff) & 0x1ff;

		niyanpai_gfxdraw(vram);
	}
}

WRITE16_HANDLER( niyanpai_paltblnum_0_w )
{
	niyanpai_paltblnum[0] = data;
}

WRITE16_HANDLER( niyanpai_paltblnum_1_w )
{
	niyanpai_paltblnum[1] = data;
}

WRITE16_HANDLER( niyanpai_paltblnum_2_w )
{
	niyanpai_paltblnum[2] = data;
}

WRITE16_HANDLER( niyanpai_paltbl_0_w )
{
	niyanpai_paltbl0[((niyanpai_paltblnum[0] & 0x00ff) * 0x10) + (offset & 0x0f)] = data;
}

WRITE16_HANDLER( niyanpai_paltbl_1_w )
{
	niyanpai_paltbl1[((niyanpai_paltblnum[1] & 0x00ff) * 0x10) + (offset & 0x0f)] = data;
}

WRITE16_HANDLER( niyanpai_paltbl_2_w )
{
	niyanpai_paltbl2[((niyanpai_paltblnum[2] & 0x00ff) * 0x10) + (offset & 0x0f)] = data;
}

int niyanpai_gfxrom_r(int vram, int offset)
{
	unsigned char *GFXROM = memory_region(REGION_GFX1);

	return (unsigned short)GFXROM[niyanpai_radr[vram]];
}

/******************************************************************************


******************************************************************************/
static void niyanpai_vramflip(int vram)
{
	int x, y;
	unsigned short color1, color2;
	unsigned short *vidram;

	switch (vram)
	{
		case	0:
			vidram = niyanpai_videoram0;
			break;
		case	1:
			vidram = niyanpai_videoram1;
			break;
		case	2:
			vidram = niyanpai_videoram2;
			break;
		default:
			vidram = niyanpai_videoram0;
			break;
	}

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

static void niyanpai_gfxdraw(int vram)
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
	unsigned short color, color1, color2;
	unsigned short drawcolor1, drawcolor2;

	if (niyanpai_flipx[vram])
	{
		niyanpai_drawx[vram] -= niyanpai_sizex[vram];
		startx = niyanpai_sizex[vram];
		sizex = (niyanpai_sizex[vram] + 1);
		skipx = -1;
	}
	else
	{
		niyanpai_drawx[vram] = (niyanpai_drawx[vram] - niyanpai_sizex[vram]);
		startx = 0;
		sizex = (niyanpai_sizex[vram] + 1);
		skipx = 1;
	}

	if (niyanpai_flipy[vram])
	{
		niyanpai_drawy[vram] -= (niyanpai_sizey[vram] + 1);
		starty = niyanpai_sizey[vram];
		sizey = (niyanpai_sizey[vram] + 1);
		skipy = -1;
	}
	else
	{
		niyanpai_drawy[vram] = (niyanpai_drawy[vram] - niyanpai_sizey[vram] - 1);
		starty = 0;
		sizey = (niyanpai_sizey[vram] + 1);
		skipy = 1;
	}

	gfxaddr = ((niyanpai_radr[vram] + 2) & 0x00ffffff);

	Machine->pens[0x0ff] = 0;	/* palette_transparent_pen */
	Machine->pens[0x1ff] = 0;	/* palette_transparent_pen */
	Machine->pens[0x2ff] = 0;	/* palette_transparent_pen */

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

			if (niyanpai_flipscreen[vram])
			{
				dx1 = (((((niyanpai_drawx[vram] + x) * 2) + 0) ^ 0x3ff) & 0x3ff);
				dx2 = (((((niyanpai_drawx[vram] + x) * 2) + 1) ^ 0x3ff) & 0x3ff);
				dy = (((niyanpai_drawy[vram] + y) ^ 0x1ff) & 0x1ff);
			}
			else
			{
				dx1 = ((((niyanpai_drawx[vram] + x) * 2) + 0) & 0x3ff);
				dx2 = ((((niyanpai_drawx[vram] + x) * 2) + 1) & 0x3ff);
				dy = ((niyanpai_drawy[vram] + y) & 0x1ff);
			}

			if (niyanpai_flipx[vram])
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

			if (vram == 0)
			{
				drawcolor1 = niyanpai_paltbl0[(niyanpai_paltblnum[0] * 0x10) + color1];
				drawcolor2 = niyanpai_paltbl0[(niyanpai_paltblnum[0] * 0x10) + color2];
			}
			else if (vram == 1)
			{
				drawcolor1 = niyanpai_paltbl1[(niyanpai_paltblnum[1] * 0x10) + color1];
				drawcolor2 = niyanpai_paltbl1[(niyanpai_paltblnum[1] * 0x10) + color2];
			}
			else
			{
				drawcolor1 = niyanpai_paltbl2[(niyanpai_paltblnum[2] * 0x10) + color1];
				drawcolor2 = niyanpai_paltbl2[(niyanpai_paltblnum[2] * 0x10) + color2];
			}

			if (niyanpai_transparency[vram])
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

			if (vram == 0)
			{
				if (tflag1)
				{
					niyanpai_videoram0[(dy * Machine->drv->screen_width) + dx1] = drawcolor1;
					plot_pixel(niyanpai_tmpbitmap0, dx1, dy, Machine->pens[drawcolor1]);
				}
				if (tflag2)
				{
					niyanpai_videoram0[(dy * Machine->drv->screen_width) + dx2] = drawcolor2;
					plot_pixel(niyanpai_tmpbitmap0, dx2, dy, Machine->pens[drawcolor2]);
				}
			}
			else if (vram == 1)
			{
				if (tflag1)
				{
					niyanpai_videoram1[(dy * Machine->drv->screen_width) + dx1] = drawcolor1;
					plot_pixel(niyanpai_tmpbitmap1, dx1, dy, Machine->pens[drawcolor1]);
				}
				if (tflag2)
				{
					niyanpai_videoram1[(dy * Machine->drv->screen_width) + dx2] = drawcolor2;
					plot_pixel(niyanpai_tmpbitmap1, dx2, dy, Machine->pens[drawcolor2]);
				}
			}
			else
			{
				if (tflag1)
				{
					niyanpai_videoram2[(dy * Machine->drv->screen_width) + dx1] = drawcolor1;
					plot_pixel(niyanpai_tmpbitmap2, dx1, dy, Machine->pens[drawcolor1]);
				}
				if (tflag2)
				{
					niyanpai_videoram2[(dy * Machine->drv->screen_width) + dx2] = drawcolor2;
					plot_pixel(niyanpai_tmpbitmap2, dx2, dy, Machine->pens[drawcolor2]);
				}
			}
		}
	}
}

/******************************************************************************


******************************************************************************/
WRITE16_HANDLER( niyanpai_gfxflag_0_w ) { niyanpai_gfxflag_w(0, offset, data); }
WRITE16_HANDLER( niyanpai_scrollx_0_w ) { niyanpai_scrollx_w(0, offset, data); }
WRITE16_HANDLER( niyanpai_scrolly_0_w ) { niyanpai_scrolly_w(0, offset, data); }
WRITE16_HANDLER( niyanpai_radr_0_w ) { niyanpai_radr_w(0, offset, data); }
WRITE16_HANDLER( niyanpai_sizex_0_w ) { niyanpai_sizex_w(0, offset, data); }
WRITE16_HANDLER( niyanpai_sizey_0_w ) { niyanpai_sizey_w(0, offset, data); }
WRITE16_HANDLER( niyanpai_drawx_0_w ) { niyanpai_drawx_w(0, offset, data); }
WRITE16_HANDLER( niyanpai_drawy_0_w ) { niyanpai_drawy_w(0, offset, data); }

WRITE16_HANDLER( niyanpai_gfxflag_1_w ) { niyanpai_gfxflag_w(1, offset, data); }
WRITE16_HANDLER( niyanpai_scrollx_1_w ) { niyanpai_scrollx_w(1, offset, data); }
WRITE16_HANDLER( niyanpai_scrolly_1_w ) { niyanpai_scrolly_w(1, offset, data); }
WRITE16_HANDLER( niyanpai_radr_1_w ) { niyanpai_radr_w(1, offset, data); }
WRITE16_HANDLER( niyanpai_sizex_1_w ) { niyanpai_sizex_w(1, offset, data); }
WRITE16_HANDLER( niyanpai_sizey_1_w ) { niyanpai_sizey_w(1, offset, data); }
WRITE16_HANDLER( niyanpai_drawx_1_w ) { niyanpai_drawx_w(1, offset, data); }
WRITE16_HANDLER( niyanpai_drawy_1_w ) { niyanpai_drawy_w(1, offset, data); }

WRITE16_HANDLER( niyanpai_gfxflag_2_w ) { niyanpai_gfxflag_w(2, offset, data); }
WRITE16_HANDLER( niyanpai_scrollx_2_w ) { niyanpai_scrollx_w(2, offset, data); }
WRITE16_HANDLER( niyanpai_scrolly_2_w ) { niyanpai_scrolly_w(2, offset, data); }
WRITE16_HANDLER( niyanpai_radr_2_w ) { niyanpai_radr_w(2, offset, data); }
WRITE16_HANDLER( niyanpai_sizex_2_w ) { niyanpai_sizex_w(2, offset, data); }
WRITE16_HANDLER( niyanpai_sizey_2_w ) { niyanpai_sizey_w(2, offset, data); }
WRITE16_HANDLER( niyanpai_drawx_2_w ) { niyanpai_drawx_w(2, offset, data); }
WRITE16_HANDLER( niyanpai_drawy_2_w ) { niyanpai_drawy_w(2, offset, data); }

READ16_HANDLER( niyanpai_gfxbusy_0_r ) { return niyanpai_gfxbusy_r(0, offset); }
READ16_HANDLER( niyanpai_gfxbusy_1_r ) { return niyanpai_gfxbusy_r(1, offset); }
READ16_HANDLER( niyanpai_gfxbusy_2_r ) { return niyanpai_gfxbusy_r(2, offset); }
READ16_HANDLER( niyanpai_gfxrom_0_r ) { return niyanpai_gfxrom_r(0, offset); }
READ16_HANDLER( niyanpai_gfxrom_1_r ) { return niyanpai_gfxrom_r(1, offset); }
READ16_HANDLER( niyanpai_gfxrom_2_r ) { return niyanpai_gfxrom_r(2, offset); }

/******************************************************************************


******************************************************************************/
VIDEO_START( niyanpai )
{
	if ((niyanpai_tmpbitmap0 = auto_bitmap_alloc(Machine->drv->screen_width, Machine->drv->screen_height)) == 0) return 1;
	if ((niyanpai_tmpbitmap1 = auto_bitmap_alloc(Machine->drv->screen_width, Machine->drv->screen_height)) == 0) return 1;
	if ((niyanpai_tmpbitmap2 = auto_bitmap_alloc(Machine->drv->screen_width, Machine->drv->screen_height)) == 0) return 1;
	if ((niyanpai_videoram0 = auto_malloc(Machine->drv->screen_width * Machine->drv->screen_height * sizeof(short))) == 0) return 1;
	if ((niyanpai_videoram1 = auto_malloc(Machine->drv->screen_width * Machine->drv->screen_height * sizeof(short))) == 0) return 1;
	if ((niyanpai_videoram2 = auto_malloc(Machine->drv->screen_width * Machine->drv->screen_height * sizeof(short))) == 0) return 1;
	if ((niyanpai_palette = auto_malloc(0x480 * sizeof(short))) == 0) return 1;
	if ((niyanpai_paltbl0 = auto_malloc(0x1000 * sizeof(char))) == 0) return 1;
	if ((niyanpai_paltbl1 = auto_malloc(0x1000 * sizeof(char))) == 0) return 1;
	if ((niyanpai_paltbl2 = auto_malloc(0x1000 * sizeof(char))) == 0) return 1;
	memset(niyanpai_videoram0, 0x0000, (Machine->drv->screen_width * Machine->drv->screen_height * sizeof(short)));
	memset(niyanpai_videoram1, 0x0000, (Machine->drv->screen_width * Machine->drv->screen_height * sizeof(short)));
	memset(niyanpai_videoram2, 0x0000, (Machine->drv->screen_width * Machine->drv->screen_height * sizeof(short)));
	return 0;
}

/******************************************************************************


******************************************************************************/
VIDEO_UPDATE( niyanpai )
{
	int x, y;
	unsigned short color;

	if (get_vh_global_attribute_changed() || niyanpai_screen_refresh)
	{
		niyanpai_screen_refresh = 0;

		Machine->pens[0x0ff] = 0;	/* palette_transparent_pen */
		Machine->pens[0x1ff] = 0;	/* palette_transparent_pen */
		Machine->pens[0x2ff] = 0;	/* palette_transparent_pen */

		for (y = 0; y < Machine->drv->screen_height; y++)
		{
			for (x = 0; x < Machine->drv->screen_width; x++)
			{
				color = niyanpai_videoram0[(y * Machine->drv->screen_width) + x];
				plot_pixel(niyanpai_tmpbitmap0, x, y, Machine->pens[color]);

				color = niyanpai_videoram1[(y * Machine->drv->screen_width) + x];
				plot_pixel(niyanpai_tmpbitmap1, x, y, Machine->pens[color]);

				color = niyanpai_videoram2[(y * Machine->drv->screen_width) + x];
				plot_pixel(niyanpai_tmpbitmap2, x, y, Machine->pens[color]);
			}
		}
	}

	if (niyanpai_dispflag[0])
	{
		copyscrollbitmap(bitmap, niyanpai_tmpbitmap0, 1, &niyanpai_scrollx[0], 1, &niyanpai_scrolly[0], &Machine->visible_area, TRANSPARENCY_NONE, 0);
	}
	else
	{
		fillbitmap(bitmap, Machine->pens[0x00ff], 0);
	}

	if (niyanpai_dispflag[1])
	{
		copyscrollbitmap(bitmap, niyanpai_tmpbitmap1, 1, &niyanpai_scrollx[1], 1, &niyanpai_scrolly[1], &Machine->visible_area, TRANSPARENCY_PEN, Machine->pens[0x01ff]);
	}

	if (niyanpai_dispflag[2])
	{
		copyscrollbitmap(bitmap, niyanpai_tmpbitmap2, 1, &niyanpai_scrollx[2], 1, &niyanpai_scrolly[2], &Machine->visible_area, TRANSPARENCY_PEN, Machine->pens[0x02ff]);
	}
}
