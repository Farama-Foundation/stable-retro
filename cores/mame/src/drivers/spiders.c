/***************************************************************************

Spiders driver by K.Wilkins May 1998


Memory map information
----------------------

Main CPU - Read Range

$0000-$1bff	Video Memory (bit0)
$4000-$5bff	Video Memory (bit1)
$8000-$9bff	Video Memory (bit2)
$c000-$c001	6845 CRT Controller (crtc6845)
$c044-$c047	MC6821 PIA 1 (Control input port - all input)
$c048-$c04b	MC6821 PIA 2 (Sprite data port - see machine/spiders.c)
$c050-$c053	MC6821 PIA 3 (Sound control - all output)
$c060		Dip Switch 1
$c080		Dip Switch 2
$c0a0		Dip Switch 3
$c100-ffff	ROM SPACE

Main CPU - Write Range

$0000-$1bff	Video Memory (bit0)
$4000-$5bff	Video Memory (bit1)
$8000-$9bff	Video Memory (bit2)
$c000-$c001	6845 CRT Controller (crtc6845)
$c044-$c047	MC6821 PIA 1
$c048-$c04b	MC6821 PIA 2 (Video port)
$c050-$c053	MC6821 PIA 3


DIP SWITCH 1
------------

   1   2   3   4   5   6   7   8    COIN/CREDIT
   ON  ON  ON                       FREE PLAY
   ON  ON  OFF                      1/2
   ON  OFF ON                       1/3
   OFF ON  ON                       2/1
   ON  OFF OFF                      4/5
   OFF OFF OFF                      1/1

DIP SWITCH 2
------------

   1   2   3   4   5   6   7   8
   ON  ON                           MODE A    A'
   ON  OFF                               A    B'
   OFF ON                                B    A'
   OFF OFF                               B    B'
           ON  ON                   14 # OF SPIDERS WHICH LAND TO
           ON  OFF                  20    COMPLETE SPIDER BELT
           OFF ON                   26
           OFF OFF                  16
                   ON               4  # 0F SPARE GUNS
                   OFF              3
                       ON   ON      NONE  SCORE FOR BONUS GUN
                       ON   OFF     20K
                       OFF  ON      25K
                       OFF  OFF     15K
                               ON   GIANT SPIDER AFTER FIRST SCREEN
                               OFF  GIANT SPIDER AFTER EVERY SCREEN

   PATTERN   1   2   3   4   5   6   7   8   9   10  11  12  13  14
   MODE A    27  36  45  54  63  72  81  98  45  54  63  72  81  98    PCS
   MODE B    20  27  34  41  48  55  62  69  34  41  48  55  62  69    PCS
   MODE A'   1   1   1   3.5 3.5 4   4.5 5   1   3.5 3.5 4   4.5 5     SECONDS
   MODE B'   .7  .7  .7  2   3   3.2 3.4 4   .7  2   3   2.3 3.4 4     SECONDS

   MODE A & B FOR THE NUMBER OF GROWABLE COCOONS
   MODE A' & B' FOR THE FREQUENCY OF SPIDERS APPEARANCE


DIP SWITCH 3
------------

   1   2   3   4   5   6   7   8
   X                                VIDEO FLIP
       ON                           UPRIGHT
       OFF                          TABLE

   SWITCHES 3,4,5 FOR ADJUSTING PICTURE VERTICALLY
   SWITCHES 6,7,8 FOR ADJUSTING PICTURE HORIZONTALLY


Unpopulated Switches
--------------------

  PS1 (Display Crosshatch)         - Connected to PIA1 CB1 via pull-up
  PS2 (Coin input, bypass counter) - Connected to PIA1 PA1 via pull-up and invertor
  PS3 (Display coin counter)       - Connected to PIA1 PA2 via pull-up and invertor


Graphic notes
-------------
Following roms appear to have graphic data

* Mapped in main CPU space

* SP1.BIN	- Appears to have some sprites in it.
* SP2.BIN	- Appears to have some 16x16 sprites. (Includes the word SIGMA)
* SP3.BIN	- Appears to have 2-4 sprites 16x16 - spiders
* SP4.BIN	- CPU Code 6809 - Main
  SP5.BIN	- Some 8x8 and 16x16 tiles/sprites
  SP6.BIN	- Some 8x8 tiles
  SP7.BIN	- Tiles/Sprites 8x8
  SP8.BIN	- Tiles/Sprites 8x8
  SP9A.BIN 	- Tiles/Sprites 8x8
  SP9B.BIN	- Tiles/Sprites 8x8
  SP10A.BIN	- Tiles/Sprites 8x8
  SP10B.BIN	- CPU Code 6802 - Sound

Spiders has a fully bitmapped display and all sprite drawing is handled by the CPU
hence no gfxdecode is done on the sprite ROMS, the CPU accesses them via PIA2.

Screen is arranged in three memory areas with three bits being combined from each
area to produce a 3bit colour send directly to the screen.

$0000-$1bff, $4000-$5bff, $8000-$9bff 	Bank 0
$2000-$3bff, %6000-$7bff, $a000-$bbff	Bank 1

The game normally runs from bank 0 only, but when lots of screen changes are required
e.g spider or explosion then it implements a double buffered scheme with bank 1.

The ram bank for screens is continuous from $0000-$bfff but is physically arranged
as 3 banks of 16k (8x16k DRAM!). The CPU stack/variables etc are stored in the unused
spaces between screens.


CODE NOTES
----------

6809 Data page = $1c00 (DP=1c)

Known data page contents
$05 - Dip switch 1 copy
$06 - Dip switch 2 copy (inverted)
$07 - Dip switch 3 copy
$18 - Bonus Gun Score
$1d - Spiders to complete belt after dipsw decode


$c496 - Wait for vblank ($c04a bit 7 goes high)
$f9cf - Clear screen (Bank0&1)
$c8c6 - RAM test of sorts, called from IRQ handler?
$de2f - Delay loop.
$f9bb - Memory clearance routine
$c761 - Partial DipSW decode
$F987 - Addresses table at $f98d containing four structs:
            3c 0C 04 0D 80 (Inverted screen bank 0)
            34 0C 00 0D 00 (Normal screen   bank 0)
            3C 0C 40 0D 80 (Inverted screen bank 1)
            34 0C 44 0D 00 (Inverted screen bank 1)
            XX             Written to PIA2 Reg 3 - B control
               XX XX       Written to CRTC addr/data
                     XX XX Written to CRTC addr/data
            These tables are used for frame flipping


***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/crtc6845.h"
#include "machine/6821pia.h"


/* VIDHRDW */

PALETTE_INIT( nyny );
VIDEO_START( spiders );
VIDEO_UPDATE( spiders );

/* MACHINE */

MACHINE_INIT( spiders );
INTERRUPT_GEN( spiders_timed_irq );


/* Driver structure definition */

static MEMORY_READ_START( readmem )
	{ 0x0000, 0xbfff, MRA_RAM },
	{ 0xc001, 0xc001, crtc6845_register_r },
	{ 0xc044, 0xc047, pia_0_r },
	{ 0xc048, 0xc04b, pia_1_r },
	{ 0xc050, 0xc053, pia_2_r },
	{ 0xc060, 0xc060, input_port_2_r },
	{ 0xc080, 0xc080, input_port_3_r },
	{ 0xc0a0, 0xc0a0, input_port_4_r },
	{ 0xc100, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0xbfff, MWA_RAM },
	{ 0xc000, 0xc000, crtc6845_address_w },
	{ 0xc001, 0xc001, crtc6845_register_w },
	{ 0xc044, 0xc047, pia_0_w },
	{ 0xc048, 0xc04b, pia_1_w },
	{ 0xc050, 0xc053, pia_2_w },
	{ 0xc100, 0xffff, MWA_ROM },
MEMORY_END


static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x007f, MRA_RAM },
	{ 0x0080, 0x0080, soundlatch_r },
	{ 0xf800, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x007f, MWA_RAM },
	{ 0xf800, 0xffff, MWA_ROM },
MEMORY_END



INPUT_PORTS_START( spiders )
    PORT_START      /* IN0 */
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_SERVICE1 )	/* PIA0 PA1 */
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )	/* PIA0 PA2 */
    PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 )
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START1 )
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
    PORT_BIT( 0x90, IP_ACTIVE_HIGH, IPT_UNUSED )

    PORT_START      /* IN1 */
    PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )
    PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
    PORT_BIT( 0xF3, IP_ACTIVE_HIGH, IPT_UNUSED )

    PORT_START  /* DSW1 */
    PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coinage ) )
    PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
    PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
    PORT_DIPSETTING(    0x06, DEF_STR( 4C_5C ) )
    PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
    PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
    PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
    PORT_BIT(0xf8, IP_ACTIVE_LOW,IPT_UNUSED)

    PORT_START  /* DSW2 */
    PORT_DIPNAME( 0x03, 0x03, "Play mode" )
    PORT_DIPSETTING(    0x00, "A A'" )
    PORT_DIPSETTING(    0x01, "A B'" )
    PORT_DIPSETTING(    0x02, "B A'" )
    PORT_DIPSETTING(    0x03, "B B'" )
    PORT_DIPNAME( 0x0c, 0x0c, "Spiders to complete belt" )
    PORT_DIPSETTING(    0x00, "14" )
    PORT_DIPSETTING(    0x04, "20" )
    PORT_DIPSETTING(    0x08, "26" )
    PORT_DIPSETTING(    0x0c, "16" )
    PORT_DIPNAME( 0x10, 0x10, "Spare Guns" )
    PORT_DIPSETTING(    0x00, "4" )
    PORT_DIPSETTING(    0x10, "3" )
    PORT_DIPNAME( 0x60, 0x60, "Score for bonus gun" )
    PORT_DIPSETTING(    0x00, "NONE" )
    PORT_DIPSETTING(    0x20, "20K" )
    PORT_DIPSETTING(    0x40, "25K" )
    PORT_DIPSETTING(    0x60, "15K" )
    PORT_DIPNAME( 0x80, 0x00, "Giant Spiders" )
    PORT_DIPSETTING(    0x00, "First screen" )
    PORT_DIPSETTING(    0x80, "Every screen" )

    PORT_START  /* DSW3 */
    PORT_DIPNAME( 0x01, 0x00, DEF_STR( Flip_Screen ) )
    PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x01, DEF_STR( On ) )
    PORT_DIPNAME( 0x02, 0x02, DEF_STR( Cabinet ) )
    PORT_DIPSETTING(    0x02, DEF_STR( Upright ) )
    PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
    PORT_DIPNAME( 0x1c, 0x00, "Vertical Adjust" )
    PORT_DIPSETTING(    0x00, "0" )
    PORT_DIPSETTING(    0x04, "1" )
    PORT_DIPSETTING(    0x08, "2" )
    PORT_DIPSETTING(    0x0c, "3" )
    PORT_DIPSETTING(    0x10, "4" )
    PORT_DIPSETTING(    0x14, "5" )
    PORT_DIPSETTING(    0x18, "6" )
    PORT_DIPSETTING(    0x1c, "7" )
    PORT_DIPNAME( 0xe0, 0x00, "Horizontal Adjust" )
    PORT_DIPSETTING(    0x00, "0" )
    PORT_DIPSETTING(    0x20, "1" )
    PORT_DIPSETTING(    0x40, "2" )
    PORT_DIPSETTING(    0x60, "3" )
    PORT_DIPSETTING(    0x80, "4" )
    PORT_DIPSETTING(    0xa0, "5" )
    PORT_DIPSETTING(    0xc0, "6" )
    PORT_DIPSETTING(    0xe0, "7" )

    PORT_START      /* Connected to PIA1 CA1 input */
    PORT_BIT( 0xFF, IP_ACTIVE_HIGH, IPT_VBLANK )

    PORT_START      /* Connected to PIA0 CB1 input */
    PORT_BITX(0xff, 0xff, IP_ACTIVE_LOW, "PS1 (Crosshatch)", KEYCODE_F1, IP_JOY_NONE )

INPUT_PORTS_END



static MACHINE_DRIVER_START( spiders )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6809, 2800000)
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_PERIODIC_INT(spiders_timed_irq , 25)   /* Timed Int  */

	MDRV_CPU_ADD(M6802,3000000/4)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(spiders)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 28*8-1)
	MDRV_PALETTE_LENGTH(8)

	MDRV_PALETTE_INIT(nyny)
	MDRV_VIDEO_START(spiders)
	MDRV_VIDEO_UPDATE(spiders)

	/* sound hardware */
MACHINE_DRIVER_END


ROM_START( spiders )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "sp-ic74",      0xc000, 0x1000, CRC(6a2578f6) SHA1(ddfe4fb2ccc925df7ae97821f8681b32e47630b4) )
	ROM_LOAD( "sp-ic73",      0xd000, 0x1000, CRC(d69b2f21) SHA1(ea2b07d19bd50c3b57da8fd8e13b8ab0e8ca3084) )
	ROM_LOAD( "sp-ic72",      0xe000, 0x1000, CRC(464125da) SHA1(94e9edd52e8bd72bbb5dc91b0aa11955e940799c) )
	ROM_LOAD( "sp-ic71",      0xf000, 0x1000, CRC(a9539b18) SHA1(2d02343a78a4a65e5a1798552cd015f16ad5423a) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for the audio CPU */
	ROM_LOAD( "sp-ic3",       0xf800, 0x0800, CRC(944d761e) SHA1(23b1f9234e0de678e96d1a6876d8d0a341150385) )

	ROM_REGION( 0x10000, REGION_GFX1, 0 )     /* 64k graphics block used at runtime */
	ROM_LOAD( "sp-ic33",      0x0000, 0x1000, CRC(b6731baa) SHA1(b551030df417b40f4a8488fc82a8b5809d3d32f6) )
	ROM_LOAD( "sp-ic25",      0x1000, 0x1000, CRC(baec64e7) SHA1(beb45e2e6270607c14cdf964c08fe320ce8236a0) )
	ROM_LOAD( "sp-ic24",      0x2000, 0x1000, CRC(a40a5517) SHA1(3f524c7dbbfe8aad7860d15c38d2702732895681) )
	ROM_LOAD( "sp-ic23",      0x3000, 0x1000, CRC(3ca08053) SHA1(20c5709d9650c426b91aed5318a9ab0a10009f17) )
	ROM_LOAD( "sp-ic22",      0x4000, 0x1000, CRC(07ea073c) SHA1(2e57831092730db5fbdb97c2d78d8842868906f4) )
	ROM_LOAD( "sp-ic21",      0x5000, 0x1000, CRC(41b344b4) SHA1(c0eac1e332da1eada062059ae742b666051da76c) )
	ROM_LOAD( "sp-ic20",      0x6000, 0x1000, CRC(4d37da5a) SHA1(37567d19596506385e9dcc7a7c0cf65120189ae0) )
ROM_END

ROM_START( spiders2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "sp-ic74",      0xc000, 0x1000, CRC(6a2578f6) SHA1(ddfe4fb2ccc925df7ae97821f8681b32e47630b4) )
	ROM_LOAD( "sp2.bin",      0xd000, 0x1000, CRC(cf71d12b) SHA1(369e91f637e8cd898354ddee04e24d4894968f79) )
	ROM_LOAD( "sp-ic72",      0xe000, 0x1000, CRC(464125da) SHA1(94e9edd52e8bd72bbb5dc91b0aa11955e940799c) )
	ROM_LOAD( "sp4.bin",      0xf000, 0x1000, CRC(f3d126bb) SHA1(ecc9156a7da661fa7543d7656aa7da77274e0842) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for the audio CPU */
	ROM_LOAD( "sp-ic3",       0xf800, 0x0800, CRC(944d761e) SHA1(23b1f9234e0de678e96d1a6876d8d0a341150385) )

	ROM_REGION( 0x10000, REGION_GFX1, 0 )     /* 64k graphics block used at runtime */
	ROM_LOAD( "sp-ic33",      0x0000, 0x1000, CRC(b6731baa) SHA1(b551030df417b40f4a8488fc82a8b5809d3d32f6) )
	ROM_LOAD( "sp-ic25",      0x1000, 0x1000, CRC(baec64e7) SHA1(beb45e2e6270607c14cdf964c08fe320ce8236a0) )
	ROM_LOAD( "sp-ic24",      0x2000, 0x1000, CRC(a40a5517) SHA1(3f524c7dbbfe8aad7860d15c38d2702732895681) )
	ROM_LOAD( "sp-ic23",      0x3000, 0x1000, CRC(3ca08053) SHA1(20c5709d9650c426b91aed5318a9ab0a10009f17) )
	ROM_LOAD( "sp-ic22",      0x4000, 0x1000, CRC(07ea073c) SHA1(2e57831092730db5fbdb97c2d78d8842868906f4) )
	ROM_LOAD( "sp-ic21",      0x5000, 0x1000, CRC(41b344b4) SHA1(c0eac1e332da1eada062059ae742b666051da76c) )
	ROM_LOAD( "sp-ic20",      0x6000, 0x1000, CRC(4d37da5a) SHA1(37567d19596506385e9dcc7a7c0cf65120189ae0) )
ROM_END



/* this is a newer version with just one bug fix */
GAMEX( 1981, spiders,  0,       spiders, spiders, 0, ROT270, "Sigma Enterprises Inc.", "Spiders (set 1)", GAME_NO_SOUND | GAME_NO_COCKTAIL )
GAMEX( 1981, spiders2, spiders, spiders, spiders, 0, ROT270, "Sigma Enterprises Inc.", "Spiders (set 2)", GAME_NO_SOUND | GAME_NO_COCKTAIL )
