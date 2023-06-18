/***************************************************************************

"Mitchell hardware". Actually used mostly by Capcom.

All games run on the same hardware except mgakuen, which runs on an
earlier version, without RAM banking, not encrypted (standard Z80)
and without EEPROM.

Other games that might run on this hardware:
"Chi-toitsu"(YUGA 1988)-Another version of"Mahjong Gakuen"
"MIRAGE -Youjyu mahjong den-"(MITCHELL 1994)

Notes:
- Super Pang has a protection which involves copying code stored in the
  EEPROM to RAM and execute it from there. The first time the game is run,
  you have to keep the player 1 start button pressed until the title screen
  appears. This forces the game to initialize the EEPROM, otherwise it will
  not work.
  This is simulated with a kluge in input_r.

TODO:
- understand what bits 0 and 3 of input port 0x05 are
- ball speed is erratic in Block Block. It was not like this at one point.
  This is probably related to interrupts and maybe to the above bits.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/eeprom.h"

/* in machine/kabuki.c */
void mgakuen2_decode(void);
void pang_decode(void);
void cworld_decode(void);
void hatena_decode(void);
void spang_decode(void);
void sbbros_decode(void);
void marukin_decode(void);
void qtono1_decode(void);
void qsangoku_decode(void);
void block_decode(void);


VIDEO_START( pang );
VIDEO_UPDATE( pang );

WRITE_HANDLER( mgakuen_paletteram_w );
READ_HANDLER( mgakuen_paletteram_r );
WRITE_HANDLER( mgakuen_videoram_w );
READ_HANDLER( mgakuen_videoram_r );
WRITE_HANDLER( mgakuen_objram_w );
READ_HANDLER( mgakuen_objram_r );

WRITE_HANDLER( pang_video_bank_w );
WRITE_HANDLER( pang_videoram_w );
READ_HANDLER( pang_videoram_r );
WRITE_HANDLER( pang_colorram_w );
READ_HANDLER( pang_colorram_r );
WRITE_HANDLER( pang_gfxctrl_w );
WRITE_HANDLER( pang_paletteram_w );
READ_HANDLER( pang_paletteram_r );

extern unsigned char *pang_videoram;
extern unsigned char *pang_colorram;

extern size_t pang_videoram_size;



static WRITE_HANDLER( pang_bankswitch_w )
{
	int bankaddress;
	unsigned char *RAM = memory_region(REGION_CPU1);

	bankaddress = 0x10000 + (data & 0x0f) * 0x4000;

	cpu_setbank(1,&RAM[bankaddress]);
}



/***************************************************************************

  EEPROM

***************************************************************************/

static struct EEPROM_interface eeprom_interface =
{
	6,		/* address bits */
	16,		/* data bits */
	"0110",	/*  read command */
	"0101",	/* write command */
	"0111"	/* erase command */
};

static unsigned char *nvram;
static size_t nvram_size;
static int init_eeprom_count;

static NVRAM_HANDLER( mitchell )
{
	if (read_or_write)
	{
		EEPROM_save(file);					/* EEPROM */
		if (nvram_size)	/* Super Pang, Block Block */
			mame_fwrite(file,nvram,nvram_size);	/* NVRAM */
	}
	else
	{
		EEPROM_init(&eeprom_interface);

		if (file)
		{
			init_eeprom_count = 0;
			EEPROM_load(file);					/* EEPROM */
			if (nvram_size)	/* Super Pang, Block Block */
				mame_fread(file,nvram,nvram_size);	/* NVRAM */
		}
		else
			init_eeprom_count = 1000;	/* for Super Pang */
	}
}

static READ_HANDLER( pang_port5_r )
{
	int bit;
	extern const struct GameDriver driver_mgakuen2;

	bit = EEPROM_read_bit() << 7;

	/* bits 0 and (sometimes) 3 are checked in the interrupt handler. */
	/* Maybe they are vblank related, but I'm not sure. */
	/* bit 3 is checked before updating the palette so it really seems to be vblank. */
	/* Many games require two interrupts per frame and for these bits to toggle, */
	/* otherwise music doesn't work. */
	if (cpu_getiloops() & 1) bit |= 0x01;
	else bit |= 0x08;
if (Machine->gamedrv == &driver_mgakuen2)	/* hack... music doesn't work otherwise */
	bit ^= 0x08;

	return (input_port_0_r(0) & 0x76) | bit;
}

static WRITE_HANDLER( eeprom_cs_w )
{
	EEPROM_set_cs_line(data ? CLEAR_LINE : ASSERT_LINE);
}

static WRITE_HANDLER( eeprom_clock_w )
{
	EEPROM_set_clock_line(data ? CLEAR_LINE : ASSERT_LINE);
}

static WRITE_HANDLER( eeprom_serial_w )
{
	EEPROM_write_bit(data);
}



/***************************************************************************

  Input handling

***************************************************************************/

static int dial[2],dial_selected;

static READ_HANDLER( block_input_r )
{
	static int dir[2];

	if (dial_selected)
	{
		int delta;

		delta = (readinputport(4 + offset) - dial[offset]) & 0xff;
		if (delta & 0x80)
		{
			delta = (-delta) & 0xff;
			if (dir[offset])
			{
			/* don't report movement on a direction change, otherwise it will stutter */
				dir[offset] = 0;
				delta = 0;
			}
		}
		else if (delta > 0)
		{
			if (dir[offset] == 0)
			{
			/* don't report movement on a direction change, otherwise it will stutter */
				dir[offset] = 1;
				delta = 0;
			}
		}
		if (delta > 0x3f) delta = 0x3f;
		return delta << 2;
	}
	else
	{
		int res;

		res = readinputport(2 + offset) & 0xf7;
		if (dir[offset]) res |= 0x08;

		return res;
	}
}

static WRITE_HANDLER( block_dial_control_w )
{
	if (data == 0x08)
	{
		/* reset the dial counters */
		dial[0] = readinputport(4);
		dial[1] = readinputport(5);
	}
	else if (data == 0x80)
		dial_selected = 0;
	else
		dial_selected = 1;
}


static int keymatrix;

static READ_HANDLER( mahjong_input_r )
{
	int i;

	for (i = 0;i < 5;i++)
		if (keymatrix & (0x80 >> i)) return readinputport(2 + 5 * offset + i);

	return 0xff;
}

static WRITE_HANDLER( mahjong_input_select_w )
{
	keymatrix = data;
}


static int input_type;

static READ_HANDLER( input_r )
{
	switch (input_type)
	{
		case 0:
		default:
			return readinputport(1 + offset);
			break;
		case 1:	/* Mahjong games */
			if (offset) return mahjong_input_r(offset-1);
			else return readinputport(1);
			break;
		case 2:	/* Block Block - dial control */
			if (offset) return block_input_r(offset-1);
			else return readinputport(1);
			break;
		case 3:	/* Super Pang - simulate START 1 press to initialize EEPROM */
			if (offset || init_eeprom_count == 0) return readinputport(1 + offset);
			else
			{
				init_eeprom_count--;
				return readinputport(1) & ~0x08;
			}
			break;
	}
}

static WRITE_HANDLER( input_w )
{
	switch (input_type)
	{
		case 0:
		default:
log_cb(RETRO_LOG_DEBUG, LOGPRE "PC %04x: write %02x to port 01\n",activecpu_get_pc(),data);
			break;
		case 1:
			mahjong_input_select_w(offset,data);
			break;
		case 2:
			block_dial_control_w(offset,data);
			break;
	}
}



/***************************************************************************

  Memory handlers

***************************************************************************/

static MEMORY_READ_START( mgakuen_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xc7ff, mgakuen_paletteram_r },	/* palette RAM */
	{ 0xc800, 0xcfff, pang_colorram_r },	/* Attribute RAM */
	{ 0xd000, 0xdfff, mgakuen_videoram_r },	/* char RAM */
	{ 0xe000, 0xefff, MRA_RAM },	/* Work RAM */
	{ 0xf000, 0xffff, mgakuen_objram_r },	/* OBJ RAM */
MEMORY_END

static MEMORY_WRITE_START( mgakuen_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc7ff, mgakuen_paletteram_w },
	{ 0xc800, 0xcfff, pang_colorram_w, &pang_colorram },
	{ 0xd000, 0xdfff, mgakuen_videoram_w, &pang_videoram, &pang_videoram_size },
	{ 0xe000, 0xefff, MWA_RAMROM },
	{ 0xf000, 0xffff, mgakuen_objram_w },	/* OBJ RAM */
MEMORY_END

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xc7ff, pang_paletteram_r },	/* Banked palette RAM */
	{ 0xc800, 0xcfff, pang_colorram_r },	/* Attribute RAM */
	{ 0xd000, 0xdfff, pang_videoram_r },	/* Banked char / OBJ RAM */
	{ 0xe000, 0xffff, MRA_RAM },	/* Work RAM */
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc7ff, pang_paletteram_w },
	{ 0xc800, 0xcfff, pang_colorram_w, &pang_colorram },
	{ 0xd000, 0xdfff, pang_videoram_w, &pang_videoram, &pang_videoram_size },
	{ 0xe000, 0xffff, MWA_RAMROM },
MEMORY_END

static PORT_READ_START( readport )
	{ 0x00, 0x02, input_r },	/* Super Pang needs a kludge to initialize EEPROM.
						The Mahjong games and Block Block need special input treatment */
	{ 0x03, 0x03, input_port_12_r },	/* mgakuen only */
	{ 0x04, 0x04, input_port_13_r },	/* mgakuen only */
	{ 0x05, 0x05, pang_port5_r },
PORT_END

static PORT_WRITE_START( writeport )
	{ 0x00, 0x00, pang_gfxctrl_w },    /* Palette bank, layer enable, coin counters, more */
	{ 0x01, 0x01, input_w },
	{ 0x02, 0x02, pang_bankswitch_w },      /* Code bank register */
	{ 0x03, 0x03, YM2413_data_port_0_w },
	{ 0x04, 0x04, YM2413_register_port_0_w },
	{ 0x05, 0x05, OKIM6295_data_0_w },
	{ 0x06, 0x06, MWA_NOP },	/* watchdog? irq ack? */
	{ 0x07, 0x07, pang_video_bank_w },      /* Video RAM bank register */
	{ 0x08, 0x08, eeprom_cs_w },
	{ 0x10, 0x10, eeprom_clock_w },
	{ 0x18, 0x18, eeprom_serial_w },
PORT_END



INPUT_PORTS_START( mgakuen )
	PORT_START      /* DSW */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* USED - handled in port5_r */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unused? */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* USED - handled in port5_r */
	PORT_BIT( 0x70, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unused? */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* data from EEPROM */

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 Kan", KEYCODE_LCONTROL, IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 M", KEYCODE_M, IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P1 I", KEYCODE_I, IP_JOY_NONE )
	PORT_BITX(0x40, IP_ACTIVE_LOW, 0, "P1 E", KEYCODE_E, IP_JOY_NONE )
	PORT_BITX(0x80, IP_ACTIVE_LOW, 0, "P1 A", KEYCODE_A, IP_JOY_NONE )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 Reach", KEYCODE_LSHIFT, IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 N", KEYCODE_N, IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P1 J", KEYCODE_J, IP_JOY_NONE )
	PORT_BITX(0x40, IP_ACTIVE_LOW, 0, "P1 F", KEYCODE_F, IP_JOY_NONE )
	PORT_BITX(0x80, IP_ACTIVE_LOW, 0, "P1 B", KEYCODE_B, IP_JOY_NONE )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 Ron", KEYCODE_Z, IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 Chi", KEYCODE_SPACE, IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P1 K", KEYCODE_K, IP_JOY_NONE )
	PORT_BITX(0x40, IP_ACTIVE_LOW, 0, "P1 G", KEYCODE_G, IP_JOY_NONE )
	PORT_BITX(0x80, IP_ACTIVE_LOW, 0, "P1 C", KEYCODE_C, IP_JOY_NONE )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 Pon", KEYCODE_LALT, IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P1 L", KEYCODE_L, IP_JOY_NONE )
	PORT_BITX(0x40, IP_ACTIVE_LOW, 0, "P1 H", KEYCODE_H, IP_JOY_NONE )
	PORT_BITX(0x80, IP_ACTIVE_LOW, 0, "P1 D", KEYCODE_D, IP_JOY_NONE )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 Flip", KEYCODE_X, IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P2 Kan", KEYCODE_LCONTROL, IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P2 M", KEYCODE_M, IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P2 I", KEYCODE_I, IP_JOY_NONE )
	PORT_BITX(0x40, IP_ACTIVE_LOW, 0, "P2 E", KEYCODE_E, IP_JOY_NONE )
	PORT_BITX(0x80, IP_ACTIVE_LOW, 0, "P2 A", KEYCODE_A, IP_JOY_NONE )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P2 Reach", KEYCODE_LSHIFT, IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P2 N", KEYCODE_N, IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P2 J", KEYCODE_J, IP_JOY_NONE )
	PORT_BITX(0x40, IP_ACTIVE_LOW, 0, "P2 F", KEYCODE_F, IP_JOY_NONE )
	PORT_BITX(0x80, IP_ACTIVE_LOW, 0, "P2 B", KEYCODE_B, IP_JOY_NONE )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P2 Ron", KEYCODE_Z, IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P2 Chi", KEYCODE_SPACE, IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P2 K", KEYCODE_K, IP_JOY_NONE )
	PORT_BITX(0x40, IP_ACTIVE_LOW, 0, "P2 G", KEYCODE_G, IP_JOY_NONE )
	PORT_BITX(0x80, IP_ACTIVE_LOW, 0, "P2 C", KEYCODE_C, IP_JOY_NONE )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P2 Pon", KEYCODE_LALT, IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P2 L", KEYCODE_L, IP_JOY_NONE )
	PORT_BITX(0x40, IP_ACTIVE_LOW, 0, "P2 H", KEYCODE_H, IP_JOY_NONE )
	PORT_BITX(0x80, IP_ACTIVE_LOW, 0, "P2 D", KEYCODE_D, IP_JOY_NONE )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P2 Flip", KEYCODE_X, IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x08, 0x08, "Rules" )
	PORT_DIPSETTING(    0x08, "Kantou" )
	PORT_DIPSETTING(    0x00, "Kansai" )
	PORT_DIPNAME( 0x10, 0x00, "Harness Type" )
	PORT_DIPSETTING(    0x10, "Generic" )
	PORT_DIPSETTING(    0x00, "Royal Mahjong" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	/* DSW2 */
	PORT_DIPNAME( 0x03, 0x03, "Player 1 Skill" )
	PORT_DIPSETTING(    0x03, "Weak" )
	PORT_DIPSETTING(    0x02, "Normal" )
	PORT_DIPSETTING(    0x01, "Strong" )
	PORT_DIPSETTING(    0x00, "Very Strong" )
	PORT_DIPNAME( 0x0c, 0x0c, "Player 1 Skill" )
	PORT_DIPSETTING(    0x0c, "Weak" )
	PORT_DIPSETTING(    0x08, "Normal" )
	PORT_DIPSETTING(    0x04, "Strong" )
	PORT_DIPSETTING(    0x00, "Very Strong" )
	PORT_DIPNAME( 0x10, 0x00, "Music" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "Help Mode" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( marukin )
	PORT_START      /* DSW */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* USED - handled in port5_r */
	PORT_BITX(0x02, 0x02, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unused? */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* USED - handled in port5_r */
	PORT_BIT( 0x70, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unused? */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* data from EEPROM */

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE )	/* same as the service mode farther down */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 Kan", KEYCODE_LCONTROL, IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 M", KEYCODE_M, IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P1 I", KEYCODE_I, IP_JOY_NONE )
	PORT_BITX(0x40, IP_ACTIVE_LOW, 0, "P1 E", KEYCODE_E, IP_JOY_NONE )
	PORT_BITX(0x80, IP_ACTIVE_LOW, 0, "P1 A", KEYCODE_A, IP_JOY_NONE )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 Reach", KEYCODE_LSHIFT, IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 N", KEYCODE_N, IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P1 J", KEYCODE_J, IP_JOY_NONE )
	PORT_BITX(0x40, IP_ACTIVE_LOW, 0, "P1 F", KEYCODE_F, IP_JOY_NONE )
	PORT_BITX(0x80, IP_ACTIVE_LOW, 0, "P1 B", KEYCODE_B, IP_JOY_NONE )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 Ron", KEYCODE_Z, IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 Chi", KEYCODE_SPACE, IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P1 K", KEYCODE_K, IP_JOY_NONE )
	PORT_BITX(0x40, IP_ACTIVE_LOW, 0, "P1 G", KEYCODE_G, IP_JOY_NONE )
	PORT_BITX(0x80, IP_ACTIVE_LOW, 0, "P1 C", KEYCODE_C, IP_JOY_NONE )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 Pon", KEYCODE_LALT, IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P1 L", KEYCODE_L, IP_JOY_NONE )
	PORT_BITX(0x40, IP_ACTIVE_LOW, 0, "P1 H", KEYCODE_H, IP_JOY_NONE )
	PORT_BITX(0x80, IP_ACTIVE_LOW, 0, "P1 D", KEYCODE_D, IP_JOY_NONE )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 Flip", KEYCODE_X, IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P2 Kan", KEYCODE_LCONTROL, IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P2 M", KEYCODE_M, IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P2 I", KEYCODE_I, IP_JOY_NONE )
	PORT_BITX(0x40, IP_ACTIVE_LOW, 0, "P2 E", KEYCODE_E, IP_JOY_NONE )
	PORT_BITX(0x80, IP_ACTIVE_LOW, 0, "P2 A", KEYCODE_A, IP_JOY_NONE )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P2 Reach", KEYCODE_LSHIFT, IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P2 N", KEYCODE_N, IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P2 J", KEYCODE_J, IP_JOY_NONE )
	PORT_BITX(0x40, IP_ACTIVE_LOW, 0, "P2 F", KEYCODE_F, IP_JOY_NONE )
	PORT_BITX(0x80, IP_ACTIVE_LOW, 0, "P2 B", KEYCODE_B, IP_JOY_NONE )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P2 Ron", KEYCODE_Z, IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P2 Chi", KEYCODE_SPACE, IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P2 K", KEYCODE_K, IP_JOY_NONE )
	PORT_BITX(0x40, IP_ACTIVE_LOW, 0, "P2 G", KEYCODE_G, IP_JOY_NONE )
	PORT_BITX(0x80, IP_ACTIVE_LOW, 0, "P2 C", KEYCODE_C, IP_JOY_NONE )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P2 Pon", KEYCODE_LALT, IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P2 L", KEYCODE_L, IP_JOY_NONE )
	PORT_BITX(0x40, IP_ACTIVE_LOW, 0, "P2 H", KEYCODE_H, IP_JOY_NONE )
	PORT_BITX(0x80, IP_ACTIVE_LOW, 0, "P2 D", KEYCODE_D, IP_JOY_NONE )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P2 Flip", KEYCODE_X, IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( pkladies )
	PORT_START      /* DSW */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* USED - handled in port5_r */
	PORT_BITX(0x02, 0x02, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unused? */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* USED - handled in port5_r */
	PORT_BIT( 0x70, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unused? */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* data from EEPROM */

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE )	/* same as the service mode farther down */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P1 Deal", KEYCODE_LCONTROL, IP_JOY_NONE )
	PORT_BITX(0x40, IP_ACTIVE_LOW, 0, "P1 E", KEYCODE_E, IP_JOY_NONE )
	PORT_BITX(0x80, IP_ACTIVE_LOW, 0, "P1 A", KEYCODE_A, IP_JOY_NONE )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P1 Cancel", KEYCODE_LALT, IP_JOY_NONE )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x80, IP_ACTIVE_LOW, 0, "P1 B", KEYCODE_B, IP_JOY_NONE )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P1 Flip", KEYCODE_SPACE, IP_JOY_NONE )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x80, IP_ACTIVE_LOW, 0, "P1 C", KEYCODE_C, IP_JOY_NONE )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x80, IP_ACTIVE_LOW, 0, "P1 D", KEYCODE_D, IP_JOY_NONE )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P2 Deal", KEYCODE_LCONTROL, IP_JOY_NONE )
	PORT_BITX(0x40, IP_ACTIVE_LOW, 0, "P2 E", KEYCODE_E, IP_JOY_NONE )
	PORT_BITX(0x80, IP_ACTIVE_LOW, 0, "P2 A", KEYCODE_A, IP_JOY_NONE )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P2 Cancel", KEYCODE_LALT, IP_JOY_NONE )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x80, IP_ACTIVE_LOW, 0, "P2 B", KEYCODE_B, IP_JOY_NONE )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P2 Flip", KEYCODE_SPACE, IP_JOY_NONE )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x80, IP_ACTIVE_LOW, 0, "P2 C", KEYCODE_C, IP_JOY_NONE )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x80, IP_ACTIVE_LOW, 0, "P2 D", KEYCODE_D, IP_JOY_NONE )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( pang )
	PORT_START      /* DSW */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* USED - handled in port5_r */
	PORT_BITX(0x02, 0x02, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unused? */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* USED - handled in port5_r */
	PORT_BIT( 0x70, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unused? */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* data from EEPROM */

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
INPUT_PORTS_END

INPUT_PORTS_START( qtono1 )
	PORT_START      /* DSW */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* USED - handled in port5_r */
	PORT_BITX(0x02, 0x02, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unused? */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* USED - handled in port5_r */
	PORT_BIT( 0x70, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unused? */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* data from EEPROM */

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE )	/* same as the service mode farther down */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
INPUT_PORTS_END

INPUT_PORTS_START( block )
	PORT_START      /* DSW */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* USED - handled in port5_r */
	PORT_BITX(0x02, 0x02, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unused? */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* USED - handled in port5_r */
	PORT_BIT( 0x70, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unused? */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* data from EEPROM */

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SPECIAL )	/* dial direction */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SPECIAL )	/* dial direction */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )

	PORT_START      /* DIAL1 */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL, 50, 20, 0, 0)

	PORT_START      /* DIAL2 */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_PLAYER2, 50, 20, 0, 0)
INPUT_PORTS_END

INPUT_PORTS_START( blockj )
	PORT_START      /* DSW */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* USED - handled in port5_r */
	PORT_BITX(0x02, 0x02, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unused? */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* USED - handled in port5_r */
	PORT_BIT( 0x70, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unused? */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* data from EEPROM */

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SPECIAL )	/* dial direction */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SPECIAL )	/* dial direction */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	32768,	/* 32768 characters */
	4,		/* 4 bits per pixel */
	{ 32768*16*8+4, 32768*16*8+0,4, 0 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8    /* every char takes 16 consecutive bytes */
};

static struct GfxLayout marukin_charlayout =
{
	8,8,	/* 8*8 characters */
	65536,	/* 65536 characters */
	4,		/* 4 bits per pixel */
	{ 3*4, 2*4, 1*4, 0*4 },
	{ 0, 1, 2, 3, 16+0, 16+1, 16+2, 16+3 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8    /* every char takes 32 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,  /* 16*16 sprites */
	2048,   /* 2048 sprites */
	4,      /* 4 bits per pixel */
	{ 2048*64*8+4, 2048*64*8+0, 4, 0 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3,
			32*8+0, 32*8+1, 32*8+2, 32*8+3, 33*8+0, 33*8+1, 33*8+2, 33*8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8    /* every sprite takes 64 consecutive bytes */
};

static struct GfxDecodeInfo mgakuen_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &marukin_charlayout, 0,  64 }, /* colors 0-1023 */
	{ REGION_GFX2, 0, &spritelayout,       0,  16 }, /* colors 0- 255 */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo marukin_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &marukin_charlayout, 0, 128 }, /* colors 0-2047 */
	{ REGION_GFX2, 0, &spritelayout,       0,  16 }, /* colors 0- 255 */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,     0, 128 }, /* colors 0-2047 */
	{ REGION_GFX2, 0, &spritelayout,   0,  16 }, /* colors 0- 255 */
	{ -1 } /* end of array */
};



static struct YM2413interface ym2413_interface =
{
	1,	/* 1 chip */
	3579545,	/* ??? */
	{ YM2413_VOL(100,MIXER_PAN_CENTER,100,MIXER_PAN_CENTER) },	/* Volume */
};

static struct OKIM6295interface okim6295_interface =
{
	1,			/* 1 chip */
	{ 8000 },	/* 8000Hz ??? */
	{ REGION_SOUND1 },		/* memory region 2 */
	{ 50 }
};



static MACHINE_DRIVER_START( mgakuen )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 6000000)	/* ??? */
	MDRV_CPU_MEMORY(mgakuen_readmem,mgakuen_writemem)
	MDRV_CPU_PORTS(readport,writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,2)	/* ??? one extra irq seems to be needed for music (see input5_r) */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(8*8, (64-8)*8-1, 1*8, 31*8-1 )
	MDRV_GFXDECODE(mgakuen_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)	/* less colors than the others */

	MDRV_VIDEO_START(pang)
	MDRV_VIDEO_UPDATE(pang)

	/* sound hardware */
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
	MDRV_SOUND_ADD(YM2413, ym2413_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( pang )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 8000000)	/* Super Pang says 8MHZ ORIGINAL BOARD */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_PORTS(readport,writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,2)	/* ??? one extra irq seems to be needed for music (see input5_r) */

	MDRV_FRAMES_PER_SECOND(57.42)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(mitchell)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(8*8, (64-8)*8-1, 1*8, 31*8-1 )
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(pang)
	MDRV_VIDEO_UPDATE(pang)

	/* sound hardware */
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
	MDRV_SOUND_ADD(YM2413, ym2413_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( marukin )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 8000000)	/* Super Pang says 8MHZ ORIGINAL BOARD */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_PORTS(readport,writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,2)	/* ??? one extra irq seems to be needed for music (see input5_r) */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(mitchell)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(8*8, (64-8)*8-1, 1*8, 31*8-1 )
	MDRV_GFXDECODE(marukin_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(pang)
	MDRV_VIDEO_UPDATE(pang)

	/* sound hardware */
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
	MDRV_SOUND_ADD(YM2413, ym2413_interface)
MACHINE_DRIVER_END



ROM_START( mgakuen )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )	/* 192k for code */
	ROM_LOAD( "mg-1.1j",      0x00000, 0x08000, CRC(bf02ea6b) SHA1(bb1f5fbb211a5ed181f1afbba6b39737639d3ee7) )
	ROM_LOAD( "mg-2.1l",      0x10000, 0x20000, CRC(64141b0c) SHA1(2de6bcd5cf2c042e5bf5c294dd7625393e99682b) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mg-1.13h",     0x000000, 0x80000, CRC(fd6a0805) SHA1(f3d4d402dd96b8e4297a074b01d803cac16ac0d3) )	/* chars */
	ROM_LOAD( "mg-2.14h",     0x080000, 0x80000, CRC(e26e871e) SHA1(00f9642ced5f1795e02b357a06deee3d093f6dc0) )
	ROM_LOAD( "mg-3.16h",     0x100000, 0x80000, CRC(dd781d9a) SHA1(db5568be7e5fc15497b979451c65d8448063e04b) )
	ROM_LOAD( "mg-4.17h",     0x180000, 0x80000, CRC(97afcc79) SHA1(a84ddf089db7d26a0043815648f1674b240b8289) )

	ROM_REGION( 0x040000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mg-6.4l",      0x000000, 0x20000, CRC(34594e62) SHA1(a28493fc120ddfa6b51eeb3c111cc611cab54332) )	/* sprites */
	ROM_LOAD( "mg-7.6l",      0x020000, 0x20000, CRC(f304c806) SHA1(a803a7be8702874fb547624be621a55f6ef5be1c) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* OKIM */
	ROM_LOAD( "mg-5.1c",      0x00000, 0x80000, CRC(170332f1) SHA1(bc60f144a224f348fd5b8c0207e18a881f739fc1) )	/* banked */
ROM_END

ROM_START( 7toitsu )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )	/* 192k for code */
	ROM_LOAD( "mc01.1j",      0x00000, 0x08000, CRC(0bebe45f) SHA1(24fadffd0033565441a75f36e2cb085a37e0f0e5) )
	ROM_LOAD( "mc02.1l",      0x10000, 0x20000, CRC(375378b0) SHA1(cbb5db5fda1d87902b22130243d579cb28803707) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mg-1.13h",     0x000000, 0x80000, CRC(fd6a0805) SHA1(f3d4d402dd96b8e4297a074b01d803cac16ac0d3) )	/* chars */
	ROM_LOAD( "mg-2.14h",     0x080000, 0x80000, CRC(e26e871e) SHA1(00f9642ced5f1795e02b357a06deee3d093f6dc0) )
	ROM_LOAD( "mg-3.16h",     0x100000, 0x80000, CRC(dd781d9a) SHA1(db5568be7e5fc15497b979451c65d8448063e04b) )
	ROM_LOAD( "mg-4.17h",     0x180000, 0x80000, CRC(97afcc79) SHA1(a84ddf089db7d26a0043815648f1674b240b8289) )

	ROM_REGION( 0x040000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mc06.4l",      0x000000, 0x20000, CRC(0ef83926) SHA1(850b382d919c86ae09d802d5183edd37c81e7c97) )	/* sprites */
	ROM_LOAD( "mc07.6l",      0x020000, 0x20000, CRC(59f9ffb1) SHA1(1c225a526860637a713d4b8add2fbc0a17c0a854) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* OKIM */
	ROM_LOAD( "mg-5.1c",      0x00000, 0x80000, CRC(170332f1) SHA1(bc60f144a224f348fd5b8c0207e18a881f739fc1) )	/* banked */
ROM_END

ROM_START( mgakuen2 )
	ROM_REGION( 2*0x50000, REGION_CPU1, 0 )	/* 320k for code + 320k for decrypted opcodes */
	ROM_LOAD( "mg2-xf.1j",    0x00000, 0x08000, CRC(c8165d2d) SHA1(95146e293b2e005c4015590811119a4070dda65b) )
	ROM_LOAD( "mg2-y.1l",     0x10000, 0x20000, CRC(75bbcc14) SHA1(52ec279fda131c8de06d8c940df12d61ec6881cc) )
	ROM_LOAD( "mg2-z.3l",     0x30000, 0x20000, CRC(bfdba961) SHA1(75045562edbdef1eb599d6a6bfc4247c33c11258) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mg2-a.13h",    0x000000, 0x80000, CRC(31a0c55e) SHA1(2a6bd9f9d1fee17fd4798ba9aad05e05b3cfb210) )	/* chars */
	ROM_LOAD( "mg2-b.14h",    0x080000, 0x80000, CRC(c18488fa) SHA1(42efb2a51305dce86ec721c747ee13d82c4f6cd6) )
	ROM_LOAD( "mg2-c.16h",    0x100000, 0x80000, CRC(9425b364) SHA1(44373e137e0b820ad705ef1c299a9d31a1e8d0ca) )
	ROM_LOAD( "mg2-d.17h",    0x180000, 0x80000, CRC(6cc9eeba) SHA1(ef4a4f44abacc8b08576846d514765ac2eadf9a6) )

	ROM_REGION( 0x040000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mg2-f.4l",     0x000000, 0x20000, CRC(3172c9fe) SHA1(7012bf2eb70c70b08f0204a4766dd8fce0bcc135) )	/* sprites */
	ROM_LOAD( "mg2-g.6l",     0x020000, 0x20000, CRC(19b8b61c) SHA1(a9f5cea6f4788886719f5f9301ef172978b3b9a2) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* OKIM */
	ROM_LOAD( "mg2-e.1c",     0x00000, 0x80000, CRC(70fd0809) SHA1(7f85fc5f575c925c3246b45fc041f57fc3eb7cc8) )	/* banked */
ROM_END

ROM_START( pkladies )
	ROM_REGION( 2*0x20000, REGION_CPU1, 0 )	/* 128k for code + 128k for decrypted opcodes */
	ROM_LOAD( "pko-prg1.14f", 0x00000, 0x08000, CRC(86585a94) SHA1(067791da20556e6c47de26fbf85389d92f9709db) )
	ROM_LOAD( "pko-prg2.15f", 0x10000, 0x10000, CRC(86cbe82d) SHA1(3997a642004d1226cfce0f590123d4e407edf094) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "pko-001.8h",   0x000000, 0x80000, CRC(1ead5d9b) SHA1(ac9b294ce1fcfb994f7c06e0e3f0ec8d86f2d908) )	/* chars */
	ROM_LOAD16_BYTE( "pko-003.8j",   0x000001, 0x80000, CRC(339ab4e6) SHA1(0dbe6801e72df1226a4df3f6911523c95cd2ac6a) )
	ROM_LOAD16_BYTE( "pko-002.9h",   0x100000, 0x80000, CRC(1cf02586) SHA1(d78fa4824c00b88049c36c1525031f3b8b5d36c8) )
	ROM_LOAD16_BYTE( "pko-004.9j",   0x100001, 0x80000, CRC(09ccb442) SHA1(c8deb7c29f75ad61237c8b737caded58f21f3bba) )

	ROM_REGION( 0x040000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "pko-chr1.2j",  0x000000, 0x20000, CRC(31ce33cd) SHA1(9e8cea7625e7436a8480c4114c9148c67ccbf247) )	/* sprites */
	ROM_LOAD( "pko-chr2.3j",  0x020000, 0x20000, CRC(ad7e055f) SHA1(062f4d3b6e11ddce035bd0d5a279dc4489149cc4) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* OKIM */
	ROM_LOAD( "pko-voi1.2d",  0x00000, 0x20000, CRC(07e0f531) SHA1(315715f7686ae09c446029da36faec5bab7fcaf0) )
	ROM_LOAD( "pko-voi2.3d",  0x20000, 0x20000, CRC(18398bf6) SHA1(9e9ab85383350d01ba597951a48f18ecee1f46c6) )
ROM_END

ROM_START( pkladiel )
	ROM_REGION( 2*0x30000, REGION_CPU1, 0 )	/* 128k for code + 128k for decrypted opcodes */
	ROM_LOAD( "pk05.14f",     0x00000, 0x08000, CRC(ea1740a6) SHA1(eafd3fb0056a648dfc67b5d0a1dc93c4262e2a8b) )
	ROM_LOAD( "pk06.15f",     0x10000, 0x20000, CRC(3078ff5e) SHA1(5d91d68a07a968ee59f693841da165833a9fcf08) )	/* larger than pkladies - 2nd half unused? */

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "pko-001.8h",   0x000000, 0x80000, CRC(1ead5d9b) SHA1(ac9b294ce1fcfb994f7c06e0e3f0ec8d86f2d908) )	/* chars */
	ROM_LOAD16_BYTE( "pko-003.8j",   0x000001, 0x80000, CRC(339ab4e6) SHA1(0dbe6801e72df1226a4df3f6911523c95cd2ac6a) )
	ROM_LOAD16_BYTE( "pko-002.9h",   0x100000, 0x80000, CRC(1cf02586) SHA1(d78fa4824c00b88049c36c1525031f3b8b5d36c8) )
	ROM_LOAD16_BYTE( "pko-004.9j",   0x100001, 0x80000, CRC(09ccb442) SHA1(c8deb7c29f75ad61237c8b737caded58f21f3bba) )

	ROM_REGION( 0x040000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "pko-chr1.2j",  0x000000, 0x20000, CRC(31ce33cd) SHA1(9e8cea7625e7436a8480c4114c9148c67ccbf247) )	/* sprites */
	ROM_LOAD( "pko-chr2.3j",  0x020000, 0x20000, CRC(ad7e055f) SHA1(062f4d3b6e11ddce035bd0d5a279dc4489149cc4) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* OKIM */
	ROM_LOAD( "pko-voi1.2d",  0x00000, 0x20000, CRC(07e0f531) SHA1(315715f7686ae09c446029da36faec5bab7fcaf0) )
	ROM_LOAD( "pko-voi2.3d",  0x20000, 0x20000, CRC(18398bf6) SHA1(9e9ab85383350d01ba597951a48f18ecee1f46c6) )
ROM_END

ROM_START( dokaben )
	ROM_REGION( 2*0x50000, REGION_CPU1, 0 )	/* 320k for code + 320k for decrypted opcodes */
	ROM_LOAD( "db06.11h",     0x00000, 0x08000, CRC(413e0886) SHA1(e9e6117fbbd980bc0f5448ada6c1856919bf92b5) )
	ROM_LOAD( "db07.13h",     0x10000, 0x20000, CRC(8bdcf49e) SHA1(7d845ae2e640ec7d8d642e3aeef741d9f7b0a57c) )
	ROM_LOAD( "db08.14h",     0x30000, 0x20000, CRC(1643bdd9) SHA1(5805e749713dbffacbb1238b1b4d42e8473d3656) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "db02.1e",      0x000000, 0x20000, CRC(9aa8470c) SHA1(8acbed381d6140e70045da232dee9b4b165953f9) )	/* chars */
	ROM_LOAD( "db03.2e",      0x020000, 0x20000, CRC(3324e43d) SHA1(ed273d4de56e382e24ab0f0a8bcd5e30a05a1c6d) )
	/* 40000-7ffff empty */
	ROM_LOAD( "db04.1g",      0x080000, 0x20000, CRC(c0c5b6c2) SHA1(5d66d8b2a62ccab9574e04a867df9bbb8c0d15aa) )
	ROM_LOAD( "db05.2g",      0x0a0000, 0x20000, CRC(d2ab25f2) SHA1(96eea06d1645e0aade4c1b3153c55e2b61fd52c7) )
	/* c0000-fffff empty */

	ROM_REGION( 0x040000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "db10.2k",      0x000000, 0x20000, CRC(9e70f7ae) SHA1(ff3833a52d3d198f14e915ce52f7449cf04a0cca) )	/* sprites */
	ROM_LOAD( "db09.1k",      0x020000, 0x20000, CRC(2d9263f7) SHA1(fe2811ae47b9a250ea1485a91c2c3be742d90622) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* OKIM */
	ROM_LOAD( "db01.1d",      0x00000, 0x20000, CRC(62fa6b81) SHA1(0168b40df583f11cb28718aa8ab8be7cc08bf561) )
ROM_END

ROM_START( pang )
	ROM_REGION( 2*0x30000, REGION_CPU1, 0 )	/* 192k for code + 192k for decrypted opcodes */
	ROM_LOAD( "pang6.bin",    0x00000, 0x08000, CRC(68be52cd) SHA1(67b9ac15f4cbd3959c417f979beae36ae17334c1) )
	ROM_LOAD( "pang7.bin",    0x10000, 0x20000, CRC(4a2e70f6) SHA1(039db1b51374e5637b5c2ba8e18ccd08816613a7) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "pang_09.bin",  0x000000, 0x20000, CRC(3a5883f5) SHA1(a8a33071e10f5992e80afdb782c334829f9ae27f) )	/* chars */
	ROM_LOAD( "bb3.bin",      0x020000, 0x20000, CRC(79a8ed08) SHA1(c1e43889e29b80c7fe2c09b11eecde24450a1ff5) )
	/* 40000-7ffff empty */
	ROM_LOAD( "pang_11.bin",  0x080000, 0x20000, CRC(166a16ae) SHA1(7f907c78b7ac8c99e3d79761a6ae689c77e3a1f5) )
	ROM_LOAD( "bb5.bin",      0x0a0000, 0x20000, CRC(2fb3db6c) SHA1(328814d28569fec763975a8ae4c2767517a680af) )
	/* c0000-fffff empty */

	ROM_REGION( 0x040000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "bb10.bin",     0x000000, 0x20000, CRC(fdba4f6e) SHA1(9a2412a97682bbd25b8942520a0c02616bd59353) )	/* sprites */
	ROM_LOAD( "bb9.bin",      0x020000, 0x20000, CRC(39f47a63) SHA1(05675ad45909a7d723acaf4d53b4e588d4e048b9) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* OKIM */
	ROM_LOAD( "bb1.bin",      0x00000, 0x20000, CRC(c52e5b8e) SHA1(933b954bfdd2d67e28b032ffabde192531249c1f) )
ROM_END

ROM_START( pangb )
	ROM_REGION( 2*0x30000, REGION_CPU1, 0 )	/* 192k for code + 192k for decrypted opcodes */
	ROM_LOAD( "pang_04.bin",  0x30000, 0x08000, CRC(f68f88a5) SHA1(6f57891d399a46d8d5a531771129552ed420d10a) )   /* Decrypted opcode + data */
	ROM_CONTINUE(             0x00000, 0x08000 )
	ROM_LOAD( "pang_02.bin",  0x40000, 0x20000, CRC(3f15bb61) SHA1(4f74ee25f32a201482840158b4d4c7aca1cda684) )   /* Decrypted op codes */
	ROM_LOAD( "pang_03.bin",  0x10000, 0x20000, CRC(0c8477ae) SHA1(a31a8c00407dfc3017d56e29fac6114b73248030) )   /* Decrypted data */

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "pang_09.bin",  0x000000, 0x20000, CRC(3a5883f5) SHA1(a8a33071e10f5992e80afdb782c334829f9ae27f) )	/* chars */
	ROM_LOAD( "bb3.bin",      0x020000, 0x20000, CRC(79a8ed08) SHA1(c1e43889e29b80c7fe2c09b11eecde24450a1ff5) )
	/* 40000-7ffff empty */
	ROM_LOAD( "pang_11.bin",  0x080000, 0x20000, CRC(166a16ae) SHA1(7f907c78b7ac8c99e3d79761a6ae689c77e3a1f5) )
	ROM_LOAD( "bb5.bin",      0x0a0000, 0x20000, CRC(2fb3db6c) SHA1(328814d28569fec763975a8ae4c2767517a680af) )
	/* c0000-fffff empty */

	ROM_REGION( 0x040000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "bb10.bin",     0x000000, 0x20000, CRC(fdba4f6e) SHA1(9a2412a97682bbd25b8942520a0c02616bd59353) )	/* sprites */
	ROM_LOAD( "bb9.bin",      0x020000, 0x20000, CRC(39f47a63) SHA1(05675ad45909a7d723acaf4d53b4e588d4e048b9) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* OKIM */
	ROM_LOAD( "bb1.bin",      0x00000, 0x20000, CRC(c52e5b8e) SHA1(933b954bfdd2d67e28b032ffabde192531249c1f) )
ROM_END

ROM_START( bbros )
	ROM_REGION( 2*0x30000, REGION_CPU1, 0 )	/* 192k for code + 192k for decrypted opcodes */
	ROM_LOAD( "bb6.bin",      0x00000, 0x08000, CRC(a3041ca4) SHA1(2accb2151f621e4802211efe986969ebd3acb6d4) )
	ROM_LOAD( "bb7.bin",      0x10000, 0x20000, CRC(09231c68) SHA1(9e735487a99a5eb89a6abb81d5d9a20414ad75bf) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "bb2.bin",      0x000000, 0x20000, CRC(62f29992) SHA1(af4d43f76228e9908fbfbf83af2f577b84cc5e1d) )	/* chars */
	ROM_LOAD( "bb3.bin",      0x020000, 0x20000, CRC(79a8ed08) SHA1(c1e43889e29b80c7fe2c09b11eecde24450a1ff5) )
	/* 40000-7ffff empty */
	ROM_LOAD( "bb4.bin",      0x080000, 0x20000, CRC(f705aa89) SHA1(cce2d90f7b767044e84bc22a16474a2f6496292e) )
	ROM_LOAD( "bb5.bin",      0x0a0000, 0x20000, CRC(2fb3db6c) SHA1(328814d28569fec763975a8ae4c2767517a680af) )
	/* c0000-fffff empty */

	ROM_REGION( 0x040000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "bb10.bin",     0x000000, 0x20000, CRC(fdba4f6e) SHA1(9a2412a97682bbd25b8942520a0c02616bd59353) )	/* sprites */
	ROM_LOAD( "bb9.bin",      0x020000, 0x20000, CRC(39f47a63) SHA1(05675ad45909a7d723acaf4d53b4e588d4e048b9) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* OKIM */
	ROM_LOAD( "bb1.bin",      0x00000, 0x20000, CRC(c52e5b8e) SHA1(933b954bfdd2d67e28b032ffabde192531249c1f) )
ROM_END

ROM_START( pompingw )
	ROM_REGION( 2*0x30000, REGION_CPU1, 0 )	/* 192k for code + 192k for decrypted opcodes */
	ROM_LOAD( "pwj_06.11h",   0x00000, 0x08000, CRC(4a0a6426) SHA1(c61346c5f80507bdf543e9ea32ee3f814be8e27f) )
	ROM_LOAD( "pwj_07.13h",   0x10000, 0x20000, CRC(a9402420) SHA1(2ca3aa59d561826477e3509fcaeeec753d64d419) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "pw_02.1e",     0x000000, 0x20000, CRC(4b5992e4) SHA1(2071a1fcfc739d7ca837c03133909101b462d5a6) )	/* chars */
	ROM_LOAD( "bb3.bin",      0x020000, 0x20000, CRC(79a8ed08) SHA1(c1e43889e29b80c7fe2c09b11eecde24450a1ff5) )
	/* 40000-7ffff empty */
	ROM_LOAD( "pwj_04.1g",    0x080000, 0x20000, CRC(01e49081) SHA1(a29ffec199f196a2b3731e4863e863bdd04e2c58) )
	ROM_LOAD( "bb5.bin",      0x0a0000, 0x20000, CRC(2fb3db6c) SHA1(328814d28569fec763975a8ae4c2767517a680af) )
	/* c0000-fffff empty */

	ROM_REGION( 0x040000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "bb10.bin",     0x000000, 0x20000, CRC(fdba4f6e) SHA1(9a2412a97682bbd25b8942520a0c02616bd59353) )	/* sprites */
	ROM_LOAD( "bb9.bin",      0x020000, 0x20000, CRC(39f47a63) SHA1(05675ad45909a7d723acaf4d53b4e588d4e048b9) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* OKIM */
	ROM_LOAD( "bb1.bin",      0x00000, 0x20000, CRC(c52e5b8e) SHA1(933b954bfdd2d67e28b032ffabde192531249c1f) )
ROM_END

ROM_START( cworld )
	ROM_REGION( 2*0x50000, REGION_CPU1, 0 )	/* 320k for code + 320k for decrypted opcodes */
	ROM_LOAD( "cw05.bin",     0x00000, 0x08000, CRC(d3c1723d) SHA1(b67f63e39f4301909c967555222820b54e98a205) )
	ROM_LOAD( "cw06.bin",     0x10000, 0x20000, CRC(d71ed4a3) SHA1(5b6d498810e6fc8041f4326087f3be56863e91d9) )
	ROM_LOAD( "cw07.bin",     0x30000, 0x20000, CRC(d419ce08) SHA1(f0a8265e839f6bdab2926f48aba88b6f9aaa3b29) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "cw08.bin",     0x000000, 0x20000, CRC(6c80da3c) SHA1(3ed8bc025703d6eccc88af0caeeb8e75a88ba5db) )	/* chars */
	ROM_LOAD( "cw09.bin",     0x020000, 0x20000, CRC(7607da71) SHA1(4486550aa96bf5be0294763a9585fafda3216b27) )
	ROM_LOAD( "cw10.bin",     0x040000, 0x20000, CRC(6f0e639f) SHA1(473804068479516694a864982e2a734f63cb1cce) )
	ROM_LOAD( "cw11.bin",     0x060000, 0x20000, CRC(130bd7c0) SHA1(fde2c358367577b7c51648610b978649424d7637) )
	ROM_LOAD( "cw18.bin",     0x080000, 0x20000, CRC(be6ee0c9) SHA1(1cff9333b32f66440cb6caca27137406d2c9493a) )
	ROM_LOAD( "cw19.bin",     0x0a0000, 0x20000, CRC(51fc5532) SHA1(bea3097492ddbe7842e37d31a633378298459511) )
	ROM_LOAD( "cw20.bin",     0x0c0000, 0x20000, CRC(58381d58) SHA1(aef01f628ad9f2280662610c58e5819611e3435a) )
	ROM_LOAD( "cw21.bin",     0x0e0000, 0x20000, CRC(910cc753) SHA1(971fe794511b336b188d3e2e6b5cda71ae16257f) )

	ROM_REGION( 0x040000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "cw16.bin",     0x000000, 0x20000, CRC(f90217d1) SHA1(1dbfeb0fd44928d9428a3798fe6d6862164fdf52) )	/* sprites */
	ROM_LOAD( "cw17.bin",     0x020000, 0x20000, CRC(c953c702) SHA1(21d497dbb9ccccce3c440e6f0ba84c1e519d7fed) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* OKIM */
	ROM_LOAD( "cw01.bin",     0x00000, 0x20000, CRC(f4368f5b) SHA1(7a8657dd4c5f3b60f5137af3c644793c479562a8) )
ROM_END

ROM_START( hatena )
	ROM_REGION( 2*0x50000, REGION_CPU1, 0 )	/* 320k for code + 320k for decrypted opcodes */
	ROM_LOAD( "q2-05.rom",    0x00000, 0x08000, CRC(66c9e1da) SHA1(7ddbc4acf9d9d5b69f0bb60af65a171f3ba185b1) )
	ROM_LOAD( "q2-06.rom",    0x10000, 0x20000, CRC(5fc39916) SHA1(84ead43d8bad3f9c88fcb02171500298613646dc) )
	ROM_LOAD( "q2-07.rom",    0x30000, 0x20000, CRC(ec6d5e5e) SHA1(6269f5a5a3af91193afe85d34a764499877c2a24) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "q2-08.rom",    0x000000, 0x20000, CRC(6c80da3c) SHA1(3ed8bc025703d6eccc88af0caeeb8e75a88ba5db) )	/* chars */
	ROM_LOAD( "q2-09.rom",    0x020000, 0x20000, CRC(abe3e15c) SHA1(5af589e58b317758d1162913f6c104c8459546c0) )
	ROM_LOAD( "q2-10.rom",    0x040000, 0x20000, CRC(6963450d) SHA1(8fff6e9653b10194940b7a7a10f57995aafdd37c) )
	ROM_LOAD( "q2-11.rom",    0x060000, 0x20000, CRC(1e319fa2) SHA1(6064491d19cf9dd320535eb1807f4e5bf3e756ab) )
	ROM_LOAD( "q2-18.rom",    0x080000, 0x20000, CRC(be6ee0c9) SHA1(1cff9333b32f66440cb6caca27137406d2c9493a) )
	ROM_LOAD( "q2-19.rom",    0x0a0000, 0x20000, CRC(70300445) SHA1(499ba7e7cb3b41c858a346888547f98f8e7fe953) )
	ROM_LOAD( "q2-20.rom",    0x0c0000, 0x20000, CRC(21a6ff42) SHA1(d3ae3a5b898fa5202516e0f23e84255fb2164b52) )
	ROM_LOAD( "q2-21.rom",    0x0e0000, 0x20000, CRC(076280c9) SHA1(bdccbd8b169f7e19b955e0ede8bbe03d4009e354) )

	ROM_REGION( 0x040000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "q2-16.rom",    0x000000, 0x20000, CRC(ec19b2f0) SHA1(52d0a0b6e583103e0c8b73ecd27b03522accb3cb) )	/* sprites */
	ROM_LOAD( "q2-17.rom",    0x020000, 0x20000, CRC(ecd69d92) SHA1(a3ac417bc93f9cb126bd0896f4d85b1bef1dc681) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* OKIM */
	ROM_LOAD( "q2-01.rom",    0x00000, 0x20000, CRC(149e7a89) SHA1(103ab075b92c895e9991e7ef23df2b38d6a792c6) )
ROM_END

ROM_START( spang )
	ROM_REGION( 2*0x50000, REGION_CPU1, 0 )	/* 320k for code + 320k for decrypted opcodes */
	ROM_LOAD( "spe_06.rom",   0x00000, 0x08000, CRC(1af106fb) SHA1(476ba5c95e090663a47d3f98451bf3b79bac7748) )
	ROM_LOAD( "spe_07.rom",   0x10000, 0x20000, CRC(208b5f54) SHA1(9d44f7240b56756dcb69d110036b1cb13b1bbc02) )
	ROM_LOAD( "spe_08.rom",   0x30000, 0x20000, CRC(2bc03ade) SHA1(3a8ee342b0556a8f6d5a417c98e5c3c43422713d) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "spe_02.rom",   0x000000, 0x20000, CRC(63c9dfd2) SHA1(ddc8ddee336855e857fb3124c8b64af33c2d0080) )	/* chars */
	ROM_LOAD( "03.f2",        0x020000, 0x20000, CRC(3ae28bc1) SHA1(4f6d9a86f624598ebc0825b50941adfb7436e98a) )
	/* 40000-7ffff empty */
	ROM_LOAD( "spe_04.rom",   0x080000, 0x20000, CRC(9d7b225b) SHA1(d949c91da6ba6b82df0b3445499761a98c7e2703) )
	ROM_LOAD( "05.g2",        0x0a0000, 0x20000, CRC(4a060884) SHA1(f83d713aee4230fc04a1d5f1d4d79c64a5bf2753) )
	/* c0000-fffff empty */

	ROM_REGION( 0x040000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "spe_10.rom",   0x000000, 0x20000, CRC(eedd0ade) SHA1(f2da2eb743c68c5c9a56a94709527110cef5d91d) )	/* sprites */
	ROM_LOAD( "spe_09.rom",   0x020000, 0x20000, CRC(04b41b75) SHA1(946ed04a17f1f71085143d43905aa310ce1e05f4) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* OKIM */
	ROM_LOAD( "spe_01.rom",   0x00000, 0x20000, CRC(2d19c133) SHA1(b3ec226f35494dfc259e910895cec8a49dd2f846) )
ROM_END

ROM_START( sbbros )
	ROM_REGION( 2*0x50000, REGION_CPU1, 0 )	/* 320k for code + 320k for decrypted opcodes */
	ROM_LOAD( "06.j12",       0x00000, 0x08000, CRC(292eee6a) SHA1(d33368d2373a1ee9e24ada6aa045e0675c8e8160) )
	ROM_LOAD( "07.j13",       0x10000, 0x20000, CRC(f46b698d) SHA1(6a1867f591aa0fb9e02dd472699df93f9d018793) )
	ROM_LOAD( "08.j14",       0x30000, 0x20000, CRC(a75e7fbe) SHA1(0331d1a3e888678909f3e6d21f97896a5350e585) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "02.f1",        0x000000, 0x20000, CRC(0c22ffc6) SHA1(f95b50617ef5cd8cffffacab0b96b4bfe8dd3a1e) )	/* chars */
	ROM_LOAD( "03.f2",        0x020000, 0x20000, CRC(3ae28bc1) SHA1(4f6d9a86f624598ebc0825b50941adfb7436e98a) )
	/* 40000-7ffff empty */
	ROM_LOAD( "04.g2",        0x080000, 0x20000, CRC(bb3dee5b) SHA1(e81875b9d9a56e91daa66375b22a4fa6dcd14faa) )
	ROM_LOAD( "05.g2",        0x0a0000, 0x20000, CRC(4a060884) SHA1(f83d713aee4230fc04a1d5f1d4d79c64a5bf2753) )
	/* c0000-fffff empty */

	ROM_REGION( 0x040000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "10.l2",        0x000000, 0x20000, CRC(d6675d8f) SHA1(1c65803fcce2305841e74772ae6ffb6e39edf5c6) )	/* sprites */
	ROM_LOAD( "09.l1",        0x020000, 0x20000, CRC(8f678bc8) SHA1(66dc7c14cc012ffa9320cd63bc84977fa76ad738) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* OKIM */
	ROM_LOAD( "01.d1",        0x00000, 0x20000, CRC(b96ea126) SHA1(83fa71994518d40b8938520faa8701c63b7f579e) )
ROM_END

ROM_START( marukin )
	ROM_REGION( 2*0x30000, REGION_CPU1, 0 )	/* 192k for code + 192k for decrypted opcodes */
	ROM_LOAD( "mg3-01.9d",    0x00000, 0x08000, CRC(04357973) SHA1(61b0b347479126213c90ef6833c09537fab03093) )
	ROM_LOAD( "mg3-02.10d",   0x10000, 0x20000, CRC(50d08da0) SHA1(5d115eb646f34827d02219be3d5346f05c0c27b6) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mg3-a.3k",     0x000000, 0x80000, CRC(420f1de7) SHA1(bc2142175f93f96c45c5ee9d23da14f3eb91e58b) )	/* chars */
	ROM_LOAD( "mg3-b.4k",     0x080000, 0x80000, CRC(d8de13fa) SHA1(4420fb6fb42d40c0c84a6f4660bd0ffff429261a) )
	ROM_LOAD( "mg3-c.6k",     0x100000, 0x80000, CRC(fbeb66e8) SHA1(a9f13b3818187af05158dfea62ed46e28acf057b) )
	ROM_LOAD( "mg3-d.7k",     0x180000, 0x80000, CRC(8f6bd831) SHA1(8fe7aeab0ebe52fde269b320e9c797cb6c036eff) )

	ROM_REGION( 0x040000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mg3-05.2g",    0x000000, 0x20000, CRC(7a738d2d) SHA1(4b2daf1824b40b961c1e18050197c817fccc2337) )	/* sprites */
	ROM_LOAD( "mg3-04.1g",    0x020000, 0x20000, CRC(56f30515) SHA1(6af85c1bbebba37d3b0d4161bc2495237ddfc494) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* OKIM */
	ROM_LOAD( "mg3-e.1d",     0x00000, 0x80000, CRC(106c2fa9) SHA1(21d4579f41282dc69ea11fe2977c427543f1c69d) )	/* banked */
ROM_END

ROM_START( qtono1 )
	ROM_REGION( 2*0x50000, REGION_CPU1, 0 )	/* 320k for code + 320k for decrypted opcodes */
	ROM_LOAD( "q3-05.rom",    0x00000, 0x08000, CRC(1dd0a344) SHA1(814049bf957b78ff2d1c8da316dfe5303abee4df) )
	ROM_LOAD( "q3-06.rom",    0x10000, 0x20000, CRC(bd6a2110) SHA1(8c4d7a10dfaee0fcd18be21c80fc3d2ff9615eae) )
	ROM_LOAD( "q3-07.rom",    0x30000, 0x20000, CRC(61e53c4f) SHA1(bcde0029a217994561ae0a6fb0482bf1e3517913) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "q3-08.rom",    0x000000, 0x20000, CRC(1533b978) SHA1(586d3b93152cc78a3ae42987e66d984645cd2849) )	/* chars */
	ROM_LOAD( "q3-09.rom",    0x020000, 0x20000, CRC(a32db2f2) SHA1(df2243bff5fd44ebdfe02c5e0bbcccaff5c32628) )
	ROM_LOAD( "q3-10.rom",    0x040000, 0x20000, CRC(ed681aa8) SHA1(9f8dcebc384ca1582d509de94c194df9e3f81441) )
	ROM_LOAD( "q3-11.rom",    0x060000, 0x20000, CRC(38b2fd10) SHA1(2eee32e7c70f9f529a48d41fa886b3695228a7d3) )
	ROM_LOAD( "q3-18.rom",    0x080000, 0x20000, CRC(9e4292ac) SHA1(e1d96ef2bdb73c291734d0f8a4d7a7efbeef4fb2) )
	ROM_LOAD( "q3-19.rom",    0x0a0000, 0x20000, CRC(b7f6d40f) SHA1(40506ff901fd31a6f67ac23d2a3fdcaac5f7c8f9) )
	ROM_LOAD( "q3-20.rom",    0x0c0000, 0x20000, CRC(6cd7f38d) SHA1(cfc549331aa86a687bd9db8b3a926e490bbd4f55) )
	ROM_LOAD( "q3-21.rom",    0x0e0000, 0x20000, CRC(b4aa6b4b) SHA1(c7c771b69051fd820e9eb3faab62779b8df19209) )

	ROM_REGION( 0x040000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "q3-16.rom",    0x000000, 0x20000, CRC(863d6836) SHA1(ec78c462bb80e01f581673f2e9431efdf05599d7) )	/* sprites */
	ROM_LOAD( "q3-17.rom",    0x020000, 0x20000, CRC(459bf59c) SHA1(89975c6ff259bf68ac0c25eb0c8afb6862f11c87) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* OKIM */
	ROM_LOAD( "q3-01.rom",    0x00000, 0x20000, CRC(6c1be591) SHA1(7cab7121d78284dc95ae4218d1e7639a659dda8b) )
ROM_END

ROM_START( qsangoku )
	ROM_REGION( 2*0x50000, REGION_CPU1, 0 )	/* 320k for code + 320k for decrypted opcodes */
	ROM_LOAD( "q4-05c.rom",   0x00000, 0x08000, CRC(e1d010b4) SHA1(7fca1ee45054331320abb6a99f10fa98dd4be994) )
	ROM_LOAD( "q4-06.rom",    0x10000, 0x20000, CRC(a0301849) SHA1(60910d84f869fd5735cd5500a93b761d8b8dbacb) )
	ROM_LOAD( "q4-07.rom",    0x30000, 0x20000, CRC(2941ef5b) SHA1(a86f5365edd315fcbb2a50489d63b4be9587ae29) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "q4-08.rom",    0x000000, 0x20000, CRC(dc84c6cb) SHA1(0fb5737bb2adeddde888d24974806d4c2ac5b2ee) )	/* chars */
	ROM_LOAD( "q4-09.rom",    0x020000, 0x20000, CRC(cbb6234c) SHA1(76b749cc39d3af1d9e4959ea513ed054723ffefd) )
	ROM_LOAD( "q4-10.rom",    0x040000, 0x20000, CRC(c20a27a8) SHA1(f462babb7090b2838326bb65e2cafab0fea12f99) )
	ROM_LOAD( "q4-11.rom",    0x060000, 0x20000, CRC(4ff66aed) SHA1(0d70aae5eb930647753650486c7f7eb56239f1ad) )
	ROM_LOAD( "q4-18.rom",    0x080000, 0x20000, CRC(ca3acea5) SHA1(2aba26a7886481691097e80ec7714a7df5873630) )
	ROM_LOAD( "q4-19.rom",    0x0a0000, 0x20000, CRC(1fd92b7d) SHA1(ca4ae05c97fcdec9f7fa024f09b797391e8b3c14) )
	ROM_LOAD( "q4-20.rom",    0x0c0000, 0x20000, CRC(b02dc6a1) SHA1(78d59ef4a3f7eaa3a003765060b8367348c4cfef) )
	ROM_LOAD( "q4-21.rom",    0x0e0000, 0x20000, CRC(432b1dc1) SHA1(9beb45fe95a2ef78401d50d70eba1e683102cd39) )

	ROM_REGION( 0x040000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "q4-16.rom",    0x000000, 0x20000, CRC(77342320) SHA1(a05684f6c75a19569350d6e14eb6cb9777fb1f09) )	/* sprites */
	ROM_LOAD( "q4-17.rom",    0x020000, 0x20000, CRC(1275c436) SHA1(ed84fb07749b49066d1caf0c21e46ada94d4c213) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* OKIM */
	ROM_LOAD( "q4-01.rom",    0x00000, 0x20000, CRC(5d0d07d8) SHA1(d36e42852dd1ec0955d19b16e7dfe157b3d48522) )
ROM_END

ROM_START( block )
	ROM_REGION( 2*0x50000, REGION_CPU1, 0 )	/* 320k for code + 320k for decrypted opcodes */
	ROM_LOAD( "ble_05.bin",   0x00000, 0x08000, CRC(fa2a4536) SHA1(8f584745116bd0ced4d66719cd80c0372b797134) )
	ROM_LOAD( "ble_06.bin",   0x10000, 0x20000, CRC(58a77402) SHA1(cb24b1edd53a0965c3a9a34fe764b5c1f8dd9733) )
	ROM_LOAD( "ble_07.rom",   0x30000, 0x20000, CRC(1d114f13) SHA1(ee3588e1752b3432fd611e2d7d4fb43f942de580) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "bl_08.rom",    0x000000, 0x20000, CRC(aa0f4ff1) SHA1(58f3c468f89d834caaf66d3c084ab87addbb75c0) )	/* chars */
	ROM_RELOAD(               0x040000, 0x20000 )
	ROM_LOAD( "bl_09.rom",    0x020000, 0x20000, CRC(6fa8c186) SHA1(d4dd26d666f2accce871f70e7882e140d924dd07) )
	ROM_RELOAD(               0x060000, 0x20000 )
	ROM_LOAD( "bl_18.rom",    0x080000, 0x20000, CRC(c0acafaf) SHA1(7c44b2605da6a324d0c145202cb8bac7af7a9c68) )
	ROM_RELOAD(               0x0c0000, 0x20000 )
	ROM_LOAD( "bl_19.rom",    0x0a0000, 0x20000, CRC(1ae942f5) SHA1(e9322790db0bf2a9e862b14e166ee3f36f9ea5ad) )
	ROM_RELOAD(               0x0e0000, 0x20000 )

	ROM_REGION( 0x040000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "bl_16.rom",    0x000000, 0x20000, CRC(fadcaff7) SHA1(f4bd8e375fe6b1e6a07b4ec4e58f5807dbd738f8) )	/* sprites */
	ROM_LOAD( "bl_17.rom",    0x020000, 0x20000, CRC(5f8cab42) SHA1(3a4c682a7938479e0be80c0494c2c8fc7303b663) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* OKIM */
	ROM_LOAD( "bl_01.rom",    0x00000, 0x20000, CRC(c2ec2abb) SHA1(89981f2a887ace4c4580e2828cbdc962f89c215e) )
ROM_END

ROM_START( blocka )
	ROM_REGION( 2*0x50000, REGION_CPU1, 0 )	/* 320k for code + 320k for decrypted opcodes */
	ROM_LOAD( "ble_05.rom",   0x00000, 0x08000, CRC(c12e7f4c) SHA1(335f4eab2323b942d5feeb3bab6f7286fabfffb4) )
	ROM_LOAD( "ble_06.rom",   0x10000, 0x20000, CRC(cdb13d55) SHA1(2e4489d12a603b4c7dfb90d246ebff9176e88a0b) )
	ROM_LOAD( "ble_07.rom",   0x30000, 0x20000, CRC(1d114f13) SHA1(ee3588e1752b3432fd611e2d7d4fb43f942de580) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "bl_08.rom",    0x000000, 0x20000, CRC(aa0f4ff1) SHA1(58f3c468f89d834caaf66d3c084ab87addbb75c0) )	/* chars */
	ROM_RELOAD(               0x040000, 0x20000 )
	ROM_LOAD( "bl_09.rom",    0x020000, 0x20000, CRC(6fa8c186) SHA1(d4dd26d666f2accce871f70e7882e140d924dd07) )
	ROM_RELOAD(               0x060000, 0x20000 )
	ROM_LOAD( "bl_18.rom",    0x080000, 0x20000, CRC(c0acafaf) SHA1(7c44b2605da6a324d0c145202cb8bac7af7a9c68) )
	ROM_RELOAD(               0x0c0000, 0x20000 )
	ROM_LOAD( "bl_19.rom",    0x0a0000, 0x20000, CRC(1ae942f5) SHA1(e9322790db0bf2a9e862b14e166ee3f36f9ea5ad) )
	ROM_RELOAD(               0x0e0000, 0x20000 )

	ROM_REGION( 0x040000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "bl_16.rom",    0x000000, 0x20000, CRC(fadcaff7) SHA1(f4bd8e375fe6b1e6a07b4ec4e58f5807dbd738f8) )	/* sprites */
	ROM_LOAD( "bl_17.rom",    0x020000, 0x20000, CRC(5f8cab42) SHA1(3a4c682a7938479e0be80c0494c2c8fc7303b663) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* OKIM */
	ROM_LOAD( "bl_01.rom",    0x00000, 0x20000, CRC(c2ec2abb) SHA1(89981f2a887ace4c4580e2828cbdc962f89c215e) )
ROM_END

ROM_START( blockj )
	ROM_REGION( 2*0x50000, REGION_CPU1, 0 )	/* 320k for code + 320k for decrypted opcodes */
	ROM_LOAD( "blj_05.rom",   0x00000, 0x08000, CRC(3b55969a) SHA1(86de2f1f5878de380a8b1e3935cffa146863f07f) )
	ROM_LOAD( "ble_06.rom",   0x10000, 0x20000, CRC(cdb13d55) SHA1(2e4489d12a603b4c7dfb90d246ebff9176e88a0b) )
	ROM_LOAD( "blj_07.rom",   0x30000, 0x20000, CRC(1723883c) SHA1(e6b7575a55c045b90fb41290a60306713121acfb) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "bl_08.rom",    0x000000, 0x20000, CRC(aa0f4ff1) SHA1(58f3c468f89d834caaf66d3c084ab87addbb75c0) )	/* chars */
	ROM_RELOAD(               0x040000, 0x20000 )
	ROM_LOAD( "bl_09.rom",    0x020000, 0x20000, CRC(6fa8c186) SHA1(d4dd26d666f2accce871f70e7882e140d924dd07) )
	ROM_RELOAD(               0x060000, 0x20000 )
	ROM_LOAD( "bl_18.rom",    0x080000, 0x20000, CRC(c0acafaf) SHA1(7c44b2605da6a324d0c145202cb8bac7af7a9c68) )
	ROM_RELOAD(               0x0c0000, 0x20000 )
	ROM_LOAD( "bl_19.rom",    0x0a0000, 0x20000, CRC(1ae942f5) SHA1(e9322790db0bf2a9e862b14e166ee3f36f9ea5ad) )
	ROM_RELOAD(               0x0e0000, 0x20000 )

	ROM_REGION( 0x040000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "bl_16.rom",    0x000000, 0x20000, CRC(fadcaff7) SHA1(f4bd8e375fe6b1e6a07b4ec4e58f5807dbd738f8) )	/* sprites */
	ROM_LOAD( "bl_17.rom",    0x020000, 0x20000, CRC(5f8cab42) SHA1(3a4c682a7938479e0be80c0494c2c8fc7303b663) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* OKIM */
	ROM_LOAD( "bl_01.rom",    0x00000, 0x20000, CRC(c2ec2abb) SHA1(89981f2a887ace4c4580e2828cbdc962f89c215e) )
ROM_END

ROM_START( blockbl )
	ROM_REGION( 2*0x50000, REGION_CPU1, 0 )	/* 320k for code + 320k for decrypted opcodes */
	ROM_LOAD( "m7.l6",        0x50000, 0x08000, CRC(3b576fd9) SHA1(99cf14eba089ed9c7d9f287277dab4a8a997a9a4) )   /* Decrypted opcode + data */
	ROM_CONTINUE(             0x00000, 0x08000 )
	ROM_LOAD( "m5.l3",        0x60000, 0x20000, CRC(7c988bb7) SHA1(138ffe62ef9186849c3db73b048132ad0349ccf7) )   /* Decrypted opcode + data */
	ROM_CONTINUE(             0x10000, 0x20000 )
	ROM_LOAD( "m6.l5",        0x30000, 0x20000, CRC(5768d8eb) SHA1(6aa9bc4e778c6a06444bba0f4022710cd2abf35c) )   /* Decrypted data */

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "m12.o10",      0x000000, 0x20000, CRC(963154d9) SHA1(ef2d5bb4de3b17a2507f9656d924593edce0f3ed) )	/* chars */
	ROM_RELOAD(               0x040000, 0x20000 )
	ROM_LOAD( "m13.o14",      0x020000, 0x20000, CRC(069480bb) SHA1(f33793822848c1c3589fd2f17bbb95254ab64736) )
	ROM_RELOAD(               0x060000, 0x20000 )
	ROM_LOAD( "m4.j17",       0x080000, 0x20000, CRC(9e3b6f4f) SHA1(d129ffd1689eaa21b354dcf60b471542ff434588) )
	ROM_RELOAD(               0x0c0000, 0x20000 )
	ROM_LOAD( "m3.j20",       0x0a0000, 0x20000, CRC(629d58fe) SHA1(936ebc993f382a2cd138b6933d1bd1acd153bc01) )
	ROM_RELOAD(               0x0e0000, 0x20000 )

	ROM_REGION( 0x040000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "m11.o7",       0x000000, 0x10000, CRC(255180a5) SHA1(8fde20c6c14b84d768ebe3634584f7d4e0702548) )	/* sprites */
	ROM_LOAD( "m10.o5",       0x010000, 0x10000, CRC(3201c088) SHA1(df4f8e42eed22e67295131d2a4abf166a9ae4a6e) )
	ROM_LOAD( "m9.o3",        0x020000, 0x10000, CRC(29357fe4) SHA1(479f9a55895e2fd14ee88a65be99cf32ade1ca3d) )
	ROM_LOAD( "m8.o2",        0x030000, 0x10000, CRC(abd665d1) SHA1(a91d05ce1d5dcec2b1a933e4f5d335b05e4b3ec9) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* OKIM */
	ROM_LOAD( "bl_01.rom",    0x00000, 0x20000, CRC(c2ec2abb) SHA1(89981f2a887ace4c4580e2828cbdc962f89c215e) )
ROM_END


static void bootleg_decode(void)
{
	unsigned char *rom = memory_region(REGION_CPU1);
	int diff = memory_region_length(REGION_CPU1) / 2;

	memory_set_opcode_base(0,rom+diff);
}



static DRIVER_INIT( dokaben )
{
	input_type = 0;
	nvram_size = 0;
	mgakuen2_decode();
}
static DRIVER_INIT( pang )
{
	input_type = 0;
	nvram_size = 0;
	pang_decode();
}
static DRIVER_INIT( pangb )
{
	input_type = 0;
	nvram_size = 0;
	bootleg_decode();
}
static DRIVER_INIT( cworld )
{
	input_type = 0;
	nvram_size = 0;
	cworld_decode();
}
static DRIVER_INIT( hatena )
{
	input_type = 0;
	nvram_size = 0;
	hatena_decode();
}
static DRIVER_INIT( spang )
{
	input_type = 3;
	nvram_size = 0x80;
	nvram = &memory_region(REGION_CPU1)[0xe000];	/* NVRAM */
	spang_decode();
}
static DRIVER_INIT( sbbros )
{
	input_type = 3;
	nvram_size = 0x80;
	nvram = &memory_region(REGION_CPU1)[0xe000];	/* NVRAM */
	sbbros_decode();
}
static DRIVER_INIT( qtono1 )
{
	input_type = 0;
	nvram_size = 0;
	qtono1_decode();
}
static DRIVER_INIT( qsangoku )
{
	input_type = 0;
	nvram_size = 0;
	qsangoku_decode();
}
static DRIVER_INIT( mgakuen )
{
	input_type = 1;
}
static DRIVER_INIT( mgakuen2 )
{
	input_type = 1;
	nvram_size = 0;
	mgakuen2_decode();
}
static DRIVER_INIT( marukin )
{
	input_type = 1;
	nvram_size = 0;
	marukin_decode();
}
static DRIVER_INIT( block )
{
	input_type = 2;
	nvram_size = 0x80;
	nvram = &memory_region(REGION_CPU1)[0xff80];	/* NVRAM */
	block_decode();
}
static DRIVER_INIT( blockbl )
{
	input_type = 2;
	nvram_size = 0x80;
	nvram = &memory_region(REGION_CPU1)[0xff80];	/* NVRAM */
	bootleg_decode();
}



GAME( 1988, mgakuen,  0,        mgakuen, mgakuen,  mgakuen,  ROT0,   "Yuga", "Mahjong Gakuen" )
GAME( 1988, 7toitsu,  mgakuen,  mgakuen, mgakuen,  mgakuen,  ROT0,   "Yuga", "Chi-Toitsu" )
GAME( 1989, mgakuen2, 0,        marukin, marukin,  mgakuen2, ROT0,   "Face", "Mahjong Gakuen 2 Gakuen-chou no Fukushuu" )
GAME( 1989, pkladies, 0,        marukin, pkladies, mgakuen2, ROT0,   "Mitchell", "Poker Ladies" )
GAME( 1989, pkladiel, pkladies, marukin, pkladies, mgakuen2, ROT0,   "Leprechaun", "Poker Ladies (Leprechaun)" )
GAME( 1989, dokaben,  0,        pang,    pang,     dokaben,  ROT0,   "Capcom", "Dokaben (Japan)" )
GAME( 1989, pang,     0,        pang,    pang,     pang,     ROT0,   "Mitchell", "Pang (World)" )
GAME( 1989, pangb,    pang,     pang,    pang,     pangb,    ROT0,   "bootleg", "Pang (bootleg)" )
GAME( 1989, bbros,    pang,     pang,    pang,     pang,     ROT0,   "Capcom", "Buster Bros. (US)" )
GAME( 1989, pompingw, pang,     pang,    pang,     pang,     ROT0,   "Mitchell", "Pomping World (Japan)" )
GAME( 1989, cworld,   0,        pang,    qtono1,   cworld,   ROT0,   "Capcom", "Capcom World (Japan)" )
GAME( 1990, hatena,   0,        pang,    qtono1,   hatena,   ROT0,   "Capcom", "Adventure Quiz 2 Hatena Hatena no Dai-Bouken (Japan)" )
GAME( 1990, spang,    0,        pang,    pang,     spang,    ROT0,   "Mitchell", "Super Pang (World)" )
GAME( 1990, sbbros,   spang,    pang,    pang,     sbbros,   ROT0,   "Mitchell + Capcom", "Super Buster Bros. (US)" )
GAME( 1990, marukin,  0,        marukin, marukin,  marukin,  ROT0,   "Yuga", "Super Marukin-Ban" )
GAME( 1991, qtono1,   0,        pang,    qtono1,   qtono1,   ROT0,   "Capcom", "Quiz Tonosama no Yabou (Japan)" )
GAME( 1991, qsangoku, 0,        pang,    qtono1,   qsangoku, ROT0,   "Capcom", "Quiz Sangokushi (Japan)" )
GAME( 1991, block,    0,        pang,    blockj,   block,    ROT270, "Capcom", "Block Block (World 911106 Joystick)" )
GAME( 1991, blocka,   block,    pang,    block,    block,    ROT270, "Capcom", "Block Block (World 910910)" )
GAME( 1991, blockj,   block,    pang,    block,    block,    ROT270, "Capcom", "Block Block (Japan 910910)" )
GAME( 1991, blockbl,  block,    pang,    block,    blockbl,  ROT270, "bootleg", "Block Block (bootleg)" )
