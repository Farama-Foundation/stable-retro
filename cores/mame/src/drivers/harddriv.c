/***************************************************************************

	Driver for Atari polygon racer games

	This collection of games uses many CPUs and many boards in many
	different combinations. There are 3 different main boards:

		- the "driver" board (A045988) is the original Hard Drivin' PCB
			- Hard Drivin'
			- Race Drivin' Upgrade

		- the "multisync" board (A046901)
			- STUN Runner
			- Steel Talons
			- Hard Drivin' Compact
			- Race Drivin' Compact
			- Street Drivin'

		- the "multisync II" board (A049852)
			- Hard Drivin's Airborne

	To first order, all of the above boards had the same basic features:

		a 68010 @ 8MHz to drive the whole game
		a TMS34010 @ 48MHz (GSP) to render the polygons and graphics
		a TMS34012 @ 50MHz (PSP, labelled SCX6218UTP) to expand pixels
		a TMS34010 @ 50MHz (MSP, optional) to handle in-game calculations

	The original "driver" board had 1MB of VRAM. The "multisync" board
	reduced that to 512k. The "multisync II" board went back to a full
	MB again.

	Stacked on top of the main board were two or more additional boards
	that were accessible through an expansion bus. Each game had at least
	an ADSP board and a sound board. Later games had additional boards for
	extra horsepower or for communications between multiple players.

	-----------------------------------------------------------------------

	The ADSP board is usually the board stacked closest to the main board.
	It also comes in four varieties, though these do not match
	one-for-one with the main boards listed above. They are:

		- the "ADSP" board (A044420)
			- early Hard Drivin' revisions

		- the "ADSP II" board (A047046)
			- later Hard Drivin'
			- STUN Runner
			- Hard Drivin' Compact
			- Race Drivin' Upgrade
			- Race Drivin' Compact

		- the "DS III" board (A049096)
			- Steel Talons
			- Street Drivin'

		- the "DS IV" board (A051973)
			- Hard Drivin's Airborne

	These boards are the workhorses of the game. They contain a single
	8MHz ADSP-2100 (ADSP and ADSP II) or 12MHz ADSP-2101 (DS III and DS IV)
	chip that is responsible for all the polygon transformations, lighting,
	and slope computations. Along with the DSP, there are several high-speed
	serial-access ROMs and RAMs.

	The "ADSP II" board is nearly identical to the original "ADSP" board
	except that is has space for extra serial ROM data. The "DS III" is
	an advanced design that contains space for a bunch of complex sound
	circuitry that was not used on Steel Talons, but was used for the
	prototype Street Drivin'. The "DS IV" looks to have the same board 
	layout as the "DS III", but the sound circuitry is actually populated.

	-----------------------------------------------------------------------

	Three sound boards were used:

		- the "driver sound" board (A046491)
			- Hard Drivin'
			- Hard Drivin' Compact
			- Race Drivin' Upgrade
			- Race Drivin' Compact

		- the "JSA II" board
			- STUN Runner

		- the "JSA IIIS" board
			- Steel Talons

	The "driver sound" board runs with a 68000 master and a TMS32010 slave
	driving a DAC. The "JSA" boards are both standard Atari sound boards
	with a 6502 driving a YM2151 and an OKI6295 ADPCM chip. Hard Drivin's
	Airborne uses the "DS IV" board for its sound.

	-----------------------------------------------------------------------

	In addition, there were a number of supplemental boards that were
	included with certain games:

		- the "DSK" board (A047724)
			- Race Drivin' Upgrade
			- Race Drivin' Compact
			- Street Drivin'

		- the "DSPCOM" board (A049349)
			- Steel Talons

		- the "DSK II" board (A051028)
			- Hard Drivin' Airborne

	-----------------------------------------------------------------------

	There are a total of 8 known games (plus variants) on this hardware:

	Hard Drivin' Cockpit
		- "driver" board (8MHz 68010, 2x50MHz TMS34010, 50MHz TMS34012)
		- "ADSP" or "ADSP II" board (8MHz ADSP-2100)
		- "driver sound" board (8MHz 68000, 20MHz TMS32010)

	Hard Drivin' Compact
		- "multisync" board (8MHz 68010, 2x50MHz TMS34010, 50MHz TMS34012)
		- "ADSP II" board (8MHz ADSP-2100)
		- "driver sound" board (8MHz 68000, 20MHz TMS32010)

	S.T.U.N. Runner
		- "multisync" board (8MHz 68010, 2x50MHz TMS34010, 50MHz TMS34012)
		- "ADSP II" board (8MHz ADSP-2100)
		- "JSA II" sound board (1.7MHz 6502, YM2151, OKI6295)

	Race Drivin' Cockpit
		- "driver" board (8MHz 68010, 50MHz TMS34010, 50MHz TMS34012)
		- "ADSP" or "ADSP II" board (8MHz ADSP-2100)
		- "DSK" board (40MHz DSP32C, 20MHz TMS32015)
		- "driver sound" board (8MHz 68000, 20MHz TMS32010)

	Race Drivin' Compact
		- "multisync" board (8MHz 68010, 50MHz TMS34010, 50MHz TMS34012)
		- "ADSP II" board (8MHz ADSP-2100)
		- "DSK" board (40MHz DSP32C, 20MHz TMS32015)
		- "driver sound" board (8MHz 68000, 20MHz TMS32010)

	Steel Talons
		- "multisync" board (8MHz 68010, 2x50MHz TMS34010, 50MHz TMS34012)
		- "DS III" board (12MHz ADSP-2101)
		- "JSA IIIS" sound board (1.7MHz 6502, YM2151, OKI6295)
		- "DSPCOM" I/O board (10MHz ADSP-2105)

	Street Drivin'
		- "multisync" board (8MHz 68010, 50MHz TMS34010, 50MHz TMS34012)
		- "DS III" board (12MHz ADSP-2101, plus 10MHz ADSP-2105 for sound)
		- "DSK" board (40MHz DSP32C, 20MHz TMS32015)

	Hard Drivin's Airborne (prototype)
		- "multisync ii" main board (8MHz 68010, 50MHz TMS34010, 50MHz TMS34012)
		- "DS IV" board (12MHz ADSP-2101, plus 2x10MHz ADSP-2105s for sound)
		- "DSK II" board (40MHz DSP32C, 20MHz TMS32015)

	BMX Heat (prototype)
		- unknown boards ???

	Police Trainer (prototype)
		- unknown boards ???

	Metal Maniax (prototype)
		- reworked hardware that is similar but not of the same layout

****************************************************************************/


#include "driver.h"
#include "sound/adpcm.h"
#include "cpu/tms34010/tms34010.h"
#include "cpu/tms32010/tms32010.h"
#include "cpu/adsp2100/adsp2100.h"
#include "cpu/dsp32/dsp32.h"
#include "machine/atarigen.h"
#include "machine/asic65.h"
#include "sndhrdw/atarijsa.h"
#include "harddriv.h"

/* from slapstic.c */
void slapstic_init(int chip);



/*************************************
 *
 *	CPU configs
 *
 *************************************/

static struct tms34010_config gsp_config =
{
	1,								/* halt on reset */
	hdgsp_irq_gen,					/* generate interrupt */
	hdgsp_write_to_shiftreg,		/* write to shiftreg function */
	hdgsp_read_from_shiftreg,		/* read from shiftreg function */
	hdgsp_display_update			/* display offset update function */
};


static struct tms34010_config msp_config =
{
	1,								/* halt on reset */
	hdmsp_irq_gen,					/* generate interrupt */
	NULL,							/* write to shiftreg function */
	NULL,							/* read from shiftreg function */
	NULL							/* display offset update function */
};


static struct dsp32_config dsp32c_config =
{
	hddsk_update_pif				/* a change has occurred on an output pin */
};



/*************************************
 *
 *	Driver board memory maps
 *
 *************************************/

static MEMORY_READ16_START( driver_readmem_68k )
	{ 0x000000, 0x0fffff, MRA16_ROM },
	{ 0x600000, 0x603fff, hd68k_port0_r },
	{ 0xa80000, 0xafffff, input_port_1_word_r },
	{ 0xb00000, 0xb7ffff, hd68k_adc8_r },
	{ 0xb80000, 0xbfffff, hd68k_adc12_r },
	{ 0xc00000, 0xc03fff, hd68k_gsp_io_r },
	{ 0xc04000, 0xc07fff, hd68k_msp_io_r },
	{ 0xff0000, 0xff001f, hd68k_duart_r },
	{ 0xff4000, 0xff4fff, hd68k_zram_r },
	{ 0xff8000, 0xffffff, MRA16_RAM },
MEMORY_END


static MEMORY_WRITE16_START( driver_writemem_68k )
	{ 0x000000, 0x0fffff, MWA16_ROM },
	{ 0x604000, 0x607fff, hd68k_nwr_w },
	{ 0x608000, 0x60bfff, watchdog_reset16_w },
	{ 0x60c000, 0x60ffff, hd68k_irq_ack_w },
	{ 0xa00000, 0xa7ffff, hd68k_wr0_write },
	{ 0xa80000, 0xafffff, hd68k_wr1_write },
	{ 0xb00000, 0xb7ffff, hd68k_wr2_write },
	{ 0xb80000, 0xbfffff, hd68k_adc_control_w },
	{ 0xc00000, 0xc03fff, hd68k_gsp_io_w },
	{ 0xc04000, 0xc07fff, hd68k_msp_io_w },
	{ 0xff0000, 0xff001f, hd68k_duart_w },
	{ 0xff4000, 0xff4fff, hd68k_zram_w, &atarigen_eeprom, &atarigen_eeprom_size },
	{ 0xff8000, 0xffffff, MWA16_RAM },
MEMORY_END


static MEMORY_READ16_START( driver_readmem_gsp )
	{ TOBYTE(0x00000000), TOBYTE(0x0000200f), MRA16_NOP },	/* used during self-test */
	{ TOBYTE(0x02000000), TOBYTE(0x0207ffff), hdgsp_vram_2bpp_r },
	{ TOBYTE(0xc0000000), TOBYTE(0xc00001ff), tms34010_io_register_r },
	{ TOBYTE(0xf4000000), TOBYTE(0xf40000ff), hdgsp_control_lo_r },
	{ TOBYTE(0xf4800000), TOBYTE(0xf48000ff), hdgsp_control_hi_r },
	{ TOBYTE(0xf5000000), TOBYTE(0xf5000fff), hdgsp_paletteram_lo_r },
	{ TOBYTE(0xf5800000), TOBYTE(0xf5800fff), hdgsp_paletteram_hi_r },
	{ TOBYTE(0xff800000), TOBYTE(0xffffffff), MRA16_BANK1 },
MEMORY_END


static MEMORY_WRITE16_START( driver_writemem_gsp )
	{ TOBYTE(0x00000000), TOBYTE(0x0000200f), MWA16_NOP },	/* used during self-test */
	{ TOBYTE(0x02000000), TOBYTE(0x0207ffff), hdgsp_vram_1bpp_w },
	{ TOBYTE(0xc0000000), TOBYTE(0xc00001ff), hdgsp_io_w },
	{ TOBYTE(0xf4000000), TOBYTE(0xf40000ff), hdgsp_control_lo_w, &hdgsp_control_lo },
	{ TOBYTE(0xf4800000), TOBYTE(0xf48000ff), hdgsp_control_hi_w, &hdgsp_control_hi },
	{ TOBYTE(0xf5000000), TOBYTE(0xf5007fff), hdgsp_paletteram_lo_w, &hdgsp_paletteram_lo },
	{ TOBYTE(0xf5800000), TOBYTE(0xf5807fff), hdgsp_paletteram_hi_w, &hdgsp_paletteram_hi },
	{ TOBYTE(0xff800000), TOBYTE(0xffffffff), MWA16_BANK1, (data16_t **)&hdgsp_vram, &hdgsp_vram_size },
MEMORY_END


static MEMORY_READ16_START( driver_readmem_msp )
	{ TOBYTE(0x00000000), TOBYTE(0x000fffff), MRA16_BANK2 },
	{ TOBYTE(0x00700000), TOBYTE(0x007fffff), MRA16_BANK3 },
	{ TOBYTE(0xc0000000), TOBYTE(0xc00001ff), tms34010_io_register_r },
	{ TOBYTE(0xfff00000), TOBYTE(0xffffffff), MRA16_RAM },
MEMORY_END


static MEMORY_WRITE16_START( driver_writemem_msp )
	{ TOBYTE(0x00000000), TOBYTE(0x000fffff), MWA16_BANK2 },
	{ TOBYTE(0x00700000), TOBYTE(0x007fffff), MWA16_BANK3 },
	{ TOBYTE(0xc0000000), TOBYTE(0xc00001ff), tms34010_io_register_w },
	{ TOBYTE(0xfff00000), TOBYTE(0xffffffff), MWA16_RAM, &hdmsp_ram },
MEMORY_END



/*************************************
 *
 *	Multisync board memory maps
 *
 *************************************/

static MEMORY_READ16_START( multisync_readmem_68k )
	{ 0x000000, 0x0fffff, MRA16_ROM },
	{ 0x600000, 0x603fff, atarigen_sound_upper_r },
	{ 0x604000, 0x607fff, hd68k_sound_reset_r },
	{ 0x60c000, 0x60ffff, hd68k_port0_r },
	{ 0xa80000, 0xafffff, input_port_1_word_r },
	{ 0xb00000, 0xb7ffff, hd68k_adc8_r },
	{ 0xb80000, 0xbfffff, hd68k_adc12_r },
	{ 0xc00000, 0xc03fff, hd68k_gsp_io_r },
	{ 0xc04000, 0xc07fff, hd68k_msp_io_r },
	{ 0xff0000, 0xff001f, hd68k_duart_r },
	{ 0xff4000, 0xff4fff, hd68k_zram_r },
	{ 0xff8000, 0xffffff, MRA16_RAM },
MEMORY_END


static MEMORY_WRITE16_START( multisync_writemem_68k )
	{ 0x000000, 0x0fffff, MWA16_ROM },
	{ 0x600000, 0x603fff, atarigen_sound_upper_w },
	{ 0x604000, 0x607fff, hd68k_nwr_w },
	{ 0x608000, 0x60bfff, watchdog_reset16_w },
	{ 0x60c000, 0x60ffff, hd68k_irq_ack_w },
	{ 0xa00000, 0xa7ffff, hd68k_wr0_write },
	{ 0xa80000, 0xafffff, hd68k_wr1_write },
	{ 0xb00000, 0xb7ffff, hd68k_wr2_write },
	{ 0xb80000, 0xbfffff, hd68k_adc_control_w },
	{ 0xc00000, 0xc03fff, hd68k_gsp_io_w },
	{ 0xc04000, 0xc07fff, hd68k_msp_io_w },
	{ 0xff0000, 0xff001f, hd68k_duart_w },
	{ 0xff4000, 0xff4fff, hd68k_zram_w, &atarigen_eeprom, &atarigen_eeprom_size },
	{ 0xff8000, 0xffffff, MWA16_RAM },
MEMORY_END


static MEMORY_READ16_START( multisync_readmem_gsp )
	{ TOBYTE(0x00000000), TOBYTE(0x0000200f), MRA16_NOP },	/* used during self-test */
	{ TOBYTE(0x02000000), TOBYTE(0x020fffff), hdgsp_vram_2bpp_r },
	{ TOBYTE(0xc0000000), TOBYTE(0xc00001ff), tms34010_io_register_r },
	{ TOBYTE(0xf4000000), TOBYTE(0xf40000ff), hdgsp_control_lo_r },
	{ TOBYTE(0xf4800000), TOBYTE(0xf48000ff), hdgsp_control_hi_r },
	{ TOBYTE(0xf5000000), TOBYTE(0xf5000fff), hdgsp_paletteram_lo_r },
	{ TOBYTE(0xf5800000), TOBYTE(0xf5800fff), hdgsp_paletteram_hi_r },
	{ TOBYTE(0xff800000), TOBYTE(0xffbfffff), MRA16_BANK1 },
	{ TOBYTE(0xffc00000), TOBYTE(0xffffffff), MRA16_RAM },
MEMORY_END


static MEMORY_WRITE16_START( multisync_writemem_gsp )
	{ TOBYTE(0x00000000), TOBYTE(0x00afffff), MWA16_NOP },	/* hit during self-test */
	{ TOBYTE(0x02000000), TOBYTE(0x020fffff), hdgsp_vram_2bpp_w },
	{ TOBYTE(0xc0000000), TOBYTE(0xc00001ff), hdgsp_io_w },
	{ TOBYTE(0xf4000000), TOBYTE(0xf40000ff), hdgsp_control_lo_w, &hdgsp_control_lo },
	{ TOBYTE(0xf4800000), TOBYTE(0xf48000ff), hdgsp_control_hi_w, &hdgsp_control_hi },
	{ TOBYTE(0xf5000000), TOBYTE(0xf5007fff), hdgsp_paletteram_lo_w, &hdgsp_paletteram_lo },
	{ TOBYTE(0xf5800000), TOBYTE(0xf5807fff), hdgsp_paletteram_hi_w, &hdgsp_paletteram_hi },
	{ TOBYTE(0xff800000), TOBYTE(0xffbfffff), MWA16_BANK1 },
	{ TOBYTE(0xffc00000), TOBYTE(0xffffffff), MWA16_RAM, (data16_t **)&hdgsp_vram, &hdgsp_vram_size },
MEMORY_END


/* MSP is identical to original driver */
#define multisync_readmem_msp driver_readmem_msp
#define multisync_writemem_msp driver_writemem_msp



/*************************************
 *
 *	Multisync II board memory maps
 *
 *************************************/

static MEMORY_READ16_START( multisync2_readmem_68k )
	{ 0x000000, 0x1fffff, MRA16_ROM },
	{ 0x60c000, 0x60ffff, hd68k_port0_r },
	{ 0xa80000, 0xafffff, input_port_1_word_r },
	{ 0xb00000, 0xb7ffff, hd68k_adc8_r },
	{ 0xb80000, 0xbfffff, hd68k_adc12_r },
	{ 0xc00000, 0xc03fff, hd68k_gsp_io_r },
	{ 0xc04000, 0xc07fff, hd68k_msp_io_r },
	{ 0xfc0000, 0xfc001f, hd68k_duart_r },
	{ 0xfd0000, 0xfd0fff, hd68k_zram_r },
	{ 0xfd4000, 0xfd4fff, hd68k_zram_r },
	{ 0xff0000, 0xffffff, MRA16_RAM },
MEMORY_END


static MEMORY_WRITE16_START( multisync2_writemem_68k )
	{ 0x000000, 0x1fffff, MWA16_ROM },
	{ 0x604000, 0x607fff, hd68k_nwr_w },
	{ 0x608000, 0x60bfff, watchdog_reset16_w },
	{ 0x60c000, 0x60ffff, hd68k_irq_ack_w },
	{ 0xa00000, 0xa7ffff, hd68k_wr0_write },
	{ 0xa80000, 0xafffff, hd68k_wr1_write },
	{ 0xb00000, 0xb7ffff, hd68k_wr2_write },
	{ 0xb80000, 0xbfffff, hd68k_adc_control_w },
	{ 0xc00000, 0xc03fff, hd68k_gsp_io_w },
	{ 0xc04000, 0xc07fff, hd68k_msp_io_w },
	{ 0xfc0000, 0xfc001f, hd68k_duart_w },
	{ 0xfd0000, 0xfd0fff, hd68k_zram_w, &atarigen_eeprom, &atarigen_eeprom_size },
	{ 0xff0000, 0xffffff, MWA16_RAM },
MEMORY_END


/* GSP is identical to original multisync */
static MEMORY_READ16_START( multisync2_readmem_gsp )
	{ TOBYTE(0x00000000), TOBYTE(0x0000200f), MRA16_NOP },	/* used during self-test */
	{ TOBYTE(0x02000000), TOBYTE(0x020fffff), hdgsp_vram_2bpp_r },
	{ TOBYTE(0xc0000000), TOBYTE(0xc00001ff), tms34010_io_register_r },
	{ TOBYTE(0xf4000000), TOBYTE(0xf40000ff), hdgsp_control_lo_r },
	{ TOBYTE(0xf4800000), TOBYTE(0xf48000ff), hdgsp_control_hi_r },
	{ TOBYTE(0xf5000000), TOBYTE(0xf5000fff), hdgsp_paletteram_lo_r },
	{ TOBYTE(0xf5800000), TOBYTE(0xf5800fff), hdgsp_paletteram_hi_r },
	{ TOBYTE(0xff800000), TOBYTE(0xffffffff), MRA16_BANK1 },
MEMORY_END


static MEMORY_WRITE16_START( multisync2_writemem_gsp )
	{ TOBYTE(0x00000000), TOBYTE(0x00afffff), MWA16_NOP },	/* hit during self-test */
	{ TOBYTE(0x02000000), TOBYTE(0x020fffff), hdgsp_vram_2bpp_w },
	{ TOBYTE(0xc0000000), TOBYTE(0xc00001ff), hdgsp_io_w },
	{ TOBYTE(0xf4000000), TOBYTE(0xf40000ff), hdgsp_control_lo_w, &hdgsp_control_lo },
	{ TOBYTE(0xf4800000), TOBYTE(0xf48000ff), hdgsp_control_hi_w, &hdgsp_control_hi },
	{ TOBYTE(0xf5000000), TOBYTE(0xf5007fff), hdgsp_paletteram_lo_w, &hdgsp_paletteram_lo },
	{ TOBYTE(0xf5800000), TOBYTE(0xf5807fff), hdgsp_paletteram_hi_w, &hdgsp_paletteram_hi },
	{ TOBYTE(0xff800000), TOBYTE(0xffffffff), MWA16_BANK1, (data16_t **)&hdgsp_vram, &hdgsp_vram_size },
MEMORY_END



/*************************************
 *
 *	ADSP/ADSP II board memory maps
 *
 *************************************/

static MEMORY_READ16_START( adsp_readmem )
	{ ADSP_DATA_ADDR_RANGE(0x0000, 0x1fff), MRA16_RAM },
	{ ADSP_DATA_ADDR_RANGE(0x2000, 0x2fff), hdadsp_special_r },
	{ ADSP_PGM_ADDR_RANGE (0x0000, 0x1fff), MRA16_RAM },
MEMORY_END


static MEMORY_WRITE16_START( adsp_writemem )
	{ ADSP_DATA_ADDR_RANGE(0x0000, 0x1fff), MWA16_RAM },
	{ ADSP_DATA_ADDR_RANGE(0x2000, 0x2fff), hdadsp_special_w },
	{ ADSP_PGM_ADDR_RANGE (0x0000, 0x1fff), MWA16_RAM },
MEMORY_END



/*************************************
 *
 *	DS III/IV board memory maps
 *
 *************************************/

static MEMORY_READ16_START( ds3_readmem )
	{ ADSP_DATA_ADDR_RANGE(0x0000, 0x1fff), MRA16_RAM },
	{ ADSP_DATA_ADDR_RANGE(0x3800, 0x3bff), MRA16_RAM },		/* internal RAM */
	{ ADSP_DATA_ADDR_RANGE(0x3fe0, 0x3fff), hdds3_control_r },	/* adsp control regs */
	{ ADSP_DATA_ADDR_RANGE(0x2000, 0x3fff), hdds3_special_r },
	{ ADSP_PGM_ADDR_RANGE (0x0000, 0x3fff), MRA16_RAM },
MEMORY_END


static MEMORY_WRITE16_START( ds3_writemem )
	{ ADSP_DATA_ADDR_RANGE(0x0000, 0x1fff), MWA16_RAM },
	{ ADSP_DATA_ADDR_RANGE(0x3800, 0x3bff), MWA16_RAM },		/* internal RAM */
	{ ADSP_DATA_ADDR_RANGE(0x3fe0, 0x3fff), hdds3_control_w },	/* adsp control regs */
	{ ADSP_DATA_ADDR_RANGE(0x2000, 0x3fff), hdds3_special_w },
	{ ADSP_PGM_ADDR_RANGE (0x0000, 0x3fff), MWA16_RAM },
MEMORY_END


static MEMORY_READ16_START( ds3snd_readmem )
	{ ADSP_DATA_ADDR_RANGE(0x0000, 0x1fff), MRA16_RAM },
	{ ADSP_DATA_ADDR_RANGE(0x3800, 0x3bff), MRA16_RAM },		/* internal RAM */
	{ ADSP_DATA_ADDR_RANGE(0x3fe0, 0x3fff), hdds3_control_r },	/* adsp control regs */
	{ ADSP_PGM_ADDR_RANGE (0x0000, 0x3fff), MRA16_RAM },
/**/
/*	/SIRQ2 = IRQ2*/
/*	/SRES -> RESET*/
/**/
/*	2xx0 W = SWR0 (POUT)*/
/*	2xx1 W = SWR1 (SINT)*/
/*	2xx2 W = SWR2 (TFLAG)*/
/*	2xx3 W = SWR3 (INTSRC)*/
/*	2xx4 W = DACL*/
/*	2xx5 W = DACR*/
/*	2xx6 W = SRMADL*/
/*	2xx7 W = SRMADH*/
/**/
/*	2xx0 R = SRD0 (PIN)*/
/*	2xx1 R = SRD1 (RSAT)*/
/*	2xx4 R = SROM*/
/*	2xx7 R = SFWCLR*/
/**/
/**/
/*	/XRES -> RESET*/
/*	communicate over serial I/O*/

MEMORY_END


static MEMORY_WRITE16_START( ds3snd_writemem )
	{ ADSP_DATA_ADDR_RANGE(0x0000, 0x1fff), MWA16_RAM },
	{ ADSP_DATA_ADDR_RANGE(0x3800, 0x3bff), MWA16_RAM },		/* internal RAM */
	{ ADSP_DATA_ADDR_RANGE(0x3fe0, 0x3fff), hdds3_control_w },	/* adsp control regs */
	{ ADSP_PGM_ADDR_RANGE (0x0000, 0x3fff), MWA16_RAM },
MEMORY_END



/*************************************
 *
 *	DSK board memory maps
 *
 *************************************/

static MEMORY_READ32_START( dsk_readmem_dsp32 )
	{ 0x000000, 0x001fff, MRA32_RAM },
	{ 0x600000, 0x63ffff, MRA32_RAM },
	{ 0xfff800, 0xffffff, MRA32_RAM },
MEMORY_END


static MEMORY_WRITE32_START( dsk_writemem_dsp32 )
	{ 0x000000, 0x001fff, MWA32_RAM },
	{ 0x600000, 0x63ffff, MWA32_RAM },
	{ 0xfff800, 0xffffff, MWA32_RAM },
MEMORY_END



/*************************************
 *
 *	DSK II board memory maps
 *
 *************************************/

static MEMORY_READ32_START( dsk2_readmem_dsp32 )
	{ 0x000000, 0x001fff, MRA32_RAM },
	{ 0x200000, 0x23ffff, MRA32_RAM },
	{ 0x400000, 0x5fffff, MRA32_BANK4 },
	{ 0xfff800, 0xffffff, MRA32_RAM },
MEMORY_END


static MEMORY_WRITE32_START( dsk2_writemem_dsp32 )
	{ 0x000000, 0x001fff, MWA32_RAM },
	{ 0x200000, 0x23ffff, MWA32_RAM },
	{ 0x400000, 0x5fffff, MWA32_ROM },
	{ 0xfff800, 0xffffff, MWA32_RAM },
MEMORY_END



/*************************************
 *
 *	Driver sound board memory maps
 *
 *************************************/

static MEMORY_READ16_START( driversnd_readmem_68k )
	{ 0x000000, 0x01ffff, MRA16_ROM },
	{ 0xff0000, 0xff0fff, hdsnd68k_data_r },
	{ 0xff1000, 0xff1fff, hdsnd68k_switches_r },
	{ 0xff2000, 0xff2fff, hdsnd68k_320port_r },
	{ 0xff3000, 0xff3fff, hdsnd68k_status_r },
	{ 0xff4000, 0xff5fff, hdsnd68k_320ram_r },
	{ 0xff6000, 0xff7fff, hdsnd68k_320ports_r },
	{ 0xff8000, 0xffbfff, hdsnd68k_320com_r },
	{ 0xffc000, 0xffffff, MRA16_RAM },
MEMORY_END


static MEMORY_WRITE16_START( driversnd_writemem_68k )
	{ 0x000000, 0x01ffff, MWA16_ROM },
	{ 0xff0000, 0xff0fff, hdsnd68k_data_w },
	{ 0xff1000, 0xff1fff, hdsnd68k_latches_w },
	{ 0xff2000, 0xff2fff, hdsnd68k_speech_w },
	{ 0xff3000, 0xff3fff, hdsnd68k_irqclr_w },
	{ 0xff4000, 0xff5fff, hdsnd68k_320ram_w },
	{ 0xff6000, 0xff7fff, hdsnd68k_320ports_w },
	{ 0xff8000, 0xffbfff, hdsnd68k_320com_w },
	{ 0xffc000, 0xffffff, MWA16_RAM },
MEMORY_END


static MEMORY_READ16_START( driversnd_readmem_dsp )
	{ TMS32010_DATA_ADDR_RANGE(0x000, 0x0ff), MRA16_RAM },
	{ TMS32010_PGM_ADDR_RANGE(0x000, 0xfff), MRA16_RAM },
MEMORY_END


static MEMORY_WRITE16_START( driversnd_writemem_dsp )
	{ TMS32010_DATA_ADDR_RANGE(0x000, 0x0ff), MWA16_RAM },
	{ TMS32010_PGM_ADDR_RANGE(0x000, 0xfff), MWA16_RAM, &hdsnddsp_ram },
MEMORY_END


static PORT_READ16_START( driversnd_readport_dsp )
	{ TMS32010_PORT_RANGE(0, 0), hdsnddsp_rom_r },
	{ TMS32010_PORT_RANGE(1, 1), hdsnddsp_comram_r },
	{ TMS32010_PORT_RANGE(2, 2), hdsnddsp_compare_r },
	{ TMS32010_PORT_RANGE(TMS32010_BIO, TMS32010_BIO), hdsnddsp_get_bio },
PORT_END


static PORT_WRITE16_START( driversnd_writeport_dsp )
	{ TMS32010_PORT_RANGE(0, 0), hdsnddsp_dac_w },
	{ TMS32010_PORT_RANGE(1, 2), MWA16_NOP },
	{ TMS32010_PORT_RANGE(3, 3), hdsnddsp_comport_w },
	{ TMS32010_PORT_RANGE(4, 4), hdsnddsp_mute_w },
	{ TMS32010_PORT_RANGE(5, 5), hdsnddsp_gen68kirq_w },
	{ TMS32010_PORT_RANGE(6, 7), hdsnddsp_soundaddr_w },
PORT_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( harddriv )
	PORT_START		/* 600000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNUSED )	/* diagnostic switch */
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_SPECIAL )	/* HBLANK */
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* 12-bit EOC */
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* 8-bit EOC */
	PORT_SERVICE( 0x0020, IP_ACTIVE_LOW )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNUSED )	/* option switches */

	PORT_START		/* a80000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START2 )	/* abort */
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_START1 )	/* key */
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_COIN3 )	/* aux coin */
	PORT_BIT( 0xfff8, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b00000 - 8 bit ADC 0 - gas pedal */
	PORT_ANALOG( 0xff, 0x00, IPT_PEDAL, 100, 20, 0x00, 0xff )

	PORT_START		/* b00000 - 8 bit ADC 1 - clutch pedal */
	PORT_ANALOG( 0xff, 0x00, IPT_PEDAL | IPF_PLAYER3, 100, 25, 0x00, 0xff )

	PORT_START		/* b00000 - 8 bit ADC 2 - seat */
	PORT_BIT( 0xff, 0x80, IPT_SPECIAL )

	PORT_START		/* b00000 - 8 bit ADC 3 - shifter lever Y */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_Y | IPF_PLAYER2, 25, 128, 0x00, 0xff )

	PORT_START		/* b00000 - 8 bit ADC 4 - shifter lever X*/
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_X | IPF_PLAYER2, 25, 128, 0x00, 0xff )

	PORT_START		/* b00000 - 8 bit ADC 5 - wheel */
	PORT_ANALOG( 0xff, 0x80, IPT_PADDLE, 25, 5, 0x10, 0xf0 )

	PORT_START		/* b00000 - 8 bit ADC 6 - line volts */
	PORT_BIT( 0xff, 0x80, IPT_SPECIAL )

	PORT_START		/* b00000 - 8 bit ADC 7 - shift force */
	PORT_BIT( 0xff, 0x80, IPT_SPECIAL )

	PORT_START		/* b80000 - 12 bit ADC 0 - steering wheel */
	PORT_ANALOG( 0xff, 0x80, IPT_PADDLE, 25, 5, 0x10, 0xf0 )

	PORT_START		/* b80000 - 12 bit ADC 1 - force brake */
	PORT_ANALOG( 0xff, 0x00, IPT_PEDAL2 | IPF_REVERSE, 100, 40, 0x00, 0xff )

	PORT_START		/* b80000 - 12 bit ADC 2 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b80000 - 12 bit ADC 3 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( racedriv )
	PORT_START		/* 600000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNUSED )	/* diagnostic switch */
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_SPECIAL )	/* HBLANK */
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* 12-bit EOC */
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* 8-bit EOC */
	PORT_SERVICE( 0x0020, IP_ACTIVE_LOW )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNUSED )	/* option switches */

	PORT_START		/* a80000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START2 )	/* abort */
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_START1 )	/* key */
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_COIN3 )	/* aux coin */
	PORT_BIT( 0xfff8, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b00000 - 8 bit ADC 0 - gas pedal */
	PORT_ANALOG( 0xff, 0x00, IPT_PEDAL, 100, 20, 0x00, 0xff )

	PORT_START		/* b00000 - 8 bit ADC 1 - clutch pedal */
	PORT_ANALOG( 0xff, 0x00, IPT_PEDAL | IPF_PLAYER3, 100, 25, 0x00, 0xff )

	PORT_START		/* b00000 - 8 bit ADC 2 - seat */
	PORT_BIT( 0xff, 0x80, IPT_SPECIAL )

	PORT_START		/* b00000 - 8 bit ADC 3 - shifter lever Y */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_Y | IPF_PLAYER2, 25, 128, 0x00, 0xff )

	PORT_START		/* b00000 - 8 bit ADC 4 - shifter lever X*/
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_X | IPF_PLAYER2, 25, 128, 0x00, 0xff )

	PORT_START		/* b00000 - 8 bit ADC 5 - wheel */
	PORT_ANALOG( 0xff, 0x80, IPT_PADDLE, 25, 5, 0x10, 0xf0 )

	PORT_START		/* b00000 - 8 bit ADC 6 - line volts */
	PORT_BIT( 0xff, 0x80, IPT_SPECIAL )

	PORT_START		/* b00000 - 8 bit ADC 7 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b80000 - 12 bit ADC 0 - steering wheel */
	PORT_ANALOG( 0xff, 0x80, IPT_PADDLE, 25, 5, 0x10, 0xf0 )

	PORT_START		/* b80000 - 12 bit ADC 1 - force brake */
	PORT_ANALOG( 0xff, 0x00, IPT_PEDAL2 | IPF_REVERSE, 100, 40, 0x00, 0xff )

	PORT_START		/* b80000 - 12 bit ADC 2 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b80000 - 12 bit ADC 3 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( racedrvc )
	PORT_START		/* 60c000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNUSED )	/* diagnostic switch */
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_SPECIAL )	/* HBLANK */
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* 12-bit EOC */
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* 8-bit EOC */
	PORT_SERVICE( 0x0020, IP_ACTIVE_LOW )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNUSED )	/* option switches */

	PORT_START		/* a80000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START2 )	/* abort */
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_START1 )	/* key */
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_COIN3 )	/* aux coin */
	PORT_BIT( 0x00f8, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON2 )	/* 1st gear */
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON3 )	/* 2nd gear */
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON4 )	/* 3rd gear */
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON5 )	/* 4th gear */
	PORT_BIT( 0x3000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_SPECIAL )	/* center edge on steering wheel */
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b00000 - 8 bit ADC 0 - gas pedal */
	PORT_ANALOG( 0xff, 0x00, IPT_PEDAL, 100, 20, 0x00, 0xff )

	PORT_START		/* b00000 - 8 bit ADC 1 - clutch pedal */
	PORT_ANALOG( 0xff, 0x00, IPT_PEDAL | IPF_PLAYER3, 100, 25, 0x00, 0xff )

	PORT_START		/* b00000 - 8 bit ADC 2 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b00000 - 8 bit ADC 3 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b00000 - 8 bit ADC 4 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b00000 - 8 bit ADC 5 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b00000 - 8 bit ADC 6 - force brake */
	PORT_ANALOG( 0xff, 0x00, IPT_PEDAL2 | IPF_REVERSE, 100, 40, 0x00, 0xff )

	PORT_START		/* b00000 - 8 bit ADC 7 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* 400000 - steering wheel */
	PORT_ANALOG( 0xff, 0x80, IPT_PADDLE, 25, 5, 0x10, 0xf0 )

	/* dummy ADC ports to end up with the same number as the full version */
	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( stunrun )
	PORT_START		/* 60c000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_SPECIAL )	/* HBLANK */
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* 12-bit EOC */
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* 8-bit EOC */
	PORT_SERVICE( 0x0020, IP_ACTIVE_LOW )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNUSED )	/* Option switches */

	PORT_START		/* a80000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0xfff8, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b00000 - 8 bit ADC 0 */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_X, 25, 10, 0x00, 0xff )

	PORT_START		/* b00000 - 8 bit ADC 1 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b00000 - 8 bit ADC 2 */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_Y, 25, 10, 0x00, 0xff )

	PORT_START		/* b00000 - 8 bit ADC 3 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b00000 - 8 bit ADC 4 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b00000 - 8 bit ADC 5 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b00000 - 8 bit ADC 6 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b00000 - 8 bit ADC 7 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b80000 - 12 bit ADC 0 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b80000 - 12 bit ADC 1 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b80000 - 12 bit ADC 2 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b80000 - 12 bit ADC 3 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	JSA_II_PORT		/* audio port */
INPUT_PORTS_END


INPUT_PORTS_START( steeltal )
	PORT_START		/* 60c000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_SPECIAL )	/* HBLANK */
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* 12-bit EOC */
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* 8-bit EOC */
	PORT_SERVICE( 0x0020, IP_ACTIVE_LOW )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* a80000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 )	/* trigger */
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON2 )	/* thumb */
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON3 )	/* zoom */
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON4 )	/* real helicopter flight */
	PORT_BIT( 0xfff0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b00000 - 8 bit ADC 0 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b00000 - 8 bit ADC 1 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )		/* volume control */

	PORT_START		/* b00000 - 8 bit ADC 2 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b00000 - 8 bit ADC 3 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b00000 - 8 bit ADC 4 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b00000 - 8 bit ADC 5 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b00000 - 8 bit ADC 6 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b00000 - 8 bit ADC 7 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b80000 - 12 bit ADC 0 */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_X, 25, 10, 0x00, 0xff )	/* left/right */

	PORT_START		/* b80000 - 12 bit ADC 1 */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_Y, 25, 10, 0x00, 0xff )	/* up/down */

	PORT_START		/* b80000 - 12 bit ADC 2 */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_Y | IPF_PLAYER2 | IPF_REVERSE, 25, 10, 0x00, 0xff )	/* collective */

	PORT_START		/* b80000 - 12 bit ADC 3 */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_X | IPF_PLAYER2, 25, 10, 0x00, 0xff )	/* rudder */

	JSA_III_PORT	/* audio port */
INPUT_PORTS_END


INPUT_PORTS_START( hdrivair )
	PORT_START		/* 60c000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_SPECIAL )	/* HBLANK */
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* 12-bit EOC */
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* 8-bit EOC */
	PORT_SERVICE( 0x0020, IP_ACTIVE_LOW )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* a80000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START2 )	/* abort */
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_START1 )	/* start */
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_COIN3 )	/* aux coin */
	PORT_BIT( 0x00f8, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON5 )	/* ??? */
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_TOGGLE )	/* reverse */
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON6 )	/* ??? */
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON2 )	/* wings */
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON3 )	/* wings */
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_SPECIAL )	/* center edge on steering wheel */
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b00000 - 8 bit ADC 0 - gas pedal */
	PORT_ANALOG( 0xff, 0x00, IPT_PEDAL, 100, 20, 0x00, 0xff )

	PORT_START		/* b00000 - 8 bit ADC 1 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b00000 - 8 bit ADC 2 - voice mic */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* b00000 - 8 bit ADC 3 - volume */
	PORT_BIT( 0xff, 0X80, IPT_UNUSED )

	PORT_START		/* b00000 - 8 bit ADC 4 - elevator */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_Y | IPF_REVERSE, 25, 10, 0x00, 0xff )	/* up/down */

	PORT_START		/* b00000 - 8 bit ADC 5 - canopy */
	PORT_BIT( 0xff, 0X80, IPT_UNUSED )

	PORT_START		/* b00000 - 8 bit ADC 6 - brake */
	PORT_ANALOG( 0xff, 0x00, IPT_PEDAL2 | IPF_REVERSE, 100, 40, 0x00, 0xff )

	PORT_START		/* b00000 - 8 bit ADC 7 - seat adjust */
	PORT_BIT( 0xff, 0X80, IPT_UNUSED )

	PORT_START		/* 400000 - steering wheel */
	PORT_ANALOG( 0xff, 0x80, IPT_PADDLE | IPF_REVERSE, 25, 5, 0x10, 0xf0 )

	/* dummy ADC ports to end up with the same number as the full version */
	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END



/*************************************
 *
 *	Sound interfaces
 *
 *************************************/

static struct DACinterface dac_interface =
{
	1,
	{ MIXER(100, MIXER_PAN_CENTER) }
};


static struct DACinterface dac2_interface =
{
	2,
	{ MIXER(100, MIXER_PAN_LEFT), MIXER(100, MIXER_PAN_RIGHT) }
};



/*************************************
 *
 *	Main board pieces
 *
 *************************************/

/*
	Video timing:

				VERTICAL					HORIZONTAL
	Harddriv:	001D-019D / 01A0 (384)		001A-0099 / 009F (508)
	Harddrvc:	0011-0131 / 0133 (288)		003A-013A / 0142 (512)
	Racedriv:	001D-019D / 01A0 (384)		001A-0099 / 009F (508)
	Racedrvc:	0011-0131 / 0133 (288)		003A-013A / 0142 (512)
	Stunrun:	0013-00F8 / 0105 (229)		0037-0137 / 013C (512)
	Steeltal:	0011-0131 / 0133 (288)		003A-013A / 0142 (512)
	Hdrivair:	0011-0131 / 0133 (288)		003A-013A / 0142 (512)
*/

/* Driver board without MSP (used by Race Drivin' cockpit) */
static MACHINE_DRIVER_START( driver_nomsp )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M68010, 32000000/4)
	MDRV_CPU_MEMORY(driver_readmem_68k,driver_writemem_68k)
	MDRV_CPU_VBLANK_INT(atarigen_video_int_gen,1)
	MDRV_CPU_PERIODIC_INT(hd68k_irq_gen,244)

	MDRV_CPU_ADD_TAG("gsp", TMS34010, 48000000/TMS34010_CLOCK_DIVIDER)
	MDRV_CPU_MEMORY(driver_readmem_gsp,driver_writemem_gsp)
	MDRV_CPU_CONFIG(gsp_config)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION((1000000 * (416 - 384)) / (60 * 416))
	MDRV_INTERLEAVE(500)

	MDRV_MACHINE_INIT(harddriv)
	MDRV_NVRAM_HANDLER(atarigen)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(640, 384)
	MDRV_VISIBLE_AREA(97, 596, 0, 383)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(harddriv)
	MDRV_VIDEO_EOF(harddriv)
	MDRV_VIDEO_UPDATE(harddriv)
MACHINE_DRIVER_END


/* Driver board with MSP (used by Hard Drivin' cockpit) */
static MACHINE_DRIVER_START( driver_msp )
	MDRV_IMPORT_FROM(driver_nomsp)

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("msp", TMS34010, 50000000/TMS34010_CLOCK_DIVIDER)
	MDRV_CPU_MEMORY(driver_readmem_msp,driver_writemem_msp)
	MDRV_CPU_CONFIG(msp_config)

	/* video hardware */
	MDRV_VISIBLE_AREA(89, 596, 0, 383)
MACHINE_DRIVER_END


/* Multisync board without MSP (used by STUN Runner, Steel Talons, Race Drivin' compact) */
static MACHINE_DRIVER_START( multisync_nomsp )
	MDRV_IMPORT_FROM(driver_nomsp)

	/* basic machine hardware */
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(multisync_readmem_68k,multisync_writemem_68k)

	MDRV_CPU_MODIFY("gsp")
	MDRV_CPU_MEMORY(multisync_readmem_gsp,multisync_writemem_gsp)

	MDRV_VBLANK_DURATION((1000000 * (307 - 288)) / (60 * 307))

	/* video hardware */
	MDRV_SCREEN_SIZE(640, 288)
	MDRV_VISIBLE_AREA(109, 620, 0, 287)
MACHINE_DRIVER_END


/* Multisync board with MSP (used by Hard Drivin' compact) */
static MACHINE_DRIVER_START( multisync_msp )
	MDRV_IMPORT_FROM(multisync_nomsp)

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("msp", TMS34010, 50000000/TMS34010_CLOCK_DIVIDER)
	MDRV_CPU_MEMORY(multisync_readmem_msp,multisync_writemem_msp)
	MDRV_CPU_CONFIG(msp_config)
MACHINE_DRIVER_END


/* Multisync II board (used by Hard Drivin's Airborne) */
static MACHINE_DRIVER_START( multisync2 )
	MDRV_IMPORT_FROM(multisync_nomsp)

	/* basic machine hardware */
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(multisync2_readmem_68k,multisync2_writemem_68k)

	MDRV_CPU_MODIFY("gsp")
	MDRV_CPU_MEMORY(multisync2_readmem_gsp,multisync2_writemem_gsp)
MACHINE_DRIVER_END



/*************************************
 *
 *	ADSP board pieces
 *
 *************************************/

/* ADSP/ADSP II boards (used by Hard/Race Drivin', STUN Runner) */
static MACHINE_DRIVER_START( adsp )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("adsp", ADSP2100, 8000000)
	MDRV_CPU_MEMORY(adsp_readmem,adsp_writemem)
MACHINE_DRIVER_END


/* DS III board (used by Steel Talons) */
static MACHINE_DRIVER_START( ds3 )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("adsp", ADSP2101, 12000000)
	MDRV_CPU_MEMORY(ds3_readmem,ds3_writemem)
	
	MDRV_INTERLEAVE(1000)
MACHINE_DRIVER_END


/* DS IV board (used by Hard Drivin's Airborne) */
static MACHINE_DRIVER_START( ds4 )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("adsp", ADSP2101, 12000000)
	MDRV_CPU_MEMORY(ds3_readmem,ds3_writemem)

/*	MDRV_CPU_ADD_TAG("sound", ADSP2105, 10000000)*/
/*	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)*/
/*	MDRV_CPU_MEMORY(ds3snd_readmem,ds3snd_writemem)*/

/*	MDRV_CPU_ADD_TAG("sounddsp", ADSP2105, 10000000)*/
/*	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)*/
/*	MDRV_CPU_MEMORY(ds3snd_readmem,ds3snd_writemem)*/

	MDRV_SOUND_ADD(DAC, dac2_interface)
MACHINE_DRIVER_END



/*************************************
 *
 *	DSK board pieces
 *
 *************************************/

/* DSK board (used by Race Drivin') */
static MACHINE_DRIVER_START( dsk )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("dsp32", DSP32C, 40000000)
	MDRV_CPU_CONFIG(dsp32c_config)
	MDRV_CPU_MEMORY(dsk_readmem_dsp32,dsk_writemem_dsp32)
MACHINE_DRIVER_END


/* DSK II board (used by Hard Drivin's Airborne) */
static MACHINE_DRIVER_START( dsk2 )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("dsp32", DSP32C, 40000000)
	MDRV_CPU_CONFIG(dsp32c_config)
	MDRV_CPU_MEMORY(dsk2_readmem_dsp32,dsk2_writemem_dsp32)
MACHINE_DRIVER_END



/*************************************
 *
 *	Sound board pieces
 *
 *************************************/

static MACHINE_DRIVER_START( driversnd )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("sound", M68000, 16000000/2)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(driversnd_readmem_68k,driversnd_writemem_68k)

	MDRV_CPU_ADD_TAG("sounddsp", TMS32010, 20000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(driversnd_readmem_dsp,driversnd_writemem_dsp)
	MDRV_CPU_PORTS(driversnd_readport_dsp,driversnd_writeport_dsp)

	/* sound hardware */
	MDRV_SOUND_ADD(DAC, dac_interface)
MACHINE_DRIVER_END



/*************************************
 *
 *	Machine drivers
 *
 *************************************/

static MACHINE_DRIVER_START( harddriv )

	/* basic machine hardware */
	MDRV_IMPORT_FROM( driver_msp )		/* original driver board with MSP */
	MDRV_IMPORT_FROM( adsp )			/* ADSP board */
	MDRV_IMPORT_FROM( driversnd )		/* driver sound board */
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( harddrvc )

	/* basic machine hardware */
	MDRV_IMPORT_FROM( multisync_msp )	/* multisync board with MSP */
	MDRV_IMPORT_FROM( adsp )			/* ADSP board */
	MDRV_IMPORT_FROM( driversnd )		/* driver sound board */
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( racedriv )

	/* basic machine hardware */
	MDRV_IMPORT_FROM( driver_nomsp )	/* original driver board without MSP */
	MDRV_IMPORT_FROM( adsp )			/* ADSP board */
	MDRV_IMPORT_FROM( dsk )				/* DSK board */
	MDRV_IMPORT_FROM( driversnd )		/* driver sound board */
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( racedrvc )

	/* basic machine hardware */
	MDRV_IMPORT_FROM( multisync_nomsp )	/* multisync board without MSP */
	MDRV_IMPORT_FROM( adsp )			/* ADSP board */
	MDRV_IMPORT_FROM( dsk )				/* DSK board */
	MDRV_IMPORT_FROM( driversnd )		/* driver sound board */
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( stunrun )

	/* basic machine hardware */
	MDRV_IMPORT_FROM( multisync_nomsp )	/* multisync board without MSP */
	MDRV_IMPORT_FROM( adsp )			/* ADSP board */
	MDRV_IMPORT_FROM( jsa_ii_mono )		/* JSA II sound board */

	MDRV_VBLANK_DURATION((1000000 * (261 - 240)) / (60 * 261))

	/* video hardware */
	MDRV_SCREEN_SIZE(640, 240)
	MDRV_VISIBLE_AREA(103, 614, 0, 239)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( steeltal )

	/* basic machine hardware */
	MDRV_IMPORT_FROM( multisync_msp )	/* multisync board with MSP */
	MDRV_IMPORT_FROM( ds3 )				/* DS III board */
	MDRV_IMPORT_FROM( jsa_iii_mono )	/* JSA III sound board */
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( hdrivair )

	/* basic machine hardware */
	MDRV_IMPORT_FROM( multisync2 )		/* multisync II board */
	MDRV_IMPORT_FROM( ds4 )				/* DS IV board */
	MDRV_IMPORT_FROM( dsk2 )			/* DSK II board */
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definition(s)
 *
 *************************************/

/*
	these sound ROMs were original being used in MAME:
		ROM_LOAD16_BYTE( "hd_s.70n", 0x00000, 0x08000, CRC(0c77fab6) SHA1(4efcb64c261c7c4bfdd1f94d082404d6b4d25e54) )
		ROM_LOAD16_BYTE( "hd_s.45n", 0x00001, 0x08000, CRC(54d6dd5f) SHA1(b93e918a395f6cdea787650d4b7beffba1a77b8f) )
	they look legit, but I can't find any official record of them
*/

ROM_START( harddriv )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 1MB for 68000 code */
	ROM_LOAD16_BYTE( "7105200r.052", 0x000000, 0x010000, CRC(af5d5c3e) SHA1(09ccf4aa933413b71b6a42796d42299e1334902f) )
	ROM_LOAD16_BYTE( "7113210r.052", 0x000001, 0x010000, CRC(3330a942) SHA1(b9210f6befd875be8bc1dbd31f44d0cb63166748) )
	ROM_LOAD16_BYTE( "2106200s.052", 0x020000, 0x010000, CRC(a668db0e) SHA1(8ac405a0ba12bac9acabdb64970608d1b2b1a99b) )
	ROM_LOAD16_BYTE( "2114210s.052", 0x020001, 0x010000, CRC(ab689a94) SHA1(c6c09e088bcc32030217e3521c862acce113bf93) )
	ROM_LOAD16_BYTE( "1110200w.052", 0x0a0000, 0x010000, CRC(908ccbbe) SHA1(b6947ade664172a4553ea083fadfcb77c8c3938d) )
	ROM_LOAD16_BYTE( "1118210w.052", 0x0a0001, 0x010000, CRC(5b25023c) SHA1(e6c5bf0de5ee071b8733fc890ae4f906732adde4) )
	ROM_LOAD16_BYTE( "1111200x.052", 0x0c0000, 0x010000, CRC(e1f455a3) SHA1(68462a33bbfcc526d8f27ec082e55937a26ead8b) )
	ROM_LOAD16_BYTE( "1119210x.052", 0x0c0001, 0x010000, CRC(a7fc3aaa) SHA1(ce8d4a8f83e25008cafa2a2242ed26b90b8517da) )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU4, 0 )	/* region for ADSP 2100 */

	ROM_REGION( 0x20000, REGION_CPU5, 0 )		/* 2*64k for audio 68000 code */
	ROM_LOAD16_BYTE( "1121-45n.052", 0x000001, 0x008000, CRC(04316e6f) SHA1(9836b8d16cebd6013834432c9e5a5aca0050c889) )
	ROM_LOAD16_BYTE( "1122-70n.052", 0x000000, 0x008000, CRC(0c446eec) SHA1(53576c2800484d098cf250ab9a865314167c9d96) )

	ROM_REGION( 0x10000, REGION_CPU6, 0 )		/* region for audio 32010 */

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )	/* 384k for ADSP object ROM */
	ROM_LOAD16_BYTE( "1101-10h.052", 0x000000, 0x010000, CRC(1b77f171) SHA1(10434e492e4e9de5cd8543271914d5ba37c52b50) )
	ROM_LOAD16_BYTE( "1103-10k.052", 0x000001, 0x010000, CRC(e50bec32) SHA1(30c504c730e8e568e78e06c756a23b8923e85b4b) )
	ROM_LOAD16_BYTE( "1102-10j.052", 0x020000, 0x010000, CRC(998d3da2) SHA1(6ed560c2132e33858c91b1f4ab0247399665b5fd) )
	ROM_LOAD16_BYTE( "1104-10l.052", 0x020001, 0x010000, CRC(bc59a2b7) SHA1(7dfde5bbaa0cf349b1ef5d6b076baded7330376a) )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for ADSP I/O buffers */

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )		/* 4*128k for audio serial ROMs */
	ROM_LOAD( "1123-65a.052", 0x000000, 0x010000, CRC(a88411dc) SHA1(1fd53c7eadffa163d5423df2f8338757e58d5f2e) )
	ROM_LOAD( "1124-55a.052", 0x010000, 0x010000, CRC(071a4309) SHA1(c623bd51d6a4a56503fbf138138854d6a30b11d6) )
	ROM_LOAD( "1125-45a.052", 0x020000, 0x010000, CRC(ebf391af) SHA1(3c4097db8d625b994b39d46fe652585a74378ca0) )
	ROM_LOAD( "1126-30a.052", 0x030000, 0x010000, CRC(f46ef09c) SHA1(ba62f73ee3b33d8f26b430ffa468f8792dca23de) )
ROM_END


ROM_START( harddrvg )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 1MB for 68000 code */
	ROM_LOAD16_BYTE( "7205200r.052", 0x000000, 0x010000, CRC(b50b1968) SHA1(b6fd9f0600cba995bf31fd0f30f15f091bbbfb09) )
	ROM_LOAD16_BYTE( "7213210r.052", 0x000001, 0x010000, CRC(e6d57108) SHA1(a4eda6db4ec67b1cb8f9bfee17d8adc93675ef8a) )
	ROM_LOAD16_BYTE( "4206200s.052", 0x020000, 0x010000, CRC(70f78c50) SHA1(6baed202937c455c7c6e4baa74ee7858673a37fc) )
	ROM_LOAD16_BYTE( "4214210s.052", 0x020001, 0x010000, CRC(b7988de4) SHA1(0bc94c3eb12f8d0506eca819d79dffb7758a8421) )
	ROM_LOAD16_BYTE( "4210200w.052", 0x0a0000, 0x010000, CRC(5b8b2947) SHA1(7390e978361dfb6bf5675014a906f3f9879a8a59) )
	ROM_LOAD16_BYTE( "4218210w.052", 0x0a0001, 0x010000, CRC(9be232c5) SHA1(4fd30f53814049f8462937bfe55410edae438411) )
	ROM_LOAD16_BYTE( "4211200x.052", 0x0c0000, 0x010000, CRC(20d1b3d5) SHA1(b7c4a2cf6ba729530c24980704989b3a3efb343b) )
	ROM_LOAD16_BYTE( "4219210x.052", 0x0c0001, 0x010000, CRC(105e7052) SHA1(a8d13c35418e58410cfd74c243b74963f1553068) )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU4, 0 )	/* region for ADSP 2100 */

	ROM_REGION( 0x20000, REGION_CPU5, 0 )		/* 2*64k for audio 68000 code */
	ROM_LOAD16_BYTE( "1121-45n.052", 0x000001, 0x008000, CRC(04316e6f) SHA1(9836b8d16cebd6013834432c9e5a5aca0050c889) )
	ROM_LOAD16_BYTE( "1122-70n.052", 0x000000, 0x008000, CRC(0c446eec) SHA1(53576c2800484d098cf250ab9a865314167c9d96) )

	ROM_REGION( 0x10000, REGION_CPU6, 0 )		/* region for audio 32010 */

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )	/* 384k for ADSP object ROM */
	ROM_LOAD16_BYTE( "1101-10h.052", 0x000000, 0x010000, CRC(1b77f171) SHA1(10434e492e4e9de5cd8543271914d5ba37c52b50) )
	ROM_LOAD16_BYTE( "1103-10k.052", 0x000001, 0x010000, CRC(e50bec32) SHA1(30c504c730e8e568e78e06c756a23b8923e85b4b) )
	ROM_LOAD16_BYTE( "1102-10j.052", 0x020000, 0x010000, CRC(998d3da2) SHA1(6ed560c2132e33858c91b1f4ab0247399665b5fd) )
	ROM_LOAD16_BYTE( "1104-10l.052", 0x020001, 0x010000, CRC(bc59a2b7) SHA1(7dfde5bbaa0cf349b1ef5d6b076baded7330376a) )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for ADSP I/O buffers */

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )		/* 4*128k for audio serial ROMs */
	ROM_LOAD( "1123-65a.052", 0x000000, 0x010000, CRC(a88411dc) SHA1(1fd53c7eadffa163d5423df2f8338757e58d5f2e) )
	ROM_LOAD( "1124-55a.052", 0x010000, 0x010000, CRC(071a4309) SHA1(c623bd51d6a4a56503fbf138138854d6a30b11d6) )
	ROM_LOAD( "1125-45a.052", 0x020000, 0x010000, CRC(ebf391af) SHA1(3c4097db8d625b994b39d46fe652585a74378ca0) )
	ROM_LOAD( "1126-30a.052", 0x030000, 0x010000, CRC(f46ef09c) SHA1(ba62f73ee3b33d8f26b430ffa468f8792dca23de) )
ROM_END


ROM_START( harddrvj )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 1MB for 68000 code */
	ROM_LOAD16_BYTE( "7805200r.052", 0x000000, 0x010000, CRC(f170c842) SHA1(842cef66646a664300a7e2b827c0bbd53e5f27ad) )
	ROM_LOAD16_BYTE( "7813210r.052", 0x000001, 0x010000, CRC(e5a3367d) SHA1(96b3a6fb5561fa59d7ac4b840fa1891d02f1bd8c) )
	ROM_LOAD16_BYTE( "6806200s.052", 0x020000, 0x010000, CRC(bd68696e) SHA1(33b173f210183fef84c4c04b3bff515d79b4924d) )
	ROM_LOAD16_BYTE( "6814210s.052", 0x020001, 0x010000, CRC(d0f1c6ed) SHA1(1960e70af17795a9fabd0171eaefebaaaff47305) )
	ROM_LOAD16_BYTE( "6807200s.052", 0x040000, 0x010000, CRC(b7d348a7) SHA1(b7543241a0a9159f3b49e3b6e65505f6b02ec3d7) )
	ROM_LOAD16_BYTE( "6815210s.052", 0x040001, 0x010000, CRC(c0790e8c) SHA1(5ef9cdb191583ae5ce2d8285a32f1b060f7ddb11) )
	ROM_LOAD16_BYTE( "6810200w.052", 0x0a0000, 0x010000, CRC(c3030c51) SHA1(fac9c7d715d66d523908fd7f76a5a599bf05e090) )
	ROM_LOAD16_BYTE( "6818210w.052", 0x0a0001, 0x010000, CRC(d778128a) SHA1(e9556c39dcba59abb9eba0d2ab113f98c9044558) )
	ROM_LOAD16_BYTE( "6811200x.052", 0x0c0000, 0x010000, CRC(7bc5c8da) SHA1(ac228d6391bc9ac4238f953f9ad6eb93b3ff9958) )
	ROM_LOAD16_BYTE( "6819210x.052", 0x0c0001, 0x010000, CRC(368b7e17) SHA1(e8742edec6961173df4450073a427cf3b9f3ff57) )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU4, 0 )	/* region for ADSP 2100 */

	ROM_REGION( 0x20000, REGION_CPU5, 0 )		/* 2*64k for audio 68000 code */
	ROM_LOAD16_BYTE( "1121-45n.052", 0x000001, 0x008000, CRC(04316e6f) SHA1(9836b8d16cebd6013834432c9e5a5aca0050c889) )
	ROM_LOAD16_BYTE( "1122-70n.052", 0x000000, 0x008000, CRC(0c446eec) SHA1(53576c2800484d098cf250ab9a865314167c9d96) )

	ROM_REGION( 0x10000, REGION_CPU6, 0 )		/* region for audio 32010 */

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )	/* 384k for ADSP object ROM */
	ROM_LOAD16_BYTE( "1101-10h.052", 0x000000, 0x010000, CRC(1b77f171) SHA1(10434e492e4e9de5cd8543271914d5ba37c52b50) )
	ROM_LOAD16_BYTE( "1103-10k.052", 0x000001, 0x010000, CRC(e50bec32) SHA1(30c504c730e8e568e78e06c756a23b8923e85b4b) )
	ROM_LOAD16_BYTE( "1102-10j.052", 0x020000, 0x010000, CRC(998d3da2) SHA1(6ed560c2132e33858c91b1f4ab0247399665b5fd) )
	ROM_LOAD16_BYTE( "1104-10l.052", 0x020001, 0x010000, CRC(bc59a2b7) SHA1(7dfde5bbaa0cf349b1ef5d6b076baded7330376a) )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for ADSP I/O buffers */

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )		/* 4*128k for audio serial ROMs */
	ROM_LOAD( "1123-65a.052", 0x000000, 0x010000, CRC(a88411dc) SHA1(1fd53c7eadffa163d5423df2f8338757e58d5f2e) )
	ROM_LOAD( "1124-55a.052", 0x010000, 0x010000, CRC(071a4309) SHA1(c623bd51d6a4a56503fbf138138854d6a30b11d6) )
	ROM_LOAD( "1125-45a.052", 0x020000, 0x010000, CRC(ebf391af) SHA1(3c4097db8d625b994b39d46fe652585a74378ca0) )
	ROM_LOAD( "1126-30a.052", 0x030000, 0x010000, CRC(f46ef09c) SHA1(ba62f73ee3b33d8f26b430ffa468f8792dca23de) )
ROM_END


ROM_START( harddrvb )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 1MB for 68000 code */
	ROM_LOAD16_BYTE( "7905200r.052", 0x000000, 0x010000, CRC(c4379a2e) SHA1(41dc66c1576c591520543343a798e0d80def2188) )
	ROM_LOAD16_BYTE( "7913210r.052", 0x000001, 0x010000, CRC(1e76fa17) SHA1(80ec1de4c8a4622fb5c65ec3e8c194a8eabacc38) )
	ROM_LOAD16_BYTE( "6906200s.052", 0x020000, 0x010000, CRC(47c4498b) SHA1(aad3463f29ffc733ab42a7abf739ca698cdd8ec1) )
	ROM_LOAD16_BYTE( "6914210s.052", 0x020001, 0x010000, CRC(c375add9) SHA1(2f2b419d2d092dbc5d2d9a2a44b206c1746e6cca) )
	ROM_LOAD16_BYTE( "5910200w.052", 0x0a0000, 0x010000, CRC(0d375673) SHA1(fa42f2df39e2a8d20565e981e692bde7256b277e) )
	ROM_LOAD16_BYTE( "5918210w.052", 0x0a0001, 0x010000, CRC(e2b6a8b8) SHA1(ec847ffb4bf228cf1197d6c8642000491b9e19be) )
	ROM_LOAD16_BYTE( "5911200x.052", 0x0c0000, 0x010000, CRC(564ac427) SHA1(f71d5a8d06681fc96e753e7cd18e16b32ba6907f) )
	ROM_LOAD16_BYTE( "5919210x.052", 0x0c0001, 0x010000, CRC(752d9a6d) SHA1(28edb54c7217f5ccdcb2b5614d4e8c2290d96b2a) )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU4, 0 )	/* region for ADSP 2100 */

	ROM_REGION( 0x20000, REGION_CPU5, 0 )		/* 2*64k for audio 68000 code */
	ROM_LOAD16_BYTE( "1121-45n.052", 0x000001, 0x008000, CRC(04316e6f) SHA1(9836b8d16cebd6013834432c9e5a5aca0050c889) )
	ROM_LOAD16_BYTE( "1122-70n.052", 0x000000, 0x008000, CRC(0c446eec) SHA1(53576c2800484d098cf250ab9a865314167c9d96) )

	ROM_REGION( 0x10000, REGION_CPU6, 0 )		/* region for audio 32010 */

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )	/* 384k for ADSP object ROM */
	ROM_LOAD16_BYTE( "1101-10h.052", 0x000000, 0x010000, CRC(1b77f171) SHA1(10434e492e4e9de5cd8543271914d5ba37c52b50) )
	ROM_LOAD16_BYTE( "1103-10k.052", 0x000001, 0x010000, CRC(e50bec32) SHA1(30c504c730e8e568e78e06c756a23b8923e85b4b) )
	ROM_LOAD16_BYTE( "1102-10j.052", 0x020000, 0x010000, CRC(998d3da2) SHA1(6ed560c2132e33858c91b1f4ab0247399665b5fd) )
	ROM_LOAD16_BYTE( "1104-10l.052", 0x020001, 0x010000, CRC(bc59a2b7) SHA1(7dfde5bbaa0cf349b1ef5d6b076baded7330376a) )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for ADSP I/O buffers */

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )		/* 4*128k for audio serial ROMs */
	ROM_LOAD( "1123-65a.052", 0x000000, 0x010000, CRC(a88411dc) SHA1(1fd53c7eadffa163d5423df2f8338757e58d5f2e) )
	ROM_LOAD( "1124-55a.052", 0x010000, 0x010000, CRC(071a4309) SHA1(c623bd51d6a4a56503fbf138138854d6a30b11d6) )
	ROM_LOAD( "1125-45a.052", 0x020000, 0x010000, CRC(ebf391af) SHA1(3c4097db8d625b994b39d46fe652585a74378ca0) )
	ROM_LOAD( "1126-30a.052", 0x030000, 0x010000, CRC(f46ef09c) SHA1(ba62f73ee3b33d8f26b430ffa468f8792dca23de) )
ROM_END


ROM_START( harddrb6 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 1MB for 68000 code */
	ROM_LOAD16_BYTE( "6905200r.052", 0x000000, 0x010000, CRC(b1f38593) SHA1(958622128c7c687071211343ef657c38f928e25a) )
	ROM_LOAD16_BYTE( "6913210r.052", 0x000001, 0x010000, CRC(15eb0da5) SHA1(0200328582c77d4d64e9f7df1f8755f7f4975ac3) )
	ROM_LOAD16_BYTE( "6906200s.052", 0x020000, 0x010000, CRC(47c4498b) SHA1(aad3463f29ffc733ab42a7abf739ca698cdd8ec1) )
	ROM_LOAD16_BYTE( "6914210s.052", 0x020001, 0x010000, CRC(c375add9) SHA1(2f2b419d2d092dbc5d2d9a2a44b206c1746e6cca) )
	ROM_LOAD16_BYTE( "5910200w.052", 0x0a0000, 0x010000, CRC(0d375673) SHA1(fa42f2df39e2a8d20565e981e692bde7256b277e) )
	ROM_LOAD16_BYTE( "5918210w.052", 0x0a0001, 0x010000, CRC(e2b6a8b8) SHA1(ec847ffb4bf228cf1197d6c8642000491b9e19be) )
	ROM_LOAD16_BYTE( "5911200x.052", 0x0c0000, 0x010000, CRC(564ac427) SHA1(f71d5a8d06681fc96e753e7cd18e16b32ba6907f) )
	ROM_LOAD16_BYTE( "5919210x.052", 0x0c0001, 0x010000, CRC(752d9a6d) SHA1(28edb54c7217f5ccdcb2b5614d4e8c2290d96b2a) )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU4, 0 )	/* region for ADSP 2100 */

	ROM_REGION( 0x20000, REGION_CPU5, 0 )		/* 2*64k for audio 68000 code */
	ROM_LOAD16_BYTE( "1121-45n.052", 0x000001, 0x008000, CRC(04316e6f) SHA1(9836b8d16cebd6013834432c9e5a5aca0050c889) )
	ROM_LOAD16_BYTE( "1122-70n.052", 0x000000, 0x008000, CRC(0c446eec) SHA1(53576c2800484d098cf250ab9a865314167c9d96) )

	ROM_REGION( 0x10000, REGION_CPU6, 0 )		/* region for audio 32010 */

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )	/* 384k for ADSP object ROM */
	ROM_LOAD16_BYTE( "1101-10h.052", 0x000000, 0x010000, CRC(1b77f171) SHA1(10434e492e4e9de5cd8543271914d5ba37c52b50) )
	ROM_LOAD16_BYTE( "1103-10k.052", 0x000001, 0x010000, CRC(e50bec32) SHA1(30c504c730e8e568e78e06c756a23b8923e85b4b) )
	ROM_LOAD16_BYTE( "1102-10j.052", 0x020000, 0x010000, CRC(998d3da2) SHA1(6ed560c2132e33858c91b1f4ab0247399665b5fd) )
	ROM_LOAD16_BYTE( "1104-10l.052", 0x020001, 0x010000, CRC(bc59a2b7) SHA1(7dfde5bbaa0cf349b1ef5d6b076baded7330376a) )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for ADSP I/O buffers */

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )		/* 4*128k for audio serial ROMs */
	ROM_LOAD( "1123-65a.052", 0x000000, 0x010000, CRC(a88411dc) SHA1(1fd53c7eadffa163d5423df2f8338757e58d5f2e) )
	ROM_LOAD( "1124-55a.052", 0x010000, 0x010000, CRC(071a4309) SHA1(c623bd51d6a4a56503fbf138138854d6a30b11d6) )
	ROM_LOAD( "1125-45a.052", 0x020000, 0x010000, CRC(ebf391af) SHA1(3c4097db8d625b994b39d46fe652585a74378ca0) )
	ROM_LOAD( "1126-30a.052", 0x030000, 0x010000, CRC(f46ef09c) SHA1(ba62f73ee3b33d8f26b430ffa468f8792dca23de) )
ROM_END


ROM_START( harddrj6 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 1MB for 68000 code */
	ROM_LOAD16_BYTE( "6805200r.052", 0x000000, 0x010000, CRC(e61ce7af) SHA1(38115a5a44f330abf1f7d684e630a1a232769f7c) )
	ROM_LOAD16_BYTE( "6813210r.052", 0x000001, 0x010000, CRC(adfef1e7) SHA1(dc1bdb5ce9143a94d734ce9ecdf9195589608b65) )
	ROM_LOAD16_BYTE( "6806200s.052", 0x020000, 0x010000, CRC(bd68696e) SHA1(33b173f210183fef84c4c04b3bff515d79b4924d) )
	ROM_LOAD16_BYTE( "6814210s.052", 0x020001, 0x010000, CRC(d0f1c6ed) SHA1(1960e70af17795a9fabd0171eaefebaaaff47305) )
	ROM_LOAD16_BYTE( "6807200s.052", 0x040000, 0x010000, CRC(b7d348a7) SHA1(b7543241a0a9159f3b49e3b6e65505f6b02ec3d7) )
	ROM_LOAD16_BYTE( "6815210s.052", 0x040001, 0x010000, CRC(c0790e8c) SHA1(5ef9cdb191583ae5ce2d8285a32f1b060f7ddb11) )
	ROM_LOAD16_BYTE( "6810200w.052", 0x0a0000, 0x010000, CRC(c3030c51) SHA1(fac9c7d715d66d523908fd7f76a5a599bf05e090) )
	ROM_LOAD16_BYTE( "6818210w.052", 0x0a0001, 0x010000, CRC(d778128a) SHA1(e9556c39dcba59abb9eba0d2ab113f98c9044558) )
	ROM_LOAD16_BYTE( "6811200x.052", 0x0c0000, 0x010000, CRC(7bc5c8da) SHA1(ac228d6391bc9ac4238f953f9ad6eb93b3ff9958) )
	ROM_LOAD16_BYTE( "6819210x.052", 0x0c0001, 0x010000, CRC(368b7e17) SHA1(e8742edec6961173df4450073a427cf3b9f3ff57) )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU4, 0 )	/* region for ADSP 2100 */

	ROM_REGION( 0x20000, REGION_CPU5, 0 )		/* 2*64k for audio 68000 code */
	ROM_LOAD16_BYTE( "1121-45n.052", 0x000001, 0x008000, CRC(04316e6f) SHA1(9836b8d16cebd6013834432c9e5a5aca0050c889) )
	ROM_LOAD16_BYTE( "1122-70n.052", 0x000000, 0x008000, CRC(0c446eec) SHA1(53576c2800484d098cf250ab9a865314167c9d96) )

	ROM_REGION( 0x10000, REGION_CPU6, 0 )		/* region for audio 32010 */

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )	/* 384k for ADSP object ROM */
	ROM_LOAD16_BYTE( "1101-10h.052", 0x000000, 0x010000, CRC(1b77f171) SHA1(10434e492e4e9de5cd8543271914d5ba37c52b50) )
	ROM_LOAD16_BYTE( "1103-10k.052", 0x000001, 0x010000, CRC(e50bec32) SHA1(30c504c730e8e568e78e06c756a23b8923e85b4b) )
	ROM_LOAD16_BYTE( "1102-10j.052", 0x020000, 0x010000, CRC(998d3da2) SHA1(6ed560c2132e33858c91b1f4ab0247399665b5fd) )
	ROM_LOAD16_BYTE( "1104-10l.052", 0x020001, 0x010000, CRC(bc59a2b7) SHA1(7dfde5bbaa0cf349b1ef5d6b076baded7330376a) )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for ADSP I/O buffers */

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )		/* 4*128k for audio serial ROMs */
	ROM_LOAD( "1123-65a.052", 0x000000, 0x010000, CRC(a88411dc) SHA1(1fd53c7eadffa163d5423df2f8338757e58d5f2e) )
	ROM_LOAD( "1124-55a.052", 0x010000, 0x010000, CRC(071a4309) SHA1(c623bd51d6a4a56503fbf138138854d6a30b11d6) )
	ROM_LOAD( "1125-45a.052", 0x020000, 0x010000, CRC(ebf391af) SHA1(3c4097db8d625b994b39d46fe652585a74378ca0) )
	ROM_LOAD( "1126-30a.052", 0x030000, 0x010000, CRC(f46ef09c) SHA1(ba62f73ee3b33d8f26b430ffa468f8792dca23de) )
ROM_END


ROM_START( harddrb5 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 1MB for 68000 code */
	ROM_LOAD16_BYTE( "5905200r.052", 0x000000, 0x010000, CRC(6dd28313) SHA1(c1f860470070dad7d8865aca508ecf5eb92fa8c5) )
	ROM_LOAD16_BYTE( "5913210r.052", 0x000001, 0x010000, CRC(2e5a006f) SHA1(8a2da1868bbad54b33e993f5bf9ba61186caef5b) )
	ROM_LOAD16_BYTE( "5906200s.052", 0x020000, 0x010000, CRC(206b59db) SHA1(2bbae0ac4af9cdda44d6d8b5fcf6b3b1c3103796) )
	ROM_LOAD16_BYTE( "5914210s.052", 0x020001, 0x010000, CRC(50408162) SHA1(b99f9de8e2cfdf13818d46de3e2d34c8d28d9b50) )
	ROM_LOAD16_BYTE( "5910200w.052", 0x0a0000, 0x010000, CRC(0d375673) SHA1(fa42f2df39e2a8d20565e981e692bde7256b277e) )
	ROM_LOAD16_BYTE( "5918210w.052", 0x0a0001, 0x010000, CRC(e2b6a8b8) SHA1(ec847ffb4bf228cf1197d6c8642000491b9e19be) )
	ROM_LOAD16_BYTE( "5911200x.052", 0x0c0000, 0x010000, CRC(564ac427) SHA1(f71d5a8d06681fc96e753e7cd18e16b32ba6907f) )
	ROM_LOAD16_BYTE( "5919210x.052", 0x0c0001, 0x010000, CRC(752d9a6d) SHA1(28edb54c7217f5ccdcb2b5614d4e8c2290d96b2a) )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU4, 0 )	/* region for ADSP 2100 */

	ROM_REGION( 0x20000, REGION_CPU5, 0 )		/* 2*64k for audio 68000 code */
	ROM_LOAD16_BYTE( "1121-45n.052", 0x000001, 0x008000, CRC(04316e6f) SHA1(9836b8d16cebd6013834432c9e5a5aca0050c889) )
	ROM_LOAD16_BYTE( "1122-70n.052", 0x000000, 0x008000, CRC(0c446eec) SHA1(53576c2800484d098cf250ab9a865314167c9d96) )

	ROM_REGION( 0x10000, REGION_CPU6, 0 )		/* region for audio 32010 */

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )	/* 384k for ADSP object ROM */
	ROM_LOAD16_BYTE( "1101-10h.052", 0x000000, 0x010000, CRC(1b77f171) SHA1(10434e492e4e9de5cd8543271914d5ba37c52b50) )
	ROM_LOAD16_BYTE( "1103-10k.052", 0x000001, 0x010000, CRC(e50bec32) SHA1(30c504c730e8e568e78e06c756a23b8923e85b4b) )
	ROM_LOAD16_BYTE( "1102-10j.052", 0x020000, 0x010000, CRC(998d3da2) SHA1(6ed560c2132e33858c91b1f4ab0247399665b5fd) )
	ROM_LOAD16_BYTE( "1104-10l.052", 0x020001, 0x010000, CRC(bc59a2b7) SHA1(7dfde5bbaa0cf349b1ef5d6b076baded7330376a) )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for ADSP I/O buffers */

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )		/* 4*128k for audio serial ROMs */
	ROM_LOAD( "1123-65a.052", 0x000000, 0x010000, CRC(a88411dc) SHA1(1fd53c7eadffa163d5423df2f8338757e58d5f2e) )
	ROM_LOAD( "1124-55a.052", 0x010000, 0x010000, CRC(071a4309) SHA1(c623bd51d6a4a56503fbf138138854d6a30b11d6) )
	ROM_LOAD( "1125-45a.052", 0x020000, 0x010000, CRC(ebf391af) SHA1(3c4097db8d625b994b39d46fe652585a74378ca0) )
	ROM_LOAD( "1126-30a.052", 0x030000, 0x010000, CRC(f46ef09c) SHA1(ba62f73ee3b33d8f26b430ffa468f8792dca23de) )
ROM_END


ROM_START( harddrg4 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 1MB for 68000 code */
	ROM_LOAD16_BYTE( "4205200r.052", 0x000000, 0x010000, CRC(94db6d9e) SHA1(98a509daacb9f18c7171246d1782d09a6456e241) )
	ROM_LOAD16_BYTE( "4213210r.052", 0x000001, 0x010000, CRC(cacaf671) SHA1(7bd0f531402b2f10cb00bb8f7ca0b62eed23bffa) )
	ROM_LOAD16_BYTE( "4206200s.052", 0x020000, 0x010000, CRC(70f78c50) SHA1(6baed202937c455c7c6e4baa74ee7858673a37fc) )
	ROM_LOAD16_BYTE( "4214210s.052", 0x020001, 0x010000, CRC(b7988de4) SHA1(0bc94c3eb12f8d0506eca819d79dffb7758a8421) )
	ROM_LOAD16_BYTE( "4210200w.052", 0x0a0000, 0x010000, CRC(5b8b2947) SHA1(7390e978361dfb6bf5675014a906f3f9879a8a59) )
	ROM_LOAD16_BYTE( "4218210w.052", 0x0a0001, 0x010000, CRC(9be232c5) SHA1(4fd30f53814049f8462937bfe55410edae438411) )
	ROM_LOAD16_BYTE( "4211200x.052", 0x0c0000, 0x010000, CRC(20d1b3d5) SHA1(b7c4a2cf6ba729530c24980704989b3a3efb343b) )
	ROM_LOAD16_BYTE( "4219210x.052", 0x0c0001, 0x010000, CRC(105e7052) SHA1(a8d13c35418e58410cfd74c243b74963f1553068) )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU4, 0 )	/* region for ADSP 2100 */

	ROM_REGION( 0x20000, REGION_CPU5, 0 )		/* 2*64k for audio 68000 code */
	ROM_LOAD16_BYTE( "1121-45n.052", 0x000001, 0x008000, CRC(04316e6f) SHA1(9836b8d16cebd6013834432c9e5a5aca0050c889) )
	ROM_LOAD16_BYTE( "1122-70n.052", 0x000000, 0x008000, CRC(0c446eec) SHA1(53576c2800484d098cf250ab9a865314167c9d96) )

	ROM_REGION( 0x10000, REGION_CPU6, 0 )		/* region for audio 32010 */

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )	/* 384k for ADSP object ROM */
	ROM_LOAD16_BYTE( "1101-10h.052", 0x000000, 0x010000, CRC(1b77f171) SHA1(10434e492e4e9de5cd8543271914d5ba37c52b50) )
	ROM_LOAD16_BYTE( "1103-10k.052", 0x000001, 0x010000, CRC(e50bec32) SHA1(30c504c730e8e568e78e06c756a23b8923e85b4b) )
	ROM_LOAD16_BYTE( "1102-10j.052", 0x020000, 0x010000, CRC(998d3da2) SHA1(6ed560c2132e33858c91b1f4ab0247399665b5fd) )
	ROM_LOAD16_BYTE( "1104-10l.052", 0x020001, 0x010000, CRC(bc59a2b7) SHA1(7dfde5bbaa0cf349b1ef5d6b076baded7330376a) )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for ADSP I/O buffers */

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )		/* 4*128k for audio serial ROMs */
	ROM_LOAD( "1123-65a.052", 0x000000, 0x010000, CRC(a88411dc) SHA1(1fd53c7eadffa163d5423df2f8338757e58d5f2e) )
	ROM_LOAD( "1124-55a.052", 0x010000, 0x010000, CRC(071a4309) SHA1(c623bd51d6a4a56503fbf138138854d6a30b11d6) )
	ROM_LOAD( "1125-45a.052", 0x020000, 0x010000, CRC(ebf391af) SHA1(3c4097db8d625b994b39d46fe652585a74378ca0) )
	ROM_LOAD( "1126-30a.052", 0x030000, 0x010000, CRC(f46ef09c) SHA1(ba62f73ee3b33d8f26b430ffa468f8792dca23de) )
ROM_END


ROM_START( harddrv3 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 1MB for 68000 code */
	ROM_LOAD16_BYTE( "3105200r.052", 0x000000, 0x010000, CRC(a42a2c69) SHA1(66233f25533106aab69df21db69f96368f1399a9) )
	ROM_LOAD16_BYTE( "3113210r.052", 0x000001, 0x010000, CRC(358995b5) SHA1(f18c0da58ec7befefc61d5f0d35787516b775c92) )
	ROM_LOAD16_BYTE( "2106200s.052", 0x020000, 0x010000, CRC(a668db0e) SHA1(8ac405a0ba12bac9acabdb64970608d1b2b1a99b) )
	ROM_LOAD16_BYTE( "2114210s.052", 0x020001, 0x010000, CRC(ab689a94) SHA1(c6c09e088bcc32030217e3521c862acce113bf93) )
	ROM_LOAD16_BYTE( "1110200w.052", 0x0a0000, 0x010000, CRC(908ccbbe) SHA1(b6947ade664172a4553ea083fadfcb77c8c3938d) )
	ROM_LOAD16_BYTE( "1118210w.052", 0x0a0001, 0x010000, CRC(5b25023c) SHA1(e6c5bf0de5ee071b8733fc890ae4f906732adde4) )
	ROM_LOAD16_BYTE( "1111200x.052", 0x0c0000, 0x010000, CRC(e1f455a3) SHA1(68462a33bbfcc526d8f27ec082e55937a26ead8b) )
	ROM_LOAD16_BYTE( "1119210x.052", 0x0c0001, 0x010000, CRC(a7fc3aaa) SHA1(ce8d4a8f83e25008cafa2a2242ed26b90b8517da) )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU4, 0 )	/* region for ADSP 2100 */

	ROM_REGION( 0x20000, REGION_CPU5, 0 )		/* 2*64k for audio 68000 code */
	ROM_LOAD16_BYTE( "1121-45n.052", 0x000001, 0x008000, CRC(04316e6f) SHA1(9836b8d16cebd6013834432c9e5a5aca0050c889) )
	ROM_LOAD16_BYTE( "1122-70n.052", 0x000000, 0x008000, CRC(0c446eec) SHA1(53576c2800484d098cf250ab9a865314167c9d96) )

	ROM_REGION( 0x10000, REGION_CPU6, 0 )		/* region for audio 32010 */

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )	/* 384k for ADSP object ROM */
	ROM_LOAD16_BYTE( "1101-10h.052", 0x000000, 0x010000, CRC(1b77f171) SHA1(10434e492e4e9de5cd8543271914d5ba37c52b50) )
	ROM_LOAD16_BYTE( "1103-10k.052", 0x000001, 0x010000, CRC(e50bec32) SHA1(30c504c730e8e568e78e06c756a23b8923e85b4b) )
	ROM_LOAD16_BYTE( "1102-10j.052", 0x020000, 0x010000, CRC(998d3da2) SHA1(6ed560c2132e33858c91b1f4ab0247399665b5fd) )
	ROM_LOAD16_BYTE( "1104-10l.052", 0x020001, 0x010000, CRC(bc59a2b7) SHA1(7dfde5bbaa0cf349b1ef5d6b076baded7330376a) )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for ADSP I/O buffers */

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )		/* 4*128k for audio serial ROMs */
	ROM_LOAD( "1123-65a.052", 0x000000, 0x010000, CRC(a88411dc) SHA1(1fd53c7eadffa163d5423df2f8338757e58d5f2e) )
	ROM_LOAD( "1124-55a.052", 0x010000, 0x010000, CRC(071a4309) SHA1(c623bd51d6a4a56503fbf138138854d6a30b11d6) )
	ROM_LOAD( "1125-45a.052", 0x020000, 0x010000, CRC(ebf391af) SHA1(3c4097db8d625b994b39d46fe652585a74378ca0) )
	ROM_LOAD( "1126-30a.052", 0x030000, 0x010000, CRC(f46ef09c) SHA1(ba62f73ee3b33d8f26b430ffa468f8792dca23de) )
ROM_END


ROM_START( harddrv2 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 1MB for 68000 code */
	ROM_LOAD16_BYTE( "2105200r.052", 0x000000, 0x010000, CRC(967903b9) SHA1(4363f3b3f08d00819f47a3682462fd0e5dc4e0ac) )
	ROM_LOAD16_BYTE( "2113210r.052", 0x000001, 0x010000, CRC(c87f7067) SHA1(3d85d2ba818739ed05086502cd8e57438251b20b) )
	ROM_LOAD16_BYTE( "2106200s.052", 0x020000, 0x010000, CRC(a668db0e) SHA1(8ac405a0ba12bac9acabdb64970608d1b2b1a99b) )
	ROM_LOAD16_BYTE( "2114210s.052", 0x020001, 0x010000, CRC(ab689a94) SHA1(c6c09e088bcc32030217e3521c862acce113bf93) )
	ROM_LOAD16_BYTE( "1110200w.052", 0x0a0000, 0x010000, CRC(908ccbbe) SHA1(b6947ade664172a4553ea083fadfcb77c8c3938d) )
	ROM_LOAD16_BYTE( "1118210w.052", 0x0a0001, 0x010000, CRC(5b25023c) SHA1(e6c5bf0de5ee071b8733fc890ae4f906732adde4) )
	ROM_LOAD16_BYTE( "1111200x.052", 0x0c0000, 0x010000, CRC(e1f455a3) SHA1(68462a33bbfcc526d8f27ec082e55937a26ead8b) )
	ROM_LOAD16_BYTE( "1119210x.052", 0x0c0001, 0x010000, CRC(a7fc3aaa) SHA1(ce8d4a8f83e25008cafa2a2242ed26b90b8517da) )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU4, 0 )	/* region for ADSP 2100 */

	ROM_REGION( 0x20000, REGION_CPU5, 0 )		/* 2*64k for audio 68000 code */
	ROM_LOAD16_BYTE( "1121-45n.052", 0x000001, 0x008000, CRC(04316e6f) SHA1(9836b8d16cebd6013834432c9e5a5aca0050c889) )
	ROM_LOAD16_BYTE( "1122-70n.052", 0x000000, 0x008000, CRC(0c446eec) SHA1(53576c2800484d098cf250ab9a865314167c9d96) )

	ROM_REGION( 0x10000, REGION_CPU6, 0 )		/* region for audio 32010 */

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )	/* 384k for ADSP object ROM */
	ROM_LOAD16_BYTE( "1101-10h.052", 0x000000, 0x010000, CRC(1b77f171) SHA1(10434e492e4e9de5cd8543271914d5ba37c52b50) )
	ROM_LOAD16_BYTE( "1103-10k.052", 0x000001, 0x010000, CRC(e50bec32) SHA1(30c504c730e8e568e78e06c756a23b8923e85b4b) )
	ROM_LOAD16_BYTE( "1102-10j.052", 0x020000, 0x010000, CRC(998d3da2) SHA1(6ed560c2132e33858c91b1f4ab0247399665b5fd) )
	ROM_LOAD16_BYTE( "1104-10l.052", 0x020001, 0x010000, CRC(bc59a2b7) SHA1(7dfde5bbaa0cf349b1ef5d6b076baded7330376a) )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for ADSP I/O buffers */

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )		/* 4*128k for audio serial ROMs */
	ROM_LOAD( "1123-65a.052", 0x000000, 0x010000, CRC(a88411dc) SHA1(1fd53c7eadffa163d5423df2f8338757e58d5f2e) )
	ROM_LOAD( "1124-55a.052", 0x010000, 0x010000, CRC(071a4309) SHA1(c623bd51d6a4a56503fbf138138854d6a30b11d6) )
	ROM_LOAD( "1125-45a.052", 0x020000, 0x010000, CRC(ebf391af) SHA1(3c4097db8d625b994b39d46fe652585a74378ca0) )
	ROM_LOAD( "1126-30a.052", 0x030000, 0x010000, CRC(f46ef09c) SHA1(ba62f73ee3b33d8f26b430ffa468f8792dca23de) )
ROM_END


ROM_START( harddrv1 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 1MB for 68000 code */
	ROM_LOAD16_BYTE( "1105200r.052", 0x000000, 0x010000, CRC(1dfda789) SHA1(83be0d66fe21e974f8d4582b0ab60245f1db7e55) )
	ROM_LOAD16_BYTE( "1113210r.052", 0x000001, 0x010000, CRC(db2f6593) SHA1(864ea5604cea2507b180a91b35e349d3924bf9bc) )
	ROM_LOAD16_BYTE( "1106200s.052", 0x020000, 0x010000, CRC(e82210fd) SHA1(916e674017f5187531c2de7961791c060f8412c7) )
	ROM_LOAD16_BYTE( "1114210s.052", 0x020001, 0x010000, CRC(3d152e8c) SHA1(c55ac13922b7d2126dd9643ef5026ac272abcf16) )
	ROM_LOAD16_BYTE( "1110200w.052", 0x0a0000, 0x010000, CRC(908ccbbe) SHA1(b6947ade664172a4553ea083fadfcb77c8c3938d) )
	ROM_LOAD16_BYTE( "1118210w.052", 0x0a0001, 0x010000, CRC(5b25023c) SHA1(e6c5bf0de5ee071b8733fc890ae4f906732adde4) )
	ROM_LOAD16_BYTE( "1111200x.052", 0x0c0000, 0x010000, CRC(e1f455a3) SHA1(68462a33bbfcc526d8f27ec082e55937a26ead8b) )
	ROM_LOAD16_BYTE( "1119210x.052", 0x0c0001, 0x010000, CRC(a7fc3aaa) SHA1(ce8d4a8f83e25008cafa2a2242ed26b90b8517da) )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU4, 0 )	/* region for ADSP 2100 */

	ROM_REGION( 0x20000, REGION_CPU5, 0 )		/* 2*64k for audio 68000 code */
	ROM_LOAD16_BYTE( "1121-45n.052", 0x000001, 0x008000, CRC(04316e6f) SHA1(9836b8d16cebd6013834432c9e5a5aca0050c889) )
	ROM_LOAD16_BYTE( "1122-70n.052", 0x000000, 0x008000, CRC(0c446eec) SHA1(53576c2800484d098cf250ab9a865314167c9d96) )

	ROM_REGION( 0x10000, REGION_CPU6, 0 )		/* region for audio 32010 */

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )	/* 384k for ADSP object ROM */
	ROM_LOAD16_BYTE( "1101-10h.052", 0x000000, 0x010000, CRC(1b77f171) SHA1(10434e492e4e9de5cd8543271914d5ba37c52b50) )
	ROM_LOAD16_BYTE( "1103-10k.052", 0x000001, 0x010000, CRC(e50bec32) SHA1(30c504c730e8e568e78e06c756a23b8923e85b4b) )
	ROM_LOAD16_BYTE( "1102-10j.052", 0x020000, 0x010000, CRC(998d3da2) SHA1(6ed560c2132e33858c91b1f4ab0247399665b5fd) )
	ROM_LOAD16_BYTE( "1104-10l.052", 0x020001, 0x010000, CRC(bc59a2b7) SHA1(7dfde5bbaa0cf349b1ef5d6b076baded7330376a) )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for ADSP I/O buffers */

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )		/* 4*128k for audio serial ROMs */
	ROM_LOAD( "1123-65a.052", 0x000000, 0x010000, CRC(a88411dc) SHA1(1fd53c7eadffa163d5423df2f8338757e58d5f2e) )
	ROM_LOAD( "1124-55a.052", 0x010000, 0x010000, CRC(071a4309) SHA1(c623bd51d6a4a56503fbf138138854d6a30b11d6) )
	ROM_LOAD( "1125-45a.052", 0x020000, 0x010000, CRC(ebf391af) SHA1(3c4097db8d625b994b39d46fe652585a74378ca0) )
	ROM_LOAD( "1126-30a.052", 0x030000, 0x010000, CRC(f46ef09c) SHA1(ba62f73ee3b33d8f26b430ffa468f8792dca23de) )
ROM_END


ROM_START( harddrvc )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )	/* 1MB for 68000 code */
	ROM_LOAD16_BYTE( "2102200r.068", 0x000000, 0x010000, CRC(6252048b) SHA1(64caf3adfad6965768fc6d39a8bcde62fe6dfa9e) )
	ROM_LOAD16_BYTE( "2101210r.068", 0x000001, 0x010000, CRC(4805ba06) SHA1(e0c2d935ced05b8162f2925520422184a81d5294) )
	ROM_LOAD16_BYTE( "2104200s.068", 0x020000, 0x010000, CRC(8246f945) SHA1(633b6c9a5d3e33d3035ccdb7b6ad883c334a4db9) )
	ROM_LOAD16_BYTE( "2103210s.068", 0x020001, 0x010000, CRC(729941e8) SHA1(30d1e76803154195492acacf8c911d1f70cb92f5) )
	ROM_LOAD16_BYTE( "1112200w.068", 0x0a0000, 0x010000, CRC(e5ea74e4) SHA1(58a8c0f16573fcc2c8739e6f72e485271e45af88) )
	ROM_LOAD16_BYTE( "1111210w.068", 0x0a0001, 0x010000, CRC(4d759891) SHA1(b82087d9549ccc2a7eef22591dd8b869f2768075) )
	ROM_LOAD16_BYTE( "1114200x.068", 0x0c0000, 0x010000, CRC(293c153b) SHA1(6300a50766b19ad203b5c7da28d51bf22054b39e) )
	ROM_LOAD16_BYTE( "1113210x.068", 0x0c0001, 0x010000, CRC(5630390d) SHA1(cd1932cee70cddd1fb2110d1aeebb573a13f1339) )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU4, 0 )	/* region for ADSP 2100 */

	ROM_REGION( 0x20000, REGION_CPU5, 0 )		/* 2*64k for audio 68000 code */
	ROM_LOAD16_BYTE( "3122-70n.052", 0x000000, 0x008000, CRC(3f20a396) SHA1(f34819796087c543083f6baac6c778e0cdb7340a) )
	ROM_LOAD16_BYTE( "3121-45n.052", 0x000001, 0x008000, CRC(6346bca3) SHA1(707dc86305142722a4757ba431cf6c7e9cf116b3) )

	ROM_REGION( 0x10000, REGION_CPU6, 0 )		/* region for audio 32010 */

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )	/* 384k for ADSP object ROM */
	ROM_LOAD16_BYTE( "1101-10h.052", 0x000000, 0x010000, CRC(1b77f171) SHA1(10434e492e4e9de5cd8543271914d5ba37c52b50) )
	ROM_LOAD16_BYTE( "1103-10k.052", 0x000001, 0x010000, CRC(e50bec32) SHA1(30c504c730e8e568e78e06c756a23b8923e85b4b) )
	ROM_LOAD16_BYTE( "1102-10j.052", 0x020000, 0x010000, CRC(998d3da2) SHA1(6ed560c2132e33858c91b1f4ab0247399665b5fd) )
	ROM_LOAD16_BYTE( "1104-10l.052", 0x020001, 0x010000, CRC(bc59a2b7) SHA1(7dfde5bbaa0cf349b1ef5d6b076baded7330376a) )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for ADSP I/O buffers */

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )		/* 4*128k for audio serial ROMs */
	ROM_LOAD( "1123-65a.052", 0x000000, 0x010000, CRC(a88411dc) SHA1(1fd53c7eadffa163d5423df2f8338757e58d5f2e) )
	ROM_LOAD( "1124-55a.052", 0x010000, 0x010000, CRC(071a4309) SHA1(c623bd51d6a4a56503fbf138138854d6a30b11d6) )
	ROM_LOAD( "3125-45a.052", 0x020000, 0x010000, CRC(856548ff) SHA1(e8a17b274185c5e4ecf5f9f1c211e18b3ef2456d) )
	ROM_LOAD( "1126-30a.052", 0x030000, 0x010000, CRC(f46ef09c) SHA1(ba62f73ee3b33d8f26b430ffa468f8792dca23de) )
ROM_END


ROM_START( harddrcg )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )	/* 1MB for 68000 code */
	ROM_LOAD16_BYTE( "2202200r.068", 0x000000, 0x010000, CRC(ea09bdc2) SHA1(9e38ed0a47a2bfb6b80cbf1e0b3e891b94f30c10) )
	ROM_LOAD16_BYTE( "2201210r.068", 0x000001, 0x010000, CRC(a5e9947f) SHA1(75973346c9399752d8ae82fbada3a24c6d5fec94) )
	ROM_LOAD16_BYTE( "2204200s.068", 0x020000, 0x010000, CRC(65792c94) SHA1(89e93cf0d2afc4bd8a01155ba908c9aa7149dc78) )
	ROM_LOAD16_BYTE( "2203210s.068", 0x020001, 0x010000, CRC(c93e36af) SHA1(51f96122b20cfacc6a7bc6e18c75bd57bb51bf11) )
	ROM_LOAD16_BYTE( "1112200w.068", 0x0a0000, 0x010000, CRC(e5ea74e4) SHA1(58a8c0f16573fcc2c8739e6f72e485271e45af88) )
	ROM_LOAD16_BYTE( "1111210w.068", 0x0a0001, 0x010000, CRC(4d759891) SHA1(b82087d9549ccc2a7eef22591dd8b869f2768075) )
	ROM_LOAD16_BYTE( "1114200x.068", 0x0c0000, 0x010000, CRC(293c153b) SHA1(6300a50766b19ad203b5c7da28d51bf22054b39e) )
	ROM_LOAD16_BYTE( "1113210x.068", 0x0c0001, 0x010000, CRC(5630390d) SHA1(cd1932cee70cddd1fb2110d1aeebb573a13f1339) )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU4, 0 )	/* region for ADSP 2100 */

	ROM_REGION( 0x20000, REGION_CPU5, 0 )		/* 2*64k for audio 68000 code */
	ROM_LOAD16_BYTE( "3122-70n.052", 0x000000, 0x008000, CRC(3f20a396) SHA1(f34819796087c543083f6baac6c778e0cdb7340a) )
	ROM_LOAD16_BYTE( "3121-45n.052", 0x000001, 0x008000, CRC(6346bca3) SHA1(707dc86305142722a4757ba431cf6c7e9cf116b3) )

	ROM_REGION( 0x10000, REGION_CPU6, 0 )		/* region for audio 32010 */

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )	/* 384k for ADSP object ROM */
	ROM_LOAD16_BYTE( "1101-10h.052", 0x000000, 0x010000, CRC(1b77f171) SHA1(10434e492e4e9de5cd8543271914d5ba37c52b50) )
	ROM_LOAD16_BYTE( "1103-10k.052", 0x000001, 0x010000, CRC(e50bec32) SHA1(30c504c730e8e568e78e06c756a23b8923e85b4b) )
	ROM_LOAD16_BYTE( "1102-10j.052", 0x020000, 0x010000, CRC(998d3da2) SHA1(6ed560c2132e33858c91b1f4ab0247399665b5fd) )
	ROM_LOAD16_BYTE( "1104-10l.052", 0x020001, 0x010000, CRC(bc59a2b7) SHA1(7dfde5bbaa0cf349b1ef5d6b076baded7330376a) )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for ADSP I/O buffers */

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )		/* 4*128k for audio serial ROMs */
	ROM_LOAD( "1123-65a.052", 0x000000, 0x010000, CRC(a88411dc) SHA1(1fd53c7eadffa163d5423df2f8338757e58d5f2e) )
	ROM_LOAD( "1124-55a.052", 0x010000, 0x010000, CRC(071a4309) SHA1(c623bd51d6a4a56503fbf138138854d6a30b11d6) )
	ROM_LOAD( "3125-45a.052", 0x020000, 0x010000, CRC(856548ff) SHA1(e8a17b274185c5e4ecf5f9f1c211e18b3ef2456d) )
	ROM_LOAD( "1126-30a.052", 0x030000, 0x010000, CRC(f46ef09c) SHA1(ba62f73ee3b33d8f26b430ffa468f8792dca23de) )
ROM_END


ROM_START( harddrcb )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )	/* 1MB for 68000 code */
	ROM_LOAD16_BYTE( "2102200r.068", 0x000000, 0x010000, CRC(6252048b) SHA1(64caf3adfad6965768fc6d39a8bcde62fe6dfa9e) )
	ROM_LOAD16_BYTE( "2101210r.068", 0x000001, 0x010000, CRC(4805ba06) SHA1(e0c2d935ced05b8162f2925520422184a81d5294) )
	ROM_LOAD16_BYTE( "2104200s.068", 0x020000, 0x010000, CRC(8246f945) SHA1(633b6c9a5d3e33d3035ccdb7b6ad883c334a4db9) )
	ROM_LOAD16_BYTE( "2103210s.068", 0x020001, 0x010000, CRC(729941e8) SHA1(30d1e76803154195492acacf8c911d1f70cb92f5) )
	ROM_LOAD16_BYTE( "2912200w.068", 0x0a0000, 0x010000, CRC(ab060b8b) SHA1(84cbb5bd0114536869503afc4b9fbf6b55883afc) )
	ROM_LOAD16_BYTE( "2911210w.068", 0x0a0001, 0x010000, CRC(4fea9caf) SHA1(f5c7d9b53334c7875f0662ec165458b03ab9c0d0) )
	ROM_LOAD16_BYTE( "2914200x.068", 0x0c0000, 0x010000, CRC(dd2998a5) SHA1(36c71549d114309b9072ced83ed149cad1d23646) )
	ROM_LOAD16_BYTE( "2913210x.068", 0x0c0001, 0x010000, CRC(a211754d) SHA1(b93d4867f664caa76f2170ef0934194de29a5516) )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU4, 0 )	/* region for ADSP 2100 */

	ROM_REGION( 0x20000, REGION_CPU5, 0 )		/* 2*64k for audio 68000 code */
	ROM_LOAD16_BYTE( "3122-70n.052", 0x000000, 0x008000, CRC(3f20a396) SHA1(f34819796087c543083f6baac6c778e0cdb7340a) )
	ROM_LOAD16_BYTE( "3121-45n.052", 0x000001, 0x008000, CRC(6346bca3) SHA1(707dc86305142722a4757ba431cf6c7e9cf116b3) )

	ROM_REGION( 0x10000, REGION_CPU6, 0 )		/* region for audio 32010 */

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )	/* 384k for ADSP object ROM */
	ROM_LOAD16_BYTE( "1101-10h.052", 0x000000, 0x010000, CRC(1b77f171) SHA1(10434e492e4e9de5cd8543271914d5ba37c52b50) )
	ROM_LOAD16_BYTE( "1103-10k.052", 0x000001, 0x010000, CRC(e50bec32) SHA1(30c504c730e8e568e78e06c756a23b8923e85b4b) )
	ROM_LOAD16_BYTE( "1102-10j.052", 0x020000, 0x010000, CRC(998d3da2) SHA1(6ed560c2132e33858c91b1f4ab0247399665b5fd) )
	ROM_LOAD16_BYTE( "1104-10l.052", 0x020001, 0x010000, CRC(bc59a2b7) SHA1(7dfde5bbaa0cf349b1ef5d6b076baded7330376a) )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for ADSP I/O buffers */

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )		/* 4*128k for audio serial ROMs */
	ROM_LOAD( "1123-65a.052", 0x000000, 0x010000, CRC(a88411dc) SHA1(1fd53c7eadffa163d5423df2f8338757e58d5f2e) )
	ROM_LOAD( "1124-55a.052", 0x010000, 0x010000, CRC(071a4309) SHA1(c623bd51d6a4a56503fbf138138854d6a30b11d6) )
	ROM_LOAD( "3125-45a.052", 0x020000, 0x010000, CRC(856548ff) SHA1(e8a17b274185c5e4ecf5f9f1c211e18b3ef2456d) )
	ROM_LOAD( "1126-30a.052", 0x030000, 0x010000, CRC(f46ef09c) SHA1(ba62f73ee3b33d8f26b430ffa468f8792dca23de) )
ROM_END


ROM_START( harddrc1 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )	/* 1MB for 68000 code */
	ROM_LOAD16_BYTE( "1102200r.068", 0x000000, 0x010000, CRC(f1d3779f) SHA1(1ab84fc920f6810aa7c1af80b0472ea17d07e58b) )
	ROM_LOAD16_BYTE( "1101210r.068", 0x000001, 0x010000, CRC(3ea4d3d9) SHA1(3901e8c07bed35bf6a55ab92f5c7eb9335675792) )
	ROM_LOAD16_BYTE( "1104200s.068", 0x020000, 0x010000, CRC(34133511) SHA1(71c56607cecd379761f700d181f6e91e7bb6fdfc) )
	ROM_LOAD16_BYTE( "1103210s.068", 0x020001, 0x010000, CRC(05bb8376) SHA1(19bde6996b84368d28374a981dca88a946653dd2) )
	ROM_LOAD16_BYTE( "1112200w.068", 0x0a0000, 0x010000, CRC(e5ea74e4) SHA1(58a8c0f16573fcc2c8739e6f72e485271e45af88) )
	ROM_LOAD16_BYTE( "1111210w.068", 0x0a0001, 0x010000, CRC(4d759891) SHA1(b82087d9549ccc2a7eef22591dd8b869f2768075) )
	ROM_LOAD16_BYTE( "1114200x.068", 0x0c0000, 0x010000, CRC(293c153b) SHA1(6300a50766b19ad203b5c7da28d51bf22054b39e) )
	ROM_LOAD16_BYTE( "1113210x.068", 0x0c0001, 0x010000, CRC(5630390d) SHA1(cd1932cee70cddd1fb2110d1aeebb573a13f1339) )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU4, 0 )	/* region for ADSP 2100 */

	ROM_REGION( 0x20000, REGION_CPU5, 0 )		/* 2*64k for audio 68000 code */
	ROM_LOAD16_BYTE( "3122-70n.052", 0x000000, 0x008000, CRC(3f20a396) SHA1(f34819796087c543083f6baac6c778e0cdb7340a) )
	ROM_LOAD16_BYTE( "3121-45n.052", 0x000001, 0x008000, CRC(6346bca3) SHA1(707dc86305142722a4757ba431cf6c7e9cf116b3) )

	ROM_REGION( 0x10000, REGION_CPU6, 0 )		/* region for audio 32010 */

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )	/* 384k for ADSP object ROM */
	ROM_LOAD16_BYTE( "1101-10h.052", 0x000000, 0x010000, CRC(1b77f171) SHA1(10434e492e4e9de5cd8543271914d5ba37c52b50) )
	ROM_LOAD16_BYTE( "1103-10k.052", 0x000001, 0x010000, CRC(e50bec32) SHA1(30c504c730e8e568e78e06c756a23b8923e85b4b) )
	ROM_LOAD16_BYTE( "1102-10j.052", 0x020000, 0x010000, CRC(998d3da2) SHA1(6ed560c2132e33858c91b1f4ab0247399665b5fd) )
	ROM_LOAD16_BYTE( "1104-10l.052", 0x020001, 0x010000, CRC(bc59a2b7) SHA1(7dfde5bbaa0cf349b1ef5d6b076baded7330376a) )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for ADSP I/O buffers */

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )		/* 4*128k for audio serial ROMs */
	ROM_LOAD( "1123-65a.052", 0x000000, 0x010000, CRC(a88411dc) SHA1(1fd53c7eadffa163d5423df2f8338757e58d5f2e) )
	ROM_LOAD( "1124-55a.052", 0x010000, 0x010000, CRC(071a4309) SHA1(c623bd51d6a4a56503fbf138138854d6a30b11d6) )
	ROM_LOAD( "3125-45a.052", 0x020000, 0x010000, CRC(856548ff) SHA1(e8a17b274185c5e4ecf5f9f1c211e18b3ef2456d) )
	ROM_LOAD( "1126-30a.052", 0x030000, 0x010000, CRC(f46ef09c) SHA1(ba62f73ee3b33d8f26b430ffa468f8792dca23de) )
ROM_END


ROM_START( stunrun )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 1MB for 68000 code */
	ROM_LOAD16_BYTE( "2102200r.070", 0x000000, 0x010000, CRC(e0ed54d8) SHA1(15850568d8308b6499cbe55b5d8308041d906a29) )
	ROM_LOAD16_BYTE( "2101210r.070", 0x000001, 0x010000, CRC(3008bcf8) SHA1(9d3a20b639969bab68441f76467ed60e395c10e3) )
	ROM_LOAD16_BYTE( "6104200s.070", 0x020000, 0x010000, CRC(f469f91c) SHA1(488ea6251ff4028edaf00bb1ded74cefdc71a373) )
	ROM_LOAD16_BYTE( "6103210s.070", 0x020001, 0x010000, CRC(096aad68) SHA1(f83c9f61854dc9c1fa7dc668dbf99f4a4bbe79ca) )
	ROM_LOAD16_BYTE( "6106200t.070", 0x040000, 0x010000, CRC(b445bae5) SHA1(d530b535a47c4920c43852e7904080e6abf30098) )
	ROM_LOAD16_BYTE( "6105210t.070", 0x040001, 0x010000, CRC(42736c52) SHA1(42e0b451b172223cce9c43835363136146eeddba) )
	ROM_LOAD16_BYTE( "2108200u.070", 0x060000, 0x010000, CRC(0ce849aa) SHA1(19252caf180586cadced5c456a755dd954267688) )
	ROM_LOAD16_BYTE( "2107210u.070", 0x060001, 0x010000, CRC(19bc7495) SHA1(8a93bb8e0998b34c92dad263ea78972155c5b785) )
	ROM_LOAD16_BYTE( "2110200v.070", 0x080000, 0x010000, CRC(4f6d22c5) SHA1(fd28782593444f1607f322a2f1971ba8f3d14131) )
	ROM_LOAD16_BYTE( "2109210v.070", 0x080001, 0x010000, CRC(ac6d4d4a) SHA1(fef902700561bb789ff7462f30a438ee9138b472) )
	ROM_LOAD16_BYTE( "2112200w.070", 0x0a0000, 0x010000, CRC(3f896aaf) SHA1(817136ddc37566108de15f6bfedc6e0da13a2df2) )
	ROM_LOAD16_BYTE( "2111210w.070", 0x0a0001, 0x010000, CRC(47f010ad) SHA1(a2587ce1d01c78f1d757fb3e4512be9655d17f9c) )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU3, 0 )	/* region for ADSP 2100 */

	ROM_REGION( 0x14000, REGION_CPU4, 0 )		/* 64k for 6502 code */
	ROM_LOAD( "2123-10c.070", 0x010000, 0x004000, CRC(121ab09a) SHA1(c26b8ddbcb011416e6ab695980d2cf37e672e973) )
	ROM_CONTINUE(             0x004000, 0x00c000 )

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )	/* 384k for ADSP object ROM */
	ROM_LOAD16_BYTE( "2121-90h.070", 0x000000, 0x010000, CRC(0ebf8e58) SHA1(b6bf3e020b29a34ef3eaca6b5e1f17bb89fdc476) )
	ROM_LOAD16_BYTE( "2118-90k.070", 0x000001, 0x010000, CRC(fb98abaf) SHA1(6a141effee644f34634b57d1fe4c03f56981f966) )
	ROM_LOAD16_BYTE( "2122-10h.070", 0x020000, 0x010000, CRC(bd5380bd) SHA1(e1e2b3c9f9bfc988f0dcc9a9f520f51957e13a97) )
	ROM_LOAD16_BYTE( "2119-10k.070", 0x020001, 0x010000, CRC(bde8bd31) SHA1(efb8878382adfe16ba590a28a949029749fc6a63) )
	ROM_LOAD16_BYTE( "2120-9h.070",  0x040000, 0x010000, CRC(55a30976) SHA1(045a04d3d24e783a6a643cab08e8974ee5dc2128) )
	ROM_LOAD16_BYTE( "2117-9k.070",  0x040001, 0x010000, CRC(d4a9696d) SHA1(574e5f3758ac2e18423ae350e8509aa135ca6da0) )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for ADSP I/O buffers */

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* 256k for ADPCM samples */
	ROM_LOAD( "2124-1fh.070", 0x000000, 0x010000, CRC(4dc14fe8) SHA1(c7cc00715f6687ced9d69ec793d6e9d4bc1b5287) )
	ROM_LOAD( "2125-1ef.070", 0x010000, 0x010000, CRC(cbdabbcc) SHA1(4d102a5677d96e68d27c1960dc3a237ae6751c2f) )
	ROM_LOAD( "2126-1de.070", 0x020000, 0x010000, CRC(b973d9d1) SHA1(a74a3c981497a9c5557f793d49381a9b776cb025) )
	ROM_LOAD( "2127-1cd.070", 0x030000, 0x010000, CRC(3e419f4e) SHA1(e382e047f02591a934a53e5fbf07cccf285abb29) )
ROM_END


ROM_START( stunrunj )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 1MB for 68000 code */
	ROM_LOAD16_BYTE( "3402200r.070", 0x000000, 0x010000, CRC(d8d56923) SHA1(248b22dab791c2d9ee6e6097dac9f3bf85d572be) )
	ROM_LOAD16_BYTE( "3401210r.070", 0x000001, 0x010000, CRC(ce11126c) SHA1(cf27b56103048cc23dc6763763b7419583a27c76) )
	ROM_LOAD16_BYTE( "7404200s.070", 0x020000, 0x010000, CRC(81da3e74) SHA1(38864aac479a8927d168ee977fd0981a7926d0fd) )
	ROM_LOAD16_BYTE( "7403210s.070", 0x020001, 0x010000, CRC(54878b3b) SHA1(42301d10c41ded3741821b52ae191dc46fa417d6) )
	ROM_LOAD16_BYTE( "7406200t.070", 0x040000, 0x010000, CRC(8e3352b9) SHA1(07d2d9b1b96ff21d4ce7f16469451c58229a672c) )
	ROM_LOAD16_BYTE( "7405210t.070", 0x040001, 0x010000, CRC(f40499b1) SHA1(e6d6975da3a1f04460cd6ffb4616a7f27d1af688) )
	ROM_LOAD16_BYTE( "2108200u.070", 0x060000, 0x010000, CRC(0ce849aa) SHA1(19252caf180586cadced5c456a755dd954267688) )
	ROM_LOAD16_BYTE( "2107210u.070", 0x060001, 0x010000, CRC(19bc7495) SHA1(8a93bb8e0998b34c92dad263ea78972155c5b785) )
	ROM_LOAD16_BYTE( "2110200v.070", 0x080000, 0x010000, CRC(4f6d22c5) SHA1(fd28782593444f1607f322a2f1971ba8f3d14131) )
	ROM_LOAD16_BYTE( "2109210v.070", 0x080001, 0x010000, CRC(ac6d4d4a) SHA1(fef902700561bb789ff7462f30a438ee9138b472) )
	ROM_LOAD16_BYTE( "2112200w.070", 0x0a0000, 0x010000, CRC(3f896aaf) SHA1(817136ddc37566108de15f6bfedc6e0da13a2df2) )
	ROM_LOAD16_BYTE( "2111210w.070", 0x0a0001, 0x010000, CRC(47f010ad) SHA1(a2587ce1d01c78f1d757fb3e4512be9655d17f9c) )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU3, 0 )	/* region for ADSP 2100 */

	ROM_REGION( 0x14000, REGION_CPU4, 0 )		/* 64k for 6502 code */
	ROM_LOAD( "2123-10c.070", 0x010000, 0x004000, CRC(121ab09a) SHA1(c26b8ddbcb011416e6ab695980d2cf37e672e973) )
	ROM_CONTINUE(             0x004000, 0x00c000 )

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )	/* 384k for ADSP object ROM */
	ROM_LOAD16_BYTE( "2121-90h.070", 0x000000, 0x010000, CRC(0ebf8e58) SHA1(b6bf3e020b29a34ef3eaca6b5e1f17bb89fdc476) )
	ROM_LOAD16_BYTE( "2118-90k.070", 0x000001, 0x010000, CRC(fb98abaf) SHA1(6a141effee644f34634b57d1fe4c03f56981f966) )
	ROM_LOAD16_BYTE( "2122-10h.070", 0x020000, 0x010000, CRC(bd5380bd) SHA1(e1e2b3c9f9bfc988f0dcc9a9f520f51957e13a97) )
	ROM_LOAD16_BYTE( "2119-10k.070", 0x020001, 0x010000, CRC(bde8bd31) SHA1(efb8878382adfe16ba590a28a949029749fc6a63) )
	ROM_LOAD16_BYTE( "2120-9h.070",  0x040000, 0x010000, CRC(55a30976) SHA1(045a04d3d24e783a6a643cab08e8974ee5dc2128) )
	ROM_LOAD16_BYTE( "2117-9k.070",  0x040001, 0x010000, CRC(d4a9696d) SHA1(574e5f3758ac2e18423ae350e8509aa135ca6da0) )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for ADSP I/O buffers */

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* 256k for ADPCM samples */
	ROM_LOAD( "2124-1fh.070", 0x000000, 0x010000, CRC(4dc14fe8) SHA1(c7cc00715f6687ced9d69ec793d6e9d4bc1b5287) )
	ROM_LOAD( "2125-1ef.070", 0x010000, 0x010000, CRC(cbdabbcc) SHA1(4d102a5677d96e68d27c1960dc3a237ae6751c2f) )
	ROM_LOAD( "2126-1de.070", 0x020000, 0x010000, CRC(b973d9d1) SHA1(a74a3c981497a9c5557f793d49381a9b776cb025) )
	ROM_LOAD( "2127-1cd.070", 0x030000, 0x010000, CRC(3e419f4e) SHA1(e382e047f02591a934a53e5fbf07cccf285abb29) )
ROM_END


ROM_START( stunrun5 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 1MB for 68000 code */
	ROM_LOAD16_BYTE( "2102200r.070", 0x000000, 0x010000, CRC(e0ed54d8) SHA1(15850568d8308b6499cbe55b5d8308041d906a29) )
	ROM_LOAD16_BYTE( "2101210r.070", 0x000001, 0x010000, CRC(3008bcf8) SHA1(9d3a20b639969bab68441f76467ed60e395c10e3) )
	ROM_LOAD16_BYTE( "5104200s.070", 0x020000, 0x010000, CRC(d5c601fe) SHA1(ab61e11723ab9b92fb98b6f8e520a7b323caae64) )
	ROM_LOAD16_BYTE( "5103210s.070", 0x020001, 0x010000, CRC(19d040d6) SHA1(029d6b341a2d7b0d766538395af75ddeec9cca31) )
	ROM_LOAD16_BYTE( "5106200t.070", 0x040000, 0x010000, CRC(9167a032) SHA1(6d5a273ea37e67fe47b5d0428e34302a53332adf) )
	ROM_LOAD16_BYTE( "5105210t.070", 0x040001, 0x010000, CRC(f935d15a) SHA1(6bb102db450da9c5d34d5a293c379343aac6a853) )
	ROM_LOAD16_BYTE( "2108200u.070", 0x060000, 0x010000, CRC(0ce849aa) SHA1(19252caf180586cadced5c456a755dd954267688) )
	ROM_LOAD16_BYTE( "2107210u.070", 0x060001, 0x010000, CRC(19bc7495) SHA1(8a93bb8e0998b34c92dad263ea78972155c5b785) )
	ROM_LOAD16_BYTE( "2110200v.070", 0x080000, 0x010000, CRC(4f6d22c5) SHA1(fd28782593444f1607f322a2f1971ba8f3d14131) )
	ROM_LOAD16_BYTE( "2109210v.070", 0x080001, 0x010000, CRC(ac6d4d4a) SHA1(fef902700561bb789ff7462f30a438ee9138b472) )
	ROM_LOAD16_BYTE( "2112200w.070", 0x0a0000, 0x010000, CRC(3f896aaf) SHA1(817136ddc37566108de15f6bfedc6e0da13a2df2) )
	ROM_LOAD16_BYTE( "2111210w.070", 0x0a0001, 0x010000, CRC(47f010ad) SHA1(a2587ce1d01c78f1d757fb3e4512be9655d17f9c) )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU3, 0 )	/* region for ADSP 2100 */

	ROM_REGION( 0x14000, REGION_CPU4, 0 )		/* 64k for 6502 code */
	ROM_LOAD( "2123-10c.070", 0x010000, 0x004000, CRC(121ab09a) SHA1(c26b8ddbcb011416e6ab695980d2cf37e672e973) )
	ROM_CONTINUE(             0x004000, 0x00c000 )

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )	/* 384k for ADSP object ROM */
	ROM_LOAD16_BYTE( "2121-90h.070", 0x000000, 0x010000, CRC(0ebf8e58) SHA1(b6bf3e020b29a34ef3eaca6b5e1f17bb89fdc476) )
	ROM_LOAD16_BYTE( "2118-90k.070", 0x000001, 0x010000, CRC(fb98abaf) SHA1(6a141effee644f34634b57d1fe4c03f56981f966) )
	ROM_LOAD16_BYTE( "2122-10h.070", 0x020000, 0x010000, CRC(bd5380bd) SHA1(e1e2b3c9f9bfc988f0dcc9a9f520f51957e13a97) )
	ROM_LOAD16_BYTE( "2119-10k.070", 0x020001, 0x010000, CRC(bde8bd31) SHA1(efb8878382adfe16ba590a28a949029749fc6a63) )
	ROM_LOAD16_BYTE( "2120-9h.070",  0x040000, 0x010000, CRC(55a30976) SHA1(045a04d3d24e783a6a643cab08e8974ee5dc2128) )
	ROM_LOAD16_BYTE( "2117-9k.070",  0x040001, 0x010000, CRC(d4a9696d) SHA1(574e5f3758ac2e18423ae350e8509aa135ca6da0) )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for ADSP I/O buffers */

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* 256k for ADPCM samples */
	ROM_LOAD( "2124-1fh.070", 0x000000, 0x010000, CRC(4dc14fe8) SHA1(c7cc00715f6687ced9d69ec793d6e9d4bc1b5287) )
	ROM_LOAD( "2125-1ef.070", 0x010000, 0x010000, CRC(cbdabbcc) SHA1(4d102a5677d96e68d27c1960dc3a237ae6751c2f) )
	ROM_LOAD( "2126-1de.070", 0x020000, 0x010000, CRC(b973d9d1) SHA1(a74a3c981497a9c5557f793d49381a9b776cb025) )
	ROM_LOAD( "2127-1cd.070", 0x030000, 0x010000, CRC(3e419f4e) SHA1(e382e047f02591a934a53e5fbf07cccf285abb29) )
ROM_END


ROM_START( stunrune )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 1MB for 68000 code */
	ROM_LOAD16_BYTE( "2102200r.070", 0x000000, 0x010000, CRC(e0ed54d8) SHA1(15850568d8308b6499cbe55b5d8308041d906a29) )
	ROM_LOAD16_BYTE( "2101210r.070", 0x000001, 0x010000, CRC(3008bcf8) SHA1(9d3a20b639969bab68441f76467ed60e395c10e3) )
	ROM_LOAD16_BYTE( "5004200s.070", 0x020000, 0x010000, CRC(c2e4bde8) SHA1(766242455b0be6dc7b698cc6fe86d90991ca2e0a) )
	ROM_LOAD16_BYTE( "5003210s.070", 0x020001, 0x010000, CRC(0c902d21) SHA1(e56efc34a746718a844558c2df65fd560834c26e) )
	ROM_LOAD16_BYTE( "5006200t.070", 0x040000, 0x010000, CRC(b0fc2fcc) SHA1(780d5358afcbfdc02226614c9a6b29c79f4cce69) )
	ROM_LOAD16_BYTE( "5005210t.070", 0x040001, 0x010000, CRC(44f290e3) SHA1(cd96c9058ccb8084ab6761528d20528ab2b987a2) )
	ROM_LOAD16_BYTE( "2108200u.070", 0x060000, 0x010000, CRC(0ce849aa) SHA1(19252caf180586cadced5c456a755dd954267688) )
	ROM_LOAD16_BYTE( "2107210u.070", 0x060001, 0x010000, CRC(19bc7495) SHA1(8a93bb8e0998b34c92dad263ea78972155c5b785) )
	ROM_LOAD16_BYTE( "2110200v.070", 0x080000, 0x010000, CRC(4f6d22c5) SHA1(fd28782593444f1607f322a2f1971ba8f3d14131) )
	ROM_LOAD16_BYTE( "2109210v.070", 0x080001, 0x010000, CRC(ac6d4d4a) SHA1(fef902700561bb789ff7462f30a438ee9138b472) )
	ROM_LOAD16_BYTE( "2112200w.070", 0x0a0000, 0x010000, CRC(3f896aaf) SHA1(817136ddc37566108de15f6bfedc6e0da13a2df2) )
	ROM_LOAD16_BYTE( "2111210w.070", 0x0a0001, 0x010000, CRC(47f010ad) SHA1(a2587ce1d01c78f1d757fb3e4512be9655d17f9c) )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU3, 0 )	/* region for ADSP 2100 */

	ROM_REGION( 0x14000, REGION_CPU4, 0 )		/* 64k for 6502 code */
	ROM_LOAD( "2123-10c.070", 0x010000, 0x004000, CRC(121ab09a) SHA1(c26b8ddbcb011416e6ab695980d2cf37e672e973) )
	ROM_CONTINUE(             0x004000, 0x00c000 )

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )	/* 384k for ADSP object ROM */
	ROM_LOAD16_BYTE( "2121-90h.070", 0x000000, 0x010000, CRC(0ebf8e58) SHA1(b6bf3e020b29a34ef3eaca6b5e1f17bb89fdc476) )
	ROM_LOAD16_BYTE( "2118-90k.070", 0x000001, 0x010000, CRC(fb98abaf) SHA1(6a141effee644f34634b57d1fe4c03f56981f966) )
	ROM_LOAD16_BYTE( "2122-10h.070", 0x020000, 0x010000, CRC(bd5380bd) SHA1(e1e2b3c9f9bfc988f0dcc9a9f520f51957e13a97) )
	ROM_LOAD16_BYTE( "2119-10k.070", 0x020001, 0x010000, CRC(bde8bd31) SHA1(efb8878382adfe16ba590a28a949029749fc6a63) )
	ROM_LOAD16_BYTE( "2120-9h.070",  0x040000, 0x010000, CRC(55a30976) SHA1(045a04d3d24e783a6a643cab08e8974ee5dc2128) )
	ROM_LOAD16_BYTE( "2117-9k.070",  0x040001, 0x010000, CRC(d4a9696d) SHA1(574e5f3758ac2e18423ae350e8509aa135ca6da0) )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for ADSP I/O buffers */

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* 256k for ADPCM samples */
	ROM_LOAD( "2124-1fh.070", 0x000000, 0x010000, CRC(4dc14fe8) SHA1(c7cc00715f6687ced9d69ec793d6e9d4bc1b5287) )
	ROM_LOAD( "2125-1ef.070", 0x010000, 0x010000, CRC(cbdabbcc) SHA1(4d102a5677d96e68d27c1960dc3a237ae6751c2f) )
	ROM_LOAD( "2126-1de.070", 0x020000, 0x010000, CRC(b973d9d1) SHA1(a74a3c981497a9c5557f793d49381a9b776cb025) )
	ROM_LOAD( "2127-1cd.070", 0x030000, 0x010000, CRC(3e419f4e) SHA1(e382e047f02591a934a53e5fbf07cccf285abb29) )
ROM_END


ROM_START( stunrun4 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 1MB for 68000 code */
	ROM_LOAD16_BYTE( "2102200r.070", 0x000000, 0x010000, CRC(e0ed54d8) SHA1(15850568d8308b6499cbe55b5d8308041d906a29) )
	ROM_LOAD16_BYTE( "2101210r.070", 0x000001, 0x010000, CRC(3008bcf8) SHA1(9d3a20b639969bab68441f76467ed60e395c10e3) )
	ROM_LOAD16_BYTE( "4104200s.070", 0x020000, 0x010000, CRC(62c7700b) SHA1(31d7a39df7353e60b806c204bfdd75c06b17c984) )
	ROM_LOAD16_BYTE( "4103210s.070", 0x020001, 0x010000, CRC(f379fcb9) SHA1(20a1b6775cc517ca159f5d233db3e3e864198125) )
	ROM_LOAD16_BYTE( "4106200t.070", 0x040000, 0x010000, CRC(4346669c) SHA1(13c19bbb9235879829d96fd4e227b725d13bb869) )
	ROM_LOAD16_BYTE( "4105210t.070", 0x040001, 0x010000, CRC(54d823dd) SHA1(5f9702b7a593009703a94987d56bc8ab3b72997d) )
	ROM_LOAD16_BYTE( "2108200u.070", 0x060000, 0x010000, CRC(0ce849aa) SHA1(19252caf180586cadced5c456a755dd954267688) )
	ROM_LOAD16_BYTE( "2107210u.070", 0x060001, 0x010000, CRC(19bc7495) SHA1(8a93bb8e0998b34c92dad263ea78972155c5b785) )
	ROM_LOAD16_BYTE( "2110200v.070", 0x080000, 0x010000, CRC(4f6d22c5) SHA1(fd28782593444f1607f322a2f1971ba8f3d14131) )
	ROM_LOAD16_BYTE( "2109210v.070", 0x080001, 0x010000, CRC(ac6d4d4a) SHA1(fef902700561bb789ff7462f30a438ee9138b472) )
	ROM_LOAD16_BYTE( "2112200w.070", 0x0a0000, 0x010000, CRC(3f896aaf) SHA1(817136ddc37566108de15f6bfedc6e0da13a2df2) )
	ROM_LOAD16_BYTE( "2111210w.070", 0x0a0001, 0x010000, CRC(47f010ad) SHA1(a2587ce1d01c78f1d757fb3e4512be9655d17f9c) )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU3, 0 )	/* region for ADSP 2100 */

	ROM_REGION( 0x14000, REGION_CPU4, 0 )		/* 64k for 6502 code */
	ROM_LOAD( "2123-10c.070", 0x010000, 0x004000, CRC(121ab09a) SHA1(c26b8ddbcb011416e6ab695980d2cf37e672e973) )
	ROM_CONTINUE(             0x004000, 0x00c000 )

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )	/* 384k for ADSP object ROM */
	ROM_LOAD16_BYTE( "2121-90h.070", 0x000000, 0x010000, CRC(0ebf8e58) SHA1(b6bf3e020b29a34ef3eaca6b5e1f17bb89fdc476) )
	ROM_LOAD16_BYTE( "2118-90k.070", 0x000001, 0x010000, CRC(fb98abaf) SHA1(6a141effee644f34634b57d1fe4c03f56981f966) )
	ROM_LOAD16_BYTE( "2122-10h.070", 0x020000, 0x010000, CRC(bd5380bd) SHA1(e1e2b3c9f9bfc988f0dcc9a9f520f51957e13a97) )
	ROM_LOAD16_BYTE( "2119-10k.070", 0x020001, 0x010000, CRC(bde8bd31) SHA1(efb8878382adfe16ba590a28a949029749fc6a63) )
	ROM_LOAD16_BYTE( "2120-9h.070",  0x040000, 0x010000, CRC(55a30976) SHA1(045a04d3d24e783a6a643cab08e8974ee5dc2128) )
	ROM_LOAD16_BYTE( "2117-9k.070",  0x040001, 0x010000, CRC(d4a9696d) SHA1(574e5f3758ac2e18423ae350e8509aa135ca6da0) )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for ADSP I/O buffers */

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* 256k for ADPCM samples */
	ROM_LOAD( "2124-1fh.070", 0x000000, 0x010000, CRC(4dc14fe8) SHA1(c7cc00715f6687ced9d69ec793d6e9d4bc1b5287) )
	ROM_LOAD( "2125-1ef.070", 0x010000, 0x010000, CRC(cbdabbcc) SHA1(4d102a5677d96e68d27c1960dc3a237ae6751c2f) )
	ROM_LOAD( "2126-1de.070", 0x020000, 0x010000, CRC(b973d9d1) SHA1(a74a3c981497a9c5557f793d49381a9b776cb025) )
	ROM_LOAD( "2127-1cd.070", 0x030000, 0x010000, CRC(3e419f4e) SHA1(e382e047f02591a934a53e5fbf07cccf285abb29) )
ROM_END


ROM_START( stunrun3 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 1MB for 68000 code */
	ROM_LOAD16_BYTE( "2102200r.070", 0x000000, 0x010000, CRC(e0ed54d8) SHA1(15850568d8308b6499cbe55b5d8308041d906a29) )
	ROM_LOAD16_BYTE( "2101210r.070", 0x000001, 0x010000, CRC(3008bcf8) SHA1(9d3a20b639969bab68441f76467ed60e395c10e3) )
	ROM_LOAD16_BYTE( "2104200s.070", 0x020000, 0x010000, CRC(6acdeeaa) SHA1(a4cbe648ad2fee3bb945fbc8055b76be1f5c03d1) )
	ROM_LOAD16_BYTE( "3103210s.070", 0x020001, 0x010000, CRC(e8b1262a) SHA1(a304602023ffa8598dee8ec44f972dc8f1dad1b6) )
	ROM_LOAD16_BYTE( "3106200t.070", 0x040000, 0x010000, CRC(41c4778c) SHA1(f453adca7d864e0e030db36500ca072bfa935703) )
	ROM_LOAD16_BYTE( "3105210t.070", 0x040001, 0x010000, CRC(0d6c9b8f) SHA1(6e7e664ff5c19fdeaa4d82a02be9d74cea025fff) )
	ROM_LOAD16_BYTE( "2108200u.070", 0x060000, 0x010000, CRC(0ce849aa) SHA1(19252caf180586cadced5c456a755dd954267688) )
	ROM_LOAD16_BYTE( "2107210u.070", 0x060001, 0x010000, CRC(19bc7495) SHA1(8a93bb8e0998b34c92dad263ea78972155c5b785) )
	ROM_LOAD16_BYTE( "2110200v.070", 0x080000, 0x010000, CRC(4f6d22c5) SHA1(fd28782593444f1607f322a2f1971ba8f3d14131) )
	ROM_LOAD16_BYTE( "2109210v.070", 0x080001, 0x010000, CRC(ac6d4d4a) SHA1(fef902700561bb789ff7462f30a438ee9138b472) )
	ROM_LOAD16_BYTE( "2112200w.070", 0x0a0000, 0x010000, CRC(3f896aaf) SHA1(817136ddc37566108de15f6bfedc6e0da13a2df2) )
	ROM_LOAD16_BYTE( "2111210w.070", 0x0a0001, 0x010000, CRC(47f010ad) SHA1(a2587ce1d01c78f1d757fb3e4512be9655d17f9c) )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU3, 0 )	/* region for ADSP 2100 */

	ROM_REGION( 0x14000, REGION_CPU4, 0 )		/* 64k for 6502 code */
	ROM_LOAD( "2123-10c.070", 0x010000, 0x004000, CRC(121ab09a) SHA1(c26b8ddbcb011416e6ab695980d2cf37e672e973) )
	ROM_CONTINUE(             0x004000, 0x00c000 )

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )	/* 384k for ADSP object ROM */
	ROM_LOAD16_BYTE( "2121-90h.070", 0x000000, 0x010000, CRC(0ebf8e58) SHA1(b6bf3e020b29a34ef3eaca6b5e1f17bb89fdc476) )
	ROM_LOAD16_BYTE( "2118-90k.070", 0x000001, 0x010000, CRC(fb98abaf) SHA1(6a141effee644f34634b57d1fe4c03f56981f966) )
	ROM_LOAD16_BYTE( "2122-10h.070", 0x020000, 0x010000, CRC(bd5380bd) SHA1(e1e2b3c9f9bfc988f0dcc9a9f520f51957e13a97) )
	ROM_LOAD16_BYTE( "2119-10k.070", 0x020001, 0x010000, CRC(bde8bd31) SHA1(efb8878382adfe16ba590a28a949029749fc6a63) )
	ROM_LOAD16_BYTE( "2120-9h.070",  0x040000, 0x010000, CRC(55a30976) SHA1(045a04d3d24e783a6a643cab08e8974ee5dc2128) )
	ROM_LOAD16_BYTE( "2117-9k.070",  0x040001, 0x010000, CRC(d4a9696d) SHA1(574e5f3758ac2e18423ae350e8509aa135ca6da0) )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for ADSP I/O buffers */

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* 256k for ADPCM samples */
	ROM_LOAD( "2124-1fh.070", 0x000000, 0x010000, CRC(4dc14fe8) SHA1(c7cc00715f6687ced9d69ec793d6e9d4bc1b5287) )
	ROM_LOAD( "2125-1ef.070", 0x010000, 0x010000, CRC(cbdabbcc) SHA1(4d102a5677d96e68d27c1960dc3a237ae6751c2f) )
	ROM_LOAD( "2126-1de.070", 0x020000, 0x010000, CRC(b973d9d1) SHA1(a74a3c981497a9c5557f793d49381a9b776cb025) )
	ROM_LOAD( "2127-1cd.070", 0x030000, 0x010000, CRC(3e419f4e) SHA1(e382e047f02591a934a53e5fbf07cccf285abb29) )
ROM_END


ROM_START( stunrn3e )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 1MB for 68000 code */
	ROM_LOAD16_BYTE( "2102200r.070", 0x000000, 0x010000, CRC(e0ed54d8) SHA1(15850568d8308b6499cbe55b5d8308041d906a29) )
	ROM_LOAD16_BYTE( "2101210r.070", 0x000001, 0x010000, CRC(3008bcf8) SHA1(9d3a20b639969bab68441f76467ed60e395c10e3) )
	ROM_LOAD16_BYTE( "3004200s.070", 0x020000, 0x010000, CRC(d9474678) SHA1(090b78f9e9d2ecf4218821fc102861df1e27d95a) )
	ROM_LOAD16_BYTE( "3003210s.070", 0x020001, 0x010000, CRC(3155287d) SHA1(6a612ab0bcbf425ee5d6a2e5da81ca33f53b3889) )
	ROM_LOAD16_BYTE( "3006200t.070", 0x040000, 0x010000, CRC(617db84c) SHA1(ca4bbb1ecb528980cf98aaaad06636ce3c16eba3) )
	ROM_LOAD16_BYTE( "3005210t.070", 0x040001, 0x010000, CRC(a9cd8fcb) SHA1(50a3ef321a3cb381df96aef9c8e180e23e442c3a) )
	ROM_LOAD16_BYTE( "2108200u.070", 0x060000, 0x010000, CRC(0ce849aa) SHA1(19252caf180586cadced5c456a755dd954267688) )
	ROM_LOAD16_BYTE( "2107210u.070", 0x060001, 0x010000, CRC(19bc7495) SHA1(8a93bb8e0998b34c92dad263ea78972155c5b785) )
	ROM_LOAD16_BYTE( "2110200v.070", 0x080000, 0x010000, CRC(4f6d22c5) SHA1(fd28782593444f1607f322a2f1971ba8f3d14131) )
	ROM_LOAD16_BYTE( "2109210v.070", 0x080001, 0x010000, CRC(ac6d4d4a) SHA1(fef902700561bb789ff7462f30a438ee9138b472) )
	ROM_LOAD16_BYTE( "2112200w.070", 0x0a0000, 0x010000, CRC(3f896aaf) SHA1(817136ddc37566108de15f6bfedc6e0da13a2df2) )
	ROM_LOAD16_BYTE( "2111210w.070", 0x0a0001, 0x010000, CRC(47f010ad) SHA1(a2587ce1d01c78f1d757fb3e4512be9655d17f9c) )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU3, 0 )	/* region for ADSP 2100 */

	ROM_REGION( 0x14000, REGION_CPU4, 0 )		/* 64k for 6502 code */
	ROM_LOAD( "2123-10c.070", 0x010000, 0x004000, CRC(121ab09a) SHA1(c26b8ddbcb011416e6ab695980d2cf37e672e973) )
	ROM_CONTINUE(             0x004000, 0x00c000 )

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )	/* 384k for ADSP object ROM */
	ROM_LOAD16_BYTE( "2121-90h.070", 0x000000, 0x010000, CRC(0ebf8e58) SHA1(b6bf3e020b29a34ef3eaca6b5e1f17bb89fdc476) )
	ROM_LOAD16_BYTE( "2118-90k.070", 0x000001, 0x010000, CRC(fb98abaf) SHA1(6a141effee644f34634b57d1fe4c03f56981f966) )
	ROM_LOAD16_BYTE( "2122-10h.070", 0x020000, 0x010000, CRC(bd5380bd) SHA1(e1e2b3c9f9bfc988f0dcc9a9f520f51957e13a97) )
	ROM_LOAD16_BYTE( "2119-10k.070", 0x020001, 0x010000, CRC(bde8bd31) SHA1(efb8878382adfe16ba590a28a949029749fc6a63) )
	ROM_LOAD16_BYTE( "2120-9h.070",  0x040000, 0x010000, CRC(55a30976) SHA1(045a04d3d24e783a6a643cab08e8974ee5dc2128) )
	ROM_LOAD16_BYTE( "2117-9k.070",  0x040001, 0x010000, CRC(d4a9696d) SHA1(574e5f3758ac2e18423ae350e8509aa135ca6da0) )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for ADSP I/O buffers */

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* 256k for ADPCM samples */
	ROM_LOAD( "2124-1fh.070", 0x000000, 0x010000, CRC(4dc14fe8) SHA1(c7cc00715f6687ced9d69ec793d6e9d4bc1b5287) )
	ROM_LOAD( "2125-1ef.070", 0x010000, 0x010000, CRC(cbdabbcc) SHA1(4d102a5677d96e68d27c1960dc3a237ae6751c2f) )
	ROM_LOAD( "2126-1de.070", 0x020000, 0x010000, CRC(b973d9d1) SHA1(a74a3c981497a9c5557f793d49381a9b776cb025) )
	ROM_LOAD( "2127-1cd.070", 0x030000, 0x010000, CRC(3e419f4e) SHA1(e382e047f02591a934a53e5fbf07cccf285abb29) )
ROM_END


ROM_START( stunrun2 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 1MB for 68000 code */
	ROM_LOAD16_BYTE( "2102200r.070", 0x000000, 0x010000, CRC(e0ed54d8) SHA1(15850568d8308b6499cbe55b5d8308041d906a29) )
	ROM_LOAD16_BYTE( "2101210r.070", 0x000001, 0x010000, CRC(3008bcf8) SHA1(9d3a20b639969bab68441f76467ed60e395c10e3) )
	ROM_LOAD16_BYTE( "2104200s.070", 0x020000, 0x010000, CRC(6acdeeaa) SHA1(a4cbe648ad2fee3bb945fbc8055b76be1f5c03d1) )
	ROM_LOAD16_BYTE( "2103210s.070", 0x020001, 0x010000, CRC(75e08b1d) SHA1(42d78d65697117829a6ea3c6db589ad35a2d306d) )
	ROM_LOAD16_BYTE( "2106200t.070", 0x040000, 0x010000, CRC(ae8fa7ca) SHA1(a86de221d79d295bff3ba2026e45124fde340eac) )
	ROM_LOAD16_BYTE( "2105210t.070", 0x040001, 0x010000, CRC(22d6b977) SHA1(916eff6557cd20c708408bff77a2536122ada9c2) )
	ROM_LOAD16_BYTE( "2108200u.070", 0x060000, 0x010000, CRC(0ce849aa) SHA1(19252caf180586cadced5c456a755dd954267688) )
	ROM_LOAD16_BYTE( "2107210u.070", 0x060001, 0x010000, CRC(19bc7495) SHA1(8a93bb8e0998b34c92dad263ea78972155c5b785) )
	ROM_LOAD16_BYTE( "2110200v.070", 0x080000, 0x010000, CRC(4f6d22c5) SHA1(fd28782593444f1607f322a2f1971ba8f3d14131) )
	ROM_LOAD16_BYTE( "2109210v.070", 0x080001, 0x010000, CRC(ac6d4d4a) SHA1(fef902700561bb789ff7462f30a438ee9138b472) )
	ROM_LOAD16_BYTE( "2112200w.070", 0x0a0000, 0x010000, CRC(3f896aaf) SHA1(817136ddc37566108de15f6bfedc6e0da13a2df2) )
	ROM_LOAD16_BYTE( "2111210w.070", 0x0a0001, 0x010000, CRC(47f010ad) SHA1(a2587ce1d01c78f1d757fb3e4512be9655d17f9c) )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU3, 0 )	/* region for ADSP 2100 */

	ROM_REGION( 0x14000, REGION_CPU4, 0 )		/* 64k for 6502 code */
	ROM_LOAD( "2123-10c.070", 0x010000, 0x004000, CRC(121ab09a) SHA1(c26b8ddbcb011416e6ab695980d2cf37e672e973) )
	ROM_CONTINUE(             0x004000, 0x00c000 )

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )	/* 384k for ADSP object ROM */
	ROM_LOAD16_BYTE( "2121-90h.070", 0x000000, 0x010000, CRC(0ebf8e58) SHA1(b6bf3e020b29a34ef3eaca6b5e1f17bb89fdc476) )
	ROM_LOAD16_BYTE( "2118-90k.070", 0x000001, 0x010000, CRC(fb98abaf) SHA1(6a141effee644f34634b57d1fe4c03f56981f966) )
	ROM_LOAD16_BYTE( "2122-10h.070", 0x020000, 0x010000, CRC(bd5380bd) SHA1(e1e2b3c9f9bfc988f0dcc9a9f520f51957e13a97) )
	ROM_LOAD16_BYTE( "2119-10k.070", 0x020001, 0x010000, CRC(bde8bd31) SHA1(efb8878382adfe16ba590a28a949029749fc6a63) )
	ROM_LOAD16_BYTE( "2120-9h.070",  0x040000, 0x010000, CRC(55a30976) SHA1(045a04d3d24e783a6a643cab08e8974ee5dc2128) )
	ROM_LOAD16_BYTE( "2117-9k.070",  0x040001, 0x010000, CRC(d4a9696d) SHA1(574e5f3758ac2e18423ae350e8509aa135ca6da0) )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for ADSP I/O buffers */

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* 256k for ADPCM samples */
	ROM_LOAD( "2124-1fh.070", 0x000000, 0x010000, CRC(4dc14fe8) SHA1(c7cc00715f6687ced9d69ec793d6e9d4bc1b5287) )
	ROM_LOAD( "2125-1ef.070", 0x010000, 0x010000, CRC(cbdabbcc) SHA1(4d102a5677d96e68d27c1960dc3a237ae6751c2f) )
	ROM_LOAD( "2126-1de.070", 0x020000, 0x010000, CRC(b973d9d1) SHA1(a74a3c981497a9c5557f793d49381a9b776cb025) )
	ROM_LOAD( "2127-1cd.070", 0x030000, 0x010000, CRC(3e419f4e) SHA1(e382e047f02591a934a53e5fbf07cccf285abb29) )
ROM_END


ROM_START( stunrn2e )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 1MB for 68000 code */
	ROM_LOAD16_BYTE( "2102200r.070", 0x000000, 0x010000, CRC(e0ed54d8) SHA1(15850568d8308b6499cbe55b5d8308041d906a29) )
	ROM_LOAD16_BYTE( "2101210r.070", 0x000001, 0x010000, CRC(3008bcf8) SHA1(9d3a20b639969bab68441f76467ed60e395c10e3) )
	ROM_LOAD16_BYTE( "2004200s.070", 0x020000, 0x010000, CRC(30303326) SHA1(49a279b5b02af97395dfb8c66410319a807aa4b9) )
	ROM_LOAD16_BYTE( "2003210s.070", 0x020001, 0x010000, CRC(f29b54c1) SHA1(6339283ecc22cdad0ef6b9142f8d083e3a57b232) )
	ROM_LOAD16_BYTE( "2006200t.070", 0x040000, 0x010000, CRC(812bd4f7) SHA1(af87c137883dfecc97a96858b8e685993b8526e3) )
	ROM_LOAD16_BYTE( "2005210t.070", 0x040001, 0x010000, CRC(4aab737f) SHA1(6df23b6a43527b9d27d95393b3f5e0fb90002e1e) )
	ROM_LOAD16_BYTE( "2108200u.070", 0x060000, 0x010000, CRC(0ce849aa) SHA1(19252caf180586cadced5c456a755dd954267688) )
	ROM_LOAD16_BYTE( "2107210u.070", 0x060001, 0x010000, CRC(19bc7495) SHA1(8a93bb8e0998b34c92dad263ea78972155c5b785) )
	ROM_LOAD16_BYTE( "2110200v.070", 0x080000, 0x010000, CRC(4f6d22c5) SHA1(fd28782593444f1607f322a2f1971ba8f3d14131) )
	ROM_LOAD16_BYTE( "2109210v.070", 0x080001, 0x010000, CRC(ac6d4d4a) SHA1(fef902700561bb789ff7462f30a438ee9138b472) )
	ROM_LOAD16_BYTE( "2112200w.070", 0x0a0000, 0x010000, CRC(3f896aaf) SHA1(817136ddc37566108de15f6bfedc6e0da13a2df2) )
	ROM_LOAD16_BYTE( "2111210w.070", 0x0a0001, 0x010000, CRC(47f010ad) SHA1(a2587ce1d01c78f1d757fb3e4512be9655d17f9c) )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU3, 0 )	/* region for ADSP 2100 */

	ROM_REGION( 0x14000, REGION_CPU4, 0 )		/* 64k for 6502 code */
	ROM_LOAD( "2123-10c.070", 0x010000, 0x004000, CRC(121ab09a) SHA1(c26b8ddbcb011416e6ab695980d2cf37e672e973) )
	ROM_CONTINUE(             0x004000, 0x00c000 )

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )	/* 384k for ADSP object ROM */
	ROM_LOAD16_BYTE( "2121-90h.070", 0x000000, 0x010000, CRC(0ebf8e58) SHA1(b6bf3e020b29a34ef3eaca6b5e1f17bb89fdc476) )
	ROM_LOAD16_BYTE( "2118-90k.070", 0x000001, 0x010000, CRC(fb98abaf) SHA1(6a141effee644f34634b57d1fe4c03f56981f966) )
	ROM_LOAD16_BYTE( "2122-10h.070", 0x020000, 0x010000, CRC(bd5380bd) SHA1(e1e2b3c9f9bfc988f0dcc9a9f520f51957e13a97) )
	ROM_LOAD16_BYTE( "2119-10k.070", 0x020001, 0x010000, CRC(bde8bd31) SHA1(efb8878382adfe16ba590a28a949029749fc6a63) )
	ROM_LOAD16_BYTE( "2120-9h.070",  0x040000, 0x010000, CRC(55a30976) SHA1(045a04d3d24e783a6a643cab08e8974ee5dc2128) )
	ROM_LOAD16_BYTE( "2117-9k.070",  0x040001, 0x010000, CRC(d4a9696d) SHA1(574e5f3758ac2e18423ae350e8509aa135ca6da0) )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for ADSP I/O buffers */

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* 256k for ADPCM samples */
	ROM_LOAD( "2124-1fh.070", 0x000000, 0x010000, CRC(4dc14fe8) SHA1(c7cc00715f6687ced9d69ec793d6e9d4bc1b5287) )
	ROM_LOAD( "2125-1ef.070", 0x010000, 0x010000, CRC(cbdabbcc) SHA1(4d102a5677d96e68d27c1960dc3a237ae6751c2f) )
	ROM_LOAD( "2126-1de.070", 0x020000, 0x010000, CRC(b973d9d1) SHA1(a74a3c981497a9c5557f793d49381a9b776cb025) )
	ROM_LOAD( "2127-1cd.070", 0x030000, 0x010000, CRC(3e419f4e) SHA1(e382e047f02591a934a53e5fbf07cccf285abb29) )
ROM_END


ROM_START( stunrun0 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 1MB for 68000 code */
	ROM_LOAD16_BYTE( "2102200r.070", 0x000000, 0x010000, CRC(e0ed54d8) SHA1(15850568d8308b6499cbe55b5d8308041d906a29) )
	ROM_LOAD16_BYTE( "2101210r.070", 0x000001, 0x010000, CRC(3008bcf8) SHA1(9d3a20b639969bab68441f76467ed60e395c10e3) )
	ROM_LOAD16_BYTE( "0304200s.070", 0x020000, 0x010000, CRC(e7968b1b) SHA1(f2e5b4f64e553041abbe2610968f59cca425a4d4) )
	ROM_LOAD16_BYTE( "0303210s.070", 0x020001, 0x010000, CRC(315209b6) SHA1(fab76c8f8a72895ded7ec4662cafc68379fea58c) )
	ROM_LOAD16_BYTE( "0306200t.070", 0x040000, 0x010000, CRC(d46669df) SHA1(b03e4f230d7acdbc9ef794b027a0a9a4c5090c46) )
	ROM_LOAD16_BYTE( "0305210t.070", 0x040001, 0x010000, CRC(bf35eb46) SHA1(03dae6c794ee057faa4aafba0c9dc92b00e66a13) )
	ROM_LOAD16_BYTE( "2108200u.070", 0x060000, 0x010000, CRC(0ce849aa) SHA1(19252caf180586cadced5c456a755dd954267688) )
	ROM_LOAD16_BYTE( "2107210u.070", 0x060001, 0x010000, CRC(19bc7495) SHA1(8a93bb8e0998b34c92dad263ea78972155c5b785) )
	ROM_LOAD16_BYTE( "2110200v.070", 0x080000, 0x010000, CRC(4f6d22c5) SHA1(fd28782593444f1607f322a2f1971ba8f3d14131) )
	ROM_LOAD16_BYTE( "2109210v.070", 0x080001, 0x010000, CRC(ac6d4d4a) SHA1(fef902700561bb789ff7462f30a438ee9138b472) )
	ROM_LOAD16_BYTE( "2112200w.070", 0x0a0000, 0x010000, CRC(3f896aaf) SHA1(817136ddc37566108de15f6bfedc6e0da13a2df2) )
	ROM_LOAD16_BYTE( "2111210w.070", 0x0a0001, 0x010000, CRC(47f010ad) SHA1(a2587ce1d01c78f1d757fb3e4512be9655d17f9c) )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU3, 0 )	/* region for ADSP 2100 */

	ROM_REGION( 0x14000, REGION_CPU4, 0 )		/* 64k for 6502 code */
	ROM_LOAD( "2123-10c.070", 0x010000, 0x004000, CRC(121ab09a) SHA1(c26b8ddbcb011416e6ab695980d2cf37e672e973) )
	ROM_CONTINUE(             0x004000, 0x00c000 )

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )	/* 384k for ADSP object ROM */
	ROM_LOAD16_BYTE( "2121-90h.070", 0x000000, 0x010000, CRC(0ebf8e58) SHA1(b6bf3e020b29a34ef3eaca6b5e1f17bb89fdc476) )
	ROM_LOAD16_BYTE( "2118-90k.070", 0x000001, 0x010000, CRC(fb98abaf) SHA1(6a141effee644f34634b57d1fe4c03f56981f966) )
	ROM_LOAD16_BYTE( "2122-10h.070", 0x020000, 0x010000, CRC(bd5380bd) SHA1(e1e2b3c9f9bfc988f0dcc9a9f520f51957e13a97) )
	ROM_LOAD16_BYTE( "2119-10k.070", 0x020001, 0x010000, CRC(bde8bd31) SHA1(efb8878382adfe16ba590a28a949029749fc6a63) )
	ROM_LOAD16_BYTE( "2120-9h.070",  0x040000, 0x010000, CRC(55a30976) SHA1(045a04d3d24e783a6a643cab08e8974ee5dc2128) )
	ROM_LOAD16_BYTE( "2117-9k.070",  0x040001, 0x010000, CRC(d4a9696d) SHA1(574e5f3758ac2e18423ae350e8509aa135ca6da0) )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for ADSP I/O buffers */

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* 256k for ADPCM samples */
	ROM_LOAD( "2124-1fh.070", 0x000000, 0x010000, CRC(4dc14fe8) SHA1(c7cc00715f6687ced9d69ec793d6e9d4bc1b5287) )
	ROM_LOAD( "2125-1ef.070", 0x010000, 0x010000, CRC(cbdabbcc) SHA1(4d102a5677d96e68d27c1960dc3a237ae6751c2f) )
	ROM_LOAD( "2126-1de.070", 0x020000, 0x010000, CRC(b973d9d1) SHA1(a74a3c981497a9c5557f793d49381a9b776cb025) )
	ROM_LOAD( "2127-1cd.070", 0x030000, 0x010000, CRC(3e419f4e) SHA1(e382e047f02591a934a53e5fbf07cccf285abb29) )
ROM_END


ROM_START( stunrunp )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 1MB for 68000 code */
	ROM_LOAD16_BYTE( "2102200r.070", 0x000000, 0x010000, CRC(e0ed54d8) SHA1(15850568d8308b6499cbe55b5d8308041d906a29) )
	ROM_LOAD16_BYTE( "2101210r.070", 0x000001, 0x010000, CRC(3008bcf8) SHA1(9d3a20b639969bab68441f76467ed60e395c10e3) )
	ROM_LOAD16_BYTE( "prog-hi0.s20", 0x020000, 0x010000, CRC(0be15a99) SHA1(52b152b23af305e95765c72052bb7aba846510d6) )
	ROM_LOAD16_BYTE( "prog-lo0.s21", 0x020001, 0x010000, CRC(757c0840) SHA1(aaad808cef825d9690667b47eba8920443906fbe) )
	ROM_LOAD16_BYTE( "prog-hi.t20",  0x040000, 0x010000, CRC(49bcde9d) SHA1(d3276b1be4a7dd5e46aaecf793fd239ca4a646b7) )
	ROM_LOAD16_BYTE( "prog-lo1.t21", 0x040001, 0x010000, CRC(3bdafd89) SHA1(3934cf38445c2d9bc9a152e5da42ebf7a709b74c) )
	ROM_LOAD16_BYTE( "2108200u.070", 0x060000, 0x010000, CRC(0ce849aa) SHA1(19252caf180586cadced5c456a755dd954267688) )
	ROM_LOAD16_BYTE( "2107210u.070", 0x060001, 0x010000, CRC(19bc7495) SHA1(8a93bb8e0998b34c92dad263ea78972155c5b785) )
	ROM_LOAD16_BYTE( "2110200v.070", 0x080000, 0x010000, CRC(4f6d22c5) SHA1(fd28782593444f1607f322a2f1971ba8f3d14131) )
	ROM_LOAD16_BYTE( "2109210v.070", 0x080001, 0x010000, CRC(ac6d4d4a) SHA1(fef902700561bb789ff7462f30a438ee9138b472) )
	ROM_LOAD16_BYTE( "2112200w.070", 0x0a0000, 0x010000, CRC(3f896aaf) SHA1(817136ddc37566108de15f6bfedc6e0da13a2df2) )
	ROM_LOAD16_BYTE( "2111210w.070", 0x0a0001, 0x010000, CRC(47f010ad) SHA1(a2587ce1d01c78f1d757fb3e4512be9655d17f9c) )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU3, 0 )	/* region for ADSP 2100 */

	ROM_REGION( 0x14000, REGION_CPU4, 0 )		/* 64k for 6502 code */
	ROM_LOAD( "2123-10c.070", 0x010000, 0x004000, CRC(121ab09a) SHA1(c26b8ddbcb011416e6ab695980d2cf37e672e973) )
	ROM_CONTINUE(             0x004000, 0x00c000 )

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )	/* 384k for ADSP object ROM */
	ROM_LOAD16_BYTE( "2121-90h.070", 0x000000, 0x010000, CRC(0ebf8e58) SHA1(b6bf3e020b29a34ef3eaca6b5e1f17bb89fdc476) )
	ROM_LOAD16_BYTE( "2118-90k.070", 0x000001, 0x010000, CRC(fb98abaf) SHA1(6a141effee644f34634b57d1fe4c03f56981f966) )
	ROM_LOAD16_BYTE( "2122-10h.070", 0x020000, 0x010000, CRC(bd5380bd) SHA1(e1e2b3c9f9bfc988f0dcc9a9f520f51957e13a97) )
	ROM_LOAD16_BYTE( "2119-10k.070", 0x020001, 0x010000, CRC(bde8bd31) SHA1(efb8878382adfe16ba590a28a949029749fc6a63) )
	ROM_LOAD16_BYTE( "2120-9h.070",  0x040000, 0x010000, CRC(55a30976) SHA1(045a04d3d24e783a6a643cab08e8974ee5dc2128) )
	ROM_LOAD16_BYTE( "2117-9k.070",  0x040001, 0x010000, CRC(d4a9696d) SHA1(574e5f3758ac2e18423ae350e8509aa135ca6da0) )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for ADSP I/O buffers */

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* 256k for ADPCM samples */
	ROM_LOAD( "2124-1fh.070", 0x000000, 0x010000, CRC(4dc14fe8) SHA1(c7cc00715f6687ced9d69ec793d6e9d4bc1b5287) )
	ROM_LOAD( "2125-1ef.070", 0x010000, 0x010000, CRC(cbdabbcc) SHA1(4d102a5677d96e68d27c1960dc3a237ae6751c2f) )
	ROM_LOAD( "2126-1de.070", 0x020000, 0x010000, CRC(b973d9d1) SHA1(a74a3c981497a9c5557f793d49381a9b776cb025) )
	ROM_LOAD( "2127-1cd.070", 0x030000, 0x010000, CRC(3e419f4e) SHA1(e382e047f02591a934a53e5fbf07cccf285abb29) )
ROM_END


ROM_START( racedriv )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 1MB for 68000 code */
	ROM_LOAD16_BYTE( "5002200r.077", 0x000000, 0x010000, CRC(0a78adca) SHA1(a44722340ff7c99253107be092bec2e87cae340b) )
	ROM_LOAD16_BYTE( "5001210r.077", 0x000001, 0x010000, CRC(74b4cd49) SHA1(48fc4344c092c9eb14249874ac305b87bba53e7e) )
	ROM_LOAD16_BYTE( "5004200s.077", 0x020000, 0x010000, CRC(c0cbdf4e) SHA1(8c7f4f79e90dc7206d9d83d588822000a7a53c52) )
	ROM_LOAD16_BYTE( "5003210s.077", 0x020001, 0x010000, CRC(28eeff77) SHA1(ccbc021c1230f5fbc2f51bdd4b82014f4a043d4a) )
	ROM_LOAD16_BYTE( "5006200t.077", 0x040000, 0x010000, CRC(11cd9323) SHA1(43bdefb159c2a1c3cb07a629b8b924cdc29606f5) )
	ROM_LOAD16_BYTE( "5005210t.077", 0x040001, 0x010000, CRC(49c33786) SHA1(9597b5b3d4b3bd113c60ba9bd7689c331bf26bbb) )
	ROM_LOAD16_BYTE( "4008200u.077", 0x060000, 0x010000, CRC(aef71435) SHA1(7aa17ce2807bc9d8cd2721c8b709b5056f561055) )
	ROM_LOAD16_BYTE( "4007210u.077", 0x060001, 0x010000, CRC(446e62fb) SHA1(af2464035f35467da6ce1073ce00d60ceb7666ea) )
	ROM_LOAD16_BYTE( "4010200v.077", 0x080000, 0x010000, CRC(e7e03770) SHA1(98cbe3169efcb143f0b59b3154e5ea61f3c12f62) )
	ROM_LOAD16_BYTE( "4009210v.077", 0x080001, 0x010000, CRC(5dd8ebe4) SHA1(98faf28169d16e88280fcd131c5988f040f48ad9) )
	ROM_LOAD16_BYTE( "1012200w.077", 0x0a0000, 0x010000, CRC(9a78b952) SHA1(53270d4d8c28579ebda477a63c034f6d1b9e5a58) )
	ROM_LOAD16_BYTE( "1011210w.077", 0x0a0001, 0x010000, CRC(c5cd5491) SHA1(ede5a3bb888342032d6758b0fb149451b6543d8b) )
	ROM_LOAD16_BYTE( "1014200x.077", 0x0c0000, 0x010000, CRC(a872792a) SHA1(9269e397567940013e5f46cadfe2bad2ca1a2bc4) )
	ROM_LOAD16_BYTE( "1013210x.077", 0x0c0001, 0x010000, CRC(ca7b3e53) SHA1(cdb3a6360893fd3dd2947c050dca8a4dfaa9ced9) )
	ROM_LOAD16_BYTE( "1016200y.077", 0x0e0000, 0x010000, CRC(e83a9c99) SHA1(1d4093902133bb6da981f294e6947544c3564393) )
	ROM_LOAD16_BYTE( "4015210y.077", 0x0e0001, 0x010000, CRC(725806f3) SHA1(0fa4601465dc94f27c71db789ad625bbcd254169) )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU3, 0 )	/* region for ADSP 2100 */

	ROM_REGION( 0x20000, REGION_CPU5, 0 )		/* 2*64k for audio 68000 code */
	ROM_LOAD16_BYTE( "1032-70n.077", 0x000000, 0x008000, CRC(fbc903a6) SHA1(047240a5192918ced52d90b0555ba2e19a26329e) )
	ROM_LOAD16_BYTE( "1033-45n.077", 0x000001, 0x008000, CRC(e032ca7c) SHA1(a80c980c8e58cf8cada72140e42a3cd1ea987b3d) )

	ROM_REGION( 0x10000, REGION_CPU6, 0 )		/* region for audio 32010 */

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )	/* 384k for ADSP object ROM */
	ROM_LOAD16_BYTE( "2021-10h.077", 0x000000, 0x010000, CRC(8b2a98da) SHA1(264b7ec218e423ea85c54e586f8ff091f033d472) )
	ROM_LOAD16_BYTE( "2023-10k.077", 0x000001, 0x010000, CRC(c6d83d38) SHA1(e42c186a7fc0d88982b26eafdb834406b4ed3c8a) )
	ROM_LOAD16_BYTE( "1022-10j.077", 0x020000, 0x010000, CRC(c0393c31) SHA1(31726c01eb0d4650936908c90d45161197b7efba) )
	ROM_LOAD16_BYTE( "1024-10l.077", 0x020001, 0x010000, CRC(1e2fb25f) SHA1(4940091bbad6144bce091d2737191d266d4b0310) )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for ADSP I/O buffers */

	ROM_REGION16_BE( 0x51000, REGION_USER3, 0 )	/* 256k for DSK ROMs + 64k for RAM + 4k for ZRAM */
	ROM_LOAD16_BYTE( "4030-30e.077", 0x000000, 0x010000, CRC(4207c784) SHA1(5ec410bd75c281ac57d9856d08ce65431f3af994) )
	ROM_LOAD16_BYTE( "4031-10e.077", 0x000001, 0x010000, CRC(796486b3) SHA1(937e27c012c5fb457bee1b43fc8e075b3e9405b4) )

	ROM_REGION( 0x50000, REGION_SOUND1, 0 )		/* 10*128k for audio serial ROMs */
	ROM_LOAD( "1123-65a.052", 0x000000, 0x010000, CRC(a88411dc) SHA1(1fd53c7eadffa163d5423df2f8338757e58d5f2e) )
	ROM_LOAD( "1124-55a.052", 0x010000, 0x010000, CRC(071a4309) SHA1(c623bd51d6a4a56503fbf138138854d6a30b11d6) )
	ROM_LOAD( "3125-45a.052", 0x020000, 0x010000, CRC(856548ff) SHA1(e8a17b274185c5e4ecf5f9f1c211e18b3ef2456d) )
	ROM_LOAD( "1126-30a.052", 0x030000, 0x010000, CRC(f46ef09c) SHA1(ba62f73ee3b33d8f26b430ffa468f8792dca23de) )
	ROM_LOAD( "1017-45c.077", 0x040000, 0x010000, CRC(e93129a3) SHA1(1221b08c8efbfd8cf6bfbfd956545f10bef48663) )
ROM_END


ROM_START( racedrvb )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 1MB for 68000 code */
	ROM_LOAD16_BYTE( "5002200r.077", 0x000000, 0x010000, CRC(0a78adca) SHA1(a44722340ff7c99253107be092bec2e87cae340b) )
	ROM_LOAD16_BYTE( "5001210r.077", 0x000001, 0x010000, CRC(74b4cd49) SHA1(48fc4344c092c9eb14249874ac305b87bba53e7e) )
	ROM_LOAD16_BYTE( "5004200s.077", 0x020000, 0x010000, CRC(c0cbdf4e) SHA1(8c7f4f79e90dc7206d9d83d588822000a7a53c52) )
	ROM_LOAD16_BYTE( "5003210s.077", 0x020001, 0x010000, CRC(28eeff77) SHA1(ccbc021c1230f5fbc2f51bdd4b82014f4a043d4a) )
	ROM_LOAD16_BYTE( "5006200t.077", 0x040000, 0x010000, CRC(11cd9323) SHA1(43bdefb159c2a1c3cb07a629b8b924cdc29606f5) )
	ROM_LOAD16_BYTE( "5005210t.077", 0x040001, 0x010000, CRC(49c33786) SHA1(9597b5b3d4b3bd113c60ba9bd7689c331bf26bbb) )
	ROM_LOAD16_BYTE( "4008200u.077", 0x060000, 0x010000, CRC(aef71435) SHA1(7aa17ce2807bc9d8cd2721c8b709b5056f561055) )
	ROM_LOAD16_BYTE( "4007210u.077", 0x060001, 0x010000, CRC(446e62fb) SHA1(af2464035f35467da6ce1073ce00d60ceb7666ea) )
	ROM_LOAD16_BYTE( "4010200v.077", 0x080000, 0x010000, CRC(e7e03770) SHA1(98cbe3169efcb143f0b59b3154e5ea61f3c12f62) )
	ROM_LOAD16_BYTE( "4009210v.077", 0x080001, 0x010000, CRC(5dd8ebe4) SHA1(98faf28169d16e88280fcd131c5988f040f48ad9) )
	ROM_LOAD16_BYTE( "1112200w.077", 0x0a0000, 0x010000, CRC(5b5a2527) SHA1(a5762b4f9d0abbc9a54b1ac3014ae69394c7e03e) )
	ROM_LOAD16_BYTE( "1111210w.077", 0x0a0001, 0x010000, CRC(1f6224ec) SHA1(54d91406629dc64c9458d73e73e7c9fff6dbd915) )
	ROM_LOAD16_BYTE( "1014200x.077", 0x0c0000, 0x010000, CRC(a872792a) SHA1(9269e397567940013e5f46cadfe2bad2ca1a2bc4) )
	ROM_LOAD16_BYTE( "1013210x.077", 0x0c0001, 0x010000, CRC(ca7b3e53) SHA1(cdb3a6360893fd3dd2947c050dca8a4dfaa9ced9) )
	ROM_LOAD16_BYTE( "1016200y.077", 0x0e0000, 0x010000, CRC(e83a9c99) SHA1(1d4093902133bb6da981f294e6947544c3564393) )
	ROM_LOAD16_BYTE( "4015210y.077", 0x0e0001, 0x010000, CRC(725806f3) SHA1(0fa4601465dc94f27c71db789ad625bbcd254169) )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU3, 0 )	/* region for ADSP 2100 */

	ROM_REGION( 0x20000, REGION_CPU5, 0 )		/* 2*64k for audio 68000 code */
	ROM_LOAD16_BYTE( "1032-70n.077", 0x000000, 0x008000, CRC(fbc903a6) SHA1(047240a5192918ced52d90b0555ba2e19a26329e) )
	ROM_LOAD16_BYTE( "1033-45n.077", 0x000001, 0x008000, CRC(e032ca7c) SHA1(a80c980c8e58cf8cada72140e42a3cd1ea987b3d) )

	ROM_REGION( 0x10000, REGION_CPU6, 0 )		/* region for audio 32010 */

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )	/* 384k for ADSP object ROM */
	ROM_LOAD16_BYTE( "2021-10h.077", 0x000000, 0x010000, CRC(8b2a98da) SHA1(264b7ec218e423ea85c54e586f8ff091f033d472) )
	ROM_LOAD16_BYTE( "2023-10k.077", 0x000001, 0x010000, CRC(c6d83d38) SHA1(e42c186a7fc0d88982b26eafdb834406b4ed3c8a) )
	ROM_LOAD16_BYTE( "1022-10j.077", 0x020000, 0x010000, CRC(c0393c31) SHA1(31726c01eb0d4650936908c90d45161197b7efba) )
	ROM_LOAD16_BYTE( "1024-10l.077", 0x020001, 0x010000, CRC(1e2fb25f) SHA1(4940091bbad6144bce091d2737191d266d4b0310) )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for ADSP I/O buffers */

	ROM_REGION16_BE( 0x51000, REGION_USER3, 0 )	/* 256k for DSK ROMs + 64k for RAM + 4k for ZRAM */
	ROM_LOAD16_BYTE( "4030-30e.077", 0x000000, 0x010000, CRC(4207c784) SHA1(5ec410bd75c281ac57d9856d08ce65431f3af994) )
	ROM_LOAD16_BYTE( "4031-10e.077", 0x000001, 0x010000, CRC(796486b3) SHA1(937e27c012c5fb457bee1b43fc8e075b3e9405b4) )

	ROM_REGION( 0x50000, REGION_SOUND1, 0 )		/* 10*128k for audio serial ROMs */
	ROM_LOAD( "1123-65a.052", 0x000000, 0x010000, CRC(a88411dc) SHA1(1fd53c7eadffa163d5423df2f8338757e58d5f2e) )
	ROM_LOAD( "1124-55a.052", 0x010000, 0x010000, CRC(071a4309) SHA1(c623bd51d6a4a56503fbf138138854d6a30b11d6) )
	ROM_LOAD( "3125-45a.052", 0x020000, 0x010000, CRC(856548ff) SHA1(e8a17b274185c5e4ecf5f9f1c211e18b3ef2456d) )
	ROM_LOAD( "1126-30a.052", 0x030000, 0x010000, CRC(f46ef09c) SHA1(ba62f73ee3b33d8f26b430ffa468f8792dca23de) )
	ROM_LOAD( "1017-45c.077", 0x040000, 0x010000, CRC(e93129a3) SHA1(1221b08c8efbfd8cf6bfbfd956545f10bef48663) )
ROM_END


ROM_START( racedrvg )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 1MB for 68000 code */
	ROM_LOAD16_BYTE( "5202200r.077", 0x000000, 0x010000, CRC(69016b69) SHA1(8da1314b4b6da6fc95479b9034190181bf778b25) )
	ROM_LOAD16_BYTE( "5201210r.077", 0x000001, 0x010000, CRC(439a1ce7) SHA1(fc376c654c494d2895bcf85d5d8a9d6f17b6887e) )
	ROM_LOAD16_BYTE( "5204200s.077", 0x020000, 0x010000, CRC(15df2b3d) SHA1(49235687b7be684dcd491b854f80c1d849894956) )
	ROM_LOAD16_BYTE( "5203210s.077", 0x020001, 0x010000, CRC(1ac45a5d) SHA1(51463df829a5b0e2649f91b54ba718a1e690b492) )
	ROM_LOAD16_BYTE( "5206200t.077", 0x040000, 0x010000, CRC(02da252f) SHA1(9a63f150913654e9de6e23fcd450e06aae91b10c) )
	ROM_LOAD16_BYTE( "5205210t.077", 0x040001, 0x010000, CRC(c810c576) SHA1(664082c6aaddc3b967c19b4505a0c5022821b445) )
	ROM_LOAD16_BYTE( "4008200u.077", 0x060000, 0x010000, CRC(aef71435) SHA1(7aa17ce2807bc9d8cd2721c8b709b5056f561055) )
	ROM_LOAD16_BYTE( "4007210u.077", 0x060001, 0x010000, CRC(446e62fb) SHA1(af2464035f35467da6ce1073ce00d60ceb7666ea) )
	ROM_LOAD16_BYTE( "4010200v.077", 0x080000, 0x010000, CRC(e7e03770) SHA1(98cbe3169efcb143f0b59b3154e5ea61f3c12f62) )
	ROM_LOAD16_BYTE( "4009210v.077", 0x080001, 0x010000, CRC(5dd8ebe4) SHA1(98faf28169d16e88280fcd131c5988f040f48ad9) )
	ROM_LOAD16_BYTE( "1012200w.077", 0x0a0000, 0x010000, CRC(9a78b952) SHA1(53270d4d8c28579ebda477a63c034f6d1b9e5a58) )
	ROM_LOAD16_BYTE( "1011210w.077", 0x0a0001, 0x010000, CRC(c5cd5491) SHA1(ede5a3bb888342032d6758b0fb149451b6543d8b) )
	ROM_LOAD16_BYTE( "1014200x.077", 0x0c0000, 0x010000, CRC(a872792a) SHA1(9269e397567940013e5f46cadfe2bad2ca1a2bc4) )
	ROM_LOAD16_BYTE( "1013210x.077", 0x0c0001, 0x010000, CRC(ca7b3e53) SHA1(cdb3a6360893fd3dd2947c050dca8a4dfaa9ced9) )
	ROM_LOAD16_BYTE( "1016200y.077", 0x0e0000, 0x010000, CRC(e83a9c99) SHA1(1d4093902133bb6da981f294e6947544c3564393) )
	ROM_LOAD16_BYTE( "4015210y.077", 0x0e0001, 0x010000, CRC(725806f3) SHA1(0fa4601465dc94f27c71db789ad625bbcd254169) )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU3, 0 )	/* region for ADSP 2100 */

	ROM_REGION( 0x20000, REGION_CPU5, 0 )		/* 2*64k for audio 68000 code */
	ROM_LOAD16_BYTE( "1032-70n.077", 0x000000, 0x008000, CRC(fbc903a6) SHA1(047240a5192918ced52d90b0555ba2e19a26329e) )
	ROM_LOAD16_BYTE( "1033-45n.077", 0x000001, 0x008000, CRC(e032ca7c) SHA1(a80c980c8e58cf8cada72140e42a3cd1ea987b3d) )

	ROM_REGION( 0x10000, REGION_CPU6, 0 )		/* region for audio 32010 */

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )	/* 384k for ADSP object ROM */
	ROM_LOAD16_BYTE( "2021-10h.077", 0x000000, 0x010000, CRC(8b2a98da) SHA1(264b7ec218e423ea85c54e586f8ff091f033d472) )
	ROM_LOAD16_BYTE( "2023-10k.077", 0x000001, 0x010000, CRC(c6d83d38) SHA1(e42c186a7fc0d88982b26eafdb834406b4ed3c8a) )
	ROM_LOAD16_BYTE( "1022-10j.077", 0x020000, 0x010000, CRC(c0393c31) SHA1(31726c01eb0d4650936908c90d45161197b7efba) )
	ROM_LOAD16_BYTE( "1024-10l.077", 0x020001, 0x010000, CRC(1e2fb25f) SHA1(4940091bbad6144bce091d2737191d266d4b0310) )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for ADSP I/O buffers */

	ROM_REGION16_BE( 0x51000, REGION_USER3, 0 )	/* 256k for DSK ROMs + 64k for RAM + 4k for ZRAM */
	ROM_LOAD16_BYTE( "4030-30e.077", 0x000000, 0x010000, CRC(4207c784) SHA1(5ec410bd75c281ac57d9856d08ce65431f3af994) )
	ROM_LOAD16_BYTE( "4031-10e.077", 0x000001, 0x010000, CRC(796486b3) SHA1(937e27c012c5fb457bee1b43fc8e075b3e9405b4) )

	ROM_REGION( 0x50000, REGION_SOUND1, 0 )		/* 10*128k for audio serial ROMs */
	ROM_LOAD( "1123-65a.052", 0x000000, 0x010000, CRC(a88411dc) SHA1(1fd53c7eadffa163d5423df2f8338757e58d5f2e) )
	ROM_LOAD( "1124-55a.052", 0x010000, 0x010000, CRC(071a4309) SHA1(c623bd51d6a4a56503fbf138138854d6a30b11d6) )
	ROM_LOAD( "3125-45a.052", 0x020000, 0x010000, CRC(856548ff) SHA1(e8a17b274185c5e4ecf5f9f1c211e18b3ef2456d) )
	ROM_LOAD( "1126-30a.052", 0x030000, 0x010000, CRC(f46ef09c) SHA1(ba62f73ee3b33d8f26b430ffa468f8792dca23de) )
	ROM_LOAD( "1017-45c.077", 0x040000, 0x010000, CRC(e93129a3) SHA1(1221b08c8efbfd8cf6bfbfd956545f10bef48663) )
ROM_END


ROM_START( racedrv4 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 1MB for 68000 code */
	ROM_LOAD16_BYTE( "4002200r.077", 0x000000, 0x010000, CRC(afe1beb5) SHA1(66dba58f2a02eaa2d6306e53adffb662a8021031) )
	ROM_LOAD16_BYTE( "4001210r.077", 0x000001, 0x010000, CRC(8a449e44) SHA1(03800b96a3992baa4bd37db20e559e08b12e55d1) )
	ROM_LOAD16_BYTE( "4004200s.077", 0x020000, 0x010000, CRC(6d886150) SHA1(514de55de66a2b7fb14d6e2d896911e1b1af62f2) )
	ROM_LOAD16_BYTE( "4003210s.077", 0x020001, 0x010000, CRC(84e9f56b) SHA1(bc22f3e56c530a3e3f4c43bc400919a7b23a7a1f) )
	ROM_LOAD16_BYTE( "4006200t.077", 0x040000, 0x010000, CRC(4a2340f6) SHA1(fb28977bb437bc1891f70b37b066878ef878ecc2) )
	ROM_LOAD16_BYTE( "4005210t.077", 0x040001, 0x010000, CRC(9572b2ac) SHA1(ab5da7f379cff1c574c42317fabc749f0ad0e967) )
	ROM_LOAD16_BYTE( "4008200u.077", 0x060000, 0x010000, CRC(aef71435) SHA1(7aa17ce2807bc9d8cd2721c8b709b5056f561055) )
	ROM_LOAD16_BYTE( "4007210u.077", 0x060001, 0x010000, CRC(446e62fb) SHA1(af2464035f35467da6ce1073ce00d60ceb7666ea) )
	ROM_LOAD16_BYTE( "4010200v.077", 0x080000, 0x010000, CRC(e7e03770) SHA1(98cbe3169efcb143f0b59b3154e5ea61f3c12f62) )
	ROM_LOAD16_BYTE( "4009210v.077", 0x080001, 0x010000, CRC(5dd8ebe4) SHA1(98faf28169d16e88280fcd131c5988f040f48ad9) )
	ROM_LOAD16_BYTE( "1012200w.077", 0x0a0000, 0x010000, CRC(9a78b952) SHA1(53270d4d8c28579ebda477a63c034f6d1b9e5a58) )
	ROM_LOAD16_BYTE( "1011210w.077", 0x0a0001, 0x010000, CRC(c5cd5491) SHA1(ede5a3bb888342032d6758b0fb149451b6543d8b) )
	ROM_LOAD16_BYTE( "1014200x.077", 0x0c0000, 0x010000, CRC(a872792a) SHA1(9269e397567940013e5f46cadfe2bad2ca1a2bc4) )
	ROM_LOAD16_BYTE( "1013210x.077", 0x0c0001, 0x010000, CRC(ca7b3e53) SHA1(cdb3a6360893fd3dd2947c050dca8a4dfaa9ced9) )
	ROM_LOAD16_BYTE( "1016200y.077", 0x0e0000, 0x010000, CRC(e83a9c99) SHA1(1d4093902133bb6da981f294e6947544c3564393) )
	ROM_LOAD16_BYTE( "4015210y.077", 0x0e0001, 0x010000, CRC(725806f3) SHA1(0fa4601465dc94f27c71db789ad625bbcd254169) )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU3, 0 )	/* region for ADSP 2100 */

	ROM_REGION( 0x20000, REGION_CPU5, 0 )		/* 2*64k for audio 68000 code */
	ROM_LOAD16_BYTE( "1032-70n.077", 0x000000, 0x008000, CRC(fbc903a6) SHA1(047240a5192918ced52d90b0555ba2e19a26329e) )
	ROM_LOAD16_BYTE( "1033-45n.077", 0x000001, 0x008000, CRC(e032ca7c) SHA1(a80c980c8e58cf8cada72140e42a3cd1ea987b3d) )

	ROM_REGION( 0x10000, REGION_CPU6, 0 )		/* region for audio 32010 */

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )	/* 384k for ADSP object ROM */
	ROM_LOAD16_BYTE( "2021-10h.077", 0x000000, 0x010000, CRC(8b2a98da) SHA1(264b7ec218e423ea85c54e586f8ff091f033d472) )
	ROM_LOAD16_BYTE( "2023-10k.077", 0x000001, 0x010000, CRC(c6d83d38) SHA1(e42c186a7fc0d88982b26eafdb834406b4ed3c8a) )
	ROM_LOAD16_BYTE( "1022-10j.077", 0x020000, 0x010000, CRC(c0393c31) SHA1(31726c01eb0d4650936908c90d45161197b7efba) )
	ROM_LOAD16_BYTE( "1024-10l.077", 0x020001, 0x010000, CRC(1e2fb25f) SHA1(4940091bbad6144bce091d2737191d266d4b0310) )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for ADSP I/O buffers */

	ROM_REGION16_BE( 0x51000, REGION_USER3, 0 )	/* 256k for DSK ROMs + 64k for RAM + 4k for ZRAM */
	ROM_LOAD16_BYTE( "4030-30e.077", 0x000000, 0x010000, CRC(4207c784) SHA1(5ec410bd75c281ac57d9856d08ce65431f3af994) )
	ROM_LOAD16_BYTE( "4031-10e.077", 0x000001, 0x010000, CRC(796486b3) SHA1(937e27c012c5fb457bee1b43fc8e075b3e9405b4) )

	ROM_REGION( 0x50000, REGION_SOUND1, 0 )		/* 10*128k for audio serial ROMs */
	ROM_LOAD( "1123-65a.052", 0x000000, 0x010000, CRC(a88411dc) SHA1(1fd53c7eadffa163d5423df2f8338757e58d5f2e) )
	ROM_LOAD( "1124-55a.052", 0x010000, 0x010000, CRC(071a4309) SHA1(c623bd51d6a4a56503fbf138138854d6a30b11d6) )
	ROM_LOAD( "3125-45a.052", 0x020000, 0x010000, CRC(856548ff) SHA1(e8a17b274185c5e4ecf5f9f1c211e18b3ef2456d) )
	ROM_LOAD( "1126-30a.052", 0x030000, 0x010000, CRC(f46ef09c) SHA1(ba62f73ee3b33d8f26b430ffa468f8792dca23de) )
	ROM_LOAD( "1017-45c.077", 0x040000, 0x010000, CRC(e93129a3) SHA1(1221b08c8efbfd8cf6bfbfd956545f10bef48663) )
ROM_END


ROM_START( racedrb4 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 1MB for 68000 code */
	ROM_LOAD16_BYTE( "4002200r.077", 0x000000, 0x010000, CRC(afe1beb5) SHA1(66dba58f2a02eaa2d6306e53adffb662a8021031) )
	ROM_LOAD16_BYTE( "4001210r.077", 0x000001, 0x010000, CRC(8a449e44) SHA1(03800b96a3992baa4bd37db20e559e08b12e55d1) )
	ROM_LOAD16_BYTE( "4004200s.077", 0x020000, 0x010000, CRC(6d886150) SHA1(514de55de66a2b7fb14d6e2d896911e1b1af62f2) )
	ROM_LOAD16_BYTE( "4003210s.077", 0x020001, 0x010000, CRC(84e9f56b) SHA1(bc22f3e56c530a3e3f4c43bc400919a7b23a7a1f) )
	ROM_LOAD16_BYTE( "4006200t.077", 0x040000, 0x010000, CRC(4a2340f6) SHA1(fb28977bb437bc1891f70b37b066878ef878ecc2) )
	ROM_LOAD16_BYTE( "4005210t.077", 0x040001, 0x010000, CRC(9572b2ac) SHA1(ab5da7f379cff1c574c42317fabc749f0ad0e967) )
	ROM_LOAD16_BYTE( "4008200u.077", 0x060000, 0x010000, CRC(aef71435) SHA1(7aa17ce2807bc9d8cd2721c8b709b5056f561055) )
	ROM_LOAD16_BYTE( "4007210u.077", 0x060001, 0x010000, CRC(446e62fb) SHA1(af2464035f35467da6ce1073ce00d60ceb7666ea) )
	ROM_LOAD16_BYTE( "4010200v.077", 0x080000, 0x010000, CRC(e7e03770) SHA1(98cbe3169efcb143f0b59b3154e5ea61f3c12f62) )
	ROM_LOAD16_BYTE( "4009210v.077", 0x080001, 0x010000, CRC(5dd8ebe4) SHA1(98faf28169d16e88280fcd131c5988f040f48ad9) )
	ROM_LOAD16_BYTE( "1112200w.077", 0x0a0000, 0x010000, CRC(5b5a2527) SHA1(a5762b4f9d0abbc9a54b1ac3014ae69394c7e03e) )
	ROM_LOAD16_BYTE( "1111210w.077", 0x0a0001, 0x010000, CRC(1f6224ec) SHA1(54d91406629dc64c9458d73e73e7c9fff6dbd915) )
	ROM_LOAD16_BYTE( "1014200x.077", 0x0c0000, 0x010000, CRC(a872792a) SHA1(9269e397567940013e5f46cadfe2bad2ca1a2bc4) )
	ROM_LOAD16_BYTE( "1013210x.077", 0x0c0001, 0x010000, CRC(ca7b3e53) SHA1(cdb3a6360893fd3dd2947c050dca8a4dfaa9ced9) )
	ROM_LOAD16_BYTE( "1016200y.077", 0x0e0000, 0x010000, CRC(e83a9c99) SHA1(1d4093902133bb6da981f294e6947544c3564393) )
	ROM_LOAD16_BYTE( "4015210y.077", 0x0e0001, 0x010000, CRC(725806f3) SHA1(0fa4601465dc94f27c71db789ad625bbcd254169) )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU3, 0 )	/* region for ADSP 2100 */

	ROM_REGION( 0x20000, REGION_CPU5, 0 )		/* 2*64k for audio 68000 code */
	ROM_LOAD16_BYTE( "1032-70n.077", 0x000000, 0x008000, CRC(fbc903a6) SHA1(047240a5192918ced52d90b0555ba2e19a26329e) )
	ROM_LOAD16_BYTE( "1033-45n.077", 0x000001, 0x008000, CRC(e032ca7c) SHA1(a80c980c8e58cf8cada72140e42a3cd1ea987b3d) )

	ROM_REGION( 0x10000, REGION_CPU6, 0 )		/* region for audio 32010 */

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )	/* 384k for ADSP object ROM */
	ROM_LOAD16_BYTE( "2021-10h.077", 0x000000, 0x010000, CRC(8b2a98da) SHA1(264b7ec218e423ea85c54e586f8ff091f033d472) )
	ROM_LOAD16_BYTE( "2023-10k.077", 0x000001, 0x010000, CRC(c6d83d38) SHA1(e42c186a7fc0d88982b26eafdb834406b4ed3c8a) )
	ROM_LOAD16_BYTE( "1022-10j.077", 0x020000, 0x010000, CRC(c0393c31) SHA1(31726c01eb0d4650936908c90d45161197b7efba) )
	ROM_LOAD16_BYTE( "1024-10l.077", 0x020001, 0x010000, CRC(1e2fb25f) SHA1(4940091bbad6144bce091d2737191d266d4b0310) )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for ADSP I/O buffers */

	ROM_REGION16_BE( 0x51000, REGION_USER3, 0 )	/* 256k for DSK ROMs + 64k for RAM + 4k for ZRAM */
	ROM_LOAD16_BYTE( "4030-30e.077", 0x000000, 0x010000, CRC(4207c784) SHA1(5ec410bd75c281ac57d9856d08ce65431f3af994) )
	ROM_LOAD16_BYTE( "4031-10e.077", 0x000001, 0x010000, CRC(796486b3) SHA1(937e27c012c5fb457bee1b43fc8e075b3e9405b4) )

	ROM_REGION( 0x50000, REGION_SOUND1, 0 )		/* 10*128k for audio serial ROMs */
	ROM_LOAD( "1123-65a.052", 0x000000, 0x010000, CRC(a88411dc) SHA1(1fd53c7eadffa163d5423df2f8338757e58d5f2e) )
	ROM_LOAD( "1124-55a.052", 0x010000, 0x010000, CRC(071a4309) SHA1(c623bd51d6a4a56503fbf138138854d6a30b11d6) )
	ROM_LOAD( "3125-45a.052", 0x020000, 0x010000, CRC(856548ff) SHA1(e8a17b274185c5e4ecf5f9f1c211e18b3ef2456d) )
	ROM_LOAD( "1126-30a.052", 0x030000, 0x010000, CRC(f46ef09c) SHA1(ba62f73ee3b33d8f26b430ffa468f8792dca23de) )
	ROM_LOAD( "1017-45c.077", 0x040000, 0x010000, CRC(e93129a3) SHA1(1221b08c8efbfd8cf6bfbfd956545f10bef48663) )
ROM_END


ROM_START( racedrg4 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 1MB for 68000 code */
	ROM_LOAD16_BYTE( "4202200r.077", 0x000000, 0x010000, CRC(a8c7e15e) SHA1(6aa4b819e5c8562c8eef92a411b2dc91ed35ae4c) )
	ROM_LOAD16_BYTE( "4201210r.077", 0x000001, 0x010000, CRC(1dfa4df2) SHA1(1db0676f0ff1e7996c9791b041d9cd4ca9c70261) )
	ROM_LOAD16_BYTE( "4204200s.077", 0x020000, 0x010000, CRC(29eea8c1) SHA1(1a8020efc0b963bbd05bd33dc3176db7c3234fcc) )
	ROM_LOAD16_BYTE( "4203210s.077", 0x020001, 0x010000, CRC(a85e20c0) SHA1(f47d8858ae4f4bc8a95ea850af17dcfd4a8f6977) )
	ROM_LOAD16_BYTE( "4206200t.077", 0x040000, 0x010000, CRC(f76fde13) SHA1(18a6e9db1458b0710d1524fde9fab957a5a1c792) )
	ROM_LOAD16_BYTE( "4205210t.077", 0x040001, 0x010000, CRC(e8279f91) SHA1(7b3cf8f6adf2d29d2ae51b471beb44481accadb5) )
	ROM_LOAD16_BYTE( "4008200u.077", 0x060000, 0x010000, CRC(aef71435) SHA1(7aa17ce2807bc9d8cd2721c8b709b5056f561055) )
	ROM_LOAD16_BYTE( "4007210u.077", 0x060001, 0x010000, CRC(446e62fb) SHA1(af2464035f35467da6ce1073ce00d60ceb7666ea) )
	ROM_LOAD16_BYTE( "4010200v.077", 0x080000, 0x010000, CRC(e7e03770) SHA1(98cbe3169efcb143f0b59b3154e5ea61f3c12f62) )
	ROM_LOAD16_BYTE( "4009210v.077", 0x080001, 0x010000, CRC(5dd8ebe4) SHA1(98faf28169d16e88280fcd131c5988f040f48ad9) )
	ROM_LOAD16_BYTE( "1012200w.077", 0x0a0000, 0x010000, CRC(9a78b952) SHA1(53270d4d8c28579ebda477a63c034f6d1b9e5a58) )
	ROM_LOAD16_BYTE( "1011210w.077", 0x0a0001, 0x010000, CRC(c5cd5491) SHA1(ede5a3bb888342032d6758b0fb149451b6543d8b) )
	ROM_LOAD16_BYTE( "1014200x.077", 0x0c0000, 0x010000, CRC(a872792a) SHA1(9269e397567940013e5f46cadfe2bad2ca1a2bc4) )
	ROM_LOAD16_BYTE( "1013210x.077", 0x0c0001, 0x010000, CRC(ca7b3e53) SHA1(cdb3a6360893fd3dd2947c050dca8a4dfaa9ced9) )
	ROM_LOAD16_BYTE( "1016200y.077", 0x0e0000, 0x010000, CRC(e83a9c99) SHA1(1d4093902133bb6da981f294e6947544c3564393) )
	ROM_LOAD16_BYTE( "4015210y.077", 0x0e0001, 0x010000, CRC(725806f3) SHA1(0fa4601465dc94f27c71db789ad625bbcd254169) )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU3, 0 )	/* region for ADSP 2100 */

	ROM_REGION( 0x20000, REGION_CPU5, 0 )		/* 2*64k for audio 68000 code */
	ROM_LOAD16_BYTE( "1032-70n.077", 0x000000, 0x008000, CRC(fbc903a6) SHA1(047240a5192918ced52d90b0555ba2e19a26329e) )
	ROM_LOAD16_BYTE( "1033-45n.077", 0x000001, 0x008000, CRC(e032ca7c) SHA1(a80c980c8e58cf8cada72140e42a3cd1ea987b3d) )

	ROM_REGION( 0x10000, REGION_CPU6, 0 )		/* region for audio 32010 */

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )	/* 384k for ADSP object ROM */
	ROM_LOAD16_BYTE( "2021-10h.077", 0x000000, 0x010000, CRC(8b2a98da) SHA1(264b7ec218e423ea85c54e586f8ff091f033d472) )
	ROM_LOAD16_BYTE( "2023-10k.077", 0x000001, 0x010000, CRC(c6d83d38) SHA1(e42c186a7fc0d88982b26eafdb834406b4ed3c8a) )
	ROM_LOAD16_BYTE( "1022-10j.077", 0x020000, 0x010000, CRC(c0393c31) SHA1(31726c01eb0d4650936908c90d45161197b7efba) )
	ROM_LOAD16_BYTE( "1024-10l.077", 0x020001, 0x010000, CRC(1e2fb25f) SHA1(4940091bbad6144bce091d2737191d266d4b0310) )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for ADSP I/O buffers */

	ROM_REGION16_BE( 0x51000, REGION_USER3, 0 )	/* 256k for DSK ROMs + 64k for RAM + 4k for ZRAM */
	ROM_LOAD16_BYTE( "4030-30e.077", 0x000000, 0x010000, CRC(4207c784) SHA1(5ec410bd75c281ac57d9856d08ce65431f3af994) )
	ROM_LOAD16_BYTE( "4031-10e.077", 0x000001, 0x010000, CRC(796486b3) SHA1(937e27c012c5fb457bee1b43fc8e075b3e9405b4) )

	ROM_REGION( 0x50000, REGION_SOUND1, 0 )		/* 10*128k for audio serial ROMs */
	ROM_LOAD( "1123-65a.052", 0x000000, 0x010000, CRC(a88411dc) SHA1(1fd53c7eadffa163d5423df2f8338757e58d5f2e) )
	ROM_LOAD( "1124-55a.052", 0x010000, 0x010000, CRC(071a4309) SHA1(c623bd51d6a4a56503fbf138138854d6a30b11d6) )
	ROM_LOAD( "3125-45a.052", 0x020000, 0x010000, CRC(856548ff) SHA1(e8a17b274185c5e4ecf5f9f1c211e18b3ef2456d) )
	ROM_LOAD( "1126-30a.052", 0x030000, 0x010000, CRC(f46ef09c) SHA1(ba62f73ee3b33d8f26b430ffa468f8792dca23de) )
	ROM_LOAD( "1017-45c.077", 0x040000, 0x010000, CRC(e93129a3) SHA1(1221b08c8efbfd8cf6bfbfd956545f10bef48663) )
ROM_END


ROM_START( racedrv3 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 1MB for 68000 code */
	ROM_LOAD16_BYTE( "3002200r.077", 0x000000, 0x010000, CRC(78771253) SHA1(88fdae99eb4feb40db8ad171b3435315db3adedb) )
	ROM_LOAD16_BYTE( "3001210r.077", 0x000001, 0x010000, CRC(c75373a4) SHA1(d2f14190218cfedf4478806a26c77edd4d7c73eb) )
	ROM_LOAD16_BYTE( "2004200s.077", 0x020000, 0x010000, CRC(4eb19582) SHA1(52359d7839f3459aec4fdc16a659a29fa60feee4) )
	ROM_LOAD16_BYTE( "2003210s.077", 0x020001, 0x010000, CRC(8c36b745) SHA1(d4a39b721dffed7aa41ce0f3f1ae273c6261074f) )
	ROM_LOAD16_BYTE( "2006200t.077", 0x040000, 0x010000, CRC(07fd762e) SHA1(94d9873416fd8d13fc8705ad06c3b4dffd271d90) )
	ROM_LOAD16_BYTE( "2005210t.077", 0x040001, 0x010000, CRC(71c0a770) SHA1(011e91006c542e30213f71a910c9de67477cd6b3) )
	ROM_LOAD16_BYTE( "2008200u.077", 0x060000, 0x010000, CRC(5144d31b) SHA1(5d5b05554d5e0c2f58196834c2445ed48a729df7) )
	ROM_LOAD16_BYTE( "2007210u.077", 0x060001, 0x010000, CRC(17903148) SHA1(85001910c0e7f7fb5cef3fe989ef27c0a0b7003e) )
	ROM_LOAD16_BYTE( "2010200v.077", 0x080000, 0x010000, CRC(8674e44e) SHA1(5a81b93f6ccb3f92fdebb6500051561cb1d963dd) )
	ROM_LOAD16_BYTE( "2009210v.077", 0x080001, 0x010000, CRC(1e9e4c31) SHA1(ec77d1b181cf3268f606a513dc5103e6bb311a68) )
	ROM_LOAD16_BYTE( "1012200w.077", 0x0a0000, 0x010000, CRC(9a78b952) SHA1(53270d4d8c28579ebda477a63c034f6d1b9e5a58) )
	ROM_LOAD16_BYTE( "1011210w.077", 0x0a0001, 0x010000, CRC(c5cd5491) SHA1(ede5a3bb888342032d6758b0fb149451b6543d8b) )
	ROM_LOAD16_BYTE( "1014200x.077", 0x0c0000, 0x010000, CRC(a872792a) SHA1(9269e397567940013e5f46cadfe2bad2ca1a2bc4) )
	ROM_LOAD16_BYTE( "1013210x.077", 0x0c0001, 0x010000, CRC(ca7b3e53) SHA1(cdb3a6360893fd3dd2947c050dca8a4dfaa9ced9) )
	ROM_LOAD16_BYTE( "1016200y.077", 0x0e0000, 0x010000, CRC(e83a9c99) SHA1(1d4093902133bb6da981f294e6947544c3564393) )
	ROM_LOAD16_BYTE( "1015210y.077", 0x0e0001, 0x010000, CRC(c51f2702) SHA1(2279f15c4c09af92fe9b87dc0ed842092ca64906) )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU3, 0 )	/* region for ADSP 2100 */

	ROM_REGION( 0x20000, REGION_CPU5, 0 )		/* 2*64k for audio 68000 code */
	ROM_LOAD16_BYTE( "1032-70n.077", 0x000000, 0x008000, CRC(fbc903a6) SHA1(047240a5192918ced52d90b0555ba2e19a26329e) )
	ROM_LOAD16_BYTE( "1033-45n.077", 0x000001, 0x008000, CRC(e032ca7c) SHA1(a80c980c8e58cf8cada72140e42a3cd1ea987b3d) )

	ROM_REGION( 0x10000, REGION_CPU6, 0 )		/* region for audio 32010 */

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )	/* 384k for ADSP object ROM */
	ROM_LOAD16_BYTE( "2021-10h.077", 0x000000, 0x010000, CRC(8b2a98da) SHA1(264b7ec218e423ea85c54e586f8ff091f033d472) )
	ROM_LOAD16_BYTE( "2023-10k.077", 0x000001, 0x010000, CRC(c6d83d38) SHA1(e42c186a7fc0d88982b26eafdb834406b4ed3c8a) )
	ROM_LOAD16_BYTE( "1022-10j.077", 0x020000, 0x010000, CRC(c0393c31) SHA1(31726c01eb0d4650936908c90d45161197b7efba) )
	ROM_LOAD16_BYTE( "1024-10l.077", 0x020001, 0x010000, CRC(1e2fb25f) SHA1(4940091bbad6144bce091d2737191d266d4b0310) )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for ADSP I/O buffers */

	ROM_REGION16_BE( 0x51000, REGION_USER3, 0 )	/* 256k for DSK ROMs + 64k for RAM + 4k for ZRAM */
	ROM_LOAD16_BYTE( "1030-30e.077", 0x000000, 0x010000, CRC(ff575b3d) SHA1(7f2202db7ec9dac06499c9535b4852defb192eb3) )
	ROM_LOAD16_BYTE( "1031-10e.077", 0x000001, 0x010000, CRC(2ae2ac35) SHA1(e2a050a6e24dc6fef86b3556b69b2c3e2993febc) )

	ROM_REGION( 0x50000, REGION_SOUND1, 0 )		/* 10*128k for audio serial ROMs */
	ROM_LOAD( "1123-65a.052", 0x000000, 0x010000, CRC(a88411dc) SHA1(1fd53c7eadffa163d5423df2f8338757e58d5f2e) )
	ROM_LOAD( "1124-55a.052", 0x010000, 0x010000, CRC(071a4309) SHA1(c623bd51d6a4a56503fbf138138854d6a30b11d6) )
	ROM_LOAD( "3125-45a.052", 0x020000, 0x010000, CRC(856548ff) SHA1(e8a17b274185c5e4ecf5f9f1c211e18b3ef2456d) )
	ROM_LOAD( "1126-30a.052", 0x030000, 0x010000, CRC(f46ef09c) SHA1(ba62f73ee3b33d8f26b430ffa468f8792dca23de) )
	ROM_LOAD( "1017-45c.077", 0x040000, 0x010000, CRC(e93129a3) SHA1(1221b08c8efbfd8cf6bfbfd956545f10bef48663) )
ROM_END


ROM_START( racedrv2 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 1MB for 68000 code */
	ROM_LOAD16_BYTE( "2002200r.077", 0x000000, 0x010000, CRC(6b440e10) SHA1(8074a1de01e2cdc497a51339f9c05329d6f540b0) )
	ROM_LOAD16_BYTE( "2001210r.077", 0x000001, 0x010000, CRC(eee12973) SHA1(0cd2e88c431d08fba5a73c903327f39f8692f3aa) )
	ROM_LOAD16_BYTE( "2004200s.077", 0x020000, 0x010000, CRC(4eb19582) SHA1(52359d7839f3459aec4fdc16a659a29fa60feee4) )
	ROM_LOAD16_BYTE( "2003210s.077", 0x020001, 0x010000, CRC(8c36b745) SHA1(d4a39b721dffed7aa41ce0f3f1ae273c6261074f) )
	ROM_LOAD16_BYTE( "2006200t.077", 0x040000, 0x010000, CRC(07fd762e) SHA1(94d9873416fd8d13fc8705ad06c3b4dffd271d90) )
	ROM_LOAD16_BYTE( "2005210t.077", 0x040001, 0x010000, CRC(71c0a770) SHA1(011e91006c542e30213f71a910c9de67477cd6b3) )
	ROM_LOAD16_BYTE( "2008200u.077", 0x060000, 0x010000, CRC(5144d31b) SHA1(5d5b05554d5e0c2f58196834c2445ed48a729df7) )
	ROM_LOAD16_BYTE( "2007210u.077", 0x060001, 0x010000, CRC(17903148) SHA1(85001910c0e7f7fb5cef3fe989ef27c0a0b7003e) )
	ROM_LOAD16_BYTE( "2010200v.077", 0x080000, 0x010000, CRC(8674e44e) SHA1(5a81b93f6ccb3f92fdebb6500051561cb1d963dd) )
	ROM_LOAD16_BYTE( "2009210v.077", 0x080001, 0x010000, CRC(1e9e4c31) SHA1(ec77d1b181cf3268f606a513dc5103e6bb311a68) )
	ROM_LOAD16_BYTE( "1012200w.077", 0x0a0000, 0x010000, CRC(9a78b952) SHA1(53270d4d8c28579ebda477a63c034f6d1b9e5a58) )
	ROM_LOAD16_BYTE( "1011210w.077", 0x0a0001, 0x010000, CRC(c5cd5491) SHA1(ede5a3bb888342032d6758b0fb149451b6543d8b) )
	ROM_LOAD16_BYTE( "1014200x.077", 0x0c0000, 0x010000, CRC(a872792a) SHA1(9269e397567940013e5f46cadfe2bad2ca1a2bc4) )
	ROM_LOAD16_BYTE( "1013210x.077", 0x0c0001, 0x010000, CRC(ca7b3e53) SHA1(cdb3a6360893fd3dd2947c050dca8a4dfaa9ced9) )
	ROM_LOAD16_BYTE( "1016200y.077", 0x0e0000, 0x010000, CRC(e83a9c99) SHA1(1d4093902133bb6da981f294e6947544c3564393) )
	ROM_LOAD16_BYTE( "1015210y.077", 0x0e0001, 0x010000, CRC(c51f2702) SHA1(2279f15c4c09af92fe9b87dc0ed842092ca64906) )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU3, 0 )	/* region for ADSP 2100 */

	ROM_REGION( 0x20000, REGION_CPU5, 0 )		/* 2*64k for audio 68000 code */
	ROM_LOAD16_BYTE( "1032-70n.077", 0x000000, 0x008000, CRC(fbc903a6) SHA1(047240a5192918ced52d90b0555ba2e19a26329e) )
	ROM_LOAD16_BYTE( "1033-45n.077", 0x000001, 0x008000, CRC(e032ca7c) SHA1(a80c980c8e58cf8cada72140e42a3cd1ea987b3d) )

	ROM_REGION( 0x10000, REGION_CPU6, 0 )		/* region for audio 32010 */

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )	/* 384k for ADSP object ROM */
	ROM_LOAD16_BYTE( "2021-10h.077", 0x000000, 0x010000, CRC(8b2a98da) SHA1(264b7ec218e423ea85c54e586f8ff091f033d472) )
	ROM_LOAD16_BYTE( "2023-10k.077", 0x000001, 0x010000, CRC(c6d83d38) SHA1(e42c186a7fc0d88982b26eafdb834406b4ed3c8a) )
	ROM_LOAD16_BYTE( "1022-10j.077", 0x020000, 0x010000, CRC(c0393c31) SHA1(31726c01eb0d4650936908c90d45161197b7efba) )
	ROM_LOAD16_BYTE( "1024-10l.077", 0x020001, 0x010000, CRC(1e2fb25f) SHA1(4940091bbad6144bce091d2737191d266d4b0310) )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for ADSP I/O buffers */

	ROM_REGION16_BE( 0x51000, REGION_USER3, 0 )	/* 256k for DSK ROMs + 64k for RAM + 4k for ZRAM */
	ROM_LOAD16_BYTE( "1030-30e.077", 0x000000, 0x010000, CRC(ff575b3d) SHA1(7f2202db7ec9dac06499c9535b4852defb192eb3) )
	ROM_LOAD16_BYTE( "1031-10e.077", 0x000001, 0x010000, CRC(2ae2ac35) SHA1(e2a050a6e24dc6fef86b3556b69b2c3e2993febc) )

	ROM_REGION( 0x50000, REGION_SOUND1, 0 )		/* 10*128k for audio serial ROMs */
	ROM_LOAD( "1123-65a.052", 0x000000, 0x010000, CRC(a88411dc) SHA1(1fd53c7eadffa163d5423df2f8338757e58d5f2e) )
	ROM_LOAD( "1124-55a.052", 0x010000, 0x010000, CRC(071a4309) SHA1(c623bd51d6a4a56503fbf138138854d6a30b11d6) )
	ROM_LOAD( "3125-45a.052", 0x020000, 0x010000, CRC(856548ff) SHA1(e8a17b274185c5e4ecf5f9f1c211e18b3ef2456d) )
	ROM_LOAD( "1126-30a.052", 0x030000, 0x010000, CRC(f46ef09c) SHA1(ba62f73ee3b33d8f26b430ffa468f8792dca23de) )
	ROM_LOAD( "1017-45c.077", 0x040000, 0x010000, CRC(e93129a3) SHA1(1221b08c8efbfd8cf6bfbfd956545f10bef48663) )
ROM_END


ROM_START( racedrv1 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 1MB for 68000 code */
	ROM_LOAD16_BYTE( "1002200r.077", 0x000000, 0x010000, CRC(d39f6d53) SHA1(c278cd4109a65304a21dd1d8ba27375713a8b107) )
	ROM_LOAD16_BYTE( "1001210r.077", 0x000001, 0x010000, CRC(e29e011c) SHA1(7044e7f9c8551309b5867a06672796411007a427) )
	ROM_LOAD16_BYTE( "1004200s.077", 0x020000, 0x010000, CRC(809831e8) SHA1(681160deceff4df0f000c23f8514086dbbcc0c8d) )
	ROM_LOAD16_BYTE( "1003210s.077", 0x020001, 0x010000, CRC(4e2faf63) SHA1(a1a0b151e5e871bac0624b2def87f35ad2b5d758) )
	ROM_LOAD16_BYTE( "1006200t.077", 0x040000, 0x010000, CRC(5615561b) SHA1(6b8c69a9a4151245ff830d035e3b57701da43392) )
	ROM_LOAD16_BYTE( "1005210t.077", 0x040001, 0x010000, CRC(b115b9f5) SHA1(af19692b34f77f4fdd1d905315bfd52bb0c6d559) )
	ROM_LOAD16_BYTE( "1008200u.077", 0x060000, 0x010000, CRC(5144d31b) SHA1(5d5b05554d5e0c2f58196834c2445ed48a729df7) )
	ROM_LOAD16_BYTE( "1007210u.077", 0x060001, 0x010000, CRC(17903148) SHA1(85001910c0e7f7fb5cef3fe989ef27c0a0b7003e) )
	ROM_LOAD16_BYTE( "1010200v.077", 0x080000, 0x010000, CRC(346fafea) SHA1(2e73ccf351e7bd447a4b93c96de644606ac14cc8) )
	ROM_LOAD16_BYTE( "1009210v.077", 0x080001, 0x010000, CRC(98e95bc8) SHA1(73517ae15fafb8f861bb81021a2e92143827747d) )
	ROM_LOAD16_BYTE( "1012200w.077", 0x0a0000, 0x010000, CRC(9a78b952) SHA1(53270d4d8c28579ebda477a63c034f6d1b9e5a58) )
	ROM_LOAD16_BYTE( "1011210w.077", 0x0a0001, 0x010000, CRC(c5cd5491) SHA1(ede5a3bb888342032d6758b0fb149451b6543d8b) )
	ROM_LOAD16_BYTE( "1014200x.077", 0x0c0000, 0x010000, CRC(a872792a) SHA1(9269e397567940013e5f46cadfe2bad2ca1a2bc4) )
	ROM_LOAD16_BYTE( "1013210x.077", 0x0c0001, 0x010000, CRC(ca7b3e53) SHA1(cdb3a6360893fd3dd2947c050dca8a4dfaa9ced9) )
	ROM_LOAD16_BYTE( "1016200y.077", 0x0e0000, 0x010000, CRC(e83a9c99) SHA1(1d4093902133bb6da981f294e6947544c3564393) )
	ROM_LOAD16_BYTE( "1015210y.077", 0x0e0001, 0x010000, CRC(c51f2702) SHA1(2279f15c4c09af92fe9b87dc0ed842092ca64906) )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU3, 0 )	/* region for ADSP 2100 */

	ROM_REGION( 0x20000, REGION_CPU5, 0 )		/* 2*64k for audio 68000 code */
	ROM_LOAD16_BYTE( "1032-70n.077", 0x000000, 0x008000, CRC(fbc903a6) SHA1(047240a5192918ced52d90b0555ba2e19a26329e) )
	ROM_LOAD16_BYTE( "1033-45n.077", 0x000001, 0x008000, CRC(e032ca7c) SHA1(a80c980c8e58cf8cada72140e42a3cd1ea987b3d) )

	ROM_REGION( 0x10000, REGION_CPU6, 0 )		/* region for audio 32010 */

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )	/* 384k for ADSP object ROM */
	ROM_LOAD16_BYTE( "1021-10h.077", 0x000000, 0x010000, CRC(9831fe73) SHA1(5215ee89a0014399b7d829e443b16590a4679844) )
	ROM_LOAD16_BYTE( "1023-10k.077", 0x000001, 0x010000, CRC(74bf0e0e) SHA1(a3d91ecf13c3619e073324517a4a5afaae831982) )
	ROM_LOAD16_BYTE( "1022-10j.077", 0x020000, 0x010000, CRC(c0393c31) SHA1(31726c01eb0d4650936908c90d45161197b7efba) )
	ROM_LOAD16_BYTE( "1024-10l.077", 0x020001, 0x010000, CRC(1e2fb25f) SHA1(4940091bbad6144bce091d2737191d266d4b0310) )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for ADSP I/O buffers */

	ROM_REGION16_BE( 0x51000, REGION_USER3, 0 )	/* 256k for DSK ROMs + 64k for RAM + 4k for ZRAM */
	ROM_LOAD16_BYTE( "1030-30e.077", 0x000000, 0x010000, CRC(ff575b3d) SHA1(7f2202db7ec9dac06499c9535b4852defb192eb3) )
	ROM_LOAD16_BYTE( "1031-10e.077", 0x000001, 0x010000, CRC(2ae2ac35) SHA1(e2a050a6e24dc6fef86b3556b69b2c3e2993febc) )

	ROM_REGION( 0x50000, REGION_SOUND1, 0 )		/* 10*128k for audio serial ROMs */
	ROM_LOAD( "1123-65a.052", 0x000000, 0x010000, CRC(a88411dc) SHA1(1fd53c7eadffa163d5423df2f8338757e58d5f2e) )
	ROM_LOAD( "1124-55a.052", 0x010000, 0x010000, CRC(071a4309) SHA1(c623bd51d6a4a56503fbf138138854d6a30b11d6) )
	ROM_LOAD( "3125-45a.052", 0x020000, 0x010000, CRC(856548ff) SHA1(e8a17b274185c5e4ecf5f9f1c211e18b3ef2456d) )
	ROM_LOAD( "1126-30a.052", 0x030000, 0x010000, CRC(f46ef09c) SHA1(ba62f73ee3b33d8f26b430ffa468f8792dca23de) )
	ROM_LOAD( "1017-45c.077", 0x040000, 0x010000, CRC(e93129a3) SHA1(1221b08c8efbfd8cf6bfbfd956545f10bef48663) )
ROM_END


ROM_START( racedrg1 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 1MB for 68000 code */
	ROM_LOAD16_BYTE( "1202200r.077", 0x000000, 0x010000, CRC(bd363903) SHA1(8f0937725adabf5ee9e0ac015c83916e2575670a) )
	ROM_LOAD16_BYTE( "1201210r.077", 0x000001, 0x010000, CRC(748c3be4) SHA1(ddf149f333d23e9c782ae447f6fb3f780680b3f5) )
	ROM_LOAD16_BYTE( "1204200s.077", 0x020000, 0x010000, CRC(aef103bb) SHA1(52831858d3a45ad00e156c5b2a11e7fddf06bead) )
	ROM_LOAD16_BYTE( "1203210s.077", 0x020001, 0x010000, CRC(b0dddd6d) SHA1(78a3a47f29c7f0bf74127b73593f02a515fc1b71) )
	ROM_LOAD16_BYTE( "1206200t.077", 0x040000, 0x010000, CRC(d4a70ce6) SHA1(dce613d6993daafe3ae6578a6a32dd96516dc976) )
	ROM_LOAD16_BYTE( "1205210t.077", 0x040001, 0x010000, CRC(1d85766b) SHA1(91d9ff9412980fce36b48f82ce4f9acbd8d439db) )
	ROM_LOAD16_BYTE( "1208200u.077", 0x060000, 0x010000, CRC(5144d31b) SHA1(5d5b05554d5e0c2f58196834c2445ed48a729df7) )
	ROM_LOAD16_BYTE( "1207210u.077", 0x060001, 0x010000, CRC(17903148) SHA1(85001910c0e7f7fb5cef3fe989ef27c0a0b7003e) )
	ROM_LOAD16_BYTE( "1210200v.077", 0x080000, 0x010000, CRC(8674e44e) SHA1(5a81b93f6ccb3f92fdebb6500051561cb1d963dd) )
	ROM_LOAD16_BYTE( "1209210v.077", 0x080001, 0x010000, CRC(1e9e4c31) SHA1(ec77d1b181cf3268f606a513dc5103e6bb311a68) )
	ROM_LOAD16_BYTE( "1012200w.077", 0x0a0000, 0x010000, CRC(9a78b952) SHA1(53270d4d8c28579ebda477a63c034f6d1b9e5a58) )
	ROM_LOAD16_BYTE( "1011210w.077", 0x0a0001, 0x010000, CRC(c5cd5491) SHA1(ede5a3bb888342032d6758b0fb149451b6543d8b) )
	ROM_LOAD16_BYTE( "1014200x.077", 0x0c0000, 0x010000, CRC(a872792a) SHA1(9269e397567940013e5f46cadfe2bad2ca1a2bc4) )
	ROM_LOAD16_BYTE( "1013210x.077", 0x0c0001, 0x010000, CRC(ca7b3e53) SHA1(cdb3a6360893fd3dd2947c050dca8a4dfaa9ced9) )
	ROM_LOAD16_BYTE( "1016200y.077", 0x0e0000, 0x010000, CRC(e83a9c99) SHA1(1d4093902133bb6da981f294e6947544c3564393) )
	ROM_LOAD16_BYTE( "1015210y.077", 0x0e0001, 0x010000, CRC(c51f2702) SHA1(2279f15c4c09af92fe9b87dc0ed842092ca64906) )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU3, 0 )	/* region for ADSP 2100 */

	ROM_REGION( 0x20000, REGION_CPU5, 0 )		/* 2*64k for audio 68000 code */
	ROM_LOAD16_BYTE( "1032-70n.077", 0x000000, 0x008000, CRC(fbc903a6) SHA1(047240a5192918ced52d90b0555ba2e19a26329e) )
	ROM_LOAD16_BYTE( "1033-45n.077", 0x000001, 0x008000, CRC(e032ca7c) SHA1(a80c980c8e58cf8cada72140e42a3cd1ea987b3d) )

	ROM_REGION( 0x10000, REGION_CPU6, 0 )		/* region for audio 32010 */

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )	/* 384k for ADSP object ROM */
	ROM_LOAD16_BYTE( "2021-10h.077", 0x000000, 0x010000, CRC(8b2a98da) SHA1(264b7ec218e423ea85c54e586f8ff091f033d472) )
	ROM_LOAD16_BYTE( "2023-10k.077", 0x000001, 0x010000, CRC(c6d83d38) SHA1(e42c186a7fc0d88982b26eafdb834406b4ed3c8a) )
	ROM_LOAD16_BYTE( "1022-10j.077", 0x020000, 0x010000, CRC(c0393c31) SHA1(31726c01eb0d4650936908c90d45161197b7efba) )
	ROM_LOAD16_BYTE( "1024-10l.077", 0x020001, 0x010000, CRC(1e2fb25f) SHA1(4940091bbad6144bce091d2737191d266d4b0310) )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for ADSP I/O buffers */

	ROM_REGION16_BE( 0x51000, REGION_USER3, 0 )	/* 256k for DSK ROMs + 64k for RAM + 4k for ZRAM */
	ROM_LOAD16_BYTE( "1030-30e.077", 0x000000, 0x010000, CRC(ff575b3d) SHA1(7f2202db7ec9dac06499c9535b4852defb192eb3) )
	ROM_LOAD16_BYTE( "1031-10e.077", 0x000001, 0x010000, CRC(2ae2ac35) SHA1(e2a050a6e24dc6fef86b3556b69b2c3e2993febc) )

	ROM_REGION( 0x50000, REGION_SOUND1, 0 )		/* 10*128k for audio serial ROMs */
	ROM_LOAD( "1123-65a.052", 0x000000, 0x010000, CRC(a88411dc) SHA1(1fd53c7eadffa163d5423df2f8338757e58d5f2e) )
	ROM_LOAD( "1124-55a.052", 0x010000, 0x010000, CRC(071a4309) SHA1(c623bd51d6a4a56503fbf138138854d6a30b11d6) )
	ROM_LOAD( "3125-45a.052", 0x020000, 0x010000, CRC(856548ff) SHA1(e8a17b274185c5e4ecf5f9f1c211e18b3ef2456d) )
	ROM_LOAD( "1126-30a.052", 0x030000, 0x010000, CRC(f46ef09c) SHA1(ba62f73ee3b33d8f26b430ffa468f8792dca23de) )
	ROM_LOAD( "1017-45c.077", 0x040000, 0x010000, CRC(e93129a3) SHA1(1221b08c8efbfd8cf6bfbfd956545f10bef48663) )
ROM_END


ROM_START( racedrb1 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 1MB for 68000 code */
	ROM_LOAD16_BYTE( "2002200r.077", 0x000000, 0x010000, CRC(6b440e10) SHA1(8074a1de01e2cdc497a51339f9c05329d6f540b0) )
	ROM_LOAD16_BYTE( "2001210r.077", 0x000001, 0x010000, CRC(eee12973) SHA1(0cd2e88c431d08fba5a73c903327f39f8692f3aa) )
	ROM_LOAD16_BYTE( "1104200s.077", 0x020000, 0x010000, CRC(a9495f53) SHA1(68e7d983c0e1a84f5ad0743a230e6126169a6fd6) )
	ROM_LOAD16_BYTE( "1103210s.077", 0x020001, 0x010000, CRC(3c5c853c) SHA1(cbf6a4827889896d63671ed76e69e2f0dfcda2ef) )
	ROM_LOAD16_BYTE( "1106200t.077", 0x040000, 0x010000, CRC(5615561b) SHA1(6b8c69a9a4151245ff830d035e3b57701da43392) )
	ROM_LOAD16_BYTE( "1105210t.077", 0x040001, 0x010000, CRC(b115b9f5) SHA1(af19692b34f77f4fdd1d905315bfd52bb0c6d559) )
	ROM_LOAD16_BYTE( "1008200u.077", 0x060000, 0x010000, CRC(5144d31b) SHA1(5d5b05554d5e0c2f58196834c2445ed48a729df7) )
	ROM_LOAD16_BYTE( "1007210u.077", 0x060001, 0x010000, CRC(17903148) SHA1(85001910c0e7f7fb5cef3fe989ef27c0a0b7003e) )
	ROM_LOAD16_BYTE( "1010200v.077", 0x080000, 0x010000, CRC(346fafea) SHA1(2e73ccf351e7bd447a4b93c96de644606ac14cc8) )
	ROM_LOAD16_BYTE( "1009210v.077", 0x080001, 0x010000, CRC(98e95bc8) SHA1(73517ae15fafb8f861bb81021a2e92143827747d) )
	ROM_LOAD16_BYTE( "1112200w.077", 0x0a0000, 0x010000, CRC(5b5a2527) SHA1(a5762b4f9d0abbc9a54b1ac3014ae69394c7e03e) )
	ROM_LOAD16_BYTE( "1111210w.077", 0x0a0001, 0x010000, CRC(1f6224ec) SHA1(54d91406629dc64c9458d73e73e7c9fff6dbd915) )
	ROM_LOAD16_BYTE( "1014200x.077", 0x0c0000, 0x010000, CRC(a872792a) SHA1(9269e397567940013e5f46cadfe2bad2ca1a2bc4) )
	ROM_LOAD16_BYTE( "1013210x.077", 0x0c0001, 0x010000, CRC(ca7b3e53) SHA1(cdb3a6360893fd3dd2947c050dca8a4dfaa9ced9) )
	ROM_LOAD16_BYTE( "1016200y.077", 0x0e0000, 0x010000, CRC(e83a9c99) SHA1(1d4093902133bb6da981f294e6947544c3564393) )
	ROM_LOAD16_BYTE( "1015210y.077", 0x0e0001, 0x010000, CRC(c51f2702) SHA1(2279f15c4c09af92fe9b87dc0ed842092ca64906) )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU3, 0 )	/* region for ADSP 2100 */

	ROM_REGION( 0x20000, REGION_CPU5, 0 )		/* 2*64k for audio 68000 code */
	ROM_LOAD16_BYTE( "1032-70n.077", 0x000000, 0x008000, CRC(fbc903a6) SHA1(047240a5192918ced52d90b0555ba2e19a26329e) )
	ROM_LOAD16_BYTE( "1033-45n.077", 0x000001, 0x008000, CRC(e032ca7c) SHA1(a80c980c8e58cf8cada72140e42a3cd1ea987b3d) )

	ROM_REGION( 0x10000, REGION_CPU6, 0 )		/* region for audio 32010 */

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )	/* 384k for ADSP object ROM */
	ROM_LOAD16_BYTE( "1021-10h.077", 0x000000, 0x010000, CRC(9831fe73) SHA1(5215ee89a0014399b7d829e443b16590a4679844) )
	ROM_LOAD16_BYTE( "1023-10k.077", 0x000001, 0x010000, CRC(74bf0e0e) SHA1(a3d91ecf13c3619e073324517a4a5afaae831982) )
	ROM_LOAD16_BYTE( "1022-10j.077", 0x020000, 0x010000, CRC(c0393c31) SHA1(31726c01eb0d4650936908c90d45161197b7efba) )
	ROM_LOAD16_BYTE( "1024-10l.077", 0x020001, 0x010000, CRC(1e2fb25f) SHA1(4940091bbad6144bce091d2737191d266d4b0310) )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for ADSP I/O buffers */

	ROM_REGION16_BE( 0x51000, REGION_USER3, 0 )	/* 256k for DSK ROMs + 64k for RAM + 4k for ZRAM */
	ROM_LOAD16_BYTE( "1030-30e.077", 0x000000, 0x010000, CRC(ff575b3d) SHA1(7f2202db7ec9dac06499c9535b4852defb192eb3) )
	ROM_LOAD16_BYTE( "1031-10e.077", 0x000001, 0x010000, CRC(2ae2ac35) SHA1(e2a050a6e24dc6fef86b3556b69b2c3e2993febc) )

	ROM_REGION( 0x50000, REGION_SOUND1, 0 )		/* 10*128k for audio serial ROMs */
	ROM_LOAD( "1123-65a.052", 0x000000, 0x010000, CRC(a88411dc) SHA1(1fd53c7eadffa163d5423df2f8338757e58d5f2e) )
	ROM_LOAD( "1124-55a.052", 0x010000, 0x010000, CRC(071a4309) SHA1(c623bd51d6a4a56503fbf138138854d6a30b11d6) )
	ROM_LOAD( "3125-45a.052", 0x020000, 0x010000, CRC(856548ff) SHA1(e8a17b274185c5e4ecf5f9f1c211e18b3ef2456d) )
	ROM_LOAD( "1126-30a.052", 0x030000, 0x010000, CRC(f46ef09c) SHA1(ba62f73ee3b33d8f26b430ffa468f8792dca23de) )
	ROM_LOAD( "1017-45c.077", 0x040000, 0x010000, CRC(e93129a3) SHA1(1221b08c8efbfd8cf6bfbfd956545f10bef48663) )
ROM_END


ROM_START( racedrvc )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 1MB for 68000 code */
	ROM_LOAD16_BYTE( "5002200r.078", 0x000000, 0x010000, CRC(ae8f79bd) SHA1(054f2d1948d6d03832c4557c3dd7667c0784716d) )
	ROM_LOAD16_BYTE( "5001210r.078", 0x000001, 0x010000, CRC(ad896e7f) SHA1(7b047f0c972fac075db00faf19cdf9ccdcef5c63) )
	ROM_LOAD16_BYTE( "5004200s.078", 0x020000, 0x010000, CRC(b8650bee) SHA1(a9905081e622e4a4fcec9aa08b87566ecdb9198b) )
	ROM_LOAD16_BYTE( "5003210s.078", 0x020001, 0x010000, CRC(bf56a29f) SHA1(1ef17dab9ed1fd92b005dd31f7dccf1535583ec4) )
	ROM_LOAD16_BYTE( "5006200t.078", 0x040000, 0x010000, CRC(8559a61b) SHA1(ad1796c39ac9cbb50e8a97d9e8c9cd5adfd0ff5e) )
	ROM_LOAD16_BYTE( "5005210t.078", 0x040001, 0x010000, CRC(33d654b1) SHA1(1e6886d49008b13886184c9ced0f6bd642d9562c) )
	ROM_LOAD16_BYTE( "4008200u.078", 0x060000, 0x010000, CRC(5b119d16) SHA1(ec808b5e325ab1a6dbddcef75871cd0126a98377) )
	ROM_LOAD16_BYTE( "4007210u.078", 0x060001, 0x010000, CRC(fa234861) SHA1(4c6753c64f942f6ea98f96e1e8f4de880800573e) )
	ROM_LOAD16_BYTE( "4010200v.078", 0x080000, 0x010000, CRC(150acc9e) SHA1(e172e6a1ed5ac0791fe053d4bde2d0c62202f399) )
	ROM_LOAD16_BYTE( "4009210v.078", 0x080001, 0x010000, CRC(85a9aa37) SHA1(e403a41c44edfca190cb15452d67a427de9cf56e) )
	ROM_LOAD16_BYTE( "1012200w.077", 0x0a0000, 0x010000, CRC(9a78b952) SHA1(53270d4d8c28579ebda477a63c034f6d1b9e5a58) )
	ROM_LOAD16_BYTE( "1011210w.077", 0x0a0001, 0x010000, CRC(c5cd5491) SHA1(ede5a3bb888342032d6758b0fb149451b6543d8b) )
	ROM_LOAD16_BYTE( "1014200x.077", 0x0c0000, 0x010000, CRC(a872792a) SHA1(9269e397567940013e5f46cadfe2bad2ca1a2bc4) )
	ROM_LOAD16_BYTE( "1013210x.077", 0x0c0001, 0x010000, CRC(ca7b3e53) SHA1(cdb3a6360893fd3dd2947c050dca8a4dfaa9ced9) )
	ROM_LOAD16_BYTE( "4016200y.078", 0x0e0000, 0x010000, CRC(aa296776) SHA1(991e33568e47a82cb183724f7df1f835850c30fa) )
	ROM_LOAD16_BYTE( "4015210y.078", 0x0e0001, 0x010000, CRC(030c803d) SHA1(bb249c9d1d28f6851a529b93c4f96d80665f2bce) )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU3, 0 )	/* region for ADSP 2100 */

	ROM_REGION( 0x20000, REGION_CPU5, 0 )		/* 2*64k for audio 68000 code */
	ROM_LOAD16_BYTE( "1032-70n.077", 0x000000, 0x008000, CRC(fbc903a6) SHA1(047240a5192918ced52d90b0555ba2e19a26329e) )
	ROM_LOAD16_BYTE( "1033-45n.077", 0x000001, 0x008000, CRC(e032ca7c) SHA1(a80c980c8e58cf8cada72140e42a3cd1ea987b3d) )

	ROM_REGION( 0x10000, REGION_CPU6, 0 )		/* region for audio 32010 */

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )	/* 384k for ADSP object ROM */
	ROM_LOAD16_BYTE( "1021-10h.077", 0x000000, 0x010000, CRC(9831fe73) SHA1(5215ee89a0014399b7d829e443b16590a4679844) )
	ROM_LOAD16_BYTE( "1023-10k.077", 0x000001, 0x010000, CRC(74bf0e0e) SHA1(a3d91ecf13c3619e073324517a4a5afaae831982) )
	ROM_LOAD16_BYTE( "1022-10j.077", 0x020000, 0x010000, CRC(c0393c31) SHA1(31726c01eb0d4650936908c90d45161197b7efba) )
	ROM_LOAD16_BYTE( "1024-10l.077", 0x020001, 0x010000, CRC(1e2fb25f) SHA1(4940091bbad6144bce091d2737191d266d4b0310) )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for ADSP I/O buffers */

	ROM_REGION16_BE( 0x51000, REGION_USER3, 0 )	/* 256k for DSK ROMs + 64k for RAM + 4k for ZRAM */
	ROM_LOAD16_BYTE( "1030-30e.078", 0x000000, 0x010000, CRC(d355a1c2) SHA1(6b0fb52dbecbe574959739f49c3c0fccdd5cebdb) )
	ROM_LOAD16_BYTE( "1031-10e.078", 0x000001, 0x010000, CRC(18fd5f44) SHA1(1c3bd780b72cfa61bcbd82683da18b4ee8d03a6d) )

	ROM_REGION( 0x50000, REGION_SOUND1, 0 )		/* 10*128k for audio serial ROMs */
	ROM_LOAD( "1123-65a.052", 0x000000, 0x010000, CRC(a88411dc) SHA1(1fd53c7eadffa163d5423df2f8338757e58d5f2e) )
	ROM_LOAD( "1124-55a.052", 0x010000, 0x010000, CRC(071a4309) SHA1(c623bd51d6a4a56503fbf138138854d6a30b11d6) )
	ROM_LOAD( "3125-45a.052", 0x020000, 0x010000, CRC(856548ff) SHA1(e8a17b274185c5e4ecf5f9f1c211e18b3ef2456d) )
	ROM_LOAD( "1126-30a.052", 0x030000, 0x010000, CRC(f46ef09c) SHA1(ba62f73ee3b33d8f26b430ffa468f8792dca23de) )
	ROM_LOAD( "1017-45c.077", 0x040000, 0x010000, CRC(e93129a3) SHA1(1221b08c8efbfd8cf6bfbfd956545f10bef48663) )
ROM_END


ROM_START( racedrcb )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 1MB for 68000 code */
	ROM_LOAD16_BYTE( "5002200r.078", 0x000000, 0x010000, CRC(ae8f79bd) SHA1(054f2d1948d6d03832c4557c3dd7667c0784716d) )
	ROM_LOAD16_BYTE( "5001210r.078", 0x000001, 0x010000, CRC(ad896e7f) SHA1(7b047f0c972fac075db00faf19cdf9ccdcef5c63) )
	ROM_LOAD16_BYTE( "5004200s.078", 0x020000, 0x010000, CRC(b8650bee) SHA1(a9905081e622e4a4fcec9aa08b87566ecdb9198b) )
	ROM_LOAD16_BYTE( "5003210s.078", 0x020001, 0x010000, CRC(bf56a29f) SHA1(1ef17dab9ed1fd92b005dd31f7dccf1535583ec4) )
	ROM_LOAD16_BYTE( "5006200t.078", 0x040000, 0x010000, CRC(8559a61b) SHA1(ad1796c39ac9cbb50e8a97d9e8c9cd5adfd0ff5e) )
	ROM_LOAD16_BYTE( "5005210t.078", 0x040001, 0x010000, CRC(33d654b1) SHA1(1e6886d49008b13886184c9ced0f6bd642d9562c) )
	ROM_LOAD16_BYTE( "4008200u.078", 0x060000, 0x010000, CRC(5b119d16) SHA1(ec808b5e325ab1a6dbddcef75871cd0126a98377) )
	ROM_LOAD16_BYTE( "4007210u.078", 0x060001, 0x010000, CRC(fa234861) SHA1(4c6753c64f942f6ea98f96e1e8f4de880800573e) )
	ROM_LOAD16_BYTE( "4010200v.078", 0x080000, 0x010000, CRC(150acc9e) SHA1(e172e6a1ed5ac0791fe053d4bde2d0c62202f399) )
	ROM_LOAD16_BYTE( "4009210v.078", 0x080001, 0x010000, CRC(85a9aa37) SHA1(e403a41c44edfca190cb15452d67a427de9cf56e) )
	ROM_LOAD16_BYTE( "1112200w.077", 0x0a0000, 0x010000, CRC(5b5a2527) SHA1(a5762b4f9d0abbc9a54b1ac3014ae69394c7e03e) )
	ROM_LOAD16_BYTE( "1111210w.077", 0x0a0001, 0x010000, CRC(1f6224ec) SHA1(54d91406629dc64c9458d73e73e7c9fff6dbd915) )
	ROM_LOAD16_BYTE( "1014200x.077", 0x0c0000, 0x010000, CRC(a872792a) SHA1(9269e397567940013e5f46cadfe2bad2ca1a2bc4) )
	ROM_LOAD16_BYTE( "1013210x.077", 0x0c0001, 0x010000, CRC(ca7b3e53) SHA1(cdb3a6360893fd3dd2947c050dca8a4dfaa9ced9) )
	ROM_LOAD16_BYTE( "4016200y.078", 0x0e0000, 0x010000, CRC(aa296776) SHA1(991e33568e47a82cb183724f7df1f835850c30fa) )
	ROM_LOAD16_BYTE( "4015210y.078", 0x0e0001, 0x010000, CRC(030c803d) SHA1(bb249c9d1d28f6851a529b93c4f96d80665f2bce) )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU3, 0 )	/* region for ADSP 2100 */

	ROM_REGION( 0x20000, REGION_CPU5, 0 )		/* 2*64k for audio 68000 code */
	ROM_LOAD16_BYTE( "1032-70n.077", 0x000000, 0x008000, CRC(fbc903a6) SHA1(047240a5192918ced52d90b0555ba2e19a26329e) )
	ROM_LOAD16_BYTE( "1033-45n.077", 0x000001, 0x008000, CRC(e032ca7c) SHA1(a80c980c8e58cf8cada72140e42a3cd1ea987b3d) )

	ROM_REGION( 0x10000, REGION_CPU6, 0 )		/* region for audio 32010 */

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )	/* 384k for ADSP object ROM */
	ROM_LOAD16_BYTE( "1021-10h.077", 0x000000, 0x010000, CRC(9831fe73) SHA1(5215ee89a0014399b7d829e443b16590a4679844) )
	ROM_LOAD16_BYTE( "1023-10k.077", 0x000001, 0x010000, CRC(74bf0e0e) SHA1(a3d91ecf13c3619e073324517a4a5afaae831982) )
	ROM_LOAD16_BYTE( "1022-10j.077", 0x020000, 0x010000, CRC(c0393c31) SHA1(31726c01eb0d4650936908c90d45161197b7efba) )
	ROM_LOAD16_BYTE( "1024-10l.077", 0x020001, 0x010000, CRC(1e2fb25f) SHA1(4940091bbad6144bce091d2737191d266d4b0310) )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for ADSP I/O buffers */

	ROM_REGION16_BE( 0x51000, REGION_USER3, 0 )	/* 256k for DSK ROMs + 64k for RAM + 4k for ZRAM */
	ROM_LOAD16_BYTE( "1030-30e.078", 0x000000, 0x010000, CRC(d355a1c2) SHA1(6b0fb52dbecbe574959739f49c3c0fccdd5cebdb) )
	ROM_LOAD16_BYTE( "1031-10e.078", 0x000001, 0x010000, CRC(18fd5f44) SHA1(1c3bd780b72cfa61bcbd82683da18b4ee8d03a6d) )

	ROM_REGION( 0x50000, REGION_SOUND1, 0 )		/* 10*128k for audio serial ROMs */
	ROM_LOAD( "1123-65a.052", 0x000000, 0x010000, CRC(a88411dc) SHA1(1fd53c7eadffa163d5423df2f8338757e58d5f2e) )
	ROM_LOAD( "1124-55a.052", 0x010000, 0x010000, CRC(071a4309) SHA1(c623bd51d6a4a56503fbf138138854d6a30b11d6) )
	ROM_LOAD( "3125-45a.052", 0x020000, 0x010000, CRC(856548ff) SHA1(e8a17b274185c5e4ecf5f9f1c211e18b3ef2456d) )
	ROM_LOAD( "1126-30a.052", 0x030000, 0x010000, CRC(f46ef09c) SHA1(ba62f73ee3b33d8f26b430ffa468f8792dca23de) )
	ROM_LOAD( "1017-45c.077", 0x040000, 0x010000, CRC(e93129a3) SHA1(1221b08c8efbfd8cf6bfbfd956545f10bef48663) )
ROM_END


ROM_START( racedrcg )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 1MB for 68000 code */
	ROM_LOAD16_BYTE( "5202200r.078", 0x000000, 0x010000, CRC(c37b5281) SHA1(1ff7a879853dd1e10881fd56e8ae0962676ef319) )
	ROM_LOAD16_BYTE( "5201210r.078", 0x000001, 0x010000, CRC(80072dad) SHA1(ba33465f639431cf6e9ce377ed31e538a0304dc5) )
	ROM_LOAD16_BYTE( "5204200s.078", 0x020000, 0x010000, CRC(5d45fbd2) SHA1(5279c814c2876c642630fed5c114eccea6d703b0) )
	ROM_LOAD16_BYTE( "5203210s.078", 0x020001, 0x010000, CRC(b0d4d55f) SHA1(ec4d7b43b981d5ac4db96c3c063f456c0799b577) )
	ROM_LOAD16_BYTE( "5206200t.078", 0x040000, 0x010000, CRC(df7a1c87) SHA1(31ef0583c3f68a951f05ae054512ecfa4a3696bd) )
	ROM_LOAD16_BYTE( "5205210t.078", 0x040001, 0x010000, CRC(3e209cc6) SHA1(54649720e9edf15ed59c37f54cfcfb92f1498b0e) )
	ROM_LOAD16_BYTE( "4008200u.078", 0x060000, 0x010000, CRC(5b119d16) SHA1(ec808b5e325ab1a6dbddcef75871cd0126a98377) )
	ROM_LOAD16_BYTE( "4007210u.078", 0x060001, 0x010000, CRC(fa234861) SHA1(4c6753c64f942f6ea98f96e1e8f4de880800573e) )
	ROM_LOAD16_BYTE( "4010200v.078", 0x080000, 0x010000, CRC(150acc9e) SHA1(e172e6a1ed5ac0791fe053d4bde2d0c62202f399) )
	ROM_LOAD16_BYTE( "4009210v.078", 0x080001, 0x010000, CRC(85a9aa37) SHA1(e403a41c44edfca190cb15452d67a427de9cf56e) )
	ROM_LOAD16_BYTE( "1012200w.077", 0x0a0000, 0x010000, CRC(9a78b952) SHA1(53270d4d8c28579ebda477a63c034f6d1b9e5a58) )
	ROM_LOAD16_BYTE( "1011210w.077", 0x0a0001, 0x010000, CRC(c5cd5491) SHA1(ede5a3bb888342032d6758b0fb149451b6543d8b) )
	ROM_LOAD16_BYTE( "1014200x.077", 0x0c0000, 0x010000, CRC(a872792a) SHA1(9269e397567940013e5f46cadfe2bad2ca1a2bc4) )
	ROM_LOAD16_BYTE( "1013210x.077", 0x0c0001, 0x010000, CRC(ca7b3e53) SHA1(cdb3a6360893fd3dd2947c050dca8a4dfaa9ced9) )
	ROM_LOAD16_BYTE( "4016200y.078", 0x0e0000, 0x010000, CRC(aa296776) SHA1(991e33568e47a82cb183724f7df1f835850c30fa) )
	ROM_LOAD16_BYTE( "4015210y.078", 0x0e0001, 0x010000, CRC(030c803d) SHA1(bb249c9d1d28f6851a529b93c4f96d80665f2bce) )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU3, 0 )	/* region for ADSP 2100 */

	ROM_REGION( 0x20000, REGION_CPU5, 0 )		/* 2*64k for audio 68000 code */
	ROM_LOAD16_BYTE( "1032-70n.077", 0x000000, 0x008000, CRC(fbc903a6) SHA1(047240a5192918ced52d90b0555ba2e19a26329e) )
	ROM_LOAD16_BYTE( "1033-45n.077", 0x000001, 0x008000, CRC(e032ca7c) SHA1(a80c980c8e58cf8cada72140e42a3cd1ea987b3d) )

	ROM_REGION( 0x10000, REGION_CPU6, 0 )		/* region for audio 32010 */

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )	/* 384k for ADSP object ROM */
	ROM_LOAD16_BYTE( "1021-10h.077", 0x000000, 0x010000, CRC(9831fe73) SHA1(5215ee89a0014399b7d829e443b16590a4679844) )
	ROM_LOAD16_BYTE( "1023-10k.077", 0x000001, 0x010000, CRC(74bf0e0e) SHA1(a3d91ecf13c3619e073324517a4a5afaae831982) )
	ROM_LOAD16_BYTE( "1022-10j.077", 0x020000, 0x010000, CRC(c0393c31) SHA1(31726c01eb0d4650936908c90d45161197b7efba) )
	ROM_LOAD16_BYTE( "1024-10l.077", 0x020001, 0x010000, CRC(1e2fb25f) SHA1(4940091bbad6144bce091d2737191d266d4b0310) )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for ADSP I/O buffers */

	ROM_REGION16_BE( 0x51000, REGION_USER3, 0 )	/* 256k for DSK ROMs + 64k for RAM + 4k for ZRAM */
	ROM_LOAD16_BYTE( "1030-30e.078", 0x000000, 0x010000, CRC(d355a1c2) SHA1(6b0fb52dbecbe574959739f49c3c0fccdd5cebdb) )
	ROM_LOAD16_BYTE( "1031-10e.078", 0x000001, 0x010000, CRC(18fd5f44) SHA1(1c3bd780b72cfa61bcbd82683da18b4ee8d03a6d) )

	ROM_REGION( 0x50000, REGION_SOUND1, 0 )		/* 10*128k for audio serial ROMs */
	ROM_LOAD( "1123-65a.052", 0x000000, 0x010000, CRC(a88411dc) SHA1(1fd53c7eadffa163d5423df2f8338757e58d5f2e) )
	ROM_LOAD( "1124-55a.052", 0x010000, 0x010000, CRC(071a4309) SHA1(c623bd51d6a4a56503fbf138138854d6a30b11d6) )
	ROM_LOAD( "3125-45a.052", 0x020000, 0x010000, CRC(856548ff) SHA1(e8a17b274185c5e4ecf5f9f1c211e18b3ef2456d) )
	ROM_LOAD( "1126-30a.052", 0x030000, 0x010000, CRC(f46ef09c) SHA1(ba62f73ee3b33d8f26b430ffa468f8792dca23de) )
	ROM_LOAD( "1017-45c.077", 0x040000, 0x010000, CRC(e93129a3) SHA1(1221b08c8efbfd8cf6bfbfd956545f10bef48663) )
ROM_END


ROM_START( racedrc4 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 1MB for 68000 code */
	ROM_LOAD16_BYTE( "4002200r.078", 0x000000, 0x010000, CRC(d6522b97) SHA1(0e4c4ce9f6147cc3878a3f1e60cf58f00bfe9e4c) )
	ROM_LOAD16_BYTE( "4001210r.078", 0x000001, 0x010000, CRC(d658a8ed) SHA1(59d823d8150de62ba5c4fddf87584bd2044e87f7) )
	ROM_LOAD16_BYTE( "4004200s.078", 0x020000, 0x010000, CRC(c8520b01) SHA1(f0bd0505cbf24f7e43dc2e775ecbbf0db5419b6e) )
	ROM_LOAD16_BYTE( "4003210s.078", 0x020001, 0x010000, CRC(2d1972ad) SHA1(1126b043b6ac68088decbf28c818725d3f26acc4) )
	ROM_LOAD16_BYTE( "4006200t.078", 0x040000, 0x010000, CRC(31f338c0) SHA1(19de0e4982e4b746e5b70622c889c63f14da5f4c) )
	ROM_LOAD16_BYTE( "4005210t.078", 0x040001, 0x010000, CRC(148c92bb) SHA1(1a7070ff7ff62143e086d39d821dca6a0c4d0c1b) )
	ROM_LOAD16_BYTE( "4008200u.078", 0x060000, 0x010000, CRC(5b119d16) SHA1(ec808b5e325ab1a6dbddcef75871cd0126a98377) )
	ROM_LOAD16_BYTE( "4007210u.078", 0x060001, 0x010000, CRC(fa234861) SHA1(4c6753c64f942f6ea98f96e1e8f4de880800573e) )
	ROM_LOAD16_BYTE( "4010200v.078", 0x080000, 0x010000, CRC(150acc9e) SHA1(e172e6a1ed5ac0791fe053d4bde2d0c62202f399) )
	ROM_LOAD16_BYTE( "4009210v.078", 0x080001, 0x010000, CRC(85a9aa37) SHA1(e403a41c44edfca190cb15452d67a427de9cf56e) )
	ROM_LOAD16_BYTE( "1012200w.077", 0x0a0000, 0x010000, CRC(9a78b952) SHA1(53270d4d8c28579ebda477a63c034f6d1b9e5a58) )
	ROM_LOAD16_BYTE( "1011210w.077", 0x0a0001, 0x010000, CRC(c5cd5491) SHA1(ede5a3bb888342032d6758b0fb149451b6543d8b) )
	ROM_LOAD16_BYTE( "1014200x.077", 0x0c0000, 0x010000, CRC(a872792a) SHA1(9269e397567940013e5f46cadfe2bad2ca1a2bc4) )
	ROM_LOAD16_BYTE( "1013210x.077", 0x0c0001, 0x010000, CRC(ca7b3e53) SHA1(cdb3a6360893fd3dd2947c050dca8a4dfaa9ced9) )
	ROM_LOAD16_BYTE( "4016200y.078", 0x0e0000, 0x010000, CRC(aa296776) SHA1(991e33568e47a82cb183724f7df1f835850c30fa) )
	ROM_LOAD16_BYTE( "4015210y.078", 0x0e0001, 0x010000, CRC(030c803d) SHA1(bb249c9d1d28f6851a529b93c4f96d80665f2bce) )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU3, 0 )	/* region for ADSP 2100 */

	ROM_REGION( 0x20000, REGION_CPU5, 0 )		/* 2*64k for audio 68000 code */
	ROM_LOAD16_BYTE( "1032-70n.077", 0x000000, 0x008000, CRC(fbc903a6) SHA1(047240a5192918ced52d90b0555ba2e19a26329e) )
	ROM_LOAD16_BYTE( "1033-45n.077", 0x000001, 0x008000, CRC(e032ca7c) SHA1(a80c980c8e58cf8cada72140e42a3cd1ea987b3d) )

	ROM_REGION( 0x10000, REGION_CPU6, 0 )		/* region for audio 32010 */

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )	/* 384k for ADSP object ROM */
	ROM_LOAD16_BYTE( "1021-10h.077", 0x000000, 0x010000, CRC(9831fe73) SHA1(5215ee89a0014399b7d829e443b16590a4679844) )
	ROM_LOAD16_BYTE( "1023-10k.077", 0x000001, 0x010000, CRC(74bf0e0e) SHA1(a3d91ecf13c3619e073324517a4a5afaae831982) )
	ROM_LOAD16_BYTE( "1022-10j.077", 0x020000, 0x010000, CRC(c0393c31) SHA1(31726c01eb0d4650936908c90d45161197b7efba) )
	ROM_LOAD16_BYTE( "1024-10l.077", 0x020001, 0x010000, CRC(1e2fb25f) SHA1(4940091bbad6144bce091d2737191d266d4b0310) )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for ADSP I/O buffers */

	ROM_REGION16_BE( 0x51000, REGION_USER3, 0 )	/* 256k for DSK ROMs + 64k for RAM + 4k for ZRAM */
	ROM_LOAD16_BYTE( "1030-30e.078", 0x000000, 0x010000, CRC(d355a1c2) SHA1(6b0fb52dbecbe574959739f49c3c0fccdd5cebdb) )
	ROM_LOAD16_BYTE( "1031-10e.078", 0x000001, 0x010000, CRC(18fd5f44) SHA1(1c3bd780b72cfa61bcbd82683da18b4ee8d03a6d) )

	ROM_REGION( 0x50000, REGION_SOUND1, 0 )		/* 10*128k for audio serial ROMs */
	ROM_LOAD( "1123-65a.052", 0x000000, 0x010000, CRC(a88411dc) SHA1(1fd53c7eadffa163d5423df2f8338757e58d5f2e) )
	ROM_LOAD( "1124-55a.052", 0x010000, 0x010000, CRC(071a4309) SHA1(c623bd51d6a4a56503fbf138138854d6a30b11d6) )
	ROM_LOAD( "3125-45a.052", 0x020000, 0x010000, CRC(856548ff) SHA1(e8a17b274185c5e4ecf5f9f1c211e18b3ef2456d) )
	ROM_LOAD( "1126-30a.052", 0x030000, 0x010000, CRC(f46ef09c) SHA1(ba62f73ee3b33d8f26b430ffa468f8792dca23de) )
	ROM_LOAD( "1017-45c.077", 0x040000, 0x010000, CRC(e93129a3) SHA1(1221b08c8efbfd8cf6bfbfd956545f10bef48663) )
ROM_END


ROM_START( racedcb4 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 1MB for 68000 code */
	ROM_LOAD16_BYTE( "4002200r.078", 0x000000, 0x010000, CRC(d6522b97) SHA1(0e4c4ce9f6147cc3878a3f1e60cf58f00bfe9e4c) )
	ROM_LOAD16_BYTE( "4001210r.078", 0x000001, 0x010000, CRC(d658a8ed) SHA1(59d823d8150de62ba5c4fddf87584bd2044e87f7) )
	ROM_LOAD16_BYTE( "4004200s.078", 0x020000, 0x010000, CRC(c8520b01) SHA1(f0bd0505cbf24f7e43dc2e775ecbbf0db5419b6e) )
	ROM_LOAD16_BYTE( "4003210s.078", 0x020001, 0x010000, CRC(2d1972ad) SHA1(1126b043b6ac68088decbf28c818725d3f26acc4) )
	ROM_LOAD16_BYTE( "4006200t.078", 0x040000, 0x010000, CRC(31f338c0) SHA1(19de0e4982e4b746e5b70622c889c63f14da5f4c) )
	ROM_LOAD16_BYTE( "4005210t.078", 0x040001, 0x010000, CRC(148c92bb) SHA1(1a7070ff7ff62143e086d39d821dca6a0c4d0c1b) )
	ROM_LOAD16_BYTE( "4008200u.078", 0x060000, 0x010000, CRC(5b119d16) SHA1(ec808b5e325ab1a6dbddcef75871cd0126a98377) )
	ROM_LOAD16_BYTE( "4007210u.078", 0x060001, 0x010000, CRC(fa234861) SHA1(4c6753c64f942f6ea98f96e1e8f4de880800573e) )
	ROM_LOAD16_BYTE( "4010200v.078", 0x080000, 0x010000, CRC(150acc9e) SHA1(e172e6a1ed5ac0791fe053d4bde2d0c62202f399) )
	ROM_LOAD16_BYTE( "4009210v.078", 0x080001, 0x010000, CRC(85a9aa37) SHA1(e403a41c44edfca190cb15452d67a427de9cf56e) )
	ROM_LOAD16_BYTE( "1112200w.077", 0x0a0000, 0x010000, CRC(5b5a2527) SHA1(a5762b4f9d0abbc9a54b1ac3014ae69394c7e03e) )
	ROM_LOAD16_BYTE( "1111210w.077", 0x0a0001, 0x010000, CRC(1f6224ec) SHA1(54d91406629dc64c9458d73e73e7c9fff6dbd915) )
	ROM_LOAD16_BYTE( "1014200x.077", 0x0c0000, 0x010000, CRC(a872792a) SHA1(9269e397567940013e5f46cadfe2bad2ca1a2bc4) )
	ROM_LOAD16_BYTE( "1013210x.077", 0x0c0001, 0x010000, CRC(ca7b3e53) SHA1(cdb3a6360893fd3dd2947c050dca8a4dfaa9ced9) )
	ROM_LOAD16_BYTE( "4016200y.078", 0x0e0000, 0x010000, CRC(aa296776) SHA1(991e33568e47a82cb183724f7df1f835850c30fa) )
	ROM_LOAD16_BYTE( "4015210y.078", 0x0e0001, 0x010000, CRC(030c803d) SHA1(bb249c9d1d28f6851a529b93c4f96d80665f2bce) )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU3, 0 )	/* region for ADSP 2100 */

	ROM_REGION( 0x20000, REGION_CPU5, 0 )		/* 2*64k for audio 68000 code */
	ROM_LOAD16_BYTE( "1032-70n.077", 0x000000, 0x008000, CRC(fbc903a6) SHA1(047240a5192918ced52d90b0555ba2e19a26329e) )
	ROM_LOAD16_BYTE( "1033-45n.077", 0x000001, 0x008000, CRC(e032ca7c) SHA1(a80c980c8e58cf8cada72140e42a3cd1ea987b3d) )

	ROM_REGION( 0x10000, REGION_CPU6, 0 )		/* region for audio 32010 */

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )	/* 384k for ADSP object ROM */
	ROM_LOAD16_BYTE( "1021-10h.077", 0x000000, 0x010000, CRC(9831fe73) SHA1(5215ee89a0014399b7d829e443b16590a4679844) )
	ROM_LOAD16_BYTE( "1023-10k.077", 0x000001, 0x010000, CRC(74bf0e0e) SHA1(a3d91ecf13c3619e073324517a4a5afaae831982) )
	ROM_LOAD16_BYTE( "1022-10j.077", 0x020000, 0x010000, CRC(c0393c31) SHA1(31726c01eb0d4650936908c90d45161197b7efba) )
	ROM_LOAD16_BYTE( "1024-10l.077", 0x020001, 0x010000, CRC(1e2fb25f) SHA1(4940091bbad6144bce091d2737191d266d4b0310) )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for ADSP I/O buffers */

	ROM_REGION16_BE( 0x51000, REGION_USER3, 0 )	/* 256k for DSK ROMs + 64k for RAM + 4k for ZRAM */
	ROM_LOAD16_BYTE( "1030-30e.078", 0x000000, 0x010000, CRC(d355a1c2) SHA1(6b0fb52dbecbe574959739f49c3c0fccdd5cebdb) )
	ROM_LOAD16_BYTE( "1031-10e.078", 0x000001, 0x010000, CRC(18fd5f44) SHA1(1c3bd780b72cfa61bcbd82683da18b4ee8d03a6d) )

	ROM_REGION( 0x50000, REGION_SOUND1, 0 )		/* 10*128k for audio serial ROMs */
	ROM_LOAD( "1123-65a.052", 0x000000, 0x010000, CRC(a88411dc) SHA1(1fd53c7eadffa163d5423df2f8338757e58d5f2e) )
	ROM_LOAD( "1124-55a.052", 0x010000, 0x010000, CRC(071a4309) SHA1(c623bd51d6a4a56503fbf138138854d6a30b11d6) )
	ROM_LOAD( "3125-45a.052", 0x020000, 0x010000, CRC(856548ff) SHA1(e8a17b274185c5e4ecf5f9f1c211e18b3ef2456d) )
	ROM_LOAD( "1126-30a.052", 0x030000, 0x010000, CRC(f46ef09c) SHA1(ba62f73ee3b33d8f26b430ffa468f8792dca23de) )
	ROM_LOAD( "1017-45c.077", 0x040000, 0x010000, CRC(e93129a3) SHA1(1221b08c8efbfd8cf6bfbfd956545f10bef48663) )
ROM_END


ROM_START( racedcg4 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 1MB for 68000 code */
	ROM_LOAD16_BYTE( "4202200r.078", 0x000000, 0x010000, CRC(be58215d) SHA1(0497efbd696e8d2db4e42678dd6cfe6cd3f78688) )
	ROM_LOAD16_BYTE( "4201210r.078", 0x000001, 0x010000, CRC(609c5f23) SHA1(a66e1f10f8ad9d25fca5b681ad543967287f2036) )
	ROM_LOAD16_BYTE( "4204200s.078", 0x020000, 0x010000, CRC(2e590c4f) SHA1(dcb56bacdde17eade5b347d5549cec1de89774e6) )
	ROM_LOAD16_BYTE( "4203210s.078", 0x020001, 0x010000, CRC(f1ea50ac) SHA1(ac27f95df42e0e9d33dc99a3fa135189b590e375) )
	ROM_LOAD16_BYTE( "4206200t.078", 0x040000, 0x010000, CRC(aef8c898) SHA1(323a4253dd6e4e72a2a79b65d4fb850ec19e3150) )
	ROM_LOAD16_BYTE( "4205210t.078", 0x040001, 0x010000, CRC(a2603e7e) SHA1(8b1b46a3367fee2a55afab7e7c24da29670821ec) )
	ROM_LOAD16_BYTE( "4008200u.078", 0x060000, 0x010000, CRC(5b119d16) SHA1(ec808b5e325ab1a6dbddcef75871cd0126a98377) )
	ROM_LOAD16_BYTE( "4007210u.078", 0x060001, 0x010000, CRC(fa234861) SHA1(4c6753c64f942f6ea98f96e1e8f4de880800573e) )
	ROM_LOAD16_BYTE( "4010200v.078", 0x080000, 0x010000, CRC(150acc9e) SHA1(e172e6a1ed5ac0791fe053d4bde2d0c62202f399) )
	ROM_LOAD16_BYTE( "4009210v.078", 0x080001, 0x010000, CRC(85a9aa37) SHA1(e403a41c44edfca190cb15452d67a427de9cf56e) )
	ROM_LOAD16_BYTE( "1012200w.077", 0x0a0000, 0x010000, CRC(9a78b952) SHA1(53270d4d8c28579ebda477a63c034f6d1b9e5a58) )
	ROM_LOAD16_BYTE( "1011210w.077", 0x0a0001, 0x010000, CRC(c5cd5491) SHA1(ede5a3bb888342032d6758b0fb149451b6543d8b) )
	ROM_LOAD16_BYTE( "1014200x.077", 0x0c0000, 0x010000, CRC(a872792a) SHA1(9269e397567940013e5f46cadfe2bad2ca1a2bc4) )
	ROM_LOAD16_BYTE( "1013210x.077", 0x0c0001, 0x010000, CRC(ca7b3e53) SHA1(cdb3a6360893fd3dd2947c050dca8a4dfaa9ced9) )
	ROM_LOAD16_BYTE( "4016200y.078", 0x0e0000, 0x010000, CRC(aa296776) SHA1(991e33568e47a82cb183724f7df1f835850c30fa) )
	ROM_LOAD16_BYTE( "4015210y.078", 0x0e0001, 0x010000, CRC(030c803d) SHA1(bb249c9d1d28f6851a529b93c4f96d80665f2bce) )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU3, 0 )	/* region for ADSP 2100 */

	ROM_REGION( 0x20000, REGION_CPU5, 0 )		/* 2*64k for audio 68000 code */
	ROM_LOAD16_BYTE( "1032-70n.077", 0x000000, 0x008000, CRC(fbc903a6) SHA1(047240a5192918ced52d90b0555ba2e19a26329e) )
	ROM_LOAD16_BYTE( "1033-45n.077", 0x000001, 0x008000, CRC(e032ca7c) SHA1(a80c980c8e58cf8cada72140e42a3cd1ea987b3d) )

	ROM_REGION( 0x10000, REGION_CPU6, 0 )		/* region for audio 32010 */

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )	/* 384k for ADSP object ROM */
	ROM_LOAD16_BYTE( "1021-10h.077", 0x000000, 0x010000, CRC(9831fe73) SHA1(5215ee89a0014399b7d829e443b16590a4679844) )
	ROM_LOAD16_BYTE( "1023-10k.077", 0x000001, 0x010000, CRC(74bf0e0e) SHA1(a3d91ecf13c3619e073324517a4a5afaae831982) )
	ROM_LOAD16_BYTE( "1022-10j.077", 0x020000, 0x010000, CRC(c0393c31) SHA1(31726c01eb0d4650936908c90d45161197b7efba) )
	ROM_LOAD16_BYTE( "1024-10l.077", 0x020001, 0x010000, CRC(1e2fb25f) SHA1(4940091bbad6144bce091d2737191d266d4b0310) )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for ADSP I/O buffers */

	ROM_REGION16_BE( 0x51000, REGION_USER3, 0 )	/* 256k for DSK ROMs + 64k for RAM + 4k for ZRAM */
	ROM_LOAD16_BYTE( "1030-30e.078", 0x000000, 0x010000, CRC(d355a1c2) SHA1(6b0fb52dbecbe574959739f49c3c0fccdd5cebdb) )
	ROM_LOAD16_BYTE( "1031-10e.078", 0x000001, 0x010000, CRC(18fd5f44) SHA1(1c3bd780b72cfa61bcbd82683da18b4ee8d03a6d) )

	ROM_REGION( 0x50000, REGION_SOUND1, 0 )		/* 10*128k for audio serial ROMs */
	ROM_LOAD( "1123-65a.052", 0x000000, 0x010000, CRC(a88411dc) SHA1(1fd53c7eadffa163d5423df2f8338757e58d5f2e) )
	ROM_LOAD( "1124-55a.052", 0x010000, 0x010000, CRC(071a4309) SHA1(c623bd51d6a4a56503fbf138138854d6a30b11d6) )
	ROM_LOAD( "3125-45a.052", 0x020000, 0x010000, CRC(856548ff) SHA1(e8a17b274185c5e4ecf5f9f1c211e18b3ef2456d) )
	ROM_LOAD( "1126-30a.052", 0x030000, 0x010000, CRC(f46ef09c) SHA1(ba62f73ee3b33d8f26b430ffa468f8792dca23de) )
	ROM_LOAD( "1017-45c.077", 0x040000, 0x010000, CRC(e93129a3) SHA1(1221b08c8efbfd8cf6bfbfd956545f10bef48663) )
ROM_END


ROM_START( racedrc2 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 1MB for 68000 code */
	ROM_LOAD16_BYTE( "2002200r.078", 0x000000, 0x010000, CRC(669fe6fe) SHA1(1775ee3ef4817f553113772cf0fb35cbbe2e73a5) )
	ROM_LOAD16_BYTE( "2001210r.078", 0x000001, 0x010000, CRC(9312fd5f) SHA1(9dd1b30ebceedf50fb18d744540e2003a8110d09) )
	ROM_LOAD16_BYTE( "1004200s.078", 0x020000, 0x010000, CRC(4a1beb65) SHA1(3a4e3dd07a1415aef27e8bbebe0503296dfc7586) )
	ROM_LOAD16_BYTE( "1003210s.078", 0x020001, 0x010000, CRC(15bcbfe3) SHA1(a98f7fdb7486e7f3c6bdee8a19f483ca83b52a61) )
	ROM_LOAD16_BYTE( "1006200t.078", 0x040000, 0x010000, CRC(8539e6f3) SHA1(d6c0de7ebfeece9f4c98eff1f2c6f80a0a94739d) )
	ROM_LOAD16_BYTE( "1005210t.078", 0x040001, 0x010000, CRC(5f44fe33) SHA1(594d498566dda5f3a62b818b4da1b14c969fcc85) )
	ROM_LOAD16_BYTE( "1007210u.078", 0x060001, 0x010000, CRC(2517035a) SHA1(74ff3de6a0dd4a072097420b48eb6e8318654f34) )
	ROM_LOAD16_BYTE( "1008200u.078", 0x060000, 0x010000, CRC(0540d53d) SHA1(ca3b36c47df0f15da593a2c8c03407dd1547d403) )
	ROM_LOAD16_BYTE( "1009210v.078", 0x080001, 0x010000, CRC(33556cb5) SHA1(ece3801be3913e02fc77f2d2a1e2915a5d69d455) )
	ROM_LOAD16_BYTE( "1010200v.078", 0x080000, 0x010000, CRC(84329826) SHA1(1fbce8f1ffe898714d58bfa337aa6ab15275963e) )
	ROM_LOAD16_BYTE( "1012200w.077", 0x0a0000, 0x010000, CRC(9a78b952) SHA1(53270d4d8c28579ebda477a63c034f6d1b9e5a58) )
	ROM_LOAD16_BYTE( "1011210w.077", 0x0a0001, 0x010000, CRC(c5cd5491) SHA1(ede5a3bb888342032d6758b0fb149451b6543d8b) )
	ROM_LOAD16_BYTE( "1014200x.077", 0x0c0000, 0x010000, CRC(a872792a) SHA1(9269e397567940013e5f46cadfe2bad2ca1a2bc4) )
	ROM_LOAD16_BYTE( "1013210x.077", 0x0c0001, 0x010000, CRC(ca7b3e53) SHA1(cdb3a6360893fd3dd2947c050dca8a4dfaa9ced9) )
	ROM_LOAD16_BYTE( "1015210y.078", 0x0e0001, 0x010000, CRC(64dd6040) SHA1(bcadf4f1d9a0685ca39af903d3342d590850513c) )
	ROM_LOAD16_BYTE( "1016200y.078", 0x0e0000, 0x010000, CRC(a2a0ed28) SHA1(6f308a38594f7e54ebdd6983d28664ba595bc525) )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU3, 0 )	/* region for ADSP 2100 */

	ROM_REGION( 0x20000, REGION_CPU5, 0 )		/* 2*64k for audio 68000 code */
	ROM_LOAD16_BYTE( "1032-70n.077", 0x000000, 0x008000, CRC(fbc903a6) SHA1(047240a5192918ced52d90b0555ba2e19a26329e) )
	ROM_LOAD16_BYTE( "1033-45n.077", 0x000001, 0x008000, CRC(e032ca7c) SHA1(a80c980c8e58cf8cada72140e42a3cd1ea987b3d) )

	ROM_REGION( 0x10000, REGION_CPU6, 0 )		/* region for audio 32010 */

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )	/* 384k for ADSP object ROM */
	ROM_LOAD16_BYTE( "1021-10h.077", 0x000000, 0x010000, CRC(9831fe73) SHA1(5215ee89a0014399b7d829e443b16590a4679844) )
	ROM_LOAD16_BYTE( "1023-10k.077", 0x000001, 0x010000, CRC(74bf0e0e) SHA1(a3d91ecf13c3619e073324517a4a5afaae831982) )
	ROM_LOAD16_BYTE( "1022-10j.077", 0x020000, 0x010000, CRC(c0393c31) SHA1(31726c01eb0d4650936908c90d45161197b7efba) )
	ROM_LOAD16_BYTE( "1024-10l.077", 0x020001, 0x010000, CRC(1e2fb25f) SHA1(4940091bbad6144bce091d2737191d266d4b0310) )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for ADSP I/O buffers */

	ROM_REGION16_BE( 0x51000, REGION_USER3, 0 )	/* 256k for DSK ROMs + 64k for RAM + 4k for ZRAM */
	ROM_LOAD16_BYTE( "1030-30e.078", 0x000000, 0x010000, CRC(d355a1c2) SHA1(6b0fb52dbecbe574959739f49c3c0fccdd5cebdb) )
	ROM_LOAD16_BYTE( "1031-10e.078", 0x000001, 0x010000, CRC(18fd5f44) SHA1(1c3bd780b72cfa61bcbd82683da18b4ee8d03a6d) )

	ROM_REGION( 0x50000, REGION_SOUND1, 0 )		/* 10*128k for audio serial ROMs */
	ROM_LOAD( "1123-65a.052", 0x000000, 0x010000, CRC(a88411dc) SHA1(1fd53c7eadffa163d5423df2f8338757e58d5f2e) )
	ROM_LOAD( "1124-55a.052", 0x010000, 0x010000, CRC(071a4309) SHA1(c623bd51d6a4a56503fbf138138854d6a30b11d6) )
	ROM_LOAD( "3125-45a.052", 0x020000, 0x010000, CRC(856548ff) SHA1(e8a17b274185c5e4ecf5f9f1c211e18b3ef2456d) )
	ROM_LOAD( "1126-30a.052", 0x030000, 0x010000, CRC(f46ef09c) SHA1(ba62f73ee3b33d8f26b430ffa468f8792dca23de) )
	ROM_LOAD( "1017-45c.077", 0x040000, 0x010000, CRC(e93129a3) SHA1(1221b08c8efbfd8cf6bfbfd956545f10bef48663) )
ROM_END


ROM_START( racedrc1 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 1MB for 68000 code */
	ROM_LOAD16_BYTE( "1002200r.078", 0x000000, 0x010000, CRC(ed997f08) SHA1(9512f347120174345dbe8c756e5c239fdce438a5) )
	ROM_LOAD16_BYTE( "1001210r.078", 0x000001, 0x010000, CRC(5540ee7d) SHA1(f1d16cfa20b61f1cf075e3275ea4af0b46aa5470) )
	ROM_LOAD16_BYTE( "1004200s.078", 0x020000, 0x010000, CRC(4a1beb65) SHA1(3a4e3dd07a1415aef27e8bbebe0503296dfc7586) )
	ROM_LOAD16_BYTE( "1003210s.078", 0x020001, 0x010000, CRC(15bcbfe3) SHA1(a98f7fdb7486e7f3c6bdee8a19f483ca83b52a61) )
	ROM_LOAD16_BYTE( "1006200t.078", 0x040000, 0x010000, CRC(8539e6f3) SHA1(d6c0de7ebfeece9f4c98eff1f2c6f80a0a94739d) )
	ROM_LOAD16_BYTE( "1005210t.078", 0x040001, 0x010000, CRC(5f44fe33) SHA1(594d498566dda5f3a62b818b4da1b14c969fcc85) )
	ROM_LOAD16_BYTE( "1007210u.078", 0x060001, 0x010000, CRC(2517035a) SHA1(74ff3de6a0dd4a072097420b48eb6e8318654f34) )
	ROM_LOAD16_BYTE( "1008200u.078", 0x060000, 0x010000, CRC(0540d53d) SHA1(ca3b36c47df0f15da593a2c8c03407dd1547d403) )
	ROM_LOAD16_BYTE( "1009210v.078", 0x080001, 0x010000, CRC(33556cb5) SHA1(ece3801be3913e02fc77f2d2a1e2915a5d69d455) )
	ROM_LOAD16_BYTE( "1010200v.078", 0x080000, 0x010000, CRC(84329826) SHA1(1fbce8f1ffe898714d58bfa337aa6ab15275963e) )
	ROM_LOAD16_BYTE( "1012200w.077", 0x0a0000, 0x010000, CRC(9a78b952) SHA1(53270d4d8c28579ebda477a63c034f6d1b9e5a58) )
	ROM_LOAD16_BYTE( "1011210w.077", 0x0a0001, 0x010000, CRC(c5cd5491) SHA1(ede5a3bb888342032d6758b0fb149451b6543d8b) )
	ROM_LOAD16_BYTE( "1014200x.077", 0x0c0000, 0x010000, CRC(a872792a) SHA1(9269e397567940013e5f46cadfe2bad2ca1a2bc4) )
	ROM_LOAD16_BYTE( "1013210x.077", 0x0c0001, 0x010000, CRC(ca7b3e53) SHA1(cdb3a6360893fd3dd2947c050dca8a4dfaa9ced9) )
	ROM_LOAD16_BYTE( "1015210y.078", 0x0e0001, 0x010000, CRC(64dd6040) SHA1(bcadf4f1d9a0685ca39af903d3342d590850513c) )
	ROM_LOAD16_BYTE( "1016200y.078", 0x0e0000, 0x010000, CRC(a2a0ed28) SHA1(6f308a38594f7e54ebdd6983d28664ba595bc525) )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU3, 0 )	/* region for ADSP 2100 */

	ROM_REGION( 0x20000, REGION_CPU5, 0 )		/* 2*64k for audio 68000 code */
	ROM_LOAD16_BYTE( "1032-70n.077", 0x000000, 0x008000, CRC(fbc903a6) SHA1(047240a5192918ced52d90b0555ba2e19a26329e) )
	ROM_LOAD16_BYTE( "1033-45n.077", 0x000001, 0x008000, CRC(e032ca7c) SHA1(a80c980c8e58cf8cada72140e42a3cd1ea987b3d) )

	ROM_REGION( 0x10000, REGION_CPU6, 0 )		/* region for audio 32010 */

	ROM_REGION16_BE( 0x60000, REGION_USER1, 0 )	/* 384k for ADSP object ROM */
	ROM_LOAD16_BYTE( "1021-10h.077", 0x000000, 0x010000, CRC(9831fe73) SHA1(5215ee89a0014399b7d829e443b16590a4679844) )
	ROM_LOAD16_BYTE( "1023-10k.077", 0x000001, 0x010000, CRC(74bf0e0e) SHA1(a3d91ecf13c3619e073324517a4a5afaae831982) )
	ROM_LOAD16_BYTE( "1022-10j.077", 0x020000, 0x010000, CRC(c0393c31) SHA1(31726c01eb0d4650936908c90d45161197b7efba) )
	ROM_LOAD16_BYTE( "1024-10l.077", 0x020001, 0x010000, CRC(1e2fb25f) SHA1(4940091bbad6144bce091d2737191d266d4b0310) )

	ROM_REGION( 0x8000, REGION_USER2, 0 )		/* 32k for ADSP I/O buffers */

	ROM_REGION16_BE( 0x51000, REGION_USER3, 0 )	/* 256k for DSK ROMs + 64k for RAM + 4k for ZRAM */
	ROM_LOAD16_BYTE( "1030-30e.078", 0x000000, 0x010000, CRC(d355a1c2) SHA1(6b0fb52dbecbe574959739f49c3c0fccdd5cebdb) )
	ROM_LOAD16_BYTE( "1031-10e.078", 0x000001, 0x010000, CRC(18fd5f44) SHA1(1c3bd780b72cfa61bcbd82683da18b4ee8d03a6d) )

	ROM_REGION( 0x50000, REGION_SOUND1, 0 )		/* 10*128k for audio serial ROMs */
	ROM_LOAD( "1123-65a.052", 0x000000, 0x010000, CRC(a88411dc) SHA1(1fd53c7eadffa163d5423df2f8338757e58d5f2e) )
	ROM_LOAD( "1124-55a.052", 0x010000, 0x010000, CRC(071a4309) SHA1(c623bd51d6a4a56503fbf138138854d6a30b11d6) )
	ROM_LOAD( "3125-45a.052", 0x020000, 0x010000, CRC(856548ff) SHA1(e8a17b274185c5e4ecf5f9f1c211e18b3ef2456d) )
	ROM_LOAD( "1126-30a.052", 0x030000, 0x010000, CRC(f46ef09c) SHA1(ba62f73ee3b33d8f26b430ffa468f8792dca23de) )
	ROM_LOAD( "1017-45c.077", 0x040000, 0x010000, CRC(e93129a3) SHA1(1221b08c8efbfd8cf6bfbfd956545f10bef48663) )
ROM_END


ROM_START( steeltal )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 1MB for 68000 code */
	ROM_LOAD16_BYTE( "1002200r.087", 0x000000, 0x010000, CRC(31bf01a9) SHA1(cd08a839dbb5283a6e2bb35bc9e1578a14e3c2e6) )
	ROM_LOAD16_BYTE( "1001210r.087", 0x000001, 0x010000, CRC(b4fa2900) SHA1(5e92ab4af31321b891c072305f8b8ef30a3e1fb0) )
	ROM_LOAD16_BYTE( "1004200s.087", 0x020000, 0x010000, CRC(c31ca924) SHA1(8d7d2a3d204e69d759cf767b57570c18db5a3fd8) )
	ROM_LOAD16_BYTE( "1003210s.087", 0x020001, 0x010000, CRC(7802e86d) SHA1(de5ee2f66f1e46a1bf437f15101e64bfb66fdecf) )
	ROM_LOAD16_BYTE( "1006200t.087", 0x040000, 0x010000, CRC(01ebc0c3) SHA1(34b6b837171456927d6ff83dad61ee2f64a06780) )
	ROM_LOAD16_BYTE( "1005210t.087", 0x040001, 0x010000, CRC(1107499c) SHA1(5c52db8889d8588e4c5c32b1366d47b288d7a2aa) )
	ROM_LOAD16_BYTE( "1008200u.087", 0x060000, 0x010000, CRC(78e72af9) SHA1(14bf86dd6e7c60af017ee35dfda16061b8edadfe) )
	ROM_LOAD16_BYTE( "1007210u.087", 0x060001, 0x010000, CRC(420be93b) SHA1(f22691f402307edce4ca51b30858206f353de663) )
	ROM_LOAD16_BYTE( "1010200v.087", 0x080000, 0x010000, CRC(7eff9f8b) SHA1(7e6ee7dec75bc9224834d35c0b9a7c5d8bd897bc) )
	ROM_LOAD16_BYTE( "1009210v.087", 0x080001, 0x010000, CRC(53e9fe94) SHA1(bf05ce2f8d97e7be96c99814d280289ffad1621a) )
	ROM_LOAD16_BYTE( "1012200w.087", 0x0a0000, 0x010000, CRC(d39e8cef) SHA1(ba6aa8b70c30d6db70cfcf51dfe450dcfde0f3e4) )
	ROM_LOAD16_BYTE( "1011210w.087", 0x0a0001, 0x010000, CRC(b388bf91) SHA1(3e6a17e4462023f59f6581b09c716e6c51e7ae8e) )
	ROM_LOAD16_BYTE( "1014200x.087", 0x0c0000, 0x010000, CRC(9f047de7) SHA1(58c4f062d8eef9e2d0143a9b77b066fc3bb5dc29) )
	ROM_LOAD16_BYTE( "1013210x.087", 0x0c0001, 0x010000, CRC(f6b99901) SHA1(5c162a6d945c312e49e0a1e04285c597dde4ef94) )
	ROM_LOAD16_BYTE( "1016200y.087", 0x0e0000, 0x010000, CRC(db62362e) SHA1(e1d392aa00ac36296728257fa26c6aa68a4ebe5f) )
	ROM_LOAD16_BYTE( "1015210y.087", 0x0e0001, 0x010000, CRC(ef517db7) SHA1(16e7e351326391480bf36c58d6b34ef4128b6627) )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU4, 0 )	/* region for ADSP 2101 */

	ROM_REGION( 0x14000, REGION_CPU5, 0 )		/* 64k for 6502 code */
	ROM_LOAD( "5001-1f.087",  0x010000, 0x004000, CRC(c52d8218) SHA1(3511c8c65583c7e44242f4cc48d7cc46fc748868) )
	ROM_CONTINUE(             0x004000, 0x00c000 )

	ROM_REGION( 0x10000, REGION_CPU6, 0 )		/* 64k for ASIC65 */
	ROM_LOAD( "9007-10c.087", 0x000000, 0x002000, CRC(2956984f) SHA1(63c9a99b00c3cbb63aca908b076c2c4d3f70f386) )

	ROM_REGION( 0x10000, REGION_CPU7, 0 )		/* 64k for DSP communications */
	ROM_LOAD( "1025-5f.087",  0x000000, 0x010000, CRC(4c645933) SHA1(7a1cf049e368059a79b03598de73c30d8dae5e90) )

	ROM_REGION16_BE( 0xc0000, REGION_USER1, 0 )	/* 768k for object ROM */
	ROM_LOAD16_BYTE( "1018-2t.087",  0x000000, 0x020000, CRC(a5882384) SHA1(157707b5b114fa584893dec07dc456d4a5520f44) )
	ROM_LOAD16_BYTE( "1017-2lm.087", 0x000001, 0x020000, CRC(0a29db30) SHA1(f11ad7fe27989ffd66e9bef2c14ec040a4125d8a) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* 1MB for ADPCM samples */
	ROM_LOAD( "5002-1m.087",  0x080000, 0x020000, CRC(c904db9c) SHA1(d25fff3da87d2b716cd65fb7dd157c3f1f5e5909) )
	ROM_LOAD( "5003-1n.087",  0x0a0000, 0x020000, CRC(164580b3) SHA1(03118c8323d8a49a65addc61c1402d152d42d7f9) )
	ROM_LOAD( "5004-1p.087",  0x0c0000, 0x020000, CRC(296290a0) SHA1(8a3441a5618233f561531fe456e1f5ed22183421) )
	ROM_LOAD( "5005-1r.087",  0x0e0000, 0x020000, CRC(c029d037) SHA1(0ae736c0ca3a1974911464328dd5a6b41a939130) )
ROM_END


ROM_START( steeltag )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 1MB for 68000 code */
	ROM_LOAD16_BYTE( "1002200r.087", 0x000000, 0x010000, CRC(31bf01a9) SHA1(cd08a839dbb5283a6e2bb35bc9e1578a14e3c2e6) )
	ROM_LOAD16_BYTE( "1001210r.087", 0x000001, 0x010000, CRC(b4fa2900) SHA1(5e92ab4af31321b891c072305f8b8ef30a3e1fb0) )
	ROM_LOAD16_BYTE( "2004200s.087", 0x020000, 0x010000, CRC(11fcba15) SHA1(031fc4c46a25ba8b5e15257f7443899bb484c09e) )
	ROM_LOAD16_BYTE( "2003210s.087", 0x020001, 0x010000, CRC(249d4c0f) SHA1(0fa3c2fb10b5ca6a256b641528b5c15155501d23) )
	ROM_LOAD16_BYTE( "2006200t.087", 0x040000, 0x010000, CRC(55609ae3) SHA1(0379465d0d42cf59d85fa54332ade09ca5af71ad) )
	ROM_LOAD16_BYTE( "2005210t.087", 0x040001, 0x010000, CRC(0fbf3d62) SHA1(f4477e4522ca13ab1f4a984a78fee30eec1f35b0) )
	ROM_LOAD16_BYTE( "1008200u.087", 0x060000, 0x010000, CRC(78e72af9) SHA1(14bf86dd6e7c60af017ee35dfda16061b8edadfe) )
	ROM_LOAD16_BYTE( "1007210u.087", 0x060001, 0x010000, CRC(420be93b) SHA1(f22691f402307edce4ca51b30858206f353de663) )
	ROM_LOAD16_BYTE( "1010200v.087", 0x080000, 0x010000, CRC(7eff9f8b) SHA1(7e6ee7dec75bc9224834d35c0b9a7c5d8bd897bc) )
	ROM_LOAD16_BYTE( "1009210v.087", 0x080001, 0x010000, CRC(53e9fe94) SHA1(bf05ce2f8d97e7be96c99814d280289ffad1621a) )
	ROM_LOAD16_BYTE( "1012200w.087", 0x0a0000, 0x010000, CRC(d39e8cef) SHA1(ba6aa8b70c30d6db70cfcf51dfe450dcfde0f3e4) )
	ROM_LOAD16_BYTE( "1011210w.087", 0x0a0001, 0x010000, CRC(b388bf91) SHA1(3e6a17e4462023f59f6581b09c716e6c51e7ae8e) )
	ROM_LOAD16_BYTE( "1014200x.087", 0x0c0000, 0x010000, CRC(9f047de7) SHA1(58c4f062d8eef9e2d0143a9b77b066fc3bb5dc29) )
	ROM_LOAD16_BYTE( "1013210x.087", 0x0c0001, 0x010000, CRC(f6b99901) SHA1(5c162a6d945c312e49e0a1e04285c597dde4ef94) )
	ROM_LOAD16_BYTE( "1016200y.087", 0x0e0000, 0x010000, CRC(db62362e) SHA1(e1d392aa00ac36296728257fa26c6aa68a4ebe5f) )
	ROM_LOAD16_BYTE( "1015210y.087", 0x0e0001, 0x010000, CRC(ef517db7) SHA1(16e7e351326391480bf36c58d6b34ef4128b6627) )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU4, 0 )	/* region for ADSP 2101 */

	ROM_REGION( 0x14000, REGION_CPU5, 0 )		/* 64k for 6502 code */
	ROM_LOAD( "5001-1f.087",  0x010000, 0x004000, CRC(c52d8218) SHA1(3511c8c65583c7e44242f4cc48d7cc46fc748868) )
	ROM_CONTINUE(             0x004000, 0x00c000 )

	ROM_REGION( 0x10000, REGION_CPU6, 0 )		/* 64k for ASIC65 */
	ROM_LOAD( "9007-10c.087", 0x000000, 0x002000, CRC(2956984f) SHA1(63c9a99b00c3cbb63aca908b076c2c4d3f70f386) )

	ROM_REGION( 0x10000, REGION_CPU7, 0 )		/* 64k for DSP communications */
	ROM_LOAD( "1025-5f.087",  0x000000, 0x010000, CRC(4c645933) SHA1(7a1cf049e368059a79b03598de73c30d8dae5e90) )

	ROM_REGION16_BE( 0xc0000, REGION_USER1, 0 )	/* 768k for object ROM */
	ROM_LOAD16_BYTE( "1018-2t.087",  0x000000, 0x020000, CRC(a5882384) SHA1(157707b5b114fa584893dec07dc456d4a5520f44) )
	ROM_LOAD16_BYTE( "1017-2lm.087", 0x000001, 0x020000, CRC(0a29db30) SHA1(f11ad7fe27989ffd66e9bef2c14ec040a4125d8a) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* 1MB for ADPCM samples */
	ROM_LOAD( "5002-1m.087",  0x080000, 0x020000, CRC(c904db9c) SHA1(d25fff3da87d2b716cd65fb7dd157c3f1f5e5909) )
	ROM_LOAD( "5003-1n.087",  0x0a0000, 0x020000, CRC(164580b3) SHA1(03118c8323d8a49a65addc61c1402d152d42d7f9) )
	ROM_LOAD( "5004-1p.087",  0x0c0000, 0x020000, CRC(296290a0) SHA1(8a3441a5618233f561531fe456e1f5ed22183421) )
	ROM_LOAD( "5005-1r.087",  0x0e0000, 0x020000, CRC(c029d037) SHA1(0ae736c0ca3a1974911464328dd5a6b41a939130) )
ROM_END


ROM_START( steelta1 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 1MB for 68000 code */
	ROM_LOAD16_BYTE( "1002200r.087", 0x000000, 0x010000, CRC(31bf01a9) SHA1(cd08a839dbb5283a6e2bb35bc9e1578a14e3c2e6) )
	ROM_LOAD16_BYTE( "1001210r.087", 0x000001, 0x010000, CRC(b4fa2900) SHA1(5e92ab4af31321b891c072305f8b8ef30a3e1fb0) )
	ROM_LOAD16_BYTE( "a004200s.087", 0x020000, 0x010000, CRC(392c992d) SHA1(10d1606214df675e6e59185e6b97034c4a47055a) )
	ROM_LOAD16_BYTE( "a003210s.087", 0x020001, 0x010000, CRC(562066d0) SHA1(69973dbdb1580942e8d22eee4ee130ecd67885fd) )
	ROM_LOAD16_BYTE( "a006200t.087", 0x040000, 0x010000, CRC(a99873fc) SHA1(5f4c0c2ca38d800ec8b2d4d6adf74901249bd794) )
	ROM_LOAD16_BYTE( "a005210t.087", 0x040001, 0x010000, CRC(5c1437e9) SHA1(c0dd4db5ecbbeb90b64fdaeb5abfcbe06fbd09cd) )
	ROM_LOAD16_BYTE( "a008200u.087", 0x060000, 0x010000, CRC(8a118c41) SHA1(4312cd09b2ce3ad181f07c564962640b2431b913) )
	ROM_LOAD16_BYTE( "a007210u.087", 0x060001, 0x010000, CRC(f343ae79) SHA1(63cf2e76de9ca12d916cafed7d7030965756b0b6) )
	ROM_LOAD16_BYTE( "1010200v.087", 0x080000, 0x010000, CRC(7eff9f8b) SHA1(7e6ee7dec75bc9224834d35c0b9a7c5d8bd897bc) )
	ROM_LOAD16_BYTE( "1009210v.087", 0x080001, 0x010000, CRC(53e9fe94) SHA1(bf05ce2f8d97e7be96c99814d280289ffad1621a) )
	ROM_LOAD16_BYTE( "1012200w.087", 0x0a0000, 0x010000, CRC(d39e8cef) SHA1(ba6aa8b70c30d6db70cfcf51dfe450dcfde0f3e4) )
	ROM_LOAD16_BYTE( "1011210w.087", 0x0a0001, 0x010000, CRC(b388bf91) SHA1(3e6a17e4462023f59f6581b09c716e6c51e7ae8e) )
	ROM_LOAD16_BYTE( "1014200x.087", 0x0c0000, 0x010000, CRC(9f047de7) SHA1(58c4f062d8eef9e2d0143a9b77b066fc3bb5dc29) )
	ROM_LOAD16_BYTE( "1013210x.087", 0x0c0001, 0x010000, CRC(f6b99901) SHA1(5c162a6d945c312e49e0a1e04285c597dde4ef94) )
	ROM_LOAD16_BYTE( "1016200y.087", 0x0e0000, 0x010000, CRC(db62362e) SHA1(e1d392aa00ac36296728257fa26c6aa68a4ebe5f) )
	ROM_LOAD16_BYTE( "1015210y.087", 0x0e0001, 0x010000, CRC(ef517db7) SHA1(16e7e351326391480bf36c58d6b34ef4128b6627) )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU4, 0 )	/* region for ADSP 2101 */

	ROM_REGION( 0x14000, REGION_CPU5, 0 )		/* 64k for 6502 code */
	ROM_LOAD( "5001-1f.087",  0x010000, 0x004000, CRC(c52d8218) SHA1(3511c8c65583c7e44242f4cc48d7cc46fc748868) )
	ROM_CONTINUE(             0x004000, 0x00c000 )

	ROM_REGION( 0x10000, REGION_CPU6, 0 )		/* 64k for ASIC65 */
	ROM_LOAD( "9007-10c.087", 0x000000, 0x002000, CRC(2956984f) SHA1(63c9a99b00c3cbb63aca908b076c2c4d3f70f386) )

	ROM_REGION( 0x10000, REGION_CPU7, 0 )		/* 64k for DSP communications */
	ROM_LOAD( "1025-5f.087",  0x000000, 0x010000, CRC(4c645933) SHA1(7a1cf049e368059a79b03598de73c30d8dae5e90) )

	ROM_REGION16_BE( 0xc0000, REGION_USER1, 0 )	/* 768k for object ROM */
	ROM_LOAD16_BYTE( "1018-2t.087",  0x000000, 0x020000, CRC(a5882384) SHA1(157707b5b114fa584893dec07dc456d4a5520f44) )
	ROM_LOAD16_BYTE( "1017-2lm.087", 0x000001, 0x020000, CRC(0a29db30) SHA1(f11ad7fe27989ffd66e9bef2c14ec040a4125d8a) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* 1MB for ADPCM samples */
	ROM_LOAD( "5002-1m.087",  0x080000, 0x020000, CRC(c904db9c) SHA1(d25fff3da87d2b716cd65fb7dd157c3f1f5e5909) )
	ROM_LOAD( "5003-1n.087",  0x0a0000, 0x020000, CRC(164580b3) SHA1(03118c8323d8a49a65addc61c1402d152d42d7f9) )
	ROM_LOAD( "5004-1p.087",  0x0c0000, 0x020000, CRC(296290a0) SHA1(8a3441a5618233f561531fe456e1f5ed22183421) )
	ROM_LOAD( "5005-1r.087",  0x0e0000, 0x020000, CRC(c029d037) SHA1(0ae736c0ca3a1974911464328dd5a6b41a939130) )
ROM_END


ROM_START( steeltap )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 1MB for 68000 code */
	ROM_LOAD16_BYTE( "rom-200r.bin", 0x00000, 0x10000, CRC(72a9ce3b) SHA1(6706ff32173735d16d9da1321b64a4a9bb317b2e) )
	ROM_LOAD16_BYTE( "rom-210r.bin", 0x00001, 0x10000, CRC(46d83b42) SHA1(85b178781f0595b5af0375fee32d0dd8cdba8fca) )
	ROM_LOAD16_BYTE( "rom-200s.bin", 0x20000, 0x10000, CRC(bf1b31ae) SHA1(f2d7f13854b8a3dd4de9ae98cc3034dfcf3846b8) )
	ROM_LOAD16_BYTE( "rom-210s.bin", 0x20001, 0x10000, CRC(eaf46672) SHA1(51a99811b7729a8105dd5f3c608015626b01d195) )
	ROM_LOAD16_BYTE( "rom-200t.bin", 0x40000, 0x10000, CRC(3dfe9a3e) SHA1(df303072821bae42d9169723e277a8bfafaae771) )
	ROM_LOAD16_BYTE( "rom-210t.bin", 0x40001, 0x10000, CRC(3c4e8521) SHA1(5061ae2e6b6fa7c444501418c51fdab5310bf702) )
	ROM_LOAD16_BYTE( "rom-200u.bin", 0x60000, 0x10000, CRC(7a52a980) SHA1(2e5ab7e6c59de965242686e714e9800d7b8c42fe) )
	ROM_LOAD16_BYTE( "rom-210u.bin", 0x60001, 0x10000, CRC(6c20e861) SHA1(9996809c16f249d276176030671e141f4e2bbcda) )
	ROM_LOAD16_BYTE( "rom-200v.bin", 0x80000, 0x10000, CRC(137df911) SHA1(a7c38469ab1a00bb100fdb5a2ddbeb1a37819dc7) )
	ROM_LOAD16_BYTE( "rom-210v.bin", 0x80001, 0x10000, CRC(2dd87840) SHA1(96a61f65fb1c28b34a625339bb8891e356ea9693) )
	ROM_LOAD16_BYTE( "rom-200w.bin", 0xa0000, 0x10000, CRC(0bbe5f80) SHA1(866a874833106675e97a16151a97ea2bc590fc78) )
	ROM_LOAD16_BYTE( "rom-210w.bin", 0xa0001, 0x10000, CRC(31dc9321) SHA1(e1d459b209af8106fa404803490055eac16f1c0f) )
	ROM_LOAD16_BYTE( "rom-200x.bin", 0xc0000, 0x10000, CRC(b494ba85) SHA1(f24925fcdbd67e54e1c071cd05e7ad40e1240b49) )
	ROM_LOAD16_BYTE( "rom-210x.bin", 0xc0001, 0x10000, CRC(63765dc6) SHA1(74b76e4e1f0ed4c237193e77c92450932cfd68fd) )
	ROM_LOAD16_BYTE( "rom-200y.bin", 0xe0000, 0x10000, CRC(b568e1be) SHA1(5d62037892e040515e4262db43057f33436fa12d) )
	ROM_LOAD16_BYTE( "rom-210y.bin", 0xe0001, 0x10000, CRC(3f5cdd3e) SHA1(c33c155158a5c69a7f2e61cd88b297dc14ecd479) )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU4, 0 )	/* region for ADSP 2101 */

	ROM_REGION( 0x14000, REGION_CPU5, 0 )		/* 64k for 6502 code */
	ROM_LOAD( "5001-1f.087",  0x010000, 0x004000, CRC(c52d8218) SHA1(3511c8c65583c7e44242f4cc48d7cc46fc748868) )
	ROM_CONTINUE(             0x004000, 0x00c000 )

	ROM_REGION( 0x10000, REGION_CPU6, 0 )		/* 64k for ASIC65 */
	ROM_LOAD( "9007-10c.087", 0x000000, 0x002000, CRC(2956984f) SHA1(63c9a99b00c3cbb63aca908b076c2c4d3f70f386) )

	ROM_REGION( 0x10000, REGION_CPU7, 0 )		/* 64k for DSP communications */
	ROM_LOAD( "1025-5f.087",  0x000000, 0x010000, CRC(4c645933) SHA1(7a1cf049e368059a79b03598de73c30d8dae5e90) )

	ROM_REGION16_BE( 0xc0000, REGION_USER1, 0 )	/* 768k for object ROM */
	ROM_LOAD16_BYTE( "rom.2t",  0x00000, 0x20000, CRC(05284504) SHA1(03b81c077f8ff073713f4bcc10b82087743b0d84) )
	ROM_LOAD16_BYTE( "rom.2lm", 0x00001, 0x20000, CRC(d6e65b87) SHA1(ac4b2f292f6e28a15e3a12f09f6c2f9523e8b178) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* 1MB for ADPCM samples */
	ROM_LOAD( "5002-1m.087",  0x080000, 0x020000, CRC(c904db9c) SHA1(d25fff3da87d2b716cd65fb7dd157c3f1f5e5909) )
	ROM_LOAD( "5003-1n.087",  0x0a0000, 0x020000, CRC(164580b3) SHA1(03118c8323d8a49a65addc61c1402d152d42d7f9) )
	ROM_LOAD( "5004-1p.087",  0x0c0000, 0x020000, CRC(296290a0) SHA1(8a3441a5618233f561531fe456e1f5ed22183421) )
	ROM_LOAD( "5005-1r.087",  0x0e0000, 0x020000, CRC(c029d037) SHA1(0ae736c0ca3a1974911464328dd5a6b41a939130) )
ROM_END


ROM_START( hdrivair )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )		/* 2MB for 68000 code */
	ROM_LOAD16_BYTE( "stesthi.bin", 0x000000, 0x20000, CRC(b4bfa451) SHA1(002a5d213ba8ec76ad83a87d76aefbd98b1e4c94) )
	ROM_LOAD16_BYTE( "stestlo.bin", 0x000001, 0x20000, CRC(58758419) SHA1(7951d4c8cf0b28b4fac3fe172ea3bc56f61bd9ff) )
	ROM_LOAD16_BYTE( "drivehi.bin", 0x040000, 0x20000, CRC(d15f5119) SHA1(c2c7e9675c14ba41effa6f721602f6471b348758) )
	ROM_LOAD16_BYTE( "drivelo.bin", 0x040001, 0x20000, CRC(34adf4af) SHA1(db93fe1388d092916e1db526ea0fe72b35bf5ec0) )
	ROM_LOAD16_BYTE( "wavehi.bin",  0x0c0000, 0x20000, CRC(b21a34b6) SHA1(4309774e482cb97a074884e84358618512dc4f77) )
	ROM_LOAD16_BYTE( "wavelo.bin",  0x0c0001, 0x20000, CRC(15ed4394) SHA1(8c0ae74b2adce312c41bea95dc3b4f55bc3f8b6d) )
	ROM_LOAD16_BYTE( "ms2pics.hi",  0x140000, 0x20000, CRC(bca0af7e) SHA1(f25cfdc8f8fa77bcca2723335f76ba8a7d790eec) )
	ROM_LOAD16_BYTE( "ms2pics.lo",  0x140001, 0x20000, CRC(c3c6be8c) SHA1(66f0a54979bd83a940f226a8b3a9cf2eb3eaa908) )
	ROM_LOAD16_BYTE( "univhi.bin",  0x180000, 0x20000, CRC(86351673) SHA1(34170dd48aa77fe93f0c890a4878f3d370dae9b1) )
	ROM_LOAD16_BYTE( "univlo.bin",  0x180001, 0x20000, CRC(22d3b699) SHA1(e7d3e2107f17579549d09b1bb58fbab647343a61) )
	ROM_LOAD16_BYTE( "coprochi.bin",0x1c0000, 0x20000, CRC(5d2ca109) SHA1(e1a94d3fbfd5d542732555bf60268e73d66b3a06) )
	ROM_LOAD16_BYTE( "coproclo.bin",0x1c0001, 0x20000, CRC(5f98b04d) SHA1(9c4fa4092fd85f1d67be44f2ff91a907a87db51a) )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU3, 0 )	/* dummy region for ADSP 2101 */

	ROM_REGION( ADSP2100_SIZE + 0x10000, REGION_CPU4, 0 )	/* dummy region for ADSP 2105 */
	ROM_LOAD( "sboot.bin", ADSP2100_SIZE + 0x00000, 0x10000, CRC(cde4d010) SHA1(853f4b813ff70fe74cd87e92131c46fca045610d) )

	ROM_REGION( ADSP2100_SIZE + 0x10000, REGION_CPU5, 0 )	/* dummy region for ADSP 2105 */
	ROM_LOAD( "xboot.bin", ADSP2100_SIZE + 0x00000, 0x10000, CRC(054b46a0) SHA1(038eec17e678f2755239d6795acfda621796802e) )

	ROM_REGION( 0xc0000, REGION_USER1, 0 )		/* 768k for object ROM */
	ROM_LOAD16_BYTE( "obj0l.bin",   0x00000, 0x20000, CRC(1f835f2e) SHA1(9d3419f2c1aa65ddfe9ace4e70ca1212d634afbf) )
	ROM_LOAD16_BYTE( "obj0h.bin",   0x00001, 0x20000, CRC(c321ab55) SHA1(e095e40bb1ebda7c9ff04a5086c10ab41dec2f16) )
	ROM_LOAD16_BYTE( "obj1l.bin",   0x40000, 0x20000, CRC(3d65f264) SHA1(e9232f5bf439bf4e1cf99cc7e81b7f9550563f15) )
	ROM_LOAD16_BYTE( "obj1h.bin",   0x40001, 0x20000, CRC(2c06b708) SHA1(daa16f727f2f500172f88b69d6931aa0fa13641b) )
	ROM_LOAD16_BYTE( "obj2l.bin",   0x80000, 0x20000, CRC(b206cc7e) SHA1(17f05e906c41b804fe99dd6cd8acbade919a6a10) )
	ROM_LOAD16_BYTE( "obj2h.bin",   0x80001, 0x20000, CRC(a666e98c) SHA1(90e380ff87538c7d557cf005a4a5bcedc250eb72) )

	ROM_REGION16_BE( 0x140000, REGION_USER3, 0 )/* 1MB for DSK ROMs + 256k for RAM */
	ROM_LOAD16_BYTE( "dsk2phi.bin", 0x00000, 0x80000, CRC(71c268e0) SHA1(c089248a7dfadf2eba3134fe40ebb777c115a886) )
	ROM_LOAD16_BYTE( "dsk2plo.bin", 0x00001, 0x80000, CRC(edf96363) SHA1(47f0608c2b0ab983681de021a16b1d10d4feb800) )

	ROM_REGION32_LE( 0x200000, REGION_USER4, 0 )/* 2MB for ASIC61 ROMs */
	ROM_LOAD32_BYTE( "roads0.bin",  0x000000, 0x80000, CRC(5028eb41) SHA1(abe9d73e74d4f0308f07cbe9c18c8a77456fdbc7) )
	ROM_LOAD32_BYTE( "roads1.bin",  0x000001, 0x80000, CRC(c3f2c201) SHA1(c73933d7e46f3c63c4ca86af40eb4f0abb09aedf) )
	ROM_LOAD32_BYTE( "roads2.bin",  0x000002, 0x80000, CRC(527923fe) SHA1(839de8486bb7489f059b5a629ab229ad96de7eac) )
	ROM_LOAD32_BYTE( "roads3.bin",  0x000003, 0x80000, CRC(2f2023b2) SHA1(d474892443db2f0710c2be0d6b90735a2fbee12a) )

	ROM_REGION16_BE( 0x100000, REGION_USER5, 0 )
	/* DS IV sound section (2 x ADSP2105)*/
	ROM_LOAD16_BYTE( "ds3rom0.bin", 0x00001, 0x80000, CRC(90b8dbb6) SHA1(fff693cb81e88bc00e048bb71406295fe7be5122) )
	ROM_LOAD16_BYTE( "ds3rom1.bin", 0x00000, 0x80000, CRC(58173812) SHA1(b7e9f724011a362e1fc17aa7a7a95841e01d5430) )
	ROM_LOAD16_BYTE( "ds3rom2.bin", 0x00001, 0x80000, CRC(5a4b18fa) SHA1(1e9193c1daf14fc0aeca6fab762f5753ec73435f) )
	ROM_LOAD16_BYTE( "ds3rom3.bin", 0x00000, 0x80000, CRC(63965868) SHA1(d61d9d6709a3a3c37c2652602e97fdee52e0e7cb) )
	ROM_LOAD16_BYTE( "ds3rom4.bin", 0x00001, 0x80000, CRC(15ffb19a) SHA1(030dc90b7cabcd7fc5f231b09d2aa2eaf6e60b98) )
	ROM_LOAD16_BYTE( "ds3rom5.bin", 0x00000, 0x80000, CRC(8d0e9b27) SHA1(76556f48bdf14475260c268ebdb16ecb494b2f36) )
	ROM_LOAD16_BYTE( "ds3rom6.bin", 0x00001, 0x80000, CRC(ce7edbae) SHA1(58e9d8379157bb69e323eb79332d644a32c70a6f) )
	ROM_LOAD16_BYTE( "ds3rom7.bin", 0x00000, 0x80000, CRC(323eff0b) SHA1(5d4945d77191ee44b4fbf125bc0816217321829e) )
ROM_END


ROM_START( hdrivaip )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )		/* 2MB for 68000 code */
	ROM_LOAD16_BYTE( "stest.0h",    0x000000, 0x20000, CRC(bf4bb6a0) SHA1(e38ec5ce245f98bfe8084ba684bffc85dc19d3be) )
	ROM_LOAD16_BYTE( "stest.0l",    0x000001, 0x20000, CRC(f462b511) SHA1(d88efb8cc30322a8332a1f50de775a204758e176) )
	ROM_LOAD16_BYTE( "drive.hi",    0x040000, 0x20000, CRC(56571590) SHA1(d0362b8bd438cd7dfa9ff7cf71307f44c2cfe843) )
	ROM_LOAD16_BYTE( "drive.lo",    0x040001, 0x20000, CRC(799e3138) SHA1(d4b96d8391ff3cf0ea24dfcd8930dd06bfa9d6ce) )
	ROM_LOAD16_BYTE( "wave1.hi",    0x0c0000, 0x20000, CRC(63872d12) SHA1(b56d0c40a7a3c4e4bd17eaf5603c528d17de424f) )
	ROM_LOAD16_BYTE( "wave1.lo",    0x0c0001, 0x20000, CRC(1a472475) SHA1(acfc1b3ce03bd8ce268f00ab76ace6134ad359c3) )
	ROM_LOAD16_BYTE( "ms2pics.hi",  0x140000, 0x20000, CRC(bca0af7e) SHA1(f25cfdc8f8fa77bcca2723335f76ba8a7d790eec) )
	ROM_LOAD16_BYTE( "ms2pics.lo",  0x140001, 0x20000, CRC(c3c6be8c) SHA1(66f0a54979bd83a940f226a8b3a9cf2eb3eaa908) )
	ROM_LOAD16_BYTE( "ms2univ.hi",  0x180000, 0x20000, CRC(59c91b15) SHA1(f35239efebe914e0745a77b6ecfe2d518a90aa9d) )
	ROM_LOAD16_BYTE( "ms2univ.lo",  0x180001, 0x20000, CRC(7493bf60) SHA1(35868c74e9aac6b16a18b67d0136183ea8a8232f) )
	ROM_LOAD16_BYTE( "ms2cproc.0h", 0x1c0000, 0x20000, CRC(19024f2d) SHA1(a94e8836cdc147cea5816b99b8a1ad5ff669d984) )
	ROM_LOAD16_BYTE( "ms2cproc.0l", 0x1c0001, 0x20000, CRC(1e48bd46) SHA1(1a903d889f48604bd8d2d9a0bda4ee20e7ad968b) )

	ROM_REGION( ADSP2100_SIZE, REGION_CPU3, 0 )	/* dummy region for ADSP 2101 */

	ROM_REGION( ADSP2100_SIZE + 0x10000, REGION_CPU4, 0 )	/* dummy region for ADSP 2105 */
	ROM_LOAD( "sboota.bin", ADSP2100_SIZE + 0x00000, 0x10000, CRC(3ef819cd) SHA1(c547b869a3a37a82fb46584fe0ef0cfe21a4f882) )

	ROM_REGION( ADSP2100_SIZE + 0x10000, REGION_CPU5, 0 )	/* dummy region for ADSP 2105 */
	ROM_LOAD( "xboota.bin", ADSP2100_SIZE + 0x00000, 0x10000, CRC(d9c49901) SHA1(9f90ae3a47eb1ef00c3ec3661f60402c2eae2108) )

	ROM_REGION( 0xc0000, REGION_USER1, 0 )		/* 768k for object ROM */
	ROM_LOAD16_BYTE( "objects.0l",  0x00000, 0x20000, CRC(3c9e9078) SHA1(f1daf32117236401f3cb97f332708632003e55f8) )
	ROM_LOAD16_BYTE( "objects.0h",  0x00001, 0x20000, CRC(4480dbae) SHA1(6a455173c38e80093f58bdc322cffcf25e70b6ae) )
	ROM_LOAD16_BYTE( "objects.1l",  0x40000, 0x20000, CRC(700bd978) SHA1(5cd63d4eee00d90fe29fb9697b6a0ea6b86704ae) )
	ROM_LOAD16_BYTE( "objects.1h",  0x40001, 0x20000, CRC(f613adaf) SHA1(9b9456e144a48fb73c5e084b33345667eed4905e) )
	ROM_LOAD16_BYTE( "objects.2l",  0x80000, 0x20000, CRC(e3b512f0) SHA1(080c5a21cb76edcb55d1c2488e9d91cf29cb0665) )
	ROM_LOAD16_BYTE( "objects.2h",  0x80001, 0x20000, CRC(3f83742b) SHA1(4b6e0134a806bcc9bd56432737047f86d0a16424) )

	ROM_REGION16_BE( 0x140000, REGION_USER3, 0 )/* 1MB for DSK ROMs + 256k for RAM */
	ROM_LOAD16_BYTE( "dskpics.hi",  0x00000, 0x80000, CRC(eaa88101) SHA1(ed0ebf8a9a9514d810242b9b552126f6717f9e25) )
	ROM_LOAD16_BYTE( "dskpics.lo",  0x00001, 0x80000, CRC(8c6f0750) SHA1(4cb23cedc500c1509dc875c3291a5771c8473f73) )

	ROM_REGION32_LE( 0x200000, REGION_USER4, 0 )/* 2MB for ASIC61 ROMs */
	ROM_LOAD16_BYTE( "roads.0",     0x000000, 0x80000, CRC(cab2e335) SHA1(914996c5b7905f1c20fcda6972af88debbee59cd) )
	ROM_LOAD16_BYTE( "roads.1",     0x000001, 0x80000, CRC(62c244ba) SHA1(f041a269f35a9d187c90241c5b64173663ad5268) )
	ROM_LOAD16_BYTE( "roads.2",     0x000002, 0x80000, CRC(ba57f415) SHA1(1daf5a014e9bef15466b282bcca2395fec2b0628) )
	ROM_LOAD16_BYTE( "roads.3",     0x000003, 0x80000, CRC(1e6a4ca0) SHA1(2cf06d6c73be11cf10515246fca2baa05ce5091b) )

	ROM_REGION16_BE( 0x100000, REGION_USER5, 0 )
	/* DS IV sound section (2 x ADSP2105)*/
	ROM_LOAD16_BYTE( "ds3rom.0",    0x00001, 0x80000, CRC(90b8dbb6) SHA1(fff693cb81e88bc00e048bb71406295fe7be5122) )
	ROM_LOAD16_BYTE( "ds3rom.1",    0x00000, 0x80000, CRC(03673d8d) SHA1(13596f7acb58fba78d6e4f2ac7bb21d9d2589668) )
	ROM_LOAD16_BYTE( "ds3rom.2",    0x00001, 0x80000, CRC(f67754e9) SHA1(3548412ccdfa9b482942c78778f05d67eb7835ea) )
	ROM_LOAD16_BYTE( "ds3rom.3",    0x00000, 0x80000, CRC(008d3578) SHA1(c9ff50b931c25fe86bde3eb0aae2350c29766438) )
	ROM_LOAD16_BYTE( "ds3rom.4",    0x00001, 0x80000, CRC(6281efee) SHA1(47d0f3ff973166d818877996c45dccf1d3a85fe1) )
	ROM_LOAD16_BYTE( "ds3rom.5",    0x00000, 0x80000, CRC(6ef9ed90) SHA1(8bd927a56fe99f7db96d203c1daeb8c8c83f2c17) )
	ROM_LOAD16_BYTE( "ds3rom.6",    0x00001, 0x80000, CRC(cd4cd6bc) SHA1(95689ab7cb18af54ff09aebf223f6346f13dfd7b) )
	ROM_LOAD16_BYTE( "ds3rom.7",    0x00000, 0x80000, CRC(3d695e1f) SHA1(4e5dd009ed11d299c546451141920dc1dc74a529) )
ROM_END



/*************************************
 *
 *	Common initialization
 *
 *************************************/

/* COMMON INIT: find all the CPUs */
static void find_cpus(void)
{
	hdcpu_main = mame_find_cpu_index("main");
	hdcpu_gsp = mame_find_cpu_index("gsp");
	hdcpu_msp = mame_find_cpu_index("msp");
	hdcpu_adsp = mame_find_cpu_index("adsp");
	hdcpu_sound = mame_find_cpu_index("sound");
	hdcpu_sounddsp = mame_find_cpu_index("sounddsp");
	hdcpu_jsa = mame_find_cpu_index("jsa");
	hdcpu_dsp32 = mame_find_cpu_index("dsp32");
}


static const UINT16 default_eeprom[] =
{
	1,
	0xff00,0xff00,0xff00,0xff00,0xff00,0xff00,0xff00,0xff00,
	0x0800,0
};


/* COMMON INIT: initialize the original "driver" main board */
static void init_driver(void)
{
	/* assume we're first to be called */
	find_cpus();

	/* note that we're not multisync and set the default EEPROM data */
	hdgsp_multisync = 0;
	atarigen_eeprom_default = default_eeprom;
}


/* COMMON INIT: initialize the later "multisync" main board */
static void init_multisync(int compact_inputs)
{
	/* assume we're first to be called */
	find_cpus();

	/* note that we're multisync and set the default EEPROM data */
	hdgsp_multisync = 1;
	atarigen_eeprom_default = default_eeprom;

	/* install handlers for the compact driving games' inputs */
	if (compact_inputs)
	{
		install_mem_read16_handler(hdcpu_main, 0x400000, 0x400001, hdc68k_wheel_r);
		install_mem_write16_handler(hdcpu_main, 0x408000, 0x408001, hdc68k_wheel_edge_reset_w);
		install_mem_read16_handler(hdcpu_main, 0xa80000, 0xafffff, hdc68k_port1_r);
	}
}


/* COMMON INIT: initialize the ADSP/ADSP2 board */
static void init_adsp(void)
{
	/* install ADSP program RAM */
	install_mem_read16_handler(hdcpu_main, 0x800000, 0x807fff, hd68k_adsp_program_r);
	install_mem_write16_handler(hdcpu_main, 0x800000, 0x807fff, hd68k_adsp_program_w);

	/* install ADSP data RAM */
	install_mem_read16_handler(hdcpu_main, 0x808000, 0x80bfff, hd68k_adsp_data_r);
	install_mem_write16_handler(hdcpu_main, 0x808000, 0x80bfff, hd68k_adsp_data_w);

	/* install ADSP serial buffer RAM */
	install_mem_read16_handler(hdcpu_main, 0x810000, 0x813fff, hd68k_adsp_buffer_r);
	install_mem_write16_handler(hdcpu_main, 0x810000, 0x813fff, hd68k_adsp_buffer_w);

	/* install ADSP control locations */
	install_mem_write16_handler(hdcpu_main, 0x818000, 0x81801f, hd68k_adsp_control_w);
	install_mem_write16_handler(hdcpu_main, 0x818060, 0x81807f, hd68k_adsp_irq_clear_w);
	install_mem_read16_handler(hdcpu_main, 0x838000, 0x83ffff, hd68k_adsp_irq_state_r);
}


/* COMMON INIT: initialize the DS3 board */
static void init_ds3(void)
{
	/* install ADSP program RAM */
	install_mem_read16_handler(hdcpu_main, 0x800000, 0x807fff, hd68k_ds3_program_r);
	install_mem_write16_handler(hdcpu_main, 0x800000, 0x807fff, hd68k_ds3_program_w);

	/* install ADSP data RAM */
	install_mem_read16_handler(hdcpu_main, 0x808000, 0x80bfff, hd68k_adsp_data_r);
	install_mem_write16_handler(hdcpu_main, 0x808000, 0x80bfff, hd68k_adsp_data_w);
	install_mem_read16_handler(hdcpu_main, 0x80c000, 0x80dfff, hdds3_special_r);
	install_mem_write16_handler(hdcpu_main, 0x80c000, 0x80dfff, hdds3_special_w);

	/* install ADSP control locations */
	install_mem_read16_handler(hdcpu_main, 0x820000, 0x8207ff, hd68k_ds3_gdata_r);
	install_mem_read16_handler(hdcpu_main, 0x820800, 0x820fff, hd68k_ds3_girq_state_r);
	install_mem_write16_handler(hdcpu_main, 0x820000, 0x8207ff, hd68k_ds3_gdata_w);
	install_mem_write16_handler(hdcpu_main, 0x821000, 0x8217ff, hd68k_adsp_irq_clear_w);
	install_mem_read16_handler(hdcpu_main, 0x822000, 0x8227ff, hd68k_ds3_sdata_r);
	install_mem_read16_handler(hdcpu_main, 0x822800, 0x822fff, hd68k_ds3_sirq_state_r);
	install_mem_write16_handler(hdcpu_main, 0x822000, 0x8227ff, hd68k_ds3_sdata_w);
	install_mem_write16_handler(hdcpu_main, 0x823800, 0x823fff, hd68k_ds3_control_w);

	/* if we have a sound DSP, boot it */
	if (hdcpu_sound != -1 && Machine->drv->cpu[hdcpu_sound].cpu_type == CPU_ADSP2105)
		adsp2105_load_boot_data((data8_t *)(memory_region(REGION_CPU1 + hdcpu_sound) + ADSP2100_SIZE),
								(data32_t *)(memory_region(REGION_CPU1 + hdcpu_sound) + ADSP2100_PGM_OFFSET));
	if (hdcpu_sounddsp != -1 && Machine->drv->cpu[hdcpu_sounddsp].cpu_type == CPU_ADSP2105)
		adsp2105_load_boot_data((data8_t *)(memory_region(REGION_CPU1 + hdcpu_sounddsp) + ADSP2100_SIZE),
								(data32_t *)(memory_region(REGION_CPU1 + hdcpu_sounddsp) + ADSP2100_PGM_OFFSET));

/*


/PMEM   = RVASB & EXTB &         /AB20 & /AB19 & /AB18 & /AB17 & /AB16 & /AB15
        = 0 0000 0xxx xxxx xxxx xxxx (read/write)
        = 0x000000-0x007fff

/DMEM   = RVASB & EXTB &         /AB20 & /AB19 & /AB18 & /AB17 & /AB16 &  AB15
        = 0 0000 1xxx xxxx xxxx xxxx (read/write)
		= 0x008000-0x00ffff

/G68WR  = RVASB & EXTB &  EWRB & /AB20 & /AB19 & /AB18 &  AB17 & /AB16 & /AB15 & /AB14 & /AB13 & /AB12 & /AB11
        = 0 0010 0000 0xxx xxxx xxxx (write)
        = 0x020000-0x0207ff

/G68RD0 = RVASB & EXTB & /EWRB & /AB20 & /AB19 & /AB18 &  AB17 & /AB16 & /AB15 & /AB14 & /AB13 & /AB12 & /AB11
        = 0 0010 0000 0xxx xxxx xxxx (read)
        = 0x020000-0x0207ff

/G68RD1 = RVASB & EXTB & /EWRB & /AB20 & /AB19 & /AB18 &  AB17 & /AB16 & /AB15 & /AB14 & /AB13 & /AB12 &  AB11
        = 0 0010 0000 1xxx xxxx xxxx (read)
        = 0x020800-0x020fff

/GCGINT = RVASB & EXTB &  EWRB & /AB20 & /AB19 & /AB18 &  AB17 & /AB16 & /AB15 & /AB14 & /AB13 &  AB12 & /AB11
        = 0 0010 0001 0xxx xxxx xxxx (write)
        = 0x021000-0x0217ff

/S68WR  = RVASB & EXTB &  EWRB & /AB20 & /AB19 & /AB18 & AB17 & /AB16 & /AB15 & /AB14 & AB13 & /AB12 & /AB11
        = 0 0010 0010 0xxx xxxx xxxx (write)
        = 0x022000-0x0227ff

/S68RD0 = RVASB & EXTB & /EWRB & /AB20 & /AB19 & /AB18 & AB17 & /AB16 & /AB15 & /AB14 & AB13 & /AB12 & /AB11
        = 0 0010 0010 0xxx xxxx xxxx (read)
        = 0x022000-0x0227ff

/S68RD1 = RVASB & EXTB & /EWRB & /AB20 & /AB19 & /AB18 & AB17 & /AB16 & /AB15 & /AB14 & AB13 & /AB12 &  AB11
        = 0 0010 0010 1xxx xxxx xxxx (read)
        = 0x022800-0x022fff

/SCGINT = RVASB & EXTB &  EWRB & /AB20 & /AB19 & /AB18 & AB17 & /AB16 & /AB15 & /AB14 & AB13 &  AB12 & /AB11
        = 0 0010 0011 0xxx xxxx xxxx (write)
        = 0x023000-0x0237ff

/LATCH  = RVASB & EXTB &  EWRB & /AB20 & /AB19 & /AB18 & AB17 & /AB16 & /AB15 & /AB14 & AB13 &  AB12 &  AB11
        = 0 0010 0011 1xxx xxxx xxxx (write)
        = 0x023800-0x023fff




/SBUFF  =         EXTB & /EWRB & /AB20 & /AB19 & /AB18 & AB17 & /AB16 & /AB15 & /AB14 & AB13 & /AB12
        |         EXTB &         /AB20 & /AB19 & /AB18 & AB17 & /AB16 & /AB15 & /AB14 & AB13 & /AB12 & /AB11
		= 0 0010 0010 xxxx xxxx xxxx (read)
		| 0 0010 0010 0xxx xxxx xxxx (read/write)

/GBUFF  =         EXTB &         /AB20 & /AB19 & /AB18 & /AB17 & /AB16
        |         EXTB & /EWRB & /AB20 & /AB19 & /AB18 &         /AB16 & /AB15 & /AB14 & /AB13 &         /AB11
        |         EXTB &         /AB20 & /AB19 & /AB18 &         /AB16 & /AB15 & /AB14 & /AB13 & /AB12 & /AB11
        = 0 0000 xxxx xxxx xxxx xxxx (read/write)
        | 0 00x0 000x 0xxx xxxx xxxx (read)
        | 0 00x0 0000 0xxx xxxx xxxx (read/write)

/GBUFF2 =         EXTB &         /AB20 & /AB19 & /AB18 & /AB17 & /AB16
        = 0 0000 xxxx xxxx xxxx xxxx (read/write)

*/
}


/* COMMON INIT: initialize the DSK add-on board */
static void init_dsk(void)
{
	/* install ASIC61 */
	install_mem_write16_handler(hdcpu_main, 0x85c000, 0x85c7ff, hd68k_dsk_dsp32_w);
	install_mem_read16_handler(hdcpu_main, 0x85c000, 0x85c7ff, hd68k_dsk_dsp32_r);

	/* install control registers */
	install_mem_write16_handler(hdcpu_main, 0x85c800, 0x85c81f, hd68k_dsk_control_w);

	/* install extra RAM */
	install_mem_read16_handler(hdcpu_main, 0x900000, 0x90ffff, hd68k_dsk_ram_r);
	install_mem_write16_handler(hdcpu_main, 0x900000, 0x90ffff, hd68k_dsk_ram_w);
	hddsk_ram = (data16_t *)(memory_region(REGION_USER3) + 0x40000);

	/* install extra ZRAM */
	install_mem_read16_handler(hdcpu_main, 0x910000, 0x910fff, hd68k_dsk_zram_r);
	install_mem_write16_handler(hdcpu_main, 0x910000, 0x910fff, hd68k_dsk_zram_w);
	hddsk_zram = (data16_t *)(memory_region(REGION_USER3) + 0x50000);

	/* install ASIC65 */
	install_mem_write16_handler(hdcpu_main, 0x914000, 0x917fff, asic65_data_w);
	install_mem_read16_handler(hdcpu_main, 0x914000, 0x917fff, asic65_r);
	install_mem_read16_handler(hdcpu_main, 0x918000, 0x91bfff, asic65_io_r);

	/* install extra ROM */
	install_mem_read16_handler(hdcpu_main, 0x940000, 0x9fffff, hd68k_dsk_small_rom_r);
	hddsk_rom = (data16_t *)(memory_region(REGION_USER3) + 0x00000);

	/* set up the ASIC65 */
	asic65_config(ASIC65_STANDARD);
}


/* COMMON INIT: initialize the DSK II add-on board */
static void init_dsk2(void)
{
	/* install ASIC65 */
	install_mem_write16_handler(hdcpu_main, 0x824000, 0x824003, asic65_data_w);
	install_mem_read16_handler(hdcpu_main, 0x824000, 0x824003, asic65_r);
	install_mem_read16_handler(hdcpu_main, 0x825000, 0x825001, asic65_io_r);

	/* install ASIC61 */
	install_mem_write16_handler(hdcpu_main, 0x827000, 0x8277ff, hd68k_dsk_dsp32_w);
	install_mem_read16_handler(hdcpu_main, 0x827000, 0x8277ff, hd68k_dsk_dsp32_r);

	/* install control registers */
	install_mem_write16_handler(hdcpu_main, 0x827800, 0x82781f, hd68k_dsk_control_w);

	/* install extra RAM */
	install_mem_read16_handler(hdcpu_main, 0x880000, 0x8bffff, hd68k_dsk_ram_r);
	install_mem_write16_handler(hdcpu_main, 0x880000, 0x8bffff, hd68k_dsk_ram_w);
	hddsk_ram = (data16_t *)(memory_region(REGION_USER3) + 0x100000);

	/* install extra ROM */
	install_mem_read16_handler(hdcpu_main, 0x900000, 0x9fffff, hd68k_dsk_rom_r);
	hddsk_rom = (data16_t *)(memory_region(REGION_USER3) + 0x000000);

	/* set up the ASIC65 */
	asic65_config(ASIC65_STANDARD);
}


/* COMMON INIT: initialize the DSPCOM add-on board */
static void init_dspcom(void)
{
	/* install ASIC65 */
	install_mem_write16_handler(hdcpu_main, 0x900000, 0x900003, asic65_data_w);
	install_mem_read16_handler(hdcpu_main, 0x900000, 0x900003, asic65_r);
	install_mem_read16_handler(hdcpu_main, 0x901000, 0x910001, asic65_io_r);

	/* set up the ASIC65 */
	asic65_config(ASIC65_STEELTAL);

	/* install DSPCOM control */
	install_mem_write16_handler(hdcpu_main, 0x904000, 0x90401f, hddspcom_control_w);
}


/* COMMON INIT: initialize the original "driver" sound board */
static void init_driver_sound(void)
{
	hdsnd_init();

	/* install sound handlers */
	install_mem_write16_handler(hdcpu_main, 0x840000, 0x840001, hd68k_snd_data_w);
	install_mem_read16_handler(hdcpu_main, 0x840000, 0x840001, hd68k_snd_data_r);
	install_mem_read16_handler(hdcpu_main, 0x844000, 0x844001, hd68k_snd_status_r);
	install_mem_write16_handler(hdcpu_main, 0x84c000, 0x84c001, hd68k_snd_reset_w);
}




/*************************************
 *
 *	Driver initialization
 *
 *************************************/

static DRIVER_INIT( harddriv )
{
	/* initialize the boards */
	init_driver();
	init_adsp();
	init_driver_sound();

	/* set up gsp speedup handler */
	hdgsp_speedup_addr[0] = install_mem_write16_handler(hdcpu_gsp, TOBYTE(0xfff9fc00), TOBYTE(0xfff9fc0f), hdgsp_speedup1_w);
	hdgsp_speedup_addr[1] = install_mem_write16_handler(hdcpu_gsp, TOBYTE(0xfffcfc00), TOBYTE(0xfffcfc0f), hdgsp_speedup2_w);
	install_mem_read16_handler(hdcpu_gsp, TOBYTE(0xfff9fc00), TOBYTE(0xfff9fc0f), hdgsp_speedup_r);
	hdgsp_speedup_pc = 0xffc00f10;

	/* set up msp speedup handler */
	hdmsp_speedup_addr = install_mem_write16_handler(hdcpu_msp, TOBYTE(0x00751b00), TOBYTE(0x00751b0f), hdmsp_speedup_w);
	install_mem_read16_handler(hdcpu_msp, TOBYTE(0x00751b00), TOBYTE(0x00751b0f), hdmsp_speedup_r);
	hdmsp_speedup_pc = 0x00723b00;

	/* set up adsp speedup handlers */
	install_mem_read16_handler(hdcpu_adsp, ADSP_DATA_ADDR_RANGE(0x1fff, 0x1fff), hdadsp_speedup_r);
}


static DRIVER_INIT( harddrvc )
{
	/* initialize the boards */
	init_multisync(1);
	init_adsp();
	init_driver_sound();

	/* set up gsp speedup handler */
	hdgsp_speedup_addr[0] = install_mem_write16_handler(hdcpu_gsp, TOBYTE(0xfff9fc00), TOBYTE(0xfff9fc0f), hdgsp_speedup1_w);
	hdgsp_speedup_addr[1] = install_mem_write16_handler(hdcpu_gsp, TOBYTE(0xfffcfc00), TOBYTE(0xfffcfc0f), hdgsp_speedup2_w);
	install_mem_read16_handler(hdcpu_gsp, TOBYTE(0xfff9fc00), TOBYTE(0xfff9fc0f), hdgsp_speedup_r);
	hdgsp_speedup_pc = 0xfff40ff0;

	/* set up msp speedup handler */
	hdmsp_speedup_addr = install_mem_write16_handler(hdcpu_msp, TOBYTE(0x00751b00), TOBYTE(0x00751b0f), hdmsp_speedup_w);
	install_mem_read16_handler(hdcpu_msp, TOBYTE(0x00751b00), TOBYTE(0x00751b0f), hdmsp_speedup_r);
	hdmsp_speedup_pc = 0x00723b00;

	/* set up adsp speedup handlers */
	install_mem_read16_handler(hdcpu_adsp, ADSP_DATA_ADDR_RANGE(0x1fff, 0x1fff), hdadsp_speedup_r);
}


static DRIVER_INIT( stunrun )
{
	/* initialize the boards */
	init_multisync(0);
	init_adsp();
	atarijsa_init(hdcpu_jsa, 14, 0, 0x0020);

	/* set up gsp speedup handler */
	hdgsp_speedup_addr[0] = install_mem_write16_handler(hdcpu_gsp, TOBYTE(0xfff9fc00), TOBYTE(0xfff9fc0f), hdgsp_speedup1_w);
	hdgsp_speedup_addr[1] = install_mem_write16_handler(hdcpu_gsp, TOBYTE(0xfffcfc00), TOBYTE(0xfffcfc0f), hdgsp_speedup2_w);
	install_mem_read16_handler(hdcpu_gsp, TOBYTE(0xfff9fc00), TOBYTE(0xfff9fc0f), hdgsp_speedup_r);
	hdgsp_speedup_pc = 0xfff41070;

	/* set up adsp speedup handlers */
	install_mem_read16_handler(hdcpu_adsp, ADSP_DATA_ADDR_RANGE(0x1fff, 0x1fff), hdadsp_speedup_r);
}


data32_t *rddsp32_speedup;
offs_t rddsp32_speedup_pc;
READ32_HANDLER( rddsp32_speedup_r )
{
	if (activecpu_get_pc() == rddsp32_speedup_pc && (*rddsp32_speedup >> 16) == 0)
	{
		UINT32 r14 = activecpu_get_reg(DSP32_R14);
		UINT32 r1 = cpu_readmem24ledw_word(r14 - 0x14);
		int cycles_to_burn = 17 * 4 * (0x2bc - r1 - 2);
		if (cycles_to_burn > 20 * 4)
		{
			int icount_remaining = activecpu_get_icount();
			if (cycles_to_burn > icount_remaining)
				cycles_to_burn = icount_remaining;
			activecpu_adjust_icount(-cycles_to_burn);
			cpu_writemem24ledw_word(r14 - 0x14, r1 + cycles_to_burn / 17);
		}
		msp_speedup_count[0]++;
	}
	return *rddsp32_speedup;
}


static DRIVER_INIT( racedriv )
{
	/* initialize the boards */
	init_driver();
	init_adsp();
	init_dsk();
	init_driver_sound();

	/* set up the slapstic */
	slapstic_init(117);
	hd68k_slapstic_base = install_mem_read16_handler(hdcpu_main, 0xe0000, 0xfffff, rd68k_slapstic_r);
	hd68k_slapstic_base = install_mem_write16_handler(hdcpu_main, 0xe0000, 0xfffff, rd68k_slapstic_w);

	/* synchronization */
	rddsp32_sync[0] = install_mem_write32_handler(hdcpu_dsp32, 0x613c00, 0x613c03, rddsp32_sync0_w);
	rddsp32_sync[1] = install_mem_write32_handler(hdcpu_dsp32, 0x613e00, 0x613e03, rddsp32_sync1_w);

	/* set up adsp speedup handlers */
	install_mem_read16_handler(hdcpu_adsp, ADSP_DATA_ADDR_RANGE(0x1fff, 0x1fff), hdadsp_speedup_r);

	/* set up dsp32 speedup handlers */
	rddsp32_speedup = install_mem_read32_handler(hdcpu_dsp32, 0x613e04, 0x613e07, rddsp32_speedup_r);
	rddsp32_speedup_pc = 0x6054b0;
}


static void racedrvc_init_common(offs_t gsp_protection)
{
	/* initialize the boards */
	init_multisync(1);
	init_adsp();
	init_dsk();
	init_driver_sound();

	/* set up the slapstic */
	slapstic_init(117);
	hd68k_slapstic_base = install_mem_read16_handler(hdcpu_main, 0xe0000, 0xfffff, rd68k_slapstic_r);
	hd68k_slapstic_base = install_mem_write16_handler(hdcpu_main, 0xe0000, 0xfffff, rd68k_slapstic_w);

	/* synchronization */
	rddsp32_sync[0] = install_mem_write32_handler(hdcpu_dsp32, 0x613c00, 0x613c03, rddsp32_sync0_w);
	rddsp32_sync[1] = install_mem_write32_handler(hdcpu_dsp32, 0x613e00, 0x613e03, rddsp32_sync1_w);

	/* set up protection hacks */
	hdgsp_protection = install_mem_write16_handler(hdcpu_gsp, TOBYTE(gsp_protection), TOBYTE(gsp_protection + 0x0f), hdgsp_protection_w);

	/* set up gsp speedup handler */
	hdgsp_speedup_addr[0] = install_mem_write16_handler(hdcpu_gsp, TOBYTE(0xfff76f60), TOBYTE(0xfff76f6f), rdgsp_speedup1_w);
	install_mem_read16_handler(hdcpu_gsp, TOBYTE(0xfff76f60), TOBYTE(0xfff76f6f), rdgsp_speedup1_r);
	hdgsp_speedup_pc = 0xfff43a00;

	/* set up adsp speedup handlers */
	install_mem_read16_handler(hdcpu_adsp, ADSP_DATA_ADDR_RANGE(0x1fff, 0x1fff), hdadsp_speedup_r);

	/* set up dsp32 speedup handlers */
	rddsp32_speedup = install_mem_read32_handler(hdcpu_dsp32, 0x613e04, 0x613e07, rddsp32_speedup_r);
	rddsp32_speedup_pc = 0x6054b0;
}

static DRIVER_INIT( racedrvc ) { racedrvc_init_common(0xfff95cd0); }
static DRIVER_INIT( racedrc1 ) { racedrvc_init_common(0xfff7ecd0); }


static READ16_HANDLER( steeltal_dummy_r )
{
	/* this is required so that INT 4 is recongized as a sound INT */
	return ~0;
}


static void steeltal_init_common(offs_t ds3_transfer_pc, int proto_sloop)
{
	/* initialize the boards */
	init_multisync(0);
	init_ds3();
	init_dspcom();
	atarijsa3_init_adpcm(REGION_SOUND1);
	atarijsa_init(hdcpu_jsa, 14, 0, 0x0020);

	install_mem_read16_handler(hdcpu_main, 0x908000, 0x908001, steeltal_dummy_r);

	/* set up the SLOOP */
	if (!proto_sloop)
	{
		hd68k_slapstic_base = install_mem_read16_handler(hdcpu_main, 0xe0000, 0xfffff, st68k_sloop_r);
		hd68k_slapstic_base = install_mem_write16_handler(hdcpu_main, 0xe0000, 0xfffff, st68k_sloop_w);
		st68k_sloop_alt_base = install_mem_read16_handler(hdcpu_main, 0x4e000, 0x4ffff, st68k_sloop_alt_r);
	}
	else
	{
		hd68k_slapstic_base = install_mem_read16_handler(hdcpu_main, 0xe0000, 0xfffff, st68k_protosloop_r);
		hd68k_slapstic_base = install_mem_write16_handler(hdcpu_main, 0xe0000, 0xfffff, st68k_protosloop_w);
	}

	/* synchronization */
	stmsp_sync[0] = &hdmsp_ram[TOWORD(0x80010)];
	install_mem_write16_handler(hdcpu_msp, TOBYTE(0x80010), TOBYTE(0x8007f), stmsp_sync0_w);
	stmsp_sync[1] = &hdmsp_ram[TOWORD(0x99680)];
	install_mem_write16_handler(hdcpu_msp, TOBYTE(0x99680), TOBYTE(0x9968f), stmsp_sync1_w);
	stmsp_sync[2] = &hdmsp_ram[TOWORD(0x99d30)];
	install_mem_write16_handler(hdcpu_msp, TOBYTE(0x99d30), TOBYTE(0x99d50), stmsp_sync2_w);

	/* set up protection hacks */
	hdgsp_protection = install_mem_write16_handler(hdcpu_gsp, TOBYTE(0xfff965d0), TOBYTE(0xfff965df), hdgsp_protection_w);

	/* set up msp speedup handlers */
	install_mem_read16_handler(hdcpu_msp, TOBYTE(0x80020), TOBYTE(0x8002f), stmsp_speedup_r);

	/* set up adsp speedup handlers */
	install_mem_read16_handler(hdcpu_adsp, ADSP_DATA_ADDR_RANGE(0x1fff, 0x1fff), hdadsp_speedup_r);
	install_mem_read16_handler(hdcpu_adsp, ADSP_DATA_ADDR_RANGE(0x1f99, 0x1f99), hdds3_speedup_r);
	hdds3_speedup_addr = (data16_t *)(memory_region(REGION_CPU1 + hdcpu_adsp) + ADSP2100_DATA_OFFSET) + 0x1f99;
	hdds3_speedup_pc = 0xff;
	hdds3_transfer_pc = ds3_transfer_pc;
}


static DRIVER_INIT( steeltal ) { steeltal_init_common(0x4fc18, 0); }
static DRIVER_INIT( steelta1 ) { steeltal_init_common(0x4f9c6, 0); }
static DRIVER_INIT( steeltap ) { steeltal_init_common(0x52290, 1); }


static DRIVER_INIT( hdrivair )
{
	/* initialize the boards */
	init_multisync(1);
	init_ds3();
	init_dsk2();

	install_mem_read16_handler(hdcpu_main, 0xa80000, 0xafffff, hda68k_port1_r);

	/* synchronization */
	rddsp32_sync[0] = install_mem_write32_handler(hdcpu_dsp32, 0x21fe00, 0x21fe03, rddsp32_sync0_w);
	rddsp32_sync[1] = install_mem_write32_handler(hdcpu_dsp32, 0x21ff00, 0x21ff03, rddsp32_sync1_w);

	/* set up protection hacks */
	hdgsp_protection = install_mem_write16_handler(hdcpu_gsp, TOBYTE(0xfff943f0), TOBYTE(0xfff943ff), hdgsp_protection_w);

	/* set up adsp speedup handlers */
	install_mem_read16_handler(hdcpu_adsp, ADSP_DATA_ADDR_RANGE(0x1fff, 0x1fff), hdadsp_speedup_r);
	install_mem_read16_handler(hdcpu_adsp, ADSP_DATA_ADDR_RANGE(0x1f99, 0x1f99), hdds3_speedup_r);
	hdds3_speedup_addr = (data16_t *)(memory_region(REGION_CPU1 + hdcpu_adsp) + ADSP2100_DATA_OFFSET) + 0x1f99;
	hdds3_speedup_pc = 0x2da;
	hdds3_transfer_pc = 0x407b8;
}


static DRIVER_INIT( hdrivaip )
{
	/* initialize the boards */
	init_multisync(1);
	init_ds3();
	init_dsk2();

	install_mem_read16_handler(hdcpu_main, 0xa80000, 0xafffff, hda68k_port1_r);

	/* synchronization */
	rddsp32_sync[0] = install_mem_write32_handler(hdcpu_dsp32, 0x21fe00, 0x21fe03, rddsp32_sync0_w);
	rddsp32_sync[1] = install_mem_write32_handler(hdcpu_dsp32, 0x21ff00, 0x21ff03, rddsp32_sync1_w);

	/* set up protection hacks */
	hdgsp_protection = install_mem_write16_handler(hdcpu_gsp, TOBYTE(0xfff916c0), TOBYTE(0xfff916cf), hdgsp_protection_w);

	/* set up adsp speedup handlers */
	install_mem_read16_handler(hdcpu_adsp, ADSP_DATA_ADDR_RANGE(0x1fff, 0x1fff), hdadsp_speedup_r);
	install_mem_read16_handler(hdcpu_adsp, ADSP_DATA_ADDR_RANGE(0x1f9a, 0x1f9a), hdds3_speedup_r);
	hdds3_speedup_addr = (data16_t *)(memory_region(REGION_CPU1 + hdcpu_adsp) + ADSP2100_DATA_OFFSET) + 0x1f9a;
	hdds3_speedup_pc = 0x2d9;
	hdds3_transfer_pc = 0X407da;
}



/*************************************
 *
 *	Game driver(s)
 *
 *************************************/

GAME ( 1988, harddriv, 0,        harddriv, harddriv, harddriv, ROT0, "Atari Games", "Hard Drivin' (cockpit, rev 7)" )
GAME ( 1988, harddrvb, harddriv, harddriv, harddriv, harddriv, ROT0, "Atari Games", "Hard Drivin' (cockpit, British, rev 7)" )
GAME ( 1988, harddrvg, harddriv, harddriv, harddriv, harddriv, ROT0, "Atari Games", "Hard Drivin' (cockpit, German, rev 7)" )
GAME ( 1988, harddrvj, harddriv, harddriv, harddriv, harddriv, ROT0, "Atari Games", "Hard Drivin' (cockpit, Japan, rev 7)" )
GAME ( 1988, harddrb6, harddriv, harddriv, harddriv, harddriv, ROT0, "Atari Games", "Hard Drivin' (cockpit, British, rev 6)" )
GAME ( 1988, harddrj6, harddriv, harddriv, harddriv, harddriv, ROT0, "Atari Games", "Hard Drivin' (cockpit, Japan, rev 6)" )
GAME ( 1988, harddrb5, harddriv, harddriv, harddriv, harddriv, ROT0, "Atari Games", "Hard Drivin' (cockpit, British, rev 5)" )
GAME ( 1988, harddrg4, harddriv, harddriv, harddriv, harddriv, ROT0, "Atari Games", "Hard Drivin' (cockpit, German, rev 4)" )
GAME ( 1988, harddrv3, harddriv, harddriv, harddriv, harddriv, ROT0, "Atari Games", "Hard Drivin' (cockpit, rev 3)" )
GAME ( 1988, harddrv2, harddriv, harddriv, harddriv, harddriv, ROT0, "Atari Games", "Hard Drivin' (cockpit, rev 2)" )
GAMEX( 1988, harddrv1, harddriv, harddriv, harddriv, harddriv, ROT0, "Atari Games", "Hard Drivin' (cockpit, rev 1)", GAME_NOT_WORKING )

GAME ( 1990, harddrvc, harddriv, harddrvc, racedrvc, harddrvc, ROT0, "Atari Games", "Hard Drivin' (compact, rev 2)" )
GAME ( 1990, harddrcg, harddriv, harddrvc, racedrvc, harddrvc, ROT0, "Atari Games", "Hard Drivin' (compact, German, rev 2)" )
GAME ( 1990, harddrcb, harddriv, harddrvc, racedrvc, harddrvc, ROT0, "Atari Games", "Hard Drivin' (compact, British, rev 2)" )
GAME ( 1990, harddrc1, harddriv, harddrvc, racedrvc, harddrvc, ROT0, "Atari Games", "Hard Drivin' (compact, rev 1)" )

GAME ( 1989, stunrun,  0,        stunrun,  stunrun,  stunrun,  ROT0, "Atari Games", "S.T.U.N. Runner (rev 6)" )
GAME ( 1989, stunrunj, stunrun,  stunrun,  stunrun,  stunrun,  ROT0, "Atari Games", "S.T.U.N. Runner (rev 7, Japan)" )
GAME ( 1989, stunrun5, stunrun,  stunrun,  stunrun,  stunrun,  ROT0, "Atari Games", "S.T.U.N. Runner (rev 5)" )
GAME ( 1989, stunrune, stunrun,  stunrun,  stunrun,  stunrun,  ROT0, "Atari Games", "S.T.U.N. Runner (rev 5, Europe)" )
GAME ( 1989, stunrun4, stunrun,  stunrun,  stunrun,  stunrun,  ROT0, "Atari Games", "S.T.U.N. Runner (rev 4)" )
GAME ( 1989, stunrun3, stunrun,  stunrun,  stunrun,  stunrun,  ROT0, "Atari Games", "S.T.U.N. Runner (rev 3)" )
GAME ( 1989, stunrn3e, stunrun,  stunrun,  stunrun,  stunrun,  ROT0, "Atari Games", "S.T.U.N. Runner (rev 3, Europe)" )
GAME ( 1989, stunrun2, stunrun,  stunrun,  stunrun,  stunrun,  ROT0, "Atari Games", "S.T.U.N. Runner (rev 2)" )
GAME ( 1989, stunrn2e, stunrun,  stunrun,  stunrun,  stunrun,  ROT0, "Atari Games", "S.T.U.N. Runner (rev 2, Europe)" )
GAME ( 1989, stunrun0, stunrun,  stunrun,  stunrun,  stunrun,  ROT0, "Atari Games", "S.T.U.N. Runner (rev 0)" )
GAME ( 1989, stunrunp, stunrun,  stunrun,  stunrun,  stunrun,  ROT0, "Atari Games", "S.T.U.N. Runner (upright prototype)" )

GAME ( 1990, racedriv, 0,        racedriv, racedriv, racedriv, ROT0, "Atari Games", "Race Drivin' (cockpit, rev 5)" )
GAME ( 1990, racedrvb, racedriv, racedriv, racedriv, racedriv, ROT0, "Atari Games", "Race Drivin' (cockpit, British, rev 5)" )
GAME ( 1990, racedrvg, racedriv, racedriv, racedriv, racedriv, ROT0, "Atari Games", "Race Drivin' (cockpit, German, rev 5)" )
GAME ( 1990, racedrv4, racedriv, racedriv, racedriv, racedriv, ROT0, "Atari Games", "Race Drivin' (cockpit, rev 4)" )
GAME ( 1990, racedrb4, racedriv, racedriv, racedriv, racedriv, ROT0, "Atari Games", "Race Drivin' (cockpit, British, rev 4)" )
GAME ( 1990, racedrg4, racedriv, racedriv, racedriv, racedriv, ROT0, "Atari Games", "Race Drivin' (cockpit, German, rev 4)" )
GAME ( 1990, racedrv3, racedriv, racedriv, racedriv, racedriv, ROT0, "Atari Games", "Race Drivin' (cockpit, rev 3)" )
GAMEX( 1990, racedrv2, racedriv, racedriv, racedriv, racedriv, ROT0, "Atari Games", "Race Drivin' (cockpit, rev 2)", GAME_NOT_WORKING )
GAMEX( 1990, racedrv1, racedriv, racedriv, racedriv, racedriv, ROT0, "Atari Games", "Race Drivin' (cockpit, rev 1)", GAME_NOT_WORKING )
GAMEX( 1990, racedrb1, racedriv, racedriv, racedriv, racedriv, ROT0, "Atari Games", "Race Drivin' (cockpit, British, rev 1)", GAME_NOT_WORKING )
GAMEX( 1990, racedrg1, racedriv, racedriv, racedriv, racedriv, ROT0, "Atari Games", "Race Drivin' (cockpit, German, rev 2)", GAME_NOT_WORKING )

GAME ( 1990, racedrvc, racedriv, racedrvc, racedrvc, racedrvc, ROT0, "Atari Games", "Race Drivin' (compact, rev 5)" )
GAME ( 1990, racedrcb, racedriv, racedrvc, racedrvc, racedrvc, ROT0, "Atari Games", "Race Drivin' (compact, British, rev 5)" )
GAME ( 1990, racedrcg, racedriv, racedrvc, racedrvc, racedrvc, ROT0, "Atari Games", "Race Drivin' (compact, German, rev 5)" )
GAME ( 1990, racedrc4, racedriv, racedrvc, racedrvc, racedrvc, ROT0, "Atari Games", "Race Drivin' (compact, rev 4)" )
GAME ( 1990, racedcb4, racedriv, racedrvc, racedrvc, racedrvc, ROT0, "Atari Games", "Race Drivin' (compact, British, rev 4)" )
GAME ( 1990, racedcg4, racedriv, racedrvc, racedrvc, racedrvc, ROT0, "Atari Games", "Race Drivin' (compact, German, rev 4)" )
GAME ( 1990, racedrc2, racedriv, racedrvc, racedrvc, racedrc1, ROT0, "Atari Games", "Race Drivin' (compact, rev 2)" )
GAME ( 1990, racedrc1, racedriv, racedrvc, racedrvc, racedrc1, ROT0, "Atari Games", "Race Drivin' (compact, rev 1)" )

GAME ( 1991, steeltal, 0,        steeltal, steeltal, steeltal, ROT0, "Atari Games", "Steel Talons (rev 2)" )
GAME ( 1991, steeltag, steeltal, steeltal, steeltal, steeltal, ROT0, "Atari Games", "Steel Talons (German, rev 2)" )
GAME ( 1991, steelta1, steeltal, steeltal, steeltal, steelta1, ROT0, "Atari Games", "Steel Talons (rev 1)" )
GAMEX( 1991, steeltap, steeltal, steeltal, steeltal, steeltap, ROT0, "Atari Games", "Steel Talons (prototype)", GAME_NOT_WORKING )

GAMEX( 1993, hdrivair, 0,        hdrivair, hdrivair, hdrivair, ROT0, "Atari Games", "Hard Drivin's Airborne (prototype)", GAME_NO_SOUND )
GAMEX( 1993, hdrivaip, hdrivair, hdrivair, hdrivair, hdrivaip, ROT0, "Atari Games", "Hard Drivin's Airborne (prototype, early rev)", GAME_NOT_WORKING | GAME_NO_SOUND )
