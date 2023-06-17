/*******************************************************************
R2D Tank (c) 1980 Sigma Ent. Inc.

driver by: David Haywood & Pierpaolo Prazzoli

TODO:

- 2nd PIA
- add sound


from the readme
----------------------------------------------------
Orca board number OVG-17A

r2d1.1c is ROM #1 at board position 1C, and so on.

1 = 2716
2 = 2732
3 = 2732
4 = 2732
5 = 2716 Sound

CPU = 6809
other = HD46505SP
other = MB14282(x2)
other = HD468458SP
other = MB14282
other = MB14368
other = HD6821 (x2)
other = HD46802
other = M5L8226 (x2)
RAM = 4116 (x11)

Should be very similar to Sigma's Spiders hardware.
----------------------------------------------------
********************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/6821pia.h"
#include "vidhrdw/crtc6845.h"
#include "cpu/m6800/m6800.h"
#include "cpu/m6809/m6809.h"

static int dipsw_bank;
static WRITE_HANDLER( dipsw_bank_w )
{
/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "bank = %x\n",data);*/
	dipsw_bank = data;
}

static int r2dtank_video_flip;
WRITE_HANDLER( r2dtank_video_flip_w )
{
	/*	0 -> flipped
		1 -> not flipped */

	r2dtank_video_flip = !data;
}

static READ_HANDLER( dipsw_r )
{
	switch( dipsw_bank )
	{
	case 0xff: /* @ $154*/
		return readinputport(1);

	case 0xfe: /* @ $155*/
		return readinputport(2);

	case 0xfd: /* @ $156*/
		return readinputport(3);

/*	case 0xfc:*/

/*	case 0xfb: */

/*	case 0xfa: */ /* insert a coin*/

	case 0xf8: /* finish level*/
		return 0;
		
	case 0xf9: /* game starts*/
		return 0;

/*	case 0xf3:*/

/*	case 0xf0:*/

/*	case 0xf1:*/
		
/*	case 0xec:*/

/*	case 0xeb:*/

/*	case 0xea:*/

	case 0xe9: /* game over*/
		return 0;

/*	case 0xe8: */ /* lose a life?*/
		
	default:
/*		log_cb(RETRO_LOG_DEBUG, LOGPRE "Unknown dipsw_r bank = %x\n",dipsw_bank);*/
		return 0xff;
	}
}

WRITE_HANDLER( r2dtank_pia_0_w )
{
	pia_0_w(offset, ~data);
}

WRITE_HANDLER( r2dtank_pia_1_w )
{
	pia_1_w(offset, ~data);
}

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x7fff, MRA_RAM },
	{ 0x8000, 0x8003, pia_0_r },
	{ 0x8004, 0x8004, dipsw_r },
/*	{ 0x8004, 0x8007, pia_1_r },*/
	{ 0xc000, 0xc007, MRA_RAM },
	{ 0xc800, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x7fff, MWA_RAM },	
	{ 0x8000, 0x8003, r2dtank_pia_0_w },
/*	{ 0x8004, 0x8007, r2dtank_pia_1_w },*/
	{ 0x8004, 0x8004, dipsw_bank_w },
	{ 0xb000, 0xb000, crtc6845_address_w }, 
	{ 0xb001, 0xb001, crtc6845_register_w },
	{ 0xc000, 0xc007, MWA_RAM, &generic_nvram, &generic_nvram_size },
	{ 0xc800, 0xffff, MWA_ROM },
MEMORY_END

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x00ff, MRA_RAM },
	{ 0xd000, 0xd000, MRA_RAM }, /* AY8910_read_port_0_r ?*/
	{ 0xf000, 0xf000, MRA_RAM }, /* soundlatch_r ?*/
	{ 0xf800, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x0027, MWA_RAM },
	{ 0x0077, 0x007f, MWA_RAM },
	{ 0xd000, 0xd000, MWA_RAM },
	{ 0xd001, 0xd001, MWA_RAM },
	{ 0xd002, 0xd002, MWA_RAM },
	{ 0xd003, 0xd003, MWA_RAM },
	{ 0xf800, 0xffff, MWA_ROM },
MEMORY_END

INPUT_PORTS_START( r2dtank )

	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP   | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )

	PORT_START	/* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( Free_Play ) )

	PORT_START	/* DSW B */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START	/* DSW C */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

INPUT_PORTS_END

PALETTE_INIT( r2dtank )
{
	/* colours derived from tim's screenshots */

	palette_set_color(0x0,0,0,0);		/* background*/
	palette_set_color(0x1,0,255,0); 	/* credits*/
	palette_set_color(0x2,0,0,251); 	/* presented by sigma*/
	palette_set_color(0x3,0,214,255); 	/* per points etc.*/
	palette_set_color(0x4,255,0,0); 	/* r2dtank logo*/
	palette_set_color(0x5,255,255,128); /* unused?*/
	palette_set_color(0x6,255,0,255); 	/* pink tank*/
	palette_set_color(0x7,236,141,181); /* sigma (c)*/

}

VIDEO_UPDATE( r2dtank )
{
	int loop, data0, data1;

	unsigned char *RAM = memory_region(REGION_CPU1);

	for(loop = 0; loop < 0x1e00; loop++)
	{
		int i,x,y;

		data0 = RAM[0x0200+loop];
		data1 = RAM[0x4200+loop];

		y = loop / 0x20;

		for(i = 0; i < 8; i++)
		{
			int col;
			x = ((loop % 0x20) << 3) + i;

			col = (data0 & 0x80) ? ((data1 & 0xe0) >> 5) : ((data1 & 0x0e)>> 1);

			plot_pixel(tmpbitmap, x, y, Machine->pens[col]);
			data0 <<= 1;

		}
	}

	/* Now copy the temp bitmap to the screen */
	copybitmap(bitmap,tmpbitmap,r2dtank_video_flip,r2dtank_video_flip,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);
}

/* Declare PIA structure */

/* PIA 0, main CPU */
static struct pia6821_interface pia_0_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ input_port_0_r, input_port_1_r, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, 0, 0, r2dtank_video_flip_w,
	/*irqs   : A/B             */ 0, 0
};

/* PIA 1, main CPU */
static struct pia6821_interface pia_1_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, 0, 0, 0,
	/*irqs   : A/B             */ 0, 0
};

MACHINE_INIT( r2dtank )
{
	pia_unconfig();
	pia_config(0, PIA_STANDARD_ORDERING, &pia_0_intf);
	pia_config(1, PIA_STANDARD_ORDERING, &pia_1_intf);
	pia_reset();
}

INTERRUPT_GEN( r2dtank_interrupt )
{
	cpu_set_irq_line(0, 0, HOLD_LINE);
}

static MACHINE_DRIVER_START( r2dtank )
	MDRV_CPU_ADD(M6809,3000000)		 /* ?? too fast ? */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(r2dtank_interrupt,2)

	/* Sound CPU */
	MDRV_CPU_ADD(M6802,3000000/4)			/* ?? */
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(r2dtank)
	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)

	MDRV_PALETTE_LENGTH(8)
	MDRV_PALETTE_INIT(r2dtank)

	MDRV_VIDEO_START(generic_bitmapped)
	MDRV_VIDEO_UPDATE(r2dtank)

	/* sound hardware */
MACHINE_DRIVER_END

ROM_START( r2dtank )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "r2d1.1c",      0xc800, 0x0800, CRC(20606a0f) SHA1(9a55e595c7ea332bdc89142338947be8a28a92a3) )
	ROM_LOAD( "r2d2.1a",      0xd000, 0x1000, CRC(7561c67f) SHA1(cccc7bbd7975db340fe571a4c31c25b41b2563b8) )
	ROM_LOAD( "r2d3.2c",      0xe000, 0x1000, CRC(fc53c538) SHA1(8f9a2edcf7a2cb2a8ddd084828b52f1bf45f434a) )
	ROM_LOAD( "r2d4.2a",      0xf000, 0x1000, CRC(56636225) SHA1(dcfc6e29b4c51a45cfbecf6790b7d88b89af433b) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "r2d5.7l",      0xf800, 0x0800, CRC(c49bed15) SHA1(ffa635a65c024c532bb13fb91bbd3e54923e81bf) )
ROM_END

GAMEX( 1980, r2dtank, 0, r2dtank, r2dtank, 0, ROT270, "Sigma Enterprises Inc.", "R2D Tank", GAME_NO_SOUND )
