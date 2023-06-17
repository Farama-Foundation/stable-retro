/****************************************************************************

	Gunbuster  							(c) 1992 Taito

	Driver by Bryan McPhail & David Graves.

	Board Info:

		CPU   : 68EC020 68000
		SOUND : Ensoniq
		OSC.  : 40.000MHz 16.000MHz 30.47618MHz

		* This board (K11J0717A) uses following chips:
		  - TC0470LIN
		  - TC0480SCP
		  - TC0570SPC
		  - TC0260DAR
		  - TC0510NIO

	Gunbuster uses a slightly enhanced sprite system from the one
	in Taito Z games.

	The key feature remains the use of a sprite map rom which allows
	the sprite hardware to create many large zoomed sprites on screen
	while minimizing the main cpu load.

	This feature makes the SZ system complementary to the F3 system
	which, owing to its F2 sprite hardware, is not very well suited to
	3d games. (Taito abandoned the SZ system once better 3d hardware
	platforms were available in the mid 1990s.)

	Gunbuster also uses the TC0480SCP tilemap chip (like the last Taito
	Z game, Double Axle).

	Todo:

		FLIPX support in taitoic.c is not quite correct - the Taito logo is wrong,
		and the floor in the Doom levels has horizontal scrolling where it shouldn't.

		No networked machine support

		Coin lockout not working (see gunbustr_input_w): perhaps this
		was a prototype version without proper coin handling?

***************************************************************************/

#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/taitoic.h"
#include "sndhrdw/taitosnd.h"
#include "machine/eeprom.h"

VIDEO_START( gunbustr );
VIDEO_UPDATE( gunbustr );

static UINT16 coin_word;
static data32_t *gunbustr_ram;
extern data32_t *f3_shared_ram;

/* F3 sound */
READ16_HANDLER(f3_68000_share_r);
WRITE16_HANDLER(f3_68000_share_w);
READ16_HANDLER(f3_68681_r);
WRITE16_HANDLER(f3_68681_w);
READ16_HANDLER(es5510_dsp_r);
WRITE16_HANDLER(es5510_dsp_w);
WRITE16_HANDLER(f3_volume_w);
WRITE16_HANDLER(f3_es5505_bank_w);
void f3_68681_reset(void);
extern data32_t *f3_shared_ram;

/*********************************************************************/

static void gunbustr_interrupt5(int x)
{
	cpu_set_irq_line(0,5,HOLD_LINE);
}

static INTERRUPT_GEN( gunbustr_interrupt )
{
	timer_set(TIME_IN_CYCLES(200000-500,0),0, gunbustr_interrupt5);
	cpu_set_irq_line(0, 4, HOLD_LINE);
}

static WRITE32_HANDLER( gunbustr_palette_w )
{
	int a,r,g,b;
	COMBINE_DATA(&paletteram32[offset]);

	a = paletteram32[offset] >> 16;
	r = (a &0x7c00) >> 10;
	g = (a &0x03e0) >> 5;
	b = (a &0x001f);
	r = (r << 3) | (r >> 2);
	g = (g << 3) | (g >> 2);
	b = (b << 3) | (b >> 2);
	palette_set_color(offset*2,r,g,b);

	a = paletteram32[offset] &0xffff;
	r = (a &0x7c00) >> 10;
	g = (a &0x03e0) >> 5;
	b = (a &0x001f);
	r = (r << 3) | (r >> 2);
	g = (g << 3) | (g >> 2);
	b = (b << 3) | (b >> 2);
	palette_set_color(offset*2+1,r,g,b);
}

static READ32_HANDLER( gunbustr_input_r )
{
	switch (offset)
	{
		case 0x00:
		{
			return (input_port_0_word_r(0,0) << 16) | input_port_1_word_r(0,0) |
				  (EEPROM_read_bit() << 7);
		}

		case 0x01:
		{
			return input_port_2_word_r(0,0) | (coin_word << 16);
		}
 	}
log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #0 PC %06x: read input %06x\n",activecpu_get_pc(),offset);

	return 0x0;
}

static WRITE32_HANDLER( gunbustr_input_w )
{

#if 0
{
char t[64];
static data32_t mem[2];
COMBINE_DATA(&mem[offset]);

sprintf(t,"%08x %08x",mem[0],mem[1]);
usrintf_showmessage(t);
}
#endif

	switch (offset)
	{
		case 0x00:
		{
			if (ACCESSING_MSB32)	/* $400000 is watchdog */
			{
				watchdog_reset_w(0,data >> 24);
			}

			if (ACCESSING_LSB32)
			{
				EEPROM_set_clock_line((data & 0x20) ? ASSERT_LINE : CLEAR_LINE);
				EEPROM_write_bit(data & 0x40);
				EEPROM_set_cs_line((data & 0x10) ? CLEAR_LINE : ASSERT_LINE);
				return;
			}
			return;
		}

		case 0x01:
		{
			if (ACCESSING_MSB32)
			{
				/* game does not write a separate counter for coin 2!
				   It should disable both coins when 9 credits reached
				   see code $1d8a-f6... but for some reason it's not */
				coin_lockout_w(0, data & 0x01000000);
				coin_lockout_w(1, data & 0x02000000);
				coin_counter_w(0, data & 0x04000000);
				coin_counter_w(1, data & 0x04000000);
				coin_word = (data >> 16) &0xffff;
			}
/*logerror("CPU #0 PC %06x: write input %06x\n",activecpu_get_pc(),offset);*/
		}
	}
}

static WRITE32_HANDLER( motor_control_w )
{
/*
	Standard value poked into MSW is 0x3c00
	(0x2000 and zero are written at startup)

	Three bits are written in test mode to test
	lamps and motors:

	......x. ........   Hit motor
	.......x ........   Solenoid
	........ .....x..   Hit lamp
*/
}


static READ32_HANDLER( gunbustr_gun_r )
{
	return ( input_port_3_word_r(0,0) << 24) | (input_port_4_word_r(0,0) << 16) |
		 ( input_port_5_word_r(0,0) << 8)  |  input_port_6_word_r(0,0);
}

static WRITE32_HANDLER( gunbustr_gun_w )
{
	/* 10000 cycle delay is arbitrary */
	timer_set(TIME_IN_CYCLES(10000,0),0, gunbustr_interrupt5);
}


/***********************************************************
			 MEMORY STRUCTURES
***********************************************************/

static MEMORY_READ32_START( gunbustr_readmem )
	{ 0x000000, 0x0fffff, MRA32_ROM },
	{ 0x200000, 0x21ffff, MRA32_RAM },	/* main CPUA ram */
	{ 0x300000, 0x301fff, MRA32_RAM },	/* Sprite ram */
	{ 0x390000, 0x3907ff, MRA32_RAM },	/* Sound shared ram */
	{ 0x400000, 0x400007, gunbustr_input_r },
	{ 0x500000, 0x500003, gunbustr_gun_r },	/* gun coord read */
	{ 0x800000, 0x80ffff, TC0480SCP_long_r },
	{ 0x830000, 0x83002f, TC0480SCP_ctrl_long_r },
	{ 0x900000, 0x901fff, MRA32_RAM },	/* Palette ram */
	{ 0xc00000, 0xc03fff, MRA32_RAM },	/* network ram ?? */
MEMORY_END

static MEMORY_WRITE32_START( gunbustr_writemem )
	{ 0x000000, 0x0fffff, MWA32_ROM },
	{ 0x200000, 0x21ffff, MWA32_RAM, &gunbustr_ram },
	{ 0x300000, 0x301fff, MWA32_RAM, &spriteram32, &spriteram_size },
	{ 0x380000, 0x380003, motor_control_w },	/* motor, lamps etc. */
	{ 0x390000, 0x3907ff, MWA32_RAM, &f3_shared_ram },
	{ 0x400000, 0x400007, gunbustr_input_w },	/* eerom etc. */
	{ 0x500000, 0x500003, gunbustr_gun_w },	/* gun int request */
	{ 0x800000, 0x80ffff, TC0480SCP_long_w },
	{ 0x830000, 0x83002f, TC0480SCP_ctrl_long_w },
	{ 0x900000, 0x901fff, gunbustr_palette_w, &paletteram32 },
	{ 0xc00000, 0xc03fff, MWA32_RAM},	/* network ram ?? */
MEMORY_END

/******************************************************************************/

static MEMORY_READ16_START( sound_readmem )
	{ 0x000000, 0x03ffff, MRA16_RAM },
	{ 0x140000, 0x140fff, f3_68000_share_r },
	{ 0x200000, 0x20001f, ES5505_data_0_r },
	{ 0x260000, 0x2601ff, es5510_dsp_r },
	{ 0x280000, 0x28001f, f3_68681_r },
	{ 0xc00000, 0xcfffff, MRA16_BANK1 },
	{ 0xff8000, 0xffffff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( sound_writemem )
	{ 0x000000, 0x03ffff, MWA16_RAM },
	{ 0x140000, 0x140fff, f3_68000_share_w },
	{ 0x200000, 0x20001f, ES5505_data_0_w },
	{ 0x260000, 0x2601ff, es5510_dsp_w },
	{ 0x280000, 0x28001f, f3_68681_w },
	{ 0x300000, 0x30003f, f3_es5505_bank_w },
	{ 0x340000, 0x340003, f3_volume_w }, /* 8 channel volume control */
	{ 0xc00000, 0xcfffff, MWA16_ROM },
	{ 0xff8000, 0xffffff, MWA16_RAM },
MEMORY_END

/***********************************************************
			 INPUT PORTS (dips in eprom)
***********************************************************/

INPUT_PORTS_START( gunbustr )
	PORT_START      /* IN0 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER1 )	/* Freeze input */
	PORT_BIT( 0x0002, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* reserved for EEROM */
	PORT_BIT( 0x0100, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_PLAYER2 )

	PORT_START      /* IN2 */
	PORT_BITX(0x0001, IP_ACTIVE_LOW,  IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW,  IPT_COIN2 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW,  IPT_START2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	/* Light gun inputs */

	PORT_START	/* IN 3, P1X */
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_X | IPF_PLAYER1, 30, 20, 0, 0xff)

	PORT_START	/* IN 4, P1Y */
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_Y | IPF_REVERSE | IPF_PLAYER1, 30, 20, 0, 0xff)

	PORT_START	/* IN 5, P2X */
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_X | IPF_PLAYER2, 30, 20, 0, 0xff)

	PORT_START	/* IN 6, P2Y */
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_Y | IPF_REVERSE | IPF_PLAYER2, 30, 20, 0, 0xff)
INPUT_PORTS_END


/***********************************************************
				GFX DECODING
**********************************************************/

static struct GfxLayout tile16x16_layout =
{
	16,16,	/* 16*16 sprites */
	RGN_FRAC(1,1),
	4,	/* 4 bits per pixel */
	{ 0, 8, 16, 24 },
	{ 32, 33, 34, 35, 36, 37, 38, 39, 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*64, 1*64,  2*64,  3*64,  4*64,  5*64,  6*64,  7*64,
	  8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	64*16	/* every sprite takes 128 consecutive bytes */
};

static struct GfxLayout charlayout =
{
	16,16,    /* 16*16 characters */
	RGN_FRAC(1,1),
	4,        /* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 1*4, 0*4, 5*4, 4*4, 3*4, 2*4, 7*4, 6*4, 9*4, 8*4, 13*4, 12*4, 11*4, 10*4, 15*4, 14*4 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64, 8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	128*8     /* every sprite takes 128 consecutive bytes */
};

static struct GfxDecodeInfo gunbustr_gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0x0, &tile16x16_layout,  0, 512 },
	{ REGION_GFX1, 0x0, &charlayout,        0, 512 },
	{ -1 } /* end of array */
};


/***********************************************************
			     MACHINE DRIVERS
***********************************************************/

static MACHINE_INIT( gunbustr )
{
	/* Sound cpu program loads to 0xc00000 so we use a bank */
	data16_t *RAM = (data16_t *)memory_region(REGION_CPU2);
	cpu_setbank(1,&RAM[0x80000]);

	RAM[0]=RAM[0x80000]; /* Stack and Reset vectors */
	RAM[1]=RAM[0x80001];
	RAM[2]=RAM[0x80002];
	RAM[3]=RAM[0x80003];

	f3_68681_reset();
}

static struct ES5505interface es5505_interface =
{
	1,					/* total number of chips */
	{ 30476100/2 },		/* freq */
	{ REGION_SOUND1 },	/* Bank 0: Unused by F3 games? */
	{ REGION_SOUND1 },	/* Bank 1: All games seem to use this */
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) },		/* master volume */
	{ 0 }				/* irq callback */
};

static data8_t default_eeprom[128]={
	0x00,0x01,0x00,0x85,0x00,0xfd,0x00,0xff,0x00,0x67,0x00,0x02,0x00,0x00,0x00,0x7b,
	0x00,0xff,0x00,0xff,0x00,0x78,0x00,0x03,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0x01,0x01,0x00,0x00,0x01,0x02,0x00,0x00,0x01,0x01,0x00,0x00,0x00,0x10,0x00,0x00,
	0x21,0x13,0x14,0x00,0x00,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff
};

static struct EEPROM_interface gunbustr_eeprom_interface =
{
	6,				/* address bits */
	16,				/* data bits */
	"0110",			/* read command */
	"0101",			/* write command */
	"0111",			/* erase command */
	"0100000000",	/* unlock command */
	"0100110000",	/* lock command */
};

static NVRAM_HANDLER( gunbustr )
{
	if (read_or_write)
		EEPROM_save(file);
	else {
		EEPROM_init(&gunbustr_eeprom_interface);
		if (file)
			EEPROM_load(file);
		else
			EEPROM_set_data(default_eeprom,128);  /* Default the gun setup values */
	}
}

static MACHINE_DRIVER_START( gunbustr )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68EC020, 16000000)	/* 16 MHz */
	MDRV_CPU_MEMORY(gunbustr_readmem,gunbustr_writemem)
	MDRV_CPU_VBLANK_INT(gunbustr_interrupt,1) /* VBL */

	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 16 MHz */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(gunbustr)
	MDRV_NVRAM_HANDLER(gunbustr)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0, 40*8-1, 2*8, 32*8-1)
	MDRV_GFXDECODE(gunbustr_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(8192)

	MDRV_VIDEO_START(gunbustr)
	MDRV_VIDEO_UPDATE(gunbustr)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(ES5505, es5505_interface)
MACHINE_DRIVER_END

/***************************************************************************/

ROM_START( gunbustr )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )	/* 1024K for 68020 code (CPU A) */
	ROM_LOAD32_BYTE( "d27-23.bin", 0x00000, 0x40000, CRC(cd1037cc) SHA1(8005a6a84081ce609e7a605ec8e00e740bfc6846) )
	ROM_LOAD32_BYTE( "d27-22.bin", 0x00001, 0x40000, CRC(475949fc) SHA1(3d5aa3411d2618004902f9d05dff61d9af01ff35) )
	ROM_LOAD32_BYTE( "d27-21.bin", 0x00002, 0x40000, CRC(60950a8a) SHA1(a0336bf6970baa6eaa998a112db840a7fd0452d7) )
	ROM_LOAD32_BYTE( "d27-20.bin", 0x00003, 0x40000, CRC(13735c60) SHA1(65b762b28d51b295f6fe190420af566b1b3d4a82) )

	ROM_REGION( 0x140000, REGION_CPU2, 0 )	/* Sound cpu */
	ROM_LOAD16_BYTE( "d27-25.bin", 0x100000, 0x20000, CRC(c88203cf) SHA1(a918d395b471acdce56dacabd7a1e1e023948365) )
	ROM_LOAD16_BYTE( "d27-24.bin", 0x100001, 0x20000, CRC(084bd8bd) SHA1(93229bc7de4550ead1bb12f666ddbacbe357488d) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "d27-01.bin", 0x00000, 0x80000, CRC(f41759ce) SHA1(30789f43dd09b56399e1dfdb8c6a1e01a21562bd) )	/* SCR 16x16 tiles */
	ROM_LOAD16_BYTE( "d27-02.bin", 0x00001, 0x80000, CRC(92ab6430) SHA1(28ed80391c732b09d10c74ed6b78ac76cb62e083) )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "d27-04.bin", 0x000003, 0x100000, CRC(ff8b9234) SHA1(6095b7daf9b7e9a22b0d44d9d6a642ddecb2bd29) )	/* OBJ 16x16 tiles: each rom has 1 bitplane */
	ROM_LOAD32_BYTE( "d27-05.bin", 0x000002, 0x100000, CRC(96d7c1a5) SHA1(93b6a7aea397280a5a778e736d433a85cb7da52c) )
	ROM_LOAD32_BYTE( "d27-06.bin", 0x000001, 0x100000, CRC(bbb934db) SHA1(9e9b5cf05b9275f1182f5b499b8ee897c4f25b96) )
	ROM_LOAD32_BYTE( "d27-07.bin", 0x000000, 0x100000, CRC(8ab4854e) SHA1(bd2750cdaa2918e56f8aef3732875952a1eeafea) )

	ROM_REGION16_LE( 0x80000, REGION_USER1, 0 )
	ROM_LOAD16_WORD( "d27-03.bin", 0x00000, 0x80000, CRC(23bf2000) SHA1(49b29e771a47fcd7e6cd4e2704b217f9727f8299) )	/* STY, used to create big sprites on the fly */

	ROM_REGION16_BE( 0xa00000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE( "d27-08.bin", 0x000000, 0x100000, CRC(7c147e30) SHA1(b605045154967050ec06391798da4afe3686a6e1) )
	/* 0x200000 Empty bank? */
	ROM_LOAD16_BYTE( "d27-09.bin", 0x400000, 0x100000, CRC(3e060304) SHA1(c4da4a94c168c3a454409d758c3ed45babbab170) )
	ROM_LOAD16_BYTE( "d27-10.bin", 0x600000, 0x100000, CRC(ed894fe1) SHA1(5bf2fb6abdcf25bc525a2c3b29dbf7aca0b18fea) )
ROM_END

static READ32_HANDLER( main_cycle_r )
{
	if (activecpu_get_pc()==0x55a && (gunbustr_ram[0x3acc/4]&0xff000000)==0)
		cpu_spinuntil_int();

	return gunbustr_ram[0x3acc/4];
}

static DRIVER_INIT( gunbustr )
{
	/* Speedup handler */
	install_mem_read32_handler(0, 0x203acc, 0x203acf, main_cycle_r);
}

GAME( 1992, gunbustr, 0,      gunbustr, gunbustr, gunbustr, ORIENTATION_FLIP_X, "Taito Corporation", "Gunbuster (Japan)" )
