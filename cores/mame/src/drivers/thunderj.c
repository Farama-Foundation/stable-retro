/***************************************************************************

	Atari ThunderJaws hardware

	driver by Aaron Giles

	Games supported:
		* ThunderJaws (1990)

	Known bugs:
		* none at this time

****************************************************************************

	Memory map (TBA)

***************************************************************************/


#include "driver.h"
#include "machine/atarigen.h"
#include "sndhrdw/atarijsa.h"
#include "thunderj.h"



static data16_t *shared_ram;
static data16_t *rom_base[2];


/*************************************
 *
 *	Initialization & interrupts
 *
 *************************************/

static void update_interrupts(void)
{
	int newstate = 0;
	int newstate2 = 0;

	if (atarigen_scanline_int_state)
		newstate |= 4, newstate2 |= 4;
	if (atarigen_sound_int_state)
		newstate |= 6;

	if (newstate)
		cpu_set_irq_line(0, newstate, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 7, CLEAR_LINE);

	if (newstate2)
		cpu_set_irq_line(1, newstate2, ASSERT_LINE);
	else
		cpu_set_irq_line(1, 7, CLEAR_LINE);
}


static MACHINE_INIT( thunderj )
{
	atarigen_eeprom_reset();
	atarivc_reset(atarivc_eof_data, 2);
	atarigen_interrupt_reset(update_interrupts);
	atarijsa_reset();
	
	rom_base[0] = (data16_t *)memory_region(REGION_CPU1);
	rom_base[1] = (data16_t *)memory_region(REGION_CPU2);
	cpu_setbank(1, shared_ram);
}



/*************************************
 *
 *	I/O handling
 *
 *************************************/

static READ16_HANDLER( special_port2_r )
{
	int result = readinputport(2);

	if (atarigen_sound_to_cpu_ready) result ^= 0x0004;
	if (atarigen_cpu_to_sound_ready) result ^= 0x0008;
	result ^= 0x0010;

	return result;
}


static WRITE16_HANDLER( latch_w )
{
	/* reset extra CPU */
	if (ACCESSING_LSB)
	{
		/* 0 means hold CPU 2's reset low */
		if (data & 1)
			cpu_set_reset_line(1, CLEAR_LINE);
		else
			cpu_set_reset_line(1, ASSERT_LINE);

		/* bits 2-5 are the alpha bank */
		if (thunderj_alpha_tile_bank != ((data >> 2) & 7))
		{
			force_partial_update(cpu_getscanline());
			tilemap_mark_all_tiles_dirty(atarigen_alpha_tilemap);
			thunderj_alpha_tile_bank = (data >> 2) & 7;
		}
	}
}



/*************************************
 *
 *	Synchronization helper
 *
 *************************************/
 
static void shared_sync_callback(int param)
{
	if (--param)
		timer_set(TIME_IN_USEC(50), param, shared_sync_callback);
}


static READ16_HANDLER( shared_ram_r )
{
	data16_t result = shared_ram[offset];
	
	/* look for a byte access, and then check for the high bit and a TAS opcode */
	if (mem_mask != 0 && (result & ~mem_mask & 0x8080))
	{
		offs_t ppc = activecpu_get_previouspc();
		if (ppc < 0xa0000)
		{
			int cpunum = cpu_getactivecpu();
			UINT16 opcode = rom_base[cpunum][ppc / 2];

			/* look for TAS or BTST #$7; both CPUs spin waiting for these in order to */
			/* coordinate communications. Some spins have timeouts that reset the machine */
			/* if they fail, so we must make sure they are released in time */
			if ((opcode & 0xffc0) == 0x4ac0 ||
				((opcode & 0xffc0) == 0x0080 && rom_base[cpunum][ppc / 2 + 1] == 7))
			{
				timer_set(TIME_NOW, 4, shared_sync_callback);
			}
		}
	}
	
	return result;
}



/*************************************
 *
 *	Video Controller Hack
 *
 *************************************/

READ16_HANDLER( thunderj_video_control_r )
{
	/* Sigh. CPU #1 reads the video controller register twice per frame, once at
	   the beginning of interrupt and once near the end. It stores these values in a
	   table starting at $163484. CPU #2 periodically looks at this table to make
	   sure that it is getting interrupts at the appropriate times, and that the
	   VBLANK bit is set appropriately. Unfortunately, due to all the cpu_yield()
	   calls we make to synchronize the two CPUs, we occasionally get out of time
	   and generate the interrupt outside of the tight tolerances CPU #2 expects.

	   So we fake it. Returning scanlines $f5 and $f7 alternately provides the
	   correct answer that causes CPU #2 to be happy and not aggressively trash
	   memory (which is what it does if this interrupt test fails -- see the code
	   at $1E56 to see!) */

	/* Use these lines to detect when things go south:

	if (cpu_readmem24bew_word(0x163482) > 0xfff)
		log_cb(RETRO_LOG_DEBUG, LOGPRE "You're screwed!");*/

	return atarivc_r(offset,0);
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ16_START( main_readmem )
	{ 0x000000, 0x09ffff, MRA16_ROM },
	{ 0x0e0000, 0x0e0fff, atarigen_eeprom_r },
	{ 0x160000, 0x16ffff, shared_ram_r },
	{ 0x260000, 0x26000f, input_port_0_word_r },
	{ 0x260010, 0x260011, input_port_1_word_r },
	{ 0x260012, 0x260013, special_port2_r },
	{ 0x260030, 0x260031, atarigen_sound_r },
	{ 0x3e0000, 0x3e0fff, MRA16_RAM },
	{ 0x3effc0, 0x3effff, thunderj_video_control_r },
	{ 0x3f0000, 0x3fffff, MRA16_RAM },
MEMORY_END


static MEMORY_WRITE16_START( main_writemem )
	{ 0x000000, 0x09ffff, MWA16_ROM },
	{ 0x0e0000, 0x0e0fff, atarigen_eeprom_w, &atarigen_eeprom, &atarigen_eeprom_size },
	{ 0x160000, 0x16ffff, MWA16_BANK1, &shared_ram },
	{ 0x1f0000, 0x1fffff, atarigen_eeprom_enable_w },
	{ 0x2e0000, 0x2e0001, watchdog_reset16_w },
	{ 0x360010, 0x360011, latch_w },
	{ 0x360020, 0x360021, atarigen_sound_reset_w },
	{ 0x360030, 0x360031, atarigen_sound_w },
	{ 0x3e0000, 0x3e0fff, atarigen_666_paletteram_w, &paletteram16 },
	{ 0x3effc0, 0x3effff, atarivc_w, &atarivc_data },
	{ 0x3f0000, 0x3f1fff, atarigen_playfield2_latched_msb_w, &atarigen_playfield2 },
	{ 0x3f2000, 0x3f3fff, atarigen_playfield_latched_lsb_w, &atarigen_playfield },
	{ 0x3f4000, 0x3f5fff, atarigen_playfield_dual_upper_w, &atarigen_playfield_upper },
	{ 0x3f6000, 0x3f7fff, atarimo_0_spriteram_w, &atarimo_0_spriteram },
	{ 0x3f8000, 0x3f8eff, atarigen_alpha_w, &atarigen_alpha },
	{ 0x3f8f00, 0x3f8f7f, MWA16_RAM, &atarivc_eof_data },
	{ 0x3f8f80, 0x3f8fff, atarimo_0_slipram_w, &atarimo_0_slipram },
	{ 0x3f9000, 0x3fffff, MWA16_RAM },
MEMORY_END



/*************************************
 *
 *	Extra CPU memory handlers
 *
 *************************************/

static MEMORY_READ16_START( extra_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x060000, 0x07ffff, MRA16_ROM },
	{ 0x160000, 0x16ffff, shared_ram_r },
	{ 0x260000, 0x26000f, input_port_0_word_r },
	{ 0x260010, 0x260011, input_port_1_word_r },
	{ 0x260012, 0x260013, special_port2_r },
	{ 0x260030, 0x260031, atarigen_sound_r },
MEMORY_END


static MEMORY_WRITE16_START( extra_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x060000, 0x07ffff, MWA16_ROM },
	{ 0x160000, 0x16ffff, MWA16_BANK1 },
	{ 0x360000, 0x360001, atarigen_video_int_ack_w },
	{ 0x360010, 0x360011, latch_w },
	{ 0x360020, 0x360021, atarigen_sound_reset_w },
	{ 0x360030, 0x360031, atarigen_sound_w },
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( thunderj )
	PORT_START		/* 260000 */
	PORT_BIT( 0xffff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* 260010 */
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0c00, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1 )

	PORT_START		/* 260012 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_SERVICE( 0x0002, IP_ACTIVE_LOW )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNUSED )	/* Input buffer full (@260030) */
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNUSED )	/* Output buffer full (@360030) */
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x00e0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x0c00, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )

	JSA_II_PORT		/* audio board port */
INPUT_PORTS_END



/*************************************
 *
 *	Graphics definitions
 *
 *************************************/

static struct GfxLayout anlayout =
{
	8,8,
	RGN_FRAC(1,1),
	2,
	{ 0, 4 },
	{ 0, 1, 2, 3, 8, 9, 10, 11 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	8*16
};


static struct GfxLayout pfmolayout =
{
	8,8,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(3,4), RGN_FRAC(2,4), RGN_FRAC(1,4), RGN_FRAC(0,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &pfmolayout,  512,  96 },	/* sprites & playfield */
	{ REGION_GFX2, 0, &pfmolayout,  256, 112 },	/* sprites & playfield */
	{ REGION_GFX3, 0, &anlayout,      0, 512 },	/* characters 8x8 */
	{ -1 }
};



/*************************************
 *
 *	Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( thunderj )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, ATARI_CLOCK_14MHz/2)
	MDRV_CPU_MEMORY(main_readmem,main_writemem)
	
	MDRV_CPU_ADD(M68000, ATARI_CLOCK_14MHz/2)
	MDRV_CPU_MEMORY(extra_readmem,extra_writemem)
	
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	
	MDRV_MACHINE_INIT(thunderj)
	MDRV_NVRAM_HANDLER(atarigen)
	MDRV_INTERLEAVE(100)
	
	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(42*8, 30*8)
	MDRV_VISIBLE_AREA(0*8, 42*8-1, 0*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)
	
	MDRV_VIDEO_START(thunderj)
	MDRV_VIDEO_UPDATE(thunderj)
	
	/* sound hardware */
	MDRV_IMPORT_FROM(jsa_ii_mono)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definition(s)
 *
 *************************************/

ROM_START( thunderj )
	ROM_REGION( 0xa0000, REGION_CPU1, 0 )	/* 10*64k for 68000 code */
	ROM_LOAD16_BYTE( "2001.14e",   0x00000, 0x10000, CRC(f6a71532) SHA1(b1c55968d7da9b64bde737d66aa8f0ddcdcfee27) )
	ROM_LOAD16_BYTE( "2002.14c",   0x00001, 0x10000, CRC(173ec10d) SHA1(e32eca9194336f3d7e289b2a187ed125ed03688c) )
	ROM_LOAD16_BYTE( "2003.15e",   0x20000, 0x10000, CRC(6e155469) SHA1(ba87d0a510304fd8a0f91c81580c4f09fc4d1886) )
	ROM_LOAD16_BYTE( "2004.15c",   0x20001, 0x10000, CRC(e9ff1e42) SHA1(416dd68ec2ae174a5b14fa6fb4fb88bc1afdda66) )
	ROM_LOAD16_BYTE( "2005.16e",   0x40000, 0x10000, CRC(a40242e7) SHA1(ea2311f064885912054ae2e50a60664b216a6253) )
	ROM_LOAD16_BYTE( "2006.16c",   0x40001, 0x10000, CRC(aa18b94c) SHA1(867fd1d5485eacb705ed4ec736ee8c2e78aa9bf4) )
	ROM_LOAD16_BYTE( "1005.15h",   0x60000, 0x10000, CRC(05474ebb) SHA1(74a32dba5ffe2953c81ad9639d99ed01b31b0dba) )
	ROM_LOAD16_BYTE( "1010.16h",   0x60001, 0x10000, CRC(ccff21c8) SHA1(7df8facf563cc1bb8de6cac6f2ddcc58ae0aa8b4) )
	ROM_LOAD16_BYTE( "1007.17e",   0x80000, 0x10000, CRC(9c2a8aba) SHA1(10e4fc04e64bb6a5083a56f630224b5d1af241b2) )
	ROM_LOAD16_BYTE( "1008.17c",   0x80001, 0x10000, CRC(22109d16) SHA1(8725696271c4a617f9f050d9d483fe4141bf1e00) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "1011.17l",    0x00000, 0x10000, CRC(bbbbca45) SHA1(977e785e0272a84c8d7e28e25f45064d1b37aad1) )
	ROM_LOAD16_BYTE( "1012.17n",    0x00001, 0x10000, CRC(53e5e638) SHA1(75593e5d328ede105b8db64005dd5d1c5cae11ed) )
	ROM_COPY( REGION_CPU1, 0x60000, 0x60000, 0x20000 )

	ROM_REGION( 0x14000, REGION_CPU3, 0 )	/* 64k + 16k for 6502 code */
	ROM_LOAD( "tjw65snd.bin",  0x10000, 0x4000, CRC(d8feb7fb) SHA1(684ebf2f0c0df742c98e7f45f74de86a11c8d6e8) )
	ROM_CONTINUE(              0x04000, 0xc000 )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "1021.5s",   0x000000, 0x10000, CRC(d8432766) SHA1(04e7d820974c0890fde1257b4710cf7b520d7d48) )	/* graphics, plane 0 */
	ROM_LOAD( "1025.5r",   0x010000, 0x10000, CRC(839feed5) SHA1(c683ef5b78f8fd63dd557a630544f1e21aebe665) )
	ROM_LOAD( "1029.3p",   0x020000, 0x10000, CRC(fa887662) SHA1(5d19022e8d40be86b85d0bcc28c97207ab9ec403) )
	ROM_LOAD( "1033.6p",   0x030000, 0x10000, CRC(2addda79) SHA1(5a04c718055a5637b7549598ec39ca3cc9883698) )
	ROM_LOAD( "1022.9s",   0x040000, 0x10000, CRC(dcf50371) SHA1(566e71e1dcb8e0266ca870af04b11f7bbee21b18) )	/* graphics, plane 1 */
	ROM_LOAD( "1026.9r",   0x050000, 0x10000, CRC(216e72c8) SHA1(b6155584c8760c4dee3cf2a6320c53ea2161464b) )
	ROM_LOAD( "1030.10s",  0x060000, 0x10000, CRC(dc51f606) SHA1(aa401808d915b2e6cdb17a1d58814a753648c9bb) )
	ROM_LOAD( "1034.10r",  0x070000, 0x10000, CRC(f8e35516) SHA1(dcb23ed69f5a70ac842c6004039ec403bac68d72) )
	ROM_LOAD( "1023.13s",  0x080000, 0x10000, CRC(b6dc3f13) SHA1(c3369b58012e02ad2fd85f1c9643ee5792f4b3de) )	/* graphics, plane 2 */
	ROM_LOAD( "1027.13r",  0x090000, 0x10000, CRC(621cc2ce) SHA1(15db80d61f1c624c09085ed86341f8577bfac168) )
	ROM_LOAD( "1031.14s",  0x0a0000, 0x10000, CRC(4682ceb5) SHA1(609ccd20f654982e01bcc6aea89801c01afe083e) )
	ROM_LOAD( "1035.14r",  0x0b0000, 0x10000, CRC(7a0e1b9e) SHA1(b9a2270ee7e3b3dcf05a47085890d87bf5b3e167) )
	ROM_LOAD( "1024.17s",  0x0c0000, 0x10000, CRC(d84452b5) SHA1(29bc994e37bc08fa40326b811339e7aa3290302c) )	/* graphics, plane 3 */
	ROM_LOAD( "1028.17r",  0x0d0000, 0x10000, CRC(0cc20245) SHA1(ebdcb47909374508abe9d0252fd88d6274a0f729) )
	ROM_LOAD( "1032.14p",  0x0e0000, 0x10000, CRC(f639161a) SHA1(cc2549f7fdd251fa44735a6cd5fdb8ffb97948be) )
	ROM_LOAD( "1036.16p",  0x0f0000, 0x10000, CRC(b342443d) SHA1(fa7865f8a90c0e761e1cc5e155931d0574f2d81c) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "1037.2s",   0x000000, 0x10000, CRC(07addba6) SHA1(4a3286ee570bf4263944854bf959c8ef114cc123) )
	ROM_LOAD( "1041.2r",   0x010000, 0x10000, CRC(1e9c29e4) SHA1(e4afa2c469bfa22504cba5dfd23704c5c2bb33c4) )
	ROM_LOAD( "1045.34s",  0x020000, 0x10000, CRC(e7235876) SHA1(0bb03baec1de3e520dc270a3ed44bec953e08c00) )
	ROM_LOAD( "1049.34r",  0x030000, 0x10000, CRC(a6eb8265) SHA1(f9b5fbe69b973327ebe5ea6e9fa782cb6db010d6) )
	ROM_LOAD( "1038.6s",   0x040000, 0x10000, CRC(2ea543f9) SHA1(e5dafe023b1dc5068f367293da5774ab98ec617c) )
	ROM_LOAD( "1042.6r",   0x050000, 0x10000, CRC(efabdc2b) SHA1(449fa8f229d901328aa14c1d093d10ba1c7e7cd9) )
	ROM_LOAD( "1046.7s",   0x060000, 0x10000, CRC(6692151f) SHA1(294fb66f4a25ce0282b8f3b032df7a7103842540) )
	ROM_LOAD( "1050.7r",   0x070000, 0x10000, CRC(ad7bb5f3) SHA1(1e7083cafd4c06397991cb873eeeb4b4c8a81d9d) )
	ROM_LOAD( "1039.11s",  0x080000, 0x10000, CRC(cb563a40) SHA1(086619ae47c1c0b5fb1913b3e657216e021a3713) )
	ROM_LOAD( "1043.11r",  0x090000, 0x10000, CRC(b7565eee) SHA1(610439f8e08fb88a646e76180bcd72ddfec8c06a) )
	ROM_LOAD( "1047.12s",  0x0a0000, 0x10000, CRC(60877136) SHA1(ac7154b9324782c5c22e18c1d325f58c88938170) )
	ROM_LOAD( "1051.12r",  0x0b0000, 0x10000, CRC(d4715ff0) SHA1(f0c2c7057d84b337ab1f44667e6048885922698e) )
	ROM_LOAD( "1040.15s",  0x0c0000, 0x10000, CRC(6e910fc2) SHA1(29b811e0d8283dd00e2d79c2cbe120faa6834008) )
	ROM_LOAD( "1044.15r",  0x0d0000, 0x10000, CRC(ff67a17a) SHA1(17f3572b526a14bdf3a6da5711c4d96feefc25aa) )
	ROM_LOAD( "1048.16s",  0x0e0000, 0x10000, CRC(200d45b3) SHA1(72f1b3deaebc6266140fb79a97154f80f2cadf33) )
	ROM_LOAD( "1052.16r",  0x0f0000, 0x10000, CRC(74711ef1) SHA1(c1429d6b54dc4352defdd6cf83f1a5734784e703) )

	ROM_REGION( 0x010000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "1020.4m",   0x000000, 0x10000, CRC(65470354) SHA1(9895d26fa9e01c254a3d15e657152cac717c68a3) )	/* alphanumerics */

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* 256k for ADPCM */
	ROM_LOAD( "tj1016.bin",  0x00000, 0x10000, CRC(c10bdf73) SHA1(a0371c6ddef2a95193c68879044b3338d481fc96) )
	ROM_LOAD( "tj1017.bin",  0x10000, 0x10000, CRC(4e5e25e8) SHA1(373c946abd24ce8dd5221f1a0409af4537610d3d) )
	ROM_LOAD( "tj1018.bin",  0x20000, 0x10000, CRC(ec81895d) SHA1(56acffb0700d3b70ca705fba9d240a82950fd320) )
	ROM_LOAD( "tj1019.bin",  0x30000, 0x10000, CRC(a4009037) SHA1(01cd3f4cf510f4956258f39f3ddbb42628bc2b9a) )
ROM_END



/*************************************
 *
 *	Driver initialization
 *
 *************************************/

static DRIVER_INIT( thunderj )
{
	atarigen_eeprom_default = NULL;
	atarijsa_init(2, 3, 2, 0x0002);
	atarigen_init_6502_speedup(2, 0x4159, 0x4171);
}



/*************************************
 *
 *	Game driver(s)
 *
 *************************************/

GAME( 1990, thunderj, 0, thunderj, thunderj, thunderj, ROT0, "Atari Games", "ThunderJaws" )
