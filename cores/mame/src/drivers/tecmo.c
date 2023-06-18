/***************************************************************************

tecmo.c

driver by Nicola Salmoria


Silkworm memory map (preliminary)

0000-bfff ROM
c000-c1ff Background video RAM #2
c200-c3ff Background color RAM #2
c400-c5ff Background video RAM #1
c600-c7ff Background color RAM #1
c800-cbff Video RAM
cc00-cfff Color RAM
d000-dfff RAM
e000-e7ff Sprites
e800-efff Palette RAM, groups of 2 bytes, 4 bits per gun: xB RG
          e800-e9ff sprites
          ea00-ebff characters
          ec00-edff bg #1
          ee00-efff bg #2
f000-f7ff window for banked ROM

read:
f800      IN0 (heli) bit 0-3
f801      IN0 bit 4-7
f802      IN1 (jeep) bit 0-3
f803      IN1 bit 4-7
f806      DSWA bit 0-3
f807      DSWA bit 4-7
f808      DSWB bit 0-3
f809      DSWB bit 4-7
f80f      COIN

write:
f800-f801 bg #1 x scroll
f802      bg #1 y scroll
f803-f804 bg #2 x scroll
f805      bg #2 y scroll
f806      ????
f808      ROM bank selector
f809      ????
f80b      ????

***************************************************************************/
#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"



extern int tecmo_video_type;
extern unsigned char *tecmo_txvideoram,*tecmo_fgvideoram,*tecmo_bgvideoram;

WRITE_HANDLER( tecmo_txvideoram_w );
WRITE_HANDLER( tecmo_fgvideoram_w );
WRITE_HANDLER( tecmo_bgvideoram_w );
WRITE_HANDLER( tecmo_fgscroll_w );
WRITE_HANDLER( tecmo_bgscroll_w );
WRITE_HANDLER( tecmo_flipscreen_w );

VIDEO_START( tecmo );
VIDEO_UPDATE( tecmo );



WRITE_HANDLER( tecmo_bankswitch_w )
{
	int bankaddress;
	unsigned char *RAM = memory_region(REGION_CPU1);


	bankaddress = 0x10000 + ((data & 0xf8) << 8);
	cpu_setbank(1,&RAM[bankaddress]);
}

static WRITE_HANDLER( tecmo_sound_command_w )
{
	soundlatch_w(offset,data);
	cpu_set_irq_line(1,IRQ_LINE_NMI,PULSE_LINE);
}

static int adpcm_pos,adpcm_end;

static WRITE_HANDLER( tecmo_adpcm_start_w )
{
	adpcm_pos = data << 8;
	MSM5205_reset_w(0,0);
}
static WRITE_HANDLER( tecmo_adpcm_end_w )
{
	adpcm_end = (data + 1) << 8;
}
static WRITE_HANDLER( tecmo_adpcm_vol_w )
{
	MSM5205_set_volume(0,(data & 0x0f) * 100 / 15);
}
static void tecmo_adpcm_int(int num)
{
	static int adpcm_data = -1;

	if (adpcm_pos >= adpcm_end ||
				adpcm_pos >= memory_region_length(REGION_SOUND1))
		MSM5205_reset_w(0,1);
	else if (adpcm_data != -1)
	{
		MSM5205_data_w(0,adpcm_data & 0x0f);
		adpcm_data = -1;
	}
	else
	{
		unsigned char *ROM = memory_region(REGION_SOUND1);

		adpcm_data = ROM[adpcm_pos++];
		MSM5205_data_w(0,adpcm_data >> 4);
	}
}



static MEMORY_READ_START( readmem )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xefff, MRA_RAM },
	{ 0xf000, 0xf7ff, MRA_BANK1 },
	{ 0xf800, 0xf800, input_port_0_r },
	{ 0xf801, 0xf801, input_port_1_r },
	{ 0xf802, 0xf802, input_port_2_r },
	{ 0xf803, 0xf803, input_port_3_r },
	{ 0xf804, 0xf804, input_port_4_r },
	{ 0xf805, 0xf805, input_port_5_r },
	{ 0xf806, 0xf806, input_port_6_r },
	{ 0xf807, 0xf807, input_port_7_r },
	{ 0xf808, 0xf808, input_port_8_r },
	{ 0xf809, 0xf809, input_port_9_r },
	{ 0xf80f, 0xf80f, input_port_10_r },
MEMORY_END

static MEMORY_WRITE_START( rygar_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, MWA_RAM },
	{ 0xd000, 0xd7ff, tecmo_txvideoram_w, &tecmo_txvideoram },
	{ 0xd800, 0xdbff, tecmo_fgvideoram_w, &tecmo_fgvideoram },
	{ 0xdc00, 0xdfff, tecmo_bgvideoram_w, &tecmo_bgvideoram },
	{ 0xe000, 0xe7ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xe800, 0xefff, paletteram_xxxxBBBBRRRRGGGG_swap_w, &paletteram },
	{ 0xf000, 0xf7ff, MWA_ROM },
	{ 0xf800, 0xf802, tecmo_fgscroll_w },
	{ 0xf803, 0xf805, tecmo_bgscroll_w },
	{ 0xf806, 0xf806, tecmo_sound_command_w },
	{ 0xf807, 0xf807, tecmo_flipscreen_w },
	{ 0xf808, 0xf808, tecmo_bankswitch_w },
	{ 0xf80b, 0xf80b, watchdog_reset_w },
MEMORY_END

static MEMORY_WRITE_START( gemini_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, MWA_RAM },
	{ 0xd000, 0xd7ff, tecmo_txvideoram_w, &tecmo_txvideoram },
	{ 0xd800, 0xdbff, tecmo_fgvideoram_w, &tecmo_fgvideoram },
	{ 0xdc00, 0xdfff, tecmo_bgvideoram_w, &tecmo_bgvideoram },
	{ 0xe000, 0xe7ff, paletteram_xxxxBBBBRRRRGGGG_swap_w, &paletteram },
	{ 0xe800, 0xefff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xf000, 0xf7ff, MWA_ROM },
	{ 0xf800, 0xf802, tecmo_fgscroll_w },
	{ 0xf803, 0xf805, tecmo_bgscroll_w },
	{ 0xf806, 0xf806, tecmo_sound_command_w },
	{ 0xf807, 0xf807, tecmo_flipscreen_w },
	{ 0xf808, 0xf808, tecmo_bankswitch_w },
	{ 0xf80b, 0xf80b, watchdog_reset_w },
MEMORY_END

static MEMORY_WRITE_START( silkworm_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc3ff, tecmo_bgvideoram_w, &tecmo_bgvideoram },
	{ 0xc400, 0xc7ff, tecmo_fgvideoram_w, &tecmo_fgvideoram },
	{ 0xc800, 0xcfff, tecmo_txvideoram_w, &tecmo_txvideoram },
	{ 0xd000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe7ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xe800, 0xefff, paletteram_xxxxBBBBRRRRGGGG_swap_w, &paletteram },
	{ 0xf000, 0xf7ff, MWA_ROM },
	{ 0xf800, 0xf802, tecmo_fgscroll_w },
	{ 0xf803, 0xf805, tecmo_bgscroll_w },
	{ 0xf806, 0xf806, tecmo_sound_command_w },
	{ 0xf807, 0xf807, tecmo_flipscreen_w },
	{ 0xf808, 0xf808, tecmo_bankswitch_w },
	{ 0xf809, 0xf809, MWA_NOP },	/* ? */
	{ 0xf80b, 0xf80b, MWA_NOP },	/* ? if mapped to watchdog like in the others, causes reset */
MEMORY_END

static MEMORY_READ_START( rygar_sound_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x47ff, MRA_RAM },
	{ 0xc000, 0xc000, soundlatch_r },
MEMORY_END

static MEMORY_WRITE_START( rygar_sound_writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x47ff, MWA_RAM },
	{ 0x8000, 0x8000, YM3812_control_port_0_w },
	{ 0x8001, 0x8001, YM3812_write_port_0_w },
	{ 0xc000, 0xc000, tecmo_adpcm_start_w },
	{ 0xd000, 0xd000, tecmo_adpcm_end_w },
	{ 0xe000, 0xe000, tecmo_adpcm_vol_w },
	{ 0xf000, 0xf000, MWA_NOP },	/* NMI acknowledge */
MEMORY_END

static MEMORY_READ_START( tecmo_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0xc000, 0xc000, soundlatch_r },
MEMORY_END

static MEMORY_WRITE_START( tecmo_sound_writemem )
	{ 0x2000, 0x207f, MWA_RAM },	/* Silkworm set #2 has a custom CPU which */
									/* writes code to this area */
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0xa000, 0xa000, YM3812_control_port_0_w },
	{ 0xa001, 0xa001, YM3812_write_port_0_w },
	{ 0xc000, 0xc000, tecmo_adpcm_start_w },
	{ 0xc400, 0xc400, tecmo_adpcm_end_w },
	{ 0xc800, 0xc800, tecmo_adpcm_vol_w },
	{ 0xcc00, 0xcc00, MWA_NOP },	/* NMI acknowledge */
MEMORY_END



INPUT_PORTS_START( rygar )
	PORT_START	/* IN0 bits 0-3 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY )

	PORT_START	/* IN1 bits 0-3 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN2 bits 0-3 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )

	PORT_START	/* IN3 bits 0-3 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN4 bits 0-3 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START	/* unused? */
	PORT_BIT( 0x0f, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* DSWA bit 0-3 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x0C, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0C, DEF_STR( 1C_3C ) )

	PORT_START	/* DSWA bit 4-7 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )

	PORT_START	/* DSWB bit 0-3 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "50000 200000 500000" )
	PORT_DIPSETTING(    0x01, "100000 300000 600000" )
	PORT_DIPSETTING(    0x02, "200000 500000" )
	PORT_DIPSETTING(    0x03, "100000" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )

	PORT_START	/* DSWB bit 4-7 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x01, "Normal" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x03, "Hardest" )
	PORT_DIPNAME( 0x04, 0x00, "2P Can Start Anytime" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x08, 0x08, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Yes ) )

	PORT_START	/* unused? */
	PORT_BIT( 0x0f, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( gemini )
	PORT_START	/* IN0 bits 0-3 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY )

	PORT_START	/* IN1 bits 0-3 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN2 bits 0-3 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )

	PORT_START	/* IN3 bits 0-3 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* unused? */
	PORT_BIT( 0x0f, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN4 bits 0-3 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN2 )

	PORT_START	/* DSWA bit 0-3 */
	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x08, 0x00, "Final Round Continuation" )
	PORT_DIPSETTING(    0x00, "Round 6" )
	PORT_DIPSETTING(    0x08, "Round 7" )

	PORT_START	/* DSWA bit 4-7 */
	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x08, 0x00, "Buy in During Final Round" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Yes ) )

	PORT_START	/* DSWB bit 0-3 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x04, "Normal" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPSETTING(    0x0c, "Hardest" )

	PORT_START	/* DSWB bit 4-7 */
	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "50000 200000" )
	PORT_DIPSETTING(    0x01, "50000 300000" )
	PORT_DIPSETTING(    0x02, "100000 500000" )
	PORT_DIPSETTING(    0x03, "50000" )
	PORT_DIPSETTING(    0x04, "100000" )
	PORT_DIPSETTING(    0x05, "200000" )
	PORT_DIPSETTING(    0x06, "300000" )
	PORT_DIPSETTING(    0x07, "None" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* unused? */
	PORT_BIT( 0x0f, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( silkworm )
	PORT_START	/* IN0 bit 0-3 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY )

	PORT_START	/* IN0 bit 4-7 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* unused? */

	PORT_START	/* IN1 bit 0-3 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )

	PORT_START	/* IN1 bit 4-7 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* unused? */

	PORT_START	/* unused? */
	PORT_BIT( 0x0f, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* unused? */
	PORT_BIT( 0x0f, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* DSWA bit 0-3 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x0C, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0C, DEF_STR( 1C_3C ) )

	PORT_START	/* DSWA bit 4-7 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )	/* unused? */
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )

	PORT_START	/* DSWB bit 0-3 */
	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "50000 200000 500000" )
	PORT_DIPSETTING(    0x01, "100000 300000 800000" )
	PORT_DIPSETTING(    0x02, "50000 200000" )
	PORT_DIPSETTING(    0x03, "100000 300000" )
	PORT_DIPSETTING(    0x04, "50000" )
	PORT_DIPSETTING(    0x05, "100000" )
	PORT_DIPSETTING(    0x06, "200000" )
	PORT_DIPSETTING(    0x07, "None" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )	/* unused? */
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )

	PORT_START	/* DSWB bit 4-7 */
	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x05, "5" )
	/* 0x06 and 0x07 are the same as 0x00 */
	PORT_DIPNAME( 0x08, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x08, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )

	PORT_START	/* COIN */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN2 )
INPUT_PORTS_END

INPUT_PORTS_START( backfirt )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START		/* unused? */
	PORT_BIT( 0x0f, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN2 )

	PORT_START		/* unused? */
	PORT_BIT( 0x0f, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
    PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_6C ) ) /* limit of 9? */
	PORT_DIPNAME( 0x0C, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0C, DEF_STR( 1C_3C ) )

	PORT_START
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
	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "50000 200000 500000" )
	PORT_DIPSETTING(    0x01, "100000 300000 800000" )
	PORT_DIPSETTING(    0x02, "50000 200000" )
	PORT_DIPSETTING(    0x03, "100000 300000" )
	PORT_DIPSETTING(    0x04, "50000" )
	PORT_DIPSETTING(    0x05, "100000" )
	PORT_DIPSETTING(    0x06, "200000" )
	PORT_DIPSETTING(    0x07, "None" )
	PORT_DIPNAME( 0x38, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x18, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x28, "5" )
	PORT_DIPSETTING(    0x30, "6" )
	PORT_DIPSETTING(    0x38, "7" )

	PORT_START
    PORT_DIPNAME( 0x40, 0x40, "Continue" )
    PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x80, 0x00, "Invulnerability" )
    PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END

static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

static struct GfxLayout tilelayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4,
			32*8+0*4, 32*8+1*4, 32*8+2*4, 32*8+3*4, 32*8+4*4, 32*8+5*4, 32*8+6*4, 32*8+7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			16*32, 17*32, 18*32, 19*32, 20*32, 21*32, 22*32, 23*32 },
	128*8
};

static struct GfxLayout spritelayout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 256, 16 },	/* colors 256 - 511 */
	{ REGION_GFX2, 0, &spritelayout, 0, 16 },	/* colors   0 - 255 */
	{ REGION_GFX3, 0, &tilelayout, 512, 16 },	/* colors 512 - 767 */
	{ REGION_GFX4, 0, &tilelayout, 768, 16 },	/* colors 768 - 1023 */
	{ -1 } /* end of array */
};



static void irqhandler(int linestate)
{
	cpu_set_irq_line(1,0,linestate);
}

static struct YM3526interface ym3812_interface =
{
	1,			/* 1 chip */
	4000000,	/* 4 MHz */
	{ 100 },		/* volume */
	{ irqhandler }
};

static struct MSM5205interface msm5205_interface =
{
	1,					/* 1 chip             */
	384000,				/* 384KHz             */
	{ tecmo_adpcm_int },/* interrupt function */
	{ MSM5205_S48_4B },	/* 8KHz               */
	{ 50 }				/* volume */
};



static MACHINE_DRIVER_START( rygar )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 4000000)
	MDRV_CPU_MEMORY(readmem,rygar_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD_TAG("sound", Z80, 4000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(rygar_sound_readmem,rygar_sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)	/* frames per second, vblank duration */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(tecmo)
	MDRV_VIDEO_UPDATE(tecmo)

	/* sound hardware */
	MDRV_SOUND_ADD(YM3812, ym3812_interface)
	MDRV_SOUND_ADD(MSM5205, msm5205_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( gemini )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(rygar)
	MDRV_CPU_REPLACE("main", Z80, 6000000)
	MDRV_CPU_MEMORY(readmem,gemini_writemem)

	MDRV_CPU_MODIFY("sound")
	MDRV_CPU_MEMORY(tecmo_sound_readmem,tecmo_sound_writemem)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( silkworm )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(gemini)
	MDRV_CPU_REPLACE("main", Z80, 6000000)
	MDRV_CPU_MEMORY(readmem,silkworm_writemem)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( rygar )
	ROM_REGION( 0x18000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "5.5p",         0x00000, 0x08000, CRC(062cd55d) SHA1(656e29c890f5de964920b7841b3e11469cd20051) ) /* code */
	ROM_LOAD( "cpu_5m.bin",   0x08000, 0x04000, CRC(7ac5191b) SHA1(305f39d974f906f9bc24e9fe2ca58e647925ab63) ) /* code */
	ROM_LOAD( "cpu_5j.bin",   0x10000, 0x08000, CRC(ed76d606) SHA1(39c8a07e9a1f218ad088d00a2c9dfc993efafb6b) ) /* banked at f000-f7ff */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "cpu_4h.bin",   0x0000, 0x2000, CRC(e4a2fa87) SHA1(ed58187dbbcf59358496a98ffd6c227a87d6c433) )

	ROM_REGION( 0x08000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "cpu_8k.bin",   0x00000, 0x08000, CRC(4d482fb6) SHA1(57ad838b6d30b49dbd2d0ec425f33cfb15a67918) )	/* characters */

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "vid_6k.bin",   0x00000, 0x08000, CRC(aba6db9e) SHA1(43eb6f4f92afb5fbc11adc7e2ab04878ab56cb17) )	/* sprites */
	ROM_LOAD( "vid_6j.bin",   0x08000, 0x08000, CRC(ae1f2ed6) SHA1(6e6a33e665ba0884b7f57e9ad69d3f51e41d9e7b) )	/* sprites */
	ROM_LOAD( "vid_6h.bin",   0x10000, 0x08000, CRC(46d9e7df) SHA1(a24e0bea310a03636af704a0ad3f1a9cc4aafe12) )	/* sprites */
	ROM_LOAD( "vid_6g.bin",   0x18000, 0x08000, CRC(45839c9a) SHA1(eaee5767d8b0b62b991c089ef51b922e89850b79) )	/* sprites */

	ROM_REGION( 0x20000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "vid_6p.bin",   0x00000, 0x08000, CRC(9eae5f8e) SHA1(ed83b608ca57b9bf69fa866d9b8f55d16b7cff63) )
	ROM_LOAD( "vid_6o.bin",   0x08000, 0x08000, CRC(5a10a396) SHA1(12ebed3952ff35a2c275cb27c915f82183048cd4) )
	ROM_LOAD( "vid_6n.bin",   0x10000, 0x08000, CRC(7b12cf3f) SHA1(6b9d8cad6e15317df01bab0591fab09199ca6d40) )
	ROM_LOAD( "vid_6l.bin",   0x18000, 0x08000, CRC(3cea7eaa) SHA1(1dd194d5672dfe71c2b27d2d7b76f5a611cff76f) )

	ROM_REGION( 0x20000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "vid_6f.bin",   0x00000, 0x08000, CRC(9840edd8) SHA1(f19a1a1d932214037144c533ad07ed81256c34e7) )
	ROM_LOAD( "vid_6e.bin",   0x08000, 0x08000, CRC(ff65e074) SHA1(513c1bad336ef5d871f15d6ba8943020f98d1f4a) )
	ROM_LOAD( "vid_6c.bin",   0x10000, 0x08000, CRC(89868c85) SHA1(f21550f40e7a177e95c40f2726c651f85ca8edce) )
	ROM_LOAD( "vid_6b.bin",   0x18000, 0x08000, CRC(35389a7b) SHA1(a887a89f9bbb5979bb589468d80efba1f243690b) )

	ROM_REGION( 0x4000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "cpu_1f.bin",   0x0000, 0x4000, CRC(3cc98c5a) SHA1(ea1035be939ed1a994f3273b33412c85dda0973e) )
ROM_END

ROM_START( rygar2 )
	ROM_REGION( 0x18000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "cpu_5p.bin",   0x00000, 0x08000, CRC(e79c054a) SHA1(1aaffa53d121d5c55899bf18e85c42333fe0df54) ) /* code */
	ROM_LOAD( "cpu_5m.bin",   0x08000, 0x04000, CRC(7ac5191b) SHA1(305f39d974f906f9bc24e9fe2ca58e647925ab63) ) /* code */
	ROM_LOAD( "cpu_5j.bin",   0x10000, 0x08000, CRC(ed76d606) SHA1(39c8a07e9a1f218ad088d00a2c9dfc993efafb6b) ) /* banked at f000-f7ff */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "cpu_4h.bin",   0x0000, 0x2000, CRC(e4a2fa87) SHA1(ed58187dbbcf59358496a98ffd6c227a87d6c433) )

	ROM_REGION( 0x08000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "cpu_8k.bin",   0x00000, 0x08000, CRC(4d482fb6) SHA1(57ad838b6d30b49dbd2d0ec425f33cfb15a67918) )	/* characters */

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "vid_6k.bin",   0x00000, 0x08000, CRC(aba6db9e) SHA1(43eb6f4f92afb5fbc11adc7e2ab04878ab56cb17) )	/* sprites */
	ROM_LOAD( "vid_6j.bin",   0x08000, 0x08000, CRC(ae1f2ed6) SHA1(6e6a33e665ba0884b7f57e9ad69d3f51e41d9e7b) )	/* sprites */
	ROM_LOAD( "vid_6h.bin",   0x10000, 0x08000, CRC(46d9e7df) SHA1(a24e0bea310a03636af704a0ad3f1a9cc4aafe12) )	/* sprites */
	ROM_LOAD( "vid_6g.bin",   0x18000, 0x08000, CRC(45839c9a) SHA1(eaee5767d8b0b62b991c089ef51b922e89850b79) )	/* sprites */

	ROM_REGION( 0x20000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "vid_6p.bin",   0x00000, 0x08000, CRC(9eae5f8e) SHA1(ed83b608ca57b9bf69fa866d9b8f55d16b7cff63) )
	ROM_LOAD( "vid_6o.bin",   0x08000, 0x08000, CRC(5a10a396) SHA1(12ebed3952ff35a2c275cb27c915f82183048cd4) )
	ROM_LOAD( "vid_6n.bin",   0x10000, 0x08000, CRC(7b12cf3f) SHA1(6b9d8cad6e15317df01bab0591fab09199ca6d40) )
	ROM_LOAD( "vid_6l.bin",   0x18000, 0x08000, CRC(3cea7eaa) SHA1(1dd194d5672dfe71c2b27d2d7b76f5a611cff76f) )

	ROM_REGION( 0x20000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "vid_6f.bin",   0x00000, 0x08000, CRC(9840edd8) SHA1(f19a1a1d932214037144c533ad07ed81256c34e7) )
	ROM_LOAD( "vid_6e.bin",   0x08000, 0x08000, CRC(ff65e074) SHA1(513c1bad336ef5d871f15d6ba8943020f98d1f4a) )
	ROM_LOAD( "vid_6c.bin",   0x10000, 0x08000, CRC(89868c85) SHA1(f21550f40e7a177e95c40f2726c651f85ca8edce) )
	ROM_LOAD( "vid_6b.bin",   0x18000, 0x08000, CRC(35389a7b) SHA1(a887a89f9bbb5979bb589468d80efba1f243690b) )

	ROM_REGION( 0x4000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "cpu_1f.bin",   0x0000, 0x4000, CRC(3cc98c5a) SHA1(ea1035be939ed1a994f3273b33412c85dda0973e) )
ROM_END

ROM_START( rygarj )
	ROM_REGION( 0x18000, REGION_CPU1, 0 )	/* 64k for code */

	ROM_LOAD( "cpuj_5p.bin",  0x00000, 0x08000, CRC(b39698ba) SHA1(01a5a12a71973ad117b0bbd763e470f89c439e45) ) /* code */
	ROM_LOAD( "cpuj_5m.bin",  0x08000, 0x04000, CRC(3f180979) SHA1(c4c2e9f83b06b8677978800bfcc39f4ba3b344ab) ) /* code */
	ROM_LOAD( "cpuj_5j.bin",  0x10000, 0x08000, CRC(69e44e8f) SHA1(e979760a3582e64788c043adf7e475f0e1b75033) ) /* banked at f000-f7ff */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "cpu_4h.bin",   0x0000, 0x2000, CRC(e4a2fa87) SHA1(ed58187dbbcf59358496a98ffd6c227a87d6c433) )

	ROM_REGION( 0x08000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "cpuj_8k.bin",  0x00000, 0x08000, CRC(45047707) SHA1(deb47f5ec4b22e55e0393d8108e4ffb67dd68e12) )	/* characters */

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "vid_6k.bin",   0x00000, 0x08000, CRC(aba6db9e) SHA1(43eb6f4f92afb5fbc11adc7e2ab04878ab56cb17) )	/* sprites */
	ROM_LOAD( "vid_6j.bin",   0x08000, 0x08000, CRC(ae1f2ed6) SHA1(6e6a33e665ba0884b7f57e9ad69d3f51e41d9e7b) )	/* sprites */
	ROM_LOAD( "vid_6h.bin",   0x10000, 0x08000, CRC(46d9e7df) SHA1(a24e0bea310a03636af704a0ad3f1a9cc4aafe12) )	/* sprites */
	ROM_LOAD( "vid_6g.bin",   0x18000, 0x08000, CRC(45839c9a) SHA1(eaee5767d8b0b62b991c089ef51b922e89850b79) )	/* sprites */

	ROM_REGION( 0x20000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "vid_6p.bin",   0x00000, 0x08000, CRC(9eae5f8e) SHA1(ed83b608ca57b9bf69fa866d9b8f55d16b7cff63) )
	ROM_LOAD( "vid_6o.bin",   0x08000, 0x08000, CRC(5a10a396) SHA1(12ebed3952ff35a2c275cb27c915f82183048cd4) )
	ROM_LOAD( "vid_6n.bin",   0x10000, 0x08000, CRC(7b12cf3f) SHA1(6b9d8cad6e15317df01bab0591fab09199ca6d40) )
	ROM_LOAD( "vid_6l.bin",   0x18000, 0x08000, CRC(3cea7eaa) SHA1(1dd194d5672dfe71c2b27d2d7b76f5a611cff76f) )

	ROM_REGION( 0x20000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "vid_6f.bin",   0x00000, 0x08000, CRC(9840edd8) SHA1(f19a1a1d932214037144c533ad07ed81256c34e7) )
	ROM_LOAD( "vid_6e.bin",   0x08000, 0x08000, CRC(ff65e074) SHA1(513c1bad336ef5d871f15d6ba8943020f98d1f4a) )
	ROM_LOAD( "vid_6c.bin",   0x10000, 0x08000, CRC(89868c85) SHA1(f21550f40e7a177e95c40f2726c651f85ca8edce) )
	ROM_LOAD( "vid_6b.bin",   0x18000, 0x08000, CRC(35389a7b) SHA1(a887a89f9bbb5979bb589468d80efba1f243690b) )

	ROM_REGION( 0x4000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "cpu_1f.bin",   0x0000, 0x4000, CRC(3cc98c5a) SHA1(ea1035be939ed1a994f3273b33412c85dda0973e) )
ROM_END

ROM_START( silkworm )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "silkworm.4",   0x00000, 0x10000, CRC(a5277cce) SHA1(3886a3f3d1230d49d541f884c5b29938e13f98c8) )	/* c000-ffff is not used */
	ROM_LOAD( "silkworm.5",   0x10000, 0x10000, CRC(a6c7bb51) SHA1(75f6625459ab65f2d47a282c1295d4db38f5fe51) )	/* banked at f000-f7ff */

	ROM_REGION( 0x20000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "silkworm.3",   0x0000, 0x8000, CRC(b589f587) SHA1(0be5e2bf3daf3e28d63fdc8c89bb6fe7c48c6c3f) )

	ROM_REGION( 0x08000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "silkworm.2",   0x00000, 0x08000, CRC(e80a1cd9) SHA1(ef16feb1113acc7401f8951158b25f6f201196f2) )	/* characters */

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "silkworm.6",   0x00000, 0x10000, CRC(1138d159) SHA1(3b938606d448c4effdfe414bbf495b50cc3bc1c1) )	/* sprites */
	ROM_LOAD( "silkworm.7",   0x10000, 0x10000, CRC(d96214f7) SHA1(a5b2be3ae6a6eb8afef2c18c865a998fbf4adf93) )	/* sprites */
	ROM_LOAD( "silkworm.8",   0x20000, 0x10000, CRC(0494b38e) SHA1(03255f153824056e430a0b8595103f3b58b1fd97) )	/* sprites */
	ROM_LOAD( "silkworm.9",   0x30000, 0x10000, CRC(8ce3cdf5) SHA1(635248514c4e1e5aab7a2ed4d620a5b970d4a43a) )	/* sprites */

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "silkworm.10",  0x00000, 0x10000, CRC(8c7138bb) SHA1(0cfd69fa77d5b546f7dad80537d8d2497ae758bc) )	/* tiles #1 */
	ROM_LOAD( "silkworm.11",  0x10000, 0x10000, CRC(6c03c476) SHA1(79ad800a2f4ba6d44ba5a31210cbd8566bb357b6) )	/* tiles #1 */
	ROM_LOAD( "silkworm.12",  0x20000, 0x10000, CRC(bb0f568f) SHA1(b66c6d0407ed0b068c6bf07987f1b923d4a6e4f8) )	/* tiles #1 */
	ROM_LOAD( "silkworm.13",  0x30000, 0x10000, CRC(773ad0a4) SHA1(f7576e1ac8c779b33d7ec393555fd097a34257fa) )	/* tiles #1 */

	ROM_REGION( 0x40000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "silkworm.14",  0x00000, 0x10000, CRC(409df64b) SHA1(cada970bf9cc8f6522e7a71e00fe873568852873) )	/* tiles #2 */
	ROM_LOAD( "silkworm.15",  0x10000, 0x10000, CRC(6e4052c9) SHA1(e2e3d7221b75cb044449a25a076a93c3def1f11b) )	/* tiles #2 */
	ROM_LOAD( "silkworm.16",  0x20000, 0x10000, CRC(9292ed63) SHA1(70aa46fcc187b8200c5d246870e2e2dc4b2985cb) )	/* tiles #2 */
	ROM_LOAD( "silkworm.17",  0x30000, 0x10000, CRC(3fa4563d) SHA1(46e3cc41491d63efcdda43c84c7ac1385a1926d0) )	/* tiles #2 */

	ROM_REGION( 0x8000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "silkworm.1",   0x0000, 0x8000, CRC(5b553644) SHA1(5d39d2251094c17f7b732b4861401b3516fce9b1) )
ROM_END

ROM_START( silkwrm2 )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "r4",           0x00000, 0x10000, CRC(6df3df22) SHA1(9d6201c2df014bdb6877dfff936dddde1fe6fbd0) )	/* c000-ffff is not used */
	ROM_LOAD( "silkworm.5",   0x10000, 0x10000, CRC(a6c7bb51) SHA1(75f6625459ab65f2d47a282c1295d4db38f5fe51) )	/* banked at f000-f7ff */

	ROM_REGION( 0x20000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "r3",           0x0000, 0x8000, CRC(b79848d0) SHA1(d8162ab847bd0768572454d9775b0e9ed92b9519) )

	ROM_REGION( 0x08000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "silkworm.2",   0x00000, 0x08000, CRC(e80a1cd9) SHA1(ef16feb1113acc7401f8951158b25f6f201196f2) )	/* characters */

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "silkworm.6",   0x00000, 0x10000, CRC(1138d159) SHA1(3b938606d448c4effdfe414bbf495b50cc3bc1c1) )	/* sprites */
	ROM_LOAD( "silkworm.7",   0x10000, 0x10000, CRC(d96214f7) SHA1(a5b2be3ae6a6eb8afef2c18c865a998fbf4adf93) )	/* sprites */
	ROM_LOAD( "silkworm.8",   0x20000, 0x10000, CRC(0494b38e) SHA1(03255f153824056e430a0b8595103f3b58b1fd97) )	/* sprites */
	ROM_LOAD( "silkworm.9",   0x30000, 0x10000, CRC(8ce3cdf5) SHA1(635248514c4e1e5aab7a2ed4d620a5b970d4a43a) )	/* sprites */

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "silkworm.10",  0x00000, 0x10000, CRC(8c7138bb) SHA1(0cfd69fa77d5b546f7dad80537d8d2497ae758bc) )	/* tiles #1 */
	ROM_LOAD( "silkworm.11",  0x10000, 0x10000, CRC(6c03c476) SHA1(79ad800a2f4ba6d44ba5a31210cbd8566bb357b6) )	/* tiles #1 */
	ROM_LOAD( "silkworm.12",  0x20000, 0x10000, CRC(bb0f568f) SHA1(b66c6d0407ed0b068c6bf07987f1b923d4a6e4f8) )	/* tiles #1 */
	ROM_LOAD( "silkworm.13",  0x30000, 0x10000, CRC(773ad0a4) SHA1(f7576e1ac8c779b33d7ec393555fd097a34257fa) )	/* tiles #1 */

	ROM_REGION( 0x40000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "silkworm.14",  0x00000, 0x10000, CRC(409df64b) SHA1(cada970bf9cc8f6522e7a71e00fe873568852873) )	/* tiles #2 */
	ROM_LOAD( "silkworm.15",  0x10000, 0x10000, CRC(6e4052c9) SHA1(e2e3d7221b75cb044449a25a076a93c3def1f11b) )	/* tiles #2 */
	ROM_LOAD( "silkworm.16",  0x20000, 0x10000, CRC(9292ed63) SHA1(70aa46fcc187b8200c5d246870e2e2dc4b2985cb) )	/* tiles #2 */
	ROM_LOAD( "silkworm.17",  0x30000, 0x10000, CRC(3fa4563d) SHA1(46e3cc41491d63efcdda43c84c7ac1385a1926d0) )	/* tiles #2 */

	ROM_REGION( 0x8000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "silkworm.1",   0x0000, 0x8000, CRC(5b553644) SHA1(5d39d2251094c17f7b732b4861401b3516fce9b1) )
ROM_END

ROM_START( gemini )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "gw04-5s.rom",  0x00000, 0x10000, CRC(ff9de855) SHA1(34167af8456a081f68b338f10d4319ce1e703fd4) )	/* c000-ffff is not used */
	ROM_LOAD( "gw05-6s.rom",  0x10000, 0x10000, CRC(5a6947a9) SHA1(18b7aeb0f0e2c396bc759118dd7c45fd6070b804) )	/* banked at f000-f7ff */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "gw03-5h.rom",  0x0000, 0x8000, CRC(9bc79596) SHA1(61de9ddd45140e8ed88173294bd26147e2abfa21) )

	ROM_REGION( 0x08000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "gw02-3h.rom",  0x00000, 0x08000, CRC(7acc8d35) SHA1(05056e9f077e7571b314390b508c72d56ad0f43b) )	/* characters */

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "gw06-1c.rom",  0x00000, 0x10000, CRC(4ea51631) SHA1(9aee0f1ba210ac953dc193cfc739322966b6de8a) )	/* sprites */
	ROM_LOAD( "gw07-1d.rom",  0x10000, 0x10000, CRC(da42637e) SHA1(9885c52823279f26871092c77bdbe027df08268f) )	/* sprites */
	ROM_LOAD( "gw08-1f.rom",  0x20000, 0x10000, CRC(0b4e8d70) SHA1(55069f3df1c8db83f306d46b8262fd23585e6013) )	/* sprites */
	ROM_LOAD( "gw09-1h.rom",  0x30000, 0x10000, CRC(b65c5e4c) SHA1(699e1a9e72b8d94edae7382ba119fe5da113514d) )	/* sprites */

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "gw10-1n.rom",  0x00000, 0x10000, CRC(5e84cd4f) SHA1(e85320291027a16619c87fc2365448367bda454a) )	/* tiles #1 */
	ROM_LOAD( "gw11-2na.rom", 0x10000, 0x10000, CRC(08b458e1) SHA1(b3426faa57dca51dc053db44fa4968425d8bf3ee) )	/* tiles #1 */
	ROM_LOAD( "gw12-2nb.rom", 0x20000, 0x10000, CRC(229c9714) SHA1(f4f47d6b379c973c22f9ae7d7bec7041cdf3f737) )	/* tiles #1 */
	ROM_LOAD( "gw13-3n.rom",  0x30000, 0x10000, CRC(c5dfaf47) SHA1(c3202ca8c7f3c5c7dc9acdc09c1c894e168ef9fe) )	/* tiles #1 */

	ROM_REGION( 0x40000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "gw14-1r.rom",  0x00000, 0x10000, CRC(9c10e5b5) SHA1(a81399b85d8f3ddca26883ec3535cb9044c35ada) )	/* tiles #2 */
	ROM_LOAD( "gw15-2ra.rom", 0x10000, 0x10000, CRC(4cd18cfa) SHA1(c197a098a7c1e5220aad039383a40702fe7c4f21) )	/* tiles #2 */
	ROM_LOAD( "gw16-2rb.rom", 0x20000, 0x10000, CRC(f911c7be) SHA1(3f49f6c4734f2b644d93c4a54249aae6ff080e1d) )	/* tiles #2 */
	ROM_LOAD( "gw17-3r.rom",  0x30000, 0x10000, CRC(79a9ce25) SHA1(74e3917b8e7a920ceb2135d7ef8fb2f2c5176b21) )	/* tiles #2 */

	ROM_REGION( 0x8000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "gw01-6a.rom",  0x0000, 0x8000, CRC(d78afa05) SHA1(b02a739b045f5cddf943ce59226ef234463eeebe) )
ROM_END

ROM_START( backfirt )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )
	ROM_LOAD( "b5-e3.bin",           0x00000, 0x10000, CRC(0ab3bd4d) SHA1(2653d099c894304d3f9c2b2de9a7fed67be7b6dc) )	/* c000-ffff is not used */
	ROM_LOAD( "b4-f3.bin",   0x10000, 0x10000, CRC(150b6949) SHA1(31870a2f471b71d79a4daa0b5baca0d941de12e4) )	/* banked at f000-f7ff */

	ROM_REGION( 0x20000, REGION_CPU2, 0 )
	ROM_LOAD( "b2-e10.bin",           0x0000, 0x8000, CRC(9b2ac54f) SHA1(7c10e00235dc2668dee5c97ea5c6dc7722f35f03) )

	ROM_REGION( 0x08000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b3-c10.bin",   0x00000, 0x08000, CRC(08ce729f) SHA1(8e426251b20edfb10f0837b3106b4f333bc114a4) )	/* characters */

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "b6-c2.bin",   0x00000, 0x10000, CRC(c8c25e45) SHA1(d771d5e7d2d8082680f73b778ef2d88f2e9b8591) )	/* sprites */
	ROM_LOAD( "b7-d2.bin",   0x10000, 0x10000, CRC(25fb6a57) SHA1(7f411af7417fa901d65194c348ecec58c61b7cf7) )	/* sprites */
	ROM_LOAD( "b8-e2.bin",   0x20000, 0x10000, CRC(6bccac4e) SHA1(e042d049761affe4d3d0eac3c7a24f428643a9cf) )	/* sprites */
	ROM_LOAD( "b9-h2.bin",   0x30000, 0x10000, CRC(566a99b8) SHA1(a78825f0a85235399e66906cffafda98445a89a2) )	/* sprites */

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "b13-p1.bin",  0x00000, 0x10000, CRC(8c7138bb) SHA1(0cfd69fa77d5b546f7dad80537d8d2497ae758bc) )	/* tiles #1 */
	ROM_LOAD( "b12-p2.bin",  0x10000, 0x10000, CRC(6c03c476) SHA1(79ad800a2f4ba6d44ba5a31210cbd8566bb357b6) )	/* tiles #1 */
	ROM_LOAD( "b11-p2.bin",  0x20000, 0x10000, CRC(0bc84b4b) SHA1(599041108d09fd61aab2b0aeac0e07715887476c) )	/* tiles #1 */
	ROM_LOAD( "b10-p3.bin",  0x30000, 0x10000, CRC(ec149ec3) SHA1(7817dc2659fe4ba3bb810df278378d51d97065b3) )	/* tiles #1 */

	ROM_REGION( 0x40000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "b17-s1.bin",  0x00000, 0x10000, CRC(409df64b) SHA1(cada970bf9cc8f6522e7a71e00fe873568852873) )	/* tiles #2 */
	ROM_LOAD( "b16-s2.bin",  0x10000, 0x10000, CRC(6e4052c9) SHA1(e2e3d7221b75cb044449a25a076a93c3def1f11b) )	/* tiles #2 */
	ROM_LOAD( "b15-s2.bin",  0x20000, 0x10000, CRC(2b6cc20e) SHA1(4815819288753400935836cc1b0b69f4c4b43ddc) )	/* tiles #2 */
	ROM_LOAD( "b14-s3.bin",  0x30000, 0x08000, CRC(4d29637a) SHA1(28e85925138256b8ce5a1c4a5df5b219b1b6b197) )	/* tiles #2 */

	ROM_REGION( 0x8000, REGION_SOUND1, ROMREGION_ERASEFF )	/* ADPCM samples */
	/* not on this pcb? */
ROM_END

/*
   video_type is used to distinguish Rygar, Silkworm and Gemini Wing.
   This is needed because there is a difference in the tile and sprite indexing.
*/
DRIVER_INIT( rygar )    { tecmo_video_type = 0; }
DRIVER_INIT( silkworm ) { tecmo_video_type = 1; }
DRIVER_INIT( gemini )   { tecmo_video_type = 2; }



GAME( 1986, rygar,    0,        rygar,    rygar,    rygar,    ROT0,  "Tecmo", "Rygar (US set 1)" )
GAME( 1986, rygar2,   rygar,    rygar,    rygar,    rygar,    ROT0,  "Tecmo", "Rygar (US set 2)" )
GAME( 1986, rygarj,   rygar,    rygar,    rygar,    rygar,    ROT0,  "Tecmo", "Argus no Senshi (Japan)" )
GAME( 1987, gemini,   0,        gemini,   gemini,   gemini,   ROT90, "Tecmo", "Gemini Wing" )
GAME( 1988, silkworm, 0,        silkworm, silkworm, silkworm, ROT0,  "Tecmo", "Silk Worm (set 1)" )
GAME( 1988, silkwrm2, silkworm, silkworm, silkworm, silkworm, ROT0,  "Tecmo", "Silk Worm (set 2)" )
GAME( 1988, backfirt, 0,        gemini,   backfirt, gemini,   ROT0,  "Tecmo", "Back Fire (Tecmo)" )

