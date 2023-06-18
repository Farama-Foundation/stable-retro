/***************************************************************************

Blue Print memory map (preliminary)

driver by Nicola Salmoria


CPU #1
0000-4fff ROM
8000-87ff RAM
9000-93ff Video RAM
b000-b0ff Sprite RAM
f000-f3ff Color RAM

read:
c000      IN0
c001      IN1
c003      read dip switches from the second CPU

e000      Watchdog reset

write:
c000      bit 0,1 = coin counters
d000      command for the second CPU
e000      bit 1 = flip screen

CPU #2
0000-0fff ROM
2000-2fff ROM
4000-43ff RAM

read:
6002      8910 #0 read
8002      8910 #1 read

write:
6000      8910 #0 control
6001      8910 #0 write
8000      8910 #1 control
8001      8910 #1 write

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"



extern unsigned char *blueprnt_scrollram;

PALETTE_INIT( blueprnt );
WRITE_HANDLER( blueprnt_flipscreen_w );
VIDEO_UPDATE( blueprnt );



static int dipsw;

static WRITE_HANDLER( dipsw_w )
{
	dipsw = data;
}

static READ_HANDLER( blueprnt_sh_dipsw_r )
{
	return dipsw;
}

static WRITE_HANDLER( blueprnt_sound_command_w )
{
	soundlatch_w(offset,data);
	cpu_set_irq_line(1,IRQ_LINE_NMI,PULSE_LINE);
}

static WRITE_HANDLER( blueprnt_coin_w )
{
	static int lastval;

	if (lastval == data) return;
	coin_counter_w (0, data & 0x01);
	coin_counter_w (1, data & 0x02);
	lastval = data;
}



static MEMORY_READ_START( readmem )
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0x9000, 0x93ff, MRA_RAM },
	{ 0x9400, 0x97ff, videoram_r },	/* mirror address, I THINK */
	{ 0xa000, 0xa01f, MRA_RAM },
	{ 0xb000, 0xb0ff, MRA_RAM },
	{ 0xc000, 0xc000, input_port_0_r },
	{ 0xc001, 0xc001, input_port_1_r },
	{ 0xc003, 0xc003, blueprnt_sh_dipsw_r },
	{ 0xe000, 0xe000, watchdog_reset_r },
	{ 0xf000, 0xf3ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x5fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x9000, 0x93ff, videoram_w, &videoram, &videoram_size },
	{ 0xa000, 0xa01f, MWA_RAM, &blueprnt_scrollram },
	{ 0xb000, 0xb0ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xc000, 0xc000, blueprnt_coin_w },
	{ 0xd000, 0xd000, blueprnt_sound_command_w },
	{ 0xe000, 0xe000, blueprnt_flipscreen_w },	/* + gfx bank */
	{ 0xf000, 0xf3ff, colorram_w, &colorram },
MEMORY_END



static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x0fff, MRA_ROM },
	{ 0x2000, 0x2fff, MRA_ROM },
	{ 0x4000, 0x43ff, MRA_RAM },
	{ 0x6002, 0x6002, AY8910_read_port_0_r },
	{ 0x8002, 0x8002, AY8910_read_port_1_r },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x0fff, MWA_ROM },
	{ 0x2000, 0x2fff, MWA_ROM },
	{ 0x4000, 0x43ff, MWA_RAM },
	{ 0x6000, 0x6000, AY8910_control_port_0_w },
	{ 0x6001, 0x6001, AY8910_write_port_0_w },
	{ 0x8000, 0x8000, AY8910_control_port_1_w },
	{ 0x8001, 0x8001, AY8910_write_port_1_w },
MEMORY_END



INPUT_PORTS_START( blueprnt )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_TILT )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_SERVICE( 0x04, IP_ACTIVE_HIGH )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x06, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "20000" )
	PORT_DIPSETTING(    0x02, "30000" )
	PORT_DIPSETTING(    0x04, "40000" )
	PORT_DIPSETTING(    0x06, "50000" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "Maze Monster" )
	PORT_DIPSETTING(    0x00, "2nd Maze" )
	PORT_DIPSETTING(    0x10, "3rd Maze" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x10, "Medium" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x30, "Hardest" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( saturn )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x40, "4" )
	PORT_DIPSETTING(    0x80, "5" )
	PORT_DIPSETTING(    0xc0, "6" )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x02, "A 2/1 B 1/3" )
	PORT_DIPSETTING(    0x00, "A 1/1 B 1/6" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters */
	2,	/* 2 bits per pixel */
	{ 512*8*8, 0 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	8,16,	/* 8*16 sprites */
	256,	/* 256 sprites */
	3,	/* 3 bits per pixel */
	{ 2*128*16*16, 128*16*16, 0 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	16*8	/* every sprite takes 16 consecutive bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,       0, 128 },
	{ REGION_GFX2, 0, &spritelayout, 128*4,   1 },
	{ -1 } /* end of array */
};



static struct AY8910interface ay8910_interface =
{
	2,	/* 2 chips */
	10000000/8,	/* 1.25 MHz (4H) */
	{ 25, 25 },
	{            0, input_port_2_r },
	{ soundlatch_r, input_port_3_r },
	{ dipsw_w },
	{ 0 }
};



static MACHINE_DRIVER_START( blueprnt )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,10000000/4)	/* 2.5 MHz (2H) */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80,10000000/4)	/* can't use CPU_AUDIO_CPU because this CPU reads the dip switches */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,4)	/* IRQs connected to 32V */
											/* NMIs are caused by the main CPU */
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(16)
	MDRV_COLORTABLE_LENGTH(128*4+8)

	MDRV_PALETTE_INIT(blueprnt)
	MDRV_VIDEO_START(generic)
	MDRV_VIDEO_UPDATE(blueprnt)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( blueprnt )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "1m",           0x0000, 0x1000, CRC(b20069a6) SHA1(aa0a61c898ec58fc4872a24666f422e1abdc09f3) )
	ROM_LOAD( "1n",           0x1000, 0x1000, CRC(4a30302e) SHA1(a3a22b78585cc9677bf03bbfeb20afb05f026075) )
	ROM_LOAD( "1p",           0x2000, 0x1000, CRC(6866ca07) SHA1(a0df14eee9240fad42ceb6f926d34755e8442411) )
	ROM_LOAD( "1r",           0x3000, 0x1000, CRC(5d3cfac3) SHA1(7e6ab8398d799aaf0fcaa0769a827471d8c872e9) )
	ROM_LOAD( "1s",           0x4000, 0x1000, CRC(a556cac4) SHA1(0fe7070c70792d883c29f3d12a33238b5ed8af22) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "3u",           0x0000, 0x1000, CRC(fd38777a) SHA1(0ed230e0fa047d3171e7141e5620b4c750b07629) )
	ROM_LOAD( "3v",           0x2000, 0x1000, CRC(33d5bf5b) SHA1(3ac684cd48559cd0eab32f9e7ce3ec6eca88dcd4) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "c3",           0x0000, 0x1000, CRC(ac2a61bc) SHA1(e56708d261648478d1dae4769118546411299e59) )
	ROM_LOAD( "d3",           0x1000, 0x1000, CRC(81fe85d7) SHA1(fa637631d25f7499d2325cce77d11e1d624f5e07) )

	ROM_REGION( 0x3000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "d17",          0x0000, 0x1000, CRC(a73b6483) SHA1(9f7756d032a8ffaa4aa236fc5117f476916986e0) )
	ROM_LOAD( "d18",          0x1000, 0x1000, CRC(7d622550) SHA1(8283debff8253996513148629ec55831e48e8e92) )
	ROM_LOAD( "d20",          0x2000, 0x1000, CRC(2fcb4f26) SHA1(508cb2800737bad0a7dea0789d122b7c802aecfd) )
ROM_END

ROM_START( blueprnj )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "bp_01.bin",    0x0000, 0x1000, CRC(2e746693) SHA1(4a9bb023f753ba792d1db86a0fb128d5261db685) )
	ROM_LOAD( "bp_02.bin",    0x1000, 0x1000, CRC(a0eb0b8e) SHA1(b3c830b61172880fd2843a47350d8cb9461a25a4) )
	ROM_LOAD( "bp_03.bin",    0x2000, 0x1000, CRC(c34981bb) SHA1(1c7fa9d599b3458f665e95d92cafda8851098c8f) )
	ROM_LOAD( "bp_04.bin",    0x3000, 0x1000, CRC(525e77b5) SHA1(95c898be78881802b801f071d1d88062dcb1b798) )
	ROM_LOAD( "bp_05.bin",    0x4000, 0x1000, CRC(431a015f) SHA1(e00912ac501bdd6750f63b53204553a40ad6605a) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "3u",           0x0000, 0x1000, CRC(fd38777a) SHA1(0ed230e0fa047d3171e7141e5620b4c750b07629) )
	ROM_LOAD( "3v",           0x2000, 0x1000, CRC(33d5bf5b) SHA1(3ac684cd48559cd0eab32f9e7ce3ec6eca88dcd4) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "bp_09.bin",    0x0000, 0x0800, CRC(43718c34) SHA1(5df4794a38866c7f03b264581c8555b9bec3969f) )
	ROM_LOAD( "bp_08.bin",    0x1000, 0x0800, CRC(d3ce077d) SHA1(a9086b494437f9d4d3c0a6c36595a03d3a229a24) )

	ROM_REGION( 0x3000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "bp_10.bin",    0x0000, 0x1000, CRC(83da108f) SHA1(575d6505bd3d600324c4f656e28218deaaa470e4) )
	ROM_LOAD( "bp_11.bin",    0x1000, 0x1000, CRC(b440f32f) SHA1(bd464ff324d4ef7c7c924886417b55bcb6f74fb9) )
	ROM_LOAD( "bp_12.bin",    0x2000, 0x1000, CRC(23026765) SHA1(9b16de37922208f4f2d2afc94189f11f5e5011fa) )
ROM_END

ROM_START( saturn )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "r1",           0x0000, 0x1000, CRC(18a6d68e) SHA1(816baca24dd75c6f9d4c91c86f90825dbb9a1347) )
	ROM_LOAD( "r2",           0x1000, 0x1000, CRC(a7dd2665) SHA1(02d03fb436c704ccdbad751ccf034742fcd4ae43) )
	ROM_LOAD( "r3",           0x2000, 0x1000, CRC(b9cfa791) SHA1(4f4c7b1dd347c6f402124ddf235a02e812dc536d) )
	ROM_LOAD( "r4",           0x3000, 0x1000, CRC(c5a997e7) SHA1(134d2719bf9f14dc22365d03384271f6a6f3a448) )
	ROM_LOAD( "r5",           0x4000, 0x1000, CRC(43444d00) SHA1(3b58c9387eac75e713943a5ea9c8922634772a67) )
	ROM_LOAD( "r6",           0x5000, 0x1000, CRC(4d4821f6) SHA1(e414751d73c3f6e86d265540c1ebf69b95088b43) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "r7",           0x0000, 0x1000, CRC(dd43e02f) SHA1(1d95a307cb4ef523f024cb9c60382a2ac8c17b1c) )
	ROM_LOAD( "r8",           0x2000, 0x1000, CRC(7f9d0877) SHA1(335b17d187089e91bd3002778821921e73ec59d2) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "r10",          0x0000, 0x1000, CRC(35987d61) SHA1(964503c3b17299b27b611943eebca9bc7c93a18c) )
	ROM_LOAD( "r9",           0x1000, 0x1000, CRC(ca6a7fda) SHA1(fac72535bb30d3527effa64900830403fb98c5c5) )

	ROM_REGION( 0x3000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "r11",          0x0000, 0x1000, CRC(6e4e6e5d) SHA1(f91188712e4b93d5676238c60d5a698891c3167a) )
	ROM_LOAD( "r12",          0x1000, 0x1000, CRC(46fc049e) SHA1(dc3027c2dcbf7a9b2eeeb165d0b99ce188f26d20) )
	ROM_LOAD( "r13",          0x2000, 0x1000, CRC(8b3e8c32) SHA1(65e2bf4a9f45be39419d85b2ee46b9c5eeff8f57) )
ROM_END



GAME( 1982, blueprnt, 0,        blueprnt, blueprnt, 0, ROT270, "[Zilec Electronics] Bally Midway", "Blue Print (Midway)" )
GAME( 1982, blueprnj, blueprnt, blueprnt, blueprnt, 0, ROT270, "[Zilec Electronics] Jaleco", "Blue Print (Jaleco)" )
GAME( 1983, saturn,   0,        blueprnt, saturn,   0, ROT270, "[Zilec Electronics] Jaleco", "Saturn" )
