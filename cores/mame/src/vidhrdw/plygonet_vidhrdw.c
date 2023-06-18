/*
	Polygonet Commanders (Konami, 1993)

	Video hardware emulation

	Currently contains: TTL text plane (probably not complete, needs banking? colors?)
	Needs: PSAC2 roz plane, polygons
*/

#include "driver.h"
#include "state.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/konamiic.h"

/* TTL text plane stuff */

static int ttl_gfx_index;
static struct tilemap *ttl_tilemap;
static data16_t ttl_vram[0x800];

/* TTL text plane */

static void ttl_get_tile_info(int tile_index)
{
	int attr, code;

	code = ttl_vram[tile_index]&0xff;
	attr = 0;

	tile_info.flags = 0;

	SET_TILE_INFO(ttl_gfx_index, code, attr, tile_info.flags);
}

static UINT32 ttl_scan(UINT32 col,UINT32 row,UINT32 num_cols,UINT32 num_rows)
{
	/* logical (col,row) -> memory offset */

	return row*64 + col;
}

READ32_HANDLER( polygonet_ttl_ram_r )
{
	data32_t *vram = (data32_t *)ttl_vram;

	return(vram[offset]);
}

WRITE32_HANDLER( polygonet_ttl_ram_w )
{
	data32_t *vram = (data32_t *)ttl_vram;

	COMBINE_DATA(&vram[offset]);

	tilemap_mark_tile_dirty(ttl_tilemap, offset*2);
	tilemap_mark_tile_dirty(ttl_tilemap, offset*2+1);
}

VIDEO_START(polygonet_vh_start)
{
	static struct GfxLayout charlayout =
	{
		8, 8,		/* 8x8*/
		4096,		/* # of tiles*/
		4,	   	/* 4bpp*/
		{ 0, 1, 2, 3 },	/* plane offsets*/
		{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },		/* X offsets*/
		{ 0*8*4, 1*8*4, 2*8*4, 3*8*4, 4*8*4, 5*8*4, 6*8*4, 7*8*4 },	/* Y offsets*/
		8*8*4
	};

	/* find first empty slot to decode gfx */
	for (ttl_gfx_index = 0; ttl_gfx_index < MAX_GFX_ELEMENTS; ttl_gfx_index++)
		if (Machine->gfx[ttl_gfx_index] == 0)
			break;

	if (ttl_gfx_index == MAX_GFX_ELEMENTS)
		return 1;

	/* decode the ttl layer's gfx*/
	Machine->gfx[ttl_gfx_index] = decodegfx(memory_region(REGION_GFX1), &charlayout);

	if (Machine->drv->color_table_len)
	{
	        Machine->gfx[ttl_gfx_index]->colortable = Machine->remapped_colortable;
	        Machine->gfx[ttl_gfx_index]->total_colors = Machine->drv->color_table_len / 16;
	}
	else
	{
	        Machine->gfx[ttl_gfx_index]->colortable = Machine->pens;
	        Machine->gfx[ttl_gfx_index]->total_colors = Machine->drv->total_colors / 16;
	}

	/* create the tilemap*/
	ttl_tilemap = tilemap_create(ttl_get_tile_info, ttl_scan, TILEMAP_TRANSPARENT, 8, 8, 64, 32);

	tilemap_set_transparent_pen(ttl_tilemap, 0);

	state_save_register_UINT16("PolygonetTTL", 0, "VRAM", ttl_vram, 0x800);

	return 0;
}

VIDEO_UPDATE(polygonet_vh_screenrefresh)
{
	fillbitmap(priority_bitmap, 0, NULL);
	fillbitmap(bitmap, get_black_pen(), &Machine->visible_area);

	tilemap_draw(bitmap, cliprect, ttl_tilemap, 0, 1<<0);
}
