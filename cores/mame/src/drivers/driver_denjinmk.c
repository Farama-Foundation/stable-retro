/***************************************************************************

Denjin Makai (c) Banpresto 1994
-----------

David Graves

Made from MAME D-con and Toki drivers (by Bryan McPhail, Jarek Parchanski)


Denjin Makai
------------

- Palette Ram format correct,but color offset wrong? Probably protection related,game updates paletteram with rom data (or bad program rom?)..

- Needs to patch a sound-related comm to make this to work, same as SD Gundam Psycho Salamander No Kyoui (68k never writes to port 6 for whatever reason).

- There are a bunch of unemulated registers, one of them seems to be a brightness control of some sort. Needs a PCB side-by-side test.

- there are some ROM writes from time to time, could be a coding bug or something related to the protection.

***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "sndhrdw/seibu.h"
#include "cpu/m68000/m68000.h"
#include "machine/seicop.h"
#include "includes/denjinmk.h"
#include "vidhrdw/generic.h"


/*****************************************************************************/

/* did they swap the lines, or does the protection device swap the words during the DMA?? */
WRITE16_HANDLER( denjin_paletteram16_xBBBBBGGGGGRRRRR_word_w )
{
	offset^=1;
	COMBINE_DATA(&paletteram16[offset]);
	paletteram16_xBBBBBGGGGGRRRRR_word_w(offset,data,mem_mask);
}


static MEMORY_READ16_START( denjinmk_readmem )
    { 0x000000, 0x0fffff, MRA16_ROM },
	{ 0x100000, 0x1003ff, MRA16_RAM },
	{ 0x100400, 0x1007ff, denjinmk_mcu_r },
	{ 0x100800, 0x100fff, MRA16_RAM },
	{ 0x101000, 0x1017ff, MRA16_RAM },
	{ 0x101800, 0x101fff, MRA16_RAM },
	{ 0x102000, 0x1027ff, MRA16_RAM },
	{ 0x102800, 0x103fff, MRA16_RAM },
	{ 0x104000, 0x104fff, MRA16_RAM },
	{ 0x105000, 0x105fff, MRA16_RAM },
	{ 0x106000, 0x107fff, MRA16_RAM },
	{ 0x108000, 0x11dfff, MRA16_RAM },
	{ 0x11e000, 0x11efff, MRA16_RAM },
	{ 0x11f000, 0x11ffff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( denjinmk_writemem )
    { 0x000000, 0x0fffff, MWA16_ROM },
	{ 0x100000, 0x1003ff, MWA16_RAM },
	{ 0x100400, 0x1007ff, denjinmk_mcu_w, &cop_mcu_ram },  /* COP mcu */
	{ 0x100800, 0x100fff, MWA16_RAM },
	{ 0x101000, 0x1017ff, denjinmk_background_w, &denjinmk_back_data },
	{ 0x101800, 0x101fff, denjinmk_foreground_w, &denjinmk_fore_data },
	{ 0x102000, 0x1027ff, denjinmk_midground_w, &denjinmk_mid_data },
	{ 0x102800, 0x103fff, denjinmk_text_w, &denjinmk_textram },
    { 0x104000, 0x104fff, denjin_paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
	{ 0x105000, 0x105fff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x106000, 0x107fff, MWA16_RAM },
	{ 0x108000, 0x11dfff, MWA16_RAM },
	{ 0x11e000, 0x11efff, MWA16_RAM },
	{ 0x11f000, 0x11ffff, MWA16_RAM },
MEMORY_END


/*****************************************************************************/

INPUT_PORTS_START( denjinmk )
	PORT_START	/* coin inputs read through sound cpu, an impulse of 4 frame is too much for this game, especially for coin 2 */
	PORT_BIT_IMPULSE( 0x01, IP_ACTIVE_HIGH, IPT_COIN1, 2 )
	PORT_BIT_IMPULSE( 0x02, IP_ACTIVE_HIGH, IPT_COIN2, 2 )

	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( On ) )
	PORT_SERVICE( 0x0040, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0080, 0x0080, "Language" ) /*it actually skips the story entirely, so just remain JP as default*/
	PORT_DIPSETTING(      0x0080, "Japanese" )
	PORT_DIPSETTING(      0x0000, "English" )

	PORT_DIPNAME( 0x0f00, 0x0f00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0500, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0600, "3 Coins / 5 Credits" )
	PORT_DIPSETTING(      0x0400, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(      0x0f00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0300, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(      0x0700, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0e00, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0d00, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0c00, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0b00, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0a00, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x0900, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xf000, 0xf000, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x5000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x6000, "3 Coins / 5 Credits" )
	PORT_DIPSETTING(      0x4000, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(      0xf000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x3000, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(      0x7000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0xe000, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0xd000, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0xc000, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0xb000, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0xa000, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x9000, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW,  IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW,  IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW,  IPT_BUTTON1  | IPF_PLAYER3 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW,  IPT_BUTTON2  | IPF_PLAYER3 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0xffff, IP_ACTIVE_LOW, IPT_UNUSED ) /*unused according to the test mode*/
INPUT_PORTS_END


/*****************************************************************************/

static struct GfxLayout denjinmk_new_charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 4, 8, 12 },
	{ 3, 2, 1, 0, 16+3, 16+2, 16+1, 16+0 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};


static struct GfxLayout denjinmk_tilelayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 2*4, 3*4, 0*4, 1*4 },
	{ 3, 2, 1, 0, 16+3, 16+2, 16+1, 16+0,
	  64*8+3, 64*8+2, 64*8+1, 64*8+0, 64*8+16+3, 64*8+16+2, 64*8+16+1, 64*8+16+0 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
	  8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	128*8
};


static struct GfxLayout denjinmk_spritelayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 2*4, 3*4, 0*4, 1*4 },
	{ 3, 2, 1, 0, 16+3, 16+2, 16+1, 16+0,
	  64*8+3, 64*8+2, 64*8+1, 64*8+0, 64*8+16+3, 64*8+16+2, 64*8+16+1, 64*8+16+0 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
	  8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	128*8
};



static struct GfxDecodeInfo denjinmk_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &denjinmk_new_charlayout,    48*16, 16 },
	{ REGION_GFX3, 0, &denjinmk_tilelayout,    0*16, 16 },
	{ REGION_GFX4, 0, &denjinmk_tilelayout,    32*16, 16 },	/* unused */
	{ REGION_GFX2, 0, &denjinmk_spritelayout,  0*16, 8*16 },
	{ REGION_GFX5, 0, &denjinmk_tilelayout,   32*16, 16 },
	{ REGION_GFX6, 0, &denjinmk_tilelayout,   16*16, 16 },
	{ -1 } /* end of array */
};

/*****************************************************************************/
SEIBU_SOUND_SYSTEM_YM2151_HARDWARE(14318180/4, 8000, REGION_SOUND1);
/*****************************************************************************/

static MACHINE_DRIVER_START( denjinmk )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000)
    MDRV_CPU_MEMORY(denjinmk_readmem,denjinmk_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)

	SEIBU2_SOUND_SYSTEM_CPU(14318180/4)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_INIT(seibu_sound_1)

	/* video hardware */
    MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_BUFFERS_SPRITERAM)
	MDRV_SCREEN_SIZE(42*8, 36*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 0*8, 32*8-1)
	MDRV_GFXDECODE(denjinmk_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(128*16)

	MDRV_VIDEO_START(denjinmk)
	MDRV_VIDEO_UPDATE(denjinmk)

	/* sound hardware */
	SEIBU_SOUND_SYSTEM_YM2151_INTERFACE
MACHINE_DRIVER_END



/*

Denjin Makai
Banpresto, 1994

This game runs on early 90's Seibu hardware.
(i.e. Raiden II, Godzilla, Seibu Cup Soccer, Legionairre, Heated Barrel etc).
The PCB looks to have been converted from some other game (a lot of flux traces are
left on the PCB). The game might be a simple ROM swap for some other Seibu-based game.


PCB Layout
----------

|-----------------------------------------------------|
|LA4460   YM2151   M6295  ROM6  ROM5  Z80       PAL6  |
|                                     6116      PAL9  |
|        SEI0100BU   SEI0220BP  14.31818MHz           |
|                                                     |
|        6116                                         |
|                              OBJ-0-3     OBJ-6-7    |
|J       6116      SEI0211                            |
|A                             OBJ-4-5     OBJ-8-9    |
|M                                                    |
|M                        ROM1     ROM3      62256    |
|M                                           62256    |
|                         ROM2     ROM4      62256    |
|     DSW1   S68E08  PAL7                    62256    |
|     DSW2                PAL6  PAL3                  |
|SEI0200                                              |
|     6264                                            |
|     6264                   COPX-D2                  |
|                                             SEI1000 |
|BG-1-AB            ROM7         PAL2  PAL1           |
|                                                     |
|BG-2-AB   BG-3-AB  ROM8  20MHz  PAL5  68000          |
|-----------------------------------------------------|
Notes:
      68000 clock  : 10.000MHz (20 / 2)
      Z80 clock    : 3.579545MHz (14.31818 / 4)
      YM2151 clock : 3.579545MHz (14.31818 / 4)
      M6295 clock  : 1.000MHz, sample rate = M6295 clock / 132
      VSync        : 56Hz

      62256    : 32K x8 SRAM
      6264     : 8K  x8 SRAM
      6116     : 2K  x8 SRAM
      SEI0200  : Custom Seibu QFP100 also stamped TC110G21AF
      SEI1000  : Custom Seibu QFP184 also stamped SB01-001
      SEI0211  : Custom Seibu QFP128
      SEI0100BU: Custom Seibu SDIP64 also stamped YM3931
      SEI0220BP: Custom Seibu QFP80

      ROMs 1, 2, 3, 4   Main program   27C020 EPROM
      ROM  6            OKI samples    27C020 EPROM
      ROM  5            Sound program  27C512 EPROM
      ROMs 7 and 8      Graphics       27C512 EPROM
      ROMs BG*          Graphics       8M Mask ROM
      ROMs OBJ*         Graphics       8M Mask ROM
      COPX-D2           ?              4M Mask ROM

      PAL1 : type AMI 18CV8PC,    labelled 'S68E01'
      PAL2 : type MMI PAL16L8ACN, labelled 'S68E02'
      PAL3 : type MMI PAL16L8ACN, labelled 'S68E03'
      PAL4 : type AMI 18CV8PC,    labelled 'S68E04'
      PAL5 : type AMI 18CV8PC,    labelled 'S68E05T'
      PAL6 : type AMI 18CV8PC,    labelled 'S68E06T'
      PAL7 : type AMI 18CV8PC,    labelled 'S68E07'
      PAL9 : type MMI PAL16L8ACN, labelled 'S68E09'

      S68E08 : PROM type 82S147, labelled 'S68E08'


*/

ROM_START( denjinmk )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD32_BYTE( "rom1.025",        0x000000, 0x040000, CRC(44a648e8) SHA1(a3c1721e89ac6b9fc16f80682b2f701cb24b5d76) )
	ROM_LOAD32_BYTE( "rom2.024",        0x000001, 0x040000, CRC(e5ee8fe0) SHA1(2ebff4fdbe82062fb526598e10f11358b0b5c02f) )
	ROM_LOAD32_BYTE( "rom3.026",        0x000002, 0x040000, CRC(781b942e) SHA1(f1f4ddc332de3dc29b716a1b82c2ecc2045efb3a) )
	ROM_LOAD32_BYTE( "rom4.023",        0x000003, 0x040000, CRC(502a588b) SHA1(9055b631240fe52d33b572e34275d31a9f3d290f) )

	ROM_REGION( 0x20000*2, REGION_CPU2, 0 )/* Z80 code, banked data */
	ROM_LOAD( "rom5.016",     0x000000, 0x08000, CRC(7fe7e352) SHA1(1ceae22186751ca91dfffab7bd11f275e693451f) )
	ROM_CONTINUE(			  0x010000, 0x08000 )	/* banked stuff */

	ROM_REGION( 0x020000, REGION_GFX1, 0 )
	ROM_LOAD16_BYTE( "rom7.620",       0x000000, 0x010000, CRC(e1f759b1) SHA1(ddc60e78e7791a59c59403dd4089b3f6e1ecf8cb) )
	ROM_LOAD16_BYTE( "rom8.615",       0x000001, 0x010000, CRC(cc36af0d) SHA1(69c2ae38f03be79be4d138fcc73a6a86407eb285) )

	ROM_REGION( 0x500000, REGION_GFX2, 0 )
    ROM_LOAD( "obj-0-3.748",     0x000000, 0x200000, CRC(67c26a67) SHA1(20543ca9dcf3fed0884968b5249b34b59a14b791) ) /* banks 0,1,2,3 */
	ROM_LOAD( "obj-4-5.756",     0x200000, 0x100000, CRC(01f8d4e6) SHA1(25b69da693be8c3404f750b419c330a7a56e88ec) ) /* 4,5 */
	ROM_LOAD( "obj-6-7.743",     0x300000, 0x100000, CRC(e5805757) SHA1(9d392c27eef7c1fcda560dac17ba9d7ae2287ac8) ) /* 6,7 */
	ROM_LOAD( "obj-8-9.757",     0x400000, 0x100000, CRC(c8f7e1c9) SHA1(a746d187b50a0ecdd5a7f687a2601e5dc8bfe272) ) /* 8,9 */

	ROM_REGION( 0x100000, REGION_GFX3, 0 )	/* MBK tiles */
	ROM_LOAD( "bg-1-ab.618",      0x000000, 0x100000, CRC(eaad151a) SHA1(bdd1d83ee8497efe20f21baf873e786446372bcb) )

	ROM_REGION( 0x100000, REGION_GFX4, 0 )	/* BK2 used */
	ROM_LOAD( "bg-2-ab.617",      0x000000, 0x100000, CRC(40938f74) SHA1(d68b0f8245a8b390ad5d4e6ebc7514a939b8ac51) )

	ROM_REGION( 0x100000, REGION_GFX5, 0 )	/* BK3 tiles */
	ROM_LOAD( "bg-3-ab.619",      0x000000, 0x100000,  CRC(de7366ee) SHA1(0c3969d15f3cd963e579d4164b6e0a6b4012c9c6) )

	ROM_REGION( 0x100000, REGION_GFX6, 0 )	/* LBK tiles */
    ROM_COPY( REGION_GFX4, 0x00000, 0x00000, 0x100000 )

	ROM_REGION( 0x080000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "rom6.922",      0x000000, 0x040000, CRC(09e13213) SHA1(9500e057104c6b83da0467938e46d9efa2f49f4c) )

	ROM_REGION( 0x200, REGION_PROMS, 0 )
	ROM_LOAD( "s68e08.844",   0x000000, 0x000200, CRC(96f7646e) SHA1(400a831b83d6ac4d2a46ef95b97b1ee237099e44) ) /* Priority */

	ROM_REGION( 0x080000, REGION_USER1, 0 )
	ROM_LOAD( "copx-d2.313",  0x000000, 0x080000, CRC(7c52581b) SHA1(7e668476f886806b0c06fa0bcf4bbc955878c87c) )
ROM_END



static DRIVER_INIT( denjinmk )
{
	/* problem with audio comms? */
	UINT16 *ROM = (UINT16 *)memory_region(REGION_CPU1);
	ROM[0x5fe4/2] = 0x4e71;
}



GAMEX(1994,  denjinmk,  0,  denjinmk,  denjinmk,  denjinmk,  ROT0,  "Banpresto",  "Denjin Makai", GAME_IMPERFECT_GRAPHICS )
