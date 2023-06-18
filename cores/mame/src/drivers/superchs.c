/****************************************************************************

	Super Chase  						(c) 1992 Taito

	Driver by Bryan McPhail & David Graves.

	Board Info:

		CPU board:
		68000
		68020
		TC0570SPC (Taito custom)
		TC0470LIN (Taito custom)
		TC0510NIO (Taito custom)
		TC0480SCP (Taito custom)
		TC0650FDA (Taito custom)
		ADC0809CCN

		X2=26.686MHz
		X1=40MHz
		X3=32MHz

		Sound board:
		68000
		68681
		MB8421 (x2)
		MB87078
		Ensoniq 5510
		Ensoniq 5505

	(Acknowledgments and thanks to Richard Bush and the Raine team
	for their preliminary Super Chase driver.)

***************************************************************************/

#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/taitoic.h"
#include "sndhrdw/taitosnd.h"
#include "machine/eeprom.h"

VIDEO_START( superchs );
VIDEO_UPDATE( superchs );

static UINT16 coin_word;
static data32_t *superchs_ram;
static data32_t *shared_ram;
extern data32_t *f3_shared_ram;

static int steer=0;

/* from sndhrdw/taito_f3.c */
READ16_HANDLER(f3_68000_share_r);
WRITE16_HANDLER(f3_68000_share_w);
READ16_HANDLER(f3_68681_r);
WRITE16_HANDLER(f3_68681_w);
READ16_HANDLER(es5510_dsp_r);
WRITE16_HANDLER(es5510_dsp_w);
WRITE16_HANDLER(f3_volume_w);
WRITE16_HANDLER(f3_es5505_bank_w);
void f3_68681_reset(void);
extern WRITE16_HANDLER( es5505_bank_w ); /* drivers/f3 */

/*********************************************************************/

static READ16_HANDLER( shared_ram_r )
{
	if ((offset&1)==0) return (shared_ram[offset/2]&0xffff0000)>>16;
	return (shared_ram[offset/2]&0x0000ffff);
}

static WRITE16_HANDLER( shared_ram_w )
{
	if ((offset&1)==0) {
		if (ACCESSING_MSB)
			shared_ram[offset/2]=(shared_ram[offset/2]&0x00ffffff)|((data&0xff00)<<16);
		if (ACCESSING_LSB)
			shared_ram[offset/2]=(shared_ram[offset/2]&0xff00ffff)|((data&0x00ff)<<16);
	} else {
		if (ACCESSING_MSB)
			shared_ram[offset/2]=(shared_ram[offset/2]&0xffff00ff)|((data&0xff00)<< 0);
		if (ACCESSING_LSB)
			shared_ram[offset/2]=(shared_ram[offset/2]&0xffffff00)|((data&0x00ff)<< 0);
	}
}

static WRITE32_HANDLER( cpua_ctrl_w )
{
	/*
	CPUA writes 0x00, 22, 72, f2 in that order.
	f2 seems to be the standard in-game value.
	..x...x.
	.xxx..x.
	xxxx..x.
	is there an irq enable in the top nibble?
	*/

	if (ACCESSING_MSB)
	{
		cpu_set_reset_line(2,(data &0x200) ? CLEAR_LINE : ASSERT_LINE);
		if (data&0x8000) cpu_set_irq_line(0,3,HOLD_LINE); /* Guess */
	}

	if (ACCESSING_LSB32)
	{
		/* Lamp control bits of some sort in the lsb */
	}
}

static WRITE32_HANDLER( superchs_palette_w )
{
	int a,r,g,b;
	COMBINE_DATA(&paletteram32[offset]);

	a = paletteram32[offset];
	r = (a &0xff0000) >> 16;
	g = (a &0xff00) >> 8;
	b = (a &0xff);

	palette_set_color(offset,r,g,b);
}

static READ32_HANDLER( superchs_input_r )
{
	switch (offset)
	{
		case 0x00:
			return (input_port_0_word_r(0,0) << 16) | input_port_1_word_r(0,0) |
				  (EEPROM_read_bit() << 7);

		case 0x01:
			return coin_word<<16;
 	}

	return 0xffffffff;
}

static WRITE32_HANDLER( superchs_input_w )
{

	#if 0
	{
	char t[64];
	static data32_t mem[2];
	COMBINE_DATA(&mem[offset]);
	sprintf(t,"%08x %08x",mem[0],mem[1]);
	/*usrintf_showmessage(t);*/
	}
	#endif

	switch (offset)
	{
		case 0x00:
		{
			if (ACCESSING_MSB32)	/* $300000 is watchdog */
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

		/* there are 'vibration' control bits somewhere! */

		case 0x01:
		{
			if (ACCESSING_MSB32)
			{
				coin_lockout_w(0,~data & 0x01000000);
				coin_lockout_w(1,~data & 0x02000000);
				coin_counter_w(0, data & 0x04000000);
				coin_counter_w(1, data & 0x08000000);
				coin_word=(data >> 16) &0xffff;
			}
		}
	}
}

static READ32_HANDLER( superchs_stick_r )
{
	int fake = input_port_6_word_r(0,0);
	int accel;

	if (!(fake &0x10))	/* Analogue steer (the real control method) */
	{
		steer = input_port_2_word_r(0,0);
	}
	else	/* Digital steer, with smoothing - speed depends on how often stick_r is called */
	{
		int delta;
		int goal = 0x80;
		if (fake &0x04) goal = 0xff;		/* pressing left */
		if (fake &0x08) goal = 0x0;		/* pressing right */

		if (steer!=goal)
		{
			delta = goal - steer;
			if (steer < goal)
			{
				if (delta >2) delta = 2;
			}
			else
			{
				if (delta < (-2)) delta = -2;
			}
			steer += delta;
		}
	}

	/* Accelerator is an analogue input but the game treats it as digital (on/off) */
	if (input_port_6_word_r(0,0) & 0x1)	/* pressing B1 */
		accel = 0x0;
	else
		accel = 0xff;

	/* Todo: Verify brake - and figure out other input */
	return (steer << 24) | (accel << 16) | (input_port_4_word_r(0,0) << 8) | input_port_5_word_r(0,0);
}

static WRITE32_HANDLER( superchs_stick_w )
{
	/* This is guess work - the interrupts are in groups of 4, with each writing to a
		different byte in this long word before the RTE.  I assume all but the last
		(top) byte cause an IRQ with the final one being an ACK.  (Total guess but it works). */
	if (mem_mask!=0x00ffffff)
		cpu_set_irq_line(0,3,HOLD_LINE);
}

/***********************************************************
			 MEMORY STRUCTURES
***********************************************************/

static MEMORY_READ32_START( superchs_readmem )
	{ 0x000000, 0x0fffff, MRA32_ROM },
	{ 0x100000, 0x11ffff, MRA32_RAM },	/* main CPUA ram */
	{ 0x140000, 0x141fff, MRA32_RAM },	/* Sprite ram */
	{ 0x180000, 0x18ffff, TC0480SCP_long_r },
	{ 0x1b0000, 0x1b002f, TC0480SCP_ctrl_long_r },
	{ 0x200000, 0x20ffff, MRA32_RAM },	/* Shared ram */
	{ 0x280000, 0x287fff, MRA32_RAM },	/* Palette ram */
	{ 0x2c0000, 0x2c07ff, MRA32_RAM },	/* Sound shared ram */
	{ 0x300000, 0x300007, superchs_input_r },
	{ 0x340000, 0x340003, superchs_stick_r },	/* stick coord read */
MEMORY_END

static MEMORY_WRITE32_START( superchs_writemem )
	{ 0x000000, 0x0fffff, MWA32_ROM },
	{ 0x100000, 0x11ffff, MWA32_RAM, &superchs_ram },
	{ 0x140000, 0x141fff, MWA32_RAM, &spriteram32, &spriteram_size },
	{ 0x180000, 0x18ffff, TC0480SCP_long_w },
	{ 0x1b0000, 0x1b002f, TC0480SCP_ctrl_long_w },
	{ 0x200000, 0x20ffff, MWA32_RAM, &shared_ram },
	{ 0x240000, 0x240003, cpua_ctrl_w },
	{ 0x280000, 0x287fff, superchs_palette_w, &paletteram32 },
	{ 0x2c0000, 0x2c07ff, MWA32_RAM, &f3_shared_ram },
	{ 0x300000, 0x300007, superchs_input_w },	/* eerom etc. */
	{ 0x340000, 0x340003, superchs_stick_w },	/* stick int request */
MEMORY_END

static MEMORY_READ16_START( superchs_cpub_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x200000, 0x20ffff, MRA16_RAM },	/* local ram */
	{ 0x800000, 0x80ffff, shared_ram_r },
	{ 0xa00000, 0xa001ff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( superchs_cpub_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x200000, 0x20ffff, MWA16_RAM },
	{ 0x600000, 0x60ffff, TC0480SCP_word_w }, /* Only written upon errors */
	{ 0x800000, 0x80ffff, shared_ram_w },
	{ 0xa00000, 0xa001ff, MWA16_RAM },	/* Extra road control?? */
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

/***********************************************************/

INPUT_PORTS_START( superchs )
	PORT_START      /* IN0 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_BUTTON6 | IPF_PLAYER1 )	/* Freeze input */
	PORT_BITX(0x0010, IP_ACTIVE_LOW,  IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW,  IPT_COIN2 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* reserved for EEROM */
	PORT_BIT( 0x0100, IP_ACTIVE_LOW,  IPT_BUTTON5 | IPF_PLAYER1 )	/* seat center (cockpit only) */
	PORT_BIT( 0x0200, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BITX(0x1000, IP_ACTIVE_LOW,  IPT_BUTTON3, "Nitro", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x2000, IP_ACTIVE_LOW,  IPT_BUTTON4, "Gear Shift", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x4000, IP_ACTIVE_LOW,  IPT_BUTTON2, "Brake", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW,  IPT_START1 )

	PORT_START	/* IN 2, steering wheel */
	PORT_ANALOG( 0xff, 0x7f, IPT_AD_STICK_X | IPF_REVERSE | IPF_PLAYER1, 25, 15, 0, 0xff )

	PORT_START	/* IN 3, accel [effectively also brake for the upright] */
	PORT_ANALOG( 0xff, 0x00, IPT_AD_STICK_Y | IPF_PLAYER1, 20, 10, 0, 0xff)

	PORT_START	/* IN 4, sound volume */
	PORT_ANALOG( 0xff, 0x00, IPT_AD_STICK_X | IPF_REVERSE | IPF_PLAYER2, 20, 10, 0, 0xff)

	PORT_START	/* IN 5, unknown */
	PORT_ANALOG( 0xff, 0x00, IPT_AD_STICK_Y | IPF_PLAYER2, 20, 10, 0, 0xff)

	PORT_START	/* IN 6, inputs and DSW all fake */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_PLAYER1 )
	PORT_DIPNAME( 0x10, 0x00, "Steering type" )
	PORT_DIPSETTING(    0x10, "Digital" )
	PORT_DIPSETTING(    0x00, "Analogue" )
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

static struct GfxDecodeInfo superchs_gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0x0, &tile16x16_layout,  0, 512 },
	{ REGION_GFX1, 0x0, &charlayout,        0, 512 },
	{ -1 } /* end of array */
};


/***********************************************************
			     MACHINE DRIVERS
***********************************************************/

static MACHINE_INIT( superchs )
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
	{ 13343000 },		/* freq - 26.686MHz/2??  May be 16MHz but Nancy sounds too high-pitched */
	{ REGION_SOUND1 },	/* Bank 0: Unused by F3 games? */
	{ REGION_SOUND1 },	/* Bank 1: All games seem to use this */
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) },		/* master volume */
	{ 0 }				/* irq callback */
};

static struct EEPROM_interface superchs_eeprom_interface =
{
	6,				/* address bits */
	16,				/* data bits */
	"0110",			/* read command */
	"0101",			/* write command */
	"0111",			/* erase command */
	"0100000000",	/* unlock command */
	"0100110000",	/* lock command */
};

static data8_t default_eeprom[128]={
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x53,0x00,0x2e,0x00,0x43,0x00,0x00,
	0x00,0x01,0x00,0x01,0x00,0x01,0x00,0x01,0x00,0x01,0xff,0xff,0xff,0xff,0x00,0x01,
	0x00,0x01,0x00,0x01,0x00,0x00,0x00,0x01,0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x80,0xff,0xff,0xff,0xff,
	0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff
};

static NVRAM_HANDLER( superchs )
{
	if (read_or_write)
		EEPROM_save(file);
	else
	{
		EEPROM_init(&superchs_eeprom_interface);

		if (file)
			EEPROM_load(file);
		else
			EEPROM_set_data(default_eeprom,128);  /* Default the wheel setup values */
	}
}

static MACHINE_DRIVER_START( superchs )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68EC020, 16000000)	/* 16 MHz */
	MDRV_CPU_MEMORY(superchs_readmem,superchs_writemem)
	MDRV_CPU_VBLANK_INT(irq2_line_hold,1)/* VBL */

	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 16 MHz */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_CPU_ADD(M68000, 16000000)	/* 16 MHz */
	MDRV_CPU_MEMORY(superchs_cpub_readmem,superchs_cpub_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)/* VBL */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(8)	/* CPU slices - Need to interleave Cpu's 1 & 3 */

	MDRV_MACHINE_INIT(superchs)
	MDRV_NVRAM_HANDLER(superchs)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0, 40*8-1, 2*8, 32*8-1)
	MDRV_GFXDECODE(superchs_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(8192)

	MDRV_VIDEO_START(superchs)
	MDRV_VIDEO_UPDATE(superchs)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(ES5505, es5505_interface)
MACHINE_DRIVER_END

/***************************************************************************/

ROM_START( superchs )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )	/* 1024K for 68020 code (CPU A) */
	ROM_LOAD32_BYTE( "d46-35.27", 0x00000, 0x40000, CRC(1575c9a7) SHA1(e3441d6018ed3315c62c5e5c4534d8712b025ae2) )
	ROM_LOAD32_BYTE( "d46-34.25", 0x00001, 0x40000, CRC(c72a4d2b) SHA1(6ef64de15e52007406ce3255071a1f856e0e8b49) )
	ROM_LOAD32_BYTE( "d46-33.23", 0x00002, 0x40000, CRC(3094bcd0) SHA1(b6779b81a3ebec440a9359868dc43fc3a631ee11) )
	ROM_LOAD32_BYTE( "d46-31.21", 0x00003, 0x40000, CRC(38b983a3) SHA1(c4859cecc2f3506b7090c462cecd3e4eaabe85aa) )

	ROM_REGION( 0x140000, REGION_CPU2, 0 )	/* Sound cpu */
	ROM_LOAD16_BYTE( "d46-37.8up", 0x100000, 0x20000, CRC(60b51b91) SHA1(0d0b017808e0a3bdabe8ef5a726bbe16428db06b) )
	ROM_LOAD16_BYTE( "d46-36.7lo", 0x100001, 0x20000, CRC(8f7aa276) SHA1(b3e330e33099d3cbf4cdc43063119b041e9eea3a) )

	ROM_REGION( 0x40000, REGION_CPU3, 0 )	/* 256K for 68000 code (CPU B) */
	ROM_LOAD16_BYTE( "d46-24.127", 0x00000, 0x20000, CRC(a006baa1) SHA1(e691ddab6cb79444bd6c3fc870e0dff3051d8cf9) )
	ROM_LOAD16_BYTE( "d46-23.112", 0x00001, 0x20000, CRC(9a69dbd0) SHA1(13eca492f1db834c599656750864e7003514f3d4) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "d46-05.87", 0x00000, 0x100000, CRC(150d0e4c) SHA1(9240b32900be733b8f44868ed5d64f5f1aaadb47) )	/* SCR 16x16 tiles */
	ROM_LOAD16_BYTE( "d46-06.88", 0x00001, 0x100000, CRC(321308be) SHA1(17e724cce39b1331650c1f08d693d057dcd43a3f) )

	ROM_REGION( 0x800000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "d46-01.64", 0x000003, 0x200000, CRC(5c2ae92d) SHA1(bee2caed4729a27fa0569d952d6d12170c2aa2a8) )	/* OBJ 16x16 tiles: each rom has 1 bitplane */
	ROM_LOAD32_BYTE( "d46-02.65", 0x000002, 0x200000, CRC(a83ca82e) SHA1(03759be87a8d62c0044e8a44e90c47308e32d3e5) )
	ROM_LOAD32_BYTE( "d46-03.66", 0x000001, 0x200000, CRC(e0e9cbfd) SHA1(b7deb2c58320af9d1b4273ad2758ce927d2e279c) )
	ROM_LOAD32_BYTE( "d46-04.67", 0x000000, 0x200000, CRC(832769a9) SHA1(136ead19edeee90b5be91a6e2f434193dc670fd8) )

	ROM_REGION16_LE( 0x80000, REGION_USER1, 0 )
	ROM_LOAD16_WORD( "d46-07.34", 0x00000, 0x80000, CRC(c3b8b093) SHA1(f34364248ca7fdaaa1a0f8f6f795f9b4bc935fb9) )	/* STY, used to create big sprites on the fly */

	ROM_REGION16_BE( 0x1000000, REGION_SOUND1 , ROMREGION_SOUNDONLY | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE( "d46-10.2", 0xc00000, 0x200000, CRC(306256be) SHA1(e6e5d4a4c0b98470f2aff2e94624dd19af73ec5d) )
	ROM_LOAD16_BYTE( "d46-12.4", 0x000000, 0x200000, CRC(a24a53a8) SHA1(5d5fb87a94ceabda89360064d7d9b6d23c4c606b) )
	ROM_RELOAD     (             0x400000, 0x200000 )
	ROM_LOAD16_BYTE( "d46-11.5", 0x800000, 0x200000, CRC(d4ea0f56) SHA1(dc8d2ed3c11d0b6f9ebdfde805188884320235e6) )
ROM_END

static READ32_HANDLER( main_cycle_r )
{
	if (activecpu_get_pc()==0x702)
		cpu_spinuntil_int();

	return superchs_ram[0];
}

static READ16_HANDLER( sub_cycle_r )
{
	if (activecpu_get_pc()==0x454)
		cpu_spinuntil_int();

	return superchs_ram[2]&0xffff;
}

static DRIVER_INIT( superchs )
{
	/* Speedup handlers */
	install_mem_read32_handler(0, 0x100000, 0x100003, main_cycle_r);
	install_mem_read16_handler(2, 0x80000a, 0x80000b, sub_cycle_r);
}

GAME( 1992, superchs, 0, superchs, superchs, superchs, ROT0, "Taito America Corporation", "Super Chase - Criminal Termination (US)" )
