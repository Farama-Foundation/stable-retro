/***************************************************************************

 Lethal Enforcers
 (c) 1992 Konami

 Video hardware emulation.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/konamiic.h"

#define GUNX( a ) ( ( readinputport( a*2 ) * 287 ) / 0xff )
#define GUNY( a ) ( ( readinputport( (a*2)+1 ) * 223 ) / 0xff )

static int sprite_colorbase;
static int layer_colorbase[4];
/*static int layerpri[4] ={ 1,2,4,0 }; */

static void lethalen_sprite_callback(int *code, int *color, int *priority_mask)
{
	int pri = (*color & 0xfff0);
	*color = *color & 0x000f;
	*color+=0x400/64; /* colourbase? */

	/* this isn't ideal.. shouldn't need to hardcode it? not 100% sure about it anyway*/
	if (pri==0x10) *priority_mask = 0xf0; /* guys on first level */
	else if (pri==0x90) *priority_mask = 0xf0; /* car doors */
	else if (pri==0x20) *priority_mask = 0xf0|0xcc; /* people behind glass on 1st level */
	else if (pri==0xa0) *priority_mask = 0xf0|0xcc; /* glass on 1st/2nd level */
	else if (pri==0x40) *priority_mask = 0; /* blood splats? */
	else if (pri==0x00) *priority_mask = 0; /* gunshots etc */
	else if (pri==0x30) *priority_mask = 0xf0|0xcc|0xaa; /* mask sprites (always in a bad colour, used to do special effects i think */
	else
	{
		/*log_cb(RETRO_LOG_DEBUG, LOGPRE "unknown pri %04x\n",pri);*/
		*priority_mask = 0;
	}

	*code = (*code & 0x3fff); /* | spritebanks[(*code >> 12) & 3]; */
}

static void lethalen_tile_callback(int layer, int *code, int *color)
{
	/* where are the flip bits? lethal enforcers needs them on reload and p2 start screen */
	/* they don't seem to be anywhere in the ram we're looking at... */
/*  tile_info.flags = TILE_FLIPYX(1);; */


	*color = layer_colorbase[layer] + ((*color & 0x3c)<<2);
}

VIDEO_START(lethalen)
{
	int i;

	K053251_vh_start();

	K056832_vh_start(REGION_GFX1, K056832_BPP_8LE, 1, NULL, lethalen_tile_callback, 0);

	if (K053245_vh_start(0, REGION_GFX3,NORMAL_PLANE_ORDER, lethalen_sprite_callback))
		return 1;

	/* the default drawmode table is no good for 6bpp, create a new one */
	gfx_drawmode_table[0] = DRAWMODE_NONE;
	for (i = 1;i < 64;i++)
		gfx_drawmode_table[i] = DRAWMODE_SOURCE;

	gfx_drawmode_table[63] = DRAWMODE_SHADOW;

	/* this game uses external linescroll RAM */
	K056832_SetExtLinescroll();

	/* the US and Japanese cabinets apparently use different mirror setups */
	if (!strcmp(Machine->gamedrv->name, "lethalen"))
	{
 		K056832_set_LayerOffset(0, 188, 0);
		K056832_set_LayerOffset(1, 190, 0);
		K056832_set_LayerOffset(2, 192, 0);
		K056832_set_LayerOffset(3, 194, 0);
		K053245_set_SpriteOffset(0, 95, 0);
	}
	else
	{ /* fixme */
 		K056832_set_LayerOffset(0, 64, 0);
		K056832_set_LayerOffset(1, 64, 0);
		K056832_set_LayerOffset(2, 64, 0);
		K056832_set_LayerOffset(3, 64, 0);
		K053245_set_SpriteOffset(0, -96, 8);
	}

	layer_colorbase[0] = 0x00;
	layer_colorbase[1] = 0x40;
	layer_colorbase[2] = 0x80;
	layer_colorbase[3] = 0xc0;

	return 0;
}

WRITE_HANDLER(le_palette_control)
{
	switch (offset)
	{
		case 0:	/* 40c8 - PCU1 from schematics */
			layer_colorbase[0] = ((data & 0x7)-1) * 0x40;
			layer_colorbase[1] = (((data>>4) & 0x7)-1) * 0x40;
			K056832_mark_plane_dirty(0);
			K056832_mark_plane_dirty(1);
			break;

		case 4: /* 40cc - PCU2 from schematics */
			layer_colorbase[2] = ((data & 0x7)-1) * 0x40;
			layer_colorbase[3] = (((data>>4) & 0x7)-1) * 0x40;
			K056832_mark_plane_dirty(2);
			K056832_mark_plane_dirty(3);
			break;

		case 8:	/* 40d0 - PCU3 from schematics */
			sprite_colorbase = ((data & 0x7)-1) * 0x40;
			break;
	}
}

extern data16_t *K056832_videoram;

VIDEO_UPDATE(lethalen)
{
	fillbitmap(bitmap, 7168, cliprect);
	fillbitmap(priority_bitmap, 0, cliprect);

	K056832_tilemap_draw(bitmap, cliprect, 3, TILE_LINE_DISABLED, 1);
	K056832_tilemap_draw(bitmap, cliprect, 2, 0, 2);
	K056832_tilemap_draw(bitmap, cliprect, 1, 0, 4);

	K053245_sprites_draw_lethal(0, bitmap, cliprect);

	/* force "A" layer over top of everything */
	K056832_tilemap_draw(bitmap, cliprect, 0, 0, 0);

	draw_crosshair(1, bitmap, GUNX(1)+216, 240-GUNY(1), cliprect );
	draw_crosshair(2, bitmap, GUNX(2)+216, 240-GUNY(2), cliprect );
 

#if 0
	{
		FILE *fp;

		fp=fopen("K056832_videoram", "w+b");
		if (fp)
		{
			fwrite(K056832_videoram, 0x10000, 2, fp);
			fclose(fp);
		}
	}
#endif
}
