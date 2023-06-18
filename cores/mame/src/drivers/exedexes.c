/***************************************************************************

Exed Exes

Notes:
- Flip screen is not supported, but doesn't seem to be used (no flip screen
  dip switch and no cocktail mode)
- Some writes to unknown memory locations (always 0?)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


extern UINT8 *exedexes_bg_scroll;
extern UINT8 *exedexes_nbg_yscroll;
extern UINT8 *exedexes_nbg_xscroll;

extern WRITE_HANDLER( exedexes_videoram_w );
extern WRITE_HANDLER( exedexes_colorram_w );
extern WRITE_HANDLER( exedexes_c804_w );
extern WRITE_HANDLER( exedexes_gfxctrl_w );

extern PALETTE_INIT( exedexes );
extern VIDEO_START( exedexes );
extern VIDEO_UPDATE( exedexes );
extern VIDEO_EOF( exedexes );



static INTERRUPT_GEN( exedexes_interrupt )
{
	if (cpu_getiloops() != 0)
		cpu_set_irq_line_and_vector(0, 0, HOLD_LINE, 0xcf);	/* RST 08h */
	else
		cpu_set_irq_line_and_vector(0, 0, HOLD_LINE, 0xd7);	/* RST 10h - vblank */
}



static MEMORY_READ_START( readmem )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xc000, input_port_0_r },
	{ 0xc001, 0xc001, input_port_1_r },
	{ 0xc002, 0xc002, input_port_2_r },
	{ 0xc003, 0xc003, input_port_3_r },
	{ 0xc004, 0xc004, input_port_4_r },
	{ 0xd000, 0xd7ff, MRA_RAM },
	{ 0xe000, 0xefff, MRA_RAM }, /* Work RAM */
	{ 0xf000, 0xffff, MRA_RAM }, /* Sprite RAM */
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc800, 0xc800, soundlatch_w },
	{ 0xc804, 0xc804, exedexes_c804_w },	/* coin counters + text layer enable */
	{ 0xc806, 0xc806, MWA_NOP }, /* Watchdog ?? */
	{ 0xd000, 0xd3ff, exedexes_videoram_w, &videoram },
	{ 0xd400, 0xd7ff, exedexes_colorram_w, &colorram },
	{ 0xd800, 0xd801, MWA_RAM, &exedexes_nbg_yscroll },
	{ 0xd802, 0xd803, MWA_RAM, &exedexes_nbg_xscroll },
	{ 0xd804, 0xd805, MWA_RAM, &exedexes_bg_scroll },
	{ 0xd807, 0xd807, exedexes_gfxctrl_w },	/* layer enables */
	{ 0xe000, 0xefff, MWA_RAM },
	{ 0xf000, 0xffff, MWA_RAM, &spriteram, &spriteram_size },
MEMORY_END



static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x47ff, MRA_RAM },
	{ 0x6000, 0x6000, soundlatch_r },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x47ff, MWA_RAM },
	{ 0x8000, 0x8000, AY8910_control_port_0_w },
	{ 0x8001, 0x8001, AY8910_write_port_0_w },
	{ 0x8002, 0x8002, SN76496_0_w },
	{ 0x8003, 0x8003, SN76496_1_w },
MEMORY_END



INPUT_PORTS_START( exedexes )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT_IMPULSE( 0x20, IP_ACTIVE_LOW, IPT_SERVICE1, 8 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x10, 0x10, "2 Players Game" )
	PORT_DIPSETTING(    0x00, "1 Credit" )
	PORT_DIPSETTING(    0x10, "2 Credits" )
	PORT_DIPNAME( 0x20, 0x00, "Language" )
	PORT_DIPSETTING(    0x00, "English")
	PORT_DIPSETTING(    0x20, "Japanese")
	PORT_DIPNAME( 0x40, 0x40, "Freeze" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ))
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ))
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END




static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters */
	2,	/* 2 bits per pixel */
	{ 4, 0 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8	/* every char takes 16 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
        256,    /* 256 sprites */
        4,      /* 4 bits per pixel */
        { 0x4000*8+4, 0x4000*8+0, 4, 0 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3,
			32*8+0, 32*8+1, 32*8+2, 32*8+3, 33*8+0, 33*8+1, 33*8+2, 33*8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8	/* every sprite takes 64 consecutive bytes */
};
static struct GfxLayout tilelayout =
{
	32,32,  /* 32*32 tiles */
	64,    /* 64 tiles */
	2,      /* 2 bits per pixel */
	{ 4, 0 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3,
			64*8+0, 64*8+1, 64*8+2, 64*8+3, 65*8+0, 65*8+1, 65*8+2, 65*8+3,
			128*8+0, 128*8+1, 128*8+2, 128*8+3, 129*8+0, 129*8+1, 129*8+2, 129*8+3,
			192*8+0, 192*8+1, 192*8+2, 192*8+3, 193*8+0, 193*8+1, 193*8+2, 193*8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16,
			16*16, 17*16, 18*16, 19*16, 20*16, 21*16, 22*16, 23*16,
			24*16, 25*16, 26*16, 27*16, 28*16, 29*16, 30*16, 31*16 },
	256*8	/* every tile takes 256 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,              0, 64 },
	{ REGION_GFX2, 0, &tilelayout,           64*4, 64 }, /* 32x32 Tiles */
	{ REGION_GFX3, 0, &spritelayout,       2*64*4, 16 }, /* 16x16 Tiles */
	{ REGION_GFX4, 0, &spritelayout, 2*64*4+16*16, 16 }, /* Sprites */
	{ -1 } /* end of array */
};



static struct AY8910interface ay8910_interface =
{
	2,	/* 2 chips */
	1500000,	/* 1.5 MHz ? */
	{ 10, 10 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static struct SN76496interface sn76496_interface =
{
	2,	/* 2 chips */
	{ 3000000, 3000000 },	/* 3 MHz????? */
	{ 36, 36 }
};



static MACHINE_DRIVER_START( exedexes )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz (?) */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(exedexes_interrupt,2)

	MDRV_CPU_ADD(Z80, 3000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 3 MHz ??? */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,4)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_BUFFERS_SPRITERAM)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)
	MDRV_COLORTABLE_LENGTH(64*4+64*4+16*16+16*16)

	MDRV_PALETTE_INIT(exedexes)
	MDRV_VIDEO_START(exedexes)
	MDRV_VIDEO_EOF(exedexes)
	MDRV_VIDEO_UPDATE(exedexes)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
	MDRV_SOUND_ADD(SN76496, sn76496_interface)
MACHINE_DRIVER_END



ROM_START( exedexes )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "11m_ee04.bin", 0x0000, 0x4000, CRC(44140dbd) SHA1(7b56f614f7cd7655ffa3e1f4adba5a20fa25822d) )
	ROM_LOAD( "10m_ee03.bin", 0x4000, 0x4000, CRC(bf72cfba) SHA1(9f0b9472890db95e16a71f26da954780d5ec7c16) )
	ROM_LOAD( "09m_ee02.bin", 0x8000, 0x4000, CRC(7ad95e2f) SHA1(53fd8d6985d08106bab45e83827a509486d640b7) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "11e_ee01.bin", 0x00000, 0x4000, CRC(73cdf3b2) SHA1(c9f2c91011bdeecec8fa76a42d95f3a5ec77cec9) )

	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "05c_ee00.bin", 0x00000, 0x2000, CRC(cadb75bd) SHA1(2086be5e295e5d870bcb35f116cc925f811b7583) ) /* Characters */

	ROM_REGION( 0x04000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "h01_ee08.bin", 0x00000, 0x4000, CRC(96a65c1d) SHA1(3b49c64b32f01ec72cf2d943bfe3aa575d62a765) ) /* 32x32 tiles planes 0-1 */

	ROM_REGION( 0x08000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "a03_ee06.bin", 0x00000, 0x4000, CRC(6039bdd1) SHA1(01156e02ed59e6c1e55204729e515cd4419568fb) ) /* 16x16 tiles planes 0-1 */
	ROM_LOAD( "a02_ee05.bin", 0x04000, 0x4000, CRC(b32d8252) SHA1(738225146ba38f2a9216fda278838e7ebb29a0bb) ) /* 16x16 tiles planes 2-3 */

	ROM_REGION( 0x08000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "j11_ee10.bin", 0x00000, 0x4000, CRC(bc83e265) SHA1(ac9b4cce9e539c560414abf2fc239910f2bfbb2d) ) /* Sprites planes 0-1 */
	ROM_LOAD( "j12_ee11.bin", 0x04000, 0x4000, CRC(0e0f300d) SHA1(2f973748e459b16673115abf7de8615219e39fa4) ) /* Sprites planes 2-3 */

	ROM_REGION( 0x6000, REGION_GFX5, 0 )	/* background tilemaps */
	ROM_LOAD( "c01_ee07.bin", 0x0000, 0x4000, CRC(3625a68d) SHA1(83010ca356385b713bafe03a502c566f6a9a8365) )	/* Front Tile Map */
	ROM_LOAD( "h04_ee09.bin", 0x4000, 0x2000, CRC(6057c907) SHA1(886790641b84b8cd659d2eb5fd1adbabdd7dad3d) )	/* Back Tile map */

	ROM_REGION( 0x0b20, REGION_PROMS, 0 )
	ROM_LOAD( "02d_e-02.bin", 0x0000, 0x0100, CRC(8d0d5935) SHA1(a0ab827ff3b641965ef851893c399e3988fde55e) )	/* red component */
	ROM_LOAD( "03d_e-03.bin", 0x0100, 0x0100, CRC(d3c17efc) SHA1(af88340287bd732c91bc5c75970f9de0431b4304) )	/* green component */
	ROM_LOAD( "04d_e-04.bin", 0x0200, 0x0100, CRC(58ba964c) SHA1(1f98f8e484a0462f1a9fadef9e57612a32652599) )	/* blue component */
	ROM_LOAD( "06f_e-05.bin", 0x0300, 0x0100, CRC(35a03579) SHA1(1f1b8c777622a1f5564409c5f3ce69cc68199dae) )	/* char lookup table */
	ROM_LOAD( "l04_e-10.bin", 0x0400, 0x0100, CRC(1dfad87a) SHA1(684844c24e630f46525df97ed67e2e63f7e66d0f) )	/* 32x32 tile lookup table */
	ROM_LOAD( "c04_e-07.bin", 0x0500, 0x0100, CRC(850064e0) SHA1(3884485e91bd82539d0d33f46b7abac60f4c3b1c) )	/* 16x16 tile lookup table */
	ROM_LOAD( "l09_e-11.bin", 0x0600, 0x0100, CRC(2bb68710) SHA1(cfb375316245cb8751e765f163e6acf071dda9ca) )	/* sprite lookup table */
	ROM_LOAD( "l10_e-12.bin", 0x0700, 0x0100, CRC(173184ef) SHA1(f91ecbdc67af1eed6757f660cac8a0e6866c1822) )	/* sprite palette bank */
	ROM_LOAD( "06l_e-06.bin", 0x0800, 0x0100, CRC(712ac508) SHA1(5349d722ab6733afdda65f6e0a98322f0d515e86) )	/* interrupt timing (not used) */
	ROM_LOAD( "k06_e-08.bin", 0x0900, 0x0100, CRC(0eaf5158) SHA1(bafd4108708f66cd7b280e47152b108f3e254fc9) )	/* video timing (not used) */
	ROM_LOAD( "l03_e-09.bin", 0x0a00, 0x0100, CRC(0d968558) SHA1(b376885ac8452b6cbf9ced81b1080bfd570d9b91) )	/* unknown (all 0) */
	ROM_LOAD( "03e_e-01.bin", 0x0b00, 0x0020, CRC(1acee376) SHA1(367094d924f8e0ec36d8310fada4d8143358f697) )	/* unknown (priority?) */
ROM_END

ROM_START( savgbees )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "ee04e.11m",    0x0000, 0x4000, CRC(c0caf442) SHA1(f6e137c1707db620db4f79a1e038101bb3acf812) )
	ROM_LOAD( "ee03e.10m",    0x4000, 0x4000, CRC(9cd70ae1) SHA1(ad2c5de469cdc04a8e877e334a93d68d722cec9a) )
	ROM_LOAD( "ee02e.9m",     0x8000, 0x4000, CRC(a04e6368) SHA1(ed350fb490f8f84dcd9e4a9f5fb3b23079d6b996) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "ee01e.11e",    0x00000, 0x4000, CRC(93d3f952) SHA1(5c86d1ddf03083ac2787efb7a29c09b2f46ec3fa) )

	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ee00e.5c",     0x00000, 0x2000, CRC(5972f95f) SHA1(7b90ceca37dba773f72a80da6272b00061526348) ) /* Characters */

	ROM_REGION( 0x04000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "h01_ee08.bin", 0x00000, 0x4000, CRC(96a65c1d) SHA1(3b49c64b32f01ec72cf2d943bfe3aa575d62a765) ) /* 32x32 tiles planes 0-1 */

	ROM_REGION( 0x08000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "a03_ee06.bin", 0x00000, 0x4000, CRC(6039bdd1) SHA1(01156e02ed59e6c1e55204729e515cd4419568fb) ) /* 16x16 tiles planes 0-1 */
	ROM_LOAD( "a02_ee05.bin", 0x04000, 0x4000, CRC(b32d8252) SHA1(738225146ba38f2a9216fda278838e7ebb29a0bb) ) /* 16x16 tiles planes 2-3 */

	ROM_REGION( 0x08000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "j11_ee10.bin", 0x00000, 0x4000, CRC(bc83e265) SHA1(ac9b4cce9e539c560414abf2fc239910f2bfbb2d) ) /* Sprites planes 0-1 */
	ROM_LOAD( "j12_ee11.bin", 0x04000, 0x4000, CRC(0e0f300d) SHA1(2f973748e459b16673115abf7de8615219e39fa4) ) /* Sprites planes 2-3 */

	ROM_REGION( 0x6000, REGION_GFX5, 0 )	/* background tilemaps */
	ROM_LOAD( "c01_ee07.bin", 0x0000, 0x4000, CRC(3625a68d) SHA1(83010ca356385b713bafe03a502c566f6a9a8365) )	/* Front Tile Map */
	ROM_LOAD( "h04_ee09.bin", 0x4000, 0x2000, CRC(6057c907) SHA1(886790641b84b8cd659d2eb5fd1adbabdd7dad3d) )	/* Back Tile map */

	ROM_REGION( 0x0b20, REGION_PROMS, 0 )
	ROM_LOAD( "02d_e-02.bin", 0x0000, 0x0100, CRC(8d0d5935) SHA1(a0ab827ff3b641965ef851893c399e3988fde55e) )	/* red component */
	ROM_LOAD( "03d_e-03.bin", 0x0100, 0x0100, CRC(d3c17efc) SHA1(af88340287bd732c91bc5c75970f9de0431b4304) )	/* green component */
	ROM_LOAD( "04d_e-04.bin", 0x0200, 0x0100, CRC(58ba964c) SHA1(1f98f8e484a0462f1a9fadef9e57612a32652599) )	/* blue component */
	ROM_LOAD( "06f_e-05.bin", 0x0300, 0x0100, CRC(35a03579) SHA1(1f1b8c777622a1f5564409c5f3ce69cc68199dae) )	/* char lookup table */
	ROM_LOAD( "l04_e-10.bin", 0x0400, 0x0100, CRC(1dfad87a) SHA1(684844c24e630f46525df97ed67e2e63f7e66d0f) )	/* 32x32 tile lookup table */
	ROM_LOAD( "c04_e-07.bin", 0x0500, 0x0100, CRC(850064e0) SHA1(3884485e91bd82539d0d33f46b7abac60f4c3b1c) )	/* 16x16 tile lookup table */
	ROM_LOAD( "l09_e-11.bin", 0x0600, 0x0100, CRC(2bb68710) SHA1(cfb375316245cb8751e765f163e6acf071dda9ca) )	/* sprite lookup table */
	ROM_LOAD( "l10_e-12.bin", 0x0700, 0x0100, CRC(173184ef) SHA1(f91ecbdc67af1eed6757f660cac8a0e6866c1822) )	/* sprite palette bank */
	ROM_LOAD( "06l_e-06.bin", 0x0800, 0x0100, CRC(712ac508) SHA1(5349d722ab6733afdda65f6e0a98322f0d515e86) )	/* interrupt timing (not used) */
	ROM_LOAD( "k06_e-08.bin", 0x0900, 0x0100, CRC(0eaf5158) SHA1(bafd4108708f66cd7b280e47152b108f3e254fc9) )	/* video timing (not used) */
	ROM_LOAD( "l03_e-09.bin", 0x0a00, 0x0100, CRC(0d968558) SHA1(b376885ac8452b6cbf9ced81b1080bfd570d9b91) )	/* unknown (all 0) */
	ROM_LOAD( "03e_e-01.bin", 0x0b00, 0x0020, CRC(1acee376) SHA1(367094d924f8e0ec36d8310fada4d8143358f697) )	/* unknown (priority?) */
ROM_END



GAME( 1985, exedexes, 0,        exedexes, exedexes, 0, ROT270, "Capcom", "Exed Exes" )
GAME( 1985, savgbees, exedexes, exedexes, exedexes, 0, ROT270, "Capcom (Memetron license)", "Savage Bees" )
