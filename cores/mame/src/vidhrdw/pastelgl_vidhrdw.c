/******************************************************************************

	Video Hardware for Nichibutsu Mahjong series.

	Driver by Takahiro Nogi 2000/06/07 -

******************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "nb1413m3.h"


static int pastelgl_drawx, pastelgl_drawy;
static int pastelgl_sizex, pastelgl_sizey;
static int pastelgl_radrx, pastelgl_radry;
static int pastelgl_gfxrom;
static int pastelgl_dispflag;
static int pastelgl_gfxflag;
static int pastelgl_flipscreen;
static int pastelgl_flipx, pastelgl_flipy;
static int pastelgl_screen_refresh;
static int pastelgl_palbank;

static struct mame_bitmap *pastelgl_tmpbitmap;
static unsigned char *pastelgl_videoram;
static unsigned char *pastelgl_paltbl;


void pastelgl_vramflip(void);
void pastelgl_gfxdraw(void);


/******************************************************************************


******************************************************************************/
PALETTE_INIT( pastelgl )
{
	int i;

	for (i = 0; i < Machine->drv->total_colors; i++)
	{
		int bit0, bit1, bit2, bit3, r, g, b;

		bit0 = (color_prom[0] >> 0) & 0x01;
		bit1 = (color_prom[0] >> 1) & 0x01;
		bit2 = (color_prom[0] >> 2) & 0x01;
		bit3 = (color_prom[0] >> 3) & 0x01;
		r = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[0] >> 4) & 0x01;
		bit1 = (color_prom[0] >> 5) & 0x01;
		bit2 = (color_prom[0] >> 6) & 0x01;
		bit3 = (color_prom[0] >> 7) & 0x01;
		g = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[Machine->drv->total_colors] >> 3) & 0x01;
		b = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		palette_set_color(i,r,g,b);
		color_prom++;
	}
}

void pastelgl_paltbl_w(int offset, int data)
{
	pastelgl_paltbl[offset] = data;
}

/******************************************************************************


******************************************************************************/
void pastelgl_radrx_w(int data)
{
	pastelgl_radrx = data;
}

void pastelgl_radry_w(int data)
{
	pastelgl_radry = data;
}

void pastelgl_sizex_w(int data)
{
	pastelgl_sizex = data;
}

void pastelgl_sizey_w(int data)
{
	pastelgl_sizey = data;

	pastelgl_gfxdraw();
}

void pastelgl_drawx_w(int data)
{
	pastelgl_drawx = (data ^ 0xff);
}

void pastelgl_drawy_w(int data)
{
	pastelgl_drawy = (data ^ 0xff);
}

void pastelgl_dispflag_w(int data)
{
	static int pastelgl_flipscreen_old = -1;

	pastelgl_gfxflag = data;

	pastelgl_flipx = (data & 0x01) ? 1 : 0;
	pastelgl_flipy = (data & 0x02) ? 1 : 0;
	pastelgl_flipscreen = (data & 0x04) ? 0 : 1;
	pastelgl_dispflag = (data & 0x08) ? 0 : 1;		/* unused ?*/
/*	if (data & 0xf0) usrintf_showmessage("Unknown GFXFLAG!! (%02X)", (data & 0xf0));*/

	if (nb1413m3_type == NB1413M3_PASTELGL)
	{
		pastelgl_flipscreen ^= 1;
	}

	if (pastelgl_flipscreen != pastelgl_flipscreen_old)
	{
		pastelgl_vramflip();
		pastelgl_screen_refresh = 1;
		pastelgl_flipscreen_old = pastelgl_flipscreen;
	}
}

void pastelgl_romsel_w(int data)
{
	pastelgl_gfxrom = ((data & 0xc0) >> 6);
	pastelgl_palbank = ((data & 0x10) >> 4);

	if ((pastelgl_gfxrom << 16) > (memory_region_length(REGION_GFX1) - 1))
	{
#ifdef MAME_DEBUG
		usrintf_showmessage("GFXROM BANK OVER!!");
#endif
		pastelgl_gfxrom = 0;
	}
}

/******************************************************************************


******************************************************************************/
void pastelgl_vramflip(void)
{
	int x, y;
	unsigned char color1, color2;

	for (y = 0; y < Machine->drv->screen_height; y++)
	{
		for (x = 0; x < Machine->drv->screen_width; x++)
		{
			color1 = pastelgl_videoram[(y * Machine->drv->screen_width) + x];
			color2 = pastelgl_videoram[((y ^ 0xff) * Machine->drv->screen_width) + (x ^ 0xff)];
			pastelgl_videoram[(y * Machine->drv->screen_width) + x] = color2;
			pastelgl_videoram[((y ^ 0xff) * Machine->drv->screen_width) + (x ^ 0xff)] = color1;
		}
	}
}

void pastelgl_gfxdraw(void)
{
	unsigned char *GFX = memory_region(REGION_GFX1);

	int x, y;
	int dx, dy;
	int startx, starty;
	int sizex, sizey;
	int skipx, skipy;
	int ctrx, ctry;
	int readflag;
	int tflag;
	int gfxaddr;
	unsigned char color;
	unsigned char drawcolor;

	if (pastelgl_flipx)
	{
		pastelgl_drawx -= (pastelgl_sizex << 1);
		startx = pastelgl_sizex;
		sizex = ((pastelgl_sizex ^ 0xff) + 1);
		skipx = -1;
	}
	else
	{
		pastelgl_drawx = (pastelgl_drawx - pastelgl_sizex);
		startx = 0;
		sizex = (pastelgl_sizex + 1);
		skipx = 1;
	}

	if (pastelgl_flipy)
	{
		pastelgl_drawy -= (pastelgl_sizey << 1);
		starty = pastelgl_sizey;
		sizey = ((pastelgl_sizey ^ 0xff) + 1);
		skipy = -1;
	}
	else
	{
		pastelgl_drawy = (pastelgl_drawy - pastelgl_sizey);
		starty = 0;
		sizey = (pastelgl_sizey + 1);
		skipy = 1;
	}

	gfxaddr = ((pastelgl_gfxrom << 16) + (pastelgl_radry << 8) + pastelgl_radrx);

	readflag = 0;

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

			color = GFX[gfxaddr];

			if (pastelgl_flipscreen)
			{
				dx = (((pastelgl_drawx + x) ^ 0xff) & 0xff);
				dy = (((pastelgl_drawy + y) ^ 0xff) & 0xff);
			}
			else
			{
				dx = ((pastelgl_drawx + x) & 0xff);
				dy = ((pastelgl_drawy + y) & 0xff);
			}

			if (!readflag)
			{
				/* 1st, 3rd, 5th, ... read*/
				color = (color & 0x0f);
			}
			else
			{
				/* 2nd, 4th, 6th, ... read*/
				color = (color & 0xf0) >> 4;
				gfxaddr++;
			}

			readflag ^= 1;

			tflag = 1;

			if (pastelgl_paltbl[color] & 0xf0)
			{
				if (!color) tflag = 0;
				drawcolor = ((pastelgl_palbank * 0x10) + color);
			}
			else
			{
				drawcolor = ((pastelgl_palbank * 0x10) + pastelgl_paltbl[color]);
			}

			if (tflag)
			{
				pastelgl_videoram[(dy * Machine->drv->screen_width) + dx] = drawcolor;
				plot_pixel(pastelgl_tmpbitmap, dx, dy, Machine->pens[drawcolor]);
			}

			nb1413m3_busyctr++;
		}
	}

	nb1413m3_busyflag = (nb1413m3_busyctr > 7000) ? 0 : 1;
}

/******************************************************************************


******************************************************************************/
VIDEO_START( pastelgl )
{
	if ((pastelgl_tmpbitmap = auto_bitmap_alloc(Machine->drv->screen_width, Machine->drv->screen_height)) == 0) return 1;
	if ((pastelgl_videoram = auto_malloc(Machine->drv->screen_width * Machine->drv->screen_height * sizeof(char))) == 0) return 1;
	if ((pastelgl_paltbl = auto_malloc(0x10 * sizeof(char))) == 0) return 1;
	memset(pastelgl_videoram, 0x00, (Machine->drv->screen_width * Machine->drv->screen_height * sizeof(char)));
	return 0;
}

/******************************************************************************


******************************************************************************/
VIDEO_UPDATE( pastelgl )
{
	int x, y;
	unsigned char color;

	if (get_vh_global_attribute_changed() || pastelgl_screen_refresh)
	{
		pastelgl_screen_refresh = 0;

		for (y = 0; y < Machine->drv->screen_height; y++)
		{
			for (x = 0; x < Machine->drv->screen_width; x++)
			{
				color = pastelgl_videoram[(y * Machine->drv->screen_width) + x];
				plot_pixel(pastelgl_tmpbitmap, x, y, Machine->pens[color]);
			}
		}
	}

	if (pastelgl_dispflag)
	{
		copybitmap(bitmap, pastelgl_tmpbitmap, 0, 0, 0, 0, &Machine->visible_area, TRANSPARENCY_NONE, 0);
	}
	else
	{
		fillbitmap(bitmap, Machine->pens[0x00], 0);
	}
}
