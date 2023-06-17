/***********************************************************************************************

	Sega System C/C2 Driver
	driver by David Haywood and Aaron Giles
	---------------------------------------
	Version 0.81u7 - 22 Apr 2004

	Latest Changes :
	-----+-------------------------------------------------------------------------------------
	0.81u7| Various Megaplay Improvements (BR, GreyRouge) (using version number of mame)
	0.73 | More of the Megaplay Bios Tests Pass (BR)
	0.722| Improvements to Megaplay (BR) Added a few more Megatech sets but some are bad (DH)
		 | --2 screens is going to cause problems with partial refresh and get_scanlines for
		 | Megatech
	0.71 | Started adding Support for Megatech, first step is improving the Genesis Emulation
		 | The rendering will also need improving to support mid-frame palette changes
		 | (RGB_DIRECT) maybe use tilemaps if its not too messy, added Genesis sound based on
		 | Mess code
	0.54 | Added Ribbit! support. Recoded some of the VDP to handle vertical windows. Removed
	     | the bitmap cache and added partial updating support.
	0.53 | Set the Protection Read / Write buffers to be cleared at reset in init_machine, this
		 | fixes a problem with columns when you reset it and then attempted to play.
		 |  (only drivers/segac2.c was changed)
	0.52 | Added basic save state support .. not too sure how well it will work (seems to be
		 | ok apart from sound where i guess the sound core needs updating) size of states
		 | could probably be cut down a bit with an init variables from vdp registers function
	0.51 | Added some Puyo Puyo (English Lang) Bootleg set, we don't have an *original* english
		 | set yet however.  Also added a 2nd Bootleg of Tant-R, this one still has protection
		 | inplace, however the apparently board lacks the sample playing hardware.
		 | Removed 'Button3' from Puyo Puyo 2, its unneeded, the Rannyu Button is actually the
		 | player 1 start button, its used for stopping a 2nd player interupting play.
	0.50 | Added some Columns/Puyo Puyo 2 dips from Gerardo. Fixed nasty crash bug for games
		 | not using the UPD7759. Made some minor tweaks to the VDP register accesses. Added
		 | counter/timer description. Attempted to hook up battery RAM.
	0.49 | Fixed clock speeds on sound chips and CPU. Tweaked the highlight color a bit.
	0.48 | Added a kludge to make Stack Columns high score work. Fixed several video-related
		 | differences from c2emu which caused problems in Puyo Puyo.
	0.47 | Fixed Thunder Force hanging problem. No longer calling the UPD7759 on the C-system
		 | games.
	0.46 | Cleaned up the Dip Switches. Added Ichidant and Puyopuy2 Dips.
		 | Todo: Complete Columns and Bloxeedc Dips.
	0.45 | Fixed interrupt timing to match c2emu, which fixes scroll effects in several games.
		 | Swapped sample ROM loading in some bootlegs. Added support for screen disable.
	0.44 | Major cleanup. Figured out general protection mechanism. Added INT 4 and INT 2
		 | support. Added shadow effects. Hooked up sound chips. Fixed palette banking. ASG
	0.43 | Added Support for an English Language version of Ichidant-R (and I didn't even think
		 | it was released outside of Japan, this is a nice Turn-Up) .. I wonder if Tant-R was
		 | also released in English ...
	0.42 | Removed WRITE_WORD which was being used to patch the roms as it is now obsolete,
		 | replaced it with mem16[addy >> 1] = 0xXXXX which is the newer way of doing things.
	0.41 | Mapped more Dipswitches and the odd minor fix.
	0.40 | Updated the Driver so it 'works' with the new memory system introduced in 0.37b8
		 | I'm pretty sure I've not done this right in places (for example writes to VRAM
		 | should probably use data16_t or something.  <request>Help</request> :)
		 | Also Added dipswitches to a few more of the games (Borench & PotoPoto) zzzz.
	0.38 | Started Mapping Dipswitches & Controls on a Per Game basis.  Thunderforce AC now has
		 | this information correct.
	0.37 | Set it to Clear Video Memory in vh_start, this stops the corrupt tile in Puyo Puyo 2
		 | however the game isn't really playable past the first level anyhow due to protection
	0.36 | Mainly a tidy-up release, trying to make the thing more readable :)
		 | also very minor fixes & changes
	0.35 | Added Window Emualtion, Horizontal Screen Splitting only (none of the C2 Games
		 | split the screen Vertically AFAIK so that element of the Genesis Hardware doesn't
		 | need to be emulated here.
		 | Thunderforce AC (bootleg)  tfrceacb is now playable and the test screens in Potopoto
		 | will display correctly.  Don't think I broke anything but knowing me :)
	0.34 | Fixed Some Things I accidentally broke (well forgot to put back) for the
		 | previous version ...
	0.33 | Fixed the Driver to Work in Pure Dos Mode (misunderstanding of the way MAME
		 | allocated (or in this case didn't allocate) the Palette Ram.
		 | Fixed a minor bug which was causing some problems in the scrolling in
		 | potopoto's conveyer belt.
	0.32 | Added Sprite Flipping This improves the GFX in several games, Tant-R (bootleg)
		 | is probably now playable just with bad colours on the status bar due to missing
		 | IRQ 4 Emulation.  Sprite Priority Also fixed .. another Typo
	0.31 | Fixed Several Stupid Typo Type Bugs :p
	-----+-------------------------------------------------------------------------------------

	Sega's C2 was used between 1989 and 1994, the hardware being very similar to that
	used by the Sega MegaDrive/Genesis Home Console Sega produced around the same time.

	Year  Game                  Developer         Versions Dumped  Board  Status        Gen Ver Exists?
	====  ====================  ================  ===============  =====  ============  ===============
	1989  Bloxeed               Sega / Elorg      Eng              C      Playable      n
	1990  Columns               Sega              Jpn              C      Playable      y
	1990  Columns II            Sega              Jpn              C      Playable      n
	1990  Borench               Sega              Eng              C2     Playable      n
	1990  ThunderForce AC       Sega / Technosoft Eng, Jpn, EngBL  C2     Playable      y (as ThunderForce 3?)
	1992  Ribbit!               Sega              Eng?             C2     Playable      ?
	1992  Tant-R                Sega              Jpn, JpnBL       C2     Playable      y
	1992  Puyo Puyo             Sega / Compile    Jpn (2 Vers)     C2     Playable      y
	1994  Ichidant-R            Sega              Jpn              C2     Playable      y
	1994  PotoPoto              Sega              Jpn              C2     Playable      n
	1994  Puyo Puyo 2           Compile           Jpn              C2     Playable      y
	1994  Stack Columns         Sega              Jpn              C2     Playable      n
	1994  Zunzunkyou No Yabou   Sega              Jpn              C2     Playable      n


	Notes:	Eng indicates game is in the English Language, Most Likely a European / US Romset
			Jpn indicates the game plays in Japanese and is therefore likely a Japanes Romset

			Another way to play these games is with Charles Macdonald's C2Emu, which was the
			inspiration for much of this code. Visit http://cgfm2.emuviews.com for the
			download, also home of some _very_ good Sega Genesis VDP documentation.

			The ASM 68k Core causes a scoring problem in Columns, Starscream does this also,
			with the C 68k Core the scoring in columns is correct.

			Bloxeed doesn't Read from the Protection Chip at all; all of the other games do.
			Currently the protection chip is mostly understood, and needs a table of 256
			4-bit values for each game. In all cases except for Poto Poto and Puyo Puyo 2,
			the table is embedded in the code. Workarounds for the other 2 cases are
			provided.

			I'm assuming System-C was the Board without the uPD7759 chip and System-C2 was the
			version of the board with it, this could be completely wrong but it doesn't really
			matter anyway.

			Vertical 2 Cell Based Scrolling won't be 100% accurate on a line that uses a Line
			Scroll value which isn't divisible by 8.. I've never seen a C2 game do this tho.


	Bugs:	Puyo Puyo ends up with a black screen after doing memory tests
			Battery-backed RAM needs to be figured out


	Thanks:	(in no particular order) to any MameDev that helped me out .. (OG, Mish etc.)
			Charles MacDonald for his C2Emu .. without it working out what were bugs in my code
				and issues due to protection would have probably killed the driver long ago :p
			Razoola & Antiriad .. for helping teach me some 68k ASM needed to work out just why
				the games were crashing :)
			Sega for producing some Fantastic Games...
			and anyone else who knows they've contributed :)

***********************************************************************************************/


#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m68000/m68000.h"
#include "state.h"
#include "segac2.h"
#include "machine/random.h"

#define LOG_PROTECTION		0
#define LOG_PALETTE			0
#define LOG_IOCHIP			0


/******************************************************************************
	Constants
******************************************************************************/

#define MASTER_CLOCK		53693100


/******************************************************************************
	Global variables
******************************************************************************/

/* interrupt states */
static UINT8		ym3438_int;			/* INT2 - from YM3438 */
static UINT8		scanline_int;		/* INT4 - programmable */
static UINT8		vblank_int;			/* INT6 - on every VBLANK */

/* internal states */
static UINT8		iochip_reg[0x10];	/* holds values written to the I/O chip */

/* protection-related tracking */
static const UINT32 *prot_table;		/* table of protection values */
static UINT16 		prot_write_buf;		/* remembers what was written */
static UINT16		prot_read_buf;		/* remembers what was returned */

/* Ribbit! palette swizzling */
static data16_t *	ribbit_palette_select;	/* pointer to base of ROM we're interested in */
static offs_t		swizzle_table_index;/* which kind of swizzling is active? */

/* sound-related variables */
static UINT8		sound_banks;		/* number of sound banks */
static UINT8		bloxeed_sound;		/* use kludge for bloxeed sound? */

/* RAM pointers */
static data16_t *	main_ram;			/* pointer to main RAM */

/* Genesis based */
unsigned int	z80_68000_latch			= 0;
unsigned int	z80_latch_bitcount		= 0;
static int z80running;
static data16_t *genesis_68k_ram;
static unsigned char *genesis_z80_ram;
static data16_t *ic36_ram;

/* Megatech BIOS specific */
unsigned int bios_port_ctrl;
unsigned int bios_ctrl_inputs;

/* Megaplay BIOS specific */
#define MP_ROM  0x10
#define MP_GAME 0

unsigned int bios_bank; /* ROM bank selection*/
unsigned short game_banksel;  /* Game bank selection*/
unsigned int bios_game; /* Game info selection*/
unsigned int bios_mode = MP_ROM;  /* determines whether ROM banks or Game data*/
                                  /* is to read from 0x8000-0xffff*/
unsigned int bios_width;  /* determines the way the game info ROM is read*/
unsigned char bios_ctrl[6];
unsigned char bios_6600;
unsigned char bios_6204;
unsigned char bios_6402;
unsigned char bios_6403;
unsigned char bios_6404;
static unsigned char* ic3_ram;
/*static unsigned char ic36_ram[0x4000]; */
static unsigned char ic37_ram[0x8000];

unsigned int readpos = 1;  /* serial bank selection position (9-bit)*/
extern UINT16 scanbase;

/******************************************************************************
	Interrupt handling
*******************************************************************************

	The C/C2 System uses 3 Different Interrupts, IRQ2, IRQ4 and IRQ6.

	IRQ6 = Vblank, this happens after the last visible line of the display has
			been drawn (after line 224)

	IRQ4 = H-Int, this happens based upon the value in H-Int Counter.  If the
			Horizontal Interrupt is enabled and the Counter Value = 0 there
			will be a Level 4 Interrupt Triggered

	IRQ2 = sound int, generated by the YM3438

	--------

	More H-Counter Information:

	Providing Horizontal Interrupts are active the H-Counter will be loaded
	with the value stored in register #10 (0x0A) at the following times:
		(1) At the top of the display, before drawing the first line
		(2) When the counter has expired
		(3) During the VBlank Period (lines 224-261)
	The Counter is decreased by 1 after every line.

******************************************************************************/

/* call this whenever the interrupt state has changed */
static void update_interrupts(void)
{
	int level = 0;

	/* determine which interrupt is active */
	if (ym3438_int) level = 2;
	if (scanline_int) level = 4;
	if (vblank_int) level = 6;

	/* either set or clear the appropriate lines */
	if (level)
		cpu_set_irq_line(0, level, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 7, CLEAR_LINE);
}


/* timer callback to turn off the IRQ4 signal after a short while */
static void vdp_int4_off(int param)
{
	scanline_int = 0;
	update_interrupts();
}


/* timer callback to handle reloading the H counter and generate IRQ4 */
static void vdp_reload_counter(int scanline)
{
	/* generate an int if they're enabled */
	if ((segac2_vdp_regs[0] & 0x10) && !(iochip_reg[7] & 0x10))
		if (scanline != 0 || segac2_vdp_regs[10] == 0)
		{
			scanline_int = 1;
			update_interrupts();
			timer_set(cpu_getscanlinetime(scanline + 1), 0, vdp_int4_off);
		}

	/* advance to the next scanline */
	/* behavior 2: 0 count means interrupt after one scanline */
	/* (this behavior matches the Sega C2 emulator) */
	/* (in this case the vidhrdw ichidant kludge should be -2) */
	scanline += segac2_vdp_regs[10] + 1;
	if (scanline >= 224)
		scanline = 0;

	/* set a timer */
	timer_set(cpu_getscanlinetime(scanline) + cpu_getscanlineperiod() * (320. / 342.), scanline, vdp_reload_counter);
}


/* timer callback to turn off the IRQ6 signal after a short while */
static void vdp_int6_off(int param)
{
	vblank_int = 0;
	update_interrupts();
}


/* interrupt callback to generate the VBLANK interrupt */
static INTERRUPT_GEN( vblank_interrupt )
{
	/* generate the interrupt */
	vblank_int = 1;
	update_interrupts();

	/* set a timer to turn it off */
	timer_set(cpu_getscanlineperiod() * (22. / 342.), 0, vdp_int6_off);
}


/* interrupt callback to generate the YM3438 interrupt */
static void ym3438_interrupt(int state)
{
	ym3438_int = state;
	update_interrupts();
}



/******************************************************************************
	Machine init
*******************************************************************************

	This is called at init time, when it's safe to create a timer. We use
	it to prime the scanline interrupt timer.

******************************************************************************/

static MACHINE_INIT( segac2 )
{
	/* set the first scanline 0 timer to go off */
	timer_set(cpu_getscanlinetime(0) + cpu_getscanlineperiod() * (320. / 342.), 0, vdp_reload_counter);

	/* determine how many sound banks */
	sound_banks = 0;
	if (memory_region(REGION_SOUND1))
		sound_banks = memory_region_length(REGION_SOUND1) / 0x20000;

	/* reset the protection */
	prot_write_buf = 0;
	prot_read_buf = 0;
	swizzle_table_index = 0;
}

static MACHINE_INIT( genesis )
{
	/* the following ensures that the Z80 begins without running away from 0 */
	/* 0x76 is just a forced 'halt' as soon as the CPU is initially run */
	genesis_z80_ram[0] = 0x76;
	genesis_z80_ram[0x38] = 0x76;

	cpu_set_halt_line(1, ASSERT_LINE);

	z80running = 0;
	log_cb(RETRO_LOG_DEBUG, LOGPRE "Machine init\n");

	timer_set(cpu_getscanlinetime(0) + cpu_getscanlineperiod() * (320. / 342.), 0, vdp_reload_counter);

}

static MACHINE_INIT( megatech )
{
/*	unsigned char* ram = memory_region(REGION_CPU3); */

	/* mirroring of ram etc. */
	cpu_setbank(1, &genesis_z80_ram[0]);
	cpu_setbank(2, &genesis_z80_ram[0]);
	cpu_setbank(3, &genesis_68k_ram[0]);

	machine_init_genesis();
}

static MACHINE_INIT( megaplay )
{
/*	unsigned char* ram = memory_region(REGION_CPU3); */

	/* mirroring of ram etc. */
	cpu_setbank(1, &genesis_z80_ram[0]);
	cpu_setbank(2, &ic36_ram[0]);
	cpu_setbank(3, &genesis_68k_ram[0]);

	machine_init_genesis();
}

/******************************************************************************
	Sound handlers
*******************************************************************************

	These handlers are responsible for communicating with the (genenerally)
	8-bit sound chips. All accesses are via the low byte.

	The Sega C/C2 system uses a YM3438 (compatible with the YM2612) for FM-
	based music generation, and an SN76489 for PSG and noise effects. The
	C2 board also appears to have a UPD7759 for sample playback.

******************************************************************************/

/* handle reads from the YM3438 */
static READ16_HANDLER( ym3438_r )
{
	switch (offset)
	{
		case 0: return YM2612_status_port_0_A_r(0);
		case 1: return YM2612_read_port_0_r(0);
		case 2: return YM2612_status_port_0_B_r(0);
	}
	return 0xff;
}

static READ16_HANDLER( puckpkmn_YM3438_r )
{
	return	YM2612_status_port_0_A_r(0) << 8;
}


/* handle writes to the YM3438 */
static WRITE16_HANDLER( ym3438_w )
{
	/* only works if we're accessing the low byte */
	if (ACCESSING_LSB)
	{
		static UINT8 last_port;

		/* kludge for Bloxeed - it seems to accidentally trip timer 2  */
		/* and has no recourse for clearing the interrupt; until we    */
		/* find more documentation on the 2612/3438, it's unknown what */
		/* to do here */
		if (bloxeed_sound && last_port == 0x27 && (offset & 1))
			data &= ~0x08;

		switch (offset)
		{
			case 0: YM2612_control_port_0_A_w(0, data & 0xff);	last_port = data;	break;
			case 1: YM2612_data_port_0_A_w(0, data & 0xff);							break;
			case 2: YM2612_control_port_0_B_w(0, data & 0xff);	last_port = data;	break;
			case 3: YM2612_data_port_0_B_w(0, data & 0xff);							break;
		}
	}
}

static WRITE16_HANDLER( puckpkmn_YM3438_w )
{
	switch (offset)
	{
		case 0:
			if (ACCESSING_MSB)	YM2612_control_port_0_A_w	(0,	(data >> 8) & 0xff);
			else 				YM2612_data_port_0_A_w		(0,	(data >> 0) & 0xff);
			break;
		case 1:
			if (ACCESSING_MSB)	YM2612_control_port_0_B_w	(0,	(data >> 8) & 0xff);
			else 				YM2612_data_port_0_B_w		(0,	(data >> 0) & 0xff);
			break;
	}
}



/* handle writes to the UPD7759 */
static WRITE16_HANDLER( UPD7759_w )
{
	/* make sure we have a UPD chip */
	if (!sound_banks)
		return;

	/* only works if we're accessing the low byte */
	if (ACCESSING_LSB)
	{
		upd7759_reset_w(0, 0);
		upd7759_reset_w(0, 1);
		upd7759_port_w(0, data & 0xff);
		upd7759_start_w(0, 0);
		upd7759_start_w(0, 1);
	}
}


/* handle writes to the SN764896 */
static WRITE16_HANDLER( sn76489_w )
{
	/* only works if we're accessing the low byte */
	if (ACCESSING_LSB)
		SN76496_0_w(0, data & 0xff);
}



/******************************************************************************
	Palette RAM Read / Write Handlers
*******************************************************************************

	The following Read / Write Handlers are used when accessing Palette RAM.
	The C2 Hardware appears to use 4 Banks of Colours 1 of which can be Mapped
	to 0x8C0000 - 0x8C03FF at any given time by writes to 0x84000E (This same
	address also looks to be used for things like Sample Banking)

	Each Colour uses 15-bits (from a 16-bit word) in the Format
		xBGRBBBB GGGGRRRR  (x = unused, B = Blue, G = Green, R = Red)

	As this works out the Palette RAM Stores 2048 from a Possible 4096 Colours
	at any given time.

******************************************************************************/

/* handle reads from the paletteram */
static READ16_HANDLER( palette_r )
{
	return paletteram16[(offset & 0x1ff) + segac2_palbank];
}


/* handle writes to the paletteram */
static WRITE16_HANDLER( palette_w )
{
	int r,g,b,newword;

	/* adjust for the palette bank */
	offset = (offset & 0x1ff) + segac2_palbank;

	/* combine data */
	COMBINE_DATA(&paletteram16[offset]);
	newword = paletteram16[offset];

	/* up to 8 bits */
	r = ((newword << 4) & 0xf0) | ((newword >>  9) & 0x08);
	g = ((newword >> 0) & 0xf0) | ((newword >> 10) & 0x08);
	b = ((newword >> 4) & 0xf0) | ((newword >> 11) & 0x08);
	r |= r >> 5;
	g |= g >> 5;
	b |= b >> 5;

	/* set the color */
	palette_set_color(offset + 0x0000, r, g, b);
}



/******************************************************************************
	Ribbit! Palette Swizzling
*******************************************************************************

	As additional protection, Ribbit! has some hardware that munges the
	palette addresses. The exact mechanism that enables/disables this is not
	really known, but can be reliably deduced by watching for certain ROM
	accesses.

******************************************************************************/

static const UINT8 swizzle_table[][32] =
{
	{	/* case 0 */
		0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
		0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f
	},
	{	/* case 1 */
		0x04,0x05,0x0c,0x0d,0x06,0x07,0x0e,0x0f,0x14,0x15,0x1c,0x1d,0x16,0x17,0x1e,0x1f,
		0x00,0x01,0x08,0x09,0x02,0x03,0x0a,0x0b,0x10,0x11,0x18,0x19,0x12,0x13,0x1a,0x1b
	},
	{	/* case 2 */
		0x14,0x15,0x1c,0x1d,0x00,0x01,0x02,0x03,0x16,0x17,0x1e,0x1f,0x04,0x05,0x06,0x07,
		0x10,0x11,0x18,0x19,0x08,0x09,0x0a,0x0b,0x12,0x13,0x1a,0x1b,0x0c,0x0d,0x0e,0x0f
	},
	{	/* case 3 */
		0x00,0x01,0x04,0x05,0x02,0x03,0x06,0x07,0x08,0x09,0x0c,0x0d,0x0a,0x0b,0x0e,0x0f,
		0x14,0x15,0x16,0x17,0x1c,0x1d,0x1e,0x1f,0x10,0x11,0x12,0x13,0x18,0x19,0x1a,0x1b
	}
};


/* handle palette RAM reads */
static READ16_HANDLER( ribbit_palette_r )
{
	int newoffs = (offset & 0x60f) | (swizzle_table[swizzle_table_index][(offset >> 4) & 0x1f] << 4);
	return palette_r(newoffs, mem_mask);
}


/* handle palette RAM writes */
static WRITE16_HANDLER( ribbit_palette_w )
{
	int newoffs = (offset & 0x60f) | (swizzle_table[swizzle_table_index][(offset >> 4) & 0x1f] << 4);
	if (LOG_PALETTE)
		if (offset % 16 == 0) log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:palette_w @ %03X(%03X) = %04X [swizzle_table=%d]\n", activecpu_get_previouspc(), newoffs, offset, data, swizzle_table_index);
	palette_w(newoffs, data, mem_mask);
}


/* detect palette RAM swizzle accesses */
static READ16_HANDLER( ribbit_palette_select_r )
{
	/* if this is an access from the decryption code, check it out */
	if (activecpu_get_previouspc() == 0x2b4c)
	{
		switch (0x2000 + 2 * offset)
		{
			case 0x2006:	/* logs */
			case 0x2116:	/* gears, garden */
			case 0x2236:	/* ocean, bonus 1 */
			case 0x2356:	/* factory */
				swizzle_table_index = (offset >> 7) & 3;
				break;

			case 0x2476:	/* intro screen L4 */
			case 0x2576:	/* intro screen L1 */
			case 0x2676:
				swizzle_table_index = ((offset >> 7) & 3) + 1;
				break;
		}
	}
	return ribbit_palette_select[offset];
}



/******************************************************************************
	Palette I/O Read & Write Handlers
*******************************************************************************

	Controls, and Poto Poto reads 'S' 'E' 'G' and 'A' (SEGA) from this area
	as a form of protection.

	Lots of unknown writes however offset 0E certainly seems to be banking,
	both colours and sound sample banks.

******************************************************************************/

/* handle I/O chip reads */
static READ16_HANDLER( iochip_r )
{
	switch (offset)
	{
		case 0x00:	return 0xff00 | readinputport(1);
		case 0x01:	return 0xff00 | readinputport(2);
		case 0x02:	if (sound_banks)
						return 0xff00 | (upd7759_0_busy_r(0) << 6) | 0xbf; /* must return high bit on */
					else
						return 0xffff;
		case 0x04:	return 0xff00 | readinputport(0);
		case 0x05:	return 0xff00 | readinputport(3);
		case 0x06:	return 0xff00 | readinputport(4);
		case 0x08:	return 0xff00 | 'S'; /* for Poto Poto */
		case 0x09:	return 0xff00 | 'E'; /* for Poto Poto */
		case 0x0a:	return 0xff00 | 'G'; /* for Poto Poto */
		case 0x0b:	return 0xff00 | 'A'; /* for Poto Poto */
		default:	return 0xff00 | iochip_reg[offset];
	}
	return 0xffff;
}


/* handle I/O chip writes */
static WRITE16_HANDLER( iochip_w )
{
	int newbank;

	/* only LSB matters */
	if (!ACCESSING_LSB)
		return;

	/* track what was written */
	iochip_reg[offset] = data;

	/* handle special writes */
	switch (offset)
	{
		case 0x03:
			/* Bit  4   = ??? */
			break;

		case 0x07:
			/* Bits 0-1 = master palette base - maps 1 of 4 1024-entry banks into paletteram */
			/* Bits 2-3 = UPD7759 sample bank - maps 1 of 4 128k banks into the UPD7759 space */
			/* Bit  4   = IRQ4 enable/acknowledge */
			/* Bit  5   = IRQ6 enable/acknowledge (?) */
			newbank = (data & 3) * 0x200;
			if (newbank != segac2_palbank)
			{
				force_partial_update((cpu_getscanline()) + 1 + scanbase);
				segac2_palbank = newbank;
			}
			if (sound_banks > 1)
			{
				newbank = (data >> 2) & (sound_banks - 1);
				upd7759_set_bank_base(0, newbank * 0x20000);
			}
			break;

		case 0x0e:
			/* Bit  1   = YM3438 reset? no - breaks poto poto */
/*			if (!(data & 2))
				YM2612_sh_reset();*/
			break;

		case 0x0f:
			/* ???? */
			if (data != 0x88)
				if (LOG_IOCHIP) log_cb(RETRO_LOG_DEBUG, LOGPRE "%06x:I/O write @ %02x = %02x\n", activecpu_get_previouspc(), offset, data & 0xff);
			break;

		default:
			if (LOG_IOCHIP) log_cb(RETRO_LOG_DEBUG, LOGPRE "%06x:I/O write @ %02x = %02x\n", activecpu_get_previouspc(), offset, data & 0xff);
			break;
	}
}



/******************************************************************************
	Control Write Handler
*******************************************************************************

	Seems to control some global states. The most important bit is the low
	one, which enables/disables the display. This is used while tiles are
	being modified in Bloxeed.

******************************************************************************/

static WRITE16_HANDLER( control_w )
{
	/* skip if not LSB */
	if (!ACCESSING_LSB)
		return;
	data &= 0xff;

	/* bit 0 controls display enable */
	segac2_enable_display(~data & 1);

	/* log anything suspicious */
	if (LOG_IOCHIP)
		if (data != 6 && data != 7) log_cb(RETRO_LOG_DEBUG, LOGPRE "%06x:control_w suspicious value = %02X (%d)\n", activecpu_get_previouspc(), data, cpu_getscanline());
}



/******************************************************************************
	Protection Read / Write Handlers
*******************************************************************************

	The protection chip is fairly simple. Writes to it control the palette
	banking for the sprites and backgrounds. The low 4 bits are also
	remembered in a 2-stage FIFO buffer. A read from this chip should
	return a value from a 256x4-bit table. The index into this table is
	computed by taking the second-to-last value written in the upper 4 bits,
	and the previously-fetched table value in the lower 4 bits.

******************************************************************************/

/* protection chip reads */
static READ16_HANDLER( prot_r )
{
	if (LOG_PROTECTION) log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:protection r=%02X\n", activecpu_get_previouspc(), prot_table ? prot_read_buf : 0xff);
	return prot_read_buf | 0xf0;
}


/* protection chip writes */
static WRITE16_HANDLER( prot_w )
{
	int new_sp_palbase = ((data >> 2) & 3) * 0x40 + 0x100;
	int new_bg_palbase = (data & 3) * 0x40;
	int table_index;

	/* only works for the LSB */
	if (!ACCESSING_LSB)
		return;

	/* keep track of the last few writes */
	prot_write_buf = (prot_write_buf << 4) | (data & 0x0f);

	/* compute the table index */
	table_index = (prot_write_buf & 0xf0) | prot_read_buf;

	/* determine the value to return, should a read occur */
	if (prot_table)
		prot_read_buf = (prot_table[table_index >> 3] << (4 * (table_index & 7))) >> 28;
	if (LOG_PROTECTION) log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:protection w=%02X, new result=%02X\n", activecpu_get_previouspc(), data & 0x0f, prot_read_buf);

	/* if the palette changed, force an update */
	if (new_sp_palbase != segac2_sp_palbase || new_bg_palbase != segac2_bg_palbase)
	{
		force_partial_update((cpu_getscanline()) + 1 + scanbase);
		segac2_sp_palbase = new_sp_palbase;
		segac2_bg_palbase = new_bg_palbase;
		if (LOG_PALETTE) log_cb(RETRO_LOG_DEBUG, LOGPRE "Set palbank: %d/%d (scan=%d)\n", segac2_bg_palbase, segac2_sp_palbase, cpu_getscanline());
	}
}


/* special puyo puyo 2 chip reads */
static READ16_HANDLER( puyopuy2_prot_r )
{
	/* Puyo Puyo 2 uses the same protection mechanism, but the table isn't */
	/* encoded in a way that tells us the return values directly; this */
	/* function just feeds it what it wants */
	int table_index = (prot_write_buf & 0xf0) | ((prot_write_buf >> 8) & 0x0f);
	if (prot_table)
		prot_read_buf = (prot_table[table_index >> 3] << (4 * (table_index & 7))) >> 28;
	if (LOG_PROTECTION) log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:protection r=%02X\n", activecpu_get_previouspc(), prot_table ? prot_read_buf : 0xff);
	return prot_read_buf | 0xf0;
}


/* kludge for Ribbit! */
static READ16_HANDLER( ribbit_prot_hack_r )
{
	data16_t result = main_ram[0xc166/2];

	/* Ribbit is kind of evil in that they store the table shifted one state out of sequence */
	/* the following code just makes sure that the important comparison works */
	if (activecpu_get_previouspc() >= 0xff0000)
		result = (result & 0xff00) | (result >> 8);
	return result;
}



/******************************************************************************
	Counter/timer I/O
*******************************************************************************

	There appears to be a chip that is used to count coins and track time
	played, or at the very least the current status of the game. All games
	except Puyo Puyo 2 and Poto Poto access this in a mostly consistent
	manner.

******************************************************************************/

static WRITE16_HANDLER( counter_timer_w )
{
	/* only LSB matters */
	if (ACCESSING_LSB)
	{
		/*int value = data & 1;*/
		switch (data & 0x1e)
		{
			case 0x00:	/* player 1 start/stop */
			case 0x02:	/* player 2 start/stop */
			case 0x04:	/* ??? */
			case 0x06:	/* ??? */
			case 0x08:	/* player 1 game timer? */
			case 0x0a:	/* player 2 game timer? */
			case 0x0c:	/* ??? */
			case 0x0e:	/* ??? */
				break;

			case 0x10:	/* coin counter */
				coin_counter_w(0,1);
				coin_counter_w(0,0);
				break;

			case 0x12:	/* set coinage info -- followed by two 4-bit values */
				break;

			case 0x14:	/* game timer? (see Tant-R) */
			case 0x16:	/* intro timer? (see Tant-R) */
			case 0x18:	/* ??? */
			case 0x1a:	/* ??? */
			case 0x1c:	/* ??? */
				break;

			case 0x1e:	/* reset */
				break;
		}
	}
}



/******************************************************************************
	NVRAM Handling
*******************************************************************************

	There is a battery on the board that keeps the high scores. However,
	simply saving the RAM doesn't seem to be good enough. This is still
	pending investigation.

******************************************************************************/

/*
static void nvram_handler(mame_file *file, int read_or_write)
{
	int i;

	if (read_or_write)
		mame_fwrite(file, main_ram, 0x10000);
	else if (file)
		mame_fread(file, main_ram, 0x10000);
	else
		for (i = 0; i < 0x10000/2; i++)
			main_ram[i] = rand();
}
*/

/**************

Hiscores:

Bloxeed  @ f400-????			[key = ???]
Columns  @ fc00-ffff			[key = '(C) SEGA 1990.JAN BY.TAKOSUKEZOU' @ fc00,ffe0]
Columns2 @ fc00-ffff			[key = '(C) SEGA 1990.SEP.COLUMNS2 JAPAN' @ fc00,fd00,fe00,ffe0]
Borench  @ f400-f5ff			[key = 'EIJI' in last word]
TForceAC @ 8100-817f/8180-81ff	[key = '(c)Tehcno soft90' @ 8070 and 80f0]
TantR    @ fc00-fcff/fd00-fdff	[key = 0xd483 in last word]
PuyoPuyo @ fc00-fdff/fe00-ffff	[key = 0x28e1 in first word]
Ichidant @ fc00-fcff/fd00-fdff	[key = 0x85a9 in last word]
StkClmns @ fc00-fc7f/fc80-fcff	[key = ???]
PuyoPuy2
PotoPoto
ZunkYou

***************/



/******************************************************************************
	Memory Maps
*******************************************************************************

	The System C/C2 68k Memory map is fairly similar to the Genesis in terms
	of RAM, ROM, VDP access locations, although the differences between the
	arcade system and the Genesis means its not same.

	The information here has been worked out from the games, and there may
	be some uncertain things, for example Puyo Puyo 2 I believe accesses
	0x8C0400 - 0x8C0FFF which is outside of what is seeminly valid paletteram.

******************************************************************************/

static MEMORY_READ16_START( readmem )
	{ 0x000000, 0x1fffff, MRA16_ROM },					/* Main 68k Program Roms */
	{ 0x800000, 0x800001, prot_r },						/* The Protection Chip? */
	{ 0x840000, 0x84001f, iochip_r },					/* I/O Chip */
	{ 0x840100, 0x840107, ym3438_r },					/* Ym3438 Sound Chip Status Register */
	{ 0x8c0000, 0x8c0fff, palette_r },					/* Palette Ram */
	{ 0xc00000, 0xc0001f, segac2_vdp_r },				/* VDP Access */
	{ 0xff0000, 0xffffff, MRA16_RAM },					/* Main Ram */
MEMORY_END

static MEMORY_WRITE16_START( writemem )
	{ 0x000000, 0x1fffff, MWA16_ROM },					/* Main 68k Program Roms */
	{ 0x800000, 0x800001, prot_w },						/* The Protection Chip? */
	{ 0x800200, 0x800201, control_w },					/* Seems to be global controls */
	{ 0x840000, 0x84001f, iochip_w },					/* I/O Chip */
	{ 0x840100, 0x840107, ym3438_w },					/* Ym3438 Sound Chip Writes */
	{ 0x880000, 0x880001, UPD7759_w },					/* UPD7759 Sound Writes */
	{ 0x880134, 0x880135, counter_timer_w },			/* Bookkeeping */
	{ 0x880334, 0x880335, counter_timer_w },			/* Bookkeeping (mirror) */
	{ 0x8c0000, 0x8c0fff, palette_w, &paletteram16 },	/* Palette Ram */
	{ 0xc00000, 0xc0000f, segac2_vdp_w },				/* VDP Access */
	{ 0xc00010, 0xc00017, sn76489_w },					/* SN76489 Access */
	{ 0xff0000, 0xffffff, MWA16_RAM, &main_ram },		/* Main Ram */
MEMORY_END

static MEMORY_READ16_START( ooparts_readmem )
	{ 0x000000, 0x1fffff, MRA16_ROM },					/* Main 68k Program Roms */
	{ 0x800000, 0x800001, prot_r },						/* The Protection Chip? */
	{ 0x840000, 0x84001f, iochip_r },					/* I/O Chip */
	{ 0x840100, 0x840107, ym3438_r },					/* Ym3438 Sound Chip Status Register */
	{ 0x8c0000, 0x8c0fff, palette_r },					/* Palette Ram */
	{ 0xc00000, 0xc0001f, segac2_vdp_r },				/* VDP Access */
	{ 0xe00000, 0xe0ffff, MRA16_RAM },					/* Main Ram */
	{ 0xfe0000, 0xffffff, MRA16_RAM },					/* Mirror */
MEMORY_END

static MEMORY_WRITE16_START( ooparts_writemem )
	{ 0x000000, 0x1fffff, MWA16_ROM },					/* Main 68k Program Roms */
	{ 0x800000, 0x800001, prot_w },						/* The Protection Chip? */
	{ 0x800200, 0x800201, control_w },					/* Seems to be global controls */
	{ 0x840000, 0x84001f, iochip_w },					/* I/O Chip */
	{ 0x840100, 0x840107, ym3438_w },					/* Ym3438 Sound Chip Writes */
	{ 0x880000, 0x880001, UPD7759_w },					/* UPD7759 Sound Writes */
	{ 0x880134, 0x880135, counter_timer_w },			/* Bookkeeping */
	{ 0x880334, 0x880335, counter_timer_w },			/* Bookkeeping (mirror) */
	{ 0x8c0000, 0x8c0fff, palette_w, &paletteram16 },	/* Palette Ram */
	{ 0xc00000, 0xc0000f, segac2_vdp_w },				/* VDP Access */
	{ 0xc00010, 0xc00017, sn76489_w },					/* SN76489 Access */
	{ 0xe00000, 0xe0ffff, MWA16_RAM, &main_ram },		/* Main Ram */
	{ 0xfe0000, 0xffffff, MWA16_RAM, &main_ram },		/* Mirror */
MEMORY_END

static MEMORY_READ16_START( puckpkmn_readmem )
	{ 0x000000, 0x1fffff, MRA16_ROM },					/* Main 68k Program Roms */
	{ 0x700010, 0x700011, input_port_0_word_r },		/* Input (P2) */
	{ 0x700012, 0x700013, input_port_1_word_r },		/* Input (P1) */
	{ 0x700014, 0x700015, input_port_2_word_r },		/* Input (?) */
	{ 0x700016, 0x700017, input_port_3_word_r },		/* Input (DSW1) */
	{ 0x700018, 0x700019, input_port_4_word_r },		/* Input (DSW2) */
	{ 0x700022, 0x700023, OKIM6295_status_0_lsb_r },	/* M6295 Sound Chip Status Register */
	{ 0xa04000, 0xa04001, puckpkmn_YM3438_r },			/* Ym3438 Sound Chip Status Register */
	{ 0xc00000, 0xc0001f, segac2_vdp_r },				/* VDP Access */
	{ 0xe00000, 0xe1ffff, MRA16_BANK1 },				/* VDP sees the roms here */
	{ 0xfe0000, 0xfeffff, MRA16_BANK2 },				/* VDP sees the ram here */
	{ 0xff0000, 0xffffff, MRA16_RAM	},					/* Main Ram */

	/* Unknown reads: */
/*	{ 0xa10000, 0xa10001, MRA16_NOP },*/				 /*  once */ 
	{ 0xa10002, 0xa10005, MRA16_NOP },					/* ? alternative way of reading inputs ? */
	{ 0xa11100, 0xa11101, MRA16_NOP },					/* ? */
MEMORY_END

static MEMORY_WRITE16_START( puckpkmn_writemem )
	{ 0x000000, 0x1fffff, MWA16_ROM },					/* Main 68k Program Roms */
	{ 0x700022, 0x700023, OKIM6295_data_0_lsb_w },		/* M6295 Sound Chip Writes */
	{ 0xa04000, 0xa04003, puckpkmn_YM3438_w },			/* Ym3438 Sound Chip Writes */
	{ 0xc00000, 0xc0000f, segac2_vdp_w },				/* VDP Access */
	{ 0xc00010, 0xc00017, sn76489_w },					/* SN76489 Access */
	{ 0xff0000, 0xffffff, MWA16_RAM, &main_ram },		/* Main Ram */

	/* Unknown writes: */
	{ 0xa00000, 0xa00551, MWA16_RAM },					/* ? */
	{ 0xa10002, 0xa10005, MWA16_NOP },					/* ? alternative way of reading inputs ? */
/*	{ 0xa10008, 0xa1000d, MWA16_NOP },*/				 /* ? once */ 
/*	{ 0xa14000, 0xa14003, MWA16_NOP },*/					 /* ? once */
	{ 0xa11100, 0xa11101, MWA16_NOP },					/* ? */
	{ 0xa11200, 0xa11201, MWA16_NOP },					/* ? */
MEMORY_END

/* jzth prot */
int protcount = 0;

WRITE16_HANDLER( bl_710000_w )
{
	int pc = (activecpu_get_pc());

	log_cb(RETRO_LOG_DEBUG, LOGPRE "%06x writing to bl_710000_w %04x %04x\n", pc, data, mem_mask);

	/* protection value is read from  0x710000 after a series of writes.. and stored at ff0007
	   startup */
	/*
		059ce0 writing to bl_710000_w ff08 ffff
		059d04 writing to bl_710000_w 000a ffff
		059d04 writing to bl_710000_w 000b ffff
		059d04 writing to bl_710000_w 000c ffff
		059d04 writing to bl_710000_w 000f ffff
		059d1c writing to bl_710000_w ff09 ffff
		059d2a reading from bl_710000_r  (wants 0xe)
		059ce0 writing to bl_710000_w ff08 ffff
		059d04 writing to bl_710000_w 000a ffff
		059d04 writing to bl_710000_w 000b ffff
		059d04 writing to bl_710000_w 000c ffff
		059d04 writing to bl_710000_w 000f ffff
		059d1c writing to bl_710000_w ff09 ffff
		059d2a reading from bl_710000_r  (wants 0xe)
	*/
	/* before lv stage 3 */
	/*
		059ce0 writing to bl_710000_w 0008 ffff
		059d04 writing to bl_710000_w 000b ffff
		059d04 writing to bl_710000_w 000f ffff
		059d1c writing to bl_710000_w ff09 ffff
		059d2a reading from bl_710000_r  (wants 0x4)
	*/
	/* start level 3 */
	/*
		059ce0 writing to bl_710000_w ff08 ffff
		059d04 writing to bl_710000_w 000b ffff
		059d04 writing to bl_710000_w 000c ffff
		059d04 writing to bl_710000_w 000e ffff
		059d1c writing to bl_710000_w ff09 ffff
		059d2a reading from bl_710000_r  (wants 0x5)
	/* after end sequence */
	/*
		059ce0 writing to bl_710000_w 0008 ffff
		059d04 writing to bl_710000_w 000a ffff
		059d04 writing to bl_710000_w 000b ffff
		059d04 writing to bl_710000_w 000c ffff
		059d04 writing to bl_710000_w 000f ffff
		059d1c writing to bl_710000_w ff09 ffff
		059d2a reading from bl_710000_r  (wants 0xe)
	*/
		protcount++;
}


READ16_HANDLER( bl_710000_r )
{
	UINT16 ret;
	int pc = (activecpu_get_pc());
	log_cb(RETRO_LOG_DEBUG, LOGPRE "%06x reading from bl_710000_r\n", pc);

	if (protcount==6) { ret = 0xe; }
	else if (protcount==5) { ret = 0x5; }
	else if (protcount==4) { ret = 0x4; }
	else ret = 0xf;

	protcount = 0;
	return ret;
}

static MEMORY_READ16_START( jzth_readmem )
	{ 0x000000, 0x3fffff, MRA16_ROM },					/* Main 68k Program Roms */
	{ 0x700010, 0x700011, input_port_0_word_r },		/* Input (P2) */
	{ 0x700012, 0x700013, input_port_1_word_r },		/* Input (P1) */
	{ 0x700014, 0x700015, input_port_2_word_r },		/* Input (?) */
	{ 0x700016, 0x700017, input_port_3_word_r },		/* Input (DSW1) */
	{ 0x700018, 0x700019, input_port_4_word_r },		/* Input (DSW2) */
	{ 0x700022, 0x700023, OKIM6295_status_0_lsb_r },	/* M6295 Sound Chip Status Register */
	{ 0x710000, 0x710001, bl_710000_r },                /* protection, will erase the VDP address causing writes to 0 unless this returns 0xe */
	{ 0xa04000, 0xa04001, puckpkmn_YM3438_r },			/* Ym3438 Sound Chip Status Register */
	{ 0xc00000, 0xc0001f, segac2_vdp_r },				/* VDP Access */
	{ 0xe00000, 0xe1ffff, MRA16_BANK1 },				/* VDP sees the roms here */
	{ 0xfe0000, 0xfeffff, MRA16_BANK2 },				/* VDP sees the ram here */
	{ 0xff0000, 0xffffff, MRA16_RAM	},					/* Main Ram */
	{ 0xa00000, 0xa00551, MRA16_NOP },
	{ 0xA11100, 0xA11101, MRA16_NOP },
MEMORY_END

static MEMORY_WRITE16_START( jzth_writemem )
	{ 0x000000, 0x3fffff, MWA16_ROM },					/* Main 68k Program Roms */
	{ 0x700022, 0x700023, OKIM6295_data_0_lsb_w },		/* M6295 Sound Chip Writes */
	{ 0x710000, 0x710001, bl_710000_w },                /* protection, will erase the VDP address causing writes to 0 unless this returns 0xe */
	{ 0xa04000, 0xa04003, puckpkmn_YM3438_w },			/* Ym3438 Sound Chip Writes */
	{ 0xc00000, 0xc0000f, segac2_vdp_w },				/* VDP Access */
	{ 0xc00010, 0xc00017, sn76489_w },					/* SN76489 Access */
	{ 0xff0000, 0xffffff, MWA16_RAM, &main_ram },		/* Main Ram */
	{ 0xa00000, 0xa00551, MWA16_NOP },
	{ 0xA11100, 0xA11101, MWA16_NOP },
MEMORY_END

/******************** Sega Genesis ******************************/

/* from MESS */
READ16_HANDLER(genesis_ctrl_r)
{
/*	int returnval; */

/*  log_cb(RETRO_LOG_DEBUG, LOGPRE "genesis_ctrl_r %x\n", offset); */
	switch (offset)
	{
	case 0:							/* DRAM mode is write only */
		return 0xffff;
		break;
	case 0x80:						/* return Z80 CPU Function Stop Accessible or not */
		/* log_cb(RETRO_LOG_DEBUG, LOGPRE "Returning z80 state\n"); */
		return (z80running ? 0x0100 : 0x0);
		break;
	case 0x100:						/* Z80 CPU Reset - write only */
		return 0xffff;
		break;
	}
	return 0x00;

}

/* from MESS */
WRITE16_HANDLER(genesis_ctrl_w)
{
	data &= ~mem_mask;

/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "genesis_ctrl_w %x, %x\n", offset, data); */

	switch (offset)
	{
	case 0:							/* set DRAM mode... we have to ignore this for production cartridges */
		return;
		break;
	case 0x80:						/* Z80 BusReq */
		if (data == 0x100)
		{
			z80running = 0;
			cpu_set_halt_line(1, ASSERT_LINE);	/* halt Z80 */
			/* log_cb(RETRO_LOG_DEBUG, LOGPRE "z80 stopped by 68k BusReq\n"); */
		}
		else
		{
			z80running = 1;
			cpu_setbank(1, &genesis_z80_ram[0]);

			cpu_set_halt_line(1, CLEAR_LINE);
			/* log_cb(RETRO_LOG_DEBUG, LOGPRE "z80 started, BusReq ends\n"); */
		}
		return;
		break;
	case 0x100:						/* Z80 CPU Reset */
		if (data == 0x00)
		{
			cpu_set_halt_line(1, ASSERT_LINE);
			cpu_set_reset_line(1, PULSE_LINE);

			cpu_set_halt_line(1, ASSERT_LINE);
			/* log_cb(RETRO_LOG_DEBUG, LOGPRE "z80 reset, ram is %p\n", &genesis_z80_ram[0]); */
			z80running = 0;
			return;
		}
		else
		{
			/* log_cb(RETRO_LOG_DEBUG, LOGPRE "z80 out of reset\n"); */
		}
		return;

		break;
	}
}

static READ16_HANDLER ( genesis_68k_to_z80_r )
{
	offset *= 2;
	offset &= 0x7fff;

	/* Shared Ram */
	if ((offset >= 0x0000) && (offset <= 0x3fff))
	{
		offset &=0x1fff;
/*		log_cb(RETRO_LOG_DEBUG, LOGPRE "soundram_r returning %x\n",(gen_z80_shared[offset] << 8) + gen_z80_shared[offset+1]);*/
		return (genesis_z80_ram[offset] << 8) + genesis_z80_ram[offset+1];
	}

	/* YM2610 */
	if ((offset >= 0x4000) && (offset <= 0x5fff))
	{
		switch (offset & 3)
		{
		case 0:
			if (ACCESSING_MSB)	 return YM2612_status_port_0_A_r(0) << 8;
			else 				 return YM2612_read_port_0_r(0);
			break;
		case 2:
			if (ACCESSING_MSB)	return YM2612_status_port_0_B_r(0) << 8;
			else 				return 0;
			break;
		}
	}

	/* Bank Register */
	if ((offset >= 0x6000) && (offset <= 0x60ff))
	{

	}

	/* Unused / Illegal */
	if ((offset >= 0x6100) && (offset <= 0x7eff))
	{
		/* nothing */
	}

	/* VDP */
	if ((offset >= 0x7f00) && (offset <= 0x7fff))
	{

	}

	return 0x0000;
}

static WRITE16_HANDLER ( genesis_68k_to_z80_w )
{
	offset *= 2;
	offset &= 0x7fff;

	/* Shared Ram */
	if ((offset >= 0x0000) && (offset <= 0x3fff))
	{
		offset &=0x1fff;

	if (ACCESSING_LSB) genesis_z80_ram[offset+1] = data & 0xff;
	if (ACCESSING_MSB) genesis_z80_ram[offset] = (data >> 8) & 0xff;
	}

	/* YM2610 */
	if ((offset >= 0x4000) && (offset <= 0x5fff))
	{
		switch (offset & 3)
		{
		case 0:
			if (ACCESSING_MSB)	YM2612_control_port_0_A_w	(0,	(data >> 8) & 0xff);
			else 				YM2612_data_port_0_A_w		(0,	(data >> 0) & 0xff);
			break;
		case 2:
			if (ACCESSING_MSB)	YM2612_control_port_0_B_w	(0,	(data >> 8) & 0xff);
			else 				YM2612_data_port_0_B_w		(0,	(data >> 0) & 0xff);
			break;
		}
	}

	/* Bank Register */
	if ((offset >= 0x6000) && (offset <= 0x60ff))
	{

	}

	/* Unused / Illegal */
	if ((offset >= 0x6100) && (offset <= 0x7eff))
	{
		/* nothing */
	}

	/* VDP */
	if ((offset >= 0x7f00) && (offset <= 0x7fff))
	{
		offset &= 0x1f;

		if ( (offset >= 0x10) && (offset <=0x17) )
		{
			if (ACCESSING_LSB) SN76496_0_w(0, data & 0xff);
			if (ACCESSING_MSB) SN76496_0_w(0, (data >>8) & 0xff);
		}

	}
}

static READ16_HANDLER ( megaplay_68k_to_z80_r )
{
	offset *= 2;
	offset &= 0x7fff;

	/* Shared Ram */
	if ((offset >= 0x0000) && (offset <= 0x1fff))
	{
		offset &=0x1fff;
/*		log_cb(RETRO_LOG_DEBUG, LOGPRE "soundram_r returning %x\n",(gen_z80_shared[offset] << 8) + gen_z80_shared[offset+1]); */
		return (genesis_z80_ram[offset] << 8) + genesis_z80_ram[offset+1];
	}

	if ((offset >= 0x2000) && (offset <= 0x3fff))
	{
		offset &=0x1fff;
/*		if(offset == 0) */
/*			return (readinputport(8) << 8) ^ 0xff00; */
		return (ic36_ram[offset] << 8) + ic36_ram[offset+1];
	}


	/* YM2610 */
	if ((offset >= 0x4000) && (offset <= 0x5fff))
	{
		switch (offset & 3)
		{
		case 0:
			if (ACCESSING_MSB)	 return YM2612_status_port_0_A_r(0) << 8;
			else 				 return YM2612_read_port_0_r(0);
			break;
		case 2:
			if (ACCESSING_MSB)	return YM2612_status_port_0_B_r(0) << 8;
			else 				return 0;
			break;
		}
	}

	/* Bank Register */
	if ((offset >= 0x6000) && (offset <= 0x60ff))
	{

	}

	/* Unused / Illegal */
	if ((offset >= 0x6100) && (offset <= 0x7eff))
	{
		/* nothing */
	}

	/* VDP */
	if ((offset >= 0x7f00) && (offset <= 0x7fff))
	{

	}

	return 0x0000;
}

static WRITE16_HANDLER ( megaplay_68k_to_z80_w )
{
	offset *= 2;
	offset &= 0x7fff;

	/* Shared Ram */
	if ((offset >= 0x0000) && (offset <= 0x1fff))
	{
		offset &=0x1fff;

	if (ACCESSING_LSB) genesis_z80_ram[offset+1] = data & 0xff;
	if (ACCESSING_MSB) genesis_z80_ram[offset] = (data >> 8) & 0xff;
	}

	if ((offset >= 0x2000) && (offset <= 0x3fff))
	{
		offset &=0x1fff;

	if (ACCESSING_LSB) ic36_ram[offset+1] = data & 0xff;
	if (ACCESSING_MSB) ic36_ram[offset] = (data >> 8) & 0xff;
	}


	/* YM2610 */
	if ((offset >= 0x4000) && (offset <= 0x5fff))
	{
		switch (offset & 3)
		{
		case 0:
			if (ACCESSING_MSB)	YM2612_control_port_0_A_w	(0,	(data >> 8) & 0xff);
			else 				YM2612_data_port_0_A_w		(0,	(data >> 0) & 0xff);
			break;
		case 2:
			if (ACCESSING_MSB)	YM2612_control_port_0_B_w	(0,	(data >> 8) & 0xff);
			else 				YM2612_data_port_0_B_w		(0,	(data >> 0) & 0xff);
			break;
		}
	}

	/* Bank Register */
	if ((offset >= 0x6000) && (offset <= 0x60ff))
	{

	}

	/* Unused / Illegal */
	if ((offset >= 0x6100) && (offset <= 0x7eff))
	{
		/* nothing */
	}

	/* VDP */
	if ((offset >= 0x7f00) && (offset <= 0x7fff))
	{
		offset &= 0x1f;

		if ( (offset >= 0x10) && (offset <=0x17) )
		{
			if (ACCESSING_LSB) SN76496_0_w(0, data & 0xff);
			if (ACCESSING_MSB) SN76496_0_w(0, (data >>8) & 0xff);
		}

	}
}

/* Gen I/O */

/*
cgfm info

$A10001 Version
$A10003 Port A data
$A10005 Port B data
$A10007 Port C data
$A10009 Port A control
$A1000B Port B control
$A1000D Port C control
$A1000F Port A TxData
$A10011 Port A RxData
$A10013 Port A serial control
$A10015 Port B TxData
$A10017 Port B RxData
$A10019 Port B serial control
$A1001B Port C TxData
$A1001D Port C RxData
$A1001F Port C serial control

*/

data16_t *genesis_io_ram;

READ16_HANDLER ( genesis_io_r )
{
	/* 8-bit only, data is mirrored in both halves */

	UINT8 return_value = 0;

	switch (offset)
	{
		case 0:
		/* Charles MacDonald ( http://cgfm2.emuviews.com/ )
		    D7 : Console is 1= Export (USA, Europe, etc.) 0= Domestic (Japan)
		    D6 : Video type is 1= PAL, 0= NTSC
		    D5 : Sega CD unit is 1= not present, 0= connected.
		    D4 : Unused (always returns zero)
		    D3 : Bit 3 of version number
		    D2 : Bit 2 of version number
		    D1 : Bit 1 of version number
		    D0 : Bit 0 of version number
		*/
			return_value = 0x80; /* ? megatech is usa? */
			break;

		case 1: /* port A data (joypad 1) */

			if (genesis_io_ram[offset] & 0x40)
			{
				int iport = readinputport(9);
				return_value = iport & 0x3f;
			}
			else
			{
				int iport1 = readinputport(12);
				int iport2 = readinputport(7) >> 1;
				return_value = (iport1 & 0x10) + (iport2 & 0x20);
				if(iport1 & 0x10 || iport2 & 0x20)
					return_value+=1;
			}

			return_value = (genesis_io_ram[offset] & 0x80) | return_value;
/*			log_cb(RETRO_LOG_DEBUG, LOGPRE "reading joypad 1 , type %02x %02x\n",genesis_io_ram[offset] & 0x80, return_value &0x7f);*/
			if(bios_ctrl_inputs & 0x04) return_value = 0xff;
			break;

		case 2: /* port B data (joypad 2) */

			if (genesis_io_ram[offset] & 0x40)
			{
				int iport1 = (readinputport(9) & 0xc0) >> 6;
				int iport2 = (readinputport(8) & 0x0f) << 2;
				return_value = (iport1 + iport2) & 0x3f;
			}
			else
			{
				int iport1 = readinputport(12) << 2;
				int iport2 = readinputport(7) >> 2;
				return_value = (iport1 & 0x10) + (iport2 & 0x20);
				if(iport1 & 0x10 || iport2 & 0x20)
					return_value+=1;
			}
			return_value = (genesis_io_ram[offset] & 0x80) | return_value;
/*			log_cb(RETRO_LOG_DEBUG, LOGPRE "reading joypad 2 , type %02x %02x\n",genesis_io_ram[offset] & 0x80, return_value &0x7f);*/
			if(bios_ctrl_inputs & 0x04) return_value = 0xff;
			break;

		default:
			return_value = 0xe0;

	}
	return return_value | return_value << 8;
}

READ16_HANDLER ( megaplay_genesis_io_r )
{
	/* 8-bit only, data is mirrored in both halves */

	UINT8 return_value = 0;

	switch (offset)
	{
		case 0:
		/* Charles MacDonald ( http://cgfm2.emuviews.com/ )
		    D7 : Console is 1= Export (USA, Europe, etc.) 0= Domestic (Japan)
		    D6 : Video type is 1= PAL, 0= NTSC
		    D5 : Sega CD unit is 1= not present, 0= connected.
		    D4 : Unused (always returns zero)
		    D3 : Bit 3 of version number
		    D2 : Bit 2 of version number
		    D1 : Bit 1 of version number
		    D0 : Bit 0 of version number
		*/
			return_value = 0x80; /* ? megatech is usa? */
			break;

		case 1: /* port A data (joypad 1) */

			if (genesis_io_ram[offset] & 0x40)
				return_value = readinputport(1) & (genesis_io_ram[4]^0xff);
			else
			{
				return_value = readinputport(2) & (genesis_io_ram[4]^0xff);
				return_value |= readinputport(1) & 0x03;
			}
			return_value = (genesis_io_ram[offset] & 0x80) | return_value;
/*			log_cb(RETRO_LOG_DEBUG, LOGPRE "reading joypad 1 , type %02x %02x\n",genesis_io_ram[offset] & 0xb0, return_value &0x7f); */
			break;

		case 2: /* port B data (joypad 2) */

			if (genesis_io_ram[offset] & 0x40)
				return_value = readinputport(3) & (genesis_io_ram[5]^0xff);
			else
			{
				return_value = readinputport(4) & (genesis_io_ram[5]^0xff);
				return_value |= readinputport(3) & 0x03;
			}
			return_value = (genesis_io_ram[offset] & 0x80) | return_value;
/*			log_cb(RETRO_LOG_DEBUG, LOGPRE "reading joypad 2 , type %02x %02x\n",genesis_io_ram[offset] & 0xb0, return_value &0x7f); */
			break;

/*		case 3: */ /* port C data */ 
/*			return_value = bios_6402 << 3; */
/*			break; */

	default:
			return_value = genesis_io_ram[offset];

	}
	return return_value | return_value << 8;
}

WRITE16_HANDLER ( genesis_io_w )
{
/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "write io offset :%02x data %04x PC: 0x%06x\n",offset,data,activecpu_get_previouspc()); */

	switch (offset)
	{
		case 0x00:
		/*??*/
		break;

		case 0x01:/* port A data */
		genesis_io_ram[offset] = (data & (genesis_io_ram[0x04])) | (genesis_io_ram[offset] & ~(genesis_io_ram[0x04]));
		break;

		case 0x02: /* port B data */
		genesis_io_ram[offset] = (data & (genesis_io_ram[0x05])) | (genesis_io_ram[offset] & ~(genesis_io_ram[0x05]));
		break;

		case 0x03: /* port C data */
		genesis_io_ram[offset] = (data & (genesis_io_ram[0x06])) | (genesis_io_ram[offset] & ~(genesis_io_ram[0x06]));
		bios_6204 = data & 0x07;
		break;

		case 0x04: /* port A control */
		genesis_io_ram[offset] = data;
		break;

		case 0x05: /* port B control */
		genesis_io_ram[offset] = data;
		break;

		case 0x06: /* port C control */
		genesis_io_ram[offset] = data;
		break;

		case 0x07: /* port A TxData */
		genesis_io_ram[offset] = data;
		break;

		default:
		genesis_io_ram[offset] = data;
	}

}

/* Bare Knuckle III */
int genesis_region;

void genesis_init_io (void)
{

	genesis_io_ram[0x00] = (genesis_region & 0xc0)| (0x00 & 0x3f); /* region / pal / segacd etc. important! */
	genesis_io_ram[0x01] = 0x7f;
	genesis_io_ram[0x02] = 0x7f;
	genesis_io_ram[0x03] = 0x7f;
	genesis_io_ram[0x04] = 0x00;
	genesis_io_ram[0x05] = 0x00;
	genesis_io_ram[0x06] = 0x00;
	genesis_io_ram[0x07] = 0xff;
	genesis_io_ram[0x08] = 0x00;
	genesis_io_ram[0x09] = 0x00;
	genesis_io_ram[0x0a] = 0xff;
	genesis_io_ram[0x0b] = 0x00;
	genesis_io_ram[0x0c] = 0x00;
	genesis_io_ram[0x0d] = 0xfb;
	genesis_io_ram[0x0e] = 0x00;
	genesis_io_ram[0x0f] = 0x00;


  /*
  	$A10001 = $80 (Bits 7,6,5 depend on the domestic/export, PAL/NTSC jumpers and having a Sega CD or not)
    $A10003 = $7F
    $A10005 = $7F
    $A10007 = $7F
    $A10009 = $00
    $A1000B = $00
    $A1000D = $00
    $A1000F = $FF
    $A10011 = $00
    $A10013 = $00
    $A10015 = $FF
    $A10017 = $00
    $A10019 = $00
    $A1001B = $FB
    $A1001D = $00
    $A1001F = $00
*/

}

READ16_HANDLER ( genesis_68000_io_r )
{
	int paddata,p;
	int inlines, outlines;

/*printf("I/O read .. offset %02x data %02x\n",offset,genesis_io_ram[offset]); */

	switch (offset)
	{
		case 0x00: /* version register */
			return genesis_io_ram[offset];
		case 0x01:
/*			printf("I/O Data A read \n"); */

/*
                When TH=0          When TH=1
    D6 : (TH)   0                  1
    D5 : (TR)   Start button       Button C
    D4 : (TL)   Button A           Button B
    D3 : (D3)   0                  D-pad Right
    D2 : (D2)   0                  D-pad Left
    D1 : (D1)   D-pad Down         D-pad Down
    D0 : (D0)   D-pad Up           D-pad Up

*/

			/* process pad input for std 3 button pad */


			p = readinputport(0);
			if (genesis_io_ram[offset]&0x40)
			{
				paddata = ((p&0x0f)>>0) | ((p&0xc0)>>2) | 0x40;
			}
			else
			{
				paddata = ((p&0x03)>>0) | ((p&0x30)>>0) | 0x00;
			}

			inlines = (genesis_io_ram[0x04]^0xff)&0x7f;
			outlines = (genesis_io_ram[0x04] | 0x80);

/*			printf ("ioram %02x inlines %02x paddata %02x outlines %02x othdata %02x\n",genesis_io_ram[0x04], inlines, paddata, outlines, genesis_io_ram[0x01]); */



			p = (paddata & inlines) | (genesis_io_ram[0x01] & outlines);

			return p | p << 8;


			return genesis_io_ram[offset];
		case 0x02:
/*			printf("I/O Data B read \n"); */

			p = readinputport(1);
			if (genesis_io_ram[offset]&0x40)
			{
				paddata = ((p&0x0f)>>0) | ((p&0xc0)>>2) | 0x40;
			}
			else
			{
				paddata = ((p&0x03)>>0) | ((p&0x30)>>0) | 0x00;
			}

			inlines = (genesis_io_ram[0x05]^0xff)&0x7f;
			outlines = (genesis_io_ram[0x05] | 0x80);


			p = (paddata & inlines) | (genesis_io_ram[0x02] & outlines);

			return p | p <<8;

		case 0x03:
/*			printf("I/O Data C read \n"); */
			return genesis_io_ram[offset];

		case 0x04:
		case 0x05:
		case 0x06:
		case 0x07:
		case 0x08:
		case 0x09:
		case 0x0a:
		case 0x0b:
		case 0x0c:
		case 0x0d:
		case 0x0e:
		case 0x0f:
		default:
			printf("Unhandled I/O read \n");
			return genesis_io_ram[offset];

	}

	return genesis_io_ram[offset];

/*
   D7 : Console is 1= Export (USA, Europe, etc.) 0= Domestic (Japan)
    D6 : Video type is 1= PAL, 0= NTSC
    D5 : Sega CD unit is 1= not present, 0= connected.
    D4 : Unused (always returns zero)
    D3 : Bit 3 of version number
    D2 : Bit 2 of version number
    D1 : Bit 1 of version number
    D0 : Bit 0 of version number
    */


/*	return 0x30; */
/*	return mame_rand(); */
/*	return 0xff; */
}

/*

 The I/O chip is mapped to $A10000-$A1001F in the 68000/VDP/Z80 banked address space.
 It is normally read and written in byte units at odd addresses. The chip gets /LWR only,
 so writing to even addresses has no effect. Reading even addresses returns the same data
 ou'd get from an odd address. If a word-sized write is done, the LSB is sent to the I/O chip
 and the MSB is ignored.

 */

/*

$A10001  	Version
$A10003 	Port A data
$A10005 	Port B data
$A10007 	Port C data
$A10009 	Port A control
$A1000B 	Port B control
$A1000D 	Port C control
$A1000F 	Port A TxData
$A10011 	Port A RxData
$A10013 	Port A serial control
$A10015 	Port B TxData
$A10017 	Port B RxData
$A10019 	Port B serial control
$A1001B 	Port C TxData
$A1001D 	Port C RxData
$A1001F 	Port C serial contro

*/

WRITE16_HANDLER ( genesis_68000_io_w )
{

/*	printf("I/O write offset %02x data %04x\n",offset,data); */

	switch (offset)
	{
		case 0x00:  /* Version (read only?) */
			printf("attempted write to version register?!\n");
			break;
		case 0x01: /* Port A data */
/*			printf("write to data port A with control register A %02x step1 %02x step2 %02x\n", genesis_io_ram[0x04], (genesis_io_ram[0x01] & !((genesis_io_ram[0x04]&0x7f)|0x80)),  (data & ((genesis_io_ram[0x04]&0x7f)|0x80))   ); */
			genesis_io_ram[0x01] = (genesis_io_ram[0x01] & ((genesis_io_ram[0x04]^0xff)|0x80)) | (data & ((genesis_io_ram[0x04]&0x7f)|0x80));
			break;
		case 0x02: /* Port B data */
/*			printf("write to data port B with control register B %02x\n", genesis_io_ram[0x05]); */
			genesis_io_ram[0x02] = (genesis_io_ram[0x02] & ((genesis_io_ram[0x05]^0xff)|0x80)) | (data & ((genesis_io_ram[0x05]&0x7f)|0x80));
			break;
		case 0x03: /* Port C data */
/*			printf("write to data port C with control register C %02x\n", genesis_io_ram[0x06]); */
			genesis_io_ram[0x03] = (genesis_io_ram[0x03] & ((genesis_io_ram[0x06]^0xff)|0x80)) | (data & ((genesis_io_ram[0x06]&0x7f)|0x80));
			break;
		case 0x04: /* Port A control */
			genesis_io_ram[offset]=data;
			break;
		case 0x05: /* Port B control */
			genesis_io_ram[offset]=data;
			break;
		case 0x06: /* Port C control */
			genesis_io_ram[offset]=data;
			break;

		/* unhandled */
		case 0x07:
		case 0x08:
		case 0x09:
		case 0x0a:
		case 0x0b:
		case 0x0c:
		case 0x0d:
		case 0x0e:
		case 0x0f:
		default:
			genesis_io_ram[offset]=data;
			printf("unhandled IO write (offset %02x data %02x)\n",offset,data);
			break;
	}
}

static MEMORY_READ16_START( genesis_readmem )
	{ 0x000000, 0x3fffff, MRA16_ROM },					/* Cartridge Program Rom */
	{ 0xa10000, 0xa1001f, genesis_io_r },				/* Genesis Input */
	{ 0xa00000, 0xa0ffff, genesis_68k_to_z80_r },
	{ 0xc00000, 0xc0001f, segac2_vdp_r },				/* VDP Access */
	{ 0xfe0000, 0xfeffff, MRA16_BANK3 },				/* Main Ram */
	{ 0xff0000, 0xffffff, MRA16_RAM },					/* Main Ram */
MEMORY_END

static MEMORY_WRITE16_START( genesis_writemem )
	{ 0x000000, 0x3fffff, MWA16_ROM },					/* Cartridge Program Rom */
	{ 0xa10000, 0xa1001f, genesis_io_w, &genesis_io_ram },				/* Genesis Input */
	{ 0xa11000, 0xa11203, genesis_ctrl_w },
	{ 0xa00000, 0xa0ffff, megaplay_68k_to_z80_w },
	{ 0xc00000, 0xc0000f, segac2_vdp_w },				/* VDP Access */
	{ 0xc00010, 0xc00017, sn76489_w },					/* SN76489 Access */
	{ 0xfe0000, 0xfeffff, MWA16_BANK3 },				/* Main Ram */
	{ 0xff0000, 0xffffff, MWA16_RAM, &genesis_68k_ram },/* Main Ram */
MEMORY_END

static MEMORY_READ16_START( megaplay_genesis_readmem )
	{ 0x000000, 0x3fffff, MRA16_ROM },					/* Cartridge Program Rom */
	{ 0xa10000, 0xa1001f, megaplay_genesis_io_r },				/* Genesis Input */
	{ 0xa11000, 0xa11203, genesis_ctrl_r },
	{ 0xa00000, 0xa0ffff, megaplay_68k_to_z80_r },
	{ 0xc00000, 0xc0001f, segac2_vdp_r },				/* VDP Access */
	{ 0xfe0000, 0xfeffff, MRA16_BANK3 },				/* Main Ram */
	{ 0xff0000, 0xffffff, MRA16_RAM },					/* Main Ram */
MEMORY_END

static MEMORY_WRITE16_START( megaplay_genesis_writemem )
	{ 0x000000, 0x3fffff, MWA16_ROM },					/* Cartridge Program Rom */
	{ 0xa10000, 0xa1001f, genesis_io_w, &genesis_io_ram },				/* Genesis Input */
	{ 0xa11000, 0xa11203, genesis_ctrl_w },
	{ 0xa00000, 0xa0ffff, megaplay_68k_to_z80_w, &ic36_ram }, /* ok.?? */
	{ 0xc00000, 0xc0000f, segac2_vdp_w },				/* VDP Access */
	{ 0xc00010, 0xc00017, sn76489_w },					/* SN76489 Access */
	{ 0xfe0000, 0xfeffff, MWA16_BANK3 },				/* Main Ram */
	{ 0xff0000, 0xffffff, MWA16_RAM, &genesis_68k_ram },/* Main Ram */
MEMORY_END

static MEMORY_READ16_START( sbubsm_readmem )
    { 0x000000, 0x0fffff, MRA16_ROM },					/* Cartridge Program Rom */
	{ 0x202000, 0x2023ff, MRA16_RAM },
	{ 0xa00000, 0xa0ffff, genesis_68k_to_z80_r },
	{ 0xc00000, 0xc0001f, segac2_vdp_r },			/* VDP Access */
	{ 0xe00000, 0xe1ffff, MRA16_BANK3 },
	{ 0xfe0000, 0xfeffff, MRA16_BANK4 },
	{ 0xff0000, 0xffffff, MRA16_RAM },					/* Main Ram */
MEMORY_END

static MEMORY_WRITE16_START( sbubsm_writemem )
    { 0x000000, 0x0fffff, MWA16_ROM },					/* Cartridge Program Rom */
/*  { 0x200000, 0x20007f, MWA16_RAM }, */
    { 0x200000, 0x2023ff, MWA16_RAM }, /* tested */
	{ 0xa10000, 0xa1001f, genesis_io_w, &genesis_io_ram },				/* Genesis Input */
	{ 0xa11000, 0xa11203, genesis_ctrl_w },
	{ 0xa00000, 0xa0ffff, megaplay_68k_to_z80_w },
	{ 0xc00000, 0xc0000f, segac2_vdp_w },			/* VDP Access */
	{ 0xc00010, 0xc00017, sn76489_w },				/* SN76489 Access */
    { 0xfe0000, 0xfeffff, MWA16_BANK4 },
	{ 0xff0000, 0xffffff, MWA16_RAM, &genesis_68k_ram }, /* Main Ram */
MEMORY_END

static MEMORY_READ16_START( barek2ch_readmem )
    { 0x000000, 0x1fffff, MRA16_ROM },					/* Cartridge Program Rom */
	{ 0x202000, 0x2023ff, MRA16_RAM },
	{ 0xa10000, 0xa1001f, genesis_68000_io_r },				/* Genesis Input */
	{ 0xa11000, 0xa11203, genesis_ctrl_r },
	{ 0xa00000, 0xa0ffff, genesis_68k_to_z80_r },
	{ 0xc00000, 0xc0001f, segac2_vdp_r },			/* VDP Access */
	{ 0xe00000, 0xe1ffff, MRA16_BANK3 },
	{ 0xfe0000, 0xfeffff, MRA16_BANK4 },
	{ 0xff0000, 0xffffff, MRA16_RAM },					/* Main Ram */
MEMORY_END

static MEMORY_WRITE16_START( barek2ch_writemem )
    { 0x000000, 0x1fffff, MWA16_ROM },					/* Cartridge Program Rom */
/*  { 0x200000, 0x20007f, MWA16_RAM }, */
    { 0x200000, 0x2023ff, MWA16_RAM }, /* tested */
	{ 0xa10000, 0xa1001f, genesis_68000_io_w, &genesis_io_ram },				/* Genesis Input */
	{ 0xa11000, 0xa11203, genesis_ctrl_w },
	{ 0xa00000, 0xa0ffff, megaplay_68k_to_z80_w },
	{ 0xc00000, 0xc0000f, segac2_vdp_w },			/* VDP Access */
	{ 0xc00010, 0xc00017, sn76489_w },				/* SN76489 Access */
    { 0xfe0000, 0xfeffff, MWA16_BANK4 },
	{ 0xff0000, 0xffffff, MWA16_RAM, &genesis_68k_ram }, /* Main Ram */
MEMORY_END

static MEMORY_READ16_START( barek3_readmem )
	{ 0x000000, 0x3fffff, MRA16_ROM },					/* Cartridge Program Rom */
	{ 0xa10000, 0xa1001f, genesis_68000_io_r },				/* Genesis Input */
	{ 0xa11000, 0xa11203, genesis_ctrl_r },
	{ 0xa00000, 0xa0ffff, genesis_68k_to_z80_r },
	{ 0xc00000, 0xc0001f, segac2_vdp_r },				/* VDP Access */
	{ 0xfe0000, 0xfeffff, MRA16_BANK3 },				/* Main Ram */
	{ 0xff0000, 0xffffff, MRA16_RAM },					/* Main Ram */
MEMORY_END

static MEMORY_WRITE16_START( barek3_writemem )
	{ 0x000000, 0x3fffff, MWA16_ROM },					/* Cartridge Program Rom */
	{ 0xa10000, 0xa1001f, genesis_68000_io_w, &genesis_io_ram },				/* Genesis Input */
	{ 0xa11000, 0xa11203, genesis_ctrl_w },
	{ 0xa00000, 0xa0ffff, megaplay_68k_to_z80_w },
	{ 0xc00000, 0xc0000f, segac2_vdp_w },				/* VDP Access */
	{ 0xc00010, 0xc00017, sn76489_w },					/* SN76489 Access */
	{ 0xfe0000, 0xfeffff, MWA16_BANK3 },				/* Main Ram */
	{ 0xff0000, 0xffffff, MWA16_RAM, &genesis_68k_ram },/* Main Ram */
MEMORY_END

/* Z80 Sound Hardware - based on MESS code, to be improved, it can do some strange things */

#ifdef MSB_FIRST
	#define BYTE_XOR(a) (a)
#else
	#define BYTE_XOR(a) ((a) ^ 1)
#endif



static WRITE_HANDLER ( genesis_bank_select_w ) /* note value will be meaningless unless all bits are correctly set in */
{
	if (offset !=0 ) return;
/*	if (!z80running) log_cb(RETRO_LOG_DEBUG, LOGPRE "undead Z80 latch write!\n");*/
	if (z80_latch_bitcount == 0) z80_68000_latch = 0;

	z80_68000_latch = z80_68000_latch | ((( ((unsigned char)data) & 0x01) << (15+z80_latch_bitcount)));
 	log_cb(RETRO_LOG_DEBUG, LOGPRE "value %x written to latch\n", data);
	z80_latch_bitcount++;
	if (z80_latch_bitcount == 9)
	{
		z80_latch_bitcount = 0;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "latch set, value %x\n", z80_68000_latch);
	}
}

static READ_HANDLER ( genesis_z80_r )
{
	offset += 0x4000;

	/* YM2610 */
	if ((offset >= 0x4000) && (offset <= 0x5fff))
	{
		switch (offset & 3)
		{
		case 0: return YM2612_status_port_0_A_r(0);
		case 1: return YM2612_read_port_0_r(0);
		case 2: return YM2612_status_port_0_B_r(0);
		case 3: return 0;
		}
	}

	/* Bank Register */
	if ((offset >= 0x6000) && (offset <= 0x60ff))
	{

	}

	/* Unused / Illegal */
	if ((offset >= 0x6100) && (offset <= 0x7eff))
	{
		/* nothing */
	}

	/* VDP */
	if ((offset >= 0x7f00) && (offset <= 0x7fff))
	{

	}

	return 0x00;
}

static WRITE_HANDLER ( genesis_z80_w )
{
	offset += 0x4000;

	/* YM2610 */
	if ((offset >= 0x4000) && (offset <= 0x5fff))
	{
		switch (offset & 3)
		{
		case 0: YM2612_control_port_0_A_w	(0,	data);
			break;
		case 1: YM2612_data_port_0_A_w		(0, data);
			break;
		case 2: YM2612_control_port_0_B_w	(0,	data);
			break;
		case 3: YM2612_data_port_0_B_w		(0,	data);
			break;
		}
	}

	/* Bank Register */
	if ((offset >= 0x6000) && (offset <= 0x60ff))
	{
		genesis_bank_select_w(offset & 0xff, data);
	}

	/* Unused / Illegal */
	if ((offset >= 0x6100) && (offset <= 0x7eff))
	{
		/* nothing */
	}

	/* VDP */
	if ((offset >= 0x7f00) && (offset <= 0x7fff))
	{
		offset &= 0x1f;

		if ( (offset >= 0x10) && (offset <=0x17) )
		{
			SN76496_0_w(0, data & 0xff);
		}

	}
}

static READ_HANDLER ( genesis_z80_bank_r )
{
	int address = (z80_68000_latch) + (offset & 0x7fff);

	if (!z80running) log_cb(RETRO_LOG_DEBUG, LOGPRE "undead Z80->68000 read!\n");

	if (z80_latch_bitcount != 0) log_cb(RETRO_LOG_DEBUG, LOGPRE "reading whilst latch being set!\n");

	log_cb(RETRO_LOG_DEBUG, LOGPRE "z80 read from address %x\n", address);

	/* Read the data out of the 68k ROM */
	if (address < 0x400000) return memory_region(REGION_CPU1)[BYTE_XOR(address)];
	/* else read the data out of the 68k RAM */
/* 	else if (address > 0xff0000) return genesis_68k_ram[BYTE_XOR(offset)];*/

	return -1;
}

static WRITE16_HANDLER ( genesis_z80_ram_w )
{
	if (z80running) log_cb(RETRO_LOG_DEBUG, LOGPRE "Z80 written whilst running!\n");
	log_cb(RETRO_LOG_DEBUG, LOGPRE "68000->z80 sound write, %x to %x\n", data, offset);

	if (ACCESSING_LSB) genesis_z80_ram[(offset<<1)+1] = data & 0xff;
	if (ACCESSING_MSB) genesis_z80_ram[offset<<1] = (data >> 8) & 0xff;
}

static MEMORY_READ_START(genesis_z80_readmem)
 	{ 0x0000, 0x1fff, MRA_BANK1 },
 	{ 0x2000, 0x3fff, MRA_BANK2 }, /* mirror */
	{ 0x4000, 0x7fff, genesis_z80_r },
	{ 0x8000, 0xffff, genesis_z80_bank_r },
MEMORY_END

static MEMORY_WRITE_START(genesis_z80_writemem)
	{ 0x0000, 0x1fff, MWA_BANK1, &genesis_z80_ram },
 	{ 0x2000, 0x3fff, MWA_BANK2 }, /* mirror */
	{ 0x4000, 0x7fff, genesis_z80_w },
 /*	{ 0x8000, 0xffff, genesis_z80_bank_w },*/
MEMORY_END


static MEMORY_READ_START( megaplay_z80_readmem )
	{ 0x0000, 0x1fff, MRA_BANK1 },
 	{ 0x2000, 0x3fff, MRA_BANK2 },
	{ 0x4000, 0x7fff, genesis_z80_r },
	{ 0x8000, 0xffff, genesis_z80_bank_r },
MEMORY_END

static MEMORY_WRITE_START( megaplay_z80_writemem )
	{ 0x0000, 0x1fff, MWA_BANK1, &genesis_z80_ram },
 	{ 0x2000, 0x3fff, MWA_BANK2 },
	{ 0x4000, 0x7fff, genesis_z80_w },
 /*	{ 0x8000, 0xffff, genesis_z80_bank_w }, */
MEMORY_END

/* MEGATECH specific */

UINT8 mt_ram;

static READ_HANDLER( megatech_instr_r )
{
	data8_t* instr = memory_region(REGION_USER1);

	if(mt_ram == 0)
		return instr[offset/2];
	else
		return 0xff;
}

static WRITE_HANDLER( megatech_instr_w )
{
	mt_ram = data;
}


static READ_HANDLER( bios_ctrl_r )
{
	if(offset == 0)
		return 0;
	if(offset == 2)
		return bios_ctrl[offset] & 0xfe;

	return bios_ctrl[offset];
}

static WRITE_HANDLER( bios_ctrl_w )
{
	if(offset == 1)
	{
		bios_ctrl_inputs = data & 0x04;  /* Genesis/SMS input ports disable bit*/
	}
	bios_ctrl[offset] = data;
}

/*MEGAPLAY specific*/

static READ_HANDLER( megaplay_bios_banksel_r )
{
	return bios_bank;
}

static WRITE_HANDLER( megaplay_bios_banksel_w )
{
/*	Multi-slot note:
	Bits 0 and 1 appear to determine the selected game slot.
	It should be possible to multiplex different game ROMs at
	0x000000-0x3fffff based on these bits.
*/
	bios_bank = data;
	bios_mode = MP_ROM;
/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "BIOS: ROM bank %i selected [0x%02x]\n",bios_bank >> 6, data); */
}

static READ_HANDLER( megaplay_bios_gamesel_r )
{
	return bios_6403;
}

static WRITE_HANDLER( megaplay_bios_gamesel_w )
{
	bios_6403 = data;

/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "BIOS: 0x6403 write: 0x%02x\n",data); */
	bios_mode = data & 0x10;
}

static READ_HANDLER( bank_r )
{
	data8_t* bank = memory_region(REGION_CPU3);
	data8_t* game = memory_region(REGION_CPU1);

	if(game_banksel == 0x142) /* Genesis I/O*/
		return megaplay_genesis_io_r((offset/2) & 0x1f, 0xffff);

	if(bios_mode & MP_ROM)
	{
		int sel = (bios_bank >> 6) & 0x03;

/*		usrintf_showmessage("Reading from Bank %i",sel);*/

		if(sel == 0)
			return 0xff;
		else
			return bank[0x10000 + (sel-1)*0x8000 + offset];
	}
	else
	{
		if(game_banksel == 0x60 || game_banksel == 0x61)  /* read game info ROM */
			if(bios_width & 0x08)
			{
				if(offset >= 0x2000)
					return ic36_ram[offset - 0x2000];
				else
					return ic37_ram[(0x2000 * (bios_bank & 0x03)) + offset];
			}
			else
				return game[((game_banksel)*0x8000 + offset)];
		else
			return game[(game_banksel*0x8000 + (offset ^ 0x01))];
	}
}

static WRITE_HANDLER ( bank_w )
{
	if(game_banksel == 0x142) /* Genesis I/O*/
		genesis_io_w((offset/2) & 0x1f, data, 0xffff);

	if(offset <= 0x1fff && (bios_width & 0x08))
		ic37_ram[(0x2000 * (bios_bank & 0x03)) + offset] = data;

	if(offset >= 0x2000 && (bios_width & 0x08))
/*		ic36_ram[offset] = data; */
		ic36_ram[offset - 0x2000] = data;
}


static READ_HANDLER( megaplay_bios_6402_r )
{
	return genesis_io_ram[3];/* & 0xfe; */
/*	return bios_6402; & 0xfe;*/
}

static WRITE_HANDLER( megaplay_bios_6402_w )
{
	genesis_io_ram[3] = (genesis_io_ram[3] & 0x07) | ((data & 0x70) >> 1);
/*	bios_6402 = (data >> 4) & 0x07; */
/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "BIOS: 0x6402 write: 0x%02x\n",data); */
}

static READ_HANDLER( megaplay_bios_6404_r )
{
/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "BIOS: 0x6404 read: returned 0x%02x\n",bios_6404 | (bios_6403 & 0x10) >> 4); */
	return (bios_6404 & 0xfe) | ((bios_6403 & 0x10) >> 4);
/*	return bios_6404 | (bios_6403 & 0x10) >> 4; */
}

static WRITE_HANDLER( megaplay_bios_6404_w )
{
	if(((bios_6404 & 0x0c) == 0x00) && ((data & 0x0c) == 0x0c))
		cpu_set_reset_line(0, PULSE_LINE);
	bios_6404 = data;

/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "BIOS: 0x6404 write: 0x%02x\n",data); */
}

static READ_HANDLER( megaplay_bios_6204_r )
{
	return (genesis_io_ram[3]);
/*	return (bios_width & 0xf8) + (bios_6204 & 0x07); */
}

static WRITE_HANDLER( megaplay_bios_width_w )
{
	bios_width = data;
	genesis_io_ram[3] = (genesis_io_ram[3] & 0x07) | ((data & 0xf8));

/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "BIOS: 0x6204 - Width write: %02x\n",data); */
}

static READ_HANDLER( megaplay_bios_6600_r )
{
/*	Multi-slot note:
	0x6600 appears to be used to check for extra slots being used.
	Enter the following line in place of the return statement in this
	function to make the BIOS check all 4 slots (3 and 4 will be "not used")
		return (bios_6600 & 0xfe) | (bios_bank & 0x01);
*/
	return bios_6600;/* & 0xfe; */
}

static WRITE_HANDLER( megaplay_bios_6600_w )
{
	bios_6600 = data;
/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "BIOS: 0x6600 write: 0x%02x\n",data); */
}

static WRITE_HANDLER( megaplay_game_w )
{
	if(readpos == 1)
		game_banksel = 0;
	game_banksel |= (1 << (readpos-1)) * (data & 0x01);

	readpos++;
	if(readpos > 9)
	{
		bios_mode = MP_GAME;
		readpos = 1;
/*		usrintf_showmessage("Game bank selected: 0x%03x",game_banksel); */
		log_cb(RETRO_LOG_DEBUG, LOGPRE "BIOS [0x%04x]: 68K address space bank selected: 0x%03x\n",activecpu_get_previouspc(),game_banksel);
	}
}

static MEMORY_READ_START(megatech_bios_readmem)
 	{ 0x0000, 0x2fff, MRA_ROM },
	{ 0x3000, 0x3fff, MRA_RAM },
	{ 0x4000, 0x4fff, MRA_RAM },
	{ 0x5000, 0x5fff, MRA_RAM },
	{ 0x6000, 0x63ff, MRA_RAM },
	{ 0x6400, 0x6400, input_port_10_r },
	{ 0x6401, 0x6401, input_port_11_r },
	{ 0x6800, 0x6800, input_port_6_r },
	{ 0x6801, 0x6801, input_port_7_r },
	{ 0x6802, 0x6807, bios_ctrl_r },
/*	{ 0x6805, 0x6805, input_port_8_r },*/
	{ 0x6808, 0x6fff, MRA_RAM },
	{ 0x7000, 0x77ff, MRA_RAM },
	{ 0x8000, 0xffff, megatech_instr_r },
MEMORY_END

static MEMORY_WRITE_START(megatech_bios_writemem)
	{ 0x0000, 0x2fff, MWA_ROM },
	{ 0x3000, 0x3fff, MWA_RAM },
	{ 0x4000, 0x4fff, MWA_RAM },
	{ 0x5000, 0x5fff, MWA_RAM },
	{ 0x6000, 0x63ff, MWA_RAM },
	{ 0x6802, 0x6807, bios_ctrl_w },
	{ 0x6808, 0x6fff, MWA_RAM },
	{ 0x7000, 0x7fff, MWA_RAM },
MEMORY_END

static MEMORY_READ_START(megaplay_bios_readmem)
 	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x4fff, MRA_RAM },
	{ 0x5000, 0x5fff, MRA_RAM },
	{ 0x6200, 0x6200, input_port_7_r },
	{ 0x6201, 0x6201, input_port_8_r },
	{ 0x6400, 0x6400, input_port_5_r },
	{ 0x6401, 0x6401, input_port_6_r },
	{ 0x6204, 0x6204, megaplay_bios_6204_r },
	{ 0x6203, 0x6203, megaplay_bios_banksel_r },
	{ 0x6402, 0x6402, megaplay_bios_6402_r },
	{ 0x6403, 0x6403, megaplay_bios_gamesel_r },
	{ 0x6404, 0x6404, megaplay_bios_6404_r },
	{ 0x6600, 0x6600, megaplay_bios_6600_r },
	{ 0x6800, 0x77ff, MRA_RAM },
	{ 0x8000, 0xffff, bank_r },
MEMORY_END

static MEMORY_WRITE_START(megaplay_bios_writemem)
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x4fff, MWA_RAM },
	{ 0x5000, 0x5fff, MWA_RAM },
	{ 0x6000, 0x6000, megaplay_game_w },
	{ 0x6203, 0x6203, megaplay_bios_banksel_w },
	{ 0x6204, 0x6204, megaplay_bios_width_w },
	{ 0x6402, 0x6402, megaplay_bios_6402_w },
	{ 0x6403, 0x6403, megaplay_bios_gamesel_w },
	{ 0x6404, 0x6404, megaplay_bios_6404_w },
	{ 0x6600, 0x6600, megaplay_bios_6600_w },
	{ 0x6001, 0x67ff, MWA_RAM },
	{ 0x6800, 0x77ff, MWA_RAM, &ic3_ram },
	{ 0x8000, 0xffff, bank_w },
MEMORY_END



/* basically from src/drivers/segasyse.c */
unsigned char segae_vdp_ctrl_r ( UINT8 chip );
unsigned char segae_vdp_data_r ( UINT8 chip );
void segae_vdp_ctrl_w ( UINT8 chip, UINT8 data );
void segae_vdp_data_w ( UINT8 chip, UINT8 data );

static READ_HANDLER (megatech_bios_port_be_bf_r)
{
	UINT8 temp = 0;

	switch (offset)
	{
		case 0: /* port 0xbe, VDP 1 DATA Read */
			temp = segae_vdp_data_r(0); break ;
		case 1: /* port 0xbf, VDP 1 CTRL Read */
			temp = segae_vdp_ctrl_r(0); break ;
	}
	return temp;
}
static WRITE_HANDLER (megatech_bios_port_be_bf_w)
{
	switch (offset)
	{
		case 0: /* port 0xbe, VDP 1 DATA Write */
			segae_vdp_data_w(0, data); break;
		case 1: /* port 0xbf, VDP 1 CTRL Write */
			segae_vdp_ctrl_w(0, data); break;
	}
}

static WRITE_HANDLER (megatech_bios_port_ctrl_w)
{
	bios_port_ctrl = data;
}

static READ_HANDLER (megatech_bios_port_dc_r)
{
	if(bios_port_ctrl == 0x55)
		return readinputport(12);
	else
		return readinputport(9);
}

static READ_HANDLER (megatech_bios_port_dd_r)
{
	if(bios_port_ctrl == 0x55)
		return readinputport(12);
	else
		return readinputport(8);
}

static WRITE_HANDLER (megatech_bios_port_7f_w)
{
/*	usrintf_showmessage("CPU #3: I/O port 0x7F write, data %02x",data);*/
}


static PORT_READ_START( megatech_bios_readport )
	{ 0xdc, 0xdc, megatech_bios_port_dc_r },  /* player inputs*/
	{ 0xdd, 0xdd, megatech_bios_port_dd_r },  /* other player 2 inputs*/
	{ 0xbe, 0xbf, megatech_bios_port_be_bf_r },			/* VDP */
PORT_END

static PORT_WRITE_START( megatech_bios_writeport )
	{ 0x3f, 0x3f, megatech_bios_port_ctrl_w },
	{ 0x7f, 0x7f, megatech_bios_port_7f_w },
	{ 0xbe, 0xbf, megatech_bios_port_be_bf_w },			/* VDP */
PORT_END

static PORT_READ_START( megaplay_bios_readport )
/*	{ 0xdc, 0xdc, megatech_bios_port_dc_r },  */ /* player inputs*/
/*	{ 0xdd, 0xdd, megatech_bios_port_dd_r },  */ /* other player 2 inputs*/
	{ 0xbe, 0xbf, megatech_bios_port_be_bf_r },			/* VDP */
PORT_END

static PORT_WRITE_START( megaplay_bios_writeport )
/*	{ 0x3f, 0x3f, megatech_bios_port_ctrl_w },*/
	{ 0x7f, 0x7f, SN76496_1_w },	/* SN76489 */
	{ 0xbe, 0xbf, megatech_bios_port_be_bf_w },			/* VDP */
PORT_END

static UINT8 hintcount;			/* line interrupt counter, decreased each scanline */
extern UINT8 vintpending;
extern UINT8 hintpending;
extern UINT8 *segae_vdp_regs[];		/* pointer to vdp's registers */

/* Interrupt handler - from drivers/segasyse.c*/
INTERRUPT_GEN (megatech_irq)
{
	int sline;
	sline = 261 - cpu_getiloops();

	if (sline ==0) {
		hintcount = segae_vdp_regs[0][10];
	}

	if (sline <= 192) {

/*		if (sline != 192) segae_drawscanline(sline,1,1);*/

		if (sline == 192)
			vintpending = 1;

		if (hintcount == 0) {
			hintcount = segae_vdp_regs[0][10];
			hintpending = 1;

			if  ((segae_vdp_regs[0][0] & 0x10)) {
				cpu_set_irq_line(2, 0, HOLD_LINE);
				return;
			}

		} else {
			hintcount--;
		}
	}

	if (sline > 192) {
		hintcount = segae_vdp_regs[0][10];

		if ( (sline<0xe0) && (vintpending) ) {
			cpu_set_irq_line(2, 0, HOLD_LINE);
		}
	}

}


/******************************************************************************
	Input Ports
*******************************************************************************

	The input ports on the C2 games always consist of 1 Coin Port, 2 Player
	Input ports and 2 Dipswitch Ports, 1 of those Dipswitch Ports being used
	for coinage, the other for Game Options.

	Most of the Games List the Dipswitchs and Inputs in the Test Menus, adding
	them is just a tedious task.  I think Columnns & Bloxeed are Exceptions
	and will need their Dipswitches working out by observation.  The Coin Part
	of the DSW's seems fairly common to all games.

******************************************************************************/

#define COINS \
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 ) \
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 ) \
    PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE ) \
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 ) \
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 ) \
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )

#define JOYSTICK_1 \
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) \
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) \
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) \
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )

#define JOYSTICK_2 \
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 ) \
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 ) \
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 ) \
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )

#define COIN_A \
    PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) ) \
    PORT_DIPSETTING(    0x07, DEF_STR( 4C_1C ) ) \
    PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) ) \
    PORT_DIPSETTING(    0x09, DEF_STR( 2C_1C ) ) \
    PORT_DIPSETTING(    0x05, "2 Coins/1 Credit 5/3 6/4" ) \
    PORT_DIPSETTING(    0x04, "2 Coins/1 Credit, 4/3" ) \
    PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) ) \
    PORT_DIPSETTING(    0x03, "1 Coin/1 Credit, 5/6" ) \
    PORT_DIPSETTING(    0x02, "1 Coin/1 Credit, 4/5" ) \
    PORT_DIPSETTING(    0x01, "1 Coin/1 Credit, 2/3" ) \
    PORT_DIPSETTING(    0x06, DEF_STR( 2C_3C ) ) \
    PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) ) \
    PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) ) \
    PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) ) \
    PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) ) \
    PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) ) \
    PORT_DIPSETTING(    0x00, "1 Coin/1 Credit (Freeplay if Coin B also)" )

#define COIN_B \
    PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) ) \
    PORT_DIPSETTING(    0x70, DEF_STR( 4C_1C ) ) \
    PORT_DIPSETTING(    0x80, DEF_STR( 3C_1C ) ) \
    PORT_DIPSETTING(    0x90, DEF_STR( 2C_1C ) ) \
    PORT_DIPSETTING(    0x50, "2 Coins/1 Credit 5/3 6/4" ) \
    PORT_DIPSETTING(    0x40, "2 Coins/1 Credit, 4/3" ) \
    PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) ) \
    PORT_DIPSETTING(    0x30, "1 Coin/1 Credit, 5/6" ) \
    PORT_DIPSETTING(    0x20, "1 Coin/1 Credit, 4/5" ) \
    PORT_DIPSETTING(    0x10, "1 Coin/1 Credit, 2/3" ) \
    PORT_DIPSETTING(    0x60, DEF_STR( 2C_3C ) ) \
    PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) ) \
    PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) ) \
    PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) ) \
    PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) ) \
    PORT_DIPSETTING(    0xa0, DEF_STR( 1C_6C ) ) \
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit (Freeplay if Coin A also)" )


INPUT_PORTS_START( columns ) /* Columns Input Ports */
    PORT_START
    COINS
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START		/* Player 1 Controls */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )     /* Button 'Rotate'*/
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED  )     /* Button 2 Unused*/
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED  )     /* Button 3 Unused*/
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    JOYSTICK_1

	PORT_START		/* Player 2 Controls */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )    /* Button 'Rotate'*/
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED                )    /* Button 2 Unused*/
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED                )    /* Button 3 Unused*/
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    JOYSTICK_2

	PORT_START		/* Coinage */
    COIN_A
    COIN_B

	PORT_START		 /* Game Options */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )							/* Game Options..*/
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    /* The first level increase (from 0 to 1) is allways after destroying
       35 jewels. Then, the leve gets 1 level more every : */
    PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )
    PORT_DIPSETTING(    0x00, "Easy" )     /* 50 jewels*/
    PORT_DIPSETTING(    0x10, "Medium" )   /* 40 jewels*/
    PORT_DIPSETTING(    0x30, "Hard" )     /* 35 jewels*/
    PORT_DIPSETTING(    0x20, "Hardest" )  /* 25 jewels*/
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END


INPUT_PORTS_START( columns2 ) /* Columns 2 Input Ports */
    PORT_START
    COINS
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START		/* Player 1 Controls */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )     /* Button 'Rotate'*/
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED  )     /* Button 2 Unused == Button 1*/
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED  )     /* Button 3 Unused == Button 1*/
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    JOYSTICK_1

	PORT_START		/* Player 2 Controls */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )     /* Button 'Rotate'*/
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED                )     /* Button 2 Unused == Button 1*/
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED                )     /* Button 3 Unused == Button 1*/
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    JOYSTICK_2

	PORT_START		/* Coinage */
    COIN_A
    COIN_B

	PORT_START		 /* Game Options */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
    PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, "VS. Mode Credits/Match" )
	PORT_DIPSETTING(    0x0c, "1" )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x30, 0x30, "Flash Mode Difficulty" )
	PORT_DIPSETTING(    0x20, "Easy" )
    PORT_DIPSETTING(    0x30, "Medium" )
	PORT_DIPSETTING(    0x10, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
    PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
    PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
    PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END


INPUT_PORTS_START( borench ) /* Borench Input Ports */
	PORT_START		/* Coins, Start, Service etc, Same for All */
    COINS
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START		/* Player 1 Controls */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )      /* Button 'Set'*/
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )      /* Button 'Turbo'*/
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED  )      /* Button 3 Unused*/
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    JOYSTICK_1

	PORT_START		/* Player 2 Controls */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )       /* Button 'Set'*/
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )       /* Button 'Turbo'*/
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED                )       /* Button 3 Unused*/
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    JOYSTICK_2

	PORT_START		/* Coinage */
    COIN_A
    COIN_B

	PORT_START		 /* Game Options */
	PORT_DIPNAME( 0x01, 0x01, "Credits to Start" )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x00, "2" )
    PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x0c, 0x0c, "Lives 1P Mode" )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x0c, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x04, "4" )
    PORT_DIPNAME( 0x30, 0x30, "Lives 2P Mode" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x80, "Easy" )
    PORT_DIPSETTING(    0xc0, "Medium" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
INPUT_PORTS_END


INPUT_PORTS_START( tfrceac ) /* ThunderForce AC Input Ports */
	PORT_START		/* Coins, Start, Service etc, Same for All */
    COINS
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START		/* Player 1 Controls */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 )      /* Button Speed Change*/
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )      /* Button Shot*/
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 )      /* Button Weapon Select*/
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    JOYSTICK_1

	PORT_START		/* Player 2 Controls */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )      /* Button Speed Change*/
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )      /* Button Shot*/
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )      /* Button Weapon Select*/
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    JOYSTICK_2

	PORT_START		/* Coinage */
    COIN_A
    COIN_B

	PORT_START		 /* Game Options */
	PORT_DIPNAME( 0x01, 0x01, "Credits to Start" )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x00, "2" )
    PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_DIPNAME( 0x30, 0x30,  DEF_STR( Bonus_Life ) )
    PORT_DIPSETTING(    0x10, "10k, 70k, 150k" )
    PORT_DIPSETTING(    0x30, "20k, 100k, 200k" )
    PORT_DIPSETTING(    0x20, "40k, 150k, 300k" )
    PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x80, "Easy" )
    PORT_DIPSETTING(    0xc0, "Medium" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
INPUT_PORTS_END


INPUT_PORTS_START( ribbit ) /* Ribbit! Input Ports */
	PORT_START		/* Coins, Start, Service etc, Same for All */
    COINS
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START		/* Player 1 Controls */
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNKNOWN )
    JOYSTICK_1

	PORT_START		/* Player 2 Controls */
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNKNOWN )
    JOYSTICK_2

	PORT_START		/* Coinage */
    COIN_A
    COIN_B

	PORT_START		 /* Game Options */
	PORT_DIPNAME( 0x01, 0x01, "Credits to Start" )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x00, "2" )
    PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x0c, 0x04, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPSETTING(    0x0c, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )
    PORT_DIPSETTING(    0x20, "Easy" )
    PORT_DIPSETTING(    0x30, "Normal" )
    PORT_DIPSETTING(    0x10, "Hard" )
    PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END


INPUT_PORTS_START( puyopuyo ) /* PuyoPuyo Input Ports */
	PORT_START		/* Coins, Start, Service etc, Same for All */
    COINS
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START		/* Player 1 Controls */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )        /* Button 'Rotate'*/
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED  )        /* Button 2 Unused == Button 1*/
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED  )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    JOYSTICK_1

	PORT_START		/* Player 2 Controls */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )      /* Button 'Rotate'*/
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED                )      /* Button 2 Unused == Button 1*/
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED                )      /* Button 3 Unused == Button 1*/
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    JOYSTICK_2

	PORT_START		/* Coinage */
    COIN_A
    COIN_B

	PORT_START		 /* Game Options */
    PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )
    PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "VS. Mode Credits/Match" )
	PORT_DIPSETTING(    0x04, "1" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPNAME( 0x18, 0x18, "1P Mode Difficulty" )
	PORT_DIPSETTING(    0x10, "Easy" )
    PORT_DIPSETTING(    0x18, "Medium" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
    PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
    PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
    PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Moving Seat" )
	PORT_DIPSETTING(    0x80, "No Use" )
	PORT_DIPSETTING(    0x00, "In Use" )
INPUT_PORTS_END


INPUT_PORTS_START( stkclmns ) /* Stack Columns Input Ports */
	PORT_START		/* Coins, Start, Service etc, Same for All */
    COINS
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START		/* Player 1 Controls */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )      /* Button 'Rotate'*/
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )      /* Button 'Attack'*/
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED  )      /* Button 3 Unused*/
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    JOYSTICK_1

	PORT_START		/* Player 2 Controls */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )      /* Button 'Rotate'*/
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )      /* Button 'Attack'*/
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED                )      /* Button 3 Unused*/
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    JOYSTICK_2

	PORT_START		/* Coinage */
    COIN_A
    COIN_B

	PORT_START		 /* Game Options */
    PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )
    PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
    PORT_DIPSETTING(    0x03, "Medium" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
    PORT_DIPNAME( 0x04, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Match Mode Price" )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPSETTING(    0x00, "2" )
    PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
    PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
    PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
    PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
    PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END


INPUT_PORTS_START( potopoto ) /* PotoPoto Input Ports */
	PORT_START		/* Coins, Start, Service etc, Same for All */
    COINS
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START		/* Player 1 Controls */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )     /* Button 'Bomb'*/
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED  )     /* Button 2 Unused == Button 1*/
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED  )     /* Button 3 Unused == Button 1*/
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    JOYSTICK_1

	PORT_START		/* Player 2 Controls */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )    /* Button 'Bomb'*/
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED                )    /* Button 2 Unused == Button 1*/
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED                )    /* Button 3 Unused == Button 1*/
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    JOYSTICK_2

	PORT_START		/* Coinage */
    COIN_A
    COIN_B

	PORT_START		 /* Game Options */
	PORT_DIPNAME( 0x01, 0x01, "Credits to Start" )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x00, "2" )
    PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Coin Chute Type" )
	PORT_DIPSETTING(    0x04, "Common" )
	PORT_DIPSETTING(    0x00, "Individual" )
	PORT_DIPNAME( 0x08, 0x08, "Credits to Continue" )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPNAME( 0x10, 0x10, "Buy-In" )
	PORT_DIPSETTING(    0x10, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x60, 0x60, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x40, "Easy" )
    PORT_DIPSETTING(    0x60, "Medium" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x80, 0x80, "Moving Seat" )
	PORT_DIPSETTING(    0x80, "No Use" )
	PORT_DIPSETTING(    0x00, "In Use" )
INPUT_PORTS_END


INPUT_PORTS_START( zunkyou ) /* ZunkYou Input Ports */
	PORT_START		/* Coins, Start, Service etc, Same for All */
    COINS
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START		/* Player 1 Controls */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )       /* Button 'Shot'*/
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )       /* Button 'Bomb'*/
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED  )       /* Button 3 Unused*/
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    JOYSTICK_1

	PORT_START		/* Player 2 Controls */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )      /* Button 'Shot'*/
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )      /* Button 'Bomb'*/
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED                )      /* Button 3 Unused*/
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    JOYSTICK_2

	PORT_START		/* Coinage */
    COIN_A
    COIN_B

	PORT_START		 /* Game Options */
	PORT_DIPNAME( 0x01, 0x01, "Game Difficulty 1" )
    PORT_DIPSETTING(    0x01, "Medium" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x02, 0x02, "Game Difficulty 2" )
    PORT_DIPSETTING(    0x02, "Medium" )
	PORT_DIPSETTING(    0x00, "Hard" )
    PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x00, "5" )
    PORT_DIPNAME( 0x10, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
    PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
    PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
    PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END


INPUT_PORTS_START( ichidant ) /*  Ichidant-R and Tant-R Input Ports */
	PORT_START		/* Coins, Start, Service etc, Same for All */
    COINS
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START		/* Player 1 Controls */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )      /* Button 'Rotate'*/
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2  )     /* Button 2 Unused == Button 1 (Oo Parts special weapon) */
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED  )      /* Button 3 Unused == Button 1*/
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    JOYSTICK_1

	PORT_START		/* Player 2 Controls */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )    /* Button 'Rotate'*/
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )    /* Button 2 Unused == Button 1 (Oo Parts special weapon) */
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED                )    /* Button 3 Unused == Button 1*/
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    JOYSTICK_2

	PORT_START		/* Coinage */
    COIN_A
    COIN_B

	PORT_START		 /* Game Options */
    PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
    PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x06, 0x06, DEF_STR( Difficulty ) )
    PORT_DIPSETTING(    0x04, "Easy" )
    PORT_DIPSETTING(    0x06, "Medium" )
    PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
    PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
    PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
    PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
    PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
    PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
    PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END


INPUT_PORTS_START( bloxeedc ) /*  Bloxeed Input Ports */
	PORT_START		/* Coins, Start, Service etc, Same for All */
    COINS
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START		/* Player 1 Controls */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )      /* Button 'Rotate'*/
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED  )      /* Button 2 Unused == Button 1*/
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED  )      /* Button 3 Unused == Button 1*/
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    JOYSTICK_1

	PORT_START		/* Player 2 Controls */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )     /* Button 'Rotate'*/
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED                )     /* Button 2 Unused == Button 1*/
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED                )     /* Button 3 Unused == Button 1*/
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    JOYSTICK_2

	PORT_START		/* Coinage */
    COIN_A
    COIN_B

	PORT_START		 /* Game Options */
    PORT_DIPNAME( 0x01, 0x01, "VS Mode Price" )
    PORT_DIPSETTING(    0x00, "Same as Ordinary" )
    PORT_DIPSETTING(    0x01, "Double as Ordinary" )
    PORT_DIPNAME( 0x02, 0x02, "Credits to Start" )
    PORT_DIPSETTING(    0x02, "1" )
	PORT_DIPSETTING(    0x00, "2" )
    PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )
    PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( headonch ) /*  Ichidant-R and Tant-R Input Ports */
	PORT_START		/* Coins, Start, Service etc, Same for All */
    COINS
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START		/* Player 1 Controls */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )      /* 'Accelerate' */
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED  )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED  )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    JOYSTICK_1

	PORT_START		/* Player 2 Controls */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )    /* 'Accelerate' */
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED  )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED  )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    JOYSTICK_2

	PORT_START		/* Coinage */
    COIN_A
    COIN_B

	PORT_START		 /* Game Options */
    PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
    PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x06, 0x06, DEF_STR( Difficulty ) )
    PORT_DIPSETTING(    0x04, "Easy" )
    PORT_DIPSETTING(    0x06, "Medium" )
    PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
    PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
    PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
    PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x18, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x00, "5" )
    PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
    PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
    PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
    PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( ooparts ) /*  Ichidant-R and Tant-R Input Ports */
	PORT_START		/* Coins, Start, Service etc, Same for All */
    COINS
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START		/* Player 1 Controls */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )      /* 'Paddle' */
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )      /* 'Special weapon' */
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED  )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    JOYSTICK_1

	PORT_START		/* Player 2 Controls */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )    /* 'Paddle' */
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )    /* 'Special Weapon' */
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED  )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    JOYSTICK_2

	PORT_START		/* Coinage */
    COIN_A
    COIN_B

	PORT_START		 /* Game Options */
    PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
    PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x06, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x00, "5" )
    PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
    PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
    PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x10, "Easy" )
	PORT_DIPSETTING(    0x18, "Medium" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
    PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
    PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
    PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x60, "Region" )  /* undocumented */
	PORT_DIPSETTING(    0x60, "Japan" )
	PORT_DIPSETTING(    0x40, "USA" )
	PORT_DIPSETTING(    0x20, "Export" )
	PORT_DIPSETTING(    0x00, "Export" )
    PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
    PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( ssonicbr )
	PORT_START		/* Coins, Start, Service etc, Same for All */
    COINS
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START		/* Player 1 Controls */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )      /* 'Rotate' */
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED  )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED  )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    JOYSTICK_1

	PORT_START		/* Player 2 Controls */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )     /* 'Rotate' */
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED  )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED  )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    JOYSTICK_2

	PORT_START		/* Coinage */
    COIN_A
    COIN_B

	PORT_START		 /* Game Options */
    PORT_DIPNAME( 0x01, 0x01, DEF_STR( Demo_Sounds ) )
    PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
    PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x04, "Easy" )
	PORT_DIPSETTING(    0x06, "Medium" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
    PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
    PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END


INPUT_PORTS_START( puyopuy2 ) /*  Puyo Puyo 2 Input Ports */
	PORT_START		/* Coins, Start, Service etc, Same for All */
    COINS
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START		/* Player 1 Controls */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )   /* Rotate clockwise*/
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )   /* Rotate anti-clockwise. Can be inverted using the dips*/
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED  )   /* Button 3 Unused  _NOT_ Rannyu which is Start 1*/
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    JOYSTICK_1

	PORT_START		/* Player 2 Controls */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED  )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    JOYSTICK_2

	PORT_START		/* Coinage */
    COIN_A
    COIN_B

	PORT_START		 /* Game Options */
    PORT_DIPNAME( 0x01, 0x01, "Rannyu Off Button" )
    PORT_DIPSETTING(    0x01, "Use" )
    PORT_DIPSETTING(    0x00, "No Use" )
    PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
    PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x04, 0x04, "Turn Direction" )
    PORT_DIPSETTING(    0x04, "1:Right  2:Left" )
    PORT_DIPSETTING(    0x00, "1:Left  2:Right")
    PORT_DIPNAME( 0x18, 0x18, DEF_STR( Difficulty ) )
    PORT_DIPSETTING(    0x10, "Easy" )
    PORT_DIPSETTING(    0x18, "Medium" )
    PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
    PORT_DIPNAME( 0x60, 0x60, "VS Mode Match/1 Play" )
    PORT_DIPSETTING(    0x60, "1" )
    PORT_DIPSETTING(    0x40, "2" )
    PORT_DIPSETTING(    0x20, "3" )
    PORT_DIPSETTING(    0x00, "4" )
    PORT_DIPNAME( 0x80, 0x80, "Battle Start credit" )
    PORT_DIPSETTING(    0x00, "1" )
    PORT_DIPSETTING(    0x80, "2" )
INPUT_PORTS_END

INPUT_PORTS_START( puckpkmn ) /* Puckman Pockimon Input Ports */
	PORT_START	/* Player 2 Controls ($700011.b) */
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )

	PORT_START	/* Player 1 Controls ($700013.b) */
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT_IMPULSE( 0x02, IP_ACTIVE_LOW, IPT_COIN1, 10 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )

	PORT_START	/* ? ($700015.b) */

	PORT_START	/* DSW 1 ($700017.b) */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x38, 0x28, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x38, "1" )
	PORT_DIPSETTING(    0x30, "2" )
	PORT_DIPSETTING(    0x28, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x18, "5" )
	PORT_DIPSETTING(    0x10, "6" )
	PORT_DIPSETTING(    0x08, "7" )
	PORT_DIPSETTING(    0x00, "8" )
	PORT_DIPNAME( 0xc0, 0x80, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0xc0, "Easy"    )
	PORT_DIPSETTING(    0x80, "Normal"  )
	PORT_DIPSETTING(    0x40, "Hard"    )
	PORT_DIPSETTING(    0x00, "Hardest" )

	PORT_START	/* DSW 1 ($700019.b) */
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
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
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( jzth )
	PORT_START	/* Player 2 Controls ($700011.b) */
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER2 )

	PORT_START	/* Player 1 Controls ($700013.b) */
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT_IMPULSE( 0x02, IP_ACTIVE_LOW, IPT_COIN1, 10 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER1 )

	PORT_START	/* $700015.b */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW 1 ($700017.b) */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x38, 0x28, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x38, "1" )
	PORT_DIPSETTING(    0x30, "2" )
	PORT_DIPSETTING(    0x28, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x18, "5" )
	PORT_DIPSETTING(    0x10, "6" )
	PORT_DIPSETTING(    0x08, "7" )
	PORT_DIPSETTING(    0x00, "8" )
	PORT_DIPNAME( 0xc0, 0x80, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0xc0, "Easy"    )
	PORT_DIPSETTING(    0x80, "Normal"  )
	PORT_DIPSETTING(    0x40, "Hard"    )
	PORT_DIPSETTING(    0x00, "Hardest" )

	PORT_START	/* DSW 1 ($700019.b) */
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
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
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( sbubsm )
	/* the bit ordering in the ports is strange here because this is being read through shared RAM, the MCU presumably reads the real inputs then scrambles them in RAM for the 68k to sort out */
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 |  IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 |  IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT_IMPULSE( 0x02, IP_ACTIVE_LOW, IPT_COIN1, 1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	/* no service mode here? */
INPUT_PORTS_END

INPUT_PORTS_START( pclub ) /* Print Club Input Ports */
	PORT_START		 /* Coins */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
    PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* Probably Unused */

	PORT_START		 /* Controls */
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* Probably Unused */
    PORT_BIT_NAME( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1, "Ok" )
    PORT_BIT_NAME( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2, "Cancel" )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY )

	PORT_START		 /* Controls */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* Probably Unused */

	PORT_START		 /* Coinage */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 7C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 4C_1C ) )
    PORT_DIPSETTING(    0x07, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_1C ) )
    PORT_DIPSETTING(    0x06, DEF_STR( Free_Play ) )
    PORT_DIPNAME( 0x08, 0x08, "Unknown 4-4" )
    PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x10, 0x10, "Unknown 4-5" )
    PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x20, 0x20, "Unknown 4-6" )
    PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x40, 0x40, "Unknown 4-7" )
    PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x80, 0x80, "Unknown 4-8" )
    PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START		 /* Game Options */
    PORT_DIPNAME( 0x01, 0x01, "Unknown 5-1" )
    PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x02, 0x02, "Unknown 5-2" )
    PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x04, 0x04, "Unknown 5-3" )
    PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x08, 0x08, "Unknown 5-4" )
    PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x10, 0x10, "Unknown 5-5" )
    PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ))
    PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
    PORT_DIPNAME( 0x40, 0x40, "Unknown 5-7" )
    PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x80, 0x80, "Unknown 5-8" )
    PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

#define GENESIS_PORTS \
	PORT_START \
 \
	PORT_START	/* Player 1 Controls - part 1 */ \
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) \
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) \
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) \
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) \
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON2 ) \
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON3 ) \
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNUSED ) \
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNUSED ) \
 \
	PORT_START	/* Player 1 Controls - part 2 */ \
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_UNUSED ) \
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_UNUSED ) \
	PORT_BIT(  0x04, IP_ACTIVE_HIGH, IPT_UNUSED ) \
	PORT_BIT(  0x08, IP_ACTIVE_HIGH, IPT_UNUSED ) \
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) \
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_START1 ) \
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNUSED ) \
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNUSED ) \
 \
	PORT_START	/* Player 2 Controls - part 1 */ \
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 ) \
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 ) \
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 ) \
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2  ) \
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 ) \
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 ) \
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNUSED ) \
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNUSED ) \
 \
	PORT_START	/* Player 2 Controls - part 2 */ \
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_UNUSED ) \
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_UNUSED ) \
	PORT_BIT(  0x04, IP_ACTIVE_HIGH, IPT_UNUSED ) \
	PORT_BIT(  0x08, IP_ACTIVE_HIGH, IPT_UNUSED ) \
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 ) \
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_START2 ) \
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNUSED ) \
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNUSED ) \


INPUT_PORTS_START( genesis ) /* Genesis Input Ports */
GENESIS_PORTS
INPUT_PORTS_END


INPUT_PORTS_START( megatech ) /* Genesis Input Ports */
	PORT_START


	PORT_START	/* Player 1 Controls - part 2 */
/*	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )*/
/*	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )*/
/*	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )*/
/*	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )*/
/*	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON2 )*/
/*	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON3 )*/
/*	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNUSED )*/
/*	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNUSED )*/

	PORT_START	/* Player 1 Controls - part 1 */
/*	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )*/
/*	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_START1 )*/


	PORT_START	/* Player 2 Controls - part 2 */
/*	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )*/
/*	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )*/
/*	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )*/
/*	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2  )*/
/*	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )*/
/*	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )*/
/*	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNUSED )*/
/*	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNUSED )*/

	PORT_START	/* Player 2 Controls - part 1 */
/*	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )*/
/*	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_START1 | IPF_PLAYER2 )*/

	PORT_START	/* Temp - Fake dipswitch to turn on / off sms vdp display */
/*	PORT_DIPNAME( 0x01, 0x01, "SMS VDP Display (fake)" )*/
/*	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )*/
/*	PORT_DIPSETTING(    0x01, DEF_STR( On ) )*/

	PORT_START
    PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_SERVICE2, "Select", KEYCODE_0, JOYCODE_NONE )
    PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN , "0x6800 bit 1", KEYCODE_Y, JOYCODE_NONE )
    PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN, "0x6800 bit 2", KEYCODE_U, JOYCODE_NONE )
    PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN, "0x6800 bit 3", KEYCODE_I, JOYCODE_NONE )
    PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_SPECIAL, "Door 1", KEYCODE_K, JOYCODE_NONE )
    PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_SPECIAL, "Door 2", KEYCODE_L, JOYCODE_NONE )
	PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN, "0x6800 bit 6", KEYCODE_O, JOYCODE_NONE )
	PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_SERVICE, "Test mode", KEYCODE_F2, JOYCODE_NONE )

	PORT_START
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_COIN1 )  /* a few coin inputs here*/
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1, "Service coin", KEYCODE_9, JOYCODE_NONE )
	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_SERVICE3,"Enter", KEYCODE_MINUS, JOYCODE_NONE )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2)
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN , "port DD bit 4", KEYCODE_NONE, JOYCODE_NONE )
	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN , "port DD bit 5", KEYCODE_NONE, JOYCODE_NONE )
	PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN , "port DD bit 6", KEYCODE_NONE, JOYCODE_NONE )
	PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN , "port DD bit 7", KEYCODE_NONE, JOYCODE_NONE )

	PORT_START	 /* up, down, left, right, button 2,3, 2P up, down.*/
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )

    PORT_START	/* DSW A */
	PORT_DIPNAME( 0x02, 0x02, "Coin slot 3" )
	PORT_DIPSETTING (   0x00, "Inhibit" )
	PORT_DIPSETTING (   0x02, "Accept" )
	PORT_DIPNAME( 0x01, 0x01, "Coin slot 4" )
	PORT_DIPSETTING (   0x00, "Inhibit" )
	PORT_DIPSETTING (   0x01, "Accept" )
	PORT_DIPNAME( 0x1c, 0x1c, "Coin slot 3/4 value" )
	PORT_DIPSETTING(    0x1c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x14, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_8C ) )
	PORT_DIPSETTING(    0x00, "1 Coin/10 credits" )
	PORT_DIPNAME( 0xe0, 0x60, "Coin slot 2 value" )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x00, "Inhibit" )

	PORT_START /* DSW B */
	PORT_DIPNAME( 0x0f, 0x01, "Coin Slot 1 value" )
	PORT_DIPSETTING(    0x00, "Inhibit" )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_8C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 1C_9C ) )
	PORT_DIPSETTING(    0x0a, "1 coin/10 credits" )
	PORT_DIPSETTING(    0x0b, "1 coin/11 credits" )
	PORT_DIPSETTING(    0x0c, "1 coin/12 credits" )
	PORT_DIPSETTING(    0x0d, "1 coin/13 credits" )
	PORT_DIPSETTING(    0x0e, "1 coin/14 credits" )
	PORT_DIPSETTING(    0x0f, "1 coin/15 credits" )
	PORT_DIPNAME( 0xf0, 0xa0, "Time per credit" )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x10, "7:30" )
	PORT_DIPSETTING(    0x20, "7:00" )
	PORT_DIPSETTING(    0x30, "6:30" )
	PORT_DIPSETTING(    0x40, "6:00" )
	PORT_DIPSETTING(    0x50, "5:30" )
	PORT_DIPSETTING(    0x60, "5:00" )
	PORT_DIPSETTING(    0x70, "4:30" )
	PORT_DIPSETTING(    0x80, "4:00" )
	PORT_DIPSETTING(    0x90, "3:30" )
	PORT_DIPSETTING(    0xa0, "3:00" )
	PORT_DIPSETTING(    0xb0, "2:30" )
	PORT_DIPSETTING(    0xc0, "2:00" )
	PORT_DIPSETTING(    0xd0, "1:30" )
	PORT_DIPSETTING(    0xe0, "1:00" )
	PORT_DIPSETTING(    0xf0, "0:30" )

	PORT_START	 /* BIOS input ports extra*/
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1)
/*	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN , "port DD bit 4", KEYCODE_NONE, JOYCODE_NONE )*/
/*	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN , "port DD bit 5", KEYCODE_NONE, JOYCODE_NONE )*/
/*	PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN , "port DD bit 6", KEYCODE_NONE, JOYCODE_NONE )*/
/*	PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN , "port DD bit 7", KEYCODE_NONE, JOYCODE_NONE )*/

INPUT_PORTS_END

#define MEGAPLAY_TEST \
	PORT_START \
    PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_SERVICE2, "Select", KEYCODE_0, JOYCODE_NONE ) \
    PORT_BITX( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN, "0x6400 bit 1", KEYCODE_W, JOYCODE_NONE ) \
    PORT_BITX( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN, "0x6400 bit 2", KEYCODE_E, JOYCODE_NONE ) \
    PORT_BITX( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN, "0x6400 bit 3", KEYCODE_R, JOYCODE_NONE ) \
    PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN, "0x6400 bit 4", KEYCODE_T, JOYCODE_NONE ) \
    PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN, "0x6400 bit 5", KEYCODE_Y, JOYCODE_NONE ) \
	PORT_BITX( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN, "0x6400 bit 6", KEYCODE_U, JOYCODE_NONE ) \
	PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_SERVICE, "Test", KEYCODE_F2, JOYCODE_NONE )

#define MEGAPLAY_COIN \
	PORT_START \
 	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_COIN1 ) \
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_COIN2 ) \
    PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN ) \
    PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN ) \
    PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 ) \
    PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
    PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_START1 ) \
    PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_START2 )

#define MEGAPLAY_DSWA \
	PORT_START \
	PORT_DIPNAME( 0x0f, 0x0f, "Coin slot 1" ) \
    PORT_DIPSETTING( 0x07, DEF_STR( 4C_1C ) ) \
    PORT_DIPSETTING( 0x08, DEF_STR( 3C_1C ) ) \
    PORT_DIPSETTING( 0x09, DEF_STR( 2C_1C ) ) \
    PORT_DIPSETTING( 0x05, "2 coins/1 credit - 5 coins/3 credits - 6 coins/4 credits" ) \
    PORT_DIPSETTING( 0x04, "2 coins/1 credit - 4 coins/3 credits" ) \
    PORT_DIPSETTING( 0x0f, DEF_STR( 1C_1C ) ) \
    PORT_DIPSETTING( 0x06, DEF_STR( 2C_3C ) ) \
    PORT_DIPSETTING( 0x0e, DEF_STR( 1C_2C ) ) \
    PORT_DIPSETTING( 0x0d, DEF_STR( 1C_3C ) ) \
    PORT_DIPSETTING( 0x0c, DEF_STR( 1C_4C ) ) \
    PORT_DIPSETTING( 0x0b, DEF_STR( 1C_5C ) ) \
    PORT_DIPSETTING( 0x0a, DEF_STR( 1C_6C ) ) \
    PORT_DIPSETTING( 0x03, "1 coin/1 credit - 5 coins/6 credits" ) \
    PORT_DIPSETTING( 0x02, "1 coin/1 credit - 4 coins/5 credits" ) \
    PORT_DIPSETTING( 0x01, "1 coin/1 credit - 2 coins/3 credits" ) \
    PORT_DIPSETTING( 0x00, DEF_STR( Free_Play ) )

#define MEGAPLAY_DSWB \
	PORT_DIPNAME( 0xf0, 0xf0, "Coin slot 2" ) \
    PORT_DIPSETTING( 0x70, DEF_STR( 4C_1C ) ) \
    PORT_DIPSETTING( 0x80, DEF_STR( 3C_1C ) ) \
    PORT_DIPSETTING( 0x90, DEF_STR( 2C_1C ) ) \
    PORT_DIPSETTING( 0x50, "2 coins/1 credit - 5 coins/3 credits - 6 coins/4 credits" ) \
    PORT_DIPSETTING( 0x40, "2 coins/1 credit - 4 coins/3 credits" ) \
    PORT_DIPSETTING( 0xf0, DEF_STR( 1C_1C ) ) \
    PORT_DIPSETTING( 0x60, DEF_STR( 2C_3C ) ) \
    PORT_DIPSETTING( 0xe0, DEF_STR( 1C_2C ) ) \
    PORT_DIPSETTING( 0xd0, DEF_STR( 1C_3C ) ) \
    PORT_DIPSETTING( 0xc0, DEF_STR( 1C_4C ) ) \
    PORT_DIPSETTING( 0xb0, DEF_STR( 1C_5C ) ) \
    PORT_DIPSETTING( 0xa0, DEF_STR( 1C_6C ) ) \
    PORT_DIPSETTING( 0x30, "1 coin/1 credit - 5 coins/6 credits" ) \
    PORT_DIPSETTING( 0x20, "1 coin/1 credit - 4 coins/5 credits" ) \
    PORT_DIPSETTING( 0x10, "1 coin/1 credit - 2 coins/3 credits" ) \
    PORT_DIPSETTING( 0x00, " 1 coin/1 credit" )

INPUT_PORTS_START ( megaplay )
	GENESIS_PORTS
	MEGAPLAY_TEST
	MEGAPLAY_COIN
	MEGAPLAY_DSWA
	MEGAPLAY_DSWB

INPUT_PORTS_END

INPUT_PORTS_START ( mp_sonic )
	GENESIS_PORTS
	MEGAPLAY_TEST
	MEGAPLAY_COIN
	MEGAPLAY_DSWA
	MEGAPLAY_DSWB

	PORT_START
	/* DSW C  (per game settings)*/
	PORT_DIPNAME( 0x03, 0x01, "Initial Players" )
    PORT_DIPSETTING( 0x00, "4" )
    PORT_DIPSETTING( 0x01, "3" )
    PORT_DIPSETTING( 0x02, "2" )
    PORT_DIPSETTING( 0x03, "1" )

	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
    PORT_DIPSETTING( 0x00, "Hardest" )
    PORT_DIPSETTING( 0x04, "Hard" )
    PORT_DIPSETTING( 0x08, "Easy" )
    PORT_DIPSETTING( 0x0c, "Normal" )
    /* Who knows...*/
/*	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN, "0x6201 bit 4", KEYCODE_G, JOYCODE_NONE )*/
/*  PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN, "0x6201 bit 5", KEYCODE_H, JOYCODE_NONE )*/
/*	PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN, "0x6201 bit 6", KEYCODE_J, JOYCODE_NONE )*/
/*	PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN, "0x6201 bit 7", KEYCODE_K, JOYCODE_NONE )*/

INPUT_PORTS_END

INPUT_PORTS_START ( mp_gaxe2 )
	GENESIS_PORTS
	MEGAPLAY_TEST
	MEGAPLAY_COIN
	MEGAPLAY_DSWA
	MEGAPLAY_DSWB

	PORT_START
	/* DSW C  (per game settings)*/
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Difficulty ) )
    PORT_DIPSETTING( 0x01, "Normal" )
    PORT_DIPSETTING( 0x00, "Hard" )

	PORT_DIPNAME( 0x02, 0x00, "Life" )
    PORT_DIPSETTING( 0x02, "1" )
    PORT_DIPSETTING( 0x00, "2" )

	PORT_DIPNAME( 0x04, 0x04, "Initial Players" )
    PORT_DIPSETTING( 0x00, "1" )
    PORT_DIPSETTING( 0x04, "2" )

	PORT_DIPNAME( 0x08, 0x00, "Timer" )
    PORT_DIPSETTING( 0x08, DEF_STR( Off )  )
    PORT_DIPSETTING( 0x00, DEF_STR( On ) )

    /* Who knows...*/
/*	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN, "0x6201 bit 4", KEYCODE_G, JOYCODE_NONE )*/
/*  PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN, "0x6201 bit 5", KEYCODE_H, JOYCODE_NONE )*/
/*	PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN, "0x6201 bit 6", KEYCODE_J, JOYCODE_NONE )*/
/*	PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN, "0x6201 bit 7", KEYCODE_K, JOYCODE_NONE )*/

INPUT_PORTS_END

INPUT_PORTS_START ( mp_twc )
	GENESIS_PORTS
	MEGAPLAY_TEST
	MEGAPLAY_COIN
	MEGAPLAY_DSWA
	MEGAPLAY_DSWB

	PORT_START
	/* DSW C  (per game settings) */
	PORT_DIPNAME( 0x01, 0x01, "Time" )
    PORT_DIPSETTING( 0x01, "Normal" )
    PORT_DIPSETTING( 0x00, "Short" )

	PORT_DIPNAME( 0x0e, 0x08, "Level" )
    PORT_DIPSETTING( 0x00, "0" )
    PORT_DIPSETTING( 0x02, "0" )
    PORT_DIPSETTING( 0x04, "5" )
    PORT_DIPSETTING( 0x06, "4" )
    PORT_DIPSETTING( 0x08, "3" )
    PORT_DIPSETTING( 0x0a, "2" )
    PORT_DIPSETTING( 0x0c, "1" )
    PORT_DIPSETTING( 0x0e, "0" )

INPUT_PORTS_END

INPUT_PORTS_START ( mp_soni2 )
	GENESIS_PORTS
	MEGAPLAY_TEST
	MEGAPLAY_COIN
	MEGAPLAY_DSWA
	MEGAPLAY_DSWB

	PORT_START
	/* DSW C  (per game settings) */
	PORT_DIPNAME( 0x03, 0x01, "Initial Players (Normal mode)" )
    PORT_DIPSETTING( 0x00, "4" )
    PORT_DIPSETTING( 0x01, "3" )
    PORT_DIPSETTING( 0x02, "2" )
    PORT_DIPSETTING( 0x03, "1" )

	PORT_DIPNAME( 0x0c, 0x0c, "Initial Players (Dual mode)" )
    PORT_DIPSETTING( 0x00, "4" )
    PORT_DIPSETTING( 0x04, "2" )
    PORT_DIPSETTING( 0x08, "1" )
    PORT_DIPSETTING( 0x0c, "3" )

INPUT_PORTS_END

INPUT_PORTS_START ( mp_sor2 )
	GENESIS_PORTS
	MEGAPLAY_TEST
	MEGAPLAY_COIN
	MEGAPLAY_DSWA
	MEGAPLAY_DSWB

	PORT_START
	/* DSW C  (per game settings) */
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Lives ) )
    PORT_DIPSETTING( 0x00, "4" )
    PORT_DIPSETTING( 0x01, "3" )
    PORT_DIPSETTING( 0x02, "2" )
    PORT_DIPSETTING( 0x03, "1" )

	PORT_DIPNAME( 0xc, 0x0c, DEF_STR ( Difficulty ) )
    PORT_DIPSETTING( 0x00, "Hardest" )
    PORT_DIPSETTING( 0x04, "Hard" )
    PORT_DIPSETTING( 0x08, "Easy" )
    PORT_DIPSETTING( 0x0c, "Normal" )

INPUT_PORTS_END

INPUT_PORTS_START ( mp_bio )
	GENESIS_PORTS
	MEGAPLAY_TEST
	MEGAPLAY_COIN
	MEGAPLAY_DSWA
	MEGAPLAY_DSWB

	PORT_START
	/* DSW C  (per game settings) */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
    PORT_DIPSETTING( 0x00, "5" )
    PORT_DIPSETTING( 0x01, "4" )
    PORT_DIPSETTING( 0x02, "2" )
    PORT_DIPSETTING( 0x03, "3" )

	PORT_DIPNAME( 0xc, 0x0c, DEF_STR ( Difficulty ) )
    PORT_DIPSETTING( 0x00, "Hardest" )
    PORT_DIPSETTING( 0x04, "Hard" )
    PORT_DIPSETTING( 0x08, "Easy" )
    PORT_DIPSETTING( 0x0c, "Normal" )

INPUT_PORTS_END

INPUT_PORTS_START ( mp_shnb3 )
	GENESIS_PORTS
	MEGAPLAY_TEST
	MEGAPLAY_COIN
	MEGAPLAY_DSWA
	MEGAPLAY_DSWB

	PORT_START
	/* DSW C  (per game settings) */
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Lives ) )
    PORT_DIPSETTING( 0x00, "4" )
    PORT_DIPSETTING( 0x01, "3" )
    PORT_DIPSETTING( 0x02, "2" )
    PORT_DIPSETTING( 0x03, "1" )

	PORT_DIPNAME( 0xc, 0x0c, DEF_STR ( Difficulty ) )
    PORT_DIPSETTING( 0x00, "Expert" )
    PORT_DIPSETTING( 0x04, "Hard" )
    PORT_DIPSETTING( 0x08, "Easy" )
    PORT_DIPSETTING( 0x0c, "Normal" )
INPUT_PORTS_END

INPUT_PORTS_START( aladbl )
	PORT_START /* Joypad 1 (3 button + start) NOT READ DIRECTLY */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT_NAME( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1, "P1 Throw" )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT_NAME( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1, "P1 Sword" )
	PORT_BIT_NAME( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1, "P1 Jump" )


	PORT_START /* Joypad 2 (3 button + start) Not used */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START /* 3rd I/O port */

	/* As I don't know how it is on real hardware, this is more a guess than anything */
	PORT_START /* MCU hooked up via readinputport (3) */
	PORT_DIPNAME( 0x07, 0x01, DEF_STR( Coinage ) )          /* code at 0x1b2a50 - unsure if there are so many settings */
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_7C ) )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_SPECIAL )         /* to avoid it being changed and corrupting Coinage settings */
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Difficulty ) )       /* code at 0x1b2680 */
	PORT_DIPSETTING(    0x10, "Easy" )             /* "PRACTICE" */
	PORT_DIPSETTING(    0x00, "Normal" )           /* "NORMAL" */
	PORT_DIPSETTING(    0x20, "Hard" )             /* "DIFFICULT" */
	PORT_BIT_IMPULSE(   0x100, IP_ACTIVE_HIGH, IPT_COIN1, 1 ) /* needed to avoid credits getting mad */
INPUT_PORTS_END

INPUT_PORTS_START( barek2ch )
	PORT_START	/* IN0 player 1 controller */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )

	PORT_START	/* IN1 player 2 controller */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )

	PORT_START
	PORT_BIT(  0x3f, IP_ACTIVE_LOW, IPT_UNUSED ) /* apparently no use for these */
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x08, 0x08, "SW1:4" )
    PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x10, 0x10, "SW1:5" )
    PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x20, 0x20, "SW1:6" )
    PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x40, 0x40, "SW1:7" )
    PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x80, 0x80, "SW1:8" )
    PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "SW2:1" ) /* at least some of the first 3 seem to control difficulty (enemies attack later / less frequently by switching these) */
    PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x02, 0x02, "SW2:2" )
    PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x04, 0x04, "SW2:3" )
    PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x18, "1" )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0xe0, 0xe0, "Starting Level" )
	PORT_DIPSETTING(    0xe0, "1" )
	PORT_DIPSETTING(    0xc0, "2" )
	PORT_DIPSETTING(    0xa0, "3" )
	PORT_DIPSETTING(    0x80, "4" )
	PORT_DIPSETTING(    0x60, "5" )
	PORT_DIPSETTING(    0x40, "6" )
	PORT_DIPSETTING(    0x20, "7" )
	PORT_DIPSETTING(    0x00, "8" )

	PORT_START /* present on PCB but there doesn't seem to be any read for them */
	PORT_DIPNAME( 0x01, 0x01, "SW3:1" )
    PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x02, 0x02, "SW3:2" )
    PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x04, 0x04, "SW3:3" )
    PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x08, 0x08, "SW3:4" )
    PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x10, 0x10, "SW3:5" )
    PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x20, 0x20, "SW3:6")
    PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x40, 0x40, "SW3:7" )
    PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x80, 0x80, "SW3:8" )
    PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( barek3 )
	PORT_START	/* IN0 player 1 controller */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT_NAME( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1, "P1 Button A" )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT_NAME( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1, "P1 Button B" )
	PORT_BIT_NAME( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1, "P1 Button C" )

	PORT_START	/* IN1 player 2 controller */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT_NAME( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2, "P2 Button A" )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT_NAME( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2, "P2 Button B" )
	PORT_BIT_NAME( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2, "P2 Button C" )

	PORT_START
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x18, "1" )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x60, "Very_Easy" )
	PORT_DIPSETTING(    0x40, "Easy" )
	PORT_DIPSETTING(    0xe0, "Normal" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x00, "Very_Hard" )
INPUT_PORTS_END

INPUT_PORTS_START( sonic2mb )
    PORT_START  /* IN0 player 1 controller */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START /* Joypad 2 (3 button + start) Not used */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

    PORT_START /* 3rd I/O port */

	PORT_START /* DSW via readinputport 3 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x00fc, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME(          0x0300, 0x0200, DEF_STR( Lives ) )
	PORT_DIPSETTING(       0x0000, "1" )
	PORT_DIPSETTING(       0x0100, "2" )
	PORT_DIPSETTING(       0x0200, "3" )
	PORT_DIPSETTING(       0x0300, "4" )
	PORT_DIPNAME(  0x3c00, 0x2000, "Timer Speed" ) 
	PORT_DIPSETTING(       0x3c00, "0 (Slowest)" )
	PORT_DIPSETTING(       0x3800, "1" )
	PORT_DIPSETTING(       0x3400, "2" )
	PORT_DIPSETTING(       0x3000, "3" )
	PORT_DIPSETTING(       0x2c00, "4" )
	PORT_DIPSETTING(       0x2800, "5" )
	PORT_DIPSETTING(       0x2400, "6" )
	PORT_DIPSETTING(       0x2000, "7" )
	PORT_DIPSETTING(       0x1c00, "8" )
	PORT_DIPSETTING(       0x1800, "9" )
	PORT_DIPSETTING(       0x1400, "10" )
	PORT_DIPSETTING(       0x1000, "11" )
	PORT_DIPSETTING(       0x0c00, "12" )
	PORT_DIPSETTING(       0x0800, "13" )
	PORT_DIPSETTING(       0x0400, "14" )
	PORT_DIPSETTING(       0x0000, "15 (Fastest)" )
    PORT_DIPNAME( 0x4000,  0x4000, "SW1:7" )
    PORT_DIPSETTING(       0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(       0x0000, DEF_STR( On ) )
    PORT_DIPNAME( 0x8000,  0x8000, "SW1:8" )
    PORT_DIPSETTING(       0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(       0x0000, DEF_STR( On ) )
INPUT_PORTS_END

/******************************************************************************
	Sound interfaces
******************************************************************************/

static struct upd7759_interface upd7759_intf =
{
	1,								/* One chip */
	{ UPD7759_STANDARD_CLOCK },
	{ 50 },							/* Volume */
	{ REGION_SOUND1 },				/* Memory pointer (gen.h) */
};

static struct YM2612interface ym3438_intf =
{
	1,								/* One chip */
	MASTER_CLOCK/7,					/* Clock: 7.67 MHz */
	{ YM3012_VOL(50,MIXER_PAN_CENTER,50,MIXER_PAN_CENTER) },	/* Volume */
	{ 0 },							/* port I/O */
	{ 0 },							/* port I/O */
	{ 0 },							/* port I/O */
	{ 0 },							/* port I/O */
	{ ym3438_interrupt }			/* IRQ handler */
};

static struct YM2612interface gen_ym3438_intf =
{
	1,								/* One chip */
	MASTER_CLOCK/7,					/* Clock: 7.67 MHz */
	{ YM3012_VOL(50,MIXER_PAN_CENTER,50,MIXER_PAN_CENTER) },	/* Volume */
	{ 0 },							/* port I/O */
	{ 0 },							/* port I/O */
	{ 0 },							/* port I/O */
	{ 0 },							/* port I/O */
/*	{ ym3438_interrupt }*/			 /* IRQ handler */ 
};

static struct SN76496interface sn76489_intf =
{
	1,								/* One chip */
	{ MASTER_CLOCK/15 },			/* Clock: 3.58 MHz */
	{ 50 }							/* Volume */
};

static struct SN76496interface megatech_sn76489_intf =
{
	2,		/* Two chips, one in the Genesis VDP and one in the SMS VDP */
	{ MASTER_CLOCK/15, MASTER_CLOCK/15 },			/* Clock: 3.58 MHz */
	{ 50, 50 }							/* Volume */
};

static struct OKIM6295interface puckpkmn_m6295_intf =
{
	1,
	{ 8000 },	/* ? */
	{ REGION_SOUND1 },
	{ 50 }
};


/******************************************************************************
	Machine Drivers
*******************************************************************************

	General Overview
		M68000 @ 10MHz (Main Processor)
		YM3438 (Fm Sound)
		SN76489 (PSG, Noise, Part of the VDP)
		UPD7759 (Sample Playback, C-2 Only)

******************************************************************************/

static MACHINE_DRIVER_START( segac )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main",M68000, MASTER_CLOCK/7) 		/* yes, there is a divide-by-7 circuit */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(vblank_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION((int)(((262. - 224.) / 262.) * 1000000. / 60.))

	MDRV_MACHINE_INIT(segac2)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_HAS_SHADOWS | VIDEO_HAS_HIGHLIGHTS)
	MDRV_SCREEN_SIZE(320,224)
	MDRV_VISIBLE_AREA(0, 319, 0, 223)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(segac2)
	MDRV_VIDEO_EOF(segac2)
	MDRV_VIDEO_UPDATE(segac2)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2612, ym3438_intf)
	MDRV_SOUND_ADD(SN76496, sn76489_intf)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( segac2 )

	/* basic machine hardware */
	MDRV_IMPORT_FROM( segac )

	/* sound hardware */
	MDRV_SOUND_ADD(UPD7759, upd7759_intf)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( ooparts )

	/* basic machine hardware */
	MDRV_IMPORT_FROM( segac2 )
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(ooparts_readmem,ooparts_writemem)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( puckpkmn )

	/* basic machine hardware */
	MDRV_IMPORT_FROM( segac )
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(puckpkmn_readmem,puckpkmn_writemem)

	/* video hardware */
	MDRV_VIDEO_START(puckpkmn)
	MDRV_VISIBLE_AREA(8, 319, 0, 223)

	/* sound hardware */
	MDRV_SOUND_ADD(OKIM6295, puckpkmn_m6295_intf)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( jzth )

	/* basic machine hardware */
	MDRV_IMPORT_FROM( segac )
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(jzth_readmem,jzth_writemem)

	/* video hardware */
	MDRV_VIDEO_START(puckpkmn)
	MDRV_VISIBLE_AREA(8, 319, 0, 223)

	/* sound hardware */
	MDRV_SOUND_ADD(OKIM6295, puckpkmn_m6295_intf)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( genesis_base )
	/*basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M68000, 53693100 / 7)
	MDRV_CPU_MEMORY(genesis_readmem, genesis_writemem)
	MDRV_CPU_VBLANK_INT(vblank_interrupt,1)

	MDRV_CPU_ADD_TAG("sound", Z80, 53693100 / 15)
	MDRV_CPU_MEMORY(genesis_z80_readmem, genesis_z80_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold, 1) /* from vdp at scanline 0xe0 */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION((int)(((262. - 224.) / 262.) * 1000000. / 60.))

	MDRV_INTERLEAVE(100)

	MDRV_MACHINE_INIT(genesis)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_HAS_SHADOWS | VIDEO_HAS_HIGHLIGHTS)
	MDRV_SCREEN_SIZE(320,224)
	MDRV_VISIBLE_AREA(0, 319, 0, 223)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(puckpkmn)
	MDRV_VIDEO_EOF(segac2)
	MDRV_VIDEO_UPDATE(segac2)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2612, gen_ym3438_intf )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( genesis )

	MDRV_IMPORT_FROM( genesis_base )
	MDRV_SOUND_ADD(SN76496, sn76489_intf)

MACHINE_DRIVER_END

static MACHINE_DRIVER_START( sbubsm )

	MDRV_IMPORT_FROM( genesis )
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(sbubsm_readmem,sbubsm_writemem)

	MDRV_VISIBLE_AREA(0, 319, 0, 223)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( barek2ch )

	MDRV_IMPORT_FROM( genesis )
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(barek2ch_readmem,barek2ch_writemem)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( barek3 )

	MDRV_IMPORT_FROM( genesis )
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(barek3_readmem,barek3_writemem)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( megatech )

	/* basic machine hardware */
	MDRV_IMPORT_FROM( genesis_base )
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_HAS_SHADOWS | VIDEO_HAS_HIGHLIGHTS | VIDEO_DUAL_MONITOR)

	MDRV_VIDEO_START(megatech)
	MDRV_VIDEO_UPDATE(megatech)
	MDRV_MACHINE_INIT(megatech)

	MDRV_ASPECT_RATIO(4,6)
	MDRV_SCREEN_SIZE(320,224+192) /* +192 for megatech BIOS screen/menu */
	MDRV_VISIBLE_AREA(0, 319, 0, 223+192)
	MDRV_PALETTE_LENGTH(2048+32) /* +32 for megatech bios vdp part */

	MDRV_CPU_ADD_TAG("megatech_bios", Z80, 53693100 / 15) /* ?? */
	MDRV_CPU_MEMORY(megatech_bios_readmem, megatech_bios_writemem)
	MDRV_CPU_PORTS(megatech_bios_readport,megatech_bios_writeport)
	MDRV_CPU_VBLANK_INT(megatech_irq, 262)
	MDRV_SOUND_ADD(SN76496, megatech_sn76489_intf)

MACHINE_DRIVER_END

static MACHINE_DRIVER_START( megaplay )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M68000, 53693100 / 7)
	MDRV_CPU_MEMORY(megaplay_genesis_readmem, megaplay_genesis_writemem)
	MDRV_CPU_VBLANK_INT(vblank_interrupt,1)

	MDRV_CPU_ADD_TAG("sound", Z80, 53693100 / 15)
	MDRV_CPU_MEMORY(megaplay_z80_readmem, megaplay_z80_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold, 1) /* from vdp at scanline 0xe0 */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION((int)(((262. - 224.) / 262.) * 1000000. / 60.))

	MDRV_INTERLEAVE(100)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_HAS_SHADOWS | VIDEO_HAS_HIGHLIGHTS)
	MDRV_SCREEN_SIZE(320,224)
	MDRV_VISIBLE_AREA(0, 319, 0, 223)
	MDRV_PALETTE_LENGTH(2048+32) /* +32 for megaplay bios vdp part */

	/* sound hardware */
	MDRV_SOUND_ADD(YM2612, gen_ym3438_intf )

/*	MDRV_CPU_PROGRAM_MAP(megaplay_genesis_readmem, genesis_writemem) */

	MDRV_VIDEO_START(megaplay)
	MDRV_VIDEO_UPDATE(megaplay)
	MDRV_MACHINE_INIT(megaplay)

	MDRV_CPU_ADD_TAG("megaplay_bios", Z80, 53693100 / 15) /* ?? */
	MDRV_CPU_MEMORY(megaplay_bios_readmem, megaplay_bios_writemem)
	MDRV_CPU_PORTS(megaplay_bios_readport,megaplay_bios_writeport)
	MDRV_SOUND_ADD(SN76496, megatech_sn76489_intf)
	MDRV_CPU_VBLANK_INT(megatech_irq, 262)
MACHINE_DRIVER_END

/******************************************************************************
	Rom Definitions
*******************************************************************************

	All the known System C/C2 Dumps are listed here with the exception of
	the version of Puzzle & Action (I believe its actually Ichidant-R) which
	was credited to SpainDumps in the included text file.  This appears to be
	a bad dump (half sized roms) however the roms do not match up exactly with
	the good dump of the game.  English language sets are assumed to be the
	parent where they exist.  Hopefully some more alternate version dumps will
	turn up sometime soon for example English Language version of Tant-R or
	Japanese Language versions of Borench (if of course these games were
	released in other locations.

	Games are in Order of Date (Year) with System-C titles coming first.

******************************************************************************/

/***************************************************************************
Puckman Pokemon Genie 2000
(c) 2000?  Manufacturer ?

Hardware looks bootleg-ish, but is newly manufactured.

CPU: ? (one of the SMD chips)
SND: OKI6295, U6612 (probably YM3812), U6614B (Probably YM3014B)
XTAL: 3.579545MHz, 4.0000MHz
OSC: 53.693175MHz
Other Chips: Possible CPU: TA-06SD 9933 B816453 128 pin square SMD
             GFX support chips: Y-BOX TA891945 100 pin SMD
                                TV16B 0010 ME251271 160 pin SMD

There are 13 PAL's on the PCB !

RAM: 62256 x 3, D41264 x 2 (ZIP Ram)
DIPS: 2 x 8 position
SW1:
               			1	2	3	4	5	6	7	8
coins   1coin 1 Cred.  	off	off	off
		2c 1c			on	off	off
		3c 1c			off	on	off
		4c 1c			on	on	off
		5c 1c			off	off	on
		1c 2c			on	off	on
		1c 3c			off	on	on
		1c 4c			on	on	on

players	1							off	off	off
 		2							on	off	off
		3							off	on	off
		4							on	on	off
		5							off	off	on
		6							on	off	on
		7							off	on	on
		8							on	on	on

diffic-
ulty	v.easy									off	off
		normal									on	off
		diffic.									off	on
		v. diffic.								on	on


SW2

note position 3-8 not used

               		1	2	3	4	5	6	7	8
test mode	no		off
			yes		on

demo sound	yes			off
			no			on


ROMS:
PUCKPOKE.U3	M5M27C201	Sound
PUCKPOKE.U4	27C040--\
PUCKPOKE.U5	27C040---\
PUCKPOKE.U7	27C040----- Main program & GFX
PUCKPOKE.U8	27C4001---/

ROM sockets U63 & U64 empty

Hi-res PCB scan available on request.
Screenshots available on my site at http://unemulated.emuunlim.com (under PCB Shop Raid #2)


****************************************************************************/


/* ----- System C Games ----- */

ROM_START( bloxeedc ) /* Bloxeed (C System Version)  (c)1989 Sega / Elorg */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "epr12908.32", 0x000000, 0x020000, CRC(fc77cb91) SHA1(248a462e3858ffdc171af7d806e57deecb5dae50) )
	ROM_LOAD16_BYTE( "epr12907.31", 0x000001, 0x020000, CRC(e5fcbac6) SHA1(a1adec5ef5574bff96a3d66619a24a6715097bb9) )
	ROM_LOAD16_BYTE( "epr12993.34", 0x040000, 0x020000, CRC(487bc8fc) SHA1(3fb205bf56f35443e993e08b39c1a08c13ca5e3b) )
	ROM_LOAD16_BYTE( "epr12992.33", 0x040001, 0x020000, CRC(19b0084c) SHA1(b3ba0f3d8d39a19aa66edb24885ea21192e22704) )
ROM_END


ROM_START( columns ) /* Columns (US) (c)1990 Sega */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "epr13114.32", 0x000000, 0x020000, CRC(ff78f740) SHA1(0a034103a4b942f43e62f6e717f5dbf1bfb0b613) )
	ROM_LOAD16_BYTE( "epr13113.31", 0x000001, 0x020000, CRC(9a426d9b) SHA1(3322e65ebf8d0a6047f7d408387c63ea401b8973) )
ROM_END


ROM_START( columnsj ) /* Columns (Jpn) (c)1990 Sega */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "epr13112.32", 0x000000, 0x020000, CRC(bae6e53e) SHA1(2c2fd621eecd55591f22d076323972a7d0314615) )
	ROM_LOAD16_BYTE( "epr13111.31", 0x000001, 0x020000, CRC(aa5ccd6d) SHA1(480e29e3112282d1790f1fb68075453325ba4336) )
ROM_END


ROM_START( columns2 ) /* Columns II - The Voyage Through Time (Jpn)  (c)1990 Sega */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "epr13361.rom", 0x000000, 0x020000, CRC(b54b5f12) SHA1(4d7fbae7d9bcadd433ebc25aef255dc43df611bc) )
	ROM_LOAD16_BYTE( "epr13360.rom", 0x000001, 0x020000, CRC(a59b1d4f) SHA1(e9ee315677782e1c61ae8f11260101cc03176188) )
ROM_END


ROM_START( tantrbl2 ) /* Tant-R (Puzzle & Action) (Alt Bootleg Running on C Board?, No Samples) */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "trb2_2.32",    0x000000, 0x080000, CRC(8fc99c48) SHA1(d90ed673fe1f6e1f878c0d8fc62f5439b56d0a47) )
	ROM_LOAD16_BYTE( "trb2_1.31",    0x000001, 0x080000, CRC(c318d00d) SHA1(703760d4ddc45bc0921ae96a27d9a8fbf12a1e96) )
	ROM_LOAD16_BYTE( "mpr15616.34",  0x100000, 0x080000, CRC(17b80202) SHA1(f47bf2aa0c5972647438619b8453c7dede5c422f) )
	ROM_LOAD16_BYTE( "mpr15615.33",  0x100001, 0x080000, CRC(36a88bd4) SHA1(cc7f6a947d1b79bb86957c43035b53d6d2bcfa28) )
ROM_END

ROM_START( ichirjbl ) /* Ichident-R (Puzzle & Action 2) (Bootleg Running on C Board?, No Samples) */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "27c4000.2",0x000000, 0x080000, CRC(5a194f44) SHA1(67a4d21b91704f8c2210b5106e82e22ba3366f4c) )
	ROM_LOAD16_BYTE( "27c4000.1",0x000001, 0x080000, CRC(de209f84) SHA1(0860d0ebfab2952e82fc1e292bf9410d673d9322) )
	ROM_LOAD16_BYTE( "epr16888", 0x100000, 0x080000, CRC(85d73722) SHA1(7ebe81b4d6c89f87f60200a3a8cddb07d581adef) )
	ROM_LOAD16_BYTE( "epr16887", 0x100001, 0x080000, CRC(bc3bbf25) SHA1(e760ad400bc183b38e9787d88c8ac084fbe2ae21) )
ROM_END

/* ----- System C-2 Games ----- */

ROM_START( borench ) /* Borench  (c)1990 Sega */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "ic32.bin", 0x000000, 0x040000, CRC(2c54457d) SHA1(adf3ea5393d2633ec6215e64f0cd89ad4567e765) )
	ROM_LOAD16_BYTE( "ic31.bin", 0x000001, 0x040000, CRC(b46445fc) SHA1(24e85ef5abbc5376a854b13ed90f08f0c30d7f25) )

	ROM_REGION( 0x020000, REGION_SOUND1, 0 )
	ROM_LOAD( "ic4.bin", 0x000000, 0x020000, CRC(62b85e56) SHA1(822ab733c87938bb70a9e32cc5dd36bbf6f21d11) )
ROM_END


ROM_START( tfrceac ) /* ThunderForce AC  (c)1990 Technosoft / Sega */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "ic32.bin", 0x000000, 0x040000, CRC(95ecf202) SHA1(92b0f351f2bee7d59873a4991615f14f1afe4da7) )
	ROM_LOAD16_BYTE( "ic31.bin", 0x000001, 0x040000, CRC(e63d7f1a) SHA1(a40d0a5a96f379a467048dc8fddd8aaaeb94da1d) )
	/* 0x080000 - 0x100000 Empty */
	ROM_LOAD16_BYTE( "ic34.bin", 0x100000, 0x040000, CRC(29f23461) SHA1(032a7125fef5a660b85654d595aafc46812cdde6) )
	ROM_LOAD16_BYTE( "ic33.bin", 0x100001, 0x040000, CRC(9e23734f) SHA1(64d27dc53f0ffc3513345a26ed077751b25d15f1) )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 )
	ROM_LOAD( "ic4.bin", 0x000000, 0x040000, CRC(e09961f6) SHA1(e109b5f41502b765d191f22e3bbcff97d6defaa1) )
ROM_END


ROM_START( tfrceacj ) /* ThunderForce AC (Jpn)  (c)1990 Technosoft / Sega */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "epr13657.32", 0x000000, 0x040000, CRC(a0f38ffd) SHA1(da548e7f61aed0e82a460553a119941da8857bc4) )
	ROM_LOAD16_BYTE( "epr13656.31", 0x000001, 0x040000, CRC(b9438d1e) SHA1(598209c9fec3527fde720af09e5bebd7379f5b2b) )
	/* 0x080000 - 0x100000 Empty */
	ROM_LOAD16_BYTE( "ic34.bin",    0x100000, 0x040000, CRC(29f23461) SHA1(032a7125fef5a660b85654d595aafc46812cdde6) )
	ROM_LOAD16_BYTE( "ic33.bin",    0x100001, 0x040000, CRC(9e23734f) SHA1(64d27dc53f0ffc3513345a26ed077751b25d15f1) )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 )
	ROM_LOAD( "ic4.bin", 0x000000, 0x040000, CRC(e09961f6) SHA1(e109b5f41502b765d191f22e3bbcff97d6defaa1) )
ROM_END


ROM_START( tfrceacb ) /* ThunderForce AC (Bootleg)  (c)1990 Technosoft / Sega */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "4.bin",    0x000000, 0x040000, CRC(eba059d3) SHA1(7bc04401f9a138fa151ac09a528b70acfb2021e3) )
	ROM_LOAD16_BYTE( "3.bin",    0x000001, 0x040000, CRC(3e5dc542) SHA1(4a66dc842afaa145dab82b232738eea107bdf0f8) )
	/* 0x080000 - 0x100000 Empty */
	ROM_LOAD16_BYTE( "ic34.bin", 0x100000, 0x040000, CRC(29f23461) SHA1(032a7125fef5a660b85654d595aafc46812cdde6) )
	ROM_LOAD16_BYTE( "ic33.bin", 0x100001, 0x040000, CRC(9e23734f) SHA1(64d27dc53f0ffc3513345a26ed077751b25d15f1) )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 )
	ROM_LOAD( "ic4.bin", 0x000000, 0x040000, CRC(e09961f6) SHA1(e109b5f41502b765d191f22e3bbcff97d6defaa1) )
ROM_END


ROM_START( ribbit ) /* Ribbit  (c)1991 Sega */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "ep13833.32", 0x000000, 0x040000, CRC(5347f8ce) SHA1(b95b99536157edfbf0d74a42f64235f47dca7ee1) )
	ROM_LOAD16_BYTE( "ep13832.31", 0x000001, 0x040000, CRC(889c42c2) SHA1(0839a50a68b64a66d995f1bfaff42fcb60bb4d45) )
	ROM_COPY( REGION_CPU1, 0x000000, 0x080000, 0x080000 )
	ROM_LOAD16_BYTE( "ep13838.34", 0x100000, 0x080000, CRC(a5d62ac3) SHA1(8d83a7bc4017e125ef4231278f766b2368d5fc1f) )
	ROM_LOAD16_BYTE( "ep13837.33", 0x100001, 0x080000, CRC(434de159) SHA1(cf2973131cabf2bc0ebb2bfe9f804ad3d7d0a733) )

	ROM_REGION( 0x080000, REGION_SOUND1, 0 )
	ROM_LOAD( "ep13834.4", 0x000000, 0x020000, CRC(ab0c1833) SHA1(f864e12ecf6c0524da20fc66747a4fa4280e67e9) )
ROM_END


ROM_START( tantr ) /* Tant-R (Puzzle & Action)  (c)1992 Sega */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "epr15614.32", 0x000000, 0x080000, CRC(557782bc) SHA1(1546a999ab97c380dc87f6c95d5687722206740d) )
	ROM_LOAD16_BYTE( "epr15613.31", 0x000001, 0x080000, CRC(14bbb235) SHA1(8dbfec5fb1d7a695acbb2fc0e78e4bdf76eb8d9d) )
	ROM_LOAD16_BYTE( "mpr15616.34", 0x100000, 0x080000, CRC(17b80202) SHA1(f47bf2aa0c5972647438619b8453c7dede5c422f) )
	ROM_LOAD16_BYTE( "mpr15615.33", 0x100001, 0x080000, CRC(36a88bd4) SHA1(cc7f6a947d1b79bb86957c43035b53d6d2bcfa28) )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 )
	ROM_LOAD( "epr15617.4", 0x000000, 0x040000, CRC(338324a1) SHA1(79e2782d0d4764dd723886f846c852a6f6c1fb64) )
ROM_END

ROM_START( tantrkor ) /* Tant-R (Puzzle & Action) (Korea) (c)1993 Sega */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	/* strange names, but this is what was printed on the (original) chips */
	ROM_LOAD16_BYTE( "m15592b.32", 0x000000, 0x080000, CRC(7efe26b3) SHA1(958420b9b400eafe392745af90bff729463427c7) )
	ROM_LOAD16_BYTE( "m15592b.31", 0x000001, 0x080000, CRC(af5a860f) SHA1(cb0011f420721d035e9f0e43bb72cf286982fd32) )
	ROM_LOAD16_BYTE( "m15992b.34", 0x100000, 0x080000, CRC(6282a5d4) SHA1(9220e119e79d969d7d70e8a25c75dd3d9bc340ae) )
	ROM_LOAD16_BYTE( "m15592b.33", 0x100001, 0x080000, CRC(82d78413) SHA1(9ff9c2b1632e280444965110bab90c0fc98cd6da) )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 )
	ROM_LOAD( "epr15617.4", 0x000000, 0x040000, CRC(338324a1) SHA1(79e2782d0d4764dd723886f846c852a6f6c1fb64) )
ROM_END

ROM_START( tantrbl ) /* Tant-R (Puzzle & Action) (Bootleg)  (c)1992 Sega */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "pa_e10.bin",  0x000000, 0x080000, CRC(6c3f711f) SHA1(55aa2d50422134b95d9a7c5cbdc453b207b91b4c) )
	ROM_LOAD16_BYTE( "pa_f10.bin",  0x000001, 0x080000, CRC(75526786) SHA1(8f5aa7f6918b71a79e6fca18194beec2aef15844) )
	ROM_LOAD16_BYTE( "mpr15616.34", 0x100000, 0x080000, CRC(17b80202) SHA1(f47bf2aa0c5972647438619b8453c7dede5c422f) )
	ROM_LOAD16_BYTE( "mpr15615.33", 0x100001, 0x080000, CRC(36a88bd4) SHA1(cc7f6a947d1b79bb86957c43035b53d6d2bcfa28) )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 )
	ROM_LOAD( "pa_e03.bin", 0x000000, 0x020000, CRC(72918c58) SHA1(cb42363b163727a887a0b762519c72dcdf0a6460) )
	ROM_LOAD( "pa_e02.bin", 0x020000, 0x020000, CRC(4e85b2a3) SHA1(3f92fb931d315c5a2d6c54b3204718574928cb7b) )
ROM_END


ROM_START( puyopuyo	) /* Puyo Puyo  (c)1992 Sega / Compile */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "epr15036", 0x000000, 0x020000, CRC(5310ca1b) SHA1(dcfe2bf7476b640dfb790e8716e75b483d535e48) )
	ROM_LOAD16_BYTE( "epr15035", 0x000001, 0x020000, CRC(bc62e400) SHA1(12bb6031574838a28889f6edb31dbb689265287c) )
	/* 0x040000 - 0x100000 Empty */
	ROM_LOAD16_BYTE( "epr15038", 0x100000, 0x020000, CRC(3b9eea0c) SHA1(e3e6148c1769834cc0061932eb035daa79673144) )
	ROM_LOAD16_BYTE( "epr15037", 0x100001, 0x020000, CRC(be2f7974) SHA1(77027ced7a62f94e9fc6e8a0a4ac0c62f7ea813b) )

	ROM_REGION( 0x020000, REGION_SOUND1, 0 )
	ROM_LOAD( "epr15034", 0x000000, 0x020000, CRC(5688213b) SHA1(f3f234e482871ca903a782e51008f3bfed04ee63) )
ROM_END


ROM_START( puyopuya	) /* Puyo Puyo (Rev A)  (c)1992 Sega / Compile */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "ep15036a.32", 0x000000, 0x020000, CRC(61b35257) SHA1(e09a7e992999befc88fc7928a478d1e2d14d7b08) )
	ROM_LOAD16_BYTE( "ep15035a.31", 0x000001, 0x020000, CRC(dfebb6d9) SHA1(6f685729ef4660c2eba409c5236c6d2f313eef5b) )
	/* 0x040000 - 0x100000 Empty */
	ROM_LOAD16_BYTE( "epr15038",    0x100000, 0x020000, CRC(3b9eea0c) SHA1(e3e6148c1769834cc0061932eb035daa79673144) )
	ROM_LOAD16_BYTE( "epr15037",    0x100001, 0x020000, CRC(be2f7974) SHA1(77027ced7a62f94e9fc6e8a0a4ac0c62f7ea813b) )

	ROM_REGION( 0x020000, REGION_SOUND1, 0 )
	ROM_LOAD( "epr15034", 0x000000, 0x020000, CRC(5688213b) SHA1(f3f234e482871ca903a782e51008f3bfed04ee63) )
ROM_END


ROM_START( puyopuyb ) /* Puyo Puyo  (c)1992 Sega / Compile  Bootleg */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "puyopuyb.4bo", 0x000000, 0x020000, CRC(89ea4d33) SHA1(bef9d011524e71c072d309f6da3c2ebc38878e0e) )
	ROM_LOAD16_BYTE( "puyopuyb.3bo", 0x000001, 0x020000, CRC(c002e545) SHA1(7a59ac764d60e9955830d9617b0bd122b44e7b2f) )
	/* 0x040000 - 0x100000 Empty */
	ROM_LOAD16_BYTE( "puyopuyb.6bo", 0x100000, 0x020000, CRC(0a0692e5) SHA1(d4ecc5b1791a91e3b33a5d4d0dd305f1623483d9) )
	ROM_LOAD16_BYTE( "puyopuyb.5bo", 0x100001, 0x020000, CRC(353109b8) SHA1(92440987add3124b758e7eaa77a3a6f54ca61bb8) )

	ROM_REGION( 0x020000, REGION_SOUND1, 0 )
	ROM_LOAD( "puyopuyb.abo", 0x000000, 0x020000, CRC(79112b3b) SHA1(fc3a202e1e2ff39950d4af689b7fcca86c301805) )
ROM_END

ROM_START( ichir ) /* Ichident-R (Puzzle & Action 2)  (c)1994 Sega (World) */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "pa2_32.bin", 0x000000, 0x080000, CRC(7ba0c025) SHA1(855e9bb2a20c6f51b26381233c57c26aa96ad1f6) )
	ROM_LOAD16_BYTE( "pa2_31.bin", 0x000001, 0x080000, CRC(5f86e5cc) SHA1(44e201de00dfbf7c66d0e0d40d17b162c6f0625b) )
	ROM_LOAD16_BYTE( "epr16888",   0x100000, 0x080000, CRC(85d73722) SHA1(7ebe81b4d6c89f87f60200a3a8cddb07d581adef) )
	ROM_LOAD16_BYTE( "epr16887",   0x100001, 0x080000, CRC(bc3bbf25) SHA1(e760ad400bc183b38e9787d88c8ac084fbe2ae21) )

	ROM_REGION( 0x080000, REGION_SOUND1, 0 )
	ROM_LOAD( "pa2_02.bin", 0x000000, 0x080000, CRC(fc7b0da5) SHA1(46770aa7e19b4f8a183be3f433c48ad677b552b1) )
ROM_END

ROM_START( ichirk ) /* Ichident-R (Puzzle & Action 2)  (c)1994 Sega (Korea) */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	/* Again the part numbers are quite strange for the Korean verison */
	ROM_LOAD16_BYTE( "epr_ichi.32", 0x000000, 0x080000, CRC(804dea11) SHA1(40bf8cbd40969a5880df10914252b7f64d5ce8e9) )
	ROM_LOAD16_BYTE( "epr_ichi.31", 0x000001, 0x080000, CRC(92452353) SHA1(d2e1da5b139965611cd8d707d23396b5d4c07d12) )
	ROM_LOAD16_BYTE( "epr16888",   0x100000, 0x080000, CRC(85d73722) SHA1(7ebe81b4d6c89f87f60200a3a8cddb07d581adef) )  /* m17235a.34 */
	ROM_LOAD16_BYTE( "epr16887",   0x100001, 0x080000, CRC(bc3bbf25) SHA1(e760ad400bc183b38e9787d88c8ac084fbe2ae21) )  /* m17220a.33 */

	ROM_REGION( 0x080000, REGION_SOUND1, 0 )
	ROM_LOAD( "pa2_02.bin", 0x000000, 0x080000, CRC(fc7b0da5) SHA1(46770aa7e19b4f8a183be3f433c48ad677b552b1) ) /* m17220a.4 */
ROM_END

ROM_START( ichirj ) /* Ichident-R (Puzzle & Action 2)  (c)1994 Sega (Japan) */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "epr16886", 0x000000, 0x080000, CRC(38208e28) SHA1(07fc634bdf2d3e25274c9c374b3506dec765114c) )
	ROM_LOAD16_BYTE( "epr16885", 0x000001, 0x080000, CRC(1ce4e837) SHA1(16600600e12e3f35e3da89524f7f51f019b5ad17) )
	ROM_LOAD16_BYTE( "epr16888", 0x100000, 0x080000, CRC(85d73722) SHA1(7ebe81b4d6c89f87f60200a3a8cddb07d581adef) )
	ROM_LOAD16_BYTE( "epr16887", 0x100001, 0x080000, CRC(bc3bbf25) SHA1(e760ad400bc183b38e9787d88c8ac084fbe2ae21) )

	ROM_REGION( 0x080000, REGION_SOUND1, 0 )
	ROM_LOAD( "epr16884", 0x000000, 0x080000, CRC(fd9dcdd6) SHA1(b8053a2e68072e7664ffc3c53f983f3ba72a892b) )
ROM_END


ROM_START( stkclmns ) /* Stack Columns  (c)1994 Sega */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "epr16795.32", 0x000000, 0x080000, CRC(b478fd02) SHA1(aaf9d9f9f4dc900b4e8ff6f258f26e782e5c3166) )
	ROM_LOAD16_BYTE( "epr16794.31", 0x000001, 0x080000, CRC(6d0e8c56) SHA1(8f98d9fd98a1faa70b173cfd72f15102d11e79ae) )
	ROM_LOAD16_BYTE( "mpr16797.34", 0x100000, 0x080000, CRC(b28e9bd5) SHA1(227eb591d10c9dbc52b35954ebd322e2a4451df2) )
	ROM_LOAD16_BYTE( "mpr16796.33", 0x100001, 0x080000, CRC(ec7de52d) SHA1(85bc48cef15e615ad9059500808d17916c854a87) )

	ROM_REGION( 0x020000, REGION_SOUND1, 0 )
	ROM_LOAD( "epr16793.4", 0x000000, 0x020000, CRC(ebb2d057) SHA1(4a19ee5d71e4aabe7d9b9b968ab5ee4bc6262aad) )
ROM_END


ROM_START( puyopuy2 ) /* Puyo Puyo 2  (c)1994 Compile */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "pp2.eve", 0x000000, 0x080000, CRC(1cad1149) SHA1(77fb0482fa35e615c0bed65f4d7f4dd89b241f23) )
	ROM_LOAD16_BYTE( "pp2.odd", 0x000001, 0x080000, CRC(beecf96d) SHA1(c2bdad4b6184c11f81f2a5db409cb4ea186205a7) )

	ROM_REGION( 0x080000, REGION_SOUND1, 0 )
	ROM_LOAD( "pp2.snd", 0x000000, 0x080000, CRC(020ff6ef) SHA1(6095b8277b47a6fd7a9721f15a70ae5bf6be9b1a) )
ROM_END

ROM_START( potopoto ) /* Poto Poto  (c)1994 Sega */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "epr16662", 0x000000, 0x040000, CRC(bbd305d6) SHA1(1a4f4869fefac188c69bc67df0b625e43a0c3f1f) )
	ROM_LOAD16_BYTE( "epr16661", 0x000001, 0x040000, CRC(5a7d14f4) SHA1(a615b5f481256366db7b1c6302a8dcb69708102b) )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 )
	ROM_LOAD( "epr16660", 0x000000, 0x040000, CRC(8251c61c) SHA1(03eef3aa0bdde2c1d93128648f54fd69278d85dd) )
ROM_END

ROM_START( zunkyou ) /* Zunzunkyou No Yabou  (c)1994 Sega */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "epr16812.32", 0x000000, 0x080000, CRC(eb088fb0) SHA1(69089a3516ad50f35e81971ef3c33eb3f5d52374) )
	ROM_LOAD16_BYTE( "epr16811.31", 0x000001, 0x080000, CRC(9ac7035b) SHA1(1803ffbadc1213e04646d483e27da1591e22cd06) )
	ROM_LOAD16_BYTE( "epr16814.34", 0x100000, 0x080000, CRC(821b3b77) SHA1(c45c7393a792ce8306a52f83f8ed8f6b0d7c11e9) )
	ROM_LOAD16_BYTE( "epr16813.33", 0x100001, 0x080000, CRC(3cba9e8f) SHA1(208819bc1a205eaab089542afc7a59f69ce5bb81) )

	ROM_REGION( 0x080000, REGION_SOUND1, 0 )
	ROM_LOAD( "epr16810.4", 0x000000, 0x080000, CRC(d542f0fe) SHA1(23ea50110dfe1cd9f286a535d15e0c3bcba73b00) )
ROM_END

ROM_START( headonch ) /* Head On Channel (Prototype) (c)1994 Sega */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "headonch.ic32", 0x000000, 0x080000, CRC(091cf538) SHA1(04673678f543743b395edea39ad4ee6177436dc0) )
	ROM_LOAD16_BYTE( "headonch.ic31", 0x000001, 0x080000, CRC(91f3b5f1) SHA1(15cbe7a172dde7de7b73f0c9eeddfee41e8d1f80) )
	ROM_LOAD16_BYTE( "headonch.ic34", 0x100000, 0x080000, CRC(d8dc6323) SHA1(e7e891324764641691dcb63e5222f2ed9207fb96) )
	ROM_LOAD16_BYTE( "headonch.ic33", 0x100001, 0x080000, CRC(3268e38b) SHA1(10ded2be01465014ca9e6c64ffab1190ec985359) )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 )
    ROM_LOAD( "headonch.ic4", 0x000000, 0x040000, CRC(90af7301) SHA1(227227cb5d0df6612bac7b4c94b99e2287686ccd) )
ROM_END

ROM_START( ooparts ) /* Oo Parts (Prototype) (c)1992 Sega / Success */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "ooparts.ic32", 0x000000, 0x080000, CRC(8dcf2940) SHA1(f72630e8a26e7f2089da56878a1599268c355246) )
	ROM_LOAD16_BYTE( "ooparts.ic31", 0x000001, 0x080000, CRC(35381899) SHA1(524f6e1b1292542079589275e20f45c2eb68605c) )
	ROM_LOAD16_BYTE( "ooparts.ic34", 0x100000, 0x080000, CRC(7192ac29) SHA1(d3028a9bbb7faa733285cf7e47fd840ec0d0bf69) )
	ROM_LOAD16_BYTE( "ooparts.ic33", 0x100001, 0x080000, CRC(42755dc2) SHA1(cd0aa79418b922266c5d41bf24b9136f9f105dc5) )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 )
	ROM_LOAD( "epr-13655.ic4", 0x000000, 0x040000, CRC(e09961f6) SHA1(e109b5f41502b765d191f22e3bbcff97d6defaa1) )
ROM_END

ROM_START( ssonicbr )  /* Sega Sonic Bros (Prototype) (c)1992 Sega */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "ssonicbr.ic32", 0x000000, 0x040000, CRC(cf254ecd) SHA1(4bb295ec80f8ddfeab4e360eebf12c5e2dfb9800) )
	ROM_LOAD16_BYTE( "ssonicbr.ic31", 0x000001, 0x040000, CRC(03709746) SHA1(0b457f557da77acd3f43950428117c1decdfaf26) )

	ROM_REGION( 0x020000, REGION_SOUND1, 0 )
	ROM_LOAD( "ssonicbr.ic4", 0x000000, 0x020000, CRC(78e56a51) SHA1(8a72c12975cd74919b4337e0f681273e6b5cbbc6) )
ROM_END

ROM_START( puckpkmn ) /* Puckman Pockimon  (c)2000 Genie */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "puckpoke.u5", 0x000000, 0x080000, CRC(fd334b91) SHA1(cf8bf6645a4082ea4392937e169b1686c9c7e246) )
	ROM_LOAD16_BYTE( "puckpoke.u4", 0x000001, 0x080000, CRC(839cc76b) SHA1(e15662a7175db7a8e222dda176a8ed92e0d56e9d) )
	ROM_LOAD16_BYTE( "puckpoke.u8", 0x100000, 0x080000, CRC(7936bec8) SHA1(4b350105abe514fbfeabae1c6f3aeee695c3d07a) )
	ROM_LOAD16_BYTE( "puckpoke.u7", 0x100001, 0x080000, CRC(96b66bdf) SHA1(3cc2861ad9bc232cbe683e01b58090f832d03db5) )

	ROM_REGION( 0x40000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "puckpoke.u3", 0x00000, 0x40000, CRC(7b066bac) SHA1(429616e21c672b07e0705bc63234249cac3af56f) )
ROM_END

ROM_START( jzth )
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "s.y.u5", 0x000000, 0x080000, CRC(a4a526b5) SHA1(85d0299caf91ff50b6870f845b9aacbd358ed81f) )
	ROM_LOAD16_BYTE( "s.y.u4", 0x000001, 0x080000, CRC(c16654eb) SHA1(dca4b772a3b9caa7be3fa01511c401b591c2e6f3) )
	ROM_LOAD16_BYTE( "s.y.u8", 0x100000, 0x080000, CRC(b62e1068) SHA1(2484ae49a4a2a2c551b3b84bbc0b4e40e5d281e7) )
	ROM_LOAD16_BYTE( "s.y.u7", 0x100001, 0x080000, CRC(27fe424c) SHA1(14bee8c16aac3d5b04123c994167531f817634fd) )
	ROM_LOAD16_BYTE( "s.y.u64", 0x200000, 0x080000, CRC(62f52886) SHA1(07fc9765274c03eff4a09f48a0b1b2b2afc6078e) )
	ROM_LOAD16_BYTE( "s.y.u63", 0x200001, 0x080000, CRC(a6a32c8c) SHA1(d0c779751e4af459e9bf63e55c5e2b19a243b70d) )
	ROM_LOAD16_BYTE( "s.y.u66", 0x300000, 0x080000, CRC(fa4a09f5) SHA1(67d77c91a994ecb8b29e7661c3a12e84a64eb837))
	ROM_LOAD16_BYTE( "s.y.u65", 0x300001, 0x080000, CRC(de64e526) SHA1(e3b3e5c95b8ae36c0c57f8c9a6f55084464c4c05) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* there are 2 banks in here, so find bank switch */
	ROM_LOAD( "s.y.u3", 0x00000, 0x40000, CRC(38eef2f2) SHA1(2f750dbf71fea0622e8493f0a8be7c43555ed5cf) )
	ROM_CONTINUE(0x40000,0x40000)
ROM_END

ROM_START( sbubsm )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "u11.bin", 0x000000, 0x080000, CRC(4f9337ea) SHA1(b245eb615f80afd25e29b2efdddb7f61c1deff6b) )
	ROM_LOAD16_BYTE( "u12.bin", 0x000001, 0x080000, CRC(f5374835) SHA1(3a97910f5f7327ec7ad6425dfdfa72c86196ed33) )

	ROM_REGION( 0x1000, REGION_CPU2, 0 ) /* could be the same as topshoot (same PCB) */
/*	ROM_LOAD( "89c51.bin", 0x0000, 0x1000, NO_DUMP ) */
ROM_END

ROM_START( aladmdb )
	ROM_REGION( 0x400000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "m1.bin", 0x000001, 0x080000,  CRC(5e2671e4) SHA1(54705c7614fc7b5a1065478fa41f51dd1d8045b7) )
	ROM_LOAD16_BYTE( "m2.bin", 0x000000, 0x080000,  CRC(142a0366) SHA1(6c94aa9936cd11ccda503b52019a6721e64a32f0) )
	ROM_LOAD16_BYTE( "m3.bin", 0x100001, 0x080000,  CRC(0feeeb19) SHA1(bd567a33077ab9997871d21736066140d50e3d70) )
	ROM_LOAD16_BYTE( "m4.bin", 0x100000, 0x080000,  CRC(bc712661) SHA1(dfd554d000399e17b4ddc69761e572195ed4e1f0))
ROM_END

ROM_START( barek2ch ) /* all 27c4001 */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "u14", 0x000001, 0x080000, CRC(b0ee177f) SHA1(d63e6ee30fe7f4aaab098d3920eabc456730b2c5) )
	ROM_LOAD16_BYTE( "u15", 0x000000, 0x080000, CRC(09264195) SHA1(c5439731d932c90a57d68c4d82c9ebed8a01bd53) )
	ROM_LOAD16_BYTE( "u16", 0x100001, 0x080000, CRC(6c814fc4) SHA1(edaf5117b19d3fb40218c5f7c4b5099c9189f1be) )
	ROM_LOAD16_BYTE( "u17", 0x100000, 0x080000, CRC(cae1922e) SHA1(811c2164b6c467a49af4b0d22f151cd13c9efbc9) )
ROM_END

ROM_START( barek3mb )
	ROM_REGION( 0x400000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "6.u19", 0x000000, 0x080000,  CRC(2de19519) SHA1(f5fcef1da8b5370e399f0451382e3c6e7754c9c8) )
	ROM_LOAD16_BYTE( "3.u18", 0x000001, 0x080000,  CRC(db900e82) SHA1(172a4fe01a0ffd1ea3aed74f2c58234fd55b876d) )
	ROM_LOAD16_BYTE( "4.u15", 0x100000, 0x080000,  CRC(6353b4b1) SHA1(9f89a2f02170496ca798b89e37e1f2bae0e9155d) )
	ROM_LOAD16_BYTE( "1.u14", 0x100001, 0x080000,  CRC(24d31e12) SHA1(64c1b968e1ee5d0355d902e280f33e4466f27b07) )
	ROM_LOAD16_BYTE( "5.u17", 0x200000, 0x080000,  CRC(0feb974f) SHA1(ed1a25b6f1669dc6061d519985b6373fa89176c7) )
	ROM_LOAD16_BYTE( "2.u16", 0x200001, 0x080000,  CRC(bba4a585) SHA1(32c59729943d7b4c1a39f2a2b0dae9ce16991e9c) )
ROM_END

ROM_START( sonic2mb )
	ROM_REGION( 0x400000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "m1", 0x000001, 0x080000,  CRC(7b40aa24) SHA1(247882cd1f412366d61aeb4d85bbeefd5f108e1d) )
	ROM_LOAD16_BYTE( "m2", 0x000000, 0x080000,  CRC(84b3f758) SHA1(19846b9d951db6f78f3e155d33f1b6349fb87f1a) )
ROM_END

ROM_START( pclubj ) /* Print Club (c)1995 Atlus */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "epr18171.32", 0x000000, 0x080000, CRC(6c8eb8e2) SHA1(bbd885a83269524215c1d8470544086e3e82c05c) )
	ROM_LOAD16_BYTE( "epr18170.31", 0x000001, 0x080000, CRC(72c631e6) SHA1(77c4ed793db6cb75346998f38a637db64fd258bd) )
	ROM_LOAD16_BYTE( "epr18173.34", 0x100000, 0x080000, CRC(9809dc72) SHA1(6dbe6b7d4e525aa9b6174f8dc5aee12a5e00a009) )
	ROM_LOAD16_BYTE( "epr18172.33", 0x100001, 0x080000, CRC(c61d819b) SHA1(4813ed3161e16099f482e0cf8df3cbe6c01c619c) )

	ROM_REGION( 0x080000, REGION_SOUND1, 0 )
	ROM_LOAD( "epr18169.4", 0x000000, 0x080000, CRC(5c00ccfb) SHA1(d043ffa6528bb9b76774c96df4edf8222a1878a4) )
ROM_END

ROM_START( pclubjv2 ) /* Print Club vol.2 (c)1995 Atlus */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "p2jwn.u32", 0x000000, 0x080000, CRC(dfc0f7f1) SHA1(d2399f3ff05006590903f943cd77a9c709b9b5b1) )
	ROM_LOAD16_BYTE( "p2jwn.u31", 0x000001, 0x080000, CRC(6ab4c694) SHA1(d8cfaa1a49e86842079c6e3800a95c5afaf76ab6) )
	ROM_LOAD16_BYTE( "p2jwn.u34", 0x100000, 0x080000, CRC(854fd456) SHA1(eff7413a7acd8ee37cb73bc8dfd4f4ae53c04836) )
	ROM_LOAD16_BYTE( "p2jwn.u33", 0x100001, 0x080000, CRC(64428a69) SHA1(e2c5ead4b35db76fda1db03adcd020bde5ca1dd2) )

	ROM_REGION( 0x080000, REGION_SOUND1, 0 )
	ROM_LOAD( "epr18169.4", 0x000000, 0x080000, CRC(5c00ccfb) SHA1(d043ffa6528bb9b76774c96df4edf8222a1878a4) )
ROM_END

ROM_START( pclubjv4 ) /* Print Club vol.4 (c)1996 Atlus */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "p4jsm.u32", 0x000000, 0x080000, CRC(36ff5f80) SHA1(33872aa00c8ca3f54dd7503a44562fbdad92df7d) )
	ROM_LOAD16_BYTE( "p4jsm.u31", 0x000001, 0x080000, CRC(f3c021ad) SHA1(34792d861265b609d5022955eb7d2f471c63dfb8) )
	ROM_LOAD16_BYTE( "p4jsm.u34", 0x100000, 0x080000, CRC(d0fd4b33) SHA1(c272404f09bdb6596740ab150eb158cc22cc9aa6) )
	ROM_LOAD16_BYTE( "p4jsm.u33", 0x100001, 0x080000, CRC(ec667875) SHA1(d235a1d8dfa90e1c638e1f079ce528f61450e1f0) )

	ROM_REGION( 0x080000, REGION_SOUND1, 0 )
	ROM_LOAD( "epr18169.4", 0x000000, 0x080000, CRC(5c00ccfb) SHA1(d043ffa6528bb9b76774c96df4edf8222a1878a4) )
ROM_END

ROM_START( pclubjv5 ) /* Print Club vol.5 (c)1996 Atlus */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "p5jat.u32", 0x000000, 0x080000, CRC(72220e69) SHA1(615de759d73469841987fb028eaf5d5598c32553) )
	ROM_LOAD16_BYTE( "p5jat.u31", 0x000001, 0x080000, CRC(06d83fde) SHA1(dc68375ccb16cde7900eb05f702bc15e7e702ea5) )
	ROM_LOAD16_BYTE( "p5jat.u34", 0x100000, 0x080000, CRC(b172ab58) SHA1(47a70bd678f6c4dafe70b83bd3db678cf44de48b) )
	ROM_LOAD16_BYTE( "p5jat.u33", 0x100001, 0x080000, CRC(ba38ec50) SHA1(666fdba56d8a4dab041015c5e8102305b491d293) )

	ROM_REGION( 0x080000, REGION_SOUND1, 0 )
	ROM_LOAD( "epr18169.4", 0x000000, 0x080000, CRC(5c00ccfb) SHA1(d043ffa6528bb9b76774c96df4edf8222a1878a4) )
ROM_END

/* MegaTech Games - Genesis & sms! Games with a timer */

/* 12368-xx  xx is the game number? if so there are a _lot_ of carts, mt_beast is 01, mt_sonic is 52! */

ROM_START( megatech )
	ROM_REGION( 0x8000, REGION_USER1, 0 )
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_beast ) /* Altered Beast */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mp12538.ic1", 0x000000, 0x080000, CRC(3bea3dce) SHA1(ec72e4fde191dedeb3f148f132603ed3c23f0f86) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* z80 */

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "12368-01.ic2", 0x000000, 0x08000, CRC(40cb0088) SHA1(e1711532c29f395a35a1cb34d789015881b5a1ed) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_astro ) /* Astro Warrior (Sms Game!) */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	/* z80 code because this is sms based .... */
	ROM_LOAD( "ep13817.ic2", 0x000000, 0x020000, CRC(299cbb74) SHA1(901697a3535ad70190647f34ad5b30b695d54542) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* z80 */

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "12368-13.ic1", 0x000000, 0x08000,  CRC(4038cbd1) SHA1(696bc1efce45d9f0052b2cf0332a232687c8d6ab) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END


ROM_START( mt_wcsoc ) /* World Cup Soccer */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "12607.ic1", 0x000000, 0x080000, CRC(bc591b30) SHA1(55e8577171c0933eee53af1dabd0f4c6462d5fc8) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* z80 */

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "12368-21.ic2", 0x000000, 0x08000, CRC(028ee46b) SHA1(cd8f81d66e5ae62107eb20e0ca5db4b66d4b2987) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_gng ) /* Ghouls and Ghosts (bad dump?) */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mp12605.ic1", 0x000000, 0x080000, BAD_DUMP CRC(0c6d6f25) SHA1(96c9c0e41036a23fc8e75e18ee2dad87654f200f) )
	ROM_LOAD16_WORD_SWAP( "mpr12605.14", 0x080000, 0x020000, BAD_DUMP CRC(1066c6ab) SHA1(c30e4442732bdb38c96d780542f8550a94d127b0) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* z80 */

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "12368-23.ic3", 0x000000, 0x08000, CRC(7ee58546) SHA1(ad5bb0934475eacdc5e354f67c96fe0d2512d33b) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END


ROM_START( mt_gaxe ) /* Golden Axe */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "12806.ic1", 0x000000, 0x080000, CRC(43456820) SHA1(2f7f1fcd979969ac99426f11ab99999a5494a121) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* z80 */

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "12368-25.ic2", 0x000000, 0x08000, CRC(1f07ed28) SHA1(9d54192f4c6c1f8a51c38a835c1dd1e4e3e8279e) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_smgp ) /* Super Monaco Grand Prix */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "13250.ic1", 0x000000, 0x080000, CRC(189b885f) SHA1(31c06ffcb48b1604989a94e584261457de4f1f46) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* z80 */

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "12368-39.ic2", 0x000000, 0x08000, CRC(64b3ce25) SHA1(83a9f2432d146a712b037f96f261742f7dc810bb) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_sonic ) /* Sonic */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mp13913.ic1", 0x000000, 0x080000, CRC(480b4b5c) SHA1(ab1dc1f738e3b2d0898a314b123fa71182bf572e) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* z80 */

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "12368-52.ic2", 0x0000, 0x8000,  CRC(6a69d20c) SHA1(e483b39ff6eca37dc192dc296d004049e220554a) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_gaxe2 ) /* Golden Axe 2 */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mp14272.ic1", 0x000000, 0x080000, CRC(d4784cae) SHA1(b6c286027d06fd850016a2a1ee1f1aeea080c3bb) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* z80 */

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "12368-57.ic2", 0x000000, 0x08000, CRC(dc9b4433) SHA1(efd3a598569010cdc4bf38ecbf9ed1b4e14ffe36) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_fshrk ) /* Fire Shark */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mp14341.ic1", 0x000000, 0x080000, CRC(04d65ebc) SHA1(24338aecdc52b6f416548be722ca475c83dbae96) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* z80 */

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "12368-53.ic2", 0x000000, 0x08000,  CRC(4fa61044) SHA1(7810deea221c10b0b2f5233443d81f4f1998ee58) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END


ROM_START( mt_eswat ) /* E-Swat */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mp13192.ic1", 0x000000, 0x080000, CRC(82f458ef) SHA1(58444b783312def71ecffc4ad021b72a609685cb) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* z80 */

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "12368-38.ic2", 0x000000, 0x08000, CRC(43c5529b) SHA1(104f85adea6da1612c0aa96d553efcaa387d7aaf) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_bbros ) /* Bonanza Bros */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mp13905a.ic1", 0x000000, 0x100000, CRC(68a88d60) SHA1(2f56e8a2b0999de4fa0d14a1527f4e1df0f9c7a2) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* z80 */

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "12368-49.ic2", 0x000000, 0x08000, CRC(c5101da2) SHA1(636f30043e2e9291e193ef9a2ead2e97a0bf7380) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_kcham ) /* Kid Chameleon */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mp14557.ic1", 0x000000, 0x100000, CRC(e1a889a4) SHA1(a2768eacafc47d371e5276f0cce4b12b6041337a) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* z80 */

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "12368-60.ic2", 0x000000, 0x08000, CRC(a8e4af18) SHA1(dfa49f6ec4047718f33dba1180f6204dbaff884c) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_lastb ) /* Last Battle */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mp12578f.ic1", 0x000000, 0x080000, CRC(531191a0) SHA1(f6bc26e975c01a3e10ab4033e4c5f494627a1e2f) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* z80 */

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "12368-20.ic2", 0x000000, 0x08000, CRC(e1a71c91) SHA1(c250da18660d8aea86eb2abace41ba46130dabc8) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_mwalk ) /* Moon Walker */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mp13285a.ic1", 0x000000, 0x080000, CRC(189516e4) SHA1(2a79e07da2e831832b8d448cae87a833c85e67c9) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* z80 */

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "12368-40.ic2", 0x000000, 0x08000, CRC(0482378c) SHA1(734772f3ddb5ff82b76c3514d18a464b2bce8381) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_mystd ) /* Mystic Defender */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mp12707.1", 0x000000, 0x080000, CRC(4f2c513d) SHA1(f9bb548b3688170fe18bb3f1b5b54182354143cf) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* z80 */

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "12368-27.ic2", 0x000000, 0x08000, CRC(caf46f78) SHA1(a9659e86a6a223646338cd8f29c346866e4406c7) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_shar2 ) /* Space Harrier 2 */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mp11934.ic1", 0x000000, 0x080000, CRC(932daa09) SHA1(a2d7a76f3604c6227d43229908bfbd02b0ef5fd9) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* z80 */

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "12368-02.ic1", 0x000000, 0x08000, CRC(c129c66c) SHA1(e7c0c97db9df9eb04e2f9ff561b64305219b8f1f) ) /* ic2?*/

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_stbld ) /* Super Thunder Blade */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mp11996f.ic1", 0x000000, 0x080000,  CRC(9355c34e) SHA1(26ff91c2921408673c644b0b1c8931d98524bf63) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* z80 */

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "12368-03.ic2", 0x000000, 0x08000,  CRC(1ba4ac5d) SHA1(9bde57d70189d159ebdc537a9026001abfd0deae) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_tetri ) /* Tetris (Bad Dump) */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "mp12356.ic1", 0x000001, 0x020000, BAD_DUMP CRC(136767d2) SHA1(4fa07facda60fa85588a997ff9ff2ddde0dc2534) ) /* same data in each half, fails self-check (red screen) doesn't work in GENS*/
	ROM_LOAD16_BYTE( "mp12357.ic2", 0x000000, 0x020000, BAD_DUMP CRC(6d5a20f2) SHA1(e8aef99621974a137ea81a597f600881741f94d1) ) /* same data in each half, fails self-check (red screen)*/

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* z80 */

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "12368-22.ic3", 0x000000, 0x08000, CRC(1c1b6468) SHA1(568a38f4186167486e39ab4aa2c1ceffd0b81156) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_tfor2 ) /* Thunder Force 2 */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mp12559.ic1", 0x000000, 0x080000, CRC(b093bee3) SHA1(0bf6194c3d228425f8cf1903ed70d8da1b027b6a) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* z80 */

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "12368-11.ic2", 0x000000, 0x08000, CRC(f4f27e8d) SHA1(ae1a2823deb416c53838115966f1833d5dac72d4) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_tlbba ) /* Tommy Lasorda Baseball */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mp12706.ic1", 0x000000, 0x080000, CRC(8901214f) SHA1(f5ec166be1cf9b86623b9d7a78ec903b899da32a) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* z80 */

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "12368-35.ic2", 0x000000, 0x08000, CRC(67bbe482) SHA1(6fc283b22e68befabb44b2cc61a7f82a71d6f029) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_ggolf ) /* Great Golf (Bad Dump) */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mp11129f.ic1", 0x000000, 0x020000, BAD_DUMP CRC(942738ba) SHA1(e99d4e39c965fc123a39d75521a274687e917a57) ) /* first 32kb is repeated 4 times, doesn't work in MEKA*/

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* z80 */

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "12368-04.ic2", 0x000000, 0x08000, CRC(62e5579b) SHA1(e1f531be5c40a1216d4192baeda9352384444410) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_gsocr ) /* Great Soccer. (Bad Dump) */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mp10747f.ic1", 0x000000, 0x020000, BAD_DUMP CRC(9cf53703) SHA1(c6b4d1de56bd5bf067ec7fc80449c07686d01337) ) /* first 32kb is repeated 4 times, doesn't work in MEKA*/

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* z80 */

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "12368-05.ic2", 0x000000, 0x08000, CRC(bab91fcc) SHA1(a160c9d34b253e93ac54fdcef33f95f44d8fa90c) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_parlg ) /* Parlour Games.  */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mp11404.ic1", 0x000000, 0x020000, CRC(E030E66C) SHA1(06664DAF208F07CB00B603B12ECCFC3F01213A17) )
	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* z80 */

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "12368-29.ic2", 0x000000, 0x08000, CRC(534151e8) SHA1(219238d90c1d3ac07ff64c9a2098b490fff68f04) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_shnbi ) /* Shinobi. */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mp11706.ic1", 0x000000, 0x040000, CRC(0C6FAC4E) SHA1(7C0778C055DC9C2B0AAE1D166DBDB4734E55B9D1) )
	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* z80 */

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "12368-08.ic2", 0x000000, 0x08000, CRC(103A0459) SHA1(D803DDF7926B83785E8503C985B8C78E7CCB5DAC) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_revsh ) /* The Revenge Of Shinobi. */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mp12675.ic1", 0x000000, 0x080000, CRC(672A1D4D) SHA1(5FD0AF14C8F2CF8CEAB1AE61A5A19276D861289A) )
	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* z80 */

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "12368-28.ic2", 0x000000, 0x08000, CRC(0D30BEDE) SHA1(73A090D84B78A570E02FB54A33666DCADA52849B) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_aftrb ) /* Afterburner. */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mp11271.ic1", 0x000000, 0x080000, CRC(1C951F8E) SHA1(51531DF038783C84640A0CAB93122E0B59E3B69A) )
	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* z80 */

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "12368-10.ic2", 0x000000, 0x08000, CRC(2A7CB590) SHA1(2236963BDDC89CA9045B530259CC7B5CCF889EAF) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END

ROM_START( mt_tgolf ) /* Arnold Palmer Tournament Golf */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mp12645f.ic1", 0x000000, 0x080000, CRC(c07ef8d2) SHA1(9d111fdc7bb92d52bfa048cd134aa488b4f475ef) )
	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* z80 */

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "12368-31.ic2", 0x000000, 0x08000, CRC(30af7e4a) SHA1(baf91d527393dc90aba9371abcb1e690bcc83c7e) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Bios */
	ROM_LOAD( "epr12664.20", 0x000000, 0x8000, CRC(f71e9526) SHA1(1c7887541d02c41426992d17f8e3db9e03975953) )
ROM_END


/* MegaPlay Games - Modified Genesis games */

SYSTEM_BIOS_START( megaplay )
	SYSTEM_BIOS_ADD( 0, "ver1",       "Megaplay Bios (Ver. 1)" )
	SYSTEM_BIOS_ADD( 1, "ver2",       "Megaplay Bios (Ver. 2)" ) /* this one doesn't boot .. is it ok?*/
SYSTEM_BIOS_END

#define ROM_LOAD_BIOS(bios,name,offset,length,hash) \
		ROMX_LOAD(name, offset, length, hash, ROM_BIOS(bios+1)) /* Note '+1' */

#define MEGAPLAY_BIOS \
	ROM_LOAD_BIOS( 0, "ep15294.ic2", 0x000000, 0x20000, CRC(aa8dc2d8) SHA1(96771ad7b79dc9c83a1594243250d65052d23176) ) \
	ROM_LOAD_BIOS( 1, "megaplay.bin",0x000000, 0x20000, CRC(f97c68aa) SHA1(bcabc879950bca1ced11c550a484e697ec5706b2) ) \

ROM_START( megaplay )
	ROM_REGION( 0x20000, REGION_USER1, 0 )
	MEGAPLAY_BIOS
ROM_END

ROM_START( mp_bio ) /* Bio Hazard Battle */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mpr-15699-f.ic1", 0x000000, 0x100000, CRC(4b193229) SHA1(f8629171ae9b4792f142f6957547d886e5cc6817) )
	/* Game Instruction rom copied to 0x300000 - 0x310000 (odd / even bytes equal) */

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* z80 */

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "epr-15175-06.ic2", 0x000000, 0x08000, CRC(1ef64e41) SHA1(13984b714b014ea41963b70de74a5358ed223bc5) )

	ROM_REGION( 0x28000, REGION_CPU3, 0 ) /* Bios */
	MEGAPLAY_BIOS
ROM_END

ROM_START( mp_col3 ) /* Columns 3 */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "3.ic2", 0x000000, 0x040000, CRC(a1602235) SHA1(38751b585849c8966acc3f508714937fe29dcf5c) )
	ROM_LOAD16_BYTE( "2.ic1", 0x000001, 0x040000, CRC(999b2fe6) SHA1(ad967a28e4eebd7b01273e4e04c35a0198ef834a) )
	/* Game Instruction rom copied to 0x300000 - 0x310000 (odd / even bytes equal) */

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* z80 */

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "1.ic3", 0x000000, 0x08000,  CRC(dac9bf91) SHA1(0117972a7181f8aaf942a259cc8764b821031253) )

	ROM_REGION( 0x28000,  REGION_CPU3, 0 ) /* Bios */
	MEGAPLAY_BIOS
ROM_END


ROM_START( mp_gaxe2 ) /* Golden Axe 2 */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "ep15179b.ic2", 0x000000, 0x040000, CRC(00d97b84) SHA1(914bbf566ddf940aab67b92af237d251650ddadf) )
	ROM_LOAD16_BYTE( "ep15178b.ic1", 0x000001, 0x040000, CRC(2ea576db) SHA1(6d96b948243533de1f488b1f80e0d5431a4f1f53) )
	/* Game Instruction rom copied to 0x300000 - 0x310000 (odd / even bytes equal) */

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* z80 */

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "ep15175-02b.ic3", 0x000000, 0x08000, CRC(3039b653) SHA1(b19874c74d0fc0cca1169f62e5e74f0e8ca83679) ) /* 15175-02b.ic3 */

	ROM_REGION( 0x28000, REGION_CPU3, 0 ) /* Bios */
	MEGAPLAY_BIOS
ROM_END


ROM_START( mp_shnb3 ) /* Shinobi 3 */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mpr-16197.ic1", 0x000000, 0x100000, CRC(48162361) SHA1(77d544509339b5ddf6d19941377e81d29e9e21dc) )
	/* Game Instruction rom copied to 0x300000 - 0x310000 (odd / even bytes equal) */

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* z80 */

	ROM_REGION( 0x8000, REGION_USER1, 0 )/* Game Instructions */
	ROM_LOAD( "epr-15175-09.ic2", 0x000000, 0x08000, CRC(6254e45a) SHA1(8667922a6eade03c964ce224f7fa39ba871c60a4) )

	ROM_REGION( 0x28000,  REGION_CPU3, 0 )/* Bios */
	MEGAPLAY_BIOS
ROM_END

ROM_START( mp_sonic ) /* Sonic */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "ep15177.ic2", 0x000000, 0x040000, CRC(a389b03b) SHA1(8e9e1cf3dd65ddf08757f5a1ce472130c902ea2c) )
	ROM_LOAD16_BYTE( "ep15176.ic1", 0x000001, 0x040000, CRC(d180cc21) SHA1(62805cfaaa80c1da6146dd89fc2b49d819fd4f22) )
	/* Game Instruction rom copied to 0x300000 - 0x310000 (odd / even bytes equal) */

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* z80 */

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "ep15175-01.ic3", 0x000000, 0x08000, CRC(99246889) SHA1(184aa3b7fdedcf578c5e34edb7ed44f57f832258) )

	ROM_REGION( 0x28000, REGION_CPU3, 0 ) /* Bios */
	MEGAPLAY_BIOS
ROM_END

ROM_START( mp_soni2 ) /* Sonic The Hedgehog 2 */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mpr-16011.ic1", 0x000000, 0x100000, CRC(3d7bf98a) SHA1(dce0e4e8f2573e0ffe851edaa235e4ed9e61ee2d) )
	/* Game Instruction rom copied to 0x300000 - 0x310000 (odd / even bytes equal) */

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* z80 */

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "epr-15175-07.ic1", 0x000000, 0x08000, CRC(bb5f67f0) SHA1(33b7a5d14015a5fcf41976a8f648f8f48ce9bb03) )

	ROM_REGION( 0x28000, REGION_CPU3, 0 ) /* Bios */
	MEGAPLAY_BIOS
ROM_END


ROM_START( mp_sor2 ) /* Streets of Rage 2 */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mpr-15425.ic1", 0x000000, 0x200000, CRC(cd6376af) SHA1(57ec210975e40505649f152b60ef54f99da31f0e) )
	/* Game Instruction rom copied to 0x300000 - 0x310000 (odd / even bytes equal) */

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* z80 */

	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "epr-15175-05.ic2", 0x000000, 0x08000, CRC(1df5347c) SHA1(faced2e875e1914392f61577b5256d006eebeef9) )

	ROM_REGION( 0x28000, REGION_CPU3, 0 ) /* Bios */
	MEGAPLAY_BIOS
ROM_END

ROM_START( mp_twc ) /* Tecmo World Cup */
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "ep15183.ic2", 0x000000, 0x040000, CRC(8b79b861) SHA1(c72af72840513b82f2562409eccdf13b031bf3c0) )
	ROM_LOAD16_BYTE( "ep15182.ic1", 0x000001, 0x040000, CRC(eb8325c3) SHA1(bb21ac926c353e14184dd476222bc6a8714606e5) )
	/* Game Instruction rom copied to 0x300000 - 0x310000 (odd / even bytes equal) */

 	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* z80 */

 	ROM_REGION( 0x8000, REGION_USER1, 0 ) /* Game Instructions */
	ROM_LOAD( "ep15175-04.ic3", 0x000000, 0x08000, CRC(faf7c030) SHA1(16ef405335b4d3ecb0b7d97b088dafc4278d4726) )

 	ROM_REGION( 0x28000, REGION_CPU3, 0 ) /* Bios */
 	MEGAPLAY_BIOS
ROM_END

/******************************************************************************
	Machine Init Functions
*******************************************************************************

	All of the Sega C/C2 games apart from Bloxeed used a protection chip.
	The games contain various checks which make sure this protection chip is
	present and returning the expected values.  The chip uses a table of
	256x4-bit values to produce its results.  It appears that different
	tables are used for Japanese vs. English variants of some games
	(Puzzle & Action 2) but not others (Columns).

******************************************************************************/

static void init_saves(void)
{
	/* Do we need the int states ? */
	state_save_register_UINT8 ("C2_main", 0, "Int 2 Status", &ym3438_int, 1);
	state_save_register_UINT8 ("C2_main", 0, "Int 4 Status", &scanline_int, 1);
	state_save_register_UINT8 ("C2_main", 0, "Int 6 Status", &vblank_int, 1);

	state_save_register_UINT8 ("C2_IO", 0, "I/O Writes", iochip_reg, 0x10);

	state_save_register_UINT16 ("C2 Protection", 0, "Write Buffer", &prot_write_buf, 1);
	state_save_register_UINT16 ("C2 Protection", 0, "Read Buffer", &prot_read_buf, 1);
}

static DRIVER_INIT( segac2 )
{
	bloxeed_sound = 0;
	init_saves();
}

static DRIVER_INIT( bloxeedc )
{
	init_saves();
	bloxeed_sound = 1;
}

static DRIVER_INIT( columns )
{
	static const UINT32 columns_table[256/8] =
	{
		0x20a41397, 0x64e057d3, 0x20a41397, 0x64e057d3,
		0x20a41397, 0x64e057d3, 0xa8249b17, 0xec60df53,
		0x20a41397, 0x64e057d3, 0x75f546c6, 0x31b10282,
		0x20a41397, 0x64e057d3, 0xfd75ce46, 0xb9318a02,
		0xb8348b07, 0xfc70cf43, 0xb8348b07, 0xfc70cf43,
		0x9a168b07, 0xde52cf43, 0x9a168b07, 0xde52cf43,
		0x30b40387, 0x74f047c3, 0x75f546c6, 0x31b10282,
		0x30b40387, 0x74f047c3, 0xfd75ce46, 0xb9318a02
	};
	prot_table = columns_table;
	bloxeed_sound = 0;
	init_saves();
}

static DRIVER_INIT( columns2 )
{
	static const UINT32 columns2_table[256/8] =
	{
		0x0015110c, 0x0015110c, 0x889d9984, 0xcedb9b86,
		0x4455554c, 0x4455554c, 0xddddccc4, 0x9b9bcec6,
		0x2237332e, 0x2237332e, 0x6677776e, 0x2031756c,
		0x6677776e, 0x6677776e, 0x7777666e, 0x3131646c,
		0x0015110c, 0x0015110c, 0x889d9984, 0xcedb9b86,
		0x6677776e, 0x6677776e, 0xffffeee6, 0xb9b9ece4,
		0xaabfbba6, 0xaabfbba6, 0xeeffffe6, 0xa8b9fde4,
		0xeeffffe6, 0xeeffffe6, 0xffffeee6, 0xb9b9ece4
	};
	prot_table = columns2_table;
	bloxeed_sound = 0;
	init_saves();
}

static DRIVER_INIT( borench )
{
	static const UINT32 borench_table[256/8] =
	{
		0x12fe56ba, 0x56ba56ba, 0x00aa44ee, 0xcceeccee,
		0x13ff57bb, 0x759957bb, 0x11bb55ff, 0xffddddff,
		0x12ba56fe, 0x56fe56fe, 0x00aa44ee, 0xcceeccee,
		0x933bd77f, 0xf55dd77f, 0x913bd57f, 0x7f5d5d7f,
		0x12fe56ba, 0x56ba56ab, 0x00aa44ee, 0xcceeccff,
		0xd73bd73b, 0xf519d72a, 0xd57fd57f, 0x7f5d5d6e,
		0x12ba56fe, 0x56fe56ef, 0x00aa44ee, 0xcceeccff,
		0xd77fd77f, 0xf55dd76e, 0xd57fd57f, 0x7f5d5d6e
	};
	prot_table = borench_table;
	bloxeed_sound = 0;
	init_saves();
}

static DRIVER_INIT( tfrceac )
{
	static const UINT32 tfrceac_table[256/8] =
	{
		0x3a3a6f6f, 0x38386d6d, 0x3a3a6f6f, 0x28287d7d,
		0x3a3a6f6f, 0x38386d6d, 0x3a3a6f6f, 0x28287d7d,
		0x7e3a2b6f, 0x7c38296d, 0x7eb22be7, 0x6ca039f5,
		0x7e3a2b6f, 0x7c38296d, 0x7eb22be7, 0x6ca039f5,
		0x3b3b6e6e, 0x39396c6c, 0x5dd50880, 0x4ec61b93,
		0x3b3b6e6e, 0x39396c6c, 0x3bb36ee6, 0x28a07df5,
		0x5d19084c, 0x5d19084c, 0x7ff72aa2, 0x6ee63bb3,
		0x5d19084c, 0x5d19084c, 0x5d9108c4, 0x4c8019d5
	};
	prot_table = tfrceac_table;
	bloxeed_sound = 0;
	init_saves();
}

static DRIVER_INIT( tfrceacb )
{
	/* disable the palette bank switching from the protection chip */
	install_mem_write16_handler(0, 0x800000, 0x800001, MWA16_NOP);
}

static DRIVER_INIT( ribbit )
{
	static const UINT32 ribbit_table[256/8] =
	{
		0xffeeddcc, 0xffeeddcc, 0xfeeffeef, 0xfeeffeef,
		0xbb8899aa, 0xffccddee, 0xba89ba89, 0xfecdfecd,
		0x7f6e5d4c, 0x3b2a1908, 0x7e6f7e6f, 0x3a2b3a2b,
		0x3b19193b, 0x3b19193b, 0xba98ba98, 0xba98ba98,
		0xffff5555, 0xffff5555, 0xfefe7676, 0xfefe7676,
		0xbb991133, 0xffdd5577, 0xfedc7654, 0xfedc7654,
		0x7f7ff7f7, 0x3b3bb3b3, 0x7e7ef6f6, 0x3a3ab2b2,
		0x3b19b391, 0x3b19b391, 0xfedc7654, 0xba983210
	};
	prot_table = ribbit_table;
	bloxeed_sound = 0;
	init_saves();

	/* kludge for protection */
	install_mem_read16_handler(0, 0xffc166, 0xffc167, ribbit_prot_hack_r);

	/* additional palette swizzling */
	install_mem_read16_handler(0, 0x8c0000, 0x8c0fff, ribbit_palette_r);
	install_mem_write16_handler(0, 0x8c0000, 0x8c0fff, ribbit_palette_w);
	ribbit_palette_select = install_mem_read16_handler(0, 0x2000, 0x27ff, ribbit_palette_select_r);
}

static DRIVER_INIT( tantr )
{
	static const UINT32 tantr_table[256/8] =
	{
		0x91ddd19d, 0x91ddd19d, 0xd4dc949c, 0xf6feb6be,
		0x91bbd1fb, 0x91bbd1fb, 0xd4fe94be, 0xf6feb6be,
		0x80cce2ae, 0x88cceaae, 0xc5cda7af, 0xefef8d8d,
		0x91bbf3d9, 0x99bbfbd9, 0xd4feb69c, 0xfefe9c9c,
		0x5d55959d, 0x5d55959d, 0x5c54949c, 0x7e76b6be,
		0x5d7795bf, 0x5d7795bf, 0x5c7694be, 0x7e76b6be,
		0x5d55b7bf, 0x4444aeae, 0x5c54b6be, 0x67678d8d,
		0x5d77b79d, 0x5577bf9d, 0x5c76b69c, 0x76769c9c
	};
	prot_table = tantr_table;
	bloxeed_sound = 0;
	init_saves();
}

static DRIVER_INIT( tantrkor )
{

	static const UINT32 tantrkor_table[256/8] =
	{
		0x80931102, 0xc4d75546, 0xd5825502, 0x91c61146,
		0x081b998a, 0x4c5fddce, 0x5d0add8a, 0x194e99ce,
		0xc4d77764, 0xc4d77764, 0x91c63364, 0x91c63364,
		0xc4d77764, 0xc4d77764, 0x91c63364, 0x91c63364,
		0x91930002, 0xd5d74446, 0xc4824402, 0x80c60046,
		0x191b888a, 0x5d5fccce, 0x4c0acc8a, 0x084e88ce,
		0xd5d76664, 0xd5d76664, 0x80c62264, 0x80c62264,
		0xd5d76664, 0xd5d76664, 0x80c62264, 0x80c62264
	};

#if 0
	{
		int base = 0x5ce;
		data8_t *rom	=	memory_region(REGION_CPU1);
		int a,b;

		for (a=0;a<256;a+=8) {
			printf("\n0x");
			for(b=0;b<8;b++) {
				int x;
				x = rom[(base+a+b)^1]&0xf;
				printf("%1x",x);
			}
		}
	}
#endif

	prot_table = tantrkor_table;
	bloxeed_sound = 0;
	init_saves();
}

static DRIVER_INIT( ichir )
{
	static const UINT32 ichir_table[256/8] =
	{
		0x4c4c4c4c, 0x08080808, 0x5d5d4c4c, 0x19190808,
		0x33332222, 0x33332222, 0x22222222, 0x22222222,
		0x082a082a, 0x082a082a, 0x193b082a, 0x193b082a,
		0x77556644, 0x33112200, 0x66446644, 0x22002200,
		0x6e6e6e6e, 0x2a2a2a2a, 0x7f7f6e6e, 0x3b3b2a2a,
		0xbbbbaaaa, 0xbbbbaaaa, 0xaaaaaaaa, 0xaaaaaaaa,
		0x2a082a08, 0x2a082a08, 0x3b192a08, 0x3b192a08,
		0xffddeecc, 0xbb99aa88, 0xeecceecc, 0xaa88aa88
	};
	prot_table = ichir_table;
	bloxeed_sound = 0;
	init_saves();
}

static DRIVER_INIT( ichirk )
{
	static const UINT32 ichirk_table[256/8] =
	{
		0x44004400, 0x00440044,	0x55114400, 0x11550044,
		0x55885588, 0x55885588,	0x66bb77aa, 0x66bb77aa,
		0x02020202, 0x46464646,	0x13130202, 0x57574646,
		0x138a138a, 0x138a138a,	0x20b931a8, 0x20b931a8,
		0x44004400, 0x00440044,	0x55114400, 0x11550044,
		0x55885588, 0x55885588,	0x66bb77aa, 0x66bb77aa,
		0x8a8a8a8a, 0xcececece,	0x9b9b8a8a, 0xdfdfcece,
		0x9b029b02, 0x9b029b02,	0xa831b920, 0xa831b920
	};
	prot_table = ichirk_table;
	bloxeed_sound = 0;
	init_saves();
}

static DRIVER_INIT( ichirj )
{
	/* 317-0224 */
	static const UINT32 ichirj_table[256/8] =
	{
		0x55116622, 0x55116622, 0x55117733, 0x55117733,
		0x8800aa22, 0x8800aa22, 0x8800bb33, 0x8800bb33,
		0x11550044, 0x55114400, 0x11551155, 0x55115511,
		0xcc44cc44, 0x88008800, 0xcc44dd55, 0x88009911,
		0xdd99eeaa, 0xdd99eeaa, 0xdd99ffbb, 0xdd99ffbb,
		0xaa228800, 0xaa228800, 0xaa229911, 0xaa229911,
		0x99dd88cc, 0xdd99cc88, 0x99dd99dd, 0xdd99dd99,
		0xee66ee66, 0xaa22aa22, 0xee66ff77, 0xaa22bb33
	};
	prot_table = ichirj_table;
	bloxeed_sound = 0;
	init_saves();
}

static DRIVER_INIT( ichirjbl )
{
	/* when did this actually work? - the protection is patched but the new check fails? */
	UINT16 *rom = (UINT16 *)memory_region(REGION_CPU1);
	rom[0x390/2] = 0x6600;
}


static DRIVER_INIT( potopoto )
{
	/* note: this is not the real table; Poto Poto only tests one  */
	/* very specific case, so we don't have enough data to provide */
	/* the correct table in its entirety */
	static const UINT32 potopoto_table[256/8] =
	{
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x22222222, 0x22222222,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000
	};
	prot_table = potopoto_table;
	bloxeed_sound = 0;
	init_saves();
}

static DRIVER_INIT( puyopuyo )
{
	static const UINT32 puyopuyo_table[256/8] =
	{
		0x33aa55cc, 0x33aa55cc, 0xba22fe66, 0xba22fe66,
		0x77ee55cc, 0x55cc77ee, 0xfe66fe66, 0xdc44dc44,
		0x33aa77ee, 0x77aa33ee, 0xba22fe66, 0xfe22ba66,
		0x77ee77ee, 0x11cc11cc, 0xfe66fe66, 0x98449844,
		0x22bb44dd, 0x3ba25dc4, 0xab33ef77, 0xba22fe66,
		0x66ff44dd, 0x5dc47fe6, 0xef77ef77, 0xdc44dc44,
		0x22bb66ff, 0x7fa23be6, 0xab33ef77, 0xfe22ba66,
		0x66ff66ff, 0x19c419c4, 0xef77ef77, 0x98449844
	};
	prot_table = puyopuyo_table;
	bloxeed_sound = 0;
	init_saves();
}

static DRIVER_INIT( puyopuy2 )
{
	/* note: this is not the real table; Puyo Puyo 2 doesn't  */
	/* store the original table; instead it loops through all */
	/* combinations 0-255 and expects the following results;  */
	/* to work around this, we install a custom read handler  */
	static const UINT32 puyopuy2_table[256/8] =
	{
		0x00008383, 0xb3b33030, 0xcccc4f4f, 0x7f7ffcfc,
		0x02028181, 0xb1b13232, 0xcece4d4d, 0x7d7dfefe,
		0x4444c1c1, 0x91911414, 0x99991c1c, 0x4c4cc9c9,
		0x4646c3c3, 0x93931616, 0x9b9b1e1e, 0x4e4ecbcb,
		0x5555d7d7, 0xf7f77575, 0xdddd5f5f, 0x7f7ffdfd,
		0x5757d5d5, 0xf5f57777, 0xdfdf5d5d, 0x7d7dffff,
		0x11119595, 0xd5d55151, 0x88880c0c, 0x4c4cc8c8,
		0x13139797, 0xd7d75353, 0x8a8a0e0e, 0x4e4ecaca
	};
	prot_table = puyopuy2_table;
	bloxeed_sound = 0;

	install_mem_read16_handler(0, 0x800000, 0x800001, puyopuy2_prot_r);
	init_saves();
}

static DRIVER_INIT( stkclmns )
{
	static const UINT32 stkclmns_table[256/8] =
	{
		0xcc88cc88, 0xcc88cc88, 0xcc99cc99, 0xcc99cc99,
		0x00001111, 0x88889999, 0x00111100, 0x88999988,
		0xaaee88cc, 0xeeaacc88, 0xaaff88dd, 0xeebbcc99,
		0x66665555, 0xaaaa9999, 0x66775544, 0xaabb9988,
		0xeeaaeeaa, 0xeeaaeeaa, 0xeebbeebb, 0xeebbeebb,
		0x00001111, 0x88889999, 0x00111100, 0x88999988,
		0x00442266, 0x44006622, 0x00552277, 0x44116633,
		0xeeeedddd, 0x22221111, 0xeeffddcc, 0x22331100
	};
	prot_table = stkclmns_table;
	bloxeed_sound = 0;

	/* until the battery RAM is understood, we must fill RAM with */
	/* random values so that the high scores are properly reset at */
	/* startup */
	{
		int i;
		for (i = 0; i < 0x10000/2; i++)
			main_ram[i] = mame_rand();
	}
	init_saves();
}

static DRIVER_INIT( zunkyou )
{
	static const UINT32 zunkyou_table[256/8] =
	{
		0xa0a06c6c, 0x82820a0a, 0xecec2020, 0xecec6464,
		0xa2a26e6e, 0x80800808, 0xaaaa6666, 0xaaaa2222,
		0x39287d6c, 0x1b0a1b0a, 0x75643120, 0x75647564,
		0x3b2a7f6e, 0x19081908, 0x33227766, 0x33223322,
		0xb1b17d7d, 0x93931b1b, 0xfdfd3131, 0xfdfd7575,
		0xa2a26e6e, 0x80800808, 0xaaaa6666, 0xaaaa2222,
		0x28396c7d, 0x0a1b0a1b, 0x64752031, 0x64756475,
		0x3b2a7f6e, 0x19081908, 0x33227766, 0x33223322
	};
	prot_table = zunkyou_table;
	bloxeed_sound = 0;
	init_saves();
}

/* Print Club hardware (C2 with a printer) */

static int cam_data;

static READ16_HANDLER( printer_r )
{
	return cam_data;
}

static WRITE16_HANDLER( print_club_camera_w )
{
	cam_data = data;
}

static DRIVER_INIT( pclub )
{
	static const UINT32 printc1_table[256/8] =
	{
		0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
		0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
		0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
		0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
		0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
		0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
		0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
		0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff
	};
	prot_table = printc1_table;
	bloxeed_sound = 0;
	init_saves();

	install_mem_read16_handler(0,  0x880120, 0x880121, printer_r );/*Print Club Vol.1*/
	install_mem_read16_handler(0,  0x880124, 0x880125, printer_r );/*Print Club Vol.2*/
	install_mem_write16_handler(0, 0x880124, 0x880125, print_club_camera_w);
}

/* Genie's Hardware (contains no real sega parts) */

DRIVER_INIT( puckpkmn )
{
	data8_t *rom	=	memory_region(REGION_CPU1);
	size_t len		=	memory_region_length(REGION_CPU1);
	int i;
	for (i = 0; i < len; i++)
		rom[i] = BITSWAP8(rom[i],1,4,2,0,7,5,3,6);

	cpu_setbank(1, memory_region(REGION_CPU1) );	/* VDP reads the roms from here*/
	cpu_setbank(2, main_ram );						/* VDP reads the ram from here*/

	init_segac2();
}

/* sbubsm */
static READ16_HANDLER(sbubsm_200051_r)
{
	return -0x5b;
}

static READ16_HANDLER(sbubsm_400000_r)
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "%s: sbubsm_400000_r\n");
	return 0x5500;
}

static READ16_HANDLER(sbubsm_400002_r)
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "%s: sbubsm_400002_r\n");
	return 0x0f00;
}

DRIVER_INIT( sbubsm )
{
	/* needed to boot, somme kind of hardware ident? */
	install_mem_read16_handler(0, 0x400000, 0x400001, sbubsm_400000_r);
  install_mem_read16_handler(0, 0x400002, 0x400003, sbubsm_400002_r );

	install_mem_read16_handler(0, 0x200050, 0x200051, sbubsm_200051_r );
	install_mem_read16_handler(0, 0x200042, 0x200043, input_port_0_word_r );
	install_mem_read16_handler(0, 0x200044, 0x200045, input_port_1_word_r );
	install_mem_read16_handler(0, 0x200046, 0x200047, input_port_2_word_r );
	install_mem_read16_handler(0, 0x200048, 0x200049, input_port_3_word_r );
	install_mem_read16_handler(0, 0x20007e, 0x20007f, input_port_4_word_r );

	cpu_setbank(3, memory_region(REGION_CPU1) );
	cpu_setbank(4, &genesis_68k_ram[0]);

	init_segac2();
}

WRITE16_HANDLER( aladbl_w )
{
    /*
    Values returned from the log file :
      - aladbl_w : 1b2a6c - data = 6600 (each time a coin is inserted)
      - aladbl_w : 1b2a82 - data = 0000 (each time a coin is inserted)
      - aladbl_w : 1b2d18 - data = aa00 (only once on reset)
      - aladbl_w : 1b2d42 - data = 0000 (only once on reset)
    */
	log_cb(RETRO_LOG_DEBUG, LOGPRE "aladbl_w : %06x - data = %04x\n",activecpu_get_pc(),data);
}


READ16_HANDLER( aladbl_r )
{
	if (activecpu_get_pc()==0x1b2a56)
	{
		static UINT16 mcu_port;

		mcu_port = readinputport(3);

		if(mcu_port & 0x100)
			return ((mcu_port & 0x0f) | 0x100); /* coin inserted, calculate the number of coins */
		else
			return (0x100); /* MCU status, needed if you fall into a pitfall */
	}
	if (activecpu_get_pc()==0x1b2a72) return 0x0000;
	if (activecpu_get_pc()==0x1b2d24) return (readinputport(3) & 0x00f0) | 0x1200;    /* difficulty */
	if (activecpu_get_pc()==0x1b2d4e) return 0x0000;

	log_cb(RETRO_LOG_DEBUG, LOGPRE "aladbl_r : %06x\n",activecpu_get_pc());

	return 0x0000;
}

#define ENERGY_CONSOLE_MODE 1

DRIVER_INIT( aladbl )
{
/*
Game does a check @ 1afc00 with work ram fff57c that makes it play like it was intended
(i.e. 8 energy hits instead of 2) enabled for MAME2003 as it makes the game sweeter to play.
*/
	#if ENERGY_CONSOLE_MODE
	UINT16 *rom = (UINT16 *)memory_region(REGION_CPU1);
	rom[0x1afc08/2] = 0x6600;
	#endif

	/* 220000 = writes to mcu? 330000 = reads? */
	install_mem_write16_handler(0, 0x220000, 0x220001, aladbl_w);
	install_mem_read16_handler(0, 0x330000, 0x330001, aladbl_r);

	genesis_region = 0x00; /* read via io */

	init_segac2();
}

DRIVER_INIT( barek2ch )
{
	UINT16 *src = (UINT16 *)memory_region(REGION_CPU1);
	int i;

	for (i = 0x000000; i < 0x200000 / 2; i++)
		src[i] = BITSWAP16(src[i], 8, 11, 10, 13, 12, 14, 15, 9, 7, 6, 5, 4, 3, 2, 1, 0);

	src[0x06 / 2] = 0x0210; /* TODO: why is this needed? */

	install_mem_read16_handler(0, 0x380070, 0x380071, input_port_2_word_r );
	install_mem_read16_handler(0, 0x380078, 0x380079, input_port_3_word_r );
	install_mem_read16_handler(0, 0x38007a, 0x38007b, input_port_4_word_r );

	genesis_region = 0x00; /* read via io */

	cpu_setbank(3, memory_region(REGION_CPU1) );
	cpu_setbank(4, &genesis_68k_ram[0]);

	init_segac2();
}

DRIVER_INIT( barek3 )
{
	data8_t *rom	=	memory_region(REGION_CPU1);
  int x;

	for (x = 0x00001; x < 0x300000; x += 2)
	{
		rom[x] = BITSWAP8(rom[x], 6,2,4,0,7,1,3,5);
	}

	install_mem_read16_handler(0, 0x380070, 0x380071, input_port_2_word_r );
	install_mem_read16_handler(0, 0x380078, 0x380079, input_port_3_word_r );

	genesis_region = 0x00; /* read via io */

	init_segac2();
}

DRIVER_INIT( sonic2mb )
{
   /* 100000 = writes to unpopulated MCU? */
	install_mem_write16_handler(0, 0x100000, 0x100001, aladbl_w);
    install_mem_read16_handler(0, 0x300000, 0x300001, input_port_3_word_r);
	
	genesis_region = 0x00; /* read via io */
		
	cpu_setbank(3, memory_region(REGION_CPU1) );
	cpu_setbank(4, &genesis_68k_ram[0]);

	init_segac2();
}

/******************************************************************************
	Game Drivers
*******************************************************************************

	These cover all the above games.

	Dates are all verified correct from Ingame display, some of the Titles
	such as Ichidant-R, Tant-R might be slightly incorrect as I've seen the
	games refered to by other names such as Ichident-R, Tanto-R, Tanto Arle
	etc.

	bloxeedc is set as as clone of bloxeed as it is the same game but running
	on a different piece of hardware.  The parent 'bloxeed' is a system18 game
	and does not currently work due to it being encrypted.

******************************************************************************/

/* System C Games */
GAME ( 1989, bloxeedc, bloxeed,  segac,    bloxeedc, bloxeedc, ROT0, "Sega / Elorg",           "Bloxeed (C System)" )
GAME ( 1990, columns,  0,        segac,    columns,  columns,  ROT0, "Sega",                   "Columns (US)" )
GAME ( 1990, columnsj, columns,  segac,    columns,  columns,  ROT0, "Sega",                   "Columns (Japan)" )
GAME ( 1990, columns2, 0,        segac,    columns2, columns2, ROT0, "Sega",                   "Columns II - The Voyage Through Time (Japan)" )

/* System C-2 Games */
GAME ( 1990, borench,  0,        segac2,   borench,  borench,  ROT0, "Sega",                   "Borench" )
GAME ( 1990, tfrceac,  0,        segac2,   tfrceac,  tfrceac,  ROT0, "Sega / Technosoft",      "ThunderForce AC" )
GAME ( 1990, tfrceacj, tfrceac,  segac2,   tfrceac,  tfrceac,  ROT0, "Sega / Technosoft",      "ThunderForce AC (Japan)" )
GAME ( 1990, tfrceacb, tfrceac,  segac2,   tfrceac,  tfrceacb, ROT0, "bootleg",                "ThunderForce AC (bootleg)" )
GAME ( 1991, ribbit,   0,        segac2,   ribbit,   ribbit,   ROT0, "Sega",                   "Ribbit!" )
GAME ( 1992, ooparts,  0,        ooparts,  ooparts,  segac2,   ROT270, "Sega / Success",       "OOPArts (Japan, Prototype)" )
GAME ( 1992, ssonicbr, 0,        segac2,   ssonicbr, bloxeedc, ROT0, "Sega",                   "SegaSonic Bros (Japan, prototype)" )
GAME ( 1992, tantr,    0,        segac2,   ichidant, tantr,    ROT0, "Sega",                   "Puzzle & Action: Tant-R (Japan)" )
GAME ( 1993, tantrkor, tantr,    segac2,   ichidant, tantrkor, ROT0, "Sega",                   "Puzzle & Action: Tant-R (Korea)" )
GAME ( 1992, tantrbl,  tantr,    segac2,   ichidant, segac2,   ROT0, "bootleg",                "Puzzle & Action: Tant-R (Japan) (bootleg set 1)" )
GAME ( 1994, tantrbl2, tantr,    segac,    ichidant, tantr,    ROT0, "bootleg",                "Puzzle & Action: Tant-R (Japan) (bootleg set 2)" )
GAME ( 1992, puyopuyo, 0,        segac2,   puyopuyo, puyopuyo, ROT0, "Sega / Compile",         "Puyo Puyo (Japan)" )
GAME ( 1992, puyopuya, puyopuyo, segac2,   puyopuyo, puyopuyo, ROT0, "Sega / Compile",         "Puyo Puyo (Japan) (Rev A)" )
GAME ( 1992, puyopuyb, puyopuyo, segac2,   puyopuyo, puyopuyo, ROT0, "bootleg",                "Puyo Puyo (English) (bootleg)" )
GAME ( 1994, ichir,    0,        segac2,   ichidant, ichir,    ROT0, "Sega",                   "Puzzle & Action: Ichidant-R (World)" )
GAME ( 1994, ichirk,   ichir,    segac2,   ichidant, ichirk,   ROT0, "Sega",                   "Puzzle & Action: Ichidant-R (Korea)" )
GAME ( 1994, ichirj,   ichir,    segac2,   ichidant, ichirj,   ROT0, "Sega",                   "Puzzle & Action: Ichidant-R (Japan)" )
GAME ( 1994, ichirjbl, ichir,    segac,    ichidant, ichirjbl, ROT0, "bootleg",                "Puzzle & Action: Ichidant-R (Japan) (bootleg)" )
GAME ( 1994, stkclmns, 0,        segac2,   stkclmns, stkclmns, ROT0, "Sega",                   "Stack Columns (Japan)" )
GAME ( 1994, puyopuy2, 0,        segac2,   puyopuy2, puyopuy2, ROT0, "Compile (Sega license)", "Puyo Puyo 2 (Japan)" )
GAME ( 1994, potopoto, 0,        segac2,   potopoto, potopoto, ROT0, "Sega",                   "Poto Poto (Japan)" )
GAME ( 1994, zunkyou,  0,        segac2,   zunkyou,  zunkyou,  ROT0, "Sega",                   "Zunzunkyou No Yabou (Japan)" )
GAME ( 1994, headonch, 0,        segac2,   headonch, segac2,   ROT0, "Sega",                   "Head On Channel (Japan, prototype)" )

/* Genie Hardware (uses Genesis VDP) also has 'Sun Mixing Co' put into tile ram */
GAME ( 2000, puckpkmn, 0,        puckpkmn, puckpkmn, puckpkmn, ROT0, "Genie",                  "Puckman Pockimon" )
GAMEX( 2000, jzth,     0,        jzth,     jzth,     puckpkmn, ROT0, "<unknown>",              "Juezhan Tianhuang", GAME_IMPERFECT_SOUND )

/* Bootlegs Using Genesis Hardware */
GAME ( 1993, aladmdb,  0,        barek3,   aladbl,   aladbl,   ROT0, "bootleg / Sega",         "Aladdin (bootleg of Japanese Megadrive version)" )
GAME ( 1993, sonic2mb, 0,        barek2ch, sonic2mb, sonic2mb, ROT0, "bootleg / Sega",         "Sonic The Hedgehog 2 (bootleg of Megadrive version)" )
GAME ( 1994, barek2ch, 0,        barek2ch, barek2ch, barek2ch, ROT0, "bootleg / Sega",         "Bare Knuckle II (Chinese bootleg of Megadrive version)" )
GAME ( 1994, barek3mb, 0,        barek3,   barek3,   barek3,   ROT0, "bootleg / Sega",         "Bare Knuckle III (bootleg of Megadrive version)" )
GAME ( 1996, sbubsm,   0,        sbubsm,   sbubsm,   sbubsm,   ROT0, "Sun Mixing",             "Super Bubble Bobble (Sun Mixing, Megadrive clone hardware)" )

/* Atlus Print Club 'Games' (C-2 Hardware, might not be possible to support them because they use camera + printer, really just put here for reference) */
GAMEX( 1995, pclubj,   0,        segac2, pclub,    pclub,    ROT0, "Atlus",                   "Print Club (Japan Vol.1)", GAME_NOT_WORKING )
GAMEX( 1995, pclubjv2, pclubj,   segac2, pclub,    pclub,    ROT0, "Atlus",                   "Print Club (Japan Vol.2)", GAME_NOT_WORKING )
GAMEX( 1996, pclubjv4, pclubj,   segac2, pclub,    pclub,    ROT0, "Atlus",                   "Print Club (Japan Vol.4)", GAME_NOT_WORKING )
GAMEX( 1996, pclubjv5, pclubj,   segac2, pclub,    pclub,    ROT0, "Atlus",                   "Print Club (Japan Vol.5)", GAME_NOT_WORKING )


/* nn */ /* nn is part of the instruction rom name, should there be a game for each number? */
/* -- */ GAMEX( 1989, megatech, 0,        megatech, megatech, segac2, ROT0, "Sega",                  "MegaTech - Bios", NOT_A_DRIVER )
/* 01 */ GAMEX( 1989, mt_beast, megatech, megatech, megatech, segac2, ROT0, "Sega",                  "MegaTech - Altered Beast", GAME_IMPERFECT_GRAPHICS )
/* 02 */ GAMEX( 1989, mt_shar2, megatech, megatech, megatech, segac2, ROT0, "Sega",                  "MegaTech - Space Harrier 2.", GAME_NOT_WORKING )
/* 03 */ GAMEX( 1989, mt_stbld, megatech, megatech, megatech, segac2, ROT0, "Sega",                  "MegaTech - Super Thunder Blade", GAME_NOT_WORKING )
/* 04 */ GAMEX( 1989, mt_ggolf, megatech, megatech, megatech, segac2, ROT0, "Sega",                  "MegaTech - Great Golf", GAME_NOT_WORKING ) /* sms! also bad */
/* 05 */ GAMEX( 1989, mt_gsocr, megatech, megatech, megatech, segac2, ROT0, "Sega",                  "MegaTech - Great Soccer", GAME_NOT_WORKING ) /* sms! also bad */
/* 06 */ /* unknown*/
/* 07 */ /* unknown*/
/* 08 */ GAMEX( 1989, mt_shnbi, megatech, megatech, megatech, segac2, ROT0, "Sega",                  "MegaTech - Shinobi", GAME_NOT_WORKING) /* sms */
/* 09 */ /* unknown*/
/* 10 */ GAMEX( 1989, mt_aftrb, megatech, megatech, megatech, segac2, ROT0, "Sega",                  "MegaTech - Afterburner", GAME_NOT_WORKING) /* sms */
/* 11 */ GAMEX( 1989, mt_tfor2, megatech, megatech, megatech, segac2, ROT0, "Sega",                  "MegaTech - Thunder Force 2", GAME_NOT_WORKING )
/* 12 */ /* unknown*/
/* 13 */ GAMEX( 1989, mt_astro, megatech, megatech, megatech, segac2, ROT0, "Sega",                  "MegaTech - Astro Warrior", GAME_NOT_WORKING ) /* sms! */
/* 14 */ /* unknown*/
/* 15 */ /* unknown*/
/* 16 */ /* unknown*/
/* 17 */ /* unknown*/
/* 18 */ /* unknown*/
/* 19 */ /* unknown*/
/* 20 */ GAME ( 1989, mt_lastb, megatech, megatech, megatech, segac2, ROT0, "Sega",                  "MegaTech - Last Battle." )
/* 21 */ GAME ( 1989, mt_wcsoc, megatech, megatech, megatech, segac2, ROT0, "Sega",                  "MegaTech - World Cup Soccer" )
/* 22 */ GAMEX( 1989, mt_tetri, megatech, megatech, megatech, segac2, ROT0, "Sega",                  "MegaTech - Tetris", GAME_NOT_WORKING ) /* bad dump */
/* 23 */ GAMEX( 1989, mt_gng,   megatech, megatech, megatech, segac2, ROT0, "Capcom / Sega",         "MegaTech - Ghouls and Ghosts", GAME_NOT_WORKING ) /* bad dump */
/* 24 */ /* unknown*/
/* 25 */ GAMEX( 1989, mt_gaxe,  megatech, megatech, megatech, segac2, ROT0, "Sega",                  "MegaTech - Golden Axe", GAME_NOT_WORKING )
/* 26 */ /* unknown*/
/* 27 */ GAME ( 1989, mt_mystd, megatech, megatech, megatech, segac2, ROT0, "Sega",                  "MegaTech - Mystic Defender" )
/* 28 */ GAME ( 1989, mt_revsh, megatech, megatech, megatech, segac2, ROT0, "Sega",                  "MegaTech - The Revenge Of Shinobi" )
/* 29 */ GAMEX( 1989, mt_parlg, megatech, megatech, megatech, segac2, ROT0, "Sega",                  "MegaTech - Parlour Games", GAME_NOT_WORKING ) /* sms! */
/* 30 */ /* unknown*/
/* 31 */ GAME ( 1989, mt_tgolf, megatech, megatech, megatech, segac2, ROT0, "Sega",                  "MegaTech - Arnold Palmer Tournament Golf" )
/* 32 */ /* unknown*/
/* 33 */ /* unknown*/
/* 34 */ /* unknown*/
/* 35 */ GAME ( 1989, mt_tlbba, megatech, megatech, megatech, segac2, ROT0, "Sega",                  "MegaTech - Tommy Lasorda Baseball" )
/* 36 */ /* unknown*/
/* 37 */ /* unknown*/
/* 38 */ GAME ( 1989, mt_eswat, megatech, megatech, megatech, segac2, ROT0, "Sega",                  "MegaTech - E-Swat" )
/* 39 */ GAMEX( 1990, mt_smgp,  megatech, megatech, megatech, segac2, ROT0, "Sega",                  "MegaTech - Super Monaco Grand Prix", GAME_NOT_WORKING )
/* 40 */ GAMEX( 1989, mt_mwalk, megatech, megatech, megatech, segac2, ROT0, "Sega",                  "MegaTech - Moonwalker", GAME_NOT_WORKING )
/* 41 */ /* unknown*/
/* 42 */ /* unknown*/
/* 43 */ /* unknown*/
/* 44 */ /* unknown*/
/* 45 */ /* unknown*/
/* 46 */ /* unknown*/
/* 47 */ /* unknown*/
/* 48 */ /* unknown*/
/* 49 */ GAMEX( 1989, mt_bbros, megatech, megatech, megatech, segac2, ROT0, "Sega",                  "MegaTech - Bonanza Bros.", GAME_NOT_WORKING )
/* 50 */ /* unknown*/
/* 51 */ /* unknown*/
/* 52 */ GAME ( 1989, mt_sonic, megatech, megatech, megatech, segac2, ROT0, "Sega",                  "MegaTech - Sonic the Hedgehog" )
/* 53 */ GAME ( 1989, mt_fshrk, megatech, megatech, megatech, segac2, ROT0, "Sega",                  "MegaTech - Fire Shark" )
/* 54 */ /* unknown*/
/* 55 */ /* unknown*/
/* 56 */ /* unknown*/
/* 57 */ GAMEX( 1989, mt_gaxe2, megatech, megatech, megatech, segac2, ROT0, "Sega",                  "MegaTech - Golden Axe 2", GAME_NOT_WORKING )
/* 58 */ /* unknown*/
/* 59 */ /* unknown*/
/* 60 */ GAME ( 1989, mt_kcham, megatech, megatech, megatech, segac2, ROT0, "Sega",                  "MegaTech - Kid Chameleon" )
/* more? */

static DRIVER_INIT (megaplay)
{
	data8_t *src = memory_region(REGION_CPU3);
	data8_t *instruction_rom = memory_region(REGION_USER1);
	data8_t *game_rom = memory_region(REGION_CPU1);
	int offs;

	memmove(src+0x10000,src+0x8000,0x18000); /* move bios.. */

	/* copy game instruction rom to main map.. maybe this should just be accessed
      through a handler instead?.. */
	for (offs=0;offs<0x8000;offs++)
	{
		data8_t dat;

		dat=instruction_rom[offs];

		game_rom[0x300000+offs*2] = dat;
		game_rom[0x300001+offs*2] = dat;

	}

	init_segac2();

}

/* -- */ GAMEBX(1993, megaplay, 0,        megaplay, megaplay, megaplay, megaplay, ROT0, "Sega",    "MegaPlay: BIOS", NOT_A_DRIVER, &generic_ctrl, NULL )
/* 01 */ GAMEB( 1993, mp_sonic, megaplay, megaplay, megaplay, mp_sonic, megaplay, ROT0, "Sega",    "Sonic The Hedgehog", &megaplay_ctrl, NULL  )
/* 02 */ GAMEB( 1993, mp_gaxe2, megaplay, megaplay, megaplay, mp_gaxe2, megaplay, ROT0, "Sega",    "Golden Axe II", &megaplay_ctrl, NULL  )
/* 03 */ GAMEB( 1993, mp_sor2,  megaplay, megaplay, megaplay, mp_sor2,	megaplay, ROT0, "Sega",    "Streets Of Rage II", &megaplay_ctrl, NULL  )
/* 04 */ GAMEB( 1993, mp_twc,   megaplay, megaplay, megaplay, mp_twc,	  megaplay, ROT0, "Sega",    "Tecmo World Cup", &megaplay_ctrl, NULL  )
/* 05 */ GAMEB( 1993, mp_soni2, megaplay, megaplay, megaplay, mp_soni2, megaplay, ROT0, "Sega",    "Sonic The Hedgehog 2", &megaplay_ctrl, NULL  )
/* 06 */ GAMEB( 1993, mp_bio,   megaplay, megaplay, megaplay, mp_bio,	  megaplay, ROT0, "Sega",    "Bio-Hazard Battle", &megaplay_ctrl, NULL  )
/* 07 */ GAMEB( 1993, mp_shnb3, megaplay, megaplay, megaplay, mp_shnb3, megaplay, ROT0, "Sega",    "Shinobi III", &megaplay_ctrl, NULL )
/* 10 */ GAMEB( 1993, mp_col3,  megaplay, megaplay, megaplay, megaplay, megaplay, ROT0, "Sega",    "Columns III", &megaplay_ctrl, NULL )
