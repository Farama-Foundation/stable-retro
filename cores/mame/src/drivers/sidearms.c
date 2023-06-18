/***************************************************************************

  Sidearms
  ========

  Driver provided by Paul Leaman


Change Log:

JUL-2003 AAT

- cleaned vidhrdw and corrected screen flipping

JUN-2003 (Curt Coder)

- converted driver to use tilemaps

FEB-2003 AAT

- added preliminary starfield emulation. circuit transcribed from
  schematics but still not perfect.

- rewrote video update and the following bugs seem to be fixed:

  sidearms060red:  attract mode and stage six crashing
  sidearms055gre:  strange background color
  turtship37b5yel: various graphics glitches and priority problems

Notes:

  Unknown PROMs are mostly used for timing. Only the first four sprite
  encoding parameters have been identified, the other 28(!) are
  believed to be line-buffer controls.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

extern UINT8 *sidearms_bg_scrollx;
extern UINT8 *sidearms_bg_scrolly;

extern WRITE_HANDLER( sidearms_videoram_w );
extern WRITE_HANDLER( sidearms_colorram_w );
extern WRITE_HANDLER( sidearms_star_scrollx_w );
extern WRITE_HANDLER( sidearms_star_scrolly_w );
extern WRITE_HANDLER( sidearms_c804_w );
extern WRITE_HANDLER( sidearms_gfxctrl_w );

extern PALETTE_INIT( sidearms );
extern VIDEO_START( sidearms );
extern VIDEO_UPDATE( sidearms );
extern VIDEO_EOF( sidearms );

int sidearms_gameid;

static WRITE_HANDLER( sidearms_bankswitch_w )
{
	int bankaddress;
	unsigned char *RAM = memory_region(REGION_CPU1);


	/* bits 0 and 1 select the ROM bank */
	bankaddress = 0x10000 + (data & 0x0f) * 0x4000;
	cpu_setbank(1,&RAM[bankaddress]);
}




/* Turtle Ship input ports are rotated 90 degrees */
static READ_HANDLER( turtship_ports_r )
{
	int i,res;


	res = 0;
	for (i = 0;i < 8;i++)
		res |= ((readinputport(i) >> offset) & 1) << i;

	return res;
}


static MEMORY_READ_START( readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc800, 0xc800, input_port_0_r },
	{ 0xc801, 0xc801, input_port_1_r },
	{ 0xc802, 0xc802, input_port_2_r },
	{ 0xc803, 0xc803, input_port_3_r },
	{ 0xc804, 0xc804, input_port_4_r },
	{ 0xc805, 0xc805, input_port_5_r },
	{ 0xd000, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc3ff, paletteram_xxxxBBBBRRRRGGGG_split1_w, &paletteram },
	{ 0xc400, 0xc7ff, paletteram_xxxxBBBBRRRRGGGG_split2_w, &paletteram_2 },
	{ 0xc800, 0xc800, soundlatch_w },
	{ 0xc801, 0xc801, sidearms_bankswitch_w },
	{ 0xc802, 0xc802, watchdog_reset_w },
	{ 0xc804, 0xc804, sidearms_c804_w },
	{ 0xc805, 0xc805, sidearms_star_scrollx_w },
	{ 0xc806, 0xc806, sidearms_star_scrolly_w },
	{ 0xc808, 0xc809, MWA_RAM, &sidearms_bg_scrollx },
	{ 0xc80a, 0xc80b, MWA_RAM, &sidearms_bg_scrolly },
	{ 0xc80c, 0xc80c, sidearms_gfxctrl_w },	/* background and sprite enable */
	{ 0xd000, 0xd7ff, sidearms_videoram_w, &videoram },
	{ 0xd800, 0xdfff, sidearms_colorram_w, &colorram },
	{ 0xe000, 0xefff, MWA_RAM },
	{ 0xf000, 0xffff, MWA_RAM, &spriteram, &spriteram_size },
MEMORY_END


static MEMORY_READ_START( turtship_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xe7ff, MRA_RAM },
	{ 0xe800, 0xe807, turtship_ports_r },
	{ 0xf000, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( turtship_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, MWA_RAM },
	{ 0xd000, 0xdfff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xe000, 0xe3ff, paletteram_xxxxBBBBRRRRGGGG_split1_w, &paletteram },
	{ 0xe400, 0xe7ff, paletteram_xxxxBBBBRRRRGGGG_split2_w, &paletteram_2 },
	{ 0xe800, 0xe800, soundlatch_w },
	{ 0xe801, 0xe801, sidearms_bankswitch_w },
	{ 0xe802, 0xe802, watchdog_reset_w },
	{ 0xe804, 0xe804, sidearms_c804_w },
	{ 0xe805, 0xe805, sidearms_star_scrollx_w },
	{ 0xe806, 0xe806, sidearms_star_scrolly_w },
	{ 0xe808, 0xe809, MWA_RAM, &sidearms_bg_scrollx },
	{ 0xe80a, 0xe80b, MWA_RAM, &sidearms_bg_scrolly },
	{ 0xe80c, 0xe80c, sidearms_gfxctrl_w },	/* background and sprite enable */
	{ 0xf000, 0xf7ff, sidearms_videoram_w, &videoram },
	{ 0xf800, 0xffff, sidearms_colorram_w, &colorram },
MEMORY_END


static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xd000, 0xd000, soundlatch_r },
	{ 0xf000, 0xf000, YM2203_status_port_0_r },
	{ 0xf002, 0xf002, YM2203_status_port_1_r },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xf000, 0xf000, YM2203_control_port_0_w },
	{ 0xf001, 0xf001, YM2203_write_port_0_w },
	{ 0xf002, 0xf002, YM2203_control_port_1_w },
	{ 0xf003, 0xf003, YM2203_write_port_1_w },
MEMORY_END

/* Whizz */

static WRITE_HANDLER( whizz_bankswitch_w )
{
	int bankaddress;
	unsigned char *RAM = memory_region(REGION_CPU1);
	int bank = 0;

	switch (data & 0xC0)
 	{
		case 0x00 :	bank = 0;	break;
		case 0x40 :	bank = 2;	break;
		case 0x80 :	bank = 1;	break;
		case 0xC0 :	bank = 3;	break;
	}

	bankaddress = 0x10000 + bank * 0x4000;
	cpu_setbank(1,&RAM[bankaddress]);
}

static MEMORY_READ_START( whizz_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc800, 0xc800, input_port_0_r },
	{ 0xc801, 0xc801, input_port_1_r },
	{ 0xc802, 0xc802, input_port_2_r },
	{ 0xc803, 0xc803, input_port_3_r },
	{ 0xc804, 0xc804, input_port_4_r },
	{ 0xc805, 0xc805, input_port_5_r },
	{ 0xc806, 0xc806, input_port_6_r },
	{ 0xc807, 0xc807, input_port_7_r },
	{ 0xd000, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( whizz_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc3ff, paletteram_xxxxBBBBRRRRGGGG_split1_w, &paletteram },
	{ 0xc400, 0xc7ff, paletteram_xxxxBBBBRRRRGGGG_split2_w, &paletteram_2 },
	{ 0xc800, 0xc800, soundlatch_w },
	{ 0xc801, 0xc801, whizz_bankswitch_w },
	{ 0xc802, 0xc802, watchdog_reset_w },
	{ 0xc803, 0xc803, MWA_NOP }, /* ? */
	{ 0xc804, 0xc804, sidearms_c804_w },
	{ 0xc805, 0xc805, MWA_NOP }, /* ? */
	{ 0xe805, 0xe805, sidearms_star_scrollx_w },
	{ 0xe806, 0xe806, sidearms_star_scrolly_w },
	{ 0xc808, 0xc809, MWA_RAM, &sidearms_bg_scrollx },
	{ 0xc80a, 0xc80b, MWA_RAM, &sidearms_bg_scrolly },
	{ 0xc80c, 0xc80c, sidearms_gfxctrl_w },
	{ 0xd000, 0xd7ff, sidearms_videoram_w, &videoram },
	{ 0xd800, 0xdfff, sidearms_colorram_w, &colorram },
	{ 0xe000, 0xefff, MWA_RAM },
	{ 0xf000, 0xffff, MWA_RAM, &spriteram, &spriteram_size },
MEMORY_END

static MEMORY_READ_START( whizz_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xf800, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( whizz_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xf800, 0xffff, MWA_RAM },
MEMORY_END

static PORT_READ_START( whizz_readport )
	{ 0x01, 0x01, YM2151_status_port_0_r },
	{ 0xc0, 0xc0, soundlatch_r },
MEMORY_END

static PORT_WRITE_START( whizz_writeport )
	{ 0x00, 0x00, YM2151_register_port_0_w },
	{ 0x01, 0x01, YM2151_data_port_0_w },
	{ 0x40, 0x40, MWA_NOP },
MEMORY_END


INPUT_PORTS_START( sidearms )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x08, 0x08, "Freeze" )	/* I'm not sure it's really a dip switch */
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x07, 0x04, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x07, "0 (Easiest)" )
	PORT_DIPSETTING(    0x06, "1" )
	PORT_DIPSETTING(    0x05, "2" )
	PORT_DIPSETTING(    0x04, "3 (Normal)" )
	PORT_DIPSETTING(    0x03, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x01, "6" )
	PORT_DIPSETTING(    0x00, "7 (Hardest)" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x30, "100000" )
	PORT_DIPSETTING(    0x20, "100000 100000" )
	PORT_DIPSETTING(    0x10, "150000 150000" )
	PORT_DIPSETTING(    0x00, "200000 200000" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START      /* DSW2 */
	PORT_BIT( 0x7f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )     /* not sure, but likely */
INPUT_PORTS_END

INPUT_PORTS_START( turtship )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSW0 */
	PORT_BITX( 0x01, 0x01, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", 0, IP_JOY_NONE )
	PORT_DIPSETTING( 0x01, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x02, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x10, "Normal" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0xe0, 0xa0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0xe0, "1" )
	PORT_DIPSETTING(    0x60, "2" )
	PORT_DIPSETTING(    0xa0, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0xc0, "5" )
	PORT_DIPSETTING(    0x40, "6" )
	PORT_DIPSETTING(    0x80, "7" )
	PORT_DIPSETTING(    0x00, "8" )

	PORT_START      /* DSW1 */
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x08, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x08, "Every 150000" )
	PORT_DIPSETTING(    0x00, "Every 200000" )
	PORT_DIPSETTING(    0x0c, "150000 only" )
	PORT_DIPSETTING(    0x04, "200000 only" )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	/* 0xc0 1 Coin/1 Credit */
INPUT_PORTS_END

INPUT_PORTS_START( dyger )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* seems to be 1-player only */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* seems to be 1-player only */

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x02, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x10, "Easy" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0xe0, 0xa0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0xe0, "1" )
	PORT_DIPSETTING(    0x60, "2" )
	PORT_DIPSETTING(    0xa0, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0xc0, "5" )
	PORT_DIPSETTING(    0x40, "6" )
	PORT_DIPSETTING(    0x80, "7" )
	PORT_DIPSETTING(    0x00, "8" )

	PORT_START      /* DSW1 */
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x04, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x04, "Every 150000" )
	PORT_DIPSETTING(    0x00, "Every 200000" )
	PORT_DIPSETTING(    0x0c, "150000 only" )
	PORT_DIPSETTING(    0x08, "200000 only" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	/* 0xc0 1 Coin/1 Credit */
INPUT_PORTS_END

INPUT_PORTS_START( whizz )
	PORT_START	/* 8-bit */
	PORT_DIPNAME( 0x07, 0x04, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x07, "0 (Easiest)" )
	PORT_DIPSETTING(    0x06, "1" )
	PORT_DIPSETTING(    0x05, "2" )
	PORT_DIPSETTING(    0x04, "3 (Normal)" )
	PORT_DIPSETTING(    0x03, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x01, "6" )
	PORT_DIPSETTING(    0x00, "7 (Hardest)" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Flip_Screen ) )
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

	PORT_START	/* 8-bit */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x18, "100000 Only" )
	PORT_DIPSETTING(    0x10, "Every 100000" )
	PORT_DIPSETTING(    0x08, "Every 150000" )
	PORT_DIPSETTING(    0x00, "Every 200000" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* 8-bit */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_DIPNAME( 0x10, 0x10, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* 8-bit */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* 8-bit */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN6 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH,IPT_VBLANK )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN7 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


static struct GfxLayout charlayout =
{
	8,8,    /* 8*8 characters */
	RGN_FRAC(1,1),   /* 1024 characters */
	2,      /* 2 bits per pixel */
	{ 4, 0 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8    /* every char takes 16 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,  /* 16*16 sprites */
	RGN_FRAC(1,2),   /* 2048 sprites */
	4,      /* 4 bits per pixel */
	{ RGN_FRAC(1,2)+4, RGN_FRAC(1,2)+0, 4, 0 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3,
			32*8+0, 32*8+1, 32*8+2, 32*8+3, 33*8+0, 33*8+1, 33*8+2, 33*8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8    /* every sprite takes 64 consecutive bytes */
};

static struct GfxLayout tilelayout =
{
	32,32,  /* 32*32 tiles */
	RGN_FRAC(1,2),    /* 512 tiles */
	4,      /* 4 bits per pixel */
	{ RGN_FRAC(1,2)+4, RGN_FRAC(1,2)+0, 4, 0 },
	{
		0,       1,       2,       3,       8+0,       8+1,       8+2,       8+3,
		32*16+0, 32*16+1, 32*16+2, 32*16+3, 32*16+8+0, 32*16+8+1, 32*16+8+2, 32*16+8+3,
		64*16+0, 64*16+1, 64*16+2, 64*16+3, 64*16+8+0, 64*16+8+1, 64*16+8+2, 64*16+8+3,
		96*16+0, 96*16+1, 96*16+2, 96*16+3, 96*16+8+0, 96*16+8+1, 96*16+8+2, 96*16+8+3,
	},
	{
		0*16,  1*16,  2*16,  3*16,  4*16,  5*16,  6*16,  7*16,
		8*16,  9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16,
		16*16, 17*16, 18*16, 19*16, 20*16, 21*16, 22*16, 23*16,
		24*16, 25*16, 26*16, 27*16, 28*16, 29*16, 30*16, 31*16
	},
	256*8   /* every tile takes 256 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,   768, 64 }, /* colors 768-1023 */
	{ REGION_GFX2, 0, &tilelayout,     0, 32 }, /* colors   0-511 */
	{ REGION_GFX3, 0, &spritelayout, 512, 16 }, /* colors 512-767 */
	{ -1 } /* end of array */
};



static struct GfxLayout turtship_tilelayout =
{
	32,32,  /* 32*32 tiles */
	RGN_FRAC(1,2),    /* 768 tiles */
	4,      /* 4 bits per pixel */
	{ RGN_FRAC(1,2)+4, RGN_FRAC(1,2)+0, 4, 0 },
	{
		0,       1,       2,       3,       8+0,       8+1,       8+2,       8+3,
		32*16+0, 32*16+1, 32*16+2, 32*16+3, 32*16+8+0, 32*16+8+1, 32*16+8+2, 32*16+8+3,
		64*16+0, 64*16+1, 64*16+2, 64*16+3, 64*16+8+0, 64*16+8+1, 64*16+8+2, 64*16+8+3,
		96*16+0, 96*16+1, 96*16+2, 96*16+3, 96*16+8+0, 96*16+8+1, 96*16+8+2, 96*16+8+3,
	},
	{
		0*16,  1*16,  2*16,  3*16,  4*16,  5*16,  6*16,  7*16,
		8*16,  9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16,
		16*16, 17*16, 18*16, 19*16, 20*16, 21*16, 22*16, 23*16,
		24*16, 25*16, 26*16, 27*16, 28*16, 29*16, 30*16, 31*16
	},
	256*8   /* every tile takes 256 consecutive bytes */
};

static struct GfxDecodeInfo turtship_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,          768, 64 },	/* colors 768-1023 */
	{ REGION_GFX2, 0, &turtship_tilelayout,   0, 32 },	/* colors   0-511 */
	{ REGION_GFX3, 0, &spritelayout,        512, 16 },	/* colors 512-767 */
	{ -1 } /* end of array */
};

/* handler called by the 2203 emulator when the internal timers cause an IRQ */
static void irqhandler(int irq)
{
	cpu_set_irq_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2203interface ym2203_interface =
{
	2,			/* 2 chips */
	4000000,	/* 4 MHz ? (hand tuned) */
	{ YM2203_VOL(15,25), YM2203_VOL(15,25) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler }
};

static struct YM2151interface whizz_ym2151_interface =
{
	1,
	4000000,	/* ? */
	{ YM3012_VOL(100,MIXER_PAN_CENTER,100,MIXER_PAN_CENTER) },
	{ irqhandler }
};

static MACHINE_DRIVER_START( sidearms )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 4000000) /* 4 MHz (?) */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, 4000000) /* 4 MHz (?) */
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_BUFFERS_SPRITERAM)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(8*8, (64-8)*8-1, 2*8, 30*8-1 )
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(sidearms)
	MDRV_VIDEO_EOF(sidearms)
	MDRV_VIDEO_UPDATE(sidearms)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, ym2203_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( turtship )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 4000000) /* 4 MHz (?) */
	MDRV_CPU_MEMORY(turtship_readmem,turtship_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, 4000000) /* 4 MHz (?) */
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_BUFFERS_SPRITERAM)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(8*8, (64-8)*8-1, 2*8, 30*8-1 )
	MDRV_GFXDECODE(turtship_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(sidearms)
	MDRV_VIDEO_EOF(sidearms)
	MDRV_VIDEO_UPDATE(sidearms)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, ym2203_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( whizz )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 4000000)        /* 4 MHz (?) */
	MDRV_CPU_MEMORY(whizz_readmem,whizz_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)        /* 4 MHz (?) */
	MDRV_CPU_MEMORY(whizz_sound_readmem,whizz_sound_writemem)
	MDRV_CPU_PORTS(whizz_readport,whizz_writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1000)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_BUFFERS_SPRITERAM)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(8*8, (64-8)*8-1, 2*8, 30*8-1 )
	MDRV_GFXDECODE(turtship_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(sidearms)
	MDRV_VIDEO_UPDATE(sidearms)
	MDRV_VIDEO_EOF(sidearms)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2151, whizz_ym2151_interface)
MACHINE_DRIVER_END




ROM_START( sidearms )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )     /* 64k for code + banked ROMs images */
	ROM_LOAD( "sa03.bin",     0x00000, 0x08000, CRC(e10fe6a0) SHA1(ae59461768d044f14b9aac3e4e491c76cec7adac) )        /* CODE */
	ROM_LOAD( "a_14e.rom",    0x10000, 0x08000, CRC(4925ed03) SHA1(b11dbd9889db89cff008ca21beb6b1b70d983e16) )        /* 0+1 */
	ROM_LOAD( "a_12e.rom",    0x18000, 0x08000, CRC(81d0ece7) SHA1(5c1d154f9c1de6b5f5d7abf5d413e9c493461e6f) )        /* 2+3 */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for the audio CPU */
	ROM_LOAD( "a_04k.rom",    0x0000, 0x8000, CRC(34efe2d2) SHA1(e1d8895c113e4dee1a132e2471d75dfa6c36b620) )

	ROM_REGION( 0x08000, REGION_USER1, 0 )    /* starfield data */
	ROM_LOAD( "b_11j.rom",    0x0000, 0x8000, CRC(134dc35b) SHA1(6360c1efa7c4e1d6d817a97ca43dd4af8ed6afe5) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "a_10j.rom",    0x00000, 0x4000, CRC(651fef75) SHA1(9c821a2ee30c222987f0d4192133776490d6a4e0) ) /* characters */

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "b_13d.rom",    0x00000, 0x8000, CRC(3c59afe1) SHA1(5459a5795cf13012674993aa55bbd39e9a5c2f1b) ) /* tiles */
	ROM_LOAD( "b_13e.rom",    0x08000, 0x8000, CRC(64bc3b77) SHA1(54fe6f258fda509a92eb0f5aa238102efce729e0) )
	ROM_LOAD( "b_13f.rom",    0x10000, 0x8000, CRC(e6bcea6f) SHA1(19477e284967beafc4e7cd0d0da3534eb6dec388) )
	ROM_LOAD( "b_13g.rom",    0x18000, 0x8000, CRC(c71a3053) SHA1(963e105aa0b0174e8aa5e1f7676c5c604ca72d1c) )
	ROM_LOAD( "b_14d.rom",    0x20000, 0x8000, CRC(826e8a97) SHA1(ad5ed9a81805dde54fb2703345b2ab7b56853ec6) )
	ROM_LOAD( "b_14e.rom",    0x28000, 0x8000, CRC(6cfc02a4) SHA1(491e880e85d5256fa2eea6d0fb402f0a1176b675) )
	ROM_LOAD( "b_14f.rom",    0x30000, 0x8000, CRC(9b9f6730) SHA1(0f8fe5dc32ee50ebb2051c0c0c4d635582416317) )
	ROM_LOAD( "b_14g.rom",    0x38000, 0x8000, CRC(ef6af630) SHA1(499b17eeb5e7256ede477510b0547df520316996) )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "b_11b.rom",    0x00000, 0x8000, CRC(eb6f278c) SHA1(15e250aa98ee69ac3983d4511976c35833b37cab) ) /* sprites */
	ROM_LOAD( "b_13b.rom",    0x08000, 0x8000, CRC(e91b4014) SHA1(6557344ce8bc05309ab8ebe846871ed554b256b8) )
	ROM_LOAD( "b_11a.rom",    0x10000, 0x8000, CRC(2822c522) SHA1(00b3cab899e5ac1af6300f2ec2a54303df9ab014) )
	ROM_LOAD( "b_13a.rom",    0x18000, 0x8000, CRC(3e8a9f75) SHA1(b1bfb7604791950aa0454b68b24f6ad3b9131be8) )
	ROM_LOAD( "b_12b.rom",    0x20000, 0x8000, CRC(86e43eda) SHA1(c33b0ab6f7f0f886410a3943988b737d175635be) )
	ROM_LOAD( "b_14b.rom",    0x28000, 0x8000, CRC(076e92d1) SHA1(27144834b5b2849be8c46e97aaaeaa8b304ea810) )
	ROM_LOAD( "b_12a.rom",    0x30000, 0x8000, CRC(ce107f3c) SHA1(2235281449247cb2446b008b36077788c5b15026) )
	ROM_LOAD( "b_14a.rom",    0x38000, 0x8000, CRC(dba06076) SHA1(87b3b3437bc4bd727ce7e34dd914e6fe23bcac3d) )

	ROM_REGION( 0x08000, REGION_GFX4, 0 )	/* background tilemaps */
	ROM_LOAD( "b_03d.rom",    0x0000, 0x8000, CRC(6f348008) SHA1(b500bc32ba47e9cc9dcf2254b9455ac4d61992db) )

	ROM_REGION( 0x0320, REGION_PROMS, 0 )
	ROM_LOAD( "63s141.16h",   0x0000, 0x0100, CRC(75af3553) SHA1(14da009592877a6097b34ea844fa897ceda7465e) )	/* timing*/
	ROM_LOAD( "63s141.11h",   0x0100, 0x0100, CRC(a6e4d68f) SHA1(b9367e0c959cdf0397d33a49d778a66a407572b7) )	/* color mixing*/
	ROM_LOAD( "63s141.15h",   0x0200, 0x0100, CRC(c47c182a) SHA1(47d6139256e6838f633a04084bd0a7a84912f7fb) )	/* timing*/
	ROM_LOAD( "63s081.3j",    0x0300, 0x0020, CRC(c5817816) SHA1(cc642daafa0bcb160ee04e74e2d168fd44087608) )	/* unknown*/
ROM_END

ROM_START( sidearmr )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )     /* 64k for code + banked ROMs images */
	ROM_LOAD( "03",           0x00000, 0x08000, CRC(9a799c45) SHA1(cf6836108506929ee2449546a4867a7cbf00bcc8) )        /* CODE */
	ROM_LOAD( "a_14e.rom",    0x10000, 0x08000, CRC(4925ed03) SHA1(b11dbd9889db89cff008ca21beb6b1b70d983e16) )        /* 0+1 */
	ROM_LOAD( "a_12e.rom",    0x18000, 0x08000, CRC(81d0ece7) SHA1(5c1d154f9c1de6b5f5d7abf5d413e9c493461e6f) )        /* 2+3 */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for the audio CPU */
	ROM_LOAD( "a_04k.rom",    0x0000, 0x8000, CRC(34efe2d2) SHA1(e1d8895c113e4dee1a132e2471d75dfa6c36b620) )

	ROM_REGION( 0x08000, REGION_USER1, 0 )    /* starfield data */
	ROM_LOAD( "b_11j.rom",    0x0000, 0x8000, CRC(134dc35b) SHA1(6360c1efa7c4e1d6d817a97ca43dd4af8ed6afe5) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "a_10j.rom",    0x00000, 0x4000, CRC(651fef75) SHA1(9c821a2ee30c222987f0d4192133776490d6a4e0) ) /* characters */

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "b_13d.rom",    0x00000, 0x8000, CRC(3c59afe1) SHA1(5459a5795cf13012674993aa55bbd39e9a5c2f1b) ) /* tiles */
	ROM_LOAD( "b_13e.rom",    0x08000, 0x8000, CRC(64bc3b77) SHA1(54fe6f258fda509a92eb0f5aa238102efce729e0) )
	ROM_LOAD( "b_13f.rom",    0x10000, 0x8000, CRC(e6bcea6f) SHA1(19477e284967beafc4e7cd0d0da3534eb6dec388) )
	ROM_LOAD( "b_13g.rom",    0x18000, 0x8000, CRC(c71a3053) SHA1(963e105aa0b0174e8aa5e1f7676c5c604ca72d1c) )
	ROM_LOAD( "b_14d.rom",    0x20000, 0x8000, CRC(826e8a97) SHA1(ad5ed9a81805dde54fb2703345b2ab7b56853ec6) )
	ROM_LOAD( "b_14e.rom",    0x28000, 0x8000, CRC(6cfc02a4) SHA1(491e880e85d5256fa2eea6d0fb402f0a1176b675) )
	ROM_LOAD( "b_14f.rom",    0x30000, 0x8000, CRC(9b9f6730) SHA1(0f8fe5dc32ee50ebb2051c0c0c4d635582416317) )
	ROM_LOAD( "b_14g.rom",    0x38000, 0x8000, CRC(ef6af630) SHA1(499b17eeb5e7256ede477510b0547df520316996) )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "b_11b.rom",    0x00000, 0x8000, CRC(eb6f278c) SHA1(15e250aa98ee69ac3983d4511976c35833b37cab) ) /* sprites */
	ROM_LOAD( "b_13b.rom",    0x08000, 0x8000, CRC(e91b4014) SHA1(6557344ce8bc05309ab8ebe846871ed554b256b8) )
	ROM_LOAD( "b_11a.rom",    0x10000, 0x8000, CRC(2822c522) SHA1(00b3cab899e5ac1af6300f2ec2a54303df9ab014) )
	ROM_LOAD( "b_13a.rom",    0x18000, 0x8000, CRC(3e8a9f75) SHA1(b1bfb7604791950aa0454b68b24f6ad3b9131be8) )
	ROM_LOAD( "b_12b.rom",    0x20000, 0x8000, CRC(86e43eda) SHA1(c33b0ab6f7f0f886410a3943988b737d175635be) )
	ROM_LOAD( "b_14b.rom",    0x28000, 0x8000, CRC(076e92d1) SHA1(27144834b5b2849be8c46e97aaaeaa8b304ea810) )
	ROM_LOAD( "b_12a.rom",    0x30000, 0x8000, CRC(ce107f3c) SHA1(2235281449247cb2446b008b36077788c5b15026) )
	ROM_LOAD( "b_14a.rom",    0x38000, 0x8000, CRC(dba06076) SHA1(87b3b3437bc4bd727ce7e34dd914e6fe23bcac3d) )

	ROM_REGION( 0x08000, REGION_GFX4, 0 )	/* background tilemaps */
	ROM_LOAD( "b_03d.rom",    0x0000, 0x8000, CRC(6f348008) SHA1(b500bc32ba47e9cc9dcf2254b9455ac4d61992db) )

	ROM_REGION( 0x0320, REGION_PROMS, 0 )
	ROM_LOAD( "63s141.16h",   0x0000, 0x0100, CRC(75af3553) SHA1(14da009592877a6097b34ea844fa897ceda7465e) )	/* timing*/
	ROM_LOAD( "63s141.11h",   0x0100, 0x0100, CRC(a6e4d68f) SHA1(b9367e0c959cdf0397d33a49d778a66a407572b7) )	/* color mixing*/
	ROM_LOAD( "63s141.15h",   0x0200, 0x0100, CRC(c47c182a) SHA1(47d6139256e6838f633a04084bd0a7a84912f7fb) )	/* timing*/
	ROM_LOAD( "63s081.3j",    0x0300, 0x0020, CRC(c5817816) SHA1(cc642daafa0bcb160ee04e74e2d168fd44087608) )	/* unknown*/
ROM_END

ROM_START( sidearjp )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )     /* 64k for code + banked ROMs images */
	ROM_LOAD( "a_15e.rom",    0x00000, 0x08000, CRC(61ceb0cc) SHA1(bacf28e5e02b90a9d404c3ade0267e0a7cd73cd8) )        /* CODE */
	ROM_LOAD( "a_14e.rom",    0x10000, 0x08000, CRC(4925ed03) SHA1(b11dbd9889db89cff008ca21beb6b1b70d983e16) )        /* 0+1 */
	ROM_LOAD( "a_12e.rom",    0x18000, 0x08000, CRC(81d0ece7) SHA1(5c1d154f9c1de6b5f5d7abf5d413e9c493461e6f) )        /* 2+3 */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for the audio CPU */
	ROM_LOAD( "a_04k.rom",    0x0000, 0x8000, CRC(34efe2d2) SHA1(e1d8895c113e4dee1a132e2471d75dfa6c36b620) )

	ROM_REGION( 0x08000, REGION_USER1, 0 )    /* starfield data */
	ROM_LOAD( "b_11j.rom",    0x0000, 0x8000, CRC(134dc35b) SHA1(6360c1efa7c4e1d6d817a97ca43dd4af8ed6afe5) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "a_10j.rom",    0x00000, 0x4000, CRC(651fef75) SHA1(9c821a2ee30c222987f0d4192133776490d6a4e0) ) /* characters */

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "b_13d.rom",    0x00000, 0x8000, CRC(3c59afe1) SHA1(5459a5795cf13012674993aa55bbd39e9a5c2f1b) ) /* tiles */
	ROM_LOAD( "b_13e.rom",    0x08000, 0x8000, CRC(64bc3b77) SHA1(54fe6f258fda509a92eb0f5aa238102efce729e0) )
	ROM_LOAD( "b_13f.rom",    0x10000, 0x8000, CRC(e6bcea6f) SHA1(19477e284967beafc4e7cd0d0da3534eb6dec388) )
	ROM_LOAD( "b_13g.rom",    0x18000, 0x8000, CRC(c71a3053) SHA1(963e105aa0b0174e8aa5e1f7676c5c604ca72d1c) )
	ROM_LOAD( "b_14d.rom",    0x20000, 0x8000, CRC(826e8a97) SHA1(ad5ed9a81805dde54fb2703345b2ab7b56853ec6) )
	ROM_LOAD( "b_14e.rom",    0x28000, 0x8000, CRC(6cfc02a4) SHA1(491e880e85d5256fa2eea6d0fb402f0a1176b675) )
	ROM_LOAD( "b_14f.rom",    0x30000, 0x8000, CRC(9b9f6730) SHA1(0f8fe5dc32ee50ebb2051c0c0c4d635582416317) )
	ROM_LOAD( "b_14g.rom",    0x38000, 0x8000, CRC(ef6af630) SHA1(499b17eeb5e7256ede477510b0547df520316996) )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "b_11b.rom",    0x00000, 0x8000, CRC(eb6f278c) SHA1(15e250aa98ee69ac3983d4511976c35833b37cab) ) /* sprites */
	ROM_LOAD( "b_13b.rom",    0x08000, 0x8000, CRC(e91b4014) SHA1(6557344ce8bc05309ab8ebe846871ed554b256b8) )
	ROM_LOAD( "b_11a.rom",    0x10000, 0x8000, CRC(2822c522) SHA1(00b3cab899e5ac1af6300f2ec2a54303df9ab014) )
	ROM_LOAD( "b_13a.rom",    0x18000, 0x8000, CRC(3e8a9f75) SHA1(b1bfb7604791950aa0454b68b24f6ad3b9131be8) )
	ROM_LOAD( "b_12b.rom",    0x20000, 0x8000, CRC(86e43eda) SHA1(c33b0ab6f7f0f886410a3943988b737d175635be) )
	ROM_LOAD( "b_14b.rom",    0x28000, 0x8000, CRC(076e92d1) SHA1(27144834b5b2849be8c46e97aaaeaa8b304ea810) )
	ROM_LOAD( "b_12a.rom",    0x30000, 0x8000, CRC(ce107f3c) SHA1(2235281449247cb2446b008b36077788c5b15026) )
	ROM_LOAD( "b_14a.rom",    0x38000, 0x8000, CRC(dba06076) SHA1(87b3b3437bc4bd727ce7e34dd914e6fe23bcac3d) )

	ROM_REGION( 0x08000, REGION_GFX4, 0 )	/* background tilemaps */
	ROM_LOAD( "b_03d.rom",    0x0000, 0x8000, CRC(6f348008) SHA1(b500bc32ba47e9cc9dcf2254b9455ac4d61992db) )

	ROM_REGION( 0x0320, REGION_PROMS, 0 )
	ROM_LOAD( "63s141.16h",   0x0000, 0x0100, CRC(75af3553) SHA1(14da009592877a6097b34ea844fa897ceda7465e) )	/* timing*/
	ROM_LOAD( "63s141.11h",   0x0100, 0x0100, CRC(a6e4d68f) SHA1(b9367e0c959cdf0397d33a49d778a66a407572b7) )	/* color mixing*/
	ROM_LOAD( "63s141.15h",   0x0200, 0x0100, CRC(c47c182a) SHA1(47d6139256e6838f633a04084bd0a7a84912f7fb) )	/* timing*/
	ROM_LOAD( "63s081.3j",    0x0300, 0x0020, CRC(c5817816) SHA1(cc642daafa0bcb160ee04e74e2d168fd44087608) )	/* unknown*/
ROM_END

ROM_START( turtship )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )     /* 64k for code + banked ROMs images */
	ROM_LOAD( "turtship.003",    0x00000, 0x08000, CRC(e7a7fc2e) SHA1(1a9147e82a5e56e8e5b68bbce144f96261e88669) )
	ROM_LOAD( "turtship.002",    0x10000, 0x08000, CRC(e576f482) SHA1(3be3792cb437bff0345681a3a2fdefefa3439357) )
	ROM_LOAD( "turtship.001",    0x18000, 0x08000, CRC(a9b64240) SHA1(38c59877de6055230c3250ef74abc97e4ed88cb6) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for the audio CPU */
	ROM_LOAD( "turtship.004",    0x0000, 0x8000, CRC(1cbe48e8) SHA1(6ac5981d36a44595bb8dc847c54c7be7b374f82c) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "turtship.005",    0x00000, 0x04000, CRC(651fef75) SHA1(9c821a2ee30c222987f0d4192133776490d6a4e0) )	/* characters */

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "turtship.008",    0x00000, 0x10000, CRC(e0658469) SHA1(931c41cd6af759b30f6018248c3bab4d544acb98) )	/* tiles */
	ROM_LOAD( "turtship.010",    0x10000, 0x10000, CRC(76bb73bb) SHA1(4c4acd205421674878948a0d2bed6032bde3f97f) )
	ROM_RELOAD( 0x30000, 0x10000)
	ROM_LOAD( "turtship.011",    0x20000, 0x10000, CRC(53da6cb1) SHA1(52720746298adb01828f959f81b385d268c94343) )
	ROM_LOAD( "turtship.006",    0x40000, 0x10000, CRC(a7cce654) SHA1(f6c99622dcacc1d76021ca29b0bbceefbb75c499) )
	ROM_LOAD( "turtship.007",    0x50000, 0x10000, CRC(3ccf11b9) SHA1(777cc853bfcf2db4027b35d516fa5bef8b010e63) )
	ROM_RELOAD( 0x70000, 0x10000)
	ROM_LOAD( "turtship.009",    0x60000, 0x10000, CRC(44762916) SHA1(3427066fc02d1b9b71a59ac41d3332d5cd8d1423) )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "turtship.013",    0x00000, 0x10000, CRC(599f5246) SHA1(b7e5bbff3b6117613744970c8680b7bc171516bd) )	/* sprites */
	ROM_LOAD( "turtship.015",    0x10000, 0x10000, CRC(69fd202f) SHA1(67d7d6d08f5daa0460ce51516f1d27dfd6aef297) )
	ROM_LOAD( "turtship.012",    0x20000, 0x10000, CRC(fb54cd33) SHA1(49f7b728a4de8b93f5fd929f59a65509e4556161) )
	ROM_LOAD( "turtship.014",    0x30000, 0x10000, CRC(b3ea74a3) SHA1(aa347a6cd75408a3ba4ce26d3e1015a1be1faa64) )

	ROM_REGION( 0x08000, REGION_GFX4, 0 )	/* background tilemaps */
	ROM_LOAD( "turtship.016",    0x0000, 0x8000, CRC(affd51dd) SHA1(3338aa1fdd6b9926acc215f7f3656d70803f1832) )
ROM_END

ROM_START( dyger )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )     /* 64k for code + banked ROMs images */
	ROM_LOAD( "dyger.003",    0x00000, 0x08000, CRC(bae9882e) SHA1(88194e58673ebd0841e9e07482842f6dbb823afc) )
	ROM_LOAD( "dyger.002",    0x10000, 0x08000, CRC(059ac4dc) SHA1(fe46d819946e168b4a8188302737fdde957743ea) )
	ROM_LOAD( "dyger.001",    0x18000, 0x08000, CRC(d8440f66) SHA1(3b2ee8c09d40edbe76d5004ed9074add0d4e4fd0) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for the audio CPU */
	ROM_LOAD( "dyger.004",    0x0000, 0x8000, CRC(8a256c09) SHA1(2c692af62da7c12b7d4f3f79264ee045a2cfa39f) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "dyger.005",    0x00000, 0x04000, CRC(c4bc72a5) SHA1(ee4ac5cbc9e97dd6fd0c9f507ee22a3eb36ba1b2) )	/* characters */
	ROM_CONTINUE(             0x00000, 0x04000 )	/* is the first half used? */

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "dyger.010",    0x00000, 0x10000, CRC(9715880d) SHA1(a6a400a0f4a80f3d151851a8ed182a6695a468b7) )	/* tiles */
	ROM_LOAD( "dyger.009",    0x10000, 0x10000, CRC(628dae72) SHA1(5cfd5b87f702650afaf0999a45670f956b8254b2) )
	ROM_RELOAD( 0x30000, 0x10000)
	ROM_LOAD( "dyger.011",    0x20000, 0x10000, CRC(23248db1) SHA1(47c5ef86e74be142faa0b896749d964ea1adc958) )
	ROM_LOAD( "dyger.006",    0x40000, 0x10000, CRC(4ba7a437) SHA1(14bd939e3c5c28c5c7379e57832a0d3d707984f7) )
	ROM_LOAD( "dyger.008",    0x50000, 0x10000, CRC(6c0f0e0c) SHA1(aac2b31346ebc6f2fb664faca732cd3738efcbab) )
	ROM_RELOAD( 0x70000, 0x10000)
	ROM_LOAD( "dyger.007",    0x60000, 0x10000, CRC(2c50a229) SHA1(14498a06ec7c683c161f46633b270548ca8a9b85) )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "dyger.014",    0x00000, 0x10000, CRC(99c60b26) SHA1(bcd56df5ef93c6133b61bce6472a708e340fbaaf) )	/* sprites */
	ROM_LOAD( "dyger.015",    0x10000, 0x10000, CRC(d6475ecc) SHA1(61f6a9b443810742a2d39e61d14b92924cc27da7) )
	ROM_LOAD( "dyger.012",    0x20000, 0x10000, CRC(e345705f) SHA1(0c51c0c598c0f51268108c7351b1b24977ae2b9f) )
	ROM_LOAD( "dyger.013",    0x30000, 0x10000, CRC(faf4be3a) SHA1(dcf1958a17b587845174374f9598d0a979d7a6d5) )

	ROM_REGION( 0x08000, REGION_GFX4, 0 )	/* background tilemaps */
	ROM_LOAD( "dyger.016",    0x0000, 0x8000, CRC(0792e8f2) SHA1(3716839502679ecc973571d824065b40771d5bfa) )
ROM_END

ROM_START( dygera )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )     /* 64k for code + banked ROMs images */
	ROM_LOAD( "dygar_t3.bin", 0x00000, 0x08000, CRC(fc63da8b) SHA1(f324a314cda167ae05e2eb017da355709489a7a3) )
	ROM_LOAD( "dyger.002",    0x10000, 0x08000, CRC(059ac4dc) SHA1(fe46d819946e168b4a8188302737fdde957743ea) )
	ROM_LOAD( "dyger.001",    0x18000, 0x08000, CRC(d8440f66) SHA1(3b2ee8c09d40edbe76d5004ed9074add0d4e4fd0) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for the audio CPU */
	ROM_LOAD( "dyger.004",    0x0000, 0x8000, CRC(8a256c09) SHA1(2c692af62da7c12b7d4f3f79264ee045a2cfa39f) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "dyger.005",    0x00000, 0x04000, CRC(c4bc72a5) SHA1(ee4ac5cbc9e97dd6fd0c9f507ee22a3eb36ba1b2) )	/* characters */
	ROM_CONTINUE(             0x00000, 0x04000 )	/* is the first half used? */

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "dyger.010",    0x00000, 0x10000, CRC(9715880d) SHA1(a6a400a0f4a80f3d151851a8ed182a6695a468b7) )	/* tiles */
	ROM_LOAD( "dyger.009",    0x10000, 0x10000, CRC(628dae72) SHA1(5cfd5b87f702650afaf0999a45670f956b8254b2) )
	ROM_RELOAD( 0x30000, 0x10000)
	ROM_LOAD( "dyger.011",    0x20000, 0x10000, CRC(23248db1) SHA1(47c5ef86e74be142faa0b896749d964ea1adc958) )
	ROM_LOAD( "dyger.006",    0x40000, 0x10000, CRC(4ba7a437) SHA1(14bd939e3c5c28c5c7379e57832a0d3d707984f7) )
	ROM_LOAD( "dyger.008",    0x50000, 0x10000, CRC(6c0f0e0c) SHA1(aac2b31346ebc6f2fb664faca732cd3738efcbab) )
	ROM_RELOAD( 0x70000, 0x10000)
	ROM_LOAD( "dyger.007",    0x60000, 0x10000, CRC(2c50a229) SHA1(14498a06ec7c683c161f46633b270548ca8a9b85) )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "dyger.014",    0x00000, 0x10000, CRC(99c60b26) SHA1(bcd56df5ef93c6133b61bce6472a708e340fbaaf) )	/* sprites */
	ROM_LOAD( "dyger.015",    0x10000, 0x10000, CRC(d6475ecc) SHA1(61f6a9b443810742a2d39e61d14b92924cc27da7) )
	ROM_LOAD( "dyger.012",    0x20000, 0x10000, CRC(e345705f) SHA1(0c51c0c598c0f51268108c7351b1b24977ae2b9f) )
	ROM_LOAD( "dyger.013",    0x30000, 0x10000, CRC(faf4be3a) SHA1(dcf1958a17b587845174374f9598d0a979d7a6d5) )

	ROM_REGION( 0x08000, REGION_GFX4, 0 )	/* background tilemaps */
	ROM_LOAD( "dyger.016",    0x0000, 0x8000, CRC(0792e8f2) SHA1(3716839502679ecc973571d824065b40771d5bfa) )
ROM_END

ROM_START( whizz )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )     /* 64k for code + banked ROMs images */
	ROM_LOAD( "whizz.t15",    0x00000, 0x08000, CRC(73161302) SHA1(de815bba66c376cea775139f4285de0b1a589d88) )
	ROM_LOAD( "whizz.t14",    0x10000, 0x10000, CRC(bf248879) SHA1(f46f15e3949221e59d8c37de9c23473a74c2927e) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for the audio CPU */
	ROM_LOAD( "whizz.t1",     0x0000, 0x8000, CRC(b84bc980) SHA1(d2d302a96a9e3197f27144e525a901cfb9da09e4) )

	ROM_REGION( 0x8000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "whizz.t6",     0x04000, 0x04000, CRC(8e4ca776) SHA1(412a47f030e3b491e23e5696ef88d065f9de0220) )	/* characters */
	ROM_CONTINUE(             0x00000, 0x04000 )	/* is the first half used? */

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "whizz.t10",    0x00000, 0x10000, CRC(b678ef5b) SHA1(cdddd2a033291585e25839e864e898ef36f4d287) )
	ROM_LOAD( "whizz.t9",     0x10000, 0x10000, CRC(d7345fb9) SHA1(9da907c2bcacc750426a2989bae3c3e5fcc3e3ab) )
	ROM_RELOAD( 0x30000, 0x10000)
	ROM_LOAD( "whizz.t8",     0x20000, 0x10000, CRC(41428dac) SHA1(16ae6c178b91e5cd859deb13176b7333f05c378a) )
	ROM_LOAD( "whizz.t13",    0x40000, 0x10000, CRC(0eba10bd) SHA1(e2504a5576c6af6c5bdb0263e1d3cb9ccabde3f8) )
	ROM_LOAD( "whizz.t12",    0x50000, 0x10000, CRC(c65050ce) SHA1(f90616aa4e1f80d8d7fccf5748f564cb7bc2d83a) )
	ROM_RELOAD( 0x70000, 0x10000)
	ROM_LOAD( "whizz.t11",    0x60000, 0x10000, CRC(51a2c65d) SHA1(a89f46d581d2907b7813454925ce690af007997d) )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "whizz.t2",    0x00000, 0x10000, CRC(9c106835) SHA1(7e032e65e78c380b5f03a4febd6dcd3f0bdb642b) )	/* sprites */
	ROM_LOAD( "whizz.t3",    0x10000, 0x10000, CRC(9b421ccf) SHA1(0365d48437da0f90c1c146da0605139a3da0b03b) )
	ROM_LOAD( "whizz.t4",    0x20000, 0x10000, CRC(3a1db986) SHA1(5435e891eebe5b95a5a97ee8743a8a10282e4d19) )
	ROM_LOAD( "whizz.t5",    0x30000, 0x10000, CRC(9bd22190) SHA1(7a571becde02ea4b64db4138f00408f312bf54c0) )

	ROM_REGION( 0x08000, REGION_GFX4, 0 )	/* background tilemaps */
	ROM_LOAD( "whizz.t7",    0x0000, 0x8000, CRC(a8b5f750) SHA1(94eb7af3cb8bee87ce3d31260e3bde062ebbc8f0) )
ROM_END

static DRIVER_INIT( sidearms ) { sidearms_gameid = 0; }
static DRIVER_INIT( turtship ) { sidearms_gameid = 1; }
static DRIVER_INIT( dyger    ) { sidearms_gameid = 2; }
static DRIVER_INIT( whizz    ) { sidearms_gameid = 3; }

GAMEX(1986, sidearms, 0,        sidearms, sidearms, sidearms, ROT0,   "Capcom", "Side Arms - Hyper Dyne (World)", GAME_IMPERFECT_GRAPHICS )
GAMEX(1986, sidearmr, sidearms, sidearms, sidearms, sidearms, ROT0,   "Capcom (Romstar license)", "Side Arms - Hyper Dyne (US)", GAME_IMPERFECT_GRAPHICS )
GAMEX(1986, sidearjp, sidearms, sidearms, sidearms, sidearms, ROT0,   "Capcom", "Side Arms - Hyper Dyne (Japan)", GAME_IMPERFECT_GRAPHICS )
GAME( 1988, turtship, 0,        turtship, turtship, turtship, ROT0,   "Philko", "Turtle Ship" )
GAME( 1989, dyger,    0,        turtship, dyger,    dyger,    ROT270, "Philko", "Dyger (Korea set 1)" )
GAME( 1989, dygera,   dyger,    turtship, dyger,    dyger,    ROT270, "Philko", "Dyger (Korea set 2)" )
GAME( 1989, whizz,    0,        whizz, 	  whizz,    whizz,	  ROT0,   "Philko", "Whizz" )
