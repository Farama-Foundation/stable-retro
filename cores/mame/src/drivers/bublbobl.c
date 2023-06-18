/***************************************************************************
Tokio          (c) 1986 Taito
Bubble Bobble  (c) 1986 Taito
driver by Chris Moore, Nicola Salmoria
also based on Tokio driver by Marcelo de G. Malheiros <malheiro@dca.fee.unicamp.br>

Main clock: XTAL = 24 MHz
Horizontal video frequency: HSYNC = XTAL/4/384 = 15.625 kHz
Video frequency: VSYNC = HSYNC/264 = 59.185606 Hz
VBlank duration: 1/VSYNC * (40/264) = 2560 us

***************************************************************************

Bubble Bobble ROM info

CPU Board
---------
           | Taito  |Romstar | ?????  |Romstar |
           |        |        |missing |mode sel|
17  CU1    | A78-01 |   ->   |   ->   |   ->   |   protection mcu
49  PAL1   | A78-02 |   ->   |   ->   |   ->   |   address decoder
43  PAL2   | A78-03 |   ->   |   ->   |   ->   |   address decoder
12  PAL3   | A78-04 |   ->   |   ->   |   ->   |   address decoder
53  empty  |        |        |        |        |   main prg
52  ROM1   | A78-05 | A78-21 | A78-22 | A78-24 |   main prg
51  ROM2   | A78-06 |   ->   | A78-23 | A78-25 |   main prg
46  ROM4   | A78-07 |   ->   |   ->   |   ->   |   sound prg
37  ROM3   | A78-08 |   ->   |   ->   |   ->   |   sub prg

Video Board
-----------
12  ROM1   | A78-09 |   ->   |   ->   |   ->   |   gfx
13  ROM2   | A78-10 |   ->   |   ->   |   ->   |   gfx
14  ROM3   | A78-11 |   ->   |   ->   |   ->   |   gfx
15  ROM4   | A78-12 |   ->   |   ->   |   ->   |   gfx
16  ROM5   | A78-13 |   ->   |   ->   |   ->   |   gfx
17  ROM6   | A78-14 |   ->   |   ->   |   ->   |   gfx
18  empty  |        |        |        |        |   gfx
19  empty  |        |        |        |        |   gfx
30  ROM7   | A78-15 |   ->   |   ->   |   ->   |   gfx
31  ROM8   | A78-16 |   ->   |   ->   |   ->   |   gfx
32  ROM9   | A78-17 |   ->   |   ->   |   ->   |   gfx
33  ROM10  | A78-18 |   ->   |   ->   |   ->   |   gfx
34  ROM11  | A78-19 |   ->   |   ->   |   ->   |   gfx
35  ROM12  | A78-20 |   ->   |   ->   |   ->   |   gfx
36  empty  |        |        |        |        |   gfx
37  empty  |        |        |        |        |   gfx
41  ROM13  | A71-25 |   ->   |   ->   |   ->   |   video timing


Bobble Bobble memory map

driver by Chris Moore

CPU #1
0000-bfff ROM (8000-bfff is banked)
c000-dcff Graphic RAM. This contains pointers to the video RAM columns and
          to the sprites are contained in Object RAM.
dd00-dfff Object RAM (groups of four bytes: X position, code [offset in the
          Graphic RAM], Y position, gfx bank)
CPU #2
0000-7fff ROM

CPU #1 AND #2
e000-f7fe RAM
f800-f9ff Palette RAM
fc01-fdff RAM

read:
ff00      DSWA
ff01      DSWB
ff02      IN0
ff03      IN1


Service mode works only if the language switch is set to Japanese.

- The protection feature which randomizes the EXTEND letters in the original
  version is not emulated properly.

***************************************************************************

Tokio memory map

CPU 1
0000-bfff ROM (8000-bfff is banked)
c000-dcff Graphic RAM. This contains pointers to the video RAM columns and
          to the sprites contained in Object RAM.
dd00-dfff Object RAM (groups of four bytes: X position, code [offset in the
          Graphic RAM], Y position, gfx bank)
e000-f7ff RAM (Shared)
f800-f9ff Palette RAM

fa03 - DSW0
fa04 - DSW1
fa05 - Coins
fa06 - Controls Player 1
fa07 - Controls Player 1

CPU 2
0000-7fff ROM
8000-97ff RAM (Shared)

CPU 3
0000-7fff ROM
8000-8fff RAM


  Here goes a list of known deficiencies of our drivers:

  - The bootleg romset is functional. The original one hangs at
    the title screen (protection).

  - Sound support is probably incomplete. There are a couple of unknown
    accesses done by the CPU, including to the YM2203 I/O ports. At the
	very least, there should be some filters.

  - "fake-r" routine make the "original" roms to restart the game after
    some seconds.

    Well, we know very little about the 0xFE00 address. It could be
    some watchdog or a synchronization timer.

    I remember scanning the main CPU code to find how it was
    used on the bootleg set. Then I just figured out a constant value
    that made the game run (it hang if just set unhandled, that is,
    returning zero).

    Maybe the solution is to patch the bootleg ROMs to skip some tests
    at this location (I remember some of them being in the
    initialization routine of the main CPU).

                       Marcelo de G. Malheiros <malheiro@dca.fee.unicamp.br>
                                                                   1998.9.25

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m6805/m6805.h"

static int ddr1, ddr2, ddr3, ddr4;
static int port1_in, port2_in, port3_in, port4_in;
static int port1_out, port2_out, port3_out, port4_out;

/* vidhrdw/bublbobl.c */
extern unsigned char *bublbobl_objectram;
extern size_t bublbobl_objectram_size;
VIDEO_UPDATE( bublbobl );

/* machine/bublbobl.c */
extern unsigned char *bublbobl_sharedram1,*bublbobl_sharedram2;
READ_HANDLER( bublbobl_sharedram1_r );
READ_HANDLER( bublbobl_sharedram2_r );
WRITE_HANDLER( bublbobl_sharedram1_w );
WRITE_HANDLER( bublbobl_sharedram2_w );
INTERRUPT_GEN( bublbobl_m68705_interrupt );
READ_HANDLER( bublbobl_68705_portA_r );
WRITE_HANDLER( bublbobl_68705_portA_w );
WRITE_HANDLER( bublbobl_68705_ddrA_w );
READ_HANDLER( bublbobl_68705_portB_r );
WRITE_HANDLER( bublbobl_68705_portB_w );
WRITE_HANDLER( bublbobl_68705_ddrB_w );
WRITE_HANDLER( bublbobl_bankswitch_w );
WRITE_HANDLER( tokio_bankswitch_w );
WRITE_HANDLER( tokio_videoctrl_w );
WRITE_HANDLER( bublbobl_nmitrigger_w );
WRITE_HANDLER( bublbobl_sound_command_w );
WRITE_HANDLER( bublbobl_sh_nmi_disable_w );
WRITE_HANDLER( bublbobl_sh_nmi_enable_w );


READ_HANDLER( bublbobl_mcu_ddr1_r )
{
	return ddr1;
}

WRITE_HANDLER( bublbobl_mcu_ddr1_w )
{
	ddr1 = data;
}

READ_HANDLER( bublbobl_mcu_ddr2_r )
{
	return ddr2;
}

WRITE_HANDLER( bublbobl_mcu_ddr2_w )
{
	ddr2 = data;
}

READ_HANDLER( bublbobl_mcu_ddr3_r )
{
	return ddr3;
}

WRITE_HANDLER( bublbobl_mcu_ddr3_w )
{
	ddr3 = data;
}

READ_HANDLER( bublbobl_mcu_ddr4_r )
{
	return ddr4;
}

WRITE_HANDLER( bublbobl_mcu_ddr4_w )
{
	ddr4 = data;
}

READ_HANDLER( bublbobl_mcu_port1_r )
{
	port1_in = readinputport(0);
	return (port1_out & ddr1) | (port1_in & ~ddr1);
}

WRITE_HANDLER( bublbobl_mcu_port1_w )
{
	coin_lockout_w( 0, (~data & 0x10) );
	coin_lockout_w( 1, (~data & 0x10) );

	if ((port1_out & 0x40) && (~data & 0x40))
	{
		cpu_irq_line_vector_w(0, 0, bublbobl_sharedram2[0]);
		cpu_set_irq_line(0, 0, HOLD_LINE);
	}

	/* bit 7: select read or write shared RAM*/

	port1_out = data;
}

READ_HANDLER( bublbobl_mcu_port2_r )
{
	return (port2_out & ddr2) | (port2_in & ~ddr2);
}

WRITE_HANDLER( bublbobl_mcu_port2_w )
{
	if ((~port2_out & 0x10) && (data & 0x10))
	{
		int address = port4_out | ((data & 0x0f) << 8);

		if (port1_out & 0x80)
		{
			/* read*/
			if ((address & 0x0800) == 0x0000)
				port3_in = readinputport((address & 3) + 1);
			else if ((address & 0x0c00) == 0x0c00)
				port3_in = bublbobl_sharedram2[address & 0x03ff];
		}
		else
		{
			if ((address & 0x0c00) == 0x0c00)
				bublbobl_sharedram2[address & 0x03ff] = port3_out;
		}
	}

	port2_out = data;
}

READ_HANDLER( bublbobl_mcu_port3_r )
{
	return (port3_out & ddr3) | (port3_in & ~ddr3);
}

WRITE_HANDLER( bublbobl_mcu_port3_w )
{
	port3_out = data;
}

READ_HANDLER( bublbobl_mcu_port4_r )
{
	return (port4_out & ddr4) | (port4_in & ~ddr4);
}

WRITE_HANDLER( bublbobl_mcu_port4_w )
{
	port4_out = data;
}

/* tokio mcu */
READ_HANDLER ( tokio_68705_portA_r );
WRITE_HANDLER( tokio_68705_portA_w );
READ_HANDLER ( tokio_68705_portB_r );
WRITE_HANDLER( tokio_68705_portB_w );
READ_HANDLER ( tokio_68705_portC_r );
WRITE_HANDLER( tokio_68705_portC_w );
WRITE_HANDLER( tokio_68705_ddrA_w );
WRITE_HANDLER( tokio_68705_ddrB_w );
WRITE_HANDLER( tokio_68705_ddrC_w );
WRITE_HANDLER( tokio_mcu_w );
READ_HANDLER ( tokio_mcu_r );
READ_HANDLER ( tokio_mcu_status_r );
READ_HANDLER ( tokio_fake_r ); /* faked mcu commands for bootleg */

static unsigned char from_main,from_mcu;
static int mcu_sent = 0,main_sent = 0;
static unsigned char portA_in,portA_out,ddrA;
static unsigned char portB_in,portB_out,ddrB;
static unsigned char portC_in,portC_out,ddrC;

READ_HANDLER( tokio_68705_portA_r )
{
	return (portA_out & ddrA) | (portA_in & ~ddrA);
}

WRITE_HANDLER( tokio_68705_portA_w )
{
	portA_out = data;
}

WRITE_HANDLER( tokio_68705_ddrA_w )
{
	ddrA = data;
}

READ_HANDLER( tokio_68705_portB_r )
{
	return (portB_out & ddrB) | (portB_in & ~ddrB);
}

WRITE_HANDLER( tokio_68705_portB_w )
{
	if ((ddrB & 0x02) && (~data & 0x02) && (portB_out & 0x02))
	{
		portA_in = from_main;
		if (main_sent) cpu_set_irq_line(3,0,CLEAR_LINE);
		main_sent = 0;
	}
	if ((ddrB & 0x04) && (data & 0x04) && (~portB_out & 0x04))
	{
		from_mcu = portA_out;
		mcu_sent = 1;
	}

	portB_out = data;
}

WRITE_HANDLER( tokio_68705_ddrB_w )
{
	ddrB = data;
}


READ_HANDLER( tokio_68705_portC_r )
{
	data8_t ret = 0;

	if (!main_sent)
		ret |= 0x01;
	if (mcu_sent)
		ret |= 0x02;

	ret ^= 0x3; /* inverted logic compared to tigerh */

	return ret;

}

WRITE_HANDLER( tokio_68705_portC_w )
{
	portC_out = data;
}

WRITE_HANDLER( tokio_68705_ddrC_w )
{
	ddrC = data;
}

WRITE_HANDLER( tokio_mcu_w )
{
	from_main = data;
	main_sent = 1;
	cpu_set_irq_line(3,0,ASSERT_LINE);
}

READ_HANDLER( tokio_mcu_r )
{
	mcu_sent = 0;
	return from_mcu;
}

READ_HANDLER( tokio_mcu_status_r )
{
    int res = input_port_2_r(0);
 
    res &= ~0x30; /* clear 3 bits in top nibble to make room for mcu status */
 
    if (!main_sent)
        res |= 0x10;
    if (!mcu_sent)
        res |= 0x20;
 
    return res;
}


static MEMORY_READ_START( bublbobl_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xf7ff, bublbobl_sharedram1_r },
	{ 0xf800, 0xf9ff, paletteram_r },
	{ 0xfc00, 0xffff, bublbobl_sharedram2_r },
MEMORY_END

static MEMORY_WRITE_START( bublbobl_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xdcff, MWA_RAM, &videoram, &videoram_size },
	{ 0xdd00, 0xdfff, MWA_RAM, &bublbobl_objectram, &bublbobl_objectram_size },
	{ 0xe000, 0xf7ff, bublbobl_sharedram1_w, &bublbobl_sharedram1 },
	{ 0xf800, 0xf9ff, paletteram_RRRRGGGGBBBBxxxx_swap_w, &paletteram },
	{ 0xfa00, 0xfa00, bublbobl_sound_command_w },
/*	{ 0xfa03, 0xfa03,  }, clocks reset to sound cpu*/
	{ 0xfa80, 0xfa80, watchdog_reset_w },
	{ 0xfb00, 0xfb00, bublbobl_nmitrigger_w },	/* not used by Bubble Bobble, only by Tokio */
	{ 0xfb40, 0xfb40, bublbobl_bankswitch_w },
	{ 0xfc00, 0xffff, bublbobl_sharedram2_w, &bublbobl_sharedram2 },
MEMORY_END

static MEMORY_READ_START( m6801_readmem )
    { 0x0000, 0x0000, bublbobl_mcu_ddr1_r },
	{ 0x0001, 0x0001, bublbobl_mcu_ddr2_r },
	{ 0x0002, 0x0002, bublbobl_mcu_port1_r },
	{ 0x0003, 0x0003, bublbobl_mcu_port2_r },
	{ 0x0004, 0x0004, bublbobl_mcu_ddr3_r },
	{ 0x0005, 0x0005, bublbobl_mcu_ddr4_r },
	{ 0x0006, 0x0006, bublbobl_mcu_port3_r },
	{ 0x0007, 0x0007, bublbobl_mcu_port4_r },
	{ 0x0040, 0x00ff, MRA_RAM },
	{ 0xf000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( m6801_writemem )
    { 0x0000, 0x0000, bublbobl_mcu_ddr1_w },
	{ 0x0001, 0x0001, bublbobl_mcu_ddr2_w },
	{ 0x0002, 0x0002, bublbobl_mcu_port1_w },
	{ 0x0003, 0x0003, bublbobl_mcu_port2_w },
	{ 0x0004, 0x0004, bublbobl_mcu_ddr3_w },
	{ 0x0005, 0x0005, bublbobl_mcu_ddr4_w },
	{ 0x0006, 0x0006, bublbobl_mcu_port3_w },
	{ 0x0007, 0x0007, bublbobl_mcu_port4_w },
	{ 0x0040, 0x00ff, MWA_RAM },
	{ 0xf000, 0xffff, MWA_ROM },
MEMORY_END


static MEMORY_READ_START( boblbobl_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xf7ff, bublbobl_sharedram1_r },
	{ 0xf800, 0xf9ff, paletteram_r },
	{ 0xfc00, 0xfcff, bublbobl_sharedram2_r },
	{ 0xff00, 0xff00, input_port_0_r },
	{ 0xff01, 0xff01, input_port_1_r },
	{ 0xff02, 0xff02, input_port_2_r },
	{ 0xff03, 0xff03, input_port_3_r },
MEMORY_END

static MEMORY_WRITE_START( boblbobl_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xdcff, MWA_RAM, &videoram, &videoram_size },
	{ 0xdd00, 0xdfff, MWA_RAM, &bublbobl_objectram, &bublbobl_objectram_size },
	{ 0xe000, 0xf7ff, bublbobl_sharedram1_w, &bublbobl_sharedram1 },
	{ 0xf800, 0xf9ff, paletteram_RRRRGGGGBBBBxxxx_swap_w, &paletteram },
	{ 0xfa00, 0xfa00, bublbobl_sound_command_w },
	{ 0xfa80, 0xfa80, MWA_NOP },
	{ 0xfb00, 0xfb00, bublbobl_nmitrigger_w },	/* not used by Bubble Bobble, only by Tokio */
	{ 0xfb40, 0xfb40, bublbobl_bankswitch_w },
	{ 0xfc00, 0xfcff, bublbobl_sharedram2_w, &bublbobl_sharedram2 },
MEMORY_END

static MEMORY_READ_START( bublbobl_readmem2 )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xe000, 0xf7ff, bublbobl_sharedram1_r },
MEMORY_END

static MEMORY_WRITE_START( bublbobl_writemem2 )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xe000, 0xf7ff, bublbobl_sharedram1_w },
MEMORY_END


static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x8fff, MRA_RAM },
	{ 0x9000, 0x9000, YM2203_status_port_0_r },
	{ 0x9001, 0x9001, YM2203_read_port_0_r },
	{ 0xa000, 0xa000, YM3526_status_port_0_r },
	{ 0xb000, 0xb000, soundlatch_r },
	{ 0xb001, 0xb001, MRA_NOP },	/* bit 0: message pending for main cpu */
									/* bit 1: message pending for sound cpu */
	{ 0xe000, 0xefff, MRA_ROM },	/* space for diagnostic ROM? */
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x8fff, MWA_RAM },
	{ 0x9000, 0x9000, YM2203_control_port_0_w },
	{ 0x9001, 0x9001, YM2203_write_port_0_w },
	{ 0xa000, 0xa000, YM3526_control_port_0_w },
	{ 0xa001, 0xa001, YM3526_write_port_0_w },
	{ 0xb000, 0xb000, MWA_NOP },	/* message for main cpu */
	{ 0xb001, 0xb001, bublbobl_sh_nmi_enable_w },
	{ 0xb002, 0xb002, bublbobl_sh_nmi_disable_w },
	{ 0xe000, 0xefff, MWA_ROM },	/* space for diagnostic ROM? */
MEMORY_END


static MEMORY_READ_START( tokio_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xf7ff, bublbobl_sharedram1_r },
	{ 0xf800, 0xf9ff, paletteram_r },
	{ 0xfa03, 0xfa03, input_port_0_r },
	{ 0xfa04, 0xfa04, input_port_1_r },
	{ 0xfa05, 0xfa05, tokio_mcu_status_r }, /* M68705 commands */
	{ 0xfa06, 0xfa06, input_port_3_r },
	{ 0xfa07, 0xfa07, input_port_4_r },
	{ 0xfe00, 0xfe00, tokio_mcu_r },
MEMORY_END

static MEMORY_WRITE_START( tokio_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xdcff, MWA_RAM, &videoram, &videoram_size },
	{ 0xdd00, 0xdfff, MWA_RAM, &bublbobl_objectram, &bublbobl_objectram_size },
	{ 0xe000, 0xf7ff, bublbobl_sharedram1_w, &bublbobl_sharedram1 },
	{ 0xf800, 0xf9ff, paletteram_RRRRGGGGBBBBxxxx_swap_w, &paletteram },
	{ 0xfa00, 0xfa00, MWA_NOP },
	{ 0xfa80, 0xfa80, tokio_bankswitch_w },
	{ 0xfb00, 0xfb00, tokio_videoctrl_w },
	{ 0xfb80, 0xfb80, bublbobl_nmitrigger_w },
	{ 0xfc00, 0xfc00, bublbobl_sound_command_w },
	{ 0xfe00, 0xfe00, tokio_mcu_w },
MEMORY_END

/* bootleg uses fake mcu hookup */
static MEMORY_READ_START( tokiob_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xf7ff, bublbobl_sharedram1_r },
	{ 0xf800, 0xf9ff, paletteram_r },
	{ 0xfa03, 0xfa03, input_port_0_r },
	{ 0xfa04, 0xfa04, input_port_1_r },
	{ 0xfa05, 0xfa05, input_port_2_r },
	{ 0xfa06, 0xfa06, input_port_3_r },
	{ 0xfa07, 0xfa07, input_port_4_r },
	{ 0xfe00, 0xfe00, tokio_fake_r },
MEMORY_END

static MEMORY_WRITE_START( tokiob_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xdcff, MWA_RAM, &videoram, &videoram_size },
	{ 0xdd00, 0xdfff, MWA_RAM, &bublbobl_objectram, &bublbobl_objectram_size },
	{ 0xe000, 0xf7ff, bublbobl_sharedram1_w, &bublbobl_sharedram1 },
	{ 0xf800, 0xf9ff, paletteram_RRRRGGGGBBBBxxxx_swap_w, &paletteram },
	{ 0xfa00, 0xfa00, MWA_NOP },
	{ 0xfa80, 0xfa80, tokio_bankswitch_w },
	{ 0xfb00, 0xfb00, tokio_videoctrl_w },
	{ 0xfb80, 0xfb80, bublbobl_nmitrigger_w },
	{ 0xfc00, 0xfc00, bublbobl_sound_command_w },
	{ 0xfe00, 0xfe00, MWA_NOP }, /* ??? */
MEMORY_END

static MEMORY_READ_START( tokio_readmem2 )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x97ff, bublbobl_sharedram1_r },
MEMORY_END

static MEMORY_WRITE_START( tokio_writemem2 )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x97ff, bublbobl_sharedram1_w },
MEMORY_END

static MEMORY_READ_START( tokio_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x8fff, MRA_RAM },
	{ 0x9000, 0x9000, soundlatch_r },
	{ 0x9800, 0x9800, MRA_NOP },	/* ??? */
	{ 0xb000, 0xb000, YM2203_status_port_0_r },
	{ 0xb001, 0xb001, YM2203_read_port_0_r },
	{ 0xe000, 0xffff, MRA_ROM },	/* space for diagnostic ROM? */
MEMORY_END

static MEMORY_WRITE_START( tokio_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x8fff, MWA_RAM },
	{ 0x9000, 0x9000, MWA_NOP },	/* ??? */
	{ 0xa000, 0xa000, bublbobl_sh_nmi_disable_w },
	{ 0xa800, 0xa800, bublbobl_sh_nmi_enable_w },
	{ 0xb000, 0xb000, YM2203_control_port_0_w },
	{ 0xb001, 0xb001, YM2203_write_port_0_w },
	{ 0xe000, 0xffff, MWA_ROM },	/* space for diagnostic ROM? */
MEMORY_END

static MEMORY_READ_START( tokio_m68705_readmem )
	{ 0x0000, 0x0000, tokio_68705_portA_r },
	{ 0x0001, 0x0001, tokio_68705_portB_r },
	{ 0x0002, 0x0002, tokio_68705_portC_r },
	{ 0x0010, 0x007f, MRA_RAM },
	{ 0x0080, 0x07ff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( tokio_m68705_writemem )
	{ 0x0000, 0x0000, tokio_68705_portA_w },
	{ 0x0001, 0x0001, tokio_68705_portB_w },
	{ 0x0002, 0x0002, tokio_68705_portC_w },
	{ 0x0004, 0x0004, tokio_68705_ddrA_w },
	{ 0x0005, 0x0005, tokio_68705_ddrB_w },
	{ 0x0006, 0x0006, tokio_68705_ddrC_w },
	{ 0x0010, 0x007f, MWA_RAM },
	{ 0x0080, 0x07ff, MWA_ROM },
MEMORY_END

INPUT_PORTS_START( bublbobl )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SPECIAL )	/* output: coin lockout*/
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SPECIAL )	/* output: select 1-way or 2-way coin counter*/
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SPECIAL )	/* output: trigger IRQ on main CPU (jumper switchable to vblank)*/
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SPECIAL )	/* output: select read or write shared RAM*/

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, "Language" )
	PORT_DIPSETTING(    0x01, "Japanese" )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Medium" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x08, "20000 80000" )
	PORT_DIPSETTING(    0x0c, "30000 100000" )
	PORT_DIPSETTING(    0x04, "40000 200000" )
	PORT_DIPSETTING(    0x00, "50000 250000" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x10, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( bublboblp )
    PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x01, 0x00, "Language" )
	PORT_DIPSETTING(    0x01, "Japanese" )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Medium" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x08, "20000 80000" )
	PORT_DIPSETTING(    0x0c, "30000 100000" )
	PORT_DIPSETTING(    0x04, "40000 200000" )
	PORT_DIPSETTING(    0x00, "50000 250000" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x10, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "5" )
    PORT_DIPNAME( 0x40, 0x40, "Edit Mode" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_2WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_2WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( boblbobl )
	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, "Language" )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPSETTING(    0x01, "Japanese" )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Medium" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x08, "20000 80000" )
	PORT_DIPSETTING(    0x0c, "30000 100000" )
	PORT_DIPSETTING(    0x04, "40000 200000" )
	PORT_DIPSETTING(    0x00, "50000 250000" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x10, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_DIPNAME( 0xc0, 0x00, "Monster Speed" )
	PORT_DIPSETTING(    0x00, "Normal" )
	PORT_DIPSETTING(    0x40, "Medium" )
	PORT_DIPSETTING(    0x80, "High" )
	PORT_DIPSETTING(    0xc0, "Very High" )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_TILT ) /* ?????*/
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( sboblbob )
	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, "Game" )
	PORT_DIPSETTING(    0x01, "Bobble Bobble" )
	PORT_DIPSETTING(    0x00, "Super Bobble Bobble" )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Medium" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x08, "20000 80000" )
	PORT_DIPSETTING(    0x0c, "30000 100000" )
	PORT_DIPSETTING(    0x04, "40000 200000" )
	PORT_DIPSETTING(    0x00, "50000 250000" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x10, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_BITX( 0,       0x20, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "100", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0xc0, 0x00, "Monster Speed" )
	PORT_DIPSETTING(    0x00, "Normal" )
	PORT_DIPSETTING(    0x40, "Medium" )
	PORT_DIPSETTING(    0x80, "High" )
	PORT_DIPSETTING(    0xc0, "Very High" )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_TILT ) /* ?????*/
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( tokio )
	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, "Enemies" )
	PORT_DIPSETTING(    0x01, "Few (Easy)" )
	PORT_DIPSETTING(    0x00, "Many (Hard)" )
	PORT_DIPNAME( 0x02, 0x02, "Enemy Shots" )
	PORT_DIPSETTING(    0x02, "Few (Easy)" )
	PORT_DIPSETTING(    0x00, "Many (Hard)" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x0c, "100K 400K" )
	PORT_DIPSETTING(    0x08, "200K 400K" )
	PORT_DIPSETTING(    0x04, "300K 400K" )
	PORT_DIPSETTING(    0x00, "400K 400K" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "99", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "Language" )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPSETTING(    0x80, "Japanese" )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN2 )
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_SPECIAL ) /* M68705 commands */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_SPECIAL ) /* M68705 commands */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

/* bootleg no mcu */
INPUT_PORTS_START( tokiob )
	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, "Enemies" )
	PORT_DIPSETTING(    0x01, "Few (Easy)" )
	PORT_DIPSETTING(    0x00, "Many (Hard)" )
	PORT_DIPNAME( 0x02, 0x02, "Enemy Shots" )
	PORT_DIPSETTING(    0x02, "Few (Easy)" )
	PORT_DIPSETTING(    0x00, "Many (Hard)" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x0c, "100K 400K" )
	PORT_DIPSETTING(    0x08, "200K 400K" )
	PORT_DIPSETTING(    0x04, "300K 400K" )
	PORT_DIPSETTING(    0x00, "400K 400K" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "99", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "Language" )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPSETTING(    0x80, "Japanese" )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


static struct GfxLayout charlayout =
{
	8,8,	/* the characters are 8x8 pixels */
	256*8*8,	/* 256 chars per bank * 8 banks per ROM pair * 8 ROM pairs */
	4,	/* 4 bits per pixel */
	{ 0, 4, 8*0x8000*8, 8*0x8000*8+4 },
	{ 3, 2, 1, 0, 8+3, 8+2, 8+1, 8+0 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8	/* every char takes 16 bytes in two ROMs */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	/* read all graphics into one big graphics region */
	{ REGION_GFX1, 0x00000, &charlayout, 0, 16 },
	{ -1 }	/* end of array */
};



#define MAIN_XTAL 24000000

/* handler called by the 2203 emulator when the internal timers cause an IRQ */
static void irqhandler(int irq)
{
	cpu_set_irq_line(2,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2203interface ym2203_interface =
{
	1,			/* 1 chip */
	MAIN_XTAL/8,	/* 3 MHz */
	{ YM2203_VOL(25,25) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler }
};


static struct YM3526interface ym3526_interface =
{
	1,			/* 1 chip (no more supported) */
	MAIN_XTAL/8,	/* 3 MHz */
	{ 50 }		/* volume */
};


static struct YM2203interface tokio_ym2203_interface =
{
	1,		/* 1 chip */
	MAIN_XTAL/8,	/* 3 MHz */
	{ YM2203_VOL(100,10) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler }
};


static MACHINE_DRIVER_START( bublbobl )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, MAIN_XTAL/4)	/* 6 MHz */
	MDRV_CPU_MEMORY(bublbobl_readmem,bublbobl_writemem)

	MDRV_CPU_ADD(Z80, MAIN_XTAL/4)	/* 6 MHz */
	MDRV_CPU_MEMORY(bublbobl_readmem2,bublbobl_writemem2)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, MAIN_XTAL/8)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 3 MHz */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
								/* IRQs are triggered by the YM2203 */

	MDRV_CPU_ADD(M6801,4000000/2)	/* xtal is 4MHz, I think it's divided by 2 internally */
	MDRV_CPU_MEMORY(m6801_readmem,m6801_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_pulse,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1000)	/* 100 CPU slices per frame - an high value to ensure proper */
							/* synchronization of the CPUs */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_UPDATE(bublbobl)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, ym2203_interface)
	MDRV_SOUND_ADD(YM3526, ym3526_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( boblbobl )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, MAIN_XTAL/4)	/* 6 MHz */
	MDRV_CPU_MEMORY(boblbobl_readmem,boblbobl_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)	/* interrupt mode 1, unlike Bubble Bobble */

	MDRV_CPU_ADD(Z80, MAIN_XTAL/4)	/* 6 MHz */
	MDRV_CPU_MEMORY(bublbobl_readmem2,bublbobl_writemem2)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, MAIN_XTAL/8)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 3 MHz */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
								/* IRQs are triggered by the YM2203 */
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)	/* 100 CPU slices per frame - an high value to ensure proper */
							/* synchronization of the CPUs */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_UPDATE(bublbobl)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, ym2203_interface)
	MDRV_SOUND_ADD(YM3526, ym3526_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( tokio )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, MAIN_XTAL/4)	/* 6 MHz */
	MDRV_CPU_MEMORY(tokio_readmem,tokio_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, MAIN_XTAL/4)	/* 6 MHz */
	MDRV_CPU_MEMORY(tokio_readmem2,tokio_writemem2)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, MAIN_XTAL/8)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 3 MHz */
	MDRV_CPU_MEMORY(tokio_sound_readmem,tokio_sound_writemem)
						/* NMIs are triggered by the main CPU */
						/* IRQs are triggered by the YM2203 */
	MDRV_CPU_ADD(M68705, MAIN_XTAL/8)  /* 3 MHz */
	MDRV_CPU_MEMORY(tokio_m68705_readmem,tokio_m68705_writemem)
		

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION) /* frames/second, vblank duration */
	MDRV_INTERLEAVE(100)	/* 100 CPU slices per frame - an high value to ensure proper */
							/* synchronization of the CPUs */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_UPDATE(bublbobl)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, tokio_ym2203_interface)
MACHINE_DRIVER_END

/* Bubble Bobble Prototype - Tokio bootleg hookup no mcu */
static MACHINE_DRIVER_START( tokiob )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, MAIN_XTAL/4)	/* 6 MHz */
	MDRV_CPU_MEMORY(tokiob_readmem,tokiob_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, MAIN_XTAL/4)	/* 6 MHz */
	MDRV_CPU_MEMORY(tokio_readmem2,tokio_writemem2)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, MAIN_XTAL/8)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 3 MHz */
	MDRV_CPU_MEMORY(tokio_sound_readmem,tokio_sound_writemem)
						/* NMIs are triggered by the main CPU */
						/* IRQs are triggered by the YM2203 */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION) /* frames/second, vblank duration */
	MDRV_INTERLEAVE(100)	/* 100 CPU slices per frame - an high value to ensure proper */
							/* synchronization of the CPUs */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_UPDATE(bublbobl)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, tokio_ym2203_interface)
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

/*

bublbobl.zip - a78-05-1.52
TAITO CORPORATION 1986
ALL RIGHTS RESERVED
VER 0.1 4.SEP,1986 SUMMER

Name          Size    CRC32       Chip Type
-------------------------------------------
a78-05-1.52    65536  0x9f8ee242  Fujitsu MBM27C512
a78-06-1.51    32768  0x567934b6  Intel D27256

*/

ROM_START( bublbobl )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )
	ROM_LOAD( "a78-06-1.51",    0x00000, 0x08000, CRC(567934b6) SHA1(b0c4d49fd551f465d148c25c3e80b278835e2f0d) )
	/* ROMs banked at 8000-bfff */
	ROM_LOAD( "a78-05-1.52",    0x10000, 0x10000, CRC(9f8ee242) SHA1(924150d4e7e087a9b2b0a294c2d0e9903a266c6c) )
	/* 20000-2ffff empty */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the second CPU */
	ROM_LOAD( "a78-08.37",    0x0000, 0x08000, CRC(ae11a07b) SHA1(af7a335c8da637103103cc274e077f123908ebb7) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* 64k for the third CPU */
	ROM_LOAD( "a78-07.46",    0x0000, 0x08000, CRC(4f9a26e8) SHA1(3105b34b88a7134493c2b3f584729f8b0407a011) )

	ROM_REGION( 0x10000, REGION_CPU4, 0 )	/* 64k for the MCU */
	ROM_LOAD( "a78-01.17",    0xf000, 0x1000, CRC(b1bfb53d) SHA1(31b8f31acd3aa394acd80db362774749842e1285) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "a78-09.12",    0x00000, 0x8000, CRC(20358c22) SHA1(2297af6c53d5807bf90a8e081075b8c72a994fc5) )    /* 1st plane */
	ROM_LOAD( "a78-10.13",    0x08000, 0x8000, CRC(930168a9) SHA1(fd358c3c3b424bca285f67a1589eb98a345ff670) )
	ROM_LOAD( "a78-11.14",    0x10000, 0x8000, CRC(9773e512) SHA1(33c1687ee575d66bf0e98add45d06da827813765) )
	ROM_LOAD( "a78-12.15",    0x18000, 0x8000, CRC(d045549b) SHA1(0c12077d3ddc2ce6aa45a0224ad5540f3f218446) )
	ROM_LOAD( "a78-13.16",    0x20000, 0x8000, CRC(d0af35c5) SHA1(c5a89f4d73acc0db86654540b3abfd77b3757db5) )
	ROM_LOAD( "a78-14.17",    0x28000, 0x8000, CRC(7b5369a8) SHA1(1307b26d80e6f36ebe6c442bebec41d20066eaf9) )
	/* 0x30000-0x3ffff empty */
	ROM_LOAD( "a78-15.30",    0x40000, 0x8000, CRC(6b61a413) SHA1(44eddf12fb46fceca2addbe6da929aaea7636b13) )    /* 2nd plane */
	ROM_LOAD( "a78-16.31",    0x48000, 0x8000, CRC(b5492d97) SHA1(d5b045e3ebaa44809757a4220cefb3c6815470da) )
	ROM_LOAD( "a78-17.32",    0x50000, 0x8000, CRC(d69762d5) SHA1(3326fef4e0bd86681a3047dc11886bb171ecb609) )
	ROM_LOAD( "a78-18.33",    0x58000, 0x8000, CRC(9f243b68) SHA1(32dce8d311a4be003693182a999e4053baa6bb0a) )
	ROM_LOAD( "a78-19.34",    0x60000, 0x8000, CRC(66e9438c) SHA1(b94e62b6fbe7f4e08086d0365afc5cff6e0ccafd) )
	ROM_LOAD( "a78-20.35",    0x68000, 0x8000, CRC(9ef863ad) SHA1(29f91b5a3765e4d6e6c3382db1d8d8297b6e56c8) )
	/* 0x70000-0x7ffff empty */

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "a71-25.41",    0x0000, 0x0100, CRC(2d0f8545) SHA1(089c31e2f614145ef2743164f7b52ae35bc06808) )	/* video timing */
ROM_END

/*
bublbob1.zip - a78-05.52
TAITO CORPORATION 1986
ALL RIGHTS RESERVED
VER 0.018.AUG,1986 SUMMER
*/

ROM_START( bublbob1 )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )
	ROM_LOAD( "a78-06.51",    0x00000, 0x08000, CRC(32c8305b) SHA1(6bf69b3edfbefd33cd670a762b4bf0b39629a220) )
    /* ROMs banked at 8000-bfff */
	ROM_LOAD( "a78-05.52",    0x10000, 0x10000, CRC(53f4bc6e) SHA1(15a2e6d83438d4136b154b3d90dd2cf9f1ce572c) )
	/* 20000-2ffff empty */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the second CPU */
	ROM_LOAD( "a78-08.37",    0x0000, 0x08000, CRC(ae11a07b) SHA1(af7a335c8da637103103cc274e077f123908ebb7) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* 64k for the third CPU */
	ROM_LOAD( "a78-07.46",    0x0000, 0x08000, CRC(4f9a26e8) SHA1(3105b34b88a7134493c2b3f584729f8b0407a011) )

	ROM_REGION( 0x10000, REGION_CPU4, 0 )	/* 64k for the MCU */
	ROM_LOAD( "a78-01.17",    0xf000, 0x1000, CRC(b1bfb53d) SHA1(31b8f31acd3aa394acd80db362774749842e1285) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "a78-09.12",    0x00000, 0x8000, CRC(20358c22) SHA1(2297af6c53d5807bf90a8e081075b8c72a994fc5) )    /* 1st plane */
	ROM_LOAD( "a78-10.13",    0x08000, 0x8000, CRC(930168a9) SHA1(fd358c3c3b424bca285f67a1589eb98a345ff670) )
	ROM_LOAD( "a78-11.14",    0x10000, 0x8000, CRC(9773e512) SHA1(33c1687ee575d66bf0e98add45d06da827813765) )
	ROM_LOAD( "a78-12.15",    0x18000, 0x8000, CRC(d045549b) SHA1(0c12077d3ddc2ce6aa45a0224ad5540f3f218446) )
	ROM_LOAD( "a78-13.16",    0x20000, 0x8000, CRC(d0af35c5) SHA1(c5a89f4d73acc0db86654540b3abfd77b3757db5) )
	ROM_LOAD( "a78-14.17",    0x28000, 0x8000, CRC(7b5369a8) SHA1(1307b26d80e6f36ebe6c442bebec41d20066eaf9) )
	/* 0x30000-0x3ffff empty */
	ROM_LOAD( "a78-15.30",    0x40000, 0x8000, CRC(6b61a413) SHA1(44eddf12fb46fceca2addbe6da929aaea7636b13) )    /* 2nd plane */
	ROM_LOAD( "a78-16.31",    0x48000, 0x8000, CRC(b5492d97) SHA1(d5b045e3ebaa44809757a4220cefb3c6815470da) )
	ROM_LOAD( "a78-17.32",    0x50000, 0x8000, CRC(d69762d5) SHA1(3326fef4e0bd86681a3047dc11886bb171ecb609) )
	ROM_LOAD( "a78-18.33",    0x58000, 0x8000, CRC(9f243b68) SHA1(32dce8d311a4be003693182a999e4053baa6bb0a) )
	ROM_LOAD( "a78-19.34",    0x60000, 0x8000, CRC(66e9438c) SHA1(b94e62b6fbe7f4e08086d0365afc5cff6e0ccafd) )
	ROM_LOAD( "a78-20.35",    0x68000, 0x8000, CRC(9ef863ad) SHA1(29f91b5a3765e4d6e6c3382db1d8d8297b6e56c8) )
	/* 0x70000-0x7ffff empty */

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "a71-25.41",    0x0000, 0x0100, CRC(2d0f8545) SHA1(089c31e2f614145ef2743164f7b52ae35bc06808) )	/* video timing */
ROM_END

/*
bublbobr.zip - a78-24.52
1986 TAITO AMERICA CORP.
LICENSED TO ROMSTAR FOR U.S.A.
VER 5.1 8.NOV,1986 SUMMER
*/

ROM_START( bublbobr )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )
	ROM_LOAD( "a78-25.51",    0x00000, 0x08000, CRC(2d901c9d) SHA1(72504225d3a26212e8f35508a79200eeb91138b6) )
    /* ROMs banked at 8000-bfff */
	ROM_LOAD( "a78-24.52",    0x10000, 0x10000, CRC(b7afedc4) SHA1(6e4c8712f1fdf000e231cfd622dd3b514c61a6fd) )
	/* 20000-2ffff empty */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the second CPU */
	ROM_LOAD( "a78-08.37",    0x0000, 0x08000, CRC(ae11a07b) SHA1(af7a335c8da637103103cc274e077f123908ebb7) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* 64k for the third CPU */
	ROM_LOAD( "a78-07.46",    0x0000, 0x08000, CRC(4f9a26e8) SHA1(3105b34b88a7134493c2b3f584729f8b0407a011) )

	ROM_REGION( 0x10000, REGION_CPU4, 0 )	/* 64k for the MCU */
	ROM_LOAD( "a78-01.17",    0xf000, 0x1000, CRC(b1bfb53d) SHA1(31b8f31acd3aa394acd80db362774749842e1285) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "a78-09.12",    0x00000, 0x8000, CRC(20358c22) SHA1(2297af6c53d5807bf90a8e081075b8c72a994fc5) )    /* 1st plane */
	ROM_LOAD( "a78-10.13",    0x08000, 0x8000, CRC(930168a9) SHA1(fd358c3c3b424bca285f67a1589eb98a345ff670) )
	ROM_LOAD( "a78-11.14",    0x10000, 0x8000, CRC(9773e512) SHA1(33c1687ee575d66bf0e98add45d06da827813765) )
	ROM_LOAD( "a78-12.15",    0x18000, 0x8000, CRC(d045549b) SHA1(0c12077d3ddc2ce6aa45a0224ad5540f3f218446) )
	ROM_LOAD( "a78-13.16",    0x20000, 0x8000, CRC(d0af35c5) SHA1(c5a89f4d73acc0db86654540b3abfd77b3757db5) )
	ROM_LOAD( "a78-14.17",    0x28000, 0x8000, CRC(7b5369a8) SHA1(1307b26d80e6f36ebe6c442bebec41d20066eaf9) )
	/* 0x30000-0x3ffff empty */
	ROM_LOAD( "a78-15.30",    0x40000, 0x8000, CRC(6b61a413) SHA1(44eddf12fb46fceca2addbe6da929aaea7636b13) )    /* 2nd plane */
	ROM_LOAD( "a78-16.31",    0x48000, 0x8000, CRC(b5492d97) SHA1(d5b045e3ebaa44809757a4220cefb3c6815470da) )
	ROM_LOAD( "a78-17.32",    0x50000, 0x8000, CRC(d69762d5) SHA1(3326fef4e0bd86681a3047dc11886bb171ecb609) )
	ROM_LOAD( "a78-18.33",    0x58000, 0x8000, CRC(9f243b68) SHA1(32dce8d311a4be003693182a999e4053baa6bb0a) )
	ROM_LOAD( "a78-19.34",    0x60000, 0x8000, CRC(66e9438c) SHA1(b94e62b6fbe7f4e08086d0365afc5cff6e0ccafd) )
	ROM_LOAD( "a78-20.35",    0x68000, 0x8000, CRC(9ef863ad) SHA1(29f91b5a3765e4d6e6c3382db1d8d8297b6e56c8) )
	/* 0x70000-0x7ffff empty */

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "a71-25.41",    0x0000, 0x0100, CRC(2d0f8545) SHA1(089c31e2f614145ef2743164f7b52ae35bc06808) )	/* video timing */
ROM_END

/*
bubbobr1.zip - a78-21.52
1986 TAITO AMERICA CORP.
LICENSED TO ROMSTAR FOR U.S.A.
VER 1.0 26.AUG,1986 SUMMER
*/

ROM_START( bubbobr1 )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )
	ROM_LOAD( "a78-06.51",    0x00000, 0x08000, CRC(32c8305b) SHA1(6bf69b3edfbefd33cd670a762b4bf0b39629a220) )
    /* ROMs banked at 8000-bfff */
	ROM_LOAD( "a78-21.52",    0x10000, 0x10000, CRC(2844033d) SHA1(6ac0b09d0325990cf18935f35b0adbc033758947) )
	/* 20000-2ffff empty */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the second CPU */
	ROM_LOAD( "a78-08.37",    0x0000, 0x08000, CRC(ae11a07b) SHA1(af7a335c8da637103103cc274e077f123908ebb7) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* 64k for the third CPU */
	ROM_LOAD( "a78-07.46",    0x0000, 0x08000, CRC(4f9a26e8) SHA1(3105b34b88a7134493c2b3f584729f8b0407a011) )

	ROM_REGION( 0x10000, REGION_CPU4, 0 )	/* 64k for the MCU */
	ROM_LOAD( "a78-01.17",    0xf000, 0x1000, CRC(b1bfb53d) SHA1(31b8f31acd3aa394acd80db362774749842e1285) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "a78-09.12",    0x00000, 0x8000, CRC(20358c22) SHA1(2297af6c53d5807bf90a8e081075b8c72a994fc5) )    /* 1st plane */
	ROM_LOAD( "a78-10.13",    0x08000, 0x8000, CRC(930168a9) SHA1(fd358c3c3b424bca285f67a1589eb98a345ff670) )
	ROM_LOAD( "a78-11.14",    0x10000, 0x8000, CRC(9773e512) SHA1(33c1687ee575d66bf0e98add45d06da827813765) )
	ROM_LOAD( "a78-12.15",    0x18000, 0x8000, CRC(d045549b) SHA1(0c12077d3ddc2ce6aa45a0224ad5540f3f218446) )
	ROM_LOAD( "a78-13.16",    0x20000, 0x8000, CRC(d0af35c5) SHA1(c5a89f4d73acc0db86654540b3abfd77b3757db5) )
	ROM_LOAD( "a78-14.17",    0x28000, 0x8000, CRC(7b5369a8) SHA1(1307b26d80e6f36ebe6c442bebec41d20066eaf9) )
	/* 0x30000-0x3ffff empty */
	ROM_LOAD( "a78-15.30",    0x40000, 0x8000, CRC(6b61a413) SHA1(44eddf12fb46fceca2addbe6da929aaea7636b13) )    /* 2nd plane */
	ROM_LOAD( "a78-16.31",    0x48000, 0x8000, CRC(b5492d97) SHA1(d5b045e3ebaa44809757a4220cefb3c6815470da) )
	ROM_LOAD( "a78-17.32",    0x50000, 0x8000, CRC(d69762d5) SHA1(3326fef4e0bd86681a3047dc11886bb171ecb609) )
	ROM_LOAD( "a78-18.33",    0x58000, 0x8000, CRC(9f243b68) SHA1(32dce8d311a4be003693182a999e4053baa6bb0a) )
	ROM_LOAD( "a78-19.34",    0x60000, 0x8000, CRC(66e9438c) SHA1(b94e62b6fbe7f4e08086d0365afc5cff6e0ccafd) )
	ROM_LOAD( "a78-20.35",    0x68000, 0x8000, CRC(9ef863ad) SHA1(29f91b5a3765e4d6e6c3382db1d8d8297b6e56c8) )
	/* 0x70000-0x7ffff empty */

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "a71-25.41",    0x0000, 0x0100, CRC(2d0f8545) SHA1(089c31e2f614145ef2743164f7b52ae35bc06808) )	/* video timing */
ROM_END

/* 
   Bubble Bobble prototype on Tokio hardware
   14.MAY,1986VER 0.0
*/
 
ROM_START( bublboblp )
	ROM_REGION( 0x30000, REGION_CPU1, 0 ) /* main CPU */
	ROM_LOAD( "maincpu.ic4",   0x00000, 0x8000, CRC(874ddd6c) SHA1(30efef29558c7b2336ec8ab44686e32382d2045a) ) /* blank label, under epoxy */
	/* ROMs banked at 8000-bfff */
	ROM_LOAD( "maincpu.ic5",   0x10000, 0x8000, CRC(588cc602) SHA1(83c83ddace2fddbe16e4fbf8cbdbbb3140ac8192) ) /* blank label, under epoxy */
	/* ic6 socket is empty, under epoxy */
	/* ic7 socket is empty, under epoxy */
	/* ic8 socket is empty, under epoxy */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )   /* video CPU */
	ROM_LOAD( "slave.ic1",   0x00000, 0x8000, CRC(e8187e8f) SHA1(74b0442c61fe7f745ce0014bd5b7948783a323bd) ) /* blank label, under epoxy */

	ROM_REGION( 0x10000, REGION_CPU3, 0 )    /* audio CPU */
	ROM_LOAD( "audiocpu.ic10",  0x0000, 0x08000, CRC(c516c26e) SHA1(8cdeff2b8bb21d8c118f48e43b567a4e5b5e7184) ) /* blank label, under epoxy */

	/* mcu socket is empty */

	ROM_REGION( 0x80000,  REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT ) /* gfx roms, on gfx board */
	ROM_LOAD( "c1.ic12",  0x00000, 0x8000, CRC(183d378b) SHA1(e07599212af5d822ed1cb9eba8ca3fc01f13cbe0) )    /* 1st plane */
	ROM_LOAD( "c3.ic13",  0x08000, 0x8000, CRC(55408ff9) SHA1(1337eaa9f7189ac3192ef7c631a1460b5e02a820) )
	ROM_LOAD( "c5.ic14",  0x10000, 0x8000, CRC(12cc5949) SHA1(840042304e32125448507b0396ebd8734ea78016) )
	ROM_LOAD( "c7.ic15",  0x18000, 0x8000, CRC(10e24f35) SHA1(fce643c8aa1838309d82929717fea39d4d6fbd11) )
	ROM_LOAD( "c9.ic16",  0x20000, 0x8000, CRC(dec95961) SHA1(9f8b84035a85fc3325926c87d7908dedfbe4e80d) )
	ROM_LOAD( "c11.ic17", 0x28000, 0x8000, CRC(1c49d228) SHA1(3ea40bf82bc42d0ddb8b613e3012fa334a755272) )
	/* ic18 socket is empty */
	/* ic19 socket is empty */
	ROM_LOAD( "c0.ic30",  0x40000, 0x8000, CRC(39d0ce8f) SHA1(05d77d2c8ea083851fc5652fe6e4da9645c533e6) )    /* 2nd plane */
	ROM_LOAD( "c2.ic31",  0x48000, 0x8000, CRC(f705a512) SHA1(b598899b80ab28b1e487325f088fca0ba7994b19) )
	ROM_LOAD( "c4.ic32",  0x50000, 0x8000, CRC(151df0eb) SHA1(51071fbca7af66cfabd0ab963c385682fd402213) )
	ROM_LOAD( "c6.ic33",  0x58000, 0x8000, CRC(7b737c1e) SHA1(ae1bf563e1772d4ab50ee52aaacb1a7236f1e4e1) )
	ROM_LOAD( "c8.ic34",  0x60000, 0x8000, CRC(1320e15d) SHA1(b5da80bc27c053353c701f523f89f704c11c24e9) )
	ROM_LOAD( "c10.ic35", 0x68000, 0x8000, CRC(29c41387) SHA1(6f7b433a82e34b9daf5ce3f9a28061c64db061f6) )
	/* ic36 socket is empty */
	/* ic37 socket is empty */

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "a71-25.ic41", 0x0000, 0x0100, CRC(2d0f8545) SHA1(089c31e2f614145ef2743164f7b52ae35bc06808) )	/* video timing */
ROM_END

ROM_START( boblbobl )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )
	ROM_LOAD( "bb3",          0x00000, 0x08000, CRC(01f81936) SHA1(a48489a13bfd01949e7fd273029d9cb8bfd7be48) )
    /* ROMs banked at 8000-bfff */
	ROM_LOAD( "bb5",          0x10000, 0x08000, CRC(13118eb1) SHA1(5a5da40c2cc82420f70bc58ffa32de1088c6c82f) )
	ROM_LOAD( "bb4",          0x18000, 0x08000, CRC(afda99d8) SHA1(304324074ae726501bbb08e683850639d69939fb) )
	/* 20000-2ffff empty */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the second CPU */
	ROM_LOAD( "a78-08.37",    0x0000, 0x08000, CRC(ae11a07b) SHA1(af7a335c8da637103103cc274e077f123908ebb7) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* 64k for the third CPU */
	ROM_LOAD( "a78-07.46",    0x0000, 0x08000, CRC(4f9a26e8) SHA1(3105b34b88a7134493c2b3f584729f8b0407a011) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "a78-09.12",    0x00000, 0x8000, CRC(20358c22) SHA1(2297af6c53d5807bf90a8e081075b8c72a994fc5) )    /* 1st plane */
	ROM_LOAD( "a78-10.13",    0x08000, 0x8000, CRC(930168a9) SHA1(fd358c3c3b424bca285f67a1589eb98a345ff670) )
	ROM_LOAD( "a78-11.14",    0x10000, 0x8000, CRC(9773e512) SHA1(33c1687ee575d66bf0e98add45d06da827813765) )
	ROM_LOAD( "a78-12.15",    0x18000, 0x8000, CRC(d045549b) SHA1(0c12077d3ddc2ce6aa45a0224ad5540f3f218446) )
	ROM_LOAD( "a78-13.16",    0x20000, 0x8000, CRC(d0af35c5) SHA1(c5a89f4d73acc0db86654540b3abfd77b3757db5) )
	ROM_LOAD( "a78-14.17",    0x28000, 0x8000, CRC(7b5369a8) SHA1(1307b26d80e6f36ebe6c442bebec41d20066eaf9) )
	/* 0x30000-0x3ffff empty */
	ROM_LOAD( "a78-15.30",    0x40000, 0x8000, CRC(6b61a413) SHA1(44eddf12fb46fceca2addbe6da929aaea7636b13) )    /* 2nd plane */
	ROM_LOAD( "a78-16.31",    0x48000, 0x8000, CRC(b5492d97) SHA1(d5b045e3ebaa44809757a4220cefb3c6815470da) )
	ROM_LOAD( "a78-17.32",    0x50000, 0x8000, CRC(d69762d5) SHA1(3326fef4e0bd86681a3047dc11886bb171ecb609) )
	ROM_LOAD( "a78-18.33",    0x58000, 0x8000, CRC(9f243b68) SHA1(32dce8d311a4be003693182a999e4053baa6bb0a) )
	ROM_LOAD( "a78-19.34",    0x60000, 0x8000, CRC(66e9438c) SHA1(b94e62b6fbe7f4e08086d0365afc5cff6e0ccafd) )
	ROM_LOAD( "a78-20.35",    0x68000, 0x8000, CRC(9ef863ad) SHA1(29f91b5a3765e4d6e6c3382db1d8d8297b6e56c8) )
	/* 0x70000-0x7ffff empty */

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "a71-25.41",    0x0000, 0x0100, CRC(2d0f8545) SHA1(089c31e2f614145ef2743164f7b52ae35bc06808) )	/* video timing */
ROM_END

ROM_START( sboblbob )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )
	ROM_LOAD( "bbb-3.rom",    0x00000, 0x08000, CRC(f304152a) SHA1(103d9beddccef289ed739d28ebda69bbad3d42f9) )
    /* ROMs banked at 8000-bfff */
	ROM_LOAD( "bb5",          0x10000, 0x08000, CRC(13118eb1) SHA1(5a5da40c2cc82420f70bc58ffa32de1088c6c82f) )
	ROM_LOAD( "bbb-4.rom",    0x18000, 0x08000, CRC(94c75591) SHA1(7698bc4b7d20e554a73a489cd3a15ae61b350e37) )
	/* 20000-2ffff empty */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the second CPU */
	ROM_LOAD( "a78-08.37",    0x0000, 0x08000, CRC(ae11a07b) SHA1(af7a335c8da637103103cc274e077f123908ebb7) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* 64k for the third CPU */
	ROM_LOAD( "a78-07.46",    0x0000, 0x08000, CRC(4f9a26e8) SHA1(3105b34b88a7134493c2b3f584729f8b0407a011) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "a78-09.12",    0x00000, 0x8000, CRC(20358c22) SHA1(2297af6c53d5807bf90a8e081075b8c72a994fc5) )    /* 1st plane */
	ROM_LOAD( "a78-10.13",    0x08000, 0x8000, CRC(930168a9) SHA1(fd358c3c3b424bca285f67a1589eb98a345ff670) )
	ROM_LOAD( "a78-11.14",    0x10000, 0x8000, CRC(9773e512) SHA1(33c1687ee575d66bf0e98add45d06da827813765) )
	ROM_LOAD( "a78-12.15",    0x18000, 0x8000, CRC(d045549b) SHA1(0c12077d3ddc2ce6aa45a0224ad5540f3f218446) )
	ROM_LOAD( "a78-13.16",    0x20000, 0x8000, CRC(d0af35c5) SHA1(c5a89f4d73acc0db86654540b3abfd77b3757db5) )
	ROM_LOAD( "a78-14.17",    0x28000, 0x8000, CRC(7b5369a8) SHA1(1307b26d80e6f36ebe6c442bebec41d20066eaf9) )
	/* 0x30000-0x3ffff empty */
	ROM_LOAD( "a78-15.30",    0x40000, 0x8000, CRC(6b61a413) SHA1(44eddf12fb46fceca2addbe6da929aaea7636b13) )    /* 2nd plane */
	ROM_LOAD( "a78-16.31",    0x48000, 0x8000, CRC(b5492d97) SHA1(d5b045e3ebaa44809757a4220cefb3c6815470da) )
	ROM_LOAD( "a78-17.32",    0x50000, 0x8000, CRC(d69762d5) SHA1(3326fef4e0bd86681a3047dc11886bb171ecb609) )
	ROM_LOAD( "a78-18.33",    0x58000, 0x8000, CRC(9f243b68) SHA1(32dce8d311a4be003693182a999e4053baa6bb0a) )
	ROM_LOAD( "a78-19.34",    0x60000, 0x8000, CRC(66e9438c) SHA1(b94e62b6fbe7f4e08086d0365afc5cff6e0ccafd) )
	ROM_LOAD( "a78-20.35",    0x68000, 0x8000, CRC(9ef863ad) SHA1(29f91b5a3765e4d6e6c3382db1d8d8297b6e56c8) )
	/* 0x70000-0x7ffff empty */

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "a71-25.41",    0x0000, 0x0100, CRC(2d0f8545) SHA1(089c31e2f614145ef2743164f7b52ae35bc06808) )	/* video timing */
ROM_END

ROM_START( bublboblu )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )
	ROM_LOAD( "a78-06u.51",    0x00000, 0x08000, CRC(a6345edd) SHA1(144f33002ee40acdbfa6a49119092a319048bb00) )
	ROM_LOAD( "a78-05u.52",    0x10000, 0x10000, CRC(b31d2edc) SHA1(b7d317c0b5b86c0bf39b18cfe584bca9d22d4eba) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "a78-08u.37",    0x0000, 0x08000, CRC(d544be2e) SHA1(1472db52dcd9e17a866ea2766cfea500d8f712ab) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )
	ROM_LOAD( "a78-07.46",    0x0000, 0x08000, CRC(4f9a26e8) SHA1(3105b34b88a7134493c2b3f584729f8b0407a011) )

	ROM_REGION( 0x10000, REGION_CPU4, 0 )
	ROM_LOAD( "a78-01.17",    0xf000, 0x1000, CRC(b1bfb53d) SHA1(31b8f31acd3aa394acd80db362774749842e1285) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "a78-09.12",    0x00000, 0x8000, CRC(20358c22) SHA1(2297af6c53d5807bf90a8e081075b8c72a994fc5) )
	ROM_LOAD( "a78-10.13",    0x08000, 0x8000, CRC(930168a9) SHA1(fd358c3c3b424bca285f67a1589eb98a345ff670) )
	ROM_LOAD( "a78-11.14",    0x10000, 0x8000, CRC(9773e512) SHA1(33c1687ee575d66bf0e98add45d06da827813765) )
	ROM_LOAD( "a78-12.15",    0x18000, 0x8000, CRC(d045549b) SHA1(0c12077d3ddc2ce6aa45a0224ad5540f3f218446) )
	ROM_LOAD( "a78-13.16",    0x20000, 0x8000, CRC(d0af35c5) SHA1(c5a89f4d73acc0db86654540b3abfd77b3757db5) )
	ROM_LOAD( "a78-14.17",    0x28000, 0x8000, CRC(7b5369a8) SHA1(1307b26d80e6f36ebe6c442bebec41d20066eaf9) )
	ROM_LOAD( "a78-15.30",    0x40000, 0x8000, CRC(6b61a413) SHA1(44eddf12fb46fceca2addbe6da929aaea7636b13) )
	ROM_LOAD( "a78-16.31",    0x48000, 0x8000, CRC(b5492d97) SHA1(d5b045e3ebaa44809757a4220cefb3c6815470da) )
	ROM_LOAD( "a78-17.32",    0x50000, 0x8000, CRC(d69762d5) SHA1(3326fef4e0bd86681a3047dc11886bb171ecb609) )
	ROM_LOAD( "a78-18.33",    0x58000, 0x8000, CRC(9f243b68) SHA1(32dce8d311a4be003693182a999e4053baa6bb0a) )
	ROM_LOAD( "a78-19.34",    0x60000, 0x8000, CRC(66e9438c) SHA1(b94e62b6fbe7f4e08086d0365afc5cff6e0ccafd) )
	ROM_LOAD( "a78-20.35",    0x68000, 0x8000, CRC(9ef863ad) SHA1(29f91b5a3765e4d6e6c3382db1d8d8297b6e56c8) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "a71-25.41",    0x0000, 0x0100, CRC(2d0f8545) SHA1(089c31e2f614145ef2743164f7b52ae35bc06808) )
ROM_END

ROM_START( bublcave )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )
	ROM_LOAD( "bublcave-06.51",    0x00000, 0x08000, CRC(e8b9af5e) SHA1(dec44e47634a402df212806e84e3a810f8442776) )
	ROM_LOAD( "bublcave-05.52",    0x10000, 0x10000, CRC(cfe14cb8) SHA1(17d463c755f630ae9d05943515fa4828972bd7b0) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "bublcave-08.37",    0x0000, 0x08000, CRC(a9384086) SHA1(26e686671d6d3ba3759716bf46e7f951bbb8a291) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )
	ROM_LOAD( "a78-07.46",         0x0000, 0x08000, CRC(4f9a26e8) SHA1(3105b34b88a7134493c2b3f584729f8b0407a011) )

	ROM_REGION( 0x10000, REGION_CPU4, 0 )
	ROM_LOAD( "a78-01.17",         0xf000, 0x1000, CRC(b1bfb53d) SHA1(31b8f31acd3aa394acd80db362774749842e1285) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "bublcave-09.12",    0x00000, 0x8000, CRC(b90b7eef) SHA1(de72e4635843ad76248aa3b4aa8f8a0bfd53879e) )    /* 1st plane */
	ROM_LOAD( "bublcave-10.13",    0x08000, 0x8000, CRC(4fb22f05) SHA1(880104e86dbd00ae657cbc768722427503b6a59f) )
	ROM_LOAD( "bublcave-11.14",    0x10000, 0x8000, CRC(9773e512) SHA1(33c1687ee575d66bf0e98add45d06da827813765) )
	ROM_LOAD( "bublcave-12.15",    0x18000, 0x8000, CRC(e49eb49e) SHA1(2e05dc8833e10bef1a317d238c39fb9f362e9997) )
	ROM_LOAD( "bublcave-13.16",    0x20000, 0x8000, CRC(61919734) SHA1(2c07e29f3dcc972d5eb47679ad81a0d7656b0cb2) )
	ROM_LOAD( "bublcave-14.17",    0x28000, 0x8000, CRC(7e3a13bd) SHA1(bd4dba799340fa599f11cc68e03efe70ba6ba99b) )
	ROM_LOAD( "bublcave-15.30",    0x40000, 0x8000, CRC(c253c73a) SHA1(3e187f6b9ca769772990068abe7b309417147d39) )    /* 2nd plane */
	ROM_LOAD( "bublcave-16.31",    0x48000, 0x8000, CRC(e66c92ee) SHA1(12ea193c54121d08ad110c94cc075e29fef3ff85) )
	ROM_LOAD( "bublcave-17.32",    0x50000, 0x8000, CRC(d69762d5) SHA1(3326fef4e0bd86681a3047dc11886bb171ecb609) )
	ROM_LOAD( "bublcave-18.33",    0x58000, 0x8000, CRC(47ee2544) SHA1(c6946e824043a312ed437e548a64ef599effbd42) )
	ROM_LOAD( "bublcave-19.34",    0x60000, 0x8000, CRC(1ceeb1fa) SHA1(eb29ff896d149f7ab4cf38a338df39df14ccc20c) )
	ROM_LOAD( "bublcave-20.35",    0x68000, 0x8000, CRC(64322e24) SHA1(acff8a9fcaf74f198653080759898d15cccf04e8) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "a71-25.41",         0x0000, 0x0100, CRC(2d0f8545) SHA1(089c31e2f614145ef2743164f7b52ae35bc06808) )   /* video timing */
ROM_END

ROM_START( tokio )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )	/* main CPU */
	ROM_LOAD( "a71-27-1.256", 0x00000, 0x8000, CRC(8c180896) SHA1(bc8aeb42da4bae7db6f65b9874224f60a9bc4500) )
    /* ROMs banked at 8000-bfff */
	ROM_LOAD( "a71-28-1.256", 0x10000, 0x8000, CRC(1b447527) SHA1(6939e6c1b8492825d18f4e96f39ff45f4c96eea2) )
	ROM_LOAD( "a71-04.256",   0x18000, 0x8000, CRC(a0a4ce0e) SHA1(c49bdcd85c760a5e7327d1b424772e1560f1a318) )
	ROM_LOAD( "a71-05.256",   0x20000, 0x8000, CRC(6da0b945) SHA1(6c80b8333dd95657f99e6ba5b6e877733ac02a8c) )
	ROM_LOAD( "a71-06-1.256", 0x28000, 0x8000, CRC(56927b3f) SHA1(33fb4e71b95664ecff1f35f6782a14101982a56d) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* video CPU */
	ROM_LOAD( "a71-01.256",   0x00000, 0x8000, CRC(0867c707) SHA1(7129974f1252b28e9e338bd3c7fcb87210dcf412) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* audio CPU */
	ROM_LOAD( "a71-07.256",   0x0000, 0x08000, CRC(f298cc7b) SHA1(ebf5c804aa07b7f198ec3e1f8d1e111cd89ebdf3) )

	ROM_REGION( 0x0800,  REGION_CPU4, 0 )	/* 2k for the microcontroller */
	ROM_LOAD( "a71__24.ic57", 0x0000, 0x0800, CRC(0f4b25de) SHA1(e2d82aa8d8cc6a86aaf5715ef9cb62f526fd5b11) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "a71-08.256",   0x00000, 0x8000, CRC(0439ab13) SHA1(84142220a6a29f0e34f7c7c751b583bf394df8ce) )    /* 1st plane */
	ROM_LOAD( "a71-09.256",   0x08000, 0x8000, CRC(edb3d2ff) SHA1(0c6e4bbc786a097f9d99220e72f98c1c795a7292) )
	ROM_LOAD( "a71-10.256",   0x10000, 0x8000, CRC(69f0888c) SHA1(1704ab6339981195cd09d581e83094c75037d18e) )
	ROM_LOAD( "a71-11.256",   0x18000, 0x8000, CRC(4ae07c31) SHA1(452d1eb5a70e7853791cd05e4578c1454477bdec) )
	ROM_LOAD( "a71-12.256",   0x20000, 0x8000, CRC(3f6bd706) SHA1(b03c534a95b71941331d3ffd9aa7069b5f05687e) )
	ROM_LOAD( "a71-13.256",   0x28000, 0x8000, CRC(f2c92aaa) SHA1(7dfdc473794a298032405ba918df8085b0bbe174) )
	ROM_LOAD( "a71-14.256",   0x30000, 0x8000, CRC(c574b7b2) SHA1(9839adce60c0017ae3997603a2aece511af226d2) )
	ROM_LOAD( "a71-15.256",   0x38000, 0x8000, CRC(12d87e7f) SHA1(327a80f08207ee66721738f7e1c53f75b5659be0) )
	ROM_LOAD( "a71-16.256",   0x40000, 0x8000, CRC(0bce35b6) SHA1(3f0496db6681c7be1e36ba41296115d158d7457a) )    /* 2nd plane */
	ROM_LOAD( "a71-17.256",   0x48000, 0x8000, CRC(deda6387) SHA1(40f0be3a71b0a03f0275da72f4124424b162318a) )
	ROM_LOAD( "a71-18.256",   0x50000, 0x8000, CRC(330cd9d7) SHA1(919f78036b760938d6aa72754be1a615f568b470) )
	ROM_LOAD( "a71-19.256",   0x58000, 0x8000, CRC(fc4b29e0) SHA1(d11393a24b5c6c04f5058b299e4b0fc773a03e4b) )
	ROM_LOAD( "a71-20.256",   0x60000, 0x8000, CRC(65acb265) SHA1(2ef940f994e76d4387be6e0d53a565813cc59636) )
	ROM_LOAD( "a71-21.256",   0x68000, 0x8000, CRC(33cde9b2) SHA1(9b227ab609e3c7c6be90c29739a57ea4959cd68e) )
	ROM_LOAD( "a71-22.256",   0x70000, 0x8000, CRC(fb98eac0) SHA1(57615c3934de5510eeeb0ba16024abda8ee95303) )
	ROM_LOAD( "a71-23.256",   0x78000, 0x8000, CRC(30bd46ad) SHA1(6e1618ed237c769d1a8d329fbd7a9f7216993215) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "a71-25.bin",   0x0000, 0x0100, CRC(2d0f8545) SHA1(089c31e2f614145ef2743164f7b52ae35bc06808) )	/* video timing */
ROM_END

ROM_START( tokiob )
	ROM_REGION( 0x30000, REGION_CPU1, 0 ) /* main CPU */
	ROM_LOAD( "2",            0x00000, 0x8000, CRC(f583b1ef) SHA1(a97b36299b51792953516224191f11decc579a38) )
    /* ROMs banked at 8000-bfff */
	ROM_LOAD( "3",            0x10000, 0x8000, CRC(69dacf44) SHA1(ee8c33702749c0e2562951f9f80c897d3fbd7dd7) )
	ROM_LOAD( "a71-04.256",   0x18000, 0x8000, CRC(a0a4ce0e) SHA1(c49bdcd85c760a5e7327d1b424772e1560f1a318) )
	ROM_LOAD( "a71-05.256",   0x20000, 0x8000, CRC(6da0b945) SHA1(6c80b8333dd95657f99e6ba5b6e877733ac02a8c) )
	ROM_LOAD( "6",            0x28000, 0x8000, CRC(1490e95b) SHA1(a73e1857a1029156f0b5f7f7fe34a37870e72209) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* video CPU */
	ROM_LOAD( "a71-01.256",   0x00000, 0x8000, CRC(0867c707) SHA1(7129974f1252b28e9e338bd3c7fcb87210dcf412) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* audio CPU */
	ROM_LOAD( "a71-07.256",   0x0000, 0x08000, CRC(f298cc7b) SHA1(ebf5c804aa07b7f198ec3e1f8d1e111cd89ebdf3) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "a71-08.256",   0x00000, 0x8000, CRC(0439ab13) SHA1(84142220a6a29f0e34f7c7c751b583bf394df8ce) )    /* 1st plane */
	ROM_LOAD( "a71-09.256",   0x08000, 0x8000, CRC(edb3d2ff) SHA1(0c6e4bbc786a097f9d99220e72f98c1c795a7292) )
	ROM_LOAD( "a71-10.256",   0x10000, 0x8000, CRC(69f0888c) SHA1(1704ab6339981195cd09d581e83094c75037d18e) )
	ROM_LOAD( "a71-11.256",   0x18000, 0x8000, CRC(4ae07c31) SHA1(452d1eb5a70e7853791cd05e4578c1454477bdec) )
	ROM_LOAD( "a71-12.256",   0x20000, 0x8000, CRC(3f6bd706) SHA1(b03c534a95b71941331d3ffd9aa7069b5f05687e) )
	ROM_LOAD( "a71-13.256",   0x28000, 0x8000, CRC(f2c92aaa) SHA1(7dfdc473794a298032405ba918df8085b0bbe174) )
	ROM_LOAD( "a71-14.256",   0x30000, 0x8000, CRC(c574b7b2) SHA1(9839adce60c0017ae3997603a2aece511af226d2) )
	ROM_LOAD( "a71-15.256",   0x38000, 0x8000, CRC(12d87e7f) SHA1(327a80f08207ee66721738f7e1c53f75b5659be0) )
	ROM_LOAD( "a71-16.256",   0x40000, 0x8000, CRC(0bce35b6) SHA1(3f0496db6681c7be1e36ba41296115d158d7457a) )    /* 2nd plane */
	ROM_LOAD( "a71-17.256",   0x48000, 0x8000, CRC(deda6387) SHA1(40f0be3a71b0a03f0275da72f4124424b162318a) )
	ROM_LOAD( "a71-18.256",   0x50000, 0x8000, CRC(330cd9d7) SHA1(919f78036b760938d6aa72754be1a615f568b470) )
	ROM_LOAD( "a71-19.256",   0x58000, 0x8000, CRC(fc4b29e0) SHA1(d11393a24b5c6c04f5058b299e4b0fc773a03e4b) )
	ROM_LOAD( "a71-20.256",   0x60000, 0x8000, CRC(65acb265) SHA1(2ef940f994e76d4387be6e0d53a565813cc59636) )
	ROM_LOAD( "a71-21.256",   0x68000, 0x8000, CRC(33cde9b2) SHA1(9b227ab609e3c7c6be90c29739a57ea4959cd68e) )
	ROM_LOAD( "a71-22.256",   0x70000, 0x8000, CRC(fb98eac0) SHA1(57615c3934de5510eeeb0ba16024abda8ee95303) )
	ROM_LOAD( "a71-23.256",   0x78000, 0x8000, CRC(30bd46ad) SHA1(6e1618ed237c769d1a8d329fbd7a9f7216993215) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "a71-25.bin",   0x0000, 0x0100, CRC(2d0f8545) SHA1(089c31e2f614145ef2743164f7b52ae35bc06808) )	/* video timing */
ROM_END


static DRIVER_INIT( bublbobl )
{
	extern int bublbobl_video_enable;
	unsigned char *ROM = memory_region(REGION_CPU1);

	/* in Bubble Bobble, bank 0 has code falling from 7fff to 8000, */
	/* so I have to copy it there because bank switching wouldn't catch it */
	memcpy(ROM+0x08000,ROM+0x10000,0x4000);

	bublbobl_video_enable = 1;
}

static DRIVER_INIT( boblbobl )
{
#define MOD_PAGE(page,addr,data) memory_region(REGION_CPU1)[addr-0x8000+0x10000+0x4000*page] = data;
    /* these shouldn't be necessary, surely - this is a bootleg ROM
     * with the protection removed - so what are all these JP's to
     * 0xa288 doing?  and why does the emulator fail the ROM checks?
     */

	MOD_PAGE(3,0x9a71,0x00); MOD_PAGE(3,0x9a72,0x00); MOD_PAGE(3,0x9a73,0x00);
	MOD_PAGE(3,0xa4af,0x00); MOD_PAGE(3,0xa4b0,0x00); MOD_PAGE(3,0xa4b1,0x00);
	MOD_PAGE(3,0xa55d,0x00); MOD_PAGE(3,0xa55e,0x00); MOD_PAGE(3,0xa55f,0x00);
	MOD_PAGE(3,0xb561,0x00); MOD_PAGE(3,0xb562,0x00); MOD_PAGE(3,0xb563,0x00);

	init_bublbobl();
}


static DRIVER_INIT( tokio )
{
	extern int bublbobl_video_enable;

	/* preemptively enable video, the bit is not mapped for this game and */
	/* I don't know if it even has it. */
	bublbobl_video_enable = 1;
}


GAME( 1986, bublbobl, 0,        bublbobl, bublbobl, bublbobl, ROT0,  "Taito Corporation", "Bubble Bobble" )
GAME( 1986, bublbob1, bublbobl, bublbobl, bublbobl, bublbobl, ROT0,  "Taito Corporation", "Bubble Bobble (older)" )
GAME( 1986, bublbobr, bublbobl, bublbobl, bublbobl, bublbobl, ROT0,  "Taito America Corporation (Romstar license)", "Bubble Bobble (US with mode select)" )
GAME( 1986, bubbobr1, bublbobl, bublbobl, bublbobl, bublbobl, ROT0,  "Taito America Corporation (Romstar license)", "Bubble Bobble (US)" )
GAME( 1986, bublboblp,bublbobl, tokiob,   bublboblp,bublbobl, ROT0,  "Taito Corporation", "Bubble Bobble (prototype on Tokio hardware)" )
GAME( 1986, boblbobl, bublbobl, boblbobl, boblbobl, boblbobl, ROT0,  "bootleg", "Bobble Bobble" )
GAME( 1986, sboblbob, bublbobl, boblbobl, sboblbob, bublbobl, ROT0,  "bootleg", "Super Bobble Bobble" )
GAME( 1986, tokio,    0,        tokio,    tokio,    tokio,    ROT90, "Taito America Corporation (Romstar license)", "Tokio / Scramble Formation (US)" )
GAME( 1986, tokiob,   tokio,    tokiob,   tokiob,   tokio,    ROT90, "bootleg", "Tokio / Scramble Formation (bootleg)" )

/* hacks */
GAME( 2012, bublboblu,bublbobl, bublbobl, bublbobl, bublbobl, ROT0,  "hack", "Bubble Bobble (Ultra Version)(USA)" )
GAME( 2013, bublcave, bublbobl, bublbobl, bublbobl, bublbobl, ROT0,  "hack (Bisboch and Aladar)", "Bubble Bobble: Lost Cave V1.2" )
