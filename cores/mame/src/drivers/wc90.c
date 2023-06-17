/*
World Cup 90 ( Tecmo ) driver
-----------------------------

Ernesto Corvi
(ernesto@imagina.com)

TODO:
- Dip switches mapping is not complete. ( Anyone has the manual handy? )
- Missing drums, they might be internal to the YM2608.
- Hook up trackball controls in wc90t.

CPU #1 : Handles background & foreground tiles, controllers, dipswitches.
CPU #2 : Handles sprites and palette
CPU #3 : Audio.

Memory Layout:

CPU #1
0000-8000 ROM
8000-9000 RAM
a000-a800 Color Ram for background #1 tiles
a800-b000 Video Ram for background #1 tiles
c000-c800 Color Ram for background #2 tiles
c800-d000 Video Ram for background #2 tiles
e000-e800 Color Ram for foreground tiles
e800-f000 Video Ram for foreground tiles
f800-fc00 Common Ram with CPU #2
fc00-fc00 Stick 1 input port
fc02-fc02 Stick 2 input port
fc05-fc05 Start buttons and Coins input port
fc06-fc06 Dip Switch A
fc07-fc07 Dip Switch B

CPU #2
0000-c000 ROM
c000-d000 RAM
d000-d800 RAM Sprite Ram
e000-e800 RAM Palette Ram
f800-fc00 Common Ram with CPU #1

CPU #3
0000-0xc000 ROM
???????????


To enter into input test mode:
-keep pressed one of the start buttons during P.O.S.T.(in wc90 & wc90a).
-keep pressed both start buttons during P.O.S.T. until the cross hatch test fade out(in wc90t).
Press one of the start buttons to exit.

*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"


extern data8_t *wc90_fgvideoram,*wc90_bgvideoram,*wc90_txvideoram;


extern data8_t *wc90_scroll0xlo, *wc90_scroll0xhi;
extern data8_t *wc90_scroll1xlo, *wc90_scroll1xhi;
extern data8_t *wc90_scroll2xlo, *wc90_scroll2xhi;

extern data8_t *wc90_scroll0ylo, *wc90_scroll0yhi;
extern data8_t *wc90_scroll1ylo, *wc90_scroll1yhi;
extern data8_t *wc90_scroll2ylo, *wc90_scroll2yhi;

VIDEO_START( wc90 );
VIDEO_START( wc90t );
WRITE_HANDLER( wc90_fgvideoram_w );
WRITE_HANDLER( wc90_bgvideoram_w );
WRITE_HANDLER( wc90_txvideoram_w );
VIDEO_UPDATE( wc90 );


static data8_t *wc90_shared;

static READ_HANDLER( wc90_shared_r )
{
	return wc90_shared[offset];
}

static WRITE_HANDLER( wc90_shared_w )
{
	wc90_shared[offset] = data;
}

static WRITE_HANDLER( wc90_bankswitch_w )
{
	int bankaddress;
	data8_t *RAM = memory_region(REGION_CPU1);


	bankaddress = 0x10000 + ( ( data & 0xf8 ) << 8 );
	cpu_setbank( 1,&RAM[bankaddress] );
}

static WRITE_HANDLER( wc90_bankswitch1_w )
{
	int bankaddress;
	data8_t *RAM = memory_region(REGION_CPU2);


	bankaddress = 0x10000 + ( ( data & 0xf8 ) << 8 );
	cpu_setbank( 2,&RAM[bankaddress] );
}

static WRITE_HANDLER( wc90_sound_command_w )
{
	soundlatch_w(offset,data);
	cpu_set_irq_line(2,IRQ_LINE_NMI,PULSE_LINE);
}



static MEMORY_READ_START( wc90_readmem1 )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x9fff, MRA_RAM }, /* Main RAM */
	{ 0xa000, 0xafff, MRA_RAM }, /* fg video ram */
	{ 0xb000, 0xbfff, MRA_RAM },
	{ 0xc000, 0xcfff, MRA_RAM }, /* bg video ram */
	{ 0xd000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xefff, MRA_RAM }, /* tx video ram */
	{ 0xf000, 0xf7ff, MRA_BANK1 },
	{ 0xf800, 0xfbff, wc90_shared_r },
	{ 0xfc00, 0xfc00, input_port_0_r }, /* Stick 1 */
	{ 0xfc02, 0xfc02, input_port_1_r }, /* Stick 2 */
	{ 0xfc05, 0xfc05, input_port_4_r }, /* Start & Coin */
	{ 0xfc06, 0xfc06, input_port_2_r }, /* DIP Switch A */
	{ 0xfc07, 0xfc07, input_port_3_r }, /* DIP Switch B */
MEMORY_END

static MEMORY_READ_START( wc90_readmem2 )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xcfff, MRA_RAM },
	{ 0xd000, 0xd7ff, MRA_RAM },
	{ 0xd800, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe7ff, MRA_RAM },
	{ 0xf000, 0xf7ff, MRA_BANK2 },
	{ 0xf800, 0xfbff, wc90_shared_r },
MEMORY_END

static MEMORY_WRITE_START( wc90_writemem1 )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x9fff, MWA_RAM },
	{ 0xa000, 0xafff, wc90_fgvideoram_w, &wc90_fgvideoram },
	{ 0xb000, 0xbfff, MWA_RAM },
	{ 0xc000, 0xcfff, wc90_bgvideoram_w, &wc90_bgvideoram },
	{ 0xd000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xefff, wc90_txvideoram_w, &wc90_txvideoram },
	{ 0xf000, 0xf7ff, MWA_ROM },
	{ 0xf800, 0xfbff, wc90_shared_w, &wc90_shared },
	{ 0xfc02, 0xfc02, MWA_RAM, &wc90_scroll0ylo },
	{ 0xfc03, 0xfc03, MWA_RAM, &wc90_scroll0yhi },
	{ 0xfc06, 0xfc06, MWA_RAM, &wc90_scroll0xlo },
	{ 0xfc07, 0xfc07, MWA_RAM, &wc90_scroll0xhi },
	{ 0xfc22, 0xfc22, MWA_RAM, &wc90_scroll1ylo },
	{ 0xfc23, 0xfc23, MWA_RAM, &wc90_scroll1yhi },
	{ 0xfc26, 0xfc26, MWA_RAM, &wc90_scroll1xlo },
	{ 0xfc27, 0xfc27, MWA_RAM, &wc90_scroll1xhi },
	{ 0xfc42, 0xfc42, MWA_RAM, &wc90_scroll2ylo },
	{ 0xfc43, 0xfc43, MWA_RAM, &wc90_scroll2yhi },
	{ 0xfc46, 0xfc46, MWA_RAM, &wc90_scroll2xlo },
	{ 0xfc47, 0xfc47, MWA_RAM, &wc90_scroll2xhi },
	{ 0xfcc0, 0xfcc0, wc90_sound_command_w },
	{ 0xfcd0, 0xfcd0, watchdog_reset_w },
	{ 0xfce0, 0xfce0, wc90_bankswitch_w },
MEMORY_END

static MEMORY_WRITE_START( wc90_writemem2 )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, MWA_RAM },
	{ 0xd000, 0xd7ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xd800, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe7ff, paletteram_xxxxBBBBRRRRGGGG_swap_w, &paletteram },
	{ 0xf000, 0xf7ff, MWA_ROM },
	{ 0xf800, 0xfbff, wc90_shared_w },
	{ 0xfc00, 0xfc00, wc90_bankswitch1_w },
	{ 0xfc01, 0xfc01, watchdog_reset_w },
MEMORY_END

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xf000, 0xf7ff, MRA_RAM },
	{ 0xf800, 0xf800, YM2608_status_port_0_A_r },
	{ 0xf802, 0xf802, YM2608_status_port_0_B_r },
	{ 0xfc00, 0xfc00, MRA_NOP }, /* ??? adpcm ??? */
	{ 0xfc10, 0xfc10, soundlatch_r },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xf000, 0xf7ff, MWA_RAM },
	{ 0xf800, 0xf800, YM2608_control_port_0_A_w },
	{ 0xf801, 0xf801, YM2608_data_port_0_A_w },
	{ 0xf802, 0xf802, YM2608_control_port_0_B_w },
	{ 0xf803, 0xf803, YM2608_data_port_0_B_w },
MEMORY_END



INPUT_PORTS_START( wc90 )
	PORT_START	/* IN0 bit 0-5 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN1 bit 0-5 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSWA */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, "10 Coins/1 Credit" )
	PORT_DIPSETTING(    0x08, DEF_STR( 9C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 8C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 7C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x30, "Easy" )
	PORT_DIPSETTING(    0x10, "Normal" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x40, 0x40, "Countdown Speed" )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x00, "Fast" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START	/* DSWB */
	PORT_DIPNAME( 0x03, 0x03, "1 Player Game Time" )
	PORT_DIPSETTING(    0x01, "1:00" )
	PORT_DIPSETTING(    0x02, "1:30" )
	PORT_DIPSETTING(    0x03, "2:00" )
	PORT_DIPSETTING(    0x00, "2:30" )
	PORT_DIPNAME( 0x1c, 0x1c, "2 Players Game Time" )
	PORT_DIPSETTING(    0x0c, "1:00" )
	PORT_DIPSETTING(    0x14, "1:30" )
	PORT_DIPSETTING(    0x04, "2:00" )
	PORT_DIPSETTING(    0x18, "2:30" )
	PORT_DIPSETTING(    0x1c, "3:00" )
	PORT_DIPSETTING(    0x08, "3:30" )
	PORT_DIPSETTING(    0x10, "4:00" )
	PORT_DIPSETTING(    0x00, "5:00" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "Language" )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPSETTING(    0x80, "Japanese" )

	PORT_START	/* IN2 bit 0-3 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
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
	16,16,
	RGN_FRAC(1,2),
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, RGN_FRAC(1,2)+0*4, RGN_FRAC(1,2)+1*4, 2*4, 3*4, RGN_FRAC(1,2)+2*4, RGN_FRAC(1,2)+3*4,
			16*8+0*4, 16*8+1*4, RGN_FRAC(1,2)+16*8+0*4, RGN_FRAC(1,2)+16*8+1*4, 16*8+2*4, 16*8+3*4, RGN_FRAC(1,2)+16*8+2*4, RGN_FRAC(1,2)+16*8+3*4 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			16*16, 17*16, 18*16, 19*16, 20*16, 21*16, 22*16, 23*16 },
	64*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x00000, &charlayout,      	1*16*16, 16*16 },
	{ REGION_GFX2, 0x00000, &tilelayout,		2*16*16, 16*16 },
	{ REGION_GFX3, 0x00000, &tilelayout,		3*16*16, 16*16 },
	{ REGION_GFX4, 0x00000, &spritelayout,		0*16*16, 16*16 }, /* sprites*/
	{ -1 } /* end of array */
};



/* handler called by the 2608 emulator when the internal timers cause an IRQ */
static void irqhandler(int irq)
{
	cpu_set_irq_line(2,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2608interface ym2608_interface =
{
	1,
	8000000,
	{ 50 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler },
	{ REGION_SOUND1 },
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) }
};

static MACHINE_DRIVER_START( wc90 )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 6000000)	/* 6.0 MHz ??? */
	MDRV_CPU_MEMORY(wc90_readmem1,wc90_writemem1)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, 6000000)	/* 6.0 MHz ??? */
	MDRV_CPU_MEMORY(wc90_readmem2,wc90_writemem2)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 4 MHz ???? */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
								/* NMIs are triggered by the main CPU */
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(wc90)
	MDRV_VIDEO_UPDATE(wc90)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2608, ym2608_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( wc90t )

	MDRV_IMPORT_FROM( wc90 )
	MDRV_VIDEO_START( wc90t )
MACHINE_DRIVER_END

ROM_START( wc90 )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )	/* 128k for code */
	ROM_LOAD( "ic87_01.bin",  0x00000, 0x08000, CRC(4a1affbc) SHA1(bc531e97ca31c66fdac194e2d79d5c6ba1300556) )	/* c000-ffff is not used */
	ROM_LOAD( "ic95_02.bin",  0x10000, 0x10000, CRC(847d439c) SHA1(eade31050da9e84feb4406e327d050a7496871b7) )	/* banked at f000-f7ff */

	ROM_REGION( 0x20000, REGION_CPU2, 0 )	/* 96k for code */  /* Second CPU */
	ROM_LOAD( "ic67_04.bin",  0x00000, 0x10000, CRC(dc6eaf00) SHA1(d53924070a59eee35dc0e6465702e4f04e61a073) )	/* c000-ffff is not used */
	ROM_LOAD( "ic56_03.bin",  0x10000, 0x10000, CRC(1ac02b3b) SHA1(4f8dc049d404072150342f3c2df04789a73ce244) )	/* banked at f000-f7ff */

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "ic54_05.bin",  0x00000, 0x10000, CRC(27c348b3) SHA1(cf19ff4ae4f323ae3e5a905249b7af8ae342202a) )

	ROM_REGION( 0x010000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic85_07v.bin", 0x00000, 0x10000, CRC(c5219426) SHA1(95e21fcd7de7d418ec287ae7087f6244c6bce5a8) )	/* characters */

	ROM_REGION( 0x040000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ic86_08v.bin", 0x00000, 0x20000, CRC(8fa1a1ff) SHA1(ce624617ac8c8b54e41294cf5dca7a09c91f53ba) )	/* tiles #1 */
	ROM_LOAD( "ic90_09v.bin", 0x20000, 0x20000, CRC(99f8841c) SHA1(1969b4d78ca00924a7550826e1c4f4fa0588ef02) )	/* tiles #2 */

	ROM_REGION( 0x040000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "ic87_10v.bin", 0x00000, 0x20000, CRC(8232093d) SHA1(59bf9c9a858b47326cf0c64b1ee6ac727a15a20b) )	/* tiles #3 */
	ROM_LOAD( "ic91_11v.bin", 0x20000, 0x20000, CRC(188d3789) SHA1(35654a99a20735bae09b32f74255f8132dee9af2) )	/* tiles #4 */

	ROM_REGION( 0x080000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "ic50_12v.bin", 0x00000, 0x20000, CRC(da1fe922) SHA1(5184053c2b7dd2bf1cd2e9f783686f2c0db7e47b) )	/* sprites  */
	ROM_LOAD( "ic54_13v.bin", 0x20000, 0x20000, CRC(9ad03c2c) SHA1(1c1947f9b51a58002e9992fc7c0c1a1c59b4d740) )	/* sprites  */
	ROM_LOAD( "ic60_14v.bin", 0x40000, 0x20000, CRC(499dfb1b) SHA1(ac67985d36fea18c82a4ea00019d9e6e4bcb5d0d) )	/* sprites  */
	ROM_LOAD( "ic65_15v.bin", 0x60000, 0x20000, CRC(d8ea5c81) SHA1(ccb3f7d565b1c1b8e874a2df91cda40dde2962ed) )	/* sprites  */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* 64k for ADPCM samples */
	ROM_LOAD( "ic82_06.bin",  0x00000, 0x20000, CRC(2fd692ed) SHA1(0273dc39181504320bec0187d074b2f86c821508) )
ROM_END

ROM_START( wc90a )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )	/* 128k for code */
	ROM_LOAD( "wc90-1.bin",   0x00000, 0x08000, CRC(d1804e1a) SHA1(eec7374f4d23c89843f38fffff436635adb43b63) )	/* c000-ffff is not used */
	ROM_LOAD( "ic95_02.bin",  0x10000, 0x10000, CRC(847d439c) SHA1(eade31050da9e84feb4406e327d050a7496871b7) )	/* banked at f000-f7ff */

	ROM_REGION( 0x20000, REGION_CPU2, 0 )	/* 96k for code */  /* Second CPU */
	ROM_LOAD( "ic67_04.bin",  0x00000, 0x10000, CRC(dc6eaf00) SHA1(d53924070a59eee35dc0e6465702e4f04e61a073) )	/* c000-ffff is not used */
	ROM_LOAD( "ic56_03.bin",  0x10000, 0x10000, CRC(1ac02b3b) SHA1(4f8dc049d404072150342f3c2df04789a73ce244) )	/* banked at f000-f7ff */

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "ic54_05.bin",  0x00000, 0x10000, CRC(27c348b3) SHA1(cf19ff4ae4f323ae3e5a905249b7af8ae342202a) )

	ROM_REGION( 0x010000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic85_07v.bin", 0x00000, 0x10000, CRC(c5219426) SHA1(95e21fcd7de7d418ec287ae7087f6244c6bce5a8) )	/* characters */

	ROM_REGION( 0x040000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ic86_08v.bin", 0x00000, 0x20000, CRC(8fa1a1ff) SHA1(ce624617ac8c8b54e41294cf5dca7a09c91f53ba) )	/* tiles #1 */
	ROM_LOAD( "ic90_09v.bin", 0x20000, 0x20000, CRC(99f8841c) SHA1(1969b4d78ca00924a7550826e1c4f4fa0588ef02) )	/* tiles #2 */

	ROM_REGION( 0x040000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "ic87_10v.bin", 0x00000, 0x20000, CRC(8232093d) SHA1(59bf9c9a858b47326cf0c64b1ee6ac727a15a20b) )	/* tiles #3 */
	ROM_LOAD( "ic91_11v.bin", 0x20000, 0x20000, CRC(188d3789) SHA1(35654a99a20735bae09b32f74255f8132dee9af2) )	/* tiles #4 */

	ROM_REGION( 0x080000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "ic50_12v.bin", 0x00000, 0x20000, CRC(da1fe922) SHA1(5184053c2b7dd2bf1cd2e9f783686f2c0db7e47b) )	/* sprites  */
	ROM_LOAD( "ic54_13v.bin", 0x20000, 0x20000, CRC(9ad03c2c) SHA1(1c1947f9b51a58002e9992fc7c0c1a1c59b4d740) )	/* sprites  */
	ROM_LOAD( "ic60_14v.bin", 0x40000, 0x20000, CRC(499dfb1b) SHA1(ac67985d36fea18c82a4ea00019d9e6e4bcb5d0d) )	/* sprites  */
	ROM_LOAD( "ic65_15v.bin", 0x60000, 0x20000, CRC(d8ea5c81) SHA1(ccb3f7d565b1c1b8e874a2df91cda40dde2962ed) )	/* sprites  */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* 64k for ADPCM samples */
	ROM_LOAD( "ic82_06.bin",  0x00000, 0x20000, CRC(2fd692ed) SHA1(0273dc39181504320bec0187d074b2f86c821508) )
ROM_END

ROM_START( wc90t )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )	/* 128k for code */
	ROM_LOAD( "wc90a-1.bin",  0x00000, 0x08000, CRC(b6f51a68) SHA1(e0263dee35bf99cb4288a1df825bbbca17c85d36) )	/* c000-ffff is not used */
	ROM_LOAD( "wc90a-2.bin",  0x10000, 0x10000, CRC(c50f2a98) SHA1(0fbeabadebfa75515d5e35bfcc565ecfa4d6e693) )	/* banked at f000-f7ff */

	ROM_REGION( 0x20000, REGION_CPU2, 0 )	/* 96k for code */  /* Second CPU */
	ROM_LOAD( "ic67_04.bin",  0x00000, 0x10000, CRC(dc6eaf00) SHA1(d53924070a59eee35dc0e6465702e4f04e61a073) )	/* c000-ffff is not used */
	ROM_LOAD( "wc90a-3.bin",  0x10000, 0x10000, CRC(8c7a9542) SHA1(a06a7cd40d41692c4cc2a35d9c69b944c5baf163) )	/* banked at f000-f7ff */

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "ic54_05.bin",  0x00000, 0x10000, CRC(27c348b3) SHA1(cf19ff4ae4f323ae3e5a905249b7af8ae342202a) )

	ROM_REGION( 0x010000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic85_07v.bin", 0x00000, 0x10000, CRC(c5219426) SHA1(95e21fcd7de7d418ec287ae7087f6244c6bce5a8) )	/* characters */

	ROM_REGION( 0x040000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ic86_08v.bin", 0x00000, 0x20000, CRC(8fa1a1ff) SHA1(ce624617ac8c8b54e41294cf5dca7a09c91f53ba) )	/* tiles #1 */
	ROM_LOAD( "ic90_09v.bin", 0x20000, 0x20000, CRC(99f8841c) SHA1(1969b4d78ca00924a7550826e1c4f4fa0588ef02) )	/* tiles #2 */

	ROM_REGION( 0x040000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "ic87_10v.bin", 0x00000, 0x20000, CRC(8232093d) SHA1(59bf9c9a858b47326cf0c64b1ee6ac727a15a20b) )	/* tiles #3 */
	ROM_LOAD( "ic91_11v.bin", 0x20000, 0x20000, CRC(188d3789) SHA1(35654a99a20735bae09b32f74255f8132dee9af2) )	/* tiles #4 */

	ROM_REGION( 0x080000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "ic50_12v.bin", 0x00000, 0x20000, CRC(da1fe922) SHA1(5184053c2b7dd2bf1cd2e9f783686f2c0db7e47b) )	/* sprites  */
	ROM_LOAD( "ic54_13v.bin", 0x20000, 0x20000, CRC(9ad03c2c) SHA1(1c1947f9b51a58002e9992fc7c0c1a1c59b4d740) )	/* sprites  */
	ROM_LOAD( "ic60_14v.bin", 0x40000, 0x20000, CRC(499dfb1b) SHA1(ac67985d36fea18c82a4ea00019d9e6e4bcb5d0d) )	/* sprites  */
	ROM_LOAD( "ic65_15v.bin", 0x60000, 0x20000, CRC(d8ea5c81) SHA1(ccb3f7d565b1c1b8e874a2df91cda40dde2962ed) )	/* sprites  */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* 64k for ADPCM samples */
	ROM_LOAD( "ic82_06.bin",  0x00000, 0x20000, CRC(2fd692ed) SHA1(0273dc39181504320bec0187d074b2f86c821508) )
ROM_END



GAMEX( 1989, wc90,  0,    wc90, wc90, 0, ROT0, "Tecmo", "Tecmo World Cup '90 (set 1)", GAME_IMPERFECT_SOUND | GAME_NO_COCKTAIL )
GAMEX( 1989, wc90a, wc90, wc90, wc90, 0, ROT0, "Tecmo", "Tecmo World Cup '90 (set 2)", GAME_IMPERFECT_SOUND | GAME_NO_COCKTAIL )
GAMEX( 1989, wc90t, wc90, wc90t,wc90, 0, ROT0, "Tecmo", "Tecmo World Cup '90 (trackball)", GAME_IMPERFECT_SOUND | GAME_NO_COCKTAIL )
