/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "machine/6522via.h"
#include "vidhrdw/generic.h"


static data8_t x,y,color;
static data8_t graphics_command;
static int pending;


/* RGBI palette. Is it correct? */
PALETTE_INIT( leprechn )
{
	int i;

	for (i = 0; i < 16; i++)
	{
		int bk = (i & 8) ? 0x40 : 0x00;
		int r = (i & 1) ? 0xff : bk;
		int g = (i & 2) ? 0xff : bk;
		int b = (i & 4) ? 0xff : bk;

		palette_set_color(i,r,g,b);
	}
}



VIDEO_START( leprechn )
{
	videoram_size = Machine->drv->screen_width * Machine->drv->screen_height;

	/* allocate our own dirty buffer */
	videoram = auto_malloc(videoram_size);
	if (!videoram)
		return 1;

	return video_start_generic_bitmapped();
}


WRITE_HANDLER( leprechn_graphics_command_w )
{
    graphics_command = data;
}


static void clear_screen_done_callback(int param)
{
	/* Indicate that the we are done*/
	via_0_ca1_w(0, 0);
}


WRITE_HANDLER( leprechn_videoram_w )
{
	int sx,sy;

	if (pending)
	{
		plot_pixel(tmpbitmap, x, y, Machine->pens[color]);
        videoram[y * Machine->drv->screen_width + x] = color;

        pending = 0;
   	}

    switch (graphics_command)
    {
    case 0x00:	/* Move and plot*/

        color = data & 0x0f;

		if (data & 0x10)
		{
			if (data & 0x40)
				x--;
			else
				x++;
		}

		if (data & 0x20)
		{
			if (data & 0x80)
				y--;
			else
				y++;
		}

		pending = 1;

		break;

    case 0x08:  /* X position write*/
        x = data;
        break;

    case 0x10:  /* Y position write*/
        y = data;
        break;

    case 0x18:  /* Clear bitmap*/

		/* Indicate that the we are busy*/
		via_0_ca1_w(0, 1);

        memset(videoram, data, videoram_size);

        for (sx = 0; sx < Machine->drv->screen_width; sx++)
        {
	        for (sy = 0; sy < Machine->drv->screen_height; sy++)
	        {
				plot_pixel(tmpbitmap, sx, sy, Machine->pens[data]);
			}
		}

		/* Set a timer for an arbitrarily short period.*/
		/* The real time it takes to clear to screen is not*/
		/* significant for the software.*/
		timer_set(TIME_NOW, 0, clear_screen_done_callback);

        break;

	default:
	    /* Doesn't seem to happen.*/
	    log_cb(RETRO_LOG_DEBUG, LOGPRE "Unknown Graphics Command #%2X at %04X\n", graphics_command, activecpu_get_pc());
    }
}


READ_HANDLER( leprechn_videoram_r )
{
    return videoram[y * Machine->drv->screen_width + x];
}
