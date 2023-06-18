/***************************************************************************

	Epos games

	driver by Zsolt Vasvari


	Notes:

	- To walk in IGMO, hold down button 2.
	- Super Glob seems like a later revision of The Glob, the most obvious
	  difference being an updated service mode.
	- These games don't have cocktail mode.
	- The divisor 4 was derived using the timing loop used to split the screen
	  in the middle.  This loop takes roughly 24200 cycles, giving
	  2500 + (24200 - 2500) * 2 * 60 = 2754000 = 2.75MHz for the CPU speed,
	  assuming 60 fps and a 2500 cycle VBLANK period.
	  This should be easy to check since the schematics are available, .
	- I think theglob2 is earlier than theglob.  They only differ in one routine,
	  but it appears to be a bug fix.  Also, theglob3 appears to be even older.

	To Do:

	- Super Blob uses a busy loop during the color test to split the screen
	  between the two palettes.  This effect is not emulated, but since both
	  halfs of the palette are identical, this is not an issue.  See $039c.
	  The other games have a different color test, not using the busy loop.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/8255ppi.h"
#include "epos.h"


static int counter = 0;

MACHINE_INIT( dealer );

WRITE_HANDLER( dealer_decrypt_rom )
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	if (offset & 0x04)
	{
		counter = counter + 1;
		if (counter < 0)
			counter = 0x0F;
	}
	else
	{
		counter = (counter - 1) & 0x0F;
	}

/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "PC %08x: ctr=%04x\n",activecpu_get_pc(),counter);*/

	switch(counter)
	{

		case 0x00:	cpu_setbank(1, &RAM[0x10000]);		break;
		case 0x01:	cpu_setbank(1, &RAM[0x20000]);		break;
		case 0x02:	cpu_setbank(1, &RAM[0x30000]);		break;
		case 0x03:	cpu_setbank(1, &RAM[0x40000]);		break;
		default:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "Invalid counter = %02X\n",counter);
			break;
	}
}


/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x77ff, MRA_ROM },
	{ 0x7800, 0xffff, MRA_RAM },
MEMORY_END


static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x77ff, MWA_ROM },
	{ 0x7800, 0x7fff, MWA_RAM },
	{ 0x8000, 0xffff, epos_videoram_w, &videoram, &videoram_size },
MEMORY_END


static MEMORY_READ_START( dealer_readmem )
	{ 0x0000, 0x6fff, MRA_BANK1 },
	{ 0x7000, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( dealer_writemem )
	{ 0x0000, 0x6fff, MWA_ROM },
	{ 0x7000, 0x7fff, MWA_RAM },
	{ 0x8000, 0xffff, epos_videoram_w, &videoram, &videoram_size },
MEMORY_END

/*************************************
 *
 *	Main CPU port handlers
 *
 *************************************/

static PORT_READ_START( readport )
	{ 0x00, 0x00, input_port_0_r },
	{ 0x01, 0x01, input_port_1_r },
	{ 0x02, 0x02, input_port_2_r },
	{ 0x03, 0x03, input_port_3_r },
PORT_END


static PORT_WRITE_START( writeport )
	{ 0x00, 0x00, watchdog_reset_w },
	{ 0x01, 0x01, epos_port_1_w },
	{ 0x02, 0x02, AY8910_write_port_0_w },
	{ 0x06, 0x06, AY8910_control_port_0_w },
PORT_END


static PORT_READ_START( dealer_readport )
	{ 0x10, 0x13, ppi8255_0_r },
	{ 0x38, 0x38, input_port_0_r },
PORT_END

static PORT_WRITE_START( dealer_writeport )
	{ 0x10, 0x13, ppi8255_0_w },
	{ 0x20, 0x24, dealer_decrypt_rom },
/*	{ 0x40, 0x40, watchdog_reset_w },*/
PORT_END

static ppi8255_interface ppi8255_intf =
{
	1, 					/* 1 chip */
	{ input_port_2_r },	/* Port A read */
	{ NULL },			/* Port B read */
	{ NULL },			/* Port C read */
	{ NULL },			/* Port A write */
	{ NULL },			/* Port B write */
	{ NULL },			/* Port C write */
};

/*************************************
 *
 *	Port definitions
 *
 *************************************/

/* I think the upper two bits of port 1 are used as a simple form of protection,
   so that ROMs couldn't be simply swapped.  Each game checks these bits and halts
   the processor if an unexpected value is read. */
   
INPUT_PORTS_START( eeekk )
	PORT_START      /* IN0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x50, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x40, "5" )
	PORT_DIPSETTING(    0x50, "6" )
	PORT_DIPNAME( 0x26, 0x04, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "1 (Easy)" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x20, "3" )
	PORT_DIPSETTING(    0x22, "4" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_DIPSETTING(    0x06, "6" )
	PORT_DIPSETTING(    0x24, "7" )
	PORT_DIPSETTING(    0x26, "8 (Hard)" )
	PORT_DIPNAME( 0x08, 0x08, "Extra Life Range" ) /* exact points value varies by 10000 for every level of difficulty chosen via the dips above */
	PORT_DIPSETTING(    0x08, "100000 - 170000 points" )
	PORT_DIPSETTING(    0x00, "20000 - 90000 points" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_START2 )
	PORT_BITX(0x10, IP_ACTIVE_LOW,  IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_SPECIAL )   /* this has to be LO */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SPECIAL )   /* this has to be LO */

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN3 */
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END


INPUT_PORTS_START( megadon )
	PORT_START      /* IN0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x02, 0x00, "Fuel Consumption" )
	PORT_DIPSETTING(    0x00, "Slow" )
	PORT_DIPSETTING(    0x02, "Fast" )
	PORT_DIPNAME( 0x04, 0x00, "Rotation" )
	PORT_DIPSETTING(    0x04, "Slow" )
	PORT_DIPSETTING(    0x00, "Fast" )
	PORT_DIPNAME( 0x08, 0x08, "ERG" )
	PORT_DIPSETTING(    0x08, "Easy" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x20, 0x20, "Enemy Fire Rate" )
	PORT_DIPSETTING(    0x20, "Slow" )
	PORT_DIPSETTING(    0x00, "Fast" )
	PORT_DIPNAME( 0x50, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x40, "5" )
	PORT_DIPSETTING(    0x50, "6" )
	PORT_DIPNAME( 0x80, 0x00, "Game Mode" )
	PORT_DIPSETTING(    0x00, "Arcade" )
	PORT_DIPSETTING(    0x80, "Contest" )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_START2 )
	PORT_BITX(0x10, IP_ACTIVE_LOW,  IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_SPECIAL )	/* this has to be HI */
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_SPECIAL )   /* this has to be HI */

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN3 */
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END


INPUT_PORTS_START( suprglob )
	PORT_START      /* IN0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x26, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x20, "3" )
	PORT_DIPSETTING(    0x22, "4" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_DIPSETTING(    0x06, "6" )
	PORT_DIPSETTING(    0x24, "7" )
	PORT_DIPSETTING(    0x26, "8" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "10000 + Difficulty * 10000" )
	PORT_DIPSETTING(    0x08, "90000 + Difficulty * 10000" )
	PORT_DIPNAME( 0x50, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x40, "5" )
	PORT_DIPSETTING(    0x50, "6" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_START2 )
	PORT_BITX(0x10, IP_ACTIVE_LOW,  IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* this has to be LO */
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_SPECIAL )   /* this has to be HI */

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN3 */
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END


INPUT_PORTS_START( igmo )
	PORT_START      /* IN0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x22, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "20000" )
	PORT_DIPSETTING(    0x02, "40000" )
	PORT_DIPSETTING(    0x20, "60000" )
	PORT_DIPSETTING(    0x22, "80000" )
	PORT_DIPNAME( 0x8c, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x0c, "4" )
	PORT_DIPSETTING(    0x80, "5" )
	PORT_DIPSETTING(    0x84, "6" )
	PORT_DIPSETTING(    0x88, "7" )
	PORT_DIPSETTING(    0x8c, "8" )
	PORT_DIPNAME( 0x50, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x40, "5" )
	PORT_DIPSETTING(    0x50, "6" )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_START2 )
	PORT_BITX(0x10, IP_ACTIVE_LOW,  IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_SPECIAL )	/* this has to be HI */
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_SPECIAL )   /* this has to be HI */

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN3 */
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END


INPUT_PORTS_START( dealer )
	PORT_START      /* IN0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) /*cancel*/
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) /*draw*/
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 ) /*stand*/
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON5 ) /*play*/
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON6 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON7 ) /*coin in*/
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON8 )
INPUT_PORTS_END


/*************************************
 *
 *	Sound interfaces
 *
 *************************************/

static struct AY8910interface ay8912_interface =
{
	1,	/* 1 chip */
	11000000/4,	/* 2.75 MHz */
	{ 50 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};



/*************************************
 *
 *	Machine drivers
 *
 *************************************/

static MACHINE_DRIVER_START( epos )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 11000000/4)	/* 2.75 MHz (see notes) */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_PORTS(readport,writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(272, 241)
	MDRV_VISIBLE_AREA(0, 271, 0, 235)
	MDRV_PALETTE_LENGTH(32)

	MDRV_PALETTE_INIT(epos)
	MDRV_VIDEO_START(generic_bitmapped)
	MDRV_VIDEO_UPDATE(epos)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8912_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( dealer )
	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 11000000/4)	/* 2.75 MHz (see notes) */
	MDRV_CPU_MEMORY(dealer_readmem,dealer_writemem)
	MDRV_CPU_PORTS(dealer_readport,dealer_writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(272, 241)
	MDRV_VISIBLE_AREA(0, 271, 0, 235)
	MDRV_PALETTE_LENGTH(32)

	MDRV_PALETTE_INIT(epos)
	MDRV_VIDEO_START(generic_bitmapped)
	MDRV_VIDEO_UPDATE(epos)
	MDRV_MACHINE_INIT(dealer)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8912_interface)
MACHINE_DRIVER_END


/*************************************
 *
 *	ROM definitions
 *
 *************************************/

ROM_START( megadon )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )       /* 64k for code */
	ROM_LOAD( "2732u10b.bin",   0x0000, 0x1000, CRC(af8fbe80) SHA1(2d7857616462112fe17343a9357ee51d8f965a0f) )
	ROM_LOAD( "2732u09b.bin",   0x1000, 0x1000, CRC(097d1e73) SHA1(b6141155b2c63c33a367dd18fe53ff9f01b99380) )
	ROM_LOAD( "2732u08b.bin",   0x2000, 0x1000, CRC(526784da) SHA1(7d9f43dc6975a018bec95982029ce7ac9f675869) )
	ROM_LOAD( "2732u07b.bin",   0x3000, 0x1000, CRC(5b060910) SHA1(98a719bf0ffe8010437565de681aaefa647d9a6c) )
	ROM_LOAD( "2732u06b.bin",   0x4000, 0x1000, CRC(8ac8af6d) SHA1(53c123f0e9f0443737c39c01dbdb685189cffa92) )
	ROM_LOAD( "2732u05b.bin",   0x5000, 0x1000, CRC(052bb603) SHA1(eb74a9563f44cca50dc2c475e4a376ed14e4f56f) )
	ROM_LOAD( "2732u04b.bin",   0x6000, 0x1000, CRC(9b8b7e92) SHA1(051ad9a8ba51740a865e3c95a738658b30bbbe60) )
	ROM_LOAD( "2716u11b.bin",   0x7000, 0x0800, CRC(599b8b61) SHA1(e687c6f475a0fead3e47f05b1d1b3b29cf4a83a1) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "74s288.bin",     0x0000, 0x0020, CRC(c779ea99) SHA1(7702ae3684579950b36274ea91d4267c96faeeb8) )
ROM_END


ROM_START( catapult )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )       /* 64k for code */
	ROM_LOAD( "co3223.u10",     0x0000, 0x1000, CRC(50abcfd2) SHA1(13ce04addc7bcaa1ec6659da26b1c13ed9dc28f9) )
	ROM_LOAD( "co3223.u09",     0x1000, 0x1000, CRC(fd5a9a1c) SHA1(512374e8450459537ba2cc41e7d0178052445316) )
	ROM_LOAD( "co3223.u08",     0x2000, 0x1000, BAD_DUMP CRC(4bfc36f3) SHA1(b916805eed40cfeff0c1b0cb3cdcbcc6e362a236)  ) /* BADADDR xxxx-xxxxxxx */
	ROM_LOAD( "co3223.u07",     0x3000, 0x1000, CRC(4113bb99) SHA1(3cebb874dae211d75082209e913d4afa4f621de1) )
	ROM_LOAD( "co3223.u06",     0x4000, 0x1000, CRC(966bb9f5) SHA1(1a217c6f7a88c58e0deae0290bc5ddd2789d18eb) )
	ROM_LOAD( "co3223.u05",     0x5000, 0x1000, CRC(65f9fb9a) SHA1(63b616a736d9e39a8f2f76889fd7c5fe4128a966) )
	ROM_LOAD( "co3223.u04",     0x6000, 0x1000, CRC(648453bc) SHA1(8e4538aedad4d32bd046aad474dbcc689ee8fe53) )
	ROM_LOAD( "co3223.u11",     0x7000, 0x0800, CRC(08fb8c28) SHA1(0b08cc2727a54e0ad7472234be0f637b46bc3253) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "co3223.u66",     0x0000, 0x0020, CRC(e7de76a7) SHA1(101ce85459a59c0d01ce3ea96480f1f8413a788e) )
ROM_END


ROM_START( eeekk )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "u10_e12063.u10", 0x0000, 0x1000, CRC(edd05de2) SHA1(25dfa7ad2e29b1ca9ce9bb36bf1a573baabb4d5b) )
	ROM_LOAD( "u9_e12063.u9",   0x1000, 0x1000, CRC(6f57114a) SHA1(417b910a4343da026426b4cfd0a83b9142c87353) )
	ROM_LOAD( "u8_e12063.u8",   0x2000, 0x1000, CRC(bcb0ebbd) SHA1(a2a00dedee12d6006817021e98fb44b2339127a0) )
	ROM_LOAD( "u7_e12063.u7",   0x3000, 0x1000, CRC(a0df8f77) SHA1(ee2afed25ab32bf09b14e8638d03b6e2f8e6b337) )
	ROM_LOAD( "u6_e12063.u6",   0x4000, 0x1000, CRC(61953b0a) SHA1(67bcb4286e39cdda20684a4f580392468b08800e) )
	ROM_LOAD( "u5_e12063.u5",   0x5000, 0x1000, CRC(4c22c6d9) SHA1(94a8fc951994746f8ccfb77d80f8b98fde8a6f33) )
	ROM_LOAD( "u4_e12063.u4",   0x6000, 0x1000, CRC(3d341208) SHA1(bc4d2567df2779d97e718376c4bf682ba459c01e) )
	ROM_LOAD( "u11_e12063.u11", 0x7000, 0x0800, CRC(417faff0) SHA1(7965155ee32694ea9a10245db73d8beef229408c) )
	
	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "74s288.u66",     0x0000, 0x0020, CRC(f2078c38) SHA1(7bfd49932a6fd8840514b7af93a64cedb248122d) )
ROM_END


ROM_START( suprglob )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )       /* 64k for code */
	ROM_LOAD( "u10",			0x0000, 0x1000, CRC(c0141324) SHA1(a54bd71da233eb22f45da630693fddd5a0bcf25b) )
	ROM_LOAD( "u9",				0x1000, 0x1000, CRC(58be8128) SHA1(534f0a093b3ff577a2a5461498bc11ce14dc6d97) )
	ROM_LOAD( "u8",				0x2000, 0x1000, CRC(6d088c16) SHA1(0929ea1b58eab997b5d9c9642f8b47557a4045f1) )
	ROM_LOAD( "u7",				0x3000, 0x1000, CRC(b2768203) SHA1(9de52f4dbe6a46ea1b9b7f9cf70378211d372353) )
	ROM_LOAD( "u6",				0x4000, 0x1000, CRC(976c8f46) SHA1(120c76eff8c04ccb5ad945c4333e8c9de0cbc3af) )
	ROM_LOAD( "u5",				0x5000, 0x1000, CRC(340f5290) SHA1(2e5fa0c41d1626e5a435f2c55eec0bcdcb004223) )
	ROM_LOAD( "u4",				0x6000, 0x1000, CRC(173bd589) SHA1(25690a0c3cd0e017f8d220d8fbf2eaeb86f05fc5) )
	ROM_LOAD( "u11",			0x7000, 0x0800, CRC(d45b740d) SHA1(54c15f378b6d91ea1aba0a51921178bb15854079) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.u66",		0x0000, 0x0020, CRC(f4f6ddc5) SHA1(cab915acbefb5f451f538dd538bf9b3dd14bb1f5) )
ROM_END


ROM_START( theglob )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )       /* 64k for code */
	ROM_LOAD( "globu10.bin",	0x0000, 0x1000, CRC(08fdb495) SHA1(739efa676b5a3df36a6061382aeb8c2d495ba23f) )
	ROM_LOAD( "globu9.bin",		0x1000, 0x1000, CRC(827cd56c) SHA1(3aedc1cefb463cf6b31befb33e50c832dc2e3941) )
	ROM_LOAD( "globu8.bin",		0x2000, 0x1000, CRC(d1219966) SHA1(571349f9c978fdcf826a0c66c3fb11a9e27b240a) )
	ROM_LOAD( "globu7.bin",		0x3000, 0x1000, CRC(b1649da7) SHA1(1509d48a72e545195e45d1170cdb113c6aecc8d9) )
	ROM_LOAD( "globu6.bin",		0x4000, 0x1000, CRC(b3457e67) SHA1(1347bdf085ad69879f9a9e7e4ed1ca4869e8e8cd) )
	ROM_LOAD( "globu5.bin",		0x5000, 0x1000, CRC(89d582cd) SHA1(f331c7a2fce606153992abb312c5406251a7fb3b) )
	ROM_LOAD( "globu4.bin",		0x6000, 0x1000, CRC(7ee9fdeb) SHA1(a8e0dd5d1cdcff132edc0eb182b66656ce244fa1) )
	ROM_LOAD( "globu11.bin",	0x7000, 0x0800, CRC(9e05dee3) SHA1(751799b23f0e664f59d3785b438ec3ae9f5bab2c) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.u66",		0x0000, 0x0020, CRC(f4f6ddc5) SHA1(cab915acbefb5f451f538dd538bf9b3dd14bb1f5) )
ROM_END


ROM_START( theglob2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )       /* 64k for code */
	ROM_LOAD( "611293.u10",		0x0000, 0x1000, CRC(870af7ce) SHA1(f901619663313a72997f30ccecdeac8294fe200e) )
	ROM_LOAD( "611293.u9",		0x1000, 0x1000, CRC(a3679782) SHA1(fbc26ae98e2bf10272d61159b084d78a6f410374) )
	ROM_LOAD( "611293.u8",		0x2000, 0x1000, CRC(67499d1a) SHA1(dce7041df5ed1847e0ffc82672d09e00b16de3a9) )
	ROM_LOAD( "611293.u7",		0x3000, 0x1000, CRC(55e53aac) SHA1(20a428db287e8b7fb55cb9fe1a1ed0196481114c) )
	ROM_LOAD( "611293.u6",		0x4000, 0x1000, CRC(c64ad743) SHA1(572ff6acb9b2281581974646e96699d7d2388aff) )
	ROM_LOAD( "611293.u5",		0x5000, 0x1000, CRC(f93c3203) SHA1(8cb88b5202e99d206eccf7d25e168cf23acee19b) )
	ROM_LOAD( "611293.u4",		0x6000, 0x1000, CRC(ceea0018) SHA1(511430539429ef0e5368f7b605f2e680ca9038bc) )
	ROM_LOAD( "611293.u11",		0x7000, 0x0800, CRC(6ac83f9b) SHA1(b1e8482ec04107f0e595a714b7c0f70571aca6e5) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.u66",		0x0000, 0x0020, CRC(f4f6ddc5) SHA1(cab915acbefb5f451f538dd538bf9b3dd14bb1f5) )
ROM_END


ROM_START( theglob3 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )       /* 64k for code */
	ROM_LOAD( "theglob3.u10",	0x0000, 0x1000, CRC(969cfaf6) SHA1(b63226b8694640d6452bca12755780d1b52d1d3c) )
	ROM_LOAD( "theglob3.u9",	0x1000, 0x1000, CRC(8e6c010a) SHA1(ec9627742ce52eb29bbafc9d0555d16ac7146f2e) )
	ROM_LOAD( "theglob3.u8",	0x2000, 0x1000, CRC(1c1ca5c8) SHA1(6e5f9d7f9f016a72003433375c806c5f921ed423) )
	ROM_LOAD( "theglob3.u7",	0x3000, 0x1000, CRC(a54b9d22) SHA1(3db96d1f55642ecf1ebc76387cac76e8f9721919) )
	ROM_LOAD( "theglob3.u6",	0x4000, 0x1000, CRC(5a6f82a9) SHA1(ea92ad949373e8b1f06c65f243ceedad2fdcd934) )
	ROM_LOAD( "theglob3.u5",	0x5000, 0x1000, CRC(72f935db) SHA1(d7023cf5f16a77a42590a9c97c2690ac0e3d282a) )
	ROM_LOAD( "theglob3.u4",	0x6000, 0x1000, CRC(81db53ad) SHA1(a1e4aa8e08ca0f585b3638a3849a465977d44af0) )
	ROM_LOAD( "theglob3.u11",	0x7000, 0x0800, CRC(0e2e6359) SHA1(f231637ad4c997406989cf5a701d26c95e69171e) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.u66",		0x0000, 0x0020, CRC(f4f6ddc5) SHA1(cab915acbefb5f451f538dd538bf9b3dd14bb1f5) )
ROM_END


ROM_START( igmo )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )       /* 64k for code */
	ROM_LOAD( "igmo-u10.732",	0x0000, 0x1000, CRC(a9f691a4) SHA1(e3f2dc41bd8760fc52e99b7e9faa12c7cf51ffe0) )
	ROM_LOAD( "igmo-u9.732",	0x1000, 0x1000, CRC(3c133c97) SHA1(002b5aff6b947b6a9cbabeed5be798c1ddf2bda1) )
	ROM_LOAD( "igmo-u8.732",	0x2000, 0x1000, CRC(5692f8d8) SHA1(6ab50775dff49330a85fbfb2d4d4c3a2e54df3d1) )
	ROM_LOAD( "igmo-u7.732",	0x3000, 0x1000, CRC(630ae2ed) SHA1(0c293b6192e703b16ed20c277c706ae90773f477) )
	ROM_LOAD( "igmo-u6.732",	0x4000, 0x1000, CRC(d3f20e1d) SHA1(c0e0b542ac020adc085ec90c2462c6544098447e) )
	ROM_LOAD( "igmo-u5.732",	0x5000, 0x1000, CRC(e26bb391) SHA1(ba0e44c02fbb36e18e0d779d46bb992e6aba6cf1) )
	ROM_LOAD( "igmo-u4.732",	0x6000, 0x1000, CRC(762a4417) SHA1(7fed5221950e3e1ce41c0b4ded44597a242a0177) )
	ROM_LOAD( "igmo-u11.716",	0x7000, 0x0800, CRC(8c675837) SHA1(2725729693960b53ea01ebffa0a81df2cd425890) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.u66",		0x0000, 0x0020, CRC(1ba03ffe) SHA1(f5692c06ae6d20c010430c8d08f5c60e78d36dc9) )
ROM_END


ROM_START( dealer )
	ROM_REGION( 0x50000, REGION_CPU1, 0 )
	ROM_LOAD( "u1.bin",			0x0000, 0x2000, CRC(e06f3563) SHA1(0d58cd1f2e1ca89adb9c64d7dd520bb1f2d50f1a) )
	ROM_LOAD( "u2.bin",			0x2000, 0x2000, CRC(726bbbd6) SHA1(3538f3d655899c2a0f984c43fb7545ea4be1b231) )
	ROM_LOAD( "u3.bin",			0x4000, 0x2000, CRC(ab721455) SHA1(a477da0590e0431172baae972e765473e19dcbff) )
	ROM_LOAD( "u4.bin",			0x6000, 0x2000, BAD_DUMP CRC(ddb903e4) SHA1(4c06a2048b1c6989c363b110a17c33180025b9c8) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.u66",		0x0000, 0x0020, NO_DUMP )	/* missing */
ROM_END

MACHINE_INIT( dealer )
{
	cpu_setbank(1, memory_region(REGION_CPU1) + 0x10000);

	ppi8255_init(&ppi8255_intf);
}

DRIVER_INIT( dealer )
{
	unsigned char *ROM = memory_region(REGION_CPU1);
	int A;
	int oldbyte,newbyte;

	/* Key 1 */
	for (A = 0;A < 0x7000;A++)
	{
		oldbyte = ROM[A];

		newbyte = 0;
		newbyte += ((oldbyte & 0x01 ) ^ 0x01) * 0x10;
		newbyte += ((oldbyte & 0x02 ) ^ 0x00) * 0x01;
		newbyte += ((oldbyte & 0x04 ) ^ 0x04) * 0x20;
		newbyte += ((oldbyte & 0x08 ) ^ 0x08) / 0x08;
		newbyte += ((oldbyte & 0x10 ) ^ 0x10) * 0x02;
		newbyte += ((oldbyte & 0x20 ) ^ 0x20) / 0x04;
		newbyte += ((oldbyte & 0x40 ) ^ 0x00) * 0x01;
		newbyte += ((oldbyte & 0x80 ) ^ 0x80) / 0x20;

		ROM[A + 0x10000] = newbyte;
	}

	/* Key 2 */
	for (A = 0;A < 0x7000;A++)
	{
		oldbyte = ROM[A+0x10000];

		newbyte = 0;
		newbyte += ((oldbyte & 0x01 ) ^ 0x00) * 0x01;
		newbyte += ((oldbyte & 0x02 ) ^ 0x00) * 0x01;
		newbyte += ((oldbyte & 0x04 ) ^ 0x00) * 0x01;
		newbyte += ((oldbyte & 0x08 ) ^ 0x00) * 0x01;
		newbyte += ((oldbyte & 0x10 ) ^ 0x00) * 0x02;
		newbyte += ((oldbyte & 0x20 ) ^ 0x00) * 0x02;
		newbyte += ((oldbyte & 0x40 ) ^ 0x00) / 0x04;
		newbyte += ((oldbyte & 0x80 ) ^ 0x00) * 0x01;

		ROM[A + 0x20000 ] = newbyte;
	}

	/* Key 3 */
	for (A = 0;A < 0x7000;A++)
	{
		oldbyte = ROM[A+0x10000];

		newbyte = 0;
		newbyte += ((oldbyte & 0x01 ) ^ 0x01) * 0x04;
		newbyte += ((oldbyte & 0x02 ) ^ 0x00) / 0x02;
		newbyte += ((oldbyte & 0x04 ) ^ 0x00) / 0x02;
		newbyte += ((oldbyte & 0x08 ) ^ 0x00) * 0x01;
		newbyte += ((oldbyte & 0x10 ) ^ 0x00) * 0x01;
		newbyte += ((oldbyte & 0x20 ) ^ 0x00) * 0x01;
		newbyte += ((oldbyte & 0x40 ) ^ 0x00) * 0x01;
		newbyte += ((oldbyte & 0x80 ) ^ 0x00) * 0x01;

		ROM[A + 0x30000 ] = newbyte;
	}

	/* Key 4 */
	for (A = 0;A < 0x7000;A++)
	{

/* there is not enough data to determine the last key.
   the code in question is this:

     55  =   32 		ld (793e),a
2f59 5c   	  3e
2f5a 79      78

2f5b 55   32     		ld (79xx),a
2f5c f7   53 or  D2 or  F3
2f5d 79   78

2f5e 55    32      		ld (79xx),a
2f5f cd	1B 56 77 9A BB F6
     79    78
It writes data to be read later.
I don't know the lsb of the writes.  I do know:
if 2f5c = 53 then 2f5f = 9A or BB or F6
          d2             1B 77 BB
          f3             1B 56 9A

Someone will have to check the code to see what it's looking for.
As usual I've had to make some assumptions so I can't guarantee 100% accuracy
Dave
*/

		oldbyte = ROM[A+0x10000];

		newbyte = 0;
#if 1
/*53:9a*/
		newbyte += ((oldbyte & 0x01 ) ^ 0x01) * 0x04;
		newbyte += ((oldbyte & 0x02 ) ^ 0x00) / 0x02;
		newbyte += ((oldbyte & 0x04 ) ^ 0x00) / 0x02;
		newbyte += ((oldbyte & 0x08 ) ^ 0x00) * 0x01;
		newbyte += ((oldbyte & 0x10 ) ^ 0x10) * 0x08;
		newbyte += ((oldbyte & 0x20 ) ^ 0x00) * 0x02;
		newbyte += ((oldbyte & 0x40 ) ^ 0x00) / 0x04;
		newbyte += ((oldbyte & 0x80 ) ^ 0x80) / 0x04;
#endif

#if 0
/*53:bb*/
		newbyte += ((oldbyte & 0x01 ) ^ 0x01) * 0x04;
		newbyte += ((oldbyte & 0x02 ) ^ 0x02) * 0x10;
		newbyte += ((oldbyte & 0x04 ) ^ 0x00) / 0x02;
		newbyte += ((oldbyte & 0x08 ) ^ 0x00) * 0x01;
		newbyte += ((oldbyte & 0x10 ) ^ 0x10) * 0x08;
		newbyte += ((oldbyte & 0x20 ) ^ 0x00) * 0x02;
		newbyte += ((oldbyte & 0x40 ) ^ 0x00) / 0x04;
		newbyte += ((oldbyte & 0x80 ) ^ 0x00) / 0x80;
#endif

#if 0
/*f3:9a*/
		newbyte += ((oldbyte & 0x01 ) ^ 0x01) * 0x04;
		newbyte += ((oldbyte & 0x02 ) ^ 0x00) / 0x02;
		newbyte += ((oldbyte & 0x04 ) ^ 0x00) / 0x02;
		newbyte += ((oldbyte & 0x08 ) ^ 0x00) * 0x01;
		newbyte += ((oldbyte & 0x10 ) ^ 0x00) * 0x02;
		newbyte += ((oldbyte & 0x20 ) ^ 0x00) * 0x02;
		newbyte += ((oldbyte & 0x40 ) ^ 0x00) / 0x04;
		newbyte += ((oldbyte & 0x80 ) ^ 0x00) * 0x01;
#endif

#if 0
/*f3:1b*/
		newbyte += ((oldbyte & 0x01 ) ^ 0x01) * 0x04;
		newbyte += ((oldbyte & 0x02 ) ^ 0x00) * 0x40;
		newbyte += ((oldbyte & 0x04 ) ^ 0x00) / 0x02;
		newbyte += ((oldbyte & 0x08 ) ^ 0x00) * 0x01;
		newbyte += ((oldbyte & 0x10 ) ^ 0x00) * 0x02;
		newbyte += ((oldbyte & 0x20 ) ^ 0x00) * 0x02;
		newbyte += ((oldbyte & 0x40 ) ^ 0x00) / 0x04;
		newbyte += ((oldbyte & 0x80 ) ^ 0x00) / 0x80;
#endif

#if 0
/*d2:1b*/
		newbyte += ((oldbyte & 0x01 ) ^ 0x01) * 0x04;
		newbyte += ((oldbyte & 0x02 ) ^ 0x00) * 0x40;
		newbyte += ((oldbyte & 0x04 ) ^ 0x00) / 0x02;
		newbyte += ((oldbyte & 0x08 ) ^ 0x00) * 0x01;
		newbyte += ((oldbyte & 0x10 ) ^ 0x10) / 0x10;
		newbyte += ((oldbyte & 0x20 ) ^ 0x00) * 0x02;
		newbyte += ((oldbyte & 0x40 ) ^ 0x00) / 0x04;
		newbyte += ((oldbyte & 0x80 ) ^ 0x80) / 0x04;
#endif

		ROM[A + 0x40000 ] = newbyte;
	}
}

/*************************************
 *
 *	Game drivers
 *
 *************************************/

/* Epos Tristar 8000 */ 
 
GAME ( 1982, megadon,  0,        epos,  megadon,  0,	  ROT270, "Epos Corporation (Photar Industries license)", "Megadon" )
GAMEX( 1982, catapult, 0,        epos,  igmo,     0,	  ROT270, "Epos Corporation", "Catapult", GAME_NOT_WORKING) /* bad rom, hold f2 for test mode */
GAME ( 1983, eeekk,    0,        epos,  eeekk,    0,    ROT270, "Epos Corporation", "Eeekk!" )
GAME ( 1983, suprglob, 0,        epos,  suprglob, 0,	  ROT270, "Epos Corporation", "Super Glob" )
GAME ( 1983, theglob,  suprglob, epos,  suprglob, 0,	  ROT270, "Epos Corporation", "The Glob" )
GAME ( 1983, theglob2, suprglob, epos,  suprglob, 0,	  ROT270, "Epos Corporation", "The Glob (earlier)" )
GAME ( 1983, theglob3, suprglob, epos,  suprglob, 0,	  ROT270, "Epos Corporation", "The Glob (set 3)" )
GAME ( 1984, igmo,     0,        epos,  igmo,     0,	  ROT270, "Epos Corporation", "IGMO" )

/* Epos Tristar 9000 */ 

GAMEX( 19??, dealer,   0,        dealer, dealer,  dealer, ROT270, "Epos Corporation", "The Dealer", GAME_NOT_WORKING)
