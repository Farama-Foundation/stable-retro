/***************************************************************************

Atari Poolshark Driver

***************************************************************************/

#include "driver.h"

extern VIDEO_START( poolshrk );
extern VIDEO_UPDATE( poolshrk );

extern UINT8* poolshrk_playfield_ram;
extern UINT8* poolshrk_hpos_ram;
extern UINT8* poolshrk_vpos_ram;

static int poolshrk_da_latch;


static DRIVER_INIT( poolshrk )
{
	UINT8* pSprite = memory_region(REGION_GFX1);
	UINT8* pOffset = memory_region(REGION_PROMS);

	int i;
	int j;

	/* re-arrange sprite data using the PROM */

	for (i = 0; i < 16; i++)
	{
		for (j = 0; j < 16; j++)
		{
			UINT16 v =
				(pSprite[0] << 0xC) |
				(pSprite[1] << 0x8) |
				(pSprite[2] << 0x4) |
				(pSprite[3] << 0x0);

			v >>= pOffset[j];

			pSprite[0] = (v >> 0xC) & 15;
			pSprite[1] = (v >> 0x8) & 15;
			pSprite[2] = (v >> 0x4) & 15;
			pSprite[3] = (v >> 0x0) & 15;

			pSprite += 4;
		}
	}
}


WRITE_HANDLER( poolshrk_scratch_sound_w ) {}
WRITE_HANDLER( poolshrk_score_sound_w ) {}
WRITE_HANDLER( poolshrk_click_sound_w ) {}
WRITE_HANDLER( poolshrk_bump_sound_w ) {}


WRITE_HANDLER( poolshrk_da_latch_w )
{
	poolshrk_da_latch = data & 15;
}


WRITE_HANDLER( poolshrk_led_w )
{
	if (offset & 2)
		set_led_status(0, offset & 1);
	if (offset & 4)
		set_led_status(1, offset & 1);
}


READ_HANDLER( poolshrk_input_r )
{
	UINT8 val = readinputport(offset);

	int x = readinputport(4 + (offset & 1));
	int y = readinputport(6 + (offset & 1));

	if (x >= poolshrk_da_latch) val |= 8;
	if (y >= poolshrk_da_latch) val |= 4;

	return val;
}


READ_HANDLER( poolshrk_interrupt_ack_r )
{
	cpu_set_irq_line(0, 0, CLEAR_LINE);

	return 0;
}


static MEMORY_READ_START( poolshrk_readmem )
	{ 0x0000, 0x00ff, MRA_RAM },
	{ 0x1000, 0x1003, poolshrk_input_r },
	{ 0x6800, 0x6800, poolshrk_interrupt_ack_r },
	{ 0x7000, 0x7fff, MRA_ROM }, /* program */
	{ 0xf800, 0xffff, MRA_ROM }, /* program mirror */
MEMORY_END

static MEMORY_WRITE_START( poolshrk_writemem )
	{ 0x0000, 0x00ff, MWA_RAM },
	{ 0x0400, 0x07ff, MWA_RAM, &poolshrk_playfield_ram },
	{ 0x0800, 0x0810, MWA_RAM, &poolshrk_hpos_ram },
	{ 0x0c00, 0x0c10, MWA_RAM, &poolshrk_vpos_ram },
	{ 0x1003, 0x1003, watchdog_reset_w },
	{ 0x1400, 0x1401, poolshrk_scratch_sound_w },
	{ 0x1800, 0x1800, poolshrk_score_sound_w },
	{ 0x1c00, 0x1c00, poolshrk_click_sound_w },
	{ 0x6000, 0x6000, poolshrk_da_latch_w },
	{ 0x6400, 0x6401, poolshrk_bump_sound_w },
	{ 0x6c00, 0x6c07, poolshrk_led_w },
	{ 0x7000, 0x7fff, MWA_ROM }, /* program */
	{ 0xf800, 0xffff, MWA_ROM }, /* program mirror */
MEMORY_END


INPUT_PORTS_START( poolshrk )
	PORT_START
	PORT_BIT( 0x0C, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x0C, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_DIPNAME( 0x80, 0x00, "Extended Play" )
	PORT_DIPSETTING( 0x80, DEF_STR( Off ))
	PORT_DIPSETTING( 0x00, DEF_STR( On ))

	PORT_START
	PORT_DIPNAME( 0x03, 0x02, "Racks Per Game" )
	PORT_DIPSETTING( 0x03, "2" )
	PORT_DIPSETTING( 0x02, "3" )
	PORT_DIPSETTING( 0x01, "4" )
	PORT_DIPSETTING( 0x00, "5" )
	PORT_BIT( 0x0C, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coinage ))
	PORT_DIPSETTING( 0x00, DEF_STR( 2C_1C ))
	PORT_DIPSETTING( 0x03, DEF_STR( 1C_1C ))
	PORT_DIPSETTING( 0x02, DEF_STR( 1C_2C ))
	PORT_DIPSETTING( 0x01, DEF_STR( 1C_4C ))
	PORT_BIT( 0x0C, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START
	PORT_ANALOG( 15, 8, IPT_AD_STICK_X | IPF_CENTER | IPF_PLAYER1, 25, 1, 0, 15)

	PORT_START
	PORT_ANALOG( 15, 8, IPT_AD_STICK_X | IPF_CENTER | IPF_PLAYER2, 25, 1, 0, 15)

	PORT_START
	PORT_ANALOG( 15, 8, IPT_AD_STICK_Y | IPF_CENTER | IPF_REVERSE | IPF_PLAYER1, 25, 1, 0, 15)

	PORT_START
	PORT_ANALOG( 15, 8, IPT_AD_STICK_Y | IPF_CENTER | IPF_REVERSE | IPF_PLAYER2, 25, 1, 0, 15)

INPUT_PORTS_END


static struct GfxLayout poolshrk_sprite_layout =
{
	16, 16,   /* width, height */
	16,       /* total         */
	1,        /* planes        */
	{ 0 },    /* plane offsets */
	{
		0x04, 0x05, 0x06, 0x07, 0x0C, 0x0D, 0x0E, 0x0F,
		0x14, 0x15, 0x16, 0x17, 0x1C, 0x1D, 0x1E, 0x1F
	},
	{
		0x000, 0x020, 0x040, 0x060, 0x080, 0x0A0, 0x0C0, 0x0E0,
		0x100, 0x120, 0x140, 0x160, 0x180, 0x1A0, 0x1C0, 0x1E0
	},
	0x200     /* increment */
};


static struct GfxLayout poolshrk_tile_layout =
{
	8, 8,     /* width, height */
	64,       /* total         */
	1,        /* planes        */
	{ 0 },    /* plane offsets */
	{
		7, 6, 5, 4, 3, 2, 1, 0
	},
	{
		0x000, 0x200, 0x400, 0x600, 0x800, 0xA00, 0xC00, 0xE00
	},
	0x8       /* increment */
};


static struct GfxDecodeInfo poolshrk_gfx_decode_info[] =
{
	{ REGION_GFX1, 0, &poolshrk_sprite_layout, 0, 2 },
	{ REGION_GFX2, 0, &poolshrk_tile_layout, 0, 1 },
	{ -1 } /* end of array */
};


static PALETTE_INIT( poolshrk )
{
	palette_set_color(0,0x7F, 0x7F, 0x7F);
	palette_set_color(1,0xFF, 0xFF, 0xFF);
	palette_set_color(2,0x7F, 0x7F, 0x7F);
	palette_set_color(3,0x00, 0x00, 0x00);
}


static MACHINE_DRIVER_START( poolshrk )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6800, 11055000 / 8) /* ? */
	MDRV_CPU_MEMORY(poolshrk_readmem, poolshrk_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_assert, 1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(1, 255, 24, 255)
	MDRV_GFXDECODE(poolshrk_gfx_decode_info)
	MDRV_PALETTE_LENGTH(4)
	MDRV_PALETTE_INIT(poolshrk)
	MDRV_VIDEO_START(poolshrk)
	MDRV_VIDEO_UPDATE(poolshrk)

	/* sound hardware */
MACHINE_DRIVER_END


ROM_START( poolshrk )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "7329.k1", 0x7000, 0x800, CRC(88152245) SHA1(c7c5e43ea488a197e92a1dc2231578f8ed86c98d) )
	ROM_LOAD( "7330.l1", 0x7800, 0x800, CRC(fb41d3e9) SHA1(c17994179362da13acfcd36a28f45e328428c031) )
	ROM_RELOAD(          0xF800, 0x800 )

	ROM_REGION( 0x400, REGION_GFX1, ROMREGION_DISPOSE )   /* sprites */
	ROM_LOAD( "7325.j5", 0x0000, 0x200, CRC(fae87eed) SHA1(8891d0ea60f72f826d71dc6b064a2ba81b298914) )
	ROM_LOAD( "7326.h5", 0x0200, 0x200, CRC(05ec9762) SHA1(6119c4529334c98a0a42ca13a98a8661fc594d80) )

	ROM_REGION( 0x200, REGION_GFX2, ROMREGION_DISPOSE )   /* tiles */
	ROM_LOAD( "7328.n6", 0x0000, 0x200, CRC(64bcbf3a) SHA1(a4e3ce6b4734234359e3ef784a771e40580c2a2a) )

	ROM_REGION( 0x20, REGION_PROMS, 0 )                   /* sprite offsets */
	ROM_LOAD( "7327.k6", 0x0000, 0x020, CRC(f74cef5b) SHA1(f470bf5b193dae4b44e89bc4c4476cf8d98e7cfd) )
ROM_END


GAMEX( 1977, poolshrk, 0, poolshrk, poolshrk, poolshrk, 0, "Atari", "Poolshark", GAME_NO_SOUND )
