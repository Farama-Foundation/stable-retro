/*

Raiden 2 Preliminary Driver
based on Bryan McPhail's driver

could also probably support
 Zero Team
 Raiden DX

Not Working because of protection? banking?
Missing Sound
Tilemaps are Wrong
Inputs are wrong? (protection?)
Sprite Encryption
Sprite Ram Format

to get control of player 1 start a game with player 2 start then press player 1 start during the game
it will crash shortly afterwards tho

*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"
#include "sndhrdw/seibu.h"

static struct tilemap *background_layer,*midground_layer,*foreground_layer,*text_layer;
static unsigned char *back_data,*fore_data,*mid_data;

/* SPRITE DRAWING (move to vidhrdw file) */

static void draw_sprites( struct mame_bitmap *bitmap, const struct rectangle *cliprect ,int pri_mask )
{

	const UINT8 *source = spriteram+0x1000-8;
	const UINT8 *finish = spriteram;

	const struct GfxElement *gfx = Machine->gfx[1];

	while( source>=finish ){
		int tile_number = source[0];
		int sx = source[4];
		int sy = source[6];

		drawgfx(
			bitmap,
			gfx,
			tile_number,
			0,
			0,0,
			sx,sy,
			cliprect,
			TRANSPARENCY_PEN,0
		);

		source-=8;
	}

}

/* VIDEO RELATED WRITE HANDLERS (move to vidhrdw file) */

WRITE_HANDLER ( raiden2_background_w )
{
	back_data[offset]=data;
	tilemap_mark_tile_dirty( background_layer,offset/2 );
}

WRITE_HANDLER ( raiden2_midground_w )
{
	mid_data[offset]=data;
	tilemap_mark_tile_dirty( midground_layer,offset/2 );
}

WRITE_HANDLER ( raiden2_foreground_w )
{
	fore_data[offset]=data;
	tilemap_mark_tile_dirty( foreground_layer,offset/2 );
}

WRITE_HANDLER ( raiden2_text_w )
{
	videoram[offset]=data;
	tilemap_mark_tile_dirty( text_layer,offset/2 );
}

/* TILEMAP RELATED (move to vidhrdw file) */

static void get_back_tile_info( int tile_index )
{
	int offs=tile_index*2;
	int tile=back_data[offs]+(back_data[offs+1]<<8);
	int color=tile >> 12;

	tile=tile&0xfff;

	SET_TILE_INFO(1,tile+0x0000,color,0)
}

static void get_mid_tile_info( int tile_index )
{
	int offs=tile_index*2;
	int tile=mid_data[offs]+(mid_data[offs+1]<<8);
	int color=tile >> 12;

	tile=tile&0xfff;

	SET_TILE_INFO(1,tile+0x1000,color,0)
}

static void get_fore_tile_info( int tile_index )
{
	int offs=tile_index*2;
	int tile=fore_data[offs]+(fore_data[offs+1]<<8);
	int color=tile >> 12;

	tile=tile&0xfff;

	SET_TILE_INFO(1,tile+0x1000,color,0)
}

static void get_text_tile_info( int tile_index )
{
	int offs=tile_index*2;
	int tile=videoram[offs]+(videoram[offs+1]<<8);
	int color=(tile>>12)&0xf;

	tile&=0xfff;

	SET_TILE_INFO(0,tile,color,0)
}

/* VIDEO START (move to vidhrdw file) */

VIDEO_START(raiden2)
{
	text_layer       = tilemap_create( get_text_tile_info,tilemap_scan_rows, TILEMAP_TRANSPARENT, 8,8,  64,64 );
	background_layer = tilemap_create( get_back_tile_info,tilemap_scan_rows, TILEMAP_OPAQUE,      16,16,32,32 );
	midground_layer  = tilemap_create( get_mid_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 16,16,32,32 );
	foreground_layer = tilemap_create( get_fore_tile_info,tilemap_scan_rows, TILEMAP_TRANSPARENT, 16,16,32,32 );

	tilemap_set_transparent_pen(midground_layer,15);
	tilemap_set_transparent_pen(foreground_layer,15);
	tilemap_set_transparent_pen(text_layer,15);

	return 0;
}

/* VIDEO UPDATE (move to vidhrdw file) */

VIDEO_UPDATE (raiden2)
{
	tilemap_draw(bitmap,cliprect,background_layer,0,0);
	tilemap_draw(bitmap,cliprect,midground_layer,0,0);
	tilemap_draw(bitmap,cliprect,foreground_layer,0,0);
	draw_sprites(bitmap,cliprect,0);
	tilemap_draw(bitmap,cliprect,text_layer,0,0);
}

/* MISC READ HANDLERS */

static READ_HANDLER ( raiden2_kludge_r )
{
	return 0xff;
}

/* MEMORY MAPS */

static MEMORY_READ_START( raiden2_readmem )
	{ 0x00000, 0x003ff, MRA_RAM },

	/* I have my doubts these are really mapped here, protection? */
	{ 0x00740, 0x00740, input_port_2_r }, /* dip 1*/
	{ 0x00741, 0x00741, input_port_3_r }, /* dip 2*/
	{ 0x00744, 0x00744, input_port_0_r }, /* player 1*/
	{ 0x00745, 0x00745, input_port_1_r }, /* player 2*/
	{ 0x0074b, 0x0074d, input_port_4_r }, /* start buttons*/
	{ 0x00400, 0x007ff, raiden2_kludge_r },

	{ 0x00800, 0x0afff, MRA_RAM },

	{ 0x0b000, 0x0bfff, MRA_RAM }, /* protection?*/

	{ 0x0c000, 0x0cfff, MRA_RAM }, /* sprites*/
	{ 0x0d000, 0x0d7ff, MRA_RAM }, /* background*/
	{ 0x0d800, 0x0dfff, MRA_RAM }, /* middle*/
	{ 0x0e800, 0x0f7ff, MRA_RAM }, /* front*/
	{ 0x0f800, 0x0ffff, MRA_RAM }, /* Stack area */

	{ 0x10000, 0x1efff, MRA_RAM },

	{ 0x1f000, 0x1ffff, MRA_RAM }, /* palette*/

	{ 0x20000, 0x3ffff, MRA_BANK1 }, /* rom*/
	{ 0x40000, 0xfffff, MRA_BANK2 }, /* rom*/
MEMORY_END

static MEMORY_WRITE_START( raiden2_writemem )
	{ 0x00000, 0x003ff, MWA_RAM },
/*	{ 0x00400, 0x007ff, MWA_RAM },*/
	{ 0x00800, 0x0afff, MWA_RAM },

	{ 0x0b000, 0x0bfff, MWA_RAM }, /* protection?*/

	{ 0x0c000, 0x0cfff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x0d000, 0x0d7ff, raiden2_background_w, &back_data },
	{ 0x0d800, 0x0dfff, raiden2_midground_w, &mid_data },
	{ 0x0e000, 0x0e7ff, raiden2_foreground_w, &fore_data },
	{ 0x0e800, 0x0f7ff, raiden2_text_w, &videoram },
	{ 0x0f800, 0x0ffff, MWA_RAM }, /* Stack area */

	{ 0x10000, 0x1efff, MWA_RAM },

	{ 0x1f000, 0x1ffff, paletteram_xBBBBBGGGGGRRRRR_w, &paletteram },

	{ 0x20000, 0xfffff, MWA_ROM },
MEMORY_END

/* INPUT PORTS */

INPUT_PORTS_START( raiden2 )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* Dip switch A - correct OB 031500 */
	/*
	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Coin_A ))
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ))
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ))
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ))
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_2C ))
	PORT_DIPSETTING(    0x04, DEF_STR( 4C_1C ))
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ))
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_4C ))
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	*/
	PORT_DIPNAME( 0x40, 0x40, "Starting Coin" )
	PORT_DIPSETTING(    0x40, "normal" )
	PORT_DIPSETTING(    0x00, "X 2" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* Dip switch B - correct OB 031500 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Difficulty ))
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x01, "Easy" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x00, "Very Hard" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Lives ))
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPNAME( 0x40, 0x00, "Demo Sound" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "Test Mode" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* START BUTTONS */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
INPUT_PORTS_END

/* GFX DECODING */

static struct GfxLayout raiden2_charlayout =
{
	8,8,
	4096,
	4,
	{ 8,12,0,4 },
	{ 3,2,1,0,19,18,17,16 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};


static struct GfxLayout raiden2_tilelayout =
{
	16,16,
	0x4000,
	4,
	{ 8,12,0,4 },
	{
		3,2,1,0,
		19,18,17,16,
		3+64*8, 2+64*8, 1+64*8, 0+64*8,
		19+64*8,18+64*8,17+64*8,16+64*8,
	},
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	128*8
};

static struct GfxDecodeInfo raiden2_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x00000, &raiden2_charlayout, 1792, 256 },
	{ REGION_GFX2, 0x00000, &raiden2_tilelayout, 0x400, 256 },
	{ REGION_GFX3, 0x00000, &raiden2_tilelayout, 0x400, 256 },
	{ REGION_GFX3, 0x00000, &raiden2_tilelayout, 512, 16 },
	{ -1 } /* end of array */
};

/* INTERRUPTS */

static INTERRUPT_GEN( raiden2_interrupt )
{
	cpu_set_irq_line_and_vector(cpu_getactivecpu(), 0, HOLD_LINE, 0xc0/4);	/* VBL */
}

/* MACHINE DRIVERS */

static MACHINE_DRIVER_START( raiden2 )

	/* basic machine hardware */
	MDRV_CPU_ADD(V30,32000000/2) /* NEC V30 CPU, 32? Mhz */
	MDRV_CPU_MEMORY(raiden2_readmem,raiden2_writemem)
	MDRV_CPU_VBLANK_INT(raiden2_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE(64*8, 64*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 0*8, 30*8-1)
	MDRV_GFXDECODE(raiden2_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(raiden2)
	MDRV_VIDEO_UPDATE(raiden2)
MACHINE_DRIVER_END

/* ROM LOADING */

ROM_START( raiden2 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) /* v30 main cpu */
	ROM_LOAD16_BYTE("prg0",   0x100000, 0x80000, CRC(09475ec4) SHA1(05027f2d8f9e11fcbd485659eda68ada286dae32) )
	ROM_LOAD16_BYTE("prg1",   0x100001, 0x80000, CRC(4609b5f2) SHA1(272d2aa75b8ea4d133daddf42c4fc9089093df2e) )

	ROM_REGION( 0x20000, REGION_CPU2, 0 ) /* 64k code for sound Z80 */
	ROM_LOAD( "snd",  0x000000, 0x10000, CRC(f51a28f9) SHA1(7ae2e2ba0c8159a544a8fd2bb0c2c694ba849302) )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE ) /* chars */
	ROM_LOAD( "px0",	0x000000,	0x020000,	CRC(c9ec9469) SHA1(a29f480a1bee073be7a177096ef58e1887a5af24) )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE ) /* background gfx */
	ROM_LOAD( "bg1",   0x000000, 0x200000, CRC(e61ad38e) SHA1(63b06cd38db946ad3fc5c1482dc863ef80b58fec) )
	ROM_LOAD( "bg2",   0x200000, 0x200000, CRC(a694a4bb) SHA1(39c2614d0effc899fe58f735604283097769df77) )

	ROM_REGION( 0x800000, REGION_GFX3, ROMREGION_DISPOSE ) /* sprite gfx (encrypted) */
	ROM_LOAD( "obj1",  0x000000, 0x200000, CRC(ff08ef0b) SHA1(a1858430e8171ca8bab785457ef60e151b5e5cf1) )
	ROM_LOAD( "obj2",  0x200000, 0x200000, CRC(638eb771) SHA1(9774cc070e71668d7d1d20795502dccd21ca557b) )
	ROM_LOAD( "obj3",  0x400000, 0x200000, CRC(897a0322) SHA1(abb2737a2446da5b364fc2d96524b43d808f4126) )
	ROM_LOAD( "obj4",  0x600000, 0x200000, CRC(b676e188) SHA1(19cc838f1ccf9c4203cd0e5365e5d99ff3a4ff0f) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "voi1", 0x00000, 0x80000, CRC(f340457b) SHA1(8169acb24c82f68d223a31af38ee36eb6cb3adf4) )
	ROM_LOAD( "voi2", 0x80000, 0x80000, CRC(d321ff54) SHA1(b61e602525f36eb28a1408ffb124abfbb6a08706) )
ROM_END

/* INIT */


static DRIVER_INIT (raiden2)
{
	/* wrong , there must be some banking this just stops it crashing */
	unsigned char *RAM = memory_region(REGION_CPU1);

	cpu_setbank(1,&RAM[0x100000]);
	cpu_setbank(2,&RAM[0x140000]);
}

/* GAME DRIVERS */

GAMEX( 1993, raiden2,  0,      raiden2,  raiden2, raiden2,  ROT270, "Seibu Kaihatsu", "Raiden 2", GAME_NO_SOUND | GAME_NOT_WORKING )
