#include "namcos3d.h"
#include "matrix3d.h"
#include "vidhrdw/poly.h"
#include "includes/namcos22.h"
#include <math.h>

/*
Renderer:
	each pixel->(BN,TX,TY,BRI,FLAGS,CZ,PAL)
		BN:    texture bank
		TX,TY: texture coordinates
		BRI:   brightness for gouraud shading
		FLAGS: specifies whether affected by depth cueing
		CZ:    index for depth cueing

Shader:
	(TX,TY,BN)->COL
	(PAL,COL)->palette index
	(palette index,BRI)->RGB

Depth BIAS:
	notes: blend function is expressed by a table
	it may be possible to specify multiple backcolors
	RGB,CZ,BackColor->RGB'

Mixer:
	(RGB',gamma table)->RGB''


	Other techniques:
		bump mapping perturbation (BX,BY)
		normal vector on polygon surface for lighting
		(normal vector, perturbation) -> N
		(lighting,N) -> BRI


		BRI=IaKa+{II/(Z+K)}.times.(Kd cost .phi.+Ks cos.sup.n.psi.) (1)

		Ia: Intensity of ambient light;
		II: Intensity of incident light;
		Ka: Diffuse reflection coefficient of ambient light [0];
		Kd: Diffuse reflection coefficient [0];
		Ks: Specular reflection coefficient [0];
		(a: ambient)
		(d: diffuse)
		(s: specular)
		K: Constant (for correcting the brightness in a less distant object [F];
		Z: Z-axis coordinate for each dot [0 in certain cases];
		.phi.: Angle between a light source vector L and a normal vector N;
		=Angle between a reflective light vector R and a normal vector N;
		.psi.: Angle between a reflective light vector R and a visual vector E =[0, 0, 1]; and
		n: Constant (sharpness in high-light) [0]
		[F]: Constant for each scene (field).
		[O]: Constant for each object (or polygon).
*/

static int mbShade;

INT32 *namco_zbuffer;

static data16_t *mpTextureTileMap16;
static data8_t *mpTextureTileMapAttr;
static data8_t *mpTextureTileData;

static data8_t mXYAttrToPixel[16][16][16];

static void
InitXYAttrToPixel( void )
{
	unsigned attr,x,y,ix,iy,temp;
	for( attr=0; attr<16; attr++ )
	{
		for( y=0; y<16; y++ )
		{
			for( x=0; x<16; x++ )
			{
				ix = x; iy = y;
				if( attr&4 ) ix = 15-ix;
				if( attr&2 ) iy = 15-iy;
				if( attr&8 ){ temp = ix; ix = iy; iy = temp; }
				mXYAttrToPixel[attr][x][y] = (iy<<4)|ix;
			}
		}
	}
}

int
namcos3d_Init( int width, int height, void *pTilemapROM, void *pTextureROM )
{
	namco_zbuffer = auto_malloc( width*height*sizeof(*namco_zbuffer) );
	if( namco_zbuffer )
	{
		if( pTilemapROM && pTextureROM )
		{ /* following setup is Namco System 22 specific */
			int i;
			const data8_t *pSource = 0x200000 + (data8_t *)pTilemapROM;
			data8_t *pDest = auto_malloc(0x80000*2); /* TBA: recycle pTilemapROM */
			if( pDest )
			{
				InitXYAttrToPixel();
				mpTextureTileMapAttr = pDest;
				for( i=0; i<0x80000; i++ )
				{
					*pDest++ = (*pSource)>>4;
					*pDest++ = (*pSource)&0xf;
					pSource++;
				}
				mpTextureTileMap16 = pTilemapROM;
				#ifdef MSB_FIRST
				/* if not little endian, swap each word */
				{
					unsigned i;
					for( i=0; i<0x200000/2; i++ )
					{
						data16_t data = mpTextureTileMap16[i];
						mpTextureTileMap16[i] = (data>>8)|(data<<8);
					}
				}
				#endif
				mpTextureTileData = pTextureROM;
			} /* pDest */
		}
		return 0;
	}
	return -1;
}

void
namcos3d_Rotate( double M[4][4], const struct RotParam *pParam )
{
	switch( pParam->rolt )
	{
	case 0:
		matrix3d_RotX( M, pParam->thx_sin, pParam->thx_cos );
		matrix3d_RotY( M, pParam->thy_sin, pParam->thy_cos );
		matrix3d_RotZ( M, pParam->thz_sin, pParam->thz_cos );
		break;
	case 1:
		matrix3d_RotX( M, pParam->thx_sin, pParam->thx_cos );
		matrix3d_RotZ( M, pParam->thz_sin, pParam->thz_cos );
		matrix3d_RotY( M, pParam->thy_sin, pParam->thy_cos );
		break;
	case 2:
		matrix3d_RotY( M, pParam->thy_sin, pParam->thy_cos );
		matrix3d_RotX( M, pParam->thx_sin, pParam->thx_cos );
		matrix3d_RotZ( M, pParam->thz_sin, pParam->thz_cos );
		break;
	case 3:
		matrix3d_RotY( M, pParam->thy_sin, pParam->thy_cos );
		matrix3d_RotZ( M, pParam->thz_sin, pParam->thz_cos );
		matrix3d_RotX( M, pParam->thx_sin, pParam->thx_cos );
		break;
	case 4:
		matrix3d_RotZ( M, pParam->thz_sin, pParam->thz_cos );
		matrix3d_RotX( M, pParam->thx_sin, pParam->thx_cos );
		matrix3d_RotY( M, pParam->thy_sin, pParam->thy_cos );
		break;
	case 5:
		matrix3d_RotZ( M, pParam->thz_sin, pParam->thz_cos );
		matrix3d_RotY( M, pParam->thy_sin, pParam->thy_cos );
		matrix3d_RotX( M, pParam->thx_sin, pParam->thx_cos );
		break;
	default:
		log_cb(RETRO_LOG_DEBUG, LOGPRE  "unknown rolt:%08x\n",pParam->rolt );
		break;
	}
} /* namcos3d_Rotate */

void
namcos3d_Start( struct mame_bitmap *pBitmap )
{
	memset( namco_zbuffer, 0x7f, pBitmap->width*pBitmap->height*sizeof(*namco_zbuffer) );
}

/*********************************************************************************************/

typedef struct
{
	double x,y;
	double u,v,i,z;
} vertex;

typedef struct
{
	double x;
	double u,v,i,z;
} edge;

#define SWAP(A,B) { const void *temp = A; A = B; B = temp; }

static unsigned mColor;
static INT32 mZSort;

static unsigned texel( unsigned x, unsigned y )
{
	unsigned offs = ((y&0xfff0)<<4)|((x&0xff0)>>4);
	return mpTextureTileData[(mpTextureTileMap16[offs]<<8)|mXYAttrToPixel[mpTextureTileMapAttr[offs]][x&0xf][y&0xf]];
} /* texel */

static void
renderscanline( const edge *e1, const edge *e2, int sy, const struct rectangle *clip )
{
	if( e1->x > e2->x )
	{
		SWAP(e1,e2);
	}

	{
		struct mame_bitmap *pBitmap = Machine->scrbitmap;
		UINT32 *pDest = (UINT32 *)pBitmap->line[sy];
		INT32 *pZBuf = namco_zbuffer + pBitmap->width*sy;

		int x0 = (int)e1->x;
		int x1 = (int)e2->x;
		int w = x1-x0;
		if( w )
		{
			double u = e1->u; /* u/z */
			double v = e1->v; /* v/z */
			double i = e1->i; /* i/z */
			double z = e1->z; /* 1/z */
			double du = (e2->u - e1->u)/w;
			double dv = (e2->v - e1->v)/w;
			double dz = (e2->z - e1->z)/w;
			double di = (e2->i - e1->i)/w;
			int x, crop;

			crop = clip->min_x - x0;
			if( crop>0 )
			{
				u += crop*du;
				v += crop*dv;
				i += crop*di;
				z += crop*dz;
				x0 = clip->min_x;
			}
			if( x1>clip->max_x )
			{
				x1 = clip->max_x;
			}

			for( x=x0; x<x1; x++ )
			{
				if( mZSort<pZBuf[x] )
				{
					UINT32 color = Machine->pens[texel(u/z,v/z)|mColor];
					int r = color>>16;
					int g = (color>>8)&0xff;
					int b = color&0xff;
					if( mbShade )
					{
						int shade = i/z;
						r+=shade; if( r<0 ) r = 0; else if( r>0xff ) r = 0xff;
						g+=shade; if( g<0 ) g = 0; else if( g>0xff ) g = 0xff;
						b+=shade; if( b<0 ) b = 0; else if( b>0xff ) b = 0xff;
					}
					pDest[x] = (r<<16)|(g<<8)|b;
					pZBuf[x] = mZSort;
				}
				u += du;
				v += dv;
				i += di;
				z += dz;
			}
		}
	}
}

/**
 * rendertri is a (temporary?) replacement for the scanline conversion that used to be done in poly.c
 * rendertri uses floating point arithmetic
 */
static void
rendertri( const vertex *v0, const vertex *v1, const vertex *v2, const struct rectangle *clip )
{
	int dy,ystart,yend,crop;

	/* first, sort so that v0->y <= v1->y <= v2->y */
	for(;;)
	{
		if( v0->y > v1->y )
		{
			SWAP(v0,v1);
		}
		else if( v1->y > v2->y )
		{
			SWAP(v1,v2);
		}
		else
		{
			break;
		}
	}

	ystart = v0->y;
	yend   = v2->y;
	dy = yend-ystart;
	if( dy )
	{
		int y;
		edge e1; /* short edge (top and bottom) */
		edge e2; /* long (common) edge */

		double dx2dy = (v2->x - v0->x)/dy;
		double du2dy = (v2->u - v0->u)/dy;
		double dv2dy = (v2->v - v0->v)/dy;
		double di2dy = (v2->i - v0->i)/dy;
		double dz2dy = (v2->z - v0->z)/dy;

		double dx1dy;
		double du1dy;
		double dv1dy;
		double di1dy;
		double dz1dy;

		e2.x = v0->x;
		e2.u = v0->u;
		e2.v = v0->v;
		e2.i = v0->i;
		e2.z = v0->z;
		crop = clip->min_y - ystart;
		if( crop>0 )
		{
			e2.x += dx2dy*crop;
			e2.u += du2dy*crop;
			e2.v += dv2dy*crop;
			e2.i += di2dy*crop;
			e2.z += dz2dy*crop;
		}

		ystart = v0->y;
		yend = v1->y;
		dy = yend-ystart;
		if( dy )
		{
			e1.x = v0->x;
			e1.u = v0->u;
			e1.v = v0->v;
			e1.i = v0->i;
			e1.z = v0->z;

			dx1dy = (v1->x - v0->x)/dy;
			du1dy = (v1->u - v0->u)/dy;
			dv1dy = (v1->v - v0->v)/dy;
			di1dy = (v1->i - v0->i)/dy;
			dz1dy = (v1->z - v0->z)/dy;

			crop = clip->min_y - ystart;
			if( crop>0 )
			{
				e1.x += dx1dy*crop;
				e1.u += du1dy*crop;
				e1.v += dv1dy*crop;
				e1.i += di1dy*crop;
				e1.z += dz1dy*crop;
				ystart = clip->min_y;
			}
			if( yend>clip->max_y ) yend = clip->max_y;

			for( y=ystart; y<yend; y++ )
			{
				renderscanline( &e1,&e2,y, clip );

				e2.x += dx2dy;
				e2.u += du2dy;
				e2.v += dv2dy;
				e2.i += di2dy;
				e2.z += dz2dy;

				e1.x += dx1dy;
				e1.u += du1dy;
				e1.v += dv1dy;
				e1.i += di1dy;
				e1.z += dz1dy;
			}
		}

		ystart = v1->y;
		yend = v2->y;
		dy = yend-ystart;
		if( dy )
		{
			e1.x = v1->x;
			e1.u = v1->u;
			e1.v = v1->v;
			e1.i = v1->i;
			e1.z = v1->z;

			dx1dy = (v2->x - v1->x)/dy;
			du1dy = (v2->u - v1->u)/dy;
			dv1dy = (v2->v - v1->v)/dy;
			di1dy = (v2->i - v1->i)/dy;
			dz1dy = (v2->z - v1->z)/dy;

			crop = clip->min_y - ystart;
			if( crop>0 )
			{
				e1.x += dx1dy*crop;
				e1.u += du1dy*crop;
				e1.v += dv1dy*crop;
				e1.i += di1dy*crop;
				e1.z += dz1dy*crop;
				ystart = clip->min_y;
			}
			if( yend>clip->max_y ) yend = clip->max_y;

			for( y=ystart; y<yend; y++ )
			{
				renderscanline( &e1,&e2,y, clip );

				e2.x += dx2dy;
				e2.u += du2dy;
				e2.v += dv2dy;
				e2.i += di2dy;
				e2.z += dz2dy;

				e1.x += dx1dy;
				e1.u += du1dy;
				e1.v += dv1dy;
				e1.i += di1dy;
				e1.z += dz1dy;
			}
		}
	}
} /* rendertri */

static void
ProjectPoint( const struct VerTex *v, vertex *pv, const namcos22_camera *camera )
{
	pv->x = camera->cx + v->x*camera->zoom/v->z;
	pv->y = camera->cy - v->y*camera->zoom/v->z;
	pv->u = (v->u+0.5)/v->z;
	pv->v = (v->v+0.5)/v->z;
	pv->i = (v->i+0.5 - 0x40)/v->z;
	pv->z = 1/v->z;
}

static void
BlitTriHelper(
		struct mame_bitmap *pBitmap,
		const struct VerTex *v0,
		const struct VerTex *v1,
		const struct VerTex *v2,
		unsigned color,
		const namcos22_camera *camera )
{
	vertex a,b,c;
	ProjectPoint( v0,&a,camera );
	ProjectPoint( v1,&b,camera );
	ProjectPoint( v2,&c,camera );
	mColor = color;
	rendertri( &a, &b, &c, &camera->clip );
}


#define MIN_Z (100) /* for near-plane clipping; this constant was arbitrarily chosen */
static double
interp( double x0, double namcos3d_y0, double x1, double namcos3d_y1 )
{
	double m = (namcos3d_y1-namcos3d_y0)/(x1-x0);
	double b = namcos3d_y0 - m*x0;
	return m*MIN_Z+b;
}

static int
VertexEqual( const struct VerTex *a, const struct VerTex *b )
{
	return a->x == b->x && a->y == b->y && a->z == b->z;
}

/**
 * BlitTri is used by Namco System22 to draw texture-mapped, perspective-correct triangles.
 */
void
namcos22_BlitTri(
	struct mame_bitmap *pBitmap,
	const struct VerTex v[3],
	unsigned color, INT32 zsort, INT32 flags,
	const namcos22_camera *camera )
{
	struct VerTex vc[3];
	int i,j;
	int iBad = 0, iGood = 0;
	int bad_count = 0;

	/* don't bother rendering a degenerate triangle */
	if( VertexEqual(&v[0],&v[1]) ) return;
	if( VertexEqual(&v[0],&v[2]) ) return;
	if( VertexEqual(&v[1],&v[2]) ) return;

	if( flags&0x0020 ) /* one-sided */
	{
		if( (v[2].x*((v[0].z*v[1].y)-(v[0].y*v[1].z)))+
			(v[2].y*((v[0].x*v[1].z)-(v[0].z*v[1].x)))+
			(v[2].z*((v[0].y*v[1].x)-(v[0].x*v[1].y))) >= 0 )
		{
			return; /* backface cull */
		}
	}

#if 0
	{ /* lighting (preliminary) */
		double a1 = v[1].x - v[0].x;
		double a2 = v[1].y - v[0].y;
		double a3 = v[1].z - v[0].z;

		double b1 = v[2].x - v[1].x;
		double b2 = v[2].y - v[1].y;
		double b3 = v[2].z - v[1].z;

		/* compute cross product of d1,d2 */
		double ux = a2*b3 - a3*b2;
		double uy = a3*b1 - a1*b3;
		double uz = a1*b2 - a2*b1;

		/* normalize */
		double dist = sqrt(ux*ux+uy*uy+uz*uz);
		ux /= dist;
		uy /= dist;
		uz /= dist;

		{
			double dotproduct = ux*camera->x + uy*camera->y + uz*camera->z;
			if( dotproduct<0 ) dotproduct = -dotproduct;
			mLight = dotproduct*camera->power + camera->ambient;
			if( mLight<0 ) mLight = 0;
			if( mLight>1.0 ) mLight = 1.0;
		}
	}
#endif

	mbShade = !keyboard_pressed(KEYCODE_G);

	for( i=0; i<3; i++ )
	{
		if( v[i].z<MIN_Z )
		{
			bad_count++;
			iBad = i;
		}
		else
		{
			iGood = i;
		}
	}

	mZSort = zsort;

	switch( bad_count )
	{
	case 0:
		BlitTriHelper( pBitmap, &v[0],&v[1],&v[2], color, camera );
		break;

	case 1:
		vc[0] = v[0];vc[1] = v[1];vc[2] = v[2];

		i = (iBad+1)%3;
		vc[iBad].x = interp( v[i].z,v[i].x, v[iBad].z,v[iBad].x  );
		vc[iBad].y = interp( v[i].z,v[i].y, v[iBad].z,v[iBad].y  );
		vc[iBad].u = interp( v[i].z,v[i].u, v[iBad].z,v[iBad].u );
		vc[iBad].v = interp( v[i].z,v[i].v, v[iBad].z,v[iBad].v );
		vc[iBad].i = interp( v[i].z,v[i].i, v[iBad].z,v[iBad].i );
		vc[iBad].z = MIN_Z;
		BlitTriHelper( pBitmap, &vc[0],&vc[1],&vc[2], color, camera );

		j = (iBad+2)%3;
		vc[i].x = interp(v[j].z,v[j].x, v[iBad].z,v[iBad].x  );
		vc[i].y = interp(v[j].z,v[j].y, v[iBad].z,v[iBad].y  );
		vc[i].u = interp(v[j].z,v[j].u, v[iBad].z,v[iBad].u );
		vc[i].v = interp(v[j].z,v[j].v, v[iBad].z,v[iBad].v );
		vc[i].i = interp(v[j].z,v[j].i, v[iBad].z,v[iBad].i );
		vc[i].z = MIN_Z;
		BlitTriHelper( pBitmap, &vc[0],&vc[1],&vc[2], color, camera );
		break;

	case 2:
		vc[0] = v[0];vc[1] = v[1];vc[2] = v[2];

		i = (iGood+1)%3;
		vc[i].x = interp(v[iGood].z,v[iGood].x, v[i].z,v[i].x  );
		vc[i].y = interp(v[iGood].z,v[iGood].y, v[i].z,v[i].y  );
		vc[i].u = interp(v[iGood].z,v[iGood].u, v[i].z,v[i].u );
		vc[i].v = interp(v[iGood].z,v[iGood].v, v[i].z,v[i].v );
		vc[i].i = interp(v[iGood].z,v[iGood].i, v[i].z,v[i].i );
		vc[i].z = MIN_Z;

		i = (iGood+2)%3;
		vc[i].x = interp(v[iGood].z,v[iGood].x, v[i].z,v[i].x  );
		vc[i].y = interp(v[iGood].z,v[iGood].y, v[i].z,v[i].y  );
		vc[i].u = interp(v[iGood].z,v[iGood].u, v[i].z,v[i].u );
		vc[i].v = interp(v[iGood].z,v[iGood].v, v[i].z,v[i].v );
		vc[i].i = interp(v[iGood].z,v[iGood].i, v[i].z,v[i].i );
		vc[i].z = MIN_Z;

		BlitTriHelper( pBitmap, &vc[0],&vc[1],&vc[2], color, camera );
		break;

	case 3:
		/* wholly clipped */
		break;
	}
} /* BlitTri */

/**
 * BlitFlatSpan is a helper function called by BlitTriFlat while drawing triangles
 */
static void
BlitFlatSpan(
	UINT16 *pDest, INT32 *pZBuf, const struct poly_scanline *scan, const INT64 *deltas, unsigned color)
{
	INT64 z = scan->p[0];
	INT64 dz = deltas[0];
	INT32 x;

	for (x = scan->sx; x <= scan->ex; x++)
	{
		INT32 sz = (z >> 16);
		if( sz<pZBuf[x] )
		{
			pZBuf[x] = sz;
			pDest[x] = color;
		}
		z += dz;
	}
} /* BlitFlatSpan */

/**
 * BlitTriFlat is used by Namco System21 to draw flat-shaded triangles.
 */
void
BlitTriFlat( struct mame_bitmap *pBitmap, const struct VerTex v[3], unsigned color )
{
	const struct poly_scanline_data *scans;
	const struct poly_scanline *curscan;
	struct poly_vertex pv[3];
	struct rectangle cliprect;
	INT32 y;
	int i;

	cliprect.min_x = cliprect.min_y = 0;
	cliprect.max_x = pBitmap->width - 1;
	cliprect.max_y = pBitmap->height - 1;

	if( (v[2].x*((v[0].z*v[1].y)-(v[0].y*v[1].z)))+
		(v[2].y*((v[0].x*v[1].z)-(v[0].z*v[1].x)))+
		(v[2].z*((v[0].y*v[1].x)-(v[0].x*v[1].y))) >= 0 )
	{
		return; /* backface cull */
	}

	for( i=0; i<3; i++ )
	{
		if( v[i].z <=0 ) return; /* TBA: near plane clipping */
		/* HACK! */
		pv[i].x = pBitmap->width/2 + v[i].x*0x248/v[i].z;
		pv[i].y = pBitmap->height/2 - v[i].y*0x2a0/v[i].z;
		pv[i].p[0] = v[i].z;
	}
	scans = setup_triangle_1(&pv[0], &pv[1], &pv[2], &cliprect);
	if (!scans)
		return;
	curscan = scans->scanline;
	for (y = scans->sy; y <= scans->ey; y++, curscan++)
	{
		UINT16 *pDest = (UINT16 *)pBitmap->line[y];
		INT32 *pZBuf = namco_zbuffer + pBitmap->width*y;
		BlitFlatSpan(pDest, pZBuf, curscan, scans->dp, color);
	}
} /* BlitTri */
