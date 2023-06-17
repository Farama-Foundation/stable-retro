/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


unsigned char *thepit_attributesram;

static int graphics_bank = 0;

static struct rectangle spritevisiblearea =
{
	2*8+1, 32*8-1,
	2*8, 30*8-1
};
static struct rectangle spritevisibleareaflipx =
{
	0*8, 30*8-2,
	2*8, 30*8-1
};


/***************************************************************************

  Convert the color PROMs into a more useable format.

  bit 7 -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 220 ohm resistor  -- GREEN
        -- 470 ohm resistor  -- GREEN
        -- 1  kohm resistor  -- GREEN
        -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
  bit 0 -- 1  kohm resistor  -- RED


***************************************************************************/
PALETTE_INIT( thepit )
{
	int i;



	/* first of all, allocate primary colors for the background and foreground */
	/* this is wrong, but I don't know where to pick the colors from */
	for (i = 0;i < 8;i++)
	{
		int r = 0xff * ((i >> 2) & 1);
		int g = 0xff * ((i >> 1) & 1);
		int b = 0xff * ((i >> 0) & 1);
		palette_set_color(i,r,g,b);
	}

	for (i = 0;i < Machine->drv->total_colors-8;i++)
	{
		int bit0,bit1,bit2,r,g,b;


		bit0 = (color_prom[i] >> 0) & 0x01;
		bit1 = (color_prom[i] >> 1) & 0x01;
		bit2 = (color_prom[i] >> 2) & 0x01;
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		bit0 = (color_prom[i] >> 3) & 0x01;
		bit1 = (color_prom[i] >> 4) & 0x01;
		bit2 = (color_prom[i] >> 5) & 0x01;
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		bit0 = 0;
		bit1 = (color_prom[i] >> 6) & 0x01;
		bit2 = (color_prom[i] >> 7) & 0x01;
		b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		palette_set_color(i+8,r,g,b);
	}

	for (i = 0;i < Machine->drv->color_table_len;i++)
	{
		colortable[i] = i + 8;
	}
}


/***************************************************************************

 Super Mouse has 5 bits per gun (maybe 6 for green), exact weights are
 unknown.

***************************************************************************/
PALETTE_INIT( suprmous )
{
	int i;



	/* first of all, allocate primary colors for the background and foreground */
	/* this is wrong, but I don't know where to pick the colors from */
	for (i = 0;i < 8;i++)
	{
		int r = 0xff * ((i >> 2) & 1);
		int g = 0xff * ((i >> 1) & 1);
		int b = 0xff * ((i >> 0) & 1);
		palette_set_color(i,r,g,b);
	}

	for (i = 0;i < Machine->drv->total_colors-8;i++)
	{
		int bit0,bit1,bit2,bit3,bit4,r,g,b;


		bit0 = (color_prom[i+0x20] >> 6) & 0x01;
		bit1 = (color_prom[i+0x20] >> 7) & 0x01;
		bit2 = (color_prom[i] >> 0) & 0x01;
		bit3 = (color_prom[i] >> 1) & 0x01;
		bit4 = (color_prom[i] >> 2) & 0x01;
		r = 0x10 * bit0 + 0x20 * bit1 + 0x30 * bit2 + 0x40 * bit3 + 0x50 * bit4;
		bit0 = (color_prom[i+0x20] >> 1) & 0x01;
		bit1 = (color_prom[i+0x20] >> 2) & 0x01;
		bit2 = (color_prom[i+0x20] >> 3) & 0x01;
		bit3 = (color_prom[i+0x20] >> 4) & 0x01;
		bit4 = (color_prom[i+0x20] >> 5) & 0x01;
		g = 0x50 * bit0 + 0x40 * bit1 + 0x30 * bit2 + 0x20 * bit3 + 0x10 * bit4;
		bit0 = (color_prom[i] >> 3) & 0x01;
		bit1 = (color_prom[i] >> 4) & 0x01;
		bit2 = (color_prom[i] >> 5) & 0x01;
		bit3 = (color_prom[i] >> 6) & 0x01;
		bit4 = (color_prom[i] >> 7) & 0x01;
		b = 0x50 * bit0 + 0x40 * bit1 + 0x30 * bit2 + 0x20 * bit3 + 0x10 * bit4;
		palette_set_color(i+8,r,g,b);
	}

	for (i = 0;i < Machine->drv->color_table_len;i++)
	{
		colortable[i] = i + 8;
	}
}


WRITE_HANDLER( thepit_attributes_w )
{
	if ((offset & 1) && thepit_attributesram[offset] != data)
	{
		int i;


		for (i = offset / 2;i < videoram_size;i += 32)
			dirtybuffer[i] = 1;
	}

	thepit_attributesram[offset] = data;
}


WRITE_HANDLER( intrepid_graphics_bank_select_w )
{
	set_vh_global_attribute(&graphics_bank, data << 1);
}


READ_HANDLER( thepit_input_port_0_r )
{
	/* Read either the real or the fake input ports depending on the
	   horizontal flip switch. (This is how the real PCB does it) */
	if (flip_screen_x)
	{
		return input_port_3_r(offset);
	}
	else
	{
		return input_port_0_r(offset);
	}
}


WRITE_HANDLER( thepit_sound_enable_w )
{
	mixer_sound_enable_global_w(data);
}


/***************************************************************************

  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
static void drawtiles(struct mame_bitmap *bitmap,int priority)
{
	int offs,spacechar=0;


	if (priority == 1)
	{
		/* find the space character */
		while (Machine->gfx[0]->pen_usage[spacechar] & ~1) spacechar++;
	}


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		int bgcolor;


		bgcolor = (colorram[offs] & 0x70) >> 4;

		if ((priority == 0 && dirtybuffer[offs]) ||
				(priority == 1 && bgcolor != 0 && (colorram[offs] & 0x80) == 0))
		{
			int sx,sy,code,bank,color;


			dirtybuffer[offs] = 0;

			sx = (offs % 32);
			sy = 8*(offs / 32);

			if (priority == 0)
			{
				code = videoram[offs];
				bank = graphics_bank;
			}
			else
			{
				code = spacechar;
				bank = 0;

				sy = (sy - thepit_attributesram[2 * sx]) & 0xff;
			}

			if (flip_screen_x) sx = 31 - sx;
			if (flip_screen_y) sy = 248 - sy;

			color = colorram[offs] & (Machine->drv->gfxdecodeinfo[bank].total_color_codes - 1);

			/* set up the background color */
			Machine->gfx[bank]->
					colortable[color * Machine->gfx[bank]->color_granularity] =
					Machine->pens[bgcolor];

			drawgfx(priority == 0 ? tmpbitmap : bitmap,Machine->gfx[bank],
					code,
					color,
					flip_screen_x,flip_screen_y,
					8*sx,sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the temporary bitmap to the screen */
	if (priority == 0)
	{
		int i, scroll[32];

		if (flip_screen_x)
		{
			for (i = 0;i < 32;i++)
			{
				scroll[31-i] = -thepit_attributesram[2 * i];
				if (flip_screen_y) scroll[31-i] = -scroll[31-i];
			}
		}
		else
		{
			for (i = 0;i < 32;i++)
			{
				scroll[i] = -thepit_attributesram[2 * i];
				if (flip_screen_y) scroll[i] = -scroll[i];
			}
		}

		copyscrollbitmap(bitmap,tmpbitmap,0,0,32,scroll,&Machine->visible_area,TRANSPARENCY_NONE,0);
	}
}

static void drawsprites(struct mame_bitmap *bitmap,int priority)
{
	int offs;


	/* draw low priority sprites */
	for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
	{
		if (((spriteram[offs + 2] & 0x08) >> 3) == priority)
		{
			int sx,sy,flipx,flipy;


			if (spriteram[offs + 0] == 0 ||
				spriteram[offs + 3] == 0)
			{
				continue;
			}

			sx = (spriteram[offs+3] + 1) & 0xff;
			sy = 240 - spriteram[offs];

			flipx = spriteram[offs + 1] & 0x40;
			flipy = spriteram[offs + 1] & 0x80;

			if (flip_screen_x)
			{
				sx = 242 - sx;
				flipx = !flipx;
			}

			if (flip_screen_y)
			{
				sy = 240 - sy;
				flipy = !flipy;
			}

			/* Sprites 0-3 are drawn one pixel to the left */
			if (offs <= 3*4) sy++;

			drawgfx(bitmap,Machine->gfx[graphics_bank | 1],
					spriteram[offs + 1] & 0x3f,
					spriteram[offs + 2] & 0x07,
					flipx,flipy,
					sx,sy,
					flip_screen_x & 1 ? &spritevisibleareaflipx : &spritevisiblearea,TRANSPARENCY_PEN,0);
		}
	}
}


VIDEO_UPDATE( thepit )
{
	if (get_vh_global_attribute_changed())
	{
		memset(dirtybuffer, 1, videoram_size);
	}


	/* low priority tiles */
	drawtiles(bitmap,0);

	/* low priority sprites */
	drawsprites(bitmap,0);

	/* high priority tiles */
	drawtiles(bitmap,1);

	/* high priority sprites */
	drawsprites(bitmap,1);
}
