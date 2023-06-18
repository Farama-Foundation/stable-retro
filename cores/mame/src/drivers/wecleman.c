/***************************************************************************
						WEC Le Mans 24  &   Hot Chase

						  (C)   1986 & 1988 Konami

					driver by       Luca Elia (l.elia@tin.it)

- Note: press F2 to enter service mode -


----------------------------------------------------------------------
Hardware                Main    Sub             Sound   Sound Chips
----------------------------------------------------------------------
[WEC Le Mans 24]        68000   68000   Z-80    YM2151 YM3012 1x007232

[Hot Chase]             68000   68000   68B09E                3x007232

[CPU PCB GX763 350861B]
	007641  007770  3x007232  051550

[VID PCB GX763 350860A AI AM-1]
	007634  007635  3x051316  007558  007557
----------------------------------------------------------------------


----------------------------------------------------------------
Main CPU                     [WEC Le Mans 24]     [Hot Chase]
----------------------------------------------------------------
ROM                R         000000-03ffff        <
Work RAM           RW        040000-043fff        040000-063fff*
?                  RW        060000-060007        -
Blitter             W        080000-080011        <
Page RAM           RW        100000-103fff        -
Text RAM           RW        108000-108fff        -
Palette RAM        RW        110000-110fff        110000-111fff**
Shared RAM         RW        124000-127fff        120000-123fff
Sprites RAM        RW        130000-130fff        <
Input Ports        RW        1400xx-1400xx        <
Background         RW                             100000-100fff
Background Ctrl     W        -                    101000-10101f
Foreground         RW        -                    102000-102fff
Foreground Ctrl     W        -                    103000-10301f

* weird    ** only half used


----------------------------------------------------------------
Sub CPU                      [WEC Le Mans 24]     [Hot Chase]
----------------------------------------------------------------

ROM                R         000000-00ffff        000000-01ffff
Work RAM           RW        -                    060000-060fff
Road RAM           RW        060000-060fff        020000-020fff
Shared RAM         RW        070000-073fff        040000-043fff


---------------------------------------------------------------------------
								Game code
							[WEC Le Mans 24]
---------------------------------------------------------------------------

					Interesting locations (main cpu)
					--------------------------------

There's some 68000 assembly code in ASCII around d88 :-)

040000+
7-9                             *** hi score/10 (BCD 3 bytes) ***
b-d                             *** score/10 (BCD 3 bytes) ***
1a,127806               <- 140011.b
1b,127807               <- 140013.b
1c,127808               <- 140013.b
1d,127809               <- 140015.b
1e,12780a               <- 140017.b
1f                      <- 140013.b
30                              *** credits ***
3a,3b,3c,3d             <-140021.b
3a = accelerator   3b = ??   3c = steering   3d = table

d2.w                    -> 108f24 fg y scroll
112.w                   -> 108f26 bg y scroll

16c                             influences 140031.b
174                             screen address
180                             input port selection (->140003.b ->140021.b)
181                     ->140005.b
185                             bit 7 high -> must copy sprite data to 130000
1dc+(1da).w             ->140001.b

40a.w,c.w               *** time (BCD) ***
411                             EF if brake, 0 otherwise
416                             ?
419                             gear: 0=lo,1=hi
41e.w                   speed related ->127880
424.w                   speed BCD
43c.w                   accel?
440.w                   level?

806.w                   scrollx related
80e.w                   scrolly related

c08.b                   routine select: 1>1e1a4 2>1e1ec 3>1e19e other>1e288 (map screen)

117a.b                  selected letter when entering name in hi-scores
117e.w                  cycling color in hi-scores

12c0.w                  ?time,pos,len related?
12c2.w
12c4.w
12c6.w

1400-1bff               color data (0000-1023 chars)
1c00-23ff               color data (1024-2047 sprites?)

2400                    Sprite data: 40 entries x  4 bytes =  100
2800                    Sprite data: 40 entries x 40 bytes = 1000
3800                    Sprite data: 40 entries x 10 bytes =  400

					Interesting routines (main cpu)
					-------------------------------

804                     mem test
818                     end mem test (cksums at 100, addresses at A90)
82c                     other cpu test
a0c                     rom test
c1a                     prints string (a1)
1028                    end test
204c                    print 4*3 box of chars to a1, made up from 2 2*6 (a0)=0xLR (Left,Righ index)
4e62                    raws in the fourth page of chars
6020                    test screen (print)
60d6                    test screen
62c4                    motor test?
6640                    print input port values ( 6698 = scr_disp.w,ip.b,bit.b[+/-] )

819c                    prepares sprite data
8526                    blitter: 42400->130000
800c                    8580    sprites setup on map screen

1833a                   cycle cols on hi-scores
18514                   hiscores: main loop
185e8                   hiscores: wheel selects letter

TRAP#0                  prints string: A0-> addr.l, attr.w, (char.b)*, 0

IRQs                    [1,3,6]  602
IRQs                    [2,7]    1008->12dc      ORI.W    #$2700,(A7) RTE
IRQs                    [4]      1004->124c
IRQs                    [5]      106c->1222      calls sequence: $3d24 $1984 $28ca $36d2 $3e78


					Interesting locations (sub cpu)
					-------------------------------

					Interesting routines (sub cpu)
					------------------------------

1028    'wait for command' loop.
1138    lev4 irq
1192    copies E0*4 bytes: (a1)+ -> (a0)+


---------------------------------------------------------------------------
								 Game code
								[Hot Chase]
---------------------------------------------------------------------------

This game has been probably coded by the same programmers of WEC Le Mans 24
It shares some routines and there is the (hidden?) string "WEC 2" somewhere

							Main CPU                Sub CPU

Interrupts: [1, 7]          FFFFFFFF                FFFFFFFF
Interrupts: [2,3,4,5,6]     221c                    1288

Self Test:
 0] pause,120002==55,pause,120002==AA,pause,120002==CC, (on error set bit d7.0)
 6] 60000-63fff(d7.1),40000-41fff(d7.2)
 8] 40000/2<-chksum 0-20000(e/o);40004/6<-chksum 20000-2ffff(e/o) (d7.3456)
 9] chksums from sub cpu: even->40004   odd->(40006)    (d7.78)
 A] 110000-111fff(even)(d7.9),102000-102fff(odd)(d7.a)
 C] 100000-100fff(odd)(d7.b),pause,pause,pause
10] 120004==0(d7.c),120006==0(d7.d),130000-1307ff(first $A of every $10 bytes only)(d7.e),pause
14] 102000<-hw screen+(d7==0)? jmp 1934/1000
15] 195c start of game


					Interesting locations (main cpu)
					--------------------------------

60024.b                 <- !140017.b (DSW 1 - coinage)
60025.b                 <- !140015.b (DSW 2 - options)
6102c.w                 *** time ***

					Interesting routines (main cpu)
					-------------------------------

18d2                    (d7.d6)?print BAD/OK to (a5)+, jmp(D0)
1d58                    print d2.w to (a4)+, jmp(a6)
580c                    writes at 60000
61fc                    print test strings
18cbe                   print "game over"


---------------------------------------------------------------------------
								   Issues
							  [WEC Le Mans 24]
---------------------------------------------------------------------------

- The parallactic scrolling is sometimes wrong


---------------------------------------------------------------------------
								   Issues
								[Hot Chase]
---------------------------------------------------------------------------

- Samples pitch is too low
- No zoom and rotation of the layers


---------------------------------------------------------------------------
							   Common Issues
---------------------------------------------------------------------------

- One ROM unused (32K in hotchase, 16K in wecleman)
- Incomplete DSWs
- Sprite ram is not cleared by the game and no sprite list end-marker
  is written. We cope with that with an hack in the Blitter but there
  must be a register to do the trick


Revisions:

05-05-2002 David Haywood(Haze)
- improved Wec Le Mans steering

05-01-2002 Hiromitsu Shioya(Shica)
- fixed Hot Chase volume and sound interrupt

xx-xx-2003 Acho A. Tang
[Wec Le Mans 24]
- generalized blitter to support Wec Le Mans
- emulated custom alpha blending chip used for protection
- fixed game color and sound volume
- added shadows and sprite-to-sprite priority
- added curbs effect
- modified zoom equation to close tile gaps
- fixed a few tile glitches
- converted driver to use RGB direct
- cloud transition(needs verification from board owners)
- fixed sound banking
- source clean-up

TODO:
- check dust color on title screen(I don't think it should be black)
- check brake light(LED) support
- check occational off-pitch music and samples(sound interrupt related?)

* Sprite, road and sky drawings do not support 32-bit color depth.
  Certain sprites with incorrect z-value still pop in front of closer
  billboards and some appear a few pixels off the ground. They could be
  the game's intrinsic flaws. (reference: www.system16.com)

[Hot Chase]
- shared changes with Wec Le Mans
- removed junk tiles during introduction(needs verification)

* Special thanks to Luca Elia for bringing us so many enjoyable games.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/konamiic.h"
#include "cpu/m6809/m6809.h"

/* Variables only used here: */
static data16_t *sharedram, *blitter_regs;
static int multiply_reg[2];
static data16_t *wecleman_protection_ram;
static int spr_color_offs;

/* Variables that vidhrdw has acces to: */
int wecleman_selected_ip, wecleman_irqctrl;

/* Variables defined in vidhrdw: */
extern data16_t *wecleman_videostatus;
extern data16_t *wecleman_pageram, *wecleman_txtram, *wecleman_roadram;
extern size_t wecleman_roadram_size;
extern int wecleman_bgpage[4], wecleman_fgpage[4], *wecleman_gfx16_RAM;

/* Functions defined in vidhrdw: */
WRITE16_HANDLER( hotchase_paletteram16_SBGRBBBBGGGGRRRR_word_w );
WRITE16_HANDLER( wecleman_paletteram16_SSSSBBBBGGGGRRRR_word_w );
WRITE16_HANDLER( wecleman_videostatus_w );
READ16_HANDLER( wecleman_pageram_r );
WRITE16_HANDLER( wecleman_pageram_w );
READ16_HANDLER( wecleman_txtram_r );
WRITE16_HANDLER( wecleman_txtram_w );
VIDEO_UPDATE( wecleman );
VIDEO_START( wecleman );
VIDEO_UPDATE( hotchase );
VIDEO_START( hotchase );


/***************************************************************************
							Common Routines
***************************************************************************/

static READ16_HANDLER( wecleman_protection_r )
{
	int blend, data0, data1, r0, g0, b0, r1, g1, b1;

	data0 = wecleman_protection_ram[0];
	blend = wecleman_protection_ram[2];
	data1 = wecleman_protection_ram[1];
	blend &= 0x3ff;

	/* a precalculated table will take an astronomical 4096^2(colors) x 1024(steps) x 2(word) bytes*/
	r0 = data0;  g0 = data0;  b0 = data0;
	r0 &= 0xf;   g0 &= 0xf0;  b0 &= 0xf00;
	r1 = data1;  g1 = data1;  b1 = data1;
	r1 &= 0xf;   g1 &= 0xf0;  b1 &= 0xf00;
	r1 -= r0;    g1 -= g0;    b1 -= b0;
	r1 *= blend; g1 *= blend; b1 *= blend;
	r1 >>= 10;   g1 >>= 10;   b1 >>= 10;
	r0 += r1;    g0 += g1;    b0 += b1;
	g0 &= 0xf0;  b0 &= 0xf00;

	r0 |= g0;
	r0 |= b0;

	return(r0);
}

static WRITE16_HANDLER( wecleman_protection_w )
{
	static int state = 0;

	if (offset == 2) state = data & 0x2000;
	if (!state) COMBINE_DATA(wecleman_protection_ram + offset);
}


/* Data is read from and written to *sharedram* */
static READ16_HANDLER( sharedram_r )    { return sharedram[offset]; }
static WRITE16_HANDLER( sharedram_w )   { COMBINE_DATA(&sharedram[offset]); }


/* 140005.b (WEC Le Mans 24 Schematics)

 COMMAND
 ___|____
|   CK  8|--/        7
| LS273 7| TV-KILL   6
|       6| SCR-VCNT  5
|       5| SCR-HCNT  4
|   5H  4| SOUND-RST 3
|       3| SOUND-ON  2
|       2| NSUBRST   1
|       1| SUBINT    0
|__CLR___|
	|
  NEXRES

 Schems: SUBRESET does a RST+HALT
         Sub CPU IRQ 4 generated by SUBINT, no other IRQs
*/
static WRITE16_HANDLER( irqctrl_w )
{
	if (ACCESSING_LSB)
	{
		/* log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #0 - PC = %06X - $140005 <- %02X (old value: %02X)\n",activecpu_get_pc(), data&0xFF, old_data&0xFF);*/

		/* Bit 0 : SUBINT*/
		if ( (wecleman_irqctrl & 1) && (!(data & 1)) )	/* 1->0 transition*/
			cpu_set_irq_line(1,4,HOLD_LINE);

		/* Bit 1 : NSUBRST*/
		if (data & 2)   cpu_set_reset_line(1, CLEAR_LINE  );
		else                    cpu_set_reset_line(1, ASSERT_LINE );

		/* Bit 2 : SOUND-ON*/
		/* Bit 3 : SOUNDRST*/
		/* Bit 4 : SCR-HCNT*/
		/* Bit 5 : SCR-VCNT*/
		/* Bit 6 : TV-KILL*/
		wecleman_irqctrl = data;	/* latch the value*/
	}
}

/* 140003.b (usually paired with a write to 140021.b)

	Bit:

	7-------        ?
	-65-----        input selection (0-3)
	---43---        ?
	-----2--        start light
	------10        ? out 1/2

*/
static WRITE16_HANDLER( selected_ip_w )
{
	if (ACCESSING_LSB) wecleman_selected_ip = data & 0xff;	/* latch the value*/
}

/* $140021.b - Return the previously selected input port's value */
static READ16_HANDLER( selected_ip_r )
{
	switch ( (wecleman_selected_ip >> 5) & 3 )
	{												/* From WEC Le Mans Schems:*/
		case 0:  return input_port_4_r(offset);		/* Accel - Schems: Accelevr*/
		case 1:  return ~0;							/* ????? - Schems: Not Used*/
		case 2:  return input_port_5_r(offset);		/* Wheel - Schems: Handlevr*/
		case 3:  return ~0;							/* Table - Schems: Turnvr*/

		default: return ~0;
	}
}

/* Word Blitter - Copies data around (Work RAM, Sprite RAM etc.)
                  It's fed with a list of blits to do

	Offset:

	00.b            ? Number of words - 1 to add to address per transfer
	01.b            ? logic function / blit mode
	02.w            ? (always 0)
	04.l            Source address (Base address of source data)
	08.l            List of blits address
	0c.l            Destination address
	01.b            ? Number of transfers
	10.b            Triggers the blit
	11.b            Number of words per transfer

	The list contains 4 bytes per blit:


	Offset:

	00.w            ?
	02.w            offset from Base address


	Note:

	Hot Chase explicitly copies color information from sprite parameters back to list[4n+1](byte ptr)
	and that tips me off where the colors are actually encoded. List[4n+0] is believed to hold the
	sprites' depth value. Wec Le Mans will z-sort the sprites before writing them to video RAM but
	the order is not always right. It is possible the video hardware performs additional sorting.

	The color code in the original sprite encoding has special meanings on the other hand. I'll take
	a shortcut by manually copying list[0] and list[1] to sprite RAM for further process.
*/
static WRITE16_HANDLER( blitter_w )
{
	COMBINE_DATA(&blitter_regs[offset]);

	/* do a blit if $80010.b has been written */
	if ( (offset == 0x10/2) && (ACCESSING_MSB) )
	{
		/* 80000.b = ?? usually 0 - other values: 02 ; 00 - ? logic function ? */
		/* 80001.b = ?? usually 0 - other values: 3f ; 01 - ? height ? */
		int minterm  = ( blitter_regs[0x0/2] & 0xFF00 ) >> 8;
		int list_len = ( blitter_regs[0x0/2] & 0x00FF ) >> 0;

		/* 80002.w = ?? always 0 - ? increment per horizontal line ? */
		/* no proof at all, it's always 0 */
		/*int srcdisp = blitter_regs[0x2/2] & 0xFF00;*/
		/*int destdisp = blitter_regs[0x2/2] & 0x00FF;*/

		/* 80004.l = source data address */
		int src  = ( blitter_regs[0x4/2] << 16 ) + blitter_regs[0x6/2];

		/* 80008.l = list of blits address */
		int list = ( blitter_regs[0x8/2] << 16 ) + blitter_regs[0xA/2];

		/* 8000C.l = destination address */
		int dest = ( blitter_regs[0xC/2] << 16 ) + blitter_regs[0xE/2];

		/* 80010.b = number of words to move */
		int size = ( blitter_regs[0x10/2] ) & 0x00FF;

		/* Word aligned transfers only ?? */
		src  &= (~1);   list &= (~1);    dest &= (~1);

		/* Two minterms / blit modes are used */
		if (minterm != 2)
		{
			/* One single blit */
			for ( ; size > 0 ; size--)
			{
				/* maybe slower than a memcpy but safer (and errors are logged) */
				cpu_writemem24bew_word(dest,cpu_readmem24bew_word(src));
				src += 2;
				dest += 2;
			}
		}
		else
		{
			/* Number of blits in the list */
			for ( ; list_len > 0 ; list_len-- )
			{
				int i, j, destptr;

				/* Read offset of source from the list of blits */
				i = src + cpu_readmem24bew_word(list+2);
				j = i + (size<<1);
				destptr = dest;

				for (; i<j; destptr+=2, i+=2)
					cpu_writemem24bew_word(destptr, cpu_readmem24bew_word(i));

				destptr = dest + 14;
				i = cpu_readmem24bew_word(list) + spr_color_offs;
				cpu_writemem24bew_word(destptr, i);

				dest += 16;
				list += 4;
			}

			/* hack for the blit to Sprites RAM - Sprite list end-marker */
			cpu_writemem24bew_word(dest,0xFFFF);
		}
	}
}


/***************************************************************************
					WEC Le Mans 24 Main CPU Handlers
***************************************************************************/

static WRITE16_HANDLER( wecleman_soundlatch_w );

static MEMORY_READ16_START( wecleman_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM             },	/* ROM*/
	{ 0x040000, 0x043fff, MRA16_RAM             },	/* RAM*/
	{ 0x060006, 0x060007, wecleman_protection_r },	/* MCU read*/
	{ 0x080000, 0x080011, MRA16_RAM             },	/* Blitter (reading is for debug)*/
	{ 0x100000, 0x103fff, MRA16_RAM             },	/* Background Layers*/
	{ 0x108000, 0x108fff, MRA16_RAM             },	/* Text Layer*/
	{ 0x110000, 0x110fff, MRA16_RAM             },	/* Palette*/
	{ 0x124000, 0x127fff, sharedram_r           },	/* Shared with sub CPU*/
	{ 0x130000, 0x130fff, MRA16_RAM             },	/* Sprites*/
	/* Input Ports:*/
	{ 0x140010, 0x140011, input_port_0_word_r },	/* Coins + brake + gear*/
	{ 0x140012, 0x140013, input_port_1_word_r },	/* ??*/
	{ 0x140014, 0x140015, input_port_2_word_r },	/* DSW*/
	{ 0x140016, 0x140017, input_port_3_word_r },	/* DSW*/
	{ 0x140020, 0x140021, selected_ip_r       },	/* Accelerator or Wheel or ..*/
MEMORY_END

static MEMORY_WRITE16_START( wecleman_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM                             },	/* ROM (03c000-03ffff used as RAM sometimes!)*/
	{ 0x040494, 0x040495, wecleman_videostatus_w, &wecleman_videostatus },	/* cloud blending control (HACK)*/
	{ 0x040000, 0x043fff, MWA16_RAM                             },	/* RAM*/
	{ 0x060000, 0x060005, wecleman_protection_w, &wecleman_protection_ram },
	{ 0x080000, 0x080011, blitter_w,          &blitter_regs     },	/* Blitter*/
	{ 0x100000, 0x103fff, wecleman_pageram_w, &wecleman_pageram },	/* Background Layers*/
	{ 0x108000, 0x108fff, wecleman_txtram_w,  &wecleman_txtram  },	/* Text Layer*/
	{ 0x110000, 0x110fff, wecleman_paletteram16_SSSSBBBBGGGGRRRR_word_w, &paletteram16 },
	{ 0x124000, 0x127fff, sharedram_w,        &sharedram        },	/* Shared with main CPU*/
	{ 0x130000, 0x130fff, MWA16_RAM,          &spriteram16,     },	/* Sprites*/
	{ 0x140000, 0x140001, wecleman_soundlatch_w                 },	/* To sound CPU*/
	{ 0x140002, 0x140003, selected_ip_w                         },	/* Selects accelerator / wheel / ..*/
	{ 0x140004, 0x140005, irqctrl_w                             },	/* Main CPU controls the other CPUs*/
	{ 0x140006, 0x140007, MWA16_NOP                             },	/* Watchdog reset*/
	{ 0x140020, 0x140021, MWA16_RAM                             },	/* Paired with writes to $140003*/
	{ 0x140030, 0x140031, MWA16_NOP },	/* toggles between 0 & 1 on hitting bumps and crashes (vibration?)*/
MEMORY_END


/***************************************************************************
						Hot Chase Main CPU Handlers
***************************************************************************/

static READ16_HANDLER( hotchase_K051316_0_r )
{
	return K051316_0_r(offset) & 0xff;
}

static READ16_HANDLER( hotchase_K051316_1_r )
{
	return K051316_1_r(offset) & 0xff;
}

static WRITE16_HANDLER( hotchase_K051316_0_w )
{
	if (ACCESSING_LSB)      K051316_0_w(offset, data & 0xff);
}

static WRITE16_HANDLER( hotchase_K051316_1_w )
{
	if (ACCESSING_LSB)      K051316_1_w(offset, data & 0xff);
}

static WRITE16_HANDLER( hotchase_K051316_ctrl_0_w )
{
	if (ACCESSING_LSB)      K051316_ctrl_0_w(offset, data & 0xff);
}

static WRITE16_HANDLER( hotchase_K051316_ctrl_1_w )
{
	if (ACCESSING_LSB)      K051316_ctrl_1_w(offset, data & 0xff);
}

WRITE16_HANDLER( hotchase_soundlatch_w );

static MEMORY_READ16_START( hotchase_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM                         },	/* ROM*/
	{ 0x040000, 0x063fff, MRA16_RAM                         },	/* RAM (weird size!?)*/
	{ 0x080000, 0x080011, MRA16_RAM                         },	/* Blitter*/
	{ 0x100000, 0x100fff, hotchase_K051316_0_r              },	/* Background*/
	{ 0x102000, 0x102fff, hotchase_K051316_1_r              },	/* Foreground*/
	{ 0x110000, 0x111fff, MRA16_RAM                         },	/* Palette (only the first 2048 colors used)*/
	{ 0x120000, 0x123fff, sharedram_r                       },	/* Shared with sub CPU*/
	{ 0x130000, 0x130fff, MRA16_RAM                         },	/* Sprites*/
	/* Input Ports:*/
	{ 0x140006, 0x140007, MRA16_NOP                         },	/* Watchdog reset*/
	{ 0x140010, 0x140011, input_port_0_word_r               },	/* Coins + brake + gear*/
	{ 0x140012, 0x140013, input_port_1_word_r               },	/* ?? bit 4 from sound cpu*/
	{ 0x140014, 0x140015, input_port_2_word_r               },	/* DSW 2*/
	{ 0x140016, 0x140017, input_port_3_word_r               },	/* DSW 1*/
	{ 0x140020, 0x140021, selected_ip_r                     },	/* Accelerator or Wheel*/
	{ 0x140022, 0x140023, MRA16_NOP                         },	/* ??*/
MEMORY_END

static MEMORY_WRITE16_START( hotchase_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM                 },	/* ROM*/
	{ 0x040000, 0x063fff, MWA16_RAM                 },	/* RAM (weird size!?)*/
	{ 0x080000, 0x080011, blitter_w, &blitter_regs  },	/* Blitter*/
	{ 0x100000, 0x100fff, hotchase_K051316_0_w      },	/* Background*/
	{ 0x101000, 0x10101f, hotchase_K051316_ctrl_0_w },	/* Background Ctrl*/
	{ 0x102000, 0x102fff, hotchase_K051316_1_w      },	/* Foreground*/
	{ 0x103000, 0x10301f, hotchase_K051316_ctrl_1_w },	/* Foreground Ctrl*/
	{ 0x110000, 0x111fff, hotchase_paletteram16_SBGRBBBBGGGGRRRR_word_w, &paletteram16 },
	{ 0x120000, 0x123fff, sharedram_w, &sharedram   },	/* Shared with sub CPU*/
	{ 0x130000, 0x130fff, MWA16_RAM, &spriteram16   },	/* Sprites*/
	/* Input Ports:*/
	{ 0x140000, 0x140001, hotchase_soundlatch_w     },	/* To sound CPU*/
	{ 0x140002, 0x140003, selected_ip_w             },	/* Selects accelerator / wheel /*/
	{ 0x140004, 0x140005, irqctrl_w                 },	/* Main CPU controls the other CPUs*/
	{ 0x140020, 0x140021, MWA16_NOP                 },	/* Paired with writes to $140003*/
	{ 0x140030, 0x140031, MWA16_NOP                 },	/* signal to cabinet vibration motors?*/
MEMORY_END


/***************************************************************************
					WEC Le Mans 24 Sub CPU Handlers
***************************************************************************/

static MEMORY_READ16_START( wecleman_sub_readmem )
	{ 0x000000, 0x00ffff, MRA16_ROM    },	/* ROM*/
	{ 0x060000, 0x060fff, MRA16_RAM    },	/* Road*/
	{ 0x070000, 0x073fff, &sharedram_r },	/* RAM (Shared with main CPU)*/
MEMORY_END

static MEMORY_WRITE16_START( wecleman_sub_writemem )
	{ 0x000000, 0x00ffff, MWA16_ROM    },	/* ROM*/
	{ 0x060000, 0x060fff, MWA16_RAM, &wecleman_roadram, &wecleman_roadram_size },	/* Road*/
	{ 0x070000, 0x073fff, sharedram_w  },	/* RAM (Shared with main CPU)*/
MEMORY_END


/***************************************************************************
						Hot Chase Sub CPU Handlers
***************************************************************************/

static MEMORY_READ16_START( hotchase_sub_readmem )
	{ 0x000000, 0x01ffff, MRA16_ROM    },	/* ROM*/
	{ 0x020000, 0x020fff, MRA16_RAM    },	/* Road*/
	{ 0x060000, 0x060fff, MRA16_RAM    },	/* RAM*/
	{ 0x040000, 0x043fff, &sharedram_r },	/* Shared with main CPU*/
MEMORY_END

static MEMORY_WRITE16_START( hotchase_sub_writemem )
	{ 0x000000, 0x01ffff, MWA16_ROM    },	/* ROM*/
	{ 0x020000, 0x020fff, MWA16_RAM, &wecleman_roadram, &wecleman_roadram_size },	/* Road*/
	{ 0x060000, 0x060fff, MWA16_RAM    },	/* RAM*/
	{ 0x040000, 0x043fff, sharedram_w  },	/* Shared with main CPU*/
MEMORY_END


/***************************************************************************
					WEC Le Mans 24 Sound CPU Handlers
***************************************************************************/

/* 140001.b */
WRITE16_HANDLER( wecleman_soundlatch_w )
{
	if (ACCESSING_LSB)
	{
		soundlatch_w(0,data & 0xFF);
		cpu_set_irq_line(2,0, HOLD_LINE);
	}
}

/* Protection - an external multiplyer connected to the sound CPU */
READ_HANDLER( multiply_r )
{
	return (multiply_reg[0] * multiply_reg[1]) & 0xFF;
}

WRITE_HANDLER( multiply_w )
{
	multiply_reg[offset] = data;
}

/*      K007232 registers reminder:

[Ch A]  [Ch B]  [Meaning]
00      06      address step    (low  byte)
01      07      address step    (high byte, max 1)
02      08      sample address  (low  byte)
03      09      sample address  (mid  byte)
04      0a      sample address  (high byte, max 1 -> max rom size: $20000)
05      0b      Reading this byte triggers the sample

[Ch A & B]
0c              volume
0d              play sample once or looped (2 channels -> 2 bits (0&1))

** sample playing ends when a byte with bit 7 set is reached **/

WRITE_HANDLER( wecleman_K00723216_bank_w )
{
	K007232_set_bank( 0, 0, ~data&1 );	/** (wecleman062gre)*/
}

static MEMORY_READ_START( wecleman_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM                },	/* ROM*/
	{ 0x8000, 0x83ff, MRA_RAM                },	/* RAM*/
	{ 0x9000, 0x9000, multiply_r             },	/* Protection*/
	{ 0xa000, 0xa000, soundlatch_r           },	/* From main CPU*/
	{ 0xb000, 0xb00d, K007232_read_port_0_r  },	/* K007232 (Reading offset 5/b triggers the sample)*/
	{ 0xc001, 0xc001, YM2151_status_port_0_r },	/* YM2151*/
MEMORY_END

static MEMORY_WRITE_START( wecleman_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM                   },	/* ROM*/
	{ 0x8000, 0x83ff, MWA_RAM                   },	/* RAM*/
	{ 0x8500, 0x8500, MWA_NOP                   },	/* incresed with speed (global volume)?*/
	{ 0x9000, 0x9001, multiply_w                },	/* Protection*/
	{ 0x9006, 0x9006, MWA_NOP                   },	/* ?*/
	{ 0xb000, 0xb00d, K007232_write_port_0_w    },	/* K007232*/
	{ 0xc000, 0xc000, YM2151_register_port_0_w  },	/* YM2151*/
	{ 0xc001, 0xc001, YM2151_data_port_0_w      },
	{ 0xf000, 0xf000, wecleman_K00723216_bank_w },	/* Samples banking*/
MEMORY_END


/***************************************************************************
						Hot Chase Sound CPU Handlers
***************************************************************************/

/* 140001.b */
WRITE16_HANDLER( hotchase_soundlatch_w )
{
	if (ACCESSING_LSB)
	{
		soundlatch_w(0,data & 0xFF);
		cpu_set_irq_line(2,M6809_IRQ_LINE, HOLD_LINE);
	}
}

static struct K007232_interface hotchase_k007232_interface =
{
	3,
	3579545,	/* clock */
	{ REGION_SOUND1, REGION_SOUND2, REGION_SOUND3 },
	{ K007232_VOL( 20,MIXER_PAN_CENTER, 20,MIXER_PAN_CENTER ),
	  K007232_VOL( 20,MIXER_PAN_LEFT,   20,MIXER_PAN_RIGHT  ),
	  K007232_VOL( 20,MIXER_PAN_LEFT,   20,MIXER_PAN_RIGHT  ) },
	{ 0,0,0 }
};

WRITE_HANDLER( hotchase_sound_control_w )
{
	int reg[8];

	reg[offset] = data;

	switch (offset)
	{
		case 0x0:
		case 0x1:
		case 0x2:
		case 0x3:
		case 0x4:
		case 0x5:
			/* change volume
				offset 00000xxx----- channel select (0:channel 0, 1:channel 1)
				++------ chip select ( 0:chip 1, 1:chip2, 2:chip3)
				data&0x0f left volume  (data>>4)&0x0f right volume
			*/
		  K007232_set_volume( offset>>1, offset&1,  (data&0x0f) * 0x08, (data>>4) * 0x08 );
		  break;

		case 0x06:	/* Bankswitch for chips 0 & 1 */
		{
			int bank0_a = (data >> 1) & 1;
			int bank1_a = (data >> 2) & 1;
			int bank0_b = (data >> 3) & 1;
			int bank1_b = (data >> 4) & 1;
			/* bit 6: chip 2 - ch0 ?*/
			/* bit 7: chip 2 - ch1 ?*/

			K007232_set_bank( 0, bank0_a, bank0_b );
			K007232_set_bank( 1, bank1_a, bank1_b );
		}
		break;

		case 0x07:	/* Bankswitch for chip 2 */
		{
			int bank2_a = (data >> 0) & 7;
			int bank2_b = (data >> 3) & 7;

			K007232_set_bank( 2, bank2_a, bank2_b );
		}
		break;
	}
}

/* Read and write handlers for one K007232 chip:
   even and odd register are mapped swapped */
#define HOTCHASE_K007232_RW(_chip_) \
READ_HANDLER( hotchase_K007232_##_chip_##_r ) \
{ \
	return K007232_read_port_##_chip_##_r(offset ^ 1); \
} \
WRITE_HANDLER( hotchase_K007232_##_chip_##_w ) \
{ \
	K007232_write_port_##_chip_##_w(offset ^ 1, data); \
} \

/* 3 x K007232 */
HOTCHASE_K007232_RW(0)
HOTCHASE_K007232_RW(1)
HOTCHASE_K007232_RW(2)

static MEMORY_READ_START( hotchase_sound_readmem )
	{ 0x0000, 0x07ff, MRA_RAM              },	/* RAM*/
	{ 0x1000, 0x100d, hotchase_K007232_0_r },	/* 3 x  K007232*/
	{ 0x2000, 0x200d, hotchase_K007232_1_r },
	{ 0x3000, 0x300d, hotchase_K007232_2_r },
	{ 0x6000, 0x6000, soundlatch_r         },	/* From main CPU (Read on IRQ)*/
	{ 0x8000, 0xffff, MRA_ROM              },	/* ROM*/
MEMORY_END

static MEMORY_WRITE_START( hotchase_sound_writemem )
	{ 0x0000, 0x07ff, MWA_RAM                  },	/* RAM*/
	{ 0x1000, 0x100d, hotchase_K007232_0_w     },	/* 3 x K007232*/
	{ 0x2000, 0x200d, hotchase_K007232_1_w     },
	{ 0x3000, 0x300d, hotchase_K007232_2_w     },
	{ 0x4000, 0x4007, hotchase_sound_control_w },	/* Sound volume, banking, etc.*/
	{ 0x5000, 0x5000, MWA_NOP                  },	/* ? (written with 0 on IRQ, 1 on FIRQ)*/
	{ 0x7000, 0x7000, MWA_NOP                  },	/* Command acknowledge ?*/
	{ 0x8000, 0xffff, MWA_ROM                  },	/* ROM*/
MEMORY_END


/***************************************************************************
						WEC Le Mans 24 Input Ports
***************************************************************************/

INPUT_PORTS_START( wecleman )
	PORT_START	/* IN0 - Controls and Coins - $140011.b */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_SERVICE_NO_TOGGLE( 0x04, IP_ACTIVE_HIGH )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_TOGGLE, "Shift", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_BUTTON2, "Brake", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )
	
	PORT_START	/* IN1 - Motor? - $140013.b */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE2 )	/* right sw*/
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE3 )	/* left sw*/
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE4 )	/* thermo*/
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* from sound cpu ?*/
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 - DSW A (Coinage) - $140015.b */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_7C ) )

	PORT_START	/* IN3 - DSW B (options) - $140017.b */
	PORT_DIPNAME( 0x01, 0x01, "Speed Unit" )
	PORT_DIPSETTING(    0x01, "km/h" )
	PORT_DIPSETTING(    0x00, "mph" )
	PORT_DIPNAME( 0x02, 0x02, "Unknown B-1" )	/* single*/
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Unknown B-2" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x18, "Easy" )		/* 66 seconds at the start*/
	PORT_DIPSETTING(    0x10, "Normal" )	/* 64*/
	PORT_DIPSETTING(    0x08, "Hard" )		/* 62*/
	PORT_DIPSETTING(    0x00, "Hardest" )	/* 60*/
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Unknown B-6" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Unknown B-7" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* IN4 - Accelerator - $140021.b (0) */
	PORT_ANALOG( 0xff, 0, IPT_PEDAL, 30, 10, 0, 0x80 )

	PORT_START	/* IN5 - Steering Wheel - $140021.b (2) */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_X | IPF_CENTER, 50, 5, 0, 0xff )
INPUT_PORTS_END


/***************************************************************************
							Hot Chase Input Ports
***************************************************************************/

INPUT_PORTS_START( hotchase )
	PORT_START	/* IN0 - Controls and Coins - $140011.b */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_SERVICE_NO_TOGGLE( 0x04, IP_ACTIVE_LOW )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_TOGGLE, "Shift", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_BUTTON2, "Brake", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN1 - Motor? - $140013.b */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE2 )	/* right sw*/
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE3 )	/* left sw*/
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE4 )	/* thermo*/
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* from sound cpu ?*/
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 - DSW 2 (options) - $140015.b */
	PORT_DIPNAME( 0x01, 0x01, "Unknown 2-0" )	/* single*/
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Unknown 2-1" )	/* single (wheel related)*/
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Unknown 2-2" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x18, "Unknown 2-3&4" )
	PORT_DIPSETTING(    0x18, "0" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x08, "8" )
	PORT_DIPSETTING(    0x00, "c" )
	PORT_DIPNAME( 0x20, 0x20, "Unknown 2-5" )	/* single*/
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	/* wheel <-> brake ; accel -> start */
	PORT_DIPNAME( 0x40, 0x40, "Unknown 2-6" )	/* single (wheel<->brake)*/
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 2-7" )	/* single*/
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* IN3 - DSW 1 (Coinage) - $140017.b */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 5C_3C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 5C_3C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x00, "1 Coin/99 Credits" )

	PORT_START	/* IN4 - Accelerator - $140021.b (0) */
	PORT_ANALOG( 0xff, 0, IPT_PEDAL, 30, 10, 0, 0x80 )

	PORT_START	/* IN5 - Steering Wheel - $140021.b (2) */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_X | IPF_CENTER, 50, 5, 0, 0xff )
INPUT_PORTS_END


/***************************************************************************
							WEC Le Mans 24 Graphics Layout
***************************************************************************/

static struct GfxLayout wecleman_bg_layout =
{
	8,8,
	8*0x8000*3/(8*8*3),
	3,
	{ 0,0x8000*8,0x8000*8*2 },
	{0,7,6,5,4,3,2,1},
	{0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8},
	8*8
};

/* We draw the road, made of 512 pixel lines, using 64x1 tiles */
static struct GfxLayout wecleman_road_layout =
{
	64,1,
	8*0x4000*3/(64*1*3),
	3,
	{ 0x4000*8*2,0x4000*8*1,0x4000*8*0 },
	{0,7,6,5,4,3,2,1,
	 8,15,14,13,12,11,10,9,
	 16,23,22,21,20,19,18,17,
	 24,31,30,29,28,27,26,25,

	 0+32,7+32,6+32,5+32,4+32,3+32,2+32,1+32,
	 8+32,15+32,14+32,13+32,12+32,11+32,10+32,9+32,
	 16+32,23+32,22+32,21+32,20+32,19+32,18+32,17+32,
	 24+32,31+32,30+32,29+32,28+32,27+32,26+32,25+32},
	{0},
	64*1
};

static struct GfxDecodeInfo wecleman_gfxdecodeinfo[] =
{
	/* REGION_GFX1 holds sprite, which are not decoded here*/
	{ REGION_GFX2, 0, &wecleman_bg_layout,   0, 2048/8 },	/* [0] bg + fg + txt*/
	{ REGION_GFX3, 0, &wecleman_road_layout, 0, 2048/8 },	/* [1] road*/
	{ -1 }
};


/***************************************************************************
							Hot Chase Graphics Layout
***************************************************************************/

/* We draw the road, made of 512 pixel lines, using 64x1 tiles */
/* tiles are doubled horizontally */
static struct GfxLayout hotchase_road_layout =
{
	64,1,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0*4,0*4,1*4,1*4,2*4,2*4,3*4,3*4,4*4,4*4,5*4,5*4,6*4,6*4,7*4,7*4,
	  8*4,8*4,9*4,9*4,10*4,10*4,11*4,11*4,12*4,12*4,13*4,13*4,14*4,14*4,15*4,15*4,
	 16*4,16*4,17*4,17*4,18*4,18*4,19*4,19*4,20*4,20*4,21*4,21*4,22*4,22*4,23*4,23*4,
	 24*4,24*4,25*4,25*4,26*4,26*4,27*4,27*4,28*4,28*4,29*4,29*4,30*4,30*4,31*4,31*4 },
	{0},
	32*4
};

static struct GfxDecodeInfo hotchase_gfxdecodeinfo[] =
{
	/* REGION_GFX1 holds sprite, which are not decoded here*/
	/* REGION_GFX2 and 3 are for the 051316*/
	{ REGION_GFX4, 0, &hotchase_road_layout, 0x70*16, 16 },	/* road*/
	{ -1 }
};


/***************************************************************************
						WEC Le Mans 24 Hardware Definitions
***************************************************************************/

static INTERRUPT_GEN( wecleman_interrupt )
{
	if (cpu_getiloops() == 0)
		cpu_set_irq_line(0, 4, HOLD_LINE);	/* once */
	else
		cpu_set_irq_line(0, 5, HOLD_LINE);	/* to read input ports */
}

static struct YM2151interface ym2151_interface =
{
	1,
	3579545,	/* same as sound cpu */
	{ YM3012_VOL(85,MIXER_PAN_LEFT,85,MIXER_PAN_RIGHT) },
	{ 0  }
};

static struct K007232_interface wecleman_k007232_interface =
{
	1,
	3579545,	/* clock */
	{ REGION_SOUND1 },	/* but the 2 channels use different ROMs !*/
	{ K007232_VOL( 20,MIXER_PAN_LEFT, 20,MIXER_PAN_RIGHT ) },
	{0}
};

MACHINE_INIT( wecleman )
{
	K007232_set_bank( 0, 0, 1 );
}

static MACHINE_DRIVER_START( wecleman )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000)	/* Schems show 10MHz */
	MDRV_CPU_MEMORY(wecleman_readmem,wecleman_writemem)
	MDRV_CPU_VBLANK_INT(wecleman_interrupt,5 + 1)	/* in order to read the inputs once per frame */

	MDRV_CPU_ADD(M68000, 10000000)	/* Schems show 10MHz */
	MDRV_CPU_MEMORY(wecleman_sub_readmem,wecleman_sub_writemem)

	/* Schems: can be reset, no nmi, soundlatch, 3.58MHz */
	MDRV_CPU_ADD(Z80, 3579545)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(wecleman_sound_readmem,wecleman_sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)

	MDRV_MACHINE_INIT(wecleman)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(320 +16, 224 +16)
	MDRV_VISIBLE_AREA(0 +8, 320-1 +8, 0 +8, 224-1 +8)
	MDRV_GFXDECODE(wecleman_gfxdecodeinfo)

	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(wecleman)
	MDRV_VIDEO_UPDATE(wecleman)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(K007232, wecleman_k007232_interface)
MACHINE_DRIVER_END


/***************************************************************************
						Hot Chase Hardware Definitions
***************************************************************************/

static INTERRUPT_GEN( hotchase_sound_timer )
{
	cpu_set_irq_line( 2, M6809_FIRQ_LINE, PULSE_LINE );
}

static MACHINE_DRIVER_START( hotchase )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000)	/* 10 MHz - PCB is drawn in one set's readme */
	MDRV_CPU_MEMORY(hotchase_readmem,hotchase_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)

	MDRV_CPU_ADD(M68000, 10000000)	/* 10 MHz - PCB is drawn in one set's readme */
	MDRV_CPU_MEMORY(hotchase_sub_readmem,hotchase_sub_writemem)

	MDRV_CPU_ADD(M6809, 3579545 / 2)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 3.579/2 MHz - PCB is drawn in one set's readme */
	MDRV_CPU_MEMORY(hotchase_sound_readmem,hotchase_sound_writemem)
	MDRV_CPU_PERIODIC_INT( hotchase_sound_timer, 496 )

	/* Amuse: every 2 ms */
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 224)
	MDRV_VISIBLE_AREA(0, 320-1, 0, 224-1)
	MDRV_GFXDECODE(hotchase_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048*2)

	MDRV_VIDEO_START(hotchase)
	MDRV_VIDEO_UPDATE(hotchase)

	/* sound hardware */
	MDRV_SOUND_ADD(K007232, hotchase_k007232_interface)
MACHINE_DRIVER_END


/***************************************************************************
						WEC Le Mans 24 ROM Definitions
***************************************************************************/

ROM_START( wecleman )

	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* Main CPU Code */
	ROM_LOAD16_BYTE( "602f08.17h", 0x00000, 0x10000, CRC(493b79d3) SHA1(9625e3b65c211d5081d8ed8977de287eff100842) )
	ROM_LOAD16_BYTE( "602f11.23h", 0x00001, 0x10000, CRC(6bb4f1fa) SHA1(2cfb7885b42b49dab9892e8dfd54914b64eeab06) )
	ROM_LOAD16_BYTE( "602a09.18h", 0x20000, 0x10000, CRC(8a9d756f) SHA1(12605e86ce29e6300b5400720baac7b0293d9e66) )
	ROM_LOAD16_BYTE( "602a10.22h", 0x20001, 0x10000, CRC(569f5001) SHA1(ec2dd331a279083cf847fbbe71c017038a1d562a) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* Sub CPU Code */
	ROM_LOAD16_BYTE( "602a06.18a", 0x00000, 0x08000, CRC(e12c0d11) SHA1(991afd48bf1b2c303b975ce80c754e5972c39111) )
	ROM_LOAD16_BYTE( "602a07.20a", 0x00001, 0x08000, CRC(47968e51) SHA1(9b01b2c6a14dd80327a8f66a7f1994471a4bc38e) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* Sound CPU Code */
	ROM_LOAD( "602a01.6d",  0x00000, 0x08000, CRC(deafe5f1) SHA1(4cfbe2841233b1222c22160af7287b7a7821c3a0) )

	ROM_REGION( 0x200000 * 2, REGION_GFX1, 0 )	/* x2, do not dispose, zooming sprites */
	ROM_LOAD( "602a25.12e", 0x000000, 0x20000, CRC(0eacf1f9) SHA1(b4dcd457e68175ffee3da4aff23a241fe33eb500) )
	ROM_LOAD( "602a26.14e", 0x020000, 0x20000, CRC(2182edaf) SHA1(5ae4223a76b3c0be8f66458707f2e6f63fba0b13) )
	ROM_LOAD( "602a27.15e", 0x040000, 0x20000, CRC(b22f08e9) SHA1(1ba99bc4e00e206507e9bfafc989208d6ae6f8a3) )
	ROM_LOAD( "602a28.17e", 0x060000, 0x20000, CRC(5f6741fa) SHA1(9c81634f502da8682673b3b87efe0497af8abbd7) )
	ROM_LOAD( "602a21.6e",  0x080000, 0x20000, CRC(8cab34f1) SHA1(264df01460f44cd5ccdf3c8bd2d3f327874b69ea) )
	ROM_LOAD( "602a22.7e",  0x0a0000, 0x20000, CRC(e40303cb) SHA1(da943437ea2e208ea477f35bb05f77412ecdf9ac) )
	ROM_LOAD( "602a23.9e",  0x0c0000, 0x20000, CRC(75077681) SHA1(32ad10e9e32779c36bb50b402f5c6d941e293942) )
	ROM_LOAD( "602a24.10e", 0x0e0000, 0x20000, CRC(583dadad) SHA1(181ebe87095d739a5903c17ec851864e2275f571) )
	ROM_LOAD( "602a17.12c", 0x100000, 0x20000, CRC(31612199) SHA1(dff58ec3f7d98bfa7e9405f0f23647ff4ecfee62) )
	ROM_LOAD( "602a18.14c", 0x120000, 0x20000, CRC(3f061a67) SHA1(be57c38410c5635311d26afc44b3065e42fa12b7) )
	ROM_LOAD( "602a19.15c", 0x140000, 0x20000, CRC(5915dbc5) SHA1(61ab123c8a4128a18d7eb2cae99ad58203f03ffc) )
	ROM_LOAD( "602a20.17c", 0x160000, 0x20000, CRC(f87e4ef5) SHA1(4c2f0d036925a7ccd32aef3ca12b960a27247bc3) )
	ROM_LOAD( "602a13.6c",  0x180000, 0x20000, CRC(5d3589b8) SHA1(d146cb8511cfe825bdfe8296c7758545542a0faa) )
	ROM_LOAD( "602a14.7c",  0x1a0000, 0x20000, CRC(e3a75f6c) SHA1(80b20323e3560316ffbdafe4fd2f81326e103045) )
	ROM_LOAD( "602a15.9c",  0x1c0000, 0x20000, CRC(0d493c9f) SHA1(02690a1963cadd469bd67cb362384923916900a1) )
	ROM_LOAD( "602a16.10c", 0x1e0000, 0x20000, CRC(b08770b3) SHA1(41871e9261d08fd372b7deb72d939973fb694b54) )

	ROM_REGION( 0x18000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "602a31.26g", 0x000000, 0x08000, CRC(01fa40dd) SHA1(2b8aa97f5116f39ae6a8e46f109853d70e370884) )	/* layers*/
	ROM_LOAD( "602a30.24g", 0x008000, 0x08000, CRC(be5c4138) SHA1(7aee2ee17ef3e37399a60d9b019cfa733acbf07b) )
	ROM_LOAD( "602a29.23g", 0x010000, 0x08000, CRC(f1a8d33e) SHA1(ed6531f2fd4ad6835a879e9a5600387d8cad6d17) )

	ROM_REGION( 0x0c000, REGION_GFX3, ROMREGION_DISPOSE )	/* road */
	ROM_LOAD( "602a04.11e", 0x000000, 0x08000, CRC(ade9f359) SHA1(58db6be6217ed697827015e50e99e58602042a4c) )
	ROM_LOAD( "602a05.13e", 0x008000, 0x04000, CRC(f22b7f2b) SHA1(857389c57552c4e2237cb599f4c68c381430475e) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples (Channel A 0x20000=Channel B) */
	ROM_LOAD( "602a03.10a", 0x00000, 0x20000, CRC(31392b01) SHA1(0424747bc2015c9c93afd20e6a23083c0dcc4fb7) )
	ROM_LOAD( "602a02.8a",  0x20000, 0x20000, CRC(e2be10ae) SHA1(109c31bf7252c83a062d259143cd8299681db778) )

	ROM_REGION( 0x04000, REGION_USER1, 0 )	/* extra data for road effects? */
	ROM_LOAD( "602a12.1a",  0x000000, 0x04000, CRC(77b9383d) SHA1(7cb970889677704d6324bb64aafc05326c4503ad) )

ROM_END

void wecleman_unpack_sprites(void)
{
	const int region       = REGION_GFX1;	/* sprites*/

	const unsigned int len = memory_region_length(region);
	unsigned char *src     = memory_region(region) + len / 2 - 1;
	unsigned char *dst     = memory_region(region) + len - 1;

	while(dst > src)
	{
		unsigned char data = *src--;
		if( (data&0xf0) == 0xf0 ) data &= 0x0f;
		if( (data&0x0f) == 0x0f ) data &= 0xf0;
		*dst-- = data & 0xF;    *dst-- = data >> 4;
	}
}

static void bitswap(data8_t *src,size_t len,int _14,int _13,int _12,int _11,int _10,int _f,int _e,int _d,int _c,int _b,int _a,int _9,int _8,int _7,int _6,int _5,int _4,int _3,int _2,int _1,int _0)
{
	data8_t *buffer = malloc(len);

	if (buffer)
	{
		int i;

		memcpy(buffer,src,len);
		for (i = 0;i < len;i++)
		{
			src[i] =
				buffer[BITSWAP24(i,23,22,21,_14,_13,_12,_11,_10,_f,_e,_d,_c,_b,_a,_9,_8,_7,_6,_5,_4,_3,_2,_1,_0)];
		}
		free(buffer);
	}
}

/* Unpack sprites data and do some patching */
DRIVER_INIT( wecleman )
{
	int i;
	unsigned char *RAM;
/*	data16_t *RAM1 = (data16_t *) memory_region(REGION_CPU1);	 // Main CPU patches /*/
/*	RAM1[0x08c2/2] = 0x601e;	*/ /* faster self test*/

	/* Decode GFX Roms - Compensate for the address lines scrambling */

	/*  Sprites - decrypting the sprites nearly KILLED ME!
	    It's been the main cause of the delay of this driver ...
	    I hope you'll appreciate this effort!  */

	/* let's swap even and odd *pixels* of the sprites */
	RAM = memory_region(REGION_GFX1);
	for (i = 0; i < memory_region_length(REGION_GFX1); i ++)
	{
		/* TODO: could be wrong, colors have to be fixed.       */
		/* The only certain thing is that 87 must convert to f0 */
		/* otherwise stray lines appear, made of pens 7 & 8     */
		RAM[i] = BITSWAP8(RAM[i],7,0,1,2,3,4,5,6);
	}

	bitswap(memory_region(REGION_GFX1), memory_region_length(REGION_GFX1),
			0,1,20,19,18,17,14,9,16,6,4,7,8,15,10,11,13,5,12,3,2);

	/* Now we can unpack each nibble of the sprites into a pixel (one byte) */
	wecleman_unpack_sprites();

	/* Bg & Fg & Txt */
	bitswap(memory_region(REGION_GFX2), memory_region_length(REGION_GFX2),
			20,19,18,17,16,15,12,7,14,4,2,5,6,13,8,9,11,3,10,1,0);

	/* Road */
	bitswap(memory_region(REGION_GFX3), memory_region_length(REGION_GFX3),
			20,19,18,17,16,15,14,7,12,4,2,5,6,13,8,9,11,3,10,1,0);

	spr_color_offs = 0x40;
}


/***************************************************************************
							Hot Chase ROM Definitions
***************************************************************************/

ROM_START( hotchase )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* Main Code */
	ROM_LOAD16_BYTE( "763k05", 0x000000, 0x010000, CRC(f34fef0b) SHA1(9edaf6da988348cb32d5686fe7a67fb92b1c9777) )
	ROM_LOAD16_BYTE( "763k04", 0x000001, 0x010000, CRC(60f73178) SHA1(49c919d09fa464b205d7eccce337349e3a633a14) )
	ROM_LOAD16_BYTE( "763k03", 0x020000, 0x010000, CRC(28e3a444) SHA1(106b22a3cbe8301eac2e46674a267b96e72ac72f) )
	ROM_LOAD16_BYTE( "763k02", 0x020001, 0x010000, CRC(9510f961) SHA1(45b1920cab08a0dacd044c851d4e7f0cb5772b46) )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )	/* Sub Code */
	ROM_LOAD16_BYTE( "763k07", 0x000000, 0x010000, CRC(ae12fa90) SHA1(7f76f09916fe152411b5af3c504ee7be07497ef4) )
	ROM_LOAD16_BYTE( "763k06", 0x000001, 0x010000, CRC(b77e0c07) SHA1(98bf492ac889d31419df706029fdf3d51b85c936) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* Sound Code */
	ROM_LOAD( "763f01", 0x8000, 0x8000, CRC(4fddd061) SHA1(ff0aa18605612f6102107a6be1f93ae4c5edc84f) )

	ROM_REGION( 0x300000 * 2, REGION_GFX1, 0 )	/* x2, do not dispose, zooming sprites */
	ROM_LOAD16_WORD_SWAP( "763e17", 0x000000, 0x080000, CRC(8db4e0aa) SHA1(376cb3cae110998f2f9df7e6cdd35c06732fea69) )
	ROM_LOAD16_WORD_SWAP( "763e20", 0x080000, 0x080000, CRC(a22c6fce) SHA1(174fb9c1706c092947bcce386831acd33a237046) )
	ROM_LOAD16_WORD_SWAP( "763e18", 0x100000, 0x080000, CRC(50920d01) SHA1(313c7ecbd154b3f4c96f25c29a7734a9b3facea4) )
	ROM_LOAD16_WORD_SWAP( "763e21", 0x180000, 0x080000, CRC(77e0e93e) SHA1(c8e415438a1f5ad79b10fd3ad5cb22de0d562e5d) )
	ROM_LOAD16_WORD_SWAP( "763e19", 0x200000, 0x080000, CRC(a2622e56) SHA1(0a0ed9713882b987518e6f06a02dba417c1f4f32) )
	ROM_LOAD16_WORD_SWAP( "763e22", 0x280000, 0x080000, CRC(967c49d1) SHA1(01979d216a9fd8085298445ac5f7870d1598db74) )

	ROM_REGION( 0x20000, REGION_GFX2, 0 )	/* bg */
	ROM_LOAD( "763e14", 0x000000, 0x020000, CRC(60392aa1) SHA1(8499eb40a246587e24f6fd00af2eaa6d75ee6363) )

	ROM_REGION( 0x10000, REGION_GFX3, 0 )	/* fg (patched) */
	ROM_LOAD( "763a13", 0x000000, 0x010000, CRC(8bed8e0d) SHA1(ccff330abc23fe499e76c16cab5783c3daf155dd) )

	ROM_REGION( 0x20000, REGION_GFX4, ROMREGION_DISPOSE )	/* road */
	ROM_LOAD( "763e15", 0x000000, 0x020000, CRC(7110aa43) SHA1(639dc002cc1580f0530bb5bb17f574e2258d5954) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples, 2 banks */
	ROM_LOAD( "763e11", 0x000000, 0x040000, CRC(9d99a5a7) SHA1(96e37bbb259e0a91d124c26b6b1a9b70de2e19a4) )

	ROM_REGION( 0x40000, REGION_SOUND2, 0 )	/* Samples, 2 banks */
	ROM_LOAD( "763e10", 0x000000, 0x040000, CRC(ca409210) SHA1(703d7619c4bd33d2ff5fad127d98c82906fede33) )

	ROM_REGION( 0x100000, REGION_SOUND3, 0 )	/* Samples, 4 banks for each ROM */
	ROM_LOAD( "763e08", 0x000000, 0x080000, CRC(054a9a63) SHA1(45d7926c9e7af47c041ba9b733e334bccd730a6d) )
	ROM_LOAD( "763e09", 0x080000, 0x080000, CRC(c39857db) SHA1(64b135a9ccf9e1dd50789cdd5c6bc03da8decfd0) )

	ROM_REGION( 0x08000, REGION_USER1, 0 )	/* extra data for road effects? */
	ROM_LOAD( "763a12", 0x000000, 0x008000, CRC(05f1e553) SHA1(8aaeb7374bd93038c24e6470398936f22cabb0fe) )
ROM_END

/*      Important: you must leave extra space when listing sprite ROMs
	in a ROM module definition.  This routine unpacks each sprite nibble
	into a byte, doubling the memory consumption. */

void hotchase_sprite_decode( int num16_banks, int bank_size )
{
	unsigned char *base, *temp;
	int i;

	base = memory_region(REGION_GFX1);	/* sprites*/
	temp = malloc( bank_size );
	if( !temp ) return;

	for( i = num16_banks; i >0; i-- ){
		unsigned char *finish   = base + 2*bank_size*i;
		unsigned char *dest     = finish - 2*bank_size;

		unsigned char *p1 = temp;
		unsigned char *p2 = temp+bank_size/2;

		unsigned char data;

		memcpy (temp, base+bank_size*(i-1), bank_size);

		do {
			data = *p1++;
			if( (data&0xf0) == 0xf0 ) data &= 0x0f;
			if( (data&0x0f) == 0x0f ) data &= 0xf0;
			*dest++ = data >> 4;
			*dest++ = data & 0xF;
			data = *p1++;
			if( (data&0xf0) == 0xf0 ) data &= 0x0f;
			if( (data&0x0f) == 0x0f ) data &= 0xf0;
			*dest++ = data >> 4;
			*dest++ = data & 0xF;


			data = *p2++;
			if( (data&0xf0) == 0xf0 ) data &= 0x0f;
			if( (data&0x0f) == 0x0f ) data &= 0xf0;
			*dest++ = data >> 4;
			*dest++ = data & 0xF;
			data = *p2++;
			if( (data&0xf0) == 0xf0 ) data &= 0x0f;
			if( (data&0x0f) == 0x0f ) data &= 0xf0;
			*dest++ = data >> 4;
			*dest++ = data & 0xF;
		} while( dest<finish );
	}
	free( temp );
}

/* Unpack sprites data and do some patching */
DRIVER_INIT( hotchase )
{
/*	data16_t *RAM1 = (data16_t) memory_region(REGION_CPU1);	 // Main CPU patches /*/
/*	RAM[0x1140/2] = 0x0015; RAM[0x195c/2] = 0x601A;	*/ /* faster self test*/

	unsigned char *RAM;

	/* Decode GFX Roms */

	/* Let's swap even and odd bytes of the sprites gfx roms */
	RAM = memory_region(REGION_GFX1);

	/* Now we can unpack each nibble of the sprites into a pixel (one byte) */
	hotchase_sprite_decode(3,0x80000*2);	/* num banks, bank len*/

	/* Let's copy the second half of the fg layer gfx (charset) over the first */
	RAM = memory_region(REGION_GFX3);
	memcpy(&RAM[0], &RAM[0x10000/2], 0x10000/2);

	spr_color_offs = 0;
}


/***************************************************************************
								Game driver(s)
***************************************************************************/

GAME( 1986, wecleman, 0, wecleman, wecleman, wecleman, ROT0, "Konami", "WEC Le Mans 24" )
GAME( 1988, hotchase, 0, hotchase, hotchase, hotchase, ROT0, "Konami", "Hot Chase" )
