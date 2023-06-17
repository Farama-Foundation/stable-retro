/**
 * driver\namcos22.c
 *
 * This driver describes Namco's System22 and Super System 22 hardware.
 * Currently, only Prop Cycle is fully playable.
 *
 * feel free to contact pstroffo@yahoo.com (Phil Stroffolino) with any questions.
 *
 *
 * Hardware Notes:
 * - Serial controller is C139 SCI (same as System21).
 * - S22 has two TI320C25 DSP (printed as C71).
 * - Main DSP performs handling display list and matrix operation, and kicks hardware.
 * - Sub DSP may calculates shadings and sub operations.
 * - main CPU copies some DSP code to shared RAM on start
 * - C74 is sound MCU, Mitsubishi M37702 MCU with mask ROM
 * - some external subroutines for C74 is also embedded
 * - Super System22 has a different memory map, sound system, and an additional 2d sprite layer.
 *
 **********************************************************************************************************
 *
 * Known issues:
 * - no emulation of the MCU.  The MCU provides sound and input port handling.  Proper emulation will
 *   obviate the need for a great many hacks in this driver.
 *
 * - no emulation of the DSP processors.  The DSP processors are used as slaves to render 3d scenes.
 *   All current functionality is simulated based on a study of the raw scene description and known behaviors
 *   of the games running on real hardware.  A proper emulation of the DSP processors will provide two things:
 *   1. Eliminate guesswork and errors in our simulation.  In particular, depth BIAS feature, used to
 *      manage draw order is fairly complex.
 *   2. Allow any game-specific DSP code to function correctly.  It seems that the DSP functionality varies
 *      very little from one game to the next.  But each game includes one or more self tests that involve
 *      custom code.  And an understanding of the DSP code might help to fix games like Alpine Ski or Air Combat22.
 *
 * - the intensity values used with gouraud shading aren't being interpretted quite right.
 *   Cap of vivanonno team suggests to interpret it in two ways:
 *			(a) 0x00 <= value <= 0x40 : simple scaling for source color. 0x40 means original color.
 *			(b) 0x40 <  value <= 0xff : saturates to white color. (interpolation or add).
 * 			The master contrast fader at 0x90020000 also can be fed such a strong power used for white-out.
 *
 * - our video driver does not yet handle orthogonal fader controls, or polygon-tilemap priority.
 *   On real hardware, it is possible to fade the polygon layer as a whole independently from the sprite layer
 *   or tilemap layer.
 *   And on real hardware, it is possible for a polygon to be marked so that it appears in front or behind
 *   the tilemap layer.
 *
 * Time Crisis notes:
 * - some memory tests fail because of MCU/DSP hacks.  These can be patched out or skipped using MAME's debugger,
 *   but even if they all pass, the game does not start up.  It may be something simple - missing status bits or
 *   bad interrupt handling.
 *
 * Prop Cycle notes:
 * - motor output: fan, blows air towards player when flying fast
 * - lamp output: start button can light up
 * - steering yoke should be analog
 * - could use a new analog input port type for pedal speed
 * - needs nvmem save
 *
 *	The "dipswitch" settings are ignored - this isn't a bug.  The Prop Cycle software
 *  explicitly clears the chunk of work RAM used to cache the 8 bit dipswitch value
 *  immediately after populating it.  This is done to hide secret debug routines from
 *	release versions of the game.
 *
 *	When the red namco logos slides in from top right and bottom left during splash screen,
 *	it is supposed to be translucent with respect to the polygon layer
 *
 * TBA:
 *	- fix Alpine Racer (Super System22) - encrypted point ROMs
 *
 * Special thanks to "team vivanonno" for System22 memory map, and insights on object list
 * parsing.
 *
 **********************************************************************************************************
 *
 * Detailed Hardware Notes:
 *
 * Following is a Memory Map for SYSTEM22 hardware courtesy of cap@vivanonno team (22/07/2003)

0x00000000 - 0x001fffff	Program ROM (2M bytes)

			Mounted position:
				LLB: CPU 4D
				LMB: CPU 2D
				UMB: CPU 8D
				UUB: CPU 6D

			Known ROM chip type:
				TI TMS27C040-10
				ST M27C4001-10
				M27C4001-12Z

0x10000000 - 0x1001ffff	Main RAM (128K bytes)

			Mounted position:
				CPU 3D, 5D, 7D, 9D

			Known DRAM chip type:
				TC55328P-25
				N3441256P-15

0x18000000 - 0x1801ffff	Main RAM (Mirror or Another Bank)

			Mounted position:
				unknown

0x20000000 - 0x2000000f	KEYCUS

			Mounted position:
				CPU 13R
			Known chip type:
				C370 (Ridge Racer, Ridge Racer 2)
				C388 (Rave Racer)
				C389? (Cyber Cycles)
				C392? (Ace Driver Victory Lap)

			Notes (game-specific hacks):
			 Ridge Racer
				0x20000008 = 0x0172 (= 370)
			 Ridge Racer 2
				0x20000000 = 0x0172 (= 370)
				0x20000008 = 0x0172 (= 370)
			 Rave Racer
				hacked temporarily:
				0x00018200 = 0x60 (-> BRA instruction) (RV2-B)
			 Cyber Cycles
				0x20000002 = 0x0185 (= 389)
			 Ace Driver Victory Lap
				0x20000004 = 0x0188 (= 392)

0x20010000 - 0x20013fff	C139 SCI Buffer

			Mounted position:
				CPU 4N
			Known chip type:
				M5M5179AP-25 (8k x 9bit SRAM)
			Note:
				Boot time check:
				20010000 - 20011fff / bits=0x000001ff

	20010000 - 20011fff	TX Buffer
	20012000 - 20013fff	RX FIFO Buffer (also used for TX Buffer)

0x20020000 - 0x2002000f	C139 SCI Register

			Mounted position:
				CPU 4R

	20020000  2	R/W	RX Status
				0x01 : Frame Error
				0x02 : Frame Received
				0x04 : ?

	20020002  2	R/W	Status/Control Flags
				0x01 :
				0x02 : RX flag? (cleared every vsync)
				0x04 : RX flag? (cleared every vsync)
				0x08 :

	20020004  2	W	FIFO Control Register
				0x01 : sync bit enable?
				0x02 : TX FIFO sync bit (bit-8)

	20020006  2	W	TX Control Register
				0x01 : TX start/stop
				0x02 : ?
				0x10 : ?

	20020008  2	W	-
	2002000a  2	W	TX Frame Size
	2002000c  2	R/W	RX FIFO Pointer (0x0000 - 0x0fff)
	2002000e  2	W	TX FIFO Pointer (0x0000 - 0x1fff)

0x40000000 - 0x4000001f	System Controller

			Note:
				Interrupt Control
				Peripheral Control

	40000000	IRQ (unknown)
	40000001
	40000002	SCI IRQ level
	40000003	IRQ (unknown)
	40000004	VSYNC IRQ level

	40000005	IRQ (unknown) acknowledge
	40000006
	40000007	SCI IRQ acknowledge
	40000008	IRQ (unknown) acknowledge
	40000009	VSYNC IRQ acknowledge

	4000000a
	4000000b	?
	4000000c	?
	4000000d
	4000000e	?
	4000000f
	40000010	?
	40000011	?
	40000012	?
	40000013	?
	40000014	?
	40000015	? (cyc1)
	40000016	Watchdog timer reset
	40000017
	40000018	0 or 1 -> DSP control (reset?)
	40000019	sub cpu reset?
	4000001a	0 or 1 or 0xff -> DSP control
	4000001b	?
	4000001c
	4000001d
	4000001e
	4000001f

0x48000000 - 0x4800003f	unknown (option)

			Notes:
				this device maybe optional.
				zero means not-connected.
				this may relate to device at 0x94000000

0x50000000 - 0x50000fff	DIPSW etc.

	0x50000000		DIPSW3
	0x50000001		DIPSW2

0x58000000 - 0x58001fff	EEPROM

			Mounted position:
				CPU 9E

			Known chip type:
				HN58C65P-25 (8k x 8bit EEPROM)

0x60004000 - 0x6000bfff	C74 (Mitsubishi M37702 MCU) Shared RAM

			Mounted position:
				CPU 11L, 12L

			Known chip type:
				TC55328P-25
				N341256P-15

			DATA ROM for C74 (SEQ data and external code):
			Known chip type:
				NEC D27C4096D-12

			Notes:
				C74(CPU PCB) sends/receives I/O data from C74(I/O PCB) by SIO.

	0x60004020 b4 = 1 : ???
	0x60004022.w		Volume(R)
	0x60004024.w		Volume(L)
	0x60004026.w		Volume(R) (maybe rear channels, not put on real PCB)
	0x60004028.w		Volume(L) (maybe rear channels, not put on real PCB)
	0x60004030 b0     : system type 0
	0x60004030 b1 = 0 : COIN2
	0x60004030 b2 = 0 : TEST SW
	0x60004030 b3 = 0 : SERVICE SW
	0x60004030 b4 = 0 : COIN1
	0x60004030 b5     : system type 1
				(system type on RR2 (00:50inch, 01:two in one, 20: standard, 21:
deluxe))
	0x60004031 b0 = 0 : SWITCH1 (for manual transmission)
	0x60004031 b1 = 0 : SWITCH2
	0x60004031 b2 = 0 : SWITCH3
	0x60004031 b3 = 0 : SWITCH4
	0x60004031 b4 = 0 : CLUTCH
	0x60004031 b6 = 0 : VIEW SW
	0x60004032.w		Handle A/D (=steering wheel, default of center value is
different in each game)
	0x60004034.w		Gas A/D
	0x60004036.w		Brake A/D
	0x60004038.w		A/D3 (reserved)
				(some GOUT (general outputs for lamps, etc) is also mapped this
area)
	0x60004080		Data/Code for Sub-CPU
	0x60004200		Data/Code for Sub-CPU

	0x60005000 - 0x6000bfff	Sound Work
	+0x0000 - 0x003f	Song Request #00 to 31
	+0x0100 - 0x02ff	Parameter RAM from Main MPU (for SEs)
	+0x0300 - 0x03ff?	Song Title (put messages here from Sound CPU)

0x70000000 - 0x70020000	C71 (TI TMS320C25 DSP) Shared RAM

			Mounted position:
				C71: CPU 15R, 21R
				RAM: CPU 15K, 13E, 12E
			Known chip type:
				TC55328P-25
				N341256P-15

			Notes:
				connected bits = 0x00ffffff (24bit)

	0x70000040.l		Point RAM Write Enable (ff=enable)
	0x70000044.l		Point RAM IDC (index count register)
	0x70000c00 - 0x7000ffff	Point RAM Port
				(Point RAM has 128K words, 512KB space (128K24bit))
	0x70010000 - 0x700103ff Window Attribute Register #1 (0x808)
	0x70010400 - 0x70017fff Display List Buffer #1
	0x70018000 - 0x700183ff Window Attribute Register #2 (0x808)
	0x70018400 - 0x7001ffff Display List Buffer #2

0x90000000 - 0x90000000	LED on PCB(?)

0x90010000 - 0x90017fff	Depth-cueing Look-up Table (fog density between
near
to far)

			Mounted position:
				VIDEO 8P
			Known chip type:
				TC55328P-25

0x90020000 - 0x90027fff	C305 (Display Controller)

			Mounted position:
				VIDEO 7D (C305)

	+0x0002.w	Fader Enable(?) (0: disabled)
	+0x0011.w	Display Fader (R) (0x0100 = 1.0)
	+0x0013.w	Display Fader (G) (0x0100 = 1.0)
	+0x0015.w	Display Fader (B) (0x0100 = 1.0)
	+0x0100.b	Fog1 Color (R) (world fogging)
	+0x0101.b	Fog2 Color (R) (used for heating of brake-disc on RV1)
	+0x0180.b	Fog1 Color (G)
	+0x0181.b	Fog2 Color (G)
	+0x0200.b	Fog1 Color (B)
	+0x0201.b	Fog2 Color (B)
			(many unknown registers are here)

			Notes:
				Boot time check: 0x90020100 - 0x9002027f

0x90028000 - 0x9002ffff	Palette (R)
0x90030000 - 0x90037fff	Palette (G)
0x90038000 - 0x9003ffff	Palette (B)

			Mounted position:
				VIDEO 6B, 7B, 8B (near C305)
			Note:
				0xff00-0xffff are for Tilemap (16 x 16)

0x90040000 - 0x9005ffff	unknown (option)

			Note:
				This device maybe optional.
				this may relate to device at 0x40000000

0x90080000 - 0x9009dfff	Tilemap PCG Memory
0x9009e000 - 0x9009ffff	Tilemap Memory (64 x 64)

			Mounted position:
				VIDEO  2K
			Known chip type:
				HM511664 (64k x 16bit SRAM)
			Note:
				Self test: 90084000 - 9009ffff

0x900a0000 - 0x900a000f	Tilemap Register

			Mounted position:
				unknown

	+0x0000			Position X
	+0x0002			Position Y



SYSTEM22 Known Custom Chips

CPU PCB:

C71 TI TMS320C25 DSP
	C71 WEYW40116 (TMS320C25 Main/Sub DSP)
	C71 D72260FN 980 FE-5CA891W

	M5M5178AP-25 (CPU 16R, 17R, 19R, 20R) DSP Work RAM (8K x 8bit x 2 x 2)

	Notes:
		NOP opcode: 0x5500
		RET opcode: 0xce26

C74 Mitsubishi M37702 MCU
	C74 159 408600 (OLD SUB)
	C74 159 543100 (NEW SUB)
	C74 159 414600 (OLD I/O)
	C74 159 437600 (NEW I/O)

C195 (Shared SRAM Controller)
C196 CPP x 6
C199 (CPU 18K) x 1
C317 IDC (CPU 15E) x 1 (S21B)
C337 PFP x 1
C342 x 1 (S21B)
C352 (32ch PCM)
C353 x 1


VIDEO PCB:

C305 (Palette)
C335
	9C, 10C
	12D, 12E
	14C
C300
	18B, 18C, 20B, 20C, 22B, 22C
Cxxx
	34R, 35R

TI TBP28L22N (256 x 8bit PROM)
	VIDEO 2D, 3D, 4D (RGB Gamma LUT ROM)

	Notes:
		RR1.GAM (for Ridge Raecr 1/2, Rave Racer)

**********************************************************************************************************

Alpine Racer (VER. D)
Namco, 1995

  Player stands on steps, steering left/right.
  Inner edge allows quick movement.

  The "viewpoint" button is used to select game mode
  and to change camera placement during gameplay.

1ST PCB
-------
PCB NO: SYSTEM SUPER22 CPU PCB 8646960102 (8646970102)
CPU   : 68EC020FG25
OSCs  : 40.000MHz, 49.1520MHz
RAMs  : IS61C256AH-20J (x2), CY7C182-25VC (x1)
DIPSW : 8 Position (x1), 4 Position (x1)
PALs  : PALCE 22V10H-15JC/4 (x3, labelled SS22C1, SS22C2, SS22C4)
CUSTOM: C352 (100 PIN PQFP)
        C405 (176 PIN PQFP)
        C391 (KEYCUS)
        137
        139  (64 PIN PQFP)
        C383 (100 PIN PQFP)
        M37710S4BFP (80 PIN PQFP)
OTHER : AT28C64 (EEPROM)
ROMs  : AR1DATAB.8K (near C405, 27C4096)
        AR1WAVEA.2L (near C352, 16M MASK)

        Tiny ROM PCB labelled AR2 VER. D
        SYSTEM SUPER22 MPM(F) PCB 8646961600 (8646971600)
        containing main program ....
                                     ROM1.BIN (INTEL FLASH, TYPE E28F008SA, TSOP40)
                                     ROM2.BIN         "
                                     ROM3.BIN         "
                                     ROM4.BIN         "



2ND PCB
-------
PCB NO: SYSTEM SUPER22 DSP PCB 8646960302 (8646970302)
OSC   : 40.0000MHz
PALs  : PALCE20V8H (Labelled SS22D1)
        PALCE16V8  (x4, Labelled SS22D2, SS22D3, SS22D4, SS22D5)
RAMs  : CY7C199-25VC (x9), M5M51008AFP-70L (x6)
CUSTOM: C396 (160 PIN PQFP)
        C71  (x2, 68 PIN PLCC)
        C199 (100 PIN PQFP)
        C353 (120 PIN PQFP)
        C402 (x2, 144 PIN PQFP)
        C403 (136 PIN PQFP)
        C300 (160 PIN PQFP)
        C342 (160 PIN PQFP)
        C405 (x2, 176 PIN PQFP)
        C379 (64 PIN PQFP)



3RD PCB
-------

PCB No: SYSTEM SUPER22 MROM PCB 8646960400 (8646970400)
PALs  : Labelled SS22M3, SS22M2, SS22M2 & SS22M1
RAMs  : TC551001BFL-70L (x3)
ROMs  : GFX....MANY, MANY, MANY! All ROMs are surface mounted

                                  Type
        ----------------------------------
        ar1ccrl.7b = ar1ccrl.3d   16M SOP44
        ar1ccrh.5b = ar1ccrh.1d   4M SOP32

        ar1cg1.13b = ar1cg1.10d   16M SOP44
        ar1cg2.14b = ar1cg2.12d      "
        ar1cg3.16b = ar1cg3.13d      "
        ar1cg4.18b = ar1cg4.14d      "
        ar1cg5.19b = ar1cg5.16d      "
        ar1cg6.18a = ar1cg6.18d      "
        ar1cg7.15a = ar1cg7.19d      "

        ar1scg0.12f= ar1scg0.12l  16M SOP44 (These have identical halves but)
        ar1scg1.10f= ar1scg1.10l     "      (     they *are* 16M ROMs       )

        ar1ptr*                   4M SOP32



4TH PCB
-------
PCB NO: SYSTEM SUPER22 VIDEO 8646960204 (8646970204)
OSC   : 51.2000MHz (near PLCC84)
RAMs  : KM424C257 (x22), IS61C256 (x7), LC321664 (x1)
CUSTOM: 304 (x4, 120 PIN PQFP)
        361 (120 PIN PQFP)
        374 (160 PIN PQFP)
        381 (x2, 144 PIN PQFP)
        395 (? - forgot to note leg count!)
        397 (160 PIN PQFP)
        399 (160 PIN PQFP)
        400 (x3, 100 PIN PQFP)
        401 (x5, 64 PIN PQFP)
        404 (208 PIN PQFP)
        406 (120 PIN PQFP)
        SONY CXD1178Q (48 PIN PQFP)
        ALTERA EPM7064LC84-15 (84 PIN PLCC)

	Notes on input ports:

	0xe00098: dsw
	0xe0009c: up/down

  ----------------------------
	e000a8.l	IO TR

	e000b6.l VR 0
	e000ba.l VR 1
	e000be.l VR 2
	e000c2.l VR 3

	e000a4: ?

	e000a6.w // IO SW
		bit 2: SERVICE SW
		bit 3: TEST SW
		bit 4: MID SW
		bit 5: LEFT SW
		bit 6: RIGHT SW

	e000b2.l EDGE VR
	e000ae.l SWING VR
*/

#include "namcos22.h"

enum namcos22_gametype namcos22_gametype; /* used for game-specific hacks */

/* memory region pointers */
static data32_t *namcos22_shareram;
static data32_t *namcos22_C139_SCI;
static data32_t *namcos22_system_controller;

/* Super System22 supports a sprite layer.
 * Sprites are rendered as part of the polygon draw list, based on a per-sprite Z attribute.
 * Each sprite has explicit placement/color/zoom controls.
 */
static struct GfxLayout sprite_layout =
{
	32,32,
	RGN_FRAC(1,1),
	8,
	{ 0,1,2,3,4,5,6,7 },
	{
		0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8,
		8*8,9*8,10*8,11*8,12*8,13*8,14*8,15*8,
		16*8,17*8,18*8,19*8,20*8,21*8,22*8,23*8,
		24*8,25*8,26*8,27*8,28*8,29*8,30*8,31*8 },
	{
		0*32*8,1*32*8,2*32*8,3*32*8,4*32*8,5*32*8,6*32*8,7*32*8,
		8*32*8,9*32*8,10*32*8,11*32*8,12*32*8,13*32*8,14*32*8,15*32*8,
		16*32*8,17*32*8,18*32*8,19*32*8,20*32*8,21*32*8,22*32*8,23*32*8,
		24*32*8,25*32*8,26*32*8,27*32*8,28*32*8,29*32*8,30*32*8,31*32*8 },
	32*32*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &sprite_layout,  0, 0x80 },
	{ -1 },
};

/*********************************************************************************/

/* prelim! */
static READ32_HANDLER( namcos22_C139_SCI_r )
{
	switch( offset )
	{
	case 0x0/4: return 0x0004<<16;
	default: return 0;
	}
}

static WRITE32_HANDLER( namcos22_C139_SCI_w )
{
	COMBINE_DATA( &namcos22_C139_SCI[offset] );
	/*
	20020000  2	R/W	RX Status
				0x01 : Frame Error
				0x02 : Frame Received
				0x04 : ?

	20020002  2	R/W	Status/Control Flags
				0x01 :
				0x02 : RX flag? (cleared every vsync)
				0x04 : RX flag? (cleared every vsync)
				0x08 :

	20020004  2	W	FIFO Control Register
				0x01 : sync bit enable?
				0x02 : TX FIFO sync bit (bit-8)

	20020006  2	W	TX Control Register
				0x01 : TX start/stop
				0x02 : ?
				0x10 : ?

	20020008  2	W	-
	2002000a  2	W	TX Frame Size
	2002000c  2	R/W	RX FIFO Pointer (0x0000 - 0x0fff)
	2002000e  2	W	TX FIFO Pointer (0x0000 - 0x1fff)
	*/
}

/*********************************************************************************/

/**
 * helper function used to read a byte from a chunk of 32 bit memory
 */
static data8_t
nthbyte( const data32_t *pSource, int offs )
{
	pSource += offs/4;
	return (pSource[0]<<((offs&3)*8))>>24;
}

static READ32_HANDLER( namcos22_system_controller_r )
{
	return namcos22_system_controller[offset];
}

static WRITE32_HANDLER( namcos22_system_controller_w )
{
	COMBINE_DATA( &namcos22_system_controller[offset] );
	/*
		40000000	IRQ (unknown)
		40000001
		40000002	SCI IRQ level
		40000003	IRQ (unknown)
		40000004	VSYNC IRQ level

		40000005	IRQ (unknown) acknowledge
		40000006
		40000007	SCI IRQ acknowledge
		40000008	IRQ (unknown) acknowledge
		40000009	VSYNC IRQ acknowledge

		4000000a
		4000000b	?
		4000000c	?
		4000000d
		4000000e	?
		4000000f
		40000010	?
		40000011	?
		40000012	?
		40000013	?
		40000014	?
		40000015	? (cyc1)
		40000016	Watchdog timer reset
		40000017
		40000018	0 or 1 -> DSP control (reset?)
		40000019	?
		4000001a	0 or 1 or 0xff -> DSP control
		4000001b	?
		4000001c
		4000001d
		4000001e
		4000001f
	 */
}

/*********************************************************************************/

static READ32_HANDLER( namcos22_keycus_r )
{
	switch( namcos22_gametype )
	{
	case NAMCOS22_CYBER_COMMANDO:
		return 0x185;

	case NAMCOS22_RIDGE_RACER:
		return 0x0172<<16;

	case NAMCOS22_CYBER_CYCLES:
		return 0x387;

	case NAMCOS22_ALPINE_RACER:
		return 0x0187;

	case NAMCOS22_VICTORY_LAP:
		return 0x0188<<16;

	case NAMCOS22_ACE_DRIVER:
		return 0x0173;

	default:
		/* unknown/unused */
		return 0;
	}
}

/*********************************************************************************/

/**
 * Some port values are read serially one bit at a time via word reads at
 * 0x50000008 and 0x5000000a
 *
 * Writes to 0x50000008 and 0x5000000a reset the state of the input buffer.
 *
 * Note that only the values read at 0x50000008 seem to be used in-game.
 *
 * Some of these values are redundant with respects to the work-RAM supplied input port
 * values supplied by the IO CPU.  For example, the position of the stick shift is digital,
 * and may be read through this mechanism or through shared IO RAM at 0x60004030.
 *
 * Other values seem to be digital versions of analog ports, for example "the gas pedal is
 * pressed" as a boolean flag.  IO RAM supplies it as an analog value.
 */
static data32_t mSys22PortBits;

static READ32_HANDLER( namcos22_portbit_r )
{
	data32_t data = mSys22PortBits;
	mSys22PortBits>>=1;
	return data&0x10001;
}
static WRITE32_HANDLER( namcos22_portbit_w )
{
	unsigned dat50000008 = readinputport(4);
	unsigned dat5000000a = 0xffff;
	mSys22PortBits = (dat50000008<<16)|dat5000000a;
}

static READ32_HANDLER( namcos22_dipswitch_r )
{
	return readinputport(0)<<16;
}

static READ32_HANDLER( namcos22_mcuram_r )
{
	return namcos22_shareram[offset];
}

/*********************************************************************************/

/* I don't know what "SPOT RAM" is.  It isn't directly memory mapped,
 * but rather some ports are used to populate and poll it.
 *
 * See Time Crisis "SPOT RAM" self test for sample use.
 */
#define SPOTRAM_SIZE (320*4)

static struct
{
	data16_t portR; /* next address for read */
	data16_t portW; /* next address for write */
	data16_t RAM[SPOTRAM_SIZE];
} mSpotRAM;

static READ32_HANDLER( spotram_r )
{
	/* 0x860004: read */
	if( offset==1 )
	{
		if( mSpotRAM.portR>=SPOTRAM_SIZE ) mSpotRAM.portR = 0;
		return mSpotRAM.RAM[mSpotRAM.portR++]<<16;
	}
	return 0;
}

static WRITE32_HANDLER( spotram_w )
{
	/* 0x860002: write */
	if( offset==0 )
	{
		if( mem_mask&0xffff0000 )
		{
			if( mSpotRAM.portW>=SPOTRAM_SIZE ) mSpotRAM.portW = 0;
			mSpotRAM.RAM[mSpotRAM.portW++] = data;
		}
		else
		{
			mSpotRAM.portR = 0;
			mSpotRAM.portW = 0;
		}
	}
}

/*********************************************************************************/

/* Namco Super System 22 */

static MEMORY_READ32_START( namcos22s_readmem )
	{ 0x000000, 0x3fffff, MRA32_ROM },
	{ 0x400000, 0x40001f, namcos22_keycus_r },
	{ 0x410000, 0x413fff, MRA32_RAM }, /* unused by Prop Cycle? */
	{ 0x420000, 0x42000f, MRA32_RAM },
	{ 0x430000, 0x430003, MRA32_RAM },
	{ 0x440000, 0x440003, namcos22_dipswitch_r },
	{ 0x450008, 0x45000b, namcos22_portbit_r },

	{ 0x460000, 0x463fff, MRA32_RAM }, /* unknown */
		/* possible nvmem?
		 */

	{ 0x700000, 0x70001f, namcos22_system_controller_r },

	{ 0x800000, 0x800003, MRA32_RAM }, /* 0x08380000 (always?) */
	{ 0x810000, 0x81000f, MRA32_RAM }, /* 00000000 00000000 44440000 00000000 */

	{ 0x810200, 0x8103ff, MRA32_RAM }, /* CZ RAM: depth cueing; fog density, near to far */
	{ 0x810400, 0x810403, MRA32_RAM }, /* ? air combat22 */
	{ 0x820000, 0x8202ff, MRA32_RAM },

	{ 0x824000, 0x8243ff, namcos22_gamma_r }, /* GAMMA */
	{ 0x828000, 0x83ffff, namcos22_paletteram_r }, /* PALETTE */
	{ 0x860000, 0x860007, spotram_r },

	{ 0x880000, 0x89dfff, namcos22_cgram_r },
	{ 0x89e000, 0x89ffff, namcos22_textram_r },
	{ 0x8a0000, 0x8a000f, MRA32_RAM }, /* tilemap attributes */
		/*	+0x0000			BG Position X
		 *	+0x0002			BG Position Y
		 */
	{ 0x900000, 0x90ffff, MRA32_RAM },
	{ 0x940000, 0x94007f, MRA32_RAM },
	{ 0x980000, 0x9affff, MRA32_RAM }, /* spriteram */
	{ 0xa03ffc, 0xa03fff, MRA32_RAM },
	{ 0xa04000, 0xa0bfff, namcos22_mcuram_r },
	{ 0xc00000, 0xc3ffff, namcos22_dspram_r },
	{ 0xe00000, 0xefffff, MRA32_RAM }, /* workram */
MEMORY_END

static MEMORY_WRITE32_START( namcos22s_writemem )
	{ 0x000000, 0x3fffff, MWA32_ROM },
	{ 0x400000, 0x40001f, MWA32_NOP }, /* cuskey? */
	{ 0x410000, 0x413fff, MWA32_RAM },
	{ 0x420000, 0x42000f, MWA32_RAM },
	{ 0x430000, 0x430003, MWA32_RAM }, /* I/O related; bit select? */
	{ 0x450008, 0x45000b, namcos22_portbit_w },
	{ 0x460000, 0x463fff, MWA32_RAM },
	{ 0x700000, 0x70001f, namcos22_system_controller_w, &namcos22_system_controller },
	{ 0x800000, 0x800003, MWA32_RAM }, /* ? */
	/*0838*/
	{ 0x810000, 0x81000f, MWA32_RAM }, /* ? */
	{ 0x810200, 0x8103ff, MWA32_RAM }, /* depth cueing density */
	{ 0x810400, 0x810403, MWA32_RAM }, /* ? air combat22 */
	{ 0x820000, 0x8202ff, MWA32_RAM }, /* ? */
	{ 0x824000, 0x8243ff, namcos22_gamma_w, &namcos22_gamma },
	{ 0x828000, 0x83ffff, namcos22_paletteram_w, &paletteram32 },
	{ 0x860000, 0x860007, spotram_w },
	{ 0x880000, 0x89dfff, namcos22_cgram_w, &namcos22_cgram },
	{ 0x89e000, 0x89ffff, namcos22_textram_w, &namcos22_textram },
	{ 0x8a0000, 0x8a000f, MWA32_RAM }, /* tilemap attributes */
		/* 035c0000 006e0000 01ff0000 0000000 */
	{ 0x900000, 0x90ffff, MWA32_RAM }, /* VICS(?) */
	{ 0x940000, 0x94007f, MWA32_RAM },
	{ 0x980000, 0x9affff, MWA32_RAM, &spriteram32 },
	{ 0xa03ffc, 0xa03fff, MWA32_RAM },
	{ 0xa04000, 0xa0bfff, MWA32_RAM, &namcos22_shareram }, /* MCU */
		/* 0xa0bd2f: 0x02 Prop Cycle: MOTOR ON */
		/* 0xa0bd2f: 0x04 Prop Cycle: LAMP ON */
	{ 0xc00000, 0xc3ffff, namcos22_dspram_w, &namcos22_polygonram },
	{ 0xe00000, 0xefffff, MWA32_RAM }, /* WORK */
	/* note that at least some of this is battery backed ADS RAM */
MEMORY_END

static void
SimulateAlpineRacerMCU( void )
{
	/* ori.b   #$80, $a0bd00.l: signal from main CPU
	 * $a0bd01: bit 7 indicates mcu is busy
	 *
	 * $a0bd02 (byte)
	 * $a0bd04 (byte)
	 *
	 * $a0bd0a: swing
	 * $a0bd0c: edge
	 *
	 * move.w  #$0, $a0bd2e.l
	 *
	 * move.l  #$410000, $e05950.l
	 * move.l  #$414000, $e05954.l
	 *
	 * move.l  #$a04000, $e05950.l
	 * move.l  #$a0c000, $e05954.l
	 */
	if( namcos22_gametype == NAMCOS22_ALPINE_RACER )
	{
		namcos22_shareram[0x0300/4] = 0x7551<<16; /* protection? */
		namcos22_shareram[0x7d00/4] = readinputport(1)<<8;
		namcos22_shareram[(0xa0bd0a-0xa04000)/4] = (data16_t)(readinputport(2)-0x8000); /* swing */
		namcos22_shareram[(0xa0bd0c-0xa04000)/4] = ((data16_t)(readinputport(3)-0x8000))<<16; /* edge */
	}
}

static void
SimulatePropCycleMCU( void )
{
		int dx,dy;
		UINT16 data;
		static data16_t pedal;

		namcos22_shareram[(0xa0bd00-0xa04000)/4] = readinputport(1)<<8;

		data = 0;
		if( readinputport( 2 ) & 0x20 ) data |= 0x0100;
		namcos22_shareram[(0xa0bd04-0xa04000)/4] = data<<16; /* start1*/

		dx = 0; dy = 0;
		if( readinputport( 2 ) & 0x04 ) dx++;
		if( readinputport( 2 ) & 0x08 ) dx--;
		if( readinputport( 2 ) & 0x01 ) dy--;
		if( readinputport( 2 ) & 0x02 ) dy++;

		if( readinputport( 2 ) & 0x10 ) pedal+=0x10;

		namcos22_shareram[(0xa0bd08-0xa04000)/4] = (UINT16)(dx*0x7fff);
		namcos22_shareram[(0xa0bd0c-0xa04000)/4] = (dy*0x7fff)<<16;
		namcos22_shareram[(0xa0bd1c-0xa04000)/4] = pedal<<16;
}

static void
SimulateCyberCyclesMCU( void )
{
	data32_t data = 0;
return;
	if( keyboard_pressed(KEYCODE_SPACE) ) data |= 0x01000; /* "view switch"*/
	namcos22_shareram[(0xa0bd00 - 0xa04000)/4] = data;

	data = 0; /* ? */
	namcos22_shareram[(0xa0bd2e - 0xa04000)/4] = data;
}

static void
SimulateTimeCrisisMCU( void )
{
	/* x------- -------- -------- -------- (w)
	 * -------- x------- -------- -------- (r) status
	 */
/*	namcos22_shareram[(0xa0b000 - 0xa04000)/4] ^= 0x00800000;*/
}

static INTERRUPT_GEN( namcos22s_interrupt )
{
	switch( namcos22_gametype )
	{
	case NAMCOS22_ALPINE_RACER:
		SimulateAlpineRacerMCU();
		break;

	case NAMCOS22_PROP_CYCLE:
		SimulatePropCycleMCU();
		break;

	case NAMCOS22_CYBER_CYCLES:
		SimulateCyberCyclesMCU();
		break;

	case NAMCOS22_TIME_CRISIS:
		SimulateTimeCrisisMCU();
		break;

	default:
		break;
	}

	switch( cpu_getiloops() )
	{
	case 0:
		cpu_set_irq_line(0, 4, HOLD_LINE); /* vblank */
		break;
	case 1:
		if( namcos22_gametype == NAMCOS22_CYBER_CYCLES )
		{
			cpu_set_irq_line(0, 2, HOLD_LINE); /* vblank */
		}
		else if( namcos22_gametype == NAMCOS22_AIR_COMBAT22 )
		{
			cpu_set_irq_line(0, 6, HOLD_LINE);
		}
		break;
	}
}

static MACHINE_DRIVER_START( namcos22s )
	/* basic machine hardware */
	MDRV_CPU_ADD(M68EC020,25000000) /* 25 MHz? */
	MDRV_CPU_MEMORY(namcos22s_readmem,namcos22s_writemem)
	MDRV_CPU_VBLANK_INT(namcos22s_interrupt,2)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(NAMCOS22_NUM_COLS*16,NAMCOS22_NUM_ROWS*16)
	MDRV_VISIBLE_AREA(0,NAMCOS22_NUM_COLS*16-1,0,NAMCOS22_NUM_ROWS*16-1)
	MDRV_PALETTE_LENGTH(NAMCOS22_PALETTE_SIZE)
	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_START(namcos22s)
	MDRV_VIDEO_UPDATE(namcos22s)
MACHINE_DRIVER_END

/*********************************************************************************/

/* Namco System 22 */

static MEMORY_READ32_START( namcos22_readmem )
	{ 0x00000000, 0x001fffff, MRA32_ROM },
	{ 0x10000000, 0x1001ffff, MRA32_RAM },	/* work RAM */
	{ 0x18000000, 0x1801ffff, MRA32_RAM },	/* work RAM */
	{ 0x20000000, 0x2000000f, namcos22_keycus_r },
	{ 0x20010000, 0x20013fff, MRA32_RAM },  /* C139 SCI: serial controller (TX/RX buffer) - same as System21 */
	{ 0x20020000, 0x2002000f, namcos22_C139_SCI_r },
	{ 0x40000000, 0x4000001f, namcos22_system_controller_r },
	{ 0x48000000, 0x4800003f, MRA32_NOP },	/* unknown; possible for diagnostics */
	{ 0x50000000, 0x50000003, namcos22_dipswitch_r }, /* DSW2,3 */
	{ 0x50000008, 0x5000000b, namcos22_portbit_r },
	{ 0x58000000, 0x58001fff, MRA32_RAM },	/* EPROM */
	{ 0x60004000, 0x6000bfff, MRA32_RAM },	/* shareram */
	{ 0x70000000, 0x7001ffff, namcos22_dspram_r },
	{ 0x90010000, 0x90017fff, MRA32_RAM },	/* depth-cueing */
	{ 0x90020000, 0x90027fff, MRA32_RAM },	/* C305 display controller */
	{ 0x90028000, 0x9003ffff, MRA32_RAM },	/* palette */
	{ 0x90040000, 0x9007ffff, MRA32_RAM },	/* diagnostic ROM? */
	{ 0x90080000, 0x9009dfff, MRA32_RAM },	/* bg tiles */
	{ 0x9009e000, 0x9009ffff, MRA32_RAM },	/* bg tilemap */
	{ 0x900a0000, 0x900a000f, MRA32_RAM },	/* bg control register */
MEMORY_END

static MEMORY_WRITE32_START( namcos22_writemem )
	{ 0x00000000, 0x001fffff, MWA32_ROM },
	{ 0x10000000, 0x1001ffff, MWA32_RAM },	/* work RAM */
	{ 0x18000000, 0x1801ffff, MWA32_RAM },	/* work RAM */
	{ 0x20000000, 0x2000000f, MWA32_NOP },	/* cuskey */
	{ 0x20010000, 0x20013fff, MWA32_RAM },	/* C139 SCI: TX/RX buffer (same as System21) */
	{ 0x20020000, 0x2002000f, MWA32_RAM },	/* serial controller (registers) */
	{ 0x40000000, 0x4000001f, namcos22_system_controller_w, &namcos22_system_controller },
	{ 0x48000000, 0x4800003f, MWA32_NOP },	/* unknown; possible for diagnostics */
	{ 0x50000000, 0x50000007, MWA32_NOP },
	{ 0x50000008, 0x5000000b, namcos22_portbit_w },
	{ 0x58000000, 0x58001fff, MWA32_RAM },	/* EPROM */
	{ 0x60000000, 0x60003fff, MWA32_NOP },
	{ 0x60004000, 0x6000bfff, MWA32_RAM, &namcos22_shareram },
	{ 0x70000000, 0x7001ffff, namcos22_dspram_w, &namcos22_polygonram },
	{ 0x90000000, 0x90000003, MWA32_NOP },	/* LED on PCB(?) */
	{ 0x90010000, 0x90017fff, MWA32_RAM },	/* depth-cueing */
	{ 0x90020000, 0x90027fff, MWA32_RAM },	/* C305 display controller */
	{ 0x90028000, 0x9003ffff, namcos22_paletteram_w, &paletteram32 },
	{ 0x90040000, 0x9007ffff, MWA32_RAM },	/* victory lap */
	{ 0x90080000, 0x9009dfff, namcos22_cgram_w, &namcos22_cgram },
	{ 0x9009e000, 0x9009ffff, namcos22_textram_w, &namcos22_textram },
	{ 0x900a0000, 0x900a000f, MWA32_RAM },	/* bg control register */
MEMORY_END

static INTERRUPT_GEN( namcos22_interrupt )
{
	int i;
	for( i=0; i<5; i++ )
	{
		int irqlevel = nthbyte(namcos22_system_controller,i);
		if( (irqlevel&0xf8)==0x30 )
		{
			if( i==4 || keyboard_pressed(KEYCODE_I) )
			{
				cpu_set_irq_line(0,irqlevel&7,/*ASSERT_LINE*/HOLD_LINE);
			}
		}
	}

	switch( namcos22_gametype )
	{
	case NAMCOS22_VICTORY_LAP:
		namcos22_shareram[0x00/4] = 0x10<<16; /* SUB CPU ready */
		namcos22_shareram[0x30/4] = (readinputport(4)<<16)|readinputport(3);
		namcos22_shareram[0x34/4] = (readinputport(1)<<16)|readinputport(2);
		break;

	case NAMCOS22_ACE_DRIVER:
		namcos22_shareram[0x00/4] = 0x10<<16; /* SUB CPU ready */
		namcos22_shareram[0x30/4] = (readinputport(4)<<16)|readinputport(3);
		namcos22_shareram[0x34/4] = (readinputport(1)<<16)|readinputport(2);
		break;

	case NAMCOS22_CYBER_COMMANDO:
		namcos22_shareram[0x30/4] = readinputport(0)<<16;
		/**
		 * ---- -x-- ---- ---- test switch
		 */
		break;

	default:
		break;
	}

}

static MACHINE_DRIVER_START( namcos22 )
	/* basic machine hardware */
	MDRV_CPU_ADD(M68020,25000000) /* 25 MHz? */
	MDRV_CPU_MEMORY(namcos22_readmem,namcos22_writemem)
	MDRV_CPU_VBLANK_INT(namcos22_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(NAMCOS22_NUM_COLS*16,NAMCOS22_NUM_ROWS*16)
	MDRV_VISIBLE_AREA(0,NAMCOS22_NUM_COLS*16-1,0,NAMCOS22_NUM_ROWS*16-1)
	MDRV_PALETTE_LENGTH(NAMCOS22_PALETTE_SIZE)
	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_START(namcos22s)
	MDRV_VIDEO_UPDATE(namcos22)
MACHINE_DRIVER_END

/*********************************************************************************/

ROM_START( airco22b )
	ROM_REGION( 0x400000, REGION_CPU1, 0 ) /* main program */
	ROM_LOAD32_BYTE( "acs1verb.1", 0x00003, 0x100000, CRC(062c4f61) SHA1(98e1c75dd0f493eb6ebb64b46543217c1d40116e) )
	ROM_LOAD32_BYTE( "acs1verb.2", 0x00002, 0x100000, CRC(8ae69711) SHA1(4c5323fa8f0419275e330fec66d1fb2b89bb3795) )
	ROM_LOAD32_BYTE( "acs1verb.3", 0x00001, 0x100000, CRC(71738e67) SHA1(eb8c66dedbeff911b6166ebbda466fb9656ef0fb) )
	ROM_LOAD32_BYTE( "acs1verb.4", 0x00000, 0x100000, CRC(3b193add) SHA1(5e3bca13905bfa3a2947f4f16ca01878b0a14a3a) )

	ROM_REGION( 0x200000*2, REGION_GFX1, ROMREGION_DISPOSE ) /* 32x32x8bpp sprite tiles */
	ROM_LOAD( "acs1scg0.12l", 0x200000*0, 0x200000,CRC(e5235404) SHA1(3133b71d1bde3a9815cd02e97382b8078b62b0bb) )
	ROM_LOAD( "acs1scg1.10l", 0x200000*1, 0x200000,CRC(828e91e7) SHA1(8383b029cd29fbad107fd49e866defb50c11c99a) )

	ROM_REGION( 0x200000*8, REGION_GFX2, 0 ) /* 16x16x8bpp texture tiles */
	ROM_LOAD( "acs1cg0.8d",  0x200000*0x0, 0x200000,CRC(1f31343e) SHA1(25ba730cec74e0ed0b404f5c4430b7c3368c9b52) )
	ROM_LOAD( "acs1cg1.10d", 0x200000*0x1, 0x200000,CRC(ccd5481d) SHA1(050e6fc7d4e0591f8ffc9552d140b6bd4533c06d) )
	ROM_LOAD( "acs1cg2.12d", 0x200000*0x2, 0x200000,CRC(14e5d0d2) SHA1(3147ad11098030e9cfd93fbc0a1b3aafa8b8aba6) )
	ROM_LOAD( "acs1cg3.13d", 0x200000*0x3, 0x200000,CRC(1a7bcc16) SHA1(bbc4ca5b208bea8394d1679e4e2d17d22331e2c8) )
	ROM_LOAD( "acs1cg4.14d", 0x200000*0x4, 0x200000,CRC(1920b7fb) SHA1(56318f2a96c55998bb9a8d791d56be3dfb39867e) )
	ROM_LOAD( "acs1cg5.16d", 0x200000*0x5, 0x200000,CRC(3dd109b7) SHA1(a7f914b9b80f1bca1afb6144698578a29ca74676) )
	ROM_LOAD( "acs1cg6.18d", 0x200000*0x6, 0x200000,CRC(ec71c8a3) SHA1(86892a91883d483ca0d422b78fa36042e02f3ad3) )
	ROM_LOAD( "acs1cg7.19d", 0x200000*0x7, 0x200000,CRC(82271757) SHA1(023c935e78b14da310e4c29da8785b82aa3241ac) )

	ROM_REGION( 0x280000, REGION_GFX3, 0 ) /* texture tilemap */
	ROM_LOAD( "acs1ccrl.3d",	 0x000000, 0x200000,CRC(07088ba1) SHA1(a962c0821d5af28ed508cfdbd613675454e306e3) )
	ROM_LOAD( "acs1ccrh.1d",	 0x200000, 0x080000,CRC(62936af6) SHA1(ca80b68415aa2cd2ce4e90404f10640d0ae38be9) )

	ROM_REGION( 0x80000*12, REGION_GFX4, 0 ) /* 3d model data */
	ROM_LOAD( "acs1ptl0.18k", 0x80000*0x0, 0x80000,CRC(bd5896c7) SHA1(58ec7d0f1e0bfdbf4908e1d920bbd7f094993777) )
	ROM_LOAD( "acs1ptl1.16k", 0x80000*0x1, 0x80000,CRC(e583b975) SHA1(beb0cc2b44bc69af057c2bb744cd7e1b95de577a) )
	ROM_LOAD( "acs1ptl2.15k", 0x80000*0x2, 0x80000,CRC(802d737a) SHA1(3d99a369db70d13fb87c2ff26c82b4b39afe94d9) )
	ROM_LOAD( "acs1ptl3.14k", 0x80000*0x3, 0x80000,CRC(fe556ecb) SHA1(9d9dbbb4f1d3688fb763001834640d79d9987d47) )

	ROM_LOAD( "acs1ptm0.18j", 0x80000*0x4, 0x80000,CRC(949b6c58) SHA1(6ea016551b10f5d5764921dcc5a4b81d2b93d701) )
	ROM_LOAD( "acs1ptm1.16j", 0x80000*0x5, 0x80000,CRC(8b2b99d9) SHA1(89c3545c4035509307728a9577018c1100ce3a54) )
	ROM_LOAD( "acs1ptm2.15j", 0x80000*0x6, 0x80000,CRC(f1515080) SHA1(27a87217a140477a6840a610c95ae57abc0d01a6) )
	ROM_LOAD( "acs1ptm3.14j", 0x80000*0x7, 0x80000,CRC(e364f4aa) SHA1(3af6a864765871664cccad82c4795f677be68d51) )

	ROM_LOAD( "acs1ptu0.18f", 0x80000*0x8, 0x80000,CRC(746b3084) SHA1(73397d1f22300fb3a81a0a068da4d0a8cfdc0a36) )
	ROM_LOAD( "acs1ptu1.16f", 0x80000*0x9, 0x80000,CRC(b44f1d3b) SHA1(f3f1a85c082053653e4da7d7f01f1baef1a013c8) )
	ROM_LOAD( "acs1ptu2.15f", 0x80000*0xa, 0x80000,CRC(fdd2d778) SHA1(0269f971d778e908a1efb5a63b08fb3365d98c2a) )
	ROM_LOAD( "acs1ptu3.14f", 0x80000*0xb, 0x80000,CRC(38b425d4) SHA1(8ff6dd6775d42afdff4c9fb2232e4d72b38e515a) )

	ROM_REGION( 0x080000, REGION_CPU2, 0 ) /* S22-BIOS ver1.30 */
	ROM_LOAD( "acs1data.8k", 0, 0x080000, CRC(33824bc9) SHA1(80ec63883770e5eec1f5f1ddc16a85ef8f22a48b) )
	ROM_REGION( 0x800000, REGION_USER2, 0 ) /* sound samples */
	ROM_LOAD( "acs1wav0.1", 0x000000, 0x400000, CRC(52fb9762) SHA1(125c163e62d701c2e17ba0b572ed27c944ca0412) )
	ROM_LOAD( "acs1wav1.2", 0x400000, 0x400000, CRC(b568dca2) SHA1(503deb277691d801acac1380ded2868a5d5ac501) )
ROM_END

ROM_START( alpinerc )
	ROM_REGION( 0x400000, REGION_CPU1, 0 ) /* main program */
	ROM_LOAD32_BYTE( "ar2ver-c.1", 0x00003, 0x100000, CRC(61323842) SHA1(e3c33248340bee252f230124fa9b7fa935a60565) )
	ROM_LOAD32_BYTE( "ar2ver-c.2", 0x00002, 0x100000, CRC(43795b2d) SHA1(e060f3259661279a36300431c5ca7347bde8b6ec) )
	ROM_LOAD32_BYTE( "ar2ver-c.3", 0x00001, 0x100000, CRC(acb3003b) SHA1(ea0cbf3a1607b06b108df051f38fec1f214f42d2) )
	ROM_LOAD32_BYTE( "ar2ver-c.4", 0x00000, 0x100000, CRC(a57b4e60) SHA1(9dd8508f376711f55f8e7ae195de7d05367bcdee) )

	ROM_REGION( 0x200000*2, REGION_GFX1, ROMREGION_DISPOSE ) /* 32x32x8bpp sprite tiles */
	ROM_LOAD( "ar1scg0.12f", 0x200000*0, 0x200000,CRC(e7be830a) SHA1(60e2162eecd7401a0c26c525de2715cbfb10c1c5) ) /* identical to "ar1scg0.12l" */
	ROM_LOAD( "ar1scg1.10f", 0x200000*1, 0x200000,CRC(8f15a686) SHA1(bce2d4380c6c39aa402566ddb0f62bbe6d7bfa1d) ) /* identical to "ar1scg1.10l" */

	ROM_REGION( 0x200000*8, REGION_GFX2, 0 ) /* 16x16x8bpp texture tiles */
	ROM_LOAD( "ar1cg0.12b",  0x200000*0x0, 0x200000,CRC(93f3a9d9) SHA1(7e94c81ad5ace98a2f0d00d101d464883d38c197) ) /* identical to "ar1cg0.8d" */
	ROM_LOAD( "ar1cg1.10d",  0x200000*0x1, 0x200000,CRC(39828c8b) SHA1(424aa67eb0b898c9cab8a4749893a9c5696ac430) ) /* identical to "ar1cg1.13b" */
	ROM_LOAD( "ar1cg2.12d",  0x200000*0x2, 0x200000,CRC(f7b058d1) SHA1(fffd0f01724a26dd47b1ecceecf4a139d5746f81) ) /* identical to "ar1cg2.14b" */
	ROM_LOAD( "ar1cg3.13d",  0x200000*0x3, 0x200000,CRC(c28a3d2a) SHA1(cdc44fdbc99274e860c834e42b4cfafb478d4d26) ) /* identical to "ar1cg3.16b" */
	ROM_LOAD( "ar1cg4.14d",  0x200000*0x4, 0x200000,CRC(abdb161f) SHA1(260bff9b0e94c1b2ea4b9d7fa170fbca212e85ee) ) /* identical to "ar1cg4.18b" */
	ROM_LOAD( "ar1cg5.16d",  0x200000*0x5, 0x200000,CRC(2381cfea) SHA1(1de4c8b94df233fd74771fa47843290a3d8df0c8) ) /* identical to "ar1cg5.19b" */
	ROM_LOAD( "ar1cg6.18a",  0x200000*0x6, 0x200000,CRC(ca0b6d23) SHA1(df969e0eeec557a95584b06995b0d55f2c6ec70a) ) /* identical to "ar1cg6.18d" */
	ROM_LOAD( "ar1cg7.15a",	 0x200000*0x7, 0x200000,CRC(ffb9f9f9) SHA1(2b8c75b580f77e887df7d50909a3a95cda570e20) ) /* identical to "ar1cg7.19d" */

	ROM_REGION( 0x280000, REGION_GFX3, 0 ) /* texture tilemap */
	ROM_LOAD( "ar1ccrl.3d",	 0x000000, 0x200000,CRC(17387b2c) SHA1(dfd7cadaf97917347c0fa98f395364a543e49612) ) /* identical to "ar1ccrl.7b" */
	ROM_LOAD( "ar1ccrh.1d",	 0x200000, 0x080000,CRC(ee7a4803) SHA1(8383c9a8ef5ed94df13446ca5cefa5f9e518f175) ) /* identical to "pr1ccrh.5b" */

	ROM_REGION( 0x80000*12, REGION_GFX4, 0 ) /* 3d model data */
	ROM_LOAD( "ar1ptrl0.18k", 0x80000*0x0, 0x80000,CRC(82405108) SHA1(0a40882a9bc8621c620bede404c78f6b1333f223) )
	ROM_LOAD( "ar1ptrl1.16k", 0x80000*0x1, 0x80000,CRC(8739b09c) SHA1(cd603c4dc2f9ffc4185f891eb83e4c383c564294) )
	ROM_LOAD( "ar1ptrl2.15k", 0x80000*0x2, 0x80000,CRC(bda693a9) SHA1(fe71dd3c63198737aa2d39527f0004e977e3be20) )
	ROM_LOAD( "ar1ptrl3.14k", 0x80000*0x3, 0x80000,CRC(82797405) SHA1(2f205fee2d33e183c80a906fb38900167c011240) )

	ROM_LOAD( "ar1ptrm0.18j", 0x80000*0x4, 0x80000,CRC(29d92097) SHA1(2c492747b295dd715d3a19c8a2cd919b97d095a0) )
	ROM_LOAD( "ar1ptrm1.16j", 0x80000*0x5, 0x80000,CRC(2232f0a5) SHA1(3fccf6d4a0c4100cc85e3051024d659c4a1c769e) )
	ROM_LOAD( "ar1ptrm2.15j", 0x80000*0x6, 0x80000,CRC(8ee14e6f) SHA1(f6f1cbb748b109b365255378c18e710ba6270c1c) )
	ROM_LOAD( "ar1ptrm3.14j", 0x80000*0x7, 0x80000,CRC(1094a970) SHA1(d41b10f48e1ef312bcaf09f27fabc7252c30e648) )

	ROM_LOAD( "ar1ptru0.18f", 0x80000*0x8, 0x80000,CRC(26d88467) SHA1(d528f989fab4dd5ac1aec9b596a05fbadcc0587a) )
	ROM_LOAD( "ar1ptru1.16f", 0x80000*0x9, 0x80000,CRC(c5e2c208) SHA1(152fde0b95a5df8c781e4a83577cfbbc7672ae0d) )
	ROM_LOAD( "ar1ptru2.15f", 0x80000*0xa, 0x80000,CRC(1321ec59) SHA1(dbd3687a4c6b1aa0b18e336f99dabb9010d36639) )
	ROM_LOAD( "ar1ptru3.14f", 0x80000*0xb, 0x80000,CRC(139d7dc1) SHA1(6d25e6ad552a91a0c5fc03db7e1a801ccf9c9556) )

	ROM_REGION( 0x080000, REGION_CPU2, 0 ) /* S22-BIOS ver1.30 */
	ROM_LOAD( "ar1datab.8k", 0, 0x080000, CRC(c26306f8) SHA1(6d8d993c076d5ced523143a86bd0938b3794478d) )
	ROM_REGION( 0x200000, REGION_USER2, 0 ) /* sound samples */
	ROM_LOAD( "ar1wavea.2l", 0, 0x200000, CRC(dbf64562) SHA1(454fd7d5b860f0e5557d8900393be95d6c992ad1) )
ROM_END

ROM_START( alpinerd )
	ROM_REGION( 0x400000, REGION_CPU1, 0 ) /* main program */
	ROM_LOAD32_BYTE( "ar2ver-d.1", 0x00003, 0x100000, CRC(fa3380b9) SHA1(2a46988745bd2672f8082399a68ae0d0ab3d28f2) )
	ROM_LOAD32_BYTE( "ar2ver-d.2", 0x00002, 0x100000, CRC(76141352) SHA1(0f7230dd9cd6f1b83d499034affc7bc2c4385ab5) )
	ROM_LOAD32_BYTE( "ar2ver-d.3", 0x00001, 0x100000, CRC(9beffe6a) SHA1(d8efd1e3829d32bb06537d7cecb59f8df9b6d663) )
	ROM_LOAD32_BYTE( "ar2ver-d.4", 0x00000, 0x100000, CRC(1f3f1134) SHA1(0afa78444d1463d214f1afd7ec500af76d567489) )

	ROM_REGION( 0x200000*2, REGION_GFX1, ROMREGION_DISPOSE ) /* 32x32x8bpp sprite tiles */
	ROM_LOAD( "ar1scg0.12f", 0x200000*0, 0x200000,CRC(e7be830a) SHA1(60e2162eecd7401a0c26c525de2715cbfb10c1c5) ) /* identical to "ar1scg0.12l" */
	ROM_LOAD( "ar1scg1.10f", 0x200000*1, 0x200000,CRC(8f15a686) SHA1(bce2d4380c6c39aa402566ddb0f62bbe6d7bfa1d) ) /* identical to "ar1scg1.10l" */

	ROM_REGION( 0x200000*8, REGION_GFX2, 0 ) /* 16x16x8bpp texture tiles */
	ROM_LOAD( "ar1cg0.12b",  0x200000*0x0, 0x200000,CRC(93f3a9d9) SHA1(7e94c81ad5ace98a2f0d00d101d464883d38c197) ) /* identical to "ar1cg0.8d" */
	ROM_LOAD( "ar1cg1.10d",  0x200000*0x1, 0x200000,CRC(39828c8b) SHA1(424aa67eb0b898c9cab8a4749893a9c5696ac430) ) /* identical to "ar1cg1.13b" */
	ROM_LOAD( "ar1cg2.12d",  0x200000*0x2, 0x200000,CRC(f7b058d1) SHA1(fffd0f01724a26dd47b1ecceecf4a139d5746f81) ) /* identical to "ar1cg2.14b" */
	ROM_LOAD( "ar1cg3.13d",  0x200000*0x3, 0x200000,CRC(c28a3d2a) SHA1(cdc44fdbc99274e860c834e42b4cfafb478d4d26) ) /* identical to "ar1cg3.16b" */
	ROM_LOAD( "ar1cg4.14d",  0x200000*0x4, 0x200000,CRC(abdb161f) SHA1(260bff9b0e94c1b2ea4b9d7fa170fbca212e85ee) ) /* identical to "ar1cg4.18b" */
	ROM_LOAD( "ar1cg5.16d",  0x200000*0x5, 0x200000,CRC(2381cfea) SHA1(1de4c8b94df233fd74771fa47843290a3d8df0c8) ) /* identical to "ar1cg5.19b" */
	ROM_LOAD( "ar1cg6.18a",  0x200000*0x6, 0x200000,CRC(ca0b6d23) SHA1(df969e0eeec557a95584b06995b0d55f2c6ec70a) ) /* identical to "ar1cg6.18d" */
	ROM_LOAD( "ar1cg7.15a",	 0x200000*0x7, 0x200000,CRC(ffb9f9f9) SHA1(2b8c75b580f77e887df7d50909a3a95cda570e20) ) /* identical to "ar1cg7.19d" */

	ROM_REGION( 0x280000, REGION_GFX3, 0 ) /* texture tilemap */
	ROM_LOAD( "ar1ccrl.3d",	 0x000000, 0x200000,CRC(17387b2c) SHA1(dfd7cadaf97917347c0fa98f395364a543e49612) ) /* identical to "ar1ccrl.7b" */
	ROM_LOAD( "ar1ccrh.1d",	 0x200000, 0x080000,CRC(ee7a4803) SHA1(8383c9a8ef5ed94df13446ca5cefa5f9e518f175) ) /* identical to "pr1ccrh.5b" */

	ROM_REGION( 0x80000*12, REGION_GFX4, 0 ) /* 3d model data */
	ROM_LOAD( "ar1ptrl0.18k", 0x80000*0x0, 0x80000,CRC(82405108) SHA1(0a40882a9bc8621c620bede404c78f6b1333f223) )
	ROM_LOAD( "ar1ptrl1.16k", 0x80000*0x1, 0x80000,CRC(8739b09c) SHA1(cd603c4dc2f9ffc4185f891eb83e4c383c564294) )
	ROM_LOAD( "ar1ptrl2.15k", 0x80000*0x2, 0x80000,CRC(bda693a9) SHA1(fe71dd3c63198737aa2d39527f0004e977e3be20) )
	ROM_LOAD( "ar1ptrl3.14k", 0x80000*0x3, 0x80000,CRC(82797405) SHA1(2f205fee2d33e183c80a906fb38900167c011240) )

	ROM_LOAD( "ar1ptrm0.18j", 0x80000*0x4, 0x80000,CRC(29d92097) SHA1(2c492747b295dd715d3a19c8a2cd919b97d095a0) )
	ROM_LOAD( "ar1ptrm1.16j", 0x80000*0x5, 0x80000,CRC(2232f0a5) SHA1(3fccf6d4a0c4100cc85e3051024d659c4a1c769e) )
	ROM_LOAD( "ar1ptrm2.15j", 0x80000*0x6, 0x80000,CRC(8ee14e6f) SHA1(f6f1cbb748b109b365255378c18e710ba6270c1c) )
	ROM_LOAD( "ar1ptrm3.14j", 0x80000*0x7, 0x80000,CRC(1094a970) SHA1(d41b10f48e1ef312bcaf09f27fabc7252c30e648) )

	ROM_LOAD( "ar1ptru0.18f", 0x80000*0x8, 0x80000,CRC(26d88467) SHA1(d528f989fab4dd5ac1aec9b596a05fbadcc0587a) )
	ROM_LOAD( "ar1ptru1.16f", 0x80000*0x9, 0x80000,CRC(c5e2c208) SHA1(152fde0b95a5df8c781e4a83577cfbbc7672ae0d) )
	ROM_LOAD( "ar1ptru2.15f", 0x80000*0xa, 0x80000,CRC(1321ec59) SHA1(dbd3687a4c6b1aa0b18e336f99dabb9010d36639) )
	ROM_LOAD( "ar1ptru3.14f", 0x80000*0xb, 0x80000,CRC(139d7dc1) SHA1(6d25e6ad552a91a0c5fc03db7e1a801ccf9c9556) )

	ROM_REGION( 0x080000, REGION_CPU2, 0 ) /* S22-BIOS ver1.30 */
	ROM_LOAD( "ar1datab.8k", 0, 0x080000, CRC(c26306f8) SHA1(6d8d993c076d5ced523143a86bd0938b3794478d) )
	ROM_REGION( 0x200000, REGION_USER2, 0 ) /* sound samples */
	ROM_LOAD( "ar1wavea.2l", 0, 0x200000, CRC(dbf64562) SHA1(454fd7d5b860f0e5557d8900393be95d6c992ad1) )
ROM_END

ROM_START( cybrcomm )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) /* main program */
	ROM_LOAD32_BYTE( "cy1prgll.4d", 0x00003, 0x80000, CRC(b3eab156) SHA1(2a5c4e0360c3b9500687a4d70f7110a0c30da2a5) )
	ROM_LOAD32_BYTE( "cy1prglm.2d", 0x00002, 0x80000, CRC(884a5b0e) SHA1(0e27ae366b8a2695fe112b4740c8c9013bb97e26) )
	ROM_LOAD32_BYTE( "cy1prgum.8d", 0x00001, 0x80000, CRC(c9c4a921) SHA1(76a52461165a8bd8d984a34063fbeb4cb73624af) )
	ROM_LOAD32_BYTE( "cy1prguu.6d", 0x00000, 0x80000, CRC(5f22975b) SHA1(a1a5cb66358d64a3c564b912f2eeafa182786b1e) )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )

	ROM_REGION( 0x200000*8, REGION_GFX2, 0 ) /* 16x16x8bpp texture tiles */
	ROM_LOAD( "cyc1cg0.1a", 0x200000*0x0, 0x200000,CRC(e839b9bd) SHA1(fee43d37dcca7f1fb952a6bfb886b7ee30b7d75c) ) /* cyc1cg0.6a */
	ROM_LOAD( "cyc1cg1.2a", 0x200000*0x2, 0x200000,CRC(7d13993f) SHA1(96ac82bcc63afe395bae73f005eb66dad7742d48) ) /* cyc1cg1.7a */
	ROM_LOAD( "cyc1cg2.3a", 0x200000*0x4, 0x200000,CRC(7c464566) SHA1(69817ac3a7c6e43b960e8a904962b58b23417163) ) /* cyc1cg2.8a */
	ROM_LOAD( "cyc1cg3.5a", 0x200000*0x6, 0x200000,CRC(2222e16f) SHA1(562bcd4d43b1543303d8fd66d9f0d9a8e3702492) ) /* cyc1cg3.9a */

	ROM_REGION( 0x280000, REGION_GFX3, 0 ) /* texture tilemap */
	ROM_LOAD( "cyc1ccrl.1c",	 0x000000, 0x100000,CRC(1a0dc5f0) SHA1(bf0093d9cbdcb45a82705e966c48a1f408fa344e) ) /* cyc1ccrl.8c */
	ROM_LOAD( "cyc1ccrh.2c",	 0x200000, 0x080000,CRC(8c4090b8) SHA1(456d548a48833e840c5d39d47b2dcca03f8d6321) ) /* cyc1ccrh.7c */

	ROM_REGION( 0x80000*9, REGION_GFX4, 0 ) /* 3d model data */
	ROM_LOAD( "cyc1ptl0.5b", 0x80000*0x0, 0x80000,CRC(d91de03d) SHA1(05819d285f6111867c41337bda9c4b9ad5394b6b) )
	ROM_LOAD( "cyc1ptl1.4b", 0x80000*0x1, 0x80000,CRC(e5b98021) SHA1(7416cbf74da969f822e0363ced7a25b967277e28) )
	ROM_LOAD( "cyc1ptl2.3b", 0x80000*0x2, 0x80000,CRC(7ba786c6) SHA1(1a5319dec495453bab9d70ae773a807f0036b355) )
	ROM_LOAD( "cyc1ptm0.5c", 0x80000*0x3, 0x80000,CRC(d454b5c6) SHA1(95ae6f0455e9fd7dff066e74cd4343c94d1bc212) )
	ROM_LOAD( "cyc1ptm1.4c", 0x80000*0x4, 0x80000,CRC(74fdf8cc) SHA1(f2627f400e247b6d4c4157eaf0ec69d57212e566) )
	ROM_LOAD( "cyc1ptm2.3c", 0x80000*0x5, 0x80000,CRC(b9c99a45) SHA1(c86cf594b416776eaf9a32c3cb9d34acc79777e9) )
	ROM_LOAD( "cyc1ptu0.5d", 0x80000*0x6, 0x80000,CRC(4d40897f) SHA1(ffe2a0ab66443553c83512f9a1be94b2e385cf2f) )
	ROM_LOAD( "cyc1ptu1.4d", 0x80000*0x7, 0x80000,CRC(3bdaeeeb) SHA1(826f97e2165af8569cfec03874b16030a9486559) )
	ROM_LOAD( "cyc1ptu2.3d", 0x80000*0x8, 0x80000,CRC(a0e73674) SHA1(1e22142a564e664031c12b250664fc82e3b3d43b) )

	ROM_REGION( 0x2100, REGION_USER1, 0 )
	ROM_LOAD( "cy1eeprm.9e", 0x0000, 0x2000, CRC(4e1d294b) SHA1(954ce04dcdba65214f5d0690ca59264f9090a1d6) ) /* EPROM */

	ROM_REGION( 0x080000, REGION_CPU2, 0 ) /* MCU */
	ROM_LOAD( "cy1data.6r", 0, 0x020000, CRC(10d0005b) SHA1(10508eeaf74d24a611b44cd3bb12417ceb78904f) )
	ROM_REGION( 0x400000, REGION_USER2, 0 ) /* sound samples */
	ROM_LOAD( "cy1wav0.10r", 0x000000, 0x100000, CRC(c6f366a2) SHA1(795dbee09df159d3501c748fb3de16cca81742d6) )
	ROM_LOAD( "cy1wav1.10p", 0x100000, 0x100000, CRC(f30b5e37) SHA1(9f5a94d82741ef9688c6e415ebb9009c906737c9) )
	ROM_LOAD( "cy1wav2.10n", 0x200000, 0x100000, CRC(b98c1ca6) SHA1(4b66aa05f82be5ef3315acc30031872698ff4391) )
	ROM_LOAD( "cy1wav3.10l", 0x300000, 0x100000, CRC(43dbac19) SHA1(83fd4ae4e7ec264fc217ed18caf59bf438af0c3d) )
ROM_END

ROM_START( cybrcycc )
	ROM_REGION( 0x400000, REGION_CPU1, 0 ) /* main program */
	ROM_LOAD32_BYTE( "cb2ver-c.1", 0x00003, 0x100000, CRC(a8e07a14) SHA1(9bef7068c9bf792960df922ea79e4565d7680433) )
	ROM_LOAD32_BYTE( "cb2ver-c.2", 0x00002, 0x100000, CRC(054c504f) SHA1(9bde803ff09be0402f9b0388e55407362a2508e3) )
	ROM_LOAD32_BYTE( "cb2ver-c.3", 0x00001, 0x100000, CRC(47e6306c) SHA1(39d6fc2c3cb9b4c9d3569cedb79b916a90537115) )
	ROM_LOAD32_BYTE( "cb2ver-c.4", 0x00000, 0x100000, CRC(398426e4) SHA1(f20cd4892420e7b978baa51c9129b362422a3895) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE ) /* 32x32x8bpp sprite tiles */
	ROM_LOAD( "cb1scg0.12f", 0x200000*0, 0x200000,CRC(7aaca90d) SHA1(9808819db5d86d555a03bb20a2fbedf060d04f0e) ) /* identical to "cb1scg0.12l" */

	ROM_REGION( 0x200000*7, REGION_GFX2, 0 ) /* 16x16x8bpp texture tiles */
	ROM_LOAD( "cb1cg0.12b",  0x200000*0x0, 0x200000,CRC(762a47a0) SHA1(8a49c700dca7afec5d8d6a38fedcd3ad4b0e6713) ) /* identical to "cb1cg0.8d" */
	ROM_LOAD( "cb1cg1.10d",  0x200000*0x1, 0x200000,CRC(df92c3e6) SHA1(302d7ee7e073a45e7baa948543bd30251f903a6d) ) /* identical to "cb1cg1.13b" */
	ROM_LOAD( "cb1cg2.12d",  0x200000*0x2, 0x200000,CRC(07bc508e) SHA1(7675694d10b50e57bb10b350559bd321df75d1ea) ) /* identical to "cb1cg2.14b" */
	ROM_LOAD( "cb1cg3.13d",  0x200000*0x3, 0x200000,CRC(50c86dea) SHA1(7837a1d2bd3ade470f7fbc732513dd598badd219) ) /* identical to "cb1cg3.16b" */
	ROM_LOAD( "cb1cg4.14d",  0x200000*0x4, 0x200000,CRC(e93b8894) SHA1(4d28b557b7ed2667e6af9f970f3e99cda785b940) ) /* identical to "cb1cg4.18b" */
	ROM_LOAD( "cb1cg5.16d",  0x200000*0x5, 0x200000,CRC(9ee610a1) SHA1(ebc7892b6a66461ca6b6b912a264da1594340b2d) ) /* identical to "cb1cg5.19b" */
	ROM_LOAD( "cb1cg6.18a",  0x200000*0x6, 0x200000,CRC(ddc3b5cc) SHA1(34edffee9eb6fbf4a00fce0da34d9354b1a1155f) ) /* identical to "cb1cg6.18d" */

	ROM_REGION( 0x280000, REGION_GFX3, 0 ) /* texture tilemap */
	ROM_LOAD( "cb1ccrl.3d",	 0x000000, 0x200000,CRC(2f171c48) SHA1(52b76213e37379b4a5cea7de40cf5396dc2998d8) ) /* identical to "cb1ccrl.7b" */
	ROM_LOAD( "cb1ccrh.1d",	 0x200000, 0x080000,CRC(86124b93) SHA1(f2cfd726313cbeff162d402a15de2360377630e7) ) /* identical to "cb1ccrh.5b" */

	ROM_REGION( 0x80000*12, REGION_GFX4, 0 ) /* 3d model data */
	ROM_LOAD( "cb1ptrl0.18k", 0x80000*0x0, 0x80000,CRC(f1393a03) SHA1(c9e808601eef5839e6bff630e5f83380e073c5c0) )
	ROM_LOAD( "cb1ptrl1.16k", 0x80000*0x1, 0x80000,CRC(2ad51de7) SHA1(efd102b960ca10cda70da84661acf61e4bbb9f00) )
	ROM_LOAD( "cb1ptrl2.15k", 0x80000*0x2, 0x80000,CRC(78f77c0d) SHA1(5183a8909c2ac0a3d80e707393bcbb4441d79a3c) )
	ROM_LOAD( "cb1ptrl3.14k", 0x80000*0x3, 0x80000,CRC(804bfb4a) SHA1(74b3fc3931265398e23605d3da7ca84a002da632) )
	ROM_LOAD( "cb1ptrm0.18j", 0x80000*0x4, 0x80000,CRC(f4eece49) SHA1(3f34d1ae5986f0d340563ab0fb637bfdacb8712c) )
	ROM_LOAD( "cb1ptrm1.16j", 0x80000*0x5, 0x80000,CRC(5f3cbd7d) SHA1(d00d0a96b71d6a3b98907c4ba7c702e549dd0adb) )
	ROM_LOAD( "cb1ptrm2.15j", 0x80000*0x6, 0x80000,CRC(02c7e4af) SHA1(6a541a28163b1026a824f6f8aed05d0eb0c8ae93) )
	ROM_LOAD( "cb1ptrm3.14j", 0x80000*0x7, 0x80000,CRC(ace3123b) SHA1(2b590ed967572d77b3cc6b37e341a5bdc55c762f) )
	ROM_LOAD( "cb1ptru0.18f", 0x80000*0x8, 0x80000,CRC(58d35341) SHA1(a5fe00bdcf39521f0465743664ff0dd78be5d6e8) )
	ROM_LOAD( "cb1ptru1.16f", 0x80000*0x9, 0x80000,CRC(f4d005b0) SHA1(0862ed1dd0818bfb765d97f1f9d996c321b0ec83) )
	ROM_LOAD( "cb1ptru2.15f", 0x80000*0xa, 0x80000,CRC(68ffcd50) SHA1(5ca5f71b6b079fde14d76c869d211a815bffae68) )
	ROM_LOAD( "cb1ptru3.14f", 0x80000*0xb, 0x80000,CRC(d89c1c2b) SHA1(9c25df696b2d120ce33d7774381460297740007a) )

	ROM_REGION( 0x080000, REGION_CPU2, 0 ) /* S22-BIOS ver1.30 */
	ROM_LOAD( "cb1datab.8k", 0, 0x080000, CRC(e2404221) SHA1(b88810dd45aee8a5475c30806cdfded25fa14e0e) )

	ROM_REGION( 0x600000, REGION_USER2, 0 ) /* sound samples */
	ROM_LOAD( "cb1wavea.2l", 0x000000, 0x400000, CRC(b79a624d) SHA1(c0ee358a183ba6d0835731dbdd191b64718fde6e) )
	ROM_LOAD( "cb1waveb.1l", 0x400000, 0x200000, CRC(33bf08f6) SHA1(bf9d68b26a8158ea1abfe8428b7454cac25242c5) )
ROM_END

ROM_START( propcycl )
	ROM_REGION( 0x400000, REGION_CPU1, 0 ) /* main program */
	ROM_LOAD32_BYTE( "pr2ver-a.1", 0x00003, 0x100000, CRC(3f58594c) SHA1(5fdd8c61b47b51088a201799ce0c2f08c32ef852) )
	ROM_LOAD32_BYTE( "pr2ver-a.2", 0x00002, 0x100000, CRC(c0da354a) SHA1(f27a71a62385b842404fcd8ed6513158e3639b8f) )
	ROM_LOAD32_BYTE( "pr2ver-a.3", 0x00001, 0x100000, CRC(74bf4b74) SHA1(02713aa07238cc9e30163ae24d12c034aa972ff3) )
	ROM_LOAD32_BYTE( "pr2ver-a.4", 0x00000, 0x100000, CRC(cf4d5638) SHA1(2ddd00d6ec3b85c234820507650d201e176c94a2) )

	ROM_REGION( 0x200000*2, REGION_GFX1, ROMREGION_DISPOSE ) /* 32x32x8bpp sprite tiles */
	ROM_LOAD( "pr1scg0.12f", 0x200000*0, 0x200000,CRC(2d09a869) SHA1(ce8beabaac255e1de29d944c9866022bad713519) ) /* identical to "pr1scg0.12l" */
	ROM_LOAD( "pr1scg1.10f", 0x200000*1, 0x200000,CRC(7433c5bd) SHA1(a8fd4e73de66e3d443c0cb5b5beef8f467014815) ) /* identical to "pr1scg1.10l" */

	ROM_REGION( 0x200000*8, REGION_GFX2, 0 ) /* 16x16x8bpp texture tiles */
	ROM_LOAD( "pr1cg0.12b",  0x200000*0x0, 0x200000,CRC(0a041238) SHA1(da5688970432f7fe39337ee9fb46ca25a53fdb11) ) /* identical to "pr1cg0.8d" */
	ROM_LOAD( "pr1cg1.10d",  0x200000*0x1, 0x200000,CRC(7d09e6a7) SHA1(892317ee0bd796fa5c70d64912ef2e696792a2d4) ) /* identical to "pr1cg1.13b" */
	ROM_LOAD( "pr1cg2.12d",  0x200000*0x2, 0x200000,CRC(659f006e) SHA1(23362a922cb1100950181fac4858b953d8fc0794) ) /* identical to "pr1cg2.14b" */
	ROM_LOAD( "pr1cg3.13d",  0x200000*0x3, 0x200000,CRC(d30bffa3) SHA1(2f05227d91d257db9fa8cae114974de602d98729) ) /* identical to "pr1cg3.16b" */
	ROM_LOAD( "pr1cg4.14d",  0x200000*0x4, 0x200000,CRC(f4636cc9) SHA1(4e01a476e418e5790878572e83a8a11536ce30ae) ) /* identical to "pr1cg4.18b" */
	ROM_LOAD( "pr1cg5.16d",  0x200000*0x5, 0x200000,CRC(97d333de) SHA1(e8f8383f49aae834dd8b57231b25899703cef966) ) /* identical to "pr1cg5.19b" */
	ROM_LOAD( "pr1cg6.18a",  0x200000*0x6, 0x200000,CRC(3e081c03) SHA1(6ccb162952f6076359b2785b5d800b39a9a3c5ce) ) /* identical to "pr1cg6.18d" */
	ROM_LOAD( "pr1cg7.15a",	 0x200000*0x7, 0x200000,CRC(ec9fc5c8) SHA1(16de614b26f06bbddae3ab56cebba45efd6fe81b) ) /* identical to "pr1cg7.19d" */

	ROM_REGION( 0x280000, REGION_GFX3, 0 ) /* texture tilemap */
	ROM_LOAD( "pr1ccrl.3d",	 0x000000, 0x200000,CRC(e01321fd) SHA1(5938c6eff8e1b3642728c3be733f567a97cb5aad) ) /* identical to "pr1ccrl.7b" */
	ROM_LOAD( "pr1ccrh.1d",	 0x200000, 0x080000,CRC(1d68bc31) SHA1(d534d0daebe7018e83b57cc7919c294ab89bddc8) ) /* identical to "pr1ccrh.5b" */
	/* These two ROMs define a huge texture tilemap using the tiles from REGION_GFX2.
	 * The tilemap has 0x100 columns.
	 *
	 * pr1ccrl contains little endian 16 bit words.  Each word references a 16x16 tile.
	 *
	 * pr1ccrh.1d contains packed nibbles.  Each nibble encodes three tile attributes:
	 *	0x8 = swapxy
	 *	0x4 = flipx
	 *	0x2 = flipy
	 */

	ROM_REGION( 0x80000*9, REGION_GFX4, 0 ) /* 3d model data */
	ROM_LOAD( "pr1ptrl0.18k", 0x80000*0, 0x80000,CRC(fddb27a2) SHA1(6e837b45e3f9ed7ca3d1a457d0f0124de5618d1f) )
	ROM_LOAD( "pr1ptrl1.16k", 0x80000*1, 0x80000,CRC(6964dd06) SHA1(f38a550165504693d20892a7dcfaf01db19b04ef) )
	ROM_LOAD( "pr1ptrl2.15k", 0x80000*2, 0x80000,CRC(4d7ed1d4) SHA1(8f72864a06ff8962e640cb36d062bddf5d110308) )
	ROM_LOAD( "pr1ptrm0.18j", 0x80000*3, 0x80000,CRC(b6f204b7) SHA1(3b34f240b399b6406faaf338ae0ab536247e64a6) )
	ROM_LOAD( "pr1ptrm1.16j", 0x80000*4, 0x80000,CRC(949588b7) SHA1(fdaf50ff2496200b9c981efc18b035f3c0a96ace) )
	ROM_LOAD( "pr1ptrm2.15j", 0x80000*5, 0x80000,CRC(dc1cef0a) SHA1(8cbc02cf73fac3cc110b676d77602ae628385eae) )
	ROM_LOAD( "pr1ptru0.18f", 0x80000*6, 0x80000,CRC(5d66a7c4) SHA1(c9ed1c18724192d45c1f6b40096f15d02baf2401) )
	ROM_LOAD( "pr1ptru1.16f", 0x80000*7, 0x80000,CRC(e9a3f72b) SHA1(f967e1adf8eee4fffdf4288d36a93c5bb4f9a126) )
	ROM_LOAD( "pr1ptru2.15f", 0x80000*8, 0x80000,CRC(c346a842) SHA1(299bc0a30d0e74d8adfa3dc605aebf6439f5bc18) )

	ROM_REGION( 0x080000, REGION_CPU2, 0 ) /* SS22-BIOS ver1.41 */
	ROM_LOAD( "pr1data.8k", 0, 0x080000, CRC(2e5767a4) SHA1(390bf05c90044d841fe2dd4a427177fa1570b9a6) )
	ROM_REGION( 0x800000, REGION_USER2, 0 ) /* sound samples */
	ROM_LOAD( "pr1wavea.2l", 0x000000, 0x400000, CRC(320f3913) SHA1(3887b7334ca7762794c14198dd24bab47fcd9505) )
	ROM_LOAD( "pr1waveb.1l", 0x400000, 0x400000, CRC(d91acb26) SHA1(c2161e2d70e08aed15cbc875ffee685190611daf) )
ROM_END

ROM_START( acedrvrw )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) /* main program */
	ROM_LOAD32_BYTE( "ad2prgll.4d", 0x00003, 0x80000, CRC(808c5ff8) SHA1(119c90ecb5aa099a0e5d1d7944c004beacead367) )
	ROM_LOAD32_BYTE( "ad2prglm.2d", 0x00002, 0x80000, CRC(5f726a10) SHA1(d077312c6a387fbdf906d278c73c6a3730687f32) )
	ROM_LOAD32_BYTE( "ad2prgum.8d", 0x00001, 0x80000, CRC(d5042d6e) SHA1(9ae93e7ea7126302831a879ba0aadcb6e5b842f5) )
	ROM_LOAD32_BYTE( "ad2prguu.6d", 0x00000, 0x80000, CRC(86d4661d) SHA1(2a1529a51ca5466994a2d0d84c7aab13cef95a11) )

	ROM_REGION( 0x400, REGION_GFX1, ROMREGION_DISPOSE )

	ROM_REGION( 0x200000*8, REGION_GFX2, 0 ) /* 16x16x8bpp texture tiles */
	ROM_LOAD( "ad1cg0.1a", 0x200000*0x4, 0x200000,CRC(faaa1ee2) SHA1(878f2b74587ed4d06c5110a0eb0020c49ddc5dfa) )
	ROM_LOAD( "ad1cg1.2a", 0x200000*0x5, 0x200000,CRC(1aab1eb7) SHA1(b8f9eeafec7e0de340cf48e38fa55dd14404c867) )
	ROM_LOAD( "ad1cg2.3a", 0x200000*0x6, 0x200000,CRC(cdcd1874) SHA1(5a7a4a0d897cca4956b0a4f178f39f618c921861) )
	ROM_LOAD( "ad1cg3.5a", 0x200000*0x7, 0x200000,CRC(effdd2cd) SHA1(9ff156e7e38c103b8fa6f3c29776dd38482d9cf2) )

	ROM_REGION( 0x280000, REGION_GFX3, 0 ) /* texture tilemap */
	ROM_LOAD( "ad1ccrl.1c", 0x000000, 0x200000,CRC(bc3c9b12) SHA1(088e861e5c4b37c54b7f72963113a10870bf7927) )
	ROM_LOAD( "ad1ccrh.2c", 0x200000, 0x080000,CRC(71f44526) SHA1(bb4811fc5de626380ce6a17bee73e5e47926d850) )

	ROM_REGION( 0x80000*6, REGION_GFX4, 0 ) /* 3d model data */
	ROM_LOAD( "ad1potl0.5b", 0x80000*0, 0x80000,CRC(dfc7e729) SHA1(5e3deef66d0a5dd2ff0584b8c8be4bf5e798e4d0) )
	ROM_LOAD( "ad1potl1.4b", 0x80000*1, 0x80000,CRC(5914ef8e) SHA1(f6db9c3061ceda76eef0a9538d9c048366b71124) )
	ROM_LOAD( "ad1potm0.5c", 0x80000*2, 0x80000,CRC(844bcd6b) SHA1(629b8dc0a7e94410c08c8874b69d9f4bc22f3e4f) )
	ROM_LOAD( "ad1potm1.4c", 0x80000*3, 0x80000,CRC(515cf541) SHA1(db1522813ea3e982d479cc1903d18799bf75aea9) )
	ROM_LOAD( "ad1potu0.5d", 0x80000*4, 0x80000,CRC(e0f44949) SHA1(ffdb64d600883974b05edaa9ed3071af355ee17f) )
	ROM_LOAD( "ad1potu1.4d", 0x80000*5, 0x80000,CRC(f2cd2cbb) SHA1(19fe6e3454a1e4353c7fe0a0d7a71742fea946de) )

	ROM_REGION( 0x080000, REGION_CPU2, 0 ) /* BIOS */
	ROM_LOAD( "ad1data.6r", 0, 0x080000, CRC(82024f74) SHA1(711ab0c4f027716aeab18e3a5d3d06fa82af8007) )

	ROM_REGION( 0x400000, REGION_USER2, 0 ) /* sound samples */
	ROM_LOAD( "ad1wave0.10r", 0x100000*0, 0x100000,CRC(c7879a72) SHA1(ae04d664858b0944583590ed0003a9420032d5ca) )
	ROM_LOAD( "ad1wave1.10p", 0x100000*1, 0x100000,CRC(69c1d41e) SHA1(b5cdfe7b75075c585dfd842347f8e4e692bb2781) )
	ROM_LOAD( "ad1wave2.10n", 0x100000*2, 0x100000,CRC(365a6831) SHA1(ddaa44a4436d6de120b64a5d130b1ee18f872e19) )
	ROM_LOAD( "ad1wave3.10l", 0x100000*3, 0x100000,CRC(cd8ecb0b) SHA1(7950b5a3a81f5554f57accabc7a623b8265a21a1) )
ROM_END

ROM_START( victlapw )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) /* main program */
	ROM_LOAD32_BYTE( "advprgll.4d", 0x00003, 0x80000, CRC(4dc1b0ab) SHA1(b5913388d16f824af6dbb01b5b0350d510667a87) )
	ROM_LOAD32_BYTE( "advprglm.2d", 0x00002, 0x80000, CRC(7b658bef) SHA1(cf982b49fde0c1897c4c16e77f9eb2a145d8cd42) )
	ROM_LOAD32_BYTE( "advprgum.8d", 0x00001, 0x80000, CRC(af67f2fb) SHA1(f391843ee0d053e33660c60e3718871142d932f2) )
	ROM_LOAD32_BYTE( "advprguu.6d", 0x00000, 0x80000, CRC(b60e5d2b) SHA1(f5740615c2864c5c6433275cf4388bda5122b7a7) )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )

	ROM_REGION( 0x200000*8, REGION_GFX2, 0 ) /* 16x16x8bpp texture tiles */
	ROM_LOAD( "adv1cg0.2a",  0x200000*0x0, 0x200000,CRC(13353848) SHA1(c6c7693e3cb086919daf9fcaf6bf602142213073) )
	ROM_LOAD( "adv1cg1.1c",  0x200000*0x1, 0x200000,CRC(1542066c) SHA1(20a053e919b7a81da2a17d31dc7482832a4d4ffe) )
	ROM_LOAD( "adv1cg2.2d",  0x200000*0x2, 0x200000,CRC(111f371c) SHA1(29d8062daae51b3c1712bd30baa9813a2b5b374d) )
	ROM_LOAD( "adv1cg3.1e",  0x200000*0x3, 0x200000,CRC(a077831f) SHA1(71bb95199b368e48bc474123ca84d19213f73137) )
	ROM_LOAD( "adv1cg4.2f",  0x200000*0x4, 0x200000,CRC(71abdacf) SHA1(64409e6aa40dd9e5a6dd1dc306860fbbf6ee7c3e) )
	ROM_LOAD( "adv1cg5.1j",  0x200000*0x5, 0x200000,CRC(cd6cd798) SHA1(51070997a457c0ace078174569cd548ac2226b2d) )
	ROM_LOAD( "adv1cg6.2k",  0x200000*0x6, 0x200000,CRC(94bdafba) SHA1(41e64fa99b342edd8b0ed95ae9869c23e03399e6) )
	ROM_LOAD( "adv1cg7.1n",	 0x200000*0x7, 0x200000,CRC(18823475) SHA1(a3244d665b59c352593de21f5cb8d55ddf8cee5c) )

	ROM_REGION( 0x280000, REGION_GFX3, 0 ) /* texture tilemap */
	ROM_LOAD( "adv1ccrl.5a",	 0x000000, 0x200000,CRC(dd2b96ae) SHA1(6337ce17e617234c27ebad578ba82451649aad9c) ) /* ident to adv1ccrl.5l */
	ROM_LOAD( "adv1ccrh.5c",	 0x200000, 0x080000,CRC(5719844a) SHA1(a17d7bc239235e9f566931ba4fee1d6ad7964d83) ) /* ident to adv1ccrh.5j */

	ROM_REGION( 0x80000*9, REGION_GFX4, 0 ) /* 3d model data */
	ROM_LOAD( "adv1pot.l0", 0x80000*0, 0x80000,CRC(3b85b2a4) SHA1(84c92ed0105618d4aa5508af344b4b6cfa772567) )
	ROM_LOAD( "adv1pot.l1", 0x80000*1, 0x80000,CRC(601d6488) SHA1(c7932103ba6070e17deb3cc06060eed7789f938e) )
	ROM_LOAD( "adv1pot.l2", 0x80000*2, 0x80000,CRC(a0323a84) SHA1(deadf9a47461df7b137759d6886e676137b39fd2) )
	ROM_LOAD( "adv1pot.m0", 0x80000*3, 0x80000,CRC(20951aa2) SHA1(3de55bded443a5b78699cec4845470b53b22301a) )
	ROM_LOAD( "adv1pot.m1", 0x80000*4, 0x80000,CRC(5aed6fbf) SHA1(8cee781d8a12e00635b9a1e5cc8d82e64b17e8f1) )
	ROM_LOAD( "adv1pot.m2", 0x80000*5, 0x80000,CRC(00cbff92) SHA1(09a11ba064aafc921a7ca0add5898d91b773f10a) )
	ROM_LOAD( "adv1pot.u0", 0x80000*6, 0x80000,CRC(6b73dd2a) SHA1(e3654ab2b62e4f3314558209e37c5636f871a6c7) )
	ROM_LOAD( "adv1pot.u1", 0x80000*7, 0x80000,CRC(c8788f74) SHA1(606e10b05146e3db824aa608745de80584420d12) )
	ROM_LOAD( "adv1pot.u2", 0x80000*8, 0x80000,CRC(e67f29c5) SHA1(16222afb4f1f494711dd00ebb347c824db333bae) )

	ROM_REGION( 8*1024, REGION_USER1, 0 ) /* EPROM */
	ROM_LOAD( "eeprom.9e", 0, 8*1024, CRC(35fd9f7a) SHA1(7dc542795a6b0b9580c5fd1bf80e1e6f2c402078) )

	ROM_REGION( 0x080000, REGION_CPU2, 0 ) /* MCU */
	ROM_LOAD( "adv1data.6r", 0, 0x080000, CRC(10eecdb4) SHA1(aaedeed166614e6670e765e0d7e4e9eb5f38ad10) )
	ROM_REGION( 0x400000, REGION_USER2, 0 ) /* sound samples */
	ROM_LOAD( "adv1wav0.10r", 0x000000, 0x100000, CRC(f07b2d9d) SHA1(fd46c23b336d5e9a748f7f8d825c19737125d2fb) )
	ROM_LOAD( "adv1wav1.10p", 0x100000, 0x100000, CRC(737f3c7a) SHA1(4737994f146c0076e7270785f41f3a85c53c7c5f) )
	ROM_LOAD( "adv1wav2.10n", 0x200000, 0x100000, CRC(c1a5ca5e) SHA1(27e6f9256d5fe5966e91d6be1e6e80900a764af1) )
	ROM_LOAD( "adv1wav3.10l", 0x300000, 0x100000, CRC(fc6b8004) SHA1(5c9e0805895931ec2b6a43376059bdbf5777222f) )
ROM_END

ROM_START( raveracw )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) /* main program */
	ROM_LOAD32_BYTE( "rv2prllb.4d", 0x00003, 0x80000, CRC(3017cd1e) SHA1(ccd648b4a5dfc74fd141815af2969f423311042f) )
	ROM_LOAD32_BYTE( "rv2prlmb.2d", 0x00002, 0x80000, CRC(894be0c3) SHA1(4dba93dc3ca1cf502c5f54018b64ad79bb2a632b) )
	ROM_LOAD32_BYTE( "rv2prumb.8d", 0x00001, 0x80000, CRC(6414a800) SHA1(c278ff644909d12a43ba6fc2bf8d2092e469c3e6) )
	ROM_LOAD32_BYTE( "rv2pruub.6d", 0x00000, 0x80000, CRC(a9f18714) SHA1(8e7b17749d151f92020f68d1ac06003cf1f5c573) )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )

	ROM_REGION( 0x200000*8, REGION_GFX2, 0 ) /* 16x16x8bpp texture tiles */
	ROM_LOAD( "rv1cg0.1a", 0x200000*0x0, 0x200000,CRC(c518f06b) SHA1(4c01d453244192dd13087bdc72a7f7be80b47cbc) ) /* rv1cg0.2a */
	ROM_LOAD( "rv1cg1.1c", 0x200000*0x1, 0x200000,CRC(6628f792) SHA1(7a5405c5fcb2f3f001ae17df393c31e61a834f2b) ) /* rv1cg1.2c */
	ROM_LOAD( "rv1cg2.1d", 0x200000*0x2, 0x200000,CRC(0b707cc5) SHA1(38e1a554b278062edc369565353497ac4b016f78) ) /* rv1cg2.2d */
	ROM_LOAD( "rv1cg3.1e", 0x200000*0x3, 0x200000,CRC(39b62921) SHA1(873287d81338baf10dd85214d82f6c38bfdf199e) ) /* rv1cg3.2e */
	ROM_LOAD( "rv1cg4.1f", 0x200000*0x4, 0x200000,CRC(a9791ea2) SHA1(245b2ebbadd1fbca90dc241f88e9f6f341b2a01a) ) /* rv1cg4.2f */
	ROM_LOAD( "rv1cg5.1j", 0x200000*0x5, 0x200000,CRC(b2c79ec1) SHA1(6f669996863bdf1fe09b0c1a2a876625029d3d43) ) /* rv1cg5.2j */
	ROM_LOAD( "rv1cg6.1k", 0x200000*0x6, 0x200000,CRC(8cddedc2) SHA1(e3993f5505bc7e61bec7be5b48c873572e1220f7) ) /* rv1cg6.2k */
	ROM_LOAD( "rv1cg7.1n", 0x200000*0x7, 0x200000,CRC(b39147ca) SHA1(50ca6691fc809c95e6999dd52e39f2b8c2d22f3b) ) /* rv1cg7.2n */

	ROM_REGION( 0x280000, REGION_GFX3, 0 ) /* texture tilemap */
	ROM_LOAD( "rv1ccrl.5a",	 0x000000, 0x200000,CRC(bc634f72) SHA1(b5c504ed92bca7682614fc4c51f38cff607e6f2a) ) /* rv1ccrl.5l */
	ROM_LOAD( "rv1ccrh.5c",	 0x200000, 0x080000,CRC(a741b262) SHA1(363076220a0eacc67befda05f8253963e8ffbcaa) ) /* rv1ccrh.5j */

	ROM_REGION( 0x80000*12, REGION_GFX4, 0 ) /* 3d model data */
	ROM_LOAD( "rv1potl0.5b", 0x80000*0x0, 0x80000,CRC(de2ce519) SHA1(2fe0dd000571f76d1a4df6a439d40119125170ef) )
	ROM_LOAD( "rv1potl1.4b", 0x80000*0x1, 0x80000,CRC(2215cb5a) SHA1(d48ee692ab3dbcffdc49d22f6f232ca9390da766) )
	ROM_LOAD( "rv1potl2.3b", 0x80000*0x2, 0x80000,CRC(ddb15bf7) SHA1(4c54ec98e0cba10841d43a4ce593cdacfd7f90f8) )
	ROM_LOAD( "rv1potl3.2b", 0x80000*0x3, 0x80000,CRC(fa9361ca) SHA1(35a5c2712bca9c62400b724754de3a931ad21561) )
	ROM_LOAD( "rv1potm0.5c", 0x80000*0x4, 0x80000,CRC(3c024f3a) SHA1(711f0442823797b2d410352796a5cca66af98dce) )
	ROM_LOAD( "rv1potm1.4c", 0x80000*0x5, 0x80000,CRC(b1a32a68) SHA1(e24abb3a7e35d098abae5420bf8ef5c975718987) )
	ROM_LOAD( "rv1potm2.3c", 0x80000*0x6, 0x80000,CRC(a414fe15) SHA1(eb27cdca045ab2ab27dec179043328847fb65e11) )
	ROM_LOAD( "rv1potm3.2c", 0x80000*0x7, 0x80000,CRC(2953bbb4) SHA1(aca1acd87f7130d2522d0c6f8e60beeb7ab7495a) )
	ROM_LOAD( "rv1potu0.5d", 0x80000*0x8, 0x80000,CRC(b9eaf3cc) SHA1(3b2a9041f1fa90706ecf7d4fbff918516f891a07) )
	ROM_LOAD( "rv1potu1.4d", 0x80000*0x9, 0x80000,CRC(a5c55258) SHA1(826d4dde761aec7d848456f7bc4ba6268fe99605) )
	ROM_LOAD( "rv1potu2.3d", 0x80000*0xa, 0x80000,CRC(c18fcb74) SHA1(a4009ae2b014dc89aed4741fd97f84350117c2f4) )
	ROM_LOAD( "rv1potu3.2d", 0x80000*0xb, 0x80000,CRC(79735aaa) SHA1(1cf14274669b916a7641f7a16785da1b72347485) )

	ROM_REGION( 0x2100, REGION_USER1, 0 )
	ROM_LOAD( "rv1eeprm.9e", 0x0000, 0x2000, CRC(801222e6) SHA1(a97ba76ad73f75fe7289e2c0d60b2dfdf2a99604) ) /* EPROM */
	ROM_LOAD( "rr1gam.2d",   0x2000, 0x0100, CRC(b2161bce) SHA1(d2681cc0cf8e68df0d942d392b4eb4458c4bb356) ) /* identical to rr1gam.3d,rr1gam.4d */

	ROM_REGION( 0x080000, REGION_CPU2, 0 ) /* MCU */
	ROM_LOAD( "rv1data.6r", 0, 0x080000, CRC(d358ec20) SHA1(140c513349240417bb546dd2d151f3666b818e91) )
	ROM_REGION( 0x400000, REGION_USER2, 0 ) /* sound samples */
	ROM_LOAD( "rv1wav0.10r", 0x000000, 0x100000, CRC(5aef8143) SHA1(a75d31298e3ff9b290f238976a11e8b85cfb72d3) )
	ROM_LOAD( "rv1wav1.10p", 0x100000, 0x100000, CRC(9ed9e6b3) SHA1(dd1da2b08d1b6aa0912daacc77744c9799aabb78) )
	ROM_LOAD( "rv1wav2.10n", 0x200000, 0x100000, CRC(5af9dc83) SHA1(9aeb7f8217b806a6f3ed93056513af9fbcb6b372) )
	ROM_LOAD( "rv1wav3.10l", 0x300000, 0x100000, CRC(ffb9ad75) SHA1(a9a61a597bd3bbe9732f92747d82264fe4d9af48) )
ROM_END

ROM_START( ridger2j )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) /* main program */
	ROM_LOAD32_BYTE( "rrs1pr11.4d", 0x00003, 0x80000, CRC(fbf785a2) SHA1(b9333c7623f68f48aa6ae50913a22a527a19576a) )
	ROM_LOAD32_BYTE( "rrs1prlm.2d", 0x00002, 0x80000, CRC(562f747a) SHA1(79d818b87c9a992fc9706fb39e6d560c2b0aa392) )
	ROM_LOAD32_BYTE( "rrs1prum.8d", 0x00001, 0x80000, CRC(93259fb0) SHA1(c29787e873797a003db27adbd20d7b852e26d8c6) )
	ROM_LOAD32_BYTE( "rrs1pruu.6d", 0x00000, 0x80000, CRC(31cdefe8) SHA1(ae836d389bed43dd156eb4cf3e97b6f1ad68181e) )

	ROM_REGION( 0x400, REGION_GFX1, ROMREGION_DISPOSE )

	ROM_REGION( 0x200000*8, REGION_GFX2, 0 ) /* 16x16x8bpp texture tiles */
	ROM_LOAD( "rrs1cg0.1a", 0x200000*0x4, 0x200000,CRC(714c0091) SHA1(df29512bd6e64827660c40304051366d2c4d7977) )
	ROM_LOAD( "rrs1cg1.2a", 0x200000*0x5, 0x200000,CRC(836545c1) SHA1(05e3346463d8d42b5d33216207e855033a65510d) )
	ROM_LOAD( "rrs1cg2.3a", 0x200000*0x6, 0x200000,CRC(00e9799d) SHA1(280184451138420f64080efe13e5e2795f7b61d4) )
	ROM_LOAD( "rrs1cg3.5a", 0x200000*0x7, 0x200000,CRC(3858983f) SHA1(feda270b71f1310ecf4c17823bc8827ca2951b40) )

	ROM_REGION( 0x280000, REGION_GFX3, 0 ) /* texture tilemap */
	ROM_LOAD( "rrs1ccrl.5a", 0x000000, 0x200000,CRC(304a8b57) SHA1(f4f3e7c194697d754375f36a0e41d0941fa5d225) )
	ROM_LOAD( "rrs1ccrh.5c", 0x200000, 0x080000,CRC(bd3c86ab) SHA1(cd3a8774843c5864e651fa8989c80e2d975a13e8) )

	ROM_REGION( 0x80000*6, REGION_GFX4, 0 ) /* 3d model data */
	ROM_LOAD( "rrs1pol0.5b", 0x80000*0, 0x80000,CRC(9376c384) SHA1(cde0e36db1beab1523607098a760d81fac2ea90e) )
	ROM_LOAD( "rrs1pol1.4b", 0x80000*1, 0x80000,CRC(094fa832) SHA1(cc59442540b1cdef068c4408b6e048c11042beb8) )
	ROM_LOAD( "rrs1pom0.5c", 0x80000*2, 0x80000,CRC(b47a7f8b) SHA1(0fa0456ad8b4864a7071b5b5ba1a78877c1ac0f0) )
	ROM_LOAD( "rrs1pom1.4c", 0x80000*3, 0x80000,CRC(27260361) SHA1(8775cc779eb8b6a0d79fa84d606c970ec2d6ea8d) )
	ROM_LOAD( "rrs1pou0.5d", 0x80000*4, 0x80000,CRC(74d6ec84) SHA1(63f5beee51443c98100330ec04291f71e10716c4) )
	ROM_LOAD( "rrs1pou1.4d", 0x80000*5, 0x80000,CRC(f527caaa) SHA1(f92bdd15323239d593ddac92a11d23a27e6635ed) )

	ROM_REGION( 0x080000, REGION_CPU2, 0 ) /* BIOS/music data? */
	ROM_LOAD( "rrs1data.6r", 0, 0x080000, CRC(b7063aa8) SHA1(08ff689e8dd529b91eee423c93f084945c6de417) )
	/* 0x00000..0x001ff: data
	 * 0x10000..0x1fc7f: music data?
	 * 0x20000..0x2f57f: music data?
	 * 0x30000..0x334ff: code? contains english text
	 */
	ROM_REGION( 0x400000, REGION_USER2, 0 ) /* sound samples */
	ROM_LOAD( "rrs1wav0.10r", 0x100000*0, 0x100000,CRC(99d11a2d) SHA1(1f3db98a99be0f07c03b0a7817561459a58f310e) )
	ROM_LOAD( "rrs1wav1.10p", 0x100000*1, 0x100000,CRC(ad28444a) SHA1(c31bbf3cae5015e5494fe4988b9b01d822224c69) )
	ROM_LOAD( "rrs1wav2.10n", 0x100000*2, 0x100000,CRC(6f0d4619) SHA1(cd3d57f2ea21497f388ffa29ec7d2665647a01c0) )
	ROM_LOAD( "rrs1wav3.10l", 0x100000*3, 0x100000,CRC(106e761f) SHA1(97f47b857bdcbc79b0aface53dd385e67fcc9108) )
ROM_END

ROM_START( ridgeraj )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) /* main program */
	ROM_LOAD32_BYTE( "rr1prll.4d", 0x00003, 0x80000, CRC(4bb7fc86) SHA1(8291375b8ec4d37e0d9e3bf38da2d5907b0f31bd) )
	ROM_LOAD32_BYTE( "rr1prlm.2d", 0x00002, 0x80000, CRC(68e13830) SHA1(ddc447c7afbb5c4238969d7e78bfe9cf8fac6061) )
	ROM_LOAD32_BYTE( "rr1prum.8d", 0x00001, 0x80000, CRC(705ef78a) SHA1(881903413e66d6fd83d46eb18c4e1230531832ae) )
	ROM_LOAD32_BYTE( "rr1pruu.6d", 0x00000, 0x80000, CRC(c1371f96) SHA1(a78e0bf6c147c034487a85efa0a8470f4e8f4bf0) )

	ROM_REGION( 0x400, REGION_GFX1, ROMREGION_DISPOSE )

	ROM_REGION( 0x200000*8, REGION_GFX2, 0 ) /* 16x16x8bpp texture tiles */
	ROM_LOAD( "rr1cg0.1a", 0x200000*0x4, 0x200000,CRC(d1b0eec6) SHA1(f66922c324dfc3ff408db7556c587ef90ca64c3b) )
	ROM_LOAD( "rr1cg1.2a", 0x200000*0x5, 0x200000,CRC(bb695d89) SHA1(557bac9d2718519c1f69e374d0ef9a86a43fe86c) )
	ROM_LOAD( "rr1cg2.3a", 0x200000*0x6, 0x200000,CRC(8f374c0a) SHA1(94ff8581de11a03ef86525155f8433bf5858b980) )
	ROM_LOAD( "rr1cg3.5a", 0x200000*0x7, 0x200000,CRC(072a5c47) SHA1(86b8e973ae6b78197d685fe6d14722d8e2d0dfec) )

	ROM_REGION( 0x280000, REGION_GFX3, 0 ) /* texture tilemap */
	ROM_LOAD( "rr1ccrl.5a", 0x000000, 0x200000,CRC(c15cb257) SHA1(0cb8f231c62ea37955be5d452a436a6e815af8e8) )
	ROM_LOAD( "rr1ccrh.5c", 0x200000, 0x080000,CRC(dd332fd5) SHA1(a7d9c1d6b5a8e3a937b525c1363880e404dcd147) )

	ROM_REGION( 0x80000*6, REGION_GFX4, 0 ) /* 3d model data */
	ROM_LOAD( "rr1potl0.5b", 0x80000*0, 0x80000,CRC(3ac193e3) SHA1(ff213766f15e34dc1b25187b57d94e17930090a3) )
	ROM_LOAD( "rr1potl1.4b", 0x80000*1, 0x80000,CRC(ac3ffba5) SHA1(4eb4dda5faeff237e0d35725b56d309948fba900) )
	ROM_LOAD( "rr1potm0.5c", 0x80000*2, 0x80000,CRC(42a3fa08) SHA1(15db0ae7ccf7f5a77b9dd9a9d82a488b67f8aaff) )
	ROM_LOAD( "rr1potm1.4c", 0x80000*3, 0x80000,CRC(1bc1ea7b) SHA1(52c21eef4989c45acc5fa4deda2d0b63214731c8) )
	ROM_LOAD( "rr1potu0.5d", 0x80000*4, 0x80000,CRC(5e367f72) SHA1(5887f011379dce865fef238b402678a3d2033de9) )
	ROM_LOAD( "rr1potu1.4d", 0x80000*5, 0x80000,CRC(31d92475) SHA1(51d3c0baa223e1bc16ea2950f2e085597528f870) )

	ROM_REGION( 0x080000, REGION_CPU2, 0 ) /* BIOS */
	ROM_LOAD( "rr1data.6r", 0, 0x080000, CRC(18f5f748) SHA1(e0d149a66de36156edd9b55f604c9a9801aaefa8) )

	ROM_REGION( 0x400000, REGION_USER2, 0 ) /* sound samples */
	ROM_LOAD( "rr1wav0.10r", 0x100000*0, 0x100000,CRC(a8e85bde) SHA1(b56677e9f6c98f7b600043f5dcfef3a482ca7455) )
	ROM_LOAD( "rr1wav1.10p", 0x100000*1, 0x100000,CRC(35f47c8e) SHA1(7c3f9e942f532af8008fbead2a96fee6084bcde6) )
	ROM_LOAD( "rr1wav2.10n", 0x100000*2, 0x100000,CRC(3244cb59) SHA1(b3283b30cfafbfdcbc6d482ecc4ed6a47a527ca4) )
	ROM_LOAD( "rr1wav3.10l", 0x100000*3, 0x100000,CRC(c4cda1a7) SHA1(60bc96880ec79efdff3cc70c09e848692a40bea4) )
ROM_END

ROM_START( timecrsa )
	ROM_REGION( 0x400000, REGION_CPU1, 0 ) /* main program */
	ROM_LOAD32_WORD_SWAP( "ts2ver-a.1", 0x00002, 0x200000, CRC(d57eb74b) SHA1(536dd9305d0ac44110c575776333310cc57b5242) )
	ROM_LOAD32_WORD_SWAP( "ts2ver-a.2", 0x00000, 0x200000, CRC(671588af) SHA1(63f992c6795521fd263a0ebf230f8dc88cbfc443) )

	ROM_REGION( 0x200000*6, REGION_GFX1, ROMREGION_DISPOSE ) /* 32x32x8bpp sprite tiles */
	ROM_LOAD( "ts1scg0.12f",0x200000*0, 0x200000,CRC(14a3674d) SHA1(c5792a385572452b43bbc7eb8428335b19daa3c0) )
	ROM_LOAD( "ts1scg1.10f",0x200000*1, 0x200000,CRC(11791dbf) SHA1(3d75b468d69a8bf398d45f310cdb8bc88b63f25c) )
	ROM_LOAD( "ts1scg2.8f", 0x200000*2, 0x200000,CRC(d630fff9) SHA1(691394027b858702f06282f965f5b53e6fed496b) )
	ROM_LOAD( "ts1scg3.7f", 0x200000*3, 0x200000,CRC(1a62f015) SHA1(7d09ae480ae7813391616ae0090929ba845a345a) )
	ROM_LOAD( "ts1scg4.5f", 0x200000*4, 0x200000,CRC(511b8dd6) SHA1(936649c0a61d29f024a28e4ab64cce4b55d58f64) )
	ROM_LOAD( "ts1scg5.3f", 0x200000*5, 0x200000,CRC(553bb246) SHA1(94659bee4fd0afe834a8bf3414d8825411cf9e86) )

	ROM_REGION( 0x200000*8, REGION_GFX2, 0 ) /* 16x16x8bpp texture tiles */
	ROM_LOAD( "ts1cg0.8d",   0x200000*0x0, 0x200000,CRC(de07b22c) SHA1(f4d07b8840ec8be625eff634bce619e960c334a5) )
	ROM_LOAD( "ts1cg1.10d",  0x200000*0x1, 0x200000,CRC(992d26f6) SHA1(a0b9007312804b413d4c1748527378da4d8d53b3) )
	ROM_LOAD( "ts1cg2.12d",  0x200000*0x2, 0x200000,CRC(6273954f) SHA1(d73a43888b53e4c42fc33e8e1b38e60fd3329413) )
	ROM_LOAD( "ts1cg3.13d",  0x200000*0x3, 0x200000,CRC(38171f24) SHA1(d04caaa5b1b377ced38501b014e4cb7fc831c41d) )
	ROM_LOAD( "ts1cg4.14d",  0x200000*0x4, 0x200000,CRC(51f09856) SHA1(0eef421907ee813d5117e62cf0005bf00eb29c88) )
	ROM_LOAD( "ts1cg5.16d",  0x200000*0x5, 0x200000,CRC(4cd9fd79) SHA1(0d2018ec914683a75bdec8655d678fd562eb6d15) )
	ROM_LOAD( "ts1cg6.18d",  0x200000*0x6, 0x200000,CRC(f17f2ec9) SHA1(ed88ec524626e5bbe2e1ea6838412d3ac85671dd) )

	ROM_REGION( 0x280000, REGION_GFX3, 0 ) /* texture tilemap */
	ROM_LOAD( "ts1ccrl.3d",	 0x000000, 0x200000,CRC(56cad2df) SHA1(49c0e57d5cf5d5fc4c75da6969bec01d6d443259) )
	ROM_LOAD( "ts1ccrh.1d",	 0x200000, 0x080000,CRC(a1cc3741) SHA1(7fe57924c42e287b134e5d7ad00cffdff1f18084) )

	ROM_REGION( 0x80000*9, REGION_GFX4, 0 ) /* 3d model data */
	ROM_LOAD( "ts1ptrl0.18k", 0x80000*0, 0x80000,CRC(e5f2d275) SHA1(2f5057e65ec8a3ec03f841f15f10769ae1f69139) )
	ROM_LOAD( "ts1ptrl1.16k", 0x80000*1, 0x80000,CRC(2bba3800) SHA1(1d9c944cb06417cb0ac47a58b922dddb83387586) )
	ROM_LOAD( "ts1ptrl2.15k", 0x80000*2, 0x80000,CRC(d4441c08) SHA1(6a6bb9cecbf35cb81b7681e220fc33df9a01d07f) )
	ROM_LOAD( "ts1ptrm0.18j", 0x80000*3, 0x80000,CRC(8aea02ba) SHA1(44ba85ba6d59758448d17ec39dfb628882ddc684) )
	ROM_LOAD( "ts1ptrm1.16j", 0x80000*4, 0x80000,CRC(bccf19bc) SHA1(4a6566948bdd2b0f82b7c30e57d3fe65005c26e3) )
	ROM_LOAD( "ts1ptrm2.15j", 0x80000*5, 0x80000,CRC(7280be31) SHA1(476b7171ae855d8bbd968ccbaa55b5100d274e3b) )
	ROM_LOAD( "ts1ptru0.18f", 0x80000*6, 0x80000,CRC(c30d6332) SHA1(a5c59d0abfe38de975fa0d606ed8500eb02008b7) )
	ROM_LOAD( "ts1ptru1.16f", 0x80000*7, 0x80000,CRC(993cde84) SHA1(c9cdcca1d60bcc41ad881c02dda9895563963ead) )
	ROM_LOAD( "ts1ptru2.15f", 0x80000*8, 0x80000,CRC(7cb25c73) SHA1(616eab3ac238864a584394f7ec8736ece227974a) )

	ROM_REGION( 0x080000, REGION_CPU2, 0 ) /* BIOS */
	ROM_LOAD( "ts1data.8k", 0, 0x080000, CRC(e68aa973) SHA1(663e80d249be5d5841139d98a9d72e2396851272) )
	ROM_REGION( 0x800000, REGION_USER2, 0 ) /* sound samples */
	ROM_LOAD( "ts1wavea.2l", 0x000000, 0x400000, CRC(d1123301) SHA1(4bf1fd746fef4e6befa63c61a761971d729e1573) )
	ROM_LOAD( "ts1waveb.1l", 0x400000, 0x200000, CRC(bf4d7272) SHA1(c7c7b3620e7b3176644b6784ee36e679c9e31cc1) )
ROM_END

INPUT_PORTS_START( alpiner )
	PORT_START /* DIP4 */
	PORT_DIPNAME( 0x01, 0x00, "DIP1" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "DIP2" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "DIP3" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "DIP4" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "DIP5" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "DIP6" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "DIP7" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "DIP8" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_SERVICE|IPF_TOGGLE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) /* DECISION */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 ) /* L SELECTION */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON3 ) /* R SELECTION */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START /* SWING */
	PORT_ANALOG( 0xffff, 0x8000, IPT_AD_STICK_X|IPF_CENTER, 100, 4, 0x00, 0xffff )

	PORT_START /* EDGE */
	PORT_ANALOG( 0xffff, 0x8000, IPT_AD_STICK_Y|IPF_CENTER, 100, 4, 0x00, 0xffff )
INPUT_PORTS_END

INPUT_PORTS_START( cybrcycc )
	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, "DIP2-1 (test mode)" )
	PORT_DIPSETTING(    0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, "DIP2-2" )
	PORT_DIPSETTING(    0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, "DIP2-3" )
	PORT_DIPSETTING(    0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "DIP2-4" )
	PORT_DIPSETTING(    0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, "DIP2-5" )
	PORT_DIPSETTING(    0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, "DIP2-6" )
	PORT_DIPSETTING(    0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "DIP2-7" )
	PORT_DIPSETTING(    0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "DIP2-8" )
	PORT_DIPSETTING(    0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, "DIP3-1" )
	PORT_DIPSETTING(    0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, "DIP3-2" )
	PORT_DIPSETTING(    0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, "DIP3-3" )
	PORT_DIPSETTING(    0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, "DIP3-4" )
	PORT_DIPSETTING(    0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, "DIP3-5" )
	PORT_DIPSETTING(    0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, "DIP3-6" )
	PORT_DIPSETTING(    0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, "DIP3-7" )
	PORT_DIPSETTING(    0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, "DIP3-8 (test mode)" )
	PORT_DIPSETTING(    0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )

	PORT_START /* 1:gas */
	PORT_ANALOG( 0xffff, 0x8000, IPT_AD_STICK_Y|IPF_CENTER|IPF_PLAYER1,	100, 4, 0x00, 0xffff )

	PORT_START /* 2:brake */
	PORT_ANALOG( 0xffff, 0x8000, IPT_AD_STICK_Y|IPF_CENTER|IPF_PLAYER2,	100, 4, 0x00, 0xffff )

	PORT_START /* 3:steering */
	PORT_ANALOG( 0xffff, 0x8000, IPT_AD_STICK_X|IPF_CENTER, 100, 4, 0x00, 0xffff )

	PORT_START /* 4 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 )	/* stick shift down */
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON2 )	/* stick shift up */
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 )	/* view */
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_COIN2 )	/* (unused) */
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_TILT )		/* motion stop (unused) */
INPUT_PORTS_END

INPUT_PORTS_START( propcycl )
	PORT_START /* DIP4 */
	PORT_DIPNAME( 0x01, 0x01, "DIP1" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "DIP2" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "DIP3" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "DIP4" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "DIP5" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "DIP6" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "DIP7" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "DIP8" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_SERVICE|IPF_TOGGLE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE ) /* good */
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START1 )

/*	PORT_START*/
/*	PORT_ANALOG( 0xffff, 0x8000, IPT_AD_STICK_X|IPF_CENTER, 100, 4, 0x00, 0xffff )*/

/*	PORT_START*/
/*	PORT_ANALOG( 0xffff, 0x8000, IPT_AD_STICK_Y|IPF_CENTER, 100, 4, 0x00, 0xffff )*/
INPUT_PORTS_END

INPUT_PORTS_START( victlap )
	PORT_START /* 0: DIP2 and DIP3 */
	PORT_DIPNAME( 0x0001, 0x0001, "DIP2-1" )
	PORT_DIPSETTING(    0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, "DIP2-2" )
	PORT_DIPSETTING(    0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, "DIP2-3" )
	PORT_DIPSETTING(    0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "DIP2-4" )
	PORT_DIPSETTING(    0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, "DIP2-5" )
	PORT_DIPSETTING(    0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, "DIP2-6" )
	PORT_DIPSETTING(    0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "DIP2-7" )
	PORT_DIPSETTING(    0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "DIP2-8" )
	PORT_DIPSETTING(    0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, "DIP3-1" )
	PORT_DIPSETTING(    0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, "DIP3-2" )
	PORT_DIPSETTING(    0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, "DIP3-3" )
	PORT_DIPSETTING(    0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, "DIP3-4" )
	PORT_DIPSETTING(    0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, "DIP3-5" )
	PORT_DIPSETTING(    0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, "DIP3-6" )
	PORT_DIPSETTING(    0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, "DIP3-7 (TEST MODE)" )
	PORT_DIPSETTING(    0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, "DIP3-8" )
	PORT_DIPSETTING(    0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )

	PORT_START /* 1:gas */
	PORT_ANALOG( 0xffff, 0x8000, IPT_AD_STICK_Y|IPF_CENTER|IPF_PLAYER1,	100, 4, 0x00, 0xffff )

	PORT_START /* 2:brake */
	PORT_ANALOG( 0xffff, 0x8000, IPT_AD_STICK_Y|IPF_CENTER|IPF_PLAYER2,	100, 4, 0x00, 0xffff )

	PORT_START /* 3:steering */
	PORT_ANALOG( 0xffff, 0x8000, IPT_AD_STICK_X|IPF_CENTER, 100, 4, 0x00, 0xffff )

	PORT_START /* 4 */
	PORT_DIPNAME( 0x0001, 0x0001, "DIP4-1 (Shift=Down)" )
	PORT_DIPSETTING(    0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, "DIP4-2 (Shift=Up)" )
	PORT_DIPSETTING(    0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, "DIP4-3 (unused)" )
	PORT_DIPSETTING(    0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "DIP4-4" )
	PORT_DIPSETTING(    0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, "DIP4-5" )
	PORT_DIPSETTING(    0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, "DIP4-6" )
	PORT_DIPSETTING(    0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "DIP4-7 (PREV) View" )
	PORT_DIPSETTING(    0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "DIP4-8 (NEXT)" )
	PORT_DIPSETTING(    0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, "DIP4-9 (CHOOSE)" )
	PORT_DIPSETTING(    0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, "DIP4-A (Coin2)" )
	PORT_DIPSETTING(    0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, "DIP4-B" )
	PORT_DIPSETTING(    0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, "DIP4-C (Service)" )
	PORT_DIPSETTING(    0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, "DIP4-D (Coin1)" )
	PORT_DIPSETTING(    0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, "DIP4-E" )
	PORT_DIPSETTING(    0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, "DIP4-F" )
	PORT_DIPSETTING(    0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, "DIP4-10 (Motion Stop)" )
	PORT_DIPSETTING(    0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
INPUT_PORTS_END


INPUT_PORTS_START( ridgera )
	PORT_START /* 0: DIP2 and DIP3 */
	PORT_DIPNAME( 0x0001, 0x0001, "DIP2-1" )
	PORT_DIPSETTING(    0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, "DIP2-2" )
	PORT_DIPSETTING(    0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, "DIP2-3" )
	PORT_DIPSETTING(    0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "DIP2-4" )
	PORT_DIPSETTING(    0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, "DIP2-5" )
	PORT_DIPSETTING(    0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, "DIP2-6" )
	PORT_DIPSETTING(    0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "DIP2-7" )
	PORT_DIPSETTING(    0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "DIP2-8" )
	PORT_DIPSETTING(    0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, "DIP3-1" )
	PORT_DIPSETTING(    0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, "DIP3-2" )
	PORT_DIPSETTING(    0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, "DIP3-3" )
	PORT_DIPSETTING(    0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, "DIP3-4" )
	PORT_DIPSETTING(    0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, "DIP3-5" )
	PORT_DIPSETTING(    0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, "DIP3-6" )
	PORT_DIPSETTING(    0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, "DIP3-7 (TEST MODE)" )
	PORT_DIPSETTING(    0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, "DIP3-8" )
	PORT_DIPSETTING(    0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )

	PORT_START /* 1:gas */
	PORT_ANALOG( 0xffff, 0x8000, IPT_AD_STICK_Y|IPF_CENTER|IPF_PLAYER1,	100, 4, 0x00, 0xffff )

	PORT_START /* 2:brake */
	PORT_ANALOG( 0xffff, 0x8000, IPT_AD_STICK_Y|IPF_CENTER|IPF_PLAYER2,	100, 4, 0x00, 0xffff )

	PORT_START /* 3:steering */
	PORT_ANALOG( 0xffff, 0x8000, IPT_AD_STICK_X|IPF_CENTER, 100, 4, 0x00, 0xffff )

	PORT_START /* 4 */
	PORT_DIPNAME( 0x0001, 0x0001, "DIP4-1 (Shift=Down)" )
	PORT_DIPSETTING(    0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, "DIP4-2 (Shift=Up)" )
	PORT_DIPSETTING(    0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, "DIP4-3 (unused)" )
	PORT_DIPSETTING(    0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "DIP4-4" )
	PORT_DIPSETTING(    0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, "DIP4-5" )
	PORT_DIPSETTING(    0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, "DIP4-6" )
	PORT_DIPSETTING(    0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "DIP4-7 (PREV) View" )
	PORT_DIPSETTING(    0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "DIP4-8 (NEXT)" )
	PORT_DIPSETTING(    0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, "DIP4-9 (CHOOSE)" )
	PORT_DIPSETTING(    0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, "DIP4-A (Coin2)" )
	PORT_DIPSETTING(    0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, "DIP4-B" )
	PORT_DIPSETTING(    0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, "DIP4-C (Service)" )
	PORT_DIPSETTING(    0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, "DIP4-D (Coin1)" )
	PORT_DIPSETTING(    0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, "DIP4-E" )
	PORT_DIPSETTING(    0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, "DIP4-F" )
	PORT_DIPSETTING(    0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, "DIP4-10 (Motion Stop)" )
	PORT_DIPSETTING(    0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( raveracw )
	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, "DIP2-1" )
	PORT_DIPSETTING(    0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, "DIP2-2" )
	PORT_DIPSETTING(    0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, "DIP2-3" )
	PORT_DIPSETTING(    0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "DIP2-4" )
	PORT_DIPSETTING(    0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, "DIP2-5" )
	PORT_DIPSETTING(    0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, "DIP2-6" )
	PORT_DIPSETTING(    0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "DIP2-7" )
	PORT_DIPSETTING(    0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "DIP2-8" )
	PORT_DIPSETTING(    0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, "DIP3-1 (dump memory)" )
	PORT_DIPSETTING(    0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, "DIP3-2" )
	PORT_DIPSETTING(    0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, "DIP3-3" )
	PORT_DIPSETTING(    0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, "DIP3-4" ) /* screen flip? */
	PORT_DIPSETTING(    0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, "DIP3-5" )
	PORT_DIPSETTING(    0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, "DIP3-6" )
	PORT_DIPSETTING(    0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, "DIP3-7" )
	PORT_DIPSETTING(    0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, "DIP3-8 (test mode)" )
	PORT_DIPSETTING(    0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )

	PORT_START /* 1:gas */
	PORT_ANALOG( 0xffff, 0x8000, IPT_AD_STICK_Y|IPF_CENTER|IPF_PLAYER1,	100, 4, 0x00, 0xffff )

	PORT_START /* 2:brake */
	PORT_ANALOG( 0xffff, 0x8000, IPT_AD_STICK_Y|IPF_CENTER|IPF_PLAYER2,	100, 4, 0x00, 0xffff )

	PORT_START /* 3:steering */
	PORT_ANALOG( 0xffff, 0x8000, IPT_AD_STICK_X|IPF_CENTER, 100, 4, 0x00, 0xffff )

	PORT_START /* 4 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 )	/* stick shift down */
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON2 )	/* stick shift up */
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 )	/* view */
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_COIN2 )	/* (unused) */
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_TILT )		/* motion stop (unused) */
INPUT_PORTS_END

INPUT_PORTS_START( cybrcomm )
	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, "DIP2-1" )
	PORT_DIPSETTING(    0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, "DIP2-2" )
	PORT_DIPSETTING(    0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, "DIP2-3" )
	PORT_DIPSETTING(    0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "DIP2-4" )
	PORT_DIPSETTING(    0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, "DIP2-5" )
	PORT_DIPSETTING(    0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, "DIP2-6" )
	PORT_DIPSETTING(    0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "DIP2-7" )
	PORT_DIPSETTING(    0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "DIP2-8" )
	PORT_DIPSETTING(    0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, "DIP3-1" )
	PORT_DIPSETTING(    0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, "DIP3-2" )
	PORT_DIPSETTING(    0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, "DIP3-3 (TEST MODE)" )
	PORT_DIPSETTING(    0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, "DIP3-4" )
	PORT_DIPSETTING(    0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, "DIP3-5" )
	PORT_DIPSETTING(    0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, "DIP3-6" )
	PORT_DIPSETTING(    0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, "DIP3-7" )
	PORT_DIPSETTING(    0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, "DIP3-8" )
	PORT_DIPSETTING(    0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( timecris )
	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, "DIP2-1" )
	PORT_DIPSETTING(    0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, "DIP2-2" )
	PORT_DIPSETTING(    0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, "DIP2-3" )
	PORT_DIPSETTING(    0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "DIP2-4" )
	PORT_DIPSETTING(    0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, "DIP2-5" )
	PORT_DIPSETTING(    0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, "DIP2-6" )
	PORT_DIPSETTING(    0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "DIP2-7" )
	PORT_DIPSETTING(    0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "DIP2-8" )
	PORT_DIPSETTING(    0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, "DIP3-1 (dump memory)" )
	PORT_DIPSETTING(    0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, "DIP3-2" )
	PORT_DIPSETTING(    0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, "DIP3-3" )
	PORT_DIPSETTING(    0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, "DIP3-4" ) /* screen flip? */
	PORT_DIPSETTING(    0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, "DIP3-5" )
	PORT_DIPSETTING(    0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, "DIP3-6" )
	PORT_DIPSETTING(    0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, "DIP3-7" )
	PORT_DIPSETTING(    0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, "DIP3-8 (test mode)" )
	PORT_DIPSETTING(    0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )

	PORT_START /* 1:gas */
	PORT_ANALOG( 0xffff, 0x8000, IPT_AD_STICK_Y|IPF_CENTER|IPF_PLAYER1,	100, 4, 0x00, 0xffff )

	PORT_START /* 2:brake */
	PORT_ANALOG( 0xffff, 0x8000, IPT_AD_STICK_Y|IPF_CENTER|IPF_PLAYER2,	100, 4, 0x00, 0xffff )

	PORT_START /* 3:steering */
	PORT_ANALOG( 0xffff, 0x8000, IPT_AD_STICK_X|IPF_CENTER, 100, 4, 0x00, 0xffff )

	PORT_START /* 4 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 )	/* stick shift down */
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON2 )	/* stick shift up */
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 )	/* view */
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_COIN2 )	/* (unused) */
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_TILT )		/* motion stop (unused) */
INPUT_PORTS_END

/*****************************************************************************************************/

/**
 * helper function for FixQuads
 */
static unsigned
GetPolyData( unsigned addr )
{
	size_t ptRomSize = memory_region_length(REGION_GFX4)/3;
	data8_t *pPolyL = memory_region(REGION_GFX4);
	data8_t *pPolyM = pPolyL + ptRomSize;
	data8_t *pPolyH = pPolyM + ptRomSize;
	return (pPolyH[addr]*256+pPolyM[addr])*256+pPolyL[addr];
}

/**
 * helper function for FixQuads
 */
static void PutPolyData( unsigned addr, unsigned data )
{
	size_t ptRomSize = memory_region_length(REGION_GFX4)/3;
	data8_t *pPolyL = memory_region(REGION_GFX4);
	data8_t *pPolyM = pPolyL + ptRomSize;
	data8_t *pPolyH = pPolyM + ptRomSize;
	/* log_cb(RETRO_LOG_DEBUG, LOGPRE  "%08x: %02x\n", addr, pPolyM[addr]^((data>>8)&0xff) ); */
	pPolyH[addr] = data>>16;
	pPolyM[addr] = (data>>8)&0xff;
	pPolyL[addr] = data&0xff;
}

/**
 * helper function called from DecryptPointROMs
 *
 * This routine walks a polygon definition, and applies a heuristic to the middle byte of each X/Y/Z coordinate.
 * We look at the sign of the most significant byte.  If it is negative, we fill the middle byte with 0xff.
 * If it is positive, we fill the middle byte with 0x00.
 *
 * This is a crude "fix" at best.  But it preserves the most significant and least significant bytes,
 * and avoids absurdly-sized polygons which make scenes unrecognizable.
 */
static void
FixQuads( INT32 *pIndex )
{
	int i;
	INT32 addr = *pIndex;
	INT32 size = GetPolyData(addr++);

	*pIndex = addr+(size&0xff);

	while( addr<*pIndex )
	{
		size = GetPolyData(addr++);
		size &= 0xff;

		switch( size )
		{
		case 0x0d:
			break;

		case 0x10:
			break;

		case 0x17:
			for( i=3+8; i<3+12+8; i++ )
			{
				UINT32 data = GetPolyData(addr+i);
				if( data&0x800000 ) data |= 0x00ff00; else data &= 0xff00ff; /* replace middle byte with -1 or 0 */
				PutPolyData( addr+i, data );
			}
			break;

		case 0x18:
			for( i=4+8; i<4+12+8; i++ )
			{
				UINT32 data = GetPolyData(addr+i);
				if( data&0x800000 ) data |= 0x00ff00; else data &= 0xff00ff; /* replace middle byte with -1 or 0 */
				PutPolyData( addr+i, data );
			}
			break;

		default:
			exit(1);
			break;
		}
		addr += size;
	}
}

/**
 * Alpine Racer is working for the most part, except that the middle byte of each 24 bit value in
 * Point ROMs is often scrambled.  If you attempt to render scenes using the value "as-is" you get
 * distorted polygons/textures or worse.
 *
 * The following routine walks through point ROM data, and applies several heuristics to help
 * correct the data.  It's far from perfect, but still 100% better than using the raw values.
 * This stuff could probably be hand corrected, especially if a "polygon viewer" program were written
 * for use on the main CPU, and someone were willing to collect a lot of screenshots.  But it's
 * a lot of trouble.
 */
static void
DecryptPointROMs( void )
{
	int i;
	int prev = 0xe00;
	/* walk the lookup tables in the Point ROM headers, and assume that it always increments. */
	for( i = 0x45; i<0xe2d; i++ )
	{
		int iNext = GetPolyData(i)&0xff;
		if( (iNext&0xff)<=(prev&0xff) )
		{
			prev += 0x100;
		}
		prev = (prev&0xffff00)|(iNext&0xff);
		PutPolyData( i, prev );
	}

	/* process each polygon */
	{
		INT32 iFinish = GetPolyData(3);
		INT32 iDest   = GetPolyData(0x45);
		INT32 iIndex  = GetPolyData(iDest);
		INT32 iMaster = 0x45;
		while( iIndex<iFinish )
		{
			int count = GetPolyData(iMaster+1) - GetPolyData(iMaster) - 1;
			while( count-- > 0 )
			{
				PutPolyData( iDest++, iIndex );
				FixQuads( &iIndex );
				if( iIndex>=iFinish ) break;
			}
			PutPolyData( iDest++, 0xffffff );
			iMaster++;
		}
	}

	/* the following patch hand-corrects the polygon used for the title screen */
	PutPolyData( 0x0077d0+13, 0xfffd80 );
	PutPolyData( 0x0077d0+14, 0x0001e0 );
	PutPolyData( 0x0077d0+15, 0x000000 );

	PutPolyData( 0x0077d0+16, 0x000280 );
	PutPolyData( 0x0077d0+17, 0x0001e0 );
	PutPolyData( 0x0077d0+18, 0x000000 );

	PutPolyData( 0x0077d0+19, 0x000280 );
	PutPolyData( 0x0077d0+20, 0xfffe20 );
	PutPolyData( 0x0077d0+21, 0x000000 );

	PutPolyData( 0x0077d0+22, 0xfffd80 );
	PutPolyData( 0x0077d0+23, 0xfffe20 );
	PutPolyData( 0x0077d0+24, 0x000000 );
}

DRIVER_INIT( alpiner )
{
	namcos22_gametype = NAMCOS22_ALPINE_RACER;
	DecryptPointROMs();
}

/*****************************************************************************************************/

DRIVER_INIT( airco22 )
{ /* patch DSP RAM test */
	data32_t *pROM = (data32_t *)memory_region(REGION_CPU1);
	pROM[0x6d74/4] &= 0x0000ffff;
	pROM[0x6d74/4] |= 0x4e710000;

	namcos22_gametype = NAMCOS22_AIR_COMBAT22;
	/* int1 writes 0x700005 */
	/* int2 writes 0x700007 */
	/* int3 writes 0x700006 */
	/* int4 700004, 700014, proc */
	/* int5 rte */
	/* int6 700004, 700014, proc */
	/* int7 rte */
}

DRIVER_INIT( propcycl )
{
	data32_t *pROM = (data32_t *)memory_region(REGION_CPU1);

	/* patch out protection */
	pROM[0x1992C/4] = 0x4E754E75;

	/**
	 * The dipswitch reading routine in Prop Cycle polls the
	 * dipswitch value, but promptly overwrites with zero, discarding
	 * it.
	 *
	 * By patching out this behavior, we expose an additional "secret" test.
	 *
	 * DIP5: real time display of "INST_CUNT, MODE_NUM, MODE_CUNT"
	 */
	pROM[0x22296/4] &= 0xffff0000;
	pROM[0x22296/4] |= 0x00004e75;

	namcos22_gametype = NAMCOS22_PROP_CYCLE;
}

DRIVER_INIT( ridgeraj )
{
	namcos22_gametype = NAMCOS22_RIDGE_RACER;
}

DRIVER_INIT( ridger2j )
{
	namcos22_gametype = NAMCOS22_RIDGE_RACER;
}

DRIVER_INIT( acedrvr )
{
	namcos22_gametype = NAMCOS22_ACE_DRIVER;
}

DRIVER_INIT( victlap )
{
	namcos22_gametype = NAMCOS22_VICTORY_LAP;
}

DRIVER_INIT( raveracw )
{
	namcos22_gametype = NAMCOS22_RAVE_RACER;
}

DRIVER_INIT( cybrcomm )
{
	data32_t *pROM = (data32_t *)memory_region(REGION_CPU1);
	pROM[0x18ade8/4] = 0x4e714e71;
	pROM[0x18ae38/4] = 0x4e714e71;
	pROM[0x18ae80/4] = 0x4e714e71;
	pROM[0x18aec8/4] = 0x4e714e71;
	pROM[0x18aefc/4] = 0x4e714e71;

	namcos22_gametype = NAMCOS22_CYBER_COMMANDO;
}

DRIVER_INIT( cybrcyc )
{ /* patch DSP RAM test */
	data32_t *pROM = (data32_t *)memory_region(REGION_CPU1);
	pROM[0x355C/4] &= 0x0000ffff;
	pROM[0x355C/4] |= 0x4e710000;

	namcos22_gametype = NAMCOS22_CYBER_CYCLES;
}

DRIVER_INIT( timecris )
{
	namcos22_gametype = NAMCOS22_TIME_CRISIS;
}

/*     YEAR, NAME,    PARENT,    MACHINE,   INPUT,    INIT,     MNTR,  COMPANY, FULLNAME,                                    FLAGS */
/* System22 games */
GAMEX( 1995, cybrcomm, 0,        namcos22,  cybrcomm, cybrcomm, ROT0, "Namco", "Cyber Commando (Rev. CY1, Japan)"          , GAME_NO_SOUND|GAME_NOT_WORKING ) /* almost */
GAMEX( 1995, raveracw, 0,        namcos22,  raveracw, raveracw, ROT0, "Namco", "Rave Racer (Rev. RV2, World)"              , GAME_NO_SOUND|GAME_NOT_WORKING ) /* almost */
GAMEX( 1993, ridgeraj, 0,        namcos22,  ridgera,  ridgeraj, ROT0, "Namco", "Ridge Racer (Rev. RR1, Japan)"             , GAME_NO_SOUND|GAME_NOT_WORKING ) /* ? */
GAMEX( 1994, ridger2j, 0,        namcos22,  ridgera,  ridger2j, ROT0, "Namco", "Ridge Racer 2 (Rev. RRS1, Japan)"          , GAME_NO_SOUND|GAME_NOT_WORKING ) /* ? */
GAMEX( 1994, acedrvrw, 0,        namcos22,  victlap,  acedrvr,  ROT0, "Namco", "Ace Driver (Rev. AD2, World)"              , GAME_NO_SOUND|GAME_NOT_WORKING ) /* almost */
GAMEX( 1996, victlapw, 0,        namcos22,  victlap,  victlap,  ROT0, "Namco", "Ace Driver - Victory Lap (Rev. ADV2, World)", GAME_NO_SOUND|GAME_NOT_WORKING ) /* almost */

/* Super System22 games */
GAMEX( 1995, airco22b, 0,        namcos22s, victlap,  airco22,  ROT0, "Namco", "Air Combat 22 (Rev. ACS1 Ver.B)"           , GAME_NO_SOUND|GAME_NOT_WORKING ) /* almost */
GAMEX( 1995, alpinerd, 0,        namcos22s, alpiner,  alpiner,  ROT0, "Namco", "Alpine Racer (Rev. AR2 Ver.D)"             , GAME_NO_SOUND|GAME_NOT_WORKING ) /* encrypted gfx */
GAMEX( 1995, alpinerc, alpinerd, namcos22s, alpiner,  alpiner,  ROT0, "Namco", "Alpine Racer (Rev. AR2 Ver.C)"             , GAME_NO_SOUND|GAME_NOT_WORKING ) /* encrypted gfx */
GAMEX( 1995, cybrcycc, 0,        namcos22s, cybrcycc, cybrcyc,  ROT0, "Namco", "Cyber Cycles (Rev. CB2 Ver.C)"             , GAME_NO_SOUND|GAME_NOT_WORKING ) /* almost */
/*GAMEX( 1995, dirtdshx, "Dirt Dash")*/
GAMEX( 1995, timecrsa, 0,        namcos22s, timecris, timecris, ROT0, "Namco", "Time Crisis (Rev. TS2 Ver.A)"              , GAME_NO_SOUND|GAME_NOT_WORKING ) /* locks up */
/*GAMEX( 1995, timecris, 0, namcos22s, timecris, timecris, ROT0, "Namco", "Time Crisis (Rev. TS2 Ver.B)"              , GAME_NO_SOUND|GAME_NOT_WORKING )*/
GAMEX( 1996, propcycl, 0,        namcos22s, propcycl, propcycl, ROT0, "Namco", "Prop Cycle (Rev PR2 Ver.A)"                , GAME_NO_SOUND|GAME_IMPERFECT_GRAPHICS )
/*GAMEX( 1996, tokyowrx, "Tokyo Wars")*/
/*GAMEX( 1996, alpinr2x, "Alpine Racer 2")*/
/*GAMEX( 1996, alpinesx, "Alpine Surfer")*/
/*GAMEX( 1996, aquajetx, "Aqua Jet")*/
/*GAMEX( 1997, armdilox, "Armidillo Racing")*/
/*GAMEX( 199?, downhbkx, "Downhill Bikers")*/
