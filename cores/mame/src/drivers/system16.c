/*
	Sega System16 Hardware
	major cleanup in progress - still a lot to do!H

	see vidhrdw/system16.c for more information

Changes:

04/28/04  Charles MacDonald
- Added MSM5205 sample playback to tturfbl.

03/17/04
- Added correctly dumped ROM set for eswat to replace the old one. Game is encrypted and unplayable.
- Moved Ace Attacker here from System 18 driver. Game is encrypted and unplayable.
- Added sound support for tturf, tturfu, tturfbl (no samples), fpointbl, fpointbj
- Fixed toryumon RAM test
- Cleaned up timscannr, toryumon drivers
03/11/04
- Cleaned up riotcity, aurail, altbeast, bayroute drivers
- Added missing coin control to sys16_coinctrl_w
- Removed 'extra' RAM in some drivers and replaced with sys16_tilebank_w

To do:
- tturf and tturfu have some invalid data written to the UPD7759. I think this causes the 'pop' sound when level 1 starts.
- bayroute has a bad sprite in the door of the level 1 boss
- altbeast writes to mirrored sprite RAM at $441000
- tturfu has bad single sprite frame when main character walks
- dduxbl has some 'stuck' sprites

Notes:
- toryumon RAM test accesses mirrored work RAM. Maybe there's a better way to support this than using AM_MASK.
- I separated the fpoint and fpointbl,fpointbj machine drivers as the latter two have different sound hardware,
  but the original does not. I think this creates a dependancy where fpointbl.zip needs flpoint.001 from fpointbj.zip,
  as fpointbl uses fpoint ROMs (it's parent), it's own, and the sound ROM from fpointbj.
  So add fpoint.001 to fpointbl.zip for it to work.
  I made fpointbl the parent of fpointbj so it would use the proper memory map for the sound hardware.
*/

/***************************************************************************/
/*
  ASTORMBL
          3. In the ending, the 3 heroes are floating into a half bubble. (see picture).
          Also colour problems during ending as well.
          4. In the later Shooting gallery stage (like inside the car shop and the factory (mission 3)),
		  there is some garbage graphics (sprite of death monsters that appear where they should not)

	working:
		Alex Kidd
		Alien Storm (bootleg)
		Alien Syndrome
		Altered Beast (Ver 1)
		Altered Beast (Ver 2)	(No Sound)
		Atomic Point			(No Sound)
		Aurail					(Speech quality sounds poor)
		Aurail (317-0168)
		Bay Route
		Body Slam
	    Dump Matsumoto (Japan, Body Slam)
		Dynamite Dux (bootleg)
		Enduro Racer (bootleg)
		Enduro Racer (custom bootleg)
		E-Swat (bootleg)
		Fantasy Zone (Old Ver.)
		Fantasy Zone (New Ver.)
		Flash Point  (bootleg)
		Golden Axe (Ver 1)
		Golden Axe (Ver 2)
		Hang-on
		Heavyweight Champ: some minor graphics glitches
		Major League: No game over.
		Moonwalker (bootleg): Music Speed varies
		Outrun (set 1)
		Outrun (set 2)
		Outrun (custom bootleg)
		Passing Shot (bootleg)
		Passing Shot (4 player bootleg)
		Quartet: Glitch on highscore list
		Quartet (Japan): Glitch on highscore list
		Quartet 2: Glitch on highscore list
		Riot City
		SDI
		Shadow Dancer
		Shadow Dancer (Japan)
		Shinobi
		Shinobi (Sys16A Bootleg?)
		Space Harrier
		Super Hangon (bootleg)
		Tetris (bootleg)
		Time Scanner
		Toryumon
		Tough Turf (Japan)			(No Sound)
		Tough Turf (US)				(No Sound)
		Tough Turf (bootleg)	(No Speech Roms)
		Wonderboy 3 - Monster Lair
		Wonderboy 3 - Monster Lair (bootleg)
		Wrestle War

	not really working:
		Shadow Dancer (bootleg)

	protected:
		Alex Kidd (jpn?)
		Alien Syndrome
		Alien Syndrome
		Alien Syndrome (Japan)
		Alien Storm
		Alien Storm (2 Player)
		Bay Route (317-0116)
		Bay Route (protected bootleg 1)
		Bay Route (protected bootleg 2)
		Enduro Racer
		E-Swat
		Flash Point
		Golden Axe (Ver 1 317-0121 Japan)
		Golden Axe (Ver 2 317-0110)
		Golden Axe (Ver 2 317-0122)
		Golden Axe (protected bootleg)
		Jyuohki (Japan, altered beast)
		Moonwalker (317-0158)
		Moonwalker (317-0159)
		Passing Shot (317-0080)
		Shinobi (Sys16B 317-0049)
		Shinobi (Sys16A 317-0050)
		SDI (Japan, old version)
		Super Hangon
		Tetris (Type A)
		Tetris (Type B 317-0092)
		Wonderboy 3 - Monster Lair (317-0089)

	protected (No driver):
		Ace Attacker
		Action Fighter
		Bloxeed
		Clutch Hitter
		Cotton (Japan)
		Cotton
		DD Crew
		Dunk Shot
		Excite League
		Laser Ghost
		MVP
		Ryukyu
		Super Leagu
		Turbo Outrun
		Turbo Outrun (Set 2)
*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"
#include "cpu/i8039/i8039.h"
#include "system16.h"
#include "machine/fd1089.h"
extern void mc8123_decrypt_0066(void);

/***************************************************************************/

/* 7751 emulation */
WRITE_HANDLER( sys16_7751_audio_8255_w );
 READ_HANDLER( sys16_7751_audio_8255_r );
 READ_HANDLER( sys16_7751_sh_rom_r );
 READ_HANDLER( sys16_7751_sh_t1_r );
 READ_HANDLER( sys16_7751_sh_command_r );
WRITE_HANDLER( sys16_7751_sh_dac_w );
WRITE_HANDLER( sys16_7751_sh_busy_w );
WRITE_HANDLER( sys16_7751_sh_offset_a0_a3_w );
WRITE_HANDLER( sys16_7751_sh_offset_a4_a7_w );
WRITE_HANDLER( sys16_7751_sh_offset_a8_a11_w );
WRITE_HANDLER( sys16_7751_sh_rom_select_w );

/***************************************************************************/

int sys16_soundbanktype;

static data16_t coinctrl;

static WRITE16_HANDLER( sys16_3d_coinctrl_w )
{
	if( ACCESSING_LSB ){
		coinctrl = data&0xff;
		sys16_refreshenable = coinctrl & 0x10;
		coin_counter_w(0,coinctrl & 0x01);
		/* bit 6 is also used (0 in fantzone) */

		/* Hang-On, Super Hang-On, Space Harrier, Enduro Racer */
		set_led_status(0,coinctrl & 0x04);

		/* Space Harrier */
		set_led_status(1,coinctrl & 0x08);
	}
}

static INTERRUPT_GEN( sys16_interrupt )
{
	if(sys16_custom_irq) sys16_custom_irq();
	cpu_set_irq_line(0, 4, HOLD_LINE); /* Interrupt vector 4, used by VBlank */
}



/***************************************************************************/
/*
	Tough Turf (Datsu bootleg) sound emulation

	Memory map

	0000-7fff : ROM (fixed, tt014d68 0000-7fff)
	8000-bfff : ROM (banked)
	e000      : Bank control
	e800      : Sound command latch
	f000      : MSM5205 sample data buffer
	f800-ffff : Work RAM

	Interrupts

	IRQ = Read sound command from $E800
	NMI = Copy data from fixed/banked ROM to $F000

	Bank control values

	00 = tt014d68 8000-bfff
	01 = tt014d68 c000-ffff
	02 = tt0246ff 0000-3fff
	03 = tt0246ff 4000-7fff
	04 = tt0246ff 8000-bfff

	The sample sound codes in the sound test are OK, but in-game sample playback is bad.
	There seems to be more data in the high bits of the ROM bank control word which may be related.
*/

static int sample_buffer = 0;
static int sample_select = 0;

static WRITE_HANDLER( tturfbl_msm5205_data_w )
{
	sample_buffer = data;
}

static void tturfbl_msm5205_callback(int data)
{
	MSM5205_data_w(0, (sample_buffer >> 4) & 0x0F);
	sample_buffer <<= 4;
	sample_select ^= 1;
	if(sample_select == 0)
		cpu_set_nmi_line(1, PULSE_LINE);
}

static struct MSM5205interface tturfbl_msm5205_interface =
{
	1,
	220000, /* 220KHz */
	{ tturfbl_msm5205_callback },
	{ MSM5205_S48_4B},
	{ 80 }
};


UINT8 *tturfbl_soundbank_ptr = NULL;		/* Pointer to currently selected portion of ROM */

static READ_HANDLER( tturfbl_soundbank_r )
{
	if(tturfbl_soundbank_ptr) return tturfbl_soundbank_ptr[offset & 0x3fff];
	return 0x80;
}

static WRITE_HANDLER( tturfbl_soundbank_w )
{
	UINT8 *mem = memory_region(REGION_CPU2);

	switch(data)
	{
		case 0:
			tturfbl_soundbank_ptr = &mem[0x18000]; /* tt014d68 8000-bfff */
			break;
		case 1:
			tturfbl_soundbank_ptr = &mem[0x1C000]; /* tt014d68 c000-ffff */
			break;
		case 2:
			tturfbl_soundbank_ptr = &mem[0x20000]; /* tt0246ff 0000-3fff */
			break;
		case 3:
			tturfbl_soundbank_ptr = &mem[0x24000]; /* tt0246ff 4000-7fff */
			break;
		case 4:
			tturfbl_soundbank_ptr = &mem[0x28000]; /* tt0246ff 8000-bfff */
			break;
		case 8:
			tturfbl_soundbank_ptr = mem;
			break;
		default:
			tturfbl_soundbank_ptr = NULL;
			log_cb(RETRO_LOG_DEBUG, LOGPRE "Invalid bank setting %02X (%04X)\n", data, activecpu_get_pc());
			break;
	}
}

static MEMORY_READ_START( tturfbl_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, tturfbl_soundbank_r },
	{ 0xe800, 0xe800, soundlatch_r },
	{ 0xf800, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( tturfbl_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0xbfff, MWA_NOP },/* ROM bank */
	{ 0xe000, 0xe000, tturfbl_soundbank_w },
	{ 0xf000, 0xf000, tturfbl_msm5205_data_w },
	{ 0xf800, 0xffff, MWA_RAM },
MEMORY_END

static PORT_READ_START( tturfbl_sound_readport )
	{ 0x01, 0x01, YM2151_status_port_0_r },
	{ 0x80, 0x80, MRA_NOP },
PORT_END

static PORT_WRITE_START( tturfbl_sound_writeport )
	{ 0x00, 0x00, YM2151_register_port_0_w },
	{ 0x01, 0x01, YM2151_data_port_0_w },
	{ 0x40, 0x40, MWA_NOP },
	{ 0x80, 0x80, MWA_NOP },
PORT_END

/*******************************************************************************/

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xe800, 0xe800, soundlatch_r },
	{ 0xf800, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xf800, 0xffff, MWA_RAM },
MEMORY_END

static PORT_READ_START( sound_readport )
	{ 0x01, 0x01, YM2151_status_port_0_r },
	{ 0xc0, 0xc0, soundlatch_r },
PORT_END

static PORT_WRITE_START( sound_writeport )
	{ 0x00, 0x00, YM2151_register_port_0_w },
	{ 0x01, 0x01, YM2151_data_port_0_w },
PORT_END

/* 7751 Sound */
static MEMORY_READ_START( sound_readmem_7751 )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xe800, 0xe800, soundlatch_r },
	{ 0xf800, 0xffff, MRA_RAM },
MEMORY_END

static PORT_READ_START( sound_readport_7751 )
	{ 0x01, 0x01, YM2151_status_port_0_r },
	/*{ 0x0e, 0x0e, sys16_7751_audio_8255_r }, */
	{ 0xc0, 0xc0, soundlatch_r },
PORT_END

static PORT_WRITE_START( sound_writeport_7751 )
	{ 0x00, 0x00, YM2151_register_port_0_w },
	{ 0x01, 0x01, YM2151_data_port_0_w },
	{ 0x80, 0x80, sys16_7751_audio_8255_w },
PORT_END

static MEMORY_READ_START( readmem_7751 )
	{ 0x0000, 0x03ff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem_7751 )
	{ 0x0000, 0x03ff, MWA_ROM },
MEMORY_END

static PORT_READ_START( readport_7751 )
	{ I8039_t1, I8039_t1, sys16_7751_sh_t1_r },
	{ I8039_p2, I8039_p2, sys16_7751_sh_command_r },
	{ I8039_bus, I8039_bus, sys16_7751_sh_rom_r },
PORT_END

static PORT_WRITE_START( writeport_7751 )
	{ I8039_p1, I8039_p1, sys16_7751_sh_dac_w },
	{ I8039_p2, I8039_p2, sys16_7751_sh_busy_w },
	{ I8039_p4, I8039_p4, sys16_7751_sh_offset_a0_a3_w },
	{ I8039_p5, I8039_p5, sys16_7751_sh_offset_a4_a7_w },
	{ I8039_p6, I8039_p6, sys16_7751_sh_offset_a8_a11_w },
	{ I8039_p7, I8039_p7, sys16_7751_sh_rom_select_w },
PORT_END

/* 7759 */
static MEMORY_READ_START( sound_readmem_7759 )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xdfff, MRA_BANK1 },
	{ 0xe800, 0xe800, soundlatch_r },
	{ 0xf800, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem_7759 )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0xdfff, MWA_BANK1 },
	{ 0xf800, 0xffff, MWA_RAM },
MEMORY_END

static PORT_WRITE_START( sound_writeport_7759 )
	{ 0x00, 0x00, YM2151_register_port_0_w },
	{ 0x01, 0x01, YM2151_data_port_0_w },
	{ 0x40, 0x40, UPD7759_bank_w },
	{ 0x80, 0x80, upd7759_0_port_w },
PORT_END


static WRITE16_HANDLER( sound_command_w ){
	if( ACCESSING_LSB ){
		soundlatch_w( 0,data&0xff );
		cpu_set_irq_line( 1, 0, HOLD_LINE );
	}
}

static WRITE16_HANDLER( sound_command_nmi_w ){
	if( ACCESSING_LSB ){
		soundlatch_w( 0,data&0xff );
		cpu_set_nmi_line(1, PULSE_LINE);
	}
}

/*static data16_t coinctrl; */

static READ16_HANDLER( sys16_coinctrl_r ){
	return coinctrl;
}

static WRITE16_HANDLER( sys16_coinctrl_w )
{
	if( ACCESSING_LSB ){
		coinctrl = data&0xff;
		sys16_refreshenable = coinctrl & 0x20;
		set_led_status(1,coinctrl & 0x08);
		set_led_status(0,coinctrl & 0x04);
		coin_counter_w(1,coinctrl & 0x02);
		coin_counter_w(0,coinctrl & 0x01);
		/* bit 6 is also used (1 most of the time; 0 in dduxbl, sdi, wb3;
		   tturf has it normally 1 but 0 after coin insertion) */
		/* eswat sets bit 4 */
	}
}

/***************************************************************************/

static MACHINE_DRIVER_START( system16 )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M68000, 10000000)
	MDRV_CPU_VBLANK_INT(sys16_interrupt,1)

	MDRV_CPU_ADD_TAG("sound", Z80, 4000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PORTS(sound_readport,sound_writeport)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(sys16_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048*ShadowColorsMultiplier)

	/* initilize system16 variables prior to driver_init and video_start */
	machine_init_sys16_onetime();

	MDRV_VIDEO_START(system16)
	MDRV_VIDEO_UPDATE(system16)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD_TAG("2151", YM2151, sys16_ym2151_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( system16b )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M68000, 10000000)
	MDRV_CPU_VBLANK_INT(sys16_interrupt,1)

	MDRV_CPU_ADD_TAG("sound", Z80, 5000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PORTS(sound_readport,sound_writeport)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(sys16_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048*ShadowColorsMultiplier)

	/* initilize system16 variables prior to driver_init and video_start */
	machine_init_sys16_onetime();

	MDRV_VIDEO_START(system16)
	MDRV_VIDEO_UPDATE(system16)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD_TAG("2151", YM2151, sys16_ym2151_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( system16_7759 )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16)

	MDRV_CPU_MODIFY("sound")
	MDRV_CPU_MEMORY(sound_readmem_7759,sound_writemem_7759)
	MDRV_CPU_PORTS(sound_readport,sound_writeport_7759)

	/* sound hardware */
	MDRV_SOUND_ADD_TAG("7759", UPD7759, sys16_upd7759_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( system16_7759b )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16b)

	MDRV_CPU_MODIFY("sound")
	MDRV_CPU_MEMORY(sound_readmem_7759,sound_writemem_7759)
	MDRV_CPU_PORTS(sound_readport,sound_writeport_7759)

	/* sound hardware */
	MDRV_SOUND_ADD_TAG("7759", UPD7759, sys16b_upd7759_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( system16_7751 )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16)

	MDRV_CPU_MODIFY("sound")
	MDRV_CPU_MEMORY(sound_readmem_7751,sound_writemem)
	MDRV_CPU_PORTS(sound_readport_7751,sound_writeport_7751)

	MDRV_CPU_ADD(N7751, 6000000/15)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(readmem_7751,writemem_7751)
	MDRV_CPU_PORTS(readport_7751,writeport_7751)

	/* sound hardware */
	MDRV_SOUND_ADD(DAC, sys16_7751_dac_interface)
MACHINE_DRIVER_END

/***************************************************************************/

static WRITE16_HANDLER( sys16_tilebank_w )
{
	if(ACCESSING_LSB)
	{
		switch(offset & 1)
		{
			case 0:
				sys16_tile_bank0 = data & 0x0F;
				break;
			case 1:
				sys16_tile_bank1 = data & 0x0F;
				break;
		}
	}
}

static void set_tile_bank( int data ){
	sys16_tile_bank1 = data&0xf;
	sys16_tile_bank0 = (data>>4)&0xf;
}

#if 0
static void set_tile_bank18( int data ){
	sys16_tile_bank0 = data&0xf;
	sys16_tile_bank1 = (data>>4)&0xf;
}
#endif

static void set_fg_page( int data ){
	sys16_fg_page[0] = data>>12;
	sys16_fg_page[1] = (data>>8)&0xf;
	sys16_fg_page[2] = (data>>4)&0xf;
	sys16_fg_page[3] = data&0xf;
}

static void set_bg_page( int data ){
	sys16_bg_page[0] = data>>12;
	sys16_bg_page[1] = (data>>8)&0xf;
	sys16_bg_page[2] = (data>>4)&0xf;
	sys16_bg_page[3] = data&0xf;
}

static void set_fg_page1( int data ){
	sys16_fg_page[1] = data>>12;
	sys16_fg_page[0] = (data>>8)&0xf;
	sys16_fg_page[3] = (data>>4)&0xf;
	sys16_fg_page[2] = data&0xf;
}

static void set_bg_page1( int data ){
	sys16_bg_page[1] = data>>12;
	sys16_bg_page[0] = (data>>8)&0xf;
	sys16_bg_page[3] = (data>>4)&0xf;
	sys16_bg_page[2] = data&0xf;
}

static void set_fg2_page( int data ){
	sys16_fg2_page[0] = data>>12;
	sys16_fg2_page[1] = (data>>8)&0xf;
	sys16_fg2_page[2] = (data>>4)&0xf;
	sys16_fg2_page[3] = data&0xf;
}

static void set_bg2_page( int data ){
	sys16_bg2_page[0] = data>>12;
	sys16_bg2_page[1] = (data>>8)&0xf;
	sys16_bg2_page[2] = (data>>4)&0xf;
	sys16_bg2_page[3] = data&0xf;
}

static void type0_sys16_textram( void )
{
	set_fg_page( sys16_textram[0x740] );
	set_bg_page( sys16_textram[0x741] );
	set_fg2_page( sys16_textram[0x742] );
	set_bg2_page( sys16_textram[0x743] );

	sys16_fg_scrolly  = sys16_textram[0x748];
	sys16_bg_scrolly  = sys16_textram[0x749];
	sys16_fg2_scrolly = sys16_textram[0x74a];
	sys16_bg2_scrolly = sys16_textram[0x74b];

	sys16_fg_scrollx  = sys16_textram[0x74c];
	sys16_bg_scrollx  = sys16_textram[0x74d];
	sys16_fg2_scrollx = sys16_textram[0x74e];
	sys16_bg2_scrollx = sys16_textram[0x74f];
}

static void type1_sys16_textram( void )
{
	set_bg_page1( sys16_textram[0x74e] );
	set_fg_page1( sys16_textram[0x74f] );
	sys16_fg_scrolly = sys16_textram[0x792] & 0x00ff;
	sys16_bg_scrolly = sys16_textram[0x793] & 0x01ff;
	sys16_fg_scrollx = sys16_textram[0x7fc] & 0x01ff;
	sys16_bg_scrollx = sys16_textram[0x7fd] & 0x01ff;
}

/***************************************************************************/
/* sys16A */
ROM_START( alexkidd )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr-10447.43", 0x000000, 0x10000, CRC(29e87f71) SHA1(af980e55c02b3de1121c144fee23af74d24042ac) )
	ROM_LOAD16_BYTE( "epr-10445.26", 0x000001, 0x10000, CRC(25ce5b6f) SHA1(dfec64df7e8d145d30740808bc94bdbbe667c4e8) )
	ROM_LOAD16_BYTE( "epr-10448.42", 0x020000, 0x10000, CRC(05baedb5) SHA1(fc15989bf3d850170e4e018d74f18553f0268576) )
	ROM_LOAD16_BYTE( "epr-10446.25", 0x020001, 0x10000, CRC(cd61d23c) SHA1(c235c4fef28556e9f2d07e815ad213c308e85598) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "epr-10431.95", 0x00000, 0x08000, CRC(a7962c39) SHA1(c816fc5d9f21b2ba32b9841e64b634bce7ea78c8) )
	ROM_LOAD( "epr-10432.94", 0x08000, 0x08000, CRC(db8cd24e) SHA1(656d98844ad9ccaa68e3f501137dddd0a27d999d) )
	ROM_LOAD( "epr-10433.93", 0x10000, 0x08000, CRC(e163c8c2) SHA1(ac54c5ecedca5b1a2c550de32687ca57c4d3a411) )

	ROM_REGION( 0x040000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "epr-10437.10", 0x00001, 0x8000, CRC(522f7618) SHA1(9a6bc857dfef1dd1b7bffa034523c1c4cd8b3f4c) )
	ROM_LOAD16_BYTE( "epr-10441.11", 0x00000, 0x8000, CRC(74e3a35c) SHA1(26b980a0a3aee94ac38e0e0c7d305bb35a60d1c4) )
	ROM_LOAD16_BYTE( "epr-10438.17", 0x10001, 0x8000, CRC(738a6362) SHA1(a3c5f10c263cb216d275875f6333484a1cca281b) )
	ROM_LOAD16_BYTE( "epr-10442.18", 0x10000, 0x8000, CRC(86cb9c14) SHA1(42bd0ed985de61ff183eed0192257966caa01594) )
	ROM_LOAD16_BYTE( "epr-10439.23", 0x20001, 0x8000, CRC(b391aca7) SHA1(ca9d80b67e5365f709f90a5342b5e3aa7c7126e1) )
	ROM_LOAD16_BYTE( "epr-10443.24", 0x20000, 0x8000, CRC(95d32635) SHA1(788af2af1ae783128bcdc8cd44d17cd2f1542231) )
	ROM_LOAD16_BYTE( "epr-10440.29", 0x30001, 0x8000, CRC(23939508) SHA1(68450a18fc7e35f5b0155632aa68cffd251be38c) )
	ROM_LOAD16_BYTE( "epr-10444.30", 0x30000, 0x8000, CRC(82115823) SHA1(e4103003cda949bebe57815115a5028f4fe8e7d7) )


	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr-10434.12", 0x0000, 0x8000, CRC(77141cce) SHA1(6c5e83527f7e11a5ff5cc4fa75d55618a55e1a58) )

	ROM_REGION( 0x1000, REGION_CPU3, 0 )      /* 4k for 7751 onboard ROM */
	ROM_LOAD( "7751.bin",     0x0000, 0x0400, CRC(6a9534fc) SHA1(67ad94674db5c2aab75785668f610f6f4eccd158) ) /* 7751 - U34 */

	ROM_REGION( 0x10000, REGION_SOUND1, 0 ) /* 7751 sound data (not used yet) */
	ROM_LOAD( "epr-10435.1",  0x0000, 0x8000, CRC(ad89f6e3) SHA1(812a132142065b0fe13b5f0ac534b6d8830ba102) )
	ROM_LOAD( "epr-10436.2",  0x8000, 0x8000, CRC(96c76613) SHA1(fe3e4e649fd2cb2453eec0c92015bd54b3b9a1b5) )
ROM_END

ROM_START( alexkidd1 )
	ROM_REGION( 0x40000,  REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr-10429.43", 0x000000, 0x10000, CRC(bdf49eca) SHA1(899bc2d346544e4a33de51b60e02ebf7ee82cea8) )
	ROM_LOAD16_BYTE( "epr-10427.26", 0x000001, 0x10000, CRC(f6e3dd29) SHA1(bb94ebc062bb7c6c13b68579053b9cbe8b92417c) )
	ROM_LOAD16_BYTE( "epr-10430.42", 0x020000, 0x10000, CRC(89e3439f) SHA1(7c751bb477584842d93fda6686b03e289140bd62) )
	ROM_LOAD16_BYTE( "epr-10428.25", 0x020001, 0x10000, CRC(dbed3210) SHA1(1e2d22935a633641ff88967d67ec673ee25cbf55) )

	ROM_REGION( 0x18000,  REGION_GFX1, 0 ) /* tiles */
	ROM_LOAD( "epr-10431.95", 0x00000, 0x08000, CRC(a7962c39) SHA1(c816fc5d9f21b2ba32b9841e64b634bce7ea78c8) )
	ROM_LOAD( "epr-10432.94", 0x08000, 0x08000, CRC(db8cd24e) SHA1(656d98844ad9ccaa68e3f501137dddd0a27d999d) )
	ROM_LOAD( "epr-10433.93", 0x10000, 0x08000, CRC(e163c8c2) SHA1(ac54c5ecedca5b1a2c550de32687ca57c4d3a411) )

	ROM_REGION( 0x040000,  REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "epr-10437.10", 0x00001, 0x8000, CRC(522f7618) SHA1(9a6bc857dfef1dd1b7bffa034523c1c4cd8b3f4c) )
	ROM_LOAD16_BYTE( "epr-10441.11", 0x00000, 0x8000, CRC(74e3a35c) SHA1(26b980a0a3aee94ac38e0e0c7d305bb35a60d1c4) )
	ROM_LOAD16_BYTE( "epr-10438.17", 0x10001, 0x8000, CRC(738a6362) SHA1(a3c5f10c263cb216d275875f6333484a1cca281b) )
	ROM_LOAD16_BYTE( "epr-10442.18", 0x10000, 0x8000, CRC(86cb9c14) SHA1(42bd0ed985de61ff183eed0192257966caa01594) )
	ROM_LOAD16_BYTE( "epr-10439.23", 0x20001, 0x8000, CRC(b391aca7) SHA1(ca9d80b67e5365f709f90a5342b5e3aa7c7126e1) )
	ROM_LOAD16_BYTE( "epr-10443.24", 0x20000, 0x8000, CRC(95d32635) SHA1(788af2af1ae783128bcdc8cd44d17cd2f1542231) )
	ROM_LOAD16_BYTE( "epr-10440.29", 0x30001, 0x8000, CRC(23939508) SHA1(68450a18fc7e35f5b0155632aa68cffd251be38c) )
	ROM_LOAD16_BYTE( "epr-10444.30", 0x30000, 0x8000, CRC(82115823) SHA1(e4103003cda949bebe57815115a5028f4fe8e7d7) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr-10434.12", 0x0000, 0x8000, CRC(77141cce) SHA1(6c5e83527f7e11a5ff5cc4fa75d55618a55e1a58) )

	ROM_REGION( 0x1000, REGION_CPU3, 0 )      /* 4k for 7751 onboard ROM */
	ROM_LOAD( "7751.bin",     0x0000, 0x0400, CRC(6a9534fc) SHA1(67ad94674db5c2aab75785668f610f6f4eccd158) ) /* 7751 - U34 */

	ROM_REGION( 0x10000, REGION_SOUND1, 0 ) /* 7751 sound data (not used yet) */
	ROM_LOAD( "epr-10435.1",  0x0000, 0x8000, CRC(ad89f6e3) SHA1(812a132142065b0fe13b5f0ac534b6d8830ba102) )
	ROM_LOAD( "epr-10436.2",  0x8000, 0x8000, CRC(96c76613) SHA1(fe3e4e649fd2cb2453eec0c92015bd54b3b9a1b5) )

	ROM_REGION( 0x2000,  REGION_USER2, 0 ) /* decryption key */
	ROM_LOAD( "317-0021.key", 0x0000, 0x2000, BAD_DUMP CRC(504388a3) SHA1(20625e9e99c08a28b253676cc843ae5228ec9d5b) )
ROM_END

/*
	Action Fighter, Sega System 16A
	CPU: Protected FD1089
*/

ROM_START( afighter )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr-10350", 0x00001, 0x08000, CRC(f2cd6b3f) SHA1(380f75b8c1696b388179641866cd1d23f78664e7) )
	ROM_LOAD16_BYTE( "epr-10353", 0x00000, 0x08000, CRC(5a757dc9) SHA1(b0540844c8a09195f5d12312f8e27c334641d7b8) )
	ROM_LOAD16_BYTE( "epr-10349", 0x10001, 0x08000, CRC(4b434c37) SHA1(5f3afbdb9cdb0762e56b702a195274f30193b472) )
	ROM_LOAD16_BYTE( "epr-10352", 0x10000, 0x08000, CRC(f8abb143) SHA1(97e78291c15bdf95fd35adca6b9e002480137b12) )
	ROM_LOAD16_BYTE( "epr-10348", 0x20001, 0x08000, CRC(e51e3012) SHA1(bb5522aacb55b5f04aa4cb7a642e202f0ddd7c84) )
	ROM_LOAD16_BYTE( "epr-10351", 0x20000, 0x08000, CRC(ede21d8d) SHA1(b3e3944d706c606fd01e00d9511f020ce9aec9f0) )

	ROM_REGION( 0x30000, REGION_GFX1, 0 ) /* tiles */
	ROM_LOAD( "epr-10281.95", 0x00000, 0x10000, CRC(30e92cda) SHA1(36293a2a8a22dca5350571f19f3d5d04e1b27458) )
	ROM_LOAD( "epr-10282.94", 0x10000, 0x10000, CRC(b67b8910) SHA1(f3f029a3e6547114cec28e5cf8fda65ef434c353) )
	ROM_LOAD( "epr-10283.93", 0x20000, 0x10000, CRC(e7dbfd2d) SHA1(91bae3fbc4a3c612dc507eecfa8de1c2e1e7afee) )

	ROM_REGION( 0x40000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "epr-10285", 0x00001, 0x08000, CRC(98aa3d04) SHA1(1d26d17a72e55281e3444fee9c5af69ffb9e3c69) )
	ROM_LOAD16_BYTE( "epr-10286", 0x10001, 0x08000, CRC(8da050cf) SHA1(c28e8968dbd9c110672581f4486f70d5f45df7f5) )
	ROM_LOAD16_BYTE( "epr-10287", 0x20001, 0x08000, CRC(7989b74a) SHA1(a87acafe82b37a11d8f8b1f2ee4c9b2e1bb8161c) )
	ROM_LOAD16_BYTE( "epr-10288", 0x30001, 0x08000, CRC(d3ce551a) SHA1(0ff2170d9ef89058273025dd8d5e1021094adef1) )
	ROM_LOAD16_BYTE( "epr-10289", 0x00000, 0x08000, CRC(c59d1b98) SHA1(e232f2519234981c0e4ffecdd25c48083d9f93a8) )
	ROM_LOAD16_BYTE( "epr-10290", 0x10000, 0x08000, CRC(39354223) SHA1(d8a73d3f7fc2d83d23bb7434f43bc8804f35cc16) )
	ROM_LOAD16_BYTE( "epr-10291", 0x20000, 0x08000, CRC(6e4b245c) SHA1(1f8cecf7ea2d2dfa5ce18d7ee34b0da2cc40221e) )
	ROM_LOAD16_BYTE( "epr-10292", 0x30000, 0x08000, CRC(cef289a3) SHA1(7ab817b6348c168f79be325fb3cc2cca14ee0f8e) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr-10284.12", 0x00000, 0x8000, CRC(8ff09116) SHA1(8b99b6d2499897cfbd037a7e7cf5bc53bce8a63a) )

	ROM_REGION( 0x2000, REGION_USER2, 0 ) /* decryption key */
	ROM_LOAD( "317-0018.key", 0x0000, 0x2000, CRC(65b5b1af) SHA1(9a236c0c223064f9a2a56561e10b9ffed0f567a3) )
ROM_END

/*
	Action Fighter, Sega System 16A
	CPU: Unprotected
*/

ROM_START( afightera )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr10315.bin", 0x00001, 0x08000, CRC(2ea55eae) SHA1(91d582eaa0483d547d1a37dedb8f029a0fccb526) ) /* decrypted version of 10350 */
	ROM_LOAD16_BYTE( "epr10318.bin", 0x00000, 0x08000, CRC(d05b240d) SHA1(aa4aa7b18b53ac7b533855a44f9a4a0c14e1da2a) )
	ROM_LOAD16_BYTE( "epr10316.bin", 0x10001, 0x08000, CRC(07522474) SHA1(1a4bce4c04defab3516a90b9fa62fad1945a1176) ) /* == epr10294.bin */
	ROM_LOAD16_BYTE( "epr10319.bin", 0x10000, 0x08000, CRC(e48c4d80) SHA1(9c40972f81e004de84db4199e7dc0ceb6a998e76) ) /* == epr10297.bin */
	ROM_LOAD16_BYTE( "epr10317.bin", 0x20001, 0x08000, CRC(6e00db36) SHA1(5e3d03f7441515ad7dac411f492f2e159330a90a) )
	ROM_LOAD16_BYTE( "epr10320.bin", 0x20000, 0x08000, CRC(5f97c2fa) SHA1(5fef94bc275d9132dde8b72bfb661f10efdc91b1) )
	
	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "10281", 0x00000, 0x10000, CRC(30e92cda) SHA1(36293a2a8a22dca5350571f19f3d5d04e1b27458) )
	ROM_LOAD( "10282", 0x10000, 0x10000, CRC(b67b8910) SHA1(f3f029a3e6547114cec28e5cf8fda65ef434c353) )
	ROM_LOAD( "10283", 0x20000, 0x10000, CRC(e7dbfd2d) SHA1(91bae3fbc4a3c612dc507eecfa8de1c2e1e7afee) )

	ROM_REGION( 0x40000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "10285", 0x00001, 0x08000, CRC(98aa3d04) SHA1(1d26d17a72e55281e3444fee9c5af69ffb9e3c69) )
	ROM_LOAD16_BYTE( "10286", 0x10001, 0x08000, CRC(8da050cf) SHA1(c28e8968dbd9c110672581f4486f70d5f45df7f5) )
	ROM_LOAD16_BYTE( "10287", 0x20001, 0x08000, CRC(7989b74a) SHA1(a87acafe82b37a11d8f8b1f2ee4c9b2e1bb8161c) )
	ROM_LOAD16_BYTE( "10288", 0x30001, 0x08000, CRC(d3ce551a) SHA1(0ff2170d9ef89058273025dd8d5e1021094adef1) )
	ROM_LOAD16_BYTE( "10289", 0x00000, 0x08000, CRC(c59d1b98) SHA1(e232f2519234981c0e4ffecdd25c48083d9f93a8) )
	ROM_LOAD16_BYTE( "10290", 0x10000, 0x08000, CRC(39354223) SHA1(d8a73d3f7fc2d83d23bb7434f43bc8804f35cc16) )
	ROM_LOAD16_BYTE( "10291", 0x20000, 0x08000, CRC(6e4b245c) SHA1(1f8cecf7ea2d2dfa5ce18d7ee34b0da2cc40221e) )
	ROM_LOAD16_BYTE( "10292", 0x30000, 0x08000, CRC(cef289a3) SHA1(7ab817b6348c168f79be325fb3cc2cca14ee0f8e) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "10284", 0x00000, 0x8000, CRC(8ff09116) SHA1(8b99b6d2499897cfbd037a7e7cf5bc53bce8a63a) )
ROM_END

/***************************************************************************/

static READ16_HANDLER( alexkidd_skip_r ){
	if (activecpu_get_pc()==0x242c) {cpu_spinuntil_int(); return 0xffff;}
	return sys16_workingram[0x3108/2];
}

static MEMORY_READ16_START( alexkidd_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x400000, 0x40ffff, SYS16_MRA16_TILERAM },
	{ 0x410000, 0x410fff, SYS16_MRA16_TEXTRAM },
	{ 0x440000, 0x440fff, SYS16_MRA16_SPRITERAM },
	{ 0x840000, 0x840fff, SYS16_MRA16_PALETTERAM },
	{ 0xc40002, 0xc40005, MRA16_NOP },		/*?? */
	{ 0xc41002, 0xc41003, input_port_0_word_r }, /* player1 */
	{ 0xc41006, 0xc41007, input_port_1_word_r }, /* player2 */
	{ 0xc41000, 0xc41001, input_port_2_word_r }, /* service */
	{ 0xc42000, 0xc42001, input_port_3_word_r }, /* dip1 */
	{ 0xc42002, 0xc42003, input_port_4_word_r }, /* dip2 */
	{ 0xc60000, 0xc60001, MRA16_NOP },
	{ 0xfff108, 0xfff109, alexkidd_skip_r },
	{ 0xffc000, 0xffffff, SYS16_MRA16_WORKINGRAM },
MEMORY_END

static MEMORY_WRITE16_START( alexkidd_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x410000, 0x410fff, SYS16_MWA16_TEXTRAM, &sys16_textram },
	{ 0x400000, 0x40ffff, SYS16_MWA16_TILERAM, &sys16_tileram },
	{ 0x440000, 0x440fff, SYS16_MWA16_SPRITERAM, &sys16_spriteram },
	{ 0x840000, 0x840fff, SYS16_MWA16_PALETTERAM, &paletteram16 },
	{ 0xc40000, 0xc40001, sound_command_nmi_w },
	{ 0xc40002, 0xc40005, MWA16_NOP },		/*?? */
	{ 0xffc000, 0xffffff, SYS16_MWA16_WORKINGRAM, &sys16_workingram },
MEMORY_END

static MEMORY_READ16_START( afighter_readmem )
	{ 0x000000, 0x05ffff, MRA16_ROM },
	{ 0x400000, 0x40ffff, SYS16_MRA16_TILERAM },
	{ 0x410000, 0x410fff, SYS16_MRA16_TEXTRAM },
	{ 0x440000, 0x440fff, SYS16_MRA16_SPRITERAM },
	{ 0x840000, 0x840fff, SYS16_MRA16_PALETTERAM },
	{ 0xc40002, 0xc40005, MRA16_NOP },		/*?? */
	{ 0xc41002, 0xc41003, input_port_0_word_r }, /* player1 */
	{ 0xc41006, 0xc41007, input_port_1_word_r }, /* player2 */
	{ 0xc41000, 0xc41001, input_port_2_word_r }, /* service */
	{ 0xc42000, 0xc42001, input_port_3_word_r }, /* dip1 */
	{ 0xc42002, 0xc42003, input_port_4_word_r }, /* dip2 */
	{ 0xc60000, 0xc60001, MRA16_NOP },
	{ 0xffc000, 0xffffff, SYS16_MRA16_WORKINGRAM },
MEMORY_END

static MEMORY_WRITE16_START( afighter_writemem )
	{ 0x000000, 0x05ffff, MWA16_ROM },
	{ 0x410000, 0x410fff, SYS16_MWA16_TEXTRAM, &sys16_textram },
	{ 0x400000, 0x40ffff, SYS16_MWA16_TILERAM, &sys16_tileram },
	{ 0x440000, 0x440fff, SYS16_MWA16_SPRITERAM, &sys16_spriteram },
	{ 0x840000, 0x840fff, SYS16_MWA16_PALETTERAM, &paletteram16 },
	{ 0xc40000, 0xc40001, sound_command_nmi_w },
	{ 0xc40002, 0xc40005, MWA16_NOP },		/*?? */
	{ 0xffc000, 0xffffff, SYS16_MWA16_WORKINGRAM, &sys16_workingram },
MEMORY_END

/***************************************************************************/

static MACHINE_INIT( alexkidd ){
	sys16_textmode=1;
	sys16_spritesystem = sys16_sprite_quartet2;
	sys16_sprxoffset = -0xbd;
	sys16_fgxoffset = sys16_bgxoffset = 8;
	sys16_bg_priority_mode=1;

	sys16_update_proc = type1_sys16_textram;
}

static MACHINE_INIT( afighter ){
	sys16_textmode=1;
	sys16_spritesystem = sys16_sprite_quartet2;
	sys16_sprxoffset = -0xbd;
	sys16_fgxoffset = sys16_bgxoffset = 8;
	sys16_bg_priority_mode=1;
	sys16_fg_priority_mode=4;

	sys16_tilebank_switch=0x2000; /* fixes bg tiles */

	sys16_update_proc = type1_sys16_textram;
}

/***************************************************************************/

INPUT_PORTS_START( alexkidd )
	SYS16_JOY1_SWAPPEDBUTTONS
	SYS16_JOY2_SWAPPEDBUTTONS
	SYS16_SERVICE
	SYS16_COINAGE

PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x00, "Continues" )
	PORT_DIPSETTING(    0x01, "Only before level 5" )
	PORT_DIPSETTING(    0x00, "Unlimited" )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "240", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x20, "10000" )
	PORT_DIPSETTING(    0x30, "20000" )
	PORT_DIPSETTING(    0x10, "40000" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0xc0, 0xc0, "Time Adjust" )
	PORT_DIPSETTING(    0x80, "70" )
	PORT_DIPSETTING(    0xc0, "60" )
	PORT_DIPSETTING(    0x40, "50" )
	PORT_DIPSETTING(    0x00, "40" )

INPUT_PORTS_END

INPUT_PORTS_START( afighter )
	SYS16_JOY1
	SYS16_JOY2
	SYS16_SERVICE
	SYS16_COINAGE

PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x00, "Infinite" )
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
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x30, "10000 - 20000" )
	PORT_DIPSETTING(    0x20, "20000 - 40000" )
	PORT_DIPSETTING(    0x10, "30000 - 60000" )
	PORT_DIPSETTING(    0x00, "40000 - 80000" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x80, 0x80, "Allow_Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Yes ) )
INPUT_PORTS_END

/***************************************************************************/

static MACHINE_DRIVER_START( alexkidd )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16_7751)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(alexkidd_readmem,alexkidd_writemem)

	MDRV_MACHINE_INIT(alexkidd)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( afighter )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(afighter_readmem,afighter_writemem)

	MDRV_MACHINE_INIT(afighter)
MACHINE_DRIVER_END

/***************************************************************************/
/* sys16B */
ROM_START( aliensyn )
	ROM_REGION( 0x030000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "11083.a4", 0x00000, 0x8000, CRC(cb2ad9b3) SHA1(66284b4e1071b3cb4b36960a2dba8949325f9e86) )
	ROM_LOAD16_BYTE( "11080.a1", 0x00001, 0x8000, CRC(fe7378d9) SHA1(acfee79e5fe9fdf95926562c02599e96e96f76b5) )
	ROM_LOAD16_BYTE( "11084.a5", 0x10000, 0x8000, CRC(2e1ec7b1) SHA1(666a9e402d7d02d74c0a2468853a3843b161c1a2) )
	ROM_LOAD16_BYTE( "11081.a2", 0x10001, 0x8000, CRC(1308ee63) SHA1(36a4c8cf3a310398e264c57c7c6f53c05dbc0108) )
	ROM_LOAD16_BYTE( "11085.a6", 0x20000, 0x8000, CRC(cff78f39) SHA1(78431ce6c03232150a8db15499da9371977b570b) )
	ROM_LOAD16_BYTE( "11082.a3", 0x20001, 0x8000, CRC(9cdc2a14) SHA1(c06693f45675e31d591703c27bb3f6ec02fc1215) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "10702.b9",  0x00000, 0x10000, CRC(393bc813) SHA1(672782c8fb2a6e454b27e073acc26130cacf3e50) )
	ROM_LOAD( "10703.b10", 0x10000, 0x10000, CRC(6b6dd9f5) SHA1(964409c6630caa13caf7d644d0c6fb071860b61b) )
	ROM_LOAD( "10704.b11", 0x20000, 0x10000, CRC(911e7ebc) SHA1(f03d3d3a09d19f7b705f9cb29f73140a3073463f) )

	ROM_REGION( 0x080000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "10709.b1", 0x00001, 0x10000, CRC(addf0a90) SHA1(a92c9531f1817763773471ce63f566b9e88360a0) )
	ROM_LOAD16_BYTE( "10713.b5", 0x00000, 0x10000, CRC(ececde3a) SHA1(9c12d4665179bf433c42f5ddc8a043ad592aa90e) )
	ROM_LOAD16_BYTE( "10710.b2", 0x20001, 0x10000, CRC(992369eb) SHA1(c6796acf6807e9ba4c3d241903653f91adf4764e) )
	ROM_LOAD16_BYTE( "10714.b6", 0x20000, 0x10000, CRC(91bf42fb) SHA1(4b9d3e97768323dee01e92378adafecb26bcc094) )
	ROM_LOAD16_BYTE( "10711.b3", 0x40001, 0x10000, CRC(29166ef6) SHA1(99a7cfd7d811537c821412a320beadb5a9c09af3) )
	ROM_LOAD16_BYTE( "10715.b7", 0x40000, 0x10000, CRC(a7c57384) SHA1(46f8efa691d7bbb0a18119c0ff12cff7c0d129e1) )
	ROM_LOAD16_BYTE( "10712.b4", 0x60001, 0x10000, CRC(876ad019) SHA1(39973ddb5a5746e0e094c759447bff1130c72c84) )
	ROM_LOAD16_BYTE( "10716.b8", 0x60000, 0x10000, CRC(40ba1d48) SHA1(e2d4d2689bb9b9bdc85e7f72a6665e5fd4c583aa) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr-10723.a7",  0x00000, 0x8000, CRC(99953526) SHA1(4a980370923fd5d3dc9e25d42a032c9e78c7ff47) )
	ROM_LOAD( "epr-10724.a8",  0x10000, 0x8000, CRC(f971a817) SHA1(502c95638e4fd5f87e5fc837cb44b39a5d62f4e4) )
	ROM_LOAD( "epr-10725.a9",  0x20000, 0x8000, CRC(6a50e08f) SHA1(d34b2ccadb8b07d5ad99cab5c5b5b79642c65574) )
	ROM_LOAD( "epr-10726.a10", 0x30000, 0x8000, CRC(d50b7736) SHA1(b1f8e3b0cf2ffee5382098100cfabe21b383cd51) )
  ROM_END

/* sys16A - use a different sound chip? */
ROM_START( aliensya )
	ROM_REGION( 0x030000, REGION_CPU1, 0 ) /* 68000 code. I guessing the order a bit here */
	ROM_LOAD16_BYTE( "10808", 0x00000, 0x8000, CRC(e669929f) SHA1(b5ab41d6f31f0369f8c5f5eb6fc08e8c23312b96) )
	ROM_LOAD16_BYTE( "10806", 0x00001, 0x8000, CRC(9f7f8fdd) SHA1(819e9c491b7d23deaef646d37319c38e75827d68) )
	ROM_LOAD16_BYTE( "10809", 0x10000, 0x8000, CRC(9a424919) SHA1(a7be5d9bed329099df10ff5a0104cb832485bd0a) )
	ROM_LOAD16_BYTE( "10807", 0x10001, 0x8000, CRC(3d2c3530) SHA1(567ed45c84b1d3d92371c4ad33fdb28f68cf29a3) )
	ROM_LOAD16_BYTE( "10701", 0x20000, 0x8000, CRC(92171751) SHA1(69a282c01db7224f32386a6db25309e09e29a112) )
	ROM_LOAD16_BYTE( "10698", 0x20001, 0x8000, CRC(c1e4fdc0) SHA1(65817a9336f7887d2bf14485bdff8352c960d2ab) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "10739", 0x00000, 0x10000, CRC(a29ec207) SHA1(c469d2689a7bdc2a59dfff56ce13d34e9fbac263) )
	ROM_LOAD( "10740", 0x10000, 0x10000, CRC(47f93015) SHA1(68247a6bffd1d4d1c450148dd46214d01ce1c668) )
	ROM_LOAD( "10741", 0x20000, 0x10000, CRC(4970739c) SHA1(5bdf4222209ec46e0015bfc0f90578dd9b30bdd1) )

	ROM_REGION( 0x080000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "10709.b1", 0x00001, 0x10000, CRC(addf0a90) SHA1(a92c9531f1817763773471ce63f566b9e88360a0) )
	ROM_LOAD16_BYTE( "10713.b5", 0x00000, 0x10000, CRC(ececde3a) SHA1(9c12d4665179bf433c42f5ddc8a043ad592aa90e) )
	ROM_LOAD16_BYTE( "10710.b2", 0x20001, 0x10000, CRC(992369eb) SHA1(c6796acf6807e9ba4c3d241903653f91adf4764e) )
	ROM_LOAD16_BYTE( "10714.b6", 0x20000, 0x10000, CRC(91bf42fb) SHA1(4b9d3e97768323dee01e92378adafecb26bcc094) )
	ROM_LOAD16_BYTE( "10711.b3", 0x40001, 0x10000, CRC(29166ef6) SHA1(99a7cfd7d811537c821412a320beadb5a9c09af3) )
	ROM_LOAD16_BYTE( "10715.b7", 0x40000, 0x10000, CRC(a7c57384) SHA1(46f8efa691d7bbb0a18119c0ff12cff7c0d129e1) )
	ROM_LOAD16_BYTE( "10712.b4", 0x60001, 0x10000, CRC(876ad019) SHA1(39973ddb5a5746e0e094c759447bff1130c72c84) )
	ROM_LOAD16_BYTE( "10716.b8", 0x60000, 0x10000, CRC(40ba1d48) SHA1(e2d4d2689bb9b9bdc85e7f72a6665e5fd4c583aa) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "10705", 0x00000, 0x8000, CRC(777b749e) SHA1(086b03100064a98228f95db7962b2671121c46ea) )
	ROM_LOAD( "10706", 0x10000, 0x8000, CRC(aa114acc) SHA1(81a2b3586ae90bc7fc55b82478ffe182ac49983e) )
	ROM_LOAD( "10707", 0x18000, 0x8000, CRC(800c1d82) SHA1(aac4123bd35f87da09264649f4cf8326b2ba3cb8) )
	ROM_LOAD( "10708", 0x20000, 0x8000, CRC(5921ef52) SHA1(eff9978361692e6e60a9c6caf5740dd6182cfe4a) )
ROM_END

ROM_START( aliensyj )
	ROM_REGION( 0x030000, REGION_CPU1, 0 ) /* Custom 68000 code . I guessing the order a bit here */
/* custom cpu 317-0033 */
	ROM_LOAD16_BYTE( "epr10699.43", 0x00000, 0x8000, CRC(3fd38d17) SHA1(538c1246121051a1af9ba2a4259eb1fe7e4952e1) )
	ROM_LOAD16_BYTE( "epr10696.26", 0x00001, 0x8000, CRC(d734f19f) SHA1(4a08c35084f7a9364ba0f058b9a9ffc30c8b5a78) )
	ROM_LOAD16_BYTE( "epr10700.42", 0x10000, 0x8000, CRC(3b04b252) SHA1(0e40e89e8feb7c98ee1da1c3fb3fe1d317c66842) )
	ROM_LOAD16_BYTE( "epr10697.25", 0x10001, 0x8000, CRC(f2bc123d) SHA1(7848529342495289e2d4f865767f3649cd85993b) )
	ROM_LOAD16_BYTE( "10701", 0x20000, 0x8000, CRC(92171751) SHA1(69a282c01db7224f32386a6db25309e09e29a112) )
	ROM_LOAD16_BYTE( "10698", 0x20001, 0x8000, CRC(c1e4fdc0) SHA1(65817a9336f7887d2bf14485bdff8352c960d2ab) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "10739", 0x00000, 0x10000, CRC(a29ec207) SHA1(c469d2689a7bdc2a59dfff56ce13d34e9fbac263) )
	ROM_LOAD( "10740", 0x10000, 0x10000, CRC(47f93015) SHA1(68247a6bffd1d4d1c450148dd46214d01ce1c668) )
	ROM_LOAD( "10741", 0x20000, 0x10000, CRC(4970739c) SHA1(5bdf4222209ec46e0015bfc0f90578dd9b30bdd1) )

	ROM_REGION( 0x080000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "10709.b1", 0x00001, 0x10000, CRC(addf0a90) SHA1(a92c9531f1817763773471ce63f566b9e88360a0) )
	ROM_LOAD16_BYTE( "10713.b5", 0x00000, 0x10000, CRC(ececde3a) SHA1(9c12d4665179bf433c42f5ddc8a043ad592aa90e) )
	ROM_LOAD16_BYTE( "10710.b2", 0x20001, 0x10000, CRC(992369eb) SHA1(c6796acf6807e9ba4c3d241903653f91adf4764e) )
	ROM_LOAD16_BYTE( "10714.b6", 0x20000, 0x10000, CRC(91bf42fb) SHA1(4b9d3e97768323dee01e92378adafecb26bcc094) )
	ROM_LOAD16_BYTE( "10711.b3", 0x40001, 0x10000, CRC(29166ef6) SHA1(99a7cfd7d811537c821412a320beadb5a9c09af3) )
	ROM_LOAD16_BYTE( "10715.b7", 0x40000, 0x10000, CRC(a7c57384) SHA1(46f8efa691d7bbb0a18119c0ff12cff7c0d129e1) )
	ROM_LOAD16_BYTE( "10712.b4", 0x60001, 0x10000, CRC(876ad019) SHA1(39973ddb5a5746e0e094c759447bff1130c72c84) )
	ROM_LOAD16_BYTE( "10716.b8", 0x60000, 0x10000, CRC(40ba1d48) SHA1(e2d4d2689bb9b9bdc85e7f72a6665e5fd4c583aa) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "10705", 0x00000, 0x8000, CRC(777b749e) SHA1(086b03100064a98228f95db7962b2671121c46ea) )
	ROM_LOAD( "10706", 0x10000, 0x8000, CRC(aa114acc) SHA1(81a2b3586ae90bc7fc55b82478ffe182ac49983e) )
	ROM_LOAD( "10707", 0x18000, 0x8000, CRC(800c1d82) SHA1(aac4123bd35f87da09264649f4cf8326b2ba3cb8) )
	ROM_LOAD( "10708", 0x20000, 0x8000, CRC(5921ef52) SHA1(eff9978361692e6e60a9c6caf5740dd6182cfe4a) )
ROM_END


ROM_START( aliensyb )
	ROM_REGION( 0x030000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "as_typeb.a4", 0x00000, 0x8000, CRC(17bf5304) SHA1(f6318c6c4a606d21ba43354229b60a51d8a3baa6) )
	ROM_LOAD16_BYTE( "as_typeb.a1", 0x00001, 0x8000, CRC(4cd134df) SHA1(541377bd6eba280d7f0367694032891989762485) )
	ROM_LOAD16_BYTE( "as_typeb.a5", 0x10000, 0x8000, CRC(c8b791b0) SHA1(7a83a6781ed5b43583d86d4ee5fb3120a650874b) )
	ROM_LOAD16_BYTE( "as_typeb.a2", 0x10001, 0x8000, CRC(bdcf4a30) SHA1(e11264999f15cef7c35b6569407bb3dd8e2dd236) )
	ROM_LOAD16_BYTE( "as_typeb.a6", 0x20000, 0x8000, CRC(1d0790aa) SHA1(c141b12aa7e4f86b96eabeb3f827ee26ddacde34) )
	ROM_LOAD16_BYTE( "as_typeb.a3", 0x20001, 0x8000, CRC(1e7586b7) SHA1(be4c2c03119aee1b8f26f3dd79c99ce431a43b28) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "10702.b9",  0x00000, 0x10000, CRC(393bc813) SHA1(672782c8fb2a6e454b27e073acc26130cacf3e50) )
	ROM_LOAD( "10703.b10", 0x10000, 0x10000, CRC(6b6dd9f5) SHA1(964409c6630caa13caf7d644d0c6fb071860b61b) )
	ROM_LOAD( "10704.b11", 0x20000, 0x10000, CRC(911e7ebc) SHA1(f03d3d3a09d19f7b705f9cb29f73140a3073463f) )

	ROM_REGION( 0x080000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "10709.b1", 0x00001, 0x10000, CRC(addf0a90) SHA1(a92c9531f1817763773471ce63f566b9e88360a0) )
	ROM_LOAD16_BYTE( "10713.b5", 0x00000, 0x10000, CRC(ececde3a) SHA1(9c12d4665179bf433c42f5ddc8a043ad592aa90e) )
	ROM_LOAD16_BYTE( "10710.b2", 0x20001, 0x10000, CRC(992369eb) SHA1(c6796acf6807e9ba4c3d241903653f91adf4764e) )
	ROM_LOAD16_BYTE( "10714.b6", 0x20000, 0x10000, CRC(91bf42fb) SHA1(4b9d3e97768323dee01e92378adafecb26bcc094) )
	ROM_LOAD16_BYTE( "10711.b3", 0x40001, 0x10000, CRC(29166ef6) SHA1(99a7cfd7d811537c821412a320beadb5a9c09af3) )
	ROM_LOAD16_BYTE( "10715.b7", 0x40000, 0x10000, CRC(a7c57384) SHA1(46f8efa691d7bbb0a18119c0ff12cff7c0d129e1) )
	ROM_LOAD16_BYTE( "10712.b4", 0x60001, 0x10000, CRC(876ad019) SHA1(39973ddb5a5746e0e094c759447bff1130c72c84) )
	ROM_LOAD16_BYTE( "10716.b8", 0x60000, 0x10000, CRC(40ba1d48) SHA1(e2d4d2689bb9b9bdc85e7f72a6665e5fd4c583aa) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr-10723.a7",  0x00000, 0x8000, CRC(99953526) SHA1(4a980370923fd5d3dc9e25d42a032c9e78c7ff47) )
	ROM_LOAD( "epr-10724.a8",  0x10000, 0x8000, CRC(f971a817) SHA1(502c95638e4fd5f87e5fc837cb44b39a5d62f4e4) )
	ROM_LOAD( "epr-10725.a9",  0x20000, 0x8000, CRC(6a50e08f) SHA1(d34b2ccadb8b07d5ad99cab5c5b5b79642c65574) )
	ROM_LOAD( "epr-10726.a10", 0x30000, 0x8000, CRC(d50b7736) SHA1(b1f8e3b0cf2ffee5382098100cfabe21b383cd51) )
ROM_END

/***************************************************************************/

static MEMORY_READ16_START( aliensyn_readmem )
	{ 0x000000, 0x02ffff, MRA16_ROM },
	{ 0x400000, 0x40ffff, SYS16_MRA16_TILERAM },
	{ 0x410000, 0x410fff, SYS16_MRA16_TEXTRAM },
	{ 0x440000, 0x440fff, SYS16_MRA16_SPRITERAM },
	{ 0x840000, 0x840fff, SYS16_MRA16_PALETTERAM },
	{ 0xc41002, 0xc41003, input_port_0_word_r }, /* player1 */
	{ 0xc41006, 0xc41007, input_port_1_word_r }, /* player2 */
	{ 0xc41000, 0xc41001, input_port_2_word_r }, /* service */
	{ 0xc42002, 0xc42003, input_port_3_word_r }, /* dip1 */
	{ 0xc42000, 0xc42001, input_port_4_word_r }, /* dip2 */
	{ 0xffc000, 0xffffff, SYS16_MRA16_WORKINGRAM },
MEMORY_END

static MEMORY_WRITE16_START( aliensyn_writemem )
	{ 0x000000, 0x02ffff, MWA16_ROM },
	{ 0x400000, 0x40ffff, SYS16_MWA16_TILERAM, &sys16_tileram },
	{ 0x410000, 0x410fff, SYS16_MWA16_TEXTRAM, &sys16_textram },
	{ 0x440000, 0x440fff, SYS16_MWA16_SPRITERAM, &sys16_spriteram },
	{ 0x840000, 0x840fff, SYS16_MWA16_PALETTERAM, &paletteram16 },
	{ 0xc00006, 0xc00007, sound_command_w },
	{ 0xc00020, 0xc0003f, MWA16_NOP }, /* config regs */
	{ 0xc40000, 0xc40001, sys16_coinctrl_w },
	{ 0xffc000, 0xffffff, SYS16_MWA16_WORKINGRAM, &sys16_workingram },
MEMORY_END

/***************************************************************************/

static MACHINE_INIT( aliensyn ){
	static int bank[16] = {
		0,0,0,0,
		0,0,0,3,
		0,0,0,2,
		0,1,0,0
	};
	sys16_obj_bank = bank;

	sys16_bg_priority_mode=1;
	sys16_fg_priority_mode=1;

	sys16_update_proc = type0_sys16_textram;

	sys16_soundbanktype = 1;
}

static DRIVER_INIT( aliensyn )
{
	sys16_bg1_trans=1;
}

static DRIVER_INIT( alexkida )
{
	fd1089a_decrypt();
}
/***************************************************************************/

INPUT_PORTS_START( aliensyn )
	SYS16_JOY1
	SYS16_JOY2
	SYS16_SERVICE
	SYS16_COINAGE

PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "127", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x30, 0x30, "Timer" )
	PORT_DIPSETTING(    0x00, "120" )
	PORT_DIPSETTING(    0x10, "130" )
	PORT_DIPSETTING(    0x20, "140" )
	PORT_DIPSETTING(    0x30, "150" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x80, "Easy" )
	PORT_DIPSETTING(    0xc0, "Normal" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
INPUT_PORTS_END

/****************************************************************************/

static MACHINE_DRIVER_START( aliensyn )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16_7759b)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(aliensyn_readmem,aliensyn_writemem)

	MDRV_MACHINE_INIT(aliensyn)
MACHINE_DRIVER_END

/***************************************************************************/
/* sys16B */
ROM_START( altbeast )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "11705", 0x000000, 0x20000, CRC(57dc5c7a) SHA1(a5cc9b10a00778f5163fc915b956fa5d0d7a37ce) )
	ROM_LOAD16_BYTE( "11704", 0x000001, 0x20000, CRC(33bbcf07) SHA1(534e5426580dbf72509dceb762b8b99766d3a739) )

	ROM_REGION( 0x60000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "11674", 0x00000, 0x20000, CRC(a57a66d5) SHA1(5103583d48997abad12a0c5fee26431c486ced52) )
	ROM_LOAD( "11675", 0x20000, 0x20000, CRC(2ef2f144) SHA1(38d22d609db2d9b6067b5d12f6499436de4605cb) )
	ROM_LOAD( "11676", 0x40000, 0x20000, CRC(0c04acac) SHA1(87fe2a0dd9913f9550e9b4cbc7e7465b61640e07) )

	ROM_REGION( 0x100000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "epr11677.b1", 0x00001, 0x20000, CRC(a01425cd) SHA1(72be5ec29e476601f9bf6aaedef9b73cedeb42f0) )
	ROM_LOAD16_BYTE( "epr11681.b5", 0x00000, 0x20000, CRC(d9e03363) SHA1(995a7c6a8f0c61468b57a3bb407461a2a3ae8adc) )
	ROM_LOAD16_BYTE( "epr11678.b2", 0x40001, 0x20000, CRC(17a9fc53) SHA1(85a9a605742ae5aab86db37189b9ee4d54c70e56) )
	ROM_LOAD16_BYTE( "epr11682.b6", 0x40000, 0x20000, CRC(e3f77c5e) SHA1(6b3cb7918ab0c7c97a51cc5ea19ced3374ff3914) )
	ROM_LOAD16_BYTE( "epr11679.b3", 0x80001, 0x20000, CRC(14dcc245) SHA1(1ca1b6e0f2b7bedad2a8ab70f52da8c54d40d3cf) )
	ROM_LOAD16_BYTE( "epr11683.b7", 0x80000, 0x20000, CRC(f9a60f06) SHA1(0cffcfdb02733feaa869198b7e668c58b47c321a) )
	ROM_LOAD16_BYTE( "epr11680.b4", 0xc0001, 0x20000, CRC(f43dcdec) SHA1(2941500cf33afca487f19f2329033d5d17aad826) )
	ROM_LOAD16_BYTE( "epr11684.b8", 0xc0000, 0x20000, CRC(b20c0edb) SHA1(6c8694d05e3adac37c9015037ab800233371db36) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "11671",		 0x00000, 0x08000, CRC(2b71343b) SHA1(8a657f787de2b9d5161ed2c109642a148348af09) )
	ROM_LOAD( "opr11672",    0x10000, 0x20000, CRC(bbd7f460) SHA1(bbc5c2219cb3a827d84062b19affd9780da2a3cf) )
	ROM_LOAD( "opr11673",    0x30000, 0x20000, CRC(400c4a36) SHA1(de4bdfa91734410e0a7f6a16bf8336db172f458a) )
ROM_END

ROM_START( jyuohki )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* Custom 68000 code. */
/* custom cpu 317-0065 */
	ROM_LOAD16_BYTE( "epr11670.a7", 0x000000, 0x20000, CRC(b748eb07) SHA1(f3663831610bcb358340f14c2c96833dd7591bfb) )
	ROM_LOAD16_BYTE( "epr11669.a5", 0x000001, 0x20000, CRC(005ecd11) SHA1(c392195955cf727752f03db92414701cc2bf1f4a) )

	ROM_REGION( 0x60000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "11674", 0x00000, 0x20000, CRC(a57a66d5) SHA1(5103583d48997abad12a0c5fee26431c486ced52) )
	ROM_LOAD( "11675", 0x20000, 0x20000, CRC(2ef2f144) SHA1(38d22d609db2d9b6067b5d12f6499436de4605cb) )
	ROM_LOAD( "11676", 0x40000, 0x20000, CRC(0c04acac) SHA1(87fe2a0dd9913f9550e9b4cbc7e7465b61640e07) )

	ROM_REGION( 0x100000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "epr11677.b1", 0x00001, 0x20000, CRC(a01425cd) SHA1(72be5ec29e476601f9bf6aaedef9b73cedeb42f0) )
	ROM_LOAD16_BYTE( "epr11681.b5", 0x00000, 0x20000, CRC(d9e03363) SHA1(995a7c6a8f0c61468b57a3bb407461a2a3ae8adc) )
	ROM_LOAD16_BYTE( "epr11678.b2", 0x40001, 0x20000, CRC(17a9fc53) SHA1(85a9a605742ae5aab86db37189b9ee4d54c70e56) )
	ROM_LOAD16_BYTE( "epr11682.b6", 0x40000, 0x20000, CRC(e3f77c5e) SHA1(6b3cb7918ab0c7c97a51cc5ea19ced3374ff3914) )
	ROM_LOAD16_BYTE( "epr11679.b3", 0x80001, 0x20000, CRC(14dcc245) SHA1(1ca1b6e0f2b7bedad2a8ab70f52da8c54d40d3cf) )
	ROM_LOAD16_BYTE( "epr11683.b7", 0x80000, 0x20000, CRC(f9a60f06) SHA1(0cffcfdb02733feaa869198b7e668c58b47c321a) )
	ROM_LOAD16_BYTE( "epr11680.b4", 0xc0001, 0x20000, CRC(f43dcdec) SHA1(2941500cf33afca487f19f2329033d5d17aad826) )
	ROM_LOAD16_BYTE( "epr11684.b8", 0xc0000, 0x20000, CRC(b20c0edb) SHA1(6c8694d05e3adac37c9015037ab800233371db36) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "11671",		 0x00000, 0x08000, CRC(2b71343b) SHA1(8a657f787de2b9d5161ed2c109642a148348af09) )
	ROM_LOAD( "opr11672",    0x10000, 0x20000, CRC(bbd7f460) SHA1(bbc5c2219cb3a827d84062b19affd9780da2a3cf) )
	ROM_LOAD( "opr11673",    0x30000, 0x20000, CRC(400c4a36) SHA1(de4bdfa91734410e0a7f6a16bf8336db172f458a) )
ROM_END

/* sys16B */
ROM_START( altbeas2 )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr11740", 0x000000, 0x20000, CRC(ce227542) SHA1(54adcc1246943360bb68498e971809a0d4f9fe0c) )
	ROM_LOAD16_BYTE( "epr11739", 0x000001, 0x20000, CRC(e466eb65) SHA1(c9bb57818eb81a43abdf2ad2a79a0bd45c25d208) )

	ROM_REGION( 0x60000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "11674", 0x00000, 0x20000, CRC(a57a66d5) SHA1(5103583d48997abad12a0c5fee26431c486ced52) )
	ROM_LOAD( "11675", 0x20000, 0x20000, CRC(2ef2f144) SHA1(38d22d609db2d9b6067b5d12f6499436de4605cb) )
	ROM_LOAD( "11676", 0x40000, 0x20000, CRC(0c04acac) SHA1(87fe2a0dd9913f9550e9b4cbc7e7465b61640e07) )

	ROM_REGION( 0x100000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "epr11677.b1", 0x00001, 0x20000, CRC(a01425cd) SHA1(72be5ec29e476601f9bf6aaedef9b73cedeb42f0) )
	ROM_LOAD16_BYTE( "epr11681.b5", 0x00000, 0x20000, CRC(d9e03363) SHA1(995a7c6a8f0c61468b57a3bb407461a2a3ae8adc) )
	ROM_LOAD16_BYTE( "epr11678.b2", 0x40001, 0x20000, CRC(17a9fc53) SHA1(85a9a605742ae5aab86db37189b9ee4d54c70e56) )
	ROM_LOAD16_BYTE( "epr11682.b6", 0x40000, 0x20000, CRC(e3f77c5e) SHA1(6b3cb7918ab0c7c97a51cc5ea19ced3374ff3914) )
	ROM_LOAD16_BYTE( "epr11679.b3", 0x80001, 0x20000, CRC(14dcc245) SHA1(1ca1b6e0f2b7bedad2a8ab70f52da8c54d40d3cf) )
	ROM_LOAD16_BYTE( "epr11683.b7", 0x80000, 0x20000, CRC(f9a60f06) SHA1(0cffcfdb02733feaa869198b7e668c58b47c321a) )
	ROM_LOAD16_BYTE( "epr11680.b4", 0xc0001, 0x20000, CRC(f43dcdec) SHA1(2941500cf33afca487f19f2329033d5d17aad826) )
	ROM_LOAD16_BYTE( "epr11684.b8", 0xc0000, 0x20000, CRC(b20c0edb) SHA1(6c8694d05e3adac37c9015037ab800233371db36) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "opr11686",	 0x00000, 0x08000, CRC(828a45b3) SHA1(df921701b411afac1b6716b6798a1bffc2180133) )	/* ??? */
	ROM_LOAD( "opr11672",    0x10000, 0x20000, CRC(bbd7f460) SHA1(bbc5c2219cb3a827d84062b19affd9780da2a3cf) )
	ROM_LOAD( "opr11673",    0x30000, 0x20000, CRC(400c4a36) SHA1(de4bdfa91734410e0a7f6a16bf8336db172f458a) )
ROM_END

/***************************************************************************/

static READ16_HANDLER( altbeast_skip_r )
{
	if (activecpu_get_pc()==0x3994) {cpu_spinuntil_int(); return 1<<8;}
	return sys16_workingram[0x301c/2];
}

/* ??? What is this, input test shows 4 bits to each player, but what does it do? */
static READ16_HANDLER( altbeast_io_r )
{
	return 0xff;
}

static MEMORY_READ16_START( altbeast_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x400000, 0x40ffff, SYS16_MRA16_TILERAM },
	{ 0x410000, 0x410fff, SYS16_MRA16_TEXTRAM },
	{ 0x440000, 0x440fff, SYS16_MRA16_SPRITERAM },
	{ 0x840000, 0x840fff, SYS16_MRA16_PALETTERAM },
	{ 0xc40000, 0xc40001, MRA16_NOP }, /* write-only register, game does bclr #6, $C40001 */
	{ 0xc41002, 0xc41003, input_port_0_word_r }, /* player1 */
	{ 0xc41006, 0xc41007, input_port_1_word_r }, /* player2 */
	{ 0xc41004, 0xc41005, altbeast_io_r },
	{ 0xc41000, 0xc41001, input_port_2_word_r }, /* service */
	{ 0xc42002, 0xc42003, input_port_3_word_r }, /* dip1 */
	{ 0xc42000, 0xc42001, input_port_4_word_r }, /* dip2 */
	{ 0xfff01c, 0xfff01d, altbeast_skip_r },
	{ 0xffc000, 0xffffff, SYS16_MRA16_WORKINGRAM },
MEMORY_END

static MEMORY_WRITE16_START( altbeast_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x3f0000, 0x3fffff, sys16_tilebank_w },
	{ 0x400000, 0x40ffff, SYS16_MWA16_TILERAM, &sys16_tileram },
	{ 0x410000, 0x410fff, SYS16_MWA16_TEXTRAM, &sys16_textram },
	{ 0x440000, 0x440fff, SYS16_MWA16_SPRITERAM, &sys16_spriteram },
	{ 0x840000, 0x840fff, SYS16_MWA16_PALETTERAM, &paletteram16 },
	{ 0xc40000, 0xc40001, sys16_coinctrl_w },
	{ 0xc43000, 0xc43fff, MWA16_NOP },
	{ 0xfe0006, 0xfe0007, sound_command_w },
	{ 0xfe0020, 0xfe003f, MWA16_NOP }, /* config regs */
	{ 0xffc000, 0xffffff, SYS16_MWA16_WORKINGRAM, &sys16_workingram },
MEMORY_END

/***************************************************************************/

static MACHINE_INIT( altbeast ){
	sys16_update_proc = type0_sys16_textram;
}

static MACHINE_INIT( altbeas2 ){
	static int bank[16] = {
		0x00,0x00,
		0x01,0x00,
		0x02,0x00,
		0x03,0x00,
		0x04,0x00,
		0x05,0x00,
		0x06,0x00,
		0x07,0x00
	};
	sys16_obj_bank = bank;
	sys16_update_proc = type0_sys16_textram;
}

static DRIVER_INIT( altbeas2 )
{
	mc8123_decrypt_0066();
}

/***************************************************************************/

INPUT_PORTS_START( altbeast )
	SYS16_JOY1
	SYS16_JOY2
	SYS16_SERVICE
	SYS16_COINAGE

PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, "Credits needed" )
	PORT_DIPSETTING(    0x01, "1 to start, 1 to continue" )
	PORT_DIPSETTING(    0x00, "2 to start, 1 to continue" )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "240", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x30, 0x30, "Energy Meter" )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x80, "Easy" )
	PORT_DIPSETTING(    0xc0, "Normal" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
INPUT_PORTS_END

/***************************************************************************/

static MACHINE_DRIVER_START( altbeast )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16_7759b)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(altbeast_readmem,altbeast_writemem)

	MDRV_MACHINE_INIT(altbeast)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( altbeas2 )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16_7759b)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(altbeast_readmem,altbeast_writemem)

	MDRV_MACHINE_INIT(altbeas2)
MACHINE_DRIVER_END

/***************************************************************************/
/* sys16B */
ROM_START( atomicp )
	ROM_REGION( 0x020000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "ap-t2.bin", 0x000000, 0x10000, CRC(97421047) SHA1(18d61260470da7a0bef532f81df567a613c0d821) )
	ROM_LOAD16_BYTE( "ap-t1.bin", 0x000001, 0x10000, CRC(5c65fe56) SHA1(aaf3b6f932c090b839817140c105f13c7d6b4ae2) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "ap-t4.bin",  0x00000, 0x8000, CRC(332e58f4) SHA1(cf5aeb6c14018cbd8f222a0ecf85ccf467f294a8) )
	ROM_LOAD( "ap-t3.bin",  0x08000, 0x8000, CRC(dddc122c) SHA1(3411eae360ccd615636fb85e9738affc33c2c0ad) )
	ROM_LOAD( "ap-t5.bin",  0x10000, 0x8000, CRC(ef5ecd6b) SHA1(07edc8ea4c0a5ad421df7f97e7a62a5e12a8dbd0) )

	ROM_REGION( 0x1, REGION_GFX2, 0 ) /* sprites */
ROM_END

ROM_START( snapper )
	ROM_REGION( 0x020000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "snap2.r01", 0x000000, 0x10000, CRC(9a9e4ed3) SHA1(df3df13b70d4c0d1caaf42e78d355c0492fac96b) )
	ROM_LOAD16_BYTE( "snap1.r02", 0x000001, 0x10000, CRC(cd468d6a) SHA1(28b5e1f533f5e3fd9ebffe63bda7e6d9ebe4ffaa) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "snap4.r03",  0x00000, 0x8000, CRC(0f848e1e) SHA1(79a63ff0e5775400716f7294eabda9a0b838d656) )
	ROM_LOAD( "snap3.r04",  0x08000, 0x8000, CRC(c7f8cf0e) SHA1(08376f7941bc740ce85c6f32be7b54ced192599c) )
	ROM_LOAD( "snap5.r05",  0x10000, 0x8000, CRC(378e08eb) SHA1(f2c10bd9e885c185ac2d0d51d907ceca1f21dd7a) )

	ROM_REGION( 0x1, REGION_GFX2, 0 ) /* sprites */
ROM_END

/***************************************************************************/

#if 0
static READ16_HANDLER( atomicp_skip_r ){
	if (activecpu_get_pc()==0x7fc) {cpu_spinuntil_int(); return 0xffff;}
	return sys16_workingram[0x0902/2];
}
/*	AM_RANGE(0xffc902, 0xffc903) AM_READ(atomicp_skip_r) */
#endif


static MEMORY_READ16_START( atomicp_readmem )
	{ 0x000000, 0x01ffff, MRA16_ROM },
	{ 0x400000, 0x40ffff, SYS16_MRA16_TILERAM },
	{ 0x410000, 0x410fff, SYS16_MRA16_TEXTRAM },
	{ 0x440000, 0x440fff, SYS16_MRA16_SPRITERAM },
	{ 0x840000, 0x840fff, SYS16_MRA16_PALETTERAM },
	{ 0xc41000, 0xc41001, input_port_0_word_r }, /* player1 */
	{ 0xc41002, 0xc41003, input_port_1_word_r }, /* player2 */
	{ 0xc41004, 0xc41005, input_port_3_word_r }, /* dip1 */
	{ 0xc41006, 0xc41007, input_port_4_word_r }, /* dip2 */
	{ 0xffc000, 0xffffff, SYS16_MRA16_WORKINGRAM },
MEMORY_END

static MEMORY_WRITE16_START( atomicp_writemem )
	{ 0x000000, 0x01ffff, MWA16_ROM },
	{ 0x080000, 0x080003, MWA16_NOP }, /* looks like sound chip, but isn't YM2413 data */
	{ 0x3f0000, 0x3fffff, sys16_tilebank_w },
	{ 0x400000, 0x40ffff, SYS16_MWA16_TILERAM, &sys16_tileram },
	{ 0x410000, 0x410fff, SYS16_MWA16_TEXTRAM, &sys16_textram },
	{ 0x440000, 0x44ffff, SYS16_MWA16_SPRITERAM, &sys16_spriteram },
	{ 0x840000, 0x840fff, SYS16_MWA16_PALETTERAM, &paletteram16 },
	{ 0x123406, 0x123407, MWA16_NOP }, /* ? */
	{ 0xc40000, 0xc40001, MWA16_NOP },/* ? */
	{ 0xfe0020, 0xfe003f, MWA16_NOP }, /* config regs */
	{ 0xffc000, 0xffffff, SYS16_MWA16_WORKINGRAM, &sys16_workingram },
MEMORY_END


static WRITE16_HANDLER( snapper_sound_w )
{
	if (ACCESSING_MSB)
		switch (offset & 1)
		{
			case 0:	YM2413_register_port_0_w(0, data >> 8);	break;
			case 1:	YM2413_data_port_0_w(0, data >> 8);		break;
		}
}

static MEMORY_WRITE16_START( snapper_writemem )
	{ 0x000000, 0x01ffff, MWA16_ROM },
	{ 0x080000, 0x080003, snapper_sound_w }, /* looks like sound chip, but isn't YM2413 data seems correct for Snapper */
	{ 0x3f0000, 0x3fffff, sys16_tilebank_w },
	{ 0x400000, 0x40ffff, SYS16_MWA16_TILERAM, &sys16_tileram },
	{ 0x410000, 0x410fff, SYS16_MWA16_TEXTRAM, &sys16_textram },
	{ 0x440000, 0x44ffff, SYS16_MWA16_SPRITERAM, &sys16_spriteram },
	{ 0x840000, 0x840fff, SYS16_MWA16_PALETTERAM, &paletteram16 },
	{ 0x123406, 0x123407, MWA16_NOP }, /* ? */
	{ 0xc40000, 0xc40001, MWA16_NOP },/* ? */
	{ 0xfe0020, 0xfe003f, MWA16_NOP }, /* config regs */
	{ 0xffc000, 0xffffff, SYS16_MWA16_WORKINGRAM, &sys16_workingram },
MEMORY_END

/***************************************************************************/

static void snapper_sound_irq(int param)
{
	cpu_set_irq_line(0, 2, HOLD_LINE);
}

static MACHINE_INIT( atomicp ){
	sys16_update_proc = type0_sys16_textram;
}

static MACHINE_INIT( snapper ){
	sys16_update_proc = type0_sys16_textram;
  timer_pulse(TIME_IN_HZ(2500), 0, snapper_sound_irq);
}

/***************************************************************************/

INPUT_PORTS_START( atomicp )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* dummy */

	PORT_START	/* dip1 */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_5C ) )

	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_5C ) )

	PORT_DIPNAME( 0xC0, 0xC0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0xC0, "1" )
	PORT_DIPSETTING(    0x80, "2" )
	PORT_DIPSETTING(    0x40, "3" )
	PORT_DIPSETTING(    0x00, "5" )

	PORT_START  /* dip2 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Instructions" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Continuation" )
	PORT_DIPSETTING(    0x20, "Continue" )
	PORT_DIPSETTING(    0x00, "No Continue" )
	PORT_DIPNAME( 0x40, 0x00, "Level Select" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_HIGH )
INPUT_PORTS_END

INPUT_PORTS_START( snapper )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0xfc, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* dummy */

	PORT_START	/* dip1 */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0xC0, 0xC0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0xC0, "1" )
	PORT_DIPSETTING(    0x80, "2" )
	PORT_DIPSETTING(    0x40, "3" )
	PORT_DIPSETTING(    0x00, "5" )

	PORT_START	/* dip2 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Mute Demo Sounds" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE(       0x80, IP_ACTIVE_LOW )
INPUT_PORTS_END

/***************************************************************************/
static INTERRUPT_GEN( ap_interrupt )
{
	int intleft=cpu_getiloops();
	if(intleft!=0) cpu_set_irq_line(0, 2, HOLD_LINE);
	else cpu_set_irq_line(0, 4, HOLD_LINE);
}


static MACHINE_DRIVER_START( atomicp )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(atomicp_readmem,atomicp_writemem)
	MDRV_CPU_VBLANK_INT(ap_interrupt,2)

	MDRV_CPU_REMOVE("sound")

	MDRV_MACHINE_INIT(atomicp)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(0)
	MDRV_SOUND_REPLACE("2151", YM2413, sys16_ym2413_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( snapper)

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(atomicp_readmem,snapper_writemem)

	MDRV_CPU_REMOVE("sound")

	MDRV_MACHINE_INIT(snapper)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(0)
	MDRV_SOUND_REPLACE("2151", YM2413, sys16_ym2413_interface)
  MDRV_SOUND_REMOVE("7759")
MACHINE_DRIVER_END


/***************************************************************************/
/* sys16B */
ROM_START( bayroute )
	ROM_REGION( 0xc0000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "br.4a", 0x000000, 0x10000, CRC(91c6424b) SHA1(79a968ade7690d5944ee815e19586cb82c3aa562) )
	ROM_LOAD16_BYTE( "br.1a", 0x000001, 0x10000, CRC(76954bf3) SHA1(bd617c6ddaf1e7760479b17205388c201fb67662) )
	/* empty 0x20000-0x80000*/
	ROM_LOAD16_BYTE( "br.5a", 0x080000, 0x10000, CRC(9d6fd183) SHA1(5ae78d33c0e929886d84a25c0fbd62ab45dcbff4) )
	ROM_LOAD16_BYTE( "br.2a", 0x080001, 0x10000, CRC(5ca1e3d2) SHA1(51ce67ed0a0054f9c9c4ac56c5775716c44d74b1) )
	ROM_LOAD16_BYTE( "br.6a", 0x0a0000, 0x10000, CRC(ed97ad4c) SHA1(6c7d671c3046f1adb486f053acdd2be0c981c68b) )
	ROM_LOAD16_BYTE( "br.3a", 0x0a0001, 0x10000, CRC(0d362905) SHA1(04cb35aa44cc1d9ead44c5a7b4f838efec453c85) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "opr12462.a14", 0x00000, 0x10000, CRC(a19943b5) SHA1(95cd95223ea76677227b807a7c4eff120e690d37) )
	ROM_LOAD( "opr12463.a15", 0x10000, 0x10000, CRC(62f8200d) SHA1(a5a0035249f339396b33f8a908d393777e8951c4) )
	ROM_LOAD( "opr12464.a16", 0x20000, 0x10000, CRC(c8c59703) SHA1(3a4f45b88990d27c55ddfde5fc93496954868200) )

	ROM_REGION( 0x080000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "br_obj0o.1b", 0x00001, 0x10000, CRC(098a5e82) SHA1(c5922f418773bc3629071e584457839d67a370e9) )
	ROM_LOAD16_BYTE( "br_obj0e.5b", 0x00000, 0x10000, CRC(85238af9) SHA1(39989a8d9b60c6d55272b5e2c213341a563dd993) )
	ROM_LOAD16_BYTE( "br_obj1o.2b", 0x20001, 0x10000, CRC(cc641da1) SHA1(28f8a6502702cb9e2cc7f3e98f6c5d201f462fa3) )
	ROM_LOAD16_BYTE( "br_obj1e.6b", 0x20000, 0x10000, CRC(d3123315) SHA1(16a87caed1cabb080d4f35935910b38797344ca5) )
	ROM_LOAD16_BYTE( "br_obj2o.3b", 0x40001, 0x10000, CRC(84efac1f) SHA1(41c43d70dc7ae7e361d6fa12c5790ea7ebf13ca8) )
	ROM_LOAD16_BYTE( "br_obj2e.7b", 0x40000, 0x10000, CRC(b73b12cb) SHA1(e8265ae90aabf1ee0522dbc6541a0f82fec97c7a) )
	ROM_LOAD16_BYTE( "br_obj3o.4b", 0x60001, 0x10000, CRC(a2e238ac) SHA1(c854774c0ffd1ccf6e46591a8fa3c80a4630e007) )
	ROM_LOAD16_BYTE( "br.8b",		  0x60000, 0x10000, CRC(d8de78ff) SHA1(110661ab8008543b47629722b98d0470f73a48c5) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr12459.a10", 0x00000, 0x08000, CRC(3e1d29d0) SHA1(fe3d985983e5132e8a26a02a3f2d8d420cbf1a49) )
	ROM_LOAD( "mpr12460.a11", 0x10000, 0x20000, CRC(0bae570d) SHA1(05fa4a3405666342ab66e696a7344cca97569f19) )
	ROM_LOAD( "mpr12461.a12", 0x30000, 0x20000, CRC(b03b8b46) SHA1(b0283ac377d464f3d9374a992192ec6c515a3c2f) )
ROM_END

ROM_START( bayrouta )
	ROM_REGION( 0xc0000, REGION_CPU1, 0 ) /* 68000 code */
/* custom cpu 317-0116 */
	ROM_LOAD16_BYTE( "epr12517.a7", 0x000000, 0x20000, CRC(436728a9) SHA1(0f6753493ad2c1776880862b462f10ef936a9ee5) )
	ROM_LOAD16_BYTE( "epr12516.a5", 0x000001, 0x20000, CRC(4ff0353f) SHA1(f5960f9e80e42d0a3f82d6670b1f2e39b51ffcef) )
	/* empty 0x40000-0x80000*/
	ROM_LOAD16_BYTE( "epr12458.a8", 0x080000, 0x20000, CRC(e7c7476a) SHA1(7b724d76bdc1978ddf78489edfda14533905a360) )
	ROM_LOAD16_BYTE( "epr12456.a6", 0x080001, 0x20000, CRC(25dc2eaf) SHA1(dda300840b9a90bcce7be16ff1904a7a0456c396) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "opr12462.a14", 0x00000, 0x10000, CRC(a19943b5) SHA1(95cd95223ea76677227b807a7c4eff120e690d37) )
	ROM_LOAD( "opr12463.a15", 0x10000, 0x10000, CRC(62f8200d) SHA1(a5a0035249f339396b33f8a908d393777e8951c4) )
	ROM_LOAD( "opr12464.a16", 0x20000, 0x10000, CRC(c8c59703) SHA1(3a4f45b88990d27c55ddfde5fc93496954868200) )

	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "mpr12465.b1", 0x00001, 0x20000, CRC(11d61b45) SHA1(01678e07ffcafb8c161b52763f2183dc281c7578) )
	ROM_LOAD16_BYTE( "mpr12467.b5", 0x00000, 0x20000, CRC(c3b4e4c0) SHA1(2d8dbea5278b3fac03c7ad8749f931d36cc8f341) )
	ROM_LOAD16_BYTE( "mpr12466.b2", 0x40001, 0x20000, CRC(a57f236f) SHA1(c83219cdfcee10a4fdffcbf410808f161a2b1aef) )
	ROM_LOAD16_BYTE( "mpr12468.b6", 0x40000, 0x20000, CRC(d89c77de) SHA1(0e903bf57a7515291dda7e11bdef982a1417043a) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr12459.a10", 0x00000, 0x08000, CRC(3e1d29d0) SHA1(fe3d985983e5132e8a26a02a3f2d8d420cbf1a49) )
	ROM_LOAD( "mpr12460.a11", 0x10000, 0x20000, CRC(0bae570d) SHA1(05fa4a3405666342ab66e696a7344cca97569f19) )
	ROM_LOAD( "mpr12461.a12", 0x30000, 0x20000, CRC(b03b8b46) SHA1(b0283ac377d464f3d9374a992192ec6c515a3c2f) )
ROM_END

ROM_START( bayrtbl1 )
	ROM_REGION( 0xc0000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "b4.bin", 0x000000, 0x10000, CRC(eb6646ae) SHA1(073bc0a3868e70785f44e497a949cd9e3b591a33) )
	ROM_LOAD16_BYTE( "b2.bin", 0x000001, 0x10000, CRC(ecd9cd0e) SHA1(177c38ca02c4e87d6adcae77ce4e9237938d23a9) )
	/* empty 0x20000-0x80000*/
	ROM_LOAD16_BYTE( "br.5a",  0x080000, 0x10000, CRC(9d6fd183) SHA1(5ae78d33c0e929886d84a25c0fbd62ab45dcbff4) )
	ROM_LOAD16_BYTE( "br.2a",  0x080001, 0x10000, CRC(5ca1e3d2) SHA1(51ce67ed0a0054f9c9c4ac56c5775716c44d74b1) )
	ROM_LOAD16_BYTE( "b8.bin", 0x0a0000, 0x10000, CRC(e7ca0331) SHA1(b255939576a84f4d266f31a7fde818e04ff35b24) )
	ROM_LOAD16_BYTE( "b6.bin", 0x0a0001, 0x10000, CRC(2bc748a6) SHA1(9ab760377fde24cecb703726ee3e59ee23d60a3a) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "bs16.bin", 0x00000, 0x10000, CRC(a8a5b310) SHA1(8883e1ed48a3e0f7b4c36d83579f93e84e28568c) )
	ROM_LOAD( "bs14.bin", 0x10000, 0x10000, CRC(6bc4d0a8) SHA1(90b9a61c7a140291d72554857ce26d54ebf03fc2) )
	ROM_LOAD( "bs12.bin", 0x20000, 0x10000, CRC(c1f967a6) SHA1(8eb6bbd9e17dc531830bc798b8485c8ea999e56e) )

	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "br_obj0o.1b", 0x00001, 0x10000, CRC(098a5e82) SHA1(c5922f418773bc3629071e584457839d67a370e9) )
	ROM_LOAD16_BYTE( "br_obj0e.5b", 0x00000, 0x10000, CRC(85238af9) SHA1(39989a8d9b60c6d55272b5e2c213341a563dd993) )
	ROM_LOAD16_BYTE( "br_obj1o.2b", 0x20001, 0x10000, CRC(cc641da1) SHA1(28f8a6502702cb9e2cc7f3e98f6c5d201f462fa3) )
	ROM_LOAD16_BYTE( "br_obj1e.6b", 0x20000, 0x10000, CRC(d3123315) SHA1(16a87caed1cabb080d4f35935910b38797344ca5) )
	ROM_LOAD16_BYTE( "br_obj2o.3b", 0x40001, 0x10000, CRC(84efac1f) SHA1(41c43d70dc7ae7e361d6fa12c5790ea7ebf13ca8) )
	ROM_LOAD16_BYTE( "br_obj2e.7b", 0x40000, 0x10000, CRC(b73b12cb) SHA1(e8265ae90aabf1ee0522dbc6541a0f82fec97c7a) )
	ROM_LOAD16_BYTE( "br_obj3o.4b", 0x60001, 0x10000, CRC(a2e238ac) SHA1(c854774c0ffd1ccf6e46591a8fa3c80a4630e007) )
	ROM_LOAD16_BYTE( "bs7.bin",     0x60000, 0x10000, CRC(0c91abcc) SHA1(d25608f3cbacd1bd169f1a2247f007ac8bc8dda0) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr12459.a10", 0x00000, 0x08000, CRC(3e1d29d0) SHA1(fe3d985983e5132e8a26a02a3f2d8d420cbf1a49) )
	ROM_LOAD( "mpr12460.a11", 0x10000, 0x20000, CRC(0bae570d) SHA1(05fa4a3405666342ab66e696a7344cca97569f19) )
	ROM_LOAD( "mpr12461.a12", 0x30000, 0x20000, CRC(b03b8b46) SHA1(b0283ac377d464f3d9374a992192ec6c515a3c2f) )
ROM_END

ROM_START( bayrtbl2 )
	ROM_REGION( 0xc0000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "br_04", 0x000000, 0x10000, CRC(2e33ebfc) SHA1(f6b5a4bd28d302abd6b1e5a9ec6f2a8b57ff213e) )
	ROM_LOAD16_BYTE( "br_06", 0x000001, 0x10000, CRC(3db42313) SHA1(e1c874ebf83e1a458cefaa038fbe89a9804ca30d) )
	/* empty 0x20000-0x80000*/
	ROM_LOAD16_BYTE( "br_03", 0x080000, 0x20000, CRC(285d256b) SHA1(73eac0131d14f0d7fe2a06cb2e0e57dcf4779cf9) )
	ROM_LOAD16_BYTE( "br_05", 0x080001, 0x20000, CRC(552e6384) SHA1(2770b0c9d961671576e09ada2ebd7bb486f24547) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "br_15",    0x00000, 0x10000, CRC(050079a9) SHA1(4b356eddec2f500fb0dcc20af6b7aed2f9ef0c02) )
	ROM_LOAD( "br_16",    0x10000, 0x10000, CRC(fc371928) SHA1(b36866c95bdc440aae999a90ecf3bbaed11d4351) )
	ROM_LOAD( "bs12.bin", 0x20000, 0x10000, CRC(c1f967a6) SHA1(8eb6bbd9e17dc531830bc798b8485c8ea999e56e) )

	ROM_REGION( 0x080000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "br_11",       0x00001, 0x10000, CRC(65232905) SHA1(cb195a0ce8bff9d1d3e31678060b3aaccfefcd2d) )
	ROM_LOAD16_BYTE( "br_obj0e.5b", 0x00000, 0x10000, CRC(85238af9) SHA1(39989a8d9b60c6d55272b5e2c213341a563dd993) )
	ROM_LOAD16_BYTE( "br_obj1o.2b", 0x20001, 0x10000, CRC(cc641da1) SHA1(28f8a6502702cb9e2cc7f3e98f6c5d201f462fa3) )
	ROM_LOAD16_BYTE( "br_obj1e.6b", 0x20000, 0x10000, CRC(d3123315) SHA1(16a87caed1cabb080d4f35935910b38797344ca5) )
	ROM_LOAD16_BYTE( "br_obj2o.3b", 0x40001, 0x10000, CRC(84efac1f) SHA1(41c43d70dc7ae7e361d6fa12c5790ea7ebf13ca8) )
	ROM_LOAD16_BYTE( "br_09",       0x40000, 0x10000, CRC(05e9b840) SHA1(7cc1c9ac7b85f1e1bdb68215b5e83eae3ee5ba2a) )
	ROM_LOAD16_BYTE( "br_14",       0x60001, 0x10000, CRC(4c4a177b) SHA1(a9dfd7e56b0a21a0f7750d8ec4631901ad182609) )
	ROM_LOAD16_BYTE( "bs7.bin",     0x60000, 0x10000, CRC(0c91abcc) SHA1(d25608f3cbacd1bd169f1a2247f007ac8bc8dda0) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "br_01", 0x00000, 0x10000, CRC(b87156ec) SHA1(bdfef2ab5a4d3cac4077c92ce1ef4604b4c11cf8) )
	ROM_LOAD( "br_02", 0x10000, 0x10000, CRC(ef63991b) SHA1(4221741780f88c80b3213ddca949bee7d4c1469a) )
ROM_END

/***************************************************************************/

static MEMORY_READ16_START( bayroute_readmem )
	{ 0x000000, 0x0bffff, MRA16_ROM },
	{ 0x500000, 0x503fff, SYS16_MRA16_WORKINGRAM },
	{ 0x600000, 0x600fff, SYS16_MRA16_SPRITERAM },
	{ 0x700000, 0x70ffff, SYS16_MRA16_TILERAM },
	{ 0x710000, 0x710fff, SYS16_MRA16_TEXTRAM },
	{ 0x800000, 0x800fff, SYS16_MRA16_PALETTERAM },
	{ 0x901002, 0x901003, input_port_0_word_r }, /* player1 */
	{ 0x901006, 0x901007, input_port_1_word_r }, /* player2 */
	{ 0x901000, 0x901001, input_port_2_word_r }, /* service */
	{ 0x902002, 0x902003, input_port_3_word_r }, /* dip1 */
	{ 0x902000, 0x902001, input_port_4_word_r }, /* dip2 */
MEMORY_END

static MEMORY_WRITE16_START( bayroute_writemem )
	{ 0x000000, 0x0bffff, MWA16_ROM },
	{ 0x100000, 0x100003, MWA16_NOP }, /* tilebank control? */
	{ 0x500000, 0x503fff, SYS16_MWA16_WORKINGRAM, &sys16_workingram },
	{ 0x600000, 0x600fff, SYS16_MWA16_SPRITERAM, &sys16_spriteram },
	{ 0x700000, 0x70ffff, SYS16_MWA16_TILERAM, &sys16_tileram },
	{ 0x710000, 0x710fff, SYS16_MWA16_TEXTRAM, &sys16_textram },
	{ 0x800000, 0x800fff, SYS16_MWA16_PALETTERAM, &paletteram16 },
	{ 0x900000, 0x900001, sys16_coinctrl_w },
	{ 0xff0006, 0xff0007, sound_command_w },
	{ 0xff0020, 0xff003f, MWA16_NOP },/* config regs */
MEMORY_END

/***************************************************************************/

static MACHINE_INIT( bayroute ){
	static int bank[16] = {
		0,0,0,0,
		0,0,0,3,
		0,0,0,2,
		0,1,0,0
	};
	sys16_obj_bank = bank;
	sys16_update_proc = type0_sys16_textram;
	sys16_spritesystem = sys16_sprite_shinobi;
}

static DRIVER_INIT( bayrtbl1 ){
	int i;

	/* invert the graphics bits on the tiles */
	for (i = 0; i < 0x30000; i++)
		memory_region(REGION_GFX1)[i] ^= 0xff;
}
/***************************************************************************/

INPUT_PORTS_START( bayroute )
	SYS16_JOY1
	SYS16_JOY2
	SYS16_SERVICE
	SYS16_COINAGE

PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x04, "1" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x08, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Unlimited", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x30, "10000" )
	PORT_DIPSETTING(    0x20, "15000" )
	PORT_DIPSETTING(    0x10, "20000" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0xc0, "A" )
	PORT_DIPSETTING(    0x80, "B" )
	PORT_DIPSETTING(    0x40, "C" )
	PORT_DIPSETTING(    0x00, "D" )

INPUT_PORTS_END

/***************************************************************************/

static MACHINE_DRIVER_START( bayroute )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16_7759b)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(bayroute_readmem,bayroute_writemem)

	MDRV_MACHINE_INIT(bayroute)
/* notes sample missing from title screen same problem before update */
MACHINE_DRIVER_END

/***************************************************************************

   Body Slam

***************************************************************************/
/* pre16 */
ROM_START( bodyslam )
	ROM_REGION( 0x30000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr10066.b9", 0x000000, 0x8000, CRC(6cd53290) SHA1(68ef83ad99a26a507d9bc4cd715462169f4ac41f) )
	ROM_LOAD16_BYTE( "epr10063.b6", 0x000001, 0x8000, CRC(dd849a16) SHA1(b8cb9f2685a739698a3ed18f76617fd4ac9cb424) )
	ROM_LOAD16_BYTE( "epr10067.b10",0x010000, 0x8000, CRC(db22a5ce) SHA1(95c37d4913fa31d5edf02661681bc83deec731d9) )
	ROM_LOAD16_BYTE( "epr10064.b7", 0x010001, 0x8000, CRC(53d6b7e0) SHA1(00bfa1487479629f60e1cc1b98ced47e4cb07964) )
	ROM_LOAD16_BYTE( "epr10068.b11",0x020000, 0x8000, CRC(15ccc665) SHA1(b088a9bcb1499854794b2dbf4c689f3ae3ce2808) )
	ROM_LOAD16_BYTE( "epr10065.b8", 0x020001, 0x8000, CRC(0e5fa314) SHA1(44e36fde102ba6aef2c3b4374ddc21690f2fe527) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "epr10321.c9",  0x00000, 0x8000, CRC(cd3e7cba) SHA1(4d3cfc7346c6e63e2221193601f949162d0e4f90) ) /* plane 1 */
	ROM_LOAD( "epr10322.c10", 0x08000, 0x8000, CRC(b53d3217) SHA1(baebf20925e9f8ab6660f041a24721716d5b7d92) ) /* plane 2 */
	ROM_LOAD( "epr10323.c11", 0x10000, 0x8000, CRC(915a3e61) SHA1(6504a8b26b7b4880971cd69ac2c8aae30dcfa18c) ) /* plane 3 */

	ROM_REGION( 0x40000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "epr10012.c5",  0x00001, 0x08000, CRC(990824e8) SHA1(bd45f75d07cb4e17583c2d76050e5f819f4b7efe) )
	ROM_LOAD16_BYTE( "epr10016.b2",  0x00000, 0x08000, CRC(af5dc72f) SHA1(97bbb76940c702e642d8222dda71447b8f60b616) )
	ROM_LOAD16_BYTE( "epr10013.c6",  0x10001, 0x08000, CRC(9a0919c5) SHA1(e39e60c1e834b3b46bf2ef1c5952841bebe66ade) )
	ROM_LOAD16_BYTE( "epr10017.b3",  0x10000, 0x08000, CRC(62aafd95) SHA1(e1e3a95fd11cabf81f44ac2dd3f951d3094725e6) )
	ROM_LOAD16_BYTE( "epr10027.c7",  0x20001, 0x08000, CRC(3f1c57c7) SHA1(1336da8dc167a323f09534a2f62ae6f9c62290e4) )
	ROM_LOAD16_BYTE( "epr10028.b4",  0x20000, 0x08000, CRC(80d4946d) SHA1(d4c96a18ef6c2ac6bd9d153d8862a3af894642e8) )
	ROM_LOAD16_BYTE( "epr10015.c8",  0x30001, 0x08000, CRC(582d3b6a) SHA1(4f1d0060682e3fc1147082286e00e6a296a95da2) )
	ROM_LOAD16_BYTE( "epr10019.b5",  0x30000, 0x08000, CRC(e020c38b) SHA1(d13d38a64f2afa7df3cbccef2fe505a4421b73ad) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr10026.b1", 0x00000, 0x8000, CRC(123b69b8) SHA1(c0614a8c822991e257f7218908247df278056de8) )

	ROM_REGION( 0x1000, REGION_CPU3, 0 )      /* 4k for 7751 onboard ROM */
	ROM_LOAD( "7751.bin",     0x0000, 0x0400, CRC(6a9534fc) SHA1(67ad94674db5c2aab75785668f610f6f4eccd158) ) /* 7751 - U34 */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* 7751 sound data */
	ROM_LOAD( "epr10029.c1", 0x00000, 0x8000, CRC(7e4aca83) SHA1(703486b96d493941ee87267e8363220a851f008e) )
	ROM_LOAD( "epr10030.c2", 0x08000, 0x8000, CRC(dcc1df0b) SHA1(a82a557fa48f4b3e1ab38f61b84d749cd417e80f) )
	ROM_LOAD( "epr10031.c3", 0x10000, 0x8000, CRC(ea3c4472) SHA1(ad8eac2d3d14fd6aba713f4d624861c17aabf757) )
	ROM_LOAD( "epr10032.c4", 0x18000, 0x8000, CRC(0aabebce) SHA1(fab12df8f4eab270be491c6c025d832c338e1e83) )

ROM_END

ROM_START( dumpmtmt )
	ROM_REGION( 0x30000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "7704a.bin", 0x000000, 0x8000, CRC(96de6c7b) SHA1(f23edf86c5044c151a8502957af7ca0de420d55e) )
	ROM_LOAD16_BYTE( "7701a.bin", 0x000001, 0x8000, CRC(786d1009) SHA1(c56ebd169c2792cde610a7130cffdc0363fca871) )
	ROM_LOAD16_BYTE( "7705a.bin", 0x010000, 0x8000, CRC(fc584391) SHA1(27238408fba2dda67f29094a6700b634b6fdaa58) )
	ROM_LOAD16_BYTE( "7702a.bin", 0x010001, 0x8000, CRC(2241a8fd) SHA1(d968ab57aa228dbb7ae6f17d7bf22991291e75ae) )
	ROM_LOAD16_BYTE( "7706a.bin", 0x020000, 0x8000, CRC(6bbcc9d0) SHA1(e8e0b85867f11eec6b280f3ad9e2746d3d97ab28) )
	ROM_LOAD16_BYTE( "7703a.bin", 0x020001, 0x8000, CRC(fcb0cd40) SHA1(999e107fe08fcb52729ddebc7714a85c47e748b1) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "7707a.bin",  0x00000, 0x8000, CRC(45318738) SHA1(6885347321aec8c4829a71e4518d1742f939ea9c) ) /* plane 1 */
	ROM_LOAD( "7708a.bin",  0x08000, 0x8000, CRC(411be9a4) SHA1(808a9c941d353f34c3491ca2cde984e73cc7a87d) ) /* plane 2 */
	ROM_LOAD( "7709a.bin",  0x10000, 0x8000, CRC(74ceb5a8) SHA1(93ed6bb4a3c820f3a7ee5e9b2c2ce35d2bed8529) ) /* plane 3 */

	ROM_REGION( 0x40000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "7715.bin",  	0x000001, 0x08000, CRC(bf47e040) SHA1(5aa1b9adaa2095844c10993402a0597bb5768efb) )
	ROM_LOAD16_BYTE( "7719.bin",  	0x000000, 0x08000, CRC(fa5c5d6c) SHA1(6cac5d3fd705d1365348d57a18bbeb1eb9e412b8) )
	ROM_LOAD16_BYTE( "epr10013.c6",	0x010001, 0x08000, CRC(9a0919c5) SHA1(e39e60c1e834b3b46bf2ef1c5952841bebe66ade) )	/* 7716 */
	ROM_LOAD16_BYTE( "epr10017.b3",	0x010000, 0x08000, CRC(62aafd95) SHA1(e1e3a95fd11cabf81f44ac2dd3f951d3094725e6) )	/* 7720 */
	ROM_LOAD16_BYTE( "7717.bin",  	0x020001, 0x08000, CRC(fa64c86d) SHA1(ada722dd6efbf466a719ee1fe34a36ce1ea20184) )
	ROM_LOAD16_BYTE( "7721.bin",  	0x020000, 0x08000, CRC(62a9143e) SHA1(28f0dc0329163f0a6505dd34a24a843b35118c5e) )
	ROM_LOAD16_BYTE( "epr10015.c8",	0x030001, 0x08000, CRC(582d3b6a) SHA1(4f1d0060682e3fc1147082286e00e6a296a95da2) )	/* 7718 */
	ROM_LOAD16_BYTE( "epr10019.b5",	0x030000, 0x08000, CRC(e020c38b) SHA1(d13d38a64f2afa7df3cbccef2fe505a4421b73ad) )	/* 7722 */

	ROM_REGION( 0x30000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "7710a.bin", 0x00000, 0x8000, CRC(a19b8ba8) SHA1(21b628d4ecbe38a6d96a39ca4252ff1cb728343f) )

	ROM_REGION( 0x1000, REGION_CPU3, 0 )      /* 4k for 7751 onboard ROM */
	ROM_LOAD( "7751.bin",     0x0000, 0x0400, CRC(6a9534fc) SHA1(67ad94674db5c2aab75785668f610f6f4eccd158) ) /* 7751 - U34 */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* 7751 sound data */
	ROM_LOAD( "7711.bin", 0x00000, 0x8000, CRC(efa9aabd) SHA1(b0928313b98159b95f3a6784c6279924774b9253) )
	ROM_LOAD( "7712.bin", 0x08000, 0x8000, CRC(7bcd85cf) SHA1(9acba6998327e1074d7311a9b6d06da9baf69aa0) )
	ROM_LOAD( "7713.bin", 0x10000, 0x8000, CRC(33f292e7) SHA1(4358cd3922a0dcbf109d2d697c7b8c4e090c3d52) )
	ROM_LOAD( "7714.bin", 0x18000, 0x8000, CRC(8fd48c47) SHA1(1cba63a9e7e0b477683b7758d124f4949558ba7a) )

ROM_END

/***************************************************************************/

static MEMORY_READ16_START( bodyslam_readmem )
	{ 0x000000, 0x02ffff, MRA16_ROM },
	{ 0x400000, 0x40ffff, SYS16_MRA16_TILERAM },
	{ 0x410000, 0x410fff, SYS16_MRA16_TEXTRAM },
	{ 0x440000, 0x440fff, SYS16_MRA16_SPRITERAM },
	{ 0x840000, 0x840fff, SYS16_MRA16_PALETTERAM },
	{ 0xc41002, 0xc41003, input_port_0_word_r }, /* player1 */
	{ 0xc41006, 0xc41007, input_port_1_word_r }, /* player2 */
	{ 0xc41000, 0xc41001, input_port_2_word_r }, /* service */
	{ 0xc42000, 0xc42001, input_port_3_word_r }, /* dip1 */
	{ 0xc42002, 0xc42003, input_port_4_word_r },/* dip2 */
	{ 0xffc000, 0xffffff, SYS16_MRA16_WORKINGRAM },
MEMORY_END

static MEMORY_WRITE16_START( bodyslam_writemem )
	{ 0x000000, 0x02ffff, MWA16_ROM },
	{ 0x400000, 0x40ffff, SYS16_MWA16_TILERAM, &sys16_tileram },
	{ 0x410000, 0x410fff, SYS16_MWA16_TEXTRAM, &sys16_textram },
	{ 0x440000, 0x440fff, SYS16_MWA16_SPRITERAM, &sys16_spriteram },
	{ 0x840000, 0x840fff, SYS16_MWA16_PALETTERAM, &paletteram16 },
	{ 0xc40000, 0xc40001, sound_command_nmi_w },
	{ 0xc40002, 0xc40003, sys16_3d_coinctrl_w },
	{ 0xffc000, 0xffffff, SYS16_MWA16_WORKINGRAM, &sys16_workingram },
MEMORY_END

/***************************************************************************/

static MACHINE_INIT( bodyslam ){
	sys16_textmode=1;
	sys16_spritesystem = sys16_sprite_quartet2;
	sys16_sprxoffset = -0xbd;
	sys16_fgxoffset = sys16_bgxoffset = 8;
	sys16_bg_priority_mode = 2;
	sys16_fg_priority_mode = 2;

	sys16_textlayer_lo_min=0;
	sys16_textlayer_lo_max=0x1f;
	sys16_textlayer_hi_min=0x20;
	sys16_textlayer_hi_max=0xff;

	sys16_update_proc = type1_sys16_textram;
}

/* I have no idea if this is needed, but I cannot find any code for the countdown */
/* timer in the code and this seems to work ok. */
static void bodyslam_irq_timer(void)
{
	UINT8 flag = sys16_workingram[0x200/2] >> 8;
	UINT8 tick = sys16_workingram[0x200/2] & 0xff;
	UINT8 sec = sys16_workingram[0x202/2] >> 8;
	UINT8 min = sys16_workingram[0x202/2] & 0xff;

	/* out of time? set the flag */
	if (tick == 0 && sec == 0 && min == 0)
		flag = 1;
	else
	{
		if (tick != 0)
			tick--;
		else
		{
			/* the game counts 64 ticks per second */
			tick = 0x40;

			/* seconds are counted in BCD */
			if (sec != 0)
				sec = (sec & 0xf) ? sec - 1 : (sec - 0x10) + 9;
			else
			{
				sec = 0x59;

				/* minutes are counted normally */
				if (min != 0)
					min--;
				else
				{
					flag = 1;
					tick = sec = min = 0;
				}
			}
		}
	}
	sys16_workingram[0x200/2] = (flag << 8) + tick;
	sys16_workingram[0x202/2] = (sec << 8) + min;
}

static DRIVER_INIT( bodyslam ){
	sys16_bg1_trans=1;
	sys16_custom_irq=bodyslam_irq_timer;
}

/***************************************************************************/

INPUT_PORTS_START( bodyslam )
	SYS16_JOY1
	SYS16_JOY2
	SYS16_SERVICE
	SYS16_COINAGE

PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
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
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

INPUT_PORTS_END

/***************************************************************************/

static MACHINE_DRIVER_START( bodyslam )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16_7751)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(bodyslam_readmem,bodyslam_writemem)

	MDRV_MACHINE_INIT(bodyslam)
MACHINE_DRIVER_END

/***************************************************************************/
/* sys16B */
ROM_START( dduxbl )
	ROM_REGION( 0x0c0000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "dduxb03.bin", 0x000000, 0x20000, CRC(e7526012) SHA1(a1798008bfa1ce9b87dc330f3817b1978052fcfd) )
	ROM_LOAD16_BYTE( "dduxb05.bin", 0x000001, 0x20000, CRC(459d1237) SHA1(55e9c0dc341c919d58cc789203642c397d7ac65e) )
	/* empty 0x40000 - 0x80000 */
	ROM_LOAD16_BYTE( "dduxb02.bin", 0x080000, 0x20000, CRC(d8ed3132) SHA1(a9d5ad8f79fb635cc234a99fad398688a5f15926) )
	ROM_LOAD16_BYTE( "dduxb04.bin", 0x080001, 0x20000, CRC(30c6cb92) SHA1(2e17c74eeb37c9731fc2e365cc0114f7383c0106) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "dduxb14.bin", 0x00000, 0x10000, CRC(664bd135) SHA1(674b06e01c2c8f5b8057dd24d470330c3f140473) )
	ROM_LOAD( "dduxb15.bin", 0x10000, 0x10000, CRC(ce0d2b30) SHA1(e60521c46f1650c9bdc76f2ceb91a6d61aaa0a09) )
	ROM_LOAD( "dduxb16.bin", 0x20000, 0x10000, CRC(6de95434) SHA1(7bed2a0261cf6c2fbb3756633f05f0bb2173977c) )

	ROM_REGION( 0xa0000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "dduxb10.bin", 0x00001, 0x010000, CRC(0be3aee5) SHA1(48fc779b7398abbb82cd0d0d28705ece75b3c4e3) )
	ROM_RELOAD( 0x20001, 0x010000 )
	ROM_LOAD16_BYTE( "dduxb06.bin", 0x00000, 0x010000, CRC(b0079e99) SHA1(9bb4d3fa804a3d05a6e06b45a1280d7064e96ac6) )
	ROM_RELOAD( 0x20000, 0x010000 )
	ROM_LOAD16_BYTE( "dduxb11.bin", 0x40001, 0x010000, CRC(cfb2af18) SHA1(1ad18f933a7b797f0364d1f4a6c8549351b4c9a6) )
	ROM_LOAD16_BYTE( "dduxb07.bin", 0x40000, 0x010000, CRC(0217369c) SHA1(b6ec2fa1279a27a602d79e1073c54193745ea816) )
	ROM_LOAD16_BYTE( "dduxb12.bin", 0x60001, 0x010000, CRC(28ce9b15) SHA1(1640df9c8f21893c0647ad2f4210c714a06e6f37) )
	ROM_LOAD16_BYTE( "dduxb08.bin", 0x60000, 0x010000, CRC(8844f336) SHA1(18c1baaad3bcc658d4a6d03de8c97378b5284e88) )
	ROM_LOAD16_BYTE( "dduxb13.bin", 0x80001, 0x010000, CRC(efe57759) SHA1(69b8969b20ab9480df2735bd2bcd527069196bd7) )
	ROM_LOAD16_BYTE( "dduxb09.bin", 0x80000, 0x010000, CRC(6b64f665) SHA1(df07fcf2bbec6fa78f89b95272762aebd6f3ec0e) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "dduxb01.bin", 0x0000, 0x8000, CRC(0dbef0d7) SHA1(8b9afb2fcb946cec467b1e691c267194b503f841) )
ROM_END

/***************************************************************************/
static READ16_HANDLER( dduxbl_skip_r ){
	if (activecpu_get_pc()==0x502) {cpu_spinuntil_int(); return 0xffff;}
	return sys16_workingram[0x36e0/2];
}

static MEMORY_READ16_START( dduxbl_readmem )
	{ 0x000000, 0x0bffff, MRA16_ROM },
	{ 0x400000, 0x40ffff, SYS16_MRA16_TILERAM },
	{ 0x410000, 0x410fff, SYS16_MRA16_TEXTRAM },
	{ 0x440000, 0x440fff, SYS16_MRA16_SPRITERAM },
	{ 0x840000, 0x840fff, SYS16_MRA16_PALETTERAM },
	{ 0xc41002, 0xc41003, input_port_0_word_r }, /* player1 */
	{ 0xc41004, 0xc41005, input_port_1_word_r }, /* player2 */
	{ 0xc41000, 0xc41001, input_port_2_word_r }, /* service */
	{ 0xc42002, 0xc42003, input_port_3_word_r }, /* dip1 */
	{ 0xc42000, 0xc42001, input_port_4_word_r }, /* dip2 */
	{ 0xfff6e0, 0xfff6e1, dduxbl_skip_r },
	{ 0xffc000, 0xffffff, SYS16_MRA16_WORKINGRAM },
MEMORY_END

static MEMORY_WRITE16_START( dduxbl_writemem )
	{ 0x000000, 0x0bffff, MWA16_ROM },
	{ 0x3f0000, 0x3fffff, sys16_tilebank_w },
	{ 0x400000, 0x40ffff, SYS16_MWA16_TILERAM, &sys16_tileram },
	{ 0x410000, 0x410fff, SYS16_MWA16_TEXTRAM, &sys16_textram },
	{ 0x440000, 0x440fff, SYS16_MWA16_SPRITERAM, &sys16_spriteram },
	{ 0x840000, 0x840fff, SYS16_MWA16_PALETTERAM, &paletteram16 },
	{ 0xc40000, 0xc40001, sys16_coinctrl_w },
	{ 0xc40006, 0xc40007, sound_command_w },
	{ 0xc46000, 0xc4603f, SYS16_MWA16_EXTRAM2, &sys16_extraram2 },
	{ 0xfe0020, 0xfe003f, MWA16_NOP }, /* config regs */
	{ 0xffc000, 0xffffff, SYS16_MWA16_WORKINGRAM, &sys16_workingram },
MEMORY_END

/***************************************************************************/

static void dduxbl_update_proc( void ){
	sys16_fg_scrollx = (sys16_extraram2[0x0018/2] ^ 0xffff) & 0x01ff;
	sys16_bg_scrollx = (sys16_extraram2[0x0008/2] ^ 0xffff) & 0x01ff;
	sys16_fg_scrolly = sys16_extraram2[0x0010/2] & 0x00ff;
	sys16_bg_scrolly = sys16_extraram2[0x0000/2];

	{
		unsigned char lu = sys16_extraram2[0x0020/2] & 0xff;
		unsigned char ru = sys16_extraram2[0x0022/2] & 0xff;
		unsigned char ld = sys16_extraram2[0x0024/2] & 0xff;
		unsigned char rd = sys16_extraram2[0x0026/2] & 0xff;

		if (lu==4 && ld==4 && ru==5 && rd==5)
		{ /* fix a bug in chicago round (un-tested in MAME) */
			int vs=(*(UINT16 *)(&sys16_workingram[0x36ec]));
			sys16_bg_scrolly = vs & 0xff;
			sys16_fg_scrolly = vs & 0xff;
			if (vs >= 0x100)
			{
				lu=0x26; ru=0x37;
				ld=0x04; rd=0x15;
			} else {
				ld=0x26; rd=0x37;
				lu=0x04; ru=0x15;
			}
		}
		sys16_fg_page[0] = ld&0xf;
		sys16_fg_page[1] = rd&0xf;
		sys16_fg_page[2] = lu&0xf;
		sys16_fg_page[3] = ru&0xf;

		sys16_bg_page[0] = ld>>4;
		sys16_bg_page[1] = rd>>4;
		sys16_bg_page[2] = lu>>4;
		sys16_bg_page[3] = ru>>4;
	}
}

static MACHINE_INIT( dduxbl ){
	static int bank[16] = {
		0,0,0,0,
		0,0,0,4,
		0,0,0,3,
		0,2,0,0
	};
	sys16_obj_bank = bank;

	sys16_patch_code( 0x1eb2e, 0x01 );
	sys16_patch_code( 0x1eb2f, 0x01 );
	sys16_patch_code( 0x1eb3c, 0x00 );
	sys16_patch_code( 0x1eb3d, 0x00 );
	sys16_patch_code( 0x23132, 0x01 );
	sys16_patch_code( 0x23133, 0x01 );
	sys16_patch_code( 0x23140, 0x00 );
	sys16_patch_code( 0x23141, 0x00 );
	sys16_patch_code( 0x24a9a, 0x01 );
	sys16_patch_code( 0x24a9b, 0x01 );
	sys16_patch_code( 0x24aa8, 0x00 );
	sys16_patch_code( 0x24aa9, 0x00 );

	sys16_update_proc = dduxbl_update_proc;
	sys16_sprxoffset = -0x48;
}

static DRIVER_INIT( dduxbl )
{
	int i;

	/* invert the graphics bits on the tiles */
	for (i = 0; i < 0x30000; i++)
		memory_region(REGION_GFX1)[i] ^= 0xff;
}
/***************************************************************************/

INPUT_PORTS_START( dduxbl )
	SYS16_JOY1
	SYS16_JOY2
	SYS16_SERVICE
	SYS16_COINAGE

PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x04, "Easy" )
	PORT_DIPSETTING(    0x06, "Normal" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x18, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x60, 0x60, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x40, "150000" )
	PORT_DIPSETTING(    0x60, "200000" )
	PORT_DIPSETTING(    0x20, "300000" )
	PORT_DIPSETTING(    0x00, "400000" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

INPUT_PORTS_END

/***************************************************************************/

static MACHINE_DRIVER_START( dduxbl )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(dduxbl_readmem,dduxbl_writemem)

	MDRV_MACHINE_INIT(dduxbl)
MACHINE_DRIVER_END

/***************************************************************************/
/* sys16B */

ROM_START( eswat )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* 68000 code - custom CPU 317-0192 */
	ROM_LOAD16_BYTE( "rom-e.bin", 0x000000, 0x40000, CRC(43ca72aa) SHA1(4c6c536f0ef50570992116b50ca816bbc7d42801) )
	ROM_LOAD16_BYTE( "rom-o.bin", 0x000001, 0x40000, CRC(5f018967) SHA1(753cd39bdb51126591b5814d54bb57ed1f77cf22) )

	ROM_REGION( 0x60000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "scr-0.bin", 0x00000, 0x20000, CRC(12f898db) SHA1(f5848a52d75d6204c6b9687fa9146aaec2d56097) )
	ROM_LOAD( "scr-1.bin", 0x20000, 0x20000, CRC(339746d0) SHA1(084a567d5b747c2fc085c5033f56407d6b46faef) )
	ROM_LOAD( "scr-2.bin", 0x40000, 0x20000, CRC(33cf7a55) SHA1(405ed634c393f42544c1fe39c9cfd372f08c3fac) )

	ROM_REGION( 0xc0000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "obj0-o.bin", 0x000001, 0x20000, CRC(2ff5cb9e) SHA1(2468a928515640e1bdd651aaadcbc918661c3312) )
	ROM_LOAD16_BYTE( "obj0-e.bin", 0x000000, 0x20000, CRC(10a27526) SHA1(8299d4888d5d7530d864d7e33f264efe66272b44) )
	ROM_LOAD16_BYTE( "obj1-o.bin", 0x040001, 0x20000, CRC(01b2e832) SHA1(6b7aa350498c54a9fac54fee1e65fcada4284fd6) )
	ROM_LOAD16_BYTE( "obj1-e.bin", 0x040000, 0x20000, CRC(ba3ba6fd) SHA1(799e9899630d417fc508af22e04c7c2526a88ee1) )
	ROM_LOAD16_BYTE( "obj2-o.bin", 0x060001, 0x20000, CRC(d12ef57a) SHA1(e0d6d350ce20d84f12df3ab777b9aaa40b906339) )
	ROM_LOAD16_BYTE( "obj2-e.bin", 0x060000, 0x20000, CRC(54b51ca4) SHA1(2f477885500ac4c0875ae956d574334332e225b6) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "s-prog.bin",  0x00000, 0x08000, CRC(7efecf23) SHA1(2b87af7cfaab5942a3f7b38c987fcba01d3475ab) )
	ROM_LOAD( "sound-0.bin",0x10000, 0x20000, CRC(f451705e) SHA1(2b3d1b3ffbc6ba2285c4141e6fd3447252a31c8b) )
ROM_END

ROM_START( eswatbl )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "eswat_c.rom", 0x000000, 0x10000, CRC(1028cc81) SHA1(24b4cd182419a44f3d6afa1c4273353024eb278f) )
	ROM_LOAD16_BYTE( "eswat_f.rom", 0x000001, 0x10000, CRC(f7b2d388) SHA1(8131ba8f4fa01751b9993c3c6c218c9bd3adb328) )
	ROM_LOAD16_BYTE( "eswat_b.rom", 0x020000, 0x10000, CRC(87c6b1b5) SHA1(a9f29e29a9c0e3daf272dce263a5fd7866642c77) )
	ROM_LOAD16_BYTE( "eswat_e.rom", 0x020001, 0x10000, CRC(937ddf9a) SHA1(9fc73f93e9c4221a4dc778593edc02cb405b2f78) )
	ROM_LOAD16_BYTE( "eswat_a.rom", 0x040000, 0x08000, CRC(2af4fc62) SHA1(f7b1539a5ab9560bd49dfecf44699abccfb649be) )
	ROM_LOAD16_BYTE( "eswat_d.rom", 0x040001, 0x08000, CRC(b4751e19) SHA1(57c9687dc864c163d13dbb89057cd42684a199cd) )

	ROM_REGION( 0xc0000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "ic19.bin", 0x00000, 0x40000, CRC(375a5ec4) SHA1(42b9116bdc0e0a5b1dd667ac1856b4c2252829ba) )
	ROM_LOAD( "ic20.bin", 0x40000, 0x40000, CRC(3b8c757e) SHA1(0b66e8446d059a12e47e2a6fe8f0a333245bb95c) )
	ROM_LOAD( "ic21.bin", 0x80000, 0x40000, CRC(3efca25c) SHA1(0d866bf53a16b52719f73081e933f4db27d72ece) )

	ROM_REGION( 0x180000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "ic9.bin",  0x000001, 0x040000, CRC(0d1530bf) SHA1(bb8626cd98761c1c20cee117d00315c85621ba6a) )
	ROM_LOAD16_BYTE( "ic12.bin", 0x000000, 0x040000, CRC(18ff0799) SHA1(5417223378aef16ee2b4f438d1f8f11a23fe7265) )
	ROM_LOAD16_BYTE( "ic10.bin", 0x080001, 0x040000, CRC(32069246) SHA1(4913009bc72bf4f8b171b14fe06457f5784cab15) )
	ROM_LOAD16_BYTE( "ic13.bin", 0x080000, 0x040000, CRC(a3dfe436) SHA1(640ccc552114d403f35d441574d2f3e4f1d4a8f9) )
	ROM_LOAD16_BYTE( "ic11.bin", 0x100001, 0x040000, CRC(f6b096e0) SHA1(695ad1adbdc29f4d614645867e16de038cf92709) )
	ROM_LOAD16_BYTE( "ic14.bin", 0x100000, 0x040000, CRC(6773fef6) SHA1(91e646ea447be02254d060daf255d26afe0cc79e) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "ic8.bin", 0x0000, 0x8000, CRC(7efecf23) SHA1(2b87af7cfaab5942a3f7b38c987fcba01d3475ab) )
	ROM_LOAD( "ic6.bin", 0x10000, 0x40000, CRC(254347c2) SHA1(bf2d83a69a5be375c7e42e9f7d6e65c1095a354c) )
ROM_END
/***************************************************************************/

static READ16_HANDLER( eswatbl_skip_r ){
	if (activecpu_get_pc()==0x65c) {cpu_spinuntil_int(); return 0xffff;}
	return sys16_workingram[0x0454/2];
}

static MEMORY_READ16_START( eswat_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x400000, 0x40ffff, SYS16_MRA16_TILERAM },
	{ 0x410000, 0x418fff, SYS16_MRA16_TEXTRAM },
	{ 0x440000, 0x440fff, SYS16_MRA16_SPRITERAM },
	{ 0x840000, 0x840fff, SYS16_MRA16_PALETTERAM },
	{ 0xc41002, 0xc41003, input_port_0_word_r }, /* player1 */
	{ 0xc41006, 0xc41007, input_port_1_word_r },/* player2 */
	{ 0xc41000, 0xc41001, input_port_2_word_r }, /* service */
	{ 0xc42002, 0xc42003, input_port_3_word_r }, /* dip1 */
	{ 0xc42000, 0xc42001, input_port_4_word_r }, /* dip2 */
	{ 0xffc454, 0xffc455, eswatbl_skip_r },
	{ 0xffc000, 0xffffff, SYS16_MRA16_WORKINGRAM },
MEMORY_END

static int eswat_tilebank0;

static WRITE16_HANDLER( eswat_tilebank0_w ){
	if( ACCESSING_LSB ){
		eswat_tilebank0 = data&0xff;
	}
}

static MEMORY_WRITE16_START( eswat_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x3e2000, 0x3e2001, eswat_tilebank0_w },
	{ 0x400000, 0x40ffff, SYS16_MWA16_TILERAM, &sys16_tileram },
	{ 0x410000, 0x418fff, SYS16_MWA16_TEXTRAM, &sys16_textram },
	{ 0x440000, 0x440fff, SYS16_MWA16_SPRITERAM, &sys16_spriteram },
	{ 0x840000, 0x840fff, SYS16_MWA16_PALETTERAM, &paletteram16 },
	{ 0xc42006, 0xc42007, sound_command_w },
	{ 0xc40000, 0xc40001, sys16_coinctrl_w },
	{ 0xc80000, 0xc80001, MWA16_NOP },
	{ 0xffc000, 0xffffff, SYS16_MWA16_WORKINGRAM, &sys16_workingram },
MEMORY_END

/***************************************************************************/

static void eswat_update_proc( void ){
	sys16_fg_scrollx = sys16_textram[0x8008/2] ^ 0xffff;
	sys16_bg_scrollx = sys16_textram[0x8018/2] ^ 0xffff;
	sys16_fg_scrolly = sys16_textram[0x8000/2];
	sys16_bg_scrolly = sys16_textram[0x8010/2];

	set_fg_page( sys16_textram[0x8020/2] );
	set_bg_page( sys16_textram[0x8028/2] );

	sys16_tile_bank1 = (sys16_textram[0x8030/2])&0xf;
	sys16_tile_bank0 = eswat_tilebank0;
}

static MACHINE_INIT( eswat ){
	static int bank[] = {
		0,1,	4,5,
		8,9,	12,13,
		2,3,	6,7,
		10,11,	14,15
	};
	sys16_obj_bank = bank;
	sys16_sprxoffset = -0x23c;

	sys16_patch_code( 0x3897, 0x11 );

	sys16_update_proc = eswat_update_proc;
}

static DRIVER_INIT( eswat ){
	sys16_rowscroll_scroll=0x8000;
	sys18_splittab_fg_x=&sys16_textram[0x0f80];
}

/***************************************************************************/

INPUT_PORTS_START( eswat )
	SYS16_JOY1
	SYS16_JOY2
	SYS16_SERVICE
	SYS16_COINAGE

PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, "2 Credits to Start" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Display Flip" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Time" )
	PORT_DIPSETTING(    0x08, "Normal" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x20, "Easy" )
	PORT_DIPSETTING(    0x30, "Normal" )
	PORT_DIPSETTING(    0x10, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x40, "2" )
	PORT_DIPSETTING(    0xc0, "3" )
	PORT_DIPSETTING(    0x80, "4" )
INPUT_PORTS_END

/***************************************************************************/

static MACHINE_DRIVER_START( eswat )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16_7759b)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(eswat_readmem,eswat_writemem)

	MDRV_MACHINE_INIT(eswat)
MACHINE_DRIVER_END

/***************************************************************************/
/* sys16A */
ROM_START( fantzono )
	ROM_REGION( 0x030000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "7385.43", 0x000000, 0x8000, CRC(5cb64450) SHA1(5831405359975dd7d8c6614b20fd9b18a5d6410d) )
	ROM_LOAD16_BYTE( "7382.26", 0x000001, 0x8000, CRC(3fda7416) SHA1(91f34cc8afb4ad8bc783c31d25781a1359c44cfe) )
	ROM_LOAD16_BYTE( "7386.42", 0x010000, 0x8000, CRC(15810ace) SHA1(e61a258ab6601d359f6ad1f37a2b2801bf777d26) )
	ROM_LOAD16_BYTE( "7383.25", 0x010001, 0x8000, CRC(a001e10a) SHA1(04ebb012b10817db36997d0ee877104d512decf8) )
	ROM_LOAD16_BYTE( "7387.41", 0x020000, 0x8000, CRC(0acd335d) SHA1(f39566a2069eefa7682c57c6521ea7a328738d06) )
	ROM_LOAD16_BYTE( "7384.24", 0x020001, 0x8000, CRC(fd909341) SHA1(2f1e01eb7d7b330c9c0dd98e5f8ed4973f0e93fb) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "7388.95", 0x00000, 0x08000, CRC(8eb02f6b) SHA1(80511b944b57541669010bd5a0ca52bc98eabd62) )
	ROM_LOAD( "7389.94", 0x08000, 0x08000, CRC(2f4f71b8) SHA1(ceb39e95cd43904b8e4f89c7227491e139fb3ca6) )
	ROM_LOAD( "7390.93", 0x10000, 0x08000, CRC(d90609c6) SHA1(4232f6ecb21f242c0c8d81e06b88bc742668609f) )

	ROM_REGION( 0x30000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "7392.10", 0x00001, 0x8000, CRC(5bb7c8b6) SHA1(eaa0ed63ac4f66ee285757e842bdd7b005292600) )
	ROM_LOAD16_BYTE( "7396.11", 0x00000, 0x8000, CRC(74ae4b57) SHA1(1f24b1faea765994b85f0e7ac8e944c8da22103f) )
	ROM_LOAD16_BYTE( "7393.17", 0x10001, 0x8000, CRC(14fc7e82) SHA1(ca7caca989a3577dd30ad4f66b0fcce712a454ef) )
	ROM_LOAD16_BYTE( "7397.18", 0x10000, 0x8000, CRC(e05a1e25) SHA1(9691d9f0763b7483ee6912437902f22ab4b78a05) )
	ROM_LOAD16_BYTE( "7394.23", 0x20001, 0x8000, CRC(531ca13f) SHA1(19e68bc515f6021e1145cff4f3f0e083839ee8f3) )
	ROM_LOAD16_BYTE( "7398.24", 0x20000, 0x8000, CRC(68807b49) SHA1(0a189da8cdd2090e76d6d06c55b478abce60542d) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "7535.12", 0x0000, 0x8000, CRC(0cb2126a) SHA1(42b18a81bed58ef59eaad929007eef89ad273dbb) )
ROM_END

ROM_START( fantzone )
	ROM_REGION( 0x030000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr7385a.43", 0x000000, 0x8000, CRC(4091af42) SHA1(1d4fdd32db9f75e5ccaab5766a50249ad71a60af) )
	ROM_LOAD16_BYTE( "epr7382a.26", 0x000001, 0x8000, CRC(77d67bfd) SHA1(886ce4c2d779cedd81f85737ef55fce3c94baa18) )
	ROM_LOAD16_BYTE( "epr7386a.42", 0x010000, 0x8000, CRC(b0a67cd0) SHA1(2e2bf2b7306fc567f7d13f89977543b368c19027) )
	ROM_LOAD16_BYTE( "epr7383a.25", 0x010001, 0x8000, CRC(5f79b2a9) SHA1(de3125bbd0a126fc5a67ba3134cd3f4608ebdfce) )
	ROM_LOAD16_BYTE( "7387.41", 0x020000, 0x8000, CRC(0acd335d) SHA1(f39566a2069eefa7682c57c6521ea7a328738d06) )
	ROM_LOAD16_BYTE( "7384.24", 0x020001, 0x8000, CRC(fd909341) SHA1(2f1e01eb7d7b330c9c0dd98e5f8ed4973f0e93fb) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "7388.95", 0x00000, 0x08000, CRC(8eb02f6b) SHA1(80511b944b57541669010bd5a0ca52bc98eabd62) )
	ROM_LOAD( "7389.94", 0x08000, 0x08000, CRC(2f4f71b8) SHA1(ceb39e95cd43904b8e4f89c7227491e139fb3ca6) )
	ROM_LOAD( "7390.93", 0x10000, 0x08000, CRC(d90609c6) SHA1(4232f6ecb21f242c0c8d81e06b88bc742668609f) )

	ROM_REGION( 0x30000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "7392.10", 0x00001, 0x8000, CRC(5bb7c8b6) SHA1(eaa0ed63ac4f66ee285757e842bdd7b005292600) )
	ROM_LOAD16_BYTE( "7396.11", 0x00000, 0x8000, CRC(74ae4b57) SHA1(1f24b1faea765994b85f0e7ac8e944c8da22103f) )
	ROM_LOAD16_BYTE( "7393.17", 0x10001, 0x8000, CRC(14fc7e82) SHA1(ca7caca989a3577dd30ad4f66b0fcce712a454ef) )
	ROM_LOAD16_BYTE( "7397.18", 0x10000, 0x8000, CRC(e05a1e25) SHA1(9691d9f0763b7483ee6912437902f22ab4b78a05) )
	ROM_LOAD16_BYTE( "7394.23", 0x20001, 0x8000, CRC(531ca13f) SHA1(19e68bc515f6021e1145cff4f3f0e083839ee8f3) )
	ROM_LOAD16_BYTE( "7398.24", 0x20000, 0x8000, CRC(68807b49) SHA1(0a189da8cdd2090e76d6d06c55b478abce60542d) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr7535a.12", 0x0000, 0x8000, CRC(bc1374fa) SHA1(ed2c87ae024dc251e175239f1bccc728fc096548) )
ROM_END


/***************************************************************************/

static READ16_HANDLER( fantzone_skip_r ){
	if (activecpu_get_pc()==0x91b2) {cpu_spinuntil_int(); return 0xffff;}
	return sys16_workingram[0x022a/2];
}

static MEMORY_READ16_START( fantzono_readmem )
	{ 0x000000, 0x02ffff, MRA16_ROM },
	{ 0x400000, 0x40ffff, SYS16_MRA16_TILERAM },
	{ 0x410000, 0x410fff, SYS16_MRA16_TEXTRAM },
	{ 0x440000, 0x440fff, SYS16_MRA16_SPRITERAM },
	{ 0x840000, 0x840fff, SYS16_MRA16_PALETTERAM },
	{ 0xc41002, 0xc41003, input_port_0_word_r }, /* player1 */
	{ 0xc41006, 0xc41007, input_port_1_word_r },/* player2 */
	{ 0xc41000, 0xc41001, input_port_2_word_r }, /* service */
	{ 0xc42000, 0xc42001, input_port_3_word_r }, /* dip1 */
	{ 0xc42002, 0xc42003, input_port_4_word_r }, /* dip2 */
	{ 0xc40000, 0xc40003, SYS16_MRA16_EXTRAM2 },
	{ 0xffc22a, 0xffc22b, fantzone_skip_r },
	{ 0xffc000, 0xffffff, SYS16_MRA16_WORKINGRAM },
MEMORY_END

static MEMORY_WRITE16_START( fantzono_writemem )
	{ 0x000000, 0x02ffff, MWA16_ROM },
	{ 0x400000, 0x40ffff, SYS16_MWA16_TILERAM, &sys16_tileram },
	{ 0x410000, 0x410fff, SYS16_MWA16_TEXTRAM, &sys16_textram },
	{ 0x440000, 0x440fff, SYS16_MWA16_SPRITERAM, &sys16_spriteram },
	{ 0x840000, 0x840fff, SYS16_MWA16_PALETTERAM, &paletteram16 },
	{ 0xc40000, 0xc40001, sound_command_nmi_w },
	{ 0xc40000, 0xc40003, SYS16_MWA16_EXTRAM2, &sys16_extraram2 },
	{ 0xc60000, 0xc60003, MWA16_NOP },
	{ 0xffc000, 0xffffff, SYS16_MWA16_WORKINGRAM, &sys16_workingram },
MEMORY_END

static MEMORY_READ16_START( fantzone_readmem )
	{ 0x000000, 0x02ffff, MRA16_ROM },
	{ 0x400000, 0x40ffff, SYS16_MRA16_TILERAM },
	{ 0x410000, 0x410fff, SYS16_MRA16_TEXTRAM },
	{ 0x440000, 0x440fff, SYS16_MRA16_SPRITERAM },
	{ 0x840000, 0x840fff, SYS16_MRA16_PALETTERAM },
	{ 0xc41002, 0xc41003, input_port_0_word_r }, /* player1 */
	{ 0xc41006, 0xc41007, input_port_1_word_r }, /* player2 */
	{ 0xc41000, 0xc41001, input_port_2_word_r }, /* service */
	{ 0xc42000, 0xc42001, input_port_3_word_r }, /* dip1 */
	{ 0xc42002, 0xc42003, input_port_4_word_r }, /* dip2 */
	{ 0xffc22a, 0xffc22b, fantzone_skip_r },
	{ 0xffc000, 0xffffff, SYS16_MRA16_WORKINGRAM },
MEMORY_END

static MEMORY_WRITE16_START( fantzone_writemem )
	{ 0x000000, 0x02ffff, MWA16_ROM },
	{ 0x400000, 0x40ffff, SYS16_MWA16_TILERAM, &sys16_tileram },
	{ 0x410000, 0x410fff, SYS16_MWA16_TEXTRAM, &sys16_textram },
	{ 0x440000, 0x440fff, SYS16_MWA16_SPRITERAM, &sys16_spriteram },
	{ 0x840000, 0x840fff, SYS16_MWA16_PALETTERAM, &paletteram16 },
	{ 0xc40000, 0xc40001, sound_command_nmi_w },
	{ 0xc40002, 0xc40003, sys16_3d_coinctrl_w },
	{ 0xc60000, 0xc60003, MWA16_NOP },
	{ 0xffc000, 0xffffff, SYS16_MWA16_WORKINGRAM, &sys16_workingram },
MEMORY_END

/***************************************************************************/

static MACHINE_INIT( fantzono ){
	sys16_textmode=1;
	sys16_spritesystem = sys16_sprite_fantzone;
	sys16_sprxoffset = -0xbe;
/*	sys16_fgxoffset = sys16_bgxoffset = 8; */
	sys16_fg_priority_mode=3;				/* fixes end of game priority */
	sys16_fg_priority_value=0xd000;

	sys16_patch_code( 0x20e7, 0x16 );
	sys16_patch_code( 0x30ef, 0x16 );

	/* solving Fantasy Zone scrolling bug */
	sys16_patch_code(0x308f,0x00);

	/* invincible */
/*	sys16_patch_code(0x224e,0x4e);
	sys16_patch_code(0x224f,0x71);
	sys16_patch_code(0x2250,0x4e);
	sys16_patch_code(0x2251,0x71);

	sys16_patch_code(0x2666,0x4e);
	sys16_patch_code(0x2667,0x71);
	sys16_patch_code(0x2668,0x4e);
	sys16_patch_code(0x2669,0x71);

	sys16_patch_code(0x25c0,0x4e);
	sys16_patch_code(0x25c1,0x71);
	sys16_patch_code(0x25c2,0x4e);
	sys16_patch_code(0x25c3,0x71);
*/

	sys16_update_proc = type1_sys16_textram;
}

static MACHINE_INIT( fantzone ){
	sys16_textmode=1;
	sys16_spritesystem = sys16_sprite_fantzone;
	sys16_sprxoffset = -0xbe;
	sys16_fg_priority_mode=3;				/* fixes end of game priority */
	sys16_fg_priority_value=0xd000;

	sys16_patch_code( 0x2135, 0x16 );
	sys16_patch_code( 0x3649, 0x16 );

	/* hack? solving Fantasy Zone scrolling bug */
	sys16_patch_code(0x35e9,0x00);

	sys16_update_proc = type1_sys16_textram;
}

/***************************************************************************/

INPUT_PORTS_START( fantzone )
	SYS16_JOY1
	SYS16_JOY2
	SYS16_SERVICE
	SYS16_COINAGE

PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "240", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x30, 0x30, "Extra Ship Cost" )
	PORT_DIPSETTING(    0x30, "5000" )
	PORT_DIPSETTING(    0x20, "10000" )
	PORT_DIPSETTING(    0x10, "15000" )
	PORT_DIPSETTING(    0x00, "20000" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x80, "Easy" )
	PORT_DIPSETTING(    0xc0, "Normal" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )

INPUT_PORTS_END

/***************************************************************************/

static MACHINE_DRIVER_START( fantzono )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(fantzono_readmem,fantzono_writemem)

	MDRV_MACHINE_INIT(fantzono)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( fantzone )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(fantzone_readmem,fantzone_writemem)

	MDRV_MACHINE_INIT(fantzone)
MACHINE_DRIVER_END

/***************************************************************************/
/* sys16B */
ROM_START( fpoint )
	ROM_REGION( 0x020000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "12591b.bin", 0x000000, 0x10000, CRC(248b3e1b) SHA1(b473c2a057a61896596ac4761e875d72c4f91529) )
	ROM_LOAD16_BYTE( "12590b.bin", 0x000001, 0x10000, CRC(75256e3d) SHA1(87a7d9952f29e49958c135906ac2fd19bdc29b67) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "12595.bin", 0x00000, 0x10000, CRC(5b18d60b) SHA1(8e9c81635dcefa52d1cf53c2937ae560191b5202) )
	ROM_LOAD( "12594.bin", 0x10000, 0x10000, CRC(8bfc4815) SHA1(08d28b65e5024c592a9a289b270774ef5c553cbf) )
	ROM_LOAD( "12593.bin", 0x20000, 0x10000, CRC(cc0582d8) SHA1(92c7d125a6dcb9c5e6e7bd92a5bf3008385ed487) )

	ROM_REGION( 0x20000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "12596.bin", 0x00001, 0x10000, CRC(4a4041f3) SHA1(4c52b30223d8aa80ccdbb196098cb17e64ad6583) )
	ROM_LOAD16_BYTE( "12597.bin", 0x00000, 0x10000, CRC(6961e676) SHA1(7639d2da086b57a9a8d6100fdacf40d97d7c4772) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "12592.bin", 0x0000, 0x8000, CRC(9a8c11bb) SHA1(399f8e9bdd7aaa4d25817fa9cd4bbf413e5baebe) )
ROM_END

ROM_START( fpointbl )
	ROM_REGION( 0x020000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "flpoint.003", 0x000000, 0x10000, CRC(4d6df514) SHA1(168aa1629ab7152ba1984605155406b236954a2c) )
	ROM_LOAD16_BYTE( "flpoint.002", 0x000001, 0x10000, CRC(4dff2ee8) SHA1(bd157d8c168d45e7490a05d5e1e901d9bdda9599) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "flpoint.006", 0x00000, 0x10000, CRC(c539727d) SHA1(56674effe1d273128dddd2ff9e02974ec10f3fff) )
	ROM_LOAD( "flpoint.005", 0x10000, 0x10000, CRC(82c0b8b0) SHA1(e1e2e721cb8ad53df33065582dc90edeba9c3cab) )
	ROM_LOAD( "flpoint.004", 0x20000, 0x10000, CRC(522426ae) SHA1(90fd0a19b30a8a61dc4cfa66a64115596333dcc6) )

	ROM_REGION( 0x20000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "12596.bin", 0x00001, 0x010000, CRC(4a4041f3) SHA1(4c52b30223d8aa80ccdbb196098cb17e64ad6583) )
	ROM_LOAD16_BYTE( "12597.bin", 0x00000, 0x010000, CRC(6961e676) SHA1(7639d2da086b57a9a8d6100fdacf40d97d7c4772) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "flpoint.001", 0x0000, 0x8000, CRC(c5b8e0fe) SHA1(6cf8c67151d8604326fc6dbf976c0635b452a844) )	/* bootleg rom doesn't work, but should be correct! */
ROM_END



ROM_START( fpointbj )
	ROM_REGION( 0x020000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "boot2.003", 0x000000, 0x10000, CRC(6c00d1b0) SHA1(fd0c47b8ca010a64d3ef91980f93854ebc98fbda) )
	ROM_LOAD16_BYTE( "boot2.002", 0x000001, 0x10000, CRC(c1fcd704) SHA1(697bef464e53fb9891ed15ee2d6210107b693b20) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "flpoint.006", 0x00000, 0x10000, CRC(c539727d) SHA1(56674effe1d273128dddd2ff9e02974ec10f3fff) )
	ROM_LOAD( "flpoint.005", 0x10000, 0x10000, CRC(82c0b8b0) SHA1(e1e2e721cb8ad53df33065582dc90edeba9c3cab) )
	ROM_LOAD( "flpoint.004", 0x20000, 0x10000, CRC(522426ae) SHA1(90fd0a19b30a8a61dc4cfa66a64115596333dcc6) )

	ROM_REGION( 0x20000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "12596.bin", 0x00001, 0x010000, CRC(4a4041f3) SHA1(4c52b30223d8aa80ccdbb196098cb17e64ad6583) )
	ROM_LOAD16_BYTE( "12597.bin", 0x00000, 0x010000, CRC(6961e676) SHA1(7639d2da086b57a9a8d6100fdacf40d97d7c4772) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "flpoint.001", 0x0000, 0x8000, CRC(c5b8e0fe) SHA1(6cf8c67151d8604326fc6dbf976c0635b452a844) )	/* bootleg rom doesn't work, but should be correct! */

	/* stuff below isn't used but loaded because it was on the board .. */
	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s129.1",  0x0000, 0x0100, CRC(a7c22d96) SHA1(160deae8053b09c09328325246598b3518c7e20b) )
	ROM_LOAD( "82s123.2",  0x0100, 0x0020, CRC(58bcf8bd) SHA1(e4d3d179b08c0f3424a6bec0f15058fb1b56f8d8) )

	ROM_REGION( 0x800, REGION_USER1, 0 )
	ROM_LOAD( "d2716.rom",  0x0000, 0x0800, CRC(d7fd8ac4) SHA1(87e5f1c24350adab129ad79a1f68af402580f8f0) )
ROM_END

/***************************************************************************/

static READ16_HANDLER( fp_io_service_dummy_r ){
	int data = readinputport( 2 ) & 0xff;
	return (data << 8) + data;
}

static MEMORY_READ16_START( fpoint_readmem )
	{ 0x000000, 0x01ffff, MRA16_ROM },
	{ 0x02002e, 0x020049, fp_io_service_dummy_r },
	{ 0x601002, 0x601003, input_port_0_word_r }, /* player1 */
	{ 0x601004, 0x601005, input_port_1_word_r }, /* player2 */
	{ 0x601000, 0x601001, input_port_2_word_r }, /* service */
	{ 0x600000, 0x600001, input_port_4_word_r }, /* dip2 */
	{ 0x600002, 0x600003, input_port_3_word_r }, /* dip1 */
	{ 0x400000, 0x40ffff, SYS16_MRA16_TILERAM },
	{ 0x410000, 0x410fff, SYS16_MRA16_TEXTRAM },
	{ 0x440000, 0x440fff, SYS16_MRA16_SPRITERAM },
	{ 0x44302a, 0x44304d, fp_io_service_dummy_r },
	{ 0x840000, 0x840fff, SYS16_MRA16_PALETTERAM },
	{ 0xfe003e, 0xfe003f, fp_io_service_dummy_r },
	{ 0xffc000, 0xffffff, SYS16_MRA16_WORKINGRAM },
MEMORY_END

static MEMORY_WRITE16_START( fpoint_writemem )
	{ 0x000000, 0x01ffff, MWA16_ROM },
	{ 0x600006, 0x600007, sound_command_w },
	{ 0x410000, 0x410fff, SYS16_MWA16_TEXTRAM, &sys16_textram },
	{ 0x400000, 0x40ffff, SYS16_MWA16_TILERAM, &sys16_tileram },
	{ 0x440000, 0x440fff, SYS16_MWA16_SPRITERAM, &sys16_spriteram },
	{ 0x840000, 0x840fff, SYS16_MWA16_PALETTERAM, &paletteram16 },
	{ 0xffc000, 0xffffff, SYS16_MWA16_WORKINGRAM, &sys16_workingram },
MEMORY_END

/***************************************************************************/

static MACHINE_INIT( fpoint ){

	sys16_patch_code( 0x454, 0x33 );
	sys16_patch_code( 0x455, 0xf8 );
	sys16_patch_code( 0x456, 0xe0 );
	sys16_patch_code( 0x457, 0xe2 );
	sys16_patch_code( 0x8ce8, 0x16 );
	sys16_patch_code( 0x8ce9, 0x66 );
	sys16_patch_code( 0x17687, 0x00 );
	sys16_patch_code( 0x7bed, 0x04 );

	sys16_patch_code( 0x7ea8, 0x61 );
	sys16_patch_code( 0x7ea9, 0x00 );
	sys16_patch_code( 0x7eaa, 0x84 );
	sys16_patch_code( 0x7eab, 0x16 );
	sys16_patch_code( 0x2c0, 0xe7 );
	sys16_patch_code( 0x2c1, 0x48 );
	sys16_patch_code( 0x2c2, 0xe7 );
	sys16_patch_code( 0x2c3, 0x49 );
	sys16_patch_code( 0x2c4, 0x04 );
	sys16_patch_code( 0x2c5, 0x40 );
	sys16_patch_code( 0x2c6, 0x00 );
	sys16_patch_code( 0x2c7, 0x10 );
	sys16_patch_code( 0x2c8, 0x4e );
	sys16_patch_code( 0x2c9, 0x75 );

	sys16_update_proc = type0_sys16_textram;
}

static DRIVER_INIT( fpointbl ){
	int i;

	/* invert the graphics bits on the tiles */
	for (i = 0; i < 0x30000; i++)
		memory_region(REGION_GFX1)[i] ^= 0xff;
}
/***************************************************************************/

INPUT_PORTS_START( fpoint )
PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )

PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )

	SYS16_SERVICE
	SYS16_COINAGE

PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x20, "Easy" )
	PORT_DIPSETTING(    0x30, "Normal" )
	PORT_DIPSETTING(    0x10, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x40, 0x40, "Clear round allowed" ) /* Use button 3 */
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x40, "2" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

INPUT_PORTS_END

INPUT_PORTS_START( fpointbj )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )

	SYS16_SERVICE
	SYS16_COINAGE

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) ) /* not used according to manual */
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) ) /* not used according to manual */
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) ) /* not used according to manual */
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x20, "Easy" )
	PORT_DIPSETTING(    0x30, "Normal" )
	PORT_DIPSETTING(    0x10, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) ) /* not used according to manual */
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "2 Cell Move Mode" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

INPUT_PORTS_END

/***************************************************************************/

static MACHINE_DRIVER_START( fpoint )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(fpoint_readmem,fpoint_writemem)

	MDRV_MACHINE_INIT(fpoint)
MACHINE_DRIVER_END

/*
	Flash Point (Datsu bootlegs = fpointbl, fpointbj)
	Has sound latch at $E000 instead of I/O ports $C0-FF
*/
static MEMORY_READ_START( fpointbl_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xe000, 0xe000, soundlatch_r },
	{ 0xf800, 0xffff, MRA_RAM },
MEMORY_END

static MACHINE_DRIVER_START( fpointbl )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(fpoint_readmem,fpoint_writemem)

	MDRV_CPU_MODIFY("sound")
	MDRV_CPU_MEMORY(fpointbl_sound_readmem,sound_writemem)

	MDRV_MACHINE_INIT(fpoint)
MACHINE_DRIVER_END

/***************************************************************************/
/* sys16B */
ROM_START( goldnaxe )
	ROM_REGION( 0x0c0000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr12523.a7", 0x00000, 0x20000, CRC(8e6128d7) SHA1(b8de216f4ca08815ca98d39a773024d191d21b4d) )
	ROM_LOAD16_BYTE( "epr12522.a5", 0x00001, 0x20000, CRC(b6c35160) SHA1(88015d0a486f56911360362c96a82f36a13de886) )
	/* empty 0x40000 - 0x80000 */
	ROM_LOAD16_BYTE( "epr12521.a8", 0x80000, 0x20000, CRC(5001d713) SHA1(68cf3f48d6e440e5b800503a211adda02107d956) )
	ROM_LOAD16_BYTE( "epr12519.a6", 0x80001, 0x20000, CRC(4438ca8e) SHA1(0af53d64f06abf41f4c46540d28d5f008a4835a3) )

	ROM_REGION( 0x60000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "epr12385", 0x00000, 0x20000, CRC(b8a4e7e0) SHA1(9b36f50209d45a835ded53eb045f63c649b02fc9) )
	ROM_LOAD( "epr12386", 0x20000, 0x20000, CRC(25d7d779) SHA1(2de14a76a5176d5abc7e7f7f723146c620927610) )
	ROM_LOAD( "epr12387", 0x40000, 0x20000, CRC(c7fcadf3) SHA1(5f0fd600a75a02749935af21e1e0d2c714c6417e) )

	ROM_REGION( 0x180000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "mpr12378.b1", 0x000001, 0x40000, CRC(119e5a82) SHA1(261ed2bc4ebac7142e2ecca9f03c91242e792a98) )
	ROM_LOAD16_BYTE( "mpr12379.b4", 0x000000, 0x40000, CRC(1a0e8c57) SHA1(674f1ae7db632876fff346e76786801ae19d9799) )
	ROM_LOAD16_BYTE( "mpr12380.b2", 0x080001, 0x40000, CRC(bb2c0853) SHA1(3f3b546d078f22d787c93ee74d9ad3a6e84383ac) )
	ROM_LOAD16_BYTE( "mpr12381.b5", 0x080000, 0x40000, CRC(81ba6ecc) SHA1(7f59e4d86a192b97e92729371b78c3f1c784a0b5) )
	ROM_LOAD16_BYTE( "mpr12382.b3", 0x100001, 0x40000, CRC(81601c6f) SHA1(604bc5613c6c734a06860303ba36d61bb54508a0) )
	ROM_LOAD16_BYTE( "mpr12383.b6", 0x100000, 0x40000, CRC(5dbacf7a) SHA1(236866fb94672b13cbb2cb479324e61de87eeb34) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr12390",     0x00000, 0x08000, CRC(399fc5f5) SHA1(6f290b36dc71ff4759598e2a9c185a8945a3c9e7) )
	ROM_LOAD( "mpr12384.a11", 0x10000, 0x20000, CRC(6218d8e7) SHA1(5a745c750efb4a61716f99befb7ed14cc84e9973) )
ROM_END

ROM_START( goldnaxj )
	ROM_REGION( 0x0c0000, REGION_CPU1, 0 ) /* 68000 code */
/* Custom cpu 317-0121 */
	ROM_LOAD16_BYTE( "epr12540.a7", 0x00000, 0x20000, CRC(0c7ccc6d) SHA1(25bc29eee731befc665472c2c1998cac8447cc21) )
	ROM_LOAD16_BYTE( "epr12539.a5", 0x00001, 0x20000, CRC(1f24f7d0) SHA1(a09cdf394c03069707f7ed400b8fbdc13674fa74) )
	/* emtpy 0x40000 - 0x80000 */
	ROM_LOAD16_BYTE( "epr12521.a8", 0x80000, 0x20000, CRC(5001d713) SHA1(68cf3f48d6e440e5b800503a211adda02107d956) )
	ROM_LOAD16_BYTE( "epr12519.a6", 0x80001, 0x20000, CRC(4438ca8e) SHA1(0af53d64f06abf41f4c46540d28d5f008a4835a3) )

	ROM_REGION( 0x60000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "epr12385", 0x00000, 0x20000, CRC(b8a4e7e0) SHA1(9b36f50209d45a835ded53eb045f63c649b02fc9) )
	ROM_LOAD( "epr12386", 0x20000, 0x20000, CRC(25d7d779) SHA1(2de14a76a5176d5abc7e7f7f723146c620927610) )
	ROM_LOAD( "epr12387", 0x40000, 0x20000, CRC(c7fcadf3) SHA1(5f0fd600a75a02749935af21e1e0d2c714c6417e) )

	ROM_REGION( 0x180000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "mpr12378.b1", 0x000001, 0x40000, CRC(119e5a82) SHA1(261ed2bc4ebac7142e2ecca9f03c91242e792a98) )
	ROM_LOAD16_BYTE( "mpr12379.b4", 0x000000, 0x40000, CRC(1a0e8c57) SHA1(674f1ae7db632876fff346e76786801ae19d9799) )
	ROM_LOAD16_BYTE( "mpr12380.b2", 0x080001, 0x40000, CRC(bb2c0853) SHA1(3f3b546d078f22d787c93ee74d9ad3a6e84383ac) )
	ROM_LOAD16_BYTE( "mpr12381.b5", 0x080000, 0x40000, CRC(81ba6ecc) SHA1(7f59e4d86a192b97e92729371b78c3f1c784a0b5) )
	ROM_LOAD16_BYTE( "mpr12382.b3", 0x100001, 0x40000, CRC(81601c6f) SHA1(604bc5613c6c734a06860303ba36d61bb54508a0) )
	ROM_LOAD16_BYTE( "mpr12383.b6", 0x100000, 0x40000, CRC(5dbacf7a) SHA1(236866fb94672b13cbb2cb479324e61de87eeb34) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr12390",     0x00000, 0x08000, CRC(399fc5f5) SHA1(6f290b36dc71ff4759598e2a9c185a8945a3c9e7) )
	ROM_LOAD( "mpr12384.a11", 0x10000, 0x20000, CRC(6218d8e7) SHA1(5a745c750efb4a61716f99befb7ed14cc84e9973) )
ROM_END

ROM_START( goldnabl )
	ROM_REGION( 0x0c0000, REGION_CPU1, 0 ) /* 68000 code */
/* protected code */
	ROM_LOAD16_BYTE( "ga6.a22", 0x00000, 0x10000, CRC(f95b459f) SHA1(dadf66d63454ed62fefa521d4fed249d28c63778) )
	ROM_LOAD16_BYTE( "ga4.a20", 0x00001, 0x10000, CRC(83eabdf5) SHA1(1effef966f513fbdec2026d535658e17ef7dea51) )
	ROM_LOAD16_BYTE( "ga11.a27",0x20000, 0x10000, CRC(f4ef9349) SHA1(3ffa335e74ffbc10f80387268da659643c566897) )
	ROM_LOAD16_BYTE( "ga8.a24", 0x20001, 0x10000, CRC(37a65839) SHA1(6e8055d91b840afd8526041d3752c0a55eaebe0c) )
	/* emtpy 0x40000 - 0x80000 */
	ROM_LOAD16_BYTE( "epr12521.a8", 0x80000, 0x20000, CRC(5001d713) SHA1(68cf3f48d6e440e5b800503a211adda02107d956) )
	ROM_LOAD16_BYTE( "epr12519.a6", 0x80001, 0x20000, CRC(4438ca8e) SHA1(0af53d64f06abf41f4c46540d28d5f008a4835a3) )

	ROM_REGION( 0x60000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "ga33.b16", 0x00000, 0x10000, CRC(84587263) SHA1(3a88c8578a477a487a0a214a367042b9739f39eb) )
	ROM_LOAD( "ga32.b15", 0x10000, 0x10000, CRC(63d72388) SHA1(ba0a582b1daf3a1e316237efbad17fcc0381643f) )
	ROM_LOAD( "ga31.b14", 0x20000, 0x10000, CRC(f8b6ae4f) SHA1(55132c98955107e4b247992f7917a6ce588460a7) )
	ROM_LOAD( "ga30.b13", 0x30000, 0x10000, CRC(e29baf4f) SHA1(3761cb2217599fe3f2f860f9395930b96ec52f47) )
	ROM_LOAD( "ga29.b12", 0x40000, 0x10000, CRC(22f0667e) SHA1(2d11b2ce105a3db9c914942cace85aff17deded9) )
	ROM_LOAD( "ga28.b11", 0x50000, 0x10000, CRC(afb1a7e4) SHA1(726fded9db72a881128b43f449d2baf450131f63) )

	ROM_REGION( 0x180000, REGION_GFX2, 0 ) /* sprites */
	/* wrong! */
	ROM_LOAD16_BYTE( "ga34.b17", 		0x000001, 0x10000, CRC(28ba70c8) SHA1(a6f33e1404928b6d1006943494646d6cfbd60a4b) )
	ROM_LOAD16_BYTE( "ga35.b18", 		0x010000, 0x10000, CRC(2ed96a26) SHA1(edcf915243e6f92d31cdfc53965438f6b6bff51d) )
	ROM_LOAD16_BYTE( "ga23.a14", 		0x020001, 0x10000, CRC(84dccc5b) SHA1(10263d98d663f1170c3203066f391075a1d64ff5) )
	ROM_LOAD16_BYTE( "ga18.a9",  		0x030000, 0x10000, CRC(de346006) SHA1(65aa489373b6d2cccbb024f13fc190a7cae86274) )
	ROM_LOAD16_BYTE( "mpr12379.b4", 	0x040001, 0x40000, CRC(1a0e8c57) SHA1(674f1ae7db632876fff346e76786801ae19d9799) )
	ROM_LOAD16_BYTE( "ga36.b19", 		0x080000, 0x10000, CRC(101d2fff) SHA1(1de1390c5f55f192491053c8aac31be3389aab2b) )
	ROM_LOAD16_BYTE( "ga37.b20", 		0x090001, 0x10000, CRC(677e64a6) SHA1(e3d0d31097017c6cb1a7f41292783f18ce13b41c) )
	ROM_LOAD16_BYTE( "ga19.a10", 		0x0a0000, 0x10000, CRC(11794d05) SHA1(eef52d7a644dbcc5f983222f163445a725286a32) )
	ROM_LOAD16_BYTE( "ga20.a11", 		0x0b0001, 0x10000, CRC(ad1c1c90) SHA1(155f17593cfab1a117bb755b1edd0c473d455f91) )
	ROM_LOAD16_BYTE( "mpr12381.b5",	0x0c0000, 0x40000, CRC(81ba6ecc) SHA1(7f59e4d86a192b97e92729371b78c3f1c784a0b5) )
	ROM_LOAD16_BYTE( "mpr12382.b3",	0x100001, 0x40000, CRC(81601c6f) SHA1(604bc5613c6c734a06860303ba36d61bb54508a0) )
	ROM_LOAD16_BYTE( "mpr12383.b6",	0x140000, 0x40000, CRC(5dbacf7a) SHA1(236866fb94672b13cbb2cb479324e61de87eeb34) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr12390",     0x00000, 0x08000, CRC(399fc5f5) SHA1(6f290b36dc71ff4759598e2a9c185a8945a3c9e7) )
	ROM_LOAD( "mpr12384.a11", 0x10000, 0x20000, CRC(6218d8e7) SHA1(5a745c750efb4a61716f99befb7ed14cc84e9973) )
ROM_END


/***************************************************************************/

static READ16_HANDLER( goldnaxe_skip_r ){
	if (activecpu_get_pc()==0x3cb0) {cpu_spinuntil_int(); return 0xffff;}
	return sys16_workingram[0x2c1c/2];
}

static READ16_HANDLER( ga_io_players_r ) {
	return (readinputport(0) << 8) | readinputport(1);
}
static READ16_HANDLER( ga_io_service_r ){
	return (input_port_2_word_r(0,0) << 8) | (sys16_workingram[0x2c96/2] & 0x00ff);
}

static MEMORY_READ16_START( goldnaxe_readmem )
	{ 0x000000, 0x0bffff, MRA16_ROM },
	{ 0x100000, 0x10ffff, SYS16_MRA16_TILERAM },
	{ 0x110000, 0x110fff, SYS16_MRA16_TEXTRAM },
	{ 0x140000, 0x140fff, SYS16_MRA16_PALETTERAM },
	{ 0x1f0000, 0x1f0003, SYS16_MRA16_EXTRAM },
	{ 0x200000, 0x200fff, SYS16_MRA16_SPRITERAM },
	{ 0xc41002, 0xc41003, input_port_0_word_r }, /* player1 */
	{ 0xc41006, 0xc41007, input_port_1_word_r }, /* player2 */
	{ 0xc41000, 0xc41001, input_port_2_word_r }, /* service */
	{ 0xc42002, 0xc42003, input_port_3_word_r }, /* dip1 */
	{ 0xc42000, 0xc42001, input_port_4_word_r },/* dip2 */
	{ 0xffecd0, 0xffecd1, ga_io_players_r },
	{ 0xffec96, 0xffec97, ga_io_service_r },
	{ 0xffec1c, 0xffec1d, goldnaxe_skip_r },
	{ 0xffc000, 0xffffff, SYS16_MRA16_WORKINGRAM },
MEMORY_END

static WRITE16_HANDLER( ga_sound_command_w ){
	COMBINE_DATA( &sys16_workingram[(0xecfc-0xc000)/2] );
	if( ACCESSING_MSB ){
		soundlatch_w( 0,data>>8 );
		cpu_set_irq_line( 1, 0, HOLD_LINE );
	}
}

static WRITE16_HANDLER( goldnaxe_prot_w ){
	sys16_workingram[(0xecd8 - 0xc000)/2] = 0x048c;
	sys16_workingram[(0xecda - 0xc000)/2] = 0x159d;
	sys16_workingram[(0xecdc - 0xc000)/2] = 0x26ae;
	sys16_workingram[(0xecde - 0xc000)/2] = 0x37bf;
	COMBINE_DATA( &sys16_workingram[(0xec1c-0xc000)/2] );
}

static MEMORY_WRITE16_START( goldnaxe_writemem )
	{ 0x000000, 0x0bffff, MWA16_ROM },
	{ 0x100000, 0x10ffff, SYS16_MWA16_TILERAM, &sys16_tileram },
	{ 0x110000, 0x110fff, SYS16_MWA16_TEXTRAM, &sys16_textram },
	{ 0x140000, 0x140fff, SYS16_MWA16_PALETTERAM, &paletteram16 },
	{ 0x1f0000, 0x1f0003, SYS16_MWA16_EXTRAM, &sys16_extraram },
	{ 0x200000, 0x200fff, SYS16_MWA16_SPRITERAM, &sys16_spriteram },
	{ 0xc40000, 0xc40001, sys16_coinctrl_w },
	{ 0xc43000, 0xc43001, MWA16_NOP }, /* ? */
/*	{ 0xfe0006, 0xfe0007, MWA16_NOP }, I think this is the real sound out */
	{ 0xffec1c, 0xffec1d, goldnaxe_prot_w },/* how does this really work? */
	{ 0xffecfc, 0xffecfd, ga_sound_command_w },/* probably just a buffer */
	{ 0xffc000, 0xffffff, SYS16_MWA16_WORKINGRAM, &sys16_workingram }, /* fails SCRATCH RAM test because of hacks */
/*	{ 0xfffc00, 0xffffff, MWA16_NOP },  0x400 bytes; battery backed up */
MEMORY_END

/***************************************************************************/

static void goldnaxe_update_proc( void ){
	type0_sys16_textram();
	set_tile_bank( sys16_workingram[0x2c94/2] );
}

static MACHINE_INIT( goldnaxe ){
	static int bank[16] = {
		0,1,4,5,
		8,9,0,0,
		2,3,6,7,
		10,11,0,0
	};
	sys16_obj_bank = bank;

/* protection patch; no longer needed */
/*	sys16_patch_code( 0x3CB2, 0x60 ); */
/*	sys16_patch_code( 0x3CB3, 0x1e ); */

	sys16_update_proc = goldnaxe_update_proc;
}

static DRIVER_INIT( goldnabl ){
	int i;

	/* invert the graphics bits on the tiles */
	for (i = 0; i < 0x60000; i++)
		memory_region(REGION_GFX1)[i] ^= 0xff;
}

/***************************************************************************/

INPUT_PORTS_START( goldnaxe )
	SYS16_JOY1
	SYS16_JOY2
	SYS16_SERVICE
	SYS16_COINAGE

PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, "Credits needed" )
	PORT_DIPSETTING(    0x01, "1 to start, 1 to continue" )
	PORT_DIPSETTING(    0x00, "2 to start, 1 to continue" )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPSETTING(    0x0c, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x30, 0x30, "Energy Meter" )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

/***************************************************************************/

static MACHINE_DRIVER_START( goldnaxe )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16_7759b)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(goldnaxe_readmem,goldnaxe_writemem)

	MDRV_MACHINE_INIT(goldnaxe)
MACHINE_DRIVER_END

/***************************************************************************/
/* sys16B */
ROM_START( goldnaxa )
	ROM_REGION( 0x0c0000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr12545.a2", 0x00000, 0x40000, CRC(a97c4e4d) SHA1(41cda15ae56185725233db669d9f8c4a8c1eb1c3) )
	ROM_LOAD16_BYTE( "epr12544.a1", 0x00001, 0x40000, CRC(5e38f668) SHA1(3b15a9a30adde6e852c439c8e6e45875b66252cb) )

	ROM_REGION( 0x60000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "epr12385", 0x00000, 0x20000, CRC(b8a4e7e0) SHA1(9b36f50209d45a835ded53eb045f63c649b02fc9) )
	ROM_LOAD( "epr12386", 0x20000, 0x20000, CRC(25d7d779) SHA1(2de14a76a5176d5abc7e7f7f723146c620927610) )
	ROM_LOAD( "epr12387", 0x40000, 0x20000, CRC(c7fcadf3) SHA1(5f0fd600a75a02749935af21e1e0d2c714c6417e) )

	ROM_REGION( 0x180000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "mpr12378.b1", 0x000001, 0x40000, CRC(119e5a82) SHA1(261ed2bc4ebac7142e2ecca9f03c91242e792a98) )
	ROM_LOAD16_BYTE( "mpr12379.b4", 0x000000, 0x40000, CRC(1a0e8c57) SHA1(674f1ae7db632876fff346e76786801ae19d9799) )
	ROM_LOAD16_BYTE( "mpr12380.b2", 0x080001, 0x40000, CRC(bb2c0853) SHA1(3f3b546d078f22d787c93ee74d9ad3a6e84383ac) )
	ROM_LOAD16_BYTE( "mpr12381.b5", 0x080000, 0x40000, CRC(81ba6ecc) SHA1(7f59e4d86a192b97e92729371b78c3f1c784a0b5) )
	ROM_LOAD16_BYTE( "mpr12382.b3", 0x100001, 0x40000, CRC(81601c6f) SHA1(604bc5613c6c734a06860303ba36d61bb54508a0) )
	ROM_LOAD16_BYTE( "mpr12383.b6", 0x100000, 0x40000, CRC(5dbacf7a) SHA1(236866fb94672b13cbb2cb479324e61de87eeb34) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr12390",     0x00000, 0x08000, CRC(399fc5f5) SHA1(6f290b36dc71ff4759598e2a9c185a8945a3c9e7) )
	ROM_LOAD( "mpr12384.a11", 0x10000, 0x20000, CRC(6218d8e7) SHA1(5a745c750efb4a61716f99befb7ed14cc84e9973) )
ROM_END

ROM_START( goldnaxb )
	ROM_REGION( 0x0c0000, REGION_CPU1, 0 ) /* 68000 code */
/* Custom 68000 ver 317-0110 */
	ROM_LOAD16_BYTE( "epr12389.a2", 0x00000, 0x40000, CRC(35d5fa77) SHA1(b16b312eb1c91c412fee61002599812e30e321ee) )
	ROM_LOAD16_BYTE( "epr12388.a1", 0x00001, 0x40000, CRC(72952a93) SHA1(b31888429ad81388a96333dc0b2c7e2223134834) )

	ROM_REGION( 0x60000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "epr12385", 0x00000, 0x20000, CRC(b8a4e7e0) SHA1(9b36f50209d45a835ded53eb045f63c649b02fc9) )
	ROM_LOAD( "epr12386", 0x20000, 0x20000, CRC(25d7d779) SHA1(2de14a76a5176d5abc7e7f7f723146c620927610) )
	ROM_LOAD( "epr12387", 0x40000, 0x20000, CRC(c7fcadf3) SHA1(5f0fd600a75a02749935af21e1e0d2c714c6417e) )

	ROM_REGION( 0x180000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "mpr12378.b1", 0x000001, 0x40000, CRC(119e5a82) SHA1(261ed2bc4ebac7142e2ecca9f03c91242e792a98) )
	ROM_LOAD16_BYTE( "mpr12379.b4", 0x000000, 0x40000, CRC(1a0e8c57) SHA1(674f1ae7db632876fff346e76786801ae19d9799) )
	ROM_LOAD16_BYTE( "mpr12380.b2", 0x080001, 0x40000, CRC(bb2c0853) SHA1(3f3b546d078f22d787c93ee74d9ad3a6e84383ac) )
	ROM_LOAD16_BYTE( "mpr12381.b5", 0x080000, 0x40000, CRC(81ba6ecc) SHA1(7f59e4d86a192b97e92729371b78c3f1c784a0b5) )
	ROM_LOAD16_BYTE( "mpr12382.b3", 0x100001, 0x40000, CRC(81601c6f) SHA1(604bc5613c6c734a06860303ba36d61bb54508a0) )
	ROM_LOAD16_BYTE( "mpr12383.b6", 0x100000, 0x40000, CRC(5dbacf7a) SHA1(236866fb94672b13cbb2cb479324e61de87eeb34) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr12390",     0x00000, 0x08000, CRC(399fc5f5) SHA1(6f290b36dc71ff4759598e2a9c185a8945a3c9e7) )
	ROM_LOAD( "mpr12384.a11", 0x10000, 0x20000, CRC(6218d8e7) SHA1(5a745c750efb4a61716f99befb7ed14cc84e9973) )
ROM_END

ROM_START( goldnaxc )
	ROM_REGION( 0x0c0000, REGION_CPU1, 0 ) /* 68000 code */
/* Custom 68000 ver 317-0122 */
	ROM_LOAD16_BYTE( "epr12543.a2", 0x00000, 0x40000, CRC(b0df9ca4) SHA1(240f3c2998f969569d992f796e006f5ea4434e55) )
	ROM_LOAD16_BYTE( "epr12542.a1", 0x00001, 0x40000, CRC(b7994d3c) SHA1(87570f23826922fca465c69df6b892c59f14e103) )

	ROM_REGION( 0x60000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "epr12385", 0x00000, 0x20000, CRC(b8a4e7e0) SHA1(9b36f50209d45a835ded53eb045f63c649b02fc9) )
	ROM_LOAD( "epr12386", 0x20000, 0x20000, CRC(25d7d779) SHA1(2de14a76a5176d5abc7e7f7f723146c620927610) )
	ROM_LOAD( "epr12387", 0x40000, 0x20000, CRC(c7fcadf3) SHA1(5f0fd600a75a02749935af21e1e0d2c714c6417e) )

	ROM_REGION( 0x180000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "mpr12378.b1", 0x000001, 0x40000, CRC(119e5a82) SHA1(261ed2bc4ebac7142e2ecca9f03c91242e792a98) )
	ROM_LOAD16_BYTE( "mpr12379.b4", 0x000000, 0x40000, CRC(1a0e8c57) SHA1(674f1ae7db632876fff346e76786801ae19d9799) )
	ROM_LOAD16_BYTE( "mpr12380.b2", 0x080001, 0x40000, CRC(bb2c0853) SHA1(3f3b546d078f22d787c93ee74d9ad3a6e84383ac) )
	ROM_LOAD16_BYTE( "mpr12381.b5", 0x080000, 0x40000, CRC(81ba6ecc) SHA1(7f59e4d86a192b97e92729371b78c3f1c784a0b5) )
	ROM_LOAD16_BYTE( "mpr12382.b3", 0x100001, 0x40000, CRC(81601c6f) SHA1(604bc5613c6c734a06860303ba36d61bb54508a0) )
	ROM_LOAD16_BYTE( "mpr12383.b6", 0x100000, 0x40000, CRC(5dbacf7a) SHA1(236866fb94672b13cbb2cb479324e61de87eeb34) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr12390",     0x00000, 0x08000, CRC(399fc5f5) SHA1(6f290b36dc71ff4759598e2a9c185a8945a3c9e7) )
	ROM_LOAD( "mpr12384.a11", 0x10000, 0x20000, CRC(6218d8e7) SHA1(5a745c750efb4a61716f99befb7ed14cc84e9973) )
ROM_END


/***************************************************************************/

static READ16_HANDLER( goldnaxa_skip_r ){
	if (activecpu_get_pc()==0x3ca0) {cpu_spinuntil_int(); return 0xffff;}
	return sys16_workingram[0x2c1c/2];
}

/* This version has somekind of hardware comparitor for collision detection, */
/* and a hardware multiplier. */
static data16_t ga_hardware_collision_data[5];
static WRITE16_HANDLER( ga_hardware_collision_w )
{
	static int bit=1;

	COMBINE_DATA( &ga_hardware_collision_data[offset] );
	if( offset==4/2 )
{
		if( ga_hardware_collision_data[2] <= ga_hardware_collision_data[0] &&
			ga_hardware_collision_data[2] >= ga_hardware_collision_data[1])
		{
			ga_hardware_collision_data[4] |=bit;
		}
		bit=bit<<1;
	}
	else if( offset==8/2 ) bit=1;
}

static READ16_HANDLER( ga_hardware_collision_r )
{
	return ga_hardware_collision_data[4];
}

static data16_t ga_hardware_multiplier_data[4];
static WRITE16_HANDLER( ga_hardware_multiplier_w )
{
	COMBINE_DATA( &ga_hardware_multiplier_data[offset] );
}

static READ16_HANDLER( ga_hardware_multiplier_r )
{
	if(offset==6/2)
		return ga_hardware_multiplier_data[0] * ga_hardware_multiplier_data[1];
	else
		return ga_hardware_multiplier_data[offset];
}

static MEMORY_READ16_START( goldnaxa_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x100000, 0x10ffff, SYS16_MRA16_TILERAM },
	{ 0x110000, 0x110fff, SYS16_MRA16_TEXTRAM },
	{ 0x140000, 0x140fff, SYS16_MRA16_PALETTERAM },
	{ 0x1e0008, 0x1e0009, ga_hardware_collision_r },
	{ 0x1f0000, 0x1f0007, ga_hardware_multiplier_r },
	{ 0x1f1008, 0x1f1009, ga_hardware_collision_r },
	{ 0x1f2000, 0x1f2003, SYS16_MRA16_EXTRAM },
	{ 0x200000, 0x200fff, SYS16_MRA16_SPRITERAM },
	{ 0xc41002, 0xc41003, input_port_0_word_r }, /* player1 */
	{ 0xc41006, 0xc41007, input_port_1_word_r }, /* player2 */
	{ 0xc41000, 0xc41001, input_port_2_word_r }, /* service */
	{ 0xc42002, 0xc42003, input_port_3_word_r }, /* dip1 */
	{ 0xc42000, 0xc42001, input_port_4_word_r }, /* dip2 */
	{ 0xffecd0, 0xffecd1, ga_io_players_r },
	{ 0xffec96, 0xffec97, ga_io_service_r },
	{ 0xffec1c, 0xffec1d, goldnaxa_skip_r },
	{ 0xffc000, 0xffffff, SYS16_MRA16_WORKINGRAM },
MEMORY_END

static MEMORY_WRITE16_START( goldnaxa_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x100000, 0x10ffff, SYS16_MWA16_TILERAM, &sys16_tileram },
	{ 0x110000, 0x110fff, SYS16_MWA16_TEXTRAM, &sys16_textram },
	{ 0x140000, 0x140fff, SYS16_MWA16_PALETTERAM, &paletteram16 },
	{ 0x1e0000, 0x1e0009, ga_hardware_collision_w },
	{ 0x1f0000, 0x1f0003, ga_hardware_multiplier_w },
	{ 0x1f1000, 0x1f1009, ga_hardware_collision_w },
	{ 0x1f2000, 0x1f2003, SYS16_MWA16_EXTRAM, &sys16_extraram },
	{ 0x200000, 0x200fff, SYS16_MWA16_SPRITERAM, &sys16_spriteram },
	{ 0xc40000, 0xc40001, sys16_coinctrl_w },
	{ 0xffecfc, 0xffecfd, ga_sound_command_w },
	{ 0xffc000, 0xffffff, SYS16_MWA16_WORKINGRAM, &sys16_workingram },
MEMORY_END

/***************************************************************************/

static MACHINE_INIT( goldnaxa ){
	static int bank[16] = {
		0,1,4,5,
		8,9,0,0,
		2,3,6,7,
		10,11,0,0
	};
	sys16_obj_bank = bank;
	/*? */
	sys16_patch_code( 0x3CA2, 0x60 );
	sys16_patch_code( 0x3CA3, 0x1e );

	sys16_update_proc = goldnaxe_update_proc;
}

/***************************************************************************/

static MACHINE_DRIVER_START( goldnaxa )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16_7759b)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(goldnaxa_readmem,goldnaxa_writemem)

	MDRV_MACHINE_INIT(goldnaxa)
MACHINE_DRIVER_END

/***************************************************************************/
/* sys16B */
ROM_START( hwchamp )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "rom0-e.bin", 0x000000, 0x20000, CRC(e5abfed7) SHA1(1f875dbaf8665c1dbfe336470580361b18a8ed4e) )
	ROM_LOAD16_BYTE( "rom0-o.bin", 0x000001, 0x20000, CRC(25180124) SHA1(77b414f8cd88270713c57bddadec5d8dca490e86) )

	ROM_REGION( 0xc0000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "scr01.bin", 0x00000, 0x20000, CRC(fc586a86) SHA1(2c26ef3ab94089940add3be9952804a6e62f5113) )
	ROM_LOAD( "scr11.bin", 0x20000, 0x20000, CRC(aeaaa9d8) SHA1(6b7e5320f515c1c35445d3320b3edaef911191e1) )
	ROM_LOAD( "scr02.bin", 0x40000, 0x20000, CRC(7715a742) SHA1(e6040ff0e9c68f3f502e5f6d7e7ca04b14059752) )
	ROM_LOAD( "scr12.bin", 0x60000, 0x20000, CRC(63a82afa) SHA1(a02bbb6dd84cdf7cdab8e738c6927f5b1e3fcad5) )
	ROM_LOAD( "scr03.bin", 0x80000, 0x20000, CRC(f30cd5fd) SHA1(df6118ca4b724c37b11e18d9f2ea18e9591ae7aa) )
	ROM_LOAD( "scr13.bin", 0xA0000, 0x20000, CRC(5b8494a8) SHA1(9e3f09f4037a007b6a188dd81ec8f9c635e87650) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "obj0-o.bin", 0x000001, 0x010000, CRC(fc098a13) SHA1(b4a6e00d4765265bad170dabf0b2a4a58e063b16) )
	ROM_CONTINUE( 0x040001, 0x10000 )
	ROM_RELOAD  ( 0x020001, 0x10000 )
	ROM_CONTINUE( 0x060001, 0x10000 )
	ROM_LOAD16_BYTE( "obj0-e.bin", 0x000000, 0x010000, CRC(5db934a8) SHA1(ba7cc93025af71ad2674b1376b61afbb7ae910ff) )
	ROM_CONTINUE( 0x040000, 0x10000 )
	ROM_RELOAD  ( 0x020000, 0x10000 )
	ROM_CONTINUE( 0x060000, 0x10000 )
	ROM_LOAD16_BYTE( "obj1-o.bin", 0x080001, 0x010000, CRC(1f27ee74) SHA1(a60d50a4f501623187c067a3c17bff49151ca3b2) )
	ROM_CONTINUE( 0x0c0001, 0x10000 )
	ROM_RELOAD  ( 0x0a0001, 0x10000 )
	ROM_CONTINUE( 0x0e0001, 0x10000 )
	ROM_LOAD16_BYTE( "obj1-e.bin", 0x080000, 0x010000, CRC(8a6a5cf1) SHA1(28b22aa326682ef3b54891dda7aa9a432da12a4d) )
	ROM_CONTINUE( 0x0c0000, 0x10000 )
	ROM_RELOAD  ( 0x0a0000, 0x10000 )
	ROM_CONTINUE( 0x0e0000, 0x10000 )
	ROM_LOAD16_BYTE( "obj2-o.bin", 0x100001, 0x010000, CRC(c0b2ba82) SHA1(30349c86a99bbe3dfb423027ad534a9333e27679) )
	ROM_CONTINUE( 0x140001, 0x10000 )
	ROM_RELOAD  ( 0x120001, 0x10000 )
	ROM_CONTINUE( 0x160001, 0x10000 )
	ROM_LOAD16_BYTE( "obj2-e.bin", 0x100000, 0x010000, CRC(d6c7917b) SHA1(8b313a5634c14f4c90bfa9f9616d600283f72768) )
	ROM_CONTINUE( 0x140000, 0x10000 )
	ROM_RELOAD  ( 0x120000, 0x10000 )
	ROM_CONTINUE( 0x160000, 0x10000 )
	ROM_LOAD16_BYTE( "obj3-o.bin", 0x180001, 0x010000, CRC(52fa3a49) SHA1(c2331af630d86a111cdaf21556d9df23d1471f53) )
	ROM_CONTINUE( 0x1c0001, 0x10000 )
	ROM_RELOAD  ( 0x1a0001, 0x10000 )
	ROM_CONTINUE( 0x1e0001, 0x10000 )
	ROM_LOAD16_BYTE( "obj3-e.bin", 0x180000, 0x010000, CRC(57e8f9d2) SHA1(1804677820d05a421120660f91e3a5f1df1e6a8d) )
	ROM_CONTINUE( 0x1c0000, 0x10000 )
	ROM_RELOAD  ( 0x1a0000, 0x10000 )
	ROM_CONTINUE( 0x1e0000, 0x10000 )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "s-prog.bin", 0x0000, 0x8000, CRC(96a12d9d) SHA1(f4ba70c3b5d80a1b6a187c940b922d5182d5ab12) )

	ROM_LOAD( "speech0.bin", 0x10000, 0x20000, CRC(4191c03d) SHA1(40809fb80527980015d3b5c4ca7cf159bc09cf5a) )
	ROM_LOAD( "speech1.bin", 0x30000, 0x20000, CRC(a4d53f7b) SHA1(71123a8ecfa093897c6f2bb7312e6c755be14521) )
ROM_END

/***************************************************************************/

static int hwc_handles_shifts[3];

static WRITE16_HANDLER( hwc_io_handles_w ){
	hwc_handles_shifts[offset]=7;
}

static READ16_HANDLER( hwc_io_handles_r ){
	static int dodge_toggle=0;
	int data=0,ret;
	if(offset==0){
		/* monitor */
		data=input_port_0_r( offset );
		if(input_port_1_r( offset ) & 4){
			if(dodge_toggle) data=0x38; else data=0x60;
		}
		if(input_port_1_r( offset ) & 8){
			if(dodge_toggle) data=0xc8; else data=0xa0;
		}
		if(input_port_1_r( offset ) & 0x10){
			if(dodge_toggle) data=0xff; else data=0xe0;
		}
		if(input_port_1_r( offset ) & 0x20){
			if(dodge_toggle) data=0x0; else data=0x20;
		}
		if( hwc_handles_shifts[offset]==0) dodge_toggle^=1;
	}
	else if(offset==1){
		/* left handle */
		if(input_port_1_r( offset ) & 1) data=0xff;
	}
	else {
		/* right handle */
		if(input_port_1_r( offset ) & 2) data=0xff;
	}

	ret = data>>hwc_handles_shifts[offset];
	hwc_handles_shifts[offset]--;
	return ret;
}

static WRITE16_HANDLER( hwc_ctrl1_w ){
	if( ACCESSING_LSB ){
		sys16_refreshenable = data & 0x20;
		coin_counter_w(0,data & 0x01);
		set_led_status(0,data & 0x04);
		/* bit 6 is also used (always 1?) */
	}
}

static WRITE16_HANDLER( hwc_ctrl2_w ){
	if( ACCESSING_LSB ){
		/* bit 4 is GONG */
/*		if (data & 0x10) usrintf_showmessage("GONG"); */
		/* are the following really lamps? */
/*		set_led_status(1,data & 0x20); */
/*		set_led_status(2,data & 0x40); */
/*		set_led_status(3,data & 0x80); */
	}
}

static MEMORY_READ16_START( hwchamp_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x400000, 0x40ffff, SYS16_MRA16_TILERAM },
	{ 0x410000, 0x410fff, SYS16_MRA16_TEXTRAM },
	{ 0x440000, 0x440fff, SYS16_MRA16_SPRITERAM },
	{ 0x840000, 0x840fff, SYS16_MRA16_PALETTERAM },
	{ 0xc41000, 0xc41001, input_port_2_word_r }, /* service */
	{ 0xc42002, 0xc42003, input_port_3_word_r }, /* dip1 */
	{ 0xc42000, 0xc42001, input_port_4_word_r }, /* dip2 */
	{ 0xc43020, 0xc43025, hwc_io_handles_r },
	{ 0xffc000, 0xffffff, SYS16_MRA16_WORKINGRAM },
MEMORY_END

static MEMORY_WRITE16_START( hwchamp_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x3f0000, 0x3fffff, sys16_tilebank_w },
	{ 0x400000, 0x40ffff, SYS16_MWA16_TILERAM, &sys16_tileram },
	{ 0x410000, 0x410fff, SYS16_MWA16_TEXTRAM, &sys16_textram },
	{ 0x440000, 0x440fff, SYS16_MWA16_SPRITERAM, &sys16_spriteram },
	{ 0x840000, 0x840fff, SYS16_MWA16_PALETTERAM, &paletteram16 },
	{ 0xc40000, 0xc40001, hwc_ctrl1_w },
	{ 0xc43020, 0xc43025, hwc_io_handles_w },
	{ 0xc43034, 0xc43035, hwc_ctrl2_w },
	{ 0xfe0006, 0xfe0007, sound_command_w },
	{ 0xffc000, 0xffffff, SYS16_MWA16_WORKINGRAM, &sys16_workingram },
MEMORY_END

/***************************************************************************/

static MACHINE_INIT( hwchamp ){
	sys16_update_proc = type0_sys16_textram;
	sys16_wwfix = 1;
}

/***************************************************************************/

INPUT_PORTS_START( hwchamp )

PORT_START	/* Monitor */
	PORT_ANALOG( 0xff, 0x80, IPT_PADDLE  , 70, 4, 0x0, 0xff )

PORT_START	/* Handles (Fake) */
	PORT_BITX(0x01, 0, IPT_BUTTON1, IP_NAME_DEFAULT, KEYCODE_F, IP_JOY_NONE ) /* right hit */
	PORT_BITX(0x02, 0, IPT_BUTTON2, IP_NAME_DEFAULT, KEYCODE_D, IP_JOY_NONE ) /* left hit */
	PORT_BITX(0x04, 0, IPT_BUTTON3, IP_NAME_DEFAULT, KEYCODE_B, IP_JOY_NONE ) /* right dodge */
	PORT_BITX(0x08, 0, IPT_BUTTON4, IP_NAME_DEFAULT, KEYCODE_Z, IP_JOY_NONE ) /* left dodge */
	PORT_BITX(0x10, 0, IPT_BUTTON5, IP_NAME_DEFAULT, KEYCODE_V, IP_JOY_NONE ) /* right sway */
	PORT_BITX(0x20, 0, IPT_BUTTON6, IP_NAME_DEFAULT, KEYCODE_X, IP_JOY_NONE ) /* left swat */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	SYS16_SERVICE
	SYS16_COINAGE

PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )	/* Not Used */
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "Start Level Select" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Continue Mode" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x20, "Easy" )
	PORT_DIPSETTING(    0x30, "Normal" )
	PORT_DIPSETTING(    0x10, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0xc0, 0xc0, "Time Adjust"  )
	PORT_DIPSETTING(    0x80, "Easy" )
	PORT_DIPSETTING(    0xc0, "Normal" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )

INPUT_PORTS_END

/***************************************************************************/

static MACHINE_DRIVER_START( hwchamp )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16_7759b)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(hwchamp_readmem,hwchamp_writemem)

	MDRV_MACHINE_INIT(hwchamp)
MACHINE_DRIVER_END

/***************************************************************************/
/* pre16 */
ROM_START( mjleague )
	ROM_REGION( 0x030000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr-7404.09b", 0x000000, 0x8000, CRC(ec1655b5) SHA1(5c1df364fa9733daa4478c5f88298089e4963c33) )
	ROM_LOAD16_BYTE( "epr-7401.06b", 0x000001, 0x8000, CRC(2befa5e0) SHA1(0a1681a4c7d62a5754ba6f3845436b4d08324246) )
	ROM_LOAD16_BYTE( "epr-7405.10b", 0x010000, 0x8000, CRC(7a4f4e38) SHA1(65a22097dd933e83f326bd64b3863915897780a6) )
	ROM_LOAD16_BYTE( "epr-7402.07b", 0x010001, 0x8000, CRC(b7bef762) SHA1(214450e0b094f99ef38dec2a3e5cbdb0b30e917d) )
	ROM_LOAD16_BYTE( "epra7406.11b", 0x020000, 0x8000, CRC(bb743639) SHA1(5d99638a79f02ce14374d3b1f3d9fbfc5c13c6e1) )
	ROM_LOAD16_BYTE( "epra7403.08b", 0x020001, 0x8000, CRC(d86250cf) SHA1(fb5dabb7b9b9fe0bbe93e28c60311c7b3256107a) )	/* Fails memory test. Bad rom? */

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "epr-7051.09a", 0x00000, 0x08000, CRC(10ca255a) SHA1(ccf58ffcac2f7fbdbfbdf32601a1b97f359cbd91) )
	ROM_LOAD( "epr-7052.10a", 0x08000, 0x08000, CRC(2550db0e) SHA1(28f8d68f43d26f12793fe295c205cc86adc4e96a) )
	ROM_LOAD( "epr-7053.11a", 0x10000, 0x08000, CRC(5bfea038) SHA1(01dc6e14cc7bba9f7930e68573c441fa2841f49a) )

	ROM_REGION( 0x50000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "epr-7055.05a", 0x000001, 0x8000, CRC(1fb860bd) SHA1(4a4155d0352dfae9e402a2b2f1558ef17b1303b4) )
	ROM_LOAD16_BYTE( "epr-7059.02b", 0x000000, 0x8000, CRC(3d14091d) SHA1(36208415b2012b6e948fefa15b0f7041748066be) )
	ROM_LOAD16_BYTE( "epr-7056.06a", 0x010001, 0x8000, CRC(b35dd968) SHA1(e306b5e38acf583d7b2089302622ad25ae5564b0) )
	ROM_LOAD16_BYTE( "epr-7060.03b", 0x010000, 0x8000, CRC(61bb3757) SHA1(5c87cf23be22b84e3dae746527ca057d870d6397) )
	ROM_LOAD16_BYTE( "epr-7057.07a", 0x020001, 0x8000, CRC(3e5a2b6f) SHA1(d3dbafb4acb916e02c978a156008bd75ba122fb7) )
	ROM_LOAD16_BYTE( "epr-7061.04b", 0x020000, 0x8000, CRC(c808dad5) SHA1(9b65acc8dc23b16e56327298188d1a6ab48b2b5d) )
	ROM_LOAD16_BYTE( "epr-7058.08a", 0x030001, 0x8000, CRC(b543675f) SHA1(35ffc9295a8849a18fabe156fdbc9801ea2179cd) )
	ROM_LOAD16_BYTE( "epr-7062.05b", 0x030000, 0x8000, CRC(9168eb47) SHA1(daaa7836e627a0679e65373d8f20a9383ba4c905) )
/*	ROM_LOAD16_BYTE( "epr-7055.05a", 0x040001, 0x8000, CRC(1fb860bd) SHA1(4a4155d0352dfae9e402a2b2f1558ef17b1303b4) ) loaded twice?? */
/*	ROM_LOAD16_BYTE( "epr-7059.02b", 0x040000, 0x8000, CRC(3d14091d) SHA1(36208415b2012b6e948fefa15b0f7041748066be) ) loaded twice?? */

	ROM_REGION( 0x20000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "eprc7054.01b", 0x00000, 0x8000, CRC(4443b744) SHA1(73359a6e9d62b382dee47fea31b9e17eb26a0321) )

	ROM_REGION( 0x1000, REGION_CPU3, 0 )      /* 4k for 7751 onboard ROM */
	ROM_LOAD( "7751.bin",     0x0000, 0x0400, CRC(6a9534fc) SHA1(67ad94674db5c2aab75785668f610f6f4eccd158) ) /* 7751 - U34 */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* 7751 sound data */
	ROM_LOAD( "epr-7063.01a", 0x00000, 0x8000, CRC(45d8908a) SHA1(e61f81f953c1a744ded36fed3b55774e4747af29) )
	ROM_LOAD( "epr-7065.02a", 0x08000, 0x8000, CRC(8c8f8cff) SHA1(fca5a916a8b25800ee5e8771e2ced0ed9bd737f4) )
	ROM_LOAD( "epr-7064.03a", 0x10000, 0x8000, CRC(159f6636) SHA1(66fa3f3e95a6ef3d3ff4ded09c05ab1131d9fbbb) )
	ROM_LOAD( "epr-7066.04a", 0x18000, 0x8000, CRC(f5cfa91f) SHA1(c85d68cbcd03fe1436bed12235c033610acc11ee) )
ROM_END

/***************************************************************************/

static READ16_HANDLER( mjl_io_player1_r ){
	data16_t data=input_port_0_r( offset ) & 0x80;

	if( sys16_extraram2[2/2] & 0x4 )
		data|=(input_port_5_r( offset ) & 0x3f) << 1;
	else
		data|=(input_port_6_r( offset ) & 0x3f) << 1;

	return data;
}

static READ16_HANDLER( mjl_io_service_r ){
	data16_t data=input_port_2_r( offset ) & 0x3f;

	if(sys16_extraram2[2/2] & 0x4){
		data|=(input_port_5_r( offset ) & 0x40);
		data|=(input_port_7_r( offset ) & 0x40) << 1;
	}
	else {
		data|=(input_port_6_r( offset ) & 0x40);
		data|=(input_port_8_r( offset ) & 0x40) << 1;
	}

	return data;
}

static READ16_HANDLER( mjl_io_player2_r )
{
	data16_t data=input_port_1_r( offset ) & 0x80;
	if(sys16_extraram2[2/2] & 0x4)
		data|=(input_port_7_r( offset ) & 0x3f) << 1;
	else
		data|=(input_port_8_r( offset ) & 0x3f) << 1;
	return data;
}

static READ16_HANDLER( mjl_io_bat_r )
{
	int data1=input_port_0_r( offset );
	int data2=input_port_1_r( offset );
	int ret=0;

	/* Hitting has 8 values, but for easy of playing, I've only added 3 */

	if(data1 &1) ret=0x00;
	else if(data1 &2) ret=0x03;
	else if(data1 &4) ret=0x07;
	else ret=0x0f;

	if(data2 &1) ret|=0x00;
	else if(data2 &2) ret|=0x30;
	else if(data2 &4) ret|=0x70;
	else ret|=0xf0;

	return ret;
}

static MEMORY_READ16_START( mjleague_readmem )
	{ 0x000000, 0x02ffff, MRA16_ROM },
	{ 0x400000, 0x40ffff, SYS16_MRA16_TILERAM },
	{ 0x410000, 0x410fff, SYS16_MRA16_TEXTRAM },
	{ 0x440000, 0x440fff, SYS16_MRA16_SPRITERAM },
	{ 0x840000, 0x840fff, SYS16_MRA16_PALETTERAM },
	{ 0xc40002, 0xc40007, SYS16_MRA16_EXTRAM2 },
	{ 0xc41000, 0xc41001, mjl_io_service_r },
	{ 0xc41002, 0xc41003, mjl_io_player1_r },
	{ 0xc41006, 0xc41007, mjl_io_player2_r },
	{ 0xc41004, 0xc41005, mjl_io_bat_r },
	{ 0xc42000, 0xc42001, input_port_3_word_r }, /* dip1 */
	{ 0xc42002, 0xc42003, input_port_4_word_r }, /* dip2 */
	{ 0xc60000, 0xc60001, MRA16_NOP }, /* What is this? Watchdog? */
	{ 0xffc000, 0xffffff, SYS16_MRA16_WORKINGRAM },
MEMORY_END

static MEMORY_WRITE16_START( mjleague_writemem )
	{ 0x000000, 0x02ffff, MWA16_ROM },
	{ 0x400000, 0x40ffff, SYS16_MWA16_TILERAM, &sys16_tileram },
	{ 0x410000, 0x410fff, SYS16_MWA16_TEXTRAM, &sys16_textram },
	{ 0x440000, 0x440fff, SYS16_MWA16_SPRITERAM, &sys16_spriteram },
	{ 0x840000, 0x840fff, SYS16_MWA16_PALETTERAM, &paletteram16 },
	{ 0xc40000, 0xc40001, sound_command_nmi_w },
	{ 0xc40002, 0xc40007, SYS16_MWA16_EXTRAM2, &sys16_extraram2 },
	{ 0xffc000, 0xffffff, SYS16_MWA16_WORKINGRAM, &sys16_workingram },
MEMORY_END

/***************************************************************************/

static MACHINE_INIT( mjleague ){
	sys16_textmode=1;
	sys16_spritesystem = sys16_sprite_quartet2;
	sys16_sprxoffset = -0xbd;
	sys16_fgxoffset = sys16_bgxoffset = 8;

	/* remove memory test because it fails. */
	sys16_patch_code( 0xBD42, 0x66 );

	sys16_update_proc = type1_sys16_textram;
}

/***************************************************************************/

INPUT_PORTS_START( mjleague )

PORT_START /* player 1 button fake */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON4 )

PORT_START /* player 1 button fake */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON4 | IPF_PLAYER2 )

PORT_START  /* Service */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	SYS16_COINAGE

PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x00, "Starting Points" )
	PORT_DIPSETTING(    0x0c, "2000" )
	PORT_DIPSETTING(    0x08, "3000" )
	PORT_DIPSETTING(    0x04, "5000" )
	PORT_DIPSETTING(    0x00, "10000" )
	PORT_DIPNAME( 0x10, 0x10, "Team Select" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )	/*??? something to do with cocktail mode? */
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

PORT_START	/* IN5 */
	PORT_ANALOG( 0x7f, 0x40, IPT_TRACKBALL_Y, 70, 30, 0, 127 )

PORT_START	/* IN6 */
	PORT_ANALOG( 0x7f, 0x40, IPT_TRACKBALL_X /*| IPF_REVERSE*/, 50, 30, 0, 127 )

PORT_START	/* IN7 */
	PORT_ANALOG( 0x7f, 0x40, IPT_TRACKBALL_Y | IPF_PLAYER2, 70, 30, 0, 127 )

PORT_START	/* IN8 */
	PORT_ANALOG( 0x7f, 0x40, IPT_TRACKBALL_X | IPF_PLAYER2 | IPF_REVERSE, 50, 30, 0, 127 )

INPUT_PORTS_END

/***************************************************************************/

static MACHINE_DRIVER_START( mjleague )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16_7751)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(mjleague_readmem,mjleague_writemem)

	MDRV_MACHINE_INIT(mjleague)
MACHINE_DRIVER_END

/***************************************************************************/
/* sys16B */
ROM_START( passsht )
	ROM_REGION( 0x20000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr11871.a4", 0x000000, 0x10000, CRC(0f9ccea5) SHA1(515a2721a35332df0303f4b9616122194b5c7170) )
	ROM_LOAD16_BYTE( "epr11870.a1", 0x000001, 0x10000, CRC(df43ebcf) SHA1(3ca11a25819e1e8d5162f7b36cccc928d8efe150) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "opr11854.b9",  0x00000, 0x10000, CRC(d31c0b6c) SHA1(610d04988da70c30300cc5614817eda9d2204f39) )
	ROM_LOAD( "opr11855.b10", 0x10000, 0x10000, CRC(b78762b4) SHA1(d594ef846bd7fed8da91a89906b39c1a2867a1fe) )
	ROM_LOAD( "opr11856.b11", 0x20000, 0x10000, CRC(ea49f666) SHA1(36ccd32cdcbb7fcc300628bb59c220ec3c324d82) )

	ROM_REGION( 0x60000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "opr11862.b1",  0x00001, 0x10000, CRC(b6e94727) SHA1(0838e034f1f10d9cd1312c8c94b5c57387c0c271) )
	ROM_LOAD16_BYTE( "opr11865.b5",  0x00000, 0x10000, CRC(17e8d5d5) SHA1(ac1074b0a705be13c6e3391441e6cfec1d2b3f8a) )
	ROM_LOAD16_BYTE( "opr11863.b2",  0x20001, 0x10000, CRC(3e670098) SHA1(2cfc83f4294be30cd868738886ac546bd8489962) )
	ROM_LOAD16_BYTE( "opr11866.b6",  0x20000, 0x10000, CRC(50eb71cc) SHA1(463b4917ca19c7f4ad2c2845caa104d5e4a2dda3) )
	ROM_LOAD16_BYTE( "opr11864.b3",  0x40001, 0x10000, CRC(05733ca8) SHA1(1dbc7c99450ebe6a9fd8c0244fd3cb38b74984ef) )
	ROM_LOAD16_BYTE( "opr11867.b7",  0x40000, 0x10000, CRC(81e49697) SHA1(a70fa409e3555ad6c8f28930a7026fdf2deb8c65) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr11857.a7",  0x00000, 0x08000, CRC(789edc06) SHA1(8c89c94e503513c287807d187de78a7fbd75a7cf) )
	ROM_LOAD( "epr11858.a8",  0x10000, 0x08000, CRC(08ab0018) SHA1(0685f80a7d403208c9cfffea3f2035324f3924fe) )
	ROM_LOAD( "epr11859.a9",  0x18000, 0x08000, CRC(8673e01b) SHA1(e79183ab30e683fdf61ced2e9dbe010567c324cb) )
	ROM_LOAD( "epr11860.a10", 0x20000, 0x08000, CRC(10263746) SHA1(1f981fb185c6a9795208ecdcfba36cf892a99ed5) )
	ROM_LOAD( "epr11861.a11", 0x28000, 0x08000, CRC(38b54a71) SHA1(68ec4ef5b115844214ff2213be1ce6678904fbd2) )
ROM_END

ROM_START( passht4b )
	ROM_REGION( 0x20000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "pas4p.3", 0x000000, 0x10000, CRC(2d8bc946) SHA1(35d3e529d4815543d9876fd0545c3d686467abaa) )
	ROM_LOAD16_BYTE( "pas4p.4", 0x000001, 0x10000, CRC(e759e831) SHA1(dd5727dc28010cb988e4951723171171eb645ce8) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "pas4p.11", 0x00000, 0x10000, CRC(da20fbc9) SHA1(21dc8143f4d1cebae4f86e83495fa84e5293ba48) )
	ROM_LOAD( "pas4p.12", 0x10000, 0x10000, CRC(bebb9211) SHA1(4f56048f6f70b63f74a4c0d64456213d36ce5b26) )
	ROM_LOAD( "pas4p.13", 0x20000, 0x10000, CRC(e37506c3) SHA1(e6fbf15d58f321a3d052fefbe5a1901e4a1734ae) )

	ROM_REGION( 0x60000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "opr11862.b1",  0x00001, 0x10000, CRC(b6e94727) SHA1(0838e034f1f10d9cd1312c8c94b5c57387c0c271) )
	ROM_LOAD16_BYTE( "opr11865.b5",  0x00000, 0x10000, CRC(17e8d5d5) SHA1(ac1074b0a705be13c6e3391441e6cfec1d2b3f8a) )
	ROM_LOAD16_BYTE( "opr11863.b2",  0x20001, 0x10000, CRC(3e670098) SHA1(2cfc83f4294be30cd868738886ac546bd8489962) )
	ROM_LOAD16_BYTE( "opr11866.b6",  0x20000, 0x10000, CRC(50eb71cc) SHA1(463b4917ca19c7f4ad2c2845caa104d5e4a2dda3) )
	ROM_LOAD16_BYTE( "opr11864.b3",  0x40001, 0x10000, CRC(05733ca8) SHA1(1dbc7c99450ebe6a9fd8c0244fd3cb38b74984ef) )
	ROM_LOAD16_BYTE( "opr11867.b7",  0x40000, 0x10000, CRC(81e49697) SHA1(a70fa409e3555ad6c8f28930a7026fdf2deb8c65) )

	ROM_REGION( 0x20000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "pas4p.1",  0x00000, 0x08000, CRC(e60fb017) SHA1(21298036eab55c74427f1c2e3a9623d41bca4849) )
	ROM_LOAD( "pas4p.2",  0x10000, 0x10000, CRC(092e016e) SHA1(713638749efa9dce19c547b84308236110bc85fe) )
ROM_END

ROM_START( passshtb )
	ROM_REGION( 0x020000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "pass3_2p.bin", 0x000000, 0x10000, CRC(26bb9299) SHA1(11bacf86dfdd8bcfbfb61f0ebc59890325c48adc) )
	ROM_LOAD16_BYTE( "pass4_2p.bin", 0x000001, 0x10000, CRC(06ac6d5d) SHA1(2dd71a8a956404326797de8beed7bca016c9919e) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "opr11854.b9",  0x00000, 0x10000, CRC(d31c0b6c) SHA1(610d04988da70c30300cc5614817eda9d2204f39) )
	ROM_LOAD( "opr11855.b10", 0x10000, 0x10000, CRC(b78762b4) SHA1(d594ef846bd7fed8da91a89906b39c1a2867a1fe) )
	ROM_LOAD( "opr11856.b11", 0x20000, 0x10000, CRC(ea49f666) SHA1(36ccd32cdcbb7fcc300628bb59c220ec3c324d82) )

	ROM_REGION( 0x60000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "opr11862.b1",  0x00001, 0x10000, CRC(b6e94727) SHA1(0838e034f1f10d9cd1312c8c94b5c57387c0c271) )
	ROM_LOAD16_BYTE( "opr11865.b5",  0x00000, 0x10000, CRC(17e8d5d5) SHA1(ac1074b0a705be13c6e3391441e6cfec1d2b3f8a) )
	ROM_LOAD16_BYTE( "opr11863.b2",  0x20001, 0x10000, CRC(3e670098) SHA1(2cfc83f4294be30cd868738886ac546bd8489962) )
	ROM_LOAD16_BYTE( "opr11866.b6",  0x20000, 0x10000, CRC(50eb71cc) SHA1(463b4917ca19c7f4ad2c2845caa104d5e4a2dda3) )
	ROM_LOAD16_BYTE( "opr11864.b3",  0x40001, 0x10000, CRC(05733ca8) SHA1(1dbc7c99450ebe6a9fd8c0244fd3cb38b74984ef) )
	ROM_LOAD16_BYTE( "opr11867.b7",  0x40000, 0x10000, CRC(81e49697) SHA1(a70fa409e3555ad6c8f28930a7026fdf2deb8c65) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr11857.a7",  0x00000, 0x08000, CRC(789edc06) SHA1(8c89c94e503513c287807d187de78a7fbd75a7cf) )
	ROM_LOAD( "epr11858.a8",  0x10000, 0x08000, CRC(08ab0018) SHA1(0685f80a7d403208c9cfffea3f2035324f3924fe) )
	ROM_LOAD( "epr11859.a9",  0x18000, 0x08000, CRC(8673e01b) SHA1(e79183ab30e683fdf61ced2e9dbe010567c324cb) )
	ROM_LOAD( "epr11860.a10", 0x20000, 0x08000, CRC(10263746) SHA1(1f981fb185c6a9795208ecdcfba36cf892a99ed5) )
	ROM_LOAD( "epr11861.a11", 0x28000, 0x08000, CRC(38b54a71) SHA1(68ec4ef5b115844214ff2213be1ce6678904fbd2) )
ROM_END
/***************************************************************************/

static MEMORY_READ16_START( passsht_readmem )
	{ 0x000000, 0x01ffff, MRA16_ROM },
	{ 0x400000, 0x40ffff, SYS16_MRA16_TILERAM },
	{ 0x410000, 0x410fff, SYS16_MRA16_TEXTRAM },
	{ 0x440000, 0x440fff, SYS16_MRA16_SPRITERAM },
	{ 0x840000, 0x840fff, SYS16_MRA16_PALETTERAM },
	{ 0xc41002, 0xc41003, input_port_0_word_r }, /* player1 */
	{ 0xc41004, 0xc41005, input_port_1_word_r }, /* player2 */
	{ 0xc41000, 0xc41001, input_port_2_word_r }, /* service */
	{ 0xc42002, 0xc42003, input_port_3_word_r }, /* dip1 */
	{ 0xc42000, 0xc42001, input_port_4_word_r }, /* dip2 */
	{ 0xffc000, 0xffffff, SYS16_MRA16_WORKINGRAM },
MEMORY_END

static MEMORY_WRITE16_START( passsht_writemem )
	{ 0x000000, 0x01ffff, MWA16_ROM },
	{ 0x400000, 0x40ffff, SYS16_MWA16_TILERAM, &sys16_tileram },
	{ 0x410000, 0x410fff, SYS16_MWA16_TEXTRAM, &sys16_textram },
	{ 0x440000, 0x440fff, SYS16_MWA16_SPRITERAM, &sys16_spriteram },
	{ 0x840000, 0x840fff, SYS16_MWA16_PALETTERAM, &paletteram16 },
	{ 0xc42006, 0xc42007, sound_command_w },
	{ 0xc40000, 0xc40001, sys16_coinctrl_w },
	{ 0xffc000, 0xffffff, SYS16_MWA16_WORKINGRAM, &sys16_workingram },
MEMORY_END

static int passht4b_io1_val;
static int passht4b_io2_val;
static int passht4b_io3_val;

static READ16_HANDLER( passht4b_service_r ){
	data16_t val=input_port_2_word_r(offset,0);
	if(!(readinputport(0) & 0x40)) val&=0xef;
	if(!(readinputport(1) & 0x40)) val&=0xdf;
	if(!(readinputport(5) & 0x40)) val&=0xbf;
	if(!(readinputport(6) & 0x40)) val&=0x7f;

	passht4b_io3_val=(readinputport(0)<<4) | (readinputport(5)&0xf);
	passht4b_io2_val=(readinputport(1)<<4) | (readinputport(6)&0xf);

	passht4b_io1_val=0xff;

	/* player 1 buttons */
	if(!(readinputport(0) & 0x10)) passht4b_io1_val &=0xfe;
	if(!(readinputport(0) & 0x20)) passht4b_io1_val &=0xfd;
	if(!(readinputport(0) & 0x80)) passht4b_io1_val &=0xfc;

	/* player 2 buttons */
	if(!(readinputport(1) & 0x10)) passht4b_io1_val &=0xfb;
	if(!(readinputport(1) & 0x20)) passht4b_io1_val &=0xf7;
	if(!(readinputport(1) & 0x80)) passht4b_io1_val &=0xf3;

	/* player 3 buttons */
	if(!(readinputport(5) & 0x10)) passht4b_io1_val &=0xef;
	if(!(readinputport(5) & 0x20)) passht4b_io1_val &=0xdf;
	if(!(readinputport(5) & 0x80)) passht4b_io1_val &=0xcf;

	/* player 4 buttons */
	if(!(readinputport(6) & 0x10)) passht4b_io1_val &=0xbf;
	if(!(readinputport(6) & 0x20)) passht4b_io1_val &=0x7f;
	if(!(readinputport(6) & 0x80)) passht4b_io1_val &=0x3f;

	return val;
}

static READ16_HANDLER( passht4b_io1_r ) {	return passht4b_io1_val;}
static READ16_HANDLER( passht4b_io2_r ) {	return passht4b_io2_val;}
static READ16_HANDLER( passht4b_io3_r ) {	return passht4b_io3_val;}

static MEMORY_READ16_START( passht4b_readmem )
	{ 0x000000, 0x01ffff, MRA16_ROM },
	{ 0x400000, 0x40ffff, SYS16_MRA16_TILERAM },
	{ 0x410000, 0x410fff, SYS16_MRA16_TEXTRAM },
	{ 0x440000, 0x440fff, SYS16_MRA16_SPRITERAM },
	{ 0x840000, 0x840fff, SYS16_MRA16_PALETTERAM },
	{ 0xc41000, 0xc41001, passht4b_service_r },
	{ 0xc41002, 0xc41003, passht4b_io1_r },
	{ 0xc41004, 0xc41005, passht4b_io2_r },
	{ 0xc41006, 0xc41007, passht4b_io3_r },
	{ 0xc42002, 0xc42003, input_port_3_word_r }, /* dip1 */
	{ 0xc42000, 0xc42001, input_port_4_word_r }, /* dip2 */
	{ 0xc43000, 0xc43001, input_port_0_word_r },/* player1 test mode only */
	{ 0xc43002, 0xc43003, input_port_1_word_r }, /* player2 */
	{ 0xc43004, 0xc43005, input_port_5_word_r }, /* player3 */
	{ 0xc43006, 0xc43007, input_port_6_word_r }, /* player4 */
	{ 0xffc000, 0xffffff, SYS16_MRA16_WORKINGRAM },
MEMORY_END

static MEMORY_WRITE16_START( passht4b_writemem )
	{ 0x000000, 0x01ffff, MWA16_ROM },
	{ 0x400000, 0x40ffff, SYS16_MWA16_TILERAM, &sys16_tileram },
	{ 0x410000, 0x410fff, SYS16_MWA16_TEXTRAM, &sys16_textram },
	{ 0x440000, 0x440fff, SYS16_MWA16_SPRITERAM, &sys16_spriteram },
	{ 0x840000, 0x840fff, SYS16_MWA16_PALETTERAM, &paletteram16 },
	{ 0xc42006, 0xc42007, sound_command_w },
	{ 0xc4600a, 0xc4600b, sys16_coinctrl_w },	/* coin counter doesn't work */
	{ 0xffc000, 0xffffff, SYS16_MWA16_WORKINGRAM, &sys16_workingram },
MEMORY_END

/* YM2203 no msm5205 hookup as yet */
static MEMORY_READ_START(passht4b_readmem_sound )
  { 0x0000, 0x7fff, MRA_ROM },
	{ 0xe000, 0xe000, YM2203_status_port_0_r },
	{ 0xe001, 0xe001, YM2203_read_port_0_r },
	{ 0xe400, 0xe400, YM2203_status_port_1_r },
	{ 0xe401, 0xe401, YM2203_read_port_1_r },
	{ 0xe800, 0xe800, soundlatch_r },
	{ 0xf800, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START(passht4b_writemem_sound )
  { 0x0000, 0x7fff, MWA_ROM },
	{ 0xe000, 0xe000, YM2203_control_port_0_w },
	{ 0xe001, 0xe001, YM2203_write_port_0_w },
	{ 0xe400, 0xe400, YM2203_control_port_1_w },
	{ 0xe401, 0xe401, YM2203_write_port_1_w },
	{ 0xf800, 0xffff, MWA_RAM },
MEMORY_END

static PORT_READ_START( passht4b_read_sound_port  )
/*	ADDRESS_MAP_GLOBAL_MASK(0xff) */
PORT_END

static PORT_WRITE_START( passht4b_write_sound_port )
/*	ADDRESS_MAP_GLOBAL_MASK(0xff) */
PORT_END

/***************************************************************************/

static void passsht_update_proc( void ){
	sys16_fg_scrollx = sys16_workingram[0x34be/2];
	sys16_bg_scrollx = sys16_workingram[0x34c2/2];
	sys16_fg_scrolly = sys16_workingram[0x34bc/2];
	sys16_bg_scrolly = sys16_workingram[0x34c0/2];

	set_fg_page( sys16_textram[0x0ff6/2] );
	set_bg_page( sys16_textram[0x0ff4/2] );
}

static void passht4b_update_proc( void ){
	sys16_fg_scrollx = sys16_workingram[0x34ce/2];
	sys16_bg_scrollx = sys16_workingram[0x34d2/2];
	sys16_fg_scrolly = sys16_workingram[0x34cc/2];
	sys16_bg_scrolly = sys16_workingram[0x34d0/2];

	set_fg_page( sys16_textram[0x0ff6/2] );
	set_bg_page( sys16_textram[0x0ff4/2] );
}

static MACHINE_INIT( passsht ){
	sys16_sprxoffset = -0x48;
	sys16_spritesystem = sys16_sprite_passshot;

	/* fix name entry */
	sys16_patch_code( 0x13a8,0xc0);

	sys16_update_proc = passsht_update_proc;
}

static MACHINE_INIT( passht4b ){
	sys16_spritesystem = sys16_sprite_passshot;

	/* fix name entry */
	sys16_patch_code( 0x138a,0xc0);

	sys16_update_proc = passht4b_update_proc;
}

static DRIVER_INIT( passht4b ){
	int i;

	/* invert the graphics bits on the tiles */
	for (i = 0; i < 0x30000; i++)
		memory_region(REGION_GFX1)[i] ^= 0xff;
}

/***************************************************************************/

INPUT_PORTS_START( passsht )
PORT_START /* joy 1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )

PORT_START /* joy 2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )

	SYS16_SERVICE
	SYS16_COINAGE

PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0e, 0x0e, "Initial Point" )
	PORT_DIPSETTING(    0x06, "2000" )
	PORT_DIPSETTING(    0x0a, "3000" )
	PORT_DIPSETTING(    0x0c, "4000" )
	PORT_DIPSETTING(    0x0e, "5000" )
	PORT_DIPSETTING(    0x08, "6000" )
	PORT_DIPSETTING(    0x04, "7000" )
	PORT_DIPSETTING(    0x02, "8000" )
	PORT_DIPSETTING(    0x00, "9000" )
	PORT_DIPNAME( 0x30, 0x30, "Point Table" )
	PORT_DIPSETTING(    0x20, "Easy" )
	PORT_DIPSETTING(    0x30, "Normal" )
	PORT_DIPSETTING(    0x10, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x80, "Easy" )
	PORT_DIPSETTING(    0xc0, "Normal" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
INPUT_PORTS_END

INPUT_PORTS_START( passht4b )
PORT_START /* joy 1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 )

PORT_START /* joy 2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_COCKTAIL )

PORT_START /* service */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START4 )

	SYS16_COINAGE

PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0e, 0x0e, "Initial Point" )
	PORT_DIPSETTING(    0x06, "2000" )
	PORT_DIPSETTING(    0x0a, "3000" )
	PORT_DIPSETTING(    0x0c, "4000" )
	PORT_DIPSETTING(    0x0e, "5000" )
	PORT_DIPSETTING(    0x08, "6000" )
	PORT_DIPSETTING(    0x04, "7000" )
	PORT_DIPSETTING(    0x02, "8000" )
	PORT_DIPSETTING(    0x00, "9000" )
	PORT_DIPNAME( 0x30, 0x30, "Point Table" )
	PORT_DIPSETTING(    0x20, "Easy" )
	PORT_DIPSETTING(    0x30, "Normal" )
	PORT_DIPSETTING(    0x10, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x80, "Easy" )
	PORT_DIPSETTING(    0xc0, "Normal" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )

PORT_START /* joy 3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER3 )

PORT_START /* joy 4 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER4 )

INPUT_PORTS_END

/***************************************************************************/

static MACHINE_DRIVER_START( passsht )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16_7759b)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(passsht_readmem,passsht_writemem)

	MDRV_MACHINE_INIT(passsht)
MACHINE_DRIVER_END

static struct YM2203interface ym2203_interface =
{
	2,		/* 2 chips */
	4000000,	/* 4 MHz */
	{ YM2203_VOL(50,80), YM2203_VOL(50,80) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static MACHINE_DRIVER_START( passht4b )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(passht4b_readmem,passht4b_writemem)

	MDRV_CPU_MODIFY("sound")
	MDRV_CPU_PERIODIC_INT(nmi_line_pulse,  2300) /* music tempo */
	MDRV_CPU_MEMORY(passht4b_readmem_sound,passht4b_writemem_sound)
	MDRV_CPU_PORTS(passht4b_read_sound_port,passht4b_write_sound_port)

	MDRV_SOUND_REMOVE("2151")

	MDRV_MACHINE_INIT(passht4b)

	/* sound needs fixed same on master gfx messed up */
	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(0)
	MDRV_SOUND_ADD(YM2203, ym2203_interface)
MACHINE_DRIVER_END

/***************************************************************************/
/* pre16 */
ROM_START( quartet )
	ROM_REGION( 0x030000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr7458a.9b",  0x000000, 0x8000, CRC(42e7b23e) SHA1(9df3b1b915723f9a927ef03d80ae7983a8c91a21) )
	ROM_LOAD16_BYTE( "epr7455a.6b",  0x000001, 0x8000, CRC(01631ab2) SHA1(2d613d23fe79072f850ccc9020830dea54312b23) )
	ROM_LOAD16_BYTE( "epr7459a.10b", 0x010000, 0x8000, CRC(6b540637) SHA1(4b2e9ba06b80f8fb502310ab770805f8c6a47567) )
	ROM_LOAD16_BYTE( "epr7456a.7b",  0x010001, 0x8000, CRC(31ca583e) SHA1(8ade8f7e42ae3e171b138410374e4c090fdc4ecb) )
	ROM_LOAD16_BYTE( "epr7460.11b",  0x020000, 0x8000, CRC(a444ea13) SHA1(884ed22d606e3bd30d8401fe1750687e54674e82) )
	ROM_LOAD16_BYTE( "epr7457.8b",   0x020001, 0x8000, CRC(3b282c23) SHA1(95de41a97f50f6169887c6d9724d5c42a41bb264) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "epr7461.9c",  0x00000, 0x08000, CRC(f6af07f2) SHA1(546fabbda936d61a90d2395d033fd4d6bb0bc38a) )
	ROM_LOAD( "epr7462.10c", 0x08000, 0x08000, CRC(7914af28) SHA1(4bf59fe4a0b0aa5d4cc0b6f9375ffab3c96e8a2b) )
	ROM_LOAD( "epr7463.11c", 0x10000, 0x08000, CRC(827c5603) SHA1(8db3bd6eae5aeeb229e017471049ef5347974df5) )

	ROM_REGION( 0x40000, REGION_GFX2, 0 ) /* sprites  - the same as quartet 2 */
	ROM_LOAD16_BYTE( "epr7465.5c",  0x000001, 0x8000, CRC(8a1ab7d7) SHA1(a2f317538c70a1603b65d795223407cbaaf88524) )
	ROM_LOAD16_BYTE( "epr-7469.2b", 0x000000, 0x8000, CRC(cb65ae4f) SHA1(3ee7b3b4cce113a6f394e8dfd317cdb6ffae64f7) )
	ROM_LOAD16_BYTE( "epr7466.6c",  0x010001, 0x8000, CRC(b2d3f4f3) SHA1(65e654fde10bee4cb5eee8234d0babb78fe41cfb) )
	ROM_LOAD16_BYTE( "epr-7470.3b", 0x010000, 0x8000, CRC(16fc67b1) SHA1(788fe2878c5c9faea43c2f166f32c22ee51c7d09) )
	ROM_LOAD16_BYTE( "epr7467.7c",  0x020001, 0x8000, CRC(0af68de2) SHA1(81163baf3f0e45bac950a6d9c24b3a886db1509c) )
	ROM_LOAD16_BYTE( "epr-7471.4b", 0x020000, 0x8000, CRC(13fad5ac) SHA1(75b480083fbb14cbef969126989bf9b2235fd31e) )
	ROM_LOAD16_BYTE( "epr7468.8c",  0x030001, 0x8000, CRC(ddfd40c0) SHA1(6c12ad668cd0c82e7d7d46bfbdcee8b9d46ebd09) )
	ROM_LOAD16_BYTE( "epr-7472.5b", 0x030000, 0x8000, CRC(8e2762ec) SHA1(872e19a6aab81d7a2472367d0e31dc1295da7182) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr-7464.1b", 0x0000, 0x8000, CRC(9f291306) SHA1(96a09542a863ccf2ded43e2df6f913722b3f97b1) )

	ROM_REGION( 0x1000, REGION_CPU3, 0 )      /* 4k for 7751 onboard ROM */
	ROM_LOAD( "7751.bin",     0x0000, 0x0400, CRC(6a9534fc) SHA1(67ad94674db5c2aab75785668f610f6f4eccd158) ) /* 7751 - U34 */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* 7751 sound data */
	ROM_LOAD( "epr7473.1c", 0x00000, 0x8000, CRC(06ec75fa) SHA1(5f14bc887449122700c46ad22c0379a1682e0bdb) )
	ROM_LOAD( "epr7475.2c", 0x08000, 0x8000, CRC(7abd1206) SHA1(54d52dc0b9c245cd2df647e714310a71b803cbcf) )
	ROM_LOAD( "epr7474.3c", 0x10000, 0x8000, CRC(dbf853b8) SHA1(e82f497e1144f23f3233b5c45ef182bfc7923715) )
	ROM_LOAD( "epr7476.4c", 0x18000, 0x8000, CRC(5eba655a) SHA1(6713ef12037cba3139d0f469c82bd90b44bae8ce) )
ROM_END

ROM_START( quartetj )
	ROM_REGION( 0x030000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr-7458.43",  0x000000, 0x8000, CRC(0096499f) SHA1(dcf8e33513ce7c6660ea546c8e1c574fde629a22) )
	ROM_LOAD16_BYTE( "epr-7455.26",  0x000001, 0x8000, CRC(da934390) SHA1(d40eb65b6a36a4c1ebeadb76e47a61bd8b2e4b89) )
	ROM_LOAD16_BYTE( "epr-7459.42",  0x010000, 0x8000, CRC(d130cf61) SHA1(3a065f5c296b10b97c78d49aa285ae7afb16e881) )
	ROM_LOAD16_BYTE( "epr-7456.25",  0x010001, 0x8000, CRC(7847149f) SHA1(fc8ad669f2bc426cb7af78d92ea147cbd1e181af) )
	ROM_LOAD16_BYTE( "epr7460.11b",  0x020000, 0x8000, CRC(a444ea13) SHA1(884ed22d606e3bd30d8401fe1750687e54674e82) )
	ROM_LOAD16_BYTE( "epr7457.8b",   0x020001, 0x8000, CRC(3b282c23) SHA1(95de41a97f50f6169887c6d9724d5c42a41bb264) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "epr7461.9c",  0x00000, 0x08000, CRC(f6af07f2) SHA1(546fabbda936d61a90d2395d033fd4d6bb0bc38a) )
	ROM_LOAD( "epr7462.10c", 0x08000, 0x08000, CRC(7914af28) SHA1(4bf59fe4a0b0aa5d4cc0b6f9375ffab3c96e8a2b) )
	ROM_LOAD( "epr7463.11c", 0x10000, 0x08000, CRC(827c5603) SHA1(8db3bd6eae5aeeb229e017471049ef5347974df5) )

	ROM_REGION( 0x040000, REGION_GFX2, 0 ) /* sprites  - the same as quartet 2 */
	ROM_LOAD16_BYTE( "epr7465.5c",  0x000001, 0x8000, CRC(8a1ab7d7) SHA1(a2f317538c70a1603b65d795223407cbaaf88524) )
	ROM_LOAD16_BYTE( "epr-7469.2b", 0x000000, 0x8000, CRC(cb65ae4f) SHA1(3ee7b3b4cce113a6f394e8dfd317cdb6ffae64f7) )
	ROM_LOAD16_BYTE( "epr7466.6c",  0x010001, 0x8000, CRC(b2d3f4f3) SHA1(65e654fde10bee4cb5eee8234d0babb78fe41cfb) )
	ROM_LOAD16_BYTE( "epr-7470.3b", 0x010000, 0x8000, CRC(16fc67b1) SHA1(788fe2878c5c9faea43c2f166f32c22ee51c7d09) )
	ROM_LOAD16_BYTE( "epr7467.7c",  0x020001, 0x8000, CRC(0af68de2) SHA1(81163baf3f0e45bac950a6d9c24b3a886db1509c) )
	ROM_LOAD16_BYTE( "epr-7471.4b", 0x020000, 0x8000, CRC(13fad5ac) SHA1(75b480083fbb14cbef969126989bf9b2235fd31e) )
	ROM_LOAD16_BYTE( "epr7468.8c",  0x030001, 0x8000, CRC(ddfd40c0) SHA1(6c12ad668cd0c82e7d7d46bfbdcee8b9d46ebd09) )
	ROM_LOAD16_BYTE( "epr-7472.5b", 0x030000, 0x8000, CRC(8e2762ec) SHA1(872e19a6aab81d7a2472367d0e31dc1295da7182) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr-7464.1b", 0x0000, 0x8000, CRC(9f291306) SHA1(96a09542a863ccf2ded43e2df6f913722b3f97b1) )

	ROM_REGION( 0x1000, REGION_CPU3, 0 )      /* 4k for 7751 onboard ROM */
	ROM_LOAD( "7751.bin",     0x0000, 0x0400, CRC(6a9534fc) SHA1(67ad94674db5c2aab75785668f610f6f4eccd158) ) /* 7751 - U34 */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* 7751 sound data */
	ROM_LOAD( "epr7473.1c", 0x00000, 0x8000, CRC(06ec75fa) SHA1(5f14bc887449122700c46ad22c0379a1682e0bdb) )
	ROM_LOAD( "epr7475.2c", 0x08000, 0x8000, CRC(7abd1206) SHA1(54d52dc0b9c245cd2df647e714310a71b803cbcf) )
	ROM_LOAD( "epr7474.3c", 0x10000, 0x8000, CRC(dbf853b8) SHA1(e82f497e1144f23f3233b5c45ef182bfc7923715) )
	ROM_LOAD( "epr7476.4c", 0x18000, 0x8000, CRC(5eba655a) SHA1(6713ef12037cba3139d0f469c82bd90b44bae8ce) )
ROM_END


/***************************************************************************/

#if 0
static READ16_HANDLER( quartet_skip_r ){
	if (activecpu_get_pc()==0x89b2) {cpu_spinuntil_int(); return 0xffff;}
	return sys16_workingram[0x0800/2];
}
#endif

static MEMORY_READ16_START( quartet_readmem )
	{ 0x000000, 0x02ffff, MRA16_ROM },
	{ 0x400000, 0x40ffff, SYS16_MRA16_TILERAM },
	{ 0x410000, 0x410fff, SYS16_MRA16_TEXTRAM },
	{ 0x440000, 0x440fff, SYS16_MRA16_SPRITERAM },
	{ 0x840000, 0x840fff, SYS16_MRA16_PALETTERAM },
	{ 0xc40002, 0xc40003, sys16_coinctrl_r },
	{ 0xc41000, 0xc41001, input_port_0_word_r },/* p1 */
	{ 0xc41002, 0xc41003, input_port_1_word_r }, /* p2 */
	{ 0xc41004, 0xc41005, input_port_2_word_r }, /* p3 */
	{ 0xc41006, 0xc41007, input_port_3_word_r }, /* p4 */
	{ 0xc42000, 0xc42001, input_port_3_word_r },/* dip1 */
	{ 0xc42002, 0xc42003, input_port_5_word_r }, /* dip2 */
/*	{ 0xffc800, 0xffc801, quartet_skip_r }, */
	{ 0xffc000, 0xffffff, SYS16_MRA16_WORKINGRAM },
MEMORY_END

static MEMORY_WRITE16_START( quartet_writemem )
	{ 0x000000, 0x02ffff, MWA16_ROM },
	{ 0x400000, 0x40ffff, SYS16_MWA16_TILERAM, &sys16_tileram },
	{ 0x410000, 0x410fff, SYS16_MWA16_TEXTRAM, &sys16_textram },
	{ 0x440000, 0x440fff, SYS16_MWA16_SPRITERAM, &sys16_spriteram },
	{ 0x840000, 0x840fff, SYS16_MWA16_PALETTERAM, &paletteram16 },
	{ 0xc40000, 0xc40001, sound_command_nmi_w },
	{ 0xc40002, 0xc40003, sys16_3d_coinctrl_w },
	{ 0xffc000, 0xffffff, SYS16_MWA16_WORKINGRAM, &sys16_workingram },
MEMORY_END

/***************************************************************************/

static void quartet_update_proc( void )
{
	set_fg_page1( sys16_workingram[0x0d1c/2] );
	set_bg_page1( sys16_workingram[0x0d1e/2] );
	sys16_fg_scrolly = sys16_textram[0x792] & 0x00ff;
	sys16_bg_scrolly = sys16_textram[0x793] & 0x01ff;
	sys16_fg_scrollx = sys16_workingram[0x0d14/2] & 0x01ff;
	sys16_bg_scrollx = sys16_workingram[0x0d18/2] & 0x01ff;

/*let's fix this properly */
/*	if(((*(UINT16 *)(&sys16_extraram[4])) & 0xff) == 1) */
/*		sys16_quartet_title_kludge=1; */
/*	else */
		sys16_quartet_title_kludge=0;
}

static MACHINE_INIT( quartet ){
	sys16_textmode=1;
	sys16_spritesystem = sys16_sprite_quartet2;
	sys16_sprxoffset = -0xbd;
	sys16_fgxoffset = sys16_bgxoffset = 8;

	sys16_update_proc = quartet_update_proc;
}

/***************************************************************************/

INPUT_PORTS_START( quartet )
	/* Player 1 */
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY  )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP  | IPF_8WAY  )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* player 1 coin 2 really */
	/* Player 2 */
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY  | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP  | IPF_8WAY  | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* player 2 coin 2 really */
	/* Player 3 */
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3  )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP  | IPF_8WAY  | IPF_PLAYER3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* player 3 coin 2 really */
	/* Player 4 */
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY  | IPF_PLAYER4 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP  | IPF_8WAY  | IPF_PLAYER4 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER4)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* player 4 coin 2 really */

	SYS16_COINAGE

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x06, 0x00, "Credit Power" )
	PORT_DIPSETTING(    0x04, "500" )
	PORT_DIPSETTING(    0x06, "1000" )
	PORT_DIPSETTING(    0x02, "2000" )
	PORT_DIPSETTING(    0x00, "9000" )
	PORT_DIPNAME( 0x18, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x10, "Easy" )
	PORT_DIPSETTING(    0x18, "Normal" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x20, 0x20, "Coin During Game" )
	PORT_DIPSETTING(    0x20, "Power" )
	PORT_DIPSETTING(    0x00, "Credit" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )
INPUT_PORTS_END

/***************************************************************************/

static MACHINE_DRIVER_START( quartet )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16_7751)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(quartet_readmem,quartet_writemem)

	MDRV_MACHINE_INIT(quartet)
MACHINE_DRIVER_END

/***************************************************************************/
/* pre16 */
ROM_START( quartet2 )
	ROM_REGION( 0x030000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "quartet2.b9",  0x000000, 0x8000, CRC(67177cd8) SHA1(c4ea001dfbeeb29a09d597fb50d71f54e4e9572a) )
	ROM_LOAD16_BYTE( "quartet2.b6",  0x000001, 0x8000, CRC(50f50b08) SHA1(646c0d545150b95e5d8d47bf63360f7326add08f) )
	ROM_LOAD16_BYTE( "quartet2.b10", 0x010000, 0x8000, CRC(4273c3b7) SHA1(4cae221678a6d2b7806487becd4ba09b520f9fa0) )
	ROM_LOAD16_BYTE( "quartet2.b7",  0x010001, 0x8000, CRC(0aa337bb) SHA1(f31f8f294fccd866eadebfafee067bfae44b3184) )
	ROM_LOAD16_BYTE( "quartet2.b11", 0x020000, 0x8000, CRC(3a6a375d) SHA1(8ebea6b7f1208438b47e887b46cb569725c4042a) )
	ROM_LOAD16_BYTE( "quartet2.b8",  0x020001, 0x8000, CRC(d87b2ca2) SHA1(58adf0900e41036b1b78a931ab94b30ce601909d) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "quartet2.c9",  0x00000, 0x08000, CRC(547a6058) SHA1(5248e974c8d12183c996b1fc8fda09e8a4bf0d2d) )
	ROM_LOAD( "quartet2.c10", 0x08000, 0x08000, CRC(77ec901d) SHA1(b5961895473c16a8f4a111185cce48b05ab66885) )
	ROM_LOAD( "quartet2.c11", 0x10000, 0x08000, CRC(7e348cce) SHA1(82bba65280faaf3280208c85caef48ec8baeade8) )

	ROM_REGION( 0x040000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "epr7465.5c",  0x000001, 0x8000, CRC(8a1ab7d7) SHA1(a2f317538c70a1603b65d795223407cbaaf88524) )
	ROM_LOAD16_BYTE( "epr-7469.2b", 0x000000, 0x8000, CRC(cb65ae4f) SHA1(3ee7b3b4cce113a6f394e8dfd317cdb6ffae64f7) )
	ROM_LOAD16_BYTE( "epr7466.6c",  0x010001, 0x8000, CRC(b2d3f4f3) SHA1(65e654fde10bee4cb5eee8234d0babb78fe41cfb) )
	ROM_LOAD16_BYTE( "epr-7470.3b", 0x010000, 0x8000, CRC(16fc67b1) SHA1(788fe2878c5c9faea43c2f166f32c22ee51c7d09) )
	ROM_LOAD16_BYTE( "epr7467.7c",  0x020001, 0x8000, CRC(0af68de2) SHA1(81163baf3f0e45bac950a6d9c24b3a886db1509c) )
	ROM_LOAD16_BYTE( "epr-7471.4b", 0x020000, 0x8000, CRC(13fad5ac) SHA1(75b480083fbb14cbef969126989bf9b2235fd31e) )
	ROM_LOAD16_BYTE( "epr7468.8c",  0x030001, 0x8000, CRC(ddfd40c0) SHA1(6c12ad668cd0c82e7d7d46bfbdcee8b9d46ebd09) )
	ROM_LOAD16_BYTE( "epr-7472.5b", 0x030000, 0x8000, CRC(8e2762ec) SHA1(872e19a6aab81d7a2472367d0e31dc1295da7182) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr-7464.1b", 0x0000, 0x8000, CRC(9f291306) SHA1(96a09542a863ccf2ded43e2df6f913722b3f97b1) )

	ROM_REGION( 0x1000, REGION_CPU3, 0 )      /* 4k for 7751 onboard ROM */
	ROM_LOAD( "7751.bin",     0x0000, 0x0400, CRC(6a9534fc) SHA1(67ad94674db5c2aab75785668f610f6f4eccd158) ) /* 7751 - U34 */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* 7751 sound data */
	ROM_LOAD( "epr7473.1c", 0x00000, 0x8000, CRC(06ec75fa) SHA1(5f14bc887449122700c46ad22c0379a1682e0bdb) )
	ROM_LOAD( "epr7475.2c", 0x08000, 0x8000, CRC(7abd1206) SHA1(54d52dc0b9c245cd2df647e714310a71b803cbcf) )
	ROM_LOAD( "epr7474.3c", 0x10000, 0x8000, CRC(dbf853b8) SHA1(e82f497e1144f23f3233b5c45ef182bfc7923715) )
	ROM_LOAD( "epr7476.4c", 0x18000, 0x8000, CRC(5eba655a) SHA1(6713ef12037cba3139d0f469c82bd90b44bae8ce) )
ROM_END

ROM_START( quartt2j )
	ROM_REGION( 0x030000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr-7728.43",  0x000000, 0x8000, CRC(56a8c88e) SHA1(33eaca5272f3588058952ca0b1fa298b89418e81) )
	ROM_LOAD16_BYTE( "epr-7725.26",  0x000001, 0x8000, CRC(ee15fcc9) SHA1(70d9755145245537f6aeb0d39abeda7811749b8c) )
	ROM_LOAD16_BYTE( "epr-7729.42",  0x010000, 0x8000, CRC(bc242123) SHA1(8e58dd89b70ba06d12437010a7375464647262f5) )
	ROM_LOAD16_BYTE( "epr-7726.25",  0x010001, 0x8000, CRC(9d1c48e7) SHA1(e11a358895c7809cdf7241ff9317c2b162e4040e) )
	ROM_LOAD16_BYTE( "quartet2.b11", 0x020000, 0x8000, CRC(3a6a375d) SHA1(8ebea6b7f1208438b47e887b46cb569725c4042a) )
	ROM_LOAD16_BYTE( "quartet2.b8",  0x020001, 0x8000, CRC(d87b2ca2) SHA1(58adf0900e41036b1b78a931ab94b30ce601909d) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "quartet2.c9",  0x00000, 0x08000, CRC(547a6058) SHA1(5248e974c8d12183c996b1fc8fda09e8a4bf0d2d) )
	ROM_LOAD( "quartet2.c10", 0x08000, 0x08000, CRC(77ec901d) SHA1(b5961895473c16a8f4a111185cce48b05ab66885) )
	ROM_LOAD( "quartet2.c11", 0x10000, 0x08000, CRC(7e348cce) SHA1(82bba65280faaf3280208c85caef48ec8baeade8) )

	ROM_REGION( 0x040000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "epr7465.5c",  0x000001, 0x8000, CRC(8a1ab7d7) SHA1(a2f317538c70a1603b65d795223407cbaaf88524) )
	ROM_LOAD16_BYTE( "epr-7469.2b", 0x000000, 0x8000, CRC(cb65ae4f) SHA1(3ee7b3b4cce113a6f394e8dfd317cdb6ffae64f7) )
	ROM_LOAD16_BYTE( "epr7466.6c",  0x010001, 0x8000, CRC(b2d3f4f3) SHA1(65e654fde10bee4cb5eee8234d0babb78fe41cfb) )
	ROM_LOAD16_BYTE( "epr-7470.3b", 0x010000, 0x8000, CRC(16fc67b1) SHA1(788fe2878c5c9faea43c2f166f32c22ee51c7d09) )
	ROM_LOAD16_BYTE( "epr7467.7c",  0x020001, 0x8000, CRC(0af68de2) SHA1(81163baf3f0e45bac950a6d9c24b3a886db1509c) )
	ROM_LOAD16_BYTE( "epr-7471.4b", 0x020000, 0x8000, CRC(13fad5ac) SHA1(75b480083fbb14cbef969126989bf9b2235fd31e) )
	ROM_LOAD16_BYTE( "epr7468.8c",  0x030001, 0x8000, CRC(ddfd40c0) SHA1(6c12ad668cd0c82e7d7d46bfbdcee8b9d46ebd09) )
	ROM_LOAD16_BYTE( "epr-7472.5b", 0x030000, 0x8000, CRC(8e2762ec) SHA1(872e19a6aab81d7a2472367d0e31dc1295da7182) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr-7464.1b", 0x0000, 0x8000, CRC(9f291306) SHA1(96a09542a863ccf2ded43e2df6f913722b3f97b1) )

	ROM_REGION( 0x1000, REGION_CPU3, 0 )      /* 4k for 7751 onboard ROM */
	ROM_LOAD( "7751.bin",     0x0000, 0x0400, CRC(6a9534fc) SHA1(67ad94674db5c2aab75785668f610f6f4eccd158) ) /* 7751 - U34 */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* 7751 sound data */
	ROM_LOAD( "epr7473.1c", 0x00000, 0x8000, CRC(06ec75fa) SHA1(5f14bc887449122700c46ad22c0379a1682e0bdb) )
	ROM_LOAD( "epr7475.2c", 0x08000, 0x8000, CRC(7abd1206) SHA1(54d52dc0b9c245cd2df647e714310a71b803cbcf) )
	ROM_LOAD( "epr7474.3c", 0x10000, 0x8000, CRC(dbf853b8) SHA1(e82f497e1144f23f3233b5c45ef182bfc7923715) )
	ROM_LOAD( "epr7476.4c", 0x18000, 0x8000, CRC(5eba655a) SHA1(6713ef12037cba3139d0f469c82bd90b44bae8ce) )
ROM_END

/***************************************************************************/

#if 0
static READ16_HANDLER( quartet2_skip_r ){
	if (activecpu_get_pc()==0x8f6c) {cpu_spinuntil_int(); return 0xffff;}
	return sys16_workingram[0x0800/2];
}
#endif

static MEMORY_READ16_START( quartet2_readmem )
	{ 0x000000, 0x02ffff, MRA16_ROM },
	{ 0x400000, 0x40ffff, SYS16_MRA16_TILERAM },
	{ 0x410000, 0x410fff, SYS16_MRA16_TEXTRAM },
	{ 0x440000, 0x440fff, SYS16_MRA16_SPRITERAM },
	{ 0x840000, 0x840fff, SYS16_MRA16_PALETTERAM },
	{ 0xc40002, 0xc40003, sys16_coinctrl_r },
	{ 0xc41002, 0xc41003, input_port_0_word_r }, /* player1 */
	{ 0xc41006, 0xc41007, input_port_1_word_r }, /* player2 */
	{ 0xc41000, 0xc41001, input_port_2_word_r }, /* service */
	{ 0xc42000, 0xc42001, input_port_3_word_r },/* dip1 */
	{ 0xc42002, 0xc42003, input_port_4_word_r }, /* dip2 */
/*	{ 0xffc800, 0xffc801, quartet2_skip_r }, */
	{ 0xffc000, 0xffffff, SYS16_MRA16_WORKINGRAM },
MEMORY_END

static MEMORY_WRITE16_START( quartet2_writemem )
	{ 0x000000, 0x02ffff, MWA16_ROM },
	{ 0x400000, 0x40ffff, SYS16_MWA16_TILERAM, &sys16_tileram },
	{ 0x410000, 0x410fff, SYS16_MWA16_TEXTRAM, &sys16_textram },
	{ 0x440000, 0x440fff, SYS16_MWA16_SPRITERAM, &sys16_spriteram },
	{ 0x840000, 0x840fff, SYS16_MWA16_PALETTERAM, &paletteram16 },
	{ 0xc40000, 0xc40001, sound_command_nmi_w },
	{ 0xc40002, 0xc40003, sys16_3d_coinctrl_w },
	{ 0xffc000, 0xffffff, SYS16_MWA16_WORKINGRAM, &sys16_workingram },
MEMORY_END

/***************************************************************************/

static MACHINE_INIT( quartet2 ){
	sys16_textmode=1;
	sys16_spritesystem = sys16_sprite_quartet2;
	sys16_sprxoffset = -0xbd;
	sys16_fgxoffset = sys16_bgxoffset = 8;

	sys16_update_proc = quartet_update_proc;
}

/***************************************************************************/

INPUT_PORTS_START( quartet2 )
	SYS16_JOY1_SWAPPEDBUTTONS
	SYS16_JOY2_SWAPPEDBUTTONS
	SYS16_SERVICE
	SYS16_COINAGE

PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x06, 0x00, "Credit Power" )
	PORT_DIPSETTING(    0x04, "500" )
	PORT_DIPSETTING(    0x06, "1000" )
	PORT_DIPSETTING(    0x02, "2000" )
	PORT_DIPSETTING(    0x00, "9000" )
	PORT_DIPNAME( 0x18, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x10, "Easy" )
	PORT_DIPSETTING(    0x18, "Normal" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

INPUT_PORTS_END

/***************************************************************************/

static MACHINE_DRIVER_START( quartet2 )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16_7751)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(quartet2_readmem,quartet2_writemem)

	MDRV_MACHINE_INIT(quartet2)
MACHINE_DRIVER_END

/***************************************************************************

   Riot City

***************************************************************************/
/* sys16B */
ROM_START( riotcity )
	ROM_REGION( 0xc0000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr14612.bin", 0x000000, 0x20000, CRC(a1b331ec) SHA1(49136ffed35ecc9e5e9a6ea7acbe534e6ccc9dd8) )
	ROM_LOAD16_BYTE( "epr14610.bin", 0x000001, 0x20000, CRC(cd4f2c50) SHA1(c7a7e95901c664a72195c202b50a159db8d5981d) )
	/* empty 0x40000 - 0x80000 */
	ROM_LOAD16_BYTE( "epr14613.bin", 0x080000, 0x20000, CRC(0659df4c) SHA1(a24005ce354113732306c318de373c818400a9c8) )
	ROM_LOAD16_BYTE( "epr14611.bin", 0x080001, 0x20000, CRC(d9e6f80b) SHA1(5ac56b3685bb121a4f07be3d81209807436e76ec) )

	ROM_REGION( 0xc0000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "epr14616.bin", 0x00000, 0x20000, CRC(46d30368) SHA1(a791ef0e881345b6be0b28f32af17127edb5b590) ) /* plane 1 */
	ROM_LOAD( "epr14625.bin", 0x20000, 0x20000, CRC(abfb80fe) SHA1(5f0d61d55f27e8f776b573e3193164c4d70ec12b) )
	ROM_LOAD( "epr14617.bin", 0x40000, 0x20000, CRC(884e40f9) SHA1(f7eeae57544e7d95139588137605986732937d38) ) /* plane 2 */
	ROM_LOAD( "epr14626.bin", 0x60000, 0x20000, CRC(4ef55846) SHA1(2f23474e7d1d8880dc251ada55c5fca2fc19256a) )
	ROM_LOAD( "epr14618.bin", 0x80000, 0x20000, CRC(00eb260e) SHA1(f293180fb9a053c98022ef086bf4002563752f61) ) /* plane 3 */
	ROM_LOAD( "epr14627.bin", 0xa0000, 0x20000, CRC(961e5f82) SHA1(dc88b511dff6cdebf96fe8bf388bf76098296b0f) )

	ROM_REGION( 0x180000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "epr14619.bin",  0x000001, 0x040000, CRC(6f2b5ef7) SHA1(a0186fcc5b12c31b65b84355f88bcb10b1434135) )
	ROM_LOAD16_BYTE( "epr14622.bin",  0x000000, 0x040000, CRC(7ca7e40d) SHA1(57d26cc1b530cb867b2a4779bb5108ac457b2154) )
	ROM_LOAD16_BYTE( "epr14620.bin",  0x080001, 0x040000, CRC(66183333) SHA1(44bb9d57cd0308c0d7b6a10ba9bd95763ceea775) )
	ROM_LOAD16_BYTE( "epr14623.bin",  0x080000, 0x040000, CRC(98630049) SHA1(216ae0b4a59f306b51bd50dfcbf831c3186a4d2a) )
	ROM_LOAD16_BYTE( "epr14621.bin",  0x100001, 0x040000, CRC(c0f2820e) SHA1(ae433f6c5065ed495f5f57f50d6abe5ff98b041e) )
	ROM_LOAD16_BYTE( "epr14624.bin",  0x100000, 0x040000, CRC(d1a68448) SHA1(7591f0476e899a11042d9b7e93f99b64de48b0ef) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr14614.bin", 0x00000, 0x10000, CRC(c65cc69a) SHA1(28a75dd2085b8e1447fe4e6af210a54a6666fcb1) )
	ROM_LOAD( "epr14615.bin", 0x10000, 0x20000, CRC(46653db1) SHA1(7a43d8742ee451d93bb5f1b0f4f261b274c3f0ef) )
ROM_END

/***************************************************************************/

static READ16_HANDLER( riotcity_skip_r ){
	if (activecpu_get_pc()==0x3ce) {cpu_spinuntil_int(); return 0;}
	return sys16_workingram[0x2cde/2];
}

static MEMORY_READ16_START( riotcity_readmem )
	{ 0x000000, 0x0bffff, MRA16_ROM },
	{ 0xf40000, 0xf40fff, SYS16_MRA16_SPRITERAM },
	{ 0xf60000, 0xf60fff, SYS16_MRA16_PALETTERAM },
	{ 0xf81002, 0xf81003, input_port_0_word_r }, /* player1 */
	{ 0xf81006, 0xf81007, input_port_1_word_r }, /* player2 */
	{ 0xf81000, 0xf81001, input_port_2_word_r }, /* service */
	{ 0xf82002, 0xf82003, input_port_3_word_r }, /* dip1 */
	{ 0xf82000, 0xf82001, input_port_4_word_r }, /* dip2 */
	{ 0xfa0000, 0xfaffff, SYS16_MRA16_TILERAM },
	{ 0xfb0000, 0xfb0fff, SYS16_MRA16_TEXTRAM },
	{ 0xffecde, 0xffecdf, riotcity_skip_r },
	{ 0xffc000, 0xffffff, SYS16_MRA16_WORKINGRAM },
MEMORY_END

static MEMORY_WRITE16_START( riotcity_writemem )
	{ 0x000000, 0x0bffff, MWA16_ROM },
	{ 0xf00006, 0xf00007, sound_command_w },
	{ 0xf00020, 0xf0003f, MWA16_NOP }, /* config regs */
	{ 0xf20000, 0xf2ffff, sys16_tilebank_w },
	{ 0xf40000, 0xf40fff, SYS16_MWA16_SPRITERAM, &sys16_spriteram },
	{ 0xf60000, 0xf60fff, SYS16_MWA16_PALETTERAM, &paletteram16 },
	{ 0xf80000, 0xf80001, sys16_coinctrl_w },
	{ 0xfa0000, 0xfaffff, SYS16_MWA16_TILERAM, &sys16_tileram },
	{ 0xfb0000, 0xfb0fff, SYS16_MWA16_TEXTRAM, &sys16_textram },
	{ 0xffc000, 0xffffff, SYS16_MWA16_WORKINGRAM, &sys16_workingram },
MEMORY_END

/***************************************************************************/

static MACHINE_INIT( riotcity ){
	static int bank[16] = {
		0x0,0x1,0x4,0x5,
		0x8,0x9,0x0,0x0,
		0x2,0x3,0x6,0x7,
		0xa,0xb,0x0,0x0
	};
	sys16_obj_bank = bank;
	sys16_spritesystem = sys16_sprite_shinobi;
	sys16_bg_priority_mode=1;

	sys16_update_proc = type0_sys16_textram;
}

/***************************************************************************/

INPUT_PORTS_START( riotcity )
	SYS16_JOY1
	SYS16_JOY2
	SYS16_SERVICE
	SYS16_COINAGE

PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, "2 Credits to Start" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x08, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x0c, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x20, "Easy" )
	PORT_DIPSETTING(    0x30, "Normal" )
	PORT_DIPSETTING(    0x10, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x80, 0x80, "Attack Button to Start" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

/***************************************************************************/

static MACHINE_DRIVER_START( riotcity )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16_7759b)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(riotcity_readmem,riotcity_writemem)

	MDRV_MACHINE_INIT(riotcity)
MACHINE_DRIVER_END

/***************************************************************************/

/* sys16B */
ROM_START( sdi )
	ROM_REGION( 0x030000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "a4.rom", 0x000000, 0x8000, CRC(f2c41dd6) SHA1(7fdbf819e42c7af0efe3976ffd919ee142efe1bc) )
	ROM_LOAD16_BYTE( "a1.rom", 0x000001, 0x8000, CRC(a9f816ef) SHA1(5ccab14b088d2249f83ff5e80591eccb4afb6e20) )
	ROM_LOAD16_BYTE( "a5.rom", 0x010000, 0x8000, CRC(7952e27e) SHA1(caead75724a6744fc6fc7fbbc9894932a7a22eed) )
	ROM_LOAD16_BYTE( "a2.rom", 0x010001, 0x8000, CRC(369af326) SHA1(d6517c38f3a386e8f23b058fe8fa0607918ba215) )
	ROM_LOAD16_BYTE( "a6.rom", 0x020000, 0x8000, CRC(8ee2c287) SHA1(8ed98334dab51c2eab8e1ff0724abc1f819dc8c2) )
	ROM_LOAD16_BYTE( "a3.rom", 0x020001, 0x8000, CRC(193e4231) SHA1(14fecfab010641b83e5b24d0e8003bc0de35e1c8) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "b9.rom",  0x00000, 0x10000, CRC(182b6301) SHA1(bb6f1174f82611c801d2b4b7d3596bf71619e8a1) )
	ROM_LOAD( "b10.rom", 0x10000, 0x10000, CRC(8f7129a2) SHA1(094a4065597d8d51fb2232546df1de9043fea731) )
	ROM_LOAD( "b11.rom", 0x20000, 0x10000, CRC(4409411f) SHA1(84fd7128e8440d96b0384ae3c391a59bd37ecf9d) )

	ROM_REGION( 0x60000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "b1.rom", 0x00001, 0x010000, CRC(30e2c50a) SHA1(1fb9e69d4cb97fdcb0f98c2a7ede246aaa4ac382) )
	ROM_LOAD16_BYTE( "b5.rom", 0x00000, 0x010000, CRC(794e3e8b) SHA1(91ca1cb9aabf99adc8426feed4494a992afb8c4a) )
	ROM_LOAD16_BYTE( "b2.rom", 0x20001, 0x010000, CRC(6a8b3fd0) SHA1(a122d3cb0b3263714f026e57d85b0dbf6cb110d7) )
	ROM_LOAD16_BYTE( "b6.rom", 0x20000, 0x010000, CRC(602da5d5) SHA1(d32cdde7d86c4561e7bfa547d7d7995ce9a43c24) )
	ROM_LOAD16_BYTE( "b3.rom", 0x40001, 0x010000, CRC(b9de3aeb) SHA1(2f7a55a8377e831338a884f8962d6ab2757e8c9b) )
	ROM_LOAD16_BYTE( "b7.rom", 0x40000, 0x010000, CRC(0a73a057) SHA1(7f31124c67541a245e069e5b6aac59935d99a9a9) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "a7.rom", 0x0000, 0x8000, CRC(793f9f7f) SHA1(9e4fde376db9e99a83eb2fc734c6721c122ba9af) )
ROM_END

/* sys16A */
ROM_START( sdioj )
	ROM_REGION( 0x030000, REGION_CPU1, 0 ) /* 68000 code */
/* Custom cpu 317-0027 */
	ROM_LOAD16_BYTE( "epr10970.43", 0x000000, 0x8000, CRC(b8fa4a2c) SHA1(06b448bbee0a2b2809d9af7a2a22c5847343c079) )
	ROM_LOAD16_BYTE( "epr10968.26", 0x000001, 0x8000, CRC(a3f97793) SHA1(0f924fae0d13b3387a0e5171482f6d413432ddb3) )
	ROM_LOAD16_BYTE( "epr10971.42", 0x010000, 0x8000, CRC(c44a0328) SHA1(3736bb83e728bb0e15ea58bc2a6c2fe66a1a4885) )
	ROM_LOAD16_BYTE( "epr10969.25", 0x010001, 0x8000, CRC(455d15bd) SHA1(be679ecb1687b0675614ad27973c20808ad53797) )
	ROM_LOAD16_BYTE( "epr10755.41", 0x020000, 0x8000, CRC(405e3969) SHA1(6d8c3bd06d35c971f7db005dffa2e83cae1378f8) )
	ROM_LOAD16_BYTE( "epr10752.24", 0x020001, 0x8000, CRC(77453740) SHA1(9032463e5e14c3c610c31e2eb6e2c962df9adf46) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "epr10756.95", 0x00000, 0x10000, CRC(44d8a506) SHA1(363d49dcb65ac0093f3ed3b259b1bc45f0291e9d) )
	ROM_LOAD( "epr10757.94", 0x10000, 0x10000, CRC(497e1740) SHA1(95b166a9db46a27087e417c1b2cbb76bee2e64a7) )
	ROM_LOAD( "epr10758.93", 0x20000, 0x10000, CRC(61d61486) SHA1(d48ff87216947b78903cd98a10436babdf8b75a0) )

	ROM_REGION( 0x60000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "b1.rom", 0x00001, 0x10000, CRC(30e2c50a) SHA1(1fb9e69d4cb97fdcb0f98c2a7ede246aaa4ac382) )
	ROM_LOAD16_BYTE( "b5.rom", 0x00000, 0x10000, CRC(794e3e8b) SHA1(91ca1cb9aabf99adc8426feed4494a992afb8c4a) )
	ROM_LOAD16_BYTE( "b2.rom", 0x20001, 0x10000, CRC(6a8b3fd0) SHA1(a122d3cb0b3263714f026e57d85b0dbf6cb110d7) )
	ROM_LOAD16_BYTE( "b6.rom", 0x20000, 0x10000, CRC(602da5d5) SHA1(d32cdde7d86c4561e7bfa547d7d7995ce9a43c24) )
	ROM_LOAD16_BYTE( "b3.rom", 0x40001, 0x10000, CRC(b9de3aeb) SHA1(2f7a55a8377e831338a884f8962d6ab2757e8c9b) )
	ROM_LOAD16_BYTE( "b7.rom", 0x40000, 0x10000, CRC(0a73a057) SHA1(7f31124c67541a245e069e5b6aac59935d99a9a9) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr10759.12", 0x0000, 0x8000, CRC(d7f9649f) SHA1(ce4abe7dd7e33da048569d7817063345fab75ea7) )
ROM_END


/***************************************************************************/

static READ16_HANDLER( io_p1mousex_r ){ return 0xff-input_port_5_r( offset ); }
static READ16_HANDLER( io_p1mousey_r ){ return input_port_6_r( offset ); }

static READ16_HANDLER( io_p2mousex_r ){ return input_port_7_r( offset ); }
static READ16_HANDLER( io_p2mousey_r ){ return input_port_8_r( offset ); }

static READ16_HANDLER( sdi_skip_r ){
	if (activecpu_get_pc()==0x5326) {cpu_spinuntil_int(); return 0xffff;}
	return sys16_workingram[0x0400/2];
}

static MEMORY_READ16_START( sdi_readmem )
	{ 0x000000, 0x02ffff, MRA16_ROM },
	{ 0x400000, 0x40ffff, SYS16_MRA16_TILERAM },
	{ 0x410000, 0x411fff, SYS16_MRA16_TEXTRAM },
	{ 0x440000, 0x440fff, SYS16_MRA16_SPRITERAM },
	{ 0x840000, 0x840fff, SYS16_MRA16_PALETTERAM },
	{ 0xc41004, 0xc41005, input_port_0_word_r },/* player1 */
	{ 0xc41002, 0xc41003, input_port_1_word_r }, /* player2 */
	{ 0xc41000, 0xc41001, input_port_2_word_r }, /* service */
	{ 0xc42000, 0xc42001, input_port_4_word_r }, /* dip2 */
	{ 0xc42002, 0xc42003, input_port_3_word_r }, /* dip1 */
	{ 0xc42004, 0xc42005, input_port_4_word_r }, /* dip2 */
	{ 0xc43000, 0xc43001, io_p1mousex_r },
	{ 0xc43004, 0xc43005, io_p1mousey_r },
	{ 0xc43008, 0xc43009, io_p2mousex_r },
	{ 0xc4300c, 0xc4300d, io_p2mousey_r },
/*	{ 0xc42000, 0xc42001, MRA16_NOP }, /* What is this? */ 
	{ 0xc60000, 0xc60001, MRA16_NOP }, /* What is this? */
	{ 0xffc400, 0xffc401, sdi_skip_r },
	{ 0xffc000, 0xffffff, SYS16_MRA16_WORKINGRAM },
MEMORY_END

static MEMORY_WRITE16_START( sdi_writemem )
	{ 0x000000, 0x02ffff, MWA16_ROM },
	{ 0x123406, 0x123407, sound_command_w },
	{ 0x400000, 0x40ffff, SYS16_MWA16_TILERAM, &sys16_tileram },
	{ 0x410000, 0x411fff, SYS16_MWA16_TEXTRAM, &sys16_textram },
	{ 0x440000, 0x440fff, SYS16_MWA16_SPRITERAM, &sys16_spriteram },
	{ 0x840000, 0x840fff, SYS16_MWA16_PALETTERAM, &paletteram16 },
	{ 0xc40000, 0xc40001, sys16_coinctrl_w },
	{ 0xffc000, 0xffffff, SYS16_MWA16_WORKINGRAM, &sys16_workingram },
MEMORY_END

/***************************************************************************/

static MACHINE_INIT( sdi ){
	static int bank[16] = {
		0,0,0,0,
		0,0,0,3,
		0,0,0,2,
		0,1,0,0
	};
	sys16_obj_bank = bank;

	sys16_patch_code( 0x102f2, 0x00 );
	sys16_patch_code( 0x102f3, 0x02 );

	sys16_update_proc = type0_sys16_textram;
}

static DRIVER_INIT( sdi ){
	sys18_splittab_bg_x=&sys16_textram[0x0fc0];
	sys16_rowscroll_scroll=0xff00;
}

/***************************************************************************/

INPUT_PORTS_START( sdi )
PORT_START	/* DSW1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN  | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP    | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT  | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN  | IPF_8WAY | IPF_PLAYER2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP    | IPF_8WAY | IPF_PLAYER2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT | IPF_8WAY | IPF_PLAYER2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT  | IPF_8WAY | IPF_PLAYER2)

	SYS16_JOY2

PORT_START /* Service */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2)

	SYS16_COINAGE

PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "240?", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x20, "Easy" )
	PORT_DIPSETTING(    0x30, "Normal" )
	PORT_DIPSETTING(    0x10, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x80, "Every 50000" )
	PORT_DIPSETTING(    0xc0, "50000" )
	PORT_DIPSETTING(    0x40, "100000" )
	PORT_DIPSETTING(    0x00, "None" )

	PORT_START				/* fake analog X */
	PORT_ANALOG( 0xff, 0x80, IPT_TRACKBALL_X, 75, 1, 0, 255 )

	PORT_START				/* fake analog Y */
	PORT_ANALOG( 0xff, 0x80, IPT_TRACKBALL_Y, 75, 1, 0, 255 )

	PORT_START				/* fake analog X */
	PORT_ANALOG( 0xff, 0x80, IPT_TRACKBALL_X | IPF_PLAYER2 , 75, 1, 0, 255 )

	PORT_START				/* fake analog Y */
	PORT_ANALOG( 0xff, 0x80, IPT_TRACKBALL_Y | IPF_PLAYER2, 75, 1, 0, 255 )

INPUT_PORTS_END

/***************************************************************************/

static MACHINE_DRIVER_START( sdi )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(sdi_readmem,sdi_writemem)

	MDRV_MACHINE_INIT(sdi)
MACHINE_DRIVER_END

/***************************************************************************/


/***************************************************************************/
/* sys16B */
ROM_START( shinobi )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "shinobi.a4", 0x00000, 0x10000, CRC(b930399d) SHA1(955ff2948e1990463631b0bc5c7f5275384236cc) )
	ROM_LOAD16_BYTE( "shinobi.a1", 0x00001, 0x10000, CRC(343f4c46) SHA1(2cf5d00462ad85ae9a2e16d59171c8ab85e10f49) )
	ROM_LOAD16_BYTE( "epr11283",   0x20000, 0x10000, CRC(9d46e707) SHA1(37ab25b3b37365c9f45837bfb6ec80652691dd4c) )
	ROM_LOAD16_BYTE( "epr11281",   0x20001, 0x10000, CRC(7961d07e) SHA1(38cbdab35f901532c0ad99ad0083513abd2ff182) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "shinobi.b9",  0x00000, 0x10000, CRC(5f62e163) SHA1(03f008745a1af84142ada647acf3601049f43ad5) )
	ROM_LOAD( "shinobi.b10", 0x10000, 0x10000, CRC(75f8fbc9) SHA1(29072edcd583af60ec66b4c8bb82b179a3751edf) )
	ROM_LOAD( "shinobi.b11", 0x20000, 0x10000, CRC(06508bb9) SHA1(57c9036123ec8e35d0275ab6eaff25a16aa203d4) )

	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "epr11290.10", 0x00001, 0x10000, CRC(611f413a) SHA1(180f83216e2dfbfd77b0fb3be83c3042954d12df) )
	ROM_LOAD16_BYTE( "epr11294.11", 0x00000, 0x10000, CRC(5eb00fc1) SHA1(97e02eee74f61fabcad2a9e24f1868cafaac1d51) )
	ROM_LOAD16_BYTE( "epr11291.17", 0x20001, 0x10000, CRC(3c0797c0) SHA1(df18c7987281bd9379026c6cf7f96f6ae49fd7f9) )
	ROM_LOAD16_BYTE( "epr11295.18", 0x20000, 0x10000, CRC(25307ef8) SHA1(91ffbe436f80d583524ee113a8b7c0cf5d8ab286) )
	ROM_LOAD16_BYTE( "epr11292.23", 0x40001, 0x10000, CRC(c29ac34e) SHA1(b5e9b8c3233a7d6797f91531a0d9123febcf1660) )
	ROM_LOAD16_BYTE( "epr11296.24", 0x40000, 0x10000, CRC(04a437f8) SHA1(ea5fed64443236e3404fab243761e60e2e48c84c) )
	ROM_LOAD16_BYTE( "epr11293.29", 0x60001, 0x10000, CRC(41f41063) SHA1(5cc461e9738dddf9eea06831fce3702d94674163) )
	ROM_LOAD16_BYTE( "epr11297.30", 0x60000, 0x10000, CRC(b6e1fd72) SHA1(eb86e4bf880bd1a1d9bcab3f2f2e917bcaa06172) )

	ROM_REGION( 0x20000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "shinobi.a7", 0x00000, 0x8000, CRC(2457a7cf) SHA1(ddfac640e442537acb015de8bb088659f5a217ee) )
	ROM_LOAD( "shinobi.a8", 0x10000, 0x8000, CRC(c8df8460) SHA1(0aeb41a493df155edb5f600f53ec43b798927dff) )
	ROM_LOAD( "shinobi.a9", 0x18000, 0x8000, CRC(e5a4cf30) SHA1(d1982da7a550c11ab2253f5d64ac6ab847da0a04) )
ROM_END

ROM_START( shinobib )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 code */
/* Custom cpu 317-0049 */
	ROM_LOAD16_BYTE( "epr11282", 0x00000, 0x10000, CRC(5f2e5524) SHA1(9e5fcabb95abbe6c8178e82f4971abdbc92bff73) )
	ROM_LOAD16_BYTE( "epr11280", 0x00001, 0x10000, CRC(bdfe5c38) SHA1(65f537e38b74c66576d57c770d182dc13302cca6) )
	ROM_LOAD16_BYTE( "epr11283", 0x20000, 0x10000, CRC(9d46e707) SHA1(37ab25b3b37365c9f45837bfb6ec80652691dd4c) )
	ROM_LOAD16_BYTE( "epr11281", 0x20001, 0x10000, CRC(7961d07e) SHA1(38cbdab35f901532c0ad99ad0083513abd2ff182) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "shinobi.b9",  0x00000, 0x10000, CRC(5f62e163) SHA1(03f008745a1af84142ada647acf3601049f43ad5) )
	ROM_LOAD( "shinobi.b10", 0x10000, 0x10000, CRC(75f8fbc9) SHA1(29072edcd583af60ec66b4c8bb82b179a3751edf) )
	ROM_LOAD( "shinobi.b11", 0x20000, 0x10000, CRC(06508bb9) SHA1(57c9036123ec8e35d0275ab6eaff25a16aa203d4) )

	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "epr11290.10", 0x00001, 0x10000, CRC(611f413a) SHA1(180f83216e2dfbfd77b0fb3be83c3042954d12df) )
	ROM_LOAD16_BYTE( "epr11294.11", 0x10000, 0x10000, CRC(5eb00fc1) SHA1(97e02eee74f61fabcad2a9e24f1868cafaac1d51) )
	ROM_LOAD16_BYTE( "epr11291.17", 0x20001, 0x10000, CRC(3c0797c0) SHA1(df18c7987281bd9379026c6cf7f96f6ae49fd7f9) )
	ROM_LOAD16_BYTE( "epr11295.18", 0x30000, 0x10000, CRC(25307ef8) SHA1(91ffbe436f80d583524ee113a8b7c0cf5d8ab286) )
	ROM_LOAD16_BYTE( "epr11292.23", 0x40001, 0x10000, CRC(c29ac34e) SHA1(b5e9b8c3233a7d6797f91531a0d9123febcf1660) )
	ROM_LOAD16_BYTE( "epr11296.24", 0x50000, 0x10000, CRC(04a437f8) SHA1(ea5fed64443236e3404fab243761e60e2e48c84c) )
	ROM_LOAD16_BYTE( "epr11293.29", 0x60001, 0x10000, CRC(41f41063) SHA1(5cc461e9738dddf9eea06831fce3702d94674163) )
	ROM_LOAD16_BYTE( "epr11297.30", 0x70000, 0x10000, CRC(b6e1fd72) SHA1(eb86e4bf880bd1a1d9bcab3f2f2e917bcaa06172) )

	ROM_REGION( 0x20000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "shinobi.a7", 0x00000, 0x8000, CRC(2457a7cf) SHA1(ddfac640e442537acb015de8bb088659f5a217ee) )
	ROM_LOAD( "shinobi.a8", 0x10000, 0x8000, CRC(c8df8460) SHA1(0aeb41a493df155edb5f600f53ec43b798927dff) )
	ROM_LOAD( "shinobi.a9", 0x18000, 0x8000, CRC(e5a4cf30) SHA1(d1982da7a550c11ab2253f5d64ac6ab847da0a04) )

ROM_END

/***************************************************************************/

static READ16_HANDLER( shinobi_skip_r ){
	if (activecpu_get_pc()==0x32e0) {cpu_spinuntil_int(); return 1<<8;}
	return sys16_workingram[0x301c/2];
}

static MEMORY_READ16_START( shinobi_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x400000, 0x40ffff, SYS16_MRA16_TILERAM },
	{ 0x410000, 0x410fff, SYS16_MRA16_TEXTRAM },
	{ 0x440000, 0x440fff, SYS16_MRA16_SPRITERAM },
	{ 0x840000, 0x840fff, SYS16_MRA16_PALETTERAM },
	{ 0xc41002, 0xc41003, input_port_0_word_r }, /* player1 */
	{ 0xc41006, 0xc41007, input_port_1_word_r }, /* player2 */
	{ 0xc41000, 0xc41001, input_port_2_word_r }, /* service */
	{ 0xc42002, 0xc42003, input_port_3_word_r },/* dip1 */
	{ 0xc42000, 0xc42001, input_port_4_word_r }, /* dip2 */
	{ 0xc43000, 0xc43001, MRA16_NOP },
	{ 0xfff01c, 0xfff01d, shinobi_skip_r },
	{ 0xffc000, 0xffffff, SYS16_MRA16_WORKINGRAM },
MEMORY_END

static MEMORY_WRITE16_START( shinobi_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x400000, 0x40ffff, SYS16_MWA16_TILERAM, &sys16_tileram },
	{ 0x410000, 0x410fff, SYS16_MWA16_TEXTRAM, &sys16_textram },
	{ 0x440000, 0x440fff, SYS16_MWA16_SPRITERAM, &sys16_spriteram },
	{ 0x840000, 0x840fff, SYS16_MWA16_PALETTERAM, &paletteram16 },
	{ 0xc40000, 0xc40001, sys16_coinctrl_w },
	{ 0xc43000, 0xc43001, MWA16_NOP },
	{ 0xfe0006, 0xfe0007, sound_command_w },
	{ 0xffc000, 0xffffff, SYS16_MWA16_WORKINGRAM, &sys16_workingram },
MEMORY_END

/***************************************************************************/

static MACHINE_INIT( shinobi ){
	static int bank[16] = {
		0,0,0,0,
		0,0,0,3,
		0,0,0,2,
		0,1,0,0
	};
	sys16_obj_bank = bank;
	sys16_update_proc = type0_sys16_textram;
}

/***************************************************************************/

INPUT_PORTS_START( shinobi )
	SYS16_JOY1
	SYS16_JOY2
	SYS16_SERVICE
	SYS16_COINAGE

PORT_START
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "240", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x20, "Easy" )
	PORT_DIPSETTING(    0x30, "Normal" )
	PORT_DIPSETTING(    0x10, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x40, 0x40, "Enemy's Bullet Speed" )
	PORT_DIPSETTING(    0x40, "Slow" )
	PORT_DIPSETTING(    0x00, "Fast" )
	PORT_DIPNAME( 0x80, 0x80, "Language" )
	PORT_DIPSETTING(    0x80, "Japanese" )
	PORT_DIPSETTING(    0x00, "English" )

INPUT_PORTS_END

/***************************************************************************/

static MACHINE_DRIVER_START( shinobi )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16_7759b)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(shinobi_readmem,shinobi_writemem)

	MDRV_MACHINE_INIT(shinobi)
MACHINE_DRIVER_END

/***************************************************************************/
/* sys16A */
ROM_START( shinobia )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* 68000 code */
/* custom cpu 317-0050 */
	ROM_LOAD16_BYTE( "epr11262.42", 0x000000, 0x10000, CRC(d4b8df12) SHA1(64bfa2dd8a3d99728d9eeb114887272d9590d0b8) )
	ROM_LOAD16_BYTE( "epr11260.27", 0x000001, 0x10000, CRC(2835c95d) SHA1(b5b42af265d3a16183e02d58b053ec2894072679) )
	ROM_LOAD16_BYTE( "epr11263.43", 0x020000, 0x10000, CRC(a2a620bd) SHA1(f8b135ce14d6c5eac5e40ddfd5ad2f1e6f2bc7a6) )
	ROM_LOAD16_BYTE( "epr11261.25", 0x020001, 0x10000, CRC(a3ceda52) SHA1(97a1c52a162fb1d43b3f8f16613b70ce582a8d26) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "epr11264.95", 0x00000, 0x10000, CRC(46627e7d) SHA1(66bb5b22a2100e7b9df303007a837bc2d52cf7ba) )
	ROM_LOAD( "epr11265.94", 0x10000, 0x10000, CRC(87d0f321) SHA1(885b38eaff2dcaeab4eeaa20cc8a2885d520abd6) )
	ROM_LOAD( "epr11266.93", 0x20000, 0x10000, CRC(efb4af87) SHA1(0b8a905023e1bc808fd2b1c3cfa3778cde79e659) )

	ROM_REGION( 0x080000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "epr11290.10", 0x00001, 0x10000, CRC(611f413a) SHA1(180f83216e2dfbfd77b0fb3be83c3042954d12df) )
	ROM_LOAD16_BYTE( "epr11294.11", 0x00000, 0x10000, CRC(5eb00fc1) SHA1(97e02eee74f61fabcad2a9e24f1868cafaac1d51) )
	ROM_LOAD16_BYTE( "epr11291.17", 0x20001, 0x10000, CRC(3c0797c0) SHA1(df18c7987281bd9379026c6cf7f96f6ae49fd7f9) )
	ROM_LOAD16_BYTE( "epr11295.18", 0x20000, 0x10000, CRC(25307ef8) SHA1(91ffbe436f80d583524ee113a8b7c0cf5d8ab286) )
	ROM_LOAD16_BYTE( "epr11292.23", 0x40001, 0x10000, CRC(c29ac34e) SHA1(b5e9b8c3233a7d6797f91531a0d9123febcf1660) )
	ROM_LOAD16_BYTE( "epr11296.24", 0x40000, 0x10000, CRC(04a437f8) SHA1(ea5fed64443236e3404fab243761e60e2e48c84c) )
	ROM_LOAD16_BYTE( "epr11293.29", 0x60001, 0x10000, CRC(41f41063) SHA1(5cc461e9738dddf9eea06831fce3702d94674163) )
	ROM_LOAD16_BYTE( "epr11297.30", 0x60000, 0x10000, CRC(b6e1fd72) SHA1(eb86e4bf880bd1a1d9bcab3f2f2e917bcaa06172) )

	ROM_REGION( 0x20000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr11267.12", 0x0000, 0x8000, CRC(dd50b745) SHA1(52e1977569d3713ad864d607170c9a61cd059a65) )

	ROM_REGION( 0x1000, REGION_CPU3, 0 )      /* 4k for 7751 onboard ROM */
	ROM_LOAD( "7751.bin",     0x0000, 0x0400, CRC(6a9534fc) SHA1(67ad94674db5c2aab75785668f610f6f4eccd158) ) /* 7751 - U34 */

	ROM_REGION( 0x08000, REGION_SOUND1, 0 ) /* 7751 sound data */
	ROM_LOAD( "epr11268.1", 0x0000, 0x8000, CRC(6d7966da) SHA1(90f55a99f784c21d7c135e630f4e8b1d4d043d66) )
ROM_END


ROM_START( shinobl )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* 68000 code */
/* Star Bootleg */
	ROM_LOAD16_BYTE( "b3",          0x000000, 0x10000, CRC(38e59646) SHA1(6a13015a93260ab99811b95950bb122eade01c27) )
	ROM_LOAD16_BYTE( "b1",          0x000001, 0x10000, CRC(8529d192) SHA1(202b912d20a2d82abe055b4a5e8c509ab7d69ff8) )
	ROM_LOAD16_BYTE( "epr11263.43", 0x020000, 0x10000, CRC(a2a620bd) SHA1(f8b135ce14d6c5eac5e40ddfd5ad2f1e6f2bc7a6) )
	ROM_LOAD16_BYTE( "epr11261.25", 0x020001, 0x10000, CRC(a3ceda52) SHA1(97a1c52a162fb1d43b3f8f16613b70ce582a8d26) )

/* Beta Bootleg */
/*	ROM_LOAD16_BYTE( "4",           0x000000, 0x10000, CRC(c178a39c) ) */
/*	ROM_LOAD16_BYTE( "2",           0x000001, 0x10000, CRC(5ad8ebf2) ) */
/*	ROM_LOAD16_BYTE( "epr11263.43", 0x020000, 0x10000, CRC(a2a620bd) SHA1(f8b135ce14d6c5eac5e40ddfd5ad2f1e6f2bc7a6) ) */
/*	ROM_LOAD16_BYTE( "epr11261.25", 0x020001, 0x10000, CRC(a3ceda52) SHA1(97a1c52a162fb1d43b3f8f16613b70ce582a8d26) ) */

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "epr11264.95", 0x00000, 0x10000, CRC(46627e7d) SHA1(66bb5b22a2100e7b9df303007a837bc2d52cf7ba) )
	ROM_LOAD( "epr11265.94", 0x10000, 0x10000, CRC(87d0f321) SHA1(885b38eaff2dcaeab4eeaa20cc8a2885d520abd6) )
	ROM_LOAD( "epr11266.93", 0x20000, 0x10000, CRC(efb4af87) SHA1(0b8a905023e1bc808fd2b1c3cfa3778cde79e659) )

	ROM_REGION( 0x080000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "epr11290.10", 0x00001, 0x10000, CRC(611f413a) SHA1(180f83216e2dfbfd77b0fb3be83c3042954d12df) )
	ROM_LOAD16_BYTE( "epr11294.11", 0x00000, 0x10000, CRC(5eb00fc1) SHA1(97e02eee74f61fabcad2a9e24f1868cafaac1d51) )
	ROM_LOAD16_BYTE( "epr11291.17", 0x20001, 0x10000, CRC(3c0797c0) SHA1(df18c7987281bd9379026c6cf7f96f6ae49fd7f9) )
	ROM_LOAD16_BYTE( "epr11295.18", 0x20000, 0x10000, CRC(25307ef8) SHA1(91ffbe436f80d583524ee113a8b7c0cf5d8ab286) )
	ROM_LOAD16_BYTE( "epr11292.23", 0x40001, 0x10000, CRC(c29ac34e) SHA1(b5e9b8c3233a7d6797f91531a0d9123febcf1660) )
	ROM_LOAD16_BYTE( "epr11296.24", 0x40000, 0x10000, CRC(04a437f8) SHA1(ea5fed64443236e3404fab243761e60e2e48c84c) )
	ROM_LOAD16_BYTE( "epr11293.29", 0x60001, 0x10000, CRC(41f41063) SHA1(5cc461e9738dddf9eea06831fce3702d94674163) )
/*	ROM_LOAD16_BYTE( "epr11297.30", 0x60000, 0x10000, CRC(b6e1fd72) SHA1(eb86e4bf880bd1a1d9bcab3f2f2e917bcaa06172) ) */
	ROM_LOAD16_BYTE( "b17",         0x60000, 0x10000, CRC(0315cf42) SHA1(2d129171aece883cb9c2805f894b3867ec98332b) )	/* Beta bootleg uses the rom above. */

	ROM_REGION( 0x20000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr11267.12", 0x0000, 0x8000, CRC(dd50b745) SHA1(52e1977569d3713ad864d607170c9a61cd059a65) )

	ROM_REGION( 0x1000, REGION_CPU3, 0 )      /* 4k for 7751 onboard ROM */
	ROM_LOAD( "7751.bin",     0x0000, 0x0400, CRC(6a9534fc) SHA1(67ad94674db5c2aab75785668f610f6f4eccd158) ) /* 7751 - U34 */

	ROM_REGION( 0x08000, REGION_SOUND1, 0 ) /* 7751 sound data */
	ROM_LOAD( "epr11268.1", 0x0000, 0x8000, CRC(6d7966da) SHA1(90f55a99f784c21d7c135e630f4e8b1d4d043d66) )
ROM_END

/***************************************************************************/

static MEMORY_READ16_START( shinobl_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x400000, 0x40ffff, SYS16_MRA16_TILERAM },
	{ 0x410000, 0x410fff, SYS16_MRA16_TEXTRAM },
	{ 0x440000, 0x440fff, SYS16_MRA16_SPRITERAM },
	{ 0x840000, 0x840fff, SYS16_MRA16_PALETTERAM },
	{ 0xc41002, 0xc41003, input_port_0_word_r }, /* player1 */
	{ 0xc41006, 0xc41007, input_port_1_word_r }, /* player2 */
	{ 0xc41000, 0xc41001, input_port_2_word_r }, /* service */
	{ 0xc42000, 0xc42001, input_port_3_word_r }, /* dip1 */
	{ 0xc42002, 0xc42003, input_port_4_word_r }, /* dip2 */
	{ 0xffc000, 0xffffff, SYS16_MRA16_WORKINGRAM },
MEMORY_END

static MEMORY_WRITE16_START( shinobl_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x400000, 0x40ffff, SYS16_MWA16_TILERAM, &sys16_tileram },
	{ 0x410000, 0x410fff, SYS16_MWA16_TEXTRAM, &sys16_textram },
	{ 0x440000, 0x440fff, SYS16_MWA16_SPRITERAM, &sys16_spriteram },
	{ 0x840000, 0x840fff, SYS16_MWA16_PALETTERAM, &paletteram16 },
	{ 0xc40000, 0xc40001, sound_command_nmi_w },
	{ 0xc40002, 0xc40003, sys16_3d_coinctrl_w },
	{ 0xffc000, 0xffffff, SYS16_MWA16_WORKINGRAM, &sys16_workingram },
MEMORY_END

/***************************************************************************/

static MACHINE_INIT( shinobl ){
	static int bank[] = {
		0,2,4,6,
		1,3,5,7
	};
	sys16_obj_bank = bank;
	sys16_textmode=1;
	sys16_spritesystem = sys16_sprite_quartet2;
	sys16_sprxoffset = -0xbd;
	sys16_fgxoffset = sys16_bgxoffset = 8;
	sys16_tilebank_switch=0x2000;

	sys16_update_proc = type1_sys16_textram;
}



/***************************************************************************/

static MACHINE_DRIVER_START( shinobl )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16_7751)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(shinobl_readmem,shinobl_writemem)

	MDRV_MACHINE_INIT(shinobl)
MACHINE_DRIVER_END

/***************************************************************************/

static MEMORY_READ16_START( sonicbom_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x400000, 0x40ffff, SYS16_MRA16_TILERAM },
	{ 0x410000, 0x410fff, SYS16_MRA16_TEXTRAM },
	{ 0x440000, 0x440fff, SYS16_MRA16_SPRITERAM },
	{ 0x840000, 0x840fff, SYS16_MRA16_PALETTERAM },
	{ 0xc41002, 0xc41003, input_port_0_word_r }, /* player1 */
	{ 0xc41006, 0xc41007, input_port_1_word_r }, /* player2 */
	{ 0xc41000, 0xc41001, input_port_2_word_r }, /* service */
	{ 0xc42002, 0xc42003, input_port_3_word_r }, /* dip1 */
	{ 0xc42000, 0xc42001, input_port_4_word_r }, /* dip2 */
	{ 0xffc000, 0xffffff, SYS16_MRA16_WORKINGRAM },
MEMORY_END

static MEMORY_WRITE16_START( sonicbom_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x400000, 0x40ffff, SYS16_MWA16_TILERAM, &sys16_tileram },
	{ 0x410000, 0x410fff, SYS16_MWA16_TEXTRAM, &sys16_textram },
	{ 0x440000, 0x440fff, SYS16_MWA16_SPRITERAM, &sys16_spriteram },
	{ 0x840000, 0x840fff, SYS16_MWA16_PALETTERAM, &paletteram16 },
	{ 0x123406, 0x123407, sound_command_w }, /* sonicbom */
	{ 0xfe0006, 0xfe0007, sound_command_w }, /* tetris */
	{ 0xffc000, 0xffffff, SYS16_MWA16_WORKINGRAM, &sys16_workingram },
MEMORY_END


static MACHINE_INIT( sonicbom )
{
/*	fd1094_machine_init(); using the decrypted version */
/*	sys16_sprxoffset = -0x40; */
	sys16_update_proc = type0_sys16_textram;
}

static DRIVER_INIT( sonicbom )
{
	/*fd1094_driver_init(0x0053); using the decrypted version */
}


INPUT_PORTS_START( sonicbom )
	SYS16_JOY1
	SYS16_JOY2
	SYS16_SERVICE
	SYS16_COINAGE

	PORT_START
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x06, "Easy" )
	PORT_DIPSETTING(    0x07, "Normal" )
	PORT_DIPSETTING(    0x05, "Hard 1" )
	PORT_DIPSETTING(    0x04, "Hard 2" )
	PORT_DIPSETTING(    0x03, "Hard 3" )
	PORT_DIPSETTING(    0x02, "Hard 4" )
	PORT_DIPSETTING(    0x01, "Hard 5" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x18, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x60, 0x60, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x40, "30000" )
	PORT_DIPSETTING(    0x60, "40000" )
	PORT_DIPSETTING(    0x20, "50000" )
	PORT_DIPSETTING(    0x00, "60000" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )
INPUT_PORTS_END

static MACHINE_DRIVER_START( sonicbom )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16_7759b)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(sonicbom_readmem,sonicbom_writemem)

	MDRV_MACHINE_INIT(sonicbom)
MACHINE_DRIVER_END

/***************************************************************************/

ROM_START( sonicbom )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* 68000 code */
    ROM_LOAD16_BYTE( "bootleg_epr-11342.a4",  0x000000, 0x10000, CRC(089158ef) SHA1(25c2685a39a0341dcb59f55dd0b5c7a7b8ee70d4) )
	ROM_LOAD16_BYTE( "bootleg_epr-11340.a1",  0x000001, 0x10000, CRC(253cbd27) SHA1(72dd84ce85fe651747477153b0bbaa870d3166f8) )
	ROM_LOAD16_BYTE( "epr11343.a5",  0x020000, 0x10000, CRC(edfeb7d4) SHA1(0f703e028f9ca9f3c4f5563f3c65ec9b938074a5) )
	ROM_LOAD16_BYTE( "epr11341.a2",  0x020001, 0x10000, CRC(0338f771) SHA1(a1a2928eb3f9826733bad54bbf17f622d9307285) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "opr11344.b9",  0x00000, 0x10000, CRC(59a9f940) SHA1(b1c13cfa9609a22cbe047ee39681ccf8d0b3cf9c) )
	ROM_LOAD( "opr11345.b10", 0x10000, 0x10000, CRC(b44c068b) SHA1(05e875dc418aef12fc48d1df44f680249f6952de) )
	ROM_LOAD( "opr11346.b11", 0x20000, 0x10000, CRC(e5ada66c) SHA1(7e8e34ea909848d0d1b1fcccf628bf9ec169ae9b) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_ERASE00 ) /* sprites */
	ROM_LOAD16_BYTE( "opr11352.b3", 0x60001, 0x010000, CRC(047fa4b0) SHA1(d6c6f920a71db7734000cb64f3001145c4e05c6e) ) /* | */
	ROM_LOAD16_BYTE( "opr11356.b7", 0x60000, 0x010000, CRC(aea3c39d) SHA1(2a4f01463b3f29c8d6858c3e99ef70ac548598b4) ) /*  \Guns on desert floor (2nd attract level) */
	ROM_LOAD16_BYTE( "opr11351.b2", 0xa0001, 0x010000, CRC(63b1f1ca) SHA1(1f19a3af099d4a6ad196968b0a3c17a11384e474) ) /* | */
	ROM_LOAD16_BYTE( "opr11355.b6", 0xa0000, 0x010000, CRC(fe0fa332) SHA1(15ea820f87399e35b798969d3800614701a61161) ) /*  \Turrets on buildings (1st attract level) */
	ROM_LOAD16_BYTE( "opr11350.b1", 0xc0001, 0x010000, CRC(525ba1df) SHA1(e35487c8bf4009a767e54258d9a55056d13ba02a) ) /* | */
	ROM_LOAD16_BYTE( "opr11354.b5", 0xc0000, 0x010000, CRC(793fa3ac) SHA1(14d5a71667b4745d5b556cc15334dd9bff8de93f) ) /*  \Title Logo, Player Ship */
	ROM_LOAD16_BYTE( "opr11353.b4", 0xe0001, 0x010000, CRC(4e0791f8) SHA1(3278bfd478a2fdbcf2d641268c9ca4ccd5a5bd2f) ) /* | */
	ROM_LOAD16_BYTE( "opr11357.b8", 0xe0000, 0x010000, CRC(a7c5ea41) SHA1(405e6ebd2cff22179de70fd31fae5ff967fbaf63) ) /*  \Player Bullets etc. */

	ROM_REGION( 0x30000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr11347.a7", 0x00000, 0x8000, CRC(b41f0ced) SHA1(946b58c0f5b4541fac93add065916319302fe5e0) )
	ROM_LOAD( "epr11348.a8", 0x10000, 0x8000, CRC(89924588) SHA1(9b97da0f562c5faaf12e0f3f8943cdb4fe7a9d24) )
	ROM_LOAD( "epr11349.a9", 0x20000, 0x8000, CRC(8e4b6204) SHA1(ec0a2812b4726b5ff236f2fbb63fd2dd13cf4935) )
ROM_END

/* sys16A custom */
ROM_START( tetris )
	ROM_REGION( 0x020000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr12201.rom", 0x000000, 0x8000, CRC(338e9b51) SHA1(f56a1124c963d4ad72a806b26f9aa906aaa37d2b) )
	ROM_LOAD16_BYTE( "epr12200.rom", 0x000001, 0x8000, CRC(fb058779) SHA1(0045985ea943ebc7e44bd95127c5e5212c2821e8) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "epr12202.rom", 0x00000, 0x10000, CRC(2f7da741) SHA1(51a685673b4a57a13818eca65d122230f20bd9a0) )
	ROM_LOAD( "epr12203.rom", 0x10000, 0x10000, CRC(a6e58ec5) SHA1(5a6c43c989768270e0ab61cfaa5ef86d4607fe20) )
	ROM_LOAD( "epr12204.rom", 0x20000, 0x10000, CRC(0ae98e23) SHA1(f067b81b85f9e03a6373c7c53ff52d5395b8a985) )

	ROM_REGION( 0x10000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "epr12169.rom", 0x0001, 0x8000, CRC(dacc6165) SHA1(87b1a7643e3630ff73b2b117752496e1ea5da23d) )
	ROM_LOAD16_BYTE( "epr12170.rom", 0x0000, 0x8000, CRC(87354e42) SHA1(e7fd55aee59b51d82cb9b619fbb815ad6839560c) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr12205.rom", 0x0000, 0x8000, CRC(6695dc99) SHA1(08123aa24c302bc9243329384bd9c2545a4d50c3) )
ROM_END

/* sys16B */
ROM_START( tetrisbl )
	ROM_REGION( 0x020000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "rom2.bin", 0x000000, 0x10000, CRC(4d165c38) SHA1(04706b1977ae18bd09bafaf8ea65f8e5f32e04b8) )
	ROM_LOAD16_BYTE( "rom1.bin", 0x000001, 0x10000, CRC(1e912131) SHA1(8f53504ac08942ee340489d84eab825e654d0a2c) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "scr01.rom", 0x00000, 0x10000, CRC(62640221) SHA1(c311d3847a981d0e1609f9b3d80481565d32d78c) )
	ROM_LOAD( "scr02.rom", 0x10000, 0x10000, CRC(9abd183b) SHA1(621b017cb34973f9227be383e26b5cd41aea9422) )
	ROM_LOAD( "scr03.rom", 0x20000, 0x10000, CRC(2495fd4e) SHA1(2db94ead9223a67238a97e724668076fc43e5534) )

	ROM_REGION( 0x020000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "obj0-o.rom", 0x00001, 0x10000, CRC(2fb38880) SHA1(0e1b601bbda78d1887951c1f7e752531c281bc83) )
	ROM_LOAD16_BYTE( "obj0-e.rom", 0x00000, 0x10000, CRC(d6a02cba) SHA1(d80000f92e754e89c6ca7b7273feab448fc9a061) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "s-prog.rom", 0x0000, 0x8000, CRC(bd9ba01b) SHA1(fafa7dc36cc057a50ae4cdf7a35f3594292336f4) )
ROM_END

/* sys16B */
ROM_START( tetrisa )
	ROM_REGION( 0x020000, REGION_CPU1, 0 ) /* 68000 code */
/* Custom Cpu 317-0092 */
	ROM_LOAD16_BYTE( "tetris.a7", 0x000000, 0x10000, CRC(9ce15ac9) SHA1(0fdbd3ca37e4c0efa7c66415714bfc10637ced6c) )
	ROM_LOAD16_BYTE( "tetris.a5", 0x000001, 0x10000, CRC(98d590ca) SHA1(4d18409c0b5734d0adcea5646d13f65b687dd05d) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "scr01.rom", 0x00000, 0x10000, CRC(62640221) SHA1(c311d3847a981d0e1609f9b3d80481565d32d78c) )
	ROM_LOAD( "scr02.rom", 0x10000, 0x10000, CRC(9abd183b) SHA1(621b017cb34973f9227be383e26b5cd41aea9422) )
	ROM_LOAD( "scr03.rom", 0x20000, 0x10000, CRC(2495fd4e) SHA1(2db94ead9223a67238a97e724668076fc43e5534) )

	ROM_REGION( 0x020000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "obj0-o.rom", 0x00001, 0x10000, CRC(2fb38880) SHA1(0e1b601bbda78d1887951c1f7e752531c281bc83) )
	ROM_LOAD16_BYTE( "obj0-e.rom", 0x00000, 0x10000, CRC(d6a02cba) SHA1(d80000f92e754e89c6ca7b7273feab448fc9a061) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "s-prog.rom", 0x0000, 0x8000, CRC(bd9ba01b) SHA1(fafa7dc36cc057a50ae4cdf7a35f3594292336f4) )
ROM_END

/***************************************************************************/

static MEMORY_READ16_START( tetris_readmem )
	{ 0x000000, 0x01ffff, MRA16_ROM },
	{ 0x400000, 0x40ffff, SYS16_MRA16_TILERAM },
	{ 0x410000, 0x410fff, SYS16_MRA16_TEXTRAM },
	{ 0x418000, 0x41803f, SYS16_MRA16_EXTRAM2 },
	{ 0x440000, 0x440fff, SYS16_MRA16_SPRITERAM },
	{ 0x840000, 0x840fff, SYS16_MRA16_PALETTERAM },
	{ 0xc41002, 0xc41003, input_port_0_word_r }, /* player1 */
	{ 0xc41006, 0xc41007, input_port_1_word_r }, /* player2 */
	{ 0xc41000, 0xc41001, input_port_2_word_r }, /* service */
	{ 0xc42002, 0xc42003, input_port_3_word_r }, /* dip1 */
	{ 0xc42000, 0xc42001, input_port_4_word_r }, /* dip2 */
	{ 0xc80000, 0xc80001, MRA16_NOP },
	{ 0xffc000, 0xffffff, SYS16_MRA16_WORKINGRAM },
MEMORY_END

static MEMORY_WRITE16_START( tetris_writemem )
	{ 0x000000, 0x01ffff, MWA16_ROM },
	{ 0x400000, 0x40ffff, SYS16_MWA16_TILERAM, &sys16_tileram },
	{ 0x410000, 0x410fff, SYS16_MWA16_TEXTRAM, &sys16_textram },
	{ 0x418000, 0x41803f, SYS16_MWA16_EXTRAM2, &sys16_extraram2 },
	{ 0x440000, 0x440fff, SYS16_MWA16_SPRITERAM, &sys16_spriteram },
	{ 0x840000, 0x840fff, SYS16_MWA16_PALETTERAM, &paletteram16 },
	{ 0xc40000, 0xc40001, sys16_coinctrl_w },
	{ 0xc42006, 0xc42007, sound_command_w },
	{ 0xc43034, 0xc43035, MWA16_NOP },
	{ 0xc80000, 0xc80001, MWA16_NOP },
	{ 0xffc000, 0xffffff, SYS16_MWA16_WORKINGRAM, &sys16_workingram },
MEMORY_END

/***************************************************************************/

static void tetris_update_proc( void ){
	sys16_fg_scrolly = sys16_textram[0x748];
	sys16_bg_scrolly = sys16_textram[0x749];
	sys16_fg_scrollx = sys16_textram[0x74c];
	sys16_bg_scrollx = sys16_textram[0x74d];

	set_fg_page( sys16_extraram2[0x38/2] );
	set_bg_page( sys16_extraram2[0x28/2] );
}

static MACHINE_INIT( tetris ){
	sys16_patch_code( 0xba6, 0x4e );
	sys16_patch_code( 0xba7, 0x71 );

	sys16_sprxoffset = -0x40;
	sys16_update_proc = tetris_update_proc;
}

/***************************************************************************/

INPUT_PORTS_START( tetris )
	SYS16_JOY1
	SYS16_JOY2
	SYS16_SERVICE
	SYS16_COINAGE /* unconfirmed */

PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Unknown ) )	/* from the code it looks like some kind of difficulty */
	PORT_DIPSETTING(    0x0c, "A" )					/* level, but all 4 levels points to the same place */
	PORT_DIPSETTING(    0x08, "B" )					/* so it doesn't actually change anything!! */
	PORT_DIPSETTING(    0x04, "C" )
	PORT_DIPSETTING(    0x00, "D" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x20, "Easy" )
	PORT_DIPSETTING(    0x30, "Normal" )
	PORT_DIPSETTING(    0x10, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

/***************************************************************************/

static MACHINE_DRIVER_START( tetris )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(tetris_readmem,tetris_writemem)

	MDRV_MACHINE_INIT(tetris)
MACHINE_DRIVER_END

/***************************************************************************/
/* sys16B */
ROM_START( timscanr )
	ROM_REGION( 0x030000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "ts10853.bin", 0x00000, 0x8000, CRC(24d7c5fb) SHA1(b1796e77cf968f9bdae7a47c5c02b93aaec10ade) )
	ROM_LOAD16_BYTE( "ts10850.bin", 0x00001, 0x8000, CRC(f1575732) SHA1(d0c8fc42171c436cc1cd55a33295bd019a474d47) )
	ROM_LOAD16_BYTE( "ts10854.bin", 0x10000, 0x8000, CRC(82d0b237) SHA1(c1defda4785014ccd7164f49f53f77e36fcb3a8d) )
	ROM_LOAD16_BYTE( "ts10851.bin", 0x10001, 0x8000, CRC(f5ce271b) SHA1(8b4f1178c87e657d8d5805d908100f0d5fb030da) )
	ROM_LOAD16_BYTE( "ts10855.bin", 0x20000, 0x8000, CRC(63e95a53) SHA1(60c42bbb1c316deb493a237990a7938551f8bc2e) )
	ROM_LOAD16_BYTE( "ts10852.bin", 0x20001, 0x8000, CRC(7cd1382b) SHA1(6263cc863cbf0ef66a7ba8cc1c98212917d7c131) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "timscanr.b9",  0x00000, 0x8000, CRC(07dccc37) SHA1(544cc6a3b3ef64727ecf5098b84ade2dd5330614) )
	ROM_LOAD( "timscanr.b10", 0x08000, 0x8000, CRC(84fb9a3a) SHA1(efde54cc9582f68e58cae05f717a4fc8f620c0fc) )
	ROM_LOAD( "timscanr.b11", 0x10000, 0x8000, CRC(c8694bc0) SHA1(e48fc349ef454ded86141937f70b006e64da6b6b) )

	ROM_REGION( 0x40000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "ts10548.bin", 0x00001, 0x8000, CRC(aa150735) SHA1(b6e6ff9229c641e196fc7a0a2cf7aa362f554676) )
	ROM_LOAD16_BYTE( "ts10552.bin", 0x00000, 0x8000, CRC(6fcbb9f7) SHA1(0a0fab930477d8b79e500263bbc80d3bf73778f8) )
	ROM_LOAD16_BYTE( "ts10549.bin", 0x10001, 0x8000, CRC(2f59f067) SHA1(1fb64cce2f98ddcb5ecb662e63ea636a8da08bcd) )
	ROM_LOAD16_BYTE( "ts10553.bin", 0x10000, 0x8000, CRC(8a220a9f) SHA1(c17547d85721fa19e5f445b5be30b3fbf5e8cc6e) )
	ROM_LOAD16_BYTE( "ts10550.bin", 0x20001, 0x8000, CRC(f05069ff) SHA1(bd95761036c2fad8ddf4e169d899b173822ee4b0) )
	ROM_LOAD16_BYTE( "ts10554.bin", 0x20000, 0x8000, CRC(dc64f809) SHA1(ea85eefa98ec55e9e872940821a959ff4eb1bd1c) )
	ROM_LOAD16_BYTE( "ts10551.bin", 0x30001, 0x8000, CRC(435d811f) SHA1(b28eb09620113cd7578387c4d96029f2acb8ec06) )
	ROM_LOAD16_BYTE( "ts10555.bin", 0x30000, 0x8000, CRC(2143c471) SHA1(d413aa216349ddf773a39d2826c3a940b4149229) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "ts10562.bin", 0x0000, 0x8000, CRC(3f5028bf) SHA1(02081569a5f4dc64771e97651a9902a98d54a952) )
	ROM_LOAD( "ts10563.bin", 0x10000, 0x8000, CRC(9db7eddf) SHA1(8b9a27442a623bee6b9b5b06275226734d132e17) )
ROM_END

/***************************************************************************/

static READ16_HANDLER( timscanr_skip_r ){
	if (activecpu_get_pc()==0x1044c) {cpu_spinuntil_int(); return 0;}
	return sys16_workingram[0x000c/2];
}

static MEMORY_READ16_START( timscanr_readmem )
	{ 0x000000, 0x02ffff, MRA16_ROM },
	{ 0x400000, 0x40ffff, SYS16_MRA16_TILERAM },
	{ 0x410000, 0x410fff, SYS16_MRA16_TEXTRAM },
	{ 0x440000, 0x440fff, SYS16_MRA16_SPRITERAM },
	{ 0x840000, 0x840fff, SYS16_MRA16_PALETTERAM },
	{ 0xc41002, 0xc41003, input_port_0_word_r }, /* player1 */
	{ 0xc41006, 0xc41007, input_port_1_word_r },/* player2 */
	{ 0xc41000, 0xc41001, input_port_2_word_r }, /* service */
	{ 0xc42002, 0xc42003, input_port_3_word_r },/* dip1 */
	{ 0xc42000, 0xc42001, input_port_4_word_r }, /* dip2 */
	{ 0xc41004, 0xc41005, input_port_5_word_r }, /* dip3 */
	{ 0xffc00c, 0xffc00d, timscanr_skip_r },
	{ 0xffc000, 0xffffff, SYS16_MRA16_WORKINGRAM },
MEMORY_END

static MEMORY_WRITE16_START( timscanr_writemem )
	{ 0x000000, 0x02ffff, MWA16_ROM },
	{ 0x400000, 0x40ffff, SYS16_MWA16_TILERAM, &sys16_tileram },
	{ 0x410000, 0x410fff, SYS16_MWA16_TEXTRAM, &sys16_textram },
	{ 0x440000, 0x440fff, SYS16_MWA16_SPRITERAM, &sys16_spriteram },
	{ 0x840000, 0x840fff, SYS16_MWA16_PALETTERAM, &paletteram16 },
	{ 0xc40000, 0xc40001, sys16_coinctrl_w },
	{ 0xfe0006, 0xfe0007, sound_command_w },
	{ 0xfe0020, 0xfe003f, MWA16_NOP },
	{ 0xffc000, 0xffffff, SYS16_MWA16_WORKINGRAM, &sys16_workingram },
MEMORY_END

/***************************************************************************/

static MACHINE_INIT( timscanr ){
	static int bank[16] = {
		0,0,0,0,
		0,0,0,3,
		0,0,0,2,
		0,1,0,0
	};
	sys16_obj_bank = bank;

	sys16_textmode=1;
	sys16_update_proc = type0_sys16_textram;
	sys16_wwfix = -1;
}

/***************************************************************************/

INPUT_PORTS_START( timscanr )
	SYS16_JOY1
	SYS16_JOY2
	SYS16_SERVICE
	SYS16_COINAGE

PORT_START	/* DSW2 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )		/*?? */
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x1e, 0x14, "Bonus" )
	PORT_DIPSETTING(    0x16, "Replay 1000000/2000000" )
	PORT_DIPSETTING(    0x14, "Replay 1200000/2500000" )
	PORT_DIPSETTING(    0x12, "Replay 1500000/3000000" )
	PORT_DIPSETTING(    0x10, "Replay 2000000/4000000" )
	PORT_DIPSETTING(    0x1c, "Replay 1000000" )
	PORT_DIPSETTING(    0x1e, "Replay 1200000" )
	PORT_DIPSETTING(    0x1a, "Replay 1500000" )
	PORT_DIPSETTING(    0x18, "Replay 1800000" )
	PORT_DIPSETTING(    0x0e, "ExtraBall 100000" )
	PORT_DIPSETTING(    0x0c, "ExtraBall 200000" )
	PORT_DIPSETTING(    0x0a, "ExtraBall 300000" )
	PORT_DIPSETTING(    0x08, "ExtraBall 400000" )
	PORT_DIPSETTING(    0x06, "ExtraBall 500000" )
	PORT_DIPSETTING(    0x04, "ExtraBall 600000" )
	PORT_DIPSETTING(    0x02, "ExtraBall 700000" )
	PORT_DIPSETTING(    0x00, "None" )

	PORT_DIPNAME( 0x20, 0x20, "Match" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x80, "3" )
	PORT_DIPSETTING(    0x00, "5" )

PORT_START	/* DSW3 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )		/*?? */
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, "1" )
	PORT_DIPSETTING(    0x0c, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x10, 0x10, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

INPUT_PORTS_END

/***************************************************************************/

static MACHINE_DRIVER_START( timscanr )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16_7759b)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(timscanr_readmem,timscanr_writemem)

	MDRV_MACHINE_INIT(timscanr)
MACHINE_DRIVER_END

/***************************************************************************/

/* sys16B */
ROM_START( toryumon )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "17689",  0x00000, 0x20000, CRC(4f0dee19) SHA1(efb5bf29e27029e9ea3fdd57ad7af54b4e1e9a97) )
	ROM_LOAD16_BYTE( "17688",  0x00001, 0x20000, CRC(717d81c7) SHA1(293cd9859a2a3aad89c47fdad2ca6aa48e9f74cb) )

	ROM_REGION( 0xc0000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "17700", 0x00000, 0x40000, CRC(8f288b37) SHA1(f0c1bb60ace8078566b51ab91fc35d2981c9f32c) )
	ROM_LOAD( "17701", 0x40000, 0x40000, CRC(6dfb025b) SHA1(502c16f650596a791fae1834f9bce6f3aa25c45f) )
	ROM_LOAD( "17702", 0x80000, 0x40000, CRC(ae0b7eab) SHA1(403ca2b50913a744e2c5e1cd0d59c69df5464836) )

	ROM_REGION( 0x100000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "17692", 0x00001, 0x20000, CRC(543c4327) SHA1(9f40163aaf165f5e4f5aefd8ce083d5f72ded125) )
	ROM_LOAD16_BYTE( "17695", 0x00000, 0x20000, CRC(ee60f244) SHA1(21b912e12f6a56ea6b9e5e0be117b447d1ecda43) )
	ROM_LOAD16_BYTE( "17693", 0x40001, 0x20000, CRC(4a350b3e) SHA1(70181bf713106475cdf684f86d593ee323ded2ca) )
	ROM_LOAD16_BYTE( "17696", 0x40000, 0x20000, CRC(6edb54f1) SHA1(8188d76db16bcf2f6bbfd0d61499e0d0a128bb27) )
	ROM_LOAD16_BYTE( "17694", 0x80001, 0x20000, CRC(b296d71d) SHA1(28f55fa451c4856f6fd6d4d9b16b4c3ef963ee5b) )
	ROM_LOAD16_BYTE( "17697", 0x80000, 0x20000, CRC(6ccb7b28) SHA1(7a82230b575bfabb150141183d3c37c44a0d41ad) )
	ROM_LOAD16_BYTE( "17698", 0xc0001, 0x20000, CRC(cd4dfb82) SHA1(def5847a94cd7da96f5434b62b22925fbb8da151) )
	ROM_LOAD16_BYTE( "17699", 0xc0000, 0x20000, CRC(2694ecce) SHA1(85d7ab89d4fd0aa0ca942485f94c5737deaa8eff) )


	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "17691", 0x00000,  0x08000, CRC(14205388) SHA1(0b580d4ef52eab2d71541d44fbd32676b2277aa1) )
	ROM_LOAD( "17690", 0x10000,  0x40000, CRC(4f9ba4e4) SHA1(450d821ef12dbb24cf85ac10c77e9b31c03112b1) )
ROM_END

/***************************************************************************/

static MEMORY_READ16_START( toryumon_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x400000, 0x40ffff, SYS16_MRA16_TILERAM },
	{ 0x410000, 0x410fff, SYS16_MRA16_TEXTRAM },
	{ 0x440000, 0x440fff, SYS16_MRA16_SPRITERAM },
	{ 0x840000, 0x840fff, SYS16_MRA16_PALETTERAM },
	{ 0xe41002, 0xe41003, input_port_0_word_r }, /* player1 */
	{ 0xe41004, 0xe41005, MRA16_NOP },
	{ 0xe41006, 0xe41007, input_port_1_word_r }, /* player2 */
	{ 0xe41000, 0xe41001, input_port_2_word_r }, /* service */
	{ 0xe42002, 0xe42003, input_port_3_word_r }, /* dip1 */
	{ 0xe42000, 0xe42001, input_port_4_word_r }, /* dip2 */
	{ 0xff0000, 0xffffff, SYS16_MRA16_WORKINGRAM },
MEMORY_END

static MEMORY_WRITE16_START( toryumon_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x3e2000, 0x3e2003, sys16_tilebank_w },
	{ 0x400000, 0x40ffff, SYS16_MWA16_TILERAM, &sys16_tileram },
	{ 0x410000, 0x410fff, SYS16_MWA16_TEXTRAM, &sys16_textram },
	{ 0x440000, 0x440fff, SYS16_MWA16_SPRITERAM, &sys16_spriteram },
	{ 0x840000, 0x840fff, SYS16_MWA16_PALETTERAM, &paletteram16 },
	{ 0xe40000, 0xe40001, sys16_coinctrl_w },
	{ 0xe43000, 0xe43fff, MWA16_NOP },
	{ 0xfe0006, 0xfe0007, sound_command_w },
	{ 0xfe0020, 0xfe003f, MWA16_NOP }, /* config regs */
	{ 0xff0000, 0xffffff, SYS16_MWA16_WORKINGRAM, &sys16_workingram },
MEMORY_END

/***************************************************************************/

static MACHINE_INIT( toryumon ){
	sys16_update_proc = type0_sys16_textram;
}

/***************************************************************************/

INPUT_PORTS_START( toryumon )
	SYS16_JOY1
	SYS16_JOY2
	SYS16_SERVICE
	SYS16_COINAGE

PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "VS-Mode Battle" )
	PORT_DIPSETTING(    0x10, "1" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0xc0, "Easy" )
	PORT_DIPSETTING(    0xe0, "Normal" )
	PORT_DIPSETTING(    0xa0, "Hard" )
	PORT_DIPSETTING(    0x80, "Hard+1" )
	PORT_DIPSETTING(    0x60, "Hard+2" )
	PORT_DIPSETTING(    0x40, "Hard+3" )
	PORT_DIPSETTING(    0x20, "Hard+4" )
	PORT_DIPSETTING(    0x00, "Hard+5" )
INPUT_PORTS_END

/***************************************************************************/

static MACHINE_DRIVER_START( toryumon )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16_7759b)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(toryumon_readmem,toryumon_writemem)

	MDRV_MACHINE_INIT(toryumon)
MACHINE_DRIVER_END

/***************************************************************************/

/* sys16B */
ROM_START( tturf )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "12327.7a",  0x00000, 0x20000, CRC(0376c593) SHA1(3057828b16ee3b7cbb57a76047eecf74d1d8b465) )
	ROM_LOAD16_BYTE( "12326.5a",  0x00001, 0x20000, CRC(f998862b) SHA1(69902ab0162eb42e1d6a9792651a5d41cb77477d) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "12268.14a", 0x00000, 0x10000, CRC(e0dac07f) SHA1(c7f6de42eb93a8f34afdc300628735b5f40a34c2) )
	ROM_LOAD( "12269.15a", 0x10000, 0x10000, CRC(457a8790) SHA1(b701e1a1745cefb31083c8a3daa3b23181f89576) )
	ROM_LOAD( "12270.16a", 0x20000, 0x10000, CRC(69fc025b) SHA1(20be1242de27f1b997d43890051cc5d5ac8a127a) )

	ROM_REGION16_BE( 0x80000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "12279.1b", 0x00001, 0x10000, CRC(7a169fb1) SHA1(1ec6da0d2cfcf727e61f61c847fd8b975b64f944) )
	ROM_LOAD16_BYTE( "12283.5b", 0x00000, 0x10000, CRC(ae0fa085) SHA1(ae9af92d4dd0c8a0f064d24e647522b588fbd7f7) )
	ROM_LOAD16_BYTE( "12278.2b", 0x20001, 0x10000, CRC(961d06b7) SHA1(b1a9dea63785bfa2c0e7b931387b91dfcd27d79b) )
	ROM_LOAD16_BYTE( "12282.6b", 0x20000, 0x10000, CRC(e8671ee1) SHA1(a3732938c370f1936d867aae9c3d1e9bbfb57ede) )
	ROM_LOAD16_BYTE( "12277.3b", 0x40001, 0x10000, CRC(f16b6ba2) SHA1(00cc04c7b5aad82d51d2d252e1e57bcdc5e2c9e3) )
	ROM_LOAD16_BYTE( "12281.7b", 0x40000, 0x10000, CRC(1ef1077f) SHA1(8ce6fd7d32a20b93b3f91aaa43fe22720da7236f) )
	ROM_LOAD16_BYTE( "12276.4b", 0x60001, 0x10000, CRC(838bd71f) SHA1(82d9d127438f5e1906b1cf40bf3b4727f2ee5685) )
	ROM_LOAD16_BYTE( "12280.8b", 0x60000, 0x10000, CRC(639a57cb) SHA1(84fd8b96758d38f9e1ba1a3c2cf8099ec0452784) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr-12328.a10", 0x00000, 0x08000, CRC(13a346de) SHA1(4e8cb12b7936c4c5d8ebc9ba563099ac2056ee60) )
	ROM_LOAD( "12329.11a",     0x10000, 0x10000, CRC(ed9a686d) SHA1(da433033d501ee871429ee676b3972b14179df9f) )		/* speech */
	/* note: it needs this hole otherwise voice samples won't playback */
	ROM_LOAD( "12330.12a",     0x30000, 0x10000, CRC(fb762bca) SHA1(ff9191c5ec38c711ebb7c2ad043f62b6d7e2203c) )

ROM_END

/* sys16B */
ROM_START( tturfu )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr12266.bin",  0x00000, 0x10000, CRC(f549def8) SHA1(9e913509d46274bd192455321904ec9884b5f629) )
	ROM_LOAD16_BYTE( "epr12264.bin",  0x00001, 0x10000, CRC(f7cdb289) SHA1(9c386cf33a96a977c623c2f243db38fda2c41ba4) )
	ROM_LOAD16_BYTE( "epr12267.bin",  0x20000, 0x10000, CRC(3c3ce191) SHA1(036ece2be585a85e05c047fe4a1ab3b6814c3490) )
	ROM_LOAD16_BYTE( "epr12265.bin",  0x20001, 0x10000, CRC(8cdadd9a) SHA1(a17852f0eb8f63a82ff44cbeb100da72fe5890e0) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "12268.14a", 0x00000, 0x10000, CRC(e0dac07f) SHA1(c7f6de42eb93a8f34afdc300628735b5f40a34c2) )
	ROM_LOAD( "12269.15a", 0x10000, 0x10000, CRC(457a8790) SHA1(b701e1a1745cefb31083c8a3daa3b23181f89576) )
	ROM_LOAD( "12270.16a", 0x20000, 0x10000, CRC(69fc025b) SHA1(20be1242de27f1b997d43890051cc5d5ac8a127a) )

	ROM_REGION16_BE( 0x80000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "12279.1b", 0x00001, 0x10000, CRC(7a169fb1) SHA1(1ec6da0d2cfcf727e61f61c847fd8b975b64f944) )
	ROM_LOAD16_BYTE( "12283.5b", 0x00000, 0x10000, CRC(ae0fa085) SHA1(ae9af92d4dd0c8a0f064d24e647522b588fbd7f7) )
	ROM_LOAD16_BYTE( "12278.2b", 0x20001, 0x10000, CRC(961d06b7) SHA1(b1a9dea63785bfa2c0e7b931387b91dfcd27d79b) )
	ROM_LOAD16_BYTE( "12282.6b", 0x20000, 0x10000, CRC(e8671ee1) SHA1(a3732938c370f1936d867aae9c3d1e9bbfb57ede) )
	ROM_LOAD16_BYTE( "12277.3b", 0x40001, 0x10000, CRC(f16b6ba2) SHA1(00cc04c7b5aad82d51d2d252e1e57bcdc5e2c9e3) )
	ROM_LOAD16_BYTE( "12281.7b", 0x40000, 0x10000, CRC(1ef1077f) SHA1(8ce6fd7d32a20b93b3f91aaa43fe22720da7236f) )
	ROM_LOAD16_BYTE( "12276.4b", 0x60001, 0x10000, CRC(838bd71f) SHA1(82d9d127438f5e1906b1cf40bf3b4727f2ee5685) )
	ROM_LOAD16_BYTE( "12280.8b", 0x60000, 0x10000, CRC(639a57cb) SHA1(84fd8b96758d38f9e1ba1a3c2cf8099ec0452784) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr12271.bin", 0x00000,  0x8000, CRC(99671e52) SHA1(ab21b7e7a449c8081b5db1fdb579071e31b6852c) )
	ROM_LOAD( "epr12272.bin", 0x10000, 0x8000, CRC(7cf7e69f) SHA1(62c9f75db8e2b4ece517167ba5c0712bac5f1d95) )
	ROM_LOAD( "epr12273.bin", 0x18000, 0x8000, CRC(28f0bb8b) SHA1(d99aff57d213b10ac10c37ceff2f125875816488) )
	ROM_LOAD( "epr12274.bin", 0x20000, 0x8000, CRC(8207f0c4) SHA1(169914861a52fa731a305e1ee2d230aa0d0d97fe) )
	ROM_LOAD( "epr12275.bin", 0x28000, 0x8000, CRC(182f3c3d) SHA1(1482fe08a05a721e315b1a3aa5bef4dddc72e26e) )

ROM_END

/***************************************************************************/
static READ16_HANDLER( tt_io_player1_r ){ return input_port_0_r( offset ) << 8; }
static READ16_HANDLER( tt_io_player2_r ){ return input_port_1_r( offset ) << 8; }
static READ16_HANDLER( tt_io_service_r ){ return input_port_2_r( offset ) << 8; }

static MEMORY_READ16_START( tturf_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x2001e6, 0x2001e7, tt_io_service_r },
	{ 0x2001e8, 0x2001e9, tt_io_player1_r },
	{ 0x2001ea, 0x2001eb, tt_io_player2_r },
	{ 0x200000, 0x203fff, SYS16_MRA16_EXTRAM },
	{ 0x300000, 0x300fff, SYS16_MRA16_SPRITERAM },
	{ 0x400000, 0x40ffff, SYS16_MRA16_TILERAM },
	{ 0x410000, 0x410fff, SYS16_MRA16_TEXTRAM },
	{ 0x500000, 0x500fff, SYS16_MRA16_PALETTERAM },
	{ 0x602002, 0x602003, input_port_3_word_r }, /* dip1 */
	{ 0x602000, 0x602001, input_port_4_word_r }, /* dip2 */
MEMORY_END

static MEMORY_WRITE16_START( tturf_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x200000, 0x203fff, SYS16_MWA16_EXTRAM, &sys16_extraram },
	{ 0x300000, 0x300fff, SYS16_MWA16_SPRITERAM, &sys16_spriteram },
	{ 0x400000, 0x40ffff, SYS16_MWA16_TILERAM, &sys16_tileram },
	{ 0x410000, 0x410fff, SYS16_MWA16_TEXTRAM, &sys16_textram },
	{ 0x500000, 0x500fff, SYS16_MWA16_PALETTERAM, &paletteram16 },
	{ 0x600000, 0x600001, sys16_coinctrl_w },
	{ 0xff0020, 0xff003f, MWA16_NOP }, /* config regs */
MEMORY_END

/***************************************************************************/

/*
	This game has a MCU which does the following:
	- Get Z80 sound command out of work RAM and write to Z80 sound command register
	- Read input ports and store to work RAM

	The routine which stores the sound code in RAM looks like this:

	; D0 = sound command
	movem.l    d0-d1/a0, -(a7)
	lea        $2001d6, a0         ; base of 16-byte circular buffer
	move.w     $2001d4, d1         ; get buffer index
	move.b     d0, (a0, d1.w)      ; write sound command to buffer
	addq.w     #1, d1              ; next buffer index
	andi.w     #$000f, d1          ; wrap buffer index
	move.w     d1, $2001d4         ; save buffer index
	addq.w     #1, $2001d2         ; bump 'sound code written' flag
	movem.l    (a7)+, d0-d1/a0
	rts

	Most likely the MCU reads $2001D2 and copies the sound byte from $2001D6+$2001D4 to the sound command register.
	In tturfbl, a JSR is inserted over the first LEA instruction to a subroutine which copies D0 to the sound command
	register at $600007, and restores a0 to $2001D6 before returning.

	If the circular buffer is to prioritize sound requests, then this effect is lost in tturfbl. If it's just to
	be tricky, tturfbl handles it correctly.
*/

static WRITE16_HANDLER( tturfu_mcu_sound_trigger_w )
{
	COMBINE_DATA(&sys16_extraram[offset]);

	if(activecpu_get_pc() == 0x100E)
	{
		int code;

		if(ACCESSING_LSB)
			code = (data >> 0) & 0xFF;
		else
			code = (data >> 8) & 0xFF;

		soundlatch_w(0, code);
		cpu_set_irq_line(1, 0, HOLD_LINE);
	}
}
static WRITE16_HANDLER( tturf_mcu_sound_trigger_w )
{
	COMBINE_DATA(&sys16_extraram[offset]);

	if(activecpu_get_pc() == 0x104c)
	{
		int code;

		if(ACCESSING_LSB)
			code = (data >> 0) & 0xFF;
		else
			code = (data >> 8) & 0xFF;

		soundlatch_w(0, code);
		cpu_set_irq_line(1, 0, HOLD_LINE);
	}
}


static MACHINE_INIT( tturf ){
	static int bank[16] = { 0,0,1,0,2,0,3,0 };
	sys16_obj_bank = bank;
	sys16_sprite_draw = 1;
	sys16_update_proc = type0_sys16_textram;

	install_mem_write16_handler(0, 0x2001d6, 0x2001e5, tturf_mcu_sound_trigger_w );

}

static MACHINE_INIT( tturfu ){
	static int bank[16] = {
		0,0,0,0,
		0,0,0,0,
		0,0,0,1,
		0,2,3,0
	};
	sys16_obj_bank = bank;
	sys16_sprite_draw = 1;
	sys16_update_proc = type0_sys16_textram;
	install_mem_write16_handler(0, 0x2001d6, 0x2001e5, tturfu_mcu_sound_trigger_w );
}

/***************************************************************************/

INPUT_PORTS_START( tturf )
	SYS16_JOY1
	SYS16_JOY2
	SYS16_SERVICE
	SYS16_COINAGE

PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x00, "Continues" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "Unlimited" )
	PORT_DIPSETTING(    0x03, "Unlimited" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, "Easy" )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x04, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x30, 0x20, "Starting Energy" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x20, "6" )
	PORT_DIPSETTING(    0x30, "8" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "Bonus Energy" )
	PORT_DIPSETTING(    0x80, "1" )
	PORT_DIPSETTING(    0x00, "2" )
INPUT_PORTS_END

/***************************************************************************/

static MACHINE_DRIVER_START( tturf )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16_7759b)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(tturf_readmem,tturf_writemem)

	MDRV_MACHINE_INIT(tturf)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( tturfu )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(tturf)
	MDRV_MACHINE_INIT(tturfu)
MACHINE_DRIVER_END

/***************************************************************************/
/* sys16B */
ROM_START( tturfbl )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "tt042197.rom", 0x00000, 0x10000, CRC(deee5af1) SHA1(0caba775021dc7e28ac6b7af8eac4f49d3102c83) )
	ROM_LOAD16_BYTE( "tt06c794.rom", 0x00001, 0x10000, CRC(90e6a95a) SHA1(014a0ae5cebcba9cc99e6ccde4ad5d938fab915c) )
	ROM_LOAD16_BYTE( "tt030be3.rom", 0x20000, 0x10000, CRC(100264a2) SHA1(d1ea4bf93f5472901ce95200f546ce9b58936aea) )
	ROM_LOAD16_BYTE( "tt05ef8a.rom", 0x20001, 0x10000, CRC(f787a948) SHA1(512b8cb2f5e9795171951e02c07cae957db41334) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "tt1574b3.rom", 0x00000, 0x10000, CRC(e9e630da) SHA1(e8471dedbb25475e4814d78b56f579fe9110461e) )
	ROM_LOAD( "tt16cf44.rom", 0x10000, 0x10000, CRC(4c467735) SHA1(8338b6605cbe2e076da0b3e3a47630409a79f002) )
	ROM_LOAD( "tt17d59e.rom", 0x20000, 0x10000, CRC(60c0f2fe) SHA1(3fea4ed757d47628f59ff940e40cb86b3b5b443b) )

	ROM_REGION16_BE( 0x80000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "12279.1b", 0x00001, 0x10000, CRC(7a169fb1) SHA1(1ec6da0d2cfcf727e61f61c847fd8b975b64f944) )
	ROM_LOAD16_BYTE( "12283.5b", 0x00000, 0x10000, CRC(ae0fa085) SHA1(ae9af92d4dd0c8a0f064d24e647522b588fbd7f7) )
	ROM_LOAD16_BYTE( "12278.2b", 0x20001, 0x10000, CRC(961d06b7) SHA1(b1a9dea63785bfa2c0e7b931387b91dfcd27d79b) )
	ROM_LOAD16_BYTE( "12282.6b", 0x20000, 0x10000, CRC(e8671ee1) SHA1(a3732938c370f1936d867aae9c3d1e9bbfb57ede) )
	ROM_LOAD16_BYTE( "12277.3b", 0x40001, 0x10000, CRC(f16b6ba2) SHA1(00cc04c7b5aad82d51d2d252e1e57bcdc5e2c9e3) )
	ROM_LOAD16_BYTE( "12281.7b", 0x40000, 0x10000, CRC(1ef1077f) SHA1(8ce6fd7d32a20b93b3f91aaa43fe22720da7236f) )
	ROM_LOAD16_BYTE( "12276.4b", 0x60001, 0x10000, CRC(838bd71f) SHA1(82d9d127438f5e1906b1cf40bf3b4727f2ee5685) )
	ROM_LOAD16_BYTE( "12280.8b", 0x60000, 0x10000, CRC(639a57cb) SHA1(84fd8b96758d38f9e1ba1a3c2cf8099ec0452784) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "tt014d68.rom", 0x10000, 0x10000, CRC(d4aab1d9) SHA1(94885896d59da1ecabe2377a194fcf61eaae3765) )
	ROM_LOAD( "tt0246ff.rom", 0x20000, 0x10000, CRC(bb4bba8f) SHA1(b182a7e1d0425e93c2c1b93472aafd30a6af6907) )
ROM_END

/***************************************************************************/

static MEMORY_READ16_START( tturfbl_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x2001e6, 0x2001e7, tt_io_service_r },
	{ 0x2001e8, 0x2001e9, tt_io_player1_r },
	{ 0x2001ea, 0x2001eb, tt_io_player2_r },
	{ 0x200000, 0x203fff, SYS16_MRA16_EXTRAM },
	{ 0x300000, 0x300fff, SYS16_MRA16_SPRITERAM },
	{ 0x400000, 0x40ffff, SYS16_MRA16_TILERAM },
	{ 0x410000, 0x410fff, SYS16_MRA16_TEXTRAM },
	{ 0x500000, 0x500fff, SYS16_MRA16_PALETTERAM },
	{ 0x600002, 0x600003, input_port_3_word_r }, /* dip1 */
	{ 0x600000, 0x600001, input_port_4_word_r }, /* dip2 */
	{ 0x601002, 0x601003, input_port_0_word_r }, /* player1 */
	{ 0x601004, 0x601005, input_port_1_word_r }, /* player2 */
	{ 0x601000, 0x601001, input_port_2_word_r }, /* service */
	{ 0x602002, 0x602003, input_port_3_word_r }, /* dip1 */
	{ 0x602000, 0x602001, input_port_4_word_r }, /* dip2 */
	{ 0xc46000, 0xc4601f, SYS16_MRA16_EXTRAM3 },
MEMORY_END

static MEMORY_WRITE16_START( tturfbl_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x200000, 0x203fff, SYS16_MWA16_EXTRAM, &sys16_extraram },
	{ 0x300000, 0x300fff, SYS16_MWA16_SPRITERAM, &sys16_spriteram },
	{ 0x400000, 0x40ffff, SYS16_MWA16_TILERAM, &sys16_tileram },
	{ 0x410000, 0x410fff, SYS16_MWA16_TEXTRAM, &sys16_textram },
	{ 0x500000, 0x500fff, SYS16_MWA16_PALETTERAM, &paletteram16 },
	{ 0x600000, 0x600001, sys16_coinctrl_w },
	{ 0x600006, 0x600007, sound_command_w },
	{ 0xc44000, 0xc44001, MWA16_NOP },
	{ 0xc46000, 0xc4601f, SYS16_MWA16_EXTRAM3, &sys16_extraram3 },
	{ 0xff0020, 0xff003f, MWA16_NOP }, /* config regs */
MEMORY_END

/***************************************************************************/

static void tturfbl_update_proc( void ){
	sys16_fg_scrollx = sys16_textram[0x74c] & 0x01ff;
	sys16_bg_scrollx = sys16_textram[0x74d] & 0x01ff;
	sys16_fg_scrolly = sys16_textram[0x748];
	sys16_bg_scrolly = sys16_textram[0x749];


	{
		int data1,data2;

		data1 = sys16_textram[0x740];
		data2 = sys16_textram[0x741];

		sys16_fg_page[3] = data1>>12;
		sys16_bg_page[3] = (data1>>8)&0xf;
		sys16_fg_page[1] = (data1>>4)&0xf;
		sys16_bg_page[1] = data1&0xf;

		sys16_fg_page[2] = data2>>12;
		sys16_bg_page[2] = (data2>>8)&0xf;
		sys16_fg_page[0] = (data2>>4)&0xf;
		sys16_bg_page[0] = data2&0xf;
	}
}

static MACHINE_INIT( tturfbl ){
	static int bank[16] = {
		0,0,0,0,
		0,0,0,3,
		0,0,0,2,
		0,1,0,0
	};
	sys16_obj_bank = bank;
	sys16_sprite_draw = 1;
	sys16_sprxoffset = -0x48;

	sys16_update_proc = tturfbl_update_proc;
}

static DRIVER_INIT( tturfbl )
{
	UINT8 *mem;
	int i;

	/* invert the graphics bits on the tiles */
	for (i = 0; i < 0x30000; i++)
		memory_region(REGION_GFX1)[i] ^= 0xff;

	mem = memory_region(REGION_CPU2);
	memcpy(mem, mem+0x10000, 0x8000);

}
/***************************************************************************/
/* sound ?? */
static MACHINE_DRIVER_START( tturfbl )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16_7759b)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(tturfbl_readmem,tturfbl_writemem)

	MDRV_CPU_MODIFY("sound")
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(tturfbl_sound_readmem,tturfbl_sound_writemem)
	MDRV_CPU_PORTS(tturfbl_sound_readport,tturfbl_sound_writeport)

	MDRV_SOUND_REMOVE("7759")
	MDRV_SOUND_ADD_TAG("5205", MSM5205, tturfbl_msm5205_interface)

	MDRV_MACHINE_INIT(tturfbl)
MACHINE_DRIVER_END


/***************************************************************************/
/* sys16B */
ROM_START( wb3 )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr12259.a7", 0x000000, 0x20000, CRC(54927c7e) SHA1(09a4c25b40aba2056c79b5c2e6e8cb7e6c05bc16) )
	ROM_LOAD16_BYTE( "epr12258.a5", 0x000001, 0x20000, CRC(01f5898c) SHA1(2422b4199ce5b63482f7fa1c790c90fc70a2b872) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "epr12124.a14", 0x00000, 0x10000, CRC(dacefb6f) SHA1(789a5a99ad9419aee9da5397bcea34452ea8b4b3) )
	ROM_LOAD( "epr12125.a15", 0x10000, 0x10000, CRC(9fc36df7) SHA1(b39ccc687489e9781181197505fc78aa5cf7ea55) )
	ROM_LOAD( "epr12126.a16", 0x20000, 0x10000, CRC(a693fd94) SHA1(38e5446f41b6793a8e4134fdd92b02b86e3589f7) )

	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "epr12093.b4", 0x00001, 0x010000, CRC(4891e7bb) SHA1(1be04fcabe9bfa8cf746263a5bcca67902a021a0) )
	ROM_LOAD16_BYTE( "epr12097.b8", 0x00000, 0x010000, CRC(e645902c) SHA1(497cfcf6c25cc2e042e16dbcb1963d2223def15a) )
	ROM_LOAD16_BYTE( "epr12091.b2", 0x20001, 0x010000, CRC(8409a243) SHA1(bcbb9510a6499d8147543d6befa5a49f4ac055d9) )
	ROM_LOAD16_BYTE( "epr12095.b6", 0x20000, 0x010000, CRC(e774ec2c) SHA1(a4aa15ec7be5539a740ad02ff720458018dbc536) )
	ROM_LOAD16_BYTE( "epr12090.b1", 0x40001, 0x010000, CRC(aeeecfca) SHA1(496124b170a725ad863c741d4e021ab947511e4c) )
	ROM_LOAD16_BYTE( "epr12094.b5", 0x40000, 0x010000, CRC(615e4927) SHA1(d23f164973afa770714e284a77ddf10f18cc596b) )
	ROM_LOAD16_BYTE( "epr12092.b3", 0x60001, 0x010000, CRC(5c2f0d90) SHA1(e0fbc0f841e4607ad232931368b16e81440a75c4) )
	ROM_LOAD16_BYTE( "epr12096.b7", 0x60000, 0x010000, CRC(0cd59d6e) SHA1(caf754a461feffafcfe7bfc6e89da76c4db257c5) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr12127.a10", 0x0000, 0x8000, CRC(0bb901bb) SHA1(c81b198df8e3b0ec568032c76addf0d1a1711194) )
ROM_END

ROM_START( wb3a )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 code */
/* Custom CPU 317-0089 */
	ROM_LOAD16_BYTE( "epr12137.a7", 0x000000, 0x20000, CRC(6f81238e) SHA1(b578165c1624f8a112e9eea098fb4551cc38faa1) )
	ROM_LOAD16_BYTE( "epr12136.a5", 0x000001, 0x20000, CRC(4cf05003) SHA1(bd4c64c327e53143aa94062f91946eda0a7146c2) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "epr12124.a14", 0x00000, 0x10000, CRC(dacefb6f) SHA1(789a5a99ad9419aee9da5397bcea34452ea8b4b3) )
	ROM_LOAD( "epr12125.a15", 0x10000, 0x10000, CRC(9fc36df7) SHA1(b39ccc687489e9781181197505fc78aa5cf7ea55) )
	ROM_LOAD( "epr12126.a16", 0x20000, 0x10000, CRC(a693fd94) SHA1(38e5446f41b6793a8e4134fdd92b02b86e3589f7) )

	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "epr12093.b4", 0x00001, 0x010000, CRC(4891e7bb) SHA1(1be04fcabe9bfa8cf746263a5bcca67902a021a0) )
	ROM_LOAD16_BYTE( "epr12097.b8", 0x00000, 0x010000, CRC(e645902c) SHA1(497cfcf6c25cc2e042e16dbcb1963d2223def15a) )
	ROM_LOAD16_BYTE( "epr12091.b2", 0x20001, 0x010000, CRC(8409a243) SHA1(bcbb9510a6499d8147543d6befa5a49f4ac055d9) )
	ROM_LOAD16_BYTE( "epr12095.b6", 0x20000, 0x010000, CRC(e774ec2c) SHA1(a4aa15ec7be5539a740ad02ff720458018dbc536) )
	ROM_LOAD16_BYTE( "epr12090.b1", 0x40001, 0x010000, CRC(aeeecfca) SHA1(496124b170a725ad863c741d4e021ab947511e4c) )
	ROM_LOAD16_BYTE( "epr12094.b5", 0x40000, 0x010000, CRC(615e4927) SHA1(d23f164973afa770714e284a77ddf10f18cc596b) )
	ROM_LOAD16_BYTE( "epr12092.b3", 0x60001, 0x010000, CRC(5c2f0d90) SHA1(e0fbc0f841e4607ad232931368b16e81440a75c4) )
	ROM_LOAD16_BYTE( "epr12096.b7", 0x60000, 0x010000, CRC(0cd59d6e) SHA1(caf754a461feffafcfe7bfc6e89da76c4db257c5) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr12127.a10", 0x0000, 0x8000, CRC(0bb901bb) SHA1(c81b198df8e3b0ec568032c76addf0d1a1711194) )
ROM_END

/***************************************************************************/

static MEMORY_READ16_START( wb3_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x400000, 0x40ffff, SYS16_MRA16_TILERAM },
	{ 0x410000, 0x410fff, SYS16_MRA16_TEXTRAM },
	{ 0x440000, 0x440fff, SYS16_MRA16_SPRITERAM },
	{ 0x840000, 0x840fff, SYS16_MRA16_PALETTERAM },
	{ 0xc41002, 0xc41003, input_port_0_word_r }, /* player1 */
	{ 0xc41006, 0xc41007, input_port_1_word_r }, /* player2 */
	{ 0xc41000, 0xc41001, input_port_2_word_r }, /* service */
	{ 0xc42002, 0xc42003, input_port_3_word_r }, /* dip1 */
	{ 0xc42000, 0xc42001, input_port_4_word_r }, /* dip2 */
	{ 0xffc000, 0xffffff, SYS16_MRA16_WORKINGRAM },
MEMORY_END

static WRITE16_HANDLER( wb3_sound_command_w ){
	if( ACCESSING_MSB ) sound_command_w(offset,data>>8,0xff00);
}

static MEMORY_WRITE16_START( wb3_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x3f0000, 0x3fffff, sys16_tilebank_w },
	{ 0x400000, 0x40ffff, SYS16_MWA16_TILERAM, &sys16_tileram },
	{ 0x410000, 0x410fff, SYS16_MWA16_TEXTRAM, &sys16_textram },
	{ 0x440000, 0x440fff, SYS16_MWA16_SPRITERAM, &sys16_spriteram },
	{ 0x840000, 0x840fff, SYS16_MWA16_PALETTERAM, &paletteram16 },
	{ 0xc40000, 0xc40001, sys16_coinctrl_w },
	{ 0xffc008, 0xffc009, wb3_sound_command_w },
	{ 0xffc000, 0xffffff, SYS16_MWA16_WORKINGRAM, &sys16_workingram },
MEMORY_END

/***************************************************************************/

static MACHINE_INIT( wb3 ){
	static int bank[16] = {
		2,0,
		1,0,
		3,0,
		0,3,
		0,0,
		0,2,
		0,1,
		0,0
	};
	sys16_obj_bank = bank;
	sys16_update_proc = type0_sys16_textram;
}

/***************************************************************************/

INPUT_PORTS_START( wb3 )
	SYS16_JOY1
	SYS16_JOY2
	SYS16_SERVICE
	SYS16_COINAGE

PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Bonus_Life ) )		/*?? */
	PORT_DIPSETTING(    0x10, "5000/10000/18000/30000" )
	PORT_DIPSETTING(    0x00, "5000/15000/30000" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Allow Round Select" )
	PORT_DIPSETTING(    0x40, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )			/* no collision though */
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

/***************************************************************************/

static MACHINE_DRIVER_START( wb3 )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(wb3_readmem,wb3_writemem)

	MDRV_MACHINE_INIT(wb3)
MACHINE_DRIVER_END

/***************************************************************************/
/* sys16B */
ROM_START( wb3bl )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "wb3_03", 0x000000, 0x10000, CRC(0019ab3b) SHA1(89d49a437690fa6e0c35bb9f1450042f89504714) )
	ROM_LOAD16_BYTE( "wb3_05", 0x000001, 0x10000, CRC(196e17ee) SHA1(71e4345b2c3d1612a3d424c9310fad1e23c8a9f7) )
	ROM_LOAD16_BYTE( "wb3_02", 0x020000, 0x10000, CRC(c87350cb) SHA1(55a8cb68d70b6060dd9a55e281e216ce3917ea5b) )
	ROM_LOAD16_BYTE( "wb3_04", 0x020001, 0x10000, CRC(565d5035) SHA1(e28a132f1a4ce9466945e231c54502178748af98) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "wb3_14", 0x00000, 0x10000, CRC(d3f20bca) SHA1(0a87f709f8e2a913473512ede408e2cbc535443f) )
	ROM_LOAD( "wb3_15", 0x10000, 0x10000, CRC(96ff9d52) SHA1(791a9da4860e0d42fba98f80a3c6725ad8c73e33) )
	ROM_LOAD( "wb3_16", 0x20000, 0x10000, CRC(afaf0d31) SHA1(d4309329a0a543250788146b63b27ff058c02fc3) )

	ROM_REGION( 0x080000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "epr12093.b4", 0x000001, 0x010000, CRC(4891e7bb) SHA1(1be04fcabe9bfa8cf746263a5bcca67902a021a0) )
	ROM_LOAD16_BYTE( "epr12097.b8", 0x000000, 0x010000, CRC(e645902c) SHA1(497cfcf6c25cc2e042e16dbcb1963d2223def15a) )
	ROM_LOAD16_BYTE( "epr12091.b2", 0x020001, 0x010000, CRC(8409a243) SHA1(bcbb9510a6499d8147543d6befa5a49f4ac055d9) )
	ROM_LOAD16_BYTE( "epr12095.b6", 0x020000, 0x010000, CRC(e774ec2c) SHA1(a4aa15ec7be5539a740ad02ff720458018dbc536) )
	ROM_LOAD16_BYTE( "epr12090.b1", 0x040001, 0x010000, CRC(aeeecfca) SHA1(496124b170a725ad863c741d4e021ab947511e4c) )
	ROM_LOAD16_BYTE( "epr12094.b5", 0x040000, 0x010000, CRC(615e4927) SHA1(d23f164973afa770714e284a77ddf10f18cc596b) )
	ROM_LOAD16_BYTE( "epr12092.b3", 0x060001, 0x010000, CRC(5c2f0d90) SHA1(e0fbc0f841e4607ad232931368b16e81440a75c4) )
	ROM_LOAD16_BYTE( "epr12096.b7", 0x060000, 0x010000, CRC(0cd59d6e) SHA1(caf754a461feffafcfe7bfc6e89da76c4db257c5) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr12127.a10", 0x0000, 0x8000, CRC(0bb901bb) SHA1(c81b198df8e3b0ec568032c76addf0d1a1711194) )
ROM_END

/***************************************************************************/

static MEMORY_READ16_START( wb3bl_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x400000, 0x40ffff, SYS16_MRA16_TILERAM },
	{ 0x410000, 0x410fff, SYS16_MRA16_TEXTRAM },
	{ 0x440000, 0x440fff, SYS16_MRA16_SPRITERAM },
	{ 0x840000, 0x840fff, SYS16_MRA16_PALETTERAM },
	{ 0xc41002, 0xc41003, input_port_0_word_r }, /* player1 */
	{ 0xc41004, 0xc41005, input_port_1_word_r }, /* player2 */
	{ 0xc41000, 0xc41001, input_port_2_word_r }, /* service */
	{ 0xc42002, 0xc42003, input_port_3_word_r }, /* dip1 */
	{ 0xc42000, 0xc42001, input_port_4_word_r }, /* dip2 */
	{ 0xc46000, 0xc4601f, SYS16_MRA16_EXTRAM3 },
	{ 0xff0000, 0xffffff, SYS16_MRA16_WORKINGRAM },
MEMORY_END

static MEMORY_WRITE16_START( wb3bl_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x3f0000, 0x3fffff, sys16_tilebank_w },
	{ 0x400000, 0x40ffff, SYS16_MWA16_TILERAM, &sys16_tileram },
	{ 0x410000, 0x410fff, SYS16_MWA16_TEXTRAM, &sys16_textram },
	{ 0x440000, 0x440fff, SYS16_MWA16_SPRITERAM, &sys16_spriteram },
	{ 0x840000, 0x840fff, SYS16_MWA16_PALETTERAM, &paletteram16 },
	{ 0xc40000, 0xc40001, sys16_coinctrl_w },
	{ 0xc42006, 0xc42007, sound_command_w },
	{ 0xc44000, 0xc44001, MWA16_NOP },
	{ 0xc46000, 0xc4601f, SYS16_MWA16_EXTRAM3, &sys16_extraram3 },
	{ 0xff0000, 0xffffff, SYS16_MWA16_WORKINGRAM, &sys16_workingram },
MEMORY_END

/***************************************************************************/

static void wb3bl_update_proc( void ){

	sys16_fg_scrollx = sys16_workingram[0xc030/2];
	sys16_bg_scrollx = sys16_workingram[0xc038/2];
	sys16_fg_scrolly = sys16_workingram[0xc032/2];
	sys16_bg_scrolly = sys16_workingram[0xc03c/2];

	set_fg_page( sys16_textram[0x0ff6/2] );
	set_bg_page( sys16_textram[0x0ff4/2] );
}

static MACHINE_INIT( wb3bl ){
	static int bank[16] = {
		2,0,
		1,0,
		3,0,
		0,3,
		0,0,
		0,2,
		0,1,
		0,0
	};
	sys16_obj_bank = bank;
#if 1
	sys16_patch_code( 0x17058, 0x4e );
	sys16_patch_code( 0x17059, 0xb9 );
	sys16_patch_code( 0x1705a, 0x00 );
	sys16_patch_code( 0x1705b, 0x00 );
	sys16_patch_code( 0x1705c, 0x09 );
	sys16_patch_code( 0x1705d, 0xdc );
	sys16_patch_code( 0x1705e, 0x4e );
	sys16_patch_code( 0x1705f, 0xf9 );
	sys16_patch_code( 0x17060, 0x00 );
	sys16_patch_code( 0x17061, 0x01 );
	sys16_patch_code( 0x17062, 0x70 );
	sys16_patch_code( 0x17063, 0xe0 );
	sys16_patch_code( 0x1a3a, 0x31 );
	sys16_patch_code( 0x1a3b, 0x7c );
	sys16_patch_code( 0x1a3c, 0x80 );
	sys16_patch_code( 0x1a3d, 0x00 );
	sys16_patch_code( 0x23df8, 0x14 );
	sys16_patch_code( 0x23df9, 0x41 );
	sys16_patch_code( 0x23dfa, 0x10 );
	sys16_patch_code( 0x23dfd, 0x14 );
	sys16_patch_code( 0x23dff, 0x1c );
#endif
	sys16_update_proc = wb3bl_update_proc;
}

static DRIVER_INIT( wb3bl )
{
	int i;

	/* invert the graphics bits on the tiles */
	for (i = 0; i < 0x30000; i++)
		memory_region(REGION_GFX1)[i] ^= 0xff;
}

/***************************************************************************/

static MACHINE_DRIVER_START( wb3bl )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(wb3bl_readmem,wb3bl_writemem)

	MDRV_MACHINE_INIT(wb3bl)
MACHINE_DRIVER_END

/***************************************************************************/
/* sys16B */
ROM_START( wrestwar )
	ROM_REGION( 0xc0000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "ww.a7", 0x00000, 0x20000, CRC(eeaba126) SHA1(ca1f630ff0cfc301205d2b8226d7614eadc117b7) )
	ROM_LOAD16_BYTE( "ww.a5", 0x00001, 0x20000, CRC(6714600a) SHA1(8b04347f39bb8a8bc52b1bbfa367d42ea7c61bc9) )
	/* empty 0x40000 - 0x80000 */
	ROM_LOAD16_BYTE( "ww.a8", 0x80000, 0x20000, CRC(b77ba665) SHA1(b6a01ca857b5127ebb763f18cd4123185a7765a6) )
	ROM_LOAD16_BYTE( "ww.a6", 0x80001, 0x20000, CRC(ddf075cb) SHA1(5d887f0d5786fa62757c593d937bba6f150c1b12) )

	ROM_REGION( 0x60000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "ww.a14", 0x00000, 0x20000, CRC(6a821ab9) SHA1(e69f7e534835d4c820746ffc3ad76c3b7bb9b02e) )
	ROM_LOAD( "ww.a15", 0x20000, 0x20000, CRC(2b1a0751) SHA1(8cb1027ef3728f5bdfdb5e2df0f0421f743cdc0a) )
	ROM_LOAD( "ww.a16", 0x40000, 0x20000, CRC(f6e190fe) SHA1(4c8b334fb22c449d8d00c8f49f5eccbe008e244f) )

	ROM_REGION16_BE( 0x180000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "ww.b1",  0x000001, 0x20000, CRC(ffa7d368) SHA1(e5663ef1cbe8ab27be0919a3cd78d9a7747bbac6) )
	ROM_LOAD16_BYTE( "ww.b5",  0x000000, 0x20000, CRC(8d7794c1) SHA1(ace87970cfa02ab8200173622633d0d70ef7aa9e) )
	ROM_LOAD16_BYTE( "ww.b2",  0x040001, 0x20000, CRC(0ed343f2) SHA1(951bd616e63c5fe0aa3f387c9c12153b4f29675f) )
	ROM_LOAD16_BYTE( "ww.b6",  0x040000, 0x20000, CRC(99458d58) SHA1(87020267610c5784e066bb4e3551852d27b4cf61) )
	ROM_LOAD16_BYTE( "ww.b3",  0x080001, 0x20000, CRC(3087104d) SHA1(9ad0ea2b580820443c576cbb95d57467e32ea7b5) )
	ROM_LOAD16_BYTE( "ww.b7",  0x080000, 0x20000, CRC(abcf9bed) SHA1(4f755cfd0304e877b798c31de50e15995f8c4edf) )
	ROM_LOAD16_BYTE( "ww.b4",  0x0c0001, 0x20000, CRC(41b6068b) SHA1(c1f1f51c4e0f4320cef7821bccac5b0c9e915d9b) )
	ROM_LOAD16_BYTE( "ww.b8",  0x0c0000, 0x20000, CRC(97eac164) SHA1(2bb62e6d8b2d662e9b31aa8d238a51af7c7905e8) )
	ROM_LOAD16_BYTE( "ww.a1",  0x100001, 0x20000, CRC(260311c5) SHA1(6b52b671252aef992c0546468c44b722bdb6a649) )
	ROM_LOAD16_BYTE( "ww.b10", 0x100000, 0x20000, CRC(35a4b1b1) SHA1(c3b8ba708f9f2822e48e52ea74d7e96f08182ac4) )
	ROM_LOAD16_BYTE( "ww.a2",  0x140001, 0x10000, CRC(12e38a5c) SHA1(05558a370b4e8100d2fa5e700a5ab76771ff7729) )
	ROM_LOAD16_BYTE( "ww.b11", 0x140000, 0x10000, CRC(fa06fd24) SHA1(20a578b82a75fe96a230c91645108fdc8b5bae21) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "ww.a10", 0x00000, 0x08000, CRC(c3609607) SHA1(2e0acb775c60851bf0b2037b91b07ead061d5862) )
	ROM_LOAD( "ww.a11", 0x10000, 0x20000, CRC(fb9a7f29) SHA1(7ba79c18ab4e586be2deccd78e4479d55eb75a7e) )
	ROM_LOAD( "ww.a12", 0x30000, 0x20000, CRC(d6617b19) SHA1(aa36d257eaa52c8c871a39aaa2f29c203525dbaf) )
ROM_END

/***************************************************************************/

static READ16_HANDLER( ww_io_service_r ){
	return input_port_2_word_r(offset,0) | (sys16_workingram[0x2082/2] & 0xff00);
}

static MEMORY_READ16_START( wrestwar_readmem )
	{ 0x000000, 0x0bffff, MRA16_ROM },
	{ 0x100000, 0x10ffff, SYS16_MRA16_TILERAM },
	{ 0x110000, 0x111fff, SYS16_MRA16_TEXTRAM },
	{ 0x200000, 0x200fff, SYS16_MRA16_SPRITERAM },
	{ 0x300000, 0x300fff, SYS16_MRA16_PALETTERAM },
	{ 0xc41002, 0xc41003, input_port_0_word_r }, /* player1 */
	{ 0xc41006, 0xc41007, input_port_1_word_r }, /* player2 */
	{ 0xc42002, 0xc42003, input_port_3_word_r }, /* dip1 */
	{ 0xc42000, 0xc42001, input_port_4_word_r }, /* dip2 */
	{ 0xffe082, 0xffe083, ww_io_service_r },
	{ 0xffc000, 0xffffff, SYS16_MRA16_WORKINGRAM },
MEMORY_END

static MEMORY_WRITE16_START( wrestwar_writemem )
	{ 0x000000, 0x0bffff, MWA16_ROM },
	{ 0x100000, 0x10ffff, SYS16_MWA16_TILERAM, &sys16_tileram },
	{ 0x110000, 0x111fff, SYS16_MWA16_TEXTRAM, &sys16_textram },
	{ 0x200000, 0x200fff, SYS16_MWA16_SPRITERAM, &sys16_spriteram },
	{ 0x300000, 0x300fff, SYS16_MWA16_PALETTERAM, &paletteram16 },
	{ 0x400000, 0x400003, sys16_tilebank_w },
	{ 0xc40000, 0xc40001, sys16_coinctrl_w },
	{ 0xc43034, 0xc43035, MWA16_NOP },
	{ 0xffe08e, 0xffe08f, sound_command_w },
	{ 0xffc000, 0xffffff, SYS16_MWA16_WORKINGRAM, &sys16_workingram },
MEMORY_END

/***************************************************************************/

static MACHINE_INIT( wrestwar ){
	sys16_bg_priority_mode=2;
	sys16_bg_priority_value=0x0a00;
	sys16_update_proc = type0_sys16_textram;
	sys16_sprite_draw = 1;
	sys16_wwfix = 1;
}

static DRIVER_INIT( wrestwar ){
	sys16_bg1_trans=1;
	sys16_MaxShadowColors=16;
	sys18_splittab_bg_y=&sys16_textram[0x0f40 /2];
	sys18_splittab_fg_y=&sys16_textram[0x0f00 /2];
	sys16_rowscroll_scroll=0x8000;
}

/***************************************************************************/

INPUT_PORTS_START( wrestwar )
	SYS16_JOY1
	SYS16_JOY2
	SYS16_SERVICE
	SYS16_COINAGE

PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, "Round Time" )
	PORT_DIPSETTING(    0x00, "100" )
	PORT_DIPSETTING(    0x0c, "110" )
	PORT_DIPSETTING(    0x08, "120" )
	PORT_DIPSETTING(    0x04, "130" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Continuation" )
	PORT_DIPSETTING(    0x20, "Continue" )
	PORT_DIPSETTING(    0x00, "No Continue" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x80, "Easy" )
	PORT_DIPSETTING(    0xc0, "Normal" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )

INPUT_PORTS_END

/***************************************************************************/

static MACHINE_DRIVER_START( wrestwar )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16_7759b)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(wrestwar_readmem,wrestwar_writemem)

	MDRV_MACHINE_INIT(wrestwar)
MACHINE_DRIVER_END

/*****************************************************************************/


/*****************************************************************************/
/* Dummy drivers for games that don't have a working clone and are protected */
/*****************************************************************************/

static MEMORY_READ16_START( sys16_dummy_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM },
	{ 0xff0000, 0xffffff, SYS16_MRA16_WORKINGRAM },
MEMORY_END

static MEMORY_WRITE16_START( sys16_dummy_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM },
	{ 0xff0000, 0xffffff, SYS16_MWA16_WORKINGRAM, &sys16_workingram },
MEMORY_END

static MACHINE_INIT( sys16_dummy ){
}

static DRIVER_INIT( s16dummy ){
}

INPUT_PORTS_START( s16dummy )
INPUT_PORTS_END

static MACHINE_DRIVER_START( s16dummy )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(sys16_dummy_readmem,sys16_dummy_writemem)

	MDRV_MACHINE_INIT(sys16_dummy)
MACHINE_DRIVER_END

/*****************************************************************************/
/* Cotton */

ROM_START( cotton )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 code */
/* custom cpu 317-????? */
	ROM_LOAD16_BYTE( "epr13858.a7", 0x000000, 0x20000, CRC(276f42fe) SHA1(47d2f910f6f101628cb0c660e5fd12c0e331bd99) )
	ROM_LOAD16_BYTE( "epr13856.a5", 0x000001, 0x20000, CRC(14e6b5e7) SHA1(708c69cddd1c60e729a74f539d40e67b2a6d9d6f) )
	ROM_LOAD16_BYTE( "epr13859.a8", 0x040000, 0x20000, CRC(4703ef9d) SHA1(8b03a71736a599c337ad5d95cbc812ea38b0cc43) )
	ROM_LOAD16_BYTE( "epr13857.a6", 0x040001, 0x20000, CRC(de37e527) SHA1(124ce7c7eef1199c89735556cebf71255573a155) )

	ROM_REGION( 0xc0000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "scr01.rom", 0x00000, 0x20000, CRC(a47354b6) SHA1(ce52813b245f1d491a134d4bd5ab074e71d20129) )
	ROM_LOAD( "scr11.rom", 0x20000, 0x20000, CRC(d38424b5) SHA1(884ca190936aee2d2cac86491d4d0cdf4a45efe5) )
	ROM_LOAD( "scr02.rom", 0x40000, 0x20000, CRC(8c990026) SHA1(07b4510936376c171f3b31d87ac6154361eb0cbc) )
	ROM_LOAD( "scr12.rom", 0x60000, 0x20000, CRC(21c15b8a) SHA1(690d92420ec5465885e0f4870419992961420e33) )
	ROM_LOAD( "scr03.rom", 0x80000, 0x20000, CRC(d2b175bf) SHA1(897b7c794d0e7229ea5e9a682f64266a947a818f) )
	ROM_LOAD( "scr13.rom", 0xa0000, 0x20000, CRC(b9d62531) SHA1(e8c5e7b93339c00f75a3b66ce18f7838255577be) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "obj0-e.rom", 0x000001, 0x20000, CRC(ab4b3468) SHA1(3071654a295152d609d2c2c1d4153b5ba3f174d5) )
	ROM_LOAD16_BYTE( "obj0-o.rom", 0x000000, 0x20000, CRC(7024f404) SHA1(4b2f9cdfdd97218797a3e386106e53f713b8650d) )
	ROM_LOAD16_BYTE( "obj1-e.rom", 0x040001, 0x20000, CRC(69b41ac3) SHA1(4c5a85e5a5ca9f8260557d4e97eb091dd857d63a) )
	ROM_LOAD16_BYTE( "obj1-o.rom", 0x040000, 0x20000, CRC(6169bba4) SHA1(a24a418ee7cd0c1109870a2e7a91e430671897ed) )
	ROM_LOAD16_BYTE( "obj2-e.rom", 0x080001, 0x20000, CRC(0801cf02) SHA1(3007bbbce2f327f4700e78e2b8672f4482189cd7) )
	ROM_LOAD16_BYTE( "obj2-o.rom", 0x080000, 0x20000, CRC(b014f02d) SHA1(46f5ed0b44cee03a6aec9ec57b506bb15bf35e47) )
	ROM_LOAD16_BYTE( "obj3-e.rom", 0x0c0001, 0x20000, CRC(f066f315) SHA1(bbeb24daaded994240d0cdb5cec2e662b677cb75) )
	ROM_LOAD16_BYTE( "obj3-o.rom", 0x0c0000, 0x20000, CRC(e62a7cd6) SHA1(1e6d06345f7b6cef2e887d9b9cd45e0155140c5e) )
	ROM_LOAD16_BYTE( "obj4-e.rom", 0x100001, 0x20000, CRC(1bd145f3) SHA1(4744ffe9fbda453785345b46eb61b56730048f42) )
	ROM_LOAD16_BYTE( "obj4-o.rom", 0x100000, 0x20000, CRC(943aba8b) SHA1(d0dd1665a8d9495a92ae4e35d6b15b966e8d43cd) )
	ROM_LOAD16_BYTE( "obj5-e.rom", 0x140001, 0x20000, CRC(4fd59bff) SHA1(2b4630e49b60593d668fe34d8faf712ac6928c14) )
	ROM_LOAD16_BYTE( "obj5-o.rom", 0x140000, 0x20000, CRC(7ea93200) SHA1(8e2d8cd48a12306772653f25bddc99ad0597a698) )
	ROM_LOAD16_BYTE( "obj6-e.rom", 0x180001, 0x20000, CRC(6a66868d) SHA1(60961a8b1f193d0b08c1906f4a79123fa0db443a) )
	ROM_LOAD16_BYTE( "obj6-o.rom", 0x180000, 0x20000, CRC(1c942190) SHA1(514fac5cc7362e9e3168c84975c8fe0e34bb4471) )
	ROM_LOAD16_BYTE( "obj7-e.rom", 0x1c0001, 0x20000, CRC(1c5ffad8) SHA1(13e5886ceece564cc71ba7f43a26d2b1782ccfc8) )
	ROM_LOAD16_BYTE( "obj7-o.rom", 0x1c0000, 0x20000, CRC(856f3ee2) SHA1(72346d887ff9738ebe93acb2e3f8cd80d494621e) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "s-prog.rom",	 0x00000, 0x08000, CRC(6a57b027) SHA1(8f9de548df203605bb4ab9eececf09739b55adf1) )
	ROM_LOAD( "speech0.rom", 0x10000, 0x20000, CRC(4d21153f) SHA1(173ddd9633f255c39ca508c37d0562e374704e7b) )
ROM_END


ROM_START( cottona )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 code */
/* custom cpu 317-0181a */
	ROM_LOAD16_BYTE( "ep13921a.a7", 0x000000, 0x20000, CRC(f047a037) SHA1(e4a9eeb1f2cf6b5ee3b2fcf74be917ffd3e6ca0e) )
	ROM_LOAD16_BYTE( "ep13919a.a5", 0x000001, 0x20000, CRC(651108b1) SHA1(1ab32085ca1a8cb3d464059b0abb10253decd423) )
	ROM_LOAD16_BYTE( "ep13922a.a8", 0x040000, 0x20000, CRC(1ca248c5) SHA1(f7df2ccf54a3f1f09334c3b3df3dcd2ec33f99d1) )
	ROM_LOAD16_BYTE( "ep13920a.a6", 0x040001, 0x20000, CRC(fa3610f9) SHA1(6127496bf7cd47d4343291fc2e11673d77ccc550) )

	ROM_REGION( 0xc0000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "scr01.rom", 0x00000, 0x20000, CRC(a47354b6) SHA1(ce52813b245f1d491a134d4bd5ab074e71d20129) )
	ROM_LOAD( "scr11.rom", 0x20000, 0x20000, CRC(d38424b5) SHA1(884ca190936aee2d2cac86491d4d0cdf4a45efe5) )
	ROM_LOAD( "scr02.rom", 0x40000, 0x20000, CRC(8c990026) SHA1(07b4510936376c171f3b31d87ac6154361eb0cbc) )
	ROM_LOAD( "scr12.rom", 0x60000, 0x20000, CRC(21c15b8a) SHA1(690d92420ec5465885e0f4870419992961420e33) )
	ROM_LOAD( "scr03.rom", 0x80000, 0x20000, CRC(d2b175bf) SHA1(897b7c794d0e7229ea5e9a682f64266a947a818f) )
	ROM_LOAD( "scr13.rom", 0xa0000, 0x20000, CRC(b9d62531) SHA1(e8c5e7b93339c00f75a3b66ce18f7838255577be) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "obj0-e.rom", 0x000001, 0x20000, CRC(ab4b3468) SHA1(3071654a295152d609d2c2c1d4153b5ba3f174d5) )
	ROM_LOAD16_BYTE( "obj0-o.rom", 0x000000, 0x20000, CRC(7024f404) SHA1(4b2f9cdfdd97218797a3e386106e53f713b8650d) )
	ROM_LOAD16_BYTE( "obj1-e.rom", 0x040001, 0x20000, CRC(69b41ac3) SHA1(4c5a85e5a5ca9f8260557d4e97eb091dd857d63a) )
	ROM_LOAD16_BYTE( "obj1-o.rom", 0x040000, 0x20000, CRC(6169bba4) SHA1(a24a418ee7cd0c1109870a2e7a91e430671897ed) )
	ROM_LOAD16_BYTE( "obj2-e.rom", 0x080001, 0x20000, CRC(0801cf02) SHA1(3007bbbce2f327f4700e78e2b8672f4482189cd7) )
	ROM_LOAD16_BYTE( "obj2-o.rom", 0x080000, 0x20000, CRC(b014f02d) SHA1(46f5ed0b44cee03a6aec9ec57b506bb15bf35e47) )
	ROM_LOAD16_BYTE( "obj3-e.rom", 0x0c0001, 0x20000, CRC(f066f315) SHA1(bbeb24daaded994240d0cdb5cec2e662b677cb75) )
	ROM_LOAD16_BYTE( "obj3-o.rom", 0x0c0000, 0x20000, CRC(e62a7cd6) SHA1(1e6d06345f7b6cef2e887d9b9cd45e0155140c5e) )
	ROM_LOAD16_BYTE( "obj4-e.rom", 0x100001, 0x20000, CRC(1bd145f3) SHA1(4744ffe9fbda453785345b46eb61b56730048f42) )
	ROM_LOAD16_BYTE( "obj4-o.rom", 0x100000, 0x20000, CRC(943aba8b) SHA1(d0dd1665a8d9495a92ae4e35d6b15b966e8d43cd) )
	ROM_LOAD16_BYTE( "obj5-e.rom", 0x140001, 0x20000, CRC(4fd59bff) SHA1(2b4630e49b60593d668fe34d8faf712ac6928c14) )
	ROM_LOAD16_BYTE( "obj5-o.rom", 0x140000, 0x20000, CRC(7ea93200) SHA1(8e2d8cd48a12306772653f25bddc99ad0597a698) )
	ROM_LOAD16_BYTE( "obj6-e.rom", 0x180001, 0x20000, CRC(6a66868d) SHA1(60961a8b1f193d0b08c1906f4a79123fa0db443a) )
	ROM_LOAD16_BYTE( "obj6-o.rom", 0x180000, 0x20000, CRC(1c942190) SHA1(514fac5cc7362e9e3168c84975c8fe0e34bb4471) )
	ROM_LOAD16_BYTE( "obj7-e.rom", 0x1c0001, 0x20000, CRC(1c5ffad8) SHA1(13e5886ceece564cc71ba7f43a26d2b1782ccfc8) )
	ROM_LOAD16_BYTE( "obj7-o.rom", 0x1c0000, 0x20000, CRC(856f3ee2) SHA1(72346d887ff9738ebe93acb2e3f8cd80d494621e) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "s-prog.rom",	 0x00000, 0x08000, CRC(6a57b027) SHA1(8f9de548df203605bb4ab9eececf09739b55adf1) )
	ROM_LOAD( "speech0.rom", 0x10000, 0x20000, CRC(4d21153f) SHA1(173ddd9633f255c39ca508c37d0562e374704e7b) )
ROM_END

/*****************************************************************************/
/* MVP */

ROM_START( mvp )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "13000.rom", 0x000000, 0x40000, CRC(2e0e21ec) SHA1(3b72da0746fb0ece4311d6e935bc6b9ece3549ec) )
	ROM_LOAD16_BYTE( "12999.rom", 0x000001, 0x40000, CRC(fd213d28) SHA1(5324ee402a2f28a6c152905493da0052d4976b29) )

	ROM_REGION( 0xc0000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "13011.rom", 0x00000, 0x40000, CRC(1cb871fc) SHA1(d20a46e538d57b25d78faa7deb9c11519b4111d3) )
	ROM_LOAD( "13012.rom", 0x40000, 0x40000, CRC(b75e6821) SHA1(a2b049995755d79a136a4b4b0dc78d902c5b9eed) )
	ROM_LOAD( "13013.rom", 0x80000, 0x40000, CRC(f1944a3c) SHA1(db59cadb435c26f3a957bd4996a083fa30c8bbd0) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "13010.rom", 0x000001, 0x40000, CRC(df37c567) SHA1(05c959e379a3e20fe5e70912410e3bae4db13905) )
	ROM_LOAD16_BYTE( "13009.rom", 0x000000, 0x40000, CRC(126d2e37) SHA1(91317d3bcc4f918a8607cc0c383181c3641ebccf) )
	ROM_LOAD16_BYTE( "13006.rom", 0x080001, 0x40000, CRC(2e9afd2f) SHA1(7fe0929e70e061878065fab2d26309066d14c038) )
	ROM_LOAD16_BYTE( "13003.rom", 0x080000, 0x40000, CRC(21424151) SHA1(156e15eee9ff7122c30a42bfec0b307073b7a375) )
	ROM_LOAD16_BYTE( "13007.rom", 0x100001, 0x40000, CRC(55c8605b) SHA1(6c81e5f9bcd61f6e67c87ea5b25a8fe1ee50f14d) )
	ROM_LOAD16_BYTE( "13004.rom", 0x100000, 0x40000, CRC(0aa09dd3) SHA1(0cd58d29efa714e52c494ee1ec8a0e90c0a03e3c) )
	ROM_LOAD16_BYTE( "13008.rom", 0x180001, 0x40000, CRC(b3d46dfc) SHA1(27a5c58d5fd974fbbb12b535a49aa7fb3f7d3f6a) )
	ROM_LOAD16_BYTE( "13005.rom", 0x180000, 0x40000, CRC(c899c810) SHA1(a251cfd8f99f2c2f98585cc9ba1d86f08b2eca51) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "13002.rom",	 0x00000, 0x08000, CRC(1b6e1515) SHA1(1816d48dcb1bfd819a2cfa55fb51e1ca04ad4feb) )
	ROM_LOAD( "13001.rom",   0x10000, 0x40000, CRC(e8cace8c) SHA1(5f47b935d927f2aa5f7a5f6dc52f5380baebe1bb) )
ROM_END

/*****************************************************************************/
/* Excite League */

ROM_START( exctleag )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr11937.a02",0x00000,0x10000, CRC(4ebda367) SHA1(ab4db50eb0d2e10a3b4b7fc0f4bdc82412379809) )
	ROM_LOAD16_BYTE( "epr11936.a01",0x00001,0x10000, CRC(0863de60) SHA1(540a5cae2623bce296c07603239f737a782e3b0b) )
	ROM_LOAD16_BYTE( "epr11939.a04",0x20000,0x10000, CRC(117dd98f) SHA1(0234c2cf3421849854bec3711ab64f50f12cc5dd) )
	ROM_LOAD16_BYTE( "epr11938.a03",0x20001,0x10000, CRC(07c08d47) SHA1(47d3445cfa2514918206cd29a203837a9f434b42) )
	ROM_LOAD16_BYTE( "epr11941.a06",0x40000,0x10000, CRC(4df2d451) SHA1(644541b20e034a0149117874021c158bd3759e35) )
	ROM_LOAD16_BYTE( "epr11940.a05",0x40001,0x10000, CRC(dec83274) SHA1(85919bcd372fbfb9f06c34897b4d28d08ef3c9d1) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "epr11942.b09",0x00000,0x10000, CRC(eb70e827) SHA1(0617b4411a90087c277354c3653fe994bc4fc580) )
	ROM_LOAD( "epr11943.b10",0x10000,0x10000, CRC(d97c8982) SHA1(3e604af1771caba3aa213796c4a0812a5e352580) )
	ROM_LOAD( "epr11944.b11",0x20000,0x10000, CRC(a75cae80) SHA1(17c148a33b09b5403e68f5d96e506545c2ced206) )

	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "epr11950.b01",0x00001,0x10000, CRC(af497849) SHA1(c5fdca8d3b1d83e3d377a49ecdc0fc53714afc09) )
	ROM_LOAD16_BYTE( "epr11951.b02",0x00000,0x10000, CRC(c04fa974) SHA1(b1a4fb8522126113d7857d559f169f09c5f51a13) )
	ROM_LOAD16_BYTE( "epr11952.b03",0x20001,0x10000, CRC(e64a9761) SHA1(e16b3a30165710abc11ab0f358cb7ef875cc672c) )
	ROM_LOAD16_BYTE( "epr11953.b04",0x20000,0x10000, CRC(4cae3999) SHA1(22089e43a5e2e4fe672015366154e24ad38d3c19) )
	ROM_LOAD16_BYTE( "epr11954.b05",0x40001,0x10000, CRC(5fa2106c) SHA1(2f2620fa52d07667dff4720fea32a6615d99e522) )
	ROM_LOAD16_BYTE( "epr11955.b06",0x40000,0x10000, CRC(86a0c368) SHA1(ab8d6ab5c571121bf9c5d40727b1ef385a033845) )
	ROM_LOAD16_BYTE( "epr11956.b07",0x60001,0x10000, CRC(aff5c2fa) SHA1(1ec76193f2abf3547fa610761147436548beccbc) )
	ROM_LOAD16_BYTE( "epr11957.b08",0x60000,0x10000, CRC(218f835b) SHA1(bfef3ec45665a5921c095da34701528d4d4e0e3a) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr11945.a07",0x00000,0x8000, CRC(c2a83012) SHA1(b3de8af803497438aa3e110a9c608ac290f7d1e8) )
	ROM_LOAD( "epr11140.a08",0x10000,0x8000, CRC(b297371b) SHA1(4e787aa9ee2aeab4da30237644421df407b684a5) )
	ROM_LOAD( "epr11141.a09",0x18000,0x8000, CRC(19756aa6) SHA1(81597e17b848f6a41f5fb117296e7508297815e4) )
	ROM_LOAD( "epr11142.a10",0x20000,0x8000, CRC(25d26c66) SHA1(2485afb859f84a9cc90b2091e9ae4eef263f42b3) )
	ROM_LOAD( "epr11143.a11",0x28000,0x8000, CRC(848b7b77) SHA1(8903a39f8f0ffb4ce32117d33282876196516c30) )

ROM_END



/*****************************************************************************/
/* Super League */

ROM_START( suprleag )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr11131.a02",0x00000,0x10000, CRC(9b78c2cc) SHA1(9a453dd999120590c52b17f8ff2b49fd2fde0b35) )
	ROM_LOAD16_BYTE( "epr11130.a01",0x00001,0x10000, CRC(e2451676) SHA1(d2f71d9fca933e63e2bd5ee48217801ab0cb049c) )
	ROM_LOAD16_BYTE( "epr11133.a04",0x20000,0x10000, CRC(eed72f37) SHA1(80b68abdb90a63b30754dd031e85b1020dcc0cc4) )
	ROM_LOAD16_BYTE( "epr11132.a03",0x20001,0x10000, CRC(ff199325) SHA1(2ece15b6b79ec3f948f7bec623e1df281cf89382) )
	ROM_LOAD16_BYTE( "epr11135.a06",0x40000,0x10000, CRC(3735e0e1) SHA1(ae2910099909245993ee29e5a03a5591f20962c7) )
	ROM_LOAD16_BYTE( "epr11134.a05",0x40001,0x10000, CRC(ccd857f5) SHA1(2566bb458bdd365db403e8229ecdad79e23076a1) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "epr11136.b09",0x00000,0x10000, CRC(c3860ce4) SHA1(af7618f3b5a0e8d6374877c7815ba69fff218a45) )
	ROM_LOAD( "epr11137.b10",0x10000,0x10000, CRC(92d96187) SHA1(45138795992b9842d5b0c86a96b300445bf12060) )
	ROM_LOAD( "epr11138.b11",0x20000,0x10000, CRC(c01dc773) SHA1(b27da906186e1272cdd6f8d5e5a979f6623255ac) )

	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "epr11144.b01",0x00001,0x10000, CRC(b31de51c) SHA1(011a79a37d32ab4545187e28e61f27aa0601f686) )
	ROM_LOAD16_BYTE( "epr11145.b02",0x00000,0x10000, CRC(4223d2c3) SHA1(310d5c411eeaf69fe0dc9350e6bfe6dfe950b373) )
	ROM_LOAD16_BYTE( "epr11146.b03",0x20001,0x10000, CRC(bf0359b6) SHA1(6cd5e6b685e53f062d5b04888f225c808aab3c4c) )
	ROM_LOAD16_BYTE( "epr11147.b04",0x20000,0x10000, CRC(3e592772) SHA1(b0fe2c680871dcdbe655c0b1b98bcf8118fb3a50) )
	ROM_LOAD16_BYTE( "epr11148.b05",0x40001,0x10000, CRC(126e1309) SHA1(7386ac5ac57325d8f661caf8cab0b19a42c0309d) )
	ROM_LOAD16_BYTE( "epr11149.b06",0x40000,0x10000, CRC(694d3765) SHA1(4aa8590648d5b8eb5db489edefb1326ecd01ea2c) )
	ROM_LOAD16_BYTE( "epr11150.b07",0x60001,0x10000, CRC(9fc0aded) SHA1(7ad9e8fe79a0a07c748b0c20ad46c0e00de8a23a) )
	ROM_LOAD16_BYTE( "epr11151.b08",0x60000,0x10000, CRC(9de95169) SHA1(1a2801ecd9dece3dae7ceab3b793d5005caa4614) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr11139.a07",0x00000,0x08000, CRC(9cbd99da) SHA1(55960b68b23a4f89ec810e1d31f22ba60cda0cfe) )
	ROM_LOAD( "epr11140.a08",0x10000,0x08000, CRC(b297371b) SHA1(4e787aa9ee2aeab4da30237644421df407b684a5) )
	ROM_LOAD( "epr11141.a09",0x18000,0x08000, CRC(19756aa6) SHA1(81597e17b848f6a41f5fb117296e7508297815e4) )
	ROM_LOAD( "epr11142.a10",0x20000,0x08000, CRC(25d26c66) SHA1(2485afb859f84a9cc90b2091e9ae4eef263f42b3) )
	ROM_LOAD( "epr11143.a11",0x28000,0x08000, CRC(848b7b77) SHA1(8903a39f8f0ffb4ce32117d33282876196516c30) )

ROM_END

/*****************************************************************************/

/*****************************************************************************/
/* Ryukyu */

ROM_START( ryukyu )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 code */
/* cpu 317-5023 */
	ROM_LOAD16_BYTE( "13347",0x00000,0x10000, CRC(398031fa) SHA1(5c118c85b7af1a83726b95bbeb85cb5020254f57) )
	ROM_LOAD16_BYTE( "13348",0x00001,0x10000, CRC(5f0e0c86) SHA1(f8f5912a190d0755cc5158e2e43cceb825f95b4f) )

	ROM_REGION( 0x60000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "13351",0x00000,0x20000, CRC(a68a4e6d) SHA1(ee3e317c7184b41af5dd383d41f7be3eebff0d04) )
	ROM_LOAD( "13352",0x20000,0x20000, CRC(5e5531e4) SHA1(e8e16b35f7985e6cdd77353ca5235db518914744) )
	ROM_LOAD( "13353",0x40000,0x20000, CRC(6d23dfd8) SHA1(21266340290b9854cee0b62fc107cc2981519a80) )

	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "13354",0x00001,0x20000, CRC(f07aad99) SHA1(71759525a5b7fe76d112cec93984f0f89cadbc00) )
	ROM_LOAD16_BYTE( "13355",0x00000,0x20000, CRC(67890019) SHA1(165c6a32f305273396ec0e9499e00329caadc484) )
	ROM_LOAD16_BYTE( "13356",0x40001,0x20000, CRC(5498290b) SHA1(b3115b636d8cb6ecac22d5264b7961e3b807cf04) )
	ROM_LOAD16_BYTE( "13357",0x40000,0x20000, CRC(f9e7cf03) SHA1(2258111499c79443faf84fb0495007016282bb3c) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "13349",0x00000,0x08000, CRC(b83183f8) SHA1(9d6127f51c04a16bb2637dc9992b843b94613c2b) )
	ROM_LOAD( "13350",0x10000,0x20000, CRC(3c59a658) SHA1(2cef13ee9e666bb850fe6c6e6954d7b75df665a9) )
ROM_END

/***************************************************************************/

/* Ace Attacker */
ROM_START( aceattac )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "11491.4a", 0x000000, 0x10000, CRC(77b820f1) SHA1(c33183c94c5029e2c4d6444f37404da66aacecc4) )
	ROM_LOAD16_BYTE( "11489.1a", 0x000001, 0x10000, CRC(bbe623c5) SHA1(6d047699c7b6df7ebb7a3c9bee032e2536eed84c) )
	ROM_LOAD16_BYTE( "11492.5a", 0x020000, 0x10000, CRC(d8bd3139) SHA1(54915d4e8a616e0e54135ca34daf4357b8bfa068) )
	ROM_LOAD16_BYTE( "11490.2a", 0x020001, 0x10000, CRC(38cb3a41) SHA1(1d74cc69907cdff2d85e965b80bf3f551465257e) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "11493.9b",  0x00000, 0x10000, CRC(654485d9) SHA1(b431270564c4e33fd70c8c85af1fcbff8b59ba49) )
	ROM_LOAD( "11494.10b", 0x10000, 0x10000, CRC(b67971ab) SHA1(95cb6927baf425bcc290832ea9741b19852c7a1b) )
	ROM_LOAD( "11495.11b", 0x20000, 0x10000, CRC(b687ab61) SHA1(b08130a9d777c918972895136b1bf520d7117114) )

	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "11501.1b", 0x00001, 0x10000, CRC(09179ead) SHA1(3e6bf04e1e9ea867d087a47ff04ad0a064a8e299) )
	ROM_LOAD16_BYTE( "11502.2b", 0x00000, 0x10000, CRC(a3ee36b8) SHA1(bc946ad67b8ad09d947465ab73160885a4a57be5) )
	ROM_LOAD16_BYTE( "11503.3b", 0x20001, 0x10000, CRC(344c0692) SHA1(3125701f6bb91d8f64515e214b571e169c30a444) )
	ROM_LOAD16_BYTE( "11504.4b", 0x20000, 0x10000, CRC(7cae7920) SHA1(9f00e01d7cc86a0bf4f84e78a56b7efbb97c5591) )
	ROM_LOAD16_BYTE( "11505.5b", 0x40001, 0x10000, CRC(b67f1ecf) SHA1(3a26cdf91e5a1a11c1a8857e713a9e00cc1bfce0) )
	ROM_LOAD16_BYTE( "11506.6b", 0x40000, 0x10000, CRC(b0104def) SHA1(c81a66ec3a600c1d4c5d058caef15936c59b2574) )
	ROM_LOAD16_BYTE( "11507.7b", 0x60001, 0x10000, CRC(a2af710a) SHA1(1c8b75b72797146c2eb788511f8cb1b367fc3e0d) )
	ROM_LOAD16_BYTE( "11508.8b", 0x60000, 0x10000, CRC(5cbb833c) SHA1(dc7041b6a4fa75d050bfc2176d0f9e242b55a0b8) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "11496.7a",	 0x00000, 0x08000, CRC(82cb40a9) SHA1(daf2233438331ba6e6ff8bda4015e92d23e616c5) )
	ROM_LOAD( "11497.8a",    0x10000, 0x08000, CRC(b04f62cc) SHA1(29b468e5a565dc14e00c371913663eca66ccb44d) )
	ROM_LOAD( "11498.9a",    0x18000, 0x08000, CRC(97baf52b) SHA1(97800014250b0099c7e53d597b0ef02ae14e6dba) )
	ROM_LOAD( "11499.10a",   0x20000, 0x08000, CRC(ea332866) SHA1(eba0b422b39f7f3f81af1059043a87d944c4aff7) )
	ROM_LOAD( "11500.11a",   0x28000, 0x08000, CRC(2ddf1c31) SHA1(77b20edbbd801072b20d9dc5e8fa2f468e53d79e) )
ROM_END



/* pre-System16 */
/*          rom        parent    machine   inp       init */
/* Alien Syndrome */
GAME( 1985, mjleague,  0,        mjleague, mjleague, 0,        ROT270, "Sega",    "Major League" )
GAME( 1986, bodyslam,  0,        bodyslam, bodyslam, bodyslam, ROT0,   "Sega",    "Body Slam" )
GAME( 1986, dumpmtmt,  bodyslam, bodyslam, bodyslam, bodyslam, ROT0,   "Sega",    "Dump Matsumoto (Japan)" )
GAMEX(1986, quartet,   0,        quartet,  quartet,  0,        ROT0,   "Sega",    "Quartet (Rev A, 8751 315-5194)", GAME_UNEMULATED_PROTECTION )
GAMEX(1986, quartetj,  quartet,  quartet,  quartet,  0,        ROT0,   "Sega",    "Quartet (8751 315-5194)", GAME_UNEMULATED_PROTECTION )
GAMEX(1986, quartet2,  quartet,  quartet2, quartet2, 0,        ROT0,   "Sega",    "Quartet 2 (8751 317-0010)", GAME_UNEMULATED_PROTECTION )
GAME( 1986, quartt2j,  quartet,  quartet2, quartet2, 0,        ROT0,   "Sega",    "Quartet 2 (unprotected)" )

/* System16A */
/*          rom        parent    machine   inp       init */
GAME( 1986, afighter,  0,        afighter, afighter, alexkida, ROT270, "Sega", "Action Fighter (FD1089A 317-0018)" )
GAME( 1986, afightera, afighter, afighter, afighter, 0,        ROT270, "Sega",    "Action Fighter (unprotected)" )
GAME( 1986, alexkidd,  0,        alexkidd, alexkidd, 0,        ROT0,   "Sega", "Alex Kidd: The Lost Stars (set 2, unprotected)" )
GAME( 1986, alexkidd1, alexkidd, alexkidd, alexkidd, alexkida, ROT0,   "Sega", "Alex Kidd: The Lost Stars (set 1, FD1089A 317-0021)" )
GAME( 1986, fantzone,  0,        fantzone, fantzone, 0,        ROT0,   "Sega",    "Fantasy Zone (Japan New Ver.)" )
GAME( 1986, fantzono,  fantzone, fantzono, fantzone, 0,        ROT0,   "Sega",    "Fantasy Zone (Old Ver.)" )
GAMEX(19??, ryukyu  ,  0,        s16dummy, s16dummy, 0,        ROT0,   "Sega",    "Ryukyu", GAME_NOT_WORKING )
GAME( 1987, shinobi,   0,        shinobi,  shinobi,  0,        ROT0,   "Sega",    "Shinobi (set 1)" )
GAMEX(1987, shinobib,  shinobi,  shinobi,  shinobi,  0,        ROT0,   "Sega",    "Shinobi (set 3)", GAME_NOT_WORKING )
GAMEX(1987, shinobia,  shinobi,  shinobl,  shinobi,  0,        ROT0,   "Sega",    "Shinobi (set 2)", GAME_NOT_WORKING )
GAME( 1987, shinobl,   shinobi,  shinobl,  shinobi,  0,        ROT0,   "bootleg", "Shinobi (bootleg)" )
GAME( 1987, sdi,       0,        sdi,      sdi,      sdi,      ROT0,   "Sega",    "SDI - Strategic Defense Initiative" )
GAMEX(1987, sdioj,     sdi,      sdi,      sdi,      sdi,      ROT0,   "Sega",    "SDI - Strategic Defense Initiative (Japan)", GAME_NOT_WORKING )
GAMEX(1988, tetris,    0,        tetris,   tetris,   0,        ROT0,   "Sega",    "Tetris (Sega Set 1)", GAME_NOT_WORKING )
GAME( 1988, tetrisbl,  tetris,   tetris,   tetris,   0,        ROT0,   "bootleg", "Tetris (Sega bootleg)" )
GAMEX(1988, tetrisa,   tetris,   tetris,   tetris,   0,        ROT0,   "Sega",    "Tetris (Sega Set 2)", GAME_NOT_WORKING )

/* System16B */
/*          rom        parent    machine   inp       init */
GAMEX(19??, aceattac,  0,        s16dummy, s16dummy, 0,        ROT0,   "Sega",    "Ace Attacker", GAME_NOT_WORKING )
GAMEX(1990, atomicp,   0,        atomicp,  atomicp,  0,        ROT0,   "Philko",  "Atomic Point (Korea)", GAME_NO_SOUND ) /* korean clone board.. */
GAME( 1990, snapper,   0,        snapper,  snapper,  0,        ROT0,   "Philko",  "Snapper (Korea)" ) /* korean clone board.. */
GAME( 1987, aliensyn,  0,        aliensyn, aliensyn, aliensyn, ROT0,   "Sega",    "Alien Syndrome (set 1)" )
GAMEX(1987, aliensya,  aliensyn, aliensyn, aliensyn, aliensyn, ROT0,   "Sega",    "Alien Syndrome (set 2)", GAME_NOT_WORKING )
GAMEX(1987, aliensyj,  aliensyn, aliensyn, aliensyn, aliensyn, ROT0,   "Sega",    "Alien Syndrome (Japan)", GAME_NOT_WORKING )
GAMEX(1987, aliensyb,  aliensyn, aliensyn, aliensyn, aliensyn, ROT0,   "Sega",    "Alien Syndrome (set 3)", GAME_NOT_WORKING )
GAME( 1988, altbeast,  0,        altbeast, altbeast, 0,        ROT0,   "Sega",    "Altered Beast (Version 1)" )
GAME( 1988, altbeas2,  altbeast, altbeas2, altbeast, altbeas2, ROT0,   "Sega",    "Altered Beast (Version 2)" )
GAMEX(1988, jyuohki,   altbeast, altbeast, altbeast, 0,        ROT0,   "Sega",    "Jyuohki (Japan)",           GAME_NOT_WORKING )

GAME( 1989, bayroute,  0,        bayroute, bayroute, 0,        ROT0,   "Sunsoft / Sega", "Bay Route (set 1)" )
GAMEX(1989, bayrouta,  bayroute, bayroute, bayroute, 0,        ROT0,   "Sunsoft / Sega", "Bay Route (set 2)", GAME_NOT_WORKING )
GAMEX(1989, bayrtbl1,  bayroute, bayroute, bayroute, bayrtbl1, ROT0,   "bootleg", "Bay Route (bootleg set 1)", GAME_NOT_WORKING )
GAMEX(1989, bayrtbl2,  bayroute, bayroute, bayroute, bayrtbl1, ROT0,   "bootleg", "Bay Route (bootleg set 2)", GAME_NOT_WORKING )
/* Bullet */
/* Charon */
GAMEX(19??, cotton,    0,        s16dummy, s16dummy, 0,        ROT0,   "Sega",    "Cotton (Japan)", GAME_NOT_WORKING )
GAMEX(19??, cottona,   cotton,   s16dummy, s16dummy, 0,        ROT0,   "Sega",    "Cotton", GAME_NOT_WORKING )
GAME( 1989, dduxbl,    0,        dduxbl,   dduxbl,   dduxbl,   ROT0,   "bootleg", "Dynamite Dux (bootleg)" )
GAMEX(1989, eswat,     0,        eswat,    eswat,    eswat,    ROT0,   "Sega",    "E-Swat - Cyber Police", GAME_NOT_WORKING )
GAME( 1989, eswatbl,   eswat,    eswat,    eswat,    eswat,    ROT0,   "bootleg", "E-Swat - Cyber Police (bootleg)" )
GAMEX(19??, exctleag,  0,        s16dummy, s16dummy, 0,        ROT0,   "Sega",    "Excite League", GAME_NOT_WORKING )

GAMEX(1989, fpoint,    0,        fpoint,   fpoint,   0,        ROT0,   "Sega",    "Flash Point", GAME_NOT_WORKING )
GAME( 1989, fpointbl,  fpoint,   fpointbl, fpoint,   fpointbl, ROT0,   "bootleg", "Flash Point (World, bootleg)" )
GAME( 1989, fpointbj,  fpoint,   fpointbl, fpointbj, fpointbl, ROT0,   "bootleg", "Flash Point (Japan, bootleg)" )

GAME( 1989, goldnaxe,  0,        goldnaxe, goldnaxe, 0,        ROT0,   "Sega",    "Golden Axe (Version 1)" )
GAMEX(1989, goldnaxj,  goldnaxe, goldnaxe, goldnaxe, 0,        ROT0,   "Sega",    "Golden Axe (Version 1, Japan)", GAME_NOT_WORKING )
GAMEX(1989, goldnabl,  goldnaxe, goldnaxe, goldnaxe, goldnabl, ROT0,   "bootleg", "Golden Axe (bootleg)", GAME_NOT_WORKING )
GAME( 1989, goldnaxa,  goldnaxe, goldnaxa, goldnaxe, 0,        ROT0,   "Sega",    "Golden Axe (set 6, US) (8751 317-123A)" )
GAMEX(1989, goldnaxb,  goldnaxe, goldnaxa, goldnaxe, 0,        ROT0,   "Sega",    "Golden Axe (Version 2 317-0110)", GAME_NOT_WORKING )
GAMEX(1989, goldnaxc,  goldnaxe, goldnaxa, goldnaxe, 0,        ROT0,   "Sega",    "Golden Axe (Version 2 317-0122)", GAME_NOT_WORKING )
GAME( 1987, hwchamp,   0,        hwchamp,  hwchamp,  0,        ROT0,   "Sega",    "Heavyweight Champ" )
GAMEX(19??, mvp,       0,        s16dummy, s16dummy, 0,        ROT0,   "Sega",    "MVP", GAME_NOT_WORKING )
GAMEX(1988, passsht,   0,        passsht,  passsht,  0,        ROT270, "Sega",    "Passing Shot (2 Players)", GAME_NOT_WORKING )
GAME( 1988, passshtb,  passsht,  passsht,  passsht,  0,        ROT270, "bootleg", "Passing Shot (2 Players) (bootleg)" )
GAMEX(1988, passht4b,  passsht,  passht4b, passht4b, passht4b, ROT270, "bootleg", "Passing Shot (4 Players) (bootleg)", GAME_IMPERFECT_SOUND )
GAME( 1991, riotcity,  0,        riotcity, riotcity, 0,        ROT0,   "Sega / Westone", "Riot City" )
/* Ryukyu */
/* Shinobi */
GAME( 1987, sonicbom,  0,        sonicbom, sonicbom, sonicbom, ROT270, "Sega",    "Sonic Boom" )
/* SDI */
/* Sukeban Jansi Ryuko */
GAMEX(19??, suprleag,  0,        s16dummy, s16dummy, 0,        ROT0,   "Sega",    "Super League", GAME_NOT_WORKING )
/* Tetris */
GAME( 1987, timscanr,  0,        timscanr, timscanr, 0,        ROT270, "Sega",    "Time Scanner" )
GAME (1994, toryumon,  0,        toryumon, toryumon, 0,        ROT0,   "Sega",    "Toryumon" )
GAME (1989, tturf,     0,        tturf,    tturf,    0,        ROT0,   "Sega / Sunsoft", "Tough Turf (Japan)")
GAME (1989, tturfu,    tturf,    tturfu,   tturf,    0,        ROT0,   "Sega / Sunsoft", "Tough Turf (US)")
GAME( 1989, tturfbl,   tturf,    tturfbl,  tturf,    tturfbl,  ROT0,   "bootleg", "Tough Turf (bootleg)")
GAME( 1988, wb3,       0,        wb3,      wb3,      0,        ROT0,   "Sega / Westone", "Wonder Boy III - Monster Lair (set 1)" )
GAMEX(1988, wb3a,      wb3,      wb3,      wb3,      0,        ROT0,   "Sega / Westone", "Wonder Boy III - Monster Lair (set 2)", GAME_NOT_WORKING )
GAME( 1988, wb3bl,     wb3,      wb3bl,    wb3,      wb3bl,    ROT0,   "bootleg", "Wonder Boy III - Monster Lair (bootleg)" )
GAME( 1989, wrestwar,  0,        wrestwar, wrestwar, wrestwar, ROT270, "Sega",    "Wrestle War" )

