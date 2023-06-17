/*
 05/01/2003  MooglyGuy/Ryan Holtz
	- Corrected second AY (shouldn't have been there)
	- Added first AY's status read
	- Added coinage DIP
	- What the hell are those unmapped port writes!? Not AY...

 2003.01.01. Tomasz Slanina 
  
  changes :
 	- nmi generation ( incorrect freq probably)
 	- music/sfx (partially)
 	- more sprite tiles (twice than before)
 	- fixed sprites flips 
 	- scrolling (2nd game level)
 	- better colors (weird 'hack' .. but works in most cases ( comparing with screens from emustatus ))
 	- dips - lives 
 	- visible area .. a bit smaller (at least bg 'generation' is not visible for scrolling levels )
 	- cpu clock .. now 4 mhz 
*/


#include "driver.h"
#include "vidhrdw/generic.h"

data8_t *skyarmy_videoram;
data8_t *skyarmy_colorram;
data8_t *skyarmy_scrollram;
static struct tilemap* skyarmy_tilemap;

static void get_skyarmy_tile_info(int tile_index)
{
	int code = skyarmy_videoram[tile_index];
	int attr = skyarmy_colorram[tile_index];
        
	/* bit 0 <-> bit 2 ????? */
	switch(attr)
	{
		case 1: attr=4; break;
		case 3: attr=6; break;
		case 4: attr=1; break;
		case 6: attr=3; break;
	}

	SET_TILE_INFO( 0, code, attr, 0)
}

WRITE_HANDLER( skyarmy_videoram_w )
{
        skyarmy_videoram[offset] = data;
        tilemap_mark_tile_dirty(skyarmy_tilemap,offset);
}

WRITE_HANDLER( skyarmy_colorram_w )
{
        skyarmy_colorram[offset] = data;
        tilemap_mark_tile_dirty(skyarmy_tilemap,offset);
}

WRITE_HANDLER( skyarmy_scrollram_w )
{
        skyarmy_scrollram[offset] = data;
}


READ_HANDLER( skyarmy_scrollram_r )
{
        return skyarmy_scrollram[offset];
}


PALETTE_INIT( skyarmy )
{
	int i;

	for (i = 0;i < 32;i++)
	{
		int bit0,bit1,bit2,r,g,b;
	
		bit0 = (*color_prom >> 0) & 0x01;
		bit1 = (*color_prom >> 1) & 0x01;
		bit2 = (*color_prom >> 2) & 0x01;
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
	
		bit0 = (*color_prom >> 3) & 0x01;
		bit1 = (*color_prom >> 4) & 0x01;
		bit2 = (*color_prom >> 5) & 0x01;
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
	
		bit0=0;
		bit1 = (*color_prom >> 6) & 0x01;
		bit2 = (*color_prom >> 7) & 0x01;
		b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		palette_set_color(i,r,g,b);
		color_prom++;
	}
}

VIDEO_START( skyarmy )
{
	skyarmy_tilemap = tilemap_create(get_skyarmy_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,8,8,32,32);
	tilemap_set_scroll_cols(skyarmy_tilemap,32);

	if(!skyarmy_tilemap)
		return 1;

	return 0;
}


VIDEO_UPDATE( skyarmy )
{
	int sx, sy, flipx, flipy, offs,pal;
	int i;

	for(i=0;i<0x20;i++)tilemap_set_scrolly( skyarmy_tilemap,i,skyarmy_scrollram[i]);

	tilemap_draw(bitmap,cliprect,skyarmy_tilemap,0,0);

	for (offs = 0 ; offs < 0x40; offs+=4)
	{

		pal=spriteram[offs+2]&0x7;

		switch(pal)
		{
		 case 1: pal=4; break;
		 case 2: pal=2; break;
	 	 case 3: pal=6; break;
		 case 4: pal=1; break;
		 case 6: pal=3; break;
		}
		sx = spriteram[offs+3];
		sy = 242-spriteram[offs];
		flipy = (spriteram[offs+1]&0x80)>>7;
		flipx = (spriteram[offs+1]&0x40)>>6;
		drawgfx(bitmap,Machine->gfx[1],
		spriteram[offs+1]&0x3f,
		pal,
		flipx,flipy,
		sx,sy,
		cliprect,TRANSPARENCY_PEN,0);
	}
}

static int skyarmy_nmi=0;

static INTERRUPT_GEN( skyarmy_nmi_source )
{
	 if(skyarmy_nmi) cpu_set_irq_line(0,IRQ_LINE_NMI, PULSE_LINE)	;
}


WRITE_HANDLER( nmi_enable_w )
{
        skyarmy_nmi=data&1;
}


static MEMORY_READ_START( skyarmy_readmem )
        { 0x0000, 0x7FFF, MRA_ROM },
        { 0x8000, 0x87FF, MRA_RAM },
        { 0x8800, 0x93FF, MRA_RAM }, /* Video RAM */
        { 0x9800, 0x983F, MRA_RAM }, /* Sprites */
        { 0x9840, 0x985F, skyarmy_scrollram_r }, /* Sroll RAM */
        { 0xA000, 0xA000, input_port_0_r },
        { 0xA001, 0xA001, input_port_1_r },
        { 0xA002, 0xA002, input_port_2_r },
        { 0xA003, 0xA003, input_port_3_r },
MEMORY_END

static MEMORY_WRITE_START( skyarmy_writemem )
        { 0x0000, 0x7FFF, MWA_ROM },
        { 0x8000, 0x87FF, MWA_RAM },
        { 0x8800, 0x8BFF, skyarmy_videoram_w, &skyarmy_videoram }, /* Video RAM */
        { 0x9000, 0x93FF, skyarmy_colorram_w, &skyarmy_colorram }, /* Color RAM */
        { 0x9800, 0x983F, spriteram_w, &spriteram, &spriteram_size }, /* Sprites */
        { 0x9840, 0x985F, skyarmy_scrollram_w, &skyarmy_scrollram }, /* Sprites */
        { 0xa004, 0xa004, nmi_enable_w }, /* ???*/
        { 0xa005, 0xa005, MWA_NOP }, 
        { 0xa006, 0xa006, MWA_NOP }, 
        { 0xa007, 0xa007, MWA_NOP }, 
MEMORY_END

INPUT_PORTS_START( skyarmy )
	PORT_START
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x03, DEF_STR ( Free_Play ) )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "10000" )
	PORT_DIPSETTING(    0x04, "20000" )
	PORT_DIPSETTING(    0x08, "30000" )
	PORT_DIPSETTING(    0x0c, "40000" )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0xC0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0xC0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

static struct GfxLayout charlayout =
{
	8,8,
        256,
        2,
        { 0, 256*8*8 },
        { 0, 1, 2, 3, 4, 5, 6, 7 },
        { 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
        8*8
};

static struct GfxLayout spritelayout =
{
        16,16,
        32*2,
        2,
        { 0, 256*8*8 },
        { 0, 1, 2, 3, 4, 5, 6, 7,
          8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
        { 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
         16*8,17*8,18*8,19*8,20*8,21*8,22*8,23*8 },
        32*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,   0, 8 },
	{ REGION_GFX2, 0, &spritelayout, 0, 8 },
	{ -1 } /* end of array */
};

static PORT_READ_START( readport )
	{ 0x06, 0x06, AY8910_read_port_0_r },
PORT_END

static PORT_WRITE_START( writeport )
	{ 0x04, 0x04, AY8910_control_port_0_w },
	{ 0x05, 0x05, AY8910_write_port_0_w   },
PORT_END

static struct AY8910interface ay8910_interface =
{
	1, /* number of chips */
	2500000, /* 2.5 MHz ??? */
	{ 15 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static MACHINE_DRIVER_START( skyarmy )
	MDRV_CPU_ADD(Z80,4000000)
	MDRV_CPU_MEMORY(skyarmy_readmem,skyarmy_writemem)
	MDRV_CPU_PORTS(readport,writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_pulse,1)
	MDRV_CPU_PERIODIC_INT(skyarmy_nmi_source,650)	/* Hz */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8,32*8)
	MDRV_VISIBLE_AREA(0*8,32*8-1,1*8,31*8-1) 
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32)

	MDRV_PALETTE_INIT(skyarmy)
	MDRV_VIDEO_START(skyarmy)
	MDRV_VIDEO_UPDATE(skyarmy)
        
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END


ROM_START( skyarmy )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "a1h.bin", 0x0000, 0x2000, CRC(46507488) SHA1(a9a43caa6f3eab48c3fd9e2f34fe2b4aacfd4641) )
	ROM_LOAD( "a2h.bin", 0x2000, 0x2000, CRC(0417653e) SHA1(4f6ad7335b5b7e85b4e16cce3c127488c02401b2) )
	ROM_LOAD( "a3h.bin", 0x4000, 0x2000, CRC(95485e56) SHA1(c4cbcd31ba68769d2d0d0875e2a92982265339ae) )
	ROM_LOAD( "j4.bin",  0x6000, 0x2000, CRC(843783df) SHA1(256d8375a8af7de080d456dbc6290a22473d011b) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "13b.bin", 0x0000, 0x0800, CRC(3b0e0f7c) SHA1(2bbba10121d3e745146f50c14dc6df97de40fb96) )
	ROM_LOAD( "15b.bin", 0x0800, 0x0800, CRC(5ccfd782) SHA1(408406ae068e5578b8a742abed1c37dcd3720fe5) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "8b.bin",  0x0000, 0x0800, CRC(6ac6bd98) SHA1(e653d80ec1b0f8e07821ea781942dae3de7d238d) )
	ROM_LOAD( "10b.bin", 0x0800, 0x0800, CRC(cada7682) SHA1(83ce8336274cb8006a445ac17a179d9ffd4d6809) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "a6.bin",  0x0000, 0x0020, CRC(c721220b) SHA1(61b3320fb616c0600d56840cb6438616c7e0c6eb) )
ROM_END

GAMEX( 1982, skyarmy, 0, skyarmy, skyarmy, 0, ROT90, "Shoei", "Sky Army", GAME_NO_COCKTAIL )
