/***************************************************************************

Top Speed / Full Throttle    (c) Taito 1987
-------------------------

David Graves

Sources:		Rastan driver by Jarek Burczynski
			MAME Taito F2 & Z drivers
			Raine source - special thanks to Richard Bush
			  and the Raine Team.

				*****

Top Speed / Full Throttle is the forerunner of the Taito Z system on
which Taito's driving games were based from 1988-91. (You can spot some
similarities with Continental Circus, the first of the TaitoZ games.)

The game hardware has 5 separate layers of graphics - four 64x64 tiled
scrolling background planes of 8x8 tiles (two of which are used for
drawing the road), and a sprite plane.

Taito got round the limitations of the tilemap generator they were using
(which only supports two layers) by using a pair of them.

[Trivia: Taito employed the same trick three years later, this time with
the TC0100SCN in "Thunderfox".]

Top Speed's sprites are 16x8 tiles aggregated through a RAM sprite map
area into 128x128 big sprites. (The TaitoZ system also used a similar
sprite map system, but moved the sprite map from RAM to ROM.)

Top Speed has twin 68K CPUs which communicate via $10000 bytes of
shared ram. The first 68000 handles screen, palette and sprites, and
the road. The second 68000 handles inputs/dips, and does data processing
in shared ram to relieve CPUA. There is also a Z80, which takes over
sound duties.


Dumper's info (topspedu)
-------------

Main CPUs: Dual 68000
Sound: YM2151, OKI M5205

Some of the custom Taito chips look like Rastan Hardware

Comments: Note b14-06, and b14-07, are duplicated twice on this board
for some type of hardware graphics reasons. (DG: that's because of
the twin tilemap generator chips. Can someone confirm they are
PC080SN's please...?)

There is a weird chip that is probably a Microcontroller made by Sharp.
Part number: b14-31 - Sharp LH763J-70


TODO Lists
==========

(Want to verify 68000 clocks)

Accel and brake bits work differently depending on cab DSW
Mame cannot yet support this, so accel/brake are not hooked up
sensibly when upright cabinet is selected.

The 8 level brake and accel inputs for the cockpit version
should be mapped to a pedal for analogue pedal control.
(Warlock did this but his changes need remerging.)

Minor black glitches on the road: these are all on the right
hand edge of the tilemap making up the "left" half: this is
the upper of the two road tilemaps so any gunk will be visible.

'Tearing' effect between the two road tilemaps is visible when
single-stepping. A sync issue?

*Loads* of complaints from the Taito sound system in the log.

CPUA (on all variants) could have a spin_until_int at $63a.

Motor CPU: appears to be identical to one in ChaseHQ.

DIPs


Raster line color control
-------------------------

Used to make the road move. Each word controls one pixel row.

0x800000 - 0x1ff  raster color control for one road tilemap
0x800200 - 0x3ff  raster color control for the other

Road tile colors are (all?) in the range 0x100-103. Top road section
(tilemap at 0xa08000) uses 0x100 and 0x101. Bottom section
(tilemap at 0xb00000) uses 0x102 and 0x103. This would allow colors
on left and right side of road to be different. In practice it seems
Taito didn't take advantage of this.

Each tilemap is usually all one color value. Every now and then (10s
or so) the value alternates. This seems to be determined by whether
the current section of road has white lines in the middle. (0x101/3
gives white lines.)

The raster line color control area has groups of four values which
cascade down through it so the road colors cascade down the screen.

There are three known groups (start is arbitrary; the cycles repeat ad
infinitum or until a different cycle starts; values given are from bottom
to top of screen):

(i) White lines in center of road

12	%10010
1f	%11111
00	%00000
0d	%01101

(ii) No lines in center of road

08	%01000
0c	%01100
1a	%11010
1e	%11110

(iii) Under bridge or in tunnel [note almost identical to (i)]

ffe0	%00000
ffed	%01101
fff2	%10010
ffef	%01111

(iv) Unknown 4th group for tunnels in later parts of the game that have
no white lines, analogous to (ii) ?


Correlating with screenshots suggests that these bits refer to:

x....  road body ?
.x...  lines in road center and inner edge
..x..  lines at road outer edge
...x.  outside road ?
....x  ???


Actual gfx tiles used for the road only use colors 1-5. Palette offsets:

(0 = transparency)
1 = lines in road center
2 = road edge (inner)
3 = road edge (outer)
4 = road body
5 = outside road

Each palette block contains three possible sets of 5 colors. Entries 1-5
(standard), 6-10 (alternate), 11-15 (tunnels).

In tunnels only 11-15 are used. Outside tunnels there is a choice between
the standard colors and the alternate colors. The road body could in theory
take a standard color while 'outside the road' took on an alternate. But
in practice the game is using a very limited choice of raster control words,
so we don't know.

Need to test whether sections of the road with unknown raster control words
(tunnels late in the game without central white lines) are correct against
a real machine.

Also are the 'prelines' shortly before white road lines appear correct?



CHECK screen inits at $1692

These suggest that rowscroll areas are all 0x1000 long and there are TWO
for each tilemap layer.

256 rows => 256 words => 0x200 bytes. So probably the inits are far too long.

Maybe the second area for each layer contains colscroll ?

***************************************************************************/

#include "driver.h"
#include "state.h"
#include "cpu/m68000/m68000.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/taitoic.h"
#include "sndhrdw/taitosnd.h"

WRITE16_HANDLER( rainbow_spritectrl_w );
WRITE16_HANDLER( rastan_spriteflip_w );

VIDEO_START( topspeed );
VIDEO_UPDATE( topspeed );

WRITE_HANDLER( rastan_adpcm_trigger_w );
WRITE_HANDLER( rastan_c000_w );
WRITE_HANDLER( rastan_d000_w );

static UINT16 cpua_ctrl = 0xff;
static int ioc220_port = 0;

extern data16_t *topspeed_spritemap;

static size_t sharedram_size;
static data16_t *sharedram;

extern data16_t *topspeed_raster_ctrl;


static READ16_HANDLER( sharedram_r )
{
	return sharedram[offset];
}

static WRITE16_HANDLER( sharedram_w )
{
	COMBINE_DATA(&sharedram[offset]);
}

static void parse_control(void)	/* assumes Z80 sandwiched between 68Ks */
{
	/* bit 0 enables cpu B */
	/* however this fails when recovering from a save state
	   if cpu B is disabled !! */
	cpu_set_reset_line(2,(cpua_ctrl &0x1) ? CLEAR_LINE : ASSERT_LINE);

}

static WRITE16_HANDLER( cpua_ctrl_w )
{
	if ((data &0xff00) && ((data &0xff) == 0))
		data = data >> 8;	/* for Wgp */
	cpua_ctrl = data;

	parse_control();

	log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #0 PC %06x: write %04x to cpu control\n",activecpu_get_pc(),data);
}


/***********************************************************
                        INTERRUPTS
***********************************************************/

/* 68000 A */

void topspeed_interrupt6(int x)
{
	cpu_set_irq_line(0,6,HOLD_LINE);
}

/* 68000 B */

void topspeed_cpub_interrupt6(int x)
{
	cpu_set_irq_line(2,6,HOLD_LINE);	/* assumes Z80 sandwiched between the 68Ks */
}


static INTERRUPT_GEN( topspeed_interrupt )
{
	/* Unsure how many int6's per frame */
	timer_set(TIME_IN_CYCLES(200000-500,0),0, topspeed_interrupt6);
	cpu_set_irq_line(0, 5, HOLD_LINE);
}

static INTERRUPT_GEN( topspeed_cpub_interrupt )
{
	/* Unsure how many int6's per frame */
	timer_set(TIME_IN_CYCLES(200000-500,0),0, topspeed_cpub_interrupt6);
	cpu_set_irq_line(2, 5, HOLD_LINE);
}



/**********************************************************
                       GAME INPUTS
**********************************************************/

static READ16_HANDLER( topspeed_input_bypass_r )
{
	UINT8 port = TC0220IOC_port_r(0);	/* read port number */
	int steer = 0;
	int analogue_steer = input_port_5_word_r(0,0);
	int fake = input_port_6_word_r(0,0);

	if (!(fake &0x10))	/* Analogue steer (the real control method) */
	{
		steer = analogue_steer;

	}
	else	/* Digital steer */
	{
		if (fake & 0x8)	/* pressing down */
			steer = 0xff40;

		if (fake & 0x2)	/* pressing right */
			steer = 0x007f;

		if (fake & 0x1)	/* pressing left */
			steer = 0xff80;

		/* To allow hiscore input we must let you return to
		   continuous input type while you press up */

		if (fake & 0x4)	/* pressing up */
			steer = analogue_steer;
	}

	switch (port)
	{
		case 0x0c:
			return steer &0xff;

		case 0x0d:
			return steer >> 8;

		default:
			return TC0220IOC_portreg_r(offset);
	}
}


static READ16_HANDLER( topspeed_motor_r )
{
	switch (offset)
	{
		case 0x0:
			return (rand() &0xff);	/* motor status ?? */

		case 0x101:
			return 0x55;	/* motor cpu status ? */

		default:
log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #0 PC %06x: warning - read from motor cpu %03x\n",activecpu_get_pc(),offset);
			return 0;
	}
}

static WRITE16_HANDLER( topspeed_motor_w )
{
	/* Writes $900000-25 and $900200-219 */

log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #0 PC %06x: warning - write %04x to motor cpu %03x\n",activecpu_get_pc(),data,offset);

}


/*****************************************************
                        SOUND
*****************************************************/

static int banknum = -1;

static void reset_sound_region(void)
{
	cpu_setbank( 10, memory_region(REGION_CPU2) + (banknum * 0x4000) + 0x10000 );
}

static WRITE_HANDLER( sound_bankswitch_w )	/* assumes Z80 sandwiched between 68Ks */
{
	banknum = (data - 1) & 7;
	reset_sound_region();
}


/***********************************************************
                      MEMORY STRUCTURES
***********************************************************/


static MEMORY_READ16_START( topspeed_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM },
	{ 0x400000, 0x40ffff, sharedram_r },	/* all shared ??*/
	{ 0x500000, 0x503fff, paletteram16_word_r },
	{ 0x7e0000, 0x7e0001, MRA16_NOP },
	{ 0x7e0002, 0x7e0003, taitosound_comm16_lsb_r },
	{ 0x800000, 0x8003ff, MRA16_RAM },	/* raster line color control */
	{ 0x800400, 0x80ffff, MRA16_RAM },	/* unknown or unused */
	{ 0xa00000, 0xa0ffff, PC080SN_word_0_r },	/* tilemaps */
	{ 0xb00000, 0xb0ffff, PC080SN_word_1_r },	/* tilemaps */
	{ 0xd00000, 0xd00fff, MRA16_RAM },	/* sprite ram */
	{ 0xe00000, 0xe0ffff, MRA16_RAM },	/* sprite map */
MEMORY_END

static MEMORY_WRITE16_START( topspeed_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM },
	{ 0x400000, 0x40ffff, sharedram_w, &sharedram, &sharedram_size },
	{ 0x500000, 0x503fff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
	{ 0x600002, 0x600003, cpua_ctrl_w },
	{ 0x7e0000, 0x7e0001, taitosound_port16_lsb_w },
	{ 0x7e0002, 0x7e0003, taitosound_comm16_lsb_w },
	{ 0x800000, 0x8003ff, MWA16_RAM, &topspeed_raster_ctrl },
	{ 0x800400, 0x80ffff, MWA16_RAM },
	{ 0xa00000, 0xa0ffff, PC080SN_word_0_w },
	{ 0xa20000, 0xa20003, PC080SN_yscroll_word_0_w },
	{ 0xa40000, 0xa40003, PC080SN_xscroll_word_0_w },
	{ 0xa50000, 0xa50003, PC080SN_ctrl_word_0_w },
	{ 0xb00000, 0xb0ffff, PC080SN_word_1_w },
	{ 0xb20000, 0xb20003, PC080SN_yscroll_word_1_w },
	{ 0xb40000, 0xb40003, PC080SN_xscroll_word_1_w },
	{ 0xb50000, 0xb50003, PC080SN_ctrl_word_1_w },
	{ 0xd00000, 0xd00fff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0xe00000, 0xe0ffff, MWA16_RAM, &topspeed_spritemap },
MEMORY_END

static MEMORY_READ16_START( topspeed_cpub_readmem )
	{ 0x000000, 0x01ffff, MRA16_ROM },
	{ 0x400000, 0x40ffff, sharedram_r },
	{ 0x880000, 0x880001, topspeed_input_bypass_r },
	{ 0x880002, 0x880003, TC0220IOC_halfword_port_r },
	{ 0x900000, 0x9003ff, topspeed_motor_r },	/* motor CPU */
MEMORY_END

static MEMORY_WRITE16_START( topspeed_cpub_writemem )
	{ 0x000000, 0x01ffff, MWA16_ROM },
	{ 0x400000, 0X40ffff, sharedram_w, &sharedram },
	{ 0x880000, 0x880001, TC0220IOC_halfword_portreg_w },
	{ 0x880002, 0x880003, TC0220IOC_halfword_port_w },
	{ 0x900000, 0x9003ff, topspeed_motor_w },	/* motor CPU */
MEMORY_END


/***************************************************************************/

static MEMORY_READ_START( z80_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x7fff, MRA_BANK10 },
	{ 0x8000, 0x8fff, MRA_RAM },
	{ 0x9001, 0x9001, YM2151_status_port_0_r },
	{ 0x9002, 0x9100, MRA_RAM },
	{ 0xa001, 0xa001, taitosound_slave_comm_r },
MEMORY_END

static MEMORY_WRITE_START( z80_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x8fff, MWA_RAM },
	{ 0x9000, 0x9000, YM2151_register_port_0_w },
	{ 0x9001, 0x9001, YM2151_data_port_0_w },
	{ 0xa000, 0xa000, taitosound_slave_port_w },
	{ 0xa001, 0xa001, taitosound_slave_comm_w },
	{ 0xb000, 0xb000, rastan_adpcm_trigger_w },
	{ 0xc000, 0xc000, rastan_c000_w },
	{ 0xd000, 0xd000, rastan_d000_w },
MEMORY_END


/***********************************************************
                    INPUT PORTS, DIPs
***********************************************************/

#define TAITO_COINAGE_WORLD_8 \
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) ) \
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) ) \
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) ) \
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) ) \
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

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

INPUT_PORTS_START( topspeed )
	PORT_START /* DSW A */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x03, "Deluxe Motorized Cockpit" )
	PORT_DIPSETTING(    0x02, "Upright (?)" )
	PORT_DIPSETTING(    0x01, "Upright (alt?)" )
	PORT_DIPSETTING(    0x00, "Standard Cockpit" )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	TAITO_COINAGE_WORLD_8

	PORT_START /* DSW B */
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x0c, 0x0c, "Initial Time" )
	PORT_DIPSETTING(    0x00, "40 seconds" )
	PORT_DIPSETTING(    0x04, "50 seconds" )
	PORT_DIPSETTING(    0x0c, "60 seconds" )
	PORT_DIPSETTING(    0x08, "70 seconds" )
	PORT_DIPNAME( 0x30, 0x30, "Nitros" )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x40, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	/* Next bit is brake key (active low) for non-cockpit */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON6 | IPF_PLAYER1 )	/* 3 for brake [7 levels] */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON8 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER1 )	/* main brake key */

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_BUTTON3 | IPF_PLAYER1 )	/* nitro */
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_TILT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON4 | IPF_PLAYER1 )	/* gear shift lo/hi */
	/* Next bit is accel key (active low/high, depends on cab DSW) for non-cockpit */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON5 | IPF_PLAYER1 )	/* 3 for accel [7 levels] */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON7 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )	/* main accel key */

	PORT_START      /* IN2, unused */

	/* Note that sensitivity is chosen to suit keyboard control (for
	   sound selection in test mode and hi score name entry). With
	   an analogue wheel, the user will need to adjust this. */

	PORT_START	/* continuous steer */
	PORT_ANALOG( 0xffff, 0x00, IPT_AD_STICK_X | IPF_PLAYER1, 10, 2, 0xff7f, 0x80)

	PORT_START      /* fake, allowing digital steer */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_DIPNAME( 0x10, 0x10, "Steering type" )
	PORT_DIPSETTING(    0x10, "Digital" )
	PORT_DIPSETTING(    0x00, "Analogue" )
INPUT_PORTS_END

INPUT_PORTS_START( topspedu )
	PORT_START /* DSW A */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x03, "Deluxe Motorized Cockpit" )
	PORT_DIPSETTING(    0x02, "Upright (?)" )
	PORT_DIPSETTING(    0x01, "Upright (alt?)" )
	PORT_DIPSETTING(    0x00, "Standard Cockpit" )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	TAITO_COINAGE_WORLD_8

	PORT_START /* DSW B */
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x0c, 0x0c, "Initial Time" )
	PORT_DIPSETTING(    0x00, "40 seconds" )
	PORT_DIPSETTING(    0x04, "50 seconds" )
	PORT_DIPSETTING(    0x0c, "60 seconds" )
	PORT_DIPSETTING(    0x08, "70 seconds" )
	PORT_DIPNAME( 0x30, 0x30, "Nitros" )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x40, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	/* Next bit is brake key (active low) for non-cockpit */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON6 | IPF_PLAYER1 )	/* 3 for brake [7 levels] */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON8 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER1 )	/* main brake key */

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_BUTTON3 | IPF_PLAYER1 )	/* nitro */
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_TILT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON4 | IPF_PLAYER1 )	/* gear shift lo/hi */
	/* Next bit is accel key (active low/high, depends on cab DSW) for non-cockpit */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON5 | IPF_PLAYER1 )	/* 3 for accel [7 levels] */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON7 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )	/* main accel key */

	PORT_START      /* IN2, unused */

	/* Note that sensitivity is chosen to suit keyboard control (for
	   sound selection in test mode and hi score name entry). With
	   an analogue wheel, the user will need to adjust this. */

	PORT_START	/* continuous steer */
	PORT_ANALOG( 0xffff, 0x00, IPT_AD_STICK_X | IPF_PLAYER1, 10, 2, 0xff7f, 0x80)

	PORT_START      /* fake, allowing digital steer */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_DIPNAME( 0x10, 0x10, "Steering type" )
	PORT_DIPSETTING(    0x10, "Digital" )
	PORT_DIPSETTING(    0x00, "Analogue" )
INPUT_PORTS_END

INPUT_PORTS_START( fullthrl )
	PORT_START /* DSW A */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x03, "Deluxe Motorized Cockpit" )
	PORT_DIPSETTING(    0x02, "Upright (?)" )
	PORT_DIPSETTING(    0x01, "Upright (alt?)" )
	PORT_DIPSETTING(    0x00, "Standard Cockpit" )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	TAITO_COINAGE_WORLD_8

	PORT_START /* DSW B */
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x0c, 0x0c, "Initial Time" )
	PORT_DIPSETTING(    0x00, "40 seconds" )
	PORT_DIPSETTING(    0x04, "50 seconds" )
	PORT_DIPSETTING(    0x0c, "60 seconds" )
	PORT_DIPSETTING(    0x08, "70 seconds" )
	PORT_DIPNAME( 0x30, 0x30, "Nitros" )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x40, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	/* Next bit is brake key (active low) for non-cockpit */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON6 | IPF_PLAYER1 )	/* 3 for brake [7 levels] */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON8 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER1 )	/* main brake key */

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_BUTTON3 | IPF_PLAYER1 )	/* nitro */
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_TILT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON4 | IPF_PLAYER1 )	/* gear shift lo/hi */
	/* Next bit is accel key (active low/high, depends on cab DSW) for non-cockpit */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON5 | IPF_PLAYER1 )	/* 3 for accel [7 levels] */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON7 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )	/* main accel key */

	PORT_START      /* IN2, unused */

	/* Note that sensitivity is chosen to suit keyboard control (for
	   sound selection in test mode and hi score name entry). With
	   an analogue wheel, the user will need to adjust this. */

	PORT_START	/* continuous steer */
	PORT_ANALOG( 0xffff, 0x00, IPT_AD_STICK_X | IPF_PLAYER1, 10, 2, 0xff7f, 0x80)

	PORT_START      /* fake, allowing digital steer */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_DIPNAME( 0x10, 0x10, "Steering type" )
	PORT_DIPSETTING(    0x10, "Digital" )
	PORT_DIPSETTING(    0x00, "Analogue" )
INPUT_PORTS_END


/**************************************************************
                        GFX DECODING
**************************************************************/

static struct GfxLayout tile16x8_layout =
{
	16,8,	/* 16*8 sprites */
	RGN_FRAC(1,1),
	4,	/* 4 bits per pixel */
	{ 0, 8, 16, 24 },
	{ 32, 33, 34, 35, 36, 37, 38, 39, 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64 },
	64*8	/* every sprite takes 64 consecutive bytes */
};

static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	RGN_FRAC(1,1),
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 2*4, 3*4, 0*4, 1*4, 6*4, 7*4, 4*4, 5*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxDecodeInfo topspeed_gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0x0, &tile16x8_layout,  0, 256 },	/* sprite parts */
	{ REGION_GFX1, 0x0, &charlayout,  0, 256 },		/* sprites & playfield */
	/* Road Lines gfxdecodable ?*/
	{ -1 } /* end of array */
};


/**************************************************************
                        YM2151 (SOUND)
**************************************************************/

/* handler called by the YM2151 emulator when the internal timers cause an IRQ */

static void irq_handler(int irq)	/* assumes Z80 sandwiched between 68Ks */
{
	cpu_set_irq_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	4000000,	/* 4 MHz ? */
	{ YM3012_VOL(50,MIXER_PAN_CENTER,50,MIXER_PAN_CENTER) },
	{ irq_handler },
	{ sound_bankswitch_w }
};

static struct ADPCMinterface adpcm_interface =
{
	1,			/* 1 chip */
	8000,       /* 8000Hz playback */
	REGION_SOUND1,	/* memory region */
	{ 60 }
};


/***********************************************************
                     MACHINE DRIVERS
***********************************************************/

static MACHINE_DRIVER_START( topspeed )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)	/* 12 MHz ??? */
	MDRV_CPU_MEMORY(topspeed_readmem,topspeed_writemem)
	MDRV_CPU_VBLANK_INT(topspeed_interrupt,1)

	MDRV_CPU_ADD(Z80,16000000/4)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 4 MHz ??? */
	MDRV_CPU_MEMORY(z80_readmem,z80_writemem)

	MDRV_CPU_ADD(M68000, 12000000)	/* 12 MHz ??? */
	MDRV_CPU_MEMORY(topspeed_cpub_readmem,topspeed_cpub_writemem)
	MDRV_CPU_VBLANK_INT(topspeed_cpub_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 2*8, 32*8-1)
	MDRV_GFXDECODE(topspeed_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(8192)

	MDRV_VIDEO_START(topspeed)
	MDRV_VIDEO_UPDATE(topspeed)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(ADPCM, adpcm_interface)
MACHINE_DRIVER_END



/***************************************************************************
                                DRIVERS

Note: driver does NOT make use of the zoom sprite tables rom.
***************************************************************************/

ROM_START( topspeed )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )	/* 128K for 68000 code (CPU A) */
	ROM_LOAD16_BYTE( "b14-67-1.11", 0x00000, 0x10000, CRC(23f17616) SHA1(653ab6537f2e5898a77060c82b776852ab1f2b51) )
	ROM_LOAD16_BYTE( "b14-68-1.9",  0x00001, 0x10000, CRC(835659d9) SHA1(e99967f795c3c6e14bad7a66315640ca5db43c72) )
	ROM_LOAD16_BYTE( "b14-54.24",   0x80000, 0x20000, CRC(172924d5) SHA1(4a963f2e816f4b1c5acc6d38e99a68d3baeee8c6) )	/* 4 data roms */
	ROM_LOAD16_BYTE( "b14-52.26",   0x80001, 0x20000, CRC(e1b5b2a1) SHA1(8e2b992dcd5dc2317594c0187a22767aa626edee) )
	ROM_LOAD16_BYTE( "b14-55.23",   0xc0000, 0x20000, CRC(a1f15499) SHA1(72f99108713773782fc72aae5a3f6e9e2a1e347c) )
	ROM_LOAD16_BYTE( "b14-53.25",   0xc0001, 0x20000, CRC(04a04f5f) SHA1(09c15c33967bb141cc504b70d01c154bedb7fa33) )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* 128K for 68000 code (CPU B) */
	ROM_LOAD16_BYTE( "b14-69.80",   0x00000, 0x10000, CRC(d652e300) SHA1(b559bdb564d96da4c656dc7b2c88dae84c4861ae) )
	ROM_LOAD16_BYTE( "b14-70.81",   0x00001, 0x10000, CRC(b720592b) SHA1(13298b498a198dcc1a56e533d106545dd77e1bbc) )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )	/* Z80 sound cpu */
	ROM_LOAD( "b14-25.67", 0x00000, 0x04000, CRC(9eab28ef) SHA1(9a90f2c1881f4664d6d6241f3bc57faeaf150ffc) )
	ROM_CONTINUE(          0x10000, 0x0c000 )	/* banked stuff */

	ROM_REGION( 0x40000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "b14-07.54",   0x00000, 0x20000, CRC(c6025fff) SHA1(439ed85b0160bfd6c06fd42990124a292b2e3c14) )	/* SCR tiles */
	ROM_LOAD16_BYTE( "b14-06.52",   0x00001, 0x20000, CRC(b4e2536e) SHA1(c1960ee25b37b1444ec99082521c4858edcf3484) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROMX_LOAD( "b14-48.16", 0x000003, 0x20000, CRC(30c7f265) SHA1(3e52e2aabf2c456d0b57d9414f99bd942bafc887) , ROM_SKIP(7) )	/* OBJ, bitplane 3 */
	ROMX_LOAD( "b14-49.12", 0x100003, 0x20000, CRC(32ba4265) SHA1(f468243d923726b7eff78d9bc55a3a092f211a24) , ROM_SKIP(7) )
	ROMX_LOAD( "b14-50.8",  0x000007, 0x20000, CRC(ec1ef311) SHA1(4cfa06aec9535f2044b763b071f73d23ca8ba354) , ROM_SKIP(7) )
	ROMX_LOAD( "b14-51.4",  0x100007, 0x20000, CRC(35041c5f) SHA1(71602267736396516366a8abf535db82acaa1c23) , ROM_SKIP(7) )

	ROMX_LOAD( "b14-44.15", 0x000002, 0x20000, CRC(9f6c030e) SHA1(bb278fdcc29530685aa2e76da0712195f6ab0f5f) , ROM_SKIP(7) )	/* OBJ, bitplane 2 */
	ROMX_LOAD( "b14-45.11", 0x100002, 0x20000, CRC(63e4ce03) SHA1(92e3f45754676dd15691e48c0d37490c1a3ec328) , ROM_SKIP(7) )
	ROMX_LOAD( "b14-46.7",  0x000006, 0x20000, CRC(d489adf2) SHA1(9f77916594d5ed05b79d7e8d8f534eb39f65edae) , ROM_SKIP(7) )
	ROMX_LOAD( "b14-47.3",  0x100006, 0x20000, CRC(b3a1f75b) SHA1(050dd3313b5392d131c5a62c544260b83af0b8ab) , ROM_SKIP(7) )

	ROMX_LOAD( "b14-40.14", 0x000001, 0x20000, CRC(fa2a3cb3) SHA1(1e102ae6e916fda046a154b89056a18b724d51a3) , ROM_SKIP(7) )	/* OBJ, bitplane 1 */
	ROMX_LOAD( "b14-41.10", 0x100001, 0x20000, CRC(09455a14) SHA1(dc703e1f9c4f16e330796e9945799e1038ce503b) , ROM_SKIP(7) )
	ROMX_LOAD( "b14-42.6",  0x000005, 0x20000, CRC(ab51f53c) SHA1(0ed9a2e607b0bd2b43b47e3ed29b00a8d8a09f25) , ROM_SKIP(7) )
	ROMX_LOAD( "b14-43.2",  0x100005, 0x20000, CRC(1e6d2b38) SHA1(453cd818a6cd8b238c72cc880c811227609767b8) , ROM_SKIP(7) )

	ROMX_LOAD( "b14-36.13", 0x000000, 0x20000, CRC(20a7c1b8) SHA1(053c6b733a5c33b9259dfc754ce30a880905bb11) , ROM_SKIP(7) )	/* OBJ, bitplane 0 */
	ROMX_LOAD( "b14-37.9",  0x100000, 0x20000, CRC(801b703b) SHA1(dfbe276bd484815a7e69589eb56d54bc6e12e301) , ROM_SKIP(7) )
	ROMX_LOAD( "b14-38.5",  0x000004, 0x20000, CRC(de0c213e) SHA1(1313b2051e906d22edb55f4d45d3a424b31ca2a2) , ROM_SKIP(7) )
	ROMX_LOAD( "b14-39.1",  0x100004, 0x20000, CRC(798c28c5) SHA1(d2a8b9f84b3760f3800c5760ecee7ddcbafa6d6e) , ROM_SKIP(7) )

	ROM_REGION( 0x10000, REGION_USER1, 0 )
	ROM_LOAD( "b14-30.88", 0x00000, 0x10000, CRC(dccb0c7f) SHA1(42f0af72f559133b74912a4478e1323062be4b77) )	/* zoom tables for zoom sprite h/w */

/* One dump has this 0x10000 long, but just contains the same stuff repeated 8 times */ /**/
	ROM_REGION( 0x2000, REGION_USER2, 0 )
	ROM_LOAD( "b14-31.90",  0x0000,  0x2000,  CRC(5c6b013d) SHA1(6d02d4560076213b6fb6fe856143bb533090603e) )	/* microcontroller */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "b14-28.103",  0x00000, 0x10000, CRC(df11d0ae) SHA1(259e1e6cc7ab100bfdb60e3d7a6bb46acb6fe2ea) )
	ROM_LOAD( "b14-29.109",  0x10000, 0x10000, CRC(7ad983e7) SHA1(a3515caf93d6dab86de06ee52d6a13a456507dbe) )
ROM_END

ROM_START( topspedu )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )	/* 128K for 68000 code (CPU A) */
	ROM_LOAD16_BYTE     ( "b14-23", 0x00000, 0x10000, CRC(dd0307fd) SHA1(63218a707c78b3c785d1741dabdc511a76f12af1) )
	ROM_LOAD16_BYTE     ( "b14-24", 0x00001, 0x10000, CRC(acdf08d4) SHA1(506d48d27fc26684a3f884919665cf65a1b3062f) )
	ROM_LOAD16_WORD_SWAP( "b14-05", 0x80000, 0x80000, CRC(6557e9d8) SHA1(ff528b27fcaef5c181f5f3a56d6a41b935cf07e1) )	/* data rom */

	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* 128K for 68000 code (CPU B) */
	ROM_LOAD16_BYTE( "b14-26", 0x00000, 0x10000, CRC(659dc872) SHA1(0a168122fe6324510c830e21a56eace9c8a2c189) )
	ROM_LOAD16_BYTE( "b14-56", 0x00001, 0x10000, CRC(d165cf1b) SHA1(bfbb8699c5671d3841d4057678ef4085c1927684) )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )	/* Z80 sound cpu */
	ROM_LOAD( "b14-25.67", 0x00000, 0x04000, CRC(9eab28ef) SHA1(9a90f2c1881f4664d6d6241f3bc57faeaf150ffc) )
	ROM_CONTINUE(          0x10000, 0x0c000 )	/* banked stuff */

	ROM_REGION( 0x40000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "b14-07.54", 0x00000, 0x20000, CRC(c6025fff) SHA1(439ed85b0160bfd6c06fd42990124a292b2e3c14) )	/* SCR tiles */
	ROM_LOAD16_BYTE( "b14-06.52", 0x00001, 0x20000, CRC(b4e2536e) SHA1(c1960ee25b37b1444ec99082521c4858edcf3484) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "b14-01", 0x00000, 0x80000, CRC(84a56f37) SHA1(926bcae5bd75a4172de2a2078718b2940c5c1966) )	/* OBJ: each rom has 1 bitplane, forming 16x8 tiles */
	ROM_LOAD32_BYTE( "b14-02", 0x00001, 0x80000, CRC(6889186b) SHA1(3c38e281e8bf416a401c76ebb2d8ca95d09974b6) )
	ROM_LOAD32_BYTE( "b14-03", 0x00002, 0x80000, CRC(d1ed9e71) SHA1(26a6b2ca5bf6d70ad87f5c40c8e94ec542a2ec04) )
	ROM_LOAD32_BYTE( "b14-04", 0x00003, 0x80000, CRC(b63f0519) SHA1(e9a6b49effba0cae1ae3536a8584d3efa34ca8c3) )

	ROM_REGION( 0x10000, REGION_USER1, 0 )
	ROM_LOAD( "b14-30.88", 0x00000, 0x10000, CRC(dccb0c7f) SHA1(42f0af72f559133b74912a4478e1323062be4b77) )	/* zoom tables for zoom sprite h/w */

	ROM_REGION( 0x2000, REGION_USER2, 0 )
	ROM_LOAD( "b14-31.90", 0x0000,  0x2000,  CRC(5c6b013d) SHA1(6d02d4560076213b6fb6fe856143bb533090603e) )	/* microcontroller */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "b14-28.103", 0x00000, 0x10000, CRC(df11d0ae) SHA1(259e1e6cc7ab100bfdb60e3d7a6bb46acb6fe2ea) )
	ROM_LOAD( "b14-29.109", 0x10000, 0x10000, CRC(7ad983e7) SHA1(a3515caf93d6dab86de06ee52d6a13a456507dbe) )
ROM_END

ROM_START( fullthrl )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )	/* 128K for 68000 code (CPU A) */
	ROM_LOAD16_BYTE     ( "b14-67", 0x00000, 0x10000, CRC(284c943f) SHA1(e4720b138052d9cbf1290aeca8f9dd7fe2cffcc5) )	/* Later rev?*/
	ROM_LOAD16_BYTE     ( "b14-68", 0x00001, 0x10000, CRC(54cf6196) SHA1(0e86a7bf7d43526222160f4cd09f8d29fa9abdc4) )
	ROM_LOAD16_WORD_SWAP( "b14-05", 0x80000, 0x80000, CRC(6557e9d8) SHA1(ff528b27fcaef5c181f5f3a56d6a41b935cf07e1) )	/* data rom */

	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* 128K for 68000 code (CPU B) */
	ROM_LOAD16_BYTE( "b14-69.80", 0x00000, 0x10000, CRC(d652e300) SHA1(b559bdb564d96da4c656dc7b2c88dae84c4861ae) )
	ROM_LOAD16_BYTE( "b14-71",    0x00001, 0x10000, CRC(f7081727) SHA1(f0ab6ce9975dd7a1fadd439fd3dfd2f1bf88796c) )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )	/* Z80 sound cpu */
	ROM_LOAD( "b14-25.67", 0x00000, 0x04000, CRC(9eab28ef) SHA1(9a90f2c1881f4664d6d6241f3bc57faeaf150ffc) )
	ROM_CONTINUE(          0x10000, 0x0c000 )	/* banked stuff */

	ROM_REGION( 0x40000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "b14-07.54", 0x00000, 0x20000, CRC(c6025fff) SHA1(439ed85b0160bfd6c06fd42990124a292b2e3c14) )	/* SCR tiles */
	ROM_LOAD16_BYTE( "b14-06.52", 0x00001, 0x20000, CRC(b4e2536e) SHA1(c1960ee25b37b1444ec99082521c4858edcf3484) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "b14-01", 0x00000, 0x80000, CRC(84a56f37) SHA1(926bcae5bd75a4172de2a2078718b2940c5c1966) )	/* OBJ: each rom has 1 bitplane, forming 16x8 tiles */
	ROM_LOAD32_BYTE( "b14-02", 0x00001, 0x80000, CRC(6889186b) SHA1(3c38e281e8bf416a401c76ebb2d8ca95d09974b6) )
	ROM_LOAD32_BYTE( "b14-03", 0x00002, 0x80000, CRC(d1ed9e71) SHA1(26a6b2ca5bf6d70ad87f5c40c8e94ec542a2ec04) )
	ROM_LOAD32_BYTE( "b14-04", 0x00003, 0x80000, CRC(b63f0519) SHA1(e9a6b49effba0cae1ae3536a8584d3efa34ca8c3) )

	ROM_REGION( 0x10000, REGION_USER1, 0 )
	ROM_LOAD( "b14-30.88", 0x00000, 0x10000, CRC(dccb0c7f) SHA1(42f0af72f559133b74912a4478e1323062be4b77) )	/* zoom tables for zoom sprite h/w */

	ROM_REGION( 0x2000, REGION_USER2, 0 )
	ROM_LOAD( "b14-31.90", 0x0000,  0x2000,  CRC(5c6b013d) SHA1(6d02d4560076213b6fb6fe856143bb533090603e) )	/* microcontroller */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "b14-28.103", 0x00000, 0x10000, CRC(df11d0ae) SHA1(259e1e6cc7ab100bfdb60e3d7a6bb46acb6fe2ea) )
	ROM_LOAD( "b14-29.109", 0x10000, 0x10000, CRC(7ad983e7) SHA1(a3515caf93d6dab86de06ee52d6a13a456507dbe) )
ROM_END


DRIVER_INIT( topspeed )
{
/*	taitosnd_setz80_soundcpu( 2 );*/

	cpua_ctrl = 0xff;
	state_save_register_UINT16("main1", 0, "control", &cpua_ctrl, 1);
	state_save_register_func_postload(parse_control);

	state_save_register_int   ("main2", 0, "register", &ioc220_port);

	state_save_register_int   ("sound1", 0, "sound region", &banknum);
	state_save_register_func_postload(reset_sound_region);
}


GAME( 1987, topspeed, 0,        topspeed, topspeed, topspeed, ROT0, "Taito Corporation Japan", "Top Speed (World)" )
GAME( 1987, topspedu, topspeed, topspeed, topspedu, topspeed, ROT0, "Taito America Corporation (Romstar license)", "Top Speed (US)" )
GAME( 1987, fullthrl, topspeed, topspeed, fullthrl, topspeed, ROT0, "Taito Corporation", "Full Throttle (Japan)" )
