/***************************************************************************

The New Zealand Story driver, used for tnzs & tnzs2.

TODO: - Find out how the hardware credit-counter works (MPU)
	  - Verify dip switches
- Fix video offsets (See Dr Toppel in Flip-Screen - also affects Chuka Taisen)
- Video scroll side flicker in Chuka Taisen, Insector X, Dr Toppel, Kabuki Z
- Sprite/background sync during scrolling, e.g. insectorx, kabukiz.
- Merge video driver with seta.c (it's the same thing but seta.c assumes a 16-bit CPU)

	Arkanoid 2:
	  - What do writes at $f400 do ?
	  - Why does the game zero the $fd00 area ?
	Extrmatn:
	  - What do reads from $f600 do ? (discarded)
	Chuka Taisen:
	  - What do writes at  $f400 do ? (value 40h)
	  - What do reads from $f600 do in service mode ?
	Dr Toppel:
	  - What do writes at  $f400 do ? (value 40h)
	  - What do reads from $f600 do in service mode ?

****************************************************************************

extrmatn and arknoid2 have a special test mode. The correct procedure to make
it succeed is as follows:
- enter service mode
- on the color test screen, press 2 (player 2 start)
- set dip switch 1 and dip switch 2 so that they read 00000001
- reset the emulation, and skip the previous step.
- press 5 (coin 1). Text at the bottom will change to "CHECKING NOW".
- use all the inputs, including tilt, until all inputs are OK
- press 5 (coin 1) - to confirm that coin lockout 1 works
- press 5 (coin 1) - to confirm that coin lockout 2 works
- set dip switch 1 to 00000000
- set dip switch 1 to 10101010
- set dip switch 1 to 11111111
- set dip switch 2 to 00000000
- set dip switch 2 to 10101010
- set dip switch 2 to 11111111
- speaker should now output a tone
- press 5 (coin 1) , to confirm that OPN works
- press 5 (coin 1) , to confirm that SSGCH1 works
- press 5 (coin 1) , to confirm that SSGCH2 works
- press 5 (coin 1) , to confirm that SSGCH3 works
- finished ("CHECK ALL OK!")

****************************************************************************

The New Zealand Story memory map (preliminary)

CPU #1
0000-7fff ROM
8000-bfff banked - banks 0-1 RAM; banks 2-7 ROM
c000-dfff object RAM, including:
  c000-c1ff sprites (code, low byte)
  c200-c3ff sprites (x-coord, low byte)
  c400-c5ff tiles (code, low byte)

  d000-d1ff sprites (code, high byte)
  d200-d3ff sprites (x-coord and colour, high byte)
  d400-d5ff tiles (code, high byte)
  d600-d7ff tiles (colour)
e000-efff RAM shared with CPU #2
f000-ffff VDC RAM, including:
  f000-f1ff sprites (y-coord)
  f200-f2ff scrolling info
  f300-f301 vdc controller
  f302-f303 scroll x-coords (high bits)
  f600      bankswitch
  f800-fbff palette

CPU #2
0000-7fff ROM
8000-9fff banked ROM
a000      bankswitch
b000-b001 YM2203 interface (with DIPs on YM2203 ports)
c000-c001 I8742 MCU
e000-efff RAM shared with CPU #1
f000-f003 inputs (used only by Arkanoid 2)

****************************************************************************/
/***************************************************************************

				Arkanoid 2 - Revenge of Doh!
					(C) 1987 Taito

						driver by

				Luca Elia (l.elia@tin.it)
				Mirko Buffoni

- The game doesn't write to f800-fbff (static palette)



			Interesting routines (main cpu)
			-------------------------------

1ed	prints the test screen (first string at 206)

47a	prints dipsw1&2 e 1p&2p paddleL values:
	e821		IN DIPSW1		e823-4	1P PaddleL (lo-hi)
	e822		IN DIPSW2		e825-6	2P PaddleL (lo-hi)

584	prints OK or NG on each entry:
	if (*addr)!=0 { if (*addr)!=2 OK else NG }
	e880	1P PADDLEL		e88a	IN SERVICE
	e881	1P PADDLER		e88b	IN TILT
	e882	1P BUTTON		e88c	OUT LOCKOUT1
	e883	1P START		e88d	OUT LOCKOUT2
	e884	2P PADDLEL		e88e	IN DIP-SW1
	e885	2P PADDLER		e88f	IN DIP-SW2
	e886	2P BUTTON		e890	SND OPN
	e887	2P START		e891	SND SSGCH1
	e888	IN COIN1		e892	SND SSGCH2
	e889	IN COIN2		e893	SND SSGCH3

672	prints a char
715	prints a string (0 terminated)

		Shared Memory (values written mainly by the sound cpu)
		------------------------------------------------------

e001=dip-sw A 	e399=coin counter value		e72c-d=1P paddle (lo-hi)
e002=dip-sw B 	e3a0-2=1P score/10 (BCD)	e72e-f=2P paddle (lo-hi)
e008=level=2*(shown_level-1)+x <- remember it's a binary tree (42 last)
e7f0=country code(from 9fde in sound rom)
e807=counter, reset by sound cpu, increased by main cpu each vblank
e80b=test progress=0(start) 1(first 8) 2(all ok) 3(error)
ec09-a~=ed05-6=xy pos of cursor in hi-scores
ec81-eca8=hi-scores(8bytes*5entries)

addr	bit	name		active	addr	bit	name		active
e72d	6	coin[1]		low		e729	1	2p select	low
		5	service		high			0	1p select	low
		4	coin[2]		low

addr	bit	name		active	addr	bit	name		active
e730	7	tilt		low		e7e7	4	1p fire		low
										0	2p fire		low

			Interesting routines (sound cpu)
			--------------------------------

4ae	check starts	B73,B7a,B81,B99	coin related
8c1	check coins		62e lockout check		664	dsw check

			Interesting locations (sound cpu)
			---------------------------------

d006=each bit is on if a corresponding location (e880-e887) has changed
d00b=(c001)>>4=tilt if 0E (security sequence must be reset?)
addr	bit	name		active
d00c	7	tilt
		6	?service?
		5	coin2		low
		4	coin1		low

d00d=each bit is on if the corresponding location (e880-e887) is 1 (OK)
d00e=each of the 4 MSBs is on if ..
d00f=FF if tilt, 00 otherwise
d011=if 00 checks counter, if FF doesn't
d23f=input port 1 value

***************************************************************************/
/***************************************************************************

Kageki
(c) 1988 Taito Corporation

Driver by Takahiro Nogi (nogi@kt.rim.or.jp) 1999/11/06

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/i8x41/i8x41.h"
#include "sound/dac.h"


/* prototypes for functions in ../machine/tnzs.c */
unsigned char *tnzs_objram, *tnzs_sharedram;
unsigned char *tnzs_vdcram, *tnzs_scrollram, *tnzs_objctrl;
DRIVER_INIT( plumpop );
DRIVER_INIT( extrmatn );
DRIVER_INIT( arknoid2 );
DRIVER_INIT( drtoppel );
DRIVER_INIT( chukatai );
DRIVER_INIT( tnzs );
DRIVER_INIT( tnzsb );
DRIVER_INIT( kabukiz );
DRIVER_INIT( insectx );
DRIVER_INIT( kageki );
READ_HANDLER( arknoid2_sh_f000_r );
MACHINE_INIT( tnzs );
INTERRUPT_GEN( arknoid2_interrupt );
READ_HANDLER( tnzs_port1_r );
READ_HANDLER( tnzs_port2_r );
WRITE_HANDLER( tnzs_port2_w );
READ_HANDLER( tnzs_mcu_r );
READ_HANDLER( tnzs_sharedram_r );
WRITE_HANDLER( tnzs_sharedram_w );
WRITE_HANDLER( tnzs_mcu_w );
WRITE_HANDLER( tnzs_bankswitch_w );
WRITE_HANDLER( tnzs_bankswitch1_w );


/* prototypes for functions in ../vidhrdw/tnzs.c */
PALETTE_INIT( arknoid2 );
VIDEO_UPDATE( tnzs );
VIDEO_EOF( tnzs );



/* max samples */
#define	MAX_SAMPLES	0x2f

int kageki_init_samples(const struct MachineSound *msound)
{
	struct GameSamples *samples;
	unsigned char *scan, *src, *dest;
	int start, size;
	int i, n;

	size = sizeof(struct GameSamples) + MAX_SAMPLES * sizeof(struct GameSamples *);

	if ((Machine->samples = auto_malloc(size)) == NULL) return 1;

	samples = Machine->samples;
	samples->total = MAX_SAMPLES;

	for (i = 0; i < samples->total; i++)
	{
		src = memory_region(REGION_SOUND1) + 0x0090;
		start = (src[(i * 2) + 1] * 256) + src[(i * 2)];
		scan = &src[start];
		size = 0;

		/* check sample length*/
		while (1)
		{
			if (*scan++ == 0x00)
			{
				break;
			} else {
				size++;
			}
		}
		if ((samples->sample[i] = auto_malloc(sizeof(struct GameSample) + size * sizeof(unsigned char))) == NULL) return 1;

		if (start < 0x100) start = size = 0;

		samples->sample[i]->smpfreq = 7000;	/* 7 KHz??? */
		samples->sample[i]->resolution = 8;	/* 8 bit */
		samples->sample[i]->length = size;

		/* signed 8-bit sample to unsigned 8-bit sample convert*/
		dest = (unsigned char *)samples->sample[i]->data;
		scan = &src[start];
		for (n = 0; n < size; n++)
		{
			*dest++ = ((*scan++) ^ 0x80);
		}
	/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "samples num:%02X ofs:%04X lng:%04X\n", i, start, size);*/
	}

	return 0;
}


static int kageki_csport_sel = 0;
static READ_HANDLER( kageki_csport_r )
{
	int	dsw, dsw1, dsw2;

	dsw1 = readinputport(0); 		/* DSW1*/
	dsw2 = readinputport(1); 		/* DSW2*/

	switch (kageki_csport_sel)
	{
		case	0x00:			/* DSW2 5,1 / DSW1 5,1*/
			dsw = (((dsw2 & 0x10) >> 1) | ((dsw2 & 0x01) << 2) | ((dsw1 & 0x10) >> 3) | ((dsw1 & 0x01) >> 0));
			break;
		case	0x01:			/* DSW2 7,3 / DSW1 7,3*/
			dsw = (((dsw2 & 0x40) >> 3) | ((dsw2 & 0x04) >> 0) | ((dsw1 & 0x40) >> 5) | ((dsw1 & 0x04) >> 2));
			break;
		case	0x02:			/* DSW2 6,2 / DSW1 6,2*/
			dsw = (((dsw2 & 0x20) >> 2) | ((dsw2 & 0x02) << 1) | ((dsw1 & 0x20) >> 4) | ((dsw1 & 0x02) >> 1));
			break;
		case	0x03:			/* DSW2 8,4 / DSW1 8,4*/
			dsw = (((dsw2 & 0x80) >> 4) | ((dsw2 & 0x08) >> 1) | ((dsw1 & 0x80) >> 6) | ((dsw1 & 0x08) >> 3));
			break;
		default:
			dsw = 0x00;
		/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "kageki_csport_sel error !! (0x%08X)\n", kageki_csport_sel);*/
	}

	return (dsw & 0xff);
}

static WRITE_HANDLER( kageki_csport_w )
{
	char mess[80];

	if (data > 0x3f)
	{
		/* read dipsw port*/
		kageki_csport_sel = (data & 0x03);
	} else {
		if (data > MAX_SAMPLES)
		{
			/* stop samples*/
			sample_stop(0);
			sprintf(mess, "VOICE:%02X STOP", data);
		} else {
			/* play samples*/
			sample_start(0, data, 0);
			sprintf(mess, "VOICE:%02X PLAY", data);
		}
	/*	usrintf_showmessage(mess);*/
	}
}

static WRITE_HANDLER( kabukiz_sound_bank_w )
{
	/* to avoid the write when the sound chip is initialized */
	if(data != 0xff)
	{
		data8_t *ROM = memory_region(REGION_CPU3);
		cpu_setbank(3, &ROM[0x10000 + 0x4000 * (data & 0x07)]);
	}
}

static WRITE_HANDLER( kabukiz_sample_w )
{
	/* to avoid the write when the sound chip is initialized */
	if(data != 0xff)
		DAC_0_data_w(0, data );
}

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 }, /* ROM + RAM */
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xefff, tnzs_sharedram_r },	/* WORK RAM (shared by the 2 z80's */
	{ 0xf000, 0xf1ff, MRA_RAM },	/* VDC RAM */
	{ 0xf600, 0xf600, MRA_NOP },	/* ? */
	{ 0xf800, 0xfbff, MRA_RAM },	/* not in extrmatn and arknoid2 (PROMs instead) */
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0xbfff, MWA_BANK1 },	/* ROM + RAM */
	{ 0xc000, 0xdfff, MWA_RAM, &tnzs_objram },
	{ 0xe000, 0xefff, tnzs_sharedram_w, &tnzs_sharedram },
	{ 0xf000, 0xf1ff, MWA_RAM, &tnzs_vdcram },
    { 0xf200, 0xf2ff, MWA_RAM, &tnzs_scrollram }, /* scrolling info */
	{ 0xf300, 0xf303, MWA_RAM, &tnzs_objctrl }, /* control registers (0x80 mirror used by Arkanoid 2) */
	{ 0xf3fc, 0xf3ff, MWA_RAM, &tnzs_objctrl }, /* mirror of above */
	{ 0xf400, 0xf400, MWA_NOP },	/* ? */
	{ 0xf600, 0xf600, tnzs_bankswitch_w },
	/* arknoid2, extrmatn, plumppop and drtoppel have PROMs instead of RAM */
	/* drtoppel writes here anyway! (maybe leftover from tests during development) */
	/* so the handler is patched out in init_drtopple() */
	{ 0xf800, 0xfbff, paletteram_xRRRRRGGGGGBBBBB_w, &paletteram },
MEMORY_END

static MEMORY_READ_START( tnzsb_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 }, /* ROM + RAM */
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xefff, tnzs_sharedram_r },	/* WORK RAM (shared by the 2 z80's */
	{ 0xf000, 0xf1ff, MRA_RAM },	/* VDC RAM */
	{ 0xf800, 0xfbff, MRA_RAM },	/* not in extrmatn and arknoid2 (PROMs instead) */
MEMORY_END

static MEMORY_WRITE_START( tnzsb_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0xbfff, MWA_BANK1 },	/* ROM + RAM */
	{ 0xc000, 0xdfff, MWA_RAM, &tnzs_objram },
	{ 0xe000, 0xefff, tnzs_sharedram_w, &tnzs_sharedram },
	{ 0xf000, 0xf1ff, MWA_RAM, &tnzs_vdcram },
    { 0xf200, 0xf2ff, MWA_RAM, &tnzs_scrollram }, /* scrolling info */
	{ 0xf300, 0xf303, MWA_RAM, &tnzs_objctrl }, /* control registers (0x80 mirror used by Arkanoid 2) */
	{ 0xf3fc, 0xf3ff, MWA_RAM, &tnzs_objctrl }, /* mirror of above */
	{ 0xf400, 0xf400, MWA_NOP },	/* ? */
	{ 0xf600, 0xf600, tnzs_bankswitch_w },
	/* kabukiz still writes here but it's not used (it's paletteram in type1 map) */
	{ 0xf800, 0xfbff, MWA_NOP },
MEMORY_END	

static MEMORY_READ_START( sub_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x9fff, MRA_BANK2 },
	{ 0xb000, 0xb000, YM2203_status_port_0_r },
	{ 0xb001, 0xb001, YM2203_read_port_0_r },
	{ 0xc000, 0xc001, tnzs_mcu_r },	/* plain input ports in insectx (memory handler */
									/* changed in insectx_init() ) */
	{ 0xd000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xefff, tnzs_sharedram_r },
	{ 0xf000, 0xf003, arknoid2_sh_f000_r },	/* paddles in arkanoid2/plumppop. The ports are */
						/* read but not used by the other games, and are not read at */
						/* all by insectx. */
MEMORY_END

static MEMORY_WRITE_START( sub_writemem )
	{ 0x0000, 0x9fff, MWA_ROM },
	{ 0xa000, 0xa000, tnzs_bankswitch1_w },
	{ 0xb000, 0xb000, YM2203_control_port_0_w },
	{ 0xb001, 0xb001, YM2203_write_port_0_w },
	{ 0xc000, 0xc001, tnzs_mcu_w },	/* not present in insectx */
	{ 0xd000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xefff, tnzs_sharedram_w },
MEMORY_END


static MEMORY_READ_START( kageki_sub_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x9fff, MRA_BANK2 },
	{ 0xb000, 0xb000, YM2203_status_port_0_r },
	{ 0xb001, 0xb001, YM2203_read_port_0_r },
	{ 0xc000, 0xc000, input_port_2_r },
	{ 0xc001, 0xc001, input_port_3_r },
	{ 0xc002, 0xc002, input_port_4_r },
	{ 0xd000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xefff, tnzs_sharedram_r },
MEMORY_END

static MEMORY_WRITE_START( kageki_sub_writemem )
	{ 0x0000, 0x9fff, MWA_ROM },
	{ 0xa000, 0xa000, tnzs_bankswitch1_w },
	{ 0xb000, 0xb000, YM2203_control_port_0_w },
	{ 0xb001, 0xb001, YM2203_write_port_0_w },
	{ 0xd000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xefff, tnzs_sharedram_w },
MEMORY_END

/* the later board is different, it has a third CPU (and of course no mcu) */

static WRITE_HANDLER( tnzsb_sound_command_w )
{
	soundlatch_w(offset,data);
	cpu_set_irq_line_and_vector(2,0,HOLD_LINE,0xff);
}

static MEMORY_READ_START( tnzsb_readmem1 )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x9fff, MRA_BANK2 },
	{ 0xb002, 0xb002, input_port_0_r },
	{ 0xb003, 0xb003, input_port_1_r },
	{ 0xc000, 0xc000, input_port_2_r },
	{ 0xc001, 0xc001, input_port_3_r },
	{ 0xc002, 0xc002, input_port_4_r },
	{ 0xd000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xefff, tnzs_sharedram_r },
	{ 0xf000, 0xf003, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( tnzsb_writemem1 )
	{ 0x0000, 0x9fff, MWA_ROM },
	{ 0xd000, 0xdfff, MWA_RAM },
	{ 0xa000, 0xa000, tnzs_bankswitch1_w },
	{ 0xb004, 0xb004, tnzsb_sound_command_w },
	{ 0xe000, 0xefff, tnzs_sharedram_w },
	{ 0xf000, 0xf3ff, paletteram_xRRRRRGGGGGBBBBB_w, &paletteram },
MEMORY_END

static MEMORY_READ_START( kabukiz_readmem1 )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x9fff, MRA_BANK2 },
	{ 0xb002, 0xb002, input_port_0_r },
	{ 0xb003, 0xb003, input_port_1_r },
	{ 0xc000, 0xc000, input_port_2_r },
	{ 0xc001, 0xc001, input_port_3_r },
	{ 0xc002, 0xc002, input_port_4_r },
	{ 0xd000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xefff, tnzs_sharedram_r },
MEMORY_END

static MEMORY_WRITE_START( kabukiz_writemem1 )
	{ 0x0000, 0x9fff, MWA_ROM },
	{ 0xd000, 0xdfff, MWA_RAM },
	{ 0xa000, 0xa000, tnzs_bankswitch1_w },
	{ 0xb004, 0xb004, tnzsb_sound_command_w },
	{ 0xe000, 0xefff, tnzs_sharedram_w },
	{ 0xf800, 0xfbff, paletteram_xRRRRRGGGGGBBBBB_w, &paletteram },
MEMORY_END

static MEMORY_READ_START( tnzsb_readmem2 )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xc000, 0xdfff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( tnzsb_writemem2 )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM },
MEMORY_END

static MEMORY_READ_START( kabukiz_readmem2 )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK3 },
	{ 0xe000, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( kabukiz_writemem2 )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0xbfff, MWA_BANK3 },
	{ 0xe000, 0xffff, MWA_RAM },
MEMORY_END

static PORT_READ_START( tnzsb_readport )
	{ 0x00, 0x00, YM2203_status_port_0_r  },
	{ 0x02, 0x02, soundlatch_r  },
PORT_END

static PORT_WRITE_START( tnzsb_writeport )
	{ 0x00, 0x00, YM2203_control_port_0_w  },
	{ 0x01, 0x01, YM2203_write_port_0_w  },
PORT_END


static MEMORY_READ_START( i8742_readmem )
	{ 0x0000, 0x07ff, MRA_ROM },
	{ 0x0800, 0x08ff, MRA_RAM },	/* Internal i8742 RAM */
MEMORY_END

static MEMORY_WRITE_START( i8742_writemem )
	{ 0x0000, 0x07ff, MWA_ROM },
	{ 0x0800, 0x08ff, MWA_RAM },	/* Internal i8742 RAM */
MEMORY_END

static PORT_READ_START( i8742_readport )
	{ 0x01, 0x01, tnzs_port1_r },
	{ 0x02, 0x02, tnzs_port2_r },
	{ I8X41_t0, I8X41_t0, input_port_5_r },
	{ I8X41_t1, I8X41_t1, input_port_6_r },
PORT_END

static PORT_WRITE_START( i8742_writeport )
	{ 0x02, 0x02, tnzs_port2_w },
PORT_END



INPUT_PORTS_START( extrmatn )
	PORT_START		/* DSW A */
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

	PORT_START		/* DSW B */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
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
	PORT_DIPNAME( 0xc0, 0xc0, "Damage Multiplicator" )
	PORT_DIPSETTING(    0xc0, "*1" )
	PORT_DIPSETTING(    0x80, "*1.5" )
	PORT_DIPSETTING(    0x40, "*2" )
	PORT_DIPSETTING(    0x00, "*3" )

	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START		/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START		/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START		/* Coin 1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0xfe, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START		/* Coin 2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0xfe, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( arknoid2 )
	PORT_START		/* DSW1 - IN2 */
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
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

	PORT_START		/* DSW2 - IN3 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Very Hard" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "50k 150k" )
	PORT_DIPSETTING(    0x0c, "100k 200k" )
	PORT_DIPSETTING(    0x04, "50k Only" )
	PORT_DIPSETTING(    0x08, "100k Only" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x80, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )

	PORT_START		/* IN1 - read at c000 (sound cpu) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START		/* empty */

	PORT_START		/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START		/* Coin 1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0xfe, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START		/* Coin 2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0xfe, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START		/* spinner 1 - read at f000/1 */
	PORT_ANALOG( 0x0fff, 0x0000, IPT_DIAL, 70, 15, 0, 0 )
	PORT_BIT   ( 0x1000, IP_ACTIVE_LOW,  IPT_COIN2 )	/* Mirrored for service mode */
	PORT_BIT   ( 0x2000, IP_ACTIVE_HIGH, IPT_SERVICE1 )	/* Mirrored for service mode */
	PORT_BIT   ( 0x4000, IP_ACTIVE_LOW,  IPT_COIN1 )	/* Mirrored for service mode */
	PORT_BIT   ( 0x8000, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START		/* spinner 2 - read at f002/3 */
	PORT_ANALOG( 0x0fff, 0x0000, IPT_DIAL | IPF_PLAYER2, 70, 15, 0, 0 )
	PORT_BIT   ( 0xf000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( arknid2u )
	PORT_START		/* DSW1 - IN2 */
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

	PORT_START		/* DSW2 - IN3 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Very Hard" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "50k 150k" )
	PORT_DIPSETTING(    0x0c, "100k 200k" )
	PORT_DIPSETTING(    0x04, "50k Only" )
	PORT_DIPSETTING(    0x08, "100k Only" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x80, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )

	PORT_START		/* IN1 - read at c000 (sound cpu) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START		/* empty */

	PORT_START		/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START		/* Coin 1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0xfe, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START		/* Coin 2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0xfe, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START		/* spinner 1 - read at f000/1 */
	PORT_ANALOG( 0x0fff, 0x0000, IPT_DIAL, 70, 15, 0, 0 )
	PORT_BIT   ( 0x1000, IP_ACTIVE_LOW,  IPT_COIN2 )	/* Mirrored for service mode */
	PORT_BIT   ( 0x2000, IP_ACTIVE_HIGH, IPT_SERVICE1 )	/* Mirrored for service mode */
	PORT_BIT   ( 0x4000, IP_ACTIVE_LOW,  IPT_COIN1 )	/* Mirrored for service mode */
	PORT_BIT   ( 0x8000, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START		/* spinner 2 - read at f002/3 */
	PORT_ANALOG( 0x0fff, 0x0000, IPT_DIAL | IPF_PLAYER2, 70, 15, 0, 0 )
	PORT_BIT   ( 0xf000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( plumppop )
	PORT_START		/* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
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

	PORT_START		/* DSW B */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Medium" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x08, "50k 150k" )
	PORT_DIPSETTING(    0x0c, "70k 200k" )
	PORT_DIPSETTING(    0x04, "100k 250k" )
	PORT_DIPSETTING(    0x00, "200k 300k" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Yes ) )

	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_CHEAT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START		/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_CHEAT | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START		/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START		/* Coin 1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0xfe, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START		/* Coin 2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0xfe, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START		/* spinner 1 - read at f000/1 */
	PORT_ANALOG( 0xffff, 0x0000, IPT_DIAL, 70, 15, 0, 0 )

	PORT_START		/* spinner 2 - read at f002/3 */
	PORT_ANALOG( 0xffff, 0x0000, IPT_DIAL | IPF_PLAYER2, 70, 15, 0, 0 )
INPUT_PORTS_END

INPUT_PORTS_START( drtoppel )
	PORT_START		/* DSW A */
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
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

	PORT_START		/* DSW B */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Medium" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x0c, "30000" )
	PORT_DIPSETTING(    0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Unknown ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START		/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START		/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START		/* Coin 1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0xfe, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START		/* Coin 2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0xfe, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( drtopplu )
	PORT_START		/* DSW A */
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

	PORT_START		/* DSW B */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Medium" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x0c, "30000" )
	PORT_DIPSETTING(    0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Unknown ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START		/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START		/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START		/* Coin 1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0xfe, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START		/* Coin 2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0xfe, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( chukatai )
	PORT_START		/* DSW A */
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
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

	PORT_START		/* DSW B */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Medium" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x08, "100k 300k 440k" )
	PORT_DIPSETTING(    0x00, "100k 300k 500k" )
	PORT_DIPSETTING(    0x0c, "100k 400k" )
	PORT_DIPSETTING(    0x04, "100k 500k" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START		/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START		/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START		/* Coin 1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0xfe, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START		/* Coin 2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0xfe, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( chukatau )
	PORT_START		/* DSW A */
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

	PORT_START		/* DSW B */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Medium" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x08, "100k 300k 440k" )
	PORT_DIPSETTING(    0x00, "100k 300k 500k" )
	PORT_DIPSETTING(    0x0c, "100k 400k" )
	PORT_DIPSETTING(    0x04, "100k 500k" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START		/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START		/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START		/* Coin 1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0xfe, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START		/* Coin 2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0xfe, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( tnzs )
	PORT_START		/* DSW A */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
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

	PORT_START		/* DSW B */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Medium" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "50000 150000" )
	PORT_DIPSETTING(    0x0c, "70000 200000" )
	PORT_DIPSETTING(    0x04, "100000 250000" )
	PORT_DIPSETTING(    0x08, "200000 300000" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START		/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START		/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START		/* Coin 1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0xfe, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START		/* Coin 2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0xfe, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( tnzsb )
	PORT_START		/* DSW A */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

	PORT_START		/* DSW B */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Medium" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "50000 150000" )
	PORT_DIPSETTING(    0x0c, "70000 200000" )
	PORT_DIPSETTING(    0x04, "100000 250000" )
	PORT_DIPSETTING(    0x08, "200000 300000" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START		/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START		/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( tnzs2 )
	PORT_START		/* DSW A */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

	PORT_START		/* DSW B */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Medium" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "10000 100000" )
	PORT_DIPSETTING(    0x0c, "10000 150000" )
	PORT_DIPSETTING(    0x08, "10000 200000" )
	PORT_DIPSETTING(    0x04, "10000 300000" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START		/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START		/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START		/* Coin 1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0xfe, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START		/* Coin 2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0xfe, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( insectx )
	PORT_START		/* DSW A */
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
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

	PORT_START		/* DSW B */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Medium" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x08, "100k 200k 300k 440k" )
	PORT_DIPSETTING(    0x0c, "100k 400k" )
	PORT_DIPSETTING(    0x04, "100k 500k" )
	PORT_DIPSETTING(    0x00, "150000 Only" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START		/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START		/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( kageki )
	PORT_START		/* DSW A */
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

	PORT_START		/* DSW B */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Medium" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
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
	PORT_DIPNAME( 0x80, 0x80, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Yes ) )

	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START		/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START		/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( kabukiz )
	PORT_START
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
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Allow_Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Yes ) )

	PORT_START
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, "Easy" )
	PORT_DIPSETTING(    0x02, "Medium" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )
	
    PORT_START	
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2)

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( kabukizj )
	PORT_START
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
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Allow_Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Yes ) )

	PORT_START
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, "Easy" )
	PORT_DIPSETTING(    0x02, "Medium" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
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
	
    PORT_START	
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2)

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

static struct GfxLayout tnzs_charlayout =
{
	16,16,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(3,4), RGN_FRAC(2,4), RGN_FRAC(1,4), RGN_FRAC(0,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8
};

static struct GfxLayout insectx_charlayout =
{
	16,16,
	8192,
	4,
	{ 8, 0, 8192*64*8+8, 8192*64*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*16+0, 8*16+1, 8*16+2, 8*16+3, 8*16+4, 8*16+5, 8*16+6, 8*16+7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
		16*16, 17*16, 18*16, 19*16, 20*16, 21*16, 22*16, 23*16 },
	64*8
};

static struct GfxDecodeInfo tnzs_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tnzs_charlayout, 0, 32 },
	{ -1 }	/* end of array */
};

static struct GfxDecodeInfo insectx_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &insectx_charlayout, 0, 32 },
	{ -1 }	/* end of array */
};


static struct YM2203interface ym2203_interface =
{
	1,			/* 1 chip */
	3000000,	/* 3 MHz  */
	{ YM2203_VOL(30,30) },
	{ input_port_0_r },		/* DSW1 connected to port A */
	{ input_port_1_r },		/* DSW2 connected to port B */
	{ 0 },
	{ 0 }
};


/* handler called by the 2203 emulator when the internal timers cause an IRQ */
static void irqhandler(int irq)
{
	cpu_set_nmi_line(2,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2203interface ym2203b_interface =
{
	1,			/* 1 chip */
	3000000,	/* 3 MHz  */
	{ YM2203_VOL(MIXERG(100,MIXER_GAIN_2x,MIXER_PAN_CENTER),100) },	/* compensate for very low volume */
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler }
};

static struct YM2203interface kageki_ym2203_interface =
{
	1,					/* 1 chip */
	3000000,			/* 12000000/4 ??? */
	{ YM2203_VOL(35, 15) },
	{ kageki_csport_r },
	{ 0 },
	{ 0 },
	{ kageki_csport_w },
};

static struct YM2203interface kabukiz_ym2203_interface =
{
	1,					/* 1 chip */
	3000000,			/* 12000000/4 ??? */
	{ YM2203_VOL(MIXERG(100,MIXER_GAIN_2x,MIXER_PAN_CENTER),100) },	/* compensate for very low volume */
	{ 0 },
	{ 0 },
	{ kabukiz_sound_bank_w },
	{ kabukiz_sample_w },
	{ irqhandler }
};


static struct DACinterface dac_interface =
{
	1,
	{ 50, }
};

static struct Samplesinterface samples_interface =
{
	1,					/* 1 channel */
	100					/* volume */
};

static struct CustomSound_interface custom_interface =
{
	kageki_init_samples,
	0,
	0
};


static MACHINE_DRIVER_START( arknoid2 )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 8000000)	/* ?? Hz (only crystal is 12MHz) */
								/* 8MHz is wrong, but extrmatn doesn't work properly at 6MHz */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(arknoid2_interrupt,1)

	MDRV_CPU_ADD(Z80, 6000000)	/* ?? Hz */
	MDRV_CPU_MEMORY(sub_readmem,sub_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)

	MDRV_MACHINE_INIT(tnzs)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(tnzs_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_PALETTE_INIT(arknoid2)
	MDRV_VIDEO_UPDATE(tnzs)
	MDRV_VIDEO_EOF(tnzs)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, ym2203_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( drtoppel )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,12000000/2)		/* 6.0 MHz ??? - Main board Crystal is 12MHz */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(arknoid2_interrupt,1)

	MDRV_CPU_ADD(Z80,12000000/2)		/* 6.0 MHz ??? - Main board Crystal is 12MHz */
	MDRV_CPU_MEMORY(sub_readmem,sub_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)

	MDRV_MACHINE_INIT(tnzs)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(tnzs_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_PALETTE_INIT(arknoid2)
	MDRV_VIDEO_UPDATE(tnzs)
	MDRV_VIDEO_EOF(tnzs)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, ym2203_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( tnzs )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,12000000/2)		/* 6.0 MHz ??? - Main board Crystal is 12MHz */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80,12000000/2)		/* 6.0 MHz ??? - Main board Crystal is 12MHz */
	MDRV_CPU_MEMORY(sub_readmem,sub_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(I8X41,(12000000/2)/I8X41_CLOCK_DIVIDER)	/* 400KHz ??? - Main board Crystal is 12MHz */
	MDRV_CPU_MEMORY(i8742_readmem,i8742_writemem)
	MDRV_CPU_PORTS(i8742_readport,i8742_writeport)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)

	MDRV_MACHINE_INIT(tnzs)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(tnzs_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_VIDEO_UPDATE(tnzs)
	MDRV_VIDEO_EOF(tnzs)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, ym2203_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( insectx )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 6000000)	/* 6 MHz(?) */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, 6000000)	/* 6 MHz(?) */
	MDRV_CPU_MEMORY(sub_readmem,sub_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)

	MDRV_MACHINE_INIT(tnzs)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(insectx_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_VIDEO_UPDATE(tnzs)
	MDRV_VIDEO_EOF(tnzs)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, ym2203_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( kageki )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 6000000)		/* 12000000/2 ??? */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, 4000000)		/* 12000000/3 ??? */
	MDRV_CPU_MEMORY(kageki_sub_readmem,kageki_sub_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)

	MDRV_MACHINE_INIT(tnzs)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(tnzs_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_VIDEO_UPDATE(tnzs)
	MDRV_VIDEO_EOF(tnzs)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, kageki_ym2203_interface)
	MDRV_SOUND_ADD(SAMPLES, samples_interface)
	MDRV_SOUND_ADD(CUSTOM, custom_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( tnzsb )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 6000000)		/* 6 MHz */
    MDRV_CPU_MEMORY(tnzsb_readmem,tnzsb_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, 6000000)		/* 6 MHz */
	MDRV_CPU_MEMORY(tnzsb_readmem1,tnzsb_writemem1)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, 6000000)		/* 6 MHz */
	MDRV_CPU_MEMORY(tnzsb_readmem2,tnzsb_writemem2)
	MDRV_CPU_PORTS(tnzsb_readport,tnzsb_writeport)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)

	MDRV_MACHINE_INIT(tnzs)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(tnzs_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_VIDEO_UPDATE(tnzs)
	MDRV_VIDEO_EOF(tnzs)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, ym2203b_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( kabukiz )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 6000000)		/* 6 MHz */
    MDRV_CPU_MEMORY(tnzsb_readmem,tnzsb_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, 6000000)		/* 6 MHz */
	MDRV_CPU_MEMORY(kabukiz_readmem1,kabukiz_writemem1)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, 6000000)		/* 6 MHz */
	MDRV_CPU_MEMORY(kabukiz_readmem2,kabukiz_writemem2)
	MDRV_CPU_PORTS(tnzsb_readport,tnzsb_writeport)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)

	MDRV_MACHINE_INIT(tnzs)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(tnzs_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_VIDEO_UPDATE(tnzs)
	MDRV_VIDEO_EOF(tnzs)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, kabukiz_ym2203_interface)
	MDRV_SOUND_ADD(DAC, dac_interface)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( plumppop )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )	/* 64k + bankswitch areas for the first CPU */
	ROM_LOAD( "a98-09.bin", 0x00000, 0x08000, CRC(107f9e06) SHA1(0aa7f32721c3cab96eccc7c831b9f57877c4e1dc) )
	ROM_CONTINUE(           0x18000, 0x08000 )				/* banked at 8000-bfff */
	ROM_LOAD( "a98-10.bin", 0x20000, 0x10000, CRC(df6e6af2) SHA1(792f97f587e84cdd67f0d1efe1fd13ea904d7e20) )	/* banked at 8000-bfff */

	ROM_REGION( 0x18000, REGION_CPU2, 0 )	/* 64k for the second CPU */
	ROM_LOAD( "a98-11.bin", 0x00000, 0x08000, CRC(bc56775c) SHA1(0c22c22c0e9d7ec0e34f8ab4bfe61068f65e8759) )
	ROM_CONTINUE(           0x10000, 0x08000 )		/* banked at 8000-9fff */

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* M-Chip (i8742 internal ROM) */
	ROM_LOAD( "plmp8742.bin", 0x0000, 0x0800, NO_DUMP )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "a98-01.bin", 0x00000, 0x10000, CRC(f3033dca) SHA1(130744998f0531a82de2814231dddea3ad710f60) )
	ROM_RELOAD(             0x10000, 0x10000 )
	ROM_LOAD( "a98-02.bin", 0x20000, 0x10000, CRC(f2d17b0c) SHA1(418c8e383b8d4d54d723ae3512829a95e6897ee1) )
	ROM_RELOAD(             0x30000, 0x10000 )
	ROM_LOAD( "a98-03.bin", 0x40000, 0x10000, CRC(1a519b0a) SHA1(9217c6bf564ccd4a44f9cf2045102e667dc0b036) )
	ROM_RELOAD(             0x40000, 0x10000 )
	ROM_LOAD( "a98-04.bin", 0x60000, 0x10000, CRC(b64501a1) SHA1(6d96172b7d7d2276787013fe6b47bb7fef0a4e36) )
	ROM_RELOAD(             0x70000, 0x10000 )
	ROM_LOAD( "a98-05.bin", 0x80000, 0x10000, CRC(45c36963) SHA1(2f23bff22e218f542c50bf7e4ae8ab6db93180b0) )
	ROM_RELOAD(             0x90000, 0x10000 )
	ROM_LOAD( "a98-06.bin", 0xa0000, 0x10000, CRC(e075341b) SHA1(b5e68b5da7933c7eff21fa832e089edcbb49cdb4) )
	ROM_RELOAD(             0xb0000, 0x10000 )
	ROM_LOAD( "a98-07.bin", 0xc0000, 0x10000, CRC(8e16cd81) SHA1(6bc9dc8e29197b75c3c4ac4f066037bb9b8cebb4) )
	ROM_RELOAD(             0xd0000, 0x10000 )
	ROM_LOAD( "a98-08.bin", 0xe0000, 0x10000, CRC(bfa7609a) SHA1(0b9aa89b5954334f40dda1f14b1691852c74fc37) )
	ROM_RELOAD(             0xf0000, 0x10000 )

	ROM_REGION( 0x0400, REGION_PROMS, 0 )		/* color proms */
	ROM_LOAD( "a98-13.bpr", 0x0000, 0x200, CRC(7cde2da5) SHA1(0cccfc35fb716ebb4cffa85c75681f33ca80a56e) )	/* hi bytes */
	ROM_LOAD( "a98-12.bpr", 0x0200, 0x200, CRC(90dc9da7) SHA1(f719dead7f4597e5ee6f1103599505b98cb58299) )	/* lo bytes */
ROM_END

ROM_START( extrmatn )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )				/* Region 0 - main cpu */
	ROM_LOAD( "b06-20.bin", 0x00000, 0x08000, CRC(04e3fc1f) SHA1(b1cf2f79f43fa33d6175368c897f84ec6aa6e746) )
	ROM_CONTINUE(           0x18000, 0x08000 )				/* banked at 8000-bfff */
	ROM_LOAD( "b06-21.bin", 0x20000, 0x10000, CRC(1614d6a2) SHA1(f23d465af231ab5653c55748f686d8f25f52394b) )	/* banked at 8000-bfff */

	ROM_REGION( 0x18000, REGION_CPU2, 0 )				/* Region 2 - sound cpu */
	ROM_LOAD( "b06-06.bin", 0x00000, 0x08000, CRC(744f2c84) SHA1(7565c1594c2a3bae1ae45afcbf93363fe2b12d58) )
	ROM_CONTINUE(           0x10000, 0x08000 )	/* banked at 8000-9fff */

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* M-Chip (i8742 internal ROM) */
	ROM_LOAD( "extr8742.bin", 0x0000, 0x0800, NO_DUMP )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b06-01.bin", 0x00000, 0x20000, CRC(d2afbf7e) SHA1(28b4cf94798f049a9f8375464741dbef208d7290) )
	ROM_LOAD( "b06-02.bin", 0x20000, 0x20000, CRC(e0c2757a) SHA1(3c89044caa28b10b4d1bef1515881810c23d312a) )
	ROM_LOAD( "b06-03.bin", 0x40000, 0x20000, CRC(ee80ab9d) SHA1(f4e4833cadff7d856b5a8075a61d902427653e16) )
	ROM_LOAD( "b06-04.bin", 0x60000, 0x20000, CRC(3697ace4) SHA1(6d6e4e64147365bcfcf74a84eb7ae84dffedd304) )

	ROM_REGION( 0x0400, REGION_PROMS, 0 )
	ROM_LOAD( "b06-09.bin", 0x00000, 0x200, CRC(f388b361) SHA1(f00db6ad6994cfe9b7ad76e30b7049b11f8c16e4) )	/* hi bytes */
	ROM_LOAD( "b06-08.bin", 0x00200, 0x200, CRC(10c9aac3) SHA1(09d6f791dea358e78099af7a370b00b8504ffc97) )	/* lo bytes */
ROM_END

ROM_START( arknoid2 )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )				/* Region 0 - main cpu */
	ROM_LOAD( "b08_05.11c",	0x00000, 0x08000, CRC(136edf9d) SHA1(f632321650897eee585511a84f451a205d1f7704) )
	ROM_CONTINUE(           0x18000, 0x08000 )			/* banked at 8000-bfff */
	/* 20000-2ffff empty */

	ROM_REGION( 0x18000, REGION_CPU2, 0 )				/* Region 2 - sound cpu */
	ROM_LOAD( "b08_13.3e", 0x00000, 0x08000, CRC(e8035ef1) SHA1(9a54e952cff0036c4b6affd9ffb1097cdccbe255) )
	ROM_CONTINUE(          0x10000, 0x08000 )			/* banked at 8000-9fff */

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* M-Chip (i8742 internal ROM) */
	ROM_LOAD( "ark28742.bin", 0x0000, 0x0800, NO_DUMP )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b08-01.13a",	0x00000, 0x20000, CRC(2ccc86b4) SHA1(eced1d7e687db0331507726946b6a19a690a7604) )
	ROM_LOAD( "b08-02.10a",	0x20000, 0x20000, CRC(056a985f) SHA1(6333b71c631d3307929aae633760870451830e10) )
	ROM_LOAD( "b08-03.7a",	0x40000, 0x20000, CRC(274a795f) SHA1(49353590e1a418843f57c715185e407a20021936) )
	ROM_LOAD( "b08-04.4a",	0x60000, 0x20000, CRC(9754f703) SHA1(0018ebf7da3f501345f3f5085d98d7614f8ce1b6) )

	ROM_REGION( 0x0400, REGION_PROMS, 0 )
	ROM_LOAD( "b08-08.15f",	0x00000, 0x200, CRC(a4f7ebd9) SHA1(094eb63c18898c6ee8d722492bdfd28091c61773) )	/* hi bytes */
	ROM_LOAD( "b08-07.16f",	0x00200, 0x200, CRC(ea34d9f7) SHA1(9a46edc64f961bd96908419cabd92445d300fc19) )	/* lo bytes */
ROM_END

ROM_START( arknid2u )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )				/* Region 0 - main cpu */
	ROM_LOAD( "b08_11.11c", 0x00000, 0x08000, CRC(99555231) SHA1(2798f3f5b3f1fa27598fe7a6e95c75d9142c8d34) )
	ROM_CONTINUE(           0x18000, 0x08000 )			/* banked at 8000-bfff */
	/* 20000-2ffff empty */

	ROM_REGION( 0x18000, REGION_CPU2, 0 )				/* Region 2 - sound cpu */
	ROM_LOAD( "b08_12.3e", 0x00000, 0x08000, CRC(dc84e27d) SHA1(d549d8c9fbec0521517f0c5f5cee763e27d48633) )
	ROM_CONTINUE(          0x10000, 0x08000 )			/* banked at 8000-9fff */

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* M-Chip (i8742 internal ROM) */
	ROM_LOAD( "ark28742.bin", 0x0000, 0x0800, NO_DUMP )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b08-01.13a",	0x00000, 0x20000, CRC(2ccc86b4) SHA1(eced1d7e687db0331507726946b6a19a690a7604) )
	ROM_LOAD( "b08-02.10a",	0x20000, 0x20000, CRC(056a985f) SHA1(6333b71c631d3307929aae633760870451830e10) )
	ROM_LOAD( "b08-03.7a",	0x40000, 0x20000, CRC(274a795f) SHA1(49353590e1a418843f57c715185e407a20021936) )
	ROM_LOAD( "b08-04.4a",	0x60000, 0x20000, CRC(9754f703) SHA1(0018ebf7da3f501345f3f5085d98d7614f8ce1b6) )

	ROM_REGION( 0x0400, REGION_PROMS, 0 )
	ROM_LOAD( "b08-08.15f",	0x00000, 0x200, CRC(a4f7ebd9) SHA1(094eb63c18898c6ee8d722492bdfd28091c61773) )	/* hi bytes */
	ROM_LOAD( "b08-07.16f",	0x00200, 0x200, CRC(ea34d9f7) SHA1(9a46edc64f961bd96908419cabd92445d300fc19) )	/* lo bytes */
ROM_END

ROM_START( arknid2j )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )				/* Region 0 - main cpu */
	ROM_LOAD( "b08_05.11c", 0x00000, 0x08000, CRC(136edf9d) SHA1(f632321650897eee585511a84f451a205d1f7704) )
	ROM_CONTINUE(           0x18000, 0x08000 )			/* banked at 8000-bfff */
	/* 20000-2ffff empty */

	ROM_REGION( 0x18000, REGION_CPU2, 0 )				/* Region 2 - sound cpu */
	ROM_LOAD( "b08_06.3e", 0x00000, 0x08000, CRC(adfcd40c) SHA1(f91299407ed21e2dd244c9b1a315b27ed32f5514) )
	ROM_CONTINUE(          0x10000, 0x08000 )			/* banked at 8000-9fff */

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* M-Chip (i8742 internal ROM) */
	ROM_LOAD( "ark28742.bin", 0x0000, 0x0800, NO_DUMP )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b08-01.13a",	0x00000, 0x20000, CRC(2ccc86b4) SHA1(eced1d7e687db0331507726946b6a19a690a7604) )
	ROM_LOAD( "b08-02.10a",	0x20000, 0x20000, CRC(056a985f) SHA1(6333b71c631d3307929aae633760870451830e10) )
	ROM_LOAD( "b08-03.7a",	0x40000, 0x20000, CRC(274a795f) SHA1(49353590e1a418843f57c715185e407a20021936) )
	ROM_LOAD( "b08-04.4a",	0x60000, 0x20000, CRC(9754f703) SHA1(0018ebf7da3f501345f3f5085d98d7614f8ce1b6) )

	ROM_REGION( 0x0400, REGION_PROMS, 0 )
	ROM_LOAD( "b08-08.15f",	0x00000, 0x200, CRC(a4f7ebd9) SHA1(094eb63c18898c6ee8d722492bdfd28091c61773) )	/* hi bytes */
	ROM_LOAD( "b08-07.16f",	0x00200, 0x200, CRC(ea34d9f7) SHA1(9a46edc64f961bd96908419cabd92445d300fc19) )	/* lo bytes */
ROM_END

ROM_START( drtoppel )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )	/* 64k + bankswitch areas for the first CPU */
	ROM_LOAD( "b19-09.bin", 0x00000, 0x08000, CRC(3e654f82) SHA1(d9e351d82546b08eb7887ea1d976fa97a259db6e) )
	ROM_CONTINUE(           0x18000, 0x08000 )				/* banked at 8000-bfff */
	ROM_LOAD( "b19-10.bin", 0x20000, 0x10000, CRC(7e72fd25) SHA1(6035e4db75e6dc57b13bb6e92217d1c2d0ffdfd2) )	/* banked at 8000-bfff */

	ROM_REGION( 0x18000, REGION_CPU2, 0 )	/* 64k for the second CPU */
	ROM_LOAD( "b19-11w", 0x00000, 0x08000, CRC(37a0d3fb) SHA1(f65fb9382af5f5b09725c39b660c5138b3912f53) )
	ROM_CONTINUE(        0x10000, 0x08000 )		/* banked at 8000-9fff */

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* M-Chip (i8742 internal ROM) */
	ROM_LOAD( "drt8742.bin", 0x0000, 0x0800, NO_DUMP )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b19-01.bin", 0x00000, 0x20000, CRC(a7e8a0c1) SHA1(a2f017ae5b6472d4202f126d0247b3fe4b1321d1) )
	ROM_LOAD( "b19-02.bin", 0x20000, 0x20000, CRC(790ae654) SHA1(5fd6b89918e1539e00c918959b96d2a9394c8abe) )
	ROM_LOAD( "b19-03.bin", 0x40000, 0x20000, CRC(495c4c5a) SHA1(a23b512cda4c0c535df5508a52faebe401c1797d) )
	ROM_LOAD( "b19-04.bin", 0x60000, 0x20000, CRC(647007a0) SHA1(10ec35a15091967038bb26fb116c47d730f69edc) )
	ROM_LOAD( "b19-05.bin", 0x80000, 0x20000, CRC(49f2b1a5) SHA1(5e98bb421afaa02471ad02213ea6ca23ff2f0e27) )
	ROM_LOAD( "b19-06.bin", 0xa0000, 0x20000, CRC(2d39f1d0) SHA1(2aa89a5cc7f026c8db9922b183319ff66ac4a071) )
	ROM_LOAD( "b19-07.bin", 0xc0000, 0x20000, CRC(8bb06f41) SHA1(a0c182d473317f2cdb31bdf39a2593c032002305) )
	ROM_LOAD( "b19-08.bin", 0xe0000, 0x20000, CRC(3584b491) SHA1(d0aca90708be241bbd3a1097220a85083337a4bc) )

	ROM_REGION( 0x0400, REGION_PROMS, 0 )		/* color proms */
	ROM_LOAD( "b19-13.bin", 0x0000, 0x200, CRC(6a547980) SHA1(c82f8dfad028565b4b4e5be1167f2f290c929090) )	/* hi bytes */
	ROM_LOAD( "b19-12.bin", 0x0200, 0x200, CRC(5754e9d8) SHA1(8c7d29e22c90b1f72929b95675dc15e431aae044) )	/* lo bytes */
ROM_END

ROM_START( drtopplu )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )	/* 64k + bankswitch areas for the first CPU */
	ROM_LOAD( "b19-09.bin", 0x00000, 0x08000, CRC(3e654f82) SHA1(d9e351d82546b08eb7887ea1d976fa97a259db6e) )
	ROM_CONTINUE(           0x18000, 0x08000 )				/* banked at 8000-bfff */
	ROM_LOAD( "b19-10.bin", 0x20000, 0x10000, CRC(7e72fd25) SHA1(6035e4db75e6dc57b13bb6e92217d1c2d0ffdfd2) )	/* banked at 8000-bfff */

	ROM_REGION( 0x18000, REGION_CPU2, 0 )	/* 64k for the second CPU */
	ROM_LOAD( "b19-11u", 0x00000, 0x08000, CRC(05565b22) SHA1(d1aa47b438d3b44c5177337809e38b50f6445c36) )
	ROM_CONTINUE(        0x10000, 0x08000 )		/* banked at 8000-9fff */

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* M-Chip (i8742 internal ROM) */
	ROM_LOAD( "drt8742.bin", 0x0000, 0x0800, NO_DUMP )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b19-01.bin", 0x00000, 0x20000, CRC(a7e8a0c1) SHA1(a2f017ae5b6472d4202f126d0247b3fe4b1321d1) )
	ROM_LOAD( "b19-02.bin", 0x20000, 0x20000, CRC(790ae654) SHA1(5fd6b89918e1539e00c918959b96d2a9394c8abe) )
	ROM_LOAD( "b19-03.bin", 0x40000, 0x20000, CRC(495c4c5a) SHA1(a23b512cda4c0c535df5508a52faebe401c1797d) )
	ROM_LOAD( "b19-04.bin", 0x60000, 0x20000, CRC(647007a0) SHA1(10ec35a15091967038bb26fb116c47d730f69edc) )
	ROM_LOAD( "b19-05.bin", 0x80000, 0x20000, CRC(49f2b1a5) SHA1(5e98bb421afaa02471ad02213ea6ca23ff2f0e27) )
	ROM_LOAD( "b19-06.bin", 0xa0000, 0x20000, CRC(2d39f1d0) SHA1(2aa89a5cc7f026c8db9922b183319ff66ac4a071) )
	ROM_LOAD( "b19-07.bin", 0xc0000, 0x20000, CRC(8bb06f41) SHA1(a0c182d473317f2cdb31bdf39a2593c032002305) )
	ROM_LOAD( "b19-08.bin", 0xe0000, 0x20000, CRC(3584b491) SHA1(d0aca90708be241bbd3a1097220a85083337a4bc) )

	ROM_REGION( 0x0400, REGION_PROMS, 0 )		/* color proms */
	ROM_LOAD( "b19-13.bin", 0x0000, 0x200, CRC(6a547980) SHA1(c82f8dfad028565b4b4e5be1167f2f290c929090) )	/* hi bytes */
	ROM_LOAD( "b19-12.bin", 0x0200, 0x200, CRC(5754e9d8) SHA1(8c7d29e22c90b1f72929b95675dc15e431aae044) )	/* lo bytes */
ROM_END

ROM_START( drtopplj )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )	/* 64k + bankswitch areas for the first CPU */
	ROM_LOAD( "b19-09.bin", 0x00000, 0x08000, CRC(3e654f82) SHA1(d9e351d82546b08eb7887ea1d976fa97a259db6e) )
	ROM_CONTINUE(           0x18000, 0x08000 )				/* banked at 8000-bfff */
	ROM_LOAD( "b19-10.bin", 0x20000, 0x10000, CRC(7e72fd25) SHA1(6035e4db75e6dc57b13bb6e92217d1c2d0ffdfd2) )	/* banked at 8000-bfff */

	ROM_REGION( 0x18000, REGION_CPU2, 0 )	/* 64k for the second CPU */
	ROM_LOAD( "b19-11j", 0x00000, 0x08000, CRC(524dc249) SHA1(158b2de0fcd17ad16ba72bb24888122bf704e216) )
	ROM_CONTINUE(        0x10000, 0x08000 )		/* banked at 8000-9fff */

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* M-Chip (i8742 internal ROM) */
	ROM_LOAD( "drt8742.bin", 0x0000, 0x0800, NO_DUMP )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b19-01.bin", 0x00000, 0x20000, CRC(a7e8a0c1) SHA1(a2f017ae5b6472d4202f126d0247b3fe4b1321d1) )
	ROM_LOAD( "b19-02.bin", 0x20000, 0x20000, CRC(790ae654) SHA1(5fd6b89918e1539e00c918959b96d2a9394c8abe) )
	ROM_LOAD( "b19-03.bin", 0x40000, 0x20000, CRC(495c4c5a) SHA1(a23b512cda4c0c535df5508a52faebe401c1797d) )
	ROM_LOAD( "b19-04.bin", 0x60000, 0x20000, CRC(647007a0) SHA1(10ec35a15091967038bb26fb116c47d730f69edc) )
	ROM_LOAD( "b19-05.bin", 0x80000, 0x20000, CRC(49f2b1a5) SHA1(5e98bb421afaa02471ad02213ea6ca23ff2f0e27) )
	ROM_LOAD( "b19-06.bin", 0xa0000, 0x20000, CRC(2d39f1d0) SHA1(2aa89a5cc7f026c8db9922b183319ff66ac4a071) )
	ROM_LOAD( "b19-07.bin", 0xc0000, 0x20000, CRC(8bb06f41) SHA1(a0c182d473317f2cdb31bdf39a2593c032002305) )
	ROM_LOAD( "b19-08.bin", 0xe0000, 0x20000, CRC(3584b491) SHA1(d0aca90708be241bbd3a1097220a85083337a4bc) )

	ROM_REGION( 0x0400, REGION_PROMS, 0 )		/* color proms */
	ROM_LOAD( "b19-13.bin", 0x0000, 0x200, CRC(6a547980) SHA1(c82f8dfad028565b4b4e5be1167f2f290c929090) )	/* hi bytes */
	ROM_LOAD( "b19-12.bin", 0x0200, 0x200, CRC(5754e9d8) SHA1(8c7d29e22c90b1f72929b95675dc15e431aae044) )	/* lo bytes */
ROM_END

ROM_START( kageki )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )
	ROM_LOAD( "b35-16.11c", 0x00000, 0x08000, CRC(a4e6fd58) SHA1(7cfe5b3fa6c88cdab45719f5b58541270825ad30) )	/* US ver */
	ROM_CONTINUE(           0x18000, 0x08000 )
	ROM_LOAD( "b35-10.9c",  0x20000, 0x10000, CRC(b150457d) SHA1(a58e46e7dfdc93c2cc7c04d623d7754f85ba693b) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 )
	ROM_LOAD( "b35-17.43e", 0x00000, 0x08000, CRC(fdd9c246) SHA1(ac7a59ed19d0d81748cabd8b77a6ba3937e3cc99) )	/* US ver */
	ROM_CONTINUE(           0x10000, 0x08000 )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b35-01.13a",  0x00000, 0x20000, CRC(01d83a69) SHA1(92a84329306b58a45f7bb443a8642eeaeb04d553) )
	ROM_LOAD( "b35-02.12a",  0x20000, 0x20000, CRC(d8af47ac) SHA1(2ef9ca991bf55ed6c12bf3a7dc4aa904d7749d5c) )
	ROM_LOAD( "b35-03.10a",  0x40000, 0x20000, CRC(3cb68797) SHA1(e7669b1a9a26dede560cc87695004d29510bc1f5) )
	ROM_LOAD( "b35-04.8a",   0x60000, 0x20000, CRC(71c03f91) SHA1(edce6e5a52b0c83c1c3c6bf9bc6b7957f7941521) )
	ROM_LOAD( "b35-05.7a",   0x80000, 0x20000, CRC(a4e20c08) SHA1(5d1d23d1410fea8650b18c595b0170a17e5d89a6) )
	ROM_LOAD( "b35-06.5a",   0xa0000, 0x20000, CRC(3f8ab658) SHA1(44de7ee2bdb89bc520ed9bc812c26789c3f31411) )
	ROM_LOAD( "b35-07.4a",   0xc0000, 0x20000, CRC(1b4af049) SHA1(09783816d5076219d241538e2711402eb8c4cd03) )
	ROM_LOAD( "b35-08.2a",   0xe0000, 0x20000, CRC(deb2268c) SHA1(318bf3da6cbe20758397d5f78caf3cda02f322d7) )

	ROM_REGION( 0x10000, REGION_SOUND1, 0 )	/* samples */
	ROM_LOAD( "b35-15.98g",  0x00000, 0x10000, CRC(e6212a0f) SHA1(43891f4fd141b00ed458be47a107a2550a0534c2) )	/* US ver */
ROM_END

ROM_START( kagekij )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )
	ROM_LOAD( "b35-09j.11c", 0x00000, 0x08000, CRC(829637d5) SHA1(0239ae925968336a90cbe16e23519773b6f2f2ac) )	/* JP ver */
	ROM_CONTINUE(            0x18000, 0x08000 )
	ROM_LOAD( "b35-10.9c",   0x20000, 0x10000, CRC(b150457d) SHA1(a58e46e7dfdc93c2cc7c04d623d7754f85ba693b) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 )
	ROM_LOAD( "b35-11j.43e", 0x00000, 0x08000, CRC(64d093fc) SHA1(3ca3f69d8946c453c0edb8586b92e2948a2d0b6c) )	/* JP ver */
	ROM_CONTINUE(            0x10000, 0x08000 )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b35-01.13a",  0x00000, 0x20000, CRC(01d83a69) SHA1(92a84329306b58a45f7bb443a8642eeaeb04d553) )
	ROM_LOAD( "b35-02.12a",  0x20000, 0x20000, CRC(d8af47ac) SHA1(2ef9ca991bf55ed6c12bf3a7dc4aa904d7749d5c) )
	ROM_LOAD( "b35-03.10a",  0x40000, 0x20000, CRC(3cb68797) SHA1(e7669b1a9a26dede560cc87695004d29510bc1f5) )
	ROM_LOAD( "b35-04.8a",   0x60000, 0x20000, CRC(71c03f91) SHA1(edce6e5a52b0c83c1c3c6bf9bc6b7957f7941521) )
	ROM_LOAD( "b35-05.7a",   0x80000, 0x20000, CRC(a4e20c08) SHA1(5d1d23d1410fea8650b18c595b0170a17e5d89a6) )
	ROM_LOAD( "b35-06.5a",   0xa0000, 0x20000, CRC(3f8ab658) SHA1(44de7ee2bdb89bc520ed9bc812c26789c3f31411) )
	ROM_LOAD( "b35-07.4a",   0xc0000, 0x20000, CRC(1b4af049) SHA1(09783816d5076219d241538e2711402eb8c4cd03) )
	ROM_LOAD( "b35-08.2a",   0xe0000, 0x20000, CRC(deb2268c) SHA1(318bf3da6cbe20758397d5f78caf3cda02f322d7) )

	ROM_REGION( 0x10000, REGION_SOUND1, 0 )	/* samples */
	ROM_LOAD( "b35-12j.98g", 0x00000, 0x10000, CRC(184409f1) SHA1(711bdd499670e86630ebb6820262b1d8d651c987) )	/* JP ver */
ROM_END

ROM_START( chukatai )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )	/* 64k + bankswitch areas for the first CPU */
	ROM_LOAD( "b44-10", 0x00000, 0x08000, CRC(8c69e008) SHA1(7825965f517f3562a508345b7c0d32b8a57bd38a) )
	ROM_CONTINUE(       0x18000, 0x08000 )				/* banked at 8000-bfff */
	ROM_LOAD( "b44-11", 0x20000, 0x10000, CRC(32484094) SHA1(f320fea2910816b5085ca9aa37e30af665fb6be1) )	/* banked at 8000-bfff */

	ROM_REGION( 0x18000, REGION_CPU2, 0 )	/* 64k for the second CPU */
	ROM_LOAD( "b44-12w", 0x00000, 0x08000, CRC(e80ecdca) SHA1(cd96403ca97f18f630118dcb3dc2179c01147213) )
	ROM_CONTINUE(        0x10000, 0x08000 )		/* banked at 8000-9fff */

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* M-Chip (i8742 internal ROM) */
	ROM_LOAD( "b44-8742.mcu", 0x0000, 0x0800, CRC(7dff3f9f) SHA1(bbf4e036d025fe8179b053d639f9b8ad401e6e68) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b44-01.a13", 0x00000, 0x20000, CRC(aae7b3d5) SHA1(52809ea22d98811ece2fb27e80db6ddf4fbacb07) )
	ROM_LOAD( "b44-02.a12", 0x20000, 0x20000, CRC(7f0b9568) SHA1(415d2638d1b0eb36b2e2f63219cbc0dbebe02dc6) )
	ROM_LOAD( "b44-03.a10", 0x40000, 0x20000, CRC(5a54a3b9) SHA1(6b219f1c3570f16eb4a06221d7e527c735437bac) )
	ROM_LOAD( "b44-04.a08", 0x60000, 0x20000, CRC(3c5f544b) SHA1(d3b0ee18f1027483a36ef02757b62f42a086a8e2) )
	ROM_LOAD( "b44-05.a07", 0x80000, 0x20000, CRC(d1b7e314) SHA1(8b4181caa32955b4274614a4238bb24d67ecb729) )
	ROM_LOAD( "b44-06.a05", 0xa0000, 0x20000, CRC(269978a8) SHA1(aef7b8d3d00dcc4201e0a1e28026f6f1bdafd0b7) )
	ROM_LOAD( "b44-07.a04", 0xc0000, 0x20000, CRC(3e0e737e) SHA1(f8d62c7b69c79da9df7ef5ce454060d3645e5884) )
	ROM_LOAD( "b44-08.a02", 0xe0000, 0x20000, CRC(6cb1e8fc) SHA1(4ab0c2cce1de2616044a9bfb9bf17f95a49baffd) )
ROM_END

ROM_START( chukatau )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )	/* 64k + bankswitch areas for the first CPU */
	ROM_LOAD( "b44-10", 0x00000, 0x08000, CRC(8c69e008) SHA1(7825965f517f3562a508345b7c0d32b8a57bd38a) )
	ROM_CONTINUE(       0x18000, 0x08000 )				/* banked at 8000-bfff */
	ROM_LOAD( "b44-11", 0x20000, 0x10000, CRC(32484094) SHA1(f320fea2910816b5085ca9aa37e30af665fb6be1) )  /* banked at 8000-bfff */

	ROM_REGION( 0x18000, REGION_CPU2, 0 )	/* 64k for the second CPU */
	ROM_LOAD( "b44-12u", 0x00000, 0x08000, CRC(9f09fd5c) SHA1(ae92f2e893e1e666dcabbd793f1a778c5e3d7bab) )
	ROM_CONTINUE(        0x10000, 0x08000 )		/* banked at 8000-9fff */

	ROM_REGION( 0x1000, REGION_CPU3, 0 )	/* M-Chip (i8742 internal ROM) */
	ROM_LOAD( "b44-8742.mcu", 0x0000, 0x0800, CRC(7dff3f9f) SHA1(bbf4e036d025fe8179b053d639f9b8ad401e6e68) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b44-01.a13", 0x00000, 0x20000, CRC(aae7b3d5) SHA1(52809ea22d98811ece2fb27e80db6ddf4fbacb07) )
	ROM_LOAD( "b44-02.a12", 0x20000, 0x20000, CRC(7f0b9568) SHA1(415d2638d1b0eb36b2e2f63219cbc0dbebe02dc6) )
	ROM_LOAD( "b44-03.a10", 0x40000, 0x20000, CRC(5a54a3b9) SHA1(6b219f1c3570f16eb4a06221d7e527c735437bac) )
	ROM_LOAD( "b44-04.a08", 0x60000, 0x20000, CRC(3c5f544b) SHA1(d3b0ee18f1027483a36ef02757b62f42a086a8e2) )
	ROM_LOAD( "b44-05.a07", 0x80000, 0x20000, CRC(d1b7e314) SHA1(8b4181caa32955b4274614a4238bb24d67ecb729) )
	ROM_LOAD( "b44-06.a05", 0xa0000, 0x20000, CRC(269978a8) SHA1(aef7b8d3d00dcc4201e0a1e28026f6f1bdafd0b7) )
	ROM_LOAD( "b44-07.a04", 0xc0000, 0x20000, CRC(3e0e737e) SHA1(f8d62c7b69c79da9df7ef5ce454060d3645e5884) )
	ROM_LOAD( "b44-08.a02", 0xe0000, 0x20000, CRC(6cb1e8fc) SHA1(4ab0c2cce1de2616044a9bfb9bf17f95a49baffd) )
ROM_END

ROM_START( chukataj )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )	/* 64k + bankswitch areas for the first CPU */
	ROM_LOAD( "b44-10", 0x00000, 0x08000, CRC(8c69e008) SHA1(7825965f517f3562a508345b7c0d32b8a57bd38a) )
	ROM_CONTINUE(       0x18000, 0x08000 )				/* banked at 8000-bfff */
	ROM_LOAD( "b44-11", 0x20000, 0x10000, CRC(32484094) SHA1(f320fea2910816b5085ca9aa37e30af665fb6be1) )  /* banked at 8000-bfff */

	ROM_REGION( 0x18000, REGION_CPU2, 0 )	/* 64k for the second CPU */
	ROM_LOAD( "b44-12j", 0x00000, 0x08000, CRC(0600ace6) SHA1(3d5767b91ea63128bfbff3527ddcf90fcf43af2e) )
	ROM_CONTINUE(        0x10000, 0x08000 )		/* banked at 8000-9fff */

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* M-Chip (i8742 internal ROM) */
	ROM_LOAD( "b44-8742.mcu", 0x0000, 0x0800, CRC(7dff3f9f) SHA1(bbf4e036d025fe8179b053d639f9b8ad401e6e68) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b44-01.a13", 0x00000, 0x20000, CRC(aae7b3d5) SHA1(52809ea22d98811ece2fb27e80db6ddf4fbacb07) )
	ROM_LOAD( "b44-02.a12", 0x20000, 0x20000, CRC(7f0b9568) SHA1(415d2638d1b0eb36b2e2f63219cbc0dbebe02dc6) )
	ROM_LOAD( "b44-03.a10", 0x40000, 0x20000, CRC(5a54a3b9) SHA1(6b219f1c3570f16eb4a06221d7e527c735437bac) )
	ROM_LOAD( "b44-04.a08", 0x60000, 0x20000, CRC(3c5f544b) SHA1(d3b0ee18f1027483a36ef02757b62f42a086a8e2) )
	ROM_LOAD( "b44-05.a07", 0x80000, 0x20000, CRC(d1b7e314) SHA1(8b4181caa32955b4274614a4238bb24d67ecb729) )
	ROM_LOAD( "b44-06.a05", 0xa0000, 0x20000, CRC(269978a8) SHA1(aef7b8d3d00dcc4201e0a1e28026f6f1bdafd0b7) )
	ROM_LOAD( "b44-07.a04", 0xc0000, 0x20000, CRC(3e0e737e) SHA1(f8d62c7b69c79da9df7ef5ce454060d3645e5884) )
	ROM_LOAD( "b44-08.a02", 0xe0000, 0x20000, CRC(6cb1e8fc) SHA1(4ab0c2cce1de2616044a9bfb9bf17f95a49baffd) )
ROM_END

ROM_START( tnzs )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )	/* 64k + bankswitch areas for the first CPU */
	ROM_LOAD( "b53_10.32", 0x00000, 0x08000, CRC(a73745c6) SHA1(73eb38e75e08312d752332f988dc655084b4a86d) )
	ROM_CONTINUE(          0x18000, 0x18000 )		/* banked at 8000-bfff */

	ROM_REGION( 0x18000, REGION_CPU2, 0 )	/* 64k for the second CPU */
	ROM_LOAD( "b53_11.38", 0x00000, 0x08000, CRC(9784d443) SHA1(bc3647aac9974031dbe4898417fbaa99841f9548) )
	ROM_CONTINUE(          0x10000, 0x08000 )		/* banked at 8000-9fff */

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* M-Chip (i8742 internal ROM) */
	ROM_LOAD( "tnzs8742.u46", 0x0000, 0x0800, CRC(a4bfce19) SHA1(9340862d5bdc1ad4799dc92cae9bce1428b47478) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	/* ROMs taken from another set (the ones from this set were read incorrectly) */
	ROM_LOAD( "b53-08.8",	0x00000, 0x20000, CRC(c3519c2a) SHA1(30fe7946fbc95ab6b3ccb6944fb24bf47bf3d743) )
	ROM_LOAD( "b53-07.7",	0x20000, 0x20000, CRC(2bf199e8) SHA1(4ed73e4f00ae2f5f4028a0ea5ae3cd238863a370) )
	ROM_LOAD( "b53-06.6",	0x40000, 0x20000, CRC(92f35ed9) SHA1(5fdd8d6ddbb7be9887af3c8dea9ad3b58c4e86f9) )
	ROM_LOAD( "b53-05.5",	0x60000, 0x20000, CRC(edbb9581) SHA1(539396a01ca0b69455f000d446759b232530b542) )
	ROM_LOAD( "b53-04.4",	0x80000, 0x20000, CRC(59d2aef6) SHA1(b657b7603c3eb5f169000d38497ebb93f26f7832) )
	ROM_LOAD( "b53-03.3",	0xa0000, 0x20000, CRC(74acfb9b) SHA1(90b544ed7ede7565660bdd13c94c15c54423cda9) )
	ROM_LOAD( "b53-02.2",	0xc0000, 0x20000, CRC(095d0dc0) SHA1(ced2937d0594fa00ae344a4e3a3cba23772dc160) )
	ROM_LOAD( "b53-01.1",	0xe0000, 0x20000, CRC(9800c54d) SHA1(761647177d621ac2cdd8b009876eed35809f3c92) )
ROM_END

ROM_START( tnzsb )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )	/* 64k + bankswitch areas for the first CPU */
	ROM_LOAD( "nzsb5324.bin", 0x00000, 0x08000, CRC(d66824c6) SHA1(fd381ac0dc52ce670c3fde320ea60a209e288a52) )
	ROM_CONTINUE(             0x18000, 0x18000 )		/* banked at 8000-bfff */

	ROM_REGION( 0x18000, REGION_CPU2, 0 )	/* 64k for the second CPU */
	ROM_LOAD( "nzsb5325.bin", 0x00000, 0x08000, CRC(d6ac4e71) SHA1(f3e71624a8a5e4e4c8a6aa01711ed26bdd5abf5a) )
	ROM_CONTINUE(             0x10000, 0x08000 )		/* banked at 8000-9fff */

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* 64k for the third CPU */
	ROM_LOAD( "nzsb5326.bin", 0x00000, 0x10000, CRC(cfd5649c) SHA1(4f6afccd535d39b41661dc3ccd17af125bfac015) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	/* ROMs taken from another set (the ones from this set were read incorrectly) */
	ROM_LOAD( "b53-08.8",	0x00000, 0x20000, CRC(c3519c2a) SHA1(30fe7946fbc95ab6b3ccb6944fb24bf47bf3d743) )
	ROM_LOAD( "b53-07.7",	0x20000, 0x20000, CRC(2bf199e8) SHA1(4ed73e4f00ae2f5f4028a0ea5ae3cd238863a370) )
	ROM_LOAD( "b53-06.6",	0x40000, 0x20000, CRC(92f35ed9) SHA1(5fdd8d6ddbb7be9887af3c8dea9ad3b58c4e86f9) )
	ROM_LOAD( "b53-05.5",	0x60000, 0x20000, CRC(edbb9581) SHA1(539396a01ca0b69455f000d446759b232530b542) )
	ROM_LOAD( "b53-04.4",	0x80000, 0x20000, CRC(59d2aef6) SHA1(b657b7603c3eb5f169000d38497ebb93f26f7832) )
	ROM_LOAD( "b53-03.3",	0xa0000, 0x20000, CRC(74acfb9b) SHA1(90b544ed7ede7565660bdd13c94c15c54423cda9) )
	ROM_LOAD( "b53-02.2",	0xc0000, 0x20000, CRC(095d0dc0) SHA1(ced2937d0594fa00ae344a4e3a3cba23772dc160) )
	ROM_LOAD( "b53-01.1",	0xe0000, 0x20000, CRC(9800c54d) SHA1(761647177d621ac2cdd8b009876eed35809f3c92) )
ROM_END

ROM_START( tnzs2 )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )	/* 64k + bankswitch areas for the first CPU */
	ROM_LOAD( "ns_c-11.rom", 0x00000, 0x08000, CRC(3c1dae7b) SHA1(0004fccc171714c80565326f8690f9662c5b75d9) )
	ROM_CONTINUE(            0x18000, 0x18000 )		/* banked at 8000-bfff */

	ROM_REGION( 0x18000, REGION_CPU2, 0 )	/* 64k for the second CPU */
	ROM_LOAD( "ns_e-3.rom", 0x00000, 0x08000, CRC(c7662e96) SHA1(be28298bfde4e3867cfe75633ffb0f8611dbbd8b) )
	ROM_CONTINUE(           0x10000, 0x08000 )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* M-Chip (i8742 internal ROM) */
	ROM_LOAD( "tnzs8742.u46", 0x0000, 0x0800, CRC(a4bfce19) SHA1(9340862d5bdc1ad4799dc92cae9bce1428b47478) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ns_a13.rom",   0x00000, 0x20000, CRC(7e0bd5bb) SHA1(95dfb00ec915778e02d8bfa996735ab817191adc) )
	ROM_LOAD( "ns_a12.rom",   0x20000, 0x20000, CRC(95880726) SHA1(f4fdedd23e80a6ccf32f737ab4bc57f9fc0925be) )
	ROM_LOAD( "ns_a10.rom",   0x40000, 0x20000, CRC(2bc4c053) SHA1(cd7668a7733e5e80c2c566d0cf63c4310e5743b4) )
	ROM_LOAD( "ns_a08.rom",   0x60000, 0x20000, CRC(8ff8d88c) SHA1(31977e39ad048a077e9b5bd712ff66b14a466d27) )
	ROM_LOAD( "ns_a07.rom",   0x80000, 0x20000, CRC(291bcaca) SHA1(4f659a0cd2ff6b4ec04ab95ee8a670222c402c2b) )
	ROM_LOAD( "ns_a05.rom",   0xa0000, 0x20000, CRC(6e762e20) SHA1(66731fe4053b9c09bc9c95d10aba212db08b4636) )
	ROM_LOAD( "ns_a04.rom",   0xc0000, 0x20000, CRC(e1fd1b9d) SHA1(6027491b927c2ab9c77fbf8895da1abcfbe32d62) )
	ROM_LOAD( "ns_a02.rom",   0xe0000, 0x20000, CRC(2ab06bda) SHA1(2b208b564e55c258665e1f66b26fe14a6c68eb96) )
ROM_END

ROM_START( kabukiz )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )	/* 64k + bankswitch areas for the first CPU */
	ROM_LOAD( "b50_05.u1",  0x00000, 0x08000, CRC(9cccb129) SHA1(054faf7657bad7237182e36bcc4388b1748af935) )
	ROM_CONTINUE(           0x18000, 0x18000 )		/* banked at 8000-bfff */

	ROM_REGION( 0x18000, REGION_CPU2, 0 )	/* 64k for the second CPU */
	ROM_LOAD( "b50-08.1e",  0x00000, 0x08000, CRC(cb92d34c) SHA1(3a666f0e3ff9d3daa599123edee228d94eeae754) )
	ROM_CONTINUE(           0x10000, 0x08000 )		/* banked at 8000-9fff */

	ROM_REGION( 0x30000, REGION_CPU3, 0 )	/* 64k + bankswitch areas for the third CPU */
	ROM_LOAD( "b50_07.u34", 0x00000, 0x08000, CRC(bf7fc2ed) SHA1(77008d12d9bdbfa100dcd87cd6ca7de3748408c5) )
	ROM_CONTINUE(           0x18000, 0x18000 )		/* banked at 8000-bfff */
	
	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b50-04.u35", 0x000000, 0x80000, CRC(04829aa9) SHA1(a501ec7c802478fc41ec8ef4270b1a6872bcbf34) )
	ROM_LOAD( "b50-03.u39", 0x080000, 0x80000, CRC(31489a4c) SHA1(a4b7e00e2074287b47c7e16add963c1470534376) )
	ROM_LOAD( "b50-02.u43", 0x100000, 0x80000, CRC(90b8a8e7) SHA1(a55e327307606142fbb9d500e757655b35e1f252) )
	ROM_LOAD( "b50-01.u46", 0x180000, 0x80000, CRC(f4277751) SHA1(8f50f843f0eda30d639ba397889236ff0a3edce5) )
ROM_END

ROM_START( kabukizj )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )	/* 64k + bankswitch areas for the first CPU */
	ROM_LOAD( "b50_05.u1",  0x00000, 0x08000, CRC(9cccb129) SHA1(054faf7657bad7237182e36bcc4388b1748af935) )
	ROM_CONTINUE(           0x18000, 0x18000 )		/* banked at 8000-bfff */

	ROM_REGION( 0x18000, REGION_CPU2, 0 )	/* 64k for the second CPU */
	ROM_LOAD( "b50_06.u3",  0x00000, 0x08000, CRC(45650aab) SHA1(00d1fc6044a6ad1e82476ccbe730907b4d780cb9) )
	ROM_CONTINUE(           0x10000, 0x08000 )		/* banked at 8000-9fff */

	ROM_REGION( 0x30000, REGION_CPU3, 0 )	/* 64k + bankswitch areas for the third CPU */
	ROM_LOAD( "b50_07.u34", 0x00000, 0x08000, CRC(bf7fc2ed) SHA1(77008d12d9bdbfa100dcd87cd6ca7de3748408c5) )
	ROM_CONTINUE(           0x18000, 0x18000 )		/* banked at 8000-bfff */
	
	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b50-04.u35", 0x000000, 0x80000, CRC(04829aa9) SHA1(a501ec7c802478fc41ec8ef4270b1a6872bcbf34) )
	ROM_LOAD( "b50-03.u39", 0x080000, 0x80000, CRC(31489a4c) SHA1(a4b7e00e2074287b47c7e16add963c1470534376) )
	ROM_LOAD( "b50-02.u43", 0x100000, 0x80000, CRC(90b8a8e7) SHA1(a55e327307606142fbb9d500e757655b35e1f252) )
	ROM_LOAD( "b50-01.u46", 0x180000, 0x80000, CRC(f4277751) SHA1(8f50f843f0eda30d639ba397889236ff0a3edce5) )
ROM_END

ROM_START( insectx )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )	/* 64k + bankswitch areas for the first CPU */
	ROM_LOAD( "insector.u32", 0x00000, 0x08000, CRC(18eef387) SHA1(b22633930d39be1e72fbd5b080972122da3cb3ef) )
	ROM_CONTINUE(             0x18000, 0x18000 )		/* banked at 8000-bfff */

	ROM_REGION( 0x18000, REGION_CPU2, 0 )	/* 64k for the second CPU */
	ROM_LOAD( "insector.u38", 0x00000, 0x08000, CRC(324b28c9) SHA1(db77a4ac60196d0f0f35dbc5c951ec29d6392463) )
	ROM_CONTINUE(             0x10000, 0x08000 )		/* banked at 8000-9fff */

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "insector.r15", 0x00000, 0x80000, CRC(d00294b1) SHA1(f43a4f7d13193ddbbcdef71a5085c1db0fc062d4) )
	ROM_LOAD( "insector.r16", 0x80000, 0x80000, CRC(db5a7434) SHA1(71fac872b19a13a7ad25c8ad895c322ec9573fdc) )
ROM_END


/*  ( YEAR  NAME      PARENT    MACHINE   INPUT     INIT      MONITOR COMPANY                                         FULLNAME                        FLAGS ) */
GAME( 1987, plumppop, 0,        drtoppel, plumppop, plumpop,  ROT0,   "Taito Corporation",                            "Plump Pop (Japan)" )
GAME( 1987, extrmatn, 0,        arknoid2, extrmatn, extrmatn, ROT270, "[Taito] World Games",                          "Extermination (US)" )
GAME( 1987, arknoid2, 0,        arknoid2, arknoid2, arknoid2, ROT270, "Taito Corporation Japan",                      "Arkanoid - Revenge of DOH (World)" )
GAME( 1987, arknid2u, arknoid2, arknoid2, arknid2u, arknoid2, ROT270, "Taito America Corporation (Romstar license)",  "Arkanoid - Revenge of DOH (US)" )
GAME( 1987, arknid2j, arknoid2, arknoid2, arknid2u, arknoid2, ROT270, "Taito Corporation",                            "Arkanoid - Revenge of DOH (Japan)" )
GAME( 1987, drtoppel, 0,        drtoppel, drtoppel, drtoppel, ROT90,  "Taito Corporation Japan",                      "Dr. Toppel's Adventure (World)" )
GAME( 1987, drtopplu, drtoppel, drtoppel, drtopplu, drtoppel, ROT90,  "Taito America Corporation",                    "Dr. Toppel's Adventure (US)" )
GAME( 1987, drtopplj, drtoppel, drtoppel, drtopplu, drtoppel, ROT90,  "Taito Corporation",                            "Dr. Toppel's Tankentai (Japan)" )
GAME( 1988, kageki,   0,        kageki,   kageki,   kageki,   ROT90,  "Taito America Corporation (Romstar license)",  "Kageki (US)" )
GAME( 1988, kagekij,  kageki,   kageki,   kageki,   kageki,   ROT90,  "Taito Corporation",                            "Kageki (Japan)" )
GAME( 1988, chukatai, 0,        tnzs,     chukatai, chukatai, ROT0,   "Taito Corporation Japan",                      "Chuka Taisen (World)" )
GAME( 1988, chukatau, chukatai, tnzs,     chukatau, chukatai, ROT0,   "Taito America Corporation",                    "Chuka Taisen (US)" )
GAME( 1988, chukataj, chukatai, tnzs,     chukatau, chukatai, ROT0,   "Taito Corporation",                            "Chuka Taisen (Japan)" )
GAME( 1988, kabukiz,  0,        kabukiz,  kabukiz,  kabukiz,  ROT0,   "Taito Corporation Japan",                      "Kabuki-Z (World)" )
GAME( 1988, kabukizj, kabukiz,  kabukiz,  kabukizj, kabukiz,  ROT0,   "Taito Corporation",                            "Kabuki-Z (Japan)" )
GAME( 1988, tnzs,     0,        tnzs,     tnzs,     tnzs,     ROT0,   "Taito Corporation",                            "The NewZealand Story (Japan, new version) (P0-043A PCB)" )
GAME( 1988, tnzsb,    tnzs,     tnzsb,    tnzsb,    tnzsb,    ROT0,   "Taito Corporation Japan",                      "The NewZealand Story (World, new version) (P0-043A PCB)" )
GAME( 1988, tnzs2,    tnzs,     tnzs,     tnzs2,    tnzs,     ROT0,   "Taito Corporation Japan",                      "The NewZealand Story (World, old version) (P0-041A PCB)" )
GAME( 1989, insectx,  0,        insectx,  insectx,  insectx,  ROT0,   "Taito Corporation Japan",                      "Insector X (World)" )
