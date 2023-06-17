#define VERBOSE 0

/*
 * vidhrdw/konamigx.c - Konami GX video hardware (here there be dragons)
 *
 */

#include "driver.h"
#include "state.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/konamiic.h"
#include "machine/konamigx.h"

static int layer_colorbase[4];
static int gx_tilebanks[8], gx_oldbanks[8];
static int gx_invertlayersBC;
int gx_le2_textcolour_hack;
static int gx_tilemode, gx_rozenable, psac_colorbase, last_psac_colorbase;
static struct tilemap *gx_psac_tilemap, *gx_psac_tilemap2;
extern data32_t *gx_psacram, *gx_subpaletteram32;

static void (*game_tile_callback)(int, int *, int *);

static void get_gx_psac_tile_info(int tile_index)
{
	int tileno, colour, flipx;
	data16_t *map16 = (data16_t *)gx_psacram;

	tileno = map16[tile_index*2+1] & 0x3fff;
	colour = (psac_colorbase << 4);
	flipx = 0;

	SET_TILE_INFO(0, tileno, colour, TILE_FLIPYX(flipx))
}

/* Soccer Superstars (tile and flip bits now TRUSTED) */
static void get_gx_psac3_tile_info(int tile_index)
{
	int tileno, colour, flip;
	unsigned char *tmap = memory_region(REGION_GFX4);

	tileno = tmap[tile_index*2] | ((tmap[(tile_index*2)+1] & 0x0f)<<8);
	colour = (psac_colorbase << 8);

	flip = 0;
	if (tmap[(tile_index*2)+1] & 0x20) flip |= TILE_FLIPX;
	if (tmap[(tile_index*2)+1] & 0x10) flip |= TILE_FLIPY;

	SET_TILE_INFO(0, tileno, colour, flip)
}

static void get_gx_psac1a_tile_info(int tile_index)
{
	int tileno, colour, flip;
	data8_t *map = (data8_t *)&gx_psacram[tile_index*2];	/* *8 / 4 (gx_psacram is data32_t) */

	/* this should be &0x7f for opengolf, except that makes the tilemaps unrecognizable.  huh? */
	tileno = (map[1]&0x3f)<<8 | map[0];

	colour = 0; /* (psac_colorbase << 4); */

	flip = 0;
	if (map[7] & 0x80) flip |= TILE_FLIPX;
	if (map[7] & 0x40) flip |= TILE_FLIPY;

	SET_TILE_INFO(1, tileno, colour, flip)
}		 

static void get_gx_psac1b_tile_info(int tile_index)
{
	int tileno, colour, flip;
	data8_t *map = (data8_t *)&gx_psacram[tile_index*2];

	tileno = (map[5]&0x3f)<<8 | map[4];

	colour = 0; /* (psac_colorbase << 4); */

	flip = 0;
	if (map[7] & 0x20) flip |= TILE_FLIPX;
	if (map[7] & 0x10) flip |= TILE_FLIPY;

	SET_TILE_INFO(0, tileno, colour, flip)
}

static void konamigx_type2_tile_callback(int layer, int *code, int *color)
{
	int d = *code;

	*code = (gx_tilebanks[(d & 0xe000)>>13]<<13) + (d & 0x1fff);
	K055555GX_decode_vmixcolor(layer, color);
}

static void konamigx_alpha_tile_callback(int layer, int *code, int *color)
{
	int mixcode;
	int d = *code;

	mixcode = K055555GX_decode_vmixcolor(layer, color);

	if (mixcode < 0)
		*code = (gx_tilebanks[(d & 0xe000)>>13]<<13) + (d & 0x1fff);
	else
	{
		/* save mixcode and mark tile alpha (unimplemented) */
		*code = 0;

		log_cb(RETRO_LOG_DEBUG, LOGPRE "skipped alpha tile(layer=%d mix=%d)", layer, mixcode);
	}
}

/*
> bits 8-13 are the low priority bits
> i.e. pri 0-5
> pri 6-7 can be either 1, bits 14,15 or bits 16,17
> contro.bit 2 being 0 forces the 1
> when control.bit 2 is 1, control.bit 3 selects between the two
> 0 selects 16,17
> that gives you the entire 8 bits of the sprite priority
> ok, lemme see if I've got this.  bit2 = 0 means the top bits are 11, bit2=1 means the top bits are bits 14/15 (of the whatever word?) else
+16+17?
> bit3=1 for the second

 *   6  | ---------xxxxxxx | "color", but depends on external connections


> there are 8 color lines entering the 5x5
> that means the palette is 4 bits, not 5 as you currently have
> the bits 4-9 are the low priority bits
> bits 10/11 or 12/13 are the two high priority bits, depending on the control word
> and bits 14/15 are the shadow bits
> mix0/1 and brit0/1 come from elsewhere
> they come from the '673 all right, but not from word 6
> and in fact the top address bits are highly suspect
> only 18 of the address bits go to the roms
> the next 2 go to cai0/1 and the next 4 to bk0-3
> (the '246 indexes the roms, the '673 reads the result)
> the roms are 64 bits wide
> so, well, the top bits of the code are suspicious
*/

static int _gxcommoninitnosprites(void)
{
	int i;

	K054338_vh_start();
	K055555_vh_start();

	if (konamigx_mixer_init(0)) return 1;

	for (i = 0; i < 8; i++)
	{
		gx_tilebanks[i] = gx_oldbanks[i] = 0;
	}

	state_save_register_INT32("KGXVideo", 0, "tilebanks", gx_tilebanks, 8);

	gx_invertlayersBC = 0;
	gx_tilemode = 0;
	gx_le2_textcolour_hack = 0;

	/* Documented relative offsets of non-flipped games are (-2, 0, 2, 3),(0, 0, 0, 0).*/
	/* (+ve values move layers to the right and -ve values move layers to the left)*/
	/* In most cases only a constant is needed to add to the X offsets to yield correct*/
	/* displacement. This should be done by the CCU but the CRT timings have not been*/
	/* figured out.*/
	K056832_set_LayerOffset(0, -2, 0);
	K056832_set_LayerOffset(1,  0, 0);
	K056832_set_LayerOffset(2,  2, 0);
	K056832_set_LayerOffset(3,  3, 0);

	return 0;
}

static int _gxcommoninit(void)
{
	/* (+ve values move objects to the right and -ve values move objects to the left)*/
	if (K055673_vh_start(REGION_GFX2, K055673_LAYOUT_GX, -26, -23, konamigx_type2_sprite_callback))
	{
		return 1;
	}

	gx_rozenable = 0;

	return _gxcommoninitnosprites();
}


VIDEO_START(konamigx_5bpp)
{
	if (!strcmp(Machine->gamedrv->name,"sexyparo") || !strcmp(Machine->gamedrv->name,"sexyparoa"))
		game_tile_callback = konamigx_alpha_tile_callback;
	else
		game_tile_callback = konamigx_type2_tile_callback;

	if (K056832_vh_start(REGION_GFX1, K056832_BPP_5, 0, NULL, game_tile_callback, 0))
	{
		return 1;
	}

	if (_gxcommoninit()) return 1;

	/* here are some hand tuned per game scroll offsets to go with the per game visible areas,
	   i see no better way of doing this for now... */

	if (!strcmp(Machine->gamedrv->name,"tbyahhoo"))
	{
		K056832_set_UpdateMode(1);
		gx_tilemode = 1;
	} else

	if (!strcmp(Machine->gamedrv->name,"puzldama"))
	{
		K053247GP_set_SpriteOffset(-46, -23);
		konamigx_mixer_primode(5);
	} else

	if (!strcmp(Machine->gamedrv->name,"daiskiss"))
	{
		konamigx_mixer_primode(4);
	} else

	if (!strcmp(Machine->gamedrv->name,"gokuparo") || !strcmp(Machine->gamedrv->name,"fantjour"))
 	{
		K053247GP_set_SpriteOffset(-46, -23);
	} else

	if (!strcmp(Machine->gamedrv->name,"sexyparo") || !strcmp(Machine->gamedrv->name,"sexyparoa"))
	{
		K053247GP_set_SpriteOffset(-42, -23);
	}

	return 0;
}

VIDEO_START(winspike)
{
	if (K056832_vh_start(REGION_GFX1, K056832_BPP_8, 0, NULL, konamigx_alpha_tile_callback, 0))
	{
		return 1;
	}

	if (K055673_vh_start(REGION_GFX2, K055673_LAYOUT_LE2, -53, -23, konamigx_type2_sprite_callback))
	{
		return 1;
	}

	if (_gxcommoninitnosprites()) return 1;

	return 0;
}

VIDEO_START(dragoonj)
{
	if (K056832_vh_start(REGION_GFX1, K056832_BPP_5, 1, NULL, konamigx_type2_tile_callback, 0))
	{
		return 1;
	}

	if (K055673_vh_start(REGION_GFX2, K055673_LAYOUT_RNG, -53, -23, konamigx_dragoonj_sprite_callback))
	{
		return 1;
	}

	if (_gxcommoninitnosprites()) return 1;

	K056832_set_LayerOffset(0, -2+1, 0);
	K056832_set_LayerOffset(1,  0+1, 0);
	K056832_set_LayerOffset(2,  2+1, 0);
	K056832_set_LayerOffset(3,  3+1, 0);

	return 0;
}

VIDEO_START(le2)
{
	if (K056832_vh_start(REGION_GFX1, K056832_BPP_8, 1, NULL, konamigx_type2_tile_callback, 0))
	{
		return 1;
	}

	if (K055673_vh_start(REGION_GFX2, K055673_LAYOUT_LE2, -46, -23, konamigx_le2_sprite_callback))
	{
		return 1;
	}

	if (_gxcommoninitnosprites()) return 1;

	gx_invertlayersBC = 1;
	konamigx_mixer_primode(-1); /* swapped layer B and C priorities?*/

	gx_le2_textcolour_hack = 1; /* force text layer to use the right palette */

	return 0;
}

VIDEO_START(konamigx_6bpp)
{
	if (K056832_vh_start(REGION_GFX1, K056832_BPP_6, 0, NULL, konamigx_type2_tile_callback, 0))
	{
		return 1;
	}

	if (_gxcommoninit()) return 1;

	if (!strcmp(Machine->gamedrv->name,"tokkae") || !strcmp(Machine->gamedrv->name,"tkmmpzdm"))
	{
		K053247GP_set_SpriteOffset(-46, -23);
		konamigx_mixer_primode(5);
	}

	return 0;
}

VIDEO_START(konamigx_type3)
{
	if (K056832_vh_start(REGION_GFX1, K056832_BPP_6, 0, NULL, konamigx_type2_tile_callback, 0))
	{
		return 1;
	}

	if (_gxcommoninit()) return 1;

	gx_psac_tilemap = tilemap_create(get_gx_psac3_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 16, 16, 256, 1024);
	gx_rozenable = 1;

	K053936_wraparound_enable(0, 1);
	K053936GP_set_offset(0, 0, 0);

	return 0;
}

VIDEO_START(konamigx_type4)
{
	if (K056832_vh_start(REGION_GFX1, K056832_BPP_8, 0, NULL, konamigx_type2_tile_callback, 0))
	{
		return 1;
	}

	if (_gxcommoninit()) return 1;

	gx_psac_tilemap = tilemap_create(get_gx_psac_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 16, 16, 128, 128);
	gx_rozenable = 1;

	K053936_wraparound_enable(0, 0);
	K053936GP_set_offset(0, 0, 0);

	return 0;
}

VIDEO_START(konamigx_6bpp_2)
{
	if (K056832_vh_start(REGION_GFX1, K056832_BPP_6, 1, NULL, konamigx_type2_tile_callback, 0))
	{
		return 1;
	}

	if (!strcmp(Machine->gamedrv->name,"salmndr2"))
	{
		if (K055673_vh_start(REGION_GFX2, K055673_LAYOUT_GX6, -48, -23, konamigx_salmndr2_sprite_callback))
		{
			return 1;
		}

		if (_gxcommoninitnosprites()) return 1;
	}
	else
	{
		if (_gxcommoninit()) return 1;
	}

	return 0;
}

VIDEO_START(opengolf)
{
	if (K056832_vh_start(REGION_GFX1, K056832_BPP_5, 0, NULL, konamigx_type2_tile_callback, 0))
	{
		return 1;
	}

	if (K055673_vh_start(REGION_GFX2, K055673_LAYOUT_GX6, -53, -23, konamigx_type2_sprite_callback))
	{
		return 1;
	}

	if (_gxcommoninitnosprites()) return 1;

	K056832_set_LayerOffset(0, -2+1, 0);
	K056832_set_LayerOffset(1,  0+1, 0);
	K056832_set_LayerOffset(2,  2+1, 0);
	K056832_set_LayerOffset(3,  3+1, 0);

	gx_psac_tilemap = tilemap_create(get_gx_psac1a_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 16, 16, 128, 128);
	gx_psac_tilemap2 = tilemap_create(get_gx_psac1b_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 16, 16, 128, 128);
	tilemap_set_transparent_pen(gx_psac_tilemap, 0);
	tilemap_set_transparent_pen(gx_psac_tilemap2, 0);

	gx_rozenable = 0;

	return 0;
}

VIDEO_START(racinfrc)
{
	if (K056832_vh_start(REGION_GFX1, K056832_BPP_6, 0, NULL, konamigx_type2_tile_callback, 0))
	{
		return 1;
	}

	if (K055673_vh_start(REGION_GFX2, K055673_LAYOUT_GX, -53, -23, konamigx_type2_sprite_callback))
	{
		return 1;
	}

	if (_gxcommoninitnosprites()) return 1;

	K056832_set_LayerOffset(0, -2+1, 0);
	K056832_set_LayerOffset(1,  0+1, 0);
	K056832_set_LayerOffset(2,  2+1, 0);
	K056832_set_LayerOffset(3,  3+1, 0);

	gx_psac_tilemap = tilemap_create(get_gx_psac1a_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 16, 16, 128, 128);
	gx_psac_tilemap2 = tilemap_create(get_gx_psac1b_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 16, 16, 128, 128);
	tilemap_set_transparent_pen(gx_psac_tilemap, 0);
	tilemap_set_transparent_pen(gx_psac_tilemap2, 0);

	gx_rozenable = 0;

	return 0;
}

VIDEO_UPDATE(konamigx)
{
	int i, newbank, newbase, dirty, unchained;

	/* if any banks are different from last render, we need to flush the planes */
	for (dirty = 0, i = 0; i < 8; i++)
	{
		newbank = gx_tilebanks[i];
		if (gx_oldbanks[i] != newbank) { gx_oldbanks[i] = newbank; dirty = 1; }
	}

	if (gx_tilemode == 0)
	{
		/* driver approximates tile update in mode 0 for speed*/
		unchained = K056832_get_LayerAssociation();
		for (i=0; i<4; i++)
		{
			newbase = K055555_get_palette_index(i)<<6;
			if (layer_colorbase[i] != newbase)
			{
				layer_colorbase[i] = newbase;

				if (unchained)
					K056832_mark_plane_dirty(i);
				else
					dirty = 1;
			}
		}
	}
	else
	{
		/* K056832 does all the tracking in mode 1 for accuracy (Twinbee needs this)*/
	}

	/* sub2 is PSAC colorbase on GX*/
	if (gx_rozenable)
	{
		last_psac_colorbase = psac_colorbase;
		psac_colorbase = K055555_get_palette_index(6);

		if (psac_colorbase != last_psac_colorbase)
		{
			tilemap_mark_all_tiles_dirty(gx_psac_tilemap);
			if (gx_rozenable == 3)
			{
				tilemap_mark_all_tiles_dirty(gx_psac_tilemap2);
			}
		}
	}

	if (dirty) K056832_MarkAllTilemapsDirty();

	if (gx_rozenable)
		konamigx_mixer(bitmap, cliprect, 0, 0, gx_psac_tilemap, GXSUB_8BPP, 0);
	else
		konamigx_mixer(bitmap, cliprect, 0, 0, 0, 0, 0);

	if( gx_invertlayersBC )
	{
		draw_crosshair( 1, bitmap, readinputport( 9)*287/0xff+24, readinputport(10)*223/0xff+16, cliprect );
		draw_crosshair( 2, bitmap, readinputport(11)*287/0xff+24, readinputport(12)*223/0xff+16, cliprect );
	}
}


WRITE32_HANDLER( konamigx_palette_w )
{
	int r,g,b;

	COMBINE_DATA(&paletteram32[offset]);

 	r = (paletteram32[offset] >>16) & 0xff;
	g = (paletteram32[offset] >> 8) & 0xff;
	b = (paletteram32[offset] >> 0) & 0xff;

	palette_set_color(offset,r,g,b);
}

WRITE32_HANDLER( konamigx_palette2_w )
{
	int r,g,b;

	COMBINE_DATA(&gx_subpaletteram32[offset]);

	offset += (0x8000/4);

	COMBINE_DATA(&paletteram32[offset]);

 	r = (paletteram32[offset] >>16) & 0xff;
	g = (paletteram32[offset] >> 8) & 0xff;
	b = (paletteram32[offset] >> 0) & 0xff;

	palette_set_color(offset,r,g,b);
}

/* main monitor for type 3*/
WRITE32_HANDLER( konamigx_555_palette_w )
{
	COMBINE_DATA(&paletteram32[offset]);

	paletteram16 = (data16_t *)paletteram32;
	if (ACCESSING_MSW32)
		paletteram16_xRRRRRGGGGGBBBBB_word_w(offset*2, data >> 16, mem_mask >> 16);
	if (ACCESSING_LSW32)
		paletteram16_xRRRRRGGGGGBBBBB_word_w(offset*2+1, data, mem_mask);
}

/* sub monitor for type 3*/
WRITE32_HANDLER( konamigx_555_palette2_w )
{
	COMBINE_DATA(&gx_subpaletteram32[offset]);

	offset += (0x4000/4);

	COMBINE_DATA(&paletteram32[offset]);
	
	paletteram16 = (data16_t *)paletteram32;
	if (ACCESSING_MSW32)
		paletteram16_xRRRRRGGGGGBBBBB_word_w(offset*2, data >> 16, mem_mask >> 16);
	if (ACCESSING_LSW32)
		paletteram16_xRRRRRGGGGGBBBBB_word_w(offset*2+1, data, mem_mask);
}

WRITE32_HANDLER( konamigx_tilebank_w )
{
	if (!(mem_mask & 0xff000000))
		gx_tilebanks[offset*4] = (data>>24)&0xff;
	if (!(mem_mask & 0xff0000))
		gx_tilebanks[offset*4+1] = (data>>16)&0xff;
	if (!(mem_mask & 0xff00))
		gx_tilebanks[offset*4+2] = (data>>8)&0xff;
	if (!(mem_mask & 0xff))
		gx_tilebanks[offset*4+3] = data&0xff;
}

/* type 1 RAM-based PSAC tilemap*/
WRITE32_HANDLER(konamigx_t1_psacmap_w)
{
	COMBINE_DATA(&gx_psacram[offset]);
	tilemap_mark_tile_dirty(gx_psac_tilemap, offset/2);
	tilemap_mark_tile_dirty(gx_psac_tilemap2, offset/2);
}

/* type 4 RAM-based PSAC tilemap*/
WRITE32_HANDLER( konamigx_t4_psacmap_w )
{
	COMBINE_DATA(&gx_psacram[offset]);

	tilemap_mark_tile_dirty(gx_psac_tilemap, offset);
}
