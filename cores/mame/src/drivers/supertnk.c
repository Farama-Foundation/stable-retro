/***************************************************************************

Super Tank
(c) 1981 Video Games GmbH

Reverse-engineering and MAME driver by Norbert Kehrer (December 2003).

****************************************************************************


Hardware:
---------

CPU		TMS9980 (Texas Instruments)
Sound chip	AY-3-8910


Memory map:
-----------

>0000 - >07ff	Fixed ROM
>0800 - >17ff	Bank-switched ROM
>1800 - >1bff	RAM
>1efc		Input port 1
>1efd		Input port 2
>1efe		Read:  DIP switch port 1
		Write: Control port of AY-3-8910 sound chip
>1eff		Read: DIP switch port 2
		Write: Data port of AY-3-8910
>2000 - >3fff	Video RAM (CPU view of it, in reality there are 24 KB on the video board)


Input ports:
------------

Input port 0, mapped to memory address 0x1efc:
7654 3210
0          Player 2 Right
 0         Player 2 Left
  0        Player 2 Down
   0       Player 2 Up
     0     Player 1 Right
      0    Player 1 Left
       0   Player 1 Down
        0  Player 1 Up

Input port 1, mapped to memory address 0x1efd:
7654 3210
0          Player 2 Fire
 0         Player 1 Fire
  0        ??
   0       ??
     0     Start 1 player game
      0    Start 2 players game
       0   Coin (strobe)
        0  ??


DIP switch ports:
-----------------

DIP switch port 1, mapped to memory address 0x1efe:
7654 3210
0          Not used (?)
 0         Not used (?)
  0        Tanks per player: 1 = 5 tanks, 0 = 3 tanks
   0       Extra tank: 1 = at 10,000, 0 = at 15,000 pts.
     000   Coinage
        0  Not used (?)

DIP switch port 2, mapped to memory address 0x1eff:
7654 3210
1          ??
 1         ??
  1        ??
   1       ??
     1     ??
      1    ??
       1   ??
        1  ??


CRU lines:
----------

>400	Select bitplane for writes into video RAM (bit 1)
>401	Select bitplane for writes into video RAM (bit 0)
>402	ROM bank selector (bit 1)
>404	ROM bank selector (bit 0)
>406	Interrupt acknowledge (clears interrupt line)
>407	Watchdog reset (?)
>b12	Unknown, maybe some special-hardware sound effect or lights blinking (?)
>b13	Unknown, maybe some special-hardware sound effect or lights blinking (?)

***************************************************************************/


#include "driver.h"
#include "vidhrdw/generic.h"

static int supertnk_rom_bank;
static int supertnk_video_bitplane;

static UINT8 *supertnk_videoram;



PALETTE_INIT( supertnk )
{
	int i, r, g, b;

	for (i = 0;i < 0x20;i++)
	{
		r = (*color_prom & 0x04) ? 0xff : 0x00;  /* red component   */
		g = (*color_prom & 0x20) ? 0xff : 0x00;  /* green component */
		b = (*color_prom & 0x40) ? 0xff : 0x00;  /* blue component  */

		palette_set_color(i, r, g, b);
		color_prom++;
	}
}



VIDEO_START( supertnk )
{
	supertnk_videoram = auto_malloc(0x6000);	/* allocate physical video RAM */

	if (supertnk_videoram  == NULL)
	{
		return 1;
	}

	memset(supertnk_videoram, 0, 0x6000);

	return video_start_generic_bitmapped();
}



WRITE_HANDLER( supertnk_videoram_w )
{
	int x, y, i, col, col0, col1, col2;

	if (supertnk_video_bitplane > 2)
	{
		supertnk_videoram[0x0000 + offset] =
		supertnk_videoram[0x2000 + offset] =
		supertnk_videoram[0x4000 + offset] = 0;
	}
	else
		supertnk_videoram[0x2000 * supertnk_video_bitplane + offset] = data;


	x = (offset % 32) * 8 ;
	y = (offset / 32);

	for (i=0; i<8; i++)
	{
		col0 = (supertnk_videoram[0x0000 + offset] >> (7-i)) & 0x01;
		col1 = (supertnk_videoram[0x2000 + offset] >> (7-i)) & 0x01;
		col2 = (supertnk_videoram[0x4000 + offset] >> (7-i)) & 0x01;
		col = ( (col0 << 2) | (col1 << 1) | (col2 << 0) );

		plot_pixel(tmpbitmap, x+i, y, Machine->pens[col]);
	}
}



READ_HANDLER( supertnk_videoram_r )
{
	if (supertnk_video_bitplane < 3)
		return supertnk_videoram[0x2000 * supertnk_video_bitplane + offset];
	else
		return 0;
}


VIDEO_UPDATE( supertnk )
{
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);
}



WRITE_HANDLER( supertnk_intack )
{
	cpu_set_irq_line(0, 0, CLEAR_LINE);
}



WRITE_HANDLER( supertnk_bankswitch_w )
{
	int bankaddress;
	UINT8 *ROM = memory_region(REGION_CPU1);
	switch (offset)
	{
		case 0:
			supertnk_rom_bank &= 0x02;
			supertnk_rom_bank |= (data & 0x01);
			break;
		case 2:
			supertnk_rom_bank &= 0x01;
			supertnk_rom_bank |= ((data & 0x01) << 1);
			break;
	}
	bankaddress = 0x4000 + supertnk_rom_bank * 0x1000;
	cpu_setbank(1,&ROM[bankaddress]);
}



WRITE_HANDLER( supertnk_set_video_bitplane )
{
	switch (offset)
	{
		case 0:
			supertnk_video_bitplane &= 0x02;
			supertnk_video_bitplane |= (data & 0x01);
			break;
		case 1:
			supertnk_video_bitplane &= 0x01;
			supertnk_video_bitplane |= ((data & 0x01) << 1);
			break;
	}
}



INTERRUPT_GEN( supertnk_interrupt )
{
	/* On a TMS9980, a 6 on the interrupt bus means a level 4 interrupt */
	cpu_set_irq_line_and_vector(0, 0, ASSERT_LINE, 6);
}



static MEMORY_READ_START( supertnk_readmem )
	{ 0x0000, 0x07ff, MRA_ROM },			/* Fixed ROM */
	{ 0x0800, 0x17ff, MRA_BANK1 },			/* Banked ROM */
	{ 0x2000, 0x3fff, supertnk_videoram_r },	/* Video RAM */
	{ 0x1efc, 0x1efc, input_port_0_r },		/* Input ports */
	{ 0x1efd, 0x1efd, input_port_1_r },
	{ 0x1efe, 0x1efe, input_port_2_r },		/* DIP switch ports */
	{ 0x1eff, 0x1eff, input_port_3_r },
MEMORY_END



static MEMORY_WRITE_START( supertnk_writemem )
	{ 0x0000, 0x17ff, MWA_ROM },
	{ 0x1800, 0x1bff, MWA_RAM },
	{ 0x1efe, 0x1efe, AY8910_control_port_0_w },	/* Sound chip control port */
	{ 0x1eff, 0x1eff, AY8910_write_port_0_w },	/* Sound chip data port */
	{ 0x2000, 0x3fff, supertnk_videoram_w },	/* Video RAM */
MEMORY_END



static PORT_WRITE_START( supertnk_writeport)
	{ 0x000, 0x000, MWA_NOP },
	{ 0x400, 0x401, supertnk_set_video_bitplane },
	{ 0x402, 0x404, supertnk_bankswitch_w },
	{ 0x406, 0x406, supertnk_intack },
	{ 0x407, 0x407, watchdog_reset_w },
PORT_END




static struct AY8910interface ay8910_interface =
{
	1,
	2000000,	/* ? which frequency? the same as the CPU? */
	{ 50 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};




static MACHINE_DRIVER_START( supertnk )

	/* basic machine hardware */
	MDRV_CPU_ADD(TMS9980, 2598750) /* ? to which frequency is the 20.79 Mhz crystal mapped down? */
	MDRV_CPU_MEMORY(supertnk_readmem,supertnk_writemem)
	MDRV_CPU_PORTS(0,supertnk_writeport)
	MDRV_CPU_VBLANK_INT(supertnk_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 32*8-1)
	MDRV_PALETTE_LENGTH(32)
	MDRV_COLORTABLE_LENGTH(32)

	MDRV_PALETTE_INIT(supertnk)
	MDRV_VIDEO_START(supertnk)
	MDRV_VIDEO_UPDATE(supertnk)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END



INPUT_PORTS_START( supertnk )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT_IMPULSE( 0x02, IP_ACTIVE_LOW, IPT_COIN1, 1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )

	PORT_START
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x0e, 0x02, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x0c, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0x0a, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(	0x08, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(	0x06, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x00, "at 15,000 points" )
	PORT_DIPSETTING(	0x10, "at 10,000 points" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING( 	0x00, "3" )
	PORT_DIPSETTING(    	0x20, "5" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )
INPUT_PORTS_END



ROM_START( supertnk )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for TMS9980 code - normal and banked ROM */
	ROM_LOAD( "supertan.2d",	0x0000, 0x0800, CRC(1656a2c1) SHA1(1d49945aed105003a051cfbf646af7a4be1b7e86) )
	ROM_LOAD( "supertnk.3d",	0x4800, 0x0800, CRC(8b023a9a) SHA1(1afdc8d75f2ca04153bac20c0e3e123e2a7acdb7) )
		ROM_CONTINUE(		0x4000, 0x0800)
	ROM_LOAD( "supertnk.4d",	0x5800, 0x0800, CRC(b8249e5c) SHA1(ef4bb714b0c1b97890a067f05fc50ab3426ce37f) )
		ROM_CONTINUE(		0x5000, 0x0800)
	ROM_LOAD( "supertnk.8d",	0x6800, 0x0800, CRC(d8175a4f) SHA1(cba7b426773ac86c81a9eac81087a2db268cd0f9) )
		ROM_CONTINUE(		0x6000, 0x0800)
	ROM_LOAD( "supertnk.9d",	0x7800, 0x0800, CRC(a34a494a) SHA1(9b7f0560e9d569ee25eae56f31886d50a3153dcc) )
		ROM_CONTINUE(		0x7000, 0x0800)

	ROM_REGION( 0x0060, REGION_PROMS, 0 ) /* color PROM */
	ROM_LOAD( "supertnk.clr",	0x0000, 0x0020, CRC(9ae1faee) SHA1(19de4bb8bc389d98c8f8e35c755fad96e1a6a0cd) )
 	/* unknown */
	ROM_LOAD( "supertnk.s",		0x0020, 0x0020, CRC(91722fcf) SHA1(f77386014b459cc151d2990ac823b91c04e8d319) )
	/* unknown */
	ROM_LOAD( "supertnk.t",		0x0040, 0x0020, CRC(154390bd) SHA1(4dc0fd7bd8999d2670c8d93aaada835d2a84d4db) )
ROM_END


DRIVER_INIT( supertnk ){
	/* decode the TMS9980 ROMs */
	UINT8 *pMem = memory_region( REGION_CPU1 );
	UINT8 raw, code;
	int i;
	for( i=0; i<0x8000; i++ )
	{
		raw = pMem[i];
		code = 0;
		if( raw&0x01 ) code |= 0x80;
		if( raw&0x02 ) code |= 0x40;
		if( raw&0x04 ) code |= 0x20;
		if( raw&0x08 ) code |= 0x10;
		if( raw&0x10 ) code |= 0x08;
		if( raw&0x20 ) code |= 0x04;
		if( raw&0x40 ) code |= 0x02;
		if( raw&0x80 ) code |= 0x01;
		pMem[i] = code;
	};

	supertnk_rom_bank = 0;
	supertnk_video_bitplane = 0;
}



/*          rom       parent     machine   inp       init */
GAME( 1981, supertnk,  0,        supertnk, supertnk, supertnk, ROT90, "Video Games GmbH", "Super Tank" )
