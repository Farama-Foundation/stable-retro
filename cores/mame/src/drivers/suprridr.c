/***************************************************************************

	Taito Super Rider driver
	
	driver by Aaron Giles
	
	Games supported:
		* Super Rider

	Known issues:
		* clocks on sound chips and CPU not verified yet
		* the board seems to contain a discrete sound portion
		
	Questions:
		* there appears to be a second color bank for the tilemaps, where
		  is it used, and how is it activated (if at all)?
		* what are the writes to $08DB and $08E8 for?
		  (guess: a discrete sound effect)
  
****************************************************************************

	PCB Layout
	----------

	Top board

	REF. SR-8327A-B
	|----------------------------------------------------------------|
	|                 SR-11                                          |
	|                                                                |
	|          DIPSW  SR-10                                          |
	|                                                                |
	|                                                                |
	|                                                                |
	|                 NE555                                          |
	|                                     NEC-D780C                  |
	|  AY-3-8910                                                     |
	|                                                                |
	|  AY-3-8910                                                     |
	|                 SR-09           HM6116   SR-06    SR-03        |
	|  NEC-D780C                                                     |
	|                                 SR-08    SR-05    SR-02        |
	|                                                                |
	|                       ?.000MHz  SR-07    SR-04    SR-01        |
	|----------------------------------------------------------------|
	      

	Bottom board

	REF. SR-8327B-B
	|----------------------------------------------------------------|
	|                                                                |
	|    SR-12                                                       |
	|                                                                |
	|    SR-13                                                       |
	|                                                                |
	|    SR-14                                                       |
	|                                        SR-15                   |
	|                                                                |
	|                                        SR-16                   |
	|                                                                |
	|                                        SR-17                   |
	|                                                                |
	|                                       M58725P                  |
	|                                                                |
	|                                                                |
	|    18.432MHz                                                   |
	|----------------------------------------------------------------|
	
	
	Epoxy module (exact layout unknown)
	
	REF. ???
	|-------------------------------|
	|                               |
	|      1        2        3      |
	|                               |
	|-------------------------------|

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "suprridr.h"

static UINT8 nmi_enable;
static UINT8 sound_data;



/*************************************
 *
 *	Interrupt generation
 *
 *************************************/

static WRITE_HANDLER( nmi_enable_w )
{
	nmi_enable = data;
}


static INTERRUPT_GEN( main_nmi_gen )
{
	if (nmi_enable)
		cpu_set_irq_line(0, IRQ_LINE_NMI, PULSE_LINE);
}



/*************************************
 *
 *	Sound CPU communication
 *
 *************************************/

static void delayed_sound_w(int param)
{
	sound_data = param;
	cpu_set_irq_line(1, 0, ASSERT_LINE);
}


static WRITE_HANDLER( sound_data_w )
{
	timer_set(TIME_NOW, data, delayed_sound_w);
}


static READ_HANDLER( sound_data_r )
{
	return sound_data;
}


static WRITE_HANDLER( sound_irq_ack_w )
{
	cpu_set_irq_line(1, 0, CLEAR_LINE);
}



/*************************************
 *
 *	Misc handlers
 *
 *************************************/

static WRITE_HANDLER( coin_lock_w )
{
	/* cleared when 9 credits are hit, but never reset! */
/*	coin_lockout_global_w(~data & 1); */
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ_START( main_readmem )
    { 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0x8800, 0x8bff, MRA_RAM },
	{ 0x9000, 0x97ff, MRA_RAM },
	{ 0x9800, 0x983f, MRA_RAM },
	{ 0x9840, 0x987f, MRA_RAM },
	{ 0x9880, 0x9bff, MRA_RAM },
	{ 0xa000, 0xa000, input_port_0_r },
	{ 0xa800, 0xa800, input_port_1_r },
	{ 0xb000, 0xb000, input_port_2_r },
	{ 0xc000, 0xefff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( main_writemem )
    { 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x8800, 0x8bff, suprridr_bgram_w, &suprridr_bgram },
	{ 0x9000, 0x97ff, suprridr_fgram_w, &suprridr_fgram },
	{ 0x9800, 0x983f, MWA_RAM },
	{ 0x9840, 0x987f, MWA_RAM, &spriteram },
	{ 0x9880, 0x9bff, MWA_RAM },
	{ 0xb000, 0xb000, nmi_enable_w },
	{ 0xb002, 0xb003, coin_lock_w },
	{ 0xb006, 0xb006, suprridr_flipx_w },
	{ 0xb007, 0xb007, suprridr_flipy_w },
	{ 0xb800, 0xb800, sound_data_w },
	{ 0xc801, 0xc801, suprridr_fgdisable_w },
	{ 0xc802, 0xc802, suprridr_fgscrolly_w },
	{ 0xc804, 0xc804, suprridr_bgscrolly_w },
	{ 0xc000, 0xefff, MWA_ROM },
MEMORY_END


static PORT_READ_START( main_readport )
    { 0x00, 0x00, watchdog_reset_r },
PORT_END



/*************************************
 *
 *	Sound CPU memory handlers
 *
 *************************************/

static MEMORY_READ_START( sound_readmem )
    { 0x0000, 0x0fff, MRA_ROM },
	{ 0x3800, 0x3bff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
    { 0x0000, 0x0fff, MWA_ROM },
	{ 0x3800, 0x3bff, MWA_RAM },
MEMORY_END


static PORT_READ_START( sound_readport )
    { 0x8d, 0x8d, AY8910_read_port_0_r },
	{ 0x8f, 0x8f, AY8910_read_port_1_r },
PORT_END

static PORT_WRITE_START( sound_writeport )
    { 0x00, 0x00, sound_irq_ack_w },
	{ 0x8c, 0x8c, AY8910_control_port_0_w },
	{ 0x8d, 0x8d, AY8910_write_port_0_w },
	{ 0x8e, 0x8e, AY8910_control_port_1_w },
	{ 0x8f, 0x8f, AY8910_write_port_1_w },
PORT_END




/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( suprridr )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x0c, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0xc0, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START		/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START		/* DSW0 */
	PORT_DIPNAME( 0x07, 0x01, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_7C ) )
	PORT_DIPNAME( 0x18, 0x00, "Timer Speed" )
	PORT_DIPSETTING(	0x18, "Slow" )
	PORT_DIPSETTING(	0x10, "Medium" )
	PORT_DIPSETTING(	0x08, "Fast" )
	PORT_DIPSETTING(	0x00, "Fastest" )
	PORT_DIPNAME( 0x20, 0x00, "Bonus" )
	PORT_DIPSETTING(	0x00, "200k" )
	PORT_DIPSETTING(	0x20, "400k" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x00, "Invulnerability?" )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )
INPUT_PORTS_END



/*************************************
 *
 *	Graphics layouts
 *
 *************************************/

static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2),RGN_FRAC(1,2)+4, 0,4 },
	{ STEP4(0,1), STEP4(8,1) },
	{ STEP8(0,16) },
	8*8*2
};


static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(0,3), RGN_FRAC(1,3), RGN_FRAC(2,3) },
	{ STEP8(0,1), STEP8(8*8,1) },
	{ STEP8(0,8), STEP8(8*8*2,8) },
	16*16
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,    0, 2 },
	{ REGION_GFX2, 0, &charlayout,   32, 2 },
	{ REGION_GFX3, 0, &spritelayout, 64, 2 },
	{ -1 }
};



/*************************************
 *
 *	Sound interfaces
 *
 *************************************/

static struct AY8910interface ay8910_interface =
{
	2,
	10000000/8,		/* just a guess */
	{ 25, 25 },
	{ 0, sound_data_r },
	{ 0 },
	{ 0 },
	{ 0 }
};



/*************************************
 *
 *	Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( suprridr )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 10000000/4)		/* just a guess */
	MDRV_CPU_MEMORY(main_readmem, main_writemem)
	MDRV_CPU_PORTS(main_readport,0)
	MDRV_CPU_VBLANK_INT(main_nmi_gen,1)
	
	MDRV_CPU_ADD(Z80, 10000000/4)		/* just a guess */
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem, sound_writemem)
	MDRV_CPU_PORTS(sound_readport, sound_writeport)
	
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	
	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(96)

	MDRV_PALETTE_INIT(suprridr)
	MDRV_VIDEO_START(suprridr)
	MDRV_VIDEO_UPDATE(suprridr)
	
	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definitions
 *
 *************************************/

ROM_START( suprridr )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "sr8",    0x0000, 0x1000, CRC(4a1f0a6c) SHA1(cabdeafa3b9828d7a6e056fb037abb90484bb33a) )
	ROM_LOAD( "sr7",    0x1000, 0x1000, CRC(523ee717) SHA1(dd2a53a56b0f29b4d02c4207a7260b345cab0074) )
	ROM_LOAD( "sr4",    0x2000, 0x1000, CRC(300370ae) SHA1(bf43d800e1b2a5353625c1012d22df6419292d7d) )
	ROM_LOAD( "sr5",    0x3000, 0x1000, CRC(c5bca683) SHA1(4ebb1eb9dc72128286d60fce8b5c323adb25d332) )
	ROM_LOAD( "sr6",    0x4000, 0x1000, CRC(563bab28) SHA1(47dd5de9826360ccdf2df6866b0799a0390dd939) )
	ROM_LOAD( "sr3",    0x5000, 0x1000, CRC(4b9d2ec5) SHA1(773d53be5a3797c6c16ea8260f03c8e8272b2c32) )
	ROM_LOAD( "sr2",    0x6000, 0x1000, CRC(6fe18e1d) SHA1(9b247d2ab7bfddaa3cfdb5f034100881317e09a8) )
	ROM_LOAD( "sr1",    0x7000, 0x1000, CRC(f2ae64b3) SHA1(fd1878c7f1554e257a190084950a3bcf4b68a28e) )
	ROM_LOAD( "1",      0xc000, 0x1000, CRC(caf12fa2) SHA1(ff3f68cfb7817841cff1de6f78c9ee3d57b12db6) )
	ROM_LOAD( "2",      0xd000, 0x1000, CRC(2b3c638e) SHA1(af397cc9137888ccc503aff1b3554744a2327a4c) )
	ROM_LOAD( "3",      0xe000, 0x1000, CRC(2abdb5f4) SHA1(3003b3f5e70712339bf0d88e45ca0dd7ca8cf7d0) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "sr9",    0x0000, 0x1000, CRC(1c5dba78) SHA1(c2232221ae9960295055fcf1bd75d798136e694c) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "sr10",   0x0000, 0x1000, CRC(a57ac8d0) SHA1(1d4424dcbecb75b0e3e4ef5d296e252e7e9056ff) )
	ROM_LOAD( "sr11",   0x1000, 0x1000, CRC(aa7ec7b2) SHA1(bbc6a1022c15ffbf0f6f9828674c8c9947e7ea5a) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "sr15",   0x0000, 0x1000, CRC(744f3405) SHA1(4df5932e15e68ba10f8b13ed5a59cc7d54af7b80) )
	ROM_LOAD( "sr16",   0x1000, 0x1000, CRC(3e1a876b) SHA1(15b1c40c4a6e8e3e4702699396ce0885027ab6d1) )

	ROM_REGION( 0x3000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "sr12",   0x0000, 0x1000, CRC(81494fe8) SHA1(056de41952e6fd564ecc0ecb718caf467c03bfed) )
	ROM_LOAD( "sr13",   0x1000, 0x1000, CRC(63e94648) SHA1(05fdd285f6040aa349082845fcadd6bfbd2da2f5) )
	ROM_LOAD( "sr14",   0x2000, 0x1000, CRC(277a70af) SHA1(2235b369f1a30443f058bfe895b0d2dd294b587c) )

	ROM_REGION( 0x0060, REGION_PROMS, ROMREGION_DISPOSE )
	ROM_LOAD( "clr.1b", 0x0000, 0x0020, CRC(87a79fe8) SHA1(b0e982cfd7c2c8669841cf26625cd0912f4038f3) )
	ROM_LOAD( "clr.9c", 0x0020, 0x0020, CRC(10d63240) SHA1(74b1c53dacb5d30cd4cf189dda6b452d88dd22f3) )
	ROM_LOAD( "clr.8a", 0x0040, 0x0020, CRC(917eabcd) SHA1(df417ca42a4e9e7d32b443e73efaaf395f31e44a) )
ROM_END



/*************************************
 *
 *	Game drivers
 *
 *************************************/

GAMEX( 1983, suprridr, 0, suprridr, suprridr, 0, ROT90, "Venture Line (Taito Corporation license)", "Super Rider", GAME_IMPERFECT_SOUND )
