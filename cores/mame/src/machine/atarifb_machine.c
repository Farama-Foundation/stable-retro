/*************************************************************************

	Atari Football hardware

*************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "atarifb.h"
#include "artwork.h"


static int CTRLD;
static int sign_x_1, sign_y_1;
static int sign_x_2, sign_y_2;
static int sign_x_3, sign_y_3;
static int sign_x_4, sign_y_4;


WRITE_HANDLER( atarifb_out1_w )
{
	CTRLD = data;

	discrete_sound_w(0,  data & 0x01);		/* Whistle*/
	discrete_sound_w(2,  data & 0x02);		/* Hit*/
	discrete_sound_w(3, (data & 0x10) ? 0 : 1);	/* Attract*/
	discrete_sound_w(4,  data & 0x04);		/* Noise Enable / Kicker*/

	if (GAME_IS_SOCCER)
	{
		/* bit 0 = whistle */
		/* bit 1 = hit */
		/* bit 2 = kicker */
		/* bit 3 = unused */
		/* bit 4 = 2/4 Player LED */	/* Say what?*/
		/* bit 5-6 = trackball CTRL bits */
		/* bit 7 = Rule LED */
/*		set_led_status(0,data & 0x10);	*/ /* !!!!!!!!!! Is this correct????*/
		set_led_status(1,data & 0x80);
	}

	if (GAME_IS_FOOTBALL4)
		coin_counter_w (1, data & 0x80);
	
	if (GAME_IS_BASEBALL)
	{
		if (data & 0x80)
		{
			/* Invert video */
			palette_set_color(2,0x00,0x00,0x00); /* black  */
			palette_set_color(0,0xff,0xff,0xff); /* white  */
		}
		else
		{
			/* Regular video */
			palette_set_color(0,0x00,0x00,0x00); /* black  */
			palette_set_color(2,0xff,0xff,0xff); /* white  */
		}
	}
}


WRITE_HANDLER( atarifb_out2_w )
{
	discrete_sound_w(1, data & 0x0f);	/* Crowd*/

	coin_counter_w (0, data & 0x10);

	if (GAME_IS_SOCCER)
	{
		coin_counter_w (1, data & 0x20);
		coin_counter_w (2, data & 0x40);
	}
}


/*************************************
 *
 *	LED control
 *
 *************************************/

WRITE_HANDLER( atarifb_out3_w )
{
	int loop = cpu_getiloops();

	switch (loop)
	{
		case 0x00:
			/* Player 1 play select lamp */
			atarifb_lamp1 = data;
			artwork_show("ledleft0", (atarifb_lamp1 >> 0) & 1);
			artwork_show("ledleft1", (atarifb_lamp1 >> 1) & 1);
			artwork_show("ledleft2", (atarifb_lamp1 >> 2) & 1);
			artwork_show("ledleft3", (atarifb_lamp1 >> 3) & 1);
			break;
		case 0x01:
			break;
		case 0x02:
			/* Player 2 play select lamp */
			atarifb_lamp2 = data;
			artwork_show("ledright0", (atarifb_lamp2 >> 0) & 1);
			artwork_show("ledright1", (atarifb_lamp2 >> 1) & 1);
			artwork_show("ledright2", (atarifb_lamp2 >> 2) & 1);
			artwork_show("ledright3", (atarifb_lamp2 >> 3) & 1);
			break;
		case 0x03:
			break;
	}
/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "out3_w, %02x:%02x\n", loop, data);*/
}


READ_HANDLER( atarifb_in0_r )
{
	if ((CTRLD & 0x20)==0x00)
	{
		int val;

		val = (sign_y_2 >> 7) |
			  (sign_x_2 >> 6) |
			  (sign_y_1 >> 5) |
			  (sign_x_1 >> 4) |
			  input_port_0_r(offset);
		return val;
	}
	else
	{
		static int counter_x,counter_y;
		int new_x,new_y;

		/* Read player 1 trackball */
		new_x = readinputport(3);
		if (new_x != counter_x)
		{
			sign_x_1 = (new_x - counter_x) & 0x80;
			counter_x = new_x;
		}

		new_y = readinputport(2);
		if (new_y != counter_y)
		{
			sign_y_1 = (new_y - counter_y) & 0x80;
			counter_y = new_y;
		}

		return (((counter_y & 0x0f) << 4) | (counter_x & 0x0f));
	}
}


READ_HANDLER( atarifb_in2_r )
{
	if ((CTRLD & 0x20)==0x00)
	{
		return input_port_1_r(offset);
	}
	else
	{
		static int counter_x,counter_y;
		int new_x,new_y;

		/* Read player 2 trackball */
		new_x = readinputport(5);
		if (new_x != counter_x)
		{
			sign_x_2 = (new_x - counter_x) & 0x80;
			counter_x = new_x;
		}

		new_y = readinputport(4);
		if (new_y != counter_y)
		{
			sign_y_2 = (new_y - counter_y) & 0x80;
			counter_y = new_y;
		}

		return (((counter_y & 0x0f) << 4) | (counter_x & 0x0f));
	}
}

READ_HANDLER( atarifb4_in0_r )
{
	/* LD1 and LD2 low, return sign bits */
	if ((CTRLD & 0x60)==0x00)
	{
		int val;

		val = (sign_x_4 >> 7) |
			  (sign_y_4 >> 6) |
			  (sign_x_2 >> 5) |
			  (sign_y_2 >> 4) |
			  (sign_x_3 >> 3) |
			  (sign_y_3 >> 2) |
			  (sign_x_1 >> 1) |
			  (sign_y_1 >> 0);
		return val;
	}
	else if ((CTRLD & 0x60) == 0x60)
	/* LD1 and LD2 both high, return Team 1 right player (player 1) */
	{
		static int counter_x,counter_y;
		int new_x,new_y;

		/* Read player 1 trackball */
		new_x = readinputport(4);
		if (new_x != counter_x)
		{
			sign_x_1 = (new_x - counter_x) & 0x80;
			counter_x = new_x;
		}

		new_y = readinputport(3);
		if (new_y != counter_y)
		{
			sign_y_1 = (new_y - counter_y) & 0x80;
			counter_y = new_y;
		}

		return (((counter_y & 0x0f) << 4) | (counter_x & 0x0f));
	}
	else if ((CTRLD & 0x60) == 0x40)
	/* LD1 high, LD2 low, return Team 1 left player (player 2) */
	{
		static int counter_x,counter_y;
		int new_x,new_y;

		/* Read player 2 trackball */
		new_x = readinputport(6);
		if (new_x != counter_x)
		{
			sign_x_2 = (new_x - counter_x) & 0x80;
			counter_x = new_x;
		}

		new_y = readinputport(5);
		if (new_y != counter_y)
		{
			sign_y_2 = (new_y - counter_y) & 0x80;
			counter_y = new_y;
		}

		return (((counter_y & 0x0f) << 4) | (counter_x & 0x0f));
	}

	else return 0;
}


READ_HANDLER( atarifb4_in2_r )
{
	if ((CTRLD & 0x40)==0x00)
	{
		return input_port_2_r(offset);
	}
	else if ((CTRLD & 0x60) == 0x60)
	/* LD1 and LD2 both high, return Team 2 right player (player 3) */
	{
		static int counter_x,counter_y;
		int new_x,new_y;

		/* Read player 3 trackball */
		new_x = readinputport(8);
		if (new_x != counter_x)
		{
			sign_x_3 = (new_x - counter_x) & 0x80;
			counter_x = new_x;
		}

		new_y = readinputport(7);
		if (new_y != counter_y)
		{
			sign_y_3 = (new_y - counter_y) & 0x80;
			counter_y = new_y;
		}

		return (((counter_y & 0x0f) << 4) | (counter_x & 0x0f));
	}
	else if ((CTRLD & 0x60) == 0x40)
	/* LD1 high, LD2 low, return Team 2 left player (player 4) */
	{
		static int counter_x,counter_y;
		int new_x,new_y;

		/* Read player 4 trackball */
		new_x = readinputport(10);
		if (new_x != counter_x)
		{
			sign_x_4 = (new_x - counter_x) & 0x80;
			counter_x = new_x;
		}

		new_y = readinputport(9);
		if (new_y != counter_y)
		{
			sign_y_4 = (new_y - counter_y) & 0x80;
			counter_y = new_y;
		}

		return (((counter_y & 0x0f) << 4) | (counter_x & 0x0f));
	}

	else return 0;
}

