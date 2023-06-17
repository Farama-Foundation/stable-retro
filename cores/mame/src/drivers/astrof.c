/*
    Driver For DECO   ASTRO FIGHTER/TOMAHAWK 777

    Initial Version

    Lee Taylor 28/11/1997


	Astro Fighter Sets:

    The differences are minor. From newest to oldest:

	Main Set: 16Kbit ROMs
	          Green/Hollow empty fuel bar.
			  60 points for every bomb destroyed.

	Set 2:    8Kbit ROMs
			  Blue/Solid empty fuel bar.
			  60 points for every bomb destroyed.

	Set 3:    8Kbit ROMs
			  Blue/Solid empty fuel bar.
			  300 points for every seven bombs destroyed.


To Do!!
	   Figure out the correct vblank interval. The one I'm using seems to be
	   ok for Astro Fighter, but the submarine in Tomahawk flickers.
	   Maybe the video rate should 57FPS as the Burger Time games?

	   Rotation Support

Also....
        I know there must be at least one other rom set for Astro Fighter
        I have played one that stoped between waves to show the next enemy
*/

#include "driver.h"
#include "vidhrdw/generic.h"

extern unsigned char *astrof_color;
extern unsigned char *tomahawk_protection;

PALETTE_INIT( astrof );
VIDEO_START( astrof );
VIDEO_UPDATE( astrof );
WRITE_HANDLER( astrof_videoram_w );
WRITE_HANDLER( tomahawk_videoram_w );
WRITE_HANDLER( astrof_video_control1_w );
WRITE_HANDLER( astrof_video_control2_w );
WRITE_HANDLER( tomahawk_video_control2_w );
READ_HANDLER( tomahawk_protection_r );
WRITE_HANDLER( astrof_sample1_w );
WRITE_HANDLER( astrof_sample2_w );

extern struct Samplesinterface astrof_samples_interface;
extern struct Samplesinterface tomahawk_samples_interface;

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x03ff, MRA_RAM },
	{ 0x4000, 0x5fff, MRA_RAM },
	{ 0xa000, 0xa000, input_port_0_r },
	{ 0xa001, 0xa001, input_port_1_r },	/* IN1 */
	{ 0xa003, 0xa003, tomahawk_protection_r },   /* Only on Tomahawk*/
	{ 0xd000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( astrof_writemem )
	{ 0x0000, 0x03ff, MWA_RAM },
	{ 0x4000, 0x5fff, astrof_videoram_w, &videoram, &videoram_size },
	{ 0x8003, 0x8003, MWA_RAM, &astrof_color },
	{ 0x8004, 0x8004, astrof_video_control1_w },
	{ 0x8005, 0x8005, astrof_video_control2_w },
	{ 0x8006, 0x8006, astrof_sample1_w },
	{ 0x8007, 0x8007, astrof_sample2_w },
MEMORY_END

static MEMORY_WRITE_START( tomahawk_writemem )
	{ 0x0000, 0x03ff, MWA_RAM },
	{ 0x4000, 0x5fff, tomahawk_videoram_w, &videoram, &videoram_size },
	{ 0x8003, 0x8003, MWA_RAM, &astrof_color },
	{ 0x8004, 0x8004, astrof_video_control1_w },
	{ 0x8005, 0x8005, tomahawk_video_control2_w },
	{ 0x8006, 0x8006, MWA_NOP },                        /* Sound triggers*/
	{ 0x8007, 0x8007, MWA_RAM, &tomahawk_protection },
MEMORY_END



/***************************************************************************

  These games don't have VBlank interrupts.
  Interrupts are still used by the game: but they are related to coin
  slots.

***************************************************************************/
static INTERRUPT_GEN( astrof_interrupt )
{
	if (readinputport(2) & 1)	/* Coin */
		cpu_set_irq_line(0, IRQ_LINE_NMI, PULSE_LINE);
}


INPUT_PORTS_START( astrof )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
/* Player 1 Controls */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
/* Player 2 Controls */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )

	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
/* 0x0c gives 2 Coins/1 Credit */

	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "3000" )
	PORT_DIPSETTING(    0x10, "5000" )
	PORT_DIPSETTING(    0x20, "7000" )
	PORT_DIPSETTING(    0x30, "None" )

	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x40, "Hard" )

	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )

	PORT_START	/* FAKE */
	/* The coin slots are not memory mapped. Coin insertion causes a NMI. */
	/* This fake input port is used by the interrupt */
	/* handler to be notified of coin insertions. We use IMPULSE to */
	/* trigger exactly one interrupt, without having to check when the */
	/* user releases the key. */
	/* The cabinet selector is not memory mapped, but just disables the */
	/* screen flip logic */
	PORT_BIT_IMPULSE( 0x01, IP_ACTIVE_HIGH, IPT_COIN1, 1 )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Cocktail ) )
INPUT_PORTS_END


INPUT_PORTS_START( tomahawk )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )

	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
/* 0x0c gives 2 Coins/1 Credit */

	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "5000" )
	PORT_DIPSETTING(    0x10, "7000" )
	PORT_DIPSETTING(    0x20, "10000" )
	PORT_DIPSETTING(    0x30, "None" )

	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Difficulty ) )  /* Only on Tomahawk 777 */
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x40, "Hard" )

	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )

	PORT_START	/* FAKE */
	/* The coin slots are not memory mapped. Coin insertion causes a NMI. */
	/* This fake input port is used by the interrupt */
	/* handler to be notified of coin insertions. We use IMPULSE to */
	/* trigger exactly one interrupt, without having to check when the */
	/* user releases the key. */
	/* The cabinet selector is not memory mapped, but just disables the */
	/* screen flip logic */
	PORT_BIT_IMPULSE( 0x01, IP_ACTIVE_HIGH, IPT_COIN1, 1 )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Cocktail ) )
INPUT_PORTS_END


static MACHINE_DRIVER_START( astrof )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M6502, 10595000/16)	/* 0.66 MHz */
	MDRV_CPU_MEMORY(readmem,astrof_writemem)
	MDRV_CPU_VBLANK_INT(astrof_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(3400)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(8, 256-1-8, 8, 256-1-8)
	MDRV_PALETTE_LENGTH(16)

	MDRV_PALETTE_INIT(astrof)
	MDRV_VIDEO_START(astrof)
	MDRV_VIDEO_UPDATE(astrof)

	/* sound hardware */
	MDRV_SOUND_ADD_TAG("samples", SAMPLES, astrof_samples_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( tomahawk )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(astrof)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(readmem,tomahawk_writemem)

	/* video hardware */
	MDRV_PALETTE_LENGTH(32)

	/* sound hardware */
	MDRV_SOUND_REPLACE("samples", SAMPLES, tomahawk_samples_interface)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( astrof )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "afii.6",       0xd000, 0x0800, CRC(d6cd13a4) SHA1(359b00b02f4256f1138c8526214c6a34d2e5b47a) )
	ROM_LOAD( "afii.5",       0xd800, 0x0800, CRC(6fd3c4df) SHA1(73aad03e2588ac9f249d5751eb4a7c7cd12fd3b9) )
	ROM_LOAD( "afii.4",       0xe000, 0x0800, CRC(9612dae3) SHA1(8ee1797c212e06c381972b7b555f240ff317d75d) )
	ROM_LOAD( "afii.3",       0xe800, 0x0800, CRC(5a0fef42) SHA1(92a575abdf17bbb5ed6bc67479049523a985aa75) )
	ROM_LOAD( "afii.2",       0xf000, 0x0800, CRC(69f8a4fc) SHA1(9f9a935f19187640018009ade92f8993912ef6c2) )
	ROM_LOAD( "afii.1",       0xf800, 0x0800, CRC(322c09d2) SHA1(89723e3d998ff9cb9b174bca4b072b412b290c04) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "astrf.clr",    0x0000, 0x0020, CRC(61329fd1) SHA1(15782d8757d4dda5a8b97815e94c90218f0e08dd) )
ROM_END

ROM_START( astrof2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "kei2",         0xd000, 0x0400, CRC(9f0bd355) SHA1(45db9229dcd8bbd366ff13c683625c3d1c175598) )
	ROM_LOAD( "keii",         0xd400, 0x0400, CRC(71f229f0) SHA1(be426360567066df01fb428dc5cd2d6ef01a4cf7) )
	ROM_LOAD( "kei0",         0xd800, 0x0400, CRC(88114f7c) SHA1(e64ae3cac92d2a3c02edc8e81c88d5d275e89293) )
	ROM_LOAD( "af579.08",     0xdc00, 0x0400, CRC(9793c124) SHA1(ae0352ed13fa21a00181669e92f9e66c938e4843) )
	ROM_LOAD( "ke8",          0xe000, 0x0400, CRC(08e44b12) SHA1(0e156fff081ae74321597eca1a02920bfc464651) )
	ROM_LOAD( "ke7",          0xe400, 0x0400, CRC(8a42d62c) SHA1(f5c0043be113c88f87deee3a2acd7d778a569e4f) )
	ROM_LOAD( "ke6",          0xe800, 0x0400, CRC(3e9aa743) SHA1(5f473afee7a416bb6f4e658cf8e46f8362ae3bba) )
	ROM_LOAD( "ke5",          0xec00, 0x0400, CRC(712a4557) SHA1(66a19378782c3911b8740ca25451ce84e1096fd0) )
	ROM_LOAD( "ke4",          0xf000, 0x0400, CRC(ad06f306) SHA1(d6ab7cba97658a46a63846a203eb89d9fc367e4f) )
	ROM_LOAD( "ke3",          0xf400, 0x0400, CRC(680b91b4) SHA1(004fd0f6564c19277632adec42bcf1054d043e4a) )
	ROM_LOAD( "ke2",          0xf800, 0x0400, CRC(2c4cab1a) SHA1(3171764a17f2c5fda39f0b32ccce60bc107d306e) )
	ROM_LOAD( "af583.00",     0xfc00, 0x0400, CRC(f699dda3) SHA1(e595cb93df40f64f7521afa51a879d53e1d04126) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "astrf.clr",    0x0000, 0x0020, CRC(61329fd1) SHA1(15782d8757d4dda5a8b97815e94c90218f0e08dd) )
ROM_END

ROM_START( astrof3 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "kei2",         0xd000, 0x0400, CRC(9f0bd355) SHA1(45db9229dcd8bbd366ff13c683625c3d1c175598) )
	ROM_LOAD( "keii",         0xd400, 0x0400, CRC(71f229f0) SHA1(be426360567066df01fb428dc5cd2d6ef01a4cf7) )
	ROM_LOAD( "kei0",         0xd800, 0x0400, CRC(88114f7c) SHA1(e64ae3cac92d2a3c02edc8e81c88d5d275e89293) )
	ROM_LOAD( "ke9",          0xdc00, 0x0400, CRC(29cbaea6) SHA1(da29e8156218884195b16839be9ad1e98a8348ac) )
	ROM_LOAD( "ke8",          0xe000, 0x0400, CRC(08e44b12) SHA1(0e156fff081ae74321597eca1a02920bfc464651) )
	ROM_LOAD( "ke7",          0xe400, 0x0400, CRC(8a42d62c) SHA1(f5c0043be113c88f87deee3a2acd7d778a569e4f) )
	ROM_LOAD( "ke6",          0xe800, 0x0400, CRC(3e9aa743) SHA1(5f473afee7a416bb6f4e658cf8e46f8362ae3bba) )
	ROM_LOAD( "ke5",          0xec00, 0x0400, CRC(712a4557) SHA1(66a19378782c3911b8740ca25451ce84e1096fd0) )
	ROM_LOAD( "ke4",          0xf000, 0x0400, CRC(ad06f306) SHA1(d6ab7cba97658a46a63846a203eb89d9fc367e4f) )
	ROM_LOAD( "ke3",          0xf400, 0x0400, CRC(680b91b4) SHA1(004fd0f6564c19277632adec42bcf1054d043e4a) )
	ROM_LOAD( "ke2",          0xf800, 0x0400, CRC(2c4cab1a) SHA1(3171764a17f2c5fda39f0b32ccce60bc107d306e) )
	ROM_LOAD( "kei",          0xfc00, 0x0400, CRC(fce4718d) SHA1(3a313328609f6bef644a2d906d8ca74c5d52058b) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "astrf.clr",    0x0000, 0x0020, CRC(61329fd1) SHA1(15782d8757d4dda5a8b97815e94c90218f0e08dd) )
ROM_END

ROM_START( tomahawk )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "l8-1",         0xdc00, 0x0400, CRC(7c911661) SHA1(3fc75bb0e6a89d41d76f82eeb0fde7d33809dddf) )
	ROM_LOAD( "l7-1",         0xe000, 0x0400, CRC(adeffb69) SHA1(8ff7ada883825a8b56cae3368ce377228922ab1d) )
	ROM_LOAD( "l6-1",         0xe400, 0x0400, CRC(9116e59d) SHA1(22a6d410fff8534b3aa7eb2ed0a8c096c890acf5) )
	ROM_LOAD( "l5-1",         0xe800, 0x0400, CRC(01e4c7c4) SHA1(fbb37539d08284bae6454cd57650e8507a88acdb) )
	ROM_LOAD( "l4-1",         0xec00, 0x0400, CRC(d9f69cb0) SHA1(d6a2dcaf867f33068e7d7ad7a3faf62a360456a6) )
	ROM_LOAD( "l3-1",         0xf000, 0x0400, CRC(7ce7183f) SHA1(949c7b696fe215b68af450299c91e90fb27b0141) )
	ROM_LOAD( "l2-1",         0xf400, 0x0400, CRC(43fea29d) SHA1(6890311440089a16d2e4d502855670723df41e16) )
	ROM_LOAD( "l1-1",         0xf800, 0x0400, CRC(f2096ba9) SHA1(566f6d49cdacb5e39c40eb3773640270ef5f272c) )
	ROM_LOAD( "l0-1",         0xfc00, 0x0400, CRC(42edbc28) SHA1(bab1fe8591509783dfdd4f53b9159263b9201970) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "t777.clr",     0x0000, 0x0020, CRC(d6a528fd) SHA1(5fc08252a2d7c5405f601efbfb7d84bec328d733) )
ROM_END

ROM_START( tomahaw5 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "thawk.l8",     0xdc00, 0x0400, CRC(b01dab4b) SHA1(d8b4266359a3b18d649f539fad8dce4d73cec412) )
	ROM_LOAD( "thawk.l7",     0xe000, 0x0400, CRC(3a6549e8) SHA1(2ba622d78596c72998784432cf8fbbe733c50ce5) )
	ROM_LOAD( "thawk.l6",     0xe400, 0x0400, CRC(863e47f7) SHA1(e8e48560c217025796be20f51c50ec276dba3eb5) )
	ROM_LOAD( "thawk.l5",     0xe800, 0x0400, CRC(de0183bc) SHA1(7cb8d013750c8fb423ab2759443f805bc8440d53) )
	ROM_LOAD( "thawk.l4",     0xec00, 0x0400, CRC(11e9c7ea) SHA1(9dbdce7d518891aa8b08dca50d4e8aaec89cc038) )
	ROM_LOAD( "thawk.l3",     0xf000, 0x0400, CRC(ec44d388) SHA1(7dda9db5ce2271988e9316dacf4b6ccbb72f50c9) )
	ROM_LOAD( "thawk.l2",     0xf400, 0x0400, CRC(dc0a0f54) SHA1(8e5c94706768ffafaba96382f2e757ecb825799f) )
	ROM_LOAD( "thawk.l1",     0xf800, 0x0400, CRC(1d9dab9c) SHA1(54dd91164db0489bd5984f10d4f0254184302ae4) )
	ROM_LOAD( "thawk.l0",     0xfc00, 0x0400, CRC(d21a1eba) SHA1(ce9ad7a1a3b069ef4eb8b5ce569e52c488a224f2) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "t777.clr",     0x0000, 0x0020, CRC(d6a528fd) SHA1(5fc08252a2d7c5405f601efbfb7d84bec328d733) )
ROM_END



GAME( 1979, astrof,   0,        astrof,   astrof,   0, ROT90, "Data East", "Astro Fighter (set 1)" )
GAME( 1979, astrof2,  astrof,   astrof,   astrof,   0, ROT90, "Data East", "Astro Fighter (set 2)" )
GAME( 1979, astrof3,  astrof,   astrof,   astrof,   0, ROT90, "Data East", "Astro Fighter (set 3)" )
GAMEX(1980, tomahawk, 0,        tomahawk, tomahawk, 0, ROT90, "Data East", "Tomahawk 777 (Revision 1)", GAME_NO_SOUND )
GAMEX(1980, tomahaw5, tomahawk, tomahawk, tomahawk, 0, ROT90, "Data East", "Tomahawk 777 (Revision 5)", GAME_NO_SOUND )
