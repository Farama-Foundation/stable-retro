#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <retro_miscellaneous.h>

#include "Common.h"
#include "gles2N64.h"
#include "N64.h"
#include "GBI.h"
#include "RSP.h"
#include "RDP.h"
#include "gDP.h"
#include "gSP.h"
#include "Debug.h"
#include "convert.h"
#include "OpenGL.h"
#include "CRC.h"
#include "FrameBuffer.h"
#include "DepthBuffer.h"
#include "VI.h"
#include "Config.h"
#include "ShaderCombiner.h"

#include "../../Graphics/RDP/gDP_state.h"
#include "../../Graphics/RDP/RDP_state.h"
#include "../../Graphics/RSP/gSP_state.h"

void gln64gDPSetOtherMode( uint32_t mode0, uint32_t mode1 )
{
   gDP.otherMode.h = mode0;
   gDP.otherMode.l = mode1;

   gDP.changed |= CHANGED_RENDERMODE | CHANGED_CYCLETYPE;
}

void gln64gDPSetPrimDepth( uint16_t z, uint16_t dz )
{
	if (gSP.viewport.vscale[2] == 0)
		gDP.primDepth.z = _FIXED2FLOAT(_SHIFTR(z, 0, 15), 15);
	else
		gDP.primDepth.z = MIN(1.0f, MAX(-1.0f, (_FIXED2FLOAT(_SHIFTR(z, 0, 15), 15) - gSP.viewport.vtrans[2]) / gSP.viewport.vscale[2]));
	gDP.primDepth.deltaZ = _FIXED2FLOAT(_SHIFTR(dz, 0, 15), 15);
}

void gln64gDPPipelineMode( uint32_t mode )
{
	gDP.otherMode.pipelineMode = mode;
}

void gln64gDPSetTexturePersp( uint32_t enable )
{
   gDP.otherMode.texturePersp = enable & 1;
}

void gln64gDPSetCycleType( uint32_t type )
{
   gDP.otherMode.cycleType = type;

   gDP.changed |= CHANGED_CYCLETYPE;
}

void gln64gDPSetTextureDetail( uint32_t type )
{
	gDP.otherMode.textureDetail = type;
}

void gln64gDPSetTextureLOD( uint32_t mode )
{
	gDP.otherMode.textureLOD = mode;
}

void gln64gDPSetTextureLUT( uint32_t mode )
{
   gDP.otherMode.textureLUT = mode & 3;
}

void gln64gDPSetTextureFilter( uint32_t type )
{
	gDP.otherMode.textureFilter = type;
}

void gln64gDPSetTextureConvert( uint32_t type )
{
	gDP.otherMode.textureConvert = type;
}

void gln64gDPSetCombineKey( uint32_t type )
{
   gDP.otherMode.combineKey = type;
}

void gln64gDPSetColorDither( uint32_t type )
{
	gDP.otherMode.colorDither = type;
}

void gln64gDPSetAlphaDither( uint32_t type )
{
	gDP.otherMode.alphaDither = type;
}

void gln64gDPSetAlphaCompare( uint32_t mode )
{
	gDP.otherMode.alphaCompare = mode;

	gDP.changed |= CHANGED_ALPHACOMPARE;
}

void gln64gDPSetDepthSource( uint32_t source )
{
	gDP.otherMode.depthSource = source;
}

void gln64gDPSetRenderMode( uint32_t mode1, uint32_t mode2 )
{
	gDP.otherMode.l &= 0x00000007;
	gDP.otherMode.l |= mode1 | mode2;

	gDP.changed |= CHANGED_RENDERMODE;
}

void gln64gDPSetCombine( int32_t muxs0, int32_t muxs1 )
{
   gDP.combine.muxs0 = muxs0;
   gDP.combine.muxs1 = muxs1;

   gDP.changed |= CHANGED_COMBINE;
}

void gln64gDPUpdateColorImage(void)
{
}

void gln64gDPSetColorImage( uint32_t format, uint32_t size, uint32_t width, uint32_t address )
{
   uint32_t addr = RSP_SegmentToPhysical( address );

	if (gDP.colorImage.address != address || gDP.colorImage.width != width || gDP.colorImage.size != size)
   {
		uint32_t height = 1;

		if (width == VI.width)
			height = VI.height;
#if 0
		else if (!__RSP.bLLE && width == gDP.scissor.lrx && width == gSP.viewport.width)
#else
		else if (width == gDP.scissor.lrx && width == gSP.viewport.width)
#endif
      {
			height = MAX(gDP.scissor.lry, gSP.viewport.height);
			height = MIN(height, VI.height);
		} else if (width == gDP.scissor.lrx)
			height = gDP.scissor.lry;
		else if (width <= 64)
			height = width;
		else if (gSP.viewport.height > 0)
			height = gSP.viewport.height;
#if 0
		else if (!__RSP.bLLE && gSP.viewport.height > 0)
			height = gSP.viewport.height;
#endif
		else
			height = gDP.scissor.lry;

		if (config.frameBufferEmulation.enable)
      {
         FrameBuffer_SaveBuffer(address, (uint16_t)format, (uint16_t)size, (uint16_t)width, height, false);
         gDP.colorImage.height = 0;
      }
      else
         gDP.colorImage.height = height;
   }

   gDP.colorImage.format  = format;
   gDP.colorImage.size    = size;
   gDP.colorImage.width   = width;
   gDP.colorImage.address = RSP_SegmentToPhysical(addr);
}

void gln64gDPSetTextureImage( uint32_t format, uint32_t size, uint32_t width, uint32_t address )
{
   gDP.textureImage.format  = format;
   gDP.textureImage.size    = size;
   gDP.textureImage.width   = width;
   gDP.textureImage.address = RSP_SegmentToPhysical( address );
   gDP.textureImage.bpl     = gDP.textureImage.width << gDP.textureImage.size >> 1;

	if (gSP.DMAOffsets.tex_offset != 0)
   {
      if (format == G_IM_FMT_RGBA)
      {
         uint16_t * t = (uint16_t*)(gfx_info.RDRAM + gSP.DMAOffsets.tex_offset);
         gSP.DMAOffsets.tex_shift = t[gSP.DMAOffsets.tex_count ^ 1];
         gDP.textureImage.address += gSP.DMAOffsets.tex_shift;
      }
      else
      {
         gSP.DMAOffsets.tex_offset = 0;
         gSP.DMAOffsets.tex_shift = 0;
         gSP.DMAOffsets.tex_count = 0;
      }
   }
}

/* TODO/FIXME - update */
void gln64gDPSetDepthImage( uint32_t address )
{
#if 0
	depthBufferList().saveBuffer(address);
#else
   DepthBuffer_SetBuffer(RSP_SegmentToPhysical(address));

   if (depthBuffer.current->cleared)
      OGL_ClearDepthBuffer(false);
#endif
   gDP.depthImageAddress = RSP_SegmentToPhysical(address);
}

void gln64gDPSetEnvColor( uint32_t r, uint32_t g, uint32_t b, uint32_t a )
{
   gDP.envColor.r = r * 0.0039215689f;
   gDP.envColor.g = g * 0.0039215689f;
   gDP.envColor.b = b * 0.0039215689f;
   gDP.envColor.a = a * 0.0039215689f;

   gDP.changed |= CHANGED_COMBINE_COLORS;

	ShaderCombiner_UpdateEnvColor();
}

void gln64gDPSetBlendColor( uint32_t r, uint32_t g, uint32_t b, uint32_t a )
{
   gDP.blendColor.r = r * 0.0039215689f;
   gDP.blendColor.g = g * 0.0039215689f;
   gDP.blendColor.b = b * 0.0039215689f;
   gDP.blendColor.a = a * 0.0039215689f;

   ShaderCombiner_UpdateBlendColor();
}

void gln64gDPSetFogColor( uint32_t r, uint32_t g, uint32_t b, uint32_t a )
{
   gDP.fogColor.r = r * 0.0039215689f;
   gDP.fogColor.g = g * 0.0039215689f;
   gDP.fogColor.b = b * 0.0039215689f;
   gDP.fogColor.a = a * 0.0039215689f;

   ShaderCombiner_UpdateFogColor();

	gDP.changed |= CHANGED_FOGCOLOR;
}

void gln64gDPSetFillColor( uint32_t c )
{
	gDP.fillColor.color = c;
   gDP.fillColor.z     = (float)_SHIFTR( c,  2, 14 );
   gDP.fillColor.dz    = (float)_SHIFTR( c,  0,  2 );
}

void gln64gDPSetPrimColor( uint32_t m, uint32_t l, uint32_t r, uint32_t g, uint32_t b, uint32_t a )
{
   gDP.primColor.m = m * 0.0312500000;
   gDP.primColor.l = l * 0.0039215689f;
   gDP.primColor.r = r * 0.0039215689f;
   gDP.primColor.g = g * 0.0039215689f;
   gDP.primColor.b = b * 0.0039215689f;
   gDP.primColor.a = a * 0.0039215689f;

   ShaderCombiner_UpdatePrimColor();

   gDP.changed |= CHANGED_COMBINE_COLORS;
}

void gln64gDPSetTile( uint32_t format, uint32_t size, uint32_t line, uint32_t tmem, uint32_t tile,
      uint32_t palette, uint32_t cmt, uint32_t cms, uint32_t maskt, uint32_t masks, uint32_t shiftt, uint32_t shifts )
{
   if (((size == G_IM_SIZ_4b) || (size == G_IM_SIZ_8b)) && (format == G_IM_FMT_RGBA))
      format = G_IM_FMT_CI;

   gDP.tiles[tile].format = format;
   gDP.tiles[tile].size = size;
   gDP.tiles[tile].line = line;
   gDP.tiles[tile].tmem = tmem;
   gDP.tiles[tile].palette = palette;
   gDP.tiles[tile].cmt = cmt;
   gDP.tiles[tile].cms = cms;
   gDP.tiles[tile].maskt = maskt;
   gDP.tiles[tile].masks = masks;
   gDP.tiles[tile].shiftt = shiftt;
   gDP.tiles[tile].shifts = shifts;

   if (!gDP.tiles[tile].masks) gDP.tiles[tile].clamps = 1;
   if (!gDP.tiles[tile].maskt) gDP.tiles[tile].clampt = 1;

   if (tile == gSP.texture.tile || tile == gSP.texture.tile + 1)
   {
      uint32_t nTile = 7;
      while(gDP.tiles[nTile].tmem != tmem && nTile > gSP.texture.tile + 1)
         --nTile;
      if (nTile > gSP.texture.tile + 1)
      {
         gDP.tiles[tile].textureMode = gDP.tiles[nTile].textureMode;
         gDP.tiles[tile].loadType = gDP.tiles[nTile].loadType;
         gDP.tiles[tile].frameBuffer = gDP.tiles[nTile].frameBuffer;
         gDP.tiles[tile].imageAddress = gDP.tiles[nTile].imageAddress;
      }
   }

	gDP.changed |= CHANGED_TILE;
}

void gln64gDPGetFillColor(float _fillColor[4])
{
	const uint32_t c = gDP.fillColor.color;

	if (gDP.colorImage.size < 3) {
		_fillColor[0] = _SHIFTR( c, 11, 5 ) * 0.032258064f;
		_fillColor[1] = _SHIFTR( c,  6, 5 ) * 0.032258064f;
		_fillColor[2] = _SHIFTR( c,  1, 5 ) * 0.032258064f;
		_fillColor[3] = (float)_SHIFTR( c,  0, 1 );
	} else {
		_fillColor[0] = _SHIFTR( c, 24, 8 ) * 0.0039215686f;
		_fillColor[1] = _SHIFTR( c, 16, 8 ) * 0.0039215686f;
		_fillColor[2] = _SHIFTR( c,  8, 8 ) * 0.0039215686f;
		_fillColor[3] = _SHIFTR( c,  0, 8 ) * 0.0039215686f;
	}
}

void gln64gDPSetTileSize( uint32_t tile, uint32_t uls, uint32_t ult, uint32_t lrs, uint32_t lrt )
{
   gDP.tiles[tile].uls = _SHIFTR( uls, 2, 10 );
   gDP.tiles[tile].ult = _SHIFTR( ult, 2, 10 );
   gDP.tiles[tile].lrs = _SHIFTR( lrs, 2, 10 );
   gDP.tiles[tile].lrt = _SHIFTR( lrt, 2, 10 );

   gDP.tiles[tile].fuls = _FIXED2FLOAT( uls, 2 );
   gDP.tiles[tile].fult = _FIXED2FLOAT( ult, 2 );
   gDP.tiles[tile].flrs = _FIXED2FLOAT( lrs, 2 );
   gDP.tiles[tile].flrt = _FIXED2FLOAT( lrt, 2 );

   gDP.changed |= CHANGED_TILE;
}

static bool CheckForFrameBufferTexture(uint32_t _address, uint32_t _bytes)
{
   struct FrameBuffer *pBuffer;
   int nTile;
   bool bRes = false;

	gDP.loadTile->textureMode = TEXTUREMODE_NORMAL;
	gDP.loadTile->frameBuffer = NULL;
	gDP.changed |= CHANGED_TMEM;
	if (!config.frameBufferEmulation.enable)
		return false;

	pBuffer = (struct FrameBuffer*)FrameBuffer_FindBuffer(_address);
#if 0
	FrameBuffer *pBuffer = fbList.findBuffer(_address);
#endif
	bRes = pBuffer != NULL;
	if (bRes)
   {
      uint32_t texEndAddress;
      if ((config.generalEmulation.hacks & hack_blurPauseScreen) != 0)
      {
#ifdef NEW
         if (gDP.colorImage.address == gDP.depthImageAddress && pBuffer->m_RdramCrc != 0)
         {
            memcpy(RDRAM + gDP.depthImageAddress, RDRAM + pBuffer->m_startAddress, (pBuffer->m_width* pBuffer->m_height) << pBuffer->m_size >> 1);
            pBuffer->m_RdramCrc = 0;
            frameBufferList().getCurrent()->m_isPauseScreen = true;
         }
         if (pBuffer->m_isPauseScreen)
            bRes = false;
#endif
      }

#ifdef NEW
      if (pBuffer->m_cfb)
      {
         FrameBuffer_RemoveBuffer(pBuffer->m_startAddress);
         bRes = false;
      }
#endif

#if 0
		if ((config.generalEmulation.hacks & hack_noDepthFrameBuffers) != 0 && pBuffer->m_isDepthBuffer)
#else
      if ((config.generalEmulation.hacks & hack_noDepthFrameBuffers) != 0 /* && pBuffer->m_isDepthBuffer */)
#endif
      {
         FrameBuffer_RemoveBuffer(pBuffer->m_startAddress);
         bRes = false;
      }

      texEndAddress = _address + _bytes - 1;
      if (_address > pBuffer->m_startAddress && texEndAddress > (pBuffer->m_endAddress + (pBuffer->m_width << pBuffer->m_size >> 1)))
         bRes = false;

      if (bRes && gDP.loadTile->loadType == LOADTYPE_TILE && gDP.textureImage.width != pBuffer->m_width && gDP.textureImage.size != pBuffer->m_size)
         bRes = false;


#ifdef NEW
      if (bRes && pBuffer->m_validityChecked != __RSP.DList)
      {
         if (pBuffer->m_cleared)
         {
            const uint32_t color = pBuffer->m_fillcolor & 0xFFFEFFFE;
            uint32_t wrongPixels = 0;
            for (uint32_t i = pBuffer->m_startAddress + 4; i < pBuffer->m_endAddress; i += 4) {
               if (((*(uint32_t*)&RDRAM[i]) & 0xFFFEFFFE) != color)
                  ++wrongPixels;
            }
            bRes = wrongPixels < (pBuffer->m_endAddress - pBuffer->m_startAddress)/100; // treshold level 1%
            if (bRes)
               pBuffer->m_validityChecked = __RSP.DList;
            else
               frameBufferList().removeBuffer(pBuffer->m_startAddress);
         }
         else if (pBuffer->m_RdramCrc != 0)
         {
            const uint32_t crc = textureCRC(RDRAM + pBuffer->m_startAddress, pBuffer->m_height, pBuffer->m_width << pBuffer->m_size >> 1);
            bRes = (pBuffer->m_RdramCrc == crc);
            if (bRes)
               pBuffer->m_validityChecked = __RSP.DList;
            else
               frameBufferList().removeBuffer(pBuffer->m_startAddress);
         }
      }
#endif

      if (bRes)
      {
#ifdef NEW
         pBuffer->m_pLoadTile = gDP.loadTile;
#endif
         gDP.loadTile->frameBuffer = pBuffer;
         gDP.loadTile->textureMode = TEXTUREMODE_FRAMEBUFFER;
      }
   }

	for (nTile = gSP.texture.tile; nTile < 6; ++nTile)
   {
		if (gDP.tiles[nTile].tmem == gDP.loadTile->tmem)
      {
			struct gDPTile *curTile = (struct gDPTile*)&gDP.tiles[nTile];
			curTile->textureMode  = gDP.loadTile->textureMode;
			curTile->loadType     = gDP.loadTile->loadType;
			curTile->imageAddress = gDP.loadTile->imageAddress;
			curTile->frameBuffer  = gDP.loadTile->frameBuffer;
		}
	}
	return bRes;
}

//****************************************************************
// LoadTile for 32bit RGBA texture
// Based on sources of angrylion's software plugin.
//
void gln64gDPLoadTile32b(uint32_t uls, uint32_t ult, uint32_t lrs, uint32_t lrt)
{
   uint32_t i, j;
	const uint32_t width = lrs - uls + 1;
	const uint32_t height = lrt - ult + 1;
	const uint32_t line = gDP.loadTile->line << 2;
	const uint32_t tbase = gDP.loadTile->tmem << 2;
	const uint32_t addr = gDP.textureImage.address >> 2;
	const uint32_t * src = (const uint32_t*)gfx_info.RDRAM;
	uint16_t * tmem16 = (uint16_t*)TMEM;
	uint32_t c, ptr, tline, s, xorval;

	for (j = 0; j < height; ++j)
   {
      tline = tbase + line * j;
      s = ((j + ult) * gDP.textureImage.width) + uls;
      xorval = (j & 1) ? 3 : 1;

      for (i = 0; i < width; ++i)
      {
         c = src[addr + s + i];
         ptr = ((tline + i) ^ xorval) & 0x3ff;
         tmem16[ptr] = c >> 16;
         tmem16[ptr | 0x400] = c & 0xffff;
      }
   }
}

void gln64gDPLoadTile(uint32_t tile, uint32_t uls, uint32_t ult, uint32_t lrs, uint32_t lrt)
{
    uint32_t width, height, address, bpl, bpl2, height2;
	struct gDPLoadTileInfo *info;

	gln64gDPSetTileSize( tile, uls, ult, lrs, lrt );
	gDP.loadTile = &gDP.tiles[tile];
	gDP.loadTile->loadType = LOADTYPE_TILE;
	gDP.loadTile->imageAddress = gDP.textureImage.address;

	width  = (gDP.loadTile->lrs - gDP.loadTile->uls + 1) & 0x03FF;
	height = (gDP.loadTile->lrt - gDP.loadTile->ult + 1) & 0x03FF;

	info             = (struct gDPLoadTileInfo*)&gDP.loadInfo[gDP.loadTile->tmem];
	info->texAddress = gDP.loadTile->imageAddress;
	info->uls        = gDP.loadTile->uls;
	info->ult        = gDP.loadTile->ult;
	info->width      = gDP.loadTile->masks != 0 ? (uint16_t)MIN(width, 1U<<gDP.loadTile->masks) : (uint16_t)width;
	info->height     = gDP.loadTile->maskt != 0 ? (uint16_t)MIN(height, 1U<<gDP.loadTile->maskt) : (uint16_t)height;
	info->texWidth   = gDP.textureImage.width;
	info->size       = gDP.textureImage.size;
	info->loadType   = LOADTYPE_TILE;

	if (gDP.loadTile->line == 0)
		return;

   address = gDP.textureImage.address + gDP.loadTile->ult * gDP.textureImage.bpl + (gDP.loadTile->uls << gDP.textureImage.size >> 1);
	if ((address + height * gDP.textureImage.bpl) > RDRAMSize)
		return;

	bpl = gDP.loadTile->line << 3;
    bpl2 = bpl;
	if (gDP.loadTile->lrs > gDP.textureImage.width)
		bpl2 = (gDP.textureImage.width - gDP.loadTile->uls);
	height2 = height;
	if (gDP.loadTile->lrt > gDP.scissor.lry)
		height2 = gDP.scissor.lry - gDP.loadTile->ult;
	if (CheckForFrameBufferTexture(address, bpl2*height2))
		return;

	if (gDP.loadTile->size == G_IM_SIZ_32b)
		gln64gDPLoadTile32b(gDP.loadTile->uls, gDP.loadTile->ult, gDP.loadTile->lrs, gDP.loadTile->lrt);
	else
   {
      uint32_t y;
      uint32_t tmemAddr = gDP.loadTile->tmem;

      const uint32_t line = gDP.loadTile->line;
      for (y = 0; y < height; ++y)
      {
         UnswapCopyWrap(gfx_info.RDRAM, address, (uint8_t*)TMEM, tmemAddr << 3, 0xFFF, bpl);
         if (y & 1)
            DWordInterleaveWrap((uint32_t*)TMEM, tmemAddr << 1, 0x3FF, line);

         address += gDP.textureImage.bpl;
         tmemAddr += line;
      }
   }
}

//****************************************************************
// LoadBlock for 32bit RGBA texture
// Based on sources of angrylion's software plugin.
//
void gln64gDPLoadBlock32(uint32_t uls,uint32_t lrs, uint32_t dxt)
{
	const uint32_t * src = (const uint32_t*)gfx_info.RDRAM;
	const uint32_t tb = gDP.loadTile->tmem << 2;
	const uint32_t line = gDP.loadTile->line << 2;

	uint16_t *tmem16 = (uint16_t*)TMEM;
	uint32_t addr = gDP.loadTile->imageAddress >> 2;
	uint32_t width = (lrs - uls + 1) << 2;
	if (width == 4) // lr_s == 0, 1x1 texture
		width = 1;
	else if (width & 7)
		width = (width & (~7)) + 8;

	if (dxt != 0)
   {
      uint32_t i;
		uint32_t j = 0;
		uint32_t t = 0;
		uint32_t oldt = 0;
		uint32_t ptr;

		uint32_t c = 0;
		for (i = 0; i < width; i += 2)
      {
         oldt = t;
         t = ((j >> 11) & 1) ? 3 : 1;
         if (t != oldt)
            i += line;
         ptr = ((tb + i) ^ t) & 0x3ff;
         c = src[addr + i];
         tmem16[ptr] = c >> 16;
         tmem16[ptr | 0x400] = c & 0xffff;
         ptr = ((tb + i + 1) ^ t) & 0x3ff;
         c = src[addr + i + 1];
         tmem16[ptr] = c >> 16;
         tmem16[ptr | 0x400] = c & 0xffff;
         j += dxt;
      }
	}
   else
   {
      uint32_t c, ptr, i;
      for (i = 0; i < width; i++)
      {
         ptr = ((tb + i) ^ 1) & 0x3ff;
         c = src[addr + i];
         tmem16[ptr] = c >> 16;
         tmem16[ptr | 0x400] = c & 0xffff;
      }
   }
}

void gln64gDPLoadBlock(uint32_t tile, uint32_t uls, uint32_t ult, uint32_t lrs, uint32_t dxt)
{
	uint32_t bytes, address;
    struct gDPLoadTileInfo *info;

	gln64gDPSetTileSize( tile, uls, ult, lrs, dxt );
	gDP.loadTile = &gDP.tiles[tile];
	gDP.loadTile->loadType = LOADTYPE_BLOCK;

	if (gSP.DMAOffsets.tex_offset != 0) {
		if (gSP.DMAOffsets.tex_shift % (((lrs>>2) + 1) << 3)) {
			gDP.textureImage.address -= gSP.DMAOffsets.tex_shift;
			gSP.DMAOffsets.tex_offset = 0;
			gSP.DMAOffsets.tex_shift = 0;
			gSP.DMAOffsets.tex_count = 0;
		} else
			++gSP.DMAOffsets.tex_count;
	}
	gDP.loadTile->imageAddress = gDP.textureImage.address;

	info = (struct gDPLoadTileInfo*)&gDP.loadInfo[gDP.loadTile->tmem];
	info->texAddress = gDP.loadTile->imageAddress;
	info->width = gDP.loadTile->lrs;
	info->dxt = dxt;
	info->size = gDP.textureImage.size;
	info->loadType = LOADTYPE_BLOCK;

	bytes = (lrs - uls + 1) << gDP.loadTile->size >> 1;

	if ((bytes & 7) != 0)
		bytes = (bytes & (~7)) + 8;
	address = gDP.textureImage.address + ult * gDP.textureImage.bpl + (uls << gDP.textureImage.size >> 1);

	if (bytes == 0 || (address + bytes) > RDRAMSize)
		return;

	gDP.loadTile->textureMode = TEXTUREMODE_NORMAL;
	gDP.loadTile->frameBuffer = NULL;
	CheckForFrameBufferTexture(address, bytes); // Load data to TMEM even if FB texture is found. See comment to texturedRectDepthBufferCopy

	if (gDP.loadTile->size == G_IM_SIZ_32b)
		gln64gDPLoadBlock32(gDP.loadTile->uls, gDP.loadTile->lrs, dxt);
	else if (gDP.loadTile->format == G_IM_FMT_YUV)
		memcpy(TMEM, &gfx_info.RDRAM[address], bytes); // HACK!
	else {
      uint32_t tmemAddr = gDP.loadTile->tmem;

		if (dxt > 0)
      {
         uint32_t y;
         uint32_t line = (2047 + dxt) / dxt;
         uint32_t bpl = line << 3;
         uint32_t height = bytes / bpl;

         for (y = 0; y < height; ++y)
         {
            UnswapCopyWrap(gfx_info.RDRAM, address, (uint8_t*)TMEM, tmemAddr << 3, 0xFFF, bpl);
            if (y & 1)
               DWordInterleaveWrap((uint32_t*)TMEM, tmemAddr << 1, 0x3FF, line);

            address += bpl;
            tmemAddr += line;
         }
      } else
         UnswapCopyWrap(gfx_info.RDRAM, address, (uint8_t*)TMEM, tmemAddr << 3, 0xFFF, bytes);
	}
}

void gln64gDPLoadTLUT( uint32_t tile, uint32_t uls, uint32_t ult, uint32_t lrs, uint32_t lrt )
{
   int i;
   uint32_t address;
   uint16_t count, pal, *dest, j;

	gln64gDPSetTileSize( tile, uls, ult, lrs, lrt );
	if (gDP.tiles[tile].tmem < 256)
		return;
	count = (uint16_t)((gDP.tiles[tile].lrs - gDP.tiles[tile].uls + 1) * (gDP.tiles[tile].lrt - gDP.tiles[tile].ult + 1));
	address = gDP.textureImage.address + gDP.tiles[tile].ult * gDP.textureImage.bpl + (gDP.tiles[tile].uls << gDP.textureImage.size >> 1);
	pal = (uint16_t)((gDP.tiles[tile].tmem - 256) >> 4);
	dest = (uint16_t*)&TMEM[gDP.tiles[tile].tmem];

	i = 0;
	while (i < count)
   {
		for (j = 0; (j < 16) && (i < count); ++j, ++i)
      {
			*dest = swapword(*(uint16_t*)(gfx_info.RDRAM + (address ^ 2)));
			address += 2;
			dest += 4;
		}

      /* TODO/FIXME - should be CRC_CalculatePalette here instead */
		gDP.paletteCRC16[pal] = Hash_Calculate(0xFFFFFFFF, &TMEM[256 + (pal << 4)], 16);
		++pal;
	}

	gDP.paletteCRC256 = Hash_Calculate(0xFFFFFFFF, gDP.paletteCRC16, 64);

#ifdef TEXTURE_FILTER
	if (TFH.isInited())
   {
		const uint16_t start = gDP.tiles[tile].tmem - 256; // starting location in the palettes
		uint16_t *spal = (uint16_t*)(gfx_info.RDRAM + gDP.textureImage.address);
		memcpy((uint8_t*)(gDP.TexFilterPalette + start), spal, count<<1);
	}
#endif

	gDP.changed |= CHANGED_TMEM;
}

void gln64gDPSetScissor( uint32_t mode, float ulx, float uly, float lrx, float lry )
{
   gDP.scissor.mode = mode;
   gDP.scissor.ulx = ulx;
   gDP.scissor.uly = uly;
   gDP.scissor.lrx = lrx;
   gDP.scissor.lry = lry;

   gDP.changed |= CHANGED_SCISSOR;

#if 0
	frameBufferList().correctHeight();
#endif
}

void gln64gDPFillRDRAM(uint32_t address, int32_t ulx, int32_t uly, int32_t lrx, int32_t lry, uint32_t width, uint32_t size, uint32_t color, bool scissor)
{
   uint32_t y, x, stride, lowerBound, ci_width_in_dwords, *dst;
   struct FrameBuffer *pCurrentBuffer;

   pCurrentBuffer = (struct FrameBuffer*)FrameBuffer_GetCurrent();
   
   if (pCurrentBuffer != NULL)
   {
#ifdef NEW
      pCurrentBuffer->m_cleared = true;
      pCurrentBuffer->m_fillcolor = color;
#endif
   }
	if (scissor)
   {
		ulx = MIN(MAX((float)ulx, gDP.scissor.ulx), gDP.scissor.lrx);
		lrx = MIN(MAX((float)lrx, gDP.scissor.ulx), gDP.scissor.lrx);
		uly = MIN(MAX((float)uly, gDP.scissor.uly), gDP.scissor.lry);
		lry = MIN(MAX((float)lry, gDP.scissor.uly), gDP.scissor.lry);
	}
	stride = width << size >> 1;
	lowerBound = address + lry*stride;
	if (lowerBound > RDRAMSize)
		lry -= (lowerBound - RDRAMSize) / stride;
	ci_width_in_dwords = width >> (3 - size);
	ulx >>= (3 - size);
	lrx >>= (3 - size);
	dst = (uint32_t*)(gfx_info.RDRAM + address);
	dst += uly * ci_width_in_dwords;

	for (y = uly; y < lry; ++y)
   {
		for (x = ulx; x < lrx; ++x)
			dst[x] = color;
		dst += ci_width_in_dwords;
	}
}

void gln64gDPFillRectangle( int32_t ulx, int32_t uly, int32_t lrx, int32_t lry )
{
   float fillColor[4];
   DepthBuffer *buffer = NULL;
   
   if (gDP.otherMode.cycleType == G_CYC_FILL)
   {
      ++lrx;
      ++lry;
   }
   else if (lry == uly)
      ++lry;

   buffer = (DepthBuffer*)DepthBuffer_FindBuffer( gDP.colorImage.address );
   if (buffer)
      buffer->cleared = true;

   if (gDP.depthImageAddress == gDP.colorImage.address)
   {
		/* Game may use depth texture as auxilary color texture. Example: Mario Tennis
       * If color is not depth clear color, that is most likely the case */
      if (gDP.fillColor.color == DepthClearColor)
      {
         gln64gDPFillRDRAM(gDP.colorImage.address, ulx, uly, lrx, lry,
               gDP.colorImage.width, gDP.colorImage.size, gDP.fillColor.color, true);
         OGL_ClearDepthBuffer(lrx - ulx >= gDP.scissor.lrx - gDP.scissor.ulx && lry - uly >= gDP.scissor.lry - gDP.scissor.uly);
         return;
      }
   }
   else if (gDP.fillColor.color == DepthClearColor && gDP.otherMode.cycleType == G_CYC_FILL)
   {
#ifdef NEW
		depthBufferList().saveBuffer(gDP.colorImage.address);
#endif
		gln64gDPFillRDRAM(gDP.colorImage.address, ulx, uly, lrx, lry, gDP.colorImage.width, gDP.colorImage.size, gDP.fillColor.color, true);
		OGL_ClearDepthBuffer(lrx - ulx == gDP.scissor.lrx - gDP.scissor.ulx && lry - uly == gDP.scissor.lry - gDP.scissor.uly);
		return;
	}

#ifdef NEW
	frameBufferList().setBufferChanged();
#endif

   gln64gDPGetFillColor(fillColor);

   if (gDP.otherMode.cycleType == G_CYC_FILL)
   {
      if ((ulx == 0) && (uly == 0) && (lrx == gDP.scissor.lrx) && (lry == gDP.scissor.lry))
      {
			gln64gDPFillRDRAM(gDP.colorImage.address, ulx, uly, lrx, lry,
               gDP.colorImage.width, gDP.colorImage.size, gDP.fillColor.color, true);

			if ((*gfx_info.VI_STATUS_REG & 8) != 0)
         {
				fillColor[0] = sqrtf(fillColor[0]);
				fillColor[1] = sqrtf(fillColor[1]);
				fillColor[2] = sqrtf(fillColor[2]);
			}
         OGL_ClearColorBuffer(&fillColor[0]);
         return;
      }
   }

   OGL_DrawRect( ulx, uly, lrx, lry, fillColor);

   /* TODO/FIXME - remove? */
   if (depthBuffer.current)
      depthBuffer.current->cleared = false;

   if (gDP.otherMode.cycleType == G_CYC_FILL)
   {
      if (lry > (uint32_t)gDP.scissor.lry)
         gDP.colorImage.height = (uint32_t)MAX( gDP.colorImage.height, (unsigned int)gDP.scissor.lry );
      else
         gDP.colorImage.height = (uint32_t)MAX((int32_t)gDP.colorImage.height, lry);
   }
   else
      gDP.colorImage.height = MAX( gDP.colorImage.height, (unsigned int)lry );
}

void gln64gDPSetConvert( int32_t k0, int32_t k1, int32_t k2, int32_t k3, int32_t k4, int32_t k5 )
{
	gDP.convert.k0 = SIGN(k0, 9);
	gDP.convert.k1 = SIGN(k1, 9);
	gDP.convert.k2 = SIGN(k2, 9);
	gDP.convert.k3 = SIGN(k3, 9);
	gDP.convert.k4 = SIGN(k4, 9);
	gDP.convert.k5 = SIGN(k5, 9);

   ShaderCombiner_UpdateConvertColor();
}

void gln64gDPSetKeyR( uint32_t cR, uint32_t sR, uint32_t wR )
{
   gDP.key.center.r = cR * 0.0039215689f;;
   gDP.key.scale.r = sR * 0.0039215689f;;
   gDP.key.width.r = wR * 0.0039215689f;;

}

void gln64gDPSetKeyGB(uint32_t cG, uint32_t sG, uint32_t wG, uint32_t cB, uint32_t sB, uint32_t wB )
{
   gDP.key.center.g = cG * 0.0039215689f;;
   gDP.key.scale.g = sG * 0.0039215689f;;
   gDP.key.width.g = wG * 0.0039215689f;;
   gDP.key.center.b = cB * 0.0039215689f;;
   gDP.key.scale.b = sB * 0.0039215689f;;
   gDP.key.width.b = wB * 0.0039215689f;;

   ShaderCombiner_UpdateKeyColor();
}

void gln64gDPTextureRectangle( float ulx, float uly, float lrx, float lry, int32_t tile, float s, float t, float dsdx, float dtdy )
{
   struct TexturedRectParams params;
   float lrs, lrt;
   float tmp;
	struct gDPTile *textureTileOrg[2];

   if (gDP.otherMode.cycleType == G_CYC_COPY)
   {
      dsdx = 1.0f;
      lrx += 1.0f;
      lry += 1.0f;
   }
	lry = MAX(lry, uly + 1.0f);

	textureTileOrg[0] = gSP.textureTile[0];
	textureTileOrg[1] = gSP.textureTile[1];
   gSP.textureTile[0] = &gDP.tiles[tile];
   gSP.textureTile[1] = &gDP.tiles[(tile + 1) & 7];

   if (gDP.loadTile->textureMode == TEXTUREMODE_NORMAL)
      gDP.loadTile->textureMode = TEXTUREMODE_TEXRECT;

	if (gSP.textureTile[1]->textureMode == TEXTUREMODE_NORMAL)
		gSP.textureTile[1]->textureMode = TEXTUREMODE_TEXRECT;

	// HACK ALERT!
   if (((int)(s) == 512) && (gDP.colorImage.width + gSP.textureTile[0]->uls < 512))
		s = 0.0f;

   if (__RSP.cmd == G_TEXRECTFLIP)
   {
      lrs = s + (lry - uly - 1) * dtdy;
      lrt = t + (lrx - ulx - 1) * dsdx;
   }
   else
   {
      lrs = s + (lrx - ulx - 1) * dsdx;
      lrt = t + (lry - uly - 1) * dtdy;
   }

   gDP.texRect.width = (unsigned int)(MAX( lrs, s ) + dsdx);
   gDP.texRect.height = (unsigned int)(MAX( lrt, t ) + dtdy);

   params.ulx  = ulx;
   params.uly  = uly;
   params.lrx  = lrx;
   params.lry  = lry;
   params.uls  = s;
   params.ult  = t;
   params.lrs  = lrs;
   params.lrt  = lrt;
   params.flip = (__RSP.cmd == G_TEXRECTFLIP);
   OGL_DrawTexturedRect(&params);

	gSP.textureTile[0] = textureTileOrg[0];
	gSP.textureTile[1] = textureTileOrg[1];

#if 1
   if (depthBuffer.current)
      depthBuffer.current->cleared = false;
   gDP.colorImage.changed = true;
#else
	frameBufferList().setBufferChanged();
#endif
   if (gDP.colorImage.width < 64)
		gDP.colorImage.height = (uint32_t)MAX( (float)gDP.colorImage.height, lry );
   else
      gDP.colorImage.height = (unsigned int)(MAX( gDP.colorImage.height, gDP.scissor.lry ));
}

void gln64gDPTextureRectangleFlip( float ulx, float uly, float lrx, float lry, int32_t tile, float s, float t, float dsdx, float dtdy )
{
   gln64gDPTextureRectangle( ulx, uly, lrx, lry, tile, s, t, dsdx, dtdy );
}

void gln64gDPFullSync(void)
{
#if 0
	if (config.frameBufferEmulation.copyAuxToRDRAM != 0) {
		frameBufferList().copyAux();
		frameBufferList().removeAux();
	}

	const bool sync = config.frameBufferEmulation.copyToRDRAM == Config::ctSync;
	if (config.frameBufferEmulation.copyToRDRAM != Config::ctDisable)
		FrameBuffer_CopyToRDRAM(gDP.colorImage.address, sync);

	if (__RSP.bLLE)
   {
		if (config.frameBufferEmulation.copyDepthToRDRAM != Config::ctDisable)
			FrameBuffer_CopyDepthBuffer(gDP.colorImage.address);
	}
#endif

   *gfx_info.MI_INTR_REG |= MI_INTR_DP;

   if (gfx_info.CheckInterrupts)
      gfx_info.CheckInterrupts();
}

void gln64gDPTileSync(void)
{
}

void gln64gDPPipeSync(void)
{
}

void gln64gDPLoadSync(void)
{
}

void gln64gDPNoOp(void)
{
}

/*******************************************
 *          Low level triangle             *
 *******************************************
 *    based on sources of ziggy's z64      *
 *******************************************/

#define XSCALE(x) ((float)(x)/(1<<18))
#define YSCALE(y) ((float)(y)/(1<<2))
#define ZSCALE(z) ((gDP.otherMode.depthSource == G_ZS_PRIM) ? (gDP.primDepth.z) : ((float)((uint32_t)((z))) / 0xffff0000))
#define PERSP_EN (gDP.otherMode.texturePersp != 0)
#define WSCALE(z) 1.0f/(PERSP_EN? ((float)((uint32_t)(z) + 0x10000)/0xffff0000) : 1.0f)
#define CSCALE(c) ((((c)>0x3ff0000? 0x3ff0000:((c)<0? 0 : (c)))>>18)*0.0039215689f)
#define _PERSP(w) ( w )
#define PERSP(s, w) ( ((int64_t)(s) << 20) / (_PERSP(w)? _PERSP(w):1) )
#define SSCALE(s, _w) (PERSP_EN? (float)(PERSP(s, _w))/(1 << 10) : (float)(s)/(1<<21))
#define TSCALE(s, w) (PERSP_EN? (float)(PERSP(s, w))/(1 << 10) : (float)(s)/(1<<21))

void gln64gDPLLETriangle(uint32_t _w1, uint32_t _w2, int _shade, int _texture, int _zbuffer, uint32_t * _pRdpCmd)
{
	int32_t yl, ym, yh;
	int32_t xl, xm, xh;
	int32_t dxldy, dxhdy, dxmdy;
	uint32_t w3, w4, w5, w6, w7, w8;
	struct SPVertex *vtx0, *vtx;
	int j;
	int xleft, xright, xleft_inc, xright_inc;
	int r, g, b, a, z, s, t, w;
	int drdx = 0, dgdx = 0, dbdx = 0, dadx = 0, dzdx = 0, dsdx = 0, dtdx = 0, dwdx = 0;
	int drde = 0, dgde = 0, dbde = 0, dade = 0, dzde = 0, dsde = 0, dtde = 0, dwde = 0;
	int flip = (_w1 & 0x8000000) ? 1 : 0;
	uint32_t * shade_base = _pRdpCmd + 8;
	uint32_t * texture_base = _pRdpCmd + 8;
	uint32_t * zbuffer_base = _pRdpCmd + 8;
	const uint32_t tile = _SHIFTR(_w1, 16, 3);
	struct gDPTile *textureTileOrg[2];
	textureTileOrg[0] = gSP.textureTile[0];
	textureTileOrg[1] = gSP.textureTile[1];
	gSP.textureTile[0] = &gDP.tiles[tile];
	gSP.textureTile[1] = &gDP.tiles[(tile + 1) & 7];

	if (_shade != 0) {
		texture_base += 16;
		zbuffer_base += 16;
	}
	if (_texture != 0) {
		zbuffer_base += 16;
	}

	w3 = _pRdpCmd[2];
	w4 = _pRdpCmd[3];
	w5 = _pRdpCmd[4];
	w6 = _pRdpCmd[5];
	w7 = _pRdpCmd[6];
	w8 = _pRdpCmd[7];

	yl = (_w1 & 0x3fff);
	ym = ((_w2 >> 16) & 0x3fff);
	yh = ((_w2 >>  0) & 0x3fff);
	xl = (int32_t)(w3);
	xh = (int32_t)(w5);
	xm = (int32_t)(w7);
	dxldy = (int32_t)(w4);
	dxhdy = (int32_t)(w6);
	dxmdy = (int32_t)(w8);

	if (yl & (0x800<<2)) yl |= 0xfffff000<<2;
	if (ym & (0x800<<2)) ym |= 0xfffff000<<2;
	if (yh & (0x800<<2)) yh |= 0xfffff000<<2;

	yh &= ~3;

	r = 0xff; g = 0xff; b = 0xff; a = 0xff; z = 0xffff0000; s = 0;  t = 0;  w = 0x30000;

	if (_shade != 0) {
		r    = (shade_base[0] & 0xffff0000) | ((shade_base[+4 ] >> 16) & 0x0000ffff);
		g    = ((shade_base[0 ] << 16) & 0xffff0000) | (shade_base[4 ] & 0x0000ffff);
		b    = (shade_base[1 ] & 0xffff0000) | ((shade_base[5 ] >> 16) & 0x0000ffff);
		a    = ((shade_base[1 ] << 16) & 0xffff0000) | (shade_base[5 ] & 0x0000ffff);
		drdx = (shade_base[2 ] & 0xffff0000) | ((shade_base[6 ] >> 16) & 0x0000ffff);
		dgdx = ((shade_base[2 ] << 16) & 0xffff0000) | (shade_base[6 ] & 0x0000ffff);
		dbdx = (shade_base[3 ] & 0xffff0000) | ((shade_base[7 ] >> 16) & 0x0000ffff);
		dadx = ((shade_base[3 ] << 16) & 0xffff0000) | (shade_base[7 ] & 0x0000ffff);
		drde = (shade_base[8 ] & 0xffff0000) | ((shade_base[12] >> 16) & 0x0000ffff);
		dgde = ((shade_base[8 ] << 16) & 0xffff0000) | (shade_base[12] & 0x0000ffff);
		dbde = (shade_base[9 ] & 0xffff0000) | ((shade_base[13] >> 16) & 0x0000ffff);
		dade = ((shade_base[9 ] << 16) & 0xffff0000) | (shade_base[13] & 0x0000ffff);
	}
	if (_texture != 0) {
		s    = (texture_base[0 ] & 0xffff0000) | ((texture_base[4 ] >> 16) & 0x0000ffff);
		t    = ((texture_base[0 ] << 16) & 0xffff0000)      | (texture_base[4 ] & 0x0000ffff);
		w    = (texture_base[1 ] & 0xffff0000) | ((texture_base[5 ] >> 16) & 0x0000ffff);
		//    w = abs(w);
		dsdx = (texture_base[2 ] & 0xffff0000) | ((texture_base[6 ] >> 16) & 0x0000ffff);
		dtdx = ((texture_base[2 ] << 16) & 0xffff0000)      | (texture_base[6 ] & 0x0000ffff);
		dwdx = (texture_base[3 ] & 0xffff0000) | ((texture_base[7 ] >> 16) & 0x0000ffff);
		dsde = (texture_base[8 ] & 0xffff0000) | ((texture_base[12] >> 16) & 0x0000ffff);
		dtde = ((texture_base[8 ] << 16) & 0xffff0000)      | (texture_base[12] & 0x0000ffff);
		dwde = (texture_base[9 ] & 0xffff0000) | ((texture_base[13] >> 16) & 0x0000ffff);
	}
	if (_zbuffer != 0) {
		z    = zbuffer_base[0];
		dzdx = zbuffer_base[1];
		dzde = zbuffer_base[2];
	}

	xh <<= 2;  xm <<= 2;  xl <<= 2;
	r <<= 2;  g <<= 2;  b <<= 2;  a <<= 2;
	dsde >>= 2;  dtde >>= 2;  dsdx >>= 2;  dtdx >>= 2;
	dzdx >>= 2;  dzde >>= 2;
	dwdx >>= 2;  dwde >>= 2;

	vtx0 = (struct SPVertex*)&OGL.triangles.vertices[0];
	vtx = (struct SPVertex*)vtx0;

	xleft = xm;
	xright = xh;
	xleft_inc = dxmdy;
	xright_inc = dxhdy;

	while (yh<ym &&
		!((!flip && xleft < xright+0x10000) ||
		 (flip && xleft > xright-0x10000))) {
		xleft += xleft_inc;
		xright += xright_inc;
		s += dsde;    t += dtde;    w += dwde;
		r += drde;    g += dgde;    b += dbde;    a += dade;
		z += dzde;
		yh++;
	}

	j = ym-yh;
	if (j > 0) {
		int dx = (xleft-xright)>>16;
		if ((!flip && xleft < xright) ||
				(flip/* && xleft > xright*/))
		{
			if (_shade != 0) {
				vtx->r = CSCALE(r+drdx*dx);
				vtx->g = CSCALE(g+dgdx*dx);
				vtx->b = CSCALE(b+dbdx*dx);
				vtx->a = CSCALE(a+dadx*dx);
			}
			if (_texture != 0) {
				vtx->s = SSCALE(s+dsdx*dx, w+dwdx*dx);
				vtx->t = TSCALE(t+dtdx*dx, w+dwdx*dx);
			}
			vtx->x = XSCALE(xleft);
			vtx->y = YSCALE(yh);
			vtx->z = ZSCALE(z+dzdx*dx);
			vtx->w = WSCALE(w+dwdx*dx);
			++vtx;
		}
		if ((!flip/* && xleft < xright*/) ||
				(flip && xleft > xright))
		{
			if (_shade != 0) {
				vtx->r = CSCALE(r);
				vtx->g = CSCALE(g);
				vtx->b = CSCALE(b);
				vtx->a = CSCALE(a);
			}
			if (_texture != 0) {
				vtx->s = SSCALE(s, w);
				vtx->t = TSCALE(t, w);
			}
			vtx->x = XSCALE(xright);
			vtx->y = YSCALE(yh);
			vtx->z = ZSCALE(z);
			vtx->w = WSCALE(w);
			++vtx;
		}
		xleft += xleft_inc*j;  xright += xright_inc*j;
		s += dsde*j;  t += dtde*j;
		if (w + dwde*j) w += dwde*j;
		else w += dwde*(j-1);
		r += drde*j;  g += dgde*j;  b += dbde*j;  a += dade*j;
		z += dzde*j;
		// render ...
	}

	if (xl != xh)
		xleft = xl;

	//if (yl-ym > 0)
	{
		int dx = (xleft-xright)>>16;
		if ((!flip && xleft <= xright) ||
				(flip/* && xleft >= xright*/))
		{
			if (_shade != 0) {
				vtx->r = CSCALE(r+drdx*dx);
				vtx->g = CSCALE(g+dgdx*dx);
				vtx->b = CSCALE(b+dbdx*dx);
				vtx->a = CSCALE(a+dadx*dx);
			}
			if (_texture != 0) {
				vtx->s = SSCALE(s+dsdx*dx, w+dwdx*dx);
				vtx->t = TSCALE(t+dtdx*dx, w+dwdx*dx);
			}
			vtx->x = XSCALE(xleft);
			vtx->y = YSCALE(ym);
			vtx->z = ZSCALE(z+dzdx*dx);
			vtx->w = WSCALE(w+dwdx*dx);
			++vtx;
		}
		if ((!flip/* && xleft <= xright*/) ||
				(flip && xleft >= xright))
		{
			if (_shade != 0) {
				vtx->r = CSCALE(r);
				vtx->g = CSCALE(g);
				vtx->b = CSCALE(b);
				vtx->a = CSCALE(a);
			}
			if (_texture != 0) {
				vtx->s = SSCALE(s, w);
				vtx->t = TSCALE(t, w);
			}
			vtx->x = XSCALE(xright);
			vtx->y = YSCALE(ym);
			vtx->z = ZSCALE(z);
			vtx->w = WSCALE(w);
			++vtx;
		}
	}
	xleft_inc = dxldy;
	xright_inc = dxhdy;

	j = yl-ym;
	//j--; // ?
	xleft += xleft_inc*j;  xright += xright_inc*j;
	s += dsde*j;  t += dtde*j;  w += dwde*j;
	r += drde*j;  g += dgde*j;  b += dbde*j;  a += dade*j;
	z += dzde*j;

	while (yl>ym &&
		   !((!flip && xleft < xright+0x10000) ||
			 (flip && xleft > xright-0x10000))) {
		xleft -= xleft_inc;    xright -= xright_inc;
		s -= dsde;    t -= dtde;    w -= dwde;
		r -= drde;    g -= dgde;    b -= dbde;    a -= dade;
		z -= dzde;
		--j;
		--yl;
	}

	// render ...
	if (j >= 0) {
		int dx = (xleft-xright)>>16;
		if ((!flip && xleft <= xright) ||
				(flip/* && xleft >= xright*/))
		{
			if (_shade != 0) {
				vtx->r = CSCALE(r+drdx*dx);
				vtx->g = CSCALE(g+dgdx*dx);
				vtx->b = CSCALE(b+dbdx*dx);
				vtx->a = CSCALE(a+dadx*dx);
			}
			if (_texture != 0) {
				vtx->s = SSCALE(s+dsdx*dx, w+dwdx*dx);
				vtx->t = TSCALE(t+dtdx*dx, w+dwdx*dx);
			}
			vtx->x = XSCALE(xleft);
			vtx->y = YSCALE(yl);
			vtx->z = ZSCALE(z+dzdx*dx);
			vtx->w = WSCALE(w+dwdx*dx);
			++vtx;
		}
		if ((!flip/* && xleft <= xright*/) ||
				(flip && xleft >= xright))
		{
			if (_shade != 0) {
				vtx->r = CSCALE(r);
				vtx->g = CSCALE(g);
				vtx->b = CSCALE(b);
				vtx->a = CSCALE(a);
			}
			if (_texture != 0) {
				vtx->s = SSCALE(s, w);
				vtx->t = TSCALE(t, w);
			}
			vtx->x = XSCALE(xright);
			vtx->y = YSCALE(yl);
			vtx->z = ZSCALE(z);
			vtx->w = WSCALE(w);
			++vtx;
		}
	}

	if (_texture != 0)
		gSP.changed |= CHANGED_TEXTURE;
	if (_zbuffer != 0)
		gSP.geometryMode |= G_ZBUFFER;

	OGL_DrawLLETriangle(vtx - vtx0);
	gSP.textureTile[0] = textureTileOrg[0];
	gSP.textureTile[1] = textureTileOrg[1];
}

static void gln64gDPTriangle(uint32_t _w1, uint32_t _w2, int shade, int texture, int zbuffer)
{
	gln64gDPLLETriangle(_w1, _w2, shade, texture, zbuffer, __RDP.cmd_data + __RDP.cmd_cur);
}

void gln64gDPTriFill(uint32_t w0, uint32_t w1)
{
	gln64gDPTriangle(w0, w1, 0, 0, 0);
}

void gln64gDPTriShade(uint32_t w0, uint32_t w1)
{
	gln64gDPTriangle(w0, w1, 1, 0, 0);
}

void gln64gDPTriTxtr(uint32_t w0, uint32_t w1)
{
	gln64gDPTriangle(w0, w1, 0, 1, 0);
}

void gln64gDPTriShadeTxtr(uint32_t w0, uint32_t w1)
{
	gln64gDPTriangle(w0, w1, 1, 1, 0);
}

void gln64gDPTriFillZ(uint32_t w0, uint32_t w1)
{
	gln64gDPTriangle(w0, w1, 0, 0, 1);
}

void gln64gDPTriShadeZ(uint32_t w0, uint32_t w1)
{
	gln64gDPTriangle(w0, w1, 1, 0, 1);
}

void gln64gDPTriTxtrZ(uint32_t w0, uint32_t w1)
{
	gln64gDPTriangle(w0, w1, 0, 1, 1);
}

void gln64gDPTriShadeTxtrZ(uint32_t w0, uint32_t w1)
{
	gln64gDPTriangle(w0, w1, 1, 1, 1);
}
