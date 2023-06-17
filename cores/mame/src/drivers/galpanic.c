/***************************************************************************

Gals Panic       1990 Kaneko
Fantasia         1994 Comad
New Fantasia     1995 Comad
Fantasy '95      1995 Hi-max Technology Inc. (Running on a Comad PCB)
Miss World '96   1996 Comad
Fantasia II      1997 Comad
Gals Hustler     1997 Ace International

driver by Nicola Salmoria

The Comad games run on hardware similar to Gals Panic, with a different
sprite system. They are not ROM swaps because the addresses of work RAM
and of the OKI chip change from one to the other, however everything else
is pretty much identical.


TODO:
- There is a vector for IRQ4. The function does nothing in galpanic but is
  more complicated in the Comad ones. However I'm not triggering it, and
  they seems to work anyway...
- Four unknown ROMs in fantasia. The game seems to work fine without them.
- There was a ROM in the newfant set, obj2_14.rom, which was identical to
  Terminator 2's t2.107. I can only assume this was a mistake of the dumper.
- lots of unknown reads and writes, also in galpanic but particularly in
  the Comad ones.
- fantasia and newfant have a service mode but they doesn't work well (text
  is missing or replaced by garbage). This might be just how the games are.
- Is there a background enable register? Or a background clear. fantasia and
  newfant certainly look ugly as they are.

Notes about Gals Panic:
-----------------------
The current ROM set is strange because two ROMs overlap two others replacing
the program.

It's definitely a Kaneko boardset, but it could very well be they converted
some other game to run Gals Panic, because there's some ROMs piggybacked
on top of each other and some ROMs on a daughterboard plugged into smaller
sized ROM sockets. It's not a pirate version. The piggybacked ROMs even have
Kaneko stickers. The silkscreen on the board says PAMERA-4.

There is at least another version of the Gals Panic board. It's single board,
so no daughterboard. There are only 4 IC's socketed, the rest is soldered to
the board, and no piggybacked ROMs. Board number is MDK 321 V-0    EXPRO-02


Stephh's additional notes :

  - The games might not work correctly if sound is disabled.
  - There seems to exist 3 versions of 'galpanic' (Japan, US and World),
    and we seem to have a World version according to the coinage.
    Version is stored at 0x03ffff.b :
      * 01 : Japan
      * 02 : US
      * 03 : World
    In the version we have, you can only have one type of coinage
    (there is no Dip Switch to change sort of "coin mode").
  - In Comad games, here is a possible explanation of why the "Tilt" button
    may hang the game and/or crash/exit MAME : if you look carefully at the
    code, you'll notice that you have a "rts" instruction WITHOUT restoring
    the registers saved by the "movem.l D0-D7/A0-A6, -(A7)" instruction.
    Then, a "rte" instruction is performed.
  - The "Demo Sounds" Dip Switch is told not to work and not to fit the
    manual, but it appears that 00 seems to be read from in the "trap $d"
    interruption. Is it because the addresses (0x53e830-0x53e84f) are also
    used for 'galpanic_bgvideoram' ?
    In the Comad games, the interruption is the same, but the addresses
    which are checked are in full RAM. So the Dip Switch could be checked.

  - I've added a "fake" 'galpanib' romset which is in fact the same as
    'galpanic', but with the PRG ROMS which aren't overwritten.
    Here are a few notes about what I found :
      * This version is also a World version (0x03ffff.b = 03).
      * In this version, there is a "Coin Mode" Dip Switch, but no
        "Character Test" Dip Switch.
      * Area 0xe00000-0xe00014 is a "calculator" area. I've tried to
        simulate it (see 'galpanib_calc*' handlers) by comparing the code
        with the other set. I don't know if there are some other unmapped
        reads, but the game seems to run fine with what I've done.
      * When you press the "Tilt" button, the game enters in an endless
        loop, but this isn't a bug ! Check code begining at 0x000e02 and
        ending at 0x000976 for more infos.
          -Expects watchdog to reset it- pjp
      * Sound hasn't been tested.
      * The Comad games are definitively based on this version, the main
        differences being that read/writes to 0xe00000 have been replaced.

 - On Gals Hustler there is an extra test mode if you hold down player 2
   button 1, I have no idea if its complete or not

***************************************************************************/

#include "driver.h"
#include "machine/random.h"
#include "vidhrdw/generic.h"



extern data16_t *galpanic_bgvideoram,*galpanic_fgvideoram;
extern size_t galpanic_fgvideoram_size;

PALETTE_INIT( galpanic );
WRITE16_HANDLER( galpanic_bgvideoram_w );
WRITE16_HANDLER( galpanic_paletteram_w );
VIDEO_UPDATE( galpanic );
VIDEO_UPDATE( comad );




static INTERRUPT_GEN( galpanic_interrupt )
{
	/* IRQ 3 drives the game, IRQ 5 updates the palette */
	if (cpu_getiloops() != 0)
		cpu_set_irq_line(0, 5, HOLD_LINE);
	else
		cpu_set_irq_line(0, 3, HOLD_LINE);
}

static INTERRUPT_GEN( galhustl_interrupt )
{
	switch ( cpu_getiloops() )
	{
		case 2:  cpu_set_irq_line(0, 5, HOLD_LINE); break;
		case 1:  cpu_set_irq_line(0, 4, HOLD_LINE); break;
		case 0:  cpu_set_irq_line(0, 3, HOLD_LINE); break;
	}
}


static WRITE16_HANDLER( galpanic_6295_bankswitch_w )
{
	if (ACCESSING_MSB)
	{
		UINT8 *rom = memory_region(REGION_SOUND1);

		memcpy(&rom[0x30000],&rom[0x40000 + ((data >> 8) & 0x0f) * 0x10000],0x10000);
	}
}


static data16_t *galpanib_calc_data;

static struct {
	UINT16 x1p, y1p, x1s, y1s;
	UINT16 x2p, y2p, x2s, y2s;

	INT16 x12, y12, x21, y21;

	UINT16 mult_a, mult_b;
} hit;

static READ16_HANDLER(galpanib_calc_r)
{
	UINT16 data = 0;

	switch (offset)
	{
		case 0x00 >> 1:	/* watchdog*/
			return watchdog_reset_r(0);

		case 0x04 >> 1:
			/* This is similar to the hit detection from SuperNova, but much simpler */

			/* X Absolute Collision*/
			if      (hit.x1p >  hit.x2p)	data |= 0x0200;
			else if (hit.x1p == hit.x2p)	data |= 0x0400;
			else if (hit.x1p <  hit.x2p)	data |= 0x0800;

			/* Y Absolute Collision*/
			if      (hit.y1p >  hit.y2p)	data |= 0x2000;
			else if (hit.y1p == hit.y2p)	data |= 0x4000;
			else if (hit.y1p <  hit.y2p)	data |= 0x8000;

			/* XY Overlap Collision*/
			hit.x12 = (hit.x1p) - (hit.x2p + hit.x2s);
			hit.y12 = (hit.y1p) - (hit.y2p + hit.y2s);
			hit.x21 = (hit.x1p + hit.x1s) - (hit.x2p);
			hit.y21 = (hit.y1p + hit.y1s) - (hit.y2p);

			if ((hit.x12 < 0) && (hit.y12 < 0) &&
				(hit.x21 >= 0) && (hit.y21 >= 0))
					data |= 0x0001;

			return data;

		case 0x10 >> 1:
			return (((UINT32)hit.mult_a * (UINT32)hit.mult_b) >> 16);
		case 0x12 >> 1:
			return (((UINT32)hit.mult_a * (UINT32)hit.mult_b) & 0xffff);

		case 0x14 >> 1:
			return (mame_rand() & 0xffff);

		default:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #0 PC %06x: warning - read unmapped calc address %06x\n",activecpu_get_pc(),offset<<1);
	}

	return 0;
}

static WRITE16_HANDLER(galpanib_calc_w)
{
	switch (offset)
	{
		/* p is position, s is size*/
		case 0x00 >> 1:
			hit.x1p = data;
			break;
		case 0x02 >> 1:
			hit.x1s = data;
			break;
		case 0x04 >> 1:
			hit.y1p = data;
			break;
		case 0x06 >> 1:
			hit.y1s = data;
			break;
		case 0x08 >> 1:
			hit.x2p = data;
			break;
		case 0x0a >> 1:
			hit.x2s = data;
			break;
		case 0x0c >> 1:
			hit.y2p = data;
			break;
		case 0x0e >> 1:
			hit.y2s = data;
			break;
		case 0x10 >> 1:
			hit.mult_a = data;
			break;
		case 0x12 >> 1:
			hit.mult_b = data;
			break;
		default:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #0 PC %06x: warning - write unmapped hit address %06x\n",activecpu_get_pc(),offset<<1);
	}
}

static WRITE16_HANDLER( galpanic_bgvideoram_mirror_w )
{
	int i;
	for(i = 0; i < 8; i++)
	{
		/* or offset + i * 0x2000 ?*/
		galpanic_bgvideoram_w(offset * 8 + i, data, mem_mask);
	}
}

static MEMORY_READ16_START( galpanic_readmem )
	{ 0x000000, 0x3fffff, MRA16_ROM },
	{ 0x400000, 0x400001, OKIM6295_status_0_lsb_r },
	{ 0x500000, 0x51ffff, MRA16_RAM },
	{ 0x520000, 0x53ffff, MRA16_RAM },
	{ 0x600000, 0x6007ff, MRA16_RAM },
	{ 0x700000, 0x7047ff, MRA16_RAM },
	{ 0x800000, 0x800001, input_port_0_word_r },
	{ 0x800002, 0x800003, input_port_1_word_r },
	{ 0x800004, 0x800005, input_port_2_word_r },
MEMORY_END

static MEMORY_WRITE16_START( galpanic_writemem )
	{ 0x000000, 0x3fffff, MWA16_ROM },
	{ 0x400000, 0x400001, OKIM6295_data_0_lsb_w },
	{ 0x500000, 0x51ffff, MWA16_RAM, &galpanic_fgvideoram, &galpanic_fgvideoram_size },
	{ 0x520000, 0x53ffff, galpanic_bgvideoram_w, &galpanic_bgvideoram },	/* + work RAM */
	{ 0x600000, 0x6007ff, galpanic_paletteram_w, &paletteram16 },	/* 1024 colors, but only 512 seem to be used */
	{ 0x700000, 0x7047ff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x900000, 0x900001, galpanic_6295_bankswitch_w },
	{ 0xa00000, 0xa00001, MWA16_NOP },	/* ??? */
	{ 0xb00000, 0xb00001, MWA16_NOP },	/* ??? */
	{ 0xc00000, 0xc00001, MWA16_NOP },	/* ??? */
	{ 0xd00000, 0xd00001, MWA16_NOP },	/* ??? */
MEMORY_END

static MEMORY_READ16_START( galpanib_readmem )
	{ 0x000000, 0x3fffff, MRA16_ROM },
	{ 0x400000, 0x400001, OKIM6295_status_0_lsb_r },
	{ 0x500000, 0x51ffff, MRA16_RAM },
	{ 0x520000, 0x53ffff, MRA16_RAM },
	{ 0x600000, 0x6007ff, MRA16_RAM },
	{ 0x700000, 0x7047ff, MRA16_RAM },
	{ 0x800000, 0x800001, input_port_0_word_r },
	{ 0x800002, 0x800003, input_port_1_word_r },
	{ 0x800004, 0x800005, input_port_2_word_r },
	{ 0xe00000, 0xe00015, galpanib_calc_r },
MEMORY_END

static MEMORY_WRITE16_START( galpanib_writemem )
	{ 0x000000, 0x3fffff, MWA16_ROM },
	{ 0x400000, 0x400001, OKIM6295_data_0_lsb_w },
	{ 0x500000, 0x51ffff, MWA16_RAM, &galpanic_fgvideoram, &galpanic_fgvideoram_size },
	{ 0x520000, 0x53ffff, galpanic_bgvideoram_w, &galpanic_bgvideoram },	/* + work RAM */
	{ 0x600000, 0x6007ff, galpanic_paletteram_w, &paletteram16 },	/* 1024 colors, but only 512 seem to be used */
	{ 0x700000, 0x7047ff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x900000, 0x900001, galpanic_6295_bankswitch_w },
	{ 0xa00000, 0xa00001, MWA16_NOP },	/* ??? */
	{ 0xb00000, 0xb00001, MWA16_NOP },	/* ??? */
	{ 0xc00000, 0xc00001, MWA16_NOP },	/* ??? */
	{ 0xd00000, 0xd00001, MWA16_NOP },	/* ??? */
	{ 0xe00000, 0xe00015, galpanib_calc_w, &galpanib_calc_data },
MEMORY_END

static READ16_HANDLER( kludge )
{
	return mame_rand() & 0x0700;
}

static MEMORY_READ16_START( comad_readmem )
	{ 0x000000, 0x4fffff, MRA16_ROM },
	{ 0x500000, 0x51ffff, MRA16_RAM },
	{ 0x520000, 0x53ffff, MRA16_RAM },
	{ 0x600000, 0x6007ff, MRA16_RAM },
	{ 0x700000, 0x700fff, MRA16_RAM },
	{ 0x800000, 0x800001, input_port_0_word_r },
	{ 0x800002, 0x800003, input_port_1_word_r },
	{ 0x800004, 0x800005, input_port_2_word_r },
/*	{ 0x800006, 0x800007,  },	??*/
	{ 0x80000a, 0x80000b, kludge },	/* bits 8-a = timer? palette update code waits for them to be 111 */
	{ 0x80000c, 0x80000d, kludge },	/* missw96 bits 8-a = timer? palette update code waits for them to be 111 */
	{ 0xc00000, 0xc0ffff, MRA16_RAM },	/* missw96 */
	{ 0xc80000, 0xc8ffff, MRA16_RAM },	/* fantasia, newfant */
	{ 0xf00000, 0xf00001, OKIM6295_status_0_msb_r },	/* fantasia, missw96 */
	{ 0xf80000, 0xf80001, OKIM6295_status_0_msb_r },	/* newfant */
MEMORY_END

static MEMORY_WRITE16_START( comad_writemem )
	{ 0x000000, 0x4fffff, MWA16_ROM },
	{ 0x500000, 0x51ffff, MWA16_RAM, &galpanic_fgvideoram, &galpanic_fgvideoram_size },
	{ 0x520000, 0x53ffff, galpanic_bgvideoram_w, &galpanic_bgvideoram },	/* + work RAM */
	{ 0x600000, 0x6007ff, galpanic_paletteram_w, &paletteram16 },	/* 1024 colors, but only 512 seem to be used */
	{ 0x700000, 0x700fff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x900000, 0x900001, galpanic_6295_bankswitch_w },	/* not sure */
	{ 0xc00000, 0xc0ffff, MWA16_RAM },	/* missw96 */
	{ 0xc80000, 0xc8ffff, MWA16_RAM },	/* fantasia, newfant */
	{ 0xf00000, 0xf00001, OKIM6295_data_0_msb_w },	/* fantasia, missw96 */
	{ 0xf80000, 0xf80001, OKIM6295_data_0_msb_w },	/* newfant */
MEMORY_END

static MEMORY_READ16_START( fantsia2_readmem )
	{ 0x000000, 0x4fffff, MRA16_ROM },
	{ 0x500000, 0x51ffff, MRA16_RAM },
	{ 0x520000, 0x53ffff, MRA16_RAM },
	{ 0x600000, 0x6007ff, MRA16_RAM },
	{ 0x700000, 0x700fff, MRA16_RAM },
	{ 0x800000, 0x800001, input_port_0_word_r },
	{ 0x800002, 0x800003, input_port_1_word_r },
	{ 0x800004, 0x800005, input_port_2_word_r },
/*	{ 0x800006, 0x800007,  },	??*/
	{ 0x800008, 0x800009, kludge },	/* bits 8-a = timer? palette update code waits for them to be 111 */
	{ 0xf80000, 0xf8ffff, MRA16_RAM },
	{ 0xc80000, 0xc80001, OKIM6295_status_0_msb_r },
MEMORY_END

static MEMORY_WRITE16_START( fantsia2_writemem )
	{ 0x000000, 0x4fffff, MWA16_ROM },
	{ 0x500000, 0x51ffff, MWA16_RAM, &galpanic_fgvideoram, &galpanic_fgvideoram_size },
	{ 0x520000, 0x53ffff, galpanic_bgvideoram_w, &galpanic_bgvideoram },	/* + work RAM */
	{ 0x600000, 0x6007ff, galpanic_paletteram_w, &paletteram16 },	/* 1024 colors, but only 512 seem to be used */
	{ 0x700000, 0x700fff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x900000, 0x900001, galpanic_6295_bankswitch_w },	/* not sure */
	{ 0xf80000, 0xf8ffff, MWA16_RAM },
	{ 0xa00000, 0xa00001, MWA16_NOP },	/* coin counters, + ? */
	{ 0xc80000, 0xc80001, OKIM6295_data_0_msb_w },
MEMORY_END


static MEMORY_READ16_START( galhustl_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM },
    { 0x500000, 0x51ffff, MRA16_RAM },
	{ 0x580000, 0x583fff, MRA16_RAM },
	{ 0x600000, 0x6007ff, MRA16_RAM },
	{ 0x600800, 0x600fff, MRA16_RAM },
	{ 0x680000, 0x68001f, MRA16_RAM },
	{ 0x700000, 0x700fff, MRA16_RAM },
	{ 0x780000, 0x78001f, MRA16_RAM },
	{ 0x800000, 0x800001, input_port_0_word_r },
	{ 0x800002, 0x800003, input_port_1_word_r },
	{ 0x800004, 0x800005, input_port_2_word_r },
	{ 0xd00000, 0xd00001, OKIM6295_status_0_msb_r },
	{ 0xe80000, 0xe8ffff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( galhustl_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM },
    { 0x500000, 0x51ffff, MWA16_RAM, &galpanic_fgvideoram, &galpanic_fgvideoram_size },
	{ 0x520000, 0x53ffff, galpanic_bgvideoram_w, &galpanic_bgvideoram },
	{ 0x580000, 0x583fff, galpanic_bgvideoram_mirror_w },
	{ 0x600000, 0x6007ff, galpanic_paletteram_w, &paletteram16 },	/* 1024 colors, but only 512 seem to be used */
	{ 0x600800, 0x600fff, MWA16_RAM }, /* writes only 1?*/
	{ 0x680000, 0x68001f, MWA16_RAM }, /* regs?*/
	{ 0x700000, 0x700fff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x780000, 0x78001f, MWA16_RAM }, /* regs?*/
	{ 0xa00000, 0xa00001, MWA16_NOP }, /* ?*/
	{ 0x900000, 0x900001, galpanic_6295_bankswitch_w },
	{ 0xd00000, 0xd00001, OKIM6295_data_0_msb_w },
	{ 0xe80000, 0xe8ffff, MWA16_RAM },
MEMORY_END


INPUT_PORTS_START( galpanic )
	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )	/* flip screen? - code at 0x000522 */
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0004, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	/* Coinage - World (0x03ffff.b = 03) */
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	/* Coinage - Japan (0x03ffff.b = 01) and US (0x03ffff.b = 02)
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 1C_2C ) )
	*/
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )		/* "Shot2" in "test mode" */
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0002, "Easy" )
	PORT_DIPSETTING(      0x0003, "Normal" )
	PORT_DIPSETTING(      0x0001, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0010, "2" )
	PORT_DIPSETTING(      0x0030, "3" )
	PORT_DIPSETTING(      0x0020, "4" )
	PORT_DIPSETTING(      0x0000, "5" )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )	/* demo sounds? - see notes */
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Character Test" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )		/* "Shot2" in "test mode" */
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( galpanib )
	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )	/* flip screen? - code at 0x00060a */
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0004, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0008, 0x0008, "Coin Mode" )
	PORT_DIPSETTING(      0x0008, "Mode 1" )
	PORT_DIPSETTING(      0x0000, "Mode 2" )
	/* Coinage - Japan (0x03ffff.b = 01)
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 1C_2C ) )
	*/
	/* Coin Mode 1 - US (0x03ffff.b = 02) and World (0x03ffff.b = 03) */
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	/* Coin Mode 2 - US (0x03ffff.b = 02) and World (0x03ffff.b = 03)
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_4C ) )
	*/
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )		/* "Shot2" in "test mode" */
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0002, "Easy" )
	PORT_DIPSETTING(      0x0003, "Normal" )
	PORT_DIPSETTING(      0x0001, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0010, "2" )
	PORT_DIPSETTING(      0x0030, "3" )
	PORT_DIPSETTING(      0x0020, "4" )
	PORT_DIPSETTING(      0x0000, "5" )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )	/* demo sounds? - see notes */
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )		/* "Shot2" in "test mode" */
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( fantasia )
	PORT_START
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0002, "Easy" )
	PORT_DIPSETTING(      0x0003, "Normal" )
	PORT_DIPSETTING(      0x0001, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )	/* freeze/vblank? - code at 0x000734 ('fantasia') */
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )		/* or 0x00075a ('newfant') - not called ? */
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0010, "2" )
	PORT_DIPSETTING(      0x0030, "3" )
	PORT_DIPSETTING(      0x0020, "4" )
	PORT_DIPSETTING(      0x0000, "5" )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )	/* demo sounds? - same "trap    #$d" as in 'galpanic' */
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )		/* "Shot2" in "test mode" */
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )	/* flip screen? - code at 0x00021c */
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0004, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0008, 0x0008, "Coin Mode" )
	PORT_DIPSETTING(      0x0008, "Mode 1" )
	PORT_DIPSETTING(      0x0000, "Mode 2" )
	/* Coin Mode 1 */
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	/* Coin Mode 2
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_4C ) )
	*/
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )		/* "Shot2" in "test mode" */
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_TILT )		/* MAME may crash when pressed (see notes) */
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )		/* "Service" in "test mode" */
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

/* Same as 'fantasia', but no "Service Mode" Dip Switch (and thus no "hidden" buttons) */
INPUT_PORTS_START( missw96 )
	PORT_START
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0002, "Easy" )
	PORT_DIPSETTING(      0x0003, "Normal" )
	PORT_DIPSETTING(      0x0001, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )	/* freeze/vblank? - code at 0x00074e - not called ? */
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0010, "2" )
	PORT_DIPSETTING(      0x0030, "3" )
	PORT_DIPSETTING(      0x0020, "4" )
	PORT_DIPSETTING(      0x0000, "5" )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )	/* demo sounds? - same "trap    #$d" as in 'galpanic' */
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )	/* flip screen? - code at 0x00021c */
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Coin Mode" )
	PORT_DIPSETTING(      0x0008, "Mode 1" )
	PORT_DIPSETTING(      0x0000, "Mode 2" )
	/* Coin Mode 1 */
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	/* Coin Mode 2
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_4C ) )
	*/
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_TILT )		/* MAME may crash when pressed (see notes) */
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( galhustl )
	PORT_START
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0000, "6" )
	PORT_DIPSETTING(      0x0001, "7" )
	PORT_DIPSETTING(      0x0003, "8" )
	PORT_DIPSETTING(      0x0002, "10" )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( On ) )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x0018, 0x0018, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0010, "Easy" )			/* 5000 - 7000 */
	PORT_DIPSETTING(      0x0018, "Normal" )		/* 4000 - 6000 */
	PORT_DIPSETTING(      0x0008, "Hard" )			/* 6000 - 8000 */
	PORT_DIPSETTING(      0x0000, "Hardest" )		/* 7000 - 9000 */
	PORT_DIPNAME( 0x0060, 0x0060, "Play Time" )
	PORT_DIPSETTING(      0x0040, "120 Sec" )
	PORT_DIPSETTING(      0x0060, "100 Sec" )
	PORT_DIPSETTING(      0x0020, "80 Sec" )
	PORT_DIPSETTING(      0x0000, "70 Sec" )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4,
			64*4, 65*4, 66*4, 67*4, 68*4, 69*4, 70*4, 71*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			16*32, 17*32, 18*32, 19*32, 20*32, 21*32, 22*32, 23*32 },
	128*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &spritelayout,  256, 16 },
	{ -1 } /* end of array */
};



static struct OKIM6295interface okim6295_interface =
{
	1,                  /* 1 chip */
	{ 12000 },          /* 12000Hz frequency */
	{ REGION_SOUND1 },  /* memory region */
	{ 100 }
};

static MACHINE_INIT( galpanib )
{
	/* init watchdog */
	watchdog_reset_r(0);
}

static MACHINE_DRIVER_START( galpanic )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M68000, 8000000)
	MDRV_CPU_MEMORY(galpanic_readmem,galpanic_writemem)
	MDRV_CPU_VBLANK_INT(galpanic_interrupt,2)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)	/* frames per second, vblank duration */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 256-1, 0, 224-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024 + 32768)
	MDRV_COLORTABLE_LENGTH(1024)

	MDRV_PALETTE_INIT(galpanic)
	MDRV_VIDEO_START(generic_bitmapped)
	MDRV_VIDEO_UPDATE(galpanic)

	/* sound hardware */
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( galpanib )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(galpanic)
	MDRV_CPU_REPLACE("main", M68000, 10000000)
	MDRV_CPU_MEMORY(galpanib_readmem,galpanib_writemem)

	/* arm watchdog */
	MDRV_MACHINE_INIT(galpanib)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( comad )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(galpanic)
	MDRV_CPU_REPLACE("main", M68000, 10000000)
	MDRV_CPU_MEMORY(comad_readmem,comad_writemem)

	/* video hardware */
	MDRV_VIDEO_UPDATE(comad)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( fantsia2 )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(comad)
	MDRV_CPU_REPLACE("main", M68000, 12000000)	/* ? */
	MDRV_CPU_MEMORY(fantsia2_readmem,fantsia2_writemem)

	/* video hardware */
	MDRV_VIDEO_UPDATE(comad)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( galhustl )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(comad)
	MDRV_CPU_REPLACE("main", M68000, 12000000)	/* ? */
	MDRV_CPU_MEMORY(galhustl_readmem,galhustl_writemem)
	MDRV_CPU_VBLANK_INT(galhustl_interrupt,3)

	/* video hardware */
	MDRV_VIDEO_UPDATE(comad)
MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( galpanic )
	ROM_REGION( 0x400000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "pm110.4m2",    0x000000, 0x80000, CRC(ae6b17a8) SHA1(f3a625eef45cc85cdf9760f77ea7ce93387911f9) )
	ROM_LOAD16_BYTE( "pm109.4m1",    0x000001, 0x80000, CRC(b85d792d) SHA1(0ed78e15f6e58285ce6944200b023ada1e673b0e) )
	/* The above two ROMs contain valid 68000 code, but the game doesn't */
	/* work. I think there might be a protection (addressed at e00000). */
	/* The two following ROMs replace the code with a working version. */
	ROM_LOAD16_BYTE( "pm112.6",      0x000000, 0x20000, CRC(7b972b58) SHA1(a7f619fca665b15f4f004ae739f5776ee2d4d432) )
	ROM_LOAD16_BYTE( "pm111.5",      0x000001, 0x20000, CRC(4eb7298d) SHA1(8858a40ffefbe4ecea7d5b70311c3775b7d987eb) )
	ROM_LOAD16_BYTE( "pm004e.8",     0x100001, 0x80000, CRC(d3af52bc) SHA1(46be057106388578defecab1cdd1793ec76ebe92) )
	ROM_LOAD16_BYTE( "pm005e.7",     0x100000, 0x80000, CRC(d7ec650c) SHA1(6c2250c74381497154bf516e0cf1db6bb56bb446) )
	ROM_LOAD16_BYTE( "pm000e.15",    0x200001, 0x80000, CRC(5d220f3f) SHA1(7ff373e01027c8832712f7a2d732f8e49b875878) )
	ROM_LOAD16_BYTE( "pm001e.14",    0x200000, 0x80000, CRC(90433eb1) SHA1(8688a85747ad9ecac395d782f130baa64fb9d12b) )
	ROM_LOAD16_BYTE( "pm002e.17",    0x300001, 0x80000, CRC(713ee898) SHA1(c9f608a57fb90e5ee15eb76a74a7afcc406d5b4e) )
	ROM_LOAD16_BYTE( "pm003e.16",    0x300000, 0x80000, CRC(6bb060fd) SHA1(4fc3946866c5a55e8340b62b5ad9beae723ce0da) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD( "pm006e.67",    0x000000, 0x100000, CRC(57aec037) SHA1(e6ba095b6892d4dcd76ba3343a97dd98ae29dc24) )

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	/* 00000-2ffff is fixed, 30000-3ffff is bank switched from all the ROMs */
	ROM_LOAD( "pm008e.l",     0x00000, 0x80000, CRC(d9379ba8) SHA1(5ae7c743319b1a12f2b101a9f0f8fe0728ed1476) )
	ROM_RELOAD(               0x40000, 0x80000 )
	ROM_LOAD( "pm007e.u",     0xc0000, 0x80000, CRC(c7ed7950) SHA1(133258b058d3c562208d0d00b9fac71202647c32) )
ROM_END

ROM_START( galpanib )
	ROM_REGION( 0x400000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "pm110.4m2",    0x000000, 0x80000, CRC(ae6b17a8) SHA1(f3a625eef45cc85cdf9760f77ea7ce93387911f9) )
	ROM_LOAD16_BYTE( "pm109.4m1",    0x000001, 0x80000, CRC(b85d792d) SHA1(0ed78e15f6e58285ce6944200b023ada1e673b0e) )
	ROM_LOAD16_BYTE( "pm004e.8",     0x100001, 0x80000, CRC(d3af52bc) SHA1(46be057106388578defecab1cdd1793ec76ebe92) )
	ROM_LOAD16_BYTE( "pm005e.7",     0x100000, 0x80000, CRC(d7ec650c) SHA1(6c2250c74381497154bf516e0cf1db6bb56bb446) )
	ROM_LOAD16_BYTE( "pm000e.15",    0x200001, 0x80000, CRC(5d220f3f) SHA1(7ff373e01027c8832712f7a2d732f8e49b875878) )
	ROM_LOAD16_BYTE( "pm001e.14",    0x200000, 0x80000, CRC(90433eb1) SHA1(8688a85747ad9ecac395d782f130baa64fb9d12b) )
	ROM_LOAD16_BYTE( "pm002e.17",    0x300001, 0x80000, CRC(713ee898) SHA1(c9f608a57fb90e5ee15eb76a74a7afcc406d5b4e) )
	ROM_LOAD16_BYTE( "pm003e.16",    0x300000, 0x80000, CRC(6bb060fd) SHA1(4fc3946866c5a55e8340b62b5ad9beae723ce0da) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD( "pm006e.67",    0x000000, 0x100000, CRC(57aec037) SHA1(e6ba095b6892d4dcd76ba3343a97dd98ae29dc24) )

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	/* 00000-2ffff is fixed, 30000-3ffff is bank switched from all the ROMs */
	ROM_LOAD( "pm008e.l",     0x00000, 0x80000, CRC(d9379ba8) SHA1(5ae7c743319b1a12f2b101a9f0f8fe0728ed1476) )
	ROM_RELOAD(               0x40000, 0x80000 )
	ROM_LOAD( "pm007e.u",     0xc0000, 0x80000, CRC(c7ed7950) SHA1(133258b058d3c562208d0d00b9fac71202647c32) )
ROM_END

ROM_START( fantasia )
	ROM_REGION( 0x500000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "prog2_16.rom", 0x000000, 0x80000, CRC(e27c6c57) SHA1(420b66928c46e76fa2496f221691dd6c34542287) )
	ROM_LOAD16_BYTE( "prog1_13.rom", 0x000001, 0x80000, CRC(68d27413) SHA1(84cb7d6523325496469d621f6f4da1b719162147) )
	ROM_LOAD16_BYTE( "iscr6_09.rom", 0x100000, 0x80000, CRC(2a588393) SHA1(ef66ed94dd40a95a9b0fb5c3b075c1f654f60927) )
	ROM_LOAD16_BYTE( "iscr5_05.rom", 0x100001, 0x80000, CRC(6160e0f0) SHA1(faec9d082c9039885afa4560aa87c05e9ecb5217) )
	ROM_LOAD16_BYTE( "iscr4_08.rom", 0x200000, 0x80000, CRC(f776b743) SHA1(bd4d666ede454a56181e109745ac4b3203b2a87c) )
	ROM_LOAD16_BYTE( "iscr3_04.rom", 0x200001, 0x80000, CRC(5df0dff2) SHA1(62ebd3c79f2e8ab30d6862cc4bf80f1b56f1f572) )
	ROM_LOAD16_BYTE( "iscr2_07.rom", 0x300000, 0x80000, CRC(5707d861) SHA1(33f1cff693dfcb04edbf8738d3ea2a1884e6ff0c) )
	ROM_LOAD16_BYTE( "iscr1_03.rom", 0x300001, 0x80000, CRC(36cb811a) SHA1(403cef012990b0e01b481b8afc6b5811e7137833) )
	ROM_LOAD16_BYTE( "imag2_10.rom", 0x400000, 0x80000, CRC(1f14a395) SHA1(12ca5a5a30963ecf90f5a006029aa1098b9ee1df) )
	ROM_LOAD16_BYTE( "imag1_06.rom", 0x400001, 0x80000, CRC(faf870e4) SHA1(163a9aa3e5c550d3760d32e31048a7aa1f93db7f) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD( "obj1_17.rom",  0x00000, 0x80000, CRC(aadb6eb7) SHA1(6eaa994ad7b4e8341360eaf5ddb46240316b7274) )

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	/* 00000-2ffff is fixed, 30000-3ffff is bank switched from all the ROMs */
	ROM_LOAD( "mus-1_01.rom", 0x00000, 0x80000, CRC(22955efb) SHA1(791c18d1aa0c10810da05c199108f51f99fe1d49) )
	ROM_RELOAD(               0x40000, 0x80000 )
	ROM_LOAD( "mus-2_02.rom", 0xc0000, 0x80000, CRC(4cd4d6c3) SHA1(a617472a810aef6d82f5fe75ef2980c03c21c2fa) )

	ROM_REGION16_BE( 0x200000, REGION_USER1, 0 )	/* unknown */
	ROM_LOAD16_BYTE( "gscr2_15.rom", 0x000000, 0x80000, CRC(46666768) SHA1(7281c4b45f6f9f6ad89fa2bb3f67f30433c0c513) )
	ROM_LOAD16_BYTE( "gscr1_12.rom", 0x000001, 0x80000, CRC(4bd25be6) SHA1(9834f081c0390ccaa1234efd2393b6495e946c64) )
	ROM_LOAD16_BYTE( "gscr4_14.rom", 0x100000, 0x80000, CRC(4e7e6ed4) SHA1(3e9e942e3de398edc8ac9f82769c3f41708d3741) )
	ROM_LOAD16_BYTE( "gscr3_11.rom", 0x100001, 0x80000, CRC(6d00a4c5) SHA1(8fc0d78200b82ab87658d364ebe2f2e7239722e7) )
ROM_END

ROM_START( fantsy95 )
	ROM_REGION( 0x500000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "prog2.12",  0x000000, 0x80000, CRC(1e684da7) SHA1(2104a6fb5f019011009f4faa769afcada90cff97) )
	ROM_LOAD16_BYTE( "prog1.7",   0x000001, 0x80000, CRC(dc4e4f6b) SHA1(9934121692a6d32164bef03c72c25dc727438e54) )
	ROM_LOAD16_BYTE( "i-scr2.10", 0x100000, 0x80000, CRC(ab8756ff) SHA1(0a7aa977151962e67b15a7e0f819b1412ff8dbdc) )
	ROM_LOAD16_BYTE( "i-scr1.5",  0x100001, 0x80000, CRC(d8e2ef77) SHA1(ec2c1dcc13e281288b5df43fa7a0b3cdf7357459) )
	ROM_LOAD16_BYTE( "i-scr4.9",  0x200000, 0x80000, CRC(4e52eb23) SHA1(be61c0dc68c49ded2dc6e8852fd92acac4986700) )
	ROM_LOAD16_BYTE( "i-scr3.4",  0x200001, 0x80000, CRC(797731f8) SHA1(571f939a7f85bd5b75a0660621961b531f44f736) )
	ROM_LOAD16_BYTE( "i-scr6.8",  0x300000, 0x80000, CRC(6f8e5239) SHA1(a1c2ec79e80906ca18cf3532ce38a1495ab37e44) )
	ROM_LOAD16_BYTE( "i-scr5.3",  0x300001, 0x80000, CRC(85420e3f) SHA1(d29e81cb1a33dca6232e14a0df2e21c8de45ba71) )
	ROM_LOAD16_BYTE( "i-scr8.11", 0x400000, 0x80000, CRC(33db8177) SHA1(9e9aa890dfa20e5aa6f1caec7d018d992217c2fe) )
	ROM_LOAD16_BYTE( "i-scr7.6",  0x400001, 0x80000, CRC(8662dd01) SHA1(a349c1cd965d3d51c20178fcce2f61ae76f4006a) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD( "obj1.13",  0x00000, 0x80000, CRC(832cd451) SHA1(29dfab1d4b7a15f3fe9fbedef41d405a40235a77) )

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	/* 00000-2ffff is fixed, 30000-3ffff is bank switched from all the ROMs */
	ROM_LOAD( "musc1.1", 0x00000, 0x80000, CRC(3117e2ef) SHA1(6581a7104556d44f814c537bbd74998922927034) )
	ROM_RELOAD(               0x40000, 0x80000 )
	ROM_LOAD( "musc2.2", 0xc0000, 0x80000, CRC(0c1109f9) SHA1(0e4ea534a32b1649e2e9bb8af7254b917ec03a90) )
ROM_END

ROM_START( newfant )
	ROM_REGION( 0x500000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "prog2_12.rom", 0x000000, 0x80000, CRC(de43a457) SHA1(91db13f63b46146131c58e775119ea3b073ca409) )
	ROM_LOAD16_BYTE( "prog1_07.rom", 0x000001, 0x80000, CRC(370b45be) SHA1(775873df9d3af803dbd1a392a45cad5f37b1b1c7) )
	ROM_LOAD16_BYTE( "iscr2_10.rom", 0x100000, 0x80000, CRC(4f2da2eb) SHA1(4f0b72327d1bdfad24d822953f45218bfae29cff) )
	ROM_LOAD16_BYTE( "iscr1_05.rom", 0x100001, 0x80000, CRC(63c6894f) SHA1(213544da570a167f3411357308c576805f6882f3) )
	ROM_LOAD16_BYTE( "iscr4_09.rom", 0x200000, 0x80000, CRC(725741ec) SHA1(3455cf0aed9653c66b8b2f905ad683687d517419) )
	ROM_LOAD16_BYTE( "iscr3_04.rom", 0x200001, 0x80000, CRC(51d6b362) SHA1(bcd57643ac9d79c150714ec6b6a2bb8a24acf7a5) )
	ROM_LOAD16_BYTE( "iscr6_08.rom", 0x300000, 0x80000, CRC(178b2ef3) SHA1(d3c092a282278968a319e06731481570f217d404) )
	ROM_LOAD16_BYTE( "iscr5_03.rom", 0x300001, 0x80000, CRC(d2b5c5fa) SHA1(80fde69bc5f4e958b5d57a5179b6e601192780f4) )
	ROM_LOAD16_BYTE( "iscr8_11.rom", 0x400000, 0x80000, CRC(f4148528) SHA1(4e27fff0b7ead068a159b3ed80c5793a6166fc4e) )
	ROM_LOAD16_BYTE( "iscr7_06.rom", 0x400001, 0x80000, CRC(2dee0c31) SHA1(1097006e6e5d16b24fb71615b6c0754fe0ecbe33) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD( "obj1_13.rom",  0x00000, 0x80000, CRC(e6d1bc71) SHA1(df0b6c1742c01991196659bab2691230323e7b8d) )

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	/* 00000-2ffff is fixed, 30000-3ffff is bank switched from all the ROMs */
	ROM_LOAD( "musc1_01.rom", 0x00000, 0x80000, CRC(10347fce) SHA1(f5fbe8ef363fe18b7104be5d2fa92943d1a5d7a2) )
	ROM_RELOAD(               0x40000, 0x80000 )
	ROM_LOAD( "musc2_02.rom", 0xc0000, 0x80000, CRC(b9646a8c) SHA1(e9432261ac86e4251a2c97301c6d014c05110a9c) )
ROM_END

ROM_START( missw96 )
	ROM_REGION( 0x500000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "mw96_10.bin",  0x000000, 0x80000, CRC(b1309bb1) SHA1(3cc7a903cb007d8fc0f836a33780c1c9231d1629) )
	ROM_LOAD16_BYTE( "mw96_06.bin",  0x000001, 0x80000, CRC(a5892bb3) SHA1(99130eb0af307fe66c9668414475e003f9c7d969) )
	ROM_LOAD16_BYTE( "mw96_09.bin",  0x100000, 0x80000, CRC(7032dfdf) SHA1(53728b60d0c772f6d936be47e21b069d0a75a2b4) )
	ROM_LOAD16_BYTE( "mw96_05.bin",  0x100001, 0x80000, CRC(91de5ab5) SHA1(d1153fa4745830d0fdd5bb311c38bf098ea29deb) )
	ROM_LOAD16_BYTE( "mw96_08.bin",  0x200000, 0x80000, CRC(b8e66fb5) SHA1(8abc6f8d85e0ad6acbf518e11fd81debc5a90957) )
	ROM_LOAD16_BYTE( "mw96_04.bin",  0x200001, 0x80000, CRC(e77a04f8) SHA1(e0043ec1d1bd5415c05ae93c9d785fc70174cb54) )
	ROM_LOAD16_BYTE( "mw96_07.bin",  0x300000, 0x80000, CRC(26112ed3) SHA1(f49f92a4d1bcea322b171702591315950fbd70c6) )
	ROM_LOAD16_BYTE( "mw96_03.bin",  0x300001, 0x80000, CRC(e9374a46) SHA1(eabfcc7cb9c9a2f932abc8103c3abfa8360dcbb5) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD( "mw96_11.bin",  0x00000, 0x80000, CRC(3983152f) SHA1(6308e936ba54e88b34253f1d4fbd44725e9d88ae) )

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	/* 00000-2ffff is fixed, 30000-3ffff is bank switched from all the ROMs */
	ROM_LOAD( "mw96_01.bin",  0x00000, 0x80000, CRC(e78a659e) SHA1(d209184c70e0d7e6d17034c6f536535cda782d42) )
	ROM_RELOAD(               0x40000, 0x80000 )
	ROM_LOAD( "mw96_02.bin",  0xc0000, 0x80000, CRC(60fa0c00) SHA1(391aa31e61663cc083a8a2320ba48a9859f3fd4e) )
ROM_END

ROM_START( fantsia2 )
	ROM_REGION( 0x500000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "prog2.g17",    0x000000, 0x80000, CRC(57c59972) SHA1(4b1da928b537cf340a67026d07bc3dfc078b0d0f) )
	ROM_LOAD16_BYTE( "prog1.f17",    0x000001, 0x80000, CRC(bf2d9a26) SHA1(92f0c1bd32f1e5e0ede3ba847242a212dfae4986) )
	ROM_LOAD16_BYTE( "scr2.g16",     0x100000, 0x80000, CRC(887b1bc5) SHA1(b6fcdc8a56ea25758f363224d256e9b6c8e30244) )
	ROM_LOAD16_BYTE( "scr1.f16",     0x100001, 0x80000, CRC(cbba3182) SHA1(a484819940fa1ef18ce679465c31075798748bac) )
	ROM_LOAD16_BYTE( "scr4.g15",     0x200000, 0x80000, CRC(ce97e411) SHA1(be0ed41362db03f384229c708f2ba4146e5cb501) )
	ROM_LOAD16_BYTE( "scr3.f15",     0x200001, 0x80000, CRC(480cc2e8) SHA1(38fe57ba1e34537f8be65fcc023ccd43369a5d94) )
	ROM_LOAD16_BYTE( "scr6.g14",     0x300000, 0x80000, CRC(b29d49de) SHA1(854b76755acf58fb8a4648a0ce72ea6bdf26c555) )
	ROM_LOAD16_BYTE( "scr5.f14",     0x300001, 0x80000, CRC(d5f88b83) SHA1(518a1f6732149f2851bbedca61f7313c39beb91b) )
	ROM_LOAD16_BYTE( "scr8.g20",     0x400000, 0x80000, CRC(694ae2b3) SHA1(82b7a565290fce07c8393af4718fd1e6136928e9) )
	ROM_LOAD16_BYTE( "scr7.f20",     0x400001, 0x80000, CRC(6068712c) SHA1(80a136d76dca566772e34d832ac11b8c7d6ce9ab) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD( "obj1.1i",      0x00000, 0x80000, CRC(52e6872a) SHA1(7e5274b9a415ee0e536cd3b87f73d3eae9644669) )
	ROM_LOAD( "obj2.2i",      0x80000, 0x80000, CRC(ea6e3861) SHA1(463b40f5441231a0451571a0b8afe1ed0fd4b164) )

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	/* 00000-2ffff is fixed, 30000-3ffff is bank switched from all the ROMs */
	ROM_LOAD( "music2.1b",    0x00000, 0x80000, CRC(23cc4f9c) SHA1(06b5342c25de966ce590917c571e5b19af1fef7d) )
	ROM_RELOAD(               0x40000, 0x80000 )
	ROM_LOAD( "music1.1a",    0xc0000, 0x80000, CRC(864167c2) SHA1(c454b26b6dea993e6bd64546f92beef05e46d7d7) )
ROM_END

ROM_START( galhustl )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "ue17.3", 0x00000, 0x80000, CRC(b2583dbb) SHA1(536f4aa2246ec816c4f270f9d42acc090718ee8b) )
	ROM_LOAD16_BYTE( "ud17.4", 0x00001, 0x80000, CRC(470a3668) SHA1(ad86e96ab8f1f5da23fb1feaabfb9c757965418e) )

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	/* 00000-2ffff is fixed, 30000-3ffff is bank switched from all the ROMs */
	ROM_LOAD( "galhstl1.ub6", 0x00000, 0x80000,  CRC(23848790) SHA1(2e77fbe04f46e258daecb4c5917e383c7c06a306) )
	ROM_RELOAD(               0x40000, 0x80000 )
	ROM_LOAD( "galhstl2.uc6", 0xc0000, 0x80000,  CRC(2168e54a) SHA1(87534334b16d3ddc3daefcb1b8086aff44157ccf) )

	ROM_REGION( 0x100000, REGION_GFX1, 0 )
	ROM_LOAD( "galhstl5.u5", 0x00000, 0x80000, CRC(44a18f15) SHA1(1217cf7fbbb442358b15016099efeface5dcbd22) )
ROM_END

GAMEX( 1990, galpanic, 0,        galpanic, galpanic, 0, ROT90, "Kaneko", "Gals Panic (set 1)", GAME_NO_COCKTAIL )
GAMEX( 1990, galpanib, galpanic, galpanib, galpanib, 0, ROT90, "Kaneko", "Gals Panic (set 2)", GAME_NO_COCKTAIL )
GAMEX( 1994, fantasia, 0,        comad,    fantasia, 0, ROT90, "Comad and New Japan System", "Fantasia", GAME_NO_COCKTAIL )
GAMEX( 1995, newfant,  0,        comad,    fantasia, 0, ROT90, "Comad and New Japan System", "New Fantasia", GAME_NO_COCKTAIL )
GAMEX( 1995, fantsy95, 0,        comad,    fantasia, 0, ROT90, "Hi-max Technology Inc.", "Fantasy '95", GAME_NO_COCKTAIL )
GAMEX( 1996, missw96,  0,        comad,    missw96,  0, ROT0,  "Comad", "Miss World '96 Nude", GAME_NO_COCKTAIL )
GAMEX( 1997, fantsia2, 0,        fantsia2, missw96,  0, ROT0,  "Comad", "Fantasia II", GAME_NO_COCKTAIL )
GAME(  1997, galhustl, 0,        galhustl, galhustl, 0, ROT0,  "ACE International", "Gals Hustler" )
