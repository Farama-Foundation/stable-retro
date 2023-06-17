/*************************************************************************

	Exidy 6502 hardware

*************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "exidy.h"

UINT8 *exidy_characterram;
UINT8 *exidy_color_latch;
UINT8 *exidy_sprite_no;
UINT8 *exidy_sprite_enable;
UINT8 *exidy_sprite1_xpos;
UINT8 *exidy_sprite1_ypos;
UINT8 *exidy_sprite2_xpos;
UINT8 *exidy_sprite2_ypos;

UINT8 exidy_collision_mask;
UINT8 exidy_collision_invert;

UINT8 *exidy_palette;
UINT16 *exidy_colortable;

static struct mame_bitmap *motion_object_1_vid;
static struct mame_bitmap *motion_object_2_vid;
static struct mame_bitmap *motion_object_2_clip;

static UINT8 chardirty[256];
static UINT8 update_complete;

static UINT8 int_condition;



/*************************************
 *
 *	Hard coded palettes
 *
 *************************************/

/* Sidetrack/Targ/Spectar don't have a color PROM; colors are changed by the means of 8x3 */
/* dip switches on the board. Here are the colors they map to. */
UINT8 sidetrac_palette[] =
{
	0x00,0x00,0x00,   /* BACKGND */
	0x00,0x00,0x00,   /* CSPACE0 */
	0x00,0xff,0x00,   /* CSPACE1 */
	0xff,0xff,0xff,   /* CSPACE2 */
	0xff,0xff,0xff,   /* CSPACE3 */
	0xff,0x00,0xff,   /* 5LINES (unused?) */
	0xff,0xff,0x00,   /* 5MO2VID  */
	0xff,0xff,0xff    /* 5MO1VID  */
};

/* Targ has different colors */
UINT8 targ_palette[] =
{
					/* color   use                */
	0x00,0x00,0xff, /* blue    background         */
	0x00,0xff,0xff, /* cyan    characters 192-255 */
	0xff,0xff,0x00, /* yellow  characters 128-191 */
	0xff,0xff,0xff, /* white   characters  64-127 */
	0xff,0x00,0x00, /* red     characters   0- 63 */
	0x00,0xff,0xff, /* cyan    not used           */
	0xff,0xff,0xff, /* white   bullet sprite      */
	0x00,0xff,0x00, /* green   wummel sprite      */
};

/* Spectar has different colors */
UINT8 spectar_palette[] =
{
					/* color   use                */
	0x00,0x00,0xff, /* blue    background         */
	0x00,0xff,0x00, /* green   characters 192-255 */
	0x00,0xff,0x00, /* green   characters 128-191 */
	0xff,0xff,0xff, /* white   characters  64-127 */
	0xff,0x00,0x00, /* red     characters   0- 63 */
	0x00,0xff,0x00, /* green   not used           */
	0xff,0xff,0x00, /* yellow  bullet sprite      */
	0x00,0xff,0x00, /* green   wummel sprite      */
};



/*************************************
 *
 *	Hard coded color tables
 *
 *************************************/

UINT16 exidy_1bpp_colortable[] =
{
	/* one-bit characters */
	0, 4,  /* chars 0x00-0x3F */
	0, 3,  /* chars 0x40-0x7F */
	0, 2,  /* chars 0x80-0xBF */
	0, 1,  /* chars 0xC0-0xFF */

	/* Motion Object 1 */
	0, 7,

	/* Motion Object 2 */
	0, 6,
};

UINT16 exidy_2bpp_colortable[] =
{
	/* two-bit characters */
	/* (Because this is 2-bit color, the colorspace is only divided
		in half instead of in quarters.  That's why 00-3F = 40-7F and
		80-BF = C0-FF) */
	0, 0, 4, 3,  /* chars 0x00-0x3F */
	0, 0, 4, 3,  /* chars 0x40-0x7F */
	0, 0, 2, 1,  /* chars 0x80-0xBF */
	0, 0, 2, 1,  /* chars 0xC0-0xFF */

	/* Motion Object 1 */
	0, 7,

	/* Motion Object 2 */
	0, 6,
};



/*************************************
 *
 *	Palettes and colors
 *
 *************************************/

PALETTE_INIT( exidy )
{
	if (exidy_palette)
	{
		int i;
		
		for (i = 0; i < PALETTE_LEN; i++)
			palette_set_color(i,exidy_palette[i*3+0],exidy_palette[i*3+1],exidy_palette[i*3+2]);
	}
	memcpy(colortable, exidy_colortable, COLORTABLE_LEN * sizeof(colortable[0]));
}



/*************************************
 *
 *	Video startup
 *
 *************************************/

VIDEO_START( exidy )
{
    if (video_start_generic())
        return 1;

	motion_object_1_vid = auto_bitmap_alloc(16, 16);
    if (!motion_object_1_vid)
        return 1;

	motion_object_2_vid = auto_bitmap_alloc(16, 16);
    if (!motion_object_2_vid)
        return 1;

	motion_object_2_clip = auto_bitmap_alloc(16, 16);
    if (!motion_object_2_clip)
        return 1;

    return 0;
}



/*************************************
 *
 *	Interrupt generation
 *
 *************************************/

static INLINE void latch_condition(int collision)
{
	collision ^= exidy_collision_invert;
	int_condition = (input_port_2_r(0) & ~0x1c) | (collision & exidy_collision_mask);
}


INTERRUPT_GEN( exidy_vblank_interrupt )
{
	/* latch the current condition */
	latch_condition(0);
	int_condition &= ~0x80;

	/* set the IRQ line */
	cpu_set_irq_line(0, 0, ASSERT_LINE);
}


INTERRUPT_GEN( teetert_vblank_interrupt )
{
	/* standard stuff */
	if (cpu_getiloops() == 0)
	exidy_vblank_interrupt();
	
	/* plus a pulse on the NMI line */
	cpu_set_irq_line(0, IRQ_LINE_NMI, PULSE_LINE);
}


READ_HANDLER( exidy_interrupt_r )
{
	/* clear any interrupts */
	cpu_set_irq_line(0, 0, CLEAR_LINE);

	/* return the latched condition */
	return int_condition;
}



/*************************************
 *
 *	Character RAM
 *
 *************************************/

WRITE_HANDLER( exidy_characterram_w )
{
	if (exidy_characterram[offset] != data)
	{
		exidy_characterram[offset] = data;
		chardirty[offset / 8 % 256] = 1;
	}
}



/*************************************
 *
 *	Palette RAM
 *
 *************************************/

WRITE_HANDLER( exidy_color_w )
{
	int i;

	exidy_color_latch[offset] = data;

	for (i = 0; i < 8; i++)
	{
		int b = ((exidy_color_latch[0] >> i) & 0x01) * 0xff;
		int g = ((exidy_color_latch[1] >> i) & 0x01) * 0xff;
		int r = ((exidy_color_latch[2] >> i) & 0x01) * 0xff;
		palette_set_color(i, r, g, b);
	}
}



/*************************************
 *
 *	Background update
 *
 *************************************/

static void update_background(void)
{
	int x, y, offs;

	/* update the background and any dirty characters in it */
	for (y = offs = 0; y < 32; y++)
		for (x = 0; x < 32; x++, offs++)
		{
			int code = videoram[offs];

			/* see if the character is dirty */
			if (chardirty[code] == 1)
			{
				decodechar(Machine->gfx[0], code, exidy_characterram, Machine->drv->gfxdecodeinfo[0].gfxlayout);
				chardirty[code] = 2;
			}

			/* see if the bitmap is dirty */
			if (dirtybuffer[offs] || chardirty[code])
			{
				int color = code >> 6;
				drawgfx(tmpbitmap, Machine->gfx[0], code, color, 0, 0, x * 8, y * 8, NULL, TRANSPARENCY_NONE, 0);
				dirtybuffer[offs] = 0;
			}
		}

	/* reset the char dirty array */
	for (y = 0; y < 256; y++)
		if (chardirty[y] == 2)
			chardirty[y] = 0;
}



/*************************************
 *
 *	Determine the time when the beam
 *	will intersect a given pixel
 *
 *************************************/

static double pixel_time(int x, int y)
{
	/* assuming this is called at refresh time, compute how long until we
	 * hit the given x,y position */
	return cpu_getscanlinetime(y) + (cpu_getscanlineperiod() * (double)x * (1.0 / 256.0));
}


static void collision_irq_callback(int param)
{
	/* latch the collision bits */
	latch_condition(param);

	/* set the IRQ line */
	cpu_set_irq_line(0, 0, ASSERT_LINE);
}



/*************************************
 *
 *	End-of-frame callback
 *
 *************************************/

/***************************************************************************

	Exidy hardware checks for two types of collisions based on the video
	signals.  If the Motion Object 1 and Motion Object 2 signals are on at
	the same time, an M1M2 collision bit gets set.  If the Motion Object 1
	and Background Character signals are on at the same time, an M1CHAR
	collision bit gets set.  So effectively, there's a pixel-by-pixel
	collision check comparing Motion Object 1 (the player) to the
	background and to the other Motion Object (typically a bad guy).

***************************************************************************/

static INLINE int sprite_1_enabled(void)
{
	return (!(*exidy_sprite_enable & 0x80) || (*exidy_sprite_enable & 0x10));
}

static INLINE int sprite_2_enabled(void)
{
	return (!(*exidy_sprite_enable & 0x40));
}

VIDEO_EOF( exidy )
{
	UINT8 enable_set = ((*exidy_sprite_enable & 0x20) != 0);
    struct rectangle clip = { 0, 15, 0, 15 };
    int pen0 = Machine->pens[0];
    int org_1_x = 0, org_1_y = 0;
    int org_2_x = 0, org_2_y = 0;
    int sx, sy;
	int count = 0;

	/* if there is nothing to detect, bail */
	if (exidy_collision_mask == 0)
		return;

	/* if the sprites aren't enabled, we can't collide */
	if (!sprite_1_enabled() && !sprite_2_enabled())
	{
		update_complete = 0;
		return;
	}

	/* update the background if necessary */
	if (!update_complete)
		update_background();
	update_complete = 0;

	/* draw sprite 1 */
	if (sprite_1_enabled())
	{
		org_1_x = 236 - *exidy_sprite1_xpos - 4;
		org_1_y = 244 - *exidy_sprite1_ypos - 4;
		drawgfx(motion_object_1_vid, Machine->gfx[1],
			(*exidy_sprite_no & 0x0f) + 16 * enable_set, 0,
			0, 0, 0, 0, &clip, TRANSPARENCY_NONE, 0);
	}
	else
		fillbitmap(motion_object_1_vid, pen0, &clip);

	/* draw sprite 2 */
	if (sprite_2_enabled())
	{
		org_2_x = 236 - *exidy_sprite2_xpos - 4;
		org_2_y = 244 - *exidy_sprite2_ypos - 4;
		drawgfx(motion_object_2_vid, Machine->gfx[1],
			((*exidy_sprite_no >> 4) & 0x0f) + 32, 0,
			0, 0, 0, 0, &clip, TRANSPARENCY_NONE, 0);
	}
	else
		fillbitmap(motion_object_2_vid, pen0, &clip);

    /* draw sprite 2 clipped to sprite 1's location */
	fillbitmap(motion_object_2_clip, pen0, &clip);
	if (sprite_1_enabled() && sprite_2_enabled())
	{
		sx = org_2_x - org_1_x;
		sy = org_2_y - org_1_y;
		drawgfx(motion_object_2_clip, Machine->gfx[1],
			((*exidy_sprite_no >> 4) & 0x0f) + 32, 0,
			0, 0, sx, sy, &clip, TRANSPARENCY_NONE, 0);
	}

    /* scan for collisions */
    for (sy = 0; sy < 16; sy++)
	    for (sx = 0; sx < 16; sx++)
	    {
    		if (read_pixel(motion_object_1_vid, sx, sy) != pen0)
    		{
	  			UINT8 collision_mask = 0;

                /* check for background collision (M1CHAR) */
				if (read_pixel(tmpbitmap, org_1_x + sx, org_1_y + sy) != pen0)
					collision_mask |= 0x04;

                /* check for motion object collision (M1M2) */
				if (read_pixel(motion_object_2_clip, sx, sy) != pen0)
					collision_mask |= 0x10;

				/* if we got one, trigger an interrupt */
				if ((collision_mask & exidy_collision_mask) && count++ < 128)
					timer_set(pixel_time(org_1_x + sx, org_1_y + sy), collision_mask, collision_irq_callback);
            }
            if (read_pixel(motion_object_2_vid, sx, sy) != pen0)
    		{
                /* check for background collision (M2CHAR) */
				if (read_pixel(tmpbitmap, org_2_x + sx, org_2_y + sy) != pen0)
					if ((exidy_collision_mask & 0x08) && count++ < 128)
						timer_set(pixel_time(org_2_x + sx, org_2_y + sy), 0x08, collision_irq_callback);
            }
		}
}



/*************************************
 *
 *	Standard screen refresh callback
 *
 *************************************/

VIDEO_UPDATE( exidy )
{
	int sx, sy;

	/* update the background and draw it */
	update_background();
	copybitmap(bitmap, tmpbitmap, 0, 0, 0, 0, cliprect, TRANSPARENCY_NONE, 0);

	/* draw sprite 2 first */
	if (sprite_2_enabled())
	{
		sx = 236 - *exidy_sprite2_xpos - 4;
		sy = 244 - *exidy_sprite2_ypos - 4;

		drawgfx(bitmap, Machine->gfx[1],
			((*exidy_sprite_no >> 4) & 0x0f) + 32, 1,
			0, 0, sx, sy, cliprect, TRANSPARENCY_PEN, 0);
	}

	/* draw sprite 1 next */
	if (sprite_1_enabled())
	{
		UINT8 enable_set = ((*exidy_sprite_enable & 0x20) != 0);

		sx = 236 - *exidy_sprite1_xpos - 4;
		sy = 244 - *exidy_sprite1_ypos - 4;

		if (sy < 0) sy = 0;

		drawgfx(bitmap, Machine->gfx[1],
			(*exidy_sprite_no & 0x0f) + 16 * enable_set, 0,
			0, 0, sx, sy, cliprect, TRANSPARENCY_PEN, 0);
	}

	/* indicate that we already updated the background */
	update_complete = 1;
}
