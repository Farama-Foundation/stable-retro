 /***************************************************************************
	M.A.M.E. Neo Geo driver presented to you by the Shin Emu Keikaku team.
	The following people have all spent probably far too much time on this:
	AVDB
	Bryan McPhail
	Fuzz
	Ernesto Corvi
	Andrew Prime
	Neogeo Motherboard (info - courtesy of Guru)
	--------------------------------------------
	PCB Layout (single slot, older version)
	NEO-MVH MV1
	|---------------------------------------------------------------------|
	|       4558                                                          |
	|                                          HC04  HC32                 |
	|                      SP-S2.SP1  NEO-E0   000-L0.L0   LS244  AS04    |
	|             YM2610                                                  |
	| 4558                                                                |
	|       4558                        5814  HC259   SFIX.SFIX           |
	|                                                             NEO-I0  |
	| HA13001 YM3016                    5814                              |
	--|                                                                   |
	  |     4558                                                          |
	--|                                                 mame.sm1   LS32    |
	|                                                                     |
	|                           LSPC-A0         PRO-C0            LS244   |
	|                                                                     |
	|J              68000                                                 |
	|A                                                                    |
	|M                                                                    |
	|M                                                      NEO-ZMC2      |
	|A                                                                    |
	|   LS273  NEO-G0                          58256  58256     Z80A      |
	|                           58256  58256   58256  58256     6116      |
	|   LS273 5864                                                        |
	--| LS05  5864  PRO-B0                                                |
	  |                                                                   |
	--|             LS06   HC32           D4990A    NEO-F0   24.000MHz    |
	|                      DSW1    BATT3.6V 32.768kHz       NEO-D0        |
	|                                           2003  2003                |
	|---------------------------------------------------------------------|
	Notes:
	      68k clock: 12.000MHz
	      Z80 clock: 4.000MHz
	   YM2610 clock: 8.000MHz
	          VSync: 60Hz
	          HSync: 15.21kHz
	         Custom SNK chips
	         ----------------
	         NEO-G0: QFP64
	         NEO-E0: QFP64
	         PRO-B0: QFP136
	        LSPC-A0: QFP160
	         PRO-C0: QFP136
	         NEO-F0: QFP64
	         NEO-D0: QFP64
	       NEO-ZMC2: QFP80
	         NEO-I0: QFP64
	         ROMs        Type
	         ----------------------------
	         SP-S2.SP1   TC531024 (DIP40)
	         000-L0.L0   TC531000 (DIP28)
	         SFIX.SFIX   D27C1000 (DIP32)
	         mame.sm1     MB832001 (DIP32)
	------------------------------------------------------
	GRAPHICAL ISSUES :
	- Effects created using the Raster Interrupt are probably not 100% correct,
	  e.g.:
	  - full screen zoom in trally and tpgolf is broken again :-( I think this was
	    caused by the fix for kof94 japan stage.
	  - Tests on the hardware show that there are 264 raster lines; however, there
	    are one or two line alignemnt issues with some games, SCANLINE_ADJUST is
	    a kludge to get the alignment almost right in most cases.
	    Some good places to test raster effects handling and alignment:
	    - aodk 100 mega shock logo
		- viewpoin Sammy logo
	    - zedblade parallax scrolling
		- ridhero road
	    - turfmast Japan course hole 4 (the one with the waterfall)
	    - fatfury3, 7th stage (Port Town). Raster effects are used for the background.
	  - spinmast uses IRQ2 with no noticeable effect (it seems to be always near
	    the bottom of the screen).
	  - garoup enables IRQ2 on Terry's stage, but with no noticeable effect. Note
	    that it is NOT enabled in 2 players mode, only vs cpu.
	  - strhoop enables IRQ2 on every scanline during attract mode, with no
	    noticeable effect.
	  - Money Idol Exchanger runs slow during the "vs. Computer" mode. Solo mode
	    works fine.
	- Full screen zoom has some glitches in tpgolf.
	- Gururin has bad tiles all over the place (used to work ..)
	- Bad clipping during scrolling at the sides on some games.
		(tpgolf for example)
	AUDIO ISSUES :
	- Sound (Music) was cutting out in ncommand and ncombat due to a bug in the
	  original code, which should obviously have no ill effect on the real YM2610 but
	  confused the emulated one. This is fixed in the YM2610 emulator.
	- Some rather bad sounding parts in a couple of Games
		(shocktro End of Intro)
	- In mahretsu music should stop when you begin play (correct after a continue) *untested*
	GAMEPLAY ISSUES / LOCKUPS :
	- Viewpoint resets halfway through level 1. This is a bug in the asm 68k core.
	- magdrop2 behaves strangely when P2 wins a 2 Player game (reports both as losing)
	- popbounc without a patch this locks up when sound is disabled, also for this game 'paddle'
	  conroller can be selected in the setup menus, but Mame doesn't support this.
	- ssideki2 locks up sometimes during play *not tested recently, certainly used to*
	- 2020bb apparently resets when the batter gets hit by the pitcher *not tested*
	- some games apparently crash / reset when you finish them before you get the ending *untested*
	- fatfury3 locks up when you complete the game.
	NON-ISSUES / FIXED ISSUES :
	- Auto Animation Speed is not quite right in Many Games
		(mslug waterfalls, some bg's in samsho4, blazstar lev 2 etc.)
	- shocktro locking up at the bosses, this was fixed a long long time ago, it was due to bugs
	  in the 68k Core.
	- sound, graphic, the odd game crash & any other strange happenings in kof99p and garoup are
	  probably because these machines are prototypes, the games are therefore not finished.  There
	  are 'patched' versions of these romsets available in some locations, however these will not
	  be supported.
	OTHER MINOR THINGS :
	- 2020bb version display, the program roms contains two version numbers, the one which always
	  get displayed when running in Mame is that which would be displayed on a console.
	  This depends on location 0x46 of nvram. That location is the BIOS "Demo sound" bit ('00' for
	  'set up in each game' and '01' for 'without'). If you set 0x46 to '01' ALL Demosound
	  (Neo Splash screen and in game attract mode) is off, and version number is 1.32X. If you set
	  0x46 to '00' and set 0x229 (Demosound bit for the game itself. '00' for 'ON' and '01' for
	  'OFF') to '01' The Neo splashscreen has sound but the ingame attract mode does not and
	  version is set to 1.32X. So it would seem that 1.32X gets displayed when demosund is off
	  and 1.02C when Demosound is on.
	NOTES ABOUT UNSUPPORTED GAMES :
	- Diggerman (???, 2000) - Not A Real Arcade Game .. Will Not Be Supported.
	VIEWPOINT CRASH
	"Viewpoint resets under the ASM core due to nested IRQ1."
=============================================================================
Points to note, known and proven information deleted from this map:
	0x3000001		Dipswitches
				bit 0 : Selftest
				bit 1 : Unknown (Unused ?) \ something to do with
				bit 2 : Unknown (Unused ?) / auto repeating keys ?
				bit 3 : \
				bit 4 :  | communication setting ?
				bit 5 : /
				bit 6 : free play
				bit 7 : stop mode ?
	0x320001		bit 0 : COIN 1
				bit 1 : COIN 2
				bit 2 : SERVICE
				bit 3 : UNKNOWN
				bit 4 : UNKNOWN
				bit 5 : UNKNOWN
				bit 6 : 4990 test pulse bit.
				bit 7 : 4990 data bit.
	0x380051		4990 control write register
				bit 0: C0
				bit 1: C1
				bit 2: C2
				bit 3-7: unused.
				0x02 = shift.
				0x00 = register hold.
				0x04 = ????.
				0x03 = time read (reset register).
	0x3c000c		IRQ acknowledge
	0x380011		Backup bank select
	0x3a0001		Enable display.
	0x3a0011		Disable display
	0x3a0003		Swap in Bios (0x80 bytes vector table of BIOS)
	0x3a0013		Swap in Rom  (0x80 bytes vector table of ROM bank)
	0x3a000d		lock backup ram
	0x3a001d		unlock backup ram
	0x3a000b		set game vector table (?)  mirror ?
	0x3a001b		set bios vector table (?)  mirror ?
	0x3a000c		Unknown (ghost pilots)
	0x31001c		Unknown (ghost pilots)
	IO word read
	0x3c0002		return vidram word (pointed to by 0x3c0000)
	0x3c0006		?????.
	0x3c0008		shadow adress for 0x3c0000 (not confirmed).
	0x3c000a		shadow adress for 0x3c0002 (confirmed, see
							   Puzzle de Pon).
	IO word write
	0x3c0006		Unknown, set vblank counter (?)
	0x3c0008		shadow address for 0x3c0000 (not confirmed)
	0x3c000a		shadow address for 0x3c0002 (not confirmed)
	The Neo Geo contains an NEC 4990 Serial I/O calendar & clock.
	accesed through 0x320001, 0x380050, 0x280050 (shadow adress).
	A schematic for this device can be found on the NEC webpages.
******************************************************************************/

/* Changes 25/03/2003 DH
-- using elsemi's information all the bios patches have been removed.
this required
bios / game vector selection to be emulated
calender emulation fixes
removal of hacks to change region / get info memory card manager
 *note1 memory card manager is reportedly only accessable on the
  neogeo console so should be a job for MESS, not MAME
 *note2 to change region you must now recompile with the relevant
  part of the code uncommented so the correct bios roms are loaded
 *note3 startup now takes longer, this is because it is performing
  memory checks which were previously skipped
-- added correct Bang Bead set (again based on Elsemi's Information)
-- fixed bios filename (based on info from Guru)
-- are the EURO bios roms infact ASIA bios roms?
*/


#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/pd4990a.h"
#include "cpu/z80/z80.h"
#include "neogeo.h"
#include "state.h"


/* values probed by Razoola from the real board */
#define RASTER_LINES 264
/* VBLANK should fire on line 248 */
#define RASTER_COUNTER_START 0x1f0	/* value assumed right after vblank */
#define RASTER_COUNTER_RELOAD 0x0f8	/* value assumed after 0x1ff */
#define RASTER_LINE_RELOAD (0x200 - RASTER_COUNTER_START)

#define SCANLINE_ADJUST 3	/* in theory should be 0, give or take an off-by-one mistake */


/******************************************************************************/

unsigned int neogeo_frame_counter;
unsigned int neogeo_frame_counter_speed=4;

/******************************************************************************/

static int irq2start=1000,irq2control;
static int current_rastercounter,current_rasterline,scanline_read;
static UINT32 irq2pos_value;
static int vblank_int,scanline_int;

/*	flags for irq2control:
	0x07 unused? kof94 sets some random combination of these at the character
		 selection screen but only because it does andi.w #$ff2f, $3c0006. It
		 clears them immediately after.
	0x08 shocktro2, stops autoanim counter
	Maybe 0x07 writes to the autoanim counter, meaning that in conjunction with
	0x08 one could fine control it. However, if that was the case, writing the
	the IRQ2 control bits would interfere with autoanimation, so I'm probably
	wrong.
	0x10 irq2 enable, tile engine scanline irq that is triggered
	when a certain scanline is reached.
	0x20 when set, the next values written in the irq position register
	sets irq2 to happen N lines after the current one
	0x40 when set, irq position register is automatically loaded at vblank to
	set the irq2 line.
	0x80 when set, every time irq2 is triggered the irq position register is
	automatically loaded to set the next irq2 line.
	0x80 and 0x40 may be set at the same time (Viewpoint does this).
*/

#define IRQ2CTRL_AUTOANIM_STOP		0x08
#define IRQ2CTRL_ENABLE				0x10
#define IRQ2CTRL_LOAD_RELATIVE		0x20
#define IRQ2CTRL_AUTOLOAD_VBLANK	0x40
#define IRQ2CTRL_AUTOLOAD_REPEAT	0x80


static void update_interrupts(void)
{
	int level = 0;

	/* determine which interrupt is active */
	if (vblank_int) level = 1;
	if (scanline_int) level = 2;

	/* either set or clear the appropriate lines */
	if (level)
		cpu_set_irq_line(0, level, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 7, CLEAR_LINE);
}

static WRITE16_HANDLER( neo_irqack_w )
{
	if (ACCESSING_LSB)
	{
		if (data & 4) vblank_int = 0;
		if (data & 2) scanline_int = 0;
		update_interrupts();
	}
}


static int fc = 0;

static INTERRUPT_GEN( neogeo_interrupt )
{
	int line = RASTER_LINES - cpu_getiloops();

	current_rasterline = line;

	{
		int l = line;

		if (l == RASTER_LINES) l = 0;	/* vblank */
		if (l < RASTER_LINE_RELOAD)
			current_rastercounter = RASTER_COUNTER_START + l;
		else
			current_rastercounter = RASTER_COUNTER_RELOAD + l - RASTER_LINE_RELOAD;
	}

	if (line == RASTER_LINES)	/* vblank */
	{
		current_rasterline = 0;

		/* Add a timer tick to the pd4990a */
		pd4990a_addretrace();

		/* Animation counter */
		if (!(irq2control & IRQ2CTRL_AUTOANIM_STOP))
		{
			if (fc++>neogeo_frame_counter_speed)	/* fixed animation speed */
			{
				fc=0;
				neogeo_frame_counter++;
			}
			/* fc++; */
		}

		if (irq2control & IRQ2CTRL_ENABLE)
			usrintf_showmessage("IRQ2 enabled, need raster driver");

		/* return a standard vblank interrupt */
		vblank_int = 1;	   /* vertical blank */
	}

	update_interrupts();
}


static int neogeo_raster_enable = 1;

static void raster_interrupt(int busy)
{
	int line = RASTER_LINES - cpu_getiloops();
	int do_refresh = 0;

	current_rasterline = line;

	{
		int l = line;

		if (l == RASTER_LINES) l = 0;	/* vblank */
		if (l < RASTER_LINE_RELOAD)
			current_rastercounter = RASTER_COUNTER_START + l;
		else
			current_rastercounter = RASTER_COUNTER_RELOAD + l - RASTER_LINE_RELOAD;
	}

	if (busy)
	{
		if (neogeo_raster_enable && scanline_read)
		{
			do_refresh = 1;
/*logerror("partial refresh at raster line %d (raster counter %03x)\n",line,current_rastercounter);*/
			scanline_read = 0;
		}
	}

	if (irq2control & IRQ2CTRL_ENABLE)
	{
		if (line == irq2start)
		{
/*logerror("trigger IRQ2 at raster line %d (raster counter %d)\n",line,current_rastercounter);*/
			if (!busy)
			{
				if (neogeo_raster_enable)
					do_refresh = 1;
			}

			if (irq2control & IRQ2CTRL_AUTOLOAD_REPEAT)
				irq2start += (irq2pos_value + 3) / 0x180;	/* ridhero gives 0x17d */

			scanline_int = 1;
		}
	}

	if (line == RASTER_LINES)	/* vblank */
	{
		current_rasterline = 0;

		if (keyboard_pressed_memory(KEYCODE_F1))
		{
			neogeo_raster_enable ^= 1;
			usrintf_showmessage("raster effects %sabled",neogeo_raster_enable ? "en" : "dis");
		}

		if (irq2control & IRQ2CTRL_AUTOLOAD_VBLANK)
			irq2start = (irq2pos_value + 3) / 0x180;	/* ridhero gives 0x17d */
		else
			irq2start = 1000;


		/* Add a timer tick to the pd4990a */
		pd4990a_addretrace();

		/* Animation counter */
		if (!(irq2control & IRQ2CTRL_AUTOANIM_STOP))
		{
			if (fc++>neogeo_frame_counter_speed)	/* fixed animation speed */
			{
				fc=0;
				neogeo_frame_counter++;
			}
			/* fc++; */
		}

		/* return a standard vblank interrupt */
/*logerror("trigger IRQ1\n");*/
		vblank_int = 1;	   /* vertical blank */
	}

	if (do_refresh)
	{
		if (line > RASTER_LINE_RELOAD)	/* avoid unnecessary updates after start of vblank */
			force_partial_update((current_rastercounter - 256) - 1 + SCANLINE_ADJUST);
	}

	update_interrupts();
}

static INTERRUPT_GEN( neogeo_raster_interrupt )
{
	raster_interrupt(0);
}

static INTERRUPT_GEN( neogeo_raster_interrupt_busy )
{
	raster_interrupt(1);
}



static int pending_command;
static int result_code;

/* Calendar, coins + Z80 communication */
static READ16_HANDLER( timer16_r )
{
	data16_t res;
	int coinflip = pd4990a_testbit_r(0);
	int databit = pd4990a_databit_r(0);

/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU %04x - Read timer\n",activecpu_get_pc());*/

	res = (readinputport(4) & ~(readinputport(5) & 0x20)) ^ (coinflip << 6) ^ (databit << 7);

	if (Machine->sample_rate)
	{
		res |= result_code << 8;
		if (pending_command) res &= 0x7fff;
	}
	else
		res |= 0x0100;

	return res;
}

static WRITE16_HANDLER( neo_z80_w )
{
	/* tpgold uses 16-bit writes, this can't be correct */
/*	if (ACCESSING_LSB)*/
/*		return;*/

	soundlatch_w(0,(data>>8)&0xff);
	pending_command = 1;
	cpu_set_irq_line(1, IRQ_LINE_NMI, PULSE_LINE);
	/* spin for a while to let the Z80 read the command (fixes hanging sound in pspikes2) */
/*	cpu_spinuntil_time(TIME_IN_USEC(20));*/
	cpu_boost_interleave(0, TIME_IN_USEC(20));
}

static int mjneogo_select;

static WRITE16_HANDLER ( mjneogeo_w )
{
	mjneogo_select = data;
}

static READ16_HANDLER ( mjneogeo_r )
{
	data16_t res;

/*
cpu #0 (PC=00C18B9A): unmapped memory word write to 00380000 = 0012 & 00FF
cpu #0 (PC=00C18BB6): unmapped memory word write to 00380000 = 001B & 00FF
cpu #0 (PC=00C18D54): unmapped memory word write to 00380000 = 0024 & 00FF
cpu #0 (PC=00C18D6C): unmapped memory word write to 00380000 = 0009 & 00FF
cpu #0 (PC=00C18C40): unmapped memory word write to 00380000 = 0000 & 00FF
*/
	res = 0;

	switch (mjneogo_select)
	{
		case 0x00:
		res = 0; /* nothing?*/
		break;
		case 0x09:
		res = (readinputport(7) << 8); /* a,b,c,d,e,g ....*/
		break;
		case 0x12:
		res = (readinputport(8) << 8); /* h,i,j,k,l ...*/
		break;
		case 0x1b:
		res = (readinputport(0) << 8); /* player 1 normal inputs?*/
		break;
		case 0x24:
		res = (readinputport(9) << 8); /* call etc.*/
		break;
		default:
		break;
	}

	return res + readinputport(3);
}



int neogeo_has_trackball;
static int ts;

static WRITE16_HANDLER( trackball_select_16_w )
{
	ts = data & 1;
}

static READ16_HANDLER( controller1_16_r )
{
	data16_t res;

	if (neogeo_has_trackball)
		res = (readinputport(ts?7:0) << 8) + readinputport(3);
	else
		res = (readinputport(0) << 8) + readinputport(3);

	return res;
}
static READ16_HANDLER( controller2_16_r )
{
	data16_t res;

	res = (readinputport(1) << 8);

	return res;
}
static READ16_HANDLER( controller3_16_r )
{
	if (memcard_status==0)
		return (readinputport(2) << 8);
	else
		return ((readinputport(2) << 8)&0x8FFF);
}
static READ16_HANDLER( controller4_16_r )
{
	return (readinputport(6) & ~(readinputport(5) & 0x40));
}

static READ16_HANDLER( popbounc1_16_r )
{
	data16_t res;

	res = (readinputport(ts?0:7) << 8) + readinputport(3);

	return res;
}

static READ16_HANDLER( popbounc2_16_r )
{
	data16_t res;

	res = (readinputport(ts?1:8) << 8);

	return res;
}

static WRITE16_HANDLER( neo_bankswitch_w )
{
	int bankaddress;

	if (memory_region_length(REGION_CPU1) <= 0x100000)
	{
log_cb(RETRO_LOG_DEBUG, LOGPRE "warning: bankswitch to %02x but no banks available\n",data);
		return;
	}

	data = data&0x7;
	bankaddress = (data+1)*0x100000;
	if (bankaddress >= memory_region_length(REGION_CPU1))
	{
log_cb(RETRO_LOG_DEBUG, LOGPRE "PC %06x: warning: bankswitch to empty bank %02x\n",activecpu_get_pc(),data);
		bankaddress = 0x100000;
	}

	neogeo_set_cpu1_second_bank(bankaddress);
}



/* TODO: Figure out how this really works! */
static READ16_HANDLER( neo_control_16_r )
{
	int res;

	/*
		The format of this very important location is:	AAAA AAAA A??? BCCC
		A is the raster line counter. mosyougi relies solely on this to do the
		  raster effects on the title screen; sdodgeb loops waiting for the top
		  bit to be 1; zedblade heavily depends on it to work correctly (it
		  checks the top bit in the IRQ2 handler).
		B is definitely a PAL/NTSC flag. Evidence:
		  1) trally changes the position of the speed indicator depending on
			 it (0 = lower 1 = higher).
		  2) samsho3 sets a variable to 60 when the bit is 0 and 50 when it's 1.
			 This is obviously the video refresh rate in Hz.
		  3) samsho3 sets another variable to 256 or 307. This could be the total
			 screen height (including vblank), or close to that.
		  Some games (e.g. lstbld2, samsho3) do this (or similar):
		  bclr	  #$0, $3c000e.l
		  when the bit is set, so 3c000e (whose function is unknown) has to be
		  related
		C is a variable speed counter. In blazstar, it controls the background
		  speed in level 2.
	*/

	scanline_read = 1;	/* needed for raster_busy optimization */

	res =	((current_rastercounter << 7) & 0xff80) |	/* raster counter */
			(neogeo_frame_counter & 0x0007);			/* frame counter */

	log_cb(RETRO_LOG_DEBUG, LOGPRE "PC %06x: neo_control_16_r (%04x)\n",activecpu_get_pc(),res);
	return res;
}


/* this does much more than this, but I'm not sure exactly what */
WRITE16_HANDLER( neo_control_16_w )
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "%06x: neo_control_16_w %04x\n",activecpu_get_pc(),data);

	/* Auto-Anim Speed Control */
	neogeo_frame_counter_speed = (data >> 8) & 0xff;

	irq2control = data & 0xff;
}

static WRITE16_HANDLER( neo_irq2pos_16_w )
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "%06x: neo_irq2pos_16_w offset %d %04x\n",activecpu_get_pc(),offset,data);

	if (offset)
		irq2pos_value = (irq2pos_value & 0xffff0000) | (UINT32)data;
	else
		irq2pos_value = (irq2pos_value & 0x0000ffff) | ((UINT32)data << 16);

	if (irq2control & IRQ2CTRL_LOAD_RELATIVE)
	{
/*		int line = (irq2pos_value + 3) / 0x180;*/	 /* ridhero gives 0x17d */ 
		int line = (irq2pos_value + 0x3b) / 0x180;	/* turfmast goes as low as 0x145 */

		irq2start = current_rasterline + line;

		log_cb(RETRO_LOG_DEBUG, LOGPRE "irq2start = %d, current_rasterline = %d, current_rastercounter = %d\n",irq2start,current_rasterline,current_rastercounter);
	}
}


/******************************************************************************/

static MEMORY_READ16_START( neogeo_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM },			/* Rom bank 1 */
	{ 0x100000, 0x10ffff, MRA16_BANK1 },		/* Ram bank 1 */
	{ 0x200000, 0x2fffff, MRA16_BANK4 },		/* Rom bank 2 */

	{ 0x300000, 0x300001, controller1_16_r },
	{ 0x300080, 0x300081, controller4_16_r },	/* Test switch in here */
	{ 0x320000, 0x320001, timer16_r },			/* Coins, Calendar, Z80 communication */
	{ 0x340000, 0x340001, controller2_16_r },
	{ 0x380000, 0x380001, controller3_16_r },
	{ 0x3c0000, 0x3c0001, neogeo_vidram16_data_r }, /* Baseball Stars */
	{ 0x3c0002, 0x3c0003, neogeo_vidram16_data_r },
	{ 0x3c0004, 0x3c0005, neogeo_vidram16_modulo_r },

	{ 0x3c0006, 0x3c0007, neo_control_16_r },
	{ 0x3c000a, 0x3c000b, neogeo_vidram16_data_r }, /* Puzzle de Pon */

	{ 0x400000, 0x401fff, neogeo_paletteram16_r },
	{ 0x6a0000, 0x6a1fff, MRA16_RAM },
	{ 0x800000, 0x800fff, neogeo_memcard16_r }, /* memory card */
	{ 0xc00000, 0xc1ffff, MRA16_BANK3 },		/* system bios rom */
	{ 0xd00000, 0xd0ffff, neogeo_sram16_r },	/* 64k battery backed SRAM */
MEMORY_END

static MEMORY_WRITE16_START( neogeo_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM },	  /* ghost pilots writes to ROM */
	{ 0x100000, 0x10ffff, MWA16_BANK1 },	/* WORK RAM*/
/*	{ 0x200000, 0x200fff, whp copies ROM data here. Why? Is there RAM in the banked ROM space? */
/* trally writes to 200000-200003 as well, probably looking for a serial link */
/* both games write to 0000fe before writing to 200000. The two things could be related. */
/* sidkicks reads and writes to several addresses in this range, using this for copy */
/* protection. Custom parts instead of the banked ROMs? */
	{ 0x2ffff0, 0x2fffff, neo_bankswitch_w },	/* NOTE THIS CHANGE TO END AT FF !!! */
	{ 0x300000, 0x300001, watchdog_reset16_w },
	{ 0x320000, 0x320001, neo_z80_w },				/* Sound CPU */
	{ 0x380000, 0x380001, trackball_select_16_w },	/* Used by bios, unknown */
	{ 0x380030, 0x380031, MWA16_NOP },				/* Used by bios, unknown */
	{ 0x380040, 0x380041, MWA16_NOP },				/* Output leds */
	{ 0x380050, 0x380051, pd4990a_control_16_w },
	{ 0x380060, 0x380063, MWA16_NOP },				/* Used by bios, unknown */
	{ 0x3800e0, 0x3800e3, MWA16_NOP },				/* Used by bios, unknown */

	{ 0x3a0000, 0x3a0001, MWA16_NOP },
	{ 0x3a0010, 0x3a0011, MWA16_NOP },
	{ 0x3a0002, 0x3a0003, neogeo_select_bios_vectors },
	{ 0x3a0012, 0x3a0013, neogeo_select_game_vectors },
	{ 0x3a000a, 0x3a000b, neo_board_fix_16_w }, /* Select board FIX char rom */
	{ 0x3a001a, 0x3a001b, neo_game_fix_16_w },	/* Select game FIX char rom */
	{ 0x3a000c, 0x3a000d, neogeo_sram16_lock_w },
	{ 0x3a001c, 0x3a001d, neogeo_sram16_unlock_w },
	{ 0x3a000e, 0x3a000f, neogeo_setpalbank1_16_w },
	{ 0x3a001e, 0x3a001f, neogeo_setpalbank0_16_w },	/* Palette banking */

	{ 0x3c0000, 0x3c0001, neogeo_vidram16_offset_w },
	{ 0x3c0002, 0x3c0003, neogeo_vidram16_data_w },
	{ 0x3c0004, 0x3c0005, neogeo_vidram16_modulo_w },

	{ 0x3c0006, 0x3c0007, neo_control_16_w },	/* IRQ2 control */
	{ 0x3c0008, 0x3c000b, neo_irq2pos_16_w },	/* IRQ2 position */
	{ 0x3c000c, 0x3c000d, neo_irqack_w },		/* IRQ acknowledge */
/*	{ 0x3c000e, 0x3c000f },*/  /* Unknown, see control_r /*/ 

	{ 0x400000, 0x401fff, neogeo_paletteram16_w },	/* COLOR RAM BANK1*/
	{ 0x6a0000, 0x6a1fff, MWA16_RAM },	/* COLOR RAM BANK0 (used only in startup tests?)*/
	{ 0x800000, 0x800fff, neogeo_memcard16_w }, 	/* mem card */
	{ 0xd00000, 0xd0ffff, neogeo_sram16_w, &neogeo_sram16 },	/* 64k battery backed SRAM */
MEMORY_END

/******************************************************************************/

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK5 },
	{ 0xc000, 0xdfff, MRA_BANK6 },
	{ 0xe000, 0xefff, MRA_BANK7 },
	{ 0xf000, 0xf7ff, MRA_BANK8 },
	{ 0xf800, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0xf7ff, MWA_ROM },
	{ 0xf800, 0xffff, MWA_RAM },
MEMORY_END


static UINT32 bank[4] = {
	0x08000,
	0x0c000,
	0x0e000,
	0x0f000
};


static READ_HANDLER( z80_port_r )
{
#if 0
{
	char buf[80];
	sprintf(buf,"%05x %05x %05x %05x",bank[0],bank[1],bank[2],bank[3]);
	usrintf_showmessage(buf);
}
#endif

	switch (offset & 0xff)
	{
	case 0x00:
		pending_command = 0;
		return soundlatch_r(0);
		break;

	case 0x04:
		return YM2610_status_port_0_A_r(0);
		break;

	case 0x05:
		return YM2610_read_port_0_r(0);
		break;

	case 0x06:
		return YM2610_status_port_0_B_r(0);
		break;

	case 0x08:
		{
			UINT8 *mem08 = memory_region(REGION_CPU2);
			bank[3] = 0x0800 * ((offset >> 8) & 0x7f) + 0x10000;
			cpu_setbank(8,&mem08[bank[3]]);
			return 0;
			break;
		}

	case 0x09:
		{
			UINT8 *mem08 = memory_region(REGION_CPU2);
			bank[2] = 0x1000 * ((offset >> 8) & 0x3f) + 0x10000;
			cpu_setbank(7,&mem08[bank[2]]);
			return 0;
			break;
		}

	case 0x0a:
		{
			UINT8 *mem08 = memory_region(REGION_CPU2);
			bank[1] = 0x2000 * ((offset >> 8) & 0x1f) + 0x10000;
			cpu_setbank(6,&mem08[bank[1]]);
			return 0;
			break;
		}

	case 0x0b:
		{
			UINT8 *mem08 = memory_region(REGION_CPU2);
			bank[0] = 0x4000 * ((offset >> 8) & 0x0f) + 0x10000;
			cpu_setbank(5,&mem08[bank[0]]);
			return 0;
			break;
		}

	default:
log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #1 PC %04x: read unmapped port %02x\n",activecpu_get_pc(),offset&0xff);
		return 0;
		break;
	}
}

static WRITE_HANDLER( z80_port_w )
{
	switch (offset & 0xff)
	{
	case 0x04:
		YM2610_control_port_0_A_w(0,data);
		break;

	case 0x05:
		YM2610_data_port_0_A_w(0,data);
		break;

	case 0x06:
		YM2610_control_port_0_B_w(0,data);
		break;

	case 0x07:
		YM2610_data_port_0_B_w(0,data);
		break;

	case 0x08:
		/* NMI enable / acknowledge? (the data written doesn't matter) */
		break;

	case 0x0c:
		result_code = data;
		break;

	case 0x18:
		/* NMI disable? (the data written doesn't matter) */
		break;

	default:
log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #1 PC %04x: write %02x to unmapped port %02x\n",activecpu_get_pc(),data,offset&0xff);
		break;
	}
}

static PORT_READ_START( neo_readio )
	{ 0x0000, 0xffff, z80_port_r },
PORT_END

static PORT_WRITE_START( neo_writeio )
	{ 0x0000, 0xffff, z80_port_w },
PORT_END

/******************************************************************************/

INPUT_PORTS_START( neogeo )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 )

	PORT_START		/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )

	PORT_START		/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )   /* Player 1 Start */
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "Next Game",KEYCODE_7, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )   /* Player 2 Start */
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "Previous Game",KEYCODE_8, IP_JOY_NONE )
	PORT_BIT( 0x30, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* memory card inserted */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* memory card write protection */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START		/* IN3 */
	PORT_DIPNAME( 0x01, 0x01, "Test Switch" )
	PORT_DIPSETTING(	0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Coin Chutes?" )
	PORT_DIPSETTING(	0x00, "1?" )
	PORT_DIPSETTING(	0x02, "2?" )
	PORT_DIPNAME( 0x04, 0x04, "Autofire (in some games)" )
	PORT_DIPSETTING(	0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x38, 0x38, "COMM Setting" )
	PORT_DIPSETTING(	0x38, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x30, "1" )
	PORT_DIPSETTING(	0x20, "2" )
	PORT_DIPSETTING(	0x10, "3" )
	PORT_DIPSETTING(	0x00, "4" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Freeze" )
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START		/* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* having this ACTIVE_HIGH causes you to start with 2 credits using USA bios roms*/
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* having this ACTIVE_HIGH causes you to start with 2 credits using USA bios roms*/
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SPECIAL )  /* handled by fake IN5 */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	/* Fake  IN 5 */
	PORT_START
#if 0
	PORT_DIPNAME( 0x03, 0x02,"Territory" )
	PORT_DIPSETTING(	0x00,"Japan" )
	PORT_DIPSETTING(	0x01,"USA" )
	PORT_DIPSETTING(	0x02,"Europe" )
/*	PORT_DIPNAME( 0x04, 0x04,"Machine Mode" )*/
/*	PORT_DIPSETTING(	0x00,"Home" )*/
/*	PORT_DIPSETTING(	0x04,"Arcade" )*/
	PORT_DIPNAME( 0x60, 0x60,"Game Slots" )		/* Stored at 0x47 of NVRAM*/
	PORT_DIPSETTING(	0x60,"2" )
/*	PORT_DIPSETTING(	0x40,"2" )*/
	PORT_DIPSETTING(	0x20,"4" )
	PORT_DIPSETTING(	0x00,"6" )
#endif

	PORT_START		/* Test switch */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SPECIAL )  /* handled by fake IN5 */
	PORT_BITX( 0x80, IP_ACTIVE_LOW, 0, "Test Switch", KEYCODE_F2, IP_JOY_NONE )
INPUT_PORTS_END

INPUT_PORTS_START( mjneogeo )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 )

	PORT_START		/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )

	PORT_START		/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )   /* Player 1 Start */
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "Next Game",KEYCODE_7, IP_JOY_NONE ) /* select*/
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )   /* Player 2 Start */
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "Previous Game",KEYCODE_8, IP_JOY_NONE )
	PORT_BIT( 0x30, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* memory card inserted */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* memory card write protection */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START		/* IN3 */
	PORT_DIPNAME( 0x01, 0x01, "Test Switch" )
	PORT_DIPSETTING(	0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Coin Chutes?" )
	PORT_DIPSETTING(	0x00, "1?" )
	PORT_DIPSETTING(	0x02, "2?" )
	PORT_DIPNAME( 0x04, 0x00, "Mahjong Control Panel" )
	PORT_DIPSETTING(	0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x38, 0x38, "COMM Setting" )
	PORT_DIPSETTING(	0x38, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x30, "1" )
	PORT_DIPSETTING(	0x20, "2" )
	PORT_DIPSETTING(	0x10, "3" )
	PORT_DIPSETTING(	0x00, "4" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Freeze" )
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START		/* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* having this ACTIVE_HIGH causes you to start with 2 credits using USA bios roms*/
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* having this ACTIVE_HIGH causes you to start with 2 credits using USA bios roms*/
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SPECIAL )  /* handled by fake IN5 */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	/* Fake  IN 5 */
	PORT_START
#if 0
	PORT_DIPNAME( 0x03, 0x02,"Territory" )
	PORT_DIPSETTING(	0x00,"Japan" )
	PORT_DIPSETTING(	0x01,"USA" )
	PORT_DIPSETTING(	0x02,"Europe" )
/*	PORT_DIPNAME( 0x04, 0x04,"Machine Mode" )*/
/*	PORT_DIPSETTING(	0x00,"Home" )*/
/*	PORT_DIPSETTING(	0x04,"Arcade" )*/
	PORT_DIPNAME( 0x60, 0x60,"Game Slots" )		/* Stored at 0x47 of NVRAM*/
	PORT_DIPSETTING(	0x60,"2" )
/*	PORT_DIPSETTING(	0x40,"2" )*/
	PORT_DIPSETTING(	0x20,"4" )
	PORT_DIPSETTING(	0x00,"6" )
#endif

	PORT_START		/* Test switch */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SPECIAL )  /* handled by fake IN5 */
	PORT_BITX( 0x80, IP_ACTIVE_LOW, 0, "Test Switch", KEYCODE_F2, IP_JOY_NONE )

	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "A",   KEYCODE_A,        IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "B",   KEYCODE_B,        IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "C",   KEYCODE_C,        IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "D",   KEYCODE_D,        IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "E",   KEYCODE_E,        IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "F",   KEYCODE_F,        IP_JOY_NONE )
	PORT_BITX(0x40, IP_ACTIVE_LOW, 0, "G",   KEYCODE_G,        IP_JOY_NONE )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "H",   KEYCODE_H,        IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "I",   KEYCODE_I,        IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "J",   KEYCODE_J,        IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "K",   KEYCODE_K,        IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "L",   KEYCODE_L,        IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "M",   KEYCODE_M,        IP_JOY_NONE )
	PORT_BITX(0x40, IP_ACTIVE_LOW, 0, "N",   KEYCODE_N,        IP_JOY_NONE )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "Pon",   KEYCODE_LALT,     IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "Chi",   KEYCODE_SPACE,    IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "Kan",   KEYCODE_LCONTROL, IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "Ron",   KEYCODE_Z,        IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "Reach", KEYCODE_LSHIFT,   IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


INPUT_PORTS_START( irrmaze )
	PORT_START		/* IN0 multiplexed */
	PORT_ANALOG( 0xff, 0x7f, IPT_TRACKBALL_X | IPF_REVERSE, 10, 20, 0, 0 )

	PORT_START		/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )

	PORT_START		/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )   /* Player 1 Start */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )   /* Player 2 Start */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x30, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* memory card inserted */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* memory card write protection */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START		/* IN3 */
	PORT_DIPNAME( 0x01, 0x01, "Test Switch" )
	PORT_DIPSETTING(	0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Coin Chutes?" )
	PORT_DIPSETTING(	0x00, "1?" )
	PORT_DIPSETTING(	0x02, "2?" )
	PORT_DIPNAME( 0x04, 0x04, "Autofire (in some games)" )
	PORT_DIPSETTING(	0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x38, 0x38, "COMM Setting" )
	PORT_DIPSETTING(	0x38, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x30, "1" )
	PORT_DIPSETTING(	0x20, "2" )
	PORT_DIPSETTING(	0x10, "3" )
	PORT_DIPSETTING(	0x00, "4" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Freeze" )
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START		/* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* having this ACTIVE_HIGH causes you to start with 2 credits using USA bios roms*/
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* having this ACTIVE_HIGH causes you to start with 2 credits using USA bios roms*/
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SPECIAL )  /* handled by fake IN5 */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	/* Fake  IN 5 */
	PORT_START
#if 0
	PORT_DIPNAME( 0x03, 0x02,"Territory" )
	PORT_DIPSETTING(	0x00,"Japan" )
	PORT_DIPSETTING(	0x01,"USA" )
	PORT_DIPSETTING(	0x02,"Europe" )
/*	PORT_DIPNAME( 0x04, 0x04,"Machine Mode" )*/
/*	PORT_DIPSETTING(	0x00,"Home" )*/
/*	PORT_DIPSETTING(	0x04,"Arcade" )*/
#endif

	PORT_START		/* Test switch */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )  /* This bit is used.. */
	PORT_BITX( 0x80, IP_ACTIVE_LOW, 0, "Test Switch", KEYCODE_F2, IP_JOY_NONE )

	PORT_START		/* IN0 multiplexed */
	PORT_ANALOG( 0xff, 0x7f, IPT_TRACKBALL_Y | IPF_REVERSE, 10, 20, 0, 0 )
INPUT_PORTS_END

INPUT_PORTS_START( popbounc )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x90, IP_ACTIVE_LOW, IPT_BUTTON1 ) /* note it needs it from 0x80 when using paddle*/
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )

	PORT_START		/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x90, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 ) /* note it needs it from 0x80 when using paddle*/
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )

	PORT_START		/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )   /* Player 1 Start */
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "Next Game",KEYCODE_7, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )   /* Player 2 Start */
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "Previous Game",KEYCODE_8, IP_JOY_NONE )
	PORT_BIT( 0x30, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* memory card inserted */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* memory card write protection */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START		/* IN3 */
	PORT_DIPNAME( 0x01, 0x01, "Test Switch" )
	PORT_DIPSETTING(	0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Coin Chutes?" )
	PORT_DIPSETTING(	0x00, "1?" )
	PORT_DIPSETTING(	0x02, "2?" )
	PORT_DIPNAME( 0x04, 0x04, "Autofire (in some games)" )
	PORT_DIPSETTING(	0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x38, 0x38, "COMM Setting" )
	PORT_DIPSETTING(	0x38, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x30, "1" )
	PORT_DIPSETTING(	0x20, "2" )
	PORT_DIPSETTING(	0x10, "3" )
	PORT_DIPSETTING(	0x00, "4" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Freeze" )
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START		/* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* having this ACTIVE_HIGH causes you to start with 2 credits using USA bios roms*/
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* having this ACTIVE_HIGH causes you to start with 2 credits using USA bios roms*/
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SPECIAL )  /* handled by fake IN5 */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	/* Fake  IN 5 */
	PORT_START
#if 0
	PORT_DIPNAME( 0x03, 0x02,"Territory" )
	PORT_DIPSETTING(	0x00,"Japan" )
	PORT_DIPSETTING(	0x01,"USA" )
	PORT_DIPSETTING(	0x02,"Europe" )
/*	PORT_DIPNAME( 0x04, 0x04,"Machine Mode" )*/
/*	PORT_DIPSETTING(	0x00,"Home" )*/
/*	PORT_DIPSETTING(	0x04,"Arcade" )*/
	PORT_DIPNAME( 0x60, 0x60,"Game Slots" )		/* Stored at 0x47 of NVRAM*/
	PORT_DIPSETTING(	0x60,"2" )
/*	PORT_DIPSETTING(	0x40,"2" )*/
	PORT_DIPSETTING(	0x20,"4" )
	PORT_DIPSETTING(	0x00,"6" )
#endif

	PORT_START		/* Test switch */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SPECIAL )  /* handled by fake IN5 */
	PORT_BITX( 0x80, IP_ACTIVE_LOW, 0, "Test Switch", KEYCODE_F2, IP_JOY_NONE )

	PORT_START		/* IN0 multiplexed */
	PORT_ANALOG( 0xff, 0x7f, IPT_DIAL, 10, 20, 0, 0 )

	PORT_START		/* IN1 multiplexed */
	PORT_ANALOG( 0xff, 0x7f, IPT_DIAL | IPF_PLAYER2 , 10, 20, 0, 0 )
INPUT_PORTS_END

INPUT_PORTS_START( kog )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 )

	PORT_START		/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )

	PORT_START		/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )   /* Player 1 Start */
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "Next Game",KEYCODE_7, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )   /* Player 2 Start */
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "Previous Game",KEYCODE_8, IP_JOY_NONE )
	PORT_BIT( 0x30, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* memory card inserted */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* memory card write protection */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START		/* IN3 */
	PORT_DIPNAME( 0x01, 0x01, "Test Switch" )
	PORT_DIPSETTING(	0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Coin Chutes?" )
	PORT_DIPSETTING(	0x00, "1?" )
	PORT_DIPSETTING(	0x02, "2?" )
	PORT_DIPNAME( 0x04, 0x04, "Autofire (in some games)" )
	PORT_DIPSETTING(	0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x38, 0x38, "COMM Setting" )
	PORT_DIPSETTING(	0x38, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x30, "1" )
	PORT_DIPSETTING(	0x20, "2" )
	PORT_DIPSETTING(	0x10, "3" )
	PORT_DIPSETTING(	0x00, "4" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Freeze" )
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START		/* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* having this ACTIVE_HIGH causes you to start with 2 credits using USA bios roms*/
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* having this ACTIVE_HIGH causes you to start with 2 credits using USA bios roms*/
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SPECIAL )  /* handled by fake IN5 */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	/* Fake  IN 5 */
	PORT_START
#if 0
	PORT_DIPNAME( 0x03, 0x02,"Territory" )
	PORT_DIPSETTING(	0x00,"Japan" )
	PORT_DIPSETTING(	0x01,"USA" )
	PORT_DIPSETTING(	0x02,"Europe" )
/*	PORT_DIPNAME( 0x04, 0x04,"Machine Mode" )*/
/*	PORT_DIPSETTING(	0x00,"Home" )*/
/*	PORT_DIPSETTING(	0x04,"Arcade" )*/
	PORT_DIPNAME( 0x60, 0x60,"Game Slots" )		/* Stored at 0x47 of NVRAM*/
	PORT_DIPSETTING(	0x60,"2" )
/*	PORT_DIPSETTING(	0x40,"2" )*/
	PORT_DIPSETTING(	0x20,"4" )
	PORT_DIPSETTING(	0x00,"6" )
#endif

	PORT_START		/* Test switch */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SPECIAL )  /* handled by fake IN5 */
	PORT_BITX( 0x80, IP_ACTIVE_LOW, 0, "Test Switch", KEYCODE_F2, IP_JOY_NONE )

	PORT_START		/* Test switch */
    PORT_DIPNAME( 0x01, 0x01,"Jumper (Title)" )
	PORT_DIPSETTING(	0x01,"USA" )
	PORT_DIPSETTING(	0x00, "Non-English" )
INPUT_PORTS_END

INPUT_PORTS_START( svcpcb )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 )

	PORT_START		/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )

	PORT_START		/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )   /* Player 1 Start */
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "Next Game",KEYCODE_7, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )   /* Player 2 Start */
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "Previous Game",KEYCODE_8, IP_JOY_NONE )
	PORT_BIT( 0x30, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* memory card inserted */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* memory card write protection */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START		/* IN3 */
	PORT_DIPNAME( 0x01, 0x01, "Test Switch" )
	PORT_DIPSETTING(	0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Coin Chutes?" )
	PORT_DIPSETTING(	0x00, "1?" )
	PORT_DIPSETTING(	0x02, "2?" )
	PORT_DIPNAME( 0x04, 0x04, "Autofire (in some games)" )
	PORT_DIPSETTING(	0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x38, 0x38, "COMM Setting" )
	PORT_DIPSETTING(	0x38, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x30, "1" )
	PORT_DIPSETTING(	0x20, "2" )
	PORT_DIPSETTING(	0x10, "3" )
	PORT_DIPSETTING(	0x00, "4" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Freeze" )
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START		/* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* having this ACTIVE_HIGH causes you to start with 2 credits using USA bios roms*/
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* having this ACTIVE_HIGH causes you to start with 2 credits using USA bios roms*/
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SPECIAL )  /* handled by fake IN5 */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	/* Fake  IN 5 */
	PORT_START
#if 0
	PORT_DIPNAME( 0x03, 0x02,"Territory" )
	PORT_DIPSETTING(	0x00,"Japan" )
	PORT_DIPSETTING(	0x01,"USA" )
	PORT_DIPSETTING(	0x02,"Europe" )
/*	PORT_DIPNAME( 0x04, 0x04,"Machine Mode" )*/
/*	PORT_DIPSETTING(	0x00,"Home" )*/
/*	PORT_DIPSETTING(	0x04,"Arcade" )*/
	PORT_DIPNAME( 0x60, 0x60,"Game Slots" )		/* Stored at 0x47 of NVRAM*/
	PORT_DIPSETTING(	0x60,"2" )
/*	PORT_DIPSETTING(	0x40,"2" )*/
	PORT_DIPSETTING(	0x20,"4" )
	PORT_DIPSETTING(	0x00,"6" )
#endif

	PORT_START		/* Test switch */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SPECIAL )  /* handled by fake IN5 */
	PORT_BITX( 0x80, IP_ACTIVE_LOW, 0, "Test Switch", KEYCODE_F2, IP_JOY_NONE )

	PORT_START		/* Test switch */
    PORT_DIPNAME( 0x01, 0x00,"Hard Dip 3 (Region)" )
	PORT_DIPSETTING(	0x00,"Asia" )
	PORT_DIPSETTING(	0x01, "Japan" )
INPUT_PORTS_END

/******************************************************************************/

/* character layout (same for all games) */
static struct GfxLayout charlayout =	/* All games */
{
	8,8,			/* 8 x 8 chars */
	RGN_FRAC(1,1),
	4,				/* 4 bits per pixel */
	{ 0, 1, 2, 3 },    /* planes are packed in a nibble */
	{ 33*4, 32*4, 49*4, 48*4, 1*4, 0*4, 17*4, 16*4 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	32*8	/* 32 bytes per char */
};

/* Placeholder and also reminder of how this graphic format is put together */
static struct GfxLayout dummy_mvs_tilelayout =
{
	16,16,	 /* 16*16 sprites */
	RGN_FRAC(1,1),
	4,
	{ GFX_RAW },
	{ 0 },		/* org displacement */
	{ 8*8 },	/* line modulo */
	128*8		/* char modulo */
};

static struct GfxDecodeInfo neogeo_mvs_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0, 16 },
	{ REGION_GFX2, 0, &charlayout, 0, 16 },
	{ REGION_GFX3, 0, &dummy_mvs_tilelayout, 0, 256 },
	{ -1 } /* end of array */
};

/******************************************************************************/

static void neogeo_sound_irq( int irq )
{
	cpu_set_irq_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

struct YM2610interface neogeo_ym2610_interface =
{
	1,
	8000000,
	{ MIXERG(15,MIXER_GAIN_4x,MIXER_PAN_CENTER) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ neogeo_sound_irq },
	{ REGION_SOUND2 },
	{ REGION_SOUND1 },
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) }
};

/******************************************************************************/

static MACHINE_DRIVER_START( neogeo )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M68000, 12000000) /* verified */
	MDRV_CPU_MEMORY(neogeo_readmem,neogeo_writemem)
	MDRV_CPU_VBLANK_INT(neogeo_interrupt,RASTER_LINES)

	MDRV_CPU_ADD(Z80, 4000000) /* verified */
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU | CPU_16BIT_PORT)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PORTS(neo_readio,neo_writeio)

	/* using a framerate of 59 will fix the sync of the kof98 video / sound however
	   using it would be a kludge as 60 has been measured using the hardware */
	MDRV_FRAMES_PER_SECOND(15625.0 / 264) /* verified with real PCB */
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(neogeo)
	MDRV_NVRAM_HANDLER(neogeo)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	/* Screen width *should* be 320, at least in the test mode for the crosshatch,
	   this has been verified on original hardware, glitches that occur at 320 in
	   Metal Slug have been verified to also appear on the MVS itself so its
	   probably correct in all cases, however to avoid confusion we use 304 unless
	   a game *needs* 320 */
/*	MDRV_VISIBLE_AREA(0*8, 40*8-1, 2*8, 30*8-1)*/
	MDRV_VISIBLE_AREA(1*8, 39*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(neogeo_mvs_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4096)

	MDRV_VIDEO_START(neogeo_mvs)
	MDRV_VIDEO_UPDATE(neogeo)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2610, neogeo_ym2610_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( raster )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(neogeo)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_VBLANK_INT(neogeo_raster_interrupt,RASTER_LINES)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_RGB_DIRECT)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( raster_busy )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(raster)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_VBLANK_INT(neogeo_raster_interrupt_busy,RASTER_LINES)

	MDRV_VISIBLE_AREA(0*8, 40*8-1, 2*8, 30*8-1)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( neo320 )
	MDRV_IMPORT_FROM(neogeo)
	/* Screen width *should* be 320, at least in the test mode for the crosshatch,
	   this has been verified on original hardware, glitches that occur at 320 in
	   Metal Slug have been verified to also appear on the MVS itself so its
	   probably correct in all cases, however to avoid confusion we use 304 unless
	   a game *needs* 320 */
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 2*8, 30*8-1)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( ras320 )
	MDRV_IMPORT_FROM(raster)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 2*8, 30*8-1)
MACHINE_DRIVER_END


/*
these games benefit in places from 320 but also have slight glitches so i'm
using 304 for them
nam1975
superspy
fbfrenzy
sengoku2
bstars2
quizdais
ssideki
aof3
*/

/******************************************************************************/

/****
 These are the known Bios Roms, Set options.bios to the one you want
 ****/

SYSTEM_BIOS_START( neogeo )
	SYSTEM_BIOS_ADD( 0, "euro",       "Europe MVS (Ver. 2)" )
	SYSTEM_BIOS_ADD( 1, "euro-s1",    "Europe MVS (Ver. 1)" )
	SYSTEM_BIOS_ADD( 2, "us",         "US MVS (Ver. 2?)" )
	SYSTEM_BIOS_ADD( 3, "us-e",       "US MVS (Ver. 1)" )
	SYSTEM_BIOS_ADD( 4, "asia",       "Asia MVS (Ver. 3)" )
	SYSTEM_BIOS_ADD( 5, "japan",      "Japan MVS (Ver. 3)" )
	SYSTEM_BIOS_ADD( 6, "japan-s2",   "Japan MVS (Ver. 2)" )
	SYSTEM_BIOS_ADD( 7, "unibios40",  "Universe Bios (Hack, Ver. 4.0)" )
	SYSTEM_BIOS_ADD( 8, "unibios33",  "Universe Bios (Hack, Ver. 3.3)" )
	SYSTEM_BIOS_ADD( 9, "unibios20",  "Universe Bios (Hack, Ver. 2.0)" )
	SYSTEM_BIOS_ADD(10, "unibios13",  "Universe Bios (Hack, Ver. 1.3)" )
	SYSTEM_BIOS_ADD(11, "unibios11",  "Universe Bios (Hack, Ver. 1.1)" )
	SYSTEM_BIOS_ADD(12, "unibios10",  "Universe Bios (Hack, Ver. 1.0)" )
	SYSTEM_BIOS_ADD(13, "debug",      "Debug MVS (Hack?)" )
	SYSTEM_BIOS_ADD(14, "asia-aes",   "Asia AES" )
SYSTEM_BIOS_END


#define ROM_LOAD16_WORD_SWAP_BIOS(bios,name,offset,length,hash) \
		ROMX_LOAD(name, offset, length, hash, ROM_GROUPWORD | ROM_REVERSE | ROM_BIOS(bios+1)) /* Note '+1' */

#define NEOGEO_BIOS \
	ROM_LOAD16_WORD_SWAP_BIOS( 0, "sp-s2.sp1",         0x00000, 0x020000, CRC(9036d879) SHA1(4f5ed7105b7128794654ce82b51723e16e389543) ) /* Europe, 1 Slot, has also been found on a 4 Slot (the old hacks were designed for this one) */ \
	ROM_LOAD16_WORD_SWAP_BIOS( 1, "sp-s.sp1",          0x00000, 0x020000, CRC(c7f2fa45) SHA1(09576ff20b4d6b365e78e6a5698ea450262697cd) ) /* Europe, 4 Slot */ \
	ROM_LOAD16_WORD_SWAP_BIOS( 2, "usa_2slt.bin",      0x00000, 0x020000, CRC(e72943de) SHA1(5c6bba07d2ec8ac95776aa3511109f5e1e2e92eb) ) /* US, 2 Slot */ \
	ROM_LOAD16_WORD_SWAP_BIOS( 3, "sp-e.sp1",          0x00000, 0x020000, CRC(2723a5b5) SHA1(5dbff7531cf04886cde3ef022fb5ca687573dcb8) ) /* US, 6 Slot (V5?) */ \
	ROM_LOAD16_WORD_SWAP_BIOS( 4, "asia-s3.rom",       0x00000, 0x020000, CRC(91b64be3) SHA1(720a3e20d26818632aedf2c2fd16c54f213543e1) ) /* Asia */ \
	ROM_LOAD16_WORD_SWAP_BIOS( 5, "vs-bios.rom",       0x00000, 0x020000, CRC(f0e8f27d) SHA1(ecf01eda815909f1facec62abf3594eaa8d11075) ) /* Japan, Ver 6 VS Bios */ \
	ROM_LOAD16_WORD_SWAP_BIOS( 6, "sp-j2.sp1",         0x00000, 0x020000, CRC(acede59c) SHA1(b6f97acd282fd7e94d9426078a90f059b5e9dd91) ) /* Japan, Older */ \
	ROM_LOAD16_WORD_SWAP_BIOS( 7, "uni-bios_4_0.rom",  0x00000, 0x020000, CRC(a7aab458) SHA1(938a0bda7d9a357240718c2cec319878d36b8f72) ) /* Universe Bios v4.0 (hack) */ \
	ROM_LOAD16_WORD_SWAP_BIOS( 8, "uni-bios_3_3.rom",  0x00000, 0x020000, CRC(24858466) SHA1(0ad92efb0c2338426635e0159d1f60b4473d0785) ) /* Universe Bios v3.3 (hack) */ \
	ROM_LOAD16_WORD_SWAP_BIOS( 9, "uni-bios_2_0.rom",  0x00000, 0x020000, CRC(0c12c2ad) SHA1(37bcd4d30f3892078b46841d895a6eff16dc921e) ) /* Universe Bios v2.0 (hack) */ \
	ROM_LOAD16_WORD_SWAP_BIOS(10, "uni-bios_1_3.rom",  0x00000, 0x020000, CRC(b24b44a0) SHA1(eca8851d30557b97c309a0d9f4a9d20e5b14af4e) ) /* Universe Bios v1.3 (hack) */ \
	ROM_LOAD16_WORD_SWAP_BIOS(11, "uni-bios_1_1.rom",  0x00000, 0x020000, CRC(5dda0d84) SHA1(4153d533c02926a2577e49c32657214781ff29b7) ) /* Universe Bios v1.1 (hack) */ \
	ROM_LOAD16_WORD_SWAP_BIOS(12, "uni-bios_1_0.rom",  0x00000, 0x020000, CRC(0ce453a0) SHA1(3b4c0cd26c176fc6b26c3a2f95143dd478f6abf9) ) /* Universe Bios v1.0 (hack) */ \
	ROM_LOAD16_WORD_SWAP_BIOS(13, "neodebug.rom",      0x00000, 0x020000, CRC(698ebb7d) SHA1(081c49aa8cc7dad5939833dc1b18338321ea0a07) ) /* Debug (Development) Bios */ \
	ROM_LOAD16_WORD_SWAP_BIOS(14, "neo-epo.bin",       0x00000, 0x020000, CRC(d27a71f1) SHA1(1b3b22092f30c4d1b2c15f04d1670eb1e9fbea07) ) /* AES Console (Asia?) Bios */ \

/* note you'll have to modify the last for lines of each block to use the extra bios roms,
   they're hacks / homebrew / console bios roms so Mame doesn't list them by default */
   
/* may 2018: what does the above comment mean? is it just talking about listing the bioses in a UI? that doesn't matter to mame2003-plus*/


/* we only have one irritating maze bios and thats asia */
#define IRRMAZE_BIOS \
	ROM_LOAD16_WORD_SWAP(         "236-bios.bin", 0x00000, 0x020000, CRC(853e6b96) SHA1(de369cb4a7df147b55168fa7aaf0b98c753b735e) ) \

#define NEO_BIOS_SOUND_512K(name,sum) \
	ROM_REGION16_BE( 0x20000, REGION_USER1, 0 ) \
	NEOGEO_BIOS \
	ROM_REGION( 0x90000, REGION_CPU2, 0 ) \
	ROM_LOAD( "mame.sm1", 0x00000, 0x20000, CRC(97cf998b) SHA1(977387a7c76ef9b21d0b01fa69830e949a9a9626) )  /* we don't use the BIOS anyway... */ \
	ROM_LOAD( name, 		0x00000, 0x80000, sum ) /* so overwrite it with the real thing */ \
	ROM_RELOAD(             0x10000, 0x80000 ) \
	ROM_REGION( 0x10000, REGION_GFX4, 0 ) \
	ROM_LOAD( "mamelo.lo", 0x00000, 0x10000, CRC(e09e253c) SHA1(2b1c719531dac9bb503f22644e6e4236b91e7cfc) )  /* Y zoom control */

#define NEO_BIOS_SOUND_256K(name,sum) \
	ROM_REGION16_BE( 0x20000, REGION_USER1, 0 ) \
	NEOGEO_BIOS \
	ROM_REGION( 0x50000, REGION_CPU2, 0 ) \
	ROM_LOAD( "mame.sm1", 0x00000, 0x20000, CRC(97cf998b) SHA1(977387a7c76ef9b21d0b01fa69830e949a9a9626) )  /* we don't use the BIOS anyway... */ \
	ROM_LOAD( name, 		0x00000, 0x40000, sum ) /* so overwrite it with the real thing */ \
	ROM_RELOAD(             0x10000, 0x40000 ) \
	ROM_REGION( 0x10000, REGION_GFX4, 0 ) \
	ROM_LOAD( "mamelo.lo", 0x00000, 0x10000, CRC(e09e253c) SHA1(2b1c719531dac9bb503f22644e6e4236b91e7cfc) )  /* Y zoom control */

#define NEO_BIOS_SOUND_128K(name,sum) \
	ROM_REGION16_BE( 0x20000, REGION_USER1, 0 ) \
	NEOGEO_BIOS \
	ROM_REGION( 0x50000, REGION_CPU2, 0 ) \
	ROM_LOAD( "mame.sm1", 0x00000, 0x20000, CRC(97cf998b) SHA1(977387a7c76ef9b21d0b01fa69830e949a9a9626) )  /* we don't use the BIOS anyway... */ \
	ROM_LOAD( name, 		0x00000, 0x20000, sum ) /* so overwrite it with the real thing */ \
	ROM_RELOAD(             0x10000, 0x20000 ) \
	ROM_REGION( 0x10000, REGION_GFX4, 0 ) \
	ROM_LOAD( "mamelo.lo", 0x00000, 0x10000, CRC(e09e253c) SHA1(2b1c719531dac9bb503f22644e6e4236b91e7cfc) )  /* Y zoom control */

#define NEO_BIOS_SOUND_64K(name,sum) \
	ROM_REGION16_BE( 0x20000, REGION_USER1, 0 ) \
	NEOGEO_BIOS \
	ROM_REGION( 0x50000, REGION_CPU2, 0 ) \
	ROM_LOAD( "mame.sm1", 0x00000, 0x20000, CRC(97cf998b) SHA1(977387a7c76ef9b21d0b01fa69830e949a9a9626) )  /* we don't use the BIOS anyway... */ \
	ROM_LOAD( name, 		0x00000, 0x10000, sum ) /* so overwrite it with the real thing */ \
	ROM_RELOAD(             0x10000, 0x10000 ) \
	ROM_REGION( 0x10000, REGION_GFX4, 0 ) \
	ROM_LOAD( "mamelo.lo", 0x00000, 0x10000, CRC(e09e253c) SHA1(2b1c719531dac9bb503f22644e6e4236b91e7cfc) )  /* Y zoom control */

#define NEO_BIOS_AUDIO_ENCRYPTED_512K(name,sum) \
	ROM_REGION16_BE( 0x20000, REGION_USER1, 0 ) \
	NEOGEO_BIOS \
	ROM_REGION( 0x90000, REGION_CPU2, 0 ) \
	ROM_REGION( 0x80000, audiocrypt, 0 ) \
	ROM_LOAD( "mame.sm1", 0x00000, 0x20000, CRC(97cf998b) SHA1(977387a7c76ef9b21d0b01fa69830e949a9a9626) )  /* we don't use the BIOS anyway... */ \
	ROM_LOAD( name, 		0x00000, 0x80000, sum ) /* so overwrite it with the real thing */ \
	ROM_REGION( 0x10000, REGION_GFX4, 0 ) \
	ROM_LOAD( "mamelo.lo", 0x00000, 0x10000, CRC(e09e253c) SHA1(2b1c719531dac9bb503f22644e6e4236b91e7cfc) )  /* Y zoom control */

#define NEO_BIOS_AUDIO_ENCRYPTED_256K(name,sum) \
	ROM_REGION16_BE( 0x20000, REGION_USER1, 0 ) \
	NEOGEO_BIOS \
	ROM_REGION( 0x90000, REGION_CPU2, 0 ) \
	ROM_REGION( 0x80000, audiocrypt, 0 ) \
	ROM_LOAD( "mame.sm1", 0x00000, 0x20000, CRC(97cf998b) SHA1(977387a7c76ef9b21d0b01fa69830e949a9a9626) )  /* we don't use the BIOS anyway... */ \
	ROM_LOAD( name, 		0x00000, 0x40000, sum ) /* so overwrite it with the real thing */ \
	ROM_REGION( 0x10000, REGION_GFX4, 0 ) \
	ROM_LOAD( "mamelo.lo", 0x00000, 0x10000, CRC(e09e253c) SHA1(2b1c719531dac9bb503f22644e6e4236b91e7cfc) )  /* Y zoom control */


#define NEO_BIOS_AUDIO_ENCRYPTED_128K(name,sum) \
	ROM_REGION16_BE( 0x20000, REGION_USER1, 0 ) \
	NEOGEO_BIOS \
	ROM_REGION( 0x90000, REGION_CPU2, 0 ) \
	ROM_REGION( 0x80000, audiocrypt, 0 ) \
	ROM_LOAD( "mame.sm1", 0x00000, 0x20000, CRC(97cf998b) SHA1(977387a7c76ef9b21d0b01fa69830e949a9a9626) )  /* we don't use the BIOS anyway... */ \
	ROM_LOAD( name, 		0x00000, 0x20000, sum ) /* so overwrite it with the real thing */ \
	ROM_REGION( 0x10000, REGION_GFX4, 0 ) \
	ROM_LOAD( "mamelo.lo", 0x00000, 0x10000, CRC(e09e253c) SHA1(2b1c719531dac9bb503f22644e6e4236b91e7cfc) )  /* Y zoom control */

#define NO_DELTAT_REGION

#define NEO_SFIX_512K(name, hash) \
	ROM_REGION( 0x80000, REGION_GFX1, 0 ) \
	ROM_LOAD( name, 		  0x000000, 0x80000, hash ) \
	ROM_REGION( 0x20000, REGION_GFX2, 0 ) \
	ROM_LOAD( "sfix.sfx",  0x000000, 0x20000, CRC(354029fc) SHA1(4ae4bf23b4c2acff875775d4cbff5583893ce2a1) )

#define NEO_SFIX_256K(name, hash) \
	ROM_REGION( 0x40000, REGION_GFX1, 0 ) \
	ROM_LOAD( name, 		  0x000000, 0x40000, hash ) \
	ROM_REGION( 0x20000, REGION_GFX2, 0 ) \
	ROM_LOAD( "sfix.sfx",  0x000000, 0x20000, CRC(354029fc) SHA1(4ae4bf23b4c2acff875775d4cbff5583893ce2a1) )

#define NEO_SFIX_128K(name, hash) \
	ROM_REGION( 0x20000, REGION_GFX1, 0 ) \
	ROM_LOAD( name, 		  0x000000, 0x20000, hash ) \
	ROM_REGION( 0x20000, REGION_GFX2, 0 ) \
	ROM_LOAD( "sfix.sfx",  0x000000, 0x20000, CRC(354029fc) SHA1(4ae4bf23b4c2acff875775d4cbff5583893ce2a1) )

#define NEO_SFIX_64K(name, hash) \
	ROM_REGION( 0x20000, REGION_GFX1, 0 ) \
	ROM_LOAD( name, 		  0x000000, 0x10000, hash ) \
	ROM_REGION( 0x20000, REGION_GFX2, 0 ) \
	ROM_LOAD( "sfix.sfx",  0x000000, 0x20000, CRC(354029fc) SHA1(4ae4bf23b4c2acff875775d4cbff5583893ce2a1) )

#define NEO_SFIX_32K(name, hash) \
	ROM_REGION( 0x20000, REGION_GFX1, 0 ) \
	ROM_LOAD( name, 		  0x000000, 0x08000, hash ) \
	ROM_REGION( 0x20000, REGION_GFX2, 0 ) \
	ROM_LOAD( "sfix.sfx",  0x000000, 0x20000, CRC(354029fc) SHA1(4ae4bf23b4c2acff875775d4cbff5583893ce2a1) )


/******************************************************************************/

ROM_START( nam1975 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "001-p1.bin", 0x000000, 0x080000, CRC(cc9fc951) SHA1(92f4e6ddeeb825077d92dbb70b50afea985f15c0) )

	NEO_SFIX_64K( "001-s1.bin", CRC(8ded55a5) SHA1(27c8ffac16fbcc19c843d1b5b47ae8d8ef83d44a) )

	NEO_BIOS_SOUND_64K( "001-m1.bin", CRC(cd088502) SHA1(939f745cd576905f326e6246a2eed78e9fa88178) )

	ROM_REGION( 0x080000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "001-v11.bin", 0x000000, 0x080000, CRC(a7c3d5e5) SHA1(e3efc86940f91c53b7724c4566cfc21ea1a7a465) )

	ROM_REGION( 0x180000, REGION_SOUND2, ROMREGION_SOUNDONLY )
	ROM_LOAD( "001-v21.bin", 0x000000, 0x080000, CRC(55e670b3) SHA1(a047049646a90b6db2d1882264df9256aa5a85e5) )
	ROM_LOAD( "001-v22.bin", 0x080000, 0x080000, CRC(ab0d8368) SHA1(404114db9f3295929080b87a5d0106b40da6223a) )
	ROM_LOAD( "001-v23.bin", 0x100000, 0x080000, CRC(df468e28) SHA1(4e5d4a709a4737a87bba4083aeb788f657862f1a) )

	ROM_REGION( 0x300000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "001-c1.bin", 0x000000, 0x80000, CRC(32ea98e1) SHA1(c2fb3fb7dd14523a4b4b7fbdb81f44cb4cc48239) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "001-c2.bin", 0x000001, 0x80000, CRC(cbc4064c) SHA1(224c970fd060d841fd430c946ef609bb57b6d78c) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "001-c3.bin", 0x100000, 0x80000, CRC(0151054c) SHA1(f24fb501a7845f64833f4e5a461bcf9dc3262557) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "001-c4.bin", 0x100001, 0x80000, CRC(0a32570d) SHA1(f108446ec7844fde25f7a4ab454f76d384bf5e52) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "001-c5.bin", 0x200000, 0x80000, CRC(90b74cc2) SHA1(89898da36db259180e5261ed45eafc99ca13e504) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "001-c6.bin", 0x200001, 0x80000, CRC(e62bed58) SHA1(d05b2903b212a51ee131e52c761b714cb787683e) ) /* Plane 2,3 */
ROM_END

ROM_START( bstars )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "002-p1.bin", 0x000000, 0x080000, CRC(3bc7790e) SHA1(50b2fffb1278151bb4849fbe1f8cb23916019815) )

	NEO_SFIX_128K( "002-s1.bin", CRC(1a7fd0c6) SHA1(3fc701b7afddab369ddf9dedfbc5e1aaf80b8af3) )

	NEO_BIOS_SOUND_64K( "002-m1.bin", CRC(79a8f4c2) SHA1(08ca2b72fd9b0ed068dc918ecda51d13203a3481) )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "002-v11.bin", 0x000000, 0x080000, CRC(b7b925bd) SHA1(644c92fa90e74998e73714f74b1e0680ee372a07) )
	ROM_LOAD( "002-v12.bin", 0x080000, 0x080000, CRC(329f26fc) SHA1(2c8009edc88c6b26f7be5beb2b8d260aac394ee1) )
	ROM_LOAD( "002-v13.bin", 0x100000, 0x080000, CRC(0c39f3c8) SHA1(db8f8670639601215707d918d4fb93221460446a) )
	ROM_LOAD( "002-v14.bin", 0x180000, 0x080000, CRC(c7e11c38) SHA1(5abf2a7877e0162c758a4dcf09f183930fa7ef24) )

	ROM_REGION( 0x080000, REGION_SOUND2, ROMREGION_SOUNDONLY )
	ROM_LOAD( "002-v21.bin", 0x000000, 0x080000, CRC(04a733d1) SHA1(84159368c0f6de2c3b8121227201cd3422455cf6) )

	ROM_REGION( 0x300000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "002-c1.bin", 0x000000, 0x080000, CRC(aaff2a45) SHA1(c91ee72d1d74514df8ec44fca703409d92158ae3) )
	ROM_LOAD16_BYTE( "002-c2.bin", 0x000001, 0x080000, CRC(3ba0f7e4) SHA1(f023b134b9c7994f477867307d2732026033501d) )
	ROM_LOAD16_BYTE( "002-c3.bin", 0x100000, 0x080000, CRC(96f0fdfa) SHA1(9f779a1ae46aeda54d69382b074392ade687f62f) )
	ROM_LOAD16_BYTE( "002-c4.bin", 0x100001, 0x080000, CRC(5fd87f2f) SHA1(a5dd6f26f9485f216c2428ae1792c182beb10dbc) )
	ROM_LOAD16_BYTE( "002-c5.bin", 0x200000, 0x080000, CRC(807ed83b) SHA1(3268e7d4602c3f55f1e0da2c80653d5ae461ef67) )
	ROM_LOAD16_BYTE( "002-c6.bin", 0x200001, 0x080000, CRC(5a3cad41) SHA1(c620d18f4ff32ed5489c941dfc641030a54f1c14) )
ROM_END

ROM_START( tpgolf )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "003-p1.bin", 0x000000, 0x080000, CRC(f75549ba) SHA1(3f7bdf5e2964e921fe1dd87c51a79a1a501fc73f) )
	ROM_LOAD16_WORD_SWAP( "003-p2.bin", 0x080000, 0x080000, CRC(b7809a8f) SHA1(1604c889592c9610668bff296de48a0d6906156d) )

	NEO_SFIX_128K( "003-s1.bin", CRC(7b3eb9b1) SHA1(39cd8bad9f8bfdeb8ac681b5b79ae5aa81c8dd5f) )

	NEO_BIOS_SOUND_64K( "003-m1.bin", CRC(7851d0d9) SHA1(d021cef958cc37ab170b78d7a4b3ae94947e4d13) )

	ROM_REGION( 0x080000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "003-v11.bin", 0x000000, 0x080000, CRC(ff97f1cb) SHA1(defa249d46ae220f7bfa70746f5202bbbcc3e5fe) )

	ROM_REGION( 0x200000, REGION_SOUND2, ROMREGION_SOUNDONLY )
	ROM_LOAD( "003-v21.bin", 0x000000, 0x080000, CRC(d34960c6) SHA1(36d5877d5e42aab943f4d693159f4f3ad8b0addc) )
	ROM_LOAD( "003-v22.bin", 0x080000, 0x080000, CRC(9a5f58d4) SHA1(2b580595e1820430a36f06fd3e0e0b8f7d686889) )
	ROM_LOAD( "003-v23.bin", 0x100000, 0x080000, CRC(30f53e54) SHA1(22461f88a56d272b78dbc23204c0c6816200532b) )
	ROM_LOAD( "003-v24.bin", 0x180000, 0x080000, CRC(5ba0f501) SHA1(ca02937a611a2c50c9e4b54f8fd4eaea09259894) )

	ROM_REGION( 0x400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "003-c1.bin", 0x000000, 0x80000, CRC(0315fbaf) SHA1(583c9253219c1026d81ee5e0cf5568683adc2633) )
	ROM_LOAD16_BYTE( "003-c2.bin", 0x000001, 0x80000, CRC(b4c15d59) SHA1(b0d8ec967f9b8e5216301c10b2d36912abce6515) )
	ROM_LOAD16_BYTE( "003-c3.bin", 0x100000, 0x80000, CRC(b09f1612) SHA1(03fbb5db4e377ce9cb4e65ddbc0b114c02e7bae1) )
	ROM_LOAD16_BYTE( "003-c4.bin", 0x100001, 0x80000, CRC(150ea7a1) SHA1(13edc30144b56285ef37eb8aa6fb934704de18d8) )
	ROM_LOAD16_BYTE( "003-c5.bin", 0x200000, 0x80000, CRC(9a7146da) SHA1(2fc83d13e3e9565919aab01bf2a1b028f433b547) )
	ROM_LOAD16_BYTE( "003-c6.bin", 0x200001, 0x80000, CRC(1e63411a) SHA1(ee397e2f679042e87b37d95837af62bb95a72af9) )
	ROM_LOAD16_BYTE( "003-c7.bin", 0x300000, 0x80000, CRC(2886710c) SHA1(1533dd935f0a8f92a0a3c47d1d2bc6d035454244) )
	ROM_LOAD16_BYTE( "003-c8.bin", 0x300001, 0x80000, CRC(422af22d) SHA1(f67c844c34545de6ea187f5bfdf440dec8518532) )
ROM_END

ROM_START( mahretsu )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "004-p1.bin", 0x000000, 0x080000, CRC(fc6f53db) SHA1(64a62ca4c8fb68954e06121399c9402278bd0467) )

	NEO_SFIX_64K( "004-s1.bin", CRC(b0d16529) SHA1(1483a3459309596ee3ecff68bdbde0809d82dd7a) )

	NEO_BIOS_SOUND_64K( "004-m1.bin", CRC(37965a73) SHA1(61ad03c74169f9f50a37048fb74300ee926ee766) )

	ROM_REGION( 0x100000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "004-v1.bin", 0x000000, 0x080000, CRC(b2fb2153) SHA1(36e0cc8927b11105de40188af46f6cf532794c10) )
	ROM_LOAD( "004-v2.bin", 0x080000, 0x080000, CRC(8503317b) SHA1(ab22f1aba1e977ab234a4f1d73dc6ed789dbeb85) )

	ROM_REGION( 0x180000, REGION_SOUND2, ROMREGION_SOUNDONLY )
	ROM_LOAD( "004-v3.bin", 0x000000, 0x080000, CRC(4999fb27) SHA1(2d4926a220ea21bdd5e816bb16f985fff089500a) )
	ROM_LOAD( "004-v4.bin", 0x080000, 0x080000, CRC(776fa2a2) SHA1(e7d5a362ab7806b7b009700a435c815a20e8ec68) )
	ROM_LOAD( "004-v5.bin", 0x100000, 0x080000, CRC(b3e7eeea) SHA1(4d1e97f380702a3a06e7f954b4caddd9c4119d8f) )

	ROM_REGION( 0x200000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "004-c1.bin", 0x000000, 0x80000, CRC(f1ae16bc) SHA1(df68feed4dcba1e1566032b01ebb7b478a1792bf) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "004-c2.bin", 0x000001, 0x80000, CRC(bdc13520) SHA1(2bc4c996d019a4c539f6c3188ef18089e54b7efa) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "004-c3.bin", 0x100000, 0x80000, CRC(9c571a37) SHA1(21388aeb92bb8e15a55a063701ca9df79e292127) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "004-c4.bin", 0x100001, 0x80000, CRC(7e81cb29) SHA1(5036f04df30cf6903bd1a8cc06ff6f015c24a74b) ) /* Plane 2,3 */
ROM_END

ROM_START( maglord )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "005-p1.bin", 0x000000, 0x080000, CRC(bd0a492d) SHA1(d043d3710cf2b0d2b3798008e65e4c7c3ead1af3) )

	NEO_SFIX_128K( "005-s1.bin", CRC(1c5369a2) SHA1(db0dba0a7dced6c9ca929c5abda491b05d84199c) )

	NEO_BIOS_SOUND_64K( "005-m1.bin", CRC(91ee1f73) SHA1(f060728543333a99216cc2879f7666a1f4235068) )

	ROM_REGION( 0x080000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "005-v11.bin", 0x000000, 0x080000, CRC(cc0455fd) SHA1(a8ff4270e7705e263d25ff0b301f503bccea7e59) )

	ROM_REGION( 0x100000, REGION_SOUND2, ROMREGION_SOUNDONLY )
	ROM_LOAD( "005-v21.bin", 0x000000, 0x080000, CRC(f94ab5b7) SHA1(2c16985102e3585e08622d8c54ac5c60425b9ff8) )
	ROM_LOAD( "005-v22.bin", 0x080000, 0x080000, CRC(232cfd04) SHA1(61b66a9decbbd1f500a8c186615e7fd077c6861e) )

	ROM_REGION( 0x300000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "005-c1.bin", 0x000000, 0x80000, CRC(806aee34) SHA1(3c32a0edbbddb694495b510c13979c44b83de8bc) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "005-c2.bin", 0x000001, 0x80000, CRC(34aa9a86) SHA1(cec97e1ff7f91158040c629ba75742db82c4ae5e) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "005-c3.bin", 0x100000, 0x80000, CRC(c4c2b926) SHA1(478bfafca21f5a1338808251a06ab405e6a9e65f) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "005-c4.bin", 0x100001, 0x80000, CRC(9c46dcf4) SHA1(4c05f3dc25777a87578ce09a6cefb3a4cebf3266) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "005-c5.bin", 0x200000, 0x80000, CRC(69086dec) SHA1(7fa47f4a765948813ebf366168275dcc3c42e951) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "005-c6.bin", 0x200001, 0x80000, CRC(ab7ac142) SHA1(e6ad2843947d35d8e913d2666f87946c1ba7944f) ) /* Plane 2,3 */
ROM_END

ROM_START( maglordh )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "005-p1h.bin", 0x000000, 0x080000, CRC(599043c5) SHA1(43f234b0f89b72b4c6050c40d9daa5c4e96b94ce) )

	NEO_SFIX_128K( "005-s1.bin", CRC(1c5369a2) SHA1(db0dba0a7dced6c9ca929c5abda491b05d84199c) )

	NEO_BIOS_SOUND_64K( "005-m1.bin", CRC(91ee1f73) SHA1(f060728543333a99216cc2879f7666a1f4235068) )

	ROM_REGION( 0x080000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "005-v11.bin", 0x000000, 0x080000, CRC(cc0455fd) SHA1(a8ff4270e7705e263d25ff0b301f503bccea7e59) )

	ROM_REGION( 0x100000, REGION_SOUND2, ROMREGION_SOUNDONLY )
	ROM_LOAD( "005-v21.bin", 0x000000, 0x080000, CRC(f94ab5b7) SHA1(2c16985102e3585e08622d8c54ac5c60425b9ff8) )
	ROM_LOAD( "005-v22.bin", 0x080000, 0x080000, CRC(232cfd04) SHA1(61b66a9decbbd1f500a8c186615e7fd077c6861e) )

	ROM_REGION( 0x300000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "005-c1.bin", 0x000000, 0x80000, CRC(806aee34) SHA1(3c32a0edbbddb694495b510c13979c44b83de8bc) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "005-c2.bin", 0x000001, 0x80000, CRC(34aa9a86) SHA1(cec97e1ff7f91158040c629ba75742db82c4ae5e) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "005-c3.bin", 0x100000, 0x80000, CRC(c4c2b926) SHA1(478bfafca21f5a1338808251a06ab405e6a9e65f) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "005-c4.bin", 0x100001, 0x80000, CRC(9c46dcf4) SHA1(4c05f3dc25777a87578ce09a6cefb3a4cebf3266) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "005-c5.bin", 0x200000, 0x80000, CRC(69086dec) SHA1(7fa47f4a765948813ebf366168275dcc3c42e951) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "005-c6.bin", 0x200001, 0x80000, CRC(ab7ac142) SHA1(e6ad2843947d35d8e913d2666f87946c1ba7944f) ) /* Plane 2,3 */
ROM_END

ROM_START( ridhero )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "006-p1.bin", 0x000000, 0x080000, CRC(d4aaf597) SHA1(34d35b71adb5bd06f4f1b50ffd9c58ab9c440a84) )

	NEO_SFIX_64K( "006-s1.bin", CRC(197d1a28) SHA1(3f6ec453ebdead50c9fabd71071817b699a8a82c) )

	NEO_BIOS_SOUND_128K( "006-m1.bin", CRC(f0b6425d) SHA1(ba90c665d3e84c40d0383be64b1399ba831c0cfc) )

	ROM_REGION( 0x100000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "006-v11.bin", 0x000000, 0x080000, CRC(cdf74a42) SHA1(a17106cc3f9e5c5d52b4def861c0545a98151da2) )
	ROM_LOAD( "006-v12.bin", 0x080000, 0x080000, CRC(e2fd2371) SHA1(cc95297bee7ffbdcb24ac4daeb5307cb39a52067) )

	ROM_REGION( 0x200000, REGION_SOUND2, ROMREGION_SOUNDONLY )
	ROM_LOAD( "006-v21.bin", 0x000000, 0x080000, CRC(94092bce) SHA1(1a2906271fe6bc396898a756153629a5862930eb) )
	ROM_LOAD( "006-v22.bin", 0x080000, 0x080000, CRC(4e2cd7c3) SHA1(72fb215a4f208a22a764e801186d1643d3d840ca) )
	ROM_LOAD( "006-v23.bin", 0x100000, 0x080000, CRC(069c71ed) SHA1(f450e9f60cd6ef846dbc77993159ec6157fb64e7) )
	ROM_LOAD( "006-v24.bin", 0x180000, 0x080000, CRC(89fbb825) SHA1(656a97c6a8832dab3a5e1577d9cd257b561cc356) )

	ROM_REGION( 0x200000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "006-c1.bin", 0x000000, 0x080000, CRC(4a5c7f78) SHA1(f8f1e6b7841c74368210d52a84307bb28f722a2d) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "006-c2.bin", 0x000001, 0x080000, CRC(e0b70ece) SHA1(e2b750e43cdddcea29d1c9c943a3628117a16a1b) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "006-c3.bin", 0x100000, 0x080000, CRC(8acff765) SHA1(11fe89b9d112d0658c9ddf40d928584de6ea9202) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "006-c4.bin", 0x100001, 0x080000, CRC(205e3208) SHA1(aa2acf2c6f48ffffdcc0c94ddc031acc9e4a2e68) ) /* Plane 2,3 */
ROM_END

ROM_START( ridheroh )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "006-p1h.bin", 0x000000, 0x080000, CRC(52445646) SHA1(647bb31f2f68453c1366cb6e2e867e37d1df7a54) )

	NEO_SFIX_64K( "006-s1.bin", CRC(197d1a28) SHA1(3f6ec453ebdead50c9fabd71071817b699a8a82c) )

	NEO_BIOS_SOUND_128K( "006-m1.bin", CRC(f0b6425d) SHA1(ba90c665d3e84c40d0383be64b1399ba831c0cfc) )

	ROM_REGION( 0x100000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "006-v11.bin", 0x000000, 0x080000, CRC(cdf74a42) SHA1(a17106cc3f9e5c5d52b4def861c0545a98151da2) )
	ROM_LOAD( "006-v12.bin", 0x080000, 0x080000, CRC(e2fd2371) SHA1(cc95297bee7ffbdcb24ac4daeb5307cb39a52067) )

	ROM_REGION( 0x200000, REGION_SOUND2, ROMREGION_SOUNDONLY )
	ROM_LOAD( "006-v21.bin", 0x000000, 0x080000, CRC(94092bce) SHA1(1a2906271fe6bc396898a756153629a5862930eb) )
	ROM_LOAD( "006-v22.bin", 0x080000, 0x080000, CRC(4e2cd7c3) SHA1(72fb215a4f208a22a764e801186d1643d3d840ca) )
	ROM_LOAD( "006-v23.bin", 0x100000, 0x080000, CRC(069c71ed) SHA1(f450e9f60cd6ef846dbc77993159ec6157fb64e7) )
	ROM_LOAD( "006-v24.bin", 0x180000, 0x080000, CRC(89fbb825) SHA1(656a97c6a8832dab3a5e1577d9cd257b561cc356) )

	ROM_REGION( 0x200000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "006-c1.bin", 0x000000, 0x080000, CRC(4a5c7f78) SHA1(f8f1e6b7841c74368210d52a84307bb28f722a2d) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "006-c2.bin", 0x000001, 0x080000, CRC(e0b70ece) SHA1(e2b750e43cdddcea29d1c9c943a3628117a16a1b) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "006-c3.bin", 0x100000, 0x080000, CRC(8acff765) SHA1(11fe89b9d112d0658c9ddf40d928584de6ea9202) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "006-c4.bin", 0x100001, 0x080000, CRC(205e3208) SHA1(aa2acf2c6f48ffffdcc0c94ddc031acc9e4a2e68) ) /* Plane 2,3 */
ROM_END

ROM_START( alpham2 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "007-p1.bin", 0x000000, 0x080000, CRC(5b266f47) SHA1(8afbf995989f47ad93fea1f31a884afc7228b53a) )
	ROM_LOAD16_WORD_SWAP( "007-p2.bin", 0x080000, 0x020000, CRC(eb9c1044) SHA1(65d3416dcd96663bc4e7cefe90ecb7c1eafb2dda) )

	NEO_SFIX_128K( "007-s1.bin", CRC(85ec9acf) SHA1(39a11974438ad36a2cc84307151b31474c3c5518) )

	NEO_BIOS_SOUND_128K( "007-m1.bin", CRC(28dfe2cd) SHA1(1a1a99fb917c6c8db591e3be695ce03f843ee1df) )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "007-v1.bin", 0x000000, 0x100000, CRC(cd5db931) SHA1(b59f9f2df29f49470312a6cd20f5669b6aaf51ff) )
	ROM_LOAD( "007-v2.bin", 0x100000, 0x100000, CRC(63e9b574) SHA1(1ade4cd0b15c84dd4a0fb7f7abf0885eef3a3f71) )

	NO_DELTAT_REGION

	ROM_REGION( 0x300000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "007-c1.bin", 0x000000, 0x100000, CRC(8fba8ff3) SHA1(1a682292e99eb91b0edb9771c44bc5e762867e98) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "007-c2.bin", 0x000001, 0x100000, CRC(4dad2945) SHA1(ac85a146276537fed124bda892bb93ff549f1d93) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "007-c3.bin", 0x200000, 0x080000, CRC(68c2994e) SHA1(4f8dfc6e5188942e03b853a2c9f0ea6138dec791) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "007-c4.bin", 0x200001, 0x080000, CRC(7d588349) SHA1(a5ed789d7bbc25be5c5b2d99883b64d379c103a2) ) /* Plane 2,3 */
ROM_END

ROM_START( ncombat )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "009-p1.bin", 0x000000, 0x080000, CRC(b45fcfbf) SHA1(3872147dda2d1ba905d35f4571065d87b1958b4a) )

	NEO_SFIX_128K( "009-s1.bin", CRC(d49afee8) SHA1(77615f12edf08ae8f1353f7a056a8f3a50d3ebdc) )

	NEO_BIOS_SOUND_128K( "009-m1.bin", CRC(b5819863) SHA1(6f2309d51531052dbf7d712993c9e35649db0d84) )

	ROM_REGION( 0x180000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "009-v11.bin", 0x000000, 0x080000, CRC(cf32a59c) SHA1(af5b7fcd8a4aff1307c0a1d937e5f0460c32de79) )
	ROM_LOAD( "009-v12.bin", 0x080000, 0x080000, CRC(7b3588b7) SHA1(a4e6d9d4113ff4ce48b371f65e9187d551821d3b) )
	ROM_LOAD( "009-v13.bin", 0x100000, 0x080000, CRC(505a01b5) SHA1(9426a4f5b31e16f74e72e61951c189a878f211c5) )

	ROM_REGION( 0x080000, REGION_SOUND2, ROMREGION_SOUNDONLY )
	ROM_LOAD( "009-v21.bin", 0x000000, 0x080000, CRC(365f9011) SHA1(aebd292214ab280b05ee9e759b7e9a681a099c4a) )

	ROM_REGION( 0x300000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "009-c1.bin", 0x000000, 0x80000, CRC(33cc838e) SHA1(c445c891c0ba4190aa0b472786150620e76df5b4) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "009-c2.bin", 0x000001, 0x80000, CRC(26877feb) SHA1(8f48097fb8e4757f50b6d86219122fbf4b6f87ef) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "009-c3.bin", 0x100000, 0x80000, CRC(3b60a05d) SHA1(0a165a17af4834876fcd634599cd2208adc9248f) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "009-c4.bin", 0x100001, 0x80000, CRC(39c2d039) SHA1(8ca6c3f977c43c7abba2a00a0e70f02e2a49f5f2) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "009-c5.bin", 0x200000, 0x80000, CRC(67a4344e) SHA1(b325f152c7b2388fc92c5826e1dc99094b9ea749) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "009-c6.bin", 0x200001, 0x80000, CRC(2eca8b19) SHA1(16764ef10e404325ba0a1a2ad3a4c0af287be21f) ) /* Plane 2,3 */
ROM_END

ROM_START( cyberlip )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "010-p1.bin", 0x000000, 0x080000, CRC(69a6b42d) SHA1(6e7cb089de83f1d22cc4a87db5b1a94bf76fb1e8) )

	NEO_SFIX_128K( "010-s1.bin", CRC(79a35264) SHA1(c2819a82adbe1f5e489496e0e03477863a5b7665) )

	NEO_BIOS_SOUND_64K( "010-m1.bin", CRC(47980d3a) SHA1(afd7be531d2ba69400dff5927202446873266c06) )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "010-v11.bin", 0x000000, 0x080000, CRC(90224d22) SHA1(5443ee6f90d80d43194cb4b4f0e08851a59e7784) )
	ROM_LOAD( "010-v12.bin", 0x080000, 0x080000, CRC(a0cf1834) SHA1(8df57a7941bdae7e446a6056039adb012cdde246) )
	ROM_LOAD( "010-v13.bin", 0x100000, 0x080000, CRC(ae38bc84) SHA1(c0937b4f89b8b26c8a0e747b234f44ad6a3bf2ba) )
	ROM_LOAD( "010-v14.bin", 0x180000, 0x080000, CRC(70899bd2) SHA1(8cf01144f0bcf59f09777175ae6b71846b09f3a1) )

	ROM_REGION( 0x080000, REGION_SOUND2, ROMREGION_SOUNDONLY )
	ROM_LOAD( "010-v21.bin", 0x000000, 0x080000, CRC(586f4cb2) SHA1(588460031d84c308e3353ecf714db9986425c21c) )

	ROM_REGION( 0x300000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "010-c1.bin", 0x000000, 0x80000, CRC(8bba5113) SHA1(70f0926409ab265da4b8632500d1d32d63cf77cf) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "010-c2.bin", 0x000001, 0x80000, CRC(cbf66432) SHA1(cc529640c475d08330e116ea9c5e5a28b7cd13db) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "010-c3.bin", 0x100000, 0x80000, CRC(e4f86efc) SHA1(fa60863d8a7ed4f21d30f91eb1936d0b8329db7a) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "010-c4.bin", 0x100001, 0x80000, CRC(f7be4674) SHA1(b4ad0432d4bb6d5a98e27015910343c964b73ed4) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "010-c5.bin", 0x200000, 0x80000, CRC(e8076da0) SHA1(3ec5cc19809dea688041a42b32c13d257576f3da) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "010-c6.bin", 0x200001, 0x80000, CRC(c495c567) SHA1(2f58475fbb5f1adafce027d396fb05dd71e8fb55) ) /* Plane 2,3 */
ROM_END

ROM_START( superspy )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "011-p1.bin", 0x000000, 0x080000, CRC(c7f944b5) SHA1(da7560e09187c68f1d9f7656218497b4464c56c9) )
	ROM_LOAD16_WORD_SWAP( "011-p2.bin", 0x080000, 0x020000, CRC(811a4faf) SHA1(8169dfaf79f52d80ecec402ce1b1ab9cafb7ebdd) )

	NEO_SFIX_128K( "011-s1.bin", CRC(ec5fdb96) SHA1(8003028025ac7bf531e568add6ba66c02d0b7e84) )

	NEO_BIOS_SOUND_128K( "011-m1.bin", CRC(d59d5d12) SHA1(0692b6f35c9ee3840f945703c4eb51ab83eb0714) )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "011-v11.bin", 0x000000, 0x100000, CRC(5c674d5c) SHA1(d7b9beddeb247b584cea9ca6c43ec6869809b673) )
	ROM_LOAD( "011-v12.bin", 0x100000, 0x100000, CRC(7df8898b) SHA1(23dd6df47fa51b210af800ae551295300e125106) )

	ROM_REGION( 0x100000, REGION_SOUND2, ROMREGION_SOUNDONLY )
	ROM_LOAD( "011-v21.bin", 0x000000, 0x100000, CRC(1ebe94c7) SHA1(c186810523a7df880cb080c62aa322bbcaefca17) )

	ROM_REGION( 0x400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "011-c1.bin", 0x000000, 0x100000, CRC(cae7be57) SHA1(43b35b349594535689c358d9f324adda55e5281a) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "011-c2.bin", 0x000001, 0x100000, CRC(9e29d986) SHA1(b417763bad1acf76116cd56f4203c2d2677e22e5) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "011-c3.bin", 0x200000, 0x100000, CRC(14832ff2) SHA1(1179792d773d97d5e45e7d8f009051d362d72e24) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "011-c4.bin", 0x200001, 0x100000, CRC(b7f63162) SHA1(077a81b2bb0a8f17c9df6945078608f74432877a) ) /* Plane 2,3 */
ROM_END

ROM_START( mutnat )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "014-p1.bin", 0x000000, 0x080000, CRC(6f1699c8) SHA1(87206f67a619dede7959230f9ff3701b8b78957a) )

	NEO_SFIX_128K( "014-s1.bin", CRC(99419733) SHA1(b2524af8704941acc72282aa1d62fd4c93e3e822) )

	NEO_BIOS_SOUND_128K( "014-m1.bin", CRC(b6683092) SHA1(623ec7ec2915fb077bf65b4a16c815e071c25259) )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "014-v1.bin", 0x000000, 0x100000, CRC(25419296) SHA1(c9fc04987c4e0875d276e1a0fb671740b6f548ad) )
	ROM_LOAD( "014-v2.bin", 0x100000, 0x100000, CRC(0de53d5e) SHA1(467f6040da3dfb1974785e95e14c3f608a93720a) )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "014-c1.bin", 0x000000, 0x100000, CRC(5e4381bf) SHA1(d429a5e09dafd2fb99495658b3652eecbf58f91b) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "014-c2.bin", 0x000001, 0x100000, CRC(69ba4e18) SHA1(b3369190c47771a790c7adffa958ff55d90e758b) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "014-c3.bin", 0x200000, 0x100000, CRC(890327d5) SHA1(47f97bf120a8480758e1f3bb8982be4c5325c036) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "014-c4.bin", 0x200001, 0x100000, CRC(e4002651) SHA1(17e53a5f4708866a120415bf24f3b89621ad0bcc) ) /* Plane 2,3 */
ROM_END

ROM_START( kotm )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "016-p1.bin", 0x000000, 0x080000, CRC(1b818731) SHA1(b98b1b33c0301fd79aac908f6b635dd00d1cb08d) )
	ROM_LOAD16_WORD_SWAP( "016-p2.bin", 0x080000, 0x020000, CRC(12afdc2b) SHA1(3a95f5910cbb9f17e63ddece995c6e120fa2f622) )

	NEO_SFIX_128K( "016-s1.bin", CRC(1a2eeeb3) SHA1(8d2b96d395020197bc59294b6b0c8d62b1d8d4dd) )

	NEO_BIOS_SOUND_128K( "016-m1.bin", CRC(0296abcb) SHA1(560046f256cd339fa685d0d38d55317cb6adfa99) )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "016-v1.bin", 0x000000, 0x100000, CRC(86c0a502) SHA1(7fe2db0c64aefdd14d6c36f7fcd6442591e9a014) )
	ROM_LOAD( "016-v2.bin", 0x100000, 0x100000, CRC(5bc23ec5) SHA1(f4ff5d20587469daa026d5c812739335ce53cfdf) )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "016-c1.bin", 0x000000, 0x100000, CRC(71471c25) SHA1(bc8e3fee56b33ef2bac5b4b852339d2fbcd09b7c) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "016-c2.bin", 0x000001, 0x100000, CRC(320db048) SHA1(d6b43834de6f5442e23ca8fb26b3a36e96790d8d) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "016-c3.bin", 0x200000, 0x100000, CRC(98de7995) SHA1(e33edf4d36c82196d2b474e37be180a05976f558) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "016-c4.bin", 0x200001, 0x100000, CRC(070506e2) SHA1(3a2ec365e1d87a9c5ce1ee9bea88402a8eef4ed7) ) /* Plane 2,3 */
ROM_END

ROM_START( kotmh )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "016-hp1.bin",0x000000, 0x080000, CRC(b774621e) SHA1(7684b2e07163aec68cd083ef1d8900f855f6cb42) )
	ROM_LOAD16_WORD_SWAP( "016-p2.bin", 0x080000, 0x020000, CRC(12afdc2b) SHA1(3a95f5910cbb9f17e63ddece995c6e120fa2f622) )

	NEO_SFIX_128K( "016-s1.bin", CRC(1a2eeeb3) SHA1(8d2b96d395020197bc59294b6b0c8d62b1d8d4dd) )

	NEO_BIOS_SOUND_128K( "016-m1.bin", CRC(0296abcb) SHA1(560046f256cd339fa685d0d38d55317cb6adfa99) )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "016-v1.bin", 0x000000, 0x100000, CRC(86c0a502) SHA1(7fe2db0c64aefdd14d6c36f7fcd6442591e9a014) )
	ROM_LOAD( "016-v2.bin", 0x100000, 0x100000, CRC(5bc23ec5) SHA1(f4ff5d20587469daa026d5c812739335ce53cfdf) )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "016-c1.bin", 0x000000, 0x100000, CRC(71471c25) SHA1(bc8e3fee56b33ef2bac5b4b852339d2fbcd09b7c) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "016-c2.bin", 0x000001, 0x100000, CRC(320db048) SHA1(d6b43834de6f5442e23ca8fb26b3a36e96790d8d) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "016-c3.bin", 0x200000, 0x100000, CRC(98de7995) SHA1(e33edf4d36c82196d2b474e37be180a05976f558) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "016-c4.bin", 0x200001, 0x100000, CRC(070506e2) SHA1(3a2ec365e1d87a9c5ce1ee9bea88402a8eef4ed7) ) /* Plane 2,3 */
ROM_END

ROM_START( sengoku )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "017-p1.bin", 0x000000, 0x080000, CRC(f8a63983) SHA1(7a10ecb2f0fd8315641374c065d2602107b09e72) )
	ROM_LOAD16_WORD_SWAP( "017-p2.bin", 0x080000, 0x020000, CRC(3024bbb3) SHA1(88892e1292dd60f35a76f9a22e623d4f0f9693cc) )

	NEO_SFIX_128K( "017-s1.bin", CRC(b246204d) SHA1(73dce64c61fb5bb7e836a8e60f081bb77d80d281) )

	NEO_BIOS_SOUND_128K( "017-m1.bin", CRC(9b4f34c6) SHA1(7f3a51f47fcbaa598f5c76bc66e2c53c8dfd852d) )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "017-v1.bin", 0x000000, 0x100000, CRC(23663295) SHA1(9374a5d9f3de8e6a97c11f07d8b4485ac9d55edb) )
	ROM_LOAD( "017-v2.bin", 0x100000, 0x100000, CRC(f61e6765) SHA1(1c9b287996947319eb3d288c3d82932cf01039db) )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "017-c1.bin", 0x000000, 0x100000, CRC(b4eb82a1) SHA1(79879e2ea78c07d04c88dc9a1ad59604b7a078be) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "017-c2.bin", 0x000001, 0x100000, CRC(d55c550d) SHA1(6110f693aa23710939c04153cf5af26493e4a03f) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "017-c3.bin", 0x200000, 0x100000, CRC(ed51ef65) SHA1(e8a8d86e24454948e51a75c883bc6e4091cbf820) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "017-c4.bin", 0x200001, 0x100000, CRC(f4f3c9cb) SHA1(8faafa89dbd0345218f71f891419d2e4e7578200) ) /* Plane 2,3 */
ROM_END

ROM_START( sengokh )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "017-p1h.bin", 0x000000, 0x080000, CRC(33eccae0) SHA1(000ccf9a9c73df75eeba3f2c367c3a1a9e0a3a6b) )
	ROM_LOAD16_WORD_SWAP( "017-p2.bin",  0x080000, 0x020000, CRC(3024bbb3) SHA1(88892e1292dd60f35a76f9a22e623d4f0f9693cc) )

	NEO_SFIX_128K( "017-s1.bin", CRC(b246204d) SHA1(73dce64c61fb5bb7e836a8e60f081bb77d80d281) )

	NEO_BIOS_SOUND_128K( "017-m1.bin", CRC(9b4f34c6) SHA1(7f3a51f47fcbaa598f5c76bc66e2c53c8dfd852d) )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "017-v1.bin", 0x000000, 0x100000, CRC(23663295) SHA1(9374a5d9f3de8e6a97c11f07d8b4485ac9d55edb) )
	ROM_LOAD( "017-v2.bin", 0x100000, 0x100000, CRC(f61e6765) SHA1(1c9b287996947319eb3d288c3d82932cf01039db) )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "017-c1.bin", 0x000000, 0x100000, CRC(b4eb82a1) SHA1(79879e2ea78c07d04c88dc9a1ad59604b7a078be) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "017-c2.bin", 0x000001, 0x100000, CRC(d55c550d) SHA1(6110f693aa23710939c04153cf5af26493e4a03f) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "017-c3.bin", 0x200000, 0x100000, CRC(ed51ef65) SHA1(e8a8d86e24454948e51a75c883bc6e4091cbf820) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "017-c4.bin", 0x200001, 0x100000, CRC(f4f3c9cb) SHA1(8faafa89dbd0345218f71f891419d2e4e7578200) ) /* Plane 2,3 */
ROM_END

ROM_START( burningf )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "018-p1.bin", 0x000000, 0x080000, CRC(4092c8db) SHA1(df194a4ad2c35e0e18bc053ff9284183444a4666) )

	NEO_SFIX_128K( "018-s1.bin", CRC(6799ea0d) SHA1(ec75ef9dfdcb0b123574fc6d81ebaaadfba32fb5) )

	NEO_BIOS_SOUND_128K( "018-m1.bin", CRC(0c939ee2) SHA1(57d580d3279e66b9fe66bbcc68529d3384a926ff) )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "018-v1.bin", 0x000000, 0x100000, CRC(508c9ffc) SHA1(cd3a97a233a4585f8507116aba85884623cccdc4) )
	ROM_LOAD( "018-v2.bin", 0x100000, 0x100000, CRC(854ef277) SHA1(4b3083b9c80620064cb44e812a787a700e32a6f3) )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "018-c1.bin", 0x000000, 0x100000, CRC(25a25e9b) SHA1(3cf02d0662e190678d0530d7b7d3f425209adf83) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "018-c2.bin", 0x000001, 0x100000, CRC(d4378876) SHA1(45659aa1755d96b992c977042186e47fff68bba9) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "018-c3.bin", 0x200000, 0x100000, CRC(862b60da) SHA1(e2303eb1609f1050f0b4f46693a15e37deb176fb) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "018-c4.bin", 0x200001, 0x100000, CRC(e2e0aff7) SHA1(1c691c092a6e2787de4f433b0eb9252bfdaa7e16) ) /* Plane 2,3 */
ROM_END

ROM_START( burningh )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "018-p1h.bin", 0x000000, 0x080000, CRC(ddffcbf4) SHA1(c646c4bbdb4e9b32df76c90f582ccd69fcc9f8e7) )

	NEO_SFIX_128K( "018-s1.bin", CRC(6799ea0d) SHA1(ec75ef9dfdcb0b123574fc6d81ebaaadfba32fb5) )

	NEO_BIOS_SOUND_128K( "018-m1.bin", CRC(0c939ee2) SHA1(57d580d3279e66b9fe66bbcc68529d3384a926ff) )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "018-v1.bin", 0x000000, 0x100000, CRC(508c9ffc) SHA1(cd3a97a233a4585f8507116aba85884623cccdc4) )
	ROM_LOAD( "018-v2.bin", 0x100000, 0x100000, CRC(854ef277) SHA1(4b3083b9c80620064cb44e812a787a700e32a6f3) )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "018-c1.bin", 0x000000, 0x100000, CRC(25a25e9b) SHA1(3cf02d0662e190678d0530d7b7d3f425209adf83) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "018-c2.bin", 0x000001, 0x100000, CRC(d4378876) SHA1(45659aa1755d96b992c977042186e47fff68bba9) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "018-c3.bin", 0x200000, 0x100000, CRC(862b60da) SHA1(e2303eb1609f1050f0b4f46693a15e37deb176fb) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "018-c4.bin", 0x200001, 0x100000, CRC(e2e0aff7) SHA1(1c691c092a6e2787de4f433b0eb9252bfdaa7e16) ) /* Plane 2,3 */
ROM_END

ROM_START( lbowling )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "019-p1.bin", 0x000000, 0x080000, CRC(a2de8445) SHA1(893d7ae72b4644123469de143fa35fac1cbcd61e) )

	NEO_SFIX_128K( "019-s1.bin", CRC(5fcdc0ed) SHA1(86415077e7adc3ba6153eeb4fb0c62cf36e903fa) )

	NEO_BIOS_SOUND_128K( "019-m1.bin", CRC(589d7f25) SHA1(9cffbf0a607a7c7f5bc21cf6d33c5b21c3354913) )

	ROM_REGION( 0x100000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "019-v11.bin", 0x000000, 0x080000, CRC(0fb74872) SHA1(38c555926c77576d63472bc075210c42e9ce13a3) )
	ROM_LOAD( "019-v12.bin", 0x080000, 0x080000, CRC(029faa57) SHA1(7bbaa87e38929ab1e32df5f6a2ec0fd5001e7cdb) )

	ROM_REGION( 0x080000, REGION_SOUND2, ROMREGION_SOUNDONLY )
	ROM_LOAD( "019-v21.bin", 0x000000, 0x080000, CRC(2efd5ada) SHA1(8ba70f5f665d566824333075227d9bce1253b8d8) )

	ROM_REGION( 0x100000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "019-c1.bin", 0x000000, 0x080000, CRC(4ccdef18) SHA1(5011e30ec235d0b0a5a513a11d4275777e61acdb) )
	ROM_LOAD16_BYTE( "019-c2.bin", 0x000001, 0x080000, CRC(d4dd0802) SHA1(82069752028c118d42384a95befde45844f0f247) )
ROM_END

ROM_START( gpilots )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "020-p1.bin", 0x000000, 0x080000, CRC(e6f2fe64) SHA1(50ab82517e077727d97668a4df2b9b96d2e78ab6) )
	ROM_LOAD16_WORD_SWAP( "020-p2.bin", 0x080000, 0x020000, CRC(edcb22ac) SHA1(505d2db38ae999b7d436e8f2ff56b81796d62b54) )

	NEO_SFIX_128K( "020-s1.bin", CRC(a6d83d53) SHA1(9a8c092f89521cc0b27a385aa72e29cbaca926c5) )

	NEO_BIOS_SOUND_128K( "020-m1.bin", CRC(48409377) SHA1(0e212d2c76856a90b2c2fdff675239525972ac43) )

	ROM_REGION( 0x180000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "020-v11.bin", 0x000000, 0x100000, CRC(1b526c8b) SHA1(2801868d2badcf8aaf5d490e010e4049d81d7bc1) )
	ROM_LOAD( "020-v12.bin", 0x100000, 0x080000, CRC(4a9e6f03) SHA1(d3ac11f333b03d8a318921bdaefb14598e289a14) )

	ROM_REGION( 0x080000, REGION_SOUND2, ROMREGION_SOUNDONLY )
	ROM_LOAD( "020-v21.bin", 0x000000, 0x080000, CRC(7abf113d) SHA1(5b2a0e70f2eaf4638b44702dacd4cb17838fb1d5) )

	ROM_REGION( 0x400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "020-c1.bin", 0x000000, 0x100000, CRC(bd6fe78e) SHA1(50b704862cd79d64fa488e621b079f6e413c33bc) )
	ROM_LOAD16_BYTE( "020-c2.bin", 0x000001, 0x100000, CRC(5f4a925c) SHA1(71c5ef8141234daaa7025427a6c65e79766973a5) )
	ROM_LOAD16_BYTE( "020-c3.bin", 0x200000, 0x100000, CRC(d1e42fd0) SHA1(f0d476aebbdc2ce008f5f0783be86d295b24aa44) )
	ROM_LOAD16_BYTE( "020-c4.bin", 0x200001, 0x100000, CRC(edde439b) SHA1(79be7b10ecdab54c2f77062b8f5fda0e299fa982) )
ROM_END

ROM_START( joyjoy )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "021-p1.bin", 0x000000, 0x080000, CRC(39c3478f) SHA1(06ebe54c9c4e14c5c31e770013d58b7162359ecc) )

	NEO_SFIX_128K( "021-s1.bin", CRC(6956d778) SHA1(e3757776d60dc07d8e07c9ca61b223b14732f860) )

	NEO_BIOS_SOUND_64K( "021-m1.bin", CRC(058683ec) SHA1(2aff3dc18d078911ed3a09d48664faf5958b4ab5) )

	ROM_REGION( 0x080000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "021-v1.bin", 0x000000, 0x080000, CRC(66c1e5c4) SHA1(7e85420021d4c39c36ed75a1cec567c5610ffce0) )

	ROM_REGION( 0x080000, REGION_SOUND2, ROMREGION_SOUNDONLY )
	ROM_LOAD( "021-v2.bin", 0x000000, 0x080000, CRC(8ed20a86) SHA1(d15cba5eac19ea56fdd4877541f1bb3eb755ebba) )

	ROM_REGION( 0x100000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "021-c1.bin", 0x000000, 0x080000, CRC(509250ec) SHA1(d6ddb16d8222088f153a85a905bcb99541a5f2cf) )
	ROM_LOAD16_BYTE( "021-c2.bin", 0x000001, 0x080000, CRC(09ed5258) SHA1(6bf50cd10236e29146b49e714a0e0ebcfe30a682) )
ROM_END

ROM_START( bjourney )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "022-p1.bin", 0x000000, 0x100000, CRC(6a2f6d4a) SHA1(b8ca548e56f1c7abcdce415ba7329e0cf698ee13) )

	NEO_SFIX_128K( "022-s1.bin", CRC(843c3624) SHA1(dbdf86c193b7c1d795f8c21f2c103c1d3e18abbe) )

	NEO_BIOS_SOUND_64K( "022-m1.bin",  CRC(a9e30496) SHA1(c7a42dc64007408fd9b3b9d3a54c523da3acf475) )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "022-v1.bin", 0x000000, 0x100000, CRC(2cb4ad91) SHA1(169ec7303c4275155a66a88cc08270c24132bb36) )
	ROM_LOAD( "022-v2.bin", 0x100000, 0x100000, CRC(65a54d13) SHA1(a591fbcedca8f679dacbebcd554e3aa3fd163e92) )

	NO_DELTAT_REGION

	ROM_REGION( 0x300000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "022-c1.bin", 0x000000, 0x100000, CRC(4d47a48c) SHA1(6e282285be72583d828e7765b1c1695ecdc44777) )
	ROM_LOAD16_BYTE( "022-c2.bin", 0x000001, 0x100000, CRC(e8c1491a) SHA1(c468d2556b3de095aaa05edd1bc16d71303e9478) )
	ROM_LOAD16_BYTE( "022-c3.bin", 0x200000, 0x080000, CRC(66e69753) SHA1(974b823fc62236fbc23e727f25b61a805a707a9e) )
	ROM_LOAD16_BYTE( "022-c4.bin", 0x200001, 0x080000, CRC(71bfd48a) SHA1(47288be69e6992d09ebef108b4de9ffab6293dc8) )
ROM_END

ROM_START( quizdais )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "023-p1.bin", 0x000000, 0x100000, CRC(c488fda3) SHA1(4cdf2f1837fffd720efef42f81f933bdf2ef1402) )

	NEO_SFIX_128K( "023-s1.bin", CRC(ac31818a) SHA1(93c8d67a93606a2e02f12ca4cab849dc3f3de286) )

	NEO_BIOS_SOUND_128K( "023-m1.bin", CRC(2a2105e0) SHA1(26fc13556fda2dbeb7b5b035abd994e302dc7662) )

	ROM_REGION( 0x100000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "023-v1.bin", 0x000000, 0x100000, CRC(a53e5bd3) SHA1(cf115c6478ce155d889e6a5acb962339e08e024b) )

	NO_DELTAT_REGION

	ROM_REGION( 0x200000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "023-c1.bin", 0x000000, 0x100000, CRC(2999535a) SHA1(0deabf771039987b559edc2444eea741bd7ba861) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "023-c2.bin", 0x000001, 0x100000, CRC(876a99e6) SHA1(8d1dcfc0927d7523f8be8203573192406ec654b4) ) /* Plane 2,3 */
ROM_END

ROM_START( lresort )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "024-p1.bin", 0x000000, 0x080000, CRC(89c4ab97) SHA1(3a1817c427185ea1b44fe52f009c00b0a9007c85) )

	NEO_SFIX_128K( "024-s1.bin", CRC(5cef5cc6) SHA1(9ec305007bdb356e9f8f279beae5e2bcb3f2cf7b) )

	NEO_BIOS_SOUND_128K( "024-m1.bin", CRC(cec19742) SHA1(ab6c6ba7737e68d2420a0617719c6d4c89039c45) )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "024-v1.bin", 0x000000, 0x100000, CRC(efdfa063) SHA1(e4609ecbcc1c820758f229da5145f51285b50555) )
	ROM_LOAD( "024-v2.bin", 0x100000, 0x100000, CRC(3c7997c0) SHA1(8cb7e8e69892b19d318978370dbc510d51b06a69) )

	NO_DELTAT_REGION

	ROM_REGION( 0x300000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "024-c1.bin", 0x000000, 0x100000, CRC(3617c2dc) SHA1(8de2643a618272f8aa1c705363edb007f4a5f5b7) )
	ROM_LOAD16_BYTE( "024-c2.bin", 0x000001, 0x100000, CRC(3f0a7fd8) SHA1(d0c9c7a9dde9ce175fb243d33ec11fa719d0158c) )
	ROM_LOAD16_BYTE( "024-c3.bin", 0x200000, 0x080000, CRC(e9f745f8) SHA1(bbe6141da28b0db7bf5cf321d69b7e613e2414d7) )
	ROM_LOAD16_BYTE( "024-c4.bin", 0x200001, 0x080000, CRC(7382fefb) SHA1(e916dec5bb5462eb9ae9711f08c7388937abb980) )
ROM_END

ROM_START( eightman )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "025-p1.bin", 0x000000, 0x080000, CRC(43344cb0) SHA1(29dfd699f35b0a74e20fedd6c9174c289f0ef6e0) )

	NEO_SFIX_128K( "025-s1.bin", CRC(a402202b) SHA1(75c44e1af459af155f5b892fd18706268dd5e602) )

	NEO_BIOS_SOUND_128K( "025-m1.bin", CRC(9927034c) SHA1(205665361c5b2ab4f01ec480dd3c9b69db858d09) )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "025-v1.bin", 0x000000, 0x100000, CRC(4558558a) SHA1(a4b277703ed67225c652be0d618daeca65a27b88) )
	ROM_LOAD( "025-v2.bin", 0x100000, 0x100000, CRC(c5e052e9) SHA1(fa1119c90ce4c706a6aa0c17d7bc06aa3068d9b2) )

	NO_DELTAT_REGION

	ROM_REGION( 0x300000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "025-c1.bin", 0x000000, 0x100000, CRC(555e16a4) SHA1(1c96f3d2fd0991680fbf627a6cdd26ad2cd60319) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "025-c2.bin", 0x000001, 0x100000, CRC(e1ee51c3) SHA1(da8d074bb4e923ed7b8a154fd31b42f2d65b8e96) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "025-c3.bin", 0x200000, 0x080000, CRC(0923d5b0) SHA1(ab72ba1e3ebf56dd356f9ad181f986b1360a1089) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "025-c4.bin", 0x200001, 0x080000, CRC(e3eca67b) SHA1(88154cbc1a261c2f425430119ebc08a30adc9675) ) /* Plane 2,3 */
ROM_END

ROM_START( minasan )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "027-p1.bin", 0x000000, 0x080000, CRC(c8381327) SHA1(c8f8be0ba276c6d12ef13d05af3cf83a2b924894) )

	NEO_SFIX_128K( "027-s1.bin", CRC(e5824baa) SHA1(8230ff7fe3cabeacecc762d90a084e893db84906) )

	NEO_BIOS_SOUND_128K( "027-m1.bin", CRC(add5a226) SHA1(99995bef2584abbba16777bac52f55523f7aa97d) )

	ROM_REGION( 0x100000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "027-v11.bin", 0x000000, 0x100000, CRC(59ad4459) SHA1(bbb8ba8a8e337dd2946eefda4757e80d0547d54a) )

	ROM_REGION( 0x100000, REGION_SOUND2, ROMREGION_SOUNDONLY )
	ROM_LOAD( "027-v21.bin", 0x000000, 0x100000, CRC(df5b4eeb) SHA1(134f3bcc3bb82e2a5711496af1019f343f9c0f7e) )

	ROM_REGION( 0x400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "027-c1.bin", 0x000000, 0x100000, CRC(d0086f94) SHA1(7d6579530ccb5188f653be69b1df17e47e40e7a6) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "027-c2.bin", 0x000001, 0x100000, CRC(da61f5a6) SHA1(82c5b4e5c5c5e30a3fd1c2e11c6157f39d033c42) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "027-c3.bin", 0x200000, 0x100000, CRC(08df1228) SHA1(288b7ad328c2249f28d17df4dad3584995dca7bf) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "027-c4.bin", 0x200001, 0x100000, CRC(54e87696) SHA1(90816dc86be3983dc57f56ededf7738475c0c61e) ) /* Plane 2,3 */
ROM_END

ROM_START( legendos )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "029-p1.bin", 0x000000, 0x080000, CRC(9d563f19) SHA1(9bff7bf9fdcf81a0a6c4ce3e196097d4f05e67b6) )

	NEO_SFIX_128K( "029-s1.bin",  CRC(bcd502f0) SHA1(a3400f52c037aa6a42e59e602cc24fa45fcbc951) )

	NEO_BIOS_SOUND_64K( "029-m1.bin", CRC(909d4ed9) SHA1(5230b423b3f629bb955a5b2dab7e502fa7d83254) )

	ROM_REGION( 0x100000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "029-v1.bin", 0x000000, 0x100000, CRC(85065452) SHA1(7154b7c59b16c32753ac6b5790fb50b51ce30a20) )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "029-c1.bin", 0x000000, 0x100000, CRC(2f5ab875) SHA1(3e060973bba41a6c22ff7054104bdc5eee1fa13a) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "029-c2.bin", 0x000001, 0x100000, CRC(318b2711) SHA1(7014110cee98280317e1189f306ca40652b61f6f) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "029-c3.bin", 0x200000, 0x100000, CRC(6bc52cb2) SHA1(14323a4664b7dcbcde82e594168e535d7a921e44) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "029-c4.bin", 0x200001, 0x100000, CRC(37ef298c) SHA1(7a0c4c896dc3e730e06dcadbf00cf354f08a4466) ) /* Plane 2,3 */
ROM_END

ROM_START( 2020bb )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "030-p1.bin", 0x000000, 0x080000, CRC(d396c9cb) SHA1(47ba421d14d05b965a8d44e7475b227a208e5a07) )

	NEO_SFIX_128K( "030-s1.bin", CRC(7015b8fc) SHA1(8c09bc3e6c62e0f7c9557c1e10c901be325bae7f) )

	NEO_BIOS_SOUND_128K( "030-m1.bin", CRC(4cf466ec) SHA1(6a003b53c7a4af9d7529e2c10f27ffc4e58dcda5) )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "030-v1.bin", 0x000000, 0x100000, CRC(d4ca364e) SHA1(b0573744b0ea2ef1e2167a225f0d254883f5af04) )
	ROM_LOAD( "030-v2.bin", 0x100000, 0x100000, CRC(54994455) SHA1(76eb62b86e8ed51a77f44313d5cc8091b3f58d57) )

	NO_DELTAT_REGION

	ROM_REGION( 0x300000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "030-c1.bin", 0x000000, 0x100000, CRC(4f5e19bd) SHA1(ef7975c4b33a7aea4a25a385f604799f054d3200) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "030-c2.bin", 0x000001, 0x100000, CRC(d6314bf0) SHA1(0920cc580d7997fcb0170dd619af2f305d635577) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "030-c3.bin", 0x200000, 0x080000, CRC(6a87ae30) SHA1(b2ca080d8233f43cfb2e60d894af65c7b3f8b809) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "030-c4.bin", 0x200001, 0x080000, CRC(bef75dd0) SHA1(8df572804d36d6b25d94b64e8be17a42babbbe95) ) /* Plane 2,3 */
ROM_END

ROM_START( 2020bbh )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "030-p1h.bin", 0x000000, 0x080000, CRC(12d048d7) SHA1(ee0d03a565b11ca3bee2d24f62ff46a85ef18d90) )

	NEO_SFIX_128K( "030-s1.bin", CRC(7015b8fc) SHA1(8c09bc3e6c62e0f7c9557c1e10c901be325bae7f) )

	NEO_BIOS_SOUND_128K( "030-m1.bin", CRC(4cf466ec) SHA1(6a003b53c7a4af9d7529e2c10f27ffc4e58dcda5) )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "030-v1.bin", 0x000000, 0x100000, CRC(d4ca364e) SHA1(b0573744b0ea2ef1e2167a225f0d254883f5af04) )
	ROM_LOAD( "030-v2.bin", 0x100000, 0x100000, CRC(54994455) SHA1(76eb62b86e8ed51a77f44313d5cc8091b3f58d57) )

	NO_DELTAT_REGION

	ROM_REGION( 0x300000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "030-c1.bin", 0x000000, 0x100000, CRC(4f5e19bd) SHA1(ef7975c4b33a7aea4a25a385f604799f054d3200) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "030-c2.bin", 0x000001, 0x100000, CRC(d6314bf0) SHA1(0920cc580d7997fcb0170dd619af2f305d635577) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "030-c3.bin", 0x200000, 0x080000, CRC(6a87ae30) SHA1(b2ca080d8233f43cfb2e60d894af65c7b3f8b809) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "030-c4.bin", 0x200001, 0x080000, CRC(bef75dd0) SHA1(8df572804d36d6b25d94b64e8be17a42babbbe95) ) /* Plane 2,3 */
ROM_END

ROM_START( socbrawl )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "031-p1.bin", 0x000000, 0x080000, CRC(a2801c24) SHA1(627d76ff0740ca29586f37b268f47fb469822529) )

	NEO_SFIX_64K( "031-s1.bin", CRC(2db38c3b) SHA1(8f23b32f3abd3faed0ca238938ce6a2b6d75ee10) )

	NEO_BIOS_SOUND_64K( "031-m1.bin", CRC(2f38d5d3) SHA1(0fc01750277e554978e68e7e6d596f8bd6b1e178) )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "031-v1.bin", 0x000000, 0x100000, CRC(cc78497e) SHA1(895bd647150fae9b2259ef043ed681f4c4de66ea) )
	ROM_LOAD( "031-v2.bin", 0x100000, 0x100000, CRC(dda043c6) SHA1(08165a59700ab6b1e523079dd2a3549e520cc594) )

	NO_DELTAT_REGION

	ROM_REGION( 0x300000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "031-c1.bin", 0x000000, 0x100000, CRC(bd0a4eb8) SHA1(b67988cb3e550d083e81c9bd436da55b242785ed) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "031-c2.bin", 0x000001, 0x100000, CRC(efde5382) SHA1(e42789c8d87ee3d4549d0a903e990c03338cbbd8) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "031-c3.bin", 0x200000, 0x080000, CRC(580f7f33) SHA1(f4f95a7c8de00e1366a723fc4cd0e8c1905af636) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "031-c4.bin", 0x200001, 0x080000, CRC(ed297de8) SHA1(616f8fa4c86231f3e79faf9f69f8bb909cbc35f0) ) /* Plane 2,3 */
ROM_END

ROM_START( roboarmy )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "032-p1.bin", 0x000000, 0x080000, CRC(cd11cbd4) SHA1(23163e3da2f07e830a7f4a02aea1cb01a54ccbf3) )

	NEO_SFIX_128K( "032-s1.bin", CRC(ac0daa1b) SHA1(93bae4697dc403fce19422752a514326ccf66a91) )

	NEO_BIOS_SOUND_128K( "032-m1.bin", CRC(98edc671) SHA1(04ed65ccaa1197fa55fcefe2caed2772e7040bdb) )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "032-v1.bin", 0x000000, 0x080000, CRC(daff9896) SHA1(2f8a39b98ff2f4a0c8901b1befcc69e0cc5f5aed) )
	ROM_LOAD( "032-v2.bin", 0x080000, 0x080000, CRC(8781b1bc) SHA1(4e3cc22ac0acd4033cd4a327269c68b5f56bbe34) )
	ROM_LOAD( "032-v3.bin", 0x100000, 0x080000, CRC(b69c1da5) SHA1(7d5ea49338aeef711fc64755abed51fcdd939d48) )
	ROM_LOAD( "032-v4.bin", 0x180000, 0x080000, CRC(2c929c17) SHA1(7e053035573de9f601de80d200d09ed4844506fe) )

	NO_DELTAT_REGION

	ROM_REGION( 0x300000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "032-c1.bin", 0x000000, 0x080000, CRC(e17fa618) SHA1(14347886b776c24d2dd8b19353ad8897d5f7e56c) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "032-c2.bin", 0x000001, 0x080000, CRC(d5ebdb4d) SHA1(cc811af611cc528fd3a9d1bdd8ab427fe9fea693) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "032-c3.bin", 0x100000, 0x080000, CRC(aa4d7695) SHA1(2d6c5b4f6fce82b5800fae17b4a94cf8a41216f4) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "032-c4.bin", 0x100001, 0x080000, CRC(8d4ebbe3) SHA1(384ee64db1726b0aef2d3ce8b1d914b56e7925d9) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "032-c5.bin", 0x200000, 0x080000, CRC(40adfccd) SHA1(b11f866dd70ba0ed9123424508355cb948b19bdc) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "032-c6.bin", 0x200001, 0x080000, CRC(462571de) SHA1(5c3d610d492f91564423873b3b434dcda700373f) ) /* Plane 2,3 */
ROM_END

ROM_START( fatfury1 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "033-p1.bin", 0x000000, 0x080000, CRC(47ebdc2f) SHA1(d46786502920fb510f1999db00c5e09fb641c0bd) )
	ROM_LOAD16_WORD_SWAP( "033-p2.bin", 0x080000, 0x020000, CRC(c473af1c) SHA1(4919eeca20abe807493872ca7c79a5d1f496fe68) )

	NEO_SFIX_128K( "033-s1.bin", CRC(3c3bdf8c) SHA1(2f3e5feed6c27850b2a0f6fae0b97041690e944c) )

	NEO_BIOS_SOUND_128K( "033-m1.bin", CRC(a8603979) SHA1(512f2280a43892d4ca003ac63945ce86c5211b97) )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "033-v1.bin", 0x000000, 0x100000, CRC(212fd20d) SHA1(120c040db8c01a6f140eea03725448bfa9ca98c2) )
	ROM_LOAD( "033-v2.bin", 0x100000, 0x100000, CRC(fa2ae47f) SHA1(80d0ba4cd30aab59b6f0db8fa341387bd7388afc) )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "033-c1.bin", 0x000000, 0x100000, CRC(74317e54) SHA1(67b9c2814a12603b959612456f59de55f9bf6f57) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "033-c2.bin", 0x000001, 0x100000, CRC(5bb952f3) SHA1(ea964bbcc0408b6ae07cbb5043d003281b1aca15) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "033-c3.bin", 0x200000, 0x100000, CRC(9b714a7c) SHA1(b62bdcede3207d062a89e0a4a9adf706101bb681) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "033-c4.bin", 0x200001, 0x100000, CRC(9397476a) SHA1(a12dbb74020aeb6ebf24ec2abbfba5129cabcb7d) ) /* Plane 2,3 */
ROM_END

ROM_START( fbfrenzy )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "034-p1.bin", 0x000000, 0x080000, CRC(cdef6b19) SHA1(97482db0dffc6d625fb41fa38449c0a74d741a72) )

	NEO_SFIX_128K( "034-s1.bin", CRC(8472ed44) SHA1(42e1a9671dddd090d2a634cff986f6c73ba08b70) )

	NEO_BIOS_SOUND_128K( "034-m1.bin", CRC(f41b16b8) SHA1(f3e1cfc4cd2c5baece176f169906aa796367d303) )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "034-v1.bin", 0x000000, 0x100000, CRC(50c9d0dd) SHA1(2b3f2875b00e5f307d274128bd73c1521a7d901b) )
	ROM_LOAD( "034-v2.bin", 0x100000, 0x100000, CRC(5aa15686) SHA1(efe47954827a98d539ba719347c5f8aa60e6338b) )

	NO_DELTAT_REGION

	ROM_REGION( 0x300000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "034-c1.bin", 0x000000, 0x100000, CRC(91c56e78) SHA1(2944d49ebfc71239d345209ca7f25993c2cc5a77) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "034-c2.bin", 0x000001, 0x100000, CRC(9743ea2f) SHA1(cf4fccdf10d521d555e92bc24123142393c2b3bb) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "034-c3.bin", 0x200000, 0x080000, CRC(e5aa65f5) SHA1(714356a2cee976ec0f515b1034ce971018e5c02e) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "034-c4.bin", 0x200001, 0x080000, CRC(0eb138cc) SHA1(21d31e1f136c674caa6dd44073281cd07b72ea9b) ) /* Plane 2,3 */
ROM_END

ROM_START( bakatono )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "036-p1.bin", 0x000000, 0x080000, CRC(1c66b6fa) SHA1(6c50cc452971c46c763ae0b2def95792671a1798) )

	NEO_SFIX_128K( "036-s1.bin", CRC(f3ef4485) SHA1(c30bfceed7e669e4c97b0b3ec2e9f4271e5b6662) )

	NEO_BIOS_SOUND_128K( "036-m1.bin", CRC(f1385b96) SHA1(e7e3d1484188a115e262511116aaf466b8b1f428) )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "036-v1.bin", 0x000000, 0x100000, CRC(1c335dce) SHA1(493c273fa71bf81861a20af4c4eaae159e169f39) )
	ROM_LOAD( "036-v2.bin", 0x100000, 0x100000, CRC(bbf79342) SHA1(45a4f40e415cdf35c3073851506648c8f7d53958) )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "036-c1.bin", 0x000000, 0x100000, CRC(fe7f1010) SHA1(5b6f5053821f4da8dc3768371e2cd51bb29da963) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "036-c2.bin", 0x000001, 0x100000, CRC(bbf003f5) SHA1(054b2a3327e038836eece652055bb84c115cf8ed) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "036-c3.bin", 0x200000, 0x100000, CRC(9ac0708e) SHA1(8decfe06d73a3dd3c3cf280719978fcf6d559d29) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "036-c4.bin", 0x200001, 0x100000, CRC(f2577d22) SHA1(a37db8055ca4680e244c556dc6df8bdba16c2083) ) /* Plane 2,3 */
ROM_END

ROM_START( crsword )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "037-p1.bin", 0x000000, 0x080000, CRC(e7f2553c) SHA1(8469ecb900477feed05ae3311fe9515019bbec2a) )

	NEO_SFIX_128K( "037-s1.bin", CRC(74651f27) SHA1(bff7ff2429d2be82c1647abac2ee45b339b3b310) )

	NEO_BIOS_SOUND_128K( "037-m1.bin", CRC(9c384263) SHA1(74b86284048669e316c5d241b4aaeb02d59d4dfa) )

	ROM_REGION( 0x100000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "037-v1.bin",  0x000000, 0x100000, CRC(61fedf65) SHA1(98f31d1e23bf7c1f7844e67f14707a704134042e) )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "037-c1.bin", 0x000000, 0x100000, CRC(09df6892) SHA1(df2579dcf9c9dc88d461212cb74de106be2983c1) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "037-c2.bin", 0x000001, 0x100000, CRC(ac122a78) SHA1(7bfa4d29b7d7d9443f64d81caeafa74fe05c606e) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "037-c3.bin", 0x200000, 0x100000, CRC(9d7ed1ca) SHA1(2bbd25dc3a3f825d0af79a418f06a23a1bf03cc0) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "037-c4.bin", 0x200001, 0x100000, CRC(4a24395d) SHA1(943f911f40985db901eaef4c28dfcda299fca73e) ) /* Plane 2,3 */
ROM_END

ROM_START( trally )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "038-p1.bin", 0x000000, 0x080000, CRC(1e52a576) SHA1(a1cb56354c3378e955b0cd482c3c41ae15add952) )
	ROM_LOAD16_WORD_SWAP( "038-p2.bin", 0x080000, 0x080000, CRC(a5193e2f) SHA1(96803480439e90da23cdca70d59ff519ee85beeb) )

	NEO_SFIX_128K( "038-s1.bin", CRC(fff62ae3) SHA1(6510a762ea41557a8938cbfc0557cd5921306061) )

	NEO_BIOS_SOUND_128K( "038-m1.bin", CRC(0908707e) SHA1(df7489ea6abf84d7f137ba7a8f52a4fd1b088fd7) )

	ROM_REGION( 0x180000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "038-v1.bin", 0x000000, 0x100000, CRC(5ccd9fd5) SHA1(c3c8c758a320c39e4ceb0b6d9f188ed6d122eec4) )
	ROM_LOAD( "038-v2.bin", 0x100000, 0x080000, CRC(ddd8d1e6) SHA1(65c819fa2392f264f5a1a0a4967c96775732500b) )

	NO_DELTAT_REGION

	ROM_REGION( 0x300000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "038-c1.bin", 0x000000, 0x100000, CRC(c58323d4) SHA1(a6bd277471a4b612d165f8b804f3cb662f499b70) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "038-c2.bin", 0x000001, 0x100000, CRC(bba9c29e) SHA1(b70bbfdfa8c4f9ea76406530e86b16e42498d284) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "038-c3.bin", 0x200000, 0x080000, CRC(3bb7b9d6) SHA1(bc1eae6181ad5abf79736afc8db4ca34113d43f8) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "038-c4.bin", 0x200001, 0x080000, CRC(a4513ecf) SHA1(934aa103c226eac55157b44d7b4dfa35515322c3) ) /* Plane 2,3 */
ROM_END

ROM_START( kotm2 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "039-p1.bin", 0x000000, 0x080000, CRC(b372d54c) SHA1(b70fc6f72e16a66b6e144cc01370548e3398b8b8) )
	ROM_LOAD16_WORD_SWAP( "039-p2.bin", 0x080000, 0x080000, CRC(28661afe) SHA1(6c85ff6ab334b1ca744f726f42dac211537e7315) )

	NEO_SFIX_128K( "039-s1.bin", CRC(63ee053a) SHA1(7d4b92bd022708975b1470e8f24d1f5a712e1b94) )

	NEO_BIOS_SOUND_128K( "039-m1.bin", CRC(0c5b2ad5) SHA1(15eb5ea10fecdbdbcfd06225ae6d88bb239592e7) )

	ROM_REGION( 0x300000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "039-v1.bin", 0x000000, 0x200000, CRC(86d34b25) SHA1(89bdb614b0c63d678962da52e2f596750d20828c) )
	ROM_LOAD( "039-v2.bin", 0x200000, 0x100000, CRC(8fa62a0b) SHA1(58ac2fdd73c542eb8178cfc4adfa0e5940183283) )

	NO_DELTAT_REGION

	ROM_REGION( 0x800000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "039-c1.bin", 0x000000, 0x100000, CRC(6d1c4aa9) SHA1(4fbc9d7cb37522ec298eefbe38c75a2d050fbb4a) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x400000, 0x100000 )
	ROM_LOAD16_BYTE( "039-c2.bin", 0x000001, 0x100000, CRC(f7b75337) SHA1(4d85f85948c3e6ed38b0b0ccda79de3ce026e2d9) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x400001, 0x100000 )
	ROM_LOAD16_BYTE( "039-c3.bin", 0x200000, 0x100000, CRC(40156dca) SHA1(909b04757f1b90f225dde3a4c65c6a7d3e0d7289) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x600000, 0x100000 )
	ROM_LOAD16_BYTE( "039-c4.bin", 0x200001, 0x100000, CRC(b0d44111) SHA1(9ef02149c87aeeb9fca611e57139e3ccf9ae72cd) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x600001, 0x100000 )
ROM_END

ROM_START( sengoku2 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "040-p1.bin", 0x000000, 0x080000, CRC(cc245299) SHA1(5dd4b017328526c485e31f6ca28a2f3bc2cef778) )
	ROM_LOAD16_WORD_SWAP( "040-p2.bin", 0x080000, 0x080000, CRC(2e466360) SHA1(1b26e7a1dad52b3f1973b86b15360903eb170521) )

	NEO_SFIX_128K( "040-s1.bin", CRC(cd9802a3) SHA1(f685d4638f4f68e7e3f101c0c39128454536721b) )

	NEO_BIOS_SOUND_128K( "040-m1.bin", CRC(9902dfa2) SHA1(af6284c5298328156726b76b968995ad25fdf4de) )

	ROM_REGION( 0x300000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "040-v1.bin", 0x000000, 0x100000, CRC(b3725ced) SHA1(4e62b0d3dfe473cf49dc710829552bdc59547a83) )
	ROM_LOAD( "040-v2.bin", 0x100000, 0x100000, CRC(b5e70a0e) SHA1(6b3dfb71a32dc75276fa33183c78280f4457b967) )
	ROM_LOAD( "040-v3.bin", 0x200000, 0x100000, CRC(c5cece01) SHA1(923a3377dac1919e8c3d9ab316902250caa4785f) )

	NO_DELTAT_REGION

	ROM_REGION( 0x800000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "040-c1.bin", 0x000000, 0x200000, CRC(3cacd552) SHA1(37b005988c56e85ac3649af13378c68202311cd5) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "040-c2.bin", 0x000001, 0x200000, CRC(e2aadef3) SHA1(45835db57547b48641337b5c7ce9e8a140d71ca0) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "040-c3.bin", 0x400000, 0x200000, CRC(037614d5) SHA1(af86e1f35b1bb718906608c01abec35cd60c4e61) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "040-c4.bin", 0x400001, 0x200000, CRC(e9947e5b) SHA1(06fec16539f410fe27ca8c353183aed282205903) ) /* Plane 2,3 */
ROM_END

ROM_START( bstars2 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "041-p1.bin", 0x000000, 0x080000, CRC(523567fd) SHA1(f1e81eb4678f586b214ea102cde6effea1b0f768) )

	NEO_SFIX_128K( "041-s1.bin", CRC(015c5c94) SHA1(f1c60cd3dc54986b39f630ef3bf48f68c68695dc) )

	NEO_BIOS_SOUND_64K( "041-m1.bin", CRC(b2611c03) SHA1(a2c8b850b53e445edabdfd44f05c64de596618b8) )

	ROM_REGION( 0x280000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "041-v1.bin", 0x000000, 0x100000, CRC(cb1da093) SHA1(4f4d1d5fefa9dda372083c045bf0d268a57ce8f1) )
	ROM_LOAD( "041-v2.bin", 0x100000, 0x100000, CRC(1c954a9d) SHA1(159bc6efdd531615461f6e16f83f6d4c4e67c237) )
	ROM_LOAD( "041-v3.bin", 0x200000, 0x080000, CRC(afaa0180) SHA1(c4a047e21f093830498a163598ed7bd48a8cf9d1) )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "041-c1.bin", 0x000000, 0x100000, CRC(b39a12e1) SHA1(bafe383bd7c5a6aac4cb92dabbc56e3672fe174d) )
	ROM_LOAD16_BYTE( "041-c2.bin", 0x000001, 0x100000, CRC(766cfc2f) SHA1(79e1063925d54a57df943019a88bea56c9152df3) )
	ROM_LOAD16_BYTE( "041-c3.bin", 0x200000, 0x100000, CRC(fb31339d) SHA1(f4e821299680970b2e979acc4a170029b968c807) )
	ROM_LOAD16_BYTE( "041-c4.bin", 0x200001, 0x100000, CRC(70457a0c) SHA1(a1e307f11ddab85d2e9c09d0428fac2e6da774b1) )
ROM_END

ROM_START( quizdai2 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "042-p1.bin", 0x000000, 0x100000, CRC(ed719dcf) SHA1(12baf2601e86c0e4358517b9fa1c55f2f5835f1d) )

	NEO_SFIX_128K( "042-s1.bin", CRC(164fd6e6) SHA1(dad35bedc33d502a5ae745a45a972af8d901b160) )

	NEO_BIOS_SOUND_128K( "042-m1.bin", CRC(bb19995d) SHA1(ed458fad5a23c6bd0d099927d98c31e1e6562d1b) )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "042-v1.bin", 0x000000, 0x100000, CRC(af7f8247) SHA1(99a47014017c20e4e22010c60612b6b7f6efc9e5) )
	ROM_LOAD( "042-v2.bin", 0x100000, 0x100000, CRC(c6474b59) SHA1(a6c5054032b698116247b2f09a8b94a1b588c4f1) )

	NO_DELTAT_REGION

	ROM_REGION( 0x300000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "042-c1.bin", 0x000000, 0x100000, CRC(cb5809a1) SHA1(b53d06685246dd51b82b5c1d54d639d10e2ec26d) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "042-c2.bin", 0x000001, 0x100000, CRC(1436dfeb) SHA1(27d136fb1be793bd345a741f5e55a977275fff86) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "042-c3.bin", 0x200000, 0x080000, CRC(bcd4a518) SHA1(f355298fe0f2cf50ddcc0d613db56a5c04d7230f) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "042-c4.bin", 0x200001, 0x080000, CRC(d602219b) SHA1(34cf0f16db1e224396464ac838f4cd2e6d1c640e) ) /* Plane 2,3 */
ROM_END

ROM_START( 3countb )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "043-p1.bin", 0x000000, 0x080000, CRC(eb2714c4) SHA1(ba5dbfee4160f27ff36060b777d64b93dc2e929c) )
	ROM_LOAD16_WORD_SWAP( "043-p2.bin", 0x080000, 0x080000, CRC(5e764567) SHA1(3e34d051e8e1483073e4fcce0b688e50a1e713ce) )

	NEO_SFIX_128K( "043-s1.bin", CRC(c362d484) SHA1(a3c029292572842feabe9aa8c3372628fb63978d) )

	NEO_BIOS_SOUND_128K( "043-m1.bin", CRC(3377cda3) SHA1(5712cd0717585914120ebf307391e1e3171f5396) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "043-v1.bin", 0x000000, 0x200000, CRC(63688ce8) SHA1(5c6ac29a0cc0655a87cfe3ada8706838b86b86e4) )
	ROM_LOAD( "043-v2.bin", 0x200000, 0x200000, CRC(c69a827b) SHA1(f5197ea87bb6573fa6aef3a1713c3679c58c1e74) )

	NO_DELTAT_REGION

	ROM_REGION( 0x0800000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "043-c1.bin", 0x0000000, 0x200000, CRC(d290cc33) SHA1(f794e86da80930b273a134b82d39c6a8887f5228) )
	ROM_LOAD16_BYTE( "043-c2.bin", 0x0000001, 0x200000, CRC(0b28095d) SHA1(9f8184bab13939d2ef80e007462ed083c6cdd46f) )
	ROM_LOAD16_BYTE( "043-c3.bin", 0x0400000, 0x200000, CRC(bcc0cb35) SHA1(8bbee3201212be19ab53598c8663205cf27b6b4e) )
	ROM_LOAD16_BYTE( "043-c4.bin", 0x0400001, 0x200000, CRC(4d1ff7b9) SHA1(469fda8aaca673cb25b6b3c5534d78a990140d38) )
ROM_END

ROM_START( aof )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "044-p1.bin", 0x000000, 0x080000, CRC(ca9f7a6d) SHA1(4d28ef86696f7e832510a66d3e8eb6c93b5b91a1) )

	NEO_SFIX_128K( "044-s1.bin", CRC(89903f39) SHA1(a04a0c244a5d5c7a595fcf649107969635a6a8b6) )

	NEO_BIOS_SOUND_128K( "044-m1.bin", CRC(981345f8) SHA1(5eb6a5a3f633d74a48555808e2562a0ba16d8675) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "044-v2.bin", 0x000000, 0x200000, CRC(3ec632ea) SHA1(e3f413f580b57f70d2dae16dbdacb797884d3fce) )
	ROM_LOAD( "044-v4.bin", 0x200000, 0x200000, CRC(4b0f8e23) SHA1(105da0cc5ba19869c7147fba8b177500758c232b) )

	NO_DELTAT_REGION

	ROM_REGION( 0x800000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "044-c1.bin", 0x000000, 0x100000, CRC(ddab98a7) SHA1(f20eb81ec431268798c142c482146c1545af1c24) ) /* Plane 0,1 */
	ROM_CONTINUE(      			 0x400000, 0x100000 )
	ROM_LOAD16_BYTE( "044-c2.bin", 0x000001, 0x100000, CRC(d8ccd575) SHA1(f697263fe92164e274bf34c55327b3d4a158b332) ) /* Plane 2,3 */
	ROM_CONTINUE(      			 0x400001, 0x100000 )
	ROM_LOAD16_BYTE( "044-c3.bin", 0x200000, 0x100000, CRC(403e898a) SHA1(dd5888f8b24a33b2c1f483316fe80c17849ccfc4) ) /* Plane 0,1 */
	ROM_CONTINUE(      			 0x600000, 0x100000 )
	ROM_LOAD16_BYTE( "044-c4.bin", 0x200001, 0x100000, CRC(6235fbaa) SHA1(9090e337d7beed25ba81ae0708d0aeb57e6cf405) ) /* Plane 2,3 */
	ROM_CONTINUE(      			 0x600001, 0x100000 )
ROM_END

ROM_START( samsho )
	ROM_REGION( 0x180000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "045-p1.bin", 0x000000, 0x080000, CRC(80aa6c97) SHA1(6e07a4aa7b4719ae487a10cee9389cb55a370a7a) )
	ROM_LOAD16_WORD_SWAP( "045-p2.bin", 0x080000, 0x080000, CRC(71768728) SHA1(9ff0e01d3fb73ad04279d4fdf4c53c3160888179) )
	ROM_LOAD16_WORD_SWAP( "045-p3.bin", 0x100000, 0x080000, CRC(38ee9ba9) SHA1(48190699a6be83cb6257365ae81f93fdd23abe09) )

	NEO_SFIX_128K( "045-s1.bin", CRC(9142a4d3) SHA1(54088e99fcfd75fd0f94852890a56350066a05a3) )

	NEO_BIOS_SOUND_128K( "045-m1.bin", CRC(95170640) SHA1(125c502db0693e8d11cef619b090081c14a9a300) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "045-v1.bin", 0x000000, 0x200000, CRC(37f78a9b) SHA1(6279b497d12fa90b49ab5ac3aae20fb302ec8b81) )
	ROM_LOAD( "045-v2.bin", 0x200000, 0x200000, CRC(568b20cf) SHA1(61af858685472a1fad608e230cccc2b108509ddb) )

	NO_DELTAT_REGION

	ROM_REGION( 0x900000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "045-c1.bin", 0x000000, 0x200000, CRC(2e5873a4) SHA1(65c74c1e2d34390666bbb630df7d1f4c9570c3db) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "045-c2.bin", 0x000001, 0x200000, CRC(04febb10) SHA1(16a8cbf0fd9468e81bf9eab6dbe7a8e3623a843e) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "045-c3.bin", 0x400000, 0x200000, CRC(f3dabd1e) SHA1(c80e52df42be9f8b2e89b467b11ab140a480cee8) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "045-c4.bin", 0x400001, 0x200000, CRC(935c62f0) SHA1(0053d40085fac14096b683f4341f65e543b71dc1) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "045-c5.bin", 0x800000, 0x080000, CRC(a2bb8284) SHA1(aa118e3b8c062daa219b36758b9a3814c08c69dc) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "045-c6.bin", 0x800001, 0x080000, CRC(4fa71252) SHA1(afe374a9d1f2d955a59efe7b6196b89e021b164c) ) /* Plane 2,3 */
ROM_END

ROM_START( tophuntr )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "046-p1.bin", 0x000000, 0x100000, CRC(69fa9e29) SHA1(9a40a16163193bb506a32bd34f6323b25ec69622) )
	ROM_LOAD16_WORD_SWAP( "046-p2.sp2", 0x100000, 0x100000, CRC(f182cb3e) SHA1(6b4e0af5d4e623f0682f37ff5c69e5b705e20028) )

	NEO_SFIX_128K( "046-s1.bin", CRC(14b01d7b) SHA1(618ce75c25d6cc86a3b46bd64a0aa34ab82f75ae) )

	NEO_BIOS_SOUND_128K( "046-m1.bin", CRC(3f84bb9f) SHA1(07446040871d11da3c2217ee9d1faf8c3cae7420) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "046-v1.bin", 0x000000, 0x100000, CRC(c1f9c2db) SHA1(bed95a76afefa46503a12e0f0a9787c4c967ac50) )
	ROM_LOAD( "046-v2.bin", 0x100000, 0x100000, CRC(56254a64) SHA1(1cf049cb4c414419859d2c8ee714317a35a85251) )
	ROM_LOAD( "046-v3.bin", 0x200000, 0x100000, CRC(58113fb1) SHA1(40972982a63c7adecef840f9882f4165da723ab6) )
	ROM_LOAD( "046-v4.bin", 0x300000, 0x100000, CRC(4f54c187) SHA1(63a76949301b83bdd44aa1a4462f642ab9ca3c0b) )

	NO_DELTAT_REGION

	ROM_REGION( 0x800000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "046-c1.bin", 0x000000, 0x100000, CRC(fa720a4a) SHA1(364913b9fa40d46e4e39ae3cdae914cfd0de137d) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "046-c2.bin", 0x000001, 0x100000, CRC(c900c205) SHA1(50274e79aa26f334eb806288688b30720bade883) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "046-c3.bin", 0x200000, 0x100000, CRC(880e3c25) SHA1(b6974af0c833b766866919b6f15b6f8cef82530d) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "046-c4.bin", 0x200001, 0x100000, CRC(7a2248aa) SHA1(8af0b26025a54e3b91604dd24a3c1c518fbd8536) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "046-c5.bin", 0x400000, 0x100000, CRC(4b735e45) SHA1(2f8b46388c4696aee6a97e1e21cdadf6b142b01a) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "046-c6.bin", 0x400001, 0x100000, CRC(273171df) SHA1(9c35832221e016c12ef1ed71da167f565daaf86c) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "046-c7.bin", 0x600000, 0x100000, CRC(12829c4c) SHA1(ac5f3d848d7116fc35c97f53a72c85e049dd3a2f) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "046-c8.bin", 0x600001, 0x100000, CRC(c944e03d) SHA1(be23999b8ce09ee15ba500ce4d5e2a82a4f58d9b) ) /* Plane 2,3 */
ROM_END

ROM_START( fatfury2 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "047-p1.bin", 0x000000, 0x080000, CRC(be40ea92) SHA1(958b891bb8beb6af122d5467257ab20cbc6cf574) )
	ROM_LOAD16_WORD_SWAP( "047-p2.bin", 0x080000, 0x080000, CRC(2a9beac5) SHA1(61378f89d64fef4c172825694f83229c2ab1a7af) )

	NEO_SFIX_128K( "047-s1.bin", CRC(d7dbbf39) SHA1(29253e596f475ebd41a6e3bb53952e3a0ccd2eed) )

	NEO_BIOS_SOUND_128K( "047-m1.bin", CRC(820b0ba7) SHA1(5708248d89446e49184eaadb52f7c61b2b6c13c5) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "047-v1.bin", 0x000000, 0x200000, CRC(d9d00784) SHA1(f6a91eada8c23aa4518c4b82eeebca69f79d845c) )
	ROM_LOAD( "047-v2.bin", 0x200000, 0x200000, CRC(2c9a4b33) SHA1(d4a1c0951c02c8919b3ec32ed96933634ff9e54c) )

	NO_DELTAT_REGION

	ROM_REGION( 0x800000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "047-c1.bin", 0x000000, 0x100000, CRC(f72a939e) SHA1(67fc398ec28061adca0d3be82bbe7297015800da) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x400000, 0x100000 )
	ROM_LOAD16_BYTE( "047-c2.bin", 0x000001, 0x100000, CRC(05119a0d) SHA1(c2f100b73eb04f65b6ba6089d49aceb51b470ec6) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x400001, 0x100000 )
	ROM_LOAD16_BYTE( "047-c3.bin", 0x200000, 0x100000, CRC(01e00738) SHA1(79654f24d777dd5eb68bafc3b8cb9db71d5822e2) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x600000, 0x100000 )
	ROM_LOAD16_BYTE( "047-c4.bin", 0x200001, 0x100000, CRC(9fe27432) SHA1(89d22d77ba8bc6d1f6c974195c34ad61b9010de7) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x600001, 0x100000 )
ROM_END

ROM_START( janshin )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "048-p1.bin", 0x000000, 0x100000, CRC(7514cb7a) SHA1(da512c0a8e8160a9db7f956e351245327c38eaf1) )

	NEO_SFIX_128K( "048-s1.bin", CRC(8285b25a) SHA1(d983640cda3e346e38469b4d3ec8048b116a7bb7) )

	NEO_BIOS_SOUND_64K( "048-m1.bin", CRC(e191f955) SHA1(fc19705644ffb3d1ae76bcf2b6b78fef05bcf701) )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "048-v1.bin", 0x000000, 0x200000, CRC(f1947d2b) SHA1(955ff91ab24eb2a7ec51ff46c9f9f2ec060456b2) )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "048-c1.bin", 0x000000, 0x200000, CRC(3fa890e9) SHA1(e73d2802bacfbc2b2b16fbbedddde17488e4bbde) )
	ROM_LOAD16_BYTE( "048-c2.bin", 0x000001, 0x200000, CRC(59c48ad8) SHA1(2630817e735a6d197377558f4324c1442803fe15) )
ROM_END

ROM_START( androdun )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "049-p1.bin", 0x000000, 0x080000, CRC(3b857da2) SHA1(4dd86c739944696c16c3cdd85935d6dfa9fdc276) )
	ROM_LOAD16_WORD_SWAP( "049-p2.bin", 0x080000, 0x080000, CRC(2f062209) SHA1(991cf3e3677929b2cc0b2787b0c7b6ad3700f618) )

	NEO_SFIX_128K( "049-s1.bin", CRC(6349de5d) SHA1(bcc44b9576d7bedd9a39294530bb66f707690c72) )

	NEO_BIOS_SOUND_128K( "049-m1.bin", CRC(1a009f8c) SHA1(7782dfe48e143417f34fba5353e6deb63efeaa8a) )

	ROM_REGION( 0x100000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "049-v1.bin", 0x000000, 0x080000, CRC(577c85b3) SHA1(2c3072401fe73497dca0e9009ae2ba4053fe936d) )
	ROM_LOAD( "049-v2.bin", 0x080000, 0x080000, CRC(e14551c4) SHA1(763a9912b9df55defb190af3f29ae034f6dd78d6) )

	NO_DELTAT_REGION

	ROM_REGION( 0x200000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "049-c1.bin", 0x000000, 0x100000, CRC(7ace6db3) SHA1(c41cc9de8c0788dcc49ca494fd3bb3124062d9dd) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "049-c2.bin", 0x000001, 0x100000, CRC(b17024f7) SHA1(fcf7efae48fcdccaf5255c145de414fb246128f0) ) /* Plane 2,3 */
	/* these just contain junk, probably shouldn't be here */
/*	ROM_LOAD16_BYTE( "049-c3.bin", 0x200000, 0x100000, CRC(2e0f3f9a) SHA1(8ee3442be92835922762420e8d0ff86dc14b3d69) ) */ /* Plane 0,1 /*/ 
/*	ROM_LOAD16_BYTE( "049-c4.bin", 0x200001, 0x100000, CRC(4a19fb92) SHA1(171219f0b38a04bfcee5b823c043a8181dfc87f8) ) */ /* Plane 2,3 /*/ 
ROM_END

ROM_START( ncommand )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "050-p1.bin", 0x000000, 0x100000, CRC(4e097c40) SHA1(43311a7ca14a14dcd4a99d8576a12e897b078643) )

	NEO_SFIX_128K( "050-s1.bin", CRC(db8f9c8e) SHA1(11cb82cf3c4d0fc2da5df0c26410a64808093610) )

	NEO_BIOS_SOUND_128K( "050-m1.bin", CRC(6fcf07d3) SHA1(e9ecff4bfec1f5964bf06645f75d80d611b6231c) )

	ROM_REGION( 0x180000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "050-v1.bin", 0x000000, 0x100000, CRC(23c3ab42) SHA1(b6c59bb180f1aa34c95f3ec923f3aafb689d57b0) )
	ROM_LOAD( "050-v2.bin", 0x100000, 0x080000, CRC(80b8a984) SHA1(950cf0e78ceffa4037663f1086fbbc88588f49f2) )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "050-c1.bin", 0x000000, 0x100000, CRC(87421a0a) SHA1(1d8faaf03778f7c5b062554d7333bbd3f0ca12ad) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "050-c2.bin", 0x000001, 0x100000, CRC(c4cf5548) SHA1(ef9eca5aeff9dda2209a050c2af00ed8979ae2bc) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "050-c3.bin", 0x200000, 0x100000, CRC(03422c1e) SHA1(920e5015aebe2ffc5ce43a52365c7f0a705f3b9e) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "050-c4.bin", 0x200001, 0x100000, CRC(0845eadb) SHA1(3c71a02bf0e07a5381846bb6d75bbe7dd546adea) ) /* Plane 2,3 */
ROM_END

ROM_START( viewpoin )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "051-p1.bin", 0x000000, 0x100000, CRC(17aa899d) SHA1(674cd8ace7acdf4f407de741e3d0071bcb49c902) )

	NEO_SFIX_64K( "051-s1.bin", CRC(6d0f146a) SHA1(537fc5ef099f46fef64d147c0d1264b319758b4f) )

	NEO_BIOS_SOUND_64K( "051-m1.bin", CRC(d57bd7af) SHA1(9ed766dbc3c07dbba7869ff335eeb7e297c8e2d9) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "051-v1.bin", 0x000000, 0x200000, CRC(019978b6) SHA1(7896a551115fc6ed38b5944e0c8dcb2b2c1c077d) )
	ROM_LOAD( "051-v2.bin", 0x200000, 0x200000, CRC(5758f38c) SHA1(da10f4b7d22d9139bbf068bd940be82168a74ca1) )

	NO_DELTAT_REGION

	ROM_REGION( 0x600000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "051-c1.bin", 0x000000, 0x100000, CRC(d624c132) SHA1(49c7e9f020cba45d7083b45252bcc03397f8c286) )
	ROM_CONTINUE(      			   0x400000, 0x100000 )
	ROM_LOAD16_BYTE( "051-c2.bin", 0x000001, 0x100000, CRC(40d69f1e) SHA1(ec4a13582772594957f927622d50f54b0dfcd8d8) )
	ROM_CONTINUE(      			   0x400001, 0x100000 )
ROM_END

ROM_START( ssideki )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "052-p1.bin", 0x000000, 0x080000, CRC(9cd97256) SHA1(1c780b711137fd79cc81b01941e84f3d59e0071f) )

	NEO_SFIX_128K( "052-s1.bin", CRC(97689804) SHA1(fa8dab3b3353d7115a0368f3fc749950c0186fbc) )

	NEO_BIOS_SOUND_128K( "052-m1.bin", CRC(49f17d2d) SHA1(70971fcf71ae3a6b2e26e7ade8063941fb178ae5) )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "052-v1.bin", 0x000000, 0x200000, CRC(22c097a5) SHA1(328c4e6db0a026f54a633cff1443a3f964a8daea) )

	NO_DELTAT_REGION

	ROM_REGION( 0x600000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "052-c1.bin", 0x000000, 0x100000, CRC(53e1c002) SHA1(2125b1be379ea7933893ffb1cd65d6c4bf8b03bd) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x400000, 0x100000 )
	ROM_LOAD16_BYTE( "052-c2.bin", 0x000001, 0x100000, CRC(776a2d1f) SHA1(bca0bac87443e9e78c623d284f6cc96cc9c9098f) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x400001, 0x100000 )
ROM_END

ROM_START( wh1 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "053-p1.bin", 0x000000, 0x080000, CRC(95b574cb) SHA1(b7b7af6a04c3d902e7f8852897741ecaf0b1062c) )
	ROM_LOAD16_WORD_SWAP( "053-p2.bin", 0x080000, 0x080000, CRC(f198ed45) SHA1(24ccc091e97f63796562bb5b30df51f39bd504ef) )

	NEO_SFIX_128K( "053-s1.bin", CRC(8c2c2d6b) SHA1(87fa79611c6f8886dcc8766814829c669c65b40f) )

	NEO_BIOS_SOUND_128K( "053-m1.bin", CRC(1bd9d04b) SHA1(65cd7b002123ed1a3111e3d942608d0082799ff3) )

	ROM_REGION( 0x300000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "053-v2.bin", 0x000000, 0x200000, CRC(a68df485) SHA1(007fa715423fba72c899cd3db3f4bec13281cf7a) )
	ROM_LOAD( "053-v4.bin", 0x200000, 0x100000, CRC(7bea8f66) SHA1(428e8721bd87f7faa756adb1e12672219be46c1d) )

	NO_DELTAT_REGION

	ROM_REGION( 0x600000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "053-c1.bin", 0x000000, 0x100000, CRC(85eb5bce) SHA1(3d03d29296ca6e6b5106aac4aaeec9d4b4ed1313) ) /* Plane 0,1 */
	ROM_CONTINUE(      			0x400000, 0x100000 )
	ROM_LOAD16_BYTE( "053-c2.bin", 0x000001, 0x100000, CRC(ec93b048) SHA1(d4159210df94e259f874a4671d271ec27be13451) ) /* Plane 2,3 */
	ROM_CONTINUE(      			0x400001, 0x100000 )
	ROM_LOAD16_BYTE( "053-c3.bin", 0x200000, 0x100000, CRC(0dd64965) SHA1(e97b3b8a461da5e8861b3dfdacb25e007ced37a1) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "053-c4.bin", 0x200001, 0x100000, CRC(9270d954) SHA1(a2ef909868f6b06cdcc22a63ddf6c96be12b999c) ) /* Plane 2,3 */
ROM_END

ROM_START( wh1h )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "053-p1.rom", 0x000000, 0x080000, CRC(ed29fde2) SHA1(52b8ca5b804f786f95e1dfb348d8c7b82f1d4ddf) )
	ROM_LOAD16_WORD_SWAP( "053-p2.rom", 0x080000, 0x080000, CRC(98f2b158) SHA1(a64e1425970eb53cc910891db39973dee3d54ccc) )

	NEO_SFIX_128K( "053-s1.bin", CRC(8c2c2d6b) SHA1(87fa79611c6f8886dcc8766814829c669c65b40f) )

	NEO_BIOS_SOUND_128K( "053-m1.bin", CRC(1bd9d04b) SHA1(65cd7b002123ed1a3111e3d942608d0082799ff3) )

	ROM_REGION( 0x300000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "053-v2.bin", 0x000000, 0x200000, CRC(a68df485) SHA1(007fa715423fba72c899cd3db3f4bec13281cf7a) )
	ROM_LOAD( "053-v4.bin", 0x200000, 0x100000, CRC(7bea8f66) SHA1(428e8721bd87f7faa756adb1e12672219be46c1d) )

	NO_DELTAT_REGION

	ROM_REGION( 0x600000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "053-c1.bin", 0x000000, 0x100000, CRC(85eb5bce) SHA1(3d03d29296ca6e6b5106aac4aaeec9d4b4ed1313) ) /* Plane 0,1 */
	ROM_CONTINUE(      			0x400000, 0x100000 )
	ROM_LOAD16_BYTE( "053-c2.bin", 0x000001, 0x100000, CRC(ec93b048) SHA1(d4159210df94e259f874a4671d271ec27be13451) ) /* Plane 2,3 */
	ROM_CONTINUE(      			0x400001, 0x100000 )
	ROM_LOAD16_BYTE( "053-c3.bin", 0x200000, 0x100000, CRC(0dd64965) SHA1(e97b3b8a461da5e8861b3dfdacb25e007ced37a1) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "053-c4.bin", 0x200001, 0x100000, CRC(9270d954) SHA1(a2ef909868f6b06cdcc22a63ddf6c96be12b999c) ) /* Plane 2,3 */
ROM_END

ROM_START( kof94 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "055-p1.bin", 0x100000, 0x100000, CRC(f10a2042) SHA1(d08a3f3c28be4b1793de7d362456281329fe1828) )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "055-s1.bin", CRC(825976c1) SHA1(cb6a70bdd95d449d25196ca269b621c362db6743) )

	NEO_BIOS_SOUND_128K( "055-m1.bin", CRC(f6e77cf5) SHA1(292a3e3a4918ffe72bd1c41acb927b91844e035e) )

	ROM_REGION( 0x600000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "055-v1.bin", 0x000000, 0x200000, CRC(8889596d) SHA1(c9ce713b720511438dbd3fe3bcc7c246f475c6a2) )
	ROM_LOAD( "055-v2.bin", 0x200000, 0x200000, CRC(25022b27) SHA1(2b040a831c3c92ac6e4719de38009a0d55b64f6b) )
	ROM_LOAD( "055-v3.bin", 0x400000, 0x200000, CRC(83cf32c0) SHA1(34a31a37eb10945b5169e96321bcea06eec33a00) )

	NO_DELTAT_REGION

	ROM_REGION( 0x1000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "055-c1.bin", 0x000000, 0x200000, CRC(b96ef460) SHA1(e52f5303c17b50ce165c008be2837336369c110b) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "055-c2.bin", 0x000001, 0x200000, CRC(15e096a7) SHA1(237c2a3d059de00bfca66e0016ed325d7a32bfec) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "055-c3.bin", 0x400000, 0x200000, CRC(54f66254) SHA1(c594384bcd8b03beb8c595591505fecc44b185ac) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "055-c4.bin", 0x400001, 0x200000, CRC(0b01765f) SHA1(ec1fdcc944611408367bf5023d4ebe7edd9dfa88) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "055-c5.bin", 0x800000, 0x200000, CRC(ee759363) SHA1(8a5621c1b1f8267b9b9b6a14ab4944de542e1945) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "055-c6.bin", 0x800001, 0x200000, CRC(498da52c) SHA1(1e6e6202ee053a5261db889177ce3a087e078bda) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "055-c7.bin", 0xc00000, 0x200000, CRC(62f66888) SHA1(ac91a0eab0753bee175ad40213a4ae5d38ed5b87) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "055-c8.bin", 0xc00001, 0x200000, CRC(fe0a235d) SHA1(a45c66836e4e3c77dfef9d4c6cc422cb59169149) ) /* Plane 2,3 */
ROM_END

ROM_START( aof2 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "056-p1.bin", 0x000000, 0x100000, CRC(a3b1d021) SHA1(ee42f3ca4516226b0088d0303ed28e3ecdabcd71) )

	NEO_SFIX_128K( "056-s1.bin", CRC(8b02638e) SHA1(aa4d28804ca602da776948b5f223ea89e427906b) )

	NEO_BIOS_SOUND_128K( "056-m1.bin", CRC(f27e9d52) SHA1(dddae733d87ce7c88ad2580a8f64cb6ff9572e67) )

	ROM_REGION( 0x500000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "056-v1.bin", 0x000000, 0x200000, CRC(4628fde0) SHA1(ece2a50f5270d844d58401b1447d1d856d78ea45) )
	ROM_LOAD( "056-v2.bin", 0x200000, 0x200000, CRC(b710e2f2) SHA1(df4da585203eea7554d3ce718eb107e9cb6a0254) )
	ROM_LOAD( "056-v3.bin", 0x400000, 0x100000, CRC(d168c301) SHA1(969273d1d11943e81560959359a2c4e69522af0e) )

	NO_DELTAT_REGION

	ROM_REGION( 0x1000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "056-c1.bin", 0x000000, 0x200000, CRC(17b9cbd2) SHA1(1eee81e02763d384bd1c10a6012473ca931e4093) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "056-c2.bin", 0x000001, 0x200000, CRC(5fd76b67) SHA1(11925a41a53b53c6df4a5ebd28f98300950f743b) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "056-c3.bin", 0x400000, 0x200000, CRC(d2c88768) SHA1(22e2d84aa0c095944190e249ce87ef50d3f7b8ce) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "056-c4.bin", 0x400001, 0x200000, CRC(db39b883) SHA1(59de86c513dc4e230ae25d9e3b7e84621b657b54) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "056-c5.bin", 0x800000, 0x200000, CRC(c3074137) SHA1(9a75e3d63cb98d54f900dcfb3a03e21f3148d32f) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "056-c6.bin", 0x800001, 0x200000, CRC(31de68d3) SHA1(13ba7046cdd6863125f8284e60f102d4720af5a4) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "056-c7.bin", 0xc00000, 0x200000, CRC(3f36df57) SHA1(79ee97e9ae811a51141b535633f90e1491209d54) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "056-c8.bin", 0xc00001, 0x200000, CRC(e546d7a8) SHA1(74a2fca994a5a93a5784a46c0f68193122456a09) ) /* Plane 2,3 */
ROM_END

ROM_START( wh2 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "057-p1.bin", 0x100000, 0x100000, CRC(65a891d9) SHA1(ff8d5ccb0dd22c523902bb3db3c645583a335056) )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "057-s1.bin", CRC(fcaeb3a4) SHA1(1f3f85e38b8552333261c04ae5af0d6e3b310622) )

	NEO_BIOS_SOUND_128K( "057-m1.bin", CRC(8fa3bc77) SHA1(982f92978671e4ee66630948e6bb7565b37b5dc0) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "057-v1.bin", 0x000000, 0x200000, CRC(8877e301) SHA1(1bab988d74ea8fd12db201c257ec844622cf5f4e) )
	ROM_LOAD( "057-v2.bin", 0x200000, 0x200000, CRC(c1317ff4) SHA1(4c28b2b5998abaeaa5143f2f3a9ba52c6041f4f3) )

	NO_DELTAT_REGION

	ROM_REGION( 0xc00000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "057-c1.bin", 0x000000, 0x200000, CRC(21c6bb91) SHA1(a2c17d0c91dd59528d8fa7fe110af8b20b25ff99) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "057-c2.bin", 0x000001, 0x200000, CRC(a3999925) SHA1(0ee861a77850d378d03c1bf00b9692abd860c759) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "057-c3.bin", 0x400000, 0x200000, CRC(b725a219) SHA1(4857687d156a9150a69b97d2729245a51c144a0c) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "057-c4.bin", 0x400001, 0x200000, CRC(8d96425e) SHA1(0f79c868a6a33ad25e38d842f30ec4440d809033) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "057-c5.bin", 0x800000, 0x200000, CRC(b20354af) SHA1(da7609fd467f2f4d71d92970f438a04d11ab1cc1) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "057-c6.bin", 0x800001, 0x200000, CRC(b13d1de3) SHA1(7d749c23a33d90fe50279e884540d71cf1aaaa6b) ) /* Plane 2,3 */
ROM_END

ROM_START( fatfursp )
	ROM_REGION( 0x180000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "058-p1.bin", 0x000000, 0x100000, CRC(2f585ba2) SHA1(429b4bf43fb9b1082c15d645ca328f9d175b976b) )
	ROM_LOAD16_WORD_SWAP( "058-p2.bin", 0x100000, 0x080000, CRC(d7c71a6b) SHA1(b3428063031a2e5857da40a5d2ffa87fb550c1bb) )
	/* rom below is just a modified p1 */
/*	ROM_LOAD16_WORD_SWAP( "058-p3.bin", 0x180000, 0x080000, CRC(9f0c1e1a) SHA1(02861b0f230541becccc3df6a2c85dbe8733e7ce) )*/

	NEO_SFIX_128K( "058-s1.bin", CRC(2df03197) SHA1(24083cfc97e720ac9e131c9fe37df57e27c49294) )

	NEO_BIOS_SOUND_128K( "058-m1.bin", CRC(ccc5186e) SHA1(cf9091c523c182aebfb928c91640b2d72fd70123) )

	ROM_REGION( 0x500000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "058-v1.bin", 0x000000, 0x200000, CRC(55d7ce84) SHA1(05ac6a395d9bf9166925acca176a8d6129f533c8) )
	ROM_LOAD( "058-v2.bin", 0x200000, 0x200000, CRC(ee080b10) SHA1(29814fc21bbe30d37745c8918fab00c83a309be4) )
	ROM_LOAD( "058-v3.bin", 0x400000, 0x100000, CRC(f9eb3d4a) SHA1(d1747f9460b965f6daf4f881ed4ecd04c5253434) )

	NO_DELTAT_REGION

	ROM_REGION( 0xc00000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "058-c1.bin", 0x000000, 0x200000, CRC(044ab13c) SHA1(569d283638a132bc163faac2a9055497017ee0d2) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "058-c2.bin", 0x000001, 0x200000, CRC(11e6bf96) SHA1(c093a4f93f13e07b276e28b30c2a14dda9135d8f) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "058-c3.bin", 0x400000, 0x200000, CRC(6f7938d5) SHA1(be057b0a3faeb76d5fff161d3e6fea8a26e11d2c) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "058-c4.bin", 0x400001, 0x200000, CRC(4ad066ff) SHA1(4e304646d954d5f7bbabc5d068e85de31d38830f) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "058-c5.bin", 0x800000, 0x200000, CRC(49c5e0bf) SHA1(f3784178f90751990ea47a082a6aa869ee3566c9) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "058-c6.bin", 0x800001, 0x200000, CRC(8ff1f43d) SHA1(6180ceb5412a3e2e34e9513a3283b9f63087f747) ) /* Plane 2,3 */
ROM_END

ROM_START( fatfursa )
	ROM_REGION( 0x180000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "058-ep1.bin", 0x000000, 0x100000, CRC(36be707d) SHA1(59dc26a85f967f07a1a8ce9300e7f0eb52ccf794) )
	/* the first part of this is the same as p3 in the other set, maybe the other cart was actually an upgraded one? */
	ROM_LOAD16_WORD_SWAP( "058-p2.bin", 0x100000, 0x080000, CRC(d7c71a6b) SHA1(b3428063031a2e5857da40a5d2ffa87fb550c1bb) )

	NEO_SFIX_128K( "058-s1.bin", CRC(2df03197) SHA1(24083cfc97e720ac9e131c9fe37df57e27c49294) )

	NEO_BIOS_SOUND_128K( "058-m1.bin", CRC(ccc5186e) SHA1(cf9091c523c182aebfb928c91640b2d72fd70123) )

	ROM_REGION( 0x500000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "058-v1.bin", 0x000000, 0x200000, CRC(55d7ce84) SHA1(05ac6a395d9bf9166925acca176a8d6129f533c8) )
	ROM_LOAD( "058-v2.bin", 0x200000, 0x200000, CRC(ee080b10) SHA1(29814fc21bbe30d37745c8918fab00c83a309be4) )
	ROM_LOAD( "058-v3.bin", 0x400000, 0x100000, CRC(f9eb3d4a) SHA1(d1747f9460b965f6daf4f881ed4ecd04c5253434) )

	NO_DELTAT_REGION

	ROM_REGION( 0xc00000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "058-c1.bin", 0x000000, 0x200000, CRC(044ab13c) SHA1(569d283638a132bc163faac2a9055497017ee0d2) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "058-c2.bin", 0x000001, 0x200000, CRC(11e6bf96) SHA1(c093a4f93f13e07b276e28b30c2a14dda9135d8f) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "058-c3.bin", 0x400000, 0x200000, CRC(6f7938d5) SHA1(be057b0a3faeb76d5fff161d3e6fea8a26e11d2c) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "058-c4.bin", 0x400001, 0x200000, CRC(4ad066ff) SHA1(4e304646d954d5f7bbabc5d068e85de31d38830f) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "058-c5.bin", 0x800000, 0x200000, CRC(49c5e0bf) SHA1(f3784178f90751990ea47a082a6aa869ee3566c9) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "058-c6.bin", 0x800001, 0x200000, CRC(8ff1f43d) SHA1(6180ceb5412a3e2e34e9513a3283b9f63087f747) ) /* Plane 2,3 */
ROM_END

ROM_START( savagere )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "059-p1.bin", 0x100000, 0x100000, CRC(01d4e9c0) SHA1(3179d2be59bf2de6918d506117cff50acf7e09f3) )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "059-s1.bin", CRC(e08978ca) SHA1(55152cb9bd0403ae8656b93a6b1522dba5db6d1a) )

	NEO_BIOS_SOUND_128K( "059-m1.bin", CRC(29992eba) SHA1(187be624abe8670503edb235ff21ae8fdc3866e0) )

	ROM_REGION( 0x600000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "059-v1.bin", 0x000000, 0x200000, CRC(530c50fd) SHA1(29401cee7f7d2c199c7cb58092e86b28205e81ad) )
	ROM_LOAD( "059-v2.bin", 0x200000, 0x200000, CRC(eb6f1cdb) SHA1(7a311388315ea543babf872f62219fdc4d39d013) )
	ROM_LOAD( "059-v3.bin", 0x400000, 0x200000, CRC(7038c2f9) SHA1(c1d6f86b24feba03fe009b58199d2eeabe572f4e) )

	NO_DELTAT_REGION

	ROM_REGION( 0x1000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "059-c1.bin", 0x000000, 0x200000, CRC(763ba611) SHA1(d3262e0332c894ee149c5963f882cc5e5562ee57) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "059-c2.bin", 0x000001, 0x200000, CRC(e05e8ca6) SHA1(986a9b16ff92bc101ab567d2d01348e093abea9a) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "059-c3.bin", 0x400000, 0x200000, CRC(3e4eba4b) SHA1(770adec719e63a30ebe9522cc7576caaca44f3b2) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "059-c4.bin", 0x400001, 0x200000, CRC(3c2a3808) SHA1(698adcec0715c9e78b6286be38debf0ce28fd644) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "059-c5.bin", 0x800000, 0x200000, CRC(59013f9e) SHA1(5bf48fcc450da72a8c4685f6e3887e67eae49988) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "059-c6.bin", 0x800001, 0x200000, CRC(1c8d5def) SHA1(475d89a5c4922a9f6bd756d23c2624d57b6e9d62) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "059-c7.bin", 0xc00000, 0x200000, CRC(c88f7035) SHA1(c29a428b741f4fe7b71a3bc23c87925b6bc1ca8f) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "059-c8.bin", 0xc00001, 0x200000, CRC(484ce3ba) SHA1(4f21ed20ce6e2b67e2b079404599310c94f591ff) ) /* Plane 2,3 */
ROM_END

ROM_START( fightfev )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "060-p1.bin", 0x000000, 0x080000, CRC(3032041b) SHA1(4b8ed2e6f74579ea35a53e06ccac42d6905b0f51) )
	ROM_LOAD16_WORD_SWAP( "060-p2.bin", 0x080000, 0x080000, CRC(b0801d5f) SHA1(085746d8f5d271d5f84ccbb7f577193c391f88d4) )

	NEO_SFIX_128K( "060-s1.bin", CRC(70727a1e) SHA1(e0d226be0578adbe7c1d41baba79e61d4d8fac39) )

	NEO_BIOS_SOUND_128K( "060-m1.bin", CRC(0b7c4e65) SHA1(999a1e784de18db3f1332b30bc425836ea6970be) )

	ROM_REGION(  0x300000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "060-v1.bin", 0x000000, 0x200000, CRC(f417c215) SHA1(0f53b8dd056f43b5d880628e8b74c2b27881ffac) )
	ROM_LOAD( "060-v2.bin", 0x200000, 0x100000, CRC(64470036) SHA1(eb2b34b3c01eb5c1a0a40cff6f4c0f2eee7bf7f2) )

	NO_DELTAT_REGION

	ROM_REGION( 0x0800000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "060-c1.bin", 0x0000000, 0x200000, CRC(8908fff9) SHA1(f8c16ab0248b60f3a62e0d4d65c456e2f8e4da49) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "060-c2.bin", 0x0000001, 0x200000, CRC(c6649492) SHA1(5d39b077387ed6897ac075ede4a2aa94bb64545e) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "060-c3.bin", 0x0400000, 0x200000, CRC(0956b437) SHA1(c70be8b5cebf321afe4c3f5e9a12413c3077694a) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "060-c4.bin", 0x0400001, 0x200000, CRC(026f3b62) SHA1(d608483b70d60e7aa0e41f25a8b3fed508129eb7) ) /* Plane 2,3 */
ROM_END

ROM_START( fightfva )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "060-p1a.bin", 0x0000000, 0x100000, CRC(2a104b50) SHA1(3eb663d3df7074e1cdf4c0e450a35c9cf55d8979) )
	/* there was also a copy of the 060-p1.bin with the name 060-p2.bin maybe it should be loaded over the top or this
	   larger rom is an older revision... */

	NEO_SFIX_128K( "060-s1.bin", CRC(70727a1e) SHA1(e0d226be0578adbe7c1d41baba79e61d4d8fac39) )

	NEO_BIOS_SOUND_128K( "060-m1.bin", CRC(0b7c4e65) SHA1(999a1e784de18db3f1332b30bc425836ea6970be) )

	ROM_REGION(  0x300000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "060-v1.bin", 0x000000, 0x200000, CRC(f417c215) SHA1(0f53b8dd056f43b5d880628e8b74c2b27881ffac) )
	ROM_LOAD( "060-v2.bin", 0x200000, 0x100000, CRC(64470036) SHA1(eb2b34b3c01eb5c1a0a40cff6f4c0f2eee7bf7f2) )

	NO_DELTAT_REGION

	ROM_REGION( 0x0800000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "060-c1.bin", 0x0000000, 0x200000, CRC(8908fff9) SHA1(f8c16ab0248b60f3a62e0d4d65c456e2f8e4da49) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "060-c2.bin", 0x0000001, 0x200000, CRC(c6649492) SHA1(5d39b077387ed6897ac075ede4a2aa94bb64545e) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "060-c3.bin", 0x0400000, 0x200000, CRC(0956b437) SHA1(c70be8b5cebf321afe4c3f5e9a12413c3077694a) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "060-c4.bin", 0x0400001, 0x200000, CRC(026f3b62) SHA1(d608483b70d60e7aa0e41f25a8b3fed508129eb7) ) /* Plane 2,3 */
ROM_END

ROM_START( ssideki2 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "061-p1.bin", 0x000000, 0x100000, CRC(5969e0dc) SHA1(78abea880c125ec5a85bef6404478512a34b5513) )

	NEO_SFIX_128K( "061-s1.bin", CRC(226d1b68) SHA1(de010f6fda3ddadb181fe37daa6105f22e78b970) )

	NEO_BIOS_SOUND_128K( "061-m1.bin", CRC(156f6951) SHA1(49686f615f109a02b4f23931f1c84fee13872ffd) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "061-v1.bin", 0x000000, 0x200000, CRC(f081c8d3) SHA1(fc9da0ddc1ddd1f9ae1443a726815c25e9dc38ae) )
	ROM_LOAD( "061-v2.bin", 0x200000, 0x200000, CRC(7cd63302) SHA1(c39984c0ae0a8e76f1fc036344bbb83635c18937) )

	NO_DELTAT_REGION

	ROM_REGION( 0x800000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "061-c1.bin", 0x000000, 0x200000, CRC(a626474f) SHA1(d695f0dcb9480088b3a7c1488bd541b4c159528a) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "061-c2.bin", 0x000001, 0x200000, CRC(c3be42ae) SHA1(7fa65538bd0a0a162e4d3e9f49913da59d915e02) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "061-c3.bin", 0x400000, 0x200000, CRC(2a7b98b9) SHA1(75e1019dca8a8583afcc53651ac856cba3a96315) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "061-c4.bin", 0x400001, 0x200000, CRC(c0be9a1f) SHA1(228f41eaefdf3e147761f8ef849e3b5f321877d4) ) /* Plane 2,3 */
ROM_END

ROM_START( spinmast )
	ROM_REGION( 0x180000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "062-p1.bin", 0x000000, 0x100000, CRC(37aba1aa) SHA1(1a2ab9593371cc2f665121d554eec3f6bb4d09ff) )
	ROM_LOAD16_WORD_SWAP( "062-p2.bin", 0x100000, 0x080000, CRC(43763ad2) SHA1(9b08cf1a79294c3206f6364466cae2c8b15acad5) )

	NEO_SFIX_128K( "062-s1.bin", CRC(289e2bbe) SHA1(f52c7f2bffc89df3130b3cabd200408509a28cdc) )

	NEO_BIOS_SOUND_128K( "062-m1.bin", CRC(76108b2f) SHA1(08c89a8b746dbb10ff885b41cde344173c2e3699) )

	ROM_REGION( 0x100000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "062-v1.bin", 0x000000, 0x100000, CRC(cc281aef) SHA1(68be154b3e25f837afb4a477600dbe0ee69bec44) )

	NO_DELTAT_REGION

	ROM_REGION( 0x800000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "062-c1.bin", 0x000000, 0x100000, CRC(a9375aa2) SHA1(69218d8f1361e9ea709da11e3f15fe46b1db7181) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "062-c2.bin", 0x000001, 0x100000, CRC(0e73b758) SHA1(a247f736fbca0b609818dca4844ebb8442753bc1) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "062-c3.bin", 0x200000, 0x100000, CRC(df51e465) SHA1(171953c7a870f3ab96e0f875117ee7343931fd38) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "062-c4.bin", 0x200001, 0x100000, CRC(38517e90) SHA1(f7c64b94ac20f5146f9bb48b53cb2b30fe5b8f8c) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "062-c5.bin", 0x400000, 0x100000, CRC(7babd692) SHA1(0d4cd5006baa8d951cd2b6194ace566fa2845b8a) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "062-c6.bin", 0x400001, 0x100000, CRC(cde5ade5) SHA1(5899ef5dfcdbb8cf8c6aba748dbb52f3c5fed5fe) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "062-c7.bin", 0x600000, 0x100000, CRC(bb2fd7c0) SHA1(cce11c4cf39ac60143235ff89261806df339dae5) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "062-c8.bin", 0x600001, 0x100000, CRC(8d7be933) SHA1(e7097cfa26a959f90721e2e8368ceb47ea9db661) ) /* Plane 2,3 */
ROM_END

ROM_START( samsho2 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "063-p1.bin", 0x100000, 0x100000, CRC(22368892) SHA1(0997f8284aa0f57a333be8a0fdea777d0d01afd6) )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "063-s1.bin", CRC(64a5cd66) SHA1(12cdfb27bf9ccd5a8df6ddd4628ef7cf2c6d4964) )

	NEO_BIOS_SOUND_128K( "063-m1.bin", CRC(56675098) SHA1(90429fc40d056d480d0e2bbefbc691d9fa260fc4) )

	ROM_REGION( 0x700000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "063-v1.bin", 0x000000, 0x200000, CRC(37703f91) SHA1(a373ebef4c33ba1d8340e826981a58769aada238) )
	ROM_LOAD( "063-v2.bin", 0x200000, 0x200000, CRC(0142bde8) SHA1(0be6c53acac44802bf70b6925452f70289a139d9) )
	ROM_LOAD( "063-v3.bin", 0x400000, 0x200000, CRC(d07fa5ca) SHA1(1da7f081f8b8fc86a91feacf900f573218d82676) )
	ROM_LOAD( "063-v4.bin", 0x600000, 0x100000, CRC(24aab4bb) SHA1(10ee4c5b3579865b93dcc1e4079963276aa700a6) )

	NO_DELTAT_REGION

	ROM_REGION( 0x1000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "063-c1.bin", 0x000000, 0x200000, CRC(86cd307c) SHA1(0d04336f7c436d74638d8c1cd8651faf436a6bec) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "063-c2.bin", 0x000001, 0x200000, CRC(cdfcc4ca) SHA1(179dc81432424d68cefedd20cc1c4b2a95deb891) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "063-c3.bin", 0x400000, 0x200000, CRC(7a63ccc7) SHA1(49d97c543bc2860d493a353ab0d059088c6fbd21) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "063-c4.bin", 0x400001, 0x200000, CRC(751025ce) SHA1(e1bbaa7cd67fd04e4aab7f7ea77f63ae1cbc90d0) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "063-c5.bin", 0x800000, 0x200000, CRC(20d3a475) SHA1(28da44a136bd14c73c62c147c3f6e6bcfa1066de) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "063-c6.bin", 0x800001, 0x200000, CRC(ae4c0a88) SHA1(cc8a7d11daa3821f83a6fd0942534706f939e576) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "063-c7.bin", 0xc00000, 0x200000, CRC(2df3cbcf) SHA1(e54f9022359963711451c2025825b862d36c6975) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "063-c8.bin", 0xc00001, 0x200000, CRC(1ffc6dfa) SHA1(acea18aca76c072e0bac2a364fc96d49cfc86e77) ) /* Plane 2,3 */
ROM_END

ROM_START( wh2j )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "064-p1.bin", 0x100000, 0x100000, CRC(385a2e86) SHA1(cfde4a1aeae038a3d6ca9946065624f097682d3d) )
	ROM_CONTINUE(					   0x000000, 0x100000 )

	NEO_SFIX_128K( "064-s1.bin", CRC(2a03998a) SHA1(5e33f469982f12d4622a06d323a345f192bf88e6) )

	NEO_BIOS_SOUND_128K( "064-m1.bin", CRC(d2eec9d3) SHA1(09478787045f1448d19d064dd3d540d1741fd619) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "064-v1.bin", 0x000000, 0x200000, CRC(aa277109) SHA1(35c22b15bb0a4d0ab118cb22a2d450d03995a17c) )
	ROM_LOAD( "064-v2.bin", 0x200000, 0x200000, CRC(b6527edd) SHA1(2bcf5bfa6e117cf4a3728a5e5f5771313c93f22a) )

	NO_DELTAT_REGION

	ROM_REGION( 0x1000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "064-c1.bin", 0x000000, 0x200000, CRC(2ec87cea) SHA1(e713ec7839a7665edee6ee3f82a6e530b3b4bd7c) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "064-c2.bin", 0x000001, 0x200000, CRC(526b81ab) SHA1(b5f0a2f04489539ed6b9d0810b12787356c64b23) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "064-c3.bin", 0x400000, 0x200000, CRC(436d1b31) SHA1(059776d77b91377ed0bcfc278802d659c917fc0f) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "064-c4.bin", 0x400001, 0x200000, CRC(f9c8dd26) SHA1(25a9eea1d49b21b4a988beb32c25bf2f7796f227) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "064-c5.bin", 0x800000, 0x200000, CRC(8e34a9f4) SHA1(67b839b426ef3fad0a85d951fdd44c0a45c55226) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "064-c6.bin", 0x800001, 0x200000, CRC(a43e4766) SHA1(54f282f2b1ff2934cca7acbb4386a2b99a29df3a) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "064-c7.bin", 0xc00000, 0x200000, CRC(59d97215) SHA1(85a960dc7f364df13ee0c2f99a4c53aefb081486) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "064-c8.bin", 0xc00001, 0x200000, CRC(fc092367) SHA1(69ff4ae909dd857de3ca8645d63f8b4bde117448) ) /* Plane 0,1 */
ROM_END

ROM_START( wjammers )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "065-p1.bin", 0x000000, 0x080000, CRC(e81e7a31) SHA1(bb1a8922afe269f6e3ea63c3a377c2a6e2171d2d) )

	NEO_SFIX_128K( "065-s1.bin", CRC(074b5723) SHA1(86d3b3bb5414f43e4d3b7a2af0db23cc71ce8412) )

	NEO_BIOS_SOUND_128K( "065-m1.bin", CRC(52c23cfc) SHA1(809a7e072ad9acbffc25e9bd27cdb97638d09d07) )

	ROM_REGION( 0x380000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "065-v1.bin", 0x000000, 0x100000, CRC(ce8b3698) SHA1(7d75e2a5cf8c90be422f8b425447e81597fe145a) )
	ROM_LOAD( "065-v2.bin", 0x100000, 0x100000, CRC(659f9b96) SHA1(62f40365212153bc3b92a1187fa44f6cdc7f7b83) )
	ROM_LOAD( "065-v3.bin", 0x200000, 0x100000, CRC(39f73061) SHA1(ec57cd58e7f8569cff925d11e2320d588ce4fe49) )
	ROM_LOAD( "065-v4.bin", 0x300000, 0x080000, CRC(3740edde) SHA1(aaf64df1d27289e23dc4e3979ba12a7c928d1a7f) )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "065-c1.bin", 0x000000, 0x100000, CRC(c7650204) SHA1(42918d700d59864f8ab15caf968a062a563c9b09) )
	ROM_LOAD16_BYTE( "065-c2.bin", 0x000001, 0x100000, CRC(d9f3e71d) SHA1(fad1f64061eac1bf85bf6d75d2eae974a8c94069) )
	ROM_LOAD16_BYTE( "065-c3.bin", 0x200000, 0x100000, CRC(40986386) SHA1(65795a50197049681265946713d416c9cdb68f08) )
	ROM_LOAD16_BYTE( "065-c4.bin", 0x200001, 0x100000, CRC(715e15ff) SHA1(ac8b8b01f5c7384b883afbe0cf977430378e3fef) )
ROM_END

ROM_START( karnovr )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "066-p1.bin", 0x000000, 0x100000, CRC(8c86fd22) SHA1(8cf97c6fb9c5717167ccc54bf5856248ccaf32c6) )

	NEO_SFIX_128K( "066-s1.bin", CRC(bae5d5e5) SHA1(aa69d9b235b781ec51f72a528fada9cb12e72cbc) )

	NEO_BIOS_SOUND_128K( "066-m1.bin", CRC(030beae4) SHA1(ceb6ee6c09514504efacdbca7b280901e4c97084) )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "066-v1.bin", 0x000000, 0x200000, CRC(0b7ea37a) SHA1(34e7d4f6db053674a7e8c8b2e3e398777d5b02e6) )

	NO_DELTAT_REGION

	ROM_REGION( 0xc00000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "066-c1.bin", 0x000000, 0x200000, CRC(09dfe061) SHA1(ca4c0f0ce80967b4be2f18b72435c468bbfbac4c) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "066-c2.bin", 0x000001, 0x200000, CRC(e0f6682a) SHA1(addb4fbc30da2b8ffc86819d92a874eb232f67dd) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "066-c3.bin", 0x400000, 0x200000, CRC(a673b4f7) SHA1(d138f5b38fd65c61549ce36f5c4983f7c8a3e7f6) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "066-c4.bin", 0x400001, 0x200000, CRC(cb3dc5f4) SHA1(865d9ccfc3df517c341d6aac16120f6b6aa759fe) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "066-c5.bin", 0x800000, 0x200000, CRC(9a28785d) SHA1(19723e1f7ff429e8a038d89488b279f830dfaf6e) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "066-c6.bin", 0x800001, 0x200000, CRC(c15c01ed) SHA1(7cf5583e6610bcdc3b332896cefc71df84fb3f19) ) /* Plane 2,3 */
ROM_END

ROM_START( gururin )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "067-p1.bin", 0x000000, 0x80000, CRC(4cea8a49) SHA1(cea4a35db8de898e30eb40dd339b3cbe77ac0856) )

	NEO_SFIX_128K( "067-s1.bin", CRC(4f0cbd58) SHA1(509bad8416a057d5239439e775640b71ccf09ef7) )

	NEO_BIOS_SOUND_64K( "067-m1.bin", CRC(833cdf1b) SHA1(3a92c79adbe0d37956ea46a4746d6f1cbf7d2c14) )

	ROM_REGION( 0x80000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "067-v1.bin", 0x000000, 0x80000, CRC(cf23afd0) SHA1(10f87014ee10613f92b04f482f449721a6379db7) )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "067-c1.bin", 0x000000, 0x200000, CRC(35866126) SHA1(e4b24670ccc7901af5f66b11b15fae4e67f843ab) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "067-c2.bin", 0x000001, 0x200000, CRC(9db64084) SHA1(68a43c12f63f5e98d68ad0902a6551c5d30f8543) ) /* Plane 2,3 */
ROM_END

ROM_START( pspikes2 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "068-p1.bin", 0x000000, 0x100000, CRC(105a408f) SHA1(2ee51defa1c24c66c63a6498ee542ac26de3cfbb) )

	NEO_SFIX_128K( "068-s1.bin", CRC(18082299) SHA1(efe93fabe6a76a5dc8cf12f255e571480afb40a0) )

	NEO_BIOS_SOUND_128K( "068-m1.bin", CRC(b1c7911e) SHA1(27b298e7d50981331e17aa642e2e363ffac4333a) )

	ROM_REGION( 0x300000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "068-v1.bin", 0x000000, 0x100000, CRC(2ced86df) SHA1(d6b73d1f31efbd74fb745200d4dade5f80b71541) )
	ROM_LOAD( "068-v2.bin", 0x100000, 0x100000, CRC(970851ab) SHA1(6c9b04e9cc6b92133f1154e5bdd9d38d8ef050a7) )
	ROM_LOAD( "068-v3.bin", 0x200000, 0x100000, CRC(81ff05aa) SHA1(d74302f38c59055bfc83b39dff798a585314fecd) )

	NO_DELTAT_REGION

	ROM_REGION( 0x600000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "068-c1.bin", 0x000000, 0x100000, CRC(7f250f76) SHA1(5109a41adcb7859e24dc43d88842d4cc18cd3305) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "068-c2.bin", 0x000001, 0x100000, CRC(20912873) SHA1(2df8766b531e47ffc30457e41c63b83557b4f468) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "068-c3.bin", 0x200000, 0x100000, CRC(4b641ba1) SHA1(7a9c42a30163eda455f7bde2302402b1a5de7178) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "068-c4.bin", 0x200001, 0x100000, CRC(35072596) SHA1(4150a21041f06514c97592bd8af686504b06e187) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "068-c5.bin", 0x400000, 0x100000, CRC(151dd624) SHA1(f2690a3fe9c64f70f283df785a5217d5b92a289f) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "068-c6.bin", 0x400001, 0x100000, CRC(a6722604) SHA1(b40c57fb4be93ac0b918829f88393ced3d4f8bde) ) /* Plane 2,3 */
ROM_END

ROM_START( fatfury3 )
	ROM_REGION( 0x300000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "069-p1.bin", 0x000000, 0x100000, CRC(a8bcfbbc) SHA1(519c4861151797e5f4d4f33432b83dfabed8e7c4) )
	ROM_LOAD16_WORD_SWAP( "069-p2.bin", 0x100000, 0x200000, CRC(dbe963ed) SHA1(8ece7f663cfe8e563576a397e41161d392cee67e) )

	NEO_SFIX_128K( "069-s1.bin", CRC(0b33a800) SHA1(b7d2cc97da4f30ddebc7b801f5e1d17d2306b2db) )

	NEO_BIOS_SOUND_128K( "069-m1.bin", CRC(fce72926) SHA1(a40c74f793900b8542f0b8383ce4bf46fca112d4) )

	ROM_REGION( 0xa00000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "069-v1.bin", 0x000000, 0x400000, CRC(2bdbd4db) SHA1(5f4fecf69c2329d699cbd45829c19303b1e2a80e) )
	ROM_LOAD( "069-v2.bin", 0x400000, 0x400000, CRC(a698a487) SHA1(11b8bc53bc26a51f4a408e900e3769958625c4ed) )
	ROM_LOAD( "069-v3.bin", 0x800000, 0x200000, CRC(581c5304) SHA1(e9550ec547b4f605afed996b22d711f49b48fa92) )

	NO_DELTAT_REGION

	ROM_REGION( 0x1400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "069-c1.bin", 0x0000000, 0x400000, CRC(e302f93c) SHA1(d8610b14900b2b8fe691b67ca9b1abb335dbff74) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "069-c2.bin", 0x0000001, 0x400000, CRC(1053a455) SHA1(69501bfac68739e63d798045b812badd251d57b8) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "069-c3.bin", 0x0800000, 0x400000, CRC(1c0fde2f) SHA1(cf6c2ef56c03a861de3b0b6dc0d7c9204d947f9d) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "069-c4.bin", 0x0800001, 0x400000, CRC(a25fc3d0) SHA1(83cb349e2f1032652060b233e741fb893be5af16) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "069-c5.bin", 0x1000000, 0x200000, CRC(b3ec6fa6) SHA1(7e4c8ee9dd8d9a25ff183d9d8b05f38769348bc7) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "069-c6.bin", 0x1000001, 0x200000, CRC(69210441) SHA1(6d496c549dba65caabeaffe5b762e86f9d648a26) ) /* Plane 2,3 */
ROM_END

ROM_START( panicbom )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "073-p1.bin", 0x000000, 0x040000, CRC(0b21130d) SHA1(885c67347d33c0a4cd8c91b45c72959900d707a5) )

	NEO_SFIX_128K( "073-s1.bin", CRC(b876de7e) SHA1(910347d7657470da914fb0a6b0ea02891e13c081) )

	NEO_BIOS_SOUND_128K( "073-m1.bin", CRC(3cdf5d88) SHA1(6d8365a946fbd0b7c7b896536322638d80f6a764) )

	ROM_REGION( 0x300000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "073-v1.bin", 0x000000, 0x200000, CRC(7fc86d2f) SHA1(aa4234d22157060e0ba97a09c4e85c5276b74099) )
	ROM_LOAD( "073-v2.bin", 0x200000, 0x100000, CRC(082adfc7) SHA1(19c168e9a6cadcbed79033c320bcf3a45f846daf) )

	NO_DELTAT_REGION

	ROM_REGION( 0x200000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "073-c1.bin", 0x000000, 0x100000, CRC(8582e1b5) SHA1(e17d8f57b8ebee14b8e705374b34abe928937258) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "073-c2.bin", 0x000001, 0x100000, CRC(e15a093b) SHA1(548a418c81af79cd7ab6ad165b8d6daee30abb49) ) /* Plane 2,3 */
ROM_END

ROM_START( aodk )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "074-p1.bin", 0x100000, 0x100000, CRC(62369553) SHA1(ca4d561ee08d16fe6804249d1ba49188eb3bd606) )
	ROM_CONTINUE(					   0x000000, 0x100000 )

	NEO_SFIX_128K( "074-s1.bin", CRC(96148d2b) SHA1(47725a8059346ebe5639bbdbf62a2ac8028756a9) )

	NEO_BIOS_SOUND_128K( "074-m1.bin", CRC(5a52a9d1) SHA1(ef913a9a55d29d5dd3beab1ce6039d64ce9b1a5b) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "074-v1.bin", 0x000000, 0x200000, CRC(7675b8fa) SHA1(29f4facf89d551237b31bf779693cbbbc94e1ede) )
	ROM_LOAD( "074-v2.bin", 0x200000, 0x200000, CRC(a9da86e9) SHA1(ff65af61e42b79a75060a352b24077d1fa28c83f) )

	NO_DELTAT_REGION

	ROM_REGION( 0x1000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "074-c1.bin", 0x000000, 0x200000, CRC(a0b39344) SHA1(adfff7b8836347abf030611563e6068a91164d0a) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "074-c2.bin", 0x000001, 0x200000, CRC(203f6074) SHA1(737f2d707d504df1da1ca5c5cf61cf489a33eb56) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "074-c3.bin", 0x400000, 0x200000, CRC(7fff4d41) SHA1(bebd18a75adeb34c3bbd49cfc8fd3d8c2bf9e475) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "074-c4.bin", 0x400001, 0x200000, CRC(48db3e0a) SHA1(a88505e001e01bb45fb26beda5af24943d02552a) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "074-c5.bin", 0x800000, 0x200000, CRC(c74c5e51) SHA1(0399c53e2a3d721901dddc073fda6ec22e02dfd4) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "074-c6.bin", 0x800001, 0x200000, CRC(73e8e7e0) SHA1(dd6580227743e6a3db4950456ebe870008e022b2) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "074-c7.bin", 0xc00000, 0x200000, CRC(ac7daa01) SHA1(78407a464f67d949933ce2ccaa23fbed80dff1ea) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "074-c8.bin", 0xc00001, 0x200000, CRC(14e7ad71) SHA1(d4583fbce361fd1a11ac6c1a27b0b669e8a5c718) ) /* Plane 2,3 */
ROM_END

ROM_START( sonicwi2 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "075-p1.bin", 0x100000, 0x100000, CRC(92871738) SHA1(fed040a7c1ff9e495109813a702d09fb1d2ecf3a) )
	ROM_CONTINUE(                         0x000000, 0x100000 )

	NEO_SFIX_128K( "075-s1.bin", CRC(c9eec367) SHA1(574e1afe7e0d54610c145131106e59ba2894eeb7) )

	NEO_BIOS_SOUND_128K( "075-m1.bin", CRC(bb828df1) SHA1(eab8e2868173bdaac7c7ed97305a9aa1033fd303) )

	ROM_REGION( 0x280000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "075-v1.bin", 0x000000, 0x200000, CRC(7577e949) SHA1(3ba9f11094dd0cf519f33a16016cfae0d2c6629c) )
	ROM_LOAD( "075-v2.bin", 0x200000, 0x080000, CRC(6d0a728e) SHA1(9d226b9af855d0ae1a91ace7c362fa51ced8b243) )

	NO_DELTAT_REGION

	ROM_REGION( 0x800000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "075-c1.bin", 0x000000, 0x200000, CRC(3278e73e) SHA1(d9e6c8a3a5213690a1b8747d27806d8ac5aac405) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "075-c2.bin", 0x000001, 0x200000, CRC(fe6355d6) SHA1(ca72fff7a908b6d9325761079ff2a0e28f34cf89) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "075-c3.bin", 0x400000, 0x200000, CRC(c1b438f1) SHA1(b3751c5b426bca0fcc3a58bdb86712c22ef908ab) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "075-c4.bin", 0x400001, 0x200000, CRC(1f777206) SHA1(e29c5ae65ebdcc1167a894306d2446ce909639da) ) /* Plane 2,3 */
ROM_END

ROM_START( zedblade )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "076-p1.bin", 0x000000, 0x080000, CRC(d7c1effd) SHA1(485c2308a40baecd122be9ab4996044622bdcc7e) )

	NEO_SFIX_128K( "076-s1.bin", CRC(f4c25dd5) SHA1(8ec9026219f393930634f9170edbaaee479f875e) )

	NEO_BIOS_SOUND_128K( "076-m1.bin", CRC(7b5f3d0a) SHA1(4a301781a57ff236f49492b576ff4858b0ffbdf8) )

	ROM_REGION( 0x500000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "076-v1.bin", 0x000000, 0x200000, CRC(1a21d90c) SHA1(4793ab06421228ad122e359653ed0f1339b90c7a) )
	ROM_LOAD( "076-v2.bin", 0x200000, 0x200000, CRC(b61686c3) SHA1(5a3405e833ce36abb7421190438b5cccc8537919) )
	ROM_LOAD( "076-v3.bin", 0x400000, 0x100000, CRC(b90658fa) SHA1(b9a4b34565ce3688495c47e35c9b888ef686ae9f) )

	NO_DELTAT_REGION

	ROM_REGION( 0x800000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "076-c1.bin", 0x000000, 0x200000, CRC(4d9cb038) SHA1(c0b52b32e1fa719b99ae242d61d5dbea1437331c) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "076-c2.bin", 0x000001, 0x200000, CRC(09233884) SHA1(1895cd0d126a022bce1cc4c7a569032d89f35e3f) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "076-c3.bin", 0x400000, 0x200000, CRC(d06431e3) SHA1(643bd1ad74af272795b02143ba80a76e375036ab) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "076-c4.bin", 0x400001, 0x200000, CRC(4b1c089b) SHA1(cd63961d88c5be84673cce83c683a86b222a064d) ) /* Plane 2,3 */
ROM_END

ROM_START( galaxyfg )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "078-p1.bin", 0x100000, 0x100000, CRC(45906309) SHA1(cdcd96a564acf42e959193e139e149b29c103e25) )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "078-s1.bin", CRC(72f8923e) SHA1(da908bffc2b5d8baa2002dbb5bfb3aa17d2472b7) )

	NEO_BIOS_SOUND_128K( "078-m1.bin", CRC(8e9e3b10) SHA1(7c44d1dbd4f8d337c99e90361d1dab837df85e31) )

	ROM_REGION( 0x500000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "078-v1.bin", 0x000000, 0x200000, CRC(e3b735ac) SHA1(e16dfac09aef8115a20bae0bef8c86d4e7d0dc4a) )
	ROM_LOAD( "078-v2.bin", 0x200000, 0x200000, CRC(6a8e78c2) SHA1(f60b1f8a3a945f279a582745e82f37278ce5d83b) )
	ROM_LOAD( "078-v3.bin", 0x400000, 0x100000, CRC(70bca656) SHA1(218b7079c90898e7faa382b386e77f81f415e7ac) )

	NO_DELTAT_REGION

	ROM_REGION( 0xe00000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "078-c1.bin", 0x000000, 0x200000, CRC(c890c7c0) SHA1(b96c18a41c34070a4f24ca77cb7516fae8b0fd0c) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "078-c2.bin", 0x000001, 0x200000, CRC(b6d25419) SHA1(e089df9c9a9645f706e501108d634f4d222622a2) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "078-c3.bin", 0x400000, 0x200000, CRC(9d87e761) SHA1(ea1b6d7c9d5ef3a9b48968bde5a52d5699d591cc) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "078-c4.bin", 0x400001, 0x200000, CRC(765d7cb8) SHA1(7b9c86714d688602064d928c9d2b49d70bb7541e) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "078-c5.bin", 0x800000, 0x200000, CRC(e6b77e6a) SHA1(db3b8fc62a6f21c6653621c0665450d5d9a9913d) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "078-c6.bin", 0x800001, 0x200000, CRC(d779a181) SHA1(2761026abd9698a7b56114b76631563abd41fd12) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "078-c7.bin", 0xc00000, 0x100000, CRC(4f27d580) SHA1(c0f12496b45b2fe6e94aa8ac52b0157063127e0a) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "078-c8.bin", 0xc00001, 0x100000, CRC(0a7cc0d8) SHA1(68aaee6341c87e56ce11acc1c4ec8047839fe70d) ) /* Plane 2,3 */
ROM_END

ROM_START( strhoop )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "079-p1.bin", 0x000000, 0x100000, CRC(5e78328e) SHA1(7a00b096ed6dd77afc3008c5a4c83686e475f323) )

	NEO_SFIX_128K( "079-s1.bin", CRC(3ac06665) SHA1(ba9ab51eb95c3568304377ef6d7b5f32e8fbcde1) )

	NEO_BIOS_SOUND_64K( "079-m1.bin", CRC(1a5f08db) SHA1(3121ed568fba4c30794b00d326ddb0c750b7f4ee) )

	ROM_REGION( 0x280000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "079-v1.bin", 0x000000, 0x200000, CRC(718a2400) SHA1(cefc5d0b302bd4a87ab1fa244ade4482c23c6806) )
	ROM_LOAD( "079-v2.bin", 0x200000, 0x080000, CRC(b19884f8) SHA1(5fe910f2029da19ddab4dc95c2292d7fbb086741) )

	NO_DELTAT_REGION

	ROM_REGION( 0x800000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "079-c1.bin", 0x000000, 0x200000, CRC(0581c72a) SHA1(453f7a8474195a1120da5fa24337d79674563d9e) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "079-c2.bin", 0x000001, 0x200000, CRC(5b9b8fb6) SHA1(362aa0de0d2cf9aa03758363ffb1e15e046a3930) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "079-c3.bin", 0x400000, 0x200000, CRC(cd65bb62) SHA1(6f47d77d61d4289bcee82df7c4efa5346a6e4c80) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "079-c4.bin", 0x400001, 0x200000, CRC(a4c90213) SHA1(1b9f7b5f31acd6df2bdab81b849f32c13aa1b884) ) /* Plane 2,3 */
ROM_END

ROM_START( quizkof )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "080-p1.bin", 0x000000, 0x100000, CRC(4440315e) SHA1(f4adba8e341d64a1f6280dfd98ebf6918c00608d) )

	NEO_SFIX_128K( "080-s1.bin", CRC(d7b86102) SHA1(09e1ca6451f3035ce476e3b045541646f860aad5) )

	NEO_BIOS_SOUND_128K( "080-m1.bin", CRC(f5f44172) SHA1(eaaba1781622901b91bce9257be4e05f84df053b) )

	ROM_REGION( 0x600000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "080-v1.bin", 0x000000, 0x200000, CRC(0be18f60) SHA1(05c8b7d9f5a8583015f31902ad16d9c621f47d4e) )
	ROM_LOAD( "080-v2.bin", 0x200000, 0x200000, CRC(4abde3ff) SHA1(0188bfcafa9a1aac302705736a2bcb26b9d684c2) )
	ROM_LOAD( "080-v3.bin", 0x400000, 0x200000, CRC(f02844e2) SHA1(8c65ebe146f4ddb6c904f8125cb32767f74c24d5) )

	NO_DELTAT_REGION

	ROM_REGION( 0x800000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "080-c1.bin",  0x000000, 0x200000, CRC(ea1d764a) SHA1(78cc1735624c37f90607baa92e110a3c5cc54c6f) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "080-c2.bin",  0x000001, 0x200000, CRC(c78c49da) SHA1(0b95a340842847ab304517060e506098f8e5f0e0) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "080-c3.bin",  0x400000, 0x200000, CRC(b4851bfe) SHA1(b8286c601de5755c1681ea46e177fc89006fc066) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "080-c4.bin",  0x400001, 0x200000, CRC(ca6f5460) SHA1(ed36e244c9335f4c0a97c57b7b7f1b849dd3a90d) ) /* Plane 2,3 */
ROM_END

ROM_START( ssideki3 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "081-p1.bin", 0x100000, 0x100000, CRC(6bc27a3d) SHA1(94692abe7343f9204a557acae4ab74d0af511ca3) )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "081-s1.bin", CRC(7626da34) SHA1(30bad65633d0035fd578323c22cbddb8c9d549a6) )

	NEO_BIOS_SOUND_128K( "081-m1.bin", CRC(82fcd863) SHA1(b219a5685450f9c24cc195f1c914bc3b292d72c0) )

	ROM_REGION( 0x600000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "081-v1.bin", 0x000000, 0x200000, CRC(201fa1e1) SHA1(9c27cc1b1d075223ed4a90dd02571d09a2f0d076) )
	ROM_LOAD( "081-v2.bin", 0x200000, 0x200000, CRC(acf29d96) SHA1(5426985c33aea2efc8ff774b59d34d8b03bd9a85) )
	ROM_LOAD( "081-v3.bin", 0x400000, 0x200000, CRC(e524e415) SHA1(8733e1b63471381b16c2b7c64b909745d99c8925) )

	NO_DELTAT_REGION

	ROM_REGION( 0xc00000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "081-c1.bin", 0x000000, 0x200000, CRC(1fb68ebe) SHA1(abd9dbe7b7cbe0b6cd1d87e53c6bdc6edeccf83c) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "081-c2.bin", 0x000001, 0x200000, CRC(b28d928f) SHA1(9f05148e3e1e94339752658c066f47f133db8fbf) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "081-c3.bin", 0x400000, 0x200000, CRC(3b2572e8) SHA1(41aba1554bf59d4e5d5814249eaa0d531449e1de) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "081-c4.bin", 0x400001, 0x200000, CRC(47d26a7c) SHA1(591ef24a3d381163c5da80fa64e6883b8ea9abfb) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "081-c5.bin", 0x800000, 0x200000, CRC(17d42f0d) SHA1(7de7765bf43d390c50b2f59c2288502a7121d086) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "081-c6.bin", 0x800001, 0x200000, CRC(6b53fb75) SHA1(fadf7a12661d83ae35d9258aa4947969d51c08b8) ) /* Plane 2,3 */
ROM_END

ROM_START( doubledr )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "082-p1.bin", 0x100000, 0x100000, CRC(34ab832a) SHA1(fbb1bd195f5653f7b9c89648649f838eaf83cbe4) )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "082-s1.bin", CRC(bef995c5) SHA1(9c89adbdaa5c1f827632c701688563dac2e482a4) )

	NEO_BIOS_SOUND_128K( "082-m1.bin", CRC(10b144de) SHA1(cf1ed0a447da68240c62bcfd76b1569803f6bf76) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "082-v1.bin", 0x000000, 0x200000, CRC(cc1128e4) SHA1(bfcfff24bc7fbde0b02b1bc0dffebd5270a0eb04) )
	ROM_LOAD( "082-v2.bin", 0x200000, 0x200000, CRC(c3ff5554) SHA1(c685887ad64998e5572607a916b023f8b9efac49) )

	NO_DELTAT_REGION

	ROM_REGION( 0xe00000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "082-c1.bin", 0x000000, 0x200000, CRC(b478c725) SHA1(3a777c5906220f246a6dc06cb084e6ad650d67bb) )
	ROM_LOAD16_BYTE( "082-c2.bin", 0x000001, 0x200000, CRC(2857da32) SHA1(9f13245965d23db86d46d7e73dfb6cc63e6f25a1) )
	ROM_LOAD16_BYTE( "082-c3.bin", 0x400000, 0x200000, CRC(8b0d378e) SHA1(3a347215e414b738164f1fe4144102f07d4ffb80) )
	ROM_LOAD16_BYTE( "082-c4.bin", 0x400001, 0x200000, CRC(c7d2f596) SHA1(e2d09d4d1b1fef9c0c53ecf3629e974b75e559f5) )
	ROM_LOAD16_BYTE( "082-c5.bin", 0x800000, 0x200000, CRC(ec87bff6) SHA1(3fa86da93881158c2c23443855922a7b32e55135) )
	ROM_LOAD16_BYTE( "082-c6.bin", 0x800001, 0x200000, CRC(844a8a11) SHA1(b2acbd4cacce66fb32c052b2fba9984904679bda) )
	ROM_LOAD16_BYTE( "082-c7.bin", 0xc00000, 0x100000, CRC(727c4d02) SHA1(8204c7f037d46e0c58f269f9c7a535bc2589f526) )
	ROM_LOAD16_BYTE( "082-c8.bin", 0xc00001, 0x100000, CRC(69a5fa37) SHA1(020e70e0e8b3c5d00a40fe97e418115a3187e50a) )
ROM_END

ROM_START( pbobblen )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "083-p1.bin", 0x000000, 0x040000, CRC(7c3c34e1) SHA1(50fafb3529351c82a3656e6b07ab174a7bf8da0b) )

	NEO_SFIX_128K( "083-s1.bin", CRC(9caae538) SHA1(cf2d90a7c1a42107c0bb8b9a61397634286dbe0a) )

	NEO_BIOS_SOUND_64K( "083-m1.bin", CRC(129e6054) SHA1(4edd18c44759e7a140705f1544413c649131c551) )

	ROM_REGION( 0x380000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	/* 0x000000-0x1fffff empty */
	ROM_LOAD( "083-v3.bin", 0x200000, 0x100000, CRC(0840cbc4) SHA1(1adbd7aef44fa80832f63dfb8efdf69fd7256a57) )
	ROM_LOAD( "083-v4.bin", 0x300000, 0x080000, CRC(0a548948) SHA1(e1e4afd17811cb60401c14fbcf0465035165f4fb) )

	NO_DELTAT_REGION

	ROM_REGION( 0x100000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "083-c5.bin", 0x000000, 0x080000, CRC(e89ad494) SHA1(69c9ea415773af94ac44c48af05d55ada222b138) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "083-c6.bin", 0x000001, 0x080000, CRC(4b42d7eb) SHA1(042ae50a528cea21cf07771d3915c57aa16fd5af) ) /* Plane 2,3 */
ROM_END

ROM_START( pbobblna )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "083-p1.rom", 0x000000, 0x040000, CRC(d6efe86f) SHA1(f80a0b291c2e88dd894851bd14fa8cf7523fb7fe) )

	NEO_SFIX_128K( "083-s1.bin", CRC(9caae538) SHA1(cf2d90a7c1a42107c0bb8b9a61397634286dbe0a) )

	NEO_BIOS_SOUND_64K( "083-m1.bin", CRC(129e6054) SHA1(4edd18c44759e7a140705f1544413c649131c551) )

	ROM_REGION( 0x380000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	/* 0x000000-0x1fffff empty */
	ROM_LOAD( "083-v3.bin", 0x200000, 0x100000, CRC(0840cbc4) SHA1(1adbd7aef44fa80832f63dfb8efdf69fd7256a57) )
	ROM_LOAD( "083-v4.bin", 0x300000, 0x080000, CRC(0a548948) SHA1(e1e4afd17811cb60401c14fbcf0465035165f4fb) )

	NO_DELTAT_REGION

	ROM_REGION( 0x100000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "083-c5.bin", 0x000000, 0x080000, CRC(e89ad494) SHA1(69c9ea415773af94ac44c48af05d55ada222b138) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "083-c6.bin", 0x000001, 0x080000, CRC(4b42d7eb) SHA1(042ae50a528cea21cf07771d3915c57aa16fd5af) ) /* Plane 2,3 */
ROM_END

ROM_START( kof95 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "084-p1.bin", 0x100000, 0x100000, CRC(5e54cf95) SHA1(41abe2042fdbb1526e92a0789976a9b1ac5e60f0) )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "084-s1.bin", CRC(de716f8a) SHA1(f7386454a943ed5caf625f67ee1d0197b1c6fa13) )

	NEO_BIOS_SOUND_128K( "084-m1.bin", CRC(6f2d7429) SHA1(6f8462e4f07af82a5ca3197895d5dcbb67bdaa61) )

	ROM_REGION( 0x900000, REGION_SOUND1, ROMREGION_SOUNDONLY )
/*	ROM_LOAD( "084-v1.bin", 0x000000, 0x400000, CRC(21469561) SHA1(f35c72d31f026efc9e74bc4f198a123999ab3fc3) ) */ /* bad old rom ?*/
 	ROM_LOAD( "084-v1.bin", 0x000000, 0x400000, CRC(84861b56) SHA1(1b6c91ddaed01f45eb9b7e49d9c2b9b479d50da6) )
	ROM_LOAD( "084-v2.bin", 0x400000, 0x200000, CRC(b38a2803) SHA1(dbc2c8606ca09ed7ff20906b022da3cf053b2f09) )
	/* 600000-7fffff empty */
	ROM_LOAD( "084-v3.bin", 0x800000, 0x100000, CRC(d683a338) SHA1(eb9866b4b286edc09963cb96c43ce0a8fb09adbb) )

	NO_DELTAT_REGION

	ROM_REGION( 0x1a00000, REGION_GFX3, 0 )
	/* old set had the roms like this */
#if 0
	ROM_LOAD16_BYTE( "084-c1.bin", 0x0400000, 0x200000, CRC(33bf8657) SHA1(79b0f0eb4e5c172f36c296ccabeb474804f7645a) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0000000, 0x200000 )
	ROM_LOAD16_BYTE( "084-c2.bin", 0x0400001, 0x200000, CRC(f21908a4) SHA1(a0bec5961396e62af553ba5293b1007cdf9fbf62) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0000001, 0x200000 )
	ROM_LOAD16_BYTE( "084-c3.bin", 0x0c00000, 0x200000, CRC(0cee1ddb) SHA1(f63c05020c5bee37c2598fd235d76157baea6c68) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0800000, 0x200000 )
	ROM_LOAD16_BYTE( "084-c4.bin", 0x0c00001, 0x200000, CRC(729db15d) SHA1(6167a601463d7aaba1c8d470b346a82e9aef8bb5) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0800001, 0x200000 )
#endif
 	ROM_LOAD16_BYTE( "084-c1.bin", 0x0000000, 0x400000, CRC(fe087e32) SHA1(e8e89faa616027e4fb9b8a865c1a67f409c93bdf) ) /* Plane 0,1 */
 	ROM_LOAD16_BYTE( "084-c2.bin", 0x0000001, 0x400000, CRC(07864e09) SHA1(0817fcfd75d0735fd8ff27561eaec371e4ff5829) ) /* Plane 2,3 */
 	ROM_LOAD16_BYTE( "084-c3.bin", 0x0800000, 0x400000, CRC(a4e65d1b) SHA1(740a405b40b3a4b324697d2652cae29ffe0ac0bd) ) /* Plane 0,1 */
 	ROM_LOAD16_BYTE( "084-c4.bin", 0x0800001, 0x400000, CRC(c1ace468) SHA1(74ea2a3cfd7b744f0988a05baaff10016ca8f625) ) /* Plane 2,3 */

	ROM_LOAD16_BYTE( "084-c5.bin", 0x1000000, 0x200000, CRC(8a2c1edc) SHA1(67866651bc0ce27122285a66b0aab108acf3d065) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "084-c6.bin", 0x1000001, 0x200000, CRC(f593ac35) SHA1(302c92c63f092a8d49429c3331e5e5678f0ea48d) ) /* Plane 2,3 */
	/* 1400000-17fffff empty */
	ROM_LOAD16_BYTE( "084-c7.bin", 0x1800000, 0x100000, CRC(9904025f) SHA1(eec770746a0ad073f7d353ab16a2cc3a5278d307) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "084-c8.bin", 0x1800001, 0x100000, CRC(78eb0f9b) SHA1(2925ea21ed2ce167f08a25589e94f28643379034) ) /* Plane 2,3 */
ROM_END

ROM_START( kof95a )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
 	ROM_LOAD16_WORD_SWAP( "084a-p1.bin",0x100000, 0x100000, CRC(2cba2716) SHA1(f6c2d0537c9c3e0938065c65b1797c47198fcff8) )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "084-s1.bin", CRC(de716f8a) SHA1(f7386454a943ed5caf625f67ee1d0197b1c6fa13) )

	NEO_BIOS_SOUND_128K( "084-m1.bin", CRC(6f2d7429) SHA1(6f8462e4f07af82a5ca3197895d5dcbb67bdaa61) )

	ROM_REGION( 0x900000, REGION_SOUND1, ROMREGION_SOUNDONLY )
 	ROM_LOAD( "084-v1.bin", 0x000000, 0x400000, CRC(84861b56) SHA1(1b6c91ddaed01f45eb9b7e49d9c2b9b479d50da6) )
	ROM_LOAD( "084-v2.bin", 0x400000, 0x200000, CRC(b38a2803) SHA1(dbc2c8606ca09ed7ff20906b022da3cf053b2f09) )
	/* 600000-7fffff empty */
	ROM_LOAD( "084-v3.bin", 0x800000, 0x100000, CRC(d683a338) SHA1(eb9866b4b286edc09963cb96c43ce0a8fb09adbb) )

	NO_DELTAT_REGION

	ROM_REGION( 0x1a00000, REGION_GFX3, 0 )
 	ROM_LOAD16_BYTE( "084-c1.bin", 0x0000000, 0x400000, CRC(fe087e32) SHA1(e8e89faa616027e4fb9b8a865c1a67f409c93bdf) ) /* Plane 0,1 */
 	ROM_LOAD16_BYTE( "084-c2.bin", 0x0000001, 0x400000, CRC(07864e09) SHA1(0817fcfd75d0735fd8ff27561eaec371e4ff5829) ) /* Plane 2,3 */
 	ROM_LOAD16_BYTE( "084-c3.bin", 0x0800000, 0x400000, CRC(a4e65d1b) SHA1(740a405b40b3a4b324697d2652cae29ffe0ac0bd) ) /* Plane 0,1 */
 	ROM_LOAD16_BYTE( "084-c4.bin", 0x0800001, 0x400000, CRC(c1ace468) SHA1(74ea2a3cfd7b744f0988a05baaff10016ca8f625) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "084-c5.bin", 0x1000000, 0x200000, CRC(8a2c1edc) SHA1(67866651bc0ce27122285a66b0aab108acf3d065) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "084-c6.bin", 0x1000001, 0x200000, CRC(f593ac35) SHA1(302c92c63f092a8d49429c3331e5e5678f0ea48d) ) /* Plane 2,3 */
	/* 1400000-17fffff empty */
	ROM_LOAD16_BYTE( "084-c7.bin", 0x1800000, 0x100000, CRC(9904025f) SHA1(eec770746a0ad073f7d353ab16a2cc3a5278d307) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "084-c8.bin", 0x1800001, 0x100000, CRC(78eb0f9b) SHA1(2925ea21ed2ce167f08a25589e94f28643379034) ) /* Plane 2,3 */
ROM_END

ROM_START( tws96 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "086-p1.bin", 0x000000, 0x100000, CRC(03e20ab6) SHA1(3a0a5a54649178ce7a6158980cb4445084b40fb5) )

	NEO_SFIX_128K( "086-s1.bin", CRC(6f5e2b3a) SHA1(273341489f6625d35a4a920042a60e2b86373847) )

	NEO_BIOS_SOUND_64K( "086-m1.bin", CRC(860ba8c7) SHA1(6457964fd2fdda1d4f57787fff0cec76b47692cb) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "086-v1.bin", 0x000000, 0x200000, CRC(97bf1986) SHA1(b80d3a37e18d0a52f1e0092dc300989c9647efd1) )
	ROM_LOAD( "086-v2.bin", 0x200000, 0x200000, CRC(b7eb05df) SHA1(ff2b55c7021c248cfdcfc9cd3658f2896bcbca38) )

	NO_DELTAT_REGION

	ROM_REGION( 0xa00000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "086-c1.bin", 0x400000, 0x200000, CRC(d301a867) SHA1(6ec5ef48943750ac03d7b574b1aa87b84f01dfab) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x000000, 0x200000 )
	ROM_LOAD16_BYTE( "086-c2.bin", 0x400001, 0x200000, CRC(305fc74f) SHA1(27b3c9eb96dadea05a5f3252e425eb605e12c619) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x000001, 0x200000 )
	ROM_LOAD16_BYTE( "086-c3.bin", 0x800000, 0x100000, CRC(750ddc0c) SHA1(9304a83d81afd544d88be0cd3ee47ae401d2da0e) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "086-c4.bin", 0x800001, 0x100000, CRC(7a6e7d82) SHA1(b1bb82cec3d68367d5e01e63c44c11b67e577411) ) /* Plane 2,3 */
ROM_END

ROM_START( samsho3 )
	ROM_REGION( 0x300000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "087-p1.bin", 0x000000, 0x100000, CRC(282a336e) SHA1(e062f1939d36a45f185b5dbd726cdd833dc7c28c) )
	ROM_LOAD16_WORD_SWAP( "087-p2.bin", 0x100000, 0x200000, CRC(9bbe27e0) SHA1(b18117102159903c8e8f4e4226e1cc91a400e816) )

	NEO_SFIX_128K( "087-s1.bin", CRC(74ec7d9f) SHA1(d79c479838a7ca51735a44f91f1968ec5b3c6b91) )

	NEO_BIOS_SOUND_128K( "087-m1.bin", CRC(8e6440eb) SHA1(e3f72150af4e326543b29df71cda27d73ec087c1) )

	ROM_REGION( 0x600000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "087-v1.bin", 0x000000, 0x400000, CRC(84bdd9a0) SHA1(adceceb00569eca13fcc2e0f0d9f0d9b06a06851) )
	ROM_LOAD( "087-v2.bin", 0x400000, 0x200000, CRC(ac0f261a) SHA1(5411bdff24cba7fdbc3397d45a70fb468d7a44b3) )

	NO_DELTAT_REGION

	ROM_REGION( 0x1a00000, REGION_GFX3, 0 )	/* lowering this to 0x1900000 corrupts the graphics */
	ROM_LOAD16_BYTE( "087-c1.bin", 0x0400000, 0x200000, CRC(e079f767) SHA1(0c2e983e68f3a3b25713b2aa8a5f39bc03561672) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0000000, 0x200000 )
	ROM_LOAD16_BYTE( "087-c2.bin", 0x0400001, 0x200000, CRC(fc045909) SHA1(7cb6b5d8abaf6bf54853e28454cc1f518298fdfa) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0000001, 0x200000 )
	ROM_LOAD16_BYTE( "087-c3.bin", 0x0c00000, 0x200000, CRC(c61218d7) SHA1(d92344a019bc5c8091ac4c8e6b7d9b2e99309bbe) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0800000, 0x200000 )
	ROM_LOAD16_BYTE( "087-c4.bin", 0x0c00001, 0x200000, CRC(054ec754) SHA1(324b06a80b6b268781081731756ddb1254c23991) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0800001, 0x200000 )
	ROM_LOAD16_BYTE( "087-c5.bin", 0x1400000, 0x200000, CRC(05feee47) SHA1(d5be7ca85dca73d900a30bb635a531ba39891251) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x1000000, 0x200000 )
	ROM_LOAD16_BYTE( "087-c6.bin", 0x1400001, 0x200000, CRC(ef7d9e29) SHA1(853f73769de504a6747f538829b3f9a0b7e841bc) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x1000001, 0x200000 )
	ROM_LOAD16_BYTE( "087-c7.bin", 0x1800000, 0x080000, CRC(7a01f666) SHA1(d177f165fc7bbd9742e0e236ef8182b48d89e982) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "087-c8.bin", 0x1800001, 0x080000, CRC(ffd009c2) SHA1(1b49e36596ca6bef3d7c943491b496eb759975d7) ) /* Plane 2,3 */
ROM_END

ROM_START( stakwin )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "088-p1.bin",  0x100000, 0x100000, CRC(bd5814f6) SHA1(95179a4dee61ae88bb5d9fd74af0c56c8c29f5ea) )
	ROM_CONTINUE(						 0x000000, 0x100000)

	NEO_SFIX_128K( "088-s1.bin", CRC(073cb208) SHA1(c5b4697d767575884dd49ae416c1fe4a4a92d3f6) )

	NEO_BIOS_SOUND_128K( "088-m1.bin", CRC(2fe1f499) SHA1(5b747eeef65be04423d2db05e086df9132758a47) )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "088-v1.bin", 0x000000, 0x200000, CRC(b7785023) SHA1(d11df1e623434669cd3f97f0feda747b24dac05d) )

	NO_DELTAT_REGION

	ROM_REGION( 0x800000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "088-c1.bin", 0x000000, 0x200000, CRC(6e733421) SHA1(b67c5d2654a62cc4e44bd54d28e62c7da5eea424) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "088-c2.bin", 0x000001, 0x200000, CRC(4d865347) SHA1(ad448cf96f3dce44c83412ed6878c495eb4a8a1e) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "088-c3.bin", 0x400000, 0x200000, CRC(8fa5a9eb) SHA1(7bee19d8a2bccedd8e2cf0c0e9138902b9dafc23) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "088-c4.bin", 0x400001, 0x200000, CRC(4604f0dc) SHA1(ddf5dbb5e07313998a8f695ad19354ea54585dd6) ) /* Plane 2,3 */
ROM_END

ROM_START( pulstar )
	ROM_REGION( 0x300000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "089-p1.bin", 0x000000, 0x100000, CRC(5e5847a2) SHA1(b864d0ec4184b785569ddbf67c2115b5ab86ee3e) )
	ROM_LOAD16_WORD_SWAP( "089-p2.bin", 0x100000, 0x200000, CRC(028b774c) SHA1(fc5da2821a5072f2b78245fc59b6e3eeef116d16) )

	NEO_SFIX_128K( "089-s1.bin", CRC(c79fc2c8) SHA1(914c224fb3c461a68d7425cae724cf22bd5f985d) )

	NEO_BIOS_SOUND_128K( "089-m1.bin", CRC(ff3df7c7) SHA1(59d2ef64f734f6026073b365300221909057a512) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "089-v1.bin", 0x000000, 0x400000, CRC(b458ded2) SHA1(75d659ecb1fd6002188f469fcaafc9907440e624) )
	ROM_LOAD( "089-v2.bin", 0x400000, 0x400000, CRC(9d2db551) SHA1(83f7e5db7fb1502ceadcd334df90b11b1bba78e5) )

	NO_DELTAT_REGION

	ROM_REGION( 0x1c00000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "089-c1.bin", 0x0400000, 0x200000, CRC(63020fc6) SHA1(68053804dabd146b4d39b0f39d63a30d322972f8) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0000000, 0x200000 )
	ROM_LOAD16_BYTE( "089-c2.bin", 0x0400001, 0x200000, CRC(260e9d4d) SHA1(99870c597003a3c70c6b4d11aefe5fe4e58f03f9) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0000001, 0x200000 )
	ROM_LOAD16_BYTE( "089-c3.bin", 0x0c00000, 0x200000, CRC(21ef41d7) SHA1(55b26f0f168da50a16de3de3365365df69c4998d) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0800000, 0x200000 )
	ROM_LOAD16_BYTE( "089-c4.bin", 0x0c00001, 0x200000, CRC(3b9e288f) SHA1(9bbf0ca0981dd44ed0651ffc978d60877802ec3d) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0800001, 0x200000 )
	ROM_LOAD16_BYTE( "089-c5.bin", 0x1400000, 0x200000, CRC(6fe9259c) SHA1(ff3b4da68ed01067dd0f0cde341a0da277b5401c) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x1000000, 0x200000 )
	ROM_LOAD16_BYTE( "089-c6.bin", 0x1400001, 0x200000, CRC(dc32f2b4) SHA1(3ff3d81622946d6ab21a940f2bbc3999855aa6bf) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x1000001, 0x200000 )
	ROM_LOAD16_BYTE( "089-c7.bin", 0x1800000, 0x200000, CRC(6a5618ca) SHA1(9a1d5f998b0dfabacf9dad45c94bef2bb43e5e0c) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "089-c8.bin", 0x1800001, 0x200000, CRC(a223572d) SHA1(2791b1212f57937b2b2a95bc9e420c06d0c37669) ) /* Plane 2,3 */
ROM_END

ROM_START( whp )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "090-p1.bin", 0x100000, 0x100000, CRC(afaa4702) SHA1(83d122fddf17d4774353abf4a0655f3939f7b752) )
	ROM_CONTINUE(					  0x000000, 0x100000 )

	NEO_SFIX_128K( "090-s1.bin",  CRC(174a880f) SHA1(c35d315d728d119a6e9aa42e0593937c90897449) )

	NEO_BIOS_SOUND_128K( "090-m1.bin", CRC(28065668) SHA1(0c60d4afa1dccad0135e733104f056be73b54e4e) )

	ROM_REGION( 0x600000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "090-v1.bin", 0x000000, 0x200000, CRC(30cf2709) SHA1(d1845033f16de2470afd3858ee0efb45176d9ed7) )
	ROM_LOAD( "090-v2.bin", 0x200000, 0x200000, CRC(b6527edd) SHA1(2bcf5bfa6e117cf4a3728a5e5f5771313c93f22a) )
	ROM_LOAD( "090-v3.bin", 0x400000, 0x200000, CRC(1908a7ce) SHA1(78f31bcfea33eb94752bbf5226c481baec1af5ac) )

	NO_DELTAT_REGION

	ROM_REGION( 0x1c00000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "090-c1.bin", 0x0400000, 0x200000, CRC(aecd5bb1) SHA1(9f36deef46c7918417ccfb16abf77659686f80f2) )
	ROM_CONTINUE(      			 0x0000000, 0x200000 )
	ROM_LOAD16_BYTE( "090-c2.bin", 0x0400001, 0x200000, CRC(7566ffc0) SHA1(93f7be5dbf7657e264c434d1dc6dc5d9bd82feb0) )
	ROM_CONTINUE(      			 0x0000001, 0x200000 )
	ROM_LOAD16_BYTE( "090-c3.bin", 0x0800000, 0x200000, CRC(436d1b31) SHA1(059776d77b91377ed0bcfc278802d659c917fc0f) )
	ROM_LOAD16_BYTE( "090-c4.bin", 0x0800001, 0x200000, CRC(f9c8dd26) SHA1(25a9eea1d49b21b4a988beb32c25bf2f7796f227) )
	/* 0c00000-0ffffff empty */
	ROM_LOAD16_BYTE( "090-c5.bin", 0x1000000, 0x200000, CRC(8e34a9f4) SHA1(67b839b426ef3fad0a85d951fdd44c0a45c55226) )
	ROM_LOAD16_BYTE( "090-c6.bin", 0x1000001, 0x200000, CRC(a43e4766) SHA1(54f282f2b1ff2934cca7acbb4386a2b99a29df3a) )
	/* 1400000-17fffff empty */
	ROM_LOAD16_BYTE( "090-c7.bin", 0x1800000, 0x200000, CRC(59d97215) SHA1(85a960dc7f364df13ee0c2f99a4c53aefb081486) )
	ROM_LOAD16_BYTE( "090-c8.bin", 0x1800001, 0x200000, CRC(fc092367) SHA1(69ff4ae909dd857de3ca8645d63f8b4bde117448) )
ROM_END

ROM_START( kabukikl )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "092-p1.bin", 0x100000, 0x100000, CRC(28ec9b77) SHA1(7cdc789a99f8127f437d68cbc41278c926be9efd) )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "092-s1.bin", CRC(a3d68ee2) SHA1(386f6110a16967a72fbf788f9d968fddcdcd2889) )

	NEO_BIOS_SOUND_128K( "092-m1.bin", CRC(91957ef6) SHA1(7b6907532a0e02ceb643623cbd689cf228776ed1) )

	ROM_REGION( 0x700000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "092-v1.bin", 0x000000, 0x200000, CRC(69e90596) SHA1(1a2007d7784b3ce90d115980c3353862f1664d45) )
	ROM_LOAD( "092-v2.bin", 0x200000, 0x200000, CRC(7abdb75d) SHA1(0bff764889fe02f37877514c7fc450250839f632) )
	ROM_LOAD( "092-v3.bin", 0x400000, 0x200000, CRC(eccc98d3) SHA1(b0dfbdb1ea045cb961323ac6906ab342256c3dc7) )
	ROM_LOAD( "092-v4.bin", 0x600000, 0x100000, CRC(a7c9c949) SHA1(574bc55b45e81ce357b14f5992426115de25cd35) )

	NO_DELTAT_REGION

	ROM_REGION( 0x1000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "092-c1.bin", 0x400000, 0x200000, CRC(4d896a58) SHA1(03567f31de0fa947264f49817370345e7023c2c4) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x000000, 0x200000 )
	ROM_LOAD16_BYTE( "092-c2.bin", 0x400001, 0x200000, CRC(3cf78a18) SHA1(82ab69d0899ed2d79e2097f842883dbd542c6f59) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x000001, 0x200000 )
	ROM_LOAD16_BYTE( "092-c3.bin", 0xc00000, 0x200000, CRC(58c454e7) SHA1(64dca760e473fabada869037d6c24cbc58663911) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x800000, 0x200000 )
	ROM_LOAD16_BYTE( "092-c4.bin", 0xc00001, 0x200000, CRC(e1a8aa6a) SHA1(3e33d6e0d6a0feb8cc43dc511a2792922742547d) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x800001, 0x200000 )
ROM_END

ROM_START( neobombe )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "093-p1.bin", 0x000000, 0x100000, CRC(a1a71d0d) SHA1(059284c84f61a825923d86d2f29c91baa2c439cd) )

	NEO_SFIX_128K( "093-s1.bin", CRC(4b3fa119) SHA1(41cb0909bfb017eb6f2c530cb92a423319ed7ab1) )

	NEO_BIOS_SOUND_128K( "093-m1.bin", CRC(e81e780b) SHA1(c56c53984e0f92e180e850c60a75f550ee84917c) )

	ROM_REGION( 0x600000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "093-v1.bin", 0x200000, 0x200000, CRC(43057e99) SHA1(b24a44daf54ec76801e7dc863645022dc2d4abdb) )
	ROM_CONTINUE(			  0x000000, 0x200000 )
	ROM_LOAD( "093-v2.bin", 0x400000, 0x200000, CRC(a92b8b3d) SHA1(b672c97b85d2f52eba3cb26025008ebc7a18312a) )

	NO_DELTAT_REGION

	ROM_REGION( 0x900000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "093-c1.bin", 0x400000, 0x200000, CRC(b90ebed4) SHA1(e9e441fb9b425505e500e0e0b40ef11a43b2d4b2) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x000000, 0x200000 )
	ROM_LOAD16_BYTE( "093-c2.bin", 0x400001, 0x200000, CRC(41e62b4f) SHA1(2fb1f752643d7dd3470ade76229e3756818412f7) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x000001, 0x200000 )
	ROM_LOAD16_BYTE( "093-c3.bin", 0x800000, 0x080000, CRC(e37578c5) SHA1(20024caa0f09ee887a6418dd02d02a0df93786fd) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "093-c4.bin", 0x800001, 0x080000, CRC(59826783) SHA1(0110a2b6186cca95f75225d4d0269d61c2ad25b1) ) /* Plane 2,3 */
ROM_END

ROM_START( gowcaizr )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "094-p1.bin", 0x100000, 0x100000, CRC(33019545) SHA1(213db6c0b7d24b74b809854f9c606dbea1d9ba00) )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "094-s1.bin", CRC(2f8748a2) SHA1(5cc723c4284120473d63d8b0c1a3b3be74bdc324) )

	NEO_BIOS_SOUND_128K( "094-m1.bin", CRC(78c851cb) SHA1(a9923c002e4e2171a564af45cff0958c5d57b275) )

	ROM_REGION( 0x500000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "094-v1.bin", 0x000000, 0x200000, CRC(6c31223c) SHA1(ede3a2806d7d872a0f737626a23ecce200b534e6) )
	ROM_LOAD( "094-v2.bin", 0x200000, 0x200000, CRC(8edb776c) SHA1(a9eac5e24f83ccdcf303d63261747b1bad876a24) )
	ROM_LOAD( "094-v3.bin", 0x400000, 0x100000, CRC(c63b9285) SHA1(6bbbacfe899e204e74657d6c3f3d05ce75e432f1) )

	NO_DELTAT_REGION

	ROM_REGION( 0x1000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "094-c1.bin", 0x000000, 0x200000, CRC(042f6af5) SHA1(1c50df6a1a53ffb3079ea0a19c746f5c9536a3ed) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "094-c2.bin", 0x000001, 0x200000, CRC(0fbcd046) SHA1(9a6dc920a877f27424477c3478907b23afbaa5ea) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "094-c3.bin", 0x400000, 0x200000, CRC(58bfbaa1) SHA1(4c6f9cf138c5e6dfe89a45e2a690a986c75f5bfc) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "094-c4.bin", 0x400001, 0x200000, CRC(9451ee73) SHA1(7befee4a886b1d7493c06cefb7abf4ec01c14a8b) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "094-c5.bin", 0x800000, 0x200000, CRC(ff9cf48c) SHA1(5f46fb5d0812275b0006919d8540f22be7c16492) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "094-c6.bin", 0x800001, 0x200000, CRC(31bbd918) SHA1(7ff8c5e3f17d40e7a8a189ad8f8026de55368810) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "094-c7.bin", 0xc00000, 0x200000, CRC(2091ec04) SHA1(a81d4bdbef1ac6ea49845dc30e31bf9745694100) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "094-c8.bin", 0xc00001, 0x200000, CRC(0d31dee6) SHA1(4979aa3daa7e490fbe39e7b3c70cbb2ef7551c5f) ) /* Plane 2,3 */
ROM_END

ROM_START( rbff1 )
	ROM_REGION( 0x300000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "095-p1.bin", 0x000000, 0x100000, CRC(63b4d8ae) SHA1(03aa9f6bab6aee685d1b57a52823797704eea845) )
	ROM_LOAD16_WORD_SWAP( "095-p2.bin", 0x100000, 0x200000, CRC(cc15826e) SHA1(44d6ac6c0ca697a6f367dcfd809b1e1771cb0635) )

	NEO_SFIX_128K( "095-s1.bin", CRC(b6bf5e08) SHA1(b527355c35ea097f3448676f2ffa65b8e56ae30c) )

	NEO_BIOS_SOUND_128K( "095-m1.bin", CRC(653492a7) SHA1(39e511fb9ed5d2135dc8428a31d0baafb2ab36e0) )

	ROM_REGION( 0xc00000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "069-v1.bin", 0x000000, 0x400000, CRC(2bdbd4db) SHA1(5f4fecf69c2329d699cbd45829c19303b1e2a80e) )
	ROM_LOAD( "069-v2.bin", 0x400000, 0x400000, CRC(a698a487) SHA1(11b8bc53bc26a51f4a408e900e3769958625c4ed) )
	ROM_LOAD( "095-v3.bin", 0x800000, 0x400000, CRC(189d1c6c) SHA1(f0b8cd1ee40ea3feeb2800f0723b451ec8240203) )

	NO_DELTAT_REGION

	ROM_REGION( 0x1c00000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "069-c1.bin", 0x0000000, 0x400000, CRC(e302f93c) SHA1(d8610b14900b2b8fe691b67ca9b1abb335dbff74) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "069-c2.bin", 0x0000001, 0x400000, CRC(1053a455) SHA1(69501bfac68739e63d798045b812badd251d57b8) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "069-c3.bin", 0x0800000, 0x400000, CRC(1c0fde2f) SHA1(cf6c2ef56c03a861de3b0b6dc0d7c9204d947f9d) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "069-c4.bin", 0x0800001, 0x400000, CRC(a25fc3d0) SHA1(83cb349e2f1032652060b233e741fb893be5af16) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "095-c5.bin", 0x1000000, 0x400000, CRC(8b9b65df) SHA1(e2a7e20855501f240bcd22f5cc92fcb4a9806abe) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "095-c6.bin", 0x1000001, 0x400000, CRC(3e164718) SHA1(53217f938c8964c1ca68a6fd5249c4169a5ac8e6) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "095-c7.bin", 0x1800000, 0x200000, CRC(ca605e12) SHA1(5150b835247fd705bc1dece97d423d9c20a51416) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "095-c8.bin", 0x1800001, 0x200000, CRC(4e6beb6c) SHA1(c0ac7cfc832ace6ad52c58f5da3a8101baead749) ) /* Plane 2,3 */
ROM_END

ROM_START( aof3 )
	ROM_REGION( 0x300000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "096-p1.bin", 0x000000, 0x100000, CRC(9edb420d) SHA1(150d80707325ece351c72c21c6186cfb5996adba) )
	ROM_LOAD16_WORD_SWAP( "096-p2.bin", 0x100000, 0x200000, CRC(4d5a2602) SHA1(4c26d6135d2877d9c38169662033e9d0cc24d943) )

	NEO_SFIX_128K( "096-s1.bin", CRC(cc7fd344) SHA1(2c6846cf8ea61fb192ba181dbccb63594d572c0e) )

	NEO_BIOS_SOUND_128K( "096-m1.bin", CRC(cb07b659) SHA1(940b379957c2987d7ab0443cb80c3ff58f6ba559) )

	ROM_REGION( 0x600000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "096-v1.bin", 0x000000, 0x200000, CRC(e2c32074) SHA1(69426e7e63fc31a73d1cd056cc9ae6a2c4499407) )
	ROM_LOAD( "096-v2.bin", 0x200000, 0x200000, CRC(a290eee7) SHA1(e66a98cd9740188bf999992b417f8feef941cede) )
	ROM_LOAD( "096-v3.bin", 0x400000, 0x200000, CRC(199d12ea) SHA1(a883bf34e685487705a8dafdd0b8db15eb360e80) )

	NO_DELTAT_REGION

	ROM_REGION( 0x1c00000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "096-c1.bin", 0x0400000, 0x200000, CRC(f6c74731) SHA1(7cd585d40b0993e361a24f917aa220504f12c543) ) /* Plane 0,1 */
	ROM_CONTINUE(      			  0x0000000, 0x200000 )
	ROM_LOAD16_BYTE( "096-c2.bin", 0x0400001, 0x200000, CRC(f587f149) SHA1(496446ecc7b39a034b4e28218afb147577a04ab5) ) /* Plane 2,3 */
	ROM_CONTINUE(      			  0x0000001, 0x200000 )
	ROM_LOAD16_BYTE( "096-c3.bin", 0x0c00000, 0x200000, CRC(7749f5e6) SHA1(f777d55d10684e78b05c1301529b67ed6b6b4445) ) /* Plane 0,1 */
	ROM_CONTINUE(      			  0x0800000, 0x200000 )
	ROM_LOAD16_BYTE( "096-c4.bin", 0x0c00001, 0x200000, CRC(cbd58369) SHA1(ff457eecbeef245db4e67ce4e8eddaf368bba93d) ) /* Plane 2,3 */
	ROM_CONTINUE(      			  0x0800001, 0x200000 )
	ROM_LOAD16_BYTE( "096-c5.bin", 0x1400000, 0x200000, CRC(1718bdcd) SHA1(a34e4b3aa41cc39415db7dacabe99ca6f8893545) ) /* Plane 0,1 */
	ROM_CONTINUE(      			  0x1000000, 0x200000 )
	ROM_LOAD16_BYTE( "096-c6.bin", 0x1400001, 0x200000, CRC(4fca967f) SHA1(83eea4b7d166feb4274bd2d658f4fdcb20629a40) ) /* Plane 2,3 */
	ROM_CONTINUE(      			  0x1000001, 0x200000 )
	ROM_LOAD16_BYTE( "096-c7.bin", 0x1800000, 0x200000, CRC(51bd8ab2) SHA1(c8def9c64de64571492b5b7e14b794e3c18f1393) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "096-c8.bin", 0x1800001, 0x200000, CRC(9a34f99c) SHA1(fca72d95ec42790a7f1e771a1e25dbc5bec5fc19) ) /* Plane 2,3 */
ROM_END

ROM_START( sonicwi3 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "097-p1.bin", 0x100000, 0x100000, CRC(0547121d) SHA1(e0bb6c614f572b74ba9a9f0d3d5b69fbc91ebc52) )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "097-s1.bin", CRC(8dd66743) SHA1(39214bb25a1d5b44a8524010be05bf5a0211981f) )

	NEO_BIOS_SOUND_128K( "097-m1.bin", CRC(b20e4291) SHA1(0e891ab53f9fded510295dfc7818bc59b4a9dd97) )

	ROM_REGION( 0x500000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "097-v1.bin", 0x000000, 0x400000, CRC(6f885152) SHA1(8175804d5c1420c5d37b733d4a8fa2aa81e59f1b) )
	ROM_LOAD( "097-v2.bin", 0x400000, 0x100000, CRC(32187ccd) SHA1(35a93de2a23bdec181c504d9c21a871bf86edee1) )

	NO_DELTAT_REGION

	ROM_REGION( 0xc00000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "097-c1.bin", 0x400000, 0x200000, CRC(3ca97864) SHA1(5fa9dbc698a239cbd8ea1d54e6a301a65406c8d7) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x000000, 0x200000 )
	ROM_LOAD16_BYTE( "097-c2.bin", 0x400001, 0x200000, CRC(1da4b3a9) SHA1(7de21780a9d606e22be3ad597d6e0f1480089b31) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x000001, 0x200000 )
	ROM_LOAD16_BYTE( "097-c3.bin", 0x800000, 0x200000, CRC(c339fff5) SHA1(58dfd1e30dc0ad3f816a5dbd1cc7e7ccbb792c53) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "097-c4.bin", 0x800001, 0x200000, CRC(84a40c6e) SHA1(061a13fba5fed883e5ee9566cedc208df2511bcf) ) /* Plane 2,3 */
ROM_END

ROM_START( turfmast )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "200-p1.bin",  0x100000, 0x100000, CRC(28c83048) SHA1(e7ef87e1de21d2bb17ef17bb08657e92363f0e9a) )
	ROM_CONTINUE(						 0x000000, 0x100000)

	NEO_SFIX_128K( "200-s1.bin", CRC(9a5402b2) SHA1(ae1a0b5450869d61b2bb23671c744d3dda8769c4) )

	NEO_BIOS_SOUND_128K( "200-m1.bin", CRC(9994ac00) SHA1(7bded797f3b80fd00bcbe451ac0abe6646b19a14) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "200-v1.bin", 0x000000, 0x200000, CRC(00fd48d2) SHA1(ddfee09328632e598fd51537b3ae8593219b2111) )
	ROM_LOAD( "200-v2.bin", 0x200000, 0x200000, CRC(082acb31) SHA1(2f1c053040e9d50a6d45fd7bea1b96742bae694f) )
	ROM_LOAD( "200-v3.bin", 0x400000, 0x200000, CRC(7abca053) SHA1(e229bc0ea82a371d6ee8fd9fe442b0fd141d0a71) )
	ROM_LOAD( "200-v4.bin", 0x600000, 0x200000, CRC(6c7b4902) SHA1(d55e0f542d928a9a851133ff26763c8236cbbd4d) )

	NO_DELTAT_REGION

	ROM_REGION( 0x800000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "200-c1.bin", 0x400000, 0x200000, CRC(8c6733f2) SHA1(c4d20a8df03bbb6bb72b8fc089d4833b79c75cee) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x000000, 0x200000 )
	ROM_LOAD16_BYTE( "200-c2.bin", 0x400001, 0x200000, CRC(596cc256) SHA1(705f949b3ba721b2e7973eaf2b2f9283dfef778c) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x000001, 0x200000 )
ROM_END

ROM_START( mslug )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "201-p1.bin", 0x100000, 0x100000, CRC(08d8daa5) SHA1(b888993dbb7e9f0a28a01d7d2e1da00ef9cf6f38) )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "201-s1.bin", CRC(2f55958d) SHA1(550b53628daec9f1e1e11a398854092d90f9505a) )

	NEO_BIOS_SOUND_128K( "201-m1.bin", CRC(c28b3253) SHA1(fd75bd15aed30266a8b3775f276f997af57d1c06) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "201-v1.bin", 0x000000, 0x400000, CRC(23d22ed1) SHA1(cd076928468ad6bcc5f19f88cb843ecb5e660681) )
	ROM_LOAD( "201-v2.bin", 0x400000, 0x400000, CRC(472cf9db) SHA1(5f79ea9286d22ed208128f9c31ca75552ce08b57) )

	NO_DELTAT_REGION

	ROM_REGION( 0x1000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "201-c1.bin", 0x400000, 0x200000, CRC(d00bd152) SHA1(eb688dba2233bece1c3ba120ac8eb342f37fba37) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x000000, 0x200000 )
	ROM_LOAD16_BYTE( "201-c2.bin", 0x400001, 0x200000, CRC(ddff1dea) SHA1(e6ac8950d8ad8498270097a248c4b49876804197) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x000001, 0x200000 )
	ROM_LOAD16_BYTE( "201-c3.bin", 0xc00000, 0x200000, CRC(d3d5f9e5) SHA1(7d259314c2198ee81a380d76728c3c1ac2c8b528) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x800000, 0x200000 )
	ROM_LOAD16_BYTE( "201-c4.bin", 0xc00001, 0x200000, CRC(5ac1d497) SHA1(313249ea47b3553974cde1c4c36f1ff3adeb07d1) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x800001, 0x200000 )
ROM_END

ROM_START( puzzledp )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "202-p1.bin", 0x000000, 0x080000, CRC(2b61415b) SHA1(0e3e4faf2fd6e63407425e1ac788003e75aeeb4f) )

	NEO_SFIX_64K( "202-s1.bin", CRC(4a421612) SHA1(8a79055be7bf6e2c344a72d6afe6a33be3d4a6c3) )

	NEO_BIOS_SOUND_128K( "202-m1.bin", CRC(9c0291ea) SHA1(3fa67c62acba79be6b3a98cc1601e45569fa11ae) )

	ROM_REGION( 0x080000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "202-v1.bin", 0x000000, 0x080000, CRC(debeb8fb) SHA1(49a3d3578c087f1a0050168571ef8d1b08c5dc05) )

	NO_DELTAT_REGION

	ROM_REGION( 0x200000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "202-c1.bin", 0x000000, 0x100000, CRC(cc0095ef) SHA1(3d86f455e6db10a2449b775dc386f1826ba3b62e) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "202-c2.bin", 0x000001, 0x100000, CRC(42371307) SHA1(df794f989e2883634bf7ffeea48d6bc3854529af) ) /* Plane 2,3 */
ROM_END

ROM_START( mosyougi )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "203-p1.bin", 0x000000, 0x100000, CRC(7ba70e2d) SHA1(945f472cc3e7706f613c52df18de35c986d166e7) )

	NEO_SFIX_128K( "203-s1.bin", CRC(4e132fac) SHA1(ecc5552880cc0a771121efe2a60810b70f6121ff) )

	NEO_BIOS_SOUND_128K( "203-m1.bin", CRC(a602c2c2) SHA1(19fd5d0379244c528b58343f6cbf78b4766fb23d) )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "203-v1.bin", 0x000000, 0x200000, CRC(baa2b9a5) SHA1(914782b6c81d9a76ce02251575592b0648434ba3) )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "203-c1.bin",  0x000000, 0x200000, CRC(bba9e8c0) SHA1(db89b7275a59ae6104a8308025c7e142a67b947b) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "203-c2.bin",  0x000001, 0x200000, CRC(2574be03) SHA1(198cfd697c623022919ae4118928a7fe30cd6c46) ) /* Plane 2,3 */
ROM_END

ROM_START( marukodq )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "206-p1.bin", 0x000000, 0x100000, CRC(c33ed21e) SHA1(bffff0d17e587e67672227e60c0ebd3f3a7193e6) )

	NEO_SFIX_32K( "206-s1.bin", CRC(3b52a219) SHA1(3587ab9dd1b3026c28a03349f3a13e6dfa7ff490) )

	NEO_BIOS_SOUND_128K( "206-m1.bin", CRC(0e22902e) SHA1(fb8466c342d4abd8bb4cad01c6ceab03f96cdad8) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "206-v1.bin", 0x000000, 0x200000, CRC(5385eca8) SHA1(1ca171ce74a5885ae8841d0924de21dc0af2214e) )
	ROM_LOAD( "206-v2.bin", 0x200000, 0x200000, CRC(f8c55404) SHA1(cecc41e9e08a7ff05b6f62e713fc86a816bf55a2) )

	NO_DELTAT_REGION

	ROM_REGION( 0xa00000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "206-c1.bin", 0x000000, 0x400000, CRC(4bd5e70f) SHA1(2cd546aafb73e0523655f53b134955ebc273aacd) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "206-c2.bin", 0x000001, 0x400000, CRC(67dbe24d) SHA1(37047c4e52525ff6d39a462222ec3e4cfc63e31c) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "206-c3.bin", 0x800000, 0x100000, CRC(79aa2b48) SHA1(31f94217cd35f48845c74a55256314c16fd26ed7) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "206-c4.bin", 0x800001, 0x100000, CRC(55e1314d) SHA1(fffbc9eb9000ff5b1063af1817de7ea4a267fedd) ) /* Plane 2,3 */
ROM_END

ROM_START( neomrdo )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "207-p1.bin", 0x000000, 0x80000, CRC(39efdb82) SHA1(75fe68921f871872e5fc92594e43b4cd712e819b) )

	NEO_SFIX_64K( "207-s1.bin", CRC(6c4b09c4) SHA1(2e9eac88afab606fad6c439efba0f6490389dd5f) )

	NEO_BIOS_SOUND_128K( "207-m1.bin", CRC(81eade02) SHA1(d6d135bc525f3ed14a8c255f0b83d83a52b0659c) )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "207-v1.bin", 0x000000, 0x200000, CRC(4143c052) SHA1(561b19bc8811b80f2f42ffc0b5df27132696470a) )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "207-c1.bin", 0x000000, 0x200000, CRC(c7541b9d) SHA1(25ca1a2b14cc2648d8dbe432cbd1396017af822c) )
	ROM_LOAD16_BYTE( "207-c2.bin", 0x000001, 0x200000, CRC(f57166d2) SHA1(bf3aa47d17156485c2177fb63cba093f050abb98) )
ROM_END

ROM_START( sdodgeb )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "208-p1.bin", 0x100000, 0x100000, CRC(127f3d32) SHA1(18e77b79b1197a89371533ef9b1e4d682c44d875) )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "208-s1.bin", CRC(64abd6b3) SHA1(0315d724e4d83a44ce84c531ff9b8c398363c039) )

	NEO_BIOS_SOUND_128K( "208-m1.bin", CRC(0a5f3325) SHA1(04e0236df478a5452654c823dcb42fea65b6a718) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "208-v1.bin", 0x000000, 0x200000, CRC(8b53e945) SHA1(beb7d63f6101f8435f35321fddb8479d312505c4) )
	ROM_LOAD( "208-v2.bin", 0x200000, 0x200000, CRC(af37ebf8) SHA1(f5c511479483533480f2b9ecf8edd4b7ae64c2d9) )

	NO_DELTAT_REGION

	ROM_REGION( 0x0c00000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "208-c1.bin", 0x0000000, 0x400000, CRC(93d8619b) SHA1(6588cb67e38722d5843fb29943d92e3905101aff) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "208-c2.bin", 0x0000001, 0x400000, CRC(1c737bb6) SHA1(8e341989981a713e61dfed8bde9a6459583ef46d) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "208-c3.bin", 0x0800000, 0x200000, CRC(14cb1703) SHA1(a46acec03c1b2351fe36810628f02b7c848d13db) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "208-c4.bin", 0x0800001, 0x200000, CRC(c7165f19) SHA1(221f03de893dca0e5305fa17aa94f96c67713818) ) /* Plane 2,3 */
ROM_END

ROM_START( goalx3 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "209-p1.bin", 0x100000, 0x100000, CRC(2a019a79) SHA1(422a639e74284fef2e53e1b49cf8803b0a7e80c6) )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "209-s1.bin", CRC(c0eaad86) SHA1(99412093c9707d51817893971e73fb8469cdc9d0) )

	NEO_BIOS_SOUND_64K( "209-m1.bin", CRC(dd945773) SHA1(2304d070864da79dceb29c64e8d71b7db0992d1e) )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "209-v1.bin", 0x000000, 0x200000, CRC(ef214212) SHA1(3e05ccaa2d06decb18b379b96f900c0e6b39ce70) )

	NO_DELTAT_REGION

	ROM_REGION( 0xa00000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "209-c1.bin", 0x400000, 0x200000, CRC(d061f1f5) SHA1(7cde2f4c6cc54fb0ffbe1d407831d652610a3aaf) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x000000, 0x200000 )
	ROM_LOAD16_BYTE( "209-c2.bin", 0x400001, 0x200000, CRC(3f63c1a2) SHA1(3ce2c66fb2bee3face976e9f8973ccc483101ae5) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x000001, 0x200000 )
	ROM_LOAD16_BYTE( "209-c3.bin", 0x800000, 0x100000, CRC(5f91bace) SHA1(3864be27dce6d8f8828d3bf09bfc8116116a2b56) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "209-c4.bin", 0x800001, 0x100000, CRC(1e9f76f2) SHA1(b57fdc226bfe328b8848127fb4292295f1287bf6) ) /* Plane 2,3 */
ROM_END

ROM_START( overtop )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "212-p1.bin", 0x100000, 0x100000, CRC(16c063a9) SHA1(5432869f830eed816ee5ed71c7fd39f749d15619) )
	ROM_CONTINUE(					  0x000000, 0x100000 )

	NEO_SFIX_128K( "212-s1.bin",  CRC(481d3ddc) SHA1(7b0df3fc5b19f282abfd0eb5a4c6ed836a536ece) )

	NEO_BIOS_SOUND_128K( "212-m1.bin", CRC(fcab6191) SHA1(488b8310b0957f0012fe50f73641b606f6ac4a57) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "212-v1.bin", 0x000000, 0x400000, CRC(013d4ef9) SHA1(438a697c44525bdf78b54432c4f7217ab5667047) )

	NO_DELTAT_REGION

	ROM_REGION( 0x1400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "212-c1.bin", 0x0000000, 0x400000, CRC(50f43087) SHA1(e5a8c914ef8e77c7a29bffdeb18f1877b5c2fc7d) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "212-c2.bin", 0x0000001, 0x400000, CRC(a5b39807) SHA1(e98e82cf99576cb48cc5e8dc655b7e9a428c2843) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "212-c3.bin", 0x0800000, 0x400000, CRC(9252ea02) SHA1(269066e0f893d3e8e7c308528026a486c2b023a2) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "212-c4.bin", 0x0800001, 0x400000, CRC(5f41a699) SHA1(abbb162658e06a37db8475b659ece7e1270ebb49) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "212-c5.bin", 0x1000000, 0x200000, CRC(fc858bef) SHA1(0031def13e7cf4a465a1eca7aa0d13d1b21427e2) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "212-c6.bin", 0x1000001, 0x200000, CRC(0589c15e) SHA1(b1167caf7cb61f3e05a5d342290bfe00e02e9d38) ) /* Plane 2,3 */
ROM_END

ROM_START( neodrift )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "213-p1.bin",  0x100000, 0x100000, CRC(e397d798) SHA1(10f459111db4bab7aaa63ca47e83304a84300812) )
	ROM_CONTINUE(						 0x000000, 0x100000)

	NEO_SFIX_128K( "213-s1.bin", CRC(b76b61bc) SHA1(5fdb407d16ab9e33c4f26ee09ff70891ae1d2bd0) )

	NEO_BIOS_SOUND_128K( "213-m1.bin", CRC(200045f1) SHA1(7a6cd1c8d4447ea260d7ff4520c676b8d685f2e4) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "213-v1.bin", 0x000000, 0x200000, CRC(a421c076) SHA1(129f05c1a28a6493442f47a79c2d3577a1a43ef5) )
	ROM_LOAD( "213-v2.bin", 0x200000, 0x200000, CRC(233c7dd9) SHA1(be7f980aa83831b6605aaaf4ec904180bb96c935) )

	NO_DELTAT_REGION

	ROM_REGION( 0x800000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "213-c1.bin", 0x400000, 0x200000, CRC(62c5edc9) SHA1(df3ed81b37fc03503c8702741d361f44adfcd481) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x000000, 0x200000 )
	ROM_LOAD16_BYTE( "213-c2.bin", 0x400001, 0x200000, CRC(9dc9c72a) SHA1(c3960b18d940233332c2b1ee2b2b94685c724d1e) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x000001, 0x200000 )
ROM_END

ROM_START( kof96 )
	ROM_REGION( 0x300000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "214-p1.bin", 0x000000, 0x100000, CRC(52755d74) SHA1(4232d627f1d2e6ea9fc8cf01571d77d4d5b8a1bb) )
	ROM_LOAD16_WORD_SWAP( "214-p2.bin", 0x100000, 0x200000, CRC(002ccb73) SHA1(3ae8df682c75027ca82db25491021eeba00a267e) )

	NEO_SFIX_128K( "214-s1.bin", CRC(1254cbdb) SHA1(fce5cf42588298711a3633e9c9c1d4dcb723ac76) )

	NEO_BIOS_SOUND_128K( "214-m1.bin", CRC(dabc427c) SHA1(b76722ed142ee7addceb4757424870dbd003e8b3) )

	ROM_REGION( 0xa00000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "214-v1.bin", 0x000000, 0x400000, CRC(63f7b045) SHA1(1353715f1a8476dca6f8031d9e7a401eacab8159) )
	ROM_LOAD( "214-v2.bin", 0x400000, 0x400000, CRC(25929059) SHA1(6a721c4cb8f8dc772774023877d4a9f50d5a9e31) )
	ROM_LOAD( "214-v3.bin", 0x800000, 0x200000, CRC(92a2257d) SHA1(5064aec78fa0d104e5dd5869b95382aa170214ee) )

	NO_DELTAT_REGION

	ROM_REGION( 0x2000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "214-c1.bin", 0x0000000, 0x400000, CRC(7ecf4aa2) SHA1(f773c4c1f05d58dd37e7bb2ac1d1e0ec43998a71) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "214-c2.bin", 0x0000001, 0x400000, CRC(05b54f37) SHA1(cc31653fe4cb05201fba234e080cb9c7a7592b1b) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "214-c3.bin", 0x0800000, 0x400000, CRC(64989a65) SHA1(e6f3749d43be0afa9dad7b085cb782ba694252ca) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "214-c4.bin", 0x0800001, 0x400000, CRC(afbea515) SHA1(ae875052728de33174827705646bd14cf3937b5c) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "214-c5.bin", 0x1000000, 0x400000, CRC(2a3bbd26) SHA1(7c1a7e50a10a1b082e0d0d515c34135ee9f995ac) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "214-c6.bin", 0x1000001, 0x400000, CRC(44d30dc7) SHA1(c8ae001e37224b55d9e4a4d99f6578b4f6eb055f) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "214-c7.bin", 0x1800000, 0x400000, CRC(3687331b) SHA1(2be95caab76d7af51674f93884330ba73a6053e4) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "214-c8.bin", 0x1800001, 0x400000, CRC(fa1461ad) SHA1(6c71a7f08e4044214223a6bf80984582ab5e0328) ) /* Plane 2,3 */
ROM_END

ROM_START( kof96h )
	ROM_REGION( 0x300000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "214-pg1.bin",0x000000, 0x100000, CRC(bd3757c9) SHA1(35392a044117e46c088ff0fdd07d69a3faa4f96e) )
	ROM_LOAD16_WORD_SWAP( "214-p2.bin", 0x100000, 0x200000, CRC(002ccb73) SHA1(3ae8df682c75027ca82db25491021eeba00a267e) )

	NEO_SFIX_128K( "214-s1.bin", CRC(1254cbdb) SHA1(fce5cf42588298711a3633e9c9c1d4dcb723ac76) )

	NEO_BIOS_SOUND_128K( "214-m1.bin", CRC(dabc427c) SHA1(b76722ed142ee7addceb4757424870dbd003e8b3) )

	ROM_REGION( 0xa00000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "214-v1.bin", 0x000000, 0x400000, CRC(63f7b045) SHA1(1353715f1a8476dca6f8031d9e7a401eacab8159) )
	ROM_LOAD( "214-v2.bin", 0x400000, 0x400000, CRC(25929059) SHA1(6a721c4cb8f8dc772774023877d4a9f50d5a9e31) )
	ROM_LOAD( "214-v3.bin", 0x800000, 0x200000, CRC(92a2257d) SHA1(5064aec78fa0d104e5dd5869b95382aa170214ee) )

	NO_DELTAT_REGION

	ROM_REGION( 0x2000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "214-c1.bin", 0x0000000, 0x400000, CRC(7ecf4aa2) SHA1(f773c4c1f05d58dd37e7bb2ac1d1e0ec43998a71) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "214-c2.bin", 0x0000001, 0x400000, CRC(05b54f37) SHA1(cc31653fe4cb05201fba234e080cb9c7a7592b1b) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "214-c3.bin", 0x0800000, 0x400000, CRC(64989a65) SHA1(e6f3749d43be0afa9dad7b085cb782ba694252ca) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "214-c4.bin", 0x0800001, 0x400000, CRC(afbea515) SHA1(ae875052728de33174827705646bd14cf3937b5c) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "214-c5.bin", 0x1000000, 0x400000, CRC(2a3bbd26) SHA1(7c1a7e50a10a1b082e0d0d515c34135ee9f995ac) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "214-c6.bin", 0x1000001, 0x400000, CRC(44d30dc7) SHA1(c8ae001e37224b55d9e4a4d99f6578b4f6eb055f) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "214-c7.bin", 0x1800000, 0x400000, CRC(3687331b) SHA1(2be95caab76d7af51674f93884330ba73a6053e4) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "214-c8.bin", 0x1800001, 0x400000, CRC(fa1461ad) SHA1(6c71a7f08e4044214223a6bf80984582ab5e0328) ) /* Plane 2,3 */
ROM_END

ROM_START( ssideki4 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "215-p1.bin", 0x100000, 0x100000, CRC(519b4ba3) SHA1(5aa59514b23aa663f2c4014ee94a31e9f59151de) )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "215-s1.bin", CRC(f0fe5c36) SHA1(b7badd6d2ac3788ce5cace1fcf5cdad14734e4e6) )

	NEO_BIOS_SOUND_128K( "215-m1.bin", CRC(a932081d) SHA1(376a45e19edb780ac8798c41ae2260c8a8a4bba8) )

	ROM_REGION( 0x600000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "215-v1.bin", 0x200000, 0x200000, CRC(c4bfed62) SHA1(6fec1880eef16f2d1bce152f3c73fb3ea1a931a0) )
	ROM_CONTINUE(			  0x000000, 0x200000 )
	ROM_LOAD( "215-v2.bin", 0x400000, 0x200000, CRC(1bfa218b) SHA1(344836a578bde3c0ab59b58c8734f868e7403c26) )

	NO_DELTAT_REGION

	ROM_REGION( 0x1400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "215-c1.bin", 0x0400000, 0x200000, CRC(288a9225) SHA1(403acb892c0d0f2e5cdf4b0bb4b5843ab6e95874) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0000000, 0x200000 )
	ROM_LOAD16_BYTE( "215-c2.bin", 0x0400001, 0x200000, CRC(3fc9d1c4) SHA1(247dd81fe521876b3970c3ec8e260ada5618df8c) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0000001, 0x200000 )
	ROM_LOAD16_BYTE( "215-c3.bin", 0x0c00000, 0x200000, CRC(fedfaebe) SHA1(145007cf98b16c0d82385ce5df0b17806eb93ed8) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0800000, 0x200000 )
	ROM_LOAD16_BYTE( "215-c4.bin", 0x0c00001, 0x200000, CRC(877a5bb2) SHA1(7234b1cd5ae299a3596f05f7e9387b72dd425b36) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0800001, 0x200000 )
	ROM_LOAD16_BYTE( "215-c5.bin", 0x1000000, 0x200000, CRC(0c6f97ec) SHA1(b8d297f0ba2b04404eb0f7c6673ecc206fadae0c) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "215-c6.bin", 0x1000001, 0x200000, CRC(329c5e1b) SHA1(015c36b8d3efab9b4647f110ecb5c118a9c80f43) ) /* Plane 2,3 */
ROM_END

ROM_START( kizuna )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "216-p1.bin", 0x100000, 0x100000, CRC(75d2b3de) SHA1(ee778656c26828935ee2a2bfd0ce5a22aa681c10) )
	ROM_CONTINUE(					 0x000000, 0x100000 )

	NEO_SFIX_128K( "216-s1.bin",   CRC(efdc72d7) SHA1(be37cbf1852e2e4c907cc799b754b538544b6703) )

	NEO_BIOS_SOUND_128K( "216-m1.bin", CRC(1b096820) SHA1(72852e78c620038f8dafde5e54e02e418c31be9c) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "216-v1.bin", 0x000000, 0x200000, CRC(530c50fd) SHA1(29401cee7f7d2c199c7cb58092e86b28205e81ad) )
	ROM_LOAD( "216-v2.bin", 0x200000, 0x200000, CRC(03667a8d) SHA1(3b0475e553a49f8788f32b0c84f82645cc6b4273) )
	ROM_LOAD( "216-v3.bin", 0x400000, 0x200000, CRC(7038c2f9) SHA1(c1d6f86b24feba03fe009b58199d2eeabe572f4e) )
	ROM_LOAD( "216-v4.bin", 0x600000, 0x200000, CRC(31b99bd6) SHA1(5871751f8e9e6b98337472c22b5e1c7ede0a9311) )

	NO_DELTAT_REGION

	ROM_REGION( 0x1c00000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "216-c1.bin", 0x0000000, 0x200000, CRC(763ba611) SHA1(d3262e0332c894ee149c5963f882cc5e5562ee57) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "216-c2.bin", 0x0000001, 0x200000, CRC(e05e8ca6) SHA1(986a9b16ff92bc101ab567d2d01348e093abea9a) ) /* Plane 2,3 */
	/* 400000-7fffff empty */
	ROM_LOAD16_BYTE( "216-c3.bin", 0x0800000, 0x400000, CRC(665c9f16) SHA1(7ec781a49a462f395b450460b29493f55134eac2) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "216-c4.bin", 0x0800001, 0x400000, CRC(7f5d03db) SHA1(365ed266c121f4df0bb76898955a8ae0e668a216) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "216-c5.bin", 0x1000000, 0x200000, CRC(59013f9e) SHA1(5bf48fcc450da72a8c4685f6e3887e67eae49988) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "216-c6.bin", 0x1000001, 0x200000, CRC(1c8d5def) SHA1(475d89a5c4922a9f6bd756d23c2624d57b6e9d62) ) /* Plane 2,3 */
	/* 1400000-17fffff empty */
	ROM_LOAD16_BYTE( "216-c7.bin", 0x1800000, 0x200000, CRC(c88f7035) SHA1(c29a428b741f4fe7b71a3bc23c87925b6bc1ca8f) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "216-c8.bin", 0x1800001, 0x200000, CRC(484ce3ba) SHA1(4f21ed20ce6e2b67e2b079404599310c94f591ff) ) /* Plane 2,3 */
ROM_END

ROM_START( ninjamas )
	ROM_REGION( 0x300000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "217-p1.bin", 0x000000, 0x100000, CRC(3e97ed69) SHA1(336bcae375a5109945d11356503bf0d9f4a9a50a) )
	ROM_LOAD16_WORD_SWAP( "217-p2.bin", 0x100000, 0x200000, CRC(191fca88) SHA1(e318e5931704779bbe461719a5eeeba89bd83a5d) )

	NEO_SFIX_128K( "217-s1.bin", CRC(8ff782f0) SHA1(90099c154357042ba658d4ef6abe4d9335bb7172) )

	NEO_BIOS_SOUND_128K( "217-m1.bin", CRC(d00fb2af) SHA1(6bcaa52e1641cc24288e1f22f4dc98e8d8921b90) )

	ROM_REGION( 0x600000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "217-v1.bin", 0x000000, 0x400000, CRC(1c34e013) SHA1(5368e413d2188c4fd063b6bb7d5f498ff83ea812) )
	ROM_LOAD( "217-v2.bin", 0x400000, 0x200000, CRC(22f1c681) SHA1(09da03b2e63d180e55173ff25e8735c4162f027b) )

	NO_DELTAT_REGION

	ROM_REGION( 0x2000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "217-c1.bin", 0x0400000, 0x200000, CRC(58f91ae0) SHA1(365e8b865bbd3a8a5ffc7d8ffa0e7694a0bf3a2a) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0000000, 0x200000 )
	ROM_LOAD16_BYTE( "217-c2.bin", 0x0400001, 0x200000, CRC(4258147f) SHA1(c996477ba1da2ee38e46fb3dc0ecfd322ff3dd52) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0000001, 0x200000 )
	ROM_LOAD16_BYTE( "217-c3.bin", 0x0c00000, 0x200000, CRC(36c29ce3) SHA1(90bfcf4c4d72fdc3d87055afb8ec56ab6cee7003) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0800000, 0x200000 )
	ROM_LOAD16_BYTE( "217-c4.bin", 0x0c00001, 0x200000, CRC(17e97a6e) SHA1(ecf5905a8116f2ae1ae7a42f13e0ebe770fadb14) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0800001, 0x200000 )
	ROM_LOAD16_BYTE( "217-c5.bin", 0x1400000, 0x200000, CRC(4683ffc0) SHA1(b3667cb143e2eb7a199e8b55e1815dd35abcf1ff) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x1000000, 0x200000 )
	ROM_LOAD16_BYTE( "217-c6.bin", 0x1400001, 0x200000, CRC(de004f4a) SHA1(4b685e6efbfa74d7fcab2d1c8569df20662e2a6e) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x1000001, 0x200000 )
	ROM_LOAD16_BYTE( "217-c7.bin", 0x1c00000, 0x200000, CRC(3e1885c0) SHA1(0fc611cbaebce6db3ff2caf54819a335aaf8dcab) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x1800000, 0x200000 )
	ROM_LOAD16_BYTE( "217-c8.bin", 0x1c00001, 0x200000, CRC(5a5df034) SHA1(1863cdc8af52590b94c4a55f9333fc00d9b0b8a6) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x1800001, 0x200000 )
ROM_END

ROM_START( ragnagrd )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "218-p1.bin", 0x100000, 0x100000, CRC(ca372303) SHA1(67991e4fef9b36bc7d909810eebb857ac2f906f1) )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "218-s1.bin", CRC(7d402f9a) SHA1(59ec29d03e62e7a8bef689a124a9164f43b2ace1) )

	NEO_BIOS_SOUND_128K( "218-m1.bin", CRC(17028bcf) SHA1(7a4e8f33ce9b41beac2152b8f6003f247699e2e1) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "218-v1.bin", 0x000000, 0x400000, CRC(61eee7f4) SHA1(5b11b1a0b1b74dfbc2998cbda9f8f7a5e9059957) )
	ROM_LOAD( "218-v2.bin", 0x400000, 0x400000, CRC(6104e20b) SHA1(18e8aae3e51e141977d523a10e737ff68fe81910) )

	NO_DELTAT_REGION

	ROM_REGION( 0x2000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "218-c1.bin", 0x0400000, 0x200000, CRC(18f61d79) SHA1(d815f57710403c76dc7b119b13dea629feb3c341) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0000000, 0x200000 )
	ROM_LOAD16_BYTE( "218-c2.bin", 0x0400001, 0x200000, CRC(dbf4ff4b) SHA1(8fa0fb1df9e771089d72077642a847ea2066e401) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0000001, 0x200000 )
	ROM_LOAD16_BYTE( "218-c3.bin", 0x0c00000, 0x200000, CRC(108d5589) SHA1(04e25114cbf283f2a824f9e2127dc5ed96dc6d50) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0800000, 0x200000 )
	ROM_LOAD16_BYTE( "218-c4.bin", 0x0c00001, 0x200000, CRC(7962d5ac) SHA1(fd126e19fcff517ade00bd2394d675949d7cce1c) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0800001, 0x200000 )
	ROM_LOAD16_BYTE( "218-c5.bin", 0x1400000, 0x200000, CRC(4b74021a) SHA1(021dfa2dbfb06933362c7e34350e24ab23d34ebc) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x1000000, 0x200000 )
	ROM_LOAD16_BYTE( "218-c6.bin", 0x1400001, 0x200000, CRC(f5cf90bc) SHA1(6a1ea01b1610bb20f8dc68943ff622e7e600257b) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x1000001, 0x200000 )
	ROM_LOAD16_BYTE( "218-c7.bin", 0x1c00000, 0x200000, CRC(32189762) SHA1(355c9780ed73e48316bb9402b972a47810fa8807) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x1800000, 0x200000 )
	ROM_LOAD16_BYTE( "218-c8.bin", 0x1c00001, 0x200000, CRC(d5915828) SHA1(996930728f5f6aea180aba110e5214dc37f5b84a) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x1800001, 0x200000 )
ROM_END

ROM_START( pgoal )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "219-p1.bin", 0x100000, 0x100000, CRC(6af0e574) SHA1(c3f0fed0d942e48c99c80b1713f271c033ce0f4f) )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "219-s1.bin", CRC(002f3c88) SHA1(a8a5bbc5397c8ae9858e38997ebdc713b7b4f50a) )

	NEO_BIOS_SOUND_128K( "219-m1.bin", CRC(958efdc8) SHA1(aacc6056b1ff48cde8f241a11a27473cfb4b4aa3) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "219-v1.bin", 0x000000, 0x200000, CRC(2cc1bd05) SHA1(e204b6d359e5b37661dddc2c1f14d64fd9089f75) )
	ROM_LOAD( "219-v2.bin", 0x200000, 0x200000, CRC(06ac1d3f) SHA1(c7f2287e4e8366b78bc26ede7802e2e92d44d7c9) )

	NO_DELTAT_REGION

	ROM_REGION( 0xc00000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "219-c1.bin", 0x0000000, 0x200000, CRC(2dc69faf) SHA1(658327fdea658ed40f0074fe8800423cc9d553d5) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "219-c2.bin", 0x0000001, 0x200000, CRC(5db81811) SHA1(362c4df6ae62e125c0942ff8661a771ff32afbd7) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "219-c3.bin", 0x0400000, 0x200000, CRC(9dbfece5) SHA1(f52e2a34faab0416ea152559ff9089fb988b1f0b) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "219-c4.bin", 0x0400001, 0x200000, CRC(c9f4324c) SHA1(9d0dfcd39e72cbd63252b71b3ed879971b6f3443) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "219-c5.bin", 0x0800000, 0x200000, CRC(5fdad0a5) SHA1(56f6d2a7224aa4e82a1858079f918e85cadbd6c2) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "219-c6.bin", 0x0800001, 0x200000, CRC(f57b4a1c) SHA1(875ca69afbc5304ec23f4bc9186abe92f477f6c8) ) /* Plane 2,3 */
ROM_END

ROM_START( magdrop2 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "221-p1.bin", 0x000000, 0x80000, CRC(7be82353) SHA1(08ab39f52b893591c13a7d7aa26b20ce86e9ddf5) )

	NEO_SFIX_128K( "221-s1.bin", CRC(2a4063a3) SHA1(0e09a7d88d85b1a2100888f4211960ea56ef978b) )

	NEO_BIOS_SOUND_128K( "221-m1.bin", CRC(bddae628) SHA1(02c77e6aaaed43e39778bf83a3184e7c21db63d4) )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "221-v1.bin", 0x000000, 0x200000, CRC(7e5e53e4) SHA1(72b063b2d4acaaf72a20d14ad5bfc90cb64d3fed) )

	NO_DELTAT_REGION

	ROM_REGION( 0x800000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "221-c1.bin", 0x000000, 0x400000, CRC(1f862a14) SHA1(1253e8b65d863d552d00dbdbfc5c168f5fc7edd1) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "221-c2.bin", 0x000001, 0x400000, CRC(14b90536) SHA1(e0d41f6b84d8261729f154b44ddd95c9b9c0714a) ) /* Plane 2,3 */
ROM_END

ROM_START( samsho4 )
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "222-p1.bin", 0x000000, 0x100000, CRC(1a5cb56d) SHA1(9a0a5a1c7c5d428829f22d3d17f7033d43a51b5b) )
	ROM_LOAD16_WORD_SWAP( "222-p2.bin", 0x300000, 0x200000, CRC(7587f09b) SHA1(4cbd14b1e5158ab2c96b0860afc550b5dc746ca7) )
	ROM_CONTINUE(						0x100000, 0x200000 )

	NEO_SFIX_128K( "222-s1.bin", CRC(8d3d3bf9) SHA1(9975ed9b458bdd14e23451d2534153f68a5e4e6c) )

	NEO_BIOS_SOUND_128K( "222-m1.bin", CRC(7615bc1b) SHA1(b936f7b341f6fe0921b4c41049734684583e3596) )

	ROM_REGION( 0xa00000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "222-v1.bin", 0x000000, 0x400000, CRC(7d6ba95f) SHA1(03cb4e0d770e0b332b07b64cacef624460b84c78) )
	ROM_LOAD( "222-v2.bin", 0x400000, 0x400000, CRC(6c33bb5d) SHA1(fd5d4e08a962dd0d22c52c91bad5ec7f23cfb901) )
	ROM_LOAD( "222-v3.bin", 0x800000, 0x200000, CRC(831ea8c0) SHA1(f2987b7d09bdc4311e972ce8a9ab7ca9802db4db) )

	NO_DELTAT_REGION

	ROM_REGION( 0x2000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "222-c1.bin", 0x0400000, 0x200000, CRC(289100fa) SHA1(ead308ba395e4ddaa3e4e096cc6d264529132373) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0000000, 0x200000 )
	ROM_LOAD16_BYTE( "222-c2.bin", 0x0400001, 0x200000, CRC(c2716ea0) SHA1(06f2d218c0c90d978b03e8312fc1e79157a479f8) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0000001, 0x200000 )
	ROM_LOAD16_BYTE( "222-c3.bin", 0x0c00000, 0x200000, CRC(6659734f) SHA1(200c97e449057298b38cf67b053c241f2edfa740) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0800000, 0x200000 )
	ROM_LOAD16_BYTE( "222-c4.bin", 0x0c00001, 0x200000, CRC(91ebea00) SHA1(6574795885dd1e29f02e4973a391ff1964675896) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0800001, 0x200000 )
	ROM_LOAD16_BYTE( "222-c5.bin", 0x1400000, 0x200000, CRC(e22254ed) SHA1(56212ab8e24a78a990fa92fbb911f85e72783883) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x1000000, 0x200000 )
	ROM_LOAD16_BYTE( "222-c6.bin", 0x1400001, 0x200000, CRC(00947b2e) SHA1(8e986b2f348d2977e9c0272e4d0879def07bcf1e) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x1000001, 0x200000 )
	ROM_LOAD16_BYTE( "222-c7.bin", 0x1c00000, 0x200000, CRC(e3e3b0cd) SHA1(a722d14ea2c0a6882534568b5fc6e068605c13d8) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x1800000, 0x200000 )
	ROM_LOAD16_BYTE( "222-c8.bin", 0x1c00001, 0x200000, CRC(f33967f1) SHA1(780a05ba8c1873748842599a71672140d1ffd2e8) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x1800001, 0x200000 )
ROM_END

ROM_START( rbffspec )
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "223-p1.bin", 0x000000, 0x100000, CRC(f84a2d1d) SHA1(fc19225d9dbdb6bd0808023ee32c7829f6ffdef6) )
	ROM_LOAD16_WORD_SWAP( "223-p2.bin", 0x300000, 0x200000, CRC(27e3e54b) SHA1(09f8912c9f105e54bac9781680859988f43917e8) )
	ROM_CONTINUE(						0x100000, 0x200000 )

	NEO_SFIX_128K( "223-s1.bin", CRC(7ecd6e8c) SHA1(465455afc4d83cbb118142be4671b2539ffafd79) )

	NEO_BIOS_SOUND_128K( "223-m1.bin", CRC(3fee46bf) SHA1(e750f85233953853618fcdff980a4721af1710a3) )

	ROM_REGION( 0xc00000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "223-v1.bin", 0x000000, 0x400000, CRC(76673869) SHA1(78a26afa29f73de552ffabdbf3fc4bf26be8ae9e) )
	ROM_LOAD( "223-v2.bin", 0x400000, 0x400000, CRC(7a275acd) SHA1(8afe87ce822614262b72a90b371fc79155ac0d0c) )
	ROM_LOAD( "223-v3.bin", 0x800000, 0x400000, CRC(5a797fd2) SHA1(94958e334f86d4d71059af8138f255b8d97a3b01) )

	NO_DELTAT_REGION

	ROM_REGION( 0x2000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "223-c1.bin", 0x0400000, 0x200000, CRC(436edad4) SHA1(1ee871a7b720b46a84845dbfd6c0dcb1ffc95502) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0000000, 0x200000 )
	ROM_LOAD16_BYTE( "223-c2.bin", 0x0400001, 0x200000, CRC(cc7dc384) SHA1(31c6c872d0a3a0a7a55195f703eba36f1a631b9f) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0000001, 0x200000 )
	ROM_LOAD16_BYTE( "223-c3.bin", 0x0c00000, 0x200000, CRC(375954ea) SHA1(6e5e54f614f7985b309cbcc8ca9e441860074d8f) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0800000, 0x200000 )
	ROM_LOAD16_BYTE( "223-c4.bin", 0x0c00001, 0x200000, CRC(c1a98dd7) SHA1(e9094ee40374bd5134c9060ba6526fd00e9eb8b0) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0800001, 0x200000 )
	ROM_LOAD16_BYTE( "223-c5.bin", 0x1400000, 0x200000, CRC(12c5418e) SHA1(133e15a6667e17041782ffa594845b121548e63c) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x1000000, 0x200000 )
	ROM_LOAD16_BYTE( "223-c6.bin", 0x1400001, 0x200000, CRC(c8ad71d5) SHA1(d55bd1395a48509fb091fa2321606c88a988583e) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x1000001, 0x200000 )
	ROM_LOAD16_BYTE( "223-c7.bin", 0x1c00000, 0x200000, CRC(5c33d1d8) SHA1(924121e9e29e46b5158f8caa47799230065d1b00) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x1800000, 0x200000 )
	ROM_LOAD16_BYTE( "223-c8.bin", 0x1c00001, 0x200000, CRC(efdeb140) SHA1(faf7e7a38891b08d2dd17f427fda05586cceab7f) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x1800001, 0x200000 )
ROM_END

ROM_START( twinspri )
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "224-p1.bin", 0x100000, 0x100000, CRC(7697e445) SHA1(5b55ca120f77a931d40719b14e0bfc8cac1d628c) )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "224-s1.bin", CRC(eeed5758) SHA1(24e48f396716e145b692468762cf595fb7267873) )

	NEO_BIOS_SOUND_128K( "224-m1.bin", CRC(364d6f96) SHA1(779b95a6476089b71f48c8368d9043ee1dba9032) )

	ROM_REGION( 0x600000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "224-v1.bin", 0x000000, 0x400000, CRC(ff57f088) SHA1(1641989b8aac899dbd68aa2332bcdf9b90b33564) )
	ROM_LOAD( "224-v2.bin", 0x400000, 0x200000, CRC(7ad26599) SHA1(822030037b7664795bf3d64e1452d0aecc22497e) )

	NO_DELTAT_REGION

	ROM_REGION( 0xa00000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "224-c1.bin", 0x400000, 0x200000, CRC(73b2a70b) SHA1(7444cbfc6d29810fcd6eae93508609e919e3c153) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x000000, 0x200000 )
	ROM_LOAD16_BYTE( "224-c2.bin", 0x400001, 0x200000, CRC(3a3e506c) SHA1(076e78a68b26822c3c69fe8fbc0104dcec8dd880) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x000001, 0x200000 )
	ROM_LOAD16_BYTE( "224-c3.bin", 0x800000, 0x100000, CRC(c59e4129) SHA1(93f02d1b4fbb152a9d336494fbff0d7642921de5) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "224-c4.bin", 0x800001, 0x100000, CRC(b5532e53) SHA1(7d896c25ba97f6e5d43c13d4df4ba72964a976ed) ) /* Plane 2,3 */
ROM_END

ROM_START( wakuwak7 )
	ROM_REGION( 0x300000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "225-p1.bin", 0x000000, 0x100000, CRC(b14da766) SHA1(bdffd72ff705fc6b085a4026217bac1c4bc93163) )
	ROM_LOAD16_WORD_SWAP( "225-p2.bin", 0x100000, 0x200000, CRC(fe190665) SHA1(739d9a8fc2da34381654d9e291141eacc210ae5c) )

	NEO_SFIX_128K( "225-s1.bin", CRC(71c4b4b5) SHA1(9410f13807f01082dc86f2d84051be4bed8e9f7c) )

	NEO_BIOS_SOUND_128K( "225-m1.bin", CRC(0634bba6) SHA1(153aaf016440500df7a4454f3f2f2911219cb7d8) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "225-v1.bin", 0x000000, 0x400000, CRC(6195c6b4) SHA1(66c06b5904aedb256e3997bbec60f8ab50c6ff0c) )
	ROM_LOAD( "225-v2.bin", 0x400000, 0x400000, CRC(6159c5fe) SHA1(9015e93416497f1ef877c717afed40f7ecfa42e4) )

	NO_DELTAT_REGION

	ROM_REGION( 0x1800000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "225-c1.bin", 0x0400000, 0x200000, CRC(d91d386f) SHA1(35cd98aa5153f5da2d106ea2e138f419ea8eeccd) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0000000, 0x200000 )
	ROM_LOAD16_BYTE( "225-c2.bin", 0x0400001, 0x200000, CRC(36b5cf41) SHA1(6a135cf0b950a7ea5a5084d8affbe7b318566f13) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0000001, 0x200000 )
	ROM_LOAD16_BYTE( "225-c3.bin", 0x0c00000, 0x200000, CRC(02fcff2f) SHA1(e160d6410185a9bf7dd7dd81cdbecf3d0c524ede) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0800000, 0x200000 )
	ROM_LOAD16_BYTE( "225-c4.bin", 0x0c00001, 0x200000, CRC(cd7f1241) SHA1(51b3d47025e705eee6d95da52ecd74de3be52029) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0800001, 0x200000 )
	ROM_LOAD16_BYTE( "225-c5.bin", 0x1400000, 0x200000, CRC(03d32f25) SHA1(19ca7a77dfd645170ec2e77a8836ce4ba5b4da3a) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x1000000, 0x200000 )
	ROM_LOAD16_BYTE( "225-c6.bin", 0x1400001, 0x200000, CRC(d996a90a) SHA1(f0365056a7b4f660b00c670a17efbb9f70e8db5d) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x1000001, 0x200000 )
ROM_END

ROM_START( stakwin2 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "227-p1.bin", 0x100000, 0x100000, CRC(daf101d2) SHA1(96b90f884bae2969ebd8c04aba509928464e2433) )
	ROM_CONTINUE(					  0x000000, 0x100000 )

	NEO_SFIX_128K( "227-s1.bin", CRC(2a8c4462) SHA1(9155fbb5fee6d46a68d17ea780a7a92565f9aa47) )

	NEO_BIOS_SOUND_128K( "227-m1.bin", CRC(c8e5e0f9) SHA1(09bb05ae6f09b59b9e4871fae1fc7c3bafd07394) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "227-v1.bin", 0x000000, 0x400000, CRC(b8f24181) SHA1(0a3af88d20ff65b82c58325d32c20b99fc07f7f3) )
	ROM_LOAD( "227-v2.bin", 0x400000, 0x400000, CRC(ee39e260) SHA1(4ed6802564ce262ebe92c7276424056b70998758) )

	NO_DELTAT_REGION

	ROM_REGION( 0xc00000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "227-c1.bin", 0x0000000, 0x400000, CRC(7d6c2af4) SHA1(e54f0ab15c95d7a6f965b5d8ab28b5445100650b) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "227-c2.bin", 0x0000001, 0x400000, CRC(7e402d39) SHA1(9d3a44f98ddd0b606c8b3efa0c6b9d5a46c0bfeb) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "227-c3.bin", 0x0800000, 0x200000, CRC(93dfd660) SHA1(5b473c556ef919cd7a872351dbb20a636aae32b6) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "227-c4.bin", 0x0800001, 0x200000, CRC(7efea43a) SHA1(3f2b1718fe7be06b6d75ec34badc2de2a3554d3e) ) /* Plane 2,3 */
ROM_END

ROM_START( breakers )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "230-p1.bin", 0x100000, 0x100000, CRC(ed24a6e6) SHA1(3fb77ae696d92d2f9a5d589e08b708545c7cda0a) )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "230-s1.bin", CRC(076fb64c) SHA1(c166038128d7004f69932141f83b320a35c2b4ca) )

	NEO_BIOS_SOUND_128K( "230-m1.bin", CRC(3951a1c1) SHA1(1e6442a7ea82ada9503d71045dd93e12bd05254f) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "230-v1.bin", 0x000000, 0x400000, CRC(7f9ed279) SHA1(acd0558960ec29bfc3e3ee99d00e503bebff8513) )
	ROM_LOAD( "230-v2.bin", 0x400000, 0x400000, CRC(1d43e420) SHA1(26d09b8b18b4b802dbda4d6f06626c24d0b7c512) )

	NO_DELTAT_REGION

	ROM_REGION( 0x1000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "230-c1.bin", 0x000000, 0x400000, CRC(68d4ae76) SHA1(2e820067f6963669f104bebf19e865fe4127b4dd) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "230-c2.bin", 0x000001, 0x400000, CRC(fdee05cd) SHA1(efc4ffd790953ac7c25d5f045c64a9b49d24b096) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "230-c3.bin", 0x800000, 0x400000, CRC(645077f3) SHA1(0ae74f3b4b3b88f128c6d8c0f35ffa53f5d67ef2) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "230-c4.bin", 0x800001, 0x400000, CRC(63aeb74c) SHA1(9ff6930c0c3d79b46b86356e8565ce4fcd69ac38) ) /* Plane 2,3 */
ROM_END

ROM_START( miexchng )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "231-p1.bin", 0x000000, 0x80000, CRC(61be1810) SHA1(1ab0e11352ca05329c6e3f5657b60e4a227fcbfb) )

	NEO_SFIX_128K( "231-s1.bin", CRC(fe0c0c53) SHA1(54d56d4463db193e504658f4f6f4997a62ae3d95) )

	NEO_BIOS_SOUND_128K( "231-m1.bin", CRC(de41301b) SHA1(59ce3836ac8f064d56a446c9374f05bcb40fcfd8) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "231-v1.bin", 0x000000, 0x400000, CRC(113fb898) SHA1(9168ba90c4aa969f69eb11ba3f4d76592d81e05a) )

	NO_DELTAT_REGION

	ROM_REGION( 0x500000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "231-c1.bin", 0x000000, 0x200000, CRC(6c403ba3) SHA1(3830446fbd07d5a6564f9ac68a4bec5ff5b7d5c9) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "231-c2.bin", 0x000001, 0x200000, CRC(554bcd9b) SHA1(e658161618bd41a66f1040be409efdea28020cf6) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "231-c3.bin", 0x400000, 0x080000, CRC(14524eb5) SHA1(e090b99a1ee2cca4a7f7d5262e764d1cab6bada2) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "231-c4.bin", 0x400001, 0x080000, CRC(1694f171) SHA1(a6af5d3f1e0b3f73da275e04d4434c6c9c7b9208) ) /* Plane 2,3 */
ROM_END

ROM_START( kof97 )
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "232-p1.bin", 0x000000, 0x100000, CRC(7db81ad9) SHA1(8bc42be872fd497eb198ca13bf004852b88eb1dc) )
	ROM_LOAD16_WORD_SWAP( "232-p2.bin", 0x100000, 0x400000, CRC(158b23f6) SHA1(9744620a70513490aaf9c5eda33e5ec31222be19) )

	NEO_SFIX_128K( "232-s1.bin", CRC(8514ecf5) SHA1(18d8e7feb51ea88816f1c786932a53655b0de6a0) )

	NEO_BIOS_SOUND_128K( "232-m1.bin", CRC(45348747) SHA1(ed77cbae2b208d1177a9f5f6e8cd57070e90b65b) )

	ROM_REGION( 0xc00000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "232-v1.bin", 0x000000, 0x400000, CRC(22a2b5b5) SHA1(ebdbc977332e6d93e266755000b43857e0082965) )
	ROM_LOAD( "232-v2.bin", 0x400000, 0x400000, CRC(2304e744) SHA1(98d283e2bcc9291a53f52afd35ef76dfb0828432) )
	ROM_LOAD( "232-v3.bin", 0x800000, 0x400000, CRC(759eb954) SHA1(54e77c4e9e6b89458e59824e478ddc33a9c72655) )

	NO_DELTAT_REGION

	ROM_REGION( 0x2800000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "232-c1.bin", 0x0000000, 0x800000, CRC(5f8bf0a1) SHA1(e8b63bbc814de171fd18c5864a7fc639970c1ecf) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "232-c2.bin", 0x0000001, 0x800000, CRC(e4d45c81) SHA1(fdb2b9326362e27b1c7a5beb977e0bc537488186) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "232-c3.bin", 0x1000000, 0x800000, CRC(581d6618) SHA1(14d3124a08ded59f86932c6b28e1a4e48c564ccd) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "232-c4.bin", 0x1000001, 0x800000, CRC(49bb1e68) SHA1(f769c1bd1b019521111ff3f0d22c63cb1f2640ef) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "232-c5.bin", 0x2000000, 0x400000, CRC(34fc4e51) SHA1(b39c65f27873f71a6f5a5d1d04e5435f874472ee) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "232-c6.bin", 0x2000001, 0x400000, CRC(4ff4d47b) SHA1(4d5689ede24a5fe4330bd85d4d3f4eb2795308bb) ) /* Plane 2,3 */
ROM_END

ROM_START( kof97a )
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "232-pg1.bin",0x000000, 0x100000, CRC(5c2400b7) SHA1(49e23f80c012c62146a1bb8f254a7597823de430) )
	ROM_LOAD16_WORD_SWAP( "232-p2.bin", 0x100000, 0x400000, CRC(158b23f6) SHA1(9744620a70513490aaf9c5eda33e5ec31222be19) )

	NEO_SFIX_128K( "232-s1.bin", CRC(8514ecf5) SHA1(18d8e7feb51ea88816f1c786932a53655b0de6a0) )

	NEO_BIOS_SOUND_128K( "232-m1.bin", CRC(45348747) SHA1(ed77cbae2b208d1177a9f5f6e8cd57070e90b65b) )

	ROM_REGION( 0xc00000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "232-v1.bin", 0x000000, 0x400000, CRC(22a2b5b5) SHA1(ebdbc977332e6d93e266755000b43857e0082965) )
	ROM_LOAD( "232-v2.bin", 0x400000, 0x400000, CRC(2304e744) SHA1(98d283e2bcc9291a53f52afd35ef76dfb0828432) )
	ROM_LOAD( "232-v3.bin", 0x800000, 0x400000, CRC(759eb954) SHA1(54e77c4e9e6b89458e59824e478ddc33a9c72655) )

	NO_DELTAT_REGION

	ROM_REGION( 0x2800000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "232-c1.bin", 0x0000000, 0x800000, CRC(5f8bf0a1) SHA1(e8b63bbc814de171fd18c5864a7fc639970c1ecf) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "232-c2.bin", 0x0000001, 0x800000, CRC(e4d45c81) SHA1(fdb2b9326362e27b1c7a5beb977e0bc537488186) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "232-c3.bin", 0x1000000, 0x800000, CRC(581d6618) SHA1(14d3124a08ded59f86932c6b28e1a4e48c564ccd) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "232-c4.bin", 0x1000001, 0x800000, CRC(49bb1e68) SHA1(f769c1bd1b019521111ff3f0d22c63cb1f2640ef) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "232-c5.bin", 0x2000000, 0x400000, CRC(34fc4e51) SHA1(b39c65f27873f71a6f5a5d1d04e5435f874472ee) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "232-c6.bin", 0x2000001, 0x400000, CRC(4ff4d47b) SHA1(4d5689ede24a5fe4330bd85d4d3f4eb2795308bb) ) /* Plane 2,3 */
ROM_END

ROM_START( kof97pls ) /* bootleg */
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "232-p1p.bin", 0x000000, 0x100000, CRC(c01fda46) SHA1(bc6402f5082efc80a8936364c657165f19b49415) )
	ROM_LOAD16_WORD_SWAP( "232-p2p.bin", 0x100000, 0x400000, CRC(5502b020) SHA1(37c48198d8b3798910a44075782cd1a20b687b4a) )

	NEO_SFIX_128K( "232-s1p.bin", CRC(73254270) SHA1(8d06305f9d8890da1327356272b88bdd0dc089f5) )

	NEO_BIOS_SOUND_128K( "232-m1.bin", CRC(45348747) SHA1(ed77cbae2b208d1177a9f5f6e8cd57070e90b65b) )

	ROM_REGION( 0xc00000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "232-v1.bin", 0x000000, 0x400000, CRC(22a2b5b5) SHA1(ebdbc977332e6d93e266755000b43857e0082965) )
	ROM_LOAD( "232-v2.bin", 0x400000, 0x400000, CRC(2304e744) SHA1(98d283e2bcc9291a53f52afd35ef76dfb0828432) )
	ROM_LOAD( "232-v3.bin", 0x800000, 0x400000, CRC(759eb954) SHA1(54e77c4e9e6b89458e59824e478ddc33a9c72655) )

	NO_DELTAT_REGION

	ROM_REGION( 0x2800000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "232-c1.bin", 0x0000000, 0x800000, CRC(5f8bf0a1) SHA1(e8b63bbc814de171fd18c5864a7fc639970c1ecf) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "232-c2.bin", 0x0000001, 0x800000, CRC(e4d45c81) SHA1(fdb2b9326362e27b1c7a5beb977e0bc537488186) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "232-c3.bin", 0x1000000, 0x800000, CRC(581d6618) SHA1(14d3124a08ded59f86932c6b28e1a4e48c564ccd) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "232-c4.bin", 0x1000001, 0x800000, CRC(49bb1e68) SHA1(f769c1bd1b019521111ff3f0d22c63cb1f2640ef) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "232-c5.bin", 0x2000000, 0x400000, CRC(34fc4e51) SHA1(b39c65f27873f71a6f5a5d1d04e5435f874472ee) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "232-c6.bin", 0x2000001, 0x400000, CRC(4ff4d47b) SHA1(4d5689ede24a5fe4330bd85d4d3f4eb2795308bb) ) /* Plane 2,3 */
ROM_END

ROM_START( kog )
	ROM_REGION( 0x600000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "5232-p1.bin", 0x000000, 0x200000, CRC(d2413ec6) SHA1(c0bf409d1e714cba5fdc6f79e4c2aec805316634) )
	ROM_LOAD16_WORD_SWAP( "232-p2.bin",  0x200000, 0x400000, CRC(158b23f6) SHA1(9744620a70513490aaf9c5eda33e5ec31222be19) )

	NEO_SFIX_128K( "5232-s1.bin", CRC(0bef69da) SHA1(80918586e694dce35c4dba796eb18abf6a070ebb) )

	NEO_BIOS_SOUND_128K( "232-m1.bin", CRC(45348747) SHA1(ed77cbae2b208d1177a9f5f6e8cd57070e90b65b) )

	ROM_REGION( 0xc00000, REGION_SOUND1, 0 )
	ROM_LOAD( "232-v1.bin", 0x000000, 0x400000, CRC(22a2b5b5) SHA1(ebdbc977332e6d93e266755000b43857e0082965) )
	ROM_LOAD( "232-v2.bin", 0x400000, 0x400000, CRC(2304e744) SHA1(98d283e2bcc9291a53f52afd35ef76dfb0828432) )
	ROM_LOAD( "232-v3.bin", 0x800000, 0x400000, CRC(759eb954) SHA1(54e77c4e9e6b89458e59824e478ddc33a9c72655) )

	ROM_REGION( 0x2800000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "5232-c1a.bin", 0x0000000, 0x800000, CRC(4eab9b0a) SHA1(a6f6b755215a3f41474e0a76b5463303a522c2d3) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "5232-c2a.bin", 0x0000001, 0x800000, CRC(697f8fd0) SHA1(5784464c2357ccef8e6e79b6298843fc3d13b39c) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "5232-c1b.bin", 0x1000000, 0x800000, CRC(1143fdf3) SHA1(9dc5fe9a3b7599380db62095880e2d6f237a41bd) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "5232-c2b.bin", 0x1000001, 0x800000, CRC(ea82cf8f) SHA1(3d9ab64b69cecd6b3950839ac2c6d151ad66dcf8) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "5232-c3.bin", 0x2000000, 0x400000, CRC(abd1be07) SHA1(857eb68bbee4538770bbfa77aaa540d61ab0abcd) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "5232-c4.bin", 0x2000001, 0x400000, CRC(d2bd967b) SHA1(c494e0a98e127d37ca360a28accc167fa50fb626) ) /* Plane 2,3 */
ROM_END

ROM_START( magdrop3 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "233-p1.bin", 0x000000, 0x100000, CRC(931e17fa) SHA1(4a95c4b79d0878485ce272e9f4c4f647bec0e070) )

	NEO_SFIX_128K( "233-s1.bin", CRC(7399e68a) SHA1(b535ee56a0f0995f04674e676f6aa636ffad26aa) )

	NEO_BIOS_SOUND_128K( "233-m1.bin", CRC(5beaf34e) SHA1(2905d26945932cddc2dd3a1dc5abba8aa3baee14) )

	ROM_REGION( 0x480000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "233-v1.bin", 0x000000, 0x400000, CRC(58839298) SHA1(18cae7bba997c52780761cbf119c4e4b34397a61) )
	ROM_LOAD( "233-v2.bin", 0x400000, 0x080000, CRC(d5e30df4) SHA1(bbbc0ff5b975471bd682f85976ac4a93f6d44f2e) )

	NO_DELTAT_REGION

	ROM_REGION( 0x1000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "233-c1.bin", 0x400000, 0x200000, CRC(734db3d6) SHA1(6d12b3ef34aae066f8c5cae5f6a272c8f482bdd0) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x000000, 0x200000 )
	ROM_LOAD16_BYTE( "233-c2.bin", 0x400001, 0x200000, CRC(d78f50e5) SHA1(102526982596a9d3052d3f3181a98558c596c907) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x000001, 0x200000 )
	ROM_LOAD16_BYTE( "233-c3.bin", 0xc00000, 0x200000, CRC(ec65f472) SHA1(23271ca8617d08f23417dd071333c91ef90715b5) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x800000, 0x200000 )
	ROM_LOAD16_BYTE( "233-c4.bin", 0xc00001, 0x200000, CRC(f55dddf3) SHA1(fec0930e5cb26be4d73bfa8c76ef37eb4bbec60a) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x800001, 0x200000 )
ROM_END

ROM_START( lastblad )
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "234-p1.bin", 0x000000, 0x100000, CRC(cd01c06d) SHA1(d66142571afe07c6191b52f319f1bc8bc8541c14) )
	ROM_LOAD16_WORD_SWAP( "234-p2.bin", 0x100000, 0x400000, CRC(0fdc289e) SHA1(1ff31c0b0f4f9ddbedaf4bcf927faaae81892ec7) )

	NEO_SFIX_128K( "234-s1.bin", CRC(95561412) SHA1(995de272f572fd08d909d3d0af4251b9957b3640) )

	NEO_BIOS_SOUND_128K( "234-m1.bin", CRC(087628ea) SHA1(48dcf739bb16699af4ab8ed632b7dcb25e470e06) )

	ROM_REGION( 0xe00000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "234-v1.bin", 0x000000, 0x400000, CRC(ed66b76f) SHA1(8a05ff06d9b6f01c6c16b3026282eaabb0e25b44) )
	ROM_LOAD( "234-v2.bin", 0x400000, 0x400000, CRC(a0e7f6e2) SHA1(753ff74fa9294f695aae511ae01ead119b114a57) )
	ROM_LOAD( "234-v3.bin", 0x800000, 0x400000, CRC(a506e1e2) SHA1(b3e04ba1a5cb50b77c6fbe9fe353b9b64b6f3f74) )
	ROM_LOAD( "234-v4.bin", 0xc00000, 0x200000, CRC(13583c4b) SHA1(8b84dd4117bb0d535f30531499621622967b2344) )

	NO_DELTAT_REGION

	ROM_REGION( 0x2400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "234-c1.bin", 0x0000000, 0x800000, CRC(9f7e2bd3) SHA1(2828aca0c0f5802110f10453c1cf640f69736554) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "234-c2.bin", 0x0000001, 0x800000, CRC(80623d3c) SHA1(ad460615115ec8fb25206f012da59ecfc8059b64) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "234-c3.bin", 0x1000000, 0x800000, CRC(91ab1a30) SHA1(e3cf9133784bef2c8f1bfe45f277ccf82cc6f6a1) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "234-c4.bin", 0x1000001, 0x800000, CRC(3d60b037) SHA1(78a50233bcd19e92c7b6f7ee1a53417d9db21f6a) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "234-c5.bin", 0x2000000, 0x200000, CRC(17bbd7ca) SHA1(7abb4ae5d3f5bb488c20ffe59496316bce999c23) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "234-c6.bin", 0x2000001, 0x200000, CRC(5c35d541) SHA1(6bdd3e8abc0e577420762aea5ab20b1360868905) ) /* Plane 2,3 */
ROM_END

ROM_START( lastblda )
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "234-p1.rom", 0x000000, 0x100000, CRC(e123a5a3) SHA1(a3ddabc00feeb54272b145246612ad4632b0e413) )
	ROM_LOAD16_WORD_SWAP( "234-p2.bin", 0x100000, 0x400000, CRC(0fdc289e) SHA1(1ff31c0b0f4f9ddbedaf4bcf927faaae81892ec7) )

	NEO_SFIX_128K( "234-s1.bin", CRC(95561412) SHA1(995de272f572fd08d909d3d0af4251b9957b3640) )

	NEO_BIOS_SOUND_128K( "234-m1.bin", CRC(087628ea) SHA1(48dcf739bb16699af4ab8ed632b7dcb25e470e06) )

	ROM_REGION( 0xe00000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "234-v1.bin", 0x000000, 0x400000, CRC(ed66b76f) SHA1(8a05ff06d9b6f01c6c16b3026282eaabb0e25b44) )
	ROM_LOAD( "234-v2.bin", 0x400000, 0x400000, CRC(a0e7f6e2) SHA1(753ff74fa9294f695aae511ae01ead119b114a57) )
	ROM_LOAD( "234-v3.bin", 0x800000, 0x400000, CRC(a506e1e2) SHA1(b3e04ba1a5cb50b77c6fbe9fe353b9b64b6f3f74) )
	ROM_LOAD( "234-v4.bin", 0xc00000, 0x200000, CRC(13583c4b) SHA1(8b84dd4117bb0d535f30531499621622967b2344) )

	NO_DELTAT_REGION

	ROM_REGION( 0x2400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "234-c1.bin", 0x0000000, 0x800000, CRC(9f7e2bd3) SHA1(2828aca0c0f5802110f10453c1cf640f69736554) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "234-c2.bin", 0x0000001, 0x800000, CRC(80623d3c) SHA1(ad460615115ec8fb25206f012da59ecfc8059b64) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "234-c3.bin", 0x1000000, 0x800000, CRC(91ab1a30) SHA1(e3cf9133784bef2c8f1bfe45f277ccf82cc6f6a1) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "234-c4.bin", 0x1000001, 0x800000, CRC(3d60b037) SHA1(78a50233bcd19e92c7b6f7ee1a53417d9db21f6a) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "234-c5.bin", 0x2000000, 0x200000, CRC(17bbd7ca) SHA1(7abb4ae5d3f5bb488c20ffe59496316bce999c23) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "234-c6.bin", 0x2000001, 0x200000, CRC(5c35d541) SHA1(6bdd3e8abc0e577420762aea5ab20b1360868905) ) /* Plane 2,3 */
ROM_END

ROM_START( puzzldpr )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "235-p1.bin", 0x000000, 0x080000, CRC(afed5de2) SHA1(a5d82c6dbe687505e8c8d7339908da45cd379a0b) )

	NEO_SFIX_64K( "235-s1.bin", CRC(5a68d91e) SHA1(a8c58eb68fd7e6e2d9d1153a9da514430437f342) )

	NEO_BIOS_SOUND_128K( "202-m1.bin", CRC(9c0291ea) SHA1(3fa67c62acba79be6b3a98cc1601e45569fa11ae) )

	ROM_REGION( 0x080000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "202-v1.bin", 0x000000, 0x080000, CRC(debeb8fb) SHA1(49a3d3578c087f1a0050168571ef8d1b08c5dc05) )

	NO_DELTAT_REGION

	ROM_REGION( 0x200000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "202-c1.bin", 0x000000, 0x100000, CRC(cc0095ef) SHA1(3d86f455e6db10a2449b775dc386f1826ba3b62e) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "202-c2.bin", 0x000001, 0x100000, CRC(42371307) SHA1(df794f989e2883634bf7ffeea48d6bc3854529af) ) /* Plane 2,3 */
ROM_END

ROM_START( irrmaze )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "236-p1.bin", 0x000000, 0x200000, CRC(6d536c6e) SHA1(87d66683304a6617da8af7dfdfcbf4a3ab63056a) )

	NEO_SFIX_128K( "236-s1.bin", CRC(5d1ca640) SHA1(40a9668a1742a44597a07ce72273d17119815637) )

	ROM_REGION16_BE( 0x20000, REGION_USER1, 0 )
	/* special BIOS with trackball support */
	IRRMAZE_BIOS
	ROM_REGION( 0x50000, REGION_CPU2, 0 )
	ROM_LOAD( "mame.sm1", 0x00000, 0x20000, CRC(97cf998b) SHA1(977387a7c76ef9b21d0b01fa69830e949a9a9626) )  /* we don't use the BIOS anyway... */
	ROM_LOAD( "236-m1.bin", 0x00000, 0x20000, CRC(880a1abd) SHA1(905afa157aba700e798243b842792e50729b19a0) )  /* so overwrite it with the real thing */
	ROM_RELOAD(             0x10000, 0x20000 )
	ROM_REGION( 0x10000, REGION_GFX4, 0 )
	ROM_LOAD( "mamelo.lo", 0x00000, 0x10000, CRC(e09e253c) SHA1(2b1c719531dac9bb503f22644e6e4236b91e7cfc) )  /* Y zoom control */

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "236-v1.bin", 0x000000, 0x200000, CRC(5f89c3b4) SHA1(dc8fd561cf8dfdd41696dcf14ea8d2d0ac4eec4b) )

	ROM_REGION( 0x100000, REGION_SOUND2, ROMREGION_SOUNDONLY )
	ROM_LOAD( "236-v2.bin", 0x000000, 0x100000, CRC(1e843567) SHA1(30d63887b4900571025b3077b9e41099a59c3ad9) )

	ROM_REGION( 0x0800000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "236-c1.bin", 0x000000, 0x400000, CRC(c1d47902) SHA1(727001c34f979226fc8f581113ce2aaac4fc0d42) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "236-c2.bin", 0x000001, 0x400000, CRC(e15f972e) SHA1(6a329559c57a67be73a6733513b59e9e6c8d61cc) ) /* Plane 2,3 */
ROM_END

ROM_START( popbounc )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "237-p1.bin", 0x000000, 0x100000, CRC(be96e44f) SHA1(43679da8664fbb491103a1108040ddf94d59fc2b) )

	NEO_SFIX_128K( "237-s1.bin", CRC(b61cf595) SHA1(b14f8b78af7c634d41cf34d36b11b116e61f7342) )

	NEO_BIOS_SOUND_128K( "237-m1.bin", CRC(d4c946dd) SHA1(6ca09040b5db8d89511d627954c783154d58ab01) )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "237-v1.bin", 0x000000, 0x200000, CRC(edcb1beb) SHA1(62f086b9968b366b59276ee4ae3c32c4d76fc6ce) )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "237-c1.bin", 0x000000, 0x200000, CRC(eda42d66) SHA1(2735538fcb9dc0c16e043a8728c8b642650189f4) )
	ROM_LOAD16_BYTE( "237-c2.bin", 0x000001, 0x200000, CRC(5e633c65) SHA1(9a82107caf027317c173c1c1ef676f0fdeea79b2) )
ROM_END

ROM_START( shocktro )
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "238-p1.bin", 0x000000, 0x100000, CRC(5677456f) SHA1(f76169fa5c90871d65e2a16fd1bb036c90533ac8) )
	ROM_LOAD16_WORD_SWAP( "238-p2.bin", 0x300000, 0x200000, CRC(646f6c76) SHA1(d8fb851414995ba09dbe397d9ed1c765a52d8e1e) )
	ROM_CONTINUE(						0x100000, 0x200000 )

	NEO_SFIX_128K( "238-s1.bin", CRC(1f95cedb) SHA1(adfa74868147fd260481e4c387d254d3b6de83f4) )

	NEO_BIOS_SOUND_128K( "238-m1.bin", CRC(075b9518) SHA1(ac21b88a860b9572bf24432b4cadcc96d108055d) )

	ROM_REGION( 0x600000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "238-v1.bin", 0x000000, 0x400000, CRC(260c0bef) SHA1(9c4f80ce4bb205afed11bb8b8926d20748eb5512) )
	ROM_LOAD( "238-v2.bin", 0x400000, 0x200000, CRC(4ad7d59e) SHA1(bfdf2684f7f38af4e75ad0068ff9463dc2601598) )

	NO_DELTAT_REGION

	ROM_REGION( 0x2000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "238-c1.bin", 0x0400000, 0x200000, CRC(aad087fc) SHA1(7246269496d53b4af2ee12a69620f29272ea9037) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0000000, 0x200000 )
	ROM_LOAD16_BYTE( "238-c2.bin", 0x0400001, 0x200000, CRC(7e39df1f) SHA1(ecead5bf06dc5719de0ae7593560b37e0f1481b0) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0000001, 0x200000 )
	ROM_LOAD16_BYTE( "238-c3.bin", 0x0c00000, 0x200000, CRC(6682a458) SHA1(2dc2c8c88d5c471869b08596eef585ad18f2b370) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0800000, 0x200000 )
	ROM_LOAD16_BYTE( "238-c4.bin", 0x0c00001, 0x200000, CRC(cbef1f17) SHA1(25080c4f4ac4e7075c4f976d8fa6ab4d8f7d7237) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0800001, 0x200000 )
	ROM_LOAD16_BYTE( "238-c5.bin", 0x1400000, 0x200000, CRC(e17762b1) SHA1(1cc8cb9e485d86e5e0e50bcbe0e81cfb273f0664) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x1000000, 0x200000 )
	ROM_LOAD16_BYTE( "238-c6.bin", 0x1400001, 0x200000, CRC(28beab71) SHA1(683abb317b1b95ba122859aea701f52197c10208) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x1000001, 0x200000 )
	ROM_LOAD16_BYTE( "238-c7.bin", 0x1c00000, 0x200000, CRC(a47e62d2) SHA1(39285f78a9ed1138034de91efb7045084a5925f0) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x1800000, 0x200000 )
	ROM_LOAD16_BYTE( "238-c8.bin", 0x1c00001, 0x200000, CRC(e8e890fb) SHA1(abbbfa0e162d7711d62df08a9721d8c923888c78) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x1800001, 0x200000 )
ROM_END

ROM_START( shocktra )
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "238-pg1.p1", 0x000000, 0x100000, CRC(efedf8dc) SHA1(f638df9bf7aa7d514ee2bccfc7f2adbf39ca83fc) )
	ROM_LOAD16_WORD_SWAP( "238-p2.bin", 0x300000, 0x200000, CRC(646f6c76) SHA1(d8fb851414995ba09dbe397d9ed1c765a52d8e1e) )
	ROM_CONTINUE(						0x100000, 0x200000 )

	NEO_SFIX_128K( "238-s1.bin", CRC(1f95cedb) SHA1(adfa74868147fd260481e4c387d254d3b6de83f4) )

	NEO_BIOS_SOUND_128K( "238-m1.bin", CRC(075b9518) SHA1(ac21b88a860b9572bf24432b4cadcc96d108055d) )

	ROM_REGION( 0x600000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "238-v1.bin", 0x000000, 0x400000, CRC(260c0bef) SHA1(9c4f80ce4bb205afed11bb8b8926d20748eb5512) )
	ROM_LOAD( "238-v2.bin", 0x400000, 0x200000, CRC(4ad7d59e) SHA1(bfdf2684f7f38af4e75ad0068ff9463dc2601598) )

	NO_DELTAT_REGION

	ROM_REGION( 0x2000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "238-c1.bin", 0x0400000, 0x200000, CRC(aad087fc) SHA1(7246269496d53b4af2ee12a69620f29272ea9037) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0000000, 0x200000 )
	ROM_LOAD16_BYTE( "238-c2.bin", 0x0400001, 0x200000, CRC(7e39df1f) SHA1(ecead5bf06dc5719de0ae7593560b37e0f1481b0) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0000001, 0x200000 )
	ROM_LOAD16_BYTE( "238-c3.bin", 0x0c00000, 0x200000, CRC(6682a458) SHA1(2dc2c8c88d5c471869b08596eef585ad18f2b370) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0800000, 0x200000 )
	ROM_LOAD16_BYTE( "238-c4.bin", 0x0c00001, 0x200000, CRC(cbef1f17) SHA1(25080c4f4ac4e7075c4f976d8fa6ab4d8f7d7237) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0800001, 0x200000 )
	ROM_LOAD16_BYTE( "238-c5.bin", 0x1400000, 0x200000, CRC(e17762b1) SHA1(1cc8cb9e485d86e5e0e50bcbe0e81cfb273f0664) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x1000000, 0x200000 )
	ROM_LOAD16_BYTE( "238-c6.bin", 0x1400001, 0x200000, CRC(28beab71) SHA1(683abb317b1b95ba122859aea701f52197c10208) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x1000001, 0x200000 )
	ROM_LOAD16_BYTE( "238-c7.bin", 0x1c00000, 0x200000, CRC(a47e62d2) SHA1(39285f78a9ed1138034de91efb7045084a5925f0) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x1800000, 0x200000 )
	ROM_LOAD16_BYTE( "238-c8.bin", 0x1c00001, 0x200000, CRC(e8e890fb) SHA1(abbbfa0e162d7711d62df08a9721d8c923888c78) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x1800001, 0x200000 )
ROM_END

ROM_START( blazstar )
	ROM_REGION( 0x300000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "239-p1.bin", 0x000000, 0x100000, CRC(183682f8) SHA1(dcee1c2cf4a991ca1f9f2b40c4a738f21682807b) )
	ROM_LOAD16_WORD_SWAP( "239-p2.bin", 0x100000, 0x200000, CRC(9a9f4154) SHA1(f8805453d0995c8fa16cd9accfb7a990071ca630) )

	NEO_SFIX_128K( "239-s1.bin", CRC(d56cb498) SHA1(420ce56431dc7f3f7de84fcbc8c0a17b5eab205e) )

	NEO_BIOS_SOUND_128K( "239-m1.bin", CRC(d31a3aea) SHA1(e23abfeb23052f0358edcf2c83401025fe632511) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "239-v1.bin", 0x000000, 0x400000, CRC(1b8d5bf7) SHA1(67fc1f7e36e92a89cd1d415eb31a2892f57b0d04) )
	ROM_LOAD( "239-v2.bin", 0x400000, 0x400000, CRC(74cf0a70) SHA1(b00451a2a30de2517ae3eca35eb1fe985b950eb8) )

	NO_DELTAT_REGION

	ROM_REGION( 0x2000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "239-c1.bin", 0x0400000, 0x200000, CRC(754744e0) SHA1(8f498bc1722189c037568d0fe72d2012e87a4c85) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0000000, 0x200000 )
	ROM_LOAD16_BYTE( "239-c2.bin", 0x0400001, 0x200000, CRC(af98c037) SHA1(954a860c79544310685ee22b29fd2153bee8acce) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0000001, 0x200000 )
	ROM_LOAD16_BYTE( "239-c3.bin", 0x0c00000, 0x200000, CRC(7b39b590) SHA1(27c606539f626df039ba7376dc1feeee8dc82911) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0800000, 0x200000 )
	ROM_LOAD16_BYTE( "239-c4.bin", 0x0c00001, 0x200000, CRC(6e731b30) SHA1(3499b8d9fbb881cdaf5d3022533ae1fd45a125e9) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0800001, 0x200000 )
	ROM_LOAD16_BYTE( "239-c5.bin", 0x1400000, 0x200000, CRC(9ceb113b) SHA1(cb1318ff1e814fc797ba17f9fc0764d944eedf86) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x1000000, 0x200000 )
	ROM_LOAD16_BYTE( "239-c6.bin", 0x1400001, 0x200000, CRC(6a78e810) SHA1(f55e95d467851d790b10612ebc2e0dee352f49b9) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x1000001, 0x200000 )
	ROM_LOAD16_BYTE( "239-c7.bin", 0x1c00000, 0x200000, CRC(50d28eca) SHA1(3e0f513fe07d16484a8c3896c8cb1b14c7e1a7bb) ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x1800000, 0x200000 )
	ROM_LOAD16_BYTE( "239-c8.bin", 0x1c00001, 0x200000, CRC(cdbbb7d7) SHA1(eadd2711bcb54e1ea6c449b459843ac3c0ac8415) ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x1800001, 0x200000 )
ROM_END

ROM_START( rbff2 )
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "240-p1.bin", 0x000000, 0x100000, CRC(b6969780) SHA1(e3373d18e0f0724d69efb8024a27cca121f1b5b2) )
	ROM_LOAD16_WORD_SWAP( "240-p2.bin", 0x100000, 0x400000, CRC(960aa88d) SHA1(3d9e785891871af90313f178dca2724633406674) )

	NEO_SFIX_128K( "240-s1.bin",  CRC(da3b40de) SHA1(e6bf74e057ac6fe1f249a7547f13ba7fbc694561) )

	NEO_BIOS_SOUND_256K( "240-m1.bin", CRC(ed482791) SHA1(1f54a45967cb7842c33aa24be322c9f33ff75ac3) )

	ROM_REGION( 0x1000000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "240-v1.bin", 0x000000, 0x400000, CRC(f796265a) SHA1(736dff37eb91fc856b4d189249fb0de9b6c0813a) )
	ROM_LOAD( "240-v2.bin", 0x400000, 0x400000, CRC(2cb3f3bb) SHA1(697e677890892f4b028c9a27c66809ca0a8a9b18) )
	ROM_LOAD( "240-v3.bin", 0x800000, 0x400000, CRC(df77b7fa) SHA1(4df971ce20bdb8c1ce8cc1692a32ac69505ffa9a) )
	ROM_LOAD( "240-v4.bin", 0xc00000, 0x400000, CRC(33a356ee) SHA1(b2a08052be670f301f8d4903b36c70088d1a11da) )

	NO_DELTAT_REGION

	ROM_REGION( 0x3000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "240-c1.bin", 0x0000000, 0x800000, CRC(effac504) SHA1(e36a96e7369b02c7e839b5abf3c6799453ba1927) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "240-c2.bin", 0x0000001, 0x800000, CRC(ed182d44) SHA1(a9fc0a3a786bf067c129ec7220df65953dff804f) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "240-c3.bin", 0x1000000, 0x800000, CRC(22e0330a) SHA1(0fe7f6a8aeba7f17dbb278e85003969ff10d3cd2) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "240-c4.bin", 0x1000001, 0x800000, CRC(c19a07eb) SHA1(139eac8b51cadf328dd42d8109f4e2463f57230c) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "240-c5.bin", 0x2000000, 0x800000, CRC(244dff5a) SHA1(156548156d3ceaa808d0053d0749af2526a3943e) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "240-c6.bin", 0x2000001, 0x800000, CRC(4609e507) SHA1(bb17f50a377dddb77c1eeda5944a7bcbf0cca5f7) ) /* Plane 2,3 */
ROM_END

ROM_START( rbff2a )
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "240-p1.rom", 0x000000, 0x100000, CRC(80e41205) SHA1(8f83eb8ff54be4ec40f8a0dd2cbe56c54908d00a) )
	ROM_LOAD16_WORD_SWAP( "240-p2.bin", 0x100000, 0x400000, CRC(960aa88d) SHA1(3d9e785891871af90313f178dca2724633406674) )

	NEO_SFIX_128K( "240-s1.bin",  CRC(da3b40de) SHA1(e6bf74e057ac6fe1f249a7547f13ba7fbc694561) )

	NEO_BIOS_SOUND_256K( "240-m1.bin", CRC(ed482791) SHA1(1f54a45967cb7842c33aa24be322c9f33ff75ac3) )

	ROM_REGION( 0x1000000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "240-v1.bin", 0x000000, 0x400000, CRC(f796265a) SHA1(736dff37eb91fc856b4d189249fb0de9b6c0813a) )
	ROM_LOAD( "240-v2.bin", 0x400000, 0x400000, CRC(2cb3f3bb) SHA1(697e677890892f4b028c9a27c66809ca0a8a9b18) )
	ROM_LOAD( "240-v3.bin", 0x800000, 0x400000, CRC(df77b7fa) SHA1(4df971ce20bdb8c1ce8cc1692a32ac69505ffa9a) )
	ROM_LOAD( "240-v4.bin", 0xc00000, 0x400000, CRC(33a356ee) SHA1(b2a08052be670f301f8d4903b36c70088d1a11da) )

	NO_DELTAT_REGION

	ROM_REGION( 0x3000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "240-c1.bin", 0x0000000, 0x800000, CRC(effac504) SHA1(e36a96e7369b02c7e839b5abf3c6799453ba1927) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "240-c2.bin", 0x0000001, 0x800000, CRC(ed182d44) SHA1(a9fc0a3a786bf067c129ec7220df65953dff804f) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "240-c3.bin", 0x1000000, 0x800000, CRC(22e0330a) SHA1(0fe7f6a8aeba7f17dbb278e85003969ff10d3cd2) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "240-c4.bin", 0x1000001, 0x800000, CRC(c19a07eb) SHA1(139eac8b51cadf328dd42d8109f4e2463f57230c) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "240-c5.bin", 0x2000000, 0x800000, CRC(244dff5a) SHA1(156548156d3ceaa808d0053d0749af2526a3943e) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "240-c6.bin", 0x2000001, 0x800000, CRC(4609e507) SHA1(bb17f50a377dddb77c1eeda5944a7bcbf0cca5f7) ) /* Plane 2,3 */
ROM_END

ROM_START( mslug2 )
	ROM_REGION( 0x300000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "241-p1.bin", 0x000000, 0x100000, CRC(2a53c5da) SHA1(5a6aba482cac588a6c2c51179c95b487c6e11899) )
	ROM_LOAD16_WORD_SWAP( "241-p2.bin", 0x100000, 0x200000, CRC(38883f44) SHA1(fcf34b8c6e37774741542393b963635412484a27) )

	NEO_SFIX_128K( "241-s1.bin",  CRC(f3d32f0f) SHA1(2dc38b7dfd3ff14f64d5c0733c510b6bb8c692d0) )

	NEO_BIOS_SOUND_128K( "241-m1.bin", CRC(94520ebd) SHA1(f8a1551cebcb91e416f30f50581feed7f72899e9) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "241-v1.bin", 0x000000, 0x400000, CRC(99ec20e8) SHA1(80597707f1fe115eed1941bb0701fc00790ad504) )
	ROM_LOAD( "241-v2.bin", 0x400000, 0x400000, CRC(ecb16799) SHA1(b4b4ddc680836ed55942c66d7dfe756314e02211) )

	NO_DELTAT_REGION

	ROM_REGION( 0x2000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "241-c1.bin", 0x0000000, 0x800000, CRC(394b5e0d) SHA1(4549926f5054ee6aa7689cf920be0327e3908a50) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "241-c2.bin", 0x0000001, 0x800000, CRC(e5806221) SHA1(1e5475cfab129c77acc610f09369ca42ba5aafa5) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "241-c3.bin", 0x1000000, 0x800000, CRC(9f6bfa6f) SHA1(a4319b48004e723f81a980887678e3e296049a53) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "241-c4.bin", 0x1000001, 0x800000, CRC(7d3e306f) SHA1(1499316fb381775218d897b81a6a0c3465d1a37c) ) /* Plane 2,3 */
ROM_END

ROM_START( mslug2t )
	ROM_REGION( 0x300000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "941-p1.p1",  0x000000, 0x100000, CRC(df5d6fbc) SHA1(b9cc3e29afc12dc98daac9afb4f94e2cdd8b455c) )
	ROM_LOAD16_WORD_SWAP( "241-p2.bin", 0x100000, 0x200000, CRC(38883f44) SHA1(fcf34b8c6e37774741542393b963635412484a27) )

	NEO_SFIX_128K( "241-s1.bin",  CRC(f3d32f0f) SHA1(2dc38b7dfd3ff14f64d5c0733c510b6bb8c692d0) )

	NEO_BIOS_SOUND_128K( "241-m1.bin", CRC(94520ebd) SHA1(f8a1551cebcb91e416f30f50581feed7f72899e9) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "241-v1.bin", 0x000000, 0x400000, CRC(99ec20e8) SHA1(80597707f1fe115eed1941bb0701fc00790ad504) )
	ROM_LOAD( "241-v2.bin", 0x400000, 0x400000, CRC(ecb16799) SHA1(b4b4ddc680836ed55942c66d7dfe756314e02211) )

	NO_DELTAT_REGION

	ROM_REGION( 0x2000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "241-c1.bin", 0x0000000, 0x800000, CRC(394b5e0d) SHA1(4549926f5054ee6aa7689cf920be0327e3908a50) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "241-c2.bin", 0x0000001, 0x800000, CRC(e5806221) SHA1(1e5475cfab129c77acc610f09369ca42ba5aafa5) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "241-c3.bin", 0x1000000, 0x800000, CRC(9f6bfa6f) SHA1(a4319b48004e723f81a980887678e3e296049a53) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "241-c4.bin", 0x1000001, 0x800000, CRC(7d3e306f) SHA1(1499316fb381775218d897b81a6a0c3465d1a37c) ) /* Plane 2,3 */
ROM_END

ROM_START( kof98 ) /* encrypted code + protection */
	ROM_REGION( 0x600000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "yz98-p1.160", 0x000000, 0x200000, CRC(8893df89) SHA1(0452828785110601c65f667209fc2d2926cd3751) )
	ROM_LOAD16_WORD_SWAP( "242-p2.bin", 0x200000, 0x400000, CRC(980aba4c) SHA1(5e735929ec6c3ca5b2efae3c7de47bcbb8ade2c5) )

	NEO_SFIX_128K( "242-s1.bin", CRC(7f7b4805) SHA1(80ee6e5d0ece9c34ebca54b043a7cb33f9ff6b92) )

	NEO_BIOS_SOUND_256K( "242-m1a.bin", CRC(4ef7016b) SHA1(4182235e963bd70d398a79abeb54ab4d62887c48) )

	ROM_REGION( 0x1000000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "242-v1.bin", 0x000000, 0x400000, CRC(b9ea8051) SHA1(49606f64eb249263b3341b4f50cc1763c390b2af) )
	ROM_LOAD( "242-v2.bin", 0x400000, 0x400000, CRC(cc11106e) SHA1(d3108bc05c9bf041d4236b2fa0c66b013aa8db1b) )
	ROM_LOAD( "242-v3.bin", 0x800000, 0x400000, CRC(044ea4e1) SHA1(062a2f2e52098d73bc31c9ad66f5db8080395ce8) )
	ROM_LOAD( "242-v4.bin", 0xc00000, 0x400000, CRC(7985ea30) SHA1(54ed5f0324de6164ea81943ebccb3e8d298368ec) )

	NO_DELTAT_REGION

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "242-c1.bin", 0x0000000, 0x800000, CRC(e564ecd6) SHA1(78f22787a204f26bae9b2b1c945ddbc27143352f) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "242-c2.bin", 0x0000001, 0x800000, CRC(bd959b60) SHA1(2c97c59e77c9a3fe7d664e741d37944f3d56c10b) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "242-c3.bin", 0x1000000, 0x800000, CRC(22127b4f) SHA1(bd0d00f889d9da7c6ac48f287d9ed8c605ae22cf) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "242-c4.bin", 0x1000001, 0x800000, CRC(0b4fa044) SHA1(fa13c3764fae6b035a626601bc43629f1ebaaffd) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "242-c5.bin", 0x2000000, 0x800000, CRC(9d10bed3) SHA1(4d44addc7c808649bfb03ec45fb9529da413adff) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "242-c6.bin", 0x2000001, 0x800000, CRC(da07b6a2) SHA1(9c3f0da7cde1ffa8feca89efc88f07096e502acf) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "242-c7.bin", 0x3000000, 0x800000, CRC(f6d7a38a) SHA1(dd295d974dd4a7e5cb26a3ef3febcd03f28d522b) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "242-c8.bin", 0x3000001, 0x800000, CRC(c823e045) SHA1(886fbf64bcb58bc4eabb1fc9262f6ac9901a0f28) ) /* Plane 2,3 */
ROM_END

ROM_START( kof98k ) /* encrypted code + protection, only z80 rom is different to kof98 */
	ROM_REGION( 0x600000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "yz98-p1.160", 0x000000, 0x200000, CRC(8893df89) SHA1(0452828785110601c65f667209fc2d2926cd3751) )
	ROM_LOAD16_WORD_SWAP( "242-p2.bin", 0x200000, 0x400000, CRC(980aba4c) SHA1(5e735929ec6c3ca5b2efae3c7de47bcbb8ade2c5) )

	NEO_SFIX_128K( "242-s1.bin", CRC(7f7b4805) SHA1(80ee6e5d0ece9c34ebca54b043a7cb33f9ff6b92) )

	NEO_BIOS_SOUND_256K( "242-m1k.bin", CRC(ce12da0c) SHA1(e7c01dae2852d543d1a58d55735239f6a5aa05a5) )

	ROM_REGION( 0x1000000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "242-v1.bin", 0x000000, 0x400000, CRC(b9ea8051) SHA1(49606f64eb249263b3341b4f50cc1763c390b2af) )
	ROM_LOAD( "242-v2.bin", 0x400000, 0x400000, CRC(cc11106e) SHA1(d3108bc05c9bf041d4236b2fa0c66b013aa8db1b) )
	ROM_LOAD( "242-v3.bin", 0x800000, 0x400000, CRC(044ea4e1) SHA1(062a2f2e52098d73bc31c9ad66f5db8080395ce8) )
	ROM_LOAD( "242-v4.bin", 0xc00000, 0x400000, CRC(7985ea30) SHA1(54ed5f0324de6164ea81943ebccb3e8d298368ec) )

	NO_DELTAT_REGION

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "242-c1.bin", 0x0000000, 0x800000, CRC(e564ecd6) SHA1(78f22787a204f26bae9b2b1c945ddbc27143352f) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "242-c2.bin", 0x0000001, 0x800000, CRC(bd959b60) SHA1(2c97c59e77c9a3fe7d664e741d37944f3d56c10b) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "242-c3.bin", 0x1000000, 0x800000, CRC(22127b4f) SHA1(bd0d00f889d9da7c6ac48f287d9ed8c605ae22cf) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "242-c4.bin", 0x1000001, 0x800000, CRC(0b4fa044) SHA1(fa13c3764fae6b035a626601bc43629f1ebaaffd) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "242-c5.bin", 0x2000000, 0x800000, CRC(9d10bed3) SHA1(4d44addc7c808649bfb03ec45fb9529da413adff) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "242-c6.bin", 0x2000001, 0x800000, CRC(da07b6a2) SHA1(9c3f0da7cde1ffa8feca89efc88f07096e502acf) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "242-c7.bin", 0x3000000, 0x800000, CRC(f6d7a38a) SHA1(dd295d974dd4a7e5cb26a3ef3febcd03f28d522b) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "242-c8.bin", 0x3000001, 0x800000, CRC(c823e045) SHA1(886fbf64bcb58bc4eabb1fc9262f6ac9901a0f28) ) /* Plane 2,3 */
ROM_END

ROM_START( kof98n )
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "242-p1.bin", 0x000000, 0x100000, CRC(61ac868a) SHA1(26577264aa72d6af272952a876fcd3775f53e3fa) )
	ROM_LOAD16_WORD_SWAP( "242-p2.bin", 0x100000, 0x400000, CRC(980aba4c) SHA1(5e735929ec6c3ca5b2efae3c7de47bcbb8ade2c5) )

	NEO_SFIX_128K( "242-s1.bin", CRC(7f7b4805) SHA1(80ee6e5d0ece9c34ebca54b043a7cb33f9ff6b92) )

	NEO_BIOS_SOUND_256K( "242-m1.bin", CRC(4e7a6b1b) SHA1(b54d08f88713ed0271aa06f9f7c9c572ef555b1a) )

	ROM_REGION( 0x1000000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "242-v1.bin", 0x000000, 0x400000, CRC(b9ea8051) SHA1(49606f64eb249263b3341b4f50cc1763c390b2af) )
	ROM_LOAD( "242-v2.bin", 0x400000, 0x400000, CRC(cc11106e) SHA1(d3108bc05c9bf041d4236b2fa0c66b013aa8db1b) )
	ROM_LOAD( "242-v3.bin", 0x800000, 0x400000, CRC(044ea4e1) SHA1(062a2f2e52098d73bc31c9ad66f5db8080395ce8) )
	ROM_LOAD( "242-v4.bin", 0xc00000, 0x400000, CRC(7985ea30) SHA1(54ed5f0324de6164ea81943ebccb3e8d298368ec) )

	NO_DELTAT_REGION

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "242-c1.bin", 0x0000000, 0x800000, CRC(e564ecd6) SHA1(78f22787a204f26bae9b2b1c945ddbc27143352f) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "242-c2.bin", 0x0000001, 0x800000, CRC(bd959b60) SHA1(2c97c59e77c9a3fe7d664e741d37944f3d56c10b) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "242-c3.bin", 0x1000000, 0x800000, CRC(22127b4f) SHA1(bd0d00f889d9da7c6ac48f287d9ed8c605ae22cf) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "242-c4.bin", 0x1000001, 0x800000, CRC(0b4fa044) SHA1(fa13c3764fae6b035a626601bc43629f1ebaaffd) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "242-c5.bin", 0x2000000, 0x800000, CRC(9d10bed3) SHA1(4d44addc7c808649bfb03ec45fb9529da413adff) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "242-c6.bin", 0x2000001, 0x800000, CRC(da07b6a2) SHA1(9c3f0da7cde1ffa8feca89efc88f07096e502acf) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "242-c7.bin", 0x3000000, 0x800000, CRC(f6d7a38a) SHA1(dd295d974dd4a7e5cb26a3ef3febcd03f28d522b) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "242-c8.bin", 0x3000001, 0x800000, CRC(c823e045) SHA1(886fbf64bcb58bc4eabb1fc9262f6ac9901a0f28) ) /* Plane 2,3 */
ROM_END

ROM_START( lastbld2 )
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "243-p1.bin", 0x000000, 0x100000, CRC(af1e6554) SHA1(bd8526f60c2472937728a5d933fbd19d899f2cba) )
	ROM_LOAD16_WORD_SWAP( "243-p2.bin", 0x100000, 0x400000, CRC(add4a30b) SHA1(7db62564db49fe0218cbb35b119d62582a24d658) )

	NEO_SFIX_128K( "243-s1.bin", CRC(c9cd2298) SHA1(a9a18b5347f9dbe29a2ccb63fd4c8fd19537bf8b) )

	NEO_BIOS_SOUND_128K( "243-m1.bin", CRC(acf12d10) SHA1(6e6b98cc1fa44f24a5168877559b0055e6957b60) )

	ROM_REGION( 0x1000000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "243-v1.bin", 0x000000, 0x400000, CRC(f7ee6fbb) SHA1(55137bcabeeb590e40a9b8a7c07dd106e4d12a90) )
	ROM_LOAD( "243-v2.bin", 0x400000, 0x400000, CRC(aa9e4df6) SHA1(a0b91f63e2552a8ad9e0d1af00e2c38288637161) )
	ROM_LOAD( "243-v3.bin", 0x800000, 0x400000, CRC(4ac750b2) SHA1(585a154acc67bd84ea5b944686b78ed082b768d9) )
	ROM_LOAD( "243-v4.bin", 0xc00000, 0x400000, CRC(f5c64ba6) SHA1(2eac455def8c27090862cc042f65a3a8aad88283) )

	NO_DELTAT_REGION

	ROM_REGION( 0x3000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "243-c1.bin",  0x0000000, 0x800000, CRC(5839444d) SHA1(0616921c4cce20422563578bd0e806d359508599) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "243-c2.bin",  0x0000001, 0x800000, CRC(dd087428) SHA1(ca27fdb60425664956a18c021ea465f452fb1527) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "243-c3.bin",  0x1000000, 0x800000, CRC(6054cbe0) SHA1(ec2f65e9c930250ee25fd064ee5ae76a7a9c61d9) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "243-c4.bin",  0x1000001, 0x800000, CRC(8bd2a9d2) SHA1(0935df65cd2b0891a708bcc0f1c188148058d4b5) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "243-c5.bin",  0x2000000, 0x800000, CRC(6a503dcf) SHA1(23241b16d7e20f923d41186b29487ab922c7f530) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "243-c6.bin",  0x2000001, 0x800000, CRC(ec9c36d0) SHA1(e145e9e359000dda6e1dfe95a996bc6d29cfca21) ) /* Plane 2,3 */
ROM_END

ROM_START( neocup98 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "244-p1.bin", 0x100000, 0x100000, CRC(f8fdb7a5) SHA1(f34ee5d1c24e70427d05ef488f46906dbd9f9950) )
	ROM_CONTINUE(					   0x000000, 0x100000 )

	NEO_SFIX_128K( "244-s1.bin", CRC(9bddb697) SHA1(2f479bcd5a433201168792a578de3057252d649f) )

	NEO_BIOS_SOUND_128K( "244-m1.bin", CRC(a701b276) SHA1(055550ebc650835bcf8ea4457b2c91bd73e21281) )

	ROM_REGION( 0x600000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "244-v1.bin", 0x000000, 0x400000, CRC(79def46d) SHA1(63414235de2e177654508f1c840040424f8993e6) )
	ROM_LOAD( "244-v2.bin", 0x400000, 0x200000, CRC(b231902f) SHA1(9209772e947a2c7ac31b49dd613bf2eab0cb3358) )

	NO_DELTAT_REGION

	ROM_REGION( 0x1000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "244-c1.bin", 0x000000, 0x800000, CRC(d2c40ec7) SHA1(989d972aabcc7f190bdd5d861d3e13c09dd0803e) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "244-c2.bin", 0x000001, 0x800000, CRC(33aa0f35) SHA1(3443c7765c6aa177003d42bbfcac9f31d1e12575) ) /* Plane 2,3 */
ROM_END

ROM_START( breakrev )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "245-p1.bin", 0x100000, 0x100000, CRC(c828876d) SHA1(1dcba850e5cf8219d0945612cfded6d20ca8682a) )
	ROM_CONTINUE(					   0x000000, 0x100000 )

	NEO_SFIX_128K( "245-s1.bin", CRC(e7660a5d) SHA1(1cd54964ba60b245ea57d9daf0e27b572b815d21) )

	NEO_BIOS_SOUND_128K( "245-m1.bin", CRC(00f31c66) SHA1(8488598415c9b74bce00e05b31d96e3d1625c20d) )

	ROM_REGION(  0x800000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "245-v1.bin", 0x000000, 0x400000, CRC(e255446c) SHA1(b3933340d49d4ba581f3bf1af7ad69d786205790) )
	ROM_LOAD( "245-v2.bin", 0x400000, 0x400000, CRC(9068198a) SHA1(71819b0475a5e173a2f9a6e4ff19a94655141c3c) )

	NO_DELTAT_REGION

	ROM_REGION( 0x1400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "230-c1.bin", 0x0000000, 0x400000, CRC(68d4ae76) SHA1(2e820067f6963669f104bebf19e865fe4127b4dd) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "230-c2.bin", 0x0000001, 0x400000, CRC(fdee05cd) SHA1(efc4ffd790953ac7c25d5f045c64a9b49d24b096) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "230-c3.bin", 0x0800000, 0x400000, CRC(645077f3) SHA1(0ae74f3b4b3b88f128c6d8c0f35ffa53f5d67ef2) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "230-c4.bin", 0x0800001, 0x400000, CRC(63aeb74c) SHA1(9ff6930c0c3d79b46b86356e8565ce4fcd69ac38) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "245-c5.bin",  0x1000000, 0x200000, CRC(28ff1792) SHA1(0cc5c16ac42f52cee74f88235aef0671bee33b4c) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "245-c6.bin",  0x1000001, 0x200000, CRC(23c65644) SHA1(9dc74c3075cf0f738b6b41e5e8d89a74a6c9ef07) ) /* Plane 2,3 */
ROM_END

ROM_START( shocktr2 )
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "246-p1.bin", 0x000000, 0x100000, CRC(6d4b7781) SHA1(3c9d53d5da9842bfd45037c919064dda3fb2e089) )
	ROM_LOAD16_WORD_SWAP( "246-p2.bin", 0x100000, 0x400000, CRC(72ea04c3) SHA1(4fb1d22c30f5f3db4637dd92a4d2705c88de399d) )

	NEO_SFIX_128K( "246-s1.bin", CRC(2a360637) SHA1(431b43da5377dd189e51bd93d88d8a24d1b5090a) )

	NEO_BIOS_SOUND_128K( "246-m1.bin", CRC(d0604ad1) SHA1(fae3cd52a177eadd5f5775ace957cc0f8301e65d) )

	ROM_REGION( 0x1000000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "246-v1.bin", 0x000000, 0x400000, CRC(16986fc6) SHA1(cff3103dadf2f4390460456a5bd3fb5f28e21f6a) )
	ROM_LOAD( "246-v2.bin", 0x400000, 0x400000, CRC(ada41e83) SHA1(78e37ffaaa5679c8775a3a71f6df7a0d15082bdc) )
	ROM_LOAD( "246-v3.bin", 0x800000, 0x200000, CRC(a05ba5db) SHA1(09d739cad323d918f4196f91b654627fcafd8f4d) )

	NO_DELTAT_REGION

	ROM_REGION( 0x3000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "246-c1.bin", 0x0000000, 0x800000, CRC(47ac9ec5) SHA1(2d9eea11ba87baa23b18a1a3f607dc137846e807) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "246-c2.bin", 0x0000001, 0x800000, CRC(7bcab64f) SHA1(08d0edddd14b53d606e9a7a46aa4fb4e7398e0d0) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "246-c3.bin", 0x1000000, 0x800000, CRC(db2f73e8) SHA1(8d0c3473a8b2a4e28fed1b74beb2e025b7e61867) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "246-c4.bin", 0x1000001, 0x800000, CRC(5503854e) SHA1(a0f2e7c609cbb2aa43493a39d7dcaeca3d511d26) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "246-c5.bin", 0x2000000, 0x800000, CRC(055b3701) SHA1(97f5e92538d1f2e437dcb3f80e56e1230287e8d1) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "246-c6.bin", 0x2000001, 0x800000, CRC(7e2caae1) SHA1(d9de14e3e323664a8c5b7f1df1ba9ec7dd0e6a46) ) /* Plane 2,3 */
ROM_END

ROM_START( flipshot )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "247-p1.bin", 0x000000, 0x080000, CRC(d2e7a7e3) SHA1(1ff4f070fcc658bbc7dc69e16c87f82d7392d100) )

	NEO_SFIX_128K( "247-s1.bin", CRC(6300185c) SHA1(cb2f1de085fde214f96a962b1c2fa285eb387d44) )

	NEO_BIOS_SOUND_128K( "247-m1.bin", CRC(a9fe0144) SHA1(4cc076ecce9216a373f3dcd7ba28a03d6050e522) )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "247-v1.bin", 0x000000, 0x200000, CRC(42ec743d) SHA1(f45b5167ebcbd59300f4e5b05448cd421654102a) )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "247-c1.bin",  0x000000, 0x200000, CRC(c9eedcb2) SHA1(7627f2810322c146511525eb70b573a6a5ede926) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "247-c2.bin",  0x000001, 0x200000, CRC(7d6d6e87) SHA1(6475b58b9f91c20d1f465f3e892de0c68e12a92b) ) /* Plane 2,3 */
ROM_END

ROM_START( pbobbl2n )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "248-p1.bin", 0x000000, 0x100000, CRC(9d6c0754) SHA1(95c70c2d51fc4de01e768e03cc800a850aaad5dc) )

	NEO_SFIX_128K( "248-s1.bin", CRC(0a3fee41) SHA1(0ab2120e462086be942efcf6ffb37f58ea966ca3) )

	NEO_BIOS_SOUND_128K( "248-m1.bin", CRC(883097a9) SHA1(677bf9684c0c7977a9a3f0c1288e430040a53b49) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "248-v1.bin", 0x000000, 0x400000, CRC(57fde1fa) SHA1(af39bc141fc35b78dcacfd42b3abb29d7e5c2c89) )
	ROM_LOAD( "248-v2.bin", 0x400000, 0x400000, CRC(4b966ef3) SHA1(083c0e9fd7b8e506087648cdd8ec4206103984cd) )

	NO_DELTAT_REGION

	ROM_REGION( 0xa00000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "248-c1.bin", 0x000000, 0x400000, CRC(d9115327) SHA1(a49aa836a902326cfe785428e1699fefcf8566d4) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "248-c2.bin", 0x000001, 0x400000, CRC(77f9fdac) SHA1(4642d71d32b6a05dc8bfa0f95c936a77c7cef05e) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "248-c3.bin", 0x800000, 0x100000, CRC(8890bf7c) SHA1(a52f6bafd60e72003bfe38c80c1dde24b4983b2a) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "248-c4.bin", 0x800001, 0x100000, CRC(8efead3f) SHA1(f577d2f7c6f850b3d100c36947ad15e33dfa0bed) ) /* Plane 2,3 */
ROM_END

ROM_START( ctomaday )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "249-p1.bin", 0x100000, 0x100000, CRC(c9386118) SHA1(5554662c7bc8605889cac4a67fee05bbb4eb786f) )
	ROM_CONTINUE(					   0x000000, 0x100000 )

	NEO_SFIX_128K( "249-s1.bin", CRC(dc9eb372) SHA1(b8aa142243ba303799554479bfc88eb49260f3b1) )

	NEO_BIOS_SOUND_128K( "249-m1.bin", CRC(80328a47) SHA1(34b6b1a81eab1cf38834b2eea55454ce1b6100e2) )

	ROM_REGION( 0x500000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "249-v1.bin", 0x000000, 0x400000, CRC(de7c8f27) SHA1(3681a68a702ab5da8f509b8301d6cada75959332) )
	ROM_LOAD( "249-v2.bin", 0x400000, 0x100000, CRC(c8e40119) SHA1(738f525c381ed68c0b8a89318a3e4d0089473c45) )

	NO_DELTAT_REGION

	ROM_REGION( 0x800000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "249-c1.bin",  0x000000, 0x400000, CRC(041fb8ee) SHA1(dacc84d713d76818d89a26358374afaa22fa82a2) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "249-c2.bin",  0x000001, 0x400000, CRC(74f3cdf4) SHA1(55ddabaf77f4d575f4deb24fe63e4bdc2c6f31e1) ) /* Plane 2,3 */
ROM_END

ROM_START( mslugx )
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "250-p1.bin", 0x000000, 0x100000, CRC(81f1f60b) SHA1(4c19f2e9824e606178ac1c9d4b0516fbaa625035) )
	ROM_LOAD16_WORD_SWAP( "250-p2.bin", 0x100000, 0x400000, CRC(1fda2e12) SHA1(18aaa7a3ba8da99f78c430e9be69ccde04bc04d9) )

	NEO_SFIX_128K( "250-s1.bin",  CRC(fb6f441d) SHA1(2cc392ecde5d5afb28ddbaa1030552b48571dcfb) )

	NEO_BIOS_SOUND_128K( "250-m1.bin", CRC(fd42a842) SHA1(55769bad4860f64ef53a333e0da9e073db483d6a) )

	ROM_REGION( 0xa00000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "250-v1.bin", 0x000000, 0x400000, CRC(c79ede73) SHA1(ebfcc67204ff9677cf7972fd5b6b7faabf07280c) )
	ROM_LOAD( "250-v2.bin", 0x400000, 0x400000, CRC(ea9aabe1) SHA1(526c42ca9a388f7435569400e2f132e2724c71ff) )
	ROM_LOAD( "250-v3.bin", 0x800000, 0x200000, CRC(2ca65102) SHA1(45979d1edb1fc774a415d9386f98d7cb252a2043) )

	NO_DELTAT_REGION

	ROM_REGION( 0x3000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "250-c1.bin", 0x0000000, 0x800000, CRC(09a52c6f) SHA1(c3e8a8ccdac0f8bddc4c3413277626532405fae2) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "250-c2.bin", 0x0000001, 0x800000, CRC(31679821) SHA1(554f600a3aa09c16c13c625299b087a79d0d15c5) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "250-c3.bin", 0x1000000, 0x800000, CRC(fd602019) SHA1(c56646c62387bc1439d46610258c755beb8d7dd8) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "250-c4.bin", 0x1000001, 0x800000, CRC(31354513) SHA1(31be8ea2498001f68ce4b06b8b90acbf2dcab6af) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "250-c5.bin", 0x2000000, 0x800000, CRC(a4b56124) SHA1(d41069856df990a1a99d39fb263c8303389d5475) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "250-c6.bin", 0x2000001, 0x800000, CRC(83e3e69d) SHA1(39be66287696829d243fb71b3fb8b7dc2bc3298f) ) /* Plane 0,1 */
ROM_END

ROM_START( kof99 ) /* Original Version - Encrypted Code & GFX */
	ROM_REGION( 0x900000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "251-sma.kc",  0x0c0000, 0x040000, CRC(6c9d0647) SHA1(2a0ce62ca6c18007e8fbe1b60475c7874ab79389) )	/* stored in the custom chip */
	ROM_LOAD16_WORD_SWAP( "251-p1.bin",  0x100000, 0x400000, CRC(006e4532) SHA1(47791ab4044ad55988b1d3412d95b65b91a163c8) )
	ROM_LOAD16_WORD_SWAP( "251-pg2.bin", 0x500000, 0x400000, CRC(d9057f51) SHA1(8d365b4dd40351495df99d6c765df1434b0b0548) )

	/* The Encrypted Boards do _not_ have an s1 rom, data for it comes from the Cx ROMs */
	ROM_REGION( 0x20000, REGION_GFX1, 0 )
	ROM_FILL(                 0x000000, 0x20000, 0 )
	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "sfix.sfx",  0x000000, 0x20000, CRC(354029fc) SHA1(4ae4bf23b4c2acff875775d4cbff5583893ce2a1) )

	NEO_BIOS_SOUND_128K( "251-m1.bin", CRC(5e74539c) SHA1(6f49a9343cbd026b2c6720ff3fa2e5b1f85e80da) )

	ROM_REGION( 0x0e00000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "251-v1.bin", 0x000000, 0x400000, CRC(ef2eecc8) SHA1(8ed13b9db92dba3124bc5ba66e3e275885ece24a) )
	ROM_LOAD( "251-v2.bin", 0x400000, 0x400000, CRC(73e211ca) SHA1(0e60fa64cab6255d9721e2b4bc22e3de64c874c5) )
	ROM_LOAD( "251-v3.bin", 0x800000, 0x400000, CRC(821901da) SHA1(c6d4975bfaa19a62ed59126cadf2578c0a5c257f) )
	ROM_LOAD( "251-v4.bin", 0xc00000, 0x200000, CRC(b49e6178) SHA1(dde6f76e958841e8c99b693e13ced9aa9ef316dc) )

	NO_DELTAT_REGION

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	/* Encrypted */
	ROM_LOAD16_BYTE( "251-c1.bin",   0x0000000, 0x800000, CRC(0f9e93fe) SHA1(c7d77f0833c6f526f632e4f2dce59e302f6b9a15) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "251-c2.bin",   0x0000001, 0x800000, CRC(e71e2ea3) SHA1(39c7a326fddbcca3b29c68cdc96aad4d62295c0f) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "251-c3.bin",   0x1000000, 0x800000, CRC(238755d2) SHA1(01125b5c7a28e350f091280b041954fd1ac7c98f) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "251-c4.bin",   0x1000001, 0x800000, CRC(438c8b22) SHA1(ffbc643f5b27dd00f2f95d4ef4c5f29ee814722b) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "251-c5.bin",   0x2000000, 0x800000, CRC(0b0abd0a) SHA1(d5ad324fe523bdc6f09209d236cc4932524a48f1) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "251-c6.bin",   0x2000001, 0x800000, CRC(65bbf281) SHA1(79ae174667a23dabcfe865b6cd6133c86098452e) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "251-c7.bin",   0x3000000, 0x800000, CRC(ff65f62e) SHA1(7cd335fede05b56e15db90ce407c1183a78da4e9) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "251-c8.bin",   0x3000001, 0x800000, CRC(8d921c68) SHA1(42acf1d27d52a8e3b6262eb7df50693c0b135565) ) /* Plane 2,3 */
ROM_END

ROM_START( kof99a ) /* Original Version - Encrypted Code & GFX */
	ROM_REGION( 0x900000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "251-sma.ka",  0x0c0000, 0x040000, CRC(7766d09e) SHA1(4e0a49d1ad669a62676cb30f527c6590cde80194) )	/* stored in the custom chip */
	ROM_LOAD16_WORD_SWAP( "251-p1.bin",  0x100000, 0x400000, CRC(006e4532) SHA1(47791ab4044ad55988b1d3412d95b65b91a163c8) )
	ROM_LOAD16_WORD_SWAP( "251-p2.bin",  0x500000, 0x400000, CRC(90175f15) SHA1(aa9e75810438a8b45808a8bf32cb04d91b5c0b3a) )

	/* The Encrypted Boards do _not_ have an s1 rom, data for it comes from the Cx ROMs */
	ROM_REGION( 0x20000, REGION_GFX1, 0 )
	ROM_FILL(                 0x000000, 0x20000, 0 )
	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "sfix.sfx",  0x000000, 0x20000, CRC(354029fc) SHA1(4ae4bf23b4c2acff875775d4cbff5583893ce2a1) )

	NEO_BIOS_SOUND_128K( "251-m1.bin", CRC(5e74539c) SHA1(6f49a9343cbd026b2c6720ff3fa2e5b1f85e80da) )

	ROM_REGION( 0x0e00000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "251-v1.bin", 0x000000, 0x400000, CRC(ef2eecc8) SHA1(8ed13b9db92dba3124bc5ba66e3e275885ece24a) )
	ROM_LOAD( "251-v2.bin", 0x400000, 0x400000, CRC(73e211ca) SHA1(0e60fa64cab6255d9721e2b4bc22e3de64c874c5) )
	ROM_LOAD( "251-v3.bin", 0x800000, 0x400000, CRC(821901da) SHA1(c6d4975bfaa19a62ed59126cadf2578c0a5c257f) )
	ROM_LOAD( "251-v4.bin", 0xc00000, 0x200000, CRC(b49e6178) SHA1(dde6f76e958841e8c99b693e13ced9aa9ef316dc) )

	NO_DELTAT_REGION

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	/* Encrypted */
	ROM_LOAD16_BYTE( "251-c1.bin",   0x0000000, 0x800000, CRC(0f9e93fe) SHA1(c7d77f0833c6f526f632e4f2dce59e302f6b9a15) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "251-c2.bin",   0x0000001, 0x800000, CRC(e71e2ea3) SHA1(39c7a326fddbcca3b29c68cdc96aad4d62295c0f) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "251-c3.bin",   0x1000000, 0x800000, CRC(238755d2) SHA1(01125b5c7a28e350f091280b041954fd1ac7c98f) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "251-c4.bin",   0x1000001, 0x800000, CRC(438c8b22) SHA1(ffbc643f5b27dd00f2f95d4ef4c5f29ee814722b) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "251-c5.bin",   0x2000000, 0x800000, CRC(0b0abd0a) SHA1(d5ad324fe523bdc6f09209d236cc4932524a48f1) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "251-c6.bin",   0x2000001, 0x800000, CRC(65bbf281) SHA1(79ae174667a23dabcfe865b6cd6133c86098452e) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "251-c7.bin",   0x3000000, 0x800000, CRC(ff65f62e) SHA1(7cd335fede05b56e15db90ce407c1183a78da4e9) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "251-c8.bin",   0x3000001, 0x800000, CRC(8d921c68) SHA1(42acf1d27d52a8e3b6262eb7df50693c0b135565) ) /* Plane 2,3 */
ROM_END

ROM_START( kof99e ) /* Original Version - Encrypted Code & GFX */
	ROM_REGION( 0x900000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "251-sma.ka",  0x0c0000, 0x040000, CRC(7766d09e) SHA1(4e0a49d1ad669a62676cb30f527c6590cde80194) )	/* stored in the custom chip */
	ROM_LOAD16_WORD_SWAP( "251-ep1.p1",  0x100000, 0x200000, CRC(1e8d692d) SHA1(eea1aa8c0a17f089ac14831889c36535e559072c) )
	ROM_LOAD16_WORD_SWAP( "251-ep2.p2",  0x300000, 0x200000, CRC(d6206e5a) SHA1(0e1100d03c40c6d5cfa899d009e319ae73fce6b8) )
	ROM_LOAD16_WORD_SWAP( "251-ep3.p3",  0x500000, 0x200000, CRC(d58c3ef8) SHA1(f927d90d55b49944f448d6286e0cb913cc70ade1) )
	ROM_LOAD16_WORD_SWAP( "251-ep4.p4",  0x700000, 0x200000, CRC(52de02ae) SHA1(f16924ff8eef92da7716236a6a055e22e090a02b) )

	/* The Encrypted Boards do _not_ have an s1 rom, data for it comes from the Cx ROMs */
	ROM_REGION( 0x20000, REGION_GFX1, 0 )
	ROM_FILL(                 0x000000, 0x20000, 0 )
	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "sfix.sfx",  0x000000, 0x20000, CRC(354029fc) SHA1(4ae4bf23b4c2acff875775d4cbff5583893ce2a1) )

	NEO_BIOS_SOUND_128K( "251-m1.bin", CRC(5e74539c) SHA1(6f49a9343cbd026b2c6720ff3fa2e5b1f85e80da) )

	ROM_REGION( 0x0e00000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "251-v1.bin", 0x000000, 0x400000, CRC(ef2eecc8) SHA1(8ed13b9db92dba3124bc5ba66e3e275885ece24a) )
	ROM_LOAD( "251-v2.bin", 0x400000, 0x400000, CRC(73e211ca) SHA1(0e60fa64cab6255d9721e2b4bc22e3de64c874c5) )
	ROM_LOAD( "251-v3.bin", 0x800000, 0x400000, CRC(821901da) SHA1(c6d4975bfaa19a62ed59126cadf2578c0a5c257f) )
	ROM_LOAD( "251-v4.bin", 0xc00000, 0x200000, CRC(b49e6178) SHA1(dde6f76e958841e8c99b693e13ced9aa9ef316dc) )

	NO_DELTAT_REGION

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	/* Encrypted */
	ROM_LOAD16_BYTE( "251-c1.bin",   0x0000000, 0x800000, CRC(0f9e93fe) SHA1(c7d77f0833c6f526f632e4f2dce59e302f6b9a15) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "251-c2.bin",   0x0000001, 0x800000, CRC(e71e2ea3) SHA1(39c7a326fddbcca3b29c68cdc96aad4d62295c0f) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "251-c3.bin",   0x1000000, 0x800000, CRC(238755d2) SHA1(01125b5c7a28e350f091280b041954fd1ac7c98f) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "251-c4.bin",   0x1000001, 0x800000, CRC(438c8b22) SHA1(ffbc643f5b27dd00f2f95d4ef4c5f29ee814722b) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "251-c5.bin",   0x2000000, 0x800000, CRC(0b0abd0a) SHA1(d5ad324fe523bdc6f09209d236cc4932524a48f1) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "251-c6.bin",   0x2000001, 0x800000, CRC(65bbf281) SHA1(79ae174667a23dabcfe865b6cd6133c86098452e) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "251-c7.bin",   0x3000000, 0x800000, CRC(ff65f62e) SHA1(7cd335fede05b56e15db90ce407c1183a78da4e9) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "251-c8.bin",   0x3000001, 0x800000, CRC(8d921c68) SHA1(42acf1d27d52a8e3b6262eb7df50693c0b135565) ) /* Plane 2,3 */
ROM_END

ROM_START( kof99n ) /* Original Version - Encrypted GFX */
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "152-p1.bin", 0x000000, 0x100000, CRC(f2c7ddfa) SHA1(d592eecc53d442c55c2f26a6a721fdf2924d2a5b) )
	ROM_LOAD16_WORD_SWAP( "152-p2.bin", 0x100000, 0x400000, CRC(274ef47a) SHA1(98654b68cc85c19d4a90b46f3110f551fa2e5357) )

	/* The Encrypted Boards do _not_ have an s1 rom, data for it comes from the Cx ROMs */
	ROM_REGION( 0x20000, REGION_GFX1, 0 )
	ROM_FILL(                 0x000000, 0x20000, 0 )
	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "sfix.sfx",  0x000000, 0x20000, CRC(354029fc) SHA1(4ae4bf23b4c2acff875775d4cbff5583893ce2a1) )

	NEO_BIOS_SOUND_128K( "251-m1.bin", CRC(5e74539c) SHA1(6f49a9343cbd026b2c6720ff3fa2e5b1f85e80da) )

	ROM_REGION( 0x0e00000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "251-v1.bin", 0x000000, 0x400000, CRC(ef2eecc8) SHA1(8ed13b9db92dba3124bc5ba66e3e275885ece24a) )
	ROM_LOAD( "251-v2.bin", 0x400000, 0x400000, CRC(73e211ca) SHA1(0e60fa64cab6255d9721e2b4bc22e3de64c874c5) )
	ROM_LOAD( "251-v3.bin", 0x800000, 0x400000, CRC(821901da) SHA1(c6d4975bfaa19a62ed59126cadf2578c0a5c257f) )
	ROM_LOAD( "251-v4.bin", 0xc00000, 0x200000, CRC(b49e6178) SHA1(dde6f76e958841e8c99b693e13ced9aa9ef316dc) )

	NO_DELTAT_REGION

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	/* Encrypted */
	ROM_LOAD16_BYTE( "251-c1.bin",   0x0000000, 0x800000, CRC(0f9e93fe) SHA1(c7d77f0833c6f526f632e4f2dce59e302f6b9a15) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "251-c2.bin",   0x0000001, 0x800000, CRC(e71e2ea3) SHA1(39c7a326fddbcca3b29c68cdc96aad4d62295c0f) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "251-c3.bin",   0x1000000, 0x800000, CRC(238755d2) SHA1(01125b5c7a28e350f091280b041954fd1ac7c98f) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "251-c4.bin",   0x1000001, 0x800000, CRC(438c8b22) SHA1(ffbc643f5b27dd00f2f95d4ef4c5f29ee814722b) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "251-c5.bin",   0x2000000, 0x800000, CRC(0b0abd0a) SHA1(d5ad324fe523bdc6f09209d236cc4932524a48f1) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "251-c6.bin",   0x2000001, 0x800000, CRC(65bbf281) SHA1(79ae174667a23dabcfe865b6cd6133c86098452e) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "251-c7.bin",   0x3000000, 0x800000, CRC(ff65f62e) SHA1(7cd335fede05b56e15db90ce407c1183a78da4e9) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "251-c8.bin",   0x3000001, 0x800000, CRC(8d921c68) SHA1(42acf1d27d52a8e3b6262eb7df50693c0b135565) ) /* Plane 2,3 */
ROM_END

ROM_START( kof99p ) /* Prototype Version - Possibly Hacked */
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "251-p1p.bin", 0x000000, 0x100000, CRC(f37929c4) SHA1(226e7e3d629568399b88275e5bcd4e5b3839be95) )
	ROM_LOAD16_WORD_SWAP( "251-p2p.bin", 0x100000, 0x400000, CRC(739742ad) SHA1(31acaf05a9bf186305888d3db7e4e8a83f7bb0a4) )

	/* This is the S1 from the prototype, the final is certainly be different */
	NEO_SFIX_128K( "251-s1p.bin", CRC(fb1498ed) SHA1(d40060b31b6f217a4abdf3b336439fcd7bd7aaef) )

	/* Did the Prototype really use the same sound program / voice roms, sound isn't great .. */
	NEO_BIOS_SOUND_128K( "251-m1.bin", CRC(5e74539c) SHA1(6f49a9343cbd026b2c6720ff3fa2e5b1f85e80da) )

	ROM_REGION( 0x0e00000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "251-v1.bin", 0x000000, 0x400000, CRC(ef2eecc8) SHA1(8ed13b9db92dba3124bc5ba66e3e275885ece24a) )
	ROM_LOAD( "251-v2.bin", 0x400000, 0x400000, CRC(73e211ca) SHA1(0e60fa64cab6255d9721e2b4bc22e3de64c874c5) )
	ROM_LOAD( "251-v3.bin", 0x800000, 0x400000, CRC(821901da) SHA1(c6d4975bfaa19a62ed59126cadf2578c0a5c257f) )
	ROM_LOAD( "251-v4.bin", 0xc00000, 0x200000, CRC(b49e6178) SHA1(dde6f76e958841e8c99b693e13ced9aa9ef316dc) )

	NO_DELTAT_REGION

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	/* these are probably decrypted versions of the roms found in the final */
	ROM_LOAD16_BYTE( "251-c1p.bin", 0x0000000, 0x800000, CRC(e5d8ffa4) SHA1(65f15f9f02424a7a9dd35916166594f283e8d424) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "251-c2p.bin", 0x0000001, 0x800000, CRC(d822778f) SHA1(b590055e9bf1549bd6e1ecdabd65702202615712) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "251-c3p.bin", 0x1000000, 0x800000, CRC(f20959e8) SHA1(38293043fa77ac51c5e3191118874c58f1ae4d30) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "251-c4p.bin", 0x1000001, 0x800000, CRC(54ffbe9f) SHA1(8e62442923551f07a552621951b1accab2830e3b) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "251-c5p.bin", 0x2000000, 0x800000, CRC(d87a3bbc) SHA1(430f6812088712e0eb5714dcc664d8bba75e921a) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "251-c6p.bin", 0x2000001, 0x800000, CRC(4d40a691) SHA1(2b580d0678a5e6033ef16130671e860364d35e56) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "251-c7p.bin", 0x3000000, 0x800000, CRC(a4479a58) SHA1(d50e6cc9ccfe1ddbc6d90d46b8ca2cb0304edd8c) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "251-c8p.bin", 0x3000001, 0x800000, CRC(ead513ce) SHA1(e9b07a0b01fdeb3004755a479df059c81b4d0ed6) ) /* Plane 2,3 */
ROM_END

ROM_START( ganryu ) /* Original Version - Encrypted GFX */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "252-p1.bin", 0x100000, 0x100000, CRC(4b8ac4fb) SHA1(93d90271bff281862b03beba3809cf95a47a1e44) )
	ROM_CONTINUE(						0x000000, 0x100000 )

	/* The Encrypted Boards do _not_ have an s1 rom, data for it comes from the Cx ROMs */
	ROM_REGION( 0x20000, REGION_GFX1, 0 )
	ROM_FILL(                 0x000000, 0x20000, 0 )
	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "sfix.sfx",  0x000000, 0x20000, CRC(354029fc) SHA1(4ae4bf23b4c2acff875775d4cbff5583893ce2a1) )

	NEO_BIOS_SOUND_128K( "252-m1.bin", CRC(30cc4099) SHA1(46453b7aac41855a92724a785372f8daf931d8d4) )

	ROM_REGION( 0x0400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "252-v1.bin", 0x000000, 0x400000, CRC(e5946733) SHA1(d5904a50465af03d6ff33399a98f3259721ca0b2) )

	NO_DELTAT_REGION

	ROM_REGION( 0x1000000, REGION_GFX3, 0 )
	/* Encrypted */
	ROM_LOAD16_BYTE( "252-c1.bin", 0x0000000, 0x800000, CRC(50ee7882) SHA1(ace0f95407c246d0456341cf2ad8a7668b81df8a) )
	ROM_LOAD16_BYTE( "252-c2.bin", 0x0000001, 0x800000, CRC(62585474) SHA1(b35461598087aa82886af0030c61b26cc064af5f) )
ROM_END

ROM_START( garou )
	ROM_REGION( 0x900000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "253-sma.bin", 0x0c0000, 0x040000, CRC(98bc93dc) SHA1(01fe3d18b50f770e131e8d8eeff4c630ba8c9551) )	/* stored in the custom chip */
	ROM_LOAD16_WORD_SWAP( "253-ep1.p1",  0x100000, 0x200000, CRC(ea3171a4) SHA1(bbda40f652baa0dc5fc6a006c001a1bdb0df43f6) )
	ROM_LOAD16_WORD_SWAP( "253-ep2.p2",  0x300000, 0x200000, CRC(382f704b) SHA1(0ace9c84a8b8a0524fd9a503e7d872de1bf1bd52) )
	ROM_LOAD16_WORD_SWAP( "253-ep3.p3",  0x500000, 0x200000, CRC(e395bfdd) SHA1(6b50f5ac15bf66b7e4e9bff57594fd3d7530c831) )
	ROM_LOAD16_WORD_SWAP( "253-ep4.p4",  0x700000, 0x200000, CRC(da92c08e) SHA1(5556f983ebcebc33160e90a6a6cf589d54c8cedc) )

	/* The Encrypted Boards do _not_ have an s1 rom, data for it comes from the Cx ROMs */
	ROM_REGION( 0x80000, REGION_GFX1, 0 )	/* larger char set */
	ROM_FILL(                 0x000000, 0x20000, 0 )
	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "sfix.sfx",  0x000000, 0x20000, CRC(354029fc) SHA1(4ae4bf23b4c2acff875775d4cbff5583893ce2a1) )

	NEO_BIOS_SOUND_256K( "253-m1.bin", CRC(36a806be) SHA1(90fb44dc0c3fb57946a0f35716056abb84a0f191) )

	ROM_REGION( 0x1000000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "253-v1.bin", 0x000000, 0x400000, CRC(263e388c) SHA1(11f05feee170370c4bfc5053af79246a6e3de5dc) )
	ROM_LOAD( "253-v2.bin", 0x400000, 0x400000, CRC(2c6bc7be) SHA1(c9c61054ce1a47bf1bf77a31117726b499df24a4) )
	ROM_LOAD( "253-v3.bin", 0x800000, 0x400000, CRC(0425b27d) SHA1(986863c98fc3445487242dcf2ea75b075e7f33ee) )
	ROM_LOAD( "253-v4.bin", 0xc00000, 0x400000, CRC(a54be8a9) SHA1(d7123e79b43e8adfaa5ecadbfcbeb6be890ec311) )

	NO_DELTAT_REGION

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	/* Encrypted */
	ROM_LOAD16_BYTE( "253-c1.bin", 0x0000000, 0x800000, CRC(0603e046) SHA1(5ef4557ce90ba65d36129de97be1fdc049c4a3d0) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "253-c2.bin", 0x0000001, 0x800000, CRC(0917d2a4) SHA1(d4ed3a13ae22f880fb399671c1752f1a0283f316) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "253-c3.bin", 0x1000000, 0x800000, CRC(6737c92d) SHA1(678f0c9cc1267bd131546981b9989bfb7289d8ba) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "253-c4.bin", 0x1000001, 0x800000, CRC(5ba92ec6) SHA1(aae36b050a3a0321026a96eba06dd184c0e2acca) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "253-c5.bin", 0x2000000, 0x800000, CRC(3eab5557) SHA1(47c433015aa81a0b0a1d3ee51382c4948b80c023) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "253-c6.bin", 0x2000001, 0x800000, CRC(308d098b) SHA1(b052f1fa9fbc69606004c250e2505360eaa24949) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "253-c7.bin", 0x3000000, 0x800000, CRC(c0e995ae) SHA1(8675ca787d28246174c313167f82557f021366fc) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "253-c8.bin", 0x3000001, 0x800000, CRC(21a11303) SHA1(fd61221ad257c185ef5c1f9694bd6b840b591af3) ) /* Plane 2,3 */
ROM_END

ROM_START( garouo )
	ROM_REGION( 0x900000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "253-smao.bin", 0x0c0000, 0x040000, CRC(96c72233) SHA1(29e19effd40fdf7e5144332396857f4ad0eff13e) )	/* stored in the custom chip */
	ROM_LOAD16_WORD_SWAP( "253-p1.bin",   0x100000, 0x400000, CRC(18ae5d7e) SHA1(bdb58ec9137d8653979b47132f2d10e1cc6aaa24) )
	ROM_LOAD16_WORD_SWAP( "253-p2.bin",   0x500000, 0x400000, CRC(afffa779) SHA1(ac017986f02277fbcd656b8c02492a3f4216a90e) )

	/* The Encrypted Boards do _not_ have an s1 rom, data for it comes from the Cx ROMs */
	ROM_REGION( 0x80000, REGION_GFX1, 0 )	/* larger char set */
	ROM_FILL(                 0x000000, 0x20000, 0 )
	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "sfix.sfx",  0x000000, 0x20000, CRC(354029fc) SHA1(4ae4bf23b4c2acff875775d4cbff5583893ce2a1) )

	NEO_BIOS_SOUND_256K( "253-m1.bin", CRC(36a806be) SHA1(90fb44dc0c3fb57946a0f35716056abb84a0f191) )

	ROM_REGION( 0x1000000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "253-v1.bin", 0x000000, 0x400000, CRC(263e388c) SHA1(11f05feee170370c4bfc5053af79246a6e3de5dc) )
	ROM_LOAD( "253-v2.bin", 0x400000, 0x400000, CRC(2c6bc7be) SHA1(c9c61054ce1a47bf1bf77a31117726b499df24a4) )
	ROM_LOAD( "253-v3.bin", 0x800000, 0x400000, CRC(0425b27d) SHA1(986863c98fc3445487242dcf2ea75b075e7f33ee) )
	ROM_LOAD( "253-v4.bin", 0xc00000, 0x400000, CRC(a54be8a9) SHA1(d7123e79b43e8adfaa5ecadbfcbeb6be890ec311) )

	NO_DELTAT_REGION

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	/* Encrypted */
	ROM_LOAD16_BYTE( "253-c1.bin", 0x0000000, 0x800000, CRC(0603e046) SHA1(5ef4557ce90ba65d36129de97be1fdc049c4a3d0) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "253-c2.bin", 0x0000001, 0x800000, CRC(0917d2a4) SHA1(d4ed3a13ae22f880fb399671c1752f1a0283f316) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "253-c3.bin", 0x1000000, 0x800000, CRC(6737c92d) SHA1(678f0c9cc1267bd131546981b9989bfb7289d8ba) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "253-c4.bin", 0x1000001, 0x800000, CRC(5ba92ec6) SHA1(aae36b050a3a0321026a96eba06dd184c0e2acca) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "253-c5.bin", 0x2000000, 0x800000, CRC(3eab5557) SHA1(47c433015aa81a0b0a1d3ee51382c4948b80c023) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "253-c6.bin", 0x2000001, 0x800000, CRC(308d098b) SHA1(b052f1fa9fbc69606004c250e2505360eaa24949) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "253-c7.bin", 0x3000000, 0x800000, CRC(c0e995ae) SHA1(8675ca787d28246174c313167f82557f021366fc) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "253-c8.bin", 0x3000001, 0x800000, CRC(21a11303) SHA1(fd61221ad257c185ef5c1f9694bd6b840b591af3) ) /* Plane 2,3 */
ROM_END

ROM_START( garoup ) /* Prototype Version, seems genuine */
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "253-p1p.bin", 0x000000, 0x100000, CRC(c72f0c16) SHA1(1ff6bb651682f93bef9ff02622c3cf63fe594986) )
	ROM_LOAD16_WORD_SWAP( "253-p2p.bin", 0x100000, 0x400000, CRC(bf8de565) SHA1(0e24574168cd38138bed0aa4dca49849f6901ca2) )

	NEO_SFIX_128K( "253-s1p.bin", CRC(779989de) SHA1(8bd550857b60f8a907f6d39a4225ceffdd330307) )

	NEO_BIOS_SOUND_256K( "253-m1p.bin", CRC(bbe464f7) SHA1(f5f8f3e48f5d453f45107085d6f4023bcd24c053) )

	ROM_REGION( 0x1000000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "253-v1p.bin", 0x000000, 0x400000, CRC(274f3923) SHA1(4c7a8ad1cd0e3afc1f78de3c2929120ed434f104) )
	ROM_LOAD( "253-v2p.bin", 0x400000, 0x400000, CRC(8f86dabe) SHA1(b3d2d9f5c1d97a6e7aee2c674fb6627f41bbb240) )
	ROM_LOAD( "253-v3p.bin", 0x800000, 0x400000, CRC(05fd06cd) SHA1(6cd699719614bb87547632ea3d61d92d81fdf563) )
	ROM_LOAD( "253-v4p.bin", 0xc00000, 0x400000, CRC(14984063) SHA1(170d5638327ec0eb3590b80dc11590897367250c) )

	NO_DELTAT_REGION

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "253-c1p.bin", 0x0000000, 0x800000, CRC(5bb5d137) SHA1(d648febd8e6a0bdd9bdbb6ce1f1f8b08567ec05a) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "253-c2p.bin", 0x0000001, 0x800000, CRC(5c8d2960) SHA1(f7503502be0332adf408ee0ea5ee5161c8939fd8) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "253-c3p.bin", 0x1000000, 0x800000, CRC(234d16fc) SHA1(7b9221f7ecc438150c8a10be72390329854ed21b) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "253-c4p.bin", 0x1000001, 0x800000, CRC(b9b5b993) SHA1(6059793eaf6e58c172235fe64aa9d25a40c38ed6) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "253-c5p.bin", 0x2000000, 0x800000, CRC(722615d2) SHA1(798832c535869f0e247c3db0d8253779b103e213) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "253-c6p.bin", 0x2000001, 0x800000, CRC(0a6fab38) SHA1(eaee6f2f18af91f7959d84d4b991b3fc182d07c4) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "253-c7p.bin", 0x3000000, 0x800000, CRC(d68e806f) SHA1(92bfd9839115bd590972ae4ecc45ad35dce22387) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "253-c8p.bin", 0x3000001, 0x800000, CRC(f778fe99) SHA1(c963f6ba90a36d02991728b44ffcf174ca18268a) ) /* Plane 2,3 */
ROM_END

ROM_START( s1945p ) /* Original Version, Encrypted GFX Roms */
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "254-p1.bin", 0x000000, 0x100000, CRC(ff8efcff) SHA1(dcaeaca573385c172ecc43ee6bee355359091893) )
	ROM_LOAD16_WORD_SWAP( "254-p2.bin", 0x100000, 0x400000, CRC(efdfd4dd) SHA1(254f3e1b546eed788f7ae919be9d1bf9702148ce) )

	/* The Encrypted Boards do _not_ have an s1 rom, data for it comes from the Cx ROMs */
	ROM_REGION( 0x20000, REGION_GFX1, 0 )
	ROM_FILL(                 0x000000, 0x20000, 0 )
	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "sfix.sfx",  0x000000, 0x20000, CRC(354029fc) SHA1(4ae4bf23b4c2acff875775d4cbff5583893ce2a1) )

	NEO_BIOS_SOUND_128K( "254-m1.bin", CRC(994b4487) SHA1(a4e645a3ababa48a8325980ff022e04a8b51b017) )

	ROM_REGION( 0x1000000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "254-v1.bin", 0x000000, 0x400000, CRC(844f58fb) SHA1(e59544457be9f21481eac8b5a39b9cbb502f252d) )
	ROM_LOAD( "254-v2.bin", 0x400000, 0x400000, CRC(d9a248f0) SHA1(dd3e0974b753e6f94d0943a002de45668a1b072b) )
	ROM_LOAD( "254-v3.bin", 0x800000, 0x400000, CRC(0b0d2d33) SHA1(f8e76af42a997f36a40f66b39de00f68afe6a89c) )
	ROM_LOAD( "254-v4.bin", 0xc00000, 0x400000, CRC(6d13dc91) SHA1(8433513c0b5aea61939068a25ab90efbe3e44116) )

	NO_DELTAT_REGION

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "254-c1.bin", 0x0000000, 0x800000, CRC(ae6fc8ef) SHA1(544ccdaee8a4a45cdce9483e30852811d2d5f3cc) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "254-c2.bin", 0x0000001, 0x800000, CRC(436fa176) SHA1(d70141a91a360a1b1070753086f976608fec38af) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "254-c3.bin", 0x1000000, 0x800000, CRC(e53ff2dc) SHA1(31f6aaffe28146d574aa72f14f90a9d968f36bc6) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "254-c4.bin", 0x1000001, 0x800000, CRC(818672f0) SHA1(460c6738d0ee5ae440a23fc1434fab53bbb242b5) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "254-c5.bin", 0x2000000, 0x800000, CRC(4580eacd) SHA1(feb96eb5e80c9125ddd7836e0939212cd3011c34) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "254-c6.bin", 0x2000001, 0x800000, CRC(e34970fc) SHA1(6e43e15e27bc914357f977116ab1e2d98711bb21) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "254-c7.bin", 0x3000000, 0x800000, CRC(f2323239) SHA1(5b3e8dd77474203be010ec7363858d806344a320) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "254-c8.bin", 0x3000001, 0x800000, CRC(66848c7d) SHA1(24d4ed627940a4cf8129761c1da15556e52e199c) ) /* Plane 2,3 */
ROM_END

ROM_START( preisle2 ) /* Original Version, Encrypted GFX */
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "255-p1.bin", 0x000000, 0x100000, CRC(dfa3c0f3) SHA1(793c6a46f3a794536dc0327a3f3fad20e25ab661) )
	ROM_LOAD16_WORD_SWAP( "255-p2.bin", 0x100000, 0x400000, CRC(42050b80) SHA1(0981a8295d43b264c2b95e5d7568bdda4e64c976) )

	/* The Encrypted Boards do _not_ have an s1 rom, data for it comes from the Cx ROMs */
	ROM_REGION( 0x20000, REGION_GFX1, 0 )
	ROM_FILL(                 0x000000, 0x20000, 0 )
	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "sfix.sfx",  0x000000, 0x20000, CRC(354029fc) SHA1(4ae4bf23b4c2acff875775d4cbff5583893ce2a1) )

	NEO_BIOS_SOUND_128K( "255-m1.bin", CRC(8efd4014) SHA1(5b73809b6e4e49264d281ef3e5004ac8a9de296d) )

	ROM_REGION( 0x0600000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "255-v1.bin", 0x000000, 0x400000, CRC(5a14543d) SHA1(7146ac748f846c7e2d5b0bdcf953892e39b648fe) )
	ROM_LOAD( "255-v2.bin", 0x400000, 0x200000, CRC(6610d91a) SHA1(b2c6786920dc1712e88c3cc26d2c6c3ac2615bf4) )

	NO_DELTAT_REGION

	ROM_REGION( 0x3000000, REGION_GFX3, 0 )
	/* Encrypted */
	ROM_LOAD16_BYTE( "255-c1.bin",   0x0000000, 0x800000, CRC(ea06000b) SHA1(1539b12e461fa48301190eb8171bbffff9d984b7) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "255-c2.bin",   0x0000001, 0x800000, CRC(04e67d79) SHA1(aadb6ee750da2c14c6eededa2218db95e051a32c) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "255-c3.bin",   0x1000000, 0x800000, CRC(60e31e08) SHA1(bd5b81ad9d04cdc4e0df31ac40eca305f98277eb) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "255-c4.bin",   0x1000001, 0x800000, CRC(40371d69) SHA1(90011ccc5672ff1b90737cf50c963e71b6217ce3) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "255-c5.bin",   0x2000000, 0x800000, CRC(0b2e6adf) SHA1(15c7d9aa8b1ad9a071e6fd0ef0de8a057c23b02e) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "255-c6.bin",   0x2000001, 0x800000, CRC(b001bdd3) SHA1(394ba8004644844ee97a120cfda48aeac685af8a) ) /* Plane 2,3 */
ROM_END

ROM_START( mslug3 ) /* Original Version - Encrypted Code & GFX */
	ROM_REGION( 0x900000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "256-sma.bin", 0x0c0000, 0x040000, CRC(9cd55736) SHA1(d6efb2b313127c2911d47d9324626b3f1e7c6ccb) )	/* stored in the custom chip */
	ROM_LOAD16_WORD_SWAP( "256-p1.bin",  0x100000, 0x400000, CRC(b07edfd5) SHA1(dcbd9e500bfae98d754e55cdbbbbf9401013f8ee) )
	ROM_LOAD16_WORD_SWAP( "256-p2.bin",  0x500000, 0x400000, CRC(6097c26b) SHA1(248ec29d21216f29dc6f5f3f0e1ad1601b3501b6) )

	/* The Encrypted Boards do _not_ have an s1 rom, data for it comes from the Cx ROMs */
	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* larger char set */
	ROM_FILL(                 0x000000, 0x20000, 0 )
	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "sfix.sfx",  0x000000, 0x20000, CRC(354029fc) SHA1(4ae4bf23b4c2acff875775d4cbff5583893ce2a1) )

	NEO_BIOS_SOUND_512K( "256-m1.bin", CRC(eaeec116) SHA1(54419dbb21edc8c4b37eaac2e7ad9496d2de037a) )

	ROM_REGION( 0x1000000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "256-v1.bin", 0x000000, 0x400000, CRC(f2690241) SHA1(fd56babc1934d10e0d27c32f032f9edda7ca8ce9) )
	ROM_LOAD( "256-v2.bin", 0x400000, 0x400000, CRC(7e2a10bd) SHA1(0d587fb9f64cba0315ce2d8a03e2b8fe34936dff) )
	ROM_LOAD( "256-v3.bin", 0x800000, 0x400000, CRC(0eaec17c) SHA1(c3ed613cc6993edd6fc0d62a90bcd85de8e21915) )
	ROM_LOAD( "256-v4.bin", 0xc00000, 0x400000, CRC(9b4b22d4) SHA1(9764fbf8453e52f80aa97a46fb9cf5937ef15a31) )

	NO_DELTAT_REGION

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	/* Encrypted */
	ROM_LOAD16_BYTE( "256-c1.bin",   0x0000000, 0x800000, CRC(5a79c34e) SHA1(b8aa51fa50935cae62ab3d125b723ab888691e60) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "256-c2.bin",   0x0000001, 0x800000, CRC(944c362c) SHA1(3843ab300f956280475469caee70135658f67089) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "256-c3.bin",   0x1000000, 0x800000, CRC(6e69d36f) SHA1(94e8cf42e999114b4bd8b30e0aa2f365578c4c9a) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "256-c4.bin",   0x1000001, 0x800000, CRC(b755b4eb) SHA1(804700a0966a48f130c434ede3f970792ea74fa5) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "256-c5.bin",   0x2000000, 0x800000, CRC(7aacab47) SHA1(312c1c9846175fe1a3cad51d5ae230cf674fc93d) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "256-c6.bin",   0x2000001, 0x800000, CRC(c698fd5d) SHA1(16818883b06849ba2f8d61bdd5e21aaf99bd8408) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "256-c7.bin",   0x3000000, 0x800000, CRC(cfceddd2) SHA1(7def666adf8bd1703f40c61f182fc040b6362dc9) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "256-c8.bin",   0x3000001, 0x800000, CRC(4d9be34c) SHA1(a737bdfa2b815aea7067e7af2636e83a9409c414) ) /* Plane 2,3 */
ROM_END

ROM_START( mslug3n ) /* Original Version - Encrypted GFX */
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "256-ph1.rom",  0x000000, 0x100000, CRC(9c42ca85) SHA1(7a8f77a89867b889295ae9b9dfd4ba28f02d234d) )
	ROM_LOAD16_WORD_SWAP( "256-ph2.rom",  0x100000, 0x400000, CRC(1f3d8ce8) SHA1(08b05a8abfb86ec09a5e758d6273acf1489961f9) )

	/* The Encrypted Boards do _not_ have an s1 rom, data for it comes from the Cx ROMs */
	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* larger char set */
	ROM_FILL(                 0x000000, 0x20000, 0 )
	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "sfix.sfx",  0x000000, 0x20000, CRC(354029fc) SHA1(4ae4bf23b4c2acff875775d4cbff5583893ce2a1) )

	NEO_BIOS_SOUND_512K( "256-m1.bin", CRC(eaeec116) SHA1(54419dbb21edc8c4b37eaac2e7ad9496d2de037a) )

	ROM_REGION( 0x1000000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "256-v1.bin", 0x000000, 0x400000, CRC(f2690241) SHA1(fd56babc1934d10e0d27c32f032f9edda7ca8ce9) )
	ROM_LOAD( "256-v2.bin", 0x400000, 0x400000, CRC(7e2a10bd) SHA1(0d587fb9f64cba0315ce2d8a03e2b8fe34936dff) )
	ROM_LOAD( "256-v3.bin", 0x800000, 0x400000, CRC(0eaec17c) SHA1(c3ed613cc6993edd6fc0d62a90bcd85de8e21915) )
	ROM_LOAD( "256-v4.bin", 0xc00000, 0x400000, CRC(9b4b22d4) SHA1(9764fbf8453e52f80aa97a46fb9cf5937ef15a31) )

	NO_DELTAT_REGION

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	/* Encrypted */
	ROM_LOAD16_BYTE( "256-c1.bin",   0x0000000, 0x800000, CRC(5a79c34e) SHA1(b8aa51fa50935cae62ab3d125b723ab888691e60) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "256-c2.bin",   0x0000001, 0x800000, CRC(944c362c) SHA1(3843ab300f956280475469caee70135658f67089) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "256-c3.bin",   0x1000000, 0x800000, CRC(6e69d36f) SHA1(94e8cf42e999114b4bd8b30e0aa2f365578c4c9a) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "256-c4.bin",   0x1000001, 0x800000, CRC(b755b4eb) SHA1(804700a0966a48f130c434ede3f970792ea74fa5) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "256-c5.bin",   0x2000000, 0x800000, CRC(7aacab47) SHA1(312c1c9846175fe1a3cad51d5ae230cf674fc93d) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "256-c6.bin",   0x2000001, 0x800000, CRC(c698fd5d) SHA1(16818883b06849ba2f8d61bdd5e21aaf99bd8408) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "256-c7.bin",   0x3000000, 0x800000, CRC(cfceddd2) SHA1(7def666adf8bd1703f40c61f182fc040b6362dc9) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "256-c8.bin",   0x3000001, 0x800000, CRC(4d9be34c) SHA1(a737bdfa2b815aea7067e7af2636e83a9409c414) ) /* Plane 2,3 */
ROM_END

ROM_START( kof2000 ) /* Original Version, Encrypted Code + Sound + GFX Roms */
	ROM_REGION( 0x900000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "257-sma.bin", 0x0c0000, 0x040000, CRC(71c6e6bb) SHA1(1bd29ded4c6b29780db8e8b772c452189699ca89) )	/* stored in the custom chip */
	ROM_LOAD16_WORD_SWAP( "257-p1.bin",  0x100000, 0x400000, CRC(60947b4c) SHA1(5faa0a7ac7734d6c8e276589bd12dd574264647d) )
	ROM_LOAD16_WORD_SWAP( "257-p2.bin",  0x500000, 0x400000, CRC(1b7ec415) SHA1(f19fa44e9ee5b5a6eb4a051349d6bc4acc3bbbdb) )

	/* The Encrypted Boards do _not_ have an s1 rom, data for it comes from the Cx ROMs */
	ROM_REGION( 0x80000, REGION_GFX1, 0 )	/* larger char set */
	ROM_FILL(                 0x000000, 0x20000, 0 )
	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "sfix.sfx",  0x000000, 0x20000, CRC(354029fc) SHA1(4ae4bf23b4c2acff875775d4cbff5583893ce2a1) )

	NEO_BIOS_AUDIO_ENCRYPTED_256K( "257-m1.bin", CRC(4b749113) SHA1(2af2361146edd0ce3966614d90165a5c1afb8de4) )

	ROM_REGION( 0x1000000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "257-v1.bin", 0x000000, 0x400000, CRC(17cde847) SHA1(4bcc0205b70dc6d9216b29025450c9c5b08cb65d) )
	ROM_LOAD( "257-v2.bin", 0x400000, 0x400000, CRC(1afb20ff) SHA1(57dfd2de058139345ff2b744a225790baaecd5a2) )
	ROM_LOAD( "257-v3.bin", 0x800000, 0x400000, CRC(4605036a) SHA1(51b228a0600d38a6ec37aec4822879ec3b0ee106) )
	ROM_LOAD( "257-v4.bin", 0xc00000, 0x400000, CRC(764bbd6b) SHA1(df23c09ca6cf7d0ae5e11ff16e30c159725106b3) )

	NO_DELTAT_REGION

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "257-c1.bin",   0x0000000, 0x800000, CRC(cef1cdfa) SHA1(6135080f3a6b4712b76cc217edcc58e72b55c2b9) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "257-c2.bin",   0x0000001, 0x800000, CRC(f7bf0003) SHA1(9f7b19a2100cf7d12867e742f440dd5277b4f895) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "257-c3.bin",   0x1000000, 0x800000, CRC(101e6560) SHA1(8073ae1139e215d1167f8d32c14079a46ce3ee1c) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "257-c4.bin",   0x1000001, 0x800000, CRC(bd2fc1b1) SHA1(da0006761923ad49b404a08d7a151193ee307a69) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "257-c5.bin",   0x2000000, 0x800000, CRC(89775412) SHA1(b221b30224bc4239f1b3c2d2fd1cd4fa84e3523c) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "257-c6.bin",   0x2000001, 0x800000, CRC(fa7200d5) SHA1(6f2b0d38af34e280d56a58955400e5c679906871) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "257-c7.bin",   0x3000000, 0x800000, CRC(7da11fe4) SHA1(065336cf166807acb6c8569d59d3bf37a19b0a42) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "257-c8.bin",   0x3000001, 0x800000, CRC(b1afa60b) SHA1(b916184f5cfe4121752270f4f65abf35d8eb0519) ) /* Plane 2,3 */
ROM_END

ROM_START( kof2000n ) /* Original Version, Encrypted Sound + GFX Roms */
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "257-p1n.bin",  0x000000, 0x100000, CRC(5f809dbe) SHA1(2bc233dcff5622de86d01e3b74b840c7caf12982) )
	ROM_LOAD16_WORD_SWAP( "257-p2n.bin",  0x100000, 0x400000, CRC(693c2c5e) SHA1(dc9121b7369ef46596343cac055a00aec81704d4) )

	/* The Encrypted Boards do _not_ have an s1 rom, data for it comes from the Cx ROMs */
	ROM_REGION( 0x80000, REGION_GFX1, 0 )	/* larger char set */
	ROM_FILL(                 0x000000, 0x20000, 0 )
	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "sfix.sfx",  0x000000, 0x20000, CRC(354029fc) SHA1(4ae4bf23b4c2acff875775d4cbff5583893ce2a1) )

	/* Encrypted */
	NEO_BIOS_AUDIO_ENCRYPTED_256K( "257-m1.bin", CRC(4b749113) SHA1(2af2361146edd0ce3966614d90165a5c1afb8de4) )

	ROM_REGION( 0x1000000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "257-v1.bin", 0x000000, 0x400000, CRC(17cde847) SHA1(4bcc0205b70dc6d9216b29025450c9c5b08cb65d) )
	ROM_LOAD( "257-v2.bin", 0x400000, 0x400000, CRC(1afb20ff) SHA1(57dfd2de058139345ff2b744a225790baaecd5a2) )
	ROM_LOAD( "257-v3.bin", 0x800000, 0x400000, CRC(4605036a) SHA1(51b228a0600d38a6ec37aec4822879ec3b0ee106) )
	ROM_LOAD( "257-v4.bin", 0xc00000, 0x400000, CRC(764bbd6b) SHA1(df23c09ca6cf7d0ae5e11ff16e30c159725106b3) )

	NO_DELTAT_REGION

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "257-c1.bin",   0x0000000, 0x800000, CRC(cef1cdfa) SHA1(6135080f3a6b4712b76cc217edcc58e72b55c2b9) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "257-c2.bin",   0x0000001, 0x800000, CRC(f7bf0003) SHA1(9f7b19a2100cf7d12867e742f440dd5277b4f895) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "257-c3.bin",   0x1000000, 0x800000, CRC(101e6560) SHA1(8073ae1139e215d1167f8d32c14079a46ce3ee1c) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "257-c4.bin",   0x1000001, 0x800000, CRC(bd2fc1b1) SHA1(da0006761923ad49b404a08d7a151193ee307a69) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "257-c5.bin",   0x2000000, 0x800000, CRC(89775412) SHA1(b221b30224bc4239f1b3c2d2fd1cd4fa84e3523c) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "257-c6.bin",   0x2000001, 0x800000, CRC(fa7200d5) SHA1(6f2b0d38af34e280d56a58955400e5c679906871) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "257-c7.bin",   0x3000000, 0x800000, CRC(7da11fe4) SHA1(065336cf166807acb6c8569d59d3bf37a19b0a42) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "257-c8.bin",   0x3000001, 0x800000, CRC(b1afa60b) SHA1(b916184f5cfe4121752270f4f65abf35d8eb0519) ) /* Plane 2,3 */
ROM_END

ROM_START( bangbead ) /* Original Version - Encrypted GFX */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "259-p1.bin", 0x100000, 0x100000, CRC(88a37f8b) SHA1(566db84850fad5e8fe822e8bba910a33e083b550) )
	ROM_CONTINUE(                       0x000000, 0x100000 )

	/* The Encrypted Boards do _not_ have an s1 rom, data for it comes from the Cx ROMs */
	ROM_REGION( 0x20000, REGION_GFX1, 0 )
	ROM_FILL(                 0x000000, 0x20000, 0 )
	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "sfix.sfx",  0x000000, 0x20000, CRC(354029fc) SHA1(4ae4bf23b4c2acff875775d4cbff5583893ce2a1) )

	NEO_BIOS_SOUND_128K( "259-m1.bin", CRC(85668ee9) SHA1(7d3f51710cf90c097cd3faaeeef10ceb85cbb3e8) )

	ROM_REGION( 0x500000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "259-v1.bin", 0x000000, 0x400000, CRC(088eb8ab) SHA1(608306e35501dd7d382d9f96b28e7550aa896a03) )
	ROM_LOAD( "259-v2.bin", 0x400000, 0x100000, CRC(97528fe9) SHA1(8f5eddbb3a9a225492479d1a44801f3916c8e791) )

	NO_DELTAT_REGION

	ROM_REGION( 0x1000000, REGION_GFX3, 0 )
	/* Encrypted */
	ROM_LOAD16_BYTE( "259-c1.bin", 0x0000000, 0x800000, CRC(1f537f74) SHA1(b8ef691e92191c20a5ed4f20a75cca3c7383bca6) )
	ROM_LOAD16_BYTE( "259-c2.bin", 0x0000001, 0x800000, CRC(0efd98ff) SHA1(d350315d3c7f26d638458e5ccf2126069a4c7a5b) )
ROM_END

/* same data, diff. format roms, not encrypted, it could be a bootleg, not a prototype, since its identical
   when decrypted i'm disabling it for now */
#if 0
ROM_START( bangbedp )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "259-p1.bin", 0x100000, 0x100000, CRC(88a37f8b) SHA1(566db84850fad5e8fe822e8bba910a33e083b550) )
	ROM_CONTINUE(                       0x000000, 0x100000 )
	NEO_SFIX_128K( "259-s1p.bin", CRC(bb50fb2d) SHA1(7372939f328fb5e7d09c16985e09ae8c34702b0c) )
	NEO_BIOS_SOUND_128K( "259-m1.bin", CRC(85668ee9) SHA1(7d3f51710cf90c097cd3faaeeef10ceb85cbb3e8) )
	ROM_REGION( 0x500000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "259-v1p.bin", 0x000000, 0x200000, CRC(e97b9385) SHA1(d213cae6cf5732b8ab1f8a8cf04afee5dfd9a260) )
	ROM_LOAD( "259-v2p.bin", 0x200000, 0x200000, CRC(b0cbd70a) SHA1(fc7c8183f8ff9800e8aae7c8d44d962300058cfb) )
	ROM_LOAD( "259-v3p.bin", 0x400000, 0x100000, CRC(97528fe9) SHA1(8f5eddbb3a9a225492479d1a44801f3916c8e791) )
	NO_DELTAT_REGION
	ROM_REGION( 0x600000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "259-c1p.bin", 0x000000, 0x200000, CRC(e3919e44) SHA1(54c722414b5a7ad311dc8ddf6fdda88535e829d1) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "259-c2p.bin", 0x000001, 0x200000, CRC(baf5a320) SHA1(ead3d81d9b4aeb45af4f9cb5c38157f2236b506c) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "259-c3p.bin", 0x400000, 0x100000, CRC(c8e52157) SHA1(f10f58e905c4cbaf182b20e63abe5364462133c5) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "259-c4p.bin", 0x400001, 0x100000, CRC(69fa8e60) SHA1(29c1fbdb79dedf1470683202e2cb3435732d9275) ) /* Plane 2,3 */
ROM_END
#endif

ROM_START( nitd ) /* Original Version - Encrypted GFX */
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "260-p1.bin", 0x000000, 0x080000, CRC(61361082) SHA1(441f3f41c1aa752c0e0a9a0b1d92711d9e636b85) )

	/* The Encrypted Boards do _not_ have an s1 rom, data for it comes from the Cx ROMs */
	ROM_REGION( 0x20000, REGION_GFX1, 0 )
	ROM_FILL(                 0x000000, 0x20000, 0 )
	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "sfix.sfx",  0x000000, 0x20000, CRC(354029fc) SHA1(4ae4bf23b4c2acff875775d4cbff5583893ce2a1) )

	NEO_BIOS_SOUND_512K( "260-m1.bin", CRC(6407c5e5) SHA1(d273e154cc905b63205a17a1a6d419cac3485a92) )

	ROM_REGION( 0x0400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "260-v1.bin", 0x000000, 0x400000, CRC(24b0480c) SHA1(d769e621be52a5cd2e2568891b5f95a48268e1e0) )

	NO_DELTAT_REGION

	ROM_REGION( 0x1000000, REGION_GFX3, 0 )
	/* Encrypted */
	ROM_LOAD16_BYTE( "260-c1.bin", 0x0000000, 0x800000, CRC(147b0c7f) SHA1(a647c3a2f6d146ff47521c1d39f58830601f5781) )
	ROM_LOAD16_BYTE( "260-c2.bin", 0x0000001, 0x800000, CRC(d2b04b0d) SHA1(ce4322e6cfacb627fe997efe81018861e21d3c27) )
ROM_END

ROM_START( b2b )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "071-p1.bin", 0x000000, 0x80000, CRC(7687197d) SHA1(4bb9cb7819807f7a7e1f85f1c4faac4a2f8761e8) )

	NEO_SFIX_128K( "071-s1.bin", CRC(44e5f154) SHA1(b3f80051789e60e5d8c5df0408f1aba51616e92d) )

	NEO_BIOS_SOUND_128K( "071-m1.bin", CRC(6da739ad) SHA1(cbf5f55c54b4ee00943e2a411eeee4e465ce9c34) )

	ROM_REGION( 0x100000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "071-v1.bin", 0x000000, 0x100000, CRC(50feffb0) SHA1(00127dae0130889995bfa7560bc4b0662f74fba5) )

	NO_DELTAT_REGION

	ROM_REGION( 0x1000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "071-c1.bin", 0x000000, 0x200000, CRC(23d84a7a) SHA1(9034658ad40e2c45558abc3db312aa2764102fc4) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "071-c2.bin", 0x000001, 0x200000, CRC(ce7b6248) SHA1(ad1cd5adae5c151e183ff88b68afe10f7009f48e) ) /* Plane 2,3 */
ROM_END

ROM_START( ghostlop )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "gl-p1.rom", 0x000000, 0x100000, CRC(6033172e) SHA1(f57fb706aa8dd9e5f9e992a5d35c1799578b59f8) )

	NEO_SFIX_128K( "gl-s1.rom",  CRC(83c24e81) SHA1(585ef209d8bfc23bdccc1f37d8b764eeedfedc1c) )

	NEO_BIOS_SOUND_128K( "gl-m1.rom", CRC(fd833b33) SHA1(ab6c218c42cba821654cbdae154efecb69f844f6) )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "gl-v1.rom", 0x000000, 0x200000, CRC(c603fce6) SHA1(5a866471d35895b2ae13cbd5d1cb41bf2e72e1b8) )

	NO_DELTAT_REGION

	ROM_REGION( 0x800000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "gl-c1.rom",  0x000000, 0x400000, CRC(bfc99efe) SHA1(5cd2545310142080b8286e787cf5b859f627b3db) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "gl-c2.rom",  0x000001, 0x400000, CRC(69788082) SHA1(c3ecb42ddcbd9e16d0018a0c3adb56a911d813ca) ) /* Plane 2,3 */
ROM_END

ROM_START( ironclad )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "220-p1.bin", 0x100000, 0x100000, CRC(62a942c6) SHA1(12aaa7d9bd84328d1bf4610e056b5c57d0252537) )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "220-s1.bin", CRC(372fe217) SHA1(493433e682f519bf647e1481c8bdd3a980830ffb) )

	NEO_BIOS_SOUND_128K( "220-m1.bin", CRC(3a08bb63) SHA1(d8fbbf42a006ccafc3cd99808d28c82dbaac4590) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "220-v1.bin", 0x000000, 0x400000, CRC(8f30a215) SHA1(0ee866a468c4c3608d55df2b5cb9243c8016d77c) )

	NO_DELTAT_REGION

	ROM_REGION( 0x1000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "220-c1.bin", 0x000000, 0x400000, CRC(9aa2b7dc) SHA1(6b3dff292c86f949890b1f8201bc5278f38c2668) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "220-c2.bin", 0x000001, 0x400000, CRC(8a2ad708) SHA1(9568ac4cc0552e7fd3e50d3cd8d9f0f4fe7df1d4) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "220-c3.bin", 0x800000, 0x400000, CRC(d67fb15a) SHA1(842971aeaf3c92e70f7c653bbf29058bc60f5b71) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "220-c4.bin", 0x800001, 0x400000, CRC(e73ea38b) SHA1(27138d588e61e86c292f12d16e36c3681075c607) ) /* Plane 2,3 */
ROM_END

ROM_START( sengoku3 ) /* Original Version - Encrypted GFX */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "261-p1.bin", 0x100000, 0x100000, CRC(e0d4bc0a) SHA1(8df366097f224771ca6d1aa5c1691cd46776cd12) )
	ROM_CONTINUE(                       0x000000, 0x100000 )

	/* The Encrypted Boards do _not_ have an s1 rom, data for it comes from the Cx ROMs */
	ROM_REGION( 0x20000, REGION_GFX1, 0 )
	ROM_FILL(                 0x000000, 0x20000, 0 )
	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "sfix.sfx",  0x000000, 0x20000, CRC(354029fc) SHA1(4ae4bf23b4c2acff875775d4cbff5583893ce2a1) )

	NEO_BIOS_SOUND_128K( "261-m1.bin", CRC(36ed9cdd) SHA1(78a7d755e9e9f52255ac6228d9d402fd6a02c126) )

	ROM_REGION( 0x0e00000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "261-v1.bin", 0x000000, 0x400000, CRC(64c30081) SHA1(f9ebd20cf59b72e864b7274c1bdb6d99ecaf4595) )
	ROM_LOAD( "261-v2.bin", 0x400000, 0x400000, CRC(392a9c47) SHA1(7ab90a54089236ca6c3ef1af8e566a8025d38159) )
	ROM_LOAD( "261-v3.bin", 0x800000, 0x400000, CRC(c1a7ebe3) SHA1(1d7bb481451f5ee0457e954bb5210300182c3c9c) )
	ROM_LOAD( "261-v4.bin", 0xc00000, 0x200000, CRC(9000d085) SHA1(11157b355ab4eb6627e9f322ed875332d3d77349) )

	NO_DELTAT_REGION

	ROM_REGION( 0x2000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "261-c1.bin", 0x0000000, 0x800000, CRC(ded84d9c) SHA1(d960523b813d4fae06d716298d4e431a5c77a0c5) )
	ROM_LOAD16_BYTE( "261-c2.bin", 0x0000001, 0x800000, CRC(b8eb4348) SHA1(619d24312549932959481fa58f43f11c048e1ca5) )
	ROM_LOAD16_BYTE( "261-c3.bin", 0x1000000, 0x800000, CRC(84e2034a) SHA1(38ec4ae4b86933a25c9a03799b8cade4b1346401) )
	ROM_LOAD16_BYTE( "261-c4.bin", 0x1000001, 0x800000, CRC(0b45ae53) SHA1(a19fb21408ab633aee8bbf38bf43b5e26766b355) )
ROM_END

ROM_START( zupapa ) /* Original Version - Encrypted GFX */
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "070-p1.bin", 0x000000, 0x100000, CRC(5a96203e) SHA1(49cddec9ca6cc51e5ecf8a34e447a23e1f8a15a1) )

	/* The Encrypted Boards do _not_ have an s1 rom, data for it comes from the Cx ROMs */
	ROM_REGION( 0x20000, REGION_GFX1, 0 )
	ROM_FILL(                 0x000000, 0x20000, 0 )
	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "sfix.sfx",  0x000000, 0x20000, CRC(354029fc) SHA1(4ae4bf23b4c2acff875775d4cbff5583893ce2a1) )

	NEO_BIOS_SOUND_128K( "070-m1.bin", CRC(5a3b3191) SHA1(fa9a9930e18c64e598841fb344c4471d3d2c1964) )

	ROM_REGION( 0x0200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "070-v1.bin", 0x000000, 0x200000, CRC(d3a7e1ff) SHA1(4a4a227e10f4af58168f6c26011ea1d414253f92) )

	NO_DELTAT_REGION

	ROM_REGION( 0x1000000, REGION_GFX3, 0 )
	/* Encrypted */
	ROM_LOAD16_BYTE( "070-c1.bin", 0x0000000, 0x800000, CRC(f8ad02d8) SHA1(9be54532332a8e963ec35ff1e518947bb11ebade) )
	ROM_LOAD16_BYTE( "070-c2.bin", 0x0000001, 0x800000, CRC(70156dde) SHA1(06286bf043d50199b47df9a76ca91f39cb28cb90) )
ROM_END

ROM_START( kof2001 )
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "262-p1.bin", 0x000000, 0x100000, CRC(9381750d) SHA1(dcfecd69e563ff52fe07d23c5372d0f748b07819) )
	ROM_LOAD16_WORD_SWAP( "262-p2.bin", 0x100000, 0x400000, CRC(8e0d8329) SHA1(10dcc1baf0aaf1fc84c4d856bca6bcff85aed2bc) )

	/* The Encrypted Boards do _not_ have an s1 rom, data for it comes from the Cx ROMs */
	ROM_REGION( 0x20000, REGION_GFX1, 0 )
	ROM_FILL(                 0x000000, 0x20000, 0 )
	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "sfix.sfx",  0x000000, 0x20000, CRC(354029fc) SHA1(4ae4bf23b4c2acff875775d4cbff5583893ce2a1) )

/* Encrypted */
	NEO_BIOS_AUDIO_ENCRYPTED_256K( "265-262-m1.bin", CRC(a7f8119f) SHA1(71805b39b8b09c32425cf39f9de59b2f755976c2) ) /* yes it really does have a strange name */

	ROM_REGION( 0x1000000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "262-v1.bin", 0x000000, 0x400000, CRC(83d49ecf) SHA1(2f2c116e45397652e77fcf5d951fa5f71b639572) )
	ROM_LOAD( "262-v2.bin", 0x400000, 0x400000, CRC(003f1843) SHA1(bdd58837ad542548bd4053c262f558af88e3b989) )
	ROM_LOAD( "262-v3.bin", 0x800000, 0x400000, CRC(2ae38dbe) SHA1(4e82b7dd3b899d61907620517a5a27bdaba0725d) )
	ROM_LOAD( "262-v4.bin", 0xc00000, 0x400000, CRC(26ec4dd9) SHA1(8bd68d95a2d913be41a51f51e48dbe3bff5924fb) )

	NO_DELTAT_REGION

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	/* Encrypted */
	ROM_LOAD16_BYTE( "262-c1.bin", 0x0000000, 0x800000, CRC(99cc785a) SHA1(374f0674871d0196fa274aa6c5956d7b3848d5da) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "262-c2.bin", 0x0000001, 0x800000, CRC(50368cbf) SHA1(5d9e206e98e0b0c7735b72ea46b45058fdec2352) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "262-c3.bin", 0x1000000, 0x800000, CRC(fb14ff87) SHA1(445a8db2fc69eff54a252700f2d3a89244c58e75) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "262-c4.bin", 0x1000001, 0x800000, CRC(4397faf8) SHA1(6752b394f6647502a649a3e62bd3442f936b733e) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "262-c5.bin", 0x2000000, 0x800000, CRC(91f24be4) SHA1(88190c41f7d4a0f4b1982149fc9acfc640af498d) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "262-c6.bin", 0x2000001, 0x800000, CRC(a31e4403) SHA1(5cd1a14703aa58810e2377dfb7353c61e9dc9c1f) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "262-c7.bin", 0x3000000, 0x800000, CRC(54d9d1ec) SHA1(80c3a8ec39130dd5d3da561f287709da6b8abcf4) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "262-c8.bin", 0x3000001, 0x800000, CRC(59289a6b) SHA1(ddfce7c85b2a144975db5bb14b4b51aaf881880e) ) /* Plane 2,3 */
ROM_END

ROM_START( kof2001h )
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "262-pg1.bin", 0x000000, 0x100000, CRC(2af7e741) SHA1(e41282d73ed6d521da056f1a16573bb61bfa3826) )
	ROM_LOAD16_WORD_SWAP( "262-pg2.bin", 0x100000, 0x400000, CRC(91eea062) SHA1(82bae42bbeedb9f3aa0c7c0b0a7a69be499cf98f) )

	/* The Encrypted Boards do _not_ have an s1 rom, data for it comes from the Cx ROMs */
	ROM_REGION( 0x20000, REGION_GFX1, 0 )
	ROM_FILL(                 0x000000, 0x20000, 0 )
	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "sfix.sfx",  0x000000, 0x20000, CRC(354029fc) SHA1(4ae4bf23b4c2acff875775d4cbff5583893ce2a1) )

	/* Encrypted */
	NEO_BIOS_AUDIO_ENCRYPTED_256K( "265-262-m1.bin", CRC(a7f8119f) SHA1(71805b39b8b09c32425cf39f9de59b2f755976c2) ) /* yes it really does have a strange name */

	ROM_REGION( 0x1000000, REGION_SOUND1, 0 )
	ROM_LOAD( "262-v1.bin", 0x000000, 0x400000, CRC(83d49ecf) SHA1(2f2c116e45397652e77fcf5d951fa5f71b639572) )
	ROM_LOAD( "262-v2.bin", 0x400000, 0x400000, CRC(003f1843) SHA1(bdd58837ad542548bd4053c262f558af88e3b989) )
	ROM_LOAD( "262-v3.bin", 0x800000, 0x400000, CRC(2ae38dbe) SHA1(4e82b7dd3b899d61907620517a5a27bdaba0725d) )
	ROM_LOAD( "262-v4.bin", 0xc00000, 0x400000, CRC(26ec4dd9) SHA1(8bd68d95a2d913be41a51f51e48dbe3bff5924fb) )

	NO_DELTAT_REGION

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	/* Encrypted */
	ROM_LOAD16_BYTE( "262-c1.bin", 0x0000000, 0x800000, CRC(99cc785a) SHA1(374f0674871d0196fa274aa6c5956d7b3848d5da) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "262-c2.bin", 0x0000001, 0x800000, CRC(50368cbf) SHA1(5d9e206e98e0b0c7735b72ea46b45058fdec2352) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "262-c3.bin", 0x1000000, 0x800000, CRC(fb14ff87) SHA1(445a8db2fc69eff54a252700f2d3a89244c58e75) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "262-c4.bin", 0x1000001, 0x800000, CRC(4397faf8) SHA1(6752b394f6647502a649a3e62bd3442f936b733e) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "262-c5.bin", 0x2000000, 0x800000, CRC(91f24be4) SHA1(88190c41f7d4a0f4b1982149fc9acfc640af498d) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "262-c6.bin", 0x2000001, 0x800000, CRC(a31e4403) SHA1(5cd1a14703aa58810e2377dfb7353c61e9dc9c1f) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "262-c7.bin", 0x3000000, 0x800000, CRC(54d9d1ec) SHA1(80c3a8ec39130dd5d3da561f287709da6b8abcf4) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "262-c8.bin", 0x3000001, 0x800000, CRC(59289a6b) SHA1(ddfce7c85b2a144975db5bb14b4b51aaf881880e) ) /* Plane 2,3 */
ROM_END

ROM_START( cthd2003 ) /* Protected hack/bootleg of kof2001 Phenixsoft */
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "5003-p1.bin", 0x000000, 0x100000, CRC(bb7602c1) SHA1(abf329a40f34c88f7325b255e3bc090db1edaca4) )
	ROM_LOAD16_WORD_SWAP( "5003-p2.bin", 0x100000, 0x400000, CRC(adc1c22b) SHA1(271e0629989257a0d21d280c05df53df259414b1) )

	NEO_SFIX_128K( "5003-s1.bin", CRC(5ba29aab) SHA1(e7ea67268a10243693bff722e6fd2276ca540acf) )

	NEO_BIOS_SOUND_128K( "5003-m1.bin", CRC(1a8c274b) SHA1(5f6f9c533f4a296a18c741ce59a69cf6f5c836b9) )

	/* sound roms are identical to kof2001 */
	ROM_REGION( 0x1000000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "262-v1.bin", 0x000000, 0x400000, CRC(83d49ecf) SHA1(2f2c116e45397652e77fcf5d951fa5f71b639572) )
	ROM_LOAD( "262-v2.bin", 0x400000, 0x400000, CRC(003f1843) SHA1(bdd58837ad542548bd4053c262f558af88e3b989) )
	ROM_LOAD( "262-v3.bin", 0x800000, 0x400000, CRC(2ae38dbe) SHA1(4e82b7dd3b899d61907620517a5a27bdaba0725d) )
	ROM_LOAD( "262-v4.bin", 0xc00000, 0x400000, CRC(26ec4dd9) SHA1(8bd68d95a2d913be41a51f51e48dbe3bff5924fb) )

	NO_DELTAT_REGION

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	/* Encrypted */
	ROM_LOAD16_BYTE( "5003-c1.bin", 0x0000000, 0x800000, CRC(68f54b67) SHA1(e2869709b11ea2846799fe431211c83e928e103e) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "5003-c2.bin", 0x0000001, 0x800000, CRC(2f8849d5) SHA1(7ef74981aa056f5acab4ddabffd3e98b4cb970be) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "5003-c3.bin", 0x1000000, 0x800000, CRC(ac4aff71) SHA1(c983f642e68deaa40fee3e208f2dd55f3bacbdc1) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "5003-c4.bin", 0x1000001, 0x800000, CRC(afef5d66) SHA1(39fe785563fbea54bba88de60dcc62e2458bd74a) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "5003-c5.bin", 0x2000000, 0x800000, CRC(c7c1ae50) SHA1(f54f5be7513a5ce2f01ab107a2b26f6a9ee1f2a9) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "5003-c6.bin", 0x2000001, 0x800000, CRC(613197f9) SHA1(6d1fefa1be81b79e251e55a1352544c0298e4674) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "5003-c7.bin", 0x3000000, 0x800000, CRC(64ddfe0f) SHA1(361f3f4618009bf6419961266eb9ab5002bef53c) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "5003-c8.bin", 0x3000001, 0x800000, CRC(917a1439) SHA1(6f28d1d7c6edee1283f25e632c69204dbebe40af) ) /* Plane 2,3 */
ROM_END

ROM_START( ct2k3sp ) /*  Protected hack/bootleg of kof2001 Phenixsoft */
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "5003-p1sp.bin", 0x000000, 0x100000, CRC(ab5c4de0) SHA1(ca9a6bfd4c32d791ecabb34ccbf2cbf0e84f97d5) )
	ROM_LOAD16_WORD_SWAP( "5003-p2.bin", 0x100000, 0x400000, CRC(adc1c22b) SHA1(271e0629989257a0d21d280c05df53df259414b1) )

	ROM_REGION( 0x40000, REGION_GFX1, 0 )
	ROM_LOAD( "5003-s1sp.bin", 0x00000, 0x40000, CRC(6c355ab4) SHA1(71ac2bcd3dbda8402baecc56dabc2297b148a900) )
	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "sfix.sfx",  0x000000, 0x20000, CRC(354029fc) SHA1(4ae4bf23b4c2acff875775d4cbff5583893ce2a1) )

	NEO_BIOS_SOUND_128K( "5003-m1.bin", CRC(1a8c274b) SHA1(5f6f9c533f4a296a18c741ce59a69cf6f5c836b9) )

	/* sound roms are identical to kof2001 */
	ROM_REGION( 0x1000000, REGION_SOUND1, 0 )
	ROM_LOAD( "262-v1.bin", 0x000000, 0x400000, CRC(83d49ecf) SHA1(2f2c116e45397652e77fcf5d951fa5f71b639572) )
	ROM_LOAD( "262-v2.bin", 0x400000, 0x400000, CRC(003f1843) SHA1(bdd58837ad542548bd4053c262f558af88e3b989) )
	ROM_LOAD( "262-v3.bin", 0x800000, 0x400000, CRC(2ae38dbe) SHA1(4e82b7dd3b899d61907620517a5a27bdaba0725d) )
	ROM_LOAD( "262-v4.bin", 0xc00000, 0x400000, CRC(26ec4dd9) SHA1(8bd68d95a2d913be41a51f51e48dbe3bff5924fb) )

	NO_DELTAT_REGION

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	/* Encrypted */
	ROM_LOAD16_BYTE( "5003-c1.bin", 0x0000000, 0x800000, CRC(68f54b67) SHA1(e2869709b11ea2846799fe431211c83e928e103e) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "5003-c2.bin", 0x0000001, 0x800000, CRC(2f8849d5) SHA1(7ef74981aa056f5acab4ddabffd3e98b4cb970be) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "5003-c3.bin", 0x1000000, 0x800000, CRC(ac4aff71) SHA1(c983f642e68deaa40fee3e208f2dd55f3bacbdc1) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "5003-c4.bin", 0x1000001, 0x800000, CRC(afef5d66) SHA1(39fe785563fbea54bba88de60dcc62e2458bd74a) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "5003-c5.bin", 0x2000000, 0x800000, CRC(c7c1ae50) SHA1(f54f5be7513a5ce2f01ab107a2b26f6a9ee1f2a9) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "5003-c6.bin", 0x2000001, 0x800000, CRC(613197f9) SHA1(6d1fefa1be81b79e251e55a1352544c0298e4674) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "5003-c7.bin", 0x3000000, 0x800000, CRC(64ddfe0f) SHA1(361f3f4618009bf6419961266eb9ab5002bef53c) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "5003-c8.bin", 0x3000001, 0x800000, CRC(917a1439) SHA1(6f28d1d7c6edee1283f25e632c69204dbebe40af) ) /* Plane 2,3 */
ROM_END

ROM_START( kof2002 ) /* Encrypted Set */
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "265-p1.bin", 0x000000, 0x100000, CRC(9ede7323) SHA1(ad9d45498777fda9fa58e75781f48e09aee705a6) )
	ROM_LOAD16_WORD_SWAP( "265-p2.bin", 0x100000, 0x400000, CRC(327266b8) SHA1(98f445cc0a94f8744d74bca71cb420277622b034) )

	/* The Encrypted Boards do _not_ have an s1 rom, data for it comes from the Cx ROMs */
	ROM_REGION( 0x20000, REGION_GFX1, 0 )
	ROM_FILL( 0x000000, 0x20000, 0 )
	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "sfix.sfx", 0x000000, 0x20000, CRC(354029fc) SHA1(4ae4bf23b4c2acff875775d4cbff5583893ce2a1) )

	NEO_BIOS_AUDIO_ENCRYPTED_128K( "265-m1.bin", CRC(85aaa632) SHA1(744fba4ca3bc3a5873838af886efb97a8a316104) )
	
	ROM_REGION( 0x1000000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	/* Encrypted */
	ROM_LOAD( "265-v1.bin", 0x000000, 0x800000, CRC(15e8f3f5) SHA1(7c9e6426b9fa6db0158baa17a6485ffce057d889) )
	ROM_LOAD( "265-v2.bin", 0x800000, 0x800000, CRC(da41d6f9) SHA1(a43021f1e58947dcbe3c8ca5283b20b649f0409d) )

	NO_DELTAT_REGION

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	/* Encrypted */
	ROM_LOAD16_BYTE( "265-c1.bin", 0x0000000, 0x800000, CRC(2b65a656) SHA1(9c46d8cf5b1ef322db442ac6a9b9406ab49206c5) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "265-c2.bin", 0x0000001, 0x800000, CRC(adf18983) SHA1(150cd4a5e51e9df88688469d2ea7675c2cf3658a) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "265-c3.bin", 0x1000000, 0x800000, CRC(875e9fd7) SHA1(28f52d56192d48bbc5dc3c97abf456bd34a58cbd) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "265-c4.bin", 0x1000001, 0x800000, CRC(2da13947) SHA1(f8d79ec2c236aa3d3648a4f715676899602122c1) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "265-c5.bin", 0x2000000, 0x800000, CRC(61bd165d) SHA1(b3424db84bc683d858fb635bc42728f9cdd89caf) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "265-c6.bin", 0x2000001, 0x800000, CRC(03fdd1eb) SHA1(6155c7e802062f4eafa27e414c4e73ee59b868bf) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "265-c7.bin", 0x3000000, 0x800000, CRC(1a2749d8) SHA1(af7d9ec1d576209826fa568f676bbff92f6d6ddd) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "265-c8.bin", 0x3000001, 0x800000, CRC(ab0bb549) SHA1(d23afb60b7f831f7d4a98ad3c4a00ee19877a1ce) ) /* Plane 2,3 */
ROM_END

ROM_START( kf2k2pls ) /* bootleg */
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "265-p1p.bin", 0x000000, 0x100000, CRC(3ab03781) SHA1(86946c19f1c4d9ab5cde86688d698bf63118a39d) )
	ROM_LOAD16_WORD_SWAP( "265-p2.bin", 0x100000, 0x400000, CRC(327266b8) SHA1(98f445cc0a94f8744d74bca71cb420277622b034) )

	NEO_SFIX_128K( "265-s1p.bin", CRC(595e0006) SHA1(ff086bdaa6f40e9ad963e1100a27f44618d684ed) )

	NEO_BIOS_AUDIO_ENCRYPTED_128K( "265-m1.bin", CRC(85aaa632) SHA1(744fba4ca3bc3a5873838af886efb97a8a316104) )
	
	ROM_REGION( 0x1000000, REGION_SOUND1, 0 )
	/* Encrypted */
	ROM_LOAD( "265-v1.bin", 0x000000, 0x800000, CRC(15e8f3f5) SHA1(7c9e6426b9fa6db0158baa17a6485ffce057d889) )
	ROM_LOAD( "265-v2.bin", 0x800000, 0x800000, CRC(da41d6f9) SHA1(a43021f1e58947dcbe3c8ca5283b20b649f0409d) )

	NO_DELTAT_REGION

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	/* Encrypted */
	ROM_LOAD16_BYTE( "265-c1.bin", 0x0000000, 0x800000, CRC(2b65a656) SHA1(9c46d8cf5b1ef322db442ac6a9b9406ab49206c5) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "265-c2.bin", 0x0000001, 0x800000, CRC(adf18983) SHA1(150cd4a5e51e9df88688469d2ea7675c2cf3658a) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "265-c3.bin", 0x1000000, 0x800000, CRC(875e9fd7) SHA1(28f52d56192d48bbc5dc3c97abf456bd34a58cbd) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "265-c4.bin", 0x1000001, 0x800000, CRC(2da13947) SHA1(f8d79ec2c236aa3d3648a4f715676899602122c1) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "265-c5.bin", 0x2000000, 0x800000, CRC(61bd165d) SHA1(b3424db84bc683d858fb635bc42728f9cdd89caf) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "265-c6.bin", 0x2000001, 0x800000, CRC(03fdd1eb) SHA1(6155c7e802062f4eafa27e414c4e73ee59b868bf) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "265-c7.bin", 0x3000000, 0x800000, CRC(1a2749d8) SHA1(af7d9ec1d576209826fa568f676bbff92f6d6ddd) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "265-c8.bin", 0x3000001, 0x800000, CRC(ab0bb549) SHA1(d23afb60b7f831f7d4a98ad3c4a00ee19877a1ce) ) /* Plane 2,3 */
ROM_END

ROM_START( kf2k2pla ) /* bootleg */
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "265-p1pa.bin", 0x000000, 0x100000, CRC(6a3a02f3) SHA1(c9973b64e9a87fa38dde233ee3e9a73ba085b013) )
	ROM_LOAD16_WORD_SWAP( "265-p2.bin", 0x100000, 0x400000, CRC(327266b8) SHA1(98f445cc0a94f8744d74bca71cb420277622b034) )

	NEO_SFIX_128K( "265-s1pa.bin",  CRC(1a3ed064) SHA1(9749bb55c750e6b65d651998c2649c5fb68db68e))

	NEO_BIOS_AUDIO_ENCRYPTED_128K( "265-m1.bin", CRC(85aaa632) SHA1(744fba4ca3bc3a5873838af886efb97a8a316104) )
	
	ROM_REGION( 0x1000000, REGION_SOUND1, 0 )
	/* Encrypted */
	ROM_LOAD( "265-v1.bin", 0x000000, 0x800000, CRC(15e8f3f5) SHA1(7c9e6426b9fa6db0158baa17a6485ffce057d889) )
	ROM_LOAD( "265-v2.bin", 0x800000, 0x800000, CRC(da41d6f9) SHA1(a43021f1e58947dcbe3c8ca5283b20b649f0409d) )

	NO_DELTAT_REGION

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	/* Encrypted */
	ROM_LOAD16_BYTE( "265-c1.bin", 0x0000000, 0x800000, CRC(2b65a656) SHA1(9c46d8cf5b1ef322db442ac6a9b9406ab49206c5) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "265-c2.bin", 0x0000001, 0x800000, CRC(adf18983) SHA1(150cd4a5e51e9df88688469d2ea7675c2cf3658a) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "265-c3.bin", 0x1000000, 0x800000, CRC(875e9fd7) SHA1(28f52d56192d48bbc5dc3c97abf456bd34a58cbd) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "265-c4.bin", 0x1000001, 0x800000, CRC(2da13947) SHA1(f8d79ec2c236aa3d3648a4f715676899602122c1) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "265-c5.bin", 0x2000000, 0x800000, CRC(61bd165d) SHA1(b3424db84bc683d858fb635bc42728f9cdd89caf) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "265-c6.bin", 0x2000001, 0x800000, CRC(03fdd1eb) SHA1(6155c7e802062f4eafa27e414c4e73ee59b868bf) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "265-c7.bin", 0x3000000, 0x800000, CRC(1a2749d8) SHA1(af7d9ec1d576209826fa568f676bbff92f6d6ddd) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "265-c8.bin", 0x3000001, 0x800000, CRC(ab0bb549) SHA1(d23afb60b7f831f7d4a98ad3c4a00ee19877a1ce) ) /* Plane 2,3 */
ROM_END

ROM_START( kf2k2mp )
	ROM_REGION( 0x800000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "kf02m-p1.bin", 0x000000, 0x400000, CRC(ff7c6ec0) SHA1(704c14d671dcb4cfed44d9f978a289cb7dd9d065) )
	ROM_LOAD16_WORD_SWAP( "kf02m-p2.bin", 0x400000, 0x400000, CRC(91584716) SHA1(90da863037cf775957fa154cd42536e221df5740) )

	NEO_SFIX_128K( "kf02m-s1.bin", CRC(348d6f2c) SHA1(586da8a936ebbb71af324339a4b60ec91dfa0990) )

	NEO_BIOS_AUDIO_ENCRYPTED_128K( "265-m1.bin", CRC(85aaa632) SHA1(744fba4ca3bc3a5873838af886efb97a8a316104) )
	
	ROM_REGION( 0x1000000, REGION_SOUND1, 0 )
	/* Encrypted */
	ROM_LOAD( "265-v1.bin", 0x000000, 0x800000, CRC(15e8f3f5) SHA1(7c9e6426b9fa6db0158baa17a6485ffce057d889) )
	ROM_LOAD( "265-v2.bin", 0x800000, 0x800000, CRC(da41d6f9) SHA1(a43021f1e58947dcbe3c8ca5283b20b649f0409d) )

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	/* Encrypted */
	ROM_LOAD16_BYTE( "265-c1.bin", 0x0000000, 0x800000, CRC(2b65a656) SHA1(9c46d8cf5b1ef322db442ac6a9b9406ab49206c5) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "265-c2.bin", 0x0000001, 0x800000, CRC(adf18983) SHA1(150cd4a5e51e9df88688469d2ea7675c2cf3658a) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "265-c3.bin", 0x1000000, 0x800000, CRC(875e9fd7) SHA1(28f52d56192d48bbc5dc3c97abf456bd34a58cbd) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "265-c4.bin", 0x1000001, 0x800000, CRC(2da13947) SHA1(f8d79ec2c236aa3d3648a4f715676899602122c1) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "265-c5.bin", 0x2000000, 0x800000, CRC(61bd165d) SHA1(b3424db84bc683d858fb635bc42728f9cdd89caf) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "265-c6.bin", 0x2000001, 0x800000, CRC(03fdd1eb) SHA1(6155c7e802062f4eafa27e414c4e73ee59b868bf) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "265-c7.bin", 0x3000000, 0x800000, CRC(1a2749d8) SHA1(af7d9ec1d576209826fa568f676bbff92f6d6ddd) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "265-c8.bin", 0x3000001, 0x800000, CRC(ab0bb549) SHA1(d23afb60b7f831f7d4a98ad3c4a00ee19877a1ce) ) /* Plane 2,3 */
ROM_END

ROM_START( kf2k2mp2 ) /* bootleg */
	ROM_REGION( 0x600000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "k2k2m2p1.bin", 0x000000, 0x200000, CRC(1016806c) SHA1(a583b45e9c0d6f67b95c52e44444aabe88f68d97) )
	ROM_LOAD16_WORD_SWAP( "k2k2m2p2.bin", 0x200000, 0x400000, CRC(432fdf53) SHA1(d7e542cd84d948162c60768e40ee4ed33d8e7913) )

	NEO_SFIX_128K( "k2k2m2s1.bin",  CRC(446e74c5) SHA1(efc2afb26578bad9eb21659c70eb0f827d6d1ef6) )

	NEO_BIOS_AUDIO_ENCRYPTED_128K( "265-m1.bin", CRC(85aaa632) SHA1(744fba4ca3bc3a5873838af886efb97a8a316104) )
	
	ROM_REGION( 0x1000000, REGION_SOUND1, 0 )
	/* Encrypted */
	ROM_LOAD( "265-v1.bin", 0x000000, 0x800000, CRC(15e8f3f5) SHA1(7c9e6426b9fa6db0158baa17a6485ffce057d889) )
	ROM_LOAD( "265-v2.bin", 0x800000, 0x800000, CRC(da41d6f9) SHA1(a43021f1e58947dcbe3c8ca5283b20b649f0409d) )

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	/* Encrypted */
	ROM_LOAD16_BYTE( "265-c1.bin", 0x0000000, 0x800000, CRC(2b65a656) SHA1(9c46d8cf5b1ef322db442ac6a9b9406ab49206c5) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "265-c2.bin", 0x0000001, 0x800000, CRC(adf18983) SHA1(150cd4a5e51e9df88688469d2ea7675c2cf3658a) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "265-c3.bin", 0x1000000, 0x800000, CRC(875e9fd7) SHA1(28f52d56192d48bbc5dc3c97abf456bd34a58cbd) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "265-c4.bin", 0x1000001, 0x800000, CRC(2da13947) SHA1(f8d79ec2c236aa3d3648a4f715676899602122c1) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "265-c5.bin", 0x2000000, 0x800000, CRC(61bd165d) SHA1(b3424db84bc683d858fb635bc42728f9cdd89caf) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "265-c6.bin", 0x2000001, 0x800000, CRC(03fdd1eb) SHA1(6155c7e802062f4eafa27e414c4e73ee59b868bf) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "265-c7.bin", 0x3000000, 0x800000, CRC(1a2749d8) SHA1(af7d9ec1d576209826fa568f676bbff92f6d6ddd) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "265-c8.bin", 0x3000001, 0x800000, CRC(ab0bb549) SHA1(d23afb60b7f831f7d4a98ad3c4a00ee19877a1ce) ) /* Plane 2,3 */
ROM_END

ROM_START( kf2k3pcb ) /* Encrypted Set, Decrypted C - JAMMA board */
	ROM_REGION( 0x900000, REGION_CPU1, 0 )
	ROM_LOAD32_WORD_SWAP( "271-p1.bin", 0x000000, 0x400000, CRC(b9da070c) SHA1(1a26325af142a4dd221c336061761468598c4634) )
	ROM_LOAD32_WORD_SWAP( "271-p2.bin", 0x000002, 0x400000, CRC(da3118c4) SHA1(582e4f44f03276adecb7b2848d3b96bf6da57f1e) )
	ROM_LOAD16_WORD_SWAP( "271-p3.bin", 0x800000, 0x100000, CRC(5cefd0d2) SHA1(cddc3164629fed4b6f715e12b109ad35d1009355) )

	ROM_REGION( 0x100000, REGION_GFX1, 0 ) /* larger char set */
	ROM_FILL( 0x000000, 0x100000, 0 )
	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "sfix.sfx", 0x000000, 0x20000, CRC(354029fc) SHA1(4ae4bf23b4c2acff875775d4cbff5583893ce2a1) )

	ROM_REGION16_BE( 0x80000, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "271-bios.bin", 0x00000, 0x080000, CRC(148dd727) SHA1(2cf592a16c7157de02a989675d47965f2b3a44dd) ) /* encrypted */


	NEO_BIOS_AUDIO_ENCRYPTED_512K( "271-m1.bin", CRC(d6bcf2bc) SHA1(df78bc95990eb8e8f3638dde6e1876354df7fe84) )
	

	ROM_REGION( 0x1000000, REGION_SOUND1, 0 )
	/* Encrypted */
	ROM_LOAD( "271-v1.bin", 0x000000, 0x1000000, CRC(1d96154b) SHA1(1d4e262b0d30cee79a4edc83bb9706023c736668) )

	NO_DELTAT_REGION

	ROM_REGION( 0x6000000, REGION_GFX3, 0 )
	/* Encrypted */
	ROM_LOAD32_WORD( "271-c1.bin", 0x0000000, 0x1000000, CRC(f5ebb327) SHA1(e4f799a54b09adcca13b1b0cf95971a1f4291b61) ) /* Plane 0,1 */
	ROM_LOAD32_WORD( "271-c2.bin", 0x0000002, 0x1000000, CRC(2be21620) SHA1(872c658f53bbc558e90f18d5db9cbaa82e748a6a) ) /* Plane 2,3 */
	ROM_LOAD32_WORD( "271-c3.bin", 0x2000000, 0x1000000, CRC(ddded4ff) SHA1(ff7b356125bc9e6637b164f5e81b13eabeb8d804) ) /* Plane 0,1 */
	ROM_LOAD32_WORD( "271-c4.bin", 0x2000002, 0x1000000, CRC(d85521e6) SHA1(62278fa8690972ed32aca07a4f7f97e7203d9f3a) ) /* Plane 2,3 */
	ROM_LOAD32_WORD( "271-c5.bin", 0x4000000, 0x1000000, CRC(18aa3540) SHA1(15e0a8c4e0927b1f7eb9bee8f532acea6818d5eb) ) /* Plane 0,1 */
	ROM_LOAD32_WORD( "271-c6.bin", 0x4000002, 0x1000000, CRC(1c40de87) SHA1(8d6425aed43ff6a96c88194e203df6a783286373) ) /* Plane 2,3 */
ROM_END

ROM_START( kof2003 ) /* Encrypted Code + Sound + GFX Roms */
	ROM_REGION( 0x900000, REGION_CPU1, 0 )
	ROM_LOAD32_WORD_SWAP( "271-p1c.bin", 0x000000, 0x400000, CRC(530ecc14) SHA1(812cf7e9902af3f5e9e330b7c05c2171b139ad2b) )
	ROM_LOAD32_WORD_SWAP( "271-p2c.bin", 0x000002, 0x400000, CRC(fd568da9) SHA1(46364906a1e81dc251117e91a1a7b43af1373ada) )
	ROM_LOAD16_WORD_SWAP( "271-p3c.bin", 0x800000, 0x100000, CRC(aec5b4a9) SHA1(74087f785590eda5898ce146029818f86ced42b6) ) /* Encrypted */

	/* The Encrypted Boards do _not_ have an s1 rom, data for it comes from the Cx ROMs */
	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* larger char set */
	ROM_FILL( 0x000000, 0x80000, 0 )
	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "sfix.sfx",  0x000000, 0x20000, CRC(354029fc) SHA1(4ae4bf23b4c2acff875775d4cbff5583893ce2a1) )

	NEO_BIOS_AUDIO_ENCRYPTED_512K( "271-m1c.bin", CRC(f5515629) SHA1(7516bf1b0207a3c8d41dc30c478f8d8b1f71304b) )
	
	ROM_REGION( 0x1000000, REGION_SOUND1, 0 )
	/* Encrypted */
	ROM_LOAD( "271-v1c.bin", 0x000000, 0x800000, CRC(ffa3f8c7) SHA1(7cf4a933973ca23b7f87c81151d8659e6ec4bd20) )
	ROM_LOAD( "271-v2c.bin", 0x800000, 0x800000, CRC(5382c7d1) SHA1(1bf999705eda80ba1e7b0d6bdd010d9bfb18bd76) )

	NO_DELTAT_REGION

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	/* Encrypted */
	ROM_LOAD16_BYTE( "271-c1c.bin", 0x0000000, 0x800000, CRC(b1dc25d0) SHA1(50adc3c60d5b4b3abd10a49db2267306c6dbd772) )
	ROM_LOAD16_BYTE( "271-c2c.bin", 0x0000001, 0x800000, CRC(d5362437) SHA1(66db36522dc09106388c707252df9fe1c88b4856) )
	ROM_LOAD16_BYTE( "271-c3c.bin", 0x1000000, 0x800000, CRC(0a1fbeab) SHA1(9fe30d36ba98d00fda010832ff2f27783dd577c1) )
	ROM_LOAD16_BYTE( "271-c4c.bin", 0x1000001, 0x800000, CRC(87b19a0c) SHA1(b72a8e7d9124ce859b5149bb4381ba481c161ea5) )
	ROM_LOAD16_BYTE( "271-c5c.bin", 0x2000000, 0x800000, CRC(704ea371) SHA1(e75b80422f0d72eac826f8ffadf79efeccaab124) )
	ROM_LOAD16_BYTE( "271-c6c.bin", 0x2000001, 0x800000, CRC(20a1164c) SHA1(c9843b37612a16fc95f6851793b1cfb5d49d811d) )
	ROM_LOAD16_BYTE( "271-c7c.bin", 0x3000000, 0x800000, CRC(189aba7f) SHA1(7152195a57ad36b28290810fe87ed8c206262ba9) )
	ROM_LOAD16_BYTE( "271-c8c.bin", 0x3000001, 0x800000, CRC(20ec4fdc) SHA1(deb5f7ec5a090e419b9d1a6a74877bee081198e2) )
ROM_END

ROM_START( kof2003d ) /* Decrypted C Roms */
	ROM_REGION( 0x900000, REGION_CPU1, 0 )
	ROM_LOAD32_WORD_SWAP( "271-p1.bin", 0x000000, 0x400000, CRC(b9da070c) )
	ROM_LOAD32_WORD_SWAP( "271-p2.bin", 0x000002, 0x400000, CRC(da3118c4) )
	ROM_LOAD16_WORD_SWAP( "271-p3.bin", 0x800000, 0x100000, CRC(5cefd0d2) )
	ROM_LOAD16_WORD_SWAP( "271-p3d.bin", 0x800000, 0x100000, CRC(59d376da) )

	ROM_REGION( 0x100000, REGION_GFX1, 0 )
	ROM_FILL(                 0x000000, 0x100000, 0 )
	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "sfix.sfx",  0x000000, 0x20000, CRC(354029fc) )

	ROM_REGION16_BE( 0x40000, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "271-osjd.bin", 0x00000, 0x040000, CRC(c521b5bc) )
	NEOGEO_BIOS

	ROM_REGION( 0x90000, REGION_CPU2, 0 )
	ROM_LOAD( "mame.sm1", 0x00000, 0x20000, CRC(97cf998b) )
	ROM_LOAD( "271-m1d.bin", 		0x00000, 0x80000, CRC(0e86af8f) )
	ROM_RELOAD(             0x10000, 0x80000 )
	ROM_REGION( 0x10000, REGION_GFX4, 0 )
	ROM_LOAD( "mamelo.lo", 0x00000, 0x10000, CRC(e09e253c) )

	ROM_REGION( 0x1000000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "271-v1d.bin", 0x000000, 0x1000000, CRC(2964f36e) )

	NO_DELTAT_REGION

	ROM_REGION( 0x6000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "271-c1d.bin", 0x0000000, 0x1000000, CRC(c29acd28) )
	ROM_LOAD16_BYTE( "271-c2d.bin", 0x0000001, 0x1000000, CRC(328e80b1) )
	ROM_LOAD16_BYTE( "271-c3d.bin", 0x2000000, 0x1000000, CRC(020a11f1) )
	ROM_LOAD16_BYTE( "271-c4d.bin", 0x2000001, 0x1000000, CRC(991b5ed2) )
	ROM_LOAD16_BYTE( "271-c5d.bin", 0x4000000, 0x1000000, CRC(c2de8b66) )
	ROM_LOAD16_BYTE( "271-c6d.bin", 0x4000001, 0x1000000, CRC(3ff750db) )
ROM_END

ROM_START( kf2k3bl ) 
	ROM_REGION( 0x800000, REGION_CPU1, 0 )
 	ROM_LOAD16_WORD_SWAP( "2k3-p1.bin", 0x100000, 0x400000, CRC(92ed6ee3) SHA1(5e7e21eb40dfcc453ba73808760d5ddedd49c58a) )
	ROM_LOAD16_WORD_SWAP( "2k3-p2.bin", 0x500000, 0x200000, CRC(5d3d8bb3) SHA1(7f2341f14ca12ff5721eb038b3496228a1f34b60) )
	ROM_CONTINUE( 0x000000, 0x100000 )
	ROM_CONTINUE( 0x000000, 0x100000 )

	NEO_SFIX_128K( "2k3-s1.bin", CRC(482c48a5) SHA1(27e2f5295a9a838e112be28dafc111893a388a16) )

	NEO_BIOS_SOUND_128K( "2k3-m1.bin", CRC(3a4969ff) SHA1(2fc107a023a82053a8df63025829bcf12cee9610) )

	ROM_REGION( 0x1000000, REGION_SOUND1, 0 )
	/* Encrypted */
	ROM_LOAD( "271-v1c.bin", 0x000000, 0x800000, CRC(ffa3f8c7) SHA1(7cf4a933973ca23b7f87c81151d8659e6ec4bd20) )
	ROM_LOAD( "271-v2c.bin", 0x800000, 0x800000, CRC(5382c7d1) SHA1(1bf999705eda80ba1e7b0d6bdd010d9bfb18bd76) )

	NO_DELTAT_REGION

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	/* Encrypted */
	ROM_LOAD16_BYTE( "271-c1c.bin", 0x0000000, 0x800000, CRC(b1dc25d0) SHA1(50adc3c60d5b4b3abd10a49db2267306c6dbd772) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "271-c2c.bin", 0x0000001, 0x800000, CRC(d5362437) SHA1(66db36522dc09106388c707252df9fe1c88b4856) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "271-c3c.bin", 0x1000000, 0x800000, CRC(0a1fbeab) SHA1(9fe30d36ba98d00fda010832ff2f27783dd577c1) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "271-c4c.bin", 0x1000001, 0x800000, CRC(87b19a0c) SHA1(b72a8e7d9124ce859b5149bb4381ba481c161ea5) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "271-c5c.bin", 0x2000000, 0x800000, CRC(704ea371) SHA1(e75b80422f0d72eac826f8ffadf79efeccaab124) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "271-c6c.bin", 0x2000001, 0x800000, CRC(20a1164c) SHA1(c9843b37612a16fc95f6851793b1cfb5d49d811d) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "271-c7c.bin", 0x3000000, 0x800000, CRC(189aba7f) SHA1(7152195a57ad36b28290810fe87ed8c206262ba9) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "271-c8c.bin", 0x3000001, 0x800000, CRC(20ec4fdc) SHA1(deb5f7ec5a090e419b9d1a6a74877bee081198e2) ) /* Plane 2,3 */
ROM_END

ROM_START( kf2k3bla )
	ROM_REGION( 0x700000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "2k3-p1bl.bin", 0x000000, 0x100000, CRC(4ea414dd) SHA1(c242c9709c20a8cde3ad562adbe640a5dd5abcf1) )
	ROM_LOAD16_WORD_SWAP( "2k3-p3bl.bin", 0x100000, 0x400000, CRC(370acbff) SHA1(e72544de1c5e2e4f7478fc003caba9e33a306c19) )
	ROM_LOAD16_WORD_SWAP( "2k3-p2bl.bin", 0x500000, 0x200000, CRC(9c04fc52) SHA1(f41b53c79e4209373ec68276fa5941c91424bb15) )

	NEO_SFIX_128K( "2k3-s1.bin", CRC(482c48a5) SHA1(27e2f5295a9a838e112be28dafc111893a388a16) )

	NEO_BIOS_SOUND_128K( "2k3-m1.bin", CRC(3a4969ff) SHA1(2fc107a023a82053a8df63025829bcf12cee9610) )

	ROM_REGION( 0x1000000, REGION_SOUND1, 0 )
	/* Encrypted */
	ROM_LOAD( "271-v1c.bin", 0x000000, 0x800000, CRC(ffa3f8c7) SHA1(7cf4a933973ca23b7f87c81151d8659e6ec4bd20) )
	ROM_LOAD( "271-v2c.bin", 0x800000, 0x800000, CRC(5382c7d1) SHA1(1bf999705eda80ba1e7b0d6bdd010d9bfb18bd76) )

	NO_DELTAT_REGION

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	/* Encrypted */
	ROM_LOAD16_BYTE( "271-c1c.bin", 0x0000000, 0x800000, CRC(b1dc25d0) SHA1(50adc3c60d5b4b3abd10a49db2267306c6dbd772) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "271-c2c.bin", 0x0000001, 0x800000, CRC(d5362437) SHA1(66db36522dc09106388c707252df9fe1c88b4856) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "271-c3c.bin", 0x1000000, 0x800000, CRC(0a1fbeab) SHA1(9fe30d36ba98d00fda010832ff2f27783dd577c1) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "271-c4c.bin", 0x1000001, 0x800000, CRC(87b19a0c) SHA1(b72a8e7d9124ce859b5149bb4381ba481c161ea5) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "271-c5c.bin", 0x2000000, 0x800000, CRC(704ea371) SHA1(e75b80422f0d72eac826f8ffadf79efeccaab124) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "271-c6c.bin", 0x2000001, 0x800000, CRC(20a1164c) SHA1(c9843b37612a16fc95f6851793b1cfb5d49d811d) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "271-c7c.bin", 0x3000000, 0x800000, CRC(189aba7f) SHA1(7152195a57ad36b28290810fe87ed8c206262ba9) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "271-c8c.bin", 0x3000001, 0x800000, CRC(20ec4fdc) SHA1(deb5f7ec5a090e419b9d1a6a74877bee081198e2) ) /* Plane 2,3 */
ROM_END

ROM_START( kf2k3pl )
	ROM_REGION( 0x700000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "2k3-p1pl.bin", 0x000000, 0x100000, CRC(07b84112) SHA1(0b085a928a39ff9c0745a58bfa4ce6106b5f474a) )
	ROM_LOAD16_WORD_SWAP( "2k3-p3bl.bin", 0x100000, 0x400000, CRC(370acbff) SHA1(e72544de1c5e2e4f7478fc003caba9e33a306c19) )
	ROM_LOAD16_WORD_SWAP( "2k3-p2bl.bin", 0x500000, 0x200000, CRC(9c04fc52) SHA1(f41b53c79e4209373ec68276fa5941c91424bb15) )

	NEO_SFIX_128K( "2k3-s1pl.bin", CRC(ad548a36) SHA1(7483dbe2d74a1bd1b4dc501e99e48a683416d08e) )

	NEO_BIOS_SOUND_128K( "2k3-m1.bin", CRC(3a4969ff) SHA1(2fc107a023a82053a8df63025829bcf12cee9610) )

	ROM_REGION( 0x1000000, REGION_SOUND1, 0 )
	/* Encrypted */
	ROM_LOAD( "271-v1c.bin", 0x000000, 0x800000, CRC(ffa3f8c7) SHA1(7cf4a933973ca23b7f87c81151d8659e6ec4bd20) )
	ROM_LOAD( "271-v2c.bin", 0x800000, 0x800000, CRC(5382c7d1) SHA1(1bf999705eda80ba1e7b0d6bdd010d9bfb18bd76) )

	NO_DELTAT_REGION

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	/* Encrypted */
	ROM_LOAD16_BYTE( "271-c1c.bin", 0x0000000, 0x800000, CRC(b1dc25d0) SHA1(50adc3c60d5b4b3abd10a49db2267306c6dbd772) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "271-c2c.bin", 0x0000001, 0x800000, CRC(d5362437) SHA1(66db36522dc09106388c707252df9fe1c88b4856) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "271-c3c.bin", 0x1000000, 0x800000, CRC(0a1fbeab) SHA1(9fe30d36ba98d00fda010832ff2f27783dd577c1) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "271-c4c.bin", 0x1000001, 0x800000, CRC(87b19a0c) SHA1(b72a8e7d9124ce859b5149bb4381ba481c161ea5) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "271-c5c.bin", 0x2000000, 0x800000, CRC(704ea371) SHA1(e75b80422f0d72eac826f8ffadf79efeccaab124) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "271-c6c.bin", 0x2000001, 0x800000, CRC(20a1164c) SHA1(c9843b37612a16fc95f6851793b1cfb5d49d811d) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "271-c7c.bin", 0x3000000, 0x800000, CRC(189aba7f) SHA1(7152195a57ad36b28290810fe87ed8c206262ba9) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "271-c8c.bin", 0x3000001, 0x800000, CRC(20ec4fdc) SHA1(deb5f7ec5a090e419b9d1a6a74877bee081198e2) ) /* Plane 2,3 */
ROM_END

ROM_START( kf2k3upl )
	ROM_REGION( 0x800000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "2k3-p1up.bin", 0x000000, 0x800000, CRC(87294c01) SHA1(21420415a6b2ba1b43ecc1934270dc085d6bd7d9) )

	NEO_SFIX_128K( "2k3-s1up.bin", CRC(e5708c0c) SHA1(5649446d3b0b1bd138b5a8b40b96a6d0f892f4d8) )

     NEO_BIOS_SOUND_128K( "2k3-m1.bin", CRC(3a4969ff) SHA1(2fc107a023a82053a8df63025829bcf12cee9610) )

	ROM_REGION( 0x1000000, REGION_SOUND1, 0 )
	/* Encrypted */
	ROM_LOAD( "271-v1c.bin", 0x000000, 0x800000, CRC(ffa3f8c7) SHA1(7cf4a933973ca23b7f87c81151d8659e6ec4bd20) )
	ROM_LOAD( "271-v2c.bin", 0x800000, 0x800000, CRC(5382c7d1) SHA1(1bf999705eda80ba1e7b0d6bdd010d9bfb18bd76) )

	NO_DELTAT_REGION

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	/* Encrypted */
	ROM_LOAD16_BYTE( "271-c1c.bin", 0x0000000, 0x800000, CRC(b1dc25d0) SHA1(50adc3c60d5b4b3abd10a49db2267306c6dbd772) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "271-c2c.bin", 0x0000001, 0x800000, CRC(d5362437) SHA1(66db36522dc09106388c707252df9fe1c88b4856) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "271-c3c.bin", 0x1000000, 0x800000, CRC(0a1fbeab) SHA1(9fe30d36ba98d00fda010832ff2f27783dd577c1) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "271-c4c.bin", 0x1000001, 0x800000, CRC(87b19a0c) SHA1(b72a8e7d9124ce859b5149bb4381ba481c161ea5) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "271-c5c.bin", 0x2000000, 0x800000, CRC(704ea371) SHA1(e75b80422f0d72eac826f8ffadf79efeccaab124) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "271-c6c.bin", 0x2000001, 0x800000, CRC(20a1164c) SHA1(c9843b37612a16fc95f6851793b1cfb5d49d811d) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "271-c7c.bin", 0x3000000, 0x800000, CRC(189aba7f) SHA1(7152195a57ad36b28290810fe87ed8c206262ba9) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "271-c8c.bin", 0x3000001, 0x800000, CRC(20ec4fdc) SHA1(deb5f7ec5a090e419b9d1a6a74877bee081198e2) ) /* Plane 2,3 */
ROM_END

ROM_START( mslug4 ) /* Original Version - Encrypted GFX */
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "263-p1.bin",  0x000000, 0x100000, CRC(27e4def3) SHA1(a08785e8145981bb6b5332a3b2df7eb321253cca) )
	ROM_LOAD16_WORD_SWAP( "263-p2.bin",  0x100000, 0x400000, CRC(fdb7aed8) SHA1(dbeaec38f44e58ffedba99e70fa1439c2bf0dfa3) )

	/* The Encrypted Boards do _not_ have an s1 rom, data for it comes from the Cx ROMs */
	ROM_REGION( 0x80000, REGION_GFX1, 0 )	/* larger char set */
	ROM_FILL(                 0x000000, 0x20000, 0 )
	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "sfix.sfx",  0x000000, 0x20000, CRC(354029fc) SHA1(4ae4bf23b4c2acff875775d4cbff5583893ce2a1) )

	/* Encrypted */
	NEO_BIOS_AUDIO_ENCRYPTED_128K( "263-m1.bin", CRC(46ac8228) SHA1(5aeea221050c98e4bb0f16489ce772bf1c80f787) )

	ROM_REGION( 0x1000000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "263-v1.bin", 0x000000, 0x800000, CRC(01e9b9cd) SHA1(0b045c2999449f7dab5ae8a42e957d5b6650431e) )
	ROM_LOAD( "263-v2.bin", 0x800000, 0x800000, CRC(4ab2bf81) SHA1(77ccfa48f7e3daddef5fe5229a0093eb2f803742) )

	NO_DELTAT_REGION

	ROM_REGION( 0x3000000, REGION_GFX3, 0 )
	/* Encrypted */
	ROM_LOAD16_BYTE( "263-c1.bin",   0x0000000, 0x800000, CRC(84865f8a) SHA1(34467ada896eb7c7ca58658bf2a932936d8b632c) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "263-c2.bin",   0x0000001, 0x800000, CRC(81df97f2) SHA1(2b74493b8ec8fd49216a627aeb3db493f76124e3) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "263-c3.bin",   0x1000000, 0x800000, CRC(1a343323) SHA1(bbbb5232bba538c277ce2ee02e2956ca2243b787) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "263-c4.bin",   0x1000001, 0x800000, CRC(942cfb44) SHA1(d9b46c71726383c4581fb042e63897e5a3c92d1b) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "263-c5.bin",   0x2000000, 0x800000, CRC(a748854f) SHA1(2611bbedf9b5d8e82c6b2c99b88f842c46434d41) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "263-c6.bin",   0x2000001, 0x800000, CRC(5c8ba116) SHA1(6034db09c8706d4ddbcefc053efbc47a0953eb92) ) /* Plane 2,3 */
ROM_END

ROM_START( rotd ) /* Encrypted Set */
	ROM_REGION( 0x800000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "264-p1.bin", 0x000000, 0x800000, CRC(b8cc969d) SHA1(4f2205b4bdd32dd1522106ef4df10ac0eb1b852d) )

	/* The Encrypted Boards do _not_ have an s1 rom, data for it comes from the Cx ROMs */
	ROM_REGION( 0x20000, REGION_GFX1, 0 )
	ROM_FILL( 0x000000, 0x20000, 0 )
	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "sfix.sfx", 0x000000, 0x20000, CRC(354029fc) SHA1(4ae4bf23b4c2acff875775d4cbff5583893ce2a1) )

	/* Encrypted */
	NEO_BIOS_AUDIO_ENCRYPTED_128K( "264-m1.bin", CRC(4dbd7b43) SHA1(6b63756b0d2d30bbf13fbd219833c81fd060ef96) )

	ROM_REGION( 0x1000000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	/* Encrypted */
	ROM_LOAD( "264-v1.bin", 0x000000, 0x800000, CRC(fa005812) SHA1(73723126dab5a640ac11955ed6da1bf7a91394f5) )
	ROM_LOAD( "264-v2.bin", 0x800000, 0x800000, CRC(c3dc8bf0) SHA1(a105e37262d9500a30fb8a5dac05aa4fab2562a3) )

	NO_DELTAT_REGION

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	/* Encrypted */
	ROM_LOAD16_BYTE( "264-c1.bin", 0x0000000, 0x800000, CRC(4f148fee) SHA1(0821463765fad8fbd0dfbbabb7807337d0333719) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "264-c2.bin", 0x0000001, 0x800000, CRC(7cf5ff72) SHA1(ccb2f94bce943576d224cb326806942426d25584) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "264-c3.bin", 0x1000000, 0x800000, CRC(64d84c98) SHA1(8faf153f465ce6fb7770b27a7ce63caf11dd4086) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "264-c4.bin", 0x1000001, 0x800000, CRC(2f394a95) SHA1(82347e8f2b48b0522d7d91fd3f372d5768934ab2) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "264-c5.bin", 0x2000000, 0x800000, CRC(6b99b978) SHA1(8fd0a60029b41668f9e1e3056edd3c90f62efa83) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "264-c6.bin", 0x2000001, 0x800000, CRC(847d5c7d) SHA1(a2ce03f6302edf81f2645de9ec61df1a281ddd78) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "264-c7.bin", 0x3000000, 0x800000, CRC(231d681e) SHA1(87836e64dc816f8bf1c834641535ea96baacc024) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "264-c8.bin", 0x3000001, 0x800000, CRC(c5edb5c4) SHA1(253378c8739daa5da4edb15eff7050820b2b3755) ) /* Plane 2,3 */
ROM_END

ROM_START( matrim ) /* Encrypted Set */
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "266-p1.bin", 0x000000, 0x100000, CRC(5d4c2dc7) SHA1(8d723b0d28ec344eef26009b361a2b97d300dd51) )
	ROM_LOAD16_WORD_SWAP( "266-p2.bin", 0x100000, 0x400000, CRC(a14b1906) SHA1(1daa14d73512f760ef569b06f9facb279437d1db) )

	/* The Encrypted Boards do _not_ have an s1 rom, data for it comes from the Cx ROMs */
	ROM_REGION( 0x80000, REGION_GFX1, 0 )
	ROM_FILL( 0x000000, 0x80000, 0 )
	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "sfix.sfx", 0x000000, 0x20000, CRC(354029fc) SHA1(4ae4bf23b4c2acff875775d4cbff5583893ce2a1) )

	/* Encrypted */
	NEO_BIOS_AUDIO_ENCRYPTED_128K( "266-m1.bin", CRC(456c3e6c) SHA1(5a07d0186198a18d2dda1331093cf29b0b9b2984) )
	
	ROM_REGION( 0x1000000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	/* Encrypted */
	ROM_LOAD( "266-v1.bin", 0x000000, 0x800000, CRC(a4f83690) SHA1(200d683d8c30ebc6d0ed331aa5bbba39b4e07160) )
	ROM_LOAD( "266-v2.bin", 0x800000, 0x800000, CRC(d0f69eda) SHA1(9d7e98976ad433ed8a35d7afffa38130444ba7db) )

	NO_DELTAT_REGION

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	/* Encrypted */
	ROM_LOAD16_BYTE( "266-c1.bin", 0x0000000, 0x800000, CRC(505f4e30) SHA1(f22b6f76fc0cad963555dc89d072967c8dc8b79a) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "266-c2.bin", 0x0000001, 0x800000, CRC(3cb57482) SHA1(dab15bc24391f9a5173de76af48b612fb9636ccf) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "266-c3.bin", 0x1000000, 0x800000, CRC(f1cc6ad0) SHA1(66c1cccc0332ffd2d3064f06330c41f95ca09ced) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "266-c4.bin", 0x1000001, 0x800000, CRC(45b806b7) SHA1(c2bb866fded53d62fad0fc88d89d5e7d4cb1894f) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "266-c5.bin", 0x2000000, 0x800000, CRC(9a15dd6b) SHA1(194a6973a7a9e3847efe1bdbaeaeb16e74aff2dd) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "266-c6.bin", 0x2000001, 0x800000, CRC(281cb939) SHA1(bdb7766cfde581ccfaee2be7fe48445f360a2301) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "266-c7.bin", 0x3000000, 0x800000, CRC(4b71f780) SHA1(d5611a6f6b730db58613b48f2b0174661ccfb7bb) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "266-c8.bin", 0x3000001, 0x800000, CRC(29873d33) SHA1(dc77f129ed49b8d40d0d4241feef3f6c2f19a987) ) /* Plane 2,3 */
ROM_END

ROM_START( crswd2bl )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "054-p1.p1", 0x100000, 0x100000, CRC(64836147) SHA1(083cb1626885893e736fc9998036c952cd4d503b) )
	ROM_CONTINUE( 0x000000, 0x100000 )

	NEO_SFIX_128K( "054-s1.s1", CRC(22e02ddd) SHA1(ebd834affc763cc5854abf1c6c42f43f3f3755fd) )

	NEO_BIOS_SOUND_128K( "054-m1.m1", CRC(63e28343) SHA1(f46dbc2f1d6033b11047cca31a9a7d715dc69cb2) )

	ROM_REGION( 0x200000,  REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "054-v1.v1", 0x000000, 0x200000, CRC(22d4b93b) SHA1(0515f2ee5d9a8ce424c80721e06f746ac6a543a8) )

	NO_DELTAT_REGION

	ROM_REGION( 0x800000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "054-c1.c1", 0x000000, 0x400000, CRC(8221b712) SHA1(7e68871f1bfc402ef27c8fa088c680cbd133f71a) )
	ROM_LOAD16_BYTE( "054-c2.c2", 0x000001, 0x400000, CRC(d6c6183d) SHA1(cc546ff063fae2c01c109fabcd5b2d29ec3299db) )
ROM_END

/* MVS cart*/
ROM_START( lasthope )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "NGDT-300-P1.bin", 0x000000, 0x100000, CRC(3776a88f) SHA1(ea8b669da06d7c6b5ff7fa97a195f56a9253a7a1) )

	NEO_SFIX_64K( "NGDT-300-S1.bin", CRC(0c0ff9e6) SHA1(c87d1ea8731ac1e63ab960b8182dd1043bcc10bb) )

	NEO_BIOS_SOUND_128K( "NGDT-300-M1.bin", CRC(113c870f) SHA1(854425eb4be0d7fa088a6c3bf6078fdd011707f5) )

	ROM_REGION( 0x600000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "NGDT-300-V1.bin", 0x000000, 0x200000, CRC(b765bafe) SHA1(b2048c44089bf250c8dcfabb27c7981e9ee5002a) )
	ROM_LOAD( "NGDT-300-V2.bin", 0x200000, 0x200000, CRC(9fd0d559) SHA1(09e70d5e1c6e172a33f48feb3e442515c34a8f3d) )
	ROM_LOAD( "NGDT-300-V3.bin", 0x400000, 0x200000, CRC(6d5107e2) SHA1(4ba74836e3d0421a28af47d3d8341ac16af1d7d7) )

	NO_DELTAT_REGION

	ROM_REGION( 0x1000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "NGDT-300-C1.bin", 0x000000, 0x400000, CRC(53ef41b5) SHA1(a8f1fe546403b609e12f0df211c05d7ac479d98d) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "NGDT-300-C2.bin", 0x000001, 0x400000, CRC(f9b15ab3) SHA1(d8ff2f43686bfc8c2f7ead3ef445e51c15dfbf16) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "NGDT-300-C3.bin", 0x800000, 0x400000, CRC(50cc21cf) SHA1(0350aaef480c5fa12e68e540a4c974dbf5870add) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "NGDT-300-C4.bin", 0x800001, 0x400000, CRC(8486ad9e) SHA1(19a2a73c825687e0cb9fd62bde00db91b5409529)) /* Plane 2,3 */
ROM_END

/* Xeno Crisis */

ROM_START( xeno )
	ROM_REGION( 0x200000, REGION_CPU1, ROMREGION_ERASEFF )
	ROM_LOAD16_WORD_SWAP( "BB01-p1.p1", 0x000000, 0x100000, CRC(637605a6) SHA1(179ebcdeaac3e561fd7acb72022eda8c3c74cb8a) )
	ROM_LOAD16_WORD_SWAP( "BB01-p2.p2", 0x100000, 0x100000, CRC(84838145) SHA1(c1f48d333dfc85b9371f811dd449a42d8cdecf3f) )

	NEO_SFIX_128K( "BB01-s1.s1", CRC(7537ea79) SHA1(b7242a6dd7b2ad8ccf7a223c08d626abf013f366) )

	NEO_BIOS_SOUND_64K( "BB01-m1.m1", CRC(28c13ed9) SHA1(a3c8cf36906293a24f1ed49376c9d561560d2730) )

	ROM_REGION( 0x1000000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "BB01-v1.v1", 0x000000, 0x1000000, CRC(60d57867) SHA1(e1f3f759b4af4404f19dd5b75135e6968b6be3c5) )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "BB01-c1.c1", 0x000000, 0x200000, CRC(ae51ef89) SHA1(da82214263a99520364a2b7ab8140bdc68940f6d) )
	ROM_LOAD16_BYTE( "BB01-c2.c2", 0x000001, 0x200000, CRC(a8610100) SHA1(aded6eaa17a518a8f4af9c3779c41ef8dd32a316) )
ROM_END


ROM_START( pnyaa ) /* Encrypted Set */
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "267-p1.bin", 0x000000, 0x100000, CRC(112fe2c0) SHA1(01420e051f0bdbd4f68ce306a3738161b96f8ba8) )

	/* The Encrypted Boards do _not_ have an s1 rom, data for it comes from the Cx ROMs */
	ROM_REGION( 0x20000, REGION_GFX1, 0 )
	ROM_FILL( 0x000000, 0x20000, 0 )
	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "sfix.sfx", 0x000000, 0x20000, CRC(354029fc) SHA1(4ae4bf23b4c2acff875775d4cbff5583893ce2a1) )

	/* Encrypted */
	NEO_BIOS_AUDIO_ENCRYPTED_512K( "267-m1.bin", CRC(c7853ccd) SHA1(1b7a4c5093cf0fe3861ce44fd1d3b30c71ad0abe) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	/* Encrypted */
	ROM_LOAD( "267-v1.bin", 0x000000, 0x400000, CRC(e2e8e917) SHA1(7f412d55aebff3d38a225a88c632916295ab0584) )

	NO_DELTAT_REGION

	ROM_REGION( 0x1000000, REGION_GFX3, 0 )
	/* Encrypted */
  ROM_LOAD16_BYTE( "267-c1.bin", 0x0000000, 0x800000, CRC(5eebee65) SHA1(7eb3eefdeb24e19831d0f51d4ea07a0292c25ab6) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "267-c2.bin", 0x0000001, 0x800000, CRC(2b67187b) SHA1(149c3efd3c444fd0d35a97fa2268102bf76be3ed) ) /* Plane 2,3 */
ROM_END


/* this Zintrick set appears to be a bootleg made from the CD version, not a genuine
   prototype the code is based on that of the NeoCD version with some minor patches,
   for example the ADK SAMPLE TEST text that appears on the screen is actually a hacked
   PROG LOAD ERROR message.  the set is supported in order to distinguish the hacks from
   a real prototype should one turn up. */

ROM_START( zintrckb )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "zin_p1.rom", 0x000000, 0x100000, CRC(06c8fca7) SHA1(b7bf38965c3d0db4d7a9684d14cac94a45b4a45b))

	NEO_SFIX_128K( "zin_s1.rom", CRC(a7ab0e81) SHA1(f0649819b96cea79b05411e0b15c8edc677d79ba) )

	NEO_BIOS_SOUND_128K( "zin_m1.rom", CRC(fd9627ca) SHA1(b640c1f1ff466f734bb1cb5d7b589cb7e8a55346) )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "zin_v1.rom", 0x000000, 0x200000, CRC(c09f74f1) SHA1(d0b56a780a6eba85ff092240b1f1cc6718f17c21) )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "zin_c1.rom", 0x000000, 0x200000, CRC(76aee189) SHA1(ad6929804c5b9a59aa609e6baebc6aa37e858a47) )
	ROM_LOAD16_BYTE( "zin_c2.rom", 0x000001, 0x200000, CRC(844ed4b3) SHA1(fb7cd057bdc6cbe8b78097dd124118bae7402256) )
ROM_END

ROM_START( svcpcb ) /* Encrypted Set, JAMMA PCB */
	ROM_REGION( 0x2000000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "269-p1.bin", 0x000000, 0x2000000, CRC(432cfdfc) SHA1(19b40d32188a8bace6d2d570c6cf3d2f1e31e379) )

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* larger char set */
	ROM_FILL( 0x000000, 0x80000, 0 )
	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "sfix.sfx", 0x000000, 0x20000, CRC(354029fc) SHA1(4ae4bf23b4c2acff875775d4cbff5583893ce2a1) )

	ROM_REGION16_BE( 0x80000, REGION_USER1, 0 )
	/* this contains both an ASIA and JAPAN bios, HARDDIP3 on the PCB selects which to use */
	ROM_LOAD16_WORD_SWAP( "269-bios.bin", 0x00000, 0x80000, CRC(b4590283) SHA1(47047ed5b6062babc0a0bebcc30e4b3f021e115a) )

	/* Encrypted */
	NEO_BIOS_AUDIO_ENCRYPTED_512K( "269-m1.bin", CRC(f6819d00) SHA1(d3bbe09df502464f104e53501708ac6e2c1832c6) )

	ROM_REGION( 0x10000, REGION_GFX4, 0 )
	ROM_LOAD( "000-lo.lo", 0x00000, 0x10000, CRC(e09e253c) SHA1(2b1c719531dac9bb503f22644e6e4236b91e7cfc) )

	ROM_REGION( 0x1000000, REGION_SOUND1, 0 )
	/* Encrypted */
	ROM_LOAD( "269-v1.bin", 0x000000, 0x800000, CRC(c659b34c) SHA1(1931e8111ef43946f68699f8707334c96f753a1e) )
	ROM_LOAD( "269-v2.bin", 0x800000, 0x800000, CRC(dd903835) SHA1(e58d38950a7a8697bb22a1cc7a371ae6664ae8f9) )

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	/* Encrypted */
	ROM_LOAD( "269-c1.bin", 0x0000000, 0x2000000, CRC(1b608f9c) SHA1(4e70ad182da2ca18815bd3936efb04a06ebce01e) ) /* Plane 0,1 */
	ROM_LOAD( "269-c2.bin", 0x2000000, 0x2000000, CRC(5a95f294) SHA1(6123cc7b20b494076185d27c2ffea910e124b195) ) /* Plane 0,1 */
ROM_END

ROM_START( svc ) /* Encrypted Set, MVS Set */
	ROM_REGION( 0x800000, REGION_CPU1, 0 )
	ROM_LOAD32_WORD_SWAP( "269-p1c.bin", 0x000000, 0x400000, CRC(38e2005e) SHA1(1b902905916a30969282f1399a756e32ff069097) )
	ROM_LOAD32_WORD_SWAP( "269-p2c.bin", 0x000002, 0x400000, CRC(6d13797c) SHA1(3cb71a95cea6b006b44cac0f547df88aec0007b7) )

	/* The Encrypted Boards do _not_ have an s1 rom, data for it comes from the Cx ROMs */
	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* larger char set */
	ROM_FILL( 0x000000, 0x80000, 0 )
	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "sfix.sfx", 0x000000, 0x20000, CRC(354029fc) SHA1(4ae4bf23b4c2acff875775d4cbff5583893ce2a1) )

	/* Encrypted */
  NEO_BIOS_AUDIO_ENCRYPTED_512K( "269-m1.bin", CRC(f6819d00) SHA1(d3bbe09df502464f104e53501708ac6e2c1832c6) )

	ROM_REGION( 0x1000000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	/* Encrypted */
	ROM_LOAD( "269-v1c.bin", 0x000000, 0x800000, CRC(c659b34c) SHA1(1931e8111ef43946f68699f8707334c96f753a1e) )
	ROM_LOAD( "269-v2c.bin", 0x800000, 0x800000, CRC(dd903835) SHA1(e58d38950a7a8697bb22a1cc7a371ae6664ae8f9) )

	NO_DELTAT_REGION

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	/* Encrypted */
	ROM_LOAD16_BYTE( "269-c1c.bin", 0x0000000, 0x800000, CRC(887b4068) SHA1(227cdcf7a10a415f1e3afe7ae97acc9afc2cc8e1) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "269-c2c.bin", 0x0000001, 0x800000, CRC(4e8903e4) SHA1(31daaa4fd6c23e8f0a8428931c513d97d2eee1bd) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "269-c3c.bin", 0x1000000, 0x800000, CRC(7d9c55b0) SHA1(1f94a948b3e3c31b3ff05518ef525031a3cb2c62) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "269-c4c.bin", 0x1000001, 0x800000, CRC(8acb5bb6) SHA1(2c27d6e309646d7b84da85f78c06e4aaa74e844b) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "269-c5c.bin", 0x2000000, 0x800000, CRC(097a4157) SHA1(54d839f55d27f68c704a94ea3c63c644ffc22ca4) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "269-c6c.bin", 0x2000001, 0x800000, CRC(e19df344) SHA1(20448add53ab25dd3a8f0b681131ad3b9c68acc9) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "269-c7c.bin", 0x3000000, 0x800000, CRC(d8f0340b) SHA1(43114af7557361a8903bb8cf8553f602946a9220) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "269-c8c.bin", 0x3000001, 0x800000, CRC(2570b71b) SHA1(99266e1c2ffcf324793fb5c55325fbc7e6265ac0) ) /* Plane 2,3 */
ROM_END

ROM_START( svcboot )
	ROM_REGION( 0x800000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "svc-p1.bin", 0x000000, 0x800000, CRC(0348f162) SHA1(c313351d68effd92aeb80ed320e4f8c26a3bb53e) )

	ROM_REGION( 0x20000, REGION_GFX1, 0 )
	ROM_LOAD( "svc-s1.bin", 0x10000, 0x10000, CRC(70b44df1) SHA1(52ae3f264d7b33e94e770e6b2d0cf35a64e7dda4) )
	ROM_CONTINUE(			0x00000, 0x10000 )
	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "sfix.sfx",  0x000000, 0x20000, CRC(354029fc) SHA1(4ae4bf23b4c2acff875775d4cbff5583893ce2a1) )

	ROM_REGION16_BE( 0x20000, REGION_USER1, 0 )
	NEOGEO_BIOS
	
	ROM_REGION( 0x50000, REGION_CPU2, 0 )
	ROM_LOAD( "mame.sm1", 0x00000, 0x20000, CRC(97cf998b) SHA1(977387a7c76ef9b21d0b01fa69830e949a9a9626) )
	ROM_LOAD( "svc-m1.bin", 0x20000, 0x10000, CRC(804328c3) SHA1(f931636c563b0789d4812033a77b47bf663db43f) )
	ROM_CONTINUE(           0x00000, 0x10000 )
	ROM_COPY( REGION_CPU2,  0x00000, 0x10000, 0x10000 )
	
	ROM_REGION( 0x10000, REGION_GFX4, 0 )
	ROM_LOAD( "mamelo.lo", 0x00000, 0x10000, CRC(e09e253c) SHA1(2b1c719531dac9bb503f22644e6e4236b91e7cfc) )

	ROM_REGION( 0x1000000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD16_WORD_SWAP( "svc-v2.bin", 0x000000, 0x400000, CRC(b5097287) SHA1(3ba3a9b5624879616382ed40337a3d9c50a0f314) )
	ROM_LOAD16_WORD_SWAP( "svc-v1.bin", 0x400000, 0x400000, CRC(bd3a391f) SHA1(972bf09b75e99a683ee965bec93b0da8f15d72d9) )
	ROM_LOAD16_WORD_SWAP( "svc-v4.bin", 0x800000, 0x400000, CRC(33fc0b37) SHA1(d61017d829f44c7df8795ba10c55c727d9972662) )
	ROM_LOAD16_WORD_SWAP( "svc-v3.bin", 0xc00000, 0x400000, CRC(aa9849a0) SHA1(9539b3356a070a066a89f27c287f316e7367ce2a) )

	NO_DELTAT_REGION

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "svc-c1.bin", 0x0000000, 0x800000, CRC(a7826b89) SHA1(3bbe348ce54b80b56ef032ea532a18ef3cafeb11) )
	ROM_LOAD16_BYTE( "svc-c2.bin", 0x0000001, 0x800000, CRC(ed3c2089) SHA1(b5d17692f15f5a678c273589fab2e3918711135e) )
	ROM_LOAD16_BYTE( "svc-c3.bin", 0x1000000, 0x800000, CRC(71ed8063) SHA1(ea1df9e2e382a8560a06d447421844cc588f43dd) )
	ROM_LOAD16_BYTE( "svc-c4.bin", 0x1000001, 0x800000, CRC(250bde2d) SHA1(8c72dcfceef6d022ab4b73ab37cf3ac0c3940c17) )
	ROM_LOAD16_BYTE( "svc-c5.bin", 0x2000000, 0x800000, CRC(9817c082) SHA1(1bea9c7220c2b1524896c86841d6d8fd55f5d366) )
	ROM_LOAD16_BYTE( "svc-c6.bin", 0x2000001, 0x800000, CRC(2bc0307f) SHA1(8090fa82c46eb503832359093c8cc3cee3141c90) )
	ROM_LOAD16_BYTE( "svc-c7.bin", 0x3000000, 0x800000, CRC(4358d7b9) SHA1(9270b58c2abc072a046bedda72f1395df26d0714) )
	ROM_LOAD16_BYTE( "svc-c8.bin", 0x3000001, 0x800000, CRC(366deee5) SHA1(d477ad7a5987fd6c7ef2c1680fbb7c884654590e) )
ROM_END

ROM_START( svcplus )
	ROM_REGION( 0x600000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "svc-p1p.bin", 0x000000, 0x200000, CRC(a194d842) SHA1(72b7bfa34a97632b1aa003488e074d766a6c2f08) )
	ROM_LOAD16_WORD_SWAP( "svc-p2p.bin", 0x200000, 0x200000, CRC(50c0e2b7) SHA1(97b396415ab0e692e43ddf371091e5a456712f0a) )
	ROM_LOAD16_WORD_SWAP( "svc-p3p.bin", 0x400000, 0x200000, CRC(58cdc293) SHA1(3c4f2418ec513bcc13ed33a727de11dfb98f7525) )

	NEO_SFIX_128K( "svc-s1p.bin", CRC(73344711) SHA1(04d84c4fe241b9135cd210f8ed8c725f595d11d2) )

	ROM_REGION16_BE( 0x20000, REGION_USER1, 0 )
	NEOGEO_BIOS
	ROM_REGION( 0x50000, REGION_CPU2, 0 )
	ROM_LOAD( "sm1.sm1", 0x00000, 0x20000, CRC(97cf998b) SHA1(977387a7c76ef9b21d0b01fa69830e949a9a9626) )
	ROM_LOAD( "svc-m1.bin", 0x20000, 0x10000, CRC(804328c3) SHA1(f931636c563b0789d4812033a77b47bf663db43f) )
	ROM_CONTINUE(           0x00000, 0x10000 )
	ROM_COPY( REGION_CPU2,  0x00000, 0x10000, 0x10000 )
	ROM_REGION( 0x10000, REGION_GFX4, 0 )
	ROM_LOAD( "000-lo.lo", 0x00000, 0x10000, CRC(e09e253c) SHA1(2b1c719531dac9bb503f22644e6e4236b91e7cfc) )

	ROM_REGION( 0x1000000, REGION_SOUND1, 0 )
	ROM_LOAD16_WORD_SWAP( "svc-v2.bin", 0x000000, 0x400000, CRC(b5097287) SHA1(3ba3a9b5624879616382ed40337a3d9c50a0f314) )
	ROM_LOAD16_WORD_SWAP( "svc-v1.bin", 0x400000, 0x400000, CRC(bd3a391f) SHA1(972bf09b75e99a683ee965bec93b0da8f15d72d9) )
	ROM_LOAD16_WORD_SWAP( "svc-v4.bin", 0x800000, 0x400000, CRC(33fc0b37) SHA1(d61017d829f44c7df8795ba10c55c727d9972662) )
	ROM_LOAD16_WORD_SWAP( "svc-v3.bin", 0xc00000, 0x400000, CRC(aa9849a0) SHA1(9539b3356a070a066a89f27c287f316e7367ce2a) )

	NO_DELTAT_REGION

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "svc-c1.bin", 0x0000000, 0x800000, CRC(a7826b89) SHA1(3bbe348ce54b80b56ef032ea532a18ef3cafeb11) )
	ROM_LOAD16_BYTE( "svc-c2.bin", 0x0000001, 0x800000, CRC(ed3c2089) SHA1(b5d17692f15f5a678c273589fab2e3918711135e) )
	ROM_LOAD16_BYTE( "svc-c3.bin", 0x1000000, 0x800000, CRC(71ed8063) SHA1(ea1df9e2e382a8560a06d447421844cc588f43dd) )
	ROM_LOAD16_BYTE( "svc-c4.bin", 0x1000001, 0x800000, CRC(250bde2d) SHA1(8c72dcfceef6d022ab4b73ab37cf3ac0c3940c17) )
	ROM_LOAD16_BYTE( "svc-c5.bin", 0x2000000, 0x800000, CRC(9817c082) SHA1(1bea9c7220c2b1524896c86841d6d8fd55f5d366) )
	ROM_LOAD16_BYTE( "svc-c6.bin", 0x2000001, 0x800000, CRC(2bc0307f) SHA1(8090fa82c46eb503832359093c8cc3cee3141c90) )
	ROM_LOAD16_BYTE( "svc-c7.bin", 0x3000000, 0x800000, CRC(4358d7b9) SHA1(9270b58c2abc072a046bedda72f1395df26d0714) )
	ROM_LOAD16_BYTE( "svc-c8.bin", 0x3000001, 0x800000, CRC(366deee5) SHA1(d477ad7a5987fd6c7ef2c1680fbb7c884654590e) )
ROM_END



ROM_START( svcplusa )
	ROM_REGION( 0x600000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "svc-p1pl.bin", 0x000000, 0x200000, CRC(16b44144) SHA1(5eab530274b1b6f480a39a86c199da524cddfccc) )
	ROM_LOAD16_WORD_SWAP( "svc-p2pl.bin", 0x200000, 0x400000, CRC(7231ace2) SHA1(d2f13ddd5d3ee29b4b9824e8663f7ee0241f30cf) )

	ROM_REGION( 0x20000, REGION_GFX1, 0 )
	ROM_LOAD( "svc-s1pl.bin", 0x10000, 0x10000, CRC(ca3c735e) SHA1(aebd15253c90432a2e0a4c40f37110c1e2176ee4) )
	ROM_CONTINUE(			0x00000, 0x10000 )
	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "sfix.sfx",  0x000000, 0x20000, CRC(354029fc) SHA1(4ae4bf23b4c2acff875775d4cbff5583893ce2a1) )

	ROM_REGION16_BE( 0x20000, REGION_USER1, 0 )
	NEOGEO_BIOS
	ROM_REGION( 0x50000, REGION_CPU2, 0 )
	ROM_LOAD( "sm1.sm1", 0x00000, 0x20000, CRC(97cf998b) SHA1(977387a7c76ef9b21d0b01fa69830e949a9a9626) )
	ROM_LOAD( "svc-m1.bin", 0x20000, 0x10000, CRC(804328c3) SHA1(f931636c563b0789d4812033a77b47bf663db43f) )
	ROM_CONTINUE(           0x00000, 0x10000 )
	ROM_COPY( REGION_CPU2,  0x00000, 0x10000, 0x10000 )
	ROM_REGION( 0x10000, REGION_GFX4, 0 )
	ROM_LOAD( "000-lo.lo", 0x00000, 0x10000, CRC(e09e253c) SHA1(2b1c719531dac9bb503f22644e6e4236b91e7cfc) )

	ROM_REGION( 0x1000000, REGION_SOUND1, 0 )
	ROM_LOAD16_WORD_SWAP( "svc-v2.bin", 0x000000, 0x400000, CRC(b5097287) SHA1(3ba3a9b5624879616382ed40337a3d9c50a0f314) )
	ROM_LOAD16_WORD_SWAP( "svc-v1.bin", 0x400000, 0x400000, CRC(bd3a391f) SHA1(972bf09b75e99a683ee965bec93b0da8f15d72d9) )
	ROM_LOAD16_WORD_SWAP( "svc-v4.bin", 0x800000, 0x400000, CRC(33fc0b37) SHA1(d61017d829f44c7df8795ba10c55c727d9972662) )
	ROM_LOAD16_WORD_SWAP( "svc-v3.bin", 0xc00000, 0x400000, CRC(aa9849a0) SHA1(9539b3356a070a066a89f27c287f316e7367ce2a) )

	NO_DELTAT_REGION

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "svc-c1.bin", 0x0000000, 0x800000, CRC(a7826b89) SHA1(3bbe348ce54b80b56ef032ea532a18ef3cafeb11) )
	ROM_LOAD16_BYTE( "svc-c2.bin", 0x0000001, 0x800000, CRC(ed3c2089) SHA1(b5d17692f15f5a678c273589fab2e3918711135e) )
	ROM_LOAD16_BYTE( "svc-c3.bin", 0x1000000, 0x800000, CRC(71ed8063) SHA1(ea1df9e2e382a8560a06d447421844cc588f43dd) )
	ROM_LOAD16_BYTE( "svc-c4.bin", 0x1000001, 0x800000, CRC(250bde2d) SHA1(8c72dcfceef6d022ab4b73ab37cf3ac0c3940c17) )
	ROM_LOAD16_BYTE( "svc-c5.bin", 0x2000000, 0x800000, CRC(9817c082) SHA1(1bea9c7220c2b1524896c86841d6d8fd55f5d366) )
	ROM_LOAD16_BYTE( "svc-c6.bin", 0x2000001, 0x800000, CRC(2bc0307f) SHA1(8090fa82c46eb503832359093c8cc3cee3141c90) )
	ROM_LOAD16_BYTE( "svc-c7.bin", 0x3000000, 0x800000, CRC(4358d7b9) SHA1(9270b58c2abc072a046bedda72f1395df26d0714) )
	ROM_LOAD16_BYTE( "svc-c8.bin", 0x3000001, 0x800000, CRC(366deee5) SHA1(d477ad7a5987fd6c7ef2c1680fbb7c884654590e) )
ROM_END

ROM_START( svcsplus )
	ROM_REGION( 0x800000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "svc-p1sp.bin", 0x000000, 0x400000, CRC(2601902f) SHA1(202348a13c6480f7de37a3ee983823838822fc98) )
	ROM_LOAD16_WORD_SWAP( "svc-p2sp.bin", 0x400000, 0x400000, CRC(0ca13305) SHA1(ac8fbca71b754acbcdd11802161a62ae1cf32d88) )

	NEO_SFIX_128K( "svc-s1sp.bin", CRC(233d6439) SHA1(369024c7a2405c3144c14ac016c07c3dc0f44187) )

	ROM_REGION16_BE( 0x20000, REGION_USER1, 0 )
	NEOGEO_BIOS
	ROM_REGION( 0x50000, REGION_CPU2, 0 )
	ROM_LOAD( "sm1.sm1", 0x00000, 0x20000, CRC(97cf998b) SHA1(977387a7c76ef9b21d0b01fa69830e949a9a9626) )
	ROM_LOAD( "svc-m1.bin", 0x20000, 0x10000, CRC(804328c3) SHA1(f931636c563b0789d4812033a77b47bf663db43f) )
	ROM_CONTINUE(           0x00000, 0x10000 )
	ROM_COPY( REGION_CPU2,  0x00000, 0x10000, 0x10000 )
	ROM_REGION( 0x10000, REGION_GFX4, 0 )
	ROM_LOAD( "000-lo.lo", 0x00000, 0x10000, CRC(e09e253c) SHA1(2b1c719531dac9bb503f22644e6e4236b91e7cfc) )

	ROM_REGION( 0x1000000, REGION_SOUND1, 0 )
	ROM_LOAD16_WORD_SWAP( "svc-v2.bin", 0x000000, 0x400000, CRC(b5097287) SHA1(3ba3a9b5624879616382ed40337a3d9c50a0f314) )
	ROM_LOAD16_WORD_SWAP( "svc-v1.bin", 0x400000, 0x400000, CRC(bd3a391f) SHA1(972bf09b75e99a683ee965bec93b0da8f15d72d9) )
	ROM_LOAD16_WORD_SWAP( "svc-v4.bin", 0x800000, 0x400000, CRC(33fc0b37) SHA1(d61017d829f44c7df8795ba10c55c727d9972662) )
	ROM_LOAD16_WORD_SWAP( "svc-v3.bin", 0xc00000, 0x400000, CRC(aa9849a0) SHA1(9539b3356a070a066a89f27c287f316e7367ce2a) )
	NO_DELTAT_REGION
	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "svc-c1.bin", 0x0000000, 0x800000, CRC(a7826b89) SHA1(3bbe348ce54b80b56ef032ea532a18ef3cafeb11) )
	ROM_LOAD16_BYTE( "svc-c2.bin", 0x0000001, 0x800000, CRC(ed3c2089) SHA1(b5d17692f15f5a678c273589fab2e3918711135e) )
	ROM_LOAD16_BYTE( "svc-c3.bin", 0x1000000, 0x800000, CRC(71ed8063) SHA1(ea1df9e2e382a8560a06d447421844cc588f43dd) )
	ROM_LOAD16_BYTE( "svc-c4.bin", 0x1000001, 0x800000, CRC(250bde2d) SHA1(8c72dcfceef6d022ab4b73ab37cf3ac0c3940c17) )
	ROM_LOAD16_BYTE( "svc-c5.bin", 0x2000000, 0x800000, CRC(9817c082) SHA1(1bea9c7220c2b1524896c86841d6d8fd55f5d366) )
	ROM_LOAD16_BYTE( "svc-c6.bin", 0x2000001, 0x800000, CRC(2bc0307f) SHA1(8090fa82c46eb503832359093c8cc3cee3141c90) )
	ROM_LOAD16_BYTE( "svc-c7.bin", 0x3000000, 0x800000, CRC(4358d7b9) SHA1(9270b58c2abc072a046bedda72f1395df26d0714) )
	ROM_LOAD16_BYTE( "svc-c8.bin", 0x3000001, 0x800000, CRC(366deee5) SHA1(d477ad7a5987fd6c7ef2c1680fbb7c884654590e) )
ROM_END

ROM_START( samsho5 ) /* Encrypted Set */
	ROM_REGION( 0x800000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "270-p1.bin", 0x000000, 0x400000, CRC(4a2a09e6) SHA1(2644de02cdab8ccc605488a7c76b8c9cd1d5bcb9) )
	ROM_LOAD16_WORD_SWAP( "270-p2.bin", 0x400000, 0x400000, CRC(e0c74c85) SHA1(df24a4ee76438e40c2f04a714175a7f85cacdfe0) )

	/* The Encrypted Boards do _not_ have an s1 rom, data for it comes from the Cx ROMs */
	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* larger char set */
	ROM_FILL( 0x000000, 0x80000, 0 )
	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "sfix.sfx", 0x000000, 0x20000, CRC(354029fc) SHA1(4ae4bf23b4c2acff875775d4cbff5583893ce2a1) )

	/* Encrypted */
	NEO_BIOS_AUDIO_ENCRYPTED_512K( "270-m1.bin", CRC(49c9901a) SHA1(2623e9765a0eba58fee2de72851e9dc502344a3d) )

	ROM_REGION( 0x1000000, REGION_SOUND1, 0 )
	/* Encrypted */
	ROM_LOAD( "270-v1.bin", 0x000000, 0x800000, CRC(62e434eb) SHA1(1985f5e88f8e866f9683b6cea901aa28c04b80bf) )
	ROM_LOAD( "270-v2.bin", 0x800000, 0x800000, CRC(180f3c9a) SHA1(6d7dc2605ead6e78704efa127e7e0dfe621e2c54) )

	NO_DELTAT_REGION

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	/* Encrypted */
	ROM_LOAD16_BYTE( "270-c1.bin", 0x0000000, 0x800000, CRC(14ffffac) SHA1(2ccebfdd0c7907679ae95bf6eca85b8d322441e2) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "270-c2.bin", 0x0000001, 0x800000, CRC(401f7299) SHA1(94e48cdf1682b1250f53c59f3f71d995e928d17b) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "270-c3.bin", 0x1000000, 0x800000, CRC(838f0260) SHA1(d5c8d3c6e7221d04e0b20882a847752e5ba95635) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "270-c4.bin", 0x1000001, 0x800000, CRC(041560a5) SHA1(d165e533699f15b1e079c82f97db3542b3a7dd66) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "270-c5.bin", 0x2000000, 0x800000, CRC(bd30b52d) SHA1(9f8282e684415b4045218cf764ef7d75a70e3240) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "270-c6.bin", 0x2000001, 0x800000, CRC(86a69c70) SHA1(526732cdb408cf680af9da39057bce6a4dfb5e13) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "270-c7.bin", 0x3000000, 0x800000, CRC(d28fbc3c) SHA1(a82a6ba6760fad14d9309f9147cb7d80bd6f70fc) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "270-c8.bin", 0x3000001, 0x800000, CRC(02c530a6) SHA1(7a3fafa6075506c6ef78cc4ec2cb72118ec83cb9) ) /* Plane 2,3 */
ROM_END

ROM_START( samsh5sp ) /* Encrypted Set */
	ROM_REGION( 0x800000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "272-p1.bin", 0x000000, 0x400000, CRC(fb7a6bba) SHA1(f68c527208d8a55ca44b0caaa8ab66b3a0ffdfe5) )
	ROM_LOAD16_WORD_SWAP( "272-p2.bin", 0x400000, 0x400000, CRC(63492ea6) SHA1(6ba946acb62c63ed61a42fe72b7fff3828883bcc) )

	/* The Encrypted Boards do _not_ have an s1 rom, data for it comes from the Cx ROMs */
	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* larger char set */
	ROM_FILL( 0x000000, 0x80000, 0 )
	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "sfix.sfx", 0x000000, 0x20000, CRC(354029fc) SHA1(4ae4bf23b4c2acff875775d4cbff5583893ce2a1) )

	/* Encrypted */
	NEO_BIOS_AUDIO_ENCRYPTED_512K( "272-m1.bin", CRC(adeebf40) SHA1(8cbd63dda3fff4de38060405bf70cd9308c9e66e) )


	ROM_REGION( 0x1000000, REGION_SOUND1, 0 )
	/* Encrypted */
	ROM_LOAD( "272-v1.bin", 0x000000, 0x800000, CRC(76a94127) SHA1(c3affd7ff1eb02345cfb755962ec173a8ec34acd) )
	ROM_LOAD( "272-v2.bin", 0x800000, 0x800000, CRC(4ba507f1) SHA1(728d139da3fe8a391fd8be4d24bb7fdd4bf9548a) )

	NO_DELTAT_REGION

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	/* Encrypted */
	ROM_LOAD16_BYTE( "272-c1.bin", 0x0000000, 0x800000, CRC(4f97661a) SHA1(87f1721bae5ef16bc23c06b05e64686c396413df) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "272-c2.bin", 0x0000001, 0x800000, CRC(a3afda4f) SHA1(86b475fce0bc0aa04d34e31324e8c7c7c847df19) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "272-c3.bin", 0x1000000, 0x800000, CRC(8c3c7502) SHA1(6639020a8860d2400308e110d7277cbaf6eccc2a) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "272-c4.bin", 0x1000001, 0x800000, CRC(32d5e2e2) SHA1(2b5612017152afd7433aaf99951a084ef5ad6bf0) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "272-c5.bin", 0x2000000, 0x800000, CRC(6ce085bc) SHA1(0432b04a2265c649bba1bbd934dfb425c5d80fb1) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "272-c6.bin", 0x2000001, 0x800000, CRC(05c8dc8e) SHA1(da45c222893f25495a66bdb302f9b0b1de3c8ae0) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "272-c7.bin", 0x3000000, 0x800000, CRC(1417b742) SHA1(dfe35eb4bcd022d2f2dc544ccbbb77078f08c0aa) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "272-c8.bin", 0x3000001, 0x800000, CRC(d49773cd) SHA1(cd8cf3b762d381c1f8f12919579c84a7ef7efb3f) ) /* Plane 2,3 */
ROM_END

ROM_START( mslug5 ) /* Encrypted Set */
	ROM_REGION( 0x800000, REGION_CPU1, 0 )
	ROM_LOAD32_WORD_SWAP( "268-p1cr.bin", 0x000000, 0x400000, CRC(d0466792) SHA1(880819933d997fab398f91061e9dbccb959ae8a1) )
	ROM_LOAD32_WORD_SWAP( "268-p2cr.bin", 0x000002, 0x400000, CRC(fbf6b61e) SHA1(9ec743d5988b5e3183f37f8edf45c72a8c0c893e) )

	/* The Encrypted Boards do _not_ have an s1 rom, data for it comes from the Cx ROMs */
	ROM_REGION( 0x20000, REGION_GFX1, 0 )
	ROM_FILL( 0x000000, 0x20000, 0 )
	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "sfix.sfx", 0x000000, 0x20000, CRC(354029fc) SHA1(4ae4bf23b4c2acff875775d4cbff5583893ce2a1) )

	/* Encrypted */
	NEO_BIOS_AUDIO_ENCRYPTED_512K( "268-m1.bin", CRC(4a5a6e0e) SHA1(df0f660f2465e1db7be5adfcaf5e88ad61a74a42) )

	ROM_REGION( 0x1000000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	/* Encrypted */
	ROM_LOAD( "268-v1c.bin", 0x000000, 0x800000, CRC(ae31d60c) SHA1(c42285cf4e52fea74247860813e826df5aa7600a) )
	ROM_LOAD( "268-v2c.bin", 0x800000, 0x800000, CRC(c40613ed) SHA1(af889570304e2867d7dfea1e94e388c06249fb67) )

	NO_DELTAT_REGION

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	/* Encrypted */
	ROM_LOAD16_BYTE( "268-c1c.bin", 0x0000000, 0x800000, CRC(ab7c389a) SHA1(025a188de589500bf7637fa8e7a37ab24bf4312e) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "268-c2c.bin", 0x0000001, 0x800000, CRC(3560881b) SHA1(493d218c92290b4770024d6ee2917c4022753b07) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "268-c3c.bin", 0x1000000, 0x800000, CRC(3af955ea) SHA1(cf36b6ae9b0d12744b17cb7a928399214de894be) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "268-c4c.bin", 0x1000001, 0x800000, CRC(c329c373) SHA1(5073d4079958a0ef5426885af2c9e3178f37d5e0) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "268-c5c.bin", 0x2000000, 0x800000, CRC(959c8177) SHA1(889bda7c65d71172e7d89194d1269561888fe789) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "268-c6c.bin", 0x2000001, 0x800000, CRC(010a831b) SHA1(aec140661e3ae35d264df416478ba15188544d91) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "268-c7c.bin", 0x3000000, 0x800000, CRC(6d72a969) SHA1(968dd9a4d1209b770b9b85ea6532fa24d262a262) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "268-c8c.bin", 0x3000001, 0x800000, CRC(551d720e) SHA1(ebf69e334fcaba0fda6fd432fd0970283a365d12) ) /* Plane 2,3 */
ROM_END

ROM_START( kof2k4se )
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "k2k4s-p2.bin", 0x000000, 0x080000, CRC(21a84084) SHA1(973e8a0bffa0e1f055803f663f81a8e03701802d) )
	ROM_LOAD16_WORD_SWAP( "k2k4s-p3.bin", 0x080000, 0x080000, CRC(febb484e) SHA1(4b1838795b84f22d578ad043641df0a7bf7d9774) )
	ROM_LOAD16_WORD_SWAP( "k2k4s-p1.bin", 0x100000, 0x400000, CRC(e6c50566) SHA1(cc6a3489a3bfeb4dcc65b6ddae0030f7e66fbabe) )

	NEO_SFIX_128K( "k2k4s-s1.bin", CRC(a3c9b2d8) SHA1(1472d2cbd7bb73e84824ecf773924007e6117e77) )

	NEO_BIOS_SOUND_128K( "k2k4s-m1.bin", CRC(5a47d9ad) SHA1(0197737934653acc6c97221660d789e9914f3578) )

	ROM_REGION( 0x1000000, REGION_SOUND1, 0 )
	ROM_LOAD( "k2k4s-v2.bin", 0x000000, 0x800000, CRC(e4ddfb3f) SHA1(eb8220ab01c16cf9244b7f3f9912bec0db561b85) )
	ROM_LOAD( "k2k4s-v1.bin", 0x800000, 0x800000, CRC(b887d287) SHA1(f593a5722df6f6fac023d189a739a117e976bb2f) )

	NO_DELTAT_REGION

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "k2k4s-c4.bin", 0x0000000, 0x800000, CRC(7a050288) SHA1(55a20c5b01e11a859f096af3f8e09986025d288f) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "k2k4s-c8.bin", 0x0000001, 0x800000, CRC(e924afcf) SHA1(651e974f7339d2cdcfa58c5398013197a0525b77) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "k2k4s-c3.bin", 0x1000000, 0x800000, CRC(959fad0b) SHA1(63ab83ddc5f688dc8165a7ff8d262df3fcd942a2) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "k2k4s-c7.bin", 0x1000001, 0x800000, CRC(efe6a468) SHA1(2a414285e48aa948b5b0d4a9333bab083b5fb853) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "k2k4s-c2.bin", 0x2000000, 0x800000, CRC(74bba7c6) SHA1(e01adc7a4633bc0951b9b4f09abc07d728e9a2d9) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "k2k4s-c6.bin", 0x2000001, 0x800000, CRC(e20d2216) SHA1(5d28eea7b581e780b78f391a8179f1678ee0d9a5) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "k2k4s-c1.bin", 0x3000000, 0x800000, CRC(fa705b2b) SHA1(f314c66876589601806352484dd8e45bc41be692) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "k2k4s-c5.bin", 0x3000001, 0x800000, CRC(2c912ff9) SHA1(b624a625ea3e221808b7ea43fb0b1a51d8c1853e) ) /* Plane 2,3 */
ROM_END

ROM_START( kof10th )
  ROM_REGION( 0x900000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "kf10-p1.bin", 0x000000, 0x800000, CRC(b1fd0c43) SHA1(5f842a8a27be2d957fd4140d6431ae47154997bb) )

	ROM_REGION( 0x10000, REGION_GFX4, 0 )
	ROM_LOAD( "mamelo.lo", 0x00000, 0x10000, CRC(e09e253c) SHA1(2b1c719531dac9bb503f22644e6e4236b91e7cfc) )  /* Y zoom control */

	ROM_REGION( 0x40000, REGION_GFX1, 0 )
	ROM_FILL( 0x000000, 0x40000, 0 )
	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "sfix.sfx", 0x000000, 0x20000, CRC(354029fc) SHA1(4ae4bf23b4c2acff875775d4cbff5583893ce2a1) )

	NEO_BIOS_SOUND_128K( "kf10-m1.bin", CRC(f6fab859) SHA1(0184aa1394b9f9946d610278b53b846020dd88dc) )

	ROM_REGION( 0x1000000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "kf10-v1.bin", 0x000000, 0x800000, CRC(0fc9a58d) SHA1(9d79ef00e2c2abd9f29af5521c2fbe5798bf336f) )
	ROM_LOAD( "kf10-v2.bin", 0x800000, 0x800000, CRC(b8c475a4) SHA1(10caf9c69927a223445d2c4b147864c02ce520a8) )

	NO_DELTAT_REGION

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "kf10-c1a.bin", 0x0000000, 0x400000, CRC(3bbc0364) SHA1(e8aa7ff82f151ce1db56f259377b64cceef85af0) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "kf10-c2a.bin", 0x0000001, 0x400000, CRC(91230075) SHA1(d9098e05a7ba6008661147b6bf8bc2f494b8b72b) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "kf10-c1b.bin", 0x0800000, 0x400000, CRC(b5abfc28) SHA1(eabf60992bb3485c95330065294071ec155bfe7c) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "kf10-c2b.bin", 0x0800001, 0x400000, CRC(6cc4c6e1) SHA1(be824a944e745ee18efdc45c81fd496a4d624b9c) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "kf10-c3a.bin", 0x1000000, 0x400000, CRC(5b3d4a16) SHA1(93ac1cd7739100f8c32732644f81f2a19837b131) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "kf10-c4a.bin", 0x1000001, 0x400000, CRC(c6f3419b) SHA1(340c17a73aeb7bf8a6209f8459e6f00000075b50) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "kf10-c3b.bin", 0x1800000, 0x400000, CRC(9d2bba19) SHA1(5ebbd0af3f83a60e33c8ccb743e3d5f5a96f1273) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "kf10-c4b.bin", 0x1800001, 0x400000, CRC(5a4050cb) SHA1(8fd2291f349efa1ed5cd37ad4e273b60fe831a77) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "kf10-c5a.bin", 0x2000000, 0x400000, CRC(a289d1e1) SHA1(50c7d7ebde6e118a01036cc3e40827fcd9f0d3fd) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "kf10-c6a.bin", 0x2000001, 0x400000, CRC(e6494b5d) SHA1(18e064b9867ae0b0794065f8dbefd486620419db) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "kf10-c5b.bin", 0x2800000, 0x400000, CRC(404fff02) SHA1(56d1b32c87ea4885e49264e8b21846e465a20e1f) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "kf10-c6b.bin", 0x2800001, 0x400000, CRC(f2ccfc9e) SHA1(69db7fac7023785ab94ea711a72dbc2826cfe1a3) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "kf10-c7a.bin", 0x3000000, 0x400000, CRC(be79c5a8) SHA1(ded3c5eb3571647f50533eb682c2675372ace3fb) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "kf10-c8a.bin", 0x3000001, 0x400000, CRC(a5952ca4) SHA1(76dbb3cb45ce5a4beffa1ed29491204fc6617e42) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "kf10-c7b.bin", 0x3800000, 0x400000, CRC(3fdb3542) SHA1(7d2050752a2064cd6729f483a0da93808e2c6033) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "kf10-c8b.bin", 0x3800001, 0x400000, CRC(661b7a52) SHA1(0ae2ad2389134892f156337332b77adade3ddad1) ) /* Plane 2,3 */
ROM_END

ROM_START( kf10thep ) /* this is a hack of kof2002 much like the various korean hacks / bootlegs of games */
	ROM_REGION( 0x800000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "5008-p2.rom", 0x100000, 0x400000, CRC(a649ec38) SHA1(5c63ed5e5c848940f587c966da4908d04cf1293c) )
	ROM_LOAD16_WORD_SWAP( "5008-p3.rom", 0x500000, 0x200000, CRC(e629e13c) SHA1(6ebe080ce01c51064cb2f4d89315ba98a45ae727) )

	ROM_REGION( 0x200000, REGION_USER4, 0 )
	ROM_LOAD( "5008-p1.rom", 0x000000, 0x200000, CRC(bf5469ba) SHA1(f05236d8fffab5836c0d27becdeeb80def32ee49) )

	NEO_SFIX_128K( "5008-s1.rom", CRC(92410064) SHA1(1fb800b46341858207d3b6961a760289fbec7faa) )

	NEO_BIOS_SOUND_128K( "5008-m1.rom", CRC(5a47d9ad) SHA1(0197737934653acc6c97221660d789e9914f3578) )

	ROM_REGION( 0x1000000, REGION_SOUND1, 0 )
	ROM_LOAD( "kf10-v1.bin", 0x000000, 0x800000, CRC(0fc9a58d) SHA1(9d79ef00e2c2abd9f29af5521c2fbe5798bf336f) )
	ROM_LOAD( "kf10-v2.bin", 0x800000, 0x800000, CRC(b8c475a4) SHA1(10caf9c69927a223445d2c4b147864c02ce520a8) )

	NO_DELTAT_REGION

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "kf10-c1a.bin", 0x0000000, 0x400000, CRC(3bbc0364) SHA1(e8aa7ff82f151ce1db56f259377b64cceef85af0) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "kf10-c2a.bin", 0x0000001, 0x400000, CRC(91230075) SHA1(d9098e05a7ba6008661147b6bf8bc2f494b8b72b) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "kf10-c1b.bin", 0x0800000, 0x400000, CRC(b5abfc28) SHA1(eabf60992bb3485c95330065294071ec155bfe7c) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "kf10-c2b.bin", 0x0800001, 0x400000, CRC(6cc4c6e1) SHA1(be824a944e745ee18efdc45c81fd496a4d624b9c) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "kf10-c3a.bin", 0x1000000, 0x400000, CRC(5b3d4a16) SHA1(93ac1cd7739100f8c32732644f81f2a19837b131) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "kf10-c4a.bin", 0x1000001, 0x400000, CRC(c6f3419b) SHA1(340c17a73aeb7bf8a6209f8459e6f00000075b50) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "kf10-c3b.bin", 0x1800000, 0x400000, CRC(9d2bba19) SHA1(5ebbd0af3f83a60e33c8ccb743e3d5f5a96f1273) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "kf10-c4b.bin", 0x1800001, 0x400000, CRC(5a4050cb) SHA1(8fd2291f349efa1ed5cd37ad4e273b60fe831a77) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "kf10-c5a.bin", 0x2000000, 0x400000, CRC(a289d1e1) SHA1(50c7d7ebde6e118a01036cc3e40827fcd9f0d3fd) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "kf10-c6a.bin", 0x2000001, 0x400000, CRC(e6494b5d) SHA1(18e064b9867ae0b0794065f8dbefd486620419db) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "kf10-c5b.bin", 0x2800000, 0x400000, CRC(404fff02) SHA1(56d1b32c87ea4885e49264e8b21846e465a20e1f) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "kf10-c6b.bin", 0x2800001, 0x400000, CRC(f2ccfc9e) SHA1(69db7fac7023785ab94ea711a72dbc2826cfe1a3) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "kf10-c7a.bin", 0x3000000, 0x400000, CRC(be79c5a8) SHA1(ded3c5eb3571647f50533eb682c2675372ace3fb) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "kf10-c8a.bin", 0x3000001, 0x400000, CRC(a5952ca4) SHA1(76dbb3cb45ce5a4beffa1ed29491204fc6617e42) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "5008-c7b.rom", 0x3800000, 0x400000, CRC(33604ef0) SHA1(57deec23c81d5d673ce5992cef1f2567f1a2148e) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "5008-c8b.rom", 0x3800001, 0x400000, CRC(51f6a8f8) SHA1(9ef1cdbdd125a2b430346c22b59f36902312905f) ) /* Plane 2,3 */
ROM_END



ROM_START( kf2k5uni )
	ROM_REGION( 0x800000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "5006-p2a.bin", 0x000000, 0x400000, CRC(ced883a2) )
	ROM_LOAD16_WORD_SWAP( "5006-p1.bin", 0x400000, 0x400000, CRC(72c39c46) )

	NEO_SFIX_128K( "5006-s1.bin", CRC(91f8c544) )

	NEO_BIOS_SOUND_128K( "5006-m1.bin", CRC(9050bfe7) )

	ROM_REGION( 0x1000000, REGION_SOUND1, 0 )
	ROM_LOAD( "kf10-v1.bin", 0x000000, 0x800000, CRC(0fc9a58d) SHA1(9d79ef00e2c2abd9f29af5521c2fbe5798bf336f) )
	ROM_LOAD( "kf10-v2.bin", 0x800000, 0x800000, CRC(b8c475a4) SHA1(10caf9c69927a223445d2c4b147864c02ce520a8) )

	ROM_REGION( 0x4000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "kf10-c1a.bin", 0x0000000, 0x400000, CRC(3bbc0364) SHA1(e8aa7ff82f151ce1db56f259377b64cceef85af0) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "kf10-c2a.bin", 0x0000001, 0x400000, CRC(91230075) SHA1(d9098e05a7ba6008661147b6bf8bc2f494b8b72b) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "kf10-c1b.bin", 0x0800000, 0x400000, CRC(b5abfc28) SHA1(eabf60992bb3485c95330065294071ec155bfe7c) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "kf10-c2b.bin", 0x0800001, 0x400000, CRC(6cc4c6e1) SHA1(be824a944e745ee18efdc45c81fd496a4d624b9c) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "kf10-c3a.bin", 0x1000000, 0x400000, CRC(5b3d4a16) SHA1(93ac1cd7739100f8c32732644f81f2a19837b131) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "kf10-c4a.bin", 0x1000001, 0x400000, CRC(c6f3419b) SHA1(340c17a73aeb7bf8a6209f8459e6f00000075b50) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "kf10-c3b.bin", 0x1800000, 0x400000, CRC(9d2bba19) SHA1(5ebbd0af3f83a60e33c8ccb743e3d5f5a96f1273) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "kf10-c4b.bin", 0x1800001, 0x400000, CRC(5a4050cb) SHA1(8fd2291f349efa1ed5cd37ad4e273b60fe831a77) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "kf10-c5a.bin", 0x2000000, 0x400000, CRC(a289d1e1) SHA1(50c7d7ebde6e118a01036cc3e40827fcd9f0d3fd) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "kf10-c6a.bin", 0x2000001, 0x400000, CRC(e6494b5d) SHA1(18e064b9867ae0b0794065f8dbefd486620419db) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "kf10-c5b.bin", 0x2800000, 0x400000, CRC(404fff02) SHA1(56d1b32c87ea4885e49264e8b21846e465a20e1f) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "kf10-c6b.bin", 0x2800001, 0x400000, CRC(f2ccfc9e) SHA1(69db7fac7023785ab94ea711a72dbc2826cfe1a3) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "kf10-c7a.bin", 0x3000000, 0x400000, CRC(be79c5a8) SHA1(ded3c5eb3571647f50533eb682c2675372ace3fb) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "kf10-c8a.bin", 0x3000001, 0x400000, CRC(a5952ca4) SHA1(76dbb3cb45ce5a4beffa1ed29491204fc6617e42) ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "kf10-c7b.bin", 0x3800000, 0x400000, CRC(3fdb3542) SHA1(7d2050752a2064cd6729f483a0da93808e2c6033) ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "kf10-c8b.bin", 0x3800001, 0x400000, CRC(661b7a52) SHA1(0ae2ad2389134892f156337332b77adade3ddad1) ) /* Plane 2,3 */
ROM_END


/* 447 : Hypernoid by M.Priewe. */
ROM_START( hypernoid )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "447.p1", 0x000000, 0x100000, CRC(e024fa76) SHA1(6ef393ad80ec80e30929f07c95305d97fca3ad22) )

	NEO_SFIX_128K( "447.s1", CRC(bb82ab71) SHA1(307f420446c2d411a65b59543b61c21d0360536b) )

	NEO_BIOS_SOUND_512K( "447.m1", CRC(6c8eaacc) SHA1(6bc065be18db6830a4c94b424f9380d38da6f8b7) )

	ROM_REGION( 0x1000000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "447.v1", 0x000000, 0x400000, CRC(dafa1bdd) SHA1(743a5ad24a3dba04ce8f3e2d95ebd5ca83fd9f98) )
	ROM_LOAD( "447.v2", 0x400000, 0x400000, CRC(85ad8283) SHA1(8abcf48a353dcd4777608b13471608f7290d89a5) )
	ROM_LOAD( "447.v3", 0x800000, 0x400000, CRC(86c27f0c) SHA1(85b740e0224d5ea09b600683dec6fe532a723355) )
	ROM_LOAD( "447.v4", 0xc00000, 0x400000, CRC(a3982244) SHA1(7191909d7264df3dc417dc76cee53291986d84e9) )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "447.c1", 0x000000, 0x200000, CRC(41d6140a) SHA1(862b95ca1fe2b75d7ead0de8ac50c1b8a049c774) )
	ROM_LOAD16_BYTE( "447.c2", 0x000001, 0x200000, CRC(36f35df2) SHA1(22715561d5383263e41563fcb060f83a0c544531) )
ROM_END


/******************************************************************************/

/* dummy entry for the dummy bios driver */
ROM_START( neogeo )
	ROM_REGION16_BE( 0x020000, REGION_USER1, 0 )
	NEOGEO_BIOS

	ROM_REGION( 0x50000, REGION_CPU2, 0 )
	ROM_LOAD( "mame.sm1", 0x00000, 0x20000, CRC(97cf998b) SHA1(977387a7c76ef9b21d0b01fa69830e949a9a9626) )

	ROM_REGION( 0x10000, REGION_GFX4, 0 )
	ROM_LOAD( "mamelo.lo", 0x00000, 0x10000, CRC(e09e253c) SHA1(2b1c719531dac9bb503f22644e6e4236b91e7cfc) )  /* Y zoom control */

	ROM_REGION( 0x20000, REGION_GFX1, 0 )
	ROM_FILL(                 0x000000, 0x20000, 0 )
	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "sfix.sfx",  0x000000, 0x20000, CRC(354029fc) SHA1(4ae4bf23b4c2acff875775d4cbff5583893ce2a1) )
ROM_END



DRIVER_INIT( kog )
{
	kog_px_decrypt();
	neogeo_bootleg_sx_decrypt(1);
	neogeo_bootleg_cx_decrypt();
	init_neogeo();
}

DRIVER_INIT( kof99 )
{
	kof99_decrypt_68k();
	neogeo_fix_bank_type = 1;
	kof99_neogeo_gfx_decrypt(0x00);
	init_neogeo();
	kof99_install_protection();
}

DRIVER_INIT( garou )
{
	garou_decrypt_68k();
	neogeo_fix_bank_type = 1;
	kof99_neogeo_gfx_decrypt(0x06);
	init_neogeo();
  garou_install_protection(); 
}

DRIVER_INIT( garouo )
{
  garouo_decrypt_68k();
	neogeo_fix_bank_type = 1;
	kof99_neogeo_gfx_decrypt(0x06);
	init_neogeo();
	garouo_install_protection();
}

DRIVER_INIT( mslug3 )
{
	mslug3_decrypt_68k();
	neogeo_fix_bank_type = 1;
	kof99_neogeo_gfx_decrypt(0xad);
	init_neogeo();
	mslug3_install_protection();
}

DRIVER_INIT( kof2000 )
{
	kof2000_decrypt_68k();
	neogeo_fix_bank_type = 2;
	kof2000_neogeo_gfx_decrypt(0x00);
	neogeo_cmc50_m1_decrypt();
	init_neogeo();
	kof2000_install_protection();
}

DRIVER_INIT( kof2001 )
{
	neogeo_fix_bank_type = 0;
	neogeo_cmc50_m1_decrypt();
	kof2000_neogeo_gfx_decrypt(0x1e);
	init_neogeo();
}

DRIVER_INIT( cthd2003 )
{
	decrypt_cthd2003();
 	init_neogeo();
	patch_cthd2003();
}

DRIVER_INIT ( ct2k3sp )
{
	decrypt_ct2k3sp();
	init_neogeo();
	patch_cthd2003();
}

DRIVER_INIT( mslug4 )
{
	neogeo_fix_bank_type = 1; /* maybe slightly different, USA violent content screen is wrong */
	kof2000_neogeo_gfx_decrypt(0x31);
	neogeo_cmc50_m1_decrypt();
	init_neogeo();
	neo_pcm2_snk_1999(8);
}

DRIVER_INIT( kof99n )
{
	neogeo_fix_bank_type = 1;
	kof99_neogeo_gfx_decrypt(0x00);
	init_neogeo();
}

DRIVER_INIT( ganryu )
{
	neogeo_fix_bank_type = 1;
	kof99_neogeo_gfx_decrypt(0x07);
	init_neogeo();
}

DRIVER_INIT( s1945p )
{
	neogeo_fix_bank_type = 1;
	kof99_neogeo_gfx_decrypt(0x05);
	init_neogeo();
}

DRIVER_INIT( preisle2 )
{
	neogeo_fix_bank_type = 1;
	kof99_neogeo_gfx_decrypt(0x9f);
	init_neogeo();
}

DRIVER_INIT( mslug3n )
{
	neogeo_fix_bank_type = 1;
	kof99_neogeo_gfx_decrypt(0xad);
	init_neogeo();
}

DRIVER_INIT( kof2000n )
{
	neogeo_fix_bank_type = 2;
	neogeo_cmc50_m1_decrypt();
	kof2000_neogeo_gfx_decrypt(0x00);
	init_neogeo();
}

DRIVER_INIT( bangbead )
{
	neogeo_fix_bank_type = 1;
	kof99_neogeo_gfx_decrypt(0xf8);
	init_neogeo();
}

DRIVER_INIT( nitd )
{
	neogeo_fix_bank_type = 1;
	kof99_neogeo_gfx_decrypt(0xff);
	init_neogeo();
}

DRIVER_INIT( zupapa )
{
	neogeo_fix_bank_type = 1;
	kof99_neogeo_gfx_decrypt(0xbd);
	init_neogeo();
}

DRIVER_INIT( sengoku3 )
{
	neogeo_fix_bank_type = 1;
	kof99_neogeo_gfx_decrypt(0xfe);
	init_neogeo();
}

DRIVER_INIT( pnyaa )
{
	neo_pcm2_snk_1999(4);
	neogeo_fix_bank_type = 1;
	kof2000_neogeo_gfx_decrypt(0x2e);
	neogeo_cmc50_m1_decrypt();
	init_neogeo();
}

DRIVER_INIT ( kof98 )
{
	kof98_decrypt_68k();
	init_neogeo();
	install_kof98_protection();
}

DRIVER_INIT( mjneogeo )
{
	install_mem_read16_handler (0, 0x300000, 0x300001, mjneogeo_r);
	install_mem_write16_handler(0, 0x380000, 0x380001, mjneogeo_w);
	init_neogeo();
}

DRIVER_INIT( popbounc )
{
	install_mem_read16_handler (0, 0x300000, 0x300001, popbounc1_16_r);
	install_mem_read16_handler (0, 0x340000, 0x340001, popbounc2_16_r);
	init_neogeo();
}

DRIVER_INIT( rotd )
{
  neo_pcm2_snk_1999(16);
	neogeo_fix_bank_type = 1;
	neogeo_cmc50_m1_decrypt();
	kof2000_neogeo_gfx_decrypt(0x3f);
	init_neogeo();
}

DRIVER_INIT( kof2002 )
{
	kof2002_decrypt_68k();
	neo_pcm2_swap(0);
	neogeo_cmc50_m1_decrypt();
	neogeo_fix_bank_type = 0;
	kof2000_neogeo_gfx_decrypt(0xec);
	init_neogeo();
}

DRIVER_INIT( kf2k2pls )
{
	kof2002_decrypt_68k();
	neo_pcm2_swap(0);
	neogeo_cmc50_m1_decrypt();
	neogeo_fix_bank_type = 0;
	cmc50_neogeo_gfx_decrypt(0xec);
	init_neogeo();
}

DRIVER_INIT( kf2k2mp )
{
	kf2k2mp_decrypt();
	neo_pcm2_swap(0);
	neogeo_cmc50_m1_decrypt();
	neogeo_bootleg_sx_decrypt(2);
	neogeo_fix_bank_type = 0;
	cmc50_neogeo_gfx_decrypt(0xec);
	init_neogeo();
}



DRIVER_INIT( kof2km2 )
{
	kof2km2_px_decrypt();
	neo_pcm2_swap(0);
	neogeo_cmc50_m1_decrypt();
	neogeo_bootleg_sx_decrypt(1);
	neogeo_fix_bank_type = 0;
	cmc50_neogeo_gfx_decrypt(0xec);
	init_neogeo();
}

DRIVER_INIT( matrim )
{
	matrim_decrypt_68k();
	neo_pcm2_swap(1);
	neogeo_fix_bank_type = 2;
	kof2000_neogeo_gfx_decrypt(0x6a);
	neogeo_cmc50_m1_decrypt();
	init_neogeo();
}

static UINT16 mv0_prot_offset[ 0x08 ];
static UINT16 mv0_bankswitch_offset[ 2 ];
static int mv0_bankswitch_flg;

static READ16_HANDLER( mv0_prot_r )
{
	return mv0_prot_offset[ offset ];
}

static WRITE16_HANDLER( mv0_prot_w )
{
	mv0_prot_offset[ offset ] = (mv0_prot_offset[ offset ] & mem_mask) | ((~mem_mask) & data);

	if( offset == 0 ){
		UINT8 *ofst8 = (UINT8*)mv0_prot_offset;

		ofst8[ 0x02 ] = ((ofst8[ 0x01 ] >> 4) & 0x01) + ((ofst8[ 0x00 ] & 0x0f) << 1);
		ofst8[ 0x03 ] = ((ofst8[ 0x01 ] >> 5) & 0x01) + (((ofst8[ 0x00 ] >> 4) & 0x0f) << 1);
		ofst8[ 0x04 ] = ((ofst8[ 0x01 ] >> 6) & 0x01) + ((ofst8[ 0x01 ] & 0x0f) << 1);
		ofst8[ 0x05 ] = (ofst8[ 0x01 ] >> 7);
	}else if( offset == 5 ){
		UINT8 *ofst8 = (UINT8*)mv0_prot_offset;

		ofst8[ 0x0c ] = (ofst8[ 0x08 ] >> 1) | ((ofst8[ 0x09 ] >> 1) << 4);
		ofst8[ 0x0d ] = (ofst8[ 0x0a ] >> 1) |
						((ofst8[ 0x08 ] & 0x01) << 4) |
						((ofst8[ 0x09 ] & 0x01) << 5) |
						((ofst8[ 0x0a ] & 0x01) << 6) |
						(ofst8[ 0x0b ] << 7);
	}
}

/* incomplete*/
static READ16_HANDLER( mv0_bankswitch_r )
{
	if( mv0_bankswitch_offset[ 0 ] == 0xffff && mv0_bankswitch_offset[ 1 ] == 0xffff ){

		mv0_bankswitch_flg = 1;
		if( offset == 0 ){
			return 0xfea0;
		}else if( offset == 1 ){
			return 0x7fff;
		}else{
			return mv0_bankswitch_offset[ offset ];
		}
	}else if( mv0_bankswitch_offset[ 0 ] == 0x0000 && mv0_bankswitch_offset[ 1 ] == 0x0000 && mv0_bankswitch_flg == 1 ){

		if( offset == 0 ){
			return 0x00a0;
		}else if( offset == 1 ){
			mv0_bankswitch_flg = 0;
			return 0x0000;
		}else{
			return mv0_bankswitch_offset[ offset ];
		}
	}else{
		return mv0_bankswitch_offset[ offset ];
	}
}

static WRITE16_HANDLER( mv0_bankswitch_w )
{
	UINT32 bank_addr;

	mv0_bankswitch_offset[ offset ] = (mv0_bankswitch_offset[ offset ] & mem_mask) | ((~mem_mask) & data);

	bank_addr = (mv0_bankswitch_offset[ 0 ] >> 8) +
				(mv0_bankswitch_offset[ 1 ] << 8) +
				0x100000;

	neogeo_set_cpu1_second_bank( bank_addr );
}


/***************************************************************************
 svcboot
***************************************************************************/

DRIVER_INIT( svcpcb )
{
	svcchaos_px_decrypt();
	svcpcb_gfx_decrypt();
	kof2000_neogeo_gfx_decrypt(0x57);
	svcpcb_s1data_decrypt();
	neogeo_cmc50_m1_decrypt();
	neo_pcm2_swap(3);
	neogeo_fix_bank_type = 2;
	init_neogeo();
	install_pvc_protection();
}

DRIVER_INIT( svc )
{
	svcchaos_px_decrypt();
	svcchaos_vx_decrypt();
	neogeo_cmc50_m1_decrypt();
	kof2000_neogeo_gfx_decrypt(0x57);
	neogeo_fix_bank_type = 2;
	install_mem_read16_handler( 0, 0x2fe000, 0x2fffdf, MRA16_RAM );
	install_mem_write16_handler( 0, 0x2fe000, 0x2fffdf, MWA16_RAM );
	install_mem_read16_handler( 0, 0x2fffe0, 0x2fffef, mv0_prot_r );
	install_mem_write16_handler( 0, 0x2fffe0, 0x2fffef, mv0_prot_w );
	install_mem_read16_handler( 0, 0x2ffff0, 0x2fffff, mv0_bankswitch_r );
	install_mem_write16_handler( 0, 0x2ffff0, 0x2fffff, mv0_bankswitch_w );
	init_neogeo();
}

DRIVER_INIT( svcboot )
{
	svcboot_px_decrypt();
	svcboot_cx_decrypt();
  init_neogeo();
  install_pvc_protection();
}

DRIVER_INIT( svcplus )
{
	svcplus_px_decrypt();
	svcboot_cx_decrypt();
/*  svcplus_sx_decrypt(); */
	neogeo_bootleg_sx_decrypt(1);
	svcplus_px_hack();
	init_neogeo();
}

DRIVER_INIT( svcplusa )
{
	svcplusa_px_decrypt();
	svcboot_cx_decrypt();
	svcplus_px_hack();
	init_neogeo();
}

DRIVER_INIT( svcsplus )
{
	svcsplus_px_decrypt();
/*  svcsplus_sx_decrypt(); */
	neogeo_bootleg_sx_decrypt(2);
	svcboot_cx_decrypt();
	svcsplus_px_hack();
	init_neogeo();
	install_pvc_protection();
}

DRIVER_INIT( samsho5 )
{	
	samsho5_decrypt_68k();
	neo_pcm2_swap(4);
	neogeo_fix_bank_type = 1;
	neogeo_cmc50_m1_decrypt();
	kof2000_neogeo_gfx_decrypt(0x0f);
	init_neogeo();
}

DRIVER_INIT( samsh5sp )
{
  samsh5sp_decrypt_68k();
	neogeo_fix_bank_type = 1;
	kof2000_neogeo_gfx_decrypt(0x0d);
	neogeo_cmc50_m1_decrypt();
	neo_pcm2_swap(6);
	init_neogeo();
}

DRIVER_INIT( mslug5 )
{
	mslug5_decrypt_68k();
	neo_pcm2_swap(2);
  neogeo_cmc50_m1_decrypt();
	neogeo_fix_bank_type = 1;
	kof2000_neogeo_gfx_decrypt(0x19);
	install_pvc_protection();
	init_neogeo();
}



DRIVER_INIT( kof2k4se )
{
	decrypt_kof2k4se_68k();
	init_neogeo();
}

DRIVER_INIT( kof10th )
{
	decrypt_kof10th();
	init_neogeo();
	install_kof10th_protection();
}



DRIVER_INIT( kf10thep )
{
	decrypt_kf10thep();
	init_neogeo();
}



DRIVER_INIT( kf2k5uni )
{
	decrypt_kf2k5uni();
	init_neogeo();
}

DRIVER_INIT( kof2003d )
{
	kof2003_px_decrypt();
	kof2003_sx_decrypt();

	neogeo_fix_bank_type = 2;
	

	install_mem_read16_handler( 0, 0x2fe000, 0x2fffdf, MRA16_RAM );
	install_mem_write16_handler( 0, 0x2fe000, 0x2fffdf, MWA16_RAM );

	install_mem_read16_handler( 0, 0x2fffe0, 0x2fffef, mv0_prot_r );
	install_mem_write16_handler( 0, 0x2fffe0, 0x2fffef, mv0_prot_w );

	install_mem_read16_handler( 0, 0x2ffff0, 0x2fffff, mv0_bankswitch_r );
	install_mem_write16_handler( 0, 0x2ffff0, 0x2fffff, mv0_bankswitch_w );

	install_mem_read16_handler( 0, 0xc00000, 0xc3ffff, MRA16_BANK3 );  /* 256k bios */
	init_neogeo();
}

DRIVER_INIT( kf2k3pcb ) /* Jamama, Single Board */
{
	kf2k3pcb_decrypt_68k();
	kf2k3pcb_gfx_decrypt();
	kof2000_neogeo_gfx_decrypt(0x9d);
	kf2k3pcb_decrypt_s1data();
	kof2003biosdecode();
	/* rom[i] = BITSWAP8(rom[i], 5, 6, 1, 4, 3, 0, 7, 2) -- extra encrypted m1 swap? not confirmed */
	neo_pcm2_swap(5);
	neogeo_cmc50_m1_decrypt();
	neogeo_fix_bank_type = 2;
	init_neogeo();
	install_pvc_protection();
	install_mem_read16_handler( 0, 0xc00000, 0xc7ffff, MRA16_BANK3 );  /* 512k bios */
}



DRIVER_INIT( kof2003 )
{
	kof2003_decrypt_68k();
	neo_pcm2_swap(5);
	neogeo_cmc50_m1_decrypt();
	neogeo_fix_bank_type = 2;
	kof2000_neogeo_gfx_decrypt(0x9d);
	init_neogeo();
	install_pvc_protection();
}



DRIVER_INIT( kf2k3bl )
{
	cmc50_neogeo_gfx_decrypt(0x9d);
	neo_pcm2_swap(5);
	neogeo_bootleg_sx_decrypt(1);
	init_neogeo();
	kf2k3bl_install_protection();
}



DRIVER_INIT( kf2k3pl )
{
	cmc50_neogeo_gfx_decrypt(0x9d);
	neo_pcm2_swap(5);
	kf2k3pl_px_decrypt();
	neogeo_bootleg_sx_decrypt(1);
	init_neogeo();
	kf2k3pl_install_protection();
}



DRIVER_INIT( kf2k3upl )
{
	cmc50_neogeo_gfx_decrypt(0x9d);
	neo_pcm2_swap(5);
	kf2k3upl_px_decrypt();
	neogeo_bootleg_sx_decrypt(2);
	init_neogeo();
	kf2k3upl_install_protection();
}
/* kof2003d Init End */

/******************************************************************************/

static UINT32 cpu1_second_bankaddress;

void neogeo_set_cpu1_second_bank(UINT32 bankaddress)
{
	data8_t *RAM = memory_region(REGION_CPU1);

	cpu1_second_bankaddress = bankaddress;
	cpu_setbank(4,&RAM[bankaddress]);
}

void neogeo_init_cpu2_setbank(void)
{
	UINT8 *mem08 = memory_region(REGION_CPU2);

	cpu_setbank(5,&mem08[bank[0]]);
	cpu_setbank(6,&mem08[bank[1]]);
	cpu_setbank(7,&mem08[bank[2]]);
	cpu_setbank(8,&mem08[bank[3]]);
}

static void neogeo_init_cpu_banks(void)
{
	neogeo_set_cpu1_second_bank(cpu1_second_bankaddress);
	neogeo_init_cpu2_setbank();
}

void neogeo_register_main_savestate(void)
{
	state_save_register_UINT32("neogeo", 0, "neogeo_frame_counter",       &neogeo_frame_counter,       1);
	state_save_register_UINT32("neogeo", 0, "neogeo_frame_counter_speed", &neogeo_frame_counter_speed, 1);
	state_save_register_int   ("neogeo", 0, "current_rastercounter",      &current_rastercounter);
	state_save_register_int   ("neogeo", 0, "current_rasterline",         &current_rasterline);
	state_save_register_int   ("neogeo", 0, "scanline_read",              &scanline_read);
	state_save_register_int   ("neogeo", 0, "irq2start",                  &irq2start);
	state_save_register_int   ("neogeo", 0, "irq2control",                &irq2control);
	state_save_register_UINT32("neogeo", 0, "irq2pos_value",              &irq2pos_value,              1);
	state_save_register_int   ("neogeo", 0, "vblank_int",                 &vblank_int);
	state_save_register_int   ("neogeo", 0, "scanline_int",               &scanline_int);
	state_save_register_int   ("neogeo", 0, "fc",                         &fc);
	state_save_register_int   ("neogeo", 0, "neogeo_raster_enable",       &neogeo_raster_enable);
	state_save_register_int   ("neogeo", 0, "pending_command",            &pending_command);
	state_save_register_int   ("neogeo", 0, "result_code",                &result_code);
	state_save_register_int   ("neogeo", 0, "ts",                         &ts);
	state_save_register_UINT32("neogeo", 0, "bank",                       bank,                        4);
	state_save_register_int   ("neogeo", 0, "neogeo_rng",                 &neogeo_rng);
	state_save_register_UINT32("neogeo", 0, "cpu1_second_bankaddress",    &cpu1_second_bankaddress,    1);

	state_save_register_func_postload(neogeo_init_cpu_banks);
}


/******************************************************************************/

/* A dummy driver, so that the bios can be debugged, and to serve as */
/* parent for the NEOGEO_BIOS files, so that we do not have to include */
/* them in every zip file */
GAMEBX( 1990, neogeo, 0, neogeo, neogeo, neogeo, neogeo, ROT0, "SNK", "Neo-Geo", NOT_A_DRIVER, &neogeo_ctrl, NULL )

/******************************************************************************/

/*     YEAR  NAME      PARENT    BIOS    MACHINE INPUT    INIT      MONITOR  */

/* SNK */
GAMEB( 1990, nam1975,  neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "SNK", "NAM-1975", &neogeo_ctrl, NULL )
GAMEB( 1990, bstars,   neogeo,   neogeo, neo320, neogeo,  neogeo,   ROT0, "SNK", "Baseball Stars Professional", &neogeo_ctrl, NULL )
GAMEB( 1990, tpgolf,   neogeo,   neogeo, raster, neogeo,  neogeo,   ROT0, "SNK", "Top Player's Golf", &neogeo_ctrl, NULL )
GAMEB( 1990, mahretsu, neogeo,   neogeo, neogeo, mjneogeo,mjneogeo, ROT0, "SNK", "Mahjong Kyoretsuden", &neogeo_ctrl, NULL )
GAMEB( 1990, ridhero,  neogeo,   neogeo, raster, neogeo,  neogeo,   ROT0, "SNK", "Riding Hero (set 1)", &neogeo_ctrl, NULL )
GAMEB( 1990, ridheroh, ridhero,  neogeo, raster, neogeo,  neogeo,   ROT0, "SNK", "Riding Hero (set 2)", &neogeo_ctrl, NULL )
GAMEB( 1991, alpham2,  neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "SNK", "Alpha Mission II / ASO II - Last Guardian", &neogeo_ctrl, NULL )
GAMEB( 1990, cyberlip, neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "SNK", "Cyber-Lip", &neogeo_ctrl, NULL )
GAMEB( 1990, superspy, neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "SNK", "The Super Spy", &neogeo_ctrl, NULL )
GAMEB( 1992, mutnat,   neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "SNK", "Mutation Nation", &neogeo_ctrl, NULL )
GAMEB( 1991, kotm,     neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "SNK", "King of the Monsters (set 1)", &neogeo_ctrl, NULL )
GAMEB( 1991, kotmh,    kotm,     neogeo, neogeo, neogeo,  neogeo,   ROT0, "SNK", "King of the Monsters (set 2)", &neogeo_ctrl, NULL )
GAMEB( 1991, sengoku,  neogeo,   neogeo, ras320, neogeo,  neogeo,   ROT0, "SNK", "Sengoku / Sengoku Denshou (set 1)", &neogeo_ctrl, NULL )
GAMEB( 1991, sengokh,  sengoku,  neogeo, ras320, neogeo,  neogeo,   ROT0, "SNK", "Sengoku / Sengoku Denshou (set 2)", &neogeo_ctrl, NULL )
GAMEB( 1991, burningf, neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "SNK", "Burning Fight (set 1)", &neogeo_ctrl, NULL )
GAMEB( 1991, burningh, burningf, neogeo, neogeo, neogeo,  neogeo,   ROT0, "SNK", "Burning Fight (set 2)", &neogeo_ctrl, NULL )
GAMEB( 1990, lbowling, neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "SNK", "League Bowling", &neogeo_ctrl, NULL )
GAMEB( 1991, gpilots,  neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "SNK", "Ghost Pilots", &neogeo_ctrl, NULL )
GAMEB( 1990, joyjoy,   neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "SNK", "Puzzled / Joy Joy Kid", &neogeo_ctrl, NULL )
GAMEB( 1991, quizdais, neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "SNK", "Quiz Daisousa Sen - The Last Count Down", &neogeo_ctrl, NULL )
GAMEB( 1992, lresort,  neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "SNK", "Last Resort", &neogeo_ctrl, NULL )
GAMEB( 1991, eightman, neogeo,   neogeo, ras320, neogeo,  neogeo,   ROT0, "SNK / Pallas", "Eight Man", &neogeo_ctrl, NULL )
GAMEB( 1991, legendos, neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "SNK", "Legend of Success Joe / Ashitano Joe Densetsu", &neogeo_ctrl, NULL )
GAMEB( 1991, 2020bb,   neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "SNK / Pallas", "2020 Super Baseball (set 1)", &neogeo_ctrl, NULL )
GAMEB( 1991, 2020bbh,  2020bb,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "SNK / Pallas", "2020 Super Baseball (set 2)", &neogeo_ctrl, NULL )
GAMEB( 1991, socbrawl, neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "SNK", "Soccer Brawl", &neogeo_ctrl, NULL )
GAMEB( 1991, fatfury1, neogeo,   neogeo, neo320, neogeo,  neogeo,   ROT0, "SNK", "Fatal Fury - King of Fighters / Garou Densetsu - Shukumei no Tatakai", &neogeo_ctrl, NULL )
GAMEB( 1991, roboarmy, neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "SNK", "Robo Army", &neogeo_ctrl, NULL )
GAMEB( 1992, fbfrenzy, neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "SNK", "Football Frenzy", &neogeo_ctrl, NULL )
GAMEB( 1992, kotm2,    neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "SNK", "King of the Monsters 2 - The Next Thing", &neogeo_ctrl, NULL )
GAMEB( 1993, sengoku2, neogeo,   neogeo, raster, neogeo,  neogeo,   ROT0, "SNK", "Sengoku 2 / Sengoku Denshou 2", &neogeo_ctrl, NULL )
GAMEB( 1992, bstars2,  neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "SNK", "Baseball Stars 2", &neogeo_ctrl, NULL )
GAMEB( 1992, quizdai2, neogeo,   neogeo, neo320, neogeo,  neogeo,   ROT0, "SNK", "Quiz Meitantei Neo and Geo - Quiz Daisousa Sen part 2", &neogeo_ctrl, NULL )
GAMEB( 1993, 3countb,  neogeo,   neogeo, neo320, neogeo,  neogeo,   ROT0, "SNK", "3 Count Bout / Fire Suplex (NGM-043 ~ NGH-043)", &neogeo_ctrl, NULL )
GAMEB( 1992, aof,      neogeo,   neogeo, raster, neogeo,  neogeo,   ROT0, "SNK", "Art of Fighting / Ryuuko no Ken", &neogeo_ctrl, NULL )
GAMEB( 1993, samsho,   neogeo,   neogeo, neo320, neogeo,  neogeo,   ROT0, "SNK", "Samurai Shodown / Samurai Spirits", &neogeo_ctrl, NULL )
GAMEB( 1994, tophuntr, neogeo,   neogeo, ras320, neogeo,  neogeo,   ROT0, "SNK", "Top Hunter - Roddy and Cathy", &neogeo_ctrl, NULL )
GAMEB( 1992, fatfury2, neogeo,   neogeo, neo320, neogeo,  neogeo,   ROT0, "SNK", "Fatal Fury 2 / Garou Densetsu 2 - Arata-naru Tatakai", &neogeo_ctrl, NULL )
GAMEB( 1992, ssideki,  neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "SNK", "Super Sidekicks / Tokuten Ou", &neogeo_ctrl, NULL )
GAMEB( 1994, kof94,    neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "SNK", "The King of Fighters '94", &neogeo_ctrl, NULL )
GAMEB( 1994, aof2,     neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "SNK", "Art of Fighting 2 / Ryuuko no Ken 2", &neogeo_ctrl, NULL )
GAMEB( 1993, fatfursp, neogeo,   neogeo, neo320, neogeo,  neogeo,   ROT0, "SNK", "Fatal Fury Special / Garou Densetsu Special (set 1)", &neogeo_ctrl, NULL )
GAMEB( 1993, fatfursa, fatfursp, neogeo, neo320, neogeo,  neogeo,   ROT0, "SNK", "Fatal Fury Special / Garou Densetsu Special (set 2)", &neogeo_ctrl, NULL )
GAMEB( 1995, savagere, neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "SNK", "Savage Reign / Fu'un Mokushiroku - kakutou sousei", &neogeo_ctrl, NULL )
GAMEB( 1994, ssideki2, neogeo,   neogeo, ras320, neogeo,  neogeo,   ROT0, "SNK", "Super Sidekicks 2 - The World Championship / Tokuten Ou 2 - Real Fight Football", &neogeo_ctrl, NULL )
GAMEB( 1994, samsho2,  neogeo,   neogeo, neo320, neogeo,  neogeo,   ROT0, "SNK", "Samurai Shodown II / Shin Samurai Spirits - Haohmaru jigokuhen", &neogeo_ctrl, NULL )
GAMEB( 1995, fatfury3, neogeo,   neogeo, ras320, neogeo,  neogeo,   ROT0, "SNK", "Fatal Fury 3 - Road to the Final Victory / Garou Densetsu 3 - Haruka-naru Tatakai", &neogeo_ctrl, NULL )
GAMEB( 1995, ssideki3, neogeo,   neogeo, ras320, neogeo,  neogeo,   ROT0, "SNK", "Super Sidekicks 3 - The Next Glory / Tokuten Ou 3 - eikoue no michi", &neogeo_ctrl, NULL )
GAMEB( 1995, kof95,    neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "SNK", "The King of Fighters '95 (set 1)", &neogeo_ctrl, NULL )
GAMEB( 1995, kof95a,   kof95,    neogeo, neogeo, neogeo,  neogeo,   ROT0, "SNK", "The King of Fighters '95 (set 2)", &neogeo_ctrl, NULL )
GAMEB( 1995, samsho3,  neogeo,   neogeo, raster, neogeo,  neogeo,   ROT0, "SNK", "Samurai Shodown III / Samurai Spirits - Zankurou Musouken", &neogeo_ctrl, NULL )
GAMEB( 1995, rbff1,    neogeo,   neogeo, neo320, neogeo,  neogeo,   ROT0, "SNK", "Real Bout Fatal Fury / Real Bout Garou Densetsu", &neogeo_ctrl, NULL )
GAMEB( 1996, aof3,     neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "SNK", "Art of Fighting 3 - The Path of the Warrior / Art of Fighting - Ryuuko no Ken Gaiden", &neogeo_ctrl, NULL )
GAMEB( 1996, kof96,    neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "SNK", "The King of Fighters '96 (set 1)", &neogeo_ctrl, NULL )
GAMEB( 1996, kof96h,   kof96,    neogeo, neogeo, neogeo,  neogeo,   ROT0, "SNK", "The King of Fighters '96 (set 2)", &neogeo_ctrl, NULL )
GAMEB( 1996, ssideki4, neogeo,   neogeo, ras320, neogeo,  neogeo,   ROT0, "SNK", "Ultimate 11 - The SNK Football Championship / Tokuten Ou - Honoo no Libero, The", &neogeo_ctrl, NULL )
GAMEB( 1996, kizuna,   neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "SNK", "Kizuna Encounter - Super Tag Battle / Fu'un Super Tag Battle", &neogeo_ctrl, NULL )
GAMEB( 1996, samsho4,  neogeo,   neogeo, neo320, neogeo,  neogeo,   ROT0, "SNK", "Samurai Shodown IV - Amakusa's Revenge / Samurai Spirits - Amakusa Kourin", &neogeo_ctrl, NULL )
GAMEB( 1996, rbffspec, neogeo,   neogeo, neo320, neogeo,  neogeo,   ROT0, "SNK", "Real Bout Fatal Fury Special / Real Bout Garou Densetsu Special", &neogeo_ctrl, NULL )
GAMEB( 1997, kof97,    neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "SNK", "The King of Fighters '97 (set 1)", &neogeo_ctrl, NULL )
GAMEB( 1997, kof97a,   kof97,    neogeo, neogeo, neogeo,  neogeo,   ROT0, "SNK", "The King of Fighters '97 (set 2)", &neogeo_ctrl, NULL )
GAMEB( 1997, kof97pls, kof97,    neogeo, neogeo, neogeo,  neogeo,   ROT0, "SNK", "The King of Fighters '97 Plus (bootleg)", &neogeo_ctrl, NULL )
GAMEB( 1997, kog,      kof97,    neogeo, neogeo, kog,     kog,      ROT0, "bootleg", "King of Gladiator (The King of Fighters '97 bootleg)", &neogeo_ctrl, NULL )
GAMEB( 1997, lastblad, neogeo,   neogeo, neo320, neogeo,  neogeo,   ROT0, "SNK", "Last Blade / Bakumatsu Roman - Gekka no Kenshi, The (set 1)", &neogeo_ctrl, NULL )
GAMEB( 1997, lastblda, lastblad, neogeo, neo320, neogeo,  neogeo,   ROT0, "SNK", "Last Blade / Bakumatsu Roman - Gekka no Kenshi, The (set 2)", &neogeo_ctrl, NULL )
GAMEB( 1997, irrmaze,  neogeo,   neogeo, neogeo, irrmaze, neogeo,   ROT0, "SNK / Saurus", "The Irritating Maze / Ultra Denryu Iraira Bou", &neogeo_ctrl, NULL )
GAMEB( 1998, rbff2,    neogeo,   neogeo, neo320, neogeo,  neogeo,   ROT0, "SNK", "Real Bout Fatal Fury 2 - The Newcomers / Real Bout Garou Densetsu 2 - The Newcomers (set 1)", &neogeo_ctrl, NULL )
GAMEB( 1998, rbff2a,   rbff2,    neogeo, neo320, neogeo,  neogeo,   ROT0, "SNK", "Real Bout Fatal Fury 2 - The Newcomers / Real Bout Garou Densetsu 2 - The Newcomers (set 2)", &neogeo_ctrl, NULL )
GAMEB( 1998, mslug2,   neogeo,   neogeo, raster, neogeo,  neogeo,   ROT0, "SNK", "Metal Slug 2 - Super Vehicle-001-II", &neogeo_ctrl, NULL )
GAMEB( 2015, mslug2t,  mslug2,   neogeo, raster, neogeo,  neogeo,   ROT0, "bootleg", "Metal Slug 2 - Turbo (Trap15, Hack)", &neogeo_ctrl, NULL )
GAMEB( 1998, kof98,    neogeo,   neogeo, neogeo, neogeo,  kof98,    ROT0, "SNK", "The King of Fighters '98 - The Slugfest - King of Fighters '98 / Dream Match Never Ends", &neogeo_ctrl, NULL )
GAMEB( 1998, kof98k,   kof98,    neogeo, neogeo, neogeo,  kof98,    ROT0, "SNK", "The King of Fighters '98 - The Slugfest / King of Fighters '98 - dream match never ends (Korean board)", &neogeo_ctrl, NULL )
GAMEB( 1998, kof98n,   kof98,    neogeo, neogeo, neogeo,  neogeo,   ROT0, "SNK", "The King of Fighters '98 - The Slugfest - King of Fighters '98 / Dream Match Never Ends (Decrypted)", &neogeo_ctrl, NULL )
GAMEB( 1998, lastbld2, neogeo,   neogeo, ras320, neogeo,  neogeo,   ROT0, "SNK", "Last Blade 2 / Bakumatsu Roman - Dai Ni Maku Gekka no Kenshi, The", &neogeo_ctrl, NULL )
GAMEB( 1998, neocup98, neogeo,   neogeo, ras320, neogeo,  neogeo,   ROT0, "SNK", "Neo-Geo Cup '98 - The Road to the Victory", &neogeo_ctrl, NULL )
GAMEB( 1999, mslugx,   neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "SNK", "Metal Slug X - Super Vehicle-001", &neogeo_ctrl, NULL )
GAMEB( 1999, kof99,    neogeo,   neogeo, raster, neogeo,  kof99,    ROT0, "SNK", "The King of Fighters '99 - Millennium Battle (set 1)", &neogeo_ctrl, NULL ) /* Encrypted Code & GFX */
GAMEB( 1999, kof99a,   kof99,    neogeo, raster, neogeo,  kof99,    ROT0, "SNK", "The King of Fighters '99 - Millennium Battle (set 2)", &neogeo_ctrl, NULL ) /* Encrypted Code & GFX, crashes going into attract demo */
GAMEB( 1999, kof99e,   kof99,    neogeo, raster, neogeo,  kof99,    ROT0, "SNK", "The King of Fighters '99 - Millennium Battle (earlier)", &neogeo_ctrl, NULL ) /* Encrypted Code & GFX */
GAMEB( 1999, kof99n,   kof99,    neogeo, raster, neogeo,  kof99n,   ROT0, "SNK", "The King of Fighters '99 - Millennium Battle (not encrypted)", &neogeo_ctrl, NULL )	/* Encrypted GFX */
GAMEB( 1999, kof99p,   kof99,    neogeo, raster, neogeo,  neogeo,   ROT0, "SNK", "The King of Fighters '99 - Millennium Battle (prototype)", &neogeo_ctrl, NULL )
GAMEB( 1999, garou,    neogeo,   neogeo, ras320, neogeo,  garou,    ROT0, "SNK", "Garou - Mark of the Wolves (set 1)", &neogeo_ctrl, NULL ) /* Encrypted Code & GFX */
GAMEB( 1999, garouo,   garou,    neogeo, ras320, neogeo,  garouo,   ROT0, "SNK", "Garou - Mark of the Wolves (set 2)", &neogeo_ctrl, NULL ) /* Encrypted Code & GFX */
GAMEB( 1999, garoup,   garou,    neogeo, ras320, neogeo,  neogeo,   ROT0, "SNK", "Garou - Mark of the Wolves (prototype)", &neogeo_ctrl, NULL )
GAMEB( 2000, mslug3,   neogeo,   neogeo, raster, neogeo,  mslug3,   ROT0, "SNK", "Metal Slug 3", &neogeo_ctrl, NULL ) /* Encrypted Code & GFX */
GAMEB( 2000, mslug3n,  mslug3,   neogeo, raster, neogeo,  mslug3n,  ROT0, "SNK", "Metal Slug 3 (not encrypted)", &neogeo_ctrl, NULL ) /* Encrypted GFX */
GAMEB( 2000, kof2000,  neogeo,   neogeo, neogeo, neogeo,  kof2000,  ROT0, "SNK", "The King of Fighters 2000", &neogeo_ctrl, NULL ) /* Encrypted Code & GFX */
GAMEB( 2000, kof2000n, kof2000,  neogeo, neogeo, neogeo,  kof2000n, ROT0, "SNK", "The King of Fighters 2000 (not encrypted)", &neogeo_ctrl, NULL ) /* Encrypted GFX */
GAMEB( 1996, zintrckb, neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "hack / bootleg",   "Zintrick - Oshidashi Zentrix (Hack-bootleg)", &neogeo_ctrl, NULL )

/* Alpha Denshi Co. / ADK (changed name in 1993) */
GAMEB( 1990, maglord,  neogeo,   neogeo, neo320, neogeo,  neogeo,   ROT0, "Alpha Denshi Co.", "Magician Lord (set 1)", &neogeo_ctrl, NULL )
GAMEB( 1990, maglordh, maglord,  neogeo, neo320, neogeo,  neogeo,   ROT0, "Alpha Denshi Co.", "Magician Lord (set 2)", &neogeo_ctrl, NULL )
GAMEB( 1990, ncombat,  neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "Alpha Denshi Co.", "Ninja Combat", &neogeo_ctrl, NULL )
GAMEB( 1990, bjourney, neogeo,   neogeo, neo320, neogeo,  neogeo,   ROT0, "Alpha Denshi Co.", "Blue's Journey / Raguy", &neogeo_ctrl, NULL )
GAMEB( 1991, crsword,  neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "Alpha Denshi Co.", "Crossed Swords", &neogeo_ctrl, NULL )
GAMEB( 1996, crswd2bl, neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "bootleg",          "Crossed Swords 2", &neogeo_ctrl, NULL )
GAMEB( 1991, trally,   neogeo,   neogeo, ras320, neogeo,  neogeo,   ROT0, "Alpha Denshi Co.", "Thrash Rally", &neogeo_ctrl, NULL )
GAMEB( 1992, ncommand, neogeo,   neogeo, raster, neogeo,  neogeo,   ROT0, "Alpha Denshi Co.", "Ninja Commando", &neogeo_ctrl, NULL )
GAMEB( 1992, wh1,      neogeo,   neogeo, ras320, neogeo,  neogeo,   ROT0, "Alpha Denshi Co.", "World Heroes (set 1)", &neogeo_ctrl, NULL )
GAMEB( 1992, wh1h,     wh1,      neogeo, ras320, neogeo,  neogeo,   ROT0, "Alpha Denshi Co.", "World Heroes (set 2)", &neogeo_ctrl, NULL )
GAMEB( 1993, wh2,      neogeo,   neogeo, raster, neogeo,  neogeo,   ROT0, "ADK",              "World Heroes 2", &neogeo_ctrl, NULL )
GAMEB( 1994, wh2j,     neogeo,   neogeo, raster, neogeo,  neogeo,   ROT0, "ADK / SNK",        "World Heroes 2 Jet", &neogeo_ctrl, NULL )
GAMEB( 1994, aodk,     neogeo,   neogeo, raster, neogeo,  neogeo,   ROT0, "ADK / SNK",        "Aggressors of Dark Kombat / Tsuukai GANGAN Koushinkyoku", &neogeo_ctrl, NULL )
GAMEB( 1995, whp,      neogeo,   neogeo, neo320, neogeo,  neogeo,   ROT0, "ADK / SNK",        "World Heroes Perfect", &neogeo_ctrl, NULL )
GAMEB( 1995, mosyougi, neogeo,   neogeo, raster_busy, neogeo,  neogeo,   ROT0, "ADK / SNK",   "Syougi No Tatsujin - Master of Syougi", &neogeo_ctrl, NULL )
GAMEB( 1996, overtop,  neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "ADK",              "Over Top", &neogeo_ctrl, NULL )
GAMEB( 1996, ninjamas, neogeo,   neogeo, neo320, neogeo,  neogeo,   ROT0, "ADK / SNK",        "Ninja Master's - Haoh-ninpo-cho", &neogeo_ctrl, NULL )
GAMEB( 1996, twinspri, neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "ADK",              "Twinkle Star Sprites", &neogeo_ctrl, NULL )
GAMEB( 2001, zupapa,   neogeo,   neogeo, neogeo, neogeo,  zupapa,   ROT0, "SNK",              "Zupapa!", &neogeo_ctrl, NULL )	/* Encrypted GFX */
	
/* Aicom */
GAMEB( 1994, janshin,  neogeo,   neogeo, neo320, mjneogeo,mjneogeo, ROT0, "Aicom", "Jyanshin Densetsu - Quest of Jongmaster", &neogeo_ctrl, NULL )
GAMEB( 1995, pulstar,  neogeo,   neogeo, ras320, neogeo,  neogeo,   ROT0, "Aicom", "Pulstar", &neogeo_ctrl, NULL )

/* Aiky */
GAMEB( 2003, pnyaa,    neogeo,   neogeo, neogeo, neogeo,  pnyaa,    ROT0, "Aiky", "Pochi and Nyaa", &neogeo_ctrl, NULL )

/* Data East Corporation */
GAMEB( 1996, ghostlop, neogeo,   neogeo, raster, neogeo,  neogeo,   ROT0, "Data East Corporation", "Ghostlop (Prototype)", &neogeo_ctrl, NULL )
GAMEB( 1993, spinmast, neogeo,   neogeo, raster, neogeo,  neogeo,   ROT0, "Data East Corporation", "Spin Master / Miracle Adventure", &neogeo_ctrl, NULL )
GAMEB( 1994, wjammers, neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "Data East Corporation", "Windjammers / Flying Power Disc", &neogeo_ctrl, NULL )
GAMEB( 1994, karnovr,  neogeo,   neogeo, raster, neogeo,  neogeo,   ROT0, "Data East Corporation", "Karnov's Revenge / Fighter's History Dynamite", &neogeo_ctrl, NULL )
GAMEB( 1994, strhoop,  neogeo,   neogeo, raster, neogeo,  neogeo,   ROT0, "Data East Corporation", "Street Hoop / Street Slam / Dunk Dream", &neogeo_ctrl, NULL )
GAMEB( 1996, magdrop2, neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "Data East Corporation", "Magical Drop II", &neogeo_ctrl, NULL )
GAMEB( 1997, magdrop3, neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "Data East Corporation", "Magical Drop III", &neogeo_ctrl, NULL )

/* Eleven */
GAMEB( 2000, nitd,     neogeo,   neogeo, neo320, neogeo,  nitd,     ROT0, "Eleven / Gavaking", "Nightmare in the Dark", &neogeo_ctrl, NULL ) /* Encrypted GFX */

/* Eolith */
GAMEB( 2001, kof2001,  neogeo,   neogeo, neogeo, neogeo,  kof2001,  ROT0, "Eolith", "The King of Fighters 2001", &neogeo_ctrl, NULL )
GAMEB( 2001, kof2001h, kof2001,  neogeo, neogeo, neogeo,  kof2001,  ROT0, "Eolith / SNK", "The King of Fighters 2001 (set 2)" , &neogeo_ctrl, NULL ) /* Encrypted GFX */
GAMEB( 2003, cthd2003, kof2001,  neogeo, neogeo, neogeo,  cthd2003, ROT0, "Eolith / SNK", "Crouching Tiger Hidden Dragon 2003 (The King of Fighters 2001 bootleg)", &neogeo_ctrl, NULL ) /* Protected Hack / Bootleg of kof2001 */
GAMEB( 2003, ct2k3sp,  kof2001,  neogeo, neogeo, neogeo,  ct2k3sp,  ROT0, "Eolith / SNK", "Crouching Tiger Hidden Dragon 2003 Super Plus (The King of Fighters 2001 bootleg)", &neogeo_ctrl, NULL ) /* Protected Hack / Bootleg of kof2001 */
GAMEB( 2002, kof2002,  neogeo,   neogeo, neogeo, neogeo,  kof2002,  ROT0, "Eolith / Playmore", "The King of Fighters 2002", &neogeo_ctrl, NULL )
GAMEB( 2002, kf2k2pls, kof2002,  neogeo, neogeo, neogeo,  kf2k2pls, ROT0, "bootleg", "The King of Fighters 2002 Plus (set 1, bootleg)", &neogeo_ctrl, NULL )
GAMEB( 2002, kf2k2pla, kof2002,  neogeo, neogeo, neogeo,  kf2k2pls, ROT0, "bootleg", "The King of Fighters 2002 Plus (set 2, bootleg)", &neogeo_ctrl, NULL )
GAMEB( 2002, kf2k2mp,  kof2002,  neogeo, neogeo, neogeo,  kf2k2mp,  ROT0, "bootleg", "The King of Fighters 2002 Magic Plus (bootleg)", &neogeo_ctrl, NULL )
GAMEB( 2002, kf2k2mp2, kof2002,  neogeo, neogeo, neogeo,  kof2km2,  ROT0, "bootleg", "The King of Fighters 2002 Magic Plus II (bootleg)" , &neogeo_ctrl, NULL ) 
GAMEB( 2004, kof2k4se, kof2002,  neogeo, neogeo, neogeo,  kof2k4se, ROT0, "bootleg", "The King of Fighters Special Edition 2004 (The King of Fighters 2002 bootleg)", &neogeo_ctrl, NULL ) /* Hack / Bootleg of kof2002 */
GAMEB( 2003, kof10th,  kof2002,  neogeo, neogeo, neogeo,  kof10th,  ROT0, "bootleg", "The King of Fighters 10th Anniversary", &neogeo_ctrl, NULL )
GAMEB( 2005, kf10thep, kof2002,  neogeo, neogeo, neogeo,  kf10thep, ROT0, "bootleg", "The King of Fighters 10th Anniversary Extra Plus (The King of Fighters 2002 bootleg)", &neogeo_ctrl, NULL ) /* fake SNK copyright */
GAMEB( 2004, kf2k5uni, kof2002,  neogeo, neogeo, neogeo,  kf2k5uni, ROT0, "bootleg", "The King of Fighters 10th Anniversary 2005 Unique (The King of Fighters 2002 bootleg)", &neogeo_ctrl, NULL )

/* Evoga */
GAMEB( 2002, rotd,     neogeo,	 neogeo, neogeo, neogeo,  rotd,	    ROT0, "Evoga / Playmore", "Rage of the Dragons", &neogeo_ctrl, NULL )

/* Face */
GAMEB( 1994, gururin,  neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "Face", "Gururin", &neogeo_ctrl, NULL )
GAMEB( 1997, miexchng, neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "Face", "Money Puzzle Exchanger / Money Idol Exchanger", &neogeo_ctrl, NULL )

/* Hudson Soft */
GAMEB( 1994, panicbom, neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "Eighting / Hudson", "Panic Bomber", &neogeo_ctrl, NULL )
GAMEB( 1995, kabukikl, neogeo,   neogeo, neo320, neogeo,  neogeo,   ROT0, "Hudson", "Far East of Eden - Kabuki Klash / Tengai Makyou - Shin Den", &neogeo_ctrl, NULL )
GAMEB( 1997, neobombe, neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "Hudson", "Neo Bomberman", &neogeo_ctrl, NULL )

/* Mega Enterprise */
GAMEB( 2002, mslug4,   neogeo,   neogeo, neogeo, neogeo,  mslug4,   ROT0, "Mega", "Metal Slug 4", &neogeo_ctrl, NULL )


/* Monolith Corp. */
GAMEB( 1990, minasan,  neogeo,   neogeo, neogeo, mjneogeo,mjneogeo, ROT0, "Monolith Corp.", "Minnasanno Okagesamadesu", &neogeo_ctrl, NULL )
GAMEB( 1991, bakatono, neogeo,   neogeo, neogeo, mjneogeo,mjneogeo, ROT0, "Monolith Corp.", "Bakatonosama Mahjong Manyuki", &neogeo_ctrl, NULL )

/* Nazca */
GAMEB( 1996, turfmast, neogeo,   neogeo, raster, neogeo,  neogeo,   ROT0, "Nazca", "Neo Turf Masters / Big Tournament Golf", &neogeo_ctrl, NULL )
GAMEB( 1996, mslug,    neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "Nazca", "Metal Slug - Super Vehicle-001", &neogeo_ctrl, NULL )

/* NMK */
GAMEB( 1994, zedblade, neogeo,   neogeo, raster, neogeo,  neogeo,   ROT0, "NMK", "Zed Blade / Operation Ragnarok", &neogeo_ctrl, NULL )

/* Noise Factory */
GAMEB( 2003, matrim,   neogeo,   neogeo, neogeo, neogeo,  matrim,   ROT0, "Atlus / Noise Factory", "Power Instinct Matrimelee", &neogeo_ctrl, NULL )
GAMEB( 2001, sengoku3, neogeo,   neogeo, neogeo, neogeo,  sengoku3, ROT0, "Noise Factory", "Sengoku 3", &neogeo_ctrl, NULL )

/* Psikyo */
GAMEB( 1999, s1945p,   neogeo,   neogeo, neo320, neogeo,  s1945p,   ROT0, "Psikyo", "Strikers 1945 Plus", &neogeo_ctrl, NULL )	/* Encrypted GFX */

/* Sammy */
GAMEB( 1992, viewpoin, neogeo,   neogeo, raster, neogeo,  neogeo,   ROT0, "Sammy", "Viewpoint", &neogeo_ctrl, NULL )

/* Saurus */
GAMEB( 1995, quizkof,  neogeo,   neogeo, raster, neogeo,  neogeo,   ROT0, "Saurus", "Quiz King of Fighters", &neogeo_ctrl, NULL )
GAMEB( 1995, stakwin,  neogeo,   neogeo, neo320, neogeo,  neogeo,   ROT0, "Saurus", "Stakes Winner / Stakes Winner - GI kinzen seihae no michi", &neogeo_ctrl, NULL )
GAMEB( 1996, ragnagrd, neogeo,   neogeo, neo320, neogeo,  neogeo,   ROT0, "Saurus", "Ragnagard / Shin-Oh-Ken", &neogeo_ctrl, NULL )
GAMEB( 1996, pgoal,    neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "Saurus", "Pleasure Goal / Futsal - 5 on 5 Mini Soccer", &neogeo_ctrl, NULL )
GAMEB( 1996, stakwin2, neogeo,   neogeo, neo320, neogeo,  neogeo,   ROT0, "Saurus", "Stakes Winner 2", &neogeo_ctrl, NULL )
GAMEB( 1997, shocktro, neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "Saurus", "Shock Troopers (set 1)", &neogeo_ctrl, NULL )
GAMEB( 1997, shocktra, shocktro, neogeo, neogeo, neogeo,  neogeo,   ROT0, "Saurus", "Shock Troopers (set 2)", &neogeo_ctrl, NULL )
GAMEB( 1998, shocktr2, neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "Saurus", "Shock Troopers - 2nd Squad", &neogeo_ctrl, NULL )
GAMEB( 1996, ironclad, neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "Saurus", "Choutetsu Brikin'ger - Ironclad (Prototype)", &neogeo_ctrl, NULL )

/* SNK Playmore */
GAME ( 2003, svcpcb,   0,                neogeo, svcpcb,  svcpcb,   ROT0, "Playmore", "SvC Chaos - SNK vs Capcom (JAMMA PCB)" ) /* not a clone of neogeo because it's NOT a neogeo cart. */
GAMEB( 2003, svc,      neogeo,   neogeo, neogeo, neogeo,  svc,      ROT0, "Playmore", "SvC Chaos - SNK vs Capcom (MVS)", &neogeo_ctrl, NULL )
GAMEB( 2003, svcboot,  svc,      neogeo, neogeo, neogeo,  svcboot,  ROT0, "bootleg",  "SvC Chaos - SNK vs Capcom (Bootleg)", &neogeo_ctrl, NULL )
GAMEB( 2003, svcplus,  svc,      neogeo, neogeo, neogeo,  svcplus,  ROT0, "bootleg",  "SvC Chaos - SNK vs Capcom Plus (set 1, bootleg)", &neogeo_ctrl, NULL )
GAMEB( 2003, svcplusa, svc,      neogeo, neogeo, neogeo,  svcplusa, ROT0, "bootleg",  "SvC Chaos - SNK vs Capcom Plus (set 2, bootleg)", &neogeo_ctrl, NULL )
GAMEB( 2003, svcsplus, svc,      neogeo, neogeo, neogeo,  svcsplus, ROT0, "bootleg",  "SvC Chaos - SNK vs Capcom Super Plus (bootleg)", &neogeo_ctrl, NULL )
GAMEB( 2003, mslug5,   neogeo,   neogeo, neogeo, neogeo,  mslug5,   ROT0, "SNK Playmore", "Metal Slug 5", &neogeo_ctrl, NULL )
GAME ( 2003, kf2k3pcb, 0,                neogeo, neogeo,  kf2k3pcb, ROT0, "SNK Playmore", "The King of Fighters 2003 (Japan, JAMMA PCB)" ) /* not a clone of neogeo because it's NOT a neogeo cart. */
GAMEB( 2003, kof2003,  neogeo,   neogeo, neogeo, neogeo,  kof2003,  ROT0, "SNK Playmore", "The King of Fighters 2003 (World / US, MVS)", &neogeo_ctrl, NULL )
GAMEB( 2003, kof2003d, kof2003,  neogeo, neogeo, neogeo,  kof2003d, ROT0, "bootleg", "The King of Fighters 2003 (Decrypted)", &neogeo_ctrl, NULL )
GAMEB( 2003, kf2k3bl,  kof2003,  neogeo, neogeo, neogeo,  kf2k3bl,  ROT0, "bootleg", "The King of Fighters 2003 (bootleg set 1)", &neogeo_ctrl, NULL ) /* zooming is wrong because its a bootleg of the pcb version on a cart (unless it was a bootleg pcb with the new bios?) */
GAMEB( 2003, kf2k3bla, kof2003,  neogeo, neogeo, neogeo,  kf2k3pl,  ROT0, "bootleg", "The King of Fighters 2003 (bootleg set 2)", &neogeo_ctrl, NULL ) /* zooming is wrong because its a bootleg of the pcb version on a cart */
GAMEB( 2003, kf2k3pl,  kof2003,  neogeo, neogeo, neogeo,  kf2k3pl,  ROT0, "bootleg", "The King of Fighters 2004 Plus / Hero (The King of Fighters 2003 bootleg)", &neogeo_ctrl, NULL ) /* zooming is wrong because its a bootleg of the pcb version on a cart */
GAMEB( 2003, kf2k3upl, kof2003,  neogeo, neogeo, neogeo,  kf2k3upl, ROT0, "bootleg", "The King of Fighters 2004 Ultra Plus (The King of Fighters 2003 bootleg)", &neogeo_ctrl, NULL ) /* zooming is wrong because its a bootleg of the pcb version on a cart */
	
/* Sunsoft */
GAMEB( 1995, galaxyfg, neogeo,   neogeo, raster, neogeo,  neogeo,   ROT0, "Sunsoft", "Galaxy Fight - Universal Warriors", &neogeo_ctrl, NULL )
GAMEB( 1996, wakuwak7, neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "Sunsoft", "Waku Waku 7", &neogeo_ctrl, NULL )

/* Taito */
GAMEB( 1994, pbobblen, neogeo,   neogeo, neo320, neogeo,  neogeo,   ROT0, "Taito", "Puzzle Bobble / Bust-A-Move (Neo-Geo) (set 1)", &neogeo_ctrl, NULL )
GAMEB( 1994, pbobblna, pbobblen, neogeo, neo320, neogeo,  neogeo,   ROT0, "Taito", "Puzzle Bobble / Bust-A-Move (Neo-Geo) (set 2)", &neogeo_ctrl, NULL )
GAMEB( 1999, pbobbl2n, neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "Taito (SNK license)", "Puzzle Bobble 2 / Bust-A-Move Again (Neo-Geo)", &neogeo_ctrl, NULL )

/* Takara */
GAMEB( 1995, marukodq, neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "Takara", "Chibi Marukochan Deluxe Quiz", &neogeo_ctrl, NULL )

/* Technos */
GAMEB( 1995, doubledr, neogeo,   neogeo, ras320, neogeo,  neogeo,   ROT0, "Technos", "Double Dragon (Neo-Geo)", &neogeo_ctrl, NULL )
GAMEB( 1995, gowcaizr, neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "Technos", "Voltage Fighter - Gowcaizer / Choujin Gakuen Gowcaizer", &neogeo_ctrl, NULL )
GAMEB( 1996, sdodgeb,  neogeo,   neogeo, raster, neogeo,  neogeo,   ROT0, "Technos", "Super Dodge Ball / Kunio no Nekketsu Toukyuu Densetsu", &neogeo_ctrl, NULL )

/* Tecmo */
GAMEB( 1996, tws96,    neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "Tecmo", "Tecmo World Soccer '96", &neogeo_ctrl, NULL )

/* Yumekobo */
GAMEB( 1998, blazstar, neogeo,   neogeo, neo320, neogeo,  neogeo,   ROT0, "Yumekobo", "Blazing Star", &neogeo_ctrl, NULL )
GAMEB( 1999, preisle2, neogeo,   neogeo, neogeo, neogeo,  preisle2, ROT0, "Yumekobo", "Prehistoric Isle 2", &neogeo_ctrl, NULL ) /* Encrypted GFX */

/* Viccom */
GAMEB( 1994, fightfev, neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "Viccom", "Fight Fever (set 1)", &neogeo_ctrl, NULL )
GAMEB( 1994, fightfva, fightfev, neogeo, neogeo, neogeo,  neogeo,   ROT0, "Viccom", "Fight Fever (set 2)", &neogeo_ctrl, NULL )

/* Video System Co. */
GAMEB( 1994, pspikes2, neogeo,   neogeo, ras320, neogeo,  neogeo,   ROT0, "Video System Co.", "Power Spikes II", &neogeo_ctrl, NULL )
GAMEB( 1994, sonicwi2, neogeo,   neogeo, neo320, neogeo,  neogeo,   ROT0, "Video System Co.", "Aero Fighters 2 / Sonic Wings 2", &neogeo_ctrl, NULL )
GAMEB( 1995, sonicwi3, neogeo,   neogeo, neo320, neogeo,  neogeo,   ROT0, "Video System Co.", "Aero Fighters 3 / Sonic Wings 3", &neogeo_ctrl, NULL )
GAMEB( 1997, popbounc, neogeo,   neogeo, neogeo, popbounc,popbounc, ROT0, "Video System Co.", "Pop 'n Bounce / Gapporin", &neogeo_ctrl, NULL )

/* Yuki Enterprises */
GAMEB( 2003, samsho5,  neogeo,   neogeo, raster, neogeo,  samsho5,  ROT0, "Yuki Enterprise / SNK Playmore", "Samurai Shodown V", &neogeo_ctrl, NULL )
GAMEB( 2003, samsh5sp, neogeo,	 neogeo, neogeo, neogeo,  samsh5sp, ROT0, "Yuki Enterprise / SNK Playmore", "Samurai Shodown V Special", &neogeo_ctrl, NULL )

/* Visco */
GAMEB( 1992, androdun, neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "Visco", "Andro Dunos", &neogeo_ctrl, NULL )
GAMEB( 1995, puzzledp, neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "Taito (Visco license)", "Puzzle De Pon!", &neogeo_ctrl, NULL )
GAMEB( 1996, neomrdo,  neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "Visco", "Neo Mr. Do!", &neogeo_ctrl, NULL )
GAMEB( 1995, goalx3,   neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "Visco", "Goal! Goal! Goal!", &neogeo_ctrl, NULL )
GAMEB( 1996, neodrift, neogeo,   neogeo, raster, neogeo,  neogeo,   ROT0, "Visco", "Neo Drift Out - New Technology", &neogeo_ctrl, NULL )
GAMEB( 1996, breakers, neogeo,   neogeo, neo320, neogeo,  neogeo,   ROT0, "Visco", "Breakers", &neogeo_ctrl, NULL )
GAMEB( 1997, puzzldpr, puzzledp, neogeo, neogeo, neogeo,  neogeo,   ROT0, "Taito (Visco license)", "Puzzle De Pon! R!", &neogeo_ctrl, NULL )
GAMEB( 1998, breakrev, breakers, neogeo, neo320, neogeo,  neogeo,   ROT0, "Visco", "Breakers Revenge", &neogeo_ctrl, NULL )
GAMEB( 1998, flipshot, neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "Visco", "Battle Flip Shot", &neogeo_ctrl, NULL )
GAMEB( 1999, ctomaday, neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "Visco", "Captain Tomaday", &neogeo_ctrl, NULL )
GAMEB( 1999, ganryu,   neogeo,   neogeo, neogeo, neogeo,  ganryu,   ROT0, "Visco", "Musashi Ganryuuki", &neogeo_ctrl, NULL )	/* Encrypted GFX */
GAMEB( 2000, bangbead, neogeo,   neogeo, raster, neogeo,  bangbead, ROT0, "Visco", "Bang Bead", &neogeo_ctrl, NULL )
GAMEB( 1994, b2b,      neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "Visco", "Bang Bang Busters", &neogeo_ctrl, NULL )

/* NG:DEV.TEAM */
GAMEB( 2005, lasthope, neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "NG:DEV.TEAM", "Last Hope", &neogeo_ctrl, NULL )

/* Bitmap Bureau */
GAMEB( 2019, xeno,     neogeo,   neogeo, neogeo, neogeo,  neogeo,   ROT0, "Bitmap Bureau", "Xeno Crisis (Neo Geo MVS)", &neogeo_ctrl, NULL )

/* M.Priewe */
GAMEB( 2021, hypernoid, neogeo, neogeo, neogeo, neogeo, neogeo, ROT0, "M.Priewe", "Hypernoid (2021-11-28)", &neogeo_ctrl, NULL )

#if 0
GAMEB( 2000, bangbedp, bangbead, neogeo, raster, neogeo,  neogeo,   ROT0, "Visco", "Bang Bead (prototype)", &neogeo_ctrl, NULL )
#endif
