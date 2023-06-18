/***************************************************************************

GAME PLAN driver, used for games like MegaTack, Killer Comet, Kaos, Challenger

driver by Chris Moore

TO-DO: - Fix the input ports of Kaos
	   - Graphics are still somewhat scrambled sometimes (just look at
         the tests with f2/f1)

****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

VIDEO_START( gameplan );
READ_HANDLER( gameplan_video_r );
WRITE_HANDLER( gameplan_video_w );
READ_HANDLER( gameplan_sound_r );
WRITE_HANDLER( gameplan_sound_w );
READ_HANDLER( gameplan_via5_r );
WRITE_HANDLER( gameplan_via5_w );

static int gameplan_current_port;

static WRITE_HANDLER( gameplan_port_select_w )
{
#ifdef VERY_VERBOSE
	log_cb(RETRO_LOG_DEBUG, LOGPRE "VIA 2: PC %04x: %x -> reg%X\n",activecpu_get_pc(), data, offset);
#endif /* VERY_VERBOSE */

	switch (offset)
	{
		case 0x00:
			switch(data)
			{
				case 0x01: gameplan_current_port = 0; break;
				case 0x02: gameplan_current_port = 1; break;
				case 0x04: gameplan_current_port = 2; break;
				case 0x08: gameplan_current_port = 3; break;
				case 0x80: gameplan_current_port = 4; break;
				case 0x40: gameplan_current_port = 5; break;

				default:
#ifdef VERBOSE
					log_cb(RETRO_LOG_DEBUG, LOGPRE "  VIA 2: strange port request byte: %02x\n", data);
#endif
					return;
			}

#ifdef VERBOSE
			log_cb(RETRO_LOG_DEBUG, LOGPRE "  VIA 2: selected port %d\n", gameplan_current_port);
#endif
			break;

		case 0x02:
#ifdef VERBOSE
			log_cb(RETRO_LOG_DEBUG, LOGPRE "  VIA 2: wrote %02x to Data Direction Register B\n", data);
#endif
			break;

		case 0x03:
#ifdef VERBOSE
			log_cb(RETRO_LOG_DEBUG, LOGPRE "  VIA 2: wrote %02x to Data Direction Register A\n", data);
#endif
			break;

		case 0x0c:
			if (data == 0xec || data == 0xcc)
			{
#ifdef VERBOSE
				log_cb(RETRO_LOG_DEBUG, LOGPRE "  VIA 2: initialised Peripheral Control Register to 0x%02x for VIA 2\n",data);
#endif
			}
			else
				log_cb(RETRO_LOG_DEBUG, LOGPRE "  VIA 2: unusual Peripheral Control Register value 0x%02x for VIA 2\n",data);
			break;

		default:
			logerror("  VIA 2: unexpected register written to in VIA 2: %02x -> %02x\n",
						data, offset);
			break;
	}
}

static READ_HANDLER( gameplan_port_r )
{
	return readinputport(gameplan_current_port);
}

static MEMORY_READ_START( readmem )
    { 0x0000, 0x03ff, MRA_RAM },
    { 0x032d, 0x03d8, MRA_RAM }, /* note: 300-32c and 3d9-3ff is
								  * written but never read?
								  * (write by code at e1df and e1e9,
								  * 32d is read by e258)*/
    { 0x2000, 0x200f, gameplan_video_r },
    { 0x2801, 0x2801, gameplan_port_r },
	{ 0x3000, 0x300f, gameplan_sound_r },
    { 0x9000, 0xffff, MRA_ROM },

MEMORY_END

static MEMORY_WRITE_START( writemem )
    { 0x0000, 0x03ff, MWA_RAM },
    { 0x2000, 0x200f, gameplan_video_w },		/* VIA 1 */
    { 0x2800, 0x280f, gameplan_port_select_w },	/* VIA 2 */
    { 0x3000, 0x300f, gameplan_sound_w },       /* VIA 3 */
    { 0x9000, 0xffff, MWA_ROM },

MEMORY_END

static MEMORY_READ_START( readmem_snd )
	{ 0x0000, 0x0026, MRA_RAM },
	{ 0x01f6, 0x01ff, MRA_RAM },
	{ 0x0800, 0x080f, gameplan_via5_r },

#if 0
    { 0xa001, 0xa001, gameplan_ay_3_8910_1_r }, /* AY-3-8910 */
#else
    { 0xa001, 0xa001, soundlatch_r }, /* AY-3-8910 */
#endif

    { 0xf800, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem_snd )
	{ 0x0000, 0x0026, MWA_RAM },
	{ 0x01f6, 0x01ff, MWA_RAM },
	{ 0x0800, 0x080f, gameplan_via5_w },

#if 0
    { 0xa000, 0xa000, gameplan_ay_3_8910_0_w }, /* AY-3-8910 */
    { 0xa002, 0xa002, gameplan_ay_3_8910_2_w }, /* AY-3-8910 */
#else
    { 0xa000, 0xa000, AY8910_control_port_0_w }, /* AY-3-8910 */
    { 0xa002, 0xa002, AY8910_write_port_0_w }, /* AY-3-8910 */
#endif

	{ 0xf800, 0xffff, MWA_ROM },

MEMORY_END

INPUT_PORTS_START( kaos )
	PORT_START      /* IN0 - from "TEST NO.7 - status locator - coin-door" */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* unused */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "Do Tests", KEYCODE_F1, IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_SERVICE, "Select Test", KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START      /* IN1 - from "TEST NO.7 - status locator - start sws." */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* IN2 - from "TEST NO.8 - status locator - player no.1" */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1, "P1 Jump", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1, "P1 Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1, "P1 Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1, "P1 Fire Up", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1, "P1 Fire Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */

	PORT_START      /* IN3 - from "TEST NO.8 - status locator - player no.2" */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2, "P2 Fire Up", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2, "P2 Fire Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2, "P2 Fire Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2, "P2 Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2, "P2 Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */

	PORT_START

	PORT_DIPNAME(0x0f, 0x0e, DEF_STR( Coinage ) )
	PORT_DIPSETTING(   0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(   0x0e, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(   0x0d, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(   0x0c, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(   0x0b, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(   0x0a, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(   0x09, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(   0x08, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(   0x07, DEF_STR( 1C_8C ) )
	PORT_DIPSETTING(   0x06, DEF_STR( 1C_9C ) )
	PORT_DIPSETTING(   0x05, "1 Coin/10 Credits" )
	PORT_DIPSETTING(   0x04, "1 Coin/11 Credits" )
	PORT_DIPSETTING(   0x03, "1 Coin/12 Credits" )
	PORT_DIPSETTING(   0x02, "1 Coin/13 Credits" )
	PORT_DIPSETTING(   0x01, "1 Coin/14 Credits" )
	PORT_DIPSETTING(   0x0f, DEF_STR( 2C_3C ) )
	PORT_DIPNAME(0x10, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(   0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x00, DEF_STR( On ) )
	PORT_DIPNAME(0x60, 0x60, "Max Credits" )
	PORT_DIPSETTING(   0x60, "10" )
	PORT_DIPSETTING(   0x40, "20" )
	PORT_DIPSETTING(   0x20, "30" )
	PORT_DIPSETTING(   0x00, "40" )
	PORT_DIPNAME(0x80, 0x80, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(   0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x00, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME(0x01, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(   0x01, "3" )
	PORT_DIPSETTING(   0x00, "4" )
	PORT_DIPNAME(0x02, 0x00, "Speed" )
	PORT_DIPSETTING(   0x00, "Slow" )
	PORT_DIPSETTING(   0x02, "Fast" )
	PORT_DIPNAME(0x0c, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(   0x0c, "No Bonus" )
	PORT_DIPSETTING(   0x08, "10k" )
	PORT_DIPSETTING(   0x04, "10k 30k" )
	PORT_DIPSETTING(   0x00, "10k 30k 60k" )
	PORT_DIPNAME(0x10, 0x10, "Number of $" )
	PORT_DIPSETTING(   0x10, "8" )
	PORT_DIPSETTING(   0x00, "12" )
	PORT_DIPNAME(0x20, 0x00, "Bonus erg" )
	PORT_DIPSETTING(   0x20, "Every other screen" )
	PORT_DIPSETTING(   0x00, "Every screen" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME(0x80, 0x80, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(   0x80, DEF_STR( Upright ) )
	PORT_DIPSETTING(   0x00, DEF_STR( Cocktail ) )
INPUT_PORTS_END


INPUT_PORTS_START( killcom )
	PORT_START      /* IN0 - from "TEST NO.7 - status locator - coin-door" */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* unused */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "Do Tests", KEYCODE_F1, IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_SERVICE, "Select Test", KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START      /* IN1 - from "TEST NO.7 - status locator - start sws." */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* IN2 - from "TEST NO.8 - status locator - player no.1" */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1, "P1 Hyperspace", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1, "P1 Fire Up", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1, "P1 Fire Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1, "P1 Fire Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1, "P1 Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1, "P1 Down", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1, "P1 Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1, "P1 Up", IP_KEY_DEFAULT, IP_JOY_DEFAULT )

	PORT_START      /* IN3 - from "TEST NO.8 - status locator - player no.2" */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2, "P2 Hyperspace", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2, "P2 Fire Up", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2, "P2 Fire Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2, "P2 Fire Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2, "P2 Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2, "P2 Down", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2, "P2 Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2, "P2 Up", IP_KEY_DEFAULT, IP_JOY_DEFAULT )

	PORT_START      /* IN4 - from "TEST NO.6 - dip switch A" */

	PORT_DIPNAME(0x03, 0x03, "Coinage P1/P2" )
	PORT_DIPSETTING(   0x03, "1 Credit/2 Credits" )
	PORT_DIPSETTING(   0x02, "2 Credits/3 Credits" )
	PORT_DIPSETTING(   0x01, "2 Credits/4 Credits" )
	PORT_DIPSETTING(   0x00, DEF_STR( Free_Play ) )

	PORT_DIPNAME(0x08, 0x08, DEF_STR( Lives ) )
	PORT_DIPSETTING(   0x00, "4" )
	PORT_DIPSETTING(   0x08, "5" )

	PORT_DIPNAME(0xc0, 0xc0, "Reaction" )
	PORT_DIPSETTING(   0xc0, "Slowest" )
	PORT_DIPSETTING(   0x80, "Slow" )
	PORT_DIPSETTING(   0x40, "Fast" )
	PORT_DIPSETTING(   0x00, "Fastest" )

	PORT_START      /* IN5 - from "TEST NO.6 - dip switch B" */

	PORT_DIPNAME(0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(   0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x00, DEF_STR( On ) )

	PORT_DIPNAME(0x80, 0x80, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(   0x80, DEF_STR( Upright ) )
	PORT_DIPSETTING(   0x00, DEF_STR( Cocktail ) )
INPUT_PORTS_END


INPUT_PORTS_START( megatack )
	PORT_START      /* IN0 - from "TEST NO.7 - status locator - coin-door" */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* unused */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "Do Tests", KEYCODE_F1, IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_SERVICE, "Select Test", KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START      /* IN1 - from "TEST NO.7 - status locator - start sws." */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* IN2 - from "TEST NO.8 - status locator - player no.1" */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1, "P1 Fire", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
/* PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1, "P1 Fire Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
   PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1, "P1 Fire Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT )*/
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1, "P1 Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1, "P1 Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */

	PORT_START      /* IN3 - from "TEST NO.8 - status locator - player no.2" */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2, "P2 Fire", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
/* PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2, "P2 Fire Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
   PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2, "P2 Fire Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT )*/
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2, "P2 Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2, "P2 Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */

	PORT_START      /* IN4 - from "TEST NO.6 - dip switch A" */
	PORT_DIPNAME(0x03, 0x03, "Coinage P1/P2" )
	PORT_DIPSETTING(   0x03, "1 Credit/2 Credits" )
	PORT_DIPSETTING(   0x02, "2 Credits/3 Credits" )
	PORT_DIPSETTING(   0x01, "2 Credits/4 Credits" )
	PORT_DIPSETTING(   0x00, DEF_STR( Free_Play ) )

	PORT_DIPNAME(0x08, 0x08, DEF_STR( Lives ) )
	PORT_DIPSETTING(   0x08, "3" )
	PORT_DIPSETTING(   0x00, "4" )

	PORT_START      /* IN5 - from "TEST NO.6 - dip switch B" */

	PORT_DIPNAME(0x07, 0x07, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(   0x07, "20000" )
	PORT_DIPSETTING(   0x06, "30000" )
	PORT_DIPSETTING(   0x05, "40000" )
	PORT_DIPSETTING(   0x04, "50000" )
	PORT_DIPSETTING(   0x03, "60000" )
	PORT_DIPSETTING(   0x02, "70000" )
	PORT_DIPSETTING(   0x01, "80000" )
	PORT_DIPSETTING(   0x00, "90000" )

	PORT_DIPNAME(0x10, 0x10, "Monitor View" )
	PORT_DIPSETTING(   0x10, "Direct" )
	PORT_DIPSETTING(   0x00, "Mirror" )

	PORT_DIPNAME(0x20, 0x20, "Monitor Orientation" )
	PORT_DIPSETTING(   0x20, "Horizontal" )
	PORT_DIPSETTING(   0x00, "Vertical" )

	PORT_DIPNAME(0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(   0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x00, DEF_STR( On ) )

	PORT_DIPNAME(0x80, 0x80, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(   0x80, DEF_STR( Upright ) )
	PORT_DIPSETTING(   0x00, DEF_STR( Cocktail ) )
INPUT_PORTS_END


INPUT_PORTS_START( challeng )
	PORT_START      /* IN0 - from "TEST NO.7 - status locator - coin-door" */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* unused */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "Do Tests", KEYCODE_F1, IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_SERVICE, "Select Test", KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START      /* IN1 - from "TEST NO.7 - status locator - start sws." */

	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

PORT_START      /* IN2 - from "TEST NO.8 - status locator - player no.1" */

	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1, "P1 Warp", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1, "P1 Fire", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1, "P1 Bomb", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1, "P1 Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1, "P1 Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */

	PORT_START      /* IN3 - from "TEST NO.8 - status locator - player no.2" */

	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2, "P2 Warp", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2, "P2 Fire", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2, "P2 Bomb", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2, "P2 Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2, "P2 Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* unused */

	PORT_START      /* IN4 - from "TEST NO.6 - dip switch A" */

	PORT_DIPNAME(0x03, 0x03, "Coinage P1/P2" )
	PORT_DIPSETTING(   0x03, "1 Credit/2 Credits" )
	PORT_DIPSETTING(   0x02, "2 Credits/3 Credits" )
	PORT_DIPSETTING(   0x01, "2 Credits/4 Credits" )
	PORT_DIPSETTING(   0x00, DEF_STR( Free_Play ) )

	PORT_DIPNAME(0xc0, 0xc0, DEF_STR( Lives ) )
	PORT_DIPSETTING(   0xc0, "3" )
	PORT_DIPSETTING(   0x80, "4" )
	PORT_DIPSETTING(   0x40, "5" )
	PORT_DIPSETTING(   0x00, "6" )

	PORT_START      /* IN5 - from "TEST NO.6 - dip switch B" */
	PORT_DIPNAME(0x07, 0x07, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(   0x01, "20000" )
	PORT_DIPSETTING(   0x00, "30000" )
	PORT_DIPSETTING(   0x07, "40000" )
	PORT_DIPSETTING(   0x06, "50000" )
	PORT_DIPSETTING(   0x05, "60000" )
	PORT_DIPSETTING(   0x04, "70000" )
	PORT_DIPSETTING(   0x03, "80000" )
	PORT_DIPSETTING(   0x02, "90000" )
	PORT_DIPNAME(0x10, 0x10, "Monitor View" )
	PORT_DIPSETTING(   0x10, "Direct" )
	PORT_DIPSETTING(   0x00, "Mirror" )
	PORT_DIPNAME(0x20, 0x20, "Monitor Orientation" )
	PORT_DIPSETTING(   0x20, "Horizontal" )
	PORT_DIPSETTING(   0x00, "Vertical" )
	PORT_DIPNAME(0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(   0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x00, DEF_STR( On ) )
	PORT_DIPNAME(0x80, 0x80, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(   0x80, DEF_STR( Upright ) )
	PORT_DIPSETTING(   0x00, DEF_STR( Cocktail ) )
INPUT_PORTS_END



static PALETTE_INIT( gameplan )
{
	palette_set_color(0,0xff,0xff,0xff); /* 0 WHITE   */
	palette_set_color(1,0x20,0xff,0xff); /* 1 CYAN    */
	palette_set_color(2,0xff,0x20,0xff); /* 2 MAGENTA */
	palette_set_color(3,0x20,0x20,0xFF); /* 3 BLUE    */
	palette_set_color(4,0xff,0xff,0x20); /* 4 YELLOW  */
	palette_set_color(5,0x20,0xff,0x20); /* 5 GREEN   */
	palette_set_color(6,0xff,0x20,0x20); /* 6 RED     */
	palette_set_color(7,0x00,0x00,0x00); /* 7 BLACK   */
}


static struct AY8910interface ay8910_interface =
{
	1,	/* 1 chips */
	1500000,	/* 1.5 MHz ? */
	{ 50 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};


static MACHINE_DRIVER_START( gameplan )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6502,3579000 / 4)		/* 3.579 / 4 MHz */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1) /* 1 interrupt per frame */

	MDRV_CPU_ADD(M6502,3579000 / 4)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)		/* 3.579 / 4 MHz */
	MDRV_CPU_MEMORY(readmem_snd,writemem_snd)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(57)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0, 32*8-1, 0, 32*8-1)
	MDRV_PALETTE_LENGTH(8)

	MDRV_PALETTE_INIT(gameplan)
	MDRV_VIDEO_START(gameplan)
	MDRV_VIDEO_UPDATE(generic_bitmapped)

	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*
the manuals for Megattack and the schematics for Kaos are up
on spies now. I took a quick look at the rom mapping for kaos
and it looks like the roms are split this way:

9000 G2 bot 2k
9800 J2 bot 2k
A000 J1 bot 2k
A800 G1 bot 2k
B000 F1 bot 2k
B800 E1 bot 2k

D000 G2 top 2k
D800 J2 top 2k
E000 J1 top 2k
E800 G1 top 2k
F000 F1 top 2k
F800 E1 top 2k

there are three 6522 VIAs, at 2000, 2800, and 3000
*/

ROM_START( kaos )
    ROM_REGION( 0x10000, REGION_CPU1, 0 )
    ROM_LOAD( "kaosab.g2",    0x9000, 0x0800, CRC(b23d858f) SHA1(e31fa657ace34130211a0b9fc0d115fd89bb20dd) )
    ROM_CONTINUE(		   	  0xd000, 0x0800			 )
    ROM_LOAD( "kaosab.j2",    0x9800, 0x0800, CRC(4861e5dc) SHA1(96ca0b8625af3897bd4a50a45ea964715f9e4973) )
    ROM_CONTINUE(		   	  0xd800, 0x0800			 )
    ROM_LOAD( "kaosab.j1",    0xa000, 0x0800, CRC(e055db3f) SHA1(099176629723c1a9bdc59f440339b2e8c38c3261) )
    ROM_CONTINUE(		   	  0xe000, 0x0800			 )
    ROM_LOAD( "kaosab.g1",    0xa800, 0x0800, CRC(35d7c467) SHA1(6d5bfd29ff7b96fed4b24c899ddd380e47e52bc5) )
    ROM_CONTINUE(		   	  0xe800, 0x0800			 )
    ROM_LOAD( "kaosab.f1",    0xb000, 0x0800, CRC(995b9260) SHA1(580896aa8b6f0618dc532a12d0795b0d03f7cadd) )
    ROM_CONTINUE(		   	  0xf000, 0x0800			 )
    ROM_LOAD( "kaosab.e1",    0xb800, 0x0800, CRC(3da5202a) SHA1(6b5aaf44377415763aa0895c64765a4b82086f25) )
    ROM_CONTINUE(		   	  0xf800, 0x0800			 )

    ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "kaossnd.e1",   0xf800, 0x800, CRC(ab23d52a) SHA1(505f3e4a56e78a3913010f5484891f01c9831480) )
ROM_END


ROM_START( killcom )
    ROM_REGION( 0x10000, REGION_CPU1, 0 )
    ROM_LOAD( "killcom.e2",   0xc000, 0x800, CRC(a01cbb9a) SHA1(a8769243adbdddedfda5f3c8f054e9281a0eca46) )
    ROM_LOAD( "killcom.f2",   0xc800, 0x800, CRC(bb3b4a93) SHA1(a0ea61ac30a4d191db619b7bfb697914e1500036) )
    ROM_LOAD( "killcom.g2",   0xd000, 0x800, CRC(86ec68b2) SHA1(a09238190d61684d943ce0acda25eb901d2580ac) )
    ROM_LOAD( "killcom.j2",   0xd800, 0x800, CRC(28d8c6a1) SHA1(d9003410a651221e608c0dd20d4c9c60c3b0febc) )
    ROM_LOAD( "killcom.j1",   0xe000, 0x800, CRC(33ef5ac5) SHA1(42f839ad295d3df457ced7886a0003eff7e6c4ae) )
    ROM_LOAD( "killcom.g1",   0xe800, 0x800, CRC(49cb13e2) SHA1(635e4665042ddd9b8c0b9f275d4bcc6830dc6a98) )
    ROM_LOAD( "killcom.f1",   0xf000, 0x800, CRC(ef652762) SHA1(414714e5a3f83916bd3ae54afe2cb2271ee9008b) )
    ROM_LOAD( "killcom.e1",   0xf800, 0x800, CRC(bc19dcb7) SHA1(eb983d2df010c12cb3ffb584fceafa54a4e956b3) )

    ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "killsnd.e1",   0xf800, 0x800, CRC(77d4890d) SHA1(a3ed7e11dec5d404f022c521256ff50aa6940d3c) )
ROM_END

ROM_START( megatack )
    ROM_REGION( 0x10000, REGION_CPU1, 0 )
    ROM_LOAD( "megattac.e2",  0xc000, 0x800, CRC(33fa5104) SHA1(15693eb540563e03502b53ed8a83366e395ca529) )
    ROM_LOAD( "megattac.f2",  0xc800, 0x800, CRC(af5e96b1) SHA1(5f6ab47c12d051f6af446b08f3cd459fbd2c13bf) )
    ROM_LOAD( "megattac.g2",  0xd000, 0x800, CRC(670103ea) SHA1(e11f01e8843ed918c6ea5dda75319dc95105d345) )
    ROM_LOAD( "megattac.j2",  0xd800, 0x800, CRC(4573b798) SHA1(388db11ab114b3575fe26ed65bbf49174021939a) )
    ROM_LOAD( "megattac.j1",  0xe000, 0x800, CRC(3b1d01a1) SHA1(30bbf51885b1e510b8d21cdd82244a455c5dada0) )
    ROM_LOAD( "megattac.g1",  0xe800, 0x800, CRC(eed75ef4) SHA1(7c02337344f2716d2f2771229f7dee7b651eeb95) )
    ROM_LOAD( "megattac.f1",  0xf000, 0x800, CRC(c93a8ed4) SHA1(c87e2f13f2cc00055f4941c272a3126b165a6252) )
    ROM_LOAD( "megattac.e1",  0xf800, 0x800, CRC(d9996b9f) SHA1(e71d65b695000fdfd5fd1ce9ae39c0cb0b61669e) )

    ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "megatsnd.e1",  0xf800, 0x800, CRC(0c186bdb) SHA1(233af9481a3979971f2d5aa75ec8df4333aa5e0d) )
ROM_END

ROM_START( challeng )
    ROM_REGION( 0x10000, REGION_CPU1, 0 )
    ROM_LOAD( "chall.6",      0xa000, 0x1000, CRC(b30fe7f5) SHA1(ce93a57d626f90d31ddedbc35135f70758949dfa) )
    ROM_LOAD( "chall.5",      0xb000, 0x1000, CRC(34c6a88e) SHA1(250577e2c8eb1d3a78cac679310ec38924ac1fe0) )
    ROM_LOAD( "chall.4",      0xc000, 0x1000, CRC(0ddc18ef) SHA1(9f1aa27c71d7e7533bddf7de420c06fb0058cf60) )
    ROM_LOAD( "chall.3",      0xd000, 0x1000, CRC(6ce03312) SHA1(69c047f501f327f568fe4ad1274168f9dda1ca70) )
    ROM_LOAD( "chall.2",      0xe000, 0x1000, CRC(948912ad) SHA1(e79738ab94501f858f4d5f218787267523611e92) )
    ROM_LOAD( "chall.1",      0xf000, 0x1000, CRC(7c71a9dc) SHA1(2530ada6390fb46c44bf7bf2636910ee54786493) )

    ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "chall.snd",    0xf800, 0x800, CRC(1b2bffd2) SHA1(36ceb5abbc92a17576c375019f1c5900320398f9) )
ROM_END



GAME( 1981, kaos,     0, gameplan, kaos,     0, ROT270, "GamePlan", "Kaos" )
GAME( 1980, killcom,  0, gameplan, killcom,  0, ROT0,   "GamePlan (Centuri license)", "Killer Comet" )
GAME( 1980, megatack, 0, gameplan, megatack, 0, ROT0,   "GamePlan (Centuri license)", "MegaTack" )
GAME( 1981, challeng, 0, gameplan, challeng, 0, ROT0,   "GamePlan (Centuri license)", "Challenger" )
