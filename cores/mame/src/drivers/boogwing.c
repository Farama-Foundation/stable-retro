/*
    Boogie Wings (aka The Great Ragtime Show)
    Data East, 1992
    PCB No: DE-0379-1
    CPU: DE102
    Sound: HuC6280A, YM2151, YM3012, OKI M6295 (x2)
    OSC: 32.220MHz, 28.000MHz
    DIPs: 8 position (x2)
    PALs: (not dumped) VF-00 (PAL16L8)
                       VF-01 (PAL16L8)
                       VF-02 (22CV10P)
                       VF-03 (PAL16L8)
                       VF-04 (PAL16L8)
                       VF-05 (PAL16L8)
    PROM: MB7122 (compatible to 82S137, located near MBD-09)
    RAM: 62256 (x2), 6264 (x5)
    Data East Chips:   52 (x2)
                    141 (x2)
                    102
                    104
                    113
                    99
                    71 (x2)
                    200
    ROMs:
    kn00-2.2b   27c2001   \
    kn01-2.4b   27c2001    |  Main program
    kn02-2.2e   27c2001    |
    kn03-2.4e   27c2001   /
    kn04.8e     27c512    \
    kn05.9e     27c512    /   near 141's and MBD-00, MBD-01 and MBD-02
    kn06.18p    27c512        Sound Program
    mbd-00.8b   16M
    mbd-01.9b   16M
    mbd-02.10e  4M
    mbd-03.13b  16M
    mbd-04.14b  16M
    mbd-05.16b  16M
    mbd-06.17b  16M
    mbd-07.18b  16M
    mbd-08.19b  16M
    mbd-09.16p  4M         Oki Samples
    mbd-10.17p  4M         Oki Samples
    Driver by Bryan McPhail and David Haywood.
    Todo:
        * Sprite priorities aren't verified to be 100% accurate.
          (Addendum - all known issues seem to be correct - see Sprite Priority Notes below).
        * There may be some kind of fullscreen palette effect (controlled by bit 3 in priority
          word - used at end of each level, and on final boss).
        * A shadow effect (used in level 1) is not implemented.
*/
#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "cpu/h6280/h6280.h"
#include "decocrpt.h"
#include "deco16ic.h"
#include "decoprot.h"

data16_t	*boogwing_spriteram16;
data16_t	*boogwing_spriteram16_2;
data16_t	*boogwing_buffered_spriteram16;
data16_t	*boogwing_buffered_spriteram16_2;

/**********************************************************************************/

static int flip_screen;
 

static void boogie_drawsprites(struct mame_bitmap *bitmap,const struct rectangle *cliprect, data16_t* spriteram_base, int gfx_region)
{
	int offs;
	int flipscreen=!flip_screen;

	for (offs = 0x400-4;offs >= 0;offs -= 4)
	{
		int x,y,sprite,colour,multi,fx,fy,inc,flash,mult,pri=0,spri=0;
		int trans=TRANSPARENCY_PEN;

		sprite = spriteram_base[offs+1];
		if (!sprite) continue;

		y = spriteram_base[offs];
		flash=y&0x1000;
		if (flash && (cpu_getcurrentframe() & 1)) continue;

		x = spriteram_base[offs+2];
		colour = (x >>9) & 0x1f;

		fx = y & 0x2000;
		fy = y & 0x4000;
		multi = (1 << ((y & 0x0600) >> 9)) - 1;	/* 1x, 2x, 4x, 8x height */

		/* Todo:  This should be verified from the prom*/
		if (gfx_region==4)
		{
			/* Sprite 2 priority vs sprite 1*/
			if ((spriteram_base[offs+2]&0xc000)==0xc000)
				spri=4;
			else if ((spriteram_base[offs+2]&0xc000))
				spri=16;
			else
				spri=64;

			/* Transparency*/
			if (spriteram_base[offs+2]&0x2000)
				trans=TRANSPARENCY_ALPHA;

			if (deco16_priority==0x2)
			{
				/* Additional sprite alpha in this mode*/
				if (spriteram_base[offs+2]&0x8000)
					trans=TRANSPARENCY_ALPHA;

				/* Sprite vs playfield*/
				if ((spriteram_base[offs+2]&0xc000)==0xc000)
					pri=4;
				else if ((spriteram_base[offs+2]&0xc000)==0x8000)
					pri=16;
				else
					pri=64;
			}
			else
			{
				if ((spriteram_base[offs+2]&0x8000)==0x8000)
					pri=16;
				else
					pri=64;
			}
		}
		else
		{
			/* Sprite 1 priority vs sprite 2*/
			if (spriteram_base[offs+2]&0x8000)		/* todo - check only in pri mode 2??*/
				spri=8;
			else
				spri=32;

			/* Sprite vs playfield*/
			if (deco16_priority==0x1)
			{
				if ((spriteram_base[offs+2]&0xc000))
					pri=16;
				else
					pri=64;
			}
			else
			{
				if ((spriteram_base[offs+2]&0xc000)==0xc000)
					pri=4;
				else if ((spriteram_base[offs+2]&0xc000)==0x8000)
					pri=16;
				else
					pri=64;
			}
		}

		x = x & 0x01ff;
		y = y & 0x01ff;
		if (x >= 320) x -= 512;
		if (y >= 256) y -= 512;
		y = 240 - y;
		x = 304 - x;

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
			deco16_pdrawgfx(bitmap,Machine->gfx[gfx_region],
					sprite - multi * inc,
					colour,
					fx,fy,
					x,y + mult * multi,
					cliprect,trans, 0, pri, spri, 0);

			multi--;
		}
	}
}

static int boogwing_bank_callback(const int bank)
{
	return ((bank>>4) & 0x7) * 0x1000;
}

static int boogwing_bank_callback2(const int bank)
{
	int offset=((bank>>4) & 0x7) * 0x1000;
	if ((bank&0xf)==0xa)
		offset+=0x800; /* strange - transporter level*/
	return offset;
}

VIDEO_START(boogwing)
{
	extern void deco16_set_tilemap_transparency_mask(int tilemap, int mask);
	if (deco16_2_video_init(0))
		return 1;

	deco16_set_tilemap_bank_callback(1,boogwing_bank_callback);
	deco16_set_tilemap_bank_callback(2,boogwing_bank_callback2);
	deco16_set_tilemap_bank_callback(3,boogwing_bank_callback2);
	deco16_set_tilemap_colour_base(1,0);
	deco16_set_tilemap_transparency_mask(1, 0x1f); /* 5bpp graphics*/

	alpha_set_level(0x80);

	boogwing_buffered_spriteram16 = auto_malloc(0x800);
	boogwing_buffered_spriteram16_2 = auto_malloc(0x800);

	return 0;
}

VIDEO_UPDATE(boogwing)
{
	extern void deco16_tilemap_34_combine_draw(struct mame_bitmap *bitmap, const struct rectangle *cliprect, int flags, UINT32 priority);
	flip_screen = deco16_pf12_control[0]&0x80;
	deco16_pf12_update(deco16_pf1_rowscroll,deco16_pf2_rowscroll);
	deco16_pf34_update(deco16_pf3_rowscroll,deco16_pf4_rowscroll);

	deco16_clear_sprite_priority_bitmap();
	fillbitmap(bitmap,Machine->pens[0x400],cliprect); /* pen not confirmed */
	fillbitmap(priority_bitmap,0,NULL);
	if ((deco16_priority&0x7)==0x5)
	{
		deco16_tilemap_2_draw(bitmap,cliprect,TILEMAP_IGNORE_TRANSPARENCY,0);
		deco16_tilemap_34_combine_draw(bitmap,cliprect,0,32);
	}
	else if ((deco16_priority&0x7)==0x1 || (deco16_priority&0x7)==0x2)
	{
		deco16_tilemap_4_draw(bitmap,cliprect,TILEMAP_IGNORE_TRANSPARENCY,0);
		deco16_tilemap_2_draw(bitmap,cliprect,0,8);
		deco16_tilemap_3_draw(bitmap,cliprect,0,32);
	}
	else if ((deco16_priority&0x7)==0x3)
	{
		deco16_tilemap_4_draw(bitmap,cliprect,TILEMAP_IGNORE_TRANSPARENCY,0);
		deco16_tilemap_2_draw(bitmap,cliprect,0,8);
	}
	else
	{
		deco16_tilemap_4_draw(bitmap,cliprect,TILEMAP_IGNORE_TRANSPARENCY,0);
		deco16_tilemap_3_draw(bitmap,cliprect,0,8);
		deco16_tilemap_2_draw(bitmap,cliprect,0,32);
	}

	boogie_drawsprites(bitmap, cliprect, boogwing_buffered_spriteram16, 3);
	boogie_drawsprites(bitmap, cliprect, boogwing_buffered_spriteram16_2, 4);

	deco16_tilemap_1_draw(bitmap,cliprect,0,0);
}

/**********************************************************************************/

static WRITE16_HANDLER( boogwing_buffer_spriteram16_w )
{
	memcpy (boogwing_buffered_spriteram16, boogwing_spriteram16, 0x800);
}

static WRITE16_HANDLER( boogwing_buffer_spriteram16_2_w )
{
	memcpy (boogwing_buffered_spriteram16_2, boogwing_spriteram16_2, 0x800);
}

static MEMORY_READ16_START( boogwing_readmem )
    { 0x000000, 0x1fffff, MRA16_ROM },
	{ 0x200000, 0x20ffff, MRA16_RAM },
	{ 0x242000, 0x2427ff, MRA16_RAM },
	{ 0x246000, 0x2467ff, MRA16_RAM },
	{ 0x24e6c0, 0x24e6c1, input_port_1_word_r },
	{ 0x24e138, 0x24e139, input_port_0_word_r },
	{ 0x24e344, 0x24e345, input_port_2_word_r },
	{ 0x24e000, 0x24e7ff, MRA16_RAM },
	{ 0x264000, 0x265fff, MRA16_RAM },
	{ 0x266000, 0x267fff, MRA16_RAM },
	{ 0x268000, 0x268fff, MRA16_RAM },
	{ 0x26a000, 0x26afff, MRA16_RAM },
	{ 0x274000, 0x275fff, MRA16_RAM },
	{ 0x276000, 0x277fff, MRA16_RAM },
	{ 0x278000, 0x278fff, MRA16_RAM },
	{ 0x27a000, 0x27afff, MRA16_RAM },
	{ 0x284000, 0x285fff, MRA16_RAM },
	{ 0x3c0000, 0x3c004f, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( boogwing_writemem )
    { 0x000000, 0x0fffff, MWA16_ROM },
	{ 0x200000, 0x20ffff, MWA16_RAM },
	{ 0x220000, 0x220001, deco16_priority_w },
	{ 0x240000, 0x240001, boogwing_buffer_spriteram16_w },
	{ 0x242000, 0x2427ff, MWA16_RAM, &boogwing_spriteram16 },
	{ 0x244000, 0x244001, boogwing_buffer_spriteram16_2_w },
	{ 0x246000, 0x2467ff, MWA16_RAM, &boogwing_spriteram16_2 },
	{ 0x24e000, 0x24e7ff, deco16_104_prot_w, &deco16_prot_ram },
	{ 0x260000, 0x26000f, MWA16_RAM, &deco16_pf12_control },
	{ 0x264000, 0x265fff, deco16_pf1_data_w, &deco16_pf1_data },
	{ 0x266000, 0x267fff, deco16_pf2_data_w, &deco16_pf2_data },
	{ 0x268000, 0x268fff, MWA16_RAM, &deco16_pf1_rowscroll },
	{ 0x26a000, 0x26afff, MWA16_RAM, &deco16_pf2_rowscroll },
	{ 0x270000, 0x27000f, MWA16_RAM, &deco16_pf34_control },
	{ 0x274000, 0x275fff, deco16_pf3_data_w, &deco16_pf3_data },
	{ 0x276000, 0x277fff, deco16_pf4_data_w, &deco16_pf4_data },
	{ 0x278000, 0x278fff, MWA16_RAM, &deco16_pf3_rowscroll },
	{ 0x27a000, 0x27afff, MWA16_RAM, &deco16_pf4_rowscroll },
	{ 0x282008, 0x282009, deco16_palette_dma_w },
	{ 0x284000, 0x285fff, deco16_buffered_palette_w, &paletteram16 },
	{ 0x3c0000, 0x3c004f, MWA16_RAM },
MEMORY_END

static MEMORY_READ_START( sound_readmem )
    { 0x000000, 0x00ffff, MRA_ROM },
	{ 0x100000, 0x100001, MRA_NOP },
	{ 0x110000, 0x110001, YM2151_status_port_0_r },
	{ 0x120000, 0x120001, OKIM6295_status_0_r },
	{ 0x130000, 0x130001, OKIM6295_status_1_r },
	{ 0x140000, 0x140001, soundlatch_r },
	{ 0x1f0000, 0x1f1fff, MRA_BANK8 },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
    { 0x000000, 0x00ffff, MWA_ROM },
	{ 0x100000, 0x100001, MWA_NOP },
	{ 0x110000, 0x110001, YM2151_word_0_w },
	{ 0x120000, 0x120001, OKIM6295_data_0_w },
	{ 0x130000, 0x130001, OKIM6295_data_1_w },
	{ 0x1f0000, 0x1f1fff, MWA_BANK8 },
	{ 0x1fec00, 0x1fec01, H6280_timer_w },
	{ 0x1ff402, 0x1ff403, H6280_irq_status_w },
MEMORY_END

/**********************************************************************************/

INPUT_PORTS_START( boogwing )
	PORT_START	/* 16bit */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_VBLANK )

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
	PORT_DIPNAME( 0x0080, 0x0080, "Continue Coin" )
	PORT_DIPSETTING(      0x0080, "Normal Coin Credit" )
	PORT_DIPSETTING(      0x0000, "2 Start/1 Continue" )
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0100, "1" )
	PORT_DIPSETTING(      0x0300, "2" )
	PORT_DIPSETTING(      0x0200, "3" )
	PORT_DIPSETTING(      0x0000, "4" )
	PORT_DIPNAME( 0x0c00, 0x0c00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0800, "Easy" )
	PORT_DIPSETTING(      0x0c00, "Normal" )
	PORT_DIPSETTING(      0x0400, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x1000, 0x1000, "Coin Slots" )
	PORT_DIPSETTING(      0x1000, "Common" )
	PORT_DIPSETTING(      0x0000, "Individual" )
	PORT_DIPNAME( 0x2000, 0x2000, "Stage Reset" ) /* At loss of life */
	PORT_DIPSETTING(      0x2000, "Point of Termination" )
	PORT_DIPSETTING(      0x0000, "Beginning of Stage" )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) ) /* Manual shows as OFF and states "Don't Change" */
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* 16bit */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_8WAY )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_8WAY )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_8WAY )
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

/**********************************************************************************/

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

static struct GfxLayout tile_16x16_layout_5bpp =
{
	16,16,
	RGN_FRAC(1,3),
	5,
	{ RGN_FRAC(2,3), RGN_FRAC(1,3)+8,RGN_FRAC(1,3)+0,RGN_FRAC(0,3)+8,RGN_FRAC(0,3)+0 },
	{ 256,257,258,259,260,261,262,263,0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,8*16,9*16,10*16,11*16,12*16,13*16,14*16,15*16 },
	32*16
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
	{ REGION_GFX1, 0, &tile_8x8_layout,            0, 16 },	/* Tiles (8x8) */
	{ REGION_GFX2, 0, &tile_16x16_layout_5bpp, 0x100, 16 },	/* Tiles (16x16) */
	{ REGION_GFX3, 0, &tile_16x16_layout,      0x300, 32 },	/* Tiles (16x16) */
	{ REGION_GFX4, 0, &spritelayout,           0x500, 32 },	/* Sprites (16x16) */
	{ REGION_GFX5, 0, &spritelayout,           0x700, 16 },	/* Sprites (16x16) */
	{ -1 } /* end of array */
};

/**********************************************************************************/

static void sound_irq(int state)
{
	cpu_set_irq_line(1,1,state); /* IRQ 2 */
}

static WRITE_HANDLER( sound_bankswitch_w )
{
	OKIM6295_set_bank_base(0, ((data & 1)>>0) * 0x40000);
	OKIM6295_set_bank_base(1, ((data & 2)>>1) * 0x40000);
}


static struct YM2151interface ym2151_interface =
{
	1,
	32220000/9, /* Accurate, audio section crystal is 32.220 MHz */
	{ YM3012_VOL(40,MIXER_PAN_LEFT,40,MIXER_PAN_RIGHT) },
	{ sound_irq },
	{ sound_bankswitch_w }
};

static struct OKIM6295interface okim6295_interface =
{
	2,              /* 2 chips */
	{ 32220000/32/132, 32220000/16/132 },/* Frequency */
	{ REGION_SOUND1, REGION_SOUND2 },
	{ 95, 40 } /* Note!  Keep chip 1 (voices) louder than chip 2 */
};


static MACHINE_DRIVER_START( boogwing )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 14000000)
	MDRV_CPU_MEMORY(boogwing_readmem,boogwing_writemem)
	MDRV_CPU_VBLANK_INT(irq6_line_hold,1)

	MDRV_CPU_ADD(H6280,32220000/8)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(58)
	MDRV_VBLANK_DURATION(TIME_IN_USEC(2500))

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_BUFFERS_SPRITERAM | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(boogwing)
	MDRV_VIDEO_UPDATE(boogwing)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END

/**********************************************************************************/

ROM_START( boogwing ) /* VER 1.5 EUR 92.12.07 */
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* DE102 code (encrypted) */
	ROM_LOAD16_BYTE( "kn_00-2.2b",    0x000000, 0x040000, CRC(e38892b9) SHA1(49b5637965a43e0378e1258c5f0a780926f1f283) )
	ROM_LOAD16_BYTE( "kn_02-2.2e",    0x000001, 0x040000, CRC(8426efef) SHA1(2ea33cbd58b638053d75668a484648dbf67dabb8) )
	ROM_LOAD16_BYTE( "kn_01-2.4b",    0x080000, 0x040000, CRC(3ad4b54c) SHA1(5141001768266995078407851b445378b21453de) )
	ROM_LOAD16_BYTE( "kn_03-2.4e",    0x080001, 0x040000, CRC(10b61f4a) SHA1(41d7f670defbd7dae89afafac9839a9e237814d5) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "km06.18p",    0x00000, 0x10000, CRC(3e8bc4e1) SHA1(7e4c357afefa47b8f101727e06485eb9ebae635d) )

	ROM_REGION( 0x20000, REGION_GFX1, 0 ) /* Tiles 1 */
	ROM_LOAD16_BYTE( "km05.9e",   0x00000, 0x010000, CRC(d10aef95) SHA1(a611a35ab312caee19c31da079c647679d31673d) )
	ROM_LOAD16_BYTE( "km04.8e",   0x00001, 0x010000, CRC(329323a8) SHA1(e2ec7b059301c0a2e052dfc683e044c808ad9b33) )

	ROM_REGION( 0x300000, REGION_GFX2, 0 ) /* Tiles 2 */
	ROM_LOAD( "mbd-01.9b", 0x000000, 0x100000, CRC(d7de4f4b) SHA1(4747f8795e277ed8106667b6f68e1176d95db684) )
	ROM_LOAD( "mbd-00.8b", 0x100000, 0x100000, CRC(adb20ba9) SHA1(2ffa1dd19a438a4d2f5743b1050a8037183a3e7d) )
	/* 0x100000 bytes expanded from mbd-02.10e copied here later */

	ROM_REGION( 0x200000, REGION_GFX3, 0 ) /* Tiles 3 */
	ROM_LOAD( "mbd-03.13b",   0x000000, 0x100000, CRC(cf798f2c) SHA1(f484a22679d6a4d4b0dcac820de3f1a37cbc478f) )
	ROM_LOAD( "mbd-04.14b",   0x100000, 0x100000, CRC(d9764d0b) SHA1(74d6f09d65d073606a6e10556cedf740aa50ff08) )

	ROM_REGION( 0x400000, REGION_GFX4, 0 ) /* Sprites 1 */
	ROM_LOAD16_BYTE( "mbd-05.16b",    0x000001, 0x200000, CRC(1768c66a) SHA1(06bf3bb187c65db9dcce959a43a7231e2ac45c17) )
	ROM_LOAD16_BYTE( "mbd-06.17b",    0x000000, 0x200000, CRC(7750847a) SHA1(358266ed68a9816094e7aab0905d958284c8ce98) )

	ROM_REGION( 0x400000, REGION_GFX5, 0 ) /* Sprites 2 */
	ROM_LOAD16_BYTE( "mbd-07.18b",    0x000001, 0x200000, CRC(241faac1) SHA1(588be0cf2647c1d185a99c987a5a20ab7ad8dea8) )
	ROM_LOAD16_BYTE( "mbd-08.19b",    0x000000, 0x200000, CRC(f13b1e56) SHA1(f8f5e8c4e6c159f076d4e6505bd901ade5c6a0ca) )

	ROM_REGION( 0x0100000, REGION_GFX6, 0 ) /* 1bpp graphics */
	ROM_LOAD16_BYTE( "mbd-02.10e",    0x000000, 0x080000, CRC(b25aa721) SHA1(efe800759080bd1dac2da93bd79062a48c5da2b2) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* Oki samples 1 */
	ROM_LOAD( "mbd-10.17p",    0x000000, 0x080000, CRC(f159f76a) SHA1(0b1ea69fecdd151e2b1fa96a21eade492499691d) )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 ) /* Oki samples 1 */
	ROM_LOAD( "mbd-09.16p",    0x000000, 0x080000, CRC(f44f2f87) SHA1(d941520bdfc9e6d88c45462bc1f697c18f33498e) )

	ROM_REGION( 0x000400, REGION_PROMS, 0 ) /* Priority (not used) */
	ROM_LOAD( "kj-00.15n",    0x000000, 0x00400, CRC(add4d50b) SHA1(080e5a8192a146d5141aef5c8d9996ddf8cd3ab4) )
ROM_END

ROM_START( boogwinga ) /* VER 1.5 ASA 92.12.07 */
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* DE102 code (encrypted) */
	ROM_LOAD16_BYTE( "km_00-2.2b",    0x000000, 0x040000, CRC(71ab71c6) SHA1(00bfd71dd9ae5f12c574ab0ecc07d85898930c4b) )
	ROM_LOAD16_BYTE( "km_02-2.2e",    0x000001, 0x040000, CRC(e90f07f9) SHA1(1e8bd3983ed875f4752cbf2ab1c7e748d3df019c) )
	ROM_LOAD16_BYTE( "km_01-2.4b",    0x080000, 0x040000, CRC(7fdce2d3) SHA1(5ce9b8ac26700f1c3bfb3ce4845f890b81241823) )
	ROM_LOAD16_BYTE( "km_03-2.4e",    0x080001, 0x040000, CRC(0b582de3) SHA1(f5c58c7e0e8a227506a81e38c266356596dcda7b) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "km06.18p",    0x00000, 0x10000, CRC(3e8bc4e1) SHA1(7e4c357afefa47b8f101727e06485eb9ebae635d) )

	ROM_REGION( 0x20000, REGION_GFX1, 0 ) /* Tiles 1 */
	ROM_LOAD16_BYTE( "km05.9e",   0x00000, 0x010000, CRC(d10aef95) SHA1(a611a35ab312caee19c31da079c647679d31673d) )
	ROM_LOAD16_BYTE( "km04.8e",   0x00001, 0x010000, CRC(329323a8) SHA1(e2ec7b059301c0a2e052dfc683e044c808ad9b33) )

	ROM_REGION( 0x300000, REGION_GFX2, 0 ) /* Tiles 2 */
	ROM_LOAD( "mbd-01.9b", 0x000000, 0x100000, CRC(d7de4f4b) SHA1(4747f8795e277ed8106667b6f68e1176d95db684) )
	ROM_LOAD( "mbd-00.8b", 0x100000, 0x100000, CRC(adb20ba9) SHA1(2ffa1dd19a438a4d2f5743b1050a8037183a3e7d) )
	/* 0x100000 bytes expanded from mbd-02.10e copied here later */

	ROM_REGION( 0x200000, REGION_GFX3, 0 ) /* Tiles 3 */
	ROM_LOAD( "mbd-03.13b",   0x000000, 0x100000, CRC(cf798f2c) SHA1(f484a22679d6a4d4b0dcac820de3f1a37cbc478f) )
	ROM_LOAD( "mbd-04.14b",   0x100000, 0x100000, CRC(d9764d0b) SHA1(74d6f09d65d073606a6e10556cedf740aa50ff08) )

	ROM_REGION( 0x400000, REGION_GFX4, 0 ) /* Sprites 1 */
	ROM_LOAD16_BYTE( "mbd-05.16b",    0x000001, 0x200000, CRC(1768c66a) SHA1(06bf3bb187c65db9dcce959a43a7231e2ac45c17) )
	ROM_LOAD16_BYTE( "mbd-06.17b",    0x000000, 0x200000, CRC(7750847a) SHA1(358266ed68a9816094e7aab0905d958284c8ce98) )

	ROM_REGION( 0x400000, REGION_GFX5, 0 ) /* Sprites 2 */
	ROM_LOAD16_BYTE( "mbd-07.18b",    0x000001, 0x200000, CRC(241faac1) SHA1(588be0cf2647c1d185a99c987a5a20ab7ad8dea8) )
	ROM_LOAD16_BYTE( "mbd-08.19b",    0x000000, 0x200000, CRC(f13b1e56) SHA1(f8f5e8c4e6c159f076d4e6505bd901ade5c6a0ca) )

	ROM_REGION( 0x0100000, REGION_GFX6, 0 ) /* 1bpp graphics */
	ROM_LOAD16_BYTE( "mbd-02.10e",    0x000000, 0x080000, CRC(b25aa721) SHA1(efe800759080bd1dac2da93bd79062a48c5da2b2) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* Oki samples 1 */
	ROM_LOAD( "mbd-10.17p",    0x000000, 0x080000, CRC(f159f76a) SHA1(0b1ea69fecdd151e2b1fa96a21eade492499691d) )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 ) /* Oki samples 1 */
	ROM_LOAD( "mbd-09.16p",    0x000000, 0x080000, CRC(f44f2f87) SHA1(d941520bdfc9e6d88c45462bc1f697c18f33498e) )

	ROM_REGION( 0x000400, REGION_PROMS, 0 ) /* Priority (not used) */
	ROM_LOAD( "kj-00.15n",    0x000000, 0x00400, CRC(add4d50b) SHA1(080e5a8192a146d5141aef5c8d9996ddf8cd3ab4) )
ROM_END

ROM_START( ragtime ) /* VER 1.5 JPN 92.12.07 */
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* DE102 code (encrypted) */
	ROM_LOAD16_BYTE( "kh_00-2.2b",    0x000000, 0x040000, CRC(553e179f) SHA1(ab156d9eca4a74084da944989529fd8f5a147dfc) )
	ROM_LOAD16_BYTE( "kh_02-2.2e",    0x000001, 0x040000, CRC(6c759ec0) SHA1(f503d225c31543a7cd975fc599811a31ff729251) )
	ROM_LOAD16_BYTE( "kh_01-2.4b",    0x080000, 0x040000, CRC(12dfee70) SHA1(a7c8fd118f589ef13bcb43a6aa446ff81015f5b3) )
	ROM_LOAD16_BYTE( "kh_03-2.4e",    0x080001, 0x040000, CRC(076fea18) SHA1(342ca71b6d8c8be92dbf221ada717bdbd0061226) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "km06.18p",    0x00000, 0x10000, CRC(3e8bc4e1) SHA1(7e4c357afefa47b8f101727e06485eb9ebae635d) )

	ROM_REGION( 0x20000, REGION_GFX1, 0 ) /* Tiles 1 */
	ROM_LOAD16_BYTE( "km05.9e",   0x00000, 0x010000, CRC(d10aef95) SHA1(a611a35ab312caee19c31da079c647679d31673d) )
	ROM_LOAD16_BYTE( "km04.8e",   0x00001, 0x010000, CRC(329323a8) SHA1(e2ec7b059301c0a2e052dfc683e044c808ad9b33) )

	ROM_REGION( 0x300000, REGION_GFX2, 0 ) /* Tiles 2 */
	ROM_LOAD( "mbd-01.9b", 0x000000, 0x100000, CRC(d7de4f4b) SHA1(4747f8795e277ed8106667b6f68e1176d95db684) )
	ROM_LOAD( "mbd-00.8b", 0x100000, 0x100000, CRC(adb20ba9) SHA1(2ffa1dd19a438a4d2f5743b1050a8037183a3e7d) )
	/* 0x100000 bytes expanded from mbd-02.10e copied here later */

	ROM_REGION( 0x200000, REGION_GFX3, 0 ) /* Tiles 3 */
	ROM_LOAD( "mbd-03.13b",   0x000000, 0x100000, CRC(cf798f2c) SHA1(f484a22679d6a4d4b0dcac820de3f1a37cbc478f) )
	ROM_LOAD( "mbd-04.14b",   0x100000, 0x100000, CRC(d9764d0b) SHA1(74d6f09d65d073606a6e10556cedf740aa50ff08) )

	ROM_REGION( 0x400000, REGION_GFX4, 0 ) /* Sprites 1 */
	ROM_LOAD16_BYTE( "mbd-05.16b",    0x000001, 0x200000, CRC(1768c66a) SHA1(06bf3bb187c65db9dcce959a43a7231e2ac45c17) )
	ROM_LOAD16_BYTE( "mbd-06.17b",    0x000000, 0x200000, CRC(7750847a) SHA1(358266ed68a9816094e7aab0905d958284c8ce98) )

	ROM_REGION( 0x400000, REGION_GFX5, 0 ) /* Sprites 2 */
	ROM_LOAD16_BYTE( "mbd-07.18b",    0x000001, 0x200000, CRC(241faac1) SHA1(588be0cf2647c1d185a99c987a5a20ab7ad8dea8) )
	ROM_LOAD16_BYTE( "mbd-08.19b",    0x000000, 0x200000, CRC(f13b1e56) SHA1(f8f5e8c4e6c159f076d4e6505bd901ade5c6a0ca) )

	ROM_REGION( 0x0100000, REGION_GFX6, 0 ) /* 1bpp graphics */
	ROM_LOAD16_BYTE( "mbd-02.10e",    0x000000, 0x080000, CRC(b25aa721) SHA1(efe800759080bd1dac2da93bd79062a48c5da2b2) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* Oki samples 1 */
	ROM_LOAD( "mbd-10.17p",    0x000000, 0x080000, CRC(f159f76a) SHA1(0b1ea69fecdd151e2b1fa96a21eade492499691d) )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 ) /* Oki samples 1 */
	ROM_LOAD( "mbd-09.16p",    0x000000, 0x080000, CRC(f44f2f87) SHA1(d941520bdfc9e6d88c45462bc1f697c18f33498e) )

	ROM_REGION( 0x000400, REGION_PROMS, 0 ) /* Priority (not used) */
	ROM_LOAD( "kj-00.15n",    0x000000, 0x00400, CRC(add4d50b) SHA1(080e5a8192a146d5141aef5c8d9996ddf8cd3ab4) )
ROM_END

ROM_START( ragtimea ) /* VER 1.3 JPN 92.11.26 */
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* DE102 code (encrypted) */
	ROM_LOAD16_BYTE( "kh_00-1.2b",    0x000000, 0x040000, CRC(88f0155a) SHA1(6f11cc91e36cd68b7143e3326d92b258f051012e) )
	ROM_LOAD16_BYTE( "kh_02-1.2e",    0x000001, 0x040000, CRC(8811b41b) SHA1(d395338bcd812add0de3d1554d1dc3e048d0e4c9) )
	ROM_LOAD16_BYTE( "kh_01-1.4b",    0x080000, 0x040000, CRC(4dab63ad) SHA1(8c6f6e8382bcbba6e1a7ced504397181e7d6e1d1) )
	ROM_LOAD16_BYTE( "kh_03-1.4e",    0x080001, 0x040000, CRC(8a4cbb18) SHA1(272c8e2b20b0a38ce37552be00130c4117533ea9) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "km06.18p",    0x00000, 0x10000, CRC(3e8bc4e1) SHA1(7e4c357afefa47b8f101727e06485eb9ebae635d) )

	ROM_REGION( 0x20000, REGION_GFX1, 0 ) /* Tiles 1 */
	ROM_LOAD16_BYTE( "km05.9e",   0x00000, 0x010000, CRC(d10aef95) SHA1(a611a35ab312caee19c31da079c647679d31673d) )
	ROM_LOAD16_BYTE( "km04.8e",   0x00001, 0x010000, CRC(329323a8) SHA1(e2ec7b059301c0a2e052dfc683e044c808ad9b33) )

	ROM_REGION( 0x300000, REGION_GFX2, 0 ) /* Tiles 2 */
	ROM_LOAD( "mbd-01.9b", 0x000000, 0x100000, CRC(d7de4f4b) SHA1(4747f8795e277ed8106667b6f68e1176d95db684) )
	ROM_LOAD( "mbd-00.8b", 0x100000, 0x100000, CRC(adb20ba9) SHA1(2ffa1dd19a438a4d2f5743b1050a8037183a3e7d) )
	/* 0x100000 bytes expanded from mbd-02.10e copied here later */

	ROM_REGION( 0x200000, REGION_GFX3, 0 ) /* Tiles 3 */
	ROM_LOAD( "mbd-03.13b",   0x000000, 0x100000, CRC(cf798f2c) SHA1(f484a22679d6a4d4b0dcac820de3f1a37cbc478f) )
	ROM_LOAD( "mbd-04.14b",   0x100000, 0x100000, CRC(d9764d0b) SHA1(74d6f09d65d073606a6e10556cedf740aa50ff08) )

	ROM_REGION( 0x400000, REGION_GFX4, 0 ) /* Sprites 1 */
	ROM_LOAD16_BYTE( "mbd-05.16b",    0x000001, 0x200000, CRC(1768c66a) SHA1(06bf3bb187c65db9dcce959a43a7231e2ac45c17) )
	ROM_LOAD16_BYTE( "mbd-06.17b",    0x000000, 0x200000, CRC(7750847a) SHA1(358266ed68a9816094e7aab0905d958284c8ce98) )

	ROM_REGION( 0x400000, REGION_GFX5, 0 ) /* Sprites 2 */
	ROM_LOAD16_BYTE( "mbd-07.18b",    0x000001, 0x200000, CRC(241faac1) SHA1(588be0cf2647c1d185a99c987a5a20ab7ad8dea8) )
	ROM_LOAD16_BYTE( "mbd-08.19b",    0x000000, 0x200000, CRC(f13b1e56) SHA1(f8f5e8c4e6c159f076d4e6505bd901ade5c6a0ca) )

	ROM_REGION( 0x0100000, REGION_GFX6, 0 ) /* 1bpp graphics */
	ROM_LOAD16_BYTE( "mbd-02.10e",    0x000000, 0x080000, CRC(b25aa721) SHA1(efe800759080bd1dac2da93bd79062a48c5da2b2) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* Oki samples 1 */
	ROM_LOAD( "mbd-10.17p",    0x000000, 0x080000, CRC(f159f76a) SHA1(0b1ea69fecdd151e2b1fa96a21eade492499691d) )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 ) /* Oki samples 1 */
	ROM_LOAD( "mbd-09.16p",    0x000000, 0x080000, CRC(f44f2f87) SHA1(d941520bdfc9e6d88c45462bc1f697c18f33498e) )

	ROM_REGION( 0x000400, REGION_PROMS, 0 ) /* Priority (not used) */
	ROM_LOAD( "kj-00.15n",    0x000000, 0x00400, CRC(add4d50b) SHA1(080e5a8192a146d5141aef5c8d9996ddf8cd3ab4) )
ROM_END

extern void deco56_remap_gfx(int region);
extern void deco102_decrypt_cpu(int address_xor, int data_select_xor, int opcode_select_xor);

static DRIVER_INIT( boogwing )
{
	const UINT8* src=memory_region(REGION_GFX6);
	UINT8* dst=memory_region(REGION_GFX2) + 0x200000;

	deco56_decrypt(REGION_GFX1);
	deco56_decrypt(REGION_GFX2);
	deco56_decrypt(REGION_GFX3);
	deco56_remap_gfx(REGION_GFX6);
	deco102_decrypt_cpu(0x42ba, 0x00, 0x18);
	memcpy(dst, src, 0x100000);
}

GAME( 1992, boogwing, 0,        boogwing, boogwing,  boogwing,  ROT0, "Data East Corporation", "Boogie Wings (Euro v1.5)" )
GAME( 1992, boogwinga,boogwing, boogwing, boogwing,  boogwing,  ROT0, "Data East Corporation", "Boogie Wings (Asia v1.5)" )
GAME( 1992, ragtime,  boogwing, boogwing, boogwing,  boogwing,  ROT0, "Data East Corporation", "The Great Ragtime Show (Japan v1.5)" )
GAME( 1992, ragtimea, boogwing, boogwing, boogwing,  boogwing,  ROT0, "Data East Corporation", "The Great Ragtime Show (Japan v1.3)" )
