#include "driver.h"
#include "vidhrdw/konamiic.h"

static int layer_colorbase[3],sprite_colorbase,bg_colorbase;
static int layerpri[3];

extern data16_t xmen_current_frame;
extern data16_t *K053247_ram;
extern data16_t*xmen6p_spriteramleft;
extern data16_t*xmen6p_spriteramright;
extern data16_t*xmen6p_tilemapleft;
extern data16_t*xmen6p_tilemapright;
extern data16_t*xmen6p_tilemapleftalt;
extern data16_t*xmen6p_tilemaprightalt;
extern int xmen6p_tilemap_select;
extern WRITE_HANDLER( K052109_w );

struct mame_bitmap * screen_left;
struct mame_bitmap * screen_right;

/***************************************************************************

  Callbacks for the K052109

***************************************************************************/

static void xmen_tile_callback(int layer,int bank,int *code,int *color)
{
	/* (color & 0x02) is flip y handled internally by the 052109 */
	if (layer == 0)
		*color = layer_colorbase[layer] + ((*color & 0xf0) >> 4);
	else
		*color = layer_colorbase[layer] + ((*color & 0x7c) >> 2);
}

/***************************************************************************

  Callbacks for the K053247

***************************************************************************/

static void xmen_sprite_callback(int *code,int *color,int *priority_mask)
{
	int pri = (*color & 0x00e0) >> 4;	/* ??????? */
	if (pri <= layerpri[2])								*priority_mask = 0;
	else if (pri > layerpri[2] && pri <= layerpri[1])	*priority_mask = 0xf0;
	else if (pri > layerpri[1] && pri <= layerpri[0])	*priority_mask = 0xf0|0xcc;
	else 												*priority_mask = 0xf0|0xcc|0xaa;

	*color = sprite_colorbase + (*color & 0x001f);
}

/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

VIDEO_START( xmen )
{
	K053251_vh_start();

	if (K052109_vh_start(REGION_GFX1,NORMAL_PLANE_ORDER,xmen_tile_callback))
		return 1;
	if (K053247_vh_start(REGION_GFX2,53,-2,NORMAL_PLANE_ORDER,xmen_sprite_callback))
		return 1;
	return 0;
}

VIDEO_START( xmen6p )
{
	K053251_vh_start();

	if (K052109_vh_start(REGION_GFX1,NORMAL_PLANE_ORDER,xmen_tile_callback))
		return 1;
	if (K053247_vh_start(REGION_GFX2,53,-2,NORMAL_PLANE_ORDER,xmen_sprite_callback))
		return 1;

	screen_right = auto_bitmap_alloc_depth(128*8, 32*8, 16);
	screen_left = auto_bitmap_alloc_depth(128*8, 32*8, 16);

	return 0;
}

/***************************************************************************

  Display refresh

***************************************************************************/

/* useful function to sort the three tile layers by priority order */
static void sortlayers(int *layer,int *pri)
{
#define SWAP(a,b) \
	if (pri[a] < pri[b]) \
	{ \
		int t; \
		t = pri[a]; pri[a] = pri[b]; pri[b] = t; \
		t = layer[a]; layer[a] = layer[b]; layer[b] = t; \
	}

	SWAP(0,1)
	SWAP(0,2)
	SWAP(1,2)
}

VIDEO_UPDATE( xmen )
{
	int layer[3];


	bg_colorbase       = K053251_get_palette_index(K053251_CI4);
	sprite_colorbase   = K053251_get_palette_index(K053251_CI1);
	layer_colorbase[0] = K053251_get_palette_index(K053251_CI3);
	layer_colorbase[1] = K053251_get_palette_index(K053251_CI0);
	layer_colorbase[2] = K053251_get_palette_index(K053251_CI2);

	K052109_tilemap_update();

	layer[0] = 0;
	layerpri[0] = K053251_get_priority(K053251_CI3);
	layer[1] = 1;
	layerpri[1] = K053251_get_priority(K053251_CI0);
	layer[2] = 2;
	layerpri[2] = K053251_get_priority(K053251_CI2);

	sortlayers(layer,layerpri);

	fillbitmap(priority_bitmap,0,cliprect);
	/* note the '+1' in the background color!!! */
	fillbitmap(bitmap,Machine->pens[16 * bg_colorbase+1],cliprect);
	tilemap_draw(bitmap,cliprect,K052109_tilemap[layer[0]],0,1);
	tilemap_draw(bitmap,cliprect,K052109_tilemap[layer[1]],0,2);
	tilemap_draw(bitmap,cliprect,K052109_tilemap[layer[2]],0,4);

	pdrawgfx_shadow_lowpri = 1;	/* fix shadows of boulders in front of feet */
	K053247_sprites_draw(bitmap,cliprect);
}

VIDEO_UPDATE( xmen6p )
{
	int x,y;
 /* update every other frame ...help prevent screens lagging (although it still does..)..
     but maybe the hw has its own buffering hence the extra ram */
/*  if (xmen_current_frame&0x8000)*/ /* actually don't bother */ 
	{

		for(y=0;y<32*8;y++)
		{
			UINT16* line_dest = (UINT16 *)(bitmap->line[y]);
			UINT16* line_src = (UINT16 *)(screen_right->line[y]);
			UINT16* line_src2 = (UINT16 *)(screen_left->line[y]);

			for (x=14*8;x<50*8;x++)
			{
				line_dest[x] = line_src[x];
			}
			for (x=16*8;x<52*8;x++)
			{
				line_dest[x+256] = line_src2[x];
			}
		}
	}
}

/* my lefts and rights are mixed up in several places.. */
VIDEO_EOF( xmen6p )
{
	int layer[3];
	struct mame_bitmap * renderbitmap;
	struct rectangle cliprect;
	int offset;

	xmen_current_frame ^=0x8000;

	cliprect.min_x = Machine->visible_area.min_x;
	cliprect.max_x = Machine->visible_area.max_x;
	cliprect.min_y = Machine->visible_area.min_y;
	cliprect.max_y = Machine->visible_area.max_y;

	if (xmen_current_frame&0x8000)
	{

			/* copy the desired spritelist to the chip */
		memcpy(K053247_ram,xmen6p_spriteramright,0x1000);
		/* we write the entire content of the tileram to the chip to ensure
           everything gets marked as dirty and the desired tilemap is rendered

           this is not very efficient!
           */
		for (offset=0;offset<(0xc000/2);offset++)
		{
/*          K052109_lsb_w */
			if (offset != 0x1c80 && offset != 0x1e80)
				{
					if (xmen6p_tilemap_select)
						K052109_w(offset, xmen6p_tilemaprightalt[offset] & 0x00ff);
					else
						K052109_w(offset, xmen6p_tilemapright[offset] & 0x00ff);
				}
		}

		renderbitmap = screen_left;
	}
	else
	{
		/* copy the desired spritelist to the chip */
		memcpy(K053247_ram,xmen6p_spriteramleft,0x1000);

		/* we write the entire content of the tileram to the chip to ensure
           everything gets marked as dirty and the desired tilemap is rendered

           this is not very efficient!
           */
		for (offset=0;offset<(0xc000/2);offset++)
		{
/*          K052109_lsb_w */
		    {
				if (xmen6p_tilemap_select)
				  {
						if (offset != 0x1c80 && offset != 0x1e80)
							K052109_w(offset, xmen6p_tilemapleftalt[offset] & 0x00ff);
					}
					else
						    K052109_w(offset, xmen6p_tilemapleft[offset] & 0x00ff);
		    }
		}

		renderbitmap = screen_right;
	}

	bg_colorbase       = K053251_get_palette_index(K053251_CI4);
	sprite_colorbase   = K053251_get_palette_index(K053251_CI1);
	layer_colorbase[0] = K053251_get_palette_index(K053251_CI3);
	layer_colorbase[1] = K053251_get_palette_index(K053251_CI0);
	layer_colorbase[2] = K053251_get_palette_index(K053251_CI2);

	K052109_tilemap_update();

	layer[0] = 0;
	layerpri[0] = K053251_get_priority(K053251_CI3);
	layer[1] = 1;
	layerpri[1] = K053251_get_priority(K053251_CI0);
	layer[2] = 2;
	layerpri[2] = K053251_get_priority(K053251_CI2);

	sortlayers(layer,layerpri);

	fillbitmap(priority_bitmap,0,&cliprect);
	/* note the '+1' in the background color!!! */
	fillbitmap(renderbitmap,Machine->pens[16 * bg_colorbase+1],&cliprect);
	tilemap_draw(renderbitmap,&cliprect,K052109_tilemap[layer[0]],0,1);
	tilemap_draw(renderbitmap,&cliprect,K052109_tilemap[layer[1]],0,2);
	tilemap_draw(renderbitmap,&cliprect,K052109_tilemap[layer[2]],0,4);

	pdrawgfx_shadow_lowpri = 1;	/* fix shadows of boulders in front of feet */
	K053247_sprites_draw(renderbitmap,&cliprect);
}
