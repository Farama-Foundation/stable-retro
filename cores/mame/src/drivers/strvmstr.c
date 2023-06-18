/*

 Super Trivia Master (c) 1986 Enerdyne Technologies Inc. (El Cajon, CA 92020)

 CPU: Z80
 Sound: AY-3-8912 (x3)
 Other: Dallas DS1220Y NVRAM, N8T245N (x2), PAL16L8A-2CN (x2,protected)

 XTAL = 12 MHz


 driver by Pierpaolo Prazzoli, thanks to Tomasz Slanina too.

 TODO:
	- Colours
	- Finish sound
	- Cocktail support

*/

#include "driver.h"
#include "vidhrdw/generic.h"

static int strvmstr_control = 0;
static UINT8 *bg_videoram, *fg_videoram;
static struct tilemap *bg_tilemap, *fg_tilemap;

static WRITE_HANDLER( strvmstr_fg_w )
{
	fg_videoram[offset] = data;
	tilemap_mark_tile_dirty(fg_tilemap,offset);
}

static WRITE_HANDLER( strvmstr_bg_w )
{
	bg_videoram[offset] = data;
	tilemap_mark_tile_dirty(bg_tilemap,offset);
}

static WRITE_HANDLER( strvmstr_control_w )
{

/*

bits:
	1 -> change to 1 or 0 when change screen orientation
	2 -> setted in intro
	3 -> background bank
	4,5 -> question offsets
	6 -> unused ?
	7 -> unused ?
	8 -> setted when coin is inserted

*/

	strvmstr_control = data;

	if( data & 0x04)
	{
		tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
	}
}

static WRITE_HANDLER( a000_w )
{
	/* ? */
}


static READ_HANDLER( strvmstr_question_r )
{
	data8_t *Question = memory_region(REGION_USER1);
	return Question[offset + 0x10000 * ((strvmstr_control >> 3) & 3)];
}

static int b800_prev,b000_val,b000_ret;

static WRITE_HANDLER( b000_w )
{
	b000_val = data;
}

static READ_HANDLER( b000_r )
{
	return b000_ret;
}

static WRITE_HANDLER( b800_w )
{
	switch(data)
	{
		case 0xc4:	b000_ret=AY8910_read_port_0_r(0);	break;
		case 0x94:	b000_ret=AY8910_read_port_1_r(0);	break;
		case 0x86:	b000_ret=AY8910_read_port_2_r(0);	break;
		
		case 0x80:
			switch(b800_prev)
			{
				case 0xe0: AY8910_control_port_0_w(0,b000_val);	break;		
				case 0x98: AY8910_control_port_1_w(0,b000_val);	break;		
				case 0x83: AY8910_control_port_2_w(0,b000_val);	break;		
				
				case 0xa0: AY8910_write_port_0_w(0,b000_val);	break;		
				case 0x88: AY8910_write_port_1_w(0,b000_val);	break;		
				case 0x81: AY8910_write_port_2_w(0,b000_val);	break;		
	
			}
		break;
	}	

	b800_prev = data;
}

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0xb000, 0xb000, b000_r },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xe000, 0xe7ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM, &generic_nvram, &generic_nvram_size },
	{ 0x9000, 0x9000, strvmstr_control_w },
	{ 0x9800, 0x9800, MWA_NOP }, /*always 0*/
	{ 0xa000, 0xa000, a000_w }, /*bit 8 and 7 always actived? bit 6 actived in message edit (c0, e0, df)*/
	{ 0xb000, 0xb000, b000_w },
	{ 0xb800, 0xb800, b800_w }, /*80, 83, 84, 86, 98, 94, 81, e0, a0*/
	{ 0xb801, 0xb801, MWA_NOP }, /*always 0*/
	{ 0xc000, 0xc7ff, strvmstr_fg_w, &fg_videoram },
	{ 0xe000, 0xe7ff, strvmstr_bg_w, &bg_videoram },	
MEMORY_END

static PORT_READ_START( readport )
	{ 0x0000, 0xffff, strvmstr_question_r },
PORT_END

INPUT_PORTS_START( strvmstr )
	PORT_START
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	/* cocktail controls? */
	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	
	PORT_START
	PORT_BIT_IMPULSE( 0x01, IP_ACTIVE_HIGH, IPT_COIN1, 1 )
INPUT_PORTS_END

static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,2),
	2,
	{ RGN_FRAC(0,2), RGN_FRAC(1,2) },
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,   0, 8 },
	{ REGION_GFX2, 0, &charlayout,   0, 8 },
	{ -1 }
};

static void get_tile_info_bg(int tile_index)
{
	int code = bg_videoram[tile_index];
	
	code += ((strvmstr_control & 4) << 6);

	SET_TILE_INFO(0,code,0,0)
}

static void get_tile_info_fg(int tile_index)
{
	int code = fg_videoram[tile_index];

	code += ((strvmstr_control & 4) << 6);

	SET_TILE_INFO(1,code,0,0)
}

VIDEO_START( strvmstr )
{
	bg_tilemap = tilemap_create( get_tile_info_bg,tilemap_scan_rows,TILEMAP_OPAQUE,8,8,64,32 );
	fg_tilemap = tilemap_create( get_tile_info_fg,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,32 );

	if( !bg_tilemap || !fg_tilemap )
		return 1;

	tilemap_set_transparent_pen(fg_tilemap,0);

	return 0;
}

VIDEO_UPDATE( strvmstr )
{
	tilemap_draw(bitmap,cliprect,bg_tilemap,0,0);
	tilemap_draw(bitmap,cliprect,fg_tilemap,0,0);
}

static struct AY8910interface ay8912_interface =
{
	3,					/* 3 chip */
	1500000,
	{ 25, 25, 25 },
	{ 0, input_port_1_r, input_port_0_r },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 }
};


static INTERRUPT_GEN( strvmstr_interrupt )
{
	if( readinputport(2) & 0x01 )
	{
		cpu_set_irq_line(0, IRQ_LINE_NMI, PULSE_LINE);
	}
	else
	{
		cpu_set_irq_line(0, 0, PULSE_LINE);
	}
}

#undef CLOCK
#define CLOCK 12000000/4-50000

static MACHINE_DRIVER_START( strvmstr )
	MDRV_CPU_ADD(Z80,CLOCK) /*should be ok, it gives the 300 interrupts expected*/
	MDRV_CPU_FLAGS(CPU_16BIT_PORT)
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_PORTS(readport,0)
	MDRV_CPU_VBLANK_INT(strvmstr_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(8)

	MDRV_VIDEO_START(strvmstr)
	MDRV_VIDEO_UPDATE(strvmstr)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8912_interface)
MACHINE_DRIVER_END

ROM_START( strvmstr )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "stm16.u16",    0x0000, 0x8000, CRC(ae734db9) SHA1(1bacdfdebaa1f250bfbd49053c3910f1396afe11) ) 
	
	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "stm44.rom",    0x0000, 0x1000, CRC(e69da710) SHA1(218a9d7600d67858d1f21282a0cebec0ae93e0ff) ) 
	ROM_LOAD( "stm46.rom",    0x1000, 0x1000, CRC(d927a1f1) SHA1(63a49a61107deaf7a9f28b9653c310c5331f5143) ) 

	ROM_REGION( 0x02000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "stm48.rom",    0x0000, 0x1000, CRC(51719714) SHA1(fdecbd22ea65eec7b4b5138f89ddc5876b05def6) ) 
	ROM_LOAD( "stm50.rom",    0x1000, 0x1000, CRC(cfc1a1d1) SHA1(9ef38f12360dd946651e67770742ca72fa6846f1) ) 

	ROM_REGION( 0x00200, REGION_PROMS, 0 )
	ROM_LOAD( "stm63.prm",    0x0000, 0x0100, CRC(305271cf) SHA1(6fd5fe085d79ca7aa57010cffbdb2a85b9c24701) ) 
	ROM_LOAD( "stm64.prm",    0x0100, 0x0100, CRC(69ebc0b8) SHA1(de2b936e3246e3bfc7e2ff9546c1854ec3504cc2) ) 

	ROM_REGION( 0x40000, REGION_USER1, 0 ) /* Question roms */
	ROM_LOAD( "sex2.lo0",     0x00000, 0x8000, CRC(9c68b277) SHA1(34bc9d7b973fe482abd5e34a058b72eb5ec8db64) ) 
	ROM_LOAD( "movies.lo1",   0x08000, 0x8000, CRC(16cba1b7) SHA1(8aa3eff72d1ec8dac906f2e803a88578a9fe763c) ) 
	ROM_LOAD( "sci-fi.lo2",   0x10000, 0x8000, CRC(b5595f81) SHA1(5e7fa334f6541860a5c04e5f345673ea12efafb4) ) 
	ROM_LOAD( "potprri.lo3",  0x18000, 0x8000, CRC(427eada9) SHA1(bac29ec637a17db95507c68fd73a8ce52744bf8e) ) 
	ROM_LOAD( "sports.hi0",   0x20000, 0x8000, CRC(3678fb79) SHA1(4e40cc20707195c0e88e595f752a2982b531b57e) ) 
	ROM_LOAD( "rock-pop.hi1", 0x28000, 0x8000, CRC(e2954db6) SHA1(d545236a844b63c85937ee8fb8e65bcd74b1bf43) ) 
	ROM_LOAD( "cars.hi2",     0x30000, 0x8000, CRC(50310557) SHA1(7559c603625e4df442b440b8b08e6efef06e2781) ) 
	ROM_LOAD( "entrtn.hi3",   0x38000, 0x8000, CRC(a8cf603b) SHA1(6efa5753d8d252452b3f5be8635a28364e4d8de1) ) 
ROM_END

GAMEX( 1986, strvmstr, 0, strvmstr, strvmstr, 0, ROT90, "Enerdyne Technologies Inc.", "Super Trivia Master", GAME_WRONG_COLORS | GAME_IMPERFECT_SOUND | GAME_NO_COCKTAIL )
