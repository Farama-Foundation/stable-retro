/***************************************************************************

Side Pocket - (c) 1986 Data East

The original board has an 8751 protection mcu

Ernesto Corvi
ernesto@imagina.com

Thanks must go to Mirko Buffoni for testing the music.

i8751 protection simluation and other fixes by Bryan McPhail, 15/10/00.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m6809/m6809.h"
#include "cpu/m6502/m6502.h"

/* from vidhrdw */
PALETTE_INIT( sidepckt );
VIDEO_START( sidepckt );
VIDEO_UPDATE( sidepckt );

WRITE_HANDLER( sidepckt_flipscreen_w );
WRITE_HANDLER( sidepckt_videoram_w );
WRITE_HANDLER( sidepckt_colorram_w );
static int i8751_return;


static WRITE_HANDLER( sound_cpu_command_w )
{
    soundlatch_w(offset,data);
    cpu_set_irq_line(1,IRQ_LINE_NMI,PULSE_LINE);
}

static READ_HANDLER( sidepckt_i8751_r )
{
	return i8751_return;
}

static WRITE_HANDLER( sidepckt_i8751_w )
{
	int table_1[]={5,3,2};
	int table_2[]={0x8e,0x42,0xad,0x58,0xec,0x85,0xdd,0x4c,0xad,0x9f,0x00,0x4c,0x7e,0x42,0xa2,0xff};
	int table_3[]={0xbd,0x73,0x80,0xbd,0x73,0xa7,0xbd,0x73,0xe0,0x7e,0x72,0x56,0xff,0xff,0xff,0xff};
	static int current_ptr=0,current_table=0,in_math=0,math_param;

	cpu_set_irq_line(0,M6809_FIRQ_LINE,HOLD_LINE); /* i8751 triggers FIRQ on main cpu */

	/* This function takes multiple parameters */
	if (in_math==1) {
		in_math=2;
		i8751_return=math_param=data;
	}
	else if (in_math==2) {
		in_math=0;
		i8751_return=math_param/data;
	}
	else switch (data) {
		case 1: /* ID Check */
			current_table=1; current_ptr=0; i8751_return=table_1[current_ptr++]; break;

		case 2: /* Protection data (executable code) */
			current_table=2; current_ptr=0; i8751_return=table_2[current_ptr++]; break;

		case 3: /* Protection data (executable code) */
			current_table=3; current_ptr=0; i8751_return=table_3[current_ptr++]; break;

		case 4: /* Divide function - multiple parameters */
			in_math=1;
			i8751_return=4;
			break;

		case 6: /* Read table data */
			if (current_table==1) i8751_return=table_1[current_ptr++];
			if (current_table==2) i8751_return=table_2[current_ptr++];
			if (current_table==3) i8751_return=table_3[current_ptr++];
			break;
	}
}

static WRITE_HANDLER( sidepctj_i8751_w )
{
	int table_1[]={5,3,0};
	int table_2[]={0x8e,0x42,0xb2,0x58,0xec,0x85,0xdd,0x4c,0xad,0x9f,0x00,0x4c,0x7e,0x42,0xa7,0xff};
	int table_3[]={0xbd,0x71,0xc8,0xbd,0x71,0xef,0xbd,0x72,0x28,0x7e,0x70,0x9e,0xff,0xff,0xff,0xff};
	static int current_ptr=0,current_table=0,in_math,math_param;

	cpu_set_irq_line(0,M6809_FIRQ_LINE,HOLD_LINE); /* i8751 triggers FIRQ on main cpu */

	/* This function takes multiple parameters */
	if (in_math==1) {
		in_math=2;
		i8751_return=math_param=data;
	}
	else if (in_math==2) {
		in_math=0;
		i8751_return=math_param/data;
	}
	else switch (data) {
		case 1: /* ID Check */
			current_table=1; current_ptr=0; i8751_return=table_1[current_ptr++]; break;

		case 2: /* Protection data */
			current_table=2; current_ptr=0; i8751_return=table_2[current_ptr++]; break;

		case 3: /* Protection data (executable code) */
			current_table=3; current_ptr=0; i8751_return=table_3[current_ptr++]; break;

		case 4: /* Divide function - multiple parameters */
			in_math=1;
			i8751_return=4;
			break;

		case 6: /* Read table data */
			if (current_table==1) i8751_return=table_1[current_ptr++];
			if (current_table==2) i8751_return=table_2[current_ptr++];
			if (current_table==3) i8751_return=table_3[current_ptr++];
			break;
	}
}

/******************************************************************************/

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x0fff, MRA_RAM },
	{ 0x1000, 0x17ff, videoram_r },
	{ 0x1800, 0x1fff, colorram_r },
	{ 0x2000, 0x20ff, MRA_RAM },
	{ 0x3000, 0x3000, input_port_0_r },
	{ 0x3001, 0x3001, input_port_1_r },
	{ 0x3002, 0x3002, input_port_2_r },
	{ 0x3003, 0x3003, input_port_3_r },
	{ 0x3014, 0x3014, sidepckt_i8751_r },
	{ 0x4000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x0fff, MWA_RAM },
	{ 0x1000, 0x13ff, sidepckt_videoram_w, &videoram, &videoram_size },
	{ 0x1800, 0x1bff, sidepckt_colorram_w, &colorram },
	{ 0x2000, 0x20ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x3004, 0x3004, sound_cpu_command_w },
	{ 0x300c, 0x300c, sidepckt_flipscreen_w },
	{ 0x3018, 0x3018, sidepckt_i8751_w },
	{ 0x4000, 0xffff, MWA_ROM },
MEMORY_END

static MEMORY_WRITE_START( j_writemem )
	{ 0x0000, 0x0fff, MWA_RAM },
	{ 0x1000, 0x13ff, sidepckt_videoram_w, &videoram, &videoram_size },
	{ 0x1800, 0x1bff, sidepckt_colorram_w, &colorram },
	{ 0x2000, 0x20ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x3004, 0x3004, sound_cpu_command_w },
	{ 0x300c, 0x300c, sidepckt_flipscreen_w },
	{ 0x3018, 0x3018, sidepctj_i8751_w },
	{ 0x4000, 0xffff, MWA_ROM },
MEMORY_END

static MEMORY_READ_START( sound_readmem )
    { 0x0000, 0x0fff, MRA_RAM },
    { 0x3000, 0x3000, soundlatch_r },
    { 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
    { 0x0000, 0x0fff, MWA_RAM },
    { 0x1000, 0x1000, YM2203_control_port_0_w },
    { 0x1001, 0x1001, YM2203_write_port_0_w },
    { 0x2000, 0x2000, YM3526_control_port_0_w },
    { 0x2001, 0x2001, YM3526_write_port_0_w },
    { 0x8000, 0xffff, MWA_ROM },
MEMORY_END

/******************************************************************************/

INPUT_PORTS_START( sidepckt )
    PORT_START /* 0x3000 */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

    PORT_START /* 0x3001 */
	/* I haven't found a way to make the game use the 2p controls */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

    PORT_START /* 0x3002 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x10, 0x10, "Unused?" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Unused?" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Unused?" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

    PORT_START /* 0x3003 */
	PORT_DIPNAME( 0x03, 0x03, "Timer Speed" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Stopped", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x03, "Slow" )
	PORT_DIPSETTING(    0x02, "Medium" )
	PORT_DIPSETTING(    0x01, "Fast" )
	PORT_DIPNAME( 0x0c, 0x08, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x0c, "6" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x10, "1" )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x30, "3" )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_DIPNAME( 0x80, 0x80, "Unused?" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
    8,8,    /* 8*8 characters */
    2048,   /* 2048 characters */
    3,      /* 3 bits per pixel */
    { 0, 0x8000*8, 0x10000*8 },     /* the bitplanes are separated */
    { 0, 1, 2, 3, 4, 5, 6, 7 },
    { 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
    8*8     /* every char takes 8 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
    16,16,  /* 16*16 sprites */
    1024,   /* 1024 sprites */
    3,      /* 3 bits per pixel */
    { 0, 0x8000*8, 0x10000*8 },     /* the bitplanes are separated */
    { 128+0, 128+1, 128+2, 128+3, 128+4, 128+5, 128+6, 128+7, 0, 1, 2, 3, 4, 5, 6, 7 },
    { 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
    32*8    /* every char takes 8 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,   128,  4 },	/* colors 128-159 */
	{ REGION_GFX2, 0, &spritelayout,   0, 16 },	/* colors   0-127 */
	{ -1 } /* end of array */
};



/* handler called by the 3526 emulator when the internal timers cause an IRQ */
static void irqhandler(int linestate)
{
	cpu_set_irq_line(1,0,linestate);
}

static struct YM2203interface ym2203_interface =
{
    1,      /* 1 chip */
    1500000,        /* 1.5 MHz */
    { YM2203_VOL(25,25) },
    { 0 },
    { 0 },
    { 0 },
    { 0 }
};

static struct YM3526interface ym3526_interface =
{
	1,			/* 1 chip */
	3000000,	/* 3 MHz */
	{ 100 },		/* volume */
	{ irqhandler},
};



static MACHINE_DRIVER_START( sidepckt )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6809, 2000000)        /* 2 MHz */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,1)

	MDRV_CPU_ADD(M6502, 1500000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)        /* 1.5 MHz */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
								/* NMIs are triggered by the main cpu */
	MDRV_FRAMES_PER_SECOND(58)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)  /* VERIFY:  May be 55 or 56 */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)

	MDRV_PALETTE_INIT(sidepckt)
	MDRV_VIDEO_START(sidepckt)
	MDRV_VIDEO_UPDATE(sidepckt)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, ym2203_interface)
	MDRV_SOUND_ADD(YM3526, ym3526_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( sidepctj )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6809, 2000000)        /* 2 MHz */
	MDRV_CPU_MEMORY(readmem,j_writemem)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,1)

	MDRV_CPU_ADD(M6502, 1500000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)        /* 1.5 MHz */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
								/* NMIs are triggered by the main cpu */
	MDRV_FRAMES_PER_SECOND(58)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)  /* VERIFY:  May be 55 or 56 */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)

	MDRV_PALETTE_INIT(sidepckt)
	MDRV_VIDEO_START(sidepckt)
	MDRV_VIDEO_UPDATE(sidepckt)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, ym2203_interface)
	MDRV_SOUND_ADD(YM3526, ym3526_interface)
MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( sidepckt )
    ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
    ROM_LOAD( "dh00",         0x00000, 0x10000, CRC(251b316e) SHA1(c777d87621b8fefe0e33156be03da8aed733db9a) )

    ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for the audio cpu */
    ROM_LOAD( "dh04.bin",     0x08000, 0x8000, CRC(d076e62e) SHA1(720ff1a6a58697b4a9c7c4f31c24a2cf8a04900a) )

    ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
    ROM_LOAD( "sp_07.bin",    0x00000, 0x8000, CRC(9d6f7969) SHA1(583852be0861a89c63ce09eb39146ec379b9e12d) ) /* characters */
    ROM_LOAD( "sp_06.bin",    0x08000, 0x8000, CRC(580e4e43) SHA1(de152a5d4fbc52d80e3eb9af17835ecb6258d45e) )
    ROM_LOAD( "sp_05.bin",    0x10000, 0x8000, CRC(05ab71d2) SHA1(6f06d1d1440a5fb05c01f712457d0bb167e93099) )

    ROM_REGION( 0x18000, REGION_GFX2, ROMREGION_DISPOSE )
    ROM_LOAD( "dh01.bin",     0x00000, 0x8000, CRC(a2cdfbea) SHA1(0721e538e3306d616f11008f784cf21e679f330d) ) /* sprites */
    ROM_LOAD( "dh02.bin",     0x08000, 0x8000, CRC(eeb5c3e7) SHA1(57eda1cc29124e04fe5025a904634d8ca52c0f12) )
    ROM_LOAD( "dh03.bin",     0x10000, 0x8000, CRC(8e18d21d) SHA1(74f0ddf1fcbed386332eba882b4136295b4f096d) )

    ROM_REGION( 0x0200, REGION_PROMS, 0 )	/* color PROMs */
    ROM_LOAD( "dh-09.bpr",    0x0000, 0x0100, CRC(ce049b4f) SHA1(e4918cef7b319dd40cf1722eb8bf5e79be04fd6c) )
    ROM_LOAD( "dh-08.bpr",    0x0100, 0x0100, CRC(cdf2180f) SHA1(123215d096f88b66396d40d7a579380d0b5b2b89) )
ROM_END

ROM_START( sidepctj )
    ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
    ROM_LOAD( "dh00.bin",     0x00000, 0x10000, CRC(a66bc28d) SHA1(cd62ce1dce6fe42d9745eec50d11e86b076d28e1) )

    ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for the audio cpu */
    ROM_LOAD( "dh04.bin",     0x08000, 0x8000, CRC(d076e62e) SHA1(720ff1a6a58697b4a9c7c4f31c24a2cf8a04900a) )

    ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
    ROM_LOAD( "dh07.bin",     0x00000, 0x8000, CRC(7d0ce858) SHA1(3a158f218a762e6841d2611f41ace67a1afefb35) ) /* characters */
    ROM_LOAD( "dh06.bin",     0x08000, 0x8000, CRC(b86ddf72) SHA1(7596dd1b646971d8df1bc4fd157ccf161a712d59) )
    ROM_LOAD( "dh05.bin",     0x10000, 0x8000, CRC(df6f94f2) SHA1(605796191f37cb76d496aa459243655070bb90c0) )

    ROM_REGION( 0x18000, REGION_GFX2, ROMREGION_DISPOSE )
    ROM_LOAD( "dh01.bin",     0x00000, 0x8000, CRC(a2cdfbea) SHA1(0721e538e3306d616f11008f784cf21e679f330d) ) /* sprites */
    ROM_LOAD( "dh02.bin",     0x08000, 0x8000, CRC(eeb5c3e7) SHA1(57eda1cc29124e04fe5025a904634d8ca52c0f12) )
    ROM_LOAD( "dh03.bin",     0x10000, 0x8000, CRC(8e18d21d) SHA1(74f0ddf1fcbed386332eba882b4136295b4f096d) )

    ROM_REGION( 0x0200, REGION_PROMS, 0 )	/* color PROMs */
    ROM_LOAD( "dh-09.bpr",    0x0000, 0x0100, CRC(ce049b4f) SHA1(e4918cef7b319dd40cf1722eb8bf5e79be04fd6c) )
    ROM_LOAD( "dh-08.bpr",    0x0100, 0x0100, CRC(cdf2180f) SHA1(123215d096f88b66396d40d7a579380d0b5b2b89) )
ROM_END

ROM_START( sidepctb )
    ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
    ROM_LOAD( "sp_09.bin",    0x04000, 0x4000, CRC(3c6fe54b) SHA1(4025ac48d75f171f4c979d3fcd6a2f8da18cef4f) )
    ROM_LOAD( "sp_08.bin",    0x08000, 0x8000, CRC(347f81cd) SHA1(5ab06130f35788e51a881cc0f387649532145bd6) )

    ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for the audio cpu */
    ROM_LOAD( "dh04.bin",     0x08000, 0x8000, CRC(d076e62e) SHA1(720ff1a6a58697b4a9c7c4f31c24a2cf8a04900a) )

    ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
    ROM_LOAD( "sp_07.bin",    0x00000, 0x8000, CRC(9d6f7969) SHA1(583852be0861a89c63ce09eb39146ec379b9e12d) ) /* characters */
    ROM_LOAD( "sp_06.bin",    0x08000, 0x8000, CRC(580e4e43) SHA1(de152a5d4fbc52d80e3eb9af17835ecb6258d45e) )
    ROM_LOAD( "sp_05.bin",    0x10000, 0x8000, CRC(05ab71d2) SHA1(6f06d1d1440a5fb05c01f712457d0bb167e93099) )

    ROM_REGION( 0x18000, REGION_GFX2, ROMREGION_DISPOSE )
    ROM_LOAD( "dh01.bin",     0x00000, 0x8000, CRC(a2cdfbea) SHA1(0721e538e3306d616f11008f784cf21e679f330d) ) /* sprites */
    ROM_LOAD( "dh02.bin",     0x08000, 0x8000, CRC(eeb5c3e7) SHA1(57eda1cc29124e04fe5025a904634d8ca52c0f12) )
    ROM_LOAD( "dh03.bin",     0x10000, 0x8000, CRC(8e18d21d) SHA1(74f0ddf1fcbed386332eba882b4136295b4f096d) )

    ROM_REGION( 0x0200, REGION_PROMS, 0 )	/* color PROMs */
    ROM_LOAD( "dh-09.bpr",    0x0000, 0x0100, CRC(ce049b4f) SHA1(e4918cef7b319dd40cf1722eb8bf5e79be04fd6c) )
    ROM_LOAD( "dh-08.bpr",    0x0100, 0x0100, CRC(cdf2180f) SHA1(123215d096f88b66396d40d7a579380d0b5b2b89) )
ROM_END



GAME( 1986, sidepckt, 0,        sidepckt, sidepckt, 0, ROT0, "Data East Corporation", "Side Pocket (World)" )
GAME( 1986, sidepctj, sidepckt, sidepctj, sidepckt, 0, ROT0, "Data East Corporation", "Side Pocket (Japan)" )
GAME( 1986, sidepctb, sidepckt, sidepckt, sidepckt, 0, ROT0, "bootleg", "Side Pocket (bootleg)" )
