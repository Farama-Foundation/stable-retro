/*

Steel Force

ELECTRONIC DEVICES 1994 Miland Italy"
ECOGAMES S.L. Barcelona, Spain"

driver by David Haywood
inputs etc. by Stephane Humbert

----------------------------------------

68000P12 processor
15mHZ cyrstal next to it

2 of these:

TPC 1020AFN-084c

32MHz crystal colse to this.

1 GAL
5 PROMS  (16S25H)

27c4001
u1, u27, u28, u29, u30

27c2001
u31,u32, u33, u34

27c010
u104, u105

----------------------------------------

notes:

lev 1 : 0x64 : 0000 0100 - just rte
lev 2 : 0x68 : 0000 0100 - just rte
lev 3 : 0x6c : 0000 0100 - just rte
lev 4 : 0x70 : 0000 CBD6 - vblank?
lev 5 : 0x74 : 0000 0100 - just rte
lev 6 : 0x78 : 0000 0100 - just rte
lev 7 : 0x7c : 0000 0100 - just rte


  2002.02.03 : There doesn't seem to be Dip Switches
               (you make the changes in the "test mode")
               Bits 8 to 15 of IN1 seem to be unused
               The 2nd part of the "test mode" ("sound and video") is in Spanish/Italian
               (I can't tell for the moment)
               Release date and manufacturers according to the title screen

TO DO :

  - verify the player inputs in the 2nd part of the "test mode"
    (once the colors are correct, as you can't see anything for the moment)
  - what do bits 4 and 6 of IN1 really do ?
      * bit 4 must be On (freeze, vblank ?)
      * bit 6 must be Off during P.O.S.T. (eeprom read ?)
  - check the writes to 0x400010 and 0x400011

  - sprite colours, I think they're wrong
  - fix sound (is the sound chip even right, the readme doesn't mention it ..)
  - unknown registers
  - scrolling during attract mode
  - clipping issues?
  - priority issues?

*/

#include "driver.h"

data16_t *stlforce_bg_videoram, *stlforce_mlow_videoram, *stlforce_mhigh_videoram, *stlforce_tx_videoram;
data16_t *stlforce_bg_scrollram, *stlforce_mlow_scrollram, *stlforce_mhigh_scrollram, *stlforce_vidattrram;
data16_t *stlforce_spriteram;

VIDEO_START( stlforce );
VIDEO_UPDATE( stlforce );
WRITE16_HANDLER( stlforce_tx_videoram_w );
WRITE16_HANDLER( stlforce_mhigh_videoram_w );
WRITE16_HANDLER( stlforce_mlow_videoram_w );
WRITE16_HANDLER( stlforce_bg_videoram_w );

static WRITE16_HANDLER( oki_bank_w )
{
	OKIM6295_set_bank_base(0, 0x40000 * ((data>>8) & 3));
}

static MEMORY_READ16_START( stlforce_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM }, /* rom */
	{ 0x100000, 0x1007ff, MRA16_RAM }, /* bg ram */
	{ 0x100800, 0x100fff, MRA16_RAM }, /* mlow ram */
	{ 0x101000, 0x1017ff, MRA16_RAM }, /* mhigh ram */
	{ 0x101800, 0x1027ff, MRA16_RAM }, /* tx ram */
	{ 0x102800, 0x103fff, MRA16_RAM }, /* unknown / ram */
	{ 0x104000, 0x104fff, MRA16_RAM }, /* palette */
	{ 0x105000, 0x107fff, MRA16_RAM }, /* unknown / ram */
	{ 0x108000, 0x108fff, MRA16_RAM }, /* sprite ram? */
	{ 0x109000, 0x11ffff, MRA16_RAM }, /* unknown / ram */
	{ 0x400000, 0x400001, input_port_0_word_r },
	{ 0x400002, 0x400003, input_port_1_word_r },
	{ 0x410000, 0x410001, OKIM6295_status_0_lsb_r },
MEMORY_END

static MEMORY_WRITE16_START( stlforce_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x100000, 0x1007ff, stlforce_bg_videoram_w, &stlforce_bg_videoram },
	{ 0x100800, 0x100fff, stlforce_mlow_videoram_w, &stlforce_mlow_videoram },
	{ 0x101000, 0x1017ff, stlforce_mhigh_videoram_w, &stlforce_mhigh_videoram },
	{ 0x101800, 0x1027ff, stlforce_tx_videoram_w, &stlforce_tx_videoram },
	{ 0x102800, 0x102fff, MWA16_RAM }, /* unknown / ram */
	{ 0x103000, 0x1033ff, MWA16_RAM, &stlforce_bg_scrollram }, /* unknown / ram */
	{ 0x103400, 0x1037ff, MWA16_RAM, &stlforce_mlow_scrollram }, /* unknown / ram */
	{ 0x103800, 0x103bff, MWA16_RAM, &stlforce_mhigh_scrollram }, /* unknown / ram */
	{ 0x103c00, 0x103fff, MWA16_RAM, &stlforce_vidattrram }, /* unknown / ram */
	{ 0x104000, 0x104fff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
	{ 0x105000, 0x107fff, MWA16_RAM }, /* unknown / ram */
	{ 0x108000, 0x108fff, MWA16_RAM, &stlforce_spriteram }, /* or is this not sprite ram .. */
	{ 0x109000, 0x11ffff, MWA16_RAM },
/*	{ 0x400010, 0x400013, MWA16_NOP },*/
/*	{ 0x40001E, 0x40001F, MWA16_NOP },*/
	{ 0x400012, 0x400013, oki_bank_w },
	{ 0x410000, 0x410001, OKIM6295_data_0_lsb_w },
MEMORY_END

INPUT_PORTS_START( stlforce )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_SERVICE( 0x0008, IP_ACTIVE_LOW )
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH,IPT_UNKNOWN )		/* To be confirmed (see notes)*/
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNUSED )		/* To be confirmed (see notes)*/
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

static struct GfxLayout stlforce_bglayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{0,1,2,3},
	{12,8,4,0,28,24,20,16,16*32+12,16*32+8,16*32+4,16*32+0,16*32+28,16*32+24,16*32+20,16*32+16},
	{0*32,1*32,2*32,3*32,4*32,5*32,6*32,7*32,8*32,9*32,10*32,11*32,12*32,13*32,14*32,15*32},
	32*32
};

static struct GfxLayout stlforce_txlayout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{0,1,2,3},
	{12,8,4,0,28,24,20,16},
	{0*32,1*32,2*32,3*32,4*32,5*32,6*32,7*32,},
	8*32
};

static struct GfxLayout stlforce_splayout =
{
	16,16,
	RGN_FRAC(1,4),
	4,
	{0x040000*3*8,0x040000*2*8,0x040000*1*8,0x040000*0*8},
	{16*8+7,16*8+6,16*8+5,16*8+4,16*8+3,16*8+2,16*8+1,16*8+0,7,6,5,4,3,2,1,0},
	{0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8,8*8,9*8,10*8,11*8,12*8,13*8,14*8,15*8},
	32*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &stlforce_bglayout, 0, 256  },
	{ REGION_GFX1, 0, &stlforce_txlayout, 0, 256  },
	{ REGION_GFX2, 0, &stlforce_splayout, 0, 256  },
	{ -1 } /* end of array */
};

static struct OKIM6295interface okim6295_interface =
{
	1,				/* 1 chip */
	{ 937500 / 132 },	/* frequency (Hz) */
	{ REGION_SOUND1 },	/* memory region */
	{ 75 }
};

static MACHINE_DRIVER_START( stlforce )
	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000) /* guess ... it might be 15000000 or 12000000/2 ... */
	MDRV_CPU_MEMORY(stlforce_readmem,stlforce_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */

	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(1*8, 47*8-1, 1*8, 30*8-1)
	MDRV_PALETTE_LENGTH(0x800)
	MDRV_VIDEO_START(stlforce)
	MDRV_VIDEO_UPDATE(stlforce)

	/* sound hardware */
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END

ROM_START( stlforce )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "stlforce.105", 0x00000, 0x20000, CRC(3ec804ca) SHA1(4efcf3321b7111644ac3ee0a83ad95d0571a4021) )
	ROM_LOAD16_BYTE( "stlforce.104", 0x00001, 0x20000, CRC(69b5f429) SHA1(5bd20fad91a22f4d62f85a5190d72dd824ee26a5) )

	ROM_REGION( 0x200000, REGION_GFX1, 0 ) /* 16x16 bg tiles & 8x8 tx tiles merged */
	ROM_LOAD16_BYTE( "stlforce.u27", 0x000001, 0x080000, CRC(c42ef365) SHA1(40e9ee29ea14b3bc2fbfa4e6acb7d680cf72f01a) )
	ROM_LOAD16_BYTE( "stlforce.u28", 0x000000, 0x080000, CRC(6a4b7c98) SHA1(004d7f3c703c6abc79286fa58a4c6793d66fca39) )
	ROM_LOAD16_BYTE( "stlforce.u29", 0x100001, 0x080000, CRC(30488f44) SHA1(af0d92d8952ce3cd893ab9569afdda12e17795e7) )
	ROM_LOAD16_BYTE( "stlforce.u30", 0x100000, 0x080000, CRC(cf19d43a) SHA1(dc04930548ac5b7e2b74c6041325eac06e773ed5) )

	ROM_REGION( 0x100000, REGION_GFX2, 0 ) /* 16x16 sprites */
	ROM_LOAD( "stlforce.u31", 0x40000, 0x40000, CRC(305a8eb5) SHA1(3a8d26f8bc4ec2e8246d1c59115e21cad876630d) )
	ROM_LOAD( "stlforce.u32", 0x80000, 0x40000, CRC(760e8601) SHA1(a61f1d8566e09ce811382c6e23f3881e6c438f15) )
	ROM_LOAD( "stlforce.u33", 0xc0000, 0x40000, CRC(19415cf3) SHA1(31490a1f3321558f82667b63f3963b2ec3fa0c59) )
	ROM_LOAD( "stlforce.u36", 0x00000, 0x40000, CRC(037dfa9f) SHA1(224f5cd1a95d55b065aef5c0bd03b50cabcb619b) )
	
	/* only one bank */
	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* samples */
	ROM_LOAD( "stlforce.u1", 0x00000, 0x80000, CRC(0a55edf1) SHA1(091f12e8110c62df22b370a2e710c930ba06e8ca) )
ROM_END

GAME(1994, stlforce, 0, stlforce, stlforce, 0, ROT0, "Electronic Devices Italy / Ecogames S.L. Spain", "Steel Force" )
