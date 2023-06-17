/***************************************************************************

 Berzerk Driver by Zsolt Vasvari
 Sound Driver by Alex Judd

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "includes/berzerk.h"



static MEMORY_READ_START( berzerk_readmem )
	{ 0x0000, 0x07ff, MRA_ROM },
	{ 0x0800, 0x09ff, MRA_RAM },
	{ 0x1000, 0x37ff, MRA_ROM },
	{ 0x4000, 0x87ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( berzerk_writemem )
	{ 0x0000, 0x07ff, MWA_ROM },
	{ 0x0800, 0x09ff, MWA_RAM, &generic_nvram, &generic_nvram_size },
	{ 0x1000, 0x37ff, MWA_ROM },
	{ 0x4000, 0x5fff, berzerk_videoram_w, &videoram },
	{ 0x6000, 0x7fff, berzerk_magicram_w, &berzerk_magicram },
	{ 0x8000, 0x87ff, berzerk_colorram_w, &colorram },
MEMORY_END


static MEMORY_READ_START( frenzy_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x87ff, MRA_RAM },
	{ 0xc000, 0xcfff, MRA_ROM },
	{ 0xf800, 0xf9ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( frenzy_writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x5fff, berzerk_videoram_w, &videoram },
	{ 0x6000, 0x7fff, berzerk_magicram_w, &berzerk_magicram },
	{ 0x8000, 0x87ff, berzerk_colorram_w, &colorram },
	{ 0xc000, 0xcfff, MWA_ROM },
	{ 0xf800, 0xf9ff, MWA_RAM },
MEMORY_END


static PORT_READ_START( readport )
	{ 0x44, 0x44, berzerk_voiceboard_r },
	{ 0x48, 0x48, input_port_0_r },
	{ 0x49, 0x49, input_port_1_r },
	{ 0x4a, 0x4a, input_port_2_r },
	{ 0x4c, 0x4c, berzerk_nmi_enable_r },
	{ 0x4d, 0x4d, berzerk_nmi_disable_r },
	{ 0x4e, 0x4e, berzerk_port_4e_r },
	{ 0x60, 0x60, input_port_4_r },
	{ 0x61, 0x61, input_port_5_r },
	{ 0x62, 0x62, input_port_6_r },
	{ 0x63, 0x63, input_port_7_r },
	{ 0x64, 0x64, input_port_8_r },
	{ 0x65, 0x65, input_port_9_r },
	{ 0x66, 0x66, berzerk_led_off_r },
	{ 0x67, 0x67, berzerk_led_on_r },
PORT_END

static PORT_WRITE_START( writeport )
	{ 0x40, 0x46, berzerk_sound_control_a_w }, /* First sound board */
	{ 0x47, 0x47, IOWP_NOP }, /* not used sound stuff */
	{ 0x4b, 0x4b, berzerk_magicram_control_w },
	{ 0x4c, 0x4c, berzerk_nmi_enable_w },
	{ 0x4d, 0x4d, berzerk_nmi_disable_w },
	{ 0x4f, 0x4f, berzerk_irq_enable_w },
	{ 0x50, 0x57, IOWP_NOP }, /* Second sound board but not used */
PORT_END


#define COINAGE(CHUTE) \
	PORT_DIPNAME( 0x0f, 0x00, "Coin "#CHUTE ) \
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x0d, DEF_STR( 4C_3C ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0x0e, DEF_STR( 4C_5C ) ) \
	PORT_DIPSETTING(    0x0a, DEF_STR( 2C_3C ) ) \
	PORT_DIPSETTING(    0x0f, DEF_STR( 4C_7C ) ) \
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) ) \
	PORT_DIPSETTING(    0x0b, DEF_STR( 2C_5C ) ) \
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) ) \
	PORT_DIPSETTING(    0x0c, DEF_STR( 2C_7C ) ) \
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) ) \
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_5C ) ) \
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_6C ) ) \
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_7C ) ) \
	PORT_DIPSETTING(    0x07, "1 Coin/10 Credits" ) \
	PORT_DIPSETTING(    0x08, "1 Coin/14 Credits" ) \
	PORT_BIT( 0xf0, IP_ACTIVE_LOW,  IPT_UNUSED )


INPUT_PORTS_START( berzerk )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x1c, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x60, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )

	PORT_START      /* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0x7e, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* Collision */

	PORT_START      /* IN4 */
	PORT_BITX(    0x01, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Input Test Mode", KEYCODE_F2, IP_JOY_NONE )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_BITX(    0x02, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Crosshair Pattern", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_BIT( 0x3c, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_DIPNAME( 0xc0, 0x00, "Language" )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPSETTING(    0x40, "German" )
	PORT_DIPSETTING(    0x80, "French" )
	PORT_DIPSETTING(    0xc0, "Spanish" )

	PORT_START      /* IN5 */
	PORT_BITX(    0x03, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Color Test", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x03, DEF_STR( On ) )
	PORT_BIT( 0x3c, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0xc0, "5000 and 10000" )
	PORT_DIPSETTING(    0x40, "5000" )
	PORT_DIPSETTING(    0x80, "10000" )
	PORT_DIPSETTING(    0x00, "None" )

	PORT_START      /* IN6 */
	COINAGE(3)

	PORT_START      /* IN7 */
	COINAGE(2)

	PORT_START      /* IN8 */
	COINAGE(1)

	PORT_START      /* IN9 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_BIT( 0x7e, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BITX(0x80, IP_ACTIVE_HIGH, 0, "Stats", KEYCODE_F1, IP_JOY_NONE )
INPUT_PORTS_END

INPUT_PORTS_START( frenzy )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x3c, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x60, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )

	PORT_START      /* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0x7e, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* Collision */

	PORT_START      /* IN4 */
	PORT_DIPNAME( 0x0f, 0x03, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x01, "1000" )
	PORT_DIPSETTING(    0x02, "2000" )
	PORT_DIPSETTING(    0x03, "3000" )
	PORT_DIPSETTING(    0x04, "4000" )
	PORT_DIPSETTING(    0x05, "5000" )
	PORT_DIPSETTING(    0x06, "6000" )
	PORT_DIPSETTING(    0x07, "7000" )
	PORT_DIPSETTING(    0x08, "8000" )
	PORT_DIPSETTING(    0x09, "9000" )
	PORT_DIPSETTING(    0x0a, "10000" )
	PORT_DIPSETTING(    0x0b, "11000" )
	PORT_DIPSETTING(    0x0c, "12000" )
	PORT_DIPSETTING(    0x0d, "13000" )
	PORT_DIPSETTING(    0x0e, "14000" )
	PORT_DIPSETTING(    0x0f, "15000" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_BIT( 0x30, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_DIPNAME( 0xc0, 0x00, "Language" )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPSETTING(    0x40, "German" )
	PORT_DIPSETTING(    0x80, "French" )
	PORT_DIPSETTING(    0xc0, "Spanish" )

	PORT_START      /* IN5 */
	PORT_BIT( 0x03, IP_ACTIVE_HIGH, IPT_UNUSED )  /* Bit 0 does some more hardware tests */
	PORT_BITX(    0x04, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Input Test Mode", KEYCODE_F2, IP_JOY_NONE )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_BITX(    0x08, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Crosshair Pattern", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	/* The following 3 ports use all 8 bits, but I didn't feel like adding all 256 values :-) */
	PORT_START      /* IN6 */
	PORT_DIPNAME( 0x0f, 0x01, "Coins/Credit B" )
	/*PORT_DIPSETTING(    0x00, "0" )    Can't insert coins  */
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x05, "5" )
	PORT_DIPSETTING(    0x06, "6" )
	PORT_DIPSETTING(    0x07, "7" )
	PORT_DIPSETTING(    0x08, "8" )
	PORT_DIPSETTING(    0x09, "9" )
	PORT_DIPSETTING(    0x0a, "10" )
	PORT_DIPSETTING(    0x0b, "11" )
	PORT_DIPSETTING(    0x0c, "12" )
	PORT_DIPSETTING(    0x0d, "13" )
	PORT_DIPSETTING(    0x0e, "14" )
	PORT_DIPSETTING(    0x0f, "15" )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH,  IPT_UNUSED )

	PORT_START      /* IN7 */
	PORT_DIPNAME( 0x0f, 0x01, "Coins/Credit A" )
	/*PORT_DIPSETTING(    0x00, "0" )    Can't insert coins  */
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x05, "5" )
	PORT_DIPSETTING(    0x06, "6" )
	PORT_DIPSETTING(    0x07, "7" )
	PORT_DIPSETTING(    0x08, "8" )
	PORT_DIPSETTING(    0x09, "9" )
	PORT_DIPSETTING(    0x0a, "10" )
	PORT_DIPSETTING(    0x0b, "11" )
	PORT_DIPSETTING(    0x0c, "12" )
	PORT_DIPSETTING(    0x0d, "13" )
	PORT_DIPSETTING(    0x0e, "14" )
	PORT_DIPSETTING(    0x0f, "15" )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH,  IPT_UNUSED )

	PORT_START      /* IN8 */
	PORT_DIPNAME( 0x0f, 0x01, "Coin Multiplier" )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x05, "5" )
	PORT_DIPSETTING(    0x06, "6" )
	PORT_DIPSETTING(    0x07, "7" )
	PORT_DIPSETTING(    0x08, "8" )
	PORT_DIPSETTING(    0x09, "9" )
	PORT_DIPSETTING(    0x0a, "10" )
	PORT_DIPSETTING(    0x0b, "11" )
	PORT_DIPSETTING(    0x0c, "12" )
	PORT_DIPSETTING(    0x0d, "13" )
	PORT_DIPSETTING(    0x0e, "14" )
	PORT_DIPSETTING(    0x0f, "15" )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH,  IPT_UNUSED )

	PORT_START      /* IN9 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x7e, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BITX(0x80, IP_ACTIVE_HIGH, 0, "Stats", KEYCODE_F1, IP_JOY_NONE )
INPUT_PORTS_END


static MACHINE_DRIVER_START( berzerk )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 2500000)        /* 2.5 MHz */
	MDRV_CPU_MEMORY(berzerk_readmem,berzerk_writemem)
	MDRV_CPU_PORTS(readport,writeport)
	MDRV_CPU_VBLANK_INT(berzerk_interrupt,8)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(2500)  /* Needs to be long enough so 2 of the 8 */
								/* interrupts fall inside the VBLANK */
	MDRV_MACHINE_INIT(berzerk)
	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 256-1, 32, 256-1)
	MDRV_PALETTE_LENGTH(16)

	MDRV_PALETTE_INIT(berzerk)
	MDRV_VIDEO_START(generic_bitmapped)
	MDRV_VIDEO_UPDATE(generic_bitmapped)

	/* sound hardware */
	MDRV_SOUND_ADD(SAMPLES, berzerk_samples_interface)
	MDRV_SOUND_ADD(CUSTOM, berzerk_custom_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( frenzy )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(berzerk)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(frenzy_readmem,frenzy_writemem)

	MDRV_MACHINE_INIT(NULL)
	MDRV_NVRAM_HANDLER(NULL)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( berzerk )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "1c-0",         0x0000, 0x0800, CRC(ca566dbc) SHA1(fae2647f12f1cd82826db61b53b116a5e0c9f995) )
	ROM_LOAD( "1d-1",         0x1000, 0x0800, CRC(7ba69fde) SHA1(69af170c4a39a3494dcd180737e5c87b455f9203) )
	ROM_LOAD( "3d-2",         0x1800, 0x0800, CRC(a1d5248b) SHA1(a0b7842f6a5f86c16d80d78e7012c78b3ea11d1d) )
	ROM_LOAD( "5d-3",         0x2000, 0x0800, CRC(fcaefa95) SHA1(07f849aa39f1e3db938187ffde4a46a588156ddc) )
	ROM_LOAD( "6d-4",         0x2800, 0x0800, CRC(1e35b9a0) SHA1(5a5e549ec0e4803ab2d1eac6b3e7171aedf28244) )
	ROM_LOAD( "5c-5",         0x3000, 0x0800, CRC(c8c665e5) SHA1(e9eca4b119549e0061384abf52327c14b0d56624) )
ROM_END

ROM_START( berzerk1 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "rom0.1c",      0x0000, 0x0800, CRC(5b7eb77d) SHA1(8de488e279036fe40d6fb4c0dde16075309342fd) )
	ROM_LOAD( "rom1.1d",      0x1000, 0x0800, CRC(e58c8678) SHA1(a11f08448b457d690b270512c9f02fcf1e41d9e0) )
	ROM_LOAD( "rom2.3d",      0x1800, 0x0800, CRC(705bb339) SHA1(845191df90cd7d80f8fed3d2b69305301d921549) )
	ROM_LOAD( "rom3.5d",      0x2000, 0x0800, CRC(6a1936b4) SHA1(f1635e9d2f25514c35559d2a247c3bc4b4034c19) )
	ROM_LOAD( "rom4.6d",      0x2800, 0x0800, CRC(fa5dce40) SHA1(b3a3ee52bf65bbb3a20f905d3e4ebdf6871dcb5d) )
	ROM_LOAD( "rom5.5c",      0x3000, 0x0800, CRC(2579b9f4) SHA1(890f0237afbb194166eae88c98de81989f408548) )
ROM_END

ROM_START( frenzy )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "1c-0",         0x0000, 0x1000, CRC(abdd25b8) SHA1(e6a3ab826b51b2c6ddd63d55681848fccad800dd) )
	ROM_LOAD( "1d-1",         0x1000, 0x1000, CRC(536e4ae8) SHA1(913385c43b8902d3d3ad2194a3137e19e61c6573) )
	ROM_LOAD( "3d-2",         0x2000, 0x1000, CRC(3eb9bc9b) SHA1(1e43e76ae0606a6d41d9006005d6001bdee48694) )
	ROM_LOAD( "5d-3",         0x3000, 0x1000, CRC(e1d3133c) SHA1(2af4a9bc2b29735a548ae770f872127bc009cc42) )
	ROM_LOAD( "6d-4",         0xc000, 0x1000, CRC(5581a7b1) SHA1(1f633c1c29d3b64f701c601feba26da66a6c6f23) )
	/* 1c & 2c are the voice ROMs */
ROM_END



GAME( 1980, berzerk,  0,       berzerk, berzerk, 0, ROT0, "Stern", "Berzerk (set 1)" )
GAME( 1980, berzerk1, berzerk, berzerk, berzerk, 0, ROT0, "Stern", "Berzerk (set 2)" )
GAME( 1982, frenzy,   0,       frenzy,  frenzy,  0, ROT0, "Stern", "Frenzy" )
