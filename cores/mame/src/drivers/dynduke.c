/***************************************************************************

	Dynamite Duke						(c) 1989 Seibu Kaihatsu/Fabtek
	The Double Dynamites				(c) 1989 Seibu Kaihatsu/Fabtek


	To access test mode, reset with both start buttons held.

	Coin inputs are handled by the sound CPU, so they don't work with sound
	disabled. Just put the game in Free Play mode.

	The background layer is 5bpp and I'm not 100% sure the colours are
	correct on it, although the layer is 5bpp the palette data is 4bpp.
	My current implementation looks pretty good though I've never seen
	the real game.

	There is a country code byte in the program to select between
	Seibu Kaihatsu/Fabtek/Taito licenses.

	Emulation by Bryan McPhail, mish@tendril.co.uk

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"
#include "sndhrdw/seibu.h"

READ_HANDLER( dynduke_background_r );
READ_HANDLER( dynduke_foreground_r );
WRITE_HANDLER( dynduke_background_w );
WRITE_HANDLER( dynduke_foreground_w );
WRITE_HANDLER( dynduke_text_w );
WRITE_HANDLER( dynduke_gfxbank_w );
VIDEO_START( dynduke );
WRITE_HANDLER( dynduke_control_w );
VIDEO_UPDATE( dynduke );
WRITE_HANDLER( dynduke_paletteram_w );

static unsigned char *dynduke_shared_ram;
extern unsigned char *dynduke_back_data,*dynduke_fore_data,*dynduke_scroll_ram,*dynduke_control_ram;

/***************************************************************************/

static READ_HANDLER( dynduke_shared_r ) { return dynduke_shared_ram[offset]; }
static WRITE_HANDLER( dynduke_shared_w ) { dynduke_shared_ram[offset]=data; }


/******************************************************************************/

static MEMORY_READ_START( readmem )
	{ 0x00000, 0x07fff, MRA_RAM },
	{ 0x0a000, 0x0afff, dynduke_shared_r },
	{ 0x0b000, 0x0b000, input_port_1_r },
	{ 0x0b001, 0x0b001, input_port_2_r },
	{ 0x0b002, 0x0b002, input_port_3_r },
	{ 0x0b003, 0x0b003, input_port_4_r },
	{ 0x0d000, 0x0d00d, seibu_main_v30_r },
	{ 0xa0000, 0xfffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x00000, 0x06fff, MWA_RAM },
	{ 0x07000, 0x07fff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x08000, 0x080ff, MWA_RAM, &dynduke_scroll_ram },
	{ 0x0a000, 0x0afff, dynduke_shared_w, &dynduke_shared_ram },
	{ 0x0b000, 0x0b007, dynduke_control_w, &dynduke_control_ram },
	{ 0x0c000, 0x0c7ff, dynduke_text_w, &videoram },
	{ 0x0d000, 0x0d00d, seibu_main_v30_w },
	{ 0xa0000, 0xfffff, MWA_ROM },
MEMORY_END

static MEMORY_READ_START( sub_readmem )
	{ 0x00000, 0x05fff, MRA_RAM },
	{ 0x06000, 0x067ff, dynduke_background_r },
	{ 0x06800, 0x06fff, dynduke_foreground_r },
	{ 0x07000, 0x07fff, paletteram_r },
	{ 0x08000, 0x08fff, dynduke_shared_r },
	{ 0xc0000, 0xfffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( sub_writemem )
	{ 0x00000, 0x05fff, MWA_RAM },
	{ 0x06000, 0x067ff, dynduke_background_w, &dynduke_back_data },
	{ 0x06800, 0x06fff, dynduke_foreground_w, &dynduke_fore_data },
	{ 0x07000, 0x07fff, dynduke_paletteram_w, &paletteram },
	{ 0x08000, 0x08fff, dynduke_shared_w },
	{ 0x0a000, 0x0a001, dynduke_gfxbank_w },
	{ 0x0c000, 0x0c001, MWA_NOP },
	{ 0xc0000, 0xfffff, MWA_ROM },
MEMORY_END

/******************************************************************************/

INPUT_PORTS_START( dynduke )
	SEIBU_COIN_INPUTS	/* Must be port 0: coin inputs read through sound cpu */

	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* Dip switch A */
	PORT_DIPNAME( 0x07, 0x06, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x18, 0x08, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x20, 0x20, "Starting Coin" )
	PORT_DIPSETTING(    0x20, "normal" )
	PORT_DIPSETTING(    0x00, "X 2" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* Dip switch B */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, "Difficulty?" )
	PORT_DIPSETTING(    0x30, "Normal" )
	PORT_DIPSETTING(    0x20, "Easy" )
	PORT_DIPSETTING(    0x10, "Hard" )
	PORT_DIPSETTING(    0x00, "Very Hard" )
	PORT_DIPNAME( 0x40, 0x40, "Continue?" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END

/******************************************************************************/

static struct GfxLayout charlayout =
{
	8,8,		/* 8*8 characters */
	1024,
	4,			/* 4 bits per pixel */
	{ 4,0,(0x10000*8)+4,0x10000*8 },
	{ 0,1,2,3,8,9,10,11 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	128
};

static struct GfxLayout spritelayout =
{
  16,16,	/* 16*16 tiles */
  0x4000,
  4,		/* 4 bits per pixel */
  { 12, 8, 4, 0 },
  {
    0,1,2,3, 16,17,18,19,
	512+0,512+1,512+2,512+3,
	512+8+8,512+9+8,512+10+8,512+11+8,
  },
  {
	0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
	8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32,
  },
  1024
};

static struct GfxLayout bg_layout =
{
	16,16,
	RGN_FRAC(1,3),
	6,
	{ RGN_FRAC(2,3)+4, RGN_FRAC(2,3)+0,
	  RGN_FRAC(1,3)+4, RGN_FRAC(1,3)+0,
	                4,               0 },

	{
		0,1,2,3,8,9,10,11,
		256+0,256+1,256+2,256+3,256+8,256+9,256+10,256+11
	},
	{
		0*16,1*16,2*16,3*16,4*16,5*16,6*16,7*16,
		8*16,9*16,10*16,11*16,12*16,13*16,14*16,15*16
	},
	512
};

static struct GfxLayout fg_layout =
{
	16,16,
	0x2000,
	4,
	{ 0x80000*8+4, 0x80000*8, 4, 0 },
	{
		0,1,2,3,8,9,10,11,
		256+0,256+1,256+2,256+3,256+8,256+9,256+10,256+11
	},
	{
		0*16,1*16,2*16,3*16,4*16,5*16,6*16,7*16,
		8*16,9*16,10*16,11*16,12*16,13*16,14*16,15*16
	},
	512
};

static struct GfxDecodeInfo dynduke_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,   0x500, 16 },
	{ REGION_GFX2, 0, &bg_layout,    0x000, 128 },
	{ REGION_GFX3, 0, &fg_layout,    0x200, 16 },
	{ REGION_GFX4, 0, &spritelayout, 0x300, 32 },
	{ -1 } /* end of array */
};

/******************************************************************************/

/* Parameters: YM3812 frequency, Oki frequency, Oki memory region */
SEIBU_SOUND_SYSTEM_YM3812_HARDWARE(14318180/4,8000,REGION_SOUND1);

static INTERRUPT_GEN( dynduke_interrupt )
{
	cpu_set_irq_line_and_vector(cpu_getactivecpu(), 0, HOLD_LINE, 0xc8/4);	/* VBL */
}

VIDEO_EOF( dynduke )
{
	buffer_spriteram_w(0,0); /* Could be a memory location instead */
}

static MACHINE_DRIVER_START( dynduke )

	/* basic machine hardware */
	MDRV_CPU_ADD(V30,16000000/2) /* NEC V30-8 CPU */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(dynduke_interrupt,1)

	MDRV_CPU_ADD(V30,16000000/2) /* NEC V30-8 CPU */
	MDRV_CPU_MEMORY(sub_readmem,sub_writemem)
	MDRV_CPU_VBLANK_INT(dynduke_interrupt,1)

	SEIBU_SOUND_SYSTEM_CPU(14318180/4)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(60)	/* CPU interleave  */

	MDRV_MACHINE_INIT(seibu_sound_2)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_BUFFERS_SPRITERAM)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(dynduke_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(dynduke)
	MDRV_VIDEO_EOF(dynduke)
	MDRV_VIDEO_UPDATE(dynduke)

	/* sound hardware */
	SEIBU_SOUND_SYSTEM_YM3812_INTERFACE
MACHINE_DRIVER_END

/***************************************************************************/

ROM_START( dynduke )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* v30 main cpu */
	ROM_LOAD16_BYTE("dd1.cd8",   0x0a0000, 0x10000, CRC(a5e2a95a) SHA1(135d57073d826b9cf46fb43dc49439e1400fb021) )
	ROM_LOAD16_BYTE("dd2.cd7",   0x0a0001, 0x10000, CRC(7e51af22) SHA1(b26103c0d41c469d1e2d1e4e89f591c0d9cdb67c) )
	ROM_LOAD16_BYTE("dd3.ef8",   0x0c0000, 0x20000, CRC(a56f8692) SHA1(00d86c660efae30c008f8220fdfd397b7d69b2cd) )
	ROM_LOAD16_BYTE("dd4.ef7",   0x0c0001, 0x20000, CRC(ee4b87b3) SHA1(8e470543bce07cd8682f3745e15c4f1141d9549b) )

	ROM_REGION( 0x100000, REGION_CPU2, 0 ) /* v30 sub cpu */
	ROM_LOAD16_BYTE("dd5.p8",  0x0e0000, 0x10000, CRC(883d319c) SHA1(b0df05bfe342a5289a6368be26317fa879975463) )
	ROM_LOAD16_BYTE("dd6.p7",  0x0e0001, 0x10000, CRC(d94cb4ff) SHA1(653247c420a2af037106470556e6801b29bc58e8) )

	ROM_REGION( 0x20000*2, REGION_CPU3, 0 ) /* sound Z80 */
	ROM_LOAD( "dd8.w8",       0x000000, 0x08000, CRC(3c29480b) SHA1(031a0b808df32b5ae4f722c9e9f69554d30505c1) )
	ROM_CONTINUE(             0x010000, 0x08000 )
	ROM_COPY( REGION_CPU3, 0, 0x018000, 0x08000 )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "dd9.jk5",	0x000000, 0x04000, CRC(f2bc9af4) SHA1(9092ebe9dced243c3a7f63198a1180143dd42cea) ) /* chars */
	ROM_LOAD( "dd10.jk3",	0x010000, 0x04000, CRC(c2a9f19b) SHA1(0bcea042386109c277c6c5dbe52a020f9ea5972b) )

	ROM_REGION( 0x180000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "dd.a2",		0x000000, 0x40000, CRC(598f343f) SHA1(eee794d9d0a92e066f00818bfb63e8ca46bda764) ) /* background */
	ROM_LOAD( "dd.b2",		0x040000, 0x40000, CRC(41a9088d) SHA1(eb0b7370dc773cb6f5066b044934ffb42bb06587) )
	ROM_LOAD( "dd.c2",		0x080000, 0x40000, CRC(cc341b42) SHA1(8c3cf09a3a0080a1cd7c1049cb8d11f03de50919) )
	ROM_LOAD( "dd.d2",		0x0c0000, 0x40000, CRC(4752b4d7) SHA1(4625b7885ff9d302e78d7324b3592ac5a3cead86) )
	ROM_LOAD( "dd.de3",		0x100000, 0x40000, CRC(44a4cb62) SHA1(70b2043d0428c90ee22ccd479d9710af24d359f6) )
	ROM_LOAD( "dd.ef3",		0x140000, 0x40000, CRC(aa8aee1a) SHA1(8b2b8dcb2287318e314b256f84c23424cfe29462) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "dd.mn3",		0x000000, 0x40000, CRC(2ee0ca98) SHA1(2ef2c4fd337e0ee4685e4863909985ee0a4c4b91) ) /* foreground */
	ROM_LOAD( "dd.mn4",		0x040000, 0x40000, CRC(6c71e2df) SHA1(fe87277a625010c214e05b43572fadb493b3d05d) )
	ROM_LOAD( "dd.n45",		0x080000, 0x40000, CRC(85d918e1) SHA1(882cdf633288c95f2349d7c86799875b707ca347) )
	ROM_LOAD( "dd.mn5",		0x0c0000, 0x40000, CRC(e71e34df) SHA1(dce8e3de61f3869da57d476bf861856154365058) )

	ROM_REGION( 0x200000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE(  "dd.n1", 0x000000, 0x40000, CRC(cf1db927) SHA1(3fde2ca7e7e302773ae01ed89edf0bcf69fc7aa1) ) /* sprites */
	ROM_LOAD16_BYTE(  "dd.n2", 0x000001, 0x40000, CRC(5328150f) SHA1(bb847a2ff7e5ac668e974d2853519d86feb81e03) )
	ROM_LOAD16_BYTE(  "dd.m1", 0x080000, 0x40000, CRC(80776452) SHA1(319bfc90ccf04b9e5aaac5701767d3f7bbb71626) )
	ROM_LOAD16_BYTE(  "dd.m2", 0x080001, 0x40000, CRC(ff61a573) SHA1(cfbe6c017c276d2fc1f083013b5df3686381753b) )
	ROM_LOAD16_BYTE(  "dd.e1", 0x100000, 0x40000, CRC(84a0b87c) SHA1(62075128093f21ee6ea09cc2d4bc8e630b275fce) )
	ROM_LOAD16_BYTE(  "dd.e2", 0x100001, 0x40000, CRC(a9585df2) SHA1(2eeac27dd018dd334447d539fdae2989c731e764) )
	ROM_LOAD16_BYTE(  "dd.f1", 0x180000, 0x40000, CRC(9aed24ba) SHA1(0068b5bc0d7c817eee3bfbf7de6d19652ba78d41) )
	ROM_LOAD16_BYTE(  "dd.f2", 0x180001, 0x40000, CRC(3eb5783f) SHA1(5487ceb4f3241241af1a81b1bb686bd3af10b0d1) )

	ROM_REGION( 0x10000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "dd7.x10", 0x000000, 0x10000, CRC(9cbc7b41) SHA1(107c19d3d71ee6af63d03f7278310c5e3786f91d) )
ROM_END

ROM_START( dbldyn )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* v30 main cpu */
	ROM_LOAD16_BYTE("dd1.cd8", 0x0a0000, 0x10000, CRC(a5e2a95a) SHA1(135d57073d826b9cf46fb43dc49439e1400fb021) )
	ROM_LOAD16_BYTE("dd2.cd7", 0x0a0001, 0x10000, CRC(7e51af22) SHA1(b26103c0d41c469d1e2d1e4e89f591c0d9cdb67c) )
	ROM_LOAD16_BYTE("3.8e",    0x0c0000, 0x20000, CRC(9b785028) SHA1(d94c41f9f8969c0effc05d5d6c44474a396a8177) )
	ROM_LOAD16_BYTE("4.7e",    0x0c0001, 0x20000, CRC(0d0f6350) SHA1(d289bd9ac308ba1079d5b8931cc913fd326129d3) )

	ROM_REGION( 0x100000, REGION_CPU2, 0 ) /* v30 sub cpu */
	ROM_LOAD16_BYTE("5.8p",  0x0e0000, 0x10000, CRC(ea56d719) SHA1(6cade731316c280ef4e809aa700fdbaaabff41d0) )
	ROM_LOAD16_BYTE("6.7p",  0x0e0001, 0x10000, CRC(9ffa0ecd) SHA1(a22c46312ab247cd824dadf840cf1f2b0305bb29) )

	ROM_REGION( 0x20000*2, REGION_CPU3, 0 ) /* sound Z80 */
	ROM_LOAD( "8.8w",         0x000000, 0x08000, CRC(f4066081) SHA1(0e5246f4f5513be11e6ed3ea26aada7e0a17a448) )
	ROM_CONTINUE(             0x010000, 0x08000 )
	ROM_COPY( REGION_CPU3, 0, 0x018000, 0x08000 )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "9.5k",	    0x004000, 0x4000, CRC(16bec703) SHA1(c3b5311dee0e4750bc4dbce10deb46669179ac6f) ) /* chars */
	ROM_CONTINUE(           0x000000, 0x4000 )
	ROM_CONTINUE(           0x008000, 0x8000 )
	ROM_LOAD( "10.4k",	    0x014000, 0x4000, CRC(719f909d) SHA1(fbe8d8c554caa15a19efc6ac2a3a6857f9ef3feb) )
	ROM_CONTINUE(           0x010000, 0x4000 )
	ROM_CONTINUE(           0x008000, 0x8000 )

	ROM_REGION( 0x180000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "dd.a2",		0x000000, 0x40000, CRC(598f343f) SHA1(eee794d9d0a92e066f00818bfb63e8ca46bda764) ) /* background */
	ROM_LOAD( "dd.b2",		0x040000, 0x40000, CRC(41a9088d) SHA1(eb0b7370dc773cb6f5066b044934ffb42bb06587) )
	ROM_LOAD( "dd.c2",		0x080000, 0x40000, CRC(cc341b42) SHA1(8c3cf09a3a0080a1cd7c1049cb8d11f03de50919) )
	ROM_LOAD( "dd.d2",		0x0c0000, 0x40000, CRC(4752b4d7) SHA1(4625b7885ff9d302e78d7324b3592ac5a3cead86) )
	ROM_LOAD( "dd.de3",		0x100000, 0x40000, CRC(44a4cb62) SHA1(70b2043d0428c90ee22ccd479d9710af24d359f6) )
	ROM_LOAD( "dd.ef3",		0x140000, 0x40000, CRC(aa8aee1a) SHA1(8b2b8dcb2287318e314b256f84c23424cfe29462) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "dd.mn3",		0x000000, 0x40000, CRC(2ee0ca98) SHA1(2ef2c4fd337e0ee4685e4863909985ee0a4c4b91) ) /* foreground */
	ROM_LOAD( "dd.mn4",		0x040000, 0x40000, CRC(6c71e2df) SHA1(fe87277a625010c214e05b43572fadb493b3d05d) )
	ROM_LOAD( "dd.n45",		0x080000, 0x40000, CRC(85d918e1) SHA1(882cdf633288c95f2349d7c86799875b707ca347) )
	ROM_LOAD( "dd.mn5",		0x0c0000, 0x40000, CRC(e71e34df) SHA1(dce8e3de61f3869da57d476bf861856154365058) )

	ROM_REGION( 0x200000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE(  "dd.n1", 0x000000, 0x40000, CRC(cf1db927) SHA1(3fde2ca7e7e302773ae01ed89edf0bcf69fc7aa1) ) /* sprites */
	ROM_LOAD16_BYTE(  "dd.n2", 0x000001, 0x40000, CRC(5328150f) SHA1(bb847a2ff7e5ac668e974d2853519d86feb81e03) )
	ROM_LOAD16_BYTE(  "dd.m1", 0x080000, 0x40000, CRC(80776452) SHA1(319bfc90ccf04b9e5aaac5701767d3f7bbb71626) )
	ROM_LOAD16_BYTE(  "dd.m2", 0x080001, 0x40000, CRC(ff61a573) SHA1(cfbe6c017c276d2fc1f083013b5df3686381753b) )
	ROM_LOAD16_BYTE(  "dd.e1", 0x100000, 0x40000, CRC(84a0b87c) SHA1(62075128093f21ee6ea09cc2d4bc8e630b275fce) )
	ROM_LOAD16_BYTE(  "dd.e2", 0x100001, 0x40000, CRC(a9585df2) SHA1(2eeac27dd018dd334447d539fdae2989c731e764) )
	ROM_LOAD16_BYTE(  "dd.f1", 0x180000, 0x40000, CRC(9aed24ba) SHA1(0068b5bc0d7c817eee3bfbf7de6d19652ba78d41) )
	ROM_LOAD16_BYTE(  "dd.f2", 0x180001, 0x40000, CRC(3eb5783f) SHA1(5487ceb4f3241241af1a81b1bb686bd3af10b0d1) )

	ROM_REGION( 0x10000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "dd7.x10", 0x000000, 0x10000, CRC(9cbc7b41) SHA1(107c19d3d71ee6af63d03f7278310c5e3786f91d) )
ROM_END

/***************************************************************************/


static DRIVER_INIT( dynduke )
{
	seibu_sound_decrypt(REGION_CPU3,0x20000);
}


GAME( 1989, dynduke, 0,       dynduke, dynduke, dynduke, ROT0, "Seibu Kaihatsu (Fabtek license)", "Dynamite Duke" )
GAME( 1989, dbldyn,  dynduke, dynduke, dynduke, dynduke, ROT0, "Seibu Kaihatsu (Fabtek license)", "The Double Dynamites" )
