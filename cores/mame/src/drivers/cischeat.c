/***************************************************************************

							-= Jaleco Driving Games =-

					driver by	Luca Elia (l.elia@tin.it)

- Note: press F2 to enter service mode -

---------------------------------------------------------------------------
Year + Game		Hardware:	Main 	Sub#1	Sub#2	Sound	Sound Chips
---------------------------------------------------------------------------
[89	Big Run]				68000	68000	68000	68000	YM2151 2xM6295

	BOARD #1	BR8950c
	BOARD #2	8951
	BOARD #3	8952A
	BOARD #4	8953

[90	Cisco Heat]				68000	68000	68000	68000	YM2151 2xM6295

	BOARD #1 CH-9072 EB90001-20024
	BOARD #2 CH-9071 EB90001-20023
	BOARD #3 CH-9073 EB90001-20025

[91	F1 GP Star]				68000	68000	68000	68000	YM2151 2xM6295

	TB - Top board     (audio & I/O)       GP-9190A EB90015-20039-1
	MB - Middle board  (GFX)               GP-9189  EB90015-20038
	LB - Lower board   (CPU/GFX)           GP-9188A EB90015-20037-1

	Chips:

		GS90015-02 (100 pin PQFP)	x 3		[Tilemaps]
		GS-9000406 (80 pin PQFP)	x 3

		GS900151   (44 pin PQFP) (too small for the full part No.)
		GS90015-03 (80 pin PQFP)	x 3  + 2x LH52258D-45 (32kx8 SRAM)
		GS90015-06 (100 pin PQFP)	x 2  + 2x LH52250AD-90L (32kx8 SRAM)
		GS90015-07 (64 pin PQFP)
		GS90015-08 (64 pin PQFP)
		GS90015-09 (64 pin PQFP)  + 2x MCM2018AN45 (2kx8 SRAM)
		GS90015-10 (64 pin PQFP)
		GS90015-12 (80 pin PQFP)  + 2x MCM2018AN45 (2kx8 SRAM)
		GS90015-11 (100 pin PQFP)

		CS90015-04 x 2 (64 pin PQFP)		[Road]
		GS90015-05 x 2 (100 pin PQFP)

[94	Scud Hammer]			68000	-		-		-		2xM6295

	Board CF-92128B Chips:
		GS9001501
		GS90015-02 x 2	GS600406   x 2		[Tilemaps]
		GS90015-03

	Board GP-9189 Chips:
		GS90015-03 x 2						[Sprites]
		GS90015-06 x 2
		GS90015-07
		GS90015-08
		GS90015-09
		GS90015-10
		GS90015-11
		MR90015-35 x 2
---------------------------------------------------------------------------

----------------------------------------------------------------------------------
Main CPU					[Cisco Heat]		[F1 GP Star]		[Scud Hammer]
----------------------------------------------------------------------------------
ROM					R		000000-07ffff		<					<
							100000-17ffff		<					RW : I / O + Sound
Work RAM + Sprites	RW		0f0000-0fffff		<					<
Hardware Regs		RW		080000-087fff		<					<
Units Linking RAM	RW		088000-88ffff		<					-
Shared RAM #2		RW		090000-097fff		<					-
Shared RAM #1		RW		098000-09ffff		<					-
Scroll RAM 0		RW		0a0000-0a7fff		<					<
Scroll RAM 1		RW		0a8000-0affff		<					-
Scroll RAM 2		RW		0b0000-0b7fff		<					<
Palette RAM			RW		0b8000-0bffff		<					<
	Palette Scroll 0		0b9c00-0b9fff		0b9e00-0b9fff		<
	Palette Scroll 1		0bac00-0bafff		0bae00-0bafff		-
	Palette Road 0			0bb800-0bbfff		<					-
	Palette Road 1			0bc800-0bcfff		<					-
	Palette Sprites			0bd000-0bdfff		<					0bb000-0bbfff
	Palette Scroll 2		0bec00-0befff0		0bee00-0befff		0bce00-0bcfff
----------------------------------------------------------------------------------

----------------------------------------------------------------
Sub CPU's					[Cisco Heat]		[F1 GP Star]
----------------------------------------------------------------
ROM					R		000000-03ffff		<
							200000-23ffff		-
Work RAM			RW		0c0000-0c3fff		180000-183fff
Shared RAM			RW		040000-047fff		080000-087fff
Road RAM			RW		080000-0807ff		100000-1007ff
Whatchdog					100000-100001		200000-200001
----------------------------------------------------------------

----------------------------------------------------------------
Sound CPU					[Cisco Heat]		[F1 GP Star]
----------------------------------------------------------------
ROM					R		000000-03ffff		<
Work RAM			RW		0f0000-0fffff		0e0000-0fffff
M6295 #1 Banking	 W		040002-040003		040004-040005
M6295 #2 Banking	 W		040004-040005		040008-040009
Sound Latch			 W		060002-060003		060000-060001
Sound Latch			R		060004-060005		060000-060001
YM2151 Reg Sel		 W		080001-080001		<
YM2151 Data			 W		080003-080003		<
YM2151 Status		R		080003-080003		<
M6295 #1 Status		R		0a0001-0a0001		<
M6295 #1 Data		 W		0a0000-0a0003		<
M6295 #2 Status		R		0c0001-0c0001		<
M6295 #2 Data		 W		0c0000-0c0003		<
----------------------------------------------------------------

Cheats:

[cischeat]
-	f011a.w		*** stage - 1 ***
-	f0190.l		*** score / 10 (BCD) ***
-	f0280.w		*** time * 10 (seconds) ***
-	f61Xa.w		car X data

[f1gpstar]
-	Note: This game has some leftover code from Cisco Heat, it seems.
-	f9088.w		*** lap - 1 ***
-	fa008.w		($fa000 + $08) *** time (seconds) ***
-	fa2aa.l		($fa200 + $aa) speed << 16

Common Issues:

- Some ROMs aren't used (priorities?)
- Screen control register (priorities, layers enabling etc.) - Where is it?
- In cischeat & bigrun, at the start of some levels, you can see the empty
  scrolling layers as they are filled. In f1gpstar, I'm unsure whether they
  are correct in a few places (e.g. in the attract mode, where cars move
  horizontally, the wheels don't follow for this reason, I think
- Sound communication not quite right: see Test Mode

To Do:

- Priorities!
- Use the Tilemap Manager for the road layers (when this kind of layers
  will be supported) for perfomance and better priority support.
  A line based zooming is additionally needed for f1gpstar.
- Force feedback :)

***************************************************************************/

#include "driver.h"
#include "megasys1.h"

/* Variables only used here: */

static data16_t *rom_1, *rom_2, *rom_3;
static data16_t *sharedram1, *sharedram2;

/* Variables defined in vidhrdw: */

extern data16_t *cischeat_roadram[2];
extern data16_t *f1gpstr2_ioready;

/* Functions defined in vidhrdw: */

READ16_HANDLER( bigrun_vregs_r );
READ16_HANDLER( cischeat_vregs_r );
READ16_HANDLER( f1gpstar_vregs_r );
READ16_HANDLER( f1gpstr2_vregs_r );

WRITE16_HANDLER( bigrun_vregs_w );
WRITE16_HANDLER( cischeat_vregs_w );
WRITE16_HANDLER( f1gpstar_vregs_w );
WRITE16_HANDLER( f1gpstr2_vregs_w );
WRITE16_HANDLER( scudhamm_vregs_w );

VIDEO_START( bigrun );
VIDEO_START( cischeat );
VIDEO_START( f1gpstar );

VIDEO_UPDATE( bigrun );
VIDEO_UPDATE( cischeat );
VIDEO_UPDATE( f1gpstar );
VIDEO_UPDATE( scudhamm );


/**************************************************************************


						Memory Maps - Main CPU (#1)


**************************************************************************/

READ16_HANDLER( sharedram1_r )  {return sharedram1[offset];}
READ16_HANDLER( sharedram2_r )  {return sharedram2[offset];}

WRITE16_HANDLER( sharedram1_w ) {COMBINE_DATA(&sharedram1[offset]);}
WRITE16_HANDLER( sharedram2_w ) {COMBINE_DATA(&sharedram2[offset]);}

static READ16_HANDLER( rom_1_r ) {return rom_1[offset];}
static READ16_HANDLER( rom_2_r ) {return rom_2[offset];}
static READ16_HANDLER( rom_3_r ) {return rom_3[offset];}


/**************************************************************************
								Big Run
**************************************************************************/

WRITE16_HANDLER( bigrun_paletteram16_w )
{
	data16_t word = COMBINE_DATA(&paletteram16[offset]);
	int r = ((word >> 8) & 0xF0 ) | ((word << 0) & 0x08);
	int g = ((word >> 4) & 0xF0 ) | ((word << 1) & 0x08);
	int b = ((word >> 0) & 0xF0 ) | ((word << 2) & 0x08);

	/* Scroll 0*/
	if ( (offset >= 0x0e00/2) && (offset <= 0x0fff/2) ) { palette_set_color(0x000 + offset - 0x0e00/2, r,g,b ); return;}
	/* Scroll 1*/
	if ( (offset >= 0x1600/2) && (offset <= 0x17ff/2) ) { palette_set_color(0x100 + offset - 0x1600/2, r,g,b ); return;}
	/* Road 0*/
	if ( (offset >= 0x1800/2) && (offset <= 0x1fff/2) ) { palette_set_color(0x200 + offset - 0x1800/2, r,g,b ); return;}
	/* Road 1*/
	if ( (offset >= 0x2000/2) && (offset <= 0x27ff/2) ) { palette_set_color(0x600 + offset - 0x2000/2, r,g,b ); return;}
	/* Sprites*/
	if ( (offset >= 0x2800/2) && (offset <= 0x2fff/2) ) { palette_set_color(0xa00 + offset - 0x2800/2, r,g,b ); return;}
	/* Scroll 2*/
	if ( (offset >= 0x3600/2) && (offset <= 0x37ff/2) ) { palette_set_color(0xe00 + offset - 0x3600/2, r,g,b ); return;}
}

static MEMORY_READ16_START( bigrun_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM			},	/* ROM*/
	{ 0x100000, 0x13ffff, rom_1_r			},	/* ROM*/
	{ 0x0f0000, 0x0fffff, MRA16_RAM			},	/* RAM*/
	{ 0x080000, 0x083fff, bigrun_vregs_r	},	/* Vregs*/
	{ 0x084000, 0x087fff, MRA16_RAM			},	/* Linking with other units*/

	/* this is the right order of sharedram's */
	{ 0x088000, 0x08bfff, sharedram2_r		},	/* Sharedram with sub CPU#2*/
	{ 0x08c000, 0x08ffff, sharedram1_r		},	/* Sharedram with sub CPU#1*/

	{ 0x090000, 0x093fff, MRA16_RAM			},	/* Scroll ram 0*/
	{ 0x094000, 0x097fff, MRA16_RAM			},	/* Scroll ram 1*/
	{ 0x098000, 0x09bfff, MRA16_RAM			},	/* Scroll ram 2*/

	{ 0x09c000, 0x09ffff, MRA16_RAM			},	/* Palettes*/
MEMORY_END

static MEMORY_WRITE16_START( bigrun_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM							},	/* ROM*/
	{ 0x100000, 0x13ffff, MWA16_ROM							},	/* ROM*/
	{ 0x0f0000, 0x0fffff, MWA16_RAM, &megasys1_ram			},	/* RAM*/
	{ 0x080000, 0x083fff, bigrun_vregs_w, &megasys1_vregs	},	/* Vregs*/
	{ 0x084000, 0x087fff, MWA16_RAM							},	/* Linking with other units*/

	{ 0x088000, 0x08bfff, sharedram2_w, &sharedram2			},	/* Sharedram with sub CPU#2*/
	{ 0x08c000, 0x08ffff, sharedram1_w, &sharedram1			},	/* Sharedram with sub CPU#1*/

	/* Only writes to the first 0x40000 bytes affect the tilemaps:             */
	/* either these games support larger tilemaps or have more ram than needed */
	{ 0x090000, 0x093fff, megasys1_scrollram_0_w, &megasys1_scrollram_0	},	/* Scroll ram 0*/
	{ 0x094000, 0x097fff, megasys1_scrollram_1_w, &megasys1_scrollram_1	},	/* Scroll ram 1*/
	{ 0x098000, 0x09bfff, megasys1_scrollram_2_w, &megasys1_scrollram_2	},	/* Scroll ram 2*/

	{ 0x09c000, 0x09ffff, bigrun_paletteram16_w, &paletteram16	},	/* Palettes*/
MEMORY_END


/**************************************************************************
								Cisco Heat
**************************************************************************/

/*	CISCO HEAT
	[  Test  ]		[  Real  ]
	b9c00-b9fff		<				scroll 0
	bac00-bafff		<				scroll 1
	bb800-bbbff		bb800-bbfff		road 0
	bc800-bcbff		bc800-bcfff		road 1
	bd000-bd3ff		bd000-bdfff		sprites
	bec00-befff		<				text		*/

WRITE16_HANDLER( cischeat_paletteram16_w )
{
	data16_t word = COMBINE_DATA(&paletteram16[offset]);
	int r = ((word >> 8) & 0xF0 ) | ((word << 0) & 0x08);
	int g = ((word >> 4) & 0xF0 ) | ((word << 1) & 0x08);
	int b = ((word >> 0) & 0xF0 ) | ((word << 2) & 0x08);

	/* Scroll 0*/
	if ( (offset >= 0x1c00/2) && (offset <= 0x1fff/2) ) { palette_set_color(0x000 + offset - 0x1c00/2, r,g,b ); return;}
	/* Scroll 1*/
	if ( (offset >= 0x2c00/2) && (offset <= 0x2fff/2) ) { palette_set_color(0x200 + offset - 0x2c00/2, r,g,b ); return;}
	/* Scroll 2*/
	if ( (offset >= 0x6c00/2) && (offset <= 0x6fff/2) ) { palette_set_color(0x400 + offset - 0x6c00/2, r,g,b ); return;}
	/* Road 0*/
	if ( (offset >= 0x3800/2) && (offset <= 0x3fff/2) ) { palette_set_color(0x600 + offset - 0x3800/2, r,g,b ); return;}
	/* Road 1*/
	if ( (offset >= 0x4800/2) && (offset <= 0x4fff/2) ) { palette_set_color(0xa00 + offset - 0x4800/2, r,g,b ); return;}
	/* Sprites*/
	if ( (offset >= 0x5000/2) && (offset <= 0x5fff/2) ) { palette_set_color(0xe00 + offset - 0x5000/2, r,g,b ); return;}
}

static MEMORY_READ16_START( cischeat_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM			},	/* ROM*/
	{ 0x100000, 0x17ffff, rom_1_r			},	/* ROM*/
	{ 0x0f0000, 0x0fffff, MRA16_RAM			},	/* RAM*/
	{ 0x080000, 0x087fff, cischeat_vregs_r	},	/* Vregs*/
	{ 0x088000, 0x088fff, MRA16_RAM			},	/* Linking with other units*/

/* 	Only the first 0x800 bytes are tested but:
	CPU #0 PC 0000278c: warning - write 68c0 to unmapped memory address 0009c7fe
	CPU #0 PC 0000dd58: warning - read unmapped memory address 000945ac
	No mem access error from the other CPU's, though.. */

	/* this is the right order of sharedram's */
	{ 0x090000, 0x097fff, sharedram2_r		},	/* Sharedram with sub CPU#2*/
	{ 0x098000, 0x09ffff, sharedram1_r		},	/* Sharedram with sub CPU#1*/

	{ 0x0a0000, 0x0a7fff, MRA16_RAM			},	/* Scroll ram 0*/
	{ 0x0a8000, 0x0affff, MRA16_RAM			},	/* Scroll ram 1*/
	{ 0x0b0000, 0x0b7fff, MRA16_RAM			},	/* Scroll ram 2*/

	{ 0x0b8000, 0x0bffff, MRA16_RAM			},	/* Palettes*/
MEMORY_END

static MEMORY_WRITE16_START( cischeat_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM							},	/* ROM*/
	{ 0x100000, 0x17ffff, MWA16_ROM							},	/* ROM*/
	{ 0x0f0000, 0x0fffff, MWA16_RAM, &megasys1_ram			},	/* RAM*/
	{ 0x080000, 0x087fff, cischeat_vregs_w, &megasys1_vregs	},	/* Vregs*/
	{ 0x088000, 0x088fff, MWA16_RAM							},	/* Linking with other units*/

	{ 0x090000, 0x097fff, sharedram2_w, &sharedram2			},	/* Sharedram with sub CPU#2*/
	{ 0x098000, 0x09ffff, sharedram1_w, &sharedram1			},	/* Sharedram with sub CPU#1*/

	/* Only writes to the first 0x40000 bytes affect the tilemaps:             */
	/* either these games support larger tilemaps or have more ram than needed */
	{ 0x0a0000, 0x0a7fff, megasys1_scrollram_0_w, &megasys1_scrollram_0	},	/* Scroll ram 0*/
	{ 0x0a8000, 0x0affff, megasys1_scrollram_1_w, &megasys1_scrollram_1	},	/* Scroll ram 1*/
	{ 0x0b0000, 0x0b7fff, megasys1_scrollram_2_w, &megasys1_scrollram_2	},	/* Scroll ram 2*/

	{ 0x0b8000, 0x0bffff, cischeat_paletteram16_w, &paletteram16	},	/* Palettes*/
MEMORY_END


/**************************************************************************
							F1 GrandPrix Star
**************************************************************************/

WRITE16_HANDLER( f1gpstar_paletteram16_w )
{
	data16_t word = COMBINE_DATA(&paletteram16[offset]);
	int r = ((word >> 8) & 0xF0 ) | ((word << 0) & 0x08);
	int g = ((word >> 4) & 0xF0 ) | ((word << 1) & 0x08);
	int b = ((word >> 0) & 0xF0 ) | ((word << 2) & 0x08);

	/* Scroll 0*/
	if ( (offset >= 0x1e00/2) && (offset <= 0x1fff/2) ) { palette_set_color(0x000 + offset - 0x1e00/2, r,g,b ); return;}
	/* Scroll 1*/
	if ( (offset >= 0x2e00/2) && (offset <= 0x2fff/2) ) { palette_set_color(0x100 + offset - 0x2e00/2, r,g,b ); return;}
	/* Scroll 2*/
	if ( (offset >= 0x6e00/2) && (offset <= 0x6fff/2) ) { palette_set_color(0x200 + offset - 0x6e00/2, r,g,b ); return;}
	/* Road 0*/
	if ( (offset >= 0x3800/2) && (offset <= 0x3fff/2) ) { palette_set_color(0x300 + offset - 0x3800/2, r,g,b ); return;}
	/* Road 1*/
	if ( (offset >= 0x4800/2) && (offset <= 0x4fff/2) ) { palette_set_color(0x700 + offset - 0x4800/2, r,g,b ); return;}
	/* Sprites*/
	if ( (offset >= 0x5000/2) && (offset <= 0x5fff/2) ) { palette_set_color(0xb00 + offset - 0x5000/2, r,g,b ); return;}
}

/*	F1 GP Star tests:
	0A0000-0B8000
	0F0000-100000
	0B8000-0C0000
	090800-091000
	098800-099000
	0F8000-0F9000	*/

static MEMORY_READ16_START( f1gpstar_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM			},	/* ROM*/
	{ 0x100000, 0x17ffff, rom_1_r			},	/* ROM*/
	{ 0x0f0000, 0x0fffff, MRA16_RAM			},	/* RAM*/
	{ 0x080000, 0x087fff, f1gpstar_vregs_r	},	/* Vregs*/
	{ 0x088000, 0x088fff, MRA16_RAM			},	/* Linking with other units*/

	{ 0x090000, 0x097fff, sharedram2_r		},	/* Sharedram with sub CPU#2*/
	{ 0x098000, 0x09ffff, sharedram1_r		},	/* Sharedram with sub CPU#1*/

	{ 0x0a0000, 0x0a7fff, MRA16_RAM			},	/* Scroll ram 0*/
	{ 0x0a8000, 0x0affff, MRA16_RAM			},	/* Scroll ram 1*/
	{ 0x0b0000, 0x0b7fff, MRA16_RAM			},	/* Scroll ram 2*/

	{ 0x0b8000, 0x0bffff, MRA16_RAM			},	/* Palettes*/
MEMORY_END

static MEMORY_WRITE16_START( f1gpstar_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM							},
	{ 0x100000, 0x17ffff, MWA16_ROM							},
	{ 0x0f0000, 0x0fffff, MWA16_RAM, &megasys1_ram			},	/* RAM*/
	{ 0x080000, 0x087fff, f1gpstar_vregs_w, &megasys1_vregs	},	/* Vregs*/
	{ 0x088000, 0x088fff, MWA16_RAM							},	/* Linking with other units*/

	{ 0x090000, 0x097fff, sharedram2_w, &sharedram2			},	/* Sharedram with sub CPU#2*/
	{ 0x098000, 0x09ffff, sharedram1_w, &sharedram1			},	/* Sharedram with sub CPU#1*/

	/* Only writes to the first 0x40000 bytes affect the tilemaps:             */
	/* either these games support larger tilemaps or have more ram than needed */
	{ 0x0a0000, 0x0a7fff, megasys1_scrollram_0_w, &megasys1_scrollram_0	},	/* Scroll ram 0*/
	{ 0x0a8000, 0x0affff, megasys1_scrollram_1_w, &megasys1_scrollram_1	},	/* Scroll ram 1*/
	{ 0x0b0000, 0x0b7fff, megasys1_scrollram_2_w, &megasys1_scrollram_2	},	/* Scroll ram 2*/

	{ 0x0b8000, 0x0bffff, f1gpstar_paletteram16_w, &paletteram16	},	/* Palettes*/
MEMORY_END


/**************************************************************************
							F1 GrandPrix Star II
**************************************************************************/

/* Same as f1gpstar, but vregs are slightly different:*/
static MEMORY_READ16_START( f1gpstr2_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM			},	/* ROM*/
	{ 0x100000, 0x17ffff, rom_1_r			},	/* ROM*/
	{ 0x0f0000, 0x0fffff, MRA16_RAM			},	/* RAM*/
	{ 0x080000, 0x087fff, f1gpstr2_vregs_r	},	/* Vregs (slightly different from f1gpstar)*/
	{ 0x088000, 0x088fff, MRA16_RAM			},	/* Linking with other units*/

	{ 0x090000, 0x097fff, sharedram2_r		},	/* Sharedram with sub CPU#2*/
	{ 0x098000, 0x09ffff, sharedram1_r		},	/* Sharedram with sub CPU#1*/

	{ 0x0a0000, 0x0a7fff, MRA16_RAM			},	/* Scroll ram 0*/
	{ 0x0a8000, 0x0affff, MRA16_RAM			},	/* Scroll ram 1*/
	{ 0x0b0000, 0x0b7fff, MRA16_RAM			},	/* Scroll ram 2*/

	{ 0x0b8000, 0x0bffff, MRA16_RAM			},	/* Palettes*/
MEMORY_END

static MEMORY_WRITE16_START( f1gpstr2_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM							},
	{ 0x100000, 0x17ffff, MWA16_ROM							},
	{ 0x0f0000, 0x0fffff, MWA16_RAM, &megasys1_ram			},	/* RAM*/
	{ 0x080000, 0x087fff, f1gpstr2_vregs_w, &megasys1_vregs	},	/* Vregs  (slightly different from f1gpstar)*/
	{ 0x088000, 0x088fff, MWA16_RAM							},	/* Linking with other units*/

	{ 0x090000, 0x097fff, sharedram2_w, &sharedram2			},	/* Sharedram with sub CPU#2*/
	{ 0x098000, 0x09ffff, sharedram1_w, &sharedram1			},	/* Sharedram with sub CPU#1*/

	/* Only writes to the first 0x40000 bytes affect the tilemaps:             */
	/* either these games support larger tilemaps or have more ram than needed */
	{ 0x0a0000, 0x0a7fff, megasys1_scrollram_0_w, &megasys1_scrollram_0	},	/* Scroll ram 0*/
	{ 0x0a8000, 0x0affff, megasys1_scrollram_1_w, &megasys1_scrollram_1	},	/* Scroll ram 1*/
	{ 0x0b0000, 0x0b7fff, megasys1_scrollram_2_w, &megasys1_scrollram_2	},	/* Scroll ram 2*/

	{ 0x0b8000, 0x0bffff, f1gpstar_paletteram16_w, &paletteram16	},	/* Palettes*/
MEMORY_END


/**************************************************************************
							Scud Hammer
**************************************************************************/

WRITE16_HANDLER( scudhamm_paletteram16_w )
{
	int newword = COMBINE_DATA(&paletteram16[offset]);

	int r = ((newword >> 8) & 0xF0 ) | ((newword << 0) & 0x08);
	int g = ((newword >> 4) & 0xF0 ) | ((newword << 1) & 0x08);
	int b = ((newword >> 0) & 0xF0 ) | ((newword << 2) & 0x08);

	/* Scroll 0*/
	if ( (offset >= 0x1e00/2) && (offset <= 0x1fff/2) ) { palette_set_color(0x000 + offset - 0x1e00/2, r,g,b ); return;}
	/* Scroll 2*/
	if ( (offset >= 0x4e00/2) && (offset <= 0x4fff/2) ) { palette_set_color(0x100 + offset - 0x4e00/2, r,g,b ); return;}
	/* Sprites*/
	if ( (offset >= 0x3000/2) && (offset <= 0x3fff/2) ) { palette_set_color(0x200 + offset - 0x3000/2, r,g,b ); return;}
}


data16_t scudhamm_motor_command;

/*	Motor Status.

	f--- ---- ---- ----		Rotation Limit (R?)
	-e-- ---- ---- ----		Rotation Limit (L?)
	--dc ba98 7654 32--
	---- ---- ---- --1-		Up Limit
	---- ---- ---- ---0		Down Limit	*/

READ16_HANDLER( scudhamm_motor_status_r )
{
/*	return 1 << (rand()&1);			*/ /* Motor Status*/
	return scudhamm_motor_command;	/* Motor Status*/
}


READ16_HANDLER( scudhamm_motor_pos_r )
{
	return 0x00 << 8;
}


/*	Move the motor.

	fedc ba98 7654 32--
	---- ---- ---- --1-		Move Up
	---- ---- ---- ---0		Move Down

	Within $20 vblanks the motor must reach the target.	*/

WRITE16_HANDLER( scudhamm_motor_command_w )
{
	COMBINE_DATA( &scudhamm_motor_command );
}


READ16_HANDLER( scudhamm_analog_r )
{
	return readinputport(1);
}


/*
	I don't know how many leds are there, but each bit in the buttons input
	port (coins, tilt, buttons, select etc.) triggers the corresponding bit
	in this word. I mapped the 3 buttons to the first 3 led.
*/
WRITE16_HANDLER( scudhamm_leds_w )
{
	if (ACCESSING_MSB)
	{
 		set_led_status(0, data & 0x0100);	/* 3 buttons*/
		set_led_status(1, data & 0x0200);
		set_led_status(2, data & 0x0400);
	}

	if (ACCESSING_LSB)
	{
/*		set_led_status(3, data & 0x0010);	*/ /* if we had more leds..*/
/*		set_led_status(4, data & 0x0020);*/
	}
}


/*
	$FFFC during self test, $FFFF onwards.
	It could be audio(L/R) or layers(0/2) enable.
*/
WRITE16_HANDLER( scudhamm_enable_w )
{
}


WRITE16_HANDLER( scudhamm_oki_bank_w )
{
	if (ACCESSING_LSB)
	{
		OKIM6295_set_bank_base(0, 0x40000 * ((data >> 0) & 0x3) );
		OKIM6295_set_bank_base(1, 0x40000 * ((data >> 4) & 0x3) );
	}
}

static MEMORY_READ16_START( readmem_scudhamm )
	{ 0x000000, 0x07ffff, MRA16_ROM					},	/* ROM*/
	{ 0x0f0000, 0x0fffff, MRA16_RAM					},	/* Work RAM + Spriteram*/
	{ 0x082000, 0x082fff, MRA16_RAM					},	/* Video Registers + RAM*/
	{ 0x0a0000, 0x0a3fff, MRA16_RAM					},	/* Scroll RAM 0*/
	{ 0x0b0000, 0x0b3fff, MRA16_RAM					},	/* Scroll RAM 2*/
	{ 0x0b8000, 0x0bffff, MRA16_RAM					},	/* Palette*/
	{ 0x100008, 0x100009, input_port_0_word_r		},	/* Buttons*/
	{ 0x100014, 0x100015, OKIM6295_status_0_lsb_r	},	/* Sound*/
	{ 0x100018, 0x100019, OKIM6295_status_1_lsb_r	},	/**/
	{ 0x100040, 0x100041, scudhamm_analog_r			},	/* A / D*/
	{ 0x100044, 0x100045, scudhamm_motor_pos_r		},	/* Motor Position*/
	{ 0x100050, 0x100051, scudhamm_motor_status_r	},	/* Motor Limit Switches*/
	{ 0x10005c, 0x10005d, input_port_2_word_r		},	/* 2 x DSW*/
MEMORY_END

static MEMORY_WRITE16_START( writemem_scudhamm )
 	{ 0x000000, 0x07ffff, MWA16_ROM					},	/* ROM*/
	{ 0x0f0000, 0x0fffff, MWA16_RAM,				&megasys1_ram			},	/* Work RAM + Spriteram*/
	{ 0x082000, 0x082fff, scudhamm_vregs_w,			&megasys1_vregs			},	/* Video Registers + RAM*/
	{ 0x0a0000, 0x0a3fff, megasys1_scrollram_0_w,	&megasys1_scrollram_0	},	/* Scroll RAM 0*/
	{ 0x0b0000, 0x0b3fff, megasys1_scrollram_2_w,	&megasys1_scrollram_2	},	/* Scroll RAM 2*/
	{ 0x0b8000, 0x0bffff, scudhamm_paletteram16_w,	&paletteram16			},	/* Palette*/
 	{ 0x100000, 0x100001, scudhamm_oki_bank_w		},	/* Sound*/
 	{ 0x100008, 0x100009, scudhamm_leds_w			},	/* Leds*/
	{ 0x100014, 0x100015, OKIM6295_data_0_lsb_w		},	/* Sound*/
	{ 0x100018, 0x100019, OKIM6295_data_1_lsb_w		},	/**/
	{ 0x10001c, 0x10001d, scudhamm_enable_w			},	/* ?*/
	{ 0x100040, 0x100041, MWA16_NOP					},	/* ? 0 written before reading*/
	{ 0x100050, 0x100051, scudhamm_motor_command_w	},	/* Move Motor*/
MEMORY_END


/**************************************************************************


					Memory Maps - Road CPUs (#2 & #3)


**************************************************************************/

/**************************************************************************
								Big Run
**************************************************************************/

static MEMORY_READ16_START( bigrun_readmem2 )
	{ 0x000000, 0x03ffff, MRA16_ROM		},	/* ROM*/
	{ 0x0c0000, 0x0c3fff, MRA16_RAM		},	/* RAM*/
	{ 0x040000, 0x047fff, sharedram1_r	},	/* Shared RAM (with Main CPU)*/
	{ 0x080000, 0x0807ff, MRA16_RAM		},	/* Road RAM*/
MEMORY_END
static MEMORY_WRITE16_START( bigrun_writemem2 )
	{ 0x000000, 0x03ffff, MWA16_ROM							},	/* ROM*/
	{ 0x0c0000, 0x0c3fff, MWA16_RAM							},	/* RAM*/
	/* 800 bytes tested: */
	{ 0x040000, 0x047fff, sharedram1_w						},	/* Shared RAM (with Main CPU)*/
	{ 0x080000, 0x0807ff, MWA16_RAM, &cischeat_roadram[0]	},	/* Road RAM*/
MEMORY_END


static MEMORY_READ16_START( bigrun_readmem3 )
	{ 0x000000, 0x03ffff, MRA16_ROM		},	/* ROM*/
	{ 0x0c0000, 0x0c3fff, MRA16_RAM		},	/* RAM*/
	{ 0x040000, 0x047fff, sharedram2_r	},	/* Shared RAM (with Main CPU)*/
	{ 0x080000, 0x0807ff, MRA16_RAM		},	/* Road RAM*/
MEMORY_END
static MEMORY_WRITE16_START( bigrun_writemem3 )
	{ 0x000000, 0x03ffff, MWA16_ROM							},	/* ROM*/
	{ 0x0c0000, 0x0c3fff, MWA16_RAM							},	/* RAM*/
	{ 0x040000, 0x047fff, sharedram2_w						},	/* Shared RAM (with Main CPU)*/
	{ 0x080000, 0x0807ff, MWA16_RAM, &cischeat_roadram[1]	},	/* Road RAM*/
MEMORY_END


/**************************************************************************
								Cisco Heat
**************************************************************************/

static MEMORY_READ16_START( cischeat_readmem2 )
	{ 0x000000, 0x03ffff, MRA16_ROM		},	/* ROM*/
	{ 0x200000, 0x23ffff, rom_2_r		},	/* ROM*/
	{ 0x0c0000, 0x0c3fff, MRA16_RAM		},	/* RAM*/
	{ 0x040000, 0x047fff, sharedram1_r	},	/* Shared RAM (with Main CPU)*/
	{ 0x080000, 0x0807ff, MRA16_RAM		},	/* Road RAM*/
MEMORY_END
static MEMORY_WRITE16_START( cischeat_writemem2 )
	{ 0x000000, 0x03ffff, MWA16_ROM							},	/* ROM*/
	{ 0x200000, 0x23ffff, MWA16_ROM							},	/* ROM*/
	{ 0x0c0000, 0x0c3fff, MWA16_RAM							},	/* RAM*/
	{ 0x040000, 0x047fff, sharedram1_w						},	/* Shared RAM (with Main CPU)*/
	{ 0x080000, 0x0807ff, MWA16_RAM, &cischeat_roadram[0]	},	/* Road RAM*/
	{ 0x100000, 0x100001, MWA16_NOP							},	/* watchdog*/
MEMORY_END

static MEMORY_READ16_START( cischeat_readmem3 )
	{ 0x000000, 0x03ffff, MRA16_ROM		},	/* ROM*/
	{ 0x200000, 0x23ffff, rom_3_r		},	/* ROM*/
	{ 0x0c0000, 0x0c3fff, MRA16_RAM		},	/* RAM*/
	{ 0x040000, 0x047fff, sharedram2_r	},	/* Shared RAM (with Main CPU)*/
	{ 0x080000, 0x0807ff, MRA16_RAM		},	/* Road RAM*/
MEMORY_END
static MEMORY_WRITE16_START( cischeat_writemem3 )
	{ 0x000000, 0x03ffff, MWA16_ROM							},	/* ROM*/
	{ 0x200000, 0x23ffff, MWA16_ROM							},	/* ROM*/
	{ 0x0c0000, 0x0c3fff, MWA16_RAM							},	/* RAM*/
	{ 0x040000, 0x047fff, sharedram2_w						},	/* Shared RAM (with Main CPU)*/
	{ 0x080000, 0x0807ff, MWA16_RAM, &cischeat_roadram[1]	},	/* Road RAM*/
	{ 0x100000, 0x100001, MWA16_NOP							},	/* watchdog*/
MEMORY_END



/**************************************************************************
							F1 GrandPrix Star
**************************************************************************/

static MEMORY_READ16_START( f1gpstar_readmem2 )
	{ 0x000000, 0x03ffff, MRA16_ROM		},	/* ROM*/
	{ 0x180000, 0x183fff, MRA16_RAM		},	/* RAM*/
	{ 0x080000, 0x0807ff, sharedram1_r	},	/* Shared RAM (with Main CPU)*/
	{ 0x100000, 0x1007ff, MRA16_RAM		},	/* Road RAM*/
MEMORY_END
static MEMORY_WRITE16_START( f1gpstar_writemem2 )
	{ 0x000000, 0x03ffff, MWA16_ROM							},	/* ROM*/
	{ 0x180000, 0x183fff, MWA16_RAM							},	/* RAM*/
	{ 0x080000, 0x0807ff, sharedram1_w						},	/* Shared RAM (with Main CPU)*/
	{ 0x100000, 0x1007ff, MWA16_RAM, &cischeat_roadram[0]	},	/* Road RAM*/
	{ 0x200000, 0x200001, MWA16_NOP							},	/* watchdog*/
MEMORY_END

static MEMORY_READ16_START( f1gpstar_readmem3 )
	{ 0x000000, 0x03ffff, MRA16_ROM		},	/* ROM*/
	{ 0x180000, 0x183fff, MRA16_RAM		},	/* RAM*/
	{ 0x080000, 0x0807ff, sharedram2_r	},	/* Shared RAM (with Main CPU)*/
	{ 0x100000, 0x1007ff, MRA16_RAM		},	/* Road RAM*/
MEMORY_END
static MEMORY_WRITE16_START( f1gpstar_writemem3 )
	{ 0x000000, 0x03ffff, MWA16_ROM							},	/* ROM*/
	{ 0x180000, 0x183fff, MWA16_RAM							},	/* RAM*/
	{ 0x080000, 0x0807ff, sharedram2_w						},	/* Shared RAM (with Main CPU)*/
	{ 0x100000, 0x1007ff, MWA16_RAM, &cischeat_roadram[1]	},	/* Road RAM*/
	{ 0x200000, 0x200001, MWA16_NOP							},	/* watchdog*/
MEMORY_END


/**************************************************************************


						Memory Maps - Sound CPU (#4)


**************************************************************************/

/* Music tempo driven by the YM2151 timers (status reg polled) */


/**************************************************************************
								Big Run
**************************************************************************/

WRITE16_HANDLER( bigrun_soundbank_w )
{
	if (ACCESSING_LSB)
	{
		OKIM6295_set_bank_base(0, 0x40000 * ((data >> 0) & 1) );
		OKIM6295_set_bank_base(1, 0x40000 * ((data >> 4) & 1) );
	}
}

static MEMORY_READ16_START( bigrun_sound_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM						},	/* ROM*/
	{ 0x0f0000, 0x0fffff, MRA16_RAM						},	/* RAM*/
	{ 0x040000, 0x040001, soundlatch_word_r				},	/* From Main CPU*/
	{ 0x080002, 0x080003, YM2151_status_port_0_lsb_r	},
	{ 0x0a0000, 0x0a0001, OKIM6295_status_0_lsb_r		},
	{ 0x0c0000, 0x0c0001, OKIM6295_status_1_lsb_r		},
MEMORY_END

static MEMORY_WRITE16_START( bigrun_sound_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM						},	/* ROM*/
	{ 0x0f0000, 0x0fffff, MWA16_RAM						},	/* RAM*/
	{ 0x040000, 0x040001, bigrun_soundbank_w			},	/* Sample Banking*/
	{ 0x060000, 0x060001, soundlatch2_word_w			},	/* To Main CPU*/
	{ 0x080000, 0x080001, YM2151_register_port_0_lsb_w	},
	{ 0x080002, 0x080003, YM2151_data_port_0_lsb_w		},
	{ 0x0a0000, 0x0a0003, OKIM6295_data_0_lsb_w			},
	{ 0x0c0000, 0x0c0003, OKIM6295_data_1_lsb_w			},
MEMORY_END


/**************************************************************************
								Cisco Heat
**************************************************************************/

WRITE16_HANDLER( cischeat_soundbank_0_w )
{
	if (ACCESSING_LSB)	OKIM6295_set_bank_base(0, 0x40000 * (data & 1) );
}
WRITE16_HANDLER( cischeat_soundbank_1_w )
{
	if (ACCESSING_LSB)	OKIM6295_set_bank_base(1, 0x40000 * (data & 1) );
}

static MEMORY_READ16_START( cischeat_sound_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM						},	/* ROM*/
	{ 0x0f0000, 0x0fffff, MRA16_RAM						},	/* RAM*/
	{ 0x060004, 0x060005, soundlatch_word_r				},	/* From Main CPU*/
	{ 0x080002, 0x080003, YM2151_status_port_0_lsb_r	},
	{ 0x0a0000, 0x0a0001, OKIM6295_status_0_lsb_r		},
	{ 0x0c0000, 0x0c0001, OKIM6295_status_1_lsb_r		},
MEMORY_END

static MEMORY_WRITE16_START( cischeat_sound_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM						},	/* ROM*/
	{ 0x0f0000, 0x0fffff, MWA16_RAM						},	/* RAM*/
	{ 0x040002, 0x040003, cischeat_soundbank_0_w		},	/* Sample Banking*/
	{ 0x040004, 0x040005, cischeat_soundbank_1_w		},	/* Sample Banking*/
	{ 0x060002, 0x060003, soundlatch2_word_w			},	/* To Main CPU*/
	{ 0x080000, 0x080001, YM2151_register_port_0_lsb_w	},
	{ 0x080002, 0x080003, YM2151_data_port_0_lsb_w		},
	{ 0x0a0000, 0x0a0003, OKIM6295_data_0_lsb_w			},
	{ 0x0c0000, 0x0c0003, OKIM6295_data_1_lsb_w			},
MEMORY_END


/**************************************************************************
							F1 GrandPrix Star
**************************************************************************/

static MEMORY_READ16_START( f1gpstar_sound_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM						},	/* ROM*/
	{ 0x0e0000, 0x0fffff, MRA16_RAM						},	/* RAM				(cischeat: f0000-fffff)*/
	{ 0x060000, 0x060001, soundlatch_word_r				},	/* From Main CPU	(cischeat: 60004)*/
	{ 0x080002, 0x080003, YM2151_status_port_0_lsb_r	},
	{ 0x0a0000, 0x0a0001, OKIM6295_status_0_lsb_r		},
	{ 0x0c0000, 0x0c0001, OKIM6295_status_1_lsb_r		},
MEMORY_END

static MEMORY_WRITE16_START( f1gpstar_sound_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM						},	/* ROM*/
	{ 0x0e0000, 0x0fffff, MWA16_RAM						},	/* RAM				(cischeat: f0000-fffff)*/
	{ 0x040004, 0x040005, cischeat_soundbank_0_w		},	/* Sample Banking	(cischeat: 40002)*/
	{ 0x040008, 0x040009, cischeat_soundbank_1_w		},	/* Sample Banking	(cischeat: 40004)*/
	{ 0x060000, 0x060001, soundlatch2_word_w			},	/* To Main CPU		(cischeat: 60002)*/
	{ 0x080000, 0x080001, YM2151_register_port_0_lsb_w	},
	{ 0x080002, 0x080003, YM2151_data_port_0_lsb_w		},
	{ 0x0a0000, 0x0a0003, OKIM6295_data_0_lsb_w			},
	{ 0x0c0000, 0x0c0003, OKIM6295_data_1_lsb_w			},
MEMORY_END


/**************************************************************************
							F1 GrandPrix Star II
**************************************************************************/

static MEMORY_READ16_START( f1gpstr2_sound_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM						},	/* ROM*/
	{ 0x0e0000, 0x0fffff, MRA16_RAM						},	/* RAM*/
	{ 0x060004, 0x060005, soundlatch_word_r				},	/* From Main CPU	(f1gpstar: 60000)*/
	{ 0x080002, 0x080003, YM2151_status_port_0_lsb_r	},
	{ 0x0a0000, 0x0a0001, OKIM6295_status_0_lsb_r		},
	{ 0x0c0000, 0x0c0001, OKIM6295_status_1_lsb_r		},
MEMORY_END

static MEMORY_WRITE16_START( f1gpstr2_sound_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM						},	/* ROM*/
	{ 0x0e0000, 0x0fffff, MWA16_RAM						},	/* RAM*/
	{ 0x040004, 0x040005, cischeat_soundbank_0_w		},	/* Sample Banking*/
	{ 0x040008, 0x040009, cischeat_soundbank_1_w		},	/* Sample Banking*/
	{ 0x04000e, 0x04000f, MWA16_NOP						},	/* ? 0				(f1gpstar: no)*/
	{ 0x060002, 0x060003, soundlatch2_word_w			},	/* To Main CPU		(f1gpstar: 60000)*/
	{ 0x080000, 0x080001, YM2151_register_port_0_lsb_w	},
	{ 0x080002, 0x080003, YM2151_data_port_0_lsb_w		},
	{ 0x0a0000, 0x0a0003, OKIM6295_data_0_lsb_w			},
	{ 0x0c0000, 0x0c0003, OKIM6295_data_1_lsb_w			},
MEMORY_END

/**************************************************************************


						Memory Maps - IO CPU (#5)


**************************************************************************/

READ16_HANDLER ( f1gpstr2_io_r )	{ return megasys1_vregs[offset + 0x1000/2]; }
WRITE16_HANDLER( f1gpstr2_io_w )	{ COMBINE_DATA(&megasys1_vregs[offset + 0x1000/2]); }

static MEMORY_READ16_START( f1gpstr2_io_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM						},	/* ROM*/
	{ 0x180000, 0x182fff, MRA16_RAM						},	/* RAM*/
	{ 0x080000, 0x080fff, f1gpstr2_io_r					},	/**/
MEMORY_END

static MEMORY_WRITE16_START( f1gpstr2_io_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM						},	/* ROM*/
	{ 0x180000, 0x182fff, MWA16_RAM						},	/* RAM*/
	{ 0x080000, 0x080fff, f1gpstr2_io_w					},	/**/
	{ 0x100000, 0x100001, MWA16_RAM, &f1gpstr2_ioready	},	/**/
	{ 0x200000, 0x200001, MWA16_NOP						},	/**/
MEMORY_END


/***************************************************************************


								Input Ports


***************************************************************************/

/* Fake input port to read the status of five buttons: used to
   implement the shift using 2 buttons, and the accelerator in
   f1gpstar */

/*Trust me, this macro doesn't work properly, Big Run inputs do NOT equal Cisco Heat inputs. (EC)*/

/**************************************************************************
								Big Run
**************************************************************************/

/*	Input Ports:	[0] Fake: Buttons Status*/
/*					[1] Coins		[2] Controls	[3] Unknown*/
/*					[4]	DSW 1 & 2	[5] DSW 3		[6] Driving Wheel*/

INPUT_PORTS_START( bigrun )
	PORT_START	/* IN0 - Fake input port - Buttons status*/
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 ) \
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON3 ) \
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON4 ) \

	PORT_START	/* IN1 - Coins - $80000.w*/
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_COIN1    )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_COIN2    )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE ) 	/* called "Test"*/
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_START1   )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_START2   )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START	/* IN2 - Controls - $80002.w*/
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_BUTTON2 )	/* Brake*/
/*	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_BUTTON4 )	*/ /* Shift - We handle it using buttons 3&4*/
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_BUTTON5 )	/* Horn*/

	PORT_START	/* IN3 - Motor Control? - $80004.w*/
	PORT_DIPNAME( 0x01, 0x01, "Up Limit SW"  	)	/* Limit the Cockpit movements?*/
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On )  )
	PORT_DIPNAME( 0x02, 0x02, "Down Limit SW"	)
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On )  )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
/*	PORT_DIPNAME( 0x10, 0x10, "Right Limit SW"	)*/
/*	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )*/
/*	PORT_DIPSETTING(    0x00, DEF_STR( On )  )*/
/*	PORT_DIPNAME( 0x20, 0x20, "Left Limit SW"	)*/
/*	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )*/
/*	PORT_DIPSETTING(    0x00, DEF_STR( On )  )*/
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN4 - DSW 2 & 3 - $80006.w*/
	/* DSW 3*/
	PORT_DIPNAME( 0x0003, 0x0003, "Unknown 3-0&1*" )
	PORT_DIPSETTING(      0x0003, "3" )
	PORT_DIPSETTING(      0x0002, "2" )
	PORT_DIPSETTING(      0x0001, "1" )
	PORT_DIPSETTING(      0x0000, "0" )
	PORT_DIPNAME( 0x0004, 0x0004, "Allow Continue" )
	PORT_DIPSETTING(      0x0004, DEF_STR( No )  )
	PORT_DIPSETTING(      0x0000, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Unknown 3-3" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On )  )
	PORT_DIPNAME( 0x0010, 0x0010, "Move Cabinet" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No )  )
	PORT_DIPSETTING(      0x0010, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0060, 0x0060, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0000, "Easy" )
	PORT_DIPSETTING(      0x0060, "Normal" )
	PORT_DIPSETTING(      0x0020, "Hard" )
	PORT_DIPSETTING(      0x0040, "Hardest" )
	PORT_DIPNAME( 0x0080, 0x0080, "Automatic Game Start" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0080, "After 15 Seconds" )

	/* DSW 2*/
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On )  )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( On )  )
	PORT_DIPNAME( 0x1c00, 0x1c00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x1800, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x1c00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0c00, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x1400, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xe000, 0xe000, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0xc000, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0xe000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x6000, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0xa000, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )

	PORT_START	/* IN5 - DSW 3 (4 bits, Cabinet Linking) - $82200.w*/
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x06, 0x00, "Unit ID"             )
	PORT_DIPSETTING(    0x00, "1 (Blue-White Car)"  )
	PORT_DIPSETTING(    0x02, "2 (Green-White Car)" )
	PORT_DIPSETTING(    0x04, "3 (Red-White Car)"   )
	PORT_DIPSETTING(    0x06, "4 (Yellow Car)"      )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN6 - Driving Wheel - $80010.w(0)*/
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_X | IPF_CENTER, 30, 30, 0, 0xff)
INPUT_PORTS_END


/**************************************************************************
								Cisco Heat
**************************************************************************/

/*	Input Ports:	[0] Fake: Buttons Status*/
/*					[1] Coins		[2] Controls	[3] Unknown*/
/*					[4]	DSW 1 & 2	[5] DSW 3		[6] Driving Wheel*/

INPUT_PORTS_START( cischeat )
	PORT_START	/* IN0 - Fake input port - Buttons status*/
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON3 ) \
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON4 ) \

	PORT_START	/* IN1 - Coins - $80000.w*/
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_COIN1    )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_COIN2    )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE ) 	/* called "Test"*/
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_START1   )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_START2   )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START	/* IN2 - Controls - $80002.w*/
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_BUTTON2 )	/* Brake*/
/*	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_BUTTON4 )	*/ /* Shift - We handle it using buttons 3&4*/
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )	/* Accel*/
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_BUTTON5 )	/* Horn*/

	PORT_START	/* IN3 - Motor Control? - $80004.w*/
	PORT_DIPNAME( 0x01, 0x01, "Up Limit SW"  	)	/* Limit the Cockpit movements?*/
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On )  )
	PORT_DIPNAME( 0x02, 0x02, "Down Limit SW"	)
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On )  )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x10, 0x10, "Right Limit SW"	)
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On )  )
	PORT_DIPNAME( 0x20, 0x20, "Left Limit SW"	)
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On )  )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN4 - DSW 1 & 2 - $80006.w -> !f000a.w(hi byte) !f0008.w(low byte)*/
	COINAGE_6BITS_2
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )	/* unused?*/
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On )  )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )	/* unused?*/
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On )  )
	/* DSW 2*/
	PORT_DIPNAME( 0x0300, 0x0300, "Unit ID"			)		/* -> !f0020 (ID of this unit, when linked)*/
	PORT_DIPSETTING(      0x0300, "0 (Red Car)"    )
	PORT_DIPSETTING(      0x0200, "1 (Blue Car)"   )
	PORT_DIPSETTING(      0x0100, "2 (Yellow Car)" )
	PORT_DIPSETTING(      0x0000, "3 (Green Car)"  )
	PORT_DIPNAME( 0x0c00, 0x0c00, DEF_STR( Difficulty ) )	/* -> !f0026*/
	PORT_DIPSETTING(      0x0000, "Easy"    )
	PORT_DIPSETTING(      0x0c00, "Normal"  )
	PORT_DIPSETTING(      0x0800, "Hard"    )
	PORT_DIPSETTING(      0x0400, "Hardest" )
	PORT_BITX(    0x1000, 0x1000, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Infinite Time", IP_KEY_NONE, IP_JOY_NONE ) /* -> !f0028*/
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On )  )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( On )  )
	PORT_DIPNAME( 0x4000, 0x4000, "Country" )
	PORT_DIPSETTING(      0x4000, "Japan" )
	PORT_DIPSETTING(      0x0000, "USA"   )
	PORT_DIPNAME( 0x8000, 0x8000, "Allow Continue" )		/* -> !f00c0*/
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On )  )

	PORT_START	/* IN5 - DSW 3 (4 bits, Cabinet Linking) - $82200.w*/
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x06, 0x06, "Unit ID (2)" )	/* -> f0020 (like DSW2 !!)*/
	PORT_DIPSETTING(    0x06, "Use other"      )
	PORT_DIPSETTING(    0x00, "0 (Red Car)"    )
	PORT_DIPSETTING(    0x02, "1 (Blue Car)"   )
	PORT_DIPSETTING(    0x04, "2 (Yellow Car)" )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN6 - Driving Wheel - $80010.w(0)*/
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_X | IPF_CENTER, 30, 30, 0, 0xff)
INPUT_PORTS_END


/**************************************************************************
								F1 GrandPrix Star
**************************************************************************/

/*	Input Ports:	[0] Fake: Buttons Status*/
/*					[1] DSW 1 & 2		[2] Controls		[3] Unknown*/
/*					[4]	DSW 3			[5] Driving Wheel*/
/*					[6]	Coinage JP&USA	[7] Coinage UK&FR*/

INPUT_PORTS_START( f1gpstar )
	PORT_START	/* IN0 - Fake input port - Buttons status*/
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 ) \
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON3 ) \
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON4 ) \

/*	[Country]
	Japan		"race together" in Test Mode, Always Choose Race
				Japanese, Km/h, "handle shock"  , "(c)1991",
	USA			English,  Mph , "steering shock", "(c)1992 North America Only"
	England		English,  Mph , "steering shock", "(c)1992"
	France		French,   Km/h, "steering shock", "(c)1992"	*/

	PORT_START	/* IN1 - DSW 1 & 2 - $80000.w	-> !f9012*/
	/* DSW 1 ( Coinage - it changes with Country: we use IN6 & IN7 )*/
	PORT_DIPNAME( 0x0040, 0x0040, "Free Play (UK FR)" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On )  )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )	/* unused?*/
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On )  )
	/* DSW 2*/
	PORT_DIPNAME( 0x0300, 0x0100, "Country"  )	/* -> !f901e*/
	PORT_DIPSETTING(      0x0300, "Japan"   )
	PORT_DIPSETTING(      0x0200, "USA"     )
	PORT_DIPSETTING(      0x0100, "UK"      )
	PORT_DIPSETTING(      0x0000, "France"  )
	PORT_DIPNAME( 0x0c00, 0x0c00, DEF_STR( Difficulty ) )	/* -> !f9026*/
	PORT_DIPSETTING(      0x0000, "Easy"      )	/* 58 <- Initial Time (seconds, Germany)*/
	PORT_DIPSETTING(      0x0c00, "Normal"    )	/* 51*/
	PORT_DIPSETTING(      0x0800, "Hard"      )	/* 48*/
	PORT_DIPSETTING(      0x0400, "Very Hard" )	/* 46*/
	PORT_BITX(    0x1000, 0x1000, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Infinite Time", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On )  )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( On )  )
	PORT_DIPNAME( 0x4000, 0x4000, "Choose Race (US UK FR)"  )	/* -> f0020*/
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( On )  )
	PORT_DIPNAME( 0x8000, 0x8000, "Vibrations" )
	PORT_DIPSETTING(      0x8000, "Torque" )
	PORT_DIPSETTING(      0x0000, "Shake"  )

	PORT_START	/* IN2 - Controls - $80004.w -> !f9016*/
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_COIN1    )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_COIN2    )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX( 0x0008, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE ) /* -> f0100 (called "Test")*/
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_START1   )
/*	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON4  ) */ /* Shift -> !f900e - We handle it with 2 buttons*/
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON2  ) /* Brake -> !f9010*/
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START2   ) /* "Race Together"*/

	PORT_START	/* IN3 - ? Read at boot only - $80006.w*/
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

/*	DSW3-2&1 (Country: JP)	Effect
	OFF-OFF					Red-White Car
	OFF- ON					Red Car
	ON-OFF					Blue-White Car
	ON- ON					Blue Car, "equipped with communication link"	*/

	PORT_START	/* IN4 - DSW 3 (4 bits, Cabinet Linking) - $8000c.w -> !f9014*/
	PORT_DIPNAME( 0x01, 0x01, "This Unit Is" )
	PORT_DIPSETTING(    0x01, "Slave" )
	PORT_DIPSETTING(    0x00, "Master" )
	PORT_DIPNAME( 0x06, 0x06, "Unit ID" )			/* -> !f901c*/
	PORT_DIPSETTING(    0x06, "0 (Red-White Car)" )
	PORT_DIPSETTING(    0x04, "1 (Red Car)" )
	PORT_DIPSETTING(    0x02, "2 (Blue-White Car)" )
	PORT_DIPSETTING(    0x00, "3 (Blue Car)" )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Redundant: Invert Unit ID*/
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

				/* 		 Accelerator   - $80010.b ->  !f9004.w*/
	PORT_START	/* IN5 - Driving Wheel - $80011.b ->  !f9008.w*/
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_X | IPF_CENTER, 30, 30, 0, 0xff)

	PORT_START	/* IN6 - Coinage Japan & USA (it changes with Country)*/
	PORT_DIPNAME( 0x0007, 0x0007, "Coin A (JP US)" )
	PORT_DIPSETTING(      0x0001, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0038, 0x0038, "Coin B (JP US)" )
	PORT_DIPSETTING(      0x0008, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_4C ) )

	PORT_START	/* IN7 - Coinage UK & France (it changes with Country)*/
	PORT_DIPNAME( 0x0007, 0x0007, "Coin A (UK FR)" )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 1C_7C ) )
	PORT_DIPNAME( 0x0038, 0x0038, "Coin B (UK FR)" )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_4C ) )
INPUT_PORTS_END


/**************************************************************************
								Scud Hammer
**************************************************************************/

INPUT_PORTS_START( scudhamm )
	PORT_START	/* IN0 - Buttons*/
	PORT_BIT_IMPULSE( 0x0001, IP_ACTIVE_LOW, IPT_COIN1, 1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN  ) /* GAME OVER if pressed on the selection screen*/
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX( 0x0008, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE ) 	/* called "Test"*/
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_START1   )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON4  ) /* Select*/
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_BIT(  0x0100, IP_ACTIVE_HIGH, IPT_BUTTON1 ) /* Gu*/
	PORT_BIT(  0x0200, IP_ACTIVE_HIGH, IPT_BUTTON2 ) /* Choki*/
	PORT_BIT(  0x0400, IP_ACTIVE_HIGH, IPT_BUTTON3 ) /* Pa*/
	PORT_BIT(  0x0800, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x4000, IP_ACTIVE_HIGH, IPT_TILT    )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START	/* IN1 - A/D*/
	PORT_ANALOG( 0x00ff, 0x0000, IPT_PADDLE | IPF_CENTER, 1, 0, 0x0000, 0x00ff )

	PORT_START	/* IN2 - DSW*/
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0000, "Easy" )
	PORT_DIPSETTING(      0x0003, "Normal" )
	PORT_DIPSETTING(      0x0002, "Hard" )
	PORT_DIPSETTING(      0x0001, "Hardest" )
	PORT_DIPNAME( 0x000c, 0x000c, "Time To Hit" )
	PORT_DIPSETTING(      0x000c, "2 s" )
	PORT_DIPSETTING(      0x0008, "3 s" )
	PORT_DIPSETTING(      0x0004, "4 s" )
	PORT_DIPSETTING(      0x0000, "5 s" )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0020, "3" )
	PORT_DIPSETTING(      0x0000, "5" )
	PORT_DIPNAME( 0x0040, 0x0040, "Unknown 1-6" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Unknown 1-7" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_DIPNAME( 0x0700, 0x0700, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0300, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0700, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0600, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0500, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0800, 0x0800, "Unknown 2-3" )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, "Unknown 2-4" )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, "Unknown 2-5" )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, "Unknown 2-6" )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, "Unknown 2-7" )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END



/**************************************************************************


								Gfx Layouts


**************************************************************************/

/* 8x8x4, straightforward layout */
static struct GfxLayout tiles_8x8 =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ STEP4(0,1) },
	{ STEP8(0,4) },
	{ STEP8(0,4*8) },
	8*8*4
};

/* 16x16x4, straightforward layout */
static struct GfxLayout tiles_16x16 =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ STEP4(0,1) },
	{ STEP16(0,4) },
	{ STEP16(0,4*16) },
	16*16*4
};

/* 16x16x4, made of four 8x8 tiles */
static struct GfxLayout tiles_16x16_quad =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ STEP4(0,1) },
	{ STEP8(8*8*4*0,4), STEP8(8*8*4*2,4) },
	{ STEP16(0,4*8) },
	16*16*4
};

/* Road: 64 x 1 x 4 */
static struct GfxLayout road_layout =
{
	64,1,
	RGN_FRAC(1,1),
	4,
	{ STEP4(0,1) },
	{ STEP16(16*4*0,4),STEP16(16*4*1,4),
	  STEP16(16*4*2,4),STEP16(16*4*3,4) },
	{ 0 },
	64*1*4
};

/**************************************************************************
								Big Run
**************************************************************************/

static struct GfxDecodeInfo bigrun_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tiles_8x8,	0x0000, 16 }, /* [0] Scroll 0*/
	{ REGION_GFX2, 0, &tiles_8x8,	0x0100, 16 }, /* [1] Scroll 1*/
	{ REGION_GFX3, 0, &tiles_8x8,	0x0e00, 16 }, /* [2] Scroll 2*/
	{ REGION_GFX4, 0, &tiles_16x16,	0x0a00, 64 }, /* [3] Sprites*/
	{ REGION_GFX5, 0, &road_layout,	0x0600, 64 }, /* [4] Road 0*/
	{ REGION_GFX6, 0, &road_layout,	0x0200, 64 }, /* [5] Road 1*/
	{ -1 }
};

/**************************************************************************
								Cisco Heat
**************************************************************************/

static struct GfxDecodeInfo cischeat_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tiles_8x8,	0x0000, 32  }, /* [0] Scroll 0*/
	{ REGION_GFX2, 0, &tiles_8x8,	0x0200, 32  }, /* [1] Scroll 1*/
	{ REGION_GFX3, 0, &tiles_8x8,	0x0400, 32  }, /* [2] Scroll 2*/
	{ REGION_GFX4, 0, &tiles_16x16,	0x0e00, 128 }, /* [3] Sprites*/
	{ REGION_GFX5, 0, &road_layout,	0x0600, 64  }, /* [4] Road 0*/
	{ REGION_GFX6, 0, &road_layout,	0x0a00, 64  }, /* [5] Road 1*/
	{ -1 }
};

/**************************************************************************
							F1 GrandPrix Star
**************************************************************************/

static struct GfxDecodeInfo f1gpstar_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tiles_8x8,	0x0000, 16  }, /* [0] Scroll 0*/
	{ REGION_GFX2, 0, &tiles_8x8,	0x0100, 16  }, /* [1] Scroll 1*/
	{ REGION_GFX3, 0, &tiles_8x8,	0x0200, 16  }, /* [2] Scroll 2*/
	{ REGION_GFX4, 0, &tiles_16x16,	0x0b00, 128 }, /* [3] Sprites*/
	{ REGION_GFX5, 0, &road_layout,	0x0300, 64  }, /* [4] Road 0*/
	{ REGION_GFX6, 0, &road_layout,	0x0700, 64  }, /* [5] Road 1*/
	{ -1 }
};

/**************************************************************************
								Scud Hammer
**************************************************************************/

static struct GfxDecodeInfo gfxdecodeinfo_scudhamm[] =
{
	{ REGION_GFX1, 0, &tiles_8x8,			0x0000, 16  },	/* [0] Scroll 0*/
	{ REGION_GFX1, 0, &tiles_8x8,			0x0000, 16  },	/* [1] UNUSED*/
	{ REGION_GFX3, 0, &tiles_8x8,			0x0100, 16  },	/* [2] Scroll 2*/
	{ REGION_GFX4, 0, &tiles_16x16_quad,	0x0200, 128 },	/* [3] sprites*/
	/* No Road Layers*/
	{ -1 }
};


/***************************************************************************


								Machine Drivers


**************************************************************************/

/**************************************************************************
					Big Run, Cisco Heat, F1 GrandPrix Star
**************************************************************************/

/* CPU # 1 */
#define CISCHEAT_INTERRUPT_NUM	3
INTERRUPT_GEN( cischeat_interrupt )
{
	if (cpu_getiloops()==0)
		cpu_set_irq_line(0, 4, HOLD_LINE); /* Once */
	else
	{
		if (cpu_getiloops()%2)	cpu_set_irq_line(0, 2, HOLD_LINE);
		else 					cpu_set_irq_line(0, 1, HOLD_LINE);
	}
}


/* CPU # 2 & 3 */
#define CISCHEAT_SUB_INTERRUPT_NUM	1

/* CPU # 4 */
#define CISCHEAT_SOUND_INTERRUPT_NUM	16

#define STD_FM_CLOCK	3000000
#define STD_OKI_CLOCK	  12000



static struct YM2151interface ym2151_intf =
{
	1,
	STD_FM_CLOCK,
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) },
	{ 0 }
};

static struct OKIM6295interface okim6295_intf =
{
	2,
	{STD_OKI_CLOCK, STD_OKI_CLOCK},
	{REGION_SOUND1,REGION_SOUND2},
	{ MIXER(100,MIXER_PAN_LEFT), MIXER(100,MIXER_PAN_RIGHT) }
};



static MACHINE_DRIVER_START( bigrun )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("cpu1", M68000, 10000000)
	MDRV_CPU_MEMORY(bigrun_readmem,bigrun_writemem)
	MDRV_CPU_VBLANK_INT(cischeat_interrupt,CISCHEAT_INTERRUPT_NUM)

	MDRV_CPU_ADD_TAG("cpu2", M68000, 10000000)
	MDRV_CPU_MEMORY(bigrun_readmem2,bigrun_writemem2)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,CISCHEAT_SUB_INTERRUPT_NUM)

	MDRV_CPU_ADD_TAG("cpu3", M68000, 10000000)
	MDRV_CPU_MEMORY(bigrun_readmem3,bigrun_writemem3)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,CISCHEAT_SUB_INTERRUPT_NUM)

	MDRV_CPU_ADD_TAG("sound", M68000, 6000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(bigrun_sound_readmem,bigrun_sound_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,CISCHEAT_SOUND_INTERRUPT_NUM)

	MDRV_FRAMES_PER_SECOND(30)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_30HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(20)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_AFTER_VBLANK | VIDEO_HAS_SHADOWS)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 256-1,	0+16, 256-16-1)
	MDRV_GFXDECODE(bigrun_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(16*16 * 3 + 64*16 * 2 + 64*16)	/* scroll 0,1,2; road 0,1; sprites */

	MDRV_VIDEO_START(bigrun)
	MDRV_VIDEO_UPDATE(bigrun)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_intf)
	MDRV_SOUND_ADD(OKIM6295, okim6295_intf)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( cischeat )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(bigrun)
	MDRV_CPU_MODIFY("cpu1")
	MDRV_CPU_MEMORY(cischeat_readmem,cischeat_writemem)

	MDRV_CPU_MODIFY("cpu2")
	MDRV_CPU_MEMORY(cischeat_readmem2,cischeat_writemem2)

	MDRV_CPU_MODIFY("cpu3")
	MDRV_CPU_MEMORY(cischeat_readmem3,cischeat_writemem3)

	MDRV_CPU_MODIFY("sound")
	MDRV_CPU_MEMORY(cischeat_sound_readmem,cischeat_sound_writemem)

	/* video hardware */
	MDRV_VISIBLE_AREA(0, 256-1,	0+16, 256-16-8-1)
	MDRV_GFXDECODE(cischeat_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32*16 * 3 + 64*16 * 2 + 128*16)	/* scroll 0,1,2; road 0,1; sprites */

	MDRV_VIDEO_START(cischeat)
	MDRV_VIDEO_UPDATE(cischeat)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( f1gpstar )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(bigrun)
	MDRV_CPU_REPLACE("cpu1", M68000, 12000000)
	MDRV_CPU_MEMORY(f1gpstar_readmem,f1gpstar_writemem)

	MDRV_CPU_REPLACE("cpu2", M68000, 12000000)
	MDRV_CPU_MEMORY(f1gpstar_readmem2,f1gpstar_writemem2)

	MDRV_CPU_REPLACE("cpu3", M68000, 12000000)
	MDRV_CPU_MEMORY(f1gpstar_readmem3,f1gpstar_writemem3)

	MDRV_CPU_MODIFY("sound")
	MDRV_CPU_MEMORY(f1gpstar_sound_readmem,f1gpstar_sound_writemem)

	/* video hardware */
	MDRV_GFXDECODE(f1gpstar_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(16*16 * 3 + 64*16 * 2 + 128*16)	/* scroll 0,1,2; road 0,1; sprites */

	MDRV_VIDEO_START(f1gpstar)
	MDRV_VIDEO_UPDATE(f1gpstar)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( f1gpstr2 )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(f1gpstar)

	MDRV_CPU_MODIFY("cpu1")
	MDRV_CPU_MEMORY(f1gpstr2_readmem,f1gpstr2_writemem)

	MDRV_CPU_MODIFY("sound")
	MDRV_CPU_MEMORY(f1gpstr2_sound_readmem,f1gpstr2_sound_writemem)

	MDRV_CPU_ADD_TAG("cpu5", M68000, 10000000)
	MDRV_CPU_MEMORY(f1gpstr2_io_readmem,f1gpstr2_io_writemem)
MACHINE_DRIVER_END


/**************************************************************************
								Scud Hammer
**************************************************************************/

static struct OKIM6295interface scudhamm_okim6295_intf =
{
	2,
	{ 16000,16000 },
	{ REGION_SOUND1, REGION_SOUND2 },
	{ MIXER(100,MIXER_PAN_LEFT), MIXER(100,MIXER_PAN_RIGHT) }
};

/*
	1, 5-7] 	busy loop
	2]			clr.w   $fc810.l + rte
	3]			game
	4]	 		== 3
*/
#define INTERRUPT_NUM_SCUDHAMM		30
INTERRUPT_GEN( interrupt_scudhamm )
{
	switch ( cpu_getiloops() )
	{
		case 0:		cpu_set_irq_line(0, 3, PULSE_LINE);	/* update palette, layers etc. Not the sprites.*/
		case 14:	cpu_set_irq_line(0, 2, PULSE_LINE);	/* "real" vblank. It just sets a flag that*/
														/* the main loop polls before updating the sprites.*/
	}
}


static MACHINE_DRIVER_START( scudhamm )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)
	MDRV_CPU_MEMORY(readmem_scudhamm,writemem_scudhamm)
	MDRV_CPU_VBLANK_INT(interrupt_scudhamm,INTERRUPT_NUM_SCUDHAMM)

	MDRV_FRAMES_PER_SECOND(30)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_30HZ_VBLANK_DURATION * 3)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_AFTER_VBLANK | VIDEO_HAS_SHADOWS)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 256-1, 0 +16, 256-1 -16)
	MDRV_GFXDECODE(gfxdecodeinfo_scudhamm)
	MDRV_PALETTE_LENGTH(16*16+16*16+128*16)

	MDRV_VIDEO_START(f1gpstar)
	MDRV_VIDEO_UPDATE(scudhamm)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(OKIM6295, scudhamm_okim6295_intf)
MACHINE_DRIVER_END


/***************************************************************************


								ROMs Loading


**************************************************************************/

/*
	Sprite data is stored like this:

	Sprite 0
		Line 0-15 (left half)
		Line 0-15 (right half)
	Sprite 1
	..

	We need to untangle it
*/
void cischeat_untangle_sprites(int region)
{
	unsigned char		*src = memory_region(region);
	const unsigned char	*end = memory_region(region) + memory_region_length(region);

	while (src < end)
	{
		unsigned char sprite[16*8];
		int i;

		for (i = 0; i < 16 ; i++)
		{
			memcpy(&sprite[i*8+0], &src[i*4+0],    4);
			memcpy(&sprite[i*8+4], &src[i*4+16*4], 4);
		}
		memcpy(src, sprite, 16*8);
		src += 16*8;
	}
}



/***************************************************************************

									Big Run

Jaleco 1989

BR8950c
-------

 20MHz
                                    6264 6264
68000-10  E1 3 58257        SCPT              VCTR    5   6
          E2 4 58257
                                    6264 6264
                            SCPT              VCTR    7   8

                                    6264 6264
                            SCPT              VCTR    9


        PR88004D
                                    2088  2088


    2018
                                   PR88004P


8951
------
                          22
                          21     23
                    19    T48    T49   20

                    15    T42    T44   16

                     7    T41    T43   8

8952A
-----

                              18  T46   T45  14

68000


68000


                              18  T46   T45  4


8953
----

	68000                  YM2151
    D65006C
	BR8953C.1 58257
	BR8953C.2 58257
								6295    5  T50
       							6295    8  T51

***************************************************************************/

ROM_START( bigrun )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "br8950b.e1",  0x000000, 0x040000, CRC(bfb54a62) SHA1(49f78e162e8bc19a75c62029737acd665b9b124b) )
	ROM_LOAD16_BYTE( "br8950b.e2",  0x000001, 0x040000, CRC(c0483e81) SHA1(e8fd8860191c7d8cb4dda44c69ce05cd58174a07) )

	ROM_REGION( 0x40000, REGION_CPU2, 0 )
	ROM_LOAD16_BYTE( "br8952a.19", 0x000000, 0x020000, CRC(fcf6b70c) SHA1(34f7ca29241925251a043c63062596d1c220b730) ) /* 1xxxxxxxxxxxxxxxx = 0xFF*/
	ROM_LOAD16_BYTE( "br8952a.20", 0x000001, 0x020000, CRC(c43d367b) SHA1(86242ba03cd172e95bf42eac369fb3acf42c7c45) ) /* 1xxxxxxxxxxxxxxxx = 0xFF*/

	ROM_REGION( 0x40000, REGION_CPU3, 0 )
	ROM_LOAD16_BYTE( "br8952a.9",  0x000000, 0x020000, CRC(f803f0d9) SHA1(6d2e90e74bb15cd443d901ad0fc827117432daa6) ) /* 1xxxxxxxxxxxxxxxx = 0xFF*/
	ROM_LOAD16_BYTE( "br8952a.10", 0x000001, 0x020000, CRC(8c0df287) SHA1(3a2e689e2d841e2089b18267a8462f1acc40ae99) ) /* 1xxxxxxxxxxxxxxxx = 0xFF*/

	ROM_REGION( 0x40000, REGION_CPU4, 0 )
	ROM_LOAD16_BYTE( "br8953c.2", 0x000000, 0x020000, CRC(b690d8d9) SHA1(317c0b4e9bef30c84daf16765af66f5a6f9d1c54) )
	ROM_LOAD16_BYTE( "br8953c.1", 0x000001, 0x020000, CRC(79fc7bc0) SHA1(edb6cf626f93417cdc9525627d85a0ca9bcd1b1c) )

	ROM_REGION16_BE( 0x40000, REGION_USER1, 0 )	/* second halves of program ROMs */
	ROM_LOAD16_BYTE( "br8950b.3",   0x000000, 0x020000, CRC(864cebf7) SHA1(4e63106cd5832901688dfce2e450f536a6927c81) )
	ROM_LOAD16_BYTE( "br8950b.4",   0x000001, 0x020000, CRC(702c2a6e) SHA1(cf3327919e24b7206d404b008cb00117e4308f94) )

	ROM_REGION( 0x040000, REGION_GFX1, ROMREGION_DISPOSE ) /* scroll 0*/
	ROM_LOAD( "br8950b.5",  0x000000, 0x020000, CRC(0530764e) SHA1(0a89eab2ce9bd5df574a46bb6ea884c33407f122) )
	ROM_LOAD( "br8950b.6",  0x020000, 0x020000, CRC(cf40ecfa) SHA1(96e1bfb7a33603a1eaaeb31386bba947fc005060) )

	ROM_REGION( 0x040000, REGION_GFX2, ROMREGION_DISPOSE ) /* scroll 1*/
	ROM_LOAD( "br8950b.7",  0x000000, 0x020000, CRC(bd5fd061) SHA1(e5e0afb0a3a1d5f7a27bd04b724c48ea65872233) )
	ROM_LOAD( "br8950b.8",  0x020000, 0x020000, CRC(7d66f418) SHA1(f6564ac9d65d40af17f5dd422c435aeb6a385256) )

	ROM_REGION( 0x020000, REGION_GFX3, ROMREGION_DISPOSE ) /* scroll 2*/
	ROM_LOAD( "br8950b.9",  0x000000, 0x020000, CRC(be0864c4) SHA1(e0f1c0f09b30a731f0e062b1acb1b3a3a772a5cc) )

	ROM_REGION( 0x280000, REGION_GFX4, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD16_BYTE( "mr88004a.t41",  0x000000, 0x080000, CRC(c781d8c5) SHA1(52b23842a20443b51490d701dca72fb0a3118033) )
	ROM_LOAD16_BYTE( "mr88004d.t43",  0x000001, 0x080000, CRC(e4041208) SHA1(f5bd21b42f627b01bca2324082aecee7852a37e6) )
	ROM_LOAD16_BYTE( "mr88004b.t42",  0x100000, 0x080000, CRC(2df2e7b9) SHA1(5772e0dc2f842077ea70a558b55ec5ceea693f00) )
	ROM_LOAD16_BYTE( "mr88004e.t44",  0x100001, 0x080000, CRC(7af7fbf6) SHA1(5b8e2d3bb0c9f29f2c96f68c6d4c7fbf63e640c9) )
	ROM_LOAD16_BYTE( "mb88004c.t48",  0x200000, 0x040000, CRC(02e2931d) SHA1(754b38929a2dd10d39634fb9cf737e3175a8b1ec) )
	ROM_LOAD16_BYTE( "mb88004f.t49",  0x200001, 0x040000, CRC(4f012dab) SHA1(35f756b1c7b41f2e81ccbefb2075608a5d663152) )

	ROM_REGION( 0x100000, REGION_GFX5, ROMREGION_DISPOSE ) /* Road 0*/
	ROM_LOAD( "mr88004g.t45",  0x000000, 0x080000, CRC(bb397bae) SHA1(c67d33bde6e8de2ea7581faadb96acd977adc13c) )
	ROM_LOAD( "mb88004h.t46",  0x080000, 0x080000, CRC(6b31a1ba) SHA1(71a956f0f51a63bddedfef0febdc95108ed42226) )

	ROM_REGION( 0x100000, REGION_GFX6, ROMREGION_DISPOSE ) /* Road 1*/
	ROM_LOAD( "mr88004g.t45",  0x000000, 0x080000, CRC(bb397bae) SHA1(c67d33bde6e8de2ea7581faadb96acd977adc13c) )
	ROM_LOAD( "mb88004h.t46",  0x080000, 0x080000, CRC(6b31a1ba) SHA1(71a956f0f51a63bddedfef0febdc95108ed42226) )

	/* t50 & t51: 40000-5ffff = 60000-7ffff */
	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* samples */
	ROM_LOAD( "mb88004l.t50", 0x000000, 0x020000, CRC(6b11fb10) SHA1(eb6e9614bb50b8fc332ada61882da484d34d727f) )
	ROM_CONTINUE(             0x040000, 0x020000             )
	ROM_CONTINUE(             0x020000, 0x020000             )
	ROM_CONTINUE(             0x060000, 0x020000             )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* samples */
	ROM_LOAD( "mb88004m.t51", 0x000000, 0x020000, CRC(ee52f04d) SHA1(fc45bd1d3a7552433e40846c358573c6988127c3) )
	ROM_CONTINUE(             0x040000, 0x020000             )
	ROM_CONTINUE(             0x020000, 0x020000             )
	ROM_CONTINUE(             0x060000, 0x020000             )

	ROM_REGION( 0x20000, REGION_USER2, 0 )		/* ? Unused ROMs ? */
	ROM_LOAD( "br8951b.21",  0x000000, 0x020000, CRC(59b9c26b) SHA1(09fea3b77b045d9c1ed62bf53efa8b5242a33a10) )	/* x00xxxxxxxxxxxxx, mask=0001e0*/
	ROM_LOAD( "br8951b.22",  0x000000, 0x020000, CRC(c112a803) SHA1(224a2ed690b78caef266958a93524211ff4a8e70) )	/* x00xxxxxxxxxxxxx*/
	ROM_LOAD( "br8951b.23",  0x000000, 0x010000, CRC(b9474fec) SHA1(f1f0eab014e8f52572484b83f56189e0ff6f2b0d) )	/* 000xxxxxxxxxxxxx*/
ROM_END

DRIVER_INIT( bigrun )
{
	/* Split ROMs */
	rom_1 = (data16_t *) memory_region(REGION_USER1);

	cischeat_untangle_sprites(REGION_GFX4);	/* Untangle sprites*/
	phantasm_rom_decode(3);					/* Decrypt sound cpu code*/
}


/***************************************************************************

								Cisco Heat

From "ARCADE ROMS FROM JAPAN (ARFJ)"'s readme:

 -BOARD #1 CH-9072 EB90001-20024-
|                [9]r15 [10]r16  |	EP:	[1]ch9072.01	[2]ch9072.02
|               [11]r17 [12]r18  |		[3]ch9072.03
|               [13]r25 [14]r26  |MASK:	[9]ch9072.r15	[10]ch9072.r16
|               [15]r19 [16]r20  |		[11]ch9072.r17	[12]ch9072.r18
|     [1]01                      |		[13]ch9072.r25	[14]ch9072.r26
|[2]02[3]03                      |		[15]ch9072.r19	[16]ch9072.r20
|[4] [5] [6]                     |
|[7]                             |	([4][5][6][8]:27cx322  [7]:82S135)
|[8]                             |
 --------------------------------

Video                        Sound
 -BOARD #2 CH-9071 EB90001-20023- 	X1:12MHz  X2:4MHz  X3:20MHz  X4:7MHz
|68000             [9]      68000|	YM2151x1 OKI M6295 x2 ([8]82S147 [9]:82S185)
|[1]01 X3               X4 [11]11|	EP:	[1]ch9071v2.01 "CH-9071 Ver.2  1"
|[2]02                     [10]10|		[2]ch9071.02
|[3]03                           |		[3]ch9071v2.03 "CH-9071 Ver.2  3"
|[4]04            X1 X2   [12]r23|		[4]ch9071.04
|           [8]     YM2151[13]r24|		[7]ch9071.07
|[7]07[5]a14[6]t74               |		[10]ch9071.10 	[11]ch9071.11
|                                |MASK:	[5]ch9071.a14	[6]ch9071.t74
 --------------------------------		[12]ch9071.r23	[13]ch9071.r24

 -BOARD #3 CH-9073 EB90001-20025-
|           [5]r21 [6]r22   68000|
|    [9]    [1]01  [2]02         | EP:	[1]ch9073.01	[2]ch9073.02
|                                |		[3]ch9073v1.03 "CH-9073 Ver.1  3"
|           [7]r21 [8]r22   68000|		[4]ch9073v1.04 "CH-9073 Ver.1  4"
|           [3]03  [4]04         |MASK:	[5][7]ch9073.r21
|            [10]    [11]        |		[6][8]ch9073.r22
|                                |		([9][10][11]:82S129)
 --------------------------------

DIP SW:8BITx2 , 4BITx1

According to KLOV:

Controls:	Steering: Wheel - A 'judder' motor is attached to the wheel.
			Pedals: 2 - Both foot controls are simple switches.
Sound:		Amplified Stereo (two channel)

***************************************************************************/

ROM_START( cischeat )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "ch9071v2.03", 0x000000, 0x040000, CRC(dd1bb26f) SHA1(2b9330b45edcc3291ad4ac935558c1f070ab5bd9) )
	ROM_LOAD16_BYTE( "ch9071v2.01", 0x000001, 0x040000, CRC(7b65276a) SHA1(e0075b6d09da12ab7c84b888ffe65cd33ec7c6b6) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )
	ROM_LOAD16_BYTE( "ch9073.01",  0x000000, 0x040000, CRC(ba331526) SHA1(bed5182c0d63a64cc44f9c127d1b8ce55a701184) )
	ROM_LOAD16_BYTE( "ch9073.02",  0x000001, 0x040000, CRC(b45ff10f) SHA1(11fb2d9a3b5cc4f997816e0d3c9cf47bf749db17) )

	ROM_REGION( 0x80000, REGION_CPU3, 0 )
	ROM_LOAD16_BYTE( "ch9073v1.03", 0x000000, 0x040000, CRC(bf1d1cbf) SHA1(0892ce5f35864393a6f899f02f811b8fdba03978) )
	ROM_LOAD16_BYTE( "ch9073v1.04", 0x000001, 0x040000, CRC(1ec8a597) SHA1(311d4aa8bd92dd2eea0a64f881f64d19b7ba7d12) )

	ROM_REGION( 0x40000, REGION_CPU4, 0 )
	ROM_LOAD16_BYTE( "ch9071.11", 0x000000, 0x020000, CRC(bc137bea) SHA1(ca6d781a617c797aec87e6ce0a002280aa62aebc) )
	ROM_LOAD16_BYTE( "ch9071.10", 0x000001, 0x020000, CRC(bf7b634d) SHA1(29186c41a397df322cc2c40decd1c19963f89d36) )

	ROM_REGION16_BE( 0x100000, REGION_USER1, 0 )	/* second halves of program ROMs */
	ROM_LOAD16_BYTE( "ch9071.04",   0x000000, 0x040000, CRC(7fb48cbc) SHA1(7f0442ce37b39e830fe8bcb8230cf7da2103059d) )	/* cpu #1*/
	ROM_LOAD16_BYTE( "ch9071.02",   0x000001, 0x040000, CRC(a5d0f4dc) SHA1(2e7aaa915e27ab31e38ca6759301ffe33a12b427) )
	/* cpu #2 (0x40000 bytes will be copied here)*/
	/* cpu #3 (0x40000 bytes will be copied here)*/

	ROM_REGION( 0x040000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ch9071.a14",  0x000000, 0x040000, CRC(7a6d147f) SHA1(8f52e012d9699311c2a2409130c6200c6d2e1c51) ) /* scroll 0*/

	ROM_REGION( 0x040000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ch9071.t74",  0x000000, 0x040000, CRC(735a2e25) SHA1(51f528db207283c0d2b70acd5037ffafbe24f6f3) ) /* scroll 1*/

	ROM_REGION( 0x010000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "ch9071.07",   0x000000, 0x010000, CRC(3724ccc3) SHA1(3797ea49156362467ba948c51ac7b52610d1b9de) ) /* scroll 2*/

	ROM_REGION( 0x400000, REGION_GFX4, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD16_BYTE( "ch9072.r15", 0x000000, 0x080000, CRC(38af4aea) SHA1(ea27ab44b33776984adaa9b26d85165d6700a12c) )
	ROM_LOAD16_BYTE( "ch9072.r16", 0x000001, 0x080000, CRC(71388dad) SHA1(0d2451e36cfbf7400ade849b4c8a1e8f56fc089c) )
	ROM_LOAD16_BYTE( "ch9072.r17", 0x100000, 0x080000, CRC(9d052cf3) SHA1(d6bd30965316104cb03e62a69df61eb60eb84741) )
	ROM_LOAD16_BYTE( "ch9072.r18", 0x100001, 0x080000, CRC(fe402a56) SHA1(c64aa0d83b77ce0fea5072a9332bbcbbd94a1be4) )
	ROM_LOAD16_BYTE( "ch9072.r25", 0x200000, 0x080000, CRC(be8cca47) SHA1(4e64cbbdec9b55721e420b50f0a563684e93f739) )
	ROM_LOAD16_BYTE( "ch9072.r26", 0x200001, 0x080000, CRC(2f96f47b) SHA1(045842428849bc4afb8b59f7f0594b3f537f9e12) )
	ROM_LOAD16_BYTE( "ch9072.r19", 0x300000, 0x080000, CRC(4e996fa8) SHA1(c74d761e0c8d17b3fb5d33b06136c4d0ba87c2e1) )
	ROM_LOAD16_BYTE( "ch9072.r20", 0x300001, 0x080000, CRC(fa70b92d) SHA1(01b5f7309c9c7cd6d41c0f46678772dda45344e1) )

	ROM_REGION( 0x100000, REGION_GFX5, ROMREGION_DISPOSE )
	ROM_LOAD( "ch9073.r21",  0x000000, 0x080000, CRC(2943d2f6) SHA1(ae8a25c1d76d3c36aa326d0171acb7dce93c4d87) ) /* Road*/
	ROM_LOAD( "ch9073.r22",  0x080000, 0x080000, CRC(2dd44f85) SHA1(5f20f75e96e14389187d3471bc7f2ceb0758eec4) )

	ROM_REGION( 0x100000, REGION_GFX6, ROMREGION_DISPOSE )
	ROM_LOAD( "ch9073.r21",  0x000000, 0x080000, CRC(2943d2f6) SHA1(ae8a25c1d76d3c36aa326d0171acb7dce93c4d87) ) /* Road*/
	ROM_LOAD( "ch9073.r22",  0x080000, 0x080000, CRC(2dd44f85) SHA1(5f20f75e96e14389187d3471bc7f2ceb0758eec4) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* samples */
	ROM_LOAD( "ch9071.r23", 0x000000, 0x080000, CRC(c7dbb992) SHA1(9fa802830947f4e5d41447b2ac9637daf1fd8193) ) /* 2 x 0x40000*/

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* samples */
	ROM_LOAD( "ch9071.r24", 0x000000, 0x080000, CRC(e87ca4d7) SHA1(b2a2edd701324640e438d1e84dd033ec212917b5) ) /* 2 x 0x40000 (FIRST AND SECOND HALF IDENTICAL)*/

	ROM_REGION( 0x40000, REGION_USER2, 0 )		/* ? Unused ROMs ? */
	ROM_LOAD( "ch9072.01",  0x000000, 0x020000, CRC(b2efed33) SHA1(3b347d4bc866aaa6cb53bd0991b4fb6a22e40a5c) ) /* FIXED BITS (xxxxxxxx0xxxxxxx)*/
	ROM_LOAD( "ch9072.02",  0x000000, 0x040000, CRC(536edde4) SHA1(45ebd2add357275177fcd7b6d9ea748c6756f1c0) )
	ROM_LOAD( "ch9072.03",  0x000000, 0x040000, CRC(7e79151a) SHA1(5a305cff8600446be426641ce112208b379094b9) )
ROM_END

DRIVER_INIT( cischeat )
{
	/* Split ROMs */
	rom_1 = (data16_t *) (memory_region(REGION_USER1) + 0x00000);
	rom_2 = (data16_t *) (memory_region(REGION_CPU2)  + 0x40000);
	rom_3 = (data16_t *) (memory_region(REGION_CPU3)  + 0x40000);

	memcpy(memory_region(REGION_USER1) + 0x80000, rom_2, 0x40000);
	memset(rom_2, 0, 0x40000);
	rom_2 = (data16_t *) (memory_region(REGION_USER1) + 0x80000);

	memcpy(memory_region(REGION_USER1) + 0xc0000, rom_3, 0x40000);
	memset(rom_3, 0, 0x40000);
	rom_3 = (data16_t *) (memory_region(REGION_USER1) + 0xc0000);

	cischeat_untangle_sprites(REGION_GFX4);	/* Untangle sprites*/
	astyanax_rom_decode(3);					/* Decrypt sound cpu code*/
}


/***************************************************************************

							F1 GrandPrix Star

From malcor's readme:

Location     Device      File ID     Checksum      PROM Label
-----------------------------------------------------------------
LB IC2       27C010    9188A-1.V10     46C6     GP-9188A 1  Ver1.0
LB IC27      27C010    9188A-6.V10     DB84     GP-9188A 6  Ver1.0
LB IC46      27C010    9188A-11.V10    DFEA     GP-9188A 11 Ver1.0
LB IC70      27C010    9188A-16.V10    1034     GP-9188A 16 Ver1.0
LB IC91      27C020    9188A-21.V10    B510     GP-9188A 21 Ver1.0
LB IC92      27C020    9188A-22.V20    A9BA     GP-9188A 22 Ver2.0
LB IC124     27C020    9188A-26.V10    AA81     GP-9188A 26 Ver1.0
LB IC125     27C020    9188A-27.V20    F34D     GP-9188A 27 Ver2.0
LB IC174    23C1000    9188A-30.V10    0AA5     GP-9188A 30 Ver1.0
TB IC2       27C010    9190A-1.V11     B05A     GP-9190A 1  Ver1.1
TB IC4       27C010    9190A-2.V11     ED7A     GP-9190A 2  Ver1.1
LB IC37     23C4001    90015-01.W06    F10F     MR90015-01-W06      *
LB IC79     23C4001    90015-01.W06    F10F     MR90015-01-W06      *
LB IC38     23C4001    90015-02.W07    F901     MR90015-02-W07      *
LB IC80     23C4001    90015-02.W07    F901     MR90015-02-W07      *
LB IC39     23C4001    90015-03.W08    F100     MR90015-03-W08      *
LB IC81     23C4001    90015-03.W08    F100     MR90015-03-W08      *
LB IC40     23C4001    90015-04.W09    FA00     MR90015-04-W09      *
LB IC82     23C4001    90015-04.W09    FA00     MR90015-04-W09      *
LB IC64     23C4001    90015-05.W10    5FF9     MR90015-05-W10
LB IC63     23C4001    90015-06.W11    6EDA     MR90015-06-W11
LB IC62     23C4001    90015-07.W12    E9B4     MR90015-07-W12
LB IC61     23C4001    90015-08.W14    5107     MR90015-08-W14
LB IC17     23C4001    90015-09.W13    71EE     MR90015-09-W13
LB IC16     23C4001    90015-10.W15    EFEF     MR90015-10-W15
MB IC54     23C4001    90015-20.R45    7890     MR90015-20-R45      *
MB IC67     23C4001    90015-20.R45    7890     MR90015-20-R45      *
MB IC1      23C4001    90015-21.R46    C73C     MR90015-21-R46
MB IC2      23C4001    90015-22.R47    5D58     MR90015-22-R47
MB IC5      23C4001    90015-23.R48    4E7B     MR90015-23-R48
MB IC6      23C4001    90015-24.R49    F6A0     MR90015-24-R49
MB IC11     23C4001    90015-25.R50    9FC0     MR90015-25-R50
MB IC12     23C4001    90015-26.R51    13E4     MR90015-26-R51
MB IC15     23C4001    90015-27.R52    8D5D     MR90015-27-R52
MB IC16     23C4001    90015-28.R53    E0B8     MR90015-28-R53
MB IC21     23C4001    90015-29.R54    DF33     MR90015-29-R54
MB IC22     23C4001    90015-30.R55    DA2D     MR90015-30-R55
LB IC123    23C4001    90015-31.R56    BE57     MR90015-31-R56
LB IC152    23C4001    90015-32.R57    8B57     MR90015-32-R57
TB IC12     23C4001    90015-33.W31    7C0E     MR90015-33-W31
TB IC11     23C4001    90015-34.W32    B203     MR90015-34-W32
MB IC39     27CX322    CH9072-4        085F     CH9072 4
MB IC33     27CX322    CH9072-5        641D     CH9072 5
MB IC35     27CX322    CH9072-6        EAE1     CH9072 6
MB IC59     27CX322    CH9072-8        AB60     CH9072 8
LB IC105    N82S147    PR88004Q        FCFC     PR88004Q
LB IC66     N82S135    PR88004W        20C8     PR88004W
LB IC117    N82S185    PR90015A        3326     PR90015A
LB IC153    N82S135    PR90015B        1E52     PR90015B

Notes:  TB - Top board     (audio & I/O)       GP-9190A EB90015-20039-1
        MB - Middle board  (GFX)               GP-9189  EB90015-20038
        LB - Lower board   (CPU/GFX)           GP-9188A EB90015-20037-1

         * - These ROMs are found twice on the PCB
           - There are two linked boards per cabinet (two player cabinet)
             (attract mode displays across both monitors)

Brief hardware overview:
------------------------

Main processor   - 68000
                 - program ROMs: 9188A-22.V20 (odd), 9188A-27.V20 (even) bank 1
                                 9188A-21.V10 (odd), 9188A-26.V10 (even) bank 2
                 - Processor RAM 2x LH52250AD-90L (32kx8 SRAM)

Slave processor1 - 68000
                 - program ROMs: 9188A-11.V10 (odd), 9188A-16.V10 (even)
                 - Processor RAM 2x LH5168D-10L (8kx8 SRAM)
                 - CS90015-04 (64 pin PQFP)  + 2x MCM2018AN45 (2kx8 SRAM)
                 - GS90015-05 (100 pin PQFP) + 2x MCM2018AN45 (2kx8 SRAM)
                 - uses ROMs: 90015-08.W14, 90015-07.W12, 90015-06.W11
                              90015-05.W10, 90015-01.W06, 90015-02.W07
                              90015-03.W08, 90015-04.W09

Slave processor2 - 68000
                 - Program ROMs: 9188A-1.V10 (odd), 9188A-6.V10 (even)
                 - Processor RAM 2x LH5168D-10L (8kx8 SRAM)
                 - CS90015-04 (64 pin PQFP)  + 2x MCM2018AN45 (2kx8 SRAM)
                 - GS90015-05 (100 pin PQFP) + 2x MCM2018AN45 (2kx8 SRAM)
                 - uses ROMs: 90015-01.W06, 90015-02.W07, 90015-03.W08
                              90015-10.W15, 90015-09.W13, 90015-04.W09

Sound processor  - 68000
                 - Program ROMs: 9190A-1.V11 (odd), 9190A-2.V11 (even)
                 - Processor RAM 2x LH52250AD-90L (32kx8 SRAM)
                 - M6295,  uses ROM  90015-34.W32
                 - M6295,  uses ROM  90015-33.W31
                 - YM2151

GFX & Misc       - GS90015-02 (100 pin PQFP),  uses ROM 90015-31-R56
                   GS-9000406 (80 pin PQFP)  + 2x LH5168D-10L (8kx8 SRAM)

                 - GS90015-02 (100 pin PQFP),  uses ROM 90015-32-R57
                   GS-9000406 (80 pin PQFP)  + 2x LH5168D-10L (8kx8 SRAM)

                 - GS90015-02 (100 pin PQFP),  uses ROM 9188A-30-V10
                   GS-9000406 (80 pin PQFP)  + 2x LH5168D-10L (8kx8 SRAM)

                 - GS900151   (44 pin PQFP) (too small for the full part No.)
             3x  - GS90015-03 (80 pin PQFP)  + 2x LH52258D-45 (32kx8 SRAM)
             2x  - GS90015-06 (100 pin PQFP) + 2x LH52250AD-90L (32kx8 SRAM)
                 - GS90015-07 (64 pin PQFP)
                 - GS90015-08 (64 pin PQFP)
                 - GS90015-09 (64 pin PQFP)  + 2x MCM2018AN45 (2kx8 SRAM)
                 - GS90015-10 (64 pin PQFP)
                 - GS90015-12 (80 pin PQFP)  + 2x MCM2018AN45 (2kx8 SRAM)
                 - GS90015-11 (100 pin PQFP)
                   uses ROMs 90015-30-R55, 90015-25-R50, 90015-24-R49
                             90015-29-R54, 90015-23-R48, 90015-22-R47
                             90015-28-R53, 90015-21-R46, 90015-27-R52
                             90015-26-R51

***************************************************************************/

ROM_START( f1gpstar )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "9188a-27.v20", 0x000000, 0x040000, CRC(0a9d3896) SHA1(5e3332a1b779dead1e4f9ef274a2f168721db0ed) )
	ROM_LOAD16_BYTE( "9188a-22.v20", 0x000001, 0x040000, CRC(de15c9ca) SHA1(f356b02ca66b7e8ab0293e6e28fcd3f7996c80c8) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )
	/* Should Use ROMs:	90015-01.W06, 90015-02.W07, 90015-03.W08, 90015-04.W09 */
	ROM_LOAD16_BYTE( "9188a-16.v10",  0x000000, 0x020000, CRC(ef0f7ca9) SHA1(98ad687fdab67dd9f54b50cf21fd10ac34b61e7a) )
	ROM_LOAD16_BYTE( "9188a-11.v10",  0x000001, 0x020000, CRC(de292ea3) SHA1(04ed19045edb4edfff2b8fedac37c4a3352dfa76) )

	ROM_REGION( 0x80000, REGION_CPU3, 0 )
	/* Should Use ROMs:	90015-01.W06, 90015-02.W07, 90015-03.W08, 90015-04.W09 */
	ROM_LOAD16_BYTE( "9188a-6.v10",  0x000000, 0x020000, CRC(18ba0340) SHA1(e46e10a350f18cf3a46c0d3a0cb08fc369fced6d) )
	ROM_LOAD16_BYTE( "9188a-1.v10",  0x000001, 0x020000, CRC(109d2913) SHA1(e117556481e801d51b8526a143bc202dda222f7f) )

	ROM_REGION( 0x40000, REGION_CPU4, 0 )
	ROM_LOAD16_BYTE( "9190a-2.v11", 0x000000, 0x020000, CRC(acb2fd80) SHA1(bbed505ce745490ae11df8efdd3633181cfd4dec) )
	ROM_LOAD16_BYTE( "9190a-1.v11", 0x000001, 0x020000, CRC(7cccadaf) SHA1(d1b79fbd0e27e8d479ef533fa00b18d1f2982dda) )

	ROM_REGION16_BE( 0x80000, REGION_USER1, 0 )	/* second halves of program ROMs */
	ROM_LOAD16_BYTE( "9188a-26.v10", 0x000000, 0x040000, CRC(0b76673f) SHA1(cf29333ffb51250ae2d5363d612260f536cd15af) )	/* cpu #1*/
	ROM_LOAD16_BYTE( "9188a-21.v10", 0x000001, 0x040000, CRC(3e098d77) SHA1(0bf7e8ca36086a7ae3d44a10b4ca43f869403eb0) )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "90015-31.r56",  0x000000, 0x080000, CRC(0c8f0e2b) SHA1(6b0917a632c6beaca018146b6be66a3561b863b3) ) /* scroll 0*/

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "90015-32.r57",  0x000000, 0x080000, CRC(9c921cfb) SHA1(006d4af6dbbc34bee05f3620ba0a947a568a2400) ) /* scroll 1*/

	ROM_REGION( 0x020000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "9188a-30.v10",  0x000000, 0x020000, CRC(0ef1fbf1) SHA1(28fa0b677e70833954a5fc2fdce233d0dec4f43c) ) /* scroll 2*/

	ROM_REGION( 0x500000, REGION_GFX4, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD16_BYTE( "90015-21.r46",  0x000000, 0x080000, CRC(6f30211f) SHA1(aedba39fc6aab7847a3a2314e152bc00615cbd72) )
	ROM_LOAD16_BYTE( "90015-22.r47",  0x000001, 0x080000, CRC(05a9a5da) SHA1(807c43c3ee76bce8e4874fa51d2453917b1e4f3b) )
	ROM_LOAD16_BYTE( "90015-23.r48",  0x100000, 0x080000, CRC(58e9c6d2) SHA1(b81208819dbc5887183855001c72d0d91d32fc4b) )
	ROM_LOAD16_BYTE( "90015-24.r49",  0x100001, 0x080000, CRC(abd6c91d) SHA1(ccbf47a37008a0ec64d7058225e6ba991b559a39) )
	ROM_LOAD16_BYTE( "90015-25.r50",  0x200000, 0x080000, CRC(7ded911f) SHA1(d0083c17266f03f70f2d4b2953237fed0cb0696c) )
	ROM_LOAD16_BYTE( "90015-26.r51",  0x200001, 0x080000, CRC(18a6c663) SHA1(b39cbb4b6d09150c7d1a8cf2cd3a96b61c265d83) )
	ROM_LOAD16_BYTE( "90015-27.r52",  0x300000, 0x080000, CRC(7378c82f) SHA1(3e65064a36393b5d6ecb118a560f3fccc5b3c3c2) )
	ROM_LOAD16_BYTE( "90015-28.r53",  0x300001, 0x080000, CRC(9944dacd) SHA1(722a0c152ef97830d5ab6251d5447293d951261f) )
	ROM_LOAD16_BYTE( "90015-29.r54",  0x400000, 0x080000, CRC(2cdec370) SHA1(9fd8e8d6783a6c820d1f580a8872b5cc59641aa9) )
	ROM_LOAD16_BYTE( "90015-30.r55",  0x400001, 0x080000, CRC(47e37604) SHA1(114eb01d3258bf481c01a8378f5f08b2bdeffbba) )

	ROM_REGION( 0x200000, REGION_GFX5, ROMREGION_DISPOSE )
	ROM_LOAD( "90015-05.w10",  0x000000, 0x080000, CRC(8eb48a23) SHA1(e394eb013dd1fdc1c30616ce356bebd187453d08) ) /* Road 0*/
	ROM_LOAD( "90015-06.w11",  0x080000, 0x080000, CRC(32063a68) SHA1(587d35edec2755df11f4d63ff7bfd134a0f9fb36) )
	ROM_LOAD( "90015-07.w12",  0x100000, 0x080000, CRC(0d0d54f3) SHA1(8040945ea8f9487f0527140c90d6a66965c27ff4) )
	ROM_LOAD( "90015-08.w14",  0x180000, 0x080000, CRC(f48a42c5) SHA1(5caf50fbde682d7d1e4ec0cceacf0db7682b72a9) )

	ROM_REGION( 0x100000, REGION_GFX6, ROMREGION_DISPOSE )
	ROM_LOAD( "90015-09.w13",  0x000000, 0x080000, CRC(55f49315) SHA1(ad338cb53149ccea2dbe5ad890433c9f09a8211c) ) /* Road 1*/
	ROM_LOAD( "90015-10.w15",  0x080000, 0x080000, CRC(678be0cb) SHA1(3857b549170b62b29644cf5ebdd4aac1afa9e420) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* samples */
	ROM_LOAD( "90015-34.w32", 0x000000, 0x080000, CRC(2ca9b062) SHA1(c01b8020b409d826c0ae69c153fdc5d89241771e) ) /* 2 x 0x40000*/

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* samples */
	ROM_LOAD( "90015-33.w31", 0x000000, 0x080000, CRC(6121d247) SHA1(213c7c45bc3d57c09778b1d58dbb5fe26d0b2477) ) /* 2 x 0x40000*/

	ROM_REGION( 0x80000, REGION_USER2, 0 )		/* ? Unused ROMs ? */
/* "I know that one of the ROM images in the archive looks bad (90015-04.W09)*/
/*  however, it is good as far as I can tell. There were two of those ROMs*/
/* (soldered) onto the board and I checked them both against each other. "*/

	ROM_LOAD( "90015-04.w09",  0x000000, 0x080000, CRC(5b324c81) SHA1(ce61f2ea29086a74bdcf9f4df8e2edb749e41da5) )	/* x 2 xxxxxxxxx0xxxxxxxxx = 0x00*/
	ROM_LOAD( "90015-03.w08",  0x000000, 0x080000, CRC(ccf5b158) SHA1(06250762646e0da1fb71fd7b638492eaab3f5b7f) )	/* x 2 FIXED BITS (000x000x)*/
	ROM_LOAD( "90015-02.w07",  0x000000, 0x080000, CRC(fcbecc9b) SHA1(0670c276730ee282ef8c9599c00571b8d97725ab) )	/* x 2*/
	ROM_LOAD( "90015-01.w06",  0x000000, 0x080000, CRC(ce4bfe6e) SHA1(d428eb3d5da3bd080957c585c5b72b94a7849fca) )	/* x 2 FIXED BITS (000x000x)*/

	ROM_LOAD( "90015-20.r45",  0x000000, 0x080000, CRC(9d428fb7) SHA1(02f72938d73db932bd217620a175a05215f6016a) ) /* x 2*/

	ROM_LOAD( "ch9072-4",  0x000000, 0x001000, CRC(5bc23535) SHA1(2fd1b7184175c416b19e6570de7ecb0d897deb9a) )	/* FIXED BITS (0000000x)*/
	ROM_LOAD( "ch9072-5",  0x000000, 0x001000, CRC(0efac5b4) SHA1(a3e945aaf142bb62e0e791b8ca49a34891f31077) )	/* FIXED BITS (xxxx0xxx)*/
	ROM_LOAD( "ch9072-6",  0x000000, 0x001000, CRC(76ff63c5) SHA1(652754533cc14773f4d7590a65183349eed9eb62) )
	ROM_LOAD( "ch9072-8",  0x000000, 0x001000, CRC(ca04bace) SHA1(3771ef4bf7983e97e3346309fcb0271e17a6d359) )	/* FIXED BITS (0xxx0xxx)*/

	ROM_LOAD( "pr88004q",  0x000000, 0x000200, CRC(9327dc37) SHA1(cfe7b144cdcd76170d47f1c4e0f72b6d4fca0c8d) )	/* FIXED BITS (1xxxxxxx1111x1xx)*/
	ROM_LOAD( "pr88004w",  0x000000, 0x000100, CRC(3d648467) SHA1(bf8dbaa2176c801f7370313425c87f0eefe8a3a4) )	/* FIXED BITS (00xxxxxx)*/

	ROM_LOAD( "pr90015a",  0x000000, 0x000800, CRC(777583db) SHA1(8fd060a68fbb6156feb55afcfc5afd95999a8a62) )	/* FIXED BITS (00000xxx0000xxxx)*/
	ROM_LOAD( "pr90015b",  0x000000, 0x000100, CRC(be240dac) SHA1(6203b73c1a5e09e525380a78b555c3818929d5eb) )	/* FIXED BITS (000xxxxx000xxxx1)*/
ROM_END

DRIVER_INIT( f1gpstar )
{
	/* Split ROMs */
	rom_1 = (data16_t *) memory_region(REGION_USER1);

	cischeat_untangle_sprites(REGION_GFX4);
}


/***************************************************************************

							F1 GrandPrix Star II

This game uses the same bottom and middle boards as Grand Prix Star, however the top board
(sound + I/O) is different (though I guess it has the same purpose.)
The game is mostly a simple ROM swap on a F1 Grand Prix Star board.
The swapped ROMs have been factory socketed. Some ROMs are soldered-in and match some in the
F1 Grand Prix Star archive (i.e. they are not swapped).

Top Board
---------
PCB No: WP-92116 EB92020-20053
CPUs  : MC68000P10, TMP68000P-12
SOUND : OKI M6295 (x2), YM2151, YM3012
XTALs : 4.000MHz, 7.000MHz, 24.000MHz
DIPs  : 8 position (x3)
RAM   : MCM2018AN45 (x4, 2kx8 SRAM), LH5168D-10L (x2, 8kx8 SRAM), HM62256ALSP-10 (x2, 32kx8 SRAM)
CUSTOM: GS90015-12 (QFP80)
        GS90015-03 (QFP80)
ROMs  :
        (extension = IC location on PCB)
        92116-1.4     27c040   \
        92116-2.18    27c040   |
        92116-3.37    27c010   |  Near MC68000P10 and YM2151
        92116-4.38    27c010   /

        92116-5.116   27c2001  \
        92116-6.117   27c2001  /  Near TMP68000P-12

Notes : Labels on all these ROMs say Ver 1.0



Middle Board
------------
PCB No: GP-9189 EB90015-90038
RAM   : HM62256ALSP-12 (x8, 32kx8 SRAM), MCM2018AN45 (x2, 2kx8 SRAM)
CUSTOM: GS90015-11 (QFP100)
        GS90015-08 (QFP64)
        GS90015-07 (QFP64)
        GS90015-10 (QFP64)
        GS90015-09 (QFP64)
        GS90015-03 (x2, QFP80)
        GS90015-06 (x2, QFP100)
ROMs  :
        (extension = IC location on PCB)
        92021-11.1    4M MASK   \
        92021-12.2       "      |
        92021-13.11      "      |  Socketed + Swapped
        92021-14.12      "      |
        92021-15.21      "      |
        92021-16.22      "      /

        90015-35.54   4M MASK   \  Matches 90015-20.R45 from f1gpstar archive! Mis-labelled filename?)
        90015-35.67      "      /  Soldered-in, not swapped. Both are identical, near GS90015-08

PROMs :
        (extension = IC location on PCB)
        CH9072_4.39 \
        CH9072_5.33 |  unusual type ICT 27CX322 with a clear window (re-programmable!), purpose = ?
        CH9072_6.35 |
        CH9072_8.59 /

        PR88004W.66    type 82s135, near 90015-35


Bottom Board
------------
PCB No: GP-9188A  EB90015-20037-1
CPU   : TMP68000P-12 (x3)
XTAL  : 24.000MHz
RAM   : HM62256ALSP-10 (x2, 32kx8 SRAM), LH5168D-10L (x10, 8kx8 SRAM)
        MCM2018AN45 (x8, 2kx8 SRAM), PDM41256SA20P (x2, 32kx8 SRAM)
CUSTOM: GS90015-03 (QFP80)
        GS9000406  (x3, QFP80)
        GS90015-02 (x3, QFP100)
        GS90015-01 (QFP44)
        GS90015-05 (x2, QFP100)
        GS90015-04 (x2, QFP64)

ROMs  :
        (extension = IC location on PCB)
        92021-01.14   4M MASK   \                                       \
        92021-02.15      "      |                                       |
        92021-03.16      "      |  Socketed + Swapped                   |
        92021-04.17      "      /                                       |
        90015-01.37   4M MASK   \                                       |
        90015-02.38      "      |   Soldered-in, not swapped, matches   |
        90015-03.39      "      |   same ROMs in F1 Grand Prix archive  |
        90015-04.40      "      /                                       | All these are
        92021-05.61   4M MASK   \                                       | grouped together
        92021-06.62      "      |                                       |
        92021-07.63      "      |  Socketed + Swapped                   |
        92021-08.64      "      /                                       |
        90015-01.79   4M MASK   \                                       |
        90015-02.80      "      |  Soldered-in, not swapped, matches    |
        90015-03.81      "      |  same ROMs in F1 Grand Prix archive   |
        90015-04.82      "      /                                       /

        9188A-25.123  27c040    Label says Ver 1.0   \
        9188A-28.152     "      Label says Ver 4.0   |  Near GS90015-02
        9188A-30.174  27c1001   Label says Ver 1.0   /

        9188A-21.91   27c2001   Label says Ver 4.0   \
        9188A-22.92      "      Label says Ver 4.0   |  Near a 68000
        9188A-26.124     "      Label says Ver 4.0   |
        9188A-27.125     "      Label says Ver 4.0   /

        9188A-11.46   27c1001   Label says Ver 1.0   \  Near a 68000
        9188A-16.70      "      Label says Ver 1.0   /

        9188A-1.2     27c1001   Label says Ver 1.0   \  Near a 68000
        9188A-6.27       "      Label says Ver 1.0   /

PROMs :
        (extension = IC location on PCB)
        PR90015A.117   type 82s185, near GS9000406
        PR90015B.153   type 82s135, near 9188A-26.124
        PR88004Q.105   type 82s147, near GS90015-01


***************************************************************************/

ROM_START( f1gpstr2 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "9188a-27.125", 0x000000, 0x040000, CRC(ee60b894) SHA1(92247cd3b0e3fb2ed0ad27062d1cc13dadb21465) )
	ROM_LOAD16_BYTE( "9188a-22.92" , 0x000001, 0x040000, CRC(f229332b) SHA1(f7037515d77a1f42ce555b8baa15075d7009a5c6) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )
	ROM_LOAD16_BYTE( "9188a-16.70",  0x000000, 0x020000, CRC(3b3d57a0) SHA1(2ad367fc938c385160a014b994fe45d1791d4ddb) )
	ROM_LOAD16_BYTE( "9188a-11.46",  0x000001, 0x020000, CRC(4d2afddf) SHA1(108ddbb6a008d390e43d88efd10d99b599c1d75d) )

	ROM_REGION( 0x80000, REGION_CPU3, 0 )
	ROM_LOAD16_BYTE( "9188a-6.27",  0x000000, 0x020000, CRC(68faa23b) SHA1(b18c407cc1273295ec9fc30af191fccd94447ae8) )
	ROM_LOAD16_BYTE( "9188a-1.2",   0x000001, 0x020000, CRC(e4d83b4f) SHA1(4022521e43f6361c5e04a604f8a7fa1e60a2ac99) )

	ROM_REGION( 0x40000, REGION_CPU4, 0 )
	ROM_LOAD16_BYTE( "92116-3.37", 0x000000, 0x020000, CRC(2a541095) SHA1(934ef9b6bbe3f6e2e2649dde5671547b955ffc7c) )
	ROM_LOAD16_BYTE( "92116-4.38", 0x000001, 0x020000, CRC(70da1825) SHA1(7f2077e9b40d5acf4da3f6bcc5d7b92d6d08f861) )

	ROM_REGION( 0x80000, REGION_CPU5, 0 )
	ROM_LOAD16_BYTE( "92116-5.116",  0x000000, 0x040000, CRC(da16db49) SHA1(a07fb706b0c93a83148a9fdaaca1bc5414bfe286) )
	ROM_LOAD16_BYTE( "92116-6.117",  0x000001, 0x040000, CRC(1f1e147a) SHA1(ebedcdad9cfda8fa3b5c2653232209da5be237e1) )

	ROM_REGION16_BE( 0x80000, REGION_USER1, 0 )	/* second halves of program ROMs */
	ROM_LOAD16_BYTE( "9188a-26.124", 0x000000, 0x040000, CRC(8690bb79) SHA1(8ef822cf8371cb209c30cfe5c4d5e8b36392f732) )	/* cpu1*/
	ROM_LOAD16_BYTE( "9188a-21.91",  0x000001, 0x040000, CRC(c5a5807e) SHA1(15493030d154579d2095c7304dd843aed09a69ec) )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "9188a-25.123",  0x000000, 0x080000, CRC(000c83d2) SHA1(656e4553873ad945bbd770166cc3add287e525dd) ) /* scroll 0*/

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "9188a-28.152",  0x000000, 0x080000, CRC(e734f8ea) SHA1(efed2ce4a23d16a38872892c25fa9824ca0fed8e) ) /* scroll 1*/

	ROM_REGION( 0x020000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "9188a-30.174",  0x000000, 0x020000, CRC(c5906023) SHA1(6006c86995abef1442232ff5fbd68c2a37038d1f) ) /* scroll 2*/

	ROM_REGION( 0x600000, REGION_GFX4, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD16_BYTE( "92021-11.1",  0x000000, 0x100000, CRC(fec883a7) SHA1(a17ca17fa37b35c5f28cede52efa91afe566a95a) )
	ROM_LOAD16_BYTE( "92021-12.2",  0x000001, 0x100000, CRC(df283a7e) SHA1(4e9eed9475186e6f94d9ef84a798fa807fb37903) )
	ROM_LOAD16_BYTE( "92021-13.11", 0x200000, 0x100000, CRC(1ceb593a) SHA1(e8428c42d10aa0d26717176b1bdea9a4a1d3e5f3) )
	ROM_LOAD16_BYTE( "92021-14.12", 0x200001, 0x100000, CRC(673c2c61) SHA1(5e04ac452ffd0747beaa42daca873a09cf179b18) )
	ROM_LOAD16_BYTE( "92021-15.21", 0x400000, 0x100000, CRC(66e8b197) SHA1(10f6c5beb4ab57fbd1940db0e15d07df486e4351) )
	ROM_LOAD16_BYTE( "92021-16.22", 0x400001, 0x100000, CRC(1f672dd8) SHA1(f75b8f3f9512e2ef085170888e621f54ee94f9d5) )


	ROM_REGION( 0x200000, REGION_GFX5, ROMREGION_DISPOSE )
	ROM_LOAD( "92021-08.64",  0x000000, 0x080000, CRC(54ff00c4) SHA1(f86f16c77b211206fbe39efa278634db8a3eaf75) ) /* Road 0*/
	ROM_LOAD( "92021-07.63",  0x080000, 0x080000, CRC(258d524a) SHA1(f2ba03b7fec81377b032476703cafe0fe79f6a2a) )
	ROM_LOAD( "92021-06.62",  0x100000, 0x080000, CRC(f1423efe) SHA1(bd45ba2b7908d10dc4df10b9c04dca6830894d2a) )
	ROM_LOAD( "92021-05.61",  0x180000, 0x080000, CRC(88bb6db1) SHA1(54413c41a4d02137aebc2a4866a38aadfe64825a) )

	ROM_REGION( 0x200000, REGION_GFX6, ROMREGION_DISPOSE )
	ROM_LOAD( "92021-04.17",  0x000000, 0x080000, CRC(3a2e6b1e) SHA1(350465ade24c16e4fe39613f89bf3e7277cdd31e) ) /* Road 1*/
	ROM_LOAD( "92021-03.16",  0x080000, 0x080000, CRC(1f041f65) SHA1(cc4defe3675b30e7de4b3e1eb580a71af4c36bc6) )
	ROM_LOAD( "92021-02.15",  0x100000, 0x080000, CRC(d0582ad8) SHA1(b343a6525bb9d7dbb288ddec392b23e85ae150bb) )
	ROM_LOAD( "92021-01.14",  0x180000, 0x080000, CRC(06e50be4) SHA1(60ced74d97ac5f641b7e721484abbe74522fe3ba) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* samples */
	ROM_LOAD( "92116-2.18", 0x000000, 0x080000, CRC(8c06cded) SHA1(789d620dddfebc1343d424fccd03dd51b58c50fa) )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* samples */
	ROM_LOAD( "92116-1.4",  0x000000, 0x080000, CRC(8da37b9d) SHA1(ffc67901d087e63bfbb36d15d75c57f6847ef6ea) )

	ROM_REGION( 0x80000, REGION_USER2, 0 )		/* ? Unused ROMs ? */
	ROM_LOAD( "90015-04.17",  0x000000, 0x080000, CRC(5b324c81) SHA1(ce61f2ea29086a74bdcf9f4df8e2edb749e41da5) )	/* x 2 xxxxxxxxx0xxxxxxxxx = 0x00*/
	ROM_LOAD( "90015-04.82",  0x000000, 0x080000, CRC(5b324c81) SHA1(ce61f2ea29086a74bdcf9f4df8e2edb749e41da5) )	/**/
	ROM_LOAD( "90015-03.16",  0x000000, 0x080000, CRC(ccf5b158) SHA1(06250762646e0da1fb71fd7b638492eaab3f5b7f) )	/* x 2 FIXED BITS (000x000x)*/
	ROM_LOAD( "90015-03.81",  0x000000, 0x080000, CRC(ccf5b158) SHA1(06250762646e0da1fb71fd7b638492eaab3f5b7f) )	/**/
	ROM_LOAD( "90015-02.15",  0x000000, 0x080000, CRC(fcbecc9b) SHA1(0670c276730ee282ef8c9599c00571b8d97725ab) )	/* x 2*/
	ROM_LOAD( "90015-02.80",  0x000000, 0x080000, CRC(fcbecc9b) SHA1(0670c276730ee282ef8c9599c00571b8d97725ab) )	/**/
	ROM_LOAD( "90015-01.14",  0x000000, 0x080000, CRC(ce4bfe6e) SHA1(d428eb3d5da3bd080957c585c5b72b94a7849fca) )	/* x 2 FIXED BITS (000x000x)*/
	ROM_LOAD( "90015-01.79",  0x000000, 0x080000, CRC(ce4bfe6e) SHA1(d428eb3d5da3bd080957c585c5b72b94a7849fca) )	/**/

	ROM_LOAD( "90015-35.54",  0x000000, 0x080000, CRC(9d428fb7) SHA1(02f72938d73db932bd217620a175a05215f6016a) ) /* x 2*/
	ROM_LOAD( "90015-35.67",  0x000000, 0x080000, CRC(9d428fb7) SHA1(02f72938d73db932bd217620a175a05215f6016a) ) /**/

	ROM_LOAD( "ch9072_4.39",  0x000000, 0x002000, CRC(b45b4dc0) SHA1(b9fae0c9ac2d40f0a202c538d866d5f2941ba8dd) )	/* FIXED BITS (0000000x)*/
	ROM_LOAD( "ch9072_5.33",  0x000000, 0x002000, CRC(e122916b) SHA1(86d5ecc7ecc6f175ecb28459697ef33e1ee06860) )	/* FIXED BITS (xxxx0xxx)*/
	ROM_LOAD( "ch9072_6.35",  0x000000, 0x002000, CRC(05d95bf7) SHA1(78181cf71f22c090a1e62823a43757353a9ef6ab) )
	ROM_LOAD( "ch9072_8.59",  0x000000, 0x002000, CRC(6bf52596) SHA1(bf4e7e7df3daae4aa6a441b58b15a435aa45630e) )	/* FIXED BITS (0xxx0xxx)*/

	ROM_LOAD( "pr88004q.105", 0x000000, 0x000200, CRC(9327dc37) SHA1(cfe7b144cdcd76170d47f1c4e0f72b6d4fca0c8d) )	/* FIXED BITS (1xxxxxxx1111x1xx)*/
	ROM_LOAD( "pr88004w.66",  0x000000, 0x000100, CRC(3d648467) SHA1(bf8dbaa2176c801f7370313425c87f0eefe8a3a4) )	/* FIXED BITS (00xxxxxx)*/

	ROM_LOAD( "pr90015a.117", 0x000000, 0x000800, CRC(777583db) SHA1(8fd060a68fbb6156feb55afcfc5afd95999a8a62) )	/* FIXED BITS (00000xxx0000xxxx)*/
	ROM_LOAD( "pr90015b.153", 0x000000, 0x000100, CRC(be240dac) SHA1(6203b73c1a5e09e525380a78b555c3818929d5eb) )	/* FIXED BITS (000xxxxx000xxxx1)*/
ROM_END


/***************************************************************************

								Scud Hammer

CF-92128B:

                                                      GS9001501
 2-H 2-L  6295            62256 62256
 1-H 1-L  6295  68000-12  3     4       6  GS90015-02 8464 8464 GS600406

                    24MHz               5  GS90015-02 8464 8464 GS900406

                                                       7C199
                                                       7C199 GS90015-03

GP-9189:

 1     2      62256                            62256
 3     4      62256    GS90015-06 GS90015-06   62256
 5     6      62256                            62256
 7     8      62256    GS90015-03 GS90015-03   62256
 9     10

                  GS90015-08            GS90015-07 GS90015-10

          GS90015-11


                      MR90015-35
                      MR90015-35              GS90015-09

***************************************************************************/

ROM_START( scudhamm )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )		/* Main CPU Code */
	ROM_LOAD16_BYTE( "3", 0x000000, 0x040000, CRC(a908e7bd) SHA1(be0a8f959ab5c19122eee6c3def6137f37f1a9c6) )
	ROM_LOAD16_BYTE( "4", 0x000001, 0x040000, CRC(981c8b02) SHA1(db6c8993bf1c3993ab31dd649022ab76169975e1) )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE ) /* Scroll 0 */
	ROM_LOAD( "5", 0x000000, 0x080000, CRC(714c115e) SHA1(c3e88b3972e3926f37968f3e84b932e1ac177142) )

/*	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )  // Scroll 1 /*/
/*	UNUSED*/

	ROM_REGION( 0x020000, REGION_GFX3, ROMREGION_DISPOSE ) /* Scroll 2 */
	ROM_LOAD( "6", 0x000000, 0x020000, CRC(b39aab63) SHA1(88275cce8b1323b2d835390a8fc2380b90d50d95) ) /* 1xxxxxxxxxxxxxxxx = 0xFF*/

	ROM_REGION( 0x500000, REGION_GFX4, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD16_BYTE( "1.bot",  0x000000, 0x080000, CRC(46450d73) SHA1(c9acdf1cef760e5194c346d721e859c61afbfce6) )
	ROM_LOAD16_BYTE( "2.bot",  0x000001, 0x080000, CRC(fb7b66dd) SHA1(ad6bbae4fa72f957e5c0fc7bf6199ac45f837dac) )
	ROM_LOAD16_BYTE( "3.bot",  0x100000, 0x080000, CRC(7d45960b) SHA1(abf59cf85f28c90d4c08e3a1e5408a9a700071cc) )
	ROM_LOAD16_BYTE( "4.bot",  0x100001, 0x080000, CRC(393b6a22) SHA1(0d002a8c09de2fb8aaa7f5f020badc6fc096fa41) )
	ROM_LOAD16_BYTE( "5.bot",  0x200000, 0x080000, CRC(7a3c33ad) SHA1(fe0e3722e15919ae3acfeeacae57716aae43647c) )
	ROM_LOAD16_BYTE( "6.bot",  0x200001, 0x080000, CRC(d19c4bf7) SHA1(b8aa21920d5a02f10a7ae65ade8a0a88ad23f373) )
	ROM_LOAD16_BYTE( "7.bot",  0x300000, 0x080000, CRC(9e5edf59) SHA1(fcd4136e39d40bcce365153c96e06181a24a480a) )
	ROM_LOAD16_BYTE( "8.bot",  0x300001, 0x080000, CRC(4980051e) SHA1(10de91239b5b4dab8e7fa4bf51d93356c5111ddf) )
	ROM_LOAD16_BYTE( "9.bot",  0x400000, 0x080000, CRC(c1b301f1) SHA1(776b9889703d73afc4fb0ff77498b98c943246d3) )
	ROM_LOAD16_BYTE( "10.bot", 0x400001, 0x080000, CRC(dab4528f) SHA1(f5ddc37a2d106d5438ad1b7d23a2bbbce07f2c89) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )		/* Samples (4x40000) */
	ROM_LOAD( "2.l",  0x000000, 0x080000, CRC(889311da) SHA1(fcaee3e6c98a784cfde06fc2e0e8f5abbfb4df6c) )
	ROM_LOAD( "2.h",  0x080000, 0x080000, CRC(347928fc) SHA1(36903c38b9f13594de40dfc697326327c7010d65) )

	ROM_REGION( 0x100000, REGION_SOUND2, 0 )		/* Samples (4x40000) */
	ROM_LOAD( "1.l",  0x000000, 0x080000, CRC(3c94aa90) SHA1(f9278fec9d93dac0309f30e35c727bd481f347d4) )
	ROM_LOAD( "1.h",  0x080000, 0x080000, CRC(5caee787) SHA1(267f4d3c28e71e53180a5d0ff27a6555ac6fa4a0) )	/* 1xxxxxxxxxxxxxxxxxx = 0xFF*/
ROM_END


/***************************************************************************


								Game Drivers


***************************************************************************/

GAMEX( 1989, bigrun,   0, bigrun,   bigrun,   bigrun,   ROT0,   "Jaleco", "Big Run (11th Rallye version)", GAME_IMPERFECT_GRAPHICS )	/* there's a 13th Rallye version (1991)*/
GAMEX( 1990, cischeat, 0, cischeat, cischeat, cischeat, ROT0,   "Jaleco", "Cisco Heat",                    GAME_IMPERFECT_GRAPHICS )
GAMEX( 1991, f1gpstar, 0, f1gpstar, f1gpstar, f1gpstar, ROT0,   "Jaleco", "Grand Prix Star",               GAME_IMPERFECT_GRAPHICS )
GAMEX( 1993, f1gpstr2, 0, f1gpstr2, f1gpstar, f1gpstar, ROT0,   "Jaleco", "F-1 Grand Prix Star II",        GAME_IMPERFECT_GRAPHICS )
GAMEX( 1994, scudhamm, 0, scudhamm, scudhamm, 0,        ROT270, "Jaleco", "Scud Hammer",                   GAME_IMPERFECT_GRAPHICS )
