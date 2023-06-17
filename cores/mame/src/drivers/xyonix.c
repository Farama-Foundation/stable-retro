/* Xyonix *********************************************************************

driver by David Haywood and Stephh

Notes about the board:

Ram is 2x 6264 (near Z80) and 1x 6264 near UM6845. Xtal is verified 16.000MHz,
I can also see another special chip . PHILKO PK8801. chip looks about the same as a
TMS3615 (though i have no idea what the chip actually is). its located next to the
prom, the 2x 256k roms, and the 1x 6264 ram.
Dip SW is 1 x 8-position

on the PCB is an empty socket. written next to the socket is 68705P3. "oh no" you
say..... well, its unpopulated, so maybe it was never used?


TODO:
- there are some more unknown commands for the I/O chip

******************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"

data8_t *xyonix_vidram;

/* in vidhrdw/xyonix.c */
PALETTE_INIT( xyonix );
WRITE_HANDLER( xyonix_vidram_w );
VIDEO_START(xyonix);
VIDEO_UPDATE(xyonix);


static WRITE_HANDLER( xyonix_irqack_w )
{
	cpu_set_irq_line(0, 0, CLEAR_LINE);
}


/* Inputs ********************************************************************/

static int e0_data,credits,coins;

void handle_coins(int coin)
{
	int coinage_table[4][2] = {{2,3},{2,1},{1,2},{1,1}};
	int tmp = 0;

/*	usrintf_showmessage("Coin %d",coin);*/

	if (coin & 1)	/* Coin 2 !*/
	{
		tmp = (readinputport(2) & 0xc0) >> 6;
		coins++;
		if (coins >= coinage_table[tmp][0])
		{
			credits += coinage_table[tmp][1];
			coins -= coinage_table[tmp][0];
		}
		coin_lockout_global_w(0); /* Unlock all coin slots */
		coin_counter_w(1,1); coin_counter_w(1,0); /* Count slot B */
	}

	if (coin & 2)	/* Coin 1 !*/
	{
		tmp = (readinputport(2) & 0x30) >> 4;
		coins++;
		if (coins >= coinage_table[tmp][0])
		{
			credits += coinage_table[tmp][1];
			coins -= coinage_table[tmp][0];
		}
		coin_lockout_global_w(0); /* Unlock all coin slots */
		coin_counter_w(0,1); coin_counter_w(0,0); /* Count slot A */
	}

	if (credits >= 9)
		credits = 9;
}


READ_HANDLER ( xyonix_io_r )
{
	int regPC = activecpu_get_pc();

	if (regPC == 0x27ba)
		return 0x88;

	if (regPC == 0x27c2)
		return e0_data;

	if (regPC == 0x27c7)
	{
		static int prev_coin;
		int coin;

		switch (e0_data)
		{
			case 0x81 :
				return readinputport(0) & 0x7f;
				break;
			case 0x82 :
				return readinputport(1) & 0x7f;
				break;
			case 0x91:
				/* check coin inputs */
				coin = ((readinputport(0) & 0x80) >> 7) | ((readinputport(1) & 0x80) >> 6);
				if (coin ^ prev_coin && coin != 3)
				{
					if (credits < 9) handle_coins(coin);
				}
				prev_coin = coin;
				return credits;
				break;
			case 0x92:
				return ((readinputport(0) & 0x80) >> 7) | ((readinputport(1) & 0x80) >> 6);
				break;
			case 0xe0:	/* reset? */
				coins = 0;
				credits = 0;
				return 0xff;
				break;
			case 0xe1:
				credits--;
				return 0xff;
				break;
			case 0xfe:	/* Dip Switches 1 to 4 */
				return readinputport(2) & 0x0f;
				break;
			case 0xff:	/* Dip Switches 5 to 8 */
				return readinputport(2) >> 4;
				break;
		}
	}

/*	logerror ("xyonix_port_e0_r - PC = %04x - port = %02x\n", regPC, e0_data);*/
/*	usrintf_showmessage("%02x",e0_data);*/

	return 0xff;
}

WRITE_HANDLER ( xyonix_io_w )
{
/*	logerror ("xyonix_port_e0_w %02x - PC = %04x\n", data, activecpu_get_pc());*/
	e0_data = data;
}

/* Mem / Port Maps ***********************************************************/

static MEMORY_READ_START( readmem )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xffff, xyonix_vidram_w, &xyonix_vidram },
MEMORY_END

static PORT_READ_START( port_readmem )
	{ 0x20, 0x21, IORP_NOP },	/* SN76496 ready signal */
	{ 0xe0, 0xe0, xyonix_io_r },
MEMORY_END

static PORT_WRITE_START( port_writemem )
	{ 0x20, 0x20, SN76496_0_w },
	{ 0x21, 0x21, SN76496_1_w },
	{ 0xe0, 0xe0, xyonix_io_w },
	{ 0x40, 0x40, IOWP_NOP },	/* NMI ack?*/
	{ 0x50, 0x50, xyonix_irqack_w },
	{ 0x60, 0x61, IOWP_NOP },	/* crtc6845*/
MEMORY_END

/* Inputs Ports **************************************************************/

INPUT_PORTS_START( xyonix )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )		/* handled by xyonix_io_r() */

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )		/* handled by xyonix_io_r() */

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, "Easy" )
	PORT_DIPSETTING(    0x02, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )			/* "Very Hard"*/
	PORT_DIPNAME( 0x04, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x04, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_SERVICE( 0x08, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )
INPUT_PORTS_END

/* GFX Decode ****************************************************************/

static struct GfxLayout charlayout =
{
	4,8,
	RGN_FRAC(1,2),
	4,
	{ 0, 4, RGN_FRAC(1,2)+0, RGN_FRAC(1,2)+4 },
	{ 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	4*16
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0, 16 },
	{ -1 }
};

/* MACHINE driver *************************************************************/

static struct SN76496interface sn76496_interface =
{
	2,						/* 2 chips */
	{ 16000000/4, 16000000/4 },	/* 4 MHz??? */
	{ 100, 100 }
};


static MACHINE_DRIVER_START( xyonix )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,16000000 / 4)		 /* 4 MHz ? */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_PORTS(port_readmem,port_writemem)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,1)
	MDRV_CPU_PERIODIC_INT(irq0_line_assert,4*60)	/* ?? controls music tempo */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE(80*4, 32*8)
	MDRV_VISIBLE_AREA(0, 80*4-1, 0, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)

	MDRV_PALETTE_INIT(xyonix)
	MDRV_VIDEO_START(xyonix)
	MDRV_VIDEO_UPDATE(xyonix)

	/* sound hardware */
	MDRV_SOUND_ADD(SN76496, sn76496_interface)
MACHINE_DRIVER_END

/* ROM Loading ***************************************************************/

ROM_START( xyonix )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "xyonix3.bin", 0x00000, 0x10000, CRC(1960a74e) SHA1(5fd7bc31ca2f5f1e114d3d0ccf6554ebd712cbd3) )

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "xyonix1.bin", 0x00000, 0x08000, CRC(3dfa9596) SHA1(52cdbbe18f83cea7248c29588ea3a18c4bb7984f) )
	ROM_LOAD( "xyonix2.bin", 0x08000, 0x08000, CRC(db87343e) SHA1(62bc30cd65b2f8976cd73a0b349a9ccdb3faaad2) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "xyonix.pr",   0x0000, 0x0100, CRC(0012cfc9) SHA1(c7454107a1a8083a370b662c617117b769c0dc1c) )
ROM_END

/* GAME drivers **************************************************************/

GAME( 1989, xyonix, 0, xyonix, xyonix, 0, ROT0, "Philko", "Xyonix" )
