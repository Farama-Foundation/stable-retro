/***************************************************************************

Zero Zone memory map

driver by Brad Oliver

CPU 1 : 68000, uses irq 1

0x000000 - 0x01ffff : ROM
0x080000 - 0x08000f : input ports and dipswitches
0x088000 - 0x0881ff : palette RAM, 256 total colors
0x09ce00 - 0x09d9ff : video ram, 48x32
0x0c0000 - 0x0cffff : RAM
0x0f8000 - 0x0f87ff : RAM (unused?)

Stephh's notes :

  IMO, the game only has 2 buttons (1 to rotate the pieces and 1 for help).
  The 3rd button (when the Dip Switch is activated) subs one "line"
  (0x0c0966 for player 1 and 0x0c1082 for player 2) each time it is pressed.
  As I don't see why such thing would REALLY exist, I've added the
  IPF_CHEAT flag for the Dip Switch and the 3rd button of each player.

TODO:
	* adpcm samples don't seem to be playing at the proper tempo - too fast?


***************************************************************************/
#include "driver.h"
#include "vidhrdw/generic.h"

VIDEO_START( zerozone );
VIDEO_UPDATE( zerozone );
WRITE16_HANDLER( zerozone_tilemap_w );
WRITE16_HANDLER( zerozone_tilebank_w );

extern data16_t *zerozone_videoram;

static READ16_HANDLER( zerozone_input_r )
{
	switch (offset)
	{
		case 0x00:
			return readinputport(0); /* IN0 */
		case 0x01:
			return (readinputport(1) | (readinputport(2) << 8)); /* IN1 & IN2 */
		case 0x04:
			return (readinputport(4) << 8);
		case 0x05:
			return readinputport(3);
	}

log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #0 PC %06x: warning - read unmapped memory address %06x\n",activecpu_get_pc(),0x800000+offset);

	return 0x00;
}


WRITE16_HANDLER( zerozone_sound_w )
{
	if (ACCESSING_MSB)
	{
		soundlatch_w(offset,data >> 8);
		cpu_set_irq_line_and_vector(1,0,HOLD_LINE,0xff);
	}
}

static MEMORY_READ16_START( readmem )
	{ 0x000000, 0x01ffff, MRA16_ROM },
	{ 0x080000, 0x08000f, zerozone_input_r },
	{ 0x088000, 0x0881ff, MRA16_RAM },
	{ 0x098000, 0x098001, MRA16_RAM }, /* watchdog? */
	{ 0x09ce00, 0x09ffff, MRA16_RAM },
	{ 0x0c0000, 0x0cffff, MRA16_RAM },
	{ 0x0f8000, 0x0f87ff, MRA16_RAM }, /* never actually used*/
MEMORY_END

static MEMORY_WRITE16_START( writemem )
	{ 0x000000, 0x01ffff, MWA16_ROM },
	{ 0x084000, 0x084001, zerozone_sound_w },
	{ 0x088000, 0x0881ff, paletteram16_RRRRGGGGBBBBRGBx_word_w, &paletteram16 },
	{ 0x09ce00, 0x09ddff, zerozone_tilemap_w, &zerozone_videoram, &videoram_size },
	{ 0x0b4000, 0x0b4001, zerozone_tilebank_w },
	{ 0x0c0000, 0x0cffff, MWA16_RAM }, /* RAM */
	{ 0x0f8000, 0x0f87ff, MWA16_RAM },
MEMORY_END


static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0x9800, 0x9800, OKIM6295_status_0_r },
	{ 0xa000, 0xa000, soundlatch_r },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x9800, 0x9800, OKIM6295_data_0_w },
MEMORY_END

INPUT_PORTS_START( zerozone )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 | IPF_CHEAT )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 | IPF_CHEAT )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START /* DSW A */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, "In Game Default" )		/* 130, 162 or 255 "lines"*/
	PORT_DIPSETTING(    0x00, "Always Hard" )			/* 255 "lines"*/
	PORT_DIPNAME( 0x10, 0x10, "Speed" )
	PORT_DIPSETTING(    0x10, "Normal" )			/* Drop every 20 frames*/
	PORT_DIPSETTING(    0x00, "Fast" )				/* Drop every 18 frames*/
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START /* DSW B */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Helps" )
	PORT_DIPSETTING(    0x04, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPNAME( 0x08, 0x08, "Bonus Help" )
	PORT_DIPSETTING(    0x00, "30000" )
	PORT_DIPSETTING(    0x08, "None" )
	PORT_BITX(    0x10, 0x10, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Use 3rd Button", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x10, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )
INPUT_PORTS_END


static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	RGN_FRAC(1,1),	/* 4096 characters */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 0, 4, 8+0, 8+4, 16+0, 16+4, 24+0, 24+4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8	/* every sprite takes 32 consecutive bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0, 256 },         /* sprites & playfield */
	{ -1 } /* end of array */
};


static struct OKIM6295interface okim6295_interface =
{
	1,              /* 1 chip */
	{ 8000 },           /* 8000Hz ??? TODO: find out the real frequency */
	{ REGION_SOUND1 },	/* memory region 3 */
	{ 100 }
};

static MACHINE_DRIVER_START( zerozone )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000)	/* 10 MHz */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq1_line_hold,1)

	MDRV_CPU_ADD(Z80, 1000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 1 MHz ??? */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(1*8, 47*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_START(zerozone)
	MDRV_VIDEO_UPDATE(zerozone)

	/* sound hardware */
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( zerozone )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )     /* 128k for 68000 code */
	ROM_LOAD16_BYTE( "zz-4.rom", 0x0000, 0x10000, CRC(83718b9b) SHA1(b3fc6da5816142b9c92a7b8615eb5bcb2c78ea46) )
	ROM_LOAD16_BYTE( "zz-5.rom", 0x0001, 0x10000, CRC(18557f41) SHA1(6ef908732b7775c1ea2b33f799635075db5756de) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )      /* sound cpu */
	ROM_LOAD( "zz-1.rom", 0x00000, 0x08000, CRC(223ccce5) SHA1(3aa25ca914960b929dc853d07a958ed874e42fee) )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "zz-6.rom", 0x00000, 0x80000, CRC(c8b906b9) SHA1(1775d69df6397d6772b20c65751d44556d76c033) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )      /* ADPCM samples */
	ROM_LOAD( "zz-2.rom", 0x00000, 0x20000, CRC(c7551e81) SHA1(520de3074fa6a71fef10d5a76cba5580fd1cbbae) )
	ROM_LOAD( "zz-3.rom", 0x20000, 0x20000, CRC(e348ff5e) SHA1(6d2755d9b31366f4c2ddd296790234deb8f821c8) )
ROM_END

ROM_START( lvgirl94 )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )     /* 128k for 68000 code */
	ROM_LOAD16_BYTE( "rom4", 0x0000, 0x10000, CRC(c4fb449e) SHA1(dd1c567ba2cf951267dd622e2e9af265e742f246) )
	ROM_LOAD16_BYTE( "rom5", 0x0001, 0x10000, CRC(5d446a1a) SHA1(2d7ea25e5b86e7cf4eb7f10daa1eaaaed6830a53) )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rom6", 0x00000, 0x40000, CRC(eeeb94ba) SHA1(9da09312c090ef2d40f596247d9a7decf3724e54) )

	/* sound roms are the same as zerozone */
	ROM_REGION( 0x10000, REGION_CPU2, 0 )      /* sound cpu */
	ROM_LOAD( "rom1", 0x00000, 0x08000, CRC(223ccce5) SHA1(3aa25ca914960b929dc853d07a958ed874e42fee) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )      /* ADPCM samples */
	ROM_LOAD( "rom2", 0x00000, 0x20000, CRC(c7551e81) SHA1(520de3074fa6a71fef10d5a76cba5580fd1cbbae) )
	ROM_LOAD( "rom3", 0x20000, 0x20000, CRC(e348ff5e) SHA1(6d2755d9b31366f4c2ddd296790234deb8f821c8) )
ROM_END


GAME( 1993, zerozone, 0, zerozone, zerozone, 0, ROT0, "Comad", "Zero Zone" )
GAME( 1994, lvgirl94, 0, zerozone, zerozone, 0, ROT0, "Comad", "Las Vegas Girl (Girl '94)" )
