/****************************************************
Mirax (C)1985 Current Technologies

driver by
Tomasz Slanina analog[AT]op[DOT]pl
Angelo Salese
Olivier Galibert

TODO:
- emulate sound properly;
- sprite offsets?
- score / credits display should stay above the sprites?

====================================================

CPU: Ceramic potted module, Z80C
Sound: AY-3-8912 (x2)
RAM: 74S201, 74S201 (x6), 2148 (x6), 2114 (x2), 58725 (x2), 6116
PROMS: 82S123 (x2)
XTAL: 12 MHz

Here comes a high energy space game of its own kind.

Mirax(TM)
* First person perspective
* Fully integrated game play.
* Continually changing 3-D graphics plus powerful sound effects.

HOW TO PLAY
1. Your goal is to terminate Mirax City - a giant enemy central station shown at stage 10.
2. Destroy all enemy objects (air or ground) as many as you can. Avoid indestructible building blocks.
3. Shooting a group of ground targets causes satellites to rise.
4. Enemy flagship appears after a wave of far ground vessels completely destroyed.
5. Hitting flagship puts you into power shooting mode - no fire button required.
6. Bonus flag is given for each flagship hit. 5 or 8 flags award you an extra fighter.
7. Use joystick to take sight before firing.


Pinouts

Parts          Solder
1 Gnd          Gnd
2 Gnd          Gnd
3 Gnd          Video gnd
4 Gnd          Gnd
5
6
7
8
9 Coin 1       Video sync
10 Up
11 Fire
12 Down
13 start1      start2
14 left
15 right
16 Video green
17 Video blue
18 Video red
19 Speaker-    Speaker+
20 +5          +5
21 +5          +5
22 +12         +12


The End

*************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "vidhrdw/generic.h"


static UINT8 nAyCtrl=0x00;
static UINT8 nmi_mask;

static VIDEO_START(mirax)
{
  return 0;
}

static void draw_sprites(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	int count;

	for(count=0;count<0x200;count+=4)
	{
		int spr_offs,x,y,color,fx,fy;

		if(spriteram[count] == 0x00 || spriteram[count+3] == 0x00)
			continue;

		spr_offs = (spriteram[count+1] & 0x3f);
		color = spriteram[count+2] & 0x7;
		fx = spriteram[count+1] & 0x40; /* <- guess */
		fy = spriteram[count+1] & 0x80;

		spr_offs += (spriteram[count+2] & 0xe0)<<1;
		spr_offs += (spriteram[count+2] & 0x10)<<5;

		y = 0x100 - spriteram[count];
		x = spriteram[count+3];

		drawgfx(bitmap,Machine->gfx[1],spr_offs,color,fx,fy,x,y-16,cliprect,TRANSPARENCY_PEN,0);
	}
}

static VIDEO_UPDATE(mirax)
{
	const struct GfxElement *gfx = Machine->gfx[0];
	int count = 0x00000;
	int y,x;

	for (y=0;y<32;y++)
	{
		for (x=0;x<32;x++)
		{
			int tile = videoram[count];
			int color = (colorram[x*2]<<8) | (colorram[(x*2)+1]);
			int x_scroll = (color & 0xff00)>>8;
			tile |= ((color & 0xe0)<<3);

			drawgfx(bitmap,gfx,tile,color & 7,0,0,(x*8),(y*8)-x_scroll,cliprect,TRANSPARENCY_NONE,0);
			/* wrap-around */
			drawgfx(bitmap,gfx,tile,color & 7,0,0,(x*8),(y*8)-x_scroll+256,cliprect,TRANSPARENCY_NONE,0);

			count++;
		}
	}

	draw_sprites(bitmap,cliprect);

	count = 0x00000;

	/* draw score and credit displays above the sprites */
	for (y=0;y<32;y++)
	{
		for (x=0;x<32;x++)
		{
			int tile = videoram[count];
			int color = (colorram[x*2]<<8) | (colorram[(x*2)+1]);
			int x_scroll = (color & 0xff00)>>8;
			tile |= ((color & 0xe0)<<3);

			if(x <= 1 || x >= 30)
			{
				drawgfx(bitmap,gfx,tile,color & 7,0,0,(x*8),(y*8)-x_scroll,cliprect,TRANSPARENCY_NONE,0);
				/* wrap-around */
				drawgfx(bitmap,gfx,tile,color & 7,0,0,(x*8),(y*8)-x_scroll+256,cliprect,TRANSPARENCY_NONE,0);
			}

			count++;
		}
	}

}


static WRITE_HANDLER(audio_w)
{
    nAyCtrl=offset;
}

static WRITE_HANDLER(ay_sel1)
{
	AY8910_control_port_0_w(0,nAyCtrl);
	AY8910_write_port_0_w(0,data);
}

static WRITE_HANDLER(ay_sel2)
{
	AY8910_control_port_1_w(0,nAyCtrl);
	AY8910_write_port_1_w(0,data);
}

static READ_HANDLER( unk_r )
{
	return 0xff;
}

static WRITE_HANDLER( nmi_mask_w )
{
	nmi_mask = data & 1;
	if(data & 0xfe)
		printf("Warning: %02x written at $f501\n",data);
}

static WRITE_HANDLER( mirax_sound_cmd_w )
{
	soundlatch_w(0, data & 0xff);
	cpu_set_irq_line(1,IRQ_LINE_NMI, PULSE_LINE);
}

/* might be coin counter instead */
static WRITE_HANDLER( coin_w )
{
	coin_lockout_w(0, data & 1);
	coin_lockout_w(1, data & 2);
}


static MEMORY_READ_START( mirax_main_readmem )
    { 0x0000, 0xbfff, MRA_ROM },
	{ 0xc800, 0xd7ff, MRA_RAM },
	{ 0xe000, 0xe3ff, MRA_RAM },
	{ 0xe800, 0xe9ff, MRA_RAM },
	{ 0xea00, 0xea3f, MRA_RAM },
	{ 0xf000, 0xf000, input_port_0_r },
	{ 0xf100, 0xf100, input_port_1_r },
	{ 0xf200, 0xf200, input_port_2_r },
	{ 0xf300, 0xf300, unk_r }, /* watchdog? value is always read then discarded */
	{ 0xf400, 0xf400, input_port_3_r },
MEMORY_END

static MEMORY_WRITE_START( mirax_main_writemem )
    { 0x0000, 0xbfff, MWA_ROM },
	{ 0xc800, 0xd7ff, MWA_RAM },
	{ 0xe000, 0xe3ff, MWA_RAM, &videoram },
	{ 0xe800, 0xe9ff, MWA_RAM, &spriteram },
	{ 0xea00, 0xea3f, MWA_RAM, &colorram }, /* per-column color + bank bits for the videoram */
    { 0xf500, 0xf500, coin_w },
	{ 0xf501, 0xf501, nmi_mask_w },
	{ 0xf800, 0xf800, mirax_sound_cmd_w },
MEMORY_END

static MEMORY_READ_START( mirax_sound_readmem )
    { 0x0000, 0x1fff, MRA_ROM },
	{ 0x8000, 0x8fff, MRA_RAM },
	{ 0xa000, 0xa000, soundlatch_r },
MEMORY_END

static MEMORY_WRITE_START( mirax_sound_writemem )
    { 0x0000, 0x1fff, MWA_ROM },
	{ 0x8000, 0x8fff, MWA_RAM },
	{ 0xe000, 0xe000, MWA_NOP },
	{ 0xe001, 0xe001, MWA_NOP },
	{ 0xe003, 0xe003, ay_sel1 }, /* 1st ay ? */
	{ 0xe400, 0xe400, MWA_NOP },
	{ 0xe401, 0xe401, MWA_NOP },
	{ 0xe403, 0xe403, ay_sel2 }, /* 2nd ay ? */
    { 0xf900, 0xf9ff, audio_w },
MEMORY_END

static struct GfxLayout layout16 =
{
	16,16,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(2,3),RGN_FRAC(1,3),RGN_FRAC(0,3)},
	{ 0, 1, 2, 3, 4, 5, 6, 7 ,
	  0+8*8,1+8*8,2+8*8,3+8*8,4+8*8,5+8*8,6+8*8,7+8*8},
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	0*8+8*8*2, 1*8+8*8*2, 2*8+8*8*2, 3*8+8*8*2, 4*8+8*8*2, 5*8+8*8*2, 6*8+8*8*2, 7*8+8*8*2},
	16*16
};

static struct GfxLayout layout8 =
{
	8,8,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(2,3),RGN_FRAC(1,3),RGN_FRAC(0,3)},
	{ 0, 1, 2, 3, 4, 5, 6, 7},
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8},
	8*8
};


static struct GfxDecodeInfo mirax_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout8,     0, 8 },
	{ REGION_GFX2, 0, &layout16,    0, 8 },
	{ -1 }	/* end of array */
};

INPUT_PORTS_START( mirax )
	/* up/down directions are trusted according of the continue screen */
	PORT_START /* player-1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN2 )

	PORT_START /* player-2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START /* dip-sw bank 1 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START /* dip-sw bank 2 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Bonus_Life ) )       /* table at 0x11b5 (2 * 3 * 2 bytes) */
	PORT_DIPSETTING(    0x00, "30k 80k 150k" )
	PORT_DIPSETTING(    0x01, "900k 950k 990k" )
	PORT_DIPNAME( 0x02, 0x00, "Flags for Extra Life" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPSETTING(    0x02, "8" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Allow_Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Yes ) )
	/* this dip makes the game to behave like attract mode, even if you insert a coin */
	PORT_DIPNAME( 0x10, 0x00, "Auto-Play Mode (Debug)" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END


static PALETTE_INIT( mirax )
{
	int i;

	for (i = 0;i < Machine->drv->total_colors;i++)
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

		palette_set_color(i,r,g,b);
		
	}
}


static INTERRUPT_GEN( mirax_vblank_irq )
{
	if(nmi_mask)
	cpu_set_irq_line(0, IRQ_LINE_NMI, PULSE_LINE);
}

static struct AY8910interface ay8910_interface =
{
	2,  /* 2 chips */
	12000000/4,
	{ 80, 80 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};


static MACHINE_DRIVER_START( mirax )
	MDRV_CPU_ADD(Z80, 12000000/4) /* ceramic potted module, encrypted z80 */
	MDRV_CPU_MEMORY(mirax_main_readmem, mirax_main_writemem)
	MDRV_CPU_VBLANK_INT(mirax_vblank_irq,1)

	MDRV_CPU_ADD(Z80, 12000000/4) /* audio cpu ? */
	MDRV_CPU_MEMORY(mirax_sound_readmem, mirax_sound_writemem)
	MDRV_CPU_PERIODIC_INT(irq0_line_hold, 4*60)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 1*8, 31*8-1)

	MDRV_PALETTE_LENGTH(0x40)
	MDRV_PALETTE_INIT(mirax)
	MDRV_GFXDECODE(mirax_gfxdecodeinfo)
	MDRV_VIDEO_START(mirax)
	MDRV_VIDEO_UPDATE(mirax)

	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END


ROM_START( mirax )
	ROM_REGION( 2*0xc000, REGION_CPU1, ROMREGION_ERASE00 ) /* put decrypted code there */

	ROM_REGION( 2*0xc000, REGION_USER1, 0 ) /* encrypted code for the main cpu */
	ROM_LOAD( "mxp5-42.rom",   0x0000, 0x4000, CRC(716410a0) SHA1(55171376e1e164b1d5e728789da6e04a3a33c172) )
	ROM_LOAD( "mxr5-4v.rom",   0x4000, 0x4000, CRC(c9484fc3) SHA1(101c5e4b9d49d2424ad80970eb3bdb87949a9966) )
	ROM_LOAD( "mxs5-4v.rom",   0x8000, 0x4000, CRC(e0085f91) SHA1(cf143b94048e1ebb5c899b94b500e193dfd42e18) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "mxr2-4v.rom",   0x0000, 0x2000, CRC(cd2d52dc) SHA1(0d4181dc68beac338f47a2065c7b755008877896) )
	
	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mxe3-4v.rom",   0x0000, 0x4000, CRC(0cede01f) SHA1(c723dd8ee9dc06c94a7fe5d5b5bccc42e2181af1) )
	ROM_LOAD( "mxh3-4v.rom",   0x4000, 0x4000, CRC(58221502) SHA1(daf5c508939b44616ca76308fc33f94d364ed587) )
	ROM_LOAD( "mxk3-4v.rom",   0x8000, 0x4000, CRC(6dbc2961) SHA1(5880c28f1ef704fee2d625a42682c7d65613acc8) )

	ROM_REGION( 0x18000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mxe2-4v.rom",   0x04000, 0x4000, CRC(2cf5d8b7) SHA1(f66bce4d413a48f6ae07974870dc0f31eefa68e9) )
	ROM_LOAD( "mxf2-4v.rom",   0x0c000, 0x4000, CRC(1f42c7fa) SHA1(33e56c6ddf7676a12f57de87ec740c6b6eb1cc8c) )
	ROM_LOAD( "mxh2-4v.rom",   0x14000, 0x4000, CRC(cbaff4c6) SHA1(2dc4a1f51b28e98be0cfb5ab7576047c748b6728) )
	ROM_LOAD( "mxf3-4v.rom",   0x00000, 0x4000, CRC(14b1ca85) SHA1(775a4c81a81b78490d45095af31e24c16886f0a2) )
	ROM_LOAD( "mxi3-4v.rom",   0x08000, 0x4000, CRC(20fb2099) SHA1(da6bbd5d2218ba49b8ef98e7affdcab912f84ade) )
	ROM_LOAD( "mxl3-4v.rom",   0x10000, 0x4000, CRC(918487aa) SHA1(47ba6914722a253f65c733b5edff4d15e73ea6c2) )

	ROM_REGION( 0x0060, REGION_PROMS, 0 ) /* data ? encrypted roms for cpu1 ? */
	ROM_LOAD( "mra3.prm",   0x0000, 0x0020, CRC(ae7e1a63) SHA1(f5596db77c1e352ef7845465db3e54e19cd5df9e) )
	ROM_LOAD( "mrb3.prm",   0x0020, 0x0020, CRC(e3f3d0f5) SHA1(182b06c9db5bec1e3030f705247763bd2380ba83) )
ROM_END

ROM_START( miraxa )
	ROM_REGION( 2*0xc000, REGION_CPU1, ROMREGION_ERASE00 ) /* put decrypted code there */

	ROM_REGION( 2*0xc000, REGION_USER1, 0 ) /* encrypted code for the main cpu */
	ROM_LOAD( "mx_p5_43v.p5",   0x0000, 0x4000, CRC(87664903) SHA1(ccc11ecf0658e7af0db3229f60a16b1a44bd12bc) )
	ROM_LOAD( "mx_r5_43v.r5",   0x4000, 0x4000, CRC(1ba4cd8e) SHA1(8fd22d3a4bca7c989382aaf7b08ac381a3566493) )
	ROM_LOAD( "mx_s5_43v.s5",   0x8000, 0x4000, CRC(c58cc151) SHA1(0846e22f4da6d85c6dc29ff1472bc84b419b2289) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "mxr2-4v.rom",   0x0000, 0x2000, CRC(cd2d52dc) SHA1(0d4181dc68beac338f47a2065c7b755008877896) )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mxe3-4v.rom",   0x0000, 0x4000, CRC(0cede01f) SHA1(c723dd8ee9dc06c94a7fe5d5b5bccc42e2181af1) )
	ROM_LOAD( "mxh3-4v.rom",   0x4000, 0x4000, CRC(58221502) SHA1(daf5c508939b44616ca76308fc33f94d364ed587) )
	ROM_LOAD( "mxk3-4v.rom",   0x8000, 0x4000, CRC(6dbc2961) SHA1(5880c28f1ef704fee2d625a42682c7d65613acc8) )

	ROM_REGION( 0x18000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mxe2-4v.rom",   0x04000, 0x4000, CRC(2cf5d8b7) SHA1(f66bce4d413a48f6ae07974870dc0f31eefa68e9) )
	ROM_LOAD( "mxf2-4v.rom",   0x0c000, 0x4000, CRC(1f42c7fa) SHA1(33e56c6ddf7676a12f57de87ec740c6b6eb1cc8c) )
	ROM_LOAD( "mxh2-4v.rom",   0x14000, 0x4000, CRC(cbaff4c6) SHA1(2dc4a1f51b28e98be0cfb5ab7576047c748b6728) )
	ROM_LOAD( "mxf3-4v.rom",   0x00000, 0x4000, CRC(14b1ca85) SHA1(775a4c81a81b78490d45095af31e24c16886f0a2) )
	ROM_LOAD( "mxi3-4v.rom",   0x08000, 0x4000, CRC(20fb2099) SHA1(da6bbd5d2218ba49b8ef98e7affdcab912f84ade) )
	ROM_LOAD( "mxl3-4v.rom",   0x10000, 0x4000, CRC(918487aa) SHA1(47ba6914722a253f65c733b5edff4d15e73ea6c2) )

	ROM_REGION( 0x0060, REGION_PROMS, 0 ) /* data ? encrypted roms for cpu1 ? */
	ROM_LOAD( "mra3.prm",   0x0000, 0x0020, CRC(ae7e1a63) SHA1(f5596db77c1e352ef7845465db3e54e19cd5df9e) )
	ROM_LOAD( "mrb3.prm",   0x0020, 0x0020, CRC(e3f3d0f5) SHA1(182b06c9db5bec1e3030f705247763bd2380ba83) )
ROM_END


static DRIVER_INIT( mirax )
{
	UINT8 *DATA = memory_region(REGION_USER1);
	UINT8 *ROM = memory_region(REGION_CPU1);
	int i;

	for(i=0x0000;i<0x4000;i++)
		ROM[BITSWAP16(i, 15,14,13,12,11,10,9, 5,7,6,8, 4,3,2,1,0)] = (BITSWAP8(DATA[i], 1, 3, 7, 0, 5, 6, 4, 2) ^ 0xff);

	for(i=0x4000;i<0x8000;i++)
		ROM[BITSWAP16(i, 15,14,13,12,11,10,9, 5,7,6,8, 4,3,2,1,0)] = (BITSWAP8(DATA[i], 2, 1, 0, 6, 7, 5, 3, 4) ^ 0xff);

	for(i=0x8000;i<0xc000;i++)
		ROM[BITSWAP16(i, 15,14,13,12,11,10,9, 5,7,6,8, 4,3,2,1,0)] = (BITSWAP8(DATA[i], 1, 3, 7, 0, 5, 6, 4, 2) ^ 0xff);

}

GAME(1985, mirax,  0,     mirax,  mirax,  mirax,  ROT90, "Current Technologies", "Mirax" )
GAME(1985, miraxa, mirax, mirax,  mirax,  mirax,  ROT90, "Current Technologies", "Mirax (set 2)" )
