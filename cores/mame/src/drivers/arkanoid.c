/***************************************************************************

	Arkanoid driver (Preliminary)


	Japanese version support cocktail mode (DSW #7), the others don't.

	Here are the versions we have:

	arkanoid	World version, probably an earlier revision
	arknoidu	USA version, probably a later revision; There has been code
			    inserted, NOT patched, so I don't think it's a bootleg
				The 68705 code for this one was not available; I made it up from
				the World version changing the level data pointer table.
	arknoiuo	USA version, probably an earlier revision
				ROM	a7510.bin should be identical to the real World one.
				(It only differs in the country byte from A75_11.ROM)
				This version works fine with the real MCU ROM
    arkatour    Tournament version
				The 68705 code for this one was not available; I made it up from
				the World version changing the level data pointer table.
	arknoidj	Japanese version with level selector.
				The 68705 code for this one was not available; I made it up from
				the World version changing the level data pointer table.
	arkbl2		Bootleg of the early Japanese version.
				The only difference is that the warning text has been replaced
				by "WAIT"
				ROM	E2.6F should be identical to the real Japanese one.
				(It only differs in the country byte from A75_11.ROM)
				This version works fine with the real MCU ROM
	arkatayt	Another bootleg of the early Japanese one, more heavily modified
	arkblock	Another bootleg of the early Japanese one, more heavily modified
	arkbloc2	Another bootleg
	arkbl3   	Another bootleg of the early Japanese one, more heavily modified
	paddle2   	Another bootleg of the early Japanese one, more heavily modified
	arkangc		Game Corporation bootleg with level selector


	Most if not all Arkanoid sets have a bug in their game code. It occurs on the
	final level where the player has to dodge falling objects. The bug resides in
	the collision detection routine which sometimes reads from unmapped addresses
	above $F000. For these addresses it is vital to read zero values, or else the
	player will die for no reason.


***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

extern WRITE_HANDLER( arkanoid_videoram_w );
extern VIDEO_START( arkanoid );
extern VIDEO_UPDATE( arkanoid );

extern MACHINE_INIT( arkanoid );

extern WRITE_HANDLER( arkanoid_d008_w );

extern READ_HANDLER( arkanoid_Z80_mcu_r );
extern WRITE_HANDLER( arkanoid_Z80_mcu_w );

extern READ_HANDLER( arkanoid_68705_portA_r );
extern WRITE_HANDLER( arkanoid_68705_portA_w );
extern WRITE_HANDLER( arkanoid_68705_ddrA_w );

extern READ_HANDLER( arkanoid_68705_portC_r );
extern WRITE_HANDLER( arkanoid_68705_portC_w );
extern WRITE_HANDLER( arkanoid_68705_ddrC_w );

extern READ_HANDLER( arkanoid_68705_input_0_r );
extern READ_HANDLER( arkanoid_input_2_r );

/*
Paddle 2 MCU simulation

TODO:
\-Fix crashes and level finishing.
\-Finish the level pointer table & check the real thing for true level pattern...
\-(track_kludge_r)Find a better way to handle the paddle inputs.
\-Code optimizations + add this into machine/arkanoid.c

Notes:
\-This game is an Arkanoid 1 bootleg but with level edited to match the Arkanoid 2 ones.
\-Returning the right values for commands 0x38,0xff and 0x8a gives the level that has to
be played,but I don't have any clue about the true level pattern used.Checking Arkanoid 2
doesn't help much BTW...
*/


static int paddle2_prot;

static READ_HANDLER( paddle2_prot_r )
{
	static UINT8 level_table_a[] =
	{
		0xf3,0xf7,0xf9,0xfb,0xfd,0xff,0xf5,0xe3, /* 1- 8*/
		0xe5,0xe7,0xe9,0xeb,0xed,0xef,0xf1,0xf7, /* 9-16*/
		0xf9,0xfb,0xfd,0xff,0x00,0x00,0x00,0x00, /*17-24*/
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00  /*25-32*/
	};
	static UINT8 level_table_b[] =
	{
		0x52,0x52,0x52,0x52,0x52,0x52,0x0e,0x0e, /* 1- 8*/
		0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,0x0e, /* 9-16*/
		0x0e,0x0e,0x0e,0x0e,0x00,0x00,0x00,0x00, /*17-24*/
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00  /*25-32*/
	};
	UINT8 *RAM = memory_region(REGION_CPU1);
/*	usrintf_showmessage("%04x: %02x",activecpu_get_pc(),paddle2_prot);*/

	switch (paddle2_prot)
	{
		case 0xc3: return 0x1d;
		case 0x24: return 0x9b;
		/* Level pointer table */
		case 0x38:
		if(RAM[0xed83] == 0)    return level_table_a[RAM[0xed72]];
		else					return RAM[0xed83];
		case 0xff:
		if(RAM[0xed83] == 0)	return level_table_b[RAM[0xed72]];
		else 					return RAM[0xed83];
		/* Guess this is used for building level 	   */
		/* pointers too,but I haven't tested yet...    */
		case 0x8a: return 0x0a;
		/* Goes into sub-routine $2050,controls level finishing(WRONG!!!) */
		case 0xe3:
		if(RAM[0xed83] != 0)	return 0xff;
		else					return 0;
		/* Gives BAD HW message otherwise */
		case 0x36: return 0x2d;
		case 0xf7: return 0;
		default: return paddle2_prot;
	}
}

static WRITE_HANDLER( paddle2_prot_w )
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "%04x: prot_w %02x\n",activecpu_get_pc(),data);
	paddle2_prot = data;
}

static READ_HANDLER( track_kludge_r )
{
	int track = readinputport(2);

	/* temp kludge,needed to get the right side of the screen */
	if(track < 0x44)
		return 0x23;
	return 0x03;
}

static MEMORY_READ_START( readmem )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xd001, 0xd001, AY8910_read_port_0_r },
	{ 0xd00c, 0xd00c, arkanoid_68705_input_0_r },  /* mainly an input port, with 2 bits from the 68705 */
	{ 0xd010, 0xd010, input_port_1_r },
	{ 0xd018, 0xd018, arkanoid_Z80_mcu_r },  /* input from the 68705 */
	{ 0xe000, 0xefff, MRA_RAM },
	{ 0xf000, 0xffff, MRA_NOP },	/* fixes instant death in final level */
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xd000, 0xd000, AY8910_control_port_0_w },
	{ 0xd001, 0xd001, AY8910_write_port_0_w },
	{ 0xd008, 0xd008, arkanoid_d008_w },	/* gfx bank, flip screen etc. */
	{ 0xd010, 0xd010, watchdog_reset_w },
	{ 0xd018, 0xd018, arkanoid_Z80_mcu_w }, /* output to the 68705 */
	{ 0xe000, 0xe7ff, arkanoid_videoram_w, &videoram },
	{ 0xe800, 0xe83f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xe840, 0xefff, MWA_RAM },
MEMORY_END

static MEMORY_READ_START( boot_readmem )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xd001, 0xd001, AY8910_read_port_0_r },
	{ 0xd00c, 0xd00c, input_port_0_r },
	{ 0xd010, 0xd010, input_port_1_r },
	{ 0xd018, 0xd018, arkanoid_input_2_r },
	{ 0xe000, 0xefff, MRA_RAM },
	{ 0xf000, 0xffff, MRA_NOP },	/* fixes instant death in final level */
MEMORY_END

static MEMORY_WRITE_START( boot_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xd000, 0xd000, AY8910_control_port_0_w },
	{ 0xd001, 0xd001, AY8910_write_port_0_w },
	{ 0xd008, 0xd008, arkanoid_d008_w },	/* gfx bank, flip screen etc. */
	{ 0xd010, 0xd010, watchdog_reset_w },
	{ 0xd018, 0xd018, MWA_NOP },
	{ 0xe000, 0xe7ff, arkanoid_videoram_w, &videoram },
	{ 0xe800, 0xe83f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xe840, 0xefff, MWA_RAM },
MEMORY_END


static MEMORY_READ_START( mcu_readmem )
	{ 0x0000, 0x0000, arkanoid_68705_portA_r },
	{ 0x0001, 0x0001, arkanoid_input_2_r },
	{ 0x0002, 0x0002, arkanoid_68705_portC_r },
	{ 0x0010, 0x007f, MRA_RAM },
	{ 0x0080, 0x07ff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( mcu_writemem )
	{ 0x0000, 0x0000, arkanoid_68705_portA_w },
	{ 0x0002, 0x0002, arkanoid_68705_portC_w },
	{ 0x0004, 0x0004, arkanoid_68705_ddrA_w },
	{ 0x0006, 0x0006, arkanoid_68705_ddrC_w },
	{ 0x0010, 0x007f, MWA_RAM },
	{ 0x0080, 0x07ff, MWA_ROM },
MEMORY_END


INPUT_PORTS_START( arkanoid )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SPECIAL )	/* input from the 68705, some bootlegs need it to be 1 */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* input from the 68705 */

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0xf8, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 - spinner Player 1 */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL, 30, 15, 0, 0)

	PORT_START      /* IN3 - spinner Player 2  */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_COCKTAIL, 30, 15, 0, 0)

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x01, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, "Easy" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x10, "20K, 60K and every 60K" )
	PORT_DIPSETTING(    0x00, "20K only" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x20, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )
INPUT_PORTS_END

/* These are the input ports of the real Japanese ROM set                        */
/* 'Block' uses the these ones as well.	The Tayto bootleg is different			 */
/*  in coinage and # of lives.                    								 */

INPUT_PORTS_START( arknoidj )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SPECIAL )	/* input from the 68705, some bootlegs need it to be 1 */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* input from the 68705 */

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0xf8, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 - spinner (multiplexed for player 1 and 2) */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL, 30, 15, 0, 0)

	PORT_START      /* IN3 - spinner Player 2  */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_COCKTAIL, 30, 15, 0, 0)

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x01, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, "Easy" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x10, "20K, 60K and every 60K" )
	PORT_DIPSETTING(    0x00, "20K only" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x20, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )
INPUT_PORTS_END

/* Is the same as arkanoij, but the Coinage,
  Lives and Bonus_Life dips are different */
INPUT_PORTS_START( arkatayt )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SPECIAL )	/* input from the 68705, some bootlegs need it to be 1 */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* input from the 68705 */

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0xf8, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 - spinner (multiplexed for player 1 and 2) */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL, 30, 15, 0, 0)

	PORT_START      /* IN3 - spinner Player 2  */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_COCKTAIL, 30, 15, 0, 0)

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x01, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, "Easy" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x10, "60K, 100K and every 60K" )
	PORT_DIPSETTING(    0x00, "60K only" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )
INPUT_PORTS_END


static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	4096,	/* 4096 characters */
	3,	/* 3 bits per pixel */
	{ 2*4096*8*8, 4096*8*8, 0 },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,  0, 64 },
	/* sprites use the same characters above, but are 16x8 */
	{ -1 } /* end of array */
};



static struct AY8910interface ay8910_interface =
{
	1,	/* 1 chips */
	1500000,	/* 1.5 MHz ???? */
	{ 33 },
	{ 0 },
	{ input_port_4_r },
	{ 0 },
	{ 0 }
};



static MACHINE_DRIVER_START( arkanoid )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 6000000)	/* 6 MHz ?? */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(M68705, 500000)	/* .5 MHz (don't know really how fast, but it doesn't need to even be this fast) */
	MDRV_CPU_MEMORY(mcu_readmem,mcu_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100) /* 100 CPU slices per second to synchronize between the MCU and the main CPU */

	MDRV_MACHINE_INIT(arkanoid)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_PALETTE_INIT(RRRR_GGGG_BBBB)
	MDRV_VIDEO_START(arkanoid)
	MDRV_VIDEO_UPDATE(arkanoid)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( bootleg )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 6000000)	/* 6 MHz ?? */
	MDRV_CPU_MEMORY(boot_readmem,boot_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_PALETTE_INIT(RRRR_GGGG_BBBB)
	MDRV_VIDEO_START(arkanoid)
	MDRV_VIDEO_UPDATE(arkanoid)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( arkanoid )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "a75_01-1.rom", 0x0000, 0x8000, CRC(5bcda3b0) SHA1(52cadd38b5f8e8856f007a9c602d6b508f30be65) )
	ROM_LOAD( "a75_11.rom",   0x8000, 0x8000, CRC(eafd7191) SHA1(d2f8843b716718b1de209e97a874e8ce600f3f87) )

	ROM_REGION( 0x0800, REGION_CPU2, 0 )	/* 8k for the microcontroller */
	ROM_LOAD( "arkanoid.uc",  0x0000, 0x0800, CRC(515d77b6) SHA1(a302937683d11f663abd56a2fd7c174374e4d7fb) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "a75_03.rom",   0x00000, 0x8000, CRC(038b74ba) SHA1(ac053cc4908b4075f918748b89570e07a0ba5116) )
	ROM_LOAD( "a75_04.rom",   0x08000, 0x8000, CRC(71fae199) SHA1(5d253c46ccf4cd2976a5fb8b8713f0f345443d06) )
	ROM_LOAD( "a75_05.rom",   0x10000, 0x8000, CRC(c76374e2) SHA1(7520dd48de20db60a2038f134dcaa454988e7874) )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "07.bpr",       0x0000, 0x0200, CRC(0af8b289) SHA1(6bc589e8a609b4cf450aebedc8ce02d5d45c970f) )	/* red component */
	ROM_LOAD( "08.bpr",       0x0200, 0x0200, CRC(abb002fb) SHA1(c14f56b8ef103600862e7930709d293b0aa97a73) )	/* green component */
	ROM_LOAD( "09.bpr",       0x0400, 0x0200, CRC(a7c6c277) SHA1(adaa003dcd981576ea1cc5f697d709b2d6b2ea29) )	/* blue component */
ROM_END

ROM_START( arknoidu )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "a75-19.bin",   0x0000, 0x8000, CRC(d3ad37d7) SHA1(a172a1ef5bb83ee2d8ed2842ef8968af19ad411e) )
	ROM_LOAD( "a75-18.bin",   0x8000, 0x8000, CRC(cdc08301) SHA1(05f54353cc8333af14fa985a2764960e20e8161a) )

	ROM_REGION( 0x0800, REGION_CPU2, 0 )	/* 8k for the microcontroller */
	ROM_LOAD( "arknoidu.uc",  0x0000, 0x0800, BAD_DUMP CRC(de518e47) SHA1(b8eddd1c566505fb69e3d1207c7a9720dfb9f503)  )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "a75_03.rom",   0x00000, 0x8000, CRC(038b74ba) SHA1(ac053cc4908b4075f918748b89570e07a0ba5116) )
	ROM_LOAD( "a75_04.rom",   0x08000, 0x8000, CRC(71fae199) SHA1(5d253c46ccf4cd2976a5fb8b8713f0f345443d06) )
	ROM_LOAD( "a75_05.rom",   0x10000, 0x8000, CRC(c76374e2) SHA1(7520dd48de20db60a2038f134dcaa454988e7874) )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "07.bpr",       0x0000, 0x0200, CRC(0af8b289) SHA1(6bc589e8a609b4cf450aebedc8ce02d5d45c970f) )	/* red component */
	ROM_LOAD( "08.bpr",       0x0200, 0x0200, CRC(abb002fb) SHA1(c14f56b8ef103600862e7930709d293b0aa97a73) )	/* green component */
	ROM_LOAD( "09.bpr",       0x0400, 0x0200, CRC(a7c6c277) SHA1(adaa003dcd981576ea1cc5f697d709b2d6b2ea29) )	/* blue component */
ROM_END

ROM_START( arknoiuo )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "a75_01-1.rom", 0x0000, 0x8000, CRC(5bcda3b0) SHA1(52cadd38b5f8e8856f007a9c602d6b508f30be65) )
	ROM_LOAD( "a7510.bin",    0x8000, 0x8000, CRC(a1769e15) SHA1(fbb45731246a098b29eb08de5d63074b496aaaba) )

	ROM_REGION( 0x0800, REGION_CPU2, 0 )	/* 8k for the microcontroller */
	ROM_LOAD( "arkanoid.uc",  0x0000, 0x0800, CRC(515d77b6) SHA1(a302937683d11f663abd56a2fd7c174374e4d7fb) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "a75_03.rom",   0x00000, 0x8000, CRC(038b74ba) SHA1(ac053cc4908b4075f918748b89570e07a0ba5116) )
	ROM_LOAD( "a75_04.rom",   0x08000, 0x8000, CRC(71fae199) SHA1(5d253c46ccf4cd2976a5fb8b8713f0f345443d06) )
	ROM_LOAD( "a75_05.rom",   0x10000, 0x8000, CRC(c76374e2) SHA1(7520dd48de20db60a2038f134dcaa454988e7874) )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "07.bpr",       0x0000, 0x0200, CRC(0af8b289) SHA1(6bc589e8a609b4cf450aebedc8ce02d5d45c970f) )	/* red component */
	ROM_LOAD( "08.bpr",       0x0200, 0x0200, CRC(abb002fb) SHA1(c14f56b8ef103600862e7930709d293b0aa97a73) )	/* green component */
	ROM_LOAD( "09.bpr",       0x0400, 0x0200, CRC(a7c6c277) SHA1(adaa003dcd981576ea1cc5f697d709b2d6b2ea29) )	/* blue component */
ROM_END

ROM_START( arkatour )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "t_ark1.bin",   0x0000, 0x8000, CRC(e3b8faf5) SHA1(4c09478fa41881fa89ee6afb676aeb780f17ac2e) )
	ROM_LOAD( "t_ark2.bin",   0x8000, 0x8000, CRC(326aca4d) SHA1(5a194b7a0361236d471b24905dc6434372f81252) )

	ROM_REGION( 0x0800, REGION_CPU2, 0 )	/* 8k for the microcontroller */
	ROM_LOAD( "arkatour.uc",  0x0000, 0x0800, BAD_DUMP CRC(d3249559) SHA1(b1542764450016614e9e03cedd6a2f1e59961789)  )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "t_ark3.bin",   0x00000, 0x8000, CRC(5ddea3cf) SHA1(58f16515898b7cc2697bf7663a60d9ca0db6da95) )
	ROM_LOAD( "t_ark4.bin",   0x08000, 0x8000, CRC(5fcf2e85) SHA1(f721f0afb0550cc64bff26681856a7576398d9b5) )
	ROM_LOAD( "t_ark5.bin",   0x10000, 0x8000, CRC(7b76b192) SHA1(a68aa08717646a6c322cf3455df07f50df9e9f33) )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "07.bpr",       0x0000, 0x0200, CRC(0af8b289) SHA1(6bc589e8a609b4cf450aebedc8ce02d5d45c970f) )	/* red component */
	ROM_LOAD( "08.bpr",       0x0200, 0x0200, CRC(abb002fb) SHA1(c14f56b8ef103600862e7930709d293b0aa97a73) )	/* green component */
	ROM_LOAD( "09.bpr",       0x0400, 0x0200, CRC(a7c6c277) SHA1(adaa003dcd981576ea1cc5f697d709b2d6b2ea29) )	/* blue component */
ROM_END

ROM_START( arknoidj )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "a75-21.rom",   0x0000, 0x8000, CRC(bf0455fc) SHA1(250522b84b9f491c3f4efc391bf6aa6124361369) )
	ROM_LOAD( "a75-22.rom",   0x8000, 0x8000, CRC(3a2688d3) SHA1(9633a661352def3d85f95ca830f6d761b0b5450e) )

	ROM_REGION( 0x0800, REGION_CPU2, 0 )	/* 8k for the microcontroller */
	ROM_LOAD( "arknoidj.uc",  0x0000, 0x0800, BAD_DUMP CRC(0a4abef6) SHA1(fdce0b7a2eab7fd4f1f4fc3b93120b1ebc16078e)  )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "a75_03.rom",   0x00000, 0x8000, CRC(038b74ba) SHA1(ac053cc4908b4075f918748b89570e07a0ba5116) )
	ROM_LOAD( "a75_04.rom",   0x08000, 0x8000, CRC(71fae199) SHA1(5d253c46ccf4cd2976a5fb8b8713f0f345443d06) )
	ROM_LOAD( "a75_05.rom",   0x10000, 0x8000, CRC(c76374e2) SHA1(7520dd48de20db60a2038f134dcaa454988e7874) )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "07.bpr",       0x0000, 0x0200, CRC(0af8b289) SHA1(6bc589e8a609b4cf450aebedc8ce02d5d45c970f) )	/* red component */
	ROM_LOAD( "08.bpr",       0x0200, 0x0200, CRC(abb002fb) SHA1(c14f56b8ef103600862e7930709d293b0aa97a73) )	/* green component */
	ROM_LOAD( "09.bpr",       0x0400, 0x0200, CRC(a7c6c277) SHA1(adaa003dcd981576ea1cc5f697d709b2d6b2ea29) )	/* blue component */
ROM_END

ROM_START( arkbl2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "e1.6d",        0x0000, 0x8000, CRC(dd4f2b72) SHA1(399a8636030a702dafc1da926f115df6f045bef1) )
	ROM_LOAD( "e2.6f",        0x8000, 0x8000, CRC(bbc33ceb) SHA1(e9b6fef98d0d20e77c7a1c25eff8e9a8c668a258) )

	ROM_REGION( 0x0800, REGION_CPU2, 0 )	/* 8k for the microcontroller */
	ROM_LOAD( "68705p3.6i",   0x0000, 0x0800, CRC(389a8cfb) SHA1(9530c051b61b5bdec7018c6fdc1ea91288a406bd) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "a75_03.rom",   0x00000, 0x8000, CRC(038b74ba) SHA1(ac053cc4908b4075f918748b89570e07a0ba5116) )
	ROM_LOAD( "a75_04.rom",   0x08000, 0x8000, CRC(71fae199) SHA1(5d253c46ccf4cd2976a5fb8b8713f0f345443d06) )
	ROM_LOAD( "a75_05.rom",   0x10000, 0x8000, CRC(c76374e2) SHA1(7520dd48de20db60a2038f134dcaa454988e7874) )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "07.bpr",       0x0000, 0x0200, CRC(0af8b289) SHA1(6bc589e8a609b4cf450aebedc8ce02d5d45c970f) )	/* red component */
	ROM_LOAD( "08.bpr",       0x0200, 0x0200, CRC(abb002fb) SHA1(c14f56b8ef103600862e7930709d293b0aa97a73) )	/* green component */
	ROM_LOAD( "09.bpr",       0x0400, 0x0200, CRC(a7c6c277) SHA1(adaa003dcd981576ea1cc5f697d709b2d6b2ea29) )	/* blue component */
ROM_END

ROM_START( arkbl3 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "arkanunk.1",   0x0000, 0x8000, CRC(b0f73900) SHA1(2c9a36cc1d2a3f33ec81d63c1c325554b818d2d3) )
	ROM_LOAD( "arkanunk.2",   0x8000, 0x8000, CRC(9827f297) SHA1(697874e73e045eb5a7bf333d7310934b239c0adf) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "a75_03.rom",   0x00000, 0x8000, CRC(038b74ba) SHA1(ac053cc4908b4075f918748b89570e07a0ba5116) )
	ROM_LOAD( "a75_04.rom",   0x08000, 0x8000, CRC(71fae199) SHA1(5d253c46ccf4cd2976a5fb8b8713f0f345443d06) )
	ROM_LOAD( "a75_05.rom",   0x10000, 0x8000, CRC(c76374e2) SHA1(7520dd48de20db60a2038f134dcaa454988e7874) )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "07.bpr",       0x0000, 0x0200, CRC(0af8b289) SHA1(6bc589e8a609b4cf450aebedc8ce02d5d45c970f) )	/* red component */
	ROM_LOAD( "08.bpr",       0x0200, 0x0200, CRC(abb002fb) SHA1(c14f56b8ef103600862e7930709d293b0aa97a73) )	/* green component */
	ROM_LOAD( "09.bpr",       0x0400, 0x0200, CRC(a7c6c277) SHA1(adaa003dcd981576ea1cc5f697d709b2d6b2ea29) )	/* blue component */
ROM_END

ROM_START( paddle2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "paddle2.16",   0x0000, 0x8000, CRC(a286333c) SHA1(0b2c9cb0df236f327413d0c541453e1ba979ea38) )
	ROM_LOAD( "paddle2.17",   0x8000, 0x8000, CRC(04c2acb5) SHA1(7ce8ba31224f705b2b6ed0200404ef5f8f688001) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "a75_03.rom",   0x00000, 0x8000, CRC(038b74ba) SHA1(ac053cc4908b4075f918748b89570e07a0ba5116) )
	ROM_LOAD( "a75_04.rom",   0x08000, 0x8000, CRC(71fae199) SHA1(5d253c46ccf4cd2976a5fb8b8713f0f345443d06) )
	ROM_LOAD( "a75_05.rom",   0x10000, 0x8000, CRC(c76374e2) SHA1(7520dd48de20db60a2038f134dcaa454988e7874) )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "07.bpr",       0x0000, 0x0200, CRC(0af8b289) SHA1(6bc589e8a609b4cf450aebedc8ce02d5d45c970f) )	/* red component */
	ROM_LOAD( "08.bpr",       0x0200, 0x0200, CRC(abb002fb) SHA1(c14f56b8ef103600862e7930709d293b0aa97a73) )	/* green component */
	ROM_LOAD( "09.bpr",       0x0400, 0x0200, CRC(a7c6c277) SHA1(adaa003dcd981576ea1cc5f697d709b2d6b2ea29) )	/* blue component */
ROM_END

ROM_START( arkatayt )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "arkanoid.1",   0x0000, 0x8000, CRC(6e0a2b6f) SHA1(5227d7a944cb1e815f60ec87a67f7462870ff9fe) )
	ROM_LOAD( "arkanoid.2",   0x8000, 0x8000, CRC(5a97dd56) SHA1(b71c7b5ced2b0eebbcc5996dd21a1bb1c2da4819) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "a75_03.rom",   0x00000, 0x8000, CRC(038b74ba) SHA1(ac053cc4908b4075f918748b89570e07a0ba5116) )
	ROM_LOAD( "a75_04.rom",   0x08000, 0x8000, CRC(71fae199) SHA1(5d253c46ccf4cd2976a5fb8b8713f0f345443d06) )
	ROM_LOAD( "a75_05.rom",   0x10000, 0x8000, CRC(c76374e2) SHA1(7520dd48de20db60a2038f134dcaa454988e7874) )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "07.bpr",       0x0000, 0x0200, CRC(0af8b289) SHA1(6bc589e8a609b4cf450aebedc8ce02d5d45c970f) )	/* red component */
	ROM_LOAD( "08.bpr",       0x0200, 0x0200, CRC(abb002fb) SHA1(c14f56b8ef103600862e7930709d293b0aa97a73) )	/* green component */
	ROM_LOAD( "09.bpr",       0x0400, 0x0200, CRC(a7c6c277) SHA1(adaa003dcd981576ea1cc5f697d709b2d6b2ea29) )	/* blue component */
ROM_END

ROM_START( arkblock )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "block01.bin",  0x0000, 0x8000, CRC(5be667e1) SHA1(fbc5c97d836c404a2e6c007c3836e36b52ae75a1) )
	ROM_LOAD( "block02.bin",  0x8000, 0x8000, CRC(4f883ef1) SHA1(cb090a57fc75f17a3e2ba637f0e3ec93c1d02cea) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "a75_03.rom",   0x00000, 0x8000, CRC(038b74ba) SHA1(ac053cc4908b4075f918748b89570e07a0ba5116) )
	ROM_LOAD( "a75_04.rom",   0x08000, 0x8000, CRC(71fae199) SHA1(5d253c46ccf4cd2976a5fb8b8713f0f345443d06) )
	ROM_LOAD( "a75_05.rom",   0x10000, 0x8000, CRC(c76374e2) SHA1(7520dd48de20db60a2038f134dcaa454988e7874) )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "07.bpr",       0x0000, 0x0200, CRC(0af8b289) SHA1(6bc589e8a609b4cf450aebedc8ce02d5d45c970f) )	/* red component */
	ROM_LOAD( "08.bpr",       0x0200, 0x0200, CRC(abb002fb) SHA1(c14f56b8ef103600862e7930709d293b0aa97a73) )	/* green component */
	ROM_LOAD( "09.bpr",       0x0400, 0x0200, CRC(a7c6c277) SHA1(adaa003dcd981576ea1cc5f697d709b2d6b2ea29) )	/* blue component */
ROM_END

ROM_START( arkbloc2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "ark-6.bin",    0x0000, 0x8000, CRC(0be015de) SHA1(f4209085b59d2c96a62ac9657c7bf097da55362b) )
	ROM_LOAD( "arkgc.2",      0x8000, 0x8000, CRC(9f0d4754) SHA1(731c9224616a338084edd6944c754d68eabba7f2) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "a75_03.rom",   0x00000, 0x8000, CRC(038b74ba) SHA1(ac053cc4908b4075f918748b89570e07a0ba5116) )
	ROM_LOAD( "a75_04.rom",   0x08000, 0x8000, CRC(71fae199) SHA1(5d253c46ccf4cd2976a5fb8b8713f0f345443d06) )
	ROM_LOAD( "a75_05.rom",   0x10000, 0x8000, CRC(c76374e2) SHA1(7520dd48de20db60a2038f134dcaa454988e7874) )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "07.bpr",       0x0000, 0x0200, CRC(0af8b289) SHA1(6bc589e8a609b4cf450aebedc8ce02d5d45c970f) )	/* red component */
	ROM_LOAD( "08.bpr",       0x0200, 0x0200, CRC(abb002fb) SHA1(c14f56b8ef103600862e7930709d293b0aa97a73) )	/* green component */
	ROM_LOAD( "09.bpr",       0x0400, 0x0200, CRC(a7c6c277) SHA1(adaa003dcd981576ea1cc5f697d709b2d6b2ea29) )	/* blue component */
ROM_END

ROM_START( arkangc )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "arkgc.1",      0x0000, 0x8000, CRC(c54232e6) SHA1(beb759cee68009a06824b755d2aa26d7d436b5b0) )
	ROM_LOAD( "arkgc.2",      0x8000, 0x8000, CRC(9f0d4754) SHA1(731c9224616a338084edd6944c754d68eabba7f2) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "a75_03.rom",   0x00000, 0x8000, CRC(038b74ba) SHA1(ac053cc4908b4075f918748b89570e07a0ba5116) )
	ROM_LOAD( "a75_04.rom",   0x08000, 0x8000, CRC(71fae199) SHA1(5d253c46ccf4cd2976a5fb8b8713f0f345443d06) )
	ROM_LOAD( "a75_05.rom",   0x10000, 0x8000, CRC(c76374e2) SHA1(7520dd48de20db60a2038f134dcaa454988e7874) )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "07.bpr",       0x0000, 0x0200, CRC(0af8b289) SHA1(6bc589e8a609b4cf450aebedc8ce02d5d45c970f) )	/* red component */
	ROM_LOAD( "08.bpr",       0x0200, 0x0200, CRC(abb002fb) SHA1(c14f56b8ef103600862e7930709d293b0aa97a73) )	/* green component */
	ROM_LOAD( "09.bpr",       0x0400, 0x0200, CRC(a7c6c277) SHA1(adaa003dcd981576ea1cc5f697d709b2d6b2ea29) )	/* blue component */
ROM_END



static DRIVER_INIT( paddle2 )
{
	install_mem_read_handler (0, 0xf002, 0xf002, paddle2_prot_r);
	install_mem_write_handler(0, 0xd018, 0xd018, paddle2_prot_w);
	install_mem_read_handler (0, 0xd008, 0xd008, track_kludge_r );
}


GAME( 1986, arkanoid, 0,        arkanoid, arkanoid, 0,       ROT90, "Taito Corporation Japan", "Arkanoid (World)" )
GAME( 1986, arknoidu, arkanoid, arkanoid, arkanoid, 0,       ROT90, "Taito America Corporation (Romstar license)", "Arkanoid (US)" )
GAME( 1986, arknoiuo, arkanoid, arkanoid, arkanoid, 0,       ROT90, "Taito America Corporation (Romstar license)", "Arkanoid (US, older)" )
GAME( 1986, arknoidj, arkanoid, arkanoid, arknoidj, 0,       ROT90, "Taito Corporation", "Arkanoid (Japan, newer w/level select)" )
GAMEX(1986, arkbl2,   arkanoid, arkanoid, arknoidj, 0,       ROT90, "bootleg", "Arkanoid (Japanese bootleg Set 2)", GAME_NOT_WORKING )
GAMEX(1986, arkbl3,   arkanoid, bootleg,  arknoidj, paddle2, ROT90, "bootleg", "Arkanoid (Japanese bootleg Set 3)", GAME_NOT_WORKING )
GAMEX(1988, paddle2,  arkanoid, bootleg,  arknoidj, paddle2, ROT90, "bootleg", "Paddle 2", GAME_UNEMULATED_PROTECTION )
GAME( 1986, arkatayt, arkanoid, bootleg,  arkatayt, 0,       ROT90, "bootleg", "Arkanoid (Tayto bootleg, Japanese)" )
GAMEX(1986, arkblock, arkanoid, bootleg,  arknoidj, 0,       ROT90, "bootleg", "Block (bootleg, Japanese)", GAME_NOT_WORKING )
GAME( 1986, arkbloc2, arkanoid, bootleg,  arknoidj, 0,       ROT90, "bootleg", "Block (Game Corporation bootleg)" )
GAME( 1986, arkangc,  arkanoid, bootleg,  arknoidj, 0,       ROT90, "bootleg", "Arkanoid (Game Corporation bootleg)" )
GAME( 1987, arkatour, arkanoid, arkanoid, arkanoid, 0,       ROT90, "Taito America Corporation (Romstar license)", "Tournament Arkanoid (US)" )
