#include "driver.h"
#include "state.h"
#include "vidhrdw/generic.h"
#include "namcos2.h"	/* for game-specific hacks */
#include "namcoic.h"

static data16_t mSpritePos[4];

WRITE16_HANDLER( namco_spritepos16_w )
{
	COMBINE_DATA( &mSpritePos[offset] );
}
READ16_HANDLER( namco_spritepos16_r )
{
	return mSpritePos[offset];
}

WRITE32_HANDLER( namco_spritepos32_w )
{
	data32_t v;
	offset *= 2;
	v = (mSpritePos[offset]<<16)|mSpritePos[offset+1];
	COMBINE_DATA( &v );
	mSpritePos[offset+0] = v>>16;
	mSpritePos[offset+1] = v&0xffff;
}
READ32_HANDLER( namco_spritepos32_r )
{
	offset *= 2;
	return (mSpritePos[offset]<<16)|mSpritePos[offset+1];
}

static INLINE data8_t
nth_byte16( const data16_t *pSource, int which )
{
	data16_t data = pSource[which/2];
	if( which&1 )
	{
		return data&0xff;
	}
	else
	{
		return data>>8;
	}
} /* nth_byte16 */

/* nth_word32 is a general-purpose utility function, which allows us to
 * read from 32-bit aligned memory as if it were an array of 16 bit words.
 */
static INLINE data16_t
nth_word32( const data32_t *pSource, int which )
{
	data32_t data = pSource[which/2];
	if( which&1 )
	{
		return data&0xffff;
	}
	else
	{
		return data>>16;
	}
} /* nth_word32 */

/* nth_byte32 is a general-purpose utility function, which allows us to
 * read from 32-bit aligned memory as if it were an array of bytes.
 */
static INLINE data8_t
nth_byte32( const data32_t *pSource, int which )
{
		data32_t data = pSource[which/4];
		switch( which&3 )
		{
		case 0: return data>>24;
		case 1: return (data>>16)&0xff;
		case 2: return (data>>8)&0xff;
		default: return data&0xff;
		}
} /* nth_byte32 */

/**************************************************************************************************************/

static int (*mpCodeToTile)( int code ); /* sprite banking callback */
static int mGfxC355;	/* gfx bank for sprites */
static int mPalXOR;		/* XOR'd with palette select register; needed for System21 */

/**
 * 0x00000 sprite attr (page0)
 * 0x02000 sprite list (page0)
 * 0x02400 window attributes
 * 0x04000 format
 * 0x08000 tile
 * 0x10000 sprite attr (page1)
 * 0x14000 sprite list (page1)
 */
static void
draw_spriteC355( int page, struct mame_bitmap *bitmap, const struct rectangle *cliprect, const data16_t *pSource, int pri, int zpos )
{
	unsigned screen_height_remaining, screen_width_remaining;
	unsigned source_height_remaining, source_width_remaining;
	INT32 hpos,vpos;
	data16_t hsize,vsize;
	data16_t palette;
	data16_t linkno;
	data16_t offset;
	data16_t format;
	int tile_index;
	int num_cols,num_rows;
	int dx,dy;
	int row,col;
	int sx,sy,tile;
	int flipx,flipy;
	UINT32 zoomx, zoomy;
	int tile_screen_width;
	int tile_screen_height;
	const data16_t *spriteformat16 = &spriteram16[0x4000/2];
	const data16_t *spritetile16 = &spriteram16[0x8000/2];
	int color;

	struct rectangle clip;

	/**
	 * ----xxxx-------- window select
	 * --------xxxx---- priority
	 * ------------xxxx palette select
	 */
	palette = pSource[6];
	switch( namcos2_gametype )
	{
	case NAMCONB2_OUTFOXIES:
	case NAMCONB2_MACH_BREAKERS:
	case NAMCOS2_SUZUKA_8_HOURS_2:
	case NAMCOS2_SUZUKA_8_HOURS:
	case NAMCOS2_LUCKY_AND_WILD:
		if( pri != ((palette>>5)&7) ) return;
		break;

	default:
		if( pri != ((palette>>4)&0x7) ) return;
		break;
	}

	linkno		= pSource[0]; /* LINKNO */
	offset		= pSource[1]; /* OFFSET */
	hpos		= pSource[2]; /* HPOS		0x000..0x7ff (signed) */
	vpos		= pSource[3]; /* VPOS		0x000..0x7ff (signed) */
	hsize		= pSource[4]; /* HSIZE		max 0x3ff pixels */
	vsize		= pSource[5]; /* VSIZE		max 0x3ff pixels */
	/* pSource[6] contains priority/palette */
	/* pSource[7] is used in Lucky & Wild, possibly for sprite-road priority */

	if( linkno*4>=0x4000/2 ) return; /* avoid garbage memory read */

	switch( namcos2_gametype )
	{
	case NAMCOS21_SOLVALOU: /* hack */
		hpos -= 0x80;
		vpos -= 0x40;
		clip = Machine->visible_area;
		break;

	case NAMCOS21_CYBERSLED: /* hack */
		hpos -= 0x110;
		vpos -= 2+32;
		clip = Machine->visible_area;
		break;

	case NAMCOS21_AIRCOMBAT: /* hack */
		vpos -= 0x22;
		hpos -= 0x02;
		clip = Machine->visible_area;
		break;

	case NAMCOS21_STARBLADE: /* hack */
		if( page )
		{
			hpos -= 0x80;
			vpos -= 0x20;
		}
		clip = Machine->visible_area;
		break;

	case NAMCONB1_NEBULRAY:
	case NAMCONB1_GUNBULET:
	case NAMCONB1_GSLGR94U:
	case NAMCONB1_SWS95:
	case NAMCONB1_SWS96:
	case NAMCONB1_SWS97:
	case NAMCONB2_OUTFOXIES:
	case NAMCONB2_MACH_BREAKERS:
	case NAMCONB1_VSHOOT:
	case NAMCOS2_SUZUKA_8_HOURS_2:
	case NAMCOS2_SUZUKA_8_HOURS:
	case NAMCOS2_LUCKY_AND_WILD:
	case NAMCOS2_STEEL_GUNNER_2:
	default:
		{
			int dh = mSpritePos[1];
			int dv = mSpritePos[0];
			const data16_t *pWinAttr = &spriteram16[0x2400/2+((palette>>8)&0xf)*4];

			dh &= 0x1ff; if( dh&0x100 ) dh |= ~0x1ff;
			dv &= 0x1ff; if( dv&0x100 ) dv |= ~0x1ff;
			vpos&=0x7ff; if( vpos&0x400 ) vpos |= ~0x7ff;
			hpos&=0x7ff; if( hpos&0x400 ) hpos |= ~0x7ff;
			hpos += -0x26 - dh;
			vpos += -0x19 - dv;
			/* 0026 0145 0019 00f8 (lucky&wild)*/
			/* 0025 0145 0019 00f8 (point blank)*/
			clip.min_x = pWinAttr[0] - 0x26 - dh;
			clip.max_x = pWinAttr[1] - 0x26 - dh;
			clip.min_y = pWinAttr[2] - 0x19 - dv;
			clip.max_y = pWinAttr[3] - 0x19 - dv;
			if( clip.min_x < cliprect->min_x ) clip.min_x = cliprect->min_x;
			if( clip.min_y < cliprect->min_y ) clip.min_y = cliprect->min_y;
			if( clip.max_x > cliprect->max_x ) clip.max_x = cliprect->max_x;
			if( clip.max_y > cliprect->max_y ) clip.max_y = cliprect->max_y;
		}
		break;
	}
	tile_index		= spriteformat16[linkno*4+0];
	format			= spriteformat16[linkno*4+1];
	dx				= spriteformat16[linkno*4+2];
	dy				= spriteformat16[linkno*4+3];
	num_cols		= (format>>4)&0xf;
	num_rows		= (format)&0xf;

	if( num_cols == 0 ) num_cols = 0x10;
	flipx = (hsize&0x8000)?1:0;
	hsize &= 0x1ff;
	if( hsize == 0 ) return;

	zoomx = (hsize<<16)/(num_cols*16);
	dx = (dx*zoomx+0x8000)>>16;
	if( flipx )
	{
		hpos += dx;
	}
	else
	{
		hpos -= dx;
	}

	if( num_rows == 0 ) num_rows = 0x10;
	flipy = (vsize&0x8000)?1:0;
	vsize&=0x1ff;
	if( vsize == 0 ) return;

	zoomy = (vsize<<16)/(num_rows*16);
	dy = (dy*zoomy+0x8000)>>16;
	if( flipy )
	{
		vpos += dy;
	}
	else
	{
		vpos -= dy;
	}

	color = (palette&0xf)^mPalXOR;

	source_height_remaining = num_rows*16;
	screen_height_remaining = vsize;
	sy = vpos;
	for( row=0; row<num_rows; row++ )
	{
		tile_screen_height = 16*screen_height_remaining/source_height_remaining;
		zoomy = (screen_height_remaining<<16)/source_height_remaining;
		if( flipy )
		{
			sy -= tile_screen_height;
		}
		source_width_remaining = num_cols*16;
		screen_width_remaining = hsize;
		sx = hpos;
		for( col=0; col<num_cols; col++ )
		{
			tile_screen_width = 16*screen_width_remaining/source_width_remaining;
			zoomx = (screen_width_remaining<<16)/source_width_remaining;
			if( flipx )
			{
				sx -= tile_screen_width;
			}
			tile = spritetile16[tile_index++];
			if( (tile&0x8000)==0 )
			{
				/*z*/drawgfxzoom(bitmap,Machine->gfx[mGfxC355],
					mpCodeToTile(tile) + offset,
					color,
					flipx,flipy,
					sx,sy,
					&clip,
					TRANSPARENCY_PEN,0xff,
					zoomx, zoomy/*, zpos*/ );
			}
			if( !flipx )
			{
				sx += tile_screen_width;
			}
			screen_width_remaining -= tile_screen_width;
			source_width_remaining -= 16;
		} /* next col */
		if( !flipy )
		{
			sy += tile_screen_height;
		}
		screen_height_remaining -= tile_screen_height;
		source_height_remaining -= 16;
	} /* next row */
} /* draw_spriteC355 */


static int DefaultCodeToTile( int code )
{
	return code;
}

void namco_obj_init( int gfxbank, int palXOR, int (*codeToTile)( int code ) )
{
	mGfxC355 = gfxbank;
	mPalXOR = palXOR;
	if( codeToTile )
	{
		mpCodeToTile = codeToTile;
	}
	else
	{
		mpCodeToTile = DefaultCodeToTile;
	}
	spriteram16 = auto_malloc(0x14200);
	memset( mSpritePos,0x00,sizeof(mSpritePos) );
} /* namcosC355_init */

static void
DrawObjectList(
		struct mame_bitmap *bitmap,
		const struct rectangle *cliprect,
		int pri,
		const data16_t *pSpriteList16,
		const data16_t *pSpriteTable,
		int n )
{
	data16_t which;
	int i;
	int count = 0;
	/* count the sprites */
	for( i=0; i<256; i++ )
	{
		which = pSpriteList16[i];
		count++;
		if( which&0x100 ) break;
	}
	/* draw the sprites */
	for( i=0; i<count; i++ )
	{
		which = pSpriteList16[i];
		draw_spriteC355( n, bitmap, cliprect, &pSpriteTable[(which&0xff)*8], pri, i );
	}
} /* DrawObjectList */

void
namco_obj_draw( struct mame_bitmap *bitmap, const struct rectangle *cliprect, int pri )
{
	DrawObjectList( bitmap,cliprect,pri,&spriteram16[0x02000/2], &spriteram16[0x00000/2],0 );
	DrawObjectList( bitmap,cliprect,pri,&spriteram16[0x14000/2], &spriteram16[0x10000/2],1 );
} /* namco_obj_draw */

WRITE16_HANDLER( namco_obj16_w )
{
	COMBINE_DATA( &spriteram16[offset] );
} /* namco_obj16_w */

READ16_HANDLER( namco_obj16_r )
{
	return spriteram16[offset];
} /* namco_obj16_r */

WRITE32_HANDLER( namco_obj32_w )
{
	data32_t v;
	offset *= 2;
	v = (spriteram16[offset]<<16)|spriteram16[offset+1];
	COMBINE_DATA( &v );
	spriteram16[offset] = v>>16;
	spriteram16[offset+1] = v&0xffff;
} /* namco_obj32_w */

READ32_HANDLER( namco_obj32_r )
{
	offset *= 2;
	return (spriteram16[offset]<<16)|spriteram16[offset+1];
} /* namco_obj32_r */

/**************************************************************************************************************/

/* ROZ abstraction (preliminary)
 *
 * Used by:
 *	Namco NB2 - The Outfoxies, Mach Breakers
 *	Namco System 2 - Metal Hawk, Lucky and Wild
 */
static data16_t *rozbank16;
static data16_t *rozvideoram16;
static data16_t *rozcontrol16;
static int mRozGfxBank;
static int mRozMaskRegion;
static struct tilemap *mRozTilemap[2];
static int mRozPage[2];		/* base addr for tilemap */

/* It looks like the ROZ tilemap attributes also encode
 * the source size.  Right now, the implementation assumes ROZ
 * layers are always 128x128 tiles.
 */
static void
roz_get_info( int tile_index,int which )
{
	data16_t tile;
	int bank;
	int mangle;
	/* when size control is understood, we can simulate it by masking column and row, which will
	 * mirror the contents of the ROZ tilemap.
	 */
	tile = rozvideoram16[mRozPage[which]+tile_index];

	switch( namcos2_gametype )
	{
	case NAMCONB2_MACH_BREAKERS:
		bank = nth_byte16( &rozbank16[which*8/2], (tile>>11)&0x7 );
		tile = (tile&0x7ff)|(bank*0x800);
		mangle = tile;
		break;

	case NAMCONB2_OUTFOXIES:
		bank = nth_byte16( &rozbank16[which*8/2], (tile>>11)&0x7 );
		tile = (tile&0x7ff)|(bank*0x800);
		mangle = tile&~(0x50);
		/* the pixmap index is mangled, the transparency bitmask index is not */
		if( tile&0x10 ) mangle |= 0x40;
		if( tile&0x40 ) mangle |= 0x10;
		break;

	case NAMCOS2_LUCKY_AND_WILD:
		mangle	= tile&0x01ff;
		tile &= 0x3fff;
		switch( tile>>9 )
		{
		case 0x00: mangle |= 0x1c00; break;
		case 0x01: mangle |= 0x0800; break;
		case 0x02: mangle |= 0x0000; break;

		case 0x08: mangle |= 0x1e00; break;
		case 0x09: mangle |= 0x0a00; break;
		case 0x0a: mangle |= 0x0200; break;

		case 0x10: mangle |= 0x2000; break;
		case 0x11: mangle |= 0x0c00; break;
		case 0x12: mangle |= 0x0400; break;

		case 0x18: mangle |= 0x2200; break;
		case 0x19: mangle |= 0x0e00; break;
		case 0x1a: mangle |= 0x0600; break;
		}
		break;

	case NAMCOS2_METAL_HAWK:
	default:
		mangle = tile&0x01ff;
		if( tile&0x1000 ) mangle |= 0x0200;
		if( tile&0x0200 ) mangle |= 0x0400;
		if( tile&0x0400 ) mangle |= 0x0800;
		if( tile&0x0800 ) mangle |= 0x1000;
		break;
	}
	SET_TILE_INFO( mRozGfxBank,mangle,0/*color*/,0 );
	tile_info.mask_data = 32*tile + (UINT8 *)memory_region( mRozMaskRegion );
} /* roz_get_info */

static void roz_get_info0( int tile_index )
{
	roz_get_info( tile_index,0 );
}
static void roz_get_info1( int tile_index )
{
	roz_get_info( tile_index,1 );
}

int
namco_roz_init( int gfxbank, int maskregion )
{
	/* allocate both ROZ layers */
	int i;
	static void (*roz_info[2])(int tile_index) = { roz_get_info0, roz_get_info1 };

	mRozGfxBank = gfxbank;
	mRozMaskRegion = maskregion;

	rozbank16 = auto_malloc(0x10);
	rozvideoram16 = auto_malloc(0x20000);
	rozcontrol16 = auto_malloc(0x20);
	if( rozbank16 && rozvideoram16 && rozcontrol16 )
	{
		for( i=0; i<2; i++ )
		{
			mRozPage[i] = -1;
			mRozTilemap[i] = tilemap_create( roz_info[i], tilemap_scan_rows,
				TILEMAP_BITMASK,16,16,128,128 );

			if( mRozTilemap[i] == NULL ) return 1; /* error */
		}
		return 0;
	}
	return -1;
} /* namco_roz_init */

/**
 * ROZ control attributes:
 *
 * unk  attr   xx   xy   yx   yy   x0   y0
 *
 * Lucky & Wild:
 * 0000 080e 2100 0000 0000 0100 38e0 fde0	0:badge
 * 1000 084e 00fe 0000 0000 00fe f9f6 fd96	0:zooming car
 * 0000 080e 1100 0000 0000 0000 26d0 0450	0:"LUCKY & WILD"
 * 0000 03bf 0100 0000 0000 0100 fde0 7fe0	1:talking heads
 * 1000 080a 0100 0000 0000 0100 fde0 ffe0	0:player select
 *
 * Outfoxies:
 * 1000 0211 02ff 02ff 0000 0000 02ff f97b ffa7 (tv)
 * 0000 0211 00ff 0000 0000 0000 00ff fddb 0de7 (char select)
 * 1000 0271 0101 0000 0000 0101 0fc4 56d7 (stage)
 * 0000 0252 0101 4000 0000 0101 0fc4 d6d7 (stage)
 *
 * Mach Breakers:
 * 1000 0871 4100 0000 0000 0100 f4d0 f8e0
 * 0000 0210 0100 0000 0000 0100 40d0 38e0
 * 0000 0000 0100 0000 0000 0100 f4d0 f8e0 // charsel (512x512)
 * 0000 0020 0100 0000 0000 0100 f4d0 f8e0 // map
 */
void namco_roz_draw(
	struct mame_bitmap *bitmap,
	const struct rectangle *cliprect,
	int pri )
{
	const int xoffset = 38-2,yoffset = 3;
	int which;
	for( which=0; which<2; which++ )
	{
		const data16_t *pSource = &rozcontrol16[which*8];
		data16_t attrs = pSource[1];
		if( (attrs&0x8000)==0 )
		{ /* layer is enabled */
			int color = attrs&0xf;
			int page;
			int roz_pri;
			/*int roz_size = 128;*/
			switch( namcos2_gametype )
			{
			case NAMCONB2_OUTFOXIES:
				roz_pri = 4-which; /* ? */
				page = pSource[3]&0x4000;
				if( attrs == 0x0211 ) roz_pri = 1; /* hack */
				break;

			case NAMCONB2_MACH_BREAKERS:
				roz_pri = 4-which; /* ? */
				page = (pSource[2]&0x6000)*2;
				break;

			case NAMCOS2_LUCKY_AND_WILD:
				roz_pri = 5-which; /* ? */
				page = (attrs&0x0800)?0:0x4000; /* ? */
				break;

			case NAMCOS2_METAL_HAWK:
			default:
				roz_pri = which; /* ? */
				page = pSource[3]&0x4000;
				break;
			}
			if( roz_pri==pri )
			{
				int bDirty;
				UINT32 startx,starty;
				int incxx,incxy,incyx,incyy;
				data16_t temp;

				temp = pSource[2];
				if( temp&0x8000 ) temp |= 0xf000; else temp&=0x0fff; /* sign extend */
				incxx = (INT16)temp;

				temp = pSource[3];
				if( temp&0x8000 ) temp |= 0xf000; else temp&=0x0fff; /* sign extend */
				incxy =  (INT16)temp;

				temp = pSource[4];
				if( temp&0x8000 ) temp |= 0xf000; else temp&=0x0fff; /* sign extend */
				incyx =  (INT16)temp;

				incyy =  (INT16)pSource[5];
				startx = (INT16)pSource[6];
				starty = (INT16)pSource[7];

				startx <<= 4;
				starty <<= 4;
				startx += xoffset * incxx + yoffset * incyx;
				starty += xoffset * incxy + yoffset * incyy;

				bDirty = 0;
				tilemap_set_palette_offset(mRozTilemap[which],color*256);
				if( mRozPage[which] != page )
				{
					mRozPage[which] = page;
					bDirty = 1;
				}
				if( bDirty )
				{
					tilemap_mark_all_tiles_dirty( mRozTilemap[which] );
				}

				tilemap_draw_roz(bitmap,cliprect,mRozTilemap[which],
					startx << 8,
					starty << 8,
					incxx << 8,
					incxy << 8,
					incyx << 8,
					incyy << 8,
					1,	/* copy with wraparound */
					0,0);
			}
		}
	}
} /* namco_roz_draw */

READ16_HANDLER( namco_rozcontrol16_r )
{
	return rozcontrol16[offset];
}

WRITE16_HANDLER( namco_rozcontrol16_w )
{
	COMBINE_DATA( &rozcontrol16[offset] );
}

READ16_HANDLER( namco_rozbank16_r )
{
	return rozbank16[offset];
}

WRITE16_HANDLER( namco_rozbank16_w )
{
	data16_t old_data;

	old_data = rozbank16[offset];
	COMBINE_DATA( &rozbank16[offset] );
	if( rozbank16[offset]!=old_data )
	{
		tilemap_mark_all_tiles_dirty( mRozTilemap[0] );
		tilemap_mark_all_tiles_dirty( mRozTilemap[1] );
	}
}

static void writerozvideo( int offset, data16_t data )
{
	int i;
	if( rozvideoram16[offset]!=data )
	{
		rozvideoram16[offset] = data;
		for( i=0; i<2; i++ )
		{
			if( mRozPage[i]==(offset&0xc000) )
			{
				tilemap_mark_tile_dirty( mRozTilemap[i], offset&0x3fff );
			}
		}
	}
}

READ16_HANDLER( namco_rozvideoram16_r )
{
	return rozvideoram16[offset];
}

WRITE16_HANDLER( namco_rozvideoram16_w )
{
	data16_t v;

	v = rozvideoram16[offset];
	COMBINE_DATA( &v );
	writerozvideo( offset, v );
}

READ32_HANDLER( namco_rozcontrol32_r )
{
	offset *= 2;
	return (rozcontrol16[offset]<<16)|rozcontrol16[offset+1];
}

WRITE32_HANDLER( namco_rozcontrol32_w )
{
	data32_t v;
	offset *=2;
	v = (rozcontrol16[offset]<<16)|rozcontrol16[offset+1];
	COMBINE_DATA(&v);
	rozcontrol16[offset] = v>>16;
	rozcontrol16[offset+1] = v&0xffff;
}

READ32_HANDLER( namco_rozbank32_r )
{
	offset *= 2;
	return (rozbank16[offset]<<16)|rozbank16[offset+1];
}

WRITE32_HANDLER( namco_rozbank32_w )
{
	data32_t v;
	offset *=2;
	v = (rozbank16[offset]<<16)|rozbank16[offset+1];
	COMBINE_DATA(&v);
	rozbank16[offset] = v>>16;
	rozbank16[offset+1] = v&0xffff;
}

READ32_HANDLER( namco_rozvideoram32_r )
{
	offset *= 2;
	return (rozvideoram16[offset]<<16)|rozvideoram16[offset+1];
}

WRITE32_HANDLER( namco_rozvideoram32_w )
{
	data32_t v;
	offset *= 2;
	v = (rozvideoram16[offset]<<16)|rozvideoram16[offset+1];
	COMBINE_DATA( &v );
	writerozvideo(offset,v>>16);
	writerozvideo(offset+1,v&0xffff);
}

/**************************************************************************************************************/
/*
	Land Line Buffer
	Land Generator
		0xf,0x7,0xe,0x6,0xd,0x5,0xc,0x4,
		0xb,0x3,0xa,0x2,0x9,0x1,0x8,0x0

*/

/* Preliminary!  The road circuitry is identical for all the driving games.
 *
 * There are several chunks of RAM
 *
 *	Road Tilemap:
 *		0x00000..0x0ffff	64x512 tilemap
 *
 *	Road Tiles:
 *		0x10000..0x1f9ff	16x16x2bpp tiles
 *
 *
 *	Line Attributes:
 *
 *		0x1fa00..0x1fbdf	xxx- ---- ---- ----		priority
 *							---- xxxx xxxx xxxx		xscroll
 *
 *		0x1fbfe				horizontal adjust?
 *							0x0017
 *							0x0018 (Final Lap3)
 *
 *		0x1fc00..0x1fddf	selects line in source bitmap
 *		0x1fdfe				yscroll
 *
 *		0x1fe00..0x1ffdf	---- --xx xxxx xxxx		zoomx
 *		0x1fffd				always 0xffff 0xffff?
 */
static data16_t *mpRoadRAM; /* at 0x880000 in Final Lap; at 0xa00000 in Lucky&Wild */
static unsigned char *mpRoadDirty;
static int mbRoadSomethingIsDirty;
static int mRoadGfxBank;
static struct tilemap *mpRoadTilemap;
static pen_t mRoadTransparentColor;
static int mbRoadNeedTransparent;

#define ROAD_COLS			64
#define ROAD_ROWS			512
#define ROAD_TILE_SIZE		16
#define ROAD_TILEMAP_WIDTH	(ROAD_TILE_SIZE*ROAD_COLS)
#define ROAD_TILEMAP_HEIGHT (ROAD_TILE_SIZE*ROAD_ROWS)

#define ROAD_TILE_COUNT_MAX	(0xfa00/0x40) /* 0x3e8 */
#define WORDS_PER_ROAD_TILE (0x40/2)

static struct GfxLayout RoadTileLayout =
{
	ROAD_TILE_SIZE,
	ROAD_TILE_SIZE,
	ROAD_TILE_COUNT_MAX,
	2,
	{
#ifdef MSB_FIRST
		0,8
#else
		8,0
#endif
	},
	{/* x offset */
		0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
		0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17
	},
	{/* y offset */
		0x000,0x020,0x040,0x060,0x080,0x0a0,0x0c0,0x0e0,
		0x100,0x120,0x140,0x160,0x180,0x1a0,0x1c0,0x1e0
	},
	0x200, /* offset to next tile */
};

void get_road_info( int tile_index )
{
	data16_t data = mpRoadRAM[tile_index];
	/* ------xx xxxxxxxx tile number
	 * xxxxxx-- -------- palette select
	 */
	int tile = (data&0x3ff);
	int color = (data>>10);

	SET_TILE_INFO( mRoadGfxBank, tile, color , 0 )
} /* get_road_info */

READ16_HANDLER( namco_road16_r )
{
	return mpRoadRAM[offset];
}

WRITE16_HANDLER( namco_road16_w )
{
	COMBINE_DATA( &mpRoadRAM[offset] );
	if( offset<0x10000/2 )
	{
		tilemap_mark_tile_dirty( mpRoadTilemap, offset );
	}
	else
	{
		offset -= 0x10000/2;
		if( offset<ROAD_TILE_COUNT_MAX*WORDS_PER_ROAD_TILE )
		{
			mpRoadDirty[offset/WORDS_PER_ROAD_TILE] = 1;
			mbRoadSomethingIsDirty = 1;
		}
	}
}

static void
UpdateRoad( void )
{
	int i;
	if( mbRoadSomethingIsDirty )
	{
		for( i=0; i<ROAD_TILE_COUNT_MAX; i++ )
		{
			if( mpRoadDirty[i] )
			{
				decodechar(
					Machine->gfx[mRoadGfxBank],
					i,
					0x10000+(UINT8 *)mpRoadRAM,
					&RoadTileLayout );
				mpRoadDirty[i] = 0;
			}
		}
		tilemap_mark_all_tiles_dirty( mpRoadTilemap );
		mbRoadSomethingIsDirty = 0;
	}
}

static void
RoadMarkAllDirty(void)
{
	memset( mpRoadDirty,0x01,ROAD_TILE_COUNT_MAX );
	mbRoadSomethingIsDirty = 1;
}

int
namco_road_init( int gfxbank )
{
	mbRoadNeedTransparent = 0;
	mRoadGfxBank = gfxbank;
	mpRoadDirty = auto_malloc(ROAD_TILE_COUNT_MAX);
	if( mpRoadDirty )
	{
		memset( mpRoadDirty,0x00,ROAD_TILE_COUNT_MAX );
		mbRoadSomethingIsDirty = 0;
		mpRoadRAM = auto_malloc(0x20000);
		if( mpRoadRAM )
		{
			struct GfxElement *pGfx = decodegfx( 0x10000+(UINT8 *)mpRoadRAM, &RoadTileLayout );
			if( pGfx )
			{
				pGfx->colortable = &Machine->remapped_colortable[0xf00];
				pGfx->total_colors = 0x3f;

				Machine->gfx[gfxbank] = pGfx;
				mpRoadTilemap = tilemap_create(
					get_road_info,tilemap_scan_rows,
					TILEMAP_OPAQUE,
					ROAD_TILE_SIZE,ROAD_TILE_SIZE,
					ROAD_COLS,ROAD_ROWS);

				if( mpRoadTilemap )
				{
					state_save_register_UINT8 ("namco_road", 0, "RoadDirty", mpRoadDirty, ROAD_TILE_COUNT_MAX);
					state_save_register_UINT16("namco_road", 0, "RoadRAM",   mpRoadRAM,   0x20000 / 2);
					state_save_register_func_postload(RoadMarkAllDirty);

					return 0;
				}
			}
		}
	}
	return -1;
} /* namco_road_init */

void
namco_road_set_transparent_color(pen_t pen)
{
	mbRoadNeedTransparent = 1;
	mRoadTransparentColor = pen;
}

void
namco_road_draw( struct mame_bitmap *bitmap, const struct rectangle *cliprect, int pri )
{
	struct mame_bitmap *pSourceBitmap;
	unsigned yscroll;
	int i;

	UpdateRoad();

	pSourceBitmap = tilemap_get_pixmap(mpRoadTilemap);
	yscroll = mpRoadRAM[0x1fdfe/2];

	for( i=cliprect->min_y; i<=cliprect->max_y; i++ )
	{
		UINT16 *pDest = bitmap->line[i];
		int screenx	= mpRoadRAM[0x1fa00/2+i+15];

		if( pri == ((screenx&0xe000)>>13) )
		{
			unsigned zoomx	= mpRoadRAM[0x1fe00/2+i+15]&0x3ff;
			if( zoomx )
			{
				unsigned sourcey = mpRoadRAM[0x1fc00/2+i+15]+yscroll;
				const UINT16 *pSourceGfx = pSourceBitmap->line[sourcey&(ROAD_TILEMAP_HEIGHT-1)];
				unsigned dsourcex = (ROAD_TILEMAP_WIDTH<<16)/zoomx;
				unsigned sourcex = 0;
				int numpixels = (44*ROAD_TILE_SIZE<<16)/dsourcex;

				/* draw this scanline */
				screenx &= 0x0fff; /* mask off priority bits */
				if( screenx&0x0800 )
				{
					/* sign extend */
					screenx -= 0x1000;
				}

				/* adjust the horizontal placement */
				screenx -= 64; /*needs adjustment to left*/

				if( screenx<0 )
				{ /* crop left */
					numpixels += screenx;
					sourcex -= dsourcex*screenx;
					screenx = 0;
				}

				if( screenx + numpixels > bitmap->width )
				{ /* crop right */
					numpixels = bitmap->width - screenx;
				}

				/* BUT: support transparent color for Thunder Ceptor */
				if (mbRoadNeedTransparent)
				{
					while( numpixels-- > 0 )
					{
						int pen = pSourceGfx[sourcex>>16];
						/* TBA: work out palette mapping for Final Lap, Suzuka */
						if (pen != mRoadTransparentColor)
							pDest[screenx] = pen;
						screenx++;
						sourcex += dsourcex;
					}
				}
				else
				{
					while( numpixels-- > 0 )
					{
						int pen = pSourceGfx[sourcex>>16];
						/* TBA: work out palette mapping for Final Lap, Suzuka */
						pDest[screenx++] = pen;
						sourcex += dsourcex;
					}
				}
			}
		}
	}
} /* namco_road_draw */
