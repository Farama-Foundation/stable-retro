/***************************************************************************

	Atari Batman hardware

	driver by Aaron Giles

	Games supported:
		* Batman (1991)
		* Marble Madness II (prototype)

	Known bugs:
		* none at this time

****************************************************************************

	Memory map (TBA)

***************************************************************************/


#include "driver.h"
#include "machine/atarigen.h"
#include "sndhrdw/atarijsa.h"
#include "batman.h"



/*************************************
 *
 *	Statics
 *
 *************************************/

static data16_t latch_data;



/*************************************
 *
 *	Initialization
 *
 *************************************/

static void update_interrupts(void)
{
	int newstate = 0;

	if (atarigen_scanline_int_state)
		newstate |= 4;
	if (atarigen_sound_int_state)
		newstate |= 6;

	if (newstate)
		cpu_set_irq_line(0, newstate, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 7, CLEAR_LINE);
}

static void mm2_interupt_update(int freq)
{
	update_interrupts();
}

static MACHINE_INIT( batman )
{
	atarigen_eeprom_reset();
	atarivc_reset(atarivc_eof_data, 2);
	atarigen_interrupt_reset(update_interrupts);
	atarigen_scanline_timer_reset(batman_scanline_update, 8);
	atarijsa_reset();
}


static MACHINE_INIT( marblmd2 )
{
	atarigen_eeprom_reset();
	atarivc_reset(atarivc_eof_data, 2);
	atarigen_interrupt_reset(update_interrupts);
	atarigen_scanline_timer_reset(mm2_interupt_update, 60);
	atarijsa_reset();
	mm2_startup = 0;
}

/*************************************
 *
 *	I/O handling
 *
 *************************************/

static READ16_HANDLER( special_port2_r )
{
	int result = readinputport(2);

	if (atarigen_sound_to_cpu_ready) result ^= 0x0010;
	if (atarigen_cpu_to_sound_ready) result ^= 0x0020;
	return result;
}


static WRITE16_HANDLER( latch_w )
{
	int oldword = latch_data;
	COMBINE_DATA(&latch_data);

	/* bit 4 is connected to the /RESET pin on the 6502 */
	if (latch_data & 0x0010)
		cpu_set_reset_line(1, CLEAR_LINE);
	else
		cpu_set_reset_line(1, ASSERT_LINE);

	/* alpha bank is selected by the upper 4 bits */
	if ((oldword ^ latch_data) & 0x7000)
	{
		force_partial_update(cpu_getscanline());
		tilemap_mark_all_tiles_dirty(atarigen_alpha_tilemap);
		batman_alpha_tile_bank = (latch_data >> 12) & 7;
	}
}


static WRITE16_HANDLER( mm2_latch_w )
{
	int oldword = latch_data;
	COMBINE_DATA(&latch_data);

	/* bit 4 is connected to the /RESET pin on the 6502 */
	if (latch_data & 0x0010)
		cpu_set_reset_line(1, CLEAR_LINE);
	else
		cpu_set_reset_line(1, ASSERT_LINE);

}

/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ16_START( main_readmem )
	{ 0x000000, 0x0bffff, MRA16_ROM },
	{ 0x100000, 0x10ffff, MRA16_RAM },
	{ 0x120000, 0x120fff, atarigen_eeprom_r },
	{ 0x260000, 0x260001, input_port_0_word_r },
	{ 0x260002, 0x260003, input_port_1_word_r },
	{ 0x260010, 0x260011, special_port2_r },
	{ 0x260030, 0x260031, atarigen_sound_r },
	{ 0x3e0000, 0x3e0fff, MRA16_RAM },
	{ 0x3effc0, 0x3effff, atarivc_r },
	{ 0x3f0000, 0x3fffff, MRA16_RAM },
MEMORY_END


static MEMORY_WRITE16_START( main_writemem )
	{ 0x000000, 0x0bffff, MWA16_ROM },
	{ 0x100000, 0x10ffff, MWA16_RAM },
	{ 0x120000, 0x120fff, atarigen_eeprom_w, &atarigen_eeprom, &atarigen_eeprom_size },
	{ 0x260040, 0x260041, atarigen_sound_w },
	{ 0x260050, 0x260051, latch_w },
	{ 0x260060, 0x260061, atarigen_eeprom_enable_w },
	{ 0x2a0000, 0x2a0001, watchdog_reset16_w },
	{ 0x3e0000, 0x3e0fff, atarigen_666_paletteram_w, &paletteram16 },
	{ 0x3effc0, 0x3effff, atarivc_w, &atarivc_data },
	{ 0x3f0000, 0x3f1fff, atarigen_playfield2_latched_msb_w, &atarigen_playfield2 },
	{ 0x3f2000, 0x3f3fff, atarigen_playfield_latched_lsb_w, &atarigen_playfield },
	{ 0x3f4000, 0x3f5fff, atarigen_playfield_dual_upper_w, &atarigen_playfield_upper },
	{ 0x3f6000, 0x3f7fff, atarimo_0_spriteram_w, &atarimo_0_spriteram },
	{ 0x3f8000, 0x3f8fef, atarigen_alpha_w, &atarigen_alpha },
	{ 0x3f8f00, 0x3f8f7f, MWA16_RAM, &atarivc_eof_data },
	{ 0x3f8f80, 0x3f8fff, atarimo_0_slipram_w, &atarimo_0_slipram },
	{ 0x3f9000, 0x3fffff, MWA16_RAM },
MEMORY_END


static MEMORY_READ16_START( mm2_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x600000, 0x600001, /*600000*/ input_port_0_word_r },
	{ 0x600002, 0x600003, /*600002*/ input_port_1_word_r },
	{ 0x600010, 0x600011, /*600010*/ special_port2_r },
	{ 0x600012, 0x600013, /*600012*/ input_port_3_word_r },
	{ 0x600020, 0x600021, /*600020*/ input_port_4_word_r},
	{ 0x600030, 0x600031, atarigen_sound_r },
	{ 0x601000, 0x601fff, atarigen_eeprom_r },
	{ 0x7c0000, 0x7c03ff, MRA16_RAM },
	{ 0x7cffc0, 0x7cffff, atarivc_r },
	{ 0x7d0000, 0x7d7fff, MRA16_RAM },
	{ 0x7d8000, 0x7d9fff, MRA16_RAM },
	{ 0x7da000, 0x7dbeff, MRA16_RAM },
	{ 0x7dbf00, 0x7dbf7f, MRA16_RAM },
	{ 0x7dbf80, 0x7dbfff, MRA16_RAM },
	{ 0x7f8000, 0x7fbfff, MRA16_RAM },
MEMORY_END


static MEMORY_WRITE16_START( mm2_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x600040, 0x600041, atarigen_sound_w  },
	{ 0x600050, 0x600051, mm2_latch_w },
	{ 0x600060, 0x600061, atarigen_eeprom_enable_w },
	{ 0x601000, 0x601fff, atarigen_eeprom_w, &atarigen_eeprom, &atarigen_eeprom_size },
	{ 0x607000, 0x607000, MWA16_NOP },
	{ 0x7c0000, 0x7c03ff, atarigen_expanded_666_paletteram_w, &paletteram16 },
	{ 0x7cffc0, 0x7cffff, atarivc_w, &atarivc_data },
	{ 0x7d0000, 0x7d7fff, MWA16_RAM },
	{ 0x7d8000, 0x7d9fff, atarigen_playfield_latched_lsb_w, &atarigen_playfield },
	{ 0x7da000, 0x7dbeff, atarimo_0_spriteram_w, &atarimo_0_spriteram },
	{ 0x7dbf00, 0x7dbf7f, MWA16_RAM, &atarivc_eof_data },
	{ 0x7dbf80, 0x7dbfff, atarimo_0_slipram_w, &atarimo_0_slipram },
	{ 0x7f8000, 0x7fbfff, MWA16_RAM },
MEMORY_END


/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( marblmd2 )
	PORT_START /* 600000 input_port_0_word_r */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 ) /* also acts as START3 */
	PORT_BIT( 0x00fe, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 ) /* also acts as START1 */
	PORT_BIT( 0xfe00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START /* 600002 input_port_1_word_r */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* acts as a 'freeze' input, probably not connected */
	PORT_BIT( 0x00fe, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 ) /* also acts as START2 */
	PORT_BIT( 0xfe00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START /* 600010 input_port_2_word_r */
	PORT_BIT( 0x000f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNUSED )	/* Input buffer full (@260030) */
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNUSED )	/* Output buffer full (@260040) */
	PORT_SERVICE( 0x0040, IP_ACTIVE_LOW )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START /* 600012 input_port_3_word_r */
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, "Number of Players (Test Mode)" ) /* this one controls 'number of players' in Control Test */
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPSETTING(      0x0020, "3" )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Number of Players (Game)" )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPSETTING(      0x0080, "3" )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START /* 600020 input_port_4_word_r */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER3 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER3 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER3 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER3 )
	PORT_BIT( 0xf000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	JSA_III_PORT	/* audio board port */
INPUT_PORTS_END


INPUT_PORTS_START( batman )
	PORT_START		/* 26000 */
	PORT_BIT( 0x01ff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1 )

	PORT_START		/* 26002 */
	PORT_BIT( 0xffff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* 26010 */
	PORT_BIT( 0x000f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNUSED )	/* Input buffer full (@260030) */
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNUSED )	/* Output buffer full (@260040) */
	PORT_SERVICE( 0x0040, IP_ACTIVE_LOW )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_VBLANK )

	JSA_III_PORT	/* audio board port */
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
	{ RGN_FRAC(0,4), RGN_FRAC(1,4), RGN_FRAC(2,4), RGN_FRAC(3,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX3, 0, &pfmolayout,  512, 64 },		/* sprites & playfield */
	{ REGION_GFX2, 0, &pfmolayout,  256, 64 },		/* sprites & playfield */
	{ REGION_GFX1, 0, &anlayout,      0, 64 },		/* characters 8x8 */
	{ -1 }
};


static struct GfxLayout pflayout =
{
	8,8,
	RGN_FRAC(1,2),
	8,
	{ 0,1,2,3,4,5,6,7 },
	{ 0,RGN_FRAC(1,2) + 0, 8,RGN_FRAC(1,2) + 8,  16,RGN_FRAC(1,2) + 16,24,RGN_FRAC(1,2) + 24 },
	{ 0 * 32, 1 * 32, 2 * 32, 3 * 32, 4 * 32, 5 * 32, 6 * 32, 7 * 32 },
	8 * 32
};


static struct GfxLayout molayout =
{
	8,8,
	RGN_FRAC(1,2),
	4,
	{ 0, 1, 2, 3 },
	{ RGN_FRAC(1,2) + 0, RGN_FRAC(1,2) + 4, 0, 4, RGN_FRAC(1,2) + 8, RGN_FRAC(1,2) + 12, 8, 12  },
	{ 0 * 16, 1 * 16, 2 * 16, 3 * 16, 4 * 16, 5 * 16, 6 * 16, 7 * 16 },
	16 * 8
};


static struct GfxDecodeInfo gfxdecodeinfo2[] =
{
	{  REGION_GFX1, 0, &pflayout,   0x0, 1  },
	{  REGION_GFX2, 0, &molayout,   0x0, 0x10  },
	{ -1 }
};


/*************************************
 *
 *	Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( batman )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, ATARI_CLOCK_14MHz)
	MDRV_CPU_MEMORY(main_readmem,main_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(batman)
	MDRV_NVRAM_HANDLER(atarigen)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(42*8, 30*8)
	MDRV_VISIBLE_AREA(0*8, 42*8-1, 0*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)
	MDRV_COLORTABLE_LENGTH(2048) /* can't make colortable_len = 0 because of 0xffff transparency kludge */

	MDRV_VIDEO_START(batman)
	MDRV_VIDEO_UPDATE(batman)

	/* sound hardware */
	MDRV_IMPORT_FROM(jsa_iii_mono)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( marblmd2 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, ATARI_CLOCK_14MHz)
	MDRV_CPU_MEMORY(mm2_readmem,mm2_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(marblmd2)
	MDRV_NVRAM_HANDLER(atarigen)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(42*8, 30*8)
	MDRV_VISIBLE_AREA(0*8, 42*8-1, 0*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo2)
	MDRV_PALETTE_LENGTH(256)
	MDRV_COLORTABLE_LENGTH(256) /* can't make colortable_len = 0 because of 0xffff transparency kludge */

	MDRV_VIDEO_START(mm2)
	MDRV_VIDEO_UPDATE(mm2)

	/* sound hardware */
	MDRV_IMPORT_FROM(jsa_iii_mono )
MACHINE_DRIVER_END

/*************************************
 *
 *	ROM definition(s)
 *
 *************************************/

ROM_START( batman )
	ROM_REGION( 0xc0000, REGION_CPU1, 0 )	/* 6*128k for 68000 code */
	ROM_LOAD16_BYTE( "085-2030.10r",  0x00000, 0x20000, CRC(7cf4e5bf) SHA1(d6068a65fb524d839a34e596a272fac1ce90981c) )
	ROM_LOAD16_BYTE( "085-2031.7r",   0x00001, 0x20000, CRC(7d7f3fc4) SHA1(8ee3e9ad3464006a26c36155b6099433110e2a6e) )
	ROM_LOAD16_BYTE( "085-2032.91r",  0x40000, 0x20000, CRC(d80afb20) SHA1(5696627f6fa713dba4d12443c945f3e1cb6452a3) )
	ROM_LOAD16_BYTE( "085-2033.6r",   0x40001, 0x20000, CRC(97efa2b8) SHA1(782e263ca22356c1747c50aed158d8c459364ad8) )
	ROM_LOAD16_BYTE( "085-2034.9r",   0x80000, 0x20000, CRC(05388c62) SHA1(de037203d94e72e2922c89256da080ae023ca0e7) )
	ROM_LOAD16_BYTE( "085-2035.5r",   0x80001, 0x20000, CRC(e77c92dd) SHA1(6d475092f7628114960d26b8ec1c5eae5e61ce25) )

	ROM_REGION( 0x14000, REGION_CPU2, 0 )	/* 64k + 16k for 6502 code */
	ROM_LOAD( "085-1040.12c",  0x10000, 0x4000, CRC(080db83c) SHA1(ec084b7c1dc5878acd6d081e2e8b8d1e8b3d8a45) )
	ROM_CONTINUE(              0x04000, 0xc000 )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "085-2009.10m",  0x00000, 0x20000, CRC(a82d4923) SHA1(38e03eebd95347a383f3d7357462252961bd3c7f) )	/* alphanumerics */

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "085-1010.13r",  0x000000, 0x20000, CRC(466e1365) SHA1(318530e8a97c1b6ee3e671e677fc7684df5cc3a8) )	/* graphics, plane 0 */
	ROM_LOAD( "085-1014.14r",  0x020000, 0x20000, CRC(ef53475a) SHA1(9bfc66bb8dd02757e4a79a75928b260f4518a94b) )
	ROM_LOAD( "085-1011.13m",  0x040000, 0x20000, CRC(8cda5efc) SHA1(b0410f9bf1f38f5f1e9add15079b03d7f19b4c8f) )	/* graphics, plane 1 */
	ROM_LOAD( "085-1015.14m",  0x060000, 0x20000, CRC(043e7f8b) SHA1(a3b2c539c1fa9b1e8d3dc8163b9a7c6e22408122) )
	ROM_LOAD( "085-1012.13f",  0x080000, 0x20000, CRC(b017f2c3) SHA1(12846f0ae33e808dfb0795ea4138115b0eb36c4e) )	/* graphics, plane 2 */
	ROM_LOAD( "085-1016.14f",  0x0a0000, 0x20000, CRC(70aa2360) SHA1(5b944b533be40b859b7fae64559286034409ac6c) )
	ROM_LOAD( "085-1013.13c",  0x0c0000, 0x20000, CRC(68b64975) SHA1(f3fb45dd74fc21dd2eece87e739c734963962f93) )	/* graphics, plane 3 */
	ROM_LOAD( "085-1017.14c",  0x0e0000, 0x20000, CRC(e4af157b) SHA1(ddf9eff84c882a096f7e435a6227b32d31029f9e) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "085-1018.15r",  0x000000, 0x20000, CRC(4c14f1e5) SHA1(2a65d0aafd944886d9e801c9de0f857f2e9db773) )
	ROM_LOAD( "085-1022.16r",  0x020000, 0x20000, CRC(7476a15d) SHA1(779f9aec114aa71a268a7a646d998c1593f55d08) )
	ROM_LOAD( "085-1019.15m",  0x040000, 0x20000, CRC(2046d9ec) SHA1(3b14b18545eac2e6cb1d3157ec1af251287b3e45) )
	ROM_LOAD( "085-1023.16m",  0x060000, 0x20000, CRC(75cac686) SHA1(f3b1f485e51cc4af5809ec3fa2e7353fe9acb18f) )
	ROM_LOAD( "085-1020.15f",  0x080000, 0x20000, CRC(cc4f4b94) SHA1(b8a11dbe436496898c8b6d64163a3a92eb843086) )
	ROM_LOAD( "085-1024.16f",  0x0a0000, 0x20000, CRC(d60d35e0) SHA1(d74752090aec9fe0b7a67f62ae3024da74a004e3) )
	ROM_LOAD( "085-1021.15c",  0x0c0000, 0x20000, CRC(9c8ef9ba) SHA1(c2540adfc227a654a3f91e2cfdcd98b3a04ae4fb) )
	ROM_LOAD( "085-1025.16c",  0x0e0000, 0x20000, CRC(5d30bcd1) SHA1(817e225511ab98e7575ee512d659c51fcb7716dc) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* 1MB for ADPCM */
	ROM_LOAD( "085-1041.19e",  0x80000, 0x20000, CRC(d97d5dbb) SHA1(7609841c773e3d1ae5a21da81e3387260fd8da41) )
	ROM_LOAD( "085-1042.17e",  0xa0000, 0x20000, CRC(8c496986) SHA1(07c84c68885e2ab3e81ee92942d6a0f29e4dffa8) )
	ROM_LOAD( "085-1043.15e",  0xc0000, 0x20000, CRC(51812d3b) SHA1(6748fecef753179a9257c0da5a7b7c9648437208) )
	ROM_LOAD( "085-1044.12e",  0xe0000, 0x20000, CRC(5e2d7f31) SHA1(737c7204d91f5dd5c9ed0321fc6c0d6194a18f8a) )
ROM_END


ROM_START( marblmd2 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 6*128k for 68000 code */
	ROM_LOAD16_BYTE( "rom0l.18c",  0x00001, 0x20000, CRC(a4db40d9) SHA1(ae8313c9bb513143472347a7705bec33783bad7e) )
	ROM_LOAD16_BYTE( "rom0h.20c",  0x00000, 0x20000, CRC(d1a17d67) SHA1(7ac434858fa94e4bb4bb7d0603f699667494aa0d) )
	ROM_LOAD16_BYTE( "rom1l.18e",  0x40001, 0x20000, CRC(b6fb08b5) SHA1(0ce9c1a5d70133ffc879cfe548646c04de371e13) )
	ROM_LOAD16_BYTE( "rom1h.20e",  0x40000, 0x20000, CRC(b2a361a8) SHA1(b7c58404642a494532597cceb946463e3a6f56b3) )

	ROM_REGION( 0x14000, REGION_CPU2, 0 )	/* 64k + 16k for 6502 code */
	ROM_LOAD( "aud0.12c",  0x10000, 0x4000, CRC(89a8d90a) SHA1(cd73483d0bcfe2c8134d005c4417975f9a2cb658) )
	ROM_CONTINUE(          0x04000, 0xc000 )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "pf0l.3p",  0x00000, 0x20000, CRC(a4fe377a) SHA1(a8a1a8027da778e5ad406a65814eec999b0f81af) )
	ROM_LOAD( "pf1l.3m",  0x20000, 0x20000, CRC(5dc7aaa8) SHA1(4fb815e9bcf6bcdf1b7976a3dea2b6d1dd6a8f6b) )
	ROM_LOAD( "pf2l.3k",  0x40000, 0x20000, CRC(0c7c5f74) SHA1(26f1d36f70f4e8354537d0d67764a1c9be35e8f7) )
	ROM_LOAD( "pf3l.3j",  0x60000, 0x20000, CRC(0a780429) SHA1(a9d7d564507c31dafc448726b04293d6a582cff5) )
	ROM_LOAD( "pf0h.1p",  0x80000, 0x20000, CRC(a6297a83) SHA1(ffe9ea41d1ba7bb3d0260f3fcf0e970112098d46) )
	ROM_LOAD( "pf1h.1m",  0xa0000, 0x20000, CRC(5b40f1bb) SHA1(cf0de8679ab0dd9460324ce72b4bfac029591506) )
	ROM_LOAD( "pf2h.1k",  0xc0000, 0x20000, CRC(18323df9) SHA1(9c4add4733bcfe7202b53d86f1bca4b9d207e22a) )
	ROM_LOAD( "pf3h.1j",  0xe0000, 0x20000, CRC(05d86ef8) SHA1(47eefd7112a3a3be16f0b4496cf034c8f7a69b1b) )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mo0l.7p",  0x00000, 0x20000, CRC(950d95a3) SHA1(3f38da7b6eeaa87cc84b98c9d535468b0c060f6d) )
	ROM_LOAD( "mo1l.10p", 0x20000, 0x20000, CRC(b62b6ebf) SHA1(3781cd81780c10cd245871bb8f7b6260f7bb53b7) )
	ROM_LOAD( "mo0h.12p", 0x40000, 0x20000, CRC(e47d92b0) SHA1(7953e8342450c02408e4d90f132144d55de2f491) )
	ROM_LOAD( "mo1h.14p", 0x60000, 0x20000, CRC(317a03fb) SHA1(23a7cfe7c5601c858e8b346de31441788c7a8e97) )

	/* loading based on batman, there are 2 unpopulated positions on the PCB */
	ROM_REGION( 0x200000, REGION_SOUND1, 0 )	/* 1MB for ADPCM */
	ROM_LOAD( "sound.19e",  0x80000, 0x20000, CRC(e916bef7) SHA1(e07ddc8a3e1656d7307b767e692cf4a575ca47a3) )
	ROM_LOAD( "sound.12e",  0xe0000, 0x20000, CRC(bab2f8e5) SHA1(bbe2d693d40e5eeba315fe7b6380a2030b66f23e) )
ROM_END

/*************************************
 *
 *	Driver initialization
 *
 *************************************/

static DRIVER_INIT( batman )
{
	static const data16_t default_eeprom[] =
	{
		0x0001,0x01F1,0x0154,0x01C5,0x0100,0x0113,0x0300,0x0173,
		0x0700,0x0154,0x0200,0x0107,0x0100,0x0120,0x0300,0x0165,
		0x0125,0x0100,0x0149,0x019D,0x016C,0x018B,0x01F1,0x0154,
		0x01C5,0x0100,0x0113,0x0300,0x0173,0x0700,0x0154,0x0200,
		0x0107,0x0100,0x0120,0x0300,0x0165,0x0125,0x0100,0x0149,
		0x019D,0x016C,0x018B,0x6800,0x0134,0x0113,0x0148,0x0100,
		0x019A,0x0105,0x01DC,0x01A2,0x013A,0x0139,0x0100,0x0105,
		0x01AB,0x016A,0x0149,0x0100,0x01ED,0x0105,0x0185,0x01B2,
		0x0134,0x0100,0x0105,0x0160,0x01AA,0x0149,0x0100,0x0105,
		0x012A,0x0152,0x0110,0x0100,0x0168,0x0105,0x0113,0x012E,
		0x0150,0x0218,0x01D0,0x0100,0x01D0,0x0300,0x01D0,0x0600,
		0x01D0,0x02C8,0x0000
	};
	atarigen_eeprom_default = default_eeprom;
	atarijsa_init(1, 3, 2, 0x0040);
	atarijsa3_init_adpcm(REGION_SOUND1);
	atarigen_init_6502_speedup(1, 0x4163, 0x417b);
}


static DRIVER_INIT( marblmd2 )
{
	atarigen_eeprom_default = NULL;
	atarijsa_init(1, 5, 2, 0x0040);
	atarijsa3_init_adpcm(REGION_SOUND1);
}



/*************************************
 *
 *	Game driver(s)
 *
 *************************************/

GAME( 1991, batman,   0, batman,   batman,   batman,   ROT0, "Atari Games", "Batman" )
GAME( 1991, marblmd2, 0, marblmd2, marblmd2, marblmd2, ROT0, "Atari Games", "Marble Madness II (prototype)" )
