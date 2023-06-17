/* video hardware for Namco System II */

#include "driver.h"
#include "vidhrdw/generic.h"
#include "namcos2.h"
#include "namcoic.h"

data16_t *namcos2_sprite_ram;
size_t namcos2_68k_vram_size;
data16_t *namcos2_68k_palette_ram;
size_t namcos2_68k_palette_size;
size_t namcos2_68k_roz_ram_size;
data16_t *namcos2_68k_roz_ram;

static data16_t namcos2_68k_roz_ctrl[0x8];

static struct tilemap *tilemap[6];
static struct tilemap *tilemap_roz;

static data16_t namcos2_68k_vram_ctrl[0x20];

/**
 * namcos2_gfx_ctrl selects a bank of 128 sprites within spriteram
 *
 * namcos2_gfx_ctrl also supplies palette and priority information that is applied to the output of the
 *                  Namco System 2 ROZ chip
 *
 * -xxx ---- ---- ---- roz priority
 * ---- xxxx ---- ---- roz palette
 * ---- ---- ---- xxxx sprite bank
 */
static data16_t namcos2_gfx_ctrl;

READ16_HANDLER( namcos2_gfx_ctrl_r )
{
	return namcos2_gfx_ctrl;
}

WRITE16_HANDLER( namcos2_gfx_ctrl_w )
{
	COMBINE_DATA(&namcos2_gfx_ctrl);
}

/**************************************************************************/

static INLINE void get_tile_info(int tile_index,data16_t *vram)
{
	int tile;
	tile = vram[tile_index];

	/* The tile mask DOESNT use the mangled tile number */
	tile_info.mask_data = memory_region(REGION_GFX4)+(0x08*tile);

	switch( namcos2_gametype )
	{
	case NAMCOS2_FINAL_LAP_2:
	case NAMCOS2_FINAL_LAP_3:
		tile = (tile&0x07ff)|((tile&0x4000)>>3)|((tile&0x3800)<<1);
		break;

	default:
		/* The order of bits needs to be corrected to index the right tile  14 15 11 12 13 */
		tile = (tile&0x07ff)|((tile&0xc000)>>3)|((tile&0x3800)<<2);
		break;
	}
	SET_TILE_INFO(2,tile,0,0)
}

static void get_tile_info0(int tile_index) { get_tile_info(tile_index,videoram16+0x0000); }
static void get_tile_info1(int tile_index) { get_tile_info(tile_index,videoram16+0x1000); }
static void get_tile_info2(int tile_index) { get_tile_info(tile_index,videoram16+0x2000); }
static void get_tile_info3(int tile_index) { get_tile_info(tile_index,videoram16+0x3000); }
static void get_tile_info4(int tile_index) { get_tile_info(tile_index,videoram16+0x4008); }
static void get_tile_info5(int tile_index) { get_tile_info(tile_index,videoram16+0x4408); }

static int
CreateTilemaps( void )
{
	int i;

	/* four scrolling tilemaps */
	tilemap[0] = tilemap_create(get_tile_info0,tilemap_scan_rows,TILEMAP_BITMASK,8,8,64,64);
	tilemap[1] = tilemap_create(get_tile_info1,tilemap_scan_rows,TILEMAP_BITMASK,8,8,64,64);
	tilemap[2] = tilemap_create(get_tile_info2,tilemap_scan_rows,TILEMAP_BITMASK,8,8,64,64);
	tilemap[3] = tilemap_create(get_tile_info3,tilemap_scan_rows,TILEMAP_BITMASK,8,8,64,64);

	/* two non-scrolling tilemaps */
	tilemap[4] = tilemap_create(get_tile_info4,tilemap_scan_rows,TILEMAP_BITMASK,8,8,36,28);
	tilemap[5] = tilemap_create(get_tile_info5,tilemap_scan_rows,TILEMAP_BITMASK,8,8,36,28);

	/* ensure that all tilemaps have been allocated */
	for( i=0; i<=5; i++ )
	{
		if( !tilemap[i] ) return -1;
	}

	/* define offsets for scrolling */
	for( i=0; i<4; i++ )
	{
		const int adj[4] = { 4,2,1,0 };
		int dx = 44+adj[i];
		tilemap_set_scrolldx( tilemap[i], -dx, -(-288-dx) );
		tilemap_set_scrolldy( tilemap[i], -24, -(-224-24) );
	}

	return 0;
}

static void
DrawTilemaps( struct mame_bitmap *bitmap, const struct rectangle *cliprect, int pri )
{
	int i;
	for( i=0; i<6; i++ )
	{
		if( (namcos2_68k_vram_ctrl[0x20/2+i]&0x7)==pri )
		{
			int color = namcos2_68k_vram_ctrl[0x30/2+i] & 0x07;
			tilemap_set_palette_offset( tilemap[i], color*256 );
			tilemap_draw(bitmap,cliprect,tilemap[i],0,0);
		}
	}
}

WRITE16_HANDLER( namcos2_68k_vram_w )
{
	data16_t oldword = videoram16[offset];
	COMBINE_DATA( &videoram16[offset] );

	if( oldword != videoram16[offset] )
	{
		if( offset<0x4000 )
		{
			tilemap_mark_tile_dirty(tilemap[offset>>12],offset&0xfff);
		}
		else if( offset>=0x8010/2 && offset<0x87f0/2 )
		{ /* fixed plane#1 */
			offset-=0x8010/2;
			tilemap_mark_tile_dirty( tilemap[4], offset );
		}
		else if( offset>=0x8810/2 && offset<0x8ff0/2 )
		{ /* fixed plane#2 */
			offset-=0x8810/2;
			tilemap_mark_tile_dirty( tilemap[5], offset );
		}
	}
}

READ16_HANDLER( namcos2_68k_vram_r )
{
	return videoram16[offset];
}

READ16_HANDLER( namcos2_68k_vram_ctrl_r )
{
	return namcos2_68k_vram_ctrl[offset];
}

WRITE16_HANDLER( namcos2_68k_vram_ctrl_w )
{
	int i;
	data16_t oldword, newword;
	oldword = namcos2_68k_vram_ctrl[offset];
	COMBINE_DATA( &namcos2_68k_vram_ctrl[offset] );
	newword = namcos2_68k_vram_ctrl[offset];

	if( oldword != newword )
	{
		if( offset<0x20/2 )
		{
			if( offset == 0x02/2 )
			{
				/* all planes are flipped X+Y from D15 of this word */
				int attrs = (newword & 0x8000)?(TILEMAP_FLIPX|TILEMAP_FLIPY):0;
				for( i=0; i<=5; i++ )
				{
					tilemap_set_flip(tilemap[i],attrs);
				}
			}
		}

		newword &= 0x1ff;
		if( namcos2_68k_vram_ctrl[0x02/2]&0x8000 )
		{
			newword = -newword;
		}

		switch( offset )
		{
		case 0x02/2:
			tilemap_set_scrollx( tilemap[0], 0, newword );
			break;

		case 0x06/2:
			tilemap_set_scrolly( tilemap[0], 0, newword );
			break;

		case 0x0a/2:
			tilemap_set_scrollx( tilemap[1], 0, newword );
			break;

		case 0x0e/2:
			tilemap_set_scrolly( tilemap[1], 0, newword );
			break;

		case 0x12/2:
			tilemap_set_scrollx( tilemap[2], 0, newword );
			break;

		case 0x16/2:
			tilemap_set_scrolly( tilemap[2], 0, newword );
			break;

		case 0x1a/2:
			tilemap_set_scrollx( tilemap[3], 0, newword );
			break;

		case 0x1e/2:
			tilemap_set_scrolly( tilemap[3], 0, newword );
			break;
		}
	}
}

/**************************************************************************/

static void get_tile_info_roz(int tile_index)
{
	int tile = namcos2_68k_roz_ram[tile_index];
	SET_TILE_INFO(3,tile,0/*color*/,0)
}

static void
DrawROZ(struct mame_bitmap *bitmap,const struct rectangle *cliprect)
{
	const int xoffset = 38,yoffset = 0;

	int incxx =  (INT16)namcos2_68k_roz_ctrl[0];
	int incxy =  (INT16)namcos2_68k_roz_ctrl[1];
	int incyx =  (INT16)namcos2_68k_roz_ctrl[2];
	int incyy =  (INT16)namcos2_68k_roz_ctrl[3];
	UINT32 startx = (INT16)namcos2_68k_roz_ctrl[4];
	UINT32 starty = (INT16)namcos2_68k_roz_ctrl[5];

#if 0 /* TBA */
	switch( namcos2_68k_roz_ctrl[7] )
	{
	case 0x4400: /* (2048x2048) */
		tilemap_set_bounds( tilemap_roz, 0,0,2048,2048 );
		break;

	case 0x4488:
		tilemap_set_bounds( tilemap_roz, 0,0,128,128 ); /* ? */
		break;

	case 0x44cc:
		tilemap_set_bounds( tilemap_roz, 0,0,128,128 ); /* ? */
		break;

	case 0x44ee: /* (256x256?) used in Dragon Saber */
		tilemap_set_bounds( tilemap_roz, 0,0,256,256 );
		break;
	}
#endif

	startx <<= 4;
	starty <<= 4;
	startx += xoffset * incxx + yoffset * incyx;
	starty += xoffset * incxy + yoffset * incyy;

	tilemap_set_palette_offset( tilemap_roz, namcos2_gfx_ctrl & 0x0f00 );

	tilemap_draw_roz(bitmap,cliprect,tilemap_roz,startx << 8,starty << 8,
			incxx <<8,incxy << 8,incyx << 8,incyy << 8,
			1,	/* copy with wraparound */
			0,0);
}

/**
 *	ROZ - Rotate & Zoom memory function handlers
 *
 *	0 - inc xx
 *	2 - inc xy
 *	4 - inc yx
 *	6 - inc yy
 *	8 - start x
 *	A	start y
 *	C - ??
 *	E - ??
 */
READ16_HANDLER(namcos2_68k_roz_ctrl_r)
{
	return namcos2_68k_roz_ctrl[offset];
}

WRITE16_HANDLER( namcos2_68k_roz_ctrl_w )
{
	COMBINE_DATA(&namcos2_68k_roz_ctrl[offset]);
}

READ16_HANDLER( namcos2_68k_roz_ram_r )
{
	return namcos2_68k_roz_ram[offset];
}

WRITE16_HANDLER( namcos2_68k_roz_ram_w )
{
	data16_t oldword = namcos2_68k_roz_ram[offset];
	COMBINE_DATA(&namcos2_68k_roz_ram[offset]);
	if (oldword != namcos2_68k_roz_ram[offset])
	{
		tilemap_mark_tile_dirty(tilemap_roz,offset);
	}
}

/**************************************************************************/

READ16_HANDLER( namcos2_68k_video_palette_r )
{
	offset*=2;
	/* 0x3000 offset is control registers */
	if( (offset & 0xf000) == 0x3000 )
	{
		/* Palette chip control registers */
		offset&=0x001f;
		switch( offset ){
			case 0x1a:
			case 0x1e:
				return 0xff;
				break;
			default:
				break;
		}
	}
	return namcos2_68k_palette_ram[(offset/2)&0x7fff];
}

WRITE16_HANDLER( namcos2_68k_video_palette_w )
{
	COMBINE_DATA(&namcos2_68k_palette_ram[offset]);
}

static void
UpdatePalette( void )
{
	int bank;
	for( bank=0; bank<0x20; bank++ )
	{
		int pen = bank*256;
		int offset = ((pen & 0x1800) << 2) | (pen & 0x07ff);
		int i;
		for( i=0; i<256; i++ )
		{
			int r = namcos2_68k_palette_ram[offset | 0x0000] & 0x00ff;
			int g = namcos2_68k_palette_ram[offset | 0x0800] & 0x00ff;
			int b = namcos2_68k_palette_ram[offset | 0x1000] & 0x00ff;
			palette_set_color(pen++,r,g,b);
			offset++;
		}
	}
} /* UpdatePalette */

/**************************************************************************/

static void
DrawSpriteInit( void )
{
	int i;
	/* set table for sprite color == 0x0f */
	for(i = 0;i <= 253;i++)
	{
		gfx_drawmode_table[i] = DRAWMODE_SOURCE;
	}
	gfx_drawmode_table[254] = DRAWMODE_SHADOW;
	gfx_drawmode_table[255] = DRAWMODE_NONE;
	for( i = 0; i<16*256; i++ )
	{
		palette_shadow_table[i] = i+0x2000;
	}
}

WRITE16_HANDLER( namcos2_sprite_ram_w )
{
	COMBINE_DATA(&namcos2_sprite_ram[offset]);
}

READ16_HANDLER( namcos2_sprite_ram_r )
{
	return namcos2_sprite_ram[offset];
}

static void
DrawSpritesDefault( struct mame_bitmap *bitmap, const struct rectangle *cliprect, int pri, int pri_mask )
{
	struct GfxElement gfx;
	int sprn,flipy,flipx,ypos,xpos,sizex,sizey,scalex,scaley;
	int offset,offset0,offset2,offset4,offset6;
	int loop,spr_region;

	offset=(namcos2_gfx_ctrl & 0x000f) * (128*4);

	for(loop=0;loop < 128;loop++)
	{
		/****************************************
		* Sprite data is 8 byte packed format   *
		*                                       *
		* Offset 0,1                            *
		*   Sprite Y position           D00-D08 *
		*   Sprite Size 16/32           D09     *
		*   Sprite Size Y               D10-D15 *
		*                                       *
		* Offset 2,3                            *
		*   Sprite Quadrant             D00-D01 *
		*   Sprite Number               D02-D12 *
		*   Sprite ROM Bank select      D13     *
		*   Sprite flip X               D14     *
		*   Sprite flip Y               D15     *
		*                                       *
		* Offset 4,5                            *
		*   Sprite X position           D00-D10 *
		*                                       *
		* Offset 6,7                            *
		*   Sprite priority             D00-D02 *
		*   Sprite colour index         D04-D07 *
		*   Sprite Size X               D10-D15 *
		*                                       *
		****************************************/

		offset0 = namcos2_sprite_ram[offset+(loop*4)+0];
		offset2 = namcos2_sprite_ram[offset+(loop*4)+1];
		offset4 = namcos2_sprite_ram[offset+(loop*4)+2];
		offset6 = namcos2_sprite_ram[offset+(loop*4)+3];

		/* Fetch sprite size registers */

		sizey=((offset0>>10)&0x3f)+1;
		sizex=(offset6>>10)&0x3f;

		if((offset0&0x0200)==0) sizex>>=1;

		if((sizey-1) && sizex && (offset6&pri_mask)==pri)
		{
			int color = (offset6>>4)&0x000f;

			sprn=(offset2>>2)&0x7ff;
			spr_region=(offset2&0x2000)?1:0;

			ypos=(0x1ff-(offset0&0x01ff))-0x50+0x02;
			xpos=(offset4&0x03ff)-0x50+0x07;

			flipy=offset2&0x8000;
			flipx=offset2&0x4000;

			scalex = (sizex<<16)/((offset0&0x0200)?0x20:0x10);
			scaley = (sizey<<16)/((offset0&0x0200)?0x20:0x10);

			if(scalex && scaley)
			{
				gfx = *Machine->gfx[spr_region];

				if( (offset0&0x0200)==0 )
				{
					gfx.width = 16;
					gfx.height = 16;
					if( offset2&0x0001 ) gfx.gfxdata += 16;
					if( offset2&0x0002 ) gfx.gfxdata += 16*gfx.line_modulo;
				}

				drawgfxzoom(bitmap,&gfx,
					sprn,
					color,
					flipx,flipy,
					xpos,ypos,
					cliprect,
					(color==0x0f ? TRANSPARENCY_PEN_TABLE : TRANSPARENCY_PEN),0xff,
					scalex,scaley);
			}
		}
	}
} /* DrawSpritesDefault */

static void
DrawSpritesMetalHawk( struct mame_bitmap *bitmap, const struct rectangle *cliprect, int pri )
{
	/**
	 * word#0
	 *	xxxxxx---------- ysize
	 *	------x--------- sprite tile size
	 *	-------xxxxxxxxx screeny
	 *
	 * word#1
	 *	--x------------- bank
	 *	----xxxxxxxxxxxx tile
	 *
	 * word#3				2->3 by N
	 *	xxxxxx---------- xsize
	 *	------xxxxxxxxxx screenx
	 *
	 * word#6
	 *	--------------xx unknown
	 *	-------------x-- flipy
	 *	------------x--- unknown
	 *
	 * word#7
	 *	------------xxxx unknown
	 *	--------xxxx---- color
	 *	x--------------- unknown
	 */
	const data16_t *pSource = namcos2_sprite_ram;
	struct rectangle rect;
	int loop;
	for(loop=0;loop < 128;loop++)
	{
		int ypos = pSource[0];
		int tile = pSource[1];
		int xpos = pSource[3];

		int flags = pSource[6];
		int attrs = pSource[7];
		int sizey=((ypos>>10)&0x3f)+1;
		int sizex=(xpos>>10)&0x3f;
		int sprn=(tile>>2)&0x7ff;

		if( tile&0x2000 ) sprn&=0x3ff; else sprn|=0x400;
		if((sizey-1) && sizex && ((attrs>>1)&7)==pri )
		{
			int bSmallSprite =
				(sprn>=0x208 && sprn<=0x20F)||
				(sprn>=0x3BC && sprn<=0x3BF)||
				(sprn>=0x688 && sprn<=0x68B)||
				(sprn>=0x6D8 && sprn<=0x6D9)||
				(sprn>=0x6EA && sprn<=0x6EB); /* very stupid...*/

			int color = (attrs>>4)&0xf;
			int sx = (xpos&0x03ff)-0x50+0x07;
			int sy = (0x1ff-(ypos&0x01ff))-0x50+0x02;
			int flipx = flags&2;
			int flipy = flags&4;
			int scalex = (sizex<<16)/(bSmallSprite?0x10:0x20);
			int scaley = (sizey<<16)/(bSmallSprite?0x10:0x20);

			/* 90 degrees use a turned character*/
			if( (flags&0x01) ) {
				sprn |= 0x800;
			}

			/* little zoom fix...*/
			if( !bSmallSprite ) {
				if( sizex < 0x20 ) {
					sx -= (0x20-sizex)/0x8;
				}
				if( sizey < 0x20 ) {
					sy += (0x20-sizey)/0xC;
				}
			}

			/* Set the clipping rect to mask off the other portion of the sprite */
			rect.min_x=sx;
			rect.max_x=sx+(sizex-1);
			rect.min_y=sy;
			rect.max_y=sy+(sizey-1);

			if (cliprect->min_x > rect.min_x) rect.min_x = cliprect->min_x;
			if (cliprect->max_x < rect.max_x) rect.max_x = cliprect->max_x;
			if (cliprect->min_y > rect.min_y) rect.min_y = cliprect->min_y;
			if (cliprect->max_y < rect.max_y) rect.max_y = cliprect->max_y;

			if( bSmallSprite )
			{
				sizex = 16;
				sizey = 16;
				scalex = 1<<16;
				scaley = 1<<16;

				sx -= (tile&1)?16:0;
				sy -= (tile&2)?16:0;

				rect.min_x=sx;
				rect.max_x=sx+(sizex-1);
				rect.min_y=sy;
				rect.max_y=sy+(sizey-1);
				rect.min_x += (tile&1)?16:0;
				rect.max_x += (tile&1)?16:0;
				rect.min_y += (tile&2)?16:0;
				rect.max_y += (tile&2)?16:0;
			}
			drawgfxzoom(
				bitmap,Machine->gfx[0],
				sprn, color,
				flipx,flipy,
				sx,sy,
				&rect,
				TRANSPARENCY_PEN,0xff,
				scalex, scaley );
		}
		pSource += 8;
	}
} /* DrawSpritesMetalHawk */

/**************************************************************************/

static void
DrawCrossshair( struct mame_bitmap *bitmap, const struct rectangle *cliprect )
{
	int x1port, y1port, x2port, y2port;
	int beamx, beamy;

	switch( namcos2_gametype )
	{
	case NAMCOS2_GOLLY_GHOST:
		x1port = 0;
		y1port = 1;
		x2port = 2;
		y2port = 3;
		break;
	case NAMCOS2_LUCKY_AND_WILD:
		x1port = 4;
		y1port = 2;
		x2port = 3;
		y2port = 1;
		break;
	case NAMCOS2_STEEL_GUNNER_2:
		x1port = 4;
		x2port = 5;
		y1port = 6;
		y2port = 7;
		break;
	default:
		return;
	}

	beamx = readinputport(2+x1port)*bitmap->width/256;
	beamy = readinputport(2+y1port)*bitmap->height/256;
	draw_crosshair( 1, bitmap, beamx, beamy, cliprect );

	beamx = readinputport(2+x2port)*bitmap->width/256;
	beamy = readinputport(2+y2port)*bitmap->height/256;
	draw_crosshair( 2, bitmap, beamx, beamy, cliprect );
}

/**************************************************************************/

VIDEO_START( namcos2 )
{
	if( CreateTilemaps()==0 )
	{
		tilemap_roz = tilemap_create(get_tile_info_roz,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,256,256);
		if( tilemap_roz )
		{
			tilemap_set_transparent_pen(tilemap_roz,0xff);
			DrawSpriteInit();
			return 0;
		}
	}
	return -1;
}

VIDEO_UPDATE( namcos2_default )
{
	int pri;

	UpdatePalette();

	fillbitmap(bitmap,get_black_pen(),cliprect);

	/* enable ROZ layer only if it has priority > 0 */
	tilemap_set_enable(tilemap_roz,(namcos2_gfx_ctrl & 0x7000) ? 1 : 0);

	for(pri=0;pri<16;pri++)
	{
		DrawTilemaps( bitmap, cliprect, pri );
		if(pri>=1 && ((namcos2_gfx_ctrl & 0x7000) >> 12)==pri)
		{
			DrawROZ(bitmap,cliprect);
		}
		DrawSpritesDefault( bitmap,cliprect,pri, 0x0007 );
	}
	DrawCrossshair( bitmap,cliprect );
}

/**************************************************************************/

VIDEO_START( finallap )
{
	if( CreateTilemaps()==0 )
	{
		DrawSpriteInit();
		namco_road_init(3);
		return 0;
	}
	return -1;
}

VIDEO_UPDATE( finallap )
{
	int pri;

	UpdatePalette();

	fillbitmap(bitmap,Machine->pens[0],cliprect);

	for(pri=0;pri<16;pri++)
	{
		DrawTilemaps( bitmap, cliprect, pri );
		namco_road_draw( bitmap,cliprect,pri );
		DrawSpritesDefault( bitmap,cliprect,pri,0x000f );
	}
}

/**************************************************************************/

VIDEO_START( luckywld )
{
	if( CreateTilemaps()==0 )
	{
		namco_obj_init( 0, 0x0, NULL );
		if( namcos2_gametype==NAMCOS2_LUCKY_AND_WILD )
		{
			namco_roz_init( 1, REGION_GFX5 );
		}
		if( namcos2_gametype!=NAMCOS2_STEEL_GUNNER_2 )
		{
			namco_road_init(3);
		}
		return 0;
	}
	return -1;
}

VIDEO_UPDATE( luckywld )
{
	int pri;

	UpdatePalette();

	fillbitmap(bitmap,Machine->pens[0],cliprect);

	for(pri=0;pri<16;pri++)
	{
		DrawTilemaps( bitmap, cliprect, pri );
		namco_road_draw( bitmap,cliprect,pri );
		if( namcos2_gametype==NAMCOS2_LUCKY_AND_WILD )
		{
			namco_roz_draw( bitmap, cliprect, pri );
		}
		namco_obj_draw( bitmap, cliprect, pri );
	}
	DrawCrossshair( bitmap,cliprect );
}

/**************************************************************************/

VIDEO_START( sgunner )
{
	if( CreateTilemaps()==0 )
	{
		namco_obj_init( 0, 0x0, NULL );
		return 0;
	}
	return -1;
}

VIDEO_UPDATE( sgunner )
{
	int pri;

	UpdatePalette();

	fillbitmap(bitmap,Machine->pens[0],cliprect);

	for(pri=0;pri<16;pri++)
	{
		DrawTilemaps( bitmap, cliprect, pri );
		namco_obj_draw( bitmap, cliprect, pri );
	}
	DrawCrossshair( bitmap,cliprect );
}

/**************************************************************************/

VIDEO_START( metlhawk )
{
	if( CreateTilemaps()==0 )
	{
		namco_roz_init( 1, REGION_GFX5 );
		return 0;
	}
	return -1;
}

VIDEO_UPDATE( metlhawk )
{
	int pri;

	UpdatePalette();

	fillbitmap(bitmap,Machine->pens[0],cliprect);

	for(pri=0;pri<16;pri++)
	{
		DrawTilemaps( bitmap, cliprect, pri );
		namco_roz_draw( bitmap, cliprect, pri );
		DrawSpritesMetalHawk( bitmap,cliprect,pri );
	}
}
