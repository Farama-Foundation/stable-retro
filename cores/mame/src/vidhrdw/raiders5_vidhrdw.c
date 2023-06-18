/*******************************************************************************

Raiders5 (c) 1985 Taito / UPL

Video hardware driver by Uki

	02/Jun/2001 -

*******************************************************************************/

#include "vidhrdw/generic.h"

UINT8 *raiders5_fgram;
size_t raiders5_fgram_size;

static UINT8 raiders5_xscroll,raiders5_yscroll;
static UINT8 flipscreen;


WRITE_HANDLER( raiders5_scroll_x_w )
{
	raiders5_xscroll = data;
}
WRITE_HANDLER( raiders5_scroll_y_w )
{
	raiders5_yscroll = data;
}

WRITE_HANDLER( raiders5_flipscreen_w )
{
	flipscreen = data & 0x01;
}

READ_HANDLER( raiders5_fgram_r )
{
	return raiders5_fgram[offset];
}
WRITE_HANDLER( raiders5_fgram_w )
{
	raiders5_fgram[offset] = data;
}

WRITE_HANDLER( raiders5_videoram_w )
{
	int y = (offset + ((raiders5_yscroll & 0xf8) << 2) ) & 0x3e0;
	int x = (offset + (raiders5_xscroll >> 3) ) & 0x1f;
	int offs = x+y+(offset & 0x400);

	videoram[offs] = data;
}
READ_HANDLER( raiders5_videoram_r )
{
	int y = (offset + ((raiders5_yscroll & 0xf8) << 2) ) & 0x3e0;
	int x = (offset + (raiders5_xscroll >> 3) ) & 0x1f;
	int offs = x+y+(offset & 0x400);

	return videoram[offs];
}

WRITE_HANDLER( raiders5_paletteram_w )
{
	int i;

	paletteram_BBGGRRII_w(offset,data);

	if (offset > 15)
		return;

	if (offset != 1)
	{
		for (i=0; i<16; i++)
		{
			paletteram_BBGGRRII_w(0x200+offset+i*16,data);
		}
	}
	paletteram_BBGGRRII_w(0x200+offset*16+1,data);
}

/****************************************************************************/

VIDEO_UPDATE( raiders5 )
{
	int offs;
	int chr,col;
	int x,y,px,py,fx,fy,sx,sy;
	int b1,b2;

	int size = videoram_size/2;

/* draw BG layer */

	for (y=0; y<32; y++)
	{
		for (x=0; x<32; x++)
		{
			offs = y*0x20 + x;

			if (flipscreen!=0)
				offs = (size-1)-offs;

			px = x*8;
			py = y*8;

			chr = videoram[ offs ] ;
			col = videoram[ offs + size];

			b1 = (col >> 1) & 1; /* ? */
			b2 = col & 1;

			col = (col >> 4) & 0x0f;
			chr = chr | b2*0x100;

			drawgfx(tmpbitmap,Machine->gfx[b1+3],
				chr,
				col,
				flipscreen,flipscreen,
				px,py,
				0,TRANSPARENCY_NONE,0);
		}
	}

	if (flipscreen == 0)
	{
		sx = -raiders5_xscroll+7;
		sy = -raiders5_yscroll;
	}
	else
	{
		sx = raiders5_xscroll;
		sy = raiders5_yscroll;
	}

	copyscrollbitmap(bitmap,tmpbitmap,1,&sx,1,&sy,&Machine->visible_area,TRANSPARENCY_NONE,0);

/* draw sprites */

	for (offs=0; offs<spriteram_size; offs +=32)
	{
		chr = spriteram[offs];
		col = spriteram[offs+3];

		b1 = (col >> 1) & 1;
		b2 = col & 0x01;

		fx = ((chr >> 0) & 1) ^ flipscreen;
		fy = ((chr >> 1) & 1) ^ flipscreen;

		x = spriteram[offs+1];
		y = spriteram[offs+2];

		col = (col >> 4) & 0x0f ;
		chr = (chr >> 2) | b2*0x40;

		if (flipscreen==0)
		{
			px = x;
			py = y;
		}
		else
		{
			px = 240-x;
			py = 240-y;
		}

		drawgfx(bitmap,Machine->gfx[b1],
			chr,
			col,
			fx,fy,
			px,py,
			&Machine->visible_area,TRANSPARENCY_PEN,0);

		if (px>0xf0)
			drawgfx(bitmap,Machine->gfx[b1],
				chr,
				col,
				fx,fy,
				px-0x100,py,
				&Machine->visible_area,TRANSPARENCY_PEN,0);
	}


/* draw FG layer */

	for (y=4; y<28; y++)
	{
		for (x=0; x<32; x++)
		{
			offs = y*32+x;
			chr = raiders5_fgram[offs];
			col = raiders5_fgram[offs + 0x400] >> 4;

			if (flipscreen==0)
			{
				px = 8*x;
				py = 8*y;
			}
			else
			{
				px = 248-8*x;
				py = 248-8*y;
			}

			drawgfx(bitmap,Machine->gfx[2],
				chr,
				col,
				flipscreen,flipscreen,
				px,py,
				&Machine->visible_area,TRANSPARENCY_PEN,0);
		}
	}
}
