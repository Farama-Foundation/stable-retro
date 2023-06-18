/***************************************************************************

	Atari Xybots hardware

	driver by Aaron Giles

	Games supported:
		* Xybots (1987) [5 sets]

	Known bugs:
		* none at this time

****************************************************************************

	Memory map (TBA)

***************************************************************************/


#include "driver.h"
#include "machine/atarigen.h"
#include "sndhrdw/atarijsa.h"
#include "xybots.h"



/*************************************
 *
 *	Initialization & interrupts
 *
 *************************************/

static void update_interrupts(void)
{
	int newstate = 0;

	if (atarigen_video_int_state)
		newstate = 1;
	if (atarigen_sound_int_state)
		newstate = 2;

	if (newstate)
		cpu_set_irq_line(0, newstate, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 7, CLEAR_LINE);
}


static MACHINE_INIT( xybots )
{
	atarigen_eeprom_reset();
	atarigen_slapstic_reset();
	atarigen_interrupt_reset(update_interrupts);
	atarijsa_reset();
}



/*************************************
 *
 *	I/O handlers
 *
 *************************************/

static READ16_HANDLER( special_port1_r )
{
	static int h256 = 0x0400;

	int result = readinputport(1);

	if (atarigen_cpu_to_sound_ready) result ^= 0x0200;
	result ^= h256 ^= 0x0400;
	return result;
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ16_START( main_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0xff8000, 0xffbfff, MRA16_RAM },
	{ 0xffc000, 0xffc7ff, MRA16_RAM },
	{ 0xffd000, 0xffdfff, atarigen_eeprom_r },
	{ 0xffe000, 0xffe0ff, atarigen_sound_r },
	{ 0xffe100, 0xffe1ff, input_port_0_word_r },
	{ 0xffe200, 0xffe2ff, special_port1_r },
MEMORY_END


static MEMORY_WRITE16_START( main_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0xff8000, 0xff8fff, atarigen_alpha_w, &atarigen_alpha },
	{ 0xff9000, 0xffadff, MWA16_RAM },
	{ 0xffae00, 0xffafff, atarimo_0_spriteram_w, &atarimo_0_spriteram },
	{ 0xffb000, 0xffbfff, atarigen_playfield_w, &atarigen_playfield },
	{ 0xffc000, 0xffc7ff, paletteram16_IIIIRRRRGGGGBBBB_word_w, &paletteram16 },
	{ 0xffd000, 0xffdfff, atarigen_eeprom_w, &atarigen_eeprom, &atarigen_eeprom_size },
	{ 0xffe800, 0xffe8ff, atarigen_eeprom_enable_w },
	{ 0xffe900, 0xffe9ff, atarigen_sound_w },
	{ 0xffea00, 0xffeaff, watchdog_reset16_w },
	{ 0xffeb00, 0xffebff, atarigen_video_int_ack_w },
	{ 0xffee00, 0xffeeff, atarigen_sound_reset_w },
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( xybots )
	PORT_START	/* ffe100 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2, "P2 Twist Right", KEYCODE_W, IP_JOY_DEFAULT )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2, "P2 Twist Left", KEYCODE_Q, IP_JOY_DEFAULT )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BITX(0x0400, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1, "P1 Twist Right", KEYCODE_X, IP_JOY_DEFAULT )
	PORT_BITX(0x0800, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1, "P1 Twist Left", KEYCODE_Z, IP_JOY_DEFAULT )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 | IPF_8WAY )

	PORT_START	/* ffe200 */
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_SERVICE( 0x0100, IP_ACTIVE_LOW )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_UNUSED ) 	/* /AUDBUSY */
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_UNUSED )	/* 256H */
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_VBLANK )	/* VBLANK */
	PORT_BIT( 0xf000, IP_ACTIVE_LOW, IPT_UNUSED )

	JSA_I_PORT_SWAPPED	/* audio port */
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
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0, 4, 8, 12, 16, 20, 24, 28 },
	{ 0*8, 4*8, 8*8, 12*8, 16*8, 20*8, 24*8, 28*8 },
	32*8
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &pfmolayout,    512, 16 },		/* playfield */
	{ REGION_GFX2, 0, &pfmolayout,    256, 48 },		/* sprites */
	{ REGION_GFX3, 0, &anlayout,        0, 64 },		/* characters 8x8 */
	{ -1 }
};



/*************************************
 *
 *	Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( xybots )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, ATARI_CLOCK_14MHz/2)
	MDRV_CPU_MEMORY(main_readmem,main_writemem)
	MDRV_CPU_VBLANK_INT(atarigen_video_int_gen,1)
	
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	
	MDRV_MACHINE_INIT(xybots)
	MDRV_NVRAM_HANDLER(atarigen)
	
	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(42*8, 30*8)
	MDRV_VISIBLE_AREA(0*8, 42*8-1, 0*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)
	
	MDRV_VIDEO_START(xybots)
	MDRV_VIDEO_UPDATE(xybots)
	
	/* sound hardware */
	MDRV_IMPORT_FROM(jsa_i_stereo_swapped)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definition(s)
 *
 *************************************/

ROM_START( xybots )
	ROM_REGION( 0x90000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "211217cd.054", 0x000000, 0x010000, CRC(16d64748) SHA1(3c2ba8ec3185b69c4e1947ac842f2250ee35216e) )
	ROM_LOAD16_BYTE( "211319cd.054", 0x000001, 0x010000, CRC(2677d44a) SHA1(23a3538df13a47f2fd78d4842b9f8b81e38c802e) )
	ROM_LOAD16_BYTE( "2114-17b.054", 0x020000, 0x008000, CRC(d31890cb) SHA1(b58722a4dcc79e97484c2f5e35b8dbf8c3520bd9) )
	ROM_LOAD16_BYTE( "2115-19b.054", 0x020001, 0x008000, CRC(750ab1b0) SHA1(0638de738bd804bde4b93cd23190ee0465887cf8) )

	ROM_REGION( 0x14000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "1116-2k.054",  0x010000, 0x004000, CRC(3b9f155d) SHA1(7080681a7eab282023034379825ca88adc6b300f) )
	ROM_CONTINUE(             0x004000, 0x00c000 )

	ROM_REGION( 0x40000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "2102-12l.054", 0x000000, 0x008000, CRC(c1309674) SHA1(5a163c894142c8d662557c8322dc04fded637227) )
	ROM_RELOAD(               0x008000, 0x008000 )
	ROM_LOAD( "2103-11l.054", 0x010000, 0x010000, CRC(907c024d) SHA1(d41c7471136f4a0632cbae28644ab1650af1467f) )
	ROM_LOAD( "2117-8l.054",  0x030000, 0x010000, CRC(0cc9b42d) SHA1(a744d97d40afb469ee61c2fc8d4b04ff8cc72755) )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "1105-2e.054",  0x000000, 0x010000, CRC(315a4274) SHA1(9a6cfdd655560e5d0320f95c8b60e733991a0909) )
	ROM_LOAD( "1106-2ef.054", 0x010000, 0x010000, CRC(3d8c1dd2) SHA1(dd61fc0b96c395e1e65bb7114a60b45d68d08140) )
	ROM_LOAD( "1107-2f.054",  0x020000, 0x010000, CRC(b7217da5) SHA1(b00ff4a3d0cffb94636f84cd923a78b5a02f9741) )
	ROM_LOAD( "1108-2fj.054", 0x030000, 0x010000, CRC(77ac65e1) SHA1(85a458adbc1a1c62dbd799f61e8f9f7f8811e06d) )
	ROM_LOAD( "1109-2jk.054", 0x040000, 0x010000, CRC(1b482c53) SHA1(50f463f00b7fad91c61bfeeb56bf76e120d24129) )
	ROM_LOAD( "1110-2k.054",  0x050000, 0x010000, CRC(99665ff4) SHA1(e93a85a601ae364d1e773174d488fca74b8d5753) )
	ROM_LOAD( "1111-2l.054",  0x060000, 0x010000, CRC(416107ee) SHA1(cdfe6c6bd8efaa08506cd5707887c552500c2108) )

	ROM_REGION( 0x02000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "1101-5c.054",  0x000000, 0x002000, CRC(59c028a2) SHA1(27dcde0da88f949a5e4a7632d4b403b937c8c6e0) )
ROM_END


ROM_START( xybotsg )
	ROM_REGION( 0x90000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "321217cd.054", 0x000000, 0x010000, CRC(4cac5d7c) SHA1(79cdd754fb6055249dace31fe9f8939f13aae8ca) )
	ROM_LOAD16_BYTE( "321319cd.054", 0x000001, 0x010000, CRC(bfcb0b00) SHA1(3e45f72051ea74b544c8578c6fc1284f925caa3d) )
	ROM_LOAD16_BYTE( "3214-17b.054", 0x020000, 0x008000, CRC(4ad35093) SHA1(6d2d82fb481c68819ec6c87d483eed17d4ae5d1a) )
	ROM_LOAD16_BYTE( "3215-19b.054", 0x020001, 0x008000, CRC(3a2afbaf) SHA1(61b88d15d95681eb24559d0696203cd4ee63d11f) )

	ROM_REGION( 0x14000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "1116-2k.054",  0x010000, 0x004000, CRC(3b9f155d) SHA1(7080681a7eab282023034379825ca88adc6b300f) )
	ROM_CONTINUE(             0x004000, 0x00c000 )

	ROM_REGION( 0x40000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "2102-12l.054", 0x000000, 0x008000, CRC(c1309674) SHA1(5a163c894142c8d662557c8322dc04fded637227) )
	ROM_RELOAD(               0x008000, 0x008000 )
	ROM_LOAD( "2103-11l.054", 0x010000, 0x010000, CRC(907c024d) SHA1(d41c7471136f4a0632cbae28644ab1650af1467f) )
	ROM_LOAD( "2117-8l.054",  0x030000, 0x010000, CRC(0cc9b42d) SHA1(a744d97d40afb469ee61c2fc8d4b04ff8cc72755) )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "1105-2e.054",  0x000000, 0x010000, CRC(315a4274) SHA1(9a6cfdd655560e5d0320f95c8b60e733991a0909) )
	ROM_LOAD( "1106-2ef.054", 0x010000, 0x010000, CRC(3d8c1dd2) SHA1(dd61fc0b96c395e1e65bb7114a60b45d68d08140) )
	ROM_LOAD( "1107-2f.054",  0x020000, 0x010000, CRC(b7217da5) SHA1(b00ff4a3d0cffb94636f84cd923a78b5a02f9741) )
	ROM_LOAD( "1108-2fj.054", 0x030000, 0x010000, CRC(77ac65e1) SHA1(85a458adbc1a1c62dbd799f61e8f9f7f8811e06d) )
	ROM_LOAD( "1109-2jk.054", 0x040000, 0x010000, CRC(1b482c53) SHA1(50f463f00b7fad91c61bfeeb56bf76e120d24129) )
	ROM_LOAD( "1110-2k.054",  0x050000, 0x010000, CRC(99665ff4) SHA1(e93a85a601ae364d1e773174d488fca74b8d5753) )
	ROM_LOAD( "1111-2l.054",  0x060000, 0x010000, CRC(416107ee) SHA1(cdfe6c6bd8efaa08506cd5707887c552500c2108) )

	ROM_REGION( 0x02000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "1101-5c.054",  0x000000, 0x002000, CRC(59c028a2) SHA1(27dcde0da88f949a5e4a7632d4b403b937c8c6e0) )
ROM_END


ROM_START( xybotsf )
	ROM_REGION( 0x90000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "361217cd.054", 0x000000, 0x010000, CRC(b03a3f3c) SHA1(c88ad0ba5381562095f5b5a13d338d10fa0597f5) )
	ROM_LOAD16_BYTE( "361319cd.054", 0x000001, 0x010000, CRC(ab33eb1f) SHA1(926c32f07c0bcc5832db3a1adf0357e55cae707a) )
	ROM_LOAD16_BYTE( "3614-17b.054", 0x020000, 0x008000, CRC(7385e0b6) SHA1(98a69901069872b14413c1bfe48783fdb43c1c37) )
	ROM_LOAD16_BYTE( "3615-19b.054", 0x020001, 0x008000, CRC(8e37b812) SHA1(40f973a49c4b40f3a5d982d332995e792f718dcc) )

	ROM_REGION( 0x14000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "1116-2k.054",  0x010000, 0x004000, CRC(3b9f155d) SHA1(7080681a7eab282023034379825ca88adc6b300f) )
	ROM_CONTINUE(             0x004000, 0x00c000 )

	ROM_REGION( 0x40000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "2102-12l.054", 0x000000, 0x008000, CRC(c1309674) SHA1(5a163c894142c8d662557c8322dc04fded637227) )
	ROM_RELOAD(               0x008000, 0x008000 )
	ROM_LOAD( "2103-11l.054", 0x010000, 0x010000, CRC(907c024d) SHA1(d41c7471136f4a0632cbae28644ab1650af1467f) )
	ROM_LOAD( "2117-8l.054",  0x030000, 0x010000, CRC(0cc9b42d) SHA1(a744d97d40afb469ee61c2fc8d4b04ff8cc72755) )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "1105-2e.054",  0x000000, 0x010000, CRC(315a4274) SHA1(9a6cfdd655560e5d0320f95c8b60e733991a0909) )
	ROM_LOAD( "1106-2ef.054", 0x010000, 0x010000, CRC(3d8c1dd2) SHA1(dd61fc0b96c395e1e65bb7114a60b45d68d08140) )
	ROM_LOAD( "1107-2f.054",  0x020000, 0x010000, CRC(b7217da5) SHA1(b00ff4a3d0cffb94636f84cd923a78b5a02f9741) )
	ROM_LOAD( "1108-2fj.054", 0x030000, 0x010000, CRC(77ac65e1) SHA1(85a458adbc1a1c62dbd799f61e8f9f7f8811e06d) )
	ROM_LOAD( "1109-2jk.054", 0x040000, 0x010000, CRC(1b482c53) SHA1(50f463f00b7fad91c61bfeeb56bf76e120d24129) )
	ROM_LOAD( "1110-2k.054",  0x050000, 0x010000, CRC(99665ff4) SHA1(e93a85a601ae364d1e773174d488fca74b8d5753) )
	ROM_LOAD( "1111-2l.054",  0x060000, 0x010000, CRC(416107ee) SHA1(cdfe6c6bd8efaa08506cd5707887c552500c2108) )

	ROM_REGION( 0x02000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "1101-5c.054",  0x000000, 0x002000, CRC(59c028a2) SHA1(27dcde0da88f949a5e4a7632d4b403b937c8c6e0) )
ROM_END


ROM_START( xybots1 )
	ROM_REGION( 0x90000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "111217cd.054", 0x000000, 0x010000, CRC(2dbab363) SHA1(1473bf1246c6fb3e6b8b1f86a345b532ccf18e8d) )
	ROM_LOAD16_BYTE( "111319cd.054", 0x000001, 0x010000, CRC(847b056e) SHA1(cc4b90f19d7eaee09569ba228c2654f64cec3200) )
	ROM_LOAD16_BYTE( "1114-17b.054", 0x020000, 0x008000, CRC(7444f88f) SHA1(e2a27754a57a809398ee639fe5d0920b564d4c0b) )
	ROM_LOAD16_BYTE( "1115-19b.054", 0x020001, 0x008000, CRC(848d072d) SHA1(c4d1181f0227200e60d99a99c1a83897275b055f) )

	ROM_REGION( 0x14000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "1116-2k.054",  0x010000, 0x004000, CRC(3b9f155d) SHA1(7080681a7eab282023034379825ca88adc6b300f) )
	ROM_CONTINUE(             0x004000, 0x00c000 )

	ROM_REGION( 0x40000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "2102-12l.054", 0x000000, 0x008000, CRC(c1309674) SHA1(5a163c894142c8d662557c8322dc04fded637227) )
	ROM_RELOAD(               0x008000, 0x008000 )
	ROM_LOAD( "2103-11l.054", 0x010000, 0x010000, CRC(907c024d) SHA1(d41c7471136f4a0632cbae28644ab1650af1467f) )
	ROM_LOAD( "2117-8l.054",  0x030000, 0x010000, CRC(0cc9b42d) SHA1(a744d97d40afb469ee61c2fc8d4b04ff8cc72755) )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "1105-2e.054",  0x000000, 0x010000, CRC(315a4274) SHA1(9a6cfdd655560e5d0320f95c8b60e733991a0909) )
	ROM_LOAD( "1106-2ef.054", 0x010000, 0x010000, CRC(3d8c1dd2) SHA1(dd61fc0b96c395e1e65bb7114a60b45d68d08140) )
	ROM_LOAD( "1107-2f.054",  0x020000, 0x010000, CRC(b7217da5) SHA1(b00ff4a3d0cffb94636f84cd923a78b5a02f9741) )
	ROM_LOAD( "1108-2fj.054", 0x030000, 0x010000, CRC(77ac65e1) SHA1(85a458adbc1a1c62dbd799f61e8f9f7f8811e06d) )
	ROM_LOAD( "1109-2jk.054", 0x040000, 0x010000, CRC(1b482c53) SHA1(50f463f00b7fad91c61bfeeb56bf76e120d24129) )
	ROM_LOAD( "1110-2k.054",  0x050000, 0x010000, CRC(99665ff4) SHA1(e93a85a601ae364d1e773174d488fca74b8d5753) )
	ROM_LOAD( "1111-2l.054",  0x060000, 0x010000, CRC(416107ee) SHA1(cdfe6c6bd8efaa08506cd5707887c552500c2108) )

	ROM_REGION( 0x02000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "1101-5c.054",  0x000000, 0x002000, CRC(59c028a2) SHA1(27dcde0da88f949a5e4a7632d4b403b937c8c6e0) )
ROM_END


ROM_START( xybots0 )
	ROM_REGION( 0x90000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "011217cd.054", 0x000000, 0x010000, CRC(4b830ac4) SHA1(1f6dc0c6648f74c4775b52e3f502e835a8741182) )
	ROM_LOAD16_BYTE( "011319cd.054", 0x000001, 0x010000, CRC(dcfbf8a7) SHA1(0106cd7be55147f4b59e17391e5bb339aaf80535) )
	ROM_LOAD16_BYTE( "0114-17b.054", 0x020000, 0x008000, CRC(18b875f7) SHA1(aa78553bd3556d0b209513ba80b782cfb0e3bb8b) )
	ROM_LOAD16_BYTE( "0115-19b.054", 0x020001, 0x008000, CRC(7f116360) SHA1(d12c339ce973bd74be4a4ac9e9d293f6a6e358d6) )

	ROM_REGION( 0x14000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "0116-2k.054",  0x010000, 0x004000, NO_DUMP CRC(3b9f155d) SHA1(7080681a7eab282023034379825ca88adc6b300f) )
	ROM_CONTINUE(             0x004000, 0x00c000 )

	ROM_REGION( 0x40000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1102-12l.054", 0x000000, 0x008000, CRC(0d304e5b) SHA1(203c86c865667b1538f61c0950682fb17ebd9abb) )
	ROM_RELOAD(               0x008000, 0x008000 )
	ROM_LOAD( "1103-11l.054", 0x010000, 0x010000, CRC(a514da1d) SHA1(5af3c703e0c8e8d47123241ce39f202c88a8cdb0) )
	ROM_LOAD( "1117-8l.054",  0x030000, 0x010000, CRC(6b79154d) SHA1(6fd47503c91a23f75046acd1ef8000b63f8e8ba6) )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "1105-2e.054",  0x000000, 0x010000, CRC(315a4274) SHA1(9a6cfdd655560e5d0320f95c8b60e733991a0909) )
	ROM_LOAD( "1106-2ef.054", 0x010000, 0x010000, CRC(3d8c1dd2) SHA1(dd61fc0b96c395e1e65bb7114a60b45d68d08140) )
	ROM_LOAD( "1107-2f.054",  0x020000, 0x010000, CRC(b7217da5) SHA1(b00ff4a3d0cffb94636f84cd923a78b5a02f9741) )
	ROM_LOAD( "1108-2fj.054", 0x030000, 0x010000, CRC(77ac65e1) SHA1(85a458adbc1a1c62dbd799f61e8f9f7f8811e06d) )
	ROM_LOAD( "1109-2jk.054", 0x040000, 0x010000, CRC(1b482c53) SHA1(50f463f00b7fad91c61bfeeb56bf76e120d24129) )
	ROM_LOAD( "1110-2k.054",  0x050000, 0x010000, CRC(99665ff4) SHA1(e93a85a601ae364d1e773174d488fca74b8d5753) )
	ROM_LOAD( "1111-2l.054",  0x060000, 0x010000, CRC(416107ee) SHA1(cdfe6c6bd8efaa08506cd5707887c552500c2108) )

	ROM_REGION( 0x02000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "1101-5c.054",  0x000000, 0x002000, CRC(59c028a2) SHA1(27dcde0da88f949a5e4a7632d4b403b937c8c6e0) )
ROM_END



/*************************************
 *
 *	Driver initialization
 *
 *************************************/

static DRIVER_INIT( xybots )
{
	atarigen_eeprom_default = NULL;
	atarigen_slapstic_init(0, 0x008000, 107);
	atarijsa_init(1, 2, 1, 0x0100);
}



/*************************************
 *
 *	Game driver(s)
 *
 *************************************/

GAME( 1987, xybots,  0,      xybots, xybots, xybots, ROT0, "Atari Games", "Xybots (rev 2)" )
GAME( 1987, xybotsg, xybots, xybots, xybots, xybots, ROT0, "Atari Games", "Xybots (German, rev 3)" )
GAME( 1987, xybotsf, xybots, xybots, xybots, xybots, ROT0, "Atari Games", "Xybots (French, rev 3)" )
GAME( 1987, xybots1, xybots, xybots, xybots, xybots, ROT0, "Atari Games", "Xybots (rev 1)" )
GAME( 1987, xybots0, xybots, xybots, xybots, xybots, ROT0, "Atari Games", "Xybots (rev 0)" )
