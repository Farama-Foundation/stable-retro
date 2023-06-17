/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/williams.h"
#include "cpu/m6800/m6800.h"
#include "cpu/m6809/m6809.h"
#include "6821pia.h"
#include "machine/ticket.h"
#include "williams.h"


/* banking addresses set by the drivers */
UINT8 *williams_bank_base;
UINT8 *defender_bank_base;
const UINT32 *defender_bank_list;
UINT8 *mayday_protection;

/* internal bank switching tracking */
static UINT8 blaster_bank;
static UINT8 vram_bank;
UINT8 williams2_bank;

/* switches controlled by $c900 */
UINT16 sinistar_clip;
UINT8 williams_cocktail;

/* other stuff */
static UINT16 joust2_current_sound_data;

/* older-Williams routines */
static void williams_main_irq(int state);
static void williams_main_firq(int state);
static void williams_snd_irq(int state);
static WRITE_HANDLER( williams_snd_cmd_w );
static WRITE_HANDLER( playball_snd_cmd_w );

/* input port mapping */
static UINT8 port_select;
static WRITE_HANDLER( williams_port_select_w );
static READ_HANDLER( williams_input_port_0_3_r );
static READ_HANDLER( williams_input_port_49way_0_5_r );
static READ_HANDLER( williams_input_port_1_4_r );
static READ_HANDLER( williams_49way_port_0_r );

/* newer-Williams routines */
static WRITE_HANDLER( williams2_snd_cmd_w );

/* Defender-specific code */
READ_HANDLER( defender_input_port_0_r );
static READ_HANDLER( defender_io_r );
static WRITE_HANDLER( defender_io_w );

/* Stargate-specific code */
READ_HANDLER( stargate_input_port_0_r );

/* Lotto Fun-specific code */
static READ_HANDLER( lottofun_input_port_0_r );

/* Turkey Shoot-specific code */
static READ_HANDLER( tshoot_input_port_0_3_r );
static WRITE_HANDLER( tshoot_lamp_w );
static WRITE_HANDLER( tshoot_maxvol_w );

/* Joust 2-specific code */
static WRITE_HANDLER( joust2_snd_cmd_w );
static WRITE_HANDLER( joust2_pia_3_cb1_w );



/*************************************
 *
 *	Generic old-Williams PIA interfaces
 *
 *************************************/

/* Generic PIA 0, maps to input ports 0 and 1 */
struct pia6821_interface williams_pia_0_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ input_port_0_r, input_port_1_r, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, 0, 0, 0,
	/*irqs   : A/B             */ 0, 0
};

/* Generic muxing PIA 0, maps to input ports 0/3 and 1; port select is CB2 */
struct pia6821_interface williams_muxed_pia_0_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ williams_input_port_0_3_r, input_port_1_r, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, 0, 0, williams_port_select_w,
	/*irqs   : A/B             */ 0, 0
};

/* Generic dual muxing PIA 0, maps to input ports 0/3 and 1/4; port select is CB2 */
struct pia6821_interface williams_dual_muxed_pia_0_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ williams_input_port_0_3_r, williams_input_port_1_4_r, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, 0, 0, williams_port_select_w,
	/*irqs   : A/B             */ 0, 0
};

/* Generic 49-way joystick PIA 0 for Sinistar/Blaster */
struct pia6821_interface williams_49way_pia_0_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ williams_49way_port_0_r, input_port_1_r, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, 0, 0, 0,
	/*irqs   : A/B             */ 0, 0
};

/* Muxing 49-way joystick PIA 0 for Blaster kit */
struct pia6821_interface williams_49way_muxed_pia_0_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ williams_input_port_49way_0_5_r, input_port_1_r, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, 0, 0, williams_port_select_w,
	/*irqs   : A/B             */ 0, 0
};

/* Generic PIA 1, maps to input port 2, sound command out, and IRQs */
struct pia6821_interface williams_pia_1_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ input_port_2_r, 0, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, williams_snd_cmd_w, 0, 0,
	/*irqs   : A/B             */ williams_main_irq, williams_main_irq
};

/* Generic PIA 2, maps to DAC data in and sound IRQs */
struct pia6821_interface williams_snd_pia_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ DAC_0_data_w, 0, 0, 0,
	/*irqs   : A/B             */ williams_snd_irq, williams_snd_irq
};



/*************************************
 *
 *	Game-specific old-Williams PIA interfaces
 *
 *************************************/

/* Special PIA 0 for Defender, to handle the controls */
struct pia6821_interface defender_pia_0_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ defender_input_port_0_r, input_port_1_r, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, 0, 0, 0,
	/*irqs   : A/B             */ 0, 0
};

/* Special PIA 0 for Stargate, to handle the controls */
struct pia6821_interface stargate_pia_0_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ stargate_input_port_0_r, input_port_1_r, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, 0, 0, 0,
	/*irqs   : A/B             */ 0, 0
};

/* Special PIA 0 for Lotto Fun, to handle the controls and ticket dispenser */
struct pia6821_interface lottofun_pia_0_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ lottofun_input_port_0_r, input_port_1_r, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, ticket_dispenser_w, 0, 0,
	/*irqs   : A/B             */ 0, 0
};

/* Special PIA 2 for Sinistar, to handle the CVSD */
struct pia6821_interface sinistar_snd_pia_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ DAC_0_data_w, 0, hc55516_0_digit_w, hc55516_0_clock_w,
	/*irqs   : A/B             */ williams_snd_irq, williams_snd_irq
};

/* Special PIA 1 for PlayBall, doesn't set the high bits on sound commands */
struct pia6821_interface playball_pia_1_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ input_port_2_r, 0, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, playball_snd_cmd_w, 0, 0,
	/*irqs   : A/B             */ williams_main_irq, williams_main_irq
};

/* extra PIA 3 for Speed Ball */
struct pia6821_interface spdball_pia_3_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ input_port_3_r, input_port_4_r, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, 0, 0, 0,
	/*irqs   : A/B             */ 0, 0
};



/*************************************
 *
 *	Generic later-Williams PIA interfaces
 *
 *************************************/

/* Generic muxing PIA 0, maps to input ports 0/3 and 1; port select is CA2 */
struct pia6821_interface williams2_muxed_pia_0_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ williams_input_port_0_3_r, input_port_1_r, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, 0, williams_port_select_w, 0,
	/*irqs   : A/B             */ 0, 0
};

/* Generic PIA 1, maps to input port 2, sound command out, and IRQs */
struct pia6821_interface williams2_pia_1_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ input_port_2_r, 0, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, williams2_snd_cmd_w, 0, pia_2_ca1_w,
	/*irqs   : A/B             */ williams_main_irq, williams_main_irq
};

/* Generic PIA 2, maps to DAC data in and sound IRQs */
struct pia6821_interface williams2_snd_pia_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ pia_1_portb_w, DAC_0_data_w, pia_1_cb1_w, 0,
	/*irqs   : A/B             */ williams_snd_irq, williams_snd_irq
};



/*************************************
 *
 *	Game-specific later-Williams PIA interfaces
 *
 *************************************/

/* Mystic Marathon PIA 0 */
struct pia6821_interface mysticm_pia_0_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ input_port_0_r, input_port_1_r, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, 0, 0, 0,
	/*irqs   : A/B             */ williams_main_firq, williams_main_irq
};

/* Turkey Shoot PIA 0 */
struct pia6821_interface tshoot_pia_0_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ tshoot_input_port_0_3_r, input_port_1_r, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, tshoot_lamp_w, williams_port_select_w, 0,
	/*irqs   : A/B             */ williams_main_irq, williams_main_irq
};

/* Turkey Shoot PIA 2 */
struct pia6821_interface tshoot_snd_pia_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ pia_1_portb_w, DAC_0_data_w, pia_1_cb1_w, tshoot_maxvol_w,
	/*irqs   : A/B             */ williams_snd_irq, williams_snd_irq
};

/* Joust 2 PIA 1 */
struct pia6821_interface joust2_pia_1_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ input_port_2_r, 0, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, joust2_snd_cmd_w, joust2_pia_3_cb1_w, pia_2_ca1_w,
	/*irqs   : A/B             */ williams_main_irq, williams_main_irq
};



/*************************************
 *
 *	Older Williams interrupts
 *
 *************************************/

static void williams_va11_callback(int scanline)
{
	/* the IRQ signal comes into CB1, and is set to VA11 */
	pia_1_cb1_w(0, scanline & 0x20);

	/* update the screen while we're here */
	force_partial_update(scanline - 1);

	/* set a timer for the next update */
	scanline += 8;
	if (scanline >= 256) scanline = 0;
	timer_set(cpu_getscanlinetime(scanline), scanline, williams_va11_callback);
}


static void williams_count240_off_callback(int param)
{
	/* the COUNT240 signal comes into CA1, and is set to the logical AND of VA10-VA13 */
	pia_1_ca1_w(0, 0);
}


static void williams_count240_callback(int param)
{
	/* the COUNT240 signal comes into CA1, and is set to the logical AND of VA10-VA13 */
	pia_1_ca1_w(0, 1);

	/* set a timer to turn it off once the scanline counter resets */
	timer_set(cpu_getscanlinetime(0), 0, williams_count240_off_callback);

	/* set a timer for next frame */
	timer_set(cpu_getscanlinetime(240), 0, williams_count240_callback);
}


static void williams_main_irq(int state)
{
	/* IRQ to the main CPU */
	cpu_set_irq_line(0, M6809_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}


static void williams_main_firq(int state)
{
	/* FIRQ to the main CPU */
	cpu_set_irq_line(0, M6809_FIRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}


static void williams_snd_irq(int state)
{
	/* IRQ to the sound CPU */
	cpu_set_irq_line(1, M6800_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}



/*************************************
 *
 *	Older Williams initialization
 *
 *************************************/

MACHINE_INIT( williams )
{
	/* reset the PIAs */
	pia_reset();

	/* reset the ticket dispenser (Lotto Fun) */
	ticket_dispenser_init(70, TICKET_MOTOR_ACTIVE_LOW, TICKET_STATUS_ACTIVE_HIGH);

	/* set a timer to go off every 16 scanlines, to toggle the VA11 line and update the screen */
	timer_set(cpu_getscanlinetime(0), 0, williams_va11_callback);

	/* also set a timer to go off on scanline 240 */
	timer_set(cpu_getscanlinetime(240), 0, williams_count240_callback);
}



/*************************************
 *
 *	Older Williams VRAM/ROM banking
 *
 *************************************/

WRITE_HANDLER( williams_vram_select_w )
{
	/* VRAM/ROM banking from bit 0 */
	vram_bank = data & 0x01;

	/* cocktail flip from bit 1 */
	williams_cocktail = data & 0x02;

	/* sinistar clipping enable from bit 2 */
	sinistar_clip = (data & 0x04) ? 0x7400 : 0xffff;

	/* set the bank */
	if (vram_bank)
	{
		cpu_setbank(1, williams_bank_base);
	}
	else
	{
		cpu_setbank(1, williams_videoram);
	}
}



/*************************************
 *
 *	Older Williams sound commands
 *
 *************************************/

static void williams_deferred_snd_cmd_w(int param)
{
	pia_2_portb_w(0, param);
	pia_2_cb1_w(0, (param == 0xff) ? 0 : 1);
}

WRITE_HANDLER( williams_snd_cmd_w )
{
	/* the high two bits are set externally, and should be 1 */
	timer_set(TIME_NOW, data | 0xc0, williams_deferred_snd_cmd_w);
}

WRITE_HANDLER( playball_snd_cmd_w )
{
	timer_set(TIME_NOW, data, williams_deferred_snd_cmd_w);
}



/*************************************
 *
 *	General input port handlers
 *
 *************************************/

WRITE_HANDLER( williams_port_select_w )
{
	port_select = data;
}


READ_HANDLER( williams_input_port_0_3_r )
{
	return readinputport(port_select ? 3 : 0);
}


READ_HANDLER( williams_input_port_1_4_r )
{
	return readinputport(port_select ? 4 : 1);
}


/*
 *  Williams 49-way joystick
 *
 * The joystick has 48 positions + center.
 *
 * I'm not 100% sure but it looks like it's mapped this way:
 *
 *	xxxx1000 = up full
 *	xxxx1100 = up 2/3
 *	xxxx1110 = up 1/3
 *	xxxx0111 = center
 *	xxxx0011 = down 1/3
 *	xxxx0001 = down 2/3
 *	xxxx0000 = down full
 *
 *	1000xxxx = right full
 *	1100xxxx = right 2/3
 *	1110xxxx = right 1/3
 *	0111xxxx = center
 *	0011xxxx = left 1/3
 *	0001xxxx = left 2/3
 *	0000xxxx = left full
 *
 */

READ_HANDLER( williams_49way_port_0_r )
{
	int joy_x, joy_y;
	int bits_x, bits_y;

	joy_x = readinputport(3) >> 4;	/* 0 = left 3 = center 6 = right */
	joy_y = readinputport(4) >> 4;	/* 0 = down 3 = center 6 = up */

	bits_x = (0x70 >> (7 - joy_x)) & 0x0f;
	bits_y = (0x70 >> (7 - joy_y)) & 0x0f;

	return (bits_x << 4) | bits_y;
}


READ_HANDLER( williams_input_port_49way_0_5_r )
{
	if (port_select)
		return williams_49way_port_0_r(0);
	else
		return readinputport(5);
}



/*************************************
 *
 *	Newer Williams interrupts
 *
 *************************************/

static void williams2_va11_callback(int scanline)
{
	/* the IRQ signal comes into CB1, and is set to VA11 */
	pia_0_cb1_w(0, scanline & 0x20);
	pia_1_ca1_w(0, scanline & 0x20);

	/* update the screen while we're here */
	force_partial_update(scanline);

	/* set a timer for the next update */
	scanline += 8;
	if (scanline >= 256) scanline = 0;
	timer_set(cpu_getscanlinetime(scanline), scanline, williams2_va11_callback);
}


static void williams2_endscreen_off_callback(int param)
{
	/* the /ENDSCREEN signal comes into CA1 */
	pia_0_ca1_w(0, 1);
}


static void williams2_endscreen_callback(int param)
{
	/* the /ENDSCREEN signal comes into CA1 */
	pia_0_ca1_w(0, 0);

	/* set a timer to turn it off once the scanline counter resets */
	timer_set(cpu_getscanlinetime(8), 0, williams2_endscreen_off_callback);

	/* set a timer for next frame */
	timer_set(cpu_getscanlinetime(254), 0, williams2_endscreen_callback);
}



/*************************************
 *
 *	Newer Williams initialization
 *
 *************************************/

MACHINE_INIT( williams2 )
{
	/* reset the PIAs */
	pia_reset();

	/* make sure our banking is reset */
	williams2_bank_select_w(0, 0);

	/* set a timer to go off every 16 scanlines, to toggle the VA11 line and update the screen */
	timer_set(cpu_getscanlinetime(0), 0, williams2_va11_callback);

	/* also set a timer to go off on scanline 254 */
	timer_set(cpu_getscanlinetime(254), 0, williams2_endscreen_callback);
}



/*************************************
 *
 *	Newer Williams ROM banking
 *
 *************************************/

WRITE_HANDLER( williams2_bank_select_w )
{
	static const UINT32 bank[8] = { 0, 0x10000, 0x20000, 0x10000, 0, 0x30000, 0x40000, 0x30000 };

	/* select bank index (only lower 3 bits used by IC56) */
	williams2_bank = data & 0x07;

	/* bank 0 references videoram */
	if (williams2_bank == 0)
	{
		cpu_setbank(1, williams_videoram);
		cpu_setbank(2, williams_videoram + 0x8000);
	}

	/* other banks reference ROM plus either palette RAM or the top of videoram */
	else
	{
		unsigned char *RAM = memory_region(REGION_CPU1);

		cpu_setbank(1, &RAM[bank[williams2_bank]]);

		if ((williams2_bank & 0x03) == 0x03)
		{
			cpu_setbank(2, williams2_paletteram);
		}
		else
		{
			cpu_setbank(2, williams_videoram + 0x8000);
		}
	}

	/* regardless, the top 2k references videoram */
	cpu_setbank(3, williams_videoram + 0x8800);
}



/*************************************
 *
 *	Newer Williams sound commands
 *
 *************************************/

static void williams2_deferred_snd_cmd_w(int param)
{
	pia_2_porta_w(0, param);
}


static WRITE_HANDLER( williams2_snd_cmd_w )
{
	timer_set(TIME_NOW, data, williams2_deferred_snd_cmd_w);
}



/*************************************
 *
 *	Newer Williams other stuff
 *
 *************************************/

WRITE_HANDLER( williams2_7segment_w )
{
	int n;
	char dot;

	switch (data & 0x7F)
	{
		case 0x40:	n = 0; break;
		case 0x79:	n = 1; break;
		case 0x24:	n = 2; break;
		case 0x30:	n = 3; break;
		case 0x19:	n = 4; break;
		case 0x12:	n = 5; break;
		case 0x02:	n = 6; break;
		case 0x03:	n = 6; break;
		case 0x78:	n = 7; break;
		case 0x00:	n = 8; break;
		case 0x18:	n = 9; break;
		case 0x10:	n = 9; break;
		default:	n =-1; break;
	}

	if ((data & 0x80) == 0x00)
		dot = '.';
	else
		dot = ' ';

	if (n == -1)
		log_cb(RETRO_LOG_DEBUG, LOGPRE "[ %c]\n", dot);
	else
		log_cb(RETRO_LOG_DEBUG, LOGPRE "[%d%c]\n", n, dot);
}



/*************************************
 *
 *	Defender-specific routines
 *
 *************************************/

MACHINE_INIT( defender )
{
	/* standard init */
	machine_init_williams();

	/* make sure the banking is reset to 0 */
	defender_bank_select_w(0, 0);
	cpu_setbank(1, williams_videoram);
}



WRITE_HANDLER( defender_bank_select_w )
{
	UINT32 bank_offset = defender_bank_list[data & 7];

	/* set bank address */
	cpu_setbank(2, &memory_region(REGION_CPU1)[bank_offset]);

	/* if the bank maps into normal RAM, it represents I/O space */
	if (bank_offset < 0x10000)
	{
		memory_set_bankhandler_r(2, 0, defender_io_r);
		memory_set_bankhandler_w(2, 0, defender_io_w);
	}

	/* otherwise, it's ROM space */
	else
	{
		memory_set_bankhandler_r(2, 0, MRA_BANK2);
		memory_set_bankhandler_w(2, 0, MWA_ROM);
	}
}


READ_HANDLER( defender_input_port_0_r )
{
	int keys, altkeys;

	/* read the standard keys and the cheat keys */
	keys = readinputport(0);
	altkeys = readinputport(3);

	/* modify the standard keys with the cheat keys */
	if (altkeys)
	{
		keys |= altkeys;
		if (memory_region(REGION_CPU1)[0xa0bb] == 0xfd)
		{
			if (altkeys & 0x02)
				keys = (keys & 0xfd) | 0x40;
			else if (altkeys & 0x40)
				keys = (keys & 0xbf) | 0x02;
		}
	}

	return keys;
}


READ_HANDLER( defender_io_r )
{
	/* PIAs */
	if (offset >= 0x0c00 && offset < 0x0c04)
		return pia_1_r(offset & 3);
	else if (offset >= 0x0c04 && offset < 0x0c08)
		return pia_0_r(offset & 3);

	/* video counter */
	else if (offset == 0x800)
		return williams_video_counter_r(offset);

	/* If not bank 0 then return banked RAM */
	return defender_bank_base[offset];
}


WRITE_HANDLER( defender_io_w )
{
	/* write the data through */
	defender_bank_base[offset] = data;

	/* watchdog */
	if (offset == 0x03fc)
		watchdog_reset_w(offset, data);

	/* palette */
	else if (offset < 0x10)
		paletteram_BBGGGRRR_w(offset, data);

	/* PIAs */
	else if (offset >= 0x0c00 && offset < 0x0c04)
		pia_1_w(offset & 3, data);
	else if (offset >= 0x0c04 && offset < 0x0c08)
		pia_0_w(offset & 3, data);
}



/*************************************
 *
 *	Mayday-specific routines
 *
 *************************************/

READ_HANDLER( mayday_protection_r )
{
	/* Mayday does some kind of protection check that is not currently understood  */
	/* However, the results of that protection check are stored at $a190 and $a191 */
	/* These are compared against $a193 and $a194, respectively. Thus, to prevent  */
	/* the protection from resetting the machine, we just return $a193 for $a190,  */
	/* and $a194 for $a191. */
	return mayday_protection[offset + 3];
}



/*************************************
 *
 *	Stargate-specific routines
 *
 *************************************/

READ_HANDLER( stargate_input_port_0_r )
{
	int keys, altkeys;

	/* read the standard keys and the cheat keys */
	keys = input_port_0_r(0);
	altkeys = input_port_3_r(0);

	/* modify the standard keys with the cheat keys */
	if (altkeys)
	{
		keys |= altkeys;
		if (memory_region(REGION_CPU1)[0x9c92] == 0xfd)
		{
			if (altkeys & 0x02)
				keys = (keys & 0xfd) | 0x40;
			else if (altkeys & 0x40)
				keys = (keys & 0xbf) | 0x02;
		}
	}

	return keys;
}



/*************************************
 *
 *	Blaster-specific routines
 *
 *************************************/

static const UINT32 blaster_bank_offset[16] =
{
	0x00000, 0x10000, 0x14000, 0x18000, 0x1c000, 0x20000, 0x24000, 0x28000,
	0x2c000, 0x30000, 0x34000, 0x38000, 0x2c000, 0x30000, 0x34000, 0x38000
};


WRITE_HANDLER( blaster_vram_select_w )
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	vram_bank = data;

	/* non-zero banks map to RAM and the currently-selected bank */
	if (vram_bank)
	{
		cpu_setbank(1, &RAM[blaster_bank_offset[blaster_bank]]);
		cpu_setbank(2, williams_bank_base + 0x4000);
	}

	/* bank 0 maps to videoram */
	else
	{
		cpu_setbank(1, williams_videoram);
		cpu_setbank(2, williams_videoram + 0x4000);
	}
}


WRITE_HANDLER( blaster_bank_select_w )
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	blaster_bank = data & 15;

	/* only need to change anything if we're not pointing to VRAM */
	if (vram_bank)
	{
		cpu_setbank(1, &RAM[blaster_bank_offset[blaster_bank]]);
	}
}



/*************************************
 *
 *	Lotto Fun-specific routines
 *
 *************************************/

static READ_HANDLER( lottofun_input_port_0_r )
{
	/* merge in the ticket dispenser status */
	return input_port_0_r(offset) | ticket_dispenser_r(offset);
}



/*************************************
 *
 *	Turkey Shoot-specific routines
 *
 *************************************/

static READ_HANDLER( tshoot_input_port_0_3_r )
{
	/* merge in the gun inputs with the standard data */
	int data = williams_input_port_0_3_r(offset);
	int gun = (data & 0x3f) ^ ((data & 0x3f) >> 1);
	return (data & 0xc0) | gun;
}


static WRITE_HANDLER( tshoot_maxvol_w )
{
	/* something to do with the sound volume */
	log_cb(RETRO_LOG_DEBUG, LOGPRE "tshoot maxvol = %d (pc:%x)\n", data, activecpu_get_pc());
}


static WRITE_HANDLER( tshoot_lamp_w )
{
	/* set the grenade lamp */
	set_led_status(0,data & 0x04);

	/* set the gun lamp */
	set_led_status(1,data & 0x08);

#if 0
	/* gun coil */
	if (data & 0x10)
		log_cb(RETRO_LOG_DEBUG, LOGPRE "[gun coil] ");
	else
		log_cb(RETRO_LOG_DEBUG, LOGPRE "           ");

	/* feather coil */
	if (data & 0x20)
		log_cb(RETRO_LOG_DEBUG, LOGPRE "[feather coil] ");
	else
		log_cb(RETRO_LOG_DEBUG, LOGPRE "               ");

	log_cb(RETRO_LOG_DEBUG, LOGPRE "\n");
#endif
}



/*************************************
 *
 *	Joust 2-specific routines
 *
 *************************************/

MACHINE_INIT( joust2 )
{
	/* standard init */
	machine_init_williams2();

	/* make sure sound board starts out in the reset state */
	williams_cvsd_init(2, 3);
	pia_reset();
}


static void joust2_deferred_snd_cmd_w(int param)
{
	pia_2_porta_w(0, param & 0xff);
}


static WRITE_HANDLER( joust2_pia_3_cb1_w )
{
	joust2_current_sound_data = (joust2_current_sound_data & ~0x100) | ((data << 8) & 0x100);
	pia_3_cb1_w(offset, data);
}


static WRITE_HANDLER( joust2_snd_cmd_w )
{
	joust2_current_sound_data = (joust2_current_sound_data & ~0xff) | (data & 0xff);
	williams_cvsd_data_w(joust2_current_sound_data);
	timer_set(TIME_NOW, joust2_current_sound_data, joust2_deferred_snd_cmd_w);
}
