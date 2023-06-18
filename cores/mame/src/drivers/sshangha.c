/***************************************************************************

  Super Shanghai Dragon's Eye             (c) 1992 Hot-B

  PCB is manufactured by either Hot-B or Taito, but uses Data East custom
  chips.

  HB-PCB-A4
  M6100691A (distributed by Taito)

  CPU  : 68000
  Sound: Z80B YM2203 Y3014 M6295
  OSC  : 28.0000MHz 16.0000MHz

  The original uses a protection chip which isn't fully worked out yet.
  Sound doesn't work.  The bootleg never seems to write to a sound latch
  (Not sure the bootleg even has the same sound hardware as the original).

  Emulation by Bryan McPhail, mish@tendril.co.uk


Stephh's notes (based on the games M68000 code and some tests) :

0) all games

  - There is no confirmation yet that the "Demo Sounds" Dip Switch does
    something as I don't see where bit 0 of 0xfec04a is tested 8(

  - The First "Unused" Dip Switch is probably used in other (older ?) versions
    to act as a "Debug Mode" Dip Switch. When it's ON, you have these features :

      * there is an extended "test mode" that also allows you to test the
        BG and Object ROMS via a menu.
      * You can end a level by pressing BUTTON3 from player 2 8)

  - The "Adult Mode" Dip Switch determines if "Shanghai Paradise" is available.
  - The "Quest Mode" Dip Switch determines if "Shanghai Quest" is available.
  - The "Use Mahjong Tiles" Dip Switch only has an effect when playing
    "Shanghai Advanced".

1) 'sshangha'

  - There are writes to 0x100000-0x10000f (code from 0x000964 to 0x000a8c),
    but their effect is unknown.

2) 'sshanghb'

  - There are writes to 0x101000-0x10100f (code from 0x000964 to 0x000a8c),
    but their effect is unknown. Note that the code is the SAME as the one
    in 'sshangha' (only the 0x10?00? addresses are different).

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

#define SSHANGHA_HACK	0

VIDEO_START( sshangha );
VIDEO_UPDATE( sshangha );

WRITE16_HANDLER( sshangha_pf2_data_w );
WRITE16_HANDLER( sshangha_pf1_data_w );
WRITE16_HANDLER( sshangha_control_0_w );
WRITE16_HANDLER( sshangha_palette_24bit_w );
WRITE16_HANDLER (sshangha_video_w);

extern data16_t *sshangha_pf1_data;
extern data16_t *sshangha_pf2_data;
extern data16_t *sshangha_pf1_rowscroll, *sshangha_pf2_rowscroll;

static data16_t *sshangha_prot_data;

/******************************************************************************/

static WRITE16_HANDLER( sshangha_protection16_w )
{
	COMBINE_DATA(&sshangha_prot_data[offset]);

	log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #0 PC %06x: warning - write unmapped control address %06x %04x\n",activecpu_get_pc(),offset<<1,data);

	if (offset == (0x260 >> 1)) {
		/*soundlatch_w(0,data&0xff);*/
		/*cpu_set_irq_line(1, IRQ_LINE_NMI, PULSE_LINE);*/
	}
}

static WRITE16_HANDLER( sshangha_sound_w )
{
	soundlatch_w(0,data&0xff);
	cpu_set_irq_line(1, IRQ_LINE_NMI, PULSE_LINE);
}

/* Protection/IO chip 146 */
static READ16_HANDLER( sshangha_protection16_r )
{
	switch (offset)
	{
		case 0x050 >> 1: /* Player 1 & Player 2 joysticks & fire buttons */
			return (readinputport(0) + (readinputport(1) << 8));
		case 0x76a >> 1: /* Credits */
			return readinputport(2);
		case 0x0ac >> 1: /* DIPS */
			return (readinputport(3) + (readinputport(4) << 8));

		/* Protection TODO*/
	}

	log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #0 PC %06x: warning - read unmapped control address %06x\n",activecpu_get_pc(),offset<<1);
	return sshangha_prot_data[offset];
}

static READ16_HANDLER( sshanghb_protection16_r )
{
	switch (offset)
	{
		case 0x050 >> 1: /* Player 1 & Player 2 joysticks & fire buttons */
			return (readinputport(0) + (readinputport(1) << 8));
		case 0x76a >> 1: /* Credits */
			return readinputport(2);
		case 0x0ac >> 1: /* DIPS */
			return (readinputport(3) + (readinputport(4) << 8));
	}
	return sshangha_prot_data[offset];
}

/* Probably returns 0xffff when sprite DMA is complete, the game waits on it */
static READ16_HANDLER( deco_71_r )
{
	return 0xffff;
}

/******************************************************************************/

static MACHINE_INIT( sshangha )
{
	/* Such thing is needed as there is no code to turn the screen
	   to normal orientation when the game is reset.
	   I'm using the value that forces the screen to be in normal
         orientation when entering the "test mode"
         (check the game code from 0x0006b8 to 0x0006f0).
	   I can't tell however if this is accurate or not. */
	sshangha_control_0_w(0, 0x10, 0xff00);
}

/******************************************************************************/

static MEMORY_READ16_START( sshangha_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },

	{ 0x084000, 0x0847ff, sshanghb_protection16_r },
	{ 0x101000, 0x101001, deco_71_r },/*bootleg hack*/

	{ 0x200000, 0x207fff, MRA16_RAM },
	{ 0x340000, 0x340fff, MRA16_RAM },
	{ 0x350000, 0x350001, deco_71_r },
	{ 0x360000, 0x360fff, MRA16_RAM },
	{ 0x370000, 0x370001, deco_71_r },
	{ 0x380000, 0x383fff, MRA16_RAM },
	{ 0xfec000, 0xff3fff, MRA16_RAM },
	{ 0xff4000, 0xff47ff, sshangha_protection16_r },
MEMORY_END

static MEMORY_WRITE16_START( sshangha_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x10000c, 0x10000d, sshangha_sound_w },
	{ 0x10100c, 0x10100d, sshangha_sound_w },	/* the bootleg writes here */
	{ 0x200000, 0x201fff, sshangha_pf1_data_w, &sshangha_pf1_data },
	{ 0x202000, 0x203fff, sshangha_pf2_data_w, &sshangha_pf2_data },
	{ 0x204000, 0x2047ff, MWA16_RAM, &sshangha_pf1_rowscroll },
	{ 0x206000, 0x2067ff, MWA16_RAM, &sshangha_pf2_rowscroll },
	{ 0x300000, 0x30000f, sshangha_control_0_w },
	{ 0x320000, 0x320001, sshangha_video_w },
	{ 0x320002, 0x320005, MWA16_NOP },
	{ 0x340000, 0x340fff, MWA16_RAM, &spriteram16 },
	{ 0x350000, 0x350007, MWA16_NOP },
	{ 0x360000, 0x360fff, MWA16_RAM, &spriteram16_2 },
	{ 0x370000, 0x370007, MWA16_NOP },
	{ 0x380000, 0x383fff, sshangha_palette_24bit_w, &paletteram16 },
	{ 0x3c0000, 0x3c0fff, MWA16_RAM },	/* Sprite ram buffer on bootleg only?? */
	{ 0xfec000, 0xff3fff, MWA16_RAM },
	{ 0xff4000, 0xff47ff, sshangha_protection16_w, &sshangha_prot_data },
MEMORY_END

/******************************************************************************/

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xc000, 0xc000, YM2203_status_port_0_r },
	{ 0xf800, 0xffff, MRA_RAM },
/*	{ 0xf800, 0xf800, soundlatch_r },*/
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xc000, YM2203_control_port_0_w },
	{ 0xc001, 0xc001, YM2203_write_port_0_w },
	{ 0xf800, 0xffff, MWA_RAM },
MEMORY_END

/******************************************************************************/

INPUT_PORTS_START( sshangha )
	PORT_START	/* Player 1 controls (0xfec047.b) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )	/* "Pick Tile"*/
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )	/* "Cancel"*/
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )	/* "Help"*/
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* Player 2 controls (0xfec046.b) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )	/* "Pick Tile"*/
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )	/* "Cancel"*/
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )	/* "Help"*/
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* Credits */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	/* Dips seem inverted with respect to other Deco games */

	PORT_START	/* Dip switch bank 1 (0xfec04a.b, inverted bits order) */
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Demo_Sounds ) )	/* To be confirmed*/
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x20, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x10, 0x10, "Coin Mode" )			/* Check code at 0x0010f2*/
	PORT_DIPSETTING(    0x10, "Mode 1" )
	PORT_DIPSETTING(    0x00, "Mode 2" )
	/* Settings for "Mode 1" */
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	/* Settings for "Mode 2"
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_4C ) )
	*/

	PORT_START	/* Dip switch bank 2 (0xfec04b.b, inverted bits order) */
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x40, "Easy" )
	PORT_DIPSETTING(    0xc0, "Normal" )
	PORT_DIPSETTING(    0x80, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
#if SSHANGHA_HACK
	PORT_DIPNAME( 0x20, 0x20, "Debug Mode" )
#else
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )		/* See notes*/
#endif
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Tile Animation" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x04, 0x04, "Use Mahjong Tiles" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x02, 0x02, "Adult Mode" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Yes ) )
  	PORT_DIPNAME( 0x01, 0x01, "Quest Mode" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Yes ) )
INPUT_PORTS_END

/******************************************************************************/

static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 chars */
	4096,
	4,		/* 4 bits per pixel  */
	{ 8, 0, 0x100000*8+8,0x100000*8+0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8
};

static struct GfxLayout tilelayout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ 8, 0, 0x100000*8+8, 0x100000*8+0 },
	{ 32*8+0, 32*8+1, 32*8+2, 32*8+3, 32*8+4, 32*8+5, 32*8+6, 32*8+7,
		0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,  256, 64 }, /* Characters 8x8 */
	{ REGION_GFX1, 0, &tilelayout,  256, 64 }, /* Tiles 16x16 */
	{ REGION_GFX2, 0, &tilelayout,    0, 32 }, /* Sprites 16x16 */
	{ -1 } /* end of array */
};

/******************************************************************************/

static struct OKIM6295interface okim6295_interface =
{
	1,          /* 1 chip */
	{ 7757 },	/* Frequency */
	{ REGION_SOUND1 },      /* memory region */
	{ 50 }
};

static void irqhandler(int state)
{
	cpu_set_irq_line(1,0,state);
}

static struct YM2203interface ym2203_interface =
{
	1,
	16000000/4,
	{ YM2203_VOL(60,60) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler }
};

static MACHINE_DRIVER_START( sshangha )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 28000000/2)
	MDRV_CPU_MEMORY(sshangha_readmem,sshangha_writemem)
	MDRV_CPU_VBLANK_INT(irq6_line_hold,1)

	MDRV_CPU_ADD(Z80, 16000000/4)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(529)
	MDRV_MACHINE_INIT(sshangha)	/* init machine */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN /*| VIDEO_BUFFERS_SPRITERAM*/)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4096)

	MDRV_VIDEO_START(sshangha)
	MDRV_VIDEO_UPDATE(sshangha)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2203, ym2203_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END

/******************************************************************************/

ROM_START( sshangha )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "ss007-1.u28", 0x00000, 0x20000, CRC(bc466edf) SHA1(b96525b2c879d15b46a7753fa6ebf12a851cd019) )
	ROM_LOAD16_BYTE( "ss006-1.u27", 0x00001, 0x20000, CRC(872a2a2d) SHA1(42d7a01465d5c403354aaf0f2dab8adb9afe61b0) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* Sound CPU */
	ROM_LOAD( "ss008.u82", 0x000000, 0x010000, CRC(04dc3647) SHA1(c06a7e8932c03de5759a9b69da0d761006b49517) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ss001.u8",  0x000000, 0x100000, CRC(ebeca5b7) SHA1(1746e757ad9bbef2aa9028c54f25d4aa4dedf79e) )
	ROM_LOAD( "ss002.u7",  0x100000, 0x100000, CRC(67659f29) SHA1(50944877665b7b848b3f7063892bd39a96a847cf) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ss003.u39", 0x000000, 0x100000, CRC(fbecde72) SHA1(2fe32b28e77ec390c534d276261eefac3fbe21fd) ) /* Copy of rom at u47 */
	ROM_LOAD( "ss004.u37", 0x100000, 0x100000, CRC(98b82c5e) SHA1(af1b52d4b36b1776c148478b5a5581e6a57256b8) ) /* Copy of rom at u46 */

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "ss005.u86", 0x000000, 0x040000, CRC(c53a82ad) SHA1(756e453c8b5ce8e47f93fbda3a9e48bb73e93e2e) )
ROM_END

ROM_START( sshanghb )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "sshanb_2.010", 0x00000, 0x20000, CRC(bc7ed254) SHA1(aeee4b8a8265902bb41575cc143738ecf3aff57d) )
	ROM_LOAD16_BYTE( "sshanb_1.010", 0x00001, 0x20000, CRC(7b049f49) SHA1(2570077c67dbd35053d475a18c3f10813bf914f7) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* Sound CPU */
	ROM_LOAD( "ss008.u82", 0x000000, 0x010000, CRC(04dc3647) SHA1(c06a7e8932c03de5759a9b69da0d761006b49517) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ss001.u8",  0x000000, 0x100000, CRC(ebeca5b7) SHA1(1746e757ad9bbef2aa9028c54f25d4aa4dedf79e) )
	ROM_LOAD( "ss002.u7",  0x100000, 0x100000, CRC(67659f29) SHA1(50944877665b7b848b3f7063892bd39a96a847cf) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ss003.u39", 0x000000, 0x100000, CRC(fbecde72) SHA1(2fe32b28e77ec390c534d276261eefac3fbe21fd) ) /* Copy of rom at u47 */
	ROM_LOAD( "ss004.u37", 0x100000, 0x100000, CRC(98b82c5e) SHA1(af1b52d4b36b1776c148478b5a5581e6a57256b8) ) /* Copy of rom at u46 */

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "ss005.u86", 0x000000, 0x040000, CRC(c53a82ad) SHA1(756e453c8b5ce8e47f93fbda3a9e48bb73e93e2e) )
ROM_END


static DRIVER_INIT( sshangha )
{
#if SSHANGHA_HACK
	/* This is a hack to allow you to use the extra features
         of the first "Unused" Dip Switch (see notes above). */
	data16_t *RAM = (data16_t *)memory_region(REGION_CPU1);
	RAM[0x000384/2] = 0x4e71;
	RAM[0x000386/2] = 0x4e71;
	RAM[0x000388/2] = 0x4e71;
	RAM[0x00038a/2] = 0x4e71;
	/* To avoid checksum error (only useful for 'sshangha') */
	RAM[0x000428/2] = 0x4e71;
	RAM[0x00042a/2] = 0x4e71;
#endif
}


GAMEX( 1992, sshangha, 0,        sshangha, sshangha, sshangha, ROT0, "Hot-B.",   "Super Shanghai Dragon's Eye (Japan)", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND )
GAMEX( 1992, sshanghb, sshangha, sshangha, sshangha, sshangha, ROT0, "bootleg", "Super Shanghai Dragon's Eye (World, bootleg)", GAME_NO_SOUND )
