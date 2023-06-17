/*
    SemiCom 68020 based hardware
    Driver by David Haywood

Baryon - Future Assault          (c) 1997 SemiCom / Tirano
Cute Fighter                     (c) 1998 SemiCom
Rolling Crush                    (c) 1999 Trust / SemiCom
Gaia - The Last Choice of Earth  (c) 1999 SemiCom / XESS
Dream World                      (c) 2000 SemiCom

Note: There is a SemiCom game known as Lode Quest 1998(?). This game is very similar to Dream World.
      It's not known if Lode Quest is a alternate title or a prequel of Dream World.

Note: this hardware is a copy of Psikyo's 68020 based hardware,
      the Strikers 1945 bootleg has the same unknown rom!
      It isn't quite as flexible as the original Psikyo hardware
      by the looks of it, there are various subtle changes to how
      things work, for example the tilemap sizes and missing
      transparent pen modification.  This makes it rather hard to
      merge with psikyo.c and it should probably be left separate.

Stephh's notes (based on the game M68EC020 code and some tests) :

  - Don't trust the "test mode" as it displays Dip Switches infos
    that are in fact unused by the game ! Leftover from another game ?

    PORT_START("DSW")
    PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Lives ) )        PORT_DIPLOCATION("SW2:1,2")
    PORT_DIPSETTING(      0x0002, "1" )
    PORT_DIPSETTING(      0x0003, "2" )
    PORT_DIPSETTING(      0x0001, "3" )
    PORT_DIPSETTING(      0x0000, "4" )
    PORT_DIPUNUSED_DIPLOC( 0x0004, IP_ACTIVE_LOW, "SW2:3" )
    PORT_DIPUNUSED_DIPLOC( 0x0008, IP_ACTIVE_LOW, "SW2:4" )
    PORT_DIPUNUSED_DIPLOC( 0x0010, IP_ACTIVE_LOW, "SW2:5" )
    PORT_DIPNAME( 0x0060, 0x0060, "Ticket Payout" )         PORT_DIPLOCATION("SW2:6,7")
    PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
    PORT_DIPSETTING(      0x0020, "Little" )
    PORT_DIPSETTING(      0x0060, DEF_STR( Normal ) )
    PORT_DIPSETTING(      0x0040, "Much" )
    PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Free_Play ) )    PORT_DIPLOCATION("SW2:8")
    PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
    PORT_DIPNAME( 0x0100, 0x0000, DEF_STR( Demo_Sounds ) )  PORT_DIPLOCATION("SW1:1")
    PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
    PORT_DIPNAME( 0x0e00, 0x0e00, DEF_STR( Coinage ) )      PORT_DIPLOCATION("SW1:2,3,4")
    PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
    PORT_DIPSETTING(      0x0200, DEF_STR( 4C_1C ) )
    PORT_DIPSETTING(      0x0400, DEF_STR( 3C_1C ) )
    PORT_DIPSETTING(      0x0600, DEF_STR( 2C_1C ) )
    PORT_DIPSETTING(      0x0e00, DEF_STR( 1C_1C ) )
    PORT_DIPSETTING(      0x0a00, DEF_STR( 2C_3C ) )
    PORT_DIPSETTING(      0x0c00, DEF_STR( 1C_2C ) )
    PORT_DIPSETTING(      0x0800, DEF_STR( 1C_3C ) )
    PORT_DIPNAME( 0x7000, 0x7000, DEF_STR( Difficulty ) )   PORT_DIPLOCATION("SW1:5,6,7")
    PORT_DIPSETTING(      0x2000, "Level 1" )
    PORT_DIPSETTING(      0x1000, "Level 2" )
    PORT_DIPSETTING(      0x0000, "Level 3" )
    PORT_DIPSETTING(      0x7000, "Level 4" )
    PORT_DIPSETTING(      0x6000, "Level 5" )
    PORT_DIPSETTING(      0x5000, "Level 6" )
    PORT_DIPSETTING(      0x4000, "Level 7" )
    PORT_DIPSETTING(      0x3000, "Level 8" )
    PORT_SERVICE_DIPLOC( 0x8000, IP_ACTIVE_LOW, "SW1:8" )

   note:
   Baryon has some annoying sound looping clicks / cutouts, these need to
    be verified against the HW (it's a very cheap sound system, so it might
    be accurate)

   Baryon has playfield background which fade in with very rough / visible
    edges.  In this case the tilemap size registers from the original
    psikyo hardware are set to be the alternate tilemap size, however that
    doesn't make sense in the context of the data in RAM, which doesn't
    appear to wrap properly anyway, again, it's likely this is just how the
    game is.  Furthermore the BG test in the test menu indicates that it
    tests alternate tilemap sizes, but doesn't even write to the register,
    probably a leftover from hardware development as the test menu is mostly
    incomplete.

   All: sprite priority, the original psikyo.c HW has sprite<->tilemap
    priority but we don't support it here, does the clone HW support it?

   All: sprite zooming, again the original psikyo.c HW supports this, but we
    don't support it here.  The Strikers 1945 bootleg in psikyo.c doesn't
    appear to support it properly either, so it might be missing on these
    clone boards.
*/
#include "driver.h"
#include "cpu/m68000/m68000.h"

#define MASTER_CLOCK 32000000
#define ENABLE_FAST_BG_DRAW  1 /* if enabled, inaccurate, but faster background drawing is enabled*/

static unsigned int *spriteram32;
static unsigned int *spritebuf32a;
static unsigned int *spritebuf32b;

static UINT32*dreamwld_bg_videoram;
static UINT32*dreamwld_bg2_videoram;
static UINT32*dreamwld_bg_scroll;
static int dreamwld_tilebank[2], dreamwld_tilebankold[2];
int layer0_scrolly;
int layer1_scrolly;
UINT32 layer0_ctrl;
UINT32 layer1_ctrl;
int layer0_scrollx;
int layer1_scrollx;
int j;


static struct tilemap *dreamwld_bg_tilemap;
static struct tilemap *dreamwld_bg2_tilemap;


static void draw_sprites(struct mame_bitmap *bitmap)
{
	UINT32 *source = spritebuf32b;			/* sprbuf iq_132 fix me!*/
	UINT32 *finish = spritebuf32b + 0x1000 / 4;
	UINT16 *redirect = (UINT16 *)memory_region(REGION_GFX3);

	while (source < finish)
	{
		int xpos, ypos, tileno;
		int xsize, ysize, xinc, yinc;
		int xct, yct;
		int xflip;
		int yflip;
		int colour;

		xpos  = (source[0] & 0x000001ff) >> 0;
		ypos  = (source[0] & 0x01ff0000) >> 16;
		xsize = (source[0] & 0x00000e00) >> 9;
		ysize = (source[0] & 0x0e000000) >> 25;


		tileno = (source[1] & 0x0001ffff) >>0; /*gamezfan For Cute Fighter Sprites */
		colour = (source[1] & 0x3f000000) >>24;
		xflip  = (source[1] & 0x40000000);
		yflip  = (source[1] & 0x80000000);

		xinc = 16;
		yinc = 16;

		if (xflip)
		{
			xinc = -16;
			xpos += 16 * xsize;
		}

		if (yflip)
		{
			yinc = -16;
			ypos += 16 * ysize;
		}

		ysize++; xsize++; /* size 0 = 1 tile*/

		xpos -=16;


		for (yct = 0; yct < ysize; yct++)
		{
			for (xct = 0; xct < xsize; xct++)
			{
				drawgfx(bitmap,Machine->gfx[0],redirect[tileno],colour,xflip,yflip,xpos+xct*xinc,ypos+yct*yinc,&Machine->visible_area,TRANSPARENCY_PEN,0);
				drawgfx(bitmap,Machine->gfx[0],redirect[tileno],colour,xflip,yflip,(xpos+xct*xinc)-0x200,ypos+yct*yinc,&Machine->visible_area,TRANSPARENCY_PEN,0);
				drawgfx(bitmap,Machine->gfx[0],redirect[tileno],colour,xflip,yflip,(xpos+xct*xinc)-0x200,(ypos+yct*yinc)-0x200,&Machine->visible_area,TRANSPARENCY_PEN,0);
				drawgfx(bitmap,Machine->gfx[0],redirect[tileno],colour,xflip,yflip,xpos+xct*xinc,(ypos+yct*yinc)-0x200,&Machine->visible_area,TRANSPARENCY_PEN,0);

				tileno++;
			}
		}

		source += 2;
	}
}

static WRITE32_HANDLER( dreamwld_bg_videoram_w )
{
	COMBINE_DATA(&dreamwld_bg_videoram[offset]);
	tilemap_mark_tile_dirty(dreamwld_bg_tilemap,offset*2);
	tilemap_mark_tile_dirty(dreamwld_bg_tilemap,offset*2+1);

}

static void get_dreamwld_bg_tile_info(int tile_index)
{
	int tileno,colour;
	tileno = (tile_index&1)?(dreamwld_bg_videoram[tile_index>>1]&0xffff):((dreamwld_bg_videoram[tile_index>>1]>>16)&0xffff);
	colour = tileno >> 13;
	tileno &=0x1fff;
	SET_TILE_INFO(1,tileno+dreamwld_tilebank[0]*0x2000,0x80+colour,0);
}


static WRITE32_HANDLER( dreamwld_bg2_videoram_w )
{
	COMBINE_DATA(&dreamwld_bg2_videoram[offset]);
	tilemap_mark_tile_dirty(dreamwld_bg2_tilemap,offset*2);
	tilemap_mark_tile_dirty(dreamwld_bg2_tilemap,offset*2+1);
}

static void get_dreamwld_bg2_tile_info(int tile_index)
{
	UINT16 tileno,colour;
	tileno = (tile_index&1)?(dreamwld_bg2_videoram[tile_index>>1]&0xffff):((dreamwld_bg2_videoram[tile_index>>1]>>16)&0xffff);
	colour = tileno >> 13;
	tileno &=0x1fff;
	SET_TILE_INFO(1,tileno+dreamwld_tilebank[1]*0x2000,0xc0+colour,0);
}

static VIDEO_START( dreamwld )
{
	dreamwld_bg_tilemap = tilemap_create(get_dreamwld_bg_tile_info, tilemap_scan_rows, TILEMAP_OPAQUE, 16, 16, 64,64); /*gamezfan for Gaialast changed to 16,16,64,64*/
	dreamwld_bg2_tilemap = tilemap_create(get_dreamwld_bg2_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 16, 16, 64,64); /*^^*/
	tilemap_set_transparent_pen(dreamwld_bg2_tilemap,0);
	dreamwld_tilebankold[0] = dreamwld_tilebankold[1] = -1;
	dreamwld_tilebank[0] = dreamwld_tilebank[1] = 0;

#ifndef ENABLE_FAST_BG_DRAW
	tilemap_set_scroll_rows(dreamwld_bg_tilemap,256);
	tilemap_set_scroll_rows(dreamwld_bg2_tilemap,256);
	tilemap_set_scroll_cols(dreamwld_bg_tilemap,1);
	tilemap_set_scroll_cols(dreamwld_bg2_tilemap,1);
#endif

	spritebuf32a = auto_malloc(0x2000);
	spritebuf32b = auto_malloc(0x2000);

	return 0;
}

static VIDEO_UPDATE( dreamwld )
{
	dreamwld_tilebank[0] = (dreamwld_bg_scroll[(0x400/4)+4]>>6)&1;
	dreamwld_tilebank[1] = (dreamwld_bg_scroll[(0x400/4)+5]>>6)&1;

#ifdef ENABLE_FAST_BG_DRAW /* FAST! OLD STYLE, NO LINE SCROLL*/

	tilemap_set_scrolly( dreamwld_bg_tilemap,0, dreamwld_bg_scroll[(0x400/4)]+32 );
	tilemap_set_scrolly( dreamwld_bg2_tilemap,0, dreamwld_bg_scroll[(0x400/4)+2]+32 );
	tilemap_set_scrollx( dreamwld_bg_tilemap,0, dreamwld_bg_scroll[(0x400/4)+1]+3 );
	tilemap_set_scrollx( dreamwld_bg2_tilemap,0, dreamwld_bg_scroll[(0x400/4)+3]+5 );

#else	/* SLOW! NEW STYLE. LINE SCROLL*/

	layer0_scrolly = dreamwld_bg_scroll[(0x400 / 4)] + 32;
	layer1_scrolly = dreamwld_bg_scroll[(0x400 / 4) + 2] + 32;

	layer0_scrollx = dreamwld_bg_scroll[(0x400 / 4) + 1] + 3;
	layer1_scrollx = dreamwld_bg_scroll[(0x400 / 4) + 3] + 5;
	layer0_ctrl = dreamwld_bg_scroll[0x412 / 4];
	layer1_ctrl = dreamwld_bg_scroll[0x416 / 4];

	tilemap_set_scrolly(dreamwld_bg_tilemap,  0, layer0_scrolly);
	tilemap_set_scrolly(dreamwld_bg2_tilemap, 0, layer1_scrolly);

	for (j = 0; j < 256; j++)	/* 256 screen lines */
	{
		int x0 = 0, x1 = 0;

		/* layer 0 */
		if (layer0_ctrl & 0x0300)
		{
			if (layer0_ctrl & 0x0200)
				/* per-tile rowscroll */
				x0 = ((UINT16 *)dreamwld_bg_scroll)[0x000/2 + j/16];
			else
				/* per-line rowscroll */
				x0 = ((UINT16 *)dreamwld_bg_scroll)[(0x000/2 + ((j + layer0_scrolly)&0xff))]; /* different handling to psikyo.c? ( + scrolly )*/
		}

		tilemap_set_scrollx(
			dreamwld_bg_tilemap,
			(j + layer0_scrolly) % 256 /*tilemap_width(tm0size) */,
			layer0_scrollx + x0 );


		/* layer 1 */
		if (layer1_ctrl & 0x0300)
		{
			if (layer1_ctrl & 0x0200)
				/* per-tile rowscroll */
				x1 = ((UINT16 *)dreamwld_bg_scroll)[(0x200/2 + j/16)];
			else
				/* per-line rowscroll */
				x1 = ((UINT16 *)dreamwld_bg_scroll)[(0x200/2 + ((j + layer1_scrolly)&0xff))];  /* different handling to psikyo.c? ( + scrolly )*/
		}

		tilemap_set_scrollx(
			dreamwld_bg2_tilemap,
			(j + layer1_scrolly) % 256 /* tilemap_width(tm1size) */,
			layer1_scrollx + x1 );
	}

#endif

	if (dreamwld_tilebank[0] != dreamwld_tilebankold[0])
	{
		dreamwld_tilebankold[0] = dreamwld_tilebank[0];
		tilemap_mark_all_tiles_dirty (dreamwld_bg_tilemap);
	}

	if (dreamwld_tilebank[1] != dreamwld_tilebankold[1])
	{
		dreamwld_tilebankold[1] = dreamwld_tilebank[1];
		tilemap_mark_all_tiles_dirty (dreamwld_bg2_tilemap);
	}

	tilemap_draw(bitmap,cliprect,dreamwld_bg_tilemap,0,0);
	tilemap_draw(bitmap,cliprect,dreamwld_bg2_tilemap,0,0);

	draw_sprites(bitmap);
}

VIDEO_EOF( dreamwld )
{
	memcpy(spritebuf32b, spritebuf32a, 0x2000);
	memcpy(spritebuf32a, spriteram32,  0x2000);
}

static READ32_HANDLER( dreamwld_protdata_r )
{
	static int protindex = 0;
	UINT8 *protdata    = memory_region(REGION_USER1);
	size_t  protsize = memory_region_length(REGION_USER1);
	UINT8 dat = protdata[(protindex++)%protsize];
	return dat<<24;
}


static WRITE32_HANDLER( dreamwld_palette_w )
{
	UINT16 dat;
	int color;

	COMBINE_DATA(&paletteram32[offset]);
	color = offset * 2;

	dat = paletteram32[offset] & 0x7fff;
	palette_set_color(color + 1, pal5bit(dat >> 10), pal5bit(dat >> 5), pal5bit(dat >> 0));

	dat = (paletteram32[offset] >> 16) & 0x7fff;
	palette_set_color(color, pal5bit(dat >> 10), pal5bit(dat >> 5), pal5bit(dat >> 0));
}

static void dreamwld_oki_setbank(UINT8 chip, UINT8 bank )
{
	UINT8 *sound = memory_region(chip ? REGION_SOUND1 : REGION_SOUND2);

	memcpy(sound+0x30000, sound+0xb0000+0x10000*bank, 0x10000);
}

static WRITE32_HANDLER( dreamwld_6295_0_bank_w )
{
	if (1) /*ACCESSING_BITS_0_7)*/
	{
		dreamwld_oki_setbank(0,data&0x3);
	}
	else
	{
	/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "OKI0: unk bank write %x mem_mask %8x\n", data, mem_mask);*/
	}
}

static WRITE32_HANDLER( dreamwld_6295_1_bank_w )
{
	if (1) /*ACCESSING_BITS_0_7)*/
	{
		dreamwld_oki_setbank(1,data&0x3);
	}
	else
	{
	/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "OKI1: unk bank write %x mem_mask %8x\n", data, mem_mask);*/
	}
}

static READ32_HANDLER(dreamwld_6295_0_r)
{
	return OKIM6295_status_0_r(0) << 24;
}

static WRITE32_HANDLER(dreamwld_6295_0_w)
{
	if (1) /* msb*/
	{
		OKIM6295_data_0_w(0, data >> 24);
	}
}

static READ32_HANDLER(dreamwld_6295_1_r)
{
	return OKIM6295_status_1_r(0) << 24;
}

static WRITE32_HANDLER(dreamwld_6295_1_w)
{
	if (1) /* msb*/
	{
		OKIM6295_data_1_w(0, data >> 24);
	}
}


static READ32_HANDLER(inputs_32_r)
{
	return (readinputport(1) << 16) | readinputport(0);
}

static READ32_HANDLER(dips_32_r)
{
	return (readinputport(2) << 16) | readinputport(2);
}


static MEMORY_READ32_START( dreamwld_readmem )
    { 0x000000, 0x0fffff, MRA32_ROM },
	{ 0x400000, 0x401fff, MRA32_RAM },
	{ 0x600000, 0x601fff, MRA32_RAM },
	{ 0x800000, 0x801fff, MRA32_RAM },
	{ 0x802000, 0x803fff, MRA32_RAM },
	{ 0x804000, 0x805fff, MRA32_RAM },
	{ 0xc00018, 0xc0001b, dreamwld_6295_0_r },
	{ 0xc00028, 0xc0002b, dreamwld_6295_1_r },
	{ 0xc00030, 0xc00033, dreamwld_protdata_r },
	{ 0xc00000, 0xc00003, inputs_32_r },
	{ 0xc00004, 0xc00007, dips_32_r },
	{ 0xfe0000, 0xffffff, MRA32_RAM },
MEMORY_END
	
static MEMORY_WRITE32_START( dreamwld_writemem )
    { 0x000000, 0x0fffff, MWA32_ROM },
	{ 0x400000, 0x401fff, MWA32_RAM, &spriteram32 },
	{ 0x600000, 0x601fff, dreamwld_palette_w, &paletteram32 },
	{ 0x800000, 0x801fff, dreamwld_bg_videoram_w, &dreamwld_bg_videoram },
	{ 0x802000, 0x803fff, dreamwld_bg2_videoram_w, &dreamwld_bg2_videoram },
	{ 0x804000, 0x805fff, MWA32_RAM, &dreamwld_bg_scroll },  /* scroll regs etc.*/
	{ 0xc0000c, 0xc0000f, dreamwld_6295_0_bank_w }, /* sfx*/
	{ 0xc00018, 0xc0001b, dreamwld_6295_0_w },
	{ 0xc0002c, 0xc0002f, dreamwld_6295_1_bank_w }, /* sfx*/
	{ 0xc00028, 0xc0002b, dreamwld_6295_1_w },
	{ 0xfe0000, 0xffffff, MWA32_RAM },
MEMORY_END


INPUT_PORTS_START( dreamwld )
	/* mame can't handle 32-bit inputs, so we set them as 16-bit and assemble them later*/
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0xfffc, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 ) /* "Book" (when you get one of them) */
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 ) /* "Jump" */
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 ) /* "Dig" */
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 ) 
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 ) 
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 ) /* "Book" (when you get one of them) */
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 ) /* "Jump" */
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 ) /* "Dig" */
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1 ) 
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1 ) 
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1 ) 

	PORT_START		/* 16bit */
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Lives ) ) 
	PORT_DIPSETTING(      0x0002, "2" )
	PORT_DIPSETTING(      0x0003, "3" )
	PORT_DIPSETTING(      0x0001, "4" )
	PORT_DIPSETTING(      0x0000, "5" )
	PORT_DIPNAME( 0x007c, 0x007c, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x007c, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Free_Play ) )  
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0e00, 0x0e00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0600, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0e00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0a00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0c00, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x7000, 0x7000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x7000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x8000, IP_ACTIVE_LOW )
INPUT_PORTS_END

INPUT_PORTS_START( baryon )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0xfffc, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 ) 
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 ) 
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1 ) 
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1 ) 
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1 ) 

	PORT_START		/* 16bit */
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Lives ) ) 
	PORT_DIPSETTING(      0x0002, "2" )
	PORT_DIPSETTING(      0x0003, "3" )
	PORT_DIPSETTING(      0x0001, "4" )
	PORT_DIPSETTING(      0x0000, "5" )
	PORT_DIPNAME( 0x0004, 0x0004, "Bomb Stock" )
	PORT_DIPSETTING(      0x0004, "2" )
	PORT_DIPSETTING(      0x0000, "3" )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Free_Play ) )  
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0e00, 0x0e00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0600, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0e00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0a00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0c00, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x7000, 0x7000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x7000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x8000, IP_ACTIVE_LOW )
INPUT_PORTS_END


INPUT_PORTS_START( rolcrush )
   PORT_START
   PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
   PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
   PORT_BIT( 0xfffc, IP_ACTIVE_LOW, IPT_UNUSED )

   PORT_START
   PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START2 )
   PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
   PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
   PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
   PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 ) 
   PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
   PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 ) 
   PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
   PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
   PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
   PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
   PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
   PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1 ) 
   PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
   PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1 ) 
   PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1 ) 


   PORT_START
   PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unknown ) )
   PORT_DIPSETTING( 0x0001, DEF_STR( Off ) )
   PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
   PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
   PORT_DIPSETTING( 0x0002, DEF_STR( Off ) )
   PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
   PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
   PORT_DIPSETTING( 0x0004, DEF_STR( Off ) )
   PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
   PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
   PORT_DIPSETTING( 0x0008, DEF_STR( Off ) )
   PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
   PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
   PORT_DIPSETTING( 0x0010, DEF_STR( Off ) )
   PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
   PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
   PORT_DIPSETTING( 0x0020, DEF_STR( Off ) )
   PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
   PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
   PORT_DIPSETTING( 0x0040, DEF_STR( Off ) )
   PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
   PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Free_Play ) ) 
   PORT_DIPSETTING( 0x0080, DEF_STR( Off ) )
   PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
   PORT_DIPNAME( 0x0100, 0x0000, DEF_STR( Demo_Sounds ) )
   PORT_DIPSETTING( 0x0100, DEF_STR( Off ) )
   PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
   PORT_DIPNAME( 0x0e00, 0x0e00, DEF_STR( Coinage ) )
   PORT_DIPSETTING( 0x0000, DEF_STR( 5C_1C ) )
   PORT_DIPSETTING( 0x0200, DEF_STR( 4C_1C ) )
   PORT_DIPSETTING( 0x0400, DEF_STR( 3C_1C ) )
   PORT_DIPSETTING( 0x0600, DEF_STR( 2C_1C ) )
   PORT_DIPSETTING( 0x0e00, DEF_STR( 1C_1C ) )
   PORT_DIPSETTING( 0x0a00, DEF_STR( 2C_3C ) )
   PORT_DIPSETTING( 0x0c00, DEF_STR( 1C_2C ) )
   PORT_DIPSETTING( 0x0800, DEF_STR( 1C_3C ) )
   PORT_DIPNAME( 0x7000, 0x7000, DEF_STR( Difficulty ) )
   PORT_DIPSETTING( 0x2000, "Level 1" )
   PORT_DIPSETTING( 0x1000, "Level 2" )
   PORT_DIPSETTING( 0x0000, "Level 3" )
   PORT_DIPSETTING( 0x7000, "Level 4" )
   PORT_DIPSETTING( 0x6000, "Level 5" )
   PORT_DIPSETTING( 0x5000, "Level 6" )
   PORT_DIPSETTING( 0x4000, "Level 7" )
   PORT_DIPSETTING( 0x3000, "Level 8" )
   PORT_SERVICE( 0x8000, IP_ACTIVE_LOW )
INPUT_PORTS_END

INPUT_PORTS_START( cutefght )
   PORT_START
   PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
   PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
   PORT_BIT( 0xfffc, IP_ACTIVE_LOW, IPT_UNUSED )

   PORT_START
   PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START2 )
   PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
   PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
   PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
   PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 ) 
   PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
   PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 ) 
   PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
   PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
   PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
   PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
   PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
   PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1 ) 
   PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
   PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1 ) 
   PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1 ) 

   PORT_START
   PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unknown ) )
   PORT_DIPSETTING( 0x0001, DEF_STR( Off ) )
   PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
   PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
   PORT_DIPSETTING( 0x0002, DEF_STR( Off ) )
   PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
   PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
   PORT_DIPSETTING( 0x0004, DEF_STR( Off ) )
   PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
   PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
   PORT_DIPSETTING( 0x0008, DEF_STR( Off ) )
   PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
   PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
   PORT_DIPSETTING( 0x0010, DEF_STR( Off ) )
   PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
   PORT_DIPNAME( 0x0060, 0x0060, "Ticket Payout" ) 
   PORT_DIPSETTING( 0x0000, DEF_STR( No ) )
   PORT_DIPSETTING( 0x0020, "Little" )
   PORT_DIPSETTING( 0x0060, "Normal" ) 
   PORT_DIPSETTING( 0x0040, "Much" )
   PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Free_Play ) )
   PORT_DIPSETTING( 0x0080, DEF_STR( Off ) )
   PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
   PORT_DIPNAME( 0x0100, 0x0000, DEF_STR( Demo_Sounds ) ) 
   PORT_DIPSETTING( 0x0100, DEF_STR( Off ) )
   PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
   PORT_DIPNAME( 0x0e00, 0x0e00, DEF_STR( Coinage ) )
   PORT_DIPSETTING( 0x0000, DEF_STR( 5C_1C ) )
   PORT_DIPSETTING( 0x0200, DEF_STR( 4C_1C ) )
   PORT_DIPSETTING( 0x0400, DEF_STR( 3C_1C ) )
   PORT_DIPSETTING( 0x0600, DEF_STR( 2C_1C ) )
   PORT_DIPSETTING( 0x0e00, DEF_STR( 1C_1C ) )
   PORT_DIPSETTING( 0x0a00, DEF_STR( 2C_3C ) )
   PORT_DIPSETTING( 0x0c00, DEF_STR( 1C_2C ) )
   PORT_DIPSETTING( 0x0800, DEF_STR( 1C_3C ) )
   PORT_DIPNAME( 0x7000, 0x7000, DEF_STR( Difficulty ) )
   PORT_DIPSETTING( 0x2000, "Level 1" )
   PORT_DIPSETTING( 0x1000, "Level 2" )
   PORT_DIPSETTING( 0x0000, "Level 3" )
   PORT_DIPSETTING( 0x7000, "Level 4" )
   PORT_DIPSETTING( 0x6000, "Level 5" )
   PORT_DIPSETTING( 0x5000, "Level 6" )
   PORT_DIPSETTING( 0x4000, "Level 7" )
   PORT_DIPSETTING( 0x3000, "Level 8" )
   PORT_SERVICE( 0x8000, IP_ACTIVE_LOW )
INPUT_PORTS_END

INPUT_PORTS_START( gaialast )
   PORT_START
   PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
   PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
   PORT_BIT( 0xfffc, IP_ACTIVE_LOW, IPT_UNUSED )

   PORT_START
   PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START2 )
   PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
   PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
   PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
   PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 ) 
   PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
   PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 ) 
   PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
   PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
   PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
   PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
   PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
   PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1 ) 
   PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
   PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1 ) 
   PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1 ) 
 
   PORT_START
   PORT_DIPNAME( 0x0003,  0x0001, DEF_STR( Lives )  ) 
   PORT_DIPSETTING(       0x0002, "1" )
   PORT_DIPSETTING(       0x0003, "2" )
   PORT_DIPSETTING(       0x0001, "3" )
   PORT_DIPSETTING(       0x0000, "4" )
   PORT_DIPNAME( 0x0004,  0x0000, "Bomb Stock" ) 
   PORT_DIPSETTING(       0x0004, "2" )
   PORT_DIPSETTING(       0x0000, "3" )
   PORT_DIPNAME( 0x0008,  0x0000, "Lock Vertical Scroll" )  /* if Off the game pans the screen up/down when you move up/down*/
   PORT_DIPSETTING(       0x0008, DEF_STR( Off ) )
   PORT_DIPSETTING(       0x0000, DEF_STR( On ) )
   PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
   PORT_DIPSETTING( 0x0010, DEF_STR( Off ) )
   PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
   PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
   PORT_DIPSETTING( 0x0020, DEF_STR( Off ) )
   PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
   PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
   PORT_DIPSETTING( 0x0040, DEF_STR( Off ) )
   PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
   PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Free_Play ) )
   PORT_DIPSETTING( 0x0080, DEF_STR( Off ) )
   PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
   PORT_DIPNAME( 0x0100, 0x0000, DEF_STR( Demo_Sounds ) ) 
   PORT_DIPSETTING( 0x0100, DEF_STR( Off ) )
   PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
   PORT_DIPNAME( 0x0e00, 0x0e00, DEF_STR( Coinage ) )
   PORT_DIPSETTING( 0x0000, DEF_STR( 5C_1C ) )
   PORT_DIPSETTING( 0x0200, DEF_STR( 4C_1C ) )
   PORT_DIPSETTING( 0x0400, DEF_STR( 3C_1C ) )
   PORT_DIPSETTING( 0x0600, DEF_STR( 2C_1C ) )
   PORT_DIPSETTING( 0x0e00, DEF_STR( 1C_1C ) )
   PORT_DIPSETTING( 0x0a00, DEF_STR( 2C_3C ) )
   PORT_DIPSETTING( 0x0c00, DEF_STR( 1C_2C ) )
   PORT_DIPSETTING( 0x0800, DEF_STR( 1C_3C ) )
   PORT_DIPNAME( 0x7000, 0x7000, DEF_STR( Difficulty ) )
   PORT_DIPSETTING( 0x2000, "Level 1" )
   PORT_DIPSETTING( 0x1000, "Level 2" )
   PORT_DIPSETTING( 0x0000, "Level 3" )
   PORT_DIPSETTING( 0x7000, "Level 4" )
   PORT_DIPSETTING( 0x6000, "Level 5" )
   PORT_DIPSETTING( 0x5000, "Level 6" )
   PORT_DIPSETTING( 0x4000, "Level 7" )
   PORT_DIPSETTING( 0x3000, "Level 8" )
   PORT_SERVICE( 0x8000, IP_ACTIVE_LOW )
INPUT_PORTS_END


static struct GfxLayout tiles16x16_layout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{0,1,2,3},
	{2*4,3*4,0*4,1*4,6*4,7*4,4*4,5*4,
	 10*4,11*4,8*4,9*4,14*4,15*4,12*4,13*4},
	{0*64,1*64,2*64,3*64,4*64,5*64,6*64,7*64,
	 8*64,9*64,10*64,11*64,12*64,13*64,14*64,15*64},
	16*16*4
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tiles16x16_layout,  0x0000, 256 },
	{ REGION_GFX2, 0, &tiles16x16_layout,  0x0000, 256 },
	{ -1 }
};

static struct OKIM6295interface m6295_interface =
{
	2,  /* 2 chips */
	{ MASTER_CLOCK/32/165, MASTER_CLOCK/32/165 }, /* unknown, but sounds good */
	{ REGION_SOUND1, REGION_SOUND2 },
	{ 50,50 }
};


static MACHINE_DRIVER_START( dreamwld )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68EC020, MASTER_CLOCK/2)
	MDRV_CPU_MEMORY(dreamwld_readmem,dreamwld_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)

	MDRV_FRAMES_PER_SECOND(57.793)
	MDRV_VBLANK_DURATION(TIME_IN_USEC(0))

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0, 304-1, 0, 224-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x1000)

	MDRV_VIDEO_START(dreamwld)
	MDRV_VIDEO_EOF(dreamwld)
	MDRV_VIDEO_UPDATE(dreamwld)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(OKIM6295, m6295_interface)
MACHINE_DRIVER_END


ROM_START( baryon )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD32_BYTE( "3.bin", 0x000002, 0x040000, CRC(046d4231) SHA1(05056efe5fec7f43c400f05278de516b01be0fdf) )
	ROM_LOAD32_BYTE( "4.bin", 0x000000, 0x040000, CRC(59e0df20) SHA1(ff12f4adcf731f6984db7d0fbdd7fcc71ce66aa4) )
	ROM_LOAD32_BYTE( "5.bin", 0x000003, 0x040000, CRC(63d5e7cb) SHA1(269bf5ffe10f2464f823c4d377921e19cfb8bc46) )
	ROM_LOAD32_BYTE( "6.bin", 0x000001, 0x040000, CRC(abccbb3d) SHA1(01524f094543d872d775306024f51258a11e9240) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 87C52 MCU Code */
/*	ROM_LOAD( "87c52.mcu", 0x00000, 0x10000 , NO_DUMP )  // can't be dumped. /*/

	ROM_REGION( 0x6bd, REGION_USER1, 0 ) /* Protection data  */
	ROM_LOAD( "protdata.bin", 0x000, 0x6bd, CRC(117f32a8) SHA1(837bea09d3e59ab9e13bd1103b1fc988edb361c0) ) /* extracted */

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* OKI Samples */
	ROM_LOAD( "1.bin", 0x000000, 0x80000, CRC(e0349074) SHA1(f3d53d96dff586a0ad1632f52e5559cdce5ed0d8) )
	ROM_RELOAD(0x80000,0x80000) /* for the banks*/

	ROM_REGION( 0x100000, REGION_SOUND2, 0 ) /* OKI Samples */

	ROM_REGION( 0x400000, REGION_GFX1, 0 ) /* Sprite Tiles - decoded */
	ROM_LOAD16_WORD_SWAP( "9.bin",  0x000000, 0x200000, CRC(28bf828f) SHA1(271390cc4f4015a3b69976f0d0527947f13c971b) )
	ROM_LOAD16_WORD_SWAP( "11.bin", 0x200000, 0x200000, CRC(d0ff1bc6) SHA1(4aeb795222eedeeba770cf725122e989f97119b2) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 ) /* BG Tiles - decoded */
	ROM_LOAD16_WORD_SWAP( "2.bin",0x000000, 0x200000, CRC(684012e6) SHA1(4cb60907184b67be130b8385e4336320c0f6e4a7) )

	ROM_REGION( 0x040000, REGION_GFX3, 0 ) /* Sprite Code Lookup ... */
	ROM_LOAD16_BYTE( "8.bin", 0x000000, 0x020000, CRC(fdbb08b0) SHA1(4b3ac56c4c8370b1434fb6a481fce0d9c52313e0) )
	ROM_LOAD16_BYTE( "10.bin",0x000001, 0x020000, CRC(c9d20480) SHA1(3f6170e8e08fb7508bd13c23f243ec6888a91f5e) )

	ROM_REGION( 0x10000, REGION_GFX4, 0 ) /* ???? - not decoded seems to be in blocks of 0x41 bytes.. */
	ROM_LOAD( "7.bin", 0x000000, 0x10000, CRC(0da8db45) SHA1(7d5bd71c5b0b28ff74c732edd7c662f46f2ab25b) )
ROM_END

ROM_START( cutefght )
    ROM_REGION( 0x200000, REGION_CPU1, 0 )
    ROM_LOAD32_BYTE( "5_semicom", 0x000000, 0x080000, CRC(c14fd5dc) SHA1(f332105f5f249d693e792e7115f9e6cffb6db19f) )
    ROM_LOAD32_BYTE( "6_semicom", 0x000001, 0x080000, CRC(47440088) SHA1(c45503c4b5f271b430263ca079edeaaeadf5d9f6) )
    ROM_LOAD32_BYTE( "3_semicom", 0x000002, 0x080000, CRC(e7e7a866) SHA1(a31751f4164a427de59f0c76c9a8cb34370d8183) )
    ROM_LOAD32_BYTE( "4_semicom", 0x000003, 0x080000, CRC(476a3bf5) SHA1(5be1c70bbf4fcfc534b7f20bfceaa8da2e961330) )

    ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 87C52 MCU Code */
   /* ROM_LOAD( "87c52.mcu", 0x00000, 0x10000 , NO_DUMP )  // can't be dumped. /*/

    ROM_REGION( 0x701, REGION_USER1, ROMREGION_ERASEFF ) /* Protection data */
    ROM_LOAD( "protdata.bin", 0x000, 0x701 , CRC(764c3c0e) SHA1(ae044d016850b730b2d97ccb7845b6b438c1e074) )

    ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* OKI Samples */
    ROM_LOAD( "2_semicom", 0x000000, 0x80000, CRC(694ddaf9) SHA1(f9138e7e1d8f771c4e69c17f27fb2b70fbee076a) )
	
    ROM_REGION( 0x100000, REGION_SOUND2, 0 ) /* OKI Samples */
    ROM_LOAD( "1_semicom", 0x000000, 0x80000, CRC(fa3b6890) SHA1(7534931c96d6fa05fee840a7ea07b87e2e2acc50) )

    ROM_REGION( 0x800000, REGION_GFX1, 0 ) /* Sprite Tiles - decoded */
    ROM_LOAD16_WORD_SWAP( "10_semicom", 0x000000, 0x200000, CRC(62bf1e6e) SHA1(fb4b0db313e26687f0ebc6a8505a02e5348776da) )
    ROM_LOAD16_WORD_SWAP( "11_semicom", 0x200000, 0x200000, CRC(796f23a7) SHA1(adaa4c8525de428599f4489ecc8e966fed0d514d) )
    ROM_LOAD16_WORD_SWAP( "13_semicom", 0x400000, 0x200000, CRC(24222b3c) SHA1(08163863890c01728db89b8f4447841ecb4f4f62) )
    ROM_LOAD16_WORD_SWAP( "14_semicom", 0x600000, 0x200000, CRC(385b69d7) SHA1(8e7cae5589e354bea0b77b061af1d0c81d796f7c) )

    ROM_REGION( 0x200000, REGION_GFX2, 0 ) /* BG Tiles - decoded */
    ROM_LOAD16_WORD_SWAP( "12_semicom",0x000000, 0x200000, CRC(45d29c22) SHA1(df719a061dcd14fb4388fb45dfee2054e56a1299) )

    ROM_REGION( 0x040000, REGION_GFX3, 0 ) /* Sprite Code Lookup ... */
    ROM_LOAD16_BYTE( "7_semicom", 0x000000, 0x020000, CRC(39454102) SHA1(347e9242fd7e2092cfaacdce92691cf6024471ac) )
    ROM_LOAD16_BYTE( "8_semicom", 0x000001, 0x020000, CRC(fccb1b13) SHA1(fd4aec4a660f9913651fcc084e3f13eb0adbddd6) )

    ROM_REGION( 0x10000, REGION_GFX4, 0 ) /* ???? - not decoded seems to be in blocks of 0x41 bytes.. */
    ROM_LOAD( "9_semicom", 0x000000, 0x10000, CRC(0da8db45) SHA1(7d5bd71c5b0b28ff74c732edd7c662f46f2ab25b) )
ROM_END

ROM_START( dreamwld )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD32_BYTE( "1.bin", 0x000002, 0x040000, CRC(35c94ee5) SHA1(3440a65a807622b619c97bc2a88fd7d875c26f66) )
	ROM_LOAD32_BYTE( "2.bin", 0x000003, 0x040000, CRC(5409e7fc) SHA1(2f94a6a8e4c94b36b43f0b94d58525f594339a9d) )
	ROM_LOAD32_BYTE( "3.bin", 0x000000, 0x040000, CRC(e8f7ae78) SHA1(cfd393cec6dec967c82e1131547b7e7fdc5d814f) )
	ROM_LOAD32_BYTE( "4.bin", 0x000001, 0x040000, CRC(3ef5d51b) SHA1(82a00b4ff7155f6d5553870dfd510fed9469d9b5) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 87C52 MCU Code */
	/*ROM_LOAD( "87c52.mcu", 0x00000, 0x10000 , NO_DUMP )  // can't be dumped. /*/

	ROM_REGION( 0x6c9, REGION_USER1, 0 ) /* Protection data  */
	/* The MCU supplies this data.
      The 68k reads it through a port, taking the size and destination write address from the level 1
      and level 2 irq positions in the 68k vector table (there is code to check that they haven't been
      modofied!)  It then decodes the data using the rom checksum previously calculated and puts it in
      ram.  The interrupt vectors point at the code placed in RAM. */
	ROM_LOAD( "protdata.bin", 0x000, 0x6c9 ,  CRC(f284b2fd) SHA1(9e8096c8aa8a288683f002311b38787b120748d1) ) /* extracted */

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* OKI Samples - 1st chip*/
	ROM_LOAD( "5.bin", 0x000000, 0x80000, CRC(9689570a) SHA1(4414233da8f46214ca7e9022df70953922a63aa4) )
	ROM_RELOAD(0x80000,0x80000) /* for the banks*/

	ROM_REGION( 0x100000, REGION_SOUND2, 0 ) /* OKI Samples - 2nd chip*/
	ROM_LOAD( "6.bin", 0x000000, 0x80000, CRC(c8b91f30) SHA1(706004ca56d0a74bc7a3dfd73a21cdc09eb90f05) )
	ROM_RELOAD(0x80000,0x80000) /* for the banks*/

	ROM_REGION( 0x400000, REGION_GFX1, 0 ) /* Sprite Tiles - decoded */
	ROM_LOAD16_WORD_SWAP( "9.bin", 0x000000, 0x200000, CRC(fa84e3af) SHA1(5978737d348fd382f4ec004d29870656c864d137) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 ) /* BG Tiles - decoded */
	ROM_LOAD16_WORD_SWAP( "10.bin",0x000000, 0x200000, CRC(3553e4f5) SHA1(c335494f4a12a01a88e7cd578cae922954303cfd) )

	ROM_REGION( 0x040000, REGION_GFX3, 0 ) /* Sprite Code Lookup ... */
	ROM_LOAD16_BYTE( "8.bin", 0x000000, 0x020000, CRC(8d570df6) SHA1(e53e4b099c64eca11d027e0083caa101fcd99959) )
	ROM_LOAD16_BYTE( "7.bin", 0x000001, 0x020000, CRC(a68bf35f) SHA1(f48540a5415a7d9723ca6e7e03cab039751dce17) )

	ROM_REGION( 0x10000, REGION_GFX4, 0 ) /* ???? - not decoded seems to be in blocks of 0x41 bytes.. */
	ROM_LOAD( "11.bin", 0x000000, 0x10000, CRC(0da8db45) SHA1(7d5bd71c5b0b28ff74c732edd7c662f46f2ab25b) )
ROM_END

ROM_START( gaialast )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD32_BYTE( "4", 0x000000, 0x040000, CRC(10cc2dee) SHA1(0719333ff391ee7935c6e4cea7e8e75369aeb9d0) )
	ROM_LOAD32_BYTE( "5", 0x000001, 0x040000, CRC(c55f6f11) SHA1(13d543b0770bebdd4c6e064b56fd6cc2ec929566) )
	ROM_LOAD32_BYTE( "2", 0x000002, 0x040000, CRC(549e594a) SHA1(728c6b51cc478ad7251bcbe6d7f4f4e6a2ee4a4e) )
	ROM_LOAD32_BYTE( "3", 0x000003, 0x040000, CRC(a8e845d8) SHA1(f8c7e702bd747a22e76c861effec4cd3cd2f3fc9) )
	
	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 87C52 MCU Code */
	/*ROM_LOAD( "87c52.mcu", 0x00000, 0x10000 , NO_DUMP )  // can't be dumped. /*/
	
	ROM_REGION( 0x6c9, REGION_USER1, ROMREGION_ERASEFF ) /* Protection data */
	ROM_LOAD( "protdata.bin", 0x000, 0x6c9 , CRC(d3403b7b) SHA1(712a7f27fc41b632d584237f7641e8ae20035111) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* OKI Samples - 1st chip*/
	ROM_LOAD( "1", 0x000000, 0x80000, CRC(2dbad410) SHA1(bb788ea14bb605be9af9c8f8adec94ad1c17ab55))

	ROM_REGION( 0x80000, REGION_SOUND2, ROMREGION_ERASE00 ) /* OKI Samples - 2nd chip (neither OKI or rom is present, empty sockets) */
	
	ROM_REGION( 0x800000, REGION_GFX1, 0 ) /* Sprite Tiles - decoded */
	ROM_LOAD16_WORD_SWAP( "10", 0x000000, 0x200000, CRC(5822ef93) SHA1(8ce22c30f8027f35c5f72eb6ce57a74540dd55da) )
	ROM_LOAD16_WORD_SWAP( "11", 0x200000, 0x200000, CRC(f4f5770d) SHA1(ac850483cae321d286a09fe93ce7e49725722de0) )
	ROM_LOAD16_WORD_SWAP( "12", 0x400000, 0x200000, CRC(a1f04571) SHA1(c29b3b3c209b63ad44ebfa5afb4b1832965e0936) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 ) /* BG Tiles - decoded */
	ROM_LOAD16_WORD_SWAP( "8",0x000000, 0x200000, CRC(32d16985) SHA1(2b7a20eea09e7d2debd42469e9f6ae49310f5747) )

    ROM_REGION( 0x040000, REGION_GFX3, 0 ) /* Sprite Code Lookup ... */
	ROM_LOAD16_BYTE( "6", 0x000000, 0x020000, CRC(5c82feed) SHA1(1857afecf1081adf015ade1efb5930e3a7deef78) )
	ROM_LOAD16_BYTE( "7", 0x000001, 0x020000, CRC(9d7f04ae) SHA1(55fb82626060fe0ddc03ed3ef402ccf998063d27) )

    ROM_REGION( 0x10000, REGION_GFX4, 0 ) /* ???? - not decoded seems to be in blocks of 0x41 bytes.. */
    ROM_LOAD( "9_semicom", 0x000000, 0x10000, CRC(0da8db45) SHA1(7d5bd71c5b0b28ff74c732edd7c662f46f2ab25b) )
ROM_END

ROM_START( rolcrush )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD32_BYTE( "mx27c2000_4.bin", 0x000000, 0x040000, CRC(c47f0540) SHA1(76712f41046e5852ad6be6dbf171cf34471e2409) )
	ROM_LOAD32_BYTE( "mx27c2000_3.bin", 0x000001, 0x040000, CRC(7af59294) SHA1(f36b3d100e0d963bf51b7fbe8c4a0bdcf2180ba0) )
	ROM_LOAD32_BYTE( "mx27c2000_2.bin", 0x000002, 0x040000, CRC(5eb24adb) SHA1(0329a02e18490bfe72ff34a64722d7316814720b) )
	ROM_LOAD32_BYTE( "mx27c2000_1.bin", 0x000003, 0x040000, CRC(a37e15b2) SHA1(f0fc945a894d6ed58daf05390a17051d0f3cda20) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 87C52 MCU Code */
	/*ROM_LOAD( "87c52.mcu", 0x00000, 0x10000 , NO_DUMP )  // can't be dumped. /*/

	ROM_REGION( 0x745, REGION_USER1, ROMREGION_ERASE00 ) /* Protection data  */
    ROM_LOAD( "protdata.bin", 0x000, 0x745, CRC(06b8a880) SHA1(b7d4bf26d34cb544825270c2c474bbd4c81a6c9e) ) /* extracted */

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* OKI Samples - 1st chip*/
	ROM_LOAD( "mx27c4000_5.bin", 0x000000, 0x80000, CRC(7afa6adb) SHA1(d4049e1068a5f7abf0e14d0b9fbbbc6dfb5d0170) )

	ROM_REGION( 0x80000, REGION_SOUND2, ROMREGION_ERASE00 ) /* OKI Samples - 2nd chip (neither OKI or rom is present, empty sockets) */

	ROM_REGION( 0x400000, REGION_GFX1, 0 ) /* Sprite Tiles - decoded */
	ROM_LOAD16_WORD_SWAP( "m27c160.8.bin", 0x000000, 0x200000, CRC(a509bc36) SHA1(aaa008e07e4b24ff9dbcee5925d6516d1662931c) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 ) /* BG Tiles - decoded */
	ROM_LOAD16_WORD_SWAP( "m27c160.10.bin",0x000000, 0x200000, CRC(739b0cb0) SHA1(a7cc48502d84218586afa7276fa7ba759242f05e) )

	ROM_REGION( 0x040000, REGION_GFX3, 0 ) /* Sprite Code Lookup ... */
	ROM_LOAD16_BYTE( "tms27c010_7.bin", 0x000000, 0x020000, CRC(4cb84384) SHA1(8dd02e2d9829c15cb19654779d2217a7d53d5971) )
	ROM_LOAD16_BYTE( "tms27c010_6.bin", 0x000001, 0x020000, CRC(0c9d197a) SHA1(da057c8d08f41c4a5b9cb4f8f00de7e1461d98f0) )

	ROM_REGION( 0x10000, REGION_GFX4, 0 ) /* ???? - not decoded seems to be in blocks of 0x41 bytes.. */
	ROM_LOAD( "mx27c512.9.bin", 0x000000, 0x10000, CRC(0da8db45) SHA1(7d5bd71c5b0b28ff74c732edd7c662f46f2ab25b) )
ROM_END


GAME( 1997, baryon,   0, dreamwld,  baryon,   0, ROT270,  "SemiCom / Tirano", "Baryon - Future Assault" )
GAME( 1998, cutefght, 0, dreamwld,  cutefght, 0, ROT0,    "SemiCom",          "Cute Fighter" )
GAME( 2000, dreamwld, 0, dreamwld,  dreamwld, 0, ROT0,    "SemiCom",          "Dream World" )
GAME( 1999, gaialast, 0, dreamwld,  gaialast, 0, ROT0,    "SemiCom / XESS",   "Gaia - The Last Choice of Earth" )
GAME( 1999, rolcrush, 0, dreamwld,  rolcrush, 0, ROT0,    "SemiCom / Exit",   "Rolling Crush (version 1.07.E - 1999/02/11)" )
