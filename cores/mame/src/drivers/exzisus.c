/***************************************************************************

Exzisus
-------------------------------------
driver by Yochizo

This driver is heavily dependent on the Raine source.
Very thanks to Richard Bush and the Raine team.


Supported games :
==================
 Exzisus        (C) 1987 Taito


System specs :
===============
   CPU       : Z80(4 MHz) x 4
   Sound     : YM2151 x 1
   Chips     : TC0010VCU + TC0140SYT


Known issues :
===============
 - Dip switches are not known very much.
 - Very slow due to four Z80s.

****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/taitosnd.h"


/***************************************************************************

  Variables

***************************************************************************/

static UINT8 *exzisus_sharedram_ac;
static UINT8 *exzisus_sharedram_bc;
static int exzisus_cpua_bank = 0;
static int exzisus_cpub_bank = 0;

extern UINT8 *exzisus_videoram0;
extern UINT8 *exzisus_videoram1;
extern UINT8 *exzisus_objectram0;
extern UINT8 *exzisus_objectram1;
extern size_t  exzisus_objectram_size0;
extern size_t  exzisus_objectram_size1;

READ_HANDLER ( exzisus_videoram_0_r );
READ_HANDLER ( exzisus_videoram_1_r );
READ_HANDLER ( exzisus_objectram_0_r );
READ_HANDLER ( exzisus_objectram_1_r );
WRITE_HANDLER( exzisus_videoram_0_w );
WRITE_HANDLER( exzisus_videoram_1_w );
WRITE_HANDLER( exzisus_objectram_0_w );
WRITE_HANDLER( exzisus_objectram_1_w );

VIDEO_UPDATE( exzisus );


/***************************************************************************

  Memory Handler(s)

***************************************************************************/

static WRITE_HANDLER( exzisus_cpua_bankswitch_w )
{
	UINT8 *RAM = memory_region(REGION_CPU1);

	if ( (data & 0x0f) != exzisus_cpua_bank )
	{
		exzisus_cpua_bank = data & 0x0f;
		if (exzisus_cpua_bank >= 2)
		{
			cpu_setbank( 1, &RAM[ 0x10000 + ( (exzisus_cpua_bank - 2) * 0x4000 ) ] );
		}
	}

	flip_screen_set(data & 0x40);
}

static WRITE_HANDLER( exzisus_cpub_bankswitch_w )
{
	UINT8 *RAM = memory_region(REGION_CPU4);

	if ( (data & 0x0f) != exzisus_cpub_bank )
	{
		exzisus_cpub_bank = data & 0x0f;
		if (exzisus_cpub_bank >= 2)
		{
			cpu_setbank( 2, &RAM[ 0x10000 + ( (exzisus_cpub_bank - 2) * 0x4000 ) ] );
		}
	}

	flip_screen_set(data & 0x40);
}

static WRITE_HANDLER( exzisus_coincounter_w )
{
	coin_lockout_w(0,~data & 0x01);
	coin_lockout_w(1,~data & 0x02);
	coin_counter_w(0,data & 0x04);
	coin_counter_w(1,data & 0x08);
}

static READ_HANDLER( exzisus_sharedram_ac_r )
{
	return exzisus_sharedram_ac[offset];
}

static READ_HANDLER( exzisus_sharedram_bc_r )
{
	return exzisus_sharedram_bc[offset];
}

static WRITE_HANDLER( exzisus_sharedram_ac_w )
{
	exzisus_sharedram_ac[offset] = data;
}

static WRITE_HANDLER( exzisus_sharedram_bc_w )
{
	exzisus_sharedram_bc[offset] = data;
}


/**************************************************************************

  Memory Map(s)

**************************************************************************/

static DRIVER_INIT( exzisus )
{
	UINT8 *RAM = memory_region(REGION_CPU4);

	/* Fix ROM 1 error */
	RAM[0x6829] = 0x18;

	/* Fix WORK RAM error */
	RAM[0x67fd] = 0x18;
}

static void irqhandler(int irq)
{
	cpu_set_irq_line(1, 0, irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	4000000,	/* 4 MHz ? */
	{ YM3012_VOL(50,MIXER_PAN_CENTER,50,MIXER_PAN_CENTER) },
	{ irqhandler },
};

static MEMORY_READ_START( cpua_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xc5ff, exzisus_objectram_0_r },
	{ 0xc600, 0xdfff, exzisus_videoram_0_r },
	{ 0xe000, 0xefff, MRA_RAM },
	{ 0xf000, 0xf000, MRA_NOP },
	{ 0xf001, 0xf001, taitosound_comm_r },
	{ 0xf400, 0xf400, input_port_0_r },
	{ 0xf401, 0xf401, input_port_1_r },
	{ 0xf402, 0xf402, input_port_2_r },
	{ 0xf404, 0xf404, input_port_3_r },
	{ 0xf405, 0xf405, input_port_4_r },
	{ 0xf800, 0xffff, exzisus_sharedram_ac_r },
MEMORY_END

static MEMORY_WRITE_START( cpua_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc5ff, exzisus_objectram_0_w, &exzisus_objectram0, &exzisus_objectram_size0 },
	{ 0xc600, 0xdfff, exzisus_videoram_0_w, &exzisus_videoram0 },
	{ 0xe000, 0xefff, MWA_RAM },
	{ 0xf000, 0xf000, taitosound_port_w },
	{ 0xf001, 0xf001, taitosound_comm_w },
	{ 0xf400, 0xf400, exzisus_cpua_bankswitch_w },
	{ 0xf402, 0xf402, exzisus_coincounter_w },
	{ 0xf404, 0xf404, MWA_NOP }, /* ??*/
	{ 0xf800, 0xffff, exzisus_sharedram_ac_w },
MEMORY_END

static MEMORY_READ_START( cpub_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x85ff, exzisus_objectram_1_r },
	{ 0x8600, 0x9fff, exzisus_videoram_1_r },
	{ 0xa000, 0xafff, exzisus_sharedram_bc_r },
	{ 0xb000, 0xbfff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( cpub_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x85ff, exzisus_objectram_1_w },
	{ 0x8600, 0x9fff, exzisus_videoram_1_w },
	{ 0xa000, 0xafff, exzisus_sharedram_bc_w },
	{ 0xb000, 0xbfff, MWA_RAM },
MEMORY_END

static MEMORY_READ_START( cpuc_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK2 },
	{ 0xc000, 0xc5ff, exzisus_objectram_1_r },
	{ 0xc600, 0xdfff, exzisus_videoram_1_r },
	{ 0xe000, 0xefff, exzisus_sharedram_bc_r },
	{ 0xf800, 0xffff, exzisus_sharedram_ac_r },
MEMORY_END

static MEMORY_WRITE_START( cpuc_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc5ff, exzisus_objectram_1_w, &exzisus_objectram1, &exzisus_objectram_size1 },
	{ 0xc600, 0xdfff, exzisus_videoram_1_w, &exzisus_videoram1 },
	{ 0xe000, 0xefff, exzisus_sharedram_bc_w, &exzisus_sharedram_bc },
	{ 0xf400, 0xf400, exzisus_cpub_bankswitch_w },
	{ 0xf800, 0xffff, exzisus_sharedram_ac_w, &exzisus_sharedram_ac },
MEMORY_END

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x8fff, MRA_RAM },
	{ 0x9000, 0x9000, MRA_NOP },
	{ 0x9001, 0x9001, YM2151_status_port_0_r },
	{ 0xa000, 0xa000, MRA_NOP },
	{ 0xa001, 0xa001, taitosound_slave_comm_r },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x8fff, MWA_RAM },
	{ 0x9000, 0x9000, YM2151_register_port_0_w },
	{ 0x9001, 0x9001, YM2151_data_port_0_w },
	{ 0xa000, 0xa000, taitosound_slave_port_w },
	{ 0xa001, 0xa001, taitosound_slave_comm_w },
MEMORY_END


/***************************************************************************

  Input Port(s)

***************************************************************************/

#define EXZISUS_PLAYERS_INPUT( player ) \
	PORT_START \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | player ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | player ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | player ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | player ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | player ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | player )

#define TAITO_COINAGE_JAPAN_8 \
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) ) \
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) ) \
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) ) \
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) ) \
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

#define TAITO_DIFFICULTY_8 \
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) ) \
	PORT_DIPSETTING(    0x02, "Easy" ) \
	PORT_DIPSETTING(    0x03, "Medium" ) \
	PORT_DIPSETTING(    0x01, "Hard" ) \
	PORT_DIPSETTING(    0x00, "Hardest" )

INPUT_PORTS_START( exzisus )
	/* IN0 */
	EXZISUS_PLAYERS_INPUT( IPF_PLAYER1 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	/* IN1 */
	EXZISUS_PLAYERS_INPUT( IPF_PLAYER2 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	/* IN2 */
	PORT_START      /* System control (2) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_TILT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START  /* DSW 1 (3) */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04,	IP_ACTIVE_LOW )		/* Service Mode */
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	TAITO_COINAGE_JAPAN_8

	PORT_START  /* DSW 2 (4) */
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x08, "100k and every 150k" )
	PORT_DIPSETTING(    0x04, "150k" )
	PORT_DIPSETTING(    0x0c, "150k and every 200k" )
	PORT_DIPSETTING(    0x00, "200k" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END


/***************************************************************************

  Machine Driver(s)

***************************************************************************/

static struct GfxLayout charlayout =
{
	8, 8,
	8*2048,
	4,
	{ 0x40000*8, 0x40000*8+4, 0, 4 },
	{ 3, 2, 1, 0, 8+3, 8+2, 8+1, 8+0 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8
};

static struct GfxDecodeInfo exzisus_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,   0, 256 },
	{ REGION_GFX2, 0, &charlayout, 256, 256 },
	{ -1 } /* end of array */
};

static MACHINE_DRIVER_START( exzisus )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 6000000)			/* 6 MHz ??? */
	MDRV_CPU_MEMORY(cpua_readmem,cpua_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, 4000000)			/* 4 MHz ??? */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_CPU_ADD(Z80, 6000000)			/* 6 MHz ??? */
	MDRV_CPU_MEMORY(cpub_readmem,cpub_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, 6000000)			/* 6 MHz ??? */
	MDRV_CPU_MEMORY(cpuc_readmem,cpuc_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)	/* 10 CPU slices per frame - enough for the sound CPU to read all commands */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(exzisus_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_PALETTE_INIT(RRRR_GGGG_BBBB)
	MDRV_VIDEO_UPDATE(exzisus)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( exzisus )
	ROM_REGION( 0x48000, REGION_CPU1, 0 )     				/* Z80 CPU A */
	ROM_LOAD( "b23-11.bin", 0x00000, 0x08000, CRC(d6a79cef) SHA1(e2b56aa38c017b24b50f304b9fe49ee14006f9a4) )
	ROM_CONTINUE(           0x10000, 0x08000 )
	ROM_LOAD( "b12-12.bin", 0x18000, 0x10000, CRC(a662be67) SHA1(0643480d56d8ac020288db800a705dd5d0d3ad9f) )
	ROM_LOAD( "b12-13.bin", 0x28000, 0x10000, CRC(04a29633) SHA1(39476365241718f01f9630c12467cb24791a67e1) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     				/* Z80 for Sound */
	ROM_LOAD( "b23-14.bin",  0x00000, 0x08000, CRC(f7ca7df2) SHA1(6048d9341f0303546e447a76439e1927d14cdd57) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )     				/* Z80 CPU B */
	ROM_LOAD( "b23-13.bin",  0x00000, 0x08000, CRC(51110aa1) SHA1(34c2701625eb1987affad1efd19ff8c9971456ae) )

	ROM_REGION( 0x48000, REGION_CPU4, 0 ) 					/* Z80 CPU C */
	ROM_LOAD( "b23-10.bin", 0x00000, 0x08000, CRC(c80216fc) SHA1(7b952779c420be08573768f09bd65d0a188df024) )
	ROM_CONTINUE(           0x10000, 0x08000 )
	ROM_LOAD( "b23-12.bin", 0x18000, 0x10000, CRC(13637f54) SHA1(c175bc60120e32eec6ccca822fa497a42dd59823) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT )	/* BG 0 */
	ROM_LOAD( "b12-16.bin",  0x00000, 0x10000, CRC(6fec6acb) SHA1(2289c116d3f6093988a088d011f192dd4a99aa77) )
	ROM_LOAD( "b12-18.bin",  0x10000, 0x10000, CRC(64e358aa) SHA1(cd1a23458b1a2f9c8c8aea8086dc04e0f6cc6908) )
	ROM_LOAD( "b12-20.bin",  0x20000, 0x10000, CRC(87f52e89) SHA1(3f8530aca087fa2a32dc6dfbcfe2f86604ee3ca1) )
	ROM_LOAD( "b12-15.bin",  0x40000, 0x10000, CRC(d81107c8) SHA1(c024c9b7956de493687e1373318d4cd74b3555b2) )
	ROM_LOAD( "b12-17.bin",  0x50000, 0x10000, CRC(db1d5a6c) SHA1(c2e1b8d92c2b3b2ce775ed50ca4a37e84ed35a93) )
	ROM_LOAD( "b12-19.bin",  0x60000, 0x10000, CRC(772b2641) SHA1(35cc6d5a725f1817791e710afde992e64d14104f) )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_INVERT )	/* BG 1 */
	ROM_LOAD( "b23-06.bin",  0x00000, 0x10000, CRC(44f8f661) SHA1(d77160a89e45556cd9ce211d89c398e1086d8d92) )
	ROM_LOAD( "b23-08.bin",  0x10000, 0x10000, CRC(1ce498c1) SHA1(a9ce3de997089bd40c99bd89919b459c9f215fc8) )
	ROM_LOAD( "b23-07.bin",  0x40000, 0x10000, CRC(d7f6ec89) SHA1(e8da207ddaf46ceff870b45ecec0e89c499291b4) )
	ROM_LOAD( "b23-09.bin",  0x50000, 0x10000, CRC(6651617f) SHA1(6351a0b01589cb181b896285ade70e9dfcd799ec) )

	ROM_REGION( 0x00c00, REGION_PROMS, 0 )					/* PROMS */
	ROM_LOAD( "b23-04.bin",  0x00000, 0x00400, CRC(5042cffa) SHA1(c969748866a12681cf2dbf25a46da2c4e4f92313) )
	ROM_LOAD( "b23-03.bin",  0x00400, 0x00400, CRC(9458fd45) SHA1(7f7cdacf37bb6f15de1109fa73ba3c5fc88893d0) )
	ROM_LOAD( "b23-05.bin",  0x00800, 0x00400, CRC(87f0f69a) SHA1(37df6fd56245fab9beaabfd86fd8f95d7c42c2a5) )
ROM_END


/*  ( YEAR      NAME  PARENT  MACHINE    INPUT     INIT  MONITOR  COMPANY              FULLNAME ) */
GAME( 1987,  exzisus,      0, exzisus, exzisus, exzisus,    ROT0, "Taito Corporation", "Exzisus (Japan)" )
