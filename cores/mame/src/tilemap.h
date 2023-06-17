/* tilemap.h */

#ifndef TILEMAP_H
#define TILEMAP_H

struct tilemap; /* appease compiler */

#define ALL_TILEMAPS	0
/* ALL_TILEMAPS may be used with:
	tilemap_set_flip, tilemap_mark_all_tiles_dirty
*/

#define TILEMAP_OPAQUE					0x00
#define TILEMAP_TRANSPARENT				0x01
#define TILEMAP_SPLIT					0x02
#define TILEMAP_BITMASK					0x04
#define TILEMAP_TRANSPARENT_COLOR		0x08

/* Set transparency_pen to a mask.  pen&mask determines whether each pixel is in front or back half */
#define TILEMAP_SPLIT_PENBIT			0x10
/*
	TILEMAP_SPLIT should be used if the pixels from a single tile
	can appear in more than one plane.

	TILEMAP_BITMASK is used by Namco System1, Namco System2, NamcoNA1/2, Namco NB1
*/

#define TILEMAP_IGNORE_TRANSPARENCY		0x10
#define TILEMAP_BACK					0x20
#define TILEMAP_FRONT					0x40
#define TILEMAP_ALPHA					0x80

/*
	when rendering a split layer, pass TILEMAP_FRONT or TILEMAP_BACK or'd with the
	tile_priority value to specify the part to draw.

	when rendering a layer in alpha mode, the priority parameter
	becomes the alpha parameter (0..255).  Split mode is still
	available in alpha mode, ignore_transparency isn't.
*/

extern struct tile_info
{
	/*
		you must set tile_info.pen_data, tile_info.pal_data and tile_info.pen_usage
		in the callback.  You can use the SET_TILE_INFO() macro below to do this.
		tile_info.flags and tile_info.priority will be automatically preset to 0,
		games that don't need them don't need to explicitly set them to 0
	*/
	const UINT8 *pen_data;
	const pen_t *pal_data;
	UINT32 flags;
	int skip;
	UINT32 tile_number;		/* needed for tilemap_mark_gfxdata_dirty */
	UINT32 pen_usage;		/* TBR */
	UINT32 priority;		/* tile priority */
	UINT8 *mask_data;		/* for TILEMAP_BITMASK */
	void *user_data;		/* user-supplied tilemap-wide pointer */
} tile_info;

#define SET_TILE_INFO(GFX,CODE,COLOR,FLAGS) { \
	const struct GfxElement *gfx = Machine->gfx[(GFX)]; \
	int _code = (CODE) % gfx->total_elements; \
	tile_info.tile_number = _code; \
	tile_info.pen_data = gfx->gfxdata + _code*gfx->char_modulo; \
	tile_info.pal_data = &gfx->colortable[gfx->color_granularity * (COLOR)]; \
	tile_info.pen_usage = gfx->pen_usage?gfx->pen_usage[_code]:0; \
	tile_info.flags = FLAGS; \
	if (gfx->flags & GFX_PACKED) tile_info.flags |= TILE_4BPP; \
}

/* tile flags, set by get_tile_info callback */
/* TILE_IGNORE_TRANSPARENCY is used if you need an opaque tile in a transparent layer. */
#define TILE_FLIPX					0x01
#define TILE_FLIPY					0x02
#define TILE_IGNORE_TRANSPARENCY	0x08
#define TILE_4BPP					0x10
/*		TILE_SPLIT					0x60 */

/* TILE_SPLIT is for use with TILEMAP_SPLIT layers.  It selects transparency type. */
#define TILE_SPLIT_OFFSET			5
#define TILE_SPLIT(T)				((T)<<TILE_SPLIT_OFFSET)

#define TILE_FLIPYX(YX)				(YX)
#define TILE_FLIPXY(XY)				((((XY)>>1)|((XY)<<1))&3)
/*
	TILE_FLIPYX is a shortcut that can be used by approx 80% of games,
	since yflip frequently occurs one bit higher than xflip within a
	tile attributes byte.
*/

#define TILE_LINE_DISABLED 0x80000000

extern struct mame_bitmap *priority_bitmap;

/* don't call these from drivers - they are called from mame.c */
int tilemap_init( void );
void tilemap_close( void );
void tilemap_dispose( struct tilemap *tilemap );

struct tilemap *tilemap_create(
	void (*tile_get_info)( int memory_offset ),
	UINT32 (*get_memory_offset)( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows ),
	int type,
	int tile_width, int tile_height,
	int num_cols, int num_rows );

void tilemap_set_transparent_pen( struct tilemap *tilemap, int pen );
void tilemap_set_transmask( struct tilemap *tilemap, int which, UINT32 fgmask, UINT32 bgmask );
void tilemap_set_depth( struct tilemap *tilemap, int tile_depth, int tile_granularity );

void tilemap_mark_tile_dirty( struct tilemap *tilemap, int memory_offset );
void tilemap_mark_all_tiles_dirty( struct tilemap *tilemap );
void tilemap_mark_gfxdata_dirty( struct tilemap *tilemap, UINT8 *dirty_array ); /* TBA */

void tilemap_set_scroll_rows( struct tilemap *tilemap, int scroll_rows ); /* default: 1 */
void tilemap_set_scrolldx( struct tilemap *tilemap, int dx, int dx_if_flipped );
void tilemap_set_scrollx( struct tilemap *tilemap, int row, int value );

void tilemap_set_scroll_cols( struct tilemap *tilemap, int scroll_cols ); /* default: 1 */
void tilemap_set_scrolldy( struct tilemap *tilemap, int dy, int dy_if_flipped );
void tilemap_set_scrolly( struct tilemap *tilemap, int col, int value );

void tilemap_set_palette_offset( struct tilemap *tilemap, int offset );
void tilemap_set_user_data( struct tilemap *tilemap, void *user_data );

#define TILEMAP_FLIPX 0x1
#define TILEMAP_FLIPY 0x2
void tilemap_set_flip( struct tilemap *tilemap, int attributes );
void tilemap_set_enable( struct tilemap *tilemap, int enable );

void tilemap_draw( struct mame_bitmap *dest, const struct rectangle *cliprect, struct tilemap *tilemap, UINT32 flags, UINT32 priority );

void tilemap_draw_roz(struct mame_bitmap *dest,const struct rectangle *cliprect,struct tilemap *tilemap,
		UINT32 startx,UINT32 starty,int incxx,int incxy,int incyx,int incyy,
		int wraparound,
		UINT32 flags, UINT32 priority );

/* ----xxxx tile priority
 * ---x---- opaque in foreground
 * --x----- opaque in background
 * -x------ reserved
 * x------- tile-is-dirty
 */
#define TILE_FLAG_TILE_PRIORITY	(0x0f)
#define TILE_FLAG_FG_OPAQUE		(0x10)
#define TILE_FLAG_BG_OPAQUE		(0x20)

struct mame_bitmap *tilemap_get_pixmap( struct tilemap * tilemap );
struct mame_bitmap *tilemap_get_transparency_bitmap( struct tilemap * tilemap );
UINT8 *tilemap_get_transparency_data( struct tilemap * tilemap );  /***/

/*********************************************************************/

UINT32 tilemap_scan_cols( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows );
UINT32 tilemap_scan_cols_flip_x( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows );
UINT32 tilemap_scan_cols_flip_y( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows );
UINT32 tilemap_scan_cols_flip_xy( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows );

UINT32 tilemap_scan_rows( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows );
UINT32 tilemap_scan_rows_flip_x( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows );
UINT32 tilemap_scan_rows_flip_y( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows );
UINT32 tilemap_scan_rows_flip_xy( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows );

/* For showcharset()'s sake */
UINT32 tilemap_count( void );
void tilemap_nb_size( UINT32 number, UINT32 *width, UINT32 *height );
void tilemap_nb_draw( struct mame_bitmap *dest, UINT32 number, UINT32 scrollx, UINT32 scrolly );

#endif
