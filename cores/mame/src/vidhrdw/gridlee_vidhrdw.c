/***************************************************************************

	Videa Gridlee hardware

    driver by Aaron Giles

	Based on the Bally/Sente SAC system

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


/*************************************
 *
 *	Globals
 *
 *************************************/

UINT8 gridlee_cocktail_flip;



/*************************************
 *
 *	Statics
 *
 *************************************/

static UINT8 *local_videoram;

static UINT8 palettebank_vis;



/*************************************
 *
 *	Color PROM conversion
 *
 *************************************/

PALETTE_INIT( gridlee )
{
	int i;

	for (i = 0; i < Machine->drv->total_colors; i++)
	{
		int r = color_prom[0x0000] | (color_prom[0x0000] << 4);
		int g = color_prom[0x0800] | (color_prom[0x0800] << 4);
		int b = color_prom[0x1000] | (color_prom[0x1000] << 4);
		palette_set_color(i,r,g,b);
		color_prom++;
	}
}



/*************************************
 *
 *	Video system start
 *
 *************************************/

VIDEO_START( gridlee )
{
	/* allocate a local copy of video RAM */
	local_videoram = auto_malloc(256 * 256);
	if (!local_videoram)
		return 1;

	/* reset the palette */
	palettebank_vis = 0;
	return 0;
}



/*************************************
 *
 *	Cocktail flip
 *
 *************************************/

WRITE_HANDLER( gridlee_cocktail_flip_w )
{
	if (gridlee_cocktail_flip != (data & 1))
	{
		force_partial_update(cpu_getscanline() - 1);
		gridlee_cocktail_flip = data & 1;
	}
}



/*************************************
 *
 *	Video RAM write
 *
 *************************************/

WRITE_HANDLER( gridlee_videoram_w )
{
	videoram[offset] = data;

	/* expand the two pixel values into two bytes */
	local_videoram[offset * 2 + 0] = data >> 4;
	local_videoram[offset * 2 + 1] = data & 15;
}



/*************************************
 *
 *	Palette banking
 *
 *************************************/

WRITE_HANDLER( gridlee_palette_select_w )
{
	/* update the scanline palette */
	if (palettebank_vis != (data & 0x3f))
	{
		force_partial_update(cpu_getscanline() - 1);
		palettebank_vis = data & 0x3f;
	}
}



/*************************************
 *
 *	Main screen refresh
 *
 *************************************/

VIDEO_UPDATE( gridlee )
{
	pen_t *pens = &Machine->pens[palettebank_vis * 32];
	int x, y, i;

	/* draw scanlines from the VRAM directly */
	for (y = cliprect->min_y; y <= cliprect->max_y; y++)
	{
		/* non-flipped: draw directly from the bitmap */
		if (!gridlee_cocktail_flip)
			draw_scanline8(bitmap, 0, y, 256, &local_videoram[y * 256], pens + 16, -1);

		/* flipped: x-flip the scanline into a temp buffer and draw that */
		else
		{
			int srcy = 239 - y;
			UINT8 temp[256];
			int xx;

			for (xx = 0; xx < 256; xx++)
				temp[xx] = local_videoram[srcy * 256 + 255 - xx];
			draw_scanline8(bitmap, 0, y, 256, temp, pens + 16, -1);
		}
	}

	/* draw the sprite images */
	for (i = 0; i < 32; i++)
	{
		UINT8 *sprite = spriteram + i * 4;
		UINT8 *src;
		int image = sprite[0];
		int ypos = sprite[2] + 17;
		int xpos = sprite[3];

		/* get a pointer to the source image */
		src = &memory_region(REGION_GFX1)[64 * image];

		/* loop over y */
		for (y = 0; y < 16; y++, ypos = (ypos + 1) & 255)
		{
			int currxor = 0;

			/* adjust for flip */
			if (gridlee_cocktail_flip)
			{
				ypos = 239 - ypos;
				currxor = 0xff;
			}

			if (ypos >= 16 && ypos >= cliprect->min_y && ypos <= cliprect->max_y)
			{
				int currx = xpos;

				/* loop over x */
				for (x = 0; x < 4; x++)
				{
					int ipixel = *src++;
					int left = ipixel >> 4;
					int right = ipixel & 0x0f;

					/* left pixel */
					if (left && currx >= 0 && currx < 256)
						plot_pixel(bitmap, currx ^ currxor, ypos, pens[left]);
					currx++;

					/* right pixel */
					if (right && currx >= 0 && currx < 256)
						plot_pixel(bitmap, currx ^ currxor, ypos, pens[right]);
					currx++;
				}
			}
			else
				src += 4;

			/* de-adjust for flip */
			if (gridlee_cocktail_flip)
				ypos = 239 - ypos;
		}
	}
}
