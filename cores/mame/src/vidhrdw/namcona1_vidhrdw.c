/*	Namco System NA1/2 Video Hardware */

#include "vidhrdw/generic.h"
#include "namcona1.h"

#define NAMCONA1_NUM_TILEMAPS 4

/* public variables */
data16_t *namcona1_vreg;
data16_t *namcona1_scroll;
data16_t *namcona1_workram;
data16_t *namcona1_sparevram;

/* private variables */
static char *dirtychar;
static char dirtygfx;
static data16_t *shaperam;
static data16_t *cgram;
static struct tilemap *tilemap[NAMCONA1_NUM_TILEMAPS];
static int tilemap_palette_bank[NAMCONA1_NUM_TILEMAPS];
static int palette_is_dirty;

static void tilemap_get_info(
	int tile_index,const data16_t *tilemap_videoram,int tilemap_color )
{
#ifndef MSB_FIRST
	data16_t *source;
#endif
	static UINT8 mask_data[8];

	int data = tilemap_videoram[tile_index];
	int tile = data&0xfff;

	if( data&0x8000 )
	{
		SET_TILE_INFO( 0,tile,tilemap_color,TILE_IGNORE_TRANSPARENCY );
	}
	else
	{
		SET_TILE_INFO( 0,tile,tilemap_color,0 );
#ifndef MSB_FIRST
		source = shaperam+4*tile;
		mask_data[0] = source[0]>>8;
		mask_data[1] = source[0]&0xff;
		mask_data[2] = source[1]>>8;
		mask_data[3] = source[1]&0xff;
		mask_data[4] = source[2]>>8;
		mask_data[5] = source[2]&0xff;
		mask_data[6] = source[3]>>8;
		mask_data[7] = source[3]&0xff;
		tile_info.mask_data = mask_data;
#else
		tile_info.mask_data = (UINT8 *)(shaperam+4*tile);
#endif
	}
} /* tilemap_get_info */

static void tilemap_get_info0(int tile_index){ tilemap_get_info(tile_index,0*0x1000+videoram16,tilemap_palette_bank[0]); }
static void tilemap_get_info1(int tile_index){ tilemap_get_info(tile_index,1*0x1000+videoram16,tilemap_palette_bank[1]); }
static void tilemap_get_info2(int tile_index){ tilemap_get_info(tile_index,2*0x1000+videoram16,tilemap_palette_bank[2]); }
static void tilemap_get_info3(int tile_index){ tilemap_get_info(tile_index,3*0x1000+videoram16,tilemap_palette_bank[3]); }

/*************************************************************************/

WRITE16_HANDLER( namcona1_videoram_w )
{
	COMBINE_DATA( &videoram16[offset] );
	tilemap_mark_tile_dirty( tilemap[offset/0x1000], offset&0xfff );
} /* namcona1_videoram_w */

READ16_HANDLER( namcona1_videoram_r )
{
	return videoram16[offset];
} /* namcona1_videoram_r */

/*************************************************************************/

static void
UpdatePalette( int offset )
{
	data16_t color;
	int r,g,b;

	color = paletteram16[offset];

	/* -RRRRRGGGGGBBBBB */
	r = (color>>10)&0x1f;
	g = (color>>5)&0x1f;
	b = (color)&0x1f;

	r = (r<<3)|(r>>2);
	g = (g<<3)|(g>>2);
	b = (b<<3)|(b>>2);

	palette_set_color( offset, r,g,b );
} /* namcona1_paletteram_w */

READ16_HANDLER( namcona1_paletteram_r )
{
	return paletteram16[offset];
} /* namcona1_paletteram_r */

WRITE16_HANDLER( namcona1_paletteram_w )
{
	COMBINE_DATA( &paletteram16[offset] );
	if( namcona1_vreg[0x8e/2] )
	{
		/* graphics enabled; update palette immediately */
		UpdatePalette( offset );
	}
	else
	{
		palette_is_dirty = 1;
	}
}

/*************************************************************************/

static struct GfxLayout shape_layout =
{
	8,8,
	0x1000,
	1,
	{ 0 },
	{ 0,1,2,3,4,5,6,7 },
#ifdef MSB_FIRST
	{ 8*0,8*1,8*2,8*3,8*4,8*5,8*6,8*7 },
#else
	{ 8*1,8*0,8*3,8*2,8*5,8*4,8*7,8*6 },
#endif
	8*8
}; /* shape_layout */

static struct GfxLayout cg_layout =
{
	8,8,
	0x1000,
	8, /* 8BPP */
	{ 0,1,2,3,4,5,6,7 },
#ifdef MSB_FIRST
	{ 8*0,8*1,8*2,8*3,8*4,8*5,8*6,8*7 },
#else
	{ 8*1,8*0,8*3,8*2,8*5,8*4,8*7,8*6 },
#endif
	{ 64*0,64*1,64*2,64*3,64*4,64*5,64*6,64*7 },
	64*8
}; /* cg_layout */

READ16_HANDLER( namcona1_gfxram_r )
{
	data16_t type = namcona1_vreg[0x0c/2];
	if( type == 0x03 )
	{
		if( offset<0x4000 )
		{
			return shaperam[offset];
		}
	}
	else if( type == 0x02 )
	{
		return cgram[offset];
	}
	return 0x0000;
} /* namcona1_gfxram_r */

WRITE16_HANDLER( namcona1_gfxram_w )
{
	data16_t type = namcona1_vreg[0x0c/2];
	data16_t old_word;

	if( type == 0x03 )
	{
		if( offset<0x4000 )
		{
			old_word = shaperam[offset];
			COMBINE_DATA( &shaperam[offset] );
			if( shaperam[offset]!=old_word )
			{
				dirtygfx = 1;
				dirtychar[offset/4] = 1;
			}
		}
	}
	else if( type == 0x02 )
	{
		old_word = cgram[offset];
		COMBINE_DATA( &cgram[offset] );
		if( cgram[offset]!=old_word )
		{
			dirtygfx = 1;
			dirtychar[offset/0x20] = 1;
		}
	}
} /* namcona1_gfxram_w */

static void update_gfx( void )
{
	const data16_t *pSource = videoram16;
	int page;
	int i;

	if( dirtygfx )
	{
		for( page = 0; page<4; page++ )
		{
			for( i=0; i<0x1000; i++ )
			{
				if( dirtychar[*pSource++ & 0xfff] )
				{
					tilemap_mark_tile_dirty( tilemap[page], i );
				}
			}
		}
		for( i = 0;i < 0x1000;i++ )
		{
			if( dirtychar[i] )
			{
				dirtychar[i] = 0;
				decodechar(Machine->gfx[0],i,(UINT8 *)cgram,&cg_layout);
				decodechar(Machine->gfx[1],i,(UINT8 *)shaperam,&shape_layout);
			}
		}
		dirtygfx = 0;
	}
} /* update_gfx */

VIDEO_START( namcona1 )
{
	int i;
	struct GfxElement *gfx0,*gfx1;
	static void (*get_info[4])(int tile_index) =
	{ tilemap_get_info0, tilemap_get_info1, tilemap_get_info2, tilemap_get_info3 };

	for( i=0; i<NAMCONA1_NUM_TILEMAPS; i++ )
	{
		tilemap[i] = tilemap_create(
			get_info[i],
			tilemap_scan_rows,
			TILEMAP_BITMASK,8,8,64,64 );

		if( tilemap[i]==NULL ) return -1;
		tilemap_palette_bank[i] = -1;
	}

	shaperam		= auto_malloc( 0x1000*4*2 );
	cgram			= auto_malloc( 0x1000*0x20*2 );
	dirtychar		= auto_malloc( 0x1000 );

	if( shaperam && cgram && dirtychar )
	{
		gfx0 = decodegfx( (UINT8 *)cgram,&cg_layout );
		gfx1 = decodegfx( (UINT8 *)shaperam,&shape_layout );
		if( gfx0 && gfx1 )
		{
			gfx0->colortable = Machine->remapped_colortable;
			gfx0->total_colors = Machine->drv->total_colors/256;
			Machine->gfx[0] = gfx0;

			gfx1->colortable = Machine->remapped_colortable;
			gfx1->total_colors = Machine->drv->total_colors/2;
			Machine->gfx[1] = gfx1;

			return 0;
		}
	}
	return -1; /* error */
} /* namcona1_vh_start */

/*************************************************************************/

static void pdraw_masked_tile(
		struct mame_bitmap *bitmap,
		unsigned code,
		int color,
		int sx, int sy,
		int flipx, int flipy,
		int priority,
		int bShadow )
{
	const struct GfxElement *gfx,*mask;
	const pen_t *paldata;
	data8_t *gfx_addr;
	int gfx_pitch;
	data8_t *mask_addr;
	int mask_pitch;
	int x,y,temp;

	if( Machine->orientation & ORIENTATION_SWAP_XY )
	{
		temp = sx; sx = sy; sy = temp;
		temp = flipx; flipx = flipy; flipy = temp;
	}
	if( Machine->orientation & ORIENTATION_FLIP_X )
	{
		sx = bitmap->width - 1 - sx;
		flipx = !flipx;
	}
	if( Machine->orientation & ORIENTATION_FLIP_Y )
	{
		sy = bitmap->height - 1 - sy;
		flipy = !flipy;
	}

	/*
	 *	custom blitter for drawing a masked 8x8x8BPP tile
	 *	- doesn't yet handle screen orientation (needed particularly for F/A, a vertical game)
	 *	- assumes there is an 8 pixel overdraw region on the screen bitmap for clipping
	 */
	if( sx > -8 &&
		sy > -8 &&
		sx < bitmap->width &&
		sy < bitmap->height ) /* all-or-nothing clip */
	{
		gfx = Machine->gfx[0];
		mask = Machine->gfx[1];
		code %= gfx->total_elements;
		color %= gfx->total_colors;
		paldata = &gfx->colortable[gfx->color_granularity * color];
		gfx_addr = gfx->gfxdata + code * gfx->char_modulo;
		gfx_pitch = gfx->line_modulo;
		mask_addr = mask->gfxdata + code * mask->char_modulo;
		mask_pitch = mask->line_modulo;

		/* The way we render shadows makes some Emeralda text invisible.
		 * The relevant text is composed of sprites with the shadow bit set,
		 * displayed over a black backdrop.
		 */
		if( bShadow && namcona1_gametype!=NAMCO_EMERALDA )
		{
			for( y=0; y<8; y++ )
			{
				int ypos = sy+(flipy?7-y:y);
				data8_t *pri = (data8_t *)priority_bitmap->line[ypos];
				UINT16 *dest = (UINT16 *)bitmap->line[ypos];
				if( flipx )
				{
					dest += sx+7;
					pri += sx+7;
					for( x=0; x<8; x++ )
					{
						if( mask_addr[x] )
						{ /* sprite pixel is opaque */
							if( priority>=pri[-x] )
							{
								dest[-x] |= 0x1000;
							}
							pri[-x] = 0xff;
						}
					}
				}
				else
				{
					dest += sx;
					pri += sx;
					for( x=0; x<8; x++ )
					{
						if( mask_addr[x] )
						{ /* sprite pixel is opaque */
							if( priority>=pri[x] )
							{
								dest[x] |= 0x1000;
							}
							pri[x] = 0xff;
						}
					}
				}
				mask_addr += mask_pitch;
			}
		} /* render translucent sprite */
		else
		{
			for( y=0; y<8; y++ )
			{
				int ypos = sy+(flipy?7-y:y);
				data8_t *pri = (data8_t *)priority_bitmap->line[ypos];
				UINT16 *dest = (UINT16 *)bitmap->line[ypos];
				if( flipx )
				{
					dest += sx+7;
					pri += sx+7;
					for( x=0; x<8; x++ )
					{
						if( mask_addr[x] )
						{ /* sprite pixel is opaque */
							if( priority>=pri[-x] )
							{
								dest[-x] = paldata[gfx_addr[x]];
							}
							pri[-x] = 0xff;
						}
					}
				}
				else
				{
					dest += sx;
					pri += sx;
					for( x=0; x<8; x++ )
					{
						if( mask_addr[x] )
						{ /* sprite pixel is opaque */
							if( priority>=pri[x] )
							{
								dest[x] = paldata[gfx_addr[x]];
							}
							pri[x] = 0xff;
						}
					} /* next x */
				} /* !flipx */
				gfx_addr += gfx_pitch;
				mask_addr += mask_pitch;
			} /* next y */
		} /* render normal sprite (no translucency) */
	}
} /* pdraw_masked_tile */

static void pdraw_opaque_tile(
		struct mame_bitmap *bitmap,
		unsigned code,
		int color,
		int sx, int sy,
		int flipx, int flipy,
		int priority,
		int bShadow )
{
	const struct GfxElement *gfx;
	const pen_t *paldata;
	data8_t *gfx_addr;
	int gfx_pitch;
	int x,y,temp;
	int ypos;
	data8_t *pri;
	UINT16 *dest;

	if( Machine->orientation & ORIENTATION_SWAP_XY )
	{
		temp = sx; sx = sy; sy = temp;
		temp = flipx; flipx = flipy; flipy = temp;
	}
	if( Machine->orientation & ORIENTATION_FLIP_X )
	{
		sx = bitmap->width - 1 - sx;
		flipx = !flipx;
	}
	if( Machine->orientation & ORIENTATION_FLIP_Y )
	{
		sy = bitmap->height - 1 - sy;
		flipy = !flipy;
	}

	if( sx > -8 &&
		sy > -8 &&
		sx < bitmap->width &&
		sy < bitmap->height ) /* all-or-nothing clip */
	{
		gfx = Machine->gfx[0];
		code %= gfx->total_elements;
		color %= gfx->total_colors;
		paldata = &gfx->colortable[gfx->color_granularity * color];
		gfx_addr = gfx->gfxdata + code * gfx->char_modulo;
		gfx_pitch = gfx->line_modulo;

		for( y=0; y<8; y++ )
		{
			ypos = sy+(flipy?7-y:y);
			pri = (data8_t *)priority_bitmap->line[ypos];
			dest = (UINT16 *)bitmap->line[ypos];
			if( flipx )
			{
				dest += sx+7;
				pri += sx+7;
				for( x=0; x<8; x++ )
				{
					if( priority>=pri[-x] )
					{
						dest[-x] = paldata[gfx_addr[x]];
					}
					pri[-x] = 0xff;
				}
			}
			else
			{
				dest += sx;
				pri += sx;
				for( x=0; x<8; x++ )
				{
					if( priority>=pri[x] )
					{
						dest[x] = paldata[gfx_addr[x]];
					}
					pri[x] = 0xff;
				} /* next x */
			} /* !flipx */
			gfx_addr += gfx_pitch;
		} /* next y */
	}
} /* pdraw_opaque_tile */

static const data8_t pri_mask[8] = { 0x00,0x01,0x03,0x07,0x0f,0x1f,0x3f,0x7f };

static void draw_sprites( struct mame_bitmap *bitmap )
{
	int which;
	const data16_t *source = spriteram16;
	void (*drawtile)(
		struct mame_bitmap *,
		unsigned code,
		int color,
		int sx, int sy,
		int flipx, int flipy,
		int priority,
		int bShadow );
	data16_t sprite_control;
	data16_t ypos,tile,color,xpos;
	int priority;
	int width,height;
	int flipy,flipx;
	int row,col;
	int sx,sy;

	sprite_control = namcona1_vreg[0x22/2];
	if( sprite_control&1 ) source += 0x400; /* alternate spriteram bank */

	for( which=0; which<0x100; which++ )
	{ /* max 256 sprites */
		ypos	= source[0];	/* FHHH---E YYYYYYYY	flipy, height, ypos */
		tile	= source[1];	/* O???TTTT TTTTTTTT	opaque tile */
		color	= source[2];	/* FSWW???B CCCC?PPP	flipx, shadow, width, color, pri*/
		xpos	= source[3];	/* -------X XXXXXXXX	xpos */

		priority = pri_mask[color&0x7];
		width = ((color>>12)&0x3)+1;
		height = ((ypos>>12)&0x7)+1;
		flipy = ypos&0x8000;
		flipx = color&0x8000;

		if( (tile&0x8000)==0 )
		{
			drawtile = pdraw_masked_tile;
		}
		else
		{
			drawtile = pdraw_opaque_tile;
		}
		for( row=0; row<height; row++ )
		{
			sy = (ypos&0x1ff)-30+32;
			if( flipy )
			{
				sy += (height-1-row)*8;
			}
			else
			{
				sy += row*8;
			}
			for( col=0; col<width; col++ )
			{
				sx = (xpos&0x1ff)-10;
				if( flipx )
				{
					sx += (width-1-col)*8;
				}
				else
				{
					sx+=col*8;
				}
				drawtile(
					bitmap,
					tile + row*64+col,
					(color>>4)&0xf,
					((sx+16)&0x1ff)-8,
					((sy+8)&0x1ff)-8,
					flipx,flipy,
					priority,
					color&0x4000 /* shadow */ );
			} /* next col */
		} /* next row */
		source += 4;
	}
} /* draw_sprites */

static void draw_pixel_line( UINT16 *pDest, UINT8 *pPri, data16_t *pSource, const pen_t *paldata )
{
	int x;
	data16_t data;

	memset( pPri, 0xff, 38*8 );
	for( x=0; x<38*8; x+=2 )
	{
		data = *pSource++;
		pDest[x] = paldata[data>>8];
		pDest[x+1] = paldata[data&0xff];
	} /* next x */
} /* draw_pixel_line */

static void draw_background( struct mame_bitmap *bitmap, const struct rectangle *cliprect, int which, int primask )
{
	/*			scrollx	lineselect
	 *	tmap0	ffe000	ffe200
	 *	tmap1	ffe400	ffe600
	 *	tmap2	ffe800	ffea00
	 *	tmap3	ffec00	ffee00
	 */
	int xadjust = 0x3a - which*2;
	const data16_t *scroll = namcona1_scroll+0x200*which;
	int line;
	data16_t xdata, ydata;
	int scrollx, scrolly;
	struct rectangle clip;
	const pen_t *paldata;
	struct GfxElement *pGfx;

	pGfx = Machine->gfx[0];
	paldata = &pGfx->colortable[pGfx->color_granularity * tilemap_palette_bank[which]];

	/* draw one scanline at a time */
	clip.min_x = cliprect->min_x;
	clip.max_x = cliprect->max_x;
	scrollx = 0;
	scrolly = 0;
	for( line=0; line<256; line++ )
	{
		clip.min_y = line;
		clip.max_y = line;
		xdata = scroll[line];
		if( xdata )
		{
			/* screenwise linescroll */
			scrollx = xadjust+xdata;
		}
		ydata = scroll[line+0x100];
		if( ydata&0x4000 )
		{
			/* line select: dword offset from 0xff000 or tilemap source line */
			scrolly = (ydata - line)&0x1ff;
		}

		if (line >= cliprect->min_y && line <= cliprect->max_y)
		{
			if( xdata == 0xc001 )
			{
				/* This is a simplification, but produces the correct behavior for the only game that uses this
				 * feature, Numan Athletics.
				 */
				draw_pixel_line(
					bitmap->line[line],
					priority_bitmap->line[line],
					namcona1_sparevram + (ydata-0x4000) + 25,
					paldata );
			}
			else
			{
				tilemap_set_scrollx( tilemap[which], 0, scrollx );
				tilemap_set_scrolly( tilemap[which], 0, scrolly );
				tilemap_draw( bitmap, &clip, tilemap[which], 0, primask );
			}
		}
	}
} /* draw_background */

VIDEO_UPDATE( namcona1 )
{
	int which;
	int priority;
	/* int flipscreen = namcona1_vreg[0x98/2]; (TBA) */

	if( namcona1_vreg[0x8e/2] )
	{ /* gfx enabled */
		if( palette_is_dirty )
		{
			/* palette updates are delayed when graphics are disabled */
			for( which=0; which<0x1000; which++ )
			{
				UpdatePalette( which );
			}
			palette_is_dirty = 0;
		}
		update_gfx();
		for( which=0; which<NAMCONA1_NUM_TILEMAPS; which++ )
		{
			static int tilemap_color;
			tilemap_color = namcona1_vreg[0x58+(which&3)]&0xf;
			if( tilemap_color!=tilemap_palette_bank[which] )
			{
				tilemap_mark_all_tiles_dirty( tilemap[which] );
				tilemap_palette_bank[which] = tilemap_color;
			}
		} /* next tilemap */
		fillbitmap( priority_bitmap,0,cliprect );
		fillbitmap( bitmap,0,cliprect );
		for( priority = 0; priority<8; priority++ )
		{
			for( which=NAMCONA1_NUM_TILEMAPS-1; which>=0; which-- )
			{
				if( (namcona1_vreg[0x50+which]&0x7) == priority )
				{
					draw_background( bitmap,cliprect,which,pri_mask[priority] );
				}
			} /* next tilemap */
		} /* next priority level */
		draw_sprites( bitmap );
	} /* gfx enabled */
} /* namcona1_vh_screenrefresh */
