/***************************************************************************

Vendetta (GX081) (c) 1991 Konami

Preliminary driver by:
Ernesto Corvi
someone@secureshell.com

Notes:
- collision detection is handled by a protection chip. Its emulation might
  not be 100% accurate.

****************************************************************************/

/*   Game driver for "ESCAPE KIDS (TM)"  (KONAMI, 1991)*/
/* --------------------------------------------------------------------------------*/

/*            This driver was made on the basis of 'src/drivers/vendetta.c' file.*/
/*                                         Driver by OHSAKI Masayuki (2002/08/13)*/
/**/
/* *********************************************************************************/


/* ***** NOTES ******/
/*      -------*/
/*  1) ESCAPE KIDS uses 053246's unknown function. (see vidhrdw/konamiic.c)*/
/*                   (053246 register #5  UnKnown Bit #5, #3, #2 always set "1")*/


/* ***** On the "error.log" ******/
/*      --------------------*/
/*  1) "YM2151 Write 00 to undocumented register #xx" (xx=00-1f)*/
/*                Why???*/

/*  2) "xxxx: read from unknown 052109 address yyyy"*/
/*  3) "xxxx: write zz to unknown 052109 address yyyy"*/
/*                These are vidhrdw/konamiic.c's message.*/
/*                "vidhrdw/konamiic.c" checks 052109 RAM area access.*/
/*                If accessed over 0x1800 (0x3800), logged 2) or 3) messages.*/
/*                Escape Kids use 0x1800-0x19ff and 0x3800-0x39ff area.*/


/* ***** UnEmulated ******/
/*      ------------*/
/*  1) 0x3fc0-0x3fcf (052109 RAM area) access (053252 ???)*/
/*  2) 0x7c00 (Banked ROM area) access to data WRITE (???)*/
/*  3) 0x3fda (053248 RAM area) access to data WRITE (Watchdog ???)*/


/* ***** ESCAPE KIDS PCB layout/ Need to dump ******/
/*      --------------------------------------*/
/*   (Parts side view)*/
/*   +-------------------------------------------------------+*/
/*   |   R          ROM9                               [CN1] |  CN1:Player4 Input?*/
/*   |   O                                             [CN2] |           (Labeled '4P')*/
/*   |   M          ROM8                       ROM1    [SW1] |  CN2:Player3 Input?*/
/*   |   7                              [CUS1]             +-+           (Labeled '3P')*/
/*   |        [CUS7]   [CUS8]                              +-+  CN3:Stereo sound out*/
/*   | R                                       [CUS2]        |*/
/*   | O                                                   J |  SW1:Test Switch*/
/*   | M                                                   A |*/
/*   | 6    [CUS6]                                         M | ***  Custom Chips  ****/
/*   |                                                     M |      CUS1: 053248*/
/*   | R                                                   A |      CUS2: 053252*/
/*   | O    [CUS5]                                        56P|      CUS3: 053260*/
/*   | M                                                     |      CUS4: 053246*/
/*   | 5                           ROM2  [ Z80 ]           +-+      CUS5: 053247*/
/*   |                                                     +-+      CUS6: 053251*/
/*   | R    [CUS4]                     [CUS3] [YM2151] [CN3] |      CUS7: 051962*/
/*   | O                                                     |      CUS8: 052109*/
/*   | M                                 ROM3                |*/
/*   | 4                                         [Sound AMP] |*/
/*   +-------------------------------------------------------+*/
/**/
/*  ***  Dump ROMs  ****/
/*     1) ROM1 (17C)  32Pin 1Mbit UV-EPROM          -> save "975r01" file*/
/*     2) ROM2 ( 5F)  28Pin 512Kbit One-Time PROM   -> save "975f02" file*/
/*     3) ROM3 ( 1D)  40Pin 4Mbit MASK ROM          -> save "975c03" file*/
/*     4) ROM4 ( 3K)  42Pin 8Mbit MASK ROM          -> save "975c04" file*/
/*     5) ROM5 ( 8L)  42Pin 8Mbit MASK ROM          -> save "975c05" file*/
/*     6) ROM6 (12M)  42Pin 8Mbit MASK ROM          -> save "975c06" file*/
/*     7) ROM7 (16K)  42Pin 8Mbit MASK ROM          -> save "975c07" file*/
/*     8) ROM8 (16I)  40Pin 4Mbit MASK ROM          -> save "975c08" file*/
/*     9) ROM9 (18I)  40Pin 4Mbit MASK ROM          -> save "975c09" file*/
/*                                                        vvvvvvvvvvvv*/
/*                                                        esckidsj.zip*/

/***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/konamiic.h"
#include "cpu/konami/konami.h" /* for the callback and the firq irq definition */
#include "machine/eeprom.h"

/* prototypes */
static MACHINE_INIT( vendetta );
static void vendetta_banking( int lines );
static void vendetta_video_banking( int select );

VIDEO_START( vendetta );
VIDEO_START( esckids );
VIDEO_UPDATE( vendetta );


/***************************************************************************

  EEPROM

***************************************************************************/

static int init_eeprom_count;


static struct EEPROM_interface eeprom_interface =
{
	7,				/* address bits */
	8,				/* data bits */
	"011000",		/*  read command */
	"011100",		/* write command */
	0,				/* erase command */
	"0100000000000",/* lock command */
	"0100110000000" /* unlock command */
};

static NVRAM_HANDLER( vendetta )
{
	if (read_or_write)
		EEPROM_save(file);
	else
	{
		EEPROM_init(&eeprom_interface);

		if (file)
		{
			init_eeprom_count = 0;
			EEPROM_load(file);
		}
		else
			init_eeprom_count = 1000;
	}
}

static READ_HANDLER( vendetta_eeprom_r )
{
	int res;

	res = EEPROM_read_bit();

	res |= 0x02;/* konami_eeprom_ack() << 5; add the ack */

	res |= readinputport( 3 ) & 0x0c; /* test switch */

	if (init_eeprom_count)
	{
		init_eeprom_count--;
		res &= 0xfb;
	}
	return res;
}

static int irq_enabled;

static WRITE_HANDLER( vendetta_eeprom_w )
{
	/* bit 0 - VOC0 - Video banking related */
	/* bit 1 - VOC1 - Video banking related */
	/* bit 2 - MSCHNG - Mono Sound select (Amp) */
	/* bit 3 - EEPCS - Eeprom CS */
	/* bit 4 - EEPCLK - Eeprom CLK */
	/* bit 5 - EEPDI - Eeprom data */
	/* bit 6 - IRQ enable */
	/* bit 7 - Unused */

	if ( data == 0xff ) /* this is a bug in the eeprom write code */
		return;

	/* EEPROM */
	EEPROM_write_bit(data & 0x20);
	EEPROM_set_clock_line((data & 0x10) ? ASSERT_LINE : CLEAR_LINE);
	EEPROM_set_cs_line((data & 0x08) ? CLEAR_LINE : ASSERT_LINE);

	irq_enabled = ( data >> 6 ) & 1;

	vendetta_video_banking( data & 1 );
}

/********************************************/

static READ_HANDLER( vendetta_K052109_r ) { return K052109_r( offset + 0x2000 ); }
/*static WRITE_HANDLER( vendetta_K052109_w ) { K052109_w( offset + 0x2000, data ); }*/
static WRITE_HANDLER( vendetta_K052109_w ) {
	/* **************************************************************************************/
	/* *  Escape Kids uses 052109's mirrored Tilemap ROM bank selector, but only during    **/
	/* *  Tilemap MASK-ROM Test       (0x1d80<->0x3d80, 0x1e00<->0x3e00, 0x1f00<->0x3f00)  **/
	/* **************************************************************************************/
	if ( ( offset == 0x1d80 ) || ( offset == 0x1e00 ) || ( offset == 0x1f00 ) )		K052109_w( offset, data );
	K052109_w( offset + 0x2000, data );
}

static offs_t video_banking_base;

static void vendetta_video_banking( int select )
{
	if ( select & 1 )
	{
		install_mem_read_handler(0,  video_banking_base + 0x2000, video_banking_base + 0x2fff,  paletteram_r );
		install_mem_write_handler(0, video_banking_base + 0x2000, video_banking_base + 0x2fff,  paletteram_xBBBBBGGGGGRRRRR_swap_w );
		install_mem_read_handler(0,  video_banking_base + 0x0000, video_banking_base + 0x0fff,  K053247_r );
		install_mem_write_handler(0, video_banking_base + 0x0000, video_banking_base + 0x0fff,  K053247_w );
	}
	else
	{
		install_mem_read_handler(0,  video_banking_base + 0x2000, video_banking_base + 0x2fff,  vendetta_K052109_r );
		install_mem_write_handler(0, video_banking_base + 0x2000, video_banking_base + 0x2fff,  vendetta_K052109_w );
		install_mem_read_handler(0,  video_banking_base + 0x0000, video_banking_base + 0x0fff,  K052109_r );
		install_mem_write_handler(0, video_banking_base + 0x0000, video_banking_base + 0x0fff,  K052109_w );
	}
}

static WRITE_HANDLER( vendetta_5fe0_w )
{
	/* bit 0,1 coin counters */
	coin_counter_w(0,data & 0x01);
	coin_counter_w(1,data & 0x02);

	/* bit 2 = BRAMBK ?? */

	/* bit 3 = enable char ROM reading through the video RAM */
	K052109_set_RMRD_line((data & 0x08) ? ASSERT_LINE : CLEAR_LINE);

	/* bit 4 = INIT ?? */

	/* bit 5 = enable sprite ROM reading */
	K053246_set_OBJCHA_line((data & 0x20) ? ASSERT_LINE : CLEAR_LINE);
}

static void z80_nmi_callback( int param )
{
	cpu_set_nmi_line( 1, ASSERT_LINE );
}

static WRITE_HANDLER( z80_arm_nmi_w )
{
	cpu_set_nmi_line( 1, CLEAR_LINE );

	timer_set( TIME_IN_USEC( 50 ), 0, z80_nmi_callback );
}

static WRITE_HANDLER( z80_irq_w )
{
	cpu_set_irq_line_and_vector( 1, 0, HOLD_LINE, 0xff );
}

READ_HANDLER( vendetta_sound_interrupt_r )
{
	cpu_set_irq_line_and_vector( 1, 0, HOLD_LINE, 0xff );
	return 0x00;
}

READ_HANDLER( vendetta_sound_r )
{
	/* If the sound CPU is running, read the status, otherwise
	   just make it pass the test */
	if (Machine->sample_rate != 0) 	return K053260_0_r(2 + offset);
	else
	{
		static int res = 0x00;

		res = ((res + 1) & 0x07);
		return offset ? res : 0x00;
	}
}

/********************************************/

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x1fff, MRA_BANK1	},
	{ 0x2000, 0x3fff, MRA_RAM },
	{ 0x5f80, 0x5f9f, K054000_r },
	{ 0x5fc0, 0x5fc0, input_port_0_r },
	{ 0x5fc1, 0x5fc1, input_port_1_r },
	{ 0x5fc2, 0x5fc2, input_port_4_r },
	{ 0x5fc3, 0x5fc3, input_port_5_r },
	{ 0x5fd0, 0x5fd0, vendetta_eeprom_r }, /* vblank, service */
	{ 0x5fd1, 0x5fd1, input_port_2_r },
	{ 0x5fe4, 0x5fe4, vendetta_sound_interrupt_r },
	{ 0x5fe6, 0x5fe7, vendetta_sound_r },
	{ 0x5fe8, 0x5fe9, K053246_r },
	{ 0x5fea, 0x5fea, watchdog_reset_r },
	{ 0x4000, 0x4fff, MRA_BANK3 },
	{ 0x6000, 0x6fff, MRA_BANK2 },
	{ 0x4000, 0x7fff, K052109_r },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x2000, 0x3fff, MWA_RAM },
	{ 0x5f80, 0x5f9f, K054000_w },
	{ 0x5fa0, 0x5faf, K053251_w },
	{ 0x5fb0, 0x5fb7, K053246_w },
	{ 0x5fe0, 0x5fe0, vendetta_5fe0_w },
	{ 0x5fe2, 0x5fe2, vendetta_eeprom_w },
	{ 0x5fe4, 0x5fe4, z80_irq_w },
	{ 0x5fe6, 0x5fe7, K053260_0_w },
	{ 0x4000, 0x4fff, MWA_BANK3 },
	{ 0x6000, 0x6fff, MWA_BANK2 },
	{ 0x4000, 0x7fff, K052109_w },
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END

static MEMORY_READ_START( esckids_readmem )
	{ 0x0000, 0x1fff, MRA_RAM },			/* 053248 64K SRAM*/
	{ 0x3f80, 0x3f80, input_port_0_r },		/* Player 1 Control*/
	{ 0x3f81, 0x3f81, input_port_1_r },		/* Player 2 Control*/
	{ 0x3f82, 0x3f82, input_port_4_r }, 	/* Player 3 Control ???  (But not used)*/
	{ 0x3f83, 0x3f83, input_port_5_r },		/* Player 4 Control ???  (But not used)*/
	{ 0x3f92, 0x3f92, vendetta_eeprom_r },	/* vblank, TEST SW on PCB*/
	{ 0x3f93, 0x3f93, input_port_2_r },		/* Start, Service*/
	{ 0x3fd4, 0x3fd4, vendetta_sound_interrupt_r },		/* Sound*/
	{ 0x3fd6, 0x3fd7, vendetta_sound_r },				/* Sound*/
	{ 0x3fd8, 0x3fd9, K053246_r },			/* 053246 (Sprite)*/
	{ 0x2000, 0x2fff, MRA_BANK3 },			/* 052109 (Tilemap) 0x0000-0x0fff*/
	{ 0x4000, 0x4fff, MRA_BANK2 },			/* 052109 (Tilemap) 0x2000-0x3fff, Tilemap MASK-ROM bank selector (MASK-ROM Test)*/
	{ 0x2000, 0x5fff, K052109_r },			/* 052109 (Tilemap)*/
	{ 0x6000, 0x7fff, MRA_BANK1 },			/* 053248 '975r01' 1M ROM (Banked)*/
	{ 0x8000, 0xffff, MRA_ROM },			/* 053248 '975r01' 1M ROM (0x18000-0x1ffff)*/
MEMORY_END

static MEMORY_WRITE_START( esckids_writemem )
	{ 0x0000, 0x1fff, MWA_RAM },			/* 053248 64K SRAM*/
	{ 0x3fa0, 0x3fa7, K053246_w },			/* 053246 (Sprite)*/
	{ 0x3fb0, 0x3fbf, K053251_w },			/* 053251 (Priority Encoder)*/
	{ 0x3fc0, 0x3fcf, MWA_NOP },			/* Not Emulated (053252 ???)*/
	{ 0x3fd0, 0x3fd0, vendetta_5fe0_w },	/* Coin Counter, 052109 RMRD, 053246 OBJCHA*/
	{ 0x3fd2, 0x3fd2, vendetta_eeprom_w },	/* EEPROM, Video banking*/
	{ 0x3fd4, 0x3fd4, z80_irq_w },			/* Sound*/
	{ 0x3fd6, 0x3fd7, K053260_0_w },		/* Sound*/
	{ 0x3fda, 0x3fda, MWA_NOP },			/* Not Emulated (Watchdog ???)*/
	{ 0x2000, 0x2fff, MWA_BANK3 },			/* 052109 (Tilemap) 0x0000-0x0fff*/
	{ 0x4000, 0x4fff, MWA_BANK2 },			/* 052109 (Tilemap) 0x2000-0x3fff, Tilemap MASK-ROM bank selector (MASK-ROM Test)*/
	{ 0x2000, 0x5fff, K052109_w },			/* 052109 (Tilemap)*/
	{ 0x6000, 0x7fff, MWA_ROM },			/* 053248 '975r01' 1M ROM (Banked)*/
	{ 0x8000, 0xffff, MWA_ROM },			/* 053248 '975r01' 1M ROM (0x18000-0x1ffff)*/
MEMORY_END


static MEMORY_READ_START( readmem_sound )
	{ 0x0000, 0xefff, MRA_ROM },
	{ 0xf000, 0xf7ff, MRA_RAM },
	{ 0xf801, 0xf801, YM2151_status_port_0_r },
	{ 0xfc00, 0xfc2f, K053260_0_r },
MEMORY_END

static MEMORY_WRITE_START( writemem_sound )
	{ 0x0000, 0xefff, MWA_ROM },
	{ 0xf000, 0xf7ff, MWA_RAM },
	{ 0xf800, 0xf800, YM2151_register_port_0_w },
	{ 0xf801, 0xf801, YM2151_data_port_0_w },
	{ 0xfa00, 0xfa00, z80_arm_nmi_w },
	{ 0xfc00, 0xfc2f, K053260_0_w },
MEMORY_END


/***************************************************************************

	Input Ports

***************************************************************************/

INPUT_PORTS_START( vendet4p )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* EEPROM data */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* EEPROM ready */
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_VBLANK ) /* not really vblank, object related. Its timed, otherwise sprites flicker */
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN3 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN4 )
INPUT_PORTS_END

INPUT_PORTS_START( vendetta )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* EEPROM data */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* EEPROM ready */
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_VBLANK ) /* not really vblank, object related. Its timed, otherwise sprites flicker */
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( esckids4p )
	PORT_START		/* Player 1 Control */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START		/* Player 2 Control */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START		/* Start, Service */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* EEPROM data */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* EEPROM ready */
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_VBLANK ) /* not really vblank, object related. Its timed, otherwise sprites flicker */
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )


	PORT_START		/* Player 3 Control ???  (Not used) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN3 )

	PORT_START		/* Player 4 Control ???  (Not used) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN4 )
INPUT_PORTS_END

INPUT_PORTS_START( esckids )
	PORT_START		/* Player 1 Control */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START		/* Player 2 Control */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START		/* Start, Service */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* EEPROM data */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* EEPROM ready */
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_VBLANK ) /* not really vblank, object related. Its timed, otherwise sprites flicker */
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END



/***************************************************************************

	Machine Driver

***************************************************************************/

static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	3579545,	/* 3.579545 MHz */
	{ YM3012_VOL(35,MIXER_PAN_LEFT,35,MIXER_PAN_RIGHT) },
	{ 0 }
};

static struct K053260_interface k053260_interface =
{
	1,
	{ 3579545 },
	{ REGION_SOUND1 }, /* memory region */
	{ { MIXER(75,MIXER_PAN_LEFT), MIXER(75,MIXER_PAN_RIGHT) } },
	{ 0 }
};

static INTERRUPT_GEN( vendetta_irq )
{
	if (irq_enabled)
		cpu_set_irq_line(0, KONAMI_IRQ_LINE, HOLD_LINE);
}

static MACHINE_DRIVER_START( vendetta )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", KONAMI, 6000000)		/* ? */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(vendetta_irq,1)

	MDRV_CPU_ADD(Z80, 3579545)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(readmem_sound,writemem_sound)
                            /* interrupts are triggered by the main CPU */
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(vendetta)
	MDRV_NVRAM_HANDLER(vendetta)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_HAS_SHADOWS)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(13*8, (64-13)*8-1, 2*8, 30*8-1 )
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(vendetta)
	MDRV_VIDEO_UPDATE(vendetta)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(K053260, k053260_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( esckids )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(vendetta)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(esckids_readmem,esckids_writemem)

	MDRV_VIDEO_START(esckids)

MACHINE_DRIVER_END



/***************************************************************************

  Game ROMs

***************************************************************************/

ROM_START( vendetta )
	ROM_REGION( 0x49000, REGION_CPU1, 0 ) /* code + banked roms + banked ram */
	ROM_LOAD( "081t01", 0x10000, 0x38000, CRC(e76267f5) SHA1(efef6c2edb4c181374661f358dad09123741b63d) )
	ROM_CONTINUE(		0x08000, 0x08000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the sound CPU */
	ROM_LOAD( "081b02", 0x000000, 0x10000, CRC(4c604d9b) SHA1(22d979f5dbde7912dd927bf5538fdbfc5b82905e) )

	ROM_REGION( 0x100000, REGION_GFX1, 0 ) /* graphics ( don't dispose as the program can read them ) */
	ROM_LOAD( "081a09", 0x000000, 0x080000, CRC(b4c777a9) SHA1(cc2b1dff4404ecd72b604e25d00fffdf7f0f8b52) ) /* characters */
	ROM_LOAD( "081a08", 0x080000, 0x080000, CRC(272ac8d9) SHA1(2da12fe4c13921bf0d4ebffec326f8d207ec4fad) ) /* characters */

	ROM_REGION( 0x400000, REGION_GFX2, 0 ) /* graphics ( don't dispose as the program can read them ) */
	ROM_LOAD( "081a04", 0x000000, 0x100000, CRC(464b9aa4) SHA1(28066ff0a07c3e56e7192918a882778c1b316b37) ) /* sprites */
	ROM_LOAD( "081a05", 0x100000, 0x100000, CRC(4e173759) SHA1(ce803f2aca7d7dedad00ab30e112443848747bd2) ) /* sprites */
	ROM_LOAD( "081a06", 0x200000, 0x100000, CRC(e9fe6d80) SHA1(2b7fc9d7fe43cd85dc8b975fe639c273cb0d9256) ) /* sprites */
	ROM_LOAD( "081a07", 0x300000, 0x100000, CRC(8a22b29a) SHA1(be539f21518e13038ab1d4cc2b2a901dd3e621f4) ) /* sprites */

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* 053260 samples */
	ROM_LOAD( "081a03", 0x000000, 0x100000, CRC(14b6baea) SHA1(fe15ee57f19f5acaad6c1642d51f390046a7468a) )
ROM_END

ROM_START( vendetao )
	ROM_REGION( 0x49000, REGION_CPU1, 0 ) /* code + banked roms + banked ram */
	ROM_LOAD( "081r01", 0x10000, 0x38000, CRC(84796281) SHA1(e4330c6eaa17adda5b4bd3eb824388c89fb07918) )
	ROM_CONTINUE(		0x08000, 0x08000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the sound CPU */
	ROM_LOAD( "081b02", 0x000000, 0x10000, CRC(4c604d9b) SHA1(22d979f5dbde7912dd927bf5538fdbfc5b82905e) )

	ROM_REGION( 0x100000, REGION_GFX1, 0 ) /* graphics ( don't dispose as the program can read them ) */
	ROM_LOAD( "081a09", 0x000000, 0x080000, CRC(b4c777a9) SHA1(cc2b1dff4404ecd72b604e25d00fffdf7f0f8b52) ) /* characters */
	ROM_LOAD( "081a08", 0x080000, 0x080000, CRC(272ac8d9) SHA1(2da12fe4c13921bf0d4ebffec326f8d207ec4fad) ) /* characters */

	ROM_REGION( 0x400000, REGION_GFX2, 0 ) /* graphics ( don't dispose as the program can read them ) */
	ROM_LOAD( "081a04", 0x000000, 0x100000, CRC(464b9aa4) SHA1(28066ff0a07c3e56e7192918a882778c1b316b37) ) /* sprites */
	ROM_LOAD( "081a05", 0x100000, 0x100000, CRC(4e173759) SHA1(ce803f2aca7d7dedad00ab30e112443848747bd2) ) /* sprites */
	ROM_LOAD( "081a06", 0x200000, 0x100000, CRC(e9fe6d80) SHA1(2b7fc9d7fe43cd85dc8b975fe639c273cb0d9256) ) /* sprites */
	ROM_LOAD( "081a07", 0x300000, 0x100000, CRC(8a22b29a) SHA1(be539f21518e13038ab1d4cc2b2a901dd3e621f4) ) /* sprites */

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* 053260 samples */
	ROM_LOAD( "081a03", 0x000000, 0x100000, CRC(14b6baea) SHA1(fe15ee57f19f5acaad6c1642d51f390046a7468a) )
ROM_END

ROM_START( vendet2p )
	ROM_REGION( 0x49000, REGION_CPU1, 0 ) /* code + banked roms + banked ram */
	ROM_LOAD( "081w01", 0x10000, 0x38000, CRC(cee57132) SHA1(8b6413877e127511daa76278910c2ee3247d613a) )
	ROM_CONTINUE(		0x08000, 0x08000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the sound CPU */
	ROM_LOAD( "081b02", 0x000000, 0x10000, CRC(4c604d9b) SHA1(22d979f5dbde7912dd927bf5538fdbfc5b82905e) )

	ROM_REGION( 0x100000, REGION_GFX1, 0 ) /* graphics ( don't dispose as the program can read them ) */
	ROM_LOAD( "081a09", 0x000000, 0x080000, CRC(b4c777a9) SHA1(cc2b1dff4404ecd72b604e25d00fffdf7f0f8b52) ) /* characters */
	ROM_LOAD( "081a08", 0x080000, 0x080000, CRC(272ac8d9) SHA1(2da12fe4c13921bf0d4ebffec326f8d207ec4fad) ) /* characters */

	ROM_REGION( 0x400000, REGION_GFX2, 0 ) /* graphics ( don't dispose as the program can read them ) */
	ROM_LOAD( "081a04", 0x000000, 0x100000, CRC(464b9aa4) SHA1(28066ff0a07c3e56e7192918a882778c1b316b37) ) /* sprites */
	ROM_LOAD( "081a05", 0x100000, 0x100000, CRC(4e173759) SHA1(ce803f2aca7d7dedad00ab30e112443848747bd2) ) /* sprites */
	ROM_LOAD( "081a06", 0x200000, 0x100000, CRC(e9fe6d80) SHA1(2b7fc9d7fe43cd85dc8b975fe639c273cb0d9256) ) /* sprites */
	ROM_LOAD( "081a07", 0x300000, 0x100000, CRC(8a22b29a) SHA1(be539f21518e13038ab1d4cc2b2a901dd3e621f4) ) /* sprites */

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* 053260 samples */
	ROM_LOAD( "081a03", 0x000000, 0x100000, CRC(14b6baea) SHA1(fe15ee57f19f5acaad6c1642d51f390046a7468a) )
ROM_END

ROM_START( vendetas )
	ROM_REGION( 0x49000, REGION_CPU1, 0 ) /* code + banked roms + banked ram */
	ROM_LOAD( "081u01", 0x10000, 0x38000, CRC(b4d9ade5) SHA1(fbd543738cb0b68c80ff05eed7849b608de03395) )
	ROM_CONTINUE(		0x08000, 0x08000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the sound CPU */
	ROM_LOAD( "081b02", 0x000000, 0x10000, CRC(4c604d9b) SHA1(22d979f5dbde7912dd927bf5538fdbfc5b82905e) )

	ROM_REGION( 0x100000, REGION_GFX1, 0 ) /* graphics ( don't dispose as the program can read them ) */
	ROM_LOAD( "081a09", 0x000000, 0x080000, CRC(b4c777a9) SHA1(cc2b1dff4404ecd72b604e25d00fffdf7f0f8b52) ) /* characters */
	ROM_LOAD( "081a08", 0x080000, 0x080000, CRC(272ac8d9) SHA1(2da12fe4c13921bf0d4ebffec326f8d207ec4fad) ) /* characters */

	ROM_REGION( 0x400000, REGION_GFX2, 0 ) /* graphics ( don't dispose as the program can read them ) */
	ROM_LOAD( "081a04", 0x000000, 0x100000, CRC(464b9aa4) SHA1(28066ff0a07c3e56e7192918a882778c1b316b37) ) /* sprites */
	ROM_LOAD( "081a05", 0x100000, 0x100000, CRC(4e173759) SHA1(ce803f2aca7d7dedad00ab30e112443848747bd2) ) /* sprites */
	ROM_LOAD( "081a06", 0x200000, 0x100000, CRC(e9fe6d80) SHA1(2b7fc9d7fe43cd85dc8b975fe639c273cb0d9256) ) /* sprites */
	ROM_LOAD( "081a07", 0x300000, 0x100000, CRC(8a22b29a) SHA1(be539f21518e13038ab1d4cc2b2a901dd3e621f4) ) /* sprites */

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* 053260 samples */
	ROM_LOAD( "081a03", 0x000000, 0x100000, CRC(14b6baea) SHA1(fe15ee57f19f5acaad6c1642d51f390046a7468a) )
ROM_END

ROM_START( vendtaso )
	ROM_REGION( 0x49000, REGION_CPU1, 0 ) /* code + banked roms + banked ram */
	ROM_LOAD( "081d01", 0x10000, 0x38000, CRC(335da495) SHA1(ea74680eb898aeecf9f1eec95f151bcf66e6b6cb) )
	ROM_CONTINUE(		0x08000, 0x08000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the sound CPU */
	ROM_LOAD( "081b02", 0x000000, 0x10000, CRC(4c604d9b) SHA1(22d979f5dbde7912dd927bf5538fdbfc5b82905e) )

	ROM_REGION( 0x100000, REGION_GFX1, 0 ) /* graphics ( don't dispose as the program can read them ) */
	ROM_LOAD( "081a09", 0x000000, 0x080000, CRC(b4c777a9) SHA1(cc2b1dff4404ecd72b604e25d00fffdf7f0f8b52) ) /* characters */
	ROM_LOAD( "081a08", 0x080000, 0x080000, CRC(272ac8d9) SHA1(2da12fe4c13921bf0d4ebffec326f8d207ec4fad) ) /* characters */

	ROM_REGION( 0x400000, REGION_GFX2, 0 ) /* graphics ( don't dispose as the program can read them ) */
	ROM_LOAD( "081a04", 0x000000, 0x100000, CRC(464b9aa4) SHA1(28066ff0a07c3e56e7192918a882778c1b316b37) ) /* sprites */
	ROM_LOAD( "081a05", 0x100000, 0x100000, CRC(4e173759) SHA1(ce803f2aca7d7dedad00ab30e112443848747bd2) ) /* sprites */
	ROM_LOAD( "081a06", 0x200000, 0x100000, CRC(e9fe6d80) SHA1(2b7fc9d7fe43cd85dc8b975fe639c273cb0d9256) ) /* sprites */
	ROM_LOAD( "081a07", 0x300000, 0x100000, CRC(8a22b29a) SHA1(be539f21518e13038ab1d4cc2b2a901dd3e621f4) ) /* sprites */

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* 053260 samples */
	ROM_LOAD( "081a03", 0x000000, 0x100000, CRC(14b6baea) SHA1(fe15ee57f19f5acaad6c1642d51f390046a7468a) )
ROM_END

ROM_START( vendettj )
	ROM_REGION( 0x49000, REGION_CPU1, 0 ) /* code + banked roms + banked ram */
	ROM_LOAD( "081p01", 0x10000, 0x38000, CRC(5fe30242) SHA1(2ea98e66637fa2ad60044b1a2b0dd158a82403a2) )
	ROM_CONTINUE(		0x08000, 0x08000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the sound CPU */
	ROM_LOAD( "081b02", 0x000000, 0x10000, CRC(4c604d9b) SHA1(22d979f5dbde7912dd927bf5538fdbfc5b82905e) )

	ROM_REGION( 0x100000, REGION_GFX1, 0 ) /* graphics ( don't dispose as the program can read them ) */
	ROM_LOAD( "081a09", 0x000000, 0x080000, CRC(b4c777a9) SHA1(cc2b1dff4404ecd72b604e25d00fffdf7f0f8b52) ) /* characters */
	ROM_LOAD( "081a08", 0x080000, 0x080000, CRC(272ac8d9) SHA1(2da12fe4c13921bf0d4ebffec326f8d207ec4fad) ) /* characters */

	ROM_REGION( 0x400000, REGION_GFX2, 0 ) /* graphics ( don't dispose as the program can read them ) */
	ROM_LOAD( "081a04", 0x000000, 0x100000, CRC(464b9aa4) SHA1(28066ff0a07c3e56e7192918a882778c1b316b37) ) /* sprites */
	ROM_LOAD( "081a05", 0x100000, 0x100000, CRC(4e173759) SHA1(ce803f2aca7d7dedad00ab30e112443848747bd2) ) /* sprites */
	ROM_LOAD( "081a06", 0x200000, 0x100000, CRC(e9fe6d80) SHA1(2b7fc9d7fe43cd85dc8b975fe639c273cb0d9256) ) /* sprites */
	ROM_LOAD( "081a07", 0x300000, 0x100000, CRC(8a22b29a) SHA1(be539f21518e13038ab1d4cc2b2a901dd3e621f4) ) /* sprites */

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* 053260 samples */
	ROM_LOAD( "081a03", 0x000000, 0x100000, CRC(14b6baea) SHA1(fe15ee57f19f5acaad6c1642d51f390046a7468a) )
ROM_END


ROM_START( esckids )
	ROM_REGION( 0x049000, REGION_CPU1, 0 )		/* Main CPU (053248) Code & Banked (1M x 1) */
	ROM_LOAD( "975r01", 0x010000, 0x018000, CRC(9dfba99c)  )
	ROM_CONTINUE(		0x008000, 0x008000 )

	ROM_REGION( 0x010000, REGION_CPU2, 0 )		/* Sound CPU (Z80) Code (512K x 1) */
	ROM_LOAD( "975f02", 0x000000, 0x010000, CRC(994fb229)  )

	ROM_REGION( 0x100000, REGION_GFX1, 0 )		/* Tilemap MASK-ROM (4M x 2) */
	ROM_LOAD( "975c09", 0x000000, 0x080000, CRC(bc52210e)  )
	ROM_LOAD( "975c08", 0x080000, 0x080000, CRC(fcff9256)  )

	ROM_REGION( 0x400000, REGION_GFX2, 0 )		/* Sprite MASK-ROM (8M x 4) */
	ROM_LOAD( "975c04", 0x000000, 0x100000, CRC(15688a6f)  )
	ROM_LOAD( "975c05", 0x100000, 0x100000, CRC(1ff33bb7)  )
	ROM_LOAD( "975c06", 0x200000, 0x100000, CRC(36d410f9)  )
	ROM_LOAD( "975c07", 0x300000, 0x100000, CRC(97ec541e)  )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* Samples MASK-ROM (4M x 1) */
	ROM_LOAD( "975c03", 0x000000, 0x080000, CRC(dc4a1707)  )
ROM_END

ROM_START( esckidsj )
	ROM_REGION( 0x049000, REGION_CPU1, 0 )		/* Main CPU (053248) Code & Banked (1M x 1)*/
	ROM_LOAD( "975r01j", 0x010000, 0x018000, CRC(7b5c5572) SHA1(b94b58c010539926d112c2dfd80bcbad76acc986) )
	ROM_CONTINUE(		0x008000, 0x008000 )

	ROM_REGION( 0x010000, REGION_CPU2, 0 )		/* Sound CPU (Z80) Code (512K x 1)*/
	ROM_LOAD( "975f02", 0x000000, 0x010000, CRC(994fb229) SHA1(bf194ae91240225b8edb647b1a62cd83abfa215e) )

	ROM_REGION( 0x100000, REGION_GFX1, 0 )		/* Tilemap MASK-ROM (4M x 2)*/
	ROM_LOAD( "975c09", 0x000000, 0x080000, CRC(bc52210e) SHA1(301a3892d250495c2e849d67fea5f01fb0196bed) )
	ROM_LOAD( "975c08", 0x080000, 0x080000, CRC(fcff9256) SHA1(b60d29f4d04f074120d4bb7f2a71b9e9bf252d33) )

	ROM_REGION( 0x400000, REGION_GFX2, 0 )		/* Sprite MASK-ROM (8M x 4)*/
	ROM_LOAD( "975c04", 0x000000, 0x100000, CRC(15688a6f) SHA1(a445237a11e5f98f0f9b2573a7ef0583366a137e) )
	ROM_LOAD( "975c05", 0x100000, 0x100000, CRC(1ff33bb7) SHA1(eb17da33ba2769ea02f91fece27de2e61705e75a) )
	ROM_LOAD( "975c06", 0x200000, 0x100000, CRC(36d410f9) SHA1(2b1fd93c11839480aa05a8bf27feef7591704f3d) )
	ROM_LOAD( "975c07", 0x300000, 0x100000, CRC(97ec541e) SHA1(d1aa186b17cfe6e505f5b305703319299fa54518) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* Samples MASK-ROM (4M x 1)*/
	ROM_LOAD( "975c03", 0x000000, 0x080000, CRC(dc4a1707) SHA1(f252d08483fd664f8fc03bf8f174efd452b4cdc5) )
ROM_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

static void vendetta_banking( int lines )
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	if ( lines >= 0x1c )
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "PC = %04x : Unknown bank selected %02x\n", activecpu_get_pc(), lines );
	}
	else
		cpu_setbank( 1, &RAM[ 0x10000 + ( lines * 0x2000 ) ] );
}

static MACHINE_INIT( vendetta )
{
	konami_cpu_setlines_callback = vendetta_banking;

	paletteram = &memory_region(REGION_CPU1)[0x48000];
	irq_enabled = 0;

	/* init banks */
	cpu_setbank( 1, &memory_region(REGION_CPU1)[0x10000] );
	vendetta_video_banking( 0 );
}


static DRIVER_INIT( vendetta )
{
	video_banking_base = 0x4000;
	konami_rom_deinterleave_2(REGION_GFX1);
	konami_rom_deinterleave_4(REGION_GFX2);
}

static DRIVER_INIT( esckids )
{
	video_banking_base = 0x2000;
	konami_rom_deinterleave_2(REGION_GFX1);
	konami_rom_deinterleave_4(REGION_GFX2);
}


GAME( 1991, vendetta, 0,        vendetta, vendet4p, vendetta, ROT0, "Konami", "Vendetta (World 4 Players ver. T)" )
GAME( 1991, vendetao, vendetta, vendetta, vendet4p, vendetta, ROT0, "Konami", "Vendetta (World 4 Players ver. R)" )
GAME( 1991, vendet2p, vendetta, vendetta, vendetta, vendetta, ROT0, "Konami", "Vendetta (World 2 Players ver. W)" )
GAME( 1991, vendetas, vendetta, vendetta, vendetta, vendetta, ROT0, "Konami", "Vendetta (Asia 2 Players ver. U)" )
GAME( 1991, vendtaso, vendetta, vendetta, vendetta, vendetta, ROT0, "Konami", "Vendetta (Asia 2 Players ver. D)" )
GAME( 1991, vendettj, vendetta, vendetta, vendetta, vendetta, ROT0, "Konami", "Crime Fighters 2 (Japan 2 Players ver. P)" )
GAME( 1991, esckids,  0,        esckids,  esckids4p,esckids,  ROT0, "Konami", "Escape Kids (Asia 4 Players)" )
GAME( 1991, esckidsj, esckids,  esckids,  esckids,  esckids,  ROT0, "Konami", "Escape Kids (Japan 2 Players)" )
