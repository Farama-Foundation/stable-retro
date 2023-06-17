/* tilemap.c

	When the videoram for a tile changes, call tilemap_mark_tile_dirty
	with the appropriate memory offset.

	In the video driver, follow these steps:

	1)	Set each tilemap's scroll registers.

	2)	Call tilemap_draw to draw the tilemaps to the screen, from back to front.

	Notes:
	-	You can currently configure a tilemap as xscroll + scrolling columns or
		yscroll + scrolling rows, but not both types of scrolling simultaneously.
*/

#if !defined(DECLARE) && !defined(TRANSP)

#include "driver.h"
#include "osd_cpu.h"
#include "tilemap.h"
#include "state.h"

#define SWAP(X,Y) { UINT32 temp=X; X=Y; Y=temp; }
#define MAX_TILESIZE 64

#define TILE_FLAG_DIRTY	(0x80)

typedef enum { eWHOLLY_TRANSPARENT, eWHOLLY_OPAQUE, eMASKED } trans_t;

typedef void (*tilemap_draw_func)( struct tilemap *tilemap, int xpos, int ypos, int mask, int value );

struct tilemap
{
	UINT32 (*get_memory_offset)( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows );
	int *memory_offset_to_cached_indx;
	UINT32 *cached_indx_to_memory_offset;
	int logical_flip_to_cached_flip[4];

	/* callback to interpret video RAM for the tilemap */
	void (*tile_get_info)( int memory_offset );
	void *user_data;

	UINT32 max_memory_offset;
	UINT32 num_tiles;
	UINT32 num_pens;

	UINT32 num_logical_rows, num_logical_cols;
	UINT32 num_cached_rows, num_cached_cols;

	UINT32 logical_tile_width, logical_tile_height;
	UINT32 cached_tile_width, cached_tile_height;

	UINT32 cached_width, cached_height;

	int dx, dx_if_flipped;
	int dy, dy_if_flipped;
	int scrollx_delta, scrolly_delta;

	int enable;
	int attributes;

	int type;
	int transparent_pen;
	UINT32 fgmask[4], bgmask[4]; /* for TILEMAP_SPLIT */

	UINT32 *pPenToPixel[4];

	UINT8 (*draw_tile)( struct tilemap *tilemap, UINT32 col, UINT32 row, UINT32 flags );

	int cached_scroll_rows, cached_scroll_cols;
	int *cached_rowscroll, *cached_colscroll;

	int logical_scroll_rows, logical_scroll_cols;
	int *logical_rowscroll, *logical_colscroll;

	int orientation;
	int palette_offset;

	UINT16 tile_depth, tile_granularity;
	UINT8 *tile_dirty_map;
	UINT8 all_tiles_dirty;
	UINT8 all_tiles_clean;

	/* cached color data */
	struct mame_bitmap *pixmap;
	UINT32 pixmap_pitch_line;
	UINT32 pixmap_pitch_row;

	struct mame_bitmap *transparency_bitmap;
	UINT32 transparency_bitmap_pitch_line;
	UINT32 transparency_bitmap_pitch_row;
	UINT8 *transparency_data, **transparency_data_row;

	struct tilemap *next; /* resource tracking */
};

struct mame_bitmap *		priority_bitmap;
UINT32					priority_bitmap_pitch_line;
UINT32					priority_bitmap_pitch_row;

static struct tilemap *	first_tilemap; /* resource tracking */
static UINT32			screen_width, screen_height;
struct tile_info		tile_info;

static UINT32 g_mask32[32];

typedef void (*blitmask_t)( void *dest, const void *source, const UINT8 *pMask, int mask, int value, int count, UINT8 *pri, UINT32 pcode );
typedef void (*blitopaque_t)( void *dest, const void *source, int count, UINT8 *pri, UINT32 pcode );

/* the following parameters are constant across tilemap_draw calls */
static struct
{
	blitmask_t draw_masked;
	blitopaque_t draw_opaque;
	int clip_left, clip_top, clip_right, clip_bottom;
	UINT32 tilemap_priority_code;
	struct mame_bitmap *	screen_bitmap;
	UINT32				screen_bitmap_pitch_line;
	UINT32				screen_bitmap_pitch_row;
} blit;

/***********************************************************************************/

static int PenToPixel_Init( struct tilemap *tilemap );
static void PenToPixel_Term( struct tilemap *tilemap );
static int mappings_create( struct tilemap *tilemap );
static void mappings_dispose( struct tilemap *tilemap );
static void mappings_update( struct tilemap *tilemap );
static void recalculate_scroll( struct tilemap *tilemap );

static void install_draw_handlers( struct tilemap *tilemap );
static void tilemap_reset(void);

static void update_tile_info( struct tilemap *tilemap, UINT32 cached_indx, UINT32 cached_col, UINT32 cached_row );

/***********************************************************************************/

static int PenToPixel_Init( struct tilemap *tilemap )
{
	/*
		Construct a table for all tile orientations in advance.
		This simplifies drawing tiles and masks tremendously.
		If performance is an issue, we can always (re)introduce
		customized code for each case and forgo tables.
	*/
	int i,x,y,tx,ty;
	UINT32 *pPenToPixel;
	int lError;

	lError = 0;
	for( i=0; i<4; i++ )
	{
		pPenToPixel = malloc( tilemap->num_pens*sizeof(UINT32) );
		if( pPenToPixel==NULL )
		{
			lError = 1;
		}
		else
		{
			tilemap->pPenToPixel[i] = pPenToPixel;
			for( ty=0; ty<tilemap->cached_tile_height; ty++ )
			{
				for( tx=0; tx<tilemap->cached_tile_width; tx++ )
				{
					x = tx;
					y = ty;
					if( i&TILE_FLIPX ) x = tilemap->cached_tile_width-1-x;
					if( i&TILE_FLIPY ) y = tilemap->cached_tile_height-1-y;
					*pPenToPixel++ = x+y*MAX_TILESIZE;
				}
			}
		}
	}
	return lError;
}

static void PenToPixel_Term( struct tilemap *tilemap )
{
	int i;
	for( i=0; i<4; i++ )
	{
		free( tilemap->pPenToPixel[i] );
	}
}

static void InitMask32(void)
{
	int i;

	for (i=0;i<16;i++)
	{
		UINT32 p1 = (i&1) ? 0xFFFF : 0;
		UINT32 p2 = (i&2) ? 0xFFFF : 0;
		UINT32 p3 = (i&4) ? 0xFFFF : 0;
		UINT32 p4 = (i&8) ? 0xFFFF : 0;

		g_mask32[i*2] = (p2 << 16) | p1;
		g_mask32[i*2+1] = (p4 << 16) | p3;
	}
}


void tilemap_set_transparent_pen( struct tilemap *tilemap, int pen )
{
	tilemap->transparent_pen = pen;
}

void tilemap_set_transmask( struct tilemap *tilemap, int which, UINT32 fgmask, UINT32 bgmask )
{
	if( tilemap->fgmask[which] != fgmask || tilemap->bgmask[which] != bgmask )
	{
		tilemap->fgmask[which] = fgmask;
		tilemap->bgmask[which] = bgmask;
		tilemap_mark_all_tiles_dirty( tilemap );
	}
}

void tilemap_set_depth( struct tilemap *tilemap, int tile_depth, int tile_granularity )
{
	if( tilemap->tile_dirty_map )
	{
		free( tilemap->tile_dirty_map);
	}
	tilemap->tile_dirty_map = malloc( Machine->drv->total_colors >> tile_granularity );
	if( tilemap->tile_dirty_map )
	{
		tilemap->tile_depth = tile_depth;
		tilemap->tile_granularity = tile_granularity;
	}
}

/***********************************************************************************/
/* some common mappings */

UINT32 tilemap_scan_rows( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows )
{
	/* logical (col,row) -> memory offset */
	return row*num_cols + col;
}
UINT32 tilemap_scan_rows_flip_x( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows )
{
	/* logical (col,row) -> memory offset */
	return row*num_cols + (num_cols-col-1);
}
UINT32 tilemap_scan_rows_flip_y( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows )
{
	/* logical (col,row) -> memory offset */
	return (num_rows-row-1)*num_cols + col;
}
UINT32 tilemap_scan_rows_flip_xy( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows )
{
	/* logical (col,row) -> memory offset */
	return (num_rows-row-1)*num_cols + (num_cols-col-1);
}

UINT32 tilemap_scan_cols( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows )
{
	/* logical (col,row) -> memory offset */
	return col*num_rows + row;
}
UINT32 tilemap_scan_cols_flip_x( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows )
{
	/* logical (col,row) -> memory offset */
	return (num_cols-col-1)*num_rows + row;
}
UINT32 tilemap_scan_cols_flip_y( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows )
{
	/* logical (col,row) -> memory offset */
	return col*num_rows + (num_rows-row-1);
}
UINT32 tilemap_scan_cols_flip_xy( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows )
{
	/* logical (col,row) -> memory offset */
	return (num_cols-col-1)*num_rows + (num_rows-row-1);
}

/***********************************************************************************/

static int mappings_create( struct tilemap *tilemap )
{
	int max_memory_offset = 0;
	UINT32 col,row;
	UINT32 num_logical_rows = tilemap->num_logical_rows;
	UINT32 num_logical_cols = tilemap->num_logical_cols;
	/* count offsets (might be larger than num_tiles) */
	for( row=0; row<num_logical_rows; row++ )
	{
		for( col=0; col<num_logical_cols; col++ )
		{
			UINT32 memory_offset = tilemap->get_memory_offset( col, row, num_logical_cols, num_logical_rows );
			if( memory_offset>max_memory_offset ) max_memory_offset = memory_offset;
		}
	}
	max_memory_offset++;
	tilemap->max_memory_offset = max_memory_offset;
	/* logical to cached (tilemap_mark_dirty) */
	tilemap->memory_offset_to_cached_indx = malloc( sizeof(int)*max_memory_offset );
	if( tilemap->memory_offset_to_cached_indx )
	{
		/* cached to logical (get_tile_info) */
		tilemap->cached_indx_to_memory_offset = malloc( sizeof(UINT32)*tilemap->num_tiles );
		if( tilemap->cached_indx_to_memory_offset ) return 0; /* no error */
		free( tilemap->memory_offset_to_cached_indx );
	}
	return -1; /* error */
}

static void mappings_dispose( struct tilemap *tilemap )
{
	free( tilemap->cached_indx_to_memory_offset );
	free( tilemap->memory_offset_to_cached_indx );
}

static void mappings_update( struct tilemap *tilemap )
{
	int logical_flip;
	UINT32 logical_indx, cached_indx;
	UINT32 num_cached_rows = tilemap->num_cached_rows;
	UINT32 num_cached_cols = tilemap->num_cached_cols;
	UINT32 num_logical_rows = tilemap->num_logical_rows;
	UINT32 num_logical_cols = tilemap->num_logical_cols;
	for( logical_indx=0; logical_indx<tilemap->max_memory_offset; logical_indx++ )
	{
		tilemap->memory_offset_to_cached_indx[logical_indx] = -1;
	}

	for( logical_indx=0; logical_indx<tilemap->num_tiles; logical_indx++ )
	{
		UINT32 logical_col = logical_indx%num_logical_cols;
		UINT32 logical_row = logical_indx/num_logical_cols;
		int memory_offset = tilemap->get_memory_offset( logical_col, logical_row, num_logical_cols, num_logical_rows );
		UINT32 cached_col = logical_col;
		UINT32 cached_row = logical_row;
		if( tilemap->orientation & ORIENTATION_SWAP_XY ) SWAP(cached_col,cached_row)
		if( tilemap->orientation & ORIENTATION_FLIP_X ) cached_col = (num_cached_cols-1)-cached_col;
		if( tilemap->orientation & ORIENTATION_FLIP_Y ) cached_row = (num_cached_rows-1)-cached_row;
		cached_indx = cached_row*num_cached_cols+cached_col;
		tilemap->memory_offset_to_cached_indx[memory_offset] = cached_indx;
		tilemap->cached_indx_to_memory_offset[cached_indx] = memory_offset;
	}
	for( logical_flip = 0; logical_flip<4; logical_flip++ )
	{
		int cached_flip = logical_flip;
		if( tilemap->attributes&TILEMAP_FLIPX ) cached_flip ^= TILE_FLIPX;
		if( tilemap->attributes&TILEMAP_FLIPY ) cached_flip ^= TILE_FLIPY;
#ifndef PREROTATE_GFX
		if( Machine->orientation & ORIENTATION_SWAP_XY )
		{
			if( Machine->orientation & ORIENTATION_FLIP_X ) cached_flip ^= TILE_FLIPY;
			if( Machine->orientation & ORIENTATION_FLIP_Y ) cached_flip ^= TILE_FLIPX;
		}
		else
		{
			if( Machine->orientation & ORIENTATION_FLIP_X ) cached_flip ^= TILE_FLIPX;
			if( Machine->orientation & ORIENTATION_FLIP_Y ) cached_flip ^= TILE_FLIPY;
		}
#endif
		if( tilemap->orientation & ORIENTATION_SWAP_XY )
		{
			cached_flip = ((cached_flip&1)<<1) | ((cached_flip&2)>>1);
		}
		tilemap->logical_flip_to_cached_flip[logical_flip] = cached_flip;
	}
}

/***********************************************************************************/

static void pio( void *dest, const void *source, int count, UINT8 *pri, UINT32 pcode )
{
	int i;

	if (pcode)
		for( i=0; i<count; i++ )
		{
			pri[i] |= pcode;
		}
}

static void pit( void *dest, const void *source, const UINT8 *pMask, int mask, int value, int count, UINT8 *pri, UINT32 pcode )
{
	int i;

	if (pcode)
		for( i=0; i<count; i++ )
		{
			if( (pMask[i]&mask)==value )
			{
				pri[i] |= pcode;
			}
		}
}

/***********************************************************************************/

#ifndef pdo16
static void pdo16( UINT16 *dest, const UINT16 *source, int count, UINT8 *pri, UINT32 pcode )
{
	int i;
	memcpy( dest,source,count*sizeof(UINT16) );
	for( i=0; i<count; i++ )
	{
		pri[i] |= pcode;
	}
}
#endif

#ifndef pdo16pal
static void pdo16pal( UINT16 *dest, const UINT16 *source, int count, UINT8 *pri, UINT32 pcode )
{
	int pal = pcode >> 16;
	int i;
	for( i=0; i<count; i++ )
	{
		dest[i] = source[i] + pal;
		pri[i] |= pcode;
	}
}
#endif

#ifndef pdo16np
static void pdo16np( UINT16 *dest, const UINT16 *source, int count, UINT8 *pri, UINT32 pcode )
{
	memcpy( dest,source,count*sizeof(UINT16) );
}
#endif

static void pdo15( UINT16 *dest, const UINT16 *source, int count, UINT8 *pri, UINT32 pcode )
{
	int i;
	pen_t *clut = &Machine->remapped_colortable[pcode >> 16];
	for( i=0; i<count; i++ )
	{
		dest[i] = clut[source[i]];
		pri[i] |= pcode;
	}
}

#ifndef pdo32
static void pdo32( UINT32 *dest, const UINT16 *source, int count, UINT8 *pri, UINT32 pcode )
{
	int i;
	pen_t *clut = &Machine->remapped_colortable[pcode >> 16];
	for( i=0; i<count; i++ )
	{
		dest[i] = clut[source[i]];
		pri[i] |= pcode;
	}
}
#endif

#ifndef npdo32
static void npdo32( UINT32 *dest, const UINT16 *source, int count, UINT8 *pri, UINT32 pcode )
{
	int oddcount = count & 3;
	int unrcount = count & ~3;
	int i;
	pen_t *clut = &Machine->remapped_colortable[pcode >> 16];
	for( i=0; i<oddcount; i++ )
	{
		dest[i] = clut[source[i]];
	}
	source += count; dest += count;
	for( i=-unrcount; i; i+=4 )
	{
		UINT32 eax, ebx;
		eax = source[i  ];
		ebx = source[i+1];
		eax = clut[eax];
		ebx = clut[ebx];
		dest[i  ] = eax;
		eax = source[i+2];
		dest[i+1] = ebx;
		ebx = source[i+3];
		eax = clut[eax];
		ebx = clut[ebx];
		dest[i+2] = eax;
		dest[i+3] = ebx;
	}
}
#endif

/***********************************************************************************/

#ifndef pdt16
static void pdt16( UINT16 *dest, const UINT16 *source, const UINT8 *pMask, int mask, int value, int count, UINT8 *pri, UINT32 pcode )
{
	int i;

	for( i=0; i<count; i++ )
	{
		if( (pMask[i]&mask)==value )
		{
			dest[i] = source[i];
			pri[i] |= pcode;
		}
	}
}
#endif

#ifndef pdt16pal
static void pdt16pal( UINT16 *dest, const UINT16 *source, const UINT8 *pMask, int mask, int value, int count, UINT8 *pri, UINT32 pcode )
{
	int pal = pcode >> 16;
	int i;

	for( i=0; i<count; i++ )
	{
		if( (pMask[i]&mask)==value )
		{
			dest[i] = source[i] + pal;
			pri[i] |= pcode;
		}
	}
}
#endif

#ifndef pdt16np
static void pdt16np( UINT16 *dest, const UINT16 *source, const UINT8 *pMask, int mask, int value, int count, UINT8 *pri, UINT32 pcode )
{
	int i;

	for( i=0; i<count; i++ )
	{
		if( (pMask[i]&mask)==value )
			dest[i] = source[i];
	}
}
#endif

static void pdt15( UINT16 *dest, const UINT16 *source, const UINT8 *pMask, int mask, int value, int count, UINT8 *pri, UINT32 pcode )
{
	int i;
	pen_t *clut = &Machine->remapped_colortable[pcode >> 16];
	for( i=0; i<count; i++ )
	{
		if( (pMask[i]&mask)==value )
		{
			dest[i] = clut[source[i]];
			pri[i] |= pcode;
		}
	}
}

#ifndef pdt32
static void pdt32( UINT32 *dest, const UINT16 *source, const UINT8 *pMask, int mask, int value, int count, UINT8 *pri, UINT32 pcode )
{
	int i;
	pen_t *clut = &Machine->remapped_colortable[pcode >> 16];
	for( i=0; i<count; i++ )
	{
		if( (pMask[i]&mask)==value )
		{
			dest[i] = clut[source[i]];
			pri[i] |= pcode;
		}
	}
}
#endif

#ifndef npdt32
static void npdt32( UINT32 *dest, const UINT16 *source, const UINT8 *pMask, int mask, int value, int count, UINT8 *pri, UINT32 pcode )
{
	int oddcount = count & 3;
	int unrcount = count & ~3;
	int i;
	pen_t *clut = &Machine->remapped_colortable[pcode >> 16];

	for( i=0; i<oddcount; i++ )
	{
		if( (pMask[i]&mask)==value ) dest[i] = clut[source[i]];
	}
	pMask += count, source += count; dest += count;
	for( i=-unrcount; i; i+=4 )
	{
		if( (pMask[i  ]&mask)==value ) dest[i  ] = clut[source[i  ]];
		if( (pMask[i+1]&mask)==value ) dest[i+1] = clut[source[i+1]];
		if( (pMask[i+2]&mask)==value ) dest[i+2] = clut[source[i+2]];
		if( (pMask[i+3]&mask)==value ) dest[i+3] = clut[source[i+3]];
	}
}
#endif

/***********************************************************************************/

static void pbo15( UINT16 *dest, const UINT16 *source, int count, UINT8 *pri, UINT32 pcode )
{
	int i;
	pen_t *clut = &Machine->remapped_colortable[pcode >> 16];
	for( i=0; i<count; i++ )
	{
		dest[i] = alpha_blend16(dest[i], clut[source[i]]);
		pri[i] |= pcode;
	}
}

#ifndef pbo32
static void pbo32( UINT32 *dest, const UINT16 *source, int count, UINT8 *pri, UINT32 pcode )
{
	int i;
	pen_t *clut = &Machine->remapped_colortable[pcode >> 16];
	for( i=0; i<count; i++ )
	{
		dest[i] = alpha_blend32(dest[i], clut[source[i]]);
		pri[i] |= pcode;
	}
}
#endif

#ifndef npbo32
static void npbo32( UINT32 *dest, const UINT16 *source, int count, UINT8 *pri, UINT32 pcode )
{
	int oddcount = count & 3;
	int unrcount = count & ~3;
	int i;
	pen_t *clut = &Machine->remapped_colortable[pcode >> 16];
	for( i=0; i<oddcount; i++ )
	{
		dest[i] = alpha_blend32(dest[i], clut[source[i]]);
	}
	source += count; dest += count;
	for( i=-unrcount; i; i+=4 )
	{
		dest[i  ] = alpha_blend32(dest[i  ], clut[source[i  ]]);
		dest[i+1] = alpha_blend32(dest[i+1], clut[source[i+1]]);
		dest[i+2] = alpha_blend32(dest[i+2], clut[source[i+2]]);
		dest[i+3] = alpha_blend32(dest[i+3], clut[source[i+3]]);
	}
}
#endif

/***********************************************************************************/

static void pbt15( UINT16 *dest, const UINT16 *source, const UINT8 *pMask, int mask, int value, int count, UINT8 *pri, UINT32 pcode )
{
	int i;
	pen_t *clut = &Machine->remapped_colortable[pcode >> 16];
	for( i=0; i<count; i++ )
	{
		if( (pMask[i]&mask)==value )
		{
			dest[i] = alpha_blend16(dest[i], clut[source[i]]);
			pri[i] |= pcode;
		}
	}
}

#ifndef pbt32
static void pbt32( UINT32 *dest, const UINT16 *source, const UINT8 *pMask, int mask, int value, int count, UINT8 *pri, UINT32 pcode )
{
	int i;
	pen_t *clut = &Machine->remapped_colortable[pcode >> 16];
	for( i=0; i<count; i++ )
	{
		if( (pMask[i]&mask)==value )
		{
			dest[i] = alpha_blend32(dest[i], clut[source[i]]);
			pri[i] |= pcode;
		}
	}
}
#endif

#ifndef npbt32
static void npbt32( UINT32 *dest, const UINT16 *source, const UINT8 *pMask, int mask, int value, int count, UINT8 *pri, UINT32 pcode )
{
	int oddcount = count & 3;
	int unrcount = count & ~3;
	int i;
	pen_t *clut = &Machine->remapped_colortable[pcode >> 16];

	for( i=0; i<oddcount; i++ )
	{
		if( (pMask[i]&mask)==value ) dest[i] = alpha_blend32(dest[i], clut[source[i]]);
	}
	pMask += count, source += count; dest += count;
	for( i=-unrcount; i; i+=4 )
	{
		if( (pMask[i  ]&mask)==value ) dest[i  ] = alpha_blend32(dest[i  ], clut[source[i  ]]);
		if( (pMask[i+1]&mask)==value ) dest[i+1] = alpha_blend32(dest[i+1], clut[source[i+1]]);
		if( (pMask[i+2]&mask)==value ) dest[i+2] = alpha_blend32(dest[i+2], clut[source[i+2]]);
		if( (pMask[i+3]&mask)==value ) dest[i+3] = alpha_blend32(dest[i+3], clut[source[i+3]]);
	}
}
#endif

/***********************************************************************************/

#define PAL_INIT const pen_t *pPalData = tile_info.pal_data
#define PAL_GET(pen) pPalData[pen]
#define TRANSP(f) f ## _ind
#include "tilemap.c"

#define PAL_INIT int palBase = tile_info.pal_data - Machine->remapped_colortable
#define PAL_GET(pen) (palBase + (pen))
#define TRANSP(f) f ## _raw
#include "tilemap.c"

/*********************************************************************************/

static void install_draw_handlers( struct tilemap *tilemap )
{
	if( Machine->game_colortable )
	{
		if( tilemap->type & TILEMAP_BITMASK )
			tilemap->draw_tile = HandleTransparencyBitmask_ind;
		else if( tilemap->type & TILEMAP_SPLIT_PENBIT )
			tilemap->draw_tile = HandleTransparencyPenBit_ind;
		else if( tilemap->type & TILEMAP_SPLIT )
			tilemap->draw_tile = HandleTransparencyPens_ind;
		else if( tilemap->type==TILEMAP_TRANSPARENT )
			tilemap->draw_tile = HandleTransparencyPen_ind;
		else if( tilemap->type==TILEMAP_TRANSPARENT_COLOR )
			tilemap->draw_tile = HandleTransparencyColor_ind;
		else
			tilemap->draw_tile = HandleTransparencyNone_ind;
	}
	else
	{
		if( tilemap->type & TILEMAP_BITMASK )
			tilemap->draw_tile = HandleTransparencyBitmask_raw;
		else if( tilemap->type & TILEMAP_SPLIT_PENBIT )
			tilemap->draw_tile = HandleTransparencyPenBit_raw;
		else if( tilemap->type & TILEMAP_SPLIT )
			tilemap->draw_tile = HandleTransparencyPens_raw;
		else if( tilemap->type==TILEMAP_TRANSPARENT )
			tilemap->draw_tile = HandleTransparencyPen_raw;
		else if( tilemap->type==TILEMAP_TRANSPARENT_COLOR )
			tilemap->draw_tile = HandleTransparencyColor_raw;
		else
			tilemap->draw_tile = HandleTransparencyNone_raw;
	}
}

static void copyroz_core16BPP (
      struct mame_bitmap *bitmap,
      struct tilemap *tilemap,
      UINT32 startx,UINT32 starty,int incxx,int incxy,int incyx,
      int incyy,int wraparound, const struct rectangle *clip, int mask,
      int value, UINT32 priority,UINT32 palette_offset)
{
   UINT32 cx;
   UINT32 cy;
   int x;
   int sx;
   int sy;
   int ex;
   int ey;
   struct mame_bitmap *srcbitmap = tilemap->pixmap;
   struct mame_bitmap *transparency_bitmap = tilemap->transparency_bitmap;
   const int xmask = srcbitmap->width-1;
   const int ymask = srcbitmap->height-1;
   const int widthshifted = srcbitmap->width << 16;
   const int heightshifted = srcbitmap->height << 16;
   UINT16 *dest;
   UINT8 *pri;
   const UINT16 *src;
   const UINT8 *pMask;
   
   if (clip)
   {
      startx += clip->min_x * incxx + clip->min_y * incyx;
      starty += clip->min_x * incxy + clip->min_y * incyy;
      sx = clip->min_x;
      sy = clip->min_y;
      ex = clip->max_x;
      ey = clip->max_y;
   }
   else
   {
      sx = 0;
      sy = 0;
      ex = bitmap->width-1;
      ey = bitmap->height-1;
   }
   
   if (Machine->orientation & 0x0004)
   {
      int t;
      t = startx;
      startx = starty;
      starty = t;
      t = sx;
      sx = sy;
      sy = t;
      t = ex;
      ex = ey;
      ey = t;
      t = incxx;
      incxx = incyy;
      incyy = t;
      t = incxy;
      incxy = incyx;
      incyx = t;
   }
   
   if (Machine->orientation & 0x0001)
   {
      int w = ex - sx;
      incxy = -incxy;
      incyx = -incyx;
      startx = widthshifted - startx - 1;
      startx -= incxx * w;
      starty -= incxy * w;
      w = sx;
      sx = bitmap->width-1 - ex;
      ex = bitmap->width-1 - w;
   }
   
   if (Machine->orientation & 0x0002)
   {
      int h = ey - sy;
      incxy = -incxy;
      incyx = -incyx;
      starty = heightshifted - starty - 1;
      startx -= incyx * h;
      starty -= incyy * h;
      h = sy;
      sy = bitmap->height-1 - ey;
      ey = bitmap->height-1 - h;
   }
   
   if (incxy == 0 && incyx == 0 && !wraparound)
   {
      if (incxx == 0x10000)
      {
         startx = ((INT32)startx) >> 16;
         if (startx >= srcbitmap->width)
         {
            sx += -startx; startx = 0;
         }
         
         if (sx <= ex)
         {
            while (sy <= ey)
            {
               if (starty < heightshifted)
               {
                  x = sx;
                  cx = startx;
                  cy = starty >> 16;
                  dest = ((UINT16 *)bitmap->line[sy]) + sx;
                  pri = ((UINT8 *)priority_bitmap->line[sy]) + sx;
                  src = (UINT16 *)srcbitmap->line[cy];
                  pMask = (UINT8 *)transparency_bitmap->line[cy];
                  
                  while (x <= ex && cx < srcbitmap->width)
                  {
                     if ( (pMask[cx]&mask) == value )
                     {
                        *dest = src[cx]+palette_offset;
                        *pri |= priority;
                     }
                     cx++;
                     x++;
                     dest++;
                     pri++;
                  }
               }
               starty += incyy;
               sy++;
            }
         }
      }
      else
      {
         while (startx >= widthshifted && sx <= ex)
         {
            startx += incxx;
            sx++;
         }
         
         if (sx <= ex)
         {
            while (sy <= ey)
            {
               if (starty < heightshifted)
               {
                  x = sx;
                  cx = startx;
                  cy = starty >> 16;
                  dest = ((UINT16 *)bitmap->line[sy]) + sx;
                  pri = ((UINT8 *)priority_bitmap->line[sy]) + sx;
                  src = (UINT16 *)srcbitmap->line[cy];
                  pMask = (UINT8 *)transparency_bitmap->line[cy];
                  while (x <= ex && cx < widthshifted)
                  {
                     if ( (pMask[cx>>16]&mask) == value )
                     {
                        *dest = src[cx >> 16]+palette_offset;
                        *pri |= priority;
                     }
                     cx += incxx;
                     x++;
                     dest++;
                     pri++;
                  }
               }
               starty += incyy;
               sy++;
            }
         }
      }
   }
   else
   {
      if (wraparound)
      {
         while (sy <= ey)
         {
            x = sx;
            cx = startx;
            cy = starty;
            dest = ((UINT16 *)bitmap->line[sy]) + sx;
            pri = ((UINT8 *)priority_bitmap->line[sy]) + sx;

            while (x <= ex)
            {
               if( (((UINT8 *)transparency_bitmap->line[(cy>>16)&ymask])[(cx>>16)&xmask]&mask) == value )
               {
                  *dest = ((UINT16 *)srcbitmap->line[(cy >> 16) & ymask])[(cx >> 16) & xmask]+palette_offset;
                  *pri |= priority;
               }

               cx += incxx;
               cy += incxy;
               x++;
               dest++;
               pri++;
            }
            startx += incyx;
            starty += incyy;
            sy++;
         }
      }
      else
      {
         while (sy <= ey)
         {
            x = sx;
            cx = startx;
            cy = starty;
            dest = ((UINT16 *)bitmap->line[sy]) + sx;
            pri = ((UINT8 *)priority_bitmap->line[sy]) + sx;

            while (x <= ex)
            {
               if (cx < widthshifted && cy < heightshifted)
               {
                  if( (((UINT8 *)transparency_bitmap->line[cy>>16])[cx>>16]&mask)==value )
                  {
                     *dest = ((UINT16 *)srcbitmap->line[cy >> 16])[cx >> 16]+palette_offset;
                     *pri |= priority;
                  }
               }
               cx += incxx;
               cy += incxy;
               x++;
               dest++;
               pri++;
            }
            startx += incyx;
            starty += incyy;
            sy++;
         }
      }
   }
}

static void draw16BPP (struct tilemap *tilemap, int xpos, int ypos, int mask, int value )
{
   trans_t transPrev;
   trans_t transCur;
   const UINT8 *pTrans;
   UINT32 cached_indx;
   struct mame_bitmap *screen = blit.screen_bitmap;
   int tilemap_priority_code = blit.tilemap_priority_code;
   int x1 = xpos;
   int y1 = ypos;
   int x2 = xpos+tilemap->cached_width;
   int y2 = ypos+tilemap->cached_height;
   UINT16 *dest_baseaddr = ((void *)0);
   UINT16 *dest_next;
   int dy;
   int count;
   const UINT16 *source0;
   UINT16 *dest0;
   UINT8 *pmap0;
   int i;
   int row;
   int x_start;
   int x_end;
   int column;
   int c1;
   int c2;
   int y;
   int y_next;
   UINT8 *priority_bitmap_baseaddr;
   UINT8 *priority_bitmap_next;
   const UINT16 *source_baseaddr;
   const UINT16 *source_next;
   const UINT8 *mask0;
   const UINT8 *mask_baseaddr;
   const UINT8 *mask_next;

   if( x1<blit.clip_left )
      x1 = blit.clip_left;
   if( x2>blit.clip_right )
      x2 = blit.clip_right;
   if( y1<blit.clip_top )
      y1 = blit.clip_top;
   if( y2>blit.clip_bottom )
      y2 = blit.clip_bottom;
   if( x1<x2 && y1<y2 )
   {
      priority_bitmap_baseaddr = xpos + (UINT8 *)priority_bitmap->line[y1];

      if( screen )
         dest_baseaddr = xpos + (UINT16 *)screen->line[y1];

      x1 -= xpos;
      y1 -= ypos;
      x2 -= xpos;
      y2 -= ypos;
      source_baseaddr = (UINT16 *)tilemap->pixmap->line[y1];
      mask_baseaddr = tilemap->transparency_bitmap->line[y1];
      c1 = x1/tilemap->cached_tile_width;
      c2 = (x2+tilemap->cached_tile_width-1)/tilemap->cached_tile_width;
      y = y1;
      y_next = tilemap->cached_tile_height*(y1/tilemap->cached_tile_height) + tilemap->cached_tile_height;

      if( y_next>y2 )
         y_next = y2;
      dy = y_next-y;
      dest_next = dest_baseaddr + dy*blit.screen_bitmap_pitch_line;
      priority_bitmap_next = priority_bitmap_baseaddr + dy*priority_bitmap_pitch_line;
      source_next = source_baseaddr + dy*tilemap->pixmap_pitch_line;
      mask_next = mask_baseaddr + dy*tilemap->transparency_bitmap_pitch_line;

      for(;;)
      {
         row = y/tilemap->cached_tile_height;
         x_start = x1;
         transPrev = eWHOLLY_TRANSPARENT;
         pTrans = mask_baseaddr + x_start;
         cached_indx = row*tilemap->num_cached_cols + c1;

         for( column=c1; column<=c2; column++ )
         {
            if( column == c2 )
            {
               transCur = eWHOLLY_TRANSPARENT;
               goto L_Skip;
            }

            if( tilemap->transparency_data[cached_indx]==(0x80) )
            {
               update_tile_info( tilemap, cached_indx, column, row );
            }
            if( (tilemap->transparency_data[cached_indx]&mask)!=0 )
            {
               transCur = eMASKED;
            }
            else
            {
               transCur = (((*pTrans)&mask) == value)?eWHOLLY_OPAQUE:eWHOLLY_TRANSPARENT;
            }

            pTrans += tilemap->cached_tile_width;
L_Skip:
            if( transCur!=transPrev )
            {
               x_end = column*tilemap->cached_tile_width;

               if( x_end<x1 )
                  x_end = x1;
               if( x_end>x2 )
                  x_end = x2;
               if( transPrev != eWHOLLY_TRANSPARENT )
               {
                  count = x_end - x_start;
                  source0 = source_baseaddr + x_start;
                  dest0 = dest_baseaddr + x_start;
                  pmap0 = priority_bitmap_baseaddr + x_start;

                  if( transPrev == eWHOLLY_OPAQUE )
                  {
                     i = y;

                     for(;;)
                     {
                        blit.draw_opaque( dest0, source0, count, pmap0, tilemap_priority_code );

                        if( ++i == y_next )
                           break;

                        dest0 += blit.screen_bitmap_pitch_line;
                        source0 += tilemap->pixmap_pitch_line;
                        pmap0 += priority_bitmap_pitch_line;
                     }
                  }
                  else
                  {
                     mask0 = mask_baseaddr + x_start;
                     i = y;

                     for(;;)
                     {
                        blit.draw_masked( dest0, source0, mask0, mask, value, 
                              count, pmap0, tilemap_priority_code );

                        if( ++i == y_next )
                           break;

                        dest0 += blit.screen_bitmap_pitch_line;
                        source0 += tilemap->pixmap_pitch_line;
                        mask0 += tilemap->transparency_bitmap_pitch_line;
                        pmap0 += priority_bitmap_pitch_line;
                     }
                  }
               }
               x_start = x_end;
               transPrev = transCur;
            }

            cached_indx++;
         }

         if( y_next==y2 )
            break;
         priority_bitmap_baseaddr = priority_bitmap_next;
         dest_baseaddr = dest_next;
         source_baseaddr = source_next;
         mask_baseaddr = mask_next;
         y = y_next;
         y_next += tilemap->cached_tile_height;

         if( y_next>=y2 )
            y_next = y2;
         else
         {
            dest_next += blit.screen_bitmap_pitch_row;
            priority_bitmap_next += priority_bitmap_pitch_row;
            source_next += tilemap->pixmap_pitch_row;
            mask_next += tilemap->transparency_bitmap_pitch_row;
         }
      }
   }
}

static void copyroz_core32BPP(struct mame_bitmap *bitmap,struct tilemap *tilemap,
		UINT32 startx,UINT32 starty,int incxx,int incxy,int incyx,int incyy,int wraparound,
		const struct rectangle *clip,
		int mask,int value,
		UINT32 priority,UINT32 palette_offset)
{
	UINT32 cx;
	UINT32 cy;
	int x;
	int sx;
	int sy;
	int ex;
	int ey;
	struct mame_bitmap *srcbitmap = tilemap->pixmap;
	struct mame_bitmap *transparency_bitmap = tilemap->transparency_bitmap;
	const int xmask = srcbitmap->width-1;
	const int ymask = srcbitmap->height-1;
	const int widthshifted = srcbitmap->width << 16;
	const int heightshifted = srcbitmap->height << 16;
	UINT32 *dest;
	UINT8 *pri;
	const UINT16 *src;
	const UINT8 *pMask;

	if (clip)
	{
		startx += clip->min_x * incxx + clip->min_y * incyx;
		starty += clip->min_x * incxy + clip->min_y * incyy;

		sx = clip->min_x;
		sy = clip->min_y;
		ex = clip->max_x;
		ey = clip->max_y;
	}
	else
	{
		sx = 0;
		sy = 0;
		ex = bitmap->width-1;
		ey = bitmap->height-1;
	}


	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		int t;

		t = startx; startx = starty; starty = t;
		t = sx; sx = sy; sy = t;
		t = ex; ex = ey; ey = t;
		t = incxx; incxx = incyy; incyy = t;
		t = incxy; incxy = incyx; incyx = t;
	}

	if (Machine->orientation & ORIENTATION_FLIP_X)
	{
		int w = ex - sx;

		incxy = -incxy;
		incyx = -incyx;
		startx = widthshifted - startx - 1;
		startx -= incxx * w;
		starty -= incxy * w;

		w = sx;
		sx = bitmap->width-1 - ex;
		ex = bitmap->width-1 - w;
	}

	if (Machine->orientation & ORIENTATION_FLIP_Y)
	{
		int h = ey - sy;

		incxy = -incxy;
		incyx = -incyx;
		starty = heightshifted - starty - 1;
		startx -= incyx * h;
		starty -= incyy * h;

		h = sy;
		sy = bitmap->height-1 - ey;
		ey = bitmap->height-1 - h;
	}

	if (incxy == 0 && incyx == 0 && !wraparound)
	{
		/* optimized loop for the not rotated case */

		if (incxx == 0x10000)
		{
			/* optimized loop for the not zoomed case */

			/* startx is unsigned */
			startx = ((INT32)startx) >> 16;

			if (startx >= srcbitmap->width)
			{
				sx += -startx;
				startx = 0;
			}

			if (sx <= ex)
			{
				while (sy <= ey)
				{
					if (starty < heightshifted)
					{
						x = sx;
						cx = startx;
						cy = starty >> 16;
						dest = ((UINT32 *)bitmap->line[sy]) + sx;

						pri = ((UINT8 *)priority_bitmap->line[sy]) + sx;
						src = (UINT16 *)srcbitmap->line[cy];
						pMask = (UINT8 *)transparency_bitmap->line[cy];

						while (x <= ex && cx < srcbitmap->width)
						{
							if ( (pMask[cx]&mask) == value )
							{
								*dest = src[cx]+palette_offset;
								*pri |= priority;
							}
							cx++;
							x++;
							dest++;
							pri++;
						}
					}
					starty += incyy;
					sy++;
				}
			}
		}
		else
		{
			while (startx >= widthshifted && sx <= ex)
			{
				startx += incxx;
				sx++;
			}

			if (sx <= ex)
			{
				while (sy <= ey)
				{
					if (starty < heightshifted)
					{
						x = sx;
						cx = startx;
						cy = starty >> 16;
						dest = ((UINT32 *)bitmap->line[sy]) + sx;

						pri = ((UINT8 *)priority_bitmap->line[sy]) + sx;
						src = (UINT16 *)srcbitmap->line[cy];
						pMask = (UINT8 *)transparency_bitmap->line[cy];
						while (x <= ex && cx < widthshifted)
						{
							if ( (pMask[cx>>16]&mask) == value )
							{
								*dest = src[cx >> 16]+palette_offset;
								*pri |= priority;
							}
							cx += incxx;
							x++;
							dest++;
							pri++;
						}
					}
					starty += incyy;
					sy++;
				}
			}
		}
	}
	else
	{
		if (wraparound)
		{
			/* plot with wraparound */
			while (sy <= ey)
			{
				x = sx;
				cx = startx;
				cy = starty;
				dest = ((UINT32 *)bitmap->line[sy]) + sx;
				pri = ((UINT8 *)priority_bitmap->line[sy]) + sx;
				while (x <= ex)
				{
					if( (((UINT8 *)transparency_bitmap->line[(cy>>16)&ymask])[(cx>>16)&xmask]&mask) == value )
					{
						*dest = ((UINT16 *)srcbitmap->line[(cy >> 16) & ymask])[(cx >> 16) & xmask]+palette_offset;
						*pri |= priority;
					}
					cx += incxx;
					cy += incxy;
					x++;
					dest++;
					pri++;
				}
				startx += incyx;
				starty += incyy;
				sy++;
			}
		}
		else
		{
			while (sy <= ey)
			{
				x = sx;
				cx = startx;
				cy = starty;
				dest = ((UINT32 *)bitmap->line[sy]) + sx;
				pri = ((UINT8 *)priority_bitmap->line[sy]) + sx;
				while (x <= ex)
				{
					if (cx < widthshifted && cy < heightshifted)
					{
						if( (((UINT8 *)transparency_bitmap->line[cy>>16])[cx>>16]&mask)==value )
						{
							*dest = ((UINT16 *)srcbitmap->line[cy >> 16])[cx >> 16]+palette_offset;
							*pri |= priority;
						}
					}
					cx += incxx;
					cy += incxy;
					x++;
					dest++;
					pri++;
				}
				startx += incyx;
				starty += incyy;
				sy++;
			}
		}
	}
}

static void draw32BPP(struct tilemap *tilemap, int xpos, int ypos, int mask, int value )
{
   trans_t transPrev;
   trans_t transCur;
   const UINT8 *pTrans;
   UINT32 cached_indx;
   struct mame_bitmap *screen = blit.screen_bitmap;
   int tilemap_priority_code = blit.tilemap_priority_code;
   int x1 = xpos;
   int y1 = ypos;
   int x2 = xpos+tilemap->cached_width;
   int y2 = ypos+tilemap->cached_height;
   UINT32 *dest_baseaddr = NULL;
   UINT32 *dest_next;
   int dy;
   int count;
   const UINT16 *source0;
   UINT32 *dest0;
   UINT8 *pmap0;
   int i;
   int row;
   int x_start;
   int x_end;
   int column;
   int c1; /* leftmost visible column in source tilemap */
   int c2; /* rightmost visible column in source tilemap */
   int y; /* current screen line to render */
   int y_next;
   UINT8 *priority_bitmap_baseaddr;
   UINT8 *priority_bitmap_next;
   const UINT16 *source_baseaddr;
   const UINT16 *source_next;
   const UINT8 *mask0;
   const UINT8 *mask_baseaddr;
   const UINT8 *mask_next;

   /* clip source coordinates */
   if( x1<blit.clip_left ) x1 = blit.clip_left;
   if( x2>blit.clip_right ) x2 = blit.clip_right;
   if( y1<blit.clip_top ) y1 = blit.clip_top;
   if( y2>blit.clip_bottom ) y2 = blit.clip_bottom;

   if( x1<x2 && y1<y2 ) /* do nothing if totally clipped */
   {
      priority_bitmap_baseaddr = xpos + (UINT8 *)priority_bitmap->line[y1];
      if( screen )
      {
         dest_baseaddr = xpos + (UINT32 *)screen->line[y1];
      }

      /* convert screen coordinates to source tilemap coordinates */
      x1 -= xpos;
      y1 -= ypos;
      x2 -= xpos;
      y2 -= ypos;

      source_baseaddr = (UINT16 *)tilemap->pixmap->line[y1];
      mask_baseaddr = tilemap->transparency_bitmap->line[y1];

      c1 = x1/tilemap->cached_tile_width; /* round down */
      c2 = (x2+tilemap->cached_tile_width-1)/tilemap->cached_tile_width; /* round up */

      y = y1;
      y_next = tilemap->cached_tile_height*(y1/tilemap->cached_tile_height) + tilemap->cached_tile_height;
      if( y_next>y2 ) y_next = y2;

      dy = y_next-y;
      dest_next = dest_baseaddr + dy*blit.screen_bitmap_pitch_line;
      priority_bitmap_next = priority_bitmap_baseaddr + dy*priority_bitmap_pitch_line;
      source_next = source_baseaddr + dy*tilemap->pixmap_pitch_line;
      mask_next = mask_baseaddr + dy*tilemap->transparency_bitmap_pitch_line;
      for(;;)
      {
         row = y/tilemap->cached_tile_height;
         x_start = x1;

         transPrev = eWHOLLY_TRANSPARENT;
         pTrans = mask_baseaddr + x_start;

         cached_indx = row*tilemap->num_cached_cols + c1;
         for( column=c1; column<=c2; column++ )
         {
            if( column == c2 )
            {
               transCur = eWHOLLY_TRANSPARENT;
               goto L_Skip;
            }

            if( tilemap->transparency_data[cached_indx]==TILE_FLAG_DIRTY )
            {
               update_tile_info( tilemap, cached_indx, column, row );
            }

            if( (tilemap->transparency_data[cached_indx]&mask)!=0 )
            {
               transCur = eMASKED;
            }
            else
            {
               transCur = (((*pTrans)&mask) == value)?eWHOLLY_OPAQUE:eWHOLLY_TRANSPARENT;
            }
            pTrans += tilemap->cached_tile_width;

L_Skip:
            if( transCur!=transPrev )
            {
               x_end = column*tilemap->cached_tile_width;
               if( x_end<x1 ) x_end = x1;
               if( x_end>x2 ) x_end = x2;

               if( transPrev != eWHOLLY_TRANSPARENT )
               {
                  count = x_end - x_start;
                  source0 = source_baseaddr + x_start;
                  dest0 = dest_baseaddr + x_start;
                  pmap0 = priority_bitmap_baseaddr + x_start;

                  if( transPrev == eWHOLLY_OPAQUE )
                  {
                     i = y;
                     for(;;)
                     {
                        blit.draw_opaque( dest0, source0, count, pmap0, tilemap_priority_code );
                        if( ++i == y_next ) break;

                        dest0 += blit.screen_bitmap_pitch_line;
                        source0 += tilemap->pixmap_pitch_line;
                        pmap0 += priority_bitmap_pitch_line;
                     }
                  } /* transPrev == eWHOLLY_OPAQUE */
                  else /* transPrev == eMASKED */
                  {
                     mask0 = mask_baseaddr + x_start;
                     i = y;
                     for(;;)
                     {
                        blit.draw_masked( dest0, source0, mask0, mask, value, count, pmap0, tilemap_priority_code );
                        if( ++i == y_next ) break;

                        dest0 += blit.screen_bitmap_pitch_line;
                        source0 += tilemap->pixmap_pitch_line;
                        mask0 += tilemap->transparency_bitmap_pitch_line;
                        pmap0 += priority_bitmap_pitch_line;
                     }
                  } /* transPrev == eMASKED */
               } /* transPrev != eWHOLLY_TRANSPARENT */
               x_start = x_end;
               transPrev = transCur;
            }
            cached_indx++;
         }
         if( y_next==y2 ) break; /* we are done! */

         priority_bitmap_baseaddr = priority_bitmap_next;
         dest_baseaddr = dest_next;
         source_baseaddr = source_next;
         mask_baseaddr = mask_next;
         y = y_next;
         y_next += tilemap->cached_tile_height;

         if( y_next>=y2 )
         {
            y_next = y2;
         }
         else
         {
            dest_next += blit.screen_bitmap_pitch_row;
            priority_bitmap_next += priority_bitmap_pitch_row;
            source_next += tilemap->pixmap_pitch_row;
            mask_next += tilemap->transparency_bitmap_pitch_row;
         }
      } /* process next row */
   } /* not totally clipped */
}

static INLINE tilemap_draw_func pick_draw_func( struct mame_bitmap *dest )
{
	switch (dest ? dest->depth : Machine->scrbitmap->depth)
	{
		case 32:
			return draw32BPP;

		case 16:
		case 15:
			return draw16BPP;
	}
	exit(1);
	return NULL;
}


/***********************************************************************************/

static void tilemap_reset(void)
{
	tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
}

int tilemap_init( void )
{
	screen_width	= Machine->scrbitmap->width;
	screen_height	= Machine->scrbitmap->height;
	first_tilemap	= NULL;

	state_save_register_func_postload(tilemap_reset);
	priority_bitmap = bitmap_alloc_depth( screen_width, screen_height, -8 );
	if( priority_bitmap )
	{
		priority_bitmap_pitch_line = ((UINT8 *)priority_bitmap->line[1]) - ((UINT8 *)priority_bitmap->line[0]);
		return 0;
	}
	InitMask32();
	return -1;
}

void tilemap_close( void )
{
	struct tilemap *next;

	while( first_tilemap )
	{
		next = first_tilemap->next;
		tilemap_dispose( first_tilemap );
		first_tilemap = next;
	}
	bitmap_free( priority_bitmap );
}

/***********************************************************************************/

struct tilemap *tilemap_create(
	void (*tile_get_info)( int memory_offset ),
	UINT32 (*get_memory_offset)( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows ),
	int type,
	int tile_width, int tile_height,
	int num_cols, int num_rows )
{
	struct tilemap *tilemap;
	UINT32 row;
	int num_tiles;

	tilemap = calloc( 1,sizeof( struct tilemap ) );
	if( tilemap )
	{
		num_tiles = num_cols*num_rows;
		tilemap->num_logical_cols = num_cols;
		tilemap->num_logical_rows = num_rows;
		tilemap->logical_tile_width = tile_width;
		tilemap->logical_tile_height = tile_height;
		tilemap->logical_colscroll = calloc(num_cols*tile_width,sizeof(int));
		tilemap->logical_rowscroll = calloc(num_rows*tile_height,sizeof(int));
		if( Machine->orientation & ORIENTATION_SWAP_XY )
		{
			SWAP( num_cols, num_rows )
			SWAP( tile_width, tile_height )
		}
		tilemap->num_cached_cols = num_cols;
		tilemap->num_cached_rows = num_rows;
		tilemap->num_tiles = num_tiles;
		tilemap->num_pens = tile_width*tile_height;
		tilemap->cached_tile_width = tile_width;
		tilemap->cached_tile_height = tile_height;
		tilemap->cached_width = tile_width*num_cols;
		tilemap->cached_height = tile_height*num_rows;
		tilemap->tile_get_info = tile_get_info;
		tilemap->get_memory_offset = get_memory_offset;
		tilemap->orientation = Machine->orientation;

		/* various defaults */
		tilemap->enable = 1;
		tilemap->type = type;
		tilemap->logical_scroll_rows = tilemap->cached_scroll_rows = 1;
		tilemap->logical_scroll_cols = tilemap->cached_scroll_cols = 1;
		tilemap->transparent_pen = -1;
		tilemap->tile_depth = 0;
		tilemap->tile_granularity = 0;
		tilemap->tile_dirty_map = 0;

		tilemap->cached_rowscroll	= calloc(tilemap->cached_height,sizeof(int));
		tilemap->cached_colscroll	= calloc(tilemap->cached_width, sizeof(int));

		tilemap->transparency_data = malloc( num_tiles );
		tilemap->transparency_data_row = malloc( sizeof(UINT8 *)*num_rows );

		tilemap->pixmap = bitmap_alloc_depth( tilemap->cached_width, tilemap->cached_height, -16 );
		tilemap->transparency_bitmap = bitmap_alloc_depth( tilemap->cached_width, tilemap->cached_height, -8 );

		if( tilemap->logical_rowscroll && tilemap->cached_rowscroll &&
			tilemap->logical_colscroll && tilemap->cached_colscroll &&
			tilemap->pixmap &&
			tilemap->transparency_data &&
			tilemap->transparency_data_row &&
			tilemap->transparency_bitmap &&
			(mappings_create( tilemap )==0) )
		{
			tilemap->pixmap_pitch_line = tilemap->pixmap->rowpixels;
			tilemap->pixmap_pitch_row = tilemap->pixmap_pitch_line*tile_height;

			tilemap->transparency_bitmap_pitch_line = tilemap->transparency_bitmap->rowpixels;
			tilemap->transparency_bitmap_pitch_row = tilemap->transparency_bitmap_pitch_line*tile_height;

			for( row=0; row<num_rows; row++ )
			{
				tilemap->transparency_data_row[row] = tilemap->transparency_data+num_cols*row;
			}
			install_draw_handlers( tilemap );
			mappings_update( tilemap );
			memset( tilemap->transparency_data, TILE_FLAG_DIRTY, num_tiles );
			tilemap->next = first_tilemap;
			first_tilemap = tilemap;
			if( PenToPixel_Init( tilemap ) == 0 )
			{
				recalculate_scroll(tilemap);
				return tilemap;
			}
		}
		tilemap_dispose( tilemap );
	}
	return 0;
}

void tilemap_dispose( struct tilemap *tilemap )
{
	struct tilemap *prev;

	if( tilemap==first_tilemap )
	{
		first_tilemap = tilemap->next;
	}
	else
	{
		prev = first_tilemap;
		while( prev && prev->next != tilemap ) prev = prev->next;
		if( prev ) prev->next =tilemap->next;
	}
	PenToPixel_Term( tilemap );
	free( tilemap->logical_rowscroll );
	free( tilemap->cached_rowscroll );
	free( tilemap->logical_colscroll );
	free( tilemap->cached_colscroll );
	free( tilemap->transparency_data );
	free( tilemap->transparency_data_row );
	bitmap_free( tilemap->transparency_bitmap );
	bitmap_free( tilemap->pixmap );
	mappings_dispose( tilemap );
	free( tilemap );
}

/***********************************************************************************/

void tilemap_set_enable( struct tilemap *tilemap, int enable )
{
	tilemap->enable = enable?1:0;
}


void tilemap_set_flip( struct tilemap *tilemap, int attributes )
{
	if( tilemap==ALL_TILEMAPS )
	{
		tilemap = first_tilemap;
		while( tilemap )
		{
			tilemap_set_flip( tilemap, attributes );
			tilemap = tilemap->next;
		}
	}
	else if( tilemap->attributes!=attributes )
	{
		tilemap->attributes = attributes;
		tilemap->orientation = Machine->orientation;
		if( attributes&TILEMAP_FLIPY )
		{
			tilemap->orientation ^= ORIENTATION_FLIP_Y;
		}

		if( attributes&TILEMAP_FLIPX )
		{
			tilemap->orientation ^= ORIENTATION_FLIP_X;
		}

		mappings_update( tilemap );
		recalculate_scroll( tilemap );
		tilemap_mark_all_tiles_dirty( tilemap );
	}
}

/***********************************************************************************/

void tilemap_set_scroll_cols( struct tilemap *tilemap, int n )
{
	tilemap->logical_scroll_cols = n;
	if( tilemap->orientation & ORIENTATION_SWAP_XY )
	{
		tilemap->cached_scroll_rows = n;
	}
	else
	{
		tilemap->cached_scroll_cols = n;
	}
}

void tilemap_set_scroll_rows( struct tilemap *tilemap, int n )
{
	tilemap->logical_scroll_rows = n;
	if( tilemap->orientation & ORIENTATION_SWAP_XY )
	{
		tilemap->cached_scroll_cols = n;
	}
	else
	{
		tilemap->cached_scroll_rows = n;
	}
}

/***********************************************************************************/

void tilemap_mark_tile_dirty( struct tilemap *tilemap, int memory_offset )
{
	if( memory_offset<tilemap->max_memory_offset )
	{
		int cached_indx = tilemap->memory_offset_to_cached_indx[memory_offset];
		if( cached_indx>=0 )
		{
			tilemap->transparency_data[cached_indx] = TILE_FLAG_DIRTY;
			tilemap->all_tiles_clean = 0;
		}
	}
}

void tilemap_mark_all_tiles_dirty( struct tilemap *tilemap )
{
	if( tilemap==ALL_TILEMAPS )
	{
		tilemap = first_tilemap;
		while( tilemap )
		{
			tilemap_mark_all_tiles_dirty( tilemap );
			tilemap = tilemap->next;
		}
	}
	else
	{
		tilemap->all_tiles_dirty = 1;
		tilemap->all_tiles_clean = 0;
	}
}

/***********************************************************************************/

static void update_tile_info( struct tilemap *tilemap, UINT32 cached_indx, UINT32 col, UINT32 row )
{
	UINT32 x0;
	UINT32 y0;
	UINT32 memory_offset;
	UINT32 flags;

profiler_mark(PROFILER_TILEMAP_UPDATE);

	memory_offset = tilemap->cached_indx_to_memory_offset[cached_indx];
	tilemap->tile_get_info( memory_offset );
	flags = tile_info.flags;
	flags = (flags&0xfc)|tilemap->logical_flip_to_cached_flip[flags&0x3];
	x0 = tilemap->cached_tile_width*col;
	y0 = tilemap->cached_tile_height*row;

	tilemap->transparency_data[cached_indx] = tilemap->draw_tile(tilemap,x0,y0,flags );

profiler_mark(PROFILER_END);
}

struct mame_bitmap *tilemap_get_pixmap( struct tilemap * tilemap )
{
	UINT32 cached_indx = 0;
	UINT32 row,col;

	if (tilemap->all_tiles_clean == 0)
	{
profiler_mark(PROFILER_TILEMAP_DRAW);

		/* if the whole map is dirty, mark it as such */
		if (tilemap->all_tiles_dirty)
		{
			memset( tilemap->transparency_data, TILE_FLAG_DIRTY, tilemap->num_tiles );
			tilemap->all_tiles_dirty = 0;
		}

		memset( &tile_info, 0x00, sizeof(tile_info) ); /* initialize defaults */
		tile_info.user_data = tilemap->user_data;

		/* walk over cached rows/cols (better to walk screen coords) */
		for( row=0; row<tilemap->num_cached_rows; row++ )
		{
			for( col=0; col<tilemap->num_cached_cols; col++ )
			{
				if( tilemap->transparency_data[cached_indx] == TILE_FLAG_DIRTY )
				{
					update_tile_info( tilemap, cached_indx, col, row );
				}
				cached_indx++;
			} /* next col */
		} /* next row */

		tilemap->all_tiles_clean = 1;

profiler_mark(PROFILER_END);
	}

	return tilemap->pixmap;
}

struct mame_bitmap *tilemap_get_transparency_bitmap( struct tilemap * tilemap )
{
	return tilemap->transparency_bitmap;
}

UINT8 *tilemap_get_transparency_data( struct tilemap * tilemap ) /***/
{
	return tilemap->transparency_data;
}

/***********************************************************************************/

static void
recalculate_scroll( struct tilemap *tilemap )
{
	int i;

	tilemap->scrollx_delta = (tilemap->attributes & TILEMAP_FLIPX )?tilemap->dx_if_flipped:tilemap->dx;
	tilemap->scrolly_delta = (tilemap->attributes & TILEMAP_FLIPY )?tilemap->dy_if_flipped:tilemap->dy;

	for( i=0; i<tilemap->logical_scroll_rows; i++ )
	{
		tilemap_set_scrollx( tilemap, i, tilemap->logical_rowscroll[i] );
	}
	for( i=0; i<tilemap->logical_scroll_cols; i++ )
	{
		tilemap_set_scrolly( tilemap, i, tilemap->logical_colscroll[i] );
	}
}

void
tilemap_set_scrolldx( struct tilemap *tilemap, int dx, int dx_if_flipped )
{
	tilemap->dx = dx;
	tilemap->dx_if_flipped = dx_if_flipped;
	recalculate_scroll( tilemap );
}

void
tilemap_set_scrolldy( struct tilemap *tilemap, int dy, int dy_if_flipped )
{
	tilemap->dy = dy;
	tilemap->dy_if_flipped = dy_if_flipped;
	recalculate_scroll( tilemap );
}

void tilemap_set_scrollx( struct tilemap *tilemap, int which, int value )
{
	tilemap->logical_rowscroll[which] = value;
	value = tilemap->scrollx_delta-value; /* adjust */

	if( tilemap->orientation & ORIENTATION_SWAP_XY )
	{
		/* if xy are swapped, we are actually panning the screen bitmap vertically */
		if( tilemap->orientation & ORIENTATION_FLIP_X )
		{
			/* adjust affected col */
			which = tilemap->cached_scroll_cols-1 - which;
		}
		if( tilemap->orientation & ORIENTATION_FLIP_Y )
		{
			/* adjust scroll amount */
			value = screen_height-tilemap->cached_height-value;
		}
		tilemap->cached_colscroll[which] = value;
	}
	else
	{
		if( tilemap->orientation & ORIENTATION_FLIP_Y )
		{
			/* adjust affected row */
			which = tilemap->cached_scroll_rows-1 - which;
		}
		if( tilemap->orientation & ORIENTATION_FLIP_X )
		{
			/* adjust scroll amount */
			value = screen_width-tilemap->cached_width-value;
		}
		tilemap->cached_rowscroll[which] = value;
	}
}

void tilemap_set_scrolly( struct tilemap *tilemap, int which, int value )
{
	tilemap->logical_colscroll[which] = value;
	value = tilemap->scrolly_delta - value; /* adjust */

	if( tilemap->orientation & ORIENTATION_SWAP_XY )
	{
		/* if xy are swapped, we are actually panning the screen bitmap horizontally */
		if( tilemap->orientation & ORIENTATION_FLIP_Y )
		{
			/* adjust affected row */
			which = tilemap->cached_scroll_rows-1 - which;
		}
		if( tilemap->orientation & ORIENTATION_FLIP_X )
		{
			/* adjust scroll amount */
			value = screen_width-tilemap->cached_width-value;
		}
		tilemap->cached_rowscroll[which] = value;
	}
	else
	{
		if( tilemap->orientation & ORIENTATION_FLIP_X )
		{
			/* adjust affected col */
			which = tilemap->cached_scroll_cols-1 - which;
		}
		if( tilemap->orientation & ORIENTATION_FLIP_Y )
		{
			/* adjust scroll amount */
			value = screen_height-tilemap->cached_height-value;
		}
		tilemap->cached_colscroll[which] = value;
	}
}

/***********************************************************************************/

void tilemap_set_palette_offset( struct tilemap *tilemap, int offset )
{
	tilemap->palette_offset = offset;
}

/***********************************************************************************/

void tilemap_set_user_data( struct tilemap *tilemap, void *user_data )
{
	tilemap->user_data = user_data;
}

/***********************************************************************************/

void tilemap_draw( struct mame_bitmap *dest, const struct rectangle *cliprect, struct tilemap *tilemap, UINT32 flags, UINT32 priority )
{
	tilemap_draw_func drawfunc = pick_draw_func(dest);
	int xpos,ypos,mask,value;
	int rows, cols;
	const int *rowscroll, *colscroll;
	int left, right, top, bottom;

profiler_mark(PROFILER_TILEMAP_DRAW);
	if( tilemap->enable )
	{
		/* scroll registers */
		rows		= tilemap->cached_scroll_rows;
		cols		= tilemap->cached_scroll_cols;
		rowscroll	= tilemap->cached_rowscroll;
		colscroll	= tilemap->cached_colscroll;

		/* clipping */
		if( cliprect )
		{
			left	= cliprect->min_x;
			top		= cliprect->min_y;
			right	= cliprect->max_x+1;
			bottom	= cliprect->max_y+1;

			if( Machine->orientation & ORIENTATION_SWAP_XY )
			{
				SWAP(left,top)
				SWAP(right,bottom)
			}

			if( Machine->orientation & ORIENTATION_FLIP_X )
			{
				SWAP(left,right)
				left	= screen_width-left;
				right	= screen_width-right;
			}

			if( Machine->orientation & ORIENTATION_FLIP_Y )
			{
				SWAP(top,bottom)
				top		= screen_height-top;
				bottom	= screen_height-bottom;
			}
		}
		else
		{
			left	= 0;
			top		= 0;
			right	= tilemap->cached_width;
			bottom	= tilemap->cached_height;
		}

		/* tile priority */
		mask		= TILE_FLAG_TILE_PRIORITY;
		value		= TILE_FLAG_TILE_PRIORITY&flags;

		/* initialize defaults */
		memset( &tile_info, 0x00, sizeof(tile_info) );
		tile_info.user_data = tilemap->user_data;

		/* if the whole map is dirty, mark it as such */
		if (tilemap->all_tiles_dirty)
		{
			memset( tilemap->transparency_data, TILE_FLAG_DIRTY, tilemap->num_tiles );
			tilemap->all_tiles_dirty = 0;
		}

		/* priority_bitmap_pitch_row is tilemap-specific */
		priority_bitmap_pitch_row = priority_bitmap_pitch_line*tilemap->cached_tile_height;

		blit.screen_bitmap = dest;
		if( dest == NULL )
		{
			blit.draw_masked = (blitmask_t)pit;
			blit.draw_opaque = (blitopaque_t)pio;
		}
		else
		{
			blit.screen_bitmap_pitch_line = ((UINT8 *)dest->line[1]) - ((UINT8 *)dest->line[0]);
			switch( dest->depth )
			{
			case 32:
				if (priority)
				{
					if( flags&TILEMAP_ALPHA )
					{
						blit.draw_masked = (blitmask_t)pbt32;
						blit.draw_opaque = (blitopaque_t)pbo32;
					}
					else
					{
						blit.draw_masked = (blitmask_t)pdt32;
						blit.draw_opaque = (blitopaque_t)pdo32;
					}
				}
				else
				{
					/** AAT APR2003: added 32-bit no-priority counterpart*/
					if( flags&TILEMAP_ALPHA )
					{
						blit.draw_masked = (blitmask_t)npbt32;
						blit.draw_opaque = (blitopaque_t)npbo32;
					}
					else
					{
						blit.draw_masked = (blitmask_t)npdt32;
						blit.draw_opaque = (blitopaque_t)npdo32;
					}
				}
				blit.screen_bitmap_pitch_line /= 4;
				break;
			case 15:
				if( flags&TILEMAP_ALPHA )
				{
					blit.draw_masked = (blitmask_t)pbt15;
					blit.draw_opaque = (blitopaque_t)pbo15;
				}
				else
				{
					blit.draw_masked = (blitmask_t)pdt15;
					blit.draw_opaque = (blitopaque_t)pdo15;
				}
				blit.screen_bitmap_pitch_line /= 2;
				break;

			case 16:
				if (tilemap->palette_offset)
				{
					blit.draw_masked = (blitmask_t)pdt16pal;
					blit.draw_opaque = (blitopaque_t)pdo16pal;
				}
				else if (priority)
				{
					blit.draw_masked = (blitmask_t)pdt16;
					blit.draw_opaque = (blitopaque_t)pdo16;
				}
				else
				{
					blit.draw_masked = (blitmask_t)pdt16np;
					blit.draw_opaque = (blitopaque_t)pdo16np;
				}
				blit.screen_bitmap_pitch_line /= 2;
				break;

			default:
				exit(1);
				break;
			}
			blit.screen_bitmap_pitch_row = blit.screen_bitmap_pitch_line*tilemap->cached_tile_height;
		} /* dest == bitmap */

		if( !(tilemap->type==TILEMAP_OPAQUE || (flags&TILEMAP_IGNORE_TRANSPARENCY)) )
		{
			if( flags&TILEMAP_BACK )
			{
				mask	|= TILE_FLAG_BG_OPAQUE;
				value	|= TILE_FLAG_BG_OPAQUE;
			}
			else
			{
				mask	|= TILE_FLAG_FG_OPAQUE;
				value	|= TILE_FLAG_FG_OPAQUE;
			}
		}

		blit.tilemap_priority_code = (priority & 0xffff) | (tilemap->palette_offset << 16);

		if( rows == 1 && cols == 1 )
		{ /* XY scrolling playfield */
			int scrollx = rowscroll[0];
			int scrolly = colscroll[0];

			if( scrollx < 0 )
			{
				scrollx = tilemap->cached_width - (-scrollx) % tilemap->cached_width;
			}
			else
			{
				scrollx = scrollx % tilemap->cached_width;
			}

			if( scrolly < 0 )
			{
				scrolly = tilemap->cached_height - (-scrolly) % tilemap->cached_height;
			}
			else
			{
				scrolly = scrolly % tilemap->cached_height;
			}

	 		blit.clip_left		= left;
	 		blit.clip_top		= top;
	 		blit.clip_right		= right;
	 		blit.clip_bottom	= bottom;

			for(
				ypos = scrolly - tilemap->cached_height;
				ypos < blit.clip_bottom;
				ypos += tilemap->cached_height )
			{
				for(
					xpos = scrollx - tilemap->cached_width;
					xpos < blit.clip_right;
					xpos += tilemap->cached_width )
				{
					drawfunc( tilemap, xpos, ypos, mask, value );
				}
			}
		}
		else if( rows == 1 )
		{ /* scrolling columns + horizontal scroll */
			int col = 0;
			int colwidth = tilemap->cached_width / cols;
			int scrollx = rowscroll[0];

			if( scrollx < 0 )
			{
				scrollx = tilemap->cached_width - (-scrollx) % tilemap->cached_width;
			}
			else
			{
				scrollx = scrollx % tilemap->cached_width;
			}

			blit.clip_top		= top;
			blit.clip_bottom	= bottom;

			while( col < cols )
			{
				int cons	= 1;
				int scrolly	= colscroll[col];

	 			/* count consecutive columns scrolled by the same amount */
				if( scrolly != TILE_LINE_DISABLED )
				{
					while( col + cons < cols &&	colscroll[col + cons] == scrolly ) cons++;

					if( scrolly < 0 )
					{
						scrolly = tilemap->cached_height - (-scrolly) % tilemap->cached_height;
					}
					else
					{
						scrolly %= tilemap->cached_height;
					}

					blit.clip_left = col * colwidth + scrollx;
					if (blit.clip_left < left) blit.clip_left = left;
					blit.clip_right = (col + cons) * colwidth + scrollx;
					if (blit.clip_right > right) blit.clip_right = right;

					for(
						ypos = scrolly - tilemap->cached_height;
						ypos < blit.clip_bottom;
						ypos += tilemap->cached_height )
					{
						drawfunc( tilemap, scrollx, ypos, mask, value );
					}

					blit.clip_left = col * colwidth + scrollx - tilemap->cached_width;
					if (blit.clip_left < left) blit.clip_left = left;
					blit.clip_right = (col + cons) * colwidth + scrollx - tilemap->cached_width;
					if (blit.clip_right > right) blit.clip_right = right;

					for(
						ypos = scrolly - tilemap->cached_height;
						ypos < blit.clip_bottom;
						ypos += tilemap->cached_height )
					{
						drawfunc( tilemap, scrollx - tilemap->cached_width, ypos, mask, value );
					}
				}
				col += cons;
			}
		}
		else if( cols == 1 )
		{ /* scrolling rows + vertical scroll */
			int row = 0;
			int rowheight = tilemap->cached_height / rows;
			int scrolly = colscroll[0];
			if( scrolly < 0 )
			{
				scrolly = tilemap->cached_height - (-scrolly) % tilemap->cached_height;
			}
			else
			{
				scrolly = scrolly % tilemap->cached_height;
			}
			blit.clip_left = left;
			blit.clip_right = right;
			while( row < rows )
			{
				int cons = 1;
				int scrollx = rowscroll[row];
				/* count consecutive rows scrolled by the same amount */
				if( scrollx != TILE_LINE_DISABLED )
				{
					while( row + cons < rows &&	rowscroll[row + cons] == scrollx ) cons++;
					if( scrollx < 0)
					{
						scrollx = tilemap->cached_width - (-scrollx) % tilemap->cached_width;
					}
					else
					{
						scrollx %= tilemap->cached_width;
					}
					blit.clip_top = row * rowheight + scrolly;
					if (blit.clip_top < top) blit.clip_top = top;
					blit.clip_bottom = (row + cons) * rowheight + scrolly;
					if (blit.clip_bottom > bottom) blit.clip_bottom = bottom;
					for(
						xpos = scrollx - tilemap->cached_width;
						xpos < blit.clip_right;
						xpos += tilemap->cached_width )
					{
						drawfunc( tilemap, xpos, scrolly, mask, value );
					}
					blit.clip_top = row * rowheight + scrolly - tilemap->cached_height;
					if (blit.clip_top < top) blit.clip_top = top;
					blit.clip_bottom = (row + cons) * rowheight + scrolly - tilemap->cached_height;
					if (blit.clip_bottom > bottom) blit.clip_bottom = bottom;
					for(
						xpos = scrollx - tilemap->cached_width;
						xpos < blit.clip_right;
						xpos += tilemap->cached_width )
					{
						drawfunc( tilemap, xpos, scrolly - tilemap->cached_height, mask, value );
					}
				}
				row += cons;
			}
		}
	}
profiler_mark(PROFILER_END);
}

/* notes:
   - startx and starty MUST be UINT32 for calculations to work correctly
   - srcbitmap->width and height are assumed to be a power of 2 to speed up wraparound
   */
void tilemap_draw_roz( struct mame_bitmap *dest,const struct rectangle *cliprect,struct tilemap *tilemap,
		UINT32 startx,UINT32 starty,int incxx,int incxy,int incyx,int incyy,
		int wraparound,
		UINT32 flags, UINT32 priority )
{
	if( (incxx == 1<<16) && !incxy & !incyx && (incyy == 1<<16) && wraparound )
	{
		tilemap_set_scrollx( tilemap, 0, startx >> 16 );
		tilemap_set_scrolly( tilemap, 0, starty >> 16 );
		tilemap_draw( dest, cliprect, tilemap, flags, priority );
	}
	else
	{
		int mask,value;

profiler_mark(PROFILER_TILEMAP_DRAW_ROZ);
		if( tilemap->enable )
		{
			/* tile priority */
			mask		= TILE_FLAG_TILE_PRIORITY;
			value		= TILE_FLAG_TILE_PRIORITY&flags;

			tilemap_get_pixmap( tilemap ); /* force update */

			if( !(tilemap->type==TILEMAP_OPAQUE || (flags&TILEMAP_IGNORE_TRANSPARENCY)) )
			{
				if( flags&TILEMAP_BACK )
				{
					mask	|= TILE_FLAG_BG_OPAQUE;
					value	|= TILE_FLAG_BG_OPAQUE;
				}
				else
				{
					mask	|= TILE_FLAG_FG_OPAQUE;
					value	|= TILE_FLAG_FG_OPAQUE;
				}
			}

			switch( dest->depth )
			{

			case 32:
				copyroz_core32BPP(dest,tilemap,startx,starty,incxx,incxy,incyx,incyy,
					wraparound,cliprect,mask,value,priority,tilemap->palette_offset);
				break;

			case 15:
			case 16:
				copyroz_core16BPP(dest,tilemap,startx,starty,incxx,incxy,incyx,incyy,
					wraparound,cliprect,mask,value,priority,tilemap->palette_offset);
				break;

			default:
				exit(1);
			}
		} /* tilemap->enable */
profiler_mark(PROFILER_END);
	}
}

UINT32 tilemap_count( void )
{
	UINT32 count = 0;
	struct tilemap *tilemap = first_tilemap;
	while( tilemap )
	{
		count++;
		tilemap = tilemap->next;
	}
	return count;
}

static struct tilemap *tilemap_nb_find( int number )
{
	int count = 0;
	struct tilemap *tilemap;

	tilemap = first_tilemap;
	while( tilemap )
	{
		count++;
		tilemap = tilemap->next;
	}

	number = (count-1)-number;

	tilemap = first_tilemap;
	while( number-- )
	{
		tilemap = tilemap->next;
	}
	return tilemap;
}

void tilemap_nb_size( UINT32 number, UINT32 *width, UINT32 *height )
{
	struct tilemap *tilemap = tilemap_nb_find( number );
	*width  = tilemap->cached_width;
	*height = tilemap->cached_height;
}

void tilemap_nb_draw( struct mame_bitmap *dest, UINT32 number, UINT32 scrollx, UINT32 scrolly )
{
	tilemap_draw_func drawfunc = pick_draw_func(dest);
	int xpos,ypos;
	struct tilemap *tilemap = tilemap_nb_find( number );

	blit.screen_bitmap = dest;
	blit.screen_bitmap_pitch_line = ((UINT8 *)dest->line[1]) - ((UINT8 *)dest->line[0]);
	switch( dest->depth )
	{
	case 32:
		blit.draw_opaque = (blitopaque_t)pdo32;
		blit.screen_bitmap_pitch_line /= 4;
		break;

	case 15:
		blit.draw_opaque = (blitopaque_t)pdo15;
		blit.screen_bitmap_pitch_line /= 2;
		break;

	case 16:
		blit.draw_opaque = (blitopaque_t)pdo16pal;
		blit.screen_bitmap_pitch_line /= 2;
		break;

	default:
		exit(1);
		break;
	}
	priority_bitmap_pitch_row = priority_bitmap_pitch_line*tilemap->cached_tile_height;
	blit.screen_bitmap_pitch_row = blit.screen_bitmap_pitch_line*tilemap->cached_tile_height;
	blit.tilemap_priority_code = (tilemap->palette_offset << 16);
	scrollx = tilemap->cached_width  - scrollx % tilemap->cached_width;
	scrolly = tilemap->cached_height - scrolly % tilemap->cached_height;

	blit.clip_left		= 0;
	blit.clip_top		= 0;
	blit.clip_right		= (dest->width < tilemap->cached_width) ? dest->width : tilemap->cached_width;
	blit.clip_bottom	= (dest->height < tilemap->cached_height) ? dest->height : tilemap->cached_height;

	for(
		ypos = scrolly - tilemap->cached_height;
		ypos < blit.clip_bottom;
		ypos += tilemap->cached_height )
	{
		for(
			xpos = scrollx - tilemap->cached_width;
			xpos < blit.clip_right;
			xpos += tilemap->cached_width )
		{
			drawfunc( tilemap, xpos, ypos, 0, 0 );
		}
	}
}


/***********************************************************************************/

#endif /* !DECLARE && !TRANSP*/

#ifdef TRANSP
/*************************************************************************************************/

/* Each of the following routines draws pixmap and transarency data for a single tile.
 *
 * This function returns a per-tile code.  Each bit of this code is 0 if the corresponding
 * bit is zero in every byte of transparency data in the tile, or 1 if that bit is not
 * consistant within the tile.
 *
 * This precomputed value allows us for any particular tile and mask, to determine if all pixels
 * in that tile have the same masked transparency value.
 */

static UINT8 TRANSP(HandleTransparencyBitmask)(struct tilemap *tilemap, UINT32 x0, UINT32 y0, UINT32 flags)
{
	UINT32 tile_width = tilemap->cached_tile_width;
	UINT32 tile_height = tilemap->cached_tile_height;
	struct mame_bitmap *pixmap = tilemap->pixmap;
	struct mame_bitmap *transparency_bitmap = tilemap->transparency_bitmap;
	int pitch = tile_width + tile_info.skip;
	PAL_INIT;
	UINT32 *pPenToPixel;
	const UINT8 *pPenData = tile_info.pen_data;
	const UINT8 *pSource;
	UINT32 code_transparent = tile_info.priority;
	UINT32 code_opaque = code_transparent | TILE_FLAG_FG_OPAQUE;
	UINT32 tx;
	UINT32 ty;
	UINT32 data;
	UINT32 yx;
	UINT32 x;
	UINT32 y;
	UINT32 pen;
	UINT8 *pBitmask = tile_info.mask_data;
	UINT32 bitoffs;
	int bWhollyOpaque;
	int bWhollyTransparent;
	int bDontIgnoreTransparency = !(flags&TILE_IGNORE_TRANSPARENCY);

	bWhollyOpaque = 1;
	bWhollyTransparent = 1;

	pPenToPixel = tilemap->pPenToPixel[flags&(TILE_FLIPY|TILE_FLIPX)];

	if( flags&TILE_4BPP )
	{
		for( ty=tile_height; ty!=0; ty-- )
		{
			pSource = pPenData;
			for( tx=tile_width/2; tx!=0; tx-- )
			{
				data = *pSource++;

				pen = data&0xf;
				yx = *pPenToPixel++;
				x = x0+(yx%MAX_TILESIZE);
				y = y0+(yx/MAX_TILESIZE);
				*(x+(UINT16 *)pixmap->line[y]) = PAL_GET(pen);

				pen = data>>4;
				yx = *pPenToPixel++;
				x = x0+(yx%MAX_TILESIZE);
				y = y0+(yx/MAX_TILESIZE);
				*(x+(UINT16 *)pixmap->line[y]) = PAL_GET(pen);
			}
			pPenData += pitch/2;
		}
	}
	else
	{
		for( ty=tile_height; ty!=0; ty-- )
		{
			pSource = pPenData;
			for( tx=tile_width; tx!=0; tx-- )
			{
				pen = *pSource++;
				yx = *pPenToPixel++;
				x = x0+(yx%MAX_TILESIZE);
				y = y0+(yx/MAX_TILESIZE);
				*(x+(UINT16 *)pixmap->line[y]) = PAL_GET(pen);
			}
			pPenData += pitch;
		}
	}

	pPenToPixel = tilemap->pPenToPixel[flags&(TILE_FLIPY|TILE_FLIPX)];
	bitoffs = 0;
	for( ty=tile_height; ty!=0; ty-- )
	{
		for( tx=tile_width; tx!=0; tx-- )
		{
			yx = *pPenToPixel++;
			x = x0+(yx%MAX_TILESIZE);
			y = y0+(yx/MAX_TILESIZE);
			if( bDontIgnoreTransparency && (pBitmask[bitoffs/8]&(0x80>>(bitoffs&7))) == 0 )
			{
				((UINT8 *)transparency_bitmap->line[y])[x] = code_transparent;
				bWhollyOpaque = 0;
			}
			else
			{
				((UINT8 *)transparency_bitmap->line[y])[x] = code_opaque;
				bWhollyTransparent = 0;
			}
			bitoffs++;
		}
	}

	return (bWhollyOpaque || bWhollyTransparent)?0:TILE_FLAG_FG_OPAQUE;
}

static UINT8 TRANSP(HandleTransparencyColor)(struct tilemap *tilemap, UINT32 x0, UINT32 y0, UINT32 flags)
{
	UINT32 tile_width = tilemap->cached_tile_width;
	UINT32 tile_height = tilemap->cached_tile_height;
	struct mame_bitmap *pixmap = tilemap->pixmap;
	struct mame_bitmap *transparency_bitmap = tilemap->transparency_bitmap;
	int pitch = tile_width + tile_info.skip;
	PAL_INIT;
	UINT32 *pPenToPixel = tilemap->pPenToPixel[flags&(TILE_FLIPY|TILE_FLIPX)];
	const UINT8 *pPenData = tile_info.pen_data;
	const UINT8 *pSource;
	UINT32 code_transparent = tile_info.priority;
	UINT32 code_opaque = code_transparent | TILE_FLAG_FG_OPAQUE;
	UINT32 tx;
	UINT32 ty;
	UINT32 data;
	UINT32 yx;
	UINT32 x;
	UINT32 y;
	UINT32 pen;
	UINT32 transparent_color = tilemap->transparent_pen;
	int bWhollyOpaque;
	int bWhollyTransparent;

	bWhollyOpaque = 1;
	bWhollyTransparent = 1;

	if( flags&TILE_4BPP )
	{
		for( ty=tile_height; ty!=0; ty-- )
		{
			pSource = pPenData;
			for( tx=tile_width/2; tx!=0; tx-- )
			{
				data = *pSource++;

				pen = data&0xf;
				yx = *pPenToPixel++;
				x = x0+(yx%MAX_TILESIZE);
				y = y0+(yx/MAX_TILESIZE);
				*(x+(UINT16 *)pixmap->line[y]) = PAL_GET(pen);
				if( PAL_GET(pen)==transparent_color )
				{
					((UINT8 *)transparency_bitmap->line[y])[x] = code_transparent;
					bWhollyOpaque = 0;
				}
				else
				{
					((UINT8 *)transparency_bitmap->line[y])[x] = code_opaque;
					bWhollyTransparent = 0;
				}

				pen = data>>4;
				yx = *pPenToPixel++;
				x = x0+(yx%MAX_TILESIZE);
				y = y0+(yx/MAX_TILESIZE);
				*(x+(UINT16 *)pixmap->line[y]) = PAL_GET(pen);
				if( PAL_GET(pen)==transparent_color )
				{
					((UINT8 *)transparency_bitmap->line[y])[x] = code_transparent;
					bWhollyOpaque = 0;
				}
				else
				{
					((UINT8 *)transparency_bitmap->line[y])[x] = code_opaque;
					bWhollyTransparent = 0;
				}
			}
			pPenData += pitch/2;
		}
	}
	else
	{
		for( ty=tile_height; ty!=0; ty-- )
		{
			pSource = pPenData;
			for( tx=tile_width; tx!=0; tx-- )
			{
				pen = *pSource++;
				yx = *pPenToPixel++;
				x = x0+(yx%MAX_TILESIZE);
				y = y0+(yx/MAX_TILESIZE);
				*(x+(UINT16 *)pixmap->line[y]) = PAL_GET(pen);
				if( PAL_GET(pen)==transparent_color )
				{
					((UINT8 *)transparency_bitmap->line[y])[x] = code_transparent;
					bWhollyOpaque = 0;
				}
				else
				{
					((UINT8 *)transparency_bitmap->line[y])[x] = code_opaque;
					bWhollyTransparent = 0;
				}
			}
			pPenData += pitch;
		}
	}
	return (bWhollyOpaque || bWhollyTransparent)?0:TILE_FLAG_FG_OPAQUE;
}

static UINT8 TRANSP(HandleTransparencyPen)(struct tilemap *tilemap, UINT32 x0, UINT32 y0, UINT32 flags)
{
	UINT32 tile_width = tilemap->cached_tile_width;
	UINT32 tile_height = tilemap->cached_tile_height;
	struct mame_bitmap *pixmap = tilemap->pixmap;
	struct mame_bitmap *transparency_bitmap = tilemap->transparency_bitmap;
	int pitch = tile_width + tile_info.skip;
	PAL_INIT;
	UINT32 *pPenToPixel = tilemap->pPenToPixel[flags&(TILE_FLIPY|TILE_FLIPX)];
	const UINT8 *pPenData = tile_info.pen_data;
	const UINT8 *pSource;
	UINT32 code_transparent = tile_info.priority;
	UINT32 code_opaque = code_transparent | TILE_FLAG_FG_OPAQUE;
	UINT32 tx;
	UINT32 ty;
	UINT32 data;
	UINT32 yx;
	UINT32 x;
	UINT32 y;
	UINT32 pen;
	UINT32 transparent_pen = tilemap->transparent_pen;
	int bWhollyOpaque;
	int bWhollyTransparent;

	bWhollyOpaque = 1;
	bWhollyTransparent = 1;

	if( flags&TILE_IGNORE_TRANSPARENCY )
	{
		transparent_pen = ~0;
	}

	if( flags&TILE_4BPP )
	{
		for( ty=tile_height; ty!=0; ty-- )
		{
			pSource = pPenData;
			for( tx=tile_width/2; tx!=0; tx-- )
			{
				data = *pSource++;

				pen = data&0xf;
				yx = *pPenToPixel++;
				x = x0+(yx%MAX_TILESIZE);
				y = y0+(yx/MAX_TILESIZE);
				*(x+(UINT16 *)pixmap->line[y]) = PAL_GET(pen);
				if( pen==transparent_pen )
				{
					((UINT8 *)transparency_bitmap->line[y])[x] = code_transparent;
					bWhollyOpaque = 0;
				}
				else
				{
					((UINT8 *)transparency_bitmap->line[y])[x] = code_opaque;
					bWhollyTransparent = 0;
				}

				pen = data>>4;
				yx = *pPenToPixel++;
				x = x0+(yx%MAX_TILESIZE);
				y = y0+(yx/MAX_TILESIZE);
				*(x+(UINT16 *)pixmap->line[y]) = PAL_GET(pen);
				((UINT8 *)transparency_bitmap->line[y])[x] = (pen==transparent_pen)?code_transparent:code_opaque;
			}
			pPenData += pitch/2;
		}
	}
	else
	{
		for( ty=tile_height; ty!=0; ty-- )
		{
			pSource = pPenData;
			for( tx=tile_width; tx!=0; tx-- )
			{
				pen = *pSource++;
				yx = *pPenToPixel++;
				x = x0+(yx%MAX_TILESIZE);
				y = y0+(yx/MAX_TILESIZE);
				*(x+(UINT16 *)pixmap->line[y]) = PAL_GET(pen);
				if( pen==transparent_pen )
				{
					((UINT8 *)transparency_bitmap->line[y])[x] = code_transparent;
					bWhollyOpaque = 0;

				}
				else
				{
					((UINT8 *)transparency_bitmap->line[y])[x] = code_opaque;
					bWhollyTransparent = 0;
				}
			}
			pPenData += pitch;
		}
	}

	return (bWhollyOpaque || bWhollyTransparent)?0:TILE_FLAG_FG_OPAQUE;
}

static UINT8 TRANSP(HandleTransparencyPenBit)(struct tilemap *tilemap, UINT32 x0, UINT32 y0, UINT32 flags)
{
	UINT32 tile_width = tilemap->cached_tile_width;
	UINT32 tile_height = tilemap->cached_tile_height;
	struct mame_bitmap *pixmap = tilemap->pixmap;
	struct mame_bitmap *transparency_bitmap = tilemap->transparency_bitmap;
	int pitch = tile_width + tile_info.skip;
	PAL_INIT;
	UINT32 *pPenToPixel = tilemap->pPenToPixel[flags&(TILE_FLIPY|TILE_FLIPX)];
	const UINT8 *pPenData = tile_info.pen_data;
	const UINT8 *pSource;
	UINT32 tx;
	UINT32 ty;
	UINT32 data;
	UINT32 yx;
	UINT32 x;
	UINT32 y;
	UINT32 pen;
	UINT32 penbit = tilemap->transparent_pen;
	UINT32 code_front = tile_info.priority | TILE_FLAG_FG_OPAQUE;
	UINT32 code_back = tile_info.priority | TILE_FLAG_BG_OPAQUE;
	int code;
	int and_flags = ~0;
	int or_flags = 0;

	if( flags&TILE_4BPP )
	{
		for( ty=tile_height; ty!=0; ty-- )
		{
			pSource = pPenData;
			for( tx=tile_width/2; tx!=0; tx-- )
			{
				data = *pSource++;

				pen = data&0xf;
				yx = *pPenToPixel++;
				x = x0+(yx%MAX_TILESIZE);
				y = y0+(yx/MAX_TILESIZE);
				*(x+(UINT16 *)pixmap->line[y]) = PAL_GET(pen);
				code = ((pen&penbit)==penbit)?code_front:code_back;
				and_flags &= code;
				or_flags |= code;
				((UINT8 *)transparency_bitmap->line[y])[x] = code;

				pen = data>>4;
				yx = *pPenToPixel++;
				x = x0+(yx%MAX_TILESIZE);
				y = y0+(yx/MAX_TILESIZE);
				*(x+(UINT16 *)pixmap->line[y]) = PAL_GET(pen);
				code = ((pen&penbit)==penbit)?code_front:code_back;
				and_flags &= code;
				or_flags |= code;
				((UINT8 *)transparency_bitmap->line[y])[x] = code;
			}
			pPenData += pitch/2;
		}
	}
	else
	{
		for( ty=tile_height; ty!=0; ty-- )
		{
			pSource = pPenData;
			for( tx=tile_width; tx!=0; tx-- )
			{
				pen = *pSource++;
				yx = *pPenToPixel++;
				x = x0+(yx%MAX_TILESIZE);
				y = y0+(yx/MAX_TILESIZE);
				*(x+(UINT16 *)pixmap->line[y]) = PAL_GET(pen);
				code = ((pen&penbit)==penbit)?code_front:code_back;
				and_flags &= code;
				or_flags |= code;
				((UINT8 *)transparency_bitmap->line[y])[x] = code;
			}
			pPenData += pitch;
		}
	}
	return or_flags ^ and_flags;
}

static UINT8 TRANSP(HandleTransparencyPens)(struct tilemap *tilemap, UINT32 x0, UINT32 y0, UINT32 flags)
{
	UINT32 tile_width = tilemap->cached_tile_width;
	UINT32 tile_height = tilemap->cached_tile_height;
	struct mame_bitmap *pixmap = tilemap->pixmap;
	struct mame_bitmap *transparency_bitmap = tilemap->transparency_bitmap;
	int pitch = tile_width + tile_info.skip;
	PAL_INIT;
	UINT32 *pPenToPixel = tilemap->pPenToPixel[flags&(TILE_FLIPY|TILE_FLIPX)];
	const UINT8 *pPenData = tile_info.pen_data;
	const UINT8 *pSource;
	UINT32 code_transparent = tile_info.priority;
	UINT32 tx;
	UINT32 ty;
	UINT32 data;
	UINT32 yx;
	UINT32 x;
	UINT32 y;
	UINT32 pen;
	UINT32 fgmask = tilemap->fgmask[(flags>>TILE_SPLIT_OFFSET)&3];
	UINT32 bgmask = tilemap->bgmask[(flags>>TILE_SPLIT_OFFSET)&3];
	UINT32 code;
	int and_flags = ~0;
	int or_flags = 0;

	if( flags&TILE_4BPP )
	{
		for( ty=tile_height; ty!=0; ty-- )
		{
			pSource = pPenData;
			for( tx=tile_width/2; tx!=0; tx-- )
			{
				data = *pSource++;

				pen = data&0xf;
				yx = *pPenToPixel++;
				x = x0+(yx%MAX_TILESIZE);
				y = y0+(yx/MAX_TILESIZE);
				*(x+(UINT16 *)pixmap->line[y]) = PAL_GET(pen);
				code = code_transparent;
				if( !((1<<pen)&fgmask) ) code |= TILE_FLAG_FG_OPAQUE;
				if( !((1<<pen)&bgmask) ) code |= TILE_FLAG_BG_OPAQUE;
				and_flags &= code;
				or_flags |= code;
				((UINT8 *)transparency_bitmap->line[y])[x] = code;

				pen = data>>4;
				yx = *pPenToPixel++;
				x = x0+(yx%MAX_TILESIZE);
				y = y0+(yx/MAX_TILESIZE);
				*(x+(UINT16 *)pixmap->line[y]) = PAL_GET(pen);
				code = code_transparent;
				if( !((1<<pen)&fgmask) ) code |= TILE_FLAG_FG_OPAQUE;
				if( !((1<<pen)&bgmask) ) code |= TILE_FLAG_BG_OPAQUE;
				and_flags &= code;
				or_flags |= code;
				((UINT8 *)transparency_bitmap->line[y])[x] = code;
			}
			pPenData += pitch/2;
		}
	}
	else
	{
		for( ty=tile_height; ty!=0; ty-- )
		{
			pSource = pPenData;
			for( tx=tile_width; tx!=0; tx-- )
			{
				pen = *pSource++;
				yx = *pPenToPixel++;
				x = x0+(yx%MAX_TILESIZE);
				y = y0+(yx/MAX_TILESIZE);
				*(x+(UINT16 *)pixmap->line[y]) = PAL_GET(pen);
				code = code_transparent;
				if( !((1<<pen)&fgmask) ) code |= TILE_FLAG_FG_OPAQUE;
				if( !((1<<pen)&bgmask) ) code |= TILE_FLAG_BG_OPAQUE;
				and_flags &= code;
				or_flags |= code;
				((UINT8 *)transparency_bitmap->line[y])[x] = code;
			}
			pPenData += pitch;
		}
	}
	return and_flags ^ or_flags;
}

static UINT8 TRANSP(HandleTransparencyNone)(struct tilemap *tilemap, UINT32 x0, UINT32 y0, UINT32 flags)
{
	UINT32 tile_width = tilemap->cached_tile_width;
	UINT32 tile_height = tilemap->cached_tile_height;
	struct mame_bitmap *pixmap = tilemap->pixmap;
	struct mame_bitmap *transparency_bitmap = tilemap->transparency_bitmap;
	int pitch = tile_width + tile_info.skip;
	PAL_INIT;
	UINT32 *pPenToPixel = tilemap->pPenToPixel[flags&(TILE_FLIPY|TILE_FLIPX)];
	const UINT8 *pPenData = tile_info.pen_data;
	const UINT8 *pSource;
	UINT32 code_opaque = tile_info.priority;
	UINT32 tx;
	UINT32 ty;
	UINT32 data;
	UINT32 yx;
	UINT32 x;
	UINT32 y;
	UINT32 pen;

	if( flags&TILE_4BPP )
	{
		for( ty=tile_height; ty!=0; ty-- )
		{
			pSource = pPenData;
			for( tx=tile_width/2; tx!=0; tx-- )
			{
				data = *pSource++;

				pen = data&0xf;
				yx = *pPenToPixel++;
				x = x0+(yx%MAX_TILESIZE);
				y = y0+(yx/MAX_TILESIZE);
				*(x+(UINT16 *)pixmap->line[y]) = PAL_GET(pen);
				((UINT8 *)transparency_bitmap->line[y])[x] = code_opaque;

				pen = data>>4;
				yx = *pPenToPixel++;
				x = x0+(yx%MAX_TILESIZE);
				y = y0+(yx/MAX_TILESIZE);
				*(x+(UINT16 *)pixmap->line[y]) = PAL_GET(pen);
				((UINT8 *)transparency_bitmap->line[y])[x] = code_opaque;
			}
			pPenData += pitch/2;
		}
	}
	else
	{
		for( ty=tile_height; ty!=0; ty-- )
		{
			pSource = pPenData;
			for( tx=tile_width; tx!=0; tx-- )
			{
				pen = *pSource++;
				yx = *pPenToPixel++;
				x = x0+(yx%MAX_TILESIZE);
				y = y0+(yx/MAX_TILESIZE);
				*(x+(UINT16 *)pixmap->line[y]) = PAL_GET(pen);
				((UINT8 *)transparency_bitmap->line[y])[x] = code_opaque;
			}
			pPenData += pitch;
		}
	}
	return 0;
}

#undef TRANSP
#undef PAL_INIT
#undef PAL_GET
#endif /* TRANSP*/

