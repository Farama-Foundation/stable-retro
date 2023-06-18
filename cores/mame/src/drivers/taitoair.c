/***************************************************************************

Taito Air System
----------------

Midnight Landing        *** not dumped, 1987? ***
Top Landing             (c) 1988 Taito
Air Inferno             (c) 1990 Taito


(Thanks to Raine team for their preliminary drivers)

Controls:

	P2 y analogue = throttle
	P1 analogue = pitch/yaw control

Can someone with flight sim stick confirm this is sensible.
I think we need OSD display for P1 l/r.


System specs	(from TaitoH: incorrect!)
------------

 CPU   : MC68000 (12 MHz) x 1, Z80 (4 MHz?, sound CPU) x 1
 Sound : YM2610, YM3016?
 OSC   : 20.000 MHz, 8.000 MHz, 24.000 MHz
 Chips : TC0070RGB (Palette?)
         TC0220IOC (Input)
         TC0140SYT (Sound communication)
         TC0130LNB (???)
         TC0160ROM (???)
         TC0080VCO (Video?)

From Ainferno readme
--------------------

Location     Type       File ID
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
CPU IC5       9016*     C45-01
CPU IC4       9016*     C45-02
CPU IC3       9016*     C45-03
CPU IC2       9016*     C45-04
CPU IC1       9016*     C45-05
CPU IC31      9016*     C45-06
VID IC28     27C010     C45-11
VID IC29     27C010     C45-12
VID IC30     27C010     C45-13
VID IC31     27C010     C45-14
VID IC40     27C010     C45-15
VID IC41     27C010     C45-16
VID IC42     27C010     C45-17
VID IC43     27C010     C45-18
CPU IC14     27C010     C45-20
CPU IC42     27C010     C45-21
CPU IC43     27C010     C45-22
CPU IC43     27C010     C45-23
CPU IC6      LH5763     C45-24
CPU IC35     LH5763     C45-25
CPU IC13     27C010     C45-28

VID IC6    PAL16L8B     C45-07
VID IC62   PAL16L8B     C45-08
VID IC63   PAL16L8B     C45-09
VID IC2    PAL20L8B     C45-10
CPU IC76   PAL16L8B     C45-26
CPU IC114  PAL16L8B     C45-27
CPU IC60   PAL20L8B     B62-02
CPU IC62   PAL20L8B     B62-03
CPU IC63   PAL20L8B     B62-04
CPU IC82   PAL16L8B     B62-07
VID IC23   PAL16L8B     B62-08
VID IC26   PAL16L8B     B62-11
VID IC27   PAL16L8B     B62-12


Notes:  CPU - CPU PCB      K1100586A  M4300186A
        VID - Video PCB    K1100576A  M4300186A


Known TC0080VCO issues	(from TaitoH driver)
----------------------

 - Y coordinate of sprite zooming is non-linear, so currently implemented
   hand-tuned value and this is used for only Record Breaker.
 - Sprite and BG1 priority bit is not understood. It is defined by sprite
   priority in Record Breaker and by zoom value and some scroll value in
   Dynamite League. So, some priority problems still remain.
 - Background zoom effect is not working in flip screen mode.
 - Sprite zoom is a bit wrong.


TODO	(TC0080VCO issues shared with TaitoH driver)
----

 - Need to implement BG1 : sprite priority. Currently not clear how this works.
 - Fix sprite coordinates.
 - Improve zoom y coordinate.


TODO
----

Video section hung off TaitoH driver, it should be separate.

3d graphics h/w: do the gradiation ram and line ram map to
hardware which creates the 3d background scenes? It seems
the TMS320C25 is being used as a co-processor to relieve the
68000 of 3d calculations... it has direct access to line ram
along with the 68000. Seems gradiation ram is responsibility
of 68000. Unless - unlikely IMO - there is banking
allowing the 32025 to select this area in its address map.

"Power common ram" is presumably for communication with an MCU
controlling the sit-in-cabinet (deluxe mechanized version only).

[Offer dip-selectable kludge of the analogue stick inputs so that
keyboard play is possible?]

Unknown control bits remain in the 0x140000 write.

DIPs


Topland
-------

Sprite/tile priority bad.

After demo game in attract, palette seems too dark for a while.
Palette corruption has occured with areas not restored after a fade.
Don't know why. (Perhaps 68000 relies on feedback from co-processor
in determining what parts of palette ram to write... but this would
then be fixed by hookup of 32025 core, which it isn't.)

Mechanized cabinet has a problem with test mode: there is
code at $d72 calling a sub which tests a silly amount of "power
common ram"; $80000 words (only one byte per word used).
Probably the address map wraps, and only $400 separate words
are actually accessed ?

TMS320C25 emulation: one unmapped read which appears to be
discarded. But the cpu waits for a bit to be zero... some
sort of frame flag or some "ready" message from the 3d h/w
perhaps? The two writes seem to take only two values.


Ainferno
--------

Sprite/tile priority bad.

More unmapped 320C25 reads and writes. This could be some sort of
I/O device?? The MCU program is longer than the Topland one.

cpu #2 (PC=000000C3): unmapped memory word write to 00006808 = 00FD & FFFF
cpu #2 (PC=000000C8): unmapped memory word write to 00006810 = FF38 & FFFF
cpu #2 (PC=000005A0): unmapped memory word write to 00006836 = 804E & FFFF
cpu #2 (PC=000005B2): unmapped memory word write to 00006830 = FFFF & FFFF
cpu #2 (PC=000005B5): unmapped memory word write to 00006832 = FFFE & FFFF
cpu #2 (PC=000005B8): unmapped memory word write to 00006834 = FBCA & FFFF
cpu #2 (PC=000005B9): unmapped memory word read from 00006836 & FFFF
cpu #2 (PC=000005CC): unmapped memory word write to 00006830 = FFFF & FFFF
cpu #2 (PC=000005CF): unmapped memory word write to 00006832 = FFFE & FFFF
cpu #2 (PC=000005D2): unmapped memory word write to 00006834 = FBCA & FFFF
cpu #2 (PC=000005D3): unmapped memory word read from 00006836 & FFFF
cpu #2 (PC=000005E6): unmapped memory word write to 00006830 = FFFF & FFFF
cpu #2 (PC=000005E9): unmapped memory word write to 00006832 = FFFE & FFFF
cpu #2 (PC=000005EC): unmapped memory word write to 00006834 = FC8F & FFFF
cpu #2 (PC=000005ED): unmapped memory word read from 00006836 & FFFF
cpu #2 (PC=00000600): unmapped memory word write to 00006830 = FFFF & FFFF
cpu #2 (PC=00000603): unmapped memory word write to 00006832 = FFFE & FFFF
cpu #2 (PC=00000606): unmapped memory word write to 00006834 = FC8F & FFFF
cpu #2 (PC=00000607): unmapped memory word read from 00006836 & FFFF
cpu #2 (PC=00000609): unmapped memory word read from 00006838 & FFFF
cpu #2 (PC=0000060E): unmapped memory word read from 0000683A & FFFF

****************************************************************************/

#include "driver.h"
#include "state.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/taitosnd.h"
#include "vidhrdw/taitoic.h"
#include "cpu/tms32025/tms32025.h"

static int dsp_HOLD_signal;

static data16_t *taitoh_68000_mainram;
data16_t *taitoair_line_ram;
static data16_t *dsp_ram;	/* Shared 68000/TMS32025 RAM */

VIDEO_START( taitoair );
VIDEO_UPDATE( taitoair );


/***********************************************************
				MEMORY handlers
***********************************************************/

static WRITE16_HANDLER( system_control_w )
{
	if ((ACCESSING_LSB == 0) && ACCESSING_MSB)
	{
		data >>= 8;
	}

	dsp_HOLD_signal = (data & 4) ? CLEAR_LINE : ASSERT_LINE;

	cpu_set_reset_line(2,(data & 1) ? CLEAR_LINE : ASSERT_LINE);

	log_cb(RETRO_LOG_DEBUG, LOGPRE "68K:%06x writing %04x to TMS32025.  %s HOLD , %s RESET\n",activecpu_get_previouspc(),data,((data & 4) ? "Clear" : "Assert"),((data & 1) ? "Clear" : "Assert"));
}

static READ16_HANDLER( lineram_r )
{
	return taitoair_line_ram[offset];
}

static WRITE16_HANDLER( lineram_w )
{
	if (ACCESSING_MSB && ACCESSING_LSB)
		taitoair_line_ram[offset] = data;
}

static READ16_HANDLER( dspram_r )
{
	return dsp_ram[offset];
}

static WRITE16_HANDLER( dspram_w )
{
	if (ACCESSING_MSB && ACCESSING_LSB)
		dsp_ram[offset] = data;
}

static READ16_HANDLER( dsp_HOLD_signal_r )
{
	/* HOLD signal is active low */
	/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "TMS32025:%04x Reading %01x level from HOLD signal\n",activecpu_get_previouspc(),dsp_HOLD_signal);*/

	return dsp_HOLD_signal;
}

static WRITE16_HANDLER( dsp_HOLDA_signal_w )
{
	if (offset)
		log_cb(RETRO_LOG_DEBUG, LOGPRE "TMS32025:%04x Writing %01x level to HOLD-Acknowledge signal\n",activecpu_get_previouspc(),data);
}


static WRITE16_HANDLER( airsys_paletteram16_w )	/* xxBBBBxRRRRxGGGG */
{
	int a,r,g,b;
	COMBINE_DATA(&paletteram16[offset]);

	a = paletteram16[offset];

	r = (a >> 0) & 0x0f;
	g = (a >> 5) & 0x0f;
	b = (a >> 10) & 0x0f;

	r = (r << 4) | r;
	g = (g << 4) | g;
	b = (b << 4) | b;

	palette_set_color(offset,r,g,b);
}


/***********************************************************
				INPUTS
***********************************************************/

static READ16_HANDLER( stick_input_r )
{
	switch( offset )
	{
		case 0x00:	/* "counter 1" lo */
			return input_port_4_word_r(0,0);

		case 0x01:	/* "counter 2" lo */
			return input_port_5_word_r(0,0);

		case 0x02:	/* "counter 1" hi */
			return (input_port_4_word_r(0,0) &0xff00) >> 8;

		case 0x03:	/* "counter 2" hi */
			return (input_port_5_word_r(0,0) &0xff00) >> 8;
	}

	return 0;
}

static READ16_HANDLER( stick2_input_r )
{
	switch( offset )
	{
		case 0x00:	/* "counter 3" lo */
			return input_port_6_word_r(0,0);

		case 0x02:	/* "counter 3" hi */
			return (input_port_6_word_r(0,0) &0xff00) >> 8;
	}

	return 0;
}


static int banknum = -1;

static void reset_sound_region(void)
{
	cpu_setbank(1, memory_region(REGION_CPU2) + (banknum * 0x4000) + 0x10000);
}

static WRITE_HANDLER( sound_bankswitch_w )
{
	banknum = (data - 1) & 3;
	reset_sound_region();
}


/***********************************************************
			 MEMORY STRUCTURES
***********************************************************/

static MEMORY_READ16_START( airsys_readmem )
	{ 0x000000, 0x0bffff, MRA16_ROM },
	{ 0x0c0000, 0x0cffff, MRA16_RAM },			/* 68000 RAM */
	{ 0x180000, 0x183fff, MRA16_RAM },			/* "gradiation ram (0)" */
	{ 0x184000, 0x187fff, MRA16_RAM },			/* "gradiation ram (1)" */
	{ 0x188000, 0x18bfff, paletteram16_word_r },/* "color ram" */
	{ 0x800000, 0x820fff, TC0080VCO_word_r },	/* tilemaps, sprites */
	{ 0x908000, 0x90ffff, MRA16_RAM },			/* "line ram" */
	{ 0x910000, 0x91ffff, MRA16_RAM },			/* "dsp common ram" (TMS320C25) */
	{ 0xa00000, 0xa00007, stick_input_r },
	{ 0xa00100, 0xa00107, stick2_input_r },
	{ 0xa00200, 0xa0020f, TC0220IOC_halfword_r },	/* other I/O */
	{ 0xa80000, 0xa80001, MRA16_NOP },
	{ 0xa80002, 0xa80003, taitosound_comm16_lsb_r },
	{ 0xb00000, 0xb007ff, MRA16_RAM },			/* "power common ram" (mecha drive) */
MEMORY_END

static MEMORY_WRITE16_START( airsys_writemem )
	{ 0x000000, 0x0bffff, MWA16_ROM },
	{ 0x0c0000, 0x0cffff, MWA16_RAM, &taitoh_68000_mainram },
	{ 0x140000, 0x140001, system_control_w },	/* Pause the TMS32025 */
	{ 0x180000, 0x183fff, MWA16_RAM },			/* "gradiation ram (0)" */
	{ 0x184000, 0x187fff, MWA16_RAM },			/* "gradiation ram (1)" */
	{ 0x188000, 0x18bfff, airsys_paletteram16_w, &paletteram16 },
/*	{ 0x188000, 0x18bfff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },*/
	{ 0x800000, 0x820fff, TC0080VCO_word_w },		/* tilemaps, sprites */
	{ 0x908000, 0x90ffff, MWA16_RAM, &taitoair_line_ram },	/* "line ram" */
	{ 0x910000, 0x91ffff, MWA16_RAM, &dsp_ram },	/* "dsp common ram" (TMS320C25) */
	{ 0xa00200, 0xa0020f, TC0220IOC_halfword_w },	/* I/O */
	{ 0xa80000, 0xa80001, taitosound_port16_lsb_w },
	{ 0xa80002, 0xa80003, taitosound_comm16_lsb_w },
	{ 0xb00000, 0xb007ff, MWA16_RAM },			/* "power common ram" (mecha drive) */
MEMORY_END

/************************** Z80 ****************************/

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x7fff, MRA_BANK1 },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe000, YM2610_status_port_0_A_r },
	{ 0xe001, 0xe001, YM2610_read_port_0_r },
	{ 0xe002, 0xe002, YM2610_status_port_0_B_r },
	{ 0xe200, 0xe200, MRA_NOP },
	{ 0xe201, 0xe201, taitosound_slave_comm_r },
	{ 0xea00, 0xea00, MRA_NOP },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe000, YM2610_control_port_0_A_w },
	{ 0xe001, 0xe001, YM2610_data_port_0_A_w },
	{ 0xe002, 0xe002, YM2610_control_port_0_B_w },
	{ 0xe003, 0xe003, YM2610_data_port_0_B_w },
	{ 0xe200, 0xe200, taitosound_slave_port_w },
	{ 0xe201, 0xe201, taitosound_slave_comm_w },
	{ 0xe400, 0xe403, MWA_NOP },		/* pan control */
	{ 0xee00, 0xee00, MWA_NOP }, 		/* ? */
	{ 0xf000, 0xf000, MWA_NOP }, 		/* ? */
	{ 0xf200, 0xf200, sound_bankswitch_w },
MEMORY_END

/********************************** TMS32025 ********************************/
static MEMORY_READ16_START( DSP_readmem )
	{ TMS32025_INTERNAL_MEMORY_BLOCKS_READ },
	{ TMS32025_DATA_ADDR_RANGE(0x4000, 0x7fff), lineram_r },
	{ TMS32025_DATA_ADDR_RANGE(0x8000, 0xffff), dspram_r },

	{ TMS32025_PRGM_ADDR_RANGE(0x0000, 0x1fff), MRA16_ROM },
MEMORY_END

static MEMORY_WRITE16_START( DSP_writemem )
	{ TMS32025_INTERNAL_MEMORY_BLOCKS_WRITE },
	{ TMS32025_DATA_ADDR_RANGE(0x4000, 0x7fff), lineram_w },
	{ TMS32025_DATA_ADDR_RANGE(0x8000, 0xffff), dspram_w },

	{ TMS32025_PRGM_ADDR_RANGE(0x0000, 0x1fff), MWA16_ROM },
MEMORY_END

static PORT_READ16_START( DSP_readport )
	{ TMS32025_PORT_RANGE( TMS32025_HOLD, TMS32025_HOLD ), dsp_HOLD_signal_r },
PORT_END

static PORT_WRITE16_START( DSP_writeport )
	{ TMS32025_PORT_RANGE( TMS32025_HOLDA, TMS32025_HOLDA ), dsp_HOLDA_signal_w },
PORT_END



/************************************************************
			   INPUT PORTS & DIPS
************************************************************/

#define TAITO_COINAGE_JAPAN_8 \
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) ) \
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) ) \
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) ) \
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) ) \
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

#define TAITO_COINAGE_US_8 \
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coinage ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) ) \
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) ) \
	PORT_DIPNAME( 0xc0, 0xc0, "Price to Continue" ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0xc0, "Same as Start" )

#define TAITO_DIFFICULTY_8 \
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) ) \
	PORT_DIPSETTING(    0x02, "Easy" ) \
	PORT_DIPSETTING(    0x03, "Medium" ) \
	PORT_DIPSETTING(    0x01, "Hard" ) \
	PORT_DIPSETTING(    0x00, "Hardest" )

INPUT_PORTS_START( topland )
	PORT_START  /* DSWA */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, "Mechanized (alt)?" )
	PORT_DIPSETTING(    0x01, "Standard (alt) ?" )
	PORT_DIPSETTING(    0x02, "Mechanized" )
	PORT_DIPSETTING(    0x03, "Standard" )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	TAITO_COINAGE_JAPAN_8

	PORT_START  /* DSWB, all bogus !!! */
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x08, "1000k only" )
	PORT_DIPSETTING(    0x0c, "1500k only" )
	PORT_DIPSETTING(    0x04, "2000k only" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_BUTTON3 | IPF_PLAYER1 )	/* "door" (!) */

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER1 )	/* slot down */
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_PLAYER1 )	/* slot up */
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )	/* handle */
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON4 | IPF_PLAYER1 )	/* freeze ??? */
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	/* The range of these sticks reflects the range test mode displays.
	   Eventually we want standard 0-0xff input range and a scale-up later
	   in the stick_r routines.  And fake DSW with self-centering option
	   to make keyboard control feasible! */

	PORT_START  /* Stick 1 (4) */
	PORT_ANALOG( 0xffff, 0x0000, IPT_AD_STICK_X | IPF_PLAYER1, 30, 40, 0xf800, 0x7ff )

	PORT_START  /* Stick 2 (5) */
	PORT_ANALOG( 0xffff, 0x0000, IPT_AD_STICK_Y | IPF_PLAYER1, 30, 40, 0xf800, 0x7ff )

	PORT_START  /* Stick 3 (6) */
	PORT_ANALOG( 0xffff, 0x0000, IPT_AD_STICK_Y | IPF_PLAYER2, 30, 40, 0xf800, 0x7ff )
INPUT_PORTS_END

INPUT_PORTS_START( ainferno )
	PORT_START  /* DSWA */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, "Mechanized (alt)?" )
	PORT_DIPSETTING(    0x01, "Special Sensors" )	/* on its test mode screen*/
	PORT_DIPSETTING(    0x02, "Mechanized" )
	PORT_DIPSETTING(    0x03, "Standard" )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	TAITO_COINAGE_US_8

	PORT_START  /* DSWB, all bogus !!! */
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x08, "1000k only" )
	PORT_DIPSETTING(    0x0c, "1500k only" )
	PORT_DIPSETTING(    0x04, "2000k only" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_START2 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER1 )	/* lever */
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_PLAYER1 )	/* handle x */
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_BUTTON3 | IPF_PLAYER1 )	/* handle y */
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_BUTTON4 | IPF_PLAYER1 )	/* fire */
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_BUTTON6 | IPF_PLAYER1 )	/* pedal r */
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_BUTTON5 | IPF_PLAYER1 )	/* pedal l */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON7 | IPF_PLAYER1 )	/* freeze (code at $7d6 hangs) */
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	/* The range of these sticks reflects the range test mode displays.
	   Eventually we want standard 0-0xff input range and a scale-up later
	   in the stick_r routines. And fake DSW with self-centering option
	   to make keyboard control feasible! */

	PORT_START  /* Stick 1 (4) */
	PORT_ANALOG( 0xffff, 0x0000, IPT_AD_STICK_X | IPF_PLAYER1, 30, 40, 0xf800, 0x7ff )

	PORT_START  /* Stick 2 (5) */
	PORT_ANALOG( 0xffff, 0x0000, IPT_AD_STICK_Y | IPF_PLAYER1, 30, 40, 0xf800, 0x7ff )

	PORT_START  /* Stick 3 (6) */
	PORT_ANALOG( 0xffff, 0x0000, IPT_AD_STICK_Y | IPF_PLAYER2, 30, 40, 0xf800, 0x7ff )
INPUT_PORTS_END


/************************************************************
				GFX DECODING
************************************************************/

static struct GfxLayout tilelayout =
{
	16,16,	/* 16x16 pixels */
	RGN_FRAC(1,4),
	4,
	{ 0, 1, 2, 3 },
	{ 4, 0, 12, 8,
	  RGN_FRAC(1,4)+4, RGN_FRAC(1,4), RGN_FRAC(1,4)+12, RGN_FRAC(1,4)+8,
	  RGN_FRAC(2,4)+4, RGN_FRAC(2,4), RGN_FRAC(2,4)+12, RGN_FRAC(2,4)+8,
	  RGN_FRAC(3,4)+4, RGN_FRAC(3,4), RGN_FRAC(3,4)+12, RGN_FRAC(3,4)+8 },
	{ 0*16, 1*16, 2*16,  3*16,  4*16,  5*16,  6*16,  7*16,
	  8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	16*16
};

static struct GfxDecodeInfo airsys_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tilelayout, 0, 32*16 },
	{ -1 } /* end of array */
};


/************************************************************
				YM2610 (SOUND)
************************************************************/

/* Handler called by the YM2610 emulator when the internal timers cause an IRQ */
static void irqhandler(int irq)
{
	cpu_set_irq_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2610interface airsys_ym2610_interface =
{
	1,	/* 1 chip */
	8000000,	/* 4 MHz */
	{ 30 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler },
	{ REGION_SOUND2 },
	{ REGION_SOUND1 },
	{ YM3012_VOL(60,MIXER_PAN_LEFT,60,MIXER_PAN_RIGHT) }
};


/************************************************************
				MACHINE DRIVERS
************************************************************/

static MACHINE_DRIVER_START( airsys )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,24000000 / 2)		/* 12 MHz ??? */
	MDRV_CPU_MEMORY(airsys_readmem,airsys_writemem)
	MDRV_CPU_VBLANK_INT(irq5_line_hold,1)

	MDRV_CPU_ADD(Z80,8000000 / 2)			/* 4 MHz ??? */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_CPU_ADD(TMS32025,24000000)			/* 24 MHz ??? */
	MDRV_CPU_MEMORY(DSP_readmem,DSP_writemem)
	MDRV_CPU_PORTS(DSP_readport,DSP_writeport)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*16, 64*16)
	MDRV_VISIBLE_AREA(0*16, 32*16-1, 3*16, 28*16-1)
	MDRV_GFXDECODE(airsys_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512*16)

	MDRV_VIDEO_START(taitoair)
	MDRV_VIDEO_UPDATE(taitoair)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2610, airsys_ym2610_interface)
MACHINE_DRIVER_END


/*************************************************************
				   DRIVERS

Ainferno may be missing an 0x2000 byte rom from the video
board - possibly?
*************************************************************/

ROM_START( topland )
	ROM_REGION( 0xc0000, REGION_CPU1, 0 )	/* 68000 */
	ROM_LOAD16_BYTE( "b62_41.43",  0x00000, 0x20000, CRC(28264798) SHA1(72e4441ad468f37cff69c36699867119ad28274c) )
	ROM_LOAD16_BYTE( "b62_40.14",  0x00001, 0x20000, CRC(db872f7d) SHA1(6932c62d8051b1811c30139dbd0375115305c731) )
	ROM_LOAD16_BYTE( "b62_25.42",  0x40000, 0x20000, CRC(1bd53a72) SHA1(ada679198739cd6a419d3fa4311bb92dc385099c) )
	ROM_LOAD16_BYTE( "b62_24.13",  0x40001, 0x20000, CRC(845026c5) SHA1(ab8d8f5f6597bfcde4e9ccf9e0181b8b6e769ada) )
	ROM_LOAD16_BYTE( "b62_23.41",  0x80000, 0x20000, CRC(ef3a971c) SHA1(0840668dda48f4c9a85410361bfba3ae9580a71f) )
	ROM_LOAD16_BYTE( "b62_22.12",  0x80001, 0x20000, CRC(94279201) SHA1(8518d8e722d4f2516f75224d9a21ab20d8ee6c78) )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )	/* Z80 */
	ROM_LOAD( "b62-42.34", 0x00000, 0x04000, CRC(389230e0) SHA1(3a336987aad7bf4df658f924de4bbe6f0fff6d59) )
	ROM_CONTINUE(          0x10000, 0x0c000 )

	ROM_REGION( 0x24000, REGION_CPU3, 0 )	/* TMS320C25 */
	ROM_LOAD16_BYTE( "b62-21.35", 0x20000, 0x02000, CRC(5f38460d) SHA1(0593718d15b30b10f7686959932e2c934de2a529) )	/* cpu board*/
	ROM_LOAD16_BYTE( "b62-20.6",  0x20001, 0x02000, CRC(a4afe958) SHA1(7593a327f4ea0cc9e28fd3269278871f62fb0598) )	/* cpu board*/

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )	/* 16x16 tiles */
	ROM_LOAD16_BYTE( "b62-33.39",  0x000000, 0x20000, CRC(38786867) SHA1(7292e3fa69cad6494f2e8e7efa9c3f989bdf958d) )
	ROM_LOAD16_BYTE( "b62-36.48",  0x000001, 0x20000, CRC(4259e76a) SHA1(eb0dc5d0a6f875e3b8335fb30d4c2ad3880c31b9) )
	ROM_LOAD16_BYTE( "b62-29.27",  0x040000, 0x20000, CRC(efdd5c51) SHA1(6df3e9782946cf6f4a21ee3d335548c53cd21e3a) )
	ROM_LOAD16_BYTE( "b62-34.40",  0x040001, 0x20000, CRC(a7e10ca4) SHA1(862c23c095f96f9e0cae00d70947782d5f4e45e6) )
	ROM_LOAD16_BYTE( "b62-35.47",  0x080000, 0x20000, CRC(cba7bac5) SHA1(5305c84abcbcc23281744454803b849853b26632) )
	ROM_LOAD16_BYTE( "b62-30.28",  0x080001, 0x20000, CRC(30e37cb8) SHA1(6bc777bdf1a56952dbfbe2f595279a43e2fa98fd) )
	ROM_LOAD16_BYTE( "b62-31.29",  0x0c0000, 0x20000, CRC(3feebfe3) SHA1(5b014d7d6fa1daf400ac1a437f551281debfdba6) )
	ROM_LOAD16_BYTE( "b62-32.30",  0x0c0001, 0x20000, CRC(66806646) SHA1(d8e0c37b5227d8583d523164ffc6828b4508d5a3) )

	ROM_REGION( 0xa0000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "b62-13.1",  0x00000, 0x20000, CRC(b37dc3ea) SHA1(198d4f828132316c624da998e49b1873b9886bf0) )
	ROM_LOAD( "b62-14.2",  0x20000, 0x20000, CRC(617948a3) SHA1(4660570fa6263c28cfae7ccdf154763cc6144896) )
	ROM_LOAD( "b62-15.3",  0x40000, 0x20000, CRC(e35ffe81) SHA1(f35afdd7cfd4c09907fb062beb5ae46c2286a381) )
	ROM_LOAD( "b62-16.4",  0x60000, 0x20000, CRC(203a5c27) SHA1(f6fc9322dea8d82bfec3be3fdc8616dc6adf666e) )
	ROM_LOAD( "b62-17.5",  0x80000, 0x20000, CRC(36447066) SHA1(91c8cc4e99534b2d533895a342abb22766a20090) )

	ROM_REGION( 0x20000, REGION_SOUND2, 0 )	/* Delta-T samples */
	ROM_LOAD( "b62-18.31", 0x00000, 0x20000, CRC(3a4e687a) SHA1(43f07fe19dec351e851defdf9c7810fb9df04736) )

	ROM_REGION( 0x02000, REGION_USER1, 0 )	/* unknown */
	ROM_LOAD( "b62-28.22", 0x00000, 0x02000, CRC(c4be68a6) SHA1(2c07a0e71d11bca67427331217c507d849500ec1) )	/* video board*/
ROM_END

ROM_START( ainferno )
	ROM_REGION( 0xc0000, REGION_CPU1, 0 )	/* 68000 */
	ROM_LOAD16_BYTE( "c45_22.43", 0x00000, 0x20000, CRC(50300926) SHA1(9c2a60282d3f9f115b94cb5b6d64bbfc9d726d1d) )
	ROM_LOAD16_BYTE( "c45_20.14", 0x00001, 0x20000, CRC(39b189d9) SHA1(002013c02b546d3f5a9f3a3149971975a73cc8ce) )
	ROM_LOAD16_BYTE( "c45_21.42", 0x40000, 0x20000, CRC(1b687241) SHA1(309e42f79cbd48ceae58a15afb648aef838822f0) )
	ROM_LOAD16_BYTE( "c45_28.13", 0x40001, 0x20000, CRC(c7cd2567) SHA1(cf1f163ec252e9986132095f22bca8d061bfdf9a) )

	/* 0x80000 to 0xbffff is empty for this game */

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )	/* Z80 */
	ROM_LOAD( "c45-23.34", 0x00000, 0x04000, CRC(d0750c78) SHA1(63232c2acef86e8c8ffaad36ab0b6c4cc1eb48f8) )
	ROM_CONTINUE(          0x10000, 0x0c000 )

	ROM_REGION( 0x24000, REGION_CPU3, 0 )	/* TMS320C25 */
	ROM_LOAD16_BYTE( "c45-25.35", 0x20000, 0x02000, CRC(c0d39f95) SHA1(542aa6e2af510aea00db40bf803cb6653d4e7747) )
	ROM_LOAD16_BYTE( "c45-24.6",  0x20001, 0x02000, CRC(1013d937) SHA1(817769d21583f5281ba044ce8c134c9239d1e83e) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )	/* 16x16 tiles */
	ROM_LOAD16_BYTE( "c45-11.28", 0x000000, 0x20000, CRC(d9b4b77c) SHA1(69d570efa8146fb0a712ff45e77bda6fd85769f8) )
	ROM_LOAD16_BYTE( "c45-15.40", 0x000001, 0x20000, CRC(d4610698) SHA1(5de519a23300d5b3b09ce7cf8c02a1a6b2fb985c) )
	ROM_LOAD16_BYTE( "c45-12.29", 0x040000, 0x20000, CRC(4ae305b8) SHA1(2bbb981853a7abbba90afb8eb58f6869357551d3) )
	ROM_LOAD16_BYTE( "c45-16.41", 0x040001, 0x20000, CRC(c6eb93b0) SHA1(d0b1adfce5c1f4e21c5d84527d22ace14578f2d7) )
	ROM_LOAD16_BYTE( "c45-13.30", 0x080000, 0x20000, CRC(69b82af6) SHA1(13c035e84affa59734c6dd1b07963c08654b5f5a) )
	ROM_LOAD16_BYTE( "c45-17.42", 0x080001, 0x20000, CRC(0dbee000) SHA1(41073d5cf20df12d5ba1c424c9d9f0b2d9836d5d) )
	ROM_LOAD16_BYTE( "c45-14.31", 0x0c0000, 0x20000, CRC(481b6f29) SHA1(0b047e805663b144dc2388c86438950fcdc29658) )
	ROM_LOAD16_BYTE( "c45-18.43", 0x0c0001, 0x20000, CRC(ba7ecf3b) SHA1(dd073b7bfbf2f88432337027ae9fb6c4f02a538f) )

	ROM_REGION( 0xa0000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "c45-01.5",  0x00000, 0x20000, CRC(052997b2) SHA1(3aa8b4f759a1c196de39754a9ccdf4fabdbab388) )
	ROM_LOAD( "c45-02.4",  0x20000, 0x20000, CRC(2fc0a88e) SHA1(6a635671fa2518f74015429ce580d7b7f00299ad) )
	ROM_LOAD( "c45-03.3",  0x40000, 0x20000, CRC(0e1e5b5f) SHA1(a53d5ba01825f825e31a014cb4808f59ef86f0c9) )
	ROM_LOAD( "c45-04.2",  0x60000, 0x20000, CRC(6d081044) SHA1(2d98bde55621762509dfc645d9ca5e267b1757ae) )
	ROM_LOAD( "c45-05.1",  0x80000, 0x20000, CRC(6c59a808) SHA1(6264bbe4d7ad3070c6441859eb704a42910a82f0) )

	ROM_REGION( 0x20000, REGION_SOUND2, 0 )	/* Delta-T samples */
	ROM_LOAD( "c45-06.31", 0x00000, 0x20000, CRC(6a7976d4) SHA1(a465f9bb874b1eff08742b33cc3c364703b281ca) )

	ROM_REGION( 0x02000, REGION_USER1, 0 )
	ROM_LOAD( "c45-xx.22", 0x00000, 0x02000, NO_DUMP )	/* video board*/
	/* Readme says 7 pals on video board and 6 on cpu board */
ROM_END


static DRIVER_INIT( taitoair )
{
	dsp_HOLD_signal = ASSERT_LINE;

	state_save_register_int("sound1", 0, "sound region", &banknum);
	state_save_register_func_postload(reset_sound_region);
}



/*   ( YEAR  NAME      PARENT    MACHINE   INPUT     INIT      MONITOR  COMPANY  FULLNAME */
GAMEX( 1988, topland,  0,        airsys,   topland,  taitoair, ROT0,    "Taito Corporation Japan", "Top Landing (World)", GAME_NOT_WORKING )
GAMEX( 1990, ainferno, 0,        airsys,   ainferno, taitoair, ROT0,    "Taito America Corporation", "Air Inferno (US)", GAME_NOT_WORKING )
