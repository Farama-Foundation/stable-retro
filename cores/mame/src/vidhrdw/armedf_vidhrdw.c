#include "driver.h"
#include "vidhrdw/generic.h"


data16_t armedf_vreg;

data16_t *terraf_text_videoram;
data16_t *spr_pal_clut;
data16_t *armedf_bg_videoram;
data16_t *armedf_fg_videoram;
data16_t *legion_cmd;
static data16_t armedf_fg_scrollx,armedf_fg_scrolly;

data16_t terraf_scroll_msb;

static struct tilemap *bg_tilemap, *fg_tilemap;
struct tilemap *armedf_tx_tilemap;

static int scroll_type,sprite_offy, mcu_mode, old_mcu_mode;

void armedf_setgfxtype( int type )
{
	scroll_type = type;
	mcu_mode = 0;
	old_mcu_mode = 0;
}

/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

static UINT32 armedf_scan(UINT32 col,UINT32 row,UINT32 num_cols,UINT32 num_rows)
{ /* col: 0..63; row: 0..31 */
	switch( scroll_type )
	{
	case 1: /* armed formation */
		return col*32+row;

	case 3: /* legion */
	case 6: /* legiono */
		return (col&0x1f)*32+row+0x800*(col/32);

	default:
		return 32*(31-row)+(col&0x1f)+0x800*(col/32);
	}
}

static void get_tx_tile_info(int tile_index)
{
	int tile_number = terraf_text_videoram[tile_index]&0xff;
	int attributes;

	if( scroll_type == 1 )
	{
		attributes = terraf_text_videoram[tile_index+0x800]&0xff;
	}
	else
	{
		attributes = terraf_text_videoram[tile_index+0x400]&0xff;
	}
	SET_TILE_INFO(
			0,
			tile_number + 256 * (attributes & 0x3),
			attributes >> 4,
			0)
}

static void get_legion_tx_tile_info(int tile_index)
{

	int tile_number = terraf_text_videoram[tile_index]&0xff;
	int attributes;
	
	if(tile_index<0x10) tile_number=0x20;

	if( scroll_type == 1 )
	{
		attributes = terraf_text_videoram[tile_index+0x800]&0xff;
	}
	else
	{
		attributes = terraf_text_videoram[tile_index+0x400]&0xff;
	}
	
	
	tile_info.priority = 0;

	if((attributes & 0x3) == 3)
	{
		tile_info.priority = 1;	
	}
	
	SET_TILE_INFO(
			0,
			tile_number + 256 * (attributes & 0x3),
			attributes >> 4,
			0)
}

static void get_fg_tile_info( int tile_index )
{
	int data = armedf_fg_videoram[tile_index];
	SET_TILE_INFO(
			1,
			data&0x7ff,
			data>>11,
			0)
}


static void get_bg_tile_info( int tile_index )
{
	int data = armedf_bg_videoram[tile_index];
	SET_TILE_INFO(
			2,
			data&0x3ff,
			data>>11,
			0)
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

VIDEO_START( armedf )
{
	if( scroll_type == 4 || /* cclimbr2 */
		scroll_type == 3 || /* legion */
		scroll_type == 6 )  /* legiono */
	{
		sprite_offy = 0;
	}
	else
	{
		sprite_offy = 128;
	}

	bg_tilemap = tilemap_create(get_bg_tile_info,tilemap_scan_cols,TILEMAP_TRANSPARENT,16,16,64,32);
	fg_tilemap = tilemap_create(get_fg_tile_info,tilemap_scan_cols,TILEMAP_TRANSPARENT,16,16,64,32);
	armedf_tx_tilemap = tilemap_create(get_tx_tile_info,armedf_scan,TILEMAP_TRANSPARENT,8,8,64,32);

	if (!bg_tilemap || !fg_tilemap || !armedf_tx_tilemap)
		return 1;

	tilemap_set_transparent_pen(fg_tilemap,0xf);
	tilemap_set_transparent_pen(armedf_tx_tilemap,0xf);
	tilemap_set_transparent_pen(bg_tilemap,0xf);

	if( scroll_type!=1 )
	{
		tilemap_set_scrollx(armedf_tx_tilemap,0,-128);
	}

	return 0;
}

VIDEO_START( legion )
{
	if( scroll_type == 4 || /* cclimbr2 */
		scroll_type == 3 || /* legion */
		scroll_type == 6 )  /* legiono */
	{
		sprite_offy = 0;
	}
	else
	{
		sprite_offy = 128;
	}

	bg_tilemap = tilemap_create(get_bg_tile_info,tilemap_scan_cols,TILEMAP_TRANSPARENT,16,16,64,32);
	fg_tilemap = tilemap_create(get_fg_tile_info,tilemap_scan_cols,TILEMAP_TRANSPARENT,16,16,64,32);
	armedf_tx_tilemap = tilemap_create(get_legion_tx_tile_info,armedf_scan,TILEMAP_TRANSPARENT,8,8,64,32);

	if (!bg_tilemap || !fg_tilemap || !armedf_tx_tilemap)
		return 1;

	tilemap_set_transparent_pen(fg_tilemap,0xf);
	tilemap_set_transparent_pen(armedf_tx_tilemap,0xf);
	tilemap_set_transparent_pen(bg_tilemap,0xf);

	if( scroll_type!=1 )
	{
		tilemap_set_scrollx(armedf_tx_tilemap,0,-128);
	}

	return 0;
}

/***************************************************************************

  Memory handlers

***************************************************************************/

WRITE16_HANDLER( armedf_text_videoram_w )
{
	int oldword = terraf_text_videoram[offset];
	COMBINE_DATA(&terraf_text_videoram[offset]);
	if (oldword != terraf_text_videoram[offset])
	{
		if( scroll_type == 1 )
		{
			tilemap_mark_tile_dirty(armedf_tx_tilemap,offset & 0x7ff);
		}
		else
		{
			tilemap_mark_tile_dirty(armedf_tx_tilemap,offset & 0xbff);
		}
	}

}


WRITE16_HANDLER( armedf_fg_videoram_w )
{
	int oldword = armedf_fg_videoram[offset];
	COMBINE_DATA(&armedf_fg_videoram[offset]);
	if (oldword != armedf_fg_videoram[offset])
		tilemap_mark_tile_dirty(fg_tilemap,offset);
}

WRITE16_HANDLER( armedf_bg_videoram_w )
{
	int oldword = armedf_bg_videoram[offset];
	COMBINE_DATA(&armedf_bg_videoram[offset]);
	if (oldword != armedf_bg_videoram[offset])
		tilemap_mark_tile_dirty(bg_tilemap,offset);
}

static int waiting_msb;

WRITE16_HANDLER( terraf_fg_scrollx_w )
{
	if (ACCESSING_MSB)
	{
		armedf_fg_scrollx = data >> 8;
		waiting_msb = 1;
	}
}

WRITE16_HANDLER( terraf_fg_scrolly_w )
{
	if (ACCESSING_MSB)
	{
		if (waiting_msb)
			terraf_scroll_msb = data >> 8;
		else
			armedf_fg_scrolly = data >> 8;
	}
}

WRITE16_HANDLER( terraf_fg_scroll_msb_arm_w )
{
	if (ACCESSING_MSB)
		waiting_msb = 0;
}

WRITE16_HANDLER( armedf_fg_scrollx_w )
{
	COMBINE_DATA(&armedf_fg_scrollx);
}

WRITE16_HANDLER( armedf_fg_scrolly_w )
{
	COMBINE_DATA(&armedf_fg_scrolly);
}

WRITE16_HANDLER( armedf_bg_scrollx_w )
{
	static data16_t scroll;
	COMBINE_DATA(&scroll);
	tilemap_set_scrollx(bg_tilemap,0,scroll);
}

WRITE16_HANDLER( armedf_bg_scrolly_w )
{
	static data16_t scroll;
	COMBINE_DATA(&scroll);
	tilemap_set_scrolly(bg_tilemap,0,scroll);
}

WRITE16_HANDLER( armedf_mcu_cmd )
{
	COMBINE_DATA(&mcu_mode);	
}



/***************************************************************************

  Display refresh

***************************************************************************/

/* custom code to handle color cycling effect, handled by m_spr_pal_clut */
static void armedf_drawgfx(struct mame_bitmap *dest_bmp, const struct rectangle *clip, const struct GfxElement *gfx,
							UINT32 code,UINT32 color, UINT32 clut,int flipx,int flipy,int offsx,int offsy,
							int transparent_color)
{
	const pen_t *pal = &gfx->colortable[0 + gfx->color_granularity * (color % gfx->total_colors)];
	const UINT8 *source_base = gfx->gfxdata + (code % gfx->total_elements) * gfx->char_modulo;
	int x_index_base, y_index, sx, sy, ex, ey;
	int xinc, yinc;

	xinc = flipx ? -1 : 1;
	yinc = flipy ? -1 : 1;

	x_index_base = flipx ? gfx->width-1 : 0;
	y_index = flipy ? gfx->height-1 : 0;

	/* start coordinates */
	sx = offsx;
	sy = offsy;

	/* end coordinates */
	ex = sx + gfx->width;
	ey = sy + gfx->height;

	if (clip)
	{
		if (sx < clip->min_x)
		{ /* clip left */
			int pixels = clip->min_x-sx;
			sx += pixels;
			x_index_base += xinc*pixels;
		}
		if (sy < clip->min_y)
		{ /* clip top */
			int pixels = clip->min_y-sy;
			sy += pixels;
			y_index += yinc*pixels;
		}
		/* NS 980211 - fixed incorrect clipping */
		if (ex > clip->max_x+1)
		{ /* clip right */
			ex = clip->max_x+1;
		}
		if (ey > clip->max_y+1)
		{ /* clip bottom */
			ey = clip->max_y+1;
		}
	}

	if (ex > sx)
	{ /* skip if inner loop doesn't draw anything */
		int x, y;
		{
			for (y = sy; y < ey; y++)
			{
				const UINT8 *source = source_base + y_index*gfx->line_modulo;
				UINT16 *dest = (UINT16*)dest_bmp->line[y];
				int x_index = x_index_base;
				for (x = sx; x < ex; x++)
				{
					int c = (source[x_index] & ~0xf) | ((spr_pal_clut[clut*0x10+(source[x_index] & 0xf)]) & 0xf);
					if (c != transparent_color)
						dest[x] = pal[c];

					x_index += xinc;
				}
				y_index += yinc;
			}
		}
	}
}

static void draw_sprites( struct mame_bitmap *bitmap, const struct rectangle *cliprect, int priority )
{
	int offs;

	for (offs = 0;offs < spriteram_size/2;offs += 4)
	{
		int code = buffered_spriteram16[offs+1]; /* ??YX?TTTTTTTTTTT */
		int flipx = code & 0x2000;
		int flipy = code & 0x1000;
		int color = (buffered_spriteram16[offs+2]>>8)&0x1f;
		int clut = (buffered_spriteram16[offs+2]) & 0x7f;
		int sx = buffered_spriteram16[offs+3];
		int sy = sprite_offy+240-(buffered_spriteram16[offs+0]&0x1ff);

		if (flip_screen) {
			sx = 320 - sx + 176;	/* don't ask where 176 comes from, just tried it out */
			sy = 240 - sy + 1;		/* don't ask where 1 comes from, just tried it out */
			flipx = !flipx;			/* the values seem to result in pixel-correct placement */
			flipy = !flipy;			/* in all the games supported by this driver */
		}

		if (((buffered_spriteram16[offs+0] & 0x3000) >> 12) == priority)
		{
			armedf_drawgfx(bitmap,cliprect,Machine->gfx[3],
				code & 0xfff,
				color, clut,
 				flipx,flipy,
				sx,sy,15);
		}
	}
}

static void copy_textmap(int index)
{
	/*
		(not simulated)
		1st half of the MCU ROM contains various strings and
		gfx elements (copied by MCU to textram)
				
		
		(partially simulated)
		2nd half of the MCu external ROM contains text tilemaps:
		 4 - title screen
		 5 - bottom layer gfx, visible  in later levels, during boss fight
		 6 - test mode screen (not hooked up)
		 7 - portraits (title)
	*/

	UINT8 * data = (UINT8 *)memory_region(REGION_GFX5);
	int bank;
	int tile;
    int i;
	for(i=0;i<0x400;++i)
	{
		if(i<0x10) continue;

		tile=data[0x800*index+i];
		bank=data[0x800*index+i+0x400]&3;
			
		if( (tile|(bank<<8))!=0x20)
		{
			terraf_text_videoram[i]=tile;
			terraf_text_videoram[i+0x400]=data[0x800*index+i+0x400];
		}
	
	}

	tilemap_mark_all_tiles_dirty(armedf_tx_tilemap);

}



VIDEO_UPDATE( armedf )
{
	int sprite_enable = armedf_vreg & 0x200;

	tilemap_set_enable( bg_tilemap, armedf_vreg&0x800 );
	tilemap_set_enable( fg_tilemap, armedf_vreg&0x400 );
	tilemap_set_enable( armedf_tx_tilemap, armedf_vreg&0x100 );

	if ((scroll_type == 0)||(scroll_type == 5 )) {
		if (old_mcu_mode!=mcu_mode) {
			if ((mcu_mode&0x000f)==0x0004) {		/* transparent tx */
				tilemap_set_transparent_pen(armedf_tx_tilemap, 0x0f);
				tilemap_mark_all_tiles_dirty( armedf_tx_tilemap );
				/*logerror("? Transparent TX 0x0f\n");*/
			}
			if ((mcu_mode&0x000f)==0x000f) {		/* opaque tx*/
				tilemap_set_transparent_pen(armedf_tx_tilemap, 0x10);
				tilemap_mark_all_tiles_dirty( armedf_tx_tilemap );
				/*logerror("? Opaque TX\n");*/
			}
			
			old_mcu_mode = mcu_mode;
			/*logerror("MCU Change => %04x\n",mcu_mode);*/
		}
	}

	switch (scroll_type)
	{
		case 0: /* terra force */
			tilemap_set_scrollx( fg_tilemap, 0, armedf_fg_scrolly + ((terraf_scroll_msb>>4)&3)*256 );
			tilemap_set_scrolly( fg_tilemap, 0, armedf_fg_scrollx + ((terraf_scroll_msb)&3)*256 );
			break;

		case 1: /* armed formation */
			tilemap_set_scrollx( fg_tilemap, 0, armedf_fg_scrollx );
			tilemap_set_scrolly( fg_tilemap, 0, armedf_fg_scrolly );
			break;

		case 6: /* legiono */
			tilemap_set_scrollx( fg_tilemap, 0, (legion_cmd[13] & 0xff) | ((legion_cmd[14] & 0x3)<<8) );
			tilemap_set_scrolly( fg_tilemap, 0, (legion_cmd[11] & 0xff) | ((legion_cmd[12] & 0x3)<<8) );
			break;
		case 2: /* kodure ookami */
		case 3:
		case 4: /* crazy climber 2 */
			{
				int scrollx,scrolly;

				/* scrolling is handled by the protection mcu */
				scrollx = (terraf_text_videoram[13] & 0xff) | (terraf_text_videoram[14] << 8);
				scrolly = (terraf_text_videoram[11] & 0xff) | (terraf_text_videoram[12] << 8);
				tilemap_set_scrollx( fg_tilemap, 0, scrollx);
				tilemap_set_scrolly( fg_tilemap, 0, scrolly);
			}
			break;
		case 5: /* terra force (US) */
			tilemap_set_scrollx( fg_tilemap, 0, (terraf_text_videoram[13] & 0xff) | ((terraf_text_videoram[14] & 0x3)<<8) );
			tilemap_set_scrolly( fg_tilemap, 0, (terraf_text_videoram[11] & 0xff) | ((terraf_text_videoram[12] & 0x3)<<8) );
			break;
			
	}


	fillbitmap( bitmap, 0xff, cliprect );


	if(scroll_type == 3 || scroll_type == 6) /* legion / legiono */
	{
		tilemap_draw(bitmap, cliprect, armedf_tx_tilemap, 1, 0);
	}

	if (armedf_vreg & 0x0800) tilemap_draw( bitmap, cliprect, bg_tilemap, 0, 0);
	/*if( armedf_vreg & 0x0800 )
    {
        tilemap_draw( bitmap, cliprect, bg_tilemap, 0, 0);
    }
    else
    {
        fillbitmap( bitmap, get_black_pen()&0x0f, cliprect );
    }*/

	if ((mcu_mode&0x0030)==0x0030) tilemap_draw( bitmap, cliprect, armedf_tx_tilemap, 0, 0);
	if( sprite_enable ) draw_sprites( bitmap, cliprect, 2 );
	if ((mcu_mode&0x0030)==0x0020) tilemap_draw( bitmap, cliprect, armedf_tx_tilemap, 0, 0);
	tilemap_draw( bitmap, cliprect, fg_tilemap, 0, 0);
	if ((mcu_mode&0x0030)==0x0010) tilemap_draw( bitmap, cliprect, armedf_tx_tilemap, 0, 0);
	if( sprite_enable ) draw_sprites( bitmap, cliprect, 1 );
	if ((mcu_mode&0x0030)==0x0000) tilemap_draw( bitmap, cliprect, armedf_tx_tilemap, 0, 0);
	if( sprite_enable ) draw_sprites( bitmap, cliprect, 0 );
	
	if(scroll_type == 3) /* legion */
	{
		static int oldmode=-1;	
	
		int mode=terraf_text_videoram[1]&0xff;
		
		if (mode != oldmode)
		{
			oldmode=mode;
			switch(mode)
			{
				case 0x01: copy_textmap(4); break; /* title screen */
				case 0x06: copy_textmap(7); break; /* portraits on title screen */
				case 0x1c: copy_textmap(5); break; /* bottom, in-game layer */
				default: log_cb(RETRO_LOG_DEBUG, LOGPRE "unknown mode %d\n", mode); break;
			}
		}
			
	}
	
}

VIDEO_EOF( armedf )
{
	buffer_spriteram16_w(0,0,0);
}
