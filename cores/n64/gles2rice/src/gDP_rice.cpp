#include <stdint.h>

#include <retro_miscellaneous.h>

#include "osal_preproc.h"
#include "float.h"
#include "ConvertImage.h"
#include "DeviceBuilder.h"
#include "FrameBuffer.h"
#include "Render.h"
#include "Timing.h"

#include "../../Graphics/GBI.h"
#include "../../Graphics/RDP/gDP_funcs_prot.h"
#include "../../Graphics/image_convert.h"

extern TMEMLoadMapInfo g_tmemLoadAddrMap[0x200]; /* Totally 4KB TMEM */
extern TMEMLoadMapInfo g_tmemInfo0;              /* Info for Tmem=0 */
extern TMEMLoadMapInfo g_tmemInfo1;              /* Info for Tmem=0x100 */
extern uint32_t g_TmemFlag[16];
extern uint32_t g_TxtLoadBy;

static inline void UnswapCopy( void *src, void *dest, uint32_t numBytes )
{
   // copy leading bytes
   int leadingBytes = ((uintptr_t)src) & 3;
   if (leadingBytes != 0)
   {
      leadingBytes = 4-leadingBytes;
      if ((unsigned int)leadingBytes > numBytes)
         leadingBytes = numBytes;
      numBytes -= leadingBytes;

#ifdef MSB_FIRST
      src = (void *)((uintptr_t)src);
#else
      src = (void *)((uintptr_t)src ^ 3);
#endif
      for (int i = 0; i < leadingBytes; i++)
      {
         *(uint8_t *)(dest) = *(uint8_t *)(src);
         dest = (void *)((uintptr_t)dest+1);
         src  = (void *)((uintptr_t)src -1);
      }
      src = (void *)((uintptr_t)src+5);
   }

   // copy dwords
   int numDWords = numBytes >> 2;
   while (numDWords--)
   {
      uint32_t dword = *(uint32_t *)src;
      dword = ((dword<<24)|((dword<<8)&0x00FF0000)|((dword>>8)&0x0000FF00)|(dword>>24));
      *(uint32_t *)dest = dword;
      dest = (void *)((uintptr_t)dest+4);
      src  = (void *)((uintptr_t)src +4);
   }

   // copy trailing bytes
   int trailingBytes = numBytes & 3;
   if (trailingBytes)
   {
#ifdef MSB_FIRST
      src = (void *)((uintptr_t)src);
#else
      src = (void *)((uintptr_t)src ^ 3);
#endif
      for (int i = 0; i < trailingBytes; i++)
      {
         *(uint8_t *)(dest) = *(uint8_t *)(src);
         dest = (void *)((uintptr_t)dest+1);
         src  = (void *)((uintptr_t)src -1);
      }
   }
}

static inline void DWordInterleave( void *mem, uint32_t numDWords )
{
    int tmp;
    while( numDWords-- )
    {
        tmp = *(int *)((uintptr_t)mem + 0);
        *(int *)((uintptr_t)mem + 0) = *(int *)((uintptr_t)mem + 4);
        *(int *)((uintptr_t)mem + 4) = tmp;
        mem = (void *)((uintptr_t)mem + 8);
    }
}

static inline void QWordInterleave( void *mem, uint32_t numDWords )
{
   numDWords >>= 1; // qwords
   while( numDWords-- )
   {
      int tmp0, tmp1;
      tmp0 = *(int *)((uintptr_t)mem + 0);
      tmp1 = *(int *)((uintptr_t)mem + 4);
      *(int *)((uintptr_t)mem + 0) = *(int *)((uintptr_t)mem + 8);
      *(int *)((uintptr_t)mem + 8) = tmp0;
      *(int *)((uintptr_t)mem + 4) = *(int *)((uintptr_t)mem + 12);
      *(int *)((uintptr_t)mem + 12) = tmp1;
      mem = (void *)((uintptr_t)mem + 16);
   }
}

static void SetTmemFlag(uint32_t tmemAddr, uint32_t size)
{
    uint32_t index    = tmemAddr>>5;
    uint32_t bitIndex = (tmemAddr&0x1F);

    if( bitIndex == 0 )
    {
        uint32_t i;
        for( i=0; i< (size>>5); i++ )
        {
            g_TmemFlag[index+i] = 0;
        }

        if( (size&0x1F) != 0 )
        {
            //ErrorMsg("Check me: tmemaddr=%X, size=%x", tmemAddr, size);
            g_TmemFlag[index+i] &= ~((1<<(size&0x1F))-1);
        }

        g_TmemFlag[index] |= 1;
    }
    else
    {
        if( bitIndex + size <= 0x1F )
        {
            uint32_t val = g_TmemFlag[index];
            uint32_t mask = (1<<(bitIndex))-1;
            mask |= ~((1<<(bitIndex + size))-1);
            val &= mask;
            val |= (1<<bitIndex);
            g_TmemFlag[index] = val;
        }
        else
        {
            //ErrorMsg("Check me: tmemaddr=%X, size=%x", tmemAddr, size);
            uint32_t val = g_TmemFlag[index];
            uint32_t mask = (1<<bitIndex)-1;
            val &= mask;
            val |= (1<<bitIndex);
            g_TmemFlag[index] = val;
            index++;
            size -= (0x20-bitIndex);

            uint32_t i;
            for( i=0; i< (size>>5); i++ )
            {
                g_TmemFlag[index+i] = 0;
            }

            if( (size&0x1F) != 0 )
            {
                //ErrorMsg("Check me: tmemaddr=%X, size=%x", tmemAddr, size);
                g_TmemFlag[index+i] &= ~((1<<(size&0x1F))-1);
            }
        }
    }
}

void ricegDPSetScissor(void *data, 
      uint32_t mode, float ulx, float uly, float lrx, float lry )
{
   ScissorType *tempScissor = (ScissorType*)data;

   tempScissor->mode = mode;
   tempScissor->x0   = ulx;
   tempScissor->y0   = uly;
   tempScissor->x1   = lrx;
   tempScissor->y1   = lry;
}

void ricegDPSetFillColor(uint32_t c)
{
   DP_Timing(DLParser_SetFillColor);
   gRDP.fillColor = Convert555ToRGBA(c);
}

void ricegDPSetFogColor(uint32_t r, uint32_t g, uint32_t b, uint32_t a)
{
   DP_Timing(DLParser_SetFogColor);
   CRender::g_pRender->SetFogColor(r, g, b, a );
}

void ricegDPFillRect(int32_t ulx, int32_t uly, int32_t lrx, int32_t lry )
{
   uint8_t *rdram_u8 = (uint8_t*)gfx_info.RDRAM;

   /* Note, in some modes, the right/bottom lines aren't drawn */

   if( gRDP.otherMode.cycle_type >= G_CYC_COPY )
   {
      ++lrx;
      ++lry;
   }

   //TXTRBUF_DETAIL_DUMP(DebuggerAppendMsg("FillRect: X0=%d, Y0=%d, X1=%d, Y1=%d, Color=0x%08X", ulx, uly, lrx, lry, gRDP.originalFillColor););

   // Skip this
   if( status.bHandleN64RenderTexture && options.enableHackForGames == HACK_FOR_BANJO_TOOIE )
      return;

   if (IsUsedAsDI(g_CI.dwAddr))
   {
      // Clear the Z Buffer
      if( ulx != 0 || uly != 0 || windowSetting.uViWidth- lrx > 1 || windowSetting.uViHeight - lry > 1)
      {
         if( options.enableHackForGames == HACK_FOR_GOLDEN_EYE )
         {
            // GoldenEye is using double zbuffer
            if( g_CI.dwAddr == g_ZI.dwAddr )
            {
               // The zbuffer is the upper screen
               COORDRECT rect={int(ulx * windowSetting.fMultX),int(uly * windowSetting.fMultY),int(lrx * windowSetting.fMultX),int(lry * windowSetting.fMultY)};
               CRender::g_pRender->ClearBuffer(false,true,rect);   //Check me
               LOG_UCODE("    Clearing ZBuffer");
            }
            else
            {
               // The zbuffer is the lower screen
               int h = (g_CI.dwAddr-g_ZI.dwAddr)/g_CI.dwWidth/2;
               COORDRECT rect={int(ulx * windowSetting.fMultX),int((uly + h)*windowSetting.fMultY),int(lrx * windowSetting.fMultX),int((lry + h)*windowSetting.fMultY)};
               CRender::g_pRender->ClearBuffer(false,true,rect);   //Check me
               LOG_UCODE("    Clearing ZBuffer");
            }
         }
         else
         {
            COORDRECT rect={int(ulx * windowSetting.fMultX),int(uly * windowSetting.fMultY),int(lrx * windowSetting.fMultX),int(lry * windowSetting.fMultY)};
            CRender::g_pRender->ClearBuffer(false,true,rect);   //Check me
            LOG_UCODE("    Clearing ZBuffer");
         }
      }
      else
      {
         CRender::g_pRender->ClearBuffer(false,true);    //Check me
         LOG_UCODE("    Clearing ZBuffer");
      }

      if( g_curRomInfo.bEmulateClear )
      {
         // Emulating Clear, by write the memory in RDRAM
         uint16_t color = (uint16_t)gRDP.originalFillColor;
         uint32_t pitch = g_CI.dwWidth<<1;
         int64_t base   = (int64_t)(rdram_u8 + g_CI.dwAddr);

         for( uint32_t i = uly; i < lry; i++ )
         {
            for( uint32_t j= ulx; j < lrx; j++ )
               *(uint16_t*)((base+pitch*i+j)^2) = color;
         }
      }
   }
   else if( status.bHandleN64RenderTexture )
   {
      if( !status.bCIBufferIsRendered )
         g_pFrameBufferManager->ActiveTextureBuffer();

      int cond1 =  (((int)ulx < status.leftRendered)     ? ((int)ulx) : (status.leftRendered));
      int cond2 =  (((int)uly < status.topRendered)      ? ((int)uly) : (status.topRendered));
      int cond3 =  ((status.rightRendered > ((int)lrx))  ? ((int)lrx) : (status.rightRendered));
      int cond4 =  ((status.bottomRendered > ((int)lry)) ? ((int)lry) : (status.bottomRendered));
      int cond5 =  ((g_pRenderTextureInfo->maxUsedHeight > ((int)lry)) ? ((int)lry) : (g_pRenderTextureInfo->maxUsedHeight));

      status.leftRendered = status.leftRendered < 0   ? ulx  : cond1;
      status.topRendered = status.topRendered<0       ? uly : cond2;
      status.rightRendered = status.rightRendered<0   ? lrx : cond3;
      status.bottomRendered = status.bottomRendered<0 ? lry : cond4;

      g_pRenderTextureInfo->maxUsedHeight = cond5;

      if( status.bDirectWriteIntoRDRAM || ( ulx ==0 && uly == 0 && (lrx == g_pRenderTextureInfo->N64Width || lrx == g_pRenderTextureInfo->N64Width-1 ) ) )
      {
         if( g_pRenderTextureInfo->CI_Info.dwSize == G_IM_SIZ_16b )
         {
            uint16_t color = (uint16_t)gRDP.originalFillColor;
            uint32_t pitch = g_pRenderTextureInfo->N64Width<<1;
            int64_t base   = (int64_t)(rdram_u8 + g_pRenderTextureInfo->CI_Info.dwAddr);
            for( uint32_t i = uly; i < lry; i++ )
            {
               for( uint32_t j= ulx; j< lrx; j++ )
                  *(uint16_t*)((base+pitch*i+j)^2) = color;
            }
         }
         else
         {
            uint8_t color  = (uint8_t)gRDP.originalFillColor;
            uint32_t pitch = g_pRenderTextureInfo->N64Width;
            int64_t base   = (int64_t) (rdram_u8 + g_pRenderTextureInfo->CI_Info.dwAddr);
            for( uint32_t i= uly; i< lry; i++ )
            {
               for( uint32_t j= ulx; j< lrx; j++ )
                  *(uint8_t*)((base+pitch*i+j)^3) = color;
            }
         }

         status.bFrameBufferDrawnByTriangles = false;
      }
      else
      {
         status.bFrameBufferDrawnByTriangles = true;
      }
      status.bFrameBufferDrawnByTriangles = true;

      if( !status.bDirectWriteIntoRDRAM )
      {
         status.bFrameBufferIsDrawn = true;

         //if( ulx ==0 && uly ==0 && (lrx == g_pRenderTextureInfo->N64Width || lrx == g_pRenderTextureInfo->N64Width-1 ) && gRDP.fillColor == 0)
         //{
         //  CRender::g_pRender->ClearBuffer(true,false);
         //}
         //else
         {
            if( gRDP.otherMode.cycle_type == G_CYC_FILL )
            {
               CRender::g_pRender->FillRect(ulx, uly, lrx, lry, gRDP.fillColor);
            }
            else
            {
               COLOR primColor = GetPrimitiveColor();
               CRender::g_pRender->FillRect(ulx, uly, lrx, lry, primColor);
            }
         }
      }
   }
   else
   {
      LOG_UCODE("    Filling Rectangle");
      if( frameBufferOptions.bSupportRenderTextures || frameBufferOptions.bCheckBackBufs )
      {
         int cond1 =  (((int)ulx < status.leftRendered)     ? ((int)ulx) : (status.leftRendered));
         int cond2 =  (((int)uly < status.topRendered)      ? ((int)uly) : (status.topRendered));
         int cond3 =  ((status.rightRendered > ((int)lrx))  ? ((int)lrx) : (status.rightRendered));
         int cond4 =  ((status.bottomRendered > ((int)lry)) ? ((int)lry) : (status.bottomRendered));
         int cond5 =  ((g_pRenderTextureInfo->maxUsedHeight > ((int)lry)) ? ((int)lry) : (g_pRenderTextureInfo->maxUsedHeight));

         if( !status.bCIBufferIsRendered ) g_pFrameBufferManager->ActiveTextureBuffer();

         status.leftRendered = status.leftRendered<0     ? ulx  : cond1;
         status.topRendered = status.topRendered<0       ? uly  : cond2;
         status.rightRendered = status.rightRendered<0   ? lrx  : cond3;
         status.bottomRendered = status.bottomRendered<0 ? lry  : cond4;
      }

      if( gRDP.otherMode.cycle_type == G_CYC_FILL )
      {
         if( !status.bHandleN64RenderTexture || g_pRenderTextureInfo->CI_Info.dwSize == G_IM_SIZ_16b )
            CRender::g_pRender->FillRect(ulx, uly, lrx, lry, gRDP.fillColor);
      }
      else
      {
         COLOR primColor = GetPrimitiveColor();
         //if( RGBA_GETALPHA(primColor) != 0 )
         {
            CRender::g_pRender->FillRect(ulx, uly, lrx, lry, primColor);
         }
      }
   }
}

void ricegDPSetEnvColor(uint32_t r, uint32_t g, uint32_t b, uint32_t a)
{
   gRDP.colorsAreReloaded = true;
   gRDP.envColor = COLOR_RGBA(r, g, b, a); 
   gRDP.fvEnvColor[0] = r / 255.0f;
   gRDP.fvEnvColor[1] = g / 255.0f;
   gRDP.fvEnvColor[2] = b / 255.0f;
   gRDP.fvEnvColor[3] = a / 255.0f;
}

void ricegDPLoadBlock( uint32_t tileno, uint32_t uls, uint32_t ult,
      uint32_t lrs, uint32_t dxt )
{
   TileAdditionalInfo *tileinfo   = &gRDP.tilesinfo[tileno];
   gDPTile            *tile       = &gDP.tiles[tileno];

   tileinfo->bForceWrapS = tileinfo->bForceWrapT = tileinfo->bForceClampS = tileinfo->bForceClampT = false;

   uint32_t size     = lrs+1;

   if( tile->size == G_IM_SIZ_32b )
      size<<=1;

   SetTmemFlag(tile->tmem, size>>2);

   TMEMLoadMapInfo &info = g_tmemLoadAddrMap[tile->tmem];

   info.bSwapped = (dxt == 0? true : false);

   info.sl = tileinfo->hilite_sl = tile->lrs = uls;
   info.sh = tileinfo->hilite_sh  = tile->uls = lrs;
   info.tl = tile->lrt = ult;
   info.th = tile->ult = dxt;

   tileinfo->bSizeIsValid = false;

   for( int i=0; i<8; i++ )
   {
      if( gDP.tiles[i].tmem == tile->tmem )
         tileinfo->lastTileCmd = CMD_LOADBLOCK;
   }

   info.dwLoadAddress = g_TI.dwAddr;
   info.bSetBy        = CMD_LOADBLOCK;
   info.dxt           = dxt;
   info.dwLine        = tile->line;

   info.dwFormat      = g_TI.dwFormat;
   info.dwSize        = g_TI.dwSize;
   info.dwWidth       = g_TI.dwWidth;
   info.dwTotalWords  = size;
   info.dwTmem        = tile->tmem;

   if( gDP.tiles[tileno].tmem == 0 )
   {
      if( size >= 1024 )
      {
         memcpy(&g_tmemInfo0, &info, sizeof(TMEMLoadMapInfo) );
         g_tmemInfo0.dwTotalWords = size>>2;
      }

      if( size == 2048 )
      {
         memcpy(&g_tmemInfo1, &info, sizeof(TMEMLoadMapInfo) );
         g_tmemInfo1.dwTotalWords = size>>2;
      }
   }
   else if( tile->tmem == 0x100 )
   {
      if( size == 1024 )
      {
         memcpy(&g_tmemInfo1, &info, sizeof(TMEMLoadMapInfo) );
         g_tmemInfo1.dwTotalWords = size>>2;
      }
   }

   g_TxtLoadBy = CMD_LOADBLOCK;


   if( options.bUseFullTMEM )
   {
      uint8_t *rdram_u8 = (uint8_t*)gfx_info.RDRAM;
      uint32_t bytes = (lrs + 1) << tile->size >> 1;
      uint32_t address = g_TI.dwAddr + ult * g_TI.bpl + (uls << g_TI.dwSize >> 1);
      if ((bytes == 0) || ((address + bytes) > g_dwRamSize) || (((tile->tmem << 3) + bytes) > 4096))
      {
         return;
      }
      uint64_t* src = (uint64_t*)(rdram_u8 + address);
      uint64_t* dest = &g_Tmem.g_Tmem64bit[tile->tmem];

      if( dxt > 0)
      {
         void (*Interleave)( void *mem, uint32_t numDWords );

         uint32_t line = (2047 + dxt) / dxt;
         uint32_t bpl = line << 3;
         uint32_t height = bytes / bpl;

         if (tile->size == G_IM_SIZ_32b)
            Interleave = QWordInterleave;
         else
            Interleave = DWordInterleave;

         for (uint32_t y = 0; y < height; y++)
         {
            UnswapCopy( src, dest, bpl );
            if (y & 1) Interleave( dest, line );

            src += line;
            dest += line;
         }
      }
      else
      {
         UnswapCopy( src, dest, bytes );
      }
   }
}

void ricegDPLoadTile(uint32_t tileno, uint32_t uls, uint32_t ult,
      uint32_t lrs, uint32_t lrt)
{
   TileAdditionalInfo *tileinfo = &gRDP.tilesinfo[tileno];
   gDPTile                *tile = &gDP.tiles[tileno];
   tileinfo->bForceWrapS = tileinfo->bForceWrapT = tileinfo->bForceClampS = tileinfo->bForceClampT = false;

   if (lrt < ult)
      swapdword(&lrt, &ult);
   if (lrs < uls)
      swapdword(&lrs, &uls);

   tileinfo->hilite_sl = tile->lrs = uls;
   tileinfo->hilite_tl = tile->lrt = ult;
   tileinfo->hilite_sh = tile->uls = lrs;
   tileinfo->hilite_th = tile->ult = lrt;
   tileinfo->bSizeIsValid = true;

   // compute block height, and bpl of source and destination
   uint32_t bpl    = (lrs - uls + 1) << tile->size >> 1;
   uint32_t height = lrt - ult + 1;
   uint32_t line   = tile->line;
   if (tile->size == G_IM_SIZ_32b)
      line <<= 1;

   if (((tile->tmem << 3) + line * height) > 4096)  // check destination ending point (TMEM is 4k bytes)
      return;

   if( options.bUseFullTMEM )
   {
      uint8_t *rdram_u8 = (uint8_t*)gfx_info.RDRAM;
      void (*Interleave)( void *mem, uint32_t numDWords );

      if( g_TI.bpl == 0 )
      {
         if( options.enableHackForGames == HACK_FOR_BUST_A_MOVE )
         {
            g_TI.bpl = 1024;        // Hack for Bust-A-Move
         }
         else
         {
            TRACE0("Warning: g_TI.bpl = 0" );
         }
      }

      uint32_t address = g_TI.dwAddr + tile->lrt * g_TI.bpl + (tile->lrs << g_TI.dwSize >> 1);
      uint64_t* src = (uint64_t*)&rdram_u8[address];
      uint8_t* dest = (uint8_t*)&g_Tmem.g_Tmem64bit[tile->tmem];

      if ((address + height * bpl) > g_dwRamSize) // check source ending point
      {
         return;
      }

      // Line given for 32-bit is half what it seems it should since they split the
      // high and low words. I'm cheating by putting them together.
      if (tile->size == G_IM_SIZ_32b)
      {
         Interleave = QWordInterleave;
      }
      else
      {
         Interleave = DWordInterleave;
      }

      if( tile->line == 0 )
      {
         //tile->line = 1;
         return;
      }

      for (uint32_t y = 0; y < height; y++)
      {
         UnswapCopy( src, dest, bpl );
         if (y & 1) Interleave( dest, line );

         src += g_TI.bpl;
         dest += line;
      }
   }


   for( int i=0; i<8; i++ )
   {
      if( gDP.tiles[i].tmem == tile->tmem )
         gRDP.tilesinfo[i].lastTileCmd = CMD_LOADTILE;
   }

   uint32_t size = line * height;
   SetTmemFlag(tile->tmem, size );

   TMEMLoadMapInfo &info = g_tmemLoadAddrMap[tile->tmem];

   info.dwLoadAddress = g_TI.dwAddr;
   info.dwFormat = g_TI.dwFormat;
   info.dwSize = g_TI.dwSize;
   info.dwWidth = g_TI.dwWidth;

   info.sl = uls;
   info.sh = lrs;
   info.tl = ult;
   info.th = lrt;

   info.dxt = 0;
   info.dwLine = tile->line;
   info.dwTmem = tile->tmem;
   info.dwTotalWords = size<<2;

   info.bSetBy = CMD_LOADTILE;
   info.bSwapped =false;

   g_TxtLoadBy = CMD_LOADTILE;

   if( tile->tmem == 0 )
   {
      if( size >= 256 )
      {
         memcpy(&g_tmemInfo0, &info, sizeof(TMEMLoadMapInfo) );
         g_tmemInfo0.dwTotalWords = size;
      }

      if( size == 512 )
      {
         memcpy(&g_tmemInfo1, &info, sizeof(TMEMLoadMapInfo) );
         g_tmemInfo1.dwTotalWords = size;
      }
   }
   else if( tile->tmem == 0x100 )
   {
      if( size == 256 )
      {
         memcpy(&g_tmemInfo1, &info, sizeof(TMEMLoadMapInfo) );
         g_tmemInfo1.dwTotalWords = size;
      }
   }
}

void ricegDPLoadTLUT(uint16_t dwCount, uint32_t tileno, uint32_t uls, uint32_t ult, uint32_t lrs, uint32_t lrt)
{
   uint8_t *rdram_u8      = (uint8_t*)gfx_info.RDRAM;
   /* Starting location in the palettes */
   uint32_t dwTMEMOffset  = gDP.tiles[tileno].tmem - 256;  
   /* Number to copy */
   uint32_t dwRDRAMOffset = 0;

   TileAdditionalInfo *tileinfo = &gRDP.tilesinfo[tileno];
   gDPTile            *tile     = &gDP.tiles[tileno];

   tileinfo->bForceWrapS = tileinfo->bForceWrapT = tileinfo->bForceClampS = tileinfo->bForceClampT = false;

   tileinfo->hilite_sl = tile->lrs = uls;
   tileinfo->hilite_tl = tile->lrt = ult;
   tile->uls = lrs;
   tile->ult = lrt;
   tileinfo->bSizeIsValid = true;

   tileinfo->lastTileCmd = CMD_LOADTLUT;

   dwCount = (lrs - uls)+1;
   dwRDRAMOffset = (uls + ult*g_TI.dwWidth )*2;
   uint32_t dwPalAddress = g_TI.dwAddr + dwRDRAMOffset;

   /* Copy PAL to the PAL memory */
   uint16_t *srcPal = (uint16_t*)(rdram_u8 + (dwPalAddress& (g_dwRamSize-1)) );
   for (uint32_t i=0; i<dwCount && i<0x100; i++)
      g_wRDPTlut[(i+dwTMEMOffset)^1] = srcPal[i^1];

   if( options.bUseFullTMEM )
   {
      for (uint32_t i=0; i<dwCount && i+tile->tmem < 0x200; i++)
         *(uint16_t*)(&g_Tmem.g_Tmem64bit[tile->tmem + i]) = srcPal[i^1];
   }

   extern bool RevTlutTableNeedUpdate;
   RevTlutTableNeedUpdate = true;
   g_TxtLoadBy = CMD_LOADTLUT;
}
