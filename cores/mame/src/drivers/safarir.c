/****************************************************************************

Safari Rally by SNK/Taito

Driver by Zsolt Vasvari


This hardware is a precursor to Phoenix.

----------------------------------

CPU board

76477        18MHz

              8080

Video board


 RL07  2114
       2114
       2114
       2114
       2114           RL01 RL02
       2114           RL03 RL04
       2114           RL05 RL06
 RL08  2114

11MHz

----------------------------------

TODO:

- SN76477 sound

****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


static UINT8 *safarir_ram1, *safarir_ram2;
static size_t safarir_ram_size;
static UINT8 safarir_ram_bank;

static struct tilemap *bg_tilemap, *fg_tilemap;


WRITE_HANDLER( safarir_ram_w )
{
	if (safarir_ram_bank)
		safarir_ram2[offset] = data;
	else
		safarir_ram1[offset] = data;

	tilemap_mark_tile_dirty((offset & 0x0400) ? bg_tilemap : fg_tilemap, offset & 0x03ff);
}

READ_HANDLER( safarir_ram_r )
{
	return safarir_ram_bank ? safarir_ram2[offset] : safarir_ram1[offset];
}

WRITE_HANDLER( safarir_scroll_w )
{
	tilemap_set_scrollx(bg_tilemap, 0, data);
}

WRITE_HANDLER( safarir_ram_bank_w )
{
	safarir_ram_bank = data & 0x01;

	tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
}

static void get_bg_tile_info(int tile_index)
{
	int color;
	UINT8 code = safarir_ram_r(tile_index | 0x400);

	if (code & 0x80)
		color = 6;	/* yellow */
	else
	{
		color = ((~tile_index & 0x04) >> 2) | ((tile_index & 0x04) >> 1);

		if (~tile_index & 0x100)
			color |= ((tile_index & 0xc0) == 0x80) ? 1 : 0;
		else
			color |= (tile_index & 0xc0) ? 1 : 0;
	}

	SET_TILE_INFO(0, code & 0x7f, color, 0)
}

static void get_fg_tile_info(int tile_index)
{
	int color, flags;
	UINT8 code = safarir_ram_r(tile_index);

	if (code & 0x80)
		color = 7;	/* white */
	else
		color = (~tile_index & 0x04) | ((tile_index >> 1) & 0x03);

	flags = ((tile_index & 0x1f) >= 0x03) ? 0 : TILE_IGNORE_TRANSPARENCY;

	SET_TILE_INFO(1, code & 0x7f, color, flags)
}

VIDEO_START( safarir )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows, 
		TILEMAP_OPAQUE, 8, 8, 32, 32);

	if (!bg_tilemap)
		return 1;

	fg_tilemap = tilemap_create(get_fg_tile_info, tilemap_scan_rows, 
		TILEMAP_TRANSPARENT, 8, 8, 32, 32);

	if (!fg_tilemap)
		return 1;

	tilemap_set_transparent_pen(fg_tilemap, 0);

	return 0;
}

VIDEO_UPDATE( safarir )
{
	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);
	tilemap_draw(bitmap, cliprect, fg_tilemap, 0, 0);
}


/*************************************
 *
 *  Audio system
 *
 *************************************/

#define SAMPLE_SOUND1_1		0
#define SAMPLE_SOUND1_2		1
#define SAMPLE_SOUND2		2
#define SAMPLE_SOUND3		3
#define SAMPLE_SOUND4_1		4
#define SAMPLE_SOUND4_2		5
#define SAMPLE_SOUND5_1		6
#define SAMPLE_SOUND5_2		7
#define SAMPLE_SOUND6		8
#define SAMPLE_SOUND7		9
#define SAMPLE_SOUND8		10

#define CHANNEL_SOUND1		0
#define CHANNEL_SOUND2		1
#define CHANNEL_SOUND3		2
#define CHANNEL_SOUND4		3
#define CHANNEL_SOUND5		4
#define CHANNEL_SOUND6		5

static UINT8 port_last;
static UINT8 port_last2;


WRITE_HANDLER( safarir_audio_w )
{
	UINT8 rising_bits = data & ~port_last;

	if (rising_bits == 0x12) sample_start(CHANNEL_SOUND1, SAMPLE_SOUND1_1, 0);
	if (rising_bits == 0x02) sample_start(CHANNEL_SOUND1, SAMPLE_SOUND1_2, 0);
	if (rising_bits == 0x95) sample_start(CHANNEL_SOUND1, SAMPLE_SOUND6, 0);

	if (rising_bits == 0x04 && (data == 0x15 || data ==0x16)) sample_start(CHANNEL_SOUND2, SAMPLE_SOUND2, 0);

	if (data == 0x5f && (rising_bits == 0x49 || rising_bits == 0x5f)) sample_start(CHANNEL_SOUND3, SAMPLE_SOUND3, 1);
	if (data == 0x00 || rising_bits == 0x01) sample_stop(CHANNEL_SOUND3);

	if (data == 0x13)
	{
		if ((rising_bits == 0x13 && port_last != 0x04) || (rising_bits == 0x01 && port_last == 0x12))
		{
			sample_start(CHANNEL_SOUND4, SAMPLE_SOUND7, 0);
		}
		else if (rising_bits == 0x03 && port_last2 == 0x15 && !sample_playing(CHANNEL_SOUND4))
		{
			sample_start(CHANNEL_SOUND4, SAMPLE_SOUND4_1, 0);
		}
	}
	if (data == 0x53 && port_last == 0x55) sample_start(CHANNEL_SOUND4, SAMPLE_SOUND4_2, 0);

	if (data == 0x1f && rising_bits == 0x1f) sample_start(CHANNEL_SOUND5, SAMPLE_SOUND5_1, 0);
	if (data == 0x14 && (rising_bits == 0x14 || rising_bits == 0x04)) sample_start(CHANNEL_SOUND5, SAMPLE_SOUND5_2, 0);

	if (data == 0x07 && rising_bits == 0x07 && !sample_playing(CHANNEL_SOUND6))
		sample_start(CHANNEL_SOUND6, SAMPLE_SOUND8, 0);

	port_last2 = port_last;
	port_last = data;
}


static const char *safarir_sample_names[] =
{
	"*safarir",
	"sound1-1.wav",
	"sound1-2.wav",
	"sound2.wav",
	"sound3.wav",
	"sound4-1.wav",
	"sound4-2.wav",
	"sound5-1.wav",
	"sound5-2.wav",
	"sound6.wav",
	"sound7.wav",
	"sound8.wav",
	0
};


struct Samplesinterface safarir_samples_interface =
{
	6,	/* 6 channels */
	50,	/* volume */
	safarir_sample_names
};


static PALETTE_INIT( safarir )
{
	int i;

	for (i = 0; i < Machine->drv->total_colors; i++)
	{
		palette_set_color(i, pal1bit(i >> 2), pal1bit(i >> 1), pal1bit(i >> 0));

		colortable[(i * 2) + 0] = 0;
		colortable[(i * 2) + 1] = i;
	}
}


static MEMORY_READ_START( readmem )
	{ 0x0000, 0x17ff, MRA_ROM },
	{ 0x2000, 0x27ff, safarir_ram_r },
	{ 0x3800, 0x38ff, input_port_0_r },
	{ 0x3c00, 0x3cff, input_port_1_r },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x17ff, MWA_ROM },
	{ 0x2000, 0x27ff, safarir_ram_w, &safarir_ram1, &safarir_ram_size },
	{ 0x2800, 0x28ff, safarir_ram_bank_w },
	{ 0x2c00, 0x2cff, safarir_scroll_w },
	{ 0x3000, 0x30ff, safarir_audio_w },	/* goes to SN76477 */
	{ 0x8000, 0x87ff, MWA_NOP, &safarir_ram2 },	/* only here to initialize pointer */
MEMORY_END


INPUT_PORTS_START( safarir )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x0c, 0x04, "Acceleration Rate" )
	PORT_DIPSETTING(    0x00, "Slowest" )
	PORT_DIPSETTING(    0x04, "Slow" )
	PORT_DIPSETTING(    0x08, "Fast" )
	PORT_DIPSETTING(    0x0c, "Fastest" )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "3000" )
	PORT_DIPSETTING(    0x20, "5000" )
	PORT_DIPSETTING(    0x40, "7000" )
	PORT_DIPSETTING(    0x60, "9000" )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 chars */
	128,	/* 128 characters */
	1,		/* 1 bit per pixel */
	{ 0 },
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0, 8 },
	{ REGION_GFX2, 0, &charlayout, 0, 8 },
	{ -1 } /* end of array */
};

/* the following is copied from spaceinv */
struct SN76477interface safarir_sn76477_interface =
{
	1,	/* 1 chip */
	{ 25 },  /* mixing level   pin description		 */
	{ 0	/* N/C */},		/*	4  noise_res		 */
	{ 0	/* N/C */},		/*	5  filter_res		 */
	{ 0	/* N/C */},		/*	6  filter_cap		 */
	{ 0	/* N/C */},		/*	7  decay_res		 */
	{ 0	/* N/C */},		/*	8  attack_decay_cap  */
	{ RES_K(100) },		/* 10  attack_res		 */
	{ RES_K(56)  },		/* 11  amplitude_res	 */
	{ RES_K(10)  },		/* 12  feedback_res 	 */
	{ 0	/* N/C */},		/* 16  vco_voltage		 */
	{ CAP_U(0.1) },		/* 17  vco_cap			 */
	{ RES_K(8.2) },		/* 18  vco_res			 */
	{ 5.0		 },		/* 19  pitch_voltage	 */
	{ RES_K(120) },		/* 20  slf_res			 */
	{ CAP_U(1.0) },		/* 21  slf_cap			 */
	{ 0	/* N/C */},		/* 23  oneshot_cap		 */
	{ 0	/* N/C */}		/* 24  oneshot_res		 */
};


static MACHINE_DRIVER_START( safarir )

	/* basic machine hardware */
	MDRV_CPU_ADD(8080, 18000000/8)	/* 2.25 MHz ? */
	MDRV_CPU_MEMORY(readmem, writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 26*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(8)
	MDRV_COLORTABLE_LENGTH(2*8)

	MDRV_PALETTE_INIT(safarir)
	MDRV_VIDEO_START(safarir)
	MDRV_VIDEO_UPDATE(safarir)

	/* sound hardware */
	MDRV_SOUND_ADD(SAMPLES, safarir_samples_interface)
MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( safarirj )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "rl-01.9",  0x0000, 0x0400, CRC(cf7703c9) SHA1(b4182df9332b355edaa518462217a6e31e1c07b2) )
	ROM_LOAD( "rl-02.1",  0x0400, 0x0400, CRC(1013ecd3) SHA1(2fe367db8ca367b36c5378cb7d5ff918db243c78) )
	ROM_LOAD( "rl-03.10", 0x0800, 0x0400, CRC(84545894) SHA1(377494ceeac5ad58b70f77b2b27b609491cb7ffd) )
	ROM_LOAD( "rl-04.2",  0x0c00, 0x0400, CRC(5dd12f96) SHA1(a80ac0705648f0807ea33e444fdbea450bf23f85) )
	ROM_LOAD( "rl-05.11", 0x1000, 0x0400, CRC(935ed469) SHA1(052a59df831dcc2c618e9e5e5fdfa47548550596) )
	ROM_LOAD( "rl-06.3",  0x1400, 0x0400, CRC(24c1cd42) SHA1(fe32ecea77a3777f8137ca248b8f371db37b8b85) )

	ROM_REGION( 0x0400, REGION_GFX1, 0 )
	ROM_LOAD( "rl-08.43", 0x0000, 0x0400, CRC(d6a50aac) SHA1(0a0c2cefc556e4e15085318fcac485b82bac2416) )

	ROM_REGION( 0x0400, REGION_GFX2, 0 )
	ROM_LOAD( "rl-07.40", 0x0000, 0x0400, CRC(ba525203) SHA1(1c261cc1259787a7a248766264fefe140226e465) )
ROM_END

ROM_START( safarir ) /* Taito PCB, labels are the same as Japan ver. */
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "rl-01.9",  0x0000, 0x0400, CRC(cf7703c9) SHA1(b4182df9332b355edaa518462217a6e31e1c07b2) )
	ROM_LOAD( "rl-02.1",  0x0400, 0x0400, CRC(1013ecd3) SHA1(2fe367db8ca367b36c5378cb7d5ff918db243c78) )
	ROM_LOAD( "rl-03.10", 0x0800, 0x0400, CRC(84545894) SHA1(377494ceeac5ad58b70f77b2b27b609491cb7ffd) )
	ROM_LOAD( "rl-04.2",  0x0c00, 0x0400, CRC(5dd12f96) SHA1(a80ac0705648f0807ea33e444fdbea450bf23f85) )
	ROM_LOAD( "rl-09.11", 0x1000, 0x0400, CRC(d066b382) SHA1(c82ec668f1ed2246c12a1371ee4a2c070f57a9c2) )
	ROM_LOAD( "rl-06.3",  0x1400, 0x0400, CRC(24c1cd42) SHA1(fe32ecea77a3777f8137ca248b8f371db37b8b85) )

	ROM_REGION( 0x0400, REGION_GFX1, 0 )
	ROM_LOAD( "rl-10.43", 0x0000, 0x0400, CRC(c04466c6) SHA1(da76afdfd22a7810de47376a9b23d3d538d77fdc) )

	ROM_REGION( 0x0400, REGION_GFX2, 0 )
	ROM_LOAD( "rl-07.40", 0x0000, 0x0400, CRC(ba525203) SHA1(1c261cc1259787a7a248766264fefe140226e465) )
ROM_END


DRIVER_INIT( safarir )
{
	safarir_ram1 = auto_malloc(safarir_ram_size);
	safarir_ram2 = auto_malloc(safarir_ram_size);

	port_last = 0;
	port_last2 = 0;
}


GAMEX( 1979, safarir,  0,       safarir, safarir, safarir, ROT90, "SNK (Taito license)", "Safari Rally (World)", GAME_IMPERFECT_SOUND )
GAMEX( 1979, safarirj, safarir, safarir, safarir, safarir, ROT90, "SNK",                 "Safari Rally (Japan)", GAME_IMPERFECT_SOUND )
