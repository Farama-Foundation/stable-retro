/***************************************************************************

	P&P Marketing Police Trainer hardware

	driver by Aaron Giles

	Games supported:
		* Police Trainer
		* Sharpshooter

	Known bugs:
		* perspective on the floor in some levels is not drawn correctly
		* flip screen not supported

Note:	Police Trainer v1.3B is on the same revision PCB as Sharpshooter - Rev 0.5B
		If you set the dip to serivce mode and reset the game, all program
		roms fail the checksum.  However, each checksum listed matches the
		checksum printed on the ROM label.
 

The ATTILA Video System PCB (by EXIT Entertainment):

Sharpshooter PCB is Rev 0.5B
Police Trainer PCB is Rev 0.3

               JAMMA Connector
GUN1   XILINX-1

GUN2

LED1 LED2                    93C66

                IDT71024 x 2  Bt481
   8-way DIP  AT001

U127                 U113       U162
U125  IDT71256 x 4   U112
U123                 U111       U160
U121                 U110
U126
U124  OSC    IDT79R3041    XILINX-2
U122  48.000MHz                 XILINX-3
U120                             BSMT2000

Chips:
  CPU: IDT 79R3041-25J (MIPS R3000 core)
Sound: BSMT2000
Other: Bt481AKPJ110 (44 Pin PQFP, Brooktree RAMDAC)
       AT001 (160 Pin PQFP, P & P Marketing Custom)
       ATMEL 93C66 (EEPROM)
PLDs:
       XILINX-1 Labeled as U175A (Rev 3: Not Used)
       XILINX-2 Labeled as U109A (Rev 3: Lattice - U109.P)
       XILINX-3 Labeled as U151A (Rev 3: Lattice - U151.P)


Note #1: On a Rev 3 PCB, the XILINX PLDs are replace with Lattice PLDs
Note #2: On a Rev 3 PCB there is a small daughter card to help with gun input
Note #3: Bt481A 256-Word Color Palette 15, 16 & 24-bit Color Power-Down RAMDAC

***************************************************************************/

#include "driver.h"
#include "cpu/mips/r3000.h"
#include "machine/eeprom.h"
#include "policetr.h"


/* constants */
#define MASTER_CLOCK	48000000


/* global variables */
data32_t *	policetr_rambase;


/* local variables */
static data32_t *rom_base;

static data32_t control_data;

static data32_t bsmt_reg;
static data32_t bsmt_data_bank;
static data32_t bsmt_data_offset;

static data32_t *speedup_data;
static UINT32 last_cycles;
static UINT32 loop_count;

static offs_t speedup_pc;



/*************************************
 *
 *	Interrupt handling
 *
 *************************************/

static void irq5_gen(int param)
{
	cpu_set_irq_line(0, R3000_IRQ5, ASSERT_LINE);
}


static INTERRUPT_GEN( irq4_gen )
{
	cpu_set_irq_line(0, R3000_IRQ4, ASSERT_LINE);
	timer_set(cpu_getscanlinetime(0), 0, irq5_gen);
}



/*************************************
 *
 *	Input ports
 *
 *************************************/

static READ32_HANDLER( port0_r )
{
	return readinputport(0) << 16;
}


static READ32_HANDLER( port1_r )
{
	return (readinputport(1) << 16) | (EEPROM_read_bit() << 29);
}


static READ32_HANDLER( port2_r )
{
	return readinputport(2) << 16;
}



/*************************************
 *
 *	Output ports
 *
 *************************************/

static WRITE32_HANDLER( control_w )
{
	/* bit $80000000 = BSMT access/ROM read*/
	/* bit $20000000 = toggled every 64 IRQ4's*/
	/* bit $10000000 = ????*/
	/* bit $00800000 = EEPROM data*/
	/* bit $00400000 = EEPROM clock*/
	/* bit $00200000 = EEPROM enable (on 1)*/

	COMBINE_DATA(&control_data);

	/* handle EEPROM I/O */
	if (!(mem_mask & 0x00ff0000))
	{
		EEPROM_write_bit(data & 0x00800000);
		EEPROM_set_cs_line((data & 0x00200000) ? CLEAR_LINE : ASSERT_LINE);
		EEPROM_set_clock_line((data & 0x00400000) ? ASSERT_LINE : CLEAR_LINE);
	}

	/* log any unknown bits */
	if (data & 0x4f1fffff)
		log_cb(RETRO_LOG_DEBUG, LOGPRE "%08X: control_w = %08X & %08X\n", activecpu_get_previouspc(), data, ~mem_mask);
}



/*************************************
 *
 *	BSMT2000 I/O
 *
 *************************************/

static WRITE32_HANDLER( bsmt2000_reg_w )
{
	if (control_data & 0x80000000)
		BSMT2000_data_0_w(bsmt_reg, data & 0xffff, mem_mask | 0xffff0000);
	else
		COMBINE_DATA(&bsmt_data_offset);
}


static WRITE32_HANDLER( bsmt2000_data_w )
{
	if (control_data & 0x80000000)
		COMBINE_DATA(&bsmt_reg);
	else
		COMBINE_DATA(&bsmt_data_bank);
}


static READ32_HANDLER( bsmt2000_data_r )
{
	return memory_region(REGION_SOUND1)[bsmt_data_bank * 0x10000 + bsmt_data_offset] << 8;
}



/*************************************
 *
 *	Busy loop optimization
 *
 *************************************/

static WRITE32_HANDLER( speedup_w )
{
	COMBINE_DATA(speedup_data);

	/* see if the PC matches */
	if ((activecpu_get_previouspc() & 0x1fffffff) == speedup_pc)
	{
		UINT32 curr_cycles = activecpu_gettotalcycles();

		/* if less than 50 cycles from the last time, count it */
		if (curr_cycles - last_cycles < 50)
		{
			loop_count++;

			/* more than 2 in a row and we spin */
			if (loop_count > 2)
				cpu_spinuntil_int();
		}
		else
			loop_count = 0;

		last_cycles = curr_cycles;
	}
}



/*************************************
 *
 *	EEPROM interface/saving
 *
 *************************************/

struct EEPROM_interface eeprom_interface_policetr =
{
	8,				/* address bits	8*/
	16,				/* data bits	16*/
	"*110",			/* read			1 10 aaaaaa*/
	"*101",			/* write		1 01 aaaaaa dddddddddddddddd*/
	"*111",			/* erase		1 11 aaaaaa*/
	"*10000xxxx",	/* lock			1 00 00xxxx*/
	"*10011xxxx"	/* unlock		1 00 11xxxx*/
};


static NVRAM_HANDLER( policetr )
{
	if (read_or_write)
		EEPROM_save(file);
	else
	{
		EEPROM_init(&eeprom_interface_policetr);
		if (file)	EEPROM_load(file);
	}
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ32_START( policetr_readmem )
	{ 0x00000000, 0x0001ffff, MRA32_RAM },
	{ 0x00400000, 0x00400003, policetr_video_r },
	{ 0x00600000, 0x00600003, bsmt2000_data_r },
	{ 0x00a00000, 0x00a00003, port0_r },
	{ 0x00a20000, 0x00a20003, port1_r },
	{ 0x00a40000, 0x00a40003, port2_r },
	{ 0x1fc00000, 0x1fdfffff, MRA32_ROM },
MEMORY_END


static MEMORY_WRITE32_START( policetr_writemem )
	{ 0x00000000, 0x0001ffff, MWA32_RAM, &policetr_rambase },
	{ 0x00200000, 0x0020000f, policetr_video_w },
	{ 0x00500000, 0x00500003, MWA32_NOP },		/* copies ROM here at startup, plus checksum*/
	{ 0x00700000, 0x00700003, bsmt2000_reg_w },
	{ 0x00800000, 0x00800003, bsmt2000_data_w },
	{ 0x00900000, 0x00900003, policetr_palette_offset_w },
	{ 0x00920000, 0x00920003, policetr_palette_data_w },
	{ 0x00a00000, 0x00a00003, control_w },
	{ 0x00e00000, 0x00e00003, MWA32_NOP },		/* watchdog???*/
	{ 0x1fc00000, 0x1fdfffff, MWA32_ROM, &rom_base },
MEMORY_END


static MEMORY_WRITE32_START( sshooter_writemem )
	{ 0x00000000, 0x0001ffff, MWA32_RAM, &policetr_rambase },
	{ 0x00200000, 0x00200003, bsmt2000_data_w },
	{ 0x00300000, 0x00300003, policetr_palette_offset_w },
	{ 0x00320000, 0x00320003, policetr_palette_data_w },
	{ 0x00500000, 0x00500003, MWA32_NOP },		/* copies ROM here at startup, plus checksum*/
	{ 0x00700000, 0x00700003, bsmt2000_reg_w },
	{ 0x00800000, 0x0080000f, policetr_video_w },
	{ 0x00a00000, 0x00a00003, control_w },
	{ 0x00e00000, 0x00e00003, MWA32_NOP },		/* watchdog???*/
	{ 0x1fc00000, 0x1fdfffff, MWA32_ROM, &rom_base },
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( policetr )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_SERVICE( 0x0020, IP_ACTIVE_LOW )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_SPECIAL )		/* EEPROM read*/
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ))
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x01, DEF_STR( On ))
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ))
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x02, DEF_STR( On ))
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ))
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x04, DEF_STR( On ))
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ))
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x08, DEF_STR( On ))
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ))
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x10, DEF_STR( On ))
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown )) /* Manuals show dips 1 through 6 as unused */
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x20, DEF_STR( On ))
	PORT_DIPNAME( 0x40, 0x40, "Monitor Sync")
	PORT_DIPSETTING(    0x00, "+")
	PORT_DIPSETTING(    0x40, "-")
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen )) /* For use with mirrored CRTs - Not supported */
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x80, DEF_STR( On ))	/* Will invert the Y axis of guns */
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START				/* fake analog X */
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_X, 50, 10, 0, 255 )

	PORT_START				/* fake analog Y */
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_Y, 70, 10, 0, 255 )

	PORT_START				/* fake analog X */
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_X | IPF_PLAYER2, 50, 10, 0, 255 )

	PORT_START				/* fake analog Y */
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_Y | IPF_PLAYER2, 70, 10, 0, 255 )
INPUT_PORTS_END




/*************************************
 *
 *	Sound definitions
 *
 *************************************/

static struct BSMT2000interface bsmt2000_interface =
{
	1,
	{ MASTER_CLOCK/2 },
	{ 11 },
	{ REGION_SOUND1 },
	{ 100 }
};



/*************************************
 *
 *	Machine driver
 *
 *************************************/

static struct r3000_config config =
{
	0,		/* 1 if we have an FPU, 0 otherwise */
	4096,	/* code cache size */
	4096	/* data cache size */
};


MACHINE_DRIVER_START( policetr )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", R3000BE, MASTER_CLOCK/2)
	MDRV_CPU_CONFIG(config)
	MDRV_CPU_MEMORY(policetr_readmem,policetr_writemem)
	MDRV_CPU_VBLANK_INT(irq4_gen,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(policetr)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(400, 240)
	MDRV_VISIBLE_AREA(0, 393, 0, 239)
	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_START(policetr)
	MDRV_VIDEO_UPDATE(policetr)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(BSMT2000, bsmt2000_interface)
MACHINE_DRIVER_END


MACHINE_DRIVER_START( sshooter )
	MDRV_IMPORT_FROM(policetr)

	/* basic machine hardware */
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(policetr_readmem,sshooter_writemem)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definition(s)
 *
 *************************************/

ROM_START( policetr )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )		/* dummy region for R3000 */

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE( "pt-u121.bin", 0x000000, 0x100000, CRC(56b0b00a) SHA1(4034fe373a61f756f4813f0c20b1cf05e4338059) )
	ROM_LOAD16_BYTE( "pt-u120.bin", 0x000001, 0x100000, CRC(ca664142) SHA1(2727ecb9287b4ed30088e017bb6b8763dfb75b2f) )
	ROM_LOAD16_BYTE( "pt-u125.bin", 0x200000, 0x100000, CRC(e9ccf3a0) SHA1(b3fd8c094f76ace4cf403c3d0f6bd6c5d8db7d6a) )
	ROM_LOAD16_BYTE( "pt-u124.bin", 0x200001, 0x100000, CRC(f4acf921) SHA1(5b244e9a51304318fa0c03eb7365b3c12627d19b) )

	ROM_REGION32_BE( 0x80000, REGION_USER1, 0 )	/* 2MB for R3000 code */
	ROM_LOAD32_BYTE( "pt-u113.bin", 0x00000, 0x20000, CRC(7b34d366) SHA1(b86cfe155e0685992aebbcc7db705fdbadc42bf9) )
	ROM_LOAD32_BYTE( "pt-u112.bin", 0x00001, 0x20000, CRC(57d059c8) SHA1(ed0c624fc0afbeb6616bba8a67ce5b18d7c119fc) )
	ROM_LOAD32_BYTE( "pt-u111.bin", 0x00002, 0x20000, CRC(fb5ce933) SHA1(4a07ac3e2d86262061092f112cab89f8660dce3d) )
	ROM_LOAD32_BYTE( "pt-u110.bin", 0x00003, 0x20000, CRC(40bd6f60) SHA1(156000d3c439eab45962f0a2681bd806a17f47ee) )

	ROM_REGION( 0x600000, REGION_SOUND1, 0 )
	ROM_LOAD( "pt-u160.bin", 0x000000, 0x100000, CRC(f267f813) SHA1(ae58507947fe2e9701b5df46565fd9908e2f9d77) )
	ROM_RELOAD(              0x3f8000, 0x100000 )
	ROM_LOAD( "pt-u162.bin", 0x100000, 0x100000, CRC(75fe850e) SHA1(ab8cf24ae6e5cf80f6a9a34e46f2b1596879643b) )
	ROM_RELOAD(              0x4f8000, 0x100000 )
ROM_END


ROM_START( policeto )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )		/* dummy region for R3000 */

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE( "pt-u121.bin", 0x000000, 0x100000, CRC(56b0b00a) SHA1(4034fe373a61f756f4813f0c20b1cf05e4338059) )
	ROM_LOAD16_BYTE( "pt-u120.bin", 0x000001, 0x100000, CRC(ca664142) SHA1(2727ecb9287b4ed30088e017bb6b8763dfb75b2f) )
	ROM_LOAD16_BYTE( "pt-u125.bin", 0x200000, 0x100000, CRC(e9ccf3a0) SHA1(b3fd8c094f76ace4cf403c3d0f6bd6c5d8db7d6a) )
	ROM_LOAD16_BYTE( "pt-u124.bin", 0x200001, 0x100000, CRC(f4acf921) SHA1(5b244e9a51304318fa0c03eb7365b3c12627d19b) )

	ROM_REGION32_BE( 0x80000, REGION_USER1, 0 )	/* 2MB for R3000 code */
	ROM_LOAD32_BYTE( "pt-u113.v11", 0x00000, 0x20000, CRC(3d62f6d6) SHA1(342ffa38a6972bbb03c89b4dd603c2cc60609d3d) )
	ROM_LOAD32_BYTE( "pt-u112.v11", 0x00001, 0x20000, CRC(942b280b) SHA1(c342ba3255203ce28ff59479da00f26f0bd026e0) )
	ROM_LOAD32_BYTE( "pt-u111.v11", 0x00002, 0x20000, CRC(da6c45a7) SHA1(471bd372d2ad5bcb29af19dae09f3cfab4b010fd) )
	ROM_LOAD32_BYTE( "pt-u110.v11", 0x00003, 0x20000, CRC(1360ac2b) SHA1(789673403d3acac7b3c9ebd7914b65f287a94a11) ) /* Fails Checksum, Bug in the program/checksum code???*/

	ROM_REGION( 0x600000, REGION_SOUND1, 0 )
	ROM_LOAD( "pt-u160.bin", 0x000000, 0x100000, CRC(f267f813) SHA1(ae58507947fe2e9701b5df46565fd9908e2f9d77) )
	ROM_RELOAD(              0x3f8000, 0x100000 )
	ROM_LOAD( "pt-u162.bin", 0x100000, 0x100000, CRC(75fe850e) SHA1(ab8cf24ae6e5cf80f6a9a34e46f2b1596879643b) )
	ROM_RELOAD(              0x4f8000, 0x100000 )
ROM_END


ROM_START( plctr13b )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )		/* dummy region for R3000 */

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE( "pt-u121.bin", 0x000000, 0x100000, CRC(56b0b00a) SHA1(4034fe373a61f756f4813f0c20b1cf05e4338059) )
	ROM_LOAD16_BYTE( "pt-u120.bin", 0x000001, 0x100000, CRC(ca664142) SHA1(2727ecb9287b4ed30088e017bb6b8763dfb75b2f) )
	ROM_LOAD16_BYTE( "pt-u125.bin", 0x200000, 0x100000, CRC(e9ccf3a0) SHA1(b3fd8c094f76ace4cf403c3d0f6bd6c5d8db7d6a) )
	ROM_LOAD16_BYTE( "pt-u124.bin", 0x200001, 0x100000, CRC(f4acf921) SHA1(5b244e9a51304318fa0c03eb7365b3c12627d19b) )

	ROM_REGION32_BE( 0x80000, REGION_USER1, 0 )	/* 2MB for R3000 code */
/*
Note: If you sent the dipswitch to service mode and reset the game within
Mame.  All 4 program ROMs fail the checksum code... IE: they show in red
instead of green.  But, the listed checksums on the screen match the
checksums printed on the ROM labels... this seems wierd to me.
*/
	ROM_LOAD32_BYTE( "ptb-u113.v13", 0x00000, 0x20000, CRC(d636c00d) SHA1(ef989eb85b51a64ca640297c1286514c8d7f8f76) )
	ROM_LOAD32_BYTE( "ptb-u112.v13", 0x00001, 0x20000, CRC(86f0497e) SHA1(d177023f7cb2e01de60ef072212836dc94759c1a) )
	ROM_LOAD32_BYTE( "ptb-u111.v13", 0x00002, 0x20000, CRC(39e96d6a) SHA1(efe6ffe70432b94c98f3d7247408a6d2f6f9e33d) )
	ROM_LOAD32_BYTE( "ptb-u110.v13", 0x00003, 0x20000, CRC(d7e6f4cb) SHA1(9dffe4937bc5cf47d870f06ae0dced362cd2dd66) )

	ROM_REGION( 0x600000, REGION_SOUND1, 0 )
	ROM_LOAD( "pt-u160.bin", 0x000000, 0x100000, CRC(f267f813) SHA1(ae58507947fe2e9701b5df46565fd9908e2f9d77) )
	ROM_RELOAD(              0x3f8000, 0x100000 )
	ROM_LOAD( "pt-u162.bin", 0x100000, 0x100000, CRC(75fe850e) SHA1(ab8cf24ae6e5cf80f6a9a34e46f2b1596879643b) )
	ROM_RELOAD(              0x4f8000, 0x100000 )
ROM_END

ROM_START( sshooter )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )		/* dummy region for R3000 */

	ROM_REGION( 0x800000, REGION_GFX1, ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE( "ss-u121.bin", 0x000000, 0x100000, CRC(22e27dd6) SHA1(cb9e8c450352bb116a9c0407cc8ce6d8ae9d9881) ) /* 1:1*/
	ROM_LOAD16_BYTE( "ss-u120.bin", 0x000001, 0x100000, CRC(30173b1b) SHA1(366464444ce208391ca350f1639403f0c2217330) ) /* 1:2*/
	ROM_LOAD16_BYTE( "ss-u125.bin", 0x200000, 0x100000, CRC(79e8520a) SHA1(682e5c7954f96db65a137f05cde67c310b85b526) ) /* 2:1*/
	ROM_LOAD16_BYTE( "ss-u124.bin", 0x200001, 0x100000, CRC(8e805970) SHA1(bfc9940ed6425f136d768170275279c590da7003) ) /* 2:2*/
	ROM_LOAD16_BYTE( "ss-u123.bin", 0x400000, 0x100000, CRC(d045bb62) SHA1(839209ff6a8e5db63a51a3494a6c973e0068a3c6) ) /* 3:1*/
	ROM_LOAD16_BYTE( "ss-u122.bin", 0x400001, 0x100000, CRC(163cc133) SHA1(a5e84b5060fd32362aa097d0194ce72e8a90357c) ) /* 3:2*/
	ROM_LOAD16_BYTE( "ss-u127.bin", 0x600000, 0x100000, CRC(76a7a591) SHA1(9fd7cce21b01f388966a3e8388ba95820ac10bfd) ) /* 4:1*/
	ROM_LOAD16_BYTE( "ss-u126.bin", 0x600001, 0x100000, CRC(ab1b9d60) SHA1(ff51a71443f7774d3abf96c2eb8ef6a54d73dd8e) ) /* 4:2*/

	ROM_REGION32_BE( 0x100000, REGION_USER1, 0 )	/* 2MB for R3000 code */
	ROM_LOAD32_BYTE( "ss-u113.v17", 0x00000, 0x40000, CRC(a8c96af5) SHA1(a62458156603b74e0d84ce6928f7bb868bf5a219) )
	ROM_LOAD32_BYTE( "ss-u112.v17", 0x00001, 0x40000, CRC(c732d5fa) SHA1(2bcc26c8bbf55394173ca65b4b0df01bc6b719bb) )
	ROM_LOAD32_BYTE( "ss-u111.v17", 0x00002, 0x40000, CRC(4240fa2f) SHA1(54223207c1e228d6b836918601c0f65c2692e5bc) )
	ROM_LOAD32_BYTE( "ss-u110.v17", 0x00003, 0x40000, CRC(8ae744ce) SHA1(659cd27865cf5507aae6b064c5bc24b927cf5f5a) )

	ROM_REGION( 0x600000, REGION_SOUND1, 0 )
	ROM_LOAD( "ss-u160.bin", 0x000000, 0x100000, CRC(1c603d42) SHA1(880992871be52129684052d542946de0cc32ba9a) ) /* 1:1*/
	ROM_RELOAD(              0x3f8000, 0x100000 )
	ROM_LOAD( "ss-u162.bin", 0x100000, 0x100000, CRC(40ef448a) SHA1(c96f7b169be2576e9f3783af84c07259efefb812) ) /* 2:1*/
	ROM_RELOAD(              0x4f8000, 0x100000 )
ROM_END



/*************************************
 *
 *	Driver initialization
 *
 *************************************/

static DRIVER_INIT( policetr )
{
	speedup_data = install_mem_write32_handler(0, 0x00000fc8, 0x00000fcb, speedup_w);
	speedup_pc = 0x1fc028ac;

	memcpy(rom_base, memory_region(REGION_USER1), memory_region_length(REGION_USER1));
}


static DRIVER_INIT( sshooter )
{
	speedup_data = install_mem_write32_handler(0, 0x00018fd8, 0x00018fdb, speedup_w);
	speedup_pc = 0x1fc03470;

	memcpy(rom_base, memory_region(REGION_USER1), memory_region_length(REGION_USER1));
}



/*************************************
 *
 *	Game driver(s)
 *
 *************************************/

GAME( 1996, policetr, 0,        policetr, policetr, policetr, ROT0, "PandP Marketing", "Police Trainer (Rev 1.3)" )
GAME( 1996, policeto, policetr, policetr, policetr, policetr, ROT0, "PandP Marketing", "Police Trainer (Rev 1.1)" )
GAME( 1996, plctr13b, policetr, sshooter, policetr, policetr, ROT0, "PandP Marketing", "Police Trainer (Rev 1.3B)" )
GAME( 1998, sshooter, 0,        sshooter, policetr, sshooter, ROT0, "PandP Marketing", "Sharpshooter (Rev 1.7)" )
