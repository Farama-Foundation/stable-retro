/***************************************************************************

Namco System 21 Video Hardware

- sprite Hardware is identical to Namco System NB1
- there are no tilemaps
- polygons are drawn by DSP processors

The main CPUs populate a chunk of shared RAM with an object display list.
The object display list contains references to specific 3d objects and their
position/orientation in 3d space.

The main CPUs also specify attributes for a master camera which provides
additional (optional) global transformations.

---------------------------------------------------------------------------
memory map for DSP RAM (shared with the 68000 CPUs):

	0x200000..0x2000ff	populated with ASCII Text during self-tests:
		ROM:
		RAM:
		PTR:
		SMU:
		IDC:
		CPU:ABORT
		DSP:
		CRC:OK  from cpu
		CRC:    from dsp
		ID:
		B-M:	P-M:	S-M:	SET UP

	0x200100	status
	0x200102	status
	0x20010a	checksum (starblade expects 0xed53)
	0x20010c	checksum (starblade expects 0xd5df)
	0x20010e	ack
	0x200110	status
	0x200112	status
	0x200202	status
	0x200206	work page select

	0x208000..0x2080ff	camera attributes for page#0
	0x208200..0x208fff	3d object attribute list for page#0

	0x20c000..0x20c0ff	camera attributes for page#1
	0x20c200..0x20cfff	3d object attribute list for page#1
---------------------------------------------------------------------------

Thanks to Aaron Giles for originally making sense of the Point ROM data:

Point data in ROMS (signed 24 bit words) encodes the 3d primitives.

The first part of the Point ROMs is an address table.  The first entry serves
a special (unknown) purpose.

Given an object index, this table provides an address into the second part of
the ROM.

The second part of the ROM is a series of display lists.
This is a sequence of pointers to actual polygon data. There may be
more than one, and the list is terminated by $ffffff.

The remainder of the ROM is a series of polygon data. The first word of each
entry is the length of the entry (in words, not counting the length word).

The rest of the data in each entry is organized as follows:

length (1 word)
unknown value (1 word) - this increments with each entry
vertex count (1 word) - the number of vertices encoded
unknown value (1 word) - almost always 0; possibly depth bias
vertex list (n x 3 words)
quad count (1 word) - the number of quads to draw
quad primitives (n x 5 words) - indices of four verticies plus color code
*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "namcos2.h"
#include "namcoic.h"
#include <assert.h>
#include "namcos3d.h"
#include "matrix3d.h"

#define MAX_SURFACE 64
#define MAX_VERTEX	64

#define kScreenWidth	62*8
#define kScreenHeight	60*8

static int mbDspError;

static data16_t namcos21_polyattr0[1]; /* ??? */
static data16_t namcos21_polyattr1[1]; /* ??? */

static data16_t namcos21_objattr[8];
/* fff0 0000 during startup tests (all)
 * 0000 8000 (starblade)
 * ffff 0080 (solvalou)
 * 0000 0000 (cybersled, aircombat)
 */

WRITE16_HANDLER( namcos21_polyattr0_w )
{
	COMBINE_DATA( &namcos21_polyattr0[offset] );
}
WRITE16_HANDLER( namcos21_polyattr1_w )
{
	COMBINE_DATA( &namcos21_polyattr1[offset] );
}
WRITE16_HANDLER( namcos21_objattr_w )
{
	COMBINE_DATA( &namcos21_objattr[offset] );
}

static void
DrawQuad( struct mame_bitmap *pBitmap, struct VerTex *verTex, int vi[4], unsigned color )
{
	struct VerTex vertex[5];
	int i;
	for( i=0; i<5; i++ )
	{
		vertex[i] = verTex[vi[i&3]];
	}
	BlitTriFlat( pBitmap, &vertex[0], color );
	BlitTriFlat( pBitmap, &vertex[2], color );
} /* DrawQuad */

static void
BlitPolyObject( struct mame_bitmap *bitmap, int code, double M[4][4] )
{
	const INT32 *pPointData = (INT32 *)memory_region( REGION_USER2 );
	INT32 masterAddr = pPointData[code];
	struct VerTex vertex[MAX_VERTEX];
	int vi[4];

	if( code<3 || code>=pPointData[0] )
	{ /* Trap out-of-range polyobj reference; otherwise we may read illegal memory during
	   * self test.  There's probably a master DSP enable/disable register involved,
	   * but I don't know it.
	   */
		mbDspError = 1;
		return;
	}

	for(;;)
	{
		INT32 subAddr = pPointData[masterAddr++];
		if( subAddr<0 )
		{
			break;
		}
		else
		{
			INT32 vertexCount, surfaceCount;
			unsigned color;
			int count;

			subAddr++; /* number of subsequant words in this chunk */
			subAddr++; /* unique id tag */
			vertexCount	= pPointData[subAddr++]&0xff;
			subAddr++; /* unknown */

			if( vertexCount>MAX_VERTEX )
			{
				log_cb(RETRO_LOG_DEBUG, LOGPRE  "vertex overflow: %d\n", vertexCount );
				return;
			}
			for( count=0; count<vertexCount; count++ )
			{
				double x = (INT16)(pPointData[subAddr++]&0xffff);
				double y = (INT16)(pPointData[subAddr++]&0xffff);
				double z = (INT16)(pPointData[subAddr++]&0xffff);
				struct VerTex *pVertex = &vertex[count];
				pVertex->x = M[0][0]*x + M[1][0]*y + M[2][0]*z + M[3][0];
				pVertex->y = M[0][1]*x + M[1][1]*y + M[2][1]*z + M[3][1];
				pVertex->z = M[0][2]*x + M[1][2]*y + M[2][2]*z + M[3][2];
			}
			surfaceCount = pPointData[subAddr++]&0xff;
			if( surfaceCount > MAX_SURFACE )
			{
				mbDspError = 1;
				log_cb(RETRO_LOG_DEBUG, LOGPRE  "surface overflow: %d\n", surfaceCount );
				return;
			}
			for( count=0; count<surfaceCount; count++ )
			{
				if( subAddr>=0x400000/4 )
				{
					return;
				}
				vi[0] = pPointData[subAddr++]&0xff;
				vi[1] = pPointData[subAddr++]&0xff;
				vi[2] = pPointData[subAddr++]&0xff;
				vi[3] = pPointData[subAddr++]&0xff;
				color = pPointData[subAddr++]&0x1ff;
				/*color = 0x8000 - 0x400 + color;*/
				/*color = 0x6000 - 0x400 + color;*/
				color = 0x4000 - 0x400 + color;
				DrawQuad( bitmap,vertex,vi,color );
			}
		}
	}
} /* BlitPolyObject */

static void
ApplyRotation( const INT16 *pSource, double M[4][4] )
{
	struct RotParam param;
	param.thx_sin = pSource[0]/(double)0x7fff;
	param.thx_cos = pSource[1]/(double)0x7fff;
	param.thy_sin = pSource[2]/(double)0x7fff;
	param.thy_cos = pSource[3]/(double)0x7fff;
	param.thz_sin = pSource[4]/(double)0x7fff;
	param.thz_cos = pSource[5]/(double)0x7fff;
	param.rolt = pSource[6];
	namcos3d_Rotate( M, &param );
} /* ApplyRotation */

static void
ApplyCameraTransformation( const INT16 *pCamera, double M[4][4] )
{
	ApplyRotation( &pCamera[0x40/2], M );
} /* ApplyCameraTransformation */

static int
DrawPolyObject0( struct mame_bitmap *bitmap, const INT16 *pDSPRAM, const INT16 *pCamera )
{
	INT16 code = 1 + pDSPRAM[1];
	/*INT16 window = pDSPRAM[2];*/
	double M[4][4];

	matrix3d_Identity( M );
	matrix3d_Translate( M,pDSPRAM[3],pDSPRAM[4],pDSPRAM[5] );
	ApplyCameraTransformation( pCamera, M );
	BlitPolyObject( bitmap, code, M );
	return 6;
} /* DrawPolyObject0 */

static int
DrawPolyObject1( struct mame_bitmap *bitmap, const INT16 *pDSPRAM, const INT16 *pCamera )
{
	INT16 code = 1 + pDSPRAM[1];
	/*INT16 window = pDSPRAM[2];*/
	double M[4][4];

	matrix3d_Identity( M );
	ApplyRotation( &pDSPRAM[6], M );
	matrix3d_Translate( M,pDSPRAM[3],pDSPRAM[4],pDSPRAM[5] );

	if( pCamera )
	{
		ApplyCameraTransformation( pCamera, M );
	}
	/* correct for rolt==4 */
	BlitPolyObject( bitmap, code, M );
	return 13;
} /* DrawPolyObject1 */

static void
DrawPolygons( struct mame_bitmap *bitmap )
{
	int i,size;
	const INT16 *pCamera;
	const INT16 *pDSPRAM;
	int bDebug;

	if( namcos21_dspram16[0x200/2]==0 ) return; /* hack */

	namcos3d_Start( bitmap ); /* wipe zbuffer */

	namcos21_dspram16[0x202/2] = 0; /* clear busy signal */

	if( namcos21_dspram16[0x206/2]&1 ) /* work page select */
	{
		pDSPRAM = (INT16 *)&namcos21_dspram16[0xc000/2];
	}
	else
	{
		pDSPRAM = (INT16 *)&namcos21_dspram16[0x8000/2];
	}

/*
	0000:	0002 0001 2000
	0006:	ffe7 7fff
	000a:	0298 7ff9
	000e:	0000 7fff
	0012:	1000 1000 0000 0000
	001a:	00f8		// WIDTH
	001c:	00f2		// HEIGHT
	001e:	0003
	0020:	0004 008e 0014 0000 0000 0000 0000 0000
	0030:	0000 0000 0000 0000 0000 0000 0000 0000
	0040:	073c 7fcb	// ROLX
	0044:	0045 7fff	// ROLY
	0048:	edb5 7eae	// ROLZ
	004c:	0002		// ROLT
*/
	pCamera = pDSPRAM;

	/* press "U" to dump camera attributes and formatted object list */
	bDebug = keyboard_pressed( KEYCODE_U );
	if( bDebug )
	{
		while( keyboard_pressed( KEYCODE_U ) ){}
		log_cb(RETRO_LOG_DEBUG, LOGPRE  "\nDSPRAM:\n" );
		for( i=0; i<0x30*1; i++ )
		{
			if( (i&0x7)==0 ) log_cb(RETRO_LOG_DEBUG, LOGPRE  "\n\t%04x: ",i*2 );
			log_cb(RETRO_LOG_DEBUG, LOGPRE  "%04x ", (UINT16)pDSPRAM[i] );
		}
		log_cb(RETRO_LOG_DEBUG, LOGPRE  "\n" );
	}

	pDSPRAM += 0x200/2;
	mbDspError = 0;
	for(;;)
	{
		switch( pDSPRAM[0] )
		{
		case 0x0000: /* starblade */
			/* code, win, tx,ty,tz
			 *	[use camera transform]
			 */
			size = DrawPolyObject0( bitmap, pDSPRAM, pCamera );
			break;

		case 0x0001: /* starblade */
			/* code, win, tx,ty,tz, rolx(2), roly(2), rolz(2), rolt
			 *	[use camera transform]
			 */
			size = DrawPolyObject1( bitmap, pDSPRAM, pCamera );
			break;

		case 0x0002: /* starblade */
			/* code, win, tx,ty,tz, rolx(2), roly(2), rolz(2), rolt
			 *	[local transform only]
			 */
			size = DrawPolyObject1( bitmap, pDSPRAM, NULL );
			break;

		case 0x0004: /* air combat */
			size = DrawPolyObject1( bitmap, pDSPRAM, pCamera );
			size += 3; /* unknown: 0x8000 0x8000 0x8000 */
			break;

		case 0x0005: /* air combat */
			size = DrawPolyObject0( bitmap, pDSPRAM, pCamera );
			break;

		case 0x0006: /* air combat */
			size = DrawPolyObject1( bitmap, pDSPRAM, pCamera );
			break;

		case 0x0007: /* air combat */
			size = DrawPolyObject1( bitmap, pDSPRAM, pCamera );
			/* 0x00af 0x0003
			 * 0x3518 0xe889 0xe39c
			 * 0x0000 0x7fff 0x70e0 0xc3a7 0x0000 0x7fff 0x0004
			 */
			break;

		case 0x100: /* special end-marker for CyberSled? */
		case (INT16)0xffff: /* end-of-list marker */
			if( bDebug )
			{
				log_cb(RETRO_LOG_DEBUG, LOGPRE  "\n\n" );
			}
			return;

		default:
			logerror( "***unknown obj type! %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x\n",
				(UINT16)pDSPRAM[0],(UINT16)pDSPRAM[0],(UINT16)pDSPRAM[0],(UINT16)pDSPRAM[0],
				(UINT16)pDSPRAM[0],(UINT16)pDSPRAM[0],(UINT16)pDSPRAM[0],(UINT16)pDSPRAM[0],
				(UINT16)pDSPRAM[0],(UINT16)pDSPRAM[0],(UINT16)pDSPRAM[0],(UINT16)pDSPRAM[0]);
			return;
		}
		if( mbDspError ) return;
		if( bDebug )
		{
			log_cb(RETRO_LOG_DEBUG, LOGPRE  "obj: ");
			for( i=0; i<size; i++ )
			{
				log_cb(RETRO_LOG_DEBUG, LOGPRE  "%04x ", (UINT16)pDSPRAM[i] );
			}
			log_cb(RETRO_LOG_DEBUG, LOGPRE  "\n" );
		}
		pDSPRAM += size;
	} /* next object */
} /* DrawPolygons */

static int objcode2tile( int code )
{ /* callback for sprite drawing code in namcoic.c */
	return code;
}

VIDEO_START( namcos21 )
{
	namcos3d_Init( kScreenWidth, kScreenHeight, NULL, NULL );

	namco_obj_init(
		0,		/* gfx bank */
		0xf,	/* reverse palette mapping */
		objcode2tile );

	return 0;
}

static void
update_palette( void )
{
	int i;
	INT16 data1,data2;
	int r,g,b;

	/*
	Palette:
		0x0000..0x1fff	sprite palettes (0x10 sets of 0x100 colors)

		0x2000..0x3fff	polygon palette bank0 (0x10 sets of 0x200 colors)
			(in starblade, some palette animation effects are performed here)
		0x4000..0x5fff	polygon palette bank1 (0x10 sets of 0x200 colors)
		0x6000..0x7fff	polygon palette bank2 (0x10 sets of 0x200 colors)

		The polygon-dedicated color sets within a bank typically increase in
		intensity from very dark to full intensity.

		Probably the selected palette is determined by polygon view angle.
	*/
	for( i=0; i<NAMCOS21_NUM_COLORS; i++ )
	{
		data1 = paletteram16[0x00000/2+i];
		data2 = paletteram16[0x10000/2+i];

		r = data1>>8;
		g = data1&0xff;
		b = data2&0xff;

		palette_set_color( i, r,g,b );
	}
} /* update_palette */

VIDEO_UPDATE( namcos21_default )
{
	int pri;

	update_palette();

	/* paint background */
	fillbitmap( bitmap, get_black_pen(), cliprect );

	/* draw low priority 2d sprites */
	for( pri=0; pri<3; pri++ ) namco_obj_draw( bitmap, cliprect, pri );

	DrawPolygons( bitmap );

	/* draw high priority 2d sprites */
	for( pri=3; pri<8; pri++ ) namco_obj_draw( bitmap, cliprect, pri );

#if 0
	/* debug some video attributes */
	{
		int i,data;

		for( i=0; i<4; i++ )
		{
			data = 0xf&(namcos21_polyattr0[i/4]>>(4*(3-(i&3))));
			drawgfx( bitmap, Machine->uifont, "0123456789abcdef"[data], 0,0,0,
				i*12,16*0,cliprect,TRANSPARENCY_NONE,0 );
		}
		for( i=0; i<4; i++ )
		{
			data = 0xf&(namcos21_polyattr1[i/4]>>(4*(3-(i&3))));
			drawgfx( bitmap, Machine->uifont, "0123456789abcdef"[data], 0,0,0,
				i*12,16*1,cliprect,TRANSPARENCY_NONE,0 );
		}
		for( i=0; i<4*8; i++ )
		{
			data = 0xf&(namcos21_objattr[i/4]>>(4*(3-(i&3))));
			drawgfx( bitmap, Machine->uifont, "0123456789abcdef"[data], 0,0,0,
				i*12,16*2,cliprect,TRANSPARENCY_NONE,0 );
		}
	}
#endif

	/* some DSP witchery follows; it's an attempt to simulate the DSP behavior that
	 * Starblade expects during setup.
	  */
	{
		const data16_t cmd1[] =
		{
			0x0000,0x0001,0x0002,0x000a,0xcca3,0x0000,0x0000,0x0000,
			0x0000,0x0002,0x0000,0x0000,0x0001,0x0000,0x0000,0x0000,
			0x0080,0x0004,0xffff,0xffff
		};
		if( memcmp( &namcos21_dspram16[0x100/2], cmd1, sizeof(cmd1) )==0 )
		{ /* the check above is done so we don't interfere with the DSPRAM test */
			namcos21_dspram16[0x112/2] = 0; /* status to fake working DSP */
			namcos21_dspram16[0x100/2] = 2; /* status to fake working DSP */
			namcos21_dspram16[0x102/2] = 2; /* status to fake working DSP */
			namcos21_dspram16[0x110/2] = 2; /* status to fake working DSP */
			namcos21_dspram16[0x10c/2] = 0xd5df; /* checksum computed by DSP */
			namcos21_dspram16[0x10a/2] = 0xed53; /* checksum computed by DSP */
		}
		else if( namcos21_dspram16[0x10e/2] == 0x0001 )
		{
			/* This signals that a large chunk of code/data has been written by the main CPU.
			 *
			 * Presumably the DSP processor(s) copy it to private RAM at this point.
			 * The main CPU waits for this flag to be cleared.
			 */
			namcos21_dspram16[0x10e/2] = 0; /* ack */
		}
	}
} /* namcos21_default */
