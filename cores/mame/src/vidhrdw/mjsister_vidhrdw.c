/*****************************************************************************

Mahjong Sisters (c) 1986 Toa Plan

Video hardware
	driver by Uki

*****************************************************************************/

#include "vidhrdw/generic.h"

int mjsister_screen_redraw;
int mjsister_flip_screen;
int mjsister_video_enable;

int vrambank;
int colorbank;

static struct mame_bitmap *mjsister_tmpbitmap0, *mjsister_tmpbitmap1;
static UINT8 *mjsister_videoram0, *mjsister_videoram1;

/****************************************************************************/

VIDEO_START( mjsister )
{
	if ((mjsister_tmpbitmap0 = bitmap_alloc(256,256)) == 0)
		return 1;
	if ((mjsister_tmpbitmap1 = bitmap_alloc(256,256)) == 0)
		return 1;
	if ((mjsister_videoram0 = auto_malloc(0x8000)) == 0)
		return 1;
	if ((mjsister_videoram1 = auto_malloc(0x8000)) == 0)
		return 1;

	return 0;
}

void mjsister_plot0(int offset,unsigned char data)
{
	int x,y,c1,c2;

	x = offset & 0x7f;
	y = offset / 0x80;

	c1 = (data & 0x0f)        + colorbank * 0x20;
	c2 = ((data & 0xf0) >> 4) + colorbank * 0x20;

	plot_pixel(mjsister_tmpbitmap0, x*2,   y, Machine->pens[c1] );
	plot_pixel(mjsister_tmpbitmap0, x*2+1, y, Machine->pens[c2] );
}

void mjsister_plot1(int offset,unsigned char data)
{
	int x,y,c1,c2;

	x = offset & 0x7f;
	y = offset / 0x80;

	c1 = data & 0x0f;
	c2 = (data & 0xf0) >> 4;

	if (c1)
		c1 += colorbank * 0x20 + 0x10;
	if (c2)
		c2 += colorbank * 0x20 + 0x10;

	plot_pixel(mjsister_tmpbitmap1, x*2,   y, Machine->pens[c1] );
	plot_pixel(mjsister_tmpbitmap1, x*2+1, y, Machine->pens[c2] );
}

WRITE_HANDLER( mjsister_videoram_w )
{
	if (vrambank)
	{
		mjsister_videoram1[offset] = data;
		mjsister_plot1(offset,data);
	}
	else
	{
		mjsister_videoram0[offset] = data;
		mjsister_plot0(offset,data);
	}
}

VIDEO_UPDATE( mjsister )
{
	int f = mjsister_flip_screen;
	int i,j;

	if (mjsister_screen_redraw)
	{
		int offs;

		for (offs=0; offs<0x8000; offs++)
		{
			mjsister_plot0(offs,mjsister_videoram0[offs]);
			mjsister_plot1(offs,mjsister_videoram1[offs]);
		}

		mjsister_screen_redraw = 0;
	}

	if (mjsister_video_enable)
	{
		for (i=0; i<256; i++)
		{
			for (j=0; j<4; j++)
				plot_pixel(bitmap, 256+j, i, Machine->pens[colorbank * 0x20] );
		}

		copybitmap(bitmap,mjsister_tmpbitmap0,f,f,0,0,cliprect,TRANSPARENCY_NONE,0);
		copybitmap(bitmap,mjsister_tmpbitmap1,f,f,2,0,cliprect,TRANSPARENCY_PEN,0);
	}
	else
		fillbitmap(bitmap, get_black_pen(), &Machine->visible_area);
}
