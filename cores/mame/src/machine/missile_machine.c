/***************************************************************************

	Atari Missile Command hardware

***************************************************************************/

#include "driver.h"
#include "missile.h"
#include "vidhrdw/generic.h"


static int ctrld;
static int h_pos, v_pos;



/********************************************************************************************/
READ_HANDLER( missile_IN0_r )
{
	if (ctrld)	/* trackball */
	{
		if (!flip_screen)
	  	    return ((readinputport(5) << 4) & 0xf0) | (readinputport(4) & 0x0f);
		else
	  	    return ((readinputport(7) << 4) & 0xf0) | (readinputport(6) & 0x0f);
	}
	else	/* buttons */
		return (readinputport(0));
}


/********************************************************************************************/
MACHINE_INIT( missile )
{
	h_pos = v_pos = 0;
}


/********************************************************************************************/
WRITE_HANDLER( missile_w )
{
	int pc, opcode;
	offset = offset + 0x640;

	pc = activecpu_get_previouspc();
	opcode = cpu_readop(pc);

	/* 3 different ways to write to video ram - the third is caught by the core memory handler */
	if (opcode == 0x81)
	{
		/* 	STA ($00,X) */
		missile_video_w (offset, data);
		return;
	}
	if (offset <= 0x3fff)
	{
		missile_video_mult_w (offset, data);
		return;
	}

	/* $4c00 - watchdog */
	if (offset == 0x4c00)
	{
		watchdog_reset_w (offset, data);
		return;
	}

	/* $4800 - various IO */
	if (offset == 0x4800)
	{
		flip_screen_set(~data & 0x40);
		coin_counter_w(0,data & 0x20);
		coin_counter_w(1,data & 0x10);
		coin_counter_w(2,data & 0x08);
		set_led_status(0,~data & 0x02);
		set_led_status(1,~data & 0x04);
		ctrld = data & 1;
		return;
	}

	/* $4d00 - IRQ acknowledge */
	if (offset == 0x4d00)
	{
		return;
	}

	/* $4000 - $400f - Pokey */
	if (offset >= 0x4000 && offset <= 0x400f)
	{
		pokey1_w (offset, data);
		return;
	}

	/* $4b00 - $4b07 - color RAM */
	if (offset >= 0x4b00 && offset <= 0x4b07)
	{
		int r,g,b;


		r = 0xff * ((~data >> 3) & 1);
		g = 0xff * ((~data >> 2) & 1);
		b = 0xff * ((~data >> 1) & 1);

		palette_set_color(offset - 0x4b00,r,g,b);

		return;
	}

	log_cb(RETRO_LOG_DEBUG, LOGPRE "possible unmapped write, offset: %04x, data: %02x\n", offset, data);
}


/********************************************************************************************/

unsigned char *missile_video2ram;

READ_HANDLER( missile_r )
{
	int pc, opcode;
	offset = offset + 0x1900;

	pc = activecpu_get_previouspc();
	opcode = cpu_readop(pc);

	if (opcode == 0xa1)
	{
		/* 	LDA ($00,X)  */
		return (missile_video_r(offset));
	}

	if (offset >= 0x5000)
		return missile_video2ram[offset - 0x5000];

	if (offset == 0x4800)
		return (missile_IN0_r(0));
	if (offset == 0x4900)
		return (readinputport (1));
	if (offset == 0x4a00)
		return (readinputport (2));

	if ((offset >= 0x4000) && (offset <= 0x400f))
		return (pokey1_r (offset & 0x0f));

	log_cb(RETRO_LOG_DEBUG, LOGPRE "possible unmapped read, offset: %04x\n", offset);
	return 0;
}
