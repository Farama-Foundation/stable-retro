/***************************************************************************

"AJAX/Typhoon"	(Konami GX770)

Driver by:
	Manuel Abadia <manu@teleline.es>

TO DO:
- Find the CPU core bug, that makes the 052001 to read from 0x0000

***************************************************************************/

#include "driver.h"
#include "vidhrdw/konamiic.h"

extern unsigned char *ajax_sharedram;

static WRITE_HANDLER( k007232_extvol_w );
static WRITE_HANDLER( sound_bank_w );

/* from machine/ajax.c */
WRITE_HANDLER( ajax_bankswitch_2_w );
READ_HANDLER( ajax_sharedram_r );
WRITE_HANDLER( ajax_sharedram_w );
READ_HANDLER( ajax_ls138_f10_r );
WRITE_HANDLER( ajax_ls138_f10_w );
MACHINE_INIT( ajax );
INTERRUPT_GEN( ajax_interrupt );

/* from vidhrdw/ajax.c */
VIDEO_START( ajax );
VIDEO_UPDATE( ajax );

/****************************************************************************/

static MEMORY_READ_START( ajax_readmem )
	{ 0x0000, 0x01c0, ajax_ls138_f10_r },			/* inputs + DIPSW */
	{ 0x0800, 0x0807, K051937_r },					/* sprite control registers */
	{ 0x0c00, 0x0fff, K051960_r },					/* sprite RAM 2128SL at J7 */
	{ 0x1000, 0x1fff, MRA_RAM },		/* palette */
	{ 0x2000, 0x3fff, ajax_sharedram_r },			/* shared RAM with the 6809 */
	{ 0x4000, 0x5fff, MRA_RAM },					/* RAM 6264L at K10*/
	{ 0x6000, 0x7fff, MRA_BANK2 },					/* banked ROM */
	{ 0x8000, 0xffff, MRA_ROM },					/* ROM N11 */
MEMORY_END

static MEMORY_WRITE_START( ajax_writemem )
	{ 0x0000, 0x01c0, ajax_ls138_f10_w },			/* bankswitch + sound command + FIRQ command */
	{ 0x0800, 0x0807, K051937_w },					/* sprite control registers */
	{ 0x0c00, 0x0fff, K051960_w },					/* sprite RAM 2128SL at J7 */
	{ 0x1000, 0x1fff, paletteram_xBBBBBGGGGGRRRRR_swap_w, &paletteram },/* palette */
	{ 0x2000, 0x3fff, ajax_sharedram_w },			/* shared RAM with the 6809 */
	{ 0x4000, 0x5fff, MWA_RAM },					/* RAM 6264L at K10 */
	{ 0x6000, 0x7fff, MWA_ROM },					/* banked ROM */
	{ 0x8000, 0xffff, MWA_ROM },					/* ROM N11 */
MEMORY_END

static MEMORY_READ_START( ajax_readmem_2 )
	{ 0x0000, 0x07ff, K051316_0_r },		/* 051316 zoom/rotation layer */
	{ 0x1000, 0x17ff, K051316_rom_0_r },	/* 051316 (ROM test) */
	{ 0x2000, 0x3fff, ajax_sharedram_r },	/* shared RAM with the 052001 */
	{ 0x4000, 0x7fff, K052109_r },			/* video RAM + color RAM + video registers */
	{ 0x8000, 0x9fff, MRA_BANK1 },			/* banked ROM */
	{ 0xa000, 0xffff, MRA_ROM },			/* ROM I16 */
MEMORY_END

static MEMORY_WRITE_START( ajax_writemem_2 )
	{ 0x0000, 0x07ff, K051316_0_w },			/* 051316 zoom/rotation layer */
	{ 0x0800, 0x080f, K051316_ctrl_0_w },		/* 051316 control registers */
	{ 0x1800, 0x1800, ajax_bankswitch_2_w },	/* bankswitch control */
	{ 0x2000, 0x3fff, ajax_sharedram_w, &ajax_sharedram },/* shared RAM with the 052001 */
	{ 0x4000, 0x7fff, K052109_w },				/* video RAM + color RAM + video registers */
	{ 0x8000, 0x9fff, MWA_ROM },				/* banked ROM */
	{ 0xa000, 0xffff, MWA_ROM },				/* ROM I16 */
MEMORY_END

static MEMORY_READ_START( ajax_readmem_sound )
	{ 0x0000, 0x7fff, MRA_ROM },				/* ROM F6 */
	{ 0x8000, 0x87ff, MRA_RAM },				/* RAM 2128SL at D16 */
	{ 0xa000, 0xa00d, K007232_read_port_0_r },	/* 007232 registers (chip 1) */
	{ 0xb000, 0xb00d, K007232_read_port_1_r },	/* 007232 registers (chip 2) */
	{ 0xc001, 0xc001, YM2151_status_port_0_r },	/* YM2151 */
	{ 0xe000, 0xe000, soundlatch_r },			/* soundlatch_r */
MEMORY_END

static MEMORY_WRITE_START( ajax_writemem_sound )
	{ 0x0000, 0x7fff, MWA_ROM },					/* ROM F6 */
	{ 0x8000, 0x87ff, MWA_RAM },					/* RAM 2128SL at D16 */
	{ 0x9000, 0x9000, sound_bank_w },				/* 007232 bankswitch */
	{ 0xa000, 0xa00d, K007232_write_port_0_w },		/* 007232 registers (chip 1) */
	{ 0xb000, 0xb00d, K007232_write_port_1_w },		/* 007232 registers (chip 2) */
	{ 0xb80c, 0xb80c, k007232_extvol_w },	/* extra volume, goes to the 007232 w/ A11 */
											/* selecting a different latch for the external port */
	{ 0xc000, 0xc000, YM2151_register_port_0_w },	/* YM2151 */
	{ 0xc001, 0xc001, YM2151_data_port_0_w },		/* YM2151 */
MEMORY_END


INPUT_PORTS_START( ajax )
	PORT_START	/* DSW #1 */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_7C ) )
/*	PORT_DIPSETTING(    0x00, "Coin Slot 2 Invalidity" )*/

	PORT_START	/* DSW #2 */
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x03, "2" )
	PORT_DIPSETTING(	0x02, "3" )
	PORT_DIPSETTING(	0x01, "5" )
	PORT_DIPSETTING(	0x00, "7" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x18, 0x10, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x18, "30000 150000" )
	PORT_DIPSETTING(	0x10, "50000 200000" )
	PORT_DIPSETTING(	0x08, "30000" )
	PORT_DIPSETTING(	0x00, "50000" )
	PORT_DIPNAME( 0x60, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x60, "Easy" )
	PORT_DIPSETTING(	0x40, "Normal" )
	PORT_DIPSETTING(	0x20, "Difficult" )
	PORT_DIPSETTING(	0x00, "Very difficult" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START	/* DSW #3 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Upright Controls" )
	PORT_DIPSETTING(    0x02, "Single" )
	PORT_DIPSETTING(    0x00, "Dual" )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, "Control in 3D Stages" )
	PORT_DIPSETTING(	0x08, "Normal" )
	PORT_DIPSETTING(	0x00, "Inverted" )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* COINSW & START */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )	/* service */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* PLAYER 1 INPUTS */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* PLAYER 2 INPUTS */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END



static struct YM2151interface ym2151_interface =
{
	1,
	3579545,	/* 3.58 MHz */
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) },
	{ 0 },
};

/*	sound_bank_w:
	Handled by the LS273 Octal +ve edge trigger D-type Flip-flop with Reset at B11:

	Bit	Description
	---	-----------
	7	CONT1 (???) \
	6	CONT2 (???) / One or both bits are set to 1 when you kill a enemy
	5	\
	3	/ 4MBANKH
	4	\
	2	/ 4MBANKL
	1	\
	0	/ 2MBANK
*/

static WRITE_HANDLER( sound_bank_w )
{
	int bank_A, bank_B;

	/* banks # for the 007232 (chip 1) */
	bank_A = ((data >> 1) & 0x01);
	bank_B = ((data >> 0) & 0x01);
	K007232_set_bank( 0, bank_A, bank_B );

	/* banks # for the 007232 (chip 2) */
	bank_A = ((data >> 4) & 0x03);
	bank_B = ((data >> 2) & 0x03);
	K007232_set_bank( 1, bank_A, bank_B );
}

static void volume_callback0(int v)
{
	K007232_set_volume(0,0,(v >> 4) * 0x11,0);
	K007232_set_volume(0,1,0,(v & 0x0f) * 0x11);
}

static WRITE_HANDLER( k007232_extvol_w )
{
	/* channel A volume (mono) */
	K007232_set_volume(1,0,(data & 0x0f) * 0x11/2,(data & 0x0f) * 0x11/2);
}

static void volume_callback1(int v)
{
	/* channel B volume/pan */
	K007232_set_volume(1,1,(v & 0x0f) * 0x11/2,(v >> 4) * 0x11/2);
}

static struct K007232_interface k007232_interface =
{
	2,			/* number of chips */
	3579545,	/* clock */
	{ REGION_SOUND1, REGION_SOUND2 },	/* memory regions */
	{ K007232_VOL(20,MIXER_PAN_CENTER,20,MIXER_PAN_CENTER),
		K007232_VOL(50,MIXER_PAN_LEFT,50,MIXER_PAN_RIGHT) },/* volume */
	{ volume_callback0,  volume_callback1 }	/* external port callback */
};



static MACHINE_DRIVER_START( ajax )

	/* basic machine hardware */
	MDRV_CPU_ADD(KONAMI, 3000000)	/* 12/4 MHz*/
	MDRV_CPU_MEMORY(ajax_readmem,ajax_writemem)
	MDRV_CPU_VBLANK_INT(ajax_interrupt,1)	/* IRQs triggered by the 051960 */

	MDRV_CPU_ADD(M6809, 3000000)	/* ? */
	MDRV_CPU_MEMORY(ajax_readmem_2,ajax_writemem_2)

	MDRV_CPU_ADD(Z80, 3579545)	/* 3.58 MHz */
	MDRV_CPU_MEMORY(ajax_readmem_sound,ajax_writemem_sound)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)

	MDRV_MACHINE_INIT(ajax)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_HAS_SHADOWS)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(14*8, (64-14)*8-1, 2*8, 30*8-1 )
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(ajax)
	MDRV_VIDEO_UPDATE(ajax)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(K007232, k007232_interface)
MACHINE_DRIVER_END



ROM_START( ajax )
	ROM_REGION( 0x28000, REGION_CPU1, 0 )	/* 052001 code */
	ROM_LOAD( "770_m01.n11",	0x10000, 0x08000, CRC(4a64e53a) SHA1(acd249bfcb5f248c41b3e40c7c1bce1b8c645d3a) )	/* banked ROM */
	ROM_CONTINUE(				0x08000, 0x08000 )				/* fixed ROM */
	ROM_LOAD( "770_l02.n12",	0x18000, 0x10000, CRC(ad7d592b) SHA1(c75d9696b16de231c479379dd02d33fe54021d88) )	/* banked ROM */

	ROM_REGION( 0x22000, REGION_CPU2, 0 )	/* 64k + 72k for banked ROMs */
	ROM_LOAD( "770_l05.i16",	0x20000, 0x02000, CRC(ed64fbb2) SHA1(429046edaf1299afa7fb9c385b4ef0c244ec2409) )	/* banked ROM */
	ROM_CONTINUE(				0x0a000, 0x06000 )				/* fixed ROM */
	ROM_LOAD( "770_f04.g16",	0x10000, 0x10000, CRC(e0e4ec9c) SHA1(15ae09c3ad67ec626d8178ec1417f0c57ca4eca4) )	/* banked ROM */

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* 64k for the SOUND CPU */
	ROM_LOAD( "770_h03.f16",	0x00000, 0x08000, CRC(2ffd2afc) SHA1(ca2ef684f87bcf9b70b3ec66ec80685edaf04b9b) )

    ROM_REGION( 0x080000, REGION_GFX1, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "770c13",		0x000000, 0x040000, CRC(b859ca4e) SHA1(f58678d503683f78cca0d5ed2d79f6f68ab3495a) )	/* characters (N22) */
	ROM_LOAD( "770c12",		0x040000, 0x040000, CRC(50d14b72) SHA1(e3ff4a5aeefa6c10b5f7fec18297948b7c5acfdf) )	/* characters (K22) */

    ROM_REGION( 0x100000, REGION_GFX2, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "770c09",		0x000000, 0x080000, CRC(1ab4a7ff) SHA1(fa007b41027f95d29d2a9f931a2fe235844db637) )	/* sprites (N4) */
	ROM_LOAD( "770c08",		0x080000, 0x080000, CRC(a8e80586) SHA1(0401f59baa691905287cef94427f39e0c3f0adc6) )	/* sprites (K4) */

	ROM_REGION( 0x080000, REGION_GFX3, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "770c06",		0x000000, 0x040000, CRC(d0c592ee) SHA1(c1be73dd259f2779d715659b177e47513776a0d4) )	/* zoom/rotate (F4) */
	ROM_LOAD( "770c07",		0x040000, 0x040000, CRC(0b399fb1) SHA1(fbe26f9aa9a655d08bebcdd79719d35134ca4dd5) )	/* zoom/rotate (H4) */

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "63s241.j11",	0x0000, 0x0200, CRC(9bdd719f) SHA1(de98e562080a97714047a8ad17abc6662c188897) )	/* priority encoder (not used) */

	ROM_REGION( 0x040000, REGION_SOUND1, 0 )	/* 007232 data (chip 1) */
	ROM_LOAD( "770c10",		0x000000, 0x040000, CRC(7fac825f) SHA1(581522d7a02dad16d2803ff344b4db133f767e6b) )

	ROM_REGION( 0x080000, REGION_SOUND2, 0 )	/* 007232 data (chip 2) */
	ROM_LOAD( "770c11",		0x000000, 0x080000, CRC(299a615a) SHA1(29cdcc21998c72f4cf311792b904b79bde236bab) )
ROM_END

ROM_START( typhoon )
	ROM_REGION( 0x28000, REGION_CPU1, 0 )	/* 052001 code */
	ROM_LOAD( "770_k01.n11",	0x10000, 0x08000, CRC(5ba74a22) SHA1(897d3309f2efb3bfa56e86581ee4a492e656788c) )	/* banked ROM */
	ROM_CONTINUE(				0x08000, 0x08000 )				/* fixed ROM */
	ROM_LOAD( "770_k02.n12",	0x18000, 0x10000, CRC(3bcf782a) SHA1(4b6127bced0b2519f8ad30587f32588a16368071) )	/* banked ROM */

	ROM_REGION( 0x22000, REGION_CPU2, 0 )	/* 64k + 72k for banked ROMs */
	ROM_LOAD( "770_k05.i16",	0x20000, 0x02000, CRC(0f1bebbb) SHA1(012a8867ee0febaaadd7bcbc91e462bda5d3a411) )	/* banked ROM */
	ROM_CONTINUE(				0x0a000, 0x06000 )				/* fixed ROM */
	ROM_LOAD( "770_f04.g16",	0x10000, 0x10000, CRC(e0e4ec9c) SHA1(15ae09c3ad67ec626d8178ec1417f0c57ca4eca4) )	/* banked ROM */

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* 64k for the SOUND CPU */
	ROM_LOAD( "770_h03.f16",	0x00000, 0x08000, CRC(2ffd2afc) SHA1(ca2ef684f87bcf9b70b3ec66ec80685edaf04b9b) )

    ROM_REGION( 0x080000, REGION_GFX1, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "770c13",		0x000000, 0x040000, CRC(b859ca4e) SHA1(f58678d503683f78cca0d5ed2d79f6f68ab3495a) )	/* characters (N22) */
	ROM_LOAD( "770c12",		0x040000, 0x040000, CRC(50d14b72) SHA1(e3ff4a5aeefa6c10b5f7fec18297948b7c5acfdf) )	/* characters (K22) */

    ROM_REGION( 0x100000, REGION_GFX2, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "770c09",		0x000000, 0x080000, CRC(1ab4a7ff) SHA1(fa007b41027f95d29d2a9f931a2fe235844db637) )	/* sprites (N4) */
	ROM_LOAD( "770c08",		0x080000, 0x080000, CRC(a8e80586) SHA1(0401f59baa691905287cef94427f39e0c3f0adc6) )	/* sprites (K4) */

	ROM_REGION( 0x080000, REGION_GFX3, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "770c06",		0x000000, 0x040000, CRC(d0c592ee) SHA1(c1be73dd259f2779d715659b177e47513776a0d4) )	/* zoom/rotate (F4) */
	ROM_LOAD( "770c07",		0x040000, 0x040000, CRC(0b399fb1) SHA1(fbe26f9aa9a655d08bebcdd79719d35134ca4dd5) )	/* zoom/rotate (H4) */

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "63s241.j11",	0x0000, 0x0200, CRC(9bdd719f) SHA1(de98e562080a97714047a8ad17abc6662c188897) )	/* priority encoder (not used) */

	ROM_REGION( 0x040000, REGION_SOUND1, 0 )	/* 007232 data (chip 1) */
	ROM_LOAD( "770c10",		0x000000, 0x040000, CRC(7fac825f) SHA1(581522d7a02dad16d2803ff344b4db133f767e6b) )

	ROM_REGION( 0x080000, REGION_SOUND2, 0 )	/* 007232 data (chip 2) */
	ROM_LOAD( "770c11",		0x000000, 0x080000, CRC(299a615a) SHA1(29cdcc21998c72f4cf311792b904b79bde236bab) )
ROM_END

ROM_START( ajaxj )
	ROM_REGION( 0x28000, REGION_CPU1, 0 )	/* 052001 code */
	ROM_LOAD( "770_l01.n11",	0x10000, 0x08000, CRC(7cea5274) SHA1(8e3b2b11a8189e3a1703b3b4b453fbb386f5537f) )	/* banked ROM */
	ROM_CONTINUE(				0x08000, 0x08000 )				/* fixed ROM */
	ROM_LOAD( "770_l02.n12",	0x18000, 0x10000, CRC(ad7d592b) SHA1(c75d9696b16de231c479379dd02d33fe54021d88) )	/* banked ROM */

	ROM_REGION( 0x22000, REGION_CPU2, 0 )	/* 64k + 72k for banked ROMs */
	ROM_LOAD( "770_l05.i16",	0x20000, 0x02000, CRC(ed64fbb2) SHA1(429046edaf1299afa7fb9c385b4ef0c244ec2409) )	/* banked ROM */
	ROM_CONTINUE(				0x0a000, 0x06000 )				/* fixed ROM */
	ROM_LOAD( "770_f04.g16",	0x10000, 0x10000, CRC(e0e4ec9c) SHA1(15ae09c3ad67ec626d8178ec1417f0c57ca4eca4) )	/* banked ROM */

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* 64k for the SOUND CPU */
	ROM_LOAD( "770_f03.f16",	0x00000, 0x08000, CRC(3fe914fd) SHA1(c691920402bd859e2bf765084704a8bfad302cfa) )

    ROM_REGION( 0x080000, REGION_GFX1, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "770c13",		0x000000, 0x040000, CRC(b859ca4e) SHA1(f58678d503683f78cca0d5ed2d79f6f68ab3495a) )	/* characters (N22) */
	ROM_LOAD( "770c12",		0x040000, 0x040000, CRC(50d14b72) SHA1(e3ff4a5aeefa6c10b5f7fec18297948b7c5acfdf) )	/* characters (K22) */

    ROM_REGION( 0x100000, REGION_GFX2, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "770c09",		0x000000, 0x080000, CRC(1ab4a7ff) SHA1(fa007b41027f95d29d2a9f931a2fe235844db637) )	/* sprites (N4) */
	ROM_LOAD( "770c08",		0x080000, 0x080000, CRC(a8e80586) SHA1(0401f59baa691905287cef94427f39e0c3f0adc6) )	/* sprites (K4) */

	ROM_REGION( 0x080000, REGION_GFX3, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "770c06",		0x000000, 0x040000, CRC(d0c592ee) SHA1(c1be73dd259f2779d715659b177e47513776a0d4) )	/* zoom/rotate (F4) */
	ROM_LOAD( "770c07",		0x040000, 0x040000, CRC(0b399fb1) SHA1(fbe26f9aa9a655d08bebcdd79719d35134ca4dd5) )	/* zoom/rotate (H4) */

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "63s241.j11",	0x0000, 0x0200, CRC(9bdd719f) SHA1(de98e562080a97714047a8ad17abc6662c188897) )	/* priority encoder (not used) */

	ROM_REGION( 0x040000, REGION_SOUND1, 0 )	/* 007232 data (chip 1) */
	ROM_LOAD( "770c10",		0x000000, 0x040000, CRC(7fac825f) SHA1(581522d7a02dad16d2803ff344b4db133f767e6b) )

	ROM_REGION( 0x080000, REGION_SOUND2, 0 )	/* 007232 data (chip 2) */
	ROM_LOAD( "770c11",		0x000000, 0x080000, CRC(299a615a) SHA1(29cdcc21998c72f4cf311792b904b79bde236bab) )
ROM_END


static DRIVER_INIT( ajax )
{
	konami_rom_deinterleave_2(REGION_GFX1);
	konami_rom_deinterleave_2(REGION_GFX2);
}



GAME( 1987, ajax,    0,    ajax, ajax, ajax, ROT90, "Konami", "Ajax" )
GAME( 1987, typhoon, ajax, ajax, ajax, ajax, ROT90, "Konami", "Typhoon" )
GAME( 1987, ajaxj,   ajax, ajax, ajax, ajax, ROT90, "Konami", "Ajax (Japan)" )
