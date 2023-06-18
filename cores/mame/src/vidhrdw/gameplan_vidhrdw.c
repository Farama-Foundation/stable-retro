/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/


#define VERBOSE_VIDEO
#define SHOW_CHARS

#include "driver.h"
#include "vidhrdw/generic.h"

static int gameplan_this_is_kaos;
static int gameplan_this_is_megatack;
static int clear_to_colour = 0;
static int fix_clear_to_colour = -1;
static const char *colour_names[] = {"WHITE", "CYAN", "MAGENTA", "BLUE",
							   "YELLOW", "GREEN", "RED", ".BLACK"};



void gameplan_clear_screen(void);


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/


VIDEO_START( gameplan )
{
	if (strcmp(Machine->gamedrv->name, "kaos") == 0)
		gameplan_this_is_kaos = 1;
	else
		gameplan_this_is_kaos = 0;

	if (strcmp(Machine->gamedrv->name, "megatack") == 0)
		gameplan_this_is_megatack = 1;
	else
		gameplan_this_is_megatack = 0;

	return video_start_generic_bitmapped();
}


static int port_b;
static int new_request = 0;
static int finished_sound = 0;
static int cb2 = -1;

READ_HANDLER( gameplan_sound_r )
{
#ifdef VERBOSE
	log_cb(RETRO_LOG_DEBUG, LOGPRE "GAME:  read reg%X at PC %04x\n", offset, activecpu_get_pc());
#endif

	if (offset == 0)
	{
#ifdef VERBOSE
		if (finished_sound)  log_cb(RETRO_LOG_DEBUG, LOGPRE "[GAME: checking sound request ack: OK (%d)]\n", finished_sound);
		else  log_cb(RETRO_LOG_DEBUG, LOGPRE "[GAME: checking sound request ack: BAD (%d)]\n", finished_sound);
#endif

		return finished_sound;
	}
	else
		return 0;
}

WRITE_HANDLER( gameplan_sound_w )
{
#ifdef VERBOSE
	log_cb(RETRO_LOG_DEBUG, LOGPRE "GAME: write reg%X with %02x at PC %04x\n", offset, data, activecpu_get_pc());
#endif

	if (offset == 1)
	{
#ifdef VERBOSE
		log_cb(RETRO_LOG_DEBUG, LOGPRE "[GAME: request sound number %d]\n", data);
#endif

		if (cb2 == 0)
		{
/*	enabling this causes a hang in Challenger when entering high score name*/
/*			cpu_set_reset_line(1,PULSE_LINE);*/
			return;
		}

		port_b = data;
		finished_sound = 0;
		new_request = 1;

		/* shortly after requesting a sound, the game board checks
		   whether the sound board has ackknowledged receipt of the
		   command - yield now to allow it to send the ACK */
/*		cpu_yield();	enabling this causes a hang in Challenger when entering high score name*/
	}
	else if (offset == 0x0c)	/* PCR */
	{
		if (data & 0x80)
		{
			if ((data & 0x60) == 0x60)
				cb2 = 1;
			else if ((data & 0x60) == 0x40)
				cb2 = 0;
			else cb2 = -1;
		}
	}
}

READ_HANDLER( gameplan_via5_r )
{
#ifdef VERBOSE
	log_cb(RETRO_LOG_DEBUG, LOGPRE "SOUND:  read reg%X at PC %04x\n", offset, activecpu_get_pc());
#endif

	if (offset == 0)
	{
		new_request = 0;
#ifdef VERBOSE
		log_cb(RETRO_LOG_DEBUG, LOGPRE "[SOUND: received sound request %d]\n", port_b);
#endif
		return port_b;
	}

	if (offset == 5)
	{
		if (new_request == 1)
		{
#ifdef VERBOSE
			log_cb(RETRO_LOG_DEBUG, LOGPRE "[SOUND: checking for new request - found]\n");
#endif
			return 0x40;
		}
		else
		{
#ifdef VERBOSE
			log_cb(RETRO_LOG_DEBUG, LOGPRE "[SOUND: checking for new request - none]\n");
#endif
			return 0;
		}
	}

	return 1;
}

WRITE_HANDLER( gameplan_via5_w )
{
#ifdef VERBOSE
	log_cb(RETRO_LOG_DEBUG, LOGPRE "SOUND: write reg%X with %02x at PC %04x\n", offset, data, activecpu_get_pc());
#endif

	if (offset == 2)
	{
#ifdef VERBOSE
		log_cb(RETRO_LOG_DEBUG, LOGPRE "[SOUND: ack received request %d]\n", data);
#endif
		finished_sound = data;
	}
}

READ_HANDLER( gameplan_video_r )
{
	static int x;
	x++;
#if 0
	log_cb(RETRO_LOG_DEBUG, LOGPRE "%04x: reading %d from 200d\n", activecpu_get_pc(), x);
#endif
	return x;
}

WRITE_HANDLER( gameplan_video_w )
{
	static int r0 = -1;
	static unsigned char xpos, ypos, colour = 7;

#ifdef VERBOSE
	log_cb(RETRO_LOG_DEBUG, LOGPRE "VIA 1: PC %04x: %x -> reg%X\n", activecpu_get_pc(), data, offset);
#endif

	if (offset == 0)			/* write to 2000 */
	{
		r0 = data;
#ifdef VERBOSE
		log_cb(RETRO_LOG_DEBUG, LOGPRE "  mode = %d\n", data);
#endif
	}
	else if (offset == 1)		/* write to 2001 */
	{
		if (r0 == 0)
		{
			if (gameplan_this_is_kaos)
				colour = ~data & 0x07;
			else if (data & 0x0f)
			{
#ifdef VERBOSE
				log_cb(RETRO_LOG_DEBUG, LOGPRE "  !movement command %02x unknown\n", data);
#endif
			}

#ifdef VERBOSE_VIDEO
#ifdef SHOW_CHARS
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%c", colour_names[colour][0]);
#else
			log_cb(RETRO_LOG_DEBUG, LOGPRE "  line command %02x at (%d, %d) col %d (%s)\n", data, xpos, ypos, colour, colour_names[colour]);
#endif
#endif

			if (data & 0x20)
			{
				if (data & 0x80)
					ypos--;
				else
					ypos++;
			}
			if (data & 0x10)
			{
				if (data & 0x40)
					xpos--;
				else
					xpos++;
			}

			plot_pixel(tmpbitmap, xpos, ypos, Machine->pens[colour]);
		}
		else if (r0 == 1)
		{
			xpos = data;
#ifdef VERBOSE_VIDEO
#ifdef SHOW_CHARS
			log_cb(RETRO_LOG_DEBUG, LOGPRE "\n");
#else
			log_cb(RETRO_LOG_DEBUG, LOGPRE "  X = %d\n", xpos);
#endif
#endif
		}
		else if (r0 == 2)
		{
			ypos = data;
#ifdef VERBOSE_VIDEO
#ifndef SHOW_CHARS
			log_cb(RETRO_LOG_DEBUG, LOGPRE "  Y = %d\n", ypos);
#endif
#endif
		}
		else if (r0 == 3)
		{
			if (offset == 1 && data == 0)
			{
#ifdef VERBOSE_VIDEO
				log_cb(RETRO_LOG_DEBUG, LOGPRE "  clear screen\n");
#endif
				gameplan_clear_screen();
			}
#ifdef VERBOSE
			else log_cb(RETRO_LOG_DEBUG, LOGPRE "  !not clear screen: offset = %d, data = %d\n", offset, data);
#endif
		}
#ifdef VERBOSE
		else
		{
			log_cb(RETRO_LOG_DEBUG, LOGPRE "  !offset = %d, data = %02x\n", offset, data);
		}
#endif
	}
	else if (offset == 2)
	{
		if (data == 7)
		{
			/* This whole 'fix_clear_to_colour' and special casing for
			 * megatack thing is ugly, and doesn't even work properly.
			 */

			if (!gameplan_this_is_megatack || fix_clear_to_colour == -1)
				clear_to_colour = colour;
#ifdef VERBOSE_VIDEO
			if (fix_clear_to_colour == -1)
				log_cb(RETRO_LOG_DEBUG, LOGPRE "  clear screen colour = %d (%s)\n", colour, colour_names[colour]);
			else
				log_cb(RETRO_LOG_DEBUG, LOGPRE "  clear req colour %d hidden by fixed colour %d\n", colour, fix_clear_to_colour);
#endif
		}
#ifdef VERBOSE
		else
			log_cb(RETRO_LOG_DEBUG, LOGPRE "  !offset = %d, data = %02x\n", offset, data);
#endif
	}
	else if (offset == 3)
	{
		if (r0 == 0)
		{
#ifdef VERBOSE
			if ((data & 0xf8) != 0xf8)  log_cb(RETRO_LOG_DEBUG, LOGPRE "  !unknown data (%02x) written for pixel (%3d, %3d)\n", data, xpos, ypos);
#endif

			colour = data & 7;
#ifdef VERBOSE_VIDEO
#ifndef SHOW_CHARS
			log_cb(RETRO_LOG_DEBUG, LOGPRE "  colour %d, move to (%d, %d)\n", colour, xpos, ypos);
#endif
#endif
		}
		else if ((data & 0xf8) == 0xf8 && data != 0xff)
		{
			clear_to_colour = fix_clear_to_colour = data & 0x07;
#ifdef VERBOSE
			log_cb(RETRO_LOG_DEBUG, LOGPRE "  unusual colour request %d\n", data & 7);
#endif
		}
#ifdef VERBOSE
		else
			log_cb(RETRO_LOG_DEBUG, LOGPRE "  !offset = %d, data = %02x\n", offset, data);
#endif
	}
#ifdef VERBOSE
	else
		log_cb(RETRO_LOG_DEBUG, LOGPRE "  !offset = %d, data = %02x\n", offset, data);
#endif
}


/***************************************************************************

  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/


void gameplan_clear_screen(void)
{
#ifdef VERBOSE_VIDEO
	log_cb(RETRO_LOG_DEBUG, LOGPRE "  clearing the screen to colour %d (%s)\n", clear_to_colour, colour_names[clear_to_colour]);
#endif

	fillbitmap(tmpbitmap, Machine->pens[clear_to_colour], 0);

	fix_clear_to_colour = -1;
}
