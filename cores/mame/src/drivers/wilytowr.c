/***************************************************************************

Wily Tower   (c) 1984 Irem

driver by Nicola Salmoria


Notes:
- Unless there is some special logic related to NMI enable, the game doesn't
  rely on vblank for timing. It all seems to be controlled by the CPU clock.
  The NMI handler just handles the "Stop Mode" dip switch.

TODO:
- Sound: it's difficult to guess how the I8039 is connected... there's also a
  OKI MSM80C39RS chip.
- One unknown ROM. Samples?
- Sprite positioning is wacky. The electric 'bands' that go along the pipes
  are drawn 2 pixels off in x/y directions. If you fix that, then the player
  sprite doesn't slide in the middle of the pipes when climbing...

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/i8039/i8039.h"


UINT8 *wilytowr_videoram2, *wilytowr_scrollram;

static int pal_bank;

static struct tilemap *bg_tilemap, *fg_tilemap;


PALETTE_INIT( wilytowr )
{
	int i;


	for (i = 0;i < 256;i++)
	{
		int bit0,bit1,bit2,bit3,r,g,b;

		/* red component */
		bit0 = (color_prom[i] >> 0) & 0x01;
		bit1 = (color_prom[i] >> 1) & 0x01;
		bit2 = (color_prom[i] >> 2) & 0x01;
		bit3 = (color_prom[i] >> 3) & 0x01;
		r =  0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		/* green component */
		bit0 = (color_prom[i + 256] >> 0) & 0x01;
		bit1 = (color_prom[i + 256] >> 1) & 0x01;
		bit2 = (color_prom[i + 256] >> 2) & 0x01;
		bit3 = (color_prom[i + 256] >> 3) & 0x01;
		g =  0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		/* blue component */
		bit0 = (color_prom[i + 2*256] >> 0) & 0x01;
		bit1 = (color_prom[i + 2*256] >> 1) & 0x01;
		bit2 = (color_prom[i + 2*256] >> 2) & 0x01;
		bit3 = (color_prom[i + 2*256] >> 3) & 0x01;
		b =  0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		palette_set_color(i,r,g,b);
	}

	color_prom += 3*256;

	for (i = 0;i < 4;i++)
	{
		int bit0,bit1,bit2,r,g,b;

		/* red component */
		bit0 = (color_prom[i] >> 0) & 0x01;
		bit1 = (color_prom[i] >> 1) & 0x01;
		bit2 = (color_prom[i] >> 2) & 0x01;
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = (color_prom[i] >> 3) & 0x01;
		bit1 = (color_prom[i] >> 4) & 0x01;
		bit2 = (color_prom[i] >> 5) & 0x01;
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = (color_prom[i] >> 6) & 0x01;
		bit1 = (color_prom[i] >> 7) & 0x01;
		b = 0x4f * bit0 + 0xa8 * bit1;

		palette_set_color(i+256,r,g,b);
	}
}

static WRITE_HANDLER( wilytowr_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

static WRITE_HANDLER( wilytowr_colorram_w )
{
	if (colorram[offset] != data)
	{
		colorram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

static WRITE_HANDLER( wilytowr_videoram2_w )
{
	if (wilytowr_videoram2[offset] != data)
	{
		wilytowr_videoram2[offset] = data;
		tilemap_mark_tile_dirty(fg_tilemap, offset);
	}
}

static WRITE_HANDLER( wilytwr_palbank_w )
{
	if (pal_bank != (data & 0x01))
	{
		pal_bank = data & 0x01;
		tilemap_mark_all_tiles_dirty(bg_tilemap);
	}
}

WRITE_HANDLER( wilytwr_flipscreen_w )
{
	if (flip_screen != (~data & 0x01))
	{
		flip_screen_set(~data & 0x01);
		tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
	}
}

static void get_bg_tile_info(int tile_index)
{
	int attr = colorram[tile_index];
	int code = videoram[tile_index] | ((attr & 0x30) << 4);
	int color = (attr & 0x0f) + (pal_bank << 4);

	SET_TILE_INFO(1, code, color, 0)
}

static void get_fg_tile_info(int tile_index)
{
	int code = wilytowr_videoram2[tile_index];

	SET_TILE_INFO(0, code, 0, 0)
}

VIDEO_START( wilytowr )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows, 
		TILEMAP_OPAQUE, 8, 8, 32, 32);

	if ( !bg_tilemap )
		return 1;

	fg_tilemap = tilemap_create(get_fg_tile_info, tilemap_scan_rows, 
		TILEMAP_TRANSPARENT, 8, 8, 32, 32);

	if ( !fg_tilemap )
		return 1;

	tilemap_set_scroll_cols(bg_tilemap, 32);
	tilemap_set_transparent_pen(fg_tilemap, 0);

	return 0;
}

static void wilytowr_draw_sprites( struct mame_bitmap *bitmap )
{
	int offs;

	for (offs = 0;offs < spriteram_size;offs += 4)
	{
		int code = spriteram[offs + 1];
		int color = (spriteram[offs + 2] & 0x0f) + (pal_bank << 4);
		int flipx = 0;
		int flipy = 0;
		int sx = spriteram[offs + 3];
		int sy = 238 - spriteram[offs];

		if (flip_screen)
		{
			sx = 240 - sx;
			sy = 238 - sy;
			flipx = !flipx;
			flipy = !flipy;
		}

		drawgfx(bitmap, Machine->gfx[2],
			code, color,
			flipx, flipy,
			sx, sy,
			&Machine->visible_area,
			TRANSPARENCY_PEN, 0);
	}
}

VIDEO_UPDATE( wilytowr )
{
	int col;

	for (col = 0; col < 32; col++)
		tilemap_set_scrolly(bg_tilemap, col, wilytowr_scrollram[col * 8]);

	tilemap_draw(bitmap, &Machine->visible_area, bg_tilemap, 0, 0);
	wilytowr_draw_sprites(bitmap);
	tilemap_draw(bitmap, &Machine->visible_area, fg_tilemap, 0, 0);
}


static WRITE_HANDLER( coin_w )
{
	coin_counter_w(offset, data & 0x01);
}


static WRITE_HANDLER( snd_irq_w )
{
	cpu_set_irq_line(1, 0, PULSE_LINE);
}


static int p1,p2;

static WRITE_HANDLER( snddata_w )
{
	if ((p2 & 0xf0) == 0xe0)
		AY8910_control_port_0_w(0,offset);
	else if ((p2 & 0xf0) == 0xa0)
		AY8910_write_port_0_w(0,offset);
	else if ((p1 & 0xe0) == 0x60)
		AY8910_control_port_1_w(0,offset);
	else if ((p1 & 0xe0) == 0x40)
		AY8910_write_port_1_w(0,offset);
	else /* if ((p2 & 0xf0) != 0x70)*/
		/* the port address is the data, while the data seems to be control bits */
		log_cb(RETRO_LOG_DEBUG, LOGPRE "%04x: snddata_w ctrl = %02x, p1 = %02x, p2 = %02x, data = %02x\n",activecpu_get_pc(),data,p1,p2,offset);
}

static WRITE_HANDLER( p1_w )
{
	p1 = data;
}

static WRITE_HANDLER( p2_w )
{
	p2 = data;
}


static MEMORY_READ_START( readmem )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xd000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xefff, MRA_RAM },
	{ 0xf800, 0xf800, input_port_0_r },
	{ 0xf801, 0xf801, input_port_1_r },
	{ 0xf802, 0xf802, input_port_2_r },
	{ 0xf806, 0xf806, input_port_3_r },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xd000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe1ff, MWA_RAM },
	{ 0xe200, 0xe2ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xe300, 0xe3ff, MWA_RAM, &wilytowr_scrollram },
	{ 0xe400, 0xe7ff, wilytowr_videoram2_w, &wilytowr_videoram2 },
	{ 0xe800, 0xebff, wilytowr_videoram_w, &videoram },
	{ 0xec00, 0xefff, wilytowr_colorram_w, &colorram },
	{ 0xf000, 0xf000, interrupt_enable_w },	/* NMI enable */
	{ 0xf002, 0xf002, wilytwr_flipscreen_w },
	{ 0xf003, 0xf003, wilytwr_palbank_w },
	{ 0xf006, 0xf007, coin_w },
	{ 0xf800, 0xf800, soundlatch_w },
	{ 0xf801, 0xf801, watchdog_reset_w },	/* unknown (cleared by NMI handler) */
	{ 0xf803, 0xf803, snd_irq_w },
MEMORY_END

static MEMORY_READ_START( i8039_readmem )
	{ 0x0000, 0x0fff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( i8039_writemem )
	{ 0x0000, 0x0fff, MWA_ROM },
MEMORY_END

static PORT_READ_START( i8039_readport )
/*	{ 0x00, 0xff, },*/
/*	{ I8039_t1, I8039_t1,  },*/
PORT_END

static PORT_WRITE_START( i8039_writeport )
	{ 0x00, 0xff, snddata_w },
	{ I8039_p1, I8039_p1, p1_w },
	{ I8039_p2, I8039_p2, p2_w },
PORT_END



INPUT_PORTS_START( wilytowr )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	/* TODO: support the different settings which happen in Coin Mode 2 */
	PORT_DIPNAME( 0xf0, 0x00, DEF_STR( Coinage ) ) /* mapped on coin mode 1 */
	PORT_DIPSETTING(    0x60, DEF_STR( 7C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_8C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_9C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( Free_Play ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x04, 0x00, "Coin Mode" )
	PORT_DIPSETTING(    0x00, "Mode 1" )
	PORT_DIPSETTING(    0x04, "Mode 2" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	/* In stop mode, press 1 to stop and 2 to restart */
	PORT_BITX   ( 0x10, 0x00, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Stop Mode", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_BITX(    0x40, 0x00, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_HIGH )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,2),
	2,
	{ RGN_FRAC(1,2), RGN_FRAC(0,2) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static struct GfxLayout tilelayout =
{
	8,8,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(2,3), RGN_FRAC(1,3), RGN_FRAC(0,3) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,6),
	3,
	{ RGN_FRAC(2,3), RGN_FRAC(1,3), RGN_FRAC(0,3) },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			RGN_FRAC(1,6)+0, RGN_FRAC(1,6)+1, RGN_FRAC(1,6)+2, RGN_FRAC(1,6)+3,
			RGN_FRAC(1,6)+4, RGN_FRAC(1,6)+5, RGN_FRAC(1,6)+6, RGN_FRAC(1,6)+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	16*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,   256, 1 },
	{ REGION_GFX2, 0, &tilelayout,     0, 32 },
	{ REGION_GFX3, 0, &spritelayout,   0, 32 },
	{ -1 } /* end of array */
};



static struct AY8910interface ay8910_interface =
{
	2,	/* 2 chips */
	3579545/4,	/* ??? using the same as other Irem games */
	{ 20, 20 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 }
};



static MACHINE_DRIVER_START( wilytowr )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,4000000)	/* 4 MHz ???? */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,1)

	MDRV_CPU_ADD(I8039,8000000/15)	/* ????? */
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(i8039_readmem,i8039_writemem)
	MDRV_CPU_PORTS(i8039_readport,i8039_writeport)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256+4)

	MDRV_PALETTE_INIT(wilytowr)
	MDRV_VIDEO_START(wilytowr)
	MDRV_VIDEO_UPDATE(wilytowr)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( wilytowr )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "wt4e.bin",     0x0000, 0x2000, CRC(a38e4b8a) SHA1(e296ba1764d3e8e2a5cc43bdde7f30a522b437ff) )
	ROM_LOAD( "wt4h.bin",     0x2000, 0x2000, CRC(c1405ceb) SHA1(c11dd4cd180bc9576e8042e1f56074620ea00f53) )
	ROM_LOAD( "wt4j.bin",     0x4000, 0x2000, CRC(379fb1c3) SHA1(677e4077f6d2140e4fb5c3d86bc7081d3b6cc028) )
	ROM_LOAD( "wt4k.bin",     0x6000, 0x2000, CRC(2dd6f9c7) SHA1(88ba58a1ddd25403211b7f920ba7006ed80c13eb) )
	ROM_LOAD( "wt_a-4m.bin",  0x8000, 0x2000, CRC(c1f8a7d5) SHA1(4307e7604aec728a1f5b0e6a0d6c9f4d37084da3) )
	ROM_LOAD( "wt_a-4n.bin",  0xa000, 0x2000, CRC(b212f7d2) SHA1(dd1c35559982e8bbcb0e778c733a3afb5b6611df) )

	ROM_REGION( 0x1000, REGION_CPU2, 0 )	/* 8039 */
	ROM_LOAD( "wt4d.bin",     0x0000, 0x1000, CRC(25a171bf) SHA1(7465dbfa8858d0f5822eb748b96d99753d58d243) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	/* '3' character is bad, but ROMs have been verified on four boards */
	ROM_LOAD( "wt_b-5e.bin",  0x0000, 0x1000, CRC(fe45df43) SHA1(9586a5728069e0c293bd17d4663305ce5758ca01) )
	ROM_LOAD( "wt_b-5f.bin",  0x1000, 0x1000, CRC(87a17eff) SHA1(cee2ba2889baf08dc6ee1c8e9150bd277f343be9) )

	ROM_REGION( 0x6000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "wtb5a.bin",    0x0000, 0x2000, CRC(efc1cbfa) SHA1(9a2ea29e64360ef7b143ac1b6a1ba3e672be4a42) )
	ROM_LOAD( "wtb5b.bin",    0x2000, 0x2000, CRC(ab4bfd07) SHA1(1d5010413989895c09d8e5ee903d665506836f94) )
	ROM_LOAD( "wtb5d.bin",    0x4000, 0x2000, CRC(40f23e1d) SHA1(abff583021e2cf2d2ec83adbbd4f2e96bfa3e04f) )

	ROM_REGION( 0x6000, REGION_GFX3, ROMREGION_DISPOSE )
	/* there are horizontal lines in some tiles, but ROMs have been verified on four boards */
	ROM_LOAD( "wt2j.bin",     0x0000, 0x1000, CRC(d1bf0670) SHA1(8d07bce354bb4538948c358fd696304a8e0640b8) )
	ROM_LOAD( "wt3k.bin",     0x1000, 0x1000, CRC(83c39a0e) SHA1(da98f887ac5c3d52281eece3d760c41fb9ecfd5c) )
	ROM_LOAD( "wt_a-3m.bin",  0x2000, 0x1000, CRC(e7e468ae) SHA1(17448191b440b668714d83730075938aaaf34b5a) )
	ROM_LOAD( "wt_a-3n.bin",  0x3000, 0x1000, CRC(0741d1a9) SHA1(51f5ee03db8a3f7afbf944b9e3e4ae12b2520269) )
	ROM_LOAD( "wt_a-3p.bin",  0x4000, 0x1000, CRC(7299f362) SHA1(5ba309d789df8432c08d67e4f9e8bf6c447fc425) )
	ROM_LOAD( "wt_a-3s.bin",  0x5000, 0x1000, CRC(9b37d50d) SHA1(a08d4a7654b815cb652be66dbaa097011327f5d5) )

	ROM_REGION( 0x1000, REGION_USER1, 0 )	/* unknown; sound? */
	ROM_LOAD( "wt_a-6d.bin",  0x0000, 0x1000, CRC(a5dde29b) SHA1(8f7545d2022da7c98d47112179dce717f6c3c5e2) )

	ROM_REGION( 0x0320, REGION_PROMS, 0 )
	ROM_LOAD( "wt_a-5s-.bpr", 0x0000, 0x0100, CRC(041950e7) SHA1(8276068bec3f4c5013c773033fca3cd3ed9e82ef) )	/* red */
	ROM_LOAD( "wt_a-5r-.bpr", 0x0100, 0x0100, CRC(bc04bf25) SHA1(37d0e89296760f51df5a0d434dca390fb60bb052) )	/* green */
	ROM_LOAD( "wt_a-5p-.bpr", 0x0200, 0x0100, CRC(ed819a19) SHA1(76f13dcf1674f136375738756e175ceec469d545) )	/* blue */
	ROM_LOAD( "wt_b-9l-.bpr", 0x0300, 0x0020, CRC(d2728744) SHA1(e6b1a570854ca90326414874432ab03ec85b9c8e) )	/* char palette */
ROM_END

ROM_START( atomboy )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "wt_a-4e.bin",  0x0000, 0x2000, CRC(f7978185) SHA1(6a108d1e9b1a81cedf865aba3998748dcf1d55ef) )
	ROM_LOAD( "wt_a-4h.bin",  0x2000, 0x2000, CRC(0ca9950b) SHA1(d6583fcdf17d16a8884932695caa9c5587a20795) )
	ROM_LOAD( "wt_a-4j.bin",  0x4000, 0x2000, CRC(1badbc65) SHA1(e0768f2cd7bbe8908fd68ff6d54dbef84cc7de4c) )
	ROM_LOAD( "wt_a-4k.bin",  0x6000, 0x2000, CRC(5a341f75) SHA1(9e1a180e37aaa0afbf8ff45219be40d3f75fe60a) )
	ROM_LOAD( "wt_a-4m.bin",  0x8000, 0x2000, CRC(c1f8a7d5) SHA1(4307e7604aec728a1f5b0e6a0d6c9f4d37084da3) )
	ROM_LOAD( "wt_a-4n.bin",  0xa000, 0x2000, CRC(b212f7d2) SHA1(dd1c35559982e8bbcb0e778c733a3afb5b6611df) )

	ROM_REGION( 0x1000, REGION_CPU2, 0 )	/* 8039 */
	ROM_LOAD( "wt_a-4d.bin",  0x0000, 0x1000, CRC(3d43361e) SHA1(2977df9f90d9d214909c56ab44c40ab45fd90675) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	/* '3' character is bad, but ROMs have been verified on four boards */
	ROM_LOAD( "wt_b-5e.bin",  0x0000, 0x1000, CRC(fe45df43) SHA1(9586a5728069e0c293bd17d4663305ce5758ca01) )
	ROM_LOAD( "wt_b-5f.bin",  0x1000, 0x1000, CRC(87a17eff) SHA1(cee2ba2889baf08dc6ee1c8e9150bd277f343be9) )

	ROM_REGION( 0x6000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "wt_b-5a.bin",  0x0000, 0x2000, CRC(da22c452) SHA1(bd921baa12087e996d07625e05eda00981608655) )
	ROM_LOAD( "wt_b-5b.bin",  0x2000, 0x2000, CRC(4fb25a1f) SHA1(0f90fb3b373760c33ba9be3b56b917eca92c9700) )
	ROM_LOAD( "wt_b-5d.bin",  0x4000, 0x2000, CRC(75be2604) SHA1(fe1f110e188aa34a04a9f43412a8308240391fcf) )

	ROM_REGION( 0x6000, REGION_GFX3, ROMREGION_DISPOSE )
	/* there are horizontal lines in some tiles, but ROMs have been verified on four boards */
	ROM_LOAD( "wt_a-3j.bin",  0x0000, 0x1000, CRC(b30ca38f) SHA1(885743893461b8617180a9723f6fcef160a2f05d) )
	ROM_LOAD( "wt_a-3k.bin",  0x1000, 0x1000, CRC(9a77eb73) SHA1(2564a3b3744b0be147b41c521fc7efde53bdfea7) )
	ROM_LOAD( "wt_a-3m.bin",  0x2000, 0x1000, CRC(e7e468ae) SHA1(17448191b440b668714d83730075938aaaf34b5a) )
	ROM_LOAD( "wt_a-3n.bin",  0x3000, 0x1000, CRC(0741d1a9) SHA1(51f5ee03db8a3f7afbf944b9e3e4ae12b2520269) )
	ROM_LOAD( "wt_a-3p.bin",  0x4000, 0x1000, CRC(7299f362) SHA1(5ba309d789df8432c08d67e4f9e8bf6c447fc425) )
	ROM_LOAD( "wt_a-3s.bin",  0x5000, 0x1000, CRC(9b37d50d) SHA1(a08d4a7654b815cb652be66dbaa097011327f5d5) )

	ROM_REGION( 0x1000, REGION_USER1, 0 )	/* unknown; sound? */
	ROM_LOAD( "wt_a-6d.bin",  0x0000, 0x1000, CRC(a5dde29b) SHA1(8f7545d2022da7c98d47112179dce717f6c3c5e2) )

	ROM_REGION( 0x0320, REGION_PROMS, 0 )
	ROM_LOAD( "wt_a-5s-.bpr", 0x0000, 0x0100, CRC(041950e7) SHA1(8276068bec3f4c5013c773033fca3cd3ed9e82ef) )	/* red */
	ROM_LOAD( "wt_a-5r-.bpr", 0x0100, 0x0100, CRC(bc04bf25) SHA1(37d0e89296760f51df5a0d434dca390fb60bb052) )	/* green */
	ROM_LOAD( "wt_a-5p-.bpr", 0x0200, 0x0100, CRC(ed819a19) SHA1(76f13dcf1674f136375738756e175ceec469d545) )	/* blue */
	ROM_LOAD( "wt_b-9l-.bpr", 0x0300, 0x0020, CRC(d2728744) SHA1(e6b1a570854ca90326414874432ab03ec85b9c8e) )	/* char palette */
ROM_END


GAMEX( 1984, wilytowr, 0,        wilytowr, wilytowr, 0, ROT180, "Irem",                    "Wily Tower", GAME_NO_SOUND )
GAMEX( 1985, atomboy,  wilytowr, wilytowr, wilytowr, 0, ROT180, "Irem (Memetron license)", "Atomic Boy", GAME_NO_SOUND )
