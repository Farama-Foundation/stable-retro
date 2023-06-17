/***************************************************************************

Commando memory map (preliminary)

driver by Nicola Salmoria


MAIN CPU
0000-bfff ROM
d000-d3ff Video RAM
d400-d7ff Color RAM
d800-dbff background video RAM
dc00-dfff background color RAM
e000-ffff RAM
fe00-ff7f Sprites

read:
c000      IN0
c001      IN1
c002      IN2
c003      DSW1
c004      DSW2

write:
c808-c809 background scroll x position
c80a-c80b background scroll y position

SOUND CPU
0000-3fff ROM
4000-47ff RAM

write:
8000      YM2203 #1 control
8001      YM2203 #1 write
8002      YM2203 #2 control
8003      YM2203 #2 write

****************************************************************************

Note : there is an ingame typo bug that doesn't display the bonus life values
       correctly on the title screen in 'commando', 'commandj' and 'spaceinv'.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


extern unsigned char *commando_fgvideoram;
extern unsigned char *commando_bgvideoram;

WRITE_HANDLER( commando_fgvideoram_w );
WRITE_HANDLER( commando_bgvideoram_w );
WRITE_HANDLER( commando_scrollx_w );
WRITE_HANDLER( commando_scrolly_w );
WRITE_HANDLER( commando_c804_w );
VIDEO_START( commando );
VIDEO_UPDATE( commando );
VIDEO_EOF( commando );



static INTERRUPT_GEN( commando_interrupt )
{
	cpu_set_irq_line_and_vector(0, 0, HOLD_LINE, 0xd7);	/* RST 10h - VBLANK */
}



static MEMORY_READ_START( readmem )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xc000, input_port_0_r },
	{ 0xc001, 0xc001, input_port_1_r },
	{ 0xc002, 0xc002, input_port_2_r },
	{ 0xc003, 0xc003, input_port_3_r },
	{ 0xc004, 0xc004, input_port_4_r },
	{ 0xd000, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc800, 0xc800, soundlatch_w },
	{ 0xc804, 0xc804, commando_c804_w },
	{ 0xc808, 0xc809, commando_scrollx_w },
	{ 0xc80a, 0xc80b, commando_scrolly_w },
	{ 0xd000, 0xd7ff, commando_fgvideoram_w, &commando_fgvideoram },
	{ 0xd800, 0xdfff, commando_bgvideoram_w, &commando_bgvideoram },
	{ 0xe000, 0xfdff, MWA_RAM },
	{ 0xfe00, 0xff7f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xff80, 0xffff, MWA_RAM },
MEMORY_END



static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x47ff, MRA_RAM },
	{ 0x6000, 0x6000, soundlatch_r },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x47ff, MWA_RAM },
	{ 0x8000, 0x8000, YM2203_control_port_0_w },
	{ 0x8001, 0x8001, YM2203_write_port_0_w },
	{ 0x8002, 0x8002, YM2203_control_port_1_w },
	{ 0x8003, 0x8003, YM2203_write_port_1_w },
MEMORY_END



INPUT_PORTS_START( commando )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x03, 0x03, "Starting Stage" )
	PORT_DIPSETTING(    0x03, "1" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x07, "10K, then every 50K" )
	PORT_DIPSETTING(    0x03, "10K, then every 60K" )
	PORT_DIPSETTING(    0x05, "20K, then every 60K" )
	PORT_DIPSETTING(    0x01, "20K, then every 70K" )
	PORT_DIPSETTING(    0x06, "30K, then every 70K" )
	PORT_DIPSETTING(    0x02, "30K, then every 80K" )
	PORT_DIPSETTING(    0x04, "40K, then every 100K" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x10, "Normal" )
	PORT_DIPSETTING(    0x00, "Difficult" )
    PORT_DIPNAME( 0x20, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, "Upright One Player" )
	PORT_DIPSETTING(    0x40, "Upright Two Players" )
/*	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) ) */
	PORT_DIPSETTING(    0xc0, DEF_STR( Cocktail ) )
INPUT_PORTS_END

/* Same as 'commando', but "Service Mode" Dip Switches instead of "Demo Sound" Dip Switch */
INPUT_PORTS_START( commandu )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x03, 0x03, "Starting Stage" )
	PORT_DIPSETTING(    0x03, "1" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x07, "10K, then every 50K" )
	PORT_DIPSETTING(    0x03, "10K, then every 60K" )
	PORT_DIPSETTING(    0x05, "20K, then every 60K" )
	PORT_DIPSETTING(    0x01, "20K, then every 70K" )
	PORT_DIPSETTING(    0x06, "30K, then every 70K" )
	PORT_DIPSETTING(    0x02, "30K, then every 80K" )
	PORT_DIPSETTING(    0x04, "40K, then every 100K" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_SERVICE( 0x08, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x10, "Normal" )
	PORT_DIPSETTING(    0x00, "Difficult" )
    PORT_DIPNAME( 0x20, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, "Upright One Player" )
	PORT_DIPSETTING(    0x40, "Upright Two Players" )
/*	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) ) */
	PORT_DIPSETTING(    0xc0, DEF_STR( Cocktail ) )
INPUT_PORTS_END




static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	2,
	{ 4, 0 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8
};
static struct GfxLayout tilelayout =
{
	16,16,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(0,3), RGN_FRAC(1,3), RGN_FRAC(2,3) },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8
};
static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+4, RGN_FRAC(1,2)+0, 4, 0 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3,
			32*8+0, 32*8+1, 32*8+2, 32*8+3, 33*8+0, 33*8+1, 33*8+2, 33*8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,   192, 16 },	/* colors 192-255 */
	{ REGION_GFX2, 0, &tilelayout,     0, 16 },	/* colors   0-127 */
	{ REGION_GFX3, 0, &spritelayout, 128,  4 },	/* colors 128-191 */
	{ -1 } /* end of array */
};



static struct YM2203interface ym2203_interface =
{
	2,			/* 2 chips */
	1500000,	/* 1.5 MHz ? */
	{ YM2203_VOL(15,15), YM2203_VOL(15,15) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};



static MACHINE_DRIVER_START( commando )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz (?) */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(commando_interrupt,1)

	MDRV_CPU_ADD(Z80, 3000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 3 MHz (?) */
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

	MDRV_PALETTE_INIT(RRRR_GGGG_BBBB)
	MDRV_VIDEO_START(commando)
	MDRV_VIDEO_EOF(commando)
	MDRV_VIDEO_UPDATE(commando)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, ym2203_interface)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( commando )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 )	/* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "m09_cm04.bin", 0x0000, 0x8000, CRC(8438b694) SHA1(e154478d8f1b635355bd777370acabe49cb9d309) )
	ROM_LOAD( "m08_cm03.bin", 0x8000, 0x4000, CRC(35486542) SHA1(531a85c9e03970ce037be84f2240c2df6f6e3ec1) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "f09_cm02.bin", 0x0000, 0x4000, CRC(f9cc4a74) SHA1(ee8dd73919c6f47f62cc6d999de9510db9f79b8f) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "d05_vt01.bin", 0x00000, 0x4000, CRC(505726e0) SHA1(2435c87c9c9d78a6e703cf0e1f6a0288207fcd4c) )	/* characters */

	ROM_REGION( 0x18000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "a05_vt11.bin", 0x00000, 0x4000, CRC(7b2e1b48) SHA1(5d49e1d8146e4ef744445b68f35677302e875a85) )	/* tiles */
	ROM_LOAD( "a06_vt12.bin", 0x04000, 0x4000, CRC(81b417d3) SHA1(5ec7e3f0c8069384a5f6eb39232c228b9d7b8c0c) )
	ROM_LOAD( "a07_vt13.bin", 0x08000, 0x4000, CRC(5612dbd2) SHA1(9e4e1a22b6cbf60607b9a81dae34482ae55f7c47) )
	ROM_LOAD( "a08_vt14.bin", 0x0c000, 0x4000, CRC(2b2dee36) SHA1(8792278464fa3da47176582025f6673a15a581e2) )
	ROM_LOAD( "a09_vt15.bin", 0x10000, 0x4000, CRC(de70babf) SHA1(6717e23baf55f84d3143fb432140a7c3e102ac26) )
	ROM_LOAD( "a10_vt16.bin", 0x14000, 0x4000, CRC(14178237) SHA1(f896e71c7004349c9a46155edfd9f0aaa186065d) )

	ROM_REGION( 0x18000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "e07_vt05.bin", 0x00000, 0x4000, CRC(79f16e3d) SHA1(04e1f03a4d6b4cc2b81bce3a290bbb95de900d35) )	/* sprites */
	ROM_LOAD( "e08_vt06.bin", 0x04000, 0x4000, CRC(26fee521) SHA1(2fbfc73ee860f72a20229a01d4da9f5cc2e858d3) )
	ROM_LOAD( "e09_vt07.bin", 0x08000, 0x4000, CRC(ca88bdfd) SHA1(548b05460bc7983cc81f15c70e87f47d10db2812) )
	ROM_LOAD( "h07_vt08.bin", 0x0c000, 0x4000, CRC(2019c883) SHA1(883c0156ceab99f4849fe36972c4162b4ac8c216) )
	ROM_LOAD( "h08_vt09.bin", 0x10000, 0x4000, CRC(98703982) SHA1(ba9a9b0dcadd4f52502828408c4a19b0bd518351) )
	ROM_LOAD( "h09_vt10.bin", 0x14000, 0x4000, CRC(f069d2f8) SHA1(2c92300a9407470b34965021de882f1f7a84730c) )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "01d_vtb1.bin", 0x0000, 0x0100, CRC(3aba15a1) SHA1(8b057f6e26155dd9e48bde182e680fce4519f600) )	/* red */
	ROM_LOAD( "02d_vtb2.bin", 0x0100, 0x0100, CRC(88865754) SHA1(ca6dddca98baf00a65b2fb70b69cf4704ef8c831) )	/* green */
	ROM_LOAD( "03d_vtb3.bin", 0x0200, 0x0100, CRC(4c14c3f6) SHA1(644ac17c7413f094ec9a15cba87bbd421b26321f) )	/* blue */
	ROM_LOAD( "01h_vtb4.bin", 0x0300, 0x0100, CRC(b388c246) SHA1(038f9851699331ad887b6281a9df053dca3db8fd) )	/* palette selector (not used) */
	ROM_LOAD( "06l_vtb5.bin", 0x0400, 0x0100, CRC(712ac508) SHA1(5349d722ab6733afdda65f6e0a98322f0d515e86) )	/* interrupt timing (not used) */
	ROM_LOAD( "06e_vtb6.bin", 0x0500, 0x0100, CRC(0eaf5158) SHA1(bafd4108708f66cd7b280e47152b108f3e254fc9) )	/* video timing (not used) */
ROM_END

ROM_START( commandu )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 )	/* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "u4-f",         0x0000, 0x8000, CRC(a6118935) SHA1(d5811968b23d61e344e151747bcc3c0ed2b9497b) )
	ROM_LOAD( "u3-f",         0x8000, 0x4000, CRC(24f49684) SHA1(d38a7bd9f3b506747a03f6b94c3f8a2d9fc59166) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "f09_cm02.bin", 0x0000, 0x4000, CRC(f9cc4a74) SHA1(ee8dd73919c6f47f62cc6d999de9510db9f79b8f) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "d05_vt01.bin", 0x00000, 0x4000, CRC(505726e0) SHA1(2435c87c9c9d78a6e703cf0e1f6a0288207fcd4c) )	/* characters */

	ROM_REGION( 0x18000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "a05_vt11.bin", 0x00000, 0x4000, CRC(7b2e1b48) SHA1(5d49e1d8146e4ef744445b68f35677302e875a85) )	/* tiles */
	ROM_LOAD( "a06_vt12.bin", 0x04000, 0x4000, CRC(81b417d3) SHA1(5ec7e3f0c8069384a5f6eb39232c228b9d7b8c0c) )
	ROM_LOAD( "a07_vt13.bin", 0x08000, 0x4000, CRC(5612dbd2) SHA1(9e4e1a22b6cbf60607b9a81dae34482ae55f7c47) )
	ROM_LOAD( "a08_vt14.bin", 0x0c000, 0x4000, CRC(2b2dee36) SHA1(8792278464fa3da47176582025f6673a15a581e2) )
	ROM_LOAD( "a09_vt15.bin", 0x10000, 0x4000, CRC(de70babf) SHA1(6717e23baf55f84d3143fb432140a7c3e102ac26) )
	ROM_LOAD( "a10_vt16.bin", 0x14000, 0x4000, CRC(14178237) SHA1(f896e71c7004349c9a46155edfd9f0aaa186065d) )

	ROM_REGION( 0x18000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "e07_vt05.bin", 0x00000, 0x4000, CRC(79f16e3d) SHA1(04e1f03a4d6b4cc2b81bce3a290bbb95de900d35) )	/* sprites */
	ROM_LOAD( "e08_vt06.bin", 0x04000, 0x4000, CRC(26fee521) SHA1(2fbfc73ee860f72a20229a01d4da9f5cc2e858d3) )
	ROM_LOAD( "e09_vt07.bin", 0x08000, 0x4000, CRC(ca88bdfd) SHA1(548b05460bc7983cc81f15c70e87f47d10db2812) )
	ROM_LOAD( "h07_vt08.bin", 0x0c000, 0x4000, CRC(2019c883) SHA1(883c0156ceab99f4849fe36972c4162b4ac8c216) )
	ROM_LOAD( "h08_vt09.bin", 0x10000, 0x4000, CRC(98703982) SHA1(ba9a9b0dcadd4f52502828408c4a19b0bd518351) )
	ROM_LOAD( "h09_vt10.bin", 0x14000, 0x4000, CRC(f069d2f8) SHA1(2c92300a9407470b34965021de882f1f7a84730c) )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "01d_vtb1.bin", 0x0000, 0x0100, CRC(3aba15a1) SHA1(8b057f6e26155dd9e48bde182e680fce4519f600) )	/* red */
	ROM_LOAD( "02d_vtb2.bin", 0x0100, 0x0100, CRC(88865754) SHA1(ca6dddca98baf00a65b2fb70b69cf4704ef8c831) )	/* green */
	ROM_LOAD( "03d_vtb3.bin", 0x0200, 0x0100, CRC(4c14c3f6) SHA1(644ac17c7413f094ec9a15cba87bbd421b26321f) )	/* blue */
	ROM_LOAD( "01h_vtb4.bin", 0x0300, 0x0100, CRC(b388c246) SHA1(038f9851699331ad887b6281a9df053dca3db8fd) )	/* palette selector (not used) */
	ROM_LOAD( "06l_vtb5.bin", 0x0400, 0x0100, CRC(712ac508) SHA1(5349d722ab6733afdda65f6e0a98322f0d515e86) )	/* interrupt timing (not used) */
	ROM_LOAD( "06e_vtb6.bin", 0x0500, 0x0100, CRC(0eaf5158) SHA1(bafd4108708f66cd7b280e47152b108f3e254fc9) )	/* video timing (not used) */
ROM_END

ROM_START( commandj )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 )	/* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "09m_so04.bin", 0x0000, 0x8000, CRC(d3f2bfb3) SHA1(738a5673ac6a907cb04cfb125e8aab3f7437b9d2) )
	ROM_LOAD( "08m_so03.bin", 0x8000, 0x4000, CRC(ed01f472) SHA1(fa181293ae8f0fee78d412259eb81f6de1e1307a) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "09f_so02.bin", 0x0000, 0x4000, CRC(ca20aca5) SHA1(206a8fd4a8985e7ceed7de8349ba02627e881503) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "d05_vt01.bin", 0x00000, 0x4000, CRC(505726e0) SHA1(2435c87c9c9d78a6e703cf0e1f6a0288207fcd4c) )	/* characters */

	ROM_REGION( 0x18000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "a05_vt11.bin", 0x00000, 0x4000, CRC(7b2e1b48) SHA1(5d49e1d8146e4ef744445b68f35677302e875a85) )	/* tiles */
	ROM_LOAD( "a06_vt12.bin", 0x04000, 0x4000, CRC(81b417d3) SHA1(5ec7e3f0c8069384a5f6eb39232c228b9d7b8c0c) )
	ROM_LOAD( "a07_vt13.bin", 0x08000, 0x4000, CRC(5612dbd2) SHA1(9e4e1a22b6cbf60607b9a81dae34482ae55f7c47) )
	ROM_LOAD( "a08_vt14.bin", 0x0c000, 0x4000, CRC(2b2dee36) SHA1(8792278464fa3da47176582025f6673a15a581e2) )
	ROM_LOAD( "a09_vt15.bin", 0x10000, 0x4000, CRC(de70babf) SHA1(6717e23baf55f84d3143fb432140a7c3e102ac26) )
	ROM_LOAD( "a10_vt16.bin", 0x14000, 0x4000, CRC(14178237) SHA1(f896e71c7004349c9a46155edfd9f0aaa186065d) )

	ROM_REGION( 0x18000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "e07_vt05.bin", 0x00000, 0x4000, CRC(79f16e3d) SHA1(04e1f03a4d6b4cc2b81bce3a290bbb95de900d35) )	/* sprites */
	ROM_LOAD( "e08_vt06.bin", 0x04000, 0x4000, CRC(26fee521) SHA1(2fbfc73ee860f72a20229a01d4da9f5cc2e858d3) )
	ROM_LOAD( "e09_vt07.bin", 0x08000, 0x4000, CRC(ca88bdfd) SHA1(548b05460bc7983cc81f15c70e87f47d10db2812) )
	ROM_LOAD( "h07_vt08.bin", 0x0c000, 0x4000, CRC(2019c883) SHA1(883c0156ceab99f4849fe36972c4162b4ac8c216) )
	ROM_LOAD( "h08_vt09.bin", 0x10000, 0x4000, CRC(98703982) SHA1(ba9a9b0dcadd4f52502828408c4a19b0bd518351) )
	ROM_LOAD( "h09_vt10.bin", 0x14000, 0x4000, CRC(f069d2f8) SHA1(2c92300a9407470b34965021de882f1f7a84730c) )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "01d_vtb1.bin", 0x0000, 0x0100, CRC(3aba15a1) SHA1(8b057f6e26155dd9e48bde182e680fce4519f600) )	/* red */
	ROM_LOAD( "02d_vtb2.bin", 0x0100, 0x0100, CRC(88865754) SHA1(ca6dddca98baf00a65b2fb70b69cf4704ef8c831) )	/* green */
	ROM_LOAD( "03d_vtb3.bin", 0x0200, 0x0100, CRC(4c14c3f6) SHA1(644ac17c7413f094ec9a15cba87bbd421b26321f) )	/* blue */
	ROM_LOAD( "01h_vtb4.bin", 0x0300, 0x0100, CRC(b388c246) SHA1(038f9851699331ad887b6281a9df053dca3db8fd) )	/* palette selector (not used) */
	ROM_LOAD( "06l_vtb5.bin", 0x0400, 0x0100, CRC(712ac508) SHA1(5349d722ab6733afdda65f6e0a98322f0d515e86) )	/* interrupt timing (not used) */
	ROM_LOAD( "06e_vtb6.bin", 0x0500, 0x0100, CRC(0eaf5158) SHA1(bafd4108708f66cd7b280e47152b108f3e254fc9) )	/* video timing (not used) */
ROM_END

ROM_START( sinvasn )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 )	/* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "sp04",           0x0000, 0x8000, CRC(33f9601e) SHA1(71182227b77fccbbc1d89b5828aa86dcc64ca05e) )
	ROM_LOAD( "sp03",           0x8000, 0x4000, CRC(c7fb43b3) SHA1(36d0dffdacc36a6b6a77101d942c0821846f3275) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "u2",           0x0000, 0x4000, CRC(cbf8c40e) SHA1(0c8dce034d96d075e012cbb8f68c2817b860d969) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "u1",           0x00000, 0x4000, CRC(f477e13a) SHA1(ec5b80f5d508501e72cba028dc45b2c307ac452b) )	/* characters */

	ROM_REGION( 0x18000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "a05_vt11.bin", 0x00000, 0x4000, CRC(7b2e1b48) SHA1(5d49e1d8146e4ef744445b68f35677302e875a85) )	/* tiles */
	ROM_LOAD( "a06_vt12.bin", 0x04000, 0x4000, CRC(81b417d3) SHA1(5ec7e3f0c8069384a5f6eb39232c228b9d7b8c0c) )
	ROM_LOAD( "a07_vt13.bin", 0x08000, 0x4000, CRC(5612dbd2) SHA1(9e4e1a22b6cbf60607b9a81dae34482ae55f7c47) )
	ROM_LOAD( "a08_vt14.bin", 0x0c000, 0x4000, CRC(2b2dee36) SHA1(8792278464fa3da47176582025f6673a15a581e2) )
	ROM_LOAD( "a09_vt15.bin", 0x10000, 0x4000, CRC(de70babf) SHA1(6717e23baf55f84d3143fb432140a7c3e102ac26) )
	ROM_LOAD( "a10_vt16.bin", 0x14000, 0x4000, CRC(14178237) SHA1(f896e71c7004349c9a46155edfd9f0aaa186065d) )

	ROM_REGION( 0x18000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "u5",           0x00000, 0x4000, CRC(2a97c933) SHA1(bfddb0c0f930a7caffad7e52d394d72c09ffb45f) )	/* sprites */
	ROM_LOAD( "sp06",         0x04000, 0x4000, CRC(d7887212) SHA1(43ad98263d6314d40abf33087127c23a3ad72335) )
	ROM_LOAD( "sp07",         0x08000, 0x4000, CRC(9abe7a20) SHA1(5f1b851bd66a3ab818b893286d3ebf2194f425c4) )
	ROM_LOAD( "u8",           0x0c000, 0x4000, CRC(d6b4aa2e) SHA1(5bbf536f73010182b9150dd4fb1e2a42b5b380b0) )
	ROM_LOAD( "sp09",         0x10000, 0x4000, CRC(3985b318) SHA1(ac4c67c3af42121869c1b9470377404bc88793c2) )
	ROM_LOAD( "sp10",         0x14000, 0x4000, CRC(3c131b0f) SHA1(dd3e63199120502c03eedd024a2eed3b5d3e2a1c) )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "01d_vtb1.bin", 0x0000, 0x0100, CRC(3aba15a1) SHA1(8b057f6e26155dd9e48bde182e680fce4519f600) )	/* red */
	ROM_LOAD( "02d_vtb2.bin", 0x0100, 0x0100, CRC(88865754) SHA1(ca6dddca98baf00a65b2fb70b69cf4704ef8c831) )	/* green */
	ROM_LOAD( "03d_vtb3.bin", 0x0200, 0x0100, CRC(4c14c3f6) SHA1(644ac17c7413f094ec9a15cba87bbd421b26321f) )	/* blue */
	ROM_LOAD( "01h_vtb4.bin", 0x0300, 0x0100, CRC(b388c246) SHA1(038f9851699331ad887b6281a9df053dca3db8fd) )	/* palette selector (not used) */
	ROM_LOAD( "06l_vtb5.bin", 0x0400, 0x0100, CRC(712ac508) SHA1(5349d722ab6733afdda65f6e0a98322f0d515e86) )	/* interrupt timing (not used) */
	ROM_LOAD( "06e_vtb6.bin", 0x0500, 0x0100, CRC(0eaf5158) SHA1(bafd4108708f66cd7b280e47152b108f3e254fc9) )	/* video timing (not used) */
ROM_END

ROM_START( sinvasnb )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 )	/* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "u4",           0x0000, 0x8000, CRC(834ba0de) SHA1(85f40559e6a436f3f752b6e862a419a5b9481fa8) )
	ROM_LOAD( "u3",           0x8000, 0x4000, CRC(07e4ee3a) SHA1(6d7665b3072f075893ef37e55147b10271d069ef) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "u2",           0x0000, 0x4000, CRC(cbf8c40e) SHA1(0c8dce034d96d075e012cbb8f68c2817b860d969) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "u1",           0x00000, 0x4000, CRC(f477e13a) SHA1(ec5b80f5d508501e72cba028dc45b2c307ac452b) )	/* characters */

	ROM_REGION( 0x18000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "a05_vt11.bin", 0x00000, 0x4000, CRC(7b2e1b48) SHA1(5d49e1d8146e4ef744445b68f35677302e875a85) )	/* tiles */
	ROM_LOAD( "a06_vt12.bin", 0x04000, 0x4000, CRC(81b417d3) SHA1(5ec7e3f0c8069384a5f6eb39232c228b9d7b8c0c) )
	ROM_LOAD( "a07_vt13.bin", 0x08000, 0x4000, CRC(5612dbd2) SHA1(9e4e1a22b6cbf60607b9a81dae34482ae55f7c47) )
	ROM_LOAD( "a08_vt14.bin", 0x0c000, 0x4000, CRC(2b2dee36) SHA1(8792278464fa3da47176582025f6673a15a581e2) )
	ROM_LOAD( "a09_vt15.bin", 0x10000, 0x4000, CRC(de70babf) SHA1(6717e23baf55f84d3143fb432140a7c3e102ac26) )
	ROM_LOAD( "a10_vt16.bin", 0x14000, 0x4000, CRC(14178237) SHA1(f896e71c7004349c9a46155edfd9f0aaa186065d) )

	ROM_REGION( 0x18000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "u5",           0x00000, 0x4000, CRC(2a97c933) SHA1(bfddb0c0f930a7caffad7e52d394d72c09ffb45f) )	/* sprites */
	ROM_LOAD( "e08_vt06.bin", 0x04000, 0x4000, CRC(26fee521) SHA1(2fbfc73ee860f72a20229a01d4da9f5cc2e858d3) )
	ROM_LOAD( "e09_vt07.bin", 0x08000, 0x4000, CRC(ca88bdfd) SHA1(548b05460bc7983cc81f15c70e87f47d10db2812) )
	ROM_LOAD( "u8",           0x0c000, 0x4000, CRC(d6b4aa2e) SHA1(5bbf536f73010182b9150dd4fb1e2a42b5b380b0) )
	ROM_LOAD( "h08_vt09.bin", 0x10000, 0x4000, CRC(98703982) SHA1(ba9a9b0dcadd4f52502828408c4a19b0bd518351) )
	ROM_LOAD( "h09_vt10.bin", 0x14000, 0x4000, CRC(f069d2f8) SHA1(2c92300a9407470b34965021de882f1f7a84730c) )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "01d_vtb1.bin", 0x0000, 0x0100, CRC(3aba15a1) SHA1(8b057f6e26155dd9e48bde182e680fce4519f600) )	/* red */
	ROM_LOAD( "02d_vtb2.bin", 0x0100, 0x0100, CRC(88865754) SHA1(ca6dddca98baf00a65b2fb70b69cf4704ef8c831) )	/* green */
	ROM_LOAD( "03d_vtb3.bin", 0x0200, 0x0100, CRC(4c14c3f6) SHA1(644ac17c7413f094ec9a15cba87bbd421b26321f) )	/* blue */
	ROM_LOAD( "01h_vtb4.bin", 0x0300, 0x0100, CRC(b388c246) SHA1(038f9851699331ad887b6281a9df053dca3db8fd) )	/* palette selector (not used) */
	ROM_LOAD( "06l_vtb5.bin", 0x0400, 0x0100, CRC(712ac508) SHA1(5349d722ab6733afdda65f6e0a98322f0d515e86) )	/* interrupt timing (not used) */
	ROM_LOAD( "06e_vtb6.bin", 0x0500, 0x0100, CRC(0eaf5158) SHA1(bafd4108708f66cd7b280e47152b108f3e254fc9) )	/* video timing (not used) */
ROM_END


static DRIVER_INIT( commando )
{
	int A;
	unsigned char *rom = memory_region(REGION_CPU1);
	int diff = memory_region_length(REGION_CPU1) / 2;


	memory_set_opcode_base(0,rom+diff);

	/* the first opcode is *not* encrypted */
	rom[0+diff] = rom[0];
	for (A = 1;A < 0xc000;A++)
	{
		int src;

		src = rom[A];
		rom[A+diff] = (src & 0x11) | ((src & 0xe0) >> 4) | ((src & 0x0e) << 4);
	}
}

static DRIVER_INIT( spaceinv )
{
	int A;
	unsigned char *rom = memory_region(REGION_CPU1);
	int diff = memory_region_length(REGION_CPU1) / 2;


	memory_set_opcode_base(0,rom+diff);

	/* the first opcode *is* encrypted */
	for (A = 0;A < 0xc000;A++)
	{
		int src;

		src = rom[A];
		rom[A+diff] = (src & 0x11) | ((src & 0xe0) >> 4) | ((src & 0x0e) << 4);
	}
}



GAME( 1985, commando, 0,        commando, commando, commando, ROT270, "Capcom", "Commando (World)" )
GAME( 1985, commandu, commando, commando, commandu, commando, ROT270, "Capcom (Data East USA license)", "Commando (US)" )
GAME( 1985, commandj, commando, commando, commando, commando, ROT270, "Capcom", "Senjou no Ookami" )
GAME( 1985, sinvasn,  commando, commando, commando, commando, ROT270, "Capcom", "Space Invasion (Europe)" )
GAME( 1985, sinvasnb, commando, commando, commando, spaceinv, ROT270, "bootleg", "Space Invasion (bootleg)" )
