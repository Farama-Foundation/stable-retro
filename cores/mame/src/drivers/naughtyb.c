/***************************************************************************

Naughty Boy driver by Sal and John Bugliarisi.
This driver is based largely on MAME's Phoenix driver, since Naughty Boy runs
on similar hardware as Phoenix. Phoenix driver provided by Brad Oliver.
Thanks to Richard Davies for his Phoenix emulator source.


Naughty Boy memory map

0000-3fff 16Kb Program ROM
4000-7fff 1Kb Work RAM (mirrored)
8000-87ff 2Kb Video RAM Charset A (lower priority, mirrored)
8800-8fff 2Kb Video RAM Charset b (higher priority, mirrored)
9000-97ff 2Kb Video Control write-only (mirrored)
9800-9fff 2Kb Video Scroll Register (mirrored)
a000-a7ff 2Kb Sound Control A (mirrored)
a800-afff 2Kb Sound Control B (mirrored)
b000-b7ff 2Kb 8bit Game Control read-only (mirrored)
b800-bfff 1Kb 8bit Dip Switch read-only (mirrored)
c000-0000 16Kb Unused

memory mapped ports:

read-only:
b000-b7ff IN
b800-bfff DSW


Naughty Boy Switch Settings
(C)1982 Cinematronics

 --------------------------------------------------------
|Option |Factory|Descrpt| 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
 ------------------------|-------------------------------
|Lives	|		|2		|on |on |	|	|	|	|	|	|
 ------------------------ -------------------------------
|		|	X	|3		|off|on |	|	|	|	|	|	|
 ------------------------ -------------------------------
|		|		|4		|on |off|	|	|	|	|	|	|
 ------------------------ -------------------------------
|		|		|5		|off|off|	|	|	|	|	|	|
 ------------------------ -------------------------------
|Extra	|		|10000	|	|	|on |on |	|	|	|	|
 ------------------------ -------------------------------
|		|	X	|30000	|	|	|off|on |	|	|	|	|
 ------------------------ -------------------------------
|		|		|50000	|	|	|on |off|	|	|	|	|
 ------------------------ -------------------------------
|		|		|70000	|	|	|off|off|	|	|	|	|
 ------------------------ -------------------------------
|Credits|		|2c, 1p |	|	|	|	|on |on |	|	|
 ------------------------ -------------------------------
|		|	X	|1c, 1p |	|	|	|	|off|on |	|	|
 ------------------------ -------------------------------
|		|		|1c, 2p |	|	|	|	|on |off|	|	|
 ------------------------ -------------------------------
|		|		|4c, 3p |	|	|	|	|off|off|	|	|
 ------------------------ -------------------------------
|Dffclty|	X	|Easier |	|	|	|	|	|	|on |	|
 ------------------------ -------------------------------
|		|		|Harder |	|	|	|	|	|	|off|	|
 ------------------------ -------------------------------
| Type	|		|Upright|	|	|	|	|	|	|	|on |
 ------------------------ -------------------------------
|		|		|Cktail |	|	|	|	|	|	|	|off|
 ------------------------ -------------------------------

*
* Pop Flamer
*

Pop Flamer appears to run on identical hardware as Naughty Boy.
The dipswitches are even identical. Spooky.

						1	2	3	4	5	6	7	8
-------------------------------------------------------
Number of Mr. Mouse 2 |ON |ON |   |   |   |   |   |   |
					3 |OFF|ON |   |   |   |   |   |   |
					4 |ON |OFF|   |   |   |   |   |   |
					5 |OFF|OFF|   |   |   |   |   |   |
-------------------------------------------------------
Extra Mouse    10,000 |   |   |ON |ON |   |   |   |   |
			   30,000 |   |   |OFF|ON |   |   |   |   |
			   50,000 |   |   |ON |OFF|   |   |   |   |
			   70,000 |   |   |OFF|OFF|   |   |   |   |
-------------------------------------------------------
Credit	2 coin 1 play |   |   |   |   |ON |ON |   |   |
		1 coin 1 play |   |   |   |   |OFF|ON |   |   |
		1 coin 2 play |   |   |   |   |ON |OFF|   |   |
		1 coin 3 play |   |   |   |   |OFF|OFF|   |   |
-------------------------------------------------------
Skill		   Easier |   |   |   |   |   |   |ON |   |
			   Harder |   |   |   |   |   |   |OFF|   |
-------------------------------------------------------
Game style		Table |   |   |   |   |   |   |   |OFF|
			  Upright |   |   |   |   |   |   |   |ON |


TODO:
	* sounds are a little skanky
	* Figure out how cocktail/upright mode works

 ***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



extern unsigned char *naughtyb_videoram2;
extern unsigned char *naughtyb_scrollreg;

WRITE_HANDLER( naughtyb_videoram2_w );
WRITE_HANDLER( naughtyb_scrollreg_w );
WRITE_HANDLER( naughtyb_videoreg_w );
WRITE_HANDLER( popflame_videoreg_w );
VIDEO_START( naughtyb );
PALETTE_INIT( naughtyb );
VIDEO_UPDATE( naughtyb );

WRITE_HANDLER( pleiads_sound_control_a_w );
WRITE_HANDLER( pleiads_sound_control_b_w );
int naughtyb_sh_start(const struct MachineSound *msound);
int popflame_sh_start(const struct MachineSound *msound);
void pleiads_sh_stop(void);
void pleiads_sh_update(void);


/* Pop Flamer
   1st protection relies on reading values from a device at $9000 and writing to 400A-400D (See $26A9).
   Then value stored in 400C must be xxxx1001 (rrca x 3) or else reset
   2nd protection relies on the values stored in 400A-400D matching $2690+($400E) (Starts at $460)
   If the values all match then it will jump to 0x0011 instead of 0x0009 (refresh instead of reset)
   Paul Priest: tourniquet@mameworld.net */

/*static int popflame_prot_count = 0;*/

READ_HANDLER( popflame_protection_r ) /* Not used by bootleg/hack */
{
	static int values[4] = { 0x78, 0x68, 0x48, 0x38|0x80 };
	static int count;

	count = (count + 1) % 4;
	return values[count];

#if 0
	if ( activecpu_get_pc() == (0x26F2 + 0x03) )
	{
		popflame_prot_count = 0;
		return 0x01;
	} /* Must not carry when rotated left */

	if ( activecpu_get_pc() == (0x26F9 + 0x03) )
		return 0x80; /* Must carry when rotated left */

	if ( activecpu_get_pc() == (0x270F + 0x03) )
	{
		switch( popflame_prot_count++ )
		{
			case 0: return 0x78; /* x111 1xxx, matches 0x0F at $2690, stored in $400A */
			case 1: return 0x68; /* x110 1xxx, matches 0x0D at $2691, stored in $400B */
			case 2: return 0x48; /* x100 1xxx, matches 0x09 at $2692, stored in $400C */
			case 3: return 0x38; /* x011 1xxx, matches 0x07 at $2693, stored in $400D */
		}
	}
	log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #0 PC %06x: unmapped protection read\n", activecpu_get_pc());
	return 0x00;
#endif
}



static MEMORY_READ_START( readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x8fff, MRA_RAM },
	{ 0xb000, 0xb7ff, input_port_0_r }, 	/* IN0 */
	{ 0xb800, 0xbfff, input_port_1_r }, 	/* DSW */
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x7fff, MWA_RAM },
	{ 0x8000, 0x87ff, videoram_w, &videoram, &videoram_size },
	{ 0x8800, 0x8fff, naughtyb_videoram2_w, &naughtyb_videoram2 },
	{ 0x9000, 0x97ff, naughtyb_videoreg_w },
	{ 0x9800, 0x9fff, MWA_RAM, &naughtyb_scrollreg },
	{ 0xa000, 0xa7ff, pleiads_sound_control_a_w },
	{ 0xa800, 0xafff, pleiads_sound_control_b_w },
MEMORY_END

static MEMORY_WRITE_START( popflame_writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x7fff, MWA_RAM },
	{ 0x8000, 0x87ff, videoram_w, &videoram, &videoram_size },
	{ 0x8800, 0x8fff, naughtyb_videoram2_w, &naughtyb_videoram2 },
	{ 0x9000, 0x97ff, popflame_videoreg_w },
	{ 0x9800, 0x9fff, MWA_RAM, &naughtyb_scrollreg },
	{ 0xa000, 0xa7ff, pleiads_sound_control_a_w },
	{ 0xa800, 0xafff, pleiads_sound_control_b_w },
MEMORY_END



/***************************************************************************

  Naughty Boy doesn't have VBlank interrupts.
  Interrupts are still used by the game: but they are related to coin
  slots.

***************************************************************************/

INTERRUPT_GEN( naughtyb_interrupt )
{
	if (readinputport(2) & 1)
		cpu_set_irq_line(0, IRQ_LINE_NMI, PULSE_LINE);
}

INPUT_PORTS_START( naughtyb )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )

	PORT_START	/* DSW0 & VBLANK */
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x00, "2" )
	PORT_DIPSETTING(	0x01, "3" )
	PORT_DIPSETTING(	0x02, "4" )
	PORT_DIPSETTING(	0x03, "5" )
	PORT_DIPNAME( 0x0c, 0x04, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x00, "10000" )
	PORT_DIPSETTING(	0x04, "30000" )
	PORT_DIPSETTING(	0x08, "50000" )
	PORT_DIPSETTING(	0x0c, "70000" )
	PORT_DIPNAME( 0x30, 0x10, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x10, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x20, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x30, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x00, "Easy" )
	PORT_DIPSETTING(	0x40, "Hard" )
	/* This is a bit of a mystery. Bit 0x80 is read as the vblank, but
	   it apparently also controls cocktail/table mode. */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )

	PORT_START	/* FAKE */
	/* The coin slots are not memory mapped. */
	/* This fake input port is used by the interrupt */
	/* handler to be notified of coin insertions. We use IMPULSE to */
	/* trigger exactly one interrupt, without having to check when the */
	/* user releases the key. */
		PORT_BIT_IMPULSE( 0x01, IP_ACTIVE_HIGH, IPT_COIN1, 1 )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters */
	2,		/* 2 bits per pixel */
	{ 512*8*8, 0 }, /* the two bitplanes are separated */
	{ 7, 6, 5, 4, 3, 2, 1, 0 }, /* pretty straightforward layout */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8 	/* every char takes 8 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,	  0, 32 },
	{ REGION_GFX2, 0, &charlayout, 32*4, 32 },
	{ -1 } /* end of array */
};



static struct CustomSound_interface naughtyb_custom_interface =
{
	naughtyb_sh_start,
	pleiads_sh_stop,
	pleiads_sh_update
};

static struct CustomSound_interface popflame_custom_interface =
{
	popflame_sh_start,
	pleiads_sh_stop,
	pleiads_sh_update
};

static struct TMS36XXinterface tms3615_interface =
{
	1,
	{ 60		},	/* mixing level */
	{ TMS3615	},	/* TMS36xx subtype */
	{ 350		},	/* base clock (one octave below A) */
	/*
	 * Decay times of the voices; NOTE: it's unknown if
	 * the the TMS3615 mixes more than one voice internally.
	 * A wav taken from Pop Flamer sounds like there
	 * are at least no 'odd' harmonics (5 1/3' and 2 2/3')
	 */
	{ {0.15,0.20,0,0,0,0} }
};



static MACHINE_DRIVER_START( naughtyb )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 1500000)	/* 3 MHz ? */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(naughtyb_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(36*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 36*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)
	MDRV_COLORTABLE_LENGTH(32*4+32*4)

	MDRV_PALETTE_INIT(naughtyb)
	MDRV_VIDEO_START(naughtyb)
	MDRV_VIDEO_UPDATE(naughtyb)

	/* sound hardware */
	/* uses the TMS3615NS for sound */
	MDRV_SOUND_ADD(TMS36XX, tms3615_interface)
	MDRV_SOUND_ADD(CUSTOM, naughtyb_custom_interface)
MACHINE_DRIVER_END


/* Exactly the same but for the writemem handler */
static MACHINE_DRIVER_START( popflame )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 1500000)	/* 3 MHz ? */
	MDRV_CPU_MEMORY(readmem,popflame_writemem)
	MDRV_CPU_VBLANK_INT(naughtyb_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(36*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 36*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)
	MDRV_COLORTABLE_LENGTH(32*4+32*4)

	MDRV_PALETTE_INIT(naughtyb)
	MDRV_VIDEO_START(naughtyb)
	MDRV_VIDEO_UPDATE(naughtyb)

	/* sound hardware */
	MDRV_SOUND_ADD(TMS36XX, tms3615_interface)
	MDRV_SOUND_ADD(CUSTOM, popflame_custom_interface)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( naughtyb )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )		/* 64k for code */
	ROM_LOAD( "1.30",	   0x0000, 0x0800, CRC(f6e1178e) SHA1(5cd428e1f085ff82d7237b3e261b33ff876fd4cb) )
	ROM_LOAD( "2.29",	   0x0800, 0x0800, CRC(b803eb8c) SHA1(c21b781eb329195e36e6fd1d7467bd9b0d9cbc5b) )
	ROM_LOAD( "3.28",	   0x1000, 0x0800, CRC(004d0ba7) SHA1(5c182fa6f65f7caa3459fcc5cdc3b7faa8b34769) )
	ROM_LOAD( "4.27",	   0x1800, 0x0800, CRC(3c7bcac6) SHA1(ef291cd5b2f8a64999dc015e16d3ea479fefaf8f) )
	ROM_LOAD( "5.26",	   0x2000, 0x0800, CRC(ea80f39b) SHA1(f05cc4ca48245053a8b35b594fb4c0c3b19304e0) )
	ROM_LOAD( "6.25",	   0x2800, 0x0800, CRC(66d9f942) SHA1(756b188836e9e9d86f8be59c9505288339b91899) )
	ROM_LOAD( "7.24",	   0x3000, 0x0800, CRC(00caf9be) SHA1(0599b28dfe8dd9c18564202af56ba8f272d7ac54) )
	ROM_LOAD( "8.23",	   0x3800, 0x0800, CRC(17c3b6fb) SHA1(c01c8ae27f5b9be90778f7c459c5ba0dddf443ba) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "15.44",	   0x0000, 0x0800, CRC(d692f9c7) SHA1(3573c518868690b140340d19f88c670026a6696d) )
	ROM_LOAD( "16.43",	   0x0800, 0x0800, CRC(d3ba8b27) SHA1(0ff14b8b983ab75870fb19b64327070ccd0888d6) )
	ROM_LOAD( "13.46",	   0x1000, 0x0800, CRC(c1669cd5) SHA1(9b4370ed54424e3615fa2e4d07cadae37ab8cd10) )
	ROM_LOAD( "14.45",	   0x1800, 0x0800, CRC(eef2c8e5) SHA1(5077c4052342958ee26c25047704c62eed44eb89) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "11.48",	   0x0000, 0x0800, CRC(75ec9710) SHA1(b41606930eff79ccf5bfcad01362251d7bab114a) )
	ROM_LOAD( "12.47",	   0x0800, 0x0800, CRC(ef0706c3) SHA1(0e0b82d29d710d1244384db84344bfba2e867b2e) )
	ROM_LOAD( "9.50",	   0x1000, 0x0800, CRC(8c8db764) SHA1(2641a1b8bc30896293ebd9396e304ce5eb7eb705) )
	ROM_LOAD( "10.49",	   0x1800, 0x0800, CRC(c97c97b9) SHA1(5da7fb378e85b6c9d5ab6e75544f1e64fae9997a) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "6301-1.63", 0x0000, 0x0100, CRC(98ad89a1) SHA1(ddee7dcb003b66fbc7d6d6e90d499ed090c59227) ) /* palette low bits */
	ROM_LOAD( "6301-1.64", 0x0100, 0x0100, CRC(909107d4) SHA1(138ace7845424bc3ca86b0889be634943c8c2d19) ) /* palette high bits */
ROM_END

ROM_START( naughtya )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )		/* 64k for code */
	ROM_LOAD( "91", 	   0x0000, 0x0800, CRC(42b14bc7) SHA1(a5890834105b83f6761a5ea819e94533473f0e44) )
	ROM_LOAD( "92", 	   0x0800, 0x0800, CRC(a24674b4) SHA1(2d93981c2f0dea190745cbc3926b012cfd561ec3) )
	ROM_LOAD( "3.28",	   0x1000, 0x0800, CRC(004d0ba7) SHA1(5c182fa6f65f7caa3459fcc5cdc3b7faa8b34769) )
	ROM_LOAD( "4.27",	   0x1800, 0x0800, CRC(3c7bcac6) SHA1(ef291cd5b2f8a64999dc015e16d3ea479fefaf8f) )
	ROM_LOAD( "95", 	   0x2000, 0x0800, CRC(e282f1b8) SHA1(9eb7b2fed75cd23f3c90e445021f23648503c96f) )
	ROM_LOAD( "96", 	   0x2800, 0x0800, CRC(61178ff2) SHA1(2a7fb894e7fc5ec170d00d24300f1e23307f9687) )
	ROM_LOAD( "97", 	   0x3000, 0x0800, CRC(3cafde88) SHA1(c77f03e81128341522d46056aad77e73c2818069) )
	ROM_LOAD( "8.23",	   0x3800, 0x0800, CRC(17c3b6fb) SHA1(c01c8ae27f5b9be90778f7c459c5ba0dddf443ba) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "15.44",	   0x0000, 0x0800, CRC(d692f9c7) SHA1(3573c518868690b140340d19f88c670026a6696d) )
	ROM_LOAD( "16.43",	   0x0800, 0x0800, CRC(d3ba8b27) SHA1(0ff14b8b983ab75870fb19b64327070ccd0888d6) )
	ROM_LOAD( "13.46",	   0x1000, 0x0800, CRC(c1669cd5) SHA1(9b4370ed54424e3615fa2e4d07cadae37ab8cd10) )
	ROM_LOAD( "14.45",	   0x1800, 0x0800, CRC(eef2c8e5) SHA1(5077c4052342958ee26c25047704c62eed44eb89) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "11.48",	   0x0000, 0x0800, CRC(75ec9710) SHA1(b41606930eff79ccf5bfcad01362251d7bab114a) )
	ROM_LOAD( "12.47",	   0x0800, 0x0800, CRC(ef0706c3) SHA1(0e0b82d29d710d1244384db84344bfba2e867b2e) )
	ROM_LOAD( "9.50",	   0x1000, 0x0800, CRC(8c8db764) SHA1(2641a1b8bc30896293ebd9396e304ce5eb7eb705) )
	ROM_LOAD( "10.49",	   0x1800, 0x0800, CRC(c97c97b9) SHA1(5da7fb378e85b6c9d5ab6e75544f1e64fae9997a) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "6301-1.63", 0x0000, 0x0100, CRC(98ad89a1) SHA1(ddee7dcb003b66fbc7d6d6e90d499ed090c59227) ) /* palette low bits */
	ROM_LOAD( "6301-1.64", 0x0100, 0x0100, CRC(909107d4) SHA1(138ace7845424bc3ca86b0889be634943c8c2d19) ) /* palette high bits */
ROM_END

ROM_START( naughtyc )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )		/* 64k for code */
	ROM_LOAD( "nb1ic30",   0x0000, 0x0800, CRC(3f482fa3) SHA1(5c670ad37be5bed12a65b8b02330525cfe5ae303) )
	ROM_LOAD( "nb2ic29",   0x0800, 0x0800, CRC(7ddea141) SHA1(8a725614b156f1fdb249c2767ddb3f04681e7a3f) )
	ROM_LOAD( "nb3ic28",   0x1000, 0x0800, CRC(8c72a069) SHA1(648df992a5b118d0c48aa20e8621172f50ee5b4c) )
	ROM_LOAD( "nb4ic27",   0x1800, 0x0800, CRC(30feae51) SHA1(fa28942a58c2292147e33747feecad9817c2c8ea) )
	ROM_LOAD( "nb5ic26",   0x2000, 0x0800, CRC(05242fd0) SHA1(3436a18c021643959bd5d475eeb0b8ac6afaec74) )
	ROM_LOAD( "nb6ic25",   0x2800, 0x0800, CRC(7a12ffea) SHA1(4a34d6fcd0b6dc9319424d4122d88744ddc473c7) )
	ROM_LOAD( "nb7ic24",   0x3000, 0x0800, CRC(9cc287df) SHA1(507c551ca8044479e588bd2a3fff600c77ea2255) )
	ROM_LOAD( "nb8ic23",   0x3800, 0x0800, CRC(4d84ff2c) SHA1(66e51116bae787c67c10f282700a94069d7b9fe0) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "15.44",	   0x0000, 0x0800, CRC(d692f9c7) SHA1(3573c518868690b140340d19f88c670026a6696d) )
	ROM_LOAD( "16.43",	   0x0800, 0x0800, CRC(d3ba8b27) SHA1(0ff14b8b983ab75870fb19b64327070ccd0888d6) )
	ROM_LOAD( "13.46",	   0x1000, 0x0800, CRC(c1669cd5) SHA1(9b4370ed54424e3615fa2e4d07cadae37ab8cd10) )
	ROM_LOAD( "14.45",	   0x1800, 0x0800, CRC(eef2c8e5) SHA1(5077c4052342958ee26c25047704c62eed44eb89) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "nb11ic48",  0x0000, 0x0800, CRC(23271a13) SHA1(ba46fe9af0f6b6ab366469b9058d95477620e05c) )
	ROM_LOAD( "12.47",	   0x0800, 0x0800, CRC(ef0706c3) SHA1(0e0b82d29d710d1244384db84344bfba2e867b2e) )
	ROM_LOAD( "nb9ic50",   0x1000, 0x0800, CRC(d6949c27) SHA1(2076e76ef9f8f4c9beb3dfe863608151ffae3f3c) )
	ROM_LOAD( "10.49",	   0x1800, 0x0800, CRC(c97c97b9) SHA1(5da7fb378e85b6c9d5ab6e75544f1e64fae9997a) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "6301-1.63", 0x0000, 0x0100, CRC(98ad89a1) SHA1(ddee7dcb003b66fbc7d6d6e90d499ed090c59227) ) /* palette low bits */
	ROM_LOAD( "6301-1.64", 0x0100, 0x0100, CRC(909107d4) SHA1(138ace7845424bc3ca86b0889be634943c8c2d19) ) /* palette high bits */
ROM_END

ROM_START( popflame )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )		/* 64k for code */
	ROM_LOAD( "ic86.bin",	  0x0000, 0x1000, CRC(06397a4b) SHA1(12ef8aa60033161479ba2239b61a318cbf15b3cf) )
	ROM_LOAD( "ic80.pop",	  0x1000, 0x1000, CRC(b77abf3d) SHA1(8626af8fe7d10c52bea7570dd6237de60607bab6) )
	ROM_LOAD( "ic94.bin",	  0x2000, 0x1000, CRC(ae5248ae) SHA1(39a7feb94d0392a0eeeb506d2f52299151521692) )
	ROM_LOAD( "ic100.pop",	  0x3000, 0x1000, CRC(f9f2343b) SHA1(c019a5d838152417ec76be021d659f884928ef87) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic13.pop",	  0x0000, 0x1000, CRC(2367131e) SHA1(d7c15536e5e51f406f9b874333386251f4d3934f) )
	ROM_LOAD( "ic3.pop",	  0x1000, 0x1000, CRC(deed0a8b) SHA1(1aaa854f5c6ca3847726cb8a2f2f37f3eb4f621b) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ic29.pop",	  0x0000, 0x1000, CRC(7b54f60f) SHA1(9ec979e67313351dd1165ff6cf591aadead30770) )
	ROM_LOAD( "ic38.pop",	  0x1000, 0x1000, CRC(dd2d9601) SHA1(7f495705d6b61c5416e87557a69da7e6457567ac) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "ic53",		  0x0000, 0x0100, CRC(6e66057f) SHA1(084d630f5e2f23e28a1f7839337ef608e086e8c4) ) /* palette low bits */
	ROM_LOAD( "ic54",		  0x0100, 0x0100, CRC(236bc771) SHA1(5c078eecdd9df2fbc791e440f96bc4c79476b211) ) /* palette high bits */
ROM_END

ROM_START( popflama )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "ic86.pop",	  0x0000, 0x1000, CRC(5e32bbdf) SHA1(b75e3125301d05f5fb6bcef85d0028de2ee75fab) )
	ROM_LOAD( "ic80.pop",	  0x1000, 0x1000, CRC(b77abf3d) SHA1(8626af8fe7d10c52bea7570dd6237de60607bab6) )
	ROM_LOAD( "ic94.pop",	  0x2000, 0x1000, CRC(945a3c0f) SHA1(353fce8904d869bbf654b7be99e76cadf325b47d) )
	ROM_LOAD( "ic100.pop",	  0x3000, 0x1000, CRC(f9f2343b) SHA1(c019a5d838152417ec76be021d659f884928ef87) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic13.pop",	  0x0000, 0x1000, CRC(2367131e) SHA1(d7c15536e5e51f406f9b874333386251f4d3934f) )
	ROM_LOAD( "ic3.pop",	  0x1000, 0x1000, CRC(deed0a8b) SHA1(1aaa854f5c6ca3847726cb8a2f2f37f3eb4f621b) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ic29.pop",	  0x0000, 0x1000, CRC(7b54f60f) SHA1(9ec979e67313351dd1165ff6cf591aadead30770) )
	ROM_LOAD( "ic38.pop",	  0x1000, 0x1000, CRC(dd2d9601) SHA1(7f495705d6b61c5416e87557a69da7e6457567ac) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "ic53",		  0x0000, 0x0100, CRC(6e66057f) SHA1(084d630f5e2f23e28a1f7839337ef608e086e8c4) ) /* palette low bits */
	ROM_LOAD( "ic54",		  0x0100, 0x0100, CRC(236bc771) SHA1(5c078eecdd9df2fbc791e440f96bc4c79476b211) ) /* palette high bits */
ROM_END

ROM_START( popflamb )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "popflama.30",	 0x0000, 0x1000, CRC(a9bb0e8a) SHA1(1948be9545401b8163e0f2fa8e962ea66c26d9e0) )
	ROM_LOAD( "popflama.28",	 0x1000, 0x1000, CRC(debe6d03) SHA1(2365c57a0a08563bea31ab150934dcfc1e6eba58) )
	ROM_LOAD( "popflama.26",	 0x2000, 0x1000, CRC(09df0d4d) SHA1(ddc0227035edd11bec045c09c535ad7a375698f1) )
	ROM_LOAD( "popflama.24",	 0x3000, 0x1000, CRC(f399d553) SHA1(c08c496fcb99370c344185af599e2ad57a327bc9) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic13.pop",	  0x0000, 0x1000, CRC(2367131e) SHA1(d7c15536e5e51f406f9b874333386251f4d3934f) )
	ROM_LOAD( "ic3.pop",	  0x1000, 0x1000, CRC(deed0a8b) SHA1(1aaa854f5c6ca3847726cb8a2f2f37f3eb4f621b) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ic29.pop",	  0x0000, 0x1000, CRC(7b54f60f) SHA1(9ec979e67313351dd1165ff6cf591aadead30770) )
	ROM_LOAD( "ic38.pop",	  0x1000, 0x1000, CRC(dd2d9601) SHA1(7f495705d6b61c5416e87557a69da7e6457567ac) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "ic53",		  0x0000, 0x0100, CRC(6e66057f) SHA1(084d630f5e2f23e28a1f7839337ef608e086e8c4) ) /* palette low bits */
	ROM_LOAD( "ic54",		  0x0100, 0x0100, CRC(236bc771) SHA1(5c078eecdd9df2fbc791e440f96bc4c79476b211) ) /* palette high bits */
ROM_END



DRIVER_INIT( popflame )
{
	/* install a handler to catch protection checks */
	install_mem_read_handler(0, 0x9000, 0x9000, popflame_protection_r);
}


GAMEX( 1982, naughtyb, 0,		 naughtyb, naughtyb, 0,        ROT90, "Jaleco", "Naughty Boy", GAME_NO_COCKTAIL )
GAMEX( 1982, naughtya, naughtyb, naughtyb, naughtyb, 0,        ROT90, "bootleg", "Naughty Boy (bootleg)", GAME_NO_COCKTAIL )
GAMEX( 1982, naughtyc, naughtyb, naughtyb, naughtyb, 0,        ROT90, "Jaleco (Cinematronics license)", "Naughty Boy (Cinematronics)", GAME_NO_COCKTAIL )
GAMEX( 1982, popflame, 0,		 popflame, naughtyb, popflame, ROT90, "Jaleco", "Pop Flamer (protected)", GAME_NO_COCKTAIL )
GAMEX( 1982, popflama, popflame, popflame, naughtyb, 0,        ROT90, "Jaleco", "Pop Flamer (not protected)", GAME_NO_COCKTAIL )
GAMEX( 1982, popflamb, popflame, popflame, naughtyb, 0,        ROT90, "Jaleco", "Pop Flamer (hack[Q])", GAME_NO_COCKTAIL )
