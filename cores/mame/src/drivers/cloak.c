/***************************************************************************

	Atari Cloak & Dagger hardware

	Games supported:
		* Cloak & Dagger

****************************************************************************

	Master processor

	IRQ: 4 IRQ's per frame at even intervals, 4th IRQ is at start of VBLANK

	000-3FF	   Working RAM
	400-7FF    Playfield RAM
	800-FFF    Communication RAM (shared with slave processor)

	1000-100F  Pokey 1
	1008 (R)   bit 7 = Start 2 players
	   	   bit 6 = Start 1 player

	1800-180F  Pokey 2
	1808(R)	   Dipswitches

	2000 (R):  Joysticks
	2200 (R):  Player 2 joysticks (for cocktail version)

	2400 (R)   bit 0: Vertical Blank
		   bit 1: Self test switch
		   bit 2: Left Coin
		   bit 3: Right Coin
		   bit 4: Cocktail mode
		   bit 5: Aux Coin
		   bit 6: Player 2 Igniter button
		   bit 7: Player 1 Igniter button

	2600 (W) Custom Write (this has something to do with positioning of the display out, I ignore it)

	2800-29FF: (R/W) non-volatile RAM
	3000-30FF: (R/W) Motion RAM
	3200-327F: (W) Color RAM, Address bit 6 becomes the 9th bit of color RAM

	3800: (W) Right Coin Counter
	3801: (W) Left Coint Counter
	3803: (W) Cocktail Output
	3806: (W) Start 2 LED
	3807: (W) Start 1 LED

	3A00: (W) Watchdog reset
	3C00: (W) Reset IRQ
	3E00: (W) bit 0: Enable NVRAM

	4000 - FFFF ROM
		4000-5FFF  136023.501
		6000-7FFF  136023.502
		8000-BFFF  136023.503
		C000-FFFF  136023.504


	Slave processor

	IRQ: 1 IRQ per frame at start of VBLANK

	0000-0007: Working RAM
	0008-000A, 000C-000E: (R/W) bit 0,1,2: Store to/Read From Bit Map RAM

	0008: Decrement X/Increment Y
	0009: Decrement Y
	000A: Decrement X
	000B: Set bitmap X coordinate
	000C: Increment X/Increment Y  <-- Yes this is correct
	000D: Increment Y
	000E: Increment X
	000F: Set bitmap Y coordinate

	0010-07FF: Working RAM
	0800-0FFF: Communication RAM (shared with master processor)

	1000 (W): Reset IRQ
	1200 (W):  bit 0: Swap bit maps
		   bit 1: Clear bit map
	1400 (W): Custom Write (this has something to do with positioning of the display out, I ignore it)

	2000-FFFF: Program ROM
		2000-3FFF: 136023.509
		4000-5FFF: 136023.510
		6000-7FFF: 136023.511
		8000-9FFF: 136023.512
		A000-BFFF: 136023.513
		C000-DFFF: 136023.514
		E000-EFFF: 136023.515


	Motion object ROM: 136023.307,136023.308
	Playfield ROM: 136023.306,136023.305

****************************************************************************/

/*

	TODO:

	- slave com bad at startup
	- is bitmap drawing in service mode correct?
	- real cpu speeds
	- custom write

*/

#include "driver.h"
#include "vidhrdw/generic.h"

static UINT8 *cloak_sharedram;
static int cloak_nvram_enabled;

extern WRITE_HANDLER( cloak_videoram_w );
extern WRITE_HANDLER( cloak_paletteram_w );
extern WRITE_HANDLER( cloak_clearbmp_w );
extern WRITE_HANDLER( graph_processor_w );
extern WRITE_HANDLER( cloak_flipscreen_w );
extern READ_HANDLER( graph_processor_r );

extern VIDEO_START( cloak );
extern VIDEO_UPDATE( cloak );


/*************************************
 *
 *	Shared RAM I/O
 *
 *************************************/

static READ_HANDLER( cloak_sharedram_r )
{
	return cloak_sharedram[offset];
}

static WRITE_HANDLER( cloak_sharedram_w )
{
	cloak_sharedram[offset] = data;
}

/*************************************
 *
 *	Output ports
 *
 *************************************/

static WRITE_HANDLER( cloak_led_w )
{
	set_led_status(1 - offset, ~data & 0x80);
}

static WRITE_HANDLER( cloak_coin_counter_w )
{
	coin_counter_w(1 - offset, data & 0x80);
}

static WRITE_HANDLER( cloak_custom_w )
{
}

static WRITE_HANDLER( cloak_irq_reset_0_w )
{
	cpu_set_irq_line(0, data, CLEAR_LINE);
}

static WRITE_HANDLER( cloak_irq_reset_1_w )
{
	cpu_set_irq_line(1, data, CLEAR_LINE);
}

static WRITE_HANDLER( cloak_nvram_enable_w )
{
	cloak_nvram_enabled = data & 0x01;
}

/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x07ff, MRA_RAM },
	{ 0x0800, 0x0fff, cloak_sharedram_r },
	{ 0x2800, 0x29ff, MRA_RAM },
	{ 0x1000, 0x100f, pokey1_r },		/* DSW0 also */
/*	{ 0x1008, 0x1008, MRA_RAM },*/
	{ 0x1800, 0x180f, pokey2_r },		/* DSW1 also */
	{ 0x2000, 0x2000, input_port_0_r },	/* IN0 */
	{ 0x2200, 0x2200, input_port_1_r },	/* IN1 */
	{ 0x2400, 0x2400, input_port_2_r },	/* IN2 */
	{ 0x2800, 0x29ff, MRA_RAM },
	{ 0x3000, 0x30ff, MRA_RAM },
	{ 0x4000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x03ff, MWA_RAM },
	{ 0x0400, 0x07ff, cloak_videoram_w, &videoram },
	{ 0x0800, 0x0fff, cloak_sharedram_w, &cloak_sharedram },
	{ 0x1000, 0x100f, pokey1_w },
	{ 0x1800, 0x180f, pokey2_w },
	{ 0x2600, 0x2600, cloak_custom_w },
	{ 0x2800, 0x29ff, MWA_RAM, &generic_nvram, &generic_nvram_size },
	{ 0x3000, 0x30ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x3200, 0x327f, cloak_paletteram_w },
	{ 0x3800, 0x3801, cloak_coin_counter_w },
	{ 0x3803, 0x3803, cloak_flipscreen_w },
	{ 0x3805, 0x3805, MWA_NOP },	/* ???*/
	{ 0x3806, 0x3807, cloak_led_w },
	{ 0x3a00, 0x3a00, watchdog_reset_w },
	{ 0x3c00, 0x3c00, cloak_irq_reset_0_w },
	{ 0x3e00, 0x3e00, cloak_nvram_enable_w },
	{ 0x4000, 0xffff, MWA_ROM },
MEMORY_END

/*************************************
 *
 *	Slave CPU memory handlers
 *
 *************************************/

static MEMORY_READ_START( readmem2 )
	{ 0x0000, 0x0007, MRA_RAM },
	{ 0x0008, 0x000f, graph_processor_r },
	{ 0x0010, 0x07ff, MRA_RAM },
	{ 0x0800, 0x0fff, cloak_sharedram_r },
	{ 0x2000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem2 )
	{ 0x0000, 0x0007, MWA_RAM },
	{ 0x0008, 0x000f, graph_processor_w },
	{ 0x0010, 0x07ff, MWA_RAM },
	{ 0x0800, 0x0fff, cloak_sharedram_w },
	{ 0x1000, 0x1000, cloak_irq_reset_1_w },
	{ 0x1200, 0x1200, cloak_clearbmp_w },
	{ 0x1400, 0x1400, cloak_custom_w },
	{ 0x2000, 0xffff, MWA_ROM },
MEMORY_END

/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( cloak )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN  | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP    | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT  | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN   | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP     | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT  | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT   | IPF_8WAY )

	PORT_START	/* IN1 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )		/* player 2 controls, not used*/

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_SERVICE( 0x02, IP_ACTIVE_LOW )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )		/* cocktail mode switch, not used*/
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )		/* player 2 button 1, not used*/
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON5 )

	PORT_START	/* IN3 */
	PORT_BIT( 0x2f, IP_ACTIVE_LOW, IPT_UNUSED )		/* not connected*/
	PORT_BIT( 0x30, IP_ACTIVE_HIGH, IPT_UNUSED )	/* pulled high*/
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START1 )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x02, "Credits" )
	PORT_DIPSETTING(    0x02, "1 Credit/1 Game" )
	PORT_DIPSETTING(    0x01, "1 Credit/2 Games" )
	PORT_DIPSETTING(    0x03, "2 Credits/1 Game" )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_DIPNAME( 0x40, 0x00, "Demo Freeze Mode" )	/* when active, press button 1 to freeze*/
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

/*************************************
 *
 *	Graphics definitions
 *
 *************************************/

static struct GfxLayout charlayout =
{
	8,8,
	256,
	4,
	{ 0, 1, 2, 3 },
	{ 0x1000*8+0, 0x1000*8+4, 0, 4, 0x1000*8+8, 0x1000*8+12, 8, 12 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8
};

static struct GfxLayout spritelayout =
{
	8,16,
	128,
	4,
	{ 0, 1, 2, 3 },
	{ 0x1000*8+0, 0x1000*8+4, 0, 4, 0x1000*8+8, 0x1000*8+12, 8, 12 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
		8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	16*16
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,     0,  1 },
	{ REGION_GFX2, 0, &spritelayout,  32,  1 },
	{ -1 }
};

/*************************************
 *
 *	Sound interfaces
 *
 *************************************/

static struct POKEYinterface pokey_interface =
{
	2,	/* 2 chips */
	1500000,	/* 1.5 MHz??? */
	{ 50, 50 },
	/* The 8 pot handlers */
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	/* The allpot handler */
	{ input_port_3_r, input_port_4_r }
};

/*************************************
 *
 *	Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( cloak )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6502,1000000)		/* 1 MHz ???? */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,4)

	MDRV_CPU_ADD(M6502,1250000)		/* 1.25 MHz ???? */
	MDRV_CPU_MEMORY(readmem2,writemem2)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,2)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(5)

	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 3*8, 32*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(64)

	MDRV_VIDEO_START(cloak)
	MDRV_VIDEO_UPDATE(cloak)

	/* sound hardware */
	MDRV_SOUND_ADD(POKEY, pokey_interface)
MACHINE_DRIVER_END

/*************************************
 *
 *	ROM definitions
 *
 *************************************/

ROM_START( cloak )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "501.023",      0x4000, 0x2000, CRC(c2dbef1b) SHA1(3bab091afb846ea5f09200e3b44dc8dd430993fe) )
	ROM_LOAD( "502.023",      0x6000, 0x2000, CRC(316d0c7b) SHA1(58e50661c077415d9465d85c015b8238b4552304) )
	ROM_LOAD( "503.023",      0x8000, 0x4000, CRC(b9c291a6) SHA1(b3e310110c6d76fa11c44561eb8281aec5f2d1ae) )
	ROM_LOAD( "504.023",      0xc000, 0x4000, CRC(d014a1c0) SHA1(76c375ccbd0956779942a72ad2e3c3b8c6203ab3) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for code */
	ROM_LOAD( "509.023",      0x2000, 0x2000, CRC(46c021a4) SHA1(8ed7efca766611d433a4fec16ae9c05131a157f4) )
	ROM_LOAD( "510.023",      0x4000, 0x2000, CRC(8c9cf017) SHA1(19e404354418f95ed7079420fe51110f30e349ed) )
	ROM_LOAD( "511.023",      0x6000, 0x2000, CRC(66fd8a34) SHA1(9597a02a20113baea656ad5d581311abc2865fb1) )
	ROM_LOAD( "512.023",      0x8000, 0x2000, CRC(48c8079e) SHA1(b4b74115e58d67de2f50c1a6a39f34f116f0df29) )
	ROM_LOAD( "513.023",      0xa000, 0x2000, CRC(13f1cbab) SHA1(c016db6c0ca6d72ca8425d807d95f43dc87e6788) )
	ROM_LOAD( "514.023",      0xc000, 0x2000, CRC(6f8c7991) SHA1(bd6f838b224abed78fbdb7da17baa892289fc2f2) )
	ROM_LOAD( "515.023",      0xe000, 0x2000, CRC(835438a0) SHA1(27f320b74e7bdb29d4bc505432c96284f482cacc) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "105.023",      0x0000, 0x1000, CRC(ee443909) SHA1(802c5839be9e9e33c75ca7318043ecdb7b82f721) )
	ROM_LOAD( "106.023",      0x1000, 0x1000, CRC(d708b132) SHA1(d57acdcfb7b3de65f0162bdc041efff4c7eeff18) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "107.023",      0x0000, 0x1000, CRC(c42c84a4) SHA1(6f241e772f8b46c8d3acad2e967f1ab530886b11) )
	ROM_LOAD( "108.023",      0x1000, 0x1000, CRC(4fe13d58) SHA1(b21a32b2ff5363ab35fd1438344a04deb4077dbc) )
ROM_END


ROM_START( cloaksp )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "501.023",      0x4000, 0x2000, CRC(c2dbef1b) SHA1(3bab091afb846ea5f09200e3b44dc8dd430993fe) )
	ROM_LOAD( "502.023",      0x6000, 0x2000, CRC(316d0c7b) SHA1(58e50661c077415d9465d85c015b8238b4552304) )
	ROM_LOAD( "503.023",      0x8000, 0x4000, CRC(b9c291a6) SHA1(b3e310110c6d76fa11c44561eb8281aec5f2d1ae) )
	ROM_LOAD( "804.023",      0x00c000, 0x004000, CRC(994899c7) SHA1(3835777ce3f3aaff2ce3520c71d16d4c86743a20) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for code */
	ROM_LOAD( "509.023",      0x2000, 0x2000, CRC(46c021a4) SHA1(8ed7efca766611d433a4fec16ae9c05131a157f4) )
	ROM_LOAD( "510.023",      0x4000, 0x2000, CRC(8c9cf017) SHA1(19e404354418f95ed7079420fe51110f30e349ed) )
	ROM_LOAD( "511.023",      0x6000, 0x2000, CRC(66fd8a34) SHA1(9597a02a20113baea656ad5d581311abc2865fb1) )
	ROM_LOAD( "512.023",      0x8000, 0x2000, CRC(48c8079e) SHA1(b4b74115e58d67de2f50c1a6a39f34f116f0df29) )
	ROM_LOAD( "513.023",      0xa000, 0x2000, CRC(13f1cbab) SHA1(c016db6c0ca6d72ca8425d807d95f43dc87e6788) )
	ROM_LOAD( "514.023",      0xc000, 0x2000, CRC(6f8c7991) SHA1(bd6f838b224abed78fbdb7da17baa892289fc2f2) )
	ROM_LOAD( "515.023",      0xe000, 0x2000, CRC(835438a0) SHA1(27f320b74e7bdb29d4bc505432c96284f482cacc) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "105.023",      0x0000, 0x1000, CRC(ee443909) SHA1(802c5839be9e9e33c75ca7318043ecdb7b82f721) )
	ROM_LOAD( "106.023",      0x1000, 0x1000, CRC(d708b132) SHA1(d57acdcfb7b3de65f0162bdc041efff4c7eeff18) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "107.023",      0x0000, 0x1000, CRC(c42c84a4) SHA1(6f241e772f8b46c8d3acad2e967f1ab530886b11) )
	ROM_LOAD( "108.023",      0x1000, 0x1000, CRC(4fe13d58) SHA1(b21a32b2ff5363ab35fd1438344a04deb4077dbc) )
ROM_END


ROM_START( cloakfr )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "501.023",      0x4000, 0x2000, CRC(c2dbef1b) SHA1(3bab091afb846ea5f09200e3b44dc8dd430993fe) )
	ROM_LOAD( "502.023",      0x6000, 0x2000, CRC(316d0c7b) SHA1(58e50661c077415d9465d85c015b8238b4552304) )
	ROM_LOAD( "503.023",      0x8000, 0x4000, CRC(b9c291a6) SHA1(b3e310110c6d76fa11c44561eb8281aec5f2d1ae) )
	ROM_LOAD( "704.023",      0x00c000, 0x004000, CRC(bf225ea0) SHA1(d1e45d6071eb819e6a5075b61014e24a451fb443) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for code */
	ROM_LOAD( "509.023",      0x2000, 0x2000, CRC(46c021a4) SHA1(8ed7efca766611d433a4fec16ae9c05131a157f4) )
	ROM_LOAD( "510.023",      0x4000, 0x2000, CRC(8c9cf017) SHA1(19e404354418f95ed7079420fe51110f30e349ed) )
	ROM_LOAD( "511.023",      0x6000, 0x2000, CRC(66fd8a34) SHA1(9597a02a20113baea656ad5d581311abc2865fb1) )
	ROM_LOAD( "512.023",      0x8000, 0x2000, CRC(48c8079e) SHA1(b4b74115e58d67de2f50c1a6a39f34f116f0df29) )
	ROM_LOAD( "513.023",      0xa000, 0x2000, CRC(13f1cbab) SHA1(c016db6c0ca6d72ca8425d807d95f43dc87e6788) )
	ROM_LOAD( "514.023",      0xc000, 0x2000, CRC(6f8c7991) SHA1(bd6f838b224abed78fbdb7da17baa892289fc2f2) )
	ROM_LOAD( "515.023",      0xe000, 0x2000, CRC(835438a0) SHA1(27f320b74e7bdb29d4bc505432c96284f482cacc) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "105.023",      0x0000, 0x1000, CRC(ee443909) SHA1(802c5839be9e9e33c75ca7318043ecdb7b82f721) )
	ROM_LOAD( "106.023",      0x1000, 0x1000, CRC(d708b132) SHA1(d57acdcfb7b3de65f0162bdc041efff4c7eeff18) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "107.023",      0x0000, 0x1000, CRC(c42c84a4) SHA1(6f241e772f8b46c8d3acad2e967f1ab530886b11) )
	ROM_LOAD( "108.023",      0x1000, 0x1000, CRC(4fe13d58) SHA1(b21a32b2ff5363ab35fd1438344a04deb4077dbc) )
ROM_END


ROM_START( cloakgr )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "501.023",      0x4000, 0x2000, CRC(c2dbef1b) SHA1(3bab091afb846ea5f09200e3b44dc8dd430993fe) )
	ROM_LOAD( "502.023",      0x6000, 0x2000, CRC(316d0c7b) SHA1(58e50661c077415d9465d85c015b8238b4552304) )
	ROM_LOAD( "503.023",      0x8000, 0x4000, CRC(b9c291a6) SHA1(b3e310110c6d76fa11c44561eb8281aec5f2d1ae) )
	ROM_LOAD( "604.023",      0x00c000, 0x004000, CRC(7ac66aea) SHA1(fbeb3bb2756275aabb5fee1aa44c6f32da159bb6) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for code */
	ROM_LOAD( "509.023",      0x2000, 0x2000, CRC(46c021a4) SHA1(8ed7efca766611d433a4fec16ae9c05131a157f4) )
	ROM_LOAD( "510.023",      0x4000, 0x2000, CRC(8c9cf017) SHA1(19e404354418f95ed7079420fe51110f30e349ed) )
	ROM_LOAD( "511.023",      0x6000, 0x2000, CRC(66fd8a34) SHA1(9597a02a20113baea656ad5d581311abc2865fb1) )
	ROM_LOAD( "512.023",      0x8000, 0x2000, CRC(48c8079e) SHA1(b4b74115e58d67de2f50c1a6a39f34f116f0df29) )
	ROM_LOAD( "513.023",      0xa000, 0x2000, CRC(13f1cbab) SHA1(c016db6c0ca6d72ca8425d807d95f43dc87e6788) )
	ROM_LOAD( "514.023",      0xc000, 0x2000, CRC(6f8c7991) SHA1(bd6f838b224abed78fbdb7da17baa892289fc2f2) )
	ROM_LOAD( "515.023",      0xe000, 0x2000, CRC(835438a0) SHA1(27f320b74e7bdb29d4bc505432c96284f482cacc) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "105.023",      0x0000, 0x1000, CRC(ee443909) SHA1(802c5839be9e9e33c75ca7318043ecdb7b82f721) )
	ROM_LOAD( "106.023",      0x1000, 0x1000, CRC(d708b132) SHA1(d57acdcfb7b3de65f0162bdc041efff4c7eeff18) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "107.023",      0x0000, 0x1000, CRC(c42c84a4) SHA1(6f241e772f8b46c8d3acad2e967f1ab530886b11) )
	ROM_LOAD( "108.023",      0x1000, 0x1000, CRC(4fe13d58) SHA1(b21a32b2ff5363ab35fd1438344a04deb4077dbc) )
ROM_END



/*************************************
 *
 *	Game drivers
 *
 *************************************/

GAME( 1983, cloak,   0,     cloak, cloak, 0, ROT0, "Atari", "Cloak and Dagger (rev 5)" )
GAME( 1983, cloaksp, cloak, cloak, cloak, 0, ROT0, "Atari", "Cloak and Dagger (Spanish)" )
GAME( 1983, cloakfr, cloak, cloak, cloak, 0, ROT0, "Atari", "Cloak and Dagger (French)" )
GAME( 1983, cloakgr, cloak, cloak, cloak, 0, ROT0, "Atari", "Cloak and Dagger (German)" )
