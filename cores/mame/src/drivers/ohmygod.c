/***************************************************************************

Oh My God!       (c) 1993 Atlus
Naname de Magic! (c) 1994 Atlus

driver by Nicola Salmoria

Notes:
- not sure about the scroll registers
- lots of unknown RAM, maybe other gfx planes not used by this game

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


extern data16_t *ohmygod_videoram;

WRITE16_HANDLER( ohmygod_videoram_w );
WRITE16_HANDLER( ohmygod_spritebank_w );
WRITE16_HANDLER( ohmygod_scrollx_w );
WRITE16_HANDLER( ohmygod_scrolly_w );
VIDEO_START( ohmygod );
VIDEO_UPDATE( ohmygod );


static int adpcm_bank_shift;
static int sndbank;
static int nosound_kludge_step;

static MACHINE_INIT( ohmygod )
{
	unsigned char *rom = memory_region(REGION_SOUND1);

	/* the game requires the watchdog to fire during boot, so we have
	   to initialize it */
	watchdog_reset_r(0);

	sndbank = 0;
	memcpy(rom + 0x20000,rom + 0x40000 + 0x20000 * sndbank,0x20000);

	nosound_kludge_step = 0;
}

WRITE16_HANDLER( ohmygod_ctrl_w )
{
	if (ACCESSING_LSB)
	{
		unsigned char *rom = memory_region(REGION_SOUND1);

		/* ADPCM bank switch */
		if (sndbank != ((data >> adpcm_bank_shift) & 0x0f))
		{
			sndbank = (data >> adpcm_bank_shift) & 0x0f;
			memcpy(rom + 0x20000,rom + 0x40000 + 0x20000 * sndbank,0x20000);
		}
	}
	if (ACCESSING_MSB)
	{
		coin_counter_w(0,data & 0x1000);
		coin_counter_w(1,data & 0x2000);
	}
}

READ16_HANDLER( ohmygod_sound_status_r )
{
	if(Machine->sample_rate == 0)
	{
		/* strobe 'sample playing' flags of the OKIM6295 to make it start up */

		int	data = 0x00F0;

		if(nosound_kludge_step < 4)
		{
			data |= 1 << nosound_kludge_step;
		}

		nosound_kludge_step++;

		if(nosound_kludge_step >= 5)
		{
			nosound_kludge_step = 0;
		}

		return data;
	}

	return OKIM6295_status_0_lsb_r(offset, mem_mask);
}

static MEMORY_READ16_START( readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x300000, 0x303fff, MRA16_RAM },
	{ 0x304000, 0x307fff, MRA16_RAM },
	{ 0x308000, 0x30ffff, MRA16_RAM },
	{ 0x700000, 0x703fff, MRA16_RAM },
	{ 0x704000, 0x707fff, MRA16_RAM },
	{ 0x708000, 0x70ffff, MRA16_RAM },
	{ 0x800000, 0x800001, input_port_0_word_r },
	{ 0x800002, 0x800003, input_port_1_word_r },
	{ 0xa00000, 0xa00001, input_port_2_word_r },
	{ 0xa00002, 0xa00003, input_port_3_word_r },
	{ 0xb00000, 0xb00001, ohmygod_sound_status_r },
	{ 0xc00000, 0xc00001, watchdog_reset16_r },
MEMORY_END

static MEMORY_WRITE16_START( writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x300000, 0x303fff, MWA16_RAM },
	{ 0x304000, 0x307fff, ohmygod_videoram_w, &ohmygod_videoram },
	{ 0x308000, 0x30ffff, MWA16_RAM },
	{ 0x400000, 0x400001, ohmygod_scrollx_w },
	{ 0x400002, 0x400003, ohmygod_scrolly_w },
	{ 0x600000, 0x6007ff, paletteram16_xGGGGGRRRRRBBBBB_word_w, &paletteram16 },
	{ 0x700000, 0x703fff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x704000, 0x707fff, MWA16_RAM },
	{ 0x708000, 0x70ffff, MWA16_RAM },	/* work RAM */
	{ 0x900000, 0x900001, ohmygod_ctrl_w },
	{ 0xb00000, 0xb00001, OKIM6295_data_0_lsb_w },
	{ 0xd00000, 0xd00001, ohmygod_spritebank_w },
MEMORY_END



INPUT_PORTS_START( ohmygod )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BITX(0x0200, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x0f00, 0x0f00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0700, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0500, "6 Coins/3 Credits" )
	PORT_DIPSETTING(      0x0900, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(      0x0f00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0300, "5 Coins/6 Credits" )
	PORT_DIPSETTING(      0x0200, DEF_STR( 4C_5C ) )
/*	PORT_DIPSETTING(      0x0600, DEF_STR( 2C_3C ) )*/
	PORT_DIPSETTING(      0x0100, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0e00, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0d00, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0c00, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0b00, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0a00, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xf000, 0xf000, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x7000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x5000, "6 Coins/3 Credits" )
	PORT_DIPSETTING(      0x9000, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(      0xf000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x3000, "5 Coins/6 Credits" )
	PORT_DIPSETTING(      0x2000, DEF_STR( 4C_5C ) )
/*	PORT_DIPSETTING(      0x6000, DEF_STR( 2C_3C ) )*/
	PORT_DIPSETTING(      0x1000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0xe000, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0xd000, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0xc000, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0xb000, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0xa000, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )

	PORT_START
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x0300, 0x0300, "1P Difficulty" )
	PORT_DIPSETTING(      0x0200, "Easy" )
	PORT_DIPSETTING(      0x0300, "Normal" )
	PORT_DIPSETTING(      0x0100, "Hard" )
	PORT_DIPSETTING(      0x0000, "Very Hard" )
	PORT_DIPNAME( 0x0c00, 0x0c00, "VS Difficulty" )
	PORT_DIPSETTING(      0x0c00, "Normal Jake" )
	PORT_DIPSETTING(      0x0800, "Hard Jake" )
	PORT_DIPSETTING(      0x0400, "Normal" )
	PORT_DIPSETTING(      0x0000, "Hard" )
	PORT_DIPNAME( 0x1000, 0x1000, "Vs Matches/Credit" )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPSETTING(      0x1000, "3" )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, "Balls Have Eyes" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x8000, 0x8000, "Test Mode" )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( naname )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BITX(0x0200, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x0f00, 0x0f00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0700, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0500, "6 Coins/3 Credits" )
	PORT_DIPSETTING(      0x0900, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(      0x0f00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0300, "5 Coins/6 Credits" )
	PORT_DIPSETTING(      0x0200, DEF_STR( 4C_5C ) )
/*	PORT_DIPSETTING(      0x0600, DEF_STR( 2C_3C ) )*/
	PORT_DIPSETTING(      0x0100, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0e00, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0d00, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0c00, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0b00, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0a00, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xf000, 0xf000, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x7000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x5000, "6 Coins/3 Credits" )
	PORT_DIPSETTING(      0x9000, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(      0xf000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x3000, "5 Coins/6 Credits" )
	PORT_DIPSETTING(      0x2000, DEF_STR( 4C_5C ) )
/*	PORT_DIPSETTING(      0x6000, DEF_STR( 2C_3C ) )*/
	PORT_DIPSETTING(      0x1000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0xe000, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0xd000, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0xc000, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0xb000, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0xa000, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )

	PORT_START
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0200, "Easy" )
	PORT_DIPSETTING(      0x0300, "Normal" )
	PORT_DIPSETTING(      0x0100, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x0c00, 0x0c00, "Time Difficulty" )
	PORT_DIPSETTING(      0x0800, "Easy" )
	PORT_DIPSETTING(      0x0c00, "Normal" )
	PORT_DIPSETTING(      0x0400, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x1000, 0x1000, "Vs Matches/Credit" )
	PORT_DIPSETTING(      0x1000, "1" )
	PORT_DIPSETTING(      0x0000, "3" )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, "Freeze" )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
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

static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4,
			8*4, 9*4, 10*4, 11*4, 12*4, 13*4, 14*4, 15*4, },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
			8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	128*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,     0, 16 },	/* colors   0-255 */
	{ REGION_GFX2, 0, &spritelayout, 512, 16 },	/* colors 512-767 */
	{ -1 } /* end of array */
};



static struct OKIM6295interface okim6295_interface =
{
	1,          /* 1 chip */
	{ 14000000/8/132 },	/* 13.257 kHz ??? COMPLETE GUESS!! (not even sure about the xtal) */
	{ REGION_SOUND1 },	/* memory region */
	{ 100 }
};



static MACHINE_DRIVER_START( ohmygod )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq1_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(ohmygod)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(12*8, (64-12)*8-1, 0*8, 30*8-1 )
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(ohmygod)
	MDRV_VIDEO_UPDATE(ohmygod)

	/* sound hardware */
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( ohmygod )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "omg-p.114", 0x00000, 0x80000, CRC(48fa40ca) SHA1(b1d91e1a4a888526febbe53a12b73e375f604f2b) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "omg-b.117",    0x00000, 0x80000, CRC(73621fa6) SHA1(de28c123eeaab78af83ab673431f90c97569450b) )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "omg-s.120",    0x00000, 0x80000, CRC(6413bd36) SHA1(52c455d727496eae80bfab9460127c4c5a874e32) )

	ROM_REGION( 0x240000, REGION_SOUND1, 0 )
	ROM_LOAD( "omg-g.107",    0x00000, 0x200000, CRC(7405573c) SHA1(f4e7318c0a58f43d3c6370490637aea53b28547e) )
	/* 00000-1ffff is fixed, 20000-3ffff is banked */
	ROM_RELOAD(               0x40000, 0x200000 )
ROM_END

ROM_START( naname )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "036-prg.114", 0x00000, 0x80000, CRC(3b7362f7) SHA1(ba16ec9df8569bacd387561ef2b3ea5b17cb650c) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "036-bg.117",    0x00000, 0x80000, CRC(f53e8da5) SHA1(efaec4bb90cad75380ac6eb6859379cdefd187ac) )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "036-spr.120",   0x00000, 0x80000, CRC(e36d8731) SHA1(652709d7884d40459c95761c8abcb394c4b712bf) )

	ROM_REGION( 0x240000, REGION_SOUND1, 0 )
	ROM_LOAD( "036-snd.107",  0x00000, 0x200000, CRC(a3e0caf4) SHA1(35b0eb4ae5b9df1b7c99ec2476a6d834ea50d2e3) )
	/* 00000-1ffff is fixed, 20000-3ffff is banked */
	ROM_RELOAD(               0x40000, 0x200000 )
ROM_END



static DRIVER_INIT( ohmygod )
{
	adpcm_bank_shift = 4;
}

static DRIVER_INIT( naname )
{
	adpcm_bank_shift = 0;
}


GAMEX( 1993, ohmygod, 0, ohmygod, ohmygod, ohmygod, ROT0, "Atlus", "Oh My God! (Japan)", GAME_NO_COCKTAIL )
GAMEX( 1994, naname,  0, ohmygod, naname,  naname,  ROT0, "Atlus", "Naname de Magic! (Japan)", GAME_NO_COCKTAIL )
