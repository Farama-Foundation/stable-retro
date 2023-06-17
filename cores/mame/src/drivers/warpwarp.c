/***************************************************************************

Warp Warp memory map (preliminary)

  Memory Map figured out by Chris Hardy (chrish@kcbbs.gen.nz)
  Initial Driver code by Mirko


0000-37FF ROM		Code
4800-4FFF ROM		Graphics rom which must be included in memory space

memory mapped ports:

read:

All Read ports only use bit 0

C000	  Coin slot 1
C001	  Fire (for player 2 in "cocktail mode")
C002	  Start P1
C003	  Start P2
C004	  Fire
C005	  Test Mode
C006	  Cabinet type
C007	  Coin Slot 2
C010	  Joystick (read like an analog one, but it's digital)
		  0->23 = DOWN
		  24->63 = UP
		  64->111 = LEFT
		  112->167 = RIGHT
		  168->255 = NEUTRAL
C020-C027 Dipswitch 1->8 in bit 0

write:
C000-C001 bullet x/y pos
C002	  Sound
C003	  WatchDog reset
C010	  Music 1
C020	  Music 2
C030-C032 lamps
C034	  coin lock out
C035	  coin counter
C036	  IRQ enable _and_ bullet enable (both on bit 0) (currently ignored)
C037	  flip screen


Stephh's notes :

  - The only difference between 'warpwarr' and 'warpwar2' is the copyright
    string on the first screen (when the scores are displayed) :

      * 'warpwarr' : "(c) 1981 ROCK-OLA MFG.CORP."  (text stored at 0x33ff to 0x3417)
      * 'warpwar2' : "(c) 1981 ROCK-OLA MFG.CO."    (text stored at 0x33ff to 0x3415)

    Note that the checksum at 0x37ff (used for checking ROM at 0x3000 to 0x37ff)
    is different of course.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


extern unsigned char *warpwarp_bulletsram;
PALETTE_INIT( warpwarp );
VIDEO_UPDATE( warpwarp );
WRITE_HANDLER( warpwarp_flip_screen_w );

/* from sndhrdw/warpwarp.c */
WRITE_HANDLER( warpwarp_sound_w );
WRITE_HANDLER( warpwarp_music1_w );
WRITE_HANDLER( warpwarp_music2_w );
extern int warpwarp_sh_start(const struct MachineSound *msound);
extern void warpwarp_sh_stop(void);
extern void warpwarp_sh_update(void);

/* Read System Inputs */
static READ_HANDLER( bombbee_sys_r )
{
	if (offset == 4)	/* to return BUTTON1 status*/
	{
		return (readinputport(4) >> (flip_screen & 1)) & 1;
	}
	else
		return (readinputport(0) >> offset) & 1;
}

static READ_HANDLER( warpwarp_sys_r )
{
	return (readinputport(0) >> offset) & 1;
}

/* Read Dipswitches */
static READ_HANDLER( warpwarp_dsw_r )
{
	return (readinputport(1) >> offset) & 1;
}

/* Read mux Controller Inputs */
static READ_HANDLER( bombbee_mux_r )
{
	return readinputport(2 + (flip_screen & 1));
}

static READ_HANDLER( warpwarp_mux_r )
{
	int res;

	res = readinputport(2 + (flip_screen & 1));
	if (res & 1) return 23;
	if (res & 2) return 63;
	if (res & 4) return 111;
	if (res & 8) return 167;
	return 255;
}

static WRITE_HANDLER( warpwarp_leds_w )
{
	set_led_status(offset,data & 1);
}

static WRITE_HANDLER( warpwarp_coin_counter_w )
{
	coin_counter_w(offset,data);
}



static MEMORY_READ_START( bombbee_readmem )
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x2000, 0x23ff, MRA_RAM },
	{ 0x4000, 0x47ff, MRA_RAM },
	{ 0x4800, 0x4fff, MRA_ROM },
	{ 0x6000, 0x6007, bombbee_sys_r },
	{ 0x6010, 0x6010, bombbee_mux_r },
	{ 0x6020, 0x6027, warpwarp_dsw_r },
MEMORY_END

static MEMORY_WRITE_START( bombbee_writemem )
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x2000, 0x23ff, MWA_RAM },
	{ 0x4000, 0x43ff, videoram_w, &videoram, &videoram_size },
	{ 0x4400, 0x47ff, colorram_w, &colorram },
	{ 0x4800, 0x4fff, MWA_ROM },
	{ 0x6000, 0x6001, MWA_RAM, &warpwarp_bulletsram },
	{ 0x6002, 0x6002, warpwarp_sound_w },
	{ 0x6003, 0x6003, watchdog_reset_w },
	{ 0x6010, 0x6010, warpwarp_music1_w },
	{ 0x6020, 0x6020, warpwarp_music2_w },
	{ 0x6030, 0x6032, warpwarp_leds_w },
	{ 0x6035, 0x6035, warpwarp_coin_counter_w },
	{ 0x6037, 0x6037, warpwarp_flip_screen_w },
MEMORY_END

static MEMORY_READ_START( warpwarp_readmem )
	{ 0x0000, 0x37ff, MRA_ROM },
	{ 0x4000, 0x47ff, MRA_RAM },
	{ 0x4800, 0x4fff, MRA_ROM },
	{ 0x8000, 0x83ff, MRA_RAM },
	{ 0xc000, 0xc007, warpwarp_sys_r },
	{ 0xc010, 0xc010, warpwarp_mux_r },
	{ 0xc020, 0xc027, warpwarp_dsw_r },
MEMORY_END

static MEMORY_WRITE_START( warpwarp_writemem )
	{ 0x0000, 0x37ff, MWA_ROM },
	{ 0x4000, 0x43ff, videoram_w, &videoram, &videoram_size },
	{ 0x4400, 0x47ff, colorram_w, &colorram },
	{ 0x4800, 0x4fff, MWA_ROM },
	{ 0x8000, 0x83ff, MWA_RAM },
	{ 0xc000, 0xc001, MWA_RAM, &warpwarp_bulletsram },
	{ 0xc002, 0xc002, warpwarp_sound_w },
	{ 0xc003, 0xc003, watchdog_reset_w },
	{ 0xc010, 0xc010, warpwarp_music1_w },
	{ 0xc020, 0xc020, warpwarp_music2_w },
	{ 0xc030, 0xc032, warpwarp_leds_w },
	{ 0xc035, 0xc035, warpwarp_coin_counter_w },
	{ 0xc037, 0xc037, warpwarp_flip_screen_w },
MEMORY_END



INPUT_PORTS_START( bombbee )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SPECIAL )	/* mux BUTTON1 - see Fake Input Port*/
	PORT_SERVICE( 0x20, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Cocktail ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )	/* acts as COINn, but doesn't affect coin counter*/

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x00, "3" )
	PORT_DIPSETTING(	0x04, "4" )
/*	PORT_DIPSETTING(	0x08, "4" )				*/ /* duplicated setting*/
	PORT_DIPSETTING(	0x0c, "5" )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0xe0, 0x00, "Replay" )		/* awards 1 credit*/
	PORT_DIPSETTING(	0x00, "50000" )
	PORT_DIPSETTING(	0x20, "60000" )
	PORT_DIPSETTING(	0x40, "70000" )
	PORT_DIPSETTING(	0x60, "80000" )
	PORT_DIPSETTING(	0x80, "100000" )
	PORT_DIPSETTING(	0xa0, "120000" )
	PORT_DIPSETTING(	0xc0, "150000" )
	PORT_DIPSETTING(	0xe0, "None" )

	PORT_START	/* Mux input - player 1 controller - handled by bombbee_mux_r */
	PORT_ANALOG( 0xff, 0x80, IPT_PADDLE | IPF_REVERSE, 30, 10, 0x14, 0xac )

	PORT_START	/* Mux input - player 2 controller - handled by bombbee_mux_r */
	PORT_ANALOG( 0xff, 0x80, IPT_PADDLE | IPF_REVERSE | IPF_COCKTAIL , 30, 10, 0x14, 0xac )

	PORT_START	/* Fake input port to support mux buttons - handled by bombbee_sys_r */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
INPUT_PORTS_END

INPUT_PORTS_START( cutieq )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SPECIAL )	/* mux BUTTON1 - see Fake Input Port*/
	PORT_SERVICE( 0x20, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Cocktail ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )	/* acts as COINn, but doesn't affect coin counter*/

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x00, "3" )
	PORT_DIPSETTING(	0x04, "4" )
/*	PORT_DIPSETTING(	0x08, "4" )				*/ /* duplicated setting*/
	PORT_DIPSETTING(	0x0c, "5" )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0xe0, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x00, "50000" )
	PORT_DIPSETTING(	0x20, "60000" )
	PORT_DIPSETTING(	0x40, "80000" )
	PORT_DIPSETTING(	0x60, "100000" )
	PORT_DIPSETTING(	0x80, "120000" )
	PORT_DIPSETTING(	0xa0, "150000" )
	PORT_DIPSETTING(	0xc0, "200000" )
	PORT_DIPSETTING(	0xe0, "None" )

	PORT_START	/* Mux input - player 1 controller - handled by bombbee_mux_r */
	PORT_ANALOG( 0xff, 0x80, IPT_PADDLE | IPF_REVERSE, 30, 10, 0x14, 0xac )

	PORT_START	/* Mux input - player 2 controller - handled by bombbee_mux_r */
	PORT_ANALOG( 0xff, 0x80, IPT_PADDLE | IPF_REVERSE | IPF_COCKTAIL , 30, 10, 0x14, 0xac )

	PORT_START	/* Fake input port to support mux buttons - handled by bombbee_sys_r */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
INPUT_PORTS_END

INPUT_PORTS_START( warpwarp )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_SERVICE( 0x20, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Cocktail ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x01, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0c, 0x04, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x00, "2" )
	PORT_DIPSETTING(	0x04, "3" )
	PORT_DIPSETTING(	0x08, "4" )
	PORT_DIPSETTING(	0x0c, "5" )
	/* Bonus Lives when "Lives" Dip Switch is set to "2", "3" or "4" */
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x00, "8000 30000" )
	PORT_DIPSETTING(	0x10, "10000 40000" )
	PORT_DIPSETTING(	0x20, "15000 60000" )
	PORT_DIPSETTING(	0x30, "None" )
	/* Bonus Lives when "Lives" Dip Switch is set to "5"
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x00, "30000" )
	PORT_DIPSETTING(	0x10, "40000" )
	PORT_DIPSETTING(	0x20, "60000" )
	PORT_DIPSETTING(	0x30, "None" )
	*/
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	/* when level selection is On, press 1 to increase level */
	PORT_BITX(	  0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Level Selection", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START	/* FAKE - input port to simulate an analog stick - handled by warpwarp_mux_r */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY )

	PORT_START	/* FAKE - input port to simulate an analog stick - handled by warpwarp_mux_r */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
INPUT_PORTS_END

/* has High Score Initials dip switch instead of rack test */
INPUT_PORTS_START( warpwarr )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_SERVICE( 0x20, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Cocktail ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START		/* DSW1 */
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x01, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0c, 0x04, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x00, "2" )
	PORT_DIPSETTING(	0x04, "3" )
	PORT_DIPSETTING(	0x08, "4" )
	PORT_DIPSETTING(	0x0c, "5" )
	/* Bonus Lives when "Lives" Dip Switch is set to "2", "3" or "4" */
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x00, "8000 30000" )
	PORT_DIPSETTING(	0x10, "10000 40000" )
	PORT_DIPSETTING(	0x20, "15000 60000" )
	PORT_DIPSETTING(	0x30, "None" )
	/* Bonus Lives when "Lives" Dip Switch is set to "5"
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x00, "30000" )
	PORT_DIPSETTING(	0x10, "40000" )
	PORT_DIPSETTING(	0x20, "60000" )
	PORT_DIPSETTING(	0x30, "None" )
	*/
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "High Score Initials" )
	PORT_DIPSETTING(	0x80, DEF_STR( No ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Yes ) )

	PORT_START	/* FAKE - input port to simulate an analog stick - handled by warpwarp_mux_r */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY )

	PORT_START	/* FAKE - input port to simulate an analog stick - handled by warpwarp_mux_r */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
INPUT_PORTS_END




static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	1,	/* 1 bit per pixel */
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8 /* every char takes 8 bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	64, /* 64 sprites */
	1,	/* 1 bit per pixel */
	{ 0 },
	{  0, 1, 2, 3, 4, 5, 6, 7 ,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8	/* every sprite takes 32 bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_CPU1, 0x4800, &charlayout,   0, 256 },
	{ REGION_CPU1, 0x4800, &spritelayout, 0, 256 },
	{ -1 } /* end of array */
};

static struct CustomSound_interface custom_interface =
{
	warpwarp_sh_start,
	warpwarp_sh_stop,
	warpwarp_sh_update
};


static MACHINE_DRIVER_START( bombbee )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", 8080, 2048000)	/* 3 MHz? */
	MDRV_CPU_MEMORY(bombbee_readmem,bombbee_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)	/* frames per second, vblank duration */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(34*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 34*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)
	MDRV_COLORTABLE_LENGTH(2*256)

	MDRV_PALETTE_INIT(warpwarp)
	MDRV_VIDEO_START(generic)
	MDRV_VIDEO_UPDATE(warpwarp)

	/* sound hardware */
	MDRV_SOUND_ADD(CUSTOM, custom_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( warpwarp )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(bombbee)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(warpwarp_readmem,warpwarp_writemem)
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( bombbee )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "bombbee.1k",   0x0000, 0x2000, CRC(9f8cd7af) SHA1(0d6e1ee5519660d1498eb7a093872ed5034423f2) )
	ROM_LOAD( "bombbee.4c",   0x4800, 0x0800, CRC(5f37d569) SHA1(d5e3fb4c5a1612a6e568c8970161b0290b88993f) )
ROM_END

ROM_START( cutieq )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "cutieq.1k",    0x0000, 0x2000, CRC(6486cdca) SHA1(914c36487fba2dd57c3fd1f011b2225d2baac2bf) )
	ROM_LOAD( "cutieq.4c",    0x4800, 0x0800, CRC(0e1618c9) SHA1(456e9b3d6bae8b4af7778a38e4f40bb6736b0690) )
ROM_END

ROM_START( warpwarp )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "g-n9601n.2r",  0x0000, 0x1000, CRC(f5262f38) SHA1(1c64d0282b0a209390a548ceeaaf8b7b55e50896) )
	ROM_LOAD( "g-09602n.2m",  0x1000, 0x1000, CRC(de8355dd) SHA1(133d137711d79aaeb45cd3ee041c0be3b73e1b2f) )
	ROM_LOAD( "g-09603n.1p",  0x2000, 0x1000, CRC(bdd1dec5) SHA1(bb3d9d1500e31bb271a394facaec7adc3c987e5e) )
	ROM_LOAD( "g-09613n.1t",  0x3000, 0x0800, CRC(af3d77ef) SHA1(5b79aabbe14c2997e0b1a9276c483ae76814a63a) )
	ROM_LOAD( "g-9611n.4c",   0x4800, 0x0800, CRC(380994c8) SHA1(0cdf6a05db52c423365bff9c9df6d93ac885794e) )
ROM_END

ROM_START( warpwarr )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "g-09601.2r",   0x0000, 0x1000, CRC(916ffa35) SHA1(bca2087f8b78a128cdffc55db9814854b72daab5) )
	ROM_LOAD( "g-09602.2m",   0x1000, 0x1000, CRC(398bb87b) SHA1(74373336288dc13d59e6f7e7c718aa51d857b087) )
	ROM_LOAD( "g-09603.1p",   0x2000, 0x1000, CRC(6b962fc4) SHA1(0291d0c574a1048e52121ca57e01098bff04da40) )
	ROM_LOAD( "g-09613.1t",   0x3000, 0x0800, CRC(60a67e76) SHA1(af65e7bf16a5e69fee05c0134e3b8d5bca142402) )
	ROM_LOAD( "g-9611.4c",    0x4800, 0x0800, CRC(00e6a326) SHA1(67b7ab5b7b2c9a97d4d690d88561da48b86bc66e) )
ROM_END

ROM_START( warpwar2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "g-09601.2r",   0x0000, 0x1000, CRC(916ffa35) SHA1(bca2087f8b78a128cdffc55db9814854b72daab5) )
	ROM_LOAD( "g-09602.2m",   0x1000, 0x1000, CRC(398bb87b) SHA1(74373336288dc13d59e6f7e7c718aa51d857b087) )
	ROM_LOAD( "g-09603.1p",   0x2000, 0x1000, CRC(6b962fc4) SHA1(0291d0c574a1048e52121ca57e01098bff04da40) )
	ROM_LOAD( "g-09612.1t",   0x3000, 0x0800, CRC(b91e9e79) SHA1(378323d83c550b3acabc83dba946ab089b9195cb) )
	ROM_LOAD( "g-9611.4c",    0x4800, 0x0800, CRC(00e6a326) SHA1(67b7ab5b7b2c9a97d4d690d88561da48b86bc66e) )
ROM_END



GAME( 1979, bombbee,  0,        bombbee,  bombbee,  0, ROT90, "Namco", "Bomb Bee" )
GAME( 1979, cutieq,   0,        bombbee,  cutieq,   0, ROT90, "Namco", "Cutie Q" )
GAME( 1981, warpwarp, 0,        warpwarp, warpwarp, 0, ROT90, "Namco", "Warp and Warp" )
GAME( 1981, warpwarr, warpwarp, warpwarp, warpwarr, 0, ROT90, "[Namco] (Rock-ola license)", "Warp Warp (Rock-ola set 1)" )
GAME( 1981, warpwar2, warpwarp, warpwarp, warpwarr, 0, ROT90, "[Namco] (Rock-ola license)", "Warp Warp (Rock-ola set 2)" )
