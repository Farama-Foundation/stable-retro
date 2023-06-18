/***************************************************************************

Big Twins
World Beach Volley

driver by Nicola Salmoria

The games run on different, but similar, hardware. The sprite system is the
same (almost - the tile size is different).

Even if the two games are from the same year, World Beach Volley is much more
advanced - more colourful, and stores setting in an EEPROM.

An interesting thing about this hardware is that the same gfx ROMs are used
to generate both 8x8 and 16x16 tiles for different tilemaps.


TODO:
Big Twins:
- The pixel bitmap might be larger than what I handle, or the vertical scroll
  register has an additional meaning. The usual scroll value is 0x7f0, the game
  is setting it to 0x5f0 while updating the bitmap, so this should either scroll
  the changing region out of view, or disable it. During gameplay, the image
  that scrolls down might have to be different since the scroll register is in
  the range 0x600-0x6ff.
  As it is handled now, it is certainly wrong because after game over the
  bitmap is left on screen.

World Beach Volley:
- sprite/tile priority issue during attract mode (plane should go behind palm)
- The histogram functions don't seem to work.
- Sound is controlled by a pic16c57 whose ROM is missing for this game.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/eeprom.h"
#include "cpu/pic16c5x/pic16c5x.h"


static data16_t playmark_snd_command;
static data16_t playmark_snd_flag;
static data8_t playmark_oki_control;
static data8_t playmark_oki_command;


extern data16_t *bigtwin_bgvideoram;
/*extern size_t bigtwin_bgvideoram_size; */
extern data16_t *wbeachvl_videoram1,*wbeachvl_videoram2,*wbeachvl_videoram3;
extern data16_t *wbeachvl_rowscroll;

VIDEO_START( bigtwin );
VIDEO_START( wbeachvl );
VIDEO_START( hrdtimes );
WRITE16_HANDLER( wbeachvl_txvideoram_w );
WRITE16_HANDLER( wbeachvl_fgvideoram_w );
WRITE16_HANDLER( wbeachvl_bgvideoram_w );
WRITE16_HANDLER( hrdtimes_txvideoram_w );
WRITE16_HANDLER( hrdtimes_fgvideoram_w );
WRITE16_HANDLER( hrdtimes_bgvideoram_w );
WRITE16_HANDLER( bigtwin_paletteram_w );
WRITE16_HANDLER( bigtwin_bgvideoram_w );
WRITE16_HANDLER( bigtwin_scroll_w );
WRITE16_HANDLER( wbeachvl_scroll_w );
WRITE16_HANDLER( hrdtimes_scroll_w );
VIDEO_UPDATE( bigtwin );
VIDEO_UPDATE( wbeachvl );
VIDEO_UPDATE( hrdtimes );



static WRITE16_HANDLER( coinctrl_w )
{
	if (ACCESSING_MSB)
	{
		coin_counter_w(0,data & 0x0100);
		coin_counter_w(1,data & 0x0200);
	}
	if (data & 0xfcff)
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Writing %04x to unknown coin control bits\n",data);
}


/***************************************************************************

  EEPROM

***************************************************************************/

static struct EEPROM_interface eeprom_interface =
{
	6,				/* address bits */
	16,				/* data bits */
	"*110",			/*  read command */
	"*101",			/* write command */
	0,				/* erase command */
	"*10000xxxx",	/* lock command */
	"*10011xxxx",	/* unlock command */
	0,				/* enable_multi_read */
	5				/* reset_delay (otherwise wbeachvl will hang when saving settings) */
};

static NVRAM_HANDLER( wbeachvl )
{
	if (read_or_write)
	{
		EEPROM_save(file);
	}
	else
	{
		EEPROM_init(&eeprom_interface);

		if (file)
			EEPROM_load(file);
		else
		{
			UINT8 init[128];
			memset(init,0,128);
			EEPROM_set_data(init,128);
		}
	}
}

static READ16_HANDLER( wbeachvl_port0_r )
{
	int bit;

	bit = EEPROM_read_bit() << 7;

	return (input_port_0_r(0) & 0x7f) | bit;
}

static WRITE16_HANDLER( wbeachvl_coin_eeprom_w )
{
	if (ACCESSING_LSB)
	{
		/* bits 0-3 are coin counters? (only 0 used?) */
		coin_counter_w(0,data & 0x01);
		coin_counter_w(1,data & 0x02);
		coin_counter_w(2,data & 0x04);
		coin_counter_w(3,data & 0x08);

		/* bits 5-7 control EEPROM */
		EEPROM_set_cs_line((data & 0x20) ? CLEAR_LINE : ASSERT_LINE);
		EEPROM_write_bit(data & 0x80);
		EEPROM_set_clock_line((data & 0x40) ? CLEAR_LINE : ASSERT_LINE);
	}
}

static WRITE16_HANDLER( hrdtimes_coin_w )
{
	coin_counter_w(0,data & 0x01);
	coin_counter_w(1,data & 0x02);
}

static WRITE16_HANDLER( playmark_snd_command_w )
{
	if (ACCESSING_LSB) {
		playmark_snd_command = (data & 0xff);
		playmark_snd_flag = 1;
		cpu_yield();
	}
}

static READ_HANDLER( playmark_snd_command_r )
{
	int data = 0;

	if ((playmark_oki_control & 0x38) == 0x30) {
		data = playmark_snd_command;
/*		log_cb(RETRO_LOG_DEBUG, LOGPRE "PortB reading %02x from the 68K\n",data); */
	}
	else if ((playmark_oki_control & 0x38) == 0x28) {
		data = (OKIM6295_status_0_r(0) & 0x0f);
/*		log_cb(RETRO_LOG_DEBUG, LOGPRE "PortB reading %02x from the OKI status port\n",data);*/
	}

	return data;
}

static READ_HANDLER( playmark_snd_flag_r )
{
	if (playmark_snd_flag) {
		playmark_snd_flag = 0;
		return 0x00;
	}

	return 0x40;
}


static WRITE_HANDLER( playmark_oki_banking_w )
{
	static int old_bank = 0;

	if(old_bank != (data & 7))
	{
		old_bank = data & 7;

		if(((old_bank - 1) * 0x40000) < memory_region_length(REGION_SOUND1))
		{
			OKIM6295_set_bank_base(0, 0x40000 * (old_bank - 1));
		}
	}
}

static WRITE_HANDLER( playmark_oki_w )
{
	playmark_oki_command = data;
}

static WRITE_HANDLER( playmark_snd_control_w )
{
	/*	This port controls communications to and from the 68K, and the OKI
		device.

		bit legend
		7w  ???  (No read or writes to Port B)
		6r  Flag from 68K to notify the PIC that a command is coming
		5w  Latch write data to OKI? (active low)
		4w  Activate read signal to OKI? (active low)
		3w  Set Port 1 to read sound to play command from 68K. (active low)
		2w  ???  (Read Port B)
		1   Not used
		0   Not used
	*/

	playmark_oki_control = data;

	if ((data & 0x38) == 0x18)
	{
/*		log_cb(RETRO_LOG_DEBUG, LOGPRE "Writing %02x to OKI1, PortC=%02x, Code=%02x\n",playmark_oki_command,playmark_oki_control,playmark_snd_command);*/
		OKIM6295_data_0_w(0, playmark_oki_command);
	}
}


static WRITE_HANDLER( hrdtimes_snd_control_w )
{
	/*  This port controls communications to and from the 68K and the OKI device. See playmark_snd_control_w above. OKI banking is also handled here. */
	
	static int old_bank = 0;

	if(old_bank != (data & 3))
	{
		old_bank = data & 3;

		if(((old_bank) * 0x40000) < memory_region_length(REGION_SOUND1))
		{
			OKIM6295_set_bank_base(0, 0x40000 * (old_bank));
		}
	}

	playmark_oki_control = data;

	if ((data & 0x38) == 0x18)
	{
/*		logerror("Writing %02x to OKI1, PortC=%02x, Code=%02x\n",playmark_oki_command,playmark_oki_control,playmark_snd_command); */
		OKIM6295_data_0_w(0, playmark_oki_command);
	}
}


static READ_HANDLER( PIC16C5X_T0_clk_r )
{
	return 0;
}


/***************************** 68000 Memory Maps ****************************/

static MEMORY_READ16_START( bigtwin_readmem )
    { 0x000000, 0x0fffff, MRA16_RAM },
	{ 0x304000, 0x304001, MRA16_NOP	},			/* watchdog? irq ack? */
	{ 0x440000, 0x4403ff, MRA16_RAM },
	{ 0x600000, 0x67ffff, MRA16_RAM },
	{ 0x700010, 0x700011, input_port_0_word_r },
	{ 0x700012, 0x700013, input_port_1_word_r },
	{ 0x700014, 0x700015, input_port_2_word_r },
	{ 0x70001a, 0x70001b, input_port_3_word_r },
	{ 0x70001c, 0x70001d, input_port_4_word_r },
	{ 0xff0000, 0xffffff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( bigtwin_writemem )
    { 0x000000, 0x0fffff, MWA16_RAM },
	{ 0x304000, 0x304001, MWA16_NOP	},			/* watchdog? irq ack? */
	{ 0x440000, 0x4403ff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x500000, 0x500fff, wbeachvl_fgvideoram_w, &wbeachvl_videoram2 },
	{ 0x501000, 0x501fff, MWA16_NOP },	/* unused RAM? */
	{ 0x502000, 0x503fff, wbeachvl_txvideoram_w, &wbeachvl_videoram1 },
	{ 0x504000, 0x50ffff, MWA16_NOP },	/* unused RAM? */
	{ 0x510000, 0x51000b, bigtwin_scroll_w },
	{ 0x51000c, 0x51000d, MWA16_NOP },	/* always 3? */
	{ 0x600000, 0x67ffff, MWA16_RAM, &bigtwin_bgvideoram },
	{ 0x700016, 0x700017, coinctrl_w },
	{ 0x70001e, 0x70001f, playmark_snd_command_w },
	{ 0x780000, 0x7807ff, bigtwin_paletteram_w, &paletteram16 },
	{ 0xff0000, 0xffffff, MWA16_RAM },
MEMORY_END

static MEMORY_READ16_START( wbeachvl_readmem )
    { 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x440000, 0x440fff, MRA16_RAM },
    { 0x500000, 0x501fff, MRA16_RAM },
	{ 0x504000, 0x505fff, MRA16_RAM },
	{ 0x508000, 0x509fff, MRA16_RAM },
	{ 0x50f000, 0x50ffff, MRA16_RAM },
	{ 0x710010, 0x710011, wbeachvl_port0_r },
	{ 0x710012, 0x710013, input_port_1_word_r },
	{ 0x710014, 0x710015, input_port_2_word_r },
	{ 0x710018, 0x710019, input_port_3_word_r },
	{ 0x71001a, 0x71001b, input_port_4_word_r },
	{ 0xff0000, 0xffffff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( wbeachvl_writemem )
    { 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x440000, 0x440fff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x500000, 0x501fff, wbeachvl_bgvideoram_w, &wbeachvl_videoram3 },
	{ 0x504000, 0x505fff, wbeachvl_fgvideoram_w, &wbeachvl_videoram2 },
	{ 0x508000, 0x509fff, wbeachvl_txvideoram_w, &wbeachvl_videoram1 },
	{ 0x50f000, 0x50ffff, MWA16_RAM, &wbeachvl_rowscroll },
	{ 0x510000, 0x51000b, wbeachvl_scroll_w },
	{ 0x51000c, 0x51000d, MWA16_NOP	}, /* 2 and 3 */
	{ 0x710016, 0x710017, wbeachvl_coin_eeprom_w },
/* disabled sound calls for now	 */
/*    { 0x71001e, 0x71001f, playmark_snd_command_w }, */
	{ 0x780000, 0x780fff, paletteram16_RRRRRGGGGGBBBBBx_word_w, &paletteram16 },
	{ 0xff0000, 0xffffff, MWA16_RAM },
MEMORY_END

static data16_t *hrdtimes_rom; /* trampoline */

static MEMORY_READ16_START( hrdtimes_readmem )
    { 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x080000, 0x0bffff, MRA16_RAM },
	{ 0x0c0000, 0x0fffff, MRA16_ROM }, /*AM_REGION(REGION_CPU1, 0x0c0000) */
	{ 0x100000, 0x103fff, MRA16_RAM },
	{ 0x104000, 0x107fff, MRA16_RAM },
	{ 0x108000, 0x10ffff, MRA16_RAM },
	{ 0x10c000, 0x10ffff, MRA16_RAM }, /* Unused */
	{ 0x200000, 0x200fff, MRA16_RAM },
	{ 0x280000, 0x2807ff, MRA16_RAM },
	{ 0x280800, 0x280fff, MRA16_RAM }, /* unused */
	{ 0x300010, 0x300011, input_port_0_word_r },
	{ 0x300012, 0x300013, input_port_1_word_r },
	{ 0x300014, 0x300015, input_port_2_word_r },
	{ 0x30001a, 0x30001b, input_port_3_word_r },
	{ 0x30001c, 0x30001d, input_port_4_word_r },
MEMORY_END

static MEMORY_WRITE16_START( hrdtimes_writemem )
    { 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x080000, 0x0bffff, MWA16_RAM },
	{ 0x0c0000, 0x0fffff, MWA16_ROM, &hrdtimes_rom }, /* correct.?? AM_REGION(REGION_CPU1, 0x0c0000) */
	{ 0x100000, 0x103fff, hrdtimes_bgvideoram_w, &wbeachvl_videoram3 },
	{ 0x104000, 0x107fff, hrdtimes_fgvideoram_w, &wbeachvl_videoram2 },
	{ 0x108000, 0x10ffff, hrdtimes_txvideoram_w, &wbeachvl_videoram1 },
	{ 0x10c000, 0x10ffff, MWA16_RAM }, /* Unused */
	{ 0x110000, 0x11000d, hrdtimes_scroll_w },
	{ 0x200000, 0x200fff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x280000, 0x2807ff, bigtwin_paletteram_w, &paletteram16 },
	{ 0x280800, 0x280fff, MWA16_RAM },/* unused */
	{ 0x300016, 0x300017, hrdtimes_coin_w },
/* disable sound command for now	 */
/*	{ 0x30001e, 0x30001f, playmark_snd_command_w }, */
	{ 0x304000, 0x304001, MWA16_NOP	},	/* watchdog? irq ack? */
MEMORY_END

static MEMORY_READ_START( playmark_sound_readmem )
	{ PIC16C57_MEMORY_READ },
		/* $000 - 07F  Internal memory mapped registers */
		/* $000 - 7FF  Program ROM for PIC16C57. Note: code is 12bits wide */
		/*             View the ROM at $1000 in the debugger memory windows */
MEMORY_END

static MEMORY_WRITE_START( playmark_sound_writemem )
	{ PIC16C57_MEMORY_WRITE },
MEMORY_END

static PORT_READ_START( playmark_sound_readport )
	{ 0x00, 0x00, IORP_NOP },				/* 4 bit port */
	{ 0x01, 0x01, playmark_snd_command_r },
	{ 0x02, 0x02, playmark_snd_flag_r },
	{ PIC16C5x_T0, PIC16C5x_T0, PIC16C5X_T0_clk_r },
PORT_END

static PORT_WRITE_START( playmark_sound_writeport )
    { 0x00, 0x00, playmark_oki_banking_w },	/* 4 bit port */
	{ 0x01, 0x01, playmark_oki_w },
	{ 0x02, 0x02, playmark_snd_control_w },
PORT_END

static PORT_READ_START( hrdtimes_sound_readport )
	{ 0x00, 0x00, IORP_NOP },				/* 4 bit port */
	{ 0x01, 0x01, playmark_snd_command_r },
	{ 0x02, 0x02, playmark_snd_flag_r },
	{ PIC16C5x_T0, PIC16C5x_T0, PIC16C5X_T0_clk_r },
PORT_END

static PORT_WRITE_START( hrdtimes_sound_writeport )
/*	{ 0x00, 0x00, playmark_oki_banking_w },*/				/* 4 bit port */
	{ 0x01, 0x01, playmark_oki_w },
	{ 0x02, 0x02, hrdtimes_snd_control_w }, /* OKI banking via this port */
PORT_END


INPUT_PORTS_START( bigtwin )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_DIPNAME( 0x01, 0x00, "Language" )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPSETTING(    0x01, "Italian" )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "Censor Pictures" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Coin Mode" )
	PORT_DIPSETTING(    0x01, "Mode 1" )
	PORT_DIPSETTING(    0x00, "Mode 2" )
	/* TODO: support coin mode 2 */
	PORT_DIPNAME( 0x1e, 0x1e, "Coinage Mode 1" )
	PORT_DIPSETTING(    0x14, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x16, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x1a, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 8C_3C ) )
	PORT_DIPSETTING(    0x1c, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 5C_3C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x1e, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x12, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
#if 0
	PORT_DIPNAME( 0x06, 0x06, "Coin A Mode 2" )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x18, 0x18, "Coin B Mode 2" )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )
#endif
	PORT_DIPNAME( 0x20, 0x20, "Minimum Credits to Start" )
	PORT_DIPSETTING(    0x20, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( wbeachvl )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* ?? see code at 746a. sound status? */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* EEPROM data */

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START3 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER4 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START4 )
INPUT_PORTS_END

INPUT_PORTS_START( hrdtimes )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x0c, "Every 300k - 500k" )
	PORT_DIPSETTING(    0x08, "Every 500k - 500k" )
	PORT_DIPSETTING(    0x04, "Only 500k" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x20, "Easy" )
	PORT_DIPSETTING(    0x30, "Normal" )
	PORT_DIPSETTING(    0x10, "Hard" )
	PORT_DIPSETTING(    0x00, "Very_Hard" )
	PORT_DIPNAME( 0x40, 0x40, "Allow_Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Coin Mode" )
	PORT_DIPSETTING(    0x01, "Mode 1" )
	PORT_DIPSETTING(    0x00, "Mode 2" )
	/* TODO: support coin mode 2 */
	PORT_DIPNAME( 0x1e, 0x1e, "Coinage Mode 1" )
	PORT_DIPSETTING(    0x14, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x16, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x1a, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 8C_3C ) )
	PORT_DIPSETTING(    0x1c, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 5C_3C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x1e, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x12, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
#if 0
	PORT_DIPNAME( 0x06, 0x06, "Coin A Mode 2" )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x18, 0x18, "Coin B Mode 2" )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )
#endif
	PORT_DIPNAME( 0x20, 0x20, "Minimum Credits to Start" )
	PORT_DIPSETTING(    0x20, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPNAME( 0x40, 0x40, "1 Life If Continue" )
	PORT_DIPSETTING(    0x40, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )
INPUT_PORTS_END


static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(3,4), RGN_FRAC(2,4), RGN_FRAC(1,4), RGN_FRAC(0,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	32*8
};


static struct GfxLayout hrdtimes_charlayout =
{
	8,8,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(3,4), RGN_FRAC(2,4), RGN_FRAC(1,4), RGN_FRAC(0,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};


static struct GfxLayout tilelayout =
{
	16,16,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(3,4), RGN_FRAC(2,4), RGN_FRAC(1,4), RGN_FRAC(0,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8
};

static struct GfxLayout spritelayout =
{
	32,32,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(3,4), RGN_FRAC(2,4), RGN_FRAC(1,4), RGN_FRAC(0,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7,
			32*8+0, 32*8+1, 32*8+2, 32*8+3, 32*8+4, 32*8+5, 32*8+6, 32*8+7,
			48*8+0, 48*8+1, 48*8+2, 48*8+3, 48*8+4, 48*8+5, 48*8+6, 48*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8,
			64*8, 65*8, 66*8, 67*8, 68*8, 69*8, 70*8, 71*8,
			72*8, 73*8, 74*8, 75*8, 76*8, 77*8, 78*8, 79*8 },
	128*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0, &spritelayout, 0x200, 16 },	/* colors 0x200-0x2ff */
	{ REGION_GFX1, 0, &tilelayout,   0x000,  8 },	/* colors 0x000-0x07f */
	{ REGION_GFX1, 0, &charlayout,   0x080,  8 },	/* colors 0x080-0x0ff */
	/* background bitmap uses colors 0x100-0x1ff */
	{ -1 } /* end of array */
};


static struct GfxLayout wcharlayout =
{
	8,8,
	RGN_FRAC(1,6),
	6,
	{ RGN_FRAC(5,6), RGN_FRAC(4,6), RGN_FRAC(3,6), RGN_FRAC(2,6), RGN_FRAC(1,6), RGN_FRAC(0,6) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static struct GfxLayout wtilelayout =
{
	16,16,
	RGN_FRAC(1,6),
	6,
	{ RGN_FRAC(5,6), RGN_FRAC(4,6), RGN_FRAC(3,6), RGN_FRAC(2,6), RGN_FRAC(1,6), RGN_FRAC(0,6) },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8
};

/* tiles are 6 bpp, sprites only 5bpp */
static struct GfxLayout wspritelayout =
{
	16,16,
	RGN_FRAC(1,6),
	5,
	{ RGN_FRAC(4,6), RGN_FRAC(3,6), RGN_FRAC(2,6), RGN_FRAC(1,6), RGN_FRAC(0,6) },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8
};

static struct GfxDecodeInfo wbeachvl_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &wspritelayout, 0x600, 16 },	/* colors 0x600-0x7ff */
	{ REGION_GFX1, 0, &wtilelayout,   0x000, 16 },	/* colors 0x000-0x3ff */
	{ REGION_GFX1, 0, &wcharlayout,   0x400,  8 },	/* colors 0x400-0x5ff */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo hrdtimes_gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0, &tilelayout,  0x200, 32 },	/* colors 0x200-0x2ff */
	{ REGION_GFX1, 0, &tilelayout,  0x000, 16 },	/* colors 0x000-0x0ff */
	{ REGION_GFX1, 0, &hrdtimes_charlayout,  0x100,  8 },	/* colors 0x100-0x17f */
	{ -1 } /* end of array */
};




static struct OKIM6295interface okim6295_interface =
{
	1,						/* 1 chip */
	{ 32000000/32/132 },	/* 7575Hz frequency? */
	{ REGION_SOUND1 },		/* memory region */
	{ 100 }
};


static struct OKIM6295interface hrdtimes_okim6295_interface =
{
	1,		                /* 1 chip */
	{ 1000000/132 },	    /* frequency */
	{ REGION_SOUND1 },		/* memory region */
	{ 100 }
};

static MACHINE_DRIVER_START( bigtwin )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)	/* 12 MHz? */
	MDRV_CPU_MEMORY(bigtwin_readmem,bigtwin_writemem)
	MDRV_CPU_VBLANK_INT(irq2_line_hold,1)

    MDRV_CPU_ADD(PIC16C57, (12000000/PIC16C5x_CLOCK_DIVIDER))	/* 3MHz */
	MDRV_CPU_MEMORY(playmark_sound_readmem,playmark_sound_writemem)
	MDRV_CPU_PORTS(playmark_sound_readport,playmark_sound_writeport)

	MDRV_FRAMES_PER_SECOND(50)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 64*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 2*8, 32*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(bigtwin)
	MDRV_VIDEO_UPDATE(bigtwin)

	/* sound hardware */
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( wbeachvl )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)	/* 12 MHz? */
	MDRV_CPU_MEMORY(wbeachvl_readmem,wbeachvl_writemem)
	MDRV_CPU_VBLANK_INT(irq2_line_hold,1)

/* disabled sound cpu as game fails to boot with it active */
/*    MDRV_CPU_ADD(PIC16C57, (12000000/PIC16C5x_CLOCK_DIVIDER))*/	/* 3MHz */ 
	/* Program and Data Maps are internal to the MCU */
/* 	MDRV_CPU_MEMORY(playmark_sound_readmem,playmark_sound_writemem) */
/*	MDRV_CPU_PORTS(playmark_sound_readport,playmark_sound_writeport) */

	MDRV_FRAMES_PER_SECOND(58)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(wbeachvl)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 64*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 2*8, 32*8-1)
	MDRV_GFXDECODE(wbeachvl_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(wbeachvl)
	MDRV_VIDEO_UPDATE(wbeachvl)

	/* sound hardware */
	MDRV_SOUND_ADD(OKIM6295, hrdtimes_okim6295_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( hrdtimes )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)	/* 12 MHz */
	MDRV_CPU_MEMORY(hrdtimes_readmem,hrdtimes_writemem)
	MDRV_CPU_VBLANK_INT(irq6_line_hold,1)

/* disable sound cpu for now */
/*    MDRV_CPU_ADD(PIC16C57, (12000000/PIC16C5x_CLOCK_DIVIDER))*/	/* 3MHz */
	/* Program and Data Maps are internal to the MCU */
/*    MDRV_CPU_MEMORY(playmark_sound_readmem,playmark_sound_writemem) */
/*	MDRV_CPU_PORTS(hrdtimes_sound_readport,hrdtimes_sound_writeport) */

	MDRV_FRAMES_PER_SECOND(58)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 64*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(hrdtimes_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(hrdtimes)
	MDRV_VIDEO_UPDATE(hrdtimes)

	/* sound hardware */
	MDRV_SOUND_ADD(OKIM6295, hrdtimes_okim6295_interface)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( bigtwin )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "2.302",        0x000000, 0x80000, CRC(e6767f60) SHA1(ec0ba1c786e6fde04601c2f3f619e3c6545f9239) )
	ROM_LOAD16_BYTE( "3.301",        0x000001, 0x80000, CRC(5aba6990) SHA1(4f664a91819fdd27821fa607425701d83fcbd8ce) )

	ROM_REGION( 0x4000, REGION_CPU2, 0 )	/* sound (PIC16C57) */
/*	ROM_LOAD( "16c57hs.bin",  PIC16C57_PGM_OFFSET, 0x1000, CRC(b4c95cc3) SHA1(7fc9b141e7782aa5c17310ee06db99d884537c30) )*/
	/* ROM will be copied here by the init code from REGION_USER1 */

	ROM_REGION( 0x3000, REGION_USER1, ROMREGION_DISPOSE )
	ROM_LOAD( "16c57hs.015",  0x0000, 0x2d4c, CRC(c07e9375) SHA1(7a6714ab888ea6e37bc037bc7419f0998868cfce) )	/* 16C57 .HEX dump, to be converted */

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "4.311",        0x00000, 0x40000, CRC(6f628fbc) SHA1(51cdee457aef79fef5d89d30a173afdf13fbb2ef) )
	ROM_LOAD( "5.312",        0x40000, 0x40000, CRC(6a9b1752) SHA1(7c78157cd6b3d631704d2aca1a5756c69c87d581) )
	ROM_LOAD( "6.313",        0x80000, 0x40000, CRC(411cf852) SHA1(1b66cec672b6ec6974d9e82afc6ec58b78c92ee4) )
	ROM_LOAD( "7.314",        0xc0000, 0x40000, CRC(635c81fd) SHA1(64c787a37fcd1ba7c747ec25ff5b949aad3914ec) )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "8.321",        0x00000, 0x20000, CRC(2749644d) SHA1(f506ed1a14ee411eda8a7e639f5572e35b89b13f) )
	ROM_LOAD( "9.322",        0x20000, 0x20000, CRC(1d1897af) SHA1(0ad00906b94443d7ceef383717b39c6aa8cca241) )
	ROM_LOAD( "10.323",       0x40000, 0x20000, CRC(2a03432e) SHA1(44722b83093211d88460cbcd9e9c0b638d24ad3e) )
	ROM_LOAD( "11.324",       0x60000, 0x20000, CRC(2c980c4c) SHA1(77af29a1f5d4302650915f4a7daf2918a2519a6e) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "1.013",        0x00000, 0x40000, CRC(ff6671dc) SHA1(517941946a3edfc2da0b7aa8a106ebb4ae849beb) )
ROM_END

ROM_START( wbeachvl )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "wbv_02.bin",   0x000000, 0x40000, CRC(c7cca29e) SHA1(03af361081d688c4204a95f7f5babcc598b72c23) )
	ROM_LOAD16_BYTE( "wbv_03.bin",   0x000001, 0x40000, CRC(db4e69d5) SHA1(119bf35a463d279ddde67ab08f6f1bab9f05cf0c) )

/*	ROM_REGION( 0x1009, REGION_CPU2, 0 )*/ /* sound (PIC16C57) */
	/* 0x1000 rom data (actually 0x800 12-bit words), + 0x9 config bytes */
/*	ROM_LOAD( "pic16c57",   0x0000, 0x1009, CRC(35439064) SHA1(ab0c5bafd76a2cb2a2e5ddb9d0578fd7e2241e43) ) */

	ROM_REGION( 0x600000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "wbv_10.bin",   0x000000, 0x80000, CRC(50680f0b) SHA1(ed76ef6ced70ba7e9558162aa94bbe9f19bbabe6) )
	ROM_LOAD( "wbv_04.bin",   0x080000, 0x80000, CRC(df9cbff1) SHA1(7197939d9c4e8666d37266b6326134cfb4c761da) )
	ROM_LOAD( "wbv_11.bin",   0x100000, 0x80000, CRC(e59ad0d1) SHA1(70dfc1ea45246fc8e24c96550563ab7a983f3824) )
	ROM_LOAD( "wbv_05.bin",   0x180000, 0x80000, CRC(51245c3c) SHA1(5ac27d6fc22555766b4cdd532210199f4d7bd8bb) )
	ROM_LOAD( "wbv_12.bin",   0x200000, 0x80000, CRC(36b87d0b) SHA1(702b8139d150c7cc9399dfa38536567aab40dcef) )
	ROM_LOAD( "wbv_06.bin",   0x280000, 0x80000, CRC(9eb808ef) SHA1(0e46557665f1acef0606f22f043a391d1086cfce) )
	ROM_LOAD( "wbv_13.bin",   0x300000, 0x80000, CRC(7021107b) SHA1(088fe3060dbb196e8000a3b4db1cfa3cb0c4b677) )
	ROM_LOAD( "wbv_07.bin",   0x380000, 0x80000, CRC(4fff9fe8) SHA1(e29d3b4895692fd8559c9018432f32170aecdcc3) )
	ROM_LOAD( "wbv_14.bin",   0x400000, 0x80000, CRC(0595e675) SHA1(82aebaedc919fa51b71f5519ee765ce9953d613a) )
	ROM_LOAD( "wbv_08.bin",   0x480000, 0x80000, CRC(07e4b416) SHA1(a780ef0bd11897ab437359985f6e4852030ddbbf) )
	ROM_LOAD( "wbv_15.bin",   0x500000, 0x80000, CRC(4e1a82d2) SHA1(9e66b52ba8e8144f772183396fc1a2fbb37ed2bc) )
	ROM_LOAD( "wbv_09.bin",   0x580000, 0x20000, CRC(894ce354) SHA1(331aeabbe10cd645776da2dc0829acc2275e72dc) )
	/* 5a0000-5fffff is empty */

	ROM_REGION( 0x100000, REGION_USER2, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "wbv_01.bin",   0x00000, 0x100000, CRC(ac33f25f) SHA1(5d9ed16650aeb297d565376a99b31c88ab611668) )
	
	/* $00000-$20000 stays the same in all sound banks, */
	/* the second half of the bank is what gets switched */
	ROM_REGION( 0x1c0000, REGION_SOUND1, 0 ) /* Samples */
	ROM_COPY( REGION_USER2, 0x000000, 0x000000, 0x020000)
	ROM_COPY( REGION_USER2, 0x020000, 0x020000, 0x020000)
	ROM_COPY( REGION_USER2, 0x000000, 0x040000, 0x020000)
	ROM_COPY( REGION_USER2, 0x040000, 0x060000, 0x020000)
	ROM_COPY( REGION_USER2, 0x000000, 0x080000, 0x020000)
	ROM_COPY( REGION_USER2, 0x060000, 0x0a0000, 0x020000)
	ROM_COPY( REGION_USER2, 0x000000, 0x0c0000, 0x020000)
	ROM_COPY( REGION_USER2, 0x080000, 0x0e0000, 0x020000)
	ROM_COPY( REGION_USER2, 0x000000, 0x100000, 0x020000)
	ROM_COPY( REGION_USER2, 0x0a0000, 0x120000, 0x020000)
	ROM_COPY( REGION_USER2, 0x000000, 0x140000, 0x020000)
	ROM_COPY( REGION_USER2, 0x0c0000, 0x160000, 0x020000)
	ROM_COPY( REGION_USER2, 0x000000, 0x180000, 0x020000)
	ROM_COPY( REGION_USER2, 0x0e0000, 0x1a0000, 0x020000)
ROM_END

ROM_START( hrdtimes )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "31.u67",       0x00000, 0x80000, CRC(53eb041b) SHA1(7437da1ceb26e9518a3085560b8a42f37e77ace9) )
	ROM_LOAD16_BYTE( "32.u66",       0x00001, 0x80000, CRC(f2c6b382) SHA1(d73affed091a261c4bfe17f409657e0a46b6c163) )

/*	ROM_REGION( 0x4000, REGION_CPU2,  ROMREGION_ERASE00 ) */
/*	ROM_LOAD( "pic16c57.bin", PIC16C57_PGM_OFFSET, 0x1000, CRC(db307198) SHA1(21e98a69e673f6d48eb48239b4c51f6e7aa19a66) )*/  /* PIC CPU dump */
	
	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "33.u36",       0x000000, 0x80000, CRC(d1239ce5) SHA1(8e966a39a47f66c5e904ec4357c751e896ed47cb) )
	ROM_LOAD( "37.u42",       0x080000, 0x80000, CRC(aa692005) SHA1(1e274da358a25ceebdc71cb8f7228ef39348a895) )
	ROM_LOAD( "34.u39",       0x100000, 0x80000, CRC(e4108c59) SHA1(15f7b53a7bbdc4aefdae31a00be64c419326bfd1) )
	ROM_LOAD( "38.u45",       0x180000, 0x80000, CRC(ff7cacf3) SHA1(5ed93e86fe3b0b594bdd62e314cd9e2ffd3c2a2a) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "36.u86",       0x000000, 0x80000, CRC(f2fc1ca3) SHA1(f70913d9b89338932e62ca6bb60e5f5e412d7f64) )
	ROM_LOAD( "40.u85",       0x080000, 0x80000, CRC(368c15f4) SHA1(8ae95fd672448921964c4d0312d7366903362e27) )
	ROM_LOAD( "35.u84",       0x100000, 0x80000, CRC(7bde46ec) SHA1(1d26d268e1fc937e23ae7d93a1f86386b899a0c2) )
	ROM_LOAD( "39.u83",       0x180000, 0x80000, CRC(a0bae586) SHA1(0b2bb0c5c51b2717b820f0176d5775df21652667) )


	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "io13.bin",     0x00000, 0x20000, CRC(fa5e50ae) SHA1(f3bd87c83fca9269cc2f19db1fbf55540c96f931) )
	ROM_CONTINUE(             0x60000, 0x20000 )
	ROM_CONTINUE(             0xa0000, 0x20000 )
	ROM_CONTINUE(             0xe0000, 0x20000 )
	ROM_COPY( REGION_SOUND1,  0x00000, 0x20000, 0x20000 )
	ROM_COPY( REGION_SOUND1,  0x00000, 0x40000, 0x20000 )
	ROM_COPY( REGION_SOUND1,  0x00000, 0x80000, 0x20000 )
	ROM_COPY( REGION_SOUND1,  0x00000, 0xc0000, 0x20000 )
ROM_END

static UINT8 playmark_asciitohex(UINT8 data)
{
	/* Convert ASCII data to HEX */

	if ((data >= 0x30) && (data < 0x3a)) data -= 0x30;
	data &= 0xdf;			/* remove case sensitivity */
	if ((data >= 0x41) && (data < 0x5b)) data -= 0x37;

	return data;
}


static DRIVER_INIT( bigtwin )
{
	data8_t *playmark_PICROM_HEX = memory_region(REGION_USER1);
	data8_t *playmark_PICROM = memory_region(REGION_CPU2);
	INT32   offs, data;
	UINT16  src_pos = 0;
	UINT16  dst_pos = 0;
	UINT8   data_hi, data_lo;


	playmark_snd_flag = 0;

	/**** Convert the PIC16C57 ASCII HEX dumps to pure HEX ****/
	do
	{
		if ((playmark_PICROM_HEX[src_pos + 0] == ':') &&
			(playmark_PICROM_HEX[src_pos + 1] == '1') &&
			(playmark_PICROM_HEX[src_pos + 2] == '0'))
			{
			src_pos += 9;

			for (offs = 0; offs < 32; offs += 2)
			{
				data_hi = playmark_asciitohex((playmark_PICROM_HEX[src_pos + offs + 0]));
				data_lo = playmark_asciitohex((playmark_PICROM_HEX[src_pos + offs + 1]));

				if ((data_hi <= 0x0f) && (data_lo <= 0x0f)) {
					data = (data_hi << 4) | (data_lo << 0);
					playmark_PICROM[PIC16C57_PGM_OFFSET + dst_pos] = data;
					dst_pos += 1;
				}
			}
			src_pos += 32;
		}

		/* Get the PIC16C57 Config register data */

		if ((playmark_PICROM_HEX[src_pos + 0] == ':') &&
			(playmark_PICROM_HEX[src_pos + 1] == '0') &&
			(playmark_PICROM_HEX[src_pos + 2] == '2') &&
			(playmark_PICROM_HEX[src_pos + 3] == '1'))
			{
			src_pos += 9;

			data_hi = playmark_asciitohex((playmark_PICROM_HEX[src_pos + 0]));
			data_lo = playmark_asciitohex((playmark_PICROM_HEX[src_pos + 1]));
			data =  (data_hi <<  4) | (data_lo << 0);
			data_hi = playmark_asciitohex((playmark_PICROM_HEX[src_pos + 2]));
			data_lo = playmark_asciitohex((playmark_PICROM_HEX[src_pos + 3]));
			data |= (data_hi << 12) | (data_lo << 8);

			pic16c5x_config(data);
			src_pos = 0x7fff;		/* Force Exit */
		}
		src_pos += 1;
	} while (src_pos < 0x2d4c);		/* 0x2d4c is the size of the HEX rom loaded */
}

static DRIVER_INIT( hrdtimes )
{
  /* set up data ROMs */
  memcpy(hrdtimes_rom, &memory_region(REGION_CPU1)[0x0c0000], 0x10); /* correct i think */
}

/*

Power Balls  (c) 1994 Playmark

driver by David Haywood & Pierpaolo Prazzoli

*/

static struct tilemap *bg_tilemap;
static UINT16 *magicstk_videoram;
static int magicstk_tilebank;

static int bg_yoffset;
static int xoffset;
static int yoffset;

static WRITE16_HANDLER( magicstk_bgvideoram_w )
{
	COMBINE_DATA(&magicstk_videoram[offset]);
	tilemap_mark_tile_dirty(bg_tilemap,offset);
}

static WRITE16_HANDLER( tile_banking_w )
{
	if(((data >> 12) & 0x0f) != magicstk_tilebank)
	{
		magicstk_tilebank = (data >> 12) & 0x0f;
		tilemap_mark_all_tiles_dirty(bg_tilemap);
	}
}

static WRITE16_HANDLER( oki_banking )
{
	if(data & 3)
	{
		int addr = 0x40000 * ((data & 3) - 1);

		if(addr < memory_region_length(REGION_SOUND1))
			OKIM6295_set_bank_base(0, addr);
	}
}

static MEMORY_READ16_START( powerbal_readmem )
    { 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x088000, 0x0883ff, MRA16_RAM },
	{ 0x098000, 0x098fff, MRA16_RAM },
	{ 0x099000, 0x09bfff, MRA16_RAM }, /* not used */
	{ 0x0c2010, 0x0c2011, input_port_0_word_r },
	{ 0x0c2012, 0x0c2013, input_port_1_word_r },
	{ 0x0c2014, 0x0c2015, input_port_2_word_r },
	{ 0x0c2016, 0x0c2017, input_port_3_word_r },
	{ 0x0c2018, 0x0c2019, input_port_4_word_r },
	{ 0x0c201e, 0x0c201f, OKIM6295_status_0_lsb_r },
	{ 0x0f0000, 0x0fffff, MRA16_RAM },
	{ 0x101000, 0x101fff, MRA16_RAM },
	{ 0x103000, 0x103fff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( powerbal_writemem )
    { 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x088000, 0x0883ff, bigtwin_paletteram_w, &paletteram16 },
	{ 0x094000, 0x094001, MWA16_NOP },
	{ 0x094002, 0x094003, MWA16_NOP },
	{ 0x094004, 0x094005, tile_banking_w },
	{ 0x098000, 0x098fff, magicstk_bgvideoram_w, &magicstk_videoram },
	{ 0x099000, 0x09bfff, MWA16_RAM }, /* not used */
	{ 0x0c201c, 0x0c201d, oki_banking },
	{ 0x0c201e, 0x0c201f, OKIM6295_data_0_lsb_w },
	{ 0x0c4000, 0x0c4001, MWA16_NOP },
	{ 0x0f0000, 0x0fffff, MWA16_RAM },
	{ 0x101000, 0x101fff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x102000, 0x10200d, MWA16_NOP }, /* not used scroll regs? */
	{ 0x103000, 0x103fff, MWA16_RAM },
MEMORY_END


INPUT_PORTS_START( powerbal )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Allow_Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x1c, 0x1c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x14, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x1c, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_6C ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Language" )
	PORT_DIPSETTING(    0x08, "English" )
	PORT_DIPSETTING(    0x00, "Italian" )
	PORT_DIPNAME( 0x30, 0x20, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "1" )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0xc0, "Easy" )
	PORT_DIPSETTING(    0x80, "Normal" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0x00, "Very_Hard" )
INPUT_PORTS_END

static void powerbal_get_bg_tile_info(int tile_index)
{
	int code = (magicstk_videoram[tile_index] & 0x07ff) + magicstk_tilebank * 0x800;
	int colr = magicstk_videoram[tile_index] & 0xf000;

	if (magicstk_videoram[tile_index] & 0x800) code |= 0x8000;

	SET_TILE_INFO(1,code,colr >> 12,0)
}

static void draw_sprites(struct mame_bitmap *bitmap,const struct rectangle *cliprect)
{
	int offs;
	int height = Machine->gfx[0]->height;

	for (offs = 4;offs < spriteram_size/2;offs += 4)
	{
		int sx,sy,code,color,flipx;

		sy = spriteram16[offs+3-4];	/* typical Playmark style... */
		if (sy & 0x8000) return;	/* end of list marker */

		flipx = sy & 0x4000;
		sx = (spriteram16[offs+1] & 0x01ff) - 16-7;
		sy = (256-8-height - sy) & 0xff;
		code = spriteram16[offs+2];
		color = (spriteram16[offs+1] & 0xf000) >> 12;

		drawgfx(bitmap,Machine->gfx[0],
				code,
				color,
				flipx,0,
				sx + xoffset,sy + yoffset,
				cliprect,TRANSPARENCY_PEN,0);
	}
}

VIDEO_START( powerbal )
{
	bg_tilemap = tilemap_create(powerbal_get_bg_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE, 8, 8,64,32);

	if (!bg_tilemap)
		return 1;

	xoffset = -20;

	tilemap_set_scrolly(bg_tilemap, 0, bg_yoffset);

	return 0;
}

VIDEO_UPDATE( powerbal )
{
	tilemap_draw(bitmap,cliprect,bg_tilemap,0,0);
	draw_sprites(bitmap,cliprect);
}

static struct GfxLayout magicstk_charlayout =
{
	8,8,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(3,4), RGN_FRAC(2,4), RGN_FRAC(1,4), RGN_FRAC(0,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static struct GfxLayout magicstk_tilelayout =
{
	16,16,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(3,4), RGN_FRAC(2,4), RGN_FRAC(1,4), RGN_FRAC(0,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8
};



static struct GfxDecodeInfo powerbal_gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0, &magicstk_tilelayout, 0x100, 16 },	/* colors 0x100-0x1ff */
	{ REGION_GFX1, 0, &magicstk_charlayout, 0x000, 16 },	/* colors 0x000-0x0ff */
	{ -1 } /* end of array */
};

static struct OKIM6295interface powerbal_okim6295_interface =
{
	1,		                /* 1 chip */
	{ 1000000/132 },	    /* frequency? */
	{ REGION_SOUND1 },		/* memory region */
	{ 100 }
};

static MACHINE_DRIVER_START( powerbal )
	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)	/* 12 MHz */
	MDRV_CPU_MEMORY(powerbal_readmem,powerbal_writemem)
	MDRV_CPU_VBLANK_INT(irq2_line_hold,1)

	MDRV_FRAMES_PER_SECOND(61)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(128*8, 64*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 0*8, 30*8-1)
	MDRV_GFXDECODE(powerbal_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_VIDEO_START(powerbal)
	MDRV_VIDEO_UPDATE(powerbal)

	/* sound hardware */
	MDRV_SOUND_ADD(OKIM6295, powerbal_okim6295_interface)
MACHINE_DRIVER_END



/*
Power Balls
Playmark, 1994
*/

ROM_START( powerbal )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "3.u67",  0x00000, 0x40000, CRC(3aecdde4) SHA1(e78373246d55f120e8d94f4606da874df439b823) )
	ROM_LOAD16_BYTE( "2.u66",  0x00001, 0x40000, CRC(a4552a19) SHA1(88b84daa1fd36d5c683cf0d6dce341aedbc360d1) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "4.u38",        0x000000, 0x80000, CRC(a60aa981) SHA1(46a5d2d2a353a45127a03a104e877ffd150daa92) )
	ROM_LOAD( "5.u42",        0x080000, 0x80000, CRC(966c71df) SHA1(daf4bcf3d2ef10ea9a5e2e7ea71b3783b9f5b1f0) )
	ROM_LOAD( "6.u39",        0x100000, 0x80000, CRC(668957b9) SHA1(31fc9328ff6044e17834b6d61a886a8ef2e6570c) )
	ROM_LOAD( "7.u45",        0x180000, 0x80000, CRC(f5721c66) SHA1(1e8b3a8e82da60378dad7727af21157c4059b071) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "8.u86",        0x000000, 0x80000, CRC(4130694c) SHA1(581d0035ce1624568f635bd79290be6c587a2533) )
	ROM_LOAD( "9.u85",        0x080000, 0x80000, CRC(e7bcd2e7) SHA1(01a5e5ac5da2fd79a0c9088f775096b9915bae92) )
	ROM_LOAD( "10.u84",       0x100000, 0x80000, CRC(90412135) SHA1(499619c72613a1dd63a6504e39b159a18a71f4fa) )
	ROM_LOAD( "11.u83",       0x180000, 0x80000, CRC(92d7d40a) SHA1(81879945790feb9aeb45750e9b5ded3356571503) )

	/* $00000-$20000 stays the same in all sound banks, */
	/* the second half of the bank is the area that gets switched */
	ROM_REGION( 0xc0000, REGION_SOUND1, 0 ) /* OKI Samples */
	ROM_LOAD( "1.u16",        0x00000, 0x40000, CRC(12776dbc) SHA1(9ab9930fd581296642834d2cb4ba65264a588af3) )
	ROM_CONTINUE(             0x60000, 0x20000 )
	ROM_CONTINUE(             0xa0000, 0x20000 )
	ROM_COPY( REGION_SOUND1,  0x00000, 0x40000, 0x20000)
	ROM_COPY( REGION_SOUND1,  0x00000, 0x80000, 0x20000)
ROM_END

DRIVER_INIT( powerbal )
{
	bg_yoffset = 16;
	yoffset = -8;
}


GAMEX(1995, bigtwin,  0, bigtwin,  bigtwin,  bigtwin,  ROT0, "Playmark", "Big Twin", GAME_NO_COCKTAIL )
GAMEX(1995, wbeachvl, 0, wbeachvl, wbeachvl, 0,        ROT0, "Playmark", "World Beach Volley", GAME_NO_COCKTAIL | GAME_NO_SOUND )
GAMEX(1994, hrdtimes, 0, hrdtimes, hrdtimes, hrdtimes, ROT0, "Playmark", "Hard Times", GAME_NO_SOUND )
GAME( 1994, powerbal, 0, powerbal, powerbal, powerbal, ROT0, "Playmark", "Power Balls" )
