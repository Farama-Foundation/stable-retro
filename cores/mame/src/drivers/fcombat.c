/* Field Combat (c)1985 Jaleco */

/* todo:

Very Preliminary WIP

Fix Colours
Fix Sprites
Backgrounds
Unknown Reads / Writes
See if it can be remerged with Exerion (its currently made from bits of it)

*/

/* dump info

Field Combat (c)1985 Jaleco

From a working board.

CPU: Z80 (running at 3.332 MHz measured at pin 6)
Sound: Z80 (running at 3.332 MHz measured at pin 6), YM2149 (x3)
Other: Unmarked 24 pin near ROMs 2 & 3

RAM: 6116 (x3)

X-TAL: 20 MHz


inputs + notes by stephh

*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "exerion.h"

INPUT_PORTS_START( fcombat )
	PORT_START      /* player 1 inputs (muxed on 0xe000) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* player 2 inputs (muxed on 0xe000) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* dip switches (0xe100) */
	PORT_DIPNAME( 0x07, 0x02, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x01, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x03, "4" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_BITX(0,        0x07, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x18, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "10000" )
	PORT_DIPSETTING(    0x08, "20000" )
	PORT_DIPSETTING(    0x10, "30000" )
	PORT_DIPSETTING(    0x18, "40000" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )

	PORT_START      /* dip switches/VBLANK (0xe200) */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )		/* VBLANK */
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )		/* related to vblank*/
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* FAKE */
	/* The coin slots are not memory mapped. */
	/* This fake input port is used by the interrupt */
	/* handler to be notified of coin insertions. We use IMPULSE to */
	/* trigger exactly one interrupt, without having to check when the */
	/* user releases the key. */
	PORT_BIT_IMPULSE( 0x01, IP_ACTIVE_HIGH, IPT_COIN1, 1 )
INPUT_PORTS_END



/* is it protection? */

static READ_HANDLER( fcombat_protection_r )
{
	/* Must match ONE of these values after a "and  $3E" intruction :

		76F0: 1E 04 2E 26 34 32 3A 16 3E 36

	   Check code at 0x76c8 for more infos.
	*/

	return 0xff;	/* seems enough*/
}


/* same as exerion again */

static READ_HANDLER( fcombat_port01_r )
{
	/* the cocktail flip bit muxes between ports 0 and 1 */
	return exerion_cocktail_flip ? input_port_1_r(offset) : input_port_0_r(offset);
}


static READ_HANDLER( fcombat_port3_r )
{
	/* bit 0 is VBLANK, which we simulate manually */
	int result = input_port_3_r(offset);
	int ybeam = cpu_getscanline();
	if (ybeam > Machine->visible_area.max_y)
		result |= 1;
	return result;
}


static MEMORY_READ_START( fcombat_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xe000, 0xe000, fcombat_port01_r },
	{ 0xe100, 0xe100, input_port_2_r },
	{ 0xe200, 0xe200, fcombat_port3_r },
	{ 0xe300, 0xe300, MRA_RAM }, /* unknown - even checked in "demo mode" - affects 0xec00 and 0xed00*/
	{ 0xe400, 0xe400, fcombat_protection_r }, /* protection?*/
	{ 0xc000, 0xc7ff, MRA_RAM }, /* ram?*/
	{ 0xd000, 0xd7ff, MRA_RAM }, /* bgs?*/
	{ 0xd800, 0xd87f, MRA_RAM }, /* sprites?*/
	{ 0xd880, 0xd8ff, MRA_RAM }, /* something else ..*/
MEMORY_END

static MEMORY_WRITE_START( fcombat_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xd000, 0xd7ff, videoram_w, &videoram, &videoram_size },
	{ 0xd800, 0xd87f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xd880, 0xd8ff, MWA_RAM },

	{ 0xe800, 0xe800, exerion_videoreg_w },	/* at least bit 0 for flip screen and joystick input multiplexor*/

	{ 0xe900, 0xe900, MWA_RAM },	/* video ?*/
	{ 0xea00, 0xea00, MWA_RAM },	/* video ?*/
	{ 0xeb00, 0xeb00, MWA_RAM },	/* video ?*/

	{ 0xec00, 0xec00, MWA_RAM },	/* affected by read at 0xe300*/
	{ 0xed00, 0xed00, MWA_RAM },	/* affected by read at 0xe300*/

	{ 0xe300, 0xe300, MWA_RAM },	/* for debug purpose*/

	{ 0xee00, 0xee00, MWA_RAM },	/* related to protection ? - doesn't seem to have any effect*/

	/* erk ... */

	{ 0xd880, 0xd8ff, MWA_RAM },
	{ 0xef00, 0xef00, soundlatch_w },
MEMORY_END

/* sound cpu */

static MEMORY_READ_START( fcombat_readmem2 )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x47ff, MRA_RAM },
	{ 0x6000, 0x6000, soundlatch_r },
	{ 0x8001, 0x8001, AY8910_read_port_0_r },
	{ 0xa001, 0xa001, AY8910_read_port_1_r },
	{ 0xc001, 0xc001, AY8910_read_port_2_r },
MEMORY_END

static MEMORY_WRITE_START( fcombat_writemem2 )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x47ff, MWA_RAM },
	{ 0x8002, 0x8002, AY8910_write_port_0_w },
	{ 0x8003, 0x8003, AY8910_control_port_0_w },
	{ 0xa002, 0xa002, AY8910_write_port_1_w },
	{ 0xa003, 0xa003, AY8910_control_port_1_w },
	{ 0xc002, 0xc002, AY8910_write_port_2_w },
	{ 0xc003, 0xc003, AY8910_control_port_2_w },
MEMORY_END

/*************************************
 *
 *	Graphics layouts
 *
 *************************************/

static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	2,
	{ 0, 4 },
	{ 3, 2, 1, 0, 8+3, 8+2, 8+1, 8+0 },
	{ 16*0, 16*1, 16*2, 16*3, 16*4, 16*5, 16*6, 16*7 },
	16*8
};


/* 16 x 16 sprites -- requires reorganizing characters in init_exerion() */
static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,1),
	2,
	{ 0, 4 },
	{  3, 2, 1, 0, 8+3, 8+2, 8+1, 8+0,
			16+3, 16+2, 16+1, 16+0, 24+3, 24+2, 24+1, 24+0 },
	{ 32*0, 32*1, 32*2, 32*3, 32*4, 32*5, 32*6, 32*7,
			32*8, 32*9, 32*10, 32*11, 32*12, 32*13, 32*14, 32*15 },
	64*8
};


/* Quick and dirty way to emulate pixel-doubled sprites. */
static struct GfxLayout bigspritelayout =
{
	32,32,
	RGN_FRAC(1,1),
	2,
	{ 0, 4 },
	{  3, 3, 2, 2, 1, 1, 0, 0,
			8+3, 8+3, 8+2, 8+2, 8+1, 8+1, 8+0, 8+0,
			16+3, 16+3, 16+2, 16+2, 16+1, 16+1, 16+0, 16+0,
			24+3, 24+3, 24+2, 24+2, 24+1, 24+1, 24+0, 24+0 },
	{ 32*0, 32*0, 32*1, 32*1, 32*2, 32*2, 32*3, 32*3,
			32*4, 32*4, 32*5, 32*5, 32*6, 32*6, 32*7, 32*7,
			32*8, 32*8, 32*9, 32*9, 32*10, 32*10, 32*11, 32*11,
			32*12, 32*12, 32*13, 32*13, 32*14, 32*14, 32*15, 32*15 },
	64*8
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,         0, 64 },
	{ REGION_GFX2, 0, &spritelayout,     256, 64 },
	{ REGION_GFX2, 0, &bigspritelayout,  256, 64 },
	{ -1 }
};



/*************************************
 *
 *	Sound interfaces
 *
 *************************************/

static struct AY8910interface ay8910_interface =
{
	3,	/* 3 chips */
	1500000,	/* 1.5 MHz?????? */
	{ 12, 12, 12 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

/* interrupt */


static INTERRUPT_GEN( fcombat_interrupt )
{
	/* Exerion triggers NMIs on coin insertion */
	if (readinputport(4) & 1)
		cpu_set_irq_line(0, IRQ_LINE_NMI, PULSE_LINE);
}

/*************************************
 *
 *	Machine drivers
 *
 *************************************/

static MACHINE_DRIVER_START( fcombat )

	MDRV_CPU_ADD(Z80, 10000000/3)
	MDRV_CPU_MEMORY(fcombat_readmem,fcombat_writemem)
	MDRV_CPU_VBLANK_INT(fcombat_interrupt,1)

	MDRV_CPU_ADD(Z80, 10000000/3)
	MDRV_CPU_MEMORY(fcombat_readmem2,fcombat_writemem2)

	MDRV_FRAMES_PER_SECOND(60)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(12*8, 52*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32)
	MDRV_COLORTABLE_LENGTH(256*3)

	MDRV_PALETTE_INIT(exerion)
	MDRV_VIDEO_START(exerion)
	MDRV_VIDEO_UPDATE(exerion)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END

/*************************************
 *
 *	Driver initialization
 *
 *************************************/

static DRIVER_INIT( fcombat )
{
	UINT32 oldaddr, newaddr, length;
	UINT8 *src, *dst, *temp;

	/* allocate some temporary space */
	temp = malloc(0x10000);
	if (!temp)
		return;

	/* make a temporary copy of the character data */
	src = temp;
	dst = memory_region(REGION_GFX1);
	length = memory_region_length(REGION_GFX1);
	memcpy(src, dst, length);

	/* decode the characters */
	/* the bits in the ROM are ordered: n8-n7 n6 n5 n4-v2 v1 v0 n3-n2 n1 n0 h2 */
	/* we want them ordered like this:  n8-n7 n6 n5 n4-n3 n2 n1 n0-v2 v1 v0 h2 */
	for (oldaddr = 0; oldaddr < length; oldaddr++)
	{
		newaddr = ((oldaddr     ) & 0x1f00) |       /* keep n8-n4 */
		          ((oldaddr << 3) & 0x00f0) |       /* move n3-n0 */
		          ((oldaddr >> 4) & 0x000e) |       /* move v2-v0 */
		          ((oldaddr     ) & 0x0001);        /* keep h2 */
		dst[newaddr] = src[oldaddr];
	}

	/* make a temporary copy of the sprite data */
	src = temp;
	dst = memory_region(REGION_GFX2);
	length = memory_region_length(REGION_GFX2);
	memcpy(src, dst, length);

	/* decode the sprites */
	/* the bits in the ROMs are ordered: n9 n8 n3 n7-n6 n5 n4 v3-v2 v1 v0 n2-n1 n0 h3 h2 */
	/* we want them ordered like this:   n9 n8 n7 n6-n5 n4 n3 n2-n1 n0 v3 v2-v1 v0 h3 h2 */
	for (oldaddr = 0; oldaddr < length; oldaddr++)
	{
		newaddr = ((oldaddr << 1) & 0x3c00) |       /* move n7-n4 */
		          ((oldaddr >> 4) & 0x0200) |       /* move n3 */
		          ((oldaddr << 4) & 0x01c0) |       /* move n2-n0 */
		          ((oldaddr >> 3) & 0x003c) |       /* move v3-v0 */
		          ((oldaddr     ) & 0xc003);        /* keep n9-n8 h3-h2 */
		dst[newaddr] = src[oldaddr];
	}

	free(temp);
}

ROM_START( fcombat )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "fcombat2.t9",  0x0000, 0x4000, CRC(30cb0c14) SHA1(8b5b6a4efaca2f138709184725e9e0e0b9cfc4c7) )
	ROM_LOAD( "fcombat3.10t", 0x4000, 0x4000, CRC(e8511da0) SHA1(bab5c9244c970b97c025381c37ad372aa3b5cddf) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for the second CPU */
	ROM_LOAD( "fcombat1.t5",  0x0000, 0x4000, CRC(a0cc1216) SHA1(3a8963ffde2ff4a3f428369133f94bb37717cae5) )

	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "fcombat7.l11", 0x00000, 0x2000, BAD_DUMP CRC(54e978ef) SHA1(834f428f8d3e6b2cd865db8d2a0e069484a98316) ) /* fg chars */

	ROM_REGION( 0x0c000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "fcombat8.d10", 0x00000, 0x4000, CRC(e810941e) SHA1(19ae85af0bf245caf3afe10d65e618cfb47d33c2) ) /* sprites */
	ROM_LOAD( "fcombat9.d11", 0x04000, 0x4000, CRC(f95988e6) SHA1(25876652decca7ec1e9b37a16536c15ca2d1cb12) )
	ROM_LOAD( "fcomba10.d12", 0x08000, 0x4000, CRC(908f154c) SHA1(b3761ee60d4a5ea36376759875105d23c57b4bf2) )

	ROM_REGION( 0x0c000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "fcombat4.p3",  0x00000, 0x4000, CRC(efe098ab) SHA1(fe64a5e9170835d242368109b1b221b0f8090e7e) ) /* bg data */
	ROM_LOAD( "fcombat5.l3",  0x04000, 0x4000, CRC(96194ca7) SHA1(087d6ac8f93f087cb5e378dbe9a8cfcffa2cdddc) )
	ROM_LOAD( "fcombat6.f3",  0x08000, 0x4000, CRC(97282729) SHA1(72db0593551c2d15631341bf621b96013b46ce72) )

	ROM_REGION( 0x0420, REGION_PROMS, 0 )
	ROM_LOAD( "fcprom_a.c2",  0x0000, 0x0020, CRC(7ac480f0) SHA1(f491fe4da19d8c037e3733a5836de35cc438907e) ) /* palette */
	ROM_LOAD( "fcprom_d.k12", 0x0020, 0x0100, CRC(9a348250) SHA1(faf8db4c42adee07795d06bea20704f8c51090ff) ) /* fg char lookup table */
	ROM_LOAD( "fcprom_b.c4",  0x0120, 0x0100, CRC(ac9049f6) SHA1(57aa5b5df3e181bad76149745a422c3dd1edad49) ) /* sprite lookup table */
/*	ROM_LOAD( "exerion.i3",   0x0220, 0x0100, CRC(fe72ab79) )  // bg char lookup table /*/
	ROM_LOAD( "fcprom_c.a9",  0x0320, 0x0100, CRC(768ac120) SHA1(ceede1d6cbeae08da96ef52bdca2718a839d88ab) ) /* bg char mixer */
ROM_END

GAMEX( 1985, fcombat,  0,       fcombat, fcombat, fcombat,  ROT90, "Jaleco", "Field Combat", GAME_NOT_WORKING )
