/**
 * This module emits a full set of blitters, as "mameblit.c"
 */

#include <stdio.h>

enum
{
	TRANSPARENCY_NONE,
	TRANSPARENCY_NONE_RAW,
	TRANSPARENCY_PEN,
	TRANSPARENCY_PEN_RAW,
	TRANSPARENCY_PENS,
	TRANSPARENCY_PENS_RAW,
	TRANSPARENCY_COLOR,
	TRANSPARENCY_PEN_TABLE,
	TRANSPARENCY_PEN_TABLE_RAW,
	TRANSPARENCY_BLEND,
	TRANSPARENCY_BLEND_RAW,
	TRANSPARENCY_ALPHAONE,
	TRANSPARENCY_ALPHA,
	TRANSPARENCY_ALPHARANGE,
	TRANSPARENCY_MODES
};
const char *transparency_name[TRANSPARENCY_MODES] =
{
	"none","none_raw",
	"pen","pen_raw",
	"pens","pens_raw",
	"color",
	"pen_table","pen_table_raw",
	"blend","blend_raw",
	"alphaone","alpha","alpharange"
};

enum
{
	SOURCE_4BPP,
	SOURCE_8BPP,
	SOURCE_TYPES
};
const char *source_name[SOURCE_TYPES] = { "from4bpp","from8bpp" };

enum
{
	DEST_8BPP,
	DEST_16BPP,
	DEST_32BPP,
	DEST_TYPES
};
const char *dest_name[DEST_TYPES] = { "to8bpp","to16bpp","to32bpp" };

enum
{
	BUF_NONE,
	BUF_PRI,
	BUF_Z,
	BUF_TYPES
};
const char *buf_name[BUF_TYPES] = { "nobuf", "pribuf", "zbuf" };

void
main( void )
{
	int bpp;
	int i;
	FILE *f = fopen( "mameblit.c", "w" );
	if( f )
	{
fprintf( f, "/**\n" );
fprintf( f, " * Do not modify this module directly.  It is generated code, written by blitgen.c\n" );
fprintf( f, " *\n" );
fprintf( f, " * The implementation is not yet optimal.\n" );
fprintf( f, " * To do:\n" );
fprintf( f, " * - use duff-unrolled loops\n" );
fprintf( f, " * - include ystart,yend params, so that the blitter can be called once for each drawgfx()\n" );
fprintf( f, " *   instead of once per line.  There's a lot of parameters being repeatedly pushed onto\n" );
fprintf( f, " *   the stack with the current implementation.\n" );
fprintf( f, " */\n" );
fprintf( f, "static int theColor;\n" );
fprintf( f, "\n" );
fprintf( f, "\n" );
fprintf( f, "#define SHADOW8(data) palette_shadow_table[data]\n" );
fprintf( f, "\n" );
fprintf( f, "#define SHADOW16(data) palette_shadow_table[data]\n" );
fprintf( f, "\n" );	
fprintf( f, "//* AAT 032503: added limited 32-bit shadow and highlight support\n" );
fprintf( f, "INLINE int SHADOW32(int c)\n" );
fprintf( f, "{\n" );
fprintf( f, "	#define RGB8TO5(x) (((x)>>3&0x001f)|((x)>>6&0x03e0)|((x)>>9&0x7c00))\n" );
fprintf( f, "	#define RGB5TO8(x) (((x)<<3&0x00f8)|((x)<<6&0xf800)|((x)<<9&0xf80000))\n" );
fprintf( f, "	// DEPENDENCY CHAIN!!!\n" );
fprintf( f, "	c = RGB8TO5(c);\n" );
fprintf( f, "	c = palette_shadow_table[c];\n" );
fprintf( f, "	c = RGB5TO8(c);\n" );
fprintf( f, "	return(c);\n" );
fprintf( f, "	#undef RGB8TO5\n" );
fprintf( f, "	#undef RGB5TO8\n" );
fprintf( f, "}\n" );
fprintf( f, "\n" );

		for( i=0; i<2; i++ )
		{
			int transparency_mode;
			for( transparency_mode=0; transparency_mode<TRANSPARENCY_MODES; transparency_mode++ )
			{
				int source_type;
				for( source_type=0; source_type<SOURCE_TYPES; source_type++ )
				{
					int dest_type;
					for( dest_type=0; dest_type<DEST_TYPES; dest_type++ )
					{
						int buf_type;
						for( buf_type=0; buf_type<BUF_TYPES; buf_type++ )
						{
							int bRemapPens;

fprintf( f, "\n" );
							if( i )
							{
fprintf( f, "static void blitzoom" );
							}
							else
							{
fprintf( f, "static void blit" );
							}

fprintf( f, "_%s_%s_%s_%s( ",
								transparency_name[transparency_mode],
								source_name[source_type],
								dest_name[dest_type],
								buf_name[buf_type] );

							switch( dest_type )
							{
							case DEST_8BPP:
fprintf( f, "UINT8" );
								bpp = 8;
								break;
							case DEST_16BPP:
fprintf( f, "UINT16" );
								bpp = 16;
								break;
							case DEST_32BPP:
fprintf( f, "UINT32" );
								bpp = 32;
								break;
							}
fprintf( f, " *pDst, void *pSrc, pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code ){\n" );
fprintf( f, "	int x;\n" );

							switch( transparency_mode )
							{
							case TRANSPARENCY_COLOR:
fprintf( f, "	transp = Machine->pens[transp];\n" );
								break;
							case TRANSPARENCY_ALPHAONE:
fprintf( f, "	int alpha_pen = transp>>8;\n" );
fprintf( f, "	transp &= 0xff;\n" );
								break;
							}

fprintf( f, "	for( x=sx; x<ex; x++ )\n" );
fprintf( f, "	{\n" );

							if( i )
							{
								switch( source_type )
								{
								case SOURCE_4BPP:
fprintf( f, "		int data = ((UINT8 *)pSrc)[x_index>>17];\n" );
fprintf( f, "		if( x_index&(1<<16) ) data>>=4; else data&=0xf;\n" );
									break;
								case SOURCE_8BPP:
fprintf( f, "		int data = ((UINT8 *)pSrc)[x_index>>16];\n" );
									break;
								}
							}
							else
							{
								switch( source_type )
								{
								case SOURCE_4BPP:
fprintf( f, "		int data = ((UINT8 *)pSrc)[x_index>>1];\n" );
fprintf( f, "		if( x_index&1 ) data>>=4; else data&=0xf;\n" );
									break;
								case SOURCE_8BPP:
fprintf( f, "		int data = ((UINT8 *)pSrc)[x_index];\n" );
									break;
								}
							}

							bRemapPens = 0; /* default */
							switch( transparency_mode )
							{
							case TRANSPARENCY_NONE:			/* opaque with remapping */
								bRemapPens = 1;
							/* fallthrough */
							case TRANSPARENCY_NONE_RAW:		/* opaque with no remapping */
fprintf( f, "		if(1){\n" );
								break;

							case TRANSPARENCY_COLOR:		/* single remapped pen transparency with remapping */
							case TRANSPARENCY_PEN:			/* single pen transparency with remapping */
								bRemapPens = 1;
							/* fallthrough */
							case TRANSPARENCY_PEN_RAW:		/* single pen transparency with no remapping */
fprintf( f, "		if( data!=transp ){\n" );
								break;

							case TRANSPARENCY_PENS:			/* multiple pen transparency with remapping */
								bRemapPens = 1;
							/* fallthrough */
							case TRANSPARENCY_PENS_RAW:		/* multiple pen transparency with no remapping */
fprintf( f, "		if( ((1<<data)&transp)==0 ){\n" );
								break;

							case TRANSPARENCY_PEN_TABLE:	/* special pen remapping modes (see DRAWMODE_xxx below) with remapping */
fprintf( f, "		if( data!=transp ){\n" );
								break;

							case TRANSPARENCY_PEN_TABLE_RAW:/* special pen remapping modes (see DRAWMODE_xxx below) with no remapping */
fprintf( f, "		if( data!=transp ){\n" );
								break;

							case TRANSPARENCY_BLEND:		/* blend two bitmaps, shifting the source and ORing to the dest with remapping */
fprintf( f, "		if( data!=transp ){\n" );
								break;

							case TRANSPARENCY_BLEND_RAW:	/* blend two bitmaps, shifting the source and ORing to the dest with no remapping */
fprintf( f, "		if( data!=transp ){\n" );
								break;

							case TRANSPARENCY_ALPHAONE:		/* single pen transparency, single pen alpha */
fprintf( f, "		if( data!=transp ){\n" );
								break;

							case TRANSPARENCY_ALPHA:		/* single pen transparency, other pens alpha */
fprintf( f, "		if( data!=transp ){\n" );
								break;

							case TRANSPARENCY_ALPHARANGE:	/* single pen transparency, multiple pens alpha depending on array, see psikyosh.c */
fprintf( f, "		if( data!=transp ){\n" );
								break;
							}

							if( bRemapPens )
							{
fprintf( f, "			data = pPal[data];\n" );
							}

							if( transparency_mode == TRANSPARENCY_PEN_TABLE ||
								transparency_mode == TRANSPARENCY_PEN_TABLE_RAW )
							{
fprintf( f, "			switch( gfx_drawmode_table[data] )\n" );
fprintf( f, "			{\n" );
fprintf( f, "			case DRAWMODE_SOURCE:\n" );
								if( transparency_mode == TRANSPARENCY_PEN_TABLE )
								{
fprintf( f, "				data = pPal[data];\n" );
								}
								else
								{
fprintf( f, "				data = theColor+data;\n" );
								}
								if( buf_type == BUF_PRI )
								{
fprintf( f, "				if (((1 << (((UINT8 *)pBuf)[x] & 0x1f)) & pri_code) == 0)\n" );
fprintf( f, "				{\n" );
fprintf( f, "				if (((UINT8 *)pBuf)[x] & 0x80)\n" );
fprintf( f, "					pDst[x] = SHADOW%d(data);\n", bpp );
fprintf( f, "				else\n" );
								}
fprintf( f, "					pDst[x] = data;\n" );
								if( buf_type == BUF_PRI )
								{
fprintf( f, "				}\n" );
fprintf( f, "				((UINT8 *)pBuf)[x] = (((UINT8 *)pBuf)[x] & 0x7f) | 31;\n" );
								}
fprintf( f, "				break;\n" );
fprintf( f, "			case DRAWMODE_SHADOW:\n" );
								if( buf_type == BUF_PRI )
								{
fprintf( f, "				if (((1 << ((UINT8 *)pBuf)[x]) & pri_code) == 0)\n" );
								}
fprintf( f, "					pDst[x] = SHADOW%d(pDst[x]);\n", bpp );
								if( buf_type == BUF_PRI )
								{
fprintf( f, "				((UINT8 *)pBuf)[x] |= pdrawgfx_shadow_lowpri ? 0 : 0x80;\n" );
								}
fprintf( f, "				break;\n" );
fprintf( f, "			}\n" );
							}
							else
							{
								switch( buf_type )
								{
								case BUF_NONE:
									break;
								case BUF_PRI:
fprintf( f, "			if ((1 << (((UINT8 *)pBuf)[x]) & pri_code) == 0){\n" );
									break;
								case BUF_Z:
fprintf( f, "			if( pri_code>=((UINT8 *)pBuf)[x]){\n" );
fprintf( f, "				((UINT8 *)pBuf)[x] = pri_code;\n" );
									break;
								}
								switch( transparency_mode )
								{
								case TRANSPARENCY_BLEND_RAW:
fprintf( f, "				pDst[x] |= (data+theColor);\n" );
									break;

								case TRANSPARENCY_ALPHAONE:
fprintf( f, "				if( data != alpha_pen )\n" );
fprintf( f, "				{\n" );
fprintf( f, "					pDst[x] = pPal[data];\n" );
fprintf( f, "				}\n" );
fprintf( f, "				else\n" );
fprintf( f, "				{\n" );
									if( bpp==8 )
									{
fprintf( f, "					pDst[x] = pPal[data]; /* no blend in 8bpp */\n" );
									}
									else
									{
fprintf( f, "					pDst[x] = alpha_blend%d(pDst[x], pPal[data]);\n",bpp );
									}
fprintf( f, "				}\n" );
									break;

								case TRANSPARENCY_ALPHARANGE:
fprintf( f, "				if( gfx_alpharange_table[data]==0xff )\n" );
fprintf( f, "				{\n" );
fprintf( f, "					pDst[x] = pPal[data];\n" );
fprintf( f, "				}\n" );
fprintf( f, "				else\n" );
fprintf( f, "				{\n" );
									if( bpp==8 )
									{
fprintf( f, "					pDst[x] = pPal[data]; /* no blend in 8bpp */\n" );
									}
									else
									{
fprintf( f, "					pDst[x] = alpha_blend_r%d(pDst[x], pPal[data], gfx_alpharange_table[data] );\n",bpp );
									}
fprintf( f, "				}\n" );
									break;

								case TRANSPARENCY_ALPHA:
									if( bpp==8 )
									{
fprintf( f, "				pDst[x] = data;\n" );
									}
									else
									{
fprintf( f, "pDst[x] = alpha_blend%d(pDst[x], pPal[data]);\n", bpp );
									}
									break;

								default:
fprintf( f, "				pDst[x] = data;\n" );
									break;
								}

								switch( buf_type )
								{
								case BUF_NONE:
									break;
								case BUF_PRI:
fprintf( f, "			}\n" );
fprintf( f, "			((UINT8 *)pBuf)[x] = 31;\n" );
									break;
								case BUF_Z:
fprintf( f, "			}\n" );
									break;
								}
							}

fprintf( f, "		}\n" );
fprintf( f, "		x_index += dx;\n" );
fprintf( f, "	}\n" );
fprintf( f, "}\n" );
						}
					}
				}
			}
		}

fprintf( f, "typedef void blitter( void *pDst, const void *pSrc, const pen_t *pPal, int transp, void *pBuf, int sx, int ex, int x_index, int dx, int pri_code );\n" );
fprintf( f, "static blitter *mBlitTable[] ={\n" );
		for( i=0; i<2; i++ )
		{
			int transparency_mode;
			for( transparency_mode=0; transparency_mode<TRANSPARENCY_MODES; transparency_mode++ )
			{
				int source_type;
				for( source_type=0; source_type<SOURCE_TYPES; source_type++ )
				{
					int dest_type;
					for( dest_type=0; dest_type<DEST_TYPES; dest_type++ )
					{
						int buf_type;
						for( buf_type=0; buf_type<BUF_TYPES; buf_type++ )
						{
							if( i )
							{
fprintf( f, "(blitter *)blitzoom" );
							}
							else
							{
fprintf( f, "(blitter *)blit" );
							}

fprintf( f, "_%s_%s_%s_%s,\n",
								transparency_name[transparency_mode],
								source_name[source_type],
								dest_name[dest_type],
								buf_name[buf_type] );
						}
					}
				}
			}
		}
fprintf( f, "};\n" );
	}
	fclose( f );
}
