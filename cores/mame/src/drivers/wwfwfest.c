/*******************************************************************************
 WWF Wrestlefest (C) 1991 Technos Japan  (drivers/wwfwfest.c)
********************************************************************************
 driver by David Haywood

 Special Thanks to:

 Richard Bush & the Rest of the Raine Team - Raine's WWF Wrestlefest driver on
 which some of this driver has been based.

********************************************************************************

 Hardware:

 Primary CPU : 68000 - 12MHz

 Sound CPUs : Z80 - 3.579MHz

 Sound Chips : YM2151, M6295

 4 Layers from now on if mentioned will be refered to as

 BG0 - Background Layer 0
 BG1 - Background Layer 1
 SPR - Sprites
 FG0 - Foreground / Text Layer

 Priorities of BG0, BG1 and SPR can be changed

********************************************************************************

 Change Log:
 20 Jun 2001 | Did Pretty Much everything else, the game is now playable.
 19 Jun 2001 | Started the driver, based on Raine, the WWF Superstars driver,
			 | and the Double Dragon 3 Driver, got most of the basics done,
			 | the game will boot showing some graphics.

*******************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m68000/m68000.h"
#include "cpu/z80/z80.h"
#include "wwfwfest.h"

/*- in this file -*/
static READ16_HANDLER( wwfwfest_paletteram16_xxxxBBBBGGGGRRRR_word_r );
static WRITE16_HANDLER( wwfwfest_paletteram16_xxxxBBBBGGGGRRRR_word_w );
static WRITE16_HANDLER( wwfwfest_1410_write ); /* priority write */
static WRITE16_HANDLER( wwfwfest_scroll_write ); /* scrolling write */
static READ16_HANDLER( wwfwfest_inputs_read );
static WRITE_HANDLER( oki_bankswitch_w );
static WRITE16_HANDLER ( wwfwfest_soundwrite );

static WRITE16_HANDLER( wwfwfest_flipscreen_w )
{
	flip_screen_set(data&1);
}

/*******************************************************************************
 Memory Maps
********************************************************************************
 Pretty Straightforward

 still some unknown writes however, sound cpu memory map is the same as dd3
*******************************************************************************/

static MEMORY_READ16_START( readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },	/* Rom */
	{ 0x0c0000, 0x0c1fff, MRA16_RAM },	/* FG0 Ram */
	{ 0x0c2000, 0x0c3fff, MRA16_RAM },	/* SPR Ram */
	{ 0x080000, 0x080fff, MRA16_RAM },	/* BG0 Ram */
	{ 0x082000, 0x082fff, MRA16_RAM },	/* BG1 Ram */
	{ 0x140020, 0x140027, wwfwfest_inputs_read },	/* Inputs */
	{ 0x180000, 0x18ffff, wwfwfest_paletteram16_xxxxBBBBGGGGRRRR_word_r },	/* BG0 Ram */
	{ 0x1c0000, 0x1c3fff, MRA16_RAM },	/* Work Ram */
MEMORY_END

static MEMORY_WRITE16_START( writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },	/* Rom */
	{ 0x0c0000, 0x0c1fff, wwfwfest_fg0_videoram_w, &wwfwfest_fg0_videoram },	/* FG0 Ram - 4 bytes per tile */
	{ 0x0c2000, 0x0c3fff, MWA16_RAM, &spriteram16, &spriteram_size },	/* SPR Ram */
	{ 0x080000, 0x080fff, wwfwfest_bg0_videoram_w, &wwfwfest_bg0_videoram },	/* BG0 Ram - 4 bytes per tile */
	{ 0x082000, 0x082fff, wwfwfest_bg1_videoram_w, &wwfwfest_bg1_videoram },	/* BG1 Ram - 2 bytes per tile */
	{ 0x100000, 0x100007, wwfwfest_scroll_write },
	{ 0x10000a, 0x10000b, wwfwfest_flipscreen_w },
	{ 0x140000, 0x140001, MWA16_NOP }, /* Irq 3 ack */
	{ 0x140002, 0x140003, MWA16_NOP }, /* Irq 2 ack */
	{ 0x14000C, 0x14000D, wwfwfest_soundwrite },
	{ 0x140010, 0x140011, wwfwfest_1410_write },
	{ 0x180000, 0x18ffff, wwfwfest_paletteram16_xxxxBBBBGGGGRRRR_word_w, &paletteram16 },
	{ 0x1c0000, 0x1c3fff, MWA16_RAM },	/* Work Ram */
MEMORY_END

static MEMORY_READ_START( readmem_sound )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xc801, 0xc801, YM2151_status_port_0_r },
	{ 0xd800, 0xd800, OKIM6295_status_0_r },
	{ 0xe000, 0xe000, soundlatch_r },
MEMORY_END

static MEMORY_WRITE_START( writemem_sound )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xc800, 0xc800, YM2151_register_port_0_w },
	{ 0xc801, 0xc801, YM2151_data_port_0_w },
	{ 0xd800, 0xd800, OKIM6295_data_0_w },
	{ 0xe800, 0xe800, oki_bankswitch_w },
MEMORY_END

/*******************************************************************************
 Read / Write Handlers
********************************************************************************
 as used by the above memory map
*******************************************************************************/

/*- Palette Reads/Writes -*/

static READ16_HANDLER( wwfwfest_paletteram16_xxxxBBBBGGGGRRRR_word_r )
{
	offset = (offset & 0x000f) | (offset & 0x7fc0) >> 2;
	return paletteram16[offset];
}

static WRITE16_HANDLER( wwfwfest_paletteram16_xxxxBBBBGGGGRRRR_word_w )
{
	offset = (offset & 0x000f) | (offset & 0x7fc0) >> 2;
	paletteram16_xxxxBBBBGGGGRRRR_word_w (offset, data, mem_mask);
}

/*- Priority Control -*/


static WRITE16_HANDLER( wwfwfest_1410_write )
{
	wwfwfest_pri = data;
}

/*- Scroll Control -*/

static WRITE16_HANDLER( wwfwfest_scroll_write )
{
	switch (offset) {
		case 0x00:
			wwfwfest_bg0_scrollx = data;
			break;
		case 0x01:
			wwfwfest_bg0_scrolly = data;
			break;
		case 0x02:
			wwfwfest_bg1_scrollx = data;
			break;
		case 0x03:
			wwfwfest_bg1_scrolly = data;
			break;
	}
}

static READ16_HANDLER( wwfwfest_inputs_read )
{
	int tmp = 0;

	switch (offset)
	{
	case 0x00:
	tmp = readinputport(0) | readinputport(4) << 8;
	tmp &= 0xcfff;
	tmp |= ((readinputport(7) & 0xc0) << 6);
	break;
	case 0x01:
	tmp = readinputport(1);
	tmp &= 0xc0ff;
	tmp |= ((readinputport(7) & 0x3f)<<8);
	break;
	case 0x02:
	tmp = readinputport(2);
	tmp &= 0xc0ff;
	tmp |= ((readinputport(6) & 0x3f)<<8);
	break;
	case 0x03:
	tmp = (readinputport(3) | readinputport(5) << 8) ;
	tmp &= 0xfcff;
	tmp |= ((readinputport(6) & 0xc0) << 2);
	break;
	}

	return tmp;
}

/*- Sound Related (from dd3) -*/

static WRITE_HANDLER( oki_bankswitch_w )
{
	OKIM6295_set_bank_base(0, (data & 1) * 0x40000);
}

static WRITE16_HANDLER ( wwfwfest_soundwrite )
{
	soundlatch_w(1,data & 0xff);
	cpu_set_irq_line( 1, IRQ_LINE_NMI, PULSE_LINE );
}

/*******************************************************************************
 Input Ports
********************************************************************************
 There are 4 players, 2 sets of dipswitches and 2 misc
*******************************************************************************/

INPUT_PORTS_START( wwfwfest )
	PORT_START	/* Player 1 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START /* Player 2 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START /* Player 3 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER3 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER3 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER3 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START3 )

	PORT_START /* Player 4 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER4 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER4 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER4 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER4 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START4 )

	PORT_START /* Misc 1 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START /* Misc 2 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	/* Nb:  There are actually 3 dips on the board, 2 * 8, and 1 *4 */

	PORT_START /* Dips 1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 3C_1C )  )
	PORT_DIPSETTING(	0x01, DEF_STR( 2C_1C )  )
	PORT_DIPSETTING(	0x03, DEF_STR( 1C_1C )  )
	PORT_DIPSETTING(	0x02, DEF_STR( 1C_2C )  )
	PORT_DIPNAME( 0x04, 0x04, "Buy In Price"  )
	PORT_DIPSETTING(    0x04, "1 Coin" )
	PORT_DIPSETTING(    0x00, "As start price" )
	PORT_DIPNAME( 0x08, 0x08, "Regain Power Price"  )
	PORT_DIPSETTING(    0x08, "1 Coin" )
	PORT_DIPSETTING(    0x00, "As start price" )
	PORT_DIPNAME( 0x10, 0x00, "Continue Price"  )
	PORT_DIPSETTING(    0x10, "1 Coin" )
	PORT_DIPSETTING(    0x00, "As start price" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen )  )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "FBI Logo" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START /* Dips 2 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, "Players" )
/*	PORT_DIPSETTING(	0x00, "2" )*/
	PORT_DIPSETTING(	0x04, "2" )
	PORT_DIPSETTING(	0x08, "3" )
	PORT_DIPSETTING(	0x0c, "4" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x60, "Clear Stage Power Up" )
	PORT_DIPSETTING(	0x00, "0" )
	PORT_DIPSETTING(	0x20, "12" )
	PORT_DIPSETTING(	0x60, "24" )
	PORT_DIPSETTING(	0x40, "32" )
	PORT_DIPNAME( 0x80, 0x80, "Championship Game" )
	PORT_DIPSETTING(	0x00, "4th" )
	PORT_DIPSETTING(	0x80, "5th" )
INPUT_PORTS_END

/*******************************************************************************
 Graphic Decoding
*******************************************************************************/
static struct GfxLayout tiles8x8_layout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 2, 4, 6 },
	{ 1, 0, 8*8+1, 8*8+0, 16*8+1, 16*8+0, 24*8+1, 24*8+0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	32*8
};

static struct GfxLayout tile_layout =
{
	16,16,	/* 16*16 tiles */
	4096,	/* 8192 tiles */
	4,	/* 4 bits per pixel */
	{ 8, 0, 0x40000*8+8 , 0x40000*8+0 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			32*8+0, 32*8+1, 32*8+2, 32*8+3, 32*8+4, 32*8+5, 32*8+6, 32*8+7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			16*8, 16*9, 16*10, 16*11, 16*12, 16*13, 16*14, 16*15 },
	64*8	/* every tile takes 64 consecutive bytes */
};

static struct GfxLayout sprite_layout = {
	16,16,	/* 16*16 tiles */
	RGN_FRAC(1,4),
	4,	/* 4 bits per pixel */
	{ 0, 0x200000*8, 2*0x200000*8 , 3*0x200000*8 }, /* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
		16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
		8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8	/* every tile takes 32 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tiles8x8_layout, 0x0000, 16 },
	{ REGION_GFX2, 0, &sprite_layout,   0x0400, 16 },
	{ REGION_GFX3, 0, &tile_layout,     0x1000, 16 },
	{ REGION_GFX3, 0, &tile_layout,     0x0c00, 16 },
	{ -1 }
};

/*******************************************************************************
 Interrupt Function
*******************************************************************************/

static INTERRUPT_GEN( wwfwfest_interrupt ) {
	if( cpu_getiloops() == 0 )
		cpu_set_irq_line(0, 3, HOLD_LINE);
	else
		cpu_set_irq_line(0, 2, HOLD_LINE);
}

/*******************************************************************************
 Sound Stuff..
********************************************************************************
 Straight from Ddragon 3 with some adjusted volumes
*******************************************************************************/

static void dd3_ymirq_handler(int irq)
{
	cpu_set_irq_line( 1, 0 , irq ? ASSERT_LINE : CLEAR_LINE );
}

static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	3579545,
	{ YM3012_VOL(45,MIXER_PAN_LEFT,45,MIXER_PAN_RIGHT) },
	{ dd3_ymirq_handler }
};

static struct OKIM6295interface okim6295_interface =
{
	1,				/* 1 chip */
	{ 7759 },		/* frequency (Hz) */
	{ REGION_SOUND1 },	/* memory region */
	{ 90 }
};

VIDEO_EOF( wwfwfest )
{
	buffer_spriteram16_w(0,0,0);
}

/*******************************************************************************
 Machine Driver(s)
*******************************************************************************/

static MACHINE_DRIVER_START( wwfwfest )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)	/* 24 crystal, 12 rated chip */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(wwfwfest_interrupt,2)

	MDRV_CPU_ADD(Z80, 3579545)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(readmem_sound,writemem_sound)

	MDRV_FRAMES_PER_SECOND(57.444853)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_BUFFERS_SPRITERAM)
	MDRV_SCREEN_SIZE(320, 256)
	MDRV_VISIBLE_AREA(0, 319, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(8192)

	MDRV_VIDEO_START(wwfwfest)
	MDRV_VIDEO_EOF(wwfwfest)
	MDRV_VIDEO_UPDATE(wwfwfest)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END

/*******************************************************************************
 Rom Loaders / Game Drivers
********************************************************************************
 2 sets supported,
 wwfwfest - US? Set (Tecmo License / Distribution?)
 wwfwfstj - Japan? Set

 readme / info files below

--------------------------------------------------------------------------------
 wwfwfstj: README.TXT
--------------------------------------------------------------------------------
 Wrestlefest
 Technos 1991

 TA-0031
                                                         68000-12
  31J0_IC1  6264 6264 31A14-2 31A13-2 6264 6264 31A12-0  24MHz
  31J1_IC2
                   TJ-002          TJ-004
                                   6264                   SW1
                   28MHz                                  SW2
                                                          SW3
                                             61C16-35
                        61C16-35             61C16-35
  31J2_IC8
  31J3_IC9
  31J4_IC10
  31J5_IC11
  31J6_IC12
  31J7_IC13
  31J8_IC14    TJ-003                31A11-2  M6295   31J10_IC73
  31J9_IC15    61C16-35 61C16-35     Z80      YM2151

*******************************************************************************/
ROM_START( wwfwfest )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* Main CPU  (68000) */
	ROM_LOAD16_BYTE( "31a13-2.19", 0x00001, 0x40000, CRC(7175bca7) SHA1(992b47a787b5bc2a5a381ec78b8dfaf7d42c614b) )
	ROM_LOAD16_BYTE( "31a14-2.18", 0x00000, 0x40000, CRC(5d06bfd1) SHA1(39a93da662158aa5a9953dcabfcb47c2fc196dc7) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Sound CPU (Z80)  */
	ROM_LOAD( "31a11-2.42",    0x00000, 0x10000, CRC(5ddebfea) SHA1(30073963e965250d94f0dc3bd261a054850adf95) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "wf_73a.rom",    0x00000, 0x80000, CRC(6c522edb) SHA1(8005d59c94160638ba2ea7caf4e991fff03003d5) )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE ) /* FG0 Tiles (8x8) */
	ROM_LOAD( "31a12-0.33",    0x00000, 0x20000, CRC(d0803e20) SHA1(b68758e9a5522396f831a3972571f8aed54c64de) )

	ROM_REGION( 0x800000, REGION_GFX2, ROMREGION_DISPOSE ) /* SPR Tiles (16x16) */
	ROM_LOAD( "wf_09.rom",    0x000000, 0x100000, CRC(e395cf1d) SHA1(241f98145e295993c9b6a44dc087a9b61fbc9a6f) ) /* Tiles 0 */
	ROM_LOAD( "wf_08.rom",    0x100000, 0x100000, CRC(b5a97465) SHA1(08d82c29a5c02b83fdbd0bad649b74eb35ab7e54) ) /* Tiles 1 */
	ROM_LOAD( "wf_11.rom",    0x200000, 0x100000, CRC(2ce545e8) SHA1(82173e58a8476a6fe9d2c990fce1f71af117a0ea) ) /* Tiles 0 */
	ROM_LOAD( "wf_10.rom",    0x300000, 0x100000, CRC(00edb66a) SHA1(926606d1923936b6e75391b1ab03b369d9822d13) ) /* Tiles 1 */
	ROM_LOAD( "wf_12.rom",    0x400000, 0x100000, CRC(79956cf8) SHA1(52207263620a6b6dde66d3f8749b772577899ea5) ) /* Tiles 0 */
	ROM_LOAD( "wf_13.rom",    0x500000, 0x100000, CRC(74d774c3) SHA1(a723ac5d481bf91b12e17652fbb2d869c886dec0) ) /* Tiles 1 */
	ROM_LOAD( "wf_15.rom",    0x600000, 0x100000, CRC(dd387289) SHA1(2cad42d4e7cd1a49346f844058ae18c38bc686a8) ) /* Tiles 0 */
	ROM_LOAD( "wf_14.rom",    0x700000, 0x100000, CRC(44abe127) SHA1(c723e1dea117534e976d2d383e634faf073cd57b) ) /* Tiles 1 */

	ROM_REGION( 0x80000, REGION_GFX3, ROMREGION_DISPOSE ) /* BG0 / BG1 Tiles (16x16) */
	ROM_LOAD( "wf_01.rom",    0x40000, 0x40000, CRC(8a12b450) SHA1(2e15c949efcda8bb6f11afe3ff07ba1dee9c771c) ) /* 0,1 */
	ROM_LOAD( "wf_02.rom",    0x00000, 0x40000, CRC(82ed7155) SHA1(b338e1150ffe3277c11d4d6e801a7d3bd7c58492) ) /* 2,3 */
ROM_END

ROM_START( wwfwfsta )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* Main CPU  (68000) */
	ROM_LOAD16_BYTE( "wf_18.rom", 0x00000, 0x40000, CRC(933ea1a0) SHA1(61da142cfa7abd3b77ab21979c061a078c0d0c63) )
	ROM_LOAD16_BYTE( "wf_19.rom", 0x00001, 0x40000, CRC(bd02e3c4) SHA1(7ae63e48caf9919ce7b63b4c5aa9474ba8c336da) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Sound CPU (Z80)  */
	ROM_LOAD( "31a11-2.42",    0x00000, 0x10000, CRC(5ddebfea) SHA1(30073963e965250d94f0dc3bd261a054850adf95) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "wf_73a.rom",    0x00000, 0x80000, CRC(6c522edb) SHA1(8005d59c94160638ba2ea7caf4e991fff03003d5) )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE ) /* FG0 Tiles (8x8) */
	ROM_LOAD( "wf_33.rom",    0x00000, 0x20000, CRC(06f22615) SHA1(2e9418e372da85ea597977d912d8b35753655f4e) )

	ROM_REGION( 0x800000, REGION_GFX2, ROMREGION_DISPOSE ) /* SPR Tiles (16x16) */
	ROM_LOAD( "wf_09.rom",    0x000000, 0x100000, CRC(e395cf1d) SHA1(241f98145e295993c9b6a44dc087a9b61fbc9a6f) ) /* Tiles 0 */
	ROM_LOAD( "wf_08.rom",    0x100000, 0x100000, CRC(b5a97465) SHA1(08d82c29a5c02b83fdbd0bad649b74eb35ab7e54) ) /* Tiles 1 */
	ROM_LOAD( "wf_11.rom",    0x200000, 0x100000, CRC(2ce545e8) SHA1(82173e58a8476a6fe9d2c990fce1f71af117a0ea) ) /* Tiles 0 */
	ROM_LOAD( "wf_10.rom",    0x300000, 0x100000, CRC(00edb66a) SHA1(926606d1923936b6e75391b1ab03b369d9822d13) ) /* Tiles 1 */
	ROM_LOAD( "wf_12.rom",    0x400000, 0x100000, CRC(79956cf8) SHA1(52207263620a6b6dde66d3f8749b772577899ea5) ) /* Tiles 0 */
	ROM_LOAD( "wf_13.rom",    0x500000, 0x100000, CRC(74d774c3) SHA1(a723ac5d481bf91b12e17652fbb2d869c886dec0) ) /* Tiles 1 */
	ROM_LOAD( "wf_15.rom",    0x600000, 0x100000, CRC(dd387289) SHA1(2cad42d4e7cd1a49346f844058ae18c38bc686a8) ) /* Tiles 0 */
	ROM_LOAD( "wf_14.rom",    0x700000, 0x100000, CRC(44abe127) SHA1(c723e1dea117534e976d2d383e634faf073cd57b) ) /* Tiles 1 */

	ROM_REGION( 0x80000, REGION_GFX3, ROMREGION_DISPOSE ) /* BG0 / BG1 Tiles (16x16) */
	ROM_LOAD( "wf_01.rom",    0x40000, 0x40000, CRC(8a12b450) SHA1(2e15c949efcda8bb6f11afe3ff07ba1dee9c771c) ) /* 0,1 */
	ROM_LOAD( "wf_02.rom",    0x00000, 0x40000, CRC(82ed7155) SHA1(b338e1150ffe3277c11d4d6e801a7d3bd7c58492) ) /* 2,3 */
ROM_END

ROM_START( wwfwfstj )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* Main CPU  (68000) */
	ROM_LOAD16_BYTE( "31j13-0.bin", 0x00001, 0x40000, CRC(2147780d) SHA1(9a7a5db06117f3780e084d3f0c7b642ff8a9db55) )
	ROM_LOAD16_BYTE( "31j14-0.bin", 0x00000, 0x40000, CRC(d76fc747) SHA1(5f6819bc61756d1df4ac0776ac420a59c438cf8a) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Sound CPU (Z80)  */
	ROM_LOAD( "31a11-2.42",    0x00000, 0x10000, CRC(5ddebfea) SHA1(30073963e965250d94f0dc3bd261a054850adf95) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "wf_73a.rom",    0x00000, 0x80000, CRC(6c522edb) SHA1(8005d59c94160638ba2ea7caf4e991fff03003d5) )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE ) /* FG0 Tiles (8x8) */
	ROM_LOAD( "31j12-0.bin",    0x00000, 0x20000, CRC(f4821fe0) SHA1(e5faa9860e9d4e75393b64ca85a8bfc4852fd4fd) )

	ROM_REGION( 0x800000, REGION_GFX2, ROMREGION_DISPOSE ) /* SPR Tiles (16x16) */
	ROM_LOAD( "wf_09.rom",    0x000000, 0x100000, CRC(e395cf1d) SHA1(241f98145e295993c9b6a44dc087a9b61fbc9a6f) ) /* Tiles 0 */
	ROM_LOAD( "wf_08.rom",    0x100000, 0x100000, CRC(b5a97465) SHA1(08d82c29a5c02b83fdbd0bad649b74eb35ab7e54) ) /* Tiles 1 */
	ROM_LOAD( "wf_11.rom",    0x200000, 0x100000, CRC(2ce545e8) SHA1(82173e58a8476a6fe9d2c990fce1f71af117a0ea) ) /* Tiles 0 */
	ROM_LOAD( "wf_10.rom",    0x300000, 0x100000, CRC(00edb66a) SHA1(926606d1923936b6e75391b1ab03b369d9822d13) ) /* Tiles 1 */
	ROM_LOAD( "wf_12.rom",    0x400000, 0x100000, CRC(79956cf8) SHA1(52207263620a6b6dde66d3f8749b772577899ea5) ) /* Tiles 0 */
	ROM_LOAD( "wf_13.rom",    0x500000, 0x100000, CRC(74d774c3) SHA1(a723ac5d481bf91b12e17652fbb2d869c886dec0) ) /* Tiles 1 */
	ROM_LOAD( "wf_15.rom",    0x600000, 0x100000, CRC(dd387289) SHA1(2cad42d4e7cd1a49346f844058ae18c38bc686a8) ) /* Tiles 0 */
	ROM_LOAD( "wf_14.rom",    0x700000, 0x100000, CRC(44abe127) SHA1(c723e1dea117534e976d2d383e634faf073cd57b) ) /* Tiles 1 */

	ROM_REGION( 0x80000, REGION_GFX3, ROMREGION_DISPOSE ) /* BG0 / BG1 Tiles (16x16) */
	ROM_LOAD( "wf_01.rom",    0x40000, 0x40000, CRC(8a12b450) SHA1(2e15c949efcda8bb6f11afe3ff07ba1dee9c771c) ) /* 0,1 */
	ROM_LOAD( "wf_02.rom",    0x00000, 0x40000, CRC(82ed7155) SHA1(b338e1150ffe3277c11d4d6e801a7d3bd7c58492) ) /* 2,3 */
ROM_END

GAME( 1991, wwfwfest, 0,        wwfwfest, wwfwfest, 0, ROT0, "Technos Japan", "WWF WrestleFest (US)" )
GAME( 1991, wwfwfsta, wwfwfest, wwfwfest, wwfwfest, 0, ROT0, "Technos Japan (Tecmo license)", "WWF WrestleFest (US Tecmo)" )
GAME( 1991, wwfwfstj, wwfwfest, wwfwfest, wwfwfest, 0, ROT0, "Technos Japan", "WWF WrestleFest (Japan)" )
