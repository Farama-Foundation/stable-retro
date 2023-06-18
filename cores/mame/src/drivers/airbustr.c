/***************************************************************************

								Air Buster
						    (C) 1990  Kaneko

				    driver by Luca Elia (l.elia@tin.it)

CPU   : Z-80 x 3
SOUND : YM2203C		M6295
OSC.  : 12.000MHz	16.000MHz

					Interesting routines (main cpu)
					-------------------------------

fd-fe	address of int: 0x38	(must alternate? see e600/1)
ff-100	address of int: 0x16
66		print "sub cpu caused nmi" and die!

7		after tests

1497	print string following call: (char)*,0. program continues after 0.
		base addr = c300 + HL & 08ff, DE=xy , BC=dxdy
		+0<-(e61b)	+100<-D	+200<-E	+300<-char	+400<-(e61c)

1642	A<- buttons status (bits 0&1)

					Interesting locations (main cpu)
					--------------------------------

2907	table of routines (f150 index = 1-3f, copied to e612)
		e60e<-address of routine, f150<-0. e60e is used until f150!=0

	1:	2bf4	service mode								next

	2:	2d33	3:	16bd		4:	2dcb		5:	2fcf
	6:	3262	7:	32b8

	8:	335d>	print gfx/color test page					next
	9:	33c0>	handle the above page

	a:	29c6		b:	2a24		c:	16ce

	d:	3e7e>	*
	e:	3ec5>	print "Sub Cpu / Ram Error"; **
	f:	3e17>	print "Coin error"; **
	10:	3528>	print (c) notice, not shown					next
	11:	3730>	show (c) notice, wait 0x100 calls			next

	12:		9658	13:	97c3		14:	a9fa		15:	aa6e
	16-19:	2985	1a:	9c2e		1b:	9ffa		1c:	29c6

	1d:	2988>	*

	1e:	a2c4		1f:	a31a		20:	a32f		21:	a344
	22:	a359		23:	a36e		24:	a383		25:	a398
	26:	a3ad		27:	a3c2		28:	a3d7		29:	a3f1
	2a:	a40e		2b:	a4e5		2c:	a69d		2d:	adb8
	2e:	ade9		2f:	2b8b		30:	a823

	31:	3d17>	print "warm up, wait few mins. secs left: 00"	next
	32:	3dc0>	pause (e624 counter).e626						next

	33:	96b4		34:	97ad

	35-3f:	3e7e>	*

*	Print "Command Error [$xx]" where xx is last routine index (e612)
**	ld (0000h),A (??) ; loop

3cd7	hiscores table (0x40 bytes, copied to e160)
		Try entering TERU as your name :)

7fff	country code: 0 <-> Japan; else World

e615	rank:	0-easy	1-normal	2-hard	3-hardest
e624	sound code during sound test

-- Shared RAM --

f148<-	sound code (copied from e624)
f14a->	read on nmi routine. main cpu writes the value and writes to port 02
f150<-	index of table of routines at 2907

----------------





					Interesting routines (sub cpu)
					-------------------------------

491		copy palette data	d000<-f200(a0)	d200<-f300(a0)	d400<-f400(200)

61c		f150<-A		f151<-A	(routine index of main cpu!)
		if dsw1-2 active, it does nothing (?!)

c8c		c000-c7ff<-0	c800-cfff<-0	f600<-f200(400)
1750	copies 10 lines of 20 bytes from 289e to f800

22cd	copy 0x100 bytes
22cf	copy 0x0FF bytes
238d	copy 0x0A0 bytes

					Interesting locations (sub cpu)
					--------------------------------

fd-fe	address of int: 0x36e	(same as 38)
ff-100	address of int: 0x4b0

-- Shared RAM --

f000	credits

f001/d<-IN 24 (Service)
f00e<-	bank
f002<-	dsw1 (cpl'd)
f003<-	dsw2 (cpl'd)
f004<-	IN 20 (cpl'd) (player 1)
f005<-	IN 22 (cpl'd) (player 2)
f006<-	start lives: dsw-2 & 0x30 index; values: 3,4,5,7		(5da table)
f007	current lives p1
f008	current lives p2

f009<-	coin/credit 1: dsw-1 & 0x30 index; values: 11,12,21,23	(5de table)
f00a<-	coin 1
f00b<-	coin/credit 2: dsw-1 & 0xc0 index; values: 11,12,21,23	(5e2 table)
f00c<-	coin 2

f00f	?? outa (28h)
f010	written by sub cpu, bit 4 read by main cpu.
		bit 0	p1 playing
		bit 1	p2 playing

f014	index (1-f) of routine called during int 36e (table at c3f)
	1:	62b			2:	66a			3:	6ad			4:	79f
	5:	7e0			6:	81b			7:	8a7			8:	8e9
	9:	b02			a:	0			b:	0			c:	bfc
	d:	c0d			e:	a6f			f:	ac3

f015	index of the routine to call, usually the one selected by f014
		but sometimes written directly.

Scroll registers: ports 04/06/08/0a/0c, written in sequence (101f)
port 06 <- f100 + f140	x		port 04 <- f104 + f142	y
port 0a <- f120 + f140	x		port 08 <- f124 + f142	y
port 0c <- f14c = bit 0/1/2/3 = port 6/4/a/8 val < FF

f148->	sound code (from main cpu)
f14c	scroll regs high bits

----------------








					Interesting routines (sound cpu)
					-------------------------------

50a		6295
521		6295
a96		2203 reg<-B		val<-A

					Interesting locations (sound cpu)
					---------------------------------

c715
c716	pending sound command
c760	rom bank


								To Do
								-----

- The bankswitching registers bits 4&5 are used (any Z80). What for ?
- Is the sub cpu / sound cpu communication status port (0e) correct ?
- Main cpu: port  01 ?
- Sub  cpu: ports 0x28, 0x38 ? Plus it can probably cause a nmi to main cpu
- incomplete DSW's
- Spriteram low 0x300 bytes (priority?)

***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"

unsigned char *devram, *sharedram;
int soundlatch_status, soundlatch2_status;

/* Variables that vidhrdw has access to */
extern unsigned char *spriteram;
int flipscreen;

/* Variables defined in vidhrdw */
extern unsigned char *airbustr_bgram, *airbustr_fgram;

/* Functions defined in vidhrdw */
WRITE_HANDLER( airbustr_bgram_w );
WRITE_HANDLER( airbustr_fgram_w );
WRITE_HANDLER( airbustr_scrollregs_w );
extern VIDEO_START( airbustr );
extern VIDEO_UPDATE( airbustr );

/* Debug stuff (bound to go away sometime) */
int u1, u2, u3, u4;


static WRITE_HANDLER( bankswitch_w );
static WRITE_HANDLER( bankswitch2_w );
static WRITE_HANDLER( sound_bankswitch_w );

static MACHINE_INIT( airbustr )
{
	soundlatch_status = soundlatch2_status = 0;
	bankswitch_w(0,2);
	bankswitch2_w(0,2);
	sound_bankswitch_w(0,2);
}


/*
**
**				Main cpu data
**
*/

/*	Runs in IM 2	fd-fe	address of int: 0x38
					ff-100	address of int: 0x16	*/

INTERRUPT_GEN( airbustr_interrupt )
{
static int addr = 0xff;

	addr ^= 0x02;
	cpu_set_irq_line_and_vector(0, 0, HOLD_LINE, addr);
}


static READ_HANDLER( sharedram_r )	{ return sharedram[offset]; }
static WRITE_HANDLER( sharedram_w )	{ sharedram[offset] = data; }


/* There's an MCU here, possibly */
READ_HANDLER( devram_r )
{
	switch (offset)
	{
		/* Reading efe0 probably resets a watchdog mechanism
		   that would reset the main cpu. We avoid this and patch
		   the rom instead (main cpu has to be reset once at startup) */
		case 0xfe0:
			return 0/*watchdog_reset_r(0)*/;
			break;

		/* Reading a word at eff2 probably yelds the product
   		   of the words written to eff0 and eff2 */
		case 0xff2:
		case 0xff3:
		{
			int	x = (devram[0xff0] + devram[0xff1] * 256) *
					(devram[0xff2] + devram[0xff3] * 256);
			if (offset == 0xff2)	return (x & 0x00FF) >> 0;
			else				return (x & 0xFF00) >> 8;
		}	break;

		/* Reading eff4, F0 times must yield at most 80-1 consecutive
		   equal values */
		case 0xff4:
		{
			return rand();
		}	break;

		default:	{ return devram[offset]; break;}
	}

}
WRITE_HANDLER( devram_w )	{	devram[offset] = data; }


static WRITE_HANDLER( bankswitch_w )
{
unsigned char *RAM = memory_region(REGION_CPU1);

	if ((data & 7) <  3)	RAM = &RAM[0x4000 * (data & 7)];
	else					RAM = &RAM[0x10000 + 0x4000 * ((data & 7)-3)];

	cpu_setbank(1,RAM);
/*	if (data > 7)	log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #0 - suspicious bank: %d ! - PC = %04X\n", data, activecpu_get_pc());*/

	u1 = data & 0xf8;
}

/* Memory */

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xcfff, MRA_RAM },
	{ 0xd000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xefff, devram_r },
	{ 0xf000, 0xffff, sharedram_r },
MEMORY_END
static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0xbfff, MWA_ROM },	/* writing at 0 should cause a reset*/
	{ 0xc000, 0xcfff, MWA_RAM, &spriteram },			/* RAM 0/1*/
	{ 0xd000, 0xdfff, MWA_RAM },						/* RAM 2*/
	{ 0xe000, 0xefff, devram_w, &devram },				/* RAM 3*/
	{ 0xf000, 0xffff, sharedram_w, &sharedram },
MEMORY_END

/* Ports */

static WRITE_HANDLER( cause_nmi_w )
{
	cpu_set_irq_line(1, IRQ_LINE_NMI, PULSE_LINE);
}

static PORT_WRITE_START( writeport )
	{ 0x00, 0x00, bankswitch_w },
/*	{ 0x01, 0x01, IOWP_NOP },	*/ /* ?? only 2 (see 378b)*/
	{ 0x02, 0x02, cause_nmi_w },	/* always 0. Cause a nmi to sub cpu*/
PORT_END










/*
**
**				Sub cpu data
**
**
*/

/*	Runs in IM 2	fd-fe	address of int: 0x36e	(same as 0x38)
					ff-100	address of int: 0x4b0	(only writes to port 38h)	*/

INTERRUPT_GEN( airbustr_interrupt2 )
{
static int addr = 0xfd;

	addr ^= 0x02;
	cpu_set_irq_line_and_vector(1, 0, HOLD_LINE, addr);
}


static WRITE_HANDLER( bankswitch2_w )
{
unsigned char *RAM = memory_region(REGION_CPU2);

	if ((data & 7) <  3)	RAM = &RAM[0x4000 * (data & 7)];
	else					RAM = &RAM[0x10000 + 0x4000 * ((data & 7)-3)];

	cpu_setbank(2,RAM);
/*	if (data > 7)	log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #1 - suspicious bank: %d ! - PC = %04X\n", data, activecpu_get_pc());*/

	flipscreen = data & 0x10;	/* probably..*/
	tilemap_set_flip(ALL_TILEMAPS,flipscreen ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);

	u2 = data & 0xf8;
}


WRITE_HANDLER( airbustr_paletteram_w )
{
	int r,g,b;
	int val;

	/*	! byte 1 ! ! byte 0 !	*/
	/*	xGGG GGRR 	RRRB BBBB	*/
	/*	x432 1043 	2104 3210	*/

	paletteram[offset] = data;
	val = (paletteram[offset | 1] << 8) | paletteram[offset & ~1];

	g = (val >> 10) & 0x1f;
	r = (val >>  5) & 0x1f;
	b = (val >>  0) & 0x1f;

	palette_set_color(offset/2,	(r * 0xff) / 0x1f,
									(g * 0xff) / 0x1f,
									(b * 0xff) / 0x1f );
}


/* Memory */

static MEMORY_READ_START( readmem2 )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK2 },
	{ 0xc000, 0xcfff, MRA_RAM },
	{ 0xd000, 0xd5ff, paletteram_r },
	{ 0xd600, 0xdfff, MRA_RAM },
	{ 0xe000, 0xefff, MRA_RAM },
	{ 0xf000, 0xffff, sharedram_r },
MEMORY_END

static MEMORY_WRITE_START( writemem2 )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc7ff, airbustr_fgram_w, &airbustr_fgram },
	{ 0xc800, 0xcfff, airbustr_bgram_w, &airbustr_bgram },
	{ 0xd000, 0xd5ff, airbustr_paletteram_w, &paletteram },
	{ 0xd600, 0xdfff, MWA_RAM },
	{ 0xe000, 0xefff, MWA_RAM },
	{ 0xf000, 0xffff, sharedram_w },
MEMORY_END


/* Ports */

/*
   Sub cpu and Sound cpu communicate bidirectionally:

	   Sub   cpu writes to soundlatch,  reads from soundlatch2
	   Sound cpu writes to soundlatch2, reads from soundlatch

   Each latch raises a flag when it's been written.
   The flag is cleared when the latch is read.

Code at 505: waits for bit 1 to go low, writes command, waits for bit
0 to go low, reads back value. Code at 3b2 waits bit 2 to go high
(called during int fd)

*/


static READ_HANDLER( soundcommand_status_r )
{
/* bits: 2 <-> ?	1 <-> soundlatch full	0 <-> soundlatch2 empty */
	return 4 + soundlatch_status * 2 + (1-soundlatch2_status);
}


static READ_HANDLER( soundcommand2_r )
{
	soundlatch2_status = 0;				/* soundlatch2 has been read*/
	return soundlatch2_r(0);
}


static WRITE_HANDLER( soundcommand_w )
{
	soundlatch_w(0,data);
	soundlatch_status = 1;				/* soundlatch has been written*/
	cpu_set_irq_line(2, IRQ_LINE_NMI, PULSE_LINE);	/* cause a nmi to sub cpu*/
}


WRITE_HANDLER( port_38_w )	{	u4 = data; } /* for debug*/


static PORT_READ_START( readport2 )
	{ 0x02, 0x02, soundcommand2_r },		/* from sound cpu*/
	{ 0x0e, 0x0e, soundcommand_status_r },	/* status of the latches ?*/
	{ 0x20, 0x20, input_port_0_r },			/* player 1*/
	{ 0x22, 0x22, input_port_1_r },			/* player 2*/
	{ 0x24, 0x24, input_port_2_r },			/* service*/
PORT_END

static PORT_WRITE_START( writeport2 )
	{ 0x00, 0x00, bankswitch2_w },			/* bits 2-0 bank, bit 4 (on if dsw1-1 active)?,  bit 5?*/
	{ 0x02, 0x02, soundcommand_w },			/* to sound cpu*/
	{ 0x04, 0x0c, airbustr_scrollregs_w },	/* Scroll values*/
/*	{ 0x28, 0x28, port_38_w },				*/ /* ??*/
/*	{ 0x38, 0x38, IOWP_NOP },				*/ /* ?? Followed by EI. Value isn't important*/
PORT_END










/*
**
** 				Sound cpu data
**
*/

static WRITE_HANDLER( sound_bankswitch_w )
{
unsigned char *RAM = memory_region(REGION_CPU3);

	if ((data & 7) <  3)	RAM = &RAM[0x4000 * (data & 7)];
	else					RAM = &RAM[0x10000 + 0x4000 * ((data & 7)-3)];

	cpu_setbank(3,RAM);
/*	if (data > 7)	log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #2 - suspicious bank: %d ! - PC = %04X\n", data, activecpu_get_pc());*/

	u3 = data & 0xf8;
}


/* Memory */

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK3 },
	{ 0xc000, 0xdfff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM },
MEMORY_END


/* Ports */

READ_HANDLER( soundcommand_r )
{
	soundlatch_status = 0;		/* soundlatch has been read*/
	return soundlatch_r(0);
}


WRITE_HANDLER( soundcommand2_w )
{
	soundlatch2_status = 1;		/* soundlatch2 has been written*/
	soundlatch2_w(0,data);
}


static PORT_READ_START( sound_readport )
	{ 0x02, 0x02, YM2203_status_port_0_r },
	{ 0x03, 0x03, YM2203_read_port_0_r },
	{ 0x04, 0x04, OKIM6295_status_0_r },
	{ 0x06, 0x06, soundcommand_r },			/* read command from sub cpu*/
PORT_END

static PORT_WRITE_START( sound_writeport )
	{ 0x00, 0x00, sound_bankswitch_w },
	{ 0x02, 0x02, YM2203_control_port_0_w },
	{ 0x03, 0x03, YM2203_write_port_0_w },
	{ 0x04, 0x04, OKIM6295_data_0_w },
	{ 0x06, 0x06, soundcommand2_w },		/* write command result to sub cpu*/
PORT_END




/*	Input Ports:
	[0] Player 1		[1] Player 2
	[2] Service
	[3] Dsw 1			[4] Dsw 2	*/

INPUT_PORTS_START( airbustr )

	PORT_START	/* IN0 - Player 1*/
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN1 - Player 2*/
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 - Service*/
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )		/* used*/

	PORT_START	/* IN3 - DSW-1*/
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, "Coin Mode" )			/*   routine at 0x056d :*/
	PORT_DIPSETTING(    0x08, "Mode 1" )			/*     11 21 12 16 (bit 3 active)*/
	PORT_DIPSETTING(    0x00, "Mode 2" )			/*     11 21 13 14 (bit 3 not active)*/
	/* 	Coinage Mode 1 */
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )
	/*	Coinage Mode 2 - for non-Japan version
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_4C ) )
	*/

	PORT_START	/* IN4 - DSW-2*/
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Freeze" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

INPUT_PORTS_END

INPUT_PORTS_START( airbustj )

	PORT_START	/* IN0 - Player 1*/
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN1 - Player 2*/
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 - Service*/
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )		/* used*/

	PORT_START	/* IN3 - DSW-1*/
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )		/* routine at 0x0546 : 11 12 21 23*/
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

	PORT_START	/* IN4 - DSW-2*/
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Freeze" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

INPUT_PORTS_END



/*
**
** 				Gfx data
**
*/


/* displacement in bits to lower part of gfx */
#define lo (8*8*4*2)
#define layout16x16(_name_,_romsize_) \
static struct GfxLayout _name_ =\
{\
	16,16,\
	(_romsize_)*8/(16*16*4),\
	4,\
	{0, 1, 2, 3},\
	{0*4,1*4,2*4,3*4,4*4,5*4,6*4,7*4, \
	 0*4+32*8,1*4+32*8,2*4+32*8,3*4+32*8,4*4+32*8,5*4+32*8,6*4+32*8,7*4+32*8},\
	{0*32,1*32,2*32,3*32,4*32,5*32,6*32,7*32,\
	 0*32+lo,1*32+lo,2*32+lo,3*32+lo,4*32+lo,5*32+lo,6*32+lo,7*32+lo}, \
	16*16*4\
};

layout16x16(tilelayout,  0x080000)
layout16x16(spritelayout,0x100000)

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tilelayout,   256*0, (256*2) / 16 }, /* [0] Layers*/
	{ REGION_GFX2, 0, &spritelayout, 256*2, (256*1) / 16 }, /* [1] Sprites*/
	{ -1 }
};


static struct YM2203interface ym2203_interface =
{
	1,
	3000000,					/* ?? */
	{ YM2203_VOL(50,25) },	/* gain,volume - adjusted */
	{ input_port_3_r },			/* DSW-1 connected to port A */
	{ input_port_4_r },			/* DSW-2 connected to port B */
	{ 0 },
	{ 0 },
	{ 0 }
};

static struct OKIM6295interface okim6295_interface =
{
	1,
	{ 12000000/4/165 }, /* 3MHz -> 6295 (mode A) */
	{ REGION_SOUND1 },
	{ 80 }  /* adjusted */
};



static MACHINE_DRIVER_START( airbustr )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 6000000)	/* ?? */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_PORTS(0,writeport)
	MDRV_CPU_VBLANK_INT(airbustr_interrupt,2)	/* nmi caused by sub cpu?, ? */

	MDRV_CPU_ADD(Z80, 6000000)	/* ?? */
	MDRV_CPU_MEMORY(readmem2,writemem2)
	MDRV_CPU_PORTS(readport2,writeport2)
	MDRV_CPU_VBLANK_INT(airbustr_interrupt2,2)	/* nmi caused by main cpu, ? */

	MDRV_CPU_ADD(Z80, 6000000)	/* ?? */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PORTS(sound_readport,sound_writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)	/* nmi are caused by sub cpu writing a sound command */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)	/* Palette RAM is filled by sub cpu with data supplied by main cpu */
							/* Maybe an high value is safer in order to avoid glitches */
	MDRV_MACHINE_INIT(airbustr)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 256-1, 0+16, 256-16-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(768)

	MDRV_VIDEO_START(airbustr)
	MDRV_VIDEO_UPDATE(airbustr)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, ym2203_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( airbustr )
	ROM_REGION( 0x24000, REGION_CPU1, 0 )
	ROM_LOAD( "pr12.h19", 0x00000, 0x0c000, CRC(91362eb2) SHA1(cd85acfa6542af68dd1cad46f9426a95cfc9432e) )
	ROM_CONTINUE(           0x10000, 0x14000 )

	ROM_REGION( 0x24000, REGION_CPU2, 0 )
	ROM_LOAD( "pr13.l15", 0x00000, 0x0c000, CRC(13b2257b) SHA1(325efa54e757a1f08caf81801930d61ea4e7b6d4) )
	ROM_CONTINUE(           0x10000, 0x14000 )

	ROM_REGION( 0x24000, REGION_CPU3, 0 )
	ROM_LOAD( "pr-21.bin",  0x00000, 0x0c000, CRC(6e0a5df0) SHA1(616b7c7aaf52a9a55b63c60717c1866940635cd4) )
	ROM_CONTINUE(           0x10000, 0x14000 )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "pr-000.bin", 0x000000, 0x80000, CRC(8ca68f0d) SHA1(d60389e7e63e9850bcddecb486558de1414f1276) ) /* scrolling layers*/

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "pr-001.bin", 0x000000, 0x80000, CRC(7e6cb377) SHA1(005290f9f53a0c3a6a9d04486b16b7fd52cc94b6) ) /* sprites*/
	ROM_LOAD( "pr-02.bin",  0x080000, 0x10000, CRC(6bbd5e46) SHA1(26563737f3f91ee0a056d35ce42217bb57d8a081) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* OKI-M6295 samples */
	ROM_LOAD( "pr-200.bin", 0x00000, 0x40000, CRC(a4dd3390) SHA1(2d72b46b4979857f6b66489bebda9f48799f59cf) )
ROM_END

ROM_START( airbustj )
	ROM_REGION( 0x24000, REGION_CPU1, 0 )
	ROM_LOAD( "pr-14j.bin", 0x00000, 0x0c000, CRC(6b9805bd) SHA1(db6df33cf17316a4b81d7731dca9fe8bbf81f014) )
	ROM_CONTINUE(           0x10000, 0x14000 )

	ROM_REGION( 0x24000, REGION_CPU2, 0 )
	ROM_LOAD( "pr-11j.bin", 0x00000, 0x0c000, CRC(85464124) SHA1(8cce8dfdede48032c40d5f155fd58061866668de) )
	ROM_CONTINUE(           0x10000, 0x14000 )

	ROM_REGION( 0x24000, REGION_CPU3, 0 )
	ROM_LOAD( "pr-21.bin",  0x00000, 0x0c000, CRC(6e0a5df0) SHA1(616b7c7aaf52a9a55b63c60717c1866940635cd4) )
	ROM_CONTINUE(           0x10000, 0x14000 )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "pr-000.bin", 0x000000, 0x80000, CRC(8ca68f0d) SHA1(d60389e7e63e9850bcddecb486558de1414f1276) ) /* scrolling layers*/

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "pr-001.bin", 0x000000, 0x80000, CRC(7e6cb377) SHA1(005290f9f53a0c3a6a9d04486b16b7fd52cc94b6) ) /* sprites*/
	ROM_LOAD( "pr-02.bin",  0x080000, 0x10000, CRC(6bbd5e46) SHA1(26563737f3f91ee0a056d35ce42217bb57d8a081) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* OKI-M6295 samples */
	ROM_LOAD( "pr-200.bin", 0x00000, 0x40000, CRC(a4dd3390) SHA1(2d72b46b4979857f6b66489bebda9f48799f59cf) )
ROM_END



DRIVER_INIT( airbustr )
{
	int i;
	unsigned char *RAM;

	/* One gfx rom seems to have scrambled data (bad read?): */
	/* let's swap even and odd nibbles */
	RAM = memory_region(REGION_GFX1) + 0x000000;
	for (i = 0; i < 0x80000; i ++)
	{
		RAM[i] = ((RAM[i] & 0xF0)>>4) + ((RAM[i] & 0x0F)<<4);
	}

	RAM = memory_region(REGION_CPU1);
	RAM[0x37e4] = 0x00;	RAM[0x37e5] = 0x00;	/* startup check. We need a reset*/
									/* so I patch a busy loop with jp 0*/

	RAM = memory_region(REGION_CPU2);
	RAM[0x0258] = 0x53; 					/* include EI in the busy loop.*/
									/* It's an hack to repair nested nmi troubles*/
}

DRIVER_INIT( airbustj )
{
	int i;
	unsigned char *RAM;

	/* One gfx rom seems to have scrambled data (bad read?): */
	/* let's swap even and odd nibbles */
	RAM = memory_region(REGION_GFX1) + 0x000000;
	for (i = 0; i < 0x80000; i ++)
	{
		RAM[i] = ((RAM[i] & 0xF0)>>4) + ((RAM[i] & 0x0F)<<4);
	}

	RAM = memory_region(REGION_CPU1);
	RAM[0x37f4] = 0x00;	RAM[0x37f5] = 0x00;	/* startup check. We need a reset*/
									/* so I patch a busy loop with jp 0*/

	RAM = memory_region(REGION_CPU2);
	RAM[0x0258] = 0x53; 					/* include EI in the busy loop.*/
									/* It's an hack to repair nested nmi troubles*/
}



GAME( 1990, airbustr, 0,        airbustr, airbustr, airbustr, ROT0, "Kaneko (Namco license)", "Air Buster" )	/* 891220*/
GAME( 1990, airbustj, airbustr, airbustr, airbustj, airbustj, ROT0, "Kaneko (Namco license)", "Air Buster (Japan)" )	/* 891229*/
