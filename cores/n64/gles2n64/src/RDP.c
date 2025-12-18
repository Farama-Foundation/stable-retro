#include <stdint.h>
#include <string.h>

#include <retro_inline.h>

#include "N64.h"
#include "RSP.h"
#include "GBI.h"
#include "gDP.h"
#include "Debug.h"
#include "Common.h"
#include "gSP.h"
#include "RDP.h"

#include "../../Graphics/RSP/gSP_state.h"
#include "../../Graphics/RDP/RDP_state.h"

enum
{
   gspTexRect = 0,
   gdpTexRect,
   halfTexRect
};

void RDP_Unknown( uint32_t w0, uint32_t w1 )
{
}

void RDP_NoOp( uint32_t w0, uint32_t w1 )
{
    gln64gSPNoOp();
}

void RDP_SetCImg( uint32_t w0, uint32_t w1 )
{
    gln64gDPSetColorImage( _SHIFTR( w0, 21,  3 ),        // fmt
                      _SHIFTR( w0, 19,  2 ),        // siz
                      _SHIFTR( w0,  0, 12 ) + 1,    // width
                      w1 );                         // img
}

void RDP_SetZImg( uint32_t w0, uint32_t w1 )
{
    gln64gDPSetDepthImage( w1 ); // img
}

void RDP_SetTImg( uint32_t w0, uint32_t w1 )
{
    gln64gDPSetTextureImage( _SHIFTR( w0, 21,  3),       // fmt
                        _SHIFTR( w0, 19,  2 ),      // siz
                        _SHIFTR( w0,  0, 12 ) + 1,  // width
                        w1 );                       // img
}

void RDP_SetCombine( uint32_t w0, uint32_t w1 )
{
    gln64gDPSetCombine( _SHIFTR( w0, 0, 24 ),    // muxs0
                   w1 );                    // muxs1
}

void RDP_SetEnvColor( uint32_t w0, uint32_t w1 )
{
    gln64gDPSetEnvColor( _SHIFTR( w1, 24, 8 ),       // r
                    _SHIFTR( w1, 16, 8 ),       // g
                    _SHIFTR( w1,  8, 8 ),       // b
                    _SHIFTR( w1,  0, 8 ) );     // a
}

void RDP_SetPrimColor( uint32_t w0, uint32_t w1 )
{
    gln64gDPSetPrimColor( _SHIFTL( w0,  8, 5 ),      // m
                     _SHIFTL( w0,  0, 8 ),      // l
                     _SHIFTR( w1, 24, 8 ),      // r
                     _SHIFTR( w1, 16, 8 ),      // g
                     _SHIFTR( w1,  8, 8 ),      // b
                     _SHIFTR( w1,  0, 8 ) );    // a

}

void RDP_SetBlendColor( uint32_t w0, uint32_t w1 )
{
    gln64gDPSetBlendColor( _SHIFTR( w1, 24, 8 ),     // r
                      _SHIFTR( w1, 16, 8 ),     // g
                      _SHIFTR( w1,  8, 8 ),     // b
                      _SHIFTR( w1,  0, 8 ) );   // a
}

void RDP_SetFogColor( uint32_t w0, uint32_t w1 )
{
    gln64gDPSetFogColor( _SHIFTR( w1, 24, 8 ),       // r
                    _SHIFTR( w1, 16, 8 ),       // g
                    _SHIFTR( w1,  8, 8 ),       // b
                    _SHIFTR( w1,  0, 8 ) );     // a
}

void RDP_SetFillColor( uint32_t w0, uint32_t w1 )
{
    gln64gDPSetFillColor( w1 );
}

void RDP_FillRect( uint32_t w0, uint32_t w1 )
{
	const uint32_t ulx = _SHIFTR(w1, 14, 10);
	const uint32_t uly = _SHIFTR(w1, 2, 10);
	const uint32_t lrx = _SHIFTR(w0, 14, 10);
	const uint32_t lry = _SHIFTR(w0, 2, 10);
	if (lrx < ulx || lry < uly)
		return;
	gln64gDPFillRectangle(ulx, uly, lrx, lry);
}

void RDP_SetTile( uint32_t w0, uint32_t w1 )
{

    gln64gDPSetTile( _SHIFTR( w0, 21, 3 ),   // fmt
                _SHIFTR( w0, 19, 2 ),   // siz
                _SHIFTR( w0,  9, 9 ),   // line
                _SHIFTR( w0,  0, 9 ),   // tmem
                _SHIFTR( w1, 24, 3 ),   // tile
                _SHIFTR( w1, 20, 4 ),   // palette
                _SHIFTR( w1, 18, 2 ),   // cmt
                _SHIFTR( w1,  8, 2 ),   // cms
                _SHIFTR( w1, 14, 4 ),   // maskt
                _SHIFTR( w1,  4, 4 ),   // masks
                _SHIFTR( w1, 10, 4 ),   // shiftt
                _SHIFTR( w1,  0, 4 ) ); // shifts
}

void RDP_LoadTile( uint32_t w0, uint32_t w1 )
{
    gln64gDPLoadTile( _SHIFTR( w1, 24,  3 ),     // tile
                 _SHIFTR( w0, 12, 12 ),     // uls
                 _SHIFTR( w0,  0, 12 ),     // ult
                 _SHIFTR( w1, 12, 12 ),     // lrs
                 _SHIFTR( w1,  0, 12 ) );   // lrt
}

static uint32_t lbw0, lbw1;
void RDP_LoadBlock( uint32_t w0, uint32_t w1 )
{
	lbw0 = w0;
	lbw1 = w1;

   gln64gDPLoadBlock( _SHIFTR( w1, 24,  3 ),    // tile
                  _SHIFTR( w0, 12, 12 ),    // uls
                  _SHIFTR( w0,  0, 12 ),    // ult
                  _SHIFTR( w1, 12, 12 ),    // lrs
                  _SHIFTR( w1,  0, 12 ) );  // dxt
}

void RDP_RepeatLastLoadBlock()
{
	RDP_LoadBlock(lbw0, lbw1);
}

void RDP_SetTileSize( uint32_t w0, uint32_t w1 )
{
   gln64gDPSetTileSize( _SHIFTR( w1, 24,  3 ),      // tile
                    _SHIFTR( w0, 12, 12 ),      // uls
                    _SHIFTR( w0,  0, 12 ),      // ult
                    _SHIFTR( w1, 12, 12 ),      // lrs
                    _SHIFTR( w1,  0, 12 ) );    // lrt
}

void RDP_LoadTLUT( uint32_t w0, uint32_t w1 )
{
    gln64gDPLoadTLUT(  _SHIFTR( w1, 24,  3 ),    // tile
                  _SHIFTR( w0, 12, 12 ),    // uls
                  _SHIFTR( w0,  0, 12 ),    // ult
                  _SHIFTR( w1, 12, 12 ),    // lrs
                  _SHIFTR( w1,  0, 12 ) );  // lrt
}

void RDP_SetOtherMode( uint32_t w0, uint32_t w1 )
{
    gln64gDPSetOtherMode( _SHIFTR( w0, 0, 24 ),  // mode0
                     w1 );                  // mode1
}

void RDP_SetPrimDepth( uint32_t w0, uint32_t w1 )
{
    gln64gDPSetPrimDepth( _SHIFTR( w1, 16, 16 ),     // z
                     _SHIFTR( w1,  0, 16 ) );   // dz
}

void RDP_SetScissor( uint32_t w0, uint32_t w1 )
{
    gln64gDPSetScissor( _SHIFTR( w1, 24, 2 ),                        // mode
                   _FIXED2FLOAT( _SHIFTR( w0, 12, 12 ), 2 ),    // ulx
                   _FIXED2FLOAT( _SHIFTR( w0,  0, 12 ), 2 ),    // uly
                   _FIXED2FLOAT( _SHIFTR( w1, 12, 12 ), 2 ),    // lrx
                   _FIXED2FLOAT( _SHIFTR( w1,  0, 12 ), 2 ) );  // lry
}

void RDP_SetConvert( uint32_t w0, uint32_t w1 )
{
    gln64gDPSetConvert( _SHIFTR( w0, 13, 9 ),    // k0
                   _SHIFTR( w0,  4, 9 ),    // k1
                   _SHIFTL( w0,  5, 4 ) | _SHIFTR( w1, 27, 5 ), // k2
                   _SHIFTR( w1, 18, 9 ),    // k3
                   _SHIFTR( w1,  9, 9 ),    // k4
                   _SHIFTR( w1,  0, 9 ) );  // k5
}

void RDP_SetKeyR( uint32_t w0, uint32_t w1 )
{
    gln64gDPSetKeyR( _SHIFTR( w1,  8,  8 ),      // cR
                _SHIFTR( w1,  0,  8 ),      // sR
                _SHIFTR( w1, 16, 12 ) );    // wR
}

void RDP_SetKeyGB( uint32_t w0, uint32_t w1 )
{
    gln64gDPSetKeyGB( _SHIFTR( w1, 24,  8 ),     // cG
                 _SHIFTR( w1, 16,  8 ),     // sG
                 _SHIFTR( w0, 12, 12 ),     // wG
                 _SHIFTR( w1,  8,  8 ),     // cB
                 _SHIFTR( w1,  0,  8 ),     // SB
                 _SHIFTR( w0,  0, 12 ) );   // wB
}

void RDP_FullSync( uint32_t w0, uint32_t w1 )
{
   gln64gDPFullSync();
}

void RDP_TileSync( uint32_t w0, uint32_t w1 )
{
   gln64gDPTileSync();
}

void RDP_PipeSync( uint32_t w0, uint32_t w1 )
{
   gln64gDPPipeSync();
}

void RDP_LoadSync( uint32_t w0, uint32_t w1 )
{
   gln64gDPLoadSync();
}


static void _getTexRectParams(uint32_t *w2, uint32_t *w3)
{
   unsigned texRectMode;
   uint32_t cmd1, cmd2;

   if (__RSP.bLLE)
   {
      *w2 = __RDP.w2;
      *w3 = __RDP.w3;
      return;
   }

   texRectMode = gdpTexRect;
   cmd1        = (*(uint32_t*)&gfx_info.RDRAM[__RSP.PC[__RSP.PCi] + 0]) >> 24;
   cmd2        = (*(uint32_t*)&gfx_info.RDRAM[__RSP.PC[__RSP.PCi] + 8]) >> 24;

   if (cmd1 == G_RDPHALF_1)
   {
      if (cmd2 == G_RDPHALF_2)
         texRectMode = gspTexRect;
   }
   else if (cmd1 == 0xB3)
   {
      if (cmd2 == 0xB2)
         texRectMode = gspTexRect;
      else
         texRectMode = halfTexRect;
   }
   else if (cmd1 == 0xF1)
      texRectMode = halfTexRect;

   switch (texRectMode)
   {
      case gspTexRect:
         *w2 = *(uint32_t*)&gfx_info.RDRAM[__RSP.PC[__RSP.PCi] + 4];
         __RSP.PC[__RSP.PCi] += 8;

         *w3 = *(uint32_t*)&gfx_info.RDRAM[__RSP.PC[__RSP.PCi] + 4];
         __RSP.PC[__RSP.PCi] += 8;
         break;
      case gdpTexRect:
         *w2 = *(uint32_t*)&gfx_info.RDRAM[__RSP.PC[__RSP.PCi] + 0];
         *w3 = *(uint32_t*)&gfx_info.RDRAM[__RSP.PC[__RSP.PCi] + 4];
         __RSP.PC[__RSP.PCi] += 8;
         break;
      case halfTexRect:
         *w2 = 0;
         *w3 = *(uint32_t*)&gfx_info.RDRAM[__RSP.PC[__RSP.PCi] + 4];
         __RSP.PC[__RSP.PCi] += 8;
         break;
      default:
#if 0
         assert(false && "Unknown texrect mode");
#endif
	break;
   }
}

void RDP_TexRectFlip( uint32_t w0, uint32_t w1 )
{
   uint32_t ulx, uly, lrx, lry;
	uint32_t w2, w3;
	_getTexRectParams(&w2, &w3);
	ulx = _SHIFTR(w1, 12, 12);
	uly = _SHIFTR(w1, 0, 12);
	lrx = _SHIFTR(w0, 12, 12);
	lry = _SHIFTR(w0, 0, 12);
	if ((lrx >> 2) < (ulx >> 2) || (lry >> 2) < (uly >> 2))
		return;

	gln64gDPTextureRectangleFlip(
		_FIXED2FLOAT(ulx, 2),
		_FIXED2FLOAT(uly, 2),
		_FIXED2FLOAT(lrx, 2),
		_FIXED2FLOAT(lry, 2),
		_SHIFTR(w1, 24, 3),							// tile
		_FIXED2FLOAT((int16_t)_SHIFTR(w2, 16, 16), 5),	// s
		_FIXED2FLOAT((int16_t)_SHIFTR(w2, 0, 16), 5),	// t
		_FIXED2FLOAT((int16_t)_SHIFTR(w3, 16, 16), 10),	// dsdx
		_FIXED2FLOAT((int16_t)_SHIFTR(w3, 0, 16), 10));	// dsdy
}

void RDP_TexRect( uint32_t w0, uint32_t w1 )
{
   uint32_t ulx, uly, lrx, lry;
	uint32_t w2, w3;
	_getTexRectParams(&w2, &w3);
	ulx = _SHIFTR(w1, 12, 12);
	uly = _SHIFTR(w1,  0, 12);
	lrx = _SHIFTR(w0, 12, 12);
	lry = _SHIFTR(w0,  0, 12);
	if ((lrx >> 2) < (ulx >> 2) || (lry >> 2) < (uly >> 2))
		return;

	gln64gDPTextureRectangle(
		_FIXED2FLOAT(ulx, 2),
		_FIXED2FLOAT(uly, 2),
		_FIXED2FLOAT(lrx, 2),
		_FIXED2FLOAT(lry, 2),
		_SHIFTR(w1, 24, 3),							// tile
		_FIXED2FLOAT((int16_t)_SHIFTR(w2, 16, 16), 5),	// s
		_FIXED2FLOAT((int16_t)_SHIFTR(w2, 0, 16), 5),	// t
		_FIXED2FLOAT((int16_t)_SHIFTR(w3, 16, 16), 10),	// dsdx
		_FIXED2FLOAT((int16_t)_SHIFTR(w3, 0, 16), 10));	// dsdy
}


//Low Level RDP Drawing Commands:
void RDP_TriFill(uint32_t _w0, uint32_t _w1)
{
	gln64gDPTriFill(_w0, _w1);
}

void RDP_TriTxtr(uint32_t _w0, uint32_t _w1)
{
	gln64gDPTriTxtr(_w0, _w1);
}

void RDP_TriTxtrZBuff(uint32_t w0, uint32_t w1)
{
    LOG(LOG_VERBOSE, "RSP_TRI_TXTR_ZBUFF Command\n");
}

void RDP_TriShade(uint32_t _w0, uint32_t _w1)
{
	gln64gDPTriShadeZ(_w0, _w1);
}

void RDP_TriShadeZBuff(uint32_t w0, uint32_t w1)
{
    LOG(LOG_VERBOSE, "RSP_TRI_SHADE_ZBUFF Command\n");
}

void RDP_TriShadeTxtr(uint32_t _w0, uint32_t _w1)
{
	gln64gDPTriShadeTxtr(_w0, _w1);
}

void RDP_TriFillZ( uint32_t _w0, uint32_t _w1 )
{
	gln64gDPTriFillZ(_w0, _w1);
}

void RDP_TriShadeTxtrZBuff(uint32_t _w0, uint32_t _w1)
{
	gln64gDPTriShadeTxtrZ(_w0, _w1);
}

void RDP_TriShadeZ( uint32_t _w0, uint32_t _w1 )
{
	gln64gDPTriShadeZ(_w0, _w1);
}

void RDP_TriTxtrZ( uint32_t _w0, uint32_t _w1 )
{
	gln64gDPTriTxtrZ(_w0, _w1);
}

void RDP_TriShadeTxtrZ( uint32_t _w0, uint32_t _w1 )
{
	gln64gDPTriShadeTxtrZ(_w0, _w1);
}

void RDP_Init(void)
{
   int i;
   // Initialize RDP commands to RDP_UNKNOWN
   for (i = 0xC8; i <= 0xCF; i++)
      GBI.cmd[i] = RDP_Unknown;

   // Initialize RDP commands to RDP_UNKNOWN
   for (i = 0xE4; i <= 0xFF; i++)
      GBI.cmd[i] = RDP_Unknown;

   // Set known GBI commands
   GBI.cmd[G_NOOP]             = RDP_NoOp;
   GBI.cmd[G_SETCIMG]          = RDP_SetCImg;
   GBI.cmd[G_SETZIMG]          = RDP_SetZImg;
   GBI.cmd[G_SETTIMG]          = RDP_SetTImg;
   GBI.cmd[G_SETCOMBINE]       = RDP_SetCombine;
   GBI.cmd[G_SETENVCOLOR]      = RDP_SetEnvColor;
   GBI.cmd[G_SETPRIMCOLOR]     = RDP_SetPrimColor;
   GBI.cmd[G_SETBLENDCOLOR]    = RDP_SetBlendColor;
   GBI.cmd[G_SETFOGCOLOR]      = RDP_SetFogColor;
   GBI.cmd[G_SETFILLCOLOR]     = RDP_SetFillColor;
   GBI.cmd[G_FILLRECT]         = RDP_FillRect;
   GBI.cmd[G_SETTILE]          = RDP_SetTile;
   GBI.cmd[G_LOADTILE]         = RDP_LoadTile;
   GBI.cmd[G_LOADBLOCK]        = RDP_LoadBlock;
   GBI.cmd[G_SETTILESIZE]      = RDP_SetTileSize;
   GBI.cmd[G_LOADTLUT]         = RDP_LoadTLUT;
   GBI.cmd[G_RDPSETOTHERMODE]  = RDP_SetOtherMode;
   GBI.cmd[G_SETPRIMDEPTH]     = RDP_SetPrimDepth;
   GBI.cmd[G_SETSCISSOR]       = RDP_SetScissor;
   GBI.cmd[G_SETCONVERT]       = RDP_SetConvert;
   GBI.cmd[G_SETKEYR]          = RDP_SetKeyR;
   GBI.cmd[G_SETKEYGB]         = RDP_SetKeyGB;
   GBI.cmd[G_RDPFULLSYNC]      = RDP_FullSync;
   GBI.cmd[G_RDPTILESYNC]      = RDP_TileSync;
   GBI.cmd[G_RDPPIPESYNC]      = RDP_PipeSync;
   GBI.cmd[G_RDPLOADSYNC]      = RDP_LoadSync;
   GBI.cmd[G_TEXRECTFLIP]      = RDP_TexRectFlip;
   GBI.cmd[G_TEXRECT]          = RDP_TexRect;

   __RDP.w2 = __RDP.w3 = 0;
   __RDP.cmd_ptr = __RDP.cmd_cur = 0;
}

static
GBIFunc LLEcmd[64] = {
	/* 0x00 */
	RDP_NoOp,			RDP_Unknown,		RDP_Unknown,		RDP_Unknown,
	RDP_Unknown,		RDP_Unknown,		RDP_Unknown,		RDP_Unknown,
	RDP_TriFill,		RDP_TriFillZ,		RDP_TriTxtr,		RDP_TriTxtrZ,
	RDP_TriShade,		RDP_TriShadeZ,		RDP_TriShadeTxtr,	RDP_TriShadeTxtrZ,
	/* 0x10 */
	RDP_Unknown,		RDP_Unknown,		RDP_Unknown,		RDP_Unknown,
	RDP_Unknown,		RDP_Unknown,		RDP_Unknown,		RDP_Unknown,
	RDP_Unknown,		RDP_Unknown,		RDP_Unknown,		RDP_Unknown,
	RDP_Unknown,		RDP_Unknown,		RDP_Unknown,		RDP_Unknown,
	/* 0x20 */
	RDP_Unknown,		RDP_Unknown,		RDP_Unknown,		RDP_Unknown,
	RDP_TexRect,		RDP_TexRectFlip,	RDP_LoadSync,		RDP_PipeSync,
	RDP_TileSync,		RDP_FullSync,		RDP_SetKeyGB,		RDP_SetKeyR,
	RDP_SetConvert,		RDP_SetScissor,		RDP_SetPrimDepth,	RDP_SetOtherMode,
	/* 0x30 */
	RDP_LoadTLUT,		RDP_Unknown,		RDP_SetTileSize,	RDP_LoadBlock,
	RDP_LoadTile,		RDP_SetTile,		RDP_FillRect,		RDP_SetFillColor,
	RDP_SetFogColor,	RDP_SetBlendColor,	RDP_SetPrimColor,	RDP_SetEnvColor,
	RDP_SetCombine,		RDP_SetTImg,		RDP_SetZImg,		RDP_SetCImg
};

static
const uint32_t CmdLength[64] =
{
	8,                      // 0x00, No Op
	8,                      // 0x01, ???
	8,                      // 0x02, ???
	8,                      // 0x03, ???
	8,                      // 0x04, ???
	8,                      // 0x05, ???
	8,                      // 0x06, ???
	8,                      // 0x07, ???
	32,                     // 0x08, Non-Shaded Triangle
	32+16,          // 0x09, Non-Shaded, Z-Buffered Triangle
	32+64,          // 0x0a, Textured Triangle
	32+64+16,       // 0x0b, Textured, Z-Buffered Triangle
	32+64,          // 0x0c, Shaded Triangle
	32+64+16,       // 0x0d, Shaded, Z-Buffered Triangle
	32+64+64,       // 0x0e, Shaded+Textured Triangle
	32+64+64+16,// 0x0f, Shaded+Textured, Z-Buffered Triangle
	8,                      // 0x10, ???
	8,                      // 0x11, ???
	8,                      // 0x12, ???
	8,                      // 0x13, ???
	8,                      // 0x14, ???
	8,                      // 0x15, ???
	8,                      // 0x16, ???
	8,                      // 0x17, ???
	8,                      // 0x18, ???
	8,                      // 0x19, ???
	8,                      // 0x1a, ???
	8,                      // 0x1b, ???
	8,                      // 0x1c, ???
	8,                      // 0x1d, ???
	8,                      // 0x1e, ???
	8,                      // 0x1f, ???
	8,                      // 0x20, ???
	8,                      // 0x21, ???
	8,                      // 0x22, ???
	8,                      // 0x23, ???
	16,                     // 0x24, Texture_Rectangle
	16,                     // 0x25, Texture_Rectangle_Flip
	8,                      // 0x26, Sync_Load
	8,                      // 0x27, Sync_Pipe
	8,                      // 0x28, Sync_Tile
	8,                      // 0x29, Sync_Full
	8,                      // 0x2a, Set_Key_GB
	8,                      // 0x2b, Set_Key_R
	8,                      // 0x2c, Set_Convert
	8,                      // 0x2d, Set_Scissor
	8,                      // 0x2e, Set_Prim_Depth
	8,                      // 0x2f, Set_Other_Modes
	8,                      // 0x30, Load_TLUT
	8,                      // 0x31, ???
	8,                      // 0x32, Set_Tile_Size
	8,                      // 0x33, Load_Block
	8,                      // 0x34, Load_Tile
	8,                      // 0x35, Set_Tile
	8,                      // 0x36, Fill_Rectangle
	8,                      // 0x37, Set_Fill_Color
	8,                      // 0x38, Set_Fog_Color
	8,                      // 0x39, Set_Blend_Color
	8,                      // 0x3a, Set_Prim_Color
	8,                      // 0x3b, Set_Env_Color
	8,                      // 0x3c, Set_Combine
	8,                      // 0x3d, Set_Texture_Image
	8,                      // 0x3e, Set_Mask_Image
	8                       // 0x3f, Set_Color_Image
};

void RDP_Half_1( uint32_t _c )
{
   if (RDP_Half1(_c))
      LLEcmd[__RSP.cmd](__RSP.w0, __RSP.w1);
}

static INLINE uint32_t GLN64_READ_RDP_DATA(uint32_t address)
{
	if ((*(uint32_t*)gfx_info.DPC_STATUS_REG) & 0x1)          // XBUS_DMEM_DMA enabled
		return gfx_info.DMEM[(address & 0xfff)>>2];
   return gfx_info.RDRAM[address>>2];
}

void gln64ProcessRDPList(void)
{
   uint32_t i;
   bool set_zero = true;
   const uint32_t length = gfx_info.DPC_END_REG - gfx_info.DPC_CURRENT_REG;

   (*(uint32_t*)gfx_info.DPC_STATUS_REG) &= ~0x0002;

   if (gfx_info.DPC_END_REG <= gfx_info.DPC_CURRENT_REG)
      return;

   __RSP.bLLE = true;

   /* load command data */
   for (i = 0; i < length; i += 4)
   {
      __RDP.cmd_data[__RDP.cmd_ptr] = rdp_read_data(*gfx_info.DPC_CURRENT_REG + i);
      __RDP.cmd_ptr = (__RDP.cmd_ptr + 1) & maxCMDMask;
   }

   while (__RDP.cmd_cur != __RDP.cmd_ptr)
   {
      uint32_t w0, w1;
      uint32_t cmd = (__RDP.cmd_data[__RDP.cmd_cur] >> 24) & 0x3f;

      if ((((__RDP.cmd_ptr - __RDP.cmd_cur)&maxCMDMask) * 4) < CmdLength[cmd])
      {
         set_zero = false;
         break;
      }

      if (__RDP.cmd_cur + CmdLength[cmd] / 4 > MAXCMD)
         memcpy(__RDP.cmd_data + MAXCMD, __RDP.cmd_data, CmdLength[cmd] - (MAXCMD - __RDP.cmd_cur) * 4);

      // execute the command
      w0 = __RDP.cmd_data[__RDP.cmd_cur+0];
      w1 = __RDP.cmd_data[__RDP.cmd_cur+1];
      __RDP.w2 = __RDP.cmd_data[__RDP.cmd_cur+2];
      __RDP.w3 = __RDP.cmd_data[__RDP.cmd_cur + 3];
      __RSP.cmd = cmd;
      LLEcmd[cmd](w0, w1);

      __RDP.cmd_cur = (__RDP.cmd_cur + CmdLength[cmd] / 4) & maxCMDMask;
   }

   if (set_zero)
   {
      __RDP.cmd_ptr = 0;
      __RDP.cmd_cur = 0;
   }

   __RSP.bLLE = false;
   gSP.changed |= CHANGED_COLORBUFFER;

   gfx_info.DPC_START_REG = gfx_info.DPC_CURRENT_REG = gfx_info.DPC_END_REG;
}
