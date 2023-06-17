/***************************************************************************

XOR WORLD (c) 1990 Gaelco

Driver by Manuel Abadia <manu@teleline.es>

Memory Map:
-----------
0x000000-0x01ffff	ROM
0x200000-0x200001	Input #1
0x400000-0x400001	Input #2
0x600000-0x600001	DIPSW #1 + EEPROM read
0x800000-0x800001	Philips SAA1099P write register
0x800002-0x800003	Philips SAA1099P control register
0xa00008-0xa0000d	EEPROM write/ctrl
0xffc000-0xffc7ff	Screen		(8x8 tiles	32x32		(256x256))
0xffc800-0xffc87f	Sprite RAM
0xffc884-0xffffff	Work RAM

Interrupts:
	Level 2 INT updates the timer
	Level 6 INT drives the game

Unmapped addresses in the driver:

0xffc800-0xffc801	INT 2 ACK\Watchdog timer
0xffc802-0xffc803	INT 6 ACK/Watchdog timer

EEPROM chip: 93C46

***************************************************************************/

#include "driver.h"
#include "machine/eeprom.h"
#include "vidhrdw/generic.h"
#include "cpu/m68000/m68000.h"

extern data16_t *xorworld_videoram;
extern data16_t *xorworld_spriteram;

/* from vidhrdw/xorworld.c */
READ16_HANDLER(xorworld_vram_r);
WRITE16_HANDLER(xorworld_vram_w);
VIDEO_START( xorworld );
VIDEO_UPDATE( xorworld );
PALETTE_INIT( xorworld );

/****************************************************************
				NVRAM load/save/init
****************************************************************/
static NVRAM_HANDLER( xorworld )
{
	if (read_or_write){
		EEPROM_save(file);
	} else {
		EEPROM_init(&eeprom_interface_93C46);

		if (file) {
			EEPROM_load(file);
		}
	}
}

/****************************************************************
				Init machine
****************************************************************/

#define PATCH(data) *rom = data; rom++

/*	patch some strange protection (without this, strange characters appear
	after level 5 and some pieces don't rotate properly some times) */
static DRIVER_INIT( xorworld )
{
	data16_t *rom = (data16_t *)(memory_region(REGION_CPU1) + 0x1390);

	PATCH(0x4239); PATCH(0x00ff); PATCH(0xe196);	/* clr.b $ffe196 */
	PATCH(0x4239); PATCH(0x00ff); PATCH(0xe197);	/* clr.b $ffe197 */
	PATCH(0x4239); PATCH(0x00ff); PATCH(0xe0bc);	/* clr.b $ffe0bc */
	PATCH(0x41f9); PATCH(0x00ff); PATCH(0xcfce);	/* lea $ffcfce,A0 */
	PATCH(0x3e3c); PATCH(0x000f);					/* move #$f,D7 */
	PATCH(0x4218);									/* clr.b (A0)+ */
	PATCH(0x51cf); PATCH(0xfffc);					/* dbra D7,$13ac */
	PATCH(0x4e75);									/* rts */

	PATCH(0x31ff);									/* adjust checksum */
}


/****************************************************************
				EEPROM read/write/control
****************************************************************/

/* the EEPROM is read thru bit 4 */
static READ16_HANDLER( xorworld_input_r )
{
	return readinputport(0) | ((EEPROM_read_bit() & 0x01) << 4);
	return rand() & 0x10;
}

static WRITE16_HANDLER( eeprom_cs_w )
{
	/* bit 0 is CS (active low) */
	EEPROM_set_cs_line((data & 0x01) ? CLEAR_LINE : ASSERT_LINE);
}

static WRITE16_HANDLER( eeprom_sk_w )
{
	/* bit 0 is SK (active high) */
	EEPROM_set_clock_line((data & 0x01) ? ASSERT_LINE : CLEAR_LINE);
}

static WRITE16_HANDLER( eeprom_data_w )
{
	/* bit 0 is EEPROM data (DIN) */
	EEPROM_write_bit(data & 0x01);
}


static INTERRUPT_GEN( xorworld_interrupt )
{
	if (cpu_getiloops() == 0){		
		cpu_set_irq_line(0, 2, HOLD_LINE);
	}
	else if (cpu_getiloops() % 2){
		cpu_set_irq_line(0, 6, HOLD_LINE);
	}
}

static MEMORY_READ16_START( xorworld_readmem )
	{ 0x000000, 0x01ffff, MRA16_ROM },						/* ROM */
	{ 0x200000, 0x200001, input_port_1_word_r },			/* INPUT #1 */
	{ 0x400000, 0x400001, input_port_2_word_r },			/* INPUT #2 */
	{ 0x600000, 0x600001, xorworld_input_r },				/* DIPSW #1 + EEPROM data */
	{ 0xffc000, 0xffc7ff, xorworld_vram_r },				/* Video RAM */
	{ 0xffc800, 0xffc87f, MRA16_RAM },						/* Sprite RAM */
	{ 0xffc884, 0xffffff, MRA16_RAM },						/* Work RAM */
	{ -1 }
};

static MEMORY_WRITE16_START( xorworld_writemem )
	{ 0x000000, 0x01ffff, MWA16_ROM },							/* ROM */
	{ 0x800000, 0x800001, saa1099_write_port_0_lsb_w },			/* SAA1099 write port */
	{ 0x800002, 0x800003, saa1099_control_port_0_lsb_w },		/* SAA1099 control port */
	{ 0xa00008, 0xa00009, eeprom_cs_w },						/* EEPROM chip select */
	{ 0xa0000a, 0xa0000b, eeprom_sk_w },						/* EEPROM serial clock */
	{ 0xa0000c, 0xa0000d, eeprom_data_w },						/* EEPROM data */
	{ 0xffc000, 0xffc7ff, xorworld_vram_w, &xorworld_videoram },/* Video RAM */
	{ 0xffc800, 0xffc87f, MWA16_RAM, &xorworld_spriteram },		/* Sprite RAM */
	{ 0xffc880, 0xffc883, MWA16_NOP },							/* INTs ACKs? */
	{ 0xffc884, 0xffffff, MWA16_RAM },							/* Work RAM */
	{ -1 }
};

static struct GfxLayout xorworld_tilelayout =
{
	8,8,															/* 8x8 tiles */
	0x10000/16,														/* 4096 tiles */
	4,																/* 4 bpp */
	{ 1*0x10000*8, 1*0x10000*8+4, 0, 4 },							/* plane offsets */
	{ 0*8, 0*8+1, 0*8+2, 0*8+3, 1*8+0, 1*8+1, 1*8+2, 1*8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8
};


static struct GfxLayout xorworld_spritelayout =
{
	16,16,																/* 16x16 sprites */
	0x10000/64,															/* 1024 sprites */
	4,																	/* 4 bpp */
	{ 1*0x10000*8, 1*0x10000*8+4, 0, 4 },								/* plane offsets */
	{ 0*8, 0*8+1, 0*8+2, 0*8+3, 1*8+0, 1*8+1, 1*8+2, 1*8+3,
		32*8+0, 32*8+1, 32*8+2, 32*8+3, 33*8+0, 33*8+1, 33*8+2, 33*8+3},
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
		8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8
};

static struct GfxDecodeInfo xorworld_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x000000, &xorworld_tilelayout,	0,64 },
	{ REGION_GFX1, 0x000000, &xorworld_spritelayout,0,64 },
	{ -1 }
};


INPUT_PORTS_START( xorworld )

PORT_START	/* DSW #1 */
	PORT_DIPNAME( 0x07, 0x07, "Coin A & B" )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_2C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )	
	/* 0x10 is used for accessing the NVRAM */
	PORT_DIPNAME( 0x60, 0x40, DEF_STR( Difficulty ) )	
	PORT_DIPSETTING(    0x40, "Easy" )
	PORT_DIPSETTING(    0x60, "Normal" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

PORT_START	/* 1P INPUTS & COINSW */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

PORT_START	/* 2P INPUTS & STARTSW */
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START2 )
INPUT_PORTS_END

static struct SAA1099_interface	xorworld_saa1099_interface = 
{
	1,					/* number of chips */
	{ { 100, 100 } }	/* volume (left, right) */
};


static MACHINE_DRIVER_START( xorworld )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000)	/* 10 MHz */
	MDRV_CPU_MEMORY(xorworld_readmem, xorworld_writemem)
	MDRV_CPU_VBLANK_INT(xorworld_interrupt, 4) /* 1 IRQ2 + 1 IRQ4 + 1 IRQ6 */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_NVRAM_HANDLER(xorworld)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(xorworld_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)
	MDRV_COLORTABLE_LENGTH(64*4)
	
	MDRV_PALETTE_INIT(xorworld)
	MDRV_VIDEO_START(xorworld)
	MDRV_VIDEO_UPDATE(xorworld)

	/* sound hardware */
	MDRV_SOUND_ADD(SAA1099, xorworld_saa1099_interface)
MACHINE_DRIVER_END


ROM_START( xorworld )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "c13.bin", 0x000000, 0x010000, CRC(615a864d) SHA1(db07eef19d26a4daa0bcc17ac24d237483f93bf6) )
	ROM_LOAD16_BYTE( "b13.bin", 0x000001, 0x010000, CRC(632e8ee5) SHA1(ec53e632c762f72ad1fe3fab85111bdcc1e818ae) )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "d9.bin",	0x000000, 0x010000, CRC(da8d4d65) SHA1(41bcc15f26066bd820b44c0f258e70d0102953c9) )
	ROM_LOAD( "d10.bin",	0x010000, 0x010000, CRC(3b1d6f24) SHA1(bedf60a4cbf20492b8a846b6a7b578f8fe8dbde9) )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "b4.bin",   0x0000, 0x0100, CRC(75e468af) SHA1(b5fd1a086c27ca2e837cbbf1b7e57dfdd369b0d0) )  /* Red palette ROM (4 bits) */	
	ROM_LOAD( "b7.bin",   0x0100, 0x0100, CRC(7e1cd146) SHA1(fd26a28f90c50ffcb0fe7718820c81eb9fe79e66) )  /* Green palette ROM (4 bits) */
	ROM_LOAD( "b5.bin",   0x0200, 0x0100, CRC(c1b9d9f9) SHA1(c4b02bf60db449fb308a5eb3e41c43299ad8e3e3) )  /* Blue palette ROM (4 bits) */
ROM_END



GAME( 1990, xorworld, 0, xorworld, xorworld, xorworld, ROT0, "Gaelco", "Xor World (prototype)" )
