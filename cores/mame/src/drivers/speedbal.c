/***************************************************************************

Speed Ball map

driver by Joseba Epalza

Z80 MAIN CPU
0000-7fff ROM 1 32k
8000-dbff ROM 2 data
dc00-dfff for both CPU&SOUND (Shared)
e000-e1ff Video RAM background tiles 16*16 (2 bytes: 1 for tile char + 1 for color)
e800-efff Video RAM characters 32*32 (2 bytes: 1 for char and 1 for color)
f000-feff RAM
ff00-ffff Sprites (64 total: four bytes to one)

in:
00	DSW ONE
10	DSW TWO
20	IN1  JOY 1 STATUS & COIN & START
30	IN2  JOY 2 STATUS & TILT

out:
40	????
50	????  maybe VSYNC ????

 ======================================================================

Z80 SOUND
0000-7fff ROM
dc00-dfff shared with MAIN CPU
f000-dfff RAM

out:
00	YM3812 control
01	YM3812 data
20	??

 ======================================================================

  Colors :   2 bits for foreground characters =  4 colors * 16 palettes
	     4 bits for background tiles      = 16 colors * 16 palettes
	     4 bits for sprites	       = 16 colors * 16 palettes

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


unsigned char *speedbal_foreground_videoram;
unsigned char *speedbal_background_videoram;
unsigned char *speedbal_sprites_dataram;

size_t speedbal_foreground_videoram_size;
size_t speedbal_background_videoram_size;
size_t speedbal_sprites_dataram_size;

VIDEO_START( speedbal );
VIDEO_UPDATE( speedbal );
READ_HANDLER( speedbal_foreground_videoram_r );
WRITE_HANDLER( speedbal_foreground_videoram_w );
READ_HANDLER( speedbal_background_videoram_r );
WRITE_HANDLER( speedbal_background_videoram_w );


unsigned char *speedbal_sharedram;

READ_HANDLER( speedbal_sharedram_r )
{
/*  if (offset==0x0) speedbal_sharedram[offset]+=1;*/
  return speedbal_sharedram[offset];
}


WRITE_HANDLER( speedbal_sharedram_w )
{
    speedbal_sharedram[offset] = data;
}

static MEMORY_READ_START( readmem )
	{ 0x0000, 0xdbff, MRA_ROM },
	{ 0xdc00, 0xdfff, speedbal_sharedram_r },  /* shared with SOUND*/
	{ 0xe000, 0xe1ff, speedbal_background_videoram_r },
	{ 0xe800, 0xefff, speedbal_foreground_videoram_r },
	{ 0xf000, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0xdbff, MWA_ROM },
	{ 0xdc00, 0xdfff, speedbal_sharedram_w, &speedbal_sharedram },  /* shared with SOUND*/
	{ 0xe000, 0xe1ff, speedbal_background_videoram_w, &speedbal_background_videoram, &speedbal_background_videoram_size },
	{ 0xe800, 0xefff, speedbal_foreground_videoram_w, &speedbal_foreground_videoram, &speedbal_foreground_videoram_size },
	{ 0xf000, 0xf5ff, paletteram_RRRRGGGGBBBBxxxx_swap_w, &paletteram },
	{ 0xf600, 0xfeff, MWA_RAM },
	{ 0xff00, 0xffff, MWA_RAM, &speedbal_sprites_dataram, &speedbal_sprites_dataram_size },
MEMORY_END

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xdc00, 0xdfff, speedbal_sharedram_r }, /* shared with MAIN CPU*/
	{ 0xf000, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xdc00, 0xdfff, speedbal_sharedram_w }, /* shared with MAIN CPU*/
	{ 0xf000, 0xffff, MWA_RAM },
MEMORY_END

static PORT_READ_START( readport )
	{ 0x00, 0x00, input_port_0_r },
	{ 0x10, 0x10, input_port_1_r },
	{ 0x20, 0x20, input_port_2_r },
	{ 0x30, 0x30, input_port_3_r },
PORT_END


static PORT_READ_START( sound_readport )
	{ 0x00, 0x00, YM3812_status_port_0_r },
PORT_END

static PORT_WRITE_START( sound_writeport )
	{ 0x00, 0x00, YM3812_control_port_0_w },
	{ 0x01, 0x01, YM3812_write_port_0_w },
PORT_END



INPUT_PORTS_START( speedbal )
	PORT_START      /* DSW2 */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x06, "70000 200000 1M" )
	PORT_DIPSETTING(    0x07, "70000 200000" )
	PORT_DIPSETTING(    0x03, "100000 300000 1M" )
	PORT_DIPSETTING(    0x04, "100000 300000" )
	PORT_DIPSETTING(    0x01, "200000 1M" )
	PORT_DIPSETTING(    0x05, "200000" )
/*	PORT_DIPSETTING(    0x02, "200000" ) */
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, "Difficulty 1" )
	PORT_DIPSETTING(    0x30, "Very Easy" )
	PORT_DIPSETTING(    0x20, "Easy" )
	PORT_DIPSETTING(    0x10, "Difficult" )
	PORT_DIPSETTING(    0x00, "Very Difficult" )
	PORT_DIPNAME( 0xc0, 0xc0, "Difficulty 2" )
	PORT_DIPSETTING(    0xc0, "Very Easy" )
	PORT_DIPSETTING(    0x80, "Easy" )
	PORT_DIPSETTING(    0x40, "Difficult" )
	PORT_DIPSETTING(    0x00, "Very Difficult" )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW , IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW , IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW , IPT_BUTTON4 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW , IPT_BUTTON3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW , IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW , IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW , IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW , IPT_COIN1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW , IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW , IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW , IPT_BUTTON4 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW , IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW , IPT_TILT    )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,    /* 8*8 characters */
	1024,   /* 1024 characters */
	4,      /* actually 2 bits per pixel - two of the planes are empty */
	{ 1024*16*8+4, 1024*16*8+0, 4, 0 },
	{ 8+3, 8+2, 8+1, 8+0, 3, 2, 1, 0 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },   /* characters are rotated 90 degrees */
	16*8	   /* every char takes 16 bytes */
};

static struct GfxLayout tilelayout =
{
	16,16,  /* 16*16 tiles */
	1024,   /* 1024 tiles */
	4,      /* 4 bits per pixel */
	{ 0, 2, 4, 6 }, /* the bitplanes are packed in one nibble */
	{ 0*8+0, 0*8+1, 7*8+0, 7*8+1, 6*8+0, 6*8+1, 5*8+0, 5*8+1,
			4*8+0, 4*8+1, 3*8+0, 3*8+1, 2*8+0, 2*8+1, 1*8+0, 1*8+1 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
			8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	128*8  /* every sprite takes 128 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,  /* 16*16 sprites */
	512,    /* 512 sprites */
	4,      /* 4 bits per pixel */
	{ 0, 2, 4, 6 }, /* the bitplanes are packed in one nibble */
	{ 7*8+1, 7*8+0, 6*8+1, 6*8+0, 5*8+1, 5*8+0, 4*8+1, 4*8+0,
			3*8+1, 3*8+0, 2*8+1, 2*8+0, 1*8+1, 1*8+0, 0*8+1, 0*8+0 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
			8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	128*8  /* every sprite takes 128 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,	 256, 16 },
	{ REGION_GFX2, 0, &tilelayout,	 512, 16 },
	{ REGION_GFX3, 0, &spritelayout,   0, 16 },
	{ -1 } /* end of array */
};



static struct YM3812interface ym3812_interface =
{
	1,		      /* 1 chip (no more supported) */
	3600000,	/* 3.600000 MHz ? (partially supported) */
	{ 100 }	 /* (not supported) */
};



static MACHINE_DRIVER_START( speedbal )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz ??? */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_PORTS(readport,0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, 2660000)	/* 2.66 MHz ???  Maybe yes */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PORTS(sound_readport,sound_writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,8)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(768)

	MDRV_VIDEO_START(speedbal)
	MDRV_VIDEO_UPDATE(speedbal)

	/* sound hardware */
	MDRV_SOUND_ADD(YM3812, ym3812_interface)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( speedbal )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64K for code: main */
	ROM_LOAD( "sb1.bin",  0x0000,  0x8000, CRC(1c242e34) SHA1(8b2e8983e0834c99761ce2b5ea765dba56e77964) )
	ROM_LOAD( "sb3.bin",  0x8000,  0x8000, CRC(7682326a) SHA1(15a72bf088a9adfaa50c11202b4970e07c309a21) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64K for second CPU: sound */
	ROM_LOAD( "sb2.bin",  0x0000, 0x8000, CRC(e6a6d9b7) SHA1(35d228d13d4305f606fdd84adad1d6e435f4b7ce) )

	ROM_REGION( 0x08000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD("sb10.bin",  0x00000, 0x08000, CRC(36dea4bf) SHA1(60095f482af4595a39be5ae6def8cd30298c1ef8) )    /* chars */

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "sb9.bin",  0x00000, 0x08000, CRC(b567e85e) SHA1(7036792ea70ad48384f348399ed9b136272fedb6) )    /* bg tiles */
	ROM_LOAD( "sb5.bin",  0x08000, 0x08000, CRC(b0eae4ba) SHA1(baee3fcb1399c56efaa5f97912de324d7b38f286) )
	ROM_LOAD( "sb8.bin",  0x10000, 0x08000, CRC(d2bfbdb6) SHA1(b552b055450f438729c83337f561d05b6518ae75) )
	ROM_LOAD( "sb4.bin",  0x18000, 0x08000, CRC(1d23a130) SHA1(aabf7c46f9299ffb8b8ca92839622d000a470a0b) )

	ROM_REGION( 0x10000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "sb6.bin",  0x00000, 0x08000, CRC(0e2506eb) SHA1(56f779266b977819063c475b84ca246fc6d8d6a7) )    /* sprites */
	ROM_LOAD( "sb7.bin",  0x08000, 0x08000, CRC(9f1b33d1) SHA1(1f8be8f8e6a2ee99a7dafeead142ccc629fa792d) )
ROM_END


static DRIVER_INIT( speedbal )
{
	int i;

	/* invert the graphics bits on the sprites */
	for (i = 0; i < memory_region_length(REGION_GFX3); i++)
		memory_region(REGION_GFX3)[i] ^= 0xff;
}



GAME( 1987, speedbal, 0, speedbal, speedbal, speedbal, ROT270, "Tecfri", "Speed Ball" )
