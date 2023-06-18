/***************************************************************************

	Coors Light Bowling/Bowl-O-Rama hardware

	driver by Zsolt Vasvari

	Games supported:
		* Capcom/Coors Light Bowling
		* Bowlorama

	Known issues:
		* none

****************************************************************************

	CPU Board:

	0000-3fff     3 Graphics ROMS mapped in using 0x4800 (Coors Light Bowling only)
	0000-001f		Turbo board area (Bowl-O-Rama only) See Below.
	4000          Display row selected
	4800          Graphics ROM select
	5000-57ff     Battery backed up RAM (Saves machine state after shut off)
	           Enter setup menu by holding down the F2 key on the
	           high score screen
	5800-5fff		TMS34061 area

	           First 0x20 bytes of each row provide a 16 color palette for this
	           row. 2 bytes per color: 0000RRRR GGGGBBBB.

	           Remaining 0xe0 bytes contain 2 pixels each for a total of
	           448 pixels, but only 360 seem to be displayed.
	           (Each row appears vertically because the monitor is rotated)

	6000          Sound command
	6800			Trackball Reset. Double duties as a watchdog.
	7000          Input port 1    Bit 0-3 Trackball Vertical Position
							  	Bit 4   Player 2 Hook Left
								Bit 5   Player 2 Hook Right
								Bit 6   Upright/Cocktail DIP Switch
	                           Bit 7   Coin 2
	7800          Input port 2    Bit 0-3 Trackball Horizontal Positon
	                           Bit 4   Player 1 Hook Left
	                           Bit 5   Player 1 Hook Right
	                           Bit 6   Start
	                           Bit 7   Coin 1
	8000-ffff		ROM


	Sound Board:

	0000-07ff		RAM
	1000-1001		YM2203
			  	Port A D7 Read  is ticket sensor
				Port B D7 Write is ticket dispenser enable
				Port B D6 looks like a diagnostics LED to indicate that
				          the PCB is operating. It's pulsated by the
						  sound CPU. It is kind of pointless to emulate it.
	2000			Not hooked up according to the schematics
	6000			DAC write
	7000			Sound command read (0x34 is used to dispense a ticket)
	8000-ffff		ROM


	Turbo Board Layout (Plugs in place of GR0):

	Bowl-O-Rama	Copyright 1991 P&P Marketing
				Marquee says "EXIT Entertainment"

				This portion: Mike Appolo with the help of Andrew Pines.
				Andrew was one of the game designers for Capcom Bowling,
				Coors Light Bowling, Strata Bowling, and Bowl-O-Rama.

				This game was an upgrade for Capcom Bowling and included a
				"Turbo PCB" that had a GAL address decoder / data mask

	Memory Map for turbo board (where GR0 is on Capcom Bowling PCBs:

	0000   		Read Mask
	0001-0003		Unused
	0004  		Read Data
	0005-0007		Unused
	0008  		GR Address High Byte (GR17-16)
	0009-0016		Unused
	0017			GR Address Middle Byte (GR15-0 written as a word to 0017-0018)
	0018  		GR address Low byte
	0019-001f		Unused

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/ticket.h"
#include "cpu/m6809/m6809.h"
#include "capbowl.h"



/*************************************
 *
 *	NVRAM
 *
 *************************************/

static NVRAM_HANDLER( capbowl )
{
	if (read_or_write)
		mame_fwrite(file,generic_nvram,generic_nvram_size);
	else
	{
		if (file)
			mame_fread(file,generic_nvram,generic_nvram_size);
		else
		{
			/* invalidate nvram to make the game initialize it.
			   A 0xff fill will cause the game to malfunction, so we use a
			   0x01 fill which seems OK */
			memset(generic_nvram,0x01,generic_nvram_size);
		}
	}
}



/*************************************
 *
 *	Sound commands
 *
 *************************************/

static WRITE_HANDLER( capbowl_sndcmd_w )
{
	cpu_set_irq_line(1, M6809_IRQ_LINE, HOLD_LINE);
	soundlatch_w(offset, data);
}



/*************************************
 *
 *	Handler called by the 2203 emulator
 *	when the internal timers cause an IRQ
 *
 *************************************/

static void firqhandler(int irq)
{
	cpu_set_irq_line(1, 1, irq ? ASSERT_LINE : CLEAR_LINE);
}



/*************************************
 *
 *	NMI is to trigger the self test.
 *	We use a fake input port to tie
 *	that event to a keypress
 *
 *************************************/

static INTERRUPT_GEN( capbowl_interrupt )
{
	if (readinputport(4) & 1)	/* get status of the F2 key */
		cpu_set_irq_line(0, IRQ_LINE_NMI, PULSE_LINE);	/* trigger self test */
}



/*************************************
 *
 *	Trackball input handlers
 *
 *************************************/

static int track[2];

static READ_HANDLER( track_0_r )
{
	return (input_port_0_r(offset) & 0xf0) | ((input_port_2_r(offset) - track[0]) & 0x0f);
}

static READ_HANDLER( track_1_r )
{
	return (input_port_1_r(offset) & 0xf0) | ((input_port_3_r(offset) - track[1]) & 0x0f);
}

static WRITE_HANDLER( track_reset_w )
{
	/* reset the trackball counters */
	track[0] = input_port_2_r(offset);
	track[1] = input_port_3_r(offset);

	watchdog_reset_w(offset,data);
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ_START( capbowl_readmem )
	{ 0x0000, 0x3fff, MRA_BANK1 },
	{ 0x5000, 0x57ff, MRA_RAM },
	{ 0x5800, 0x5fff, capbowl_tms34061_r },
	{ 0x7000, 0x7000, track_0_r },	/* + other inputs */
	{ 0x7800, 0x7800, track_1_r },	/* + other inputs */
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END


static MEMORY_READ_START( bowlrama_readmem )
	{ 0x0000, 0x001f, bowlrama_turbo_r },
	{ 0x5000, 0x57ff, MRA_RAM },
	{ 0x5800, 0x5fff, capbowl_tms34061_r },
	{ 0x7000, 0x7000, track_0_r },	/* + other inputs */
	{ 0x7800, 0x7800, track_1_r },	/* + other inputs */
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END


static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x001f, bowlrama_turbo_w },	/* Bowl-O-Rama only */
	{ 0x4000, 0x4000, MWA_RAM, &capbowl_rowaddress },
	{ 0x4800, 0x4800, capbowl_rom_select_w },
	{ 0x5000, 0x57ff, MWA_RAM, &generic_nvram, &generic_nvram_size },
	{ 0x5800, 0x5fff, capbowl_tms34061_w },
	{ 0x6000, 0x6000, capbowl_sndcmd_w },
	{ 0x6800, 0x6800, track_reset_w },	/* + watchdog */
MEMORY_END



/*************************************
 *
 *	Sound CPU memory handlers
 *
 *************************************/

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x07ff, MRA_RAM },
	{ 0x1000, 0x1000, YM2203_status_port_0_r },
	{ 0x1001, 0x1001, YM2203_read_port_0_r },
	{ 0x7000, 0x7000, soundlatch_r },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END


static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x07ff, MWA_RAM},
	{ 0x1000, 0x1000, YM2203_control_port_0_w },
	{ 0x1001, 0x1001, YM2203_write_port_0_w },
	{ 0x2000, 0x2000, MWA_NOP },  /* Not hooked up according to the schematics */
	{ 0x6000, 0x6000, DAC_0_data_w },
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( capbowl )
	PORT_START	/* IN0 */
	/* low 4 bits are for the trackball */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Cabinet ) ) /* This version of Bowl-O-Rama */
	PORT_DIPSETTING(    0x40, DEF_STR( Upright ) )			   /* is Upright only */
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START	/* IN1 */
	/* low 4 bits are for the trackball */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* FAKE */
	PORT_ANALOG( 0xff, 0x00, IPT_TRACKBALL_Y | IPF_REVERSE, 20, 40, 0, 0 )

	PORT_START	/* FAKE */
	PORT_ANALOG( 0xff, 0x00, IPT_TRACKBALL_X, 20, 40, 0, 0 )

	PORT_START	/* FAKE */
	/* This fake input port is used to get the status of the F2 key, */
	/* and activate the test mode, which is triggered by a NMI */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
INPUT_PORTS_END



/*************************************
 *
 *	Sound definitions
 *
 *************************************/

static struct YM2203interface ym2203_interface =
{
	1,			/* 1 chip */
	4000000,	/* 4 MHz */
	{ YM2203_VOL(40,40) },
	{ ticket_dispenser_r },
	{ 0 },
	{ 0 },
	{ ticket_dispenser_w },  /* Also a status LED. See memory map above */
	{ firqhandler }
};


static struct DACinterface dac_interface =
{
	1,
	{ 100 }
};



/*************************************
 *
 *	Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( capbowl )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M6809, 2000000)
	MDRV_CPU_MEMORY(capbowl_readmem,writemem)
	MDRV_CPU_VBLANK_INT(capbowl_interrupt,1)
	
	MDRV_CPU_ADD(M6809,2000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	
	MDRV_FRAMES_PER_SECOND(57)
	MDRV_VBLANK_DURATION(5000)
	
	MDRV_MACHINE_INIT(capbowl)
	MDRV_NVRAM_HANDLER(capbowl)
	
	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(360, 256)
	MDRV_VISIBLE_AREA(0, 359, 0, 244)
	MDRV_PALETTE_LENGTH(4096)
	
	MDRV_VIDEO_START(capbowl)
	MDRV_VIDEO_UPDATE(capbowl)
	
	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, ym2203_interface)
	MDRV_SOUND_ADD(DAC,    dac_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( bowlrama )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(capbowl)
	
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(bowlrama_readmem,writemem)
	
	/* video hardware */
	MDRV_VISIBLE_AREA(0, 359, 0, 239)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definitions
 *
 *************************************/

ROM_START( capbowl )
	ROM_REGION( 0x28000, REGION_CPU1, 0 )
	ROM_LOAD( "u6",           0x08000, 0x8000, CRC(14924c96) SHA1(d436c5115873c9c2bc7657acff1cf7d99c0c5d6d) )
	ROM_LOAD( "gr0",          0x10000, 0x8000, CRC(ef53ca7a) SHA1(219dc342595bfd23c1336f3e167e40ff0c5e7994) )
	ROM_LOAD( "gr1",          0x18000, 0x8000, CRC(27ede6ce) SHA1(14aa31cbcf089419b5b2ea8d57e82fc51895fc2e) )
	ROM_LOAD( "gr2",          0x20000, 0x8000, CRC(e49238f4) SHA1(ac76f1a761d6b0765437fb7367442667da7bb373) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "sound",        0x8000, 0x8000, CRC(8c9c3b8a) SHA1(f3cdf42ef19012817e6b7966845f9ede39f61b07) )
ROM_END


ROM_START( capbowl2 )
	ROM_REGION( 0x28000, REGION_CPU1, 0 )
	ROM_LOAD( "progrev3.u6",  0x08000, 0x8000, CRC(9162934a) SHA1(7542dd68a2aa55ad4f03b23ae2313ed6a34ae145) )
	ROM_LOAD( "gr0",          0x10000, 0x8000, CRC(ef53ca7a) SHA1(219dc342595bfd23c1336f3e167e40ff0c5e7994) )
	ROM_LOAD( "gr1",          0x18000, 0x8000, CRC(27ede6ce) SHA1(14aa31cbcf089419b5b2ea8d57e82fc51895fc2e) )
	ROM_LOAD( "gr2",          0x20000, 0x8000, CRC(e49238f4) SHA1(ac76f1a761d6b0765437fb7367442667da7bb373) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "sound",        0x8000, 0x8000, CRC(8c9c3b8a) SHA1(f3cdf42ef19012817e6b7966845f9ede39f61b07) )
ROM_END


ROM_START( clbowl )
	ROM_REGION( 0x28000, REGION_CPU1, 0 )
	ROM_LOAD( "u6.cl",        0x08000, 0x8000, CRC(91e06bc4) SHA1(efa54328417f971cc482a4529d05331a3baffc1a) )
	ROM_LOAD( "gr0.cl",       0x10000, 0x8000, CRC(899c8f15) SHA1(dbb4a9c015b5e64c62140f0c99b87da2793ae5c1) )
	ROM_LOAD( "gr1.cl",       0x18000, 0x8000, CRC(0ac0dc4c) SHA1(61afa3af1f84818b940b5c6f6a8cfb58ca557551) )
	ROM_LOAD( "gr2.cl",       0x20000, 0x8000, CRC(251f5da5) SHA1(063001cfb68e3ec35baa24eed186214e26d55b82) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "sound.cl",     0x8000, 0x8000, CRC(1eba501e) SHA1(684bdc18cf5e01a86d8018a3e228ec34e5dec57d) )
ROM_END


ROM_START( bowlrama )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "u6",           0x08000, 0x08000, CRC(7103ad55) SHA1(92dccc5e6df3e18fc8cdcb67ef14d50ce5eb8b2c) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "u30",          0x8000, 0x8000, CRC(f3168834) SHA1(40b7fbe9c15cc4442f4394b71c0666185afe4c8d) )

	ROM_REGION( 0x40000, REGION_GFX1, 0 )
	ROM_LOAD( "ux7",          0x00000, 0x40000, CRC(8727432a) SHA1(a81d366c5f8df0bdb97e795bba7752e6526ddba0) )
ROM_END



/*************************************
 *
 *	Game drivers
 *
 *************************************/

GAME( 1988, capbowl,  0,       capbowl,  capbowl, 0, ROT270, "Incredible Technologies", "Capcom Bowling (set 1)" )
GAME( 1988, capbowl2, capbowl, capbowl,  capbowl, 0, ROT270, "Incredible Technologies", "Capcom Bowling (set 2)" )
GAME( 1989, clbowl,   capbowl, capbowl,  capbowl, 0, ROT270, "Incredible Technologies", "Coors Light Bowling" )
GAME( 1991, bowlrama, 0,       bowlrama, capbowl, 0, ROT270, "PandP Marketing", "Bowl-O-Rama" )
