/*
    Diet Go Go
    Driver by Bryan McPhail and David Haywood.
	
Hold both START buttons on bootup to display version notice.
Diet Go Go (Japan)   DATA EAST
NAME    LOCATION   TYPE
-----------------------
JW-02    14M       27C512
JW-01-2  5H        27C2001
JW-00-2  4H         "
PAL16L8B 7H
PAL16L8B 6H
PAL16R6A 11H
*/
#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "cpu/h6280/h6280.h"
#include "decocrpt.h"
#include "decoprot.h"
#include "deco16ic.h"


static int flip_screen;
static data16_t *spriteram16;
static unsigned int spriteram_size;


static void dietgogo_drawsprites(struct mame_bitmap *bitmap,const struct rectangle *cliprect)
{
	int offs;

	for (offs = 0;offs < 0x400;offs += 4)
	{
		int x,y,sprite,colour,multi,fx,fy,inc,flash,mult;

		sprite = spriteram16[offs+1];
		if (!sprite) continue;

		y = spriteram16[offs];
		flash=y&0x1000;
		if (flash && (cpu_getcurrentframe() & 1)) continue;

		x = spriteram16[offs+2];
		colour = (x >>9) & 0x1f;

		fx = y & 0x2000;
		fy = y & 0x4000;
		multi = (1 << ((y & 0x0600) >> 9)) - 1;	/* 1x, 2x, 4x, 8x height */

		x = x & 0x01ff;
		y = y & 0x01ff;
		if (x >= 320) x -= 512;
		if (y >= 256) y -= 512;
		y = 240 - y;
		x = 304 - x;

		if (x>320) continue;

		sprite &= ~multi;
		if (fy)
			inc = -1;
		else
		{
			sprite += multi;
			inc = 1;
		}

		if (flip_screen)
		{
			y=240-y;
			x=304-x;
			if (fx) fx=0; else fx=1;
			if (fy) fy=0; else fy=1;
			mult=16;
		}
		else mult=-16;

		while (multi >= 0)
		{
			drawgfx(bitmap,Machine->gfx[2],
					sprite - multi * inc,
					colour,
					fx,fy,
					x,y + mult * multi,
					cliprect,TRANSPARENCY_PEN,0);

			multi--;
		}
	}
}

static int dietgo_bank_callback(const int bank)
{
	return (bank & 0x7) * 0x1000;
}

VIDEO_START(dietgo)
{
	if (deco16_1_video_init())
		return 1;

	deco16_set_tilemap_bank_callback(0, dietgo_bank_callback);
	deco16_set_tilemap_bank_callback(1, dietgo_bank_callback);

	return 0;
}

VIDEO_UPDATE(dietgo)
{
	flip_screen = deco16_pf12_control[0]&0x80;
	deco16_pf12_update(deco16_pf1_rowscroll,deco16_pf2_rowscroll);

	fillbitmap(bitmap,Machine->pens[256],cliprect); /* not verified */

	deco16_tilemap_2_draw(bitmap,cliprect,TILEMAP_IGNORE_TRANSPARENCY,0);
	deco16_tilemap_1_draw(bitmap,cliprect,0,0);

	dietgogo_drawsprites(bitmap,cliprect);
}

static READ16_HANDLER( dietgo_298 )
{
	/* iq_132, Apparently MAME 0.78's auto-vblank handling sucks.*/
	/* Just flip it on and off every other read so that the game*/
	/* doesn't take a massive dump all over everything.*/

	unsigned short ret;
	static int vblank_fake;

	vblank_fake ^= 0x08;

	ret = input_port_0_word_r(0,0);
	ret &= ~8;
	ret ^= vblank_fake;

/*	printf ("%4.4x\n", ret);*/

	return ret;
}


static MEMORY_READ16_START( dietgo_readmem )
    { 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x200000, 0x20000f, MRA16_RAM },
	{ 0x210000, 0x211fff, MRA16_RAM },
	{ 0x212000, 0x213fff, MRA16_RAM },
	{ 0x220000, 0x2207ff, MRA16_RAM },
	{ 0x222000, 0x2227ff, MRA16_RAM },
	{ 0x280000, 0x2807ff, MRA16_RAM },
	{ 0x300000, 0x300bff, MRA16_RAM },
	{ 0x340298, 0x340299, dietgo_298 }, /*Fake V-blank hack and input_port_0_word_r*/
	{ 0x340342, 0x340343, input_port_1_word_r },
	{ 0x340506, 0x340507, input_port_2_word_r },
/*  { 0x340000, 0x3407ff, dietgo_104_prot_r }, Bypass the prot read via the above hack.*/
	{ 0x380000, 0x38ffff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( dietgo_writemem )
    { 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x200000, 0x20000f, MWA16_RAM, &deco16_pf12_control },
	{ 0x210000, 0x211fff, deco16_pf1_data_w, &deco16_pf1_data },
	{ 0x212000, 0x213fff, deco16_pf2_data_w, &deco16_pf2_data },
	{ 0x220000, 0x2207ff, MWA16_RAM, &deco16_pf1_rowscroll },
	{ 0x222000, 0x2227ff, MWA16_RAM, &deco16_pf2_rowscroll },
	{ 0x280000, 0x2807ff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x300000, 0x300bff, deco16_nonbuffered_palette_w, &paletteram16 },
	{ 0x340000, 0x3407ff, dietgo_104_prot_w },
	{ 0x380000, 0x38ffff, MWA16_RAM },
MEMORY_END

static MEMORY_READ_START( sound_readmem )
    { 0x000000, 0x00ffff, MRA_ROM },
	{ 0x100000, 0x100001, MRA_NOP },
	{ 0x110000, 0x110001, YM2151_status_port_0_r },
	{ 0x120000, 0x120001, OKIM6295_status_0_r },
	{ 0x130000, 0x130001, MRA_NOP },
	{ 0x140000, 0x140001, soundlatch_r },
	{ 0x1f0000, 0x1f1fff, MRA_BANK8 },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
    { 0x000000, 0x00ffff, MWA_ROM },
	{ 0x100000, 0x100001, MWA_NOP },
	{ 0x110000, 0x110001, YM2151_word_0_w },
	{ 0x120000, 0x120001, OKIM6295_data_0_w },
	{ 0x130000, 0x130001, MWA_NOP },
	{ 0x1f0000, 0x1f1fff, MWA_BANK8 },
	{ 0x1fec00, 0x1fec01, H6280_timer_w },
	{ 0x1ff402, 0x1ff403, H6280_irq_status_w },
MEMORY_END


INPUT_PORTS_START( dietgo )
	PORT_START	/* Verified as 4 bit input port only */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START 	/* 16bit */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_8WAY )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_8WAY )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_8WAY )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 ) 
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START 	/* Dip switch bank 1 */
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Continue Coin" )
	PORT_DIPSETTING(      0x0080, "1 Start/1 Continue" )
	PORT_DIPSETTING(      0x0000, "2 Start/1 Continue" )

	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0100, "1" )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPSETTING(      0x0300, "3" )
	PORT_DIPSETTING(      0x0200, "4" )
	PORT_DIPNAME( 0x0c00, 0x0c00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0800, "Easy" )
	PORT_DIPSETTING(      0x0c00, "Normal" )
	PORT_DIPSETTING(      0x0400, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) ) /* Demo_Sounds ) )*/
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) ) /* Players dont move in attract mode if on!?*/
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END

static struct GfxLayout tile_8x8_layout =
{
	8,8,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+8,RGN_FRAC(1,2)+0,RGN_FRAC(0,2)+8,RGN_FRAC(0,2)+0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	8*16
};

static struct GfxLayout tile_16x16_layout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+8,RGN_FRAC(1,2)+0,RGN_FRAC(0,2)+8,RGN_FRAC(0,2)+0 },
	{ 256,257,258,259,260,261,262,263,0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,8*16,9*16,10*16,11*16,12*16,13*16,14*16,15*16 },
	32*16
};

static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 24,8,16,0 },
	{ 512,513,514,515,516,517,518,519, 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
	  8*32, 9*32,10*32,11*32,12*32,13*32,14*32,15*32},
	32*32
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tile_8x8_layout,          0, 32 },	/* Tiles (8x8) */
	{ REGION_GFX1, 0, &tile_16x16_layout,         0, 32 },	/* Tiles (16x16) */
	{ REGION_GFX2, 0, &spritelayout,         0x200, 16 },	/* Tiles (16x16) */
	{ -1 } /* end of array */
};

static void sound_irq(int state)
{
	cpu_set_irq_line(1,1,state); /* IRQ 2 */
}

static struct YM2151interface ym2151_interface =
{
	1,
	32220000/9, /* Accurate, audio section crystal is 32.220 MHz */
	{ YM3012_VOL(40,MIXER_PAN_LEFT,40,MIXER_PAN_RIGHT) },
	{ sound_irq },
};

static struct OKIM6295interface okim6295_interface =
{
	1, 
	{ 32220000/32/132 },/* Frequency */
	{ REGION_SOUND1 },
	{ 60 }
};

static MACHINE_DRIVER_START( dietgo )
	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 14000000)
	MDRV_CPU_MEMORY(dietgo_readmem,dietgo_writemem)
	MDRV_CPU_VBLANK_INT(irq6_line_hold,1)

	MDRV_CPU_ADD(H6280,32220000/4/3)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(58)
	MDRV_VBLANK_DURATION(TIME_IN_USEC(2500))

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(dietgo)
	MDRV_VIDEO_UPDATE(dietgo)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END


/* Diet Go Go */

ROM_START( dietgo )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* DE102 code (encrypted) */
	ROM_LOAD16_BYTE( "jy00-2.h4",    0x000001, 0x040000, CRC(014dcf62) SHA1(1a28ce4a643ec8b6f062b1200342ed4dc6db38a1) )
	ROM_LOAD16_BYTE( "jy01-2.h5",    0x000000, 0x040000, CRC(793ebd83) SHA1(b9178f18ce6e9fca848cbbf9dce3f3856672bf94) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "jy02.m14",    0x00000, 0x10000, CRC(4e3492a5) SHA1(5f302bdbacbf95ea9f3694c48545a1d6bba4b019) )

	ROM_REGION( 0x100000, REGION_GFX1, 0 )
	ROM_LOAD( "may00",    0x00000, 0x100000, CRC(234d1f8d) SHA1(42d23aad20df20cbd2359cc12bdd47636b2027d3) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "may01",    0x000000, 0x100000, CRC(2da57d04) SHA1(3898e9fef365ecaa4d86aa11756b527a4fffb494) )
	ROM_LOAD16_BYTE( "may02",    0x000001, 0x100000, CRC(3a66a713) SHA1(beeb99156332cf4870738f7769b719a02d7b40af) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "may03",    0x00000, 0x80000, CRC(b6e42bae) SHA1(c282cdf7db30fb63340cc609bf00f5ab63a75583) )
ROM_END

ROM_START( dietgou )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* DE102 code (encrypted) */
	ROM_LOAD16_BYTE( "jx.00",    0x000001, 0x040000, CRC(1a9de04f) SHA1(7ce1e7cf4cdce2b02da4df2a6ae9a9e665e24422) )
	ROM_LOAD16_BYTE( "jx.01",    0x000000, 0x040000, CRC(79c097c8) SHA1(be49055ee324535e1118d243bd49e74ec1d2a2d7) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "jx.02",    0x00000, 0x10000, CRC(4e3492a5) SHA1(5f302bdbacbf95ea9f3694c48545a1d6bba4b019) )

	ROM_REGION( 0x100000, REGION_GFX1, 0 )
	ROM_LOAD( "may00",    0x00000, 0x100000, CRC(234d1f8d) SHA1(42d23aad20df20cbd2359cc12bdd47636b2027d3) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "may01",    0x000000, 0x100000, CRC(2da57d04) SHA1(3898e9fef365ecaa4d86aa11756b527a4fffb494) )
	ROM_LOAD16_BYTE( "may02",    0x000001, 0x100000, CRC(3a66a713) SHA1(beeb99156332cf4870738f7769b719a02d7b40af) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "may03",    0x00000, 0x80000, CRC(b6e42bae) SHA1(c282cdf7db30fb63340cc609bf00f5ab63a75583) )
ROM_END

ROM_START( dietgoe )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* DE102 code (encrypted) */
	ROM_LOAD16_BYTE( "jy00-1.4h",    0x000001, 0x040000, CRC(8bce137d) SHA1(55f5b1c89330803c6147f9656f2cabe8d1de8478) )
	ROM_LOAD16_BYTE( "jy01-1.5h",    0x000000, 0x040000, CRC(eca50450) SHA1(1a24117e3b1b66d7dbc5484c94cc2c627d34e6a3) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "jy02.m14",    0x00000, 0x10000, CRC(4e3492a5) SHA1(5f302bdbacbf95ea9f3694c48545a1d6bba4b019) )

	ROM_REGION( 0x100000, REGION_GFX1, 0 )
	ROM_LOAD( "may00",    0x00000, 0x100000, CRC(234d1f8d) SHA1(42d23aad20df20cbd2359cc12bdd47636b2027d3) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "may01",    0x000000, 0x100000, CRC(2da57d04) SHA1(3898e9fef365ecaa4d86aa11756b527a4fffb494) )
	ROM_LOAD16_BYTE( "may02",    0x000001, 0x100000, CRC(3a66a713) SHA1(beeb99156332cf4870738f7769b719a02d7b40af) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "may03",    0x00000, 0x80000, CRC(b6e42bae) SHA1(c282cdf7db30fb63340cc609bf00f5ab63a75583) )
ROM_END

ROM_START( dietgoj )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* DE102 code (encrypted) */
	ROM_LOAD16_BYTE( "jw-00-2.4h",    0x000001, 0x040000, CRC(e6ba6c49) SHA1(d5eaea81f1353c58c03faae67428f7ee98e766b1) )
	ROM_LOAD16_BYTE( "jw-01-2.5h",    0x000000, 0x040000, CRC(684a3d57) SHA1(bd7a57ba837a1dc8f92b5ebcb46e50db1f98524f) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "jx.02",    0x00000, 0x10000, CRC(4e3492a5) SHA1(5f302bdbacbf95ea9f3694c48545a1d6bba4b019) )

	ROM_REGION( 0x100000, REGION_GFX1, 0 )
	ROM_LOAD( "may00",    0x00000, 0x100000, CRC(234d1f8d) SHA1(42d23aad20df20cbd2359cc12bdd47636b2027d3) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "may01",    0x000000, 0x100000, CRC(2da57d04) SHA1(3898e9fef365ecaa4d86aa11756b527a4fffb494) )
	ROM_LOAD16_BYTE( "may02",    0x000001, 0x100000, CRC(3a66a713) SHA1(beeb99156332cf4870738f7769b719a02d7b40af) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "may03",    0x00000, 0x80000, CRC(b6e42bae) SHA1(c282cdf7db30fb63340cc609bf00f5ab63a75583) )
ROM_END

extern void deco102_decrypt_cpu(int address_xor, int data_select_xor, int opcode_select_xor);

static DRIVER_INIT( dietgo )
{
	deco56_decrypt(REGION_GFX1);
	deco102_decrypt_cpu(0xe9ba, 0x01, 0x19);
}

GAME( 1992, dietgo,   0,      dietgo, dietgo,  dietgo,    ROT0, "Data East Corporation", "Diet Go Go (Euro v1.1)"  )
GAME( 1992, dietgoe,  dietgo, dietgo, dietgo,  dietgo,    ROT0, "Data East Corporation", "Diet Go Go (Euro v1.1,1992.08.04)" ) /* weird, still version 1.1 but different date*/
GAME( 1992, dietgou,  dietgo, dietgo, dietgo,  dietgo,    ROT0, "Data East Corporation", "Diet Go Go (USA v1.1)" )
GAME( 1992, dietgoj,  dietgo, dietgo, dietgo,  dietgo,    ROT0, "Data East Corporation", "Diet Go Go (Japan v1.1)" )
