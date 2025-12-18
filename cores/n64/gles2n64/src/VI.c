#include <math.h>

#include "Common.h"
#include "gles2N64.h"
#include "VI.h"
#include "OpenGL.h"
#include "N64.h"
#include "gSP.h"
#include "gDP.h"
#include "RSP.h"
#include "Debug.h"
#include "Config.h"
#include "FrameBuffer.h"

#include "../../Graphics/RDP/gDP_state.h"

VIInfo VI;

void VI_UpdateSize(void)
{
   struct FrameBuffer *pBuffer, *pDepthBuffer;
	const bool interlacedPrev = VI.interlaced;
   float xScale  = _FIXED2FLOAT( _SHIFTR( *gfx_info.VI_X_SCALE_REG, 0, 12 ), 10 );
	uint32_t vScale  = _SHIFTR(*gfx_info.VI_Y_SCALE_REG, 0, 12);

   uint32_t hEnd = _SHIFTR( *gfx_info.VI_H_START_REG, 0, 10 );
   uint32_t hStart = _SHIFTR( *gfx_info.VI_H_START_REG, 16, 10 );

   // These are in half-lines, so shift an extra bit
   uint32_t vEnd = _SHIFTR( *gfx_info.VI_V_START_REG, 0, 10 );
   uint32_t vStart = _SHIFTR( *gfx_info.VI_V_START_REG, 16, 10 );
	if (VI.width > 0)
		VI.widthPrev = VI.width;

	VI.real_height = vEnd > vStart ? (((vEnd - vStart) >> 1) * vScale) >> 10 : 0;
   VI.width = *gfx_info.VI_WIDTH_REG;
	VI.interlaced = (*gfx_info.VI_STATUS_REG & 0x40) != 0;
	if (VI.interlaced)
   {
		float fullWidth = 640.0f * xScale;
		if (*gfx_info.VI_WIDTH_REG > fullWidth)
      {
			const uint32_t scale = (uint32_t)floorf(*gfx_info.VI_WIDTH_REG / fullWidth + 0.5f);
			VI.width /= scale;
			VI.real_height *= scale;
		}
		if (VI.real_height % 2 == 1)
			--VI.real_height;
	}

	VI.PAL = (*gfx_info.VI_V_SYNC_REG & 0x3ff) > 550;
	if (VI.PAL && (vEnd - vStart) > 478)
   {
		VI.height = (uint32_t)(VI.real_height*1.0041841f);
		if (VI.height > 576)
			VI.height = VI.real_height = 576;
	}
	else
   {
		VI.height = (uint32_t)(VI.real_height*1.0126582f);
		if (VI.height > 480)
			VI.height = VI.real_height = 480;
	}
	if (VI.height % 2 == 1)
		--VI.height;

   pBuffer = FrameBuffer_FindBuffer(VI.lastOrigin);
   pDepthBuffer = (pBuffer != NULL) ? NULL /* pBuffer->DepthBuffer */: NULL;

	if (config.frameBufferEmulation.enable &&
		((interlacedPrev != VI.interlaced) ||
		(VI.width > 0 && VI.width != VI.widthPrev) ||
		(!VI.interlaced && pDepthBuffer != NULL && pDepthBuffer->m_width != VI.width) ||
		(pBuffer != NULL && pBuffer->m_height != VI.height))
	)
   {
      FrameBuffer_RemoveBuffer(VI.widthPrev);
		FrameBuffer_RemoveBuffer(VI.width);
#ifdef NEW
		depthBufferList().destroy();
		depthBufferList().init();
#endif
	}

	VI.rwidth = VI.width != 0 ? 1.0f / VI.width : 0.0f;
	VI.rheight = VI.height != 0 ? 1.0f / VI.height : 0.0f;
}

void VI_UpdateScreen(void)
{
	static uint32_t uNumCurFrameIsShown = 0;
   bool bVIUpdated = false;

   if (*gfx_info.VI_ORIGIN_REG != VI.lastOrigin)
   {
      VI_UpdateSize();
      bVIUpdated = true;
      OGL_UpdateScale();
   }

	if (config.frameBufferEmulation.enable)
   {
		const bool bCFB = config.frameBufferEmulation.detectCFB != 0 && (gSP.changed&CHANGED_CPU_FB_WRITE) == CHANGED_CPU_FB_WRITE;
		const bool bNeedUpdate = gDP.colorImage.changed != 0 || (bCFB ? true : (*gfx_info.VI_ORIGIN_REG != VI.lastOrigin));

		if (bNeedUpdate)
      {
			if ((gSP.changed&CHANGED_CPU_FB_WRITE) == CHANGED_CPU_FB_WRITE)
         {
				struct FrameBuffer * pBuffer = FrameBuffer_FindBuffer(*gfx_info.VI_ORIGIN_REG);
				if (pBuffer == NULL || pBuffer->m_width != VI.width)
            {
               uint32_t size;

					if (!bVIUpdated)
               {
						VI_UpdateSize();
						OGL_UpdateScale();
						bVIUpdated = true;
					}
					size = *gfx_info.VI_STATUS_REG & 3;

					if (VI.height > 0 && size > G_IM_SIZ_8b  && VI.width > 0)
						FrameBuffer_SaveBuffer(*gfx_info.VI_ORIGIN_REG, G_IM_FMT_RGBA, size, VI.width, VI.height, true);
				}
			}
			if ((((*gfx_info.VI_STATUS_REG) & 3) > 0) && ((config.frameBufferEmulation.copyFromRDRAM && gDP.colorImage.changed) || bCFB))
         {
            if (!bVIUpdated)
            {
               VI_UpdateSize();
               bVIUpdated = true;
            }
            FrameBuffer_CopyFromRDRAM(*gfx_info.VI_ORIGIN_REG, config.frameBufferEmulation.copyFromRDRAM && !bCFB);
         }

			FrameBuffer_RenderBuffer(*gfx_info.VI_ORIGIN_REG);

			if (gDP.colorImage.changed)
				uNumCurFrameIsShown = 0;
			else
         {
            uNumCurFrameIsShown++;
            if (uNumCurFrameIsShown > 25)
               gSP.changed |= CHANGED_CPU_FB_WRITE;
         }
#if 0
         /* TODO/FIXME - implement */
			frameBufferList().clearBuffersChanged();
#endif
			VI.lastOrigin = *gfx_info.VI_ORIGIN_REG;
#ifdef DEBUG
			while (Debug.paused && !Debug.step);
			Debug.step = false;
#endif
		} else {
			uNumCurFrameIsShown++;
			if (uNumCurFrameIsShown > 25)
				gSP.changed |= CHANGED_CPU_FB_WRITE;
		}
	}
   else
   {
      if (gSP.changed & CHANGED_COLORBUFFER)
      {
         OGL_SwapBuffers();
         gSP.changed &= ~CHANGED_COLORBUFFER;
         VI.lastOrigin = *gfx_info.VI_ORIGIN_REG;
      }
   }
}
