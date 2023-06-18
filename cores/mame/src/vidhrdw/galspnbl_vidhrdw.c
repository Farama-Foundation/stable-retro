#include "driver.h"
#include "vidhrdw/generic.h"


data16_t *galspnbl_bgvideoram,*galspnbl_videoram,*galspnbl_colorram;
static int screenscroll;



PALETTE_INIT( galspnbl )
{
	int i;

	/* initialize 555 RGB lookup */
	for (i = 0;i < 32768;i++)
	{
		int r,g,b;

		r = (i >>  5) & 0x1f;
		g = (i >> 10) & 0x1f;
		b = (i >>  0) & 0x1f;

		r = (r << 3) | (r >> 2);
		g = (g << 3) | (g >> 2);
		b = (b << 3) | (b >> 2);
		palette_set_color(i+1024,r,g,b);
	}
}



WRITE16_HANDLER( galspnbl_bgvideoram_w )
{
	int sx,sy;


	data = COMBINE_DATA(&galspnbl_bgvideoram[offset]);

	sx = offset % 512;
	sy = offset / 512;

	plot_pixel(tmpbitmap,sx,sy,Machine->pens[1024 + (data >> 1)]);
}

WRITE16_HANDLER( galspnbl_scroll_w )
{
	if (ACCESSING_LSB)
		screenscroll = 4-(data & 0xff);
}



/* sprite format (see also Ninja Gaiden):
 *
 *	word		bit					usage
 * --------+-fedcba9876543210-+----------------
 *    0    | ---------------x | flip x
 *         | --------------x- | flip y
 *         | -------------x-- | enable
 *         | ----------xx---- | priority?
 *         | ---------x------ | flicker?
 *    1    | xxxxxxxxxxxxxxxx | code
 *    2    | --------xxxx---- | color
 *         | --------------xx | size: 8x8, 16x16, 32x32, 64x64
 *    3    | xxxxxxxxxxxxxxxx | y position
 *    4    | xxxxxxxxxxxxxxxx | x position
 *    5,6,7|                  | unused
 */
static void draw_sprites(struct mame_bitmap *bitmap,int priority)
{
	int offs;
	const UINT8 layout[8][8] =
	{
		{0,1,4,5,16,17,20,21},
		{2,3,6,7,18,19,22,23},
		{8,9,12,13,24,25,28,29},
		{10,11,14,15,26,27,30,31},
		{32,33,36,37,48,49,52,53},
		{34,35,38,39,50,51,54,55},
		{40,41,44,45,56,57,60,61},
		{42,43,46,47,58,59,62,63}
	};

	for (offs = (spriteram_size-16)/2;offs >= 0;offs -= 8)
	{
		int sx,sy,code,color,size,attr,flipx,flipy;
		int col,row;

		attr = spriteram16[offs];
		if ((attr & 0x0004) && ((attr & 0x0040) == 0 || (cpu_getcurrentframe() & 1))
/*				&& ((attr & 0x0030) >> 4) == priority)*/
				&& ((attr & 0x0020) >> 5) == priority)
		{
			code = spriteram16[offs+1];
			color = spriteram16[offs+2];
			size = 1 << (color & 0x0003); /* 1,2,4,8*/
			color = (color & 0x00f0) >> 4;
			sx = spriteram16[offs+4] + screenscroll;
			sy = spriteram16[offs+3];
			flipx = attr & 0x0001;
			flipy = attr & 0x0002;

			for (row = 0;row < size;row++)
			{
				for (col = 0;col < size;col++)
				{
					int x = sx + 8*(flipx?(size-1-col):col);
					int y = sy + 8*(flipy?(size-1-row):row);
					drawgfx(bitmap,Machine->gfx[1],
						code + layout[row][col],
						color,
						flipx,flipy,
						x,y,
						&Machine->visible_area,TRANSPARENCY_PEN,0);
				}
			}
		}
	}
}


VIDEO_UPDATE( galspnbl )
{
	int offs;


	/* copy the temporary bitmap to the screen */
	copyscrollbitmap(bitmap,tmpbitmap,1,&screenscroll,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);

	draw_sprites(bitmap,0);

	for (offs = 0;offs < 0x1000/2;offs++)
	{
		int sx,sy,code,attr,color;

		code = galspnbl_videoram[offs];
		attr = galspnbl_colorram[offs];
		color = (attr & 0x00f0) >> 4;
		sx = offs % 64;
		sy = offs / 64;

		/* What is this? A priority/half transparency marker? */
		if (!(attr & 0x0008))
		{
			drawgfx(bitmap,Machine->gfx[0],
					code,
					color,
					0,0,
					16*sx + screenscroll,8*sy,
					&Machine->visible_area,TRANSPARENCY_PEN,0);
		}
	}

	draw_sprites(bitmap,1);
}
