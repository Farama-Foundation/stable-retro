/*

Pocket Gal Deluxe
Nihon System Inc., 1993

This game runs on Data East hardware.

PCB Layout
----------

DEC-22V0  DE-0378-2
|-------------------------------------|
|  M6295(2)  KG01.14F     DE102  62256|
|  M6295(1)  MAZ-03.13F          62256|
|                                     |
|   32.220MHz              KG00-2.12A |
|                                     |
|J           DE56              DE71   |
|A                                    |
|M     DE153            28MHz         |
|M                    6264     DE52   |
|A                    6264            |
|                                     |
|                                     |
|                                     |
|DSW1                   MAZ-01.3B     |
|     DE104  MAZ-02.2H                |
|DSW2                   MAZ-00.1B     |
|-------------------------------------|

Notes:
      - CPU is DE102. The clock input is 14.000MHz on pin 6
        It's a custom-made encrypted 68000.
        The package is a Quad Flat Pack, has 128 pins and is symmetrically square (1 1/4" square from tip to tip).
      - M6295(1) clock: 2.01375MHz (32.220 / 16)
        M6295(2) clock: 1.006875MHz (32.220 / 32)
      - VSync: 58Hz


    Driver by David Haywood and Bryan McPhail

todo:
    verify sound.

*/

#include "driver.h"
#include "decocrpt.h"
#include "decoprot.h"
#include "deco16ic.h"
#include "vidhrdw/generic.h"

VIDEO_START(pktgaldx);
VIDEO_UPDATE(pktgaldx);


static void pktgaldx_drawsprites(struct mame_bitmap *bitmap,const struct rectangle *cliprect)
{
	int offs;
	int flipscreen=!flip_screen;

	for (offs = 0;offs < 0x400;offs += 4)
	{
		int x,y,sprite,colour,multi,fx,fy,inc,flash,mult;

		sprite = spriteram16[offs+1];
		if (!sprite) continue;

		y = spriteram16[offs];
		flash=y&0x1000;
		if (flash && (cpu_getcurrentframe() & 1)) continue;

		x = spriteram16[offs+2];
		colour = (x >>9) & 0x1f;

		fx = y & 0x2000;
		fy = y & 0x4000;
		multi = (1 << ((y & 0x0600) >> 9)) - 1;	/* 1x, 2x, 4x, 8x height */

		x = x & 0x01ff;
		y = y & 0x01ff;
		if (x >= 320) x -= 512;
		if (y >= 256) y -= 512;
		y = 240 - y;
        x = 304 - x;

		if (x>320) continue;

		sprite &= ~multi;
		if (fy)
			inc = -1;
		else
		{
			sprite += multi;
			inc = 1;
		}

		if (flipscreen)
		{
			y=240-y;
			x=304-x;
			if (fx) fx=0; else fx=1;
			if (fy) fy=0; else fy=1;
			mult=16;
		}
		else mult=-16;

		while (multi >= 0)
		{
			drawgfx(bitmap,Machine->gfx[2],
					sprite - multi * inc,
					colour,
					fx,fy,
					x,y + mult * multi,
					cliprect,TRANSPARENCY_PEN,0);

			multi--;
		}
	}
}

static int pktgaldx_bank_callback(const int bank)
{
	return ((bank>>4) & 0x7) * 0x1000;
}


VIDEO_START(pktgaldx)
{
	if (deco16_1_video_init())
		return 1;

	deco16_set_tilemap_bank_callback(1,pktgaldx_bank_callback);

	return 0;
}

VIDEO_UPDATE(pktgaldx)
{
	flip_screen_set( deco16_pf12_control[0]&0x80 );
	deco16_pf12_update(deco16_pf1_rowscroll,deco16_pf2_rowscroll);

	fillbitmap(bitmap,Machine->pens[0x0],cliprect); /* not Confirmed */
	fillbitmap(priority_bitmap,0,NULL);

	deco16_tilemap_2_draw(bitmap,cliprect,0,0);
	pktgaldx_drawsprites(bitmap,cliprect);
	deco16_tilemap_1_draw(bitmap,cliprect,0,0);
}


WRITE16_HANDLER( pktgaldx_nonbuffered_palette_w )
{
	int r,g,b;

	COMBINE_DATA(&paletteram16[offset]);
	if (offset&1) offset--;

	b = (paletteram16[offset] >> 0) & 0xff;
	g = (paletteram16[offset+1] >> 8) & 0xff;
	r = (paletteram16[offset+1] >> 0) & 0xff;

	palette_set_color(offset/2,r,g,b);
}


/**********************************************************************************/

static WRITE16_HANDLER(pktgaldx_oki_bank_w)
{
	OKIM6295_set_bank_base(1, (data & 3) * 0x40000);
}

/**********************************************************************************/


static MEMORY_READ16_START( pktgaldx_readmem )
    { 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x100000, 0x100fff, MRA16_RAM },
	{ 0x102000, 0x102fff, MRA16_RAM },
	{ 0x110000, 0x1107ff, MRA16_RAM },
	{ 0x112000, 0x1127ff, MRA16_RAM },
	{ 0x120000, 0x1207ff, MRA16_RAM },
	{ 0x130000, 0x130fff, MRA16_RAM },
	{ 0x140006, 0x140007, OKIM6295_status_0_lsb_r },
	{ 0x150006, 0x150007, OKIM6295_status_1_lsb_r },
	{ 0x167800, 0x167fff, deco16_104_pktgaldx_prot_r },
	{ 0x170000, 0x17ffff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( pktgaldx_writemem )
    { 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x100000, 0x100fff, deco16_pf1_data_w, &deco16_pf1_data },
	{ 0x102000, 0x102fff, deco16_pf2_data_w, &deco16_pf2_data },
	{ 0x110000, 0x1107ff, MWA16_RAM, &deco16_pf1_rowscroll },
	{ 0x112000, 0x1127ff, MWA16_RAM, &deco16_pf2_rowscroll },
	{ 0x120000, 0x1207ff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x130000, 0x130fff, pktgaldx_nonbuffered_palette_w, &paletteram16 },
	{ 0x140000, 0x14000f, OKIM6295_data_0_lsb_w },
	{ 0x150000, 0x15000f, OKIM6295_data_1_lsb_w },
	{ 0x161800, 0x16180f, MWA16_RAM, &deco16_pf12_control },
	{ 0x164800, 0x164801, pktgaldx_oki_bank_w },
	{ 0x167800, 0x167fff, deco16_104_pktgaldx_prot_w, &deco16_prot_ram },
	{ 0x170000, 0x17ffff, MWA16_RAM },
MEMORY_END



/**********************************************************************************/

INPUT_PORTS_START( pktgaldx )
	PORT_START	/* 16bit */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* 16bit */
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "2 Coins to Start, 1 to Continue" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPSETTING(      0x0100, "3" )
	PORT_DIPSETTING(      0x0300, "4" )
	PORT_DIPSETTING(      0x0200, "5" )
	PORT_DIPNAME( 0x0c00, 0x0c00, "Time"  ) /* Listed as "Difficulty" */
	PORT_DIPSETTING(      0x0000, "60" )
	PORT_DIPSETTING(      0x0400, "80" )
	PORT_DIPSETTING(      0x0c00, "100" )
	PORT_DIPSETTING(      0x0800, "120" )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* 16bit */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START2 )
INPUT_PORTS_END

static struct GfxLayout tile_8x8_layout =
{
	8,8,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+8,RGN_FRAC(1,2)+0,RGN_FRAC(0,2)+8,RGN_FRAC(0,2)+0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	8*16
};

static struct GfxLayout tile_16x16_layout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+8,RGN_FRAC(1,2)+0,RGN_FRAC(0,2)+8,RGN_FRAC(0,2)+0 },
	{ 256,257,258,259,260,261,262,263,0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,8*16,9*16,10*16,11*16,12*16,13*16,14*16,15*16 },
	32*16
};

static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 24,8,16,0 },
	{ 512,513,514,515,516,517,518,519, 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
	  8*32, 9*32,10*32,11*32,12*32,13*32,14*32,15*32},
	32*32
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tile_8x8_layout,     0, 32 },	/* Tiles (8x8) */
	{ REGION_GFX1, 0, &tile_16x16_layout,   0, 32 },	/* Tiles (16x16) */
	{ REGION_GFX2, 0, &spritelayout,      512, 32 },	/* Sprites (16x16) */
	{ -1 } /* end of array */
};


static struct OKIM6295interface okim6295_interface =
{
	2,              /* 2 chips */
	{ 32220000/32/132, 32220000/16/132 },/* Frequency */
	{ REGION_SOUND1, REGION_SOUND2 },
	{ 95, 40 } /* Note!  Keep chip 1 (voices) louder than chip 2 */
};


static MACHINE_DRIVER_START( pktgaldx )
	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 14000000)
	MDRV_CPU_MEMORY(pktgaldx_readmem, pktgaldx_writemem)
	MDRV_CPU_VBLANK_INT(irq6_line_hold,1)

	MDRV_FRAMES_PER_SECOND(58)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 1*8, 31*8-1)
	MDRV_PALETTE_LENGTH(4096)
	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_START(pktgaldx)
	MDRV_VIDEO_UPDATE(pktgaldx)

	/* sound hardware */
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END


ROM_START( pktgaldx )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* DE102 code (encrypted) */
	ROM_LOAD16_WORD_SWAP( "ke00-2.12a",    0x00000, 0x80000, CRC(b04baf3a) SHA1(680d1b4ab4b6edef36cd96a60539fb7c2dac9637) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "maz-02.2h",    0x00000, 0x100000, CRC(c9d35a59) SHA1(07b44c7d7d76b668b4d6ca5672bd1c2910228e68) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "maz-00.1b",    0x000000, 0x080000, CRC(fa3071f4) SHA1(72e7d920e9ca94f8cb166007a9e9e5426a201af8) )
	ROM_LOAD16_BYTE( "maz-01.3b",    0x000001, 0x080000, CRC(4934fe21) SHA1(b852249f59906d69d32160ebaf9b4781193227e4) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "ke01.14f",    0x00000, 0x20000, CRC(8a106263) SHA1(229ab17403c2b8f4e89a90a8cda2f3c3a4b55d9e) )

	ROM_REGION( 0x100000, REGION_SOUND2, 0 ) /* Oki samples (banked?) */
	ROM_LOAD( "maz-03.13f",    0x00000, 0x100000, CRC(a313c964) SHA1(4a3664c4e2c44a017a0ab6a6d4361799cbda57b5) )
ROM_END

ROM_START( pktgaldj )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* DE102 code (encrypted) */
	ROM_LOAD16_WORD_SWAP( "kg00-2.12a",    0x00000, 0x80000, CRC(62dc4137) SHA1(23887dc3f6e7c4cdcb1bf4f4c87fe3cbe8cdbe69) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "maz-02.2h",    0x00000, 0x100000, CRC(c9d35a59) SHA1(07b44c7d7d76b668b4d6ca5672bd1c2910228e68) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "maz-00.1b",    0x000000, 0x080000, CRC(fa3071f4) SHA1(72e7d920e9ca94f8cb166007a9e9e5426a201af8) )
	ROM_LOAD16_BYTE( "maz-01.3b",    0x000001, 0x080000, CRC(4934fe21) SHA1(b852249f59906d69d32160ebaf9b4781193227e4) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "ke01.14f",    0x00000, 0x20000, CRC(8a106263) SHA1(229ab17403c2b8f4e89a90a8cda2f3c3a4b55d9e) )

	ROM_REGION( 0x100000, REGION_SOUND2, 0 ) /* Oki samples (banked?) */
	ROM_LOAD( "maz-03.13f",    0x00000, 0x100000, CRC(a313c964) SHA1(4a3664c4e2c44a017a0ab6a6d4361799cbda57b5) )
ROM_END


extern void deco102_decrypt_cpu(int address_xor, int data_select_xor, int opcode_select_xor);

static DRIVER_INIT( pktgaldx )
{
	deco56_decrypt(REGION_GFX1);
	deco102_decrypt_cpu(0x42ba, 0x00, 0x00);
}

GAME( 1992, pktgaldx, 0,        pktgaldx, pktgaldx, pktgaldx,  ROT0, "Data East Corporation", "Pocket Gal Deluxe (Euro v3.00)" )
GAME( 1993, pktgaldj, pktgaldx, pktgaldx, pktgaldx, pktgaldx,  ROT0, "Nihon System",          "Pocket Gal Deluxe (Japan v3.00)" )
