/***************************************************************************

Punch Out memory map (preliminary)
Arm Wrestling runs on about the same hardware, but the video board is different.

TODO:
- The money bag is misplaced in armwrest bonus rounds.


driver by Nicola Salmoria


main CPU:

0000-bfff ROM
c000-c3ff NVRAM
d000-d7ff RAM
d800-dfff Video RAM (info screen)
e000-e7ff Video RAM (opponent)
e800-efff Video RAM (player)
f000-f03f Background row scroll (low/high couples)
f000-ffff Video RAM (background)

memory mapped ports:
write:
dfe0-dfef ??

dff0      big sprite #1 zoom low 8 bits
dff1      big sprite #1 zoom high 4 bits
dff2      big sprite #1 x pos low 8 bits
dff3      big sprite #1 x pos high 4 bits
dff4      big sprite #1 y pos low 8 bits
dff5      big sprite #1 y pos high bit
dff6      big sprite #1 x flip (bit 0)
dff7      big sprite #1 bit 0: show on top monitor; bit 1: show on bottom monitor

dff8      big sprite #2 x pos low 8 bits
dff9      big sprite #2 x pos high bit
dffa      big sprite #2 y pos low 8 bits
dffb      big sprite #2 y pos high bit
dffc      big sprite #2 x flip (bit 0)
dffd      palette bank (bit 0 = bottom monitor bit 1 = top monitor)

I/O
read:
00        IN0
01        IN1
02        DSW0
03        DSW1 (bit 4: VLM5030 busy signal)

write:
00        to 2A03 #1 IN0 (unpopulated)
01        to 2A03 #1 IN1 (unpopulated)
02        to 2A03 #2 IN0
03        to 2A03 #2 IN1
04        to VLM5030
08        NMI enable + watchdog reset
09        watchdog reset
0a        ? latched into Z80 BUS RQ
0b        to 2A03 #1 and #2 RESET
0c        to VLM5030 RESET
0d        to VLM5030 START
0e        to VLM5030 VCU
0f        enable NVRAM ?

sound CPU:
the sound CPU is a 2A03, which is a modified 6502 with built-in input ports
and two (analog?) outputs. The input ports are memory mapped at 4016-4017;
the outputs are more complicated. The only thing I have found is that 4011
goes straight into a DAC and produces the crowd sounds, but several addresses
in the range 4000-4017 are written to. There are probably three tone generators.

0000-07ff RAM
e000-ffff ROM

read:
4016      IN0
4017      IN1

write:
4000      ? is usually ORed with 90 or 50
4001      ? usually 7f, could be associated with 4000
4002-4003 ? tone #1 freq? (bit 3 of 4003 is always 1, bits 4-7 always 0)
4004      ? is usually ORed with 90 or 50
4005      ? usually 7f, could be associated with 4004
4006-4007 ? tone #2 freq? (bit 3 of 4007 is always 1, bits 4-7 always 0)
4008      ? at one point the max value is cut at 38
400a-400b ? tone #3 freq? (bit 3 of 400b is always 1, bits 4-7 always 0)
400c      ?
400e-400f ?
4011      DAC crowd noise
4015      ?? 00 or 0f
4017      ?? always c0

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m6502/m6502.h"


extern unsigned char *punchout_videoram2;
extern size_t punchout_videoram2_size;
extern unsigned char *punchout_bigsprite1ram;
extern size_t punchout_bigsprite1ram_size;
extern unsigned char *punchout_bigsprite2ram;
extern size_t punchout_bigsprite2ram_size;
extern unsigned char *punchout_scroll;
extern unsigned char *punchout_bigsprite1;
extern unsigned char *punchout_bigsprite2;
extern unsigned char *punchout_palettebank;
WRITE_HANDLER( punchout_videoram2_w );
WRITE_HANDLER( punchout_bigsprite1ram_w );
WRITE_HANDLER( punchout_bigsprite2ram_w );
WRITE_HANDLER( punchout_palettebank_w );
VIDEO_START( punchout );
VIDEO_START( armwrest );
PALETTE_INIT( punchout );
PALETTE_INIT( armwrest );
VIDEO_UPDATE( punchout );
VIDEO_UPDATE( armwrest );

DRIVER_INIT( punchout );
DRIVER_INIT( spnchout );
DRIVER_INIT( spnchotj );
DRIVER_INIT( armwrest );



READ_HANDLER( punchout_input_3_r )
{
	int data = input_port_3_r(offset);
	/* bit 4 is busy pin level */
	if( VLM5030_BSY() ) data &= ~0x10;
	else data |= 0x10;
	return data;
}

WRITE_HANDLER( punchout_speech_reset_w )
{
	VLM5030_RST( data&0x01 );
}

WRITE_HANDLER( punchout_speech_st_w )
{
	VLM5030_ST( data&0x01 );
}

WRITE_HANDLER( punchout_speech_vcu_w )
{
	VLM5030_VCU( data & 0x01 );
}

WRITE_HANDLER( punchout_2a03_reset_w )
{
	if (data & 1)
		cpu_set_reset_line(1,ASSERT_LINE);
	else
		cpu_set_reset_line(1,CLEAR_LINE);
}

static int prot_mode_sel = -1; /* Mode selector */
static int prot_mem[16];

static READ_HANDLER( spunchout_prot_r ) {

	switch ( offset ) {
		case 0x00:
			if ( prot_mode_sel == 0x0a )
				return cpu_readmem16(0xd012);

			if ( prot_mode_sel == 0x0b || prot_mode_sel == 0x23 )
				return cpu_readmem16(0xd7c1);

			return prot_mem[offset];
		break;

		case 0x01:
			if ( prot_mode_sel == 0x08 ) /* PC = 0x0b6a */
				return 0x00; /* under 6 */
		break;

		case 0x02:
			if ( prot_mode_sel == 0x0b ) /* PC = 0x0613 */
				return 0x09; /* write "JMP (HL)"code to 0d79fh */
			if ( prot_mode_sel == 0x09 ) /* PC = 0x20f9, 0x22d9 */
				return prot_mem[offset]; /* act as registers */
		break;

		case 0x03:
			if ( prot_mode_sel == 0x09 ) /* PC = 0x1e4c */
				return prot_mem[offset] & 0x07; /* act as registers with mask */
		break;

		case 0x05:
			if ( prot_mode_sel == 0x09 ) /* PC = 0x29D1 */
				return prot_mem[offset] & 0x03; /* AND 0FH -> AND 06H */
		break;

		case 0x06:
			if ( prot_mode_sel == 0x0b ) /* PC = 0x2dd8 */
				return 0x0a; /* E=00, HL=23E6, D = (ret and 0x0f), HL+DE = 2de6 */

			if ( prot_mode_sel == 0x09 ) /* PC = 0x2289 */
				return prot_mem[offset] & 0x07; /* act as registers with mask */
		break;

		case 0x09:
			if ( prot_mode_sel == 0x09 ) /* PC = 0x0313 */
				return ( prot_mem[15] << 4 ); /* pipe through register 0xf7 << 4 */
				/* (ret or 0x10) -> (D7DF),(D7A0) - (D7DF),(D7A0) = 0d0h(ret nc) */
		break;

		case 0x0a:
			if ( prot_mode_sel == 0x0b ) /* PC = 0x060a */
				return 0x05; /* write "JMP (IX)"code to 0d79eh */
			if ( prot_mode_sel == 0x09 ) /* PC = 0x1bd7 */
				return prot_mem[offset] & 0x01; /* AND 0FH -> AND 01H */
		break;

		case 0x0b:
			if ( prot_mode_sel == 0x09 ) /* PC = 0x2AA3 */
				return prot_mem[11] & 0x03;	/* AND 0FH -> AND 03H */
		break;

		case 0x0c:
			/* PC = 0x2162 */
			/* B = 0(return value) */
			return 0x00;
		case 0x0d:
			return prot_mode_sel;
		break;
	}

	log_cb(RETRO_LOG_DEBUG, LOGPRE "Read from unknown protection? port %02x ( selector = %02x )\n", offset, prot_mode_sel );

	return prot_mem[offset];
}

static WRITE_HANDLER( spunchout_prot_w ) {

	switch ( offset ) {
		case 0x00:
			if ( prot_mode_sel == 0x0a ) {
				cpu_writemem16(0xd012, data);
				return;
			}

			if ( prot_mode_sel == 0x0b || prot_mode_sel == 0x23 ) {
				cpu_writemem16(0xd7c1, data);
				return;
			}

			prot_mem[offset] = data;
			return;
		break;

		case 0x02:
			if ( prot_mode_sel == 0x09 ) { /* PC = 0x20f7, 0x22d7 */
				prot_mem[offset] = data;
				return;
			}
		break;

		case 0x03:
			if ( prot_mode_sel == 0x09 ) { /* PC = 0x1e4c */
				prot_mem[offset] = data;
				return;
			}
		break;

		case 0x05:
			prot_mem[offset] = data;
			return;

		case 0x06:
			if ( prot_mode_sel == 0x09 ) { /* PC = 0x2287 */
				prot_mem[offset] = data;
				return;
			}
		break;

		case 0x0b:
			prot_mem[offset] = data;
			return;

		case 0x0d: /* PC = all over the code */
			prot_mode_sel = data;
			return;
		case 0x0f:
			prot_mem[offset] = data;
			return;
	}

	log_cb(RETRO_LOG_DEBUG, LOGPRE "Wrote to unknown protection? port %02x ( %02x )\n", offset, data );

	prot_mem[offset] = data;
}

static READ_HANDLER( spunchout_prot_0_r ) {
	return spunchout_prot_r( 0 );
}

static WRITE_HANDLER( spunchout_prot_0_w ) {
	spunchout_prot_w( 0, data );
}

static READ_HANDLER( spunchout_prot_1_r ) {
	return spunchout_prot_r( 1 );
}

static WRITE_HANDLER( spunchout_prot_1_w ) {
	spunchout_prot_w( 1, data );
}

static READ_HANDLER( spunchout_prot_2_r ) {
	return spunchout_prot_r( 2 );
}

static WRITE_HANDLER( spunchout_prot_2_w ) {
	spunchout_prot_w( 2, data );
}

static READ_HANDLER( spunchout_prot_3_r ) {
	return spunchout_prot_r( 3 );
}

static WRITE_HANDLER( spunchout_prot_3_w ) {
	spunchout_prot_w( 3, data );
}

static READ_HANDLER( spunchout_prot_5_r ) {
	return spunchout_prot_r( 5 );
}

static WRITE_HANDLER( spunchout_prot_5_w ) {
	spunchout_prot_w( 5, data );
}


static READ_HANDLER( spunchout_prot_6_r ) {
	return spunchout_prot_r( 6 );
}

static WRITE_HANDLER( spunchout_prot_6_w ) {
	spunchout_prot_w( 6, data );
}

static READ_HANDLER( spunchout_prot_9_r ) {
	return spunchout_prot_r( 9 );
}

static READ_HANDLER( spunchout_prot_b_r ) {
	return spunchout_prot_r( 11 );
}

static WRITE_HANDLER( spunchout_prot_b_w ) {
	spunchout_prot_w( 11, data );
}

static READ_HANDLER( spunchout_prot_c_r ) {
	return spunchout_prot_r( 12 );
}

static WRITE_HANDLER( spunchout_prot_d_w ) {
	spunchout_prot_w( 13, data );
}

static READ_HANDLER( spunchout_prot_a_r ) {
	return spunchout_prot_r( 10 );
}

static WRITE_HANDLER( spunchout_prot_a_w ) {
	spunchout_prot_w( 10, data );
}

#if 0
static READ_HANDLER( spunchout_prot_f_r ) {
	return spunchout_prot_r( 15 );
}
#endif

static WRITE_HANDLER( spunchout_prot_f_w ) {
	spunchout_prot_w( 15, data );
}



static MEMORY_READ_START( readmem )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xc3ff, MRA_RAM },
	{ 0xd000, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc3ff, MWA_RAM, &generic_nvram, &generic_nvram_size },
	{ 0xd000, 0xd7ff, MWA_RAM },
	{ 0xdff0, 0xdff7, MWA_RAM, &punchout_bigsprite1 },
	{ 0xdff8, 0xdffc, MWA_RAM, &punchout_bigsprite2 },
	{ 0xdffd, 0xdffd, punchout_palettebank_w, &punchout_palettebank },
	{ 0xd800, 0xdfff, videoram_w, &videoram, &videoram_size },
	{ 0xe000, 0xe7ff, punchout_bigsprite1ram_w, &punchout_bigsprite1ram, &punchout_bigsprite1ram_size },
	{ 0xe800, 0xefff, punchout_bigsprite2ram_w, &punchout_bigsprite2ram, &punchout_bigsprite2ram_size },
	{ 0xf000, 0xf03f, MWA_RAM, &punchout_scroll },
	{ 0xf000, 0xffff, punchout_videoram2_w, &punchout_videoram2, &punchout_videoram2_size },
MEMORY_END

static PORT_READ_START( readport )
	{ 0x00, 0x00, input_port_0_r },
	{ 0x01, 0x01, input_port_1_r },
	{ 0x02, 0x02, input_port_2_r },
	{ 0x03, 0x03, punchout_input_3_r },

	/* protection ports */
	{ 0x07, 0x07, spunchout_prot_0_r },
	{ 0x17, 0x17, spunchout_prot_1_r },
	{ 0x27, 0x27, spunchout_prot_2_r },
	{ 0x37, 0x37, spunchout_prot_3_r },
	{ 0x57, 0x57, spunchout_prot_5_r },
	{ 0x67, 0x67, spunchout_prot_6_r },
	{ 0x97, 0x97, spunchout_prot_9_r },
	{ 0xa7, 0xa7, spunchout_prot_a_r },
	{ 0xb7, 0xb7, spunchout_prot_b_r },
	{ 0xc7, 0xc7, spunchout_prot_c_r },
	/* { 0xf7, 0xf7, spunchout_prot_f_r }, */
PORT_END

static PORT_WRITE_START( writeport )
	{ 0x00, 0x01, IOWP_NOP },	/* the 2A03 #1 is not present */
	{ 0x02, 0x02, soundlatch_w },
	{ 0x03, 0x03, soundlatch2_w },
	{ 0x04, 0x04, VLM5030_data_w },	/* VLM5030 */
	{ 0x05, 0x05, IOWP_NOP },	/* unused */
	{ 0x08, 0x08, interrupt_enable_w },
	{ 0x09, 0x09, IOWP_NOP },	/* watchdog reset, seldom used because 08 clears the watchdog as well */
	{ 0x0a, 0x0a, IOWP_NOP },	/* ?? */
	{ 0x0b, 0x0b, punchout_2a03_reset_w },
	{ 0x0c, 0x0c, punchout_speech_reset_w },	/* VLM5030 */
	{ 0x0d, 0x0d, punchout_speech_st_w    },	/* VLM5030 */
	{ 0x0e, 0x0e, punchout_speech_vcu_w   },	/* VLM5030 */
	{ 0x0f, 0x0f, IOWP_NOP },	/* enable NVRAM ? */

	{ 0x06, 0x06, IOWP_NOP},

	/* protection ports */
	{ 0x07, 0x07, spunchout_prot_0_w },
	{ 0x17, 0x17, spunchout_prot_1_w },
	{ 0x27, 0x27, spunchout_prot_2_w },
	{ 0x37, 0x37, spunchout_prot_3_w },
	{ 0x57, 0x57, spunchout_prot_5_w },
	{ 0x67, 0x67, spunchout_prot_6_w },
	{ 0xa7, 0xa7, spunchout_prot_a_w },
	{ 0xb7, 0xb7, spunchout_prot_b_w },
	{ 0xd7, 0xd7, spunchout_prot_d_w },
	{ 0xf7, 0xf7, spunchout_prot_f_w },
PORT_END

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x07ff, MRA_RAM },
	{ 0x4016, 0x4016, soundlatch_r },
	{ 0x4017, 0x4017, soundlatch2_r },
	{ 0x4000, 0x4017, NESPSG_0_r },
	{ 0xe000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x07ff, MWA_RAM },
	{ 0x4000, 0x4017, NESPSG_0_w },
	{ 0xe000, 0xffff, MWA_ROM },
MEMORY_END



INPUT_PORTS_START( punchout )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x01, "Medium" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x03, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x00, "Time" )
	PORT_DIPSETTING(    0x00, "Longest" )
	PORT_DIPSETTING(    0x04, "Long" )
	PORT_DIPSETTING(    0x08, "Short" )
	PORT_DIPSETTING(    0x0c, "Shortest" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "Rematch at a Discount" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_HIGH )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x0f, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
/*	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )*/
	PORT_DIPSETTING(    0x08, "1 Coin/2 Credits (2 Credits/1 Play)" )
	PORT_DIPSETTING(    0x0d, "1 Coin/3 Credits (2 Credits/1 Play)" )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
/*	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )*/
/*	PORT_DIPSETTING(    0x09, DEF_STR( 1C_2C ) )*/
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( Free_Play ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* VLM5030 busy signal */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_DIPNAME( 0x80, 0x00, "Copyright" )
	PORT_DIPSETTING(    0x00, "Nintendo" )
	PORT_DIPSETTING(    0x80, "Nintendo of America" )
	PORT_START
INPUT_PORTS_END

/* same as punchout with additional duck button */
INPUT_PORTS_START( spnchout )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_BUTTON4 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x01, "Medium" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x03, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x00, "Time" )
	PORT_DIPSETTING(    0x00, "Longest" )
	PORT_DIPSETTING(    0x04, "Long" )
	PORT_DIPSETTING(    0x08, "Short" )
	PORT_DIPSETTING(    0x0c, "Shortest" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "Rematch at a Discount" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_HIGH )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x0f, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 4C_1C ) )
/*	PORT_DIPSETTING(    0x09, DEF_STR( 4C_1C ) )*/
	PORT_DIPSETTING(    0x0c, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
/*	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_1C ) )*/
	PORT_DIPSETTING(    0x0d, "1 Coin/3 Credits (2 Credits/1 Play)" )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0b, "1 Coin/2 Credits (3 Credits/1 Play)" )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( Free_Play ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* VLM5030 busy signal */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_DIPNAME( 0x80, 0x00, "Copyright" )
	PORT_DIPSETTING(    0x00, "Nintendo" )
	PORT_DIPSETTING(    0x80, "Nintendo of America" )
	PORT_START
INPUT_PORTS_END

INPUT_PORTS_START( armwrest )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x01, "Medium" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x03, "Hardest" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "Rematches" )
	PORT_DIPSETTING(    0x40, "3" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_SERVICE( 0x80, IP_ACTIVE_HIGH )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* VLM5030 busy signal */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END


static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	1024,	/* 1024 characters */
	2,	/* 2 bits per pixel */
	{ 1024*8*8, 0 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout armwrest_charlayout =
{
	8,8,	/* 8*8 characters */
	2048,	/* 2048 characters */
	2,	/* 2 bits per pixel */
	{ 2048*8*8, 0 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout armwrest_charlayout2 =
{
	8,8,	/* 8*8 characters */
	2048,	/* 2048 characters */
	3,	/* 3 bits per pixel */
	{ 2*2048*8*8, 2048*8*8, 0 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout charlayout1 =
{
	8,8,	/* 8*8 characters */
	8192,	/* 8192 characters */
	3,	/* 3 bits per pixel */
	{ 2*8192*8*8, 8192*8*8, 0 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout charlayout2 =
{
	8,8,	/* 8*8 characters */
	4096,	/* 4096 characters */
	2,	/* 2 bits per pixel */
	{ 4096*8*8, 0 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxDecodeInfo punchout_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,                 0, 128 },
	{ REGION_GFX2, 0, &charlayout,             128*4, 128 },
	{ REGION_GFX3, 0, &charlayout1,      128*4+128*4,  64 },
	{ REGION_GFX4, 0, &charlayout2, 128*4+128*4+64*8, 128 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo armwrest_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &armwrest_charlayout,        0, 256 },
	{ REGION_GFX2, 0, &armwrest_charlayout2,   256*4,  64 },
	{ REGION_GFX3, 0, &charlayout1,       256*4+64*8,  64 },
	{ REGION_GFX4, 0, &charlayout2,  256*4+64*8+64*8, 128 },
	{ -1 } /* end of array */
};



static struct NESinterface nes_interface =
{
	1,
	{ REGION_CPU2 },
	{ 50 },
};


static struct VLM5030interface vlm5030_interface =
{
	3580000,    /* master clock */
	50,        /* volume       */
	REGION_SOUND1,	/* memory region of speech rom */
	0           /* memory size of speech rom */
};



static MACHINE_DRIVER_START( punchout )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 8000000/2)	/* 4 MHz */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_PORTS(readport,writeport)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,1)

	MDRV_CPU_ADD(N2A03, N2A03_DEFAULTCLOCK)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_DUAL_MONITOR)
	MDRV_ASPECT_RATIO(4,6)
	MDRV_SCREEN_SIZE(32*8, 28*8*2)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 28*8*2-1)
	MDRV_GFXDECODE(punchout_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024+1)
	MDRV_COLORTABLE_LENGTH(128*4+128*4+64*8+128*4)

	MDRV_PALETTE_INIT(punchout)
	MDRV_VIDEO_START(punchout)
	MDRV_VIDEO_UPDATE(punchout)

	/* sound hardware */
	MDRV_SOUND_ADD(NES, nes_interface)
	MDRV_SOUND_ADD(VLM5030, vlm5030_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( armwrest )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(punchout)

	/* video hardware */
	MDRV_GFXDECODE(armwrest_gfxdecodeinfo)
	MDRV_COLORTABLE_LENGTH(256*4+64*8+64*8+128*4)

	MDRV_PALETTE_INIT(armwrest)
	MDRV_VIDEO_START(armwrest)
	MDRV_VIDEO_UPDATE(armwrest)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( punchout )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "chp1-c.8l",    0x0000, 0x2000, CRC(a4003adc) SHA1(a8026eb39aa883993a0c9cb4400bf1a7e5898a2b) )
	ROM_LOAD( "chp1-c.8k",    0x2000, 0x2000, CRC(745ecf40) SHA1(430f80b688a515953fab177a3ec2eb31c886df22) )
	ROM_LOAD( "chp1-c.8j",    0x4000, 0x2000, CRC(7a7f870e) SHA1(76bb9f3ef0a2fd514db63fb77f35bde12c15c29c) )
	ROM_LOAD( "chp1-c.8h",    0x6000, 0x2000, CRC(5d8123d7) SHA1(04ddfcde969db93ff31e9c8a2af4dde285b82e2e) )
	ROM_LOAD( "chp1-c.8f",    0x8000, 0x4000, CRC(c8a55ddb) SHA1(f91fb368542c50969a086f01a2e70ecce7f2697b) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the sound CPU */
	ROM_LOAD( "chp1-c.4k",    0xe000, 0x2000, CRC(cb6ef376) SHA1(503dbcc1b18a497311bf129689d5650860bf96c7) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "chp1-b.4c",    0x00000, 0x2000, CRC(e26dc8b3) SHA1(a704d39ef6f5cbad64a478e5c109b18aae427cbc) )	/* chars #1 */
	ROM_LOAD( "chp1-b.4d",    0x02000, 0x2000, CRC(dd1310ca) SHA1(918d2eda000244b692f1da7ac57d7a0edaef95fb) )

	ROM_REGION( 0x04000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "chp1-b.4a",    0x00000, 0x2000, CRC(20fb4829) SHA1(9f0ce9379eb31c19bfacdc514ac6a28aa4217cbb) )	/* chars #2 */
	ROM_LOAD( "chp1-b.4b",    0x02000, 0x2000, CRC(edc34594) SHA1(fbb4a8b979d60b183dc23bdbb7425100b9325287) )

	ROM_REGION( 0x30000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "chp1-v.2r",    0x00000, 0x4000, CRC(bd1d4b2e) SHA1(492ae301a9890c2603d564c9048b1b67895052dd) )	/* chars #3 */
	ROM_LOAD( "chp1-v.2t",    0x04000, 0x4000, CRC(dd9a688a) SHA1(fbb98eebfbaab445928da939846a2d07a8046afb) )
	ROM_LOAD( "chp1-v.2u",    0x08000, 0x2000, CRC(da6a3c4b) SHA1(e03469fb6f552f41a9b7f4b3e51c15a52b61cf84) )
	/* 0a000-0bfff empty (space for 16k ROM) */
	ROM_LOAD( "chp1-v.2v",    0x0c000, 0x2000, CRC(8c734a67) SHA1(d59b5a2517e4890e7ca7da52ca2813a6abc484a3) )
	/* 0e000-0ffff empty (space for 16k ROM) */
	ROM_LOAD( "chp1-v.3r",    0x10000, 0x4000, CRC(2e74ad1d) SHA1(538b3f9273699106a50887c927f0251537bf0f42) )
	ROM_LOAD( "chp1-v.3t",    0x14000, 0x4000, CRC(630ba9fb) SHA1(36cec8658597239385cada3bc947b940ab66954b) )
	ROM_LOAD( "chp1-v.3u",    0x18000, 0x2000, CRC(6440321d) SHA1(c8c084ad408cb6bf65959ed4db03c4b4cf9b1c1a) )
	/* 1a000-1bfff empty (space for 16k ROM) */
	ROM_LOAD( "chp1-v.3v",    0x1c000, 0x2000, CRC(bb7b7198) SHA1(64572668d30e008daf4ccaa5689518ecc41f1091) )
	/* 1e000-1ffff empty (space for 16k ROM) */
	ROM_LOAD( "chp1-v.4r",    0x20000, 0x4000, CRC(4e5b0fe9) SHA1(c5c4fb735cc232b43c49442e62af0ebe99eaab0c) )
	ROM_LOAD( "chp1-v.4t",    0x24000, 0x4000, CRC(37ffc940) SHA1(d555807a6a1025c81637c5db0184b48306aa01ac) )
	ROM_LOAD( "chp1-v.4u",    0x28000, 0x2000, CRC(1a7521d4) SHA1(4e8a8298f2ff8257d2058e5133ad295f92c7deb8) )
	/* 2a000-2bfff empty (space for 16k ROM) */
	/* 2c000-2ffff empty (4v doesn't exist, it is seen as a 0xff fill) */

	ROM_REGION( 0x10000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "chp1-v.6p",    0x00000, 0x2000, CRC(16588f7a) SHA1(1aeaaa5cc2477c3aa4bf80df7d9474cc9ded9f15) )	/* chars #4 */
	ROM_LOAD( "chp1-v.6n",    0x02000, 0x2000, CRC(dc743674) SHA1(660582c76ee68a7267d5686a2f8ea0fd6c2b25fc) )
	/* 04000-07fff empty (space for 6l and 6k) */
	ROM_LOAD( "chp1-v.8p",    0x08000, 0x2000, CRC(c2db5b4e) SHA1(39d009af597fa28d34af31aec111aa6fe09fea39) )
	ROM_LOAD( "chp1-v.8n",    0x0a000, 0x2000, CRC(e6af390e) SHA1(73984cbdc8fbf667126ada63ab9500609eb25c61) )
	/* 0c000-0ffff empty (space for 8l and 8k) */

	ROM_REGION( 0x0d00, REGION_PROMS, 0 )
	ROM_LOAD( "chp1-b.6e",    0x0000, 0x0200, CRC(e9ca3ac6) SHA1(68d9739d8a0dadc6fe3b3767437526096ca5db98) )	/* red component */
	ROM_LOAD( "chp1-b.7e",    0x0200, 0x0200, CRC(47adf7a2) SHA1(1d37d5207cd37a9c122251c60cc8f43dd680f484) )	/* red component */
	ROM_LOAD( "chp1-b.6f",    0x0400, 0x0200, CRC(02be56ab) SHA1(a88f332cb26928350ed20ab5f4c04d5324bb516a) )	/* green component */
	ROM_LOAD( "chp1-b.8e",    0x0600, 0x0200, CRC(b0fc15a8) SHA1(a1af09cfea81231240bd94f3b98de1be8235ebe7) )	/* green component */
	ROM_LOAD( "chp1-b.7f",    0x0800, 0x0200, CRC(11de55f1) SHA1(269b82f4bc73fac197e0bb6d9a90a220e77ce478) )	/* blue component */
	ROM_LOAD( "chp1-b.8f",    0x0a00, 0x0200, CRC(1ffd894a) SHA1(9e8c1c28b4c12acf42f814bc109d353729a25652) )	/* blue component */
	ROM_LOAD( "chp1-v.2d",    0x0c00, 0x0100, CRC(71dc0d48) SHA1(dd6609f547d74887f520d7e71a1a00317ff181d0) )	/* timing - not used */

	ROM_REGION( 0x4000, REGION_SOUND1, 0 )	/* 16k for the VLM5030 data */
	ROM_LOAD( "chp1-c.6p",    0x0000, 0x4000, CRC(ea0bbb31) SHA1(b1da024cb688341d39791a78d1144fe09acb00cf) )
ROM_END

ROM_START( spnchout )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "chs1-c.8l",    0x0000, 0x2000, CRC(703b9780) SHA1(93b2fd8392ef094413330cd2474ac406c3db426e) )
	ROM_LOAD( "chs1-c.8k",    0x2000, 0x2000, CRC(e13719f6) SHA1(d0f08a0999801dd5d55f2f4ae3e76f25b765b8d6) )
	ROM_LOAD( "chs1-c.8j",    0x4000, 0x2000, CRC(1fa629e8) SHA1(e0c37883e65c77e9f25e323fb4dc05f7dcdc6347) )
	ROM_LOAD( "chs1-c.8h",    0x6000, 0x2000, CRC(15a6c068) SHA1(3f42697a6d79c6fd4b638feb366c80e98a7f02e2) )
	ROM_LOAD( "chs1-c.8f",    0x8000, 0x4000, CRC(4ff3cdd9) SHA1(282edf9a3fa085bc82523249a519f2a3fe04e87e) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the sound CPU */
	ROM_LOAD( "chp1-c.4k",    0xe000, 0x2000, CRC(cb6ef376) SHA1(503dbcc1b18a497311bf129689d5650860bf96c7) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "chs1-b.4c",    0x00000, 0x0800, CRC(9f2ede2d) SHA1(58a0f8c34ff9ec425c846c1eb6c6ccd99c2d0132) )	/* chars #1 */
	ROM_CONTINUE(             0x01000, 0x0800 )
	ROM_CONTINUE(             0x00800, 0x0800 )
	ROM_CONTINUE(             0x01800, 0x0800 )
	ROM_LOAD( "chs1-b.4d",    0x02000, 0x0800, CRC(143ae5c6) SHA1(4c8426ba336941ac3341b1dd65c0d68b9aae56de) )
	ROM_CONTINUE(             0x03000, 0x0800 )
	ROM_CONTINUE(             0x02800, 0x0800 )
	ROM_CONTINUE(             0x03800, 0x0800 )

	ROM_REGION( 0x04000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "chp1-b.4a",    0x00000, 0x0800, CRC(c075f831) SHA1(f22d9e415637599420c443ce08e7e70d1eb1c6f5) )	/* chars #2 */
	ROM_CONTINUE(             0x01000, 0x0800 )
	ROM_CONTINUE(             0x00800, 0x0800 )
	ROM_CONTINUE(             0x01800, 0x0800 )
	ROM_LOAD( "chp1-b.4b",    0x02000, 0x0800, CRC(c4cc2b5a) SHA1(7b9d4dcecc67271980c3c44561fc25a6f6c93ee3) )
	ROM_CONTINUE(             0x03000, 0x0800 )
	ROM_CONTINUE(             0x02800, 0x0800 )
	ROM_CONTINUE(             0x03800, 0x0800 )

	ROM_REGION( 0x30000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "chs1-v.2r",    0x00000, 0x4000, CRC(ff33405d) SHA1(31b892d184d24a0ec05fd6facec61a532ce8535b) )	/* chars #3 */
	ROM_LOAD( "chs1-v.2t",    0x04000, 0x4000, CRC(f507818b) SHA1(fb99c5c88e829d7e81c53ead21554a614b6fdcf9) )
	ROM_LOAD( "chs1-v.2u",    0x08000, 0x4000, CRC(0995fc95) SHA1(d056fc61ad2409525622b4db69796668c3145460) )
	ROM_LOAD( "chs1-v.2v",    0x0c000, 0x2000, CRC(f44d9878) SHA1(327a8bbc8f1a33fcf95ebc75db97406feb6435d9) )
	/* 0e000-0ffff empty (space for 16k ROM) */
	ROM_LOAD( "chs1-v.3r",    0x10000, 0x4000, CRC(09570945) SHA1(c3e2a8f76eebacc9042d087db2dfdc8ea267d46a) )
	ROM_LOAD( "chs1-v.3t",    0x14000, 0x4000, CRC(42c6861c) SHA1(2b160cde3cc3ee7adb276fe719f7919c9295ba38) )
	ROM_LOAD( "chs1-v.3u",    0x18000, 0x4000, CRC(bf5d02dd) SHA1(f1f4932fc258c087783450e7c964902fa45c4568) )
	ROM_LOAD( "chs1-v.3v",    0x1c000, 0x2000, CRC(5673f4fc) SHA1(682a81b60494b2c77d1da312c97bc807021eac67) )
	/* 1e000-1ffff empty (space for 16k ROM) */
	ROM_LOAD( "chs1-v.4r",    0x20000, 0x4000, CRC(8e155758) SHA1(d21ce2d81b2d47e5ff091e48cf46d41d01ea6314) )
	ROM_LOAD( "chs1-v.4t",    0x24000, 0x4000, CRC(b4e43448) SHA1(1ed6bf913c15851cf86554713c122b55c18c5d67) )
	ROM_LOAD( "chs1-v.4u",    0x28000, 0x4000, CRC(74e0d956) SHA1(b172cdcc5d26f3be06a7f0f9e19879957e87f992) )
	/* 2c000-2ffff empty (4v doesn't exist, it is seen as a 0xff fill) */

	ROM_REGION( 0x10000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "chp1-v.6p",    0x00000, 0x0800, CRC(75be7aae) SHA1(396bc1d301b99e064de4dad699882618b1b9c958) )	/* chars #4 */
	ROM_CONTINUE(             0x01000, 0x0800 )
	ROM_CONTINUE(             0x00800, 0x0800 )
	ROM_CONTINUE(             0x01800, 0x0800 )
	ROM_LOAD( "chp1-v.6n",    0x02000, 0x0800, CRC(daf74de0) SHA1(9373d4527b675b3128a5a830f42e1dc5dcb85307) )
	ROM_CONTINUE(             0x03000, 0x0800 )
	ROM_CONTINUE(             0x02800, 0x0800 )
	ROM_CONTINUE(             0x03800, 0x0800 )
	/* 04000-07fff empty (space for 6l and 6k) */
	ROM_LOAD( "chp1-v.8p",    0x08000, 0x0800, CRC(4cb7ea82) SHA1(213b7c1431f4c92e5519a8771035bda28b3bab8a) )
	ROM_CONTINUE(             0x09000, 0x0800 )
	ROM_CONTINUE(             0x08800, 0x0800 )
	ROM_CONTINUE(             0x09800, 0x0800 )
	ROM_LOAD( "chp1-v.8n",    0x0a000, 0x0800, CRC(1c0d09aa) SHA1(3276bae7400453f3612f53d7b47fb199cbe53e6d) )
	ROM_CONTINUE(             0x0b000, 0x0800 )
	ROM_CONTINUE(             0x0a800, 0x0800 )
	ROM_CONTINUE(             0x0b800, 0x0800 )
	/* 0c000-0ffff empty (space for 8l and 8k) */

	ROM_REGION( 0x0d00, REGION_PROMS, 0 )
	ROM_LOAD( "chs1-b.6e",    0x0000, 0x0200, CRC(0ad4d727) SHA1(5fa4247d58d10b4644f0a7492efb22b7a9ce7b62) )	/* red component */
	ROM_LOAD( "chs1-b.7e",    0x0200, 0x0200, CRC(9e170f64) SHA1(9548bfec2f5b7d222e91562b5459aef8c107b3ec) )	/* red component */
	ROM_LOAD( "chs1-b.6f",    0x0400, 0x0200, CRC(86f5cfdb) SHA1(a2a3a4e9ca15826fe8c86650d50c8ce203d57eae) )	/* green component */
	ROM_LOAD( "chs1-b.8e",    0x0600, 0x0200, CRC(3a2e333b) SHA1(5cf0324cc07ac4af63598c5c6acc61d24215b233) )	/* green component */
	ROM_LOAD( "chs1-b.7f",    0x0800, 0x0200, CRC(8bd406f8) SHA1(eaf0b62eccf1f47452bf983b3ffc6cacc25d4585) )	/* blue component */
	ROM_LOAD( "chs1-b.8f",    0x0a00, 0x0200, CRC(1663eed7) SHA1(90ff876a6b885f8a80c17531cde8b91864f1a6a5) )	/* blue component */
	ROM_LOAD( "chs1-v.2d",    0x0c00, 0x0100, CRC(71dc0d48) SHA1(dd6609f547d74887f520d7e71a1a00317ff181d0) )	/* timing - not used */

	ROM_REGION( 0x10000, REGION_SOUND1, 0 )	/* 64k for the VLM5030 data */
	ROM_LOAD( "chs1-c.6p",    0x0000, 0x4000, CRC(ad8b64b8) SHA1(0f1232a10faf71b782f9f6653cca8570243c17e0) )
ROM_END

ROM_START( spnchotj )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "chs1c8la.bin", 0x0000, 0x2000, CRC(dc2a592b) SHA1(a8a7fc5c836e2723ba6abcb1137f4c4f79e21c87) )
	ROM_LOAD( "chs1c8ka.bin", 0x2000, 0x2000, CRC(ce687182) SHA1(f07d930d90eda199b089f9023b51fd4456c87bdf) )
	ROM_LOAD( "chs1-c.8j",    0x4000, 0x2000, CRC(1fa629e8) SHA1(e0c37883e65c77e9f25e323fb4dc05f7dcdc6347) )
	ROM_LOAD( "chs1-c.8h",    0x6000, 0x2000, CRC(15a6c068) SHA1(3f42697a6d79c6fd4b638feb366c80e98a7f02e2) )
	ROM_LOAD( "chs1c8fa.bin", 0x8000, 0x4000, CRC(f745b5d5) SHA1(8130b5be011848625ebe6691fbb76dc338979b60) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the sound CPU */
	ROM_LOAD( "chp1-c.4k",    0xe000, 0x2000, CRC(cb6ef376) SHA1(503dbcc1b18a497311bf129689d5650860bf96c7) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b_4c_01a.bin", 0x00000, 0x2000, CRC(b017e1e9) SHA1(39e98f48bff762a674a2506efa39b3619337a1e0) )	/* chars #1 */
	ROM_LOAD( "b_4d_01a.bin", 0x02000, 0x2000, CRC(e3de9d18) SHA1(f55b6f522e127e6239197dd7eb1564e6f275df74) )

	ROM_REGION( 0x04000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "chp1-b.4a",    0x00000, 0x0800, CRC(c075f831) SHA1(f22d9e415637599420c443ce08e7e70d1eb1c6f5) )	/* chars #2 */
	ROM_CONTINUE(             0x01000, 0x0800 )
	ROM_CONTINUE(             0x00800, 0x0800 )
	ROM_CONTINUE(             0x01800, 0x0800 )
	ROM_LOAD( "chp1-b.4b",    0x02000, 0x0800, CRC(c4cc2b5a) SHA1(7b9d4dcecc67271980c3c44561fc25a6f6c93ee3) )
	ROM_CONTINUE(             0x03000, 0x0800 )
	ROM_CONTINUE(             0x02800, 0x0800 )
	ROM_CONTINUE(             0x03800, 0x0800 )

	ROM_REGION( 0x30000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "chs1-v.2r",    0x00000, 0x4000, CRC(ff33405d) SHA1(31b892d184d24a0ec05fd6facec61a532ce8535b) )	/* chars #3 */
	ROM_LOAD( "chs1-v.2t",    0x04000, 0x4000, CRC(f507818b) SHA1(fb99c5c88e829d7e81c53ead21554a614b6fdcf9) )
	ROM_LOAD( "chs1-v.2u",    0x08000, 0x4000, CRC(0995fc95) SHA1(d056fc61ad2409525622b4db69796668c3145460) )
	ROM_LOAD( "chs1-v.2v",    0x0c000, 0x2000, CRC(f44d9878) SHA1(327a8bbc8f1a33fcf95ebc75db97406feb6435d9) )
	/* 0e000-0ffff empty (space for 16k ROM) */
	ROM_LOAD( "chs1-v.3r",    0x10000, 0x4000, CRC(09570945) SHA1(c3e2a8f76eebacc9042d087db2dfdc8ea267d46a) )
	ROM_LOAD( "chs1-v.3t",    0x14000, 0x4000, CRC(42c6861c) SHA1(2b160cde3cc3ee7adb276fe719f7919c9295ba38) )
	ROM_LOAD( "chs1-v.3u",    0x18000, 0x4000, CRC(bf5d02dd) SHA1(f1f4932fc258c087783450e7c964902fa45c4568) )
	ROM_LOAD( "chs1-v.3v",    0x1c000, 0x2000, CRC(5673f4fc) SHA1(682a81b60494b2c77d1da312c97bc807021eac67) )
	/* 1e000-1ffff empty (space for 16k ROM) */
	ROM_LOAD( "chs1-v.4r",    0x20000, 0x4000, CRC(8e155758) SHA1(d21ce2d81b2d47e5ff091e48cf46d41d01ea6314) )
	ROM_LOAD( "chs1-v.4t",    0x24000, 0x4000, CRC(b4e43448) SHA1(1ed6bf913c15851cf86554713c122b55c18c5d67) )
	ROM_LOAD( "chs1-v.4u",    0x28000, 0x4000, CRC(74e0d956) SHA1(b172cdcc5d26f3be06a7f0f9e19879957e87f992) )
	/* 2c000-2ffff empty (4v doesn't exist, it is seen as a 0xff fill) */

	ROM_REGION( 0x10000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "chp1-v.6p",    0x00000, 0x0800, CRC(75be7aae) SHA1(396bc1d301b99e064de4dad699882618b1b9c958) )	/* chars #4 */
	ROM_CONTINUE(             0x01000, 0x0800 )
	ROM_CONTINUE(             0x00800, 0x0800 )
	ROM_CONTINUE(             0x01800, 0x0800 )
	ROM_LOAD( "chp1-v.6n",    0x02000, 0x0800, CRC(daf74de0) SHA1(9373d4527b675b3128a5a830f42e1dc5dcb85307) )
	ROM_CONTINUE(             0x03000, 0x0800 )
	ROM_CONTINUE(             0x02800, 0x0800 )
	ROM_CONTINUE(             0x03800, 0x0800 )
	/* 04000-07fff empty (space for 6l and 6k) */
	ROM_LOAD( "chp1-v.8p",    0x08000, 0x0800, CRC(4cb7ea82) SHA1(213b7c1431f4c92e5519a8771035bda28b3bab8a) )
	ROM_CONTINUE(             0x09000, 0x0800 )
	ROM_CONTINUE(             0x08800, 0x0800 )
	ROM_CONTINUE(             0x09800, 0x0800 )
	ROM_LOAD( "chp1-v.8n",    0x0a000, 0x0800, CRC(1c0d09aa) SHA1(3276bae7400453f3612f53d7b47fb199cbe53e6d) )
	ROM_CONTINUE(             0x0b000, 0x0800 )
	ROM_CONTINUE(             0x0a800, 0x0800 )
	ROM_CONTINUE(             0x0b800, 0x0800 )
	/* 0c000-0ffff empty (space for 8l and 8k) */

	ROM_REGION( 0x0d00, REGION_PROMS, 0 )
	ROM_LOAD( "chs1b_6e.bpr", 0x0000, 0x0200, CRC(8efd867f) SHA1(d5f2bfe750bb5d472922bdb7e915ee28a3eec9bd) )	/* red component */
	ROM_LOAD( "chs1-b.7e",    0x0200, 0x0200, CRC(9e170f64) SHA1(9548bfec2f5b7d222e91562b5459aef8c107b3ec) )	/* red component */
	ROM_LOAD( "chs1b_6f.bpr", 0x0400, 0x0200, CRC(279d6cbc) SHA1(aea56970801908b4d51be0c15043c7b315d2637f) )	/* green component */
	ROM_LOAD( "chs1-b.8e",    0x0600, 0x0200, CRC(3a2e333b) SHA1(5cf0324cc07ac4af63598c5c6acc61d24215b233) )	/* green component */
	ROM_LOAD( "chs1b_7f.bpr", 0x0800, 0x0200, CRC(cad6b7ad) SHA1(62b61d5fa47ca6e2dd15295674dff62e4e69471a) )	/* blue component */
	ROM_LOAD( "chs1-b.8f",    0x0a00, 0x0200, CRC(1663eed7) SHA1(90ff876a6b885f8a80c17531cde8b91864f1a6a5) )	/* blue component */
	ROM_LOAD( "chs1-v.2d",    0x0c00, 0x0100, CRC(71dc0d48) SHA1(dd6609f547d74887f520d7e71a1a00317ff181d0) )	/* timing - not used */

	ROM_REGION( 0x10000, REGION_SOUND1, 0 )	/* 64k for the VLM5030 data */
	ROM_LOAD( "chs1c6pa.bin", 0x0000, 0x4000, CRC(d05fb730) SHA1(9f4c4c7e5113739312558eff4d3d3e42d513aa31) )
ROM_END

ROM_START( armwrest )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "chv1-c.8l",    0x0000, 0x2000, CRC(b09764c1) SHA1(2f32acd689ef70ec81fe958c7a604855ae39cf5e) )
	ROM_LOAD( "chv1-c.8k",    0x2000, 0x2000, CRC(0e147ff7) SHA1(7ea8b7b5562d9432c6cace2ee13377f91543975d) )
	ROM_LOAD( "chv1-c.8j",    0x4000, 0x2000, CRC(e7365289) SHA1(9d4ed5ce73b93c3917b1411ed902974e2a4f3d35) )
	ROM_LOAD( "chv1-c.8h",    0x6000, 0x2000, CRC(a2118eec) SHA1(93e1b19819352f88888b3caf67ed27cd50f866a9) )
	ROM_LOAD( "chpv-c.8f",    0x8000, 0x4000, CRC(664a07c4) SHA1(a8a049be5beeab3940079465fb0c80382f3860f0) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the sound CPU */
	ROM_LOAD( "chp1-c.4k",    0xe000, 0x2000, CRC(cb6ef376) SHA1(503dbcc1b18a497311bf129689d5650860bf96c7) )	/* same as Punch Out */

	ROM_REGION( 0x08000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "chpv-b.2e",    0x00000, 0x4000, CRC(8b45f365) SHA1(15fadccc9afe26672fbbb8eaeaa7d3ee70bcb056) )	/* chars #1 */
	ROM_LOAD( "chpv-b.2d",    0x04000, 0x4000, CRC(b1a2850c) SHA1(e3aec428bb52443921fb7ceb5eb21b5f9ee9edcb) )

	ROM_REGION( 0x0c000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "chpv-b.2m",    0x00000, 0x4000, CRC(19245b37) SHA1(711e263d487661afca09f731e9333a84eb8d1541) )	/* chars #2 */
	ROM_LOAD( "chpv-b.2l",    0x04000, 0x4000, CRC(46797941) SHA1(e21fcec8e19702f9765205a4dc89105b4e98dcdd) )
	ROM_LOAD( "chpv-b.2k",    0x08000, 0x4000, CRC(24c4c26d) SHA1(2ed4a8fbb7858aff8a724ca34a0fac915cfc3a3a) )

	ROM_REGION( 0x30000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "chv1-v.2r",    0x00000, 0x4000, CRC(d86056d9) SHA1(decedf6b54e5990ff14d8049791b2d06c33ae71b) )	/* chars #3 */
	ROM_LOAD( "chv1-v.2t",    0x04000, 0x4000, CRC(5ad77059) SHA1(05a1c7957982fa695bca62a05dc593c7913ccd7f) )
	/* 08000-0bfff empty */
	ROM_LOAD( "chv1-v.2v",    0x0c000, 0x4000, CRC(a0fd7338) SHA1(afd8d78661c3b7149f4c491ba930a8ce66d29977) )
	ROM_LOAD( "chv1-v.3r",    0x10000, 0x4000, CRC(690e26fb) SHA1(6c20daabf5db633482b288c8020130a80cc939fc) )
	ROM_LOAD( "chv1-v.3t",    0x14000, 0x4000, CRC(ea5d7759) SHA1(4d72d7b602455349be4a9cbf34127952aa2a99ea) )
	/* 18000-1bfff empty */
	ROM_LOAD( "chv1-v.3v",    0x1c000, 0x4000, CRC(ceb37c05) SHA1(9d0e3d52e018901c2f26a9de7aa9858b106487d3) )
	ROM_LOAD( "chv1-v.4r",    0x20000, 0x4000, CRC(e291cba0) SHA1(a03ff7eea3a7a841000b67a8baeca6e82e8496ef) )
	ROM_LOAD( "chv1-v.4t",    0x24000, 0x4000, CRC(e01f3b59) SHA1(9f47507094e03735adaf033f3b99e17dd9dfd5d0) )
	/* 28000-2bfff empty */
	/* 2c000-2ffff empty (4v doesn't exist, it is seen as a 0xff fill) */

	ROM_REGION( 0x10000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "chv1-v.6p",    0x00000, 0x2000, CRC(d834e142) SHA1(e7d654145b695147b744af2284173f90749fbf0e) )	/* chars #4 */
	/* 02000-03fff empty (space for 16k ROM) */
	/* 04000-07fff empty (space for 6l and 6k) */
	ROM_LOAD( "chv1-v.8p",    0x08000, 0x2000, CRC(a2f531db) SHA1(c9be180fbc608135c892e8ee396b138f058edf24) )
	/* 0a000-0bfff empty (space for 16k ROM) */
	/* 0c000-0ffff empty (space for 8l and 8k) */

	ROM_REGION( 0x0e00, REGION_PROMS, 0 )
	ROM_LOAD( "chpv-b.7b",    0x0000, 0x0200, CRC(df6fdeb3) SHA1(7766d420cb95377104e26d96afddc83b67553c2f) )	/* red component */
	ROM_LOAD( "chpv-b.4b",    0x0200, 0x0200, CRC(9d51416e) SHA1(ae933786c5fc19311144b2094305b4253dc8b75b) )	/* red component */
	ROM_LOAD( "chpv-b.7c",    0x0400, 0x0200, CRC(b1da5f42) SHA1(55e744da70bbaa855cb1403eef028771a97578a1) )	/* green component */
	ROM_LOAD( "chpv-b.4c",    0x0600, 0x0200, CRC(b8a25795) SHA1(8e41baa796fd8f00739a95b2e07066d68193bd76) )	/* green component */
	ROM_LOAD( "chpv-b.7d",    0x0800, 0x0200, CRC(4ede813e) SHA1(6603465dae7d869c483d66768fab16f282caaa8b) )	/* blue component */
	ROM_LOAD( "chpv-b.4d",    0x0a00, 0x0200, CRC(474fc3b1) SHA1(9cda1d1626285310524d048b60b1cf89e197a26d) )	/* blue component */
	ROM_LOAD( "chv1-b.3c",    0x0c00, 0x0100, CRC(c3f92ea2) SHA1(1a82cca1b9a8d9bd4a1d121d8c131a7d0be554bc) )	/* priority encoder - not used */
	ROM_LOAD( "chpv-v.2d",    0x0d00, 0x0100, CRC(71dc0d48) SHA1(dd6609f547d74887f520d7e71a1a00317ff181d0) )	/* timing - not used */

	ROM_REGION( 0x10000, REGION_SOUND1, 0 )	/* 64k for the VLM5030 data */
	ROM_LOAD( "chv1-c.6p",    0x0000, 0x4000, CRC(31b52896) SHA1(395f59ac38b46042f79e9224ac6bc7d3dc299906) )
ROM_END



GAME( 1984, punchout, 0,        punchout, punchout, punchout, ROT0, "Nintendo", "Punch-Out!!" )
GAME( 1984, spnchout, 0,        punchout, spnchout, spnchout, ROT0, "Nintendo", "Super Punch-Out!!" )
GAME( 1984, spnchotj, spnchout, punchout, spnchout, spnchotj, ROT0, "Nintendo", "Super Punch-Out!! (Japan)" )
GAME( 1985, armwrest, 0,        armwrest, armwrest, armwrest, ROT0, "Nintendo", "Arm Wrestling" )
