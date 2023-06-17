/* Tecmo System
 Driver by Farfetch, David Haywood & Tomasz Slanina
  Protection simulation by nuapete

 ToDo:
  Is tilemap scroll / sprite scroll 100% correct - seems to be protection releated
  Dump / Decap MCUs to allow for proper protection emulation.
  Fix Sound (are the sound roms good?)


T.Slanina 20040530 :
 - preliminary gfx decode,
 - Angel Eyes - patched interrupt level1 vector
 - EEPROM r/w
 - txt layer
 - added hacks to see more gfx (press Z or X)
 - palette (press X in angel eyes to see 'color bar chack'(!))
 - watchdog (?) simulation

 20080528
 - Removed ROM patches and debug keypresses
 - Added protection simulation in machine/tecmosys.c
 - Fixed inputs
 - Added watchdog

   To enter test mode, you have to press the test switch before you insert any coins.

*/


/*

Deroon Dero Dero
Tecmo, 1996

This game is a Puyo Puyo rip-off.

PCB Layout
----------

TECMO SYSTEM BOARD A
|-------------------------------------------------------------------------|
|  LM324  UPC452C      16.9MHz          |--------|    |--------|    6264  |
| TA8205 LM324  YAC513 YMF262 YMZ280B   |TECMO   |    |TECMO   |    6264  |
|        LM324  M6295  UPC452C          |AA03-8431    |AA02-1927          |
|                      YAC512           |        |    |        |          |
|                                       |--------|    |--------|          |
|        Z80  6264 28MHz 14.31818MHz                  |--------|          |
|                    16MHz             62256          |TECMO   |          |
|            TA8030                    62256          |AA02-1927    6264  |
|                                                     |        |    6264  |
|J  93C46                               |--------|    |--------|          |
|A                                      |TECMO   |    |--------|          |
|M                                      |AA03-8431    |TECMO   |          |
|M          68000                       |        |    |AA02-1927          |
|A                                      |--------|    |        |    6264  |
|                  PAL              6116              |--------|    6264  |
|                                    6116             |--------|          |
|  |--------|                                         |TECMO   |          |
|  |TECMO   |                     PAL                 |AA02-1927          |
|  |AA03-8431  62256                                  |        |    6264  |
|  |        |  62256                                  |--------|    6264  |
|  |--------|                                  |---------|                |
|                                              |TECMO    |                |
|                                              |AA03-8431|                |
|                                              |         |                |
|                                              |---------|          424260|
|                                              62256 62256          424260|
|-------------------------------------------------------------------------|
Notes:
68000 @ 16MHz
Z80 @ 8MHz [16/2]
YMZ280B @ 16.9MHz
YMF262 @ 14.31818MHz
OKI M6295 @ 2MHz [16/8]. Pin 7 HIGH

Game Board
----------

TECMO SYSTEM BOARD B2
|-------------------------------------------------------------------------|
|    T201_DIP42_MASK.UBB1                                                 |
| |----|                                              T202_DIP42_MASK.UBC1|
| |*   |                                                                  |
| |----|                                                                  |
|                                                                         |
|  T003_2M_EPROM.UZ1                        T101_SOP44.UAH1               |
|                                                                         |
|                                            T301_DIP42_MASK.UBD1         |
|                                                                         |
|                                                                         |
|                                                                         |
|  T401_DIP42_MASK.UYA1      T104_SOP44.UCL1    T001_4M_EPROM.UPAU1       |
|                              T103_SOP44.UBL1                            |
|  T501_DIP32_MASK.UAD1      T102_SOP44.UAL1                              |
|                                                  T002_4M_EPROM.UPAL1    |
|-------------------------------------------------------------------------|
Notes:
      * - QFP64 microcontroller marked 'TECMO SC432146FU E23D 185 SSAB9540B'
          this is a 68HC11A8 with 8k ROM, 512 bytes EEPROM and 256 bytes on-chip RAM.
          Clocks: pin 33 - 8MHz, pin 31: 8MHz, pin 29 - 2MHz
          GND on pins 49, 23, 24, 27
          Power on pins 55, 25
          Note - Pins 25 and 27 are tied to some jumpers, so these
          appear to be some kind of configuration setting.

CPU  : TMP68HC000P-16
Sound: TMPZ84C00AP-8 YMF262 YMZ280B M6295
OSC  : 14.3181MHz (X1) 28.0000MHz (X2) 16.0000MHz (X3) 16.9MHz (X4)

Custom chips:
TECMO AA02-1927 (160pin PQFP) (x4)
TECMO AA03-8431 (208pin PQFP) (x4)

Others:
93C46 EEPROM (settings are stored to this)

ROMs:

name            type
t001.upau1      27c040 dip32 eprom
t002.upal1      27c040 dip32 eprom
t003.uz1        27c2001 dip32 eprom

t101.uah1       23c16000 sop44 maskrom
t102.ual1       23c16000 sop44 maskrom
t103.ubl1       23c32000 sop44 maskrom
t104.ucl1       23c16000 sop44 maskrom
t201.ubb1       23c8000 dip42 maskrom
t202.ubc1       23c8000 dip42 maskrom
t301.ubd1       23c8000 dip42 maskrom
t401.uya1       23c16000 dip42 maskrom
t501.uad1       23c4001 dip32 maskrom

*/

/*

Toukidenshou - Angel Eyes
(c)1996 Tecmo
Tecmo System Board

CPU  : TMP68HC000P-16
Sound: TMPZ84C00AP-8 YMF262 YMZ280B M6295
OSC  : 14.3181MHz (X1) 28.0000MHz (X2) 16.0000MHz (X3) 16.9MHz (X4)

Custom chips:
TECMO AA02-1927 (160pin PQFP) (x4)
TECMO AA03-8431 (208pin PQFP) (x4)

Others:
93C46 EEPROM (settings are stored to this)

EPROMs:
aeprge-2.pal - Main program (even) (27c4001)
aeprgo-2.pau - Main program (odd)  (27c4001)

aesprg-2.z1 - Sound program (27c1001)

Mask ROMs:
ae100h.ah1 - Graphics (23c32000/16000 SOP)
ae100.al1  |
ae101h.bh1 |
ae101.bl1  |
ae102h.ch1 |
ae102.cl1  |
ae104.el1  |
ae105.fl1  |
ae106.gl1  /

ae200w74.ba1 - Graphics (23c16000)
ae201w75.bb1 |
ae202w76.bc1 /

ae300w36.bd1 - Graphics (23c4000)

ae400t23.ya1 - YMZ280B Samples (23c16000)
ae401t24.yb1 /

ae500w07.ad1 - M6295 Samples (23c4001)

*/

#include "driver.h"
#include "machine/eeprom.h"
#include "cpu/m68000/m68k.h"
#include "vidhrdw/generic.h"

static UINT16* tecmosys_spriteram;
static UINT16* tilemap_paletteram16;
static UINT16* bg2tilemap_ram;
static UINT16* bg1tilemap_ram;
static UINT16* bg0tilemap_ram;
static UINT16* fgtilemap_ram;
static UINT16* bg0tilemap_lineram;
static UINT16* bg1tilemap_lineram;
static UINT16* bg2tilemap_lineram;

static UINT16* tecmosys_a80000regs;
static UINT16* tecmosys_b00000regs;

static UINT16* tecmosys_c00000regs;
static UINT16* tecmosys_c80000regs;
static UINT16* tecmosys_880000regs;
static int tecmosys_spritelist;

static struct mame_bitmap *sprite_bitmap;
static struct mame_bitmap *tmp_tilemap_composebitmap;
static struct mame_bitmap *tmp_tilemap_renderbitmap;

static MACHINE_INIT( deroon );

static struct tilemap *bg0tilemap;

static void get_bg0tile_info(int tile_index)
{

	SET_TILE_INFO(
			1,
			bg0tilemap_ram[2*tile_index+1],
			(bg0tilemap_ram[2*tile_index]&0x3f),
			TILE_FLIPYX((bg0tilemap_ram[2*tile_index]&0xc0)>>6));
}

static WRITE16_HANDLER( bg0_tilemap_w )
{
	COMBINE_DATA(&bg0tilemap_ram[offset]);
	tilemap_mark_tile_dirty(bg0tilemap,offset/2);
}

static struct tilemap *bg1tilemap;

static void get_bg1tile_info(int tile_index)
{

	SET_TILE_INFO(
			2,
			bg1tilemap_ram[2*tile_index+1],
			(bg1tilemap_ram[2*tile_index]&0x3f),
			TILE_FLIPYX((bg1tilemap_ram[2*tile_index]&0xc0)>>6));
}

static WRITE16_HANDLER( bg1_tilemap_w )
{
	COMBINE_DATA(&bg1tilemap_ram[offset]);
	tilemap_mark_tile_dirty(bg1tilemap,offset/2);
}

static struct tilemap *bg2tilemap;

static void get_bg2tile_info(int tile_index)
{

	SET_TILE_INFO(
			3,
			bg2tilemap_ram[2*tile_index+1],
			(bg2tilemap_ram[2*tile_index]&0x3f),
			TILE_FLIPYX((bg2tilemap_ram[2*tile_index]&0xc0)>>6));
}

static WRITE16_HANDLER( bg2_tilemap_w )
{
	COMBINE_DATA(&bg2tilemap_ram[offset]);
	tilemap_mark_tile_dirty(bg2tilemap,offset/2);
}

static struct tilemap *txt_tilemap;

static void get_tile_info(int tile_index)
{

	SET_TILE_INFO(
			0,
			fgtilemap_ram[2*tile_index+1],
			(fgtilemap_ram[2*tile_index]&0x3f),
			TILE_FLIPYX((fgtilemap_ram[2*tile_index]&0xc0)>>6));
}

static WRITE16_HANDLER( fg_tilemap_w )
{
	COMBINE_DATA(&fgtilemap_ram[offset]);
	tilemap_mark_tile_dirty(txt_tilemap,offset/2);
}


/* It looks like this needs a synch between z80 and 68k ??? See z80:006A-0091 */
static READ16_HANDLER( sound_r )
{
	if (ACCESSING_LSB)
	{
		timer_set(TIME_NOW, 0, NULL);
		return soundlatch2_r(0);
	}

	return 0;
}

static WRITE16_HANDLER( sound_w )
{
	if (ACCESSING_LSB)
	{
		timer_set(TIME_NOW, 0, NULL);
		soundlatch_w(0x00,data & 0xff);
		cpu_set_nmi_line(1, PULSE_LINE);
	}
}

/***************************************************************************

    tecmosys protection simulation

***************************************************************************/

enum DEV_STATUS
{
	DS_IDLE,
	DS_LOGIN,
	DS_SEND_CODE,
	DS_SEND_ADRS,
	DS_SEND_CHKSUMS,
	DS_DONE
};

struct prot_data
{
	UINT8 passwd_len;
	UINT8* passwd;
	UINT8* code;
	UINT8 checksum_ranges[17];
	UINT8 checksums[4];
};

/*----------- defined in machine/tecmosys.c -----------*/

READ16_HANDLER(prot_status_r);
WRITE16_HANDLER(prot_status_w);
READ16_HANDLER(prot_data_r);
WRITE16_HANDLER(prot_data_w);

UINT8 device_read_ptr;
UINT8 device_status;
struct prot_data* device_data;
struct prot_data deroon_data;
struct prot_data tkdensho_data;
struct prot_data tkdensha_data;

/* tecmosys protection related functions */

/*
	The device validates a password,
	then uploads the size of a code upload followed by the code upload itself.
	After that, it uploads 4 ranges of code to checksum, followed by the 4 checksums.
	The 68K does the checksumming, and returns the results to the protection device.

	Apart from inital protection calls and code upload, the vblank in both games writes
	info to the protection but they seem to ignore the returned data.
	Maybe the protection is tied to something else, or maybe it was preliminary work on
	further security.
	This is what happens in the vblank:
	- prot_w( 0xff )
	- val = prot_r()
	- prot_w( checksum1[val] )
	(The area following checksum1 is the code upload in deroon, and active RAM in tkdensho,
	so the value sent may be meaningless.)

	There is provision for calling the protection read/write functions from two of the trap 0xf switch
	statements in the 68K, but I don't see it being used anywhere.

	It looks like the code upload is very plain, it can only be 0xff bytes long, and not contain the byte 0xff.
	The checksum ranges can't contain 0xff either, although the checksum values can.
	I'd be very interested in putting some trojan ROMs together if anyone has a board to run them on.
	It might be possible to use one set of ROMs to get the checksum ranges,
	and another set with dump code places outside those ranges.
	You can get me at nuapete@hotmail.com
*/

UINT8 device_read_ptr;
UINT8 device_status;
struct prot_data* device_data;
static UINT8 device_value = 0xff;

/* deroon prot data */
static UINT8 deroon_passwd[] = {'L','U','N','A',0};
static UINT8 deroon_upload[] = {0x02, 0x4e, 0x75, 0x00 }; /* code length, code, 0x00 trailer */
struct prot_data deroon_data =
{
	5,
	deroon_passwd,
	deroon_upload,
	{
		0x10,0x11,0x12,0x13,	/* range 1 using static ranges from the ROM to avoid calculating sums. */
		0x24,0x25,0x26,0x27,	/* range 2 */
		0x38,0x39,0x3a,0x3b,	/* range 3 */
		0x4c,0x4d,0x4e,0x4f,	/* range 4 */
		0x00,					/* trailer */
	},
	{ 0xa6, 0x29, 0x4b, 0x3f }
};

/* tkdensho prot data */
static UINT8 tkdensho_passwd[] = {'A','G','E','P','R','O','T','E','C','T',' ','S','T','A','R','T',0};
static UINT8 tkdensho_upload[] = {0x06, 0x4e, 0xf9, 0x00, 0x00, 0x22, 0xc4,0x00};
struct prot_data tkdensho_data =
{
	0x11,
	tkdensho_passwd,
	tkdensho_upload,
	{
		0x10,0x11,0x12,0x13,	/* range 1 */
		0x24,0x25,0x26,0x27,	/* range 2 */
		0x38,0x39,0x3a,0x3b,	/* range 3 */
		0x4c,0x4d,0x4e,0x4f,	/* range 4 */
		0x00,			/* trailer */
	},
	{ 0xbf, 0xfa, 0xda, 0xda }
};

struct prot_data tkdensha_data =
{
	0x11,
	tkdensho_passwd,
	tkdensho_upload,
	{
		0x10,0x11,0x12,0x13,	/* range 1 */
		0x24,0x25,0x26,0x27,	/* range 2 */
		0x38,0x39,0x3a,0x3b,	/* range 3 */
		0x4c,0x4d,0x4e,0x4f,	/* range 4 */
		0x00,			/* trailer */
	},
	{ 0xbf, 0xfa, 0x21, 0x5d }
};


READ16_HANDLER(prot_status_r)
{
	if (ACCESSING_MSB)
	{
		/* Bit 7: 0 = ready to write */
		/* Bit 6: 0 = ready to read */
		return 0;
	}

	return 0xc0; /* simulation is always ready */
}

WRITE16_HANDLER(prot_status_w)
{
	/* deroon clears the status in one place. */
}


READ16_HANDLER(prot_data_r)
{
	/* prot appears to be read-ready for two consecutive reads 
	 * but returns 0xff for subsequent reads. */
	UINT8 ret = device_value;
	device_value = 0xff;
	return ret << 8;
}


WRITE16_HANDLER(prot_data_w)
{
	/* Only LSB */
	data >>= 8;

	switch( device_status )
	{
		case DS_IDLE:
			if( data == 0x13 )
			{
				device_status = DS_LOGIN;
				device_value = device_data->passwd_len;
				device_read_ptr = 0;
				break;
			}
			break;

		case DS_LOGIN:
			if( device_read_ptr >= device_data->passwd_len)
			{
				device_status = DS_SEND_CODE;
				device_value = device_data->code[0];
				device_read_ptr = 1;
			}
			else
				device_value = device_data->passwd[device_read_ptr++] == data ? 0 : 0xff;
			break;

		case DS_SEND_CODE:
			if( device_read_ptr >= device_data->code[0]+2 ) /* + code_len + trailer */
			{
				device_status = DS_SEND_ADRS;
				device_value = device_data->checksum_ranges[0];
				device_read_ptr = 1;
			}
			else
				device_value = data == device_data->code[device_read_ptr-1] ? device_data->code[device_read_ptr++] : 0xff;
			break;

		case DS_SEND_ADRS:
			if( device_read_ptr >= 16+1 ) /* + trailer */
			{
				device_status = DS_SEND_CHKSUMS;
				device_value = 0;
				device_read_ptr = 0;
			}
			else
			{
				device_value = data == device_data->checksum_ranges[device_read_ptr-1] ? device_data->checksum_ranges[device_read_ptr++] : 0xff;
			}
			break;

		case DS_SEND_CHKSUMS:
			if( device_read_ptr >= 5 )
			{
				device_status = DS_DONE;
				device_value = 0;
			}
			else
				device_value = data == device_data->checksums[device_read_ptr] ? device_data->checksums[device_read_ptr++] : 0xff;
			break;

		case DS_DONE:
			switch( data )
			{
				case 0xff: /* trigger */
				case 0x00: /* checksum1[val] tkdensho */
				case 0x20: /* checksum1[val] deroon \ This is active RAM, so there may be more cases */
				case 0x01: /* checksum1[val] deroon / that can be ignored */
					break;

				default:
					log_cb(RETRO_LOG_DEBUG, LOGPRE "Protection still in use??? w=%02x\n", data );
					break;
			}
			break;
	}
}

/*
	880000 and 880002 might be video related,
	see sub @ 68k:002e5e where they are written if the screen is set to inverted.
	Also, irq code at 22c4 :
	- 880000 & 00, execute irq code
	- 880000 & 01, scroll?
	- 880000 & 03, crash
*/


static WRITE16_HANDLER( unk880000_w )
{
	COMBINE_DATA(&tecmosys_880000regs[offset]);

	switch( offset )
	{
		case 0x00/2:
			break; /* global x scroll for sprites? */
			
		case 0x02/2:
			break; /* global y scroll for sprites? */

		case 0x08/2:
			tecmosys_spritelist = data & 0x3; /* which of the 4 spritelists to use (buffering) */
			break;

		case 0x22/2:
			watchdog_400_reset_w(0,0); /* hmm not sure */
			break;

		default:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "unk880000_w( %06x, %04x ) @ %06x\n", (offset * 2)+0x880000, data, activecpu_get_pc() );
			break;
	}
}

static READ16_HANDLER( unk880000_r )
{
	tecmosys_880000regs[offset];

	log_cb(RETRO_LOG_DEBUG, LOGPRE "unk880000_r( %06x ) @ %06x = %04x\n", (offset * 2 ) +0x880000, activecpu_get_pc(), tecmosys_880000regs[offset] );


	/* this code allows scroll regs to be updated, but tkdensho at least resets perodically */

	switch( offset )
	{
		case 0:
			tecmosys_880000regs[offset];
/* hack to prevent random resets in Toukidenshou - Angel Eyes game seems to work fine now */			
            if ( cpu_getscanline() >= 240) return tecmosys_880000regs[offset] == 0;

		default:
			return 0;
	}
}



static READ16_HANDLER( eeprom_r )
{
	 return ((EEPROM_read_bit() & 0x01) << 11);
}


static MEMORY_READ16_START( readmem )
  { 0x000000, 0x0fffff, MRA16_ROM },
	{ 0x200000, 0x20ffff, MRA16_RAM }, /* work ram */
	{ 0x210000, 0x210001, MRA16_NOP }, /* single byte overflow on stack defined as 0x210000 */
	{ 0x300000, 0x3013ff, MRA16_RAM }, /* bg0 ram */
	{ 0x400000, 0x4013ff, MRA16_RAM }, /* bg1 ram */
	{ 0x500000, 0x5013ff, MRA16_RAM }, /* bg2 ram */
	{ 0x700000, 0x703fff, MRA16_RAM }, /* fix ram   (all these names from test screen) */
	{ 0x800000, 0x80ffff, MRA16_RAM }, /* obj ram */
	{ 0x880000, 0x88000b, unk880000_r },
	{ 0x900000, 0x907fff, MRA16_RAM }, /* obj pal */
	{ 0x980000, 0x9807ff, MRA16_RAM }, /* bg pal  */
	{ 0x980800, 0x980fff, MRA16_RAM }, /* fix pal */
	{ 0xb80000, 0xb80001, prot_status_r },
	{ 0xd00000, 0xd00001, input_port_0_word_r },
	{ 0xd00002, 0xd00003, input_port_1_word_r },
	{ 0xd80000, 0xd80001, eeprom_r },
	{ 0xf00000, 0xf00001, sound_r },
	{ 0xf80000, 0xf80001, prot_data_r },
MEMORY_END

static WRITE16_HANDLER( eeprom_w )
{
	if ( ACCESSING_MSB )
	{
		EEPROM_write_bit(data & 0x0800);
		EEPROM_set_cs_line((data & 0x0200) ? CLEAR_LINE : ASSERT_LINE );
		EEPROM_set_clock_line((data & 0x0400) ? CLEAR_LINE: ASSERT_LINE );
	}
}


static INLINE void set_color_555(pen_t color, int rshift, int gshift, int bshift, UINT16 data)
{
	palette_set_color(color, pal5bit(data >> rshift), pal5bit(data >> gshift), pal5bit(data >> bshift));
}


WRITE16_HANDLER( tilemap_paletteram16_xGGGGGRRRRRBBBBB_word_w )
{
	COMBINE_DATA(&tilemap_paletteram16[offset]);
	set_color_555(offset+0x4000, 5, 10, 0, tilemap_paletteram16[offset]);
}

static WRITE16_HANDLER( bg0_tilemap_lineram_w )
{
	COMBINE_DATA(&bg0tilemap_lineram[offset]);
	if (data!=0x0000) usrintf_showmessage("non 0 write to bg0 lineram %04x %04x",offset,data);
}

static WRITE16_HANDLER( bg1_tilemap_lineram_w )
{
	COMBINE_DATA(&bg1tilemap_lineram[offset]);
	if (data!=0x0000) usrintf_showmessage("non 0 write to bg1 lineram %04x %04x",offset,data);
}

static WRITE16_HANDLER( bg2_tilemap_lineram_w )
{
	COMBINE_DATA(&bg2tilemap_lineram[offset]);
	if (data!=0x0000) usrintf_showmessage("non 0 write to bg2 lineram %04x %04x",offset,data);
}


static MEMORY_WRITE16_START( writemem )
  { 0x000000, 0x0fffff, MWA16_ROM },
	{ 0x200000, 0x20ffff, MWA16_RAM }, /* work ram */
	{ 0x300000, 0x300fff, bg0_tilemap_w, &bg0tilemap_ram }, /* bg0 ram */
	{ 0x301000, 0x3013ff, bg0_tilemap_lineram_w, &bg0tilemap_lineram }, /* bg0 linescroll? (guess) */
	{ 0x400000, 0x400fff, bg1_tilemap_w, &bg1tilemap_ram }, /* bg1 ram */
	{ 0x401000, 0x4013ff, bg1_tilemap_lineram_w, &bg1tilemap_lineram }, /* bg1 linescroll? (guess) */
	{ 0x500000, 0x500fff, bg2_tilemap_w, &bg2tilemap_ram }, /* bg2 ram */
	{ 0x501000, 0x5013ff, bg2_tilemap_lineram_w, &bg2tilemap_lineram }, /* bg2 linescroll? (guess) */
	{ 0x700000, 0x703fff, fg_tilemap_w, &fgtilemap_ram },/* fix ram */
	{ 0x800000, 0x80ffff, MWA16_RAM, &tecmosys_spriteram }, /* obj ram */
	{ 0x900000, 0x907fff, paletteram16_xGGGGGRRRRRBBBBB_word_w, &paletteram16 }, /* AM_WRITE(MWA16_RAM) // obj pal */

/*
	{ 0x980000, 0x9807ff, MWA16_RAM }, /* bg pal */ 
/*	
	{ 0x980800, 0x980fff, paletteram16_xGGGGGRRRRRBBBBB_word_w, &paletteram16 }, /* fix pal */

	/* the two above are as tested by the game code, I've only rolled them into one below to get colours to show right. */
	{ 0x980000, 0x980fff, tilemap_paletteram16_xGGGGGRRRRRBBBBB_word_w, &tilemap_paletteram16 },
	{ 0x880000, 0x88002f, unk880000_w, &tecmosys_880000regs },	/* 10 byte dta@88000c, 880022=watchdog? */
	{ 0xa00000, 0xa00001, eeprom_w },
	{ 0xa80000, 0xa80005, MWA16_RAM, &tecmosys_a80000regs },	/* a80000-3 scroll? a80004 inverted ? 3 : 0 */
	{ 0xb00000, 0xb00005, MWA16_RAM, &tecmosys_b00000regs },	/* b00000-3 scrool?, b00004 inverted ? 3 : 0 */
	{ 0xb80000, 0xb80001, prot_status_w },
	{ 0xc00000, 0xc00005, MWA16_RAM, &tecmosys_c00000regs },	/* c00000-3 scroll? c00004 inverted ? 13 : 10 */
	{ 0xc80000, 0xc80005, MWA16_RAM, &tecmosys_c80000regs },	/* c80000-3 scrool? c80004 inverted ? 3 : 0 */
	{ 0xe00000, 0xe00001, sound_w },
	{ 0xe80000, 0xe80001, prot_data_w },
MEMORY_END


INPUT_PORTS_START( deroon )
	PORT_START
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP |		IPF_8WAY | IPF_PLAYER1)
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN |   IPF_8WAY | IPF_PLAYER1)
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT |   IPF_8WAY | IPF_PLAYER1)
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT |	IPF_8WAY | IPF_PLAYER1)
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 |			IPF_PLAYER1)
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 |			IPF_PLAYER1)
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 |			IPF_PLAYER1)
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START1 )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_SERVICE )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_BUTTON4 |			IPF_PLAYER1)
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP |		IPF_8WAY | IPF_PLAYER2)
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN |	IPF_8WAY | IPF_PLAYER2)
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT |	IPF_8WAY | IPF_PLAYER2)
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT |	IPF_8WAY | IPF_PLAYER2)
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 |			IPF_PLAYER2)
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 |			IPF_PLAYER2)
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 |			IPF_PLAYER2)
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START2 )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_BUTTON4 |			IPF_PLAYER2)
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

static struct GfxLayout gfxlayout =
{
   8,8,
   RGN_FRAC(1,1),
   4,
   { 0,1,2,3 },
   { 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4},
   { 0*4*8, 1*4*8, 2*4*8, 3*4*8, 4*4*8, 5*4*8, 6*4*8, 7*4*8},
   8*8*4
};

static struct GfxLayout gfxlayout2 =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4,
	  8*8*4*1+0*4, 8*8*4*1+1*4, 8*8*4*1+2*4, 8*8*4*1+3*4, 8*8*4*1+4*4, 8*8*4*1+5*4,8*8*4*1+6*4, 8*8*4*1+7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
	  8*8*4*2+0*32, 8*8*4*2+1*32, 8*8*4*2+2*32, 8*8*4*2+3*32, 8*8*4*2+4*32, 8*8*4*2+5*32, 8*8*4*2+6*32, 8*8*4*2+7*32 },
	128*8
};


static struct GfxDecodeInfo tecmosys_gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0, &gfxlayout,    0x4400, 0x40},
	{ REGION_GFX3, 0, &gfxlayout2,   0x4000, 0x40},
  { REGION_GFX4, 0, &gfxlayout2,   0x4000, 0x40},
  { REGION_GFX5, 0, &gfxlayout2,   0x4000, 0x40},
	{ -1 } /* end of array */
};

static WRITE_HANDLER( deroon_bankswitch_w )
{
	 cpu_setbank( 1, memory_region(REGION_CPU2) + ((data-2) & 0x0f) * 0x4000 + 0x10000 );
}

static MEMORY_READ_START( sound_readmem )
  { 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xe000, 0xf7ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
  { 0x0000, 0xbfff, MWA_ROM },
	{ 0xe000, 0xf7ff, MWA_RAM },
MEMORY_END

static PORT_READ_START( readport )
	{ 0x00, 0x00, YMF262_status_0_r },
	{ 0x10, 0x10, OKIM6295_status_0_r },
	{ 0x40, 0x40, soundlatch_r },
	{ 0x60, 0x60, YMZ280B_status_0_r },
PORT_END


static WRITE_HANDLER( tecmosys_oki_bank_w )
{
	UINT8 upperbank = (data & 0x30) >> 4;
	UINT8 lowerbank = (data & 0x03) >> 0;
	UINT8* region = memory_region(REGION_SOUND2);

	memcpy( region+0x00000, region+0x80000 + lowerbank * 0x20000, 0x20000  );
	memcpy( region+0x20000, region+0x80000 + upperbank * 0x20000, 0x20000  );
}

static PORT_WRITE_START( writeport )
	{ 0x00, 0x00, YMF262_register_A_0_w },
	{ 0x01, 0x01, YMF262_data_A_0_w },
	{ 0x02, 0x02, YMF262_register_B_0_w },
	{ 0x03, 0x03, YMF262_data_B_0_w },
	{ 0x10, 0x10, OKIM6295_data_0_w },
	{ 0x20, 0x20, tecmosys_oki_bank_w },
	{ 0x30, 0x30, deroon_bankswitch_w },
	{ 0x50, 0x50, soundlatch2_w },
	{ 0x60, 0x60, YMZ280B_register_0_w },
	{ 0x61, 0x61, YMZ280B_data_0_w },
PORT_END

static VIDEO_START(deroon)
{
  sprite_bitmap = auto_bitmap_alloc_depth(320,240,16);
	fillbitmap(sprite_bitmap, 0x4000, NULL);

	tmp_tilemap_composebitmap = auto_bitmap_alloc_depth(320,240,16);
	tmp_tilemap_renderbitmap = auto_bitmap_alloc_depth(320,240,16);

	fillbitmap(tmp_tilemap_composebitmap, 0x0000, NULL);
	fillbitmap(tmp_tilemap_renderbitmap, 0x0000, NULL);

	txt_tilemap = tilemap_create(get_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,32*2,32*2);
	tilemap_set_transparent_pen(txt_tilemap,0);

	bg0tilemap = tilemap_create(get_bg0tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
	tilemap_set_transparent_pen(bg0tilemap,0);

	bg1tilemap = tilemap_create(get_bg1tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
	tilemap_set_transparent_pen(bg1tilemap,0);

	bg2tilemap = tilemap_create(get_bg2tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
	tilemap_set_transparent_pen(bg2tilemap,0);

	return 0;

}

static void tecmosys_render_sprites_to_bitmap(struct mame_bitmap *bitmap, UINT16 extrax, UINT16 extray )
{
	UINT8 *gfxsrc    = memory_region       ( REGION_GFX1 );
	int i;

	/* render sprites (with priority information) to temp bitmap */
	fillbitmap(sprite_bitmap, 0x0000, NULL);
	/* there are multiple spritelists in here, to allow for buffering */
	for (i=(tecmosys_spritelist*0x4000)/2;i<((tecmosys_spritelist+1)*0x4000)/2;i+=8)
	{
		int xcnt,ycnt;
		int drawx, drawy;
		UINT16* dstptr;

		int x, y;
		int address;
		int xsize = 16;
		int ysize = 16;
		int colour;
		int flipx, flipy;
		int priority;
		int zoomx, zoomy;

		x = tecmosys_spriteram[i+0]+386;
		y = (tecmosys_spriteram[i+1]+1);

		x-= extrax;
		y-= extray;

		y&=0x1ff;
		x&=0x3ff;

		if (x&0x200) x-=0x400;
		if (y&0x100) y-=0x200;

		address =  tecmosys_spriteram[i+5]| ((tecmosys_spriteram[i+4]&0x000f)<<16);

		address<<=8;

		flipx = (tecmosys_spriteram[i+4]&0x0040)>>6;
		flipy = (tecmosys_spriteram[i+4]&0x0080)>>7; /* used by some move effects in tkdensho */



		zoomx = (tecmosys_spriteram[i+2] & 0x0fff)>>0; /* zoom? */
		zoomy = (tecmosys_spriteram[i+3] & 0x0fff)>>0; /* zoom? */

		if ((!zoomx) || (!zoomy)) continue;

		ysize =  ((tecmosys_spriteram[i+6] & 0x00ff))*16;
		xsize =  (((tecmosys_spriteram[i+6] & 0xff00)>>8))*16;

		colour =  ((tecmosys_spriteram[i+4] & 0x3f00))>>8;

		priority = ((tecmosys_spriteram[i+4] & 0x0030))>>4;


		if (tecmosys_spriteram[i+4] & 0x8000) continue;

		for (ycnt = 0; ycnt < ysize; ycnt++)
		{
			int actualycnt = (ycnt * zoomy) >> 8;
			int actualysize = (ysize * zoomy) >> 8;

			if (flipy) drawy = y + (actualysize-1) - actualycnt;
			else drawy = y + actualycnt;


			for (xcnt = 0; xcnt < xsize; xcnt++)
			{
				int actualxcnt = (xcnt * zoomx) >> 8;
				int actualxsize = (xsize *zoomx) >> 8;

				if (flipx) drawx = x + (actualxsize-1) - actualxcnt;
				else drawx = x + actualxcnt;

				if ((drawx>=0 && drawx<320) && (drawy>=0 && drawy<240))
				{
					UINT8 data;

					dstptr = &((UINT16 *)sprite_bitmap->line[drawy])[drawx]; /* correct seems like the only way which will compile.?? */
					data   =  (gfxsrc[address]);


					if(data) dstptr[0] = (data + (colour*0x100)) | (priority << 14);
				}



				address++;

			}
		}

	}
}

void tecmosys_tilemap_copy_to_compose(UINT16 pri)
{
	int y,x;
	UINT16 *srcptr;
	UINT16 *dstptr;
	for (y=0;y<240;y++)
	{
		srcptr = ((UINT16 *)tmp_tilemap_renderbitmap->line[y]);
		dstptr = ((UINT16 *)tmp_tilemap_composebitmap->line[y]);
		for (x=0;x<320;x++)
		{
			if ((srcptr[x]&0xf)!=0x0)
			    dstptr[x] =  (srcptr[x]&0x7ff) | pri;
		}
	}
}

void tecmosys_do_final_mix(struct mame_bitmap *bitmap)
{
	const pen_t *paldata = Machine->pens;
	int y,x;
	UINT16 *srcptr;
	UINT16 *srcptr2;
	UINT32 *dstptr;


	for (y=0;y<240;y++)
	{
		srcptr = ((UINT16 *)tmp_tilemap_composebitmap->line[y]);
		srcptr2 = ((UINT16 *)sprite_bitmap->line[y]);

		dstptr = ((UINT32 *)bitmap->line[y]);
		for (x=0;x<320;x++)
		{
			UINT16 pri, pri2;
			UINT16 penvalue;
			UINT16 penvalue2;
			UINT32 colour;
			UINT32 colour2;

			pri = srcptr[x] & 0xc000;
			pri2 = srcptr2[x] & 0xc000;

			penvalue = tilemap_paletteram16[srcptr[x]&0x7ff];
			colour =   paldata[(srcptr[x]&0x7ff) | 0x4000];

			if (srcptr2[x]&0x3fff)
			{
				penvalue2 = paletteram16[srcptr2[x]&0x3fff];
				colour2 = paldata[srcptr2[x]&0x3fff];
			}
			else
			{
				penvalue2 = tilemap_paletteram16[srcptr[x]&0x7ff];
				colour2 =   paldata[(srcptr[x]&0x7ff) | 0x4000];
			}

			if ((penvalue & 0x8000) && (penvalue2 & 0x8000)) /* blend */
			{
				int r,g,b;
				int r2,g2,b2;
				b = (colour & 0x000000ff) >> 0;
				g = (colour & 0x0000ff00) >> 8;
				r = (colour & 0x00ff0000) >> 16;

				b2 = (colour2 & 0x000000ff) >> 0;
				g2 = (colour2 & 0x0000ff00) >> 8;
				r2 = (colour2 & 0x00ff0000) >> 16;

				r = (r + r2) >> 1;
				g = (g + g2) >> 1;
				b = (b + b2) >> 1;

				dstptr[x] = b | (g<<8) | (r<<16);
			}
			else if (pri2 >= pri)
			{
				dstptr[x] = colour2;
			}
			else
			{
				dstptr[x] = colour;
			}
		}
	}
}

static VIDEO_UPDATE(deroon)
{

	fillbitmap(bitmap,Machine->pens[0x4000],cliprect);

	tilemap_set_scrolly( bg0tilemap, 0, tecmosys_c80000regs[1]+16);
	tilemap_set_scrollx( bg0tilemap, 0, tecmosys_c80000regs[0]+104);

	tilemap_set_scrolly( bg1tilemap, 0, tecmosys_a80000regs[1]+17);
	tilemap_set_scrollx( bg1tilemap, 0, tecmosys_a80000regs[0]+106);

	tilemap_set_scrolly( bg2tilemap, 0, tecmosys_b00000regs[1]+17);
	tilemap_set_scrollx( bg2tilemap, 0, tecmosys_b00000regs[0]+106);

	fillbitmap(tmp_tilemap_composebitmap,0,cliprect);

	fillbitmap(tmp_tilemap_renderbitmap,0,cliprect);
	tilemap_draw(tmp_tilemap_renderbitmap,cliprect,bg0tilemap,0,0);
	tecmosys_tilemap_copy_to_compose(0x0000);

	fillbitmap(tmp_tilemap_renderbitmap,0,cliprect);
	tilemap_draw(tmp_tilemap_renderbitmap,cliprect,bg1tilemap,0,0);
	tecmosys_tilemap_copy_to_compose(0x4000);

	fillbitmap(tmp_tilemap_renderbitmap,0,cliprect);
	tilemap_draw(tmp_tilemap_renderbitmap,cliprect,bg2tilemap,0,0);
	tecmosys_tilemap_copy_to_compose(0x8000);

	fillbitmap(tmp_tilemap_renderbitmap,0,cliprect);
	tilemap_draw(tmp_tilemap_renderbitmap,cliprect,txt_tilemap,0,0);
	tecmosys_tilemap_copy_to_compose(0xc000);

	tecmosys_do_final_mix(bitmap);

	/* prepare sprites for NEXT frame - causes 1 frame palette errors, but prevents sprite lag in tkdensho, which is correct? */
	tecmosys_render_sprites_to_bitmap(bitmap, tecmosys_880000regs[0x0], tecmosys_880000regs[0x1]);
	
}

/*
>>> R.Belmont wrote:
> Here's the sound info (I got it playing in M1, I
> didn't bother "porting" it since the main game doesn't
> even boot).
>
> memory map:
> 0000-7fff: fixed program ROM
> 8000-bfff: banked ROM
> e000-f7ff: work RAM
>
> I/O ports:

> 0-3: YMF262 OPL3
> 0x10: OKIM6295
> 0x30: bank select, in 0x4000 byte units based at the
> start of the ROM (so 2 = 0x8000).
> 0x40: latch from 68000
> 0x50: latch to 68000
> 0x60/0x61: YMZ280B
>
> IRQ from YMF262 goes to Z80 IRQ.
>
> NMI is asserted when the 68000 writes a command.
>
> Z80 clock appears to be 8 MHz (music slows down in
> "intense" sections if it's 4 MHz, and the crystals are
> all in the area of 16 MHz).
>
> The YMZ280B samples for both games may be misdumped,
> deroon has lots of "bad" noises but tkdensho only has
> a few.
*/


static void sound_irq(int irq)
{
	/* IRQ */
	cpu_set_irq_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}


static struct YMF262interface ymf262_interface =
{
	1,					/* 1 chip */
	14318180,			
	{ YAC512_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) },	/* channels A and B */
	{ YAC512_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) },	/* channels C and D */
	{ sound_irq },		/* irq */
};


static struct OKIM6295interface okim6295_interface =
{
	1,					/* 1 chip */
  { 16000000/8/165 },
	{ REGION_SOUND2 },
	{ 50 }
};

static struct YMZ280Binterface ymz280b_interface =
{
	1,					/* 1 chip */
  { 16934400 },
	{ REGION_SOUND1 },
	{ YM3012_VOL(30,MIXER_PAN_LEFT,30,MIXER_PAN_RIGHT) },
	{ 0 }	/* irq */
};

static MACHINE_DRIVER_START( deroon )
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq1_line_hold,1)

	/* audio CPU */
	MDRV_CPU_ADD(Z80, 16000000/2 )	/* 8 MHz ??? */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PORTS(readport,writeport)

	MDRV_FRAMES_PER_SECOND(57.4458)
	MDRV_VBLANK_DURATION(TIME_IN_USEC(3000))
	MDRV_GFXDECODE(tecmosys_gfxdecodeinfo)

	MDRV_NVRAM_HANDLER(93C46)

	/* MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32) */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN |  VIDEO_RGB_DIRECT | VIDEO_UPDATE_AFTER_VBLANK) /* correct i think */
	MDRV_SCREEN_SIZE(64*8, 64*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 0*8, 30*8-1)
	MDRV_PALETTE_LENGTH(0x4000+0x800)

	MDRV_VIDEO_START(deroon)
	MDRV_MACHINE_INIT(deroon)
	MDRV_VIDEO_UPDATE(deroon)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YMF262,   ymf262_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
	MDRV_SOUND_ADD(YMZ280B,  ymz280b_interface)
MACHINE_DRIVER_END


ROM_START( deroon )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* Main Program */
	ROM_LOAD16_BYTE( "t001.upau1", 0x00000, 0x80000, CRC(14b92c18) SHA1(b47b8c828222a3f7c0fe9271899bd38171d972fb) )
	ROM_LOAD16_BYTE( "t002.upal1", 0x00001, 0x80000, CRC(0fb05c68) SHA1(5140592e15414770fb46d5ac9ba8f76e3d4ab323) )

	ROM_REGION( 0x048000, REGION_CPU2, 0 ) /* Sound Porgram */
	ROM_LOAD( "t003.uz1", 0x000000, 0x008000, CRC(8bdfafa0) SHA1(c0cf3eb7a65d967958fe2aace171859b0faf7753) )
	ROM_CONTINUE(         0x010000, 0x038000 ) /* banked part */

	ROM_REGION( 0x2200, REGION_CPU3, 0 ) /* MCU is a 68HC11A8 with 8k ROM, 512 bytes EEPROM */
	/*
	ROM_LOAD( "deroon_68hc11a8.rom",    0x0000, 0x2000, NO_DUMP )
	ROM_LOAD( "deroon_68hc11a8.eeprom", 0x2000, 0x0200, NO_DUMP )
  */
	ROM_REGION( 0x2000000, REGION_GFX1, ROMREGION_ERASE00 ) /* Sprites (non-tile based) */
	/* all these roms need verifying, they could be half size */

	ROM_LOAD16_BYTE( "t101.uah1", 0x0000000, 0x200000, CRC(74baf845) SHA1(935d2954ba227a894542be492654a2750198e1bc) )
	ROM_LOAD16_BYTE( "t102.ual1", 0x0000001, 0x200000, CRC(1a02c4a3) SHA1(5155eeaef009fc9a9f258e3e54ca2a7f78242df5) )
	/*                            0x8000000, 0x400000 - no rom loaded here, these gfx are 4bpp */
	ROM_LOAD16_BYTE( "t103.ubl1", 0x0800001, 0x400000, CRC(84e7da88) SHA1(b5c3234f33bb945cc9762b91db087153a0589cfb) )
	/*                            0x1000000, 0x400000 - no rom loaded here, these gfx are 4bpp */
	ROM_LOAD16_BYTE( "t104.ucl1", 0x1000001, 0x200000, CRC(66eb611a) SHA1(64435d35677fea3c06fdb03c670f3f63ee481c02) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE ) /* 8x8 4bpp tiles */
	ROM_LOAD( "t301.ubd1", 0x000000, 0x100000, CRC(8b026177) SHA1(3887856bdaec4d9d3669fe3bc958ef186fbe9adb) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_ERASE00) /* 16x16 4bpp tiles */
	/* not used? */

	ROM_REGION( 0x100000, REGION_GFX4, ROMREGION_ERASE00 ) /* 16x16 4bpp tiles */
	ROM_LOAD( "t201.ubb1", 0x000000, 0x100000, CRC(d5a087ac) SHA1(5098160ce7719d93e3edae05f6edd317d4c61f0d) )

	ROM_REGION( 0x100000, REGION_GFX5, ROMREGION_ERASE00 ) /* 16x16 4bpp tiles */
	ROM_LOAD( "t202.ubc1", 0x000000, 0x100000, CRC(f051dae1) SHA1(f5677c07fe644b3838657370f0309fb09244c619) )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 ) /* YMZ280B Samples */
	ROM_LOAD( "t401.uya1", 0x000000, 0x200000, CRC(92111992) SHA1(ae27e11ae76dec0b9892ad32e1a8bf6ab11f2e6c) )

	ROM_REGION( 0x100000, REGION_SOUND2, 0 ) /* M6295 Samples */
	ROM_LOAD( "t501.uad1", 0x080000, 0x080000, CRC(2fbcfe27) SHA1(f25c830322423f0959a36955edb563a6150f2142) )
ROM_END

/*
About the Deroon DeroDero listed below:
 This set contains less Japanese text and English translations for some game aspects such as game menus.
 Coining up displays "TECMO STACKERS" but this set doesn't seem to include a "How to Play" demo like the parent set
 PCB contained genuine Tecmo development labels, it's unknown which specific region this set was intended for.
 Still missing the full English version titled Temco Stackers if it exists.
*/

ROM_START( deroon2 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* Main Program */
	ROM_LOAD16_BYTE( "stk_t01.upau1", 0x00000, 0x80000, CRC(90c794df) SHA1(b6edd62bedf609551f4e1c19ada20bd1373deca2) )
	ROM_LOAD16_BYTE( "stk_t02.upal1", 0x00001, 0x80000, CRC(cca9f87c) SHA1(0637b0b979f4c6c6b16cf2a21dd193b7d7ec311f) )

	ROM_REGION( 0x048000, REGION_CPU2, 0 ) /* Sound Porgram */
	ROM_LOAD( "t003.uz1", 0x000000, 0x008000, CRC(8bdfafa0) SHA1(c0cf3eb7a65d967958fe2aace171859b0faf7753) )
	ROM_CONTINUE(         0x010000, 0x038000 ) /* banked part */

	ROM_REGION( 0x2200, REGION_CPU3, 0 ) /* MCU is a 68HC11A8 with 8k ROM, 512 bytes EEPROM */
	/*
	ROM_LOAD( "deroon_68hc11a8.rom",    0x0000, 0x2000, NO_DUMP )
	ROM_LOAD( "deroon_68hc11a8.eeprom", 0x2000, 0x0200, NO_DUMP )
  */
	ROM_REGION( 0x2000000, REGION_GFX1, ROMREGION_ERASE00 ) /* Sprites (non-tile based) */
	/* all these roms need verifying, they could be half size */

	ROM_LOAD16_BYTE( "t101.uah1", 0x0000000, 0x200000, CRC(74baf845) SHA1(935d2954ba227a894542be492654a2750198e1bc) )
	ROM_LOAD16_BYTE( "t102.ual1", 0x0000001, 0x200000, CRC(1a02c4a3) SHA1(5155eeaef009fc9a9f258e3e54ca2a7f78242df5) )
	/*                            0x8000000, 0x400000 - no rom loaded here, these gfx are 4bpp */
	ROM_LOAD16_BYTE( "t103.ubl1", 0x0800001, 0x400000, CRC(84e7da88) SHA1(b5c3234f33bb945cc9762b91db087153a0589cfb) )
	/*                            0x1000000, 0x400000 - no rom loaded here, these gfx are 4bpp */
	ROM_LOAD16_BYTE( "t104.ucl1", 0x1000001, 0x200000, CRC(66eb611a) SHA1(64435d35677fea3c06fdb03c670f3f63ee481c02) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE ) /* 8x8 4bpp tiles */
	ROM_LOAD( "t301.ubd1", 0x000000, 0x100000, CRC(8b026177) SHA1(3887856bdaec4d9d3669fe3bc958ef186fbe9adb) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_ERASE00) /* 16x16 4bpp tiles */
	/* not used? */

	ROM_REGION( 0x100000, REGION_GFX4, ROMREGION_ERASE00 ) /* 16x16 4bpp tiles */
	ROM_LOAD( "t201.ubb1", 0x000000, 0x100000, CRC(d5a087ac) SHA1(5098160ce7719d93e3edae05f6edd317d4c61f0d) )

	ROM_REGION( 0x100000, REGION_GFX5, ROMREGION_ERASE00 ) /* 16x16 4bpp tiles */
	ROM_LOAD( "t202.ubc1", 0x000000, 0x100000, CRC(f051dae1) SHA1(f5677c07fe644b3838657370f0309fb09244c619) )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 ) /* YMZ280B Samples */
	ROM_LOAD( "t401.uya1", 0x000000, 0x200000, CRC(92111992) SHA1(ae27e11ae76dec0b9892ad32e1a8bf6ab11f2e6c) )

	ROM_REGION( 0x100000, REGION_SOUND2, 0 ) /* M6295 Samples */
	ROM_LOAD( "t501.uad1", 0x080000, 0x080000, CRC(2fbcfe27) SHA1(f25c830322423f0959a36955edb563a6150f2142) )
ROM_END

ROM_START( tkdensho )
	ROM_REGION( 0x600000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "aeprge-2.pal", 0x00000, 0x80000, CRC(25e453d6) SHA1(9c84e2af42eff5cc9b14c1759d5bab42fa7bb663) )
	ROM_LOAD16_BYTE( "aeprgo-2.pau", 0x00001, 0x80000, CRC(22d59510) SHA1(5ade482d6ab9a22df2ee8337458c22cfa9045c73) )

	ROM_REGION( 0x038000, REGION_CPU2, 0 ) /* Sound Porgram */
	ROM_LOAD( "aesprg-2.z1", 0x000000, 0x008000, CRC(43550ab6) SHA1(2580129ef8ebd9295249175de4ba985c752e06fe) )
	ROM_CONTINUE(            0x010000, 0x018000 ) /* banked part */

	ROM_REGION( 0x2200, REGION_CPU3, 0 ) /* MCU is a 68HC11A8 with 8k ROM, 512 bytes EEPROM */
	/*
	ROM_LOAD( "tkdensho_68hc11a8.rom",    0x0000, 0x2000, NO_DUMP )
	ROM_LOAD( "tkdensho_68hc11a8.eeprom", 0x2000, 0x0200, NO_DUMP )
  */
	ROM_REGION( 0x4000000, REGION_GFX1, ROMREGION_ERASE00 ) /* Graphics - mostly (maybe all?) not tile based */
	ROM_LOAD16_BYTE( "ae100h.ah1",    0x0000000, 0x0400000, CRC(06be252b) SHA1(08d1bb569fd2e66e2c2f47da7780b31945232e62) )
	ROM_LOAD16_BYTE( "ae100.al1",     0x0000001, 0x0400000, CRC(009cdff4) SHA1(fd88f07313d14fd4429b09a1e8d6b595df3b98e5) )
	ROM_LOAD16_BYTE( "ae101h.bh1",    0x0800000, 0x0400000, CRC(f2469eff) SHA1(ba49d15cc7949437ba9f56d9b425a5f0e62137df) )
	ROM_LOAD16_BYTE( "ae101.bl1",     0x0800001, 0x0400000, CRC(db7791bb) SHA1(1fe40b747b7cee7a9200683192b1d60a735a0446) )
	ROM_LOAD16_BYTE( "ae102h.ch1",    0x1000000, 0x0200000, CRC(f9d2a343) SHA1(d141ac0b20be587e77a576ef78f15d269d9c84e5) )
	ROM_LOAD16_BYTE( "ae102.cl1",     0x1000001, 0x0200000, CRC(681be889) SHA1(8044ca7cbb325e6dcadb409f91e0c01b88a1bca7) )
	ROM_LOAD16_BYTE( "ae104.el1",     0x2000001, 0x0400000, CRC(e431b798) SHA1(c2c24d4f395bba8c78a45ecf44009a830551e856) )
	ROM_LOAD16_BYTE( "ae105.fl1",     0x2800001, 0x0400000, CRC(b7f9ebc1) SHA1(987f664072b43a578b39fa6132aaaccc5fe5bfc2) )
	ROM_LOAD16_BYTE( "ae106.gl1",     0x3000001, 0x0200000, CRC(7c50374b) SHA1(40865913125230122072bb13f46fb5fb60c088ea) )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE ) /* 8x8 4bpp tiles */
	ROM_LOAD( "ae300w36.bd1",  0x000000, 0x0080000, CRC(e829f29e) SHA1(e56bfe2669ed1d1ae394c644def426db129d97e3) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE ) /* 16x16 4bpp tiles */
	ROM_LOAD( "ae200w74.ba1",  0x000000, 0x0100000, CRC(c1645041) SHA1(323670a6aa2a4524eb968cc0b4d688098ffeeb12) )

	ROM_REGION( 0x100000, REGION_GFX4, ROMREGION_DISPOSE ) /* 16x16 4bpp tiles */
	ROM_LOAD( "ae201w75.bb1",  0x000000, 0x0100000, CRC(3f63bdff) SHA1(0d3d57fdc0ec4bceef27c11403b3631d23abadbf) )

	ROM_REGION( 0x100000, REGION_GFX5, ROMREGION_DISPOSE ) /* 16x16 4bpp tiles */
	ROM_LOAD( "ae202w76.bc1",  0x000000, 0x0100000, CRC(5cc857ca) SHA1(2553fb5220433acc15dfb726dc064fe333e51d88) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* YMZ280B Samples */
	ROM_LOAD( "ae400t23.ya1", 0x000000, 0x200000, CRC(c6ffb043) SHA1(e0c6c5f6b840f63c9a685a2c3be66efa4935cbeb) )
	ROM_LOAD( "ae401t24.yb1", 0x200000, 0x200000, CRC(d83f1a73) SHA1(412b7ac9ff09a984c28b7d195330d78c4aac3dc5) )

	ROM_REGION( 0x100000, REGION_SOUND2, 0 ) /* M6295 Samples */
	ROM_LOAD( "ae500w07.ad1", 0x080000, 0x080000, CRC(3734f92c) SHA1(048555b5aa89eaf983305c439ba08d32b4a1bb80) )
ROM_END

ROM_START( tkdensha )
	ROM_REGION( 0x600000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "aeprge.pal", 0x00000, 0x80000, CRC(17a209ff) SHA1(b5dbea9868cbb89d4e27bf19fdb616ac256985b4) )
	ROM_LOAD16_BYTE( "aeprgo.pau", 0x00001, 0x80000, CRC(d265e6a1) SHA1(f39d8ce115f197a660f5210b2483108854eb12a9) )

	ROM_REGION( 0x038000, REGION_CPU2, 0 ) /* Sound Program */
	ROM_LOAD( "aesprg-2.z1", 0x000000, 0x008000, CRC(43550ab6) SHA1(2580129ef8ebd9295249175de4ba985c752e06fe) )
	ROM_CONTINUE(            0x010000, 0x018000 ) /* banked part */

	ROM_REGION( 0x2200, REGION_CPU3, 0 ) /* MCU is a 68HC11A8 with 8k ROM, 512 bytes EEPROM */
	/*
	ROM_LOAD( "tkdensho_68hc11a8.rom",    0x0000, 0x2000, NO_DUMP )
	ROM_LOAD( "tkdensho_68hc11a8.eeprom", 0x2000, 0x0200, NO_DUMP )
  */
	ROM_REGION( 0x4000000, REGION_GFX1, ROMREGION_ERASE00 ) /* Graphics - mostly (maybe all?) not tile based */
	ROM_LOAD16_BYTE( "ae100h.ah1",    0x0000000, 0x0400000, CRC(06be252b) SHA1(08d1bb569fd2e66e2c2f47da7780b31945232e62) )
	ROM_LOAD16_BYTE( "ae100.al1",     0x0000001, 0x0400000, CRC(009cdff4) SHA1(fd88f07313d14fd4429b09a1e8d6b595df3b98e5) )
	ROM_LOAD16_BYTE( "ae101h.bh1",    0x0800000, 0x0400000, CRC(f2469eff) SHA1(ba49d15cc7949437ba9f56d9b425a5f0e62137df) )
	ROM_LOAD16_BYTE( "ae101.bl1",     0x0800001, 0x0400000, CRC(db7791bb) SHA1(1fe40b747b7cee7a9200683192b1d60a735a0446) )
	ROM_LOAD16_BYTE( "ae102h.ch1",    0x1000000, 0x0200000, CRC(f9d2a343) SHA1(d141ac0b20be587e77a576ef78f15d269d9c84e5) )
	ROM_LOAD16_BYTE( "ae102.cl1",     0x1000001, 0x0200000, CRC(681be889) SHA1(8044ca7cbb325e6dcadb409f91e0c01b88a1bca7) )
	ROM_LOAD16_BYTE( "ae104.el1",     0x2000001, 0x0400000, CRC(e431b798) SHA1(c2c24d4f395bba8c78a45ecf44009a830551e856) )
	ROM_LOAD16_BYTE( "ae105.fl1",     0x2800001, 0x0400000, CRC(b7f9ebc1) SHA1(987f664072b43a578b39fa6132aaaccc5fe5bfc2) )
	ROM_LOAD16_BYTE( "ae106.gl1",     0x3000001, 0x0200000, CRC(7c50374b) SHA1(40865913125230122072bb13f46fb5fb60c088ea) )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE ) /* 8x8 4bpp tiles */
	ROM_LOAD( "ae300w36.bd1",  0x000000, 0x0080000, CRC(e829f29e) SHA1(e56bfe2669ed1d1ae394c644def426db129d97e3) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE ) /* 16x16 4bpp tiles */
	ROM_LOAD( "ae200w74.ba1",  0x000000, 0x0100000, CRC(c1645041) SHA1(323670a6aa2a4524eb968cc0b4d688098ffeeb12) )

	ROM_REGION( 0x100000, REGION_GFX4, ROMREGION_DISPOSE ) /* 16x16 4bpp tiles */
	ROM_LOAD( "ae201w75.bb1",  0x000000, 0x0100000, CRC(3f63bdff) SHA1(0d3d57fdc0ec4bceef27c11403b3631d23abadbf) )

	ROM_REGION( 0x100000, REGION_GFX5, ROMREGION_DISPOSE ) /* 16x16 4bpp tiles */
	ROM_LOAD( "ae202w76.bc1",  0x000000, 0x0100000, CRC(5cc857ca) SHA1(2553fb5220433acc15dfb726dc064fe333e51d88) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* YMZ280B Samples */
	ROM_LOAD( "ae400t23.ya1", 0x000000, 0x200000, CRC(c6ffb043) SHA1(e0c6c5f6b840f63c9a685a2c3be66efa4935cbeb) )
	ROM_LOAD( "ae401t24.yb1", 0x200000, 0x200000, CRC(d83f1a73) SHA1(412b7ac9ff09a984c28b7d195330d78c4aac3dc5) )

	ROM_REGION( 0x100000, REGION_SOUND2, 0 ) /* M6295 Samples */
	ROM_LOAD( "ae500w07.ad1", 0x080000, 0x080000, CRC(3734f92c) SHA1(048555b5aa89eaf983305c439ba08d32b4a1bb80) )
ROM_END

static void reset_callback(int param)
{
	cpu_set_reset_line(0, PULSE_LINE);
}

static MACHINE_INIT( deroon )
{
	device_read_ptr = 0;
	device_status = DS_IDLE;
}

void tecmosys_decramble(void)
{
	UINT8 *gfxsrc    = memory_region       ( REGION_GFX1 );
	size_t  srcsize = memory_region_length( REGION_GFX1 );
	int i;

	for (i=0; i < srcsize; i+=4)
	{
		UINT8 tmp[4];

		tmp[2] = ((gfxsrc[i+0]&0xf0)>>0) | ((gfxsrc[i+1]&0xf0)>>4); /*  0,1,2,3  8,9,10, 11 */
		tmp[3] = ((gfxsrc[i+0]&0x0f)<<4) | ((gfxsrc[i+1]&0x0f)<<0); /* 4,5,6,7, 12,13,14,15 */
		tmp[0] = ((gfxsrc[i+2]&0xf0)>>0) | ((gfxsrc[i+3]&0xf0)>>4); /* 16,17,18,19,24,25,26,27 */
		tmp[1] = ((gfxsrc[i+2]&0x0f)<<4) | ((gfxsrc[i+3]&0x0f)>>0); /* 20,21,22,23, 28,29,30,31 */

		gfxsrc[i+0] = tmp[0];
		gfxsrc[i+1] = tmp[1];
		gfxsrc[i+2] = tmp[2];
		gfxsrc[i+3] = tmp[3];

	}

}

static DRIVER_INIT( deroon )
{
	tecmosys_decramble();
	device_data = &deroon_data;
	timer_set(TIME_IN_SEC(2),0,reset_callback);
}

static DRIVER_INIT( tkdensho )
{
	tecmosys_decramble();
	device_data = &tkdensho_data;
	timer_set(TIME_IN_SEC(2),0,reset_callback);
}

static DRIVER_INIT( tkdensha )
{
	tecmosys_decramble();
	device_data = &tkdensha_data;
	timer_set(TIME_IN_SEC(2),0,reset_callback);
}

GAME( 1995, deroon,      0,        deroon, deroon, deroon,     ROT0, "Tecmo", "Deroon DeroDero" )
GAME( 1995, deroon2,     deroon,   deroon, deroon, deroon,     ROT0, "Tecmo", "Deroon DeroDero / Tecmo Stackers" )
GAME( 1996, tkdensho,    0,        deroon, deroon, tkdensho,   ROT0, "Tecmo", "Toukidenshou - Angel Eyes (VER. 960614)" )
GAME( 1996, tkdensha,    tkdensho, deroon, deroon, tkdensha,   ROT0, "Tecmo", "Toukidenshou - Angel Eyes (VER. 960427)" )

