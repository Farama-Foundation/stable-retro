/**
 * Do not modify this module directly.  It is generated code, written by blitgen.c
 *
 * The implementation is not yet optimal.
 * To do:
 * - use duff-unrolled loops
 * - include ystart,yend params, so that the blitter can be called once for each drawgfx()
 *   instead of once per line.  There's a lot of parameters being repeatedly pushed onto
 *   the stack with the current implementation.
 */
static int theColor;


#define SHADOW8(data) palette_shadow_table[data]

#define SHADOW16(data) palette_shadow_table[data]

/** AAT 032503: added limited 32-bit shadow and highlight support*/
static INLINE int SHADOW32(int c)
{
	#define RGB8TO5(x) (((x)>>3&0x001f)|((x)>>6&0x03e0)|((x)>>9&0x7c00))
	#define RGB5TO8(x) (((x)<<3&0x00f8)|((x)<<6&0xf800)|((x)<<9&0xf80000))
	/* DEPENDENCY CHAIN!!!*/
	c = RGB8TO5(c);
	c = palette_shadow_table[c];
	c = RGB5TO8(c);
	return(c);
	#undef RGB8TO5
	#undef RGB5TO8
}


static void blit_none_from4bpp_to8bpp_nobuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if(1){
			data = pPal[data];
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blit_none_from4bpp_to8bpp_pribuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if(1){
			data = pPal[data];
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_none_from4bpp_to8bpp_zbuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if(1){
			data = pPal[data];
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blit_none_from4bpp_to16bpp_nobuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if(1){
			data = pPal[data];
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blit_none_from4bpp_to16bpp_pribuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if(1){
			data = pPal[data];
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_none_from4bpp_to16bpp_zbuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if(1){
			data = pPal[data];
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blit_none_from4bpp_to32bpp_nobuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if(1){
			data = pPal[data];
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blit_none_from4bpp_to32bpp_pribuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if(1){
			data = pPal[data];
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_none_from4bpp_to32bpp_zbuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if(1){
			data = pPal[data];
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blit_none_from8bpp_to8bpp_nobuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if(1){
			data = pPal[data];
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blit_none_from8bpp_to8bpp_pribuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if(1){
			data = pPal[data];
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_none_from8bpp_to8bpp_zbuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if(1){
			data = pPal[data];
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blit_none_from8bpp_to16bpp_nobuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if(1){
			data = pPal[data];
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blit_none_from8bpp_to16bpp_pribuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if(1){
			data = pPal[data];
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_none_from8bpp_to16bpp_zbuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if(1){
			data = pPal[data];
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blit_none_from8bpp_to32bpp_nobuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if(1){
			data = pPal[data];
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blit_none_from8bpp_to32bpp_pribuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if(1){
			data = pPal[data];
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_none_from8bpp_to32bpp_zbuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if(1){
			data = pPal[data];
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blit_none_raw_from4bpp_to8bpp_nobuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if(1){
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blit_none_raw_from4bpp_to8bpp_pribuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if(1){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_none_raw_from4bpp_to8bpp_zbuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if(1){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blit_none_raw_from4bpp_to16bpp_nobuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if(1){
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blit_none_raw_from4bpp_to16bpp_pribuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if(1){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_none_raw_from4bpp_to16bpp_zbuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if(1){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blit_none_raw_from4bpp_to32bpp_nobuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if(1){
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blit_none_raw_from4bpp_to32bpp_pribuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if(1){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_none_raw_from4bpp_to32bpp_zbuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if(1){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blit_none_raw_from8bpp_to8bpp_nobuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if(1){
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blit_none_raw_from8bpp_to8bpp_pribuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if(1){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_none_raw_from8bpp_to8bpp_zbuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if(1){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blit_none_raw_from8bpp_to16bpp_nobuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if(1){
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blit_none_raw_from8bpp_to16bpp_pribuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if(1){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_none_raw_from8bpp_to16bpp_zbuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if(1){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blit_none_raw_from8bpp_to32bpp_nobuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if(1){
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blit_none_raw_from8bpp_to32bpp_pribuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if(1){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_none_raw_from8bpp_to32bpp_zbuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if(1){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blit_pen_from4bpp_to8bpp_nobuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			data = pPal[data];
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blit_pen_from4bpp_to8bpp_pribuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			data = pPal[data];
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_pen_from4bpp_to8bpp_zbuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			data = pPal[data];
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blit_pen_from4bpp_to16bpp_nobuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			data = pPal[data];
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blit_pen_from4bpp_to16bpp_pribuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			data = pPal[data];
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_pen_from4bpp_to16bpp_zbuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			data = pPal[data];
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blit_pen_from4bpp_to32bpp_nobuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			data = pPal[data];
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blit_pen_from4bpp_to32bpp_pribuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			data = pPal[data];
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_pen_from4bpp_to32bpp_zbuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			data = pPal[data];
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blit_pen_from8bpp_to8bpp_nobuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			data = pPal[data];
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blit_pen_from8bpp_to8bpp_pribuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			data = pPal[data];
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_pen_from8bpp_to8bpp_zbuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			data = pPal[data];
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blit_pen_from8bpp_to16bpp_nobuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			data = pPal[data];
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blit_pen_from8bpp_to16bpp_pribuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			data = pPal[data];
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_pen_from8bpp_to16bpp_zbuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			data = pPal[data];
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blit_pen_from8bpp_to32bpp_nobuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			data = pPal[data];
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blit_pen_from8bpp_to32bpp_pribuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			data = pPal[data];
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_pen_from8bpp_to32bpp_zbuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			data = pPal[data];
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blit_pen_raw_from4bpp_to8bpp_nobuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blit_pen_raw_from4bpp_to8bpp_pribuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_pen_raw_from4bpp_to8bpp_zbuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blit_pen_raw_from4bpp_to16bpp_nobuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blit_pen_raw_from4bpp_to16bpp_pribuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_pen_raw_from4bpp_to16bpp_zbuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blit_pen_raw_from4bpp_to32bpp_nobuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blit_pen_raw_from4bpp_to32bpp_pribuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_pen_raw_from4bpp_to32bpp_zbuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blit_pen_raw_from8bpp_to8bpp_nobuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blit_pen_raw_from8bpp_to8bpp_pribuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_pen_raw_from8bpp_to8bpp_zbuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blit_pen_raw_from8bpp_to16bpp_nobuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blit_pen_raw_from8bpp_to16bpp_pribuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_pen_raw_from8bpp_to16bpp_zbuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blit_pen_raw_from8bpp_to32bpp_nobuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blit_pen_raw_from8bpp_to32bpp_pribuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_pen_raw_from8bpp_to32bpp_zbuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blit_pens_from4bpp_to8bpp_nobuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( ((1<<data)&transp)==0 ){
			data = pPal[data];
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blit_pens_from4bpp_to8bpp_pribuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( ((1<<data)&transp)==0 ){
			data = pPal[data];
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_pens_from4bpp_to8bpp_zbuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( ((1<<data)&transp)==0 ){
			data = pPal[data];
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blit_pens_from4bpp_to16bpp_nobuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( ((1<<data)&transp)==0 ){
			data = pPal[data];
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blit_pens_from4bpp_to16bpp_pribuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( ((1<<data)&transp)==0 ){
			data = pPal[data];
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_pens_from4bpp_to16bpp_zbuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( ((1<<data)&transp)==0 ){
			data = pPal[data];
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blit_pens_from4bpp_to32bpp_nobuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( ((1<<data)&transp)==0 ){
			data = pPal[data];
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blit_pens_from4bpp_to32bpp_pribuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( ((1<<data)&transp)==0 ){
			data = pPal[data];
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_pens_from4bpp_to32bpp_zbuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( ((1<<data)&transp)==0 ){
			data = pPal[data];
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blit_pens_from8bpp_to8bpp_nobuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( ((1<<data)&transp)==0 ){
			data = pPal[data];
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blit_pens_from8bpp_to8bpp_pribuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( ((1<<data)&transp)==0 ){
			data = pPal[data];
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_pens_from8bpp_to8bpp_zbuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( ((1<<data)&transp)==0 ){
			data = pPal[data];
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blit_pens_from8bpp_to16bpp_nobuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( ((1<<data)&transp)==0 ){
			data = pPal[data];
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blit_pens_from8bpp_to16bpp_pribuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( ((1<<data)&transp)==0 ){
			data = pPal[data];
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_pens_from8bpp_to16bpp_zbuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( ((1<<data)&transp)==0 ){
			data = pPal[data];
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blit_pens_from8bpp_to32bpp_nobuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( ((1<<data)&transp)==0 ){
			data = pPal[data];
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blit_pens_from8bpp_to32bpp_pribuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( ((1<<data)&transp)==0 ){
			data = pPal[data];
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_pens_from8bpp_to32bpp_zbuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( ((1<<data)&transp)==0 ){
			data = pPal[data];
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blit_pens_raw_from4bpp_to8bpp_nobuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( ((1<<data)&transp)==0 ){
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blit_pens_raw_from4bpp_to8bpp_pribuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( ((1<<data)&transp)==0 ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_pens_raw_from4bpp_to8bpp_zbuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( ((1<<data)&transp)==0 ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blit_pens_raw_from4bpp_to16bpp_nobuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( ((1<<data)&transp)==0 ){
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blit_pens_raw_from4bpp_to16bpp_pribuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( ((1<<data)&transp)==0 ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_pens_raw_from4bpp_to16bpp_zbuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( ((1<<data)&transp)==0 ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blit_pens_raw_from4bpp_to32bpp_nobuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( ((1<<data)&transp)==0 ){
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blit_pens_raw_from4bpp_to32bpp_pribuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( ((1<<data)&transp)==0 ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_pens_raw_from4bpp_to32bpp_zbuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( ((1<<data)&transp)==0 ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blit_pens_raw_from8bpp_to8bpp_nobuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( ((1<<data)&transp)==0 ){
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blit_pens_raw_from8bpp_to8bpp_pribuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( ((1<<data)&transp)==0 ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_pens_raw_from8bpp_to8bpp_zbuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( ((1<<data)&transp)==0 ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blit_pens_raw_from8bpp_to16bpp_nobuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( ((1<<data)&transp)==0 ){
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blit_pens_raw_from8bpp_to16bpp_pribuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( ((1<<data)&transp)==0 ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_pens_raw_from8bpp_to16bpp_zbuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( ((1<<data)&transp)==0 ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blit_pens_raw_from8bpp_to32bpp_nobuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( ((1<<data)&transp)==0 ){
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blit_pens_raw_from8bpp_to32bpp_pribuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( ((1<<data)&transp)==0 ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_pens_raw_from8bpp_to32bpp_zbuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( ((1<<data)&transp)==0 ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blit_color_from4bpp_to8bpp_nobuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	transp = Machine->pens[transp];
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			data = pPal[data];
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blit_color_from4bpp_to8bpp_pribuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	transp = Machine->pens[transp];
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			data = pPal[data];
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_color_from4bpp_to8bpp_zbuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	transp = Machine->pens[transp];
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			data = pPal[data];
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blit_color_from4bpp_to16bpp_nobuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	transp = Machine->pens[transp];
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			data = pPal[data];
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blit_color_from4bpp_to16bpp_pribuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	transp = Machine->pens[transp];
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			data = pPal[data];
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_color_from4bpp_to16bpp_zbuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	transp = Machine->pens[transp];
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			data = pPal[data];
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blit_color_from4bpp_to32bpp_nobuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	transp = Machine->pens[transp];
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			data = pPal[data];
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blit_color_from4bpp_to32bpp_pribuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	transp = Machine->pens[transp];
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			data = pPal[data];
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_color_from4bpp_to32bpp_zbuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	transp = Machine->pens[transp];
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			data = pPal[data];
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blit_color_from8bpp_to8bpp_nobuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	transp = Machine->pens[transp];
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			data = pPal[data];
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blit_color_from8bpp_to8bpp_pribuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	transp = Machine->pens[transp];
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			data = pPal[data];
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_color_from8bpp_to8bpp_zbuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	transp = Machine->pens[transp];
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			data = pPal[data];
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blit_color_from8bpp_to16bpp_nobuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	transp = Machine->pens[transp];
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			data = pPal[data];
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blit_color_from8bpp_to16bpp_pribuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	transp = Machine->pens[transp];
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			data = pPal[data];
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_color_from8bpp_to16bpp_zbuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	transp = Machine->pens[transp];
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			data = pPal[data];
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blit_color_from8bpp_to32bpp_nobuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	transp = Machine->pens[transp];
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			data = pPal[data];
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blit_color_from8bpp_to32bpp_pribuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	transp = Machine->pens[transp];
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			data = pPal[data];
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_color_from8bpp_to32bpp_zbuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	transp = Machine->pens[transp];
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			data = pPal[data];
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blit_pen_table_from4bpp_to8bpp_nobuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = pPal[data];
					pDst[x] = data;
				break;
			case DRAWMODE_SHADOW:
					pDst[x] = SHADOW8(pDst[x]);
				break;
			}
		}
		x_index += dx;
	}
}

static void blit_pen_table_from4bpp_to8bpp_pribuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = pPal[data];
				if (((1 << (((UINT8 *)pBuf)[x] & 0x1f)) & pri_code) == 0)
				{
				if (((UINT8 *)pBuf)[x] & 0x80)
					pDst[x] = SHADOW8(data);
				else
					pDst[x] = data;
				}
				((UINT8 *)pBuf)[x] = (((UINT8 *)pBuf)[x] & 0x7f) | 31;
				break;
			case DRAWMODE_SHADOW:
				if (((1 << ((UINT8 *)pBuf)[x]) & pri_code) == 0)
					pDst[x] = SHADOW8(pDst[x]);
				((UINT8 *)pBuf)[x] |= pdrawgfx_shadow_lowpri ? 0 : 0x80;
				break;
			}
		}
		x_index += dx;
	}
}

static void blit_pen_table_from4bpp_to8bpp_zbuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = pPal[data];
					pDst[x] = data;
				break;
			case DRAWMODE_SHADOW:
					pDst[x] = SHADOW8(pDst[x]);
				break;
			}
		}
		x_index += dx;
	}
}

static void blit_pen_table_from4bpp_to16bpp_nobuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = pPal[data];
					pDst[x] = data;
				break;
			case DRAWMODE_SHADOW:
					pDst[x] = SHADOW16(pDst[x]);
				break;
			}
		}
		x_index += dx;
	}
}

static void blit_pen_table_from4bpp_to16bpp_pribuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = pPal[data];
				if (((1 << (((UINT8 *)pBuf)[x] & 0x1f)) & pri_code) == 0)
				{
				if (((UINT8 *)pBuf)[x] & 0x80)
					pDst[x] = SHADOW16(data);
				else
					pDst[x] = data;
				}
				((UINT8 *)pBuf)[x] = (((UINT8 *)pBuf)[x] & 0x7f) | 31;
				break;
			case DRAWMODE_SHADOW:
				if (((1 << ((UINT8 *)pBuf)[x]) & pri_code) == 0)
					pDst[x] = SHADOW16(pDst[x]);
				((UINT8 *)pBuf)[x] |= pdrawgfx_shadow_lowpri ? 0 : 0x80;
				break;
			}
		}
		x_index += dx;
	}
}

static void blit_pen_table_from4bpp_to16bpp_zbuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = pPal[data];
					pDst[x] = data;
				break;
			case DRAWMODE_SHADOW:
					pDst[x] = SHADOW16(pDst[x]);
				break;
			}
		}
		x_index += dx;
	}
}

static void blit_pen_table_from4bpp_to32bpp_nobuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = pPal[data];
					pDst[x] = data;
				break;
			case DRAWMODE_SHADOW:
					pDst[x] = SHADOW32(pDst[x]);
				break;
			}
		}
		x_index += dx;
	}
}

static void blit_pen_table_from4bpp_to32bpp_pribuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = pPal[data];
				if (((1 << (((UINT8 *)pBuf)[x] & 0x1f)) & pri_code) == 0)
				{
				if (((UINT8 *)pBuf)[x] & 0x80)
					pDst[x] = SHADOW32(data);
				else
					pDst[x] = data;
				}
				((UINT8 *)pBuf)[x] = (((UINT8 *)pBuf)[x] & 0x7f) | 31;
				break;
			case DRAWMODE_SHADOW:
				if (((1 << ((UINT8 *)pBuf)[x]) & pri_code) == 0)
					pDst[x] = SHADOW32(pDst[x]);
				((UINT8 *)pBuf)[x] |= pdrawgfx_shadow_lowpri ? 0 : 0x80;
				break;
			}
		}
		x_index += dx;
	}
}

static void blit_pen_table_from4bpp_to32bpp_zbuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = pPal[data];
					pDst[x] = data;
				break;
			case DRAWMODE_SHADOW:
					pDst[x] = SHADOW32(pDst[x]);
				break;
			}
		}
		x_index += dx;
	}
}

static void blit_pen_table_from8bpp_to8bpp_nobuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = pPal[data];
					pDst[x] = data;
				break;
			case DRAWMODE_SHADOW:
					pDst[x] = SHADOW8(pDst[x]);
				break;
			}
		}
		x_index += dx;
	}
}

static void blit_pen_table_from8bpp_to8bpp_pribuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = pPal[data];
				if (((1 << (((UINT8 *)pBuf)[x] & 0x1f)) & pri_code) == 0)
				{
				if (((UINT8 *)pBuf)[x] & 0x80)
					pDst[x] = SHADOW8(data);
				else
					pDst[x] = data;
				}
				((UINT8 *)pBuf)[x] = (((UINT8 *)pBuf)[x] & 0x7f) | 31;
				break;
			case DRAWMODE_SHADOW:
				if (((1 << ((UINT8 *)pBuf)[x]) & pri_code) == 0)
					pDst[x] = SHADOW8(pDst[x]);
				((UINT8 *)pBuf)[x] |= pdrawgfx_shadow_lowpri ? 0 : 0x80;
				break;
			}
		}
		x_index += dx;
	}
}

static void blit_pen_table_from8bpp_to8bpp_zbuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = pPal[data];
					pDst[x] = data;
				break;
			case DRAWMODE_SHADOW:
					pDst[x] = SHADOW8(pDst[x]);
				break;
			}
		}
		x_index += dx;
	}
}

static void blit_pen_table_from8bpp_to16bpp_nobuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = pPal[data];
					pDst[x] = data;
				break;
			case DRAWMODE_SHADOW:
					pDst[x] = SHADOW16(pDst[x]);
				break;
			}
		}
		x_index += dx;
	}
}

static void blit_pen_table_from8bpp_to16bpp_pribuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = pPal[data];
				if (((1 << (((UINT8 *)pBuf)[x] & 0x1f)) & pri_code) == 0)
				{
				if (((UINT8 *)pBuf)[x] & 0x80)
					pDst[x] = SHADOW16(data);
				else
					pDst[x] = data;
				}
				((UINT8 *)pBuf)[x] = (((UINT8 *)pBuf)[x] & 0x7f) | 31;
				break;
			case DRAWMODE_SHADOW:
				if (((1 << ((UINT8 *)pBuf)[x]) & pri_code) == 0)
					pDst[x] = SHADOW16(pDst[x]);
				((UINT8 *)pBuf)[x] |= pdrawgfx_shadow_lowpri ? 0 : 0x80;
				break;
			}
		}
		x_index += dx;
	}
}

static void blit_pen_table_from8bpp_to16bpp_zbuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = pPal[data];
					pDst[x] = data;
				break;
			case DRAWMODE_SHADOW:
					pDst[x] = SHADOW16(pDst[x]);
				break;
			}
		}
		x_index += dx;
	}
}

static void blit_pen_table_from8bpp_to32bpp_nobuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = pPal[data];
					pDst[x] = data;
				break;
			case DRAWMODE_SHADOW:
					pDst[x] = SHADOW32(pDst[x]);
				break;
			}
		}
		x_index += dx;
	}
}

static void blit_pen_table_from8bpp_to32bpp_pribuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = pPal[data];
				if (((1 << (((UINT8 *)pBuf)[x] & 0x1f)) & pri_code) == 0)
				{
				if (((UINT8 *)pBuf)[x] & 0x80)
					pDst[x] = SHADOW32(data);
				else
					pDst[x] = data;
				}
				((UINT8 *)pBuf)[x] = (((UINT8 *)pBuf)[x] & 0x7f) | 31;
				break;
			case DRAWMODE_SHADOW:
				if (((1 << ((UINT8 *)pBuf)[x]) & pri_code) == 0)
					pDst[x] = SHADOW32(pDst[x]);
				((UINT8 *)pBuf)[x] |= pdrawgfx_shadow_lowpri ? 0 : 0x80;
				break;
			}
		}
		x_index += dx;
	}
}

static void blit_pen_table_from8bpp_to32bpp_zbuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = pPal[data];
					pDst[x] = data;
				break;
			case DRAWMODE_SHADOW:
					pDst[x] = SHADOW32(pDst[x]);
				break;
			}
		}
		x_index += dx;
	}
}

static void blit_pen_table_raw_from4bpp_to8bpp_nobuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = theColor+data;
					pDst[x] = data;
				break;
			case DRAWMODE_SHADOW:
					pDst[x] = SHADOW8(pDst[x]);
				break;
			}
		}
		x_index += dx;
	}
}

static void blit_pen_table_raw_from4bpp_to8bpp_pribuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = theColor+data;
				if (((1 << (((UINT8 *)pBuf)[x] & 0x1f)) & pri_code) == 0)
				{
				if (((UINT8 *)pBuf)[x] & 0x80)
					pDst[x] = SHADOW8(data);
				else
					pDst[x] = data;
				}
				((UINT8 *)pBuf)[x] = (((UINT8 *)pBuf)[x] & 0x7f) | 31;
				break;
			case DRAWMODE_SHADOW:
				if (((1 << ((UINT8 *)pBuf)[x]) & pri_code) == 0)
					pDst[x] = SHADOW8(pDst[x]);
				((UINT8 *)pBuf)[x] |= pdrawgfx_shadow_lowpri ? 0 : 0x80;
				break;
			}
		}
		x_index += dx;
	}
}

static void blit_pen_table_raw_from4bpp_to8bpp_zbuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = theColor+data;
					pDst[x] = data;
				break;
			case DRAWMODE_SHADOW:
					pDst[x] = SHADOW8(pDst[x]);
				break;
			}
		}
		x_index += dx;
	}
}

static void blit_pen_table_raw_from4bpp_to16bpp_nobuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = theColor+data;
					pDst[x] = data;
				break;
			case DRAWMODE_SHADOW:
					pDst[x] = SHADOW16(pDst[x]);
				break;
			}
		}
		x_index += dx;
	}
}

static void blit_pen_table_raw_from4bpp_to16bpp_pribuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = theColor+data;
				if (((1 << (((UINT8 *)pBuf)[x] & 0x1f)) & pri_code) == 0)
				{
				if (((UINT8 *)pBuf)[x] & 0x80)
					pDst[x] = SHADOW16(data);
				else
					pDst[x] = data;
				}
				((UINT8 *)pBuf)[x] = (((UINT8 *)pBuf)[x] & 0x7f) | 31;
				break;
			case DRAWMODE_SHADOW:
				if (((1 << ((UINT8 *)pBuf)[x]) & pri_code) == 0)
					pDst[x] = SHADOW16(pDst[x]);
				((UINT8 *)pBuf)[x] |= pdrawgfx_shadow_lowpri ? 0 : 0x80;
				break;
			}
		}
		x_index += dx;
	}
}

static void blit_pen_table_raw_from4bpp_to16bpp_zbuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = theColor+data;
					pDst[x] = data;
				break;
			case DRAWMODE_SHADOW:
					pDst[x] = SHADOW16(pDst[x]);
				break;
			}
		}
		x_index += dx;
	}
}

static void blit_pen_table_raw_from4bpp_to32bpp_nobuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = theColor+data;
					pDst[x] = data;
				break;
			case DRAWMODE_SHADOW:
					pDst[x] = SHADOW32(pDst[x]);
				break;
			}
		}
		x_index += dx;
	}
}

static void blit_pen_table_raw_from4bpp_to32bpp_pribuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = theColor+data;
				if (((1 << (((UINT8 *)pBuf)[x] & 0x1f)) & pri_code) == 0)
				{
				if (((UINT8 *)pBuf)[x] & 0x80)
					pDst[x] = SHADOW32(data);
				else
					pDst[x] = data;
				}
				((UINT8 *)pBuf)[x] = (((UINT8 *)pBuf)[x] & 0x7f) | 31;
				break;
			case DRAWMODE_SHADOW:
				if (((1 << ((UINT8 *)pBuf)[x]) & pri_code) == 0)
					pDst[x] = SHADOW32(pDst[x]);
				((UINT8 *)pBuf)[x] |= pdrawgfx_shadow_lowpri ? 0 : 0x80;
				break;
			}
		}
		x_index += dx;
	}
}

static void blit_pen_table_raw_from4bpp_to32bpp_zbuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = theColor+data;
					pDst[x] = data;
				break;
			case DRAWMODE_SHADOW:
					pDst[x] = SHADOW32(pDst[x]);
				break;
			}
		}
		x_index += dx;
	}
}

static void blit_pen_table_raw_from8bpp_to8bpp_nobuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = theColor+data;
					pDst[x] = data;
				break;
			case DRAWMODE_SHADOW:
					pDst[x] = SHADOW8(pDst[x]);
				break;
			}
		}
		x_index += dx;
	}
}

static void blit_pen_table_raw_from8bpp_to8bpp_pribuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = theColor+data;
				if (((1 << (((UINT8 *)pBuf)[x] & 0x1f)) & pri_code) == 0)
				{
				if (((UINT8 *)pBuf)[x] & 0x80)
					pDst[x] = SHADOW8(data);
				else
					pDst[x] = data;
				}
				((UINT8 *)pBuf)[x] = (((UINT8 *)pBuf)[x] & 0x7f) | 31;
				break;
			case DRAWMODE_SHADOW:
				if (((1 << ((UINT8 *)pBuf)[x]) & pri_code) == 0)
					pDst[x] = SHADOW8(pDst[x]);
				((UINT8 *)pBuf)[x] |= pdrawgfx_shadow_lowpri ? 0 : 0x80;
				break;
			}
		}
		x_index += dx;
	}
}

static void blit_pen_table_raw_from8bpp_to8bpp_zbuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = theColor+data;
					pDst[x] = data;
				break;
			case DRAWMODE_SHADOW:
					pDst[x] = SHADOW8(pDst[x]);
				break;
			}
		}
		x_index += dx;
	}
}

static void blit_pen_table_raw_from8bpp_to16bpp_nobuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = theColor+data;
					pDst[x] = data;
				break;
			case DRAWMODE_SHADOW:
					pDst[x] = SHADOW16(pDst[x]);
				break;
			}
		}
		x_index += dx;
	}
}

static void blit_pen_table_raw_from8bpp_to16bpp_pribuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = theColor+data;
				if (((1 << (((UINT8 *)pBuf)[x] & 0x1f)) & pri_code) == 0)
				{
				if (((UINT8 *)pBuf)[x] & 0x80)
					pDst[x] = SHADOW16(data);
				else
					pDst[x] = data;
				}
				((UINT8 *)pBuf)[x] = (((UINT8 *)pBuf)[x] & 0x7f) | 31;
				break;
			case DRAWMODE_SHADOW:
				if (((1 << ((UINT8 *)pBuf)[x]) & pri_code) == 0)
					pDst[x] = SHADOW16(pDst[x]);
				((UINT8 *)pBuf)[x] |= pdrawgfx_shadow_lowpri ? 0 : 0x80;
				break;
			}
		}
		x_index += dx;
	}
}

static void blit_pen_table_raw_from8bpp_to16bpp_zbuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = theColor+data;
					pDst[x] = data;
				break;
			case DRAWMODE_SHADOW:
					pDst[x] = SHADOW16(pDst[x]);
				break;
			}
		}
		x_index += dx;
	}
}

static void blit_pen_table_raw_from8bpp_to32bpp_nobuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = theColor+data;
					pDst[x] = data;
				break;
			case DRAWMODE_SHADOW:
					pDst[x] = SHADOW32(pDst[x]);
				break;
			}
		}
		x_index += dx;
	}
}

static void blit_pen_table_raw_from8bpp_to32bpp_pribuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = theColor+data;
				if (((1 << (((UINT8 *)pBuf)[x] & 0x1f)) & pri_code) == 0)
				{
				if (((UINT8 *)pBuf)[x] & 0x80)
					pDst[x] = SHADOW32(data);
				else
					pDst[x] = data;
				}
				((UINT8 *)pBuf)[x] = (((UINT8 *)pBuf)[x] & 0x7f) | 31;
				break;
			case DRAWMODE_SHADOW:
				if (((1 << ((UINT8 *)pBuf)[x]) & pri_code) == 0)
					pDst[x] = SHADOW32(pDst[x]);
				((UINT8 *)pBuf)[x] |= pdrawgfx_shadow_lowpri ? 0 : 0x80;
				break;
			}
		}
		x_index += dx;
	}
}

static void blit_pen_table_raw_from8bpp_to32bpp_zbuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = theColor+data;
					pDst[x] = data;
				break;
			case DRAWMODE_SHADOW:
					pDst[x] = SHADOW32(pDst[x]);
				break;
			}
		}
		x_index += dx;
	}
}

static void blit_blend_from4bpp_to8bpp_nobuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blit_blend_from4bpp_to8bpp_pribuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_blend_from4bpp_to8bpp_zbuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blit_blend_from4bpp_to16bpp_nobuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blit_blend_from4bpp_to16bpp_pribuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_blend_from4bpp_to16bpp_zbuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blit_blend_from4bpp_to32bpp_nobuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blit_blend_from4bpp_to32bpp_pribuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_blend_from4bpp_to32bpp_zbuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blit_blend_from8bpp_to8bpp_nobuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blit_blend_from8bpp_to8bpp_pribuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_blend_from8bpp_to8bpp_zbuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blit_blend_from8bpp_to16bpp_nobuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blit_blend_from8bpp_to16bpp_pribuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_blend_from8bpp_to16bpp_zbuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blit_blend_from8bpp_to32bpp_nobuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blit_blend_from8bpp_to32bpp_pribuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_blend_from8bpp_to32bpp_zbuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blit_blend_raw_from4bpp_to8bpp_nobuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
				pDst[x] |= (data+theColor);
		}
		x_index += dx;
	}
}

static void blit_blend_raw_from4bpp_to8bpp_pribuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] |= (data+theColor);
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_blend_raw_from4bpp_to8bpp_zbuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] |= (data+theColor);
			}
		}
		x_index += dx;
	}
}

static void blit_blend_raw_from4bpp_to16bpp_nobuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
				pDst[x] |= (data+theColor);
		}
		x_index += dx;
	}
}

static void blit_blend_raw_from4bpp_to16bpp_pribuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] |= (data+theColor);
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_blend_raw_from4bpp_to16bpp_zbuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] |= (data+theColor);
			}
		}
		x_index += dx;
	}
}

static void blit_blend_raw_from4bpp_to32bpp_nobuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
				pDst[x] |= (data+theColor);
		}
		x_index += dx;
	}
}

static void blit_blend_raw_from4bpp_to32bpp_pribuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] |= (data+theColor);
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_blend_raw_from4bpp_to32bpp_zbuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] |= (data+theColor);
			}
		}
		x_index += dx;
	}
}

static void blit_blend_raw_from8bpp_to8bpp_nobuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
				pDst[x] |= (data+theColor);
		}
		x_index += dx;
	}
}

static void blit_blend_raw_from8bpp_to8bpp_pribuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] |= (data+theColor);
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_blend_raw_from8bpp_to8bpp_zbuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] |= (data+theColor);
			}
		}
		x_index += dx;
	}
}

static void blit_blend_raw_from8bpp_to16bpp_nobuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
				pDst[x] |= (data+theColor);
		}
		x_index += dx;
	}
}

static void blit_blend_raw_from8bpp_to16bpp_pribuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] |= (data+theColor);
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_blend_raw_from8bpp_to16bpp_zbuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] |= (data+theColor);
			}
		}
		x_index += dx;
	}
}

static void blit_blend_raw_from8bpp_to32bpp_nobuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
				pDst[x] |= (data+theColor);
		}
		x_index += dx;
	}
}

static void blit_blend_raw_from8bpp_to32bpp_pribuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] |= (data+theColor);
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_blend_raw_from8bpp_to32bpp_zbuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] |= (data+theColor);
			}
		}
		x_index += dx;
	}
}

static void blit_alphaone_from4bpp_to8bpp_nobuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	int alpha_pen = transp>>8;
	transp &= 0xff;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
				if( data != alpha_pen )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = pPal[data]; /* no blend in 8bpp */
				}
		}
		x_index += dx;
	}
}

static void blit_alphaone_from4bpp_to8bpp_pribuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	int alpha_pen = transp>>8;
	transp &= 0xff;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				if( data != alpha_pen )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = pPal[data]; /* no blend in 8bpp */
				}
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_alphaone_from4bpp_to8bpp_zbuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	int alpha_pen = transp>>8;
	transp &= 0xff;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				if( data != alpha_pen )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = pPal[data]; /* no blend in 8bpp */
				}
			}
		}
		x_index += dx;
	}
}

static void blit_alphaone_from4bpp_to16bpp_nobuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	int alpha_pen = transp>>8;
	transp &= 0xff;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
				if( data != alpha_pen )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = alpha_blend16(pDst[x], pPal[data]);
				}
		}
		x_index += dx;
	}
}

static void blit_alphaone_from4bpp_to16bpp_pribuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	int alpha_pen = transp>>8;
	transp &= 0xff;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				if( data != alpha_pen )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = alpha_blend16(pDst[x], pPal[data]);
				}
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_alphaone_from4bpp_to16bpp_zbuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	int alpha_pen = transp>>8;
	transp &= 0xff;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				if( data != alpha_pen )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = alpha_blend16(pDst[x], pPal[data]);
				}
			}
		}
		x_index += dx;
	}
}

static void blit_alphaone_from4bpp_to32bpp_nobuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	int alpha_pen = transp>>8;
	transp &= 0xff;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
				if( data != alpha_pen )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = alpha_blend32(pDst[x], pPal[data]);
				}
		}
		x_index += dx;
	}
}

static void blit_alphaone_from4bpp_to32bpp_pribuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	int alpha_pen = transp>>8;
	transp &= 0xff;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				if( data != alpha_pen )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = alpha_blend32(pDst[x], pPal[data]);
				}
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_alphaone_from4bpp_to32bpp_zbuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	int alpha_pen = transp>>8;
	transp &= 0xff;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				if( data != alpha_pen )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = alpha_blend32(pDst[x], pPal[data]);
				}
			}
		}
		x_index += dx;
	}
}

static void blit_alphaone_from8bpp_to8bpp_nobuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	int alpha_pen = transp>>8;
	transp &= 0xff;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
				if( data != alpha_pen )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = pPal[data]; /* no blend in 8bpp */
				}
		}
		x_index += dx;
	}
}

static void blit_alphaone_from8bpp_to8bpp_pribuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	int alpha_pen = transp>>8;
	transp &= 0xff;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				if( data != alpha_pen )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = pPal[data]; /* no blend in 8bpp */
				}
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_alphaone_from8bpp_to8bpp_zbuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	int alpha_pen = transp>>8;
	transp &= 0xff;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				if( data != alpha_pen )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = pPal[data]; /* no blend in 8bpp */
				}
			}
		}
		x_index += dx;
	}
}

static void blit_alphaone_from8bpp_to16bpp_nobuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	int alpha_pen = transp>>8;
	transp &= 0xff;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
				if( data != alpha_pen )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = alpha_blend16(pDst[x], pPal[data]);
				}
		}
		x_index += dx;
	}
}

static void blit_alphaone_from8bpp_to16bpp_pribuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	int alpha_pen = transp>>8;
	transp &= 0xff;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				if( data != alpha_pen )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = alpha_blend16(pDst[x], pPal[data]);
				}
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_alphaone_from8bpp_to16bpp_zbuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	int alpha_pen = transp>>8;
	transp &= 0xff;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				if( data != alpha_pen )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = alpha_blend16(pDst[x], pPal[data]);
				}
			}
		}
		x_index += dx;
	}
}

static void blit_alphaone_from8bpp_to32bpp_nobuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	int alpha_pen = transp>>8;
	transp &= 0xff;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
				if( data != alpha_pen )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = alpha_blend32(pDst[x], pPal[data]);
				}
		}
		x_index += dx;
	}
}

static void blit_alphaone_from8bpp_to32bpp_pribuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	int alpha_pen = transp>>8;
	transp &= 0xff;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				if( data != alpha_pen )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = alpha_blend32(pDst[x], pPal[data]);
				}
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_alphaone_from8bpp_to32bpp_zbuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	int alpha_pen = transp>>8;
	transp &= 0xff;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				if( data != alpha_pen )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = alpha_blend32(pDst[x], pPal[data]);
				}
			}
		}
		x_index += dx;
	}
}

static void blit_alpha_from4bpp_to8bpp_nobuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blit_alpha_from4bpp_to8bpp_pribuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_alpha_from4bpp_to8bpp_zbuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blit_alpha_from4bpp_to16bpp_nobuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
pDst[x] = alpha_blend16(pDst[x], pPal[data]);
		}
		x_index += dx;
	}
}

static void blit_alpha_from4bpp_to16bpp_pribuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
pDst[x] = alpha_blend16(pDst[x], pPal[data]);
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_alpha_from4bpp_to16bpp_zbuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
pDst[x] = alpha_blend16(pDst[x], pPal[data]);
			}
		}
		x_index += dx;
	}
}

static void blit_alpha_from4bpp_to32bpp_nobuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
pDst[x] = alpha_blend32(pDst[x], pPal[data]);
		}
		x_index += dx;
	}
}

static void blit_alpha_from4bpp_to32bpp_pribuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
pDst[x] = alpha_blend32(pDst[x], pPal[data]);
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_alpha_from4bpp_to32bpp_zbuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
pDst[x] = alpha_blend32(pDst[x], pPal[data]);
			}
		}
		x_index += dx;
	}
}

static void blit_alpha_from8bpp_to8bpp_nobuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blit_alpha_from8bpp_to8bpp_pribuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_alpha_from8bpp_to8bpp_zbuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blit_alpha_from8bpp_to16bpp_nobuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
pDst[x] = alpha_blend16(pDst[x], pPal[data]);
		}
		x_index += dx;
	}
}

static void blit_alpha_from8bpp_to16bpp_pribuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
pDst[x] = alpha_blend16(pDst[x], pPal[data]);
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_alpha_from8bpp_to16bpp_zbuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
pDst[x] = alpha_blend16(pDst[x], pPal[data]);
			}
		}
		x_index += dx;
	}
}

static void blit_alpha_from8bpp_to32bpp_nobuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
pDst[x] = alpha_blend32(pDst[x], pPal[data]);
		}
		x_index += dx;
	}
}

static void blit_alpha_from8bpp_to32bpp_pribuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
pDst[x] = alpha_blend32(pDst[x], pPal[data]);
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_alpha_from8bpp_to32bpp_zbuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
pDst[x] = alpha_blend32(pDst[x], pPal[data]);
			}
		}
		x_index += dx;
	}
}

static void blit_alpharange_from4bpp_to8bpp_nobuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
				if( gfx_alpharange_table[data]==0xff )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = pPal[data]; /* no blend in 8bpp */
				}
		}
		x_index += dx;
	}
}

static void blit_alpharange_from4bpp_to8bpp_pribuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				if( gfx_alpharange_table[data]==0xff )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = pPal[data]; /* no blend in 8bpp */
				}
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_alpharange_from4bpp_to8bpp_zbuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				if( gfx_alpharange_table[data]==0xff )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = pPal[data]; /* no blend in 8bpp */
				}
			}
		}
		x_index += dx;
	}
}

static void blit_alpharange_from4bpp_to16bpp_nobuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
				if( gfx_alpharange_table[data]==0xff )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = alpha_blend_r16(pDst[x], pPal[data], gfx_alpharange_table[data] );
				}
		}
		x_index += dx;
	}
}

static void blit_alpharange_from4bpp_to16bpp_pribuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				if( gfx_alpharange_table[data]==0xff )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = alpha_blend_r16(pDst[x], pPal[data], gfx_alpharange_table[data] );
				}
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_alpharange_from4bpp_to16bpp_zbuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				if( gfx_alpharange_table[data]==0xff )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = alpha_blend_r16(pDst[x], pPal[data], gfx_alpharange_table[data] );
				}
			}
		}
		x_index += dx;
	}
}

static void blit_alpharange_from4bpp_to32bpp_nobuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
				if( gfx_alpharange_table[data]==0xff )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = alpha_blend_r32(pDst[x], pPal[data], gfx_alpharange_table[data] );
				}
		}
		x_index += dx;
	}
}

static void blit_alpharange_from4bpp_to32bpp_pribuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				if( gfx_alpharange_table[data]==0xff )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = alpha_blend_r32(pDst[x], pPal[data], gfx_alpharange_table[data] );
				}
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_alpharange_from4bpp_to32bpp_zbuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>1];
		if( x_index&1 ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				if( gfx_alpharange_table[data]==0xff )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = alpha_blend_r32(pDst[x], pPal[data], gfx_alpharange_table[data] );
				}
			}
		}
		x_index += dx;
	}
}

static void blit_alpharange_from8bpp_to8bpp_nobuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
				if( gfx_alpharange_table[data]==0xff )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = pPal[data]; /* no blend in 8bpp */
				}
		}
		x_index += dx;
	}
}

static void blit_alpharange_from8bpp_to8bpp_pribuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				if( gfx_alpharange_table[data]==0xff )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = pPal[data]; /* no blend in 8bpp */
				}
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_alpharange_from8bpp_to8bpp_zbuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				if( gfx_alpharange_table[data]==0xff )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = pPal[data]; /* no blend in 8bpp */
				}
			}
		}
		x_index += dx;
	}
}

static void blit_alpharange_from8bpp_to16bpp_nobuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
				if( gfx_alpharange_table[data]==0xff )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = alpha_blend_r16(pDst[x], pPal[data], gfx_alpharange_table[data] );
				}
		}
		x_index += dx;
	}
}

static void blit_alpharange_from8bpp_to16bpp_pribuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				if( gfx_alpharange_table[data]==0xff )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = alpha_blend_r16(pDst[x], pPal[data], gfx_alpharange_table[data] );
				}
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_alpharange_from8bpp_to16bpp_zbuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				if( gfx_alpharange_table[data]==0xff )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = alpha_blend_r16(pDst[x], pPal[data], gfx_alpharange_table[data] );
				}
			}
		}
		x_index += dx;
	}
}

static void blit_alpharange_from8bpp_to32bpp_nobuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
				if( gfx_alpharange_table[data]==0xff )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = alpha_blend_r32(pDst[x], pPal[data], gfx_alpharange_table[data] );
				}
		}
		x_index += dx;
	}
}

static void blit_alpharange_from8bpp_to32bpp_pribuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				if( gfx_alpharange_table[data]==0xff )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = alpha_blend_r32(pDst[x], pPal[data], gfx_alpharange_table[data] );
				}
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blit_alpharange_from8bpp_to32bpp_zbuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index];
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				if( gfx_alpharange_table[data]==0xff )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = alpha_blend_r32(pDst[x], pPal[data], gfx_alpharange_table[data] );
				}
			}
		}
		x_index += dx;
	}
}

static void blitzoom_none_from4bpp_to8bpp_nobuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if(1){
			data = pPal[data];
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blitzoom_none_from4bpp_to8bpp_pribuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if(1){
			data = pPal[data];
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_none_from4bpp_to8bpp_zbuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if(1){
			data = pPal[data];
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_none_from4bpp_to16bpp_nobuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if(1){
			data = pPal[data];
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blitzoom_none_from4bpp_to16bpp_pribuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if(1){
			data = pPal[data];
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_none_from4bpp_to16bpp_zbuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if(1){
			data = pPal[data];
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_none_from4bpp_to32bpp_nobuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if(1){
			data = pPal[data];
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blitzoom_none_from4bpp_to32bpp_pribuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if(1){
			data = pPal[data];
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_none_from4bpp_to32bpp_zbuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if(1){
			data = pPal[data];
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_none_from8bpp_to8bpp_nobuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if(1){
			data = pPal[data];
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blitzoom_none_from8bpp_to8bpp_pribuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if(1){
			data = pPal[data];
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_none_from8bpp_to8bpp_zbuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if(1){
			data = pPal[data];
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_none_from8bpp_to16bpp_nobuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if(1){
			data = pPal[data];
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blitzoom_none_from8bpp_to16bpp_pribuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if(1){
			data = pPal[data];
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_none_from8bpp_to16bpp_zbuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if(1){
			data = pPal[data];
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_none_from8bpp_to32bpp_nobuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if(1){
			data = pPal[data];
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blitzoom_none_from8bpp_to32bpp_pribuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if(1){
			data = pPal[data];
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_none_from8bpp_to32bpp_zbuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if(1){
			data = pPal[data];
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_none_raw_from4bpp_to8bpp_nobuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if(1){
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blitzoom_none_raw_from4bpp_to8bpp_pribuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if(1){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_none_raw_from4bpp_to8bpp_zbuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if(1){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_none_raw_from4bpp_to16bpp_nobuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if(1){
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blitzoom_none_raw_from4bpp_to16bpp_pribuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if(1){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_none_raw_from4bpp_to16bpp_zbuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if(1){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_none_raw_from4bpp_to32bpp_nobuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if(1){
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blitzoom_none_raw_from4bpp_to32bpp_pribuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if(1){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_none_raw_from4bpp_to32bpp_zbuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if(1){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_none_raw_from8bpp_to8bpp_nobuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if(1){
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blitzoom_none_raw_from8bpp_to8bpp_pribuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if(1){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_none_raw_from8bpp_to8bpp_zbuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if(1){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_none_raw_from8bpp_to16bpp_nobuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if(1){
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blitzoom_none_raw_from8bpp_to16bpp_pribuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if(1){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_none_raw_from8bpp_to16bpp_zbuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if(1){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_none_raw_from8bpp_to32bpp_nobuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if(1){
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blitzoom_none_raw_from8bpp_to32bpp_pribuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if(1){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_none_raw_from8bpp_to32bpp_zbuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if(1){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pen_from4bpp_to8bpp_nobuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			data = pPal[data];
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blitzoom_pen_from4bpp_to8bpp_pribuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			data = pPal[data];
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_pen_from4bpp_to8bpp_zbuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			data = pPal[data];
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pen_from4bpp_to16bpp_nobuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			data = pPal[data];
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blitzoom_pen_from4bpp_to16bpp_pribuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			data = pPal[data];
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_pen_from4bpp_to16bpp_zbuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			data = pPal[data];
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pen_from4bpp_to32bpp_nobuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			data = pPal[data];
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blitzoom_pen_from4bpp_to32bpp_pribuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			data = pPal[data];
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_pen_from4bpp_to32bpp_zbuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			data = pPal[data];
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pen_from8bpp_to8bpp_nobuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			data = pPal[data];
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blitzoom_pen_from8bpp_to8bpp_pribuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			data = pPal[data];
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_pen_from8bpp_to8bpp_zbuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			data = pPal[data];
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pen_from8bpp_to16bpp_nobuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			data = pPal[data];
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blitzoom_pen_from8bpp_to16bpp_pribuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			data = pPal[data];
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_pen_from8bpp_to16bpp_zbuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			data = pPal[data];
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pen_from8bpp_to32bpp_nobuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			data = pPal[data];
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blitzoom_pen_from8bpp_to32bpp_pribuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			data = pPal[data];
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_pen_from8bpp_to32bpp_zbuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			data = pPal[data];
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pen_raw_from4bpp_to8bpp_nobuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blitzoom_pen_raw_from4bpp_to8bpp_pribuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_pen_raw_from4bpp_to8bpp_zbuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pen_raw_from4bpp_to16bpp_nobuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blitzoom_pen_raw_from4bpp_to16bpp_pribuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_pen_raw_from4bpp_to16bpp_zbuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pen_raw_from4bpp_to32bpp_nobuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blitzoom_pen_raw_from4bpp_to32bpp_pribuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_pen_raw_from4bpp_to32bpp_zbuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pen_raw_from8bpp_to8bpp_nobuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blitzoom_pen_raw_from8bpp_to8bpp_pribuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_pen_raw_from8bpp_to8bpp_zbuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pen_raw_from8bpp_to16bpp_nobuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blitzoom_pen_raw_from8bpp_to16bpp_pribuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_pen_raw_from8bpp_to16bpp_zbuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pen_raw_from8bpp_to32bpp_nobuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blitzoom_pen_raw_from8bpp_to32bpp_pribuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_pen_raw_from8bpp_to32bpp_zbuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pens_from4bpp_to8bpp_nobuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( ((1<<data)&transp)==0 ){
			data = pPal[data];
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blitzoom_pens_from4bpp_to8bpp_pribuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( ((1<<data)&transp)==0 ){
			data = pPal[data];
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_pens_from4bpp_to8bpp_zbuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( ((1<<data)&transp)==0 ){
			data = pPal[data];
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pens_from4bpp_to16bpp_nobuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( ((1<<data)&transp)==0 ){
			data = pPal[data];
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blitzoom_pens_from4bpp_to16bpp_pribuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( ((1<<data)&transp)==0 ){
			data = pPal[data];
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_pens_from4bpp_to16bpp_zbuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( ((1<<data)&transp)==0 ){
			data = pPal[data];
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pens_from4bpp_to32bpp_nobuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( ((1<<data)&transp)==0 ){
			data = pPal[data];
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blitzoom_pens_from4bpp_to32bpp_pribuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( ((1<<data)&transp)==0 ){
			data = pPal[data];
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_pens_from4bpp_to32bpp_zbuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( ((1<<data)&transp)==0 ){
			data = pPal[data];
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pens_from8bpp_to8bpp_nobuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( ((1<<data)&transp)==0 ){
			data = pPal[data];
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blitzoom_pens_from8bpp_to8bpp_pribuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( ((1<<data)&transp)==0 ){
			data = pPal[data];
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_pens_from8bpp_to8bpp_zbuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( ((1<<data)&transp)==0 ){
			data = pPal[data];
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pens_from8bpp_to16bpp_nobuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( ((1<<data)&transp)==0 ){
			data = pPal[data];
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blitzoom_pens_from8bpp_to16bpp_pribuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( ((1<<data)&transp)==0 ){
			data = pPal[data];
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_pens_from8bpp_to16bpp_zbuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( ((1<<data)&transp)==0 ){
			data = pPal[data];
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pens_from8bpp_to32bpp_nobuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( ((1<<data)&transp)==0 ){
			data = pPal[data];
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blitzoom_pens_from8bpp_to32bpp_pribuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( ((1<<data)&transp)==0 ){
			data = pPal[data];
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_pens_from8bpp_to32bpp_zbuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( ((1<<data)&transp)==0 ){
			data = pPal[data];
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pens_raw_from4bpp_to8bpp_nobuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( ((1<<data)&transp)==0 ){
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blitzoom_pens_raw_from4bpp_to8bpp_pribuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( ((1<<data)&transp)==0 ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_pens_raw_from4bpp_to8bpp_zbuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( ((1<<data)&transp)==0 ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pens_raw_from4bpp_to16bpp_nobuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( ((1<<data)&transp)==0 ){
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blitzoom_pens_raw_from4bpp_to16bpp_pribuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( ((1<<data)&transp)==0 ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_pens_raw_from4bpp_to16bpp_zbuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( ((1<<data)&transp)==0 ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pens_raw_from4bpp_to32bpp_nobuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( ((1<<data)&transp)==0 ){
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blitzoom_pens_raw_from4bpp_to32bpp_pribuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( ((1<<data)&transp)==0 ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_pens_raw_from4bpp_to32bpp_zbuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( ((1<<data)&transp)==0 ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pens_raw_from8bpp_to8bpp_nobuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( ((1<<data)&transp)==0 ){
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blitzoom_pens_raw_from8bpp_to8bpp_pribuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( ((1<<data)&transp)==0 ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_pens_raw_from8bpp_to8bpp_zbuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( ((1<<data)&transp)==0 ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pens_raw_from8bpp_to16bpp_nobuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( ((1<<data)&transp)==0 ){
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blitzoom_pens_raw_from8bpp_to16bpp_pribuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( ((1<<data)&transp)==0 ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_pens_raw_from8bpp_to16bpp_zbuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( ((1<<data)&transp)==0 ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pens_raw_from8bpp_to32bpp_nobuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( ((1<<data)&transp)==0 ){
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blitzoom_pens_raw_from8bpp_to32bpp_pribuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( ((1<<data)&transp)==0 ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_pens_raw_from8bpp_to32bpp_zbuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( ((1<<data)&transp)==0 ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_color_from4bpp_to8bpp_nobuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	transp = Machine->pens[transp];
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			data = pPal[data];
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blitzoom_color_from4bpp_to8bpp_pribuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	transp = Machine->pens[transp];
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			data = pPal[data];
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_color_from4bpp_to8bpp_zbuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	transp = Machine->pens[transp];
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			data = pPal[data];
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_color_from4bpp_to16bpp_nobuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	transp = Machine->pens[transp];
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			data = pPal[data];
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blitzoom_color_from4bpp_to16bpp_pribuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	transp = Machine->pens[transp];
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			data = pPal[data];
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_color_from4bpp_to16bpp_zbuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	transp = Machine->pens[transp];
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			data = pPal[data];
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_color_from4bpp_to32bpp_nobuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	transp = Machine->pens[transp];
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			data = pPal[data];
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blitzoom_color_from4bpp_to32bpp_pribuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	transp = Machine->pens[transp];
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			data = pPal[data];
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_color_from4bpp_to32bpp_zbuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	transp = Machine->pens[transp];
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			data = pPal[data];
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_color_from8bpp_to8bpp_nobuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	transp = Machine->pens[transp];
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			data = pPal[data];
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blitzoom_color_from8bpp_to8bpp_pribuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	transp = Machine->pens[transp];
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			data = pPal[data];
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_color_from8bpp_to8bpp_zbuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	transp = Machine->pens[transp];
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			data = pPal[data];
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_color_from8bpp_to16bpp_nobuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	transp = Machine->pens[transp];
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			data = pPal[data];
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blitzoom_color_from8bpp_to16bpp_pribuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	transp = Machine->pens[transp];
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			data = pPal[data];
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_color_from8bpp_to16bpp_zbuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	transp = Machine->pens[transp];
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			data = pPal[data];
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_color_from8bpp_to32bpp_nobuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	transp = Machine->pens[transp];
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			data = pPal[data];
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blitzoom_color_from8bpp_to32bpp_pribuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	transp = Machine->pens[transp];
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			data = pPal[data];
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_color_from8bpp_to32bpp_zbuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	transp = Machine->pens[transp];
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			data = pPal[data];
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pen_table_from4bpp_to8bpp_nobuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = pPal[data];
					pDst[x] = data;
				break;
			case DRAWMODE_SHADOW:
					pDst[x] = SHADOW8(pDst[x]);
				break;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pen_table_from4bpp_to8bpp_pribuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = pPal[data];
				if (((1 << (((UINT8 *)pBuf)[x] & 0x1f)) & pri_code) == 0)
				{
				if (((UINT8 *)pBuf)[x] & 0x80)
					pDst[x] = SHADOW8(data);
				else
					pDst[x] = data;
				}
				((UINT8 *)pBuf)[x] = (((UINT8 *)pBuf)[x] & 0x7f) | 31;
				break;
			case DRAWMODE_SHADOW:
				if (((1 << ((UINT8 *)pBuf)[x]) & pri_code) == 0)
					pDst[x] = SHADOW8(pDst[x]);
				((UINT8 *)pBuf)[x] |= pdrawgfx_shadow_lowpri ? 0 : 0x80;
				break;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pen_table_from4bpp_to8bpp_zbuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = pPal[data];
					pDst[x] = data;
				break;
			case DRAWMODE_SHADOW:
					pDst[x] = SHADOW8(pDst[x]);
				break;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pen_table_from4bpp_to16bpp_nobuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = pPal[data];
					pDst[x] = data;
				break;
			case DRAWMODE_SHADOW:
					pDst[x] = SHADOW16(pDst[x]);
				break;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pen_table_from4bpp_to16bpp_pribuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = pPal[data];
				if (((1 << (((UINT8 *)pBuf)[x] & 0x1f)) & pri_code) == 0)
				{
				if (((UINT8 *)pBuf)[x] & 0x80)
					pDst[x] = SHADOW16(data);
				else
					pDst[x] = data;
				}
				((UINT8 *)pBuf)[x] = (((UINT8 *)pBuf)[x] & 0x7f) | 31;
				break;
			case DRAWMODE_SHADOW:
				if (((1 << ((UINT8 *)pBuf)[x]) & pri_code) == 0)
					pDst[x] = SHADOW16(pDst[x]);
				((UINT8 *)pBuf)[x] |= pdrawgfx_shadow_lowpri ? 0 : 0x80;
				break;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pen_table_from4bpp_to16bpp_zbuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = pPal[data];
					pDst[x] = data;
				break;
			case DRAWMODE_SHADOW:
					pDst[x] = SHADOW16(pDst[x]);
				break;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pen_table_from4bpp_to32bpp_nobuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = pPal[data];
					pDst[x] = data;
				break;
			case DRAWMODE_SHADOW:
					pDst[x] = SHADOW32(pDst[x]);
				break;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pen_table_from4bpp_to32bpp_pribuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = pPal[data];
				if (((1 << (((UINT8 *)pBuf)[x] & 0x1f)) & pri_code) == 0)
				{
				if (((UINT8 *)pBuf)[x] & 0x80)
					pDst[x] = SHADOW32(data);
				else
					pDst[x] = data;
				}
				((UINT8 *)pBuf)[x] = (((UINT8 *)pBuf)[x] & 0x7f) | 31;
				break;
			case DRAWMODE_SHADOW:
				if (((1 << ((UINT8 *)pBuf)[x]) & pri_code) == 0)
					pDst[x] = SHADOW32(pDst[x]);
				((UINT8 *)pBuf)[x] |= pdrawgfx_shadow_lowpri ? 0 : 0x80;
				break;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pen_table_from4bpp_to32bpp_zbuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = pPal[data];
					pDst[x] = data;
				break;
			case DRAWMODE_SHADOW:
					pDst[x] = SHADOW32(pDst[x]);
				break;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pen_table_from8bpp_to8bpp_nobuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = pPal[data];
					pDst[x] = data;
				break;
			case DRAWMODE_SHADOW:
					pDst[x] = SHADOW8(pDst[x]);
				break;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pen_table_from8bpp_to8bpp_pribuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = pPal[data];
				if (((1 << (((UINT8 *)pBuf)[x] & 0x1f)) & pri_code) == 0)
				{
				if (((UINT8 *)pBuf)[x] & 0x80)
					pDst[x] = SHADOW8(data);
				else
					pDst[x] = data;
				}
				((UINT8 *)pBuf)[x] = (((UINT8 *)pBuf)[x] & 0x7f) | 31;
				break;
			case DRAWMODE_SHADOW:
				if (((1 << ((UINT8 *)pBuf)[x]) & pri_code) == 0)
					pDst[x] = SHADOW8(pDst[x]);
				((UINT8 *)pBuf)[x] |= pdrawgfx_shadow_lowpri ? 0 : 0x80;
				break;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pen_table_from8bpp_to8bpp_zbuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = pPal[data];
					pDst[x] = data;
				break;
			case DRAWMODE_SHADOW:
					pDst[x] = SHADOW8(pDst[x]);
				break;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pen_table_from8bpp_to16bpp_nobuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = pPal[data];
					pDst[x] = data;
				break;
			case DRAWMODE_SHADOW:
					pDst[x] = SHADOW16(pDst[x]);
				break;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pen_table_from8bpp_to16bpp_pribuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = pPal[data];
				if (((1 << (((UINT8 *)pBuf)[x] & 0x1f)) & pri_code) == 0)
				{
				if (((UINT8 *)pBuf)[x] & 0x80)
					pDst[x] = SHADOW16(data);
				else
					pDst[x] = data;
				}
				((UINT8 *)pBuf)[x] = (((UINT8 *)pBuf)[x] & 0x7f) | 31;
				break;
			case DRAWMODE_SHADOW:
				if (((1 << ((UINT8 *)pBuf)[x]) & pri_code) == 0)
					pDst[x] = SHADOW16(pDst[x]);
				((UINT8 *)pBuf)[x] |= pdrawgfx_shadow_lowpri ? 0 : 0x80;
				break;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pen_table_from8bpp_to16bpp_zbuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = pPal[data];
					pDst[x] = data;
				break;
			case DRAWMODE_SHADOW:
					pDst[x] = SHADOW16(pDst[x]);
				break;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pen_table_from8bpp_to32bpp_nobuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = pPal[data];
					pDst[x] = data;
				break;
			case DRAWMODE_SHADOW:
					pDst[x] = SHADOW32(pDst[x]);
				break;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pen_table_from8bpp_to32bpp_pribuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = pPal[data];
				if (((1 << (((UINT8 *)pBuf)[x] & 0x1f)) & pri_code) == 0)
				{
				if (((UINT8 *)pBuf)[x] & 0x80)
					pDst[x] = SHADOW32(data);
				else
					pDst[x] = data;
				}
				((UINT8 *)pBuf)[x] = (((UINT8 *)pBuf)[x] & 0x7f) | 31;
				break;
			case DRAWMODE_SHADOW:
				if (((1 << ((UINT8 *)pBuf)[x]) & pri_code) == 0)
					pDst[x] = SHADOW32(pDst[x]);
				((UINT8 *)pBuf)[x] |= pdrawgfx_shadow_lowpri ? 0 : 0x80;
				break;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pen_table_from8bpp_to32bpp_zbuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = pPal[data];
					pDst[x] = data;
				break;
			case DRAWMODE_SHADOW:
					pDst[x] = SHADOW32(pDst[x]);
				break;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pen_table_raw_from4bpp_to8bpp_nobuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = theColor+data;
					pDst[x] = data;
				break;
			case DRAWMODE_SHADOW:
					pDst[x] = SHADOW8(pDst[x]);
				break;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pen_table_raw_from4bpp_to8bpp_pribuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = theColor+data;
				if (((1 << (((UINT8 *)pBuf)[x] & 0x1f)) & pri_code) == 0)
				{
				if (((UINT8 *)pBuf)[x] & 0x80)
					pDst[x] = SHADOW8(data);
				else
					pDst[x] = data;
				}
				((UINT8 *)pBuf)[x] = (((UINT8 *)pBuf)[x] & 0x7f) | 31;
				break;
			case DRAWMODE_SHADOW:
				if (((1 << ((UINT8 *)pBuf)[x]) & pri_code) == 0)
					pDst[x] = SHADOW8(pDst[x]);
				((UINT8 *)pBuf)[x] |= pdrawgfx_shadow_lowpri ? 0 : 0x80;
				break;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pen_table_raw_from4bpp_to8bpp_zbuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = theColor+data;
					pDst[x] = data;
				break;
			case DRAWMODE_SHADOW:
					pDst[x] = SHADOW8(pDst[x]);
				break;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pen_table_raw_from4bpp_to16bpp_nobuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = theColor+data;
					pDst[x] = data;
				break;
			case DRAWMODE_SHADOW:
					pDst[x] = SHADOW16(pDst[x]);
				break;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pen_table_raw_from4bpp_to16bpp_pribuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = theColor+data;
				if (((1 << (((UINT8 *)pBuf)[x] & 0x1f)) & pri_code) == 0)
				{
				if (((UINT8 *)pBuf)[x] & 0x80)
					pDst[x] = SHADOW16(data);
				else
					pDst[x] = data;
				}
				((UINT8 *)pBuf)[x] = (((UINT8 *)pBuf)[x] & 0x7f) | 31;
				break;
			case DRAWMODE_SHADOW:
				if (((1 << ((UINT8 *)pBuf)[x]) & pri_code) == 0)
					pDst[x] = SHADOW16(pDst[x]);
				((UINT8 *)pBuf)[x] |= pdrawgfx_shadow_lowpri ? 0 : 0x80;
				break;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pen_table_raw_from4bpp_to16bpp_zbuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = theColor+data;
					pDst[x] = data;
				break;
			case DRAWMODE_SHADOW:
					pDst[x] = SHADOW16(pDst[x]);
				break;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pen_table_raw_from4bpp_to32bpp_nobuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = theColor+data;
					pDst[x] = data;
				break;
			case DRAWMODE_SHADOW:
					pDst[x] = SHADOW32(pDst[x]);
				break;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pen_table_raw_from4bpp_to32bpp_pribuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = theColor+data;
				if (((1 << (((UINT8 *)pBuf)[x] & 0x1f)) & pri_code) == 0)
				{
				if (((UINT8 *)pBuf)[x] & 0x80)
					pDst[x] = SHADOW32(data);
				else
					pDst[x] = data;
				}
				((UINT8 *)pBuf)[x] = (((UINT8 *)pBuf)[x] & 0x7f) | 31;
				break;
			case DRAWMODE_SHADOW:
				if (((1 << ((UINT8 *)pBuf)[x]) & pri_code) == 0)
					pDst[x] = SHADOW32(pDst[x]);
				((UINT8 *)pBuf)[x] |= pdrawgfx_shadow_lowpri ? 0 : 0x80;
				break;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pen_table_raw_from4bpp_to32bpp_zbuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = theColor+data;
					pDst[x] = data;
				break;
			case DRAWMODE_SHADOW:
					pDst[x] = SHADOW32(pDst[x]);
				break;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pen_table_raw_from8bpp_to8bpp_nobuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = theColor+data;
					pDst[x] = data;
				break;
			case DRAWMODE_SHADOW:
					pDst[x] = SHADOW8(pDst[x]);
				break;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pen_table_raw_from8bpp_to8bpp_pribuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = theColor+data;
				if (((1 << (((UINT8 *)pBuf)[x] & 0x1f)) & pri_code) == 0)
				{
				if (((UINT8 *)pBuf)[x] & 0x80)
					pDst[x] = SHADOW8(data);
				else
					pDst[x] = data;
				}
				((UINT8 *)pBuf)[x] = (((UINT8 *)pBuf)[x] & 0x7f) | 31;
				break;
			case DRAWMODE_SHADOW:
				if (((1 << ((UINT8 *)pBuf)[x]) & pri_code) == 0)
					pDst[x] = SHADOW8(pDst[x]);
				((UINT8 *)pBuf)[x] |= pdrawgfx_shadow_lowpri ? 0 : 0x80;
				break;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pen_table_raw_from8bpp_to8bpp_zbuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = theColor+data;
					pDst[x] = data;
				break;
			case DRAWMODE_SHADOW:
					pDst[x] = SHADOW8(pDst[x]);
				break;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pen_table_raw_from8bpp_to16bpp_nobuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = theColor+data;
					pDst[x] = data;
				break;
			case DRAWMODE_SHADOW:
					pDst[x] = SHADOW16(pDst[x]);
				break;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pen_table_raw_from8bpp_to16bpp_pribuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = theColor+data;
				if (((1 << (((UINT8 *)pBuf)[x] & 0x1f)) & pri_code) == 0)
				{
				if (((UINT8 *)pBuf)[x] & 0x80)
					pDst[x] = SHADOW16(data);
				else
					pDst[x] = data;
				}
				((UINT8 *)pBuf)[x] = (((UINT8 *)pBuf)[x] & 0x7f) | 31;
				break;
			case DRAWMODE_SHADOW:
				if (((1 << ((UINT8 *)pBuf)[x]) & pri_code) == 0)
					pDst[x] = SHADOW16(pDst[x]);
				((UINT8 *)pBuf)[x] |= pdrawgfx_shadow_lowpri ? 0 : 0x80;
				break;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pen_table_raw_from8bpp_to16bpp_zbuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = theColor+data;
					pDst[x] = data;
				break;
			case DRAWMODE_SHADOW:
					pDst[x] = SHADOW16(pDst[x]);
				break;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pen_table_raw_from8bpp_to32bpp_nobuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = theColor+data;
					pDst[x] = data;
				break;
			case DRAWMODE_SHADOW:
					pDst[x] = SHADOW32(pDst[x]);
				break;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pen_table_raw_from8bpp_to32bpp_pribuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = theColor+data;
				if (((1 << (((UINT8 *)pBuf)[x] & 0x1f)) & pri_code) == 0)
				{
				if (((UINT8 *)pBuf)[x] & 0x80)
					pDst[x] = SHADOW32(data);
				else
					pDst[x] = data;
				}
				((UINT8 *)pBuf)[x] = (((UINT8 *)pBuf)[x] & 0x7f) | 31;
				break;
			case DRAWMODE_SHADOW:
				if (((1 << ((UINT8 *)pBuf)[x]) & pri_code) == 0)
					pDst[x] = SHADOW32(pDst[x]);
				((UINT8 *)pBuf)[x] |= pdrawgfx_shadow_lowpri ? 0 : 0x80;
				break;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_pen_table_raw_from8bpp_to32bpp_zbuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			switch( gfx_drawmode_table[data] )
			{
			case DRAWMODE_SOURCE:
				data = theColor+data;
					pDst[x] = data;
				break;
			case DRAWMODE_SHADOW:
					pDst[x] = SHADOW32(pDst[x]);
				break;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_blend_from4bpp_to8bpp_nobuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blitzoom_blend_from4bpp_to8bpp_pribuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_blend_from4bpp_to8bpp_zbuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_blend_from4bpp_to16bpp_nobuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blitzoom_blend_from4bpp_to16bpp_pribuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_blend_from4bpp_to16bpp_zbuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_blend_from4bpp_to32bpp_nobuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blitzoom_blend_from4bpp_to32bpp_pribuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_blend_from4bpp_to32bpp_zbuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_blend_from8bpp_to8bpp_nobuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blitzoom_blend_from8bpp_to8bpp_pribuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_blend_from8bpp_to8bpp_zbuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_blend_from8bpp_to16bpp_nobuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blitzoom_blend_from8bpp_to16bpp_pribuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_blend_from8bpp_to16bpp_zbuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_blend_from8bpp_to32bpp_nobuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blitzoom_blend_from8bpp_to32bpp_pribuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_blend_from8bpp_to32bpp_zbuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_blend_raw_from4bpp_to8bpp_nobuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
				pDst[x] |= (data+theColor);
		}
		x_index += dx;
	}
}

static void blitzoom_blend_raw_from4bpp_to8bpp_pribuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] |= (data+theColor);
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_blend_raw_from4bpp_to8bpp_zbuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] |= (data+theColor);
			}
		}
		x_index += dx;
	}
}

static void blitzoom_blend_raw_from4bpp_to16bpp_nobuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
				pDst[x] |= (data+theColor);
		}
		x_index += dx;
	}
}

static void blitzoom_blend_raw_from4bpp_to16bpp_pribuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] |= (data+theColor);
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_blend_raw_from4bpp_to16bpp_zbuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] |= (data+theColor);
			}
		}
		x_index += dx;
	}
}

static void blitzoom_blend_raw_from4bpp_to32bpp_nobuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
				pDst[x] |= (data+theColor);
		}
		x_index += dx;
	}
}

static void blitzoom_blend_raw_from4bpp_to32bpp_pribuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] |= (data+theColor);
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_blend_raw_from4bpp_to32bpp_zbuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] |= (data+theColor);
			}
		}
		x_index += dx;
	}
}

static void blitzoom_blend_raw_from8bpp_to8bpp_nobuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
				pDst[x] |= (data+theColor);
		}
		x_index += dx;
	}
}

static void blitzoom_blend_raw_from8bpp_to8bpp_pribuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] |= (data+theColor);
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_blend_raw_from8bpp_to8bpp_zbuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] |= (data+theColor);
			}
		}
		x_index += dx;
	}
}

static void blitzoom_blend_raw_from8bpp_to16bpp_nobuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
				pDst[x] |= (data+theColor);
		}
		x_index += dx;
	}
}

static void blitzoom_blend_raw_from8bpp_to16bpp_pribuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] |= (data+theColor);
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_blend_raw_from8bpp_to16bpp_zbuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] |= (data+theColor);
			}
		}
		x_index += dx;
	}
}

static void blitzoom_blend_raw_from8bpp_to32bpp_nobuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
				pDst[x] |= (data+theColor);
		}
		x_index += dx;
	}
}

static void blitzoom_blend_raw_from8bpp_to32bpp_pribuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] |= (data+theColor);
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_blend_raw_from8bpp_to32bpp_zbuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] |= (data+theColor);
			}
		}
		x_index += dx;
	}
}

static void blitzoom_alphaone_from4bpp_to8bpp_nobuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	int alpha_pen = transp>>8;
	transp &= 0xff;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
				if( data != alpha_pen )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = pPal[data]; /* no blend in 8bpp */
				}
		}
		x_index += dx;
	}
}

static void blitzoom_alphaone_from4bpp_to8bpp_pribuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	int alpha_pen = transp>>8;
	transp &= 0xff;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				if( data != alpha_pen )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = pPal[data]; /* no blend in 8bpp */
				}
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_alphaone_from4bpp_to8bpp_zbuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	int alpha_pen = transp>>8;
	transp &= 0xff;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				if( data != alpha_pen )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = pPal[data]; /* no blend in 8bpp */
				}
			}
		}
		x_index += dx;
	}
}

static void blitzoom_alphaone_from4bpp_to16bpp_nobuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	int alpha_pen = transp>>8;
	transp &= 0xff;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
				if( data != alpha_pen )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = alpha_blend16(pDst[x], pPal[data]);
				}
		}
		x_index += dx;
	}
}

static void blitzoom_alphaone_from4bpp_to16bpp_pribuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	int alpha_pen = transp>>8;
	transp &= 0xff;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				if( data != alpha_pen )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = alpha_blend16(pDst[x], pPal[data]);
				}
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_alphaone_from4bpp_to16bpp_zbuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	int alpha_pen = transp>>8;
	transp &= 0xff;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				if( data != alpha_pen )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = alpha_blend16(pDst[x], pPal[data]);
				}
			}
		}
		x_index += dx;
	}
}

static void blitzoom_alphaone_from4bpp_to32bpp_nobuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	int alpha_pen = transp>>8;
	transp &= 0xff;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
				if( data != alpha_pen )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = alpha_blend32(pDst[x], pPal[data]);
				}
		}
		x_index += dx;
	}
}

static void blitzoom_alphaone_from4bpp_to32bpp_pribuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	int alpha_pen = transp>>8;
	transp &= 0xff;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				if( data != alpha_pen )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = alpha_blend32(pDst[x], pPal[data]);
				}
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_alphaone_from4bpp_to32bpp_zbuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	int alpha_pen = transp>>8;
	transp &= 0xff;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				if( data != alpha_pen )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = alpha_blend32(pDst[x], pPal[data]);
				}
			}
		}
		x_index += dx;
	}
}

static void blitzoom_alphaone_from8bpp_to8bpp_nobuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	int alpha_pen = transp>>8;
	transp &= 0xff;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
				if( data != alpha_pen )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = pPal[data]; /* no blend in 8bpp */
				}
		}
		x_index += dx;
	}
}

static void blitzoom_alphaone_from8bpp_to8bpp_pribuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	int alpha_pen = transp>>8;
	transp &= 0xff;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				if( data != alpha_pen )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = pPal[data]; /* no blend in 8bpp */
				}
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_alphaone_from8bpp_to8bpp_zbuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	int alpha_pen = transp>>8;
	transp &= 0xff;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				if( data != alpha_pen )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = pPal[data]; /* no blend in 8bpp */
				}
			}
		}
		x_index += dx;
	}
}

static void blitzoom_alphaone_from8bpp_to16bpp_nobuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	int alpha_pen = transp>>8;
	transp &= 0xff;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
				if( data != alpha_pen )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = alpha_blend16(pDst[x], pPal[data]);
				}
		}
		x_index += dx;
	}
}

static void blitzoom_alphaone_from8bpp_to16bpp_pribuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	int alpha_pen = transp>>8;
	transp &= 0xff;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				if( data != alpha_pen )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = alpha_blend16(pDst[x], pPal[data]);
				}
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_alphaone_from8bpp_to16bpp_zbuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	int alpha_pen = transp>>8;
	transp &= 0xff;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				if( data != alpha_pen )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = alpha_blend16(pDst[x], pPal[data]);
				}
			}
		}
		x_index += dx;
	}
}

static void blitzoom_alphaone_from8bpp_to32bpp_nobuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	int alpha_pen = transp>>8;
	transp &= 0xff;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
				if( data != alpha_pen )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = alpha_blend32(pDst[x], pPal[data]);
				}
		}
		x_index += dx;
	}
}

static void blitzoom_alphaone_from8bpp_to32bpp_pribuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	int alpha_pen = transp>>8;
	transp &= 0xff;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				if( data != alpha_pen )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = alpha_blend32(pDst[x], pPal[data]);
				}
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_alphaone_from8bpp_to32bpp_zbuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	int alpha_pen = transp>>8;
	transp &= 0xff;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				if( data != alpha_pen )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = alpha_blend32(pDst[x], pPal[data]);
				}
			}
		}
		x_index += dx;
	}
}

static void blitzoom_alpha_from4bpp_to8bpp_nobuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blitzoom_alpha_from4bpp_to8bpp_pribuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_alpha_from4bpp_to8bpp_zbuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_alpha_from4bpp_to16bpp_nobuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
pDst[x] = alpha_blend16(pDst[x], pPal[data]);
		}
		x_index += dx;
	}
}

static void blitzoom_alpha_from4bpp_to16bpp_pribuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
pDst[x] = alpha_blend16(pDst[x], pPal[data]);
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_alpha_from4bpp_to16bpp_zbuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
pDst[x] = alpha_blend16(pDst[x], pPal[data]);
			}
		}
		x_index += dx;
	}
}

static void blitzoom_alpha_from4bpp_to32bpp_nobuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
pDst[x] = alpha_blend32(pDst[x], pPal[data]);
		}
		x_index += dx;
	}
}

static void blitzoom_alpha_from4bpp_to32bpp_pribuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
pDst[x] = alpha_blend32(pDst[x], pPal[data]);
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_alpha_from4bpp_to32bpp_zbuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
pDst[x] = alpha_blend32(pDst[x], pPal[data]);
			}
		}
		x_index += dx;
	}
}

static void blitzoom_alpha_from8bpp_to8bpp_nobuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
				pDst[x] = data;
		}
		x_index += dx;
	}
}

static void blitzoom_alpha_from8bpp_to8bpp_pribuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				pDst[x] = data;
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_alpha_from8bpp_to8bpp_zbuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				pDst[x] = data;
			}
		}
		x_index += dx;
	}
}

static void blitzoom_alpha_from8bpp_to16bpp_nobuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
pDst[x] = alpha_blend16(pDst[x], pPal[data]);
		}
		x_index += dx;
	}
}

static void blitzoom_alpha_from8bpp_to16bpp_pribuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
pDst[x] = alpha_blend16(pDst[x], pPal[data]);
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_alpha_from8bpp_to16bpp_zbuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
pDst[x] = alpha_blend16(pDst[x], pPal[data]);
			}
		}
		x_index += dx;
	}
}

static void blitzoom_alpha_from8bpp_to32bpp_nobuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
pDst[x] = alpha_blend32(pDst[x], pPal[data]);
		}
		x_index += dx;
	}
}

static void blitzoom_alpha_from8bpp_to32bpp_pribuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
pDst[x] = alpha_blend32(pDst[x], pPal[data]);
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_alpha_from8bpp_to32bpp_zbuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
pDst[x] = alpha_blend32(pDst[x], pPal[data]);
			}
		}
		x_index += dx;
	}
}

static void blitzoom_alpharange_from4bpp_to8bpp_nobuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
				if( gfx_alpharange_table[data]==0xff )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = pPal[data]; /* no blend in 8bpp */
				}
		}
		x_index += dx;
	}
}

static void blitzoom_alpharange_from4bpp_to8bpp_pribuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				if( gfx_alpharange_table[data]==0xff )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = pPal[data]; /* no blend in 8bpp */
				}
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_alpharange_from4bpp_to8bpp_zbuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				if( gfx_alpharange_table[data]==0xff )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = pPal[data]; /* no blend in 8bpp */
				}
			}
		}
		x_index += dx;
	}
}

static void blitzoom_alpharange_from4bpp_to16bpp_nobuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
				if( gfx_alpharange_table[data]==0xff )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = alpha_blend_r16(pDst[x], pPal[data], gfx_alpharange_table[data] );
				}
		}
		x_index += dx;
	}
}

static void blitzoom_alpharange_from4bpp_to16bpp_pribuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				if( gfx_alpharange_table[data]==0xff )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = alpha_blend_r16(pDst[x], pPal[data], gfx_alpharange_table[data] );
				}
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_alpharange_from4bpp_to16bpp_zbuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				if( gfx_alpharange_table[data]==0xff )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = alpha_blend_r16(pDst[x], pPal[data], gfx_alpharange_table[data] );
				}
			}
		}
		x_index += dx;
	}
}

static void blitzoom_alpharange_from4bpp_to32bpp_nobuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
				if( gfx_alpharange_table[data]==0xff )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = alpha_blend_r32(pDst[x], pPal[data], gfx_alpharange_table[data] );
				}
		}
		x_index += dx;
	}
}

static void blitzoom_alpharange_from4bpp_to32bpp_pribuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				if( gfx_alpharange_table[data]==0xff )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = alpha_blend_r32(pDst[x], pPal[data], gfx_alpharange_table[data] );
				}
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_alpharange_from4bpp_to32bpp_zbuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>17];
		if( x_index&(1<<16) ) data>>=4; else data&=0xf;
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				if( gfx_alpharange_table[data]==0xff )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = alpha_blend_r32(pDst[x], pPal[data], gfx_alpharange_table[data] );
				}
			}
		}
		x_index += dx;
	}
}

static void blitzoom_alpharange_from8bpp_to8bpp_nobuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
				if( gfx_alpharange_table[data]==0xff )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = pPal[data]; /* no blend in 8bpp */
				}
		}
		x_index += dx;
	}
}

static void blitzoom_alpharange_from8bpp_to8bpp_pribuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				if( gfx_alpharange_table[data]==0xff )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = pPal[data]; /* no blend in 8bpp */
				}
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_alpharange_from8bpp_to8bpp_zbuf( UINT8 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				if( gfx_alpharange_table[data]==0xff )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = pPal[data]; /* no blend in 8bpp */
				}
			}
		}
		x_index += dx;
	}
}

static void blitzoom_alpharange_from8bpp_to16bpp_nobuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
				if( gfx_alpharange_table[data]==0xff )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = alpha_blend_r16(pDst[x], pPal[data], gfx_alpharange_table[data] );
				}
		}
		x_index += dx;
	}
}

static void blitzoom_alpharange_from8bpp_to16bpp_pribuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				if( gfx_alpharange_table[data]==0xff )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = alpha_blend_r16(pDst[x], pPal[data], gfx_alpharange_table[data] );
				}
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_alpharange_from8bpp_to16bpp_zbuf( UINT16 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				if( gfx_alpharange_table[data]==0xff )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = alpha_blend_r16(pDst[x], pPal[data], gfx_alpharange_table[data] );
				}
			}
		}
		x_index += dx;
	}
}

static void blitzoom_alpharange_from8bpp_to32bpp_nobuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
				if( gfx_alpharange_table[data]==0xff )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = alpha_blend_r32(pDst[x], pPal[data], gfx_alpharange_table[data] );
				}
		}
		x_index += dx;
	}
}

static void blitzoom_alpharange_from8bpp_to32bpp_pribuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){
				if( gfx_alpharange_table[data]==0xff )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = alpha_blend_r32(pDst[x], pPal[data], gfx_alpharange_table[data] );
				}
			}
			((UINT8 *)pBuf)[x] = 31;
		}
		x_index += dx;
	}
}

static void blitzoom_alpharange_from8bpp_to32bpp_zbuf( UINT32 *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){
	int x;
	for( x=sx; x<ex; x++ )
	{
		int data = ((UINT8 *)pSrc)[x_index>>16];
		if( data!=transp ){
			if( pri_code>=((UINT8 *)pBuf)[x]){
				((UINT8 *)pBuf)[x] = pri_code;
				if( gfx_alpharange_table[data]==0xff )
				{
					pDst[x] = pPal[data];
				}
				else
				{
					pDst[x] = alpha_blend_r32(pDst[x], pPal[data], gfx_alpharange_table[data] );
				}
			}
		}
		x_index += dx;
	}
}
typedef void blitter( void *pDst, const void *pSrc, const pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code );
static blitter *mBlitTable[] ={
(blitter *)blit_none_from4bpp_to8bpp_nobuf,
(blitter *)blit_none_from4bpp_to8bpp_pribuf,
(blitter *)blit_none_from4bpp_to8bpp_zbuf,
(blitter *)blit_none_from4bpp_to16bpp_nobuf,
(blitter *)blit_none_from4bpp_to16bpp_pribuf,
(blitter *)blit_none_from4bpp_to16bpp_zbuf,
(blitter *)blit_none_from4bpp_to32bpp_nobuf,
(blitter *)blit_none_from4bpp_to32bpp_pribuf,
(blitter *)blit_none_from4bpp_to32bpp_zbuf,
(blitter *)blit_none_from8bpp_to8bpp_nobuf,
(blitter *)blit_none_from8bpp_to8bpp_pribuf,
(blitter *)blit_none_from8bpp_to8bpp_zbuf,
(blitter *)blit_none_from8bpp_to16bpp_nobuf,
(blitter *)blit_none_from8bpp_to16bpp_pribuf,
(blitter *)blit_none_from8bpp_to16bpp_zbuf,
(blitter *)blit_none_from8bpp_to32bpp_nobuf,
(blitter *)blit_none_from8bpp_to32bpp_pribuf,
(blitter *)blit_none_from8bpp_to32bpp_zbuf,
(blitter *)blit_none_raw_from4bpp_to8bpp_nobuf,
(blitter *)blit_none_raw_from4bpp_to8bpp_pribuf,
(blitter *)blit_none_raw_from4bpp_to8bpp_zbuf,
(blitter *)blit_none_raw_from4bpp_to16bpp_nobuf,
(blitter *)blit_none_raw_from4bpp_to16bpp_pribuf,
(blitter *)blit_none_raw_from4bpp_to16bpp_zbuf,
(blitter *)blit_none_raw_from4bpp_to32bpp_nobuf,
(blitter *)blit_none_raw_from4bpp_to32bpp_pribuf,
(blitter *)blit_none_raw_from4bpp_to32bpp_zbuf,
(blitter *)blit_none_raw_from8bpp_to8bpp_nobuf,
(blitter *)blit_none_raw_from8bpp_to8bpp_pribuf,
(blitter *)blit_none_raw_from8bpp_to8bpp_zbuf,
(blitter *)blit_none_raw_from8bpp_to16bpp_nobuf,
(blitter *)blit_none_raw_from8bpp_to16bpp_pribuf,
(blitter *)blit_none_raw_from8bpp_to16bpp_zbuf,
(blitter *)blit_none_raw_from8bpp_to32bpp_nobuf,
(blitter *)blit_none_raw_from8bpp_to32bpp_pribuf,
(blitter *)blit_none_raw_from8bpp_to32bpp_zbuf,
(blitter *)blit_pen_from4bpp_to8bpp_nobuf,
(blitter *)blit_pen_from4bpp_to8bpp_pribuf,
(blitter *)blit_pen_from4bpp_to8bpp_zbuf,
(blitter *)blit_pen_from4bpp_to16bpp_nobuf,
(blitter *)blit_pen_from4bpp_to16bpp_pribuf,
(blitter *)blit_pen_from4bpp_to16bpp_zbuf,
(blitter *)blit_pen_from4bpp_to32bpp_nobuf,
(blitter *)blit_pen_from4bpp_to32bpp_pribuf,
(blitter *)blit_pen_from4bpp_to32bpp_zbuf,
(blitter *)blit_pen_from8bpp_to8bpp_nobuf,
(blitter *)blit_pen_from8bpp_to8bpp_pribuf,
(blitter *)blit_pen_from8bpp_to8bpp_zbuf,
(blitter *)blit_pen_from8bpp_to16bpp_nobuf,
(blitter *)blit_pen_from8bpp_to16bpp_pribuf,
(blitter *)blit_pen_from8bpp_to16bpp_zbuf,
(blitter *)blit_pen_from8bpp_to32bpp_nobuf,
(blitter *)blit_pen_from8bpp_to32bpp_pribuf,
(blitter *)blit_pen_from8bpp_to32bpp_zbuf,
(blitter *)blit_pen_raw_from4bpp_to8bpp_nobuf,
(blitter *)blit_pen_raw_from4bpp_to8bpp_pribuf,
(blitter *)blit_pen_raw_from4bpp_to8bpp_zbuf,
(blitter *)blit_pen_raw_from4bpp_to16bpp_nobuf,
(blitter *)blit_pen_raw_from4bpp_to16bpp_pribuf,
(blitter *)blit_pen_raw_from4bpp_to16bpp_zbuf,
(blitter *)blit_pen_raw_from4bpp_to32bpp_nobuf,
(blitter *)blit_pen_raw_from4bpp_to32bpp_pribuf,
(blitter *)blit_pen_raw_from4bpp_to32bpp_zbuf,
(blitter *)blit_pen_raw_from8bpp_to8bpp_nobuf,
(blitter *)blit_pen_raw_from8bpp_to8bpp_pribuf,
(blitter *)blit_pen_raw_from8bpp_to8bpp_zbuf,
(blitter *)blit_pen_raw_from8bpp_to16bpp_nobuf,
(blitter *)blit_pen_raw_from8bpp_to16bpp_pribuf,
(blitter *)blit_pen_raw_from8bpp_to16bpp_zbuf,
(blitter *)blit_pen_raw_from8bpp_to32bpp_nobuf,
(blitter *)blit_pen_raw_from8bpp_to32bpp_pribuf,
(blitter *)blit_pen_raw_from8bpp_to32bpp_zbuf,
(blitter *)blit_pens_from4bpp_to8bpp_nobuf,
(blitter *)blit_pens_from4bpp_to8bpp_pribuf,
(blitter *)blit_pens_from4bpp_to8bpp_zbuf,
(blitter *)blit_pens_from4bpp_to16bpp_nobuf,
(blitter *)blit_pens_from4bpp_to16bpp_pribuf,
(blitter *)blit_pens_from4bpp_to16bpp_zbuf,
(blitter *)blit_pens_from4bpp_to32bpp_nobuf,
(blitter *)blit_pens_from4bpp_to32bpp_pribuf,
(blitter *)blit_pens_from4bpp_to32bpp_zbuf,
(blitter *)blit_pens_from8bpp_to8bpp_nobuf,
(blitter *)blit_pens_from8bpp_to8bpp_pribuf,
(blitter *)blit_pens_from8bpp_to8bpp_zbuf,
(blitter *)blit_pens_from8bpp_to16bpp_nobuf,
(blitter *)blit_pens_from8bpp_to16bpp_pribuf,
(blitter *)blit_pens_from8bpp_to16bpp_zbuf,
(blitter *)blit_pens_from8bpp_to32bpp_nobuf,
(blitter *)blit_pens_from8bpp_to32bpp_pribuf,
(blitter *)blit_pens_from8bpp_to32bpp_zbuf,
(blitter *)blit_pens_raw_from4bpp_to8bpp_nobuf,
(blitter *)blit_pens_raw_from4bpp_to8bpp_pribuf,
(blitter *)blit_pens_raw_from4bpp_to8bpp_zbuf,
(blitter *)blit_pens_raw_from4bpp_to16bpp_nobuf,
(blitter *)blit_pens_raw_from4bpp_to16bpp_pribuf,
(blitter *)blit_pens_raw_from4bpp_to16bpp_zbuf,
(blitter *)blit_pens_raw_from4bpp_to32bpp_nobuf,
(blitter *)blit_pens_raw_from4bpp_to32bpp_pribuf,
(blitter *)blit_pens_raw_from4bpp_to32bpp_zbuf,
(blitter *)blit_pens_raw_from8bpp_to8bpp_nobuf,
(blitter *)blit_pens_raw_from8bpp_to8bpp_pribuf,
(blitter *)blit_pens_raw_from8bpp_to8bpp_zbuf,
(blitter *)blit_pens_raw_from8bpp_to16bpp_nobuf,
(blitter *)blit_pens_raw_from8bpp_to16bpp_pribuf,
(blitter *)blit_pens_raw_from8bpp_to16bpp_zbuf,
(blitter *)blit_pens_raw_from8bpp_to32bpp_nobuf,
(blitter *)blit_pens_raw_from8bpp_to32bpp_pribuf,
(blitter *)blit_pens_raw_from8bpp_to32bpp_zbuf,
(blitter *)blit_color_from4bpp_to8bpp_nobuf,
(blitter *)blit_color_from4bpp_to8bpp_pribuf,
(blitter *)blit_color_from4bpp_to8bpp_zbuf,
(blitter *)blit_color_from4bpp_to16bpp_nobuf,
(blitter *)blit_color_from4bpp_to16bpp_pribuf,
(blitter *)blit_color_from4bpp_to16bpp_zbuf,
(blitter *)blit_color_from4bpp_to32bpp_nobuf,
(blitter *)blit_color_from4bpp_to32bpp_pribuf,
(blitter *)blit_color_from4bpp_to32bpp_zbuf,
(blitter *)blit_color_from8bpp_to8bpp_nobuf,
(blitter *)blit_color_from8bpp_to8bpp_pribuf,
(blitter *)blit_color_from8bpp_to8bpp_zbuf,
(blitter *)blit_color_from8bpp_to16bpp_nobuf,
(blitter *)blit_color_from8bpp_to16bpp_pribuf,
(blitter *)blit_color_from8bpp_to16bpp_zbuf,
(blitter *)blit_color_from8bpp_to32bpp_nobuf,
(blitter *)blit_color_from8bpp_to32bpp_pribuf,
(blitter *)blit_color_from8bpp_to32bpp_zbuf,
(blitter *)blit_pen_table_from4bpp_to8bpp_nobuf,
(blitter *)blit_pen_table_from4bpp_to8bpp_pribuf,
(blitter *)blit_pen_table_from4bpp_to8bpp_zbuf,
(blitter *)blit_pen_table_from4bpp_to16bpp_nobuf,
(blitter *)blit_pen_table_from4bpp_to16bpp_pribuf,
(blitter *)blit_pen_table_from4bpp_to16bpp_zbuf,
(blitter *)blit_pen_table_from4bpp_to32bpp_nobuf,
(blitter *)blit_pen_table_from4bpp_to32bpp_pribuf,
(blitter *)blit_pen_table_from4bpp_to32bpp_zbuf,
(blitter *)blit_pen_table_from8bpp_to8bpp_nobuf,
(blitter *)blit_pen_table_from8bpp_to8bpp_pribuf,
(blitter *)blit_pen_table_from8bpp_to8bpp_zbuf,
(blitter *)blit_pen_table_from8bpp_to16bpp_nobuf,
(blitter *)blit_pen_table_from8bpp_to16bpp_pribuf,
(blitter *)blit_pen_table_from8bpp_to16bpp_zbuf,
(blitter *)blit_pen_table_from8bpp_to32bpp_nobuf,
(blitter *)blit_pen_table_from8bpp_to32bpp_pribuf,
(blitter *)blit_pen_table_from8bpp_to32bpp_zbuf,
(blitter *)blit_pen_table_raw_from4bpp_to8bpp_nobuf,
(blitter *)blit_pen_table_raw_from4bpp_to8bpp_pribuf,
(blitter *)blit_pen_table_raw_from4bpp_to8bpp_zbuf,
(blitter *)blit_pen_table_raw_from4bpp_to16bpp_nobuf,
(blitter *)blit_pen_table_raw_from4bpp_to16bpp_pribuf,
(blitter *)blit_pen_table_raw_from4bpp_to16bpp_zbuf,
(blitter *)blit_pen_table_raw_from4bpp_to32bpp_nobuf,
(blitter *)blit_pen_table_raw_from4bpp_to32bpp_pribuf,
(blitter *)blit_pen_table_raw_from4bpp_to32bpp_zbuf,
(blitter *)blit_pen_table_raw_from8bpp_to8bpp_nobuf,
(blitter *)blit_pen_table_raw_from8bpp_to8bpp_pribuf,
(blitter *)blit_pen_table_raw_from8bpp_to8bpp_zbuf,
(blitter *)blit_pen_table_raw_from8bpp_to16bpp_nobuf,
(blitter *)blit_pen_table_raw_from8bpp_to16bpp_pribuf,
(blitter *)blit_pen_table_raw_from8bpp_to16bpp_zbuf,
(blitter *)blit_pen_table_raw_from8bpp_to32bpp_nobuf,
(blitter *)blit_pen_table_raw_from8bpp_to32bpp_pribuf,
(blitter *)blit_pen_table_raw_from8bpp_to32bpp_zbuf,
(blitter *)blit_blend_from4bpp_to8bpp_nobuf,
(blitter *)blit_blend_from4bpp_to8bpp_pribuf,
(blitter *)blit_blend_from4bpp_to8bpp_zbuf,
(blitter *)blit_blend_from4bpp_to16bpp_nobuf,
(blitter *)blit_blend_from4bpp_to16bpp_pribuf,
(blitter *)blit_blend_from4bpp_to16bpp_zbuf,
(blitter *)blit_blend_from4bpp_to32bpp_nobuf,
(blitter *)blit_blend_from4bpp_to32bpp_pribuf,
(blitter *)blit_blend_from4bpp_to32bpp_zbuf,
(blitter *)blit_blend_from8bpp_to8bpp_nobuf,
(blitter *)blit_blend_from8bpp_to8bpp_pribuf,
(blitter *)blit_blend_from8bpp_to8bpp_zbuf,
(blitter *)blit_blend_from8bpp_to16bpp_nobuf,
(blitter *)blit_blend_from8bpp_to16bpp_pribuf,
(blitter *)blit_blend_from8bpp_to16bpp_zbuf,
(blitter *)blit_blend_from8bpp_to32bpp_nobuf,
(blitter *)blit_blend_from8bpp_to32bpp_pribuf,
(blitter *)blit_blend_from8bpp_to32bpp_zbuf,
(blitter *)blit_blend_raw_from4bpp_to8bpp_nobuf,
(blitter *)blit_blend_raw_from4bpp_to8bpp_pribuf,
(blitter *)blit_blend_raw_from4bpp_to8bpp_zbuf,
(blitter *)blit_blend_raw_from4bpp_to16bpp_nobuf,
(blitter *)blit_blend_raw_from4bpp_to16bpp_pribuf,
(blitter *)blit_blend_raw_from4bpp_to16bpp_zbuf,
(blitter *)blit_blend_raw_from4bpp_to32bpp_nobuf,
(blitter *)blit_blend_raw_from4bpp_to32bpp_pribuf,
(blitter *)blit_blend_raw_from4bpp_to32bpp_zbuf,
(blitter *)blit_blend_raw_from8bpp_to8bpp_nobuf,
(blitter *)blit_blend_raw_from8bpp_to8bpp_pribuf,
(blitter *)blit_blend_raw_from8bpp_to8bpp_zbuf,
(blitter *)blit_blend_raw_from8bpp_to16bpp_nobuf,
(blitter *)blit_blend_raw_from8bpp_to16bpp_pribuf,
(blitter *)blit_blend_raw_from8bpp_to16bpp_zbuf,
(blitter *)blit_blend_raw_from8bpp_to32bpp_nobuf,
(blitter *)blit_blend_raw_from8bpp_to32bpp_pribuf,
(blitter *)blit_blend_raw_from8bpp_to32bpp_zbuf,
(blitter *)blit_alphaone_from4bpp_to8bpp_nobuf,
(blitter *)blit_alphaone_from4bpp_to8bpp_pribuf,
(blitter *)blit_alphaone_from4bpp_to8bpp_zbuf,
(blitter *)blit_alphaone_from4bpp_to16bpp_nobuf,
(blitter *)blit_alphaone_from4bpp_to16bpp_pribuf,
(blitter *)blit_alphaone_from4bpp_to16bpp_zbuf,
(blitter *)blit_alphaone_from4bpp_to32bpp_nobuf,
(blitter *)blit_alphaone_from4bpp_to32bpp_pribuf,
(blitter *)blit_alphaone_from4bpp_to32bpp_zbuf,
(blitter *)blit_alphaone_from8bpp_to8bpp_nobuf,
(blitter *)blit_alphaone_from8bpp_to8bpp_pribuf,
(blitter *)blit_alphaone_from8bpp_to8bpp_zbuf,
(blitter *)blit_alphaone_from8bpp_to16bpp_nobuf,
(blitter *)blit_alphaone_from8bpp_to16bpp_pribuf,
(blitter *)blit_alphaone_from8bpp_to16bpp_zbuf,
(blitter *)blit_alphaone_from8bpp_to32bpp_nobuf,
(blitter *)blit_alphaone_from8bpp_to32bpp_pribuf,
(blitter *)blit_alphaone_from8bpp_to32bpp_zbuf,
(blitter *)blit_alpha_from4bpp_to8bpp_nobuf,
(blitter *)blit_alpha_from4bpp_to8bpp_pribuf,
(blitter *)blit_alpha_from4bpp_to8bpp_zbuf,
(blitter *)blit_alpha_from4bpp_to16bpp_nobuf,
(blitter *)blit_alpha_from4bpp_to16bpp_pribuf,
(blitter *)blit_alpha_from4bpp_to16bpp_zbuf,
(blitter *)blit_alpha_from4bpp_to32bpp_nobuf,
(blitter *)blit_alpha_from4bpp_to32bpp_pribuf,
(blitter *)blit_alpha_from4bpp_to32bpp_zbuf,
(blitter *)blit_alpha_from8bpp_to8bpp_nobuf,
(blitter *)blit_alpha_from8bpp_to8bpp_pribuf,
(blitter *)blit_alpha_from8bpp_to8bpp_zbuf,
(blitter *)blit_alpha_from8bpp_to16bpp_nobuf,
(blitter *)blit_alpha_from8bpp_to16bpp_pribuf,
(blitter *)blit_alpha_from8bpp_to16bpp_zbuf,
(blitter *)blit_alpha_from8bpp_to32bpp_nobuf,
(blitter *)blit_alpha_from8bpp_to32bpp_pribuf,
(blitter *)blit_alpha_from8bpp_to32bpp_zbuf,
(blitter *)blit_alpharange_from4bpp_to8bpp_nobuf,
(blitter *)blit_alpharange_from4bpp_to8bpp_pribuf,
(blitter *)blit_alpharange_from4bpp_to8bpp_zbuf,
(blitter *)blit_alpharange_from4bpp_to16bpp_nobuf,
(blitter *)blit_alpharange_from4bpp_to16bpp_pribuf,
(blitter *)blit_alpharange_from4bpp_to16bpp_zbuf,
(blitter *)blit_alpharange_from4bpp_to32bpp_nobuf,
(blitter *)blit_alpharange_from4bpp_to32bpp_pribuf,
(blitter *)blit_alpharange_from4bpp_to32bpp_zbuf,
(blitter *)blit_alpharange_from8bpp_to8bpp_nobuf,
(blitter *)blit_alpharange_from8bpp_to8bpp_pribuf,
(blitter *)blit_alpharange_from8bpp_to8bpp_zbuf,
(blitter *)blit_alpharange_from8bpp_to16bpp_nobuf,
(blitter *)blit_alpharange_from8bpp_to16bpp_pribuf,
(blitter *)blit_alpharange_from8bpp_to16bpp_zbuf,
(blitter *)blit_alpharange_from8bpp_to32bpp_nobuf,
(blitter *)blit_alpharange_from8bpp_to32bpp_pribuf,
(blitter *)blit_alpharange_from8bpp_to32bpp_zbuf,
(blitter *)blitzoom_none_from4bpp_to8bpp_nobuf,
(blitter *)blitzoom_none_from4bpp_to8bpp_pribuf,
(blitter *)blitzoom_none_from4bpp_to8bpp_zbuf,
(blitter *)blitzoom_none_from4bpp_to16bpp_nobuf,
(blitter *)blitzoom_none_from4bpp_to16bpp_pribuf,
(blitter *)blitzoom_none_from4bpp_to16bpp_zbuf,
(blitter *)blitzoom_none_from4bpp_to32bpp_nobuf,
(blitter *)blitzoom_none_from4bpp_to32bpp_pribuf,
(blitter *)blitzoom_none_from4bpp_to32bpp_zbuf,
(blitter *)blitzoom_none_from8bpp_to8bpp_nobuf,
(blitter *)blitzoom_none_from8bpp_to8bpp_pribuf,
(blitter *)blitzoom_none_from8bpp_to8bpp_zbuf,
(blitter *)blitzoom_none_from8bpp_to16bpp_nobuf,
(blitter *)blitzoom_none_from8bpp_to16bpp_pribuf,
(blitter *)blitzoom_none_from8bpp_to16bpp_zbuf,
(blitter *)blitzoom_none_from8bpp_to32bpp_nobuf,
(blitter *)blitzoom_none_from8bpp_to32bpp_pribuf,
(blitter *)blitzoom_none_from8bpp_to32bpp_zbuf,
(blitter *)blitzoom_none_raw_from4bpp_to8bpp_nobuf,
(blitter *)blitzoom_none_raw_from4bpp_to8bpp_pribuf,
(blitter *)blitzoom_none_raw_from4bpp_to8bpp_zbuf,
(blitter *)blitzoom_none_raw_from4bpp_to16bpp_nobuf,
(blitter *)blitzoom_none_raw_from4bpp_to16bpp_pribuf,
(blitter *)blitzoom_none_raw_from4bpp_to16bpp_zbuf,
(blitter *)blitzoom_none_raw_from4bpp_to32bpp_nobuf,
(blitter *)blitzoom_none_raw_from4bpp_to32bpp_pribuf,
(blitter *)blitzoom_none_raw_from4bpp_to32bpp_zbuf,
(blitter *)blitzoom_none_raw_from8bpp_to8bpp_nobuf,
(blitter *)blitzoom_none_raw_from8bpp_to8bpp_pribuf,
(blitter *)blitzoom_none_raw_from8bpp_to8bpp_zbuf,
(blitter *)blitzoom_none_raw_from8bpp_to16bpp_nobuf,
(blitter *)blitzoom_none_raw_from8bpp_to16bpp_pribuf,
(blitter *)blitzoom_none_raw_from8bpp_to16bpp_zbuf,
(blitter *)blitzoom_none_raw_from8bpp_to32bpp_nobuf,
(blitter *)blitzoom_none_raw_from8bpp_to32bpp_pribuf,
(blitter *)blitzoom_none_raw_from8bpp_to32bpp_zbuf,
(blitter *)blitzoom_pen_from4bpp_to8bpp_nobuf,
(blitter *)blitzoom_pen_from4bpp_to8bpp_pribuf,
(blitter *)blitzoom_pen_from4bpp_to8bpp_zbuf,
(blitter *)blitzoom_pen_from4bpp_to16bpp_nobuf,
(blitter *)blitzoom_pen_from4bpp_to16bpp_pribuf,
(blitter *)blitzoom_pen_from4bpp_to16bpp_zbuf,
(blitter *)blitzoom_pen_from4bpp_to32bpp_nobuf,
(blitter *)blitzoom_pen_from4bpp_to32bpp_pribuf,
(blitter *)blitzoom_pen_from4bpp_to32bpp_zbuf,
(blitter *)blitzoom_pen_from8bpp_to8bpp_nobuf,
(blitter *)blitzoom_pen_from8bpp_to8bpp_pribuf,
(blitter *)blitzoom_pen_from8bpp_to8bpp_zbuf,
(blitter *)blitzoom_pen_from8bpp_to16bpp_nobuf,
(blitter *)blitzoom_pen_from8bpp_to16bpp_pribuf,
(blitter *)blitzoom_pen_from8bpp_to16bpp_zbuf,
(blitter *)blitzoom_pen_from8bpp_to32bpp_nobuf,
(blitter *)blitzoom_pen_from8bpp_to32bpp_pribuf,
(blitter *)blitzoom_pen_from8bpp_to32bpp_zbuf,
(blitter *)blitzoom_pen_raw_from4bpp_to8bpp_nobuf,
(blitter *)blitzoom_pen_raw_from4bpp_to8bpp_pribuf,
(blitter *)blitzoom_pen_raw_from4bpp_to8bpp_zbuf,
(blitter *)blitzoom_pen_raw_from4bpp_to16bpp_nobuf,
(blitter *)blitzoom_pen_raw_from4bpp_to16bpp_pribuf,
(blitter *)blitzoom_pen_raw_from4bpp_to16bpp_zbuf,
(blitter *)blitzoom_pen_raw_from4bpp_to32bpp_nobuf,
(blitter *)blitzoom_pen_raw_from4bpp_to32bpp_pribuf,
(blitter *)blitzoom_pen_raw_from4bpp_to32bpp_zbuf,
(blitter *)blitzoom_pen_raw_from8bpp_to8bpp_nobuf,
(blitter *)blitzoom_pen_raw_from8bpp_to8bpp_pribuf,
(blitter *)blitzoom_pen_raw_from8bpp_to8bpp_zbuf,
(blitter *)blitzoom_pen_raw_from8bpp_to16bpp_nobuf,
(blitter *)blitzoom_pen_raw_from8bpp_to16bpp_pribuf,
(blitter *)blitzoom_pen_raw_from8bpp_to16bpp_zbuf,
(blitter *)blitzoom_pen_raw_from8bpp_to32bpp_nobuf,
(blitter *)blitzoom_pen_raw_from8bpp_to32bpp_pribuf,
(blitter *)blitzoom_pen_raw_from8bpp_to32bpp_zbuf,
(blitter *)blitzoom_pens_from4bpp_to8bpp_nobuf,
(blitter *)blitzoom_pens_from4bpp_to8bpp_pribuf,
(blitter *)blitzoom_pens_from4bpp_to8bpp_zbuf,
(blitter *)blitzoom_pens_from4bpp_to16bpp_nobuf,
(blitter *)blitzoom_pens_from4bpp_to16bpp_pribuf,
(blitter *)blitzoom_pens_from4bpp_to16bpp_zbuf,
(blitter *)blitzoom_pens_from4bpp_to32bpp_nobuf,
(blitter *)blitzoom_pens_from4bpp_to32bpp_pribuf,
(blitter *)blitzoom_pens_from4bpp_to32bpp_zbuf,
(blitter *)blitzoom_pens_from8bpp_to8bpp_nobuf,
(blitter *)blitzoom_pens_from8bpp_to8bpp_pribuf,
(blitter *)blitzoom_pens_from8bpp_to8bpp_zbuf,
(blitter *)blitzoom_pens_from8bpp_to16bpp_nobuf,
(blitter *)blitzoom_pens_from8bpp_to16bpp_pribuf,
(blitter *)blitzoom_pens_from8bpp_to16bpp_zbuf,
(blitter *)blitzoom_pens_from8bpp_to32bpp_nobuf,
(blitter *)blitzoom_pens_from8bpp_to32bpp_pribuf,
(blitter *)blitzoom_pens_from8bpp_to32bpp_zbuf,
(blitter *)blitzoom_pens_raw_from4bpp_to8bpp_nobuf,
(blitter *)blitzoom_pens_raw_from4bpp_to8bpp_pribuf,
(blitter *)blitzoom_pens_raw_from4bpp_to8bpp_zbuf,
(blitter *)blitzoom_pens_raw_from4bpp_to16bpp_nobuf,
(blitter *)blitzoom_pens_raw_from4bpp_to16bpp_pribuf,
(blitter *)blitzoom_pens_raw_from4bpp_to16bpp_zbuf,
(blitter *)blitzoom_pens_raw_from4bpp_to32bpp_nobuf,
(blitter *)blitzoom_pens_raw_from4bpp_to32bpp_pribuf,
(blitter *)blitzoom_pens_raw_from4bpp_to32bpp_zbuf,
(blitter *)blitzoom_pens_raw_from8bpp_to8bpp_nobuf,
(blitter *)blitzoom_pens_raw_from8bpp_to8bpp_pribuf,
(blitter *)blitzoom_pens_raw_from8bpp_to8bpp_zbuf,
(blitter *)blitzoom_pens_raw_from8bpp_to16bpp_nobuf,
(blitter *)blitzoom_pens_raw_from8bpp_to16bpp_pribuf,
(blitter *)blitzoom_pens_raw_from8bpp_to16bpp_zbuf,
(blitter *)blitzoom_pens_raw_from8bpp_to32bpp_nobuf,
(blitter *)blitzoom_pens_raw_from8bpp_to32bpp_pribuf,
(blitter *)blitzoom_pens_raw_from8bpp_to32bpp_zbuf,
(blitter *)blitzoom_color_from4bpp_to8bpp_nobuf,
(blitter *)blitzoom_color_from4bpp_to8bpp_pribuf,
(blitter *)blitzoom_color_from4bpp_to8bpp_zbuf,
(blitter *)blitzoom_color_from4bpp_to16bpp_nobuf,
(blitter *)blitzoom_color_from4bpp_to16bpp_pribuf,
(blitter *)blitzoom_color_from4bpp_to16bpp_zbuf,
(blitter *)blitzoom_color_from4bpp_to32bpp_nobuf,
(blitter *)blitzoom_color_from4bpp_to32bpp_pribuf,
(blitter *)blitzoom_color_from4bpp_to32bpp_zbuf,
(blitter *)blitzoom_color_from8bpp_to8bpp_nobuf,
(blitter *)blitzoom_color_from8bpp_to8bpp_pribuf,
(blitter *)blitzoom_color_from8bpp_to8bpp_zbuf,
(blitter *)blitzoom_color_from8bpp_to16bpp_nobuf,
(blitter *)blitzoom_color_from8bpp_to16bpp_pribuf,
(blitter *)blitzoom_color_from8bpp_to16bpp_zbuf,
(blitter *)blitzoom_color_from8bpp_to32bpp_nobuf,
(blitter *)blitzoom_color_from8bpp_to32bpp_pribuf,
(blitter *)blitzoom_color_from8bpp_to32bpp_zbuf,
(blitter *)blitzoom_pen_table_from4bpp_to8bpp_nobuf,
(blitter *)blitzoom_pen_table_from4bpp_to8bpp_pribuf,
(blitter *)blitzoom_pen_table_from4bpp_to8bpp_zbuf,
(blitter *)blitzoom_pen_table_from4bpp_to16bpp_nobuf,
(blitter *)blitzoom_pen_table_from4bpp_to16bpp_pribuf,
(blitter *)blitzoom_pen_table_from4bpp_to16bpp_zbuf,
(blitter *)blitzoom_pen_table_from4bpp_to32bpp_nobuf,
(blitter *)blitzoom_pen_table_from4bpp_to32bpp_pribuf,
(blitter *)blitzoom_pen_table_from4bpp_to32bpp_zbuf,
(blitter *)blitzoom_pen_table_from8bpp_to8bpp_nobuf,
(blitter *)blitzoom_pen_table_from8bpp_to8bpp_pribuf,
(blitter *)blitzoom_pen_table_from8bpp_to8bpp_zbuf,
(blitter *)blitzoom_pen_table_from8bpp_to16bpp_nobuf,
(blitter *)blitzoom_pen_table_from8bpp_to16bpp_pribuf,
(blitter *)blitzoom_pen_table_from8bpp_to16bpp_zbuf,
(blitter *)blitzoom_pen_table_from8bpp_to32bpp_nobuf,
(blitter *)blitzoom_pen_table_from8bpp_to32bpp_pribuf,
(blitter *)blitzoom_pen_table_from8bpp_to32bpp_zbuf,
(blitter *)blitzoom_pen_table_raw_from4bpp_to8bpp_nobuf,
(blitter *)blitzoom_pen_table_raw_from4bpp_to8bpp_pribuf,
(blitter *)blitzoom_pen_table_raw_from4bpp_to8bpp_zbuf,
(blitter *)blitzoom_pen_table_raw_from4bpp_to16bpp_nobuf,
(blitter *)blitzoom_pen_table_raw_from4bpp_to16bpp_pribuf,
(blitter *)blitzoom_pen_table_raw_from4bpp_to16bpp_zbuf,
(blitter *)blitzoom_pen_table_raw_from4bpp_to32bpp_nobuf,
(blitter *)blitzoom_pen_table_raw_from4bpp_to32bpp_pribuf,
(blitter *)blitzoom_pen_table_raw_from4bpp_to32bpp_zbuf,
(blitter *)blitzoom_pen_table_raw_from8bpp_to8bpp_nobuf,
(blitter *)blitzoom_pen_table_raw_from8bpp_to8bpp_pribuf,
(blitter *)blitzoom_pen_table_raw_from8bpp_to8bpp_zbuf,
(blitter *)blitzoom_pen_table_raw_from8bpp_to16bpp_nobuf,
(blitter *)blitzoom_pen_table_raw_from8bpp_to16bpp_pribuf,
(blitter *)blitzoom_pen_table_raw_from8bpp_to16bpp_zbuf,
(blitter *)blitzoom_pen_table_raw_from8bpp_to32bpp_nobuf,
(blitter *)blitzoom_pen_table_raw_from8bpp_to32bpp_pribuf,
(blitter *)blitzoom_pen_table_raw_from8bpp_to32bpp_zbuf,
(blitter *)blitzoom_blend_from4bpp_to8bpp_nobuf,
(blitter *)blitzoom_blend_from4bpp_to8bpp_pribuf,
(blitter *)blitzoom_blend_from4bpp_to8bpp_zbuf,
(blitter *)blitzoom_blend_from4bpp_to16bpp_nobuf,
(blitter *)blitzoom_blend_from4bpp_to16bpp_pribuf,
(blitter *)blitzoom_blend_from4bpp_to16bpp_zbuf,
(blitter *)blitzoom_blend_from4bpp_to32bpp_nobuf,
(blitter *)blitzoom_blend_from4bpp_to32bpp_pribuf,
(blitter *)blitzoom_blend_from4bpp_to32bpp_zbuf,
(blitter *)blitzoom_blend_from8bpp_to8bpp_nobuf,
(blitter *)blitzoom_blend_from8bpp_to8bpp_pribuf,
(blitter *)blitzoom_blend_from8bpp_to8bpp_zbuf,
(blitter *)blitzoom_blend_from8bpp_to16bpp_nobuf,
(blitter *)blitzoom_blend_from8bpp_to16bpp_pribuf,
(blitter *)blitzoom_blend_from8bpp_to16bpp_zbuf,
(blitter *)blitzoom_blend_from8bpp_to32bpp_nobuf,
(blitter *)blitzoom_blend_from8bpp_to32bpp_pribuf,
(blitter *)blitzoom_blend_from8bpp_to32bpp_zbuf,
(blitter *)blitzoom_blend_raw_from4bpp_to8bpp_nobuf,
(blitter *)blitzoom_blend_raw_from4bpp_to8bpp_pribuf,
(blitter *)blitzoom_blend_raw_from4bpp_to8bpp_zbuf,
(blitter *)blitzoom_blend_raw_from4bpp_to16bpp_nobuf,
(blitter *)blitzoom_blend_raw_from4bpp_to16bpp_pribuf,
(blitter *)blitzoom_blend_raw_from4bpp_to16bpp_zbuf,
(blitter *)blitzoom_blend_raw_from4bpp_to32bpp_nobuf,
(blitter *)blitzoom_blend_raw_from4bpp_to32bpp_pribuf,
(blitter *)blitzoom_blend_raw_from4bpp_to32bpp_zbuf,
(blitter *)blitzoom_blend_raw_from8bpp_to8bpp_nobuf,
(blitter *)blitzoom_blend_raw_from8bpp_to8bpp_pribuf,
(blitter *)blitzoom_blend_raw_from8bpp_to8bpp_zbuf,
(blitter *)blitzoom_blend_raw_from8bpp_to16bpp_nobuf,
(blitter *)blitzoom_blend_raw_from8bpp_to16bpp_pribuf,
(blitter *)blitzoom_blend_raw_from8bpp_to16bpp_zbuf,
(blitter *)blitzoom_blend_raw_from8bpp_to32bpp_nobuf,
(blitter *)blitzoom_blend_raw_from8bpp_to32bpp_pribuf,
(blitter *)blitzoom_blend_raw_from8bpp_to32bpp_zbuf,
(blitter *)blitzoom_alphaone_from4bpp_to8bpp_nobuf,
(blitter *)blitzoom_alphaone_from4bpp_to8bpp_pribuf,
(blitter *)blitzoom_alphaone_from4bpp_to8bpp_zbuf,
(blitter *)blitzoom_alphaone_from4bpp_to16bpp_nobuf,
(blitter *)blitzoom_alphaone_from4bpp_to16bpp_pribuf,
(blitter *)blitzoom_alphaone_from4bpp_to16bpp_zbuf,
(blitter *)blitzoom_alphaone_from4bpp_to32bpp_nobuf,
(blitter *)blitzoom_alphaone_from4bpp_to32bpp_pribuf,
(blitter *)blitzoom_alphaone_from4bpp_to32bpp_zbuf,
(blitter *)blitzoom_alphaone_from8bpp_to8bpp_nobuf,
(blitter *)blitzoom_alphaone_from8bpp_to8bpp_pribuf,
(blitter *)blitzoom_alphaone_from8bpp_to8bpp_zbuf,
(blitter *)blitzoom_alphaone_from8bpp_to16bpp_nobuf,
(blitter *)blitzoom_alphaone_from8bpp_to16bpp_pribuf,
(blitter *)blitzoom_alphaone_from8bpp_to16bpp_zbuf,
(blitter *)blitzoom_alphaone_from8bpp_to32bpp_nobuf,
(blitter *)blitzoom_alphaone_from8bpp_to32bpp_pribuf,
(blitter *)blitzoom_alphaone_from8bpp_to32bpp_zbuf,
(blitter *)blitzoom_alpha_from4bpp_to8bpp_nobuf,
(blitter *)blitzoom_alpha_from4bpp_to8bpp_pribuf,
(blitter *)blitzoom_alpha_from4bpp_to8bpp_zbuf,
(blitter *)blitzoom_alpha_from4bpp_to16bpp_nobuf,
(blitter *)blitzoom_alpha_from4bpp_to16bpp_pribuf,
(blitter *)blitzoom_alpha_from4bpp_to16bpp_zbuf,
(blitter *)blitzoom_alpha_from4bpp_to32bpp_nobuf,
(blitter *)blitzoom_alpha_from4bpp_to32bpp_pribuf,
(blitter *)blitzoom_alpha_from4bpp_to32bpp_zbuf,
(blitter *)blitzoom_alpha_from8bpp_to8bpp_nobuf,
(blitter *)blitzoom_alpha_from8bpp_to8bpp_pribuf,
(blitter *)blitzoom_alpha_from8bpp_to8bpp_zbuf,
(blitter *)blitzoom_alpha_from8bpp_to16bpp_nobuf,
(blitter *)blitzoom_alpha_from8bpp_to16bpp_pribuf,
(blitter *)blitzoom_alpha_from8bpp_to16bpp_zbuf,
(blitter *)blitzoom_alpha_from8bpp_to32bpp_nobuf,
(blitter *)blitzoom_alpha_from8bpp_to32bpp_pribuf,
(blitter *)blitzoom_alpha_from8bpp_to32bpp_zbuf,
(blitter *)blitzoom_alpharange_from4bpp_to8bpp_nobuf,
(blitter *)blitzoom_alpharange_from4bpp_to8bpp_pribuf,
(blitter *)blitzoom_alpharange_from4bpp_to8bpp_zbuf,
(blitter *)blitzoom_alpharange_from4bpp_to16bpp_nobuf,
(blitter *)blitzoom_alpharange_from4bpp_to16bpp_pribuf,
(blitter *)blitzoom_alpharange_from4bpp_to16bpp_zbuf,
(blitter *)blitzoom_alpharange_from4bpp_to32bpp_nobuf,
(blitter *)blitzoom_alpharange_from4bpp_to32bpp_pribuf,
(blitter *)blitzoom_alpharange_from4bpp_to32bpp_zbuf,
(blitter *)blitzoom_alpharange_from8bpp_to8bpp_nobuf,
(blitter *)blitzoom_alpharange_from8bpp_to8bpp_pribuf,
(blitter *)blitzoom_alpharange_from8bpp_to8bpp_zbuf,
(blitter *)blitzoom_alpharange_from8bpp_to16bpp_nobuf,
(blitter *)blitzoom_alpharange_from8bpp_to16bpp_pribuf,
(blitter *)blitzoom_alpharange_from8bpp_to16bpp_zbuf,
(blitter *)blitzoom_alpharange_from8bpp_to32bpp_nobuf,
(blitter *)blitzoom_alpharange_from8bpp_to32bpp_pribuf,
(blitter *)blitzoom_alpharange_from8bpp_to32bpp_zbuf,
};
