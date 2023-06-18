/***************************************************************************

  Capcom System 2
  ===============


    Driver by Paul Leaman (paul@vortexcomputing.demon.co.uk)

    Thanks to Raz, Crashtest and the CPS2 decryption team without whom
    none of this would have been possible.


	Some Encryption notes.
	----------------------

	Encryption Watchdog;

	The CPS2 system has a watchdog system that will disable the decryption
	of data if the watchdog isn't triggered at least once every few seconds.
	The trigger varies from game to game (some games do use the same) and
	is basically a 68000 opcode/s instruction. Below is a list;

  	cmpi.l	#$7B5D94F1,D6		1944: The Loop Master
	cmpi.l	#$00951101,D1		19xx: The War Against Destiny
	cmpi.l	#$12345678,D0		Alien Vs. Predator
	cmpi.l	#$00970131,D1		Battle Circuit
	cmpi.l	#$00970310,D1		Capcom Sports Club
	 unknown 					Choko
	cmpi.b	#$FF,$0C38			Cyberbots
	cmpi.w	#$1019,$4000		Dungeons & Dragons: Tower of Doom
  	cmpi.l	#$19660419,D1		Dungeons & Dragons: Shadow over Mystara
	 unknown 					Galum Pa!
	cmpi.l	#$19721027,D1		Giga Wing

  	cmp.w	A4,D7 \
	cmp.w	D4,D1  }-			Great Mahou Daisakusen
	cmpa.w	D5,A3 /

  	 unknown 					Jyangokushi
  	 unknown 					Ken Sei Mogura

  	cmpa.w	D0,A3 \
	cmp.w	D7,D2  }-			Mars Matrix
	cmpa.w	A7,A5 /

  	cmpi.l	#$19660419,D1		Marvel Super Heroes
	cmpi.l	#$19721027,D1		Marvel Super Heroes Vs. Street Fighter
	cmpi.l	#$19720121,D1		Marvel Vs. Capcom
	cmpi.l	#$347D89A3,D4		Mighty! Pang
  	 unknown 					Pnickies
	cmpi.l	#$1F740D12,D0		Pocket Fighters
	move.w	$00804020,D0		Powered Gear
  	 unknown 					Progear no Arashi
  	 unknown 					Puzz Loop 2
	cmpi.l	#$19730827,D1		Quiz Nanairo Dreams
  	cmpi.l	#$05642194,D0		Rockman					(found in CPS1 version)
	cmpi.l	#$01647101,D0		Rockman2
	cmpi.l	#$05642194,D0		Street Fighter Zero
	cmpi.l	#$30399783,D0		Street Fighter Zero 2
	cmpi.l	#$8E739110,D0		Street Fighter Zero 2 Alpha
	cmpi.l	#$1C62F5A8,D0		Street Fighter Zero 3
	move.w	$00804020,D0		Super Muscle Bomber
	cmpi.l	#$30399819,D0		Super Puzzle Fighter 2X
	btst	#7,$2000			Super Street Fighter 2
	btst	#7,$2000			Super Street Fighter 2X
	btst	#3,$7345			Ultimate Ecology
	btst	#0,$6160			Vampire
	btst	#0,$6160			Vampire Hunter
	cmpi.l	#$06920760,D0		Vampire Hunter 2
	cmpi.l	#$726A4BAF,D0		Vampire Savior
	cmpi.l	#$06920760,D0		Vampire Savior 2
	cmpi.l	#$19720301,D0		X-Men, Children of the Atom
	cmpi.l	#$19720327,D1		X-Men Vs. Street Fighter

	Encryption

	Currently the algorhythm is unknown so instead we use ready decrypted
	data obtained from real hardware to create XOR files. These files are
	then used against the encrypted program ROM's to decrypt them.

	All that is known about the algorhythm so far is that it only uses the
	lower 17 bits of the possible 24 address lines, this basically means
	the encryption loops on 0x20000 boundries.


***************************************************************************/

/* SPECIAL NOTE*/
/* Dimahoo requires the C 68k core for attract mode to sync collectly with mucic.*/
/* If the ASM 68k core is used GFX slow at the point where the blue pictures fade*/
/* in and out. This bug also seems to change the speed of the medallion fade out*/
/* at the start of Giga Wing's attract mode. This could be the same bug in the*/
/* ASM core that causes the NeoGeo game view point to stop working.*/

#include "driver.h"
#include "machine/eeprom.h"
#include "cpu/m68000/m68000.h"
#include "bootstrap.h"
#include "inptport.h"

#include "cps1.h"       /* External CPS1 definitions */

extern DRIVER_INIT( my_cps2 );
/*
Export this function so that the vidhrdw routine can drive the
Q-Sound hardware
*/
WRITE16_HANDLER( cps2_qsound_sharedram_w )
{
    qsound_sharedram1_w(offset/2, data, 0xff00);
}

/* Maximum size of Q Sound Z80 region */
#define QSOUND_SIZE 0x50000

/* Maximum 680000 code size */
#undef  CODE_SIZE
#define CODE_SIZE   0x0400000


extern data16_t *cps2_objram1,*cps2_objram2;
extern data16_t *cps2_output;
extern size_t cps2_output_size;
extern VIDEO_START( cps2 );

extern int scanline1;
extern int scanline2;
extern int scancalls;
void cps2_objram_latch(void);
void cps2_set_sprite_priorities(void);


static INTERRUPT_GEN( cps2_interrupt )
{
	static int scancount;

	/* 2 is vblank, 4 is some sort of scanline interrupt, 6 is both at the same time. */

	if(scancount >= 261)
	{
		scancount = -1;
		scancalls = 0;
	}
	scancount++;

	if(cps1_output[0x50/2] & 0x8000)
		cps1_output[0x50/2] = cps1_output[0x50/2] & 0x1ff;
	if(cps1_output[0x52/2] & 0x8000)
		cps1_output[0x52/2] = cps1_output[0x52/2] & 0x1ff;

/*	usrintf_showmessage("%04x %04x - %04x %04x",scanline1,scanline2,cps1_output[0x50/2],cps1_output[0x52/2]);*/

	/* raster effects */
	if(scanline1 == scancount || (scanline1 < scancount && !scancalls))
	{
		cps1_output[0x50/2] = 0;
		cpu_set_irq_line(0, 4, HOLD_LINE);
		cps2_set_sprite_priorities();
		force_partial_update(16 - 10 + scancount);	/* Machine->visible_area.min_y - [first visible line?] + scancount */
		scancalls++;
/*			usrintf_showmessage("IRQ4 scancounter = %04i",scancount);*/
	}

	/* raster effects */
	if(scanline2 == scancount || (scanline2 < scancount && !scancalls))
	{
		cps1_output[0x52/2] = 0;
		cpu_set_irq_line(0, 4, HOLD_LINE);
		cps2_set_sprite_priorities();
		force_partial_update(16 - 10 + scancount);	/* Machine->visible_area.min_y - [first visible line?] + scancount */
		scancalls++;
/*			usrintf_showmessage("IRQ4 scancounter = %04i",scancount);*/
	}

	if(scancount == 240)  /* VBlank */
	{
		cps1_output[0x50/2] = scanline1;
		cps1_output[0x52/2] = scanline2;
		cpu_set_irq_line(0, 2, HOLD_LINE);
		if(scancalls)
		{
			cps2_set_sprite_priorities();
			force_partial_update(240);
		}
		cps2_objram_latch();
	}
	/*usrintf_showmessage("Raster calls = %i",scancalls);*/
}



static struct EEPROM_interface cps2_eeprom_interface =
{
	6,		/* address bits */
	16,		/* data bits */
	"0110",	/*  read command */
	"0101",	/* write command */
	"0111"	/* erase command */
};

static NVRAM_HANDLER( cps2 )
{
	if (read_or_write)
		EEPROM_save(file);
	else
	{
        EEPROM_init(&cps2_eeprom_interface);

		if (file)
			EEPROM_load(file);
	}
}

READ16_HANDLER( cps2_eeprom_port_r )
{
    return (input_port_2_word_r(offset,0) & 0xfffe) | EEPROM_read_bit();
}

WRITE16_HANDLER( cps2_eeprom_port_w )
{
    if (ACCESSING_MSB)
    {
	/* bit 0 - Unused */
	/* bit 1 - Unused */
	/* bit 2 - Unused */
	/* bit 3 - Unused? */
	/* bit 4 - Eeprom data  */
	/* bit 5 - Eeprom clock */
	/* bit 6 - */
	/* bit 7 - */

	/* EEPROM */
	EEPROM_write_bit(data & 0x1000);
	EEPROM_set_clock_line((data & 0x2000) ? ASSERT_LINE : CLEAR_LINE);
	EEPROM_set_cs_line((data & 0x4000) ? CLEAR_LINE : ASSERT_LINE);
	}

	if (ACCESSING_LSB)
	{
	/* bit 0 - coin counter 1 */
	/* bit 0 - coin counter 2 */
	/* bit 2 - Unused */
	/* bit 3 - Allows access to Z80 address space (Z80 reset) */
	/* bit 4 - lock 1  */
	/* bit 5 - lock 2  */
	/* bit 6 - */
	/* bit 7 - */

        /* Z80 Reset */
		cpu_set_reset_line(1,(data & 0x0008) ? CLEAR_LINE : ASSERT_LINE);

	coin_counter_w(0, data & 0x0001);
	coin_counter_w(1, data & 0x0002);

	if(strncmp(Machine->gamedrv->name,"mmatrix",7)==0)		/* Mars Matrix seems to require the coin lockout bit to be reversed*/
	{
		coin_lockout_w(0,data & 0x0010);
		coin_lockout_w(1,data & 0x0020);
		coin_lockout_w(2,data & 0x0040);
		coin_lockout_w(3,data & 0x0080);
	}
	else
	{
		coin_lockout_w(0,~data & 0x0010);
		coin_lockout_w(1,~data & 0x0020);
		coin_lockout_w(2,~data & 0x0040);
		coin_lockout_w(3,~data & 0x0080);
	}

	/*
	set_led_status(0,data & 0x01);
	set_led_status(1,data & 0x10);
	set_led_status(2,data & 0x20);
	*/
    }
}

READ16_HANDLER( cps2_qsound_volume_r )
{
	/* Extra adapter memory (0x660000-0x663fff) available when bit 14 = 0 */
	/* Network adapter (ssf2tb) present when bit 15 = 0 */
	/* Only game known to use both these so far is SSF2TB */

	if(strcmp(Machine->gamedrv->name,"ssf2tb")==0)
		return 0x2021;
	else
		return 0xe021;
}


static data8_t CPS2_Read8(offs_t address)
{
	return m68k_read_pcrelative_8(address);
}

static data16_t CPS2_Read16(offs_t address)
{
	return m68k_read_pcrelative_16(address);
}

static data32_t CPS2_Read32(offs_t address)
{
	return m68k_read_pcrelative_32(address);
}



static READ16_HANDLER( kludge_r )
{
	return 0xffff;
}


static MEMORY_READ16_START( cps2_readmem )
	{ 0x000000, 0x3fffff, MRA16_ROM },				/* 68000 ROM */
	{ 0x400000, 0x40000b, MRA16_RAM },				/* CPS2 object output */
	{ 0x618000, 0x619fff, qsound_sharedram1_r },		/* Q RAM */
	{ 0x662000, 0x662001, MRA16_RAM },				/* Network adapter related, accessed in SSF2TB */
	{ 0x662008, 0x662009, MRA16_RAM },				/* Network adapter related, accessed in SSF2TB */
	{ 0x662020, 0x662021, MRA16_RAM },				/* Network adapter related, accessed in SSF2TB */
	{ 0x660000, 0x663fff, MRA16_RAM },				/* When bit 14 of 0x804030 equals 0 this space is available. Many games store highscores and other info here if available. */
	{ 0x664000, 0x664001, MRA16_RAM },				/* Unknown - Only used if 0x660000-0x663fff available (could be RAM enable?) */
	{ 0x708000, 0x709fff, cps2_objram2_r },			/* Object RAM */
	{ 0x70a000, 0x70bfff, cps2_objram2_r },			/* mirror */
	{ 0x70c000, 0x70dfff, cps2_objram2_r },			/* mirror */
	{ 0x70e000, 0x70ffff, cps2_objram2_r },			/* mirror */
	{ 0x800100, 0x8001ff, cps1_output_r },			/* Output ports mirror (sfa) */
	{ 0x804000, 0x804001, input_port_0_word_r },		/* IN0 */
	{ 0x804010, 0x804011, input_port_1_word_r },		/* IN1 */
	{ 0x804020, 0x804021, cps2_eeprom_port_r  },		/* IN2 + EEPROM */
	{ 0x804030, 0x804031, cps2_qsound_volume_r },		/* Master volume. Also when bit 14=0 addon memory is present, when bit 15=0 network adapter present. */
	{ 0x8040b0, 0x8040b3, kludge_r },  				/* unknown (xmcotaj hangs if this is 0) */
	{ 0x804100, 0x8041ff, cps1_output_r },			/* CPS1 Output ports */
	{ 0x900000, 0x92ffff, MRA16_RAM },				/* Video RAM */
	{ 0xff0000, 0xffffff, MRA16_RAM },				/* RAM */
MEMORY_END

static MEMORY_WRITE16_START( cps2_writemem )
	{ 0x000000, 0x3fffff, MWA16_ROM },				/* ROM */
	{ 0x400000, 0x40000b, MWA16_RAM, &cps2_output, &cps2_output_size },	/* CPS2 output */
	{ 0x618000, 0x619fff, qsound_sharedram1_w },		/* Q RAM */
	{ 0x662000, 0x662001, MWA16_RAM },				/* Network adapter related, accessed in SSF2TB */
	{ 0x662008, 0x662009, MWA16_RAM },				/* Network adapter related, accessed in SSF2TB (not sure if this port is write only yet)*/
	{ 0x662020, 0x662021, MWA16_RAM },				/* Network adapter related, accessed in SSF2TB */
	{ 0x660000, 0x663fff, MWA16_RAM },				/* When bit 14 of 0x804030 equals 0 this space is available. Many games store highscores and other info here if available. */
	{ 0x664000, 0x664001, MWA16_RAM },				/* Unknown - Only used if 0x660000-0x663fff available (could be RAM enable?) */
	{ 0x700000, 0x701fff, cps2_objram1_w, &cps2_objram1 },	/* Object RAM, no game seems to use it directly */
	{ 0x708000, 0x709fff, cps2_objram2_w, &cps2_objram2 },	/* Object RAM */
	{ 0x70a000, 0x70bfff, cps2_objram2_w },			/* mirror */
	{ 0x70c000, 0x70dfff, cps2_objram2_w },			/* mirror */
	{ 0x70e000, 0x70ffff, cps2_objram2_w },			/* mirror */
	{ 0x800100, 0x8001ff, cps1_output_w },			/* Output ports mirror (sfa) */
	{ 0x804040, 0x804041, cps2_eeprom_port_w },		/* EEPROM */
	{ 0x8040a0, 0x8040a1, MWA16_NOP },				/* Unknown (reset once on startup) */
	{ 0x8040e0, 0x8040e1, cps2_objram_bank_w },		/* bit 0 = Object ram bank swap */
	{ 0x804100, 0x8041ff, cps1_output_w, &cps1_output, &cps1_output_size },  /* Output ports */
	{ 0x900000, 0x92ffff, cps1_gfxram_w, &cps1_gfxram, &cps1_gfxram_size },
	{ 0xff0000, 0xffffff, MWA16_RAM },				/* RAM */
MEMORY_END




INPUT_PORTS_START( 19xx )
    PORT_START      /* IN0 (0x00) */
    PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
    PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
    PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
    PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
    PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
    PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
    PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
    PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
    PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
    PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
    PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
    PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
    PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

    PORT_START      /* IN1 (0x10) */
    PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

    PORT_START      /* IN2 (0x20) */
    PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_SPECIAL )   /* EEPROM bit */
	PORT_BITX(0x0002, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
    PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
    PORT_BIT( 0x00f8, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
    PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
    PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_COIN1 )
    PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_COIN2 )
    PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( cybots )
    PORT_START      /* IN0 (0x00) */
    PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
    PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
    PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
    PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
    PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
    PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
    PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
    PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
    PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
    PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
    PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
    PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
    PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
    PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
    PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

    PORT_START      /* IN1 (0x10) */
    PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
    PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
    PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

    PORT_START      /* IN2 (0x20) */
    PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_SPECIAL )   /* EEPROM bit */
	PORT_BITX(0x0002, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
    PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
    PORT_BIT( 0x00f8, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
    PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
    PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_COIN1 )
    PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_COIN2 )
    PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( ssf2 )
    PORT_START      /* IN0 (0x00) */
    PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
    PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
    PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
    PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
    PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
    PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
    PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
    PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
    PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
    PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
    PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
    PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
    PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
    PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
    PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

    PORT_START      /* IN1 (0x10) */
    PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
    PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1 )
    PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER1 )
    PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
    PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER2 )
    PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

    PORT_START      /* IN2 (0x20) */
    PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_SPECIAL )   /* EEPROM bit */
	PORT_BITX(0x0002, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
    PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
    PORT_BIT( 0x00f8, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
    PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
    PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_COIN1 )
    PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_COIN2 )
    PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER2 )
    PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( ddtod )
    PORT_START      /* IN0 (0x00) */
    PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
    PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
    PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
    PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
    PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
    PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
    PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
    PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
    PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
    PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
    PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
    PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
    PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
    PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
    PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
    PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )

    PORT_START      /* IN1 (0x10) */
    PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
    PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
    PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
    PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
    PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
    PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
    PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )
    PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER3 )
    PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
    PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER4 )
    PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER4 )
    PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER4 )
    PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
    PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
    PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER4 )
    PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER4 )

    PORT_START      /* IN2 (0x20) */
    PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_SPECIAL )   /* EEPROM bit */
	PORT_BITX(0x0002, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
    PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
    PORT_BIT( 0x00f8, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
    PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
    PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_START3 )
    PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_START4 )
    PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_COIN1 )
    PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_COIN2 )
    PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_COIN3 )
    PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_COIN4 )
INPUT_PORTS_END

INPUT_PORTS_START( avsp )
    PORT_START      /* IN0 (0x00) */
    PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
    PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
    PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
    PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
    PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
    PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
    PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
    PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
    PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
    PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
    PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
    PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
    PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
    PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
    PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

    PORT_START      /* IN1 (0x10) */
    PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
    PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
    PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
    PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
    PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
    PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
    PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )
    PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

    PORT_START      /* IN2 (0x20) */
    PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_SPECIAL )   /* EEPROM bit */
	PORT_BITX(0x0002, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
    PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
    PORT_BIT( 0x00f8, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
    PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
    PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_START3 )
    PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_COIN1 )
    PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_COIN2 )
    PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_COIN3 )
    PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( qndream )
    PORT_START      /* IN0 (0x00) */
    PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
    PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
    PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
    PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
    PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
    PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
    PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
    PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
    PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

    PORT_START      /* IN1 (0x10) */
    PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

    PORT_START      /* IN2 (0x20) */
    PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_SPECIAL )   /* EEPROM bit */
	PORT_BITX(0x0002, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
    PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
    PORT_BIT( 0x00f8, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
    PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
    PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_COIN1 )
    PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_COIN2 )
    PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( batcir )
    PORT_START      /* IN0 (0x00) */
    PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
    PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
    PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
    PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
    PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
    PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
    PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
    PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
    PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
    PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
    PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
    PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
    PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

    PORT_START      /* IN1 (0x10) */
    PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
    PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
    PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
    PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
    PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
    PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
    PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
    PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER4 )
    PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER4 )
    PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER4 )
    PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
    PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
    PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

    PORT_START      /* IN2 (0x20) */
    PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_SPECIAL )   /* EEPROM bit */
	PORT_BITX(0x0002, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
    PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
    PORT_BIT( 0x00f8, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
    PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
    PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_START3 )
    PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_START4 )
    PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_COIN1 )
    PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_COIN2 )
    PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_COIN3 )
    PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_COIN4 )
INPUT_PORTS_END

INPUT_PORTS_START( sgemf )
    PORT_START      /* IN0 (0x00) */
    PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
    PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
    PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
    PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
    PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
    PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
    PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
    PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
    PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
    PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
    PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
    PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
    PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
    PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
    PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

    PORT_START      /* IN1 (0x10) */
    PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

    PORT_START      /* IN2 (0x20) */
    PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_SPECIAL )   /* EEPROM bit */
	PORT_BITX(0x0002, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
    PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
    PORT_BIT( 0x00f8, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
    PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
    PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_COIN1 )
    PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_COIN2 )
    PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( ringdest )
    PORT_START      /* IN0 (0x00) */
    PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
    PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
    PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
    PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
    PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
    PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
    PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
    PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
    PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
    PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
    PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
    PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
    PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
    PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
    PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

    PORT_START      /* IN1 (0x10) */
    PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
    PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1 )
    PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
    PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

    PORT_START      /* IN2 (0x20) */
    PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_SPECIAL )   /* EEPROM bit */
	PORT_BITX(0x0002, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
    PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
    PORT_BIT( 0x00f8, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
    PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
    PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_COIN1 )
    PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_COIN2 )
    PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER2 )
    PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( cps2 )
    PORT_START      /* IN0 (0x00) */
    PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
    PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
    PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
    PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
    PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
    PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
    PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
    PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
    PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
    PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
    PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
    PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
    PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
    PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
    PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

    PORT_START      /* IN1 (0x10) */
    PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
    PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1 )
    PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER1 )
    PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
    PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER2 )
    PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

    PORT_START      /* IN2 (0x20) */
    PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_SPECIAL )   /* EEPROM bit */
	PORT_BITX(0x0002, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
    PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
    PORT_BIT( 0x00f8, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
    PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
    PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_COIN1 )
    PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_COIN2 )
    PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER2 )
    PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END



static struct GfxLayout layout8x8 =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ GFX_RAW },
	{ 4*8 },	/* org displacement - 8x8 tiles are taken from the RIGHT side of the 16x16 tile
				   (fixes cawing which uses character 0x0002 as space, typo instead of 0x20?) */
	{ 8*8 },	/* line modulo */
	64*8		/* char modulo */
};

static struct GfxLayout layout16x16 =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ GFX_RAW },
	{ 0 },		/* org displacement */
	{ 8*8 },	/* line modulo */
	128*8		/* char modulo */
};

static struct GfxLayout layout32x32 =
{
	32,32,
	RGN_FRAC(1,1),
	4,
	{ GFX_RAW },
	{ 0 },		/* org displacement */
	{ 16*8 },	/* line modulo */
	512*8		/* char modulo */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout8x8,   0, 0x100 },
	{ REGION_GFX1, 0, &layout16x16, 0, 0x100 },
	{ REGION_GFX1, 0, &layout32x32, 0, 0x100 },
	{ -1 }
};



static struct m68k_encryption_interface cps2_encryption =
{
	CPS2_Read8, CPS2_Read16, CPS2_Read32,
	CPS2_Read16, CPS2_Read32
};

static MACHINE_DRIVER_START( cps2 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 11800000)
	MDRV_CPU_CONFIG(cps2_encryption)
	MDRV_CPU_MEMORY(cps2_readmem,cps2_writemem)
	MDRV_CPU_VBLANK_INT(cps2_interrupt,262)	/* 262  ??? interrupts per frame*/

	MDRV_CPU_ADD(Z80, 8000000)
	MDRV_CPU_MEMORY(qsound_readmem,qsound_writemem)
	MDRV_CPU_PERIODIC_INT(irq0_line_hold,251)	/* 251 is good (see 'mercy mercy mercy'section of sgemf attract mode for accurate sound sync */

	MDRV_FRAMES_PER_SECOND(59.633333)
	MDRV_VBLANK_DURATION(0)
/*	MDRV_INTERLEAVE(262)    262 scanlines, for raster effects */

	MDRV_NVRAM_HANDLER(cps2)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(8*8, (64-8)*8-1, 2*8, 30*8-1 )
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4096)

	MDRV_VIDEO_START(cps2)
	MDRV_VIDEO_EOF(cps1)
	MDRV_VIDEO_UPDATE(cps1)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(QSOUND, qsound_interface)
MACHINE_DRIVER_END

ROM_START( 1944 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "nffu.03", 0x000000, 0x80000, CRC(9693cf8f) SHA1(c296cb008e282f77b44374d1c3638a3f4d5d5d4e) )
	ROM_LOAD16_WORD_SWAP( "nff.04",  0x080000, 0x80000, CRC(dba1c66e) SHA1(4764e77d4da5d19d9acded27df1e1bcba06b0fcf) )
	ROM_LOAD16_WORD_SWAP( "nffu.05", 0x100000, 0x80000, CRC(ea813eb7) SHA1(34e0175a5f22d08c3538369b4bfd077a7427a128) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "nffux.03", 0x000000, 0x80000, CRC(675c8109) SHA1(ab9756dc81c231e75050f22c8cdd3759a592bbe1) )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "nff.13",   0x0000000, 0x400000, CRC(c9fca741) SHA1(1781d4fc18b6d6f79b7b39d9bcace750fb61a5cf) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "nff.15",   0x0000002, 0x400000, CRC(f809d898) SHA1(a0b6af49e1780678d808c317b875161cedddb314) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "nff.17",   0x0000004, 0x400000, CRC(15ba4507) SHA1(bed6a82bf1dc1aa501d4c2d098115a15e18d446a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "nff.19",   0x0000006, 0x400000, CRC(3dd41b8c) SHA1(676078baad789e25f6e5a79de29672587be7ff00) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "nff.14",   0x1000000, 0x100000, CRC(3fe3a54b) SHA1(0a8e5cae141d24fd8b3cb11796c44728b0acd69e) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "nff.16",   0x1000002, 0x100000, CRC(565cd231) SHA1(0aecd433fb4ca2de1aca9fbb1e314fb1f6979321) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "nff.18",   0x1000004, 0x100000, CRC(63ca5988) SHA1(30137fa77573c84bcc24570bccb7dba61ddb413c) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "nff.20",   0x1000006, 0x100000, CRC(21eb8f3b) SHA1(efa69f19a958047dd91a294c88857ed3133fcbef) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "nff.01",   0x00000, 0x08000, CRC(d2e44318) SHA1(33e45f6fe9fed098a4c072b8c39406aef1a949b2) )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "nff.11",   0x000000, 0x400000, CRC(243e4e05) SHA1(83281f7290ac105a3f9a7507cbc11317d45ba706) )
	ROM_LOAD16_WORD_SWAP( "nff.12",   0x400000, 0x400000, CRC(4fcf1600) SHA1(36f18c5d92b79433bdf7088b29a244708929d48e) )
ROM_END

ROM_START( 1944j )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "nffj.03", 0x000000, 0x80000, CRC(247521ef) SHA1(c6a04f514dd5ab40d8813dcfb8430bce54e7aa28) )
	ROM_LOAD16_WORD_SWAP( "nff.04",  0x080000, 0x80000, CRC(dba1c66e) SHA1(4764e77d4da5d19d9acded27df1e1bcba06b0fcf) )
	ROM_LOAD16_WORD_SWAP( "nffj.05", 0x100000, 0x80000, CRC(7f20c2ef) SHA1(380dc54d94c29c049a4c00ed58013e04eec87086) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
/*	ROM_LOAD16_WORD_SWAP( "nffjx.03", 0x000000, 0x80000, NO_DUMP )*/

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "nff.13",   0x0000000, 0x400000, CRC(c9fca741) SHA1(1781d4fc18b6d6f79b7b39d9bcace750fb61a5cf) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "nff.15",   0x0000002, 0x400000, CRC(f809d898) SHA1(a0b6af49e1780678d808c317b875161cedddb314) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "nff.17",   0x0000004, 0x400000, CRC(15ba4507) SHA1(bed6a82bf1dc1aa501d4c2d098115a15e18d446a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "nff.19",   0x0000006, 0x400000, CRC(3dd41b8c) SHA1(676078baad789e25f6e5a79de29672587be7ff00) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "nff.14",   0x1000000, 0x100000, CRC(3fe3a54b) SHA1(0a8e5cae141d24fd8b3cb11796c44728b0acd69e) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "nff.16",   0x1000002, 0x100000, CRC(565cd231) SHA1(0aecd433fb4ca2de1aca9fbb1e314fb1f6979321) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "nff.18",   0x1000004, 0x100000, CRC(63ca5988) SHA1(30137fa77573c84bcc24570bccb7dba61ddb413c) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "nff.20",   0x1000006, 0x100000, CRC(21eb8f3b) SHA1(efa69f19a958047dd91a294c88857ed3133fcbef) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "nff.01",   0x00000, 0x08000, CRC(d2e44318) SHA1(33e45f6fe9fed098a4c072b8c39406aef1a949b2) )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "nff.11",   0x000000, 0x400000, CRC(243e4e05) SHA1(83281f7290ac105a3f9a7507cbc11317d45ba706) )
	ROM_LOAD16_WORD_SWAP( "nff.12",   0x400000, 0x400000, CRC(4fcf1600) SHA1(36f18c5d92b79433bdf7088b29a244708929d48e) )
ROM_END

ROM_START( 19xx )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "19xe.03b",     0x000000, 0x080000, CRC(9bf9d9b1) SHA1(df32f2e6b3ebafd892368430d786a58e3614ef80) )
	ROM_LOAD16_WORD_SWAP( "19xe.04b",     0x080000, 0x080000, CRC(9aa46476) SHA1(dd677b87620198c22738e9f54105998a761298de) )
	ROM_LOAD16_WORD_SWAP( "19xe.05b",     0x100000, 0x080000, CRC(a04a2c5e) SHA1(aff5a324daa4641bfdef486824127010e4b803f7) )
	ROM_LOAD16_WORD_SWAP( "19xe.06b",     0x180000, 0x080000, CRC(f0a81c33) SHA1(8c44ce3fe5f7bafb3087fd5d1217697fb965886f) )
	ROM_LOAD16_WORD_SWAP( "19x.07",       0x200000, 0x080000, CRC(61c0296c) SHA1(9e225beccffd14bb53a32f8c0f2aef7f331dae30) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROM_LOAD64_WORD( "19x.13m",   0x0000000, 0x080000, CRC(427aeb18) SHA1(901029b5423e4bda85f592735036c06b7d426680) )
	ROM_LOAD64_WORD( "19x.15m",   0x0000002, 0x080000, CRC(63bdbf54) SHA1(9beb64ef0a8c92490848599d5d979bf42532609d) )
	ROM_LOAD64_WORD( "19x.17m",   0x0000004, 0x080000, CRC(2dfe18b5) SHA1(8a44364d9af6b9e1664b44b9235dc172182c9eb8) )
	ROM_LOAD64_WORD( "19x.19m",   0x0000006, 0x080000, CRC(cbef9579) SHA1(172413f220b242411218c7865e04014ec6417537) )
	ROM_LOAD64_WORD( "19x.14m",   0x0800000, 0x200000, CRC(e916967c) SHA1(3f937022166149a80585f91388de521055ca88ca) )
	ROM_LOAD64_WORD( "19x.16m",   0x0800002, 0x200000, CRC(6e75f3db) SHA1(4e1c8466eaa612102d0807d2e8bf1004e97476ea) )
	ROM_LOAD64_WORD( "19x.18m",   0x0800004, 0x200000, CRC(2213e798) SHA1(b1a9d5547f3f6c3ab59e8b761d224793c6ca33cb) )
	ROM_LOAD64_WORD( "19x.20m",   0x0800006, 0x200000, CRC(ab9d5b96) SHA1(52b755da401fde90c13181b02ab33e5e4b2aa1f7) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "19x.01",   0x00000, 0x08000, CRC(ef55195e) SHA1(813f465f2d392f6abeadbf661c54cf51171fa006) )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "19x.11m",   0x000000, 0x200000, CRC(d38beef3) SHA1(134e961b926a97cca5e45d3558efb98f6f278e08) )
	ROM_LOAD16_WORD_SWAP( "19x.12m",   0x200000, 0x200000, CRC(d47c96e2) SHA1(3c1b5563f8e7ee1c450b3592fcb319e928caec3c) )

	ROM_REGION( 0x20, REGION_USER5, 0 )
	ROM_LOAD( "19xx.key",     0x000000, 0x000014, CRC(6f5d6406) SHA1(f5d9b871de743da952fca33f059b4481e3eb6b89) )
ROM_END

ROM_START( 19xxu )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "19xu.03", 0x000000, 0x80000, CRC(05955268) SHA1(d3b6b416f1f9eb1c1cbca6647630d1155647082d) )
	ROM_LOAD16_WORD_SWAP( "19xu.04", 0x080000, 0x80000, CRC(3111ab7f) SHA1(8bbce20ae7ba47949f4939b2f35014fb6decd283) )
	ROM_LOAD16_WORD_SWAP( "19xu.05", 0x100000, 0x80000, CRC(38df4a63) SHA1(1303f7ab6296f1454907a24d64878bdbd1ef88a7) )
	ROM_LOAD16_WORD_SWAP( "19xu.06", 0x180000, 0x80000, CRC(5c7e60d3) SHA1(26bf0936962051be871d7a7776cf78abfca5b5ee) )
	ROM_LOAD16_WORD_SWAP( "19x.07",  0x200000, 0x80000, CRC(61c0296c) SHA1(9e225beccffd14bb53a32f8c0f2aef7f331dae30) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROM_LOAD64_WORD( "19x.13m",   0x0000000, 0x080000, CRC(427aeb18) SHA1(901029b5423e4bda85f592735036c06b7d426680) )
	ROM_LOAD64_WORD( "19x.15m",   0x0000002, 0x080000, CRC(63bdbf54) SHA1(9beb64ef0a8c92490848599d5d979bf42532609d) )
	ROM_LOAD64_WORD( "19x.17m",   0x0000004, 0x080000, CRC(2dfe18b5) SHA1(8a44364d9af6b9e1664b44b9235dc172182c9eb8) )
	ROM_LOAD64_WORD( "19x.19m",   0x0000006, 0x080000, CRC(cbef9579) SHA1(172413f220b242411218c7865e04014ec6417537) )
	ROM_LOAD64_WORD( "19x.14m",   0x0800000, 0x200000, CRC(e916967c) SHA1(3f937022166149a80585f91388de521055ca88ca) )
	ROM_LOAD64_WORD( "19x.16m",   0x0800002, 0x200000, CRC(6e75f3db) SHA1(4e1c8466eaa612102d0807d2e8bf1004e97476ea) )
	ROM_LOAD64_WORD( "19x.18m",   0x0800004, 0x200000, CRC(2213e798) SHA1(b1a9d5547f3f6c3ab59e8b761d224793c6ca33cb) )
	ROM_LOAD64_WORD( "19x.20m",   0x0800006, 0x200000, CRC(ab9d5b96) SHA1(52b755da401fde90c13181b02ab33e5e4b2aa1f7) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "19x.01",   0x00000, 0x08000, CRC(ef55195e) SHA1(813f465f2d392f6abeadbf661c54cf51171fa006) )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "19x.11m",   0x000000, 0x200000, CRC(d38beef3) SHA1(134e961b926a97cca5e45d3558efb98f6f278e08) )
	ROM_LOAD16_WORD_SWAP( "19x.12m",   0x200000, 0x200000, CRC(d47c96e2) SHA1(3c1b5563f8e7ee1c450b3592fcb319e928caec3c) )

	ROM_REGION( 0x20, REGION_USER5, 0 )
	ROM_LOAD( "19xxu.key",    0x000000, 0x000014, CRC(77e67ba1) SHA1(8c995bd00941ffe0145cb80e4f105ef8ef0a0001) )
ROM_END

ROM_START( 19xxj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "19xj-03b.6a", 0x000000, 0x80000, CRC(bcad93dd) SHA1(9b08cfdf278fc0cf60827518c4fecb99f224578b) )
	ROM_LOAD16_WORD_SWAP( "19xj-04b.7a", 0x080000, 0x80000, CRC(931882a1) SHA1(940a4fe52c1eb208071ab9d0a0c256eb39620091) )
	ROM_LOAD16_WORD_SWAP( "19xj-05b.8a", 0x100000, 0x80000, CRC(e7eeddc4) SHA1(40c79bc454d5fb8e5004db3df4fdcb10fae6c1e9) )
	ROM_LOAD16_WORD_SWAP( "19xj-06b.9a", 0x180000, 0x80000, CRC(f27cd6b8) SHA1(7d38660703707382120e6af770543ff50d8190f6) )
	ROM_LOAD16_WORD_SWAP( "19xj-07.6d",  0x200000, 0x80000, CRC(61c0296c) SHA1(9e225beccffd14bb53a32f8c0f2aef7f331dae30))

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROM_LOAD64_WORD( "19x-69.4j",   0x0000000, 0x080000, CRC(427aeb18) SHA1(901029b5423e4bda85f592735036c06b7d426680) )
	ROM_LOAD64_WORD( "19x-59.4d",   0x0000002, 0x080000, CRC(63bdbf54) SHA1(9beb64ef0a8c92490848599d5d979bf42532609d) )
	ROM_LOAD64_WORD( "19x-79.4m",   0x0000004, 0x080000, CRC(2dfe18b5) SHA1(8a44364d9af6b9e1664b44b9235dc172182c9eb8) )
	ROM_LOAD64_WORD( "19x-89.4p",   0x0000006, 0x080000, CRC(cbef9579) SHA1(172413f220b242411218c7865e04014ec6417537) )
	ROM_LOAD64_WORD( "19x-73.8j",   0x0800000, 0x080000, CRC(8e81f595) SHA1(221016c97300b253301ad4da568ed912e3da6c24) )
	ROM_LOAD64_WORD( "19x-74.9j",   0x0a00000, 0x080000, CRC(6d7ad22e) SHA1(3280f2849361b443c64ca3de4e77390fee4c047a) )
	ROM_LOAD64_WORD( "19x-75.10j",  0x0c00000, 0x080000, CRC(cb1a1b6a) SHA1(e2b30956684c89525bbc3fed841ba839ca732e21) )
	ROM_LOAD64_WORD( "19x-76.11j",  0x0e00000, 0x080000, CRC(26fc2b08) SHA1(4d962e1172044c1996139cfd56cc6c00ee2137d5) )
	ROM_LOAD64_WORD( "19x-63.8d",   0x0800002, 0x080000, CRC(6f8b045e) SHA1(41bc45c89c529011f755b6805ad8bad1a1f5e5e6) )
	ROM_LOAD64_WORD( "19x-64.9d",   0x0a00002, 0x080000, CRC(ccd5725a) SHA1(20d599ff61632e44a0143957572cb74b934a4aef) )
	ROM_LOAD64_WORD( "19x-65.10d",  0x0c00002, 0x080000, CRC(6cf6db35) SHA1(4b28b61cb01c81a24b0aea3bafac049b50338515) )
	ROM_LOAD64_WORD( "19x-66.11d",  0x0e00002, 0x080000, CRC(16115dd3) SHA1(e83886dba35cf31c956cd084141c3ae4078b3b72) )
	ROM_LOAD64_WORD( "19x-83.8m",   0x0800004, 0x080000, CRC(c11f88c1) SHA1(3118843063a9caaf9c8627ca2adedcce437cf8d5) )
	ROM_LOAD64_WORD( "19x-84.9m",   0x0a00004, 0x080000, CRC(68cc9cd8) SHA1(acb530475e3c66b7e46445332d93fcf6b6058cd2) )
	ROM_LOAD64_WORD( "19x-85.10m",  0x0c00004, 0x080000, CRC(f213666b) SHA1(33ede06b46022fc348b9b81530892d132071da0d) )
	ROM_LOAD64_WORD( "19x-86.11m",  0x0e00004, 0x080000, CRC(574e0473) SHA1(6ac52750c1e4ddc0ed8fbe5ace2e827944aa9275) )
	ROM_LOAD64_WORD( "19x-93.8p",   0x0800006, 0x080000, CRC(9fad3c55) SHA1(9774d015ae417acbcf14c1c84eabd754a299cd50) )
	ROM_LOAD64_WORD( "19x-94.9p",   0x0a00006, 0x080000, CRC(e10e252c) SHA1(6c4acc7f8b3f8f5df3768b2fabaf160502c17573) )
	ROM_LOAD64_WORD( "19x-95.10p",  0x0c00006, 0x080000, CRC(2b86fa67) SHA1(96a3cb8b203738fff33e2ca6340fdef928a816b9) )
	ROM_LOAD64_WORD( "19x-96.11p",  0x0e00006, 0x080000, CRC(ae6eb692) SHA1(4b564a375a08872aea534635ec526cce62dcdadd) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "19x-01.1a",   0x00000, 0x08000, CRC(ef55195e) SHA1(813f465f2d392f6abeadbf661c54cf51171fa006) )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "19x-51.6a",   0x000000, 0x080000, CRC(e9cd7780) SHA1(23c3d3968c2aec01ca25338b687be14407580b32) )
	ROM_LOAD16_WORD_SWAP( "19x-52.7a",   0x080000, 0x080000, CRC(b27b91a8) SHA1(1fd424c118fabcdb70ea025804552e99b8f8348a) )
	ROM_LOAD16_WORD_SWAP( "19x-53.8a",   0x100000, 0x080000, CRC(2e563ee2) SHA1(ed91404f27e36ff7c4bf4c297e81c4ebeb125db7) )
	ROM_LOAD16_WORD_SWAP( "19x-54.9a",   0x180000, 0x080000, CRC(f47c1f24) SHA1(3c2ae88946f86288886a901310165f5885b2d878) )
	ROM_LOAD16_WORD_SWAP( "19x-55.10a",  0x200000, 0x080000, CRC(0b1af6e0) SHA1(b889e58c8b223c38858680c5e2d0bab5cfa323da) )
	ROM_LOAD16_WORD_SWAP( "19x-56.11a",  0x280000, 0x080000, CRC(dfa8819f) SHA1(b5112ced4632c888aee0ca6c1bc4b5097bbdabda) )
	ROM_LOAD16_WORD_SWAP( "19x-57.12a",  0x300000, 0x080000, CRC(229ba777) SHA1(2506b967689697fdb4e43d8b01fc9b564641a70d) )
	ROM_LOAD16_WORD_SWAP( "19x-58.13a",  0x380000, 0x080000, CRC(c7dceba4) SHA1(6a2684bc9738a3f6d071d72ffa678316a029f1c5) )

	ROM_REGION( 0x20, REGION_USER5, 0 )
	ROM_LOAD( "19xxj.key",    0x000000, 0x000014, CRC(9aafa71a) SHA1(82188cc69e59d5ce86d0e178cf6d9f8f04da0633) )
ROM_END

ROM_START( 19xxjr1 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "19xj.03a", 0x000000, 0x80000, CRC(ed08bdd1) SHA1(4b49f988faf4a6a99d3596bb12e4685862a20a3e) )
	ROM_LOAD16_WORD_SWAP( "19xj.04a", 0x080000, 0x80000, CRC(fb8e3f29) SHA1(513b85bfe5b86692faa4d20b755fe261a7f95bfc) )
	ROM_LOAD16_WORD_SWAP( "19xj.05a", 0x100000, 0x80000, CRC(aa508ac4) SHA1(c906f4a92872f4ecda662146690acbe5165ae79e) )
	ROM_LOAD16_WORD_SWAP( "19xj.06a", 0x180000, 0x80000, CRC(ff2d785b) SHA1(9294fb3ed378ecc9dccdeff05df09cbb4eeaa9b3) )
	ROM_LOAD16_WORD_SWAP( "19x.07",   0x200000, 0x80000, CRC(61c0296c) SHA1(9e225beccffd14bb53a32f8c0f2aef7f331dae30) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROM_LOAD64_WORD( "19x.13m",   0x0000000, 0x080000, CRC(427aeb18) SHA1(901029b5423e4bda85f592735036c06b7d426680) )
	ROM_LOAD64_WORD( "19x.15m",   0x0000002, 0x080000, CRC(63bdbf54) SHA1(9beb64ef0a8c92490848599d5d979bf42532609d) )
	ROM_LOAD64_WORD( "19x.17m",   0x0000004, 0x080000, CRC(2dfe18b5) SHA1(8a44364d9af6b9e1664b44b9235dc172182c9eb8) )
	ROM_LOAD64_WORD( "19x.19m",   0x0000006, 0x080000, CRC(cbef9579) SHA1(172413f220b242411218c7865e04014ec6417537) )
	ROM_LOAD64_WORD( "19x.14m",   0x0800000, 0x200000, CRC(e916967c) SHA1(3f937022166149a80585f91388de521055ca88ca) )
	ROM_LOAD64_WORD( "19x.16m",   0x0800002, 0x200000, CRC(6e75f3db) SHA1(4e1c8466eaa612102d0807d2e8bf1004e97476ea) )
	ROM_LOAD64_WORD( "19x.18m",   0x0800004, 0x200000, CRC(2213e798) SHA1(b1a9d5547f3f6c3ab59e8b761d224793c6ca33cb) )
	ROM_LOAD64_WORD( "19x.20m",   0x0800006, 0x200000, CRC(ab9d5b96) SHA1(52b755da401fde90c13181b02ab33e5e4b2aa1f7) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "19x.01",   0x00000, 0x08000, CRC(ef55195e) SHA1(813f465f2d392f6abeadbf661c54cf51171fa006) )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "19x.11m",   0x000000, 0x200000, CRC(d38beef3) SHA1(134e961b926a97cca5e45d3558efb98f6f278e08) )
	ROM_LOAD16_WORD_SWAP( "19x.12m",   0x200000, 0x200000, CRC(d47c96e2) SHA1(3c1b5563f8e7ee1c450b3592fcb319e928caec3c) )

	ROM_REGION( 0x20, REGION_USER5, 0 )
	ROM_LOAD( "19xxj.key",    0x000000, 0x000014, CRC(9aafa71a) SHA1(82188cc69e59d5ce86d0e178cf6d9f8f04da0633) )
ROM_END

ROM_START( 19xxjr2 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "19xj.03", 0x000000, 0x80000, CRC(26a381ed) SHA1(9a6bd9a8c152096f653c0b5a161dd08314fdb5e7) )
	ROM_LOAD16_WORD_SWAP( "19xj.04", 0x080000, 0x80000, CRC(30100cca) SHA1(3fc964e6daffa5dd7b9f72c8ace3a4b9d515e9ce) )
	ROM_LOAD16_WORD_SWAP( "19xj.05", 0x100000, 0x80000, CRC(de67e938) SHA1(5f977c07c6ffa816ccfa2c7bab8a77b64c232610) )
	ROM_LOAD16_WORD_SWAP( "19xj.06", 0x180000, 0x80000, CRC(39f9a409) SHA1(45799204d2400a591c526f8c750e4728701372bf) )
	ROM_LOAD16_WORD_SWAP( "19x.07",  0x200000, 0x80000, CRC(61c0296c) SHA1(9e225beccffd14bb53a32f8c0f2aef7f331dae30) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROM_LOAD64_WORD( "19x.13m",   0x0000000, 0x080000, CRC(427aeb18) SHA1(901029b5423e4bda85f592735036c06b7d426680) )
	ROM_LOAD64_WORD( "19x.15m",   0x0000002, 0x080000, CRC(63bdbf54) SHA1(9beb64ef0a8c92490848599d5d979bf42532609d) )
	ROM_LOAD64_WORD( "19x.17m",   0x0000004, 0x080000, CRC(2dfe18b5) SHA1(8a44364d9af6b9e1664b44b9235dc172182c9eb8) )
	ROM_LOAD64_WORD( "19x.19m",   0x0000006, 0x080000, CRC(cbef9579) SHA1(172413f220b242411218c7865e04014ec6417537) )
	ROM_LOAD64_WORD( "19x.14m",   0x0800000, 0x200000, CRC(e916967c) SHA1(3f937022166149a80585f91388de521055ca88ca) )
	ROM_LOAD64_WORD( "19x.16m",   0x0800002, 0x200000, CRC(6e75f3db) SHA1(4e1c8466eaa612102d0807d2e8bf1004e97476ea) )
	ROM_LOAD64_WORD( "19x.18m",   0x0800004, 0x200000, CRC(2213e798) SHA1(b1a9d5547f3f6c3ab59e8b761d224793c6ca33cb) )
	ROM_LOAD64_WORD( "19x.20m",   0x0800006, 0x200000, CRC(ab9d5b96) SHA1(52b755da401fde90c13181b02ab33e5e4b2aa1f7) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "19x.01",   0x00000, 0x08000, CRC(ef55195e) SHA1(813f465f2d392f6abeadbf661c54cf51171fa006) )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "19x.11m",   0x000000, 0x200000, CRC(d38beef3) SHA1(134e961b926a97cca5e45d3558efb98f6f278e08) )
	ROM_LOAD16_WORD_SWAP( "19x.12m",   0x200000, 0x200000, CRC(d47c96e2) SHA1(3c1b5563f8e7ee1c450b3592fcb319e928caec3c) )

	ROM_REGION( 0x20, REGION_USER5, 0 )
	ROM_LOAD( "19xxj.key",    0x000000, 0x000014, CRC(9aafa71a) SHA1(82188cc69e59d5ce86d0e178cf6d9f8f04da0633) )
ROM_END

ROM_START( 19xxa )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "09xa.03b", 0x000000, 0x80000, CRC(2e994897) SHA1(8dee5d84fd01f416fa6dfef76be0dfff90f45595) ) /* Yes it's actually 09xa, that's not a typo */
	ROM_LOAD16_WORD_SWAP( "09xa.04b", 0x080000, 0x80000, CRC(6364d001) SHA1(a6f3f0015c9a3c27bc2109d557b88d0773a83465) ) /* Yes it's actually 09xa, that's not a typo */
	ROM_LOAD16_WORD_SWAP( "09xa.05b", 0x100000, 0x80000, CRC(00c1949b) SHA1(4120a57d06638c637b74442b200e46d9b13fa431) ) /* Yes it's actually 09xa, that's not a typo */
	ROM_LOAD16_WORD_SWAP( "09xa.06b", 0x180000, 0x80000, CRC(363c1f6e) SHA1(2e358e345f1d5963836b0ac8c6a1b455638bd19d) ) /* Yes it's actually 09xa, that's not a typo */
	ROM_LOAD16_WORD_SWAP( "19xa.07",  0x200000, 0x80000, CRC(61c0296c) SHA1(9e225beccffd14bb53a32f8c0f2aef7f331dae30) ) /* This one was different, it actually was 19xa */

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROM_LOAD64_WORD( "19x.13m",   0x0000000, 0x080000, CRC(427aeb18) SHA1(901029b5423e4bda85f592735036c06b7d426680) )
	ROM_LOAD64_WORD( "19x.15m",   0x0000002, 0x080000, CRC(63bdbf54) SHA1(9beb64ef0a8c92490848599d5d979bf42532609d) )
	ROM_LOAD64_WORD( "19x.17m",   0x0000004, 0x080000, CRC(2dfe18b5) SHA1(8a44364d9af6b9e1664b44b9235dc172182c9eb8) )
	ROM_LOAD64_WORD( "19x.19m",   0x0000006, 0x080000, CRC(cbef9579) SHA1(172413f220b242411218c7865e04014ec6417537) )
	ROM_LOAD64_WORD( "19x.14m",   0x0800000, 0x200000, CRC(e916967c) SHA1(3f937022166149a80585f91388de521055ca88ca) )
	ROM_LOAD64_WORD( "19x.16m",   0x0800002, 0x200000, CRC(6e75f3db) SHA1(4e1c8466eaa612102d0807d2e8bf1004e97476ea) )
	ROM_LOAD64_WORD( "19x.18m",   0x0800004, 0x200000, CRC(2213e798) SHA1(b1a9d5547f3f6c3ab59e8b761d224793c6ca33cb) )
	ROM_LOAD64_WORD( "19x.20m",   0x0800006, 0x200000, CRC(ab9d5b96) SHA1(52b755da401fde90c13181b02ab33e5e4b2aa1f7) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "19x.01",   0x00000, 0x08000, CRC(ef55195e) SHA1(813f465f2d392f6abeadbf661c54cf51171fa006) )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "19x.11m",   0x000000, 0x200000, CRC(d38beef3) SHA1(134e961b926a97cca5e45d3558efb98f6f278e08) )
	ROM_LOAD16_WORD_SWAP( "19x.12m",   0x200000, 0x200000, CRC(d47c96e2) SHA1(3c1b5563f8e7ee1c450b3592fcb319e928caec3c) )

	ROM_REGION( 0x20, REGION_USER5, 0 )
	ROM_LOAD( "19xxa.key",    0x000000, 0x000014, CRC(2cd32eb9) SHA1(3c2df0e93175c2e25faae2ab3e062ac5d79b35a2) )
ROM_END

ROM_START( 19xxar1 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "19xa.03", 0x000000, 0x80000, CRC(0c20fd50) SHA1(3aeb698ac67e6c8d0224e68d9258ef45f735432a) )
	ROM_LOAD16_WORD_SWAP( "19xa.04", 0x080000, 0x80000, CRC(1fc37508) SHA1(f4b858b5dc6243c5cd432d1a72d828831c8eca6f) )
	ROM_LOAD16_WORD_SWAP( "19xa.05", 0x100000, 0x80000, CRC(6c9cc4ed) SHA1(2b01ffe0bba41640ffc0c13dfdacf3cf0e3e131d) )
	ROM_LOAD16_WORD_SWAP( "19xa.06", 0x180000, 0x80000, CRC(ca5b9f76) SHA1(961aed25cb445722de5001ba687dbe85b80cba29) )
	ROM_LOAD16_WORD_SWAP( "19x.07",  0x200000, 0x80000, CRC(61c0296c) SHA1(9e225beccffd14bb53a32f8c0f2aef7f331dae30) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROM_LOAD64_WORD( "19x.13m",   0x0000000, 0x080000, CRC(427aeb18) SHA1(901029b5423e4bda85f592735036c06b7d426680) )
	ROM_LOAD64_WORD( "19x.15m",   0x0000002, 0x080000, CRC(63bdbf54) SHA1(9beb64ef0a8c92490848599d5d979bf42532609d) )
	ROM_LOAD64_WORD( "19x.17m",   0x0000004, 0x080000, CRC(2dfe18b5) SHA1(8a44364d9af6b9e1664b44b9235dc172182c9eb8) )
	ROM_LOAD64_WORD( "19x.19m",   0x0000006, 0x080000, CRC(cbef9579) SHA1(172413f220b242411218c7865e04014ec6417537) )
	ROM_LOAD64_WORD( "19x.14m",   0x0800000, 0x200000, CRC(e916967c) SHA1(3f937022166149a80585f91388de521055ca88ca) )
	ROM_LOAD64_WORD( "19x.16m",   0x0800002, 0x200000, CRC(6e75f3db) SHA1(4e1c8466eaa612102d0807d2e8bf1004e97476ea) )
	ROM_LOAD64_WORD( "19x.18m",   0x0800004, 0x200000, CRC(2213e798) SHA1(b1a9d5547f3f6c3ab59e8b761d224793c6ca33cb) )
	ROM_LOAD64_WORD( "19x.20m",   0x0800006, 0x200000, CRC(ab9d5b96) SHA1(52b755da401fde90c13181b02ab33e5e4b2aa1f7) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "19x.01",   0x00000, 0x08000, CRC(ef55195e) SHA1(813f465f2d392f6abeadbf661c54cf51171fa006) )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "19x.11m",   0x000000, 0x200000, CRC(d38beef3) SHA1(134e961b926a97cca5e45d3558efb98f6f278e08) )
	ROM_LOAD16_WORD_SWAP( "19x.12m",   0x200000, 0x200000, CRC(d47c96e2) SHA1(3c1b5563f8e7ee1c450b3592fcb319e928caec3c) )

	ROM_REGION( 0x20, REGION_USER5, 0 )
	ROM_LOAD( "19xxa.key",    0x000000, 0x000014, CRC(2cd32eb9) SHA1(3c2df0e93175c2e25faae2ab3e062ac5d79b35a2) )
ROM_END

ROM_START( 19xxh )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "19xh.03a", 0x000000, 0x80000, CRC(357be2ac) SHA1(660641d8cd2f7b574809badf99924f0a31a0cccd) )
	ROM_LOAD16_WORD_SWAP( "19xh.04a", 0x080000, 0x80000, CRC(bb13ea3b) SHA1(3ae0fa09ae031e2a0f1ea8645a9baced44289383) )
	ROM_LOAD16_WORD_SWAP( "19xh.05a", 0x100000, 0x80000, CRC(cbd76601) SHA1(a6b64e5f4b35a120dc463a6c9e98e2ec8e739e59) )
	ROM_LOAD16_WORD_SWAP( "19xh.06a", 0x180000, 0x80000, CRC(b362de8b) SHA1(0383a44efbfccdc78637995ed4f99740ef96cbad) )
	ROM_LOAD16_WORD_SWAP( "19x.07",   0x200000, 0x80000, CRC(61c0296c) SHA1(9e225beccffd14bb53a32f8c0f2aef7f331dae30) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROM_LOAD64_WORD( "19x.13m",   0x0000000, 0x080000, CRC(427aeb18) SHA1(901029b5423e4bda85f592735036c06b7d426680) )
	ROM_LOAD64_WORD( "19x.15m",   0x0000002, 0x080000, CRC(63bdbf54) SHA1(9beb64ef0a8c92490848599d5d979bf42532609d) )
	ROM_LOAD64_WORD( "19x.17m",   0x0000004, 0x080000, CRC(2dfe18b5) SHA1(8a44364d9af6b9e1664b44b9235dc172182c9eb8) )
	ROM_LOAD64_WORD( "19x.19m",   0x0000006, 0x080000, CRC(cbef9579) SHA1(172413f220b242411218c7865e04014ec6417537) )
	ROM_LOAD64_WORD( "19x.14m",   0x0800000, 0x200000, CRC(e916967c) SHA1(3f937022166149a80585f91388de521055ca88ca) )
	ROM_LOAD64_WORD( "19x.16m",   0x0800002, 0x200000, CRC(6e75f3db) SHA1(4e1c8466eaa612102d0807d2e8bf1004e97476ea) )
	ROM_LOAD64_WORD( "19x.18m",   0x0800004, 0x200000, CRC(2213e798) SHA1(b1a9d5547f3f6c3ab59e8b761d224793c6ca33cb) )
	ROM_LOAD64_WORD( "19x.20m",   0x0800006, 0x200000, CRC(ab9d5b96) SHA1(52b755da401fde90c13181b02ab33e5e4b2aa1f7) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "19x.01",   0x00000, 0x08000, CRC(ef55195e) SHA1(813f465f2d392f6abeadbf661c54cf51171fa006) )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "19x.11m",   0x000000, 0x200000, CRC(d38beef3) SHA1(134e961b926a97cca5e45d3558efb98f6f278e08) )
	ROM_LOAD16_WORD_SWAP( "19x.12m",   0x200000, 0x200000, CRC(d47c96e2) SHA1(3c1b5563f8e7ee1c450b3592fcb319e928caec3c) )

	ROM_REGION( 0x20, REGION_USER5, 0 )
	ROM_LOAD( "19xxh.key",    0x000000, 0x000014, CRC(215cf208) SHA1(86b83775d0bb76a21d12be81d4dcdeb17d7787ef) )
ROM_END

ROM_START( 19xxb )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "19xb.03a", 0x000000, 0x80000, CRC(341bdf4a) SHA1(e7deccd034f2a2664507dbb7fed9757c2424dbf7) )
	ROM_LOAD16_WORD_SWAP( "19xb.04a", 0x080000, 0x80000, CRC(dff8069e) SHA1(3b31d1f66680cee1da3f3d3fd822739e99f48ccd) )
	ROM_LOAD16_WORD_SWAP( "19xb.05a", 0x100000, 0x80000, CRC(a47a92a8) SHA1(20254ec70029ec027793d4fbd9a7067c2a756315) )
	ROM_LOAD16_WORD_SWAP( "19xb.06a", 0x180000, 0x80000, CRC(c52df10d) SHA1(88707c4d1ec8649a7e10ec60e5bbc6f5ffb26f73) )
	ROM_LOAD16_WORD_SWAP( "19x.07",   0x200000, 0x80000, CRC(61c0296c) SHA1(9e225beccffd14bb53a32f8c0f2aef7f331dae30) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROM_LOAD64_WORD( "19x.13m",   0x0000000, 0x080000, CRC(427aeb18) SHA1(901029b5423e4bda85f592735036c06b7d426680) )
	ROM_LOAD64_WORD( "19x.15m",   0x0000002, 0x080000, CRC(63bdbf54) SHA1(9beb64ef0a8c92490848599d5d979bf42532609d) )
	ROM_LOAD64_WORD( "19x.17m",   0x0000004, 0x080000, CRC(2dfe18b5) SHA1(8a44364d9af6b9e1664b44b9235dc172182c9eb8) )
	ROM_LOAD64_WORD( "19x.19m",   0x0000006, 0x080000, CRC(cbef9579) SHA1(172413f220b242411218c7865e04014ec6417537) )
	ROM_LOAD64_WORD( "19x.14m",   0x0800000, 0x200000, CRC(e916967c) SHA1(3f937022166149a80585f91388de521055ca88ca) )
	ROM_LOAD64_WORD( "19x.16m",   0x0800002, 0x200000, CRC(6e75f3db) SHA1(4e1c8466eaa612102d0807d2e8bf1004e97476ea) )
	ROM_LOAD64_WORD( "19x.18m",   0x0800004, 0x200000, CRC(2213e798) SHA1(b1a9d5547f3f6c3ab59e8b761d224793c6ca33cb) )
	ROM_LOAD64_WORD( "19x.20m",   0x0800006, 0x200000, CRC(ab9d5b96) SHA1(52b755da401fde90c13181b02ab33e5e4b2aa1f7) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "19x.01",   0x00000, 0x08000, CRC(ef55195e) SHA1(813f465f2d392f6abeadbf661c54cf51171fa006) )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "19x.11m",   0x000000, 0x200000, CRC(d38beef3) SHA1(134e961b926a97cca5e45d3558efb98f6f278e08) )
	ROM_LOAD16_WORD_SWAP( "19x.12m",   0x200000, 0x200000, CRC(d47c96e2) SHA1(3c1b5563f8e7ee1c450b3592fcb319e928caec3c) )

	ROM_REGION( 0x20, REGION_USER5, 0 )
	ROM_LOAD( "19xxb.key",    0x000000, 0x000014, CRC(4200e334) SHA1(8adae391e2e62ca1cf3caece813bf7f7e9a1f164) )
ROM_END

ROM_START( armwar )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "pwge.03b", 0x000000, 0x80000, CRC(e822e3e9) SHA1(dcd153bb70f6c2baffa2f3687def30d85fca44ba) )
	ROM_LOAD16_WORD_SWAP( "pwge.04b", 0x080000, 0x80000, CRC(4f89de39) SHA1(1e54ed70a6ed9330ec83fb189f76e9417c6dfc13) )
	ROM_LOAD16_WORD_SWAP( "pwge.05a", 0x100000, 0x80000, CRC(83df24e5) SHA1(39801452769569c3271b26c6be8d3ce5e72b0629) )
	ROM_LOAD16_WORD_SWAP( "pwg.06",   0x180000, 0x80000, CRC(87a60ce8) SHA1(e2085c7c8c6792d055dbbb023c7f4e4aa38ae924) )
	ROM_LOAD16_WORD_SWAP( "pwg.07",   0x200000, 0x80000, CRC(f7b148df) SHA1(f369669713cf647222094c570a2eacd48a8637cf) )
	ROM_LOAD16_WORD_SWAP( "pwg.08",   0x280000, 0x80000, CRC(cc62823e) SHA1(edaf9bebdfc65ae5414090abd6844176eec39a00) )
	ROM_LOAD16_WORD_SWAP( "pwg.09",   0x300000, 0x80000, CRC(ddc85ca6) SHA1(e794c679531632e2142c6a5e3b858494389ce65e) )
	ROM_LOAD16_WORD_SWAP( "pwg.10",   0x380000, 0x80000, CRC(07c4fb28) SHA1(58a1ff3d105be7df833dd4f32973766649efcbcf) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "pwgex.03b", 0x000000, 0x80000, CRC(741fc2b0) SHA1(9bcaf48a753ca9c5a6a06485782c3d185aba3cef) )
	ROM_LOAD16_WORD_SWAP( "pwgex.04b", 0x080000, 0x80000, CRC(5bb96a5d) SHA1(3ac1bc6dec365cccdabb3edb365ff1ac52255aa9) )

	ROM_REGION( 0x1400000, REGION_GFX1, 0 )
	ROMX_LOAD( "pwg.13",   0x0000000, 0x400000, CRC(ae8fe08e) SHA1(b6f09663dcda69b5d7ac13e4afaf1efd692fb61e) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pwg.15",   0x0000002, 0x400000, CRC(db560f58) SHA1(0c3716b32eb24544ff5d16b5dcadce195cd10d00) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pwg.17",   0x0000004, 0x400000, CRC(bc475b94) SHA1(a157664450895a146a532581dd6f4b63dff21c86) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pwg.19",   0x0000006, 0x400000, CRC(07439ff7) SHA1(f71e07c6d77c32828f5e319268b24b13a1a4b0c2) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pwg.14",   0x1000000, 0x100000, CRC(c3f9ba63) SHA1(66191a52c39daa89b17ede5804ee41c028036f14) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pwg.16",   0x1000002, 0x100000, CRC(815b0e7b) SHA1(549785daac3122253fb94f6541bc7016147f5306) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pwg.18",   0x1000004, 0x100000, CRC(0109c71b) SHA1(eb51284ee0c85ff8f605fe1d166b7aa202be1344) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pwg.20",   0x1000006, 0x100000, CRC(eb75ffbe) SHA1(e9d1deca60be696ac5bff2017fb5de3525e5239a) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "pwg.01",   0x00000, 0x08000, CRC(18a5c0e4) SHA1(bb1353dd74884aaeec9b5f1d0b284d9cad53c0ff) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "pwg.02",   0x28000, 0x20000, CRC(c9dfffa6) SHA1(64e71028befe9a2514074be765dd020e1d2ea70b) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "pwg.11",   0x000000, 0x200000, CRC(a78f7433) SHA1(e47ffba7b9dac9d0dda985c5d966194be18260f7) )
	ROM_LOAD16_WORD_SWAP( "pwg.12",   0x200000, 0x200000, CRC(77438ed0) SHA1(733ca6c6a792e66e2aa12c5fc06dd459527afe4b) )
ROM_END

ROM_START( armwaru )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "pwgu.03b", 0x000000, 0x80000, CRC(8b95497a) SHA1(0c037b8a484d69f5e8c9600de71177fb78e9ede0) )
	ROM_LOAD16_WORD_SWAP( "pwgu.04b", 0x080000, 0x80000, CRC(29eb5661) SHA1(7ee9150072882c9e158ca8231f26a9f62c8fa50e) )
	ROM_LOAD16_WORD_SWAP( "pwgu.05b", 0x100000, 0x80000, CRC(a54e9e44) SHA1(e235dcdbd0111f018519d9c8eef130121ea20a20) )
	ROM_LOAD16_WORD_SWAP( "pwg.06",   0x180000, 0x80000, CRC(87a60ce8) SHA1(e2085c7c8c6792d055dbbb023c7f4e4aa38ae924) )
	ROM_LOAD16_WORD_SWAP( "pwg.07",   0x200000, 0x80000, CRC(f7b148df) SHA1(f369669713cf647222094c570a2eacd48a8637cf) )
	ROM_LOAD16_WORD_SWAP( "pwg.08",   0x280000, 0x80000, CRC(cc62823e) SHA1(edaf9bebdfc65ae5414090abd6844176eec39a00) )
	ROM_LOAD16_WORD_SWAP( "pwg.09a",  0x300000, 0x80000, CRC(4c26baee) SHA1(685f050206b9b904ce6a1ae9a8e8f019012cea43) )
	ROM_LOAD16_WORD_SWAP( "pwg.10",   0x380000, 0x80000, CRC(07c4fb28) SHA1(58a1ff3d105be7df833dd4f32973766649efcbcf) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "pwgux.03b", 0x000000, 0x80000, CRC(5d41ddde) SHA1(61ac83a74d0f54cea50d97bda584ba28fb0a278f) )
	ROM_LOAD16_WORD_SWAP( "pwgux.04b", 0x080000, 0x80000, CRC(4d0619f3) SHA1(ec89017df9a3f36e492c95bce664e65ed2a51e42) )

	ROM_REGION( 0x1400000, REGION_GFX1, 0 )
	ROMX_LOAD( "pwg.13",   0x0000000, 0x400000, CRC(ae8fe08e) SHA1(b6f09663dcda69b5d7ac13e4afaf1efd692fb61e) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pwg.15",   0x0000002, 0x400000, CRC(db560f58) SHA1(0c3716b32eb24544ff5d16b5dcadce195cd10d00) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pwg.17",   0x0000004, 0x400000, CRC(bc475b94) SHA1(a157664450895a146a532581dd6f4b63dff21c86) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pwg.19",   0x0000006, 0x400000, CRC(07439ff7) SHA1(f71e07c6d77c32828f5e319268b24b13a1a4b0c2) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pwg.14",   0x1000000, 0x100000, CRC(c3f9ba63) SHA1(66191a52c39daa89b17ede5804ee41c028036f14) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pwg.16",   0x1000002, 0x100000, CRC(815b0e7b) SHA1(549785daac3122253fb94f6541bc7016147f5306) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pwg.18",   0x1000004, 0x100000, CRC(0109c71b) SHA1(eb51284ee0c85ff8f605fe1d166b7aa202be1344) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pwg.20",   0x1000006, 0x100000, CRC(eb75ffbe) SHA1(e9d1deca60be696ac5bff2017fb5de3525e5239a) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "pwg.01",   0x00000, 0x08000, CRC(18a5c0e4) SHA1(bb1353dd74884aaeec9b5f1d0b284d9cad53c0ff) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "pwg.02",   0x28000, 0x20000, CRC(c9dfffa6) SHA1(64e71028befe9a2514074be765dd020e1d2ea70b) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "pwg.11",   0x000000, 0x200000, CRC(a78f7433) SHA1(e47ffba7b9dac9d0dda985c5d966194be18260f7) )
	ROM_LOAD16_WORD_SWAP( "pwg.12",   0x200000, 0x200000, CRC(77438ed0) SHA1(733ca6c6a792e66e2aa12c5fc06dd459527afe4b) )
ROM_END

ROM_START( pgear )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "pwgj.03a", 0x000000, 0x80000, CRC(c79c0c02) SHA1(4e24d34be255bb3886bd6b767779ee5fc81dca6e) )
	ROM_LOAD16_WORD_SWAP( "pwgj.04a", 0x080000, 0x80000, CRC(167c6ed8) SHA1(23a4a7faae817ffc6c5faa4db5b96b8c8c0dfe86) )
	ROM_LOAD16_WORD_SWAP( "pwgj.05a", 0x100000, 0x80000, CRC(a63fb400) SHA1(b27464b000cd12d9247254f843be27639fbf3a48) )
	ROM_LOAD16_WORD_SWAP( "pwg.06",   0x180000, 0x80000, CRC(87a60ce8) SHA1(e2085c7c8c6792d055dbbb023c7f4e4aa38ae924) )
	ROM_LOAD16_WORD_SWAP( "pwg.07",   0x200000, 0x80000, CRC(f7b148df) SHA1(f369669713cf647222094c570a2eacd48a8637cf) )
	ROM_LOAD16_WORD_SWAP( "pwg.08",   0x280000, 0x80000, CRC(cc62823e) SHA1(edaf9bebdfc65ae5414090abd6844176eec39a00) )
	ROM_LOAD16_WORD_SWAP( "pwg.09a",  0x300000, 0x80000, CRC(4c26baee) SHA1(685f050206b9b904ce6a1ae9a8e8f019012cea43) )
	ROM_LOAD16_WORD_SWAP( "pwg.10",   0x380000, 0x80000, CRC(07c4fb28) SHA1(58a1ff3d105be7df833dd4f32973766649efcbcf) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "pwgjx.03a", 0x000000, 0x80000, CRC(cf0284a9) SHA1(444d7788072be253bb7c4cecc487a26d5f61d73c) )
	ROM_LOAD16_WORD_SWAP( "pwgjx.04a", 0x080000, 0x80000, CRC(99437cf1) SHA1(f2b4ad733a2467e89c8d082291e330bc29411998) )

	ROM_REGION( 0x1400000, REGION_GFX1, 0 )
	ROMX_LOAD( "pwg.13",   0x0000000, 0x400000, CRC(ae8fe08e) SHA1(b6f09663dcda69b5d7ac13e4afaf1efd692fb61e) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pwg.15",   0x0000002, 0x400000, CRC(db560f58) SHA1(0c3716b32eb24544ff5d16b5dcadce195cd10d00) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pwg.17",   0x0000004, 0x400000, CRC(bc475b94) SHA1(a157664450895a146a532581dd6f4b63dff21c86) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pwg.19",   0x0000006, 0x400000, CRC(07439ff7) SHA1(f71e07c6d77c32828f5e319268b24b13a1a4b0c2) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pwg.14",   0x1000000, 0x100000, CRC(c3f9ba63) SHA1(66191a52c39daa89b17ede5804ee41c028036f14) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pwg.16",   0x1000002, 0x100000, CRC(815b0e7b) SHA1(549785daac3122253fb94f6541bc7016147f5306) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pwg.18",   0x1000004, 0x100000, CRC(0109c71b) SHA1(eb51284ee0c85ff8f605fe1d166b7aa202be1344) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pwg.20",   0x1000006, 0x100000, CRC(eb75ffbe) SHA1(e9d1deca60be696ac5bff2017fb5de3525e5239a) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "pwg.01",   0x00000, 0x08000, CRC(18a5c0e4) SHA1(bb1353dd74884aaeec9b5f1d0b284d9cad53c0ff) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "pwg.02",   0x28000, 0x20000, CRC(c9dfffa6) SHA1(64e71028befe9a2514074be765dd020e1d2ea70b) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "pwg.11",   0x000000, 0x200000, CRC(a78f7433) SHA1(e47ffba7b9dac9d0dda985c5d966194be18260f7) )
	ROM_LOAD16_WORD_SWAP( "pwg.12",   0x200000, 0x200000, CRC(77438ed0) SHA1(733ca6c6a792e66e2aa12c5fc06dd459527afe4b) )
ROM_END

ROM_START( pgearr1 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "pwgj.03", 0x000000, 0x80000, CRC(f264e74b) SHA1(db0a675a9d46df9227334259db633e27f7dc79ab) )
	ROM_LOAD16_WORD_SWAP( "pwgj.04", 0x080000, 0x80000, CRC(23a84983) SHA1(a3ed606f6213bb6e447c4ff84d6d3435a0170762) )
	ROM_LOAD16_WORD_SWAP( "pwgj.05", 0x100000, 0x80000, CRC(bef58c62) SHA1(178c255171c4010cec758ee11d96bdcee85abee0) )
	ROM_LOAD16_WORD_SWAP( "pwg.06",  0x180000, 0x80000, CRC(87a60ce8) SHA1(e2085c7c8c6792d055dbbb023c7f4e4aa38ae924) )
	ROM_LOAD16_WORD_SWAP( "pwg.07",  0x200000, 0x80000, CRC(f7b148df) SHA1(f369669713cf647222094c570a2eacd48a8637cf) )
	ROM_LOAD16_WORD_SWAP( "pwg.08",  0x280000, 0x80000, CRC(cc62823e) SHA1(edaf9bebdfc65ae5414090abd6844176eec39a00) )
	ROM_LOAD16_WORD_SWAP( "pwg.09",  0x300000, 0x80000, CRC(ddc85ca6) SHA1(e794c679531632e2142c6a5e3b858494389ce65e) )
	ROM_LOAD16_WORD_SWAP( "pwg.10",  0x380000, 0x80000, CRC(07c4fb28) SHA1(58a1ff3d105be7df833dd4f32973766649efcbcf) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "pwgjx.03", 0x000000, 0x80000, CRC(ebd9e371) SHA1(e4ed713ed924918d44ad8fdcbca7a805098b7892) )
	ROM_LOAD16_WORD_SWAP( "pwgjx.04", 0x080000, 0x80000, CRC(e05a4756) SHA1(ef29b663493f4101202df601134ffe61e2fa5edf) )

	ROM_REGION( 0x1400000, REGION_GFX1, 0 )
	ROMX_LOAD( "pwg.13",   0x0000000, 0x400000, CRC(ae8fe08e) SHA1(b6f09663dcda69b5d7ac13e4afaf1efd692fb61e) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pwg.15",   0x0000002, 0x400000, CRC(db560f58) SHA1(0c3716b32eb24544ff5d16b5dcadce195cd10d00) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pwg.17",   0x0000004, 0x400000, CRC(bc475b94) SHA1(a157664450895a146a532581dd6f4b63dff21c86) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pwg.19",   0x0000006, 0x400000, CRC(07439ff7) SHA1(f71e07c6d77c32828f5e319268b24b13a1a4b0c2) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pwg.14",   0x1000000, 0x100000, CRC(c3f9ba63) SHA1(66191a52c39daa89b17ede5804ee41c028036f14) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pwg.16",   0x1000002, 0x100000, CRC(815b0e7b) SHA1(549785daac3122253fb94f6541bc7016147f5306) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pwg.18",   0x1000004, 0x100000, CRC(0109c71b) SHA1(eb51284ee0c85ff8f605fe1d166b7aa202be1344) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pwg.20",   0x1000006, 0x100000, CRC(eb75ffbe) SHA1(e9d1deca60be696ac5bff2017fb5de3525e5239a) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "pwg.01",   0x00000, 0x08000, CRC(18a5c0e4) SHA1(bb1353dd74884aaeec9b5f1d0b284d9cad53c0ff) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "pwg.02",   0x28000, 0x20000, CRC(c9dfffa6) SHA1(64e71028befe9a2514074be765dd020e1d2ea70b) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "pwg.11",   0x000000, 0x200000, CRC(a78f7433) SHA1(e47ffba7b9dac9d0dda985c5d966194be18260f7) )
	ROM_LOAD16_WORD_SWAP( "pwg.12",   0x200000, 0x200000, CRC(77438ed0) SHA1(733ca6c6a792e66e2aa12c5fc06dd459527afe4b) )
ROM_END

ROM_START( armwara )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "pwga.03a", 0x000000, 0x80000, CRC(8d474ab1) SHA1(46baa3a263189001cfc6003fcb346a1996be8b24) )
	ROM_LOAD16_WORD_SWAP( "pwga.04a", 0x080000, 0x80000, CRC(81b5aec7) SHA1(f1371149a00e7c52d022d5c0cb6f8821c6474d35) )
	ROM_LOAD16_WORD_SWAP( "pwga.05a", 0x100000, 0x80000, CRC(2618e819) SHA1(58c857988e0ad2839d936d3e405637d8e2a45fe9) )
	ROM_LOAD16_WORD_SWAP( "pwg.06",   0x180000, 0x80000, CRC(87a60ce8) SHA1(e2085c7c8c6792d055dbbb023c7f4e4aa38ae924) )
	ROM_LOAD16_WORD_SWAP( "pwg.07",   0x200000, 0x80000, CRC(f7b148df) SHA1(f369669713cf647222094c570a2eacd48a8637cf) )
	ROM_LOAD16_WORD_SWAP( "pwg.08",   0x280000, 0x80000, CRC(cc62823e) SHA1(edaf9bebdfc65ae5414090abd6844176eec39a00) )
	ROM_LOAD16_WORD_SWAP( "pwg.09",   0x300000, 0x80000, CRC(ddc85ca6) SHA1(e794c679531632e2142c6a5e3b858494389ce65e) )
	ROM_LOAD16_WORD_SWAP( "pwg.10",   0x380000, 0x80000, CRC(07c4fb28) SHA1(58a1ff3d105be7df833dd4f32973766649efcbcf) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
/*	ROM_LOAD16_WORD_SWAP( "pwgax.03a", 0x000000, 0x80000, NO_DUMP )*/
/*	ROM_LOAD16_WORD_SWAP( "pwgax.04a", 0x080000, 0x80000, NO_DUMP )*/

	ROM_REGION( 0x1400000, REGION_GFX1, 0 )
	ROMX_LOAD( "pwg.13",   0x0000000, 0x400000, CRC(ae8fe08e) SHA1(b6f09663dcda69b5d7ac13e4afaf1efd692fb61e) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pwg.15",   0x0000002, 0x400000, CRC(db560f58) SHA1(0c3716b32eb24544ff5d16b5dcadce195cd10d00) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pwg.17",   0x0000004, 0x400000, CRC(bc475b94) SHA1(a157664450895a146a532581dd6f4b63dff21c86) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pwg.19",   0x0000006, 0x400000, CRC(07439ff7) SHA1(f71e07c6d77c32828f5e319268b24b13a1a4b0c2) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pwg.14",   0x1000000, 0x100000, CRC(c3f9ba63) SHA1(66191a52c39daa89b17ede5804ee41c028036f14) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pwg.16",   0x1000002, 0x100000, CRC(815b0e7b) SHA1(549785daac3122253fb94f6541bc7016147f5306) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pwg.18",   0x1000004, 0x100000, CRC(0109c71b) SHA1(eb51284ee0c85ff8f605fe1d166b7aa202be1344) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pwg.20",   0x1000006, 0x100000, CRC(eb75ffbe) SHA1(e9d1deca60be696ac5bff2017fb5de3525e5239a) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "pwg.01",   0x00000, 0x08000, CRC(18a5c0e4) SHA1(bb1353dd74884aaeec9b5f1d0b284d9cad53c0ff) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "pwg.02",   0x28000, 0x20000, CRC(c9dfffa6) SHA1(64e71028befe9a2514074be765dd020e1d2ea70b) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "pwg.11",   0x000000, 0x200000, CRC(a78f7433) SHA1(e47ffba7b9dac9d0dda985c5d966194be18260f7) )
	ROM_LOAD16_WORD_SWAP( "pwg.12",   0x200000, 0x200000, CRC(77438ed0) SHA1(733ca6c6a792e66e2aa12c5fc06dd459527afe4b) )
ROM_END

ROM_START( avsp )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "avpe.03d", 0x000000, 0x80000, CRC(774334a9) SHA1(f60b0e39139ea40e0b0ba97ed01d4a757ed65e1a) )
	ROM_LOAD16_WORD_SWAP( "avpe.04d", 0x080000, 0x80000, CRC(7fa83769) SHA1(930f02e4d35686e80fbdd673380c4b2bd784a9e5) )
	ROM_LOAD16_WORD_SWAP( "avp.05d",  0x100000, 0x80000, CRC(fbfb5d7a) SHA1(5549bc9d780753bc9c10fba82588e5c3d4a2acb2) )
	ROM_LOAD16_WORD_SWAP( "avp.06",   0x180000, 0x80000, CRC(190b817f) SHA1(9bcfc0a015ffba9cdac25b6270939a9690de5da7) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "avpex.03d", 0x000000, 0x80000, CRC(73dd740e) SHA1(c577928eb42616c87439fa30299daa5986514c37) )
	ROM_LOAD16_WORD_SWAP( "avpex.04d", 0x080000, 0x80000, CRC(185f8c43) SHA1(6802787918a794c8c44aec887c7f17eeeee1885d) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROMX_LOAD( "avp.13",   0x0000000, 0x200000, CRC(8f8b5ae4) SHA1(457ce959aa5db3a003de7dda2b3799b2f1ae279b) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "avp.15",   0x0000002, 0x200000, CRC(b00280df) SHA1(bc1291a4a222d410bc99b6f1ed392067d9c3999e) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "avp.17",   0x0000004, 0x200000, CRC(94403195) SHA1(efaad001527a5eba8f626aea9037ac6ef9a2c295) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "avp.19",   0x0000006, 0x200000, CRC(e1981245) SHA1(809ccb7f10262e227d5e9d9f710e06f0e751f550) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "avp.14",   0x0800000, 0x200000, CRC(ebba093e) SHA1(77aaf4197d1dae3321cf9c6d2b7967ee54cf3f30) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "avp.16",   0x0800002, 0x200000, CRC(fb228297) SHA1(ebd02a4ba085dc70c0603662e14d61625fa04648) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "avp.18",   0x0800004, 0x200000, CRC(34fb7232) SHA1(8b1f15bfa758a61e6ad519af24ca774edc70d194) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "avp.20",   0x0800006, 0x200000, CRC(f90baa21) SHA1(20a900819a9d321316e3dfd241210725d7191ecf) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "avp.01",   0x00000, 0x08000, CRC(2d3b4220) SHA1(2b2d04d4282550fa9f6e1ad8528f20d1f2ac02eb) )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "avp.11",   0x000000, 0x200000, CRC(83499817) SHA1(e65b0ebd61ddc748842a9d4d92404b5305307623) )
	ROM_LOAD16_WORD_SWAP( "avp.12",   0x200000, 0x200000, CRC(f4110d49) SHA1(f27538776cc1ba8213f19f98728ed8c02508d3ac) )
ROM_END

ROM_START( avspu )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "avpu.03d", 0x000000, 0x80000, CRC(42757950) SHA1(e6acae73a300c0e07c21f776e6aa87628184b152) )
	ROM_LOAD16_WORD_SWAP( "avpu.04d", 0x080000, 0x80000, CRC(5abcdee6) SHA1(205e1ac8f4e359fd04e3a1e12425ba0b8330b1c1) )
	ROM_LOAD16_WORD_SWAP( "avp.05d",  0x100000, 0x80000, CRC(fbfb5d7a) SHA1(5549bc9d780753bc9c10fba82588e5c3d4a2acb2) )
	ROM_LOAD16_WORD_SWAP( "avp.06",   0x180000, 0x80000, CRC(190b817f) SHA1(9bcfc0a015ffba9cdac25b6270939a9690de5da7) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "avpux.03d", 0x000000, 0x80000, CRC(d5b01046) SHA1(59ddf1949fcbd5aae344696bcac9659d33884eed) )
	ROM_LOAD16_WORD_SWAP( "avpux.04d", 0x080000, 0x80000, CRC(94bd7603) SHA1(4662c6a21f32eaac3127593079d72800758293bb) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROMX_LOAD( "avp.13",   0x0000000, 0x200000, CRC(8f8b5ae4) SHA1(457ce959aa5db3a003de7dda2b3799b2f1ae279b) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "avp.15",   0x0000002, 0x200000, CRC(b00280df) SHA1(bc1291a4a222d410bc99b6f1ed392067d9c3999e) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "avp.17",   0x0000004, 0x200000, CRC(94403195) SHA1(efaad001527a5eba8f626aea9037ac6ef9a2c295) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "avp.19",   0x0000006, 0x200000, CRC(e1981245) SHA1(809ccb7f10262e227d5e9d9f710e06f0e751f550) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "avp.14",   0x0800000, 0x200000, CRC(ebba093e) SHA1(77aaf4197d1dae3321cf9c6d2b7967ee54cf3f30) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "avp.16",   0x0800002, 0x200000, CRC(fb228297) SHA1(ebd02a4ba085dc70c0603662e14d61625fa04648) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "avp.18",   0x0800004, 0x200000, CRC(34fb7232) SHA1(8b1f15bfa758a61e6ad519af24ca774edc70d194) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "avp.20",   0x0800006, 0x200000, CRC(f90baa21) SHA1(20a900819a9d321316e3dfd241210725d7191ecf) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "avp.01",   0x00000, 0x08000, CRC(2d3b4220) SHA1(2b2d04d4282550fa9f6e1ad8528f20d1f2ac02eb) )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "avp.11",   0x000000, 0x200000, CRC(83499817) SHA1(e65b0ebd61ddc748842a9d4d92404b5305307623) )
	ROM_LOAD16_WORD_SWAP( "avp.12",   0x200000, 0x200000, CRC(f4110d49) SHA1(f27538776cc1ba8213f19f98728ed8c02508d3ac) )
ROM_END

ROM_START( avspj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "avpj.03d", 0x000000, 0x80000, CRC(49799119) SHA1(71a938b779291c3092ef6ef22935d89fd9c1186c) )
	ROM_LOAD16_WORD_SWAP( "avpj.04d", 0x080000, 0x80000, CRC(8cd2bba8) SHA1(1ea493d0d4b6e202ad38843b93035fa3f7e1b8c7) )
	ROM_LOAD16_WORD_SWAP( "avp.05d",  0x100000, 0x80000, CRC(fbfb5d7a) SHA1(5549bc9d780753bc9c10fba82588e5c3d4a2acb2) )
	ROM_LOAD16_WORD_SWAP( "avp.06",   0x180000, 0x80000, CRC(190b817f) SHA1(9bcfc0a015ffba9cdac25b6270939a9690de5da7) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "avpjx.03d", 0x000000, 0x80000, CRC(94095fb0) SHA1(06cc9951b7868bedf6185902a105ca003ecf6c62) )
	ROM_LOAD16_WORD_SWAP( "avpjx.04d", 0x080000, 0x80000, CRC(a56b00ae) SHA1(41c9f830129ad48f10bd0ac0b818328dc8e52e4c) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROMX_LOAD( "avp.13",   0x0000000, 0x200000, CRC(8f8b5ae4) SHA1(457ce959aa5db3a003de7dda2b3799b2f1ae279b) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "avp.15",   0x0000002, 0x200000, CRC(b00280df) SHA1(bc1291a4a222d410bc99b6f1ed392067d9c3999e) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "avp.17",   0x0000004, 0x200000, CRC(94403195) SHA1(efaad001527a5eba8f626aea9037ac6ef9a2c295) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "avp.19",   0x0000006, 0x200000, CRC(e1981245) SHA1(809ccb7f10262e227d5e9d9f710e06f0e751f550) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "avp.14",   0x0800000, 0x200000, CRC(ebba093e) SHA1(77aaf4197d1dae3321cf9c6d2b7967ee54cf3f30) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "avp.16",   0x0800002, 0x200000, CRC(fb228297) SHA1(ebd02a4ba085dc70c0603662e14d61625fa04648) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "avp.18",   0x0800004, 0x200000, CRC(34fb7232) SHA1(8b1f15bfa758a61e6ad519af24ca774edc70d194) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "avp.20",   0x0800006, 0x200000, CRC(f90baa21) SHA1(20a900819a9d321316e3dfd241210725d7191ecf) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "avp.01",   0x00000, 0x08000, CRC(2d3b4220) SHA1(2b2d04d4282550fa9f6e1ad8528f20d1f2ac02eb) )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "avp.11",   0x000000, 0x200000, CRC(83499817) SHA1(e65b0ebd61ddc748842a9d4d92404b5305307623) )
	ROM_LOAD16_WORD_SWAP( "avp.12",   0x200000, 0x200000, CRC(f4110d49) SHA1(f27538776cc1ba8213f19f98728ed8c02508d3ac) )
ROM_END

ROM_START( avspa )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "avpa.03d", 0x000000, 0x80000, CRC(6c1c1858) SHA1(29af268cf070ea2adc0aac0c5187debdd9706037) )
	ROM_LOAD16_WORD_SWAP( "avpa.04d", 0x080000, 0x80000, CRC(94f50b0c) SHA1(607b13e4cb4968c47a598f7dfec965c6d6ba68f0) )
	ROM_LOAD16_WORD_SWAP( "avp.05d",  0x100000, 0x80000, CRC(fbfb5d7a) SHA1(5549bc9d780753bc9c10fba82588e5c3d4a2acb2) )
	ROM_LOAD16_WORD_SWAP( "avp.06",   0x180000, 0x80000, CRC(190b817f) SHA1(9bcfc0a015ffba9cdac25b6270939a9690de5da7) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
/*	ROM_LOAD16_WORD_SWAP( "avpax.03d", 0x000000, 0x80000, NO_DUMP )*/
/*	ROM_LOAD16_WORD_SWAP( "avpax.04d", 0x080000, 0x80000, NO_DUMP )*/

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROMX_LOAD( "avp.13",   0x0000000, 0x200000, CRC(8f8b5ae4) SHA1(457ce959aa5db3a003de7dda2b3799b2f1ae279b) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "avp.15",   0x0000002, 0x200000, CRC(b00280df) SHA1(bc1291a4a222d410bc99b6f1ed392067d9c3999e) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "avp.17",   0x0000004, 0x200000, CRC(94403195) SHA1(efaad001527a5eba8f626aea9037ac6ef9a2c295) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "avp.19",   0x0000006, 0x200000, CRC(e1981245) SHA1(809ccb7f10262e227d5e9d9f710e06f0e751f550) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "avp.14",   0x0800000, 0x200000, CRC(ebba093e) SHA1(77aaf4197d1dae3321cf9c6d2b7967ee54cf3f30) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "avp.16",   0x0800002, 0x200000, CRC(fb228297) SHA1(ebd02a4ba085dc70c0603662e14d61625fa04648) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "avp.18",   0x0800004, 0x200000, CRC(34fb7232) SHA1(8b1f15bfa758a61e6ad519af24ca774edc70d194) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "avp.20",   0x0800006, 0x200000, CRC(f90baa21) SHA1(20a900819a9d321316e3dfd241210725d7191ecf) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "avp.01",   0x00000, 0x08000, CRC(2d3b4220) SHA1(2b2d04d4282550fa9f6e1ad8528f20d1f2ac02eb) )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "avp.11",   0x000000, 0x200000, CRC(83499817) SHA1(e65b0ebd61ddc748842a9d4d92404b5305307623) )
	ROM_LOAD16_WORD_SWAP( "avp.12",   0x200000, 0x200000, CRC(f4110d49) SHA1(f27538776cc1ba8213f19f98728ed8c02508d3ac) )
ROM_END

ROM_START( batcir )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "btce.03", 0x000000, 0x80000, CRC(bc60484b) SHA1(9b4e46d0f3d96edcd1c3707409507a5027c69039) )
	ROM_LOAD16_WORD_SWAP( "btce.04", 0x080000, 0x80000, CRC(457d55f6) SHA1(19a39ec30166d4b797babe9d70328ac572d1f916) )
	ROM_LOAD16_WORD_SWAP( "btce.05", 0x100000, 0x80000, CRC(e86560d7) SHA1(a978a7f5e0069cd78c8588c2d91b825796c723a5) )
	ROM_LOAD16_WORD_SWAP( "btce.06", 0x180000, 0x80000, CRC(f778e61b) SHA1(e8321dece8977131e41c9207946b627074c13ee7) )
	ROM_LOAD16_WORD_SWAP( "btc.07",  0x200000, 0x80000, CRC(7322d5db) SHA1(473be1f1bf603bdd82451661a6206507f50ed2b6) )
	ROM_LOAD16_WORD_SWAP( "btc.08",  0x280000, 0x80000, CRC(6aac85ab) SHA1(ad02d4185c2b3664fb96350d8ad317d3939a7554) )
	ROM_LOAD16_WORD_SWAP( "btc.09",  0x300000, 0x80000, CRC(1203db08) SHA1(fdbea14618b277132f9e010ef36c134a8ea42162) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "btcex.03", 0x000000, 0x80000, CRC(2d193cd0) SHA1(534934bb9ab117ffc8593b5a7e32ce2aa78818ca) )
	ROM_LOAD16_WORD_SWAP( "btcex.04", 0x080000, 0x80000, CRC(a3895d8b) SHA1(b2bb8a3924ee2d436cfd7d91fdf875af6225d0aa) )
	ROM_LOAD16_WORD_SWAP( "btcex.05", 0x100000, 0x80000, CRC(bdbed16f) SHA1(3a3d84a2002c216bb77293dc0657ad425caa8236) )
	ROM_LOAD16_WORD_SWAP( "btcex.06", 0x180000, 0x80000, CRC(02048217) SHA1(cb65ec945f6ebf4b994c172bfb337ef08473e946) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROMX_LOAD( "btc.13",   0x000000, 0x400000, CRC(dc705bad) SHA1(96e37147674bf9cd21c770897da59daac25d921a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "btc.15",   0x000002, 0x400000, CRC(e5779a3c) SHA1(bbd7fbe061e751388d2f02434144daf9b1e36640) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "btc.17",   0x000004, 0x400000, CRC(b33f4112) SHA1(e501fd921c8bcede69946b029e05d422714c1040) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "btc.19",   0x000006, 0x400000, CRC(a6fcdb7e) SHA1(7a28d5d7aa036d23d97fad17d0cdb8210dc8153a) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "btc.01",   0x00000, 0x08000, CRC(1e194310) SHA1(3b29de0aca9dbca59d6b50fb2509e2a913c6b0af) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "btc.02",   0x28000, 0x20000, CRC(01aeb8e6) SHA1(50a5d1cce0caf7c5143d4904431e8f41e2a57464) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "btc.11",   0x000000, 0x200000, CRC(c27f2229) SHA1(df2459493af40937b6656a16fad43ff51bed2204) )
	ROM_LOAD16_WORD_SWAP( "btc.12",   0x200000, 0x200000, CRC(418a2e33) SHA1(0642ddff2ab9255f154419da24ba644ed63f34ab) )
ROM_END

ROM_START( batcirj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "btcj.03", 0x000000, 0x80000, CRC(6b7e168d) SHA1(7e95cc436d53d1ce34b575bc7e2b6e2a7ae06cfb) )
	ROM_LOAD16_WORD_SWAP( "btcj.04", 0x080000, 0x80000, CRC(46ba3467) SHA1(0cc4a6c82f110d2334fd81f2d3abe5de882768bd) )
	ROM_LOAD16_WORD_SWAP( "btcj.05", 0x100000, 0x80000, CRC(0e23a859) SHA1(6c7eec9bf823c66fddbc6b297ea6aa883d03bff5) )
	ROM_LOAD16_WORD_SWAP( "btcj.06", 0x180000, 0x80000, CRC(a853b59c) SHA1(841c178722d4850994afee90ef3079607d8847ed) )
	ROM_LOAD16_WORD_SWAP( "btc.07",  0x200000, 0x80000, CRC(7322d5db) SHA1(473be1f1bf603bdd82451661a6206507f50ed2b6) )
	ROM_LOAD16_WORD_SWAP( "btc.08",  0x280000, 0x80000, CRC(6aac85ab) SHA1(ad02d4185c2b3664fb96350d8ad317d3939a7554) )
	ROM_LOAD16_WORD_SWAP( "btc.09",  0x300000, 0x80000, CRC(1203db08) SHA1(fdbea14618b277132f9e010ef36c134a8ea42162) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "btcjx.03", 0x000000, 0x80000, CRC(01482d08) SHA1(72c0681722b4c4204a42c145d906e351ddb620de) )
	ROM_LOAD16_WORD_SWAP( "btcjx.04", 0x080000, 0x80000, CRC(3d4c976b) SHA1(45f0f4beb47e3f24d53f44d01c828529a615fb45) )
	ROM_LOAD16_WORD_SWAP( "btcjx.05", 0x100000, 0x80000, CRC(5bf819e1) SHA1(4afd2a80220e359be47d66c4f11d8e79abb8a874) )
	ROM_LOAD16_WORD_SWAP( "btcjx.06", 0x180000, 0x80000, CRC(5d2fd190) SHA1(4c7413befe9010b15d1701d8784c9529a4d5a3dd) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROMX_LOAD( "btc.13",   0x000000, 0x400000, CRC(dc705bad) SHA1(96e37147674bf9cd21c770897da59daac25d921a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "btc.15",   0x000002, 0x400000, CRC(e5779a3c) SHA1(bbd7fbe061e751388d2f02434144daf9b1e36640) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "btc.17",   0x000004, 0x400000, CRC(b33f4112) SHA1(e501fd921c8bcede69946b029e05d422714c1040) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "btc.19",   0x000006, 0x400000, CRC(a6fcdb7e) SHA1(7a28d5d7aa036d23d97fad17d0cdb8210dc8153a) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "btc.01",   0x00000, 0x08000, CRC(1e194310) SHA1(3b29de0aca9dbca59d6b50fb2509e2a913c6b0af) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "btc.02",   0x28000, 0x20000, CRC(01aeb8e6) SHA1(50a5d1cce0caf7c5143d4904431e8f41e2a57464) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "btc.11",   0x000000, 0x200000, CRC(c27f2229) SHA1(df2459493af40937b6656a16fad43ff51bed2204) )
	ROM_LOAD16_WORD_SWAP( "btc.12",   0x200000, 0x200000, CRC(418a2e33) SHA1(0642ddff2ab9255f154419da24ba644ed63f34ab) )
ROM_END

ROM_START( batcira )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "btca.03", 0x000000, 0x80000, CRC(1ad20d87) SHA1(0ad8b7725604a61314883cd4ed8599445fe1cbf8) )
	ROM_LOAD16_WORD_SWAP( "btca.04", 0x080000, 0x80000, CRC(2b3f4dbe) SHA1(be4ab2ac411523def5e05081a754b651ead52e1f) )
	ROM_LOAD16_WORD_SWAP( "btca.05", 0x100000, 0x80000, CRC(8238a3d9) SHA1(4b0fe0e6c6a8a6572fc3554f2ee77dc01c2f75c3) )
	ROM_LOAD16_WORD_SWAP( "btca.06", 0x180000, 0x80000, CRC(446c7c02) SHA1(2fda5d0fef3ca556976ec9126cb04af4fa883a38) )
	ROM_LOAD16_WORD_SWAP( "btc.07",  0x200000, 0x80000, CRC(7322d5db) SHA1(473be1f1bf603bdd82451661a6206507f50ed2b6) )
	ROM_LOAD16_WORD_SWAP( "btc.08",  0x280000, 0x80000, CRC(6aac85ab) SHA1(ad02d4185c2b3664fb96350d8ad317d3939a7554) )
	ROM_LOAD16_WORD_SWAP( "btc.09",  0x300000, 0x80000, CRC(1203db08) SHA1(fdbea14618b277132f9e010ef36c134a8ea42162) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
/*	ROM_LOAD16_WORD_SWAP( "btcax.03", 0x000000, 0x80000, NO_DUMP )*/
/*	ROM_LOAD16_WORD_SWAP( "btcax.04", 0x080000, 0x80000, NO_DUMP )*/
/*	ROM_LOAD16_WORD_SWAP( "btcax.05", 0x100000, 0x80000, NO_DUMP )*/
/*	ROM_LOAD16_WORD_SWAP( "btcax.06", 0x180000, 0x80000, NO_DUMP )*/

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROMX_LOAD( "btc.13",   0x000000, 0x400000, CRC(dc705bad) SHA1(96e37147674bf9cd21c770897da59daac25d921a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "btc.15",   0x000002, 0x400000, CRC(e5779a3c) SHA1(bbd7fbe061e751388d2f02434144daf9b1e36640) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "btc.17",   0x000004, 0x400000, CRC(b33f4112) SHA1(e501fd921c8bcede69946b029e05d422714c1040) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "btc.19",   0x000006, 0x400000, CRC(a6fcdb7e) SHA1(7a28d5d7aa036d23d97fad17d0cdb8210dc8153a) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "btc.01",   0x00000, 0x08000, CRC(1e194310) SHA1(3b29de0aca9dbca59d6b50fb2509e2a913c6b0af) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "btc.02",   0x28000, 0x20000, CRC(01aeb8e6) SHA1(50a5d1cce0caf7c5143d4904431e8f41e2a57464) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "btc.11",   0x000000, 0x200000, CRC(c27f2229) SHA1(df2459493af40937b6656a16fad43ff51bed2204) )
	ROM_LOAD16_WORD_SWAP( "btc.12",   0x200000, 0x200000, CRC(418a2e33) SHA1(0642ddff2ab9255f154419da24ba644ed63f34ab) )
ROM_END

ROM_START( csclubj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "cscj.03", 0x000000, 0x80000, CRC(ec4ddaa2) SHA1(f84af8bd01cc994ecd6ac24e829e2bd33817d862) )
	ROM_LOAD16_WORD_SWAP( "cscj.04", 0x080000, 0x80000, CRC(60c632bb) SHA1(0d42c33aa476d2cc4efcdad78667353b88225966) )
	ROM_LOAD16_WORD_SWAP( "cscj.05", 0x100000, 0x80000, CRC(ad042003) SHA1(1e167c88f3b0617c38c9f43bdc816045ac0296e0) )
	ROM_LOAD16_WORD_SWAP( "cscj.06", 0x180000, 0x80000, CRC(169e4d40) SHA1(6540d89df5e76189d32b696be7626087fe26e33b) )
	ROM_LOAD16_WORD_SWAP( "csc.07",  0x200000, 0x80000, CRC(01b05caa) SHA1(5b84487da68e6b6f2889c76bf9e070e25941988c) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "cscjx.03", 0x000000, 0x80000, CRC(2de1d45d) SHA1(204574607d2dc45e233ed2f88fadc1d5a3790ba6) )
	ROM_LOAD16_WORD_SWAP( "cscjx.04", 0x080000, 0x80000, CRC(81b25d76) SHA1(e81a5768c053cea10d340c6624e270dd5604c855) )
	ROM_LOAD16_WORD_SWAP( "cscjx.05", 0x100000, 0x80000, CRC(5adb1c93) SHA1(734aff59e3819ca2250d1fe3e945bd0f0410deef) )
	ROM_LOAD16_WORD_SWAP( "cscjx.06", 0x180000, 0x80000, CRC(f5558f79) SHA1(0cb75f19db9c83dffb998f6fc6dcf35a58d35dd9) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROM_FILL(              0x000000, 0x800000, 0 )
	ROMX_LOAD( "csc.14",   0x800000, 0x200000, CRC(e8904afa) SHA1(39713ffca4e3a754c7c44c0ef4d99fb5a77d8da7) , ROM_GROUPWORD | ROM_SKIP(6) ) /* roms 73 to 76 joined in all eprom version */
	ROMX_LOAD( "csc.16",   0x800002, 0x200000, CRC(c98c8079) SHA1(22d68ba2ef62b51981bb3e99ec2cde8d1b36514b) , ROM_GROUPWORD | ROM_SKIP(6) ) /* roms 63 to 66 joined in all eprom version */
	ROMX_LOAD( "csc.18",   0x800004, 0x200000, CRC(c030df5a) SHA1(6d5e5a05531e168d0d44c591f9185ae300908fc2) , ROM_GROUPWORD | ROM_SKIP(6) ) /* roms 83 to 86 joined in all eprom version */
	ROMX_LOAD( "csc.20",   0x800006, 0x200000, CRC(b4e55863) SHA1(da66f0a36266b906e4c149aec152c323bb184c57) , ROM_GROUPWORD | ROM_SKIP(6) ) /* roms 93 to 96 joined in all eprom version */

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "csc.01",   0x00000, 0x08000, CRC(ee162111) SHA1(ce8d4bd32bb10ee8b0274ba6fcef05a583b39d48) )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "csc.11",   0x000000, 0x200000, CRC(a027b827) SHA1(6d58a63efc7bd5d07353d9b55826c01a3c416c33) ) /* roms 51 to 54 joined in all eprom version */
	ROM_LOAD16_WORD_SWAP( "csc.12",   0x200000, 0x200000, CRC(cb7f6e55) SHA1(b64e6b663fd09e887d2dc0f4b545e88688c0af55) ) /* roms 55 to 58 joined in all eprom version */
ROM_END

ROM_START( csclub )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "csce.03", 0x000000, 0x80000, CRC(f2c852ef) SHA1(bc2d403958640d7ab0785d01a3df79ec31d0c239) )
	ROM_LOAD16_WORD_SWAP( "csce.04", 0x080000, 0x80000, CRC(1184530f) SHA1(18565f6a06e6078fc20dd9cf70802ac1da60c67a) )
	ROM_LOAD16_WORD_SWAP( "csce.05", 0x100000, 0x80000, CRC(804e2b6b) SHA1(e638f73442e3165ace84cdb1bd2a9d419e2d8c41) )
	ROM_LOAD16_WORD_SWAP( "csce.06", 0x180000, 0x80000, CRC(09277cb9) SHA1(51a0d335b5d6cde61c32f4e7ea49403f400db7fb) )
	ROM_LOAD16_WORD_SWAP( "csc.07",  0x200000, 0x80000, CRC(01b05caa) SHA1(5b84487da68e6b6f2889c76bf9e070e25941988c) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "cscex.03", 0x000000, 0x80000, CRC(305fce70) SHA1(c5f1d532a29a80d4997a9f83a85763f0dde641e8) )
	ROM_LOAD16_WORD_SWAP( "cscex.04", 0x080000, 0x80000, CRC(d11da066) SHA1(fc50cfc732192dfc6e7015d9f37bd6975a46f653) )
	ROM_LOAD16_WORD_SWAP( "cscex.05", 0x100000, 0x80000, CRC(410c6220) SHA1(72dcd5fa07182a6d447d700b9fc4107eb293c944) )
	ROM_LOAD16_WORD_SWAP( "cscex.06", 0x180000, 0x80000, CRC(eaf70123) SHA1(299f488911862b3ca8d67506e658da46d0dc670b) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROM_FILL(              0x000000, 0x800000, 0 )
	ROMX_LOAD( "csc.14",   0x800000, 0x200000, CRC(e8904afa) SHA1(39713ffca4e3a754c7c44c0ef4d99fb5a77d8da7) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "csc.16",   0x800002, 0x200000, CRC(c98c8079) SHA1(22d68ba2ef62b51981bb3e99ec2cde8d1b36514b) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "csc.18",   0x800004, 0x200000, CRC(c030df5a) SHA1(6d5e5a05531e168d0d44c591f9185ae300908fc2) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "csc.20",   0x800006, 0x200000, CRC(b4e55863) SHA1(da66f0a36266b906e4c149aec152c323bb184c57) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "csc.01",   0x00000, 0x08000, CRC(ee162111) SHA1(ce8d4bd32bb10ee8b0274ba6fcef05a583b39d48) )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "csc.11",   0x000000, 0x200000, CRC(a027b827) SHA1(6d58a63efc7bd5d07353d9b55826c01a3c416c33) )
	ROM_LOAD16_WORD_SWAP( "csc.12",   0x200000, 0x200000, CRC(cb7f6e55) SHA1(b64e6b663fd09e887d2dc0f4b545e88688c0af55) )
ROM_END

ROM_START( cscluba )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "csca.03", 0x000000, 0x80000, CRC(b6acd708) SHA1(27d316053b0e74b1e9db979d500767cfa49fbce3) )
	ROM_LOAD16_WORD_SWAP( "csca.04", 0x080000, 0x80000, CRC(d44ae35f) SHA1(cd464792fe777183b0b0587239fb1b52bd7f9ec7) )
	ROM_LOAD16_WORD_SWAP( "csca.05", 0x100000, 0x80000, CRC(8da76aec) SHA1(04552f2c9c424d808703136a7909df903aec290a) )
	ROM_LOAD16_WORD_SWAP( "csca.06", 0x180000, 0x80000, CRC(a1b7b1ee) SHA1(77ba745f094a29521bb686982399b8b9babd7cc6) )
	ROM_LOAD16_WORD_SWAP( "csc.07",  0x200000, 0x80000, CRC(01b05caa) SHA1(5b84487da68e6b6f2889c76bf9e070e25941988c) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "cscax.03", 0x000000, 0x80000, CRC(9f95b1e1) SHA1(22cd75f030bbef6b9655dad4ac83fb7fcd083f05) )
	ROM_LOAD16_WORD_SWAP( "cscax.04", 0x080000, 0x80000, CRC(08e85ab2) SHA1(e4e4062caefdc73b97e3dd374eef9ceda0a1f9fb) )
	ROM_LOAD16_WORD_SWAP( "cscax.05", 0x100000, 0x80000, CRC(1b2fae1d) SHA1(3a345f7c5511150d034dd57369e2372bef837dee) )
	ROM_LOAD16_WORD_SWAP( "cscax.06", 0x180000, 0x80000, CRC(9e548ba8) SHA1(f40acdf2bd1820afe12c115b89678f46e64bd015) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROM_FILL(              0x000000, 0x800000, 0 )
	ROMX_LOAD( "csc.14",   0x800000, 0x200000, CRC(e8904afa) SHA1(39713ffca4e3a754c7c44c0ef4d99fb5a77d8da7) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "csc.16",   0x800002, 0x200000, CRC(c98c8079) SHA1(22d68ba2ef62b51981bb3e99ec2cde8d1b36514b) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "csc.18",   0x800004, 0x200000, CRC(c030df5a) SHA1(6d5e5a05531e168d0d44c591f9185ae300908fc2) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "csc.20",   0x800006, 0x200000, CRC(b4e55863) SHA1(da66f0a36266b906e4c149aec152c323bb184c57) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "csc.01",   0x00000, 0x08000, CRC(ee162111) SHA1(ce8d4bd32bb10ee8b0274ba6fcef05a583b39d48) )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "csc.11",   0x000000, 0x200000, CRC(a027b827) SHA1(6d58a63efc7bd5d07353d9b55826c01a3c416c33) )
	ROM_LOAD16_WORD_SWAP( "csc.12",   0x200000, 0x200000, CRC(cb7f6e55) SHA1(b64e6b663fd09e887d2dc0f4b545e88688c0af55) )
ROM_END

ROM_START( csclubh )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "csch.03", 0x000000, 0x80000, CRC(0dd7e46d) SHA1(deacd350b8954998636065cf070c9955d08402b8) )
	ROM_LOAD16_WORD_SWAP( "csch.04", 0x080000, 0x80000, CRC(486e8143) SHA1(d50ab8a5fdc194a9cded74cff94e5b3b69069826) )
	ROM_LOAD16_WORD_SWAP( "csch.05", 0x100000, 0x80000, CRC(9e509dfb) SHA1(4a6cd8488a63ad3f7d5a08f2a6af4728dc147790) )
	ROM_LOAD16_WORD_SWAP( "csch.06", 0x180000, 0x80000, CRC(817ba313) SHA1(674e10e642c09d26886f3deb829dee330ff472be) )
	ROM_LOAD16_WORD_SWAP( "csc.07",  0x200000, 0x80000, CRC(01b05caa) SHA1(5b84487da68e6b6f2889c76bf9e070e25941988c) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
/*	ROM_LOAD16_WORD_SWAP( "cschx.03", 0x000000, 0x80000, NO_DUMP )*/
/*	ROM_LOAD16_WORD_SWAP( "cschx.04", 0x080000, 0x80000, NO_DUMP )*/
/*	ROM_LOAD16_WORD_SWAP( "cschx.05", 0x100000, 0x80000, NO_DUMP )*/
/*	ROM_LOAD16_WORD_SWAP( "cschx.06", 0x180000, 0x80000, NO_DUMP )*/

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROM_FILL(              0x000000, 0x800000, 0 )
	ROMX_LOAD( "csc.14",   0x800000, 0x200000, CRC(e8904afa) SHA1(39713ffca4e3a754c7c44c0ef4d99fb5a77d8da7) , ROM_GROUPWORD | ROM_SKIP(6) ) /* roms 73 to 76 joined in all eprom version */
	ROMX_LOAD( "csc.16",   0x800002, 0x200000, CRC(c98c8079) SHA1(22d68ba2ef62b51981bb3e99ec2cde8d1b36514b) , ROM_GROUPWORD | ROM_SKIP(6) ) /* roms 63 to 66 joined in all eprom version */
	ROMX_LOAD( "csc.18",   0x800004, 0x200000, CRC(c030df5a) SHA1(6d5e5a05531e168d0d44c591f9185ae300908fc2) , ROM_GROUPWORD | ROM_SKIP(6) ) /* roms 83 to 86 joined in all eprom version */
	ROMX_LOAD( "csc.20",   0x800006, 0x200000, CRC(b4e55863) SHA1(da66f0a36266b906e4c149aec152c323bb184c57) , ROM_GROUPWORD | ROM_SKIP(6) ) /* roms 93 to 96 joined in all eprom version */

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "csc.01",   0x00000, 0x08000, CRC(ee162111) SHA1(ce8d4bd32bb10ee8b0274ba6fcef05a583b39d48) )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "csc.11",   0x000000, 0x200000, CRC(a027b827) SHA1(6d58a63efc7bd5d07353d9b55826c01a3c416c33) ) /* roms 51 to 54 joined in all eprom version */
	ROM_LOAD16_WORD_SWAP( "csc.12",   0x200000, 0x200000, CRC(cb7f6e55) SHA1(b64e6b663fd09e887d2dc0f4b545e88688c0af55) ) /* roms 55 to 58 joined in all eprom version */
ROM_END

ROM_START( cybots )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "cybu.03", 0x000000, 0x80000, CRC(db4da8f4) SHA1(de9f3f261003f4f70ae32114a15e498387c23f6d) )
	ROM_LOAD16_WORD_SWAP( "cybu.04", 0x080000, 0x80000, CRC(1eec68ac) SHA1(b2b9379c84b121048cb83a8c48756b48cdbc3ea1) )
	ROM_LOAD16_WORD_SWAP( "cyb.05",  0x100000, 0x80000, CRC(ec40408e) SHA1(dd611c1708e7ef86e4f7cac4b7b0dff7baaee5ed) )
	ROM_LOAD16_WORD_SWAP( "cyb.06",  0x180000, 0x80000, CRC(1ad0bed2) SHA1(2ea005f3e73b05f8f0ec006cd9e95f7731a73897) )
	ROM_LOAD16_WORD_SWAP( "cyb.07",  0x200000, 0x80000, CRC(6245a39a) SHA1(4f607e733e2dea80211497522be6d0f09571928d) )
	ROM_LOAD16_WORD_SWAP( "cyb.08",  0x280000, 0x80000, CRC(4b48e223) SHA1(9714579a7a78b9716e44bca6c18bf1a93aa4e482) )
	ROM_LOAD16_WORD_SWAP( "cyb.09",  0x300000, 0x80000, CRC(e15238f6) SHA1(16abd92ebed921a6a7e8eac4b098dc61f7e5485c) )
	ROM_LOAD16_WORD_SWAP( "cyb.10",  0x380000, 0x80000, CRC(75f4003b) SHA1(8a65026ae35247cda016ce85a34034c62b3aa1a6) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "cybux.03", 0x000000, 0x80000, CRC(4b4f5b32) SHA1(634515d67be539ff71b9da6fdd38151d5c133a8a) )
	ROM_LOAD16_WORD_SWAP( "cybux.04", 0x080000, 0x80000, CRC(6615a7e9) SHA1(231f81b5fa863143b590c6cd45ea8fa995f76fc1) )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "cyb.13",   0x0000000, 0x400000, CRC(f0dce192) SHA1(b743938dc8e772dc3f63ed88a4a54c34fffdba21) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "cyb.15",   0x0000002, 0x400000, CRC(187aa39c) SHA1(80e3cf5c69f13343de667e1476bb716d45d3ff63) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "cyb.17",   0x0000004, 0x400000, CRC(8a0e4b12) SHA1(40132f3cc79b0a74460ebd4e0d4ddbe240efc06f) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "cyb.19",   0x0000006, 0x400000, CRC(34b62612) SHA1(154bbceb7d303a208abb1b2f3d507d5afacc71ed) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "cyb.14",   0x1000000, 0x400000, CRC(c1537957) SHA1(bfb1cc6786277b94ce28bfd464e2bbb6f6d3486e) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "cyb.16",   0x1000002, 0x400000, CRC(15349e86) SHA1(b0cde577d29a9f4e718b673c8645529ef0ababc9) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "cyb.18",   0x1000004, 0x400000, CRC(d83e977d) SHA1(e03f4a120c95a2f476ffc8492bca85e0c5cea068) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "cyb.20",   0x1000006, 0x400000, CRC(77cdad5c) SHA1(94d0cc5f05de4bc2d43977d91f887005dc10310c) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "cyb.01",   0x00000, 0x08000, CRC(9c0fb079) SHA1(06d260875a76da08d56ea2b2ae277e8c2dbae6e3) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "cyb.02",   0x28000, 0x20000, CRC(51cb0c4e) SHA1(c322957558d8d3e9dad090aebbe485978cbce8f5) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "cyb.11",   0x000000, 0x200000, CRC(362ccab2) SHA1(28e537067d4846f22657ee37166d18b8f05f4da1) )
	ROM_LOAD16_WORD_SWAP( "cyb.12",   0x200000, 0x200000, CRC(7066e9cc) SHA1(eb6a9d4998b3311344d73bae88d661d81609c492) )
ROM_END

ROM_START( cybotsj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "cybj.03", 0x000000, 0x80000, CRC(6096eada) SHA1(ea3fa2e6229d90cc3f69c59f447b6b373d64d2aa) )
	ROM_LOAD16_WORD_SWAP( "cybj.04", 0x080000, 0x80000, CRC(7b0ffaa9) SHA1(595c3e679ea02282bf8a5aa6c7c09e5c30e839c7) )
	ROM_LOAD16_WORD_SWAP( "cyb.05",  0x100000, 0x80000, CRC(ec40408e) SHA1(dd611c1708e7ef86e4f7cac4b7b0dff7baaee5ed) )
	ROM_LOAD16_WORD_SWAP( "cyb.06",  0x180000, 0x80000, CRC(1ad0bed2) SHA1(2ea005f3e73b05f8f0ec006cd9e95f7731a73897) )
	ROM_LOAD16_WORD_SWAP( "cyb.07",  0x200000, 0x80000, CRC(6245a39a) SHA1(4f607e733e2dea80211497522be6d0f09571928d) )
	ROM_LOAD16_WORD_SWAP( "cyb.08",  0x280000, 0x80000, CRC(4b48e223) SHA1(9714579a7a78b9716e44bca6c18bf1a93aa4e482) )
	ROM_LOAD16_WORD_SWAP( "cyb.09",  0x300000, 0x80000, CRC(e15238f6) SHA1(16abd92ebed921a6a7e8eac4b098dc61f7e5485c) )
	ROM_LOAD16_WORD_SWAP( "cyb.10",  0x380000, 0x80000, CRC(75f4003b) SHA1(8a65026ae35247cda016ce85a34034c62b3aa1a6) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "cybjx.03", 0x000000, 0x80000, CRC(867c9acd) SHA1(3a494d96ceb3adc8209101792bfbd54768f044e3) )
	ROM_LOAD16_WORD_SWAP( "cybjx.04", 0x080000, 0x80000, CRC(57ed677f) SHA1(4edae387d0399ab58c07928bea28b77937602af9) )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "cyb.13",   0x0000000, 0x400000, CRC(f0dce192) SHA1(b743938dc8e772dc3f63ed88a4a54c34fffdba21) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "cyb.15",   0x0000002, 0x400000, CRC(187aa39c) SHA1(80e3cf5c69f13343de667e1476bb716d45d3ff63) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "cyb.17",   0x0000004, 0x400000, CRC(8a0e4b12) SHA1(40132f3cc79b0a74460ebd4e0d4ddbe240efc06f) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "cyb.19",   0x0000006, 0x400000, CRC(34b62612) SHA1(154bbceb7d303a208abb1b2f3d507d5afacc71ed) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "cyb.14",   0x1000000, 0x400000, CRC(c1537957) SHA1(bfb1cc6786277b94ce28bfd464e2bbb6f6d3486e) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "cyb.16",   0x1000002, 0x400000, CRC(15349e86) SHA1(b0cde577d29a9f4e718b673c8645529ef0ababc9) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "cyb.18",   0x1000004, 0x400000, CRC(d83e977d) SHA1(e03f4a120c95a2f476ffc8492bca85e0c5cea068) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "cyb.20",   0x1000006, 0x400000, CRC(77cdad5c) SHA1(94d0cc5f05de4bc2d43977d91f887005dc10310c) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "cyb.01",   0x00000, 0x08000, CRC(9c0fb079) SHA1(06d260875a76da08d56ea2b2ae277e8c2dbae6e3) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "cyb.02",   0x28000, 0x20000, CRC(51cb0c4e) SHA1(c322957558d8d3e9dad090aebbe485978cbce8f5) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "cyb.11",   0x000000, 0x200000, CRC(362ccab2) SHA1(28e537067d4846f22657ee37166d18b8f05f4da1) )
	ROM_LOAD16_WORD_SWAP( "cyb.12",   0x200000, 0x200000, CRC(7066e9cc) SHA1(eb6a9d4998b3311344d73bae88d661d81609c492) )
ROM_END

ROM_START( ddtod )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "dade.03c", 0x000000, 0x80000, CRC(8e73533d) SHA1(6eece222e562dd0c453d8dec188c9553c46dfe3c) )
	ROM_LOAD16_WORD_SWAP( "dade.04c", 0x080000, 0x80000, CRC(00c2e82e) SHA1(fad4dcdac8d6ef04b71e987936bf27e3d93809fc) )
	ROM_LOAD16_WORD_SWAP( "dade.05c", 0x100000, 0x80000, CRC(ea996008) SHA1(9f41679531e971e62483415c07ef4ee7489ff779) )
	ROM_LOAD16_WORD_SWAP( "dade.06a", 0x180000, 0x80000, CRC(6225495a) SHA1(a9a02abb072e3482ac92d7aed8ce9a5bcf636bc0) )
	ROM_LOAD16_WORD_SWAP( "dade.07a", 0x200000, 0x80000, CRC(b3480ec3) SHA1(a66f8dba67101fd71c2af4f3c3d71e55778a9f2c) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "dadex.03c", 0x000000, 0x80000, CRC(7b8b9b2c) SHA1(56abd1c1db0ded9d504ee8d7eaedf6798d646118) )
	ROM_LOAD16_WORD_SWAP( "dadex.04c", 0x080000, 0x80000, CRC(370626d3) SHA1(a858658af04e96e905e0db0ccd896989760b9f99) )
	ROM_LOAD16_WORD_SWAP( "dadex.05c", 0x100000, 0x80000, CRC(5fd29e95) SHA1(7a7ef9ee84ce23e19a1a8b3b2a35f5d2fc14e5d9) )

	ROM_REGION( 0xc00000, REGION_GFX1, 0 )
	ROMX_LOAD( "dad.13",   0x000000, 0x200000, CRC(da3cb7d6) SHA1(d59bb53d5f32889eb6eb7f8b1c8781948c97283d) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.15",   0x000002, 0x200000, CRC(92b63172) SHA1(9bed7dbbb17729f2ad3d318396f5335c0bd39937) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.17",   0x000004, 0x200000, CRC(b98757f5) SHA1(3eead22e097906bf0e1e151cd0a9c75abc5a32d4) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.19",   0x000006, 0x200000, CRC(8121ce46) SHA1(40c4dc969318d38f0c6d5401c9c64371f51aa12c) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.14",   0x800000, 0x100000, CRC(837e6f3f) SHA1(c060183474fba0e82d765b9f282b84838550dff6) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.16",   0x800002, 0x100000, CRC(f0916bdb) SHA1(9354d258dd26cbbf12c78ecfc277c357cbdb360e) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.18",   0x800004, 0x100000, CRC(cef393ef) SHA1(830b33c86cc24776d17ad65fa89a3b16c40446a1) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.20",   0x800006, 0x100000, CRC(8953fe9e) SHA1(f4795beb006335d13e3934aa9760e775eb0bb950) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "dad.01",   0x00000, 0x08000, CRC(3f5e2424) SHA1(4aa744576bc6752c43a90a27a816ebd90076b248) )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "dad.11",   0x000000, 0x200000, CRC(0c499b67) SHA1(a8ebd8a1cd6dece8344b7cb0439d85843fb97616) )
	ROM_LOAD16_WORD_SWAP( "dad.12",   0x200000, 0x200000, CRC(2f0b5a4e) SHA1(8d1ebbb811aa469b0f0d29d719d2b9af28fb63a2) )
ROM_END

ROM_START( ddtodu )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "dadu.03b", 0x000000, 0x80000, CRC(a519905f) SHA1(7f846d7ac5d5e0d06657f712a7a09bee984a4f4b) )
	ROM_LOAD16_WORD_SWAP( "dadu.04b", 0x080000, 0x80000, CRC(52562d38) SHA1(3ee21399a19ee5e2db2a8c2a893d8a31a3419399) )
	ROM_LOAD16_WORD_SWAP( "dadu.05b", 0x100000, 0x80000, CRC(ee1cfbfe) SHA1(4107e495827ada1712a2393dffcdf52d98aca2e0) )
	ROM_LOAD16_WORD_SWAP( "dad.06",   0x180000, 0x80000, CRC(13aa3e56) SHA1(ccd3cda528d625bbf4dc0e8c5ad629af6080d705) )
	ROM_LOAD16_WORD_SWAP( "dad.07",   0x200000, 0x80000, CRC(431cb6dd) SHA1(ad3342e2fb8f0b3d7f57e845d5b80a871923324d) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "dadux.03b", 0x000000, 0x80000, CRC(f59ee70c) SHA1(df902b8c10f9fcfa738b26b65233e4b67c0ef163) )
	ROM_LOAD16_WORD_SWAP( "dadux.04b", 0x080000, 0x80000, CRC(622628ae) SHA1(f2d0f99acee91331c6e198b4c63332c50cdac076) )
	ROM_LOAD16_WORD_SWAP( "dadux.05b", 0x100000, 0x80000, CRC(424bd6e3) SHA1(fcd1705654b88f14927d19fa4ea2bc891728bcdf) )

	ROM_REGION( 0xc00000, REGION_GFX1, 0 )
	ROMX_LOAD( "dad.13",   0x000000, 0x200000, CRC(da3cb7d6) SHA1(d59bb53d5f32889eb6eb7f8b1c8781948c97283d) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.15",   0x000002, 0x200000, CRC(92b63172) SHA1(9bed7dbbb17729f2ad3d318396f5335c0bd39937) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.17",   0x000004, 0x200000, CRC(b98757f5) SHA1(3eead22e097906bf0e1e151cd0a9c75abc5a32d4) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.19",   0x000006, 0x200000, CRC(8121ce46) SHA1(40c4dc969318d38f0c6d5401c9c64371f51aa12c) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.14",   0x800000, 0x100000, CRC(837e6f3f) SHA1(c060183474fba0e82d765b9f282b84838550dff6) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.16",   0x800002, 0x100000, CRC(f0916bdb) SHA1(9354d258dd26cbbf12c78ecfc277c357cbdb360e) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.18",   0x800004, 0x100000, CRC(cef393ef) SHA1(830b33c86cc24776d17ad65fa89a3b16c40446a1) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.20",   0x800006, 0x100000, CRC(8953fe9e) SHA1(f4795beb006335d13e3934aa9760e775eb0bb950) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "dad.01",   0x00000, 0x08000, CRC(3f5e2424) SHA1(4aa744576bc6752c43a90a27a816ebd90076b248) )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "dad.11",   0x000000, 0x200000, CRC(0c499b67) SHA1(a8ebd8a1cd6dece8344b7cb0439d85843fb97616) )
	ROM_LOAD16_WORD_SWAP( "dad.12",   0x200000, 0x200000, CRC(2f0b5a4e) SHA1(8d1ebbb811aa469b0f0d29d719d2b9af28fb63a2) )
ROM_END

ROM_START( ddtodur1 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
  	ROM_LOAD16_WORD_SWAP( "dadu.03a", 0x000000, 0x80000, CRC(4413f177) SHA1(26c8d06adc83ffc5bec4abf05aa64e874e85d539) )
	ROM_LOAD16_WORD_SWAP( "dadu.04a", 0x080000, 0x80000, CRC(168de230) SHA1(3f8af1625bb0d9097e538f8ba7cd23d95b0233aa) )
	ROM_LOAD16_WORD_SWAP( "dadu.05a", 0x100000, 0x80000, CRC(03d39e91) SHA1(92461b87c55cb41bbe89bcb3e3f2e9b1ed521067) )
	ROM_LOAD16_WORD_SWAP( "dad.06",   0x180000, 0x80000, CRC(13aa3e56) SHA1(ccd3cda528d625bbf4dc0e8c5ad629af6080d705) )
	ROM_LOAD16_WORD_SWAP( "dad.07",   0x200000, 0x80000, CRC(431cb6dd) SHA1(ad3342e2fb8f0b3d7f57e845d5b80a871923324d) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "dadux.03a", 0x000000, 0x80000, CRC(f9ba14b6) SHA1(8e3648de732e44267119995a489f8c635643420d) )
	ROM_LOAD16_WORD_SWAP( "dadux.04a", 0x080000, 0x80000, CRC(ed85ec29) SHA1(001a360bcfa45904c6b2c6ab4d7e033c8dd93055) )
	ROM_LOAD16_WORD_SWAP( "dadux.05a", 0x100000, 0x80000, CRC(dbae3d1b) SHA1(c3fa5dc0ff88f05c1514449c050e8de3ff04fab5) )

	ROM_REGION( 0xc00000, REGION_GFX1, 0 )
	ROMX_LOAD( "dad.13",   0x000000, 0x200000, CRC(da3cb7d6) SHA1(d59bb53d5f32889eb6eb7f8b1c8781948c97283d) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.15",   0x000002, 0x200000, CRC(92b63172) SHA1(9bed7dbbb17729f2ad3d318396f5335c0bd39937) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.17",   0x000004, 0x200000, CRC(b98757f5) SHA1(3eead22e097906bf0e1e151cd0a9c75abc5a32d4) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.19",   0x000006, 0x200000, CRC(8121ce46) SHA1(40c4dc969318d38f0c6d5401c9c64371f51aa12c) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.14",   0x800000, 0x100000, CRC(837e6f3f) SHA1(c060183474fba0e82d765b9f282b84838550dff6) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.16",   0x800002, 0x100000, CRC(f0916bdb) SHA1(9354d258dd26cbbf12c78ecfc277c357cbdb360e) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.18",   0x800004, 0x100000, CRC(cef393ef) SHA1(830b33c86cc24776d17ad65fa89a3b16c40446a1) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.20",   0x800006, 0x100000, CRC(8953fe9e) SHA1(f4795beb006335d13e3934aa9760e775eb0bb950) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "dad.01",   0x00000, 0x08000, CRC(3f5e2424) SHA1(4aa744576bc6752c43a90a27a816ebd90076b248) )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "dad.11",   0x000000, 0x200000, CRC(0c499b67) SHA1(a8ebd8a1cd6dece8344b7cb0439d85843fb97616) )
	ROM_LOAD16_WORD_SWAP( "dad.12",   0x200000, 0x200000, CRC(2f0b5a4e) SHA1(8d1ebbb811aa469b0f0d29d719d2b9af28fb63a2) )
ROM_END

ROM_START( ddtodj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "dadj.03a", 0x000000, 0x80000, CRC(711638dc) SHA1(30c1d1a694aa8e51d072b26b47ba55aed6d77b7b) )
	ROM_LOAD16_WORD_SWAP( "dadj.04a", 0x080000, 0x80000, CRC(4869639c) SHA1(1544813e6712a78267c1d27b6b49148d42c11127) )
	ROM_LOAD16_WORD_SWAP( "dadj.05a", 0x100000, 0x80000, CRC(484c0efa) SHA1(d4ddef54149ef0141dcbe05df5f669fccf462559) )
	ROM_LOAD16_WORD_SWAP( "dad.06",   0x180000, 0x80000, CRC(13aa3e56) SHA1(ccd3cda528d625bbf4dc0e8c5ad629af6080d705) )
	ROM_LOAD16_WORD_SWAP( "dad.07",   0x200000, 0x80000, CRC(431cb6dd) SHA1(ad3342e2fb8f0b3d7f57e845d5b80a871923324d) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "dadjx.03a", 0x000000, 0x80000, CRC(4c7334d3) SHA1(83b11f0a78fd6c30dc5aaed92fec52ee38a6d034) )
	ROM_LOAD16_WORD_SWAP( "dadjx.04a", 0x080000, 0x80000, CRC(cfd15109) SHA1(d16657ee83d6aa6a682b0aa6330d7cec7a0185de) )
	ROM_LOAD16_WORD_SWAP( "dadjx.05a", 0x100000, 0x80000, CRC(d23bcd71) SHA1(557fd7e8f76aa7aae940ff5ed80ec6e6d0957353) )

	ROM_REGION( 0xc00000, REGION_GFX1, 0 )
	ROMX_LOAD( "dad.13",   0x000000, 0x200000, CRC(da3cb7d6) SHA1(d59bb53d5f32889eb6eb7f8b1c8781948c97283d) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.15",   0x000002, 0x200000, CRC(92b63172) SHA1(9bed7dbbb17729f2ad3d318396f5335c0bd39937) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.17",   0x000004, 0x200000, CRC(b98757f5) SHA1(3eead22e097906bf0e1e151cd0a9c75abc5a32d4) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.19",   0x000006, 0x200000, CRC(8121ce46) SHA1(40c4dc969318d38f0c6d5401c9c64371f51aa12c) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.14",   0x800000, 0x100000, CRC(837e6f3f) SHA1(c060183474fba0e82d765b9f282b84838550dff6) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.16",   0x800002, 0x100000, CRC(f0916bdb) SHA1(9354d258dd26cbbf12c78ecfc277c357cbdb360e) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.18",   0x800004, 0x100000, CRC(cef393ef) SHA1(830b33c86cc24776d17ad65fa89a3b16c40446a1) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.20",   0x800006, 0x100000, CRC(8953fe9e) SHA1(f4795beb006335d13e3934aa9760e775eb0bb950) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "dad.01",   0x00000, 0x08000, CRC(3f5e2424) SHA1(4aa744576bc6752c43a90a27a816ebd90076b248) )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "dad.11",   0x000000, 0x200000, CRC(0c499b67) SHA1(a8ebd8a1cd6dece8344b7cb0439d85843fb97616) )
	ROM_LOAD16_WORD_SWAP( "dad.12",   0x200000, 0x200000, CRC(2f0b5a4e) SHA1(8d1ebbb811aa469b0f0d29d719d2b9af28fb63a2) )
ROM_END

ROM_START( ddtoda )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "dada.03a", 0x000000, 0x80000, CRC(fc6f2dd7) SHA1(82f59670ec77a11e9765e2acd0e846d1c768b542) )
	ROM_LOAD16_WORD_SWAP( "dada.04a", 0x080000, 0x80000, CRC(d4be4009) SHA1(c914ddc8f0c237efb52dd1a8f56395b17a6583be) )
	ROM_LOAD16_WORD_SWAP( "dada.05a", 0x100000, 0x80000, CRC(6712d1cf) SHA1(a716ee5ca434badc57f67e0802c6b184bf243dbb) )
	ROM_LOAD16_WORD_SWAP( "dad.06",   0x180000, 0x80000, CRC(13aa3e56) SHA1(ccd3cda528d625bbf4dc0e8c5ad629af6080d705) )
	ROM_LOAD16_WORD_SWAP( "dad.07",   0x200000, 0x80000, CRC(431cb6dd) SHA1(ad3342e2fb8f0b3d7f57e845d5b80a871923324d) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
/*	ROM_LOAD16_WORD_SWAP( "dadax.03a", 0x000000, 0x80000, NO_DUMP )*/
/*	ROM_LOAD16_WORD_SWAP( "dadax.04a", 0x080000, 0x80000, NO_DUMP )*/
/*	ROM_LOAD16_WORD_SWAP( "dadax.05a", 0x100000, 0x80000, NO_DUMP )*/

	ROM_REGION( 0xc00000, REGION_GFX1, 0 )
	ROMX_LOAD( "dad.13",   0x000000, 0x200000, CRC(da3cb7d6) SHA1(d59bb53d5f32889eb6eb7f8b1c8781948c97283d) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.15",   0x000002, 0x200000, CRC(92b63172) SHA1(9bed7dbbb17729f2ad3d318396f5335c0bd39937) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.17",   0x000004, 0x200000, CRC(b98757f5) SHA1(3eead22e097906bf0e1e151cd0a9c75abc5a32d4) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.19",   0x000006, 0x200000, CRC(8121ce46) SHA1(40c4dc969318d38f0c6d5401c9c64371f51aa12c) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.14",   0x800000, 0x100000, CRC(837e6f3f) SHA1(c060183474fba0e82d765b9f282b84838550dff6) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.16",   0x800002, 0x100000, CRC(f0916bdb) SHA1(9354d258dd26cbbf12c78ecfc277c357cbdb360e) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.18",   0x800004, 0x100000, CRC(cef393ef) SHA1(830b33c86cc24776d17ad65fa89a3b16c40446a1) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.20",   0x800006, 0x100000, CRC(8953fe9e) SHA1(f4795beb006335d13e3934aa9760e775eb0bb950) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "dad.01",   0x00000, 0x08000, CRC(3f5e2424) SHA1(4aa744576bc6752c43a90a27a816ebd90076b248) )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "dad.11",   0x000000, 0x200000, CRC(0c499b67) SHA1(a8ebd8a1cd6dece8344b7cb0439d85843fb97616) )
	ROM_LOAD16_WORD_SWAP( "dad.12",   0x200000, 0x200000, CRC(2f0b5a4e) SHA1(8d1ebbb811aa469b0f0d29d719d2b9af28fb63a2) )
ROM_END

ROM_START( ddtodh )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "dadh.03b", 0x000000, 0x80000, CRC(ae0cb98e) SHA1(e85fb56d8f55fd1626a47301953b66597814e516) )
	ROM_LOAD16_WORD_SWAP( "dadh.04b", 0x080000, 0x80000, CRC(b5774363) SHA1(c91a6b257de4355a29d0a9742909592e69d287fb) )
	ROM_LOAD16_WORD_SWAP( "dadh.05b", 0x100000, 0x80000, CRC(6ce2a485) SHA1(7397105bbf88f6f2aa46614395df38b205e6461c) )
	ROM_LOAD16_WORD_SWAP( "dad.06",   0x180000, 0x80000, CRC(13aa3e56) SHA1(ccd3cda528d625bbf4dc0e8c5ad629af6080d705) )
	ROM_LOAD16_WORD_SWAP( "dad.07",   0x200000, 0x80000, CRC(431cb6dd) SHA1(ad3342e2fb8f0b3d7f57e845d5b80a871923324d) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "dadhx.03b", 0x000000, 0x80000, CRC(b9a16065) SHA1(4d98dbc117d0b940f43838234e06c64ca88d896a) )
	ROM_LOAD16_WORD_SWAP( "dadhx.04b", 0x080000, 0x80000, CRC(6b0d5086) SHA1(f3e7b7d456a1d7e9199040069c41f3095f9ad663) )
	ROM_LOAD16_WORD_SWAP( "dadhx.05b", 0x100000, 0x80000, CRC(e01ee2b2) SHA1(11a962da760a769b7c6e08f9d74b5037fb70d677) )

	ROM_REGION( 0xc00000, REGION_GFX1, 0 )
	ROMX_LOAD( "dad.13",   0x000000, 0x200000, CRC(da3cb7d6) SHA1(d59bb53d5f32889eb6eb7f8b1c8781948c97283d) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.15",   0x000002, 0x200000, CRC(92b63172) SHA1(9bed7dbbb17729f2ad3d318396f5335c0bd39937) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.17",   0x000004, 0x200000, CRC(b98757f5) SHA1(3eead22e097906bf0e1e151cd0a9c75abc5a32d4) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.19",   0x000006, 0x200000, CRC(8121ce46) SHA1(40c4dc969318d38f0c6d5401c9c64371f51aa12c) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.14",   0x800000, 0x100000, CRC(837e6f3f) SHA1(c060183474fba0e82d765b9f282b84838550dff6) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.16",   0x800002, 0x100000, CRC(f0916bdb) SHA1(9354d258dd26cbbf12c78ecfc277c357cbdb360e) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.18",   0x800004, 0x100000, CRC(cef393ef) SHA1(830b33c86cc24776d17ad65fa89a3b16c40446a1) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.20",   0x800006, 0x100000, CRC(8953fe9e) SHA1(f4795beb006335d13e3934aa9760e775eb0bb950) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "dad.01",   0x00000, 0x08000, CRC(3f5e2424) SHA1(4aa744576bc6752c43a90a27a816ebd90076b248) )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "dad.11",   0x000000, 0x200000, CRC(0c499b67) SHA1(a8ebd8a1cd6dece8344b7cb0439d85843fb97616) )
	ROM_LOAD16_WORD_SWAP( "dad.12",   0x200000, 0x200000, CRC(2f0b5a4e) SHA1(8d1ebbb811aa469b0f0d29d719d2b9af28fb63a2) )
ROM_END

ROM_START( ddsom )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "dd2e.03e", 0x000000, 0x80000, CRC(449361af) SHA1(14af2b35e6f43f92c9e071f1dc85b18cf73ecb35) )
	ROM_LOAD16_WORD_SWAP( "dd2e.04e", 0x080000, 0x80000, CRC(5b7052b6) SHA1(8a5f069f450da939d0f02518751cd9815d621d81) )
	ROM_LOAD16_WORD_SWAP( "dd2e.05e", 0x100000, 0x80000, CRC(788d5f60) SHA1(b8b42c11530a34c2878fb119c0a388e33067b66d) )
	ROM_LOAD16_WORD_SWAP( "dd2e.06e", 0x180000, 0x80000, CRC(e0807e1e) SHA1(4b978f5f647fff84d456eb14c9fd202d9a276997) )
	ROM_LOAD16_WORD_SWAP( "dd2e.07",  0x200000, 0x80000, CRC(bb777a02) SHA1(4b2c65a9129fc2262b35be1c10d06f60f5108981) )
	ROM_LOAD16_WORD_SWAP( "dd2e.08",  0x280000, 0x80000, CRC(30970890) SHA1(fd366a9323230f6997006ab4cc216f9a97865ebe) )
	ROM_LOAD16_WORD_SWAP( "dd2e.09",  0x300000, 0x80000, CRC(99e2194d) SHA1(cbcecdf5beeac3eac6c2c3fa395710e1b8347531) )
	ROM_LOAD16_WORD_SWAP( "dd2e.10",  0x380000, 0x80000, CRC(e198805e) SHA1(37ae9d88d98c59337b657cfa6feb56e4f9cae95f) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "dd2ex.03e", 0x000000, 0x80000, CRC(bdbc9b38) SHA1(4ccd94a588e6b24cb0e5ae91bcb6965d2de63380) )
	ROM_LOAD16_WORD_SWAP( "dd2ex.04e", 0x080000, 0x80000, CRC(24d1be86) SHA1(154c697b3a92eaf5a7c1c8dc083a78edb7def180) )

	ROM_REGION( 0x1800000, REGION_GFX1, 0 )
	ROMX_LOAD( "dd2.13",   0x0000000, 0x400000, CRC(a46b4e6e) SHA1(fb90f42868c581c481b4ceff9f692753fb186b30) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.15",   0x0000002, 0x400000, CRC(d5fc50fc) SHA1(bc692f17b18bb47a724cd5152377cd5ccd6e184a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.17",   0x0000004, 0x400000, CRC(837c0867) SHA1(3d6db290a8f76299a23543f0ccf6a7905e1088ac) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.19",   0x0000006, 0x400000, CRC(bb0ec21c) SHA1(e43ccc1cf63ccd2b504cc9fd701af849a7321914) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.14",   0x1000000, 0x200000, CRC(6d824ce2) SHA1(0ccfe6c8a944937718e28a1a373b5822c7b7001b) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.16",   0x1000002, 0x200000, CRC(79682ae5) SHA1(ee84f4791c29ce9e2bae06ba3ec47ff4d2cd7054) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.18",   0x1000004, 0x200000, CRC(acddd149) SHA1(7f50de9b2d1cc733594c642be1804190519caffa) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.20",   0x1000006, 0x200000, CRC(117fb0c0) SHA1(15c01fa1a71b6469b0e1bde0ce5835c5ff9d938c) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "dd2.01",   0x00000, 0x08000, CRC(99d657e5) SHA1(1528dd6b07a0e79951a35c0457c8a9c9770e9c78) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "dd2.02",   0x28000, 0x20000, CRC(117a3824) SHA1(14f3a12170b601c5466c93af9d2f24e0b386b4e4) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "dd2.11",   0x000000, 0x200000, CRC(98d0c325) SHA1(7406e8d943d77c468eb418c4113261f4ab973bbf) )
	ROM_LOAD16_WORD_SWAP( "dd2.12",   0x200000, 0x200000, CRC(5ea2e7fa) SHA1(0e6a9fd007f637adcb2226c902394f07de45e803) )
ROM_END

ROM_START( ddsomr1 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "dd2e.03b", 0x000000, 0x80000, CRC(cd2deb66) SHA1(8a3fa5aca364f11bea76f69504e82416efc0ec11) )
	ROM_LOAD16_WORD_SWAP( "dd2e.04d", 0x080000, 0x80000, CRC(bfee43cc) SHA1(16cb34103bede42599ff3083a70ff918fdce9929) )
	ROM_LOAD16_WORD_SWAP( "dd2e.05b", 0x100000, 0x80000, CRC(049ab19d) SHA1(dfd2ed64c409389fed9b1d96955cbe0cf2abd2b7) )
	ROM_LOAD16_WORD_SWAP( "dd2e.06d", 0x180000, 0x80000, CRC(3994fb8b) SHA1(9b864f6cbd9b12d9409fcc2739e12f9a0775f205) )
	ROM_LOAD16_WORD_SWAP( "dd2e.07",  0x200000, 0x80000, CRC(bb777a02) SHA1(4b2c65a9129fc2262b35be1c10d06f60f5108981) )
	ROM_LOAD16_WORD_SWAP( "dd2e.08",  0x280000, 0x80000, CRC(30970890) SHA1(fd366a9323230f6997006ab4cc216f9a97865ebe) )
	ROM_LOAD16_WORD_SWAP( "dd2e.09",  0x300000, 0x80000, CRC(99e2194d) SHA1(cbcecdf5beeac3eac6c2c3fa395710e1b8347531) )
	ROM_LOAD16_WORD_SWAP( "dd2e.10",  0x380000, 0x80000, CRC(e198805e) SHA1(37ae9d88d98c59337b657cfa6feb56e4f9cae95f) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "dd2ex.03b", 0x000000, 0x80000, CRC(e4924c80) SHA1(de7cf9a9aac67cb50b34af69e9569a1fcd1f562d) )
	ROM_LOAD16_WORD_SWAP( "dd2ex.04d", 0x080000, 0x80000, CRC(13c8b16f) SHA1(75d6a97935a0798eec5dadc628c5f0a539c9fabc) )

	ROM_REGION( 0x1800000, REGION_GFX1, 0 )
	ROMX_LOAD( "dd2.13",   0x0000000, 0x400000, CRC(a46b4e6e) SHA1(fb90f42868c581c481b4ceff9f692753fb186b30) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.15",   0x0000002, 0x400000, CRC(d5fc50fc) SHA1(bc692f17b18bb47a724cd5152377cd5ccd6e184a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.17",   0x0000004, 0x400000, CRC(837c0867) SHA1(3d6db290a8f76299a23543f0ccf6a7905e1088ac) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.19",   0x0000006, 0x400000, CRC(bb0ec21c) SHA1(e43ccc1cf63ccd2b504cc9fd701af849a7321914) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.14",   0x1000000, 0x200000, CRC(6d824ce2) SHA1(0ccfe6c8a944937718e28a1a373b5822c7b7001b) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.16",   0x1000002, 0x200000, CRC(79682ae5) SHA1(ee84f4791c29ce9e2bae06ba3ec47ff4d2cd7054) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.18",   0x1000004, 0x200000, CRC(acddd149) SHA1(7f50de9b2d1cc733594c642be1804190519caffa) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.20",   0x1000006, 0x200000, CRC(117fb0c0) SHA1(15c01fa1a71b6469b0e1bde0ce5835c5ff9d938c) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "dd2.01",   0x00000, 0x08000, CRC(99d657e5) SHA1(1528dd6b07a0e79951a35c0457c8a9c9770e9c78) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "dd2.02",   0x28000, 0x20000, CRC(117a3824) SHA1(14f3a12170b601c5466c93af9d2f24e0b386b4e4) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "dd2.11",   0x000000, 0x200000, CRC(98d0c325) SHA1(7406e8d943d77c468eb418c4113261f4ab973bbf) )
	ROM_LOAD16_WORD_SWAP( "dd2.12",   0x200000, 0x200000, CRC(5ea2e7fa) SHA1(0e6a9fd007f637adcb2226c902394f07de45e803) )
ROM_END

ROM_START( ddsomu )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "dd2u.03g", 0x000000, 0x80000, CRC(fb089b39) SHA1(2d00ad87d5e862745d730a84a8b9b9a8c9423282) )
	ROM_LOAD16_WORD_SWAP( "dd2u.04g", 0x080000, 0x80000, CRC(cd432b73) SHA1(7c5ddad66f9f08fef79efb01ccf230a9eae366c6) )
	ROM_LOAD16_WORD_SWAP( "dd2.05g",  0x100000, 0x80000, CRC(5eb1991c) SHA1(429a60b5396ff4192904867fbe0524268f0edbcb) )
	ROM_LOAD16_WORD_SWAP( "dd2.06g",  0x180000, 0x80000, CRC(c26b5e55) SHA1(9590206f30459941880ff4b56c7f276cc78e3a22) )
	ROM_LOAD16_WORD_SWAP( "dd2.07",   0x200000, 0x80000, CRC(909a0b8b) SHA1(58bda17c36063a79df8b5031755c7909a9bda221) )
	ROM_LOAD16_WORD_SWAP( "dd2.08",   0x280000, 0x80000, CRC(e53c4d01) SHA1(bad872e4e793a39f68bc0e580772e982714b5876) )
	ROM_LOAD16_WORD_SWAP( "dd2.09",   0x300000, 0x80000, CRC(5f86279f) SHA1(c2a454e5f821b1cdd49f2cf0602e9bfb7ba63340) )
	ROM_LOAD16_WORD_SWAP( "dd2.10",   0x380000, 0x80000, CRC(ad954c26) SHA1(468c01735dbdb1114b37060546a660678290a97f) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "dd2ux.03g", 0x000000, 0x80000, CRC(124ad51a) SHA1(2328afe9e9c07f9d931b545cd03abdeb0e760881) )
	ROM_LOAD16_WORD_SWAP( "dd2ux.04g", 0x080000, 0x80000, CRC(5401c248) SHA1(432458c3146d1aa40d72bc9feb97c05a7e5d64db) )

	ROM_REGION( 0x1800000, REGION_GFX1, 0 )
	ROMX_LOAD( "dd2.13",   0x0000000, 0x400000, CRC(a46b4e6e) SHA1(fb90f42868c581c481b4ceff9f692753fb186b30) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.15",   0x0000002, 0x400000, CRC(d5fc50fc) SHA1(bc692f17b18bb47a724cd5152377cd5ccd6e184a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.17",   0x0000004, 0x400000, CRC(837c0867) SHA1(3d6db290a8f76299a23543f0ccf6a7905e1088ac) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.19",   0x0000006, 0x400000, CRC(bb0ec21c) SHA1(e43ccc1cf63ccd2b504cc9fd701af849a7321914) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.14",   0x1000000, 0x200000, CRC(6d824ce2) SHA1(0ccfe6c8a944937718e28a1a373b5822c7b7001b) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.16",   0x1000002, 0x200000, CRC(79682ae5) SHA1(ee84f4791c29ce9e2bae06ba3ec47ff4d2cd7054) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.18",   0x1000004, 0x200000, CRC(acddd149) SHA1(7f50de9b2d1cc733594c642be1804190519caffa) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.20",   0x1000006, 0x200000, CRC(117fb0c0) SHA1(15c01fa1a71b6469b0e1bde0ce5835c5ff9d938c) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "dd2.01",   0x00000, 0x08000, CRC(99d657e5) SHA1(1528dd6b07a0e79951a35c0457c8a9c9770e9c78) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "dd2.02",   0x28000, 0x20000, CRC(117a3824) SHA1(14f3a12170b601c5466c93af9d2f24e0b386b4e4) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "dd2.11",   0x000000, 0x200000, CRC(98d0c325) SHA1(7406e8d943d77c468eb418c4113261f4ab973bbf) )
	ROM_LOAD16_WORD_SWAP( "dd2.12",   0x200000, 0x200000, CRC(5ea2e7fa) SHA1(0e6a9fd007f637adcb2226c902394f07de45e803) )
ROM_END

ROM_START( ddsomur1 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "dd2u.03d", 0x000000, 0x80000, CRC(0f700d84) SHA1(f4788d4046e0f6aba146c18a930196f5f9f8f14a) )
	ROM_LOAD16_WORD_SWAP( "dd2u.04d", 0x080000, 0x80000, CRC(b99eb254) SHA1(507ad31b0d77dfbaaaf0fa5830c4ef14845a80de) )
	ROM_LOAD16_WORD_SWAP( "dd2.05d",  0x100000, 0x80000, CRC(b23061f3) SHA1(471a1238770a5109f34a0b450b214a5490cc6ecb) )
	ROM_LOAD16_WORD_SWAP( "dd2.06d",  0x180000, 0x80000, CRC(8bf1d8ce) SHA1(384dda9dfa2a851d30432f29bba456e138a5ca28) )
	ROM_LOAD16_WORD_SWAP( "dd2.07",   0x200000, 0x80000, CRC(909a0b8b) SHA1(58bda17c36063a79df8b5031755c7909a9bda221) )
	ROM_LOAD16_WORD_SWAP( "dd2.08",   0x280000, 0x80000, CRC(e53c4d01) SHA1(bad872e4e793a39f68bc0e580772e982714b5876) )
	ROM_LOAD16_WORD_SWAP( "dd2.09",   0x300000, 0x80000, CRC(5f86279f) SHA1(c2a454e5f821b1cdd49f2cf0602e9bfb7ba63340) )
	ROM_LOAD16_WORD_SWAP( "dd2.10",   0x380000, 0x80000, CRC(ad954c26) SHA1(468c01735dbdb1114b37060546a660678290a97f) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "dd2ux.03d", 0x000000, 0x80000, CRC(5cecbdb3) SHA1(61a9e42711bd0b92244a269948e843b302566c3d) )
	ROM_LOAD16_WORD_SWAP( "dd2ux.04d", 0x080000, 0x80000, CRC(1307a77d) SHA1(5c0330c4ef657fa8af27c540f98d8c25f83e5994) )

	ROM_REGION( 0x1800000, REGION_GFX1, 0 )
	ROMX_LOAD( "dd2.13",   0x0000000, 0x400000, CRC(a46b4e6e) SHA1(fb90f42868c581c481b4ceff9f692753fb186b30) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.15",   0x0000002, 0x400000, CRC(d5fc50fc) SHA1(bc692f17b18bb47a724cd5152377cd5ccd6e184a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.17",   0x0000004, 0x400000, CRC(837c0867) SHA1(3d6db290a8f76299a23543f0ccf6a7905e1088ac) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.19",   0x0000006, 0x400000, CRC(bb0ec21c) SHA1(e43ccc1cf63ccd2b504cc9fd701af849a7321914) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.14",   0x1000000, 0x200000, CRC(6d824ce2) SHA1(0ccfe6c8a944937718e28a1a373b5822c7b7001b) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.16",   0x1000002, 0x200000, CRC(79682ae5) SHA1(ee84f4791c29ce9e2bae06ba3ec47ff4d2cd7054) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.18",   0x1000004, 0x200000, CRC(acddd149) SHA1(7f50de9b2d1cc733594c642be1804190519caffa) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.20",   0x1000006, 0x200000, CRC(117fb0c0) SHA1(15c01fa1a71b6469b0e1bde0ce5835c5ff9d938c) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "dd2.01",   0x00000, 0x08000, CRC(99d657e5) SHA1(1528dd6b07a0e79951a35c0457c8a9c9770e9c78) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "dd2.02",   0x28000, 0x20000, CRC(117a3824) SHA1(14f3a12170b601c5466c93af9d2f24e0b386b4e4) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "dd2.11",   0x000000, 0x200000, CRC(98d0c325) SHA1(7406e8d943d77c468eb418c4113261f4ab973bbf) )
	ROM_LOAD16_WORD_SWAP( "dd2.12",   0x200000, 0x200000, CRC(5ea2e7fa) SHA1(0e6a9fd007f637adcb2226c902394f07de45e803) )
ROM_END

ROM_START( ddsomj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "dd2j.03g", 0x000000, 0x80000, CRC(e6c8c985) SHA1(0736a84d7d9d37d51826eac6826a7728260bc625) )
	ROM_LOAD16_WORD_SWAP( "dd2j.04g", 0x080000, 0x80000, CRC(8386c0bd) SHA1(59bfc71914ec2bb7d1b9f327b25d2399181d4bb2) )
	ROM_LOAD16_WORD_SWAP( "dd2.05g",  0x100000, 0x80000, CRC(5eb1991c) SHA1(429a60b5396ff4192904867fbe0524268f0edbcb) )
	ROM_LOAD16_WORD_SWAP( "dd2.06g",  0x180000, 0x80000, CRC(c26b5e55) SHA1(9590206f30459941880ff4b56c7f276cc78e3a22) )
	ROM_LOAD16_WORD_SWAP( "dd2.07",   0x200000, 0x80000, CRC(909a0b8b) SHA1(58bda17c36063a79df8b5031755c7909a9bda221) )
	ROM_LOAD16_WORD_SWAP( "dd2.08",   0x280000, 0x80000, CRC(e53c4d01) SHA1(bad872e4e793a39f68bc0e580772e982714b5876) )
	ROM_LOAD16_WORD_SWAP( "dd2.09",   0x300000, 0x80000, CRC(5f86279f) SHA1(c2a454e5f821b1cdd49f2cf0602e9bfb7ba63340) )
	ROM_LOAD16_WORD_SWAP( "dd2.10",   0x380000, 0x80000, CRC(ad954c26) SHA1(468c01735dbdb1114b37060546a660678290a97f) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "dd2jx.03g", 0x000000, 0x80000, CRC(f6abe885) SHA1(dbc4409cb84591c4b65a2e4d76c5000b6d38155d) )
	ROM_LOAD16_WORD_SWAP( "dd2jx.04g", 0x080000, 0x80000, CRC(3abd7f79) SHA1(f4e6cc98e04ba8f7dfdd15c7ed2b40fb0933ab8f) )

	ROM_REGION( 0x1800000, REGION_GFX1, 0 )
	ROMX_LOAD( "dd2.13",   0x0000000, 0x400000, CRC(a46b4e6e) SHA1(fb90f42868c581c481b4ceff9f692753fb186b30) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.15",   0x0000002, 0x400000, CRC(d5fc50fc) SHA1(bc692f17b18bb47a724cd5152377cd5ccd6e184a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.17",   0x0000004, 0x400000, CRC(837c0867) SHA1(3d6db290a8f76299a23543f0ccf6a7905e1088ac) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.19",   0x0000006, 0x400000, CRC(bb0ec21c) SHA1(e43ccc1cf63ccd2b504cc9fd701af849a7321914) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.14",   0x1000000, 0x200000, CRC(6d824ce2) SHA1(0ccfe6c8a944937718e28a1a373b5822c7b7001b) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.16",   0x1000002, 0x200000, CRC(79682ae5) SHA1(ee84f4791c29ce9e2bae06ba3ec47ff4d2cd7054) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.18",   0x1000004, 0x200000, CRC(acddd149) SHA1(7f50de9b2d1cc733594c642be1804190519caffa) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.20",   0x1000006, 0x200000, CRC(117fb0c0) SHA1(15c01fa1a71b6469b0e1bde0ce5835c5ff9d938c) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "dd2.01",   0x00000, 0x08000, CRC(99d657e5) SHA1(1528dd6b07a0e79951a35c0457c8a9c9770e9c78) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "dd2.02",   0x28000, 0x20000, CRC(117a3824) SHA1(14f3a12170b601c5466c93af9d2f24e0b386b4e4) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "dd2.11",   0x000000, 0x200000, CRC(98d0c325) SHA1(7406e8d943d77c468eb418c4113261f4ab973bbf) )
	ROM_LOAD16_WORD_SWAP( "dd2.12",   0x200000, 0x200000, CRC(5ea2e7fa) SHA1(0e6a9fd007f637adcb2226c902394f07de45e803) )
ROM_END

ROM_START( ddsomjr1 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "dd2j.03b", 0x000000, 0x80000, CRC(965d74e5) SHA1(d7aa1b78043cdf09ee71a6dd5fe78e0588ca7875) )
	ROM_LOAD16_WORD_SWAP( "dd2j.04b", 0x080000, 0x80000, CRC(958eb8f3) SHA1(3d9747bc9091b0b42c953a19992b94cb2bf69159) )
	ROM_LOAD16_WORD_SWAP( "dd2.05b",  0x100000, 0x80000, CRC(d38571ca) SHA1(f0105a4f201e11f489e44c8061b0025de2e32f93) )
	ROM_LOAD16_WORD_SWAP( "dd2.06b",  0x180000, 0x80000, CRC(6d5a3bbb) SHA1(549e31398e706a80d41db6600555e27e902c335c) )
	ROM_LOAD16_WORD_SWAP( "dd2.07",   0x200000, 0x80000, CRC(909a0b8b) SHA1(58bda17c36063a79df8b5031755c7909a9bda221) )
	ROM_LOAD16_WORD_SWAP( "dd2.08",   0x280000, 0x80000, CRC(e53c4d01) SHA1(bad872e4e793a39f68bc0e580772e982714b5876) )
	ROM_LOAD16_WORD_SWAP( "dd2.09",   0x300000, 0x80000, CRC(5f86279f) SHA1(c2a454e5f821b1cdd49f2cf0602e9bfb7ba63340) )
	ROM_LOAD16_WORD_SWAP( "dd2.10",   0x380000, 0x80000, CRC(ad954c26) SHA1(468c01735dbdb1114b37060546a660678290a97f) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "dd2jx.03b", 0x000000, 0x80000, CRC(a63488bb) SHA1(2b224a9f405a5ae67c99f67885c5fc0536d2207c) )
	ROM_LOAD16_WORD_SWAP( "dd2jx.04b", 0x080000, 0x80000, CRC(e3ff7985) SHA1(3fbe184d7a066cb929239066ee2fccfd1d0314e6) )

	ROM_REGION( 0x1800000, REGION_GFX1, 0 )
	ROMX_LOAD( "dd2.13",   0x0000000, 0x400000, CRC(a46b4e6e) SHA1(fb90f42868c581c481b4ceff9f692753fb186b30) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.15",   0x0000002, 0x400000, CRC(d5fc50fc) SHA1(bc692f17b18bb47a724cd5152377cd5ccd6e184a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.17",   0x0000004, 0x400000, CRC(837c0867) SHA1(3d6db290a8f76299a23543f0ccf6a7905e1088ac) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.19",   0x0000006, 0x400000, CRC(bb0ec21c) SHA1(e43ccc1cf63ccd2b504cc9fd701af849a7321914) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.14",   0x1000000, 0x200000, CRC(6d824ce2) SHA1(0ccfe6c8a944937718e28a1a373b5822c7b7001b) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.16",   0x1000002, 0x200000, CRC(79682ae5) SHA1(ee84f4791c29ce9e2bae06ba3ec47ff4d2cd7054) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.18",   0x1000004, 0x200000, CRC(acddd149) SHA1(7f50de9b2d1cc733594c642be1804190519caffa) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.20",   0x1000006, 0x200000, CRC(117fb0c0) SHA1(15c01fa1a71b6469b0e1bde0ce5835c5ff9d938c) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "dd2.01",   0x00000, 0x08000, CRC(99d657e5) SHA1(1528dd6b07a0e79951a35c0457c8a9c9770e9c78) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "dd2.02",   0x28000, 0x20000, CRC(117a3824) SHA1(14f3a12170b601c5466c93af9d2f24e0b386b4e4) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "dd2.11",   0x000000, 0x200000, CRC(98d0c325) SHA1(7406e8d943d77c468eb418c4113261f4ab973bbf) )
	ROM_LOAD16_WORD_SWAP( "dd2.12",   0x200000, 0x200000, CRC(5ea2e7fa) SHA1(0e6a9fd007f637adcb2226c902394f07de45e803) )
ROM_END

ROM_START( ddsoma )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "dd2a.03g", 0x000000, 0x80000, CRC(0b4fec22) SHA1(4dd69637898e0bc64d1b1dc34561ce1807da314b) )
	ROM_LOAD16_WORD_SWAP( "dd2a.04g", 0x080000, 0x80000, CRC(055b7019) SHA1(5dab39552fee20bd6f94c992c1c3a995595fdf94) )
	ROM_LOAD16_WORD_SWAP( "dd2.05g",  0x100000, 0x80000, CRC(5eb1991c) SHA1(429a60b5396ff4192904867fbe0524268f0edbcb) )
	ROM_LOAD16_WORD_SWAP( "dd2.06g",  0x180000, 0x80000, CRC(c26b5e55) SHA1(9590206f30459941880ff4b56c7f276cc78e3a22) )
	ROM_LOAD16_WORD_SWAP( "dd2.07",   0x200000, 0x80000, CRC(909a0b8b) SHA1(58bda17c36063a79df8b5031755c7909a9bda221) )
	ROM_LOAD16_WORD_SWAP( "dd2.08",   0x280000, 0x80000, CRC(e53c4d01) SHA1(bad872e4e793a39f68bc0e580772e982714b5876) )
	ROM_LOAD16_WORD_SWAP( "dd2.09",   0x300000, 0x80000, CRC(5f86279f) SHA1(c2a454e5f821b1cdd49f2cf0602e9bfb7ba63340) )
	ROM_LOAD16_WORD_SWAP( "dd2.10",   0x380000, 0x80000, CRC(ad954c26) SHA1(468c01735dbdb1114b37060546a660678290a97f) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "dd2ax.03g", 0x000000, 0x80000, CRC(3eacb6c3) SHA1(dda41f01973e64b82eff4382acaa224330af2991) )
	ROM_LOAD16_WORD_SWAP( "dd2ax.04g", 0x080000, 0x80000, CRC(2afaa486) SHA1(cc7f608dd180614018582a0417cc6f187f2eb292) )

	ROM_REGION( 0x1800000, REGION_GFX1, 0 )
	ROMX_LOAD( "dd2.13",   0x0000000, 0x400000, CRC(a46b4e6e) SHA1(fb90f42868c581c481b4ceff9f692753fb186b30) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.15",   0x0000002, 0x400000, CRC(d5fc50fc) SHA1(bc692f17b18bb47a724cd5152377cd5ccd6e184a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.17",   0x0000004, 0x400000, CRC(837c0867) SHA1(3d6db290a8f76299a23543f0ccf6a7905e1088ac) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.19",   0x0000006, 0x400000, CRC(bb0ec21c) SHA1(e43ccc1cf63ccd2b504cc9fd701af849a7321914) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.14",   0x1000000, 0x200000, CRC(6d824ce2) SHA1(0ccfe6c8a944937718e28a1a373b5822c7b7001b) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.16",   0x1000002, 0x200000, CRC(79682ae5) SHA1(ee84f4791c29ce9e2bae06ba3ec47ff4d2cd7054) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.18",   0x1000004, 0x200000, CRC(acddd149) SHA1(7f50de9b2d1cc733594c642be1804190519caffa) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.20",   0x1000006, 0x200000, CRC(117fb0c0) SHA1(15c01fa1a71b6469b0e1bde0ce5835c5ff9d938c) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "dd2.01",   0x00000, 0x08000, CRC(99d657e5) SHA1(1528dd6b07a0e79951a35c0457c8a9c9770e9c78) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "dd2.02",   0x28000, 0x20000, CRC(117a3824) SHA1(14f3a12170b601c5466c93af9d2f24e0b386b4e4) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "dd2.11",   0x000000, 0x200000, CRC(98d0c325) SHA1(7406e8d943d77c468eb418c4113261f4ab973bbf) )
	ROM_LOAD16_WORD_SWAP( "dd2.12",   0x200000, 0x200000, CRC(5ea2e7fa) SHA1(0e6a9fd007f637adcb2226c902394f07de45e803) )
ROM_END

ROM_START( dimahoo )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "gmdu.03", 0x000000, 0x80000, CRC(43bcb15f) SHA1(8cf758f9b3b416273e5b20e5d1c09c0a67029a01) )
	ROM_LOAD16_WORD_SWAP( "gmd.04",  0x080000, 0x80000, CRC(37485567) SHA1(643c41fce6057bcaef0e0bedc62914c33d97eeaf) )
	ROM_LOAD16_WORD_SWAP( "gmd.05",  0x100000, 0x80000, CRC(da269ffb) SHA1(e99b04192030b6006cf67b563f40cea29c1b2e78) )
	ROM_LOAD16_WORD_SWAP( "gmd.06",  0x180000, 0x80000, CRC(55b483c9) SHA1(d47e077312f3c044d3647b79fa9e0581ccff5992) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "gmdux.03", 0x000000, 0x80000, CRC(9f820809) SHA1(6cc77fad178c972be61d23d31bf2e3973ab57f17) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROMX_LOAD( "gmd.13",   0x000000, 0x400000, CRC(80dd19f0) SHA1(0fd8b1e8d73cc83e6c34f0d94487938da2344f76) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "gmd.15",   0x000002, 0x400000, CRC(dfd93a78) SHA1(c343d5ddcc25bd0739491e7439d7c0d0a8881a04) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "gmd.17",   0x000004, 0x400000, CRC(16356520) SHA1(058713bef30c1b1d8b7dd0ceaaa57a3ab9751a70) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "gmd.19",   0x000006, 0x400000, CRC(dfc33031) SHA1(a1ceaeddc2a79d5b79f1b107cac2ef6a5e621e77) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "gmd.01",   0x00000, 0x08000, CRC(3f9bc985) SHA1(1616bbee82877b1052a07531066f5009a80706be) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "gmd.02",   0x28000, 0x20000, CRC(3fd39dde) SHA1(6a6e3ef9baa430ee83ab2312aa0221bae4d73dbd) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "gmd.11",   0x000000, 0x400000, CRC(06a65542) SHA1(a1b3df70c90055a3cd59d0149fd18a74eff5bcc9) )
	ROM_LOAD16_WORD_SWAP( "gmd.12",   0x400000, 0x400000, CRC(50bc7a31) SHA1(7283569fc646c39f4c693f14e0ce7ff2ee49111a) )
ROM_END

ROM_START( gmahou )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "gmdj.03", 0x000000, 0x80000, CRC(cd6979e3) SHA1(b033408f49299eac376fc798c3429e5db97dd4fe) )
	ROM_LOAD16_WORD_SWAP( "gmd.04",  0x080000, 0x80000, CRC(37485567) SHA1(643c41fce6057bcaef0e0bedc62914c33d97eeaf) )
	ROM_LOAD16_WORD_SWAP( "gmd.05",  0x100000, 0x80000, CRC(da269ffb) SHA1(e99b04192030b6006cf67b563f40cea29c1b2e78) )
	ROM_LOAD16_WORD_SWAP( "gmd.06",  0x180000, 0x80000, CRC(55b483c9) SHA1(d47e077312f3c044d3647b79fa9e0581ccff5992) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "gmdjx.03", 0x000000, 0x80000, CRC(c0c8fd1e) SHA1(7864611dd90ec814a3ec39891a00cfc3f6d76870) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROMX_LOAD( "gmd.13",   0x000000, 0x400000, CRC(80dd19f0) SHA1(0fd8b1e8d73cc83e6c34f0d94487938da2344f76) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "gmd.15",   0x000002, 0x400000, CRC(dfd93a78) SHA1(c343d5ddcc25bd0739491e7439d7c0d0a8881a04) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "gmd.17",   0x000004, 0x400000, CRC(16356520) SHA1(058713bef30c1b1d8b7dd0ceaaa57a3ab9751a70) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "gmd.19",   0x000006, 0x400000, CRC(dfc33031) SHA1(a1ceaeddc2a79d5b79f1b107cac2ef6a5e621e77) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "gmd.01",   0x00000, 0x08000, CRC(3f9bc985) SHA1(1616bbee82877b1052a07531066f5009a80706be) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "gmd.02",   0x28000, 0x20000, CRC(3fd39dde) SHA1(6a6e3ef9baa430ee83ab2312aa0221bae4d73dbd) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "gmd.11",   0x000000, 0x400000, CRC(06a65542) SHA1(a1b3df70c90055a3cd59d0149fd18a74eff5bcc9) )
	ROM_LOAD16_WORD_SWAP( "gmd.12",   0x400000, 0x400000, CRC(50bc7a31) SHA1(7283569fc646c39f4c693f14e0ce7ff2ee49111a) )
ROM_END

ROM_START( dstlk )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "vame.03a", 0x000000, 0x80000, CRC(004c9cff) SHA1(9cab8ab734b29abf321b47e46271dab549bf46df) )
	ROM_LOAD16_WORD_SWAP( "vame.04a", 0x080000, 0x80000, CRC(ae413ff2) SHA1(e9b85ac04d6d1a57368c70aa24e3ab8a8d67409f) )
	ROM_LOAD16_WORD_SWAP( "vame.05a", 0x100000, 0x80000, CRC(60678756) SHA1(5d10829ad7522b5de3b318dd8cbf1b506ba4c2d4) )
	ROM_LOAD16_WORD_SWAP( "vame.06a", 0x180000, 0x80000, CRC(912870b3) SHA1(9c7620c7e25d236050411ba94fbc5b3b501970a3) )
	ROM_LOAD16_WORD_SWAP( "vame.07a", 0x200000, 0x80000, CRC(dabae3e8) SHA1(126f8433491db36649f5e1908bbe45eb123048e4) )
	ROM_LOAD16_WORD_SWAP( "vame.08a", 0x280000, 0x80000, CRC(2c6e3077) SHA1(d8042312ec546e3e807e3ef0a14af9b4f716e415) )
	ROM_LOAD16_WORD_SWAP( "vame.09a", 0x300000, 0x80000, CRC(f16db74b) SHA1(7b7e31916a61e7fb35ec20849c6d22d74e169ec0) )
	ROM_LOAD16_WORD_SWAP( "vame.10a", 0x380000, 0x80000, CRC(701e2147) SHA1(c0a0603e01fbed67a600b83902091c1073e2ed27) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "vamex.03a", 0x000000, 0x80000, CRC(2d1e4919) SHA1(804c573caf4f93df55159ba09f237951edc18de0) )
	ROM_LOAD16_WORD_SWAP( "vamex.04a", 0x080000, 0x80000, CRC(e5172837) SHA1(d9f06ddad9f8ba9277dc8b4afb75a3a47f7f5415) )

	ROM_REGION( 0x1400000, REGION_GFX1, 0 )
	ROMX_LOAD( "vam.13",   0x0000000, 0x400000, CRC(c51baf99) SHA1(2fb6642908e542e404391eb17392f8270e87bf48) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.15",   0x0000002, 0x400000, CRC(3ce83c77) SHA1(93369b23c6d7d834297434691bb047ee3dd9539c) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.17",   0x0000004, 0x400000, CRC(4f2408e0) SHA1(cd49c6b3c7e6470c6058f98ccc5210b052bb13e2) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.19",   0x0000006, 0x400000, CRC(9ff60250) SHA1(d69ba4dc6bd37d003245f0cf3211d6e2623005b8) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.14",   0x1000000, 0x100000, CRC(bd87243c) SHA1(87b33aeb72514e1228ffc27ec6dd534f14882760) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.16",   0x1000002, 0x100000, CRC(afec855f) SHA1(cd117833b8d475489b90ff44b57e2c5cb1af3af5) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.18",   0x1000004, 0x100000, CRC(3a033625) SHA1(294238f30cba5cf4f8f1de951d54c2077bd95de9) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.20",   0x1000006, 0x100000, CRC(2bff6a89) SHA1(8f4e131e5ce0af48fb89f98026d9f0356c7c301f) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "vam.01",   0x00000, 0x08000, CRC(64b685d5) SHA1(6c180e7420db754eca5cad17a40f5a64f5c3bd15) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "vam.02",   0x28000, 0x20000, CRC(cf7c97c7) SHA1(109a4b56ecd59be9c3f5869de99d40619bdaef21) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "vam.11",   0x000000, 0x200000, CRC(4a39deb2) SHA1(7e63e615869958db66a4e52a0272afee5a10e446) )
	ROM_LOAD16_WORD_SWAP( "vam.12",   0x200000, 0x200000, CRC(1a3e5c03) SHA1(c5a556e125d6c3d68da745b4d56cd7a851f2a23d) )
ROM_END

ROM_START( dstlku )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "vamu.03b", 0x000000, 0x80000, CRC(68a6343f) SHA1(9e1b13e3419470b3c14065c85342b2dcf42eb4cd) )
	ROM_LOAD16_WORD_SWAP( "vamu.04b", 0x080000, 0x80000, CRC(58161453) SHA1(7b5674b8bdb7e3165e090105f6716073696d4bd0) )
	ROM_LOAD16_WORD_SWAP( "vamu.05b", 0x100000, 0x80000, CRC(dfc038b8) SHA1(1b8911033a458f2d20f740c1bd1b3a2157d24b8a) )
	ROM_LOAD16_WORD_SWAP( "vamu.06b", 0x180000, 0x80000, CRC(c3842c89) SHA1(38137ae2c4ec2a6523413c0891287ad7ae70f005) )
	ROM_LOAD16_WORD_SWAP( "vamu.07b", 0x200000, 0x80000, CRC(25b60b6e) SHA1(8b7dc014d1953a6f4c003811ef8813d46136959d) )
	ROM_LOAD16_WORD_SWAP( "vamu.08b", 0x280000, 0x80000, CRC(2113c596) SHA1(6c0e5c406c08af922920500679eaa89e0b83f029) )
	ROM_LOAD16_WORD_SWAP( "vamu.09b", 0x300000, 0x80000, CRC(2d1e9ae5) SHA1(1c4aced7dd0356ee445ca1e5db2c3a2ad4ee56c6) )
	ROM_LOAD16_WORD_SWAP( "vamu.10b", 0x380000, 0x80000, CRC(81145622) SHA1(66c5439b564cea4b49c47db7e095283481d962c7) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "vamux.03b", 0x000000, 0x80000, CRC(15ff2d3e) SHA1(a9a8043430785e8b38070bacbe66601399286d37) )
	ROM_LOAD16_WORD_SWAP( "vamux.04b", 0x080000, 0x80000, CRC(4cf62f1b) SHA1(b5ef1c9ac163435274ae26b90632afd5df063273) )

	ROM_REGION( 0x1400000, REGION_GFX1, 0 )
	ROMX_LOAD( "vam.13",   0x0000000, 0x400000, CRC(c51baf99) SHA1(2fb6642908e542e404391eb17392f8270e87bf48) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.15",   0x0000002, 0x400000, CRC(3ce83c77) SHA1(93369b23c6d7d834297434691bb047ee3dd9539c) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.17",   0x0000004, 0x400000, CRC(4f2408e0) SHA1(cd49c6b3c7e6470c6058f98ccc5210b052bb13e2) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.19",   0x0000006, 0x400000, CRC(9ff60250) SHA1(d69ba4dc6bd37d003245f0cf3211d6e2623005b8) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.14",   0x1000000, 0x100000, CRC(bd87243c) SHA1(87b33aeb72514e1228ffc27ec6dd534f14882760) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.16",   0x1000002, 0x100000, CRC(afec855f) SHA1(cd117833b8d475489b90ff44b57e2c5cb1af3af5) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.18",   0x1000004, 0x100000, CRC(3a033625) SHA1(294238f30cba5cf4f8f1de951d54c2077bd95de9) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.20",   0x1000006, 0x100000, CRC(2bff6a89) SHA1(8f4e131e5ce0af48fb89f98026d9f0356c7c301f) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "vam.01",   0x00000, 0x08000, CRC(64b685d5) SHA1(6c180e7420db754eca5cad17a40f5a64f5c3bd15) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "vam.02",   0x28000, 0x20000, CRC(cf7c97c7) SHA1(109a4b56ecd59be9c3f5869de99d40619bdaef21) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "vam.11",   0x000000, 0x200000, CRC(4a39deb2) SHA1(7e63e615869958db66a4e52a0272afee5a10e446) )
	ROM_LOAD16_WORD_SWAP( "vam.12",   0x200000, 0x200000, CRC(1a3e5c03) SHA1(c5a556e125d6c3d68da745b4d56cd7a851f2a23d) )
ROM_END

ROM_START( dstlkur1 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "vamu.03a", 0x000000, 0x80000, CRC(628899f9) SHA1(989414a62aed67504f15a542a148e32a4b349949) )
	ROM_LOAD16_WORD_SWAP( "vamu.04a", 0x080000, 0x80000, CRC(696d9b25) SHA1(743c53ac7fc27960ecc80fed3f2a3c506ee655a1) )
	ROM_LOAD16_WORD_SWAP( "vamu.05a", 0x100000, 0x80000, CRC(673ed50a) SHA1(7dff27dba1da55a18eb459e4a2d679cf699f2804) )
	ROM_LOAD16_WORD_SWAP( "vamu.06a", 0x180000, 0x80000, CRC(f2377be7) SHA1(4520d44f94a03bd40c27062344e56ba8718c2fb8) )
	ROM_LOAD16_WORD_SWAP( "vamu.07a", 0x200000, 0x80000, CRC(d8f498c4) SHA1(569d9c309e9d95d2501a7c0a2c1291b49320d767) )
	ROM_LOAD16_WORD_SWAP( "vamu.08a", 0x280000, 0x80000, CRC(e6a8a1a0) SHA1(adf621e12623a2af4dbf0858a8fa3816e7c7073b) )
	ROM_LOAD16_WORD_SWAP( "vamu.09a", 0x300000, 0x80000, CRC(8dd55b24) SHA1(d99c2cbc4a9899a3d187201e6e730b7b8fb13d1d) )
	ROM_LOAD16_WORD_SWAP( "vamu.10a", 0x380000, 0x80000, CRC(c1a3d9be) SHA1(82b4ce3325a7ecf3a60dd781f9b224fdde8daa65) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "vamux.03a", 0x000000, 0x80000, CRC(db0837f5) SHA1(a90b2f0b2d20f5c970813bb296f21cf6fd7bcfb0) )
	ROM_LOAD16_WORD_SWAP( "vamux.04a", 0x080000, 0x80000, CRC(8a924055) SHA1(83399b1808400c5754595007e2699250852a8049) )

	ROM_REGION( 0x1400000, REGION_GFX1, 0 )
	ROMX_LOAD( "vam.13",   0x0000000, 0x400000, CRC(c51baf99) SHA1(2fb6642908e542e404391eb17392f8270e87bf48) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.15",   0x0000002, 0x400000, CRC(3ce83c77) SHA1(93369b23c6d7d834297434691bb047ee3dd9539c) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.17",   0x0000004, 0x400000, CRC(4f2408e0) SHA1(cd49c6b3c7e6470c6058f98ccc5210b052bb13e2) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.19",   0x0000006, 0x400000, CRC(9ff60250) SHA1(d69ba4dc6bd37d003245f0cf3211d6e2623005b8) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.14",   0x1000000, 0x100000, CRC(bd87243c) SHA1(87b33aeb72514e1228ffc27ec6dd534f14882760) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.16",   0x1000002, 0x100000, CRC(afec855f) SHA1(cd117833b8d475489b90ff44b57e2c5cb1af3af5) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.18",   0x1000004, 0x100000, CRC(3a033625) SHA1(294238f30cba5cf4f8f1de951d54c2077bd95de9) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.20",   0x1000006, 0x100000, CRC(2bff6a89) SHA1(8f4e131e5ce0af48fb89f98026d9f0356c7c301f) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "vam.01",   0x00000, 0x08000, CRC(64b685d5) SHA1(6c180e7420db754eca5cad17a40f5a64f5c3bd15) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "vam.02",   0x28000, 0x20000, CRC(cf7c97c7) SHA1(109a4b56ecd59be9c3f5869de99d40619bdaef21) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "vam.11",   0x000000, 0x200000, CRC(4a39deb2) SHA1(7e63e615869958db66a4e52a0272afee5a10e446) )
	ROM_LOAD16_WORD_SWAP( "vam.12",   0x200000, 0x200000, CRC(1a3e5c03) SHA1(c5a556e125d6c3d68da745b4d56cd7a851f2a23d) )
ROM_END

ROM_START( vampj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "vamj.03a", 0x000000, 0x80000, CRC(f55d3722) SHA1(e26bbcc47a2485914d567a6cf1cddd0f668689a1) )
	ROM_LOAD16_WORD_SWAP( "vamj.04b", 0x080000, 0x80000, CRC(4d9c43c4) SHA1(2087090306646fed959d503ee75e24996ad95b88) )
	ROM_LOAD16_WORD_SWAP( "vamj.05a", 0x100000, 0x80000, CRC(6c497e92) SHA1(7c1ccdfd77fb50afe024c8402376daaeab641a24) )
	ROM_LOAD16_WORD_SWAP( "vamj.06a", 0x180000, 0x80000, CRC(f1bbecb6) SHA1(6adba89393e05f16f70b57085cabd6b4c20f53e8) )
	ROM_LOAD16_WORD_SWAP( "vamj.07a", 0x200000, 0x80000, CRC(1067ad84) SHA1(5e4cc75cfdfd512b6230c656e7304262b5143aee) )
	ROM_LOAD16_WORD_SWAP( "vamj.08a", 0x280000, 0x80000, CRC(4b89f41f) SHA1(bd78f33a6d448655eecf7448921d282b302fa4cb) )
	ROM_LOAD16_WORD_SWAP( "vamj.09a", 0x300000, 0x80000, CRC(fc0a4aac) SHA1(a2c79eb4dc838c238e182a4da3567ac8db3488d8) )
	ROM_LOAD16_WORD_SWAP( "vamj.10a", 0x380000, 0x80000, CRC(9270c26b) SHA1(c2a7e199a74c9f27704cf935483ebddc6da256a1) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "vamjx.03a", 0x000000, 0x80000, CRC(2549f7bc) SHA1(de2262756c61a4309a35d2548ed2cc68a7e924b7) )
	ROM_LOAD16_WORD_SWAP( "vamjx.04b", 0x080000, 0x80000, CRC(bb5a30a5) SHA1(b8703ce0bcb99e969d4e2b76e9bd0b3eefac01b5) )

	ROM_REGION( 0x1400000, REGION_GFX1, 0 )
	ROMX_LOAD( "vam.13",   0x0000000, 0x400000, CRC(c51baf99) SHA1(2fb6642908e542e404391eb17392f8270e87bf48) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.15",   0x0000002, 0x400000, CRC(3ce83c77) SHA1(93369b23c6d7d834297434691bb047ee3dd9539c) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.17",   0x0000004, 0x400000, CRC(4f2408e0) SHA1(cd49c6b3c7e6470c6058f98ccc5210b052bb13e2) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.19",   0x0000006, 0x400000, CRC(9ff60250) SHA1(d69ba4dc6bd37d003245f0cf3211d6e2623005b8) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.14",   0x1000000, 0x100000, CRC(bd87243c) SHA1(87b33aeb72514e1228ffc27ec6dd534f14882760) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.16",   0x1000002, 0x100000, CRC(afec855f) SHA1(cd117833b8d475489b90ff44b57e2c5cb1af3af5) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.18",   0x1000004, 0x100000, CRC(3a033625) SHA1(294238f30cba5cf4f8f1de951d54c2077bd95de9) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.20",   0x1000006, 0x100000, CRC(2bff6a89) SHA1(8f4e131e5ce0af48fb89f98026d9f0356c7c301f) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "vam.01",   0x00000, 0x08000, CRC(64b685d5) SHA1(6c180e7420db754eca5cad17a40f5a64f5c3bd15) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "vam.02",   0x28000, 0x20000, CRC(cf7c97c7) SHA1(109a4b56ecd59be9c3f5869de99d40619bdaef21) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "vam.11",   0x000000, 0x200000, CRC(4a39deb2) SHA1(7e63e615869958db66a4e52a0272afee5a10e446) )
	ROM_LOAD16_WORD_SWAP( "vam.12",   0x200000, 0x200000, CRC(1a3e5c03) SHA1(c5a556e125d6c3d68da745b4d56cd7a851f2a23d) )
ROM_END

ROM_START( vampja )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "vamj.03a", 0x000000, 0x80000, CRC(f55d3722) SHA1(e26bbcc47a2485914d567a6cf1cddd0f668689a1) )
	ROM_LOAD16_WORD_SWAP( "vamj.04a", 0x080000, 0x80000, CRC(fdcbdae3) SHA1(46a1251a2affbe13b200448cf77a455d840f3c9f) )
	ROM_LOAD16_WORD_SWAP( "vamj.05a", 0x100000, 0x80000, CRC(6c497e92) SHA1(7c1ccdfd77fb50afe024c8402376daaeab641a24) )
	ROM_LOAD16_WORD_SWAP( "vamj.06a", 0x180000, 0x80000, CRC(f1bbecb6) SHA1(6adba89393e05f16f70b57085cabd6b4c20f53e8) )
	ROM_LOAD16_WORD_SWAP( "vamj.07a", 0x200000, 0x80000, CRC(1067ad84) SHA1(5e4cc75cfdfd512b6230c656e7304262b5143aee) )
	ROM_LOAD16_WORD_SWAP( "vamj.08a", 0x280000, 0x80000, CRC(4b89f41f) SHA1(bd78f33a6d448655eecf7448921d282b302fa4cb) )
	ROM_LOAD16_WORD_SWAP( "vamj.09a", 0x300000, 0x80000, CRC(fc0a4aac) SHA1(a2c79eb4dc838c238e182a4da3567ac8db3488d8) )
	ROM_LOAD16_WORD_SWAP( "vamj.10a", 0x380000, 0x80000, CRC(9270c26b) SHA1(c2a7e199a74c9f27704cf935483ebddc6da256a1) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "vamjx.03a", 0x000000, 0x80000, CRC(2549f7bc) SHA1(de2262756c61a4309a35d2548ed2cc68a7e924b7) )
	ROM_LOAD16_WORD_SWAP( "vamjx.04a", 0x080000, 0x80000, CRC(fe64a5cf) SHA1(f58c03bc27460826718ae91faa3d4bb28237c994) )

	ROM_REGION( 0x1400000, REGION_GFX1, 0 )
	ROMX_LOAD( "vam.13",   0x0000000, 0x400000, CRC(c51baf99) SHA1(2fb6642908e542e404391eb17392f8270e87bf48) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.15",   0x0000002, 0x400000, CRC(3ce83c77) SHA1(93369b23c6d7d834297434691bb047ee3dd9539c) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.17",   0x0000004, 0x400000, CRC(4f2408e0) SHA1(cd49c6b3c7e6470c6058f98ccc5210b052bb13e2) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.19",   0x0000006, 0x400000, CRC(9ff60250) SHA1(d69ba4dc6bd37d003245f0cf3211d6e2623005b8) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.14",   0x1000000, 0x100000, CRC(bd87243c) SHA1(87b33aeb72514e1228ffc27ec6dd534f14882760) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.16",   0x1000002, 0x100000, CRC(afec855f) SHA1(cd117833b8d475489b90ff44b57e2c5cb1af3af5) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.18",   0x1000004, 0x100000, CRC(3a033625) SHA1(294238f30cba5cf4f8f1de951d54c2077bd95de9) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.20",   0x1000006, 0x100000, CRC(2bff6a89) SHA1(8f4e131e5ce0af48fb89f98026d9f0356c7c301f) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "vam.01",   0x00000, 0x08000, CRC(64b685d5) SHA1(6c180e7420db754eca5cad17a40f5a64f5c3bd15) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "vam.02",   0x28000, 0x20000, CRC(cf7c97c7) SHA1(109a4b56ecd59be9c3f5869de99d40619bdaef21) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "vam.11",   0x000000, 0x200000, CRC(4a39deb2) SHA1(7e63e615869958db66a4e52a0272afee5a10e446) )
	ROM_LOAD16_WORD_SWAP( "vam.12",   0x200000, 0x200000, CRC(1a3e5c03) SHA1(c5a556e125d6c3d68da745b4d56cd7a851f2a23d) )
ROM_END

ROM_START( vampjr1 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "vamj.03", 0x000000, 0x80000, CRC(8895bf77) SHA1(7977dad8c4baf89f28668f54225233a8e759aa3e) )
	ROM_LOAD16_WORD_SWAP( "vamj.04", 0x080000, 0x80000, CRC(5027db3d) SHA1(64bd09f2b5fd2435d8ec86f64543b640ab08f82f) )
	ROM_LOAD16_WORD_SWAP( "vamj.05", 0x100000, 0x80000, CRC(97c66fdb) SHA1(fe5c099dd29797aef28a247913f8931aa8ce6160) )
	ROM_LOAD16_WORD_SWAP( "vamj.06", 0x180000, 0x80000, CRC(9b4c3426) SHA1(a527535e5d23c3d12bac7617fd5d8e15c2522bbd) )
	ROM_LOAD16_WORD_SWAP( "vamj.07", 0x200000, 0x80000, CRC(303bc4fd) SHA1(2e3b687c725d389afa7c3e1fe8720a53d0f40269) )
	ROM_LOAD16_WORD_SWAP( "vamj.08", 0x280000, 0x80000, CRC(3dea3646) SHA1(3b3f7105284a04b12b3de40633bc8f21a8d73f58) )
	ROM_LOAD16_WORD_SWAP( "vamj.09", 0x300000, 0x80000, CRC(c119a827) SHA1(422864dda2a12621175350b8a130f970ed690719) )
	ROM_LOAD16_WORD_SWAP( "vamj.10", 0x380000, 0x80000, CRC(46593b79) SHA1(ff003cc80ed4f3cfaff722b43a09076828c9a9d7) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "vamjx.03", 0x000000, 0x80000, CRC(f6fe646a) SHA1(3f67cad0deb128418a693eca3697a12e0f6be6b0) )
	ROM_LOAD16_WORD_SWAP( "vamjx.04", 0x080000, 0x80000, CRC(566014e5) SHA1(11ba392b2537d2a88e87b1109dab31ac442dcbaf) )

	ROM_REGION( 0x1400000, REGION_GFX1, 0 )
	ROMX_LOAD( "vam.13",   0x0000000, 0x400000, CRC(c51baf99) SHA1(2fb6642908e542e404391eb17392f8270e87bf48) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.15",   0x0000002, 0x400000, CRC(3ce83c77) SHA1(93369b23c6d7d834297434691bb047ee3dd9539c) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.17",   0x0000004, 0x400000, CRC(4f2408e0) SHA1(cd49c6b3c7e6470c6058f98ccc5210b052bb13e2) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.19",   0x0000006, 0x400000, CRC(9ff60250) SHA1(d69ba4dc6bd37d003245f0cf3211d6e2623005b8) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.14",   0x1000000, 0x100000, CRC(bd87243c) SHA1(87b33aeb72514e1228ffc27ec6dd534f14882760) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.16",   0x1000002, 0x100000, CRC(afec855f) SHA1(cd117833b8d475489b90ff44b57e2c5cb1af3af5) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.18",   0x1000004, 0x100000, CRC(3a033625) SHA1(294238f30cba5cf4f8f1de951d54c2077bd95de9) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.20",   0x1000006, 0x100000, CRC(2bff6a89) SHA1(8f4e131e5ce0af48fb89f98026d9f0356c7c301f) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "vam.01",   0x00000, 0x08000, CRC(64b685d5) SHA1(6c180e7420db754eca5cad17a40f5a64f5c3bd15) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "vam.02",   0x28000, 0x20000, CRC(cf7c97c7) SHA1(109a4b56ecd59be9c3f5869de99d40619bdaef21) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "vam.11",   0x000000, 0x200000, CRC(4a39deb2) SHA1(7e63e615869958db66a4e52a0272afee5a10e446) )
	ROM_LOAD16_WORD_SWAP( "vam.12",   0x200000, 0x200000, CRC(1a3e5c03) SHA1(c5a556e125d6c3d68da745b4d56cd7a851f2a23d) )
ROM_END

ROM_START( dstlka )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "vama.03a", 0x000000, 0x80000, CRC(294e0bec) SHA1(e90844cd18ad431e999e606d076738384b346b9d) )
	ROM_LOAD16_WORD_SWAP( "vama.04a", 0x080000, 0x80000, CRC(bc18e128) SHA1(53116cddb7123b573d76064640c3829fd978c67a) )
	ROM_LOAD16_WORD_SWAP( "vama.05a", 0x100000, 0x80000, CRC(e709fa59) SHA1(824d2b22c5627b9dba046b76c1ff5a46f577eddd) )
	ROM_LOAD16_WORD_SWAP( "vama.06a", 0x180000, 0x80000, CRC(55e4d387) SHA1(c8b9be072e5de44e6d50f7a80d4c79ae1449588e) )
	ROM_LOAD16_WORD_SWAP( "vama.07a", 0x200000, 0x80000, CRC(24e8f981) SHA1(5dd28efa325fded290d9eb1643be83ab84a2ac8e) )
	ROM_LOAD16_WORD_SWAP( "vama.08a", 0x280000, 0x80000, CRC(743f3a8e) SHA1(f7bde0f989582ba2cf93c9397cc38d3eec9ad92d) )
	ROM_LOAD16_WORD_SWAP( "vama.09a", 0x300000, 0x80000, CRC(67fa5573) SHA1(2dab32cf0d361d2c52cce5eb41b389a0e32dd192) )
	ROM_LOAD16_WORD_SWAP( "vama.10a", 0x380000, 0x80000, CRC(5e03d747) SHA1(044ef85ca927108f5e66967819dbf7c25bb34f77) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "vamax.03a", 0x000000, 0x80000, CRC(9b7d6254) SHA1(8208ee3f9da9a294f2cf2b64d42a3012673ffc85) )
	ROM_LOAD16_WORD_SWAP( "vamax.04a", 0x080000, 0x80000, CRC(d9ec3a82) SHA1(4afaaa38a736efb7a21f771a190a54e40b57edcc) )

	ROM_REGION( 0x1400000, REGION_GFX1, 0 )
	ROMX_LOAD( "vam.13",   0x0000000, 0x400000, CRC(c51baf99) SHA1(2fb6642908e542e404391eb17392f8270e87bf48) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.15",   0x0000002, 0x400000, CRC(3ce83c77) SHA1(93369b23c6d7d834297434691bb047ee3dd9539c) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.17",   0x0000004, 0x400000, CRC(4f2408e0) SHA1(cd49c6b3c7e6470c6058f98ccc5210b052bb13e2) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.19",   0x0000006, 0x400000, CRC(9ff60250) SHA1(d69ba4dc6bd37d003245f0cf3211d6e2623005b8) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.14",   0x1000000, 0x100000, CRC(bd87243c) SHA1(87b33aeb72514e1228ffc27ec6dd534f14882760) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.16",   0x1000002, 0x100000, CRC(afec855f) SHA1(cd117833b8d475489b90ff44b57e2c5cb1af3af5) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.18",   0x1000004, 0x100000, CRC(3a033625) SHA1(294238f30cba5cf4f8f1de951d54c2077bd95de9) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.20",   0x1000006, 0x100000, CRC(2bff6a89) SHA1(8f4e131e5ce0af48fb89f98026d9f0356c7c301f) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "vam.01",   0x00000, 0x08000, CRC(64b685d5) SHA1(6c180e7420db754eca5cad17a40f5a64f5c3bd15) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "vam.02",   0x28000, 0x20000, CRC(cf7c97c7) SHA1(109a4b56ecd59be9c3f5869de99d40619bdaef21) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "vam.11",   0x000000, 0x200000, CRC(4a39deb2) SHA1(7e63e615869958db66a4e52a0272afee5a10e446) )
	ROM_LOAD16_WORD_SWAP( "vam.12",   0x200000, 0x200000, CRC(1a3e5c03) SHA1(c5a556e125d6c3d68da745b4d56cd7a851f2a23d) )
ROM_END

ROM_START( ecofghtr )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "uece.03", 0x000000, 0x80000, CRC(ec2c1137) SHA1(19c5b68cccd682d3996faf8c5f07a644b2384b1c) )
	ROM_LOAD16_WORD_SWAP( "uece.04", 0x080000, 0x80000, CRC(b35f99db) SHA1(4dd5c4840406a9323431f5bda7224cadacf8b419) )
	ROM_LOAD16_WORD_SWAP( "uece.05", 0x100000, 0x80000, CRC(d9d42d31) SHA1(58e7438fa212655ca56cbb477ea353e1083e0933) )
	ROM_LOAD16_WORD_SWAP( "uece.06", 0x180000, 0x80000, CRC(9d9771cf) SHA1(d1c76672f2e0437cd1204d5552d32ed3377c1356) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "uecex.03", 0x000000, 0x80000, CRC(e0ff3d51) SHA1(fe1ebbed46d44ade3b6de8d326c7d25217bda104) )
	ROM_LOAD16_WORD_SWAP( "uecex.04", 0x080000, 0x80000, CRC(b9f998e8) SHA1(ef617f18822546a0bf233d315a6d9edfbf704abd) )
	ROM_LOAD16_WORD_SWAP( "uecex.05", 0x100000, 0x80000, CRC(12410260) SHA1(19ae7d8f175804f464dd07bc6c1b7b250f511b5f) )
	ROM_LOAD16_WORD_SWAP( "uecex.06", 0x180000, 0x80000, CRC(d5b4b1a2) SHA1(0093f805e4ca60b522586de12cdf3a7c8bea6395) )

	ROM_REGION( 0xc00000, REGION_GFX1, 0 )
	ROMX_LOAD( "uec.13",   0x000000, 0x200000, CRC(dcaf1436) SHA1(ba124cc0bb10c1d1c07592a3623add4ed054182e) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "uec.15",   0x000002, 0x200000, CRC(2807df41) SHA1(66a9800af435055737ce50a0b0ced7c5718c2004) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "uec.17",   0x000004, 0x200000, CRC(8a708d02) SHA1(95ec527edc904a66e325667521b4d07d72579211) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "uec.19",   0x000006, 0x200000, CRC(de7be0ef) SHA1(bf8df9a31f8923f4b726ea12fe8327368463ebe1) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "uec.14",   0x800000, 0x100000, CRC(1a003558) SHA1(64bbd89e65dc0cf6f4ab5ea93a4cc6312d0d0802) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "uec.16",   0x800002, 0x100000, CRC(4ff8a6f9) SHA1(03968a301417e8843d42d4e0db42aa0a3a38664b) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "uec.18",   0x800004, 0x100000, CRC(b167ae12) SHA1(48c552d02caad27d680aa51170560794f2a51478) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "uec.20",   0x800006, 0x100000, CRC(1064bdc2) SHA1(c51f75ac8d3f02a771feda0a933314a928555c4e) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "uec.01",   0x00000, 0x08000, CRC(c235bd15) SHA1(feb7cd7db9dc0b9887b33eed9796bb0205fb719d) )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "uec.11",   0x000000, 0x200000, CRC(81b25d39) SHA1(448adfcc7d98873a48c710d857225cdd1580e5c9) )
	ROM_LOAD16_WORD_SWAP( "uec.12",   0x200000, 0x200000, CRC(27729e52) SHA1(a55c8159adf766dda70cb047f5ac85ce6bc0a3f3) )
ROM_END

ROM_START( uecology )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "uecj.03", 0x000000, 0x80000, CRC(94c40a4c) SHA1(6446b22a30a9a3c87b7a9fc2f15fbceccfbfb942) )
	ROM_LOAD16_WORD_SWAP( "uecj.04", 0x080000, 0x80000, CRC(8d6e3a09) SHA1(80167275f288a4c4b2bb61bdde956015f4206b78) )
	ROM_LOAD16_WORD_SWAP( "uecj.05", 0x100000, 0x80000, CRC(8604ecd7) SHA1(e1690565b40db84f4ce30e6eb2d7940b82989678) )
	ROM_LOAD16_WORD_SWAP( "uecj.06", 0x180000, 0x80000, CRC(b7e1d31f) SHA1(6567f14af9fd567dea963fda5cd37c55cab30704) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
/*	ROM_LOAD16_WORD_SWAP( "uecjx.03", 0x000000, 0x80000, NO_DUMP )*/
/*	ROM_LOAD16_WORD_SWAP( "uecjx.04", 0x080000, 0x80000, NO_DUMP )*/
/*	ROM_LOAD16_WORD_SWAP( "uecjx.05", 0x100000, 0x80000, NO_DUMP )*/
/*	ROM_LOAD16_WORD_SWAP( "uecjx.06", 0x180000, 0x80000, NO_DUMP )*/

	ROM_REGION( 0xc00000, REGION_GFX1, 0 )
	ROMX_LOAD( "uec.13",   0x000000, 0x200000, CRC(dcaf1436) SHA1(ba124cc0bb10c1d1c07592a3623add4ed054182e) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "uec.15",   0x000002, 0x200000, CRC(2807df41) SHA1(66a9800af435055737ce50a0b0ced7c5718c2004) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "uec.17",   0x000004, 0x200000, CRC(8a708d02) SHA1(95ec527edc904a66e325667521b4d07d72579211) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "uec.19",   0x000006, 0x200000, CRC(de7be0ef) SHA1(bf8df9a31f8923f4b726ea12fe8327368463ebe1) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "uec.14",   0x800000, 0x100000, CRC(1a003558) SHA1(64bbd89e65dc0cf6f4ab5ea93a4cc6312d0d0802) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "uec.16",   0x800002, 0x100000, CRC(4ff8a6f9) SHA1(03968a301417e8843d42d4e0db42aa0a3a38664b) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "uec.18",   0x800004, 0x100000, CRC(b167ae12) SHA1(48c552d02caad27d680aa51170560794f2a51478) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "uec.20",   0x800006, 0x100000, CRC(1064bdc2) SHA1(c51f75ac8d3f02a771feda0a933314a928555c4e) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "uec.01",   0x00000, 0x08000, CRC(c235bd15) SHA1(feb7cd7db9dc0b9887b33eed9796bb0205fb719d) )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "uec.11",   0x000000, 0x200000, CRC(81b25d39) SHA1(448adfcc7d98873a48c710d857225cdd1580e5c9) )
	ROM_LOAD16_WORD_SWAP( "uec.12",   0x200000, 0x200000, CRC(27729e52) SHA1(a55c8159adf766dda70cb047f5ac85ce6bc0a3f3) )
ROM_END

ROM_START( ecofghta )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "ueca.03", 0x000000, 0x80000, CRC(bd4589b1) SHA1(8ec03a750de155c6ce0a2c3a6b57e6a6dcaf9ebc) )
	ROM_LOAD16_WORD_SWAP( "ueca.04", 0x080000, 0x80000, CRC(1d134b7d) SHA1(c9dd725ff45f29a3fa68bfe6d5aea2e8c3c64bd8) )
	ROM_LOAD16_WORD_SWAP( "ueca.05", 0x100000, 0x80000, CRC(9c581fc7) SHA1(300983148da59da7d2fcbc5bc45b068fdfbcb512) )
	ROM_LOAD16_WORD_SWAP( "ueca.06", 0x180000, 0x80000, CRC(c92a7c50) SHA1(820dfa8fff32404caee65a7a5bcf7cafa9939f74) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
/*	ROM_LOAD16_WORD_SWAP( "uecax.03", 0x000000, 0x80000, NO_DUMP )*/
/*	ROM_LOAD16_WORD_SWAP( "uecax.04", 0x080000, 0x80000, NO_DUMP )*/
/*	ROM_LOAD16_WORD_SWAP( "uecax.05", 0x100000, 0x80000, NO_DUMP )*/
/*	ROM_LOAD16_WORD_SWAP( "uecax.06", 0x180000, 0x80000, NO_DUMP )*/

	ROM_REGION( 0xc00000, REGION_GFX1, 0 )
	ROMX_LOAD( "uec.13",   0x000000, 0x200000, CRC(dcaf1436) SHA1(ba124cc0bb10c1d1c07592a3623add4ed054182e) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "uec.15",   0x000002, 0x200000, CRC(2807df41) SHA1(66a9800af435055737ce50a0b0ced7c5718c2004) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "uec.17",   0x000004, 0x200000, CRC(8a708d02) SHA1(95ec527edc904a66e325667521b4d07d72579211) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "uec.19",   0x000006, 0x200000, CRC(de7be0ef) SHA1(bf8df9a31f8923f4b726ea12fe8327368463ebe1) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "uec.14",   0x800000, 0x100000, CRC(1a003558) SHA1(64bbd89e65dc0cf6f4ab5ea93a4cc6312d0d0802) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "uec.16",   0x800002, 0x100000, CRC(4ff8a6f9) SHA1(03968a301417e8843d42d4e0db42aa0a3a38664b) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "uec.18",   0x800004, 0x100000, CRC(b167ae12) SHA1(48c552d02caad27d680aa51170560794f2a51478) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "uec.20",   0x800006, 0x100000, CRC(1064bdc2) SHA1(c51f75ac8d3f02a771feda0a933314a928555c4e) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "uec.01",   0x00000, 0x08000, CRC(c235bd15) SHA1(feb7cd7db9dc0b9887b33eed9796bb0205fb719d) )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "uec.11",   0x000000, 0x200000, CRC(81b25d39) SHA1(448adfcc7d98873a48c710d857225cdd1580e5c9) )
	ROM_LOAD16_WORD_SWAP( "uec.12",   0x200000, 0x200000, CRC(27729e52) SHA1(a55c8159adf766dda70cb047f5ac85ce6bc0a3f3) )
ROM_END

ROM_START( gigawing )
	ROM_REGION(CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "ggwu.03", 0x000000, 0x80000, CRC(ac725eb2) SHA1(a4be9fe537cdb47b37478c8397f6effe8a536233) )
	ROM_LOAD16_WORD_SWAP( "ggwu.04", 0x080000, 0x80000, CRC(392f4118) SHA1(3bb0bd9503ef60892d5abd8640af524cf71da848) )
	ROM_LOAD16_WORD_SWAP( "ggwu.05", 0x100000, 0x80000, CRC(3239d642) SHA1(2fe3984c46a72aedb30a28e3db5af2612bdf0045) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "ggwux.03", 0x000000, 0x80000, CRC(2f18fd96) SHA1(b3c438906f14740a6dde176fd822018d433e1024) )
	ROM_LOAD16_WORD_SWAP( "ggwux.04", 0x080000, 0x80000, CRC(7be82def) SHA1(fd651fea8c9b934079d859a50e3d8fd70b39a550) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROMX_LOAD( "ggw.13",   0x000000, 0x400000, CRC(105530a4) SHA1(3be06c032985ea6bd3805d73a407bf748385087b) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ggw.15",   0x000002, 0x400000, CRC(9e774ab9) SHA1(adea1e844f3d9ccd5ad116ff8277f16a96e68d76) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ggw.17",   0x000004, 0x400000, CRC(466e0ba4) SHA1(9563455b95d36fafe508290659088b153539cfdf) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ggw.19",   0x000006, 0x400000, CRC(840c8dea) SHA1(ea04afce17f00b45d3d2cd5140d0dd7ab4bccc00) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION(QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "ggw.01",   0x00000, 0x08000, CRC(4c6351d5) SHA1(cef81fb7c4b8cb2ef1f8f3c27982aefbcbe38160) )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "ggw.11",   0x000000, 0x400000, CRC(e172acf5) SHA1(d7b0963d66165f3607d887741c5e7ab952bcf2ff) )
	ROM_LOAD16_WORD_SWAP( "ggw.12",   0x400000, 0x400000, CRC(4bee4e8f) SHA1(c440b5a38359ec3b8002f39690b79bf78703f5d0) )
ROM_END

ROM_START( gwingj )
	ROM_REGION(CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "ggwj.03a", 0x000000, 0x80000, CRC(fdd23b91) SHA1(c805473d3dc6bdb7ce232a9d7181d213544b2e7b) )
	ROM_LOAD16_WORD_SWAP( "ggwj.04a", 0x080000, 0x80000, CRC(8c6e093c) SHA1(a4864b3b54cf648af81f74e2936d2bb8b99d68a9) )
	ROM_LOAD16_WORD_SWAP( "ggwj.05a", 0x100000, 0x80000, CRC(43811454) SHA1(2a9563c840bd934c7e94f434a01686b7ff92e6d2) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "ggwjx.03a", 0x000000, 0x80000, CRC(deb45124) SHA1(b15292882108726a9215b1e500594e0f921ee94f) )
	ROM_LOAD16_WORD_SWAP( "ggwjx.04a", 0x080000, 0x80000, CRC(8b981d04) SHA1(a4764710932544da35a607850ec2b70a366652d9) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROMX_LOAD( "ggw.13",   0x000000, 0x400000, CRC(105530a4) SHA1(3be06c032985ea6bd3805d73a407bf748385087b) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ggw.15",   0x000002, 0x400000, CRC(9e774ab9) SHA1(adea1e844f3d9ccd5ad116ff8277f16a96e68d76) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ggw.17",   0x000004, 0x400000, CRC(466e0ba4) SHA1(9563455b95d36fafe508290659088b153539cfdf) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ggw.19",   0x000006, 0x400000, CRC(840c8dea) SHA1(ea04afce17f00b45d3d2cd5140d0dd7ab4bccc00) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION(QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "ggw.01",   0x00000, 0x08000, CRC(4c6351d5) SHA1(cef81fb7c4b8cb2ef1f8f3c27982aefbcbe38160) )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "ggw.11",   0x000000, 0x400000, CRC(e172acf5) SHA1(d7b0963d66165f3607d887741c5e7ab952bcf2ff) )
	ROM_LOAD16_WORD_SWAP( "ggw.12",   0x400000, 0x400000, CRC(4bee4e8f) SHA1(c440b5a38359ec3b8002f39690b79bf78703f5d0) )
ROM_END

ROM_START( megaman2 )
	ROM_REGION(CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "rm2u.03", 0x000000, 0x80000, CRC(8ffc2cd1) SHA1(919ef08311008288b31ed42fb13172580d50433a) )
	ROM_LOAD16_WORD_SWAP( "rm2u.04", 0x080000, 0x80000, CRC(bb30083a) SHA1(466b818a01ad367a8df6df8661f616f5a0236714) )
	ROM_LOAD16_WORD_SWAP( "rm2.05",  0x100000, 0x80000, CRC(02ee9efc) SHA1(1b80c40389b51a03b930051f232630616c12e6c5) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "rm2ux.03", 0x000000, 0x80000, CRC(93e28bde) SHA1(b71779d095556308c6eb9264bfa1e5d269dd3e8b) )
	ROM_LOAD16_WORD_SWAP( "rm2ux.04", 0x080000, 0x80000, CRC(74ebf83c) SHA1(e0831c188816f13b8ce48ded556bb51dd8cd026c) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROM_FILL(              0x000000, 0x800000, 0 )
	ROMX_LOAD( "rm2.14",   0x800000, 0x200000, CRC(9b1f00b4) SHA1(c1c5c2d9d00121425ae6598444d704f420ef4eef) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "rm2.16",   0x800002, 0x200000, CRC(c2bb0c24) SHA1(38724c49d9db49765a4ed9bc2dc8f57cec45ec7c) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "rm2.18",   0x800004, 0x200000, CRC(12257251) SHA1(20cb58afda0e6200991277817485340a6a41ae2b) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "rm2.20",   0x800006, 0x200000, CRC(f9b6e786) SHA1(aeb4acff7208e66a35198143fd2478039fdaa3a6) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION(QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "rm2.01a",  0x00000, 0x08000, CRC(d18e7859) SHA1(0939fac70042d0b4db5c2fdcef1f79b95febd45e) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "rm2.02",   0x28000, 0x20000, CRC(c463ece0) SHA1(5c3e41eb61610b3f8c431206f6672907e3a0bdb0) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "rm2.11",   0x000000, 0x200000, CRC(2106174d) SHA1(0a35d9ca8ebcad74904b20648d5320f839d6377e) )
	ROM_LOAD16_WORD_SWAP( "rm2.12",   0x200000, 0x200000, CRC(546c1636) SHA1(f96b172ab899f2c6ee17a5dd1fb61af9432e3cd2) )
ROM_END

ROM_START( megamn2a )
	ROM_REGION(CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "rm2a.03", 0x000000, 0x80000, CRC(2b330ca7) SHA1(afa86ef73f5660600d18ff221ed135c026042e05) )
	ROM_LOAD16_WORD_SWAP( "rm2a.04", 0x080000, 0x80000, CRC(8b47942b) SHA1(160574a38e89d31b975c56264f3f5a7a68ce760c) )
	ROM_LOAD16_WORD_SWAP( "rm2.05",  0x100000, 0x80000, CRC(02ee9efc) SHA1(1b80c40389b51a03b930051f232630616c12e6c5) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
/*	ROM_LOAD16_WORD_SWAP( "rm2ax.03", 0x000000, 0x80000, NO_DUMP )*/
/*	ROM_LOAD16_WORD_SWAP( "rm2ax.04", 0x080000, 0x80000, NO_DUMP )*/

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROM_FILL(              0x000000, 0x800000, 0 )
	ROMX_LOAD( "rm2.14",   0x800000, 0x200000, CRC(9b1f00b4) SHA1(c1c5c2d9d00121425ae6598444d704f420ef4eef) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "rm2.16",   0x800002, 0x200000, CRC(c2bb0c24) SHA1(38724c49d9db49765a4ed9bc2dc8f57cec45ec7c) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "rm2.18",   0x800004, 0x200000, CRC(12257251) SHA1(20cb58afda0e6200991277817485340a6a41ae2b) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "rm2.20",   0x800006, 0x200000, CRC(f9b6e786) SHA1(aeb4acff7208e66a35198143fd2478039fdaa3a6) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION(QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "rm2.01a",  0x00000, 0x08000, CRC(d18e7859) SHA1(0939fac70042d0b4db5c2fdcef1f79b95febd45e) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "rm2.02",   0x28000, 0x20000, CRC(c463ece0) SHA1(5c3e41eb61610b3f8c431206f6672907e3a0bdb0) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "rm2.11",   0x000000, 0x200000, CRC(2106174d) SHA1(0a35d9ca8ebcad74904b20648d5320f839d6377e) )
	ROM_LOAD16_WORD_SWAP( "rm2.12",   0x200000, 0x200000, CRC(546c1636) SHA1(f96b172ab899f2c6ee17a5dd1fb61af9432e3cd2) )
ROM_END

ROM_START( rckman2j )
	ROM_REGION(CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "rm2j.03", 0x000000, 0x80000, CRC(dbaa1437) SHA1(849572090bdbde7d9f191959f4b6ad26f46811f4) )
	ROM_LOAD16_WORD_SWAP( "rm2j.04", 0x080000, 0x80000, CRC(cf5ba612) SHA1(f0b56db8df7ad676e00325c97cf16791f409e35a) )
	ROM_LOAD16_WORD_SWAP( "rm2.05",  0x100000, 0x80000, CRC(02ee9efc) SHA1(1b80c40389b51a03b930051f232630616c12e6c5) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "rm2jx.03", 0x000000, 0x80000, CRC(2c297750) SHA1(bf93b275113aac4fa290aea920772060324db9b3) )
	ROM_LOAD16_WORD_SWAP( "rm2jx.04", 0x080000, 0x80000, CRC(676a116e) SHA1(c8b6ab70efb22fdd9b7b4b736084a0d3eb123e3c) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROM_FILL(              0x000000, 0x800000, 0 )
	ROMX_LOAD( "rm2.14",   0x800000, 0x200000, CRC(9b1f00b4) SHA1(c1c5c2d9d00121425ae6598444d704f420ef4eef) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "rm2.16",   0x800002, 0x200000, CRC(c2bb0c24) SHA1(38724c49d9db49765a4ed9bc2dc8f57cec45ec7c) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "rm2.18",   0x800004, 0x200000, CRC(12257251) SHA1(20cb58afda0e6200991277817485340a6a41ae2b) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "rm2.20",   0x800006, 0x200000, CRC(f9b6e786) SHA1(aeb4acff7208e66a35198143fd2478039fdaa3a6) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION(QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "rm2.01a",  0x00000, 0x08000, CRC(d18e7859) SHA1(0939fac70042d0b4db5c2fdcef1f79b95febd45e) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "rm2.02",   0x28000, 0x20000, CRC(c463ece0) SHA1(5c3e41eb61610b3f8c431206f6672907e3a0bdb0) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "rm2.11",   0x000000, 0x200000, CRC(2106174d) SHA1(0a35d9ca8ebcad74904b20648d5320f839d6377e) )
	ROM_LOAD16_WORD_SWAP( "rm2.12",   0x200000, 0x200000, CRC(546c1636) SHA1(f96b172ab899f2c6ee17a5dd1fb61af9432e3cd2) )
ROM_END

ROM_START( mmatrix )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "mmxu.03", 0x000000, 0x80000, CRC(ab65b599) SHA1(d4c35f5a6cf0b37a35c466f0e347a660b2e0b21b) )
	ROM_LOAD16_WORD_SWAP( "mmxu.04", 0x080000, 0x80000, CRC(0135fc6c) SHA1(e40c8fa51dcb300b3ee72dc7de137e0b39dea490) )
	ROM_LOAD16_WORD_SWAP( "mmxu.05", 0x100000, 0x80000, CRC(f1fd2b84) SHA1(d34816eff4af98009f94f5dd14097b39426e0468) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "mmxux.03", 0x000000, 0x80000, CRC(7868ae77) SHA1(f75db041cfc8641ccdf55d9e5d13a06a39f87f70) )
	ROM_LOAD16_WORD_SWAP( "mmxux.04", 0x080000, 0x80000, CRC(a5ee6d07) SHA1(7cdefc0b28257673b722698eb29629cf0ccf4548) )
	ROM_LOAD16_WORD_SWAP( "mmxux.05", 0x100000, 0x80000, CRC(b07745ff) SHA1(578e601ab620439b04716742568c1deae62b1a8c) )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "mmx.13",   0x0000000, 0x400000, CRC(04748718) SHA1(d2e84d9dcc779c08469d815ccd709f30705954b8) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mmx.15",   0x0000002, 0x400000, CRC(38074f44) SHA1(2002c4862c156b314bc4f3372b713c48e0667ec3) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mmx.17",   0x0000004, 0x400000, CRC(e4635e35) SHA1(48ef7a82df83b981ddd6138c241ca129ab770e8e) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mmx.19",   0x0000006, 0x400000, CRC(4400a3f2) SHA1(d0aa805ccbb153896e5983da1c398d1df4f40371) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mmx.14",   0x1000000, 0x400000, CRC(d52bf491) SHA1(2398895cfdcf86fc485472e33df2cc446539e977) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mmx.16",   0x1000002, 0x400000, CRC(23f70780) SHA1(691ee8964815b0ce54704e7feb59ca79b634f26d) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mmx.18",   0x1000004, 0x400000, CRC(2562c9d5) SHA1(e7defc3d33db632c4035ae069f2f2332c58afaf5) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mmx.20",   0x1000006, 0x400000, CRC(583a9687) SHA1(1d0b08b1e88509245db3c2090f0201938fd750b4) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "mmx.01",   0x00000, 0x08000, CRC(c57e8171) SHA1(dedb92af1910d38727f816e6f507d25148f31b74) )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "mmx.11",   0x000000, 0x400000, CRC(4180b39f) SHA1(cabb1c358eae1bb6cfed07f5b92e4acd38650667) )
	ROM_LOAD16_WORD_SWAP( "mmx.12",   0x400000, 0x400000, CRC(95e22a59) SHA1(b3431d170c0a1a0d826ad0af21300b9180e3f114) )
ROM_END

ROM_START( mmatrixj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "mmxj.03", 0x000000, 0x80000, CRC(1d5de213) SHA1(2d7ad9cb50540a14aa0ac564d4ab84a3779d595c) )
	ROM_LOAD16_WORD_SWAP( "mmxj.04", 0x080000, 0x80000, CRC(d943a339) SHA1(ae3d217b35f92fc727bda3b14f13f3658dab3dd8) )
	ROM_LOAD16_WORD_SWAP( "mmxj.05", 0x100000, 0x80000, CRC(0c8b4abb) SHA1(c136186b9f57d68c0b36f5a4273347f696a227c0) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "mmxjx.03", 0x000000, 0x80000, CRC(4ca1424f) SHA1(579fac7c07b82bd7d005f67d7a1b7c143f1fb60b) )
	ROM_LOAD16_WORD_SWAP( "mmxjx.04", 0x080000, 0x80000, CRC(61b9b2a1) SHA1(874ffbc87fb631fca6e5bfa9042758e0a5c5ce1d) )
	ROM_LOAD16_WORD_SWAP( "mmxjx.05", 0x100000, 0x80000, CRC(bdd304cf) SHA1(4f0e828786a1b65aea2156f8cd8fa53c4a94b31c) )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "mmx.13",   0x0000000, 0x400000, CRC(04748718) SHA1(d2e84d9dcc779c08469d815ccd709f30705954b8) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mmx.15",   0x0000002, 0x400000, CRC(38074f44) SHA1(2002c4862c156b314bc4f3372b713c48e0667ec3) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mmx.17",   0x0000004, 0x400000, CRC(e4635e35) SHA1(48ef7a82df83b981ddd6138c241ca129ab770e8e) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mmx.19",   0x0000006, 0x400000, CRC(4400a3f2) SHA1(d0aa805ccbb153896e5983da1c398d1df4f40371) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mmx.14",   0x1000000, 0x400000, CRC(d52bf491) SHA1(2398895cfdcf86fc485472e33df2cc446539e977) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mmx.16",   0x1000002, 0x400000, CRC(23f70780) SHA1(691ee8964815b0ce54704e7feb59ca79b634f26d) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mmx.18",   0x1000004, 0x400000, CRC(2562c9d5) SHA1(e7defc3d33db632c4035ae069f2f2332c58afaf5) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mmx.20",   0x1000006, 0x400000, CRC(583a9687) SHA1(1d0b08b1e88509245db3c2090f0201938fd750b4) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "mmx.01",   0x00000, 0x08000, CRC(c57e8171) SHA1(dedb92af1910d38727f816e6f507d25148f31b74) )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "mmx.11",   0x000000, 0x400000, CRC(4180b39f) SHA1(cabb1c358eae1bb6cfed07f5b92e4acd38650667) )
	ROM_LOAD16_WORD_SWAP( "mmx.12",   0x400000, 0x400000, CRC(95e22a59) SHA1(b3431d170c0a1a0d826ad0af21300b9180e3f114) )
ROM_END

ROM_START( msh )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "mshe.03e", 0x000000, 0x80000, CRC(bd951414) SHA1(5585bdd1484dc18c7630d689f60d91c068aafc97) )
	ROM_LOAD16_WORD_SWAP( "mshe.04e", 0x080000, 0x80000, CRC(19dd42f2) SHA1(48bd3e4d2d7e9e07275bd9c00530719deb100090) )
	ROM_LOAD16_WORD_SWAP( "msh.05",   0x100000, 0x80000, CRC(6a091b9e) SHA1(7fa54e69e1a1ca348cb08d892d55023e9a3ff4cb) )
	ROM_LOAD16_WORD_SWAP( "msh.06b",  0x180000, 0x80000, CRC(803e3fa4) SHA1(0acdeda65002521bf24130cbf06f9faa1dcef9e5) )
	ROM_LOAD16_WORD_SWAP( "msh.07a",  0x200000, 0x80000, CRC(c45f8e27) SHA1(4d28e0782c31ce56e728ac6ef5edd10437f00637) )
	ROM_LOAD16_WORD_SWAP( "msh.08a",  0x280000, 0x80000, CRC(9ca6f12c) SHA1(26ad682667b983b805e1f577426e5fca8ee3c82b) )
	ROM_LOAD16_WORD_SWAP( "msh.09a",  0x300000, 0x80000, CRC(82ec27af) SHA1(caf76268063ba91d28e8af684d60c2d71f29b9b9) )
	ROM_LOAD16_WORD_SWAP( "msh.10b",  0x380000, 0x80000, CRC(8d931196) SHA1(983e62efcdb4c8db6bce6acf4f86acb9447b565d) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
/*	ROM_LOAD16_WORD_SWAP( "mshex.03e", 0x000000, 0x80000, NO_DUMP )*/
/*	ROM_LOAD16_WORD_SWAP( "mshex.04e", 0x080000, 0x80000, NO_DUMP )*/

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "msh.13",   0x0000000, 0x400000, CRC(09d14566) SHA1(c96463654043f22da5e844c6da17aa9273dc3439) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.15",   0x0000002, 0x400000, CRC(ee962057) SHA1(24e359accb5f71a5863d7bad4088719fa547f88c) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.17",   0x0000004, 0x400000, CRC(604ece14) SHA1(880fb62b33ba4cceb38635e4ec056fac11a3c70f) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.19",   0x0000006, 0x400000, CRC(94a731e8) SHA1(1e784a3412e7361e3001494e1daf840ef8c20449) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.14",   0x1000000, 0x400000, CRC(4197973e) SHA1(93aeea1a480b5f452c8a40ae3fff956796b859fa) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.16",   0x1000002, 0x400000, CRC(438da4a0) SHA1(ca93b14c3a570f9dd582efbb3f0536a92e535042) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.18",   0x1000004, 0x400000, CRC(4db92d94) SHA1(f1b25ccc0627139ad5b287a8f2ab3b4a2fb8b8e4) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.20",   0x1000006, 0x400000, CRC(a2b0c6c0) SHA1(71016c01c1a706b73cf5b9ac7e384a030c6cf08d) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "msh.01",   0x00000, 0x08000, CRC(c976e6f9) SHA1(281025e5aaf97c0aeddc8bd0f737d092daadad9e) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "msh.02",   0x28000, 0x20000, CRC(ce67d0d9) SHA1(324226597cc5a11603f04085fef7715a314ecc05) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "msh.11",   0x000000, 0x200000, CRC(37ac6d30) SHA1(ec67421fbf4a08a686e76792cb35e9cbf04d022d) )
	ROM_LOAD16_WORD_SWAP( "msh.12",   0x200000, 0x200000, CRC(de092570) SHA1(a03d0df901f6ea79685eaed67db65bee14ec29c6) )
ROM_END

ROM_START( mshu )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "mshu.03", 0x000000, 0x80000, CRC(d2805bdd) SHA1(a6f78c31a82168bb5f7d614dcebbeab8231e2d75) )
	ROM_LOAD16_WORD_SWAP( "mshu.04", 0x080000, 0x80000, CRC(743f96ff) SHA1(abb82359bb68966028ea33e94996803599f3e273) )
	ROM_LOAD16_WORD_SWAP( "msh.05",  0x100000, 0x80000, CRC(6a091b9e) SHA1(7fa54e69e1a1ca348cb08d892d55023e9a3ff4cb) )
	ROM_LOAD16_WORD_SWAP( "msh.06b", 0x180000, 0x80000, CRC(803e3fa4) SHA1(0acdeda65002521bf24130cbf06f9faa1dcef9e5) )
	ROM_LOAD16_WORD_SWAP( "msh.07a", 0x200000, 0x80000, CRC(c45f8e27) SHA1(4d28e0782c31ce56e728ac6ef5edd10437f00637) )
	ROM_LOAD16_WORD_SWAP( "msh.08a", 0x280000, 0x80000, CRC(9ca6f12c) SHA1(26ad682667b983b805e1f577426e5fca8ee3c82b) )
	ROM_LOAD16_WORD_SWAP( "msh.09a", 0x300000, 0x80000, CRC(82ec27af) SHA1(caf76268063ba91d28e8af684d60c2d71f29b9b9) )
	ROM_LOAD16_WORD_SWAP( "msh.10b", 0x380000, 0x80000, CRC(8d931196) SHA1(983e62efcdb4c8db6bce6acf4f86acb9447b565d) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "mshux.03", 0x000000, 0x80000, CRC(10bfc357) SHA1(6d833ce68249cef57cc49682ebd5f32d9b495126) )
	ROM_LOAD16_WORD_SWAP( "mshux.04", 0x080000, 0x80000, CRC(871f0863) SHA1(5a080bdfcfb58740c0890976d338621e2a326bf2) )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "msh.13",   0x0000000, 0x400000, CRC(09d14566) SHA1(c96463654043f22da5e844c6da17aa9273dc3439) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.15",   0x0000002, 0x400000, CRC(ee962057) SHA1(24e359accb5f71a5863d7bad4088719fa547f88c) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.17",   0x0000004, 0x400000, CRC(604ece14) SHA1(880fb62b33ba4cceb38635e4ec056fac11a3c70f) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.19",   0x0000006, 0x400000, CRC(94a731e8) SHA1(1e784a3412e7361e3001494e1daf840ef8c20449) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.14",   0x1000000, 0x400000, CRC(4197973e) SHA1(93aeea1a480b5f452c8a40ae3fff956796b859fa) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.16",   0x1000002, 0x400000, CRC(438da4a0) SHA1(ca93b14c3a570f9dd582efbb3f0536a92e535042) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.18",   0x1000004, 0x400000, CRC(4db92d94) SHA1(f1b25ccc0627139ad5b287a8f2ab3b4a2fb8b8e4) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.20",   0x1000006, 0x400000, CRC(a2b0c6c0) SHA1(71016c01c1a706b73cf5b9ac7e384a030c6cf08d) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "msh.01",   0x00000, 0x08000, CRC(c976e6f9) SHA1(281025e5aaf97c0aeddc8bd0f737d092daadad9e) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "msh.02",   0x28000, 0x20000, CRC(ce67d0d9) SHA1(324226597cc5a11603f04085fef7715a314ecc05) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "msh.11",   0x000000, 0x200000, CRC(37ac6d30) SHA1(ec67421fbf4a08a686e76792cb35e9cbf04d022d) )
	ROM_LOAD16_WORD_SWAP( "msh.12",   0x200000, 0x200000, CRC(de092570) SHA1(a03d0df901f6ea79685eaed67db65bee14ec29c6) )
ROM_END

ROM_START( mshj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "mshj.03f", 0x000000, 0x80000, CRC(ff172fd2) SHA1(2dd507e3fcf1a30fde1e6ce63d4233a67e7bfc9e) )
	ROM_LOAD16_WORD_SWAP( "mshj.04f", 0x080000, 0x80000, CRC(ebbb205a) SHA1(0b110ea4c71bdab819b72e6f9736368575e4cccf) )
	ROM_LOAD16_WORD_SWAP( "msh.05",   0x100000, 0x80000, CRC(6a091b9e) SHA1(7fa54e69e1a1ca348cb08d892d55023e9a3ff4cb) )
	ROM_LOAD16_WORD_SWAP( "msh.06b",  0x180000, 0x80000, CRC(803e3fa4) SHA1(0acdeda65002521bf24130cbf06f9faa1dcef9e5) )
	ROM_LOAD16_WORD_SWAP( "msh.07a",  0x200000, 0x80000, CRC(c45f8e27) SHA1(4d28e0782c31ce56e728ac6ef5edd10437f00637) )
	ROM_LOAD16_WORD_SWAP( "msh.08a",  0x280000, 0x80000, CRC(9ca6f12c) SHA1(26ad682667b983b805e1f577426e5fca8ee3c82b) )
	ROM_LOAD16_WORD_SWAP( "msh.09a",  0x300000, 0x80000, CRC(82ec27af) SHA1(caf76268063ba91d28e8af684d60c2d71f29b9b9) )
	ROM_LOAD16_WORD_SWAP( "msh.10b",  0x380000, 0x80000, CRC(8d931196) SHA1(983e62efcdb4c8db6bce6acf4f86acb9447b565d) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "mshjx.03f", 0x000000, 0x80000, CRC(a15f32f2) SHA1(a5368870c3a040066760f8e1f938d74db023edc3) )
	ROM_LOAD16_WORD_SWAP( "mshjx.04f", 0x080000, 0x80000, CRC(c81be228) SHA1(45023d22c0bdd29868396bc32b6244e4385ab33e) )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "msh.13",   0x0000000, 0x400000, CRC(09d14566) SHA1(c96463654043f22da5e844c6da17aa9273dc3439) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.15",   0x0000002, 0x400000, CRC(ee962057) SHA1(24e359accb5f71a5863d7bad4088719fa547f88c) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.17",   0x0000004, 0x400000, CRC(604ece14) SHA1(880fb62b33ba4cceb38635e4ec056fac11a3c70f) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.19",   0x0000006, 0x400000, CRC(94a731e8) SHA1(1e784a3412e7361e3001494e1daf840ef8c20449) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.14",   0x1000000, 0x400000, CRC(4197973e) SHA1(93aeea1a480b5f452c8a40ae3fff956796b859fa) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.16",   0x1000002, 0x400000, CRC(438da4a0) SHA1(ca93b14c3a570f9dd582efbb3f0536a92e535042) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.18",   0x1000004, 0x400000, CRC(4db92d94) SHA1(f1b25ccc0627139ad5b287a8f2ab3b4a2fb8b8e4) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.20",   0x1000006, 0x400000, CRC(a2b0c6c0) SHA1(71016c01c1a706b73cf5b9ac7e384a030c6cf08d) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "msh.01",   0x00000, 0x08000, CRC(c976e6f9) SHA1(281025e5aaf97c0aeddc8bd0f737d092daadad9e) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "msh.02",   0x28000, 0x20000, CRC(ce67d0d9) SHA1(324226597cc5a11603f04085fef7715a314ecc05) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "msh.11",   0x000000, 0x200000, CRC(37ac6d30) SHA1(ec67421fbf4a08a686e76792cb35e9cbf04d022d) )
	ROM_LOAD16_WORD_SWAP( "msh.12",   0x200000, 0x200000, CRC(de092570) SHA1(a03d0df901f6ea79685eaed67db65bee14ec29c6) )
ROM_END

ROM_START( msha )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "msha.03e", 0x000000, 0x80000, CRC(ec84ec44) SHA1(4d434df6cf5c961f0dbba352d1353db0a8f353dc) )
	ROM_LOAD16_WORD_SWAP( "msha.04e", 0x080000, 0x80000, CRC(098b8503) SHA1(4cc74754796d5e41f13bf5cd4e8868b0d0c7852c) )
	ROM_LOAD16_WORD_SWAP( "msh.05",   0x100000, 0x80000, CRC(6a091b9e) SHA1(7fa54e69e1a1ca348cb08d892d55023e9a3ff4cb) )
	ROM_LOAD16_WORD_SWAP( "msh.06b",  0x180000, 0x80000, CRC(803e3fa4) SHA1(0acdeda65002521bf24130cbf06f9faa1dcef9e5) )
	ROM_LOAD16_WORD_SWAP( "msh.07a",  0x200000, 0x80000, CRC(c45f8e27) SHA1(4d28e0782c31ce56e728ac6ef5edd10437f00637) )
	ROM_LOAD16_WORD_SWAP( "msh.08a",  0x280000, 0x80000, CRC(9ca6f12c) SHA1(26ad682667b983b805e1f577426e5fca8ee3c82b) )
	ROM_LOAD16_WORD_SWAP( "msh.09a",  0x300000, 0x80000, CRC(82ec27af) SHA1(caf76268063ba91d28e8af684d60c2d71f29b9b9) )
	ROM_LOAD16_WORD_SWAP( "msh.10b",  0x380000, 0x80000, CRC(8d931196) SHA1(983e62efcdb4c8db6bce6acf4f86acb9447b565d) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "mshax.03e", 0x000000, 0x80000, CRC(7423947b) SHA1(0c70032c5b8f73fde014f4eeeac02214310c6c04) )
	ROM_LOAD16_WORD_SWAP( "mshax.04e", 0x080000, 0x80000, CRC(bd4a3071) SHA1(b69ae310747f5daef2cda56a326598f8597ed6e0) )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "msh.13",   0x0000000, 0x400000, CRC(09d14566) SHA1(c96463654043f22da5e844c6da17aa9273dc3439) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.15",   0x0000002, 0x400000, CRC(ee962057) SHA1(24e359accb5f71a5863d7bad4088719fa547f88c) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.17",   0x0000004, 0x400000, CRC(604ece14) SHA1(880fb62b33ba4cceb38635e4ec056fac11a3c70f) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.19",   0x0000006, 0x400000, CRC(94a731e8) SHA1(1e784a3412e7361e3001494e1daf840ef8c20449) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.14",   0x1000000, 0x400000, CRC(4197973e) SHA1(93aeea1a480b5f452c8a40ae3fff956796b859fa) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.16",   0x1000002, 0x400000, CRC(438da4a0) SHA1(ca93b14c3a570f9dd582efbb3f0536a92e535042) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.18",   0x1000004, 0x400000, CRC(4db92d94) SHA1(f1b25ccc0627139ad5b287a8f2ab3b4a2fb8b8e4) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.20",   0x1000006, 0x400000, CRC(a2b0c6c0) SHA1(71016c01c1a706b73cf5b9ac7e384a030c6cf08d) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "msh.01",   0x00000, 0x08000, CRC(c976e6f9) SHA1(281025e5aaf97c0aeddc8bd0f737d092daadad9e) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "msh.02",   0x28000, 0x20000, CRC(ce67d0d9) SHA1(324226597cc5a11603f04085fef7715a314ecc05) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "msh.11",   0x000000, 0x200000, CRC(37ac6d30) SHA1(ec67421fbf4a08a686e76792cb35e9cbf04d022d) )
	ROM_LOAD16_WORD_SWAP( "msh.12",   0x200000, 0x200000, CRC(de092570) SHA1(a03d0df901f6ea79685eaed67db65bee14ec29c6) )
ROM_END

ROM_START( mshh )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "mshh.03c", 0x000000, 0x80000, CRC(8d84b0fa) SHA1(e1fd2869abbe4f8736e496f194e23a1ab0526811) )
	ROM_LOAD16_WORD_SWAP( "mshh.04c", 0x080000, 0x80000, CRC(d638f601) SHA1(cbdd9776f71c6ef8d80be23a57cba3529d53a070) )
	ROM_LOAD16_WORD_SWAP( "msh.05a",  0x100000, 0x80000, CRC(f37539e6) SHA1(770febc25ca5615b6c2023727edab3c68b15b2c4) )
	ROM_LOAD16_WORD_SWAP( "msh.06b",  0x180000, 0x80000, CRC(803e3fa4) SHA1(0acdeda65002521bf24130cbf06f9faa1dcef9e5) )
	ROM_LOAD16_WORD_SWAP( "msh.07a",  0x200000, 0x80000, CRC(c45f8e27) SHA1(4d28e0782c31ce56e728ac6ef5edd10437f00637) )
	ROM_LOAD16_WORD_SWAP( "msh.08a",  0x280000, 0x80000, CRC(9ca6f12c) SHA1(26ad682667b983b805e1f577426e5fca8ee3c82b) )
	ROM_LOAD16_WORD_SWAP( "msh.09a",  0x300000, 0x80000, CRC(82ec27af) SHA1(caf76268063ba91d28e8af684d60c2d71f29b9b9) )
	ROM_LOAD16_WORD_SWAP( "msh.10b",  0x380000, 0x80000, CRC(8d931196) SHA1(983e62efcdb4c8db6bce6acf4f86acb9447b565d) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "mshhx.03c", 0x000000, 0x80000, CRC(6daf52bb) SHA1(27b926f0c527897f8a5ba0421aae825d9596d2ea) )
	ROM_LOAD16_WORD_SWAP( "mshhx.04c", 0x080000, 0x80000, CRC(5684655a) SHA1(d60e0b3927ed394bb2bcf6cf8cd24e74792bf167) )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "msh.13",   0x0000000, 0x400000, CRC(09d14566) SHA1(c96463654043f22da5e844c6da17aa9273dc3439) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.15",   0x0000002, 0x400000, CRC(ee962057) SHA1(24e359accb5f71a5863d7bad4088719fa547f88c) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.17",   0x0000004, 0x400000, CRC(604ece14) SHA1(880fb62b33ba4cceb38635e4ec056fac11a3c70f) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.19",   0x0000006, 0x400000, CRC(94a731e8) SHA1(1e784a3412e7361e3001494e1daf840ef8c20449) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.14",   0x1000000, 0x400000, CRC(4197973e) SHA1(93aeea1a480b5f452c8a40ae3fff956796b859fa) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.16",   0x1000002, 0x400000, CRC(438da4a0) SHA1(ca93b14c3a570f9dd582efbb3f0536a92e535042) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.18",   0x1000004, 0x400000, CRC(4db92d94) SHA1(f1b25ccc0627139ad5b287a8f2ab3b4a2fb8b8e4) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.20",   0x1000006, 0x400000, CRC(a2b0c6c0) SHA1(71016c01c1a706b73cf5b9ac7e384a030c6cf08d) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "msh.01",   0x00000, 0x08000, CRC(c976e6f9) SHA1(281025e5aaf97c0aeddc8bd0f737d092daadad9e) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "msh.02",   0x28000, 0x20000, CRC(ce67d0d9) SHA1(324226597cc5a11603f04085fef7715a314ecc05) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "msh.11",   0x000000, 0x200000, CRC(37ac6d30) SHA1(ec67421fbf4a08a686e76792cb35e9cbf04d022d) )
	ROM_LOAD16_WORD_SWAP( "msh.12",   0x200000, 0x200000, CRC(de092570) SHA1(a03d0df901f6ea79685eaed67db65bee14ec29c6) )
ROM_END

ROM_START( mshb )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "mshb.03c", 0x000000, 0x80000, CRC(19697f74) SHA1(c3809ecbdb242bdbb57f8d9b029264e9c0ed8a13) )
	ROM_LOAD16_WORD_SWAP( "mshb.04c", 0x080000, 0x80000, CRC(95317a6f) SHA1(143a26e349f21d3a720320bb7010a26f767e5e73) )
	ROM_LOAD16_WORD_SWAP( "msh.05a",  0x100000, 0x80000, CRC(f37539e6) SHA1(770febc25ca5615b6c2023727edab3c68b15b2c4) )
	ROM_LOAD16_WORD_SWAP( "msh.06b",  0x180000, 0x80000, CRC(803e3fa4) SHA1(0acdeda65002521bf24130cbf06f9faa1dcef9e5) )
	ROM_LOAD16_WORD_SWAP( "msh.07a",  0x200000, 0x80000, CRC(c45f8e27) SHA1(4d28e0782c31ce56e728ac6ef5edd10437f00637) )
	ROM_LOAD16_WORD_SWAP( "msh.08a",  0x280000, 0x80000, CRC(9ca6f12c) SHA1(26ad682667b983b805e1f577426e5fca8ee3c82b) )
	ROM_LOAD16_WORD_SWAP( "msh.09a",  0x300000, 0x80000, CRC(82ec27af) SHA1(caf76268063ba91d28e8af684d60c2d71f29b9b9) )
	ROM_LOAD16_WORD_SWAP( "msh.10b",  0x380000, 0x80000, CRC(8d931196) SHA1(983e62efcdb4c8db6bce6acf4f86acb9447b565d) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
/*	ROM_LOAD16_WORD_SWAP( "mshbx.03c", 0x000000, 0x80000, NO_DUMP )*/
/*	ROM_LOAD16_WORD_SWAP( "mshbx.04c", 0x080000, 0x80000, NO_DUMP )*/

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "msh.13",   0x0000000, 0x400000, CRC(09d14566) SHA1(c96463654043f22da5e844c6da17aa9273dc3439) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.15",   0x0000002, 0x400000, CRC(ee962057) SHA1(24e359accb5f71a5863d7bad4088719fa547f88c) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.17",   0x0000004, 0x400000, CRC(604ece14) SHA1(880fb62b33ba4cceb38635e4ec056fac11a3c70f) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.19",   0x0000006, 0x400000, CRC(94a731e8) SHA1(1e784a3412e7361e3001494e1daf840ef8c20449) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.14",   0x1000000, 0x400000, CRC(4197973e) SHA1(93aeea1a480b5f452c8a40ae3fff956796b859fa) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.16",   0x1000002, 0x400000, CRC(438da4a0) SHA1(ca93b14c3a570f9dd582efbb3f0536a92e535042) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.18",   0x1000004, 0x400000, CRC(4db92d94) SHA1(f1b25ccc0627139ad5b287a8f2ab3b4a2fb8b8e4) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.20",   0x1000006, 0x400000, CRC(a2b0c6c0) SHA1(71016c01c1a706b73cf5b9ac7e384a030c6cf08d) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "msh.01",   0x00000, 0x08000, CRC(c976e6f9) SHA1(281025e5aaf97c0aeddc8bd0f737d092daadad9e) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "msh.02",   0x28000, 0x20000, CRC(ce67d0d9) SHA1(324226597cc5a11603f04085fef7715a314ecc05) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "msh.11",   0x000000, 0x200000, CRC(37ac6d30) SHA1(ec67421fbf4a08a686e76792cb35e9cbf04d022d) )
	ROM_LOAD16_WORD_SWAP( "msh.12",   0x200000, 0x200000, CRC(de092570) SHA1(a03d0df901f6ea79685eaed67db65bee14ec29c6) )
ROM_END

ROM_START( mshvsf )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "mvsu.03g", 0x000000, 0x80000, CRC(0664ab15) SHA1(939fb1e3c06c33fc212b26ecfceac3180e108e9d) )
	ROM_LOAD16_WORD_SWAP( "mvsu.04g", 0x080000, 0x80000, CRC(97e060ee) SHA1(787924e04508c83ecd4c3a872882d2be9e57eb50) )
	ROM_LOAD16_WORD_SWAP( "mvs.05d",  0x100000, 0x80000, CRC(921fc542) SHA1(b813082a480d42d663c713062892245faabe9101) )
	ROM_LOAD16_WORD_SWAP( "mvs.06a",  0x180000, 0x80000, CRC(959f3030) SHA1(fbbaa915324815246738f3426232e623f039ce26) )
	ROM_LOAD16_WORD_SWAP( "mvs.07b",  0x200000, 0x80000, CRC(7f915bdb) SHA1(683da09c5ba55e31b59aa95a8e13c45dc574ab3c) )
	ROM_LOAD16_WORD_SWAP( "mvs.08a",  0x280000, 0x80000, CRC(c2813884) SHA1(49e5d4bc48f90c8146cb6aafb9240aff0119f1a7) )
	ROM_LOAD16_WORD_SWAP( "mvs.09b",  0x300000, 0x80000, CRC(3ba08818) SHA1(9ab132a3cac55fcccebe6c99b6fb0ba1305f8f6e) )
	ROM_LOAD16_WORD_SWAP( "mvs.10b",  0x380000, 0x80000, CRC(cf0dba98) SHA1(f4c1f8a6e7a79ecc6241d5268b3039f8a09ea516) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "mvsux.03g", 0x000000, 0x80000, CRC(17b12a95) SHA1(7bc665e96a975f2963e551114cb5264357730bb5) )
	ROM_LOAD16_WORD_SWAP( "mvsux.04g", 0x080000, 0x80000, CRC(f98200cf) SHA1(e8c1844e021e147b39b29b65d6e4c9cb1d36112f) )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "mvs.13",   0x0000000, 0x400000, CRC(29b05fd9) SHA1(e8fdb1ee5515a560eb4256ae4fd99bb1192e1a87) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.15",   0x0000002, 0x400000, CRC(faddccf1) SHA1(4ed03ea91883a0413325f57edcc1614120b5922c) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.17",   0x0000004, 0x400000, CRC(97aaf4c7) SHA1(6a054921cc14fe080cb3f62c391f8ae3cc7e8ba9) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.19",   0x0000006, 0x400000, CRC(cb70e915) SHA1(da4d2480d348ac6dfd01256a88f4f3db8357ae46) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.14",   0x1000000, 0x400000, CRC(b3b1972d) SHA1(0f2c3fb7de014181ee481ec35d0578b2c116c2dc) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.16",   0x1000002, 0x400000, CRC(08aadb5d) SHA1(3a2c222eca3e7df80ce69951b3db6442312751a4) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.18",   0x1000004, 0x400000, CRC(c1228b35) SHA1(7afdfb552888c79d0fbb30242b3d917b87fad57a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.20",   0x1000006, 0x400000, CRC(366cc6c2) SHA1(6f2a789087c8e404c5227b927fa8328c03593243) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "mvs.01",   0x00000, 0x08000, CRC(68252324) SHA1(138ef320ef27956b2ab5591d49a1315b7b0a194c) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "mvs.02",   0x28000, 0x20000, CRC(b34e773d) SHA1(3bcf44bf06c35814cff29d244142db7abe05bd39) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "mvs.11",   0x000000, 0x400000, CRC(86219770) SHA1(4e5b68d382a5aa37f8b0b6434c53a2b95f5f9a4d) )
	ROM_LOAD16_WORD_SWAP( "mvs.12",   0x400000, 0x400000, CRC(f2fd7f68) SHA1(28a30d55d3eaf963006c7cbe7c288099cd3ba536) )
ROM_END

ROM_START( mshvsfu1 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "mvsu.03d", 0x000000, 0x80000, CRC(ae60a66a) SHA1(1fa7e6534d02ec8059153705b1161a55b9cfe803) )
	ROM_LOAD16_WORD_SWAP( "mvsu.04d", 0x080000, 0x80000, CRC(91f67d8a) SHA1(e95f7a3fb281e1bafdbe7a1b22532c4fab5ec89d) )
	ROM_LOAD16_WORD_SWAP( "mvs.05a",  0x100000, 0x80000, CRC(1a5de0cb) SHA1(738a27f83704c208d36d73bf766d861ef2d51a89) )
	ROM_LOAD16_WORD_SWAP( "mvs.06a",  0x180000, 0x80000, CRC(959f3030) SHA1(fbbaa915324815246738f3426232e623f039ce26) )
	ROM_LOAD16_WORD_SWAP( "mvs.07b",  0x200000, 0x80000, CRC(7f915bdb) SHA1(683da09c5ba55e31b59aa95a8e13c45dc574ab3c) )
	ROM_LOAD16_WORD_SWAP( "mvs.08a",  0x280000, 0x80000, CRC(c2813884) SHA1(49e5d4bc48f90c8146cb6aafb9240aff0119f1a7) )
	ROM_LOAD16_WORD_SWAP( "mvs.09b",  0x300000, 0x80000, CRC(3ba08818) SHA1(9ab132a3cac55fcccebe6c99b6fb0ba1305f8f6e) )
	ROM_LOAD16_WORD_SWAP( "mvs.10b",  0x380000, 0x80000, CRC(cf0dba98) SHA1(f4c1f8a6e7a79ecc6241d5268b3039f8a09ea516) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "mvsux.03d", 0x000000, 0x80000, CRC(281bcb48) SHA1(060ab490ecd29a4d088309d45fa595a2d3e7afd2) )
	ROM_LOAD16_WORD_SWAP( "mvsux.04d", 0x080000, 0x80000, CRC(a2d68628) SHA1(f703a41833148138276fa6118a9317cb36a69ec6) )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "mvs.13",   0x0000000, 0x400000, CRC(29b05fd9) SHA1(e8fdb1ee5515a560eb4256ae4fd99bb1192e1a87) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.15",   0x0000002, 0x400000, CRC(faddccf1) SHA1(4ed03ea91883a0413325f57edcc1614120b5922c) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.17",   0x0000004, 0x400000, CRC(97aaf4c7) SHA1(6a054921cc14fe080cb3f62c391f8ae3cc7e8ba9) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.19",   0x0000006, 0x400000, CRC(cb70e915) SHA1(da4d2480d348ac6dfd01256a88f4f3db8357ae46) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.14",   0x1000000, 0x400000, CRC(b3b1972d) SHA1(0f2c3fb7de014181ee481ec35d0578b2c116c2dc) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.16",   0x1000002, 0x400000, CRC(08aadb5d) SHA1(3a2c222eca3e7df80ce69951b3db6442312751a4) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.18",   0x1000004, 0x400000, CRC(c1228b35) SHA1(7afdfb552888c79d0fbb30242b3d917b87fad57a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.20",   0x1000006, 0x400000, CRC(366cc6c2) SHA1(6f2a789087c8e404c5227b927fa8328c03593243) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "mvs.01",   0x00000, 0x08000, CRC(68252324) SHA1(138ef320ef27956b2ab5591d49a1315b7b0a194c) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "mvs.02",   0x28000, 0x20000, CRC(b34e773d) SHA1(3bcf44bf06c35814cff29d244142db7abe05bd39) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "mvs.11",   0x000000, 0x400000, CRC(86219770) SHA1(4e5b68d382a5aa37f8b0b6434c53a2b95f5f9a4d) )
	ROM_LOAD16_WORD_SWAP( "mvs.12",   0x400000, 0x400000, CRC(f2fd7f68) SHA1(28a30d55d3eaf963006c7cbe7c288099cd3ba536) )
ROM_END

ROM_START( mshvsfj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "mvsj.03i", 0x000000, 0x80000, CRC(d8cbb691) SHA1(16820cf3bc7285477e61bd598a3ed4ea5e0e770d) )
	ROM_LOAD16_WORD_SWAP( "mvsj.04i", 0x080000, 0x80000, CRC(32741ace) SHA1(36db3a3aeaf29369977593c051bf5665cffefb2d) )
	ROM_LOAD16_WORD_SWAP( "mvs.05h",  0x100000, 0x80000, CRC(77870dc3) SHA1(924a7c82456bb44d7b0be65af11dbe1a2420a3f0) )
	ROM_LOAD16_WORD_SWAP( "mvs.06a",  0x180000, 0x80000, CRC(959f3030) SHA1(fbbaa915324815246738f3426232e623f039ce26) )
	ROM_LOAD16_WORD_SWAP( "mvs.07b",  0x200000, 0x80000, CRC(7f915bdb) SHA1(683da09c5ba55e31b59aa95a8e13c45dc574ab3c) )
	ROM_LOAD16_WORD_SWAP( "mvs.08a",  0x280000, 0x80000, CRC(c2813884) SHA1(49e5d4bc48f90c8146cb6aafb9240aff0119f1a7) )
	ROM_LOAD16_WORD_SWAP( "mvs.09b",  0x300000, 0x80000, CRC(3ba08818) SHA1(9ab132a3cac55fcccebe6c99b6fb0ba1305f8f6e) )
	ROM_LOAD16_WORD_SWAP( "mvs.10b",  0x380000, 0x80000, CRC(cf0dba98) SHA1(f4c1f8a6e7a79ecc6241d5268b3039f8a09ea516) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "mvsjx.03i", 0x000000, 0x80000, CRC(55170c4c) SHA1(021a8a136d3212f60c63ee2df3556a1598db866e) )
	ROM_LOAD16_WORD_SWAP( "mvsjx.04i", 0x080000, 0x80000, CRC(e7883768) SHA1(c17be39eccc816d09199e77478f9eb2f42f348d9) )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "mvs.13",   0x0000000, 0x400000, CRC(29b05fd9) SHA1(e8fdb1ee5515a560eb4256ae4fd99bb1192e1a87) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.15",   0x0000002, 0x400000, CRC(faddccf1) SHA1(4ed03ea91883a0413325f57edcc1614120b5922c) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.17",   0x0000004, 0x400000, CRC(97aaf4c7) SHA1(6a054921cc14fe080cb3f62c391f8ae3cc7e8ba9) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.19",   0x0000006, 0x400000, CRC(cb70e915) SHA1(da4d2480d348ac6dfd01256a88f4f3db8357ae46) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.14",   0x1000000, 0x400000, CRC(b3b1972d) SHA1(0f2c3fb7de014181ee481ec35d0578b2c116c2dc) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.16",   0x1000002, 0x400000, CRC(08aadb5d) SHA1(3a2c222eca3e7df80ce69951b3db6442312751a4) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.18",   0x1000004, 0x400000, CRC(c1228b35) SHA1(7afdfb552888c79d0fbb30242b3d917b87fad57a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.20",   0x1000006, 0x400000, CRC(366cc6c2) SHA1(6f2a789087c8e404c5227b927fa8328c03593243) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "mvs.01",   0x00000, 0x08000, CRC(68252324) SHA1(138ef320ef27956b2ab5591d49a1315b7b0a194c) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "mvs.02",   0x28000, 0x20000, CRC(b34e773d) SHA1(3bcf44bf06c35814cff29d244142db7abe05bd39) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "mvs.11",   0x000000, 0x400000, CRC(86219770) SHA1(4e5b68d382a5aa37f8b0b6434c53a2b95f5f9a4d) )
	ROM_LOAD16_WORD_SWAP( "mvs.12",   0x400000, 0x400000, CRC(f2fd7f68) SHA1(28a30d55d3eaf963006c7cbe7c288099cd3ba536) )
ROM_END

ROM_START( mshvsfj1 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "mvsj.03h", 0x000000, 0x80000, CRC(fbe2115f) SHA1(b2d8a62e394c2eb4070cac742b0f403252e46a25) )
	ROM_LOAD16_WORD_SWAP( "mvsj.04h", 0x080000, 0x80000, CRC(b528a367) SHA1(ecac71b032b431c63a4cf73a1d1d1be1faebc12b) )
	ROM_LOAD16_WORD_SWAP( "mvs.05g",  0x100000, 0x80000, CRC(9515a245) SHA1(eafa877fd4a4e58e7c98336658e986a4a27d6b91) )
	ROM_LOAD16_WORD_SWAP( "mvs.06a",  0x180000, 0x80000, CRC(959f3030) SHA1(fbbaa915324815246738f3426232e623f039ce26) )
	ROM_LOAD16_WORD_SWAP( "mvs.07b",  0x200000, 0x80000, CRC(7f915bdb) SHA1(683da09c5ba55e31b59aa95a8e13c45dc574ab3c) )
	ROM_LOAD16_WORD_SWAP( "mvs.08a",  0x280000, 0x80000, CRC(c2813884) SHA1(49e5d4bc48f90c8146cb6aafb9240aff0119f1a7) )
	ROM_LOAD16_WORD_SWAP( "mvs.09b",  0x300000, 0x80000, CRC(3ba08818) SHA1(9ab132a3cac55fcccebe6c99b6fb0ba1305f8f6e) )
	ROM_LOAD16_WORD_SWAP( "mvs.10b",  0x380000, 0x80000, CRC(cf0dba98) SHA1(f4c1f8a6e7a79ecc6241d5268b3039f8a09ea516) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "mvsjx.03h", 0x000000, 0x80000, CRC(6b4201c1) SHA1(bf99fef1989161d3e6bbaa92714cb250b33a215a) )
	ROM_LOAD16_WORD_SWAP( "mvsjx.04h", 0x080000, 0x80000, CRC(ab1b04cc) SHA1(29f40e2e151c3cdeafb5cbce038433e94818300e) )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "mvs.13",   0x0000000, 0x400000, CRC(29b05fd9) SHA1(e8fdb1ee5515a560eb4256ae4fd99bb1192e1a87) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.15",   0x0000002, 0x400000, CRC(faddccf1) SHA1(4ed03ea91883a0413325f57edcc1614120b5922c) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.17",   0x0000004, 0x400000, CRC(97aaf4c7) SHA1(6a054921cc14fe080cb3f62c391f8ae3cc7e8ba9) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.19",   0x0000006, 0x400000, CRC(cb70e915) SHA1(da4d2480d348ac6dfd01256a88f4f3db8357ae46) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.14",   0x1000000, 0x400000, CRC(b3b1972d) SHA1(0f2c3fb7de014181ee481ec35d0578b2c116c2dc) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.16",   0x1000002, 0x400000, CRC(08aadb5d) SHA1(3a2c222eca3e7df80ce69951b3db6442312751a4) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.18",   0x1000004, 0x400000, CRC(c1228b35) SHA1(7afdfb552888c79d0fbb30242b3d917b87fad57a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.20",   0x1000006, 0x400000, CRC(366cc6c2) SHA1(6f2a789087c8e404c5227b927fa8328c03593243) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "mvs.01",   0x00000, 0x08000, CRC(68252324) SHA1(138ef320ef27956b2ab5591d49a1315b7b0a194c) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "mvs.02",   0x28000, 0x20000, CRC(b34e773d) SHA1(3bcf44bf06c35814cff29d244142db7abe05bd39) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "mvs.11",   0x000000, 0x400000, CRC(86219770) SHA1(4e5b68d382a5aa37f8b0b6434c53a2b95f5f9a4d) )
	ROM_LOAD16_WORD_SWAP( "mvs.12",   0x400000, 0x400000, CRC(f2fd7f68) SHA1(28a30d55d3eaf963006c7cbe7c288099cd3ba536) )
ROM_END

ROM_START( mshvsfj2 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "mvsj.03g", 0x000000, 0x80000, CRC(fdfa7e26) SHA1(e9fb93249e48e1bb7c769c3ce674dd4be404574f) )
	ROM_LOAD16_WORD_SWAP( "mvsj.04g", 0x080000, 0x80000, CRC(c921825f) SHA1(471e44268cebba631b81f131bf31e27b8a28c548) )
	ROM_LOAD16_WORD_SWAP( "mvs.05a",  0x100000, 0x80000, CRC(1a5de0cb) SHA1(738a27f83704c208d36d73bf766d861ef2d51a89) )
	ROM_LOAD16_WORD_SWAP( "mvs.06a",  0x180000, 0x80000, CRC(959f3030) SHA1(fbbaa915324815246738f3426232e623f039ce26) )
	ROM_LOAD16_WORD_SWAP( "mvs.07b",  0x200000, 0x80000, CRC(7f915bdb) SHA1(683da09c5ba55e31b59aa95a8e13c45dc574ab3c) )
	ROM_LOAD16_WORD_SWAP( "mvs.08a",  0x280000, 0x80000, CRC(c2813884) SHA1(49e5d4bc48f90c8146cb6aafb9240aff0119f1a7) )
	ROM_LOAD16_WORD_SWAP( "mvs.09b",  0x300000, 0x80000, CRC(3ba08818) SHA1(9ab132a3cac55fcccebe6c99b6fb0ba1305f8f6e) )
	ROM_LOAD16_WORD_SWAP( "mvs.10b",  0x380000, 0x80000, CRC(cf0dba98) SHA1(f4c1f8a6e7a79ecc6241d5268b3039f8a09ea516) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
/*	ROM_LOAD16_WORD_SWAP( "mvsjx.03g", 0x000000, 0x80000, NO_DUMP )*/
/*	ROM_LOAD16_WORD_SWAP( "mvsjx.04g", 0x000000, 0x80000, NO_DUMP )*/

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "mvs.13",   0x0000000, 0x400000, CRC(29b05fd9) SHA1(e8fdb1ee5515a560eb4256ae4fd99bb1192e1a87) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.15",   0x0000002, 0x400000, CRC(faddccf1) SHA1(4ed03ea91883a0413325f57edcc1614120b5922c) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.17",   0x0000004, 0x400000, CRC(97aaf4c7) SHA1(6a054921cc14fe080cb3f62c391f8ae3cc7e8ba9) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.19",   0x0000006, 0x400000, CRC(cb70e915) SHA1(da4d2480d348ac6dfd01256a88f4f3db8357ae46) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.14",   0x1000000, 0x400000, CRC(b3b1972d) SHA1(0f2c3fb7de014181ee481ec35d0578b2c116c2dc) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.16",   0x1000002, 0x400000, CRC(08aadb5d) SHA1(3a2c222eca3e7df80ce69951b3db6442312751a4) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.18",   0x1000004, 0x400000, CRC(c1228b35) SHA1(7afdfb552888c79d0fbb30242b3d917b87fad57a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.20",   0x1000006, 0x400000, CRC(366cc6c2) SHA1(6f2a789087c8e404c5227b927fa8328c03593243) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "mvs.01",   0x00000, 0x08000, CRC(68252324) SHA1(138ef320ef27956b2ab5591d49a1315b7b0a194c) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "mvs.02",   0x28000, 0x20000, CRC(b34e773d) SHA1(3bcf44bf06c35814cff29d244142db7abe05bd39) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "mvs.11",   0x000000, 0x400000, CRC(86219770) SHA1(4e5b68d382a5aa37f8b0b6434c53a2b95f5f9a4d) )
	ROM_LOAD16_WORD_SWAP( "mvs.12",   0x400000, 0x400000, CRC(f2fd7f68) SHA1(28a30d55d3eaf963006c7cbe7c288099cd3ba536) )
ROM_END

ROM_START( mshvsfh )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "mvsh.03f", 0x000000, 0x80000, CRC(4f60f41e) SHA1(dd926a9cac4bff05845615d0b61948e2dc4b1ed8) )
	ROM_LOAD16_WORD_SWAP( "mvsh.04f", 0x080000, 0x80000, CRC(dc08ec12) SHA1(594e4383eb776c09075577cd1f4e42ef11748f0f) )
	ROM_LOAD16_WORD_SWAP( "mvs.05a",  0x100000, 0x80000, CRC(1a5de0cb) SHA1(738a27f83704c208d36d73bf766d861ef2d51a89) )
	ROM_LOAD16_WORD_SWAP( "mvs.06a",  0x180000, 0x80000, CRC(959f3030) SHA1(fbbaa915324815246738f3426232e623f039ce26) )
	ROM_LOAD16_WORD_SWAP( "mvs.07b",  0x200000, 0x80000, CRC(7f915bdb) SHA1(683da09c5ba55e31b59aa95a8e13c45dc574ab3c) )
	ROM_LOAD16_WORD_SWAP( "mvs.08a",  0x280000, 0x80000, CRC(c2813884) SHA1(49e5d4bc48f90c8146cb6aafb9240aff0119f1a7) )
	ROM_LOAD16_WORD_SWAP( "mvs.09b",  0x300000, 0x80000, CRC(3ba08818) SHA1(9ab132a3cac55fcccebe6c99b6fb0ba1305f8f6e) )
	ROM_LOAD16_WORD_SWAP( "mvs.10b",  0x380000, 0x80000, CRC(cf0dba98) SHA1(f4c1f8a6e7a79ecc6241d5268b3039f8a09ea516) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
/*	ROM_LOAD16_WORD_SWAP( "mvshx.03f", 0x000000, 0x80000, NO_DUMP )*/
/*	ROM_LOAD16_WORD_SWAP( "mvshx.04f", 0x080000, 0x80000, NO_DUMP )*/

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "mvs.13",   0x0000000, 0x400000, CRC(29b05fd9) SHA1(e8fdb1ee5515a560eb4256ae4fd99bb1192e1a87) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.15",   0x0000002, 0x400000, CRC(faddccf1) SHA1(4ed03ea91883a0413325f57edcc1614120b5922c) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.17",   0x0000004, 0x400000, CRC(97aaf4c7) SHA1(6a054921cc14fe080cb3f62c391f8ae3cc7e8ba9) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.19",   0x0000006, 0x400000, CRC(cb70e915) SHA1(da4d2480d348ac6dfd01256a88f4f3db8357ae46) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.14",   0x1000000, 0x400000, CRC(b3b1972d) SHA1(0f2c3fb7de014181ee481ec35d0578b2c116c2dc) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.16",   0x1000002, 0x400000, CRC(08aadb5d) SHA1(3a2c222eca3e7df80ce69951b3db6442312751a4) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.18",   0x1000004, 0x400000, CRC(c1228b35) SHA1(7afdfb552888c79d0fbb30242b3d917b87fad57a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.20",   0x1000006, 0x400000, CRC(366cc6c2) SHA1(6f2a789087c8e404c5227b927fa8328c03593243) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "mvs.01",   0x00000, 0x08000, CRC(68252324) SHA1(138ef320ef27956b2ab5591d49a1315b7b0a194c) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "mvs.02",   0x28000, 0x20000, CRC(b34e773d) SHA1(3bcf44bf06c35814cff29d244142db7abe05bd39) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "mvs.11",   0x000000, 0x400000, CRC(86219770) SHA1(4e5b68d382a5aa37f8b0b6434c53a2b95f5f9a4d) )
	ROM_LOAD16_WORD_SWAP( "mvs.12",   0x400000, 0x400000, CRC(f2fd7f68) SHA1(28a30d55d3eaf963006c7cbe7c288099cd3ba536) )
ROM_END

ROM_START( mshvsfa )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "mvsa.03d", 0x000000, 0x80000, CRC(5b863716) SHA1(6a129274711765bbf5acbb225e3fce6d93d7f421) )
	ROM_LOAD16_WORD_SWAP( "mvsa.04d", 0x080000, 0x80000, CRC(4886e65f) SHA1(758fc9c453a864e32588c7fb33166c93e798a39c) )
	ROM_LOAD16_WORD_SWAP( "mvs.05a",  0x100000, 0x80000, CRC(1a5de0cb) SHA1(738a27f83704c208d36d73bf766d861ef2d51a89) )
	ROM_LOAD16_WORD_SWAP( "mvs.06a",  0x180000, 0x80000, CRC(959f3030) SHA1(fbbaa915324815246738f3426232e623f039ce26) )
	ROM_LOAD16_WORD_SWAP( "mvs.07b",  0x200000, 0x80000, CRC(7f915bdb) SHA1(683da09c5ba55e31b59aa95a8e13c45dc574ab3c) )
	ROM_LOAD16_WORD_SWAP( "mvs.08a",  0x280000, 0x80000, CRC(c2813884) SHA1(49e5d4bc48f90c8146cb6aafb9240aff0119f1a7) )
	ROM_LOAD16_WORD_SWAP( "mvs.09b",  0x300000, 0x80000, CRC(3ba08818) SHA1(9ab132a3cac55fcccebe6c99b6fb0ba1305f8f6e) )
	ROM_LOAD16_WORD_SWAP( "mvs.10b",  0x380000, 0x80000, CRC(cf0dba98) SHA1(f4c1f8a6e7a79ecc6241d5268b3039f8a09ea516) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
/*	ROM_LOAD16_WORD_SWAP( "mvsax.03d", 0x000000, 0x80000, NO_DUMP )*/
/*	ROM_LOAD16_WORD_SWAP( "mvsax.04d", 0x080000, 0x80000, NO_DUMP )*/

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "mvs.13",   0x0000000, 0x400000, CRC(29b05fd9) SHA1(e8fdb1ee5515a560eb4256ae4fd99bb1192e1a87) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.15",   0x0000002, 0x400000, CRC(faddccf1) SHA1(4ed03ea91883a0413325f57edcc1614120b5922c) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.17",   0x0000004, 0x400000, CRC(97aaf4c7) SHA1(6a054921cc14fe080cb3f62c391f8ae3cc7e8ba9) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.19",   0x0000006, 0x400000, CRC(cb70e915) SHA1(da4d2480d348ac6dfd01256a88f4f3db8357ae46) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.14",   0x1000000, 0x400000, CRC(b3b1972d) SHA1(0f2c3fb7de014181ee481ec35d0578b2c116c2dc) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.16",   0x1000002, 0x400000, CRC(08aadb5d) SHA1(3a2c222eca3e7df80ce69951b3db6442312751a4) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.18",   0x1000004, 0x400000, CRC(c1228b35) SHA1(7afdfb552888c79d0fbb30242b3d917b87fad57a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.20",   0x1000006, 0x400000, CRC(366cc6c2) SHA1(6f2a789087c8e404c5227b927fa8328c03593243) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "mvs.01",   0x00000, 0x08000, CRC(68252324) SHA1(138ef320ef27956b2ab5591d49a1315b7b0a194c) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "mvs.02",   0x28000, 0x20000, CRC(b34e773d) SHA1(3bcf44bf06c35814cff29d244142db7abe05bd39) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "mvs.11",   0x000000, 0x400000, CRC(86219770) SHA1(4e5b68d382a5aa37f8b0b6434c53a2b95f5f9a4d) )
	ROM_LOAD16_WORD_SWAP( "mvs.12",   0x400000, 0x400000, CRC(f2fd7f68) SHA1(28a30d55d3eaf963006c7cbe7c288099cd3ba536) )
ROM_END

ROM_START( mshvsfa1 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "mvsa.03", 0x000000, 0x80000, CRC(92ef1933) SHA1(34e6a074734032af74afa52bfebbc213a9c886d7) )
	ROM_LOAD16_WORD_SWAP( "mvsa.04", 0x080000, 0x80000, CRC(4b24373c) SHA1(f340dda7d5339645fd1ea523e72783fb7bb7aba1) )
	ROM_LOAD16_WORD_SWAP( "mvs.05",  0x100000, 0x80000, CRC(ac180c1c) SHA1(1b368ebe7680796dc068b511b72359eec546cd9f) )
	ROM_LOAD16_WORD_SWAP( "mvs.06a", 0x180000, 0x80000, CRC(959f3030) SHA1(fbbaa915324815246738f3426232e623f039ce26) )
	ROM_LOAD16_WORD_SWAP( "mvs.07b", 0x200000, 0x80000, CRC(7f915bdb) SHA1(683da09c5ba55e31b59aa95a8e13c45dc574ab3c) )
	ROM_LOAD16_WORD_SWAP( "mvs.08a", 0x280000, 0x80000, CRC(c2813884) SHA1(49e5d4bc48f90c8146cb6aafb9240aff0119f1a7) )
	ROM_LOAD16_WORD_SWAP( "mvs.09b", 0x300000, 0x80000, CRC(3ba08818) SHA1(9ab132a3cac55fcccebe6c99b6fb0ba1305f8f6e) )
	ROM_LOAD16_WORD_SWAP( "mvs.10b", 0x380000, 0x80000, CRC(cf0dba98) SHA1(f4c1f8a6e7a79ecc6241d5268b3039f8a09ea516) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
/*	ROM_LOAD16_WORD_SWAP( "mvsax.03", 0x000000, 0x80000, NO_DUMP )*/
/*	ROM_LOAD16_WORD_SWAP( "mvsax.04", 0x080000, 0x80000, NO_DUMP )*/

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "mvs.13",   0x0000000, 0x400000, CRC(29b05fd9) SHA1(e8fdb1ee5515a560eb4256ae4fd99bb1192e1a87) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.15",   0x0000002, 0x400000, CRC(faddccf1) SHA1(4ed03ea91883a0413325f57edcc1614120b5922c) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.17",   0x0000004, 0x400000, CRC(97aaf4c7) SHA1(6a054921cc14fe080cb3f62c391f8ae3cc7e8ba9) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.19",   0x0000006, 0x400000, CRC(cb70e915) SHA1(da4d2480d348ac6dfd01256a88f4f3db8357ae46) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.14",   0x1000000, 0x400000, CRC(b3b1972d) SHA1(0f2c3fb7de014181ee481ec35d0578b2c116c2dc) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.16",   0x1000002, 0x400000, CRC(08aadb5d) SHA1(3a2c222eca3e7df80ce69951b3db6442312751a4) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.18",   0x1000004, 0x400000, CRC(c1228b35) SHA1(7afdfb552888c79d0fbb30242b3d917b87fad57a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.20",   0x1000006, 0x400000, CRC(366cc6c2) SHA1(6f2a789087c8e404c5227b927fa8328c03593243) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "mvs.01",   0x00000, 0x08000, CRC(68252324) SHA1(138ef320ef27956b2ab5591d49a1315b7b0a194c) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "mvs.02",   0x28000, 0x20000, CRC(b34e773d) SHA1(3bcf44bf06c35814cff29d244142db7abe05bd39) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "mvs.11",   0x000000, 0x400000, CRC(86219770) SHA1(4e5b68d382a5aa37f8b0b6434c53a2b95f5f9a4d) )
	ROM_LOAD16_WORD_SWAP( "mvs.12",   0x400000, 0x400000, CRC(f2fd7f68) SHA1(28a30d55d3eaf963006c7cbe7c288099cd3ba536) )
ROM_END

ROM_START( mshvsfb )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "mvsb.03g", 0x000000, 0x80000, CRC(143895ef) SHA1(0664fad64996118df86e9887bd6e301d04d84978) )
	ROM_LOAD16_WORD_SWAP( "mvsb.04g", 0x080000, 0x80000, CRC(dd8a886c) SHA1(a16f262fd14e726c7837980d0556a9c3bdc7fb11) )
	ROM_LOAD16_WORD_SWAP( "mvs.05d",  0x100000, 0x80000, CRC(921fc542) SHA1(b813082a480d42d663c713062892245faabe9101) )
	ROM_LOAD16_WORD_SWAP( "mvs.06a",  0x180000, 0x80000, CRC(959f3030) SHA1(fbbaa915324815246738f3426232e623f039ce26) )
	ROM_LOAD16_WORD_SWAP( "mvs.07b",  0x200000, 0x80000, CRC(7f915bdb) SHA1(683da09c5ba55e31b59aa95a8e13c45dc574ab3c) )
	ROM_LOAD16_WORD_SWAP( "mvs.08a",  0x280000, 0x80000, CRC(c2813884) SHA1(49e5d4bc48f90c8146cb6aafb9240aff0119f1a7) )
	ROM_LOAD16_WORD_SWAP( "mvs.09b",  0x300000, 0x80000, CRC(3ba08818) SHA1(9ab132a3cac55fcccebe6c99b6fb0ba1305f8f6e) )
	ROM_LOAD16_WORD_SWAP( "mvs.10b",  0x380000, 0x80000, CRC(cf0dba98) SHA1(f4c1f8a6e7a79ecc6241d5268b3039f8a09ea516) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
/*	ROM_LOAD16_WORD_SWAP( "mvsbx.03g", 0x000000, 0x80000, NO_DUMP )*/
/*	ROM_LOAD16_WORD_SWAP( "mvsbx.04g", 0x080000, 0x80000, NO_DUMP )*/

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "mvs.13",   0x0000000, 0x400000, CRC(29b05fd9) SHA1(e8fdb1ee5515a560eb4256ae4fd99bb1192e1a87) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.15",   0x0000002, 0x400000, CRC(faddccf1) SHA1(4ed03ea91883a0413325f57edcc1614120b5922c) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.17",   0x0000004, 0x400000, CRC(97aaf4c7) SHA1(6a054921cc14fe080cb3f62c391f8ae3cc7e8ba9) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.19",   0x0000006, 0x400000, CRC(cb70e915) SHA1(da4d2480d348ac6dfd01256a88f4f3db8357ae46) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.14",   0x1000000, 0x400000, CRC(b3b1972d) SHA1(0f2c3fb7de014181ee481ec35d0578b2c116c2dc) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.16",   0x1000002, 0x400000, CRC(08aadb5d) SHA1(3a2c222eca3e7df80ce69951b3db6442312751a4) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.18",   0x1000004, 0x400000, CRC(c1228b35) SHA1(7afdfb552888c79d0fbb30242b3d917b87fad57a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.20",   0x1000006, 0x400000, CRC(366cc6c2) SHA1(6f2a789087c8e404c5227b927fa8328c03593243) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "mvs.01",   0x00000, 0x08000, CRC(68252324) SHA1(138ef320ef27956b2ab5591d49a1315b7b0a194c) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "mvs.02",   0x28000, 0x20000, CRC(b34e773d) SHA1(3bcf44bf06c35814cff29d244142db7abe05bd39) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "mvs.11",   0x000000, 0x400000, CRC(86219770) SHA1(4e5b68d382a5aa37f8b0b6434c53a2b95f5f9a4d) )
	ROM_LOAD16_WORD_SWAP( "mvs.12",   0x400000, 0x400000, CRC(f2fd7f68) SHA1(28a30d55d3eaf963006c7cbe7c288099cd3ba536) )
ROM_END

ROM_START( mshvsfb1 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "mvsb.03f", 0x000000, 0x80000, CRC(9c4bb950) SHA1(85d0026a691682c195d6e617bf28def50029cb03) )
	ROM_LOAD16_WORD_SWAP( "mvsb.04f", 0x080000, 0x80000, CRC(d3320d13) SHA1(c6fa2b8b727a1192fd21131496067447053b5547) )
	ROM_LOAD16_WORD_SWAP( "mvs.05a",  0x100000, 0x80000, CRC(1a5de0cb) SHA1(738a27f83704c208d36d73bf766d861ef2d51a89) )
	ROM_LOAD16_WORD_SWAP( "mvs.06a",  0x180000, 0x80000, CRC(959f3030) SHA1(fbbaa915324815246738f3426232e623f039ce26) )
	ROM_LOAD16_WORD_SWAP( "mvs.07b",  0x200000, 0x80000, CRC(7f915bdb) SHA1(683da09c5ba55e31b59aa95a8e13c45dc574ab3c) )
	ROM_LOAD16_WORD_SWAP( "mvs.08a",  0x280000, 0x80000, CRC(c2813884) SHA1(49e5d4bc48f90c8146cb6aafb9240aff0119f1a7) )
	ROM_LOAD16_WORD_SWAP( "mvs.09b",  0x300000, 0x80000, CRC(3ba08818) SHA1(9ab132a3cac55fcccebe6c99b6fb0ba1305f8f6e) )
	ROM_LOAD16_WORD_SWAP( "mvs.10b",  0x380000, 0x80000, CRC(cf0dba98) SHA1(f4c1f8a6e7a79ecc6241d5268b3039f8a09ea516) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
/*	ROM_LOAD16_WORD_SWAP( "mvsbx.03f", 0x000000, 0x80000, NO_DUMP )*/
/*	ROM_LOAD16_WORD_SWAP( "mvsbx.04f", 0x080000, 0x80000, NO_DUMP )*/

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "mvs.13",   0x0000000, 0x400000, CRC(29b05fd9) SHA1(e8fdb1ee5515a560eb4256ae4fd99bb1192e1a87) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.15",   0x0000002, 0x400000, CRC(faddccf1) SHA1(4ed03ea91883a0413325f57edcc1614120b5922c) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.17",   0x0000004, 0x400000, CRC(97aaf4c7) SHA1(6a054921cc14fe080cb3f62c391f8ae3cc7e8ba9) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.19",   0x0000006, 0x400000, CRC(cb70e915) SHA1(da4d2480d348ac6dfd01256a88f4f3db8357ae46) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.14",   0x1000000, 0x400000, CRC(b3b1972d) SHA1(0f2c3fb7de014181ee481ec35d0578b2c116c2dc) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.16",   0x1000002, 0x400000, CRC(08aadb5d) SHA1(3a2c222eca3e7df80ce69951b3db6442312751a4) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.18",   0x1000004, 0x400000, CRC(c1228b35) SHA1(7afdfb552888c79d0fbb30242b3d917b87fad57a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.20",   0x1000006, 0x400000, CRC(366cc6c2) SHA1(6f2a789087c8e404c5227b927fa8328c03593243) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "mvs.01",   0x00000, 0x08000, CRC(68252324) SHA1(138ef320ef27956b2ab5591d49a1315b7b0a194c) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "mvs.02",   0x28000, 0x20000, CRC(b34e773d) SHA1(3bcf44bf06c35814cff29d244142db7abe05bd39) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "mvs.11",   0x000000, 0x400000, CRC(86219770) SHA1(4e5b68d382a5aa37f8b0b6434c53a2b95f5f9a4d) )
	ROM_LOAD16_WORD_SWAP( "mvs.12",   0x400000, 0x400000, CRC(f2fd7f68) SHA1(28a30d55d3eaf963006c7cbe7c288099cd3ba536) )
ROM_END

ROM_START( mvsc )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "mvcu.03d", 0x000000, 0x80000, CRC(c6007557) SHA1(c027c1a204345ce611cb042d60939e4de156763f) )
	ROM_LOAD16_WORD_SWAP( "mvcu.04d", 0x080000, 0x80000, CRC(724b2b20) SHA1(872bbcf5d344d634f3523318fa4763e6d6302bb5) )
	ROM_LOAD16_WORD_SWAP( "mvc.05a",  0x100000, 0x80000, CRC(2d8c8e86) SHA1(b07d640a734c5d336054ed05195786224c9a6cd4) )
	ROM_LOAD16_WORD_SWAP( "mvc.06a",  0x180000, 0x80000, CRC(8528e1f5) SHA1(cd065c05268ab581b05676da544baf6af642acac) )
	ROM_LOAD16_WORD_SWAP( "mvc.07",   0x200000, 0x80000, CRC(c3baa32b) SHA1(d35589847e0753e869ffcd7c3abed925bfdb0fa2) )
	ROM_LOAD16_WORD_SWAP( "mvc.08",   0x280000, 0x80000, CRC(bc002fcd) SHA1(0b6735a071a9274f7ab25c743271fc30411fe819) )
	ROM_LOAD16_WORD_SWAP( "mvc.09",   0x300000, 0x80000, CRC(c67b26df) SHA1(6e9969246c57269d7ba0992a5cc319c8910bf8a9) )
	ROM_LOAD16_WORD_SWAP( "mvc.10",   0x380000, 0x80000, CRC(0fdd1e26) SHA1(5fa684d823b4f4eec61ed9e9b8938af5272ae1ed) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "mvcux.03d", 0x000000, 0x80000, CRC(86685cbf) SHA1(f1dd866bf564ae30c516b1a33e6d5c86bb9cd523) )
	ROM_LOAD16_WORD_SWAP( "mvcux.04d", 0x080000, 0x80000, CRC(438ba92b) SHA1(e4aae2051c71eff4e15529fff695842a0103d52a) )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "mvc.13",   0x0000000, 0x400000, CRC(fa5f74bc) SHA1(79a619248938a85ce4f7794a704647b9cf564fbc) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvc.15",   0x0000002, 0x400000, CRC(71938a8f) SHA1(6982f7203458c1c46a1c1c13c0d0f2a5e109d271) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvc.17",   0x0000004, 0x400000, CRC(92741d07) SHA1(ddfd70eab7c983ab452194b1860059f8ad694459) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvc.19",   0x0000006, 0x400000, CRC(bcb72fc6) SHA1(46ab98dcdf6f5d611646a22a7355939ef5b2bbe5) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvc.14",   0x1000000, 0x400000, CRC(7f1df4e4) SHA1(ede92b31c1fe87f91b4fe74ac211f2fb5f863bc2) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvc.16",   0x1000002, 0x400000, CRC(90bd3203) SHA1(ed83208c486ea0f407b7e5d16a8cf242a6f73774) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvc.18",   0x1000004, 0x400000, CRC(67aaf727) SHA1(e0e69104e31d2c41e18c0d24e9ab962406a7ca9a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvc.20",   0x1000006, 0x400000, CRC(8b0bade8) SHA1(c5732361bb4bf284c4d12a82ac2c5750b1f9d441) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "mvc.01",   0x00000, 0x08000, CRC(41629e95) SHA1(36925c05b5fdcbe43283a882d021e5360c947061) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "mvc.02",   0x28000, 0x20000, CRC(963abf6b) SHA1(6b784870e338701cefabbbe4669984b5c4e8a9a5) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "mvc.11",   0x000000, 0x400000, CRC(850fe663) SHA1(81e519d05a08855f242ea2e17ee0859b449db895) )
	ROM_LOAD16_WORD_SWAP( "mvc.12",   0x400000, 0x400000, CRC(7ccb1896) SHA1(74caadf3282fcc6acffb1bbe3734106f81124121) )
ROM_END

ROM_START( mvscj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "mvcj.03a", 0x000000, 0x80000, CRC(3df18879) SHA1(2b91da6e5dd792967337e873ebb08ecf5194a97b) )
	ROM_LOAD16_WORD_SWAP( "mvcj.04a", 0x080000, 0x80000, CRC(07d212e8) SHA1(c5420e9bd580910c1f1d0264240aeef20aac30a7) )
	ROM_LOAD16_WORD_SWAP( "mvc.05a",  0x100000, 0x80000, CRC(2d8c8e86) SHA1(b07d640a734c5d336054ed05195786224c9a6cd4) )
	ROM_LOAD16_WORD_SWAP( "mvc.06a",  0x180000, 0x80000, CRC(8528e1f5) SHA1(cd065c05268ab581b05676da544baf6af642acac) )
	ROM_LOAD16_WORD_SWAP( "mvc.07",   0x200000, 0x80000, CRC(c3baa32b) SHA1(d35589847e0753e869ffcd7c3abed925bfdb0fa2) )
	ROM_LOAD16_WORD_SWAP( "mvc.08",   0x280000, 0x80000, CRC(bc002fcd) SHA1(0b6735a071a9274f7ab25c743271fc30411fe819) )
	ROM_LOAD16_WORD_SWAP( "mvc.09",   0x300000, 0x80000, CRC(c67b26df) SHA1(6e9969246c57269d7ba0992a5cc319c8910bf8a9) )
	ROM_LOAD16_WORD_SWAP( "mvc.10",   0x380000, 0x80000, CRC(0fdd1e26) SHA1(5fa684d823b4f4eec61ed9e9b8938af5272ae1ed) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "mvcjx.03a", 0x000000, 0x80000, CRC(e696a098) SHA1(2ff7bb4642e1a5611f29789cccffc92595e259df) )
	ROM_LOAD16_WORD_SWAP( "mvcjx.04a", 0x080000, 0x80000, CRC(7faeee82) SHA1(eeb0967698e3f84d9d11d713bd5b016fe5cc899c) )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "mvc.13",   0x0000000, 0x400000, CRC(fa5f74bc) SHA1(79a619248938a85ce4f7794a704647b9cf564fbc) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvc.15",   0x0000002, 0x400000, CRC(71938a8f) SHA1(6982f7203458c1c46a1c1c13c0d0f2a5e109d271) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvc.17",   0x0000004, 0x400000, CRC(92741d07) SHA1(ddfd70eab7c983ab452194b1860059f8ad694459) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvc.19",   0x0000006, 0x400000, CRC(bcb72fc6) SHA1(46ab98dcdf6f5d611646a22a7355939ef5b2bbe5) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvc.14",   0x1000000, 0x400000, CRC(7f1df4e4) SHA1(ede92b31c1fe87f91b4fe74ac211f2fb5f863bc2) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvc.16",   0x1000002, 0x400000, CRC(90bd3203) SHA1(ed83208c486ea0f407b7e5d16a8cf242a6f73774) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvc.18",   0x1000004, 0x400000, CRC(67aaf727) SHA1(e0e69104e31d2c41e18c0d24e9ab962406a7ca9a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvc.20",   0x1000006, 0x400000, CRC(8b0bade8) SHA1(c5732361bb4bf284c4d12a82ac2c5750b1f9d441) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "mvc.01",   0x00000, 0x08000, CRC(41629e95) SHA1(36925c05b5fdcbe43283a882d021e5360c947061) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "mvc.02",   0x28000, 0x20000, CRC(963abf6b) SHA1(6b784870e338701cefabbbe4669984b5c4e8a9a5) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "mvc.11",   0x000000, 0x400000, CRC(850fe663) SHA1(81e519d05a08855f242ea2e17ee0859b449db895) )
	ROM_LOAD16_WORD_SWAP( "mvc.12",   0x400000, 0x400000, CRC(7ccb1896) SHA1(74caadf3282fcc6acffb1bbe3734106f81124121) )
ROM_END

ROM_START( mvscjr1 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "mvcj.03", 0x000000, 0x80000, CRC(2164213f) SHA1(00e3500ed334bb80d4d159eacf439860a2bfc3b7) )
	ROM_LOAD16_WORD_SWAP( "mvcj.04", 0x080000, 0x80000, CRC(c905c86f) SHA1(965fa3bdc29bd901e9efcc53b195c6be3a74c9f9) )
	ROM_LOAD16_WORD_SWAP( "mvc.05",  0x100000, 0x80000, CRC(7db71ce9) SHA1(a0097109e9f4aba40791932269d600c0ffa099a7) )
	ROM_LOAD16_WORD_SWAP( "mvc.06",  0x180000, 0x80000, CRC(4b0b6d3e) SHA1(375372adf0a508bb6fc6a79326b2d4171db9ca0f) )
	ROM_LOAD16_WORD_SWAP( "mvc.07",  0x200000, 0x80000, CRC(c3baa32b) SHA1(d35589847e0753e869ffcd7c3abed925bfdb0fa2) )
	ROM_LOAD16_WORD_SWAP( "mvc.08",  0x280000, 0x80000, CRC(bc002fcd) SHA1(0b6735a071a9274f7ab25c743271fc30411fe819) )
	ROM_LOAD16_WORD_SWAP( "mvc.09",  0x300000, 0x80000, CRC(c67b26df) SHA1(6e9969246c57269d7ba0992a5cc319c8910bf8a9) )
	ROM_LOAD16_WORD_SWAP( "mvc.10",  0x380000, 0x80000, CRC(0fdd1e26) SHA1(5fa684d823b4f4eec61ed9e9b8938af5272ae1ed) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "mvcjx.03", 0x000000, 0x80000, CRC(cc94ab89) SHA1(405c6ba53f1e91cbc817c9486436447b6f27c556) )
	ROM_LOAD16_WORD_SWAP( "mvcjx.04", 0x080000, 0x80000, CRC(c5a83202) SHA1(a113c6a5f4244eae8f9d2f62e342a8dbd4e4c553) )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "mvc.13",   0x0000000, 0x400000, CRC(fa5f74bc) SHA1(79a619248938a85ce4f7794a704647b9cf564fbc) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvc.15",   0x0000002, 0x400000, CRC(71938a8f) SHA1(6982f7203458c1c46a1c1c13c0d0f2a5e109d271) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvc.17",   0x0000004, 0x400000, CRC(92741d07) SHA1(ddfd70eab7c983ab452194b1860059f8ad694459) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvc.19",   0x0000006, 0x400000, CRC(bcb72fc6) SHA1(46ab98dcdf6f5d611646a22a7355939ef5b2bbe5) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvc.14",   0x1000000, 0x400000, CRC(7f1df4e4) SHA1(ede92b31c1fe87f91b4fe74ac211f2fb5f863bc2) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvc.16",   0x1000002, 0x400000, CRC(90bd3203) SHA1(ed83208c486ea0f407b7e5d16a8cf242a6f73774) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvc.18",   0x1000004, 0x400000, CRC(67aaf727) SHA1(e0e69104e31d2c41e18c0d24e9ab962406a7ca9a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvc.20",   0x1000006, 0x400000, CRC(8b0bade8) SHA1(c5732361bb4bf284c4d12a82ac2c5750b1f9d441) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "mvc.01",   0x00000, 0x08000, CRC(41629e95) SHA1(36925c05b5fdcbe43283a882d021e5360c947061) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "mvc.02",   0x28000, 0x20000, CRC(963abf6b) SHA1(6b784870e338701cefabbbe4669984b5c4e8a9a5) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "mvc.11",   0x000000, 0x400000, CRC(850fe663) SHA1(81e519d05a08855f242ea2e17ee0859b449db895) )
	ROM_LOAD16_WORD_SWAP( "mvc.12",   0x400000, 0x400000, CRC(7ccb1896) SHA1(74caadf3282fcc6acffb1bbe3734106f81124121) )
ROM_END

ROM_START( mvsca )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "mvca.03", 0x000000, 0x80000, CRC(fe5fa7b9) SHA1(c27b987ffb631c3433aa32a29989dbf2b3e53f1e) )
	ROM_LOAD16_WORD_SWAP( "mvca.04", 0x080000, 0x80000, CRC(082b701c) SHA1(363770ecd5f4e160db6448845ba6d7fd0beea291) )
	ROM_LOAD16_WORD_SWAP( "mvc.05",  0x100000, 0x80000, CRC(7db71ce9) SHA1(a0097109e9f4aba40791932269d600c0ffa099a7) )
	ROM_LOAD16_WORD_SWAP( "mvc.06",  0x180000, 0x80000, CRC(4b0b6d3e) SHA1(375372adf0a508bb6fc6a79326b2d4171db9ca0f) )
	ROM_LOAD16_WORD_SWAP( "mvc.07",  0x200000, 0x80000, CRC(c3baa32b) SHA1(d35589847e0753e869ffcd7c3abed925bfdb0fa2) )
	ROM_LOAD16_WORD_SWAP( "mvc.08",  0x280000, 0x80000, CRC(bc002fcd) SHA1(0b6735a071a9274f7ab25c743271fc30411fe819) )
	ROM_LOAD16_WORD_SWAP( "mvc.09",  0x300000, 0x80000, CRC(c67b26df) SHA1(6e9969246c57269d7ba0992a5cc319c8910bf8a9) )
	ROM_LOAD16_WORD_SWAP( "mvc.10",  0x380000, 0x80000, CRC(0fdd1e26) SHA1(5fa684d823b4f4eec61ed9e9b8938af5272ae1ed) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "mvcax.03", 0x000000, 0x80000, CRC(4512a6af) SHA1(6586973cc89213ddb1a4dab0f9e764b96c861515) )
	ROM_LOAD16_WORD_SWAP( "mvcax.04", 0x080000, 0x80000, CRC(e6f537d2) SHA1(25c1ec6318bd84f2fb1918ea1279f7fbde23428d) )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "mvc.13",   0x0000000, 0x400000, CRC(fa5f74bc) SHA1(79a619248938a85ce4f7794a704647b9cf564fbc) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvc.15",   0x0000002, 0x400000, CRC(71938a8f) SHA1(6982f7203458c1c46a1c1c13c0d0f2a5e109d271) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvc.17",   0x0000004, 0x400000, CRC(92741d07) SHA1(ddfd70eab7c983ab452194b1860059f8ad694459) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvc.19",   0x0000006, 0x400000, CRC(bcb72fc6) SHA1(46ab98dcdf6f5d611646a22a7355939ef5b2bbe5) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvc.14",   0x1000000, 0x400000, CRC(7f1df4e4) SHA1(ede92b31c1fe87f91b4fe74ac211f2fb5f863bc2) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvc.16",   0x1000002, 0x400000, CRC(90bd3203) SHA1(ed83208c486ea0f407b7e5d16a8cf242a6f73774) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvc.18",   0x1000004, 0x400000, CRC(67aaf727) SHA1(e0e69104e31d2c41e18c0d24e9ab962406a7ca9a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvc.20",   0x1000006, 0x400000, CRC(8b0bade8) SHA1(c5732361bb4bf284c4d12a82ac2c5750b1f9d441) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "mvc.01",   0x00000, 0x08000, CRC(41629e95) SHA1(36925c05b5fdcbe43283a882d021e5360c947061) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "mvc.02",   0x28000, 0x20000, CRC(963abf6b) SHA1(6b784870e338701cefabbbe4669984b5c4e8a9a5) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "mvc.11",   0x000000, 0x400000, CRC(850fe663) SHA1(81e519d05a08855f242ea2e17ee0859b449db895) )
	ROM_LOAD16_WORD_SWAP( "mvc.12",   0x400000, 0x400000, CRC(7ccb1896) SHA1(74caadf3282fcc6acffb1bbe3734106f81124121) )
ROM_END

ROM_START( mvsch )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "mvch.03", 0x000000, 0x80000, CRC(6a0ec9f7) SHA1(62d7b28cc9ddf975ccdc8992d51bd3d085e3e136) )
	ROM_LOAD16_WORD_SWAP( "mvch.04", 0x080000, 0x80000, CRC(00f03fa4) SHA1(3a79400a7ac6e7594ca7e0fbb2486ddf6c759d18) )
	ROM_LOAD16_WORD_SWAP( "mvc.05a", 0x100000, 0x80000, CRC(2d8c8e86) SHA1(b07d640a734c5d336054ed05195786224c9a6cd4) )
	ROM_LOAD16_WORD_SWAP( "mvc.06a", 0x180000, 0x80000, CRC(8528e1f5) SHA1(cd065c05268ab581b05676da544baf6af642acac) )
	ROM_LOAD16_WORD_SWAP( "mvc.07",  0x200000, 0x80000, CRC(c3baa32b) SHA1(d35589847e0753e869ffcd7c3abed925bfdb0fa2) )
	ROM_LOAD16_WORD_SWAP( "mvc.08",  0x280000, 0x80000, CRC(bc002fcd) SHA1(0b6735a071a9274f7ab25c743271fc30411fe819) )
	ROM_LOAD16_WORD_SWAP( "mvc.09",  0x300000, 0x80000, CRC(c67b26df) SHA1(6e9969246c57269d7ba0992a5cc319c8910bf8a9) )
	ROM_LOAD16_WORD_SWAP( "mvc.10",  0x380000, 0x80000, CRC(0fdd1e26) SHA1(5fa684d823b4f4eec61ed9e9b8938af5272ae1ed) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
/*	ROM_LOAD16_WORD_SWAP( "mvchx.03", 0x000000, 0x80000, NO_DUMP )*/
/*	ROM_LOAD16_WORD_SWAP( "mvchx.04", 0x080000, 0x80000, NO_DUMP )*/

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "mvc.13",   0x0000000, 0x400000, CRC(fa5f74bc) SHA1(79a619248938a85ce4f7794a704647b9cf564fbc) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvc.15",   0x0000002, 0x400000, CRC(71938a8f) SHA1(6982f7203458c1c46a1c1c13c0d0f2a5e109d271) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvc.17",   0x0000004, 0x400000, CRC(92741d07) SHA1(ddfd70eab7c983ab452194b1860059f8ad694459) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvc.19",   0x0000006, 0x400000, CRC(bcb72fc6) SHA1(46ab98dcdf6f5d611646a22a7355939ef5b2bbe5) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvc.14",   0x1000000, 0x400000, CRC(7f1df4e4) SHA1(ede92b31c1fe87f91b4fe74ac211f2fb5f863bc2) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvc.16",   0x1000002, 0x400000, CRC(90bd3203) SHA1(ed83208c486ea0f407b7e5d16a8cf242a6f73774) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvc.18",   0x1000004, 0x400000, CRC(67aaf727) SHA1(e0e69104e31d2c41e18c0d24e9ab962406a7ca9a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvc.20",   0x1000006, 0x400000, CRC(8b0bade8) SHA1(c5732361bb4bf284c4d12a82ac2c5750b1f9d441) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "mvc.01",   0x00000, 0x08000, CRC(41629e95) SHA1(36925c05b5fdcbe43283a882d021e5360c947061) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "mvc.02",   0x28000, 0x20000, CRC(963abf6b) SHA1(6b784870e338701cefabbbe4669984b5c4e8a9a5) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "mvc.11",   0x000000, 0x400000, CRC(850fe663) SHA1(81e519d05a08855f242ea2e17ee0859b449db895) )
	ROM_LOAD16_WORD_SWAP( "mvc.12",   0x400000, 0x400000, CRC(7ccb1896) SHA1(74caadf3282fcc6acffb1bbe3734106f81124121) )
ROM_END

ROM_START( mpang )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "mpne.03c", 0x000000, 0x80000, CRC(fe16fc9f) SHA1(be22bf8a0abd53d2e7ebc12d3d0020bf799a25e9) )
	ROM_LOAD16_WORD_SWAP( "mpne.04c", 0x080000, 0x80000, CRC(2cc5ec22) SHA1(c188349c26a64bad325cfa218849ed1e94303087) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROM_LOAD64_BYTE( "mpn-simm.01c",   0x0000000, 0x200000, CRC(388db66b) SHA1(7416cce3d0dbea71c92ea9f72f5536146f757b45) ) /* ROM on a SIMM */
	ROM_LOAD64_BYTE( "mpn-simm.01d",   0x0000001, 0x200000, CRC(aff1b494) SHA1(d376c02ce01e71a7707d3d3fe5b0ae59ce781686) ) /* ROM on a SIMM */
	ROM_LOAD64_BYTE( "mpn-simm.01a",   0x0000002, 0x200000, CRC(a9c4857b) SHA1(66f538105c710d1480141e48a15b1a760f5ce985) ) /* ROM on a SIMM */
	ROM_LOAD64_BYTE( "mpn-simm.01b",   0x0000003, 0x200000, CRC(f759df22) SHA1(1678e3e819dd808f3a6fdd52b7c933eac4777b5b) ) /* ROM on a SIMM */
	ROM_LOAD64_BYTE( "mpn-simm.03c",   0x0000004, 0x200000, CRC(dec6b720) SHA1(331776e1cba3fb82071e7c2195dc4ae27b3613a2) ) /* ROM on a SIMM */
	ROM_LOAD64_BYTE( "mpn-simm.03d",   0x0000005, 0x200000, CRC(f8774c18) SHA1(58e0ea4dd62e39bcfaa3a2be4ef08eb2f0bd3c00) ) /* ROM on a SIMM */
	ROM_LOAD64_BYTE( "mpn-simm.03a",   0x0000006, 0x200000, CRC(c2aea4ec) SHA1(f5e2a815fa802598611efa48e5de97e929155e77) ) /* ROM on a SIMM */
	ROM_LOAD64_BYTE( "mpn-simm.03b",   0x0000007, 0x200000, CRC(84d6dc33) SHA1(f5ababb479facc08c425381570644230c09334e7) ) /* ROM on a SIMM */

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "mpn.01",   0x00000, 0x08000, CRC(90c7adb6) SHA1(a2653e977e5e0457b249e098e5ca0abc93dac336) )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "mpn-simm.05a",   0x000000, 0x200000, CRC(318a2e21) SHA1(c573cd88f8279a062c73ef1d79cd8421dbdcd93e) ) /* ROM on a SIMM */
	ROM_LOAD16_WORD_SWAP( "mpn-simm.05b",   0x200000, 0x200000, CRC(5462f4e8) SHA1(299fbdab700e735e6395c5d9e3f079bb2e3dbd73) ) /* ROM on a SIMM */

	ROM_REGION( 0x20, REGION_USER5, 0 )
	ROM_LOAD( "mpang.key",    0x000000, 0x000014, CRC(95354b0f) SHA1(6e015eda29ac94aa420d7c70cdebffee52fde032) )
ROM_END

ROM_START( mpangr1 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "mpne.03b", 0x000000, 0x80000, CRC(6ef0f9b2) SHA1(fd1c76e151466fe27b02a6d07683fdd9c6a4816d) )
	ROM_LOAD16_WORD_SWAP( "mpne.04b", 0x080000, 0x80000, CRC(30a468bb) SHA1(f2f8f600a079ff050c9a0c12e1a6368943bdc536) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROM_LOAD64_BYTE( "mpn-simm.01c",   0x0000000, 0x200000, CRC(388db66b) SHA1(7416cce3d0dbea71c92ea9f72f5536146f757b45) ) /* ROM on a SIMM */
	ROM_LOAD64_BYTE( "mpn-simm.01d",   0x0000001, 0x200000, CRC(aff1b494) SHA1(d376c02ce01e71a7707d3d3fe5b0ae59ce781686) ) /* ROM on a SIMM */
	ROM_LOAD64_BYTE( "mpn-simm.01a",   0x0000002, 0x200000, CRC(a9c4857b) SHA1(66f538105c710d1480141e48a15b1a760f5ce985) ) /* ROM on a SIMM */
	ROM_LOAD64_BYTE( "mpn-simm.01b",   0x0000003, 0x200000, CRC(f759df22) SHA1(1678e3e819dd808f3a6fdd52b7c933eac4777b5b) ) /* ROM on a SIMM */
	ROM_LOAD64_BYTE( "mpn-simm.03c",   0x0000004, 0x200000, CRC(dec6b720) SHA1(331776e1cba3fb82071e7c2195dc4ae27b3613a2) ) /* ROM on a SIMM */
	ROM_LOAD64_BYTE( "mpn-simm.03d",   0x0000005, 0x200000, CRC(f8774c18) SHA1(58e0ea4dd62e39bcfaa3a2be4ef08eb2f0bd3c00) ) /* ROM on a SIMM */
	ROM_LOAD64_BYTE( "mpn-simm.03a",   0x0000006, 0x200000, CRC(c2aea4ec) SHA1(f5e2a815fa802598611efa48e5de97e929155e77) ) /* ROM on a SIMM */
	ROM_LOAD64_BYTE( "mpn-simm.03b",   0x0000007, 0x200000, CRC(84d6dc33) SHA1(f5ababb479facc08c425381570644230c09334e7) ) /* ROM on a SIMM */

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "mpn.01",   0x00000, 0x08000, CRC(90c7adb6) SHA1(a2653e977e5e0457b249e098e5ca0abc93dac336) )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "mpn-simm.05a",   0x000000, 0x200000, CRC(318a2e21) SHA1(c573cd88f8279a062c73ef1d79cd8421dbdcd93e) ) /* ROM on a SIMM */
	ROM_LOAD16_WORD_SWAP( "mpn-simm.05b",   0x200000, 0x200000, CRC(5462f4e8) SHA1(299fbdab700e735e6395c5d9e3f079bb2e3dbd73) ) /* ROM on a SIMM */

	ROM_REGION( 0x20, REGION_USER5, 0 )
	ROM_LOAD( "mpang.key",    0x000000, 0x000014, CRC(95354b0f) SHA1(6e015eda29ac94aa420d7c70cdebffee52fde032) )
ROM_END

ROM_START( mpangu )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "mpnu.03", 0x000000, 0x80000, CRC(6e7ed03c) SHA1(3562362d9573252d4d19dbfd0ec7e47e9eaa5f46) ) /* USA version, but has no "u" in label code */
	ROM_LOAD16_WORD_SWAP( "mpnu.04", 0x080000, 0x80000, CRC(de079131) SHA1(95da2a03cb642963aabfebd8337003961ca4db71) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROM_FILL(              0x000000, 0x800000, 0x00 )
	ROM_LOAD64_WORD( "mpn.13m",  0x800000, 0x200000, CRC(c5f123dc) SHA1(e459571416ab64f0280cda7dfa0d8836faa745c9) ) /* This version uses roms chips */
	ROM_LOAD64_WORD( "mpn.15m",  0x800002, 0x200000, CRC(8e033265) SHA1(d05ca8fb825423ff6b099c06aaa500c0a947454a) )
	ROM_LOAD64_WORD( "mpn.17m",  0x800004, 0x200000, CRC(cfcd73d2) SHA1(3415c063ac3632159df0fa74899bde8c90f4a2a1) )
	ROM_LOAD64_WORD( "mpn.19m",  0x800006, 0x200000, CRC(2db1ffbc) SHA1(ca58c9359c2c629896c40e78c14907f562e2cdad) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "mpn.01",   0x00000, 0x08000, CRC(90c7adb6) SHA1(a2653e977e5e0457b249e098e5ca0abc93dac336) )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD( "mpn.q1",   0x000000, 0x100000, CRC(d21c1f5a) SHA1(94cfcf01e656c0fb690e6204964ac70fbc89064d) ) /* This version uses roms chips */
	ROM_LOAD( "mpn.q2",   0x100000, 0x100000, CRC(d22090b1) SHA1(0ba65c0efb46af3cfb8ea3fe5087186248c57420) )
	ROM_LOAD( "mpn.q3",   0x200000, 0x100000, CRC(60aa5ef2) SHA1(03fa994d7f5b43e05e8417a8769e07c22548a27a) )
	ROM_LOAD( "mpn.q4",   0x300000, 0x100000, CRC(3a67d203) SHA1(7213364745d0c4c7fe11573afb9678483e24acb4) )

	ROM_REGION( 0x20, REGION_USER5, 0 )
	ROM_LOAD( "mpang.key",    0x000000, 0x000014, CRC(95354b0f) SHA1(6e015eda29ac94aa420d7c70cdebffee52fde032) )
ROM_END

ROM_START( mpangj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "mpnj.03a", 0x000000, 0x80000, CRC(bf597b1c) SHA1(0412e826eec7a9f3e70c84b64c9fbcecf7e0c56a) )
	ROM_LOAD16_WORD_SWAP( "mpnj.04a", 0x080000, 0x80000, CRC(f4a3ab0f) SHA1(2e54bbc95304827fcd24dab35e4895f4e6566be0) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROM_LOAD64_BYTE( "mpn-simm.01c",   0x0000000, 0x200000, CRC(388db66b) SHA1(7416cce3d0dbea71c92ea9f72f5536146f757b45) ) /* ROM on a SIMM */
	ROM_LOAD64_BYTE( "mpn-simm.01d",   0x0000001, 0x200000, CRC(aff1b494) SHA1(d376c02ce01e71a7707d3d3fe5b0ae59ce781686) ) /* ROM on a SIMM */
	ROM_LOAD64_BYTE( "mpn-simm.01a",   0x0000002, 0x200000, CRC(a9c4857b) SHA1(66f538105c710d1480141e48a15b1a760f5ce985) ) /* ROM on a SIMM */
	ROM_LOAD64_BYTE( "mpn-simm.01b",   0x0000003, 0x200000, CRC(f759df22) SHA1(1678e3e819dd808f3a6fdd52b7c933eac4777b5b) ) /* ROM on a SIMM */
	ROM_LOAD64_BYTE( "mpn-simm.03c",   0x0000004, 0x200000, CRC(dec6b720) SHA1(331776e1cba3fb82071e7c2195dc4ae27b3613a2) ) /* ROM on a SIMM */
	ROM_LOAD64_BYTE( "mpn-simm.03d",   0x0000005, 0x200000, CRC(f8774c18) SHA1(58e0ea4dd62e39bcfaa3a2be4ef08eb2f0bd3c00) ) /* ROM on a SIMM */
	ROM_LOAD64_BYTE( "mpn-simm.03a",   0x0000006, 0x200000, CRC(c2aea4ec) SHA1(f5e2a815fa802598611efa48e5de97e929155e77) ) /* ROM on a SIMM */
	ROM_LOAD64_BYTE( "mpn-simm.03b",   0x0000007, 0x200000, CRC(84d6dc33) SHA1(f5ababb479facc08c425381570644230c09334e7) ) /* ROM on a SIMM */

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "mpn.01",   0x00000, 0x08000, CRC(90c7adb6) SHA1(a2653e977e5e0457b249e098e5ca0abc93dac336) )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "mpn-simm.05a",   0x000000, 0x200000, CRC(318a2e21) SHA1(c573cd88f8279a062c73ef1d79cd8421dbdcd93e) ) /* ROM on a SIMM */
	ROM_LOAD16_WORD_SWAP( "mpn-simm.05b",   0x200000, 0x200000, CRC(5462f4e8) SHA1(299fbdab700e735e6395c5d9e3f079bb2e3dbd73) ) /* ROM on a SIMM */

	ROM_REGION( 0x20, REGION_USER5, 0 )
	ROM_LOAD( "mpang.key",    0x000000, 0x000014, CRC(95354b0f) SHA1(6e015eda29ac94aa420d7c70cdebffee52fde032) )
ROM_END

ROM_START( mpanga )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "mpn_03a.6a", 0x000000, 0x80000, CRC(b05ebc34) SHA1(b30784dcc1aaf0f37930c125331dfa41240c5859) )
	ROM_LOAD16_WORD_SWAP( "mpn_04a.7a", 0x080000, 0x80000, CRC(8e4814b4) SHA1(f86e0b7986de115b76c579e6ffd4e77da2360a01) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROM_LOAD64_BYTE( "mpn-simm.01c",   0x0000000, 0x200000, CRC(388db66b) SHA1(7416cce3d0dbea71c92ea9f72f5536146f757b45) ) /* ROM on a SIMM */
	ROM_LOAD64_BYTE( "mpn-simm.01d",   0x0000001, 0x200000, CRC(aff1b494) SHA1(d376c02ce01e71a7707d3d3fe5b0ae59ce781686) ) /* ROM on a SIMM */
	ROM_LOAD64_BYTE( "mpn-simm.01a",   0x0000002, 0x200000, CRC(a9c4857b) SHA1(66f538105c710d1480141e48a15b1a760f5ce985) ) /* ROM on a SIMM */
	ROM_LOAD64_BYTE( "mpn-simm.01b",   0x0000003, 0x200000, CRC(f759df22) SHA1(1678e3e819dd808f3a6fdd52b7c933eac4777b5b) ) /* ROM on a SIMM */
	ROM_LOAD64_BYTE( "mpn-simm.03c",   0x0000004, 0x200000, CRC(dec6b720) SHA1(331776e1cba3fb82071e7c2195dc4ae27b3613a2) ) /* ROM on a SIMM */
	ROM_LOAD64_BYTE( "mpn-simm.03d",   0x0000005, 0x200000, CRC(f8774c18) SHA1(58e0ea4dd62e39bcfaa3a2be4ef08eb2f0bd3c00) ) /* ROM on a SIMM */
	ROM_LOAD64_BYTE( "mpn-simm.03a",   0x0000006, 0x200000, CRC(c2aea4ec) SHA1(f5e2a815fa802598611efa48e5de97e929155e77) ) /* ROM on a SIMM */
	ROM_LOAD64_BYTE( "mpn-simm.03b",   0x0000007, 0x200000, CRC(84d6dc33) SHA1(f5ababb479facc08c425381570644230c09334e7) ) /* ROM on a SIMM */

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "mpn.01",   0x00000, 0x08000, CRC(90c7adb6) SHA1(a2653e977e5e0457b249e098e5ca0abc93dac336) )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "mpn-simm.05a",   0x000000, 0x200000, CRC(318a2e21) SHA1(c573cd88f8279a062c73ef1d79cd8421dbdcd93e) ) /* ROM on a SIMM */
	ROM_LOAD16_WORD_SWAP( "mpn-simm.05b",   0x200000, 0x200000, CRC(5462f4e8) SHA1(299fbdab700e735e6395c5d9e3f079bb2e3dbd73) ) /* ROM on a SIMM */

	ROM_REGION( 0x20, REGION_USER5, 0 )
	ROM_LOAD( "mpang.key",    0x000000, 0x000014, CRC(95354b0f) SHA1(6e015eda29ac94aa420d7c70cdebffee52fde032) )
ROM_END

ROM_START( progear )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "pgau.03", 0x000000, 0x80000, CRC(343a783e) SHA1(7ba8ae041b062767bf64328adf22ef100c38cdfd) )
	ROM_LOAD16_WORD_SWAP( "pgau.04", 0x080000, 0x80000, CRC(16208d79) SHA1(c477de7f31df44144a60d10dc4d933f3a7c20722) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "pgaux.03", 0x000000, 0x80000, CRC(fd58616b) SHA1(af88523ae53d00ebf8664a3be121e0e91cd79aaf) )
	ROM_LOAD16_WORD_SWAP( "pgaux.04", 0x080000, 0x80000, CRC(6081a891) SHA1(4d12636317337f9dfc606fc519b7dc7fee4d89fc) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROMX_LOAD( "pga-simm.01c",   0x0000000, 0x200000,  CRC(452f98b0) SHA1(a10e615c32098f6d25becd466da8faa967523a7b) , ROM_GROUPBYTE | ROM_SKIP(7) ) /* ROM on a simm*/
	ROMX_LOAD( "pga-simm.01d",   0x0000001, 0x200000,  CRC(9e672092) SHA1(fce0b8b43a1c069262f4e3e81c1a04621e232c88) , ROM_GROUPBYTE | ROM_SKIP(7) ) /* ROM on a simm*/
	ROMX_LOAD( "pga-simm.01a",   0x0000002, 0x200000,  CRC(ae9ddafe) SHA1(afbb26fed6cd0cb5c0099a10d35aeb453318c14d) , ROM_GROUPBYTE | ROM_SKIP(7) ) /* ROM on a simm*/
	ROMX_LOAD( "pga-simm.01b",   0x0000003, 0x200000,  CRC(94d72d94) SHA1(df6a3fe49c008f73b160eb6f2a44dc371ff73cba) , ROM_GROUPBYTE | ROM_SKIP(7) ) /* ROM on a simm*/
	ROMX_LOAD( "pga-simm.03c",   0x0000004, 0x200000,  CRC(48a1886d) SHA1(ebf44b42d784924e08a832a7e5f66a887bab244b) , ROM_GROUPBYTE | ROM_SKIP(7) ) /* ROM on a simm*/
	ROMX_LOAD( "pga-simm.03d",   0x0000005, 0x200000,  CRC(172d7e37) SHA1(0eaedd24cd3fa87b6f35fbd63078d40c493c92d0) , ROM_GROUPBYTE | ROM_SKIP(7) ) /* ROM on a simm*/
	ROMX_LOAD( "pga-simm.03a",   0x0000006, 0x200000,  CRC(9ee33d98) SHA1(85d1bd31940e35ac8c732165020881a2d65cd6b1) , ROM_GROUPBYTE | ROM_SKIP(7) ) /* ROM on a simm*/
	ROMX_LOAD( "pga-simm.03b",   0x0000007, 0x200000,  CRC(848dee32) SHA1(c591288e86ad1624d0fe66563808af9fac786e64) , ROM_GROUPBYTE | ROM_SKIP(7) ) /* ROM on a simm*/

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "pga.01",   0x00000, 0x08000, CRC(bdbfa992) SHA1(7c5496c1daaea6a7ab95c0b25625d325ec3427cc) )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "pga-simm.05a",   0x000000, 0x200000, CRC(c0aac80c) SHA1(91784d35d4f7e113529bb5be6081b67094b150ea) ) /* ROM on a simm*/
	ROM_LOAD16_WORD_SWAP( "pga-simm.05b",   0x200000, 0x200000, CRC(37a65d86) SHA1(374d562a4648734f82aa2ddb6d258e870896dd45) ) /* ROM on a simm*/
	ROM_LOAD16_WORD_SWAP( "pga-simm.06a",   0x400000, 0x200000, CRC(d3f1e934) SHA1(5dcea28c873d0d472f5b94e07d97cd77ace2b252) ) /* ROM on a simm*/
	ROM_LOAD16_WORD_SWAP( "pga-simm.06b",   0x600000, 0x200000, CRC(8b39489a) SHA1(fd790efaf37dc2c4c16f657941044e3e2d3c2711) ) /* ROM on a simm*/
ROM_END

ROM_START( progearj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "pgaj.03", 0x000000, 0x80000, CRC(06dbba54) SHA1(b0b808e9974c727bd187f2cdcba71a301b78c759) )
	ROM_LOAD16_WORD_SWAP( "pgaj.04", 0x080000, 0x80000, CRC(a1f1f1bc) SHA1(839cdc89d9483632883c185951c76deb4ff7657e) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "pgajx.03", 0x000000, 0x80000, CRC(b44e4bcf) SHA1(d6228ed4aeeb6e37543fe220487a298308daaa5d) )
	ROM_LOAD16_WORD_SWAP( "pgajx.04", 0x080000, 0x80000, CRC(354db29d) SHA1(fd625951bee5c9b5b01326245793bf4ccd1027ba) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROMX_LOAD( "pga-simm.01c",   0x0000000, 0x200000,  CRC(452f98b0) SHA1(a10e615c32098f6d25becd466da8faa967523a7b) , ROM_GROUPBYTE | ROM_SKIP(7) ) /* ROM on a simm*/
	ROMX_LOAD( "pga-simm.01d",   0x0000001, 0x200000,  CRC(9e672092) SHA1(fce0b8b43a1c069262f4e3e81c1a04621e232c88) , ROM_GROUPBYTE | ROM_SKIP(7) ) /* ROM on a simm*/
	ROMX_LOAD( "pga-simm.01a",   0x0000002, 0x200000,  CRC(ae9ddafe) SHA1(afbb26fed6cd0cb5c0099a10d35aeb453318c14d) , ROM_GROUPBYTE | ROM_SKIP(7) ) /* ROM on a simm*/
	ROMX_LOAD( "pga-simm.01b",   0x0000003, 0x200000,  CRC(94d72d94) SHA1(df6a3fe49c008f73b160eb6f2a44dc371ff73cba) , ROM_GROUPBYTE | ROM_SKIP(7) ) /* ROM on a simm*/
	ROMX_LOAD( "pga-simm.03c",   0x0000004, 0x200000,  CRC(48a1886d) SHA1(ebf44b42d784924e08a832a7e5f66a887bab244b) , ROM_GROUPBYTE | ROM_SKIP(7) ) /* ROM on a simm*/
	ROMX_LOAD( "pga-simm.03d",   0x0000005, 0x200000,  CRC(172d7e37) SHA1(0eaedd24cd3fa87b6f35fbd63078d40c493c92d0) , ROM_GROUPBYTE | ROM_SKIP(7) ) /* ROM on a simm*/
	ROMX_LOAD( "pga-simm.03a",   0x0000006, 0x200000,  CRC(9ee33d98) SHA1(85d1bd31940e35ac8c732165020881a2d65cd6b1) , ROM_GROUPBYTE | ROM_SKIP(7) ) /* ROM on a simm*/
	ROMX_LOAD( "pga-simm.03b",   0x0000007, 0x200000,  CRC(848dee32) SHA1(c591288e86ad1624d0fe66563808af9fac786e64) , ROM_GROUPBYTE | ROM_SKIP(7) ) /* ROM on a simm*/

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "pga.01",   0x00000, 0x08000, CRC(bdbfa992) SHA1(7c5496c1daaea6a7ab95c0b25625d325ec3427cc) )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "pga-simm.05a",   0x000000, 0x200000, CRC(c0aac80c) SHA1(91784d35d4f7e113529bb5be6081b67094b150ea) ) /* ROM on a simm*/
	ROM_LOAD16_WORD_SWAP( "pga-simm.05b",   0x200000, 0x200000, CRC(37a65d86) SHA1(374d562a4648734f82aa2ddb6d258e870896dd45) ) /* ROM on a simm*/
	ROM_LOAD16_WORD_SWAP( "pga-simm.06a",   0x400000, 0x200000, CRC(d3f1e934) SHA1(5dcea28c873d0d472f5b94e07d97cd77ace2b252) ) /* ROM on a simm*/
	ROM_LOAD16_WORD_SWAP( "pga-simm.06b",   0x600000, 0x200000, CRC(8b39489a) SHA1(fd790efaf37dc2c4c16f657941044e3e2d3c2711) ) /* ROM on a simm*/
ROM_END

ROM_START( pzloop2 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "pl2e.03", 0x000000, 0x80000, CRC(3b1285b2) SHA1(f90f98fb15068306a57109ad954845be0a99e8ab) )
	ROM_LOAD16_WORD_SWAP( "pl2e.04", 0x080000, 0x80000, CRC(40a2d647) SHA1(d1c5fa87b368efe0d2cc3f614d0165bd95748b81) )
	ROM_LOAD16_WORD_SWAP( "pl2e.05", 0x100000, 0x80000, CRC(0f11d818) SHA1(ca2d5ea892aebfa1a2a825fb45c57b7923936917) )
	ROM_LOAD16_WORD_SWAP( "pl2e.06", 0x180000, 0x80000, CRC(86fbbdf4) SHA1(325ba5dc54f3d82f39e1299d5f27de7227a458b2) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROM_LOAD64_BYTE( "pl2-simm.01c",   0x0000000, 0x200000, CRC(137b13a7) SHA1(a1ca1bc8699ddfc54d5de1b39a9db9a5ac8b12e5) ) /* ROM on a SIMM */
	ROM_LOAD64_BYTE( "pl2-simm.01d",   0x0000001, 0x200000, CRC(a2db1507) SHA1(61c84c8d698a846d54a571b5f7b4824e22136db7) ) /* ROM on a SIMM */
	ROM_LOAD64_BYTE( "pl2-simm.01a",   0x0000002, 0x200000, CRC(7e80ff8e) SHA1(afcebfa995ace8b8973e75f1589980c5c4535bca) ) /* ROM on a SIMM */
	ROM_LOAD64_BYTE( "pl2-simm.01b",   0x0000003, 0x200000, CRC(cd93e6ed) SHA1(e4afce48fe481d8291ed2475d5de446afad65351) ) /* ROM on a SIMM */
	ROM_LOAD64_BYTE( "pl2-simm.03c",   0x0000004, 0x200000, CRC(0f52bbca) SHA1(e76c29d445062f5e16d06bdc4ab44640ba35aaac) ) /* ROM on a SIMM */
	ROM_LOAD64_BYTE( "pl2-simm.03d",   0x0000005, 0x200000, CRC(a62712c3) SHA1(2abfe0209e188010a0ae969f0d9eb7d28820b3f2) ) /* ROM on a SIMM */
	ROM_LOAD64_BYTE( "pl2-simm.03a",   0x0000006, 0x200000, CRC(b60c9f8e) SHA1(40c7985e04463fb2bd59b3bb6aa5897328d37ff3) ) /* ROM on a SIMM */
	ROM_LOAD64_BYTE( "pl2-simm.03b",   0x0000007, 0x200000, CRC(83fef284) SHA1(ef4429f54c456d6485a7d642d49dffafef4435fe) ) /* ROM on a SIMM */

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "pl2.01",   0x00000, 0x08000, CRC(35697569) SHA1(13718923cffb9ec53cef9e22d8875370b5f3dd74) )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "pl2-simm.05a",   0x000000, 0x200000, CRC(85d8fbe8) SHA1(c19d5e9084d07e610379b6e1b6be7bdf0b9b7f7f) ) /* ROM on a SIMM */
	ROM_LOAD16_WORD_SWAP( "pl2-simm.05b",   0x200000, 0x200000, CRC(1ed62584) SHA1(28411f610f48cca6424a2d53e2a4ac691e826317) ) /* ROM on a SIMM */

	ROM_REGION( 0x20, REGION_USER5, 0 )
	ROM_LOAD( "pzloop2.key",  0x000000, 0x000014, CRC(ae13be78) SHA1(5c715f0ef1e0664027faa6c2a7f0f878462cb7ae) )
ROM_END

ROM_START( pzloop2j )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "pl2j.03c", 0x000000, 0x80000, CRC(3b76b806) SHA1(743808ccd470da075ca2f4a8872b22feb7e993ef) ) /* 'c' handwritten over 'a' label */
	ROM_LOAD16_WORD_SWAP( "pl2j.04c", 0x080000, 0x80000, CRC(8878a42a) SHA1(178efe9be9ab7f209489f35d653cb5fef4e9565b) ) /* ^^ */
	ROM_LOAD16_WORD_SWAP( "pl2j.05c", 0x100000, 0x80000, CRC(51081ea4) SHA1(175dc70ba0b6a158d23e7271dd65f28e7beea746) ) /* ^^ */
	ROM_LOAD16_WORD_SWAP( "pl2j.06c", 0x180000, 0x80000, CRC(51c68494) SHA1(c3b2055af11138f5280fe89e248536069a22ddca) ) /* ^^ */

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROM_LOAD64_BYTE( "pl2-simm.01c",   0x0000000, 0x200000, CRC(137b13a7) SHA1(a1ca1bc8699ddfc54d5de1b39a9db9a5ac8b12e5) ) /* ROM on a SIMM */
	ROM_LOAD64_BYTE( "pl2-simm.01d",   0x0000001, 0x200000, CRC(a2db1507) SHA1(61c84c8d698a846d54a571b5f7b4824e22136db7) ) /* ROM on a SIMM */
	ROM_LOAD64_BYTE( "pl2-simm.01a",   0x0000002, 0x200000, CRC(7e80ff8e) SHA1(afcebfa995ace8b8973e75f1589980c5c4535bca) ) /* ROM on a SIMM */
	ROM_LOAD64_BYTE( "pl2-simm.01b",   0x0000003, 0x200000, CRC(cd93e6ed) SHA1(e4afce48fe481d8291ed2475d5de446afad65351) ) /* ROM on a SIMM */
	ROM_LOAD64_BYTE( "pl2-simm.03c",   0x0000004, 0x200000, CRC(0f52bbca) SHA1(e76c29d445062f5e16d06bdc4ab44640ba35aaac) ) /* ROM on a SIMM */
	ROM_LOAD64_BYTE( "pl2-simm.03d",   0x0000005, 0x200000, CRC(a62712c3) SHA1(2abfe0209e188010a0ae969f0d9eb7d28820b3f2) ) /* ROM on a SIMM */
	ROM_LOAD64_BYTE( "pl2-simm.03a",   0x0000006, 0x200000, CRC(b60c9f8e) SHA1(40c7985e04463fb2bd59b3bb6aa5897328d37ff3) ) /* ROM on a SIMM */
	ROM_LOAD64_BYTE( "pl2-simm.03b",   0x0000007, 0x200000, CRC(83fef284) SHA1(ef4429f54c456d6485a7d642d49dffafef4435fe) ) /* ROM on a SIMM */

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "pl2.01",   0x00000, 0x08000, CRC(35697569) SHA1(13718923cffb9ec53cef9e22d8875370b5f3dd74) )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "pl2-simm.05a",   0x000000, 0x200000, CRC(85d8fbe8) SHA1(c19d5e9084d07e610379b6e1b6be7bdf0b9b7f7f) ) /* ROM on a SIMM */
	ROM_LOAD16_WORD_SWAP( "pl2-simm.05b",   0x200000, 0x200000, CRC(1ed62584) SHA1(28411f610f48cca6424a2d53e2a4ac691e826317) ) /* ROM on a SIMM */

	ROM_REGION( 0x20, REGION_USER5, 0 )
	ROM_LOAD( "pzloop2.key",  0x000000, 0x000014, CRC(ae13be78) SHA1(5c715f0ef1e0664027faa6c2a7f0f878462cb7ae) )
ROM_END

ROM_START( pzloop2jr1 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "pl2j.03a", 0x000000, 0x80000, CRC(0a751bd0) SHA1(a5a0b60387aacdafdf46ecd1acd764c9cb086b90) )
	ROM_LOAD16_WORD_SWAP( "pl2j.04a", 0x080000, 0x80000, CRC(c3f72afe) SHA1(597a302e4bba50193c53f239e715962fcc4e3e5e) )
	ROM_LOAD16_WORD_SWAP( "pl2j.05a", 0x100000, 0x80000, CRC(6ea9dbfc) SHA1(c3065e02516755e8b94a741dd2ab960c96d0ff8c) )
	ROM_LOAD16_WORD_SWAP( "pl2j.06a", 0x180000, 0x80000, CRC(0f14848d) SHA1(94a3ee00d65cd9a310b3a330e2c37467b5863c64) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROM_LOAD64_BYTE( "pl2-simm.01c",   0x0000000, 0x200000, CRC(137b13a7) SHA1(a1ca1bc8699ddfc54d5de1b39a9db9a5ac8b12e5) ) /* ROM on a SIMM */
	ROM_LOAD64_BYTE( "pl2-simm.01d",   0x0000001, 0x200000, CRC(a2db1507) SHA1(61c84c8d698a846d54a571b5f7b4824e22136db7) ) /* ROM on a SIMM */
	ROM_LOAD64_BYTE( "pl2-simm.01a",   0x0000002, 0x200000, CRC(7e80ff8e) SHA1(afcebfa995ace8b8973e75f1589980c5c4535bca) ) /* ROM on a SIMM */
	ROM_LOAD64_BYTE( "pl2-simm.01b",   0x0000003, 0x200000, CRC(cd93e6ed) SHA1(e4afce48fe481d8291ed2475d5de446afad65351) ) /* ROM on a SIMM */
	ROM_LOAD64_BYTE( "pl2-simm.03c",   0x0000004, 0x200000, CRC(0f52bbca) SHA1(e76c29d445062f5e16d06bdc4ab44640ba35aaac) ) /* ROM on a SIMM */
	ROM_LOAD64_BYTE( "pl2-simm.03d",   0x0000005, 0x200000, CRC(a62712c3) SHA1(2abfe0209e188010a0ae969f0d9eb7d28820b3f2) ) /* ROM on a SIMM */
	ROM_LOAD64_BYTE( "pl2-simm.03a",   0x0000006, 0x200000, CRC(b60c9f8e) SHA1(40c7985e04463fb2bd59b3bb6aa5897328d37ff3) ) /* ROM on a SIMM */
	ROM_LOAD64_BYTE( "pl2-simm.03b",   0x0000007, 0x200000, CRC(83fef284) SHA1(ef4429f54c456d6485a7d642d49dffafef4435fe) ) /* ROM on a SIMM */

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "pl2.01",   0x00000, 0x08000, CRC(35697569) SHA1(13718923cffb9ec53cef9e22d8875370b5f3dd74) )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "pl2-simm.05a",   0x000000, 0x200000, CRC(85d8fbe8) SHA1(c19d5e9084d07e610379b6e1b6be7bdf0b9b7f7f) ) /* ROM on a SIMM */
	ROM_LOAD16_WORD_SWAP( "pl2-simm.05b",   0x200000, 0x200000, CRC(1ed62584) SHA1(28411f610f48cca6424a2d53e2a4ac691e826317) ) /* ROM on a SIMM */

	ROM_REGION( 0x20, REGION_USER5, 0 )
	ROM_LOAD( "pzloop2.key",  0x000000, 0x000014, CRC(ae13be78) SHA1(5c715f0ef1e0664027faa6c2a7f0f878462cb7ae) )
ROM_END

ROM_START( nwarr )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "vphu.03f", 0x000000, 0x80000, CRC(85d6a359) SHA1(38f4bb5cf2e1e82ce9673b911329fdc8220ce0dc) )
	ROM_LOAD16_WORD_SWAP( "vphu.04c", 0x080000, 0x80000, CRC(cb7fce77) SHA1(85a8c8b1c71df0eee5f23e0bf28b2d95af2ce830) )
	ROM_LOAD16_WORD_SWAP( "vphu.05e", 0x100000, 0x80000, CRC(e08f2bba) SHA1(0f6d01a3b05085df23ead4c09c5363a1587b527e) )
	ROM_LOAD16_WORD_SWAP( "vphu.06c", 0x180000, 0x80000, CRC(08c04cdb) SHA1(b78d87631a13c26cc1580d2ecc0d137105c23f0a) )
	ROM_LOAD16_WORD_SWAP( "vphu.07b", 0x200000, 0x80000, CRC(b5a5ab19) SHA1(f7b35b8cba81f88a6bdfea7e2dc12eca480c276c) )
	ROM_LOAD16_WORD_SWAP( "vphu.08b", 0x280000, 0x80000, CRC(51bb20fb) SHA1(a98c569dd45b4bd2275f9bd1df060d6eaead53df) )
	ROM_LOAD16_WORD_SWAP( "vphu.09b", 0x300000, 0x80000, CRC(41a64205) SHA1(1f5af658b7c3fb09cab3dd10d6dc433a0605f81a) )
	ROM_LOAD16_WORD_SWAP( "vphu.10b", 0x380000, 0x80000, CRC(2b1d43ae) SHA1(fa9a456fe92783c7cb93ca231b24387cf56644d7) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "vphux.03f", 0x000000, 0x80000, CRC(d1c35094) SHA1(fd16aea7e6aa52dc94421f48efa79af10ba55ede) )
	ROM_LOAD16_WORD_SWAP( "vphux.04c", 0x080000, 0x80000, CRC(48b01f4e) SHA1(5849022b600cf702b09a4cfb13763552635ca992) )
	ROM_LOAD16_WORD_SWAP( "vphux.05e", 0x100000, 0x80000, CRC(0147c2a5) SHA1(47248709d9181b74aa0b915d4f451f71e897dec9) )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "vph.13",   0x0000000, 0x400000, CRC(c51baf99) SHA1(2fb6642908e542e404391eb17392f8270e87bf48) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vph.15",   0x0000002, 0x400000, CRC(3ce83c77) SHA1(93369b23c6d7d834297434691bb047ee3dd9539c) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vph.17",   0x0000004, 0x400000, CRC(4f2408e0) SHA1(cd49c6b3c7e6470c6058f98ccc5210b052bb13e2) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vph.19",   0x0000006, 0x400000, CRC(9ff60250) SHA1(d69ba4dc6bd37d003245f0cf3211d6e2623005b8) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vph.14",   0x1000000, 0x400000, CRC(7a0e1add) SHA1(6b28a91bd59bba97886fdea30116a5b1071109ed) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vph.16",   0x1000002, 0x400000, CRC(2f41ca75) SHA1(f4a67e60b62001e6fe75cb05b9c81040a8a09f54) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vph.18",   0x1000004, 0x400000, CRC(64498eed) SHA1(d64e54a9ad1cbb927b7bac2eb16e1487834c5706) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vph.20",   0x1000006, 0x400000, CRC(17f2433f) SHA1(0cbf8c96f92016fefb4a9c668ce5fd260342d712) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "vph.01",   0x00000, 0x08000, CRC(5045dcac) SHA1(fd1a6586fbdd48a707df1fa52309b4cf50e3cc4c) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "vph.02",   0x28000, 0x20000, CRC(86b60e59) SHA1(197d07ced8b9850729c83fa59b7afc283500bdee) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "vph.11",   0x000000, 0x200000, CRC(e1837d33) SHA1(e3cb69f64767bacbec7286d0b4cd0ce7a0ba13d8) )
	ROM_LOAD16_WORD_SWAP( "vph.12",   0x200000, 0x200000, CRC(fbd3cd90) SHA1(4813c25802ad71b77ca04fd8f3a86344f99f0d6a) )
ROM_END

ROM_START( nwarrh )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "vphh.03d", 0x000000, 0x80000, CRC(6029c7be) SHA1(687c382d02d18ef5781c9d928e74f161461c2641) )
	ROM_LOAD16_WORD_SWAP( "vphh.04a", 0x080000, 0x80000, CRC(d26625ee) SHA1(2b415a28ee39949a1e80e7e65b89f3d707bdfae7) )
	ROM_LOAD16_WORD_SWAP( "vphh.05c", 0x100000, 0x80000, CRC(73ee0b8a) SHA1(d31ccb2cbc6133972f6dc76f0a40154368953625) )
	ROM_LOAD16_WORD_SWAP( "vphh.06a", 0x180000, 0x80000, CRC(a5b3a50a) SHA1(de382ab707eeb4ec7ffbc637611296ee35acdce1) )
	ROM_LOAD16_WORD_SWAP( "vphh.07",  0x200000, 0x80000, CRC(5fc2bdc1) SHA1(5936f2d3eb6becefa3ede98107eb03723555cc22) )
	ROM_LOAD16_WORD_SWAP( "vphh.08",  0x280000, 0x80000, CRC(e65588d9) SHA1(4b15009d5aa2d91736af1ee7c52d6b49cc696724) )
	ROM_LOAD16_WORD_SWAP( "vphh.09",  0x300000, 0x80000, CRC(a2ce6d63) SHA1(52aed61d0c7a6191016f1ec4b0a4372fbf55bf49) )
	ROM_LOAD16_WORD_SWAP( "vphh.10",  0x380000, 0x80000, CRC(e2f4f4b9) SHA1(8d3e857ccd4654d2801ce6830c0d556a81c4d433) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
/*	ROM_LOAD16_WORD_SWAP( "vphhx.03d", 0x000000, 0x80000, NO_DUMP )*/
/*	ROM_LOAD16_WORD_SWAP( "vphhx.04a", 0x080000, 0x80000, NO_DUMP )*/
/*	ROM_LOAD16_WORD_SWAP( "vphhx.05c", 0x100000, 0x80000, NO_DUMP )*/

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "vph.13",   0x0000000, 0x400000, CRC(c51baf99) SHA1(2fb6642908e542e404391eb17392f8270e87bf48) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vph.15",   0x0000002, 0x400000, CRC(3ce83c77) SHA1(93369b23c6d7d834297434691bb047ee3dd9539c) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vph.17",   0x0000004, 0x400000, CRC(4f2408e0) SHA1(cd49c6b3c7e6470c6058f98ccc5210b052bb13e2) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vph.19",   0x0000006, 0x400000, CRC(9ff60250) SHA1(d69ba4dc6bd37d003245f0cf3211d6e2623005b8) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vph.14",   0x1000000, 0x400000, CRC(7a0e1add) SHA1(6b28a91bd59bba97886fdea30116a5b1071109ed) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vph.16",   0x1000002, 0x400000, CRC(2f41ca75) SHA1(f4a67e60b62001e6fe75cb05b9c81040a8a09f54) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vph.18",   0x1000004, 0x400000, CRC(64498eed) SHA1(d64e54a9ad1cbb927b7bac2eb16e1487834c5706) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vph.20",   0x1000006, 0x400000, CRC(17f2433f) SHA1(0cbf8c96f92016fefb4a9c668ce5fd260342d712) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "vph.01",   0x00000, 0x08000, CRC(5045dcac) SHA1(fd1a6586fbdd48a707df1fa52309b4cf50e3cc4c) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "vph.02",   0x28000, 0x20000, CRC(86b60e59) SHA1(197d07ced8b9850729c83fa59b7afc283500bdee) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "vph.11",   0x000000, 0x200000, CRC(e1837d33) SHA1(e3cb69f64767bacbec7286d0b4cd0ce7a0ba13d8) )
	ROM_LOAD16_WORD_SWAP( "vph.12",   0x200000, 0x200000, CRC(fbd3cd90) SHA1(4813c25802ad71b77ca04fd8f3a86344f99f0d6a) )
ROM_END

ROM_START( nwarrb )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "vphb.03d", 0x000000, 0x80000, CRC(3a426d3f) SHA1(76d7c39c901aa768bb1600179509752d1fc0d558) )
	ROM_LOAD16_WORD_SWAP( "vphb.04a", 0x080000, 0x80000, CRC(51c4bb2f) SHA1(c885813ff13bfd251accf38da1bc0bd9c526e4c5) )
	ROM_LOAD16_WORD_SWAP( "vphb.05c", 0x100000, 0x80000, CRC(ac44d997) SHA1(b28e55d83a33e885125f2def76259d0ab21b0f9b) )
	ROM_LOAD16_WORD_SWAP( "vphb.06a", 0x180000, 0x80000, CRC(5072a5fe) SHA1(78b3f2ef8bc16441d0d977dbec2246c9f9b28dbc) )
	ROM_LOAD16_WORD_SWAP( "vphb.07",  0x200000, 0x80000, CRC(9b355192) SHA1(10b5542fcc0af936868af9abf70d3303be543f21) )
	ROM_LOAD16_WORD_SWAP( "vphb.08",  0x280000, 0x80000, CRC(42220f84) SHA1(f6ef52b1dff86c25852aa05be4a5b39995c26dd7) )
	ROM_LOAD16_WORD_SWAP( "vphb.09",  0x300000, 0x80000, CRC(029e015d) SHA1(441d0ea36484cbffe783cb0a1133537c09783022) )
	ROM_LOAD16_WORD_SWAP( "vphb.10",  0x380000, 0x80000, CRC(37b3ce37) SHA1(5919b44415e4d5b242fcdd69efd0ab1722e4da8c) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
/*	ROM_LOAD16_WORD_SWAP( "vphbx.03d", 0x000000, 0x80000, NO_DUMP )*/
/*	ROM_LOAD16_WORD_SWAP( "vphbx.04a", 0x080000, 0x80000, NO_DUMP )*/
/*	ROM_LOAD16_WORD_SWAP( "vphbx.05c", 0x100000, 0x80000, NO_DUMP )*/

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "vph.13",   0x0000000, 0x400000, CRC(c51baf99) SHA1(2fb6642908e542e404391eb17392f8270e87bf48) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vph.15",   0x0000002, 0x400000, CRC(3ce83c77) SHA1(93369b23c6d7d834297434691bb047ee3dd9539c) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vph.17",   0x0000004, 0x400000, CRC(4f2408e0) SHA1(cd49c6b3c7e6470c6058f98ccc5210b052bb13e2) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vph.19",   0x0000006, 0x400000, CRC(9ff60250) SHA1(d69ba4dc6bd37d003245f0cf3211d6e2623005b8) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vph.14",   0x1000000, 0x400000, CRC(7a0e1add) SHA1(6b28a91bd59bba97886fdea30116a5b1071109ed) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vph.16",   0x1000002, 0x400000, CRC(2f41ca75) SHA1(f4a67e60b62001e6fe75cb05b9c81040a8a09f54) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vph.18",   0x1000004, 0x400000, CRC(64498eed) SHA1(d64e54a9ad1cbb927b7bac2eb16e1487834c5706) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vph.20",   0x1000006, 0x400000, CRC(17f2433f) SHA1(0cbf8c96f92016fefb4a9c668ce5fd260342d712) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "vph.01",   0x00000, 0x08000, CRC(5045dcac) SHA1(fd1a6586fbdd48a707df1fa52309b4cf50e3cc4c) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "vph.02",   0x28000, 0x20000, CRC(86b60e59) SHA1(197d07ced8b9850729c83fa59b7afc283500bdee) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "vph.11",   0x000000, 0x200000, CRC(e1837d33) SHA1(e3cb69f64767bacbec7286d0b4cd0ce7a0ba13d8) )
	ROM_LOAD16_WORD_SWAP( "vph.12",   0x200000, 0x200000, CRC(fbd3cd90) SHA1(4813c25802ad71b77ca04fd8f3a86344f99f0d6a) )
ROM_END

ROM_START( vhuntj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "vphj.03f", 0x000000, 0x80000, CRC(3de2e333) SHA1(2f9f756c5c91646625d70debd5b19b8dbd13a62f) )
	ROM_LOAD16_WORD_SWAP( "vphj.04c", 0x080000, 0x80000, CRC(c95cf304) SHA1(0544ab9d0f398b558e1119d94885058ad4a7d929) )
	ROM_LOAD16_WORD_SWAP( "vphj.05d", 0x100000, 0x80000, CRC(50de5ddd) SHA1(2bcc6c254ead06e9ea0a9ae4348195d3d55de277) )
	ROM_LOAD16_WORD_SWAP( "vphj.06c", 0x180000, 0x80000, CRC(ac3bd3d5) SHA1(c0aa04c43dba2876d97d95fffd4766a28193b300) )
	ROM_LOAD16_WORD_SWAP( "vphj.07b", 0x200000, 0x80000, CRC(0761309f) SHA1(7c6f9ec4d93ea9dbd634142558baaaf170cd4c76) )
	ROM_LOAD16_WORD_SWAP( "vphj.08b", 0x280000, 0x80000, CRC(5a5c2bf5) SHA1(296c6a5a0062b58bc71a297bc8b27eea099c8518) )
	ROM_LOAD16_WORD_SWAP( "vphj.09b", 0x300000, 0x80000, CRC(823d6d99) SHA1(17be75b2ebfbf60a2141aef67c386454d23565f2) )
	ROM_LOAD16_WORD_SWAP( "vphj.10b", 0x380000, 0x80000, CRC(32c7d8f0) SHA1(47075fa80ceff6adfa6cc58dbe32ed4ee01ba4fc) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "vphjx.03f", 0x000000, 0x80000, CRC(dde66666) SHA1(9d030cb5e3eb12274fda59b42356ed58b4b9db2c) )
	ROM_LOAD16_WORD_SWAP( "vphjx.04c", 0x080000, 0x80000, CRC(2dfed55f) SHA1(e60c7321ccf2c4182ae64369def196884d190043) )
	ROM_LOAD16_WORD_SWAP( "vphjx.05d", 0x100000, 0x80000, CRC(56c18710) SHA1(91b3ddc2c32d01c2b313ef10ac19abe6c2d8d2fa) )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "vph.13",   0x0000000, 0x400000, CRC(c51baf99) SHA1(2fb6642908e542e404391eb17392f8270e87bf48) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vph.15",   0x0000002, 0x400000, CRC(3ce83c77) SHA1(93369b23c6d7d834297434691bb047ee3dd9539c) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vph.17",   0x0000004, 0x400000, CRC(4f2408e0) SHA1(cd49c6b3c7e6470c6058f98ccc5210b052bb13e2) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vph.19",   0x0000006, 0x400000, CRC(9ff60250) SHA1(d69ba4dc6bd37d003245f0cf3211d6e2623005b8) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vph.14",   0x1000000, 0x400000, CRC(7a0e1add) SHA1(6b28a91bd59bba97886fdea30116a5b1071109ed) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vph.16",   0x1000002, 0x400000, CRC(2f41ca75) SHA1(f4a67e60b62001e6fe75cb05b9c81040a8a09f54) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vph.18",   0x1000004, 0x400000, CRC(64498eed) SHA1(d64e54a9ad1cbb927b7bac2eb16e1487834c5706) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vph.20",   0x1000006, 0x400000, CRC(17f2433f) SHA1(0cbf8c96f92016fefb4a9c668ce5fd260342d712) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "vph.01",   0x00000, 0x08000, CRC(5045dcac) SHA1(fd1a6586fbdd48a707df1fa52309b4cf50e3cc4c) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "vph.02",   0x28000, 0x20000, CRC(86b60e59) SHA1(197d07ced8b9850729c83fa59b7afc283500bdee) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "vph.11",   0x000000, 0x200000, CRC(e1837d33) SHA1(e3cb69f64767bacbec7286d0b4cd0ce7a0ba13d8) )
	ROM_LOAD16_WORD_SWAP( "vph.12",   0x200000, 0x200000, CRC(fbd3cd90) SHA1(4813c25802ad71b77ca04fd8f3a86344f99f0d6a) )
ROM_END

ROM_START( vhuntjr1 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "vphj.03b", 0x000000, 0x80000, CRC(679c3fa9) SHA1(25c3f595e4d93c16ac483e4f9ba20ad714ecf4ef) )
	ROM_LOAD16_WORD_SWAP( "vphj.04a", 0x080000, 0x80000, CRC(eb6e71e4) SHA1(7a7cd34f7a70d87b817c0a4242844103db3e9f66) )
	ROM_LOAD16_WORD_SWAP( "vphj.05a", 0x100000, 0x80000, CRC(eaf634ea) SHA1(d46cb9d5172bb626396354ff2742d4394f0816f1) )
	ROM_LOAD16_WORD_SWAP( "vphj.06a", 0x180000, 0x80000, CRC(b70cc6be) SHA1(02fc8070bb75a2075de01b891249e6891687440a) )
	ROM_LOAD16_WORD_SWAP( "vphj.07a", 0x200000, 0x80000, CRC(46ab907d) SHA1(18215ff19e2b0c6505b5b5dfe24ef09fc8539ae5) )
	ROM_LOAD16_WORD_SWAP( "vphj.08a", 0x280000, 0x80000, CRC(1c00355e) SHA1(72b94b6c5a10ecd11169048d991bcb7550968cc9) )
	ROM_LOAD16_WORD_SWAP( "vphj.09a", 0x300000, 0x80000, CRC(026e6f82) SHA1(4dffda5e2bcd2fbc9084782e9a79ebd2be1338e7) )
	ROM_LOAD16_WORD_SWAP( "vphj.10a", 0x380000, 0x80000, CRC(aadfb3ea) SHA1(f42b76a98f657ba67aee69025476e8114acce4c5) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "vphjx.03b", 0x000000, 0x80000, CRC(b3c43690) SHA1(7f8b0b181b9804adef39a7a7251474d828014190) )
	ROM_LOAD16_WORD_SWAP( "vphjx.04a", 0x080000, 0x80000, CRC(22ff1003) SHA1(0b7795af8a06b0cb76575632a919b4cb691c6a5b) )
	ROM_LOAD16_WORD_SWAP( "vphjx.05a", 0x100000, 0x80000, CRC(c35b50cb) SHA1(bed3e74a170cdf979aa216b3347eb205ed05e6a4) )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "vph.13",   0x0000000, 0x400000, CRC(c51baf99) SHA1(2fb6642908e542e404391eb17392f8270e87bf48) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vph.15",   0x0000002, 0x400000, CRC(3ce83c77) SHA1(93369b23c6d7d834297434691bb047ee3dd9539c) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vph.17",   0x0000004, 0x400000, CRC(4f2408e0) SHA1(cd49c6b3c7e6470c6058f98ccc5210b052bb13e2) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vph.19",   0x0000006, 0x400000, CRC(9ff60250) SHA1(d69ba4dc6bd37d003245f0cf3211d6e2623005b8) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vph.14",   0x1000000, 0x400000, CRC(7a0e1add) SHA1(6b28a91bd59bba97886fdea30116a5b1071109ed) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vph.16",   0x1000002, 0x400000, CRC(2f41ca75) SHA1(f4a67e60b62001e6fe75cb05b9c81040a8a09f54) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vph.18",   0x1000004, 0x400000, CRC(64498eed) SHA1(d64e54a9ad1cbb927b7bac2eb16e1487834c5706) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vph.20",   0x1000006, 0x400000, CRC(17f2433f) SHA1(0cbf8c96f92016fefb4a9c668ce5fd260342d712) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "vph.01",   0x00000, 0x08000, CRC(5045dcac) SHA1(fd1a6586fbdd48a707df1fa52309b4cf50e3cc4c) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "vph.02",   0x28000, 0x20000, CRC(86b60e59) SHA1(197d07ced8b9850729c83fa59b7afc283500bdee) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "vph.11",   0x000000, 0x200000, CRC(e1837d33) SHA1(e3cb69f64767bacbec7286d0b4cd0ce7a0ba13d8) )
	ROM_LOAD16_WORD_SWAP( "vph.12",   0x200000, 0x200000, CRC(fbd3cd90) SHA1(4813c25802ad71b77ca04fd8f3a86344f99f0d6a) )
ROM_END

ROM_START( qndream )
	ROM_REGION(CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "tqzj.03a", 0x000000, 0x80000, CRC(7acf3e30) SHA1(5e2a697f98185731afc4130286a2699033dd02af) )
	ROM_LOAD16_WORD_SWAP( "tqzj.04",  0x080000, 0x80000, CRC(f1044a87) SHA1(3fd6e5dd6be8c037c8a77cb840bf7d387497a98b) )
	ROM_LOAD16_WORD_SWAP( "tqzj.05",  0x100000, 0x80000, CRC(4105ba0e) SHA1(73aacdf4176029f8e21506319e41ce03ed480122) )
	ROM_LOAD16_WORD_SWAP( "tqzj.06",  0x180000, 0x80000, CRC(c371e8a5) SHA1(5a93e46e46acfdc93fcb069e2426627e948655bf) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "tqzjx.03a", 0x000000, 0x80000, CRC(5804a8f8) SHA1(79fd1438078578abd3c669c0f8d08a29c1303eb4) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROM_FILL(              0x000000, 0x800000, 0 )
	ROMX_LOAD( "tqz.14",   0x800000, 0x200000, CRC(98af88a2) SHA1(d3620faf2162a1f3a62a238715da4da429376d3c) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "tqz.16",   0x800002, 0x200000, CRC(df82d491) SHA1(fd3c8303cbcacb132a90398ff61f47d2d68157ae) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "tqz.18",   0x800004, 0x200000, CRC(42f132ff) SHA1(0e0a128524010dba033a9b9ab2c56fe92a1767da) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "tqz.20",   0x800006, 0x200000, CRC(b2e128a3) SHA1(8ae3161749d5206f16b755c29466cd5ca249b665) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION(QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "tqz.01",   0x00000, 0x08000, CRC(e9ce9d0a) SHA1(29f2987788e914e0a55f9130a99e411d15a7cc9b) )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "tqz.11",   0x000000, 0x200000, CRC(78e7884f) SHA1(82fbbf704ac4bc0e0e7a6f407686861aa3693c23) )
	ROM_LOAD16_WORD_SWAP( "tqz.12",   0x200000, 0x200000, CRC(2e049b13) SHA1(e026f444b905e679e8240c7dd371658c4a3fd713) )
ROM_END

ROM_START( ringdest )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "smbe.03b", 0x000000, 0x80000, CRC(b8016278) SHA1(f744b08b27c11b8567ca7a94fbd75e398563c008) )
	ROM_LOAD16_WORD_SWAP( "smbe.04b", 0x080000, 0x80000, CRC(18c4c447) SHA1(3723ad6d6939fa1ac7dd016254b32017b5e7b24e) )
	ROM_LOAD16_WORD_SWAP( "smbe.05b", 0x100000, 0x80000, CRC(18ebda7f) SHA1(eacc3e76af5c47abe0a778be7a7beacf0924884e) )
	ROM_LOAD16_WORD_SWAP( "smbe.06b", 0x180000, 0x80000, CRC(89c80007) SHA1(4c85aa5b224fdbb64f719a7b8b5b2e7413107c70) )
	ROM_LOAD16_WORD_SWAP( "smb.07",   0x200000, 0x80000, CRC(b9a11577) SHA1(e9b58ef8acd1fedd3c9e0a3489593c7e931106c0) )
	ROM_LOAD16_WORD_SWAP( "smb.08",   0x280000, 0x80000, CRC(f931b76b) SHA1(0b7e8d1278dcba89f0063bd09cda96d6ae1bc282) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "smbex.03b", 0x000000, 0x80000, CRC(3b1457bd) SHA1(7e5eaaed00c4ed106a685b1f34a3695fed9b014e) )
	ROM_LOAD16_WORD_SWAP( "smbex.04b", 0x080000, 0x80000, CRC(6299eb4e) SHA1(232b09cc1e65b47e11369d0dbe5b0724567269e1) )
	ROM_LOAD16_WORD_SWAP( "smbex.05b", 0x100000, 0x80000, CRC(be4a84d1) SHA1(a958c98140fa52798f78d955cf56e24c7c0b8f21) )

	ROM_REGION( 0x1200000, REGION_GFX1, 0 )
	ROMX_LOAD( "smb.13",   0x0000000, 0x200000, CRC(d9b2d1de) SHA1(e8658983070dadcd1300a680a42c8431579e2b4f) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "smb.15",   0x0000002, 0x200000, CRC(9a766d92) SHA1(afdf88afbec527268d63c11ea32f861b52e11489) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "smb.17",   0x0000004, 0x200000, CRC(51800f0f) SHA1(9526cd69a23340a81841271b51de03d9bf2b979f) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "smb.19",   0x0000006, 0x200000, CRC(35757e96) SHA1(c915f3b9e4fdec3defc7eecb2c1f7377e6072228) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "smb.14",   0x0800000, 0x200000, CRC(e5bfd0e7) SHA1(327e626df4c2152f921f15535c01dda6c4437527) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "smb.16",   0x0800002, 0x200000, CRC(c56c0866) SHA1(1e2218e852ae72a9a95861dd37129fe78d4b1329) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "smb.18",   0x0800004, 0x200000, CRC(4ded3910) SHA1(d883541ce4d83f4e7ab095f2ef273408d9911f9a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "smb.20",   0x0800006, 0x200000, CRC(26ea1ec5) SHA1(22be249b1f73272feacf4026f09fc877f5d86353) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "smb.21",   0x1000000, 0x080000, CRC(0a08c5fc) SHA1(ff3fad4fbc98e3013291c7ba7ee5e057a2628b36) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "smb.23",   0x1000002, 0x080000, CRC(0911b6c4) SHA1(e7a7061b192658724d98cae8693f63dd5bc40c00) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "smb.25",   0x1000004, 0x080000, CRC(82d6c4ec) SHA1(ed8ed02a00f59a048b9891ec2a77720bb6a5e03d) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "smb.27",   0x1000006, 0x080000, CRC(9b48678b) SHA1(4fa300d356c538947983ae85bb5c5cfd1fb835e7) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "smb.01",   0x00000, 0x08000, CRC(0abc229a) SHA1(967f574e6358dfc1b01e6a4a4df1a8f34eb3d814) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "smb.02",   0x28000, 0x20000, CRC(d051679a) SHA1(583c2521a30db1740d95dd94a38751fbeff3aae5) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "smb.11",   0x000000, 0x200000, CRC(c56935f9) SHA1(ca1705e48e31ddc13505e6297bceca2bec1bb209) )
	ROM_LOAD16_WORD_SWAP( "smb.12",   0x200000, 0x200000, CRC(955b0782) SHA1(ee09500e7b44e923126533613bfe26cdabc7ab5f) )
ROM_END

ROM_START( smbomb )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "smbj.03a", 0x000000, 0x80000, CRC(1c5613de) SHA1(e6257078ad2e18537aa606b0d0c5e04806244386) )
	ROM_LOAD16_WORD_SWAP( "smbj.04a", 0x080000, 0x80000, CRC(29071ed7) SHA1(eb438fcb42e3fbe38e20bc021be079a3dd7a89fa) )
	ROM_LOAD16_WORD_SWAP( "smbj.05a", 0x100000, 0x80000, CRC(eb20bce4) SHA1(b78a447d3d1d3f9a62a6b5abcd893f5e091f1bbc) )
	ROM_LOAD16_WORD_SWAP( "smbj.06a", 0x180000, 0x80000, CRC(94b420cd) SHA1(4cc43d3f7fed224443e26df5b0076bd24e6cd04b) )
	ROM_LOAD16_WORD_SWAP( "smb.07",  0x200000, 0x80000, CRC(b9a11577) SHA1(e9b58ef8acd1fedd3c9e0a3489593c7e931106c0) )
	ROM_LOAD16_WORD_SWAP( "smb.08",  0x280000, 0x80000, CRC(f931b76b) SHA1(0b7e8d1278dcba89f0063bd09cda96d6ae1bc282) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "smbjx.03a", 0x000000, 0x80000, CRC(3600f8d8) SHA1(6ab25c8ab17e5a53ab48103c235542c69373be1b) )
	ROM_LOAD16_WORD_SWAP( "smbjx.04a", 0x080000, 0x80000, CRC(6d0f1b81) SHA1(870d057c1f9ad22a1b2163fabfbad67107956ea4) )
	ROM_LOAD16_WORD_SWAP( "smbjx.05a", 0x100000, 0x80000, CRC(97f5b4af) SHA1(559c95b29607c223a8ad7f410d89ff4d45087d74) )

	ROM_REGION( 0x1200000, REGION_GFX1, 0 )
	ROMX_LOAD( "smb.13",   0x0000000, 0x200000, CRC(d9b2d1de) SHA1(e8658983070dadcd1300a680a42c8431579e2b4f) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "smb.15",   0x0000002, 0x200000, CRC(9a766d92) SHA1(afdf88afbec527268d63c11ea32f861b52e11489) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "smb.17",   0x0000004, 0x200000, CRC(51800f0f) SHA1(9526cd69a23340a81841271b51de03d9bf2b979f) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "smb.19",   0x0000006, 0x200000, CRC(35757e96) SHA1(c915f3b9e4fdec3defc7eecb2c1f7377e6072228) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "smb.14",   0x0800000, 0x200000, CRC(e5bfd0e7) SHA1(327e626df4c2152f921f15535c01dda6c4437527) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "smb.16",   0x0800002, 0x200000, CRC(c56c0866) SHA1(1e2218e852ae72a9a95861dd37129fe78d4b1329) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "smb.18",   0x0800004, 0x200000, CRC(4ded3910) SHA1(d883541ce4d83f4e7ab095f2ef273408d9911f9a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "smb.20",   0x0800006, 0x200000, CRC(26ea1ec5) SHA1(22be249b1f73272feacf4026f09fc877f5d86353) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "smb.21",   0x1000000, 0x080000, CRC(0a08c5fc) SHA1(ff3fad4fbc98e3013291c7ba7ee5e057a2628b36) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "smb.23",   0x1000002, 0x080000, CRC(0911b6c4) SHA1(e7a7061b192658724d98cae8693f63dd5bc40c00) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "smb.25",   0x1000004, 0x080000, CRC(82d6c4ec) SHA1(ed8ed02a00f59a048b9891ec2a77720bb6a5e03d) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "smb.27",   0x1000006, 0x080000, CRC(9b48678b) SHA1(4fa300d356c538947983ae85bb5c5cfd1fb835e7) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "smb.01",   0x00000, 0x08000, CRC(0abc229a) SHA1(967f574e6358dfc1b01e6a4a4df1a8f34eb3d814) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "smb.02",   0x28000, 0x20000, CRC(d051679a) SHA1(583c2521a30db1740d95dd94a38751fbeff3aae5) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "smb.11",   0x000000, 0x200000, CRC(c56935f9) SHA1(ca1705e48e31ddc13505e6297bceca2bec1bb209) )
	ROM_LOAD16_WORD_SWAP( "smb.12",   0x200000, 0x200000, CRC(955b0782) SHA1(ee09500e7b44e923126533613bfe26cdabc7ab5f) )
ROM_END

ROM_START( smbombr1 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "smbj.03", 0x000000, 0x80000, CRC(52eafb10) SHA1(5abfe07e948748eba982dc8f2e21462aec187590) )
	ROM_LOAD16_WORD_SWAP( "smbj.04", 0x080000, 0x80000, CRC(aa6e8078) SHA1(58b4e15e7e3209e59a37ce48d8b9f0dc8b933cdc) )
	ROM_LOAD16_WORD_SWAP( "smbj.05", 0x100000, 0x80000, CRC(b69e7d5f) SHA1(e66430ef05ed0d1c848d24c7436ee5f1b511dcea) )
	ROM_LOAD16_WORD_SWAP( "smbj.06", 0x180000, 0x80000, CRC(8d857b56) SHA1(48c4e5f195e4343a8b7b9ec496fa1a77d659f72e) )
	ROM_LOAD16_WORD_SWAP( "smb.07",  0x200000, 0x80000, CRC(b9a11577) SHA1(e9b58ef8acd1fedd3c9e0a3489593c7e931106c0) )
	ROM_LOAD16_WORD_SWAP( "smb.08",  0x280000, 0x80000, CRC(f931b76b) SHA1(0b7e8d1278dcba89f0063bd09cda96d6ae1bc282) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "smbjx.03", 0x000000, 0x80000, CRC(b0b67439) SHA1(eec7e9c4172379946d642e9bb2cb0188e77ae2e2) )
	ROM_LOAD16_WORD_SWAP( "smbjx.04", 0x080000, 0x80000, CRC(a012f690) SHA1(b9c591d9cedfc7e69267c79bfab9b198a22d8c6e) )
	ROM_LOAD16_WORD_SWAP( "smbjx.05", 0x100000, 0x80000, CRC(f6e886d0) SHA1(ea7e3bfa39b115b74b8969cbedcc0c5c0fe404cb) )

	ROM_REGION( 0x1200000, REGION_GFX1, 0 )
	ROMX_LOAD( "smb.13",   0x0000000, 0x200000, CRC(d9b2d1de) SHA1(e8658983070dadcd1300a680a42c8431579e2b4f) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "smb.15",   0x0000002, 0x200000, CRC(9a766d92) SHA1(afdf88afbec527268d63c11ea32f861b52e11489) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "smb.17",   0x0000004, 0x200000, CRC(51800f0f) SHA1(9526cd69a23340a81841271b51de03d9bf2b979f) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "smb.19",   0x0000006, 0x200000, CRC(35757e96) SHA1(c915f3b9e4fdec3defc7eecb2c1f7377e6072228) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "smb.14",   0x0800000, 0x200000, CRC(e5bfd0e7) SHA1(327e626df4c2152f921f15535c01dda6c4437527) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "smb.16",   0x0800002, 0x200000, CRC(c56c0866) SHA1(1e2218e852ae72a9a95861dd37129fe78d4b1329) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "smb.18",   0x0800004, 0x200000, CRC(4ded3910) SHA1(d883541ce4d83f4e7ab095f2ef273408d9911f9a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "smb.20",   0x0800006, 0x200000, CRC(26ea1ec5) SHA1(22be249b1f73272feacf4026f09fc877f5d86353) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "smb.21",   0x1000000, 0x080000, CRC(0a08c5fc) SHA1(ff3fad4fbc98e3013291c7ba7ee5e057a2628b36) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "smb.23",   0x1000002, 0x080000, CRC(0911b6c4) SHA1(e7a7061b192658724d98cae8693f63dd5bc40c00) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "smb.25",   0x1000004, 0x080000, CRC(82d6c4ec) SHA1(ed8ed02a00f59a048b9891ec2a77720bb6a5e03d) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "smb.27",   0x1000006, 0x080000, CRC(9b48678b) SHA1(4fa300d356c538947983ae85bb5c5cfd1fb835e7) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "smb.01",   0x00000, 0x08000, CRC(0abc229a) SHA1(967f574e6358dfc1b01e6a4a4df1a8f34eb3d814) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "smb.02",   0x28000, 0x20000, CRC(d051679a) SHA1(583c2521a30db1740d95dd94a38751fbeff3aae5) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "smb.11",   0x000000, 0x200000, CRC(c56935f9) SHA1(ca1705e48e31ddc13505e6297bceca2bec1bb209) )
	ROM_LOAD16_WORD_SWAP( "smb.12",   0x200000, 0x200000, CRC(955b0782) SHA1(ee09500e7b44e923126533613bfe26cdabc7ab5f) )
ROM_END

ROM_START( sfa )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "sfze.03b", 0x000000, 0x80000, CRC(ebf2054d) SHA1(6e7b9e4202b86ab237ea5634c98b71b82d812ef2) )
	ROM_LOAD16_WORD_SWAP( "sfz.04b",  0x080000, 0x80000, CRC(8b73b0e5) SHA1(5318761f615c21395366b5333e75eaaa73ef2073) )
	ROM_LOAD16_WORD_SWAP( "sfz.05a",  0x100000, 0x80000, CRC(0810544d) SHA1(5f39bda3e7b16508eb58e5a2e0cc58c09cf428ce) )
	ROM_LOAD16_WORD_SWAP( "sfz.06",   0x180000, 0x80000, CRC(806e8f38) SHA1(b6d6912aa8f2f590335d7ff9a8214648e7131ebb) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "sfzex.03b", 0x000000, 0x80000, CRC(505a1b4a) SHA1(3bfdd956ceb4412f260a6044ab2bcee334c1c5cb) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROM_FILL(              0x000000, 0x800000, 0 )
	ROMX_LOAD( "sfz.14",   0x800000, 0x200000, CRC(90fefdb3) SHA1(5eb28c8de57acfeaefebdd01509c7d9ba5244705) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz.16",   0x800002, 0x200000, CRC(5354c948) SHA1(07588f1ba6addc04fef3274c971174aaf3e632ab) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz.18",   0x800004, 0x200000, CRC(41a1e790) SHA1(ce25dad542308691dbe9606b26279bbd59ea4b81) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz.20",   0x800006, 0x200000, CRC(a549df98) SHA1(f054e95df650a891ef56da8bfb31cb2c945a9aed) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sfz.01",   0x00000, 0x08000, CRC(ffffec7d) SHA1(75b4aef001b72a0f571b51b2b97803facc1832dd) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "sfz.02",   0x28000, 0x20000, CRC(45f46a08) SHA1(e32dbd27b52ab708278045b5a829376e55a4ca81) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "sfz.11",   0x000000, 0x200000, CRC(c4b093cd) SHA1(256526bb693a0b72443f047e060304c9b739acd1) )
	ROM_LOAD16_WORD_SWAP( "sfz.12",   0x200000, 0x200000, CRC(8bdbc4b4) SHA1(0e21c9a75a17a7e7dfd8bb51098c2b9dc4c933ec) )
ROM_END

ROM_START( sfar1 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "sfze.03c", 0x000000, 0x80000, CRC(a1b69dd7) SHA1(b41440eba8f33eed955c987a04d99fca6c5c90e5) )  /* Rom name dosnt appear to follow normal capcom naming system and was written on rom by hand*/
	ROM_LOAD16_WORD_SWAP( "sfze.04b", 0x080000, 0x80000, CRC(bb90acd5) SHA1(a19795963b90f1152f44cae29e78dd2ce67a41d6) )  /* Rom name dosnt appear to follow normal capcom naming system and was written on rom by hand*/
	ROM_LOAD16_WORD_SWAP( "sfz.05a",  0x100000, 0x80000, CRC(0810544d) SHA1(5f39bda3e7b16508eb58e5a2e0cc58c09cf428ce) )
	ROM_LOAD16_WORD_SWAP( "sfz.06",   0x180000, 0x80000, CRC(806e8f38) SHA1(b6d6912aa8f2f590335d7ff9a8214648e7131ebb) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "sfzex.03c", 0x000000, 0x80000, CRC(818f6bde) SHA1(d139da916736ecbec4d3752c9ef2d0641834ca08) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROM_FILL(              0x000000, 0x800000, 0 )
	ROMX_LOAD( "sfz.14",   0x800000, 0x200000, CRC(90fefdb3) SHA1(5eb28c8de57acfeaefebdd01509c7d9ba5244705) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz.16",   0x800002, 0x200000, CRC(5354c948) SHA1(07588f1ba6addc04fef3274c971174aaf3e632ab) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz.18",   0x800004, 0x200000, CRC(41a1e790) SHA1(ce25dad542308691dbe9606b26279bbd59ea4b81) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz.20",   0x800006, 0x200000, CRC(a549df98) SHA1(f054e95df650a891ef56da8bfb31cb2c945a9aed) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sfz.01",   0x00000, 0x08000, CRC(ffffec7d) SHA1(75b4aef001b72a0f571b51b2b97803facc1832dd) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "sfz.02",   0x28000, 0x20000, CRC(45f46a08) SHA1(e32dbd27b52ab708278045b5a829376e55a4ca81) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "sfz.11",   0x000000, 0x200000, CRC(c4b093cd) SHA1(256526bb693a0b72443f047e060304c9b739acd1) )
	ROM_LOAD16_WORD_SWAP( "sfz.12",   0x200000, 0x200000, CRC(8bdbc4b4) SHA1(0e21c9a75a17a7e7dfd8bb51098c2b9dc4c933ec) )
ROM_END

ROM_START( sfar2 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "sfze.03a", 0x000000, 0x80000, CRC(fdbcd434) SHA1(1d5f9b821d9e0d45be61896969500b877a112fad) )
	ROM_LOAD16_WORD_SWAP( "sfz.04",   0x080000, 0x80000, CRC(0c436d30) SHA1(84229896c99bb2a4fbbab33644f779c9f86704fb) )
	ROM_LOAD16_WORD_SWAP( "sfz.05",   0x100000, 0x80000, CRC(1f363612) SHA1(87203b5db2d3887762da431d6fc2f2b76d4feedb) )
	ROM_LOAD16_WORD_SWAP( "sfz.06",   0x180000, 0x80000, CRC(806e8f38) SHA1(b6d6912aa8f2f590335d7ff9a8214648e7131ebb) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "sfzex.03a", 0x000000, 0x80000, CRC(b50d87c7) SHA1(f2361f669875bc261862014f331f94f9d1d3ac71) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROM_FILL(              0x000000, 0x800000, 0 )
	ROMX_LOAD( "sfz.14",   0x800000, 0x200000, CRC(90fefdb3) SHA1(5eb28c8de57acfeaefebdd01509c7d9ba5244705) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz.16",   0x800002, 0x200000, CRC(5354c948) SHA1(07588f1ba6addc04fef3274c971174aaf3e632ab) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz.18",   0x800004, 0x200000, CRC(41a1e790) SHA1(ce25dad542308691dbe9606b26279bbd59ea4b81) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz.20",   0x800006, 0x200000, CRC(a549df98) SHA1(f054e95df650a891ef56da8bfb31cb2c945a9aed) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sfz.01",   0x00000, 0x08000, CRC(ffffec7d) SHA1(75b4aef001b72a0f571b51b2b97803facc1832dd) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "sfz.02",   0x28000, 0x20000, CRC(45f46a08) SHA1(e32dbd27b52ab708278045b5a829376e55a4ca81) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "sfz.11",   0x000000, 0x200000, CRC(c4b093cd) SHA1(256526bb693a0b72443f047e060304c9b739acd1) )
	ROM_LOAD16_WORD_SWAP( "sfz.12",   0x200000, 0x200000, CRC(8bdbc4b4) SHA1(0e21c9a75a17a7e7dfd8bb51098c2b9dc4c933ec) )
ROM_END

ROM_START( sfau )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "sfzu.03a", 0x000000, 0x80000, CRC(49fc7db9) SHA1(2a13d987fade88e0372f418cf451f34de67372d5) )
	ROM_LOAD16_WORD_SWAP( "sfz.04a",  0x080000, 0x80000, CRC(5f99e9a5) SHA1(e9f286315d17096adc08e6b4e6ff7c5351f5bef3) )
	ROM_LOAD16_WORD_SWAP( "sfz.05a",  0x100000, 0x80000, CRC(0810544d) SHA1(5f39bda3e7b16508eb58e5a2e0cc58c09cf428ce) )
	ROM_LOAD16_WORD_SWAP( "sfz.06",   0x180000, 0x80000, CRC(806e8f38) SHA1(b6d6912aa8f2f590335d7ff9a8214648e7131ebb) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "sfzux.03a", 0x000000, 0x80000, CRC(1a3160ed) SHA1(da2487b1cbd73a399a6feb15eb8c4d5f4ef26fc3) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROM_FILL(              0x000000, 0x800000, 0 )
	ROMX_LOAD( "sfz.14",   0x800000, 0x200000, CRC(90fefdb3) SHA1(5eb28c8de57acfeaefebdd01509c7d9ba5244705) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz.16",   0x800002, 0x200000, CRC(5354c948) SHA1(07588f1ba6addc04fef3274c971174aaf3e632ab) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz.18",   0x800004, 0x200000, CRC(41a1e790) SHA1(ce25dad542308691dbe9606b26279bbd59ea4b81) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz.20",   0x800006, 0x200000, CRC(a549df98) SHA1(f054e95df650a891ef56da8bfb31cb2c945a9aed) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sfz.01",   0x00000, 0x08000, CRC(ffffec7d) SHA1(75b4aef001b72a0f571b51b2b97803facc1832dd) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "sfz.02",   0x28000, 0x20000, CRC(45f46a08) SHA1(e32dbd27b52ab708278045b5a829376e55a4ca81) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "sfz.11",   0x000000, 0x200000, CRC(c4b093cd) SHA1(256526bb693a0b72443f047e060304c9b739acd1) )
	ROM_LOAD16_WORD_SWAP( "sfz.12",   0x200000, 0x200000, CRC(8bdbc4b4) SHA1(0e21c9a75a17a7e7dfd8bb51098c2b9dc4c933ec) )
ROM_END

ROM_START( sfzj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "sfzj.03c", 0x000000, 0x80000, CRC(f5444120) SHA1(22158894971754ad83b8eeb8bdfb9874794b98c0) )
	ROM_LOAD16_WORD_SWAP( "sfz.04b",  0x080000, 0x80000, CRC(8b73b0e5) SHA1(5318761f615c21395366b5333e75eaaa73ef2073) )
	ROM_LOAD16_WORD_SWAP( "sfz.05a",  0x100000, 0x80000, CRC(0810544d) SHA1(5f39bda3e7b16508eb58e5a2e0cc58c09cf428ce) )
	ROM_LOAD16_WORD_SWAP( "sfz.06",   0x180000, 0x80000, CRC(806e8f38) SHA1(b6d6912aa8f2f590335d7ff9a8214648e7131ebb) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "sfzjx.03c", 0x000000, 0x80000, CRC(d6b17a9b) SHA1(f6d3726427c6bee7e02505a9332759fc959b7d43) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROM_FILL(              0x000000, 0x800000, 0 )
	ROMX_LOAD( "sfz.14",   0x800000, 0x200000, CRC(90fefdb3) SHA1(5eb28c8de57acfeaefebdd01509c7d9ba5244705) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz.16",   0x800002, 0x200000, CRC(5354c948) SHA1(07588f1ba6addc04fef3274c971174aaf3e632ab) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz.18",   0x800004, 0x200000, CRC(41a1e790) SHA1(ce25dad542308691dbe9606b26279bbd59ea4b81) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz.20",   0x800006, 0x200000, CRC(a549df98) SHA1(f054e95df650a891ef56da8bfb31cb2c945a9aed) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sfz.01",   0x00000, 0x08000, CRC(ffffec7d) SHA1(75b4aef001b72a0f571b51b2b97803facc1832dd) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "sfz.02",   0x28000, 0x20000, CRC(45f46a08) SHA1(e32dbd27b52ab708278045b5a829376e55a4ca81) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "sfz.11",   0x000000, 0x200000, CRC(c4b093cd) SHA1(256526bb693a0b72443f047e060304c9b739acd1) )
	ROM_LOAD16_WORD_SWAP( "sfz.12",   0x200000, 0x200000, CRC(8bdbc4b4) SHA1(0e21c9a75a17a7e7dfd8bb51098c2b9dc4c933ec) )
ROM_END

ROM_START( sfzjr1 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "sfzj.03b", 0x000000, 0x80000, CRC(844220c2) SHA1(ff295207e0f9679285d805aa494537ae7daf1634) )
	ROM_LOAD16_WORD_SWAP( "sfz.04a",  0x080000, 0x80000, CRC(5f99e9a5) SHA1(e9f286315d17096adc08e6b4e6ff7c5351f5bef3) )
	ROM_LOAD16_WORD_SWAP( "sfz.05a",  0x100000, 0x80000, CRC(0810544d) SHA1(5f39bda3e7b16508eb58e5a2e0cc58c09cf428ce) )
	ROM_LOAD16_WORD_SWAP( "sfz.06",   0x180000, 0x80000, CRC(806e8f38) SHA1(b6d6912aa8f2f590335d7ff9a8214648e7131ebb) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "sfzjx.03b", 0x000000, 0x80000, CRC(b501f03c) SHA1(9f73a6177acae9174f06811642f7675e51a5809d) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROM_FILL(              0x000000, 0x800000, 0 )
	ROMX_LOAD( "sfz.14",   0x800000, 0x200000, CRC(90fefdb3) SHA1(5eb28c8de57acfeaefebdd01509c7d9ba5244705) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz.16",   0x800002, 0x200000, CRC(5354c948) SHA1(07588f1ba6addc04fef3274c971174aaf3e632ab) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz.18",   0x800004, 0x200000, CRC(41a1e790) SHA1(ce25dad542308691dbe9606b26279bbd59ea4b81) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz.20",   0x800006, 0x200000, CRC(a549df98) SHA1(f054e95df650a891ef56da8bfb31cb2c945a9aed) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sfz.01",   0x00000, 0x08000, CRC(ffffec7d) SHA1(75b4aef001b72a0f571b51b2b97803facc1832dd) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "sfz.02",   0x28000, 0x20000, CRC(45f46a08) SHA1(e32dbd27b52ab708278045b5a829376e55a4ca81) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "sfz.11",   0x000000, 0x200000, CRC(c4b093cd) SHA1(256526bb693a0b72443f047e060304c9b739acd1) )
	ROM_LOAD16_WORD_SWAP( "sfz.12",   0x200000, 0x200000, CRC(8bdbc4b4) SHA1(0e21c9a75a17a7e7dfd8bb51098c2b9dc4c933ec) )
ROM_END

ROM_START( sfzjr2 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "sfzj.03a", 0x000000, 0x80000, CRC(3cfce93c) SHA1(5f64e9707cb3d911f44e041d980e4b2250f49d75) )
	ROM_LOAD16_WORD_SWAP( "sfz.04",   0x080000, 0x80000, CRC(0c436d30) SHA1(84229896c99bb2a4fbbab33644f779c9f86704fb) )
	ROM_LOAD16_WORD_SWAP( "sfz.05",   0x100000, 0x80000, CRC(1f363612) SHA1(87203b5db2d3887762da431d6fc2f2b76d4feedb) )
	ROM_LOAD16_WORD_SWAP( "sfz.06",   0x180000, 0x80000, CRC(806e8f38) SHA1(b6d6912aa8f2f590335d7ff9a8214648e7131ebb) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "sfzjx.03a", 0x000000, 0x80000, CRC(3cc138b5) SHA1(9192989793c4388744d89df93781bb73e99315a0) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROM_FILL(              0x000000, 0x800000, 0 )
	ROMX_LOAD( "sfz.14",   0x800000, 0x200000, CRC(90fefdb3) SHA1(5eb28c8de57acfeaefebdd01509c7d9ba5244705) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz.16",   0x800002, 0x200000, CRC(5354c948) SHA1(07588f1ba6addc04fef3274c971174aaf3e632ab) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz.18",   0x800004, 0x200000, CRC(41a1e790) SHA1(ce25dad542308691dbe9606b26279bbd59ea4b81) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz.20",   0x800006, 0x200000, CRC(a549df98) SHA1(f054e95df650a891ef56da8bfb31cb2c945a9aed) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sfz.01",   0x00000, 0x08000, CRC(ffffec7d) SHA1(75b4aef001b72a0f571b51b2b97803facc1832dd) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "sfz.02",   0x28000, 0x20000, CRC(45f46a08) SHA1(e32dbd27b52ab708278045b5a829376e55a4ca81) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "sfz.11",   0x000000, 0x200000, CRC(c4b093cd) SHA1(256526bb693a0b72443f047e060304c9b739acd1) )
	ROM_LOAD16_WORD_SWAP( "sfz.12",   0x200000, 0x200000, CRC(8bdbc4b4) SHA1(0e21c9a75a17a7e7dfd8bb51098c2b9dc4c933ec) )
ROM_END

ROM_START( sfzh )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "sfzh.03c", 0x000000, 0x80000, CRC(bce635aa) SHA1(323da2de6c3ff6fd8c2c66ce6bd1d287873db9b1) )
	ROM_LOAD16_WORD_SWAP( "sfz.04a",  0x080000, 0x80000, CRC(5f99e9a5) SHA1(e9f286315d17096adc08e6b4e6ff7c5351f5bef3) )
	ROM_LOAD16_WORD_SWAP( "sfz.05a",  0x100000, 0x80000, CRC(0810544d) SHA1(5f39bda3e7b16508eb58e5a2e0cc58c09cf428ce) )
	ROM_LOAD16_WORD_SWAP( "sfz.06",   0x180000, 0x80000, CRC(806e8f38) SHA1(b6d6912aa8f2f590335d7ff9a8214648e7131ebb) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
/*	ROM_LOAD16_WORD_SWAP( "sfzhx.03c", 0x000000, 0x80000, NO_DUMP )*/

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROM_FILL(              0x000000, 0x800000, 0 )
	ROMX_LOAD( "sfz.14",   0x800000, 0x200000, CRC(90fefdb3) SHA1(5eb28c8de57acfeaefebdd01509c7d9ba5244705) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz.16",   0x800002, 0x200000, CRC(5354c948) SHA1(07588f1ba6addc04fef3274c971174aaf3e632ab) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz.18",   0x800004, 0x200000, CRC(41a1e790) SHA1(ce25dad542308691dbe9606b26279bbd59ea4b81) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz.20",   0x800006, 0x200000, CRC(a549df98) SHA1(f054e95df650a891ef56da8bfb31cb2c945a9aed) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sfz.01",   0x00000, 0x08000, CRC(ffffec7d) SHA1(75b4aef001b72a0f571b51b2b97803facc1832dd) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "sfz.02",   0x28000, 0x20000, CRC(45f46a08) SHA1(e32dbd27b52ab708278045b5a829376e55a4ca81) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "sfz.11",   0x000000, 0x200000, CRC(c4b093cd) SHA1(256526bb693a0b72443f047e060304c9b739acd1) )
	ROM_LOAD16_WORD_SWAP( "sfz.12",   0x200000, 0x200000, CRC(8bdbc4b4) SHA1(0e21c9a75a17a7e7dfd8bb51098c2b9dc4c933ec) )
ROM_END

ROM_START( sfzb )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "sfzb.03g", 0x000000, 0x80000, CRC(348862d4) SHA1(b48c7df17f8b681fc726931dbf81f5aeb762a5b3) )
	ROM_LOAD16_WORD_SWAP( "sfzb.04e", 0x080000, 0x80000, CRC(8d9b2480) SHA1(405305c1572908d00eab735f28676fbbadb4fac6) )
	ROM_LOAD16_WORD_SWAP( "sfz.05a", 0x100000,  0x80000, CRC(0810544d) SHA1(5f39bda3e7b16508eb58e5a2e0cc58c09cf428ce) )
	ROM_LOAD16_WORD_SWAP( "sfz.06",  0x180000,  0x80000, CRC(806e8f38) SHA1(b6d6912aa8f2f590335d7ff9a8214648e7131ebb) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
/*	ROM_LOAD16_WORD_SWAP( "sfzbx.03", 0x000000, 0x80000, NO_DUMP )*/

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROM_FILL(              0x000000, 0x800000, 0 )
	ROMX_LOAD( "sfz.14",   0x800000, 0x200000, CRC(90fefdb3) SHA1(5eb28c8de57acfeaefebdd01509c7d9ba5244705) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz.16",   0x800002, 0x200000, CRC(5354c948) SHA1(07588f1ba6addc04fef3274c971174aaf3e632ab) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz.18",   0x800004, 0x200000, CRC(41a1e790) SHA1(ce25dad542308691dbe9606b26279bbd59ea4b81) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz.20",   0x800006, 0x200000, CRC(a549df98) SHA1(f054e95df650a891ef56da8bfb31cb2c945a9aed) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sfz.01",   0x00000, 0x08000, CRC(ffffec7d) SHA1(75b4aef001b72a0f571b51b2b97803facc1832dd) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "sfz.02",   0x28000, 0x20000, CRC(45f46a08) SHA1(e32dbd27b52ab708278045b5a829376e55a4ca81) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "sfz.11",   0x000000, 0x200000, CRC(c4b093cd) SHA1(256526bb693a0b72443f047e060304c9b739acd1) )
	ROM_LOAD16_WORD_SWAP( "sfz.12",   0x200000, 0x200000, CRC(8bdbc4b4) SHA1(0e21c9a75a17a7e7dfd8bb51098c2b9dc4c933ec) )
ROM_END

ROM_START( sfa2 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "sz2u.03", 0x000000, 0x80000, CRC(84a09006) SHA1(334c33f9eb324d71443cc9c44e94f5a72451fa3f) )
	ROM_LOAD16_WORD_SWAP( "sz2u.04", 0x080000, 0x80000, CRC(ac46e5ed) SHA1(a01b57daba4c255d5f07465c553bcbfe51d9ab0d) )
	ROM_LOAD16_WORD_SWAP( "sz2u.05", 0x100000, 0x80000, CRC(6c0c79d3) SHA1(ae2a4e2903beec1f10fff6edac1a2385d6ac1c38) )
	ROM_LOAD16_WORD_SWAP( "sz2u.06", 0x180000, 0x80000, CRC(c5c8eb63) SHA1(4ea033834c7b260877335296f88c0db484dea289) )
	ROM_LOAD16_WORD_SWAP( "sz2u.07", 0x200000, 0x80000, CRC(5de01cc5) SHA1(b19bfe970b217c96e782860fc3ae3fcb976ed30d) )
	ROM_LOAD16_WORD_SWAP( "sz2u.08", 0x280000, 0x80000, CRC(bea11d56) SHA1(a1d475066d36de7cc5d931671ccdcd89737bc7ee) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "sz2ux.03", 0x000000, 0x80000, CRC(6bb6005f) SHA1(faf8052047052cb353c2fbcf2aa2840c9d3b2cea) )
	ROM_LOAD16_WORD_SWAP( "sz2ux.04", 0x080000, 0x80000, CRC(74308a4b) SHA1(d3dfeec64198be73b5594fd6a8f8d5440c637da1) )

	ROM_REGION( 0x1400000, REGION_GFX1, 0 )
	ROMX_LOAD( "sz2.13",   0x0000000, 0x400000, CRC(4d1f1f22) SHA1(659fb4305bcf0cbbbbec97ede6e68a8323b13308) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.15",   0x0000002, 0x400000, CRC(19cea680) SHA1(4cb88963a0fbcef191c8419b6379387c01b4c81e) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.17",   0x0000004, 0x400000, CRC(e01b4588) SHA1(c2936608fd75ff6cd5fa94c6d6d6f0c77c44a450) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.19",   0x0000006, 0x400000, CRC(0feeda64) SHA1(f5b350601437bd94b70d97feb23d791df19da6b3) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.14",   0x1000000, 0x100000, CRC(0560c6aa) SHA1(f2bed3a8efef18052b51a7f0f6a888a18db813a1) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.16",   0x1000002, 0x100000, CRC(ae940f87) SHA1(39ee26333abbe302ba76dced0196a2e6b3b1d02a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.18",   0x1000004, 0x100000, CRC(4bc3c8bc) SHA1(6256963c515bf56f39b6e559afefd653ead56c54) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.20",   0x1000006, 0x100000, CRC(39e674c0) SHA1(8e771a2d8c2accad0463bccd21d7b23af0c895a1) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sz2.01a",   0x00000, 0x08000, CRC(1bc323cf) SHA1(83fbd6e9b327700dc9f1c59700b7385bc3705749) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "sz2.02a",   0x28000, 0x20000, CRC(ba6a5013) SHA1(7814f3e56b69529b9860dd61c3b1e8d700244b03) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "sz2.11",   0x000000, 0x200000, CRC(aa47a601) SHA1(a4d1ee89c84a3b9db06469bb66e85293b5aa9ac9) )
	ROM_LOAD16_WORD_SWAP( "sz2.12",   0x200000, 0x200000, CRC(2237bc53) SHA1(96d5693047e4cf1ed10a8ee1905cea267a278e92) )
ROM_END

ROM_START( sfz2j )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "sz2j.03a", 0x000000, 0x80000, CRC(97461e28) SHA1(8fbe4c9a59f51612f86adb8ef5057e43be0348bf) )
	ROM_LOAD16_WORD_SWAP( "sz2j.04a", 0x080000, 0x80000, CRC(ae4851a9) SHA1(4771bc22fe1b376b753a68506c012c52bd4b886d) )
	ROM_LOAD16_WORD_SWAP( "sz2.05",   0x100000, 0x80000, CRC(98e8e992) SHA1(41745b63e6b3888081d189b8315ed3b7526b3d20) )
	ROM_LOAD16_WORD_SWAP( "sz2.06",   0x180000, 0x80000, CRC(5b1d49c0) SHA1(f0a0c894c9cbe2b18e7f59058665949ee0025732) )
	ROM_LOAD16_WORD_SWAP( "sz2j.07a", 0x200000, 0x80000, CRC(d910b2a2) SHA1(aa201660caa9cef993c147a1077c9e7767b34a78) )
	ROM_LOAD16_WORD_SWAP( "sz2.08",   0x280000, 0x80000, CRC(0fe8585d) SHA1(0cd5369a5aa90c98d8dc1ff3342cd4d990631cff) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "sz2jx.03a", 0x000000, 0x80000, CRC(6a765c08) SHA1(dab39e17e749b32908a22fa62812034a0e1a5189) )
	ROM_LOAD16_WORD_SWAP( "sz2jx.04a", 0x080000, 0x80000, CRC(66139273) SHA1(e9a3c56bfd0f29f192cc9a53447bddadadb3815a) )

	ROM_REGION( 0x1400000, REGION_GFX1, 0 )
	ROMX_LOAD( "sz2.13",   0x0000000, 0x400000, CRC(4d1f1f22) SHA1(659fb4305bcf0cbbbbec97ede6e68a8323b13308) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.15",   0x0000002, 0x400000, CRC(19cea680) SHA1(4cb88963a0fbcef191c8419b6379387c01b4c81e) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.17",   0x0000004, 0x400000, CRC(e01b4588) SHA1(c2936608fd75ff6cd5fa94c6d6d6f0c77c44a450) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.19",   0x0000006, 0x400000, CRC(0feeda64) SHA1(f5b350601437bd94b70d97feb23d791df19da6b3) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.14",   0x1000000, 0x100000, CRC(0560c6aa) SHA1(f2bed3a8efef18052b51a7f0f6a888a18db813a1) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.16",   0x1000002, 0x100000, CRC(ae940f87) SHA1(39ee26333abbe302ba76dced0196a2e6b3b1d02a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.18",   0x1000004, 0x100000, CRC(4bc3c8bc) SHA1(6256963c515bf56f39b6e559afefd653ead56c54) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.20",   0x1000006, 0x100000, CRC(39e674c0) SHA1(8e771a2d8c2accad0463bccd21d7b23af0c895a1) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sz2.01a",   0x00000, 0x08000, CRC(1bc323cf) SHA1(83fbd6e9b327700dc9f1c59700b7385bc3705749) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "sz2.02a",   0x28000, 0x20000, CRC(ba6a5013) SHA1(7814f3e56b69529b9860dd61c3b1e8d700244b03) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "sz2.11",   0x000000, 0x200000, CRC(aa47a601) SHA1(a4d1ee89c84a3b9db06469bb66e85293b5aa9ac9) )
	ROM_LOAD16_WORD_SWAP( "sz2.12",   0x200000, 0x200000, CRC(2237bc53) SHA1(96d5693047e4cf1ed10a8ee1905cea267a278e92) )
ROM_END

ROM_START( sfz2a)
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "sz2a.03a", 0x000000, 0x80000, CRC(30d2099f) SHA1(d4c7d8c2ad08cae228544bd692aedecd4fab829c) )
	ROM_LOAD16_WORD_SWAP( "sz2a.04a", 0x080000, 0x80000, CRC(1cc94db1) SHA1(518151f443ff5219b20c9fd59b7614920302aecd) )
	ROM_LOAD16_WORD_SWAP( "sz2.05",   0x100000, 0x80000, CRC(98e8e992) SHA1(41745b63e6b3888081d189b8315ed3b7526b3d20) )
	ROM_LOAD16_WORD_SWAP( "sz2.06",   0x180000, 0x80000, CRC(5b1d49c0) SHA1(f0a0c894c9cbe2b18e7f59058665949ee0025732) )
	ROM_LOAD16_WORD_SWAP( "sz2a.07a", 0x200000, 0x80000, CRC(0aed2494) SHA1(7beb1a394f17cd78a27128292b626aae28754ca2) )
	ROM_LOAD16_WORD_SWAP( "sz2.08",   0x280000, 0x80000, CRC(0fe8585d) SHA1(0cd5369a5aa90c98d8dc1ff3342cd4d990631cff) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "sz2ax.03a", 0x000000, 0x80000, CRC(b76d8578) SHA1(78a9599e8d5d20d8c30772a498d3d5d0af4ec85b) )
	ROM_LOAD16_WORD_SWAP( "sz2ax.04a", 0x080000, 0x80000, CRC(d4dc0b1e) SHA1(fa87ed3135061a32efd0cfdc0f18aa5aa03aab55) )

	ROM_REGION( 0x1400000, REGION_GFX1, 0 )
	ROMX_LOAD( "sz2.13",   0x0000000, 0x400000, CRC(4d1f1f22) SHA1(659fb4305bcf0cbbbbec97ede6e68a8323b13308) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.15",   0x0000002, 0x400000, CRC(19cea680) SHA1(4cb88963a0fbcef191c8419b6379387c01b4c81e) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.17",   0x0000004, 0x400000, CRC(e01b4588) SHA1(c2936608fd75ff6cd5fa94c6d6d6f0c77c44a450) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.19",   0x0000006, 0x400000, CRC(0feeda64) SHA1(f5b350601437bd94b70d97feb23d791df19da6b3) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.14",   0x1000000, 0x100000, CRC(0560c6aa) SHA1(f2bed3a8efef18052b51a7f0f6a888a18db813a1) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.16",   0x1000002, 0x100000, CRC(ae940f87) SHA1(39ee26333abbe302ba76dced0196a2e6b3b1d02a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.18",   0x1000004, 0x100000, CRC(4bc3c8bc) SHA1(6256963c515bf56f39b6e559afefd653ead56c54) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.20",   0x1000006, 0x100000, CRC(39e674c0) SHA1(8e771a2d8c2accad0463bccd21d7b23af0c895a1) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sz2.01a",   0x00000, 0x08000, CRC(1bc323cf) SHA1(83fbd6e9b327700dc9f1c59700b7385bc3705749) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "sz2.02a",   0x28000, 0x20000, CRC(ba6a5013) SHA1(7814f3e56b69529b9860dd61c3b1e8d700244b03) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "sz2.11",   0x000000, 0x200000, CRC(aa47a601) SHA1(a4d1ee89c84a3b9db06469bb66e85293b5aa9ac9) )
	ROM_LOAD16_WORD_SWAP( "sz2.12",   0x200000, 0x200000, CRC(2237bc53) SHA1(96d5693047e4cf1ed10a8ee1905cea267a278e92) )
ROM_END

ROM_START( sfz2b )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "sz2b.03b", 0x000000, 0x80000, CRC(1ac12812) SHA1(b948b939021ffe20437b19325fe94daa072c1271) )
	ROM_LOAD16_WORD_SWAP( "sz2b.04b", 0x080000, 0x80000, CRC(e4ffaf68) SHA1(e22bb4f92a965108570c2beee1fd533380838d90) )
	ROM_LOAD16_WORD_SWAP( "sz2b.05",  0x100000, 0x80000, CRC(4b442a7c) SHA1(a0d7d229cff8efb2a253ff06270258b0b4d2761e) )
	ROM_LOAD16_WORD_SWAP( "sz2b.06b", 0x180000, 0x80000, CRC(a45a75a6) SHA1(e9cd4ad08ac0d058e9e1660acb07eb350a141fd6) )
	ROM_LOAD16_WORD_SWAP( "sz2b.07b", 0x200000, 0x80000, CRC(7d19d5ec) SHA1(ab88dfcb2029499578837b8f97fbf55412c8f756) )
	ROM_LOAD16_WORD_SWAP( "sz2b.08",  0x280000, 0x80000, CRC(92b66e01) SHA1(f09cb38aa49b22a9c98219fb2ad8a66b11fa5872) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
/*	ROM_LOAD16_WORD_SWAP( "sz2bx.03b", 0x000000, 0x80000, NO_DUMP )*/
/*	ROM_LOAD16_WORD_SWAP( "sz2bx.04b", 0x080000, 0x80000, NO_DUMP )*/

	ROM_REGION( 0x1400000, REGION_GFX1, 0 )
	ROMX_LOAD( "sz2.13",   0x0000000, 0x400000, CRC(4d1f1f22) SHA1(659fb4305bcf0cbbbbec97ede6e68a8323b13308) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.15",   0x0000002, 0x400000, CRC(19cea680) SHA1(4cb88963a0fbcef191c8419b6379387c01b4c81e) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.17",   0x0000004, 0x400000, CRC(e01b4588) SHA1(c2936608fd75ff6cd5fa94c6d6d6f0c77c44a450) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.19",   0x0000006, 0x400000, CRC(0feeda64) SHA1(f5b350601437bd94b70d97feb23d791df19da6b3) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.14",   0x1000000, 0x100000, CRC(0560c6aa) SHA1(f2bed3a8efef18052b51a7f0f6a888a18db813a1) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.16",   0x1000002, 0x100000, CRC(ae940f87) SHA1(39ee26333abbe302ba76dced0196a2e6b3b1d02a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.18",   0x1000004, 0x100000, CRC(4bc3c8bc) SHA1(6256963c515bf56f39b6e559afefd653ead56c54) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.20",   0x1000006, 0x100000, CRC(39e674c0) SHA1(8e771a2d8c2accad0463bccd21d7b23af0c895a1) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sz2.01a",   0x00000, 0x08000, CRC(1bc323cf) SHA1(83fbd6e9b327700dc9f1c59700b7385bc3705749) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "sz2.02a",   0x28000, 0x20000, CRC(ba6a5013) SHA1(7814f3e56b69529b9860dd61c3b1e8d700244b03) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "sz2.11",   0x000000, 0x200000, CRC(aa47a601) SHA1(a4d1ee89c84a3b9db06469bb66e85293b5aa9ac9) )
	ROM_LOAD16_WORD_SWAP( "sz2.12",   0x200000, 0x200000, CRC(2237bc53) SHA1(96d5693047e4cf1ed10a8ee1905cea267a278e92) )
ROM_END

ROM_START( sfz2br1 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "sz2b.03", 0x000000, 0x80000, CRC(e6ce530b) SHA1(044c3f6f6c64d18f4f9ce96b67ff86b3c8bcd065) )
	ROM_LOAD16_WORD_SWAP( "sz2b.04", 0x080000, 0x80000, CRC(1605a0cb) SHA1(5041c87dbb3ed41fe1cb3e9eade195bc2f7cba2a) )
	ROM_LOAD16_WORD_SWAP( "sz2b.05", 0x100000, 0x80000, CRC(4b442a7c) SHA1(a0d7d229cff8efb2a253ff06270258b0b4d2761e) )
	ROM_LOAD16_WORD_SWAP( "sz2.06",  0x180000, 0x80000, CRC(5b1d49c0) SHA1(f0a0c894c9cbe2b18e7f59058665949ee0025732) )
	ROM_LOAD16_WORD_SWAP( "sz2b.07", 0x200000, 0x80000, CRC(947e8ac6) SHA1(da82be7cba9cd557da3ee35be9194130a959d5cb) )
	ROM_LOAD16_WORD_SWAP( "sz2b.08", 0x280000, 0x80000, CRC(92b66e01) SHA1(f09cb38aa49b22a9c98219fb2ad8a66b11fa5872) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
/*	ROM_LOAD16_WORD_SWAP( "sz2bx.03", 0x000000, 0x80000, NO_DUMP )*/
/*	ROM_LOAD16_WORD_SWAP( "sz2bx.04", 0x080000, 0x80000, NO_DUMP )*/

	ROM_REGION( 0x1400000, REGION_GFX1, 0 )
	ROMX_LOAD( "sz2.13",   0x0000000, 0x400000, CRC(4d1f1f22) SHA1(659fb4305bcf0cbbbbec97ede6e68a8323b13308) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.15",   0x0000002, 0x400000, CRC(19cea680) SHA1(4cb88963a0fbcef191c8419b6379387c01b4c81e) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.17",   0x0000004, 0x400000, CRC(e01b4588) SHA1(c2936608fd75ff6cd5fa94c6d6d6f0c77c44a450) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.19",   0x0000006, 0x400000, CRC(0feeda64) SHA1(f5b350601437bd94b70d97feb23d791df19da6b3) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.14",   0x1000000, 0x100000, CRC(0560c6aa) SHA1(f2bed3a8efef18052b51a7f0f6a888a18db813a1) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.16",   0x1000002, 0x100000, CRC(ae940f87) SHA1(39ee26333abbe302ba76dced0196a2e6b3b1d02a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.18",   0x1000004, 0x100000, CRC(4bc3c8bc) SHA1(6256963c515bf56f39b6e559afefd653ead56c54) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.20",   0x1000006, 0x100000, CRC(39e674c0) SHA1(8e771a2d8c2accad0463bccd21d7b23af0c895a1) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sz2.01a",   0x00000, 0x08000, CRC(1bc323cf) SHA1(83fbd6e9b327700dc9f1c59700b7385bc3705749) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "sz2.02a",   0x28000, 0x20000, CRC(ba6a5013) SHA1(7814f3e56b69529b9860dd61c3b1e8d700244b03) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "sz2.11",   0x000000, 0x200000, CRC(aa47a601) SHA1(a4d1ee89c84a3b9db06469bb66e85293b5aa9ac9) )
	ROM_LOAD16_WORD_SWAP( "sz2.12",   0x200000, 0x200000, CRC(2237bc53) SHA1(96d5693047e4cf1ed10a8ee1905cea267a278e92) )
ROM_END

ROM_START( sfz2aj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "szaj.03a", 0x000000, 0x80000, CRC(a3802fe3) SHA1(c983a15ed675b22aebfe6ac55890b4e0b5eb8d48) )
	ROM_LOAD16_WORD_SWAP( "szaj.04a", 0x080000, 0x80000, CRC(e7ca87c7) SHA1(e44c930b27431dd2b983d93471a440d292e7a8bb) )
	ROM_LOAD16_WORD_SWAP( "szaj.05a", 0x100000, 0x80000, CRC(c88ebf88) SHA1(e37cf232fc70b9a3254dea99754e288232f04e25) )
	ROM_LOAD16_WORD_SWAP( "szaj.06a", 0x180000, 0x80000, CRC(35ed5b7a) SHA1(b03cb92f594eb35fa374445f74930e9040a2baff) )
	ROM_LOAD16_WORD_SWAP( "szaj.07a", 0x200000, 0x80000, CRC(975dcb3e) SHA1(a2ca8e5a768e49cce9e2137ec0dcba9337ed2ad5) )
	ROM_LOAD16_WORD_SWAP( "szaj.08a", 0x280000, 0x80000, CRC(dc73f2d7) SHA1(09fa10e7d1ff5f0dac87a6cf3d66730e3ab9ad25) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "szajx.03a", 0x000000, 0x80000, CRC(6d3aa71e) SHA1(f7827993639c7fe626f15dcb520de4d9ee361ca3) )
	ROM_LOAD16_WORD_SWAP( "szajx.04a", 0x080000, 0x80000, CRC(006d5cb8) SHA1(45773cb0b376faef5053603acca71ce577ada2aa) )

	ROM_REGION( 0x1400000, REGION_GFX1, 0 )
	ROMX_LOAD( "sz2.13",   0x0000000, 0x400000, CRC(4d1f1f22) SHA1(659fb4305bcf0cbbbbec97ede6e68a8323b13308) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.15",   0x0000002, 0x400000, CRC(19cea680) SHA1(4cb88963a0fbcef191c8419b6379387c01b4c81e) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.17",   0x0000004, 0x400000, CRC(e01b4588) SHA1(c2936608fd75ff6cd5fa94c6d6d6f0c77c44a450) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.19",   0x0000006, 0x400000, CRC(0feeda64) SHA1(f5b350601437bd94b70d97feb23d791df19da6b3) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.14",   0x1000000, 0x100000, CRC(0560c6aa) SHA1(f2bed3a8efef18052b51a7f0f6a888a18db813a1) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.16",   0x1000002, 0x100000, CRC(ae940f87) SHA1(39ee26333abbe302ba76dced0196a2e6b3b1d02a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.18",   0x1000004, 0x100000, CRC(4bc3c8bc) SHA1(6256963c515bf56f39b6e559afefd653ead56c54) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.20",   0x1000006, 0x100000, CRC(39e674c0) SHA1(8e771a2d8c2accad0463bccd21d7b23af0c895a1) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sz2.01a",   0x00000, 0x08000, CRC(1bc323cf) SHA1(83fbd6e9b327700dc9f1c59700b7385bc3705749) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "sz2.02a",   0x28000, 0x20000, CRC(ba6a5013) SHA1(7814f3e56b69529b9860dd61c3b1e8d700244b03) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "sz2.11",   0x000000, 0x200000, CRC(aa47a601) SHA1(a4d1ee89c84a3b9db06469bb66e85293b5aa9ac9) )
	ROM_LOAD16_WORD_SWAP( "sz2.12",   0x200000, 0x200000, CRC(2237bc53) SHA1(96d5693047e4cf1ed10a8ee1905cea267a278e92) )
ROM_END

ROM_START( sfz2aa )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "szaa.03", 0x000000, 0x80000, CRC(88e7023e) SHA1(34e74ec54c05d75e5cf207abb6e536fcca233b8b) )
	ROM_LOAD16_WORD_SWAP( "szaa.04", 0x080000, 0x80000, CRC(ae8ec36e) SHA1(b2f3de9e33169f6266aaabd5eae6c057ea10dcab) )
	ROM_LOAD16_WORD_SWAP( "szaa.05", 0x100000, 0x80000, CRC(f053a55e) SHA1(f98a8af5cd33a543a5596d59381f9adafed38854) )
	ROM_LOAD16_WORD_SWAP( "szaa.06", 0x180000, 0x80000, CRC(cfc0e7a8) SHA1(31ed58451c7a6ac88a8fccab369167694698f044) )
	ROM_LOAD16_WORD_SWAP( "szaa.07", 0x200000, 0x80000, CRC(5feb8b20) SHA1(13c79c9b72c3abf0a0b75d507d91ece71e460c06) )
	ROM_LOAD16_WORD_SWAP( "szaa.08", 0x280000, 0x80000, CRC(6eb6d412) SHA1(c858fec9c1dfea70dfcca629c1c24306f8ae6d81) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "szaax.03", 0x000000, 0x80000, CRC(a75f58bf) SHA1(82c940661992fb2537efba27f55897df3c80ac7a) )
	ROM_LOAD16_WORD_SWAP( "szaax.04", 0x080000, 0x80000, CRC(d02351ab) SHA1(5b53e4c50b854205a02e48aa7a670ec690d63c13) )

	ROM_REGION( 0x1400000, REGION_GFX1, 0 )
	ROMX_LOAD( "sz2.13",   0x0000000, 0x400000, CRC(4d1f1f22) SHA1(659fb4305bcf0cbbbbec97ede6e68a8323b13308) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.15",   0x0000002, 0x400000, CRC(19cea680) SHA1(4cb88963a0fbcef191c8419b6379387c01b4c81e) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.17",   0x0000004, 0x400000, CRC(e01b4588) SHA1(c2936608fd75ff6cd5fa94c6d6d6f0c77c44a450) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.19",   0x0000006, 0x400000, CRC(0feeda64) SHA1(f5b350601437bd94b70d97feb23d791df19da6b3) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.14",   0x1000000, 0x100000, CRC(0560c6aa) SHA1(f2bed3a8efef18052b51a7f0f6a888a18db813a1) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.16",   0x1000002, 0x100000, CRC(ae940f87) SHA1(39ee26333abbe302ba76dced0196a2e6b3b1d02a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.18",   0x1000004, 0x100000, CRC(4bc3c8bc) SHA1(6256963c515bf56f39b6e559afefd653ead56c54) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.20",   0x1000006, 0x100000, CRC(39e674c0) SHA1(8e771a2d8c2accad0463bccd21d7b23af0c895a1) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sz2.01a",   0x00000, 0x08000, CRC(1bc323cf) SHA1(83fbd6e9b327700dc9f1c59700b7385bc3705749) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "sz2.02a",   0x28000, 0x20000, CRC(ba6a5013) SHA1(7814f3e56b69529b9860dd61c3b1e8d700244b03) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "sz2.11",   0x000000, 0x200000, CRC(aa47a601) SHA1(a4d1ee89c84a3b9db06469bb66e85293b5aa9ac9) )
	ROM_LOAD16_WORD_SWAP( "sz2.12",   0x200000, 0x200000, CRC(2237bc53) SHA1(96d5693047e4cf1ed10a8ee1905cea267a278e92) )
ROM_END

ROM_START( sfz2ah )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "szah.03", 0x000000, 0x80000, CRC(06f93d1d) SHA1(495de176ba55b35270fc05f19edf17a0f249ff0e) )
	ROM_LOAD16_WORD_SWAP( "szah.04", 0x080000, 0x80000, CRC(e62ee914) SHA1(def4f27c1b64be5143234f1f402260adae66cdde) )
	ROM_LOAD16_WORD_SWAP( "szah.05", 0x100000, 0x80000, CRC(2b7f4b20) SHA1(5511263f5f6e532ee7fe1995f08f16651a1d45a1) )
	ROM_LOAD16_WORD_SWAP( "sza.06",  0x180000, 0x80000, CRC(0abda2fc) SHA1(830da40f6a9bb3bc866ee9c5cab1b0eb3c4dcb71) )
	ROM_LOAD16_WORD_SWAP( "sza.07",  0x200000, 0x80000, CRC(e9430762) SHA1(923aea8db5f9b59212ec6dbc35be0808ea015140) )
	ROM_LOAD16_WORD_SWAP( "sza.08",  0x280000, 0x80000, CRC(b65711a9) SHA1(3918f44e1bb189e2a115625b35f477eb91a65f04) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
/*	ROM_LOAD16_WORD_SWAP( "szahx.03", 0x000000, 0x80000, NO_DUMP )*/
/*	ROM_LOAD16_WORD_SWAP( "szahx.04", 0x080000, 0x80000, NO_DUMP )*/

	ROM_REGION( 0x1400000, REGION_GFX1, 0 )
	ROMX_LOAD( "sz2.13",   0x0000000, 0x400000, CRC(4d1f1f22) SHA1(659fb4305bcf0cbbbbec97ede6e68a8323b13308) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.15",   0x0000002, 0x400000, CRC(19cea680) SHA1(4cb88963a0fbcef191c8419b6379387c01b4c81e) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.17",   0x0000004, 0x400000, CRC(e01b4588) SHA1(c2936608fd75ff6cd5fa94c6d6d6f0c77c44a450) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.19",   0x0000006, 0x400000, CRC(0feeda64) SHA1(f5b350601437bd94b70d97feb23d791df19da6b3) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.14",   0x1000000, 0x100000, CRC(0560c6aa) SHA1(f2bed3a8efef18052b51a7f0f6a888a18db813a1) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.16",   0x1000002, 0x100000, CRC(ae940f87) SHA1(39ee26333abbe302ba76dced0196a2e6b3b1d02a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.18",   0x1000004, 0x100000, CRC(4bc3c8bc) SHA1(6256963c515bf56f39b6e559afefd653ead56c54) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.20",   0x1000006, 0x100000, CRC(39e674c0) SHA1(8e771a2d8c2accad0463bccd21d7b23af0c895a1) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sz2.01a",   0x00000, 0x08000, CRC(1bc323cf) SHA1(83fbd6e9b327700dc9f1c59700b7385bc3705749) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "sz2.02a",   0x28000, 0x20000, CRC(ba6a5013) SHA1(7814f3e56b69529b9860dd61c3b1e8d700244b03) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "sz2.11",   0x000000, 0x200000, CRC(aa47a601) SHA1(a4d1ee89c84a3b9db06469bb66e85293b5aa9ac9) )
	ROM_LOAD16_WORD_SWAP( "sz2.12",   0x200000, 0x200000, CRC(2237bc53) SHA1(96d5693047e4cf1ed10a8ee1905cea267a278e92) )
ROM_END

ROM_START( sfz2ab )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "szab.03", 0x000000, 0x80000, CRC(cb436eca) SHA1(406bea614429f78c0150c2f5042abc8673a6722e) )
	ROM_LOAD16_WORD_SWAP( "szab.04", 0x080000, 0x80000, CRC(14534bea) SHA1(8fff2cd9221ef12de9364cc15429b6df6bacc48e) )
	ROM_LOAD16_WORD_SWAP( "szab.05", 0x100000, 0x80000, CRC(7fb10658) SHA1(f9eba0271d92d6d29156a7b4dd8b1cdb3dd8aa48) )
	ROM_LOAD16_WORD_SWAP( "sza.06",  0x180000, 0x80000, CRC(0abda2fc) SHA1(830da40f6a9bb3bc866ee9c5cab1b0eb3c4dcb71) )
	ROM_LOAD16_WORD_SWAP( "sza.07",  0x200000, 0x80000, CRC(e9430762) SHA1(923aea8db5f9b59212ec6dbc35be0808ea015140) )
	ROM_LOAD16_WORD_SWAP( "sza.08",  0x280000, 0x80000, CRC(b65711a9) SHA1(3918f44e1bb189e2a115625b35f477eb91a65f04) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
/*	ROM_LOAD16_WORD_SWAP( "szabx.03", 0x000000, 0x80000, NO_DUMP )*/
/*	ROM_LOAD16_WORD_SWAP( "szabx.04", 0x080000, 0x80000, NO_DUMP )*/

	ROM_REGION( 0x1400000, REGION_GFX1, 0 )
	ROMX_LOAD( "sz2.13",   0x0000000, 0x400000, CRC(4d1f1f22) SHA1(659fb4305bcf0cbbbbec97ede6e68a8323b13308) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.15",   0x0000002, 0x400000, CRC(19cea680) SHA1(4cb88963a0fbcef191c8419b6379387c01b4c81e) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.17",   0x0000004, 0x400000, CRC(e01b4588) SHA1(c2936608fd75ff6cd5fa94c6d6d6f0c77c44a450) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.19",   0x0000006, 0x400000, CRC(0feeda64) SHA1(f5b350601437bd94b70d97feb23d791df19da6b3) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.14",   0x1000000, 0x100000, CRC(0560c6aa) SHA1(f2bed3a8efef18052b51a7f0f6a888a18db813a1) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.16",   0x1000002, 0x100000, CRC(ae940f87) SHA1(39ee26333abbe302ba76dced0196a2e6b3b1d02a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.18",   0x1000004, 0x100000, CRC(4bc3c8bc) SHA1(6256963c515bf56f39b6e559afefd653ead56c54) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.20",   0x1000006, 0x100000, CRC(39e674c0) SHA1(8e771a2d8c2accad0463bccd21d7b23af0c895a1) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sz2.01a",   0x00000, 0x08000, CRC(1bc323cf) SHA1(83fbd6e9b327700dc9f1c59700b7385bc3705749) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "sz2.02a",   0x28000, 0x20000, CRC(ba6a5013) SHA1(7814f3e56b69529b9860dd61c3b1e8d700244b03) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "sz2.11",   0x000000, 0x200000, CRC(aa47a601) SHA1(a4d1ee89c84a3b9db06469bb66e85293b5aa9ac9) )
	ROM_LOAD16_WORD_SWAP( "sz2.12",   0x200000, 0x200000, CRC(2237bc53) SHA1(96d5693047e4cf1ed10a8ee1905cea267a278e92) )
ROM_END

ROM_START( sfa3 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "sz3u.03c", 0x000000, 0x80000, CRC(e007da2e) SHA1(d190ac7ca2c27f11b9b4f96860b226bbea0ee403) )
	ROM_LOAD16_WORD_SWAP( "sz3u.04c", 0x080000, 0x80000, CRC(5f78f0e7) SHA1(f4df30fd3515fe9f1125f470b96028052c61f57b) )
	ROM_LOAD16_WORD_SWAP( "sz3.05c",  0x100000, 0x80000, CRC(57fd0a40) SHA1(bc2d5f4d57117bbf58b1adb088e00424ef489e92) )
	ROM_LOAD16_WORD_SWAP( "sz3.06c",  0x180000, 0x80000, CRC(f6305f8b) SHA1(3fd1ebdbad96103aca604e950b488e52460a71ec) )
	ROM_LOAD16_WORD_SWAP( "sz3.07c",  0x200000, 0x80000, CRC(6eab0f6f) SHA1(f8d093dda65cf4e8a3000dc1b96355bb03dcb495) )
	ROM_LOAD16_WORD_SWAP( "sz3.08c",  0x280000, 0x80000, CRC(910c4a3b) SHA1(dbd41280f9b16ad6a5b8f12092549970349395f1) )
	ROM_LOAD16_WORD_SWAP( "sz3.09c",  0x300000, 0x80000, CRC(b29e5199) SHA1(c6c215eb5aa37f678a9cafcbd8620969fb5ca12f) )
	ROM_LOAD16_WORD_SWAP( "sz3.10b",  0x380000, 0x80000, CRC(deb2ff52) SHA1(0aa4722aad68a04164946c78bf05752f947b4322) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "sz3ux.03c", 0x000000, 0x80000, CRC(7091276b) SHA1(3e101c5c1399a76196e08394420b4b332123063b) )
	ROM_LOAD16_WORD_SWAP( "sz3ux.04c", 0x080000, 0x80000, CRC(83b213b1) SHA1(d246ba1db1930ef03b00615575a51e78612a2118) )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "sz3.13",   0x0000000, 0x400000, CRC(0f7a60d9) SHA1(c69e0ee22537312909dacc86d2e4be319d54e426) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz3.15",   0x0000002, 0x400000, CRC(8e933741) SHA1(f4ac4bfe830dc7df9fe4f680e4e0c053e7cbd8fe) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz3.17",   0x0000004, 0x400000, CRC(d6e98147) SHA1(37f331fbb1284db446faecade6f484f58c0e1b2a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz3.19",   0x0000006, 0x400000, CRC(f31a728a) SHA1(f14136564648f006c1b74afda78349f260524b5f) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz3.14",   0x1000000, 0x400000, CRC(5ff98297) SHA1(9e0ce43380b776c7a03872bafd4856f6fa60bda7) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz3.16",   0x1000002, 0x400000, CRC(52b5bdee) SHA1(7918204dc457f7a146d8fb8cf7242dfed3109fd8) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz3.18",   0x1000004, 0x400000, CRC(40631ed5) SHA1(c18c56822b90a71ca5fbdf3440eb2671011f3d8f) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz3.20",   0x1000006, 0x400000, CRC(763409b4) SHA1(af60a5116c1ca9050366a35ea29128921867f3cc) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sz3.01",   0x00000, 0x08000, CRC(de810084) SHA1(fd0b969b732921ed8b40c16fbfa30ee09c7a7cbd) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "sz3.02",   0x28000, 0x20000, CRC(72445dc4) SHA1(14fca7596ac45ba655016eef5b6120f9f9671c23) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "sz3.11",   0x000000, 0x400000, CRC(1c89eed1) SHA1(649a0b0a3eb72e2e69e9fb1ac51a58b70daa39f3) )
	ROM_LOAD16_WORD_SWAP( "sz3.12",   0x400000, 0x400000, CRC(f392b13a) SHA1(fa04ce0370144a49bd1d5acd873eef87b0dc9d15) )
ROM_END

ROM_START( sfa3r1 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "sz3u.03", 0x000000, 0x80000, CRC(b5984a19) SHA1(e225dd1d3a5d1b94adcfc5f720775e9ba321996e) )
	ROM_LOAD16_WORD_SWAP( "sz3u.04", 0x080000, 0x80000, CRC(7e8158ba) SHA1(a9984d7c9d02a9ebaf98cfd0dcbcf26e82e904de) )
	ROM_LOAD16_WORD_SWAP( "sz3.05",  0x100000, 0x80000, CRC(9b21518a) SHA1(5a928307cb90a98a62e7598cb101fb66d62b85f9) )
	ROM_LOAD16_WORD_SWAP( "sz3.06",  0x180000, 0x80000, CRC(e7a6c3a7) SHA1(63441eb19efcbf9149f4b723d3e9191fa972de2a) )
	ROM_LOAD16_WORD_SWAP( "sz3.07",  0x200000, 0x80000, CRC(ec4c0cfd) SHA1(1a5148e77bf633c728a8179dacb59c776f981bc4) )
	ROM_LOAD16_WORD_SWAP( "sz3.08",  0x280000, 0x80000, CRC(5c7e7240) SHA1(33bdcdd1889f8fa77916373ed33b0854410d0263) )
	ROM_LOAD16_WORD_SWAP( "sz3.09",  0x300000, 0x80000, CRC(c5589553) SHA1(cda1fdc2ab2f390a2358defd9923a2796093926d) )
	ROM_LOAD16_WORD_SWAP( "sz3.10",  0x380000, 0x80000, CRC(a9717252) SHA1(7ee94ace2a49e4e5d30474e49c0da04a488010fe) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "sz3ux.03", 0x000000, 0x80000, CRC(42994875) SHA1(5d304d75cce15097681a33c7934e188097a8611d) )
	ROM_LOAD16_WORD_SWAP( "sz3ux.04", 0x080000, 0x80000, CRC(86ee79e3) SHA1(093c2759de7b06dde633c7a09fb8595045c2d950) )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "sz3.13",   0x0000000, 0x400000, CRC(0f7a60d9) SHA1(c69e0ee22537312909dacc86d2e4be319d54e426) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz3.15",   0x0000002, 0x400000, CRC(8e933741) SHA1(f4ac4bfe830dc7df9fe4f680e4e0c053e7cbd8fe) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz3.17",   0x0000004, 0x400000, CRC(d6e98147) SHA1(37f331fbb1284db446faecade6f484f58c0e1b2a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz3.19",   0x0000006, 0x400000, CRC(f31a728a) SHA1(f14136564648f006c1b74afda78349f260524b5f) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz3.14",   0x1000000, 0x400000, CRC(5ff98297) SHA1(9e0ce43380b776c7a03872bafd4856f6fa60bda7) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz3.16",   0x1000002, 0x400000, CRC(52b5bdee) SHA1(7918204dc457f7a146d8fb8cf7242dfed3109fd8) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz3.18",   0x1000004, 0x400000, CRC(40631ed5) SHA1(c18c56822b90a71ca5fbdf3440eb2671011f3d8f) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz3.20",   0x1000006, 0x400000, CRC(763409b4) SHA1(af60a5116c1ca9050366a35ea29128921867f3cc) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sz3.01",   0x00000, 0x08000, CRC(de810084) SHA1(fd0b969b732921ed8b40c16fbfa30ee09c7a7cbd) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "sz3.02",   0x28000, 0x20000, CRC(72445dc4) SHA1(14fca7596ac45ba655016eef5b6120f9f9671c23) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "sz3.11",   0x000000, 0x400000, CRC(1c89eed1) SHA1(649a0b0a3eb72e2e69e9fb1ac51a58b70daa39f3) )
	ROM_LOAD16_WORD_SWAP( "sz3.12",   0x400000, 0x400000, CRC(f392b13a) SHA1(fa04ce0370144a49bd1d5acd873eef87b0dc9d15) )
ROM_END

ROM_START( sfz3j )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "sz3j.03a", 0x000000, 0x80000, CRC(6ee0beae) SHA1(243436fb64628f70cd130c7029d365ae97f3f42d) )
	ROM_LOAD16_WORD_SWAP( "sz3j.04a", 0x080000, 0x80000, CRC(a6e2978d) SHA1(27e350e78aa204670c0ee6c60baddee46a92a584) )
	ROM_LOAD16_WORD_SWAP( "sz3.05a",  0x100000, 0x80000, CRC(05964b7d) SHA1(ac9fa2c69c712a01647f0572381d875b1eb90886) )
	ROM_LOAD16_WORD_SWAP( "sz3.06a",  0x180000, 0x80000, CRC(78ce2179) SHA1(98a6f55bbdc45167fcc04cd6c3b7d71ffab31911) )
	ROM_LOAD16_WORD_SWAP( "sz3.07a",  0x200000, 0x80000, CRC(398bf52f) SHA1(2c8880b65b83724b956294b903b5038091b543c5) )
	ROM_LOAD16_WORD_SWAP( "sz3.08a",  0x280000, 0x80000, CRC(866d0588) SHA1(f2e9ca1bb606e4d2e3c9b62dd80074670a2e8e45) )
	ROM_LOAD16_WORD_SWAP( "sz3.09a",  0x300000, 0x80000, CRC(2180892c) SHA1(65a44c612b1c6dd527b306c262caa5040897ce7b) )
	ROM_LOAD16_WORD_SWAP( "sz3.10",   0x380000, 0x80000, CRC(a9717252) SHA1(7ee94ace2a49e4e5d30474e49c0da04a488010fe) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "sz3jx.03a", 0x000000, 0x80000, CRC(b2f4046d) SHA1(c6f2149162bc70cb28a8894028ab01b4ccad51e7) )
	ROM_LOAD16_WORD_SWAP( "sz3jx.04a", 0x080000, 0x80000, CRC(85c38642) SHA1(82fab019daaef12476a5cf7130ec78fcb871b915) )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "sz3.13",   0x0000000, 0x400000, CRC(0f7a60d9) SHA1(c69e0ee22537312909dacc86d2e4be319d54e426) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz3.15",   0x0000002, 0x400000, CRC(8e933741) SHA1(f4ac4bfe830dc7df9fe4f680e4e0c053e7cbd8fe) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz3.17",   0x0000004, 0x400000, CRC(d6e98147) SHA1(37f331fbb1284db446faecade6f484f58c0e1b2a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz3.19",   0x0000006, 0x400000, CRC(f31a728a) SHA1(f14136564648f006c1b74afda78349f260524b5f) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz3.14",   0x1000000, 0x400000, CRC(5ff98297) SHA1(9e0ce43380b776c7a03872bafd4856f6fa60bda7) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz3.16",   0x1000002, 0x400000, CRC(52b5bdee) SHA1(7918204dc457f7a146d8fb8cf7242dfed3109fd8) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz3.18",   0x1000004, 0x400000, CRC(40631ed5) SHA1(c18c56822b90a71ca5fbdf3440eb2671011f3d8f) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz3.20",   0x1000006, 0x400000, CRC(763409b4) SHA1(af60a5116c1ca9050366a35ea29128921867f3cc) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sz3.01",   0x00000, 0x08000, CRC(de810084) SHA1(fd0b969b732921ed8b40c16fbfa30ee09c7a7cbd) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "sz3.02",   0x28000, 0x20000, CRC(72445dc4) SHA1(14fca7596ac45ba655016eef5b6120f9f9671c23) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "sz3.11",   0x000000, 0x400000, CRC(1c89eed1) SHA1(649a0b0a3eb72e2e69e9fb1ac51a58b70daa39f3) )
	ROM_LOAD16_WORD_SWAP( "sz3.12",   0x400000, 0x400000, CRC(f392b13a) SHA1(fa04ce0370144a49bd1d5acd873eef87b0dc9d15) )
ROM_END

ROM_START( sfz3jr1 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "sz3j.03", 0x000000, 0x80000, CRC(f7cb4b13) SHA1(5f86d23cf3725d9440200732405b437545ac8dd7) )
	ROM_LOAD16_WORD_SWAP( "sz3j.04", 0x080000, 0x80000, CRC(0846c29d) SHA1(f2e96b4f6e0187c382411276ff3a485cdc2df289) )
	ROM_LOAD16_WORD_SWAP( "sz3.05",  0x100000, 0x80000, CRC(9b21518a) SHA1(5a928307cb90a98a62e7598cb101fb66d62b85f9) )
	ROM_LOAD16_WORD_SWAP( "sz3.06",  0x180000, 0x80000, CRC(e7a6c3a7) SHA1(63441eb19efcbf9149f4b723d3e9191fa972de2a) )
	ROM_LOAD16_WORD_SWAP( "sz3.07",  0x200000, 0x80000, CRC(ec4c0cfd) SHA1(1a5148e77bf633c728a8179dacb59c776f981bc4) )
	ROM_LOAD16_WORD_SWAP( "sz3.08",  0x280000, 0x80000, CRC(5c7e7240) SHA1(33bdcdd1889f8fa77916373ed33b0854410d0263) )
	ROM_LOAD16_WORD_SWAP( "sz3.09",  0x300000, 0x80000, CRC(c5589553) SHA1(cda1fdc2ab2f390a2358defd9923a2796093926d) )
	ROM_LOAD16_WORD_SWAP( "sz3.10",  0x380000, 0x80000, CRC(a9717252) SHA1(7ee94ace2a49e4e5d30474e49c0da04a488010fe) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "sz3jx.03", 0x000000, 0x80000, CRC(acd88307) SHA1(bbe12ea38015497d4ef2cf067df112ce6a4a7c1b) )
	ROM_LOAD16_WORD_SWAP( "sz3jx.04", 0x080000, 0x80000, CRC(2c15655b) SHA1(3277c8fd9319a16b3c30b1690d7a7b19a2d94004) )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "sz3.13",   0x0000000, 0x400000, CRC(0f7a60d9) SHA1(c69e0ee22537312909dacc86d2e4be319d54e426) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz3.15",   0x0000002, 0x400000, CRC(8e933741) SHA1(f4ac4bfe830dc7df9fe4f680e4e0c053e7cbd8fe) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz3.17",   0x0000004, 0x400000, CRC(d6e98147) SHA1(37f331fbb1284db446faecade6f484f58c0e1b2a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz3.19",   0x0000006, 0x400000, CRC(f31a728a) SHA1(f14136564648f006c1b74afda78349f260524b5f) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz3.14",   0x1000000, 0x400000, CRC(5ff98297) SHA1(9e0ce43380b776c7a03872bafd4856f6fa60bda7) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz3.16",   0x1000002, 0x400000, CRC(52b5bdee) SHA1(7918204dc457f7a146d8fb8cf7242dfed3109fd8) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz3.18",   0x1000004, 0x400000, CRC(40631ed5) SHA1(c18c56822b90a71ca5fbdf3440eb2671011f3d8f) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz3.20",   0x1000006, 0x400000, CRC(763409b4) SHA1(af60a5116c1ca9050366a35ea29128921867f3cc) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sz3.01",   0x00000, 0x08000, CRC(de810084) SHA1(fd0b969b732921ed8b40c16fbfa30ee09c7a7cbd) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "sz3.02",   0x28000, 0x20000, CRC(72445dc4) SHA1(14fca7596ac45ba655016eef5b6120f9f9671c23) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "sz3.11",   0x000000, 0x400000, CRC(1c89eed1) SHA1(649a0b0a3eb72e2e69e9fb1ac51a58b70daa39f3) )
	ROM_LOAD16_WORD_SWAP( "sz3.12",   0x400000, 0x400000, CRC(f392b13a) SHA1(fa04ce0370144a49bd1d5acd873eef87b0dc9d15) )
ROM_END

ROM_START( sfz3a )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "sz3a.03a", 0x000000, 0x80000, CRC(29c681fd) SHA1(5ee4c4e282789e4cdba5a317c7049e8c0d8b774b) )
	ROM_LOAD16_WORD_SWAP( "sz3a.04",  0x080000, 0x80000, CRC(9ddd1484) SHA1(d484b93d1653f522ee33285e58139167b3214902) )
	ROM_LOAD16_WORD_SWAP( "sz3.05",   0x100000, 0x80000, CRC(9b21518a) SHA1(5a928307cb90a98a62e7598cb101fb66d62b85f9) )
	ROM_LOAD16_WORD_SWAP( "sz3.06",   0x180000, 0x80000, CRC(e7a6c3a7) SHA1(63441eb19efcbf9149f4b723d3e9191fa972de2a) )
	ROM_LOAD16_WORD_SWAP( "sz3.07",   0x200000, 0x80000, CRC(ec4c0cfd) SHA1(1a5148e77bf633c728a8179dacb59c776f981bc4) )
	ROM_LOAD16_WORD_SWAP( "sz3.08",   0x280000, 0x80000, CRC(5c7e7240) SHA1(33bdcdd1889f8fa77916373ed33b0854410d0263) )
	ROM_LOAD16_WORD_SWAP( "sz3.09",   0x300000, 0x80000, CRC(c5589553) SHA1(cda1fdc2ab2f390a2358defd9923a2796093926d) )
	ROM_LOAD16_WORD_SWAP( "sz3.10",   0x380000, 0x80000, CRC(a9717252) SHA1(7ee94ace2a49e4e5d30474e49c0da04a488010fe) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "sz3ax.03a", 0x000000, 0x80000, CRC(f3ad39ac) SHA1(70a8b1abf1a9fe4a4250122a0584ba5c214f9933) )
	ROM_LOAD16_WORD_SWAP( "sz3ax.04",  0x080000, 0x80000, CRC(792527ca) SHA1(8aef6a30267ba64e86c53ef2610575ec6050388e) )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "sz3.13",   0x0000000, 0x400000, CRC(0f7a60d9) SHA1(c69e0ee22537312909dacc86d2e4be319d54e426) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz3.15",   0x0000002, 0x400000, CRC(8e933741) SHA1(f4ac4bfe830dc7df9fe4f680e4e0c053e7cbd8fe) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz3.17",   0x0000004, 0x400000, CRC(d6e98147) SHA1(37f331fbb1284db446faecade6f484f58c0e1b2a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz3.19",   0x0000006, 0x400000, CRC(f31a728a) SHA1(f14136564648f006c1b74afda78349f260524b5f) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz3.14",   0x1000000, 0x400000, CRC(5ff98297) SHA1(9e0ce43380b776c7a03872bafd4856f6fa60bda7) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz3.16",   0x1000002, 0x400000, CRC(52b5bdee) SHA1(7918204dc457f7a146d8fb8cf7242dfed3109fd8) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz3.18",   0x1000004, 0x400000, CRC(40631ed5) SHA1(c18c56822b90a71ca5fbdf3440eb2671011f3d8f) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz3.20",   0x1000006, 0x400000, CRC(763409b4) SHA1(af60a5116c1ca9050366a35ea29128921867f3cc) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sz3.01",   0x00000, 0x08000, CRC(de810084) SHA1(fd0b969b732921ed8b40c16fbfa30ee09c7a7cbd) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "sz3.02",   0x28000, 0x20000, CRC(72445dc4) SHA1(14fca7596ac45ba655016eef5b6120f9f9671c23) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "sz3.11",   0x000000, 0x400000, CRC(1c89eed1) SHA1(649a0b0a3eb72e2e69e9fb1ac51a58b70daa39f3) )
	ROM_LOAD16_WORD_SWAP( "sz3.12",   0x400000, 0x400000, CRC(f392b13a) SHA1(fa04ce0370144a49bd1d5acd873eef87b0dc9d15) )
ROM_END

ROM_START( hsf2 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "hs2u.03",  0x000000, 0x80000, CRC(b308151e) SHA1(afdfd3b049c6435e2291bc35d8c26ff5bff223d8) )
	ROM_LOAD16_WORD_SWAP( "hs2u.04",  0x080000, 0x80000, CRC(327aa49c) SHA1(6719cd6ecc2a4487fdbf5cbcd47e35fc43000607) )
	ROM_LOAD16_WORD_SWAP( "hs2.05",   0x100000, 0x80000, CRC(dde34a35) SHA1(f5be2d2916db6e86e0886d61d55bddf138273ebc) )
	ROM_LOAD16_WORD_SWAP( "hs2.06",   0x180000, 0x80000, CRC(f4e56dda) SHA1(c6490707c2a416ab88612c2d73abbe5853d8cb92) )
	ROM_LOAD16_WORD_SWAP( "hs2.07",   0x200000, 0x80000, CRC(ee4420fc) SHA1(06cf76660b0c794d2460c52d9fe8334fff51e9de) )
	ROM_LOAD16_WORD_SWAP( "hs2.08",   0x280000, 0x80000, CRC(c9441533) SHA1(bf178fac1f060fcce3ff9118333c8517dadc9429) )
	ROM_LOAD16_WORD_SWAP( "hs2.09",   0x300000, 0x80000, CRC(3fc638a8) SHA1(2a42877b26c8abc437da46225701f0bba6e40058) )
	ROM_LOAD16_WORD_SWAP( "hs2.10",   0x380000, 0x80000, CRC(20d0f9e4) SHA1(80a5eeef9472e327b0d4ee26434bad109a9434ea) )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROM_LOAD64_WORD( "hs2.13m",   0x0000000, 0x800000, CRC(a6ecab17) SHA1(6749a4c8dc81f4b10f910c31c82cf6674e2a44eb) )
	ROM_LOAD64_WORD( "hs2.15m",   0x0000002, 0x800000, CRC(10a0ae4d) SHA1(701b4900fbc8bef20efa1a706891c8df4bf14641) )
	ROM_LOAD64_WORD( "hs2.17m",   0x0000004, 0x800000, CRC(adfa7726) SHA1(8d36ec125a8c91abfe5213893d794f8bc11c8acd) )
	ROM_LOAD64_WORD( "hs2.19m",   0x0000006, 0x800000, CRC(bb3ae322) SHA1(ecd289d7a0fe365fdd7c5527cb17796002beb553) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "hs2.01",   0x00000, 0x08000, CRC(c1a13786) SHA1(c7392c7efb15ea4042e75bd9007e974293d8935d) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "hs2.02",   0x28000, 0x20000, CRC(2d8794aa) SHA1(c634affdc2568020cce6af97b4fa79925d9943f3) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "hs2.11m",   0x000000, 0x800000, CRC(0e15c359) SHA1(176108b0d76d821a849324680aba0cd04b5016c1) )

	ROM_REGION( 0x20, REGION_USER5, 0 )
	ROM_LOAD( "hsf2.key",     0x000000, 0x000014, CRC(fc9b18c9) SHA1(457b6ba05458f1be9ebb8aa9cbab0a8808cb6857) )
ROM_END

ROM_START( hsf2a )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "hs2a.03",  0x000000, 0x80000, CRC(d50a17e0) SHA1(5d8d6d309260cc2d862aa080d44a72886ee08c77) )
	ROM_LOAD16_WORD_SWAP( "hs2a.04",  0x080000, 0x80000, CRC(a27f42de) SHA1(7a355831b57a35e327b2618fd5dca11afed2a233) )
	ROM_LOAD16_WORD_SWAP( "hs2.05",   0x100000, 0x80000, CRC(dde34a35) SHA1(f5be2d2916db6e86e0886d61d55bddf138273ebc) )
	ROM_LOAD16_WORD_SWAP( "hs2.06",   0x180000, 0x80000, CRC(f4e56dda) SHA1(c6490707c2a416ab88612c2d73abbe5853d8cb92) )
	ROM_LOAD16_WORD_SWAP( "hs2.07",   0x200000, 0x80000, CRC(ee4420fc) SHA1(06cf76660b0c794d2460c52d9fe8334fff51e9de) )
	ROM_LOAD16_WORD_SWAP( "hs2.08",   0x280000, 0x80000, CRC(c9441533) SHA1(bf178fac1f060fcce3ff9118333c8517dadc9429) )
	ROM_LOAD16_WORD_SWAP( "hs2.09",   0x300000, 0x80000, CRC(3fc638a8) SHA1(2a42877b26c8abc437da46225701f0bba6e40058) )
	ROM_LOAD16_WORD_SWAP( "hs2.10",   0x380000, 0x80000, CRC(20d0f9e4) SHA1(80a5eeef9472e327b0d4ee26434bad109a9434ea) )
	
	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROM_LOAD64_WORD( "hs2.13m",   0x0000000, 0x800000, CRC(a6ecab17) SHA1(6749a4c8dc81f4b10f910c31c82cf6674e2a44eb) )
	ROM_LOAD64_WORD( "hs2.15m",   0x0000002, 0x800000, CRC(10a0ae4d) SHA1(701b4900fbc8bef20efa1a706891c8df4bf14641) )
	ROM_LOAD64_WORD( "hs2.17m",   0x0000004, 0x800000, CRC(adfa7726) SHA1(8d36ec125a8c91abfe5213893d794f8bc11c8acd) )
	ROM_LOAD64_WORD( "hs2.19m",   0x0000006, 0x800000, CRC(bb3ae322) SHA1(ecd289d7a0fe365fdd7c5527cb17796002beb553) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "hs2.01",   0x00000, 0x08000, CRC(c1a13786) SHA1(c7392c7efb15ea4042e75bd9007e974293d8935d) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "hs2.02",   0x28000, 0x20000, CRC(2d8794aa) SHA1(c634affdc2568020cce6af97b4fa79925d9943f3) )
	
	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "hs2.11m",   0x000000, 0x800000, CRC(0e15c359) SHA1(176108b0d76d821a849324680aba0cd04b5016c1) )


	ROM_REGION( 0x20, REGION_USER5, 0 )
	ROM_LOAD( "hsf2a.key",    0x000000, 0x000014, CRC(2cd9eb99) SHA1(65f999569d8479bb8d27ba8c3e3c92f880b69195) )
ROM_END

ROM_START( hsf2j )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "hs2j.03c", 0x000000, 0x80000, CRC(6efe661f) SHA1(5e8df6183cc04ae0f3002f85ffedaa09b4397cd9) )
	ROM_LOAD16_WORD_SWAP( "hs2j.04b", 0x080000, 0x80000, CRC(93f2500a) SHA1(d340e8afd0a05066e19d795fdea31b4d1e00ad97) )
	ROM_LOAD16_WORD_SWAP( "hs2j.05",  0x100000, 0x80000, CRC(dde34a35) SHA1(f5be2d2916db6e86e0886d61d55bddf138273ebc) )
	ROM_LOAD16_WORD_SWAP( "hs2j.06",  0x180000, 0x80000, CRC(f4e56dda) SHA1(c6490707c2a416ab88612c2d73abbe5853d8cb92) )
	ROM_LOAD16_WORD_SWAP( "hs2j.07",  0x200000, 0x80000, CRC(ee4420fc) SHA1(06cf76660b0c794d2460c52d9fe8334fff51e9de) )
	ROM_LOAD16_WORD_SWAP( "hs2j.08",  0x280000, 0x80000, CRC(c9441533) SHA1(bf178fac1f060fcce3ff9118333c8517dadc9429) )
	ROM_LOAD16_WORD_SWAP( "hs2j.09",  0x300000, 0x80000, CRC(3fc638a8) SHA1(2a42877b26c8abc437da46225701f0bba6e40058) )
	ROM_LOAD16_WORD_SWAP( "hs2j.10",  0x380000, 0x80000, CRC(20d0f9e4) SHA1(80a5eeef9472e327b0d4ee26434bad109a9434ea) )
	
	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROM_LOAD64_WORD( "hs2.13m",   0x0000000, 0x800000, CRC(a6ecab17) SHA1(6749a4c8dc81f4b10f910c31c82cf6674e2a44eb) )
	ROM_LOAD64_WORD( "hs2.15m",   0x0000002, 0x800000, CRC(10a0ae4d) SHA1(701b4900fbc8bef20efa1a706891c8df4bf14641) )
	ROM_LOAD64_WORD( "hs2.17m",   0x0000004, 0x800000, CRC(adfa7726) SHA1(8d36ec125a8c91abfe5213893d794f8bc11c8acd) )
	ROM_LOAD64_WORD( "hs2.19m",   0x0000006, 0x800000, CRC(bb3ae322) SHA1(ecd289d7a0fe365fdd7c5527cb17796002beb553) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "hs2.01",   0x00000, 0x08000, CRC(c1a13786) SHA1(c7392c7efb15ea4042e75bd9007e974293d8935d) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "hs2.02",   0x28000, 0x20000, CRC(2d8794aa) SHA1(c634affdc2568020cce6af97b4fa79925d9943f3) )
	
	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "hs2.11m",   0x000000, 0x800000, CRC(0e15c359) SHA1(176108b0d76d821a849324680aba0cd04b5016c1) )


	ROM_REGION( 0x20, REGION_USER5, 0 )
	ROM_LOAD( "hsf2j.key",    0x000000, 0x000014, CRC(19455a93) SHA1(925cee29b37cb7a4048383ac715ad96f83280d37) )
ROM_END

ROM_START( hsf2j1 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "hs2j.03",  0x000000, 0x80000, CRC(00738f73) SHA1(99947a621f21a88dc4c425d9bfbc30b3c5f76ecd) )
	ROM_LOAD16_WORD_SWAP( "hs2j.04",  0x080000, 0x80000, CRC(40072c4a) SHA1(85b95bd3c907b4276a31777e092b8c40d3763257) )
	ROM_LOAD16_WORD_SWAP( "hs2.05",   0x100000, 0x80000, CRC(dde34a35) SHA1(f5be2d2916db6e86e0886d61d55bddf138273ebc) )
	ROM_LOAD16_WORD_SWAP( "hs2.06",   0x180000, 0x80000, CRC(f4e56dda) SHA1(c6490707c2a416ab88612c2d73abbe5853d8cb92) )
	ROM_LOAD16_WORD_SWAP( "hs2.07",   0x200000, 0x80000, CRC(ee4420fc) SHA1(06cf76660b0c794d2460c52d9fe8334fff51e9de) )
	ROM_LOAD16_WORD_SWAP( "hs2.08",   0x280000, 0x80000, CRC(c9441533) SHA1(bf178fac1f060fcce3ff9118333c8517dadc9429) )
	ROM_LOAD16_WORD_SWAP( "hs2.09",   0x300000, 0x80000, CRC(3fc638a8) SHA1(2a42877b26c8abc437da46225701f0bba6e40058) )
	ROM_LOAD16_WORD_SWAP( "hs2.10",   0x380000, 0x80000, CRC(20d0f9e4) SHA1(80a5eeef9472e327b0d4ee26434bad109a9434ea) )
	
	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROM_LOAD64_WORD( "hs2.13m",   0x0000000, 0x800000, CRC(a6ecab17) SHA1(6749a4c8dc81f4b10f910c31c82cf6674e2a44eb) )
	ROM_LOAD64_WORD( "hs2.15m",   0x0000002, 0x800000, CRC(10a0ae4d) SHA1(701b4900fbc8bef20efa1a706891c8df4bf14641) )
	ROM_LOAD64_WORD( "hs2.17m",   0x0000004, 0x800000, CRC(adfa7726) SHA1(8d36ec125a8c91abfe5213893d794f8bc11c8acd) )
	ROM_LOAD64_WORD( "hs2.19m",   0x0000006, 0x800000, CRC(bb3ae322) SHA1(ecd289d7a0fe365fdd7c5527cb17796002beb553) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "hs2.01",   0x00000, 0x08000, CRC(c1a13786) SHA1(c7392c7efb15ea4042e75bd9007e974293d8935d) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "hs2.02",   0x28000, 0x20000, CRC(2d8794aa) SHA1(c634affdc2568020cce6af97b4fa79925d9943f3) )
	
	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "hs2.11m",   0x000000, 0x800000, CRC(0e15c359) SHA1(176108b0d76d821a849324680aba0cd04b5016c1) )

	ROM_REGION( 0x20, REGION_USER5, 0 )
	ROM_LOAD( "hsf2j.key",    0x000000, 0x000014, CRC(19455a93) SHA1(925cee29b37cb7a4048383ac715ad96f83280d37) )
ROM_END

ROM_START( sgemf )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "pcfu.03", 0x000000, 0x80000, CRC(ac2e8566) SHA1(5975aae46bded231c0f478f40c7257434ade36b0) )
	ROM_LOAD16_WORD_SWAP( "pcf.04",  0x080000, 0x80000, CRC(f4314c96) SHA1(c40ed74039bf0096eb3648b7243a8e697638e0a6) )
	ROM_LOAD16_WORD_SWAP( "pcf.05",  0x100000, 0x80000, CRC(215655f6) SHA1(242c0f4401520f2a3b0deafc3a807b18b987e496) )
	ROM_LOAD16_WORD_SWAP( "pcf.06",  0x180000, 0x80000, CRC(ea6f13ea) SHA1(1bc924a8a9da1d2ad7667685cdb92fe317a39aba) )
	ROM_LOAD16_WORD_SWAP( "pcf.07",  0x200000, 0x80000, CRC(5ac6d5ea) SHA1(9ce8e4668b565658597a868830545fb75a5eeaa6) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "pcfux.03", 0x000000, 0x80000, CRC(652b7647) SHA1(03259f536e8c988b288de5e13df8b2ee22d8a168) )

	ROM_REGION( 0x1400000, REGION_GFX1, 0 )
	ROMX_LOAD( "pcf.13",   0x0000000, 0x400000, CRC(22d72ab9) SHA1(653efd95c34b4b9d2ab0d219f41a99ca84e12214) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pcf.15",   0x0000002, 0x400000, CRC(16a4813c) SHA1(bf5fce6008214f353414d1b64bea4ed0c7673670) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pcf.17",   0x0000004, 0x400000, CRC(1097e035) SHA1(4bd51e4e9447af27d2cac1f6d2201e37c949912b) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pcf.19",   0x0000006, 0x400000, CRC(d362d874) SHA1(30c42af18440496cc05e4418e4efa41172ae4ced) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pcf.14",   0x1000000, 0x100000, CRC(0383897c) SHA1(aba14afa1d0c971afcee4317f480e88117d77b5e) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pcf.16",   0x1000002, 0x100000, CRC(76f91084) SHA1(3d1e32467f2aa5dd6fb96bd5c866ecc9691660fc) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pcf.18",   0x1000004, 0x100000, CRC(756c3754) SHA1(be2f709b90222a567f198f851cf07ffb0ad433d7) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pcf.20",   0x1000006, 0x100000, CRC(9ec9277d) SHA1(b7ceeaca30dfcdf498b61a6961f0aa1a068b8ec4) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "pcf.01",   0x00000, 0x08000, CRC(254e5f33) SHA1(c413ec0630b9bdd15e64f42893eba8958a09b573) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "pcf.02",   0x28000, 0x20000, CRC(6902f4f9) SHA1(9bfe4ddade3c666076d26a2b545120f6d059fd7c) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "pcf.11",   0x000000, 0x400000, CRC(a5dea005) SHA1(3ae79baf6ff5bd527f82b26f164c7e3c65423ae2) )
	ROM_LOAD16_WORD_SWAP( "pcf.12",   0x400000, 0x400000, CRC(4ce235fe) SHA1(795b94557e954cc0e45fd3778b609064d57a34a2) )
ROM_END

ROM_START( pfghtj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "pcfj.03", 0x000000, 0x80000, CRC(681da43e) SHA1(1bd4b6b395ac7208c0208b254455276719e98c4b) )
	ROM_LOAD16_WORD_SWAP( "pcf.04",  0x080000, 0x80000, CRC(f4314c96) SHA1(c40ed74039bf0096eb3648b7243a8e697638e0a6) )
	ROM_LOAD16_WORD_SWAP( "pcf.05",  0x100000, 0x80000, CRC(215655f6) SHA1(242c0f4401520f2a3b0deafc3a807b18b987e496) )
	ROM_LOAD16_WORD_SWAP( "pcf.06",  0x180000, 0x80000, CRC(ea6f13ea) SHA1(1bc924a8a9da1d2ad7667685cdb92fe317a39aba) )
	ROM_LOAD16_WORD_SWAP( "pcf.07",  0x200000, 0x80000, CRC(5ac6d5ea) SHA1(9ce8e4668b565658597a868830545fb75a5eeaa6) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "pcfjx.03", 0x000000, 0x80000, CRC(a523f21e) SHA1(3452dcb7950a9b0df3d6b66088f3c17c0d6b1dc9) )

	ROM_REGION( 0x1400000, REGION_GFX1, 0 )
	ROMX_LOAD( "pcf.13",   0x0000000, 0x400000, CRC(22d72ab9) SHA1(653efd95c34b4b9d2ab0d219f41a99ca84e12214) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pcf.15",   0x0000002, 0x400000, CRC(16a4813c) SHA1(bf5fce6008214f353414d1b64bea4ed0c7673670) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pcf.17",   0x0000004, 0x400000, CRC(1097e035) SHA1(4bd51e4e9447af27d2cac1f6d2201e37c949912b) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pcf.19",   0x0000006, 0x400000, CRC(d362d874) SHA1(30c42af18440496cc05e4418e4efa41172ae4ced) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pcf.14",   0x1000000, 0x100000, CRC(0383897c) SHA1(aba14afa1d0c971afcee4317f480e88117d77b5e) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pcf.16",   0x1000002, 0x100000, CRC(76f91084) SHA1(3d1e32467f2aa5dd6fb96bd5c866ecc9691660fc) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pcf.18",   0x1000004, 0x100000, CRC(756c3754) SHA1(be2f709b90222a567f198f851cf07ffb0ad433d7) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pcf.20",   0x1000006, 0x100000, CRC(9ec9277d) SHA1(b7ceeaca30dfcdf498b61a6961f0aa1a068b8ec4) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "pcf.01",   0x00000, 0x08000, CRC(254e5f33) SHA1(c413ec0630b9bdd15e64f42893eba8958a09b573) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "pcf.02",   0x28000, 0x20000, CRC(6902f4f9) SHA1(9bfe4ddade3c666076d26a2b545120f6d059fd7c) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "pcf.11",   0x000000, 0x400000, CRC(a5dea005) SHA1(3ae79baf6ff5bd527f82b26f164c7e3c65423ae2) )
	ROM_LOAD16_WORD_SWAP( "pcf.12",   0x400000, 0x400000, CRC(4ce235fe) SHA1(795b94557e954cc0e45fd3778b609064d57a34a2) )
ROM_END

ROM_START( sgemfa )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "pcfa.03", 0x000000, 0x80000, CRC(e17c089a) SHA1(59529957aeb430df48a88414637e67848fdaaaca) )
	ROM_LOAD16_WORD_SWAP( "pcf.04",  0x080000, 0x80000, CRC(f4314c96) SHA1(c40ed74039bf0096eb3648b7243a8e697638e0a6) )
	ROM_LOAD16_WORD_SWAP( "pcf.05",  0x100000, 0x80000, CRC(215655f6) SHA1(242c0f4401520f2a3b0deafc3a807b18b987e496) )
	ROM_LOAD16_WORD_SWAP( "pcf.06",  0x180000, 0x80000, CRC(ea6f13ea) SHA1(1bc924a8a9da1d2ad7667685cdb92fe317a39aba) )
	ROM_LOAD16_WORD_SWAP( "pcf.07",  0x200000, 0x80000, CRC(5ac6d5ea) SHA1(9ce8e4668b565658597a868830545fb75a5eeaa6) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "pcfax.03", 0x000000, 0x80000, CRC(750fabe2) SHA1(4509945b4eab73552cb30ee329c0949b4c8fa797) )

	ROM_REGION( 0x1400000, REGION_GFX1, 0 )
	ROMX_LOAD( "pcf.13",   0x0000000, 0x400000, CRC(22d72ab9) SHA1(653efd95c34b4b9d2ab0d219f41a99ca84e12214) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pcf.15",   0x0000002, 0x400000, CRC(16a4813c) SHA1(bf5fce6008214f353414d1b64bea4ed0c7673670) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pcf.17",   0x0000004, 0x400000, CRC(1097e035) SHA1(4bd51e4e9447af27d2cac1f6d2201e37c949912b) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pcf.19",   0x0000006, 0x400000, CRC(d362d874) SHA1(30c42af18440496cc05e4418e4efa41172ae4ced) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pcf.14",   0x1000000, 0x100000, CRC(0383897c) SHA1(aba14afa1d0c971afcee4317f480e88117d77b5e) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pcf.16",   0x1000002, 0x100000, CRC(76f91084) SHA1(3d1e32467f2aa5dd6fb96bd5c866ecc9691660fc) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pcf.18",   0x1000004, 0x100000, CRC(756c3754) SHA1(be2f709b90222a567f198f851cf07ffb0ad433d7) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pcf.20",   0x1000006, 0x100000, CRC(9ec9277d) SHA1(b7ceeaca30dfcdf498b61a6961f0aa1a068b8ec4) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "pcf.01",   0x00000, 0x08000, CRC(254e5f33) SHA1(c413ec0630b9bdd15e64f42893eba8958a09b573) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "pcf.02",   0x28000, 0x20000, CRC(6902f4f9) SHA1(9bfe4ddade3c666076d26a2b545120f6d059fd7c) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "pcf.11",   0x000000, 0x400000, CRC(a5dea005) SHA1(3ae79baf6ff5bd527f82b26f164c7e3c65423ae2) )
	ROM_LOAD16_WORD_SWAP( "pcf.12",   0x400000, 0x400000, CRC(4ce235fe) SHA1(795b94557e954cc0e45fd3778b609064d57a34a2) )
ROM_END

ROM_START( sgemfh )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "pcfh.03", 0x000000, 0x80000, CRC(e9103347) SHA1(7a32a151146a15bf5fb5ed993fee2f616077a58c) )
	ROM_LOAD16_WORD_SWAP( "pcf.04",  0x080000, 0x80000, CRC(f4314c96) SHA1(c40ed74039bf0096eb3648b7243a8e697638e0a6) )
	ROM_LOAD16_WORD_SWAP( "pcf.05",  0x100000, 0x80000, CRC(215655f6) SHA1(242c0f4401520f2a3b0deafc3a807b18b987e496) )
	ROM_LOAD16_WORD_SWAP( "pcf.06",  0x180000, 0x80000, CRC(ea6f13ea) SHA1(1bc924a8a9da1d2ad7667685cdb92fe317a39aba) )
	ROM_LOAD16_WORD_SWAP( "pcf.07",  0x200000, 0x80000, CRC(5ac6d5ea) SHA1(9ce8e4668b565658597a868830545fb75a5eeaa6) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
/*	ROM_LOAD16_WORD_SWAP( "pcfhx.03", 0x000000, 0x80000, NO_DUMP )*/

	ROM_REGION( 0x1400000, REGION_GFX1, 0 )
	ROMX_LOAD( "pcf.13",   0x0000000, 0x400000, CRC(22d72ab9) SHA1(653efd95c34b4b9d2ab0d219f41a99ca84e12214) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pcf.15",   0x0000002, 0x400000, CRC(16a4813c) SHA1(bf5fce6008214f353414d1b64bea4ed0c7673670) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pcf.17",   0x0000004, 0x400000, CRC(1097e035) SHA1(4bd51e4e9447af27d2cac1f6d2201e37c949912b) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pcf.19",   0x0000006, 0x400000, CRC(d362d874) SHA1(30c42af18440496cc05e4418e4efa41172ae4ced) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pcf.14",   0x1000000, 0x100000, CRC(0383897c) SHA1(aba14afa1d0c971afcee4317f480e88117d77b5e) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pcf.16",   0x1000002, 0x100000, CRC(76f91084) SHA1(3d1e32467f2aa5dd6fb96bd5c866ecc9691660fc) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pcf.18",   0x1000004, 0x100000, CRC(756c3754) SHA1(be2f709b90222a567f198f851cf07ffb0ad433d7) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pcf.20",   0x1000006, 0x100000, CRC(9ec9277d) SHA1(b7ceeaca30dfcdf498b61a6961f0aa1a068b8ec4) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "pcf.01",   0x00000, 0x08000, CRC(254e5f33) SHA1(c413ec0630b9bdd15e64f42893eba8958a09b573) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "pcf.02",   0x28000, 0x20000, CRC(6902f4f9) SHA1(9bfe4ddade3c666076d26a2b545120f6d059fd7c) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "pcf.11",   0x000000, 0x400000, CRC(a5dea005) SHA1(3ae79baf6ff5bd527f82b26f164c7e3c65423ae2) )
	ROM_LOAD16_WORD_SWAP( "pcf.12",   0x400000, 0x400000, CRC(4ce235fe) SHA1(795b94557e954cc0e45fd3778b609064d57a34a2) )
ROM_END

ROM_START( spf2t )
	ROM_REGION(CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "pzfu.03a", 0x000000, 0x80000, CRC(346e62ef) SHA1(9db5ea0aac2d459be957f8b6e2e0d18421587d4d) )
	ROM_LOAD16_WORD_SWAP( "pzf.04a",  0x080000, 0x80000, CRC(b80649e2) SHA1(5bfccd656aea7ff82e9a20bb5856f4ab99b5a007) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "pzfux.03a", 0x000000, 0x80000, CRC(200feea6) SHA1(6f549caa9d7456b8ddc59739be87a1b2c0cc844e) )

	ROM_REGION( 0xC00000, REGION_GFX1, 0 )
	ROM_FILL(             0x000000, 0x800000, 0 )
	ROMX_LOAD( "pzf.14",  0x800000, 0x100000, CRC(2d4881cb) SHA1(fd3baa183c25bed153b19c251980e2fb761600e2) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pzf.16",  0x800002, 0x100000, CRC(4b0fd1be) SHA1(377aafdcdb7a866b1c8487670e3598d8197976e4) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pzf.18",  0x800004, 0x100000, CRC(e43aac33) SHA1(d041e0688c3807d3363861a7f216de43b34d846c) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pzf.20",  0x800006, 0x100000, CRC(7f536ff1) SHA1(905b9d62ef7bef47297c7f4a4dd697aed6df38a5) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION(QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "pzf.01",   0x00000, 0x08000, CRC(600fb2a3) SHA1(1fab1c2a23bf6ad8309d29ddbbc29435a8aeea13) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "pzf.02",   0x28000, 0x20000, CRC(496076e0) SHA1(1ee4e135140afd0e8e03231e570cd77d140f6367) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "pzf.11",   0x000000, 0x200000, CRC(78442743) SHA1(b61190bb586871de6d54af580e3e1d9cc0de0acb) )
	ROM_LOAD16_WORD_SWAP( "pzf.12",   0x200000, 0x200000, CRC(399d2c7b) SHA1(e849dea97b8d16540415c0d9bbc4f9f4eb755ec4) )
ROM_END

ROM_START( spf2xj )
	ROM_REGION(CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "pzfj.03a", 0x000000, 0x80000, CRC(2070554a) SHA1(fa818e6bd2e11667345d3d8f2397b60802ef72f9) )
	ROM_LOAD16_WORD_SWAP( "pzf.04a",  0x080000, 0x80000, CRC(b80649e2) SHA1(5bfccd656aea7ff82e9a20bb5856f4ab99b5a007) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "pzfjx.03a", 0x000000, 0x80000, CRC(c2e3f231) SHA1(094054502e42c8a5347cc1d8d914bef786328b5a) )

	ROM_REGION( 0xC00000, REGION_GFX1, 0 )
	ROM_FILL(             0x000000, 0x800000, 0 )
	ROMX_LOAD( "pzf.14",  0x800000, 0x100000, CRC(2d4881cb) SHA1(fd3baa183c25bed153b19c251980e2fb761600e2) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pzf.16",  0x800002, 0x100000, CRC(4b0fd1be) SHA1(377aafdcdb7a866b1c8487670e3598d8197976e4) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pzf.18",  0x800004, 0x100000, CRC(e43aac33) SHA1(d041e0688c3807d3363861a7f216de43b34d846c) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pzf.20",  0x800006, 0x100000, CRC(7f536ff1) SHA1(905b9d62ef7bef47297c7f4a4dd697aed6df38a5) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION(QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "pzf.01",   0x00000, 0x08000, CRC(600fb2a3) SHA1(1fab1c2a23bf6ad8309d29ddbbc29435a8aeea13) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "pzf.02",   0x28000, 0x20000, CRC(496076e0) SHA1(1ee4e135140afd0e8e03231e570cd77d140f6367) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "pzf.11",   0x000000, 0x200000, CRC(78442743) SHA1(b61190bb586871de6d54af580e3e1d9cc0de0acb) )
	ROM_LOAD16_WORD_SWAP( "pzf.12",   0x200000, 0x200000, CRC(399d2c7b) SHA1(e849dea97b8d16540415c0d9bbc4f9f4eb755ec4) )
ROM_END

ROM_START( ssf2 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "ssfe.03",  0x000000, 0x80000, CRC(a597745d) SHA1(5b12e09c14f0ea93b668b97ca2d27a686c85f641) )
	ROM_LOAD16_WORD_SWAP( "ssfe.04",  0x080000, 0x80000, CRC(b082aa67) SHA1(ca26b4bb1947cb30eaf6b61f606b859d18da4c4c) )
	ROM_LOAD16_WORD_SWAP( "ssfe.05",  0x100000, 0x80000, CRC(02b9c137) SHA1(ba624441e1b4bfb67c71f6a116fe43539eaa4a15) )
	ROM_LOAD16_WORD_SWAP( "ssfe.06",  0x180000, 0x80000, CRC(70d470c5) SHA1(ba03c8f4c76f72f4483e91547e03d1a0cf6db485) )
	ROM_LOAD16_WORD_SWAP( "ssfe.07",  0x200000, 0x80000, CRC(2409001d) SHA1(f532ebb2efbb8f8ba311d10ff897490352c87f97) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "ssfex.03",  0x000000, 0x80000, CRC(29690c24) SHA1(b42ed9344c56c59a0a3e85a6dc1ac415ad55a5b3) )
	ROM_LOAD16_WORD_SWAP( "ssfex.04",  0x080000, 0x80000, CRC(d001e53a) SHA1(8585c9f829692ce914a7a557e7d5c1093c613320) )
	ROM_LOAD16_WORD_SWAP( "ssfex.05",  0x100000, 0x80000, CRC(a45602e4) SHA1(ae10d898d4f8973657c76a8b7e3c790a7f33fc22) )
	ROM_LOAD16_WORD_SWAP( "ssfex.06",  0x180000, 0x80000, CRC(ae3c8a14) SHA1(52352d758f0d5d6df69283751c6376f2568e98ed) )
	ROM_LOAD16_WORD_SWAP( "ssfex.07",  0x200000, 0x80000, CRC(f88f584e) SHA1(3e8280ec13d5f72f133d893f439775763d510346) )

	ROM_REGION( 0xc00000, REGION_GFX1, 0 )
	ROMX_LOAD( "ssf.13",   0x000000, 0x200000, CRC(cf94d275) SHA1(bf2a6d98a656d1cb5734da7836686242d3211137) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.15",   0x000002, 0x200000, CRC(5eb703af) SHA1(4b302dbb66e8a5c2ad92798699391e981bada427) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.17",   0x000004, 0x200000, CRC(ffa60e0f) SHA1(b21b1c749a8241440879bf8e7cb33968ccef97e5) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.19",   0x000006, 0x200000, CRC(34e825c5) SHA1(4d320fc96d1ef0b9928a8ce801734245a4c097a5) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.14",   0x800000, 0x100000, CRC(b7cc32e7) SHA1(0f4d26af338dab5dce5b7b34d32ad0c573434ace) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.16",   0x800002, 0x100000, CRC(8376ad18) SHA1(f4456833fb396e6501f4174c0fe5fd63ea40a188) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.18",   0x800004, 0x100000, CRC(f5b1b336) SHA1(4b060501e56b9d61294748da5387cdae5280ec4d) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.20",   0x800006, 0x100000, CRC(459d5c6b) SHA1(32b11ba7a12004aff810d719bff7508204c7b7c0) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "ssf.01",   0x00000, 0x08000, CRC(eb247e8c) SHA1(24296c18d9b1136d69712bf1c9d9d15463041e83) )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD( "ssf.q01",  0x000000, 0x080000, CRC(a6f9da5c) SHA1(6d19f83a01bd25b838d5c2871f7964529d926c98) )
	ROM_LOAD( "ssf.q02",  0x080000, 0x080000, CRC(8c66ae26) SHA1(32a82aee6ed4480e5a990f9af161734c7c0a1403) )
	ROM_LOAD( "ssf.q03",  0x100000, 0x080000, CRC(695cc2ca) SHA1(c2675f0233608b76de528d2a6ef19846d1348060) )
	ROM_LOAD( "ssf.q04",  0x180000, 0x080000, CRC(9d9ebe32) SHA1(9b26329370041374f1a90b479a172d2bc2801c4d) )
	ROM_LOAD( "ssf.q05",  0x200000, 0x080000, CRC(4770e7b7) SHA1(0e764f0befb9227b0b36508d8ca8ec9be31bcb05) )
	ROM_LOAD( "ssf.q06",  0x280000, 0x080000, CRC(4e79c951) SHA1(1144781d7dc57ef8a6458d982f5c91548ff59e27) )
	ROM_LOAD( "ssf.q07",  0x300000, 0x080000, CRC(cdd14313) SHA1(ebe767a9d4b51dba4282fa0a36a546a88620dd59) )
	ROM_LOAD( "ssf.q08",  0x380000, 0x080000, CRC(6f5a088c) SHA1(6c0b4690479647a99d76335f174be8455b4ff118) )
ROM_END

ROM_START( ssf2u )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "ssfu.03a", 0x000000, 0x80000, CRC(72f29c33) SHA1(c24769ca9568d4f6847979929b2a059e57dae6b3) )
	ROM_LOAD16_WORD_SWAP( "ssfu.04a", 0x080000, 0x80000, CRC(935cea44) SHA1(1360254debf179919def1485b5758f529c94f65a) )
	ROM_LOAD16_WORD_SWAP( "ssfu.05",  0x100000, 0x80000, CRC(a0acb28a) SHA1(55c0c0ea9b9e6ef8d7c12f888cf42b6418bbf82e) )
	ROM_LOAD16_WORD_SWAP( "ssfu.06",  0x180000, 0x80000, CRC(47413dcf) SHA1(1a94e38ee899e6356ad22bde4f85e99dd3b6a934) )
	ROM_LOAD16_WORD_SWAP( "ssfu.07",  0x200000, 0x80000, CRC(e6066077) SHA1(889e2cad30b16bfaf0c54f3a38d04dd02deac6f9) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "ssfux.03a", 0x000000, 0x80000, CRC(ec278279) SHA1(c5548704e10f0278673388d944205749d1c7522d) )
	ROM_LOAD16_WORD_SWAP( "ssfux.04a", 0x080000, 0x80000, CRC(6194d896) SHA1(c68665233d1ce360327bf11f66eea66f5058e964) )
	ROM_LOAD16_WORD_SWAP( "ssfux.05",  0x100000, 0x80000, CRC(aa846b9f) SHA1(4f0b5d849ac4ce097ee63570b023ffd9769c92e5) )
	ROM_LOAD16_WORD_SWAP( "ssfux.06",  0x180000, 0x80000, CRC(235268c4) SHA1(30942da5b83bb410210adcc212cb99868b2475be) )
	ROM_LOAD16_WORD_SWAP( "ssfux.07",  0x200000, 0x80000, CRC(e46e033c) SHA1(5891b42dce7ff48a6443836a9808e64907ad3be7) )

	ROM_REGION( 0xc00000, REGION_GFX1, 0 )
	ROMX_LOAD( "ssf.13",   0x000000, 0x200000, CRC(cf94d275) SHA1(bf2a6d98a656d1cb5734da7836686242d3211137) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.15",   0x000002, 0x200000, CRC(5eb703af) SHA1(4b302dbb66e8a5c2ad92798699391e981bada427) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.17",   0x000004, 0x200000, CRC(ffa60e0f) SHA1(b21b1c749a8241440879bf8e7cb33968ccef97e5) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.19",   0x000006, 0x200000, CRC(34e825c5) SHA1(4d320fc96d1ef0b9928a8ce801734245a4c097a5) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.14",   0x800000, 0x100000, CRC(b7cc32e7) SHA1(0f4d26af338dab5dce5b7b34d32ad0c573434ace) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.16",   0x800002, 0x100000, CRC(8376ad18) SHA1(f4456833fb396e6501f4174c0fe5fd63ea40a188) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.18",   0x800004, 0x100000, CRC(f5b1b336) SHA1(4b060501e56b9d61294748da5387cdae5280ec4d) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.20",   0x800006, 0x100000, CRC(459d5c6b) SHA1(32b11ba7a12004aff810d719bff7508204c7b7c0) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "ssf.01",   0x00000, 0x08000, CRC(eb247e8c) SHA1(24296c18d9b1136d69712bf1c9d9d15463041e83) )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD( "ssf.q01",  0x000000, 0x080000, CRC(a6f9da5c) SHA1(6d19f83a01bd25b838d5c2871f7964529d926c98) )
	ROM_LOAD( "ssf.q02",  0x080000, 0x080000, CRC(8c66ae26) SHA1(32a82aee6ed4480e5a990f9af161734c7c0a1403) )
	ROM_LOAD( "ssf.q03",  0x100000, 0x080000, CRC(695cc2ca) SHA1(c2675f0233608b76de528d2a6ef19846d1348060) )
	ROM_LOAD( "ssf.q04",  0x180000, 0x080000, CRC(9d9ebe32) SHA1(9b26329370041374f1a90b479a172d2bc2801c4d) )
	ROM_LOAD( "ssf.q05",  0x200000, 0x080000, CRC(4770e7b7) SHA1(0e764f0befb9227b0b36508d8ca8ec9be31bcb05) )
	ROM_LOAD( "ssf.q06",  0x280000, 0x080000, CRC(4e79c951) SHA1(1144781d7dc57ef8a6458d982f5c91548ff59e27) )
	ROM_LOAD( "ssf.q07",  0x300000, 0x080000, CRC(cdd14313) SHA1(ebe767a9d4b51dba4282fa0a36a546a88620dd59) )
	ROM_LOAD( "ssf.q08",  0x380000, 0x080000, CRC(6f5a088c) SHA1(6c0b4690479647a99d76335f174be8455b4ff118) )
ROM_END

ROM_START( ssf2a )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "ssfa.03b", 0x000000, 0x80000, CRC(83a059bf) SHA1(3279a792fb884f856cd5bac59eaae7d9e3be286c) )
	ROM_LOAD16_WORD_SWAP( "ssfa.04a", 0x080000, 0x80000, CRC(5d873642) SHA1(74e3541ed586454a8b56e331bc9ffdb8d69f7983) )
	ROM_LOAD16_WORD_SWAP( "ssfa.05",  0x100000, 0x80000, CRC(f8fb4de2) SHA1(e3cde329405d4d59b7c234a30a7c178afb22deef) )
	ROM_LOAD16_WORD_SWAP( "ssfa.06b", 0x180000, 0x80000, CRC(3185d19d) SHA1(9a354b0ee6243a3aaaa0027cce438dcfd9f93a74) )
	ROM_LOAD16_WORD_SWAP( "ssfa.07",  0x200000, 0x80000, CRC(36e29217) SHA1(86563b42676c923c6e3d760e22621e687de3a991) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "ssfax.03b", 0x000000, 0x80000, CRC(903b997e) SHA1(07edd2787f76a29f183a9608f08e282c268943a3) )
	ROM_LOAD16_WORD_SWAP( "ssfax.04a", 0x080000, 0x80000, CRC(99d5e67f) SHA1(4b18cb60125b1c837a5fad57f29d3a58ee93f94d) )
	ROM_LOAD16_WORD_SWAP( "ssfax.05",  0x100000, 0x80000, CRC(d104f415) SHA1(97aa8a1e93aec097bff3ba2873084b2f23bf4e5b) )
	ROM_LOAD16_WORD_SWAP( "ssfax.06b", 0x180000, 0x80000, CRC(16516200) SHA1(48ec25c7fcbb0758e52d98bf1d5d602b67964e7b) )
	ROM_LOAD16_WORD_SWAP( "ssfax.07",  0x200000, 0x80000, CRC(ca49b7f6) SHA1(9b6291259b12a39ceec13109923f2f8c139db0f8) )

	ROM_REGION( 0xc00000, REGION_GFX1, 0 )
	ROMX_LOAD( "ssf.13",   0x000000, 0x200000, CRC(cf94d275) SHA1(bf2a6d98a656d1cb5734da7836686242d3211137) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.15",   0x000002, 0x200000, CRC(5eb703af) SHA1(4b302dbb66e8a5c2ad92798699391e981bada427) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.17",   0x000004, 0x200000, CRC(ffa60e0f) SHA1(b21b1c749a8241440879bf8e7cb33968ccef97e5) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.19",   0x000006, 0x200000, CRC(34e825c5) SHA1(4d320fc96d1ef0b9928a8ce801734245a4c097a5) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.14",   0x800000, 0x100000, CRC(b7cc32e7) SHA1(0f4d26af338dab5dce5b7b34d32ad0c573434ace) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.16",   0x800002, 0x100000, CRC(8376ad18) SHA1(f4456833fb396e6501f4174c0fe5fd63ea40a188) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.18",   0x800004, 0x100000, CRC(f5b1b336) SHA1(4b060501e56b9d61294748da5387cdae5280ec4d) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.20",   0x800006, 0x100000, CRC(459d5c6b) SHA1(32b11ba7a12004aff810d719bff7508204c7b7c0) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "ssf.01",   0x00000, 0x08000, CRC(eb247e8c) SHA1(24296c18d9b1136d69712bf1c9d9d15463041e83) )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD( "ssf.q01",  0x000000, 0x080000, CRC(a6f9da5c) SHA1(6d19f83a01bd25b838d5c2871f7964529d926c98) )
	ROM_LOAD( "ssf.q02",  0x080000, 0x080000, CRC(8c66ae26) SHA1(32a82aee6ed4480e5a990f9af161734c7c0a1403) )
	ROM_LOAD( "ssf.q03",  0x100000, 0x080000, CRC(695cc2ca) SHA1(c2675f0233608b76de528d2a6ef19846d1348060) )
	ROM_LOAD( "ssf.q04",  0x180000, 0x080000, CRC(9d9ebe32) SHA1(9b26329370041374f1a90b479a172d2bc2801c4d) )
	ROM_LOAD( "ssf.q05",  0x200000, 0x080000, CRC(4770e7b7) SHA1(0e764f0befb9227b0b36508d8ca8ec9be31bcb05) )
	ROM_LOAD( "ssf.q06",  0x280000, 0x080000, CRC(4e79c951) SHA1(1144781d7dc57ef8a6458d982f5c91548ff59e27) )
	ROM_LOAD( "ssf.q07",  0x300000, 0x080000, CRC(cdd14313) SHA1(ebe767a9d4b51dba4282fa0a36a546a88620dd59) )
	ROM_LOAD( "ssf.q08",  0x380000, 0x080000, CRC(6f5a088c) SHA1(6c0b4690479647a99d76335f174be8455b4ff118) )
ROM_END

ROM_START( ssf2ar1 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "ssfa.03a", 0x000000, 0x80000, CRC(d2a3c520) SHA1(514131f0d8a7c6b5bf68630250e4b1b5983d490d) )
	ROM_LOAD16_WORD_SWAP( "ssfa.04a", 0x080000, 0x80000, CRC(5d873642) SHA1(74e3541ed586454a8b56e331bc9ffdb8d69f7983) )
	ROM_LOAD16_WORD_SWAP( "ssfa.05",  0x100000, 0x80000, CRC(f8fb4de2) SHA1(e3cde329405d4d59b7c234a30a7c178afb22deef) )
	ROM_LOAD16_WORD_SWAP( "ssfa.06", 0x180000, 0x80000, CRC(aa8acee7) SHA1(e696b0391e41728f0cc7f190681c5fa7c96a3f81) )
	ROM_LOAD16_WORD_SWAP( "ssfa.07",  0x200000, 0x80000, CRC(36e29217) SHA1(86563b42676c923c6e3d760e22621e687de3a991) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "ssfax.03a", 0x000000, 0x80000, CRC(d628422c) SHA1(5e76205cc92f1c8456fedb150bf43ce5ad27ff1c) )
	ROM_LOAD16_WORD_SWAP( "ssfax.04a", 0x080000, 0x80000, CRC(99d5e67f) SHA1(4b18cb60125b1c837a5fad57f29d3a58ee93f94d) )
	ROM_LOAD16_WORD_SWAP( "ssfax.05",  0x100000, 0x80000, CRC(d104f415) SHA1(97aa8a1e93aec097bff3ba2873084b2f23bf4e5b) )
	ROM_LOAD16_WORD_SWAP( "ssfax.06",  0x180000, 0x80000, CRC(210e787c) SHA1(737a3d1acf954cd32732adeea404b60a697c8b21) )
	ROM_LOAD16_WORD_SWAP( "ssfax.07",  0x200000, 0x80000, CRC(ca49b7f6) SHA1(9b6291259b12a39ceec13109923f2f8c139db0f8) )

	ROM_REGION( 0xc00000, REGION_GFX1, 0 )
	ROMX_LOAD( "ssf.13",   0x000000, 0x200000, CRC(cf94d275) SHA1(bf2a6d98a656d1cb5734da7836686242d3211137) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.15",   0x000002, 0x200000, CRC(5eb703af) SHA1(4b302dbb66e8a5c2ad92798699391e981bada427) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.17",   0x000004, 0x200000, CRC(ffa60e0f) SHA1(b21b1c749a8241440879bf8e7cb33968ccef97e5) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.19",   0x000006, 0x200000, CRC(34e825c5) SHA1(4d320fc96d1ef0b9928a8ce801734245a4c097a5) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.14",   0x800000, 0x100000, CRC(b7cc32e7) SHA1(0f4d26af338dab5dce5b7b34d32ad0c573434ace) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.16",   0x800002, 0x100000, CRC(8376ad18) SHA1(f4456833fb396e6501f4174c0fe5fd63ea40a188) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.18",   0x800004, 0x100000, CRC(f5b1b336) SHA1(4b060501e56b9d61294748da5387cdae5280ec4d) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.20",   0x800006, 0x100000, CRC(459d5c6b) SHA1(32b11ba7a12004aff810d719bff7508204c7b7c0) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "ssf.01",   0x00000, 0x08000, CRC(eb247e8c) SHA1(24296c18d9b1136d69712bf1c9d9d15463041e83) )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD( "ssf.q01",  0x000000, 0x080000, CRC(a6f9da5c) SHA1(6d19f83a01bd25b838d5c2871f7964529d926c98) )
	ROM_LOAD( "ssf.q02",  0x080000, 0x080000, CRC(8c66ae26) SHA1(32a82aee6ed4480e5a990f9af161734c7c0a1403) )
	ROM_LOAD( "ssf.q03",  0x100000, 0x080000, CRC(695cc2ca) SHA1(c2675f0233608b76de528d2a6ef19846d1348060) )
	ROM_LOAD( "ssf.q04",  0x180000, 0x080000, CRC(9d9ebe32) SHA1(9b26329370041374f1a90b479a172d2bc2801c4d) )
	ROM_LOAD( "ssf.q05",  0x200000, 0x080000, CRC(4770e7b7) SHA1(0e764f0befb9227b0b36508d8ca8ec9be31bcb05) )
	ROM_LOAD( "ssf.q06",  0x280000, 0x080000, CRC(4e79c951) SHA1(1144781d7dc57ef8a6458d982f5c91548ff59e27) )
	ROM_LOAD( "ssf.q07",  0x300000, 0x080000, CRC(cdd14313) SHA1(ebe767a9d4b51dba4282fa0a36a546a88620dd59) )
	ROM_LOAD( "ssf.q08",  0x380000, 0x080000, CRC(6f5a088c) SHA1(6c0b4690479647a99d76335f174be8455b4ff118) )
ROM_END

ROM_START( ssf2j )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "ssfj.03b", 0x000000, 0x80000, CRC(5c2e356d) SHA1(379f1e508778adda4a4087ec52c89b2253265f82) )
	ROM_LOAD16_WORD_SWAP( "ssfj.04a", 0x080000, 0x80000, CRC(013bd55c) SHA1(2482f823a980d45baeea8009dadae7f996bcdb5d) )
	ROM_LOAD16_WORD_SWAP( "ssfj.05",  0x100000, 0x80000, CRC(0918d19a) SHA1(c23be61dd193058eb1391d39fbc22fbcf0640ee0) )
	ROM_LOAD16_WORD_SWAP( "ssfj.06b", 0x180000, 0x80000, CRC(014e0c6d) SHA1(4a5689a05900564c2544c95741cd450ce55da0a7) )
	ROM_LOAD16_WORD_SWAP( "ssfj.07",  0x200000, 0x80000, CRC(eb6a9b1b) SHA1(daedb669b0025f6efb0f3302a40d88dcde2fc76f) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "ssfjx.03b", 0x000000, 0x80000, CRC(693985dd) SHA1(eb37c10a1be5ba80587837ada371ae815420d28d) )
	ROM_LOAD16_WORD_SWAP( "ssfjx.04a", 0x080000, 0x80000, CRC(f866d34a) SHA1(858a2d7ddf2f1743363e506d55aaab20fe40811a) )
	ROM_LOAD16_WORD_SWAP( "ssfjx.05",  0x100000, 0x80000, CRC(7282bb56) SHA1(7627d0262cb5ecf54b402c56c603173e9e096c4c) )
	ROM_LOAD16_WORD_SWAP( "ssfjx.06b", 0x180000, 0x80000, CRC(c597bc4a) SHA1(a65df50ca71bf140536d603a6adb2d7747b82082) )
	ROM_LOAD16_WORD_SWAP( "ssfjx.07",  0x200000, 0x80000, CRC(2af7cab2) SHA1(adcebcca11f746d7b7d881338e0f2b053584e04e) )

	ROM_REGION( 0xc00000, REGION_GFX1, 0 )
	ROMX_LOAD( "ssf.13",   0x000000, 0x200000, CRC(cf94d275) SHA1(bf2a6d98a656d1cb5734da7836686242d3211137) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.15",   0x000002, 0x200000, CRC(5eb703af) SHA1(4b302dbb66e8a5c2ad92798699391e981bada427) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.17",   0x000004, 0x200000, CRC(ffa60e0f) SHA1(b21b1c749a8241440879bf8e7cb33968ccef97e5) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.19",   0x000006, 0x200000, CRC(34e825c5) SHA1(4d320fc96d1ef0b9928a8ce801734245a4c097a5) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.14",   0x800000, 0x100000, CRC(b7cc32e7) SHA1(0f4d26af338dab5dce5b7b34d32ad0c573434ace) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.16",   0x800002, 0x100000, CRC(8376ad18) SHA1(f4456833fb396e6501f4174c0fe5fd63ea40a188) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.18",   0x800004, 0x100000, CRC(f5b1b336) SHA1(4b060501e56b9d61294748da5387cdae5280ec4d) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.20",   0x800006, 0x100000, CRC(459d5c6b) SHA1(32b11ba7a12004aff810d719bff7508204c7b7c0) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "ssf.01",   0x00000, 0x08000, CRC(eb247e8c) SHA1(24296c18d9b1136d69712bf1c9d9d15463041e83) )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD( "ssf.q01",  0x000000, 0x080000, CRC(a6f9da5c) SHA1(6d19f83a01bd25b838d5c2871f7964529d926c98) )
	ROM_LOAD( "ssf.q02",  0x080000, 0x080000, CRC(8c66ae26) SHA1(32a82aee6ed4480e5a990f9af161734c7c0a1403) )
	ROM_LOAD( "ssf.q03",  0x100000, 0x080000, CRC(695cc2ca) SHA1(c2675f0233608b76de528d2a6ef19846d1348060) )
	ROM_LOAD( "ssf.q04",  0x180000, 0x080000, CRC(9d9ebe32) SHA1(9b26329370041374f1a90b479a172d2bc2801c4d) )
	ROM_LOAD( "ssf.q05",  0x200000, 0x080000, CRC(4770e7b7) SHA1(0e764f0befb9227b0b36508d8ca8ec9be31bcb05) )
	ROM_LOAD( "ssf.q06",  0x280000, 0x080000, CRC(4e79c951) SHA1(1144781d7dc57ef8a6458d982f5c91548ff59e27) )
	ROM_LOAD( "ssf.q07",  0x300000, 0x080000, CRC(cdd14313) SHA1(ebe767a9d4b51dba4282fa0a36a546a88620dd59) )
	ROM_LOAD( "ssf.q08",  0x380000, 0x080000, CRC(6f5a088c) SHA1(6c0b4690479647a99d76335f174be8455b4ff118) )
ROM_END

ROM_START( ssf2jr1 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "ssfj.03a", 0x000000, 0x80000, CRC(0bbf1304) SHA1(be93b559ebfcc0fd72cde787c5ea4f50eac52bbf) )
	ROM_LOAD16_WORD_SWAP( "ssfj.04a", 0x080000, 0x80000, CRC(013bd55c) SHA1(2482f823a980d45baeea8009dadae7f996bcdb5d) )
	ROM_LOAD16_WORD_SWAP( "ssfj.05",  0x100000, 0x80000, CRC(0918d19a) SHA1(c23be61dd193058eb1391d39fbc22fbcf0640ee0) )
	ROM_LOAD16_WORD_SWAP( "ssfj.06",  0x180000, 0x80000, CRC(d808a6cd) SHA1(214a4281abacdf6b74b7f51379a93cc64b4c1d7d) )
	ROM_LOAD16_WORD_SWAP( "ssfj.07",  0x200000, 0x80000, CRC(eb6a9b1b) SHA1(daedb669b0025f6efb0f3302a40d88dcde2fc76f) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "ssfjx.03a", 0x000000, 0x80000, CRC(c1b1d0c1) SHA1(77a4d4e6b07b8e37760f0bb0559ea90766941b5d) )
	ROM_LOAD16_WORD_SWAP( "ssfjx.04a", 0x080000, 0x80000, CRC(f866d34a) SHA1(858a2d7ddf2f1743363e506d55aaab20fe40811a) )
	ROM_LOAD16_WORD_SWAP( "ssfjx.05",  0x100000, 0x80000, CRC(7282bb56) SHA1(7627d0262cb5ecf54b402c56c603173e9e096c4c) )
	ROM_LOAD16_WORD_SWAP( "ssfjx.06",  0x180000, 0x80000, CRC(cc027506) SHA1(5968863f6fd7cd0622d3aadcbd4584a195f51636) )
	ROM_LOAD16_WORD_SWAP( "ssfjx.07",  0x200000, 0x80000, CRC(2af7cab2) SHA1(adcebcca11f746d7b7d881338e0f2b053584e04e) )

	ROM_REGION( 0xc00000, REGION_GFX1, 0 )
	ROMX_LOAD( "ssf.13",   0x000000, 0x200000, CRC(cf94d275) SHA1(bf2a6d98a656d1cb5734da7836686242d3211137) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.15",   0x000002, 0x200000, CRC(5eb703af) SHA1(4b302dbb66e8a5c2ad92798699391e981bada427) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.17",   0x000004, 0x200000, CRC(ffa60e0f) SHA1(b21b1c749a8241440879bf8e7cb33968ccef97e5) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.19",   0x000006, 0x200000, CRC(34e825c5) SHA1(4d320fc96d1ef0b9928a8ce801734245a4c097a5) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.14",   0x800000, 0x100000, CRC(b7cc32e7) SHA1(0f4d26af338dab5dce5b7b34d32ad0c573434ace) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.16",   0x800002, 0x100000, CRC(8376ad18) SHA1(f4456833fb396e6501f4174c0fe5fd63ea40a188) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.18",   0x800004, 0x100000, CRC(f5b1b336) SHA1(4b060501e56b9d61294748da5387cdae5280ec4d) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.20",   0x800006, 0x100000, CRC(459d5c6b) SHA1(32b11ba7a12004aff810d719bff7508204c7b7c0) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "ssf.01",   0x00000, 0x08000, CRC(eb247e8c) SHA1(24296c18d9b1136d69712bf1c9d9d15463041e83) )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD( "ssf.q01",  0x000000, 0x080000, CRC(a6f9da5c) SHA1(6d19f83a01bd25b838d5c2871f7964529d926c98) )
	ROM_LOAD( "ssf.q02",  0x080000, 0x080000, CRC(8c66ae26) SHA1(32a82aee6ed4480e5a990f9af161734c7c0a1403) )
	ROM_LOAD( "ssf.q03",  0x100000, 0x080000, CRC(695cc2ca) SHA1(c2675f0233608b76de528d2a6ef19846d1348060) )
	ROM_LOAD( "ssf.q04",  0x180000, 0x080000, CRC(9d9ebe32) SHA1(9b26329370041374f1a90b479a172d2bc2801c4d) )
	ROM_LOAD( "ssf.q05",  0x200000, 0x080000, CRC(4770e7b7) SHA1(0e764f0befb9227b0b36508d8ca8ec9be31bcb05) )
	ROM_LOAD( "ssf.q06",  0x280000, 0x080000, CRC(4e79c951) SHA1(1144781d7dc57ef8a6458d982f5c91548ff59e27) )
	ROM_LOAD( "ssf.q07",  0x300000, 0x080000, CRC(cdd14313) SHA1(ebe767a9d4b51dba4282fa0a36a546a88620dd59) )
	ROM_LOAD( "ssf.q08",  0x380000, 0x080000, CRC(6f5a088c) SHA1(6c0b4690479647a99d76335f174be8455b4ff118) )
ROM_END

ROM_START( ssf2jr2 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "ssfj.03", 0x000000, 0x80000, CRC(7eb0efed) SHA1(c389301cf26cc72ef10c20a7d37223622d05e9ca) )
	ROM_LOAD16_WORD_SWAP( "ssfj.04", 0x080000, 0x80000, CRC(d7322164) SHA1(b83c8523d152384a3eb9f459685b11c6e77cd6d4) )
	ROM_LOAD16_WORD_SWAP( "ssfj.05", 0x100000, 0x80000, CRC(0918d19a) SHA1(c23be61dd193058eb1391d39fbc22fbcf0640ee0) )
	ROM_LOAD16_WORD_SWAP( "ssfj.06", 0x180000, 0x80000, CRC(d808a6cd) SHA1(214a4281abacdf6b74b7f51379a93cc64b4c1d7d) )
	ROM_LOAD16_WORD_SWAP( "ssfj.07", 0x200000, 0x80000, CRC(eb6a9b1b) SHA1(daedb669b0025f6efb0f3302a40d88dcde2fc76f) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "ssfjx.03", 0x000000, 0x80000, CRC(c3eca34c) SHA1(3c81e7125b6c0988cc73f68ac0b25ab3a53d0dda) )
	ROM_LOAD16_WORD_SWAP( "ssfjx.04", 0x080000, 0x80000, CRC(4e1080c2) SHA1(30a676795b8011530c7792585471f68065915f28) )
	ROM_LOAD16_WORD_SWAP( "ssfjx.05", 0x100000, 0x80000, CRC(7282bb56) SHA1(7627d0262cb5ecf54b402c56c603173e9e096c4c) )
	ROM_LOAD16_WORD_SWAP( "ssfjx.06", 0x180000, 0x80000, CRC(cc027506) SHA1(5968863f6fd7cd0622d3aadcbd4584a195f51636) )
	ROM_LOAD16_WORD_SWAP( "ssfjx.07", 0x200000, 0x80000, CRC(2af7cab2) SHA1(adcebcca11f746d7b7d881338e0f2b053584e04e) )

	ROM_REGION( 0xc00000, REGION_GFX1, 0 )
	ROMX_LOAD( "ssf.13",   0x000000, 0x200000, CRC(cf94d275) SHA1(bf2a6d98a656d1cb5734da7836686242d3211137) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.15",   0x000002, 0x200000, CRC(5eb703af) SHA1(4b302dbb66e8a5c2ad92798699391e981bada427) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.17",   0x000004, 0x200000, CRC(ffa60e0f) SHA1(b21b1c749a8241440879bf8e7cb33968ccef97e5) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.19",   0x000006, 0x200000, CRC(34e825c5) SHA1(4d320fc96d1ef0b9928a8ce801734245a4c097a5) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.14",   0x800000, 0x100000, CRC(b7cc32e7) SHA1(0f4d26af338dab5dce5b7b34d32ad0c573434ace) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.16",   0x800002, 0x100000, CRC(8376ad18) SHA1(f4456833fb396e6501f4174c0fe5fd63ea40a188) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.18",   0x800004, 0x100000, CRC(f5b1b336) SHA1(4b060501e56b9d61294748da5387cdae5280ec4d) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.20",   0x800006, 0x100000, CRC(459d5c6b) SHA1(32b11ba7a12004aff810d719bff7508204c7b7c0) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "ssf.01",   0x00000, 0x08000, CRC(eb247e8c) SHA1(24296c18d9b1136d69712bf1c9d9d15463041e83) )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD( "ssf.q01",  0x000000, 0x080000, CRC(a6f9da5c) SHA1(6d19f83a01bd25b838d5c2871f7964529d926c98) )
	ROM_LOAD( "ssf.q02",  0x080000, 0x080000, CRC(8c66ae26) SHA1(32a82aee6ed4480e5a990f9af161734c7c0a1403) )
	ROM_LOAD( "ssf.q03",  0x100000, 0x080000, CRC(695cc2ca) SHA1(c2675f0233608b76de528d2a6ef19846d1348060) )
	ROM_LOAD( "ssf.q04",  0x180000, 0x080000, CRC(9d9ebe32) SHA1(9b26329370041374f1a90b479a172d2bc2801c4d) )
	ROM_LOAD( "ssf.q05",  0x200000, 0x080000, CRC(4770e7b7) SHA1(0e764f0befb9227b0b36508d8ca8ec9be31bcb05) )
	ROM_LOAD( "ssf.q06",  0x280000, 0x080000, CRC(4e79c951) SHA1(1144781d7dc57ef8a6458d982f5c91548ff59e27) )
	ROM_LOAD( "ssf.q07",  0x300000, 0x080000, CRC(cdd14313) SHA1(ebe767a9d4b51dba4282fa0a36a546a88620dd59) )
	ROM_LOAD( "ssf.q08",  0x380000, 0x080000, CRC(6f5a088c) SHA1(6c0b4690479647a99d76335f174be8455b4ff118) )
ROM_END

ROM_START( ssf2tb )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "ssfe.3tc", 0x000000, 0x80000, CRC(496a8409) SHA1(3101689e86ab78c544524e31057478fce336ddaa) )
	ROM_LOAD16_WORD_SWAP( "ssfe.4tc", 0x080000, 0x80000, CRC(4b45c18b) SHA1(9c7ecb6fee70e317d1005bcadadf59cf11f58050) )
	ROM_LOAD16_WORD_SWAP( "ssfe.5t",  0x100000, 0x80000, CRC(6a9c6444) SHA1(76ba626136268a48b139f6aacd6eeded94d1354d) )
	ROM_LOAD16_WORD_SWAP( "ssfe.6tb", 0x180000, 0x80000, CRC(e4944fc3) SHA1(2d77bc19140c8895eca445b6a290bc793946ccfb) )
	ROM_LOAD16_WORD_SWAP( "ssfe.7t",  0x200000, 0x80000, CRC(2c9f4782) SHA1(de046e6bd9823129fb3d1bfff3710689816a6b0a) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "ssfex.3tc", 0x000000, 0x80000, CRC(9b2cda8c) SHA1(610c44fbe02ad93abdf9f6c978bdf6732ca588f7) )
	ROM_LOAD16_WORD_SWAP( "ssfex.4tc", 0x080000, 0x80000, CRC(62d26dc2) SHA1(fdf7955256f3be49f60d4866e095f5ef26261209) )
	ROM_LOAD16_WORD_SWAP( "ssfex.5t",  0x100000, 0x80000, CRC(3ae42ff3) SHA1(0654debcf58847e5bb005c8bebb817b8f2518ca6) )
	ROM_LOAD16_WORD_SWAP( "ssfex.6tb", 0x180000, 0x80000, CRC(f12e7228) SHA1(1d90c5fbc51325069c4d1f971d948208b22ecb2e) )
	ROM_LOAD16_WORD_SWAP( "ssfex.7t",  0x200000, 0x80000, CRC(4d51f760) SHA1(a15de77b304a4e1847017f146fef80ead54fe658) )

	ROM_REGION( 0xc00000, REGION_GFX1, 0 )
	ROMX_LOAD( "ssf.13",   0x000000, 0x200000, CRC(cf94d275) SHA1(bf2a6d98a656d1cb5734da7836686242d3211137) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.15",   0x000002, 0x200000, CRC(5eb703af) SHA1(4b302dbb66e8a5c2ad92798699391e981bada427) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.17",   0x000004, 0x200000, CRC(ffa60e0f) SHA1(b21b1c749a8241440879bf8e7cb33968ccef97e5) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.19",   0x000006, 0x200000, CRC(34e825c5) SHA1(4d320fc96d1ef0b9928a8ce801734245a4c097a5) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.14",   0x800000, 0x100000, CRC(b7cc32e7) SHA1(0f4d26af338dab5dce5b7b34d32ad0c573434ace) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.16",   0x800002, 0x100000, CRC(8376ad18) SHA1(f4456833fb396e6501f4174c0fe5fd63ea40a188) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.18",   0x800004, 0x100000, CRC(f5b1b336) SHA1(4b060501e56b9d61294748da5387cdae5280ec4d) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.20",   0x800006, 0x100000, CRC(459d5c6b) SHA1(32b11ba7a12004aff810d719bff7508204c7b7c0) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "ssf.01",   0x00000, 0x08000, CRC(eb247e8c) SHA1(24296c18d9b1136d69712bf1c9d9d15463041e83) )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD( "ssf.q01",  0x000000, 0x080000, CRC(a6f9da5c) SHA1(6d19f83a01bd25b838d5c2871f7964529d926c98) )
	ROM_LOAD( "ssf.q02",  0x080000, 0x080000, CRC(8c66ae26) SHA1(32a82aee6ed4480e5a990f9af161734c7c0a1403) )
	ROM_LOAD( "ssf.q03",  0x100000, 0x080000, CRC(695cc2ca) SHA1(c2675f0233608b76de528d2a6ef19846d1348060) )
	ROM_LOAD( "ssf.q04",  0x180000, 0x080000, CRC(9d9ebe32) SHA1(9b26329370041374f1a90b479a172d2bc2801c4d) )
	ROM_LOAD( "ssf.q05",  0x200000, 0x080000, CRC(4770e7b7) SHA1(0e764f0befb9227b0b36508d8ca8ec9be31bcb05) )
	ROM_LOAD( "ssf.q06",  0x280000, 0x080000, CRC(4e79c951) SHA1(1144781d7dc57ef8a6458d982f5c91548ff59e27) )
	ROM_LOAD( "ssf.q07",  0x300000, 0x080000, CRC(cdd14313) SHA1(ebe767a9d4b51dba4282fa0a36a546a88620dd59) )
	ROM_LOAD( "ssf.q08",  0x380000, 0x080000, CRC(6f5a088c) SHA1(6c0b4690479647a99d76335f174be8455b4ff118) )
ROM_END

ROM_START( ssf2tbj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "ssfj.3t", 0x000000, 0x80000, CRC(980d4450) SHA1(1a7a7000dc11473d06e2bb552c7a506eb0019235) )
	ROM_LOAD16_WORD_SWAP( "ssfj.4t", 0x080000, 0x80000, CRC(b4dc1906) SHA1(b29497c8562d004c6f0393eb61ba80978f4b3aff) )
	ROM_LOAD16_WORD_SWAP( "ssfj.5t", 0x100000, 0x80000, CRC(a7e35fbc) SHA1(c59737f4dbd9ccde30b0a1e2f151a78f162ceafc) )
	ROM_LOAD16_WORD_SWAP( "ssfj.6t", 0x180000, 0x80000, CRC(cb592b30) SHA1(d9464c99f813ee50041adfc077ebe998c6e9a5f7) )
	ROM_LOAD16_WORD_SWAP( "ssfj.7t", 0x200000, 0x80000, CRC(1f239515) SHA1(e5e314e7fe8d1448cc452e515415adf8aa62056d) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
/*	ROM_LOAD16_WORD_SWAP( "ssfjx.3t", 0x000000, 0x80000, NO_DUMP )*/
/*	ROM_LOAD16_WORD_SWAP( "ssfjx.4t", 0x080000, 0x80000, NO_DUMP )*/
/*	ROM_LOAD16_WORD_SWAP( "ssfjx.5t", 0x100000, 0x80000, NO_DUMP )*/
/*	ROM_LOAD16_WORD_SWAP( "ssfjx.6t", 0x180000, 0x80000, NO_DUMP )*/
/*	ROM_LOAD16_WORD_SWAP( "ssfjx.7t", 0x200000, 0x80000, NO_DUMP )*/

	ROM_REGION( 0xc00000, REGION_GFX1, 0 )
	ROMX_LOAD( "ssf.13",   0x000000, 0x200000, CRC(cf94d275) SHA1(bf2a6d98a656d1cb5734da7836686242d3211137) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.15",   0x000002, 0x200000, CRC(5eb703af) SHA1(4b302dbb66e8a5c2ad92798699391e981bada427) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.17",   0x000004, 0x200000, CRC(ffa60e0f) SHA1(b21b1c749a8241440879bf8e7cb33968ccef97e5) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.19",   0x000006, 0x200000, CRC(34e825c5) SHA1(4d320fc96d1ef0b9928a8ce801734245a4c097a5) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.14",   0x800000, 0x100000, CRC(b7cc32e7) SHA1(0f4d26af338dab5dce5b7b34d32ad0c573434ace) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.16",   0x800002, 0x100000, CRC(8376ad18) SHA1(f4456833fb396e6501f4174c0fe5fd63ea40a188) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.18",   0x800004, 0x100000, CRC(f5b1b336) SHA1(4b060501e56b9d61294748da5387cdae5280ec4d) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.20",   0x800006, 0x100000, CRC(459d5c6b) SHA1(32b11ba7a12004aff810d719bff7508204c7b7c0) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "ssf.01",   0x00000, 0x08000, CRC(eb247e8c) SHA1(24296c18d9b1136d69712bf1c9d9d15463041e83) )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD( "ssf.q01",  0x000000, 0x080000, CRC(a6f9da5c) SHA1(6d19f83a01bd25b838d5c2871f7964529d926c98) )
	ROM_LOAD( "ssf.q02",  0x080000, 0x080000, CRC(8c66ae26) SHA1(32a82aee6ed4480e5a990f9af161734c7c0a1403) )
	ROM_LOAD( "ssf.q03",  0x100000, 0x080000, CRC(695cc2ca) SHA1(c2675f0233608b76de528d2a6ef19846d1348060) )
	ROM_LOAD( "ssf.q04",  0x180000, 0x080000, CRC(9d9ebe32) SHA1(9b26329370041374f1a90b479a172d2bc2801c4d) )
	ROM_LOAD( "ssf.q05",  0x200000, 0x080000, CRC(4770e7b7) SHA1(0e764f0befb9227b0b36508d8ca8ec9be31bcb05) )
	ROM_LOAD( "ssf.q06",  0x280000, 0x080000, CRC(4e79c951) SHA1(1144781d7dc57ef8a6458d982f5c91548ff59e27) )
	ROM_LOAD( "ssf.q07",  0x300000, 0x080000, CRC(cdd14313) SHA1(ebe767a9d4b51dba4282fa0a36a546a88620dd59) )
	ROM_LOAD( "ssf.q08",  0x380000, 0x080000, CRC(6f5a088c) SHA1(6c0b4690479647a99d76335f174be8455b4ff118) )
ROM_END

ROM_START( ssf2t )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "sfxe.03c", 0x000000, 0x80000, CRC(2fa1f396) SHA1(2aa58309811f34901554b84396556630a22ce9bc) )
	ROM_LOAD16_WORD_SWAP( "sfxe.04a", 0x080000, 0x80000, CRC(d0bc29c6) SHA1(d9f89bcd79cba26db2100a00dd7bd8ee6ecb75f3) )
	ROM_LOAD16_WORD_SWAP( "sfxe.05",  0x100000, 0x80000, CRC(65222964) SHA1(025bb708dc5b6365cc7fe60fc3f242511ad8f384) )
	ROM_LOAD16_WORD_SWAP( "sfxe.06a", 0x180000, 0x80000, CRC(8fe9f531) SHA1(b5d9ed498f730fdb968992bdec33605db1a007f4) )
	ROM_LOAD16_WORD_SWAP( "sfxe.07",  0x200000, 0x80000, CRC(8a7d0cb6) SHA1(27ea0cea73a93c27257bf2a170d1deaf938cc311) )
	ROM_LOAD16_WORD_SWAP( "sfxe.08",  0x280000, 0x80000, CRC(74c24062) SHA1(f3eca09e0544c6aa46b0c4bead2246ab1e9a97d9) )
	ROM_LOAD16_WORD_SWAP( "sfx.09",   0x300000, 0x80000, CRC(642fae3f) SHA1(746df99b826b9837bba267104132161153c1daff) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "sfxex.03c", 0x000000, 0x80000, CRC(a181b207) SHA1(9e8d6d9aa19d39d2893be8faaa9a8cc74221bbdc) )
	ROM_LOAD16_WORD_SWAP( "sfxex.04a", 0x080000, 0x80000, CRC(df28bd00) SHA1(50e8c003e665c647d37ab12c3fa8b52604060322) )
	ROM_LOAD16_WORD_SWAP( "sfxex.05",  0x100000, 0x80000, CRC(29b7bda4) SHA1(beccb5a2678ca3ba2a06cf84af123ec3839e5b30) )
	ROM_LOAD16_WORD_SWAP( "sfxex.06a", 0x180000, 0x80000, CRC(6c8719b3) SHA1(e6a5a3b04256fbfe8baa463785a3f247c89ac9e4) )
	ROM_LOAD16_WORD_SWAP( "sfxex.07",  0x200000, 0x80000, CRC(dfc3b3cd) SHA1(2b2c2fe474ba7dfd6d46343cdae1be30dab0cf65) )
	ROM_LOAD16_WORD_SWAP( "sfxex.08",  0x280000, 0x80000, CRC(d7436ae9) SHA1(1861f9c81ff2ffd956b94412ef6eb096f7558b96) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROMX_LOAD( "ssf.13",   0x000000, 0x200000, CRC(cf94d275) SHA1(bf2a6d98a656d1cb5734da7836686242d3211137) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.15",   0x000002, 0x200000, CRC(5eb703af) SHA1(4b302dbb66e8a5c2ad92798699391e981bada427) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.17",   0x000004, 0x200000, CRC(ffa60e0f) SHA1(b21b1c749a8241440879bf8e7cb33968ccef97e5) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.19",   0x000006, 0x200000, CRC(34e825c5) SHA1(4d320fc96d1ef0b9928a8ce801734245a4c097a5) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.14",   0x800000, 0x100000, CRC(b7cc32e7) SHA1(0f4d26af338dab5dce5b7b34d32ad0c573434ace) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.16",   0x800002, 0x100000, CRC(8376ad18) SHA1(f4456833fb396e6501f4174c0fe5fd63ea40a188) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.18",   0x800004, 0x100000, CRC(f5b1b336) SHA1(4b060501e56b9d61294748da5387cdae5280ec4d) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.20",   0x800006, 0x100000, CRC(459d5c6b) SHA1(32b11ba7a12004aff810d719bff7508204c7b7c0) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfx.21",   0xc00000, 0x100000, CRC(e32854af) SHA1(1a5e11e9caa2b96108d89ae660ef1f6bcb469a74) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfx.23",   0xc00002, 0x100000, CRC(760f2927) SHA1(491e28e14ee06821fc9e709efa7b91313bc0c2db) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfx.25",   0xc00004, 0x100000, CRC(1ee90208) SHA1(83df1d9953560edddc2951ea426d29fb014e6a8a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfx.27",   0xc00006, 0x100000, CRC(f814400f) SHA1(ad6921af36d0bd5dfb89b1fb53c3ca3fd92d7204) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sfx.01",   0x00000, 0x08000, CRC(b47b8835) SHA1(c8b2d50fe3a329bd0592ea160d505155d873dab1) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "sfx.02",   0x28000, 0x20000, CRC(0022633f) SHA1(cab3afc79da53e3887eb1ccd1f4d19790728e6cd) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "sfx.11",   0x000000, 0x200000, CRC(9bdbd476) SHA1(a8520f77f30b97aae36408e0c4ca1ebbde1808a5) )
	ROM_LOAD16_WORD_SWAP( "sfx.12",   0x200000, 0x200000, CRC(a05e3aab) SHA1(d4eb9cae66c74e956569fea8b815156fbd420f83) )
ROM_END

ROM_START( ssf2tu )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "sfxu.03e", 0x000000, 0x80000, CRC(d6ff689e) SHA1(bea1a8aafbbbe9cb0895561a57dead1579361a8e) )
	ROM_LOAD16_WORD_SWAP( "sfxu.04a", 0x080000, 0x80000, CRC(532b5ffd) SHA1(769a8a9d4e04e291ce7427b89e537bba2258ca82) )
	ROM_LOAD16_WORD_SWAP( "sfxu.05",  0x100000, 0x80000, CRC(ffa3c6de) SHA1(7cce55a3e07b5ba2e2e37e4c66a52678a1b19a63) )
	ROM_LOAD16_WORD_SWAP( "sfxu.06b", 0x180000, 0x80000, CRC(83f9382b) SHA1(273ff4d4242ce22b755d35e5d2cf2517d625bdd2) )
	ROM_LOAD16_WORD_SWAP( "sfxu.07a", 0x200000, 0x80000, CRC(6ab673e8) SHA1(840af0d0ce634fb98e4f89173c4f1f95ec2cf94b) )
	ROM_LOAD16_WORD_SWAP( "sfxu.08",  0x280000, 0x80000, CRC(b3c71810) SHA1(b51515f4f4aee5bbbfc8b79372d0bc6e0c140912) )
	ROM_LOAD16_WORD_SWAP( "sfx.09",   0x300000, 0x80000, CRC(642fae3f) SHA1(746df99b826b9837bba267104132161153c1daff) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
/*	ROM_LOAD16_WORD_SWAP( "sfxux.03e", 0x000000, 0x80000, NO_DUMP )*/
/*	ROM_LOAD16_WORD_SWAP( "sfxux.04a", 0x080000, 0x80000, NO_DUMP )*/
/*	ROM_LOAD16_WORD_SWAP( "sfxux.05",  0x100000, 0x80000, NO_DUMP )*/
/*	ROM_LOAD16_WORD_SWAP( "sfxux.06b", 0x180000, 0x80000, NO_DUMP )*/
/*	ROM_LOAD16_WORD_SWAP( "sfxux.07a", 0x200000, 0x80000, NO_DUMP )*/
/*	ROM_LOAD16_WORD_SWAP( "sfxux.08",  0x280000, 0x80000, NO_DUMP )*/

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROMX_LOAD( "ssf.13",   0x000000, 0x200000, CRC(cf94d275) SHA1(bf2a6d98a656d1cb5734da7836686242d3211137) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.15",   0x000002, 0x200000, CRC(5eb703af) SHA1(4b302dbb66e8a5c2ad92798699391e981bada427) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.17",   0x000004, 0x200000, CRC(ffa60e0f) SHA1(b21b1c749a8241440879bf8e7cb33968ccef97e5) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.19",   0x000006, 0x200000, CRC(34e825c5) SHA1(4d320fc96d1ef0b9928a8ce801734245a4c097a5) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.14",   0x800000, 0x100000, CRC(b7cc32e7) SHA1(0f4d26af338dab5dce5b7b34d32ad0c573434ace) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.16",   0x800002, 0x100000, CRC(8376ad18) SHA1(f4456833fb396e6501f4174c0fe5fd63ea40a188) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.18",   0x800004, 0x100000, CRC(f5b1b336) SHA1(4b060501e56b9d61294748da5387cdae5280ec4d) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.20",   0x800006, 0x100000, CRC(459d5c6b) SHA1(32b11ba7a12004aff810d719bff7508204c7b7c0) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfx.21",   0xc00000, 0x100000, CRC(e32854af) SHA1(1a5e11e9caa2b96108d89ae660ef1f6bcb469a74) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfx.23",   0xc00002, 0x100000, CRC(760f2927) SHA1(491e28e14ee06821fc9e709efa7b91313bc0c2db) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfx.25",   0xc00004, 0x100000, CRC(1ee90208) SHA1(83df1d9953560edddc2951ea426d29fb014e6a8a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfx.27",   0xc00006, 0x100000, CRC(f814400f) SHA1(ad6921af36d0bd5dfb89b1fb53c3ca3fd92d7204) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sfx.01",   0x00000, 0x08000, CRC(b47b8835) SHA1(c8b2d50fe3a329bd0592ea160d505155d873dab1) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "sfx.02",   0x28000, 0x20000, CRC(0022633f) SHA1(cab3afc79da53e3887eb1ccd1f4d19790728e6cd) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "sfx.11",   0x000000, 0x200000, CRC(9bdbd476) SHA1(a8520f77f30b97aae36408e0c4ca1ebbde1808a5) )
	ROM_LOAD16_WORD_SWAP( "sfx.12",   0x200000, 0x200000, CRC(a05e3aab) SHA1(d4eb9cae66c74e956569fea8b815156fbd420f83) )
ROM_END

ROM_START( ssf2tur1 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "sfxu.03c", 0x000000, 0x80000, CRC(86e4a335) SHA1(04db3fd519973aeb7b32be62871e0fb4605946eb) )
	ROM_LOAD16_WORD_SWAP( "sfxu.04a", 0x080000, 0x80000, CRC(532b5ffd) SHA1(769a8a9d4e04e291ce7427b89e537bba2258ca82) )
	ROM_LOAD16_WORD_SWAP( "sfxu.05",  0x100000, 0x80000, CRC(ffa3c6de) SHA1(7cce55a3e07b5ba2e2e37e4c66a52678a1b19a63) )
	ROM_LOAD16_WORD_SWAP( "sfxu.06a", 0x180000, 0x80000, CRC(e4c04c99) SHA1(01fe284363e4795e7bdf4206f54a6108fcdac18b) )
	ROM_LOAD16_WORD_SWAP( "sfxu.07",  0x200000, 0x80000, CRC(d8199e41) SHA1(aa5647446f7e076cdf895dd5cbc5b30a8d4fdac2) )
	ROM_LOAD16_WORD_SWAP( "sfxu.08",  0x280000, 0x80000, CRC(b3c71810) SHA1(b51515f4f4aee5bbbfc8b79372d0bc6e0c140912) )
	ROM_LOAD16_WORD_SWAP( "sfx.09",   0x300000, 0x80000, CRC(642fae3f) SHA1(746df99b826b9837bba267104132161153c1daff) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "sfxux.03c", 0x000000, 0x80000, CRC(441df197) SHA1(c9109d736febb88d74844300ea480841ecbdfa5a) )
	ROM_LOAD16_WORD_SWAP( "sfxux.04a", 0x080000, 0x80000, CRC(7390df1b) SHA1(1f9412f35e431ca99ce1dda3fda729adfa012403) )
	ROM_LOAD16_WORD_SWAP( "sfxux.05",  0x100000, 0x80000, CRC(1d3310a0) SHA1(519e19c38ce1734afd43478074f0d8b3e1e7a8ea) )
	ROM_LOAD16_WORD_SWAP( "sfxux.06a", 0x180000, 0x80000, CRC(6fc5efa6) SHA1(f98e2fdbdf9c04548833bf47ca371992943b26d0) )
	ROM_LOAD16_WORD_SWAP( "sfxux.07",  0x200000, 0x80000, CRC(88455606) SHA1(94ed78ee15f1a332ee91528b16f364e473840ecd) )
	ROM_LOAD16_WORD_SWAP( "sfxux.08",  0x280000, 0x80000, CRC(fa2396a6) SHA1(80da9a5355eebd9dc2f3846020338966a1de596f) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROMX_LOAD( "ssf.13",   0x000000, 0x200000, CRC(cf94d275) SHA1(bf2a6d98a656d1cb5734da7836686242d3211137) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.15",   0x000002, 0x200000, CRC(5eb703af) SHA1(4b302dbb66e8a5c2ad92798699391e981bada427) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.17",   0x000004, 0x200000, CRC(ffa60e0f) SHA1(b21b1c749a8241440879bf8e7cb33968ccef97e5) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.19",   0x000006, 0x200000, CRC(34e825c5) SHA1(4d320fc96d1ef0b9928a8ce801734245a4c097a5) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.14",   0x800000, 0x100000, CRC(b7cc32e7) SHA1(0f4d26af338dab5dce5b7b34d32ad0c573434ace) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.16",   0x800002, 0x100000, CRC(8376ad18) SHA1(f4456833fb396e6501f4174c0fe5fd63ea40a188) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.18",   0x800004, 0x100000, CRC(f5b1b336) SHA1(4b060501e56b9d61294748da5387cdae5280ec4d) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.20",   0x800006, 0x100000, CRC(459d5c6b) SHA1(32b11ba7a12004aff810d719bff7508204c7b7c0) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfx.21",   0xc00000, 0x100000, CRC(e32854af) SHA1(1a5e11e9caa2b96108d89ae660ef1f6bcb469a74) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfx.23",   0xc00002, 0x100000, CRC(760f2927) SHA1(491e28e14ee06821fc9e709efa7b91313bc0c2db) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfx.25",   0xc00004, 0x100000, CRC(1ee90208) SHA1(83df1d9953560edddc2951ea426d29fb014e6a8a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfx.27",   0xc00006, 0x100000, CRC(f814400f) SHA1(ad6921af36d0bd5dfb89b1fb53c3ca3fd92d7204) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sfx.01",   0x00000, 0x08000, CRC(b47b8835) SHA1(c8b2d50fe3a329bd0592ea160d505155d873dab1) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "sfx.02",   0x28000, 0x20000, CRC(0022633f) SHA1(cab3afc79da53e3887eb1ccd1f4d19790728e6cd) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "sfx.11",   0x000000, 0x200000, CRC(9bdbd476) SHA1(a8520f77f30b97aae36408e0c4ca1ebbde1808a5) )
	ROM_LOAD16_WORD_SWAP( "sfx.12",   0x200000, 0x200000, CRC(a05e3aab) SHA1(d4eb9cae66c74e956569fea8b815156fbd420f83) )
ROM_END

ROM_START( ssf2ta )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "sfxa.03c", 0x000000, 0x80000, CRC(04b9ff34) SHA1(69feb2c9c03634e6f964dae310d7b72b4c76140d) )
	ROM_LOAD16_WORD_SWAP( "sfxa.04a", 0x080000, 0x80000, CRC(16ea5f7a) SHA1(08404c6a79b9a36eceb06e0d3e1d747a21fac186) )
	ROM_LOAD16_WORD_SWAP( "sfxa.05",  0x100000, 0x80000, CRC(53d61f0c) SHA1(b30e666d0dae7b738a76a27d1d68fbb9a630c27c) )
	ROM_LOAD16_WORD_SWAP( "sfxa.06a", 0x180000, 0x80000, CRC(066d09b5) SHA1(221972629b094809f7c431f86b3f3b10354487b5) )
	ROM_LOAD16_WORD_SWAP( "sfxa.07",  0x200000, 0x80000, CRC(a428257b) SHA1(620f3a264b2c82ef1af0e33310d110e1f3e6fddf) )
	ROM_LOAD16_WORD_SWAP( "sfxa.08",  0x280000, 0x80000, CRC(39be596c) SHA1(f7ab80e64cbb703535dd39b875273eefa57df489) )
	ROM_LOAD16_WORD_SWAP( "sfx.09",   0x300000, 0x80000, CRC(642fae3f) SHA1(746df99b826b9837bba267104132161153c1daff) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "sfxax.03c", 0x000000, 0x80000, CRC(c8a664fa) SHA1(cc3869405f55a150859d372825af23dc2eeb9646) )
	ROM_LOAD16_WORD_SWAP( "sfxax.04a", 0x080000, 0x80000, CRC(149d6352) SHA1(4b949492118fc1c11bbd7441ae458e0ef5510e90) )
	ROM_LOAD16_WORD_SWAP( "sfxax.05",  0x100000, 0x80000, CRC(ac169aa9) SHA1(996c1dc6c5469d4aeb3eab7adf4aaed36bf18c47) )
	ROM_LOAD16_WORD_SWAP( "sfxax.06a", 0x180000, 0x80000, CRC(bb60394c) SHA1(cf30cdadf7294023ccd8650c5a1db8d568cddbdb) )
	ROM_LOAD16_WORD_SWAP( "sfxax.07",  0x200000, 0x80000, CRC(e62b1b69) SHA1(49fe3437c3fe5057fd82fc6bf8da4af653c37eba) )
	ROM_LOAD16_WORD_SWAP( "sfxax.08",  0x280000, 0x80000, CRC(7c5fd202) SHA1(e08db90a71528b8479de820cd6d6a7304de9775c) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROMX_LOAD( "ssf.13",   0x000000, 0x200000, CRC(cf94d275) SHA1(bf2a6d98a656d1cb5734da7836686242d3211137) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.15",   0x000002, 0x200000, CRC(5eb703af) SHA1(4b302dbb66e8a5c2ad92798699391e981bada427) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.17",   0x000004, 0x200000, CRC(ffa60e0f) SHA1(b21b1c749a8241440879bf8e7cb33968ccef97e5) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.19",   0x000006, 0x200000, CRC(34e825c5) SHA1(4d320fc96d1ef0b9928a8ce801734245a4c097a5) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.14",   0x800000, 0x100000, CRC(b7cc32e7) SHA1(0f4d26af338dab5dce5b7b34d32ad0c573434ace) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.16",   0x800002, 0x100000, CRC(8376ad18) SHA1(f4456833fb396e6501f4174c0fe5fd63ea40a188) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.18",   0x800004, 0x100000, CRC(f5b1b336) SHA1(4b060501e56b9d61294748da5387cdae5280ec4d) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.20",   0x800006, 0x100000, CRC(459d5c6b) SHA1(32b11ba7a12004aff810d719bff7508204c7b7c0) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfx.21",   0xc00000, 0x100000, CRC(e32854af) SHA1(1a5e11e9caa2b96108d89ae660ef1f6bcb469a74) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfx.23",   0xc00002, 0x100000, CRC(760f2927) SHA1(491e28e14ee06821fc9e709efa7b91313bc0c2db) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfx.25",   0xc00004, 0x100000, CRC(1ee90208) SHA1(83df1d9953560edddc2951ea426d29fb014e6a8a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfx.27",   0xc00006, 0x100000, CRC(f814400f) SHA1(ad6921af36d0bd5dfb89b1fb53c3ca3fd92d7204) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sfx.01",   0x00000, 0x08000, CRC(b47b8835) SHA1(c8b2d50fe3a329bd0592ea160d505155d873dab1) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "sfx.02",   0x28000, 0x20000, CRC(0022633f) SHA1(cab3afc79da53e3887eb1ccd1f4d19790728e6cd) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "sfx.11",   0x000000, 0x200000, CRC(9bdbd476) SHA1(a8520f77f30b97aae36408e0c4ca1ebbde1808a5) )
	ROM_LOAD16_WORD_SWAP( "sfx.12",   0x200000, 0x200000, CRC(a05e3aab) SHA1(d4eb9cae66c74e956569fea8b815156fbd420f83) )
ROM_END

ROM_START( ssf2xj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "sfxj.03c", 0x000000, 0x80000, CRC(a7417b79) SHA1(189c3ed546bb2844e9fa9fe7e9aacef728bc8939) )
	ROM_LOAD16_WORD_SWAP( "sfxj.04a", 0x080000, 0x80000, CRC(af7767b4) SHA1(61e7364408bf07c01634913c112b6245acce48ab) )
	ROM_LOAD16_WORD_SWAP( "sfxj.05",  0x100000, 0x80000, CRC(f4ff18f5) SHA1(aa713c9e1a2eba35bf1c9b40bb262ff7e46b9ce4) )
	ROM_LOAD16_WORD_SWAP( "sfxj.06a", 0x180000, 0x80000, CRC(260d0370) SHA1(5339cf87000caef74d491815391be59cfd701c8b) )
	ROM_LOAD16_WORD_SWAP( "sfxj.07",  0x200000, 0x80000, CRC(1324d02a) SHA1(c23a6ea09819bd33b6e2f58aa28c317ce53a46a0) )
	ROM_LOAD16_WORD_SWAP( "sfxj.08",  0x280000, 0x80000, CRC(2de76f10) SHA1(8cbe96dfeaa41306caa2819b82272ce3b0b9f926) )
	ROM_LOAD16_WORD_SWAP( "sfx.09",   0x300000, 0x80000, CRC(642fae3f) SHA1(746df99b826b9837bba267104132161153c1daff) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "sfxjx.03c", 0x000000, 0x80000, CRC(084e929f) SHA1(ff93e44f7a95932c055d26baea493cb126dfe399) )
	ROM_LOAD16_WORD_SWAP( "sfxjx.04a", 0x080000, 0x80000, CRC(9ea7a7c0) SHA1(1ef513be7ffa7c8881abfa107f95c26a85accb16) )
	ROM_LOAD16_WORD_SWAP( "sfxjx.05",  0x100000, 0x80000, CRC(46184b32) SHA1(f43015355962b64890067181e866e3a2da64e629) )
	ROM_LOAD16_WORD_SWAP( "sfxjx.06a", 0x180000, 0x80000, CRC(9877b7a4) SHA1(f20ddc959215490ab3cbb0dc900ac956bb69ce52) )
	ROM_LOAD16_WORD_SWAP( "sfxjx.07",  0x200000, 0x80000, CRC(eb8c5317) SHA1(9d75d79173883cc533e12ae966c41a9946028aee) )
	ROM_LOAD16_WORD_SWAP( "sfxjx.08",  0x280000, 0x80000, CRC(656a9858) SHA1(58e2364a945cc0a52079cb170e72abd436660610) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROMX_LOAD( "ssf.13",   0x000000, 0x200000, CRC(cf94d275) SHA1(bf2a6d98a656d1cb5734da7836686242d3211137) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.15",   0x000002, 0x200000, CRC(5eb703af) SHA1(4b302dbb66e8a5c2ad92798699391e981bada427) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.17",   0x000004, 0x200000, CRC(ffa60e0f) SHA1(b21b1c749a8241440879bf8e7cb33968ccef97e5) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.19",   0x000006, 0x200000, CRC(34e825c5) SHA1(4d320fc96d1ef0b9928a8ce801734245a4c097a5) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.14",   0x800000, 0x100000, CRC(b7cc32e7) SHA1(0f4d26af338dab5dce5b7b34d32ad0c573434ace) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.16",   0x800002, 0x100000, CRC(8376ad18) SHA1(f4456833fb396e6501f4174c0fe5fd63ea40a188) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.18",   0x800004, 0x100000, CRC(f5b1b336) SHA1(4b060501e56b9d61294748da5387cdae5280ec4d) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.20",   0x800006, 0x100000, CRC(459d5c6b) SHA1(32b11ba7a12004aff810d719bff7508204c7b7c0) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfx.21",   0xc00000, 0x100000, CRC(e32854af) SHA1(1a5e11e9caa2b96108d89ae660ef1f6bcb469a74) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfx.23",   0xc00002, 0x100000, CRC(760f2927) SHA1(491e28e14ee06821fc9e709efa7b91313bc0c2db) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfx.25",   0xc00004, 0x100000, CRC(1ee90208) SHA1(83df1d9953560edddc2951ea426d29fb014e6a8a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfx.27",   0xc00006, 0x100000, CRC(f814400f) SHA1(ad6921af36d0bd5dfb89b1fb53c3ca3fd92d7204) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sfx.01",   0x00000, 0x08000, CRC(b47b8835) SHA1(c8b2d50fe3a329bd0592ea160d505155d873dab1) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "sfx.02",   0x28000, 0x20000, CRC(0022633f) SHA1(cab3afc79da53e3887eb1ccd1f4d19790728e6cd) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "sfx.11",   0x000000, 0x200000, CRC(9bdbd476) SHA1(a8520f77f30b97aae36408e0c4ca1ebbde1808a5) )
	ROM_LOAD16_WORD_SWAP( "sfx.12",   0x200000, 0x200000, CRC(a05e3aab) SHA1(d4eb9cae66c74e956569fea8b815156fbd420f83) )
ROM_END

ROM_START( vhunt2 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "vh2j.03a", 0x000000, 0x80000, CRC(9ae8f186) SHA1(f4e3a1b6ae823737d3b18561469f206921b48587) )
	ROM_LOAD16_WORD_SWAP( "vh2j.04a", 0x080000, 0x80000, CRC(e2fabf53) SHA1(78c53f8e984b00245486b751515248879df77437) )
	ROM_LOAD16_WORD_SWAP( "vh2j.05",  0x100000, 0x80000, CRC(de34f624) SHA1(60bbbd1765e76839b01c38765da2368c5188ec61) )
	ROM_LOAD16_WORD_SWAP( "vh2j.06",  0x180000, 0x80000, CRC(6a3b9897) SHA1(4f3b37004db8a3d3dde709b51c94c392615134b5) )
	ROM_LOAD16_WORD_SWAP( "vh2j.07",  0x200000, 0x80000, CRC(b021c029) SHA1(de4299197600608e83fe50775e3f352f5add844d) )
	ROM_LOAD16_WORD_SWAP( "vh2j.08",  0x280000, 0x80000, CRC(ac873dff) SHA1(ad9a085b8403801035683b6f63eee33daf4e97ae) )
	ROM_LOAD16_WORD_SWAP( "vh2j.09",  0x300000, 0x80000, CRC(eaefce9c) SHA1(d842a824f0d0adc13a86f780084164c1273c45a4) )
	ROM_LOAD16_WORD_SWAP( "vh2j.10",  0x380000, 0x80000, CRC(11730952) SHA1(2966b80b99ab065614a6ddb546110f482b998e32) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
/*	ROM_LOAD16_WORD_SWAP( "vh2jx.03a", 0x000000, 0x80000, NO_DUMP )*/
/*	ROM_LOAD16_WORD_SWAP( "vh2jx.04a", 0x080000, 0x80000, NO_DUMP )*/

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "vh2.13",   0x0000000, 0x400000, CRC(3b02ddaa) SHA1(a73b0554afbfc7ace41bdf8e6cafd4c1ef0b0a08) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vh2.15",   0x0000002, 0x400000, CRC(4e40de66) SHA1(e8b80eadffad6070aa04c8ab426311c44e7c5507) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vh2.17",   0x0000004, 0x400000, CRC(b31d00c9) SHA1(7e7be64690663f52d10c8946aabec4250c8a8740) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vh2.19",   0x0000006, 0x400000, CRC(149be3ab) SHA1(afc8e96e6aa3cf1db6dfd8075030a6c50b4419a9) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vh2.14",   0x1000000, 0x400000, CRC(cd09bd63) SHA1(e582b20a948ae54f52590496051688dbfae2bc9c) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vh2.16",   0x1000002, 0x400000, CRC(e0182c15) SHA1(a924d53ab39f4d85173bdb92a197dde2db0dc3f7) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vh2.18",   0x1000004, 0x400000, CRC(778dc4f6) SHA1(8d0cd1c387b4b6ac7f92bb2e5a25983856328cdc) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vh2.20",   0x1000006, 0x400000, CRC(605d9d1d) SHA1(99bc27557741527ca678d7b6307164bc04ebedc6) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "vh2.01",  0x00000, 0x08000, CRC(67b9f779) SHA1(3994c65f888004b56ea9f478b1feaa81e306347e) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "vh2.02",  0x28000, 0x20000, CRC(aaf15fcb) SHA1(6f61daa162c835165a8aabaf1d0ea8816fbfbd40) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "vh2.11",  0x000000, 0x400000, CRC(38922efd) SHA1(8cfb36bdce3a524d0a81fec12ca0cba82222fa30) )
	ROM_LOAD16_WORD_SWAP( "vh2.12",  0x400000, 0x400000, CRC(6e2430af) SHA1(b475faf943bec4171ba0130f287e1948743ca273) )
ROM_END

ROM_START( vhunt2r1 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "vh2j.03", 0x000000, 0x80000, CRC(1a5feb13) SHA1(a6dd6af2601e2da14032bcbf17e9f79c7a4ba2db) )
	ROM_LOAD16_WORD_SWAP( "vh2j.04", 0x080000, 0x80000, CRC(434611a5) SHA1(ee093017405db6c16bfee3fe446bae659c6accc2) )
	ROM_LOAD16_WORD_SWAP( "vh2j.05", 0x100000, 0x80000, CRC(de34f624) SHA1(60bbbd1765e76839b01c38765da2368c5188ec61) )
	ROM_LOAD16_WORD_SWAP( "vh2j.06", 0x180000, 0x80000, CRC(6a3b9897) SHA1(4f3b37004db8a3d3dde709b51c94c392615134b5) )
	ROM_LOAD16_WORD_SWAP( "vh2j.07", 0x200000, 0x80000, CRC(b021c029) SHA1(de4299197600608e83fe50775e3f352f5add844d) )
	ROM_LOAD16_WORD_SWAP( "vh2j.08", 0x280000, 0x80000, CRC(ac873dff) SHA1(ad9a085b8403801035683b6f63eee33daf4e97ae) )
	ROM_LOAD16_WORD_SWAP( "vh2j.09", 0x300000, 0x80000, CRC(eaefce9c) SHA1(d842a824f0d0adc13a86f780084164c1273c45a4) )
	ROM_LOAD16_WORD_SWAP( "vh2j.10", 0x380000, 0x80000, CRC(11730952) SHA1(2966b80b99ab065614a6ddb546110f482b998e32) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "vh2jx.03", 0x000000, 0x80000, CRC(7d5d7aae) SHA1(d2909d33e1b3133bd7c0802710ee54647d82b32c) )
	ROM_LOAD16_WORD_SWAP( "vh2jx.04", 0x080000, 0x80000, CRC(c66e9aea) SHA1(ded62ef1b03bda002b879ef520cf619bd938d1ba) )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "vh2.13",   0x0000000, 0x400000, CRC(3b02ddaa) SHA1(a73b0554afbfc7ace41bdf8e6cafd4c1ef0b0a08) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vh2.15",   0x0000002, 0x400000, CRC(4e40de66) SHA1(e8b80eadffad6070aa04c8ab426311c44e7c5507) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vh2.17",   0x0000004, 0x400000, CRC(b31d00c9) SHA1(7e7be64690663f52d10c8946aabec4250c8a8740) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vh2.19",   0x0000006, 0x400000, CRC(149be3ab) SHA1(afc8e96e6aa3cf1db6dfd8075030a6c50b4419a9) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vh2.14",   0x1000000, 0x400000, CRC(cd09bd63) SHA1(e582b20a948ae54f52590496051688dbfae2bc9c) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vh2.16",   0x1000002, 0x400000, CRC(e0182c15) SHA1(a924d53ab39f4d85173bdb92a197dde2db0dc3f7) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vh2.18",   0x1000004, 0x400000, CRC(778dc4f6) SHA1(8d0cd1c387b4b6ac7f92bb2e5a25983856328cdc) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vh2.20",   0x1000006, 0x400000, CRC(605d9d1d) SHA1(99bc27557741527ca678d7b6307164bc04ebedc6) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "vh2.01",  0x00000, 0x08000, CRC(67b9f779) SHA1(3994c65f888004b56ea9f478b1feaa81e306347e) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "vh2.02",  0x28000, 0x20000, CRC(aaf15fcb) SHA1(6f61daa162c835165a8aabaf1d0ea8816fbfbd40) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "vh2.11",  0x000000, 0x400000, CRC(38922efd) SHA1(8cfb36bdce3a524d0a81fec12ca0cba82222fa30) )
	ROM_LOAD16_WORD_SWAP( "vh2.12",  0x400000, 0x400000, CRC(6e2430af) SHA1(b475faf943bec4171ba0130f287e1948743ca273) )
ROM_END

ROM_START( vsav )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "vm3e.03d", 0x000000, 0x80000, CRC(f5962a8c) SHA1(e37d48b78186c7c097894d6c17faf7c9333f61eb) )
	ROM_LOAD16_WORD_SWAP( "vm3e.04d", 0x080000, 0x80000, CRC(21b40ea2) SHA1(6790fa3e618850f518cbd470f44434a71be6f29f) )
	ROM_LOAD16_WORD_SWAP( "vm3.05a",  0x100000, 0x80000, CRC(4118e00f) SHA1(94ce8abc5ff547667f4c6022d84d0ed4cd062d7e) )
	ROM_LOAD16_WORD_SWAP( "vm3.06a",  0x180000, 0x80000, CRC(2f4fd3a9) SHA1(48549ff0121312ea4a18d0fa167a32f905c14c9f) )
	ROM_LOAD16_WORD_SWAP( "vm3.07b",  0x200000, 0x80000, CRC(cbda91b8) SHA1(31b20aa92422384b1d7a4706ad4c01ea2bd0e0d1) )
	ROM_LOAD16_WORD_SWAP( "vm3.08a",  0x280000, 0x80000, CRC(6ca47259) SHA1(485d8f3a132ccb3f7930cae74de8662d2d44e412) )
	ROM_LOAD16_WORD_SWAP( "vm3.09b",  0x300000, 0x80000, CRC(f4a339e3) SHA1(abd101a55f7d9ddb8aba04fe8d3f0f5d2006c925) )
	ROM_LOAD16_WORD_SWAP( "vm3.10b",  0x380000, 0x80000, CRC(fffbb5b8) SHA1(38aecb820bd1cbd17287848c3ffb013e1d464ddf) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "vm3ex.03d", 0x000000, 0x80000, CRC(fa586edf) SHA1(75a6b2c20a59c668d262121db4ba56464823e09b) )
	ROM_LOAD16_WORD_SWAP( "vm3ex.04d", 0x080000, 0x80000, CRC(eb912f2b) SHA1(4067e2c4ae5c014825d1283eba6a890845a5632c) )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "vm3.13",   0x0000000, 0x400000, CRC(fd8a11eb) SHA1(21b9773959e17976ff46b75a6a405042836b2c5f) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vm3.15",   0x0000002, 0x400000, CRC(dd1e7d4e) SHA1(30476e061cdebdb1838b83f4ebd5efae12b7dbfb) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vm3.17",   0x0000004, 0x400000, CRC(6b89445e) SHA1(2abd489839d143c46e25f4fc3db476b70607dc03) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vm3.19",   0x0000006, 0x400000, CRC(3830fdc7) SHA1(ebd3f559c254d349e256c9feb3477f1ed7518206) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vm3.14",   0x1000000, 0x400000, CRC(c1a28e6c) SHA1(012803af33174c0602649d2a2d84f6ee79f54ad2) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vm3.16",   0x1000002, 0x400000, CRC(194a7304) SHA1(a19a9a6fb829953b054dc5c3b0dc017f60d37928) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vm3.18",   0x1000004, 0x400000, CRC(df9a9f47) SHA1(ce29ff00cf4b6fdd9b3b1ed87823534f1d364eab) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vm3.20",   0x1000006, 0x400000, CRC(c22fc3d9) SHA1(df7538c05b03a4ad94d369f8083799979e6fac42) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "vm3.01",   0x00000, 0x08000, CRC(f778769b) SHA1(788ce1ad8a322179f634df9e62a31ad776b96762) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "vm3.02",   0x28000, 0x20000, CRC(cc09faa1) SHA1(2962ef0ceaf7e7279de3c421ea998763330eb43e) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "vm3.11",   0x000000, 0x400000, CRC(e80e956e) SHA1(74181fca4b764fb3c56ceef2cb4c6fd6c18ec4b6) )
	ROM_LOAD16_WORD_SWAP( "vm3.12",   0x400000, 0x400000, CRC(9cd71557) SHA1(7059db25698a0b286314c5961c618f6d2e6f24a1) )
ROM_END

ROM_START( vsavu )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "vm3u.03d", 0x000000, 0x80000, CRC(1f295274) SHA1(c926d8af4fccee5104507ee0196b05dcd419ee20) )
	ROM_LOAD16_WORD_SWAP( "vm3u.04d", 0x080000, 0x80000, CRC(c46adf81) SHA1(85ffb9b3282874d6ce9318a88429666e98f67cea) )
	ROM_LOAD16_WORD_SWAP( "vm3.05a",  0x100000, 0x80000, CRC(4118e00f) SHA1(94ce8abc5ff547667f4c6022d84d0ed4cd062d7e) )
	ROM_LOAD16_WORD_SWAP( "vm3.06a",  0x180000, 0x80000, CRC(2f4fd3a9) SHA1(48549ff0121312ea4a18d0fa167a32f905c14c9f) )
	ROM_LOAD16_WORD_SWAP( "vm3.07b",  0x200000, 0x80000, CRC(cbda91b8) SHA1(31b20aa92422384b1d7a4706ad4c01ea2bd0e0d1) )
	ROM_LOAD16_WORD_SWAP( "vm3.08a",  0x280000, 0x80000, CRC(6ca47259) SHA1(485d8f3a132ccb3f7930cae74de8662d2d44e412) )
	ROM_LOAD16_WORD_SWAP( "vm3.09b",  0x300000, 0x80000, CRC(f4a339e3) SHA1(abd101a55f7d9ddb8aba04fe8d3f0f5d2006c925) )
	ROM_LOAD16_WORD_SWAP( "vm3.10b",  0x380000, 0x80000, CRC(fffbb5b8) SHA1(38aecb820bd1cbd17287848c3ffb013e1d464ddf) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "vm3ux.03d", 0x000000, 0x80000, CRC(fb135627) SHA1(0ee29f632fb830ac477a276117e8cbb53d28717f) )
	ROM_LOAD16_WORD_SWAP( "vm3ux.04d", 0x080000, 0x80000, CRC(cf02f61d) SHA1(99fb941dca6d337679332c7d319721adac247c0e) )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "vm3.13",   0x0000000, 0x400000, CRC(fd8a11eb) SHA1(21b9773959e17976ff46b75a6a405042836b2c5f) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vm3.15",   0x0000002, 0x400000, CRC(dd1e7d4e) SHA1(30476e061cdebdb1838b83f4ebd5efae12b7dbfb) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vm3.17",   0x0000004, 0x400000, CRC(6b89445e) SHA1(2abd489839d143c46e25f4fc3db476b70607dc03) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vm3.19",   0x0000006, 0x400000, CRC(3830fdc7) SHA1(ebd3f559c254d349e256c9feb3477f1ed7518206) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vm3.14",   0x1000000, 0x400000, CRC(c1a28e6c) SHA1(012803af33174c0602649d2a2d84f6ee79f54ad2) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vm3.16",   0x1000002, 0x400000, CRC(194a7304) SHA1(a19a9a6fb829953b054dc5c3b0dc017f60d37928) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vm3.18",   0x1000004, 0x400000, CRC(df9a9f47) SHA1(ce29ff00cf4b6fdd9b3b1ed87823534f1d364eab) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vm3.20",   0x1000006, 0x400000, CRC(c22fc3d9) SHA1(df7538c05b03a4ad94d369f8083799979e6fac42) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "vm3.01",   0x00000, 0x08000, CRC(f778769b) SHA1(788ce1ad8a322179f634df9e62a31ad776b96762) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "vm3.02",   0x28000, 0x20000, CRC(cc09faa1) SHA1(2962ef0ceaf7e7279de3c421ea998763330eb43e) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "vm3.11",   0x000000, 0x400000, CRC(e80e956e) SHA1(74181fca4b764fb3c56ceef2cb4c6fd6c18ec4b6) )
	ROM_LOAD16_WORD_SWAP( "vm3.12",   0x400000, 0x400000, CRC(9cd71557) SHA1(7059db25698a0b286314c5961c618f6d2e6f24a1) )
ROM_END

ROM_START( vsavj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "vm3j.03d", 0x000000, 0x80000, CRC(2a2e74a4) SHA1(e9e7bce3c2ad0c9eebcbcd5139979d1fa19187ad) )
	ROM_LOAD16_WORD_SWAP( "vm3j.04d", 0x080000, 0x80000, CRC(1c2427bc) SHA1(9047ac0ccd875d91a8ceafdf1ccf9d21c4c71644) )
	ROM_LOAD16_WORD_SWAP( "vm3j.05a", 0x100000, 0x80000, CRC(95ce88d5) SHA1(ba5e64c2551d97a71d2f4d7a78663aede4b722e8) )
	ROM_LOAD16_WORD_SWAP( "vm3j.06b", 0x180000, 0x80000, CRC(2c4297e0) SHA1(3a7103456ba3937f63c28dd42020cac1955b5741) )
	ROM_LOAD16_WORD_SWAP( "vm3j.07b", 0x200000, 0x80000, CRC(a38aaae7) SHA1(0a5719eb2b0bbde955f605b1057ed6a8eb54ad80) )
	ROM_LOAD16_WORD_SWAP( "vm3j.08a", 0x280000, 0x80000, CRC(5773e5c9) SHA1(551afc5d921f9ef1fe928ca83d072b6a6105ab0e) )
	ROM_LOAD16_WORD_SWAP( "vm3j.09b", 0x300000, 0x80000, CRC(d064f8b9) SHA1(09f77f7b466c147a5d894a4ec3b40bd068dfab26) )
	ROM_LOAD16_WORD_SWAP( "vm3j.10b", 0x380000, 0x80000, CRC(434518e9) SHA1(ce1c8557a9e6c5451ab41a96f01b0cd4ba02ea3e) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "vm3jx.03d", 0x000000, 0x80000, CRC(a9ab54df) SHA1(fecbfce14c334832846e5211207f211660c19f5d) )
	ROM_LOAD16_WORD_SWAP( "vm3jx.04d", 0x080000, 0x80000, CRC(20c4aa2d) SHA1(a75a8946c331de1f8e6730b47e95b03a39605115) )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "vm3.13",   0x0000000, 0x400000, CRC(fd8a11eb) SHA1(21b9773959e17976ff46b75a6a405042836b2c5f) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vm3.15",   0x0000002, 0x400000, CRC(dd1e7d4e) SHA1(30476e061cdebdb1838b83f4ebd5efae12b7dbfb) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vm3.17",   0x0000004, 0x400000, CRC(6b89445e) SHA1(2abd489839d143c46e25f4fc3db476b70607dc03) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vm3.19",   0x0000006, 0x400000, CRC(3830fdc7) SHA1(ebd3f559c254d349e256c9feb3477f1ed7518206) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vm3.14",   0x1000000, 0x400000, CRC(c1a28e6c) SHA1(012803af33174c0602649d2a2d84f6ee79f54ad2) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vm3.16",   0x1000002, 0x400000, CRC(194a7304) SHA1(a19a9a6fb829953b054dc5c3b0dc017f60d37928) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vm3.18",   0x1000004, 0x400000, CRC(df9a9f47) SHA1(ce29ff00cf4b6fdd9b3b1ed87823534f1d364eab) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vm3.20",   0x1000006, 0x400000, CRC(c22fc3d9) SHA1(df7538c05b03a4ad94d369f8083799979e6fac42) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "vm3.01",   0x00000, 0x08000, CRC(f778769b) SHA1(788ce1ad8a322179f634df9e62a31ad776b96762) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "vm3.02",   0x28000, 0x20000, CRC(cc09faa1) SHA1(2962ef0ceaf7e7279de3c421ea998763330eb43e) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "vm3.11",   0x000000, 0x400000, CRC(e80e956e) SHA1(74181fca4b764fb3c56ceef2cb4c6fd6c18ec4b6) )
	ROM_LOAD16_WORD_SWAP( "vm3.12",   0x400000, 0x400000, CRC(9cd71557) SHA1(7059db25698a0b286314c5961c618f6d2e6f24a1) )
ROM_END

ROM_START( vsava )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "vm3a.03d", 0x000000, 0x80000, CRC(44c1198f) SHA1(35714b2f6ebeafea93be6467b5b22ea41b9f3826) )
	ROM_LOAD16_WORD_SWAP( "vm3a.04d", 0x080000, 0x80000, CRC(2218b781) SHA1(5dd28cc1b70b2953fbd4d5fd14abeeb3b83b193e) )
	ROM_LOAD16_WORD_SWAP( "vm3.05a",  0x100000, 0x80000, CRC(4118e00f) SHA1(94ce8abc5ff547667f4c6022d84d0ed4cd062d7e) )
	ROM_LOAD16_WORD_SWAP( "vm3.06a",  0x180000, 0x80000, CRC(2f4fd3a9) SHA1(48549ff0121312ea4a18d0fa167a32f905c14c9f) )
	ROM_LOAD16_WORD_SWAP( "vm3.07b",  0x200000, 0x80000, CRC(cbda91b8) SHA1(31b20aa92422384b1d7a4706ad4c01ea2bd0e0d1) )
	ROM_LOAD16_WORD_SWAP( "vm3.08a",  0x280000, 0x80000, CRC(6ca47259) SHA1(485d8f3a132ccb3f7930cae74de8662d2d44e412) )
	ROM_LOAD16_WORD_SWAP( "vm3.09b",  0x300000, 0x80000, CRC(f4a339e3) SHA1(abd101a55f7d9ddb8aba04fe8d3f0f5d2006c925) )
	ROM_LOAD16_WORD_SWAP( "vm3.10b",  0x380000, 0x80000, CRC(fffbb5b8) SHA1(38aecb820bd1cbd17287848c3ffb013e1d464ddf) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "vm3ax.03d", 0x000000, 0x80000, CRC(3aea7d92) SHA1(733cb81b19c94feaf708d8de2e0811fc2e8bf008) )
	ROM_LOAD16_WORD_SWAP( "vm3ax.04d", 0x080000, 0x80000, CRC(b5697f3a) SHA1(525f659604261bfbdde8bcfbb2eb0b56d2d0a60b) )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "vm3.13",   0x0000000, 0x400000, CRC(fd8a11eb) SHA1(21b9773959e17976ff46b75a6a405042836b2c5f) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vm3.15",   0x0000002, 0x400000, CRC(dd1e7d4e) SHA1(30476e061cdebdb1838b83f4ebd5efae12b7dbfb) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vm3.17",   0x0000004, 0x400000, CRC(6b89445e) SHA1(2abd489839d143c46e25f4fc3db476b70607dc03) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vm3.19",   0x0000006, 0x400000, CRC(3830fdc7) SHA1(ebd3f559c254d349e256c9feb3477f1ed7518206) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vm3.14",   0x1000000, 0x400000, CRC(c1a28e6c) SHA1(012803af33174c0602649d2a2d84f6ee79f54ad2) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vm3.16",   0x1000002, 0x400000, CRC(194a7304) SHA1(a19a9a6fb829953b054dc5c3b0dc017f60d37928) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vm3.18",   0x1000004, 0x400000, CRC(df9a9f47) SHA1(ce29ff00cf4b6fdd9b3b1ed87823534f1d364eab) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vm3.20",   0x1000006, 0x400000, CRC(c22fc3d9) SHA1(df7538c05b03a4ad94d369f8083799979e6fac42) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "vm3.01",   0x00000, 0x08000, CRC(f778769b) SHA1(788ce1ad8a322179f634df9e62a31ad776b96762) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "vm3.02",   0x28000, 0x20000, CRC(cc09faa1) SHA1(2962ef0ceaf7e7279de3c421ea998763330eb43e) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "vm3.11",   0x000000, 0x400000, CRC(e80e956e) SHA1(74181fca4b764fb3c56ceef2cb4c6fd6c18ec4b6) )
	ROM_LOAD16_WORD_SWAP( "vm3.12",   0x400000, 0x400000, CRC(9cd71557) SHA1(7059db25698a0b286314c5961c618f6d2e6f24a1) )
ROM_END

ROM_START( vsavh )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "vm3h.03a", 0x000000, 0x80000, CRC(7cc62df8) SHA1(716ad31f0e253868a5b1b89943ddc980f130d5b7) )
	ROM_LOAD16_WORD_SWAP( "vm3h.04d", 0x080000, 0x80000, CRC(d716f3b5) SHA1(7900440071eafa4d1559b1fa8faefaa0588a65d5) )
	ROM_LOAD16_WORD_SWAP( "vm3.05a",  0x100000, 0x80000, CRC(4118e00f) SHA1(94ce8abc5ff547667f4c6022d84d0ed4cd062d7e) )
	ROM_LOAD16_WORD_SWAP( "vm3.06a",  0x180000, 0x80000, CRC(2f4fd3a9) SHA1(48549ff0121312ea4a18d0fa167a32f905c14c9f) )
	ROM_LOAD16_WORD_SWAP( "vm3.07b",  0x200000, 0x80000, CRC(cbda91b8) SHA1(31b20aa92422384b1d7a4706ad4c01ea2bd0e0d1) )
	ROM_LOAD16_WORD_SWAP( "vm3.08a",  0x280000, 0x80000, CRC(6ca47259) SHA1(485d8f3a132ccb3f7930cae74de8662d2d44e412) )
	ROM_LOAD16_WORD_SWAP( "vm3.09b",  0x300000, 0x80000, CRC(f4a339e3) SHA1(abd101a55f7d9ddb8aba04fe8d3f0f5d2006c925) )
	ROM_LOAD16_WORD_SWAP( "vm3.10b",  0x380000, 0x80000, CRC(fffbb5b8) SHA1(38aecb820bd1cbd17287848c3ffb013e1d464ddf) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
/*	ROM_LOAD16_WORD_SWAP( "vm3hx.03a", 0x000000, 0x80000, NO_DUMP )*/
/*	ROM_LOAD16_WORD_SWAP( "vm3hx.04d", 0x080000, 0x80000, NO_DUMP )*/

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "vm3.13",   0x0000000, 0x400000, CRC(fd8a11eb) SHA1(21b9773959e17976ff46b75a6a405042836b2c5f) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vm3.15",   0x0000002, 0x400000, CRC(dd1e7d4e) SHA1(30476e061cdebdb1838b83f4ebd5efae12b7dbfb) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vm3.17",   0x0000004, 0x400000, CRC(6b89445e) SHA1(2abd489839d143c46e25f4fc3db476b70607dc03) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vm3.19",   0x0000006, 0x400000, CRC(3830fdc7) SHA1(ebd3f559c254d349e256c9feb3477f1ed7518206) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vm3.14",   0x1000000, 0x400000, CRC(c1a28e6c) SHA1(012803af33174c0602649d2a2d84f6ee79f54ad2) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vm3.16",   0x1000002, 0x400000, CRC(194a7304) SHA1(a19a9a6fb829953b054dc5c3b0dc017f60d37928) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vm3.18",   0x1000004, 0x400000, CRC(df9a9f47) SHA1(ce29ff00cf4b6fdd9b3b1ed87823534f1d364eab) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vm3.20",   0x1000006, 0x400000, CRC(c22fc3d9) SHA1(df7538c05b03a4ad94d369f8083799979e6fac42) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "vm3.01",   0x00000, 0x08000, CRC(f778769b) SHA1(788ce1ad8a322179f634df9e62a31ad776b96762) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "vm3.02",   0x28000, 0x20000, CRC(cc09faa1) SHA1(2962ef0ceaf7e7279de3c421ea998763330eb43e) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "vm3.11",   0x000000, 0x400000, CRC(e80e956e) SHA1(74181fca4b764fb3c56ceef2cb4c6fd6c18ec4b6) )
	ROM_LOAD16_WORD_SWAP( "vm3.12",   0x400000, 0x400000, CRC(9cd71557) SHA1(7059db25698a0b286314c5961c618f6d2e6f24a1) )
ROM_END

ROM_START( vsav2 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "vs2j.03", 0x000000, 0x80000, CRC(89fd86b4) SHA1(a52f40618d7f12f1df5862ad8e15fea60bef22a2) )
	ROM_LOAD16_WORD_SWAP( "vs2j.04", 0x080000, 0x80000, CRC(107c091b) SHA1(bf5c2e4339e1a66b3c819900cc9b723a537adf6b) )
	ROM_LOAD16_WORD_SWAP( "vs2j.05", 0x100000, 0x80000, CRC(61979638) SHA1(4d5625a9a06926c1a42c8f6e3a4c943f17750ec2) )
	ROM_LOAD16_WORD_SWAP( "vs2j.06", 0x180000, 0x80000, CRC(f37c5bc2) SHA1(d8c1040a6ee6b9fc677a6a32b99bf02b6a707812) )
	ROM_LOAD16_WORD_SWAP( "vs2j.07", 0x200000, 0x80000, CRC(8f885809) SHA1(69dac07e1f483b6478f792d20a137d6a081fbea3) )
	ROM_LOAD16_WORD_SWAP( "vs2j.08", 0x280000, 0x80000, CRC(2018c120) SHA1(de1184ab771c6f075cdefa744a28b09f78d91643) )
	ROM_LOAD16_WORD_SWAP( "vs2j.09", 0x300000, 0x80000, CRC(fac3c217) SHA1(0e9dd54e401e6d7c4fe81107ffd27e42ca810fcb) )
	ROM_LOAD16_WORD_SWAP( "vs2j.10", 0x380000, 0x80000, CRC(eb490213) SHA1(bf0416df66a33c7a4678ab4a047de334dfd3b31e) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "vs2jx.03", 0x000000, 0x80000, CRC(8f83159a) SHA1(d7e74cf23697f56daa4adba6cdf27b6424c70898) )
	ROM_LOAD16_WORD_SWAP( "vs2jx.04", 0x080000, 0x80000, CRC(e9822de8) SHA1(06fabf8b6532d710e3cd0f9c625198b39c9b7cce) )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "vs2.13",   0x0000000, 0x400000, CRC(5c852f52) SHA1(528ce7fc9a0451e2e2d221dbf5e4a5796584e053) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vs2.15",   0x0000002, 0x400000, CRC(a20f58af) SHA1(e873ad3e0fc8a06a5029113faf991f5c1b765316) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vs2.17",   0x0000004, 0x400000, CRC(39db59ad) SHA1(da94f1529da82a6bf2129f51548412e1ab2b001a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vs2.19",   0x0000006, 0x400000, CRC(00c763a7) SHA1(0ff528e12e255ebf699101ac71f05b1f6bef7165) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vs2.14",   0x1000000, 0x400000, CRC(cd09bd63) SHA1(e582b20a948ae54f52590496051688dbfae2bc9c) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vs2.16",   0x1000002, 0x400000, CRC(e0182c15) SHA1(a924d53ab39f4d85173bdb92a197dde2db0dc3f7) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vs2.18",   0x1000004, 0x400000, CRC(778dc4f6) SHA1(8d0cd1c387b4b6ac7f92bb2e5a25983856328cdc) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vs2.20",   0x1000006, 0x400000, CRC(605d9d1d) SHA1(99bc27557741527ca678d7b6307164bc04ebedc6) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "vs2.01",   0x00000, 0x08000, CRC(35190139) SHA1(07f8e53ea398461de5dcda9814dde7c09faf9f65) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "vs2.02",   0x28000, 0x20000, CRC(c32dba09) SHA1(1fe337ff334fab79847f9677ba0e168e93daa1c8) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "vs2.11",   0x000000, 0x400000, CRC(d67e47b7) SHA1(15a3f6779eccb10551ed94edf7e6e406a79b3de7) )
	ROM_LOAD16_WORD_SWAP( "vs2.12",   0x400000, 0x400000, CRC(6d020a14) SHA1(e98f862fac1e357c90949768bb2646263d9981a0) )
ROM_END

ROM_START( xmcota )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "xmne.03e", 0x000000, 0x80000, CRC(a9a09b09) SHA1(e316f443d393139894592dbb1b676f3a2385ed14) )
	ROM_LOAD16_WORD_SWAP( "xmne.04e", 0x080000, 0x80000, CRC(52fa2106) SHA1(6904eef0fb11e44046e160a1c0ff6ea48337f630) )
	ROM_LOAD16_WORD_SWAP( "xmn.05a",  0x100000, 0x80000, CRC(ac0d7759) SHA1(650d4474b13f16af7910a0f721fcda2ddb2414fd) )
	ROM_LOAD16_WORD_SWAP( "xmn.06a",  0x180000, 0x80000, CRC(1b86a328) SHA1(2469cd705139ee9f1142e6e379e68d0c9675b37e) )
	ROM_LOAD16_WORD_SWAP( "xmn.07a",  0x200000, 0x80000, CRC(2c142a44) SHA1(7624875f9c39b361fc83e52e87e0fd5e96279713) )
	ROM_LOAD16_WORD_SWAP( "xmn.08a",  0x280000, 0x80000, CRC(f712d44f) SHA1(0d18d4a4eacad94a66beca6ec509ac7f690c6882) )
	ROM_LOAD16_WORD_SWAP( "xmn.09a",  0x300000, 0x80000, CRC(9241cae8) SHA1(bb6980abf25aaf3eb14e230ca6942f3e2ab2c660) )
	ROM_LOAD16_WORD_SWAP( "xmn.10a",  0x380000, 0x80000, CRC(53c0eab9) SHA1(e3b1ec1fd517735f7801cfebb257c43185c6d3fb) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "xmnex.03e", 0x000000, 0x80000, CRC(2f5daa9f) SHA1(16a25c45ab2a267b402539c5b9b6dbaf8bcc56ba) )
	ROM_LOAD16_WORD_SWAP( "xmnex.04e", 0x080000, 0x80000, CRC(f0e24605) SHA1(c72105491d9c1d97286eea09c9ac506fa234a776) )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "xmn.13",   0x0000000, 0x400000, CRC(bf4df073) SHA1(4d2740c3a827f0ec2cf75ad99c65e393c6a11c23) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.15",   0x0000002, 0x400000, CRC(4d7e4cef) SHA1(50b8797b8099a8d76ad063ba1201a13dbb88ae3a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.17",   0x0000004, 0x400000, CRC(513eea17) SHA1(a497477ad9ac13180911d8745ef6ee1955c0b877) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.19",   0x0000006, 0x400000, CRC(d23897fc) SHA1(1e31627999736652252164d32662779a1ac6ca29) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.14",   0x1000000, 0x400000, CRC(778237b7) SHA1(89a759ec383518ec52f5059d10ec342f2247aa20) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.16",   0x1000002, 0x400000, CRC(67b36948) SHA1(692fb6e4096b880aa22996d554b160f664bbd907) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.18",   0x1000004, 0x400000, CRC(015a7c4c) SHA1(cccc95dafd076a1a9fa004710006149c42d058ba) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.20",   0x1000006, 0x400000, CRC(9dde2758) SHA1(17ba259cad03c7b5d56c0a5eda9ab53521665729) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "xmn.01a",  0x00000, 0x08000, CRC(40f479ea) SHA1(f29e15f537675305264ae2138a0a537fb9e2008b) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "xmn.02a",  0x28000, 0x20000, CRC(39d9b5ad) SHA1(af502debfd36100d4fc971ed25fdf9d7121d6f18) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "xmn.11",   0x000000, 0x200000, CRC(c848a6bc) SHA1(ac8ac564d3c43225822f8bc330eba9f35b24b0a4) )
	ROM_LOAD16_WORD_SWAP( "xmn.12",   0x200000, 0x200000, CRC(729c188f) SHA1(3279774ad8aebbcf0fc779cdfcbe21044dd192ad) )
ROM_END

ROM_START( xmcotau )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "xmnu.03e", 0x000000, 0x80000, CRC(0bafeb0e) SHA1(170c819bd7ffafefb9b2a587509bdf2c0415474b) )
	ROM_LOAD16_WORD_SWAP( "xmnu.04e", 0x080000, 0x80000, CRC(c29bdae3) SHA1(c605a4fd90336459c7b24cd7b2b243eef10f6407) )
	ROM_LOAD16_WORD_SWAP( "xmn.05a",  0x100000, 0x80000, CRC(ac0d7759) SHA1(650d4474b13f16af7910a0f721fcda2ddb2414fd) )
	ROM_LOAD16_WORD_SWAP( "xmn.06a",  0x180000, 0x80000, CRC(1b86a328) SHA1(2469cd705139ee9f1142e6e379e68d0c9675b37e) )
	ROM_LOAD16_WORD_SWAP( "xmn.07a",  0x200000, 0x80000, CRC(2c142a44) SHA1(7624875f9c39b361fc83e52e87e0fd5e96279713) )
	ROM_LOAD16_WORD_SWAP( "xmn.08a",  0x280000, 0x80000, CRC(f712d44f) SHA1(0d18d4a4eacad94a66beca6ec509ac7f690c6882) )
	ROM_LOAD16_WORD_SWAP( "xmn.09a",  0x300000, 0x80000, CRC(9241cae8) SHA1(bb6980abf25aaf3eb14e230ca6942f3e2ab2c660) )
	ROM_LOAD16_WORD_SWAP( "xmn.10a",  0x380000, 0x80000, CRC(53c0eab9) SHA1(e3b1ec1fd517735f7801cfebb257c43185c6d3fb) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "xmnux.03e", 0x000000, 0x80000, CRC(27636ac7) SHA1(b1aee9afbc1feebc7bf810411175771cac1addfb) )
	ROM_LOAD16_WORD_SWAP( "xmnux.04e", 0x080000, 0x80000, CRC(0aed300c) SHA1(9f1b69ebb4c31cbf3ff195e9297da0742c487482) )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "xmn.13",   0x0000000, 0x400000, CRC(bf4df073) SHA1(4d2740c3a827f0ec2cf75ad99c65e393c6a11c23) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.15",   0x0000002, 0x400000, CRC(4d7e4cef) SHA1(50b8797b8099a8d76ad063ba1201a13dbb88ae3a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.17",   0x0000004, 0x400000, CRC(513eea17) SHA1(a497477ad9ac13180911d8745ef6ee1955c0b877) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.19",   0x0000006, 0x400000, CRC(d23897fc) SHA1(1e31627999736652252164d32662779a1ac6ca29) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.14",   0x1000000, 0x400000, CRC(778237b7) SHA1(89a759ec383518ec52f5059d10ec342f2247aa20) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.16",   0x1000002, 0x400000, CRC(67b36948) SHA1(692fb6e4096b880aa22996d554b160f664bbd907) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.18",   0x1000004, 0x400000, CRC(015a7c4c) SHA1(cccc95dafd076a1a9fa004710006149c42d058ba) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.20",   0x1000006, 0x400000, CRC(9dde2758) SHA1(17ba259cad03c7b5d56c0a5eda9ab53521665729) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "xmn.01a",  0x00000, 0x08000, CRC(40f479ea) SHA1(f29e15f537675305264ae2138a0a537fb9e2008b) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "xmn.02a",  0x28000, 0x20000, CRC(39d9b5ad) SHA1(af502debfd36100d4fc971ed25fdf9d7121d6f18) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "xmn.11",   0x000000, 0x200000, CRC(c848a6bc) SHA1(ac8ac564d3c43225822f8bc330eba9f35b24b0a4) )
	ROM_LOAD16_WORD_SWAP( "xmn.12",   0x200000, 0x200000, CRC(729c188f) SHA1(3279774ad8aebbcf0fc779cdfcbe21044dd192ad) )
ROM_END

ROM_START( xmcotah )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "xmnh.03", 0x000000, 0x80000, CRC(e4b85a90) SHA1(1eaf94ce42438eea45cd5c813f2859abf258dd3a) )
	ROM_LOAD16_WORD_SWAP( "xmnh.04", 0x080000, 0x80000, CRC(7dfe1406) SHA1(4ddc0a8947d78ce587220f8188c8a8f00c7372c4) )
	ROM_LOAD16_WORD_SWAP( "xmnh.05", 0x100000, 0x80000, CRC(87b0ed0f) SHA1(f4d78fdd9fcf864e909d9a2bb351b49a5f8ec7a0) )
	ROM_LOAD16_WORD_SWAP( "xmn.06a", 0x180000, 0x80000, CRC(1b86a328) SHA1(2469cd705139ee9f1142e6e379e68d0c9675b37e) )
	ROM_LOAD16_WORD_SWAP( "xmn.07a", 0x200000, 0x80000, CRC(2c142a44) SHA1(7624875f9c39b361fc83e52e87e0fd5e96279713) )
	ROM_LOAD16_WORD_SWAP( "xmn.08a", 0x280000, 0x80000, CRC(f712d44f) SHA1(0d18d4a4eacad94a66beca6ec509ac7f690c6882) )
	ROM_LOAD16_WORD_SWAP( "xmn.09a", 0x300000, 0x80000, CRC(9241cae8) SHA1(bb6980abf25aaf3eb14e230ca6942f3e2ab2c660) )
	ROM_LOAD16_WORD_SWAP( "xmnh.10", 0x380000, 0x80000, CRC(cb36b0a4) SHA1(f21e3f2da405dfe43843ad32d381ea51f5d2fdd7) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
/*	ROM_LOAD16_WORD_SWAP( "xmnhx.03", 0x000000, 0x80000, NO_DUMP )*/
/*	ROM_LOAD16_WORD_SWAP( "xmnhx.04", 0x080000, 0x80000, NO_DUMP )*/

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "xmn.13",   0x0000000, 0x400000, CRC(bf4df073) SHA1(4d2740c3a827f0ec2cf75ad99c65e393c6a11c23) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.15",   0x0000002, 0x400000, CRC(4d7e4cef) SHA1(50b8797b8099a8d76ad063ba1201a13dbb88ae3a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.17",   0x0000004, 0x400000, CRC(513eea17) SHA1(a497477ad9ac13180911d8745ef6ee1955c0b877) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.19",   0x0000006, 0x400000, CRC(d23897fc) SHA1(1e31627999736652252164d32662779a1ac6ca29) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.14",   0x1000000, 0x400000, CRC(778237b7) SHA1(89a759ec383518ec52f5059d10ec342f2247aa20) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.16",   0x1000002, 0x400000, CRC(67b36948) SHA1(692fb6e4096b880aa22996d554b160f664bbd907) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.18",   0x1000004, 0x400000, CRC(015a7c4c) SHA1(cccc95dafd076a1a9fa004710006149c42d058ba) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.20",   0x1000006, 0x400000, CRC(9dde2758) SHA1(17ba259cad03c7b5d56c0a5eda9ab53521665729) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "xmn.01a",  0x00000, 0x08000, CRC(40f479ea) SHA1(f29e15f537675305264ae2138a0a537fb9e2008b) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "xmn.02a",  0x28000, 0x20000, CRC(39d9b5ad) SHA1(af502debfd36100d4fc971ed25fdf9d7121d6f18) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "xmn.11",   0x000000, 0x200000, CRC(c848a6bc) SHA1(ac8ac564d3c43225822f8bc330eba9f35b24b0a4) )
	ROM_LOAD16_WORD_SWAP( "xmn.12",   0x200000, 0x200000, CRC(729c188f) SHA1(3279774ad8aebbcf0fc779cdfcbe21044dd192ad) )
ROM_END

ROM_START( xmcotaj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "xmnj.03b", 0x000000, 0x80000, CRC(c8175fb3) SHA1(ea25bd165f8794324a1e07719312798cf9742924) )
	ROM_LOAD16_WORD_SWAP( "xmnj.04b", 0x080000, 0x80000, CRC(54b3fba3) SHA1(47eaff5d36a45e4196f87ed3d02e54d5407e7962) )
	ROM_LOAD16_WORD_SWAP( "xmn.05",   0x100000, 0x80000, CRC(c3ed62a2) SHA1(4e3317d7ca981e33318822103a16e59f4ce20deb) )
	ROM_LOAD16_WORD_SWAP( "xmn.06",   0x180000, 0x80000, CRC(f03c52e1) SHA1(904b2312ee594f5ece0484cad0eed25cc758185e) )
	ROM_LOAD16_WORD_SWAP( "xmn.07",   0x200000, 0x80000, CRC(325626b1) SHA1(3f3a0aabbe5ffad8136ac91e0de785103b16059b) )
	ROM_LOAD16_WORD_SWAP( "xmn.08",   0x280000, 0x80000, CRC(7194ea10) SHA1(40a5892d816f24cbfd4c310792eeabf689c6fa7e) )
	ROM_LOAD16_WORD_SWAP( "xmn.09",   0x300000, 0x80000, CRC(ae946df3) SHA1(733671f76d766bda7110df9d338791cc5202b050) )
	ROM_LOAD16_WORD_SWAP( "xmn.10",   0x380000, 0x80000, CRC(32a6be1d) SHA1(8f5fcb33b528abed670b4fc3fa62431a6e033c56) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "xmnjx.03b", 0x000000, 0x80000, CRC(523c9589) SHA1(4bdfe221476be2c2a73249cc60254d3d4167f1c2) )
	ROM_LOAD16_WORD_SWAP( "xmnjx.04b", 0x080000, 0x80000, CRC(673765ba) SHA1(36fe1dc90ac3c3485b4687d9e99bf14429bb0e92) )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "xmn.13",   0x0000000, 0x400000, CRC(bf4df073) SHA1(4d2740c3a827f0ec2cf75ad99c65e393c6a11c23) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.15",   0x0000002, 0x400000, CRC(4d7e4cef) SHA1(50b8797b8099a8d76ad063ba1201a13dbb88ae3a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.17",   0x0000004, 0x400000, CRC(513eea17) SHA1(a497477ad9ac13180911d8745ef6ee1955c0b877) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.19",   0x0000006, 0x400000, CRC(d23897fc) SHA1(1e31627999736652252164d32662779a1ac6ca29) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.14",   0x1000000, 0x400000, CRC(778237b7) SHA1(89a759ec383518ec52f5059d10ec342f2247aa20) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.16",   0x1000002, 0x400000, CRC(67b36948) SHA1(692fb6e4096b880aa22996d554b160f664bbd907) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.18",   0x1000004, 0x400000, CRC(015a7c4c) SHA1(cccc95dafd076a1a9fa004710006149c42d058ba) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.20",   0x1000006, 0x400000, CRC(9dde2758) SHA1(17ba259cad03c7b5d56c0a5eda9ab53521665729) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "xmn.01a",  0x00000, 0x08000, CRC(40f479ea) SHA1(f29e15f537675305264ae2138a0a537fb9e2008b) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "xmn.02a",  0x28000, 0x20000, CRC(39d9b5ad) SHA1(af502debfd36100d4fc971ed25fdf9d7121d6f18) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "xmn.11",   0x000000, 0x200000, CRC(c848a6bc) SHA1(ac8ac564d3c43225822f8bc330eba9f35b24b0a4) )
	ROM_LOAD16_WORD_SWAP( "xmn.12",   0x200000, 0x200000, CRC(729c188f) SHA1(3279774ad8aebbcf0fc779cdfcbe21044dd192ad) )
ROM_END

ROM_START( xmcotaj1 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "xmnj.03a", 0x000000, 0x80000, CRC(00761611) SHA1(e780dbe1d21a0d5b6981f0395942c9fa59688113) )
	ROM_LOAD16_WORD_SWAP( "xmnj.04a", 0x080000, 0x80000, CRC(614d3f60) SHA1(2272ae243557562a0bc85d2cd2b37dd876f6902c) )
	ROM_LOAD16_WORD_SWAP( "xmn.05",   0x100000, 0x80000, CRC(c3ed62a2) SHA1(4e3317d7ca981e33318822103a16e59f4ce20deb) )
	ROM_LOAD16_WORD_SWAP( "xmn.06",   0x180000, 0x80000, CRC(f03c52e1) SHA1(904b2312ee594f5ece0484cad0eed25cc758185e) )
	ROM_LOAD16_WORD_SWAP( "xmn.07",   0x200000, 0x80000, CRC(325626b1) SHA1(3f3a0aabbe5ffad8136ac91e0de785103b16059b) )
	ROM_LOAD16_WORD_SWAP( "xmn.08",   0x280000, 0x80000, CRC(7194ea10) SHA1(40a5892d816f24cbfd4c310792eeabf689c6fa7e) )
	ROM_LOAD16_WORD_SWAP( "xmn.09",   0x300000, 0x80000, CRC(ae946df3) SHA1(733671f76d766bda7110df9d338791cc5202b050) )
	ROM_LOAD16_WORD_SWAP( "xmn.10",   0x380000, 0x80000, CRC(32a6be1d) SHA1(8f5fcb33b528abed670b4fc3fa62431a6e033c56) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "xmnjx.03a", 0x000000, 0x80000, CRC(515b9bf9) SHA1(57e4604e7e013d95629546988db2448896dc95f2) )
	ROM_LOAD16_WORD_SWAP( "xmnjx.04a", 0x080000, 0x80000, CRC(5419572b) SHA1(7f25cd7aaa30569b4e3889d9b6066bb6b638e91e) )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "xmn.13",   0x0000000, 0x400000, CRC(bf4df073) SHA1(4d2740c3a827f0ec2cf75ad99c65e393c6a11c23) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.15",   0x0000002, 0x400000, CRC(4d7e4cef) SHA1(50b8797b8099a8d76ad063ba1201a13dbb88ae3a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.17",   0x0000004, 0x400000, CRC(513eea17) SHA1(a497477ad9ac13180911d8745ef6ee1955c0b877) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.19",   0x0000006, 0x400000, CRC(d23897fc) SHA1(1e31627999736652252164d32662779a1ac6ca29) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.14",   0x1000000, 0x400000, CRC(778237b7) SHA1(89a759ec383518ec52f5059d10ec342f2247aa20) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.16",   0x1000002, 0x400000, CRC(67b36948) SHA1(692fb6e4096b880aa22996d554b160f664bbd907) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.18",   0x1000004, 0x400000, CRC(015a7c4c) SHA1(cccc95dafd076a1a9fa004710006149c42d058ba) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.20",   0x1000006, 0x400000, CRC(9dde2758) SHA1(17ba259cad03c7b5d56c0a5eda9ab53521665729) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "xmn.01a",  0x00000, 0x08000, CRC(40f479ea) SHA1(f29e15f537675305264ae2138a0a537fb9e2008b) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "xmn.02a",  0x28000, 0x20000, CRC(39d9b5ad) SHA1(af502debfd36100d4fc971ed25fdf9d7121d6f18) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "xmn.11",   0x000000, 0x200000, CRC(c848a6bc) SHA1(ac8ac564d3c43225822f8bc330eba9f35b24b0a4) )
	ROM_LOAD16_WORD_SWAP( "xmn.12",   0x200000, 0x200000, CRC(729c188f) SHA1(3279774ad8aebbcf0fc779cdfcbe21044dd192ad) )
ROM_END

ROM_START( xmcotajr )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "xmno.03a", 0x000000, 0x80000, CRC(7ab19acf) SHA1(ca02e58f1d713ee74c6c1515772da0ca26f9deb9) )
	ROM_LOAD16_WORD_SWAP( "xmno.04a", 0x080000, 0x80000, CRC(7615dd21) SHA1(f393c985ae1c7f378f9184fd2c8530b7494ba06d) )
	ROM_LOAD16_WORD_SWAP( "xmno.05a", 0x100000, 0x80000, CRC(0303d672) SHA1(4816b5ac6a9bf78665112d54a8f3569d590721b2) )
	ROM_LOAD16_WORD_SWAP( "xmno.06a", 0x180000, 0x80000, CRC(332839a5) SHA1(c7b80fad1130cc025de3fad372b727d360adc47b) )
	ROM_LOAD16_WORD_SWAP( "xmno.07",  0x200000, 0x80000, CRC(6255e8d5) SHA1(159f7983b93ee82c2012a3a6a9f451a521f98ed6) )
	ROM_LOAD16_WORD_SWAP( "xmno.08",  0x280000, 0x80000, CRC(b8ebe77c) SHA1(3ef06f19f2ba0aee8be9d9a9f0b1742f9ee1282a) )
	ROM_LOAD16_WORD_SWAP( "xmno.09",  0x300000, 0x80000, CRC(5440d950) SHA1(d5338718964b6f36655ddd62dbbe2bbfb44db114) )
	ROM_LOAD16_WORD_SWAP( "xmno.10a", 0x380000, 0x80000, CRC(b8296966) SHA1(b13496956d8288302bf5c9a7478d4791e41e1bfd) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "xmnox.03a", 0x000000, 0x80000, CRC(c2d26e40) SHA1(2cf214c3bc1f5060e4a7cec2c4cab97bca1269bc) )
	ROM_LOAD16_WORD_SWAP( "xmnox.04a", 0x080000, 0x80000, CRC(9fb6b396) SHA1(82f121e22bb345ed173e46e13b1f190a9a0f5c62) )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "xmn.13",   0x0000000, 0x400000, CRC(bf4df073) SHA1(4d2740c3a827f0ec2cf75ad99c65e393c6a11c23) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.15",   0x0000002, 0x400000, CRC(4d7e4cef) SHA1(50b8797b8099a8d76ad063ba1201a13dbb88ae3a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.17",   0x0000004, 0x400000, CRC(513eea17) SHA1(a497477ad9ac13180911d8745ef6ee1955c0b877) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.19",   0x0000006, 0x400000, CRC(d23897fc) SHA1(1e31627999736652252164d32662779a1ac6ca29) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.14",   0x1000000, 0x400000, CRC(778237b7) SHA1(89a759ec383518ec52f5059d10ec342f2247aa20) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.16",   0x1000002, 0x400000, CRC(67b36948) SHA1(692fb6e4096b880aa22996d554b160f664bbd907) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.18",   0x1000004, 0x400000, CRC(015a7c4c) SHA1(cccc95dafd076a1a9fa004710006149c42d058ba) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.20",   0x1000006, 0x400000, CRC(9dde2758) SHA1(17ba259cad03c7b5d56c0a5eda9ab53521665729) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "xmn.01",   0x00000, 0x08000, CRC(7178336e) SHA1(d94cddcc144336fa3ee2778b3531badcc4646e9d) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "xmn.02",   0x28000, 0x20000, CRC(0ec58501) SHA1(3af500049f901897086bd35b83ca83f4bbc8b3f6) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "xmn.11",   0x000000, 0x200000, CRC(c848a6bc) SHA1(ac8ac564d3c43225822f8bc330eba9f35b24b0a4) )
	ROM_LOAD16_WORD_SWAP( "xmn.12",   0x200000, 0x200000, CRC(729c188f) SHA1(3279774ad8aebbcf0fc779cdfcbe21044dd192ad) )
ROM_END

ROM_START( xmcotaa )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "xmna.03a", 0x000000, 0x80000, CRC(7df8b27e) SHA1(2f0ce6b10857e04ddaf7a76edf126282c53511b3) )
	ROM_LOAD16_WORD_SWAP( "xmna.04a", 0x080000, 0x80000, CRC(b44e30a7) SHA1(27b0a8b06aa11673dd145717c6286eb27186cf79) )
	ROM_LOAD16_WORD_SWAP( "xmn.05",   0x100000, 0x80000, CRC(c3ed62a2) SHA1(4e3317d7ca981e33318822103a16e59f4ce20deb) )
	ROM_LOAD16_WORD_SWAP( "xmn.06",   0x180000, 0x80000, CRC(f03c52e1) SHA1(904b2312ee594f5ece0484cad0eed25cc758185e) )
	ROM_LOAD16_WORD_SWAP( "xmn.07",   0x200000, 0x80000, CRC(325626b1) SHA1(3f3a0aabbe5ffad8136ac91e0de785103b16059b) )
	ROM_LOAD16_WORD_SWAP( "xmn.08",   0x280000, 0x80000, CRC(7194ea10) SHA1(40a5892d816f24cbfd4c310792eeabf689c6fa7e) )
	ROM_LOAD16_WORD_SWAP( "xmn.09",   0x300000, 0x80000, CRC(ae946df3) SHA1(733671f76d766bda7110df9d338791cc5202b050) )
	ROM_LOAD16_WORD_SWAP( "xmn.10",   0x380000, 0x80000, CRC(32a6be1d) SHA1(8f5fcb33b528abed670b4fc3fa62431a6e033c56) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "xmnax.03a", 0x000000, 0x80000, CRC(978a0de4) SHA1(cdf3542f03cbf373efc8596ac886566b3458647d) )
	ROM_LOAD16_WORD_SWAP( "xmnax.04a", 0x080000, 0x80000, CRC(07cb0839) SHA1(1aaf5ba8e256107ce8ef7dbd961b15149dc6d11b) )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "xmn.13",   0x0000000, 0x400000, CRC(bf4df073) SHA1(4d2740c3a827f0ec2cf75ad99c65e393c6a11c23) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.15",   0x0000002, 0x400000, CRC(4d7e4cef) SHA1(50b8797b8099a8d76ad063ba1201a13dbb88ae3a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.17",   0x0000004, 0x400000, CRC(513eea17) SHA1(a497477ad9ac13180911d8745ef6ee1955c0b877) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.19",   0x0000006, 0x400000, CRC(d23897fc) SHA1(1e31627999736652252164d32662779a1ac6ca29) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.14",   0x1000000, 0x400000, CRC(778237b7) SHA1(89a759ec383518ec52f5059d10ec342f2247aa20) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.16",   0x1000002, 0x400000, CRC(67b36948) SHA1(692fb6e4096b880aa22996d554b160f664bbd907) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.18",   0x1000004, 0x400000, CRC(015a7c4c) SHA1(cccc95dafd076a1a9fa004710006149c42d058ba) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.20",   0x1000006, 0x400000, CRC(9dde2758) SHA1(17ba259cad03c7b5d56c0a5eda9ab53521665729) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "xmn.01a",  0x00000, 0x08000, CRC(40f479ea) SHA1(f29e15f537675305264ae2138a0a537fb9e2008b) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "xmn.02a",  0x28000, 0x20000, CRC(39d9b5ad) SHA1(af502debfd36100d4fc971ed25fdf9d7121d6f18) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "xmn.11",   0x000000, 0x200000, CRC(c848a6bc) SHA1(ac8ac564d3c43225822f8bc330eba9f35b24b0a4) )
	ROM_LOAD16_WORD_SWAP( "xmn.12",   0x200000, 0x200000, CRC(729c188f) SHA1(3279774ad8aebbcf0fc779cdfcbe21044dd192ad) )
ROM_END

ROM_START( xmvsf )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "xvse.03d", 0x000000, 0x80000, CRC(5ae5bd3b) SHA1(f687f018008cef24f86f53373c3f5547741a4c5b) )
	ROM_LOAD16_WORD_SWAP( "xvse.04d", 0x080000, 0x80000, CRC(5eb9c02e) SHA1(25a392913213b98ce1bbd463bf5e5e10729bde0c) )
	ROM_LOAD16_WORD_SWAP( "xvs.05a",  0x100000, 0x80000, CRC(7db6025d) SHA1(2d74f48f83f45359bfaca28ab686625766af12ee) )
	ROM_LOAD16_WORD_SWAP( "xvs.06a",  0x180000, 0x80000, CRC(e8e2c75c) SHA1(929408cb5d98e95cec75ea58e4701b0cbdbcd016) )
	ROM_LOAD16_WORD_SWAP( "xvs.07",   0x200000, 0x80000, CRC(08f0abed) SHA1(ef16c376232dba63b0b9bc3aa0640f9001ccb68a) )
	ROM_LOAD16_WORD_SWAP( "xvs.08",   0x280000, 0x80000, CRC(81929675) SHA1(19cf7afbc1daaefec40195e40ba74970f3906a1c) )
	ROM_LOAD16_WORD_SWAP( "xvs.09",   0x300000, 0x80000, CRC(9641f36b) SHA1(dcba3482d1ba37ccfb30d402793ee063c6621aed) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "xvsex.03d", 0x000000, 0x80000, CRC(3b3e7836) SHA1(fca470d98a04f4391dee3dd3bb1cb9fa47999341) )
	ROM_LOAD16_WORD_SWAP( "xvsex.04d", 0x080000, 0x80000, CRC(108863ce) SHA1(5ffa4af1afa4ea3530927fabfc49bf95ba87c5c6) )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "xvs.13",   0x0000000, 0x400000, CRC(f6684efd) SHA1(c0a2f3a9e82ab8b084a500aec71ac633e947328c) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.15",   0x0000002, 0x400000, CRC(29109221) SHA1(898b8f678fd03c462ce0d8eb7fb3441ef601085b) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.17",   0x0000004, 0x400000, CRC(92db3474) SHA1(7b6f4c8ebfdac167b25f35029068b6253c141fe6) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.19",   0x0000006, 0x400000, CRC(3733473c) SHA1(6579da7145c95b3ad00844a5fc8c2e22c23365e2) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.14",   0x1000000, 0x400000, CRC(bcac2e41) SHA1(838ff24f7e8543a787a55a5d592c9517ce3b8b93) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.16",   0x1000002, 0x400000, CRC(ea04a272) SHA1(cd7c79037b5b4a39bef5156433e984dc4dc2c081) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.18",   0x1000004, 0x400000, CRC(b0def86a) SHA1(da3a6705ea7050fc5c2c10d33400ed67be9f455d) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.20",   0x1000006, 0x400000, CRC(4b40ff9f) SHA1(9a981d442132efff09a27408d74646ba357c7357) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "xvs.01",   0x00000, 0x08000, CRC(3999e93a) SHA1(fefcff8a9a5c83df7655a16187cf9ba3e7efbb25) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "xvs.02",   0x28000, 0x20000, CRC(101bdee9) SHA1(75920e88bf46fcd33a7957777a1d799818ffb0d6) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "xvs.11",   0x000000, 0x200000, CRC(9cadcdbc) SHA1(64d3bd53b04daec84c9af4aa3ff010867b3d306d) )
	ROM_LOAD16_WORD_SWAP( "xvs.12",   0x200000, 0x200000, CRC(7b11e460) SHA1(a581c84acaaf0ce056841c15a6f36889e88be68d) )
ROM_END

ROM_START( xmvsfu )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "xvsu.03k", 0x000000, 0x80000, CRC(8739ef61) SHA1(2eb5912d3026bed0f720d28e1bf3a7ceb5b80803) )
	ROM_LOAD16_WORD_SWAP( "xvsu.04k", 0x080000, 0x80000, CRC(e11d35c1) SHA1(d838199b2767d9f02fa0f103c5d587a4c78c0d21) )
	ROM_LOAD16_WORD_SWAP( "xvs.05a",  0x100000, 0x80000, CRC(7db6025d) SHA1(2d74f48f83f45359bfaca28ab686625766af12ee) )
	ROM_LOAD16_WORD_SWAP( "xvs.06a",  0x180000, 0x80000, CRC(e8e2c75c) SHA1(929408cb5d98e95cec75ea58e4701b0cbdbcd016) )
	ROM_LOAD16_WORD_SWAP( "xvs.07",   0x200000, 0x80000, CRC(08f0abed) SHA1(ef16c376232dba63b0b9bc3aa0640f9001ccb68a) )
	ROM_LOAD16_WORD_SWAP( "xvs.08",   0x280000, 0x80000, CRC(81929675) SHA1(19cf7afbc1daaefec40195e40ba74970f3906a1c) )
	ROM_LOAD16_WORD_SWAP( "xvs.09",   0x300000, 0x80000, CRC(9641f36b) SHA1(dcba3482d1ba37ccfb30d402793ee063c6621aed) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
/*	ROM_LOAD16_WORD_SWAP( "xvsux.03k", 0x000000, 0x80000, NO_DUMP )*/
/*	ROM_LOAD16_WORD_SWAP( "xvsux.04k", 0x080000, 0x80000, NO_DUMP )*/

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "xvs.13",   0x0000000, 0x400000, CRC(f6684efd) SHA1(c0a2f3a9e82ab8b084a500aec71ac633e947328c) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.15",   0x0000002, 0x400000, CRC(29109221) SHA1(898b8f678fd03c462ce0d8eb7fb3441ef601085b) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.17",   0x0000004, 0x400000, CRC(92db3474) SHA1(7b6f4c8ebfdac167b25f35029068b6253c141fe6) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.19",   0x0000006, 0x400000, CRC(3733473c) SHA1(6579da7145c95b3ad00844a5fc8c2e22c23365e2) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.14",   0x1000000, 0x400000, CRC(bcac2e41) SHA1(838ff24f7e8543a787a55a5d592c9517ce3b8b93) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.16",   0x1000002, 0x400000, CRC(ea04a272) SHA1(cd7c79037b5b4a39bef5156433e984dc4dc2c081) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.18",   0x1000004, 0x400000, CRC(b0def86a) SHA1(da3a6705ea7050fc5c2c10d33400ed67be9f455d) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.20",   0x1000006, 0x400000, CRC(4b40ff9f) SHA1(9a981d442132efff09a27408d74646ba357c7357) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "xvs.01",   0x00000, 0x08000, CRC(3999e93a) SHA1(fefcff8a9a5c83df7655a16187cf9ba3e7efbb25) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "xvs.02",   0x28000, 0x20000, CRC(101bdee9) SHA1(75920e88bf46fcd33a7957777a1d799818ffb0d6) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "xvs.11",   0x000000, 0x200000, CRC(9cadcdbc) SHA1(64d3bd53b04daec84c9af4aa3ff010867b3d306d) )
	ROM_LOAD16_WORD_SWAP( "xvs.12",   0x200000, 0x200000, CRC(7b11e460) SHA1(a581c84acaaf0ce056841c15a6f36889e88be68d) )
ROM_END

ROM_START( xmvsfur1 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "xvsu.03h", 0x000000, 0x80000, CRC(5481155a) SHA1(799a2488684cbead33206498d13261b79624a46e) )
	ROM_LOAD16_WORD_SWAP( "xvsu.04h", 0x080000, 0x80000, CRC(1e236388) SHA1(329c08103840fadbc4176785c4b24013a7a2b1bc) )
	ROM_LOAD16_WORD_SWAP( "xvs.05a",  0x100000, 0x80000, CRC(7db6025d) SHA1(2d74f48f83f45359bfaca28ab686625766af12ee) )
	ROM_LOAD16_WORD_SWAP( "xvs.06a",  0x180000, 0x80000, CRC(e8e2c75c) SHA1(929408cb5d98e95cec75ea58e4701b0cbdbcd016) )
	ROM_LOAD16_WORD_SWAP( "xvs.07",   0x200000, 0x80000, CRC(08f0abed) SHA1(ef16c376232dba63b0b9bc3aa0640f9001ccb68a) )
	ROM_LOAD16_WORD_SWAP( "xvs.08",   0x280000, 0x80000, CRC(81929675) SHA1(19cf7afbc1daaefec40195e40ba74970f3906a1c) )
	ROM_LOAD16_WORD_SWAP( "xvs.09",   0x300000, 0x80000, CRC(9641f36b) SHA1(dcba3482d1ba37ccfb30d402793ee063c6621aed) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "xvsux.03h", 0x000000, 0x80000, CRC(1539c639) SHA1(cf3535ca6fc1916458120b3151ee1f5c66aef942) )
	ROM_LOAD16_WORD_SWAP( "xvsux.04h", 0x080000, 0x80000, CRC(68916b3f) SHA1(ec8e109b30826387d567d0567a625c9fe5e7aa1f) )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "xvs.13",   0x0000000, 0x400000, CRC(f6684efd) SHA1(c0a2f3a9e82ab8b084a500aec71ac633e947328c) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.15",   0x0000002, 0x400000, CRC(29109221) SHA1(898b8f678fd03c462ce0d8eb7fb3441ef601085b) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.17",   0x0000004, 0x400000, CRC(92db3474) SHA1(7b6f4c8ebfdac167b25f35029068b6253c141fe6) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.19",   0x0000006, 0x400000, CRC(3733473c) SHA1(6579da7145c95b3ad00844a5fc8c2e22c23365e2) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.14",   0x1000000, 0x400000, CRC(bcac2e41) SHA1(838ff24f7e8543a787a55a5d592c9517ce3b8b93) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.16",   0x1000002, 0x400000, CRC(ea04a272) SHA1(cd7c79037b5b4a39bef5156433e984dc4dc2c081) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.18",   0x1000004, 0x400000, CRC(b0def86a) SHA1(da3a6705ea7050fc5c2c10d33400ed67be9f455d) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.20",   0x1000006, 0x400000, CRC(4b40ff9f) SHA1(9a981d442132efff09a27408d74646ba357c7357) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "xvs.01",   0x00000, 0x08000, CRC(3999e93a) SHA1(fefcff8a9a5c83df7655a16187cf9ba3e7efbb25) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "xvs.02",   0x28000, 0x20000, CRC(101bdee9) SHA1(75920e88bf46fcd33a7957777a1d799818ffb0d6) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "xvs.11",   0x000000, 0x200000, CRC(9cadcdbc) SHA1(64d3bd53b04daec84c9af4aa3ff010867b3d306d) )
	ROM_LOAD16_WORD_SWAP( "xvs.12",   0x200000, 0x200000, CRC(7b11e460) SHA1(a581c84acaaf0ce056841c15a6f36889e88be68d) )
ROM_END

ROM_START( xmvsfj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "xvsj.03i", 0x000000, 0x80000, CRC(ef24da96) SHA1(8f4a2a626a059bcf36048770153a9ffc85bba304) )
	ROM_LOAD16_WORD_SWAP( "xvsj.04i", 0x080000, 0x80000, CRC(70a59b35) SHA1(786d9b243373024735848f785503c6aa883b1c2f) )
	ROM_LOAD16_WORD_SWAP( "xvs.05a",  0x100000, 0x80000, CRC(7db6025d) SHA1(2d74f48f83f45359bfaca28ab686625766af12ee) )
	ROM_LOAD16_WORD_SWAP( "xvs.06a",  0x180000, 0x80000, CRC(e8e2c75c) SHA1(929408cb5d98e95cec75ea58e4701b0cbdbcd016) )
	ROM_LOAD16_WORD_SWAP( "xvs.07",   0x200000, 0x80000, CRC(08f0abed) SHA1(ef16c376232dba63b0b9bc3aa0640f9001ccb68a) )
	ROM_LOAD16_WORD_SWAP( "xvs.08",   0x280000, 0x80000, CRC(81929675) SHA1(19cf7afbc1daaefec40195e40ba74970f3906a1c) )
	ROM_LOAD16_WORD_SWAP( "xvs.09",   0x300000, 0x80000, CRC(9641f36b) SHA1(dcba3482d1ba37ccfb30d402793ee063c6621aed) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
/*	ROM_LOAD16_WORD_SWAP( "xvsjx.03i", 0x000000, 0x80000, NO_DUMP )*/
/*	ROM_LOAD16_WORD_SWAP( "xvsjx.04i", 0x080000, 0x80000, NO_DUMP )*/

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "xvs.13",   0x0000000, 0x400000, CRC(f6684efd) SHA1(c0a2f3a9e82ab8b084a500aec71ac633e947328c) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.15",   0x0000002, 0x400000, CRC(29109221) SHA1(898b8f678fd03c462ce0d8eb7fb3441ef601085b) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.17",   0x0000004, 0x400000, CRC(92db3474) SHA1(7b6f4c8ebfdac167b25f35029068b6253c141fe6) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.19",   0x0000006, 0x400000, CRC(3733473c) SHA1(6579da7145c95b3ad00844a5fc8c2e22c23365e2) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.14",   0x1000000, 0x400000, CRC(bcac2e41) SHA1(838ff24f7e8543a787a55a5d592c9517ce3b8b93) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.16",   0x1000002, 0x400000, CRC(ea04a272) SHA1(cd7c79037b5b4a39bef5156433e984dc4dc2c081) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.18",   0x1000004, 0x400000, CRC(b0def86a) SHA1(da3a6705ea7050fc5c2c10d33400ed67be9f455d) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.20",   0x1000006, 0x400000, CRC(4b40ff9f) SHA1(9a981d442132efff09a27408d74646ba357c7357) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "xvs.01",   0x00000, 0x08000, CRC(3999e93a) SHA1(fefcff8a9a5c83df7655a16187cf9ba3e7efbb25) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "xvs.02",   0x28000, 0x20000, CRC(101bdee9) SHA1(75920e88bf46fcd33a7957777a1d799818ffb0d6) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "xvs.11",   0x000000, 0x200000, CRC(9cadcdbc) SHA1(64d3bd53b04daec84c9af4aa3ff010867b3d306d) )
	ROM_LOAD16_WORD_SWAP( "xvs.12",   0x200000, 0x200000, CRC(7b11e460) SHA1(a581c84acaaf0ce056841c15a6f36889e88be68d) )
ROM_END

ROM_START( xmvsfjr1 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "xvsj.03d", 0x000000, 0x80000, CRC(beb81de9) SHA1(fce0d43b193a521d026be6508a91be6e2d03f480) )
	ROM_LOAD16_WORD_SWAP( "xvsj.04d", 0x080000, 0x80000, CRC(23d11271) SHA1(45e4ac52001f0c2b6cd6e07413b5e503c2b90329) )
	ROM_LOAD16_WORD_SWAP( "xvs.05a",  0x100000, 0x80000, CRC(7db6025d) SHA1(2d74f48f83f45359bfaca28ab686625766af12ee) )
	ROM_LOAD16_WORD_SWAP( "xvs.06a",  0x180000, 0x80000, CRC(e8e2c75c) SHA1(929408cb5d98e95cec75ea58e4701b0cbdbcd016) )
	ROM_LOAD16_WORD_SWAP( "xvs.07",   0x200000, 0x80000, CRC(08f0abed) SHA1(ef16c376232dba63b0b9bc3aa0640f9001ccb68a) )
	ROM_LOAD16_WORD_SWAP( "xvs.08",   0x280000, 0x80000, CRC(81929675) SHA1(19cf7afbc1daaefec40195e40ba74970f3906a1c) )
	ROM_LOAD16_WORD_SWAP( "xvs.09",   0x300000, 0x80000, CRC(9641f36b) SHA1(dcba3482d1ba37ccfb30d402793ee063c6621aed) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
/*	ROM_LOAD16_WORD_SWAP( "xvsjx.03d", 0x000000, 0x80000, NO_DUMP )*/
/*	ROM_LOAD16_WORD_SWAP( "xvsjx.04d", 0x080000, 0x80000, NO_DUMP )*/

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "xvs.13",   0x0000000, 0x400000, CRC(f6684efd) SHA1(c0a2f3a9e82ab8b084a500aec71ac633e947328c) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.15",   0x0000002, 0x400000, CRC(29109221) SHA1(898b8f678fd03c462ce0d8eb7fb3441ef601085b) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.17",   0x0000004, 0x400000, CRC(92db3474) SHA1(7b6f4c8ebfdac167b25f35029068b6253c141fe6) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.19",   0x0000006, 0x400000, CRC(3733473c) SHA1(6579da7145c95b3ad00844a5fc8c2e22c23365e2) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.14",   0x1000000, 0x400000, CRC(bcac2e41) SHA1(838ff24f7e8543a787a55a5d592c9517ce3b8b93) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.16",   0x1000002, 0x400000, CRC(ea04a272) SHA1(cd7c79037b5b4a39bef5156433e984dc4dc2c081) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.18",   0x1000004, 0x400000, CRC(b0def86a) SHA1(da3a6705ea7050fc5c2c10d33400ed67be9f455d) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.20",   0x1000006, 0x400000, CRC(4b40ff9f) SHA1(9a981d442132efff09a27408d74646ba357c7357) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "xvs.01",   0x00000, 0x08000, CRC(3999e93a) SHA1(fefcff8a9a5c83df7655a16187cf9ba3e7efbb25) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "xvs.02",   0x28000, 0x20000, CRC(101bdee9) SHA1(75920e88bf46fcd33a7957777a1d799818ffb0d6) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "xvs.11",   0x000000, 0x200000, CRC(9cadcdbc) SHA1(64d3bd53b04daec84c9af4aa3ff010867b3d306d) )
	ROM_LOAD16_WORD_SWAP( "xvs.12",   0x200000, 0x200000, CRC(7b11e460) SHA1(a581c84acaaf0ce056841c15a6f36889e88be68d) )
ROM_END

ROM_START( xmvsfjr2 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "xvsj.03c", 0x000000, 0x80000, CRC(180656a1) SHA1(aec2dfcfe8bcab03a48f749977e6f08fc21558bc) )
	ROM_LOAD16_WORD_SWAP( "xvsj.04c", 0x080000, 0x80000, CRC(5832811c) SHA1(e900b343241310d4dd1b45f42573e1e90f2dcbda) )
	ROM_LOAD16_WORD_SWAP( "xvs.05",   0x100000, 0x80000, CRC(030e0e1e) SHA1(164e3023bb1965768448e1bf6c45ff9e0ac964c7) )
	ROM_LOAD16_WORD_SWAP( "xvs.06",   0x180000, 0x80000, CRC(5d04a8ff) SHA1(3b5a524f3f1c4b540c88275418bdaf50c7186713) )
	ROM_LOAD16_WORD_SWAP( "xvs.07",   0x200000, 0x80000, CRC(08f0abed) SHA1(ef16c376232dba63b0b9bc3aa0640f9001ccb68a) )
	ROM_LOAD16_WORD_SWAP( "xvs.08",   0x280000, 0x80000, CRC(81929675) SHA1(19cf7afbc1daaefec40195e40ba74970f3906a1c) )
	ROM_LOAD16_WORD_SWAP( "xvs.09",   0x300000, 0x80000, CRC(9641f36b) SHA1(dcba3482d1ba37ccfb30d402793ee063c6621aed) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
/*	ROM_LOAD16_WORD_SWAP( "xvsjx.03c", 0x000000, 0x80000, NO_DUMP )*/
/*	ROM_LOAD16_WORD_SWAP( "xvsjx.04c", 0x080000, 0x80000, NO_DUMP )*/

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "xvs.13",   0x0000000, 0x400000, CRC(f6684efd) SHA1(c0a2f3a9e82ab8b084a500aec71ac633e947328c) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.15",   0x0000002, 0x400000, CRC(29109221) SHA1(898b8f678fd03c462ce0d8eb7fb3441ef601085b) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.17",   0x0000004, 0x400000, CRC(92db3474) SHA1(7b6f4c8ebfdac167b25f35029068b6253c141fe6) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.19",   0x0000006, 0x400000, CRC(3733473c) SHA1(6579da7145c95b3ad00844a5fc8c2e22c23365e2) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.14",   0x1000000, 0x400000, CRC(bcac2e41) SHA1(838ff24f7e8543a787a55a5d592c9517ce3b8b93) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.16",   0x1000002, 0x400000, CRC(ea04a272) SHA1(cd7c79037b5b4a39bef5156433e984dc4dc2c081) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.18",   0x1000004, 0x400000, CRC(b0def86a) SHA1(da3a6705ea7050fc5c2c10d33400ed67be9f455d) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.20",   0x1000006, 0x400000, CRC(4b40ff9f) SHA1(9a981d442132efff09a27408d74646ba357c7357) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "xvs.01",   0x00000, 0x08000, CRC(3999e93a) SHA1(fefcff8a9a5c83df7655a16187cf9ba3e7efbb25) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "xvs.02",   0x28000, 0x20000, CRC(101bdee9) SHA1(75920e88bf46fcd33a7957777a1d799818ffb0d6) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "xvs.11",   0x000000, 0x200000, CRC(9cadcdbc) SHA1(64d3bd53b04daec84c9af4aa3ff010867b3d306d) )
	ROM_LOAD16_WORD_SWAP( "xvs.12",   0x200000, 0x200000, CRC(7b11e460) SHA1(a581c84acaaf0ce056841c15a6f36889e88be68d) )
ROM_END

ROM_START( xmvsfa )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "xvsa.03", 0x000000, 0x80000, CRC(d0cca7a8) SHA1(70e0dd0725a52208e9e71fed82fba1d851a6bb42) )
	ROM_LOAD16_WORD_SWAP( "xvsa.04", 0x080000, 0x80000, CRC(8c8e76fd) SHA1(ac1c8200951131bea0bda417b6bc2f77130b5fdd) )
	ROM_LOAD16_WORD_SWAP( "xvs.05a", 0x100000, 0x80000, CRC(7db6025d) SHA1(2d74f48f83f45359bfaca28ab686625766af12ee) )
	ROM_LOAD16_WORD_SWAP( "xvs.06a", 0x180000, 0x80000, CRC(e8e2c75c) SHA1(929408cb5d98e95cec75ea58e4701b0cbdbcd016) )
	ROM_LOAD16_WORD_SWAP( "xvs.07",  0x200000, 0x80000, CRC(08f0abed) SHA1(ef16c376232dba63b0b9bc3aa0640f9001ccb68a) )
	ROM_LOAD16_WORD_SWAP( "xvs.08",  0x280000, 0x80000, CRC(81929675) SHA1(19cf7afbc1daaefec40195e40ba74970f3906a1c) )
	ROM_LOAD16_WORD_SWAP( "xvs.09",  0x300000, 0x80000, CRC(9641f36b) SHA1(dcba3482d1ba37ccfb30d402793ee063c6621aed) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
/*	ROM_LOAD16_WORD_SWAP( "xvsax.03", 0x000000, 0x80000, NO_DUMP )*/
/*	ROM_LOAD16_WORD_SWAP( "xvsax.04", 0x080000, 0x80000, NO_DUMP )*/

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "xvs.13",   0x0000000, 0x400000, CRC(f6684efd) SHA1(c0a2f3a9e82ab8b084a500aec71ac633e947328c) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.15",   0x0000002, 0x400000, CRC(29109221) SHA1(898b8f678fd03c462ce0d8eb7fb3441ef601085b) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.17",   0x0000004, 0x400000, CRC(92db3474) SHA1(7b6f4c8ebfdac167b25f35029068b6253c141fe6) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.19",   0x0000006, 0x400000, CRC(3733473c) SHA1(6579da7145c95b3ad00844a5fc8c2e22c23365e2) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.14",   0x1000000, 0x400000, CRC(bcac2e41) SHA1(838ff24f7e8543a787a55a5d592c9517ce3b8b93) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.16",   0x1000002, 0x400000, CRC(ea04a272) SHA1(cd7c79037b5b4a39bef5156433e984dc4dc2c081) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.18",   0x1000004, 0x400000, CRC(b0def86a) SHA1(da3a6705ea7050fc5c2c10d33400ed67be9f455d) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.20",   0x1000006, 0x400000, CRC(4b40ff9f) SHA1(9a981d442132efff09a27408d74646ba357c7357) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "xvs.01",   0x00000, 0x08000, CRC(3999e93a) SHA1(fefcff8a9a5c83df7655a16187cf9ba3e7efbb25) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "xvs.02",   0x28000, 0x20000, CRC(101bdee9) SHA1(75920e88bf46fcd33a7957777a1d799818ffb0d6) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "xvs.11",   0x000000, 0x200000, CRC(9cadcdbc) SHA1(64d3bd53b04daec84c9af4aa3ff010867b3d306d) )
	ROM_LOAD16_WORD_SWAP( "xvs.12",   0x200000, 0x200000, CRC(7b11e460) SHA1(a581c84acaaf0ce056841c15a6f36889e88be68d) )
ROM_END

ROM_START( xmvsfh )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "xvsh.03a", 0x000000, 0x80000, CRC(d4fffb04) SHA1(989ed975cfc1318998c2da26f450949bdac41d0c) )
	ROM_LOAD16_WORD_SWAP( "xvsh.04a", 0x080000, 0x80000, CRC(1b4ea638) SHA1(7523be63c1eef153e47fc8e1c10eb99ab40b94a0) )
	ROM_LOAD16_WORD_SWAP( "xvs.05a",  0x100000, 0x80000, CRC(7db6025d) SHA1(2d74f48f83f45359bfaca28ab686625766af12ee) )
	ROM_LOAD16_WORD_SWAP( "xvs.06a",  0x180000, 0x80000, CRC(e8e2c75c) SHA1(929408cb5d98e95cec75ea58e4701b0cbdbcd016) )
	ROM_LOAD16_WORD_SWAP( "xvs.07",   0x200000, 0x80000, CRC(08f0abed) SHA1(ef16c376232dba63b0b9bc3aa0640f9001ccb68a) )
	ROM_LOAD16_WORD_SWAP( "xvs.08",   0x280000, 0x80000, CRC(81929675) SHA1(19cf7afbc1daaefec40195e40ba74970f3906a1c) )
	ROM_LOAD16_WORD_SWAP( "xvs.09",   0x300000, 0x80000, CRC(9641f36b) SHA1(dcba3482d1ba37ccfb30d402793ee063c6621aed) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
/*	ROM_LOAD16_WORD_SWAP( "xvshx.03a", 0x000000, 0x80000, NO_DUMP )*/
/*	ROM_LOAD16_WORD_SWAP( "xvshx.04a", 0x080000, 0x80000, NO_DUMP )*/

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "xvs.13",   0x0000000, 0x400000, CRC(f6684efd) SHA1(c0a2f3a9e82ab8b084a500aec71ac633e947328c) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.15",   0x0000002, 0x400000, CRC(29109221) SHA1(898b8f678fd03c462ce0d8eb7fb3441ef601085b) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.17",   0x0000004, 0x400000, CRC(92db3474) SHA1(7b6f4c8ebfdac167b25f35029068b6253c141fe6) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.19",   0x0000006, 0x400000, CRC(3733473c) SHA1(6579da7145c95b3ad00844a5fc8c2e22c23365e2) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.14",   0x1000000, 0x400000, CRC(bcac2e41) SHA1(838ff24f7e8543a787a55a5d592c9517ce3b8b93) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.16",   0x1000002, 0x400000, CRC(ea04a272) SHA1(cd7c79037b5b4a39bef5156433e984dc4dc2c081) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.18",   0x1000004, 0x400000, CRC(b0def86a) SHA1(da3a6705ea7050fc5c2c10d33400ed67be9f455d) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.20",   0x1000006, 0x400000, CRC(4b40ff9f) SHA1(9a981d442132efff09a27408d74646ba357c7357) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "xvs.01",   0x00000, 0x08000, CRC(3999e93a) SHA1(fefcff8a9a5c83df7655a16187cf9ba3e7efbb25) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "xvs.02",   0x28000, 0x20000, CRC(101bdee9) SHA1(75920e88bf46fcd33a7957777a1d799818ffb0d6) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "xvs.11",   0x000000, 0x200000, CRC(9cadcdbc) SHA1(64d3bd53b04daec84c9af4aa3ff010867b3d306d) )
	ROM_LOAD16_WORD_SWAP( "xvs.12",   0x200000, 0x200000, CRC(7b11e460) SHA1(a581c84acaaf0ce056841c15a6f36889e88be68d) )
ROM_END

ROM_START( xmvsfb )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "xvsb.03h", 0x000000, 0x80000, CRC(05baccca) SHA1(7124e49e2180f77674ae014257a816cd4409d613) )
	ROM_LOAD16_WORD_SWAP( "xvsb.04h", 0x080000, 0x80000, CRC(e350c755) SHA1(5e615fd4b9954410c05b34151fae70d910340a6c) )
	ROM_LOAD16_WORD_SWAP( "xvs.05a",  0x100000, 0x80000, CRC(7db6025d) SHA1(2d74f48f83f45359bfaca28ab686625766af12ee) )
	ROM_LOAD16_WORD_SWAP( "xvs.06a",  0x180000, 0x80000, CRC(e8e2c75c) SHA1(929408cb5d98e95cec75ea58e4701b0cbdbcd016) )
	ROM_LOAD16_WORD_SWAP( "xvs.07",   0x200000, 0x80000, CRC(08f0abed) SHA1(ef16c376232dba63b0b9bc3aa0640f9001ccb68a) )
	ROM_LOAD16_WORD_SWAP( "xvs.08",   0x280000, 0x80000, CRC(81929675) SHA1(19cf7afbc1daaefec40195e40ba74970f3906a1c) )
	ROM_LOAD16_WORD_SWAP( "xvs.09",   0x300000, 0x80000, CRC(9641f36b) SHA1(dcba3482d1ba37ccfb30d402793ee063c6621aed) )

	ROM_REGION16_BE( CODE_SIZE, REGION_USER1, 0 )
/*	ROM_LOAD16_WORD_SWAP( "xvsbx.03h", 0x000000, 0x80000, NO_DUMP )*/
/*	ROM_LOAD16_WORD_SWAP( "xvsbx.04h", 0x080000, 0x80000, NO_DUMP )*/

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "xvs.13",   0x0000000, 0x400000, CRC(f6684efd) SHA1(c0a2f3a9e82ab8b084a500aec71ac633e947328c) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.15",   0x0000002, 0x400000, CRC(29109221) SHA1(898b8f678fd03c462ce0d8eb7fb3441ef601085b) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.17",   0x0000004, 0x400000, CRC(92db3474) SHA1(7b6f4c8ebfdac167b25f35029068b6253c141fe6) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.19",   0x0000006, 0x400000, CRC(3733473c) SHA1(6579da7145c95b3ad00844a5fc8c2e22c23365e2) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.14",   0x1000000, 0x400000, CRC(bcac2e41) SHA1(838ff24f7e8543a787a55a5d592c9517ce3b8b93) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.16",   0x1000002, 0x400000, CRC(ea04a272) SHA1(cd7c79037b5b4a39bef5156433e984dc4dc2c081) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.18",   0x1000004, 0x400000, CRC(b0def86a) SHA1(da3a6705ea7050fc5c2c10d33400ed67be9f455d) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.20",   0x1000006, 0x400000, CRC(4b40ff9f) SHA1(9a981d442132efff09a27408d74646ba357c7357) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "xvs.01",   0x00000, 0x08000, CRC(3999e93a) SHA1(fefcff8a9a5c83df7655a16187cf9ba3e7efbb25) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "xvs.02",   0x28000, 0x20000, CRC(101bdee9) SHA1(75920e88bf46fcd33a7957777a1d799818ffb0d6) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "xvs.11",   0x000000, 0x200000, CRC(9cadcdbc) SHA1(64d3bd53b04daec84c9af4aa3ff010867b3d306d) )
	ROM_LOAD16_WORD_SWAP( "xvs.12",   0x200000, 0x200000, CRC(7b11e460) SHA1(a581c84acaaf0ce056841c15a6f36889e88be68d) )
ROM_END

GAME( 1993, ssf2,     0,       cps2, ssf2,    cps2, ROT0,   "Capcom", "Super Street Fighter 2: The New Challengers (World 930911)" )
GAME( 1993, ssf2u,    ssf2,    cps2, ssf2,    cps2, ROT0,   "Capcom", "Super Street Fighter 2: The New Challengers (USA 930911)" )
GAME( 1993, ssf2a,    ssf2,    cps2, ssf2,    cps2, ROT0,   "Capcom", "Super Street Fighter 2: The New Challengers (Asia 931005)" )
GAME( 1993, ssf2ar1,  ssf2,    cps2, ssf2,    cps2, ROT0,   "Capcom", "Super Street Fighter 2: The New Challengers (Asia 930914)" )
GAME( 1993, ssf2j,    ssf2,    cps2, ssf2,    cps2, ROT0,   "Capcom", "Super Street Fighter 2: The New Challengers (Japan 931005)" )
GAME( 1993, ssf2jr1,  ssf2,    cps2, ssf2,    cps2, ROT0,   "Capcom", "Super Street Fighter 2: The New Challengers (Japan 930911)" )
GAME( 1993, ssf2jr2,  ssf2,    cps2, ssf2,    cps2, ROT0,   "Capcom", "Super Street Fighter 2: The New Challengers (Japan 930910)" )
GAMEX(1993, ssf2tb,   ssf2,    cps2, ssf2,    cps2, ROT0,   "Capcom", "Super Street Fighter 2: The Tournament Battle (World 931119)", GAME_NOT_WORKING )
GAMEX(1993, ssf2tbj,  ssf2,    cps2, ssf2,    cps2, ROT0,   "Capcom", "Super Street Fighter 2: The Tournament Battle (Japan 930910)", GAME_NOT_WORKING )
GAME( 1993, ddtod,    0,       cps2, ddtod,   cps2, ROT0,   "Capcom", "Dungeons and Dragons: Tower of Doom (Euro 940412)" )
GAME( 1993, ddtodu,   ddtod,   cps2, ddtod,   cps2, ROT0,   "Capcom", "Dungeons and Dragons: Tower of Doom (USA 940125)" )
GAME( 1993, ddtodur1, ddtod,   cps2, ddtod,   cps2, ROT0,   "Capcom", "Dungeons and Dragons: Tower of Doom (USA 940113)" )
GAME( 1993, ddtodj,   ddtod,   cps2, ddtod,   cps2, ROT0,   "Capcom", "Dungeons and Dragons: Tower of Doom (Japan 940113)" )
GAMEX(1993, ddtoda,   ddtod,   cps2, ddtod,   cps2, ROT0,   "Capcom", "Dungeons and Dragons: Tower of Doom (Asia 940113)", GAME_NOT_WORKING )
GAME( 1993, ddtodh,   ddtod,   cps2, ddtod,   cps2, ROT0,   "Capcom", "Dungeons and Dragons: Tower of Doom (Hispanic 940125)" )
GAME( 1993, ecofghtr, 0,       cps2, sgemf,   cps2, ROT0,   "Capcom", "Eco Fighters (World 931203)" )
GAMEX(1993, uecology, ecofghtr,cps2, sgemf,   cps2, ROT0,   "Capcom", "Ultimate Ecology (Japan 931203)", GAME_NOT_WORKING )
GAMEX(1993, ecofghta, ecofghtr,cps2, sgemf,   cps2, ROT0,   "Capcom", "Eco Fighters (Asia 931203)", GAME_NOT_WORKING )
GAME( 1994, ssf2t,    ssf2,    cps2, ssf2,    cps2, ROT0,   "Capcom", "Super Street Fighter 2 Turbo (World 940223)" )
GAME( 1994, ssf2ta,   ssf2,    cps2, ssf2,    cps2, ROT0,   "Capcom", "Super Street Fighter 2 Turbo (Asia 940223)" )
GAMEX(1994, ssf2tu,   ssf2,    cps2, ssf2,    cps2, ROT0,   "Capcom", "Super Street Fighter 2 Turbo (USA 940323)", GAME_NOT_WORKING )
GAME( 1994, ssf2tur1, ssf2,    cps2, ssf2,    cps2, ROT0,   "Capcom", "Super Street Fighter 2 Turbo (USA 940223)" )
GAME( 1994, ssf2xj,   ssf2,    cps2, ssf2,    cps2, ROT0,   "Capcom", "Super Street Fighter 2 X: Grand Master Challenge (Japan 940223)" )
GAME( 1994, xmcota,   0,       cps2, ssf2,    cps2, ROT0,   "Capcom", "X-Men: Children of the Atom (Euro 950105)" )
GAME( 1994, xmcotau,  xmcota,  cps2, ssf2,    cps2, ROT0,   "Capcom", "X-Men: Children of the Atom (USA 950105)" )
GAMEX(1994, xmcotah,  xmcota,  cps2, ssf2,    cps2, ROT0,   "Capcom", "X-Men: Children of the Atom (Hispanic 950331)", GAME_NOT_WORKING )
GAME( 1994, xmcotaj,  xmcota,  cps2, ssf2,    cps2, ROT0,   "Capcom", "X-Men: Children of the Atom (Japan 941219)" )
GAME( 1994, xmcotaj1, xmcota,  cps2, ssf2,    cps2, ROT0,   "Capcom", "X-Men: Children of the Atom (Japan 941217)" )
GAME( 1994, xmcotajr, xmcota,  cps2, ssf2,    cps2, ROT0,   "Capcom", "X-Men: Children of the Atom (Japan 941208 rent version)" )
GAME( 1994, xmcotaa,  xmcota,  cps2, ssf2,    cps2, ROT0,   "Capcom", "X-Men: Children of the Atom (Asia 941217)" )
GAME( 1994, armwar,   0,       cps2, avsp,    cps2, ROT0,   "Capcom", "Armored Warriors (Euro 941011)" )
GAME( 1994, armwaru,  armwar,  cps2, avsp,    cps2, ROT0,   "Capcom", "Armored Warriors (USA 941024)" )
GAME( 1994, pgear,    armwar,  cps2, avsp,    cps2, ROT0,   "Capcom", "Powered Gear: Strategic Variant Armor Equipment (Japan 941024)" )
GAME( 1994, pgearr1,  armwar,  cps2, avsp,    cps2, ROT0,   "Capcom", "Powered Gear: Strategic Variant Armor Equipment (Japan 940916)" )
GAMEX(1994, armwara,  armwar,  cps2, avsp,    cps2, ROT0,   "Capcom", "Armored Warriors (Asia 940920)", GAME_NOT_WORKING )
GAME (1994, avsp,     0,       cps2, avsp,    cps2, ROT0,   "Capcom", "Alien vs. Predator (Euro 940520)" )
GAME( 1994, avspu,    avsp,    cps2, avsp,    cps2, ROT0,   "Capcom", "Alien vs. Predator (USA 940520)" )
GAME( 1994, avspj,    avsp,    cps2, avsp,    cps2, ROT0,   "Capcom", "Alien vs. Predator (Japan 940520)" )
GAMEX(1994, avspa,    avsp,    cps2, avsp,    cps2, ROT0,   "Capcom", "Alien vs. Predator (Asia 940520)", GAME_NOT_WORKING )
GAME( 1994, dstlk,    0,       cps2, ssf2,    cps2, ROT0,   "Capcom", "Darkstalkers: The Night Warriors (Euro 940705)" )
GAME( 1994, dstlku,   dstlk,   cps2, ssf2,    cps2, ROT0,   "Capcom", "Darkstalkers: The Night Warriors (USA 940818)" )
GAME( 1994, dstlkur1, dstlk,   cps2, ssf2,    cps2, ROT0,   "Capcom", "Darkstalkers: The Night Warriors (USA 940705)" )
GAME( 1994, dstlka,   dstlk,   cps2, ssf2,    cps2, ROT0,   "Capcom", "Darkstalkers: The Night Warriors (Asia 940705)" )
GAME( 1994, vampj,    dstlk,   cps2, ssf2,    cps2, ROT0,   "Capcom", "Vampire: The Night Warriors (Japan 940705)" )
GAME( 1994, vampja,   dstlk,   cps2, ssf2,    cps2, ROT0,   "Capcom", "Vampire: The Night Warriors (Japan 940705 alt)" )
GAME( 1994, vampjr1,  dstlk,   cps2, ssf2,    cps2, ROT0,   "Capcom", "Vampire: The Night Warriors (Japan 940630)" )
GAME( 1994, ringdest, 0,       cps2, ringdest,cps2, ROT0,   "Capcom", "Ring of Destruction: Slammasters II (Euro 940902)" )
GAME( 1994, smbomb,   ringdest,cps2, ringdest,cps2, ROT0,   "Capcom", "Super Muscle Bomber: The International Blowout (Japan 940831)" )
GAME( 1994, smbombr1, ringdest,cps2, ringdest,cps2, ROT0,   "Capcom", "Super Muscle Bomber: The International Blowout (Japan 940808)" )
GAME( 1995, cybots,   0,       cps2, cybots,  cps2, ROT0,   "Capcom", "Cyberbots: Fullmetal Madness (USA 950424)" )
GAME( 1995, cybotsj,  cybots,  cps2, cybots,  cps2, ROT0,   "Capcom", "Cyberbots: Fullmetal Madness (Japan 950420)" )
GAMEX(1995, msh,      0,       cps2, ssf2,    cps2, ROT0,   "Capcom", "Marvel Super Heroes (Euro 951024)", GAME_NOT_WORKING )
GAME( 1995, mshu,     msh,     cps2, ssf2,    cps2, ROT0,   "Capcom", "Marvel Super Heroes (USA 951024)" )
GAME( 1995, mshj,     msh,     cps2, ssf2,    cps2, ROT0,   "Capcom", "Marvel Super Heroes (Japan 951024)" )
GAME( 1995, msha,     msh,     cps2, ssf2,    cps2, ROT0,   "Capcom", "Marvel Super Heroes (Asia 951024)" )
GAME( 1995, mshh,     msh,     cps2, ssf2,    cps2, ROT0,   "Capcom", "Marvel Super Heroes (Hispanic 951117)" )
GAMEX(1995, mshb,     msh,     cps2, ssf2,    cps2, ROT0,   "Capcom", "Marvel Super Heroes (Brazil 951117)", GAME_NOT_WORKING )
GAME( 1995, nwarr,    0,       cps2, ssf2,    cps2, ROT0,   "Capcom", "Night Warriors: Darkstalkers' Revenge (USA 950406)" )
GAMEX(1995, nwarrh,   nwarr,   cps2, ssf2,    cps2, ROT0,   "Capcom", "Night Warriors: Darkstalkers' Revenge (Hispanic 950403)", GAME_NOT_WORKING )
GAMEX(1995, nwarrb,   nwarr,   cps2, ssf2,    cps2, ROT0,   "Capcom", "Night Warriors: Darkstalkers' Revenge (Brazil 950403)", GAME_NOT_WORKING )
GAME( 1995, vhuntj,   nwarr,   cps2, ssf2,    cps2, ROT0,   "Capcom", "Vampire Hunter: Darkstalkers' Revenge (Japan 950316)" )
GAME( 1995, vhuntjr1, nwarr,   cps2, ssf2,    cps2, ROT0,   "Capcom", "Vampire Hunter: Darkstalkers' Revenge (Japan 950302)" )
GAME( 1995, sfa,      0,       cps2, ssf2,    cps2, ROT0,   "Capcom", "Street Fighter Alpha: Warriors' Dreams (Euro 950727)" )
GAME( 1995, sfar1,    sfa,     cps2, ssf2,    cps2, ROT0,   "Capcom", "Street Fighter Alpha: Warriors' Dreams (Euro 950718)" )
GAME( 1995, sfar2,    sfa,     cps2, ssf2,    cps2, ROT0,   "Capcom", "Street Fighter Alpha: Warriors' Dreams (Euro 950605)" )
GAME( 1995, sfau,     sfa,     cps2, ssf2,    cps2, ROT0,   "Capcom", "Street Fighter Alpha: Warriors' Dreams (USA 950627)" )
GAME( 1995, sfzj,     sfa,     cps2, ssf2,    cps2, ROT0,   "Capcom", "Street Fighter Zero (Japan 950727)" )
GAME( 1995, sfzjr1,   sfa,     cps2, ssf2,    cps2, ROT0,   "Capcom", "Street Fighter Zero (Japan 950627)" )
GAME( 1995, sfzjr2,   sfa,     cps2, ssf2,    cps2, ROT0,   "Capcom", "Street Fighter Zero (Japan 950605)" )
GAMEX(1995, sfzh,     sfa,     cps2, ssf2,    cps2, ROT0,   "Capcom", "Street Fighter Zero (Hispanic 950627)", GAME_NOT_WORKING )
GAMEX(1995, sfzb,     sfa,     cps2, ssf2,    cps2, ROT0,   "Capcom", "Street Fighter Zero (Brazil 951109)", GAME_NOT_WORKING )
GAME( 1996, 19xx,       0,     cps2, 19xx, my_cps2, ROT270, "Capcom", "19XX: The War Against Destiny (Euro 960104)" )
GAME( 1996, 19xxu,   19xx,     cps2, 19xx, my_cps2, ROT270, "Capcom", "19XX: The War Against Destiny (USA 951207)" )
GAME( 1996, 19xxa,   19xx,     cps2, 19xx, my_cps2, ROT270, "Capcom", "19XX: The War Against Destiny (Asia 960104)" )
GAME( 1996, 19xxar1, 19xx,     cps2, 19xx, my_cps2, ROT270, "Capcom", "19XX: The War Against Destiny (Asia 951207)" )
GAME( 1996, 19xxj,   19xx,     cps2, 19xx, my_cps2, ROT270, "Capcom", "19XX: The War Against Destiny (Japan 960104, yellow case)" )
GAME( 1996, 19xxjr1, 19xx,     cps2, 19xx, my_cps2, ROT270, "Capcom", "19XX: The War Against Destiny (Japan 951225)" )
GAME( 1996, 19xxjr2, 19xx,     cps2, 19xx, my_cps2, ROT270, "Capcom", "19XX: The War Against Destiny (Japan 951207)" )
GAME( 1996, 19xxh,   19xx,     cps2, 19xx, my_cps2, ROT270, "Capcom", "19XX: The War Against Destiny (Hispanic 951218)" )
GAME( 1996, 19xxb,   19xx,     cps2, 19xx, my_cps2, ROT270, "Capcom", "19XX: The War Against Destiny (Brazil 951218)" )
GAMEC(1996, ddsom,    0,       cps2, ddtod,   cps2, ROT0,   "Capcom", "Dungeons and Dragons: Shadow over Mystara (Euro 960619)", &ddsom_ctrl, &ddsom_bootstrap )
GAMEC(1996, ddsomr1,  ddsom,   cps2, ddtod,   cps2, ROT0,   "Capcom", "Dungeons and Dragons: Shadow over Mystara (Euro 960209)", &ddsom_ctrl, &ddsom_bootstrap )
GAMEC(1996, ddsomu,   ddsom,   cps2, ddtod,   cps2, ROT0,   "Capcom", "Dungeons and Dragons: Shadow over Mystara (USA 960619)", &ddsom_ctrl, &ddsomu_bootstrap )
GAMEC(1996, ddsomur1, ddsom,   cps2, ddtod,   cps2, ROT0,   "Capcom", "Dungeons and Dragons: Shadow over Mystara (USA 960209)", &ddsom_ctrl, &ddsomu_bootstrap )
GAMEC(1996, ddsomj,   ddsom,   cps2, ddtod,   cps2, ROT0,   "Capcom", "Dungeons and Dragons: Shadow over Mystara (Japan 960619)", &ddsom_ctrl, &ddsomj_bootstrap )
GAMEC(1996, ddsomjr1, ddsom,   cps2, ddtod,   cps2, ROT0,   "Capcom", "Dungeons and Dragons: Shadow over Mystara (Japan 960206)", &ddsom_ctrl, &ddsomj_bootstrap )
GAMEC(1996, ddsoma,   ddsom,   cps2, ddtod,   cps2, ROT0,   "Capcom", "Dungeons and Dragons: Shadow over Mystara (Asia 960619)", &ddsom_ctrl, &ddsoma_bootstrap )
GAME( 1996, megaman2, 0,       cps2, sgemf,   cps2, ROT0,   "Capcom", "Mega Man 2: The Power Fighters (USA 960708)" )
GAMEX(1996, megamn2a, megaman2,cps2, sgemf,   cps2, ROT0,   "Capcom", "Mega Man 2: The Power Fighters (Asia 960708)", GAME_NOT_WORKING )
GAME( 1996, rckman2j, megaman2,cps2, sgemf,   cps2, ROT0,   "Capcom", "Rockman 2: The Power Fighters (Japan 960708)" )
GAME( 1996, qndream,  0,       cps2, qndream, cps2, ROT0,   "Capcom", "Quiz Nanairo Dreams: Nijiirochou no Kiseki (Japan 960826)" )
GAME( 1996, sfa2,     0,       cps2, ssf2,    cps2, ROT0,   "Capcom", "Street Fighter Alpha 2 (USA 960306)" )
GAME( 1996, sfz2j,    sfa2,    cps2, ssf2,    cps2, ROT0,   "Capcom", "Street Fighter Zero 2 (Japan 960227)" )
GAME( 1996, sfz2a,    sfa2,    cps2, ssf2,    cps2, ROT0,   "Capcom", "Street Fighter Zero 2 (Asia 960227)" )
GAMEX(1996, sfz2b,    sfa2,    cps2, ssf2,    cps2, ROT0,   "Capcom", "Street Fighter Zero 2 (Brazil 960531)", GAME_NOT_WORKING )
GAMEX(1996, sfz2br1,  sfa2,    cps2, ssf2,    cps2, ROT0,   "Capcom", "Street Fighter Zero 2 (Brazil 960304)", GAME_NOT_WORKING )
GAME( 1996, sfz2aj,   sfa2,    cps2, ssf2,    cps2, ROT0,   "Capcom", "Street Fighter Zero 2 Alpha (Japan 960805)" )
GAMEX(1996, sfz2ah,   sfa2,    cps2, ssf2,    cps2, ROT0,   "Capcom", "Street Fighter Zero 2 Alpha (Hispanic 960813)", GAME_NOT_WORKING )
GAMEX(1996, sfz2ab,   sfa2,    cps2, ssf2,    cps2, ROT0,   "Capcom", "Street Fighter Zero 2 Alpha (Brazil 960813)", GAME_NOT_WORKING )
GAME( 1996, sfz2aa,   sfa2,    cps2, ssf2,    cps2, ROT0,   "Capcom", "Street Fighter Zero 2 Alpha (Asia 960826)" )
GAME( 1996, spf2t,    0,       cps2, 19xx,    cps2, ROT0,   "Capcom", "Super Puzzle Fighter 2 Turbo (USA 960620)" )
GAME( 1996, spf2xj,   spf2t,   cps2, 19xx,    cps2, ROT0,   "Capcom", "Super Puzzle Fighter 2 X (Japan 960531)" )
GAME( 1996, xmvsf,    0,       cps2, ssf2,    cps2, ROT0,   "Capcom", "X-Men Vs. Street Fighter (Euro 960910)" )
GAMEX(1996, xmvsfu,   xmvsf,   cps2, ssf2,    cps2, ROT0,   "Capcom", "X-Men Vs. Street Fighter (USA 961023)", GAME_NOT_WORKING )
GAME( 1996, xmvsfur1, xmvsf,   cps2, ssf2,    cps2, ROT0,   "Capcom", "X-Men Vs. Street Fighter (USA 961004)" )
GAMEX(1996, xmvsfj,   xmvsf,   cps2, ssf2,    cps2, ROT0,   "Capcom", "X-Men Vs. Street Fighter (Japan 961004)", GAME_NOT_WORKING )
GAMEX(1996, xmvsfjr1, xmvsf,   cps2, ssf2,    cps2, ROT0,   "Capcom", "X-Men Vs. Street Fighter (Japan 960910)", GAME_NOT_WORKING )
GAMEX(1996, xmvsfjr2, xmvsf,   cps2, ssf2,    cps2, ROT0,   "Capcom", "X-Men Vs. Street Fighter (Japan 960909)", GAME_NOT_WORKING )
GAMEX(1996, xmvsfa,   xmvsf,   cps2, ssf2,    cps2, ROT0,   "Capcom", "X-Men Vs. Street Fighter (Asia 961023)", GAME_NOT_WORKING )
GAMEX(1996, xmvsfh,   xmvsf,   cps2, ssf2,    cps2, ROT0,   "Capcom", "X-Men Vs. Street Fighter (Hispanic 961004)", GAME_NOT_WORKING )
GAMEX(1996, xmvsfb,   xmvsf,   cps2, ssf2,    cps2, ROT0,   "Capcom", "X-Men Vs. Street Fighter (Brazil 961023)", GAME_NOT_WORKING )
GAME( 1997, batcir,   0,       cps2, batcir,  cps2, ROT0,   "Capcom", "Battle Circuit (Euro 970319)" )
GAMEX(1997, batcira,  batcir,  cps2, batcir,  cps2, ROT0,   "Capcom", "Battle Circuit (Asia 970319)", GAME_NOT_WORKING )
GAME( 1997, batcirj,  batcir,  cps2, batcir,  cps2, ROT0,   "Capcom", "Battle Circuit (Japan 970319)" )
GAME( 1997, csclub,   0,       cps2, sgemf,   cps2, ROT0,   "Capcom", "Capcom Sports Club (Euro 970722)" )
GAME( 1997, cscluba,  csclub,  cps2, sgemf,   cps2, ROT0,   "Capcom", "Capcom Sports Club (Asia 970722)" )
GAME( 1997, csclubj,  csclub,  cps2, sgemf,   cps2, ROT0,   "Capcom", "Capcom Sports Club (Japan 970722)" )
GAMEX(1997, csclubh,  csclub,  cps2, sgemf,   cps2, ROT0,   "Capcom", "Capcom Sports Club (Hispanic 970722)", GAME_NOT_WORKING )
GAME( 1997, mshvsf,   0,       cps2, ssf2,    cps2, ROT0,   "Capcom", "Marvel Super Heroes Vs. Street Fighter (USA 970827)" )
GAME( 1997, mshvsfu1, mshvsf,  cps2, ssf2,    cps2, ROT0,   "Capcom", "Marvel Super Heroes Vs. Street Fighter (USA 970625)" )
GAME( 1997, mshvsfj,  mshvsf,  cps2, ssf2,    cps2, ROT0,   "Capcom", "Marvel Super Heroes Vs. Street Fighter (Japan 970707)" )
GAME( 1997, mshvsfj1, mshvsf,  cps2, ssf2,    cps2, ROT0,   "Capcom", "Marvel Super Heroes Vs. Street Fighter (Japan 970702)" )
GAMEX(1997, mshvsfj2, mshvsf,  cps2, ssf2,    cps2, ROT0,   "Capcom", "Marvel Super Heroes Vs. Street Fighter (Japan 970625)", GAME_NOT_WORKING )
GAMEX(1997, mshvsfh,  mshvsf,  cps2, ssf2,    cps2, ROT0,   "Capcom", "Marvel Super Heroes Vs. Street Fighter (Hispanic 970625)", GAME_NOT_WORKING )
GAMEX(1997, mshvsfa,  mshvsf,  cps2, ssf2,    cps2, ROT0,   "Capcom", "Marvel Super Heroes Vs. Street Fighter (Asia 970625)", GAME_NOT_WORKING )
GAMEX(1997, mshvsfa1, mshvsf,  cps2, ssf2,    cps2, ROT0,   "Capcom", "Marvel Super Heroes Vs. Street Fighter (Asia 970620)", GAME_NOT_WORKING )
GAMEX(1997, mshvsfb,  mshvsf,  cps2, ssf2,    cps2, ROT0,   "Capcom", "Marvel Super Heroes Vs. Street Fighter (Brazil 970827)", GAME_NOT_WORKING )
GAMEX(1997, mshvsfb1, mshvsf,  cps2, ssf2,    cps2, ROT0,   "Capcom", "Marvel Super Heroes Vs. Street Fighter (Brazil 970625)", GAME_NOT_WORKING )
GAME( 1997, sgemf,    0,       cps2, sgemf,   cps2, ROT0,   "Capcom", "Super Gem Fighter Mini Mix (USA 970904)" )
GAME( 1997, pfghtj,   sgemf,   cps2, sgemf,   cps2, ROT0,   "Capcom", "Pocket Fighter (Japan 970904)" )
GAME( 1997, sgemfa,   sgemf,   cps2, sgemf,   cps2, ROT0,   "Capcom", "Super Gem Fighter: Mini Mix (Asia 970904)" )
GAMEX(1997, sgemfh,   sgemf,   cps2, sgemf,   cps2, ROT0,   "Capcom", "Super Gem Fighter: Mini Mix (Hispanic 970904)", GAME_NOT_WORKING )
GAMEX(1997, vhunt2,   0,       cps2, ssf2,    cps2, ROT0,   "Capcom", "Vampire Hunter 2: Darkstalkers Revenge (Japan 970929)", GAME_NOT_WORKING )
GAME( 1997, vhunt2r1, vhunt2,  cps2, ssf2,    cps2, ROT0,   "Capcom", "Vampire Hunter 2: Darkstalkers Revenge (Japan 970913)" )
GAME( 1997, vsav,     0,       cps2, ssf2,    cps2, ROT0,   "Capcom", "Vampire Savior: The Lord of Vampire (Euro 970519)" )
GAME( 1997, vsavu,    vsav,    cps2, ssf2,    cps2, ROT0,   "Capcom", "Vampire Savior: The Lord of Vampire (USA 970519)" )
GAME( 1997, vsavj,    vsav,    cps2, ssf2,    cps2, ROT0,   "Capcom", "Vampire Savior: The Lord of Vampire (Japan 970519)" )
GAME( 1997, vsava,    vsav,    cps2, ssf2,    cps2, ROT0,   "Capcom", "Vampire Savior: The Lord of Vampire (Asia 970519)" )
GAMEX(1997, vsavh,    vsav,    cps2, ssf2,    cps2, ROT0,   "Capcom", "Vampire Savior: The Lord of Vampire (Hispanic 970519)", GAME_NOT_WORKING )
GAME( 1997, vsav2,    0,       cps2, ssf2,    cps2, ROT0,   "Capcom", "Vampire Savior 2: The Lord of Vampire (Japan 970913)" )
GAME( 1998, mvsc,     0,       cps2, ssf2,    cps2, ROT0,   "Capcom", "Marvel Vs. Capcom: Clash of Super Heroes (USA 980123)" )
GAME( 1998, mvscj,    mvsc,    cps2, ssf2,    cps2, ROT0,   "Capcom", "Marvel Vs. Capcom: Clash of Super Heroes (Japan 980123)" )
GAME( 1998, mvscjr1,  mvsc,    cps2, ssf2,    cps2, ROT0,   "Capcom", "Marvel Vs. Capcom: Clash of Super Heroes (Japan 980112)" )
GAME( 1998, mvsca,    mvsc,    cps2, ssf2,    cps2, ROT0,   "Capcom", "Marvel Vs. Capcom: Clash of Super Heroes (Asia 980112)" )
GAMEX(1998, mvsch,    mvsc,    cps2, ssf2,    cps2, ROT0,   "Capcom", "Marvel Vs. Capcom: Clash of Super Heroes (Hispanic 980123)", GAME_NOT_WORKING )
GAME( 1998, sfa3,     0,       cps2, ssf2,    cps2, ROT0,   "Capcom", "Street Fighter Alpha 3 (USA 980904)" )
GAME( 1998, sfa3r1,   sfa3,    cps2, ssf2,    cps2, ROT0,   "Capcom", "Street Fighter Alpha 3 (USA 980629)" )
GAME( 1998, sfz3j,    sfa3,    cps2, ssf2,    cps2, ROT0,   "Capcom", "Street Fighter Zero 3 (Japan 980727)" )
GAME( 1998, sfz3jr1,  sfa3,    cps2, ssf2,    cps2, ROT0,   "Capcom", "Street Fighter Zero 3 (Japan 980629)" )
GAME( 1998, sfz3a,    sfa3,    cps2, ssf2,    cps2, ROT0,   "Capcom", "Street Fighter Zero 3 (Asia 980701)" )
GAMEC(2004, hsf2,     0,       cps2, ssf2, my_cps2, ROT0,   "Capcom", "Hyper Street Fighter II: The Anniversary Edition (USA 040202)", &sf2_ctrl, NULL )
GAMEC(2004, hsf2a,    hsf2,    cps2, ssf2, my_cps2, ROT0,   "Capcom", "Hyper Street Fighter II: The Anniversary Edition (Asia 040202)", &sf2_ctrl, NULL )
GAMEC(2004, hsf2j,    hsf2,    cps2, ssf2, my_cps2, ROT0,   "Capcom", "Hyper Street Fighter II: The Anniversary Edition (Japan 040202)", &sf2_ctrl, NULL )
GAMEC(2004, hsf2j1,   hsf2,    cps2, ssf2, my_cps2, ROT0,   "Capcom", "Hyper Street Fighter II: The Anniversary Edition (Japan 031222)", &sf2_ctrl, NULL )


/* Games released on CPS-2 hardware by Takumi */

GAME( 1999, gigawing, 0,       cps2, 19xx,    cps2, ROT0,   "Capcom, supported by Takumi", "Giga Wing (USA 990222)" )
GAME( 1999, gwingj,   gigawing,cps2, 19xx,    cps2, ROT0,   "Capcom, supported by Takumi", "Giga Wing (Japan 990223)" )
GAME( 2000, mmatrix,  0,       cps2, 19xx,    cps2, ROT0,   "Capcom, supported by Takumi", "Mars Matrix: Hyper Solid Shooting (USA 000412)" )
GAME( 2000, mmatrixj, mmatrix, cps2, 19xx,    cps2, ROT0,   "Capcom, supported by Takumi", "Mars Matrix: Hyper Solid Shooting (Japan 000412)" )

/* Games released on CPS-2 hardware by Mitchell */

GAME( 2000, mpang,     0,       cps2, cps2,    my_cps2,     ROT0,   "Mitchell (Capcom license)", "Mighty! Pang (Euro 001010)"        )
GAME( 2000, mpangr1,   mpang,   cps2, ssf2,    my_cps2,     ROT0,   "Mitchell (Capcom license)", "Mighty! Pang (Euro 000925)"        )
GAME( 2000, mpangu,    mpang,   cps2, ssf2,    my_cps2,     ROT0,   "Mitchell (Capcom license)", "Mighty! Pang (USA 001010)"         )
GAME( 2000, mpangj,    mpang,   cps2, ssf2,    my_cps2,     ROT0,   "Mitchell (Capcom license)", "Mighty! Pang (Japan 001011)"       )
GAME( 2000, mpanga,    mpang,   cps2, ssf2,    my_cps2,     ROT0,   "Mitchell (Capcom license)", "Mighty! Pang (Asia 001010)"        )
GAME( 2001, pzloop2,   0,       cps2, cps2,    my_cps2,     ROT0,   "Mitchell (Capcom license)", "Puzz Loop 2 (Euro 010302)"         )
GAME( 2001, pzloop2j,  pzloop2, cps2, cps2,    my_cps2,     ROT0,   "Mitchell (Capcom license)", "Puzz Loop 2 (Japan 010226)"        )
GAME( 2001, pzloop2jr1,pzloop2, cps2, cps2,    my_cps2,     ROT0,   "Mitchell (Capcom license)", "Puzz Loop 2 (Japan 010205)"        )

/* Games released on CPS-2 hardware by Eighting/Raizing */


GAME( 2000, dimahoo,  0,       cps2, sgemf,   cps2, ROT270, "Eighting/Raizing, distributed by Capcom", "Dimahoo (USA 000121)" )
GAME( 2000, gmahou,   dimahoo, cps2, sgemf,   cps2, ROT270, "Eighting/Raizing, distributed by Capcom", "Great Mahou Daisakusen (Japan 000121)" )
GAME( 2000, 1944,     0,       cps2, 19xx,    cps2, ROT0,   "Capcom, supported by Eighting/Raizing", "1944: The Loop Master (USA 000620)" )
GAMEX(2000, 1944j,    1944,    cps2, 19xx,    cps2, ROT0,   "Capcom, supported by Eighting/Raizing", "1944: The Loop Master (Japan 000620)", GAME_NOT_WORKING )
	
/* Games released on CPS-2 hardware by Cave */

GAME( 2001, progear,  0,       cps2, sgemf,   cps2, ROT0,   "Cave, distributed by Capcom", "Progear (USA 010117)" )
GAME( 2001, progearj, progear, cps2, sgemf,   cps2, ROT0,   "Cave, distributed by Capcom", "Progear No Arashi (Japan 010117)" )
