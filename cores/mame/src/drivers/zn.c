/***************************************************************************

  Sony ZN1/ZN2 - Arcade PSX Hardware
  ==================================
  Driver by smf

  QSound emulation based on information from the cps1/cps2 driver, Amuse & Miguel Angel Horna
  Taito FX1a sound emulation based on information from the Taito F2 driver

  The ZN1/ZN2 boards are standard boards from Sony. Extra sound h/w is
  included on the rom board,

  The BIOS is protected in all the games that run on this hardware.
  During boot up the BIOS decrypts parts of itself into ram using
  a device connected to the PSX controller port. As this is not
  emulated yet you only get the boot up colours.


  Capcom ZN1 ( coh-1000 ) / ZN2 ( coh-3002c )
  -------------------------------------------

  The QSound hardware is different to cps2 as it uses an i/o port and
  nmi's instead of shared memory. The driver uses 8bit i/o addresses
  but the real ZN1 hardware may use 16bit i/o addresses as the code
  always accesses port 0xf100. The ZN2 code however seems to vary the
  top eight bits of the address, this may or may not be important but
  it is currently ignored.

  The Gallop Racer dump came from a Capcom ZN1 QSound board but there
  were no QSound program/sample roms & the game doesn't appear to use
  it. Tecmo went on to produce games on their own boards.

    Key
    ---
    CP09 - kikaioh rom board
    CP10 - kikaioh/shiryu2 motherboard
    CP13 - shiryu2 rom board


  Taito FX1a
  ----------

  The extra sound h/w is the same as taito f2 ( z80-a + ym2610b ).

    Key
    ---
    TT02 - Super Football Champ
    TT06 - Magical Date


  Taito FX1b
  ----------

  The extra sound h/w isn't emulated and consists of an mn1020012a +
  zoom zsg-2 + tms57002dpha.

  mn1020012a is a 16bit panasonic mpu & the zoom zsg-2 is a sound generator
  lsi produced by zoom corp in 1995.

    Key
    ---
    TT04 - Ray Storm
    TT05 - Fighter's Impact Ace
    TT07 - G-Darius

  Beastorizer bootleg runs on a ray storm motherboard, but doesn't have fx1b sound h/w.

  PS Arcade 95 ( coh-1002e )
  --------------------------
Main Board: Sony ZN-1  (1-659-709-12  COH-1002E)
 Sub Board: Raizing RA9701 SUB

    Key
    ---
    ET01 - Main board
    ET02 - Beastorizer

TMP68HC00N-16 (Toshiba 68K 12MHz)
OSC: 12.0000MHz (Kyocera KX-01-1)
Mach211 (x2) Labled "Main_IF2" & "SUB_IF2"
ST M628032-15E1 (x2) Ram??
Yamaha YMF271-F
GAL16V8B
CAT702 103090-ET02 (the "ET02" is a printed sticker/label) ??


  Acclaim PSX ( coh-1000 )
  ------------------------

    Key
    ---
    AC01 - Motherboard
    AC02 - NBA Jam Extreme

Sound: Analog Devices ADSP-2181

  Video System
  ------------

    Key
    ---
    KN01 - Motherboard
    KN02 - Sonic Wings Limited

  Tecmo ( coh-1002m )
  -------------------

    Key
    ---
    MG01 - Motherboard
    MG02 - Gallop Racer 2
    MG05 - Dead or Alive ++
    MG09 - Tondemo Crisis

  ***************************************************************************/

#define PROTECTION_HACK ( 1 )

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/mips/psx.h"
#include "cpu/z80/z80.h"
#include "sndhrdw/taitosnd.h"
#include "includes/psx.h"

#if PROTECTION_HACK

/* decodebp.c */
static unsigned char kn_protection_hack[] =
{
	/* unknown */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00,

	/* first test, this byte is ignored */
	0xef,

	/* checked for explicitly ( skips red screen ) */
	0x76, 0x00, 0xe3, 0xeb, 0x5b, 0x2f, 0x73, 0x56,
	0xc5, 0xc9, 0x74, 0x66, 0x79, 0xce, 0xa7, 0x02,
	0x77, 0x79, 0x2e, 0xbb, 0xda, 0x2c, 0xb3, 0x5b,
	0x65, 0xdc, 0xb7, 0x5b, 0x08, 0x14, 0xf1, 0xe4,
	0x73, 0x0e, 0x57, 0xc1, 0x29, 0x62, 0x76, 0x95,
	0xb9, 0x22, 0x3b, 0x7f, 0x28, 0x66, 0x32, 0xdd,
	0xcd,

	/* unknown */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	
	/* second test, this byte is ignored */
	0x00,

	/* checks the result of a calculation ( skips blue screen ) */
	0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00,

	/* default value */
	0x00,
};

/* decodesce.c */
static unsigned char mg_protection_hack[] =
{
	/* unknown */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00,

	/* first test, this byte is ignored */
	0x6f,

	/* checked for explicitly ( skips red screen ) */
	0x5b, 0x35, 0xda, 0xf5, 0x42, 0x84, 0x84, 0xed,
	0x6f, 0x1f, 0x80, 0x99, 0x67, 0x1b, 0xcc, 0xa4,
	0x6c, 0x77, 0x8a, 0xad, 0xf7, 0xb6, 0xfa, 0xa7,
	0x36, 0x94, 0xcd, 0x64, 0x87, 0x7c, 0x52, 0x6c,
	0x76, 0x2a, 0x5e, 0x84, 0x7c, 0xa2, 0x7f, 0xf2,
	0xe5, 0xff, 0x56, 0x9b, 0x00, 0xb5, 0xf9, 0x69,
	0xdb,

	/* unknown */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

	/* second test, this byte is ignored */
	0x00,

	/* checks the result of a calculation ( skips blue screen ) */
	0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00,

	/* default value */
	0x00,
};

/* decodetaito.c */
static unsigned char tt_protection_hack[] =
{
	/* unknown */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00,

	/* first test, this byte is ignored */
	0xef,

	/* checked for explicitly ( skips red screen ) */
	0xd7, 0x5f, 0xb7, 0xea, 0x51, 0x15, 0x96, 0x02,
	0x0b, 0xf8, 0xf5, 0x54, 0xc5, 0xf9, 0x5d, 0x5d,
	0xea, 0x13, 0xd0, 0x63, 0x95, 0xd4, 0x9f, 0xee,
	0xeb, 0x3f, 0x94, 0x36, 0x42, 0x45, 0x81, 0xb0,
	0x80, 0xc4, 0x3f, 0x42, 0x39, 0x2a, 0xa4, 0x28,
	0x28, 0x20, 0xac, 0x00, 0x20, 0x8a, 0x28, 0x92,
	0xa8,

	/* unknown */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

	/* second test, this byte is ignored */
	0x00,

	/* checks the result of a calculation ( skips blue screen ) */
	0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00,

	/* default value */
	0x00,
};

/* decodecapcon.c */
static unsigned char cpzn1_protection_hack [] =
{
	
	/* unknown */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00,

	/* first test, this byte is ignored */
	0xef,

	/* checked for explicitly ( skips red screen ) */
	0xe3, 0x9a, 0x29, 0xba, 0x16, 0x09, 0x82, 0x64,
	0x8b, 0xad, 0xf6, 0xfb, 0xbb, 0xba, 0x93, 0x3f,
	0xec, 0xc4, 0xe4, 0xad, 0xbe, 0xaf, 0x7a, 0x9e,
	0x7f, 0x79, 0xba, 0xac, 0xb6, 0x59, 0x11, 0x25,
	0xf3, 0x98, 0xa9, 0x57, 0x4d, 0xe0, 0xf6, 0x74,
	0xc4, 0x2c, 0x2b, 0xa0, 0x28, 0xee, 0xce, 0x83,
	0xc5,

	/* unknown */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

	/* second test, this byte is ignored */
	0x00,

	/* checks the result of a calculation ( skips blue screen ) */
	0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00,

	/* default value */
	0x00,
};

/* decodecapcon.c */
static unsigned char cpzn2_protection_hack [] =
{
	
	/* unknown */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00,

	/* first test, this byte is ignored */
	0xef,

	/* checked for explicitly ( skips red screen ) */
	0xa7, 0x5d, 0x52, 0x7c, 0xdb, 0x26, 0x51, 0x1c,
	0x9d, 0x64, 0x87, 0xdd, 0x36, 0x25, 0xf1, 0xff,
	0xc4, 0xb7, 0x6d, 0xa7, 0x19, 0xca, 0xa2, 0xc3,
	0x76, 0x92, 0xcb, 0x64, 0xab, 0xc1, 0xae, 0xd1,
	0xda, 0x90, 0x90, 0x20, 0xfd, 0x2f, 0x93, 0x48,
	0x7e, 0xaf, 0x35, 0x82, 0xd4, 0x26, 0x6d, 0x76,
	0xa3,

	/* unknown */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

	/* second test, this byte is ignored */
	0x00,

	/* checks the result of a calculation ( skips blue screen ) */
	0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00,

	/* default value */
	0x00,
};

/* ?.c */
static unsigned char psarc95_protection_hack [] =
{
	/* not dumped yet */0x00,
};

static unsigned short n_sio0_ptr;
static unsigned char *p_sio0_ret;
static unsigned short n_sio0_ret_size;

#endif

static READ32_HANDLER( sio0_r )
{
	unsigned short n_return;

	n_return = 0;
	switch( offset )
	{
	case 0x00:
		/* data */
#if PROTECTION_HACK
		if( p_sio0_ret != NULL )
		{
			n_return = p_sio0_ret[ n_sio0_ptr ];
			if( n_sio0_ptr < n_sio0_ret_size - 1 )
			{
				n_sio0_ptr++;
			}
		}
#endif
		break;
	case 0x01:
		/* status */
#if PROTECTION_HACK
		if( p_sio0_ret != NULL )
		{
			n_return = 2;
		}
#endif
		break;
	case 0x02:
		/* mode */
		break;
	case 0x03:
		/* control */
		break;
	case 0x04:
		/* baud */
		break;
	}
	log_cb(RETRO_LOG_DEBUG, LOGPRE  "%08x: sio0_r( %04x ) %04x\n", activecpu_get_pc(), offset, n_return );
	return n_return;
}

static WRITE32_HANDLER( sio0_w )
{
	int n_char;
	char s_char[ 5 ];

	s_char[ 0 ] = 0;
	switch( offset )
	{
	case 0x00:
		/* data */
		n_char = ( data & 0xff );
		if( n_char >= 0x20 && n_char <= 0x7f )
		{
			sprintf( s_char, " '%c'", n_char );
		}
		else
		{
			sprintf( s_char, " $%02x", n_char );
		}
		break;
	case 0x01:
		/* status */
		break;
	case 0x02:
		/* mode */
		break;
	case 0x03:
		/* control */
		break;
	case 0x04:
		/* baud */
		break;
	}
	log_cb(RETRO_LOG_DEBUG, LOGPRE  "%08x: sio0_w( %04x, %08x )%s\n", activecpu_get_pc(), offset, data, s_char );
}

static WRITE_HANDLER( qsound_bankswitch_w )
{
	cpu_setbank( 10, memory_region( REGION_CPU2 ) + 0x10000 + ( ( data & 0x0f ) * 0x4000 ) );
}

static WRITE_HANDLER( fx1a_sound_bankswitch_w )
{
	cpu_setbank( 10, memory_region( REGION_CPU2 ) + 0x10000 + ( ( ( data - 1 ) & 0x07 ) * 0x4000 ) );
}

static MEMORY_READ32_START( zn_readmem )
	{ 0x00000000, 0x003fffff, MRA32_RAM },		/* ram */
	{ 0x1f000000, 0x1f3fffff, MRA32_BANK1 },	/* game rom */
	{ 0x1f800000, 0x1f8003ff, MRA32_BANK3 },	/* scratchpad */
	{ 0x1f801040, 0x1f80104f, sio0_r },
	{ 0x1f801810, 0x1f801817, psx_gpu_r },
	{ 0x1fb80000, 0x1fbbffff, MRA32_BANK6 },	/* country rom */
	{ 0x80000000, 0x803fffff, MRA32_BANK4 },	/* ram mirror */
	{ 0xa0000000, 0xa03fffff, MRA32_BANK5 },	/* ram mirror */
	{ 0xbfc00000, 0xbfc7ffff, MRA32_BANK7 },	/* bios */
MEMORY_END

static MEMORY_WRITE32_START( zn_writemem )
	{ 0x00000000, 0x003fffff, MWA32_RAM },		/* ram */
	{ 0x1f000000, 0x1f3fffff, MWA32_ROM },		/* game rom */
	{ 0x1f800000, 0x1f8003ff, MWA32_BANK3 },	/* scratchpad */
	{ 0x1f801040, 0x1f80104f, sio0_w },
	{ 0x1f801810, 0x1f801817, psx_gpu_w },
	{ 0x1fb80000, 0x1fbbffff, MWA32_ROM },		/* country rom */
	{ 0x80000000, 0x803fffff, MWA32_BANK4 },	/* ram mirror */
	{ 0xa0000000, 0xa03fffff, MWA32_BANK5 },	/* ram mirror */
	{ 0xbfc00000, 0xbfc7ffff, MWA32_ROM },		/* bios */
MEMORY_END

static MEMORY_READ_START( qsound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK10 },	/* banked (contains music data) */
	{ 0xd007, 0xd007, qsound_status_r },
	{ 0xf000, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( qsound_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xd000, 0xd000, qsound_data_h_w },
	{ 0xd001, 0xd001, qsound_data_l_w },
	{ 0xd002, 0xd002, qsound_cmd_w },
	{ 0xd003, 0xd003, qsound_bankswitch_w },
	{ 0xf000, 0xffff, MWA_RAM },
MEMORY_END

static PORT_READ_START( qsound_readport )
	{ 0x00, 0x00, soundlatch_r },
PORT_END

static MEMORY_READ_START( link_readmem )
MEMORY_END

static MEMORY_WRITE_START( link_writemem )
MEMORY_END

static MEMORY_READ_START( fx1a_sound_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x7fff, MRA_BANK10 },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe000, YM2610_status_port_0_A_r },
	{ 0xe001, 0xe001, YM2610_read_port_0_r },
	{ 0xe002, 0xe002, YM2610_status_port_0_B_r },
	{ 0xe200, 0xe200, MRA_NOP },
	{ 0xe201, 0xe201, taitosound_slave_comm_r },
	{ 0xea00, 0xea00, MRA_NOP },
MEMORY_END

static MEMORY_WRITE_START( fx1a_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe000, YM2610_control_port_0_A_w },
	{ 0xe001, 0xe001, YM2610_data_port_0_A_w },
	{ 0xe002, 0xe002, YM2610_control_port_0_B_w },
	{ 0xe003, 0xe003, YM2610_data_port_0_B_w },
	{ 0xe200, 0xe200, taitosound_slave_port_w },
	{ 0xe201, 0xe201, taitosound_slave_comm_w },
	{ 0xe400, 0xe403, MWA_NOP }, /* pan */
	{ 0xee00, 0xee00, MWA_NOP }, /* ? */
	{ 0xf000, 0xf000, MWA_NOP }, /* ? */
	{ 0xf200, 0xf200, fx1a_sound_bankswitch_w },
MEMORY_END

static DRIVER_INIT( zn )
{
	cpu_setbank( 1, memory_region( REGION_USER1 ) ); /* game rom */
	cpu_setbank( 3, memory_region( REGION_CPU1 ) + 0x0480000 ); /* scratch pad */
	cpu_setbank( 4, memory_region( REGION_CPU1 ) ); /* ram mirror */
	cpu_setbank( 5, memory_region( REGION_CPU1 ) ); /* ram mirror */
	cpu_setbank( 6, memory_region( REGION_USER2 ) ); /* capcom country rom */
	cpu_setbank( 7, memory_region( REGION_CPU1 ) + 0x0400000 ); /* bios */

	if( strcmp( Machine->gamedrv->name, "glpracr" ) == 0 ||
		strcmp( Machine->gamedrv->name, "glprac2l" ) == 0 )
	{
		/* disable:
			the QSound CPU for glpracr as it doesn't have any roms &
			the link cpu for glprac2l as the h/w is not emulated yet. */
		timer_suspendcpu( 1, 1, SUSPEND_REASON_DISABLE );
	}

#if PROTECTION_HACK
	if( strcmp( Machine->gamedrv->name, "doapp" ) == 0 ||
		strcmp( Machine->gamedrv->name, "glpracr2" ) == 0 ||
		strcmp( Machine->gamedrv->name, "glprac2j" ) == 0 ||
		strcmp( Machine->gamedrv->name, "glprac2l" ) == 0 ||
		strcmp( Machine->gamedrv->name, "tondemo" ) == 0 )
	{
		p_sio0_ret = mg_protection_hack;
		n_sio0_ret_size = sizeof( mg_protection_hack );
	}
	else if( strcmp( Machine->gamedrv->name, "sncwgltd" ) == 0 )
	{
		p_sio0_ret = kn_protection_hack;
		n_sio0_ret_size = sizeof( kn_protection_hack );
	}
	else if( strcmp( Machine->gamedrv->name, "ftimpcta" ) == 0 ||
		strcmp( Machine->gamedrv->name, "gdarius" ) == 0 ||
		strcmp( Machine->gamedrv->name, "gdarius2" ) == 0 ||
		strcmp( Machine->gamedrv->name, "mgcldate" ) == 0 ||
		strcmp( Machine->gamedrv->name, "psyforce" ) == 0 ||
		strcmp( Machine->gamedrv->name, "raystorm" ) == 0 ||
		strcmp( Machine->gamedrv->name, "sfchamp" ) == 0 ||
		strcmp( Machine->gamedrv->name, "beastrzb" ) == 0 )
	{
		p_sio0_ret = tt_protection_hack;
		n_sio0_ret_size = sizeof( tt_protection_hack );
	}
	else if( strcmp( Machine->gamedrv->name, "starglad" ) == 0 ||
		strcmp( Machine->gamedrv->name, "ts2u" ) == 0 ||
		strcmp( Machine->gamedrv->name, "ts2j" ) == 0 ||
		strcmp( Machine->gamedrv->name, "sfex" ) == 0 ||
		strcmp( Machine->gamedrv->name, "sfexj" ) == 0 ||
		strcmp( Machine->gamedrv->name, "glpracr" ) == 0 ||
		strcmp( Machine->gamedrv->name, "sfexp" ) == 0 ||
		strcmp( Machine->gamedrv->name, "sfexpj" ) == 0 ||
		strcmp( Machine->gamedrv->name, "rvschool" ) == 0 ||
		strcmp( Machine->gamedrv->name, "jgakuen" ) == 0 ||
		strcmp( Machine->gamedrv->name, "tgmj" ) == 0 )
	{
		p_sio0_ret = cpzn1_protection_hack;
		n_sio0_ret_size = sizeof( cpzn1_protection_hack );
	}
	else if( strcmp( Machine->gamedrv->name, "sfex2" ) == 0 ||
		strcmp( Machine->gamedrv->name, "sg2j" ) == 0 ||
		strcmp( Machine->gamedrv->name, "techromn" ) == 0 ||
		strcmp( Machine->gamedrv->name, "kikaioh" ) == 0 ||
		strcmp( Machine->gamedrv->name, "sfex2p" ) == 0 ||
		strcmp( Machine->gamedrv->name, "strider2" ) == 0 ||
		strcmp( Machine->gamedrv->name, "stridr2a" ) == 0 ||
		strcmp( Machine->gamedrv->name, "shiryu2" ) == 0 )
	{
		p_sio0_ret = cpzn2_protection_hack;
		n_sio0_ret_size = sizeof( cpzn2_protection_hack );
	}
	else if ( strcmp( Machine->gamedrv->name, "beastrzr" ) == 0 ||
		strcmp( Machine->gamedrv->name, "brvblade" ) == 0 )
	{
		p_sio0_ret = psarc95_protection_hack;
		n_sio0_ret_size = sizeof( psarc95_protection_hack );
	}
	else
	{
		p_sio0_ret = NULL;
		n_sio0_ret_size = 0;
	}
#endif
	psx_driver_init();
}

static DRIVER_INIT( fx1b )
{
	cpu_setbank( 10, memory_region( REGION_CPU2 ) );
	init_zn();
}

/* sound player */

static int scode;
static int scode_last;
static int queue_data;
static int queue_len;
static int n_playermode;

static WRITE32_HANDLER( player_queue_w )
{
	if( cpu_getstatus( 1 ) != 0 )
	{
		queue_data = data;
		queue_len = 4;
	}
}

static void player_reset( void )
{
#if PROTECTION_HACK
	n_sio0_ptr = 0;
#endif
	queue_len = 0;
	scode_last = -1;

	if( strcmp( Machine->gamedrv->name, "sfex2" ) == 0 ||
		strcmp( Machine->gamedrv->name, "sfex2p" ) == 0 ||
		strcmp( Machine->gamedrv->name, "tgmj" ) == 0 )
	{
		scode = 0x0400;
	}
	else if( strcmp( Machine->gamedrv->name, "techromn" ) == 0 ||
		strcmp( Machine->gamedrv->name, "kikaioh" ) == 0 )
	{
		scode = 0x8000;
	}
	else
	{
		scode = 0x0000;
	}
	cpu_set_reset_line( 0, PULSE_LINE );
	cpu_set_reset_line( 1, PULSE_LINE );
	if( n_playermode == 0 )
	{
		timer_suspendcpu( 0, 0, SUSPEND_ANY_REASON );
	}
	else
	{
		timer_suspendcpu( 0, 1, SUSPEND_REASON_DISABLE );
	}
}

static VIDEO_UPDATE( player )
{
	if( keyboard_pressed_memory( KEYCODE_F1 ) )
	{
		n_playermode = !n_playermode;
		player_reset();
	}

	if( n_playermode == 0 )
	{
		video_update_psx( bitmap, cliprect );
	}
	else
	{
		struct DisplayText dt[ 4 ];
		char text1[ 256 ];
		char text2[ 256 ];
		char text3[ 256 ];

		if( queue_len == 0 )
		{
			int stick;
			static int old_stick = 0x0f;

			stick = ~readinputport( 4 );
			if( ( stick & old_stick & 0x08 ) != 0 )
			{
				scode=( scode & 0xff00 ) | ( ( scode + 0x0001 ) & 0xff );
			}
			if( ( stick & old_stick & 0x04 ) != 0 )
			{
				scode=( scode & 0xff00 ) | ( ( scode - 0x0001 ) & 0xff );
			}
			if( ( stick & old_stick & 0x01 ) != 0 )
			{
				scode=( ( scode + 0x0100 ) & 0xff00 ) | ( scode & 0xff );
			}
			if( ( stick & old_stick & 0x02 ) != 0 )
			{
				scode=( ( scode - 0x0100 ) & 0xff00 ) | ( scode & 0xff );
			}
			old_stick = ~stick;
		}

		if( scode != scode_last )
		{
			player_queue_w( 0, scode, 0x0000ffff );
			scode_last = scode;
		}

		fillbitmap( bitmap, 0, &Machine->visible_area );

		sprintf( text1, "%s", Machine->gamedrv->description );
		if( strlen( text1 ) > Machine->uiwidth / Machine->uifontwidth )
		{
			text1[ Machine->uiwidth / Machine->uifontwidth ] = 0;
		}
		sprintf( text2, "SOUND CODE=%02x/%02x", scode >> 8, scode & 0xff );
		if( strlen( text2 ) > Machine->uiwidth / Machine->uifontwidth )
		{
			text2[ Machine->uiwidth / Machine->uifontwidth ] = 0;
		}
		sprintf( text3, "SELECT WITH RIGHT&LEFT/UP&DN" );
		if( strlen( text3 ) > Machine->uiwidth / Machine->uifontwidth )
		{
			text3[ Machine->uiwidth / Machine->uifontwidth ] = 0;
		}
		dt[ 0 ].text = text1;
		dt[ 0 ].color = UI_COLOR_NORMAL;
		dt[ 0 ].x = ( Machine->uiwidth - Machine->uifontwidth * strlen( dt[ 0 ].text ) ) / 2;
		dt[ 0 ].y = Machine->uiheight - Machine->uifontheight * 5;
		dt[ 1 ].text = text2;
		dt[ 1 ].color = UI_COLOR_NORMAL;
		dt[ 1 ].x = ( Machine->uiwidth - Machine->uifontwidth * strlen( dt[ 1 ].text ) ) / 2;
		dt[ 1 ].y = Machine->uiheight - Machine->uifontheight * 3;
		dt[ 2 ].text = text3;
		dt[ 2 ].color = UI_COLOR_NORMAL;
		dt[ 2 ].x = ( Machine->uiwidth - Machine->uifontwidth * strlen( dt[ 2 ].text ) ) / 2;
		dt[ 2 ].y = Machine->uiheight - Machine->uifontheight * 1;
		dt[ 3 ].text = 0; /* terminate array */
		displaytext( Machine->scrbitmap, dt );
	}
}

static MACHINE_INIT( zn )
{
	n_playermode = 0;
	player_reset();
	psx_machine_init();
}

static INTERRUPT_GEN( qsound_interrupt )
{
	if( queue_len == 4 )
	{
		soundlatch_w( 0, queue_data >> 8 );
		queue_len -= 2;
		cpu_set_irq_line(1, IRQ_LINE_NMI, PULSE_LINE);
	}
	else if( queue_len == 2 )
	{
		soundlatch_w( 0, queue_data & 0xff );
		queue_len -= 2;
		cpu_set_irq_line(1, IRQ_LINE_NMI, PULSE_LINE);
	}
	else
	{
		cpu_set_irq_line(1, 0, HOLD_LINE);
	}
}

static INTERRUPT_GEN( fx1a_sound_interrupt )
{
	if( queue_len == 4 )
	{
		taitosound_port_w( 0, 0 );
		taitosound_comm_w( 0, ( queue_data >> 0 ) & 0x0f );
		queue_len--;
	}
	else if( queue_len == 3 )
	{
		taitosound_port_w( 0, 1 );
		taitosound_comm_w( 0, ( queue_data >> 4 ) & 0x0f );
		queue_len--;
	}
	if( queue_len == 2 )
	{
		taitosound_port_w( 0, 2 );
		taitosound_comm_w( 0, ( queue_data >> 8 ) & 0x0f );
		queue_len--;
	}
	else if( queue_len == 1 )
	{
		taitosound_port_w( 0, 3 );
		taitosound_comm_w( 0, ( queue_data >> 12 ) & 0x0f );
		queue_len--;
	}
}

static struct QSound_interface qsound_interface =
{
	QSOUND_CLOCK,
	REGION_SOUND1,
	{ 100,100 }
};

/* handler called by the YM2610 emulator when the internal timers cause an IRQ */
static void irq_handler(int irq)
{
	cpu_set_irq_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2610interface ym2610_interface =
{
	1,	/* 1 chip */
	16000000/2,	/* 8 MHz */
	{ 25 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irq_handler },
	{ REGION_SOUND1 },	/* Delta-T */
	{ REGION_SOUND1 },	/* ADPCM */
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) }
};

static MACHINE_DRIVER_START( zn )
	/* basic machine hardware */
	MDRV_CPU_ADD(PSXCPU, 33868800) /* 33MHz ?? */
	MDRV_CPU_MEMORY(zn_readmem,zn_writemem)
	MDRV_CPU_VBLANK_INT(psx_vblank,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(0)

	MDRV_MACHINE_INIT(zn)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(640, 480)
	MDRV_VISIBLE_AREA(0, 479, 0, 479)
	MDRV_PALETTE_LENGTH(65536)

	MDRV_PALETTE_INIT(psx)
	MDRV_VIDEO_START(psx_type2_1024x1024)
	MDRV_VIDEO_UPDATE(psx)
	MDRV_VIDEO_STOP(psx)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( znqsound )
	/* basic machine hardware */
	MDRV_CPU_ADD(PSXCPU, 33000000) /* 33MHz ?? */
	MDRV_CPU_MEMORY(zn_readmem,zn_writemem)
	MDRV_CPU_VBLANK_INT(psx_vblank,1)

	MDRV_CPU_ADD(Z80, 8000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)  /* 8MHz ?? */
	MDRV_CPU_MEMORY(qsound_readmem,qsound_writemem)
	MDRV_CPU_PORTS(qsound_readport,0)
	MDRV_CPU_VBLANK_INT(qsound_interrupt,4) /* 4 interrupts per frame ?? */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(0)

	MDRV_MACHINE_INIT(zn)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(640, 480)
	MDRV_VISIBLE_AREA(0, 479, 0, 479)
	MDRV_PALETTE_LENGTH(65536)

	MDRV_PALETTE_INIT(psx)
	MDRV_VIDEO_START(psx_type2_1024x1024)
	MDRV_VIDEO_UPDATE(player)
	MDRV_VIDEO_STOP(psx)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(QSOUND, qsound_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( znlink )
	/* basic machine hardware */
	MDRV_CPU_ADD(PSXCPU, 33000000) /* 33MHz ?? */
	MDRV_CPU_MEMORY(zn_readmem,zn_writemem)
	MDRV_CPU_VBLANK_INT(psx_vblank,1)

	MDRV_CPU_ADD(Z80, 8000000)
	MDRV_CPU_MEMORY(link_readmem,link_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(0)

	MDRV_MACHINE_INIT(zn)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(640, 480)
	MDRV_VISIBLE_AREA(0, 479, 0, 479)
	MDRV_PALETTE_LENGTH(65536)

	MDRV_PALETTE_INIT(psx)
	MDRV_VIDEO_START(psx_type2_1024x1024)
	MDRV_VIDEO_UPDATE(psx)
	MDRV_VIDEO_STOP(psx)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( fx1a )
	/* basic machine hardware */
	MDRV_CPU_ADD(PSXCPU, 33000000) /* 33MHz ?? */
	MDRV_CPU_MEMORY(zn_readmem,zn_writemem)
	MDRV_CPU_VBLANK_INT(psx_vblank,1)

	MDRV_CPU_ADD(Z80,16000000/4)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 4 MHz */
	MDRV_CPU_MEMORY(fx1a_sound_readmem,fx1a_sound_writemem)
	MDRV_CPU_VBLANK_INT(fx1a_sound_interrupt,1) /* 4 interrupts per frame ?? */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(0)

	MDRV_MACHINE_INIT(zn)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(640, 480)
	MDRV_VISIBLE_AREA(0, 479, 0, 479)
	MDRV_PALETTE_LENGTH(65536)

	MDRV_PALETTE_INIT(psx)
	MDRV_VIDEO_START(psx_type2_1024x1024)
	MDRV_VIDEO_UPDATE(player)
	MDRV_VIDEO_STOP(psx)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2610, ym2610_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( fx1b )
	/* basic machine hardware */
	MDRV_CPU_ADD(PSXCPU, 33000000) /* 33MHz ?? */
	MDRV_CPU_MEMORY(zn_readmem,zn_writemem)
	MDRV_CPU_VBLANK_INT(psx_vblank,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(0)

	MDRV_MACHINE_INIT(zn)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(640, 480)
	MDRV_VISIBLE_AREA(0, 479, 0, 479)
	MDRV_PALETTE_LENGTH(65536)

	MDRV_PALETTE_INIT(psx)
	MDRV_VIDEO_START(psx_type2_1024x1024)
	MDRV_VIDEO_UPDATE(psx)
	MDRV_VIDEO_STOP(psx)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
MACHINE_DRIVER_END

INPUT_PORTS_START( zn )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE )	/* pause */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE	)	/* pause */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER2  )

	PORT_START		/* DSWA */
	PORT_DIPNAME( 0xff, 0xff, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0xff, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START		/* DSWB */
	PORT_DIPNAME( 0xff, 0xff, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0xff, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START		/* DSWC */
	PORT_DIPNAME( 0xff, 0xff, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0xff, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START		/* Player 1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )

	PORT_START		/* Player 2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
INPUT_PORTS_END


/* Capcom ZN1 */

#define CPZN1_BIOS \
	ROM_REGION( 0x480400, REGION_CPU1, 0 ) \
	ROM_LOAD( "coh1002c.bin", 0x0400000, 0x080000, CRC(50033af6) SHA1(486d92ff6c7f1e54f8e0ef41cd9116eca0e10e1a) )

ROM_START( cpzn1 )
	CPZN1_BIOS
ROM_END

ROM_START( glpracr )
	CPZN1_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER2, 0 )
	ROM_LOAD( "graj-04.2j", 0x0000000, 0x080000, CRC(53bf551c) SHA1(320632b5010630cee4c5ccb1578d5ee6d2754632) )

	ROM_REGION32_LE( 0x0c00000, REGION_USER1, 0 )
	ROM_LOAD( "gra-05m.3j", 0x0000000, 0x400000, CRC(78053700) SHA1(38727c8cc34bb57b7b7e73041e382fb0361f184e) )
	ROM_LOAD( "gra-06m.4j", 0x0400000, 0x400000, CRC(d73b392b) SHA1(241ddf474cea035e81a2abc580d3c0395ee925bb) )
	ROM_LOAD( "gra-07m.5j", 0x0800000, 0x400000, CRC(acaefe3a) SHA1(32d596b0f975e1558fa7929c3166d8dad40a1c80) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
/* there are no QSound program roms
	ROM_LOAD( "gra-02",  0x00000, 0x08000, NO_DUMP )
	ROM_CONTINUE(        0x10000, 0x18000 )
	ROM_LOAD( "gra-03",  0x28000, 0x20000, NO_DUMP )
*/
	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
/* or QSound sample roms either
	ROM_LOAD( "gra-01m", 0x0000000, 0x400000, NO_DUMP )
*/
ROM_END

ROM_START( rvschool )
	CPZN1_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER2, 0 )
	ROM_LOAD( "jst-04a", 0x0000000, 0x080000, CRC(034b1011) SHA1(6773246be242ee336503d21d7d44a3884832eb1e) )

	ROM_REGION32_LE( 0x2400000, REGION_USER1, 0 )
	ROM_LOAD( "jst-05m", 0x0000000, 0x400000, CRC(723372b8) SHA1(2a7c95d1f9a3f58c469dfc28ead1fd192eaaebd1) )
	ROM_LOAD( "jst-06m", 0x0400000, 0x400000, CRC(4248988e) SHA1(4bdf7cac17d70ea85aa2002fc6b21a64d05e6e5a) )
	ROM_LOAD( "jst-07m", 0x0800000, 0x400000, CRC(c84c5a16) SHA1(5c0ca7454189c766f1ca7305504ff1867007c8e6) )
	ROM_LOAD( "jst-08m", 0x0c00000, 0x400000, CRC(791b57f3) SHA1(4ea12a0f7a7110d7dcbc55b3f02aa9a92dea4b12) )
	ROM_LOAD( "jst-09m", 0x1000000, 0x400000, CRC(6df42048) SHA1(9e2b4a424de3918e5e54bc87fd9dcceff8d162be) )
	ROM_LOAD( "jst-10m", 0x1400000, 0x400000, CRC(d7e22769) SHA1(733f96dce2586fc0a8af3cec18153085750c9a4d) )
	ROM_LOAD( "jst-11m", 0x1800000, 0x400000, CRC(0a033ac5) SHA1(218b33cb51db99d3e9ee180da6a74460f4444fc6) )
	ROM_LOAD( "jst-12m", 0x1c00000, 0x400000, CRC(43bd2ddd) SHA1(7f2976e394362cb648f620e430b3bf11b71485a6) )
	ROM_LOAD( "jst-13m", 0x2000000, 0x400000, CRC(6b443235) SHA1(c764d8b742aa1c46bc8d37f36e864ef50a1ff4e4) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "jst-02",  0x00000, 0x08000, CRC(7809e2c3) SHA1(0216a665f7978bc8db3f7fdab038e1c7aa120844) )
	ROM_CONTINUE(        0x10000, 0x18000 )
	ROM_LOAD( "jst-03",  0x28000, 0x20000, CRC(860ff24d) SHA1(eea72fa5eaf407a112a5b3daf60f7ac8ad191cc7) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "jst-01m", 0x0000000, 0x400000, CRC(9a7c98f9) SHA1(764c6c4f41047e1f36d2dceac4aa9b943a9d529a) )
ROM_END

ROM_START( jgakuen )
	CPZN1_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER2, 0 )
	ROM_LOAD( "jst-04j", 0x0000000, 0x080000, CRC(28b8000a) SHA1(9ebf74b453d775cadca9c2d7d8e2c7eb57bb9a38) )

	ROM_REGION32_LE( 0x2400000, REGION_USER1, 0 )
	ROM_LOAD( "jst-05m", 0x0000000, 0x400000, CRC(723372b8) SHA1(2a7c95d1f9a3f58c469dfc28ead1fd192eaaebd1) )
	ROM_LOAD( "jst-06m", 0x0400000, 0x400000, CRC(4248988e) SHA1(4bdf7cac17d70ea85aa2002fc6b21a64d05e6e5a) )
	ROM_LOAD( "jst-07m", 0x0800000, 0x400000, CRC(c84c5a16) SHA1(5c0ca7454189c766f1ca7305504ff1867007c8e6) )
	ROM_LOAD( "jst-08m", 0x0c00000, 0x400000, CRC(791b57f3) SHA1(4ea12a0f7a7110d7dcbc55b3f02aa9a92dea4b12) )
	ROM_LOAD( "jst-09m", 0x1000000, 0x400000, CRC(6df42048) SHA1(9e2b4a424de3918e5e54bc87fd9dcceff8d162be) )
	ROM_LOAD( "jst-10m", 0x1400000, 0x400000, CRC(d7e22769) SHA1(733f96dce2586fc0a8af3cec18153085750c9a4d) )
	ROM_LOAD( "jst-11m", 0x1800000, 0x400000, CRC(0a033ac5) SHA1(218b33cb51db99d3e9ee180da6a74460f4444fc6) )
	ROM_LOAD( "jst-12m", 0x1c00000, 0x400000, CRC(43bd2ddd) SHA1(7f2976e394362cb648f620e430b3bf11b71485a6) )
	ROM_LOAD( "jst-13m", 0x2000000, 0x400000, CRC(6b443235) SHA1(c764d8b742aa1c46bc8d37f36e864ef50a1ff4e4) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "jst-02",  0x00000, 0x08000, CRC(7809e2c3) SHA1(0216a665f7978bc8db3f7fdab038e1c7aa120844) )
	ROM_CONTINUE(        0x10000, 0x18000 )
	ROM_LOAD( "jst-03",  0x28000, 0x20000, CRC(860ff24d) SHA1(eea72fa5eaf407a112a5b3daf60f7ac8ad191cc7) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "jst-01m", 0x0000000, 0x400000, CRC(9a7c98f9) SHA1(764c6c4f41047e1f36d2dceac4aa9b943a9d529a) )
ROM_END

ROM_START( sfex )
	CPZN1_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER2, 0 )
	ROM_LOAD( "sfe-04a", 0x0000000, 0x080000, CRC(08247bd4) SHA1(07f356ef2827b3fbd0bfaf2010915315d9d60ef1) )

	ROM_REGION32_LE( 0x1800000, REGION_USER1, 0 )
	ROM_LOAD( "sfe-05m", 0x0000000, 0x400000, CRC(eab781fe) SHA1(205476cb72c8dac915e140fb32243dfc5d209ba4) )
	ROM_LOAD( "sfe-06m", 0x0400000, 0x400000, CRC(999de60c) SHA1(092882698c411fc5c3bcb43105bf1886f94b8e40) )
	ROM_LOAD( "sfe-07m", 0x0800000, 0x400000, CRC(76117b0a) SHA1(027233199170fa6e5b32f28da2031638c6d3d14a) )
	ROM_LOAD( "sfe-08m", 0x0c00000, 0x400000, CRC(a36bbec5) SHA1(fa22ea50d4d8bed2ded97a346f61b2f5f68769b9) )
	ROM_LOAD( "sfe-09m", 0x1000000, 0x400000, CRC(62c424cc) SHA1(ea19c49b486473b150dbf8541286e225655496db) )
	ROM_LOAD( "sfe-10m", 0x1400000, 0x400000, CRC(83791a8b) SHA1(534969797640834ca692c11d0ce7c3a060fc7e4b) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sfe-02",  0x00000, 0x08000, CRC(1908475c) SHA1(99f68cff2d92f5697eec0846201f6fb317d5dc08) )
	ROM_CONTINUE(        0x10000, 0x18000 )
	ROM_LOAD( "sfe-03",  0x28000, 0x20000, CRC(95c1e2e0) SHA1(383bbe9613798a3ac6944d18768280a840994e40) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "sfe-01m", 0x0000000, 0x400000, CRC(f5afff0d) SHA1(7f9ac32ba0a3d9c6fef367e36a92d47c9ac1feb3) )
ROM_END

ROM_START( sfexj )
	CPZN1_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER2, 0 )
	ROM_LOAD( "sfe-04j", 0x0000000, 0x080000, CRC(ea100607) SHA1(27ef8c619804999d32d14fcc5ec783c057b4dc73) )

	ROM_REGION32_LE( 0x1800000, REGION_USER1, 0 )
	ROM_LOAD( "sfe-05m", 0x0000000, 0x400000, CRC(eab781fe) SHA1(205476cb72c8dac915e140fb32243dfc5d209ba4) )
	ROM_LOAD( "sfe-06m", 0x0400000, 0x400000, CRC(999de60c) SHA1(092882698c411fc5c3bcb43105bf1886f94b8e40) )
	ROM_LOAD( "sfe-07m", 0x0800000, 0x400000, CRC(76117b0a) SHA1(027233199170fa6e5b32f28da2031638c6d3d14a) )
	ROM_LOAD( "sfe-08m", 0x0c00000, 0x400000, CRC(a36bbec5) SHA1(fa22ea50d4d8bed2ded97a346f61b2f5f68769b9) )
	ROM_LOAD( "sfe-09m", 0x1000000, 0x400000, CRC(62c424cc) SHA1(ea19c49b486473b150dbf8541286e225655496db) )
	ROM_LOAD( "sfe-10m", 0x1400000, 0x400000, CRC(83791a8b) SHA1(534969797640834ca692c11d0ce7c3a060fc7e4b) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sfe-02",  0x00000, 0x08000, CRC(1908475c) SHA1(99f68cff2d92f5697eec0846201f6fb317d5dc08) )
	ROM_CONTINUE(        0x10000, 0x18000 )
	ROM_LOAD( "sfe-03",  0x28000, 0x20000, CRC(95c1e2e0) SHA1(383bbe9613798a3ac6944d18768280a840994e40) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "sfe-01m", 0x0000000, 0x400000, CRC(f5afff0d) SHA1(7f9ac32ba0a3d9c6fef367e36a92d47c9ac1feb3) )
ROM_END

ROM_START( sfexp )
	CPZN1_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER2, 0 )
	ROM_LOAD( "sfp-04e", 0x0000000, 0x080000, CRC(305e4ec0) SHA1(0df9572d7fc1bbc7131483960771d016fa5487a5) )

	ROM_REGION32_LE( 0x1880000, REGION_USER1, 0 )
	ROM_LOAD( "sfp-05",  0x0000000, 0x400000, CRC(ac7dcc5e) SHA1(216de2de691a9bd7982d5d6b5b1e3e35ff381a2f) )
	ROM_LOAD( "sfp-06",  0x0400000, 0x400000, CRC(1d504758) SHA1(bd56141aba35dbb5b318445ba5db12eff7442221) )
	ROM_LOAD( "sfp-07",  0x0800000, 0x400000, CRC(0f585f30) SHA1(24ffdbc360f8eddb702905c99d315614327861a7) )
	ROM_LOAD( "sfp-08",  0x0c00000, 0x400000, CRC(65eabc61) SHA1(bbeb3bcd8dd8f7f88ed82412a81134a3d6f6ffd9) )
	ROM_LOAD( "sfp-09",  0x1000000, 0x400000, CRC(15f8b71e) SHA1(efb28fbe750f443550ee9718385355aae7e858c9) )
	ROM_LOAD( "sfp-10",  0x1400000, 0x400000, CRC(c1ecf652) SHA1(616e14ff63d38272730c810b933a6b3412e2da17) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sfe-02",  0x00000, 0x08000, CRC(1908475c) SHA1(99f68cff2d92f5697eec0846201f6fb317d5dc08) )
	ROM_CONTINUE(        0x10000, 0x18000 )
	ROM_LOAD( "sfe-03",  0x28000, 0x20000, CRC(95c1e2e0) SHA1(383bbe9613798a3ac6944d18768280a840994e40) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "sfe-01m", 0x0000000, 0x400000, CRC(f5afff0d) SHA1(7f9ac32ba0a3d9c6fef367e36a92d47c9ac1feb3) )
ROM_END

ROM_START( sfexpj )
	CPZN1_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER2, 0 )
	ROM_LOAD( "sfp-04j", 0x0000000, 0x080000, CRC(18d043f5) SHA1(9e6e24a722d13888fbfd391ddb1a5045b162488c) )

	ROM_REGION32_LE( 0x1800000, REGION_USER1, 0 )
	ROM_LOAD( "sfp-05",  0x0000000, 0x400000, CRC(ac7dcc5e) SHA1(216de2de691a9bd7982d5d6b5b1e3e35ff381a2f) )
	ROM_LOAD( "sfp-06",  0x0400000, 0x400000, CRC(1d504758) SHA1(bd56141aba35dbb5b318445ba5db12eff7442221) )
	ROM_LOAD( "sfp-07",  0x0800000, 0x400000, CRC(0f585f30) SHA1(24ffdbc360f8eddb702905c99d315614327861a7) )
	ROM_LOAD( "sfp-08",  0x0c00000, 0x400000, CRC(65eabc61) SHA1(bbeb3bcd8dd8f7f88ed82412a81134a3d6f6ffd9) )
	ROM_LOAD( "sfp-09",  0x1000000, 0x400000, CRC(15f8b71e) SHA1(efb28fbe750f443550ee9718385355aae7e858c9) )
	ROM_LOAD( "sfp-10",  0x1400000, 0x400000, CRC(c1ecf652) SHA1(616e14ff63d38272730c810b933a6b3412e2da17) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sfe-02",  0x00000, 0x08000, CRC(1908475c) SHA1(99f68cff2d92f5697eec0846201f6fb317d5dc08) )
	ROM_CONTINUE(        0x10000, 0x18000 )
	ROM_LOAD( "sfe-03",  0x28000, 0x20000, CRC(95c1e2e0) SHA1(383bbe9613798a3ac6944d18768280a840994e40) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "sfe-01m", 0x0000000, 0x400000, CRC(f5afff0d) SHA1(7f9ac32ba0a3d9c6fef367e36a92d47c9ac1feb3) )
ROM_END

ROM_START( starglad )
	CPZN1_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER2, 0 )
	ROM_LOAD( "ps1u-04.2h",   0x000000, 0x080000, CRC(121fb234) SHA1(697d18d37afd95f302b40a5a6a78d8c92a41ea73) )

	ROM_REGION32_LE( 0x0800000, REGION_USER1, 0 )
	ROM_LOAD( "ps1-05m.3h",   0x000000, 0x400000, CRC(8ad72c4f) SHA1(c848c37eb5365000b4d4720b5c08d89ddd8e2c33) )
	ROM_LOAD( "ps1-06m.4h",   0x000000, 0x400000, CRC(95d8ed61) SHA1(e9f259d589dc38a8321a6fea1f5dac741cadc0ff) )
	ROM_LOAD( "ps1-07m.5h",   0x000000, 0x400000, CRC(c06752db) SHA1(0884b308e9cd9dde8660b422bc8fec9a362bcb52) )
	ROM_LOAD( "ps1-08m.2k",   0x000000, 0x400000, CRC(381f9ded) SHA1(b7878a90740f5b3c5881ac7d46e2b84b18727337) )
	ROM_LOAD( "ps1-09m.3k",   0x000000, 0x400000, CRC(bd894812) SHA1(9f0c3365e685a53ae793f4a256a6c177a843a424) )
	ROM_LOAD( "ps1-10m.4k",   0x000000, 0x400000, CRC(ff80c18a) SHA1(8d01717eed6ec1f508fe7c445da941fb84ef7d22) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "ps1-02a.2e",   0x000000, 0x008000, CRC(b854df92) SHA1(ea71a613b5b19ec7e9c6e342e7743d320582a6bb) )
	ROM_CONTINUE(             0x010000, 0x018000 )
	ROM_LOAD( "ps1-03a.3e",   0x028000, 0x020000, CRC(a2562fbb) SHA1(3de02a4aa7ea620961ca2a5c331f38134033db79) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "ps1-01m.3b",   0x000000, 0x400000, CRC(0bfb17aa) SHA1(cf4482785a2a33ad814c8b1461c5bc8e8e027895) )
ROM_END

ROM_START( tgmj )
	CPZN1_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER2, 0 )
	ROM_LOAD( "ate-04j", 0x0000000, 0x080000, CRC(bb4bbb96) SHA1(808f4b29493e74efd661d561d11cbec2f4afd1c8) )

	ROM_REGION32_LE( 0x0800000, REGION_USER1, 0 )
	ROM_LOAD( "ate-05",  0x0000000, 0x400000, CRC(50977f5a) SHA1(78c2b1965957ff1756c25b76e549f11fc0001153) )
	ROM_LOAD( "ate-06",  0x0400000, 0x400000, CRC(05973f16) SHA1(c9262e8de14c4a9489f7050316012913c1caf0ff) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "ate-02",  0x00000, 0x08000, CRC(f4f6e82f) SHA1(ad6c49197a60f456367c9f78353741fb847819a1) )
	ROM_CONTINUE(        0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "ate-01",  0x0000000, 0x400000, CRC(a21c6521) SHA1(560e4855f6e00def5277bdd12064b49e55c3b46b) )
ROM_END

ROM_START( ts2 )
	CPZN1_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER2, 0 )
	ROM_LOAD( "ts2u-04", 0x0000000, 0x080000, CRC(ddb52e7c) SHA1(e77891abae7681d911ef6eba2e0920d81433ebe6) ) 

	ROM_REGION32_LE( 0x0e00000, REGION_USER1, 0 )
	ROM_LOAD( "ts2-05",  0x0000000, 0x400000, CRC(7f4228e2) SHA1(3690a76d19d97e55bc7b05a8456328697cfd7a77) )
	ROM_LOAD( "ts2-06m", 0x0400000, 0x400000, CRC(cd7e0a27) SHA1(325b5f2e653cdea07cddc9d20d12b5ab50dca949) )
	ROM_LOAD( "ts2-08m", 0x0800000, 0x400000, CRC(b1f7f115) SHA1(3f416d2aac07aa73a99593b5a21b047da60cea6a) )
	ROM_LOAD( "ts2-10",  0x0c00000, 0x200000, CRC(ad90679a) SHA1(19dd30764f892ee7f89c78ccbccdaf4d6b0e6e09) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "ts2-02",  0x00000, 0x08000, CRC(2f45c461) SHA1(513b6b9b5a2f7c567c30c958e0e13834cd9bd266) )
	ROM_CONTINUE(        0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "ts2-01",  0x0000000, 0x400000, CRC(d7a505e0) SHA1(f1b0cdea712101f695bd326eccd753eb79a07490) )
ROM_END

ROM_START( ts2j )
	CPZN1_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER2, 0 )
	ROM_LOAD( "ts2j-04", 0x0000000, 0x080000, CRC(4aba8c5e) SHA1(a56001bf50bfc1b03036e88ae1febd1aac8c63c0) )

	ROM_REGION32_LE( 0x0e00000, REGION_USER1, 0 )
	ROM_LOAD( "ts2-05",  0x0000000, 0x400000, CRC(7f4228e2) SHA1(3690a76d19d97e55bc7b05a8456328697cfd7a77) )
	ROM_LOAD( "ts2-06m", 0x0400000, 0x400000, CRC(cd7e0a27) SHA1(325b5f2e653cdea07cddc9d20d12b5ab50dca949) )
	ROM_LOAD( "ts2-08m", 0x0800000, 0x400000, CRC(b1f7f115) SHA1(3f416d2aac07aa73a99593b5a21b047da60cea6a) )
	ROM_LOAD( "ts2-10",  0x0c00000, 0x200000, CRC(ad90679a) SHA1(19dd30764f892ee7f89c78ccbccdaf4d6b0e6e09) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "ts2-02",  0x00000, 0x08000, CRC(2f45c461) SHA1(513b6b9b5a2f7c567c30c958e0e13834cd9bd266) )
	ROM_CONTINUE(        0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "ts2-01",  0x0000000, 0x400000, CRC(d7a505e0) SHA1(f1b0cdea712101f695bd326eccd753eb79a07490) )
ROM_END

/* Capcom ZN2 */

#define CPZN2_BIOS \
	ROM_REGION32_LE( 0x480400, REGION_CPU1, 0 ) \
	ROM_LOAD( "coh3002c.bin", 0x0400000, 0x080000, CRC(e860ea8b) SHA1(66e7e1d4e426466b8f48a2ba055a91b475569504) )

ROM_START( cpzn2 )
	CPZN2_BIOS
ROM_END

ROM_START( techromn )
	CPZN2_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER2, 0 )
	ROM_LOAD( "kiou-04",     0x0000000, 0x080000, CRC(08aca34a) SHA1(768a37f719af5d96993db5592b6505b013e0d6f4) )

	ROM_REGION32_LE( 0x3000000, REGION_USER1, 0 )
	ROM_LOAD( "kio-05m.bin", 0x0000000, 0x800000, CRC(98e9eb24) SHA1(144773296c213ab09d626c915f90bb74e24487f0) )
	ROM_LOAD( "kio-06m.bin", 0x0800000, 0x800000, CRC(be8d7d73) SHA1(bcbbbd0b83503f2ed32527444e0da3afd774d3f7) )
	ROM_LOAD( "kio-07m.bin", 0x1000000, 0x800000, CRC(ffd81f18) SHA1(f8387a9d45e79f97ccdffabe755638a60f80ccf5) )
	ROM_LOAD( "kio-08m.bin", 0x1800000, 0x800000, CRC(17302226) SHA1(976ba7f48c9a52d24388cd63d02be08627cf2e30) )
	ROM_LOAD( "kio-09m.bin", 0x2000000, 0x800000, CRC(a34f2119) SHA1(50fa992eba5324a173fcc0923227c13cad4f97e5) )
	ROM_LOAD( "kio-10m.bin", 0x2800000, 0x800000, CRC(7400037a) SHA1(d58641e1d6bf1c6ca04f6c98d6809edaa7df75d3) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "kio-02.bin",  0x00000, 0x08000, CRC(174309b3) SHA1(b35b9c3905d2fabaa8410f70f7b382e916c89733) )
	ROM_CONTINUE(            0x10000, 0x18000 )
	ROM_LOAD( "kio-03.bin",  0x28000, 0x20000, CRC(0b313ae5) SHA1(0ea39305ca30f376930e39b134fd1a52200624fa) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "kio-01m.bin", 0x0000000, 0x400000, CRC(6dc5bd07) SHA1(e1755a48465f741691ea0fa1166cb2dc09210ed9) )
ROM_END

ROM_START( kikaioh )
	CPZN2_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER2, 0 )
	ROM_LOAD( "kioj-04.bin", 0x0000000, 0x080000, CRC(3a2a3bc8) SHA1(3c4ae3cfe00a7f60ab2196ae042dab4a8eb6f597) )

	ROM_REGION32_LE( 0x3000000, REGION_USER1, 0 )
	ROM_LOAD( "kio-05m.bin", 0x0000000, 0x800000, CRC(98e9eb24) SHA1(144773296c213ab09d626c915f90bb74e24487f0) )
	ROM_LOAD( "kio-06m.bin", 0x0800000, 0x800000, CRC(be8d7d73) SHA1(bcbbbd0b83503f2ed32527444e0da3afd774d3f7) )
	ROM_LOAD( "kio-07m.bin", 0x1000000, 0x800000, CRC(ffd81f18) SHA1(f8387a9d45e79f97ccdffabe755638a60f80ccf5) )
	ROM_LOAD( "kio-08m.bin", 0x1800000, 0x800000, CRC(17302226) SHA1(976ba7f48c9a52d24388cd63d02be08627cf2e30) )
	ROM_LOAD( "kio-09m.bin", 0x2000000, 0x800000, CRC(a34f2119) SHA1(50fa992eba5324a173fcc0923227c13cad4f97e5) )
	ROM_LOAD( "kio-10m.bin", 0x2800000, 0x800000, CRC(7400037a) SHA1(d58641e1d6bf1c6ca04f6c98d6809edaa7df75d3) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "kio-02.bin",  0x00000, 0x08000, CRC(174309b3) SHA1(b35b9c3905d2fabaa8410f70f7b382e916c89733) )
	ROM_CONTINUE(            0x10000, 0x18000 )
	ROM_LOAD( "kio-03.bin",  0x28000, 0x20000, CRC(0b313ae5) SHA1(0ea39305ca30f376930e39b134fd1a52200624fa) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "kio-01m.bin", 0x0000000, 0x400000, CRC(6dc5bd07) SHA1(e1755a48465f741691ea0fa1166cb2dc09210ed9) )
ROM_END

ROM_START( sfex2 )
	CPZN2_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER2, 0 )
	ROM_LOAD( "ex2j-04", 0x0000000, 0x080000, CRC(5d603586) SHA1(ff546d3bd011d6441e9672b88bab763d3cd89be2) )

	ROM_REGION32_LE( 0x2400000, REGION_USER1, 0 )
	ROM_LOAD( "ex2-05m", 0x0000000, 0x800000, CRC(78726b17) SHA1(2da449df335ef133ebc3997bbad73ef4137f4771) )
	ROM_LOAD( "ex2-06m", 0x0800000, 0x800000, CRC(be1075ed) SHA1(36dc673372f30f8b3ff5689ae568c5cd01fe2c07) )
	ROM_LOAD( "ex2-07m", 0x1000000, 0x800000, CRC(6496c6ed) SHA1(054bcecbb04033abea14d9ffe6634b2bd11ca88b) )
	ROM_LOAD( "ex2-08m", 0x1800000, 0x800000, CRC(3194132e) SHA1(d1324fcf0a8528fc683791d6342697a7e08674f4) )
	ROM_LOAD( "ex2-09m", 0x2000000, 0x400000, CRC(075ae585) SHA1(6b88851db618fc3e96f1d740c46c1bc5be0ee21b) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "ex2-02",  0x00000, 0x08000, CRC(9489875e) SHA1(1fc9985ff98232c63ea8d05a69f7d77cdf72919f) )
	ROM_CONTINUE(        0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "ex2-01m", 0x0000000, 0x400000, CRC(14a5bb0e) SHA1(dfe3c3a53bd4c58743d8039b5344d3afbe2a9c24) )
ROM_END

ROM_START( sfex2p )
	CPZN2_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER2, 0 )
	ROM_LOAD( "sf2p-04", 0x0000000, 0x080000, CRC(c6d0aea3) SHA1(f48ee889dd743109f830063da3eb0f687db2d86c) )

	ROM_REGION32_LE( 0x3000000, REGION_USER1, 0 )
	ROM_LOAD( "sf2p-05", 0x0000000, 0x800000, CRC(4ee3110f) SHA1(704f8dca7d0b698659af9e3271ea5072dfd42b8b) )
	ROM_LOAD( "sf2p-06", 0x0800000, 0x800000, CRC(4cd53a45) SHA1(39499ea6c9aa51c71f4fe44cc02f93d5a39e14ec) )
	ROM_LOAD( "sf2p-07", 0x1000000, 0x800000, CRC(11207c2a) SHA1(0182652819f1c3a36e7b42e34ef86d2455a2dd90) )
	ROM_LOAD( "sf2p-08", 0x1800000, 0x800000, CRC(3560c2cc) SHA1(8b0ce22d954387f7bb032b5220d1014ef68741e8) )
	ROM_LOAD( "sf2p-09", 0x2000000, 0x800000, CRC(344aa227) SHA1(69dc6f511939bf7fa25c2531ecf307a7565fe7a8) )
	ROM_LOAD( "sf2p-10", 0x2800000, 0x800000, CRC(2eef5931) SHA1(e5227529fb68eeb1b2f25813694173a75d906b52) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sf2p-02", 0x00000, 0x08000, CRC(3705de5e) SHA1(847007ca271da64bf13ffbf496d4291429eee27a) )
	ROM_CONTINUE(        0x10000, 0x18000 )
	ROM_LOAD( "sf2p-03", 0x28000, 0x20000, CRC(6ae828f6) SHA1(41c54165e87b846a845da581f408b96979288158) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "ex2-01m", 0x0000000, 0x400000, CRC(14a5bb0e) SHA1(dfe3c3a53bd4c58743d8039b5344d3afbe2a9c24) )
ROM_END

ROM_START( sg2j )
	CPZN2_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER2, 0 )
	ROM_LOAD( "sg2j-04", 0x0000000, 0x080000, CRC(cf4ce6ac) SHA1(52b6f61d79671c9c108b3dfbd3c2ac333285412c) )

	ROM_REGION32_LE( 0x2400000, REGION_USER1, 0 )
	ROM_LOAD( "sg2-05m", 0x0000000, 0x800000, CRC(f1759236) SHA1(fbe3a820a8c571dfb186eae68346e6461168ed48) )
	ROM_LOAD( "sg2-06m", 0x0800000, 0x800000, CRC(33de4f72) SHA1(ab32af76b5682e3d9f67dadbaed35abc043912b4) )
	ROM_LOAD( "sg2-07m", 0x1000000, 0x800000, CRC(72f724ba) SHA1(e6658b495d308d1de6710f87b5b9d346008b0c5a) )
	ROM_LOAD( "sg2-08m", 0x1800000, 0x800000, CRC(9e169eee) SHA1(6141b1a7863fdfb200ca35d2893979a34dcc3f6c) )
	ROM_LOAD( "sg2-09m", 0x2000000, 0x400000, CRC(33f73d4c) SHA1(954695a43e77b58585409678bd87c76adac1d855) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sg2-02",  0x00000, 0x08000, CRC(415ee138) SHA1(626083c8705f012552691c450f95401ddc88065b) )
	ROM_CONTINUE(        0x10000, 0x18000 )
	ROM_LOAD( "sg2-03",  0x28000, 0x20000, CRC(43806735) SHA1(88d389bcc79cbd4fa1f4b62008e171a897e77652) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "sg2-01m", 0x0000000, 0x400000, CRC(643ea27b) SHA1(40747432d5cfebac54d3824b6a6f26b5e7742fc1) )
ROM_END

ROM_START( strider2 )
	CPZN2_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER2, 0 )
	ROM_LOAD( "hr2_04u.bin",  0x000000, 0x080000, CRC(6e18a49d) SHA1(e2dbc22d8e37f0e6b05d40bf2e450d0683b10dc0) )

	ROM_REGION32_LE( 0x2c00000, REGION_USER1, 0 )
	ROM_LOAD( "hr2-05m.bin", 0x0000000, 0x800000, CRC(18716fe8) SHA1(bb923f18120086054cd6fd91f77d27a190c1eed4) )
	ROM_LOAD( "hr2-06m.bin", 0x0800000, 0x800000, CRC(6f13b69c) SHA1(9a14ecc72631bc44053af71fe7e3934bedf1a71e) )
	ROM_LOAD( "hr2-07m.bin", 0x1000000, 0x800000, CRC(3925701b) SHA1(d93218d2b97cc0fc6c30221bd6b5e955520fbc46) )
	ROM_LOAD( "hr2-08m.bin", 0x1800000, 0x800000, CRC(d844c0dc) SHA1(6010cfbf4dc42fda182884d78e12dcb63df00249) )
	ROM_LOAD( "hr2-09m.bin", 0x2000000, 0x800000, CRC(cdd43e6b) SHA1(346a83deadecd56428276acefc2ce95249a49921) )
	ROM_LOAD( "hr2-10m.bin", 0x2800000, 0x400000, CRC(d95b3f37) SHA1(b6566c1184718f6c0986d13060894c0fb400c201) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "hr2-02.bin",  0x00000, 0x08000, CRC(acd8d385) SHA1(5edb61c3d66d2d09a28a71db52eee3a9f7db8c9d) )
	ROM_CONTINUE(            0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "hr2-01m.bin", 0x0000000, 0x200000, CRC(510a16d1) SHA1(05f10c2921a4d3b1fab4d0a4ea06351809bdbb07) )
	ROM_RELOAD( 0x0200000, 0x200000 )
ROM_END

ROM_START( stridr2a )
	CPZN2_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER2, 0 )
	ROM_LOAD( "hr2a04.bin",   0x000000, 0x080000, CRC(56ff9394) SHA1(fe8417965d945210ac098c6678c02f1c678bd13b) )

	ROM_REGION32_LE( 0x2c00000, REGION_USER1, 0 )
	ROM_LOAD( "hr2-05m.bin", 0x0000000, 0x800000, CRC(18716fe8) SHA1(bb923f18120086054cd6fd91f77d27a190c1eed4) )
	ROM_LOAD( "hr2-06m.bin", 0x0800000, 0x800000, CRC(6f13b69c) SHA1(9a14ecc72631bc44053af71fe7e3934bedf1a71e) )
	ROM_LOAD( "hr2-07m.bin", 0x1000000, 0x800000, CRC(3925701b) SHA1(d93218d2b97cc0fc6c30221bd6b5e955520fbc46) )
	ROM_LOAD( "hr2-08m.bin", 0x1800000, 0x800000, CRC(d844c0dc) SHA1(6010cfbf4dc42fda182884d78e12dcb63df00249) )
	ROM_LOAD( "hr2-09m.bin", 0x2000000, 0x800000, CRC(cdd43e6b) SHA1(346a83deadecd56428276acefc2ce95249a49921) )
	ROM_LOAD( "hr2-10m.bin", 0x2800000, 0x400000, CRC(d95b3f37) SHA1(b6566c1184718f6c0986d13060894c0fb400c201) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "hr2-02.bin",  0x00000, 0x08000, CRC(acd8d385) SHA1(5edb61c3d66d2d09a28a71db52eee3a9f7db8c9d) )
	ROM_CONTINUE(            0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "hr2-01m.bin", 0x0000000, 0x200000, CRC(510a16d1) SHA1(05f10c2921a4d3b1fab4d0a4ea06351809bdbb07) )
	ROM_RELOAD( 0x0200000, 0x200000 )
ROM_END

ROM_START( shiryu2 )
	CPZN2_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER2, 0 )
	ROM_LOAD( "hr2j-04.bin", 0x0000000, 0x080000, CRC(0824ee5f) SHA1(a296ffe03f0d947deb9803d05de3c240a26b52bb) )

	ROM_REGION32_LE( 0x2c00000, REGION_USER1, 0 )
	ROM_LOAD( "hr2-05m.bin", 0x0000000, 0x800000, CRC(18716fe8) SHA1(bb923f18120086054cd6fd91f77d27a190c1eed4) )
	ROM_LOAD( "hr2-06m.bin", 0x0800000, 0x800000, CRC(6f13b69c) SHA1(9a14ecc72631bc44053af71fe7e3934bedf1a71e) )
	ROM_LOAD( "hr2-07m.bin", 0x1000000, 0x800000, CRC(3925701b) SHA1(d93218d2b97cc0fc6c30221bd6b5e955520fbc46) )
	ROM_LOAD( "hr2-08m.bin", 0x1800000, 0x800000, CRC(d844c0dc) SHA1(6010cfbf4dc42fda182884d78e12dcb63df00249) )
	ROM_LOAD( "hr2-09m.bin", 0x2000000, 0x800000, CRC(cdd43e6b) SHA1(346a83deadecd56428276acefc2ce95249a49921) )
	ROM_LOAD( "hr2-10m.bin", 0x2800000, 0x400000, CRC(d95b3f37) SHA1(b6566c1184718f6c0986d13060894c0fb400c201) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "hr2-02.bin",  0x00000, 0x08000, CRC(acd8d385) SHA1(5edb61c3d66d2d09a28a71db52eee3a9f7db8c9d) )
	ROM_CONTINUE(            0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "hr2-01m.bin", 0x0000000, 0x200000, CRC(510a16d1) SHA1(05f10c2921a4d3b1fab4d0a4ea06351809bdbb07) )
	ROM_RELOAD( 0x0200000, 0x200000 )
ROM_END


/* Tecmo */

#define TPS_BIOS \
	ROM_REGION( 0x480400, REGION_CPU1, 0 ) \
	ROM_LOAD( "coh1002m.bin", 0x0400000, 0x080000, CRC(69ffbcb4) SHA1(03eb2febfab3fcde716defff291babd9392de965) )

ROM_START( tps )
	TPS_BIOS
ROM_END

ROM_START( doapp )
	TPS_BIOS

	ROM_REGION32_LE( 0x01a00000, REGION_USER1, 0 )
	ROM_LOAD16_BYTE( "doapp119.bin", 0x0000001, 0x100000, CRC(bbe04cef) SHA1(f2dae4810ca78075fc3007a6001531a455235a2e) )
	ROM_LOAD16_BYTE( "doapp120.bin", 0x0000000, 0x100000, CRC(b614d7e6) SHA1(373756d9b88b45c677e987ee1e5cb2d5446ecfe8) )
	ROM_LOAD( "doapp-0.216",         0x0200000, 0x400000, CRC(acc6c539) SHA1(a744567a3d75634098b1749103307981be9acbdd) )
	ROM_LOAD( "doapp-1.217",         0x0600000, 0x400000, CRC(14b961c4) SHA1(3fae1fcb4665ba8bad391881b26c2d087718d42f) )
	ROM_LOAD( "doapp-2.218",         0x0a00000, 0x400000, CRC(134f698f) SHA1(6422972cf5d30a0f09f0c20f042691d5969207b4) )
	ROM_LOAD( "doapp-3.219",         0x0e00000, 0x400000, CRC(1c6540f3) SHA1(8631fde93a1da6325d7b31c7edf12c964f0ac4fc) )
	ROM_LOAD( "doapp-4.220",         0x1200000, 0x400000, CRC(f83bacf7) SHA1(5bd66da993f0db966581dde80dd7e5b377754412) )
	ROM_LOAD( "doapp-5.221",         0x1600000, 0x400000, CRC(e11e8b71) SHA1(b1d1b9532b5f074ce216a603436d5674d136865d) )
ROM_END

ROM_START( glpracr2 )
	TPS_BIOS

	ROM_REGION32_LE( 0x02200000, REGION_USER1, 0 )
	ROM_LOAD16_BYTE( "gallop2u.119",        0x0000001, 0x100000, CRC(9899911c) SHA1(f043fb97760c53422ad6aeb214474c0be00017ce) )
	ROM_LOAD16_BYTE( "gallop2u.120",        0x0000000, 0x100000, CRC(fd69bd4b) SHA1(26a183bdc3b2fb3d93bd7694e429a676106f4e58) )
	ROM_LOAD( "gra2-0.217",          0x0200000, 0x400000, CRC(a077ffa3) SHA1(73492ec2145246276bfe25b27d7de4f6393124f4) )
	ROM_LOAD( "gra2-1.218",          0x0600000, 0x400000, CRC(28ce033c) SHA1(4dc53e5c82fde683efd72c66b397d56aa72d52b9) )
	ROM_LOAD( "gra2-2.219",          0x0a00000, 0x400000, CRC(0c9cb7da) SHA1(af23c11e69428413ff4d1c2746adb786de927cb5) )
	ROM_LOAD( "gra2-3.220",          0x0e00000, 0x400000, CRC(264e3a0c) SHA1(c1509b16d7192b9f61dbceb299290239219adefd) )
	ROM_LOAD( "gra2-4.221",          0x1200000, 0x400000, CRC(2b070307) SHA1(43c028aaca297358f87c6633c2020d71e34317b8) )
	ROM_LOAD( "gra2-5.222",          0x1600000, 0x400000, CRC(94a363c1) SHA1(4c53822a672ac99b001c9fe82f9d0f8496989e67) )
	ROM_LOAD( "gra2-6.223",          0x1a00000, 0x400000, CRC(8c6b4c4c) SHA1(0053f736dcd437c01da8cadd820e8af658ce6077) )
	ROM_LOAD( "gra2-7.323",          0x1e00000, 0x400000, CRC(7dfb6c54) SHA1(6e9a9a4172f957ba354ddd82c30735a56c5934b1) )
ROM_END

ROM_START( glprac2j )
	TPS_BIOS

	ROM_REGION32_LE( 0x02200000, REGION_USER1, 0 )
	ROM_LOAD16_BYTE( "1.119",        0x0000001, 0x100000, CRC(0fe2d2df) SHA1(031369f4e1138e2ee293c321e5ee418e560b3f06) )
	ROM_LOAD16_BYTE( "2.120",        0x0000000, 0x100000, CRC(8e3fb1c0) SHA1(2126c1e43bee7cd938e0f2a3ea841da8811223cd) )
	ROM_LOAD( "gra2-0.217",          0x0200000, 0x400000, CRC(a077ffa3) SHA1(73492ec2145246276bfe25b27d7de4f6393124f4) )
	ROM_LOAD( "gra2-1.218",          0x0600000, 0x400000, CRC(28ce033c) SHA1(4dc53e5c82fde683efd72c66b397d56aa72d52b9) )
	ROM_LOAD( "gra2-2.219",          0x0a00000, 0x400000, CRC(0c9cb7da) SHA1(af23c11e69428413ff4d1c2746adb786de927cb5) )
	ROM_LOAD( "gra2-3.220",          0x0e00000, 0x400000, CRC(264e3a0c) SHA1(c1509b16d7192b9f61dbceb299290239219adefd) )
	ROM_LOAD( "gra2-4.221",          0x1200000, 0x400000, CRC(2b070307) SHA1(43c028aaca297358f87c6633c2020d71e34317b8) )
	ROM_LOAD( "gra2-5.222",          0x1600000, 0x400000, CRC(94a363c1) SHA1(4c53822a672ac99b001c9fe82f9d0f8496989e67) )
	ROM_LOAD( "gra2-6.223",          0x1a00000, 0x400000, CRC(8c6b4c4c) SHA1(0053f736dcd437c01da8cadd820e8af658ce6077) )
	ROM_LOAD( "gra2-7.323",          0x1e00000, 0x400000, CRC(7dfb6c54) SHA1(6e9a9a4172f957ba354ddd82c30735a56c5934b1) )
ROM_END

ROM_START( glprac2l )
	TPS_BIOS

	ROM_REGION32_LE( 0x02200000, REGION_USER1, 0 )
	ROM_LOAD16_BYTE( "gra2b.119",    0x0000001, 0x100000, CRC(43abee7c) SHA1(ea0afc820d8480c12c9af54057877ff11a8012fb) )
	ROM_LOAD16_BYTE( "gra2a.120",    0x0000000, 0x100000, CRC(f60096d4) SHA1(5349d780d41a5711b483cd7eb66cd4e496b4fbe4) )
	ROM_LOAD( "gra2-0.217",          0x0200000, 0x400000, CRC(a077ffa3) SHA1(73492ec2145246276bfe25b27d7de4f6393124f4) )
	ROM_LOAD( "gra2-1.218",          0x0600000, 0x400000, CRC(28ce033c) SHA1(4dc53e5c82fde683efd72c66b397d56aa72d52b9) )
	ROM_LOAD( "gra2-2.219",          0x0a00000, 0x400000, CRC(0c9cb7da) SHA1(af23c11e69428413ff4d1c2746adb786de927cb5) )
	ROM_LOAD( "gra2-3.220",          0x0e00000, 0x400000, CRC(264e3a0c) SHA1(c1509b16d7192b9f61dbceb299290239219adefd) )
	ROM_LOAD( "gra2-4.221",          0x1200000, 0x400000, CRC(2b070307) SHA1(43c028aaca297358f87c6633c2020d71e34317b8) )
	ROM_LOAD( "gra2-5.222",          0x1600000, 0x400000, CRC(94a363c1) SHA1(4c53822a672ac99b001c9fe82f9d0f8496989e67) )
	ROM_LOAD( "gra2-6.223",          0x1a00000, 0x400000, CRC(8c6b4c4c) SHA1(0053f736dcd437c01da8cadd820e8af658ce6077) )
	ROM_LOAD( "gra2-7.323",          0x1e00000, 0x400000, CRC(7dfb6c54) SHA1(6e9a9a4172f957ba354ddd82c30735a56c5934b1) )

	ROM_REGION( 0x040000, REGION_CPU2, 0 )
	ROM_LOAD( "link3118.bin", 0x0000000, 0x040000, CRC(a4d4761e) SHA1(3fb25dfa5220d25093588d9501e0666214491100) )
ROM_END

ROM_START( tondemo )
	TPS_BIOS

	ROM_REGION32_LE( 0x01600000, REGION_USER1, 0 )
	ROM_LOAD16_BYTE( "u0119.bin",    0x0000001, 0x100000, CRC(5711e301) SHA1(005375d32c1eda9bd39e46326880a62506d06389) )
	ROM_LOAD16_BYTE( "u0120.bin",    0x0000000, 0x100000, CRC(0b8312c6) SHA1(93e0e4b796cc953daf7ed2ff2f327aed07cf833a) )
	ROM_LOAD( "tca-0.217",           0x0200000, 0x400000, CRC(ef175910) SHA1(b77aa9016804172d433d97d5fdc242a1361e941c) )
	ROM_LOAD( "tca-1.218",           0x0600000, 0x400000, CRC(c3474e8a) SHA1(46dd0ae7cd2e54c639fe39d6965ef71ce6a1b921) )
	ROM_LOAD( "tca-2.219",           0x0a00000, 0x400000, CRC(89b8e1a8) SHA1(70c5f0f2d0a7869e29b62b32fa485f941b683678) )
	ROM_LOAD( "tca-3.220",           0x0e00000, 0x400000, CRC(4fcf8032) SHA1(3ea815548c3bda32b1d4e88454c29e5025431b1c) )
	ROM_LOAD( "tca-4.221",           0x1200000, 0x400000, CRC(c9e23f25) SHA1(145d4e7f0cb67d2552559ce90305a56802a253f9) )
ROM_END


/* video system */

#define KN_BIOS \
	ROM_REGION( 0x480400, REGION_CPU1, 0 ) \
	ROM_LOAD( "coh1002v.bin", 0x0400000, 0x080000, CRC(5ff165f3) SHA1(8f59314c1093446b9bcb06d232244da6df78e206) )

ROM_START( sncwgltd )
	KN_BIOS

	ROM_REGION32_LE( 0x01a80000, REGION_USER1, 0 )
	ROM_LOAD( "ic5.bin",      0x0000000, 0x080000, CRC(458f14aa) SHA1(b4e50be60ffb9b7911561dd35b6a7e0df3432a3a) )
	ROM_LOAD( "ic6.bin",      0x0080000, 0x080000, CRC(8233dd1e) SHA1(1422b4530d671e3b8b471ec16c20ef7c819ab762) )
	ROM_LOAD( "ic7.bin",      0x0100000, 0x080000, CRC(df5ba2f7) SHA1(19153084e7cff632380b67a2fff800644a2fbf7d) )
	ROM_LOAD( "ic8.bin",      0x0180000, 0x080000, CRC(e8145f2b) SHA1(3a1cb189426998856dfeda47267fde64be34c6ec) )
	ROM_LOAD( "ic9.bin",      0x0200000, 0x080000, CRC(605c9370) SHA1(9734549cae3028c089f4c9f2336ee374b3f950f8) )
	ROM_LOAD( "ic11.bin",     0x0280000, 0x400000, CRC(a93f6fee) SHA1(6f079643b50833f8fb497c49945ad23326cc9170) )
	ROM_LOAD( "ic12.bin",     0x0680000, 0x400000, CRC(9f584ef7) SHA1(12c04e198f17d1915f58e83aff45ca2e76773df8) )
	ROM_LOAD( "ic13.bin",     0x0a80000, 0x400000, CRC(652e9c78) SHA1(a929b2944de72606338acb822c1031463e2b1cc5) )
	ROM_LOAD( "ic14.bin",     0x0e80000, 0x400000, CRC(c4ef1424) SHA1(1734a6ee6d0be94d24afefcf2a125b74747f53d0) )
	ROM_LOAD( "ic15.bin",     0x1280000, 0x400000, CRC(2551d816) SHA1(e1500d4bfa8cc55220c366a5852263ac2070da82) )
	ROM_LOAD( "ic16.bin",     0x1680000, 0x400000, CRC(21b401bc) SHA1(89374b80453c474aa1dd3a219422f557f95a262c) )
ROM_END


/* Taito FX1a/FX1b */

#define TAITOFX1_BIOS \
	ROM_REGION( 0x480400, REGION_CPU1, 0 ) \
	ROM_LOAD( "coh1002t.bin", 0x0400000, 0x080000, CRC(e3f23b6e) SHA1(e18907cf8c6ba54d96edba0a9a00487a90219e0d) )

ROM_START( taitofx1 )
	TAITOFX1_BIOS
ROM_END

ROM_START( ftimpcta )
	TAITOFX1_BIOS

	ROM_REGION32_LE( 0x00e00000, REGION_USER1, 0 )
	ROM_LOAD16_BYTE( "e25-14.3",     0x0000001, 0x100000, CRC(0c5f474f) SHA1(ce7031ba860297b99cddd6d0177f07e03520faeb) )
	ROM_LOAD16_BYTE( "e25-13.4",     0x0000000, 0x100000, CRC(7f078d7b) SHA1(df9800dd6885dbc33736c5143d877b0847221061) )
	ROM_LOAD( "e25-01.1",            0x0200000, 0x400000, CRC(8cc4be0c) SHA1(9ca15558a83b7e332e50accf1f7852444a7ce730) )
	ROM_LOAD( "e25-02.2",            0x0600000, 0x400000, CRC(8e8b4c82) SHA1(55c9d4d3a08fc3226a75ab3a674be433af83e289) )
	ROM_LOAD( "e25-03.12",           0x0a00000, 0x400000, CRC(43b1c085) SHA1(6e53550e9be0d2f415fc6b4f3b8a71185c5370b2) )

	ROM_REGION( 0x080000, REGION_CPU2, 0 )
	ROM_LOAD( "e25-10.14",    0x0000000, 0x080000, CRC(2b2ad1b1) SHA1(6d064d0b6805d43ce42929ac8f5645b56384f53c) )

	ROM_REGION( 0x600000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "e25-04.27",    0x0000000, 0x400000, CRC(09a66d35) SHA1(f0df24bc9bfc9eb0f5150dc035c19fc5b8a39bf9) )
	ROM_LOAD( "e25-05.28",    0x0040000, 0x200000, CRC(3fb57636) SHA1(aa38bfac11ecf10fd55143cf4525a2a529be8bb6) )
ROM_END

ROM_START( gdarius )
	TAITOFX1_BIOS

	ROM_REGION32_LE( 0x00e00000, REGION_USER1, 0 )
	ROM_LOAD16_BYTE( "e39-05.3",     0x0000001, 0x100000, CRC(750e5b13) SHA1(68fe9cbd7d506cfd587dccc40b6ae0b0b6ee7c29) )
	ROM_LOAD16_BYTE( "e39-06.4",     0x0000000, 0x100000, CRC(2980c30d) SHA1(597321642125c3ae37581c2d9abc2723c7909996) )
	ROM_LOAD( "e39-01.1",            0x0200000, 0x400000, CRC(bdaaa251) SHA1(a42daa706ee859c2b66be179e08c0ad7990f919e) )
	ROM_LOAD( "e39-02.2",            0x0600000, 0x400000, CRC(a47aab5d) SHA1(64b58e47035ad9d8d6dcaf475cbcc3ad85f4d82f) )
	ROM_LOAD( "e39-03.12",           0x0a00000, 0x400000, CRC(a883b6a5) SHA1(b8d00d944c90f8cd9c2b076688f4c68b2e6d557a) )

	ROM_REGION( 0x080000, REGION_CPU2, 0 )
	ROM_LOAD( "e39-07.14",    0x0000000, 0x080000, CRC(2252c7c1) SHA1(92b9908e0d87cad6587f1acc0eef69eaae8c6a98) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "e39-04.27",    0x0000000, 0x400000, CRC(6ee35e68) SHA1(fdfe63203d8cecf84cb869039fb893d5b63cdd67) )
ROM_END

ROM_START( gdarius2 )
	TAITOFX1_BIOS

	ROM_REGION32_LE( 0x00e00000, REGION_USER1, 0 )
	ROM_LOAD16_BYTE( "e39-11.3",     0x0000001, 0x100000, CRC(766f73df) SHA1(9ce24c153920d259bc7fdef0778083eb6d639be3) )
	ROM_LOAD16_BYTE( "e39-12.4",     0x0000000, 0x100000, CRC(b23266c3) SHA1(80aaddaaf10e40280ade4c7d11f45ddab47ee9a6) )
	ROM_LOAD( "e39-01.1",            0x0200000, 0x400000, CRC(bdaaa251) SHA1(a42daa706ee859c2b66be179e08c0ad7990f919e) )
	ROM_LOAD( "e39-02.2",            0x0600000, 0x400000, CRC(a47aab5d) SHA1(64b58e47035ad9d8d6dcaf475cbcc3ad85f4d82f) )
	ROM_LOAD( "e39-03.12",           0x0a00000, 0x400000, CRC(a883b6a5) SHA1(b8d00d944c90f8cd9c2b076688f4c68b2e6d557a) )

	ROM_REGION( 0x080000, REGION_CPU2, 0 )
	ROM_LOAD( "e39-07.14",    0x0000000, 0x080000, CRC(2252c7c1) SHA1(92b9908e0d87cad6587f1acc0eef69eaae8c6a98) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "e39-04.27",    0x0000000, 0x400000, CRC(6ee35e68) SHA1(fdfe63203d8cecf84cb869039fb893d5b63cdd67) )
ROM_END

ROM_START( mgcldate )
	TAITOFX1_BIOS

	ROM_REGION32_LE( 0x00c00000, REGION_USER1, 0 )
	ROM_LOAD16_BYTE( "e32-08.2",     0x0000000, 0x100000, CRC(3d42cd28) SHA1(9017922e835a359ba5126c8a9e8c27380a5ce081) )
	ROM_LOAD16_BYTE( "e32-09.7",     0x0000001, 0x100000, CRC(db7ec115) SHA1(fa6f18de71ba997389d887d7ffe745aa25e24c20) )
	ROM_LOAD( "e32-01.1",            0x0200000, 0x400000, CRC(cf5f1d01) SHA1(5417f8aef5c8d0e9e63ba8c68efb5b3ef37b4693) )
	ROM_LOAD( "e32-02.6",            0x0600000, 0x400000, CRC(61c8438c) SHA1(bdbe6079cc634c0cd6580f76619eb2944c9a31d9) )
	ROM_LOAD( "e32-03.12",           0x0a00000, 0x200000, CRC(190d1618) SHA1(838a651d32752015baa7e8caea62fd739631b8be) )

	ROM_REGION( 0x2c000, REGION_CPU2, 0 )     /* 64k for Z80 code */
	ROM_LOAD( "e32-10.22",           0x0000000, 0x004000, CRC(adf3feb5) SHA1(bae5bc3fad99a92a3492be1b775dab861007eb3b) )
	ROM_CONTINUE(                    0x0010000, 0x01c000 ) /* banked stuff */

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "e32-04.15",           0x0000000, 0x400000, CRC(c72f9eea) SHA1(7ab8b412a8ed00a42016acb7d13d3b074155780a) )
ROM_END

ROM_START( mgcldtea )
	TAITOFX1_BIOS

	ROM_REGION32_LE( 0x00c00000, REGION_USER1, 0 )
	ROM_LOAD16_BYTE( "e32-05.2",     0x0000000, 0x080000, CRC(72fc7f7b) SHA1(50d9e84bc74fb63ec1900ab149051888bc3d03a5) )
	ROM_LOAD16_BYTE( "e32-06.7",     0x0000001, 0x080000, CRC(d11c3881) SHA1(f7046c5bed4818152edcf697a49664b0bcf12a1b) )
	ROM_LOAD( "e32-01.1",            0x0200000, 0x400000, CRC(cf5f1d01) SHA1(5417f8aef5c8d0e9e63ba8c68efb5b3ef37b4693) )
	ROM_LOAD( "e32-02.6",            0x0600000, 0x400000, CRC(61c8438c) SHA1(bdbe6079cc634c0cd6580f76619eb2944c9a31d9) )
	ROM_LOAD( "e32-03.12",           0x0a00000, 0x200000, CRC(190d1618) SHA1(838a651d32752015baa7e8caea62fd739631b8be) )

	ROM_REGION( 0x2c000, REGION_CPU2, 0 )     /* 64k for Z80 code */
	ROM_LOAD( "e32-10.22",           0x0000000, 0x004000, CRC(adf3feb5) SHA1(bae5bc3fad99a92a3492be1b775dab861007eb3b) )
	ROM_CONTINUE(                    0x0010000, 0x01c000 ) /* banked stuff */

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "e32-04.15",           0x0000000, 0x400000, CRC(c72f9eea) SHA1(7ab8b412a8ed00a42016acb7d13d3b074155780a) )
ROM_END

ROM_START( psyforce )
	TAITOFX1_BIOS

	ROM_REGION32_LE( 0x00900000, REGION_USER1, 0 )
	ROM_LOAD16_BYTE( "e22-05.2",     0x0000000, 0x080000, CRC(7770242c) SHA1(dd37575d3d9ffdef60fe0e4cab6c9e42d087f714) )
	ROM_LOAD16_BYTE( "e22-10.7",     0x0000001, 0x080000, CRC(f6341d63) SHA1(99dc27aa694ae5951148054291912a486726e8c9) )
	ROM_LOAD( "e22-02.16",           0x0100000, 0x200000, CRC(03b50064) SHA1(0259537e86b266b3f34308c4fc0bcc04c037da71) )
	ROM_LOAD( "e22-03.1",            0x0300000, 0x200000, CRC(8372f839) SHA1(646b3919b6be63412c11850ec1524685abececc0) )
	ROM_LOAD( "e22-04.21",           0x0500000, 0x200000, CRC(397b71aa) SHA1(48743c362503c1d2dbeb3c8be4cb2aaaae015b88) )

	ROM_REGION( 0x2c000, REGION_CPU2, 0 )     /* 64k for Z80 code */
	ROM_LOAD( "e22-07.22",           0x0000000, 0x004000, CRC(739af589) SHA1(dbb4d1c6d824a99ccf27168e2c21644e19811523) )
	ROM_CONTINUE(                    0x0010000, 0x01c000 ) /* banked stuff */

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "e22-01.15",           0x000000,  0x200000, CRC(808b8340) SHA1(d8bde850dd9b5b71e94ea707d2d728754f907977) )
ROM_END

ROM_START( raystorm )
	TAITOFX1_BIOS

	ROM_REGION32_LE( 0x00900000, REGION_USER1, 0 )
	ROM_LOAD16_BYTE( "e24-06.3",     0x0000001, 0x080000, CRC(d70cdf46) SHA1(da6163d69d3ea9c1e3f4b7961a548f1f9d8d9909) )
	ROM_LOAD16_BYTE( "e24-05.4",     0x0000000, 0x080000, CRC(40097ab9) SHA1(67e73568b35515c2c5a9119e97ac4709baff8c5a) )
	ROM_LOAD( "e24-02.1",            0x0100000, 0x400000, CRC(9f70950d) SHA1(b3e4f925a61ae2e5dd4cc5d7ec3030a0d5c2c04d) )
	ROM_LOAD( "e24-03.2",            0x0500000, 0x400000, CRC(6c1f0a5d) SHA1(1aac37a7ff23e54021a4cec18c9bb93242337180) )

	ROM_REGION( 0x080000, REGION_CPU2, 0 )
	ROM_LOAD( "e24-09.14",    0x0000000, 0x080000, CRC(808589e1) SHA1(46ada4c6d68c2462186a0b962abb435ee740c0ba) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "e24-04.27",    0x0000000, 0x400000, CRC(f403493a) SHA1(3e49fd2a060a3893e26f14cc3cf47c4ba91e17d4) )
ROM_END

ROM_START( sfchamp )
	TAITOFX1_BIOS

	ROM_REGION32_LE( 0x00900000, REGION_USER1, 0 )
	ROM_LOAD16_BYTE( "e18-07.2",     0x0000000, 0x080000, CRC(1b484e1c) SHA1(f29f40a9988475d8abbb126095b0716133c087a0) )
	ROM_LOAD16_BYTE( "e18-08.7",     0x0000001, 0x080000, CRC(6a5558cd) SHA1(75b26bcaaa213283e7e0dace69ee58f305b4572d) )
	ROM_LOAD( "e18-02.12",           0x0100000, 0x200000, CRC(c7b4fe29) SHA1(7f823bd61abf2b15d3ba62bca829a5b1acacfd09) )
	ROM_LOAD( "e18-03.16",           0x0300000, 0x200000, CRC(76392346) SHA1(2c5b70c4708208f866feea0472fcc72333061124) )
	ROM_LOAD( "e18-04.19",           0x0500000, 0x200000, CRC(fc3731da) SHA1(58948aad8d7bb7a8449d2bf12e9d5e6d7b4426b5) )
	ROM_LOAD( "e18-05.21",           0x0700000, 0x200000, CRC(2e984c50) SHA1(6d8255e38c67d68bf489c9885663ed2edf148188) )

	ROM_REGION( 0x2c000, REGION_CPU2, 0 )     /* 64k for Z80 code */
	ROM_LOAD( "e18-09.22",           0x0000000, 0x004000, CRC(bb5a5319) SHA1(0bb700cafc157d3af663cc9bebb8167487ff2852) )
	ROM_CONTINUE(                    0x0010000, 0x01c000 ) /* banked stuff */

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "e18-01.15",           0x0000000, 0x200000, CRC(dbd1408c) SHA1(ef81064f2f95e5ae25eb1f10d1e78f27f9e294f5) )
ROM_END

/* Eighteen Raizing */

#define PSARC95_BIOS \
	ROM_REGION( 0x480400, REGION_CPU1, 0 ) \
	ROM_LOAD( "coh1002e.bin", 0x0400000, 0x080000, NO_DUMP )

ROM_START( psarc95 )
	PSARC95_BIOS
ROM_END

ROM_START( beastrzr )
	PSARC95_BIOS

	ROM_REGION32_LE( 0x0200000, REGION_USER1, 0 )
	ROM_LOAD( "broar_u0.212", 0x000000, 0x080000, CRC(1c85d7fb) SHA1(aa406a42c424cc16a9e5330c68dda9acf8760088) )
	ROM_LOAD( "broar_u0.213", 0x080000, 0x080000, CRC(2c586534) SHA1(a38dfc3a45446d24a1caac89b0f560989d46ded5) )
	ROM_LOAD( "broar_u0.214", 0x100000, 0x080000, CRC(911e6c90) SHA1(724e4cae49bb124200e188a0288516b3a7d5ab53) )
	ROM_LOAD( "broar_u0.215", 0x180000, 0x080000, CRC(31c8e055) SHA1(2811789ab6221b972d1e3ffe98916587990f7564) )

	ROM_REGION( 0x080000, REGION_CPU2, 0 )
	ROM_LOAD( "broar_u.042",  0x000000, 0x040000, CRC(4d537f88) SHA1(1760367d70a81606e29885ea315185d2c2a9409b) )
	ROM_LOAD( "broar_u.046",  0x040000, 0x040000, CRC(d4bb261a) SHA1(9a295b1354ef15f37ea09bb209cf0cb98437c462) )

	ROM_REGION( 0xc00000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "rabroar1.217", 0x000000, 0x400000, CRC(11f1ba36) SHA1(d41ae686c2c607640cbadf906215c89134758050) )
	ROM_LOAD( "rabroar2.216", 0x400000, 0x400000, CRC(d46d46b7) SHA1(1c42cb5dcda4b26c08c4ecf95efeadaf3a1d1dd2) )
	ROM_LOAD( "rabroar3.snd", 0x800000, 0x400000, CRC(b74cc4d1) SHA1(eb5485582a12959ae06927a2f1d8a7e63e0f956f) )
ROM_END

ROM_START( beastrzb )
	TAITOFX1_BIOS

	ROM_REGION32_LE( 0x0900000, REGION_USER1, 0 )
	ROM_LOAD( "27c160.1",     0x0000000, 0x200000, CRC(820855e2) SHA1(18bdd4d0b4a92ae4fde457e1f37c813be6eece71) )
	ROM_LOAD( "27c160.2",     0x0200000, 0x200000, CRC(1712af34) SHA1(3a78997a2ad0fec1b09828b47150a4be611cd9ad) )
	ROM_LOAD( "4",            0x0400000, 0x200000, CRC(bff21f44) SHA1(2dffc518c069f0692a3b75e10091658d9c10ecb5) )
	ROM_LOAD( "27c160.5",     0x0600000, 0x200000, CRC(10fe6f4d) SHA1(9faee2faa6d741e1caf25edd093644be5723aa5c) )
	ROM_LOAD( "27c800.3",     0x0800000, 0x100000, CRC(7192eb4e) SHA1(bb276a38261099d91080d8613dc7500322f6fcab) )

	ROM_REGION( 0x180000, REGION_CPU2, 0 )
	ROM_LOAD( "27c4096.1",    0x0000000, 0x080000, CRC(217734a1) SHA1(de4f519215123c09b3b5f27509b4d74604b5e03d) )
	ROM_LOAD( "27c4096.2",    0x0080000, 0x080000, CRC(d1f2a9b2) SHA1(d1475a453ce4e3b9f2ff59abedf0f57ba3c408fe) )
	ROM_LOAD( "27c240.3",     0x0100000, 0x080000, CRC(509cdc8b) SHA1(8b92b79be09de56e7d40c2d02fcbeca92bb60226) )
ROM_END

ROM_START( brvblade )
	PSARC95_BIOS

	ROM_REGION32_LE( 0x0c00000, REGION_USER1, 0 )
	/* TSOP56 Flash EEPROMs are not dumped */
	ROM_LOAD( "tsop56.1",     0x0000000, 0x400000, NO_DUMP )
	ROM_LOAD( "tsop56.2",     0x0400000, 0x400000, NO_DUMP )

	ROM_REGION( 0x100000, REGION_CPU2, 0 )
	ROM_LOAD( "spu0u049.bin", 0x0000000, 0x080000, CRC(c9df8ed9) SHA1(00a58522189091c48d781b6703e4378e04343c33) )
	ROM_LOAD( "spu1u412.bin", 0x0080000, 0x080000, CRC(6408b5b2) SHA1(ba60aa1074df87e98fa260211e9ec99cea25023f) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "rom1u028.bin", 0x0000000, 0x200000, CRC(cb7bc657) SHA1(e29ec5f5e18a1022757631caa341c5d6ef7b7033) )
	ROM_LOAD( "rom2u336.bin", 0x0200000, 0x200000, CRC(4709026a) SHA1(f9e66bb267931b0b5860c92cd5277966f7ecc12f) )
ROM_END

/* Atari PSX */

#define AT_BIOS \
	ROM_REGION( 0x480400, REGION_CPU1, 0 ) \
	ROM_LOAD( "coh1000t.bin", 0x0400000, 0x080000, NO_DUMP )

ROM_START( atpsx )
	AT_BIOS
ROM_END

ROM_START( primrag2 )
	AT_BIOS

	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	ROM_LOAD( "pr2_036.u14",  0x000000, 0x080000, CRC(c86450cd) SHA1(19c3c50d839a9efb6ffa9ada8a072f56697c1abb) ) 
	ROM_LOAD( "pr2_036.u15",  0x000000, 0x080000, CRC(4b24bd54) SHA1(7f27cd524d10e5869aab6d4dc6a4217d049c475d) ) 
	ROM_LOAD( "pr2_036.u16",  0x000000, 0x080000, CRC(3ee39e4f) SHA1(dbd859b54fb9be33effc14eb847dcd829024eea3) ) 
	ROM_LOAD( "pr2_036.u17",  0x000000, 0x080000, CRC(3681516c) SHA1(714f73ea4ac190c36a6eb2308616a4aecabc4e69) ) 

	DISK_REGION( REGION_DISKS )
/*	DISK_IMAGE() */
ROM_END

/* Acclaim PSX */

#define AC_BIOS \
	ROM_REGION( 0x480400, REGION_CPU1, 0 ) \
	ROM_LOAD( "coh1000a.bin", 0x0400000, 0x080000, CRC(8d8d0764) SHA1(7ee83d317190bb1cef2f8f01c81eaaae47150ebb) )

ROM_START( acpsx )
	AC_BIOS
ROM_END

ROM_START( nbajamex )
	AC_BIOS

	ROM_REGION32_LE( 0x01a00000, REGION_USER1, 0 )
	ROM_LOAD( "360mpa1e.u35", 0x000000, 0x100000, CRC(d8f5b2f7) SHA1(e38609d314721b8b612e047406e2888395917b0d) ) 
	ROM_LOAD( "360mpa1o.u36", 0x000000, 0x100000, CRC(c433e827) SHA1(1d2a5a6990a1b1864e63ce3ba7306d48ebbd4775) ) 
	ROM_LOAD( "nbax0e.u41",   0x000000, 0x200000, CRC(077f4355) SHA1(63c52bb82943b52bb0906d114acd5ea8643068b6) ) 
	ROM_LOAD( "nbax0o.u28",   0x000000, 0x200000, CRC(be13c5af) SHA1(eee5c9d985384ecfe4f00fae27d66fbefc15b28e) ) 
	ROM_LOAD( "nbax1e.u42",   0x000000, 0x200000, CRC(f1212cf9) SHA1(b2f80af3ec4d559056e86f695d89d1d32b500f50) ) 
	ROM_LOAD( "nbax1o.u29",   0x000000, 0x200000, CRC(3650e85b) SHA1(a36bfa235c8e3bb516e178f54d3c5e3955c7e918) ) 
	ROM_LOAD( "nbax2e.u43",   0x000000, 0x200000, CRC(c20ab628) SHA1(7ffe5005e1913da56770452ae2f907a4a270ab24) ) 
	ROM_LOAD( "nbax2o.u30",   0x000000, 0x200000, CRC(ccbb6125) SHA1(998eda99182b984f88f5fc58095cb35bf232a26b) ) 
	ROM_LOAD( "nbax3e.u44",   0x000000, 0x200000, CRC(07ba00a3) SHA1(c14bffd35ee715b07d6065b454b0443438ab6536) ) 
	ROM_LOAD( "nbax3o.u31",   0x000000, 0x200000, CRC(d5238edf) SHA1(d1eb30ec65dd6cfa8cbb2b36af3a83820d1de99a) ) 
	ROM_LOAD( "nbax4e.u17",   0x000000, 0x200000, CRC(b5977765) SHA1(08acdfe413a5a9182ca117f44b7acac9dac9ecea) ) 
	ROM_LOAD( "nbax4o.u3",    0x000000, 0x200000, CRC(1cf16a34) SHA1(a7e984a2db846854f1c4a9a2fdefd0d17ce3108c) ) 
	ROM_LOAD( "nbax5e.u18",   0x000000, 0x200000, CRC(0eb917da) SHA1(d6c8991ba7cd492668757658ee64078d0e82b596) ) 
	ROM_LOAD( "nbax5o.u4",    0x000000, 0x200000, CRC(5272754b) SHA1(c35ba5377eb812991e4bf0d954a34af90b986341) ) 
	ROM_LOAD( "nbax6e.u19",   0x000000, 0x200000, CRC(6f17d8c1) SHA1(22cf263efb64cf62030e02b641c485debe75944d) ) 
	ROM_LOAD( "nbax6o.u5",    0x000000, 0x200000, CRC(b1dfb42e) SHA1(fb9627e228bf2a744842eb44afbca4a6232cadb2) ) 

	ROM_REGION( 0x01a00000, REGION_CPU2, 0 )
	ROM_LOAD( "sound0.u48",   0x000000, 0x200000, CRC(38873b67) SHA1(b2f8d32270ae604c099a1b9b71d2e06468c7d4a9) ) 
	ROM_LOAD( "sound1.u49",   0x000000, 0x200000, CRC(57014589) SHA1(d360ff1c52424bd91a5a8d1a2a9c10bf7abb0602) ) 

	ROM_REGION( 0x01a00000, REGION_SOUND1, 0 )
	ROM_LOAD( "360snda1.u52", 0x000000, 0x080000, CRC(36d8a628) SHA1(944a01c9128f5e90c7dba3557a3ecb2c5ca90831) ) 
ROM_END

ROM_START( jdredd )
	AC_BIOS

	ROM_REGION32_LE( 0x01a00000, REGION_USER1, 0 )
	ROM_LOAD( "j-dread.u35",  0x000000, 0x020000, CRC(c1e17191) SHA1(82901439b1a51b9aadb4df4b9d944f26697a1460) ) 
	ROM_LOAD( "j-dread.u36",  0x000000, 0x020000, CRC(37addbf9) SHA1(a4061a1ba9e230f080f0bfea69bf77efe9264a92) ) 

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE( "jdreddc", 0, MD5(b66074f3df1e90ec4e3ff09dcdf33033) SHA1(83ed8df25d100b1c060f4dde2f162ba31803db7d) )
ROM_END

ROM_START( jdreddb )
	AC_BIOS

	ROM_REGION32_LE( 0x01a00000, REGION_USER1, 0 )
	ROM_LOAD( "j-dread.u35",  0x000000, 0x020000, CRC(c1e17191) SHA1(82901439b1a51b9aadb4df4b9d944f26697a1460) ) 
	ROM_LOAD( "j-dread.u36",  0x000000, 0x020000, CRC(37addbf9) SHA1(a4061a1ba9e230f080f0bfea69bf77efe9264a92) ) 

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE( "jdreddb", 0, MD5(0da1d048d7223df74fca4f349473cefa) SHA1(9b810e3a16de62cabfc8271b6606574c7034cf41) )
ROM_END

/* Capcom ZN1 */

/* A dummy driver, so that the bios can be debugged, and to serve as */
/* parent for the coh1002c.bin file, so that we do not have to include */
/* it in every zip file */
GAMEX( 1997, cpzn1,    0,        zn,       zn, zn,   ROT0, "Sony/Capcom", "ZN1", NOT_A_DRIVER )

GAMEX( 1995, ts2,      cpzn1,    znqsound, zn, zn,   ROT0, "Capcom/Takara", "Battle Arena Toshinden 2 (USA 951124)", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX( 1995, ts2j,     ts2,      znqsound, zn, zn,   ROT0, "Capcom/Takara", "Battle Arena Toshinden 2 (JAPAN 951124)", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX( 1996, starglad, cpzn1,    znqsound, zn, zn,   ROT0, "Capcom/", "Star Gladiator (USA 960627)", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX( 1996, sfex,     cpzn1,    znqsound, zn, zn,   ROT0, "Capcom/Arika", "Street Fighter EX (ASIA 961219)", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX( 1996, sfexj,    sfex,     znqsound, zn, zn,   ROT0, "Capcom/Arika", "Street Fighter EX (JAPAN 961130)", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX( 1996, glpracr,  cpzn1,    zn,       zn, zn,   ROT0, "Tecmo", "Gallop Racer (JAPAN Ver 9.01.12)", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1997, sfexp,    cpzn1,    znqsound, zn, zn,   ROT0, "Capcom/Arika", "Street Fighter EX Plus (USA 970311)", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX( 1997, sfexpj,   sfexp,    znqsound, zn, zn,   ROT0, "Capcom/Arika", "Street Fighter EX Plus (JAPAN 970311)", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX( 1997, rvschool, cpzn1,    znqsound, zn, zn,   ROT0, "Capcom", "Rival Schools (ASIA 971117)", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX( 1997, jgakuen,  rvschool, znqsound, zn, zn,   ROT0, "Capcom", "Justice Gakuen (JAPAN 971117)", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX( 1998, tgmj,     cpzn1,    znqsound, zn, zn,   ROT0, "Capcom/Akira", "Tetris The Grand Master (JAPAN 980710)", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )

/* Capcom ZN2 */

/* A dummy driver, so that the bios can be debugged, and to serve as */
/* parent for the coh3002c.bin file, so that we do not have to include */
/* it in every zip file */
GAMEX( 1997, cpzn2,    0,        zn,       zn, zn,   ROT0, "Sony/Capcom", "ZN2", NOT_A_DRIVER )

GAMEX( 1998, sfex2,    cpzn2,    znqsound, zn, zn,   ROT0, "Capcom/Arika", "Street Fighter EX 2 (JAPAN 980312)", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX( 1998, sg2j,     cpzn2,    znqsound, zn, zn,   ROT0, "Capcom","Star Gladiator 2 (JAPAN 980316)", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX( 1998, techromn, cpzn2,    znqsound, zn, zn,   ROT0, "Capcom", "Tech Romancer (USA 980914)", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX( 1998, kikaioh,  techromn, znqsound, zn, zn,   ROT0, "Capcom", "Kikaioh (JAPAN 980914)", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX( 1999, sfex2p,   sfex2,    znqsound, zn, zn,   ROT0, "Capcom/Arika", "Street Fighter EX 2 Plus (JAPAN 990611)", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX( 1999, strider2, cpzn2,    znqsound, zn, zn,   ROT0, "Capcom", "Strider 2 (USA 991213)", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX( 1999, stridr2a, strider2, znqsound, zn, zn,   ROT0, "Capcom", "Strider 2 (ASIA 991213)", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX( 1999, shiryu2,  strider2, znqsound, zn, zn,   ROT0, "Capcom", "Strider Hiryu 2 (JAPAN 991213)", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )

/* Atari */

/* A dummy driver, so that the bios can be debugged, and to serve as */
/* parent for the coh1000t.bin file, so that we do not have to include */
/* it in every zip file */

GAMEX( 1996, atpsx,    0,        zn,       zn, zn,   ROT0, "Atari", "Atari PSX", NOT_A_DRIVER )

GAMEX( 1996, primrag2, 0,        zn,       zn, zn,   ROT0, "Atari", "Primal Rage 2", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND | GAME_NOT_WORKING )

/* Acclaim */

/* A dummy driver, so that the bios can be debugged, and to serve as */
/* parent for the coh1000a.bin file, so that we do not have to include */
/* it in every zip file */

GAMEX( 1996, acpsx,    0,        zn,       zn, zn,   ROT0, "Acclaim", "Acclaim PSX", NOT_A_DRIVER )

GAMEX( 1996, nbajamex, acpsx,    zn,       zn, zn,   ROT0, "Acclaim", "NBA Jam Extreme", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1996, jdredd,   acpsx,    zn,       zn, zn,   ROT0, "Acclaim", "Judge Dredd (Rev C)", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1996, jdreddb,  jdredd,   zn,       zn, zn,   ROT0, "Acclaim", "Judge Dredd (Rev B)", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND | GAME_NOT_WORKING )

/* Tecmo */

/* A dummy driver, so that the bios can be debugged, and to serve as */
/* parent for the coh1002m.bin file, so that we do not have to include */
/* it in every zip file */
GAMEX( 1997, tps,      0,        zn,       zn, zn,   ROT0, "Sony/Tecmo", "TPS", NOT_A_DRIVER )

GAMEX( 1997, glpracr2, tps,      zn,       zn, zn,   ROT0, "Tecmo", "Gallop Racer 2 (USA)", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1997, glprac2j, glpracr2, zn,       zn, zn,   ROT0, "Tecmo", "Gallop Racer 2 (JAPAN)", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1997, glprac2l, glpracr2, znlink,   zn, zn,   ROT0, "Tecmo", "Gallop Racer 2 Link HW (JAPAN)", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1998, doapp,    tps,      zn,       zn, zn,   ROT0, "Tecmo", "Dead Or Alive ++ (JAPAN)", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1999, tondemo,  tps,      zn,       zn, zn,   ROT0, "Tecmo", "Tondemo Crisis (JAPAN)", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND | GAME_NOT_WORKING )

/* Video System */

/* only one game dumped on this system, so coh1002v.bin is included in the game zip file */

GAMEX( 1996, sncwgltd, 0,        zn,       zn, zn,   ROT0, "Video System", "Sonic Wings Limited (JAPAN)", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND | GAME_NOT_WORKING )

/* Taito */

/* A dummy driver, so that the bios can be debugged, and to serve as */
/* parent for the coh1002t.bin file, so that we do not have to include */
/* it in every zip file */
GAMEX( 1995, taitofx1, 0,        fx1b,     zn, zn,   ROT0, "Sony/Taito", "Taito FX1", NOT_A_DRIVER )

GAMEX( 1995, psyforce, taitofx1, fx1a,     zn, zn,   ROT0, "Taito", "Psychic Force (JAPAN)", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX( 1995, sfchamp,  taitofx1, fx1a,     zn, zn,   ROT0, "Taito", "Super Football Champ (JAPAN)", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX( 1996, raystorm, taitofx1, fx1b,     zn, fx1b, ROT0, "Taito", "Ray Storm (JAPAN)", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1996, ftimpcta, taitofx1, fx1b,     zn, fx1b, ROT0, "Taito", "Fighter's Impact Ace (JAPAN)", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1996, mgcldate, taitofx1, fx1a,     zn, zn,   ROT0, "Taito", "Magical Date (JAPAN) set 1", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX( 1996, mgcldtea, mgcldate, fx1a,     zn, fx1b, ROT0, "Taito", "Magical Date (JAPAN) set 2", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX( 1997, gdarius,  taitofx1, fx1b,     zn, fx1b, ROT0, "Taito", "G-Darius (JAPAN)", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1997, gdarius2, gdarius,  fx1b,     zn, fx1b, ROT0, "Taito", "G-Darius Ver.2 (JAPAN)", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND | GAME_NOT_WORKING )

/* Eighting/Raizing */

/* A dummy driver, so that the bios can be debugged, and to serve as */
/* parent for the coh1002e.bin file, so that we do not have to include */
/* it in every zip file */

GAMEX( 1997, psarc95,  0,        zn,       zn, zn,   ROT0, "Sony/Eighting/Raizing", "PS Arcade 95", NOT_A_DRIVER )

GAMEX( 1997, beastrzr, psarc95,  zn,       zn, zn,   ROT0, "Eighting/Raizing", "Beastorizer", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1997, beastrzb, taitofx1, zn,       zn, zn,   ROT0, "Eighting/Raizing", "Beastorizer (bootleg)", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 2000, brvblade, psarc95,  zn,       zn, zn,   ROT0, "Eighting/Raizing", "Brave Blade (JAPAN)", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND | GAME_NOT_WORKING )
