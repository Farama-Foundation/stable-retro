/***************************************************************************
Namco NA-1 / NA-2 System

NA-1 Games:
-	Bakuretsu Quiz Ma-Q Dai Bouken
-	F/A
-	Super World Court (C354, C357)
-	Nettou! Gekitou! Quiztou!! (C354, C365 - both are 32pin)
-	Exvania (C350, C354)
-	Cosmo Gang the Puzzle (C356)
-	Tinkle Pit (C354, C367)
-	Emeraldia (C354, C358)

NA-2 Games:
-	Knuckle Heads
-	Numan Athletics
-	X-Day 2

To Do:

- Pressing "F3" (reset) crashes MAME

- Emeralda:
	After selecting the game type, tilemap scrolling is briefly incorrect

- Emeralda:
	Shadow sprites, if enabled, make the score display invisible

- Hook up ROZ registers

- Is view area controlled by registers?

- Xday 2:
	has some graphics glitches (wrong sprite tiles); probably blitter-related
	input ports are wonky; can add "coins" while in free-play mode, but can't start game

	Rom board  M112
	Rom board custom Key chip i.d. C394
	Game uses a small cash-register type printer (connects to rom board)
	Game also has a large L.E.D. type score board with several
    displays for various scores. (connects to rom board)
	Game uses coin-type battery on rom board. (not suicide)
	Game won't startup unless printer is connected and with paper.

The board has a 28c16 EEPROM

No other CPUs on the board, but there are many custom chips.
Mother Board:
210 (28pin SOP)
C70 (80pin PQFP)
215 (176pin PQFP)
C218 (208pin PQFP)
219 (160pin PQFP)
Plus 1 or 2 custom chips on ROM board.

Notes:

-	NA-2 is backwards compatible with NA-1.

-	NA-2 games use a different MCU BIOS

-	Test mode for NA2 games includes an additional item: UART Test.
	No games are known to actually link up and use the UART feature.
	It's been confirmed that a Numan Athletics fails the UART test,
	behaving as it does in MAME.

-	Quiz games use 1p button 1 to pick test, 2p button 1 to begin test,
	and 2p button 2 to exit. Because quiz games doesn't have joystick.

-	Almost all quiz games using JAMMA edge connector assign
	button1 to up, button 2 to down, button 3 to left, button 4 to right.
	But Taito F2 quiz games assign button 3 to right and button 4 to left.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "namcona1.h"
#include "sound/namcona.h"
#include "machine/random.h"

static data16_t *mpBank0, *mpBank1;
static data8_t mCoinCount[4];
static data8_t mCoinState;
static data16_t *mcu_ram;
static int mEnableInterrupts;

/*************************************************************************/

static const UINT8 ExvaniaDefaultNvMem[] =
{
/* This data oughtn't be necessary; when Exbania's EPROM area is uninitialized,
 * the game software automatically writes these values there, but then jumps
 * to an unmapped (bogus) address, causing MAME to crash.
 */
 	0x30,0x32,0x4f,0x63,0x74,0x39,0x32,0x52,0x45,0x56,0x49,0x53,0x49,0x4f,0x4e,0x35,
	0x00,0x01,0x00,0x01,0x00,0x01,0x00,0x01,0x00,0x01,0x00,0x01,0x00,0x00,0x00,0x01,
	0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x51,0x01,0x38,0x38,0xa7,0xbf,0xf1,0x04,0x0d,0x15,0x9b,0x80,0x1f,0x83,0xd5,0xa4,
	0x69,0x88,0x7c,0x9f,0xb6,0x01,0xda,0x93,0x17,0x45,0x8b,0x12,0xb2,0x02,0x33,0x5c,
	0x50,0xd6,0xe1,0x56,0xa4,0xad,0x42,0x4a,0x5c,0xdd,0x86,0x61,0xe9,0x03,0x12,0xe1,
	0x0f,0x9b,0xea,0x26,0x2c,0x61,0xdc,0x62,0x48,0x6b,0x6d,0x14,0xe0,0x03,0x85,0x4a,
	0x72,0x46,0xda,0x96,0xc8,0x7d,0x1c,0xd1,0x05,0x3e,0xe5,0x92,0x70,0x43,0x5f,0x6c,
	0x03,0x05,0xb3,0xeb,0xb3,0x20,0x35,0x4d,0x7e,0x66,0x50,0x01,0x36,0xc0,0x33,0xe1,
	0x0f,0xc9,0x38,0x2e,0xe9,0x29,0x19,0x4f,0x5e,0xb1,0xd1,0x49,0x8b,0x3b,0x53,0xfd,
	0x9f,0x3f,0xee,0x25,0x25,0x35,0x7b,0x0d,0x11,0xaf,0x4c,0x11,0x8c,0x32,0xd4,0xda,
	0x7f,0xd8,0x16,0x57,0xe1,0xa6,0xce,0x7d,0xc1,0xae,0x62,0xbf,0x13,0xe4,0x87,0x4c,
	0x3a,0xc1,0xb3,0x0c,0x59,0x99,0x47,0x58,0x5a,0xbd,0x78,0x7c,0xba,0x50,0x01,0xed,
	0x1b,0xea,0x8a,0x49,0x88,0xee,0xd6,0x14,0x85,0xab,0xb0,0x2c,0xde,0x35,0x93,0x11,
	0x2d,0x01,0x1c,0xd7,0x28,0x43,0x30,0xe7,0xb0,0x08,0xed,0x79
}; /* ExvaniaDefaultNvMem */

static const UINT8 QuiztouDefaultNvMem[] =
{
/* This data oughtn't be necessary; when QuiztouDefaultNvMem's EPROM area is uninitialized,
 * the game software automatically writes these values there, but then jumps
 * to an unmapped (bogus) address, causing MAME to crash.
 */
	0x30,0x32,0x4F,0x63,0x74,0x39,0x32,0x52,0x45,0x56,0x49,0x53,0x49,0x4F,0x4E,0x35,
	0x00,0x01,0x00,0x01,0x00,0x01,0x00,0x01,0x00,0x01,0x00,0x01,0x00,0x00,0x00,0x01,
	0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0xC1,0x9B,0xE1,0xC0,0x7E,0xE9,0xA8,0x9A,0xA7,0x86,0xC2,0xB5,0x54,0xBF,0x9A,0xE7,
	0xD9,0x23,0xD1,0x55,0x90,0x38,0x28,0xD1,0xD9,0x6C,0xA1,0x66,0x5E,0x4E,0xE1,0x30,
	0x9C,0xFE,0xD9,0x71,0x9F,0xE2,0xA5,0xE2,0x0C,0x9B,0xB4,0x47,0x65,0x38,0x2A,0x46,
	0x89,0xA9,0x82,0x79,0x7A,0x76,0x78,0xC2,0x63,0xB1,0x26,0xDF,0xDA,0x29,0x6D,0x3E,
	0x62,0xE0,0x96,0x12,0x34,0xBF,0x39,0xA6,0x3F,0x89,0x5E,0xF1,0x6D,0x0E,0xE3,0x6C,
	0x28,0xA1,0x1E,0x20,0x1D,0xCB,0xC2,0x03,0x3F,0x41,0x07,0x84,0x0F,0x14,0x05,0x65,
	0x1B,0x28,0x61,0xC9,0xC5,0xE7,0x2C,0x8E,0x46,0x36,0x08,0xDC,0xF3,0xA8,0x8D,0xFE,
	0xBE,0xF2,0xEB,0x71,0xFF,0xA0,0xD0,0x3B,0x75,0x06,0x8C,0x7E,0x87,0x78,0x73,0x4D,
	0xD0,0xBE,0x82,0xBE,0xDB,0xC2,0x46,0x41,0x2B,0x8C,0xFA,0x30,0x7F,0x70,0xF0,0xA7,
	0x54,0x86,0x32,0x95,0xAA,0x5B,0x68,0x13,0x0B,0xE6,0xFC,0xF5,0xCA,0xBE,0x7D,0x9F,
	0x89,0x8A,0x41,0x1B,0xFD,0xB8,0x4F,0x68,0xF6,0x72,0x7B,0x14,0x99,0xCD,0xD3,0x0D,
	0xF0,0x44,0x3A,0xB4,0xA6,0x66,0x53,0x33,0x0B,0xCB,0xA1,0x10,0x5E,0x4C,0xEC,0x03,
	0x4C,0x73,0xE6,0x05,0xB4,0x31,0x0E,0xAA,0xAD,0xCF,0xD5,0xB0,0xCA,0x27,0xFF,0xD8,
	0x9D,0x14,0x4D,0xF4,0x79,0x27,0x59,0x42,0x7C,0x9C,0xC1,0xF8,0xCD,0x8C,0x87,0x20,
	0x23,0x64,0xB8,0xA6
}; /* QuiztouDefaultNvMem */

static data8_t namcona1_nvmem[NA1_NVRAM_SIZE];

static NVRAM_HANDLER( namcosna1 )
{
	if( read_or_write )
	{
		mame_fwrite( file, namcona1_nvmem, NA1_NVRAM_SIZE );
	}
	else
	{
		if (file)
		{
			mame_fread( file, namcona1_nvmem, NA1_NVRAM_SIZE );
		}
		else
		{
			memset( namcona1_nvmem, 0x00, NA1_NVRAM_SIZE );

			switch( namcona1_gametype )
			{
			case NAMCO_EXBANIA:
				memcpy( namcona1_nvmem, ExvaniaDefaultNvMem, sizeof(ExvaniaDefaultNvMem) );
				break;

			case NAMCO_QUIZTOU:
				memcpy( namcona1_nvmem, QuiztouDefaultNvMem, sizeof(QuiztouDefaultNvMem) );
				break;
			}
		}
	}
} /* namcosna1_nvram_handler */

static READ16_HANDLER( namcona1_nvram_r )
{
	return namcona1_nvmem[offset];
} /* namcona1_nvram_r */

static WRITE16_HANDLER( namcona1_nvram_w )
{
	if( ACCESSING_LSB )
	{
		namcona1_nvmem[offset] = data&0xff;
	}
} /* namcona1_nvram_w */

/***************************************************************************/

INPUT_PORTS_START( namcona1_joy )
	PORT_START
	PORT_DIPNAME( 0x01, 0x00, "DIP2 (Freeze)" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "DIP1 (Test)" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "Test" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "SERVICE" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SERVICE )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER3 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START3 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER4 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START4 )

	PORT_START
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN4 )
INPUT_PORTS_END

INPUT_PORTS_START( namcona1_quiz )
	PORT_START
	PORT_DIPNAME( 0x01, 0x00, "DIP2 (Freeze)" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "DIP1 (Test)" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "Test" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "SERVICE" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SERVICE )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON4 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON4 | IPF_PLAYER3 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER3 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START3 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON4 | IPF_PLAYER4 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER4 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START4 )

	PORT_START
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN4 )
INPUT_PORTS_END

INPUT_PORTS_START( xday2 )
	PORT_START
	PORT_DIPNAME( 0x01, 0x00, "DIP2 (Freeze)" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "DIP1 (Test)" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "Test" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "SERVICE" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SERVICE )

	PORT_START
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON3 ) /* next */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 ) /* prev */
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x8f, IP_ACTIVE_HIGH, IPT_UNUSED )
	/* 0x2000 enter was pressed (in game settings screen)*/
	/* 0x1000 exit was pressed (in game settings screen)*/

	PORT_START
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER2 ) /* enter */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 ) /* exit */
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x8f, IP_ACTIVE_HIGH, IPT_UNUSED )
	/* 0x2000 next was pressed (in game settings screen)*/
	/* 0x1000 prev was pressed (in game settings screen)*/

	PORT_START
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER3 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x8f, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER4 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x8f, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN4 )
INPUT_PORTS_END

/***************************************************************************/

static void
simulate_mcu( void )
{
	int i;
	data16_t data;
	data8_t poll_coins;

	mcu_ram[0xf60/2] = 0x0000; /* mcu ready */

	mcu_ram[0xfc0/2] = readinputport(0x0); /* dipswitch */

	for( i=1; i<=4; i++ )
	{
		data = readinputport(i)<<8;
		switch( namcona1_gametype )
		{
		case NAMCO_KNCKHEAD:
		case NAMCO_BKRTMAQ:
		case NAMCO_QUIZTOU:
		case NAMCO_EXBANIA:
			data |= data>>8;
			break;

		case NAMCO_XDAY2:
			data |= data>>8; /* wrong! */
			break;

		case NAMCO_TINKLPIT:
			if( data&0x2000 ) data |= 0x20; /* throw */
			if( data&0x4000 ) data |= 0x10; /* jump */
			if( i==1 )
			{
				if( readinputport(1)&0x80 ) data |= 0x80; /* P1 start */
				if( readinputport(2)&0x80 ) data |= 0x40; /* P2 start */
			}
			break;

		default:
			break;
		}
		mcu_ram[0xfc0/2+i] = data;
	}

	/* "analog" and "encoder" ports are polled during test mode,
	 * but I haven't found any games that make use of them.
	 */
	mcu_ram[0xfc0/2+0x05] = 0xffff; /* analog0,1 */
	mcu_ram[0xfc0/2+0x06] = 0xffff; /* analog2,3 */
	mcu_ram[0xfc0/2+0x07] = 0xffff; /* analog4,5 */
	mcu_ram[0xfc0/2+0x08] = 0xffff; /* analog6,7 */
	mcu_ram[0xfc0/2+0x09] = 0xffff; /* encoder0,1 */

	poll_coins = readinputport(5); /* coin input */
	if( (poll_coins&0x8)&~(mCoinState&0x8) ) mCoinCount[0]++;
	if( (poll_coins&0x4)&~(mCoinState&0x4) ) mCoinCount[1]++;
	if( (poll_coins&0x2)&~(mCoinState&0x2) ) mCoinCount[2]++;
	if( (poll_coins&0x1)&~(mCoinState&0x1) ) mCoinCount[3]++;
	mCoinState = poll_coins;

	mcu_ram[0xfc0/2+0xa] = (mCoinCount[0]<<8)|mCoinCount[1];
	mcu_ram[0xfc0/2+0xb] = (mCoinCount[2]<<8)|mCoinCount[3];

	/* special handling for F/A */
	data = ~((readinputport(1)<<8)|readinputport(2));
	mcu_ram[0xffc/2] = data;
	mcu_ram[0xffe/2] = data;

	if( namcona1_gametype == NAMCO_XDAY2 )
	{
		int p1 = readinputport(1);
		int p2 = readinputport(2);
		data32_t code = 0;
		if( p2&0x40 ) code |= 0x2000; /* enter (top-level of self-test)*/
		if( p2&0x20 ) code |= 0x1000; /* exit  (top-level of self-test)*/
		if( p1&0x40 ) code |= 0x0020; /* next  (top-level of self-test)*/
		if( p1&0x20 ) code |= 0x0010; /* prev  (top-level of self-test)*/
		code = ~code;
		mcu_ram[0xffc/2] = code>>16;
		mcu_ram[0xffe/2] = code&0xffff;
	}
} /* simulate_mcu */

static READ16_HANDLER( namcona1_mcu_r )
{
	return mcu_ram[offset];
}

static WRITE16_HANDLER( namcona1_mcu_w )
{
	COMBINE_DATA( &mcu_ram[offset] );
	if( offset>=0x400/2 && (offset<0x820/2 || (offset>=0xf30/2 && offset<0xf72/2)) )
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE  "0x%03x: 0x%04x\n", offset*2, mcu_ram[offset] );
	}
	/*
		400..53d  code for MCU?

		820:					song select
		822:					song control (volume? tempo?)
		824,826,828,....89e:	sample select
			0x40 is written to odd addresses to signal the MCU that a sound command has been issued

		8f0: 0x07 unknown
		8f2: 0x01 unknown
		8f4: 0xa4 unknown

		f30..f71	data for MCU
		f72: MCU command:
			0x07 = identify version
			0x03 = process data
			0x87 = ?

		fc0..fc9: used by knuckleheads (NA2-specific?)

		fd8: ?

		fbf: watchdog
	*/
}

/* NA2 hardware sends a special command to the MCU, then tests to
 * see if the proper BIOS version string appears in shared memory.
 */
static void write_version_info( void )
{
	const data16_t source[0x8] =
	{ /* "NSA-BIOS ver"... */
		0x534e,0x2d41,0x4942,0x534f,0x7620,0x7265,0x2e31,0x3133
	};
	int i;
	for( i=0; i<8; i++ )
	{
		namcona1_workram[i] = source[i];
	}
} /* write_version_info */

static WRITE16_HANDLER( mcu_command_w )
{
	data16_t *pMem = (data16_t *)memory_region( REGION_CPU1 );
	data16_t cmd = pMem[0xf72/2]>>8;

	switch( cmd ){
	case 0x03:
		/* Process data at 0xf30..0xf71
		 *
		 * f30: 0101 0020 0400 013e 8a00 0000 0000 011e
		 * f40: 0301 0000 0000 0000 8a16 0000 0000 012c
		 * f50: 0301 0000 0000 0000 8a61 0000 0000 019e
		 * f60: 0301 0000 0000 0000 8a61 0000 0000 01ae
		 * f70: 0000
		 *
		 * f30: 0301 0000 0000 0000 8a88 0000 0000 0120
		 * f40: 0301 0000 0000 0000 8ad1 0000 0000 015a
		 * f50: 0301 0000 0000 0000 8af6 0000 0000 011c
		 * f60: 0301 0000 0000 0000 8b08 0000 0000 0114
		 * f70: 0000
		 *
		 * f30: 0300 0000 0000 0000 8b1f 2004 0000 0000
		 * f40: 0301 0000 0000 0000 8b33 0000 0000 8902
		 * f50: 0000
		 */
		break;

	case 0x07:
		/* This command is used to detect Namco NA-2 hardware; without it,
		 * NA-2 games (Knuckleheads, Numan Athletics) refuse to run.
		 */
		write_version_info();
		break;
	}
} /* mcu_command_w */

/***************************************************************************/
/* sound
 *
 * 8 bit signed PCM data
 * copied to workram
 *
 *	0x01fffc: pointer
 *	0x020000: samples
 *	0x040000: samples
 *	0x060000: samples
 *
 *	0x070000: metadata; 10 byte frames
 */

/***************************************************************************/

/**
 * "Custom Key" Emulation
 *
 * The primary purpose of the custom key chip is protection.  It prevents
 * ROM swaps from working with games that otherwise run on the same hardware.
 *
 * The secondary purpose of the custom key chip is to act as a random number
 * generator in some games.
 */
static READ16_HANDLER( custom_key_r )
{
	static unsigned char keyseq;
	static data16_t count;
	int old_count;

	old_count = count;
	do
	{
		count = mame_rand();
	} while( old_count == count );

	switch( namcona1_gametype )
	{
	case NAMCO_BKRTMAQ:
		if( offset==2 ) return 0x015c;
		break;

	case NAMCO_FA:
		if( offset==2 ) return 0x015d;
		if( offset==4 ) return count;
		break;

	case NAMCO_EXBANIA:
		if( offset==2 ) return 0x015e;
		break;

	case NAMCO_CGANGPZL:
		if( offset==1 ) return 0x0164;
		if( offset==2 ) return count;
		break;

	case NAMCO_SWCOURT:
		if( offset==1 ) return 0x0165;
		if( offset==2 ) return count;
		break;

	case NAMCO_EMERALDA:
		if( offset==1 ) return 0x0166;
		if( offset==2 ) return count;
		break;

	case NAMCO_NUMANATH:
		if( offset==1 ) return 0x0167;
		if( offset==2 ) return count;
		break;

	case NAMCO_KNCKHEAD:
		if( offset==1 ) return 0x0168;
		if( offset==2 ) return count;
		break;

	case NAMCO_QUIZTOU:
		if( offset==2 ) return 0x016d;
		break;

	case NAMCO_TINKLPIT:
		if( offset==7 ) return 0x016f;
		if( offset==4 ) keyseq = 0;
		if( offset==3 )
		{
			const data16_t data[] =
			{
				0x0000,0x2000,0x2100,0x2104,0x0106,0x0007,0x4003,0x6021,
				0x61a0,0x31a4,0x9186,0x9047,0xc443,0x6471,0x6db0,0x39bc,
				0x9b8e,0x924f,0xc643,0x6471,0x6db0,0x19bc,0xba8e,0xb34b,
				0xe745,0x4576,0x0cb7,0x789b,0xdb29,0xc2ec,0x16e2,0xb491
			};
			return data[(keyseq++)&0x1f];
		}
		break;

	case NAMCO_XDAY2:
		if( offset==2 ) return 0x018a;
		if( offset==3 ) return count;
		break;

	default:
		return 0;
	}
	return mame_rand()&0xffff;
} /* custom_key_r */

static WRITE16_HANDLER( custom_key_w )
{
} /* custom_key_w */

/***************************************************************/

static READ16_HANDLER( namcona1_vreg_r )
{
	return namcona1_vreg[offset];
} /* namcona1_vreg_r */

static int
transfer_dword( UINT32 dest, UINT32 source )
{
	data16_t data;

	if( source>=0x400000 && source<0xc00000 )
	{
		data = mpBank1[(source-0x400000)/2];
	}
	else if( source>=0xc00000 && source<0xe00000 )
	{
		data = mpBank0[(source-0xc00000)/2];
	}
	else if( source<0x80000 && source>=0x1000 )
	{
		data = namcona1_workram[(source-0x001000)/2];
	}
	else
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE  "bad blt src %08x\n", source );
		return -1;
	}
	if( dest>=0xf00000 && dest<=0xf02000 )
	{
		namcona1_paletteram_w( (dest-0xf00000)/2, data, 0x0000 );
	}
	else if( dest>=0xf40000 && dest<=0xf80000 )
	{
		namcona1_gfxram_w( (dest-0xf40000)/2, data, 0x0000 );
	}
	else if( dest>=0xff0000 && dest<0xff8000 )
	{
		namcona1_videoram_w( (dest-0xff0000)/2, data, 0x0000 );
	}
	else if( dest>=0xff8000 && dest<=0xffdfff )
	{
		namcona1_sparevram[(dest-0xff8000)/2] = data;
	}
	else if( dest>=0xfff000 && dest<=0xffffff )
	{
		spriteram16[(dest-0xfff000)/2] = data;
	}
	else
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE  "bad blt dst %08x\n", dest );
		return -1;
	}
	return 0;
} /* transfer_dword */

static void
blit_setup( int format, int *bytes_per_row, int *pitch, int mode )
{
	if( mode == 3 )
	{
		switch( format )
		{
		case 0x0001:
			*bytes_per_row = 0x1000;
			*pitch = 0x1000;
			break;

		case 0x0081:
			*bytes_per_row = 4*8;
			*pitch = 36*8;
			break;

		default:
/*		case 0x00f1:*/
/*		case 0x00f9:*/
/*		case 0x00fd:*/
			*bytes_per_row = (64 - (format>>2))*0x08;
			*pitch = 0x200;
			break;
		}
	}
	else
	{
		switch( format )
		{
		case 0x00bd: /* Numan Athletics */
			*bytes_per_row = 4;
			*pitch = 0x120;
			break;
		case 0x008d: /* Numan Athletics */
			*bytes_per_row = 8;
			*pitch = 0x120;
			break;

		case 0x0000: /* Numan (used to clear spriteram) */
/*		0000 0000 0000 : src0*/
/*		0000 0001 0000 : dst0*/
/*		003d 75a0      : src (7AEB40)*/
/*		---- ----      : spriteram*/
/*		0800		   : numbytes*/
/*		0000		   : blit*/
			*bytes_per_row = 0x10;
			*pitch = 0;
			break;

		case 0x0001:
			*bytes_per_row = 0x1000;
			*pitch = 0x1000;
			break;

		case 0x0401: /* F/A */
			*bytes_per_row = 4*0x40;
			*pitch = 36*0x40;
			break;

		default:
/*		case 0x00f1:*/
/*		case 0x0781:*/
/*		case 0x07c1:*/
/*		case 0x07e1:*/
			*bytes_per_row = (64 - (format>>5))*0x40;
			*pitch = 0x1000;
			break;
		}
	}
} /* blit_setup */

/*
$efff20: sprite control: 0x3a,0x3e,0x3f
			bit 0x01 selects spriteram bank

               0    2    4    6    8    a    c    e
$efff00:	src0 src1 src2 dst0 dst1 dst2 BANK [src
$efff10:	src] [dst dst] #BYT BLIT eINT 001f 0001
$efff20:	003f 003f IACK ---- ---- ---- ---- ----
$efff30:	---- ---- ---- ---- ---- ---- ---- ----
$efff40:	---- ---- ---- ---- ---- ---- ---- ----
$efff50:	---- ---- ---- ---- ---- ---- ---- ----
$efff60:	---- ---- ---- ---- ---- ---- ---- ----
$efff70:	---- ---- ---- ---- ---- ---- ---- ----
$efff80:	0050 0170 0020 0100 0000 0000 0000 GFXE
$efff90:	0000 0001 0002 0003 FLIP ---- ---- ----
$efffa0:	PRI  PRI  PRI  PRI  ---- ---- 00c0 ----		priority (0..7)
$efffb0:	COLR COLR COLR COLR 0001 0004 0000 ----		color (0..f)
$efffc0:	???? ???? ???? ????	???? ???? ???? ----		ROZ

Emeralda:
$efff80:	0048 0177 0020 0100 0000 00fd 0000 GFXE
*/

static void namcona1_blit( void )
{
	int src0 = namcona1_vreg[0x0];
	int src1 = namcona1_vreg[0x1];
	int src2 = namcona1_vreg[0x2];

	int dst0 = namcona1_vreg[0x3];
	int dst1 = namcona1_vreg[0x4];
	int dst2 = namcona1_vreg[0x5];

	int gfxbank = namcona1_vreg[0x6];

	/* dest and source are provided as dword offsets */
	UINT32 src_baseaddr	= 2*((namcona1_vreg[0x7]<<16)|namcona1_vreg[0x8]);
	UINT32 dst_baseaddr	= 2*((namcona1_vreg[0x9]<<16)|namcona1_vreg[0xa]);

	int num_bytes = namcona1_vreg[0xb];

	int dest_offset, source_offset;
	int dest_bytes_per_row, dst_pitch;
	int source_bytes_per_row, src_pitch;

	(void)dst2;
	(void)dst0;
	(void)src2;
	(void)src0;

	log_cb(RETRO_LOG_DEBUG, LOGPRE  "0x%08x: blt(%08x,%08x,%08x);%04x %04x %04x; %04x %04x %04x; gfx=%04x\n",
		activecpu_get_pc(),
		dst_baseaddr,src_baseaddr,num_bytes,
		src0,src1,src2,
		dst0,dst1,dst2,
		gfxbank );

	blit_setup( dst1, &dest_bytes_per_row, &dst_pitch, gfxbank);
	blit_setup( src1, &source_bytes_per_row, &src_pitch, gfxbank );

	if( num_bytes&1 )
	{
		num_bytes++;
	}

	if( dst_baseaddr < 0xf00000 )
	{
		dst_baseaddr += 0xf40000;
	}

	dest_offset		= 0;
	source_offset	= 0;

	while( num_bytes>0 )
	{
		if( transfer_dword(
			dst_baseaddr + dest_offset,
			src_baseaddr + source_offset ) )
		{
			return;
		}

		num_bytes -= 2;

		dest_offset+=2;
		if( dest_offset >= dest_bytes_per_row )
		{
			dst_baseaddr += dst_pitch;
			dest_offset = 0;
		}

		source_offset+=2;
		if( source_offset >= source_bytes_per_row )
		{
			src_baseaddr += src_pitch;
			source_offset = 0;
		}
	}
} /* namcona1_blit */

static WRITE16_HANDLER( namcona1_vreg_w )
{
	COMBINE_DATA( &namcona1_vreg[offset] );

	switch( offset )
	{
	case 0x18/2:
		namcona1_blit();
		/* see also 0x1e */
		break;

	case 0x1a/2:
		mEnableInterrupts = 1;
		/* interrupt enable mask; 0 enables INT level */
		break;
	}
} /* namcona1_vreg_w */

/***************************************************************/

static WRITE16_HANDLER( bogus_w )
{
/*	extern int debug_key_pressed;*/
/*	debug_key_pressed = 1;*/
}
static READ16_HANDLER( bogus_r )
{
/*	extern int debug_key_pressed;*/
/*	debug_key_pressed = 1;*/
	return 0;
}

static MEMORY_READ16_START( namcona1_readmem )
	{ 0x000000, 0x000fff, namcona1_mcu_r },
	{ 0x001000, 0x07ffff, MRA16_RAM },		/* work RAM */
	{ 0x080000, 0x3fffff, bogus_r },
	{ 0x400000, 0xbfffff, MRA16_BANK2 },	/* data */
	{ 0xc00000, 0xdfffff, MRA16_BANK1 },	/* code */
	{ 0xe00000, 0xe00fff, namcona1_nvram_r },
	{ 0xe40000, 0xe4000f, custom_key_r },
	{ 0xe40010, 0xeffeff, bogus_r },
	{ 0xefff00, 0xefffff, namcona1_vreg_r },
	{ 0xf00000, 0xf01fff, namcona1_paletteram_r },
	{ 0xf02000, 0xf3ffff, bogus_r },
	{ 0xf40000, 0xf7ffff, namcona1_gfxram_r },
	{ 0xf80000, 0xfeffff, bogus_r },
	{ 0xff0000, 0xff7fff, namcona1_videoram_r },
	{ 0xff8000, 0xffdfff, MRA16_RAM },		/* spare videoram */
	{ 0xffe000, 0xffefff, MRA16_RAM },		/* scroll registers */
	{ 0xfff000, 0xffffff, MRA16_RAM },		/* spriteram */
MEMORY_END

static MEMORY_WRITE16_START( namcona1_writemem )
	{ 0x000000, 0x000fff, namcona1_mcu_w, &mcu_ram },
	{ 0x001000, 0x07ffff, MWA16_RAM, &namcona1_workram },
	{ 0x080000, 0x3f8007, bogus_w },
	{ 0x3f8008, 0x3f8009, mcu_command_w },
	{ 0x3f800a, 0x3fffff, bogus_w },
	{ 0x400000, 0xdfffff, MWA16_ROM }, /* data + code */
	{ 0xe00000, 0xe00fff, namcona1_nvram_w },
	{ 0xe01000, 0xe3ffff, bogus_w },
	{ 0xe40000, 0xe4000f, custom_key_w },
	{ 0xe40010, 0xeffeff, bogus_w },
	{ 0xefff00, 0xefffff, namcona1_vreg_w, &namcona1_vreg },
	{ 0xf00000, 0xf01fff, namcona1_paletteram_w, &paletteram16 },
	{ 0xf02000, 0xf3ffff, bogus_w },
	{ 0xf40000, 0xf7ffff, namcona1_gfxram_w },
	{ 0xf80000, 0xfeffff, bogus_w },
	{ 0xff0000, 0xff7fff, namcona1_videoram_w, &videoram16 },
	{ 0xff8000, 0xffdfff, MWA16_RAM, &namcona1_sparevram },
	{ 0xffe000, 0xffefff, MWA16_RAM, &namcona1_scroll },
	{ 0xfff000, 0xffffff, MWA16_RAM, &spriteram16 },
MEMORY_END

INTERRUPT_GEN( namcona1_interrupt )
{
	int level = cpu_getiloops(); /* 0,1,2,3,4 */
	if( level==0 )
	{
		simulate_mcu();
	}
	if( mEnableInterrupts )
	{
		if( (namcona1_vreg[0x1a/2]&(1<<level))==0 )
		{
			cpu_set_irq_line(0, level+1, HOLD_LINE);
		}
	}
}

static struct NAMCONAinterface NAMCONA_interface =
{
	4*8000,
	REGION_CPU1,
	100
};

/* cropped at sides */
static MACHINE_DRIVER_START( namcona1 )
	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 50113000/4)
	MDRV_CPU_MEMORY(namcona1_readmem,namcona1_writemem)
	MDRV_CPU_VBLANK_INT(namcona1_interrupt,5)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(namcosna1)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER|VIDEO_HAS_SHADOWS)
	MDRV_SCREEN_SIZE(38*8, 32*8)
	MDRV_VISIBLE_AREA(8, 38*8-1-8, 4*8, 32*8-1)
	MDRV_PALETTE_LENGTH(0x1000)

	MDRV_VIDEO_START(namcona1)
	MDRV_VIDEO_UPDATE(namcona1)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(NAMCONA, NAMCONA_interface)
MACHINE_DRIVER_END


/* full-width */
static MACHINE_DRIVER_START( namcona1w )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(namcona1)

	/* video hardware */
	MDRV_VISIBLE_AREA(0, 38*8-1-0, 4*8, 32*8-1)
MACHINE_DRIVER_END


static void
init_namcona1( void )
{
	data16_t *pMem = (data16_t *)memory_region( REGION_CPU1 );
	pMem[0] = 0x0007; pMem[1] = 0xfffc; /* (?) stack */
	pMem[2] = 0x00c0; pMem[3] = 0x0000; /* reset vector */

	mpBank0 = &pMem[0x80000/2];
	mpBank1 = mpBank0 +  0x200000/2;

	cpu_setbank( 1, mpBank0 ); /* code */
	cpu_setbank( 2, mpBank1 ); /* data */

	mCoinCount[0] = mCoinCount[1] = mCoinCount[2] = mCoinCount[3] = 0;
	mCoinState = 0;
	mEnableInterrupts = 0;
}

DRIVER_INIT( bkrtmaq ){		init_namcona1(); namcona1_gametype = NAMCO_BKRTMAQ; }
DRIVER_INIT( cgangpzl ){	init_namcona1(); namcona1_gametype = NAMCO_CGANGPZL; }
DRIVER_INIT( emeralda ){	init_namcona1(); namcona1_gametype = NAMCO_EMERALDA; }
DRIVER_INIT( exbania ){		init_namcona1(); namcona1_gametype = NAMCO_EXBANIA; }
DRIVER_INIT( fa ){			init_namcona1(); namcona1_gametype = NAMCO_FA; }
DRIVER_INIT( knckhead ){	init_namcona1(); namcona1_gametype = NAMCO_KNCKHEAD; }
DRIVER_INIT( numanath ){	init_namcona1(); namcona1_gametype = NAMCO_NUMANATH; }
DRIVER_INIT( quiztou ){		init_namcona1(); namcona1_gametype = NAMCO_QUIZTOU; }
DRIVER_INIT( swcourt ){		init_namcona1(); namcona1_gametype = NAMCO_SWCOURT; }
DRIVER_INIT( tinklpit ){	init_namcona1(); namcona1_gametype = NAMCO_TINKLPIT; }
DRIVER_INIT( xday2 ){		init_namcona1(); namcona1_gametype = NAMCO_XDAY2; }

ROM_START( bkrtmaq )
	ROM_REGION( 0xa80000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "mq1_ep0l.bin", 0x080001, 0x080000, CRC(f029bc57) SHA1(fdbf8b8b9f69d5755ca5197dda4f887b12dd66f4) ) /* 0xc00000 */
	ROM_LOAD16_BYTE( "mq1_ep0u.bin", 0x080000, 0x080000, CRC(4cff62b8) SHA1(5cac170dcfbeb3dcfa0840bdbe7541a9d2f44a14) )
	ROM_LOAD16_BYTE( "mq1_ep1l.bin", 0x180001, 0x080000, CRC(e3be6f4b) SHA1(75d9a4cff25e63a9d6c092aa6e241eccd1c61f91) )
	ROM_LOAD16_BYTE( "mq1_ep1u.bin", 0x180000, 0x080000, CRC(b44e31b2) SHA1(3d8c63789b98ada3663ba9e28c370815a9a9c3ed) )

	ROM_LOAD16_BYTE( "mq1_ma0l.bin", 0x280001, 0x100000, CRC(11fed35f) SHA1(511d98b6b42b330238a1874bca031b1892654a48) ) /* 0x400000 */
	ROM_LOAD16_BYTE( "mq1_ma0u.bin", 0x280000, 0x100000, CRC(23442ac0) SHA1(fac706f24045d51a2712f51530967140ea8e875f) )
	ROM_LOAD16_BYTE( "mq1_ma1l.bin", 0x480001, 0x100000, CRC(fe82205f) SHA1(860cc7a96ae3f848ce594077c1362e4e22a36908) )
	ROM_LOAD16_BYTE( "mq1_ma1u.bin", 0x480000, 0x100000, CRC(0cdb6bd0) SHA1(b8b398477c9654e96921110fb30c754240183897) )
ROM_END

ROM_START( cgangpzl )
	ROM_REGION( 0x180000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "cp2-ep0l.bin", 0x080001, 0x80000, CRC(8f5cdcc5) SHA1(925db3f3f16224bc28f97a57aba0ab2b51c5067c) ) /* 0xc00000 */
	ROM_LOAD16_BYTE( "cp2-ep0u.bin", 0x080000, 0x80000, CRC(3a816140) SHA1(613c367e08a0a20ec62e1938faab0128743b26f8) )
ROM_END

ROM_START( cgangpzj )
	ROM_REGION( 0x180000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "cp1-ep0l.bin", 0x080001, 0x80000, CRC(2825f7ba) SHA1(5f6f8df6bdf0f45656904411cdbb31fdcf8f3be0) ) /* 0xc00000 */
	ROM_LOAD16_BYTE( "cp1-ep0u.bin", 0x080000, 0x80000, CRC(94d7d6fc) SHA1(2460741e0dbb2ccff28f4fbc419a7507382467d2) )
ROM_END

ROM_START( emeralda )
	ROM_REGION( 0x280000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "ep0lb.bin",    0x080001, 0x080000, CRC(fcd55293) SHA1(fdabf9d5f528c37196ac1e031b097618b4c887b5) ) /* 0xc00000 */
	ROM_LOAD16_BYTE( "ep0ub.bin",    0x080000, 0x080000, CRC(a52f00d5) SHA1(85f95d2a69a2df2e9195f55583645c064b0b6fe6) )
	ROM_LOAD16_BYTE( "em1-ep1l.bin", 0x180001, 0x080000, CRC(373c1c59) SHA1(385cb3bc056b798878de890dbff97a8bdd48fe4e) )
	ROM_LOAD16_BYTE( "em1-ep1u.bin", 0x180000, 0x080000, CRC(4e969152) SHA1(2c89ae5d43585f479f16cf8278f8fc001e077e45) )
ROM_END

ROM_START( emerldaa )
	ROM_REGION( 0x280000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "em1-ep0l.bin", 0x080001, 0x080000, CRC(443f3fce) SHA1(35b6c834e5716c1e9b55f1e39f4e7336dbbe2d9b) ) /* 0xc00000 */
	ROM_LOAD16_BYTE( "em1-ep0u.bin", 0x080000, 0x080000, CRC(484a2a81) SHA1(1b60c18dfb2aebfd4aa8b2a85a1e90883a1f8e61) )
	ROM_LOAD16_BYTE( "em1-ep1l.bin", 0x180001, 0x080000, CRC(373c1c59) SHA1(385cb3bc056b798878de890dbff97a8bdd48fe4e) )
	ROM_LOAD16_BYTE( "em1-ep1u.bin", 0x180000, 0x080000, CRC(4e969152) SHA1(2c89ae5d43585f479f16cf8278f8fc001e077e45) )
ROM_END

ROM_START( exvania )
	ROM_REGION( 0xa80000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "ex1-ep0l.bin", 0x080001, 0x080000, CRC(18c12015) SHA1(e4f3524e798545c434549719b377c8b5863f580f) ) /* 0xc00000 */
	ROM_LOAD16_BYTE( "ex1-ep0u.bin", 0x080000, 0x080000, CRC(07d054d1) SHA1(e2d2cb81acd309c519686572804648bef4cbd191) )

	ROM_LOAD16_BYTE( "ex1-ma0l.bin", 0x280001, 0x100000, CRC(17922cd4) SHA1(af92c2335e7110c0c5e712f3148c1534d22d3814) ) /* 0x400000 */
	ROM_LOAD16_BYTE( "ex1-ma0u.bin", 0x280000, 0x100000, CRC(93d66106) SHA1(c5d665db04ae0e8992ef46544e2cb7b0e27c8bfe) )
	ROM_LOAD16_BYTE( "ex1-ma1l.bin", 0x480001, 0x100000, CRC(e4bba6ed) SHA1(6483ef91e5a5b8ddd13a3d889936c39829fa50d6) )
	ROM_LOAD16_BYTE( "ex1-ma1u.bin", 0x480000, 0x100000, CRC(04e7c4b0) SHA1(78180d96cd1fae583617d4d227ed4ee24f2f9e29) )
ROM_END

ROM_START( knckhead )
	ROM_REGION( 0xa80000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "kh2-ep0l.bin", 0x080001, 0x080000, CRC(b4b88077) SHA1(9af03d1832ad6c77222e18427f4afca330a41ce6) ) /* 0xc00000 */
	ROM_LOAD16_BYTE( "kh2-ep0u.bin", 0x080000, 0x080000, CRC(a578d97e) SHA1(9a5bb6649cca7b98daf538a66c813f61cca2e2ec) )
	ROM_LOAD16_BYTE( "kh1-ep1l.bin", 0x180001, 0x080000, CRC(27e6ab4e) SHA1(66f397cc2117c1e73652c4800c0937e6d8116380) )
	ROM_LOAD16_BYTE( "kh1-ep1u.bin", 0x180000, 0x080000, CRC(487b2434) SHA1(2d62db85ceac1fca61c39e4db92c96ae80ba3323) )

	ROM_LOAD16_BYTE( "kh1-ma0l.bin", 0x280001, 0x100000, CRC(7b2db5df) SHA1(ecc392c4683cf0718d986e73336b69952d324548) ) /* 0x400000 */
	ROM_LOAD16_BYTE( "kh1-ma0u.bin", 0x280000, 0x100000, CRC(6983228b) SHA1(5f3eeb780e9d91445b4c11da63d4ca580e654f34) )
	ROM_LOAD16_BYTE( "kh1-ma1l.bin", 0x480001, 0x100000, CRC(b24f93e6) SHA1(3d8951485dc8a2810da9ddf2f4896efa31779bf4) )
	ROM_LOAD16_BYTE( "kh1-ma1u.bin", 0x480000, 0x100000, CRC(18a60348) SHA1(298e0e0e7649e872791c3c99c81a19f273e9eb8a) )
	ROM_LOAD16_BYTE( "kh1-ma2l.bin", 0x680001, 0x100000, CRC(82064ee9) SHA1(0b984565d17e580f49fff982a1621ef90e14c064) )
	ROM_LOAD16_BYTE( "kh1-ma2u.bin", 0x680000, 0x100000, CRC(17fe8c3d) SHA1(88c45076477725faa5f8a23512e65a40385bb27d) )
	ROM_LOAD16_BYTE( "kh1-ma3l.bin", 0x880001, 0x100000, CRC(ad9a7807) SHA1(c40f18a68306e76acd89ccb3fc82b8106556912e) )
	ROM_LOAD16_BYTE( "kh1-ma3u.bin", 0x880000, 0x100000, CRC(efeb768d) SHA1(15d016244549f3ea0d19f5cfb04bcebd65ac6134) )
ROM_END

ROM_START( knckhedj )
	ROM_REGION( 0xa80000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "kh1-ep0l.bin", 0x080001, 0x080000, CRC(94660bec) SHA1(42fa23f759cf66b05f30c2fc03a12fd14ae1f796) ) /* 0xc00000 */
	ROM_LOAD16_BYTE( "kh1-ep0u.bin", 0x080000, 0x080000, CRC(ad640d69) SHA1(62595a9d1d5952cbe3dd7266cfda9292be51d269) )
	ROM_LOAD16_BYTE( "kh1-ep1l.bin", 0x180001, 0x080000, CRC(27e6ab4e) SHA1(66f397cc2117c1e73652c4800c0937e6d8116380) )
	ROM_LOAD16_BYTE( "kh1-ep1u.bin", 0x180000, 0x080000, CRC(487b2434) SHA1(2d62db85ceac1fca61c39e4db92c96ae80ba3323) )

	ROM_LOAD16_BYTE( "kh1-ma0l.bin", 0x280001, 0x100000, CRC(7b2db5df) SHA1(ecc392c4683cf0718d986e73336b69952d324548) ) /* 0x400000 */
	ROM_LOAD16_BYTE( "kh1-ma0u.bin", 0x280000, 0x100000, CRC(6983228b) SHA1(5f3eeb780e9d91445b4c11da63d4ca580e654f34) )
	ROM_LOAD16_BYTE( "kh1-ma1l.bin", 0x480001, 0x100000, CRC(b24f93e6) SHA1(3d8951485dc8a2810da9ddf2f4896efa31779bf4) )
	ROM_LOAD16_BYTE( "kh1-ma1u.bin", 0x480000, 0x100000, CRC(18a60348) SHA1(298e0e0e7649e872791c3c99c81a19f273e9eb8a) )
	ROM_LOAD16_BYTE( "kh1-ma2l.bin", 0x680001, 0x100000, CRC(82064ee9) SHA1(0b984565d17e580f49fff982a1621ef90e14c064) )
	ROM_LOAD16_BYTE( "kh1-ma2u.bin", 0x680000, 0x100000, CRC(17fe8c3d) SHA1(88c45076477725faa5f8a23512e65a40385bb27d) )
	ROM_LOAD16_BYTE( "kh1-ma3l.bin", 0x880001, 0x100000, CRC(ad9a7807) SHA1(c40f18a68306e76acd89ccb3fc82b8106556912e) )
	ROM_LOAD16_BYTE( "kh1-ma3u.bin", 0x880000, 0x100000, CRC(efeb768d) SHA1(15d016244549f3ea0d19f5cfb04bcebd65ac6134) )
ROM_END

ROM_START( numanatj )
	ROM_REGION( 0xa80000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "nm1_ep0l.bin", 0x080001, 0x080000, CRC(4398b898) SHA1(0d1517409ba181f796f7f413cac704c60085b505) ) /* 0xc00000 */
	ROM_LOAD16_BYTE( "nm1_ep0u.bin", 0x080000, 0x080000, CRC(be90aa79) SHA1(6884a8d72dd34c889527e8e653f5e5b4cf3fb5d6) )
	ROM_LOAD16_BYTE( "nm1_ep1l.bin", 0x180001, 0x080000, CRC(4581dcb4) SHA1(1f46f98e63a7c9cdfde9e8ee2696a13c3f9bcc8e) )
	ROM_LOAD16_BYTE( "nm1_ep1u.bin", 0x180000, 0x080000, CRC(30cd589a) SHA1(74a14ec41fe4fc9f73e5357b0903f1199ed96337) )

	ROM_LOAD16_BYTE( "nm1_ma0l.bin", 0x280001, 0x100000, CRC(20faaa57) SHA1(9dbfc0dd48eec37b2c0715a5691c6e6f923fc7f7) ) /* 0x400000 */
	ROM_LOAD16_BYTE( "nm1_ma0u.bin", 0x280000, 0x100000, CRC(ed7c37f2) SHA1(829751af33754ade941f76982e196b494d56ab0a) )
	ROM_LOAD16_BYTE( "nm1_ma1l.bin", 0x480001, 0x100000, CRC(2232e3b4) SHA1(e9da3dc34eb2576c8a88e23cb9007129e885496d) )
	ROM_LOAD16_BYTE( "nm1_ma1u.bin", 0x480000, 0x100000, CRC(6cc9675c) SHA1(fec74da4479f2a088760efc6908e6acfaea3989f) )
	ROM_LOAD16_BYTE( "nm1_ma2l.bin", 0x680001, 0x100000, CRC(208abb39) SHA1(52d7247a71c6a14467f12f5270921bba1824cc3f) )
	ROM_LOAD16_BYTE( "nm1_ma2u.bin", 0x680000, 0x100000, CRC(03a3f204) SHA1(9cb0422c8ecc819d0cc8a65c29a228369d78d986) )
	ROM_LOAD16_BYTE( "nm1_ma3l.bin", 0x880001, 0x100000, CRC(42a539e9) SHA1(1c53a5a031648891ab7a37cf026c979404ce9589) )
	ROM_LOAD16_BYTE( "nm1_ma3u.bin", 0x880000, 0x100000, CRC(f79e2112) SHA1(8bb8639a9d3a5d3ac5c9bb78e72b3d76582a9c25) )
ROM_END

ROM_START( numanath )
	ROM_REGION( 0xa80000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "nm2_ep0l.bin", 0x080001, 0x080000, CRC(f24414bb) SHA1(68b13dfdc2292afd5279edb891fe63972f991e7b) ) /* 0xc00000 */
	ROM_LOAD16_BYTE( "nm2_ep0u.bin", 0x080000, 0x080000, CRC(25c41616) SHA1(68ba67d3dd45f3bdddfa2fd21b574535306c1214) )
	ROM_LOAD16_BYTE( "nm1_ep1l.bin", 0x180001, 0x080000, CRC(4581dcb4) SHA1(1f46f98e63a7c9cdfde9e8ee2696a13c3f9bcc8e) )
	ROM_LOAD16_BYTE( "nm1_ep1u.bin", 0x180000, 0x080000, CRC(30cd589a) SHA1(74a14ec41fe4fc9f73e5357b0903f1199ed96337) )

	ROM_LOAD16_BYTE( "nm1_ma0l.bin", 0x280001, 0x100000, CRC(20faaa57) SHA1(9dbfc0dd48eec37b2c0715a5691c6e6f923fc7f7) ) /* 0x400000 */
	ROM_LOAD16_BYTE( "nm1_ma0u.bin", 0x280000, 0x100000, CRC(ed7c37f2) SHA1(829751af33754ade941f76982e196b494d56ab0a) )
	ROM_LOAD16_BYTE( "nm1_ma1l.bin", 0x480001, 0x100000, CRC(2232e3b4) SHA1(e9da3dc34eb2576c8a88e23cb9007129e885496d) )
	ROM_LOAD16_BYTE( "nm1_ma1u.bin", 0x480000, 0x100000, CRC(6cc9675c) SHA1(fec74da4479f2a088760efc6908e6acfaea3989f) )
	ROM_LOAD16_BYTE( "nm1_ma2l.bin", 0x680001, 0x100000, CRC(208abb39) SHA1(52d7247a71c6a14467f12f5270921bba1824cc3f) )
	ROM_LOAD16_BYTE( "nm1_ma2u.bin", 0x680000, 0x100000, CRC(03a3f204) SHA1(9cb0422c8ecc819d0cc8a65c29a228369d78d986) )
	ROM_LOAD16_BYTE( "nm1_ma3l.bin", 0x880001, 0x100000, CRC(42a539e9) SHA1(1c53a5a031648891ab7a37cf026c979404ce9589) )
	ROM_LOAD16_BYTE( "nm1_ma3u.bin", 0x880000, 0x100000, CRC(f79e2112) SHA1(8bb8639a9d3a5d3ac5c9bb78e72b3d76582a9c25) )
ROM_END

ROM_START( quiztou )
	ROM_REGION( 0xa80000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "qt1_ep0l.bin", 0x080001, 0x080000, CRC(b680e543) SHA1(f10f38113a46c821d8e9d66f52d7311d9d52e595) ) /* 0xc00000 */
	ROM_LOAD16_BYTE( "qt1_ep0u.bin", 0x080000, 0x080000, CRC(143c5e4d) SHA1(24c584986c97a5e6fe7e73f0e9af4af28ed20c4a) )
	ROM_LOAD16_BYTE( "qt1_ep1l.bin", 0x180001, 0x080000, CRC(33a72242) SHA1(5d17f033878d28dbebba50931a549ccf84802c05) )
	ROM_LOAD16_BYTE( "qt1_ep1u.bin", 0x180000, 0x080000, CRC(69f876cb) SHA1(d0c7e972a04c45d3ab34ef5be88614d6389189c6) )

	ROM_LOAD16_BYTE( "qt1_ma0l.bin", 0x280001, 0x100000, CRC(5597f2b9) SHA1(747c4be867d4eb37ffab8303740729686a00b825) ) /* 0x400000 */
	ROM_LOAD16_BYTE( "qt1_ma0u.bin", 0x280000, 0x100000, CRC(f0a4cb7d) SHA1(364e85af956e7cfc29c957da11574a4b389f7797) )
	ROM_LOAD16_BYTE( "qt1_ma1l.bin", 0x480001, 0x100000, CRC(1b9ce7a6) SHA1(dac1da9dd8076f238211fed5c780b4b8bededf22) )
	ROM_LOAD16_BYTE( "qt1_ma1u.bin", 0x480000, 0x100000, CRC(58910872) SHA1(c0acbd64e90672564c3839fd21870672aa32e439) )
	ROM_LOAD16_BYTE( "qt1_ma2l.bin", 0x680001, 0x100000, CRC(94739917) SHA1(b5be5c9fd7223d3fb601f769cb80f56a5a586de0) )
	ROM_LOAD16_BYTE( "qt1_ma2u.bin", 0x680000, 0x100000, CRC(6ba5b893) SHA1(071caed9cf261f1f8af7079875bd206177baef1a) )
	ROM_LOAD16_BYTE( "qt1_ma3l.bin", 0x880001, 0x100000, CRC(aa9dc6ff) SHA1(c738f8c59bb5245874576c5bcf88c7138fa9a147) )
	ROM_LOAD16_BYTE( "qt1_ma3u.bin", 0x880000, 0x100000, CRC(14a5a163) SHA1(1107f50e491bedeb4ab7ac3f32cfe47727274ba9) )
ROM_END

ROM_START( swcourt )
	ROM_REGION( 0xa80000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "sc1-ep0l.bin", 0x080001, 0x080000, CRC(145111dd) SHA1(f8f74f77fb80af2ea37ea8ddbf02c1f3fcaf3fdb) ) /* 0xc00000 */
	ROM_LOAD16_BYTE( "sc1-ep0u.bin", 0x080000, 0x080000, CRC(c721c138) SHA1(5d30d66629d982b54c3bb62118be940dc7b69a6b) )
	ROM_LOAD16_BYTE( "sc1-ep1l.bin", 0x180001, 0x080000, CRC(fb45cf5f) SHA1(6ded351daa9b39d0b8149100caefc4fa0c598e79) )
	ROM_LOAD16_BYTE( "sc1-ep1u.bin", 0x180000, 0x080000, CRC(1ce07b15) SHA1(b1b28cc480301c9ad642597c7cdd8e9cdec996a6) )

	ROM_LOAD16_BYTE( "sc1-ma0l.bin", 0x280001, 0x100000, CRC(3e531f5e) SHA1(6da56630bdfbb19f1639c539779c180d106f6ee2) ) /* 0x400000 */
	ROM_LOAD16_BYTE( "sc1-ma0u.bin", 0x280000, 0x100000, CRC(31e76a45) SHA1(5c278c167c1025c648ce2da2c3764645e96dcd55) )
	ROM_LOAD16_BYTE( "sc1-ma1l.bin", 0x480001, 0x100000, CRC(8ba3a4ec) SHA1(f881e7b4728f388d18450ba85e13e233071fbc88) )
	ROM_LOAD16_BYTE( "sc1-ma1u.bin", 0x480000, 0x100000, CRC(252dc4b7) SHA1(f1be6bd045495c7a0ecd97f01d1dc8ad341fecfd) )
ROM_END

ROM_START( tinklpit )
	ROM_REGION( 0xa80000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "tk1-ep0l.bin", 0x080001, 0x080000, CRC(fdccae42) SHA1(398384482ccb3eb08bfb9db495513272a5188d92) ) /* 0xc00000 */
	ROM_LOAD16_BYTE( "tk1-ep0u.bin", 0x080000, 0x080000, CRC(62cdb48c) SHA1(73c7b99b117b8dc567bc254b0ffcc117c9d42fb5) )
	ROM_LOAD16_BYTE( "tk1-ep1l.bin", 0x180001, 0x080000, CRC(7e90f104) SHA1(79e371426b2e32dc8f687e4d124d23c251198937) )
	ROM_LOAD16_BYTE( "tk1-ep1u.bin", 0x180000, 0x080000, CRC(9c0b70d6) SHA1(eac44d3470f4c2ddd9c41f82e6398bca0cc8a4fd) )

	ROM_LOAD16_BYTE( "tk1-ma0l.bin", 0x280001, 0x100000, CRC(c6b4e15d) SHA1(55252ba4d904b14940436f1b4dc5e2a6bd163bdf) ) /* 0x400000 */
	ROM_LOAD16_BYTE( "tk1-ma0u.bin", 0x280000, 0x100000, CRC(a3ad6f67) SHA1(54289eed5347defb5464ec5a610a6748909159f6) )
	ROM_LOAD16_BYTE( "tk1-ma1l.bin", 0x480001, 0x100000, CRC(61cfb92a) SHA1(eacf0e7557f33d552045f43a116ff08c533a2771) )
	ROM_LOAD16_BYTE( "tk1-ma1u.bin", 0x480000, 0x100000, CRC(54b77816) SHA1(9341d07858623e1920eaae7b2b90126c7057297e) )
	ROM_LOAD16_BYTE( "tk1-ma2l.bin", 0x680001, 0x100000, CRC(087311d2) SHA1(6fe50f9e08551e57d15a15b01e3822a6cb7c8352) )
	ROM_LOAD16_BYTE( "tk1-ma2u.bin", 0x680000, 0x100000, CRC(5ce20c2c) SHA1(7eaff21714bae44f8b21b6db98f055e04bfbae18) )
ROM_END

ROM_START( fghtatck )
	ROM_REGION( 0xa80000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "fa2_ep0l.bin", 0x080001, 0x080000, CRC(8996db9c) SHA1(ebbe7d4cb2960a346cfbdf38c77638d71b6ba20e) ) /* 0xc00000 */
	ROM_LOAD16_BYTE( "fa2_ep0u.bin", 0x080000, 0x080000, CRC(58d5e090) SHA1(950219d4e9bf440f92e3c8765f47e23a9019d2d1) )
	ROM_LOAD16_BYTE( "fa1_ep1l.bin", 0x180001, 0x080000, CRC(b23a5b01) SHA1(4ba9bc2102fffc93a5ff73a107d557fc0f3beefd) )
	ROM_LOAD16_BYTE( "fa1_ep1u.bin", 0x180000, 0x080000, CRC(de2eb129) SHA1(912993cab1c2edcaf986478f2ae22a2f10edf807) )

	ROM_LOAD16_BYTE( "fa1_ma0l.bin", 0x280001, 0x100000, CRC(a0a95e54) SHA1(da35f8a6a5bc9e2b5b6cacf8eb0d900ef1073a67) ) /* 0x400000 */
	ROM_LOAD16_BYTE( "fa1_ma0u.bin", 0x280000, 0x100000, CRC(1d0135bd) SHA1(2a7f8d09c213629a68376ce0379be61b37711d0a) )
	ROM_LOAD16_BYTE( "fa1_ma1l.bin", 0x480001, 0x100000, CRC(c4adf0a2) SHA1(4cc7adc68b1db7e725a973b31d52720bd7dc1140) )
	ROM_LOAD16_BYTE( "fa1_ma1u.bin", 0x480000, 0x100000, CRC(900297be) SHA1(57bb2078ff104c6f631c67219f80f8ede5ddbd09) )
ROM_END

ROM_START( fa )
	ROM_REGION( 0xa80000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "fa1_ep0l.bin", 0x080001, 0x080000, CRC(182eee5c) SHA1(49769e3b72b59fc3e7b73364fe97168977dbe66b) ) /* 0xc00000 */
	ROM_LOAD16_BYTE( "fa1_ep0u.bin", 0x080000, 0x080000, CRC(7ea7830e) SHA1(79390943eea0b8029b2b8869233caf27228e776a) )
	ROM_LOAD16_BYTE( "fa1_ep1l.bin", 0x180001, 0x080000, CRC(b23a5b01) SHA1(4ba9bc2102fffc93a5ff73a107d557fc0f3beefd) )
	ROM_LOAD16_BYTE( "fa1_ep1u.bin", 0x180000, 0x080000, CRC(de2eb129) SHA1(912993cab1c2edcaf986478f2ae22a2f10edf807) )

	ROM_LOAD16_BYTE( "fa1_ma0l.bin", 0x280001, 0x100000, CRC(a0a95e54) SHA1(da35f8a6a5bc9e2b5b6cacf8eb0d900ef1073a67) ) /* 0x400000 */
	ROM_LOAD16_BYTE( "fa1_ma0u.bin", 0x280000, 0x100000, CRC(1d0135bd) SHA1(2a7f8d09c213629a68376ce0379be61b37711d0a) )
	ROM_LOAD16_BYTE( "fa1_ma1l.bin", 0x480001, 0x100000, CRC(c4adf0a2) SHA1(4cc7adc68b1db7e725a973b31d52720bd7dc1140) )
	ROM_LOAD16_BYTE( "fa1_ma1u.bin", 0x480000, 0x100000, CRC(900297be) SHA1(57bb2078ff104c6f631c67219f80f8ede5ddbd09) )
ROM_END

ROM_START( xday2 )
	ROM_REGION( 0xa80000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "xds1mpr0.4b", 0x080001, 0x080000, CRC(83539aaa) SHA1(42d97bb2daaf5ff48efac70f0ff37869c5ba177d) ) /* 0xc00000 */
	ROM_LOAD16_BYTE( "xds1mpr1.8b", 0x080000, 0x080000, CRC(468b36de) SHA1(52817be9913a6938ce6add2834ba1a727b1d677e) )

	ROM_LOAD16_BYTE( "xds1dat0.4b", 0x280001, 0x200000, CRC(42cecc8b) SHA1(7510f16b908dd0f7828887dcfa26c5e4643df66c) ) /* 0x400000 */
	ROM_LOAD16_BYTE( "xds1dat1.8b", 0x280000, 0x200000, CRC(d250b7e8) SHA1(b99251ae8e25fae062d33e74ff800ab43fb308a2) )
	ROM_LOAD16_BYTE( "xds1dat2.4c", 0x680001, 0x200000, CRC(99d72a08) SHA1(4615b43b9a81240ffee8b0f021037f554f4f1f24) )
	ROM_LOAD16_BYTE( "xds1dat3.8c", 0x680000, 0x200000, CRC(8980acc4) SHA1(ecd94a3d3a38923e8e322cd8863671af26e30812) )
ROM_END

GAMEX( 1992,bkrtmaq,  0,        namcona1w, namcona1_quiz,	bkrtmaq,  ROT0, "Namco", "Bakuretsu Quiz Ma-Q Dai Bouken (Japan)", GAME_IMPERFECT_SOUND )
GAMEX( 1992,cgangpzl, 0,        namcona1w, namcona1_joy,	cgangpzl, ROT0, "Namco", "Cosmo Gang the Puzzle (US)", GAME_IMPERFECT_SOUND )
GAMEX( 1992,cgangpzj, cgangpzl, namcona1w, namcona1_joy,	cgangpzl, ROT0, "Namco", "Cosmo Gang the Puzzle (Japan)", GAME_IMPERFECT_SOUND )
GAMEX( 1992,exvania,  0,        namcona1,  namcona1_joy,	exbania,  ROT0, "Namco", "Exvania (Japan)", GAME_IMPERFECT_SOUND )
GAMEX( 1992,fghtatck, 0,        namcona1,  namcona1_joy,	fa,       ROT90,"Namco", "Fighter and Attacker (US)", GAME_IMPERFECT_SOUND )
GAMEX( 1992,fa,       fghtatck, namcona1,  namcona1_joy,	fa,       ROT90,"Namco", "F-A (Japan)", GAME_IMPERFECT_SOUND )
GAMEX( 1992,knckhead, 0,        namcona1,  namcona1_joy,	knckhead, ROT0, "Namco", "Knuckle Heads (World)", GAME_IMPERFECT_SOUND )
GAMEX( 1992,knckhedj, knckhead, namcona1,  namcona1_joy,	knckhead, ROT0, "Namco", "Knuckle Heads (Japan)", GAME_IMPERFECT_SOUND )
GAMEX( 1992,swcourt,  0,        namcona1w, namcona1_joy,	swcourt,  ROT0, "Namco", "Super World Court (Japan)", GAME_IMPERFECT_SOUND )
GAMEX( 1993,emeralda, 0,        namcona1w, namcona1_joy,	emeralda, ROT0, "Namco", "Emeraldia (Japan Version B)", GAME_IMPERFECT_SOUND )
GAMEX( 1993,emerldaa, emeralda, namcona1w, namcona1_joy,	emeralda, ROT0, "Namco", "Emeraldia (Japan)", GAME_IMPERFECT_SOUND )
GAMEX( 1993,numanath, 0,        namcona1,  namcona1_joy,	numanath, ROT0, "Namco", "Numan Athletics (World)", GAME_IMPERFECT_SOUND )
GAMEX( 1993,numanatj, numanath, namcona1,  namcona1_joy,	numanath, ROT0, "Namco", "Numan Athletics (Japan)", GAME_IMPERFECT_SOUND )
GAMEX( 1993,quiztou,  0,        namcona1,  namcona1_quiz,	quiztou,  ROT0, "Namco", "Nettou! Gekitou! Quiztou!! (Japan)", GAME_IMPERFECT_SOUND )
GAMEX( 1993,tinklpit, 0,        namcona1w, namcona1_joy,	tinklpit, ROT0, "Namco", "Tinkle Pit (Japan)", GAME_IMPERFECT_SOUND )
GAMEX( 1995,xday2,    0,        namcona1,  xday2,           xday2,    ROT0, "Namco", "X-Day 2 (Japan)", GAME_IMPERFECT_SOUND|GAME_NOT_WORKING )
