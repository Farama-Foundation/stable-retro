/*
	Polygonet Commanders (Konami, 1993)

	Preliminary driver by R. Belmont

	This is Konami's first 3D game!

	Hardware:
	68EC020 @ 16 MHz
	Motorola XC56156-40 DSP @ 40 MHz (needs core)
	Z80 + K054539 for sound
	Network to connect up to 4 PCBs.

	Video hardware:
	TTL text plane similar to Run and Gun.
	Konami K054009(x2) + K054010(x2) (polygon rasterizers)
	Konami K053936 "PSAC2" (3d roz plane, used for backgrounds)

	Driver includes:
	- 68020 memory map
	- Z80 + sound system
	- EEPROM
	- service switch
	- TTL text plane

	Driver needs:
	- 56156 DSP core
	- Polygon rasterization (K054009 + K054010)
	- Hook up PSAC2 (gfx decode for it is already present and correct)
	- Palettes
	- Controls
	- Priorities.  From the original board it appears they're fixed, in front to back order:
	  (all the way in front) TTL text layer -> polygons -> PSAC2 (all the way in back)

Notes:

 506000 is the DSP control
 506004 is DSP status (in bit 0, where 0 = not ready, 1 = ready)
 50600C and 50600E are the DSP comms ports (read/write)

*/

#include "driver.h"
#include "state.h"
#include "cpuintrf.h"

#include "vidhrdw/generic.h"
#include "vidhrdw/konamiic.h"
#include "cpu/m68000/m68000.h"
#include "cpu/z80/z80.h"
#include "sound/k054539.h"
#include "machine/eeprom.h"

VIDEO_START(polygonet_vh_start);
VIDEO_UPDATE(polygonet_vh_screenrefresh);

READ32_HANDLER( polygonet_ttl_ram_r );
WRITE32_HANDLER( polygonet_ttl_ram_w );

static int init_eeprom_count;
static data32_t *dsp_shared_ram;

static struct EEPROM_interface eeprom_interface =
{
	7,			/* address bits */
	8,			/* data bits */
	"011000",		/*  read command */
	"010100",		/* write command */
	"0100100000000",/* erase command */
	"0100000000000",/* lock command */
	"0100110000000" /* unlock command */
};

static NVRAM_HANDLER(nvram_handler)
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
			init_eeprom_count = 10;
	}
}

static READ32_HANDLER( polygonet_eeprom_r )
{
	if (ACCESSING_LSW32)
	{
		return 0x0200 | (EEPROM_read_bit()<<8);
	}
	else
	{
		return (readinputport(0)<<24);
	}

	log_cb(RETRO_LOG_DEBUG, LOGPRE "unk access to eeprom port (mask %x)\n", mem_mask);
	return 0;
}


static WRITE32_HANDLER( polygonet_eeprom_w )
{
	if (ACCESSING_MSB32)
	{
		EEPROM_write_bit((data & 0x01000000) ? ASSERT_LINE : CLEAR_LINE);
		EEPROM_set_cs_line((data & 0x02000000) ? CLEAR_LINE : ASSERT_LINE);
		EEPROM_set_clock_line((data & 0x04000000) ? ASSERT_LINE : CLEAR_LINE);
		return;
	}

	log_cb(RETRO_LOG_DEBUG, LOGPRE "unknown write %x (mask %x) to eeprom\n", data, mem_mask);
}

/* TTL tile readback for ROM test */
static READ32_HANDLER( ttl_rom_r )
{
	data32_t *ROM;

	ROM = (data32_t *)memory_region(REGION_GFX1);

	return ROM[offset];
}

/* PSAC2 tile readback for ROM test */
static READ32_HANDLER( psac_rom_r )
{
	data32_t *ROM;

	ROM = (data32_t *)memory_region(REGION_GFX2);

	return ROM[offset];
}

/* irqs 3, 5, and 7 have valid vectors*/
/* irq 3 does ??? (vblank?)*/
/* irq 5 does ??? (polygon end of draw?)*/
/* irq 7 does nothing (it jsrs to a rts and then rte)*/

static INTERRUPT_GEN(polygonet_interrupt)
{
	if (cpu_getiloops())
		cpu_set_irq_line(0, MC68000_IRQ_5, HOLD_LINE);
	else
		cpu_set_irq_line(0, MC68000_IRQ_3, HOLD_LINE);
}

/* sound CPU communications */

static READ32_HANDLER( sound_r )
{
	int latch = soundlatch3_r(0);

	if (latch == 0xe) latch = 0xf;	/* hack: until 54539 NMI disable found*/

	return latch<<8;
}

static WRITE32_HANDLER( sound_w )
{
	if (ACCESSING_MSB)
	{
		soundlatch_w(0, (data>>8)&0xff);
	}
	else
	{
		soundlatch2_w(0, data&0xff);
	}
}

static WRITE32_HANDLER( sound_irq_w )
{
	cpu_set_irq_line(1, 0, HOLD_LINE);
}

/* hack DSP communications, just enough to pass the ram/rom test */

static int dsp_state;

/* DSP communications */
static READ32_HANDLER( dsp_r )
{
	static int dsp_states[] =
	{
		0, 1, 0, 1, 1,
	};

	log_cb(RETRO_LOG_DEBUG, LOGPRE "dsp state %d at PC=%x\n", dsp_state, activecpu_get_pc());

	return dsp_states[dsp_state]<<24;
}

static WRITE32_HANDLER( dsp_2_w )
{
	if (mem_mask == 0xffffff)
	{
		if (data>>24 == 5)
		{
			dsp_state = 2;
			log_cb(RETRO_LOG_DEBUG, LOGPRE "entering state 2\n");
		}
	}
}

static WRITE32_HANDLER( dsp_w )
{
	if (mem_mask == 0xffffff)
	{
		/* write to 6000*/
		if (data>>24 == 8)
		{
			dsp_state = 0;
			log_cb(RETRO_LOG_DEBUG, LOGPRE "entering state 0: uploaded program wakeup\n");
		}
	}
	else
	{
		/* write to 6002*/
		if (data>>8 == 0x97)
		{
			unsigned char *RAM = (unsigned char *)dsp_shared_ram;
			int i;
			unsigned short write;

			dsp_state = 1;
			log_cb(RETRO_LOG_DEBUG, LOGPRE "entering state 1: shared RAM test #1\n");

			write = 0xfff0;
			for (i = 0; i < 0x2000/2; i++)
			{
				RAM[i*4+3] = (write>>8)&0xff;
				RAM[i*4+2] = (write&0xff);
				write--;
				RAM[i*4+1] = (write>>8)&0xff;
				RAM[i*4]   = (write&0xff);
				write--;
			}
		}

		if (data>>8 == 0x98)
		{
			unsigned char *RAM = (unsigned char *)dsp_shared_ram;
			int i;
			unsigned short write, track;

			dsp_state = 1;
			log_cb(RETRO_LOG_DEBUG, LOGPRE "entering state 3: shared RAM test #2\n");

			track = 0xfff0;
			for (i = 0; i < 0x2000/2; i++)
			{
				write = track ^ 0xffff;
				RAM[i*4+3] = (write>>8)&0xff;
				RAM[i*4+2] = (write&0xff);
				track--;
				write = track ^ 0xffff;
				RAM[i*4+1] = (write>>8)&0xff;
				RAM[i*4]   = (write&0xff);
				track--;
			}
		}

		if (data>>8 == 0x99)
		{
			log_cb(RETRO_LOG_DEBUG, LOGPRE "entering state 4: DSP board RAM test\n");
			dsp_state = 4;
		}
	}
}

static MEMORY_READ32_START( polygonet_readmem )
	{ 0x000000, 0x1fffff, MRA32_ROM },
	{ 0x200000, 0x21ffff, MRA32_RAM },
	{ 0x440000, 0x440fff, MRA32_RAM },
	{ 0x480000, 0x480003, polygonet_eeprom_r },
	{ 0x500000, 0x503fff, MRA32_RAM },
	{ 0x506004, 0x506007, dsp_r },
	{ 0x540000, 0x540fff, polygonet_ttl_ram_r },
	{ 0x541000, 0x54101f, MRA32_RAM },
	{ 0x580000, 0x5807ff, MRA32_RAM },
/*	{ 0x580800, 0x580803, MRA32_RAM },	*/ /* network RAM / registers?*/
	{ 0x600008, 0x60000b, sound_r },
	{ 0x700000, 0x73ffff, psac_rom_r },
	{ 0x780000, 0x79ffff, ttl_rom_r },
	{ 0xff8000, 0xffffff, MRA32_RAM },
MEMORY_END

static MEMORY_WRITE32_START( polygonet_writemem )
	{ 0x000000, 0x1fffff, MWA32_ROM },
	{ 0x200000, 0x21ffff, MWA32_RAM },	/* PSAC2 tilemap*/
	{ 0x440000, 0x440fff, MWA32_RAM,},	/* PSAC2 lineram*/
	{ 0x4C0000, 0x4C0003, polygonet_eeprom_w },
	{ 0x500000, 0x503fff, MWA32_RAM, &dsp_shared_ram },
	{ 0x504000, 0x504003, dsp_2_w },
	{ 0x506000, 0x506003, dsp_w },
	{ 0x540000, 0x540fff, polygonet_ttl_ram_w },
	{ 0x541000, 0x54101f, MWA32_RAM },
	{ 0x580000, 0x5807ff, MWA32_RAM },	/* A21K*/
/*	{ 0x580800, 0x580803, MWA32_RAM },	*/ /* network RAM / registers?*/
	{ 0x600004, 0x600007, sound_w },
	{ 0x640000, 0x640003, sound_irq_w },
	{ 0x680000, 0x680003, watchdog_reset32_w },
	{ 0xff8000, 0xffffff, MWA32_RAM },
MEMORY_END

/**********************************************************************************/

static int cur_sound_region;

static void reset_sound_region(void)
{
	cpu_setbank(2, memory_region(REGION_CPU2) + 0x10000 + cur_sound_region*0x4000);
}

static WRITE_HANDLER( sound_bankswitch_w )
{
	cur_sound_region = (data & 0x1f);

	reset_sound_region();
}

static INTERRUPT_GEN(audio_interrupt)
{
	cpu_set_nmi_line(1, PULSE_LINE);
}

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK2 },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe22f, K054539_0_r },
	{ 0xe230, 0xe3ff, MRA_RAM },
	{ 0xe400, 0xe62f, K054539_1_r },
	{ 0xe630, 0xe7ff, MRA_RAM },
	{ 0xf002, 0xf002, soundlatch_r },
	{ 0xf003, 0xf003, soundlatch2_r },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0xbfff, MWA_NOP },
	{ 0xc000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe22f, K054539_0_w },
	{ 0xe230, 0xe3ff, MWA_RAM },
	{ 0xe400, 0xe62f, K054539_1_w },
	{ 0xe630, 0xe7ff, MWA_RAM },
	{ 0xf000, 0xf000, soundlatch3_w },
	{ 0xf800, 0xf800, sound_bankswitch_w },
	{ 0xfff1, 0xfff3, MWA_NOP },
MEMORY_END

static struct K054539interface k054539_interface =
{
	1,			/* 1 chip */
	48000,
	{ REGION_SOUND1 },
	{ { 100, 100 }, },
	{ NULL }
};

/**********************************************************************************/

static struct GfxLayout bglayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4, 8*4,
	  9*4, 10*4, 11*4, 12*4, 13*4, 14*4, 15*4 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
 	  8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	128*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0, &bglayout,     0x0000, 64 },
	{ -1 } /* end of array */
};

MACHINE_DRIVER_START( plygonet )
	MDRV_CPU_ADD(M68EC020, 16000000)	/* 16 MHz (xtal is 32.0 MHz) */
	MDRV_CPU_MEMORY(polygonet_readmem,polygonet_writemem)
	MDRV_CPU_VBLANK_INT(polygonet_interrupt, 2)

	MDRV_CPU_ADD(Z80, 8000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PERIODIC_INT(audio_interrupt, 480)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_NVRAM_HANDLER(nvram_handler)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0, 64*8-1, 0, 32*8-1 )
	MDRV_PALETTE_LENGTH(32768)

	MDRV_VIDEO_START(polygonet_vh_start)
	MDRV_VIDEO_UPDATE(polygonet_vh_screenrefresh)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(K054539, k054539_interface)
MACHINE_DRIVER_END

INPUT_PORTS_START( polygonet )
	PORT_START

	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN3)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START4)

	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* EEPROM data */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SPECIAL )	/* EEPROM ready (always 1) */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_DIPNAME(0x10, 0x00, "Monitors")
	PORT_DIPSETTING(0x00, "1 Monitor")
	PORT_DIPSETTING(0x10, "2 Monitors")
/*	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )*/
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )
INPUT_PORTS_END

static DRIVER_INIT(polygonet)
{
	/* set default bankswitch */
	cur_sound_region = 2;
	reset_sound_region();
}

ROM_START( plygonet )
	/* main program */
	ROM_REGION( 0x200000, REGION_CPU1, 0)
	ROM_LOAD32_BYTE( "305a01.bin", 0x000003, 512*1024, CRC(8bdb6c95) SHA1(e981833842f8fd89b9726901fbe2058444204792) )
	ROM_LOAD32_BYTE( "305a02.bin", 0x000002, 512*1024, CRC(4d7e32b3) SHA1(25731526535036972577637d186f02ae467296bd) )
	ROM_LOAD32_BYTE( "305a03.bin", 0x000001, 512*1024, CRC(36e4e3fe) SHA1(e8fcad4f196c9b225a0fbe70791493ff07c648a9) )
	ROM_LOAD32_BYTE( "305a04.bin", 0x000000, 512*1024, CRC(d8394e72) SHA1(eb6bcf8aedb9ba5843204ab8aacb735cbaafb74d) )

	/* Z80 sound program */
	ROM_REGION( 0x30000, REGION_CPU2, 0 )
	ROM_LOAD("305b05.bin", 0x000000, 0x20000, CRC(2d3d9654) SHA1(784a409df47cee877e507b8bbd3610d161d63753) )
	ROM_RELOAD( 0x10000, 0x20000)

	/* TTL text plane tiles */
	ROM_REGION( 0x20000, REGION_GFX1, 0 )
	ROM_LOAD( "305b06.bin", 0x000000, 0x20000, CRC(decd6e42) SHA1(4c23dcb1d68132d3381007096e014ee4b6007086) )

	/* '936 tiles */
 	ROM_REGION( 0x40000, REGION_GFX2, 0 )
	ROM_LOAD( "305b07.bin", 0x000000, 0x40000, CRC(e4320bc3) SHA1(b0bb2dac40d42f97da94516d4ebe29b1c3d77c37) )

	/* sound data */
	ROM_REGION( 0x200000, REGION_SOUND1, 0 )
	ROM_LOAD( "305b08.bin", 0x000000, 0x200000, CRC(874607df) SHA1(763b44a80abfbc355bcb9be8bf44373254976019) )
ROM_END

/*          ROM        parent   machine    inp        init */
GAMEX( 1993, plygonet, 0,       plygonet, polygonet, polygonet, ROT90, "Konami", "Polygonet Commanders (ver UAA)", GAME_NOT_WORKING )
