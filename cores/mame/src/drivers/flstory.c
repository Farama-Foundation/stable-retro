/***************************************************************************

The FairyLand Story

TODO:
- game behaves VERY strangely in attract mode. Wait until the demo game will
  be shown. Insert a coin - game screen will be replaced with "start screen"
  but you can still see demo sprites and hear the in-game sounds.
- TA7630 emulation needs filter support (bass sounds from MSM5232 should be about 2 times louder)

***************************************************************************/

#include <math.h>
#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"

VIDEO_START( flstory );
VIDEO_UPDATE( flstory );
VIDEO_START( rumba );
VIDEO_UPDATE( rumba );

extern UINT8 *flstory_scrlram;

WRITE_HANDLER( flstory_videoram_w );
READ_HANDLER( flstory_palette_r );
WRITE_HANDLER( flstory_palette_w );
WRITE_HANDLER( flstory_gfxctrl_w );
READ_HANDLER( flstory_scrlram_r );
WRITE_HANDLER( flstory_scrlram_w );
WRITE_HANDLER( rumba_gfxctrl_w );

READ_HANDLER( flstory_68705_portA_r );
WRITE_HANDLER( flstory_68705_portA_w );
READ_HANDLER( flstory_68705_portB_r );
WRITE_HANDLER( flstory_68705_portB_w );
READ_HANDLER( flstory_68705_portC_r );
WRITE_HANDLER( flstory_68705_portC_w );
WRITE_HANDLER( flstory_68705_ddrA_w );
WRITE_HANDLER( flstory_68705_ddrB_w );
WRITE_HANDLER( flstory_68705_ddrC_w );
WRITE_HANDLER( flstory_mcu_w );
READ_HANDLER( flstory_mcu_r );
READ_HANDLER( flstory_mcu_status_r );


static UINT8 snd_data;
static UINT8 snd_flag;

static READ_HANDLER( from_snd_r )
{
	snd_flag = 0;
	return snd_data;
}

static READ_HANDLER( snd_flag_r )
{
	return snd_flag | 0xfd;
}

static WRITE_HANDLER( to_main_w )
{
	snd_data = data;
	snd_flag = 2;
}


static int sound_nmi_enable,pending_nmi;

static void nmi_callback(int param)
{
	if (sound_nmi_enable) cpu_set_irq_line(1,IRQ_LINE_NMI,PULSE_LINE);
	else pending_nmi = 1;
}

static WRITE_HANDLER( sound_command_w )
{
	soundlatch_w(0,data);
	timer_set(TIME_NOW,data,nmi_callback);
}


static WRITE_HANDLER( nmi_disable_w )
{
	sound_nmi_enable = 0;
}

static WRITE_HANDLER( nmi_enable_w )
{
	sound_nmi_enable = 1;
	if (pending_nmi)
	{
		cpu_set_irq_line(1,IRQ_LINE_NMI,PULSE_LINE);
		pending_nmi = 0;
	}
}


static MEMORY_READ_START( readmem )
    { 0x0000, 0xbfff, MRA_ROM },
    { 0xc000, 0xc7ff, MRA_RAM },
    { 0xc800, 0xcfff, MRA_RAM }, /* unknown */
	{ 0xd000, 0xd000, flstory_mcu_r },
	{ 0xd400, 0xd400, from_snd_r },
	{ 0xd401, 0xd401, snd_flag_r },
	{ 0xd403, 0xd403, MRA_NOP }, /* unknown */
	{ 0xd800, 0xd800, input_port_0_r },
	{ 0xd801, 0xd801, input_port_1_r },
	{ 0xd802, 0xd802, input_port_2_r },
	{ 0xd803, 0xd803, input_port_3_r },
	{ 0xd804, 0xd804, input_port_4_r },
	{ 0xd805, 0xd805, flstory_mcu_status_r },
	{ 0xd806, 0xd806, input_port_5_r },
	{ 0xdc00, 0xdcff, MRA_RAM }, /* spriteram / scrollram */
	{ 0xdd00, 0xdeff, flstory_palette_r },
    { 0xe000, 0xe7ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc7ff, flstory_videoram_w, &videoram, &videoram_size },
	{ 0xc800, 0xcfff, MWA_RAM },
	{ 0xd000, 0xd000, flstory_mcu_w },
	{ 0xd001, 0xd001, MWA_NOP },	/* watchdog? */
	{ 0xd002, 0xd002, MWA_NOP },	/* coin lock out? */
	{ 0xd400, 0xd400, sound_command_w },
	{ 0xd403, 0xd403, MWA_NOP },	/* unknown */
/*	{ 0xda00, 0xda00, MWA_RAM },*/
	{ 0xdc00, 0xdc9f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xdca0, 0xdcbf, flstory_scrlram_w, &flstory_scrlram },
	{ 0xdcc0, 0xdcff, MWA_RAM }, /* unknown */
	{ 0xdd00, 0xdeff, flstory_palette_w },
	{ 0xdf03, 0xdf03, flstory_gfxctrl_w },
	{ 0xe000, 0xe7ff, MWA_RAM },	/* work RAM */
MEMORY_END

/* Using Fairyland Story MCU hookup for Rumba Lumba and
Onna Sansirou - Typhoon Gal, 95% of Taito 68705 MCU's 
all use the same address values so it's highly likely 
to be correct anyhow.
*/

static MEMORY_READ_START( onna34ro_readmem )
    { 0x0000, 0xbfff, MRA_ROM },
    { 0xc000, 0xc7ff, MRA_RAM },
    { 0xc800, 0xcfff, MRA_RAM }, /* unknown */
	{ 0xd000, 0xd000, flstory_mcu_r },
	{ 0xd400, 0xd400, from_snd_r },
	{ 0xd401, 0xd401, snd_flag_r },
	{ 0xd403, 0xd403, MRA_NOP }, /* unknown */
	{ 0xd800, 0xd800, input_port_0_r },
	{ 0xd801, 0xd801, input_port_1_r },
	{ 0xd802, 0xd802, input_port_2_r },
	{ 0xd803, 0xd803, input_port_3_r },
	{ 0xd804, 0xd804, input_port_4_r },
	{ 0xd805, 0xd805, flstory_mcu_status_r },
	{ 0xd806, 0xd806, input_port_5_r },
	{ 0xdc00, 0xdcff, MRA_RAM }, /* spriteram / scrollram */
	{ 0xdd00, 0xdeff, flstory_palette_r },
    { 0xe000, 0xe7ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( onna34ro_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc7ff, flstory_videoram_w, &videoram, &videoram_size },
	{ 0xc800, 0xcfff, MWA_RAM },
	{ 0xd000, 0xd000, flstory_mcu_w },
	{ 0xd001, 0xd001, MWA_NOP },	/* watchdog? */
	{ 0xd002, 0xd002, MWA_NOP },	/* coin lock out? */
	{ 0xd400, 0xd400, sound_command_w },
	{ 0xd403, 0xd403, MWA_NOP },	/* unknown */
/*	{ 0xda00, 0xda00, MWA_RAM }, */
	{ 0xdc00, 0xdc9f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xdca0, 0xdcbf, flstory_scrlram_w, &flstory_scrlram },
	{ 0xdcc0, 0xdcff, MWA_RAM }, /* unknown */
	{ 0xdd00, 0xdeff, flstory_palette_w },
	{ 0xdf03, 0xdf03, flstory_gfxctrl_w },
	{ 0xe000, 0xe7ff, MWA_RAM },	/* work RAM */
MEMORY_END

/* bootleg no mcu */
static MEMORY_READ_START( onna34ra_readmem )
    { 0x0000, 0xbfff, MRA_ROM },
    { 0xc000, 0xc7ff, MRA_RAM },
    { 0xc800, 0xcfff, MRA_RAM }, /* unknown */
/*	{ 0xd000, 0xd000, flstory_mcu_r }, */
	{ 0xd400, 0xd400, from_snd_r },
	{ 0xd401, 0xd401, snd_flag_r },
	{ 0xd403, 0xd403, MRA_NOP }, /* unknown */
	{ 0xd800, 0xd800, input_port_0_r },
	{ 0xd801, 0xd801, input_port_1_r },
	{ 0xd802, 0xd802, input_port_2_r },
	{ 0xd803, 0xd803, input_port_3_r },
	{ 0xd804, 0xd804, input_port_4_r },
/*	{ 0xd805, 0xd805, flstory_mcu_status_r }, */
	{ 0xd806, 0xd806, input_port_5_r },
	{ 0xdc00, 0xdcff, MRA_RAM }, /* spriteram / scrollram */
	{ 0xdd00, 0xdeff, flstory_palette_r },
    { 0xe000, 0xe7ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( onna34ra_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc7ff, flstory_videoram_w, &videoram, &videoram_size },
	{ 0xc800, 0xcfff, MWA_RAM },
/*	{ 0xd000, 0xd000, flstory_mcu_w }, */
	{ 0xd001, 0xd001, MWA_NOP },	/* watchdog? */
	{ 0xd002, 0xd002, MWA_NOP },	/* coin lock out? */
	{ 0xd400, 0xd400, sound_command_w },
	{ 0xd403, 0xd403, MWA_NOP },	/* unknown */
/*	{ 0xda00, 0xda00, MWA_RAM },*/
	{ 0xdc00, 0xdc9f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xdca0, 0xdcbf, flstory_scrlram_w, &flstory_scrlram },
	{ 0xdcc0, 0xdcff, MWA_RAM }, /* unknown */
	{ 0xdd00, 0xdeff, flstory_palette_w },
	{ 0xdf03, 0xdf03, flstory_gfxctrl_w },
	{ 0xe000, 0xe7ff, MWA_RAM },	/* work RAM */
MEMORY_END


static MEMORY_READ_START( rumba_readmem )
    { 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
/*	{ 0xc800, 0xcfff, MRA_RAM }, unknown */
    { 0xd000, 0xd000, flstory_mcu_r },
	{ 0xd400, 0xd400, from_snd_r },
	{ 0xd401, 0xd401, snd_flag_r },
/*	{ 0xd403, 0xd403, MRA_NOP }, unknown */
    { 0xd800, 0xd800, input_port_0_r },
	{ 0xd801, 0xd801, input_port_1_r },
	{ 0xd802, 0xd802, input_port_2_r },
	{ 0xd803, 0xd803, input_port_3_r },
	{ 0xd804, 0xd804, input_port_4_r },
	{ 0xd805, 0xd805, flstory_mcu_status_r },
	{ 0xd806, 0xd806, input_port_6_r },
	{ 0xd807, 0xd807, input_port_7_r },
	{ 0xdc00, 0xdc9f, MRA_RAM }, /* spriteram / scrollram */
	{ 0xdd00, 0xdeff, flstory_palette_r },
	{ 0xe000, 0xe7ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( rumba_writemem )
    { 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc7ff, flstory_videoram_w, &videoram, &videoram_size },
/*	{ 0xc800, 0xcfff, MWA_RAM }, */
    { 0xd000, 0xd000, flstory_mcu_w },
	{ 0xd001, 0xd001, MWA_NOP },	/* watchdog? */
/*	{ 0xd002, 0xd002, MWA_NOP },	 coin lock out? */
    { 0xd400, 0xd400, sound_command_w },
	{ 0xd403, 0xd403, MWA_NOP },	/* unknown */
/*	{ 0xda00, 0xda00, MWA_RAM }, */
    { 0xdc00, 0xdc9f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xdca0, 0xdcbf, flstory_scrlram_w, &flstory_scrlram },
	{ 0xdd00, 0xdeff, flstory_palette_w },
	{ 0xdce0, 0xdce0, rumba_gfxctrl_w },
	{ 0xe000, 0xe7ff, MWA_RAM },	/* work RAM */
MEMORY_END

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xd800, 0xd800, soundlatch_r },
	{ 0xda00, 0xda00, MRA_NOP }, /* unknown */
	{ 0xde00, 0xde00, MRA_NOP }, /* unknown */
	{ 0xe000, 0xefff, MRA_ROM },	/* space for diagnostics ROM */
MEMORY_END

static int vol_ctrl[16];

static MACHINE_INIT( ta7630 )
{
	int i;

	double db			= 0.0;
	double db_step		= 1.50;	/* 1.50 dB step (at least, maybe more) */
	double db_step_inc	= 0.125;
	for (i=0; i<16; i++)
	{
		double max = 100.0 / pow(10.0, db/20.0 );
		vol_ctrl[ 15-i ] = max;
		/*logerror("vol_ctrl[%x] = %i (%f dB)\n",15-i,vol_ctrl[ 15-i ],db);*/
		db += db_step;
		db_step += db_step_inc;
	}

	/* for (i=0; i<8; i++)
		log_cb(RETRO_LOG_DEBUG, LOGPRE "SOUND Chan#%i name=%s\n", i, mixer_get_name(i) ); */
/*
  channels 0-2 AY#0
  channels 3,4 MSM5232 group1,group2
*/
}

static UINT8 snd_ctrl0=0;
static UINT8 snd_ctrl1=0;
static UINT8 snd_ctrl2=0;
static UINT8 snd_ctrl3=0;

static WRITE_HANDLER( sound_control_0_w )
{
	snd_ctrl0 = data & 0xff;
/*	usrintf_showmessage("SND0 0=%02x 1=%02x 2=%02x 3=%02x", snd_ctrl0, snd_ctrl1, snd_ctrl2, snd_ctrl3);*/

	/* this definitely controls main melody voice on 2'-1 and 4'-1 outputs */
	mixer_set_volume (3, vol_ctrl[ (snd_ctrl0>>4) & 15 ]);	/* group1 from msm5232 */

}
static WRITE_HANDLER( sound_control_1_w )
{
	snd_ctrl1 = data & 0xff;
/*	usrintf_showmessage("SND1 0=%02x 1=%02x 2=%02x 3=%02x", snd_ctrl0, snd_ctrl1, snd_ctrl2, snd_ctrl3);*/
	mixer_set_volume (4, vol_ctrl[ (snd_ctrl1>>4) & 15 ]);	/* group2 from msm5232 */
}

static WRITE_HANDLER( sound_control_2_w )
{
	int i;

	snd_ctrl2 = data & 0xff;
/*	usrintf_showmessage("SND2 0=%02x 1=%02x 2=%02x 3=%02x", snd_ctrl0, snd_ctrl1, snd_ctrl2, snd_ctrl3);*/

	for (i=0; i<3; i++)
		mixer_set_volume (i, vol_ctrl[ (snd_ctrl2>>4) & 15 ]);	/* ym2149f all */
}

static WRITE_HANDLER( sound_control_3_w ) /* unknown */
{
	snd_ctrl3 = data & 0xff;
/*	usrintf_showmessage("SND3 0=%02x 1=%02x 2=%02x 3=%02x", snd_ctrl0, snd_ctrl1, snd_ctrl2, snd_ctrl3);*/
}


static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xc800, 0xc800, AY8910_control_port_0_w },
	{ 0xc801, 0xc801, AY8910_write_port_0_w },
	{ 0xca00, 0xca0d, MSM5232_0_w },
	{ 0xcc00, 0xcc00, sound_control_0_w },
	{ 0xce00, 0xce00, sound_control_1_w },
	{ 0xd800, 0xd800, to_main_w },
	{ 0xda00, 0xda00, nmi_enable_w },
	{ 0xdc00, 0xdc00, nmi_disable_w },
	{ 0xde00, 0xde00, DAC_0_signed_data_w },		/* signed 8-bit DAC */
	{ 0xe000, 0xefff, MWA_ROM },
MEMORY_END

static MEMORY_READ_START( m68705_readmem )
	{ 0x0000, 0x0000, flstory_68705_portA_r },
	{ 0x0001, 0x0001, flstory_68705_portB_r },
	{ 0x0002, 0x0002, flstory_68705_portC_r },
	{ 0x0010, 0x007f, MRA_RAM },
	{ 0x0080, 0x07ff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( m68705_writemem )
	{ 0x0000, 0x0000, flstory_68705_portA_w },
	{ 0x0001, 0x0001, flstory_68705_portB_w },
	{ 0x0002, 0x0002, flstory_68705_portC_w },
	{ 0x0004, 0x0004, flstory_68705_ddrA_w },
	{ 0x0005, 0x0005, flstory_68705_ddrB_w },
	{ 0x0006, 0x0006, flstory_68705_ddrC_w },
	{ 0x0010, 0x007f, MWA_RAM },
	{ 0x0080, 0x07ff, MWA_ROM },
MEMORY_END



/* When "Debug Mode" Dip Switch is ON, keep IPT_SERVICE1 ('9') pressed to freeze the game.
   Once the game is frozen, you can press IPT_START1 ('5') to advance 1 frame, or IPT_START2
   ('6') to advance 6 frames.

   When "Continue" Dip Switch is ON, you can only continue in a 1 player game AND when level
   (0xe781) is between 8 and 98 (included).
*/

INPUT_PORTS_START( flstory )
	PORT_START      /* D800: DSW0 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "30000 100000" )
	PORT_DIPSETTING(    0x01, "30000 150000" )
	PORT_DIPSETTING(    0x02, "50000 150000" )
	PORT_DIPSETTING(    0x03, "70000 150000" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x08, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x18, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x20, 0x20, "Debug Mode" )			/* Check code at 0x0679*/
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )

	PORT_START      /* D801: DSW1 */
	PORT_DIPNAME( 0x0f, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 9C_1C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 8C_1C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 7C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_8C ) )
	PORT_DIPNAME( 0xf0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 9C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 8C_1C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 7C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_8C ) )

	PORT_START      /* D802: COINS */
	PORT_BIT( 0x07, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_DIPNAME( 0x08, 0x08, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x10, 0x10, "Attract Animation" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )		/* Check code at 0x7859*/
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )		/* (must be OFF or the game will*/
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )			/* hang after the game is over !)*/
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Coin Slots" )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x80, "2" )

	PORT_START      /* D803: START BUTTONS */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* "BAD IO" if low */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* "BAD IO" if low */

	PORT_START      /* D804: P1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* D806: P2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( onna34ro )
	PORT_START      /* D800: DSW0 */
	PORT_DIPNAME(0x03, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(   0x00, "200000 200000" )
	PORT_DIPSETTING(   0x01, "200000 300000" )
	PORT_DIPSETTING(   0x02, "100000 200000" )
	PORT_DIPSETTING(   0x03, "200000 100000" )
	PORT_DIPNAME(0x04, 0x00, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(   0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x04, DEF_STR( On ) )
	PORT_DIPNAME(0x18, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(   0x10, "1" )
	PORT_DIPSETTING(   0x08, "2" )
	PORT_DIPSETTING(   0x00, "3" )
	PORT_BITX( 0,      0x18, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Endless", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME(0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(   0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x00, DEF_STR( On ) )
	PORT_DIPNAME(0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(   0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x00, DEF_STR( On ) )
	PORT_DIPNAME(0x80, 0x80, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(   0x80, DEF_STR( Upright ) )
	PORT_DIPSETTING(   0x00, DEF_STR( Cocktail ) )

	PORT_START      /* D801: DSW1 */
	PORT_DIPNAME(0x0f, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(   0x0f, DEF_STR( 9C_1C ) )
	PORT_DIPSETTING(   0x0e, DEF_STR( 8C_1C ) )
	PORT_DIPSETTING(   0x0d, DEF_STR( 7C_1C ) )
	PORT_DIPSETTING(   0x0c, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(   0x0b, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(   0x0a, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(   0x09, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(   0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(   0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(   0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(   0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(   0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(   0x04, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(   0x05, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(   0x06, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(   0x07, DEF_STR( 1C_8C ) )
	PORT_DIPNAME(0xf0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(   0xf0, DEF_STR( 9C_1C ) )
	PORT_DIPSETTING(   0xe0, DEF_STR( 8C_1C ) )
	PORT_DIPSETTING(   0xd0, DEF_STR( 7C_1C ) )
	PORT_DIPSETTING(   0xc0, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(   0xb0, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(   0xa0, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(   0x90, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(   0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(   0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(   0x10, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(   0x20, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(   0x30, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(   0x40, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(   0x50, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(   0x60, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(   0x70, DEF_STR( 1C_8C ) )

	PORT_START      /* D802: DSW2 */
	PORT_BITX(   0x01, 0x00, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(   0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x01, DEF_STR( On ) )
	PORT_DIPNAME(0x02, 0x00, "Rack Test" )
	PORT_DIPSETTING(   0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x02, DEF_STR( On ) )
	PORT_DIPNAME(0x04, 0x00, DEF_STR( Unknown ) ) /* demo sounds */
	PORT_DIPSETTING(   0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x00, DEF_STR( On ) )
	PORT_DIPNAME(0x08, 0x00, "Freeze" )
	PORT_DIPSETTING(   0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x08, DEF_STR( On ) )
	PORT_DIPNAME(0x10, 0x00, "Coinage Display" )
	PORT_DIPSETTING(   0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x00, DEF_STR( On ) )
	PORT_DIPNAME(0x60, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(   0x20, "Easy" )
	PORT_DIPSETTING(   0x00, "Normal" )
	PORT_DIPSETTING(   0x40, "Difficult" )
	PORT_DIPSETTING(   0x60, "Very Difficult" )
	PORT_DIPNAME(0x80, 0x80, DEF_STR( Coinage ) )
	PORT_DIPSETTING(   0x80, "A and B" )
	PORT_DIPSETTING(   0x00, "A only" )

	PORT_START      /* D803: START BUTTONS */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* "BAD IO" if low */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* "BAD IO" if low */

	PORT_START      /* D804: P1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* D806: P2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( rumba )
	PORT_START     /* D800 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "20000 50000" )
	PORT_DIPSETTING(    0x01, "10000 60000" )
	PORT_DIPSETTING(    0x02, "10000 40000" )
	PORT_DIPSETTING(    0x03, "10000 20000" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x18, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x08, "5" )
	PORT_DIPSETTING(    0x00, "6")
    PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )


	PORT_START     /* D801 */
	PORT_DIPNAME(0x0f, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(   0x0f, DEF_STR( 9C_1C ) )
	PORT_DIPSETTING(   0x0e, DEF_STR( 8C_1C ) )
	PORT_DIPSETTING(   0x0d, DEF_STR( 7C_1C ) )
	PORT_DIPSETTING(   0x0c, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(   0x0b, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(   0x0a, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(   0x09, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(   0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(   0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(   0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(   0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(   0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(   0x04, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(   0x05, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(   0x06, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(   0x07, DEF_STR( 1C_8C ) )
	PORT_DIPNAME(0xf0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(   0xf0, DEF_STR( 9C_1C ) )
	PORT_DIPSETTING(   0xe0, DEF_STR( 8C_1C ) )
	PORT_DIPSETTING(   0xd0, DEF_STR( 7C_1C ) )
	PORT_DIPSETTING(   0xc0, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(   0xb0, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(   0xa0, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(   0x90, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(   0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(   0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(   0x10, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(   0x20, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(   0x30, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(   0x40, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(   0x50, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(   0x60, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(   0x70, DEF_STR( 1C_8C ) )

	PORT_START     /* D802 */
    PORT_DIPNAME( 0x01,   0x01, "Training Stage" )
    PORT_DIPSETTING(      0x00, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02,   0x02, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x02, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x04,   0x00, "Language" )
    PORT_DIPSETTING(      0x04, "Japanese" )
    PORT_DIPSETTING(      0x00, "English" )
	PORT_DIPNAME( 0x08,   0x00, "Attract Sound" ) /* At title sequence only - NOT Demo Sounds */
	PORT_DIPSETTING(      0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10,   0x00, "Coinage Display" )
	PORT_DIPSETTING(      0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x20,   0x20, "Copyright String" )
    PORT_DIPSETTING(      0x20, "Taito Corp. MCMLXXXIV" )
    PORT_DIPSETTING(      0x00, "Taito Corporation" )
    PORT_DIPNAME( 0x40,   0x40, "Infinite Lives" ) /* ??? */
    PORT_DIPSETTING(      0x40, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x80,   0x80, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x80, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x00, DEF_STR( On ) )

	PORT_START      /* D803 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* D804 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )	/* A */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )	/* C */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START     /* D806 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )	/* A */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )	/* C */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START     /* D807 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+0, RGN_FRAC(1,2)+4, 0, 4 },
	{ 3, 2, 1, 0, 8+3, 8+2, 8+1, 8+0 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8
};

static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+0, RGN_FRAC(1,2)+4, 0, 4 },
	{ 3, 2, 1, 0, 8+3, 8+2, 8+1, 8+0,
			16*8+3, 16*8+2, 16*8+1, 16*8+0, 16*8+8+3, 16*8+8+2, 16*8+8+1, 16*8+8+0 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			16*16, 17*16, 18*16, 19*16, 20*16, 21*16, 22*16, 23*16 },
	64*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,     0, 16 },
	{ REGION_GFX1, 0, &spritelayout, 256, 16 },
	{ -1 }	/* end of array */
};


static struct AY8910interface ay8910_interface =
{
	1,	/* 1 chip */
	8000000/4,	/* ??? */
	{ 10 },
	{ 0 },
	{ 0 },
	{ sound_control_2_w },
	{ sound_control_3_w }
};

static struct MSM5232interface msm5232_interface =
{
	1, /* number of chips */
	2000000, /* 2 MHz ?*/
	{ { 1.0e-6, 1.0e-6, 1.0e-6, 1.0e-6, 1.0e-6, 1.0e-6, 1.0e-6, 1.0e-6 } },	/* 1.0 uF capacitors (verified on real PCB) */
	{ 100 }	/* ? */
};

static struct DACinterface dac_interface =
{
	1,
	{ 20 }
};


static MACHINE_DRIVER_START( flstory )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,10733000/2)		/* ??? */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80,8000000/2)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)		/* 4 MHz */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,2)	/* IRQ generated by ??? */
						/* NMI generated by the main CPU */

	MDRV_CPU_ADD(M68705,4000000/2)	/* ??? */
	MDRV_CPU_MEMORY(m68705_readmem,m68705_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)	/* 100 CPU slices per frame - an high value to ensure proper */
							/* synchronization of the CPUs */
	MDRV_MACHINE_INIT(ta7630)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_VIDEO_START(flstory)
	MDRV_VIDEO_UPDATE(flstory)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
	MDRV_SOUND_ADD(MSM5232, msm5232_interface)
	MDRV_SOUND_ADD(DAC, dac_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( onna34ro )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,10733000/2)		/* ??? */
	MDRV_CPU_MEMORY(onna34ro_readmem,onna34ro_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80,8000000/2)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)		/* 4 MHz */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,2)	/* IRQ generated by ??? */
						/* NMI generated by the main CPU */
	MDRV_CPU_ADD(M68705,4000000/2)	 /* ??? */
	MDRV_CPU_MEMORY(m68705_readmem,m68705_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)	/* 100 CPU slices per frame - an high value to ensure proper */
							/* synchronization of the CPUs */
	MDRV_MACHINE_INIT(ta7630)

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_VIDEO_START(flstory)
	MDRV_VIDEO_UPDATE(flstory)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
	MDRV_SOUND_ADD(MSM5232, msm5232_interface)
	MDRV_SOUND_ADD(DAC, dac_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( onna34ra )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,10733000/2)		/* ??? */
	MDRV_CPU_MEMORY(onna34ra_readmem,onna34ra_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80,8000000/2)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)		/* 4 MHz */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,2)	/* IRQ generated by ??? */
						/* NMI generated by the main CPU */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)	/* 100 CPU slices per frame - an high value to ensure proper */
							/* synchronization of the CPUs */
	MDRV_MACHINE_INIT(ta7630)

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_VIDEO_START(flstory)
	MDRV_VIDEO_UPDATE(flstory)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
	MDRV_SOUND_ADD(MSM5232, msm5232_interface)
	MDRV_SOUND_ADD(DAC, dac_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( rumba )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,8000000/2)		/* 4 MHz */
	MDRV_CPU_MEMORY(rumba_readmem,rumba_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80,8000000/2)		/* 4 MHz */
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)		/* 4 MHz */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,2)	/* IRQ generated by ??? */
						/* NMI generated by the main CPU */
	MDRV_CPU_ADD(M68705,4000000/2)	/* ??? */
	MDRV_CPU_MEMORY(m68705_readmem,m68705_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)	/* 100 CPU slices per frame - an high value to ensure proper */
							/* synchronization of the CPUs */
	MDRV_MACHINE_INIT(ta7630)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
    MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_VIDEO_START(rumba)
	MDRV_VIDEO_UPDATE(rumba)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
	MDRV_SOUND_ADD(MSM5232, msm5232_interface)
	MDRV_SOUND_ADD(DAC, dac_interface)
MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( flstory )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for the first CPU */
	ROM_LOAD( "cpu-a45.15",   0x0000, 0x4000, CRC(f03fc969) SHA1(c8dd25ca25fd413b1a29bd4e58ce5820e5f852b2) )
	ROM_LOAD( "cpu-a45.16",   0x4000, 0x4000, CRC(311aa82e) SHA1(c2dd806f70ea917818ec844a275fb2fecc2e6c19) )
	ROM_LOAD( "cpu-a45.17",   0x8000, 0x4000, CRC(a2b5d17d) SHA1(0198d048aedcbd2498d490a5c0c506f8fc66ed03) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the second CPU */
	ROM_LOAD( "snd.22",       0x0000, 0x2000, CRC(d58b201d) SHA1(1c9c2936ec95a8fa920d58668bea420c5e15008f) )
	ROM_LOAD( "snd.23",       0x2000, 0x2000, CRC(25e7fd9d) SHA1(b9237459e3d8acf8502a693914e50714a37d515e) )

	ROM_REGION( 0x0800, REGION_CPU3, 0 )	/* 2k for the microcontroller */
	ROM_LOAD( "a45-20.mcu",   0x0000, 0x0800, CRC(7d2cdd9b) SHA1(b9a7b4c7d9d58b4b7cab1304beaa9d17f9559419) )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "vid-a45.18",   0x00000, 0x4000, CRC(6f08f69e) SHA1(8f1b7e63a38f855cf26d57aed678da7cf1378fdf) )
	ROM_LOAD( "vid-a45.06",   0x04000, 0x4000, CRC(dc856a75) SHA1(6eedbf6b027c884502b6e7329f13829787138165) )
	ROM_LOAD( "vid-a45.08",   0x08000, 0x4000, CRC(d0b028ca) SHA1(c8bd9136ad3180002961ecfe600fc91a3c891539) )
	ROM_LOAD( "vid-a45.20",   0x0c000, 0x4000, CRC(1b0edf34) SHA1(e749c78053ed09bdb42c03cf4589b0fe122d9095) )
	ROM_LOAD( "vid-a45.19",   0x10000, 0x4000, CRC(2b572dc9) SHA1(9e14428663819e18829c625b4ae91a8a5530eb33) )
	ROM_LOAD( "vid-a45.07",   0x14000, 0x4000, CRC(aa4b0762) SHA1(6d4246753e80fe3ca05d47bd279f7ccc603f4700) )
	ROM_LOAD( "vid-a45.09",   0x18000, 0x4000, CRC(8336be58) SHA1(b92d37856870c4128a860d8ae02fa647743b99e3) )
	ROM_LOAD( "vid-a45.21",   0x1c000, 0x4000, CRC(fc382bd1) SHA1(a773c87454a3d7b80374a6d38ecb8633af2cd990) )
ROM_END

ROM_START( flstoryj )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for the first CPU */
	ROM_LOAD( "cpu-a45.15",   0x0000, 0x4000, CRC(f03fc969) SHA1(c8dd25ca25fd413b1a29bd4e58ce5820e5f852b2) )
	ROM_LOAD( "cpu-a45.16",   0x4000, 0x4000, CRC(311aa82e) SHA1(c2dd806f70ea917818ec844a275fb2fecc2e6c19) )
	ROM_LOAD( "cpu-a45.17",   0x8000, 0x4000, CRC(a2b5d17d) SHA1(0198d048aedcbd2498d490a5c0c506f8fc66ed03) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the second CPU */
	ROM_LOAD( "a45_12.8",     0x0000, 0x2000, CRC(d6f593fb) SHA1(8551ef22c2cdd9df8d7949a178883f56ea56a4a2) )
	ROM_LOAD( "a45_13.9",     0x2000, 0x2000, CRC(451f92f9) SHA1(f4196e6d3420983b74001303936d086a48b10827) )

	ROM_REGION( 0x0800, REGION_CPU3, 0 )	/* 2k for the microcontroller */
	ROM_LOAD( "a45-20.mcu",   0x0000, 0x0800, CRC(7d2cdd9b) SHA1(b9a7b4c7d9d58b4b7cab1304beaa9d17f9559419) )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "vid-a45.18",   0x00000, 0x4000, CRC(6f08f69e) SHA1(8f1b7e63a38f855cf26d57aed678da7cf1378fdf) )
	ROM_LOAD( "vid-a45.06",   0x04000, 0x4000, CRC(dc856a75) SHA1(6eedbf6b027c884502b6e7329f13829787138165) )
	ROM_LOAD( "vid-a45.08",   0x08000, 0x4000, CRC(d0b028ca) SHA1(c8bd9136ad3180002961ecfe600fc91a3c891539) )
	ROM_LOAD( "vid-a45.20",   0x0c000, 0x4000, CRC(1b0edf34) SHA1(e749c78053ed09bdb42c03cf4589b0fe122d9095) )
	ROM_LOAD( "vid-a45.19",   0x10000, 0x4000, CRC(2b572dc9) SHA1(9e14428663819e18829c625b4ae91a8a5530eb33) )
	ROM_LOAD( "vid-a45.07",   0x14000, 0x4000, CRC(aa4b0762) SHA1(6d4246753e80fe3ca05d47bd279f7ccc603f4700) )
	ROM_LOAD( "vid-a45.09",   0x18000, 0x4000, CRC(8336be58) SHA1(b92d37856870c4128a860d8ae02fa647743b99e3) )
	ROM_LOAD( "vid-a45.21",   0x1c000, 0x4000, CRC(fc382bd1) SHA1(a773c87454a3d7b80374a6d38ecb8633af2cd990) )
ROM_END

ROM_START( onna34ro )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for the first CPU */
	ROM_LOAD( "a52-01-1.40c", 0x0000, 0x4000, CRC(ffddcb02) SHA1(d7002e8a577a5f9c2f63ec8d93076cd720443e05) )
	ROM_LOAD( "a52-02-1.41c", 0x4000, 0x4000, CRC(da97150d) SHA1(9b18f4d0bff811e332f6d2e151c7583400d60f23) )
	ROM_LOAD( "a52-03-1.42c", 0x8000, 0x4000, CRC(b9749a53) SHA1(15fd9624a500512f7b2c6766ed96f3734f61f160) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the second CPU */
	ROM_LOAD( "a52-12.08s",   0x0000, 0x2000, CRC(28f48096) SHA1(20aa5041cd71003e0981c32e34005bcbad53f707) )
	ROM_LOAD( "a52-13.09s",   0x2000, 0x2000, CRC(4d3b16f3) SHA1(8687b76398da875f69e9565277f00478c2b82a99) )
	ROM_LOAD( "a52-14.10s",   0x4000, 0x2000, CRC(90a6f4e8) SHA1(101767a90e963f3031e0830fd25a537ca8296de9) )
	ROM_LOAD( "a52-15.37s",   0x6000, 0x2000, CRC(5afc21d0) SHA1(317d5fb3a48ce5e13e02c5c6431fa08ada115d27) )
	ROM_LOAD( "a52-16.38s",   0x8000, 0x2000, CRC(ccf42aee) SHA1(a6eb01c5384724999631b55700dade430b71ca95) )

	ROM_REGION( 0x0800, REGION_CPU3, 0 )	/* 2k for the microcontroller */
    ROM_LOAD( "a52_17.54c",   0x0000, 0x0800, CRC(0ab2612e) SHA1(2bc74e9ef5b9dd51d733dc62902d92c269f7d6a7) )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "a52-04.11v",   0x00000, 0x4000, CRC(5b126294) SHA1(fc31e062e665f7313f923e84d6497716f0658ac0) )
	ROM_LOAD( "a52-06.10v",   0x04000, 0x4000, CRC(78114721) SHA1(d0e52544e05ab4fd1b131ed49beb252048bcbe31) )
	ROM_LOAD( "a52-08.09v",   0x08000, 0x4000, CRC(4a293745) SHA1(a54c1cfced63306db0ba7ee635dce41134c91dc8) )
	ROM_LOAD( "a52-10.08v",   0x0c000, 0x4000, CRC(8be7b4db) SHA1(e7ab373942b8ce75b36d0c9f547902fe65a3964d) )
	ROM_LOAD( "a52-05.35v",   0x10000, 0x4000, CRC(a1a99588) SHA1(eae63ae89058da1a92065e1d352cf81a15b556bc) )
	ROM_LOAD( "a52-07.34v",   0x14000, 0x4000, CRC(0bf420f2) SHA1(367e76efbed772fc8a6d7ac854407b62f8897d78) )
	ROM_LOAD( "a52-09.33v",   0x18000, 0x4000, CRC(39c543b5) SHA1(978c42f5eb23c15a96dae3578e742ef41bac689b) )
	ROM_LOAD( "a52-11.32v",   0x1c000, 0x4000, CRC(d1dda6b3) SHA1(fadf1404e8a03ec7e3fafb6281d33bc73bb5c473) )
ROM_END

ROM_START( onna34ra )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for the first CPU */
	ROM_LOAD( "ry-08.rom", 0x0000, 0x4000, CRC(e4587b85) SHA1(2fc4439953dd086eac11ba6d7937d8075fc39639) )
	ROM_LOAD( "ry-07.rom", 0x4000, 0x4000, CRC(6ffda515) SHA1(429e7bb22c66eb3c6d31981c2021af61c44ed51b) )
	ROM_LOAD( "ry-06.rom", 0x8000, 0x4000, CRC(6fefcda8) SHA1(f532e254a8bd7372bd9f8f21c907e44e0f5f4f32) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the second CPU */
	ROM_LOAD( "a52-12.08s",   0x0000, 0x2000, CRC(28f48096) SHA1(20aa5041cd71003e0981c32e34005bcbad53f707) )
	ROM_LOAD( "a52-13.09s",   0x2000, 0x2000, CRC(4d3b16f3) SHA1(8687b76398da875f69e9565277f00478c2b82a99) )
	ROM_LOAD( "a52-14.10s",   0x4000, 0x2000, CRC(90a6f4e8) SHA1(101767a90e963f3031e0830fd25a537ca8296de9) )
	ROM_LOAD( "a52-15.37s",   0x6000, 0x2000, CRC(5afc21d0) SHA1(317d5fb3a48ce5e13e02c5c6431fa08ada115d27) )
	ROM_LOAD( "a52-16.38s",   0x8000, 0x2000, CRC(ccf42aee) SHA1(a6eb01c5384724999631b55700dade430b71ca95) )
	
	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "a52-04.11v",   0x00000, 0x4000, CRC(5b126294) SHA1(fc31e062e665f7313f923e84d6497716f0658ac0) )
	ROM_LOAD( "a52-06.10v",   0x04000, 0x4000, CRC(78114721) SHA1(d0e52544e05ab4fd1b131ed49beb252048bcbe31) )
	ROM_LOAD( "a52-08.09v",   0x08000, 0x4000, CRC(4a293745) SHA1(a54c1cfced63306db0ba7ee635dce41134c91dc8) )
	ROM_LOAD( "a52-10.08v",   0x0c000, 0x4000, CRC(8be7b4db) SHA1(e7ab373942b8ce75b36d0c9f547902fe65a3964d) )
	ROM_LOAD( "a52-05.35v",   0x10000, 0x4000, CRC(a1a99588) SHA1(eae63ae89058da1a92065e1d352cf81a15b556bc) )
	ROM_LOAD( "a52-07.34v",   0x14000, 0x4000, CRC(0bf420f2) SHA1(367e76efbed772fc8a6d7ac854407b62f8897d78) )
	ROM_LOAD( "a52-09.33v",   0x18000, 0x4000, CRC(39c543b5) SHA1(978c42f5eb23c15a96dae3578e742ef41bac689b) )
	ROM_LOAD( "a52-11.32v",   0x1c000, 0x4000, CRC(d1dda6b3) SHA1(fadf1404e8a03ec7e3fafb6281d33bc73bb5c473) )
ROM_END

ROM_START( rumba )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for the first CPU */
	ROM_LOAD( "a23_01-1.bin",   0x0000, 0x4000, CRC(4bea6e18) SHA1(b9a85e65105773b5f93dcc5fc1e7c588b2d25056) )
	ROM_LOAD( "a23_02-1.bin",   0x4000, 0x4000, CRC(08f98c6f) SHA1(f2a850b1138cfefab6ff1d1adcda9e084f52e9c2) )
	ROM_LOAD( "a23_03-1.bin",   0x8000, 0x4000, CRC(ab595427) SHA1(1ff51740e1c7915e1f79a55801d11c8fdce764c8) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the second CPU */
	ROM_LOAD( "a23_08-1.bin",    0x0000, 0x2000, CRC(a18eae00) SHA1(6ac1ad07bb5a97c6edaaf0e1fb842e1741f4cf1e) )
	ROM_LOAD( "a23_09.bin",      0x2000, 0x2000, CRC(d0a101d3) SHA1(c92bb1ce67bec394fd8ce303d9e61eac12493b5d) )
	ROM_LOAD( "a23_10.bin",      0x4000, 0x2000, CRC(f9447bd4) SHA1(68c02249ca0e5b923cddb4bff8d090963b9c78e4) )

	ROM_REGION( 0x0800, REGION_CPU3, 0 )	/* 2k for the microcontroller */
	ROM_LOAD( "a23_11.bin",      0x0000, 0x0800, CRC(fddc99ce) SHA1(a9c7f76752ce74a780ca74004106c969d78ba931) )

	ROM_REGION( 0x8000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "a23_07.bin",   0x02000, 0x2000, CRC(c98fbea6) SHA1(edd1e0b2551f726018ca6e0b2cf629046a482711) )
	ROM_LOAD( "a23_06.bin",   0x00000, 0x2000, CRC(bf1e3a7f) SHA1(1258be10739cee6e6a8b2ce4d39f89bff1ea7f16) ) /* should be a good read */
	ROM_LOAD( "a23_05.bin",   0x06000, 0x2000, CRC(b40db231) SHA1(85204efc05e95334576807e4dab866f4f40081e6) )
	ROM_LOAD( "a23_04.bin",   0x04000, 0x2000, CRC(1d4f001f) SHA1(c3245650e57138ed89e7de8289fe37c5d933ddca) )
ROM_END

GAMEX( 1985, flstory,  0,        flstory,  flstory,  0, ROT180, "Taito", "The FairyLand Story", GAME_IMPERFECT_SOUND )
GAMEX( 1985, flstoryj, flstory,  flstory,  flstory,  0, ROT180, "Taito", "The FairyLand Story (Japan)", GAME_IMPERFECT_SOUND )
GAMEX( 1985, onna34ro, 0,        onna34ro, onna34ro, 0, ROT0,   "Taito", "Onna Sansirou - Typhoon Gal", GAME_IMPERFECT_SOUND )
GAMEX( 1985, onna34ra, onna34ro, onna34ra, onna34ro, 0, ROT0,   "Taito", "Onna Sansirou - Typhoon Gal (bootleg)", GAME_IMPERFECT_SOUND )
GAMEX( 1984, rumba,    0,        rumba,    rumba,    0, ROT270, "Taito", "Rumba Lumber", GAME_IMPERFECT_SOUND )
