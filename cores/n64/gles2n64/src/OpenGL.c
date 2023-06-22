#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif

#include <boolean.h>
#include <retro_miscellaneous.h>

#include "Common.h"
#include "gles2N64.h"
#include "OpenGL.h"
#include "N64.h"
#include "gSP.h"
#include "gDP.h"
#include "Textures.h"
#include "FrameBuffer.h"
#include "ShaderCombiner.h"
#include "VI.h"
#include "RSP.h"
#include "Config.h"

#include "../../Graphics/RDP/gDP_state.h"
#include "../../Graphics/RSP/gSP_state.h"

GLInfo OGL;

static bool m_bFlatColors;

int OGL_IsExtSupported( const char *extension )
{
   const GLubyte *extensions = NULL;
   const GLubyte *start;
   GLubyte *where, *terminator;

   where = (GLubyte *) strchr(extension, ' ');
   if (where || *extension == '\0')
      return 0;

   extensions = glGetString(GL_EXTENSIONS);

   if (!extensions) return 0;

   start = extensions;
   for (;;)
   {
      where = (GLubyte *) strstr((const char *) start, extension);
      if (!where)
         break;

      terminator = where + strlen(extension);
      if (where == start || *(where - 1) == ' ')
         if (*terminator == ' ' || *terminator == '\0')
            return 1;

      start = terminator;
   }

   return 0;
}

static void _initStates(void)
{
   glDisable( GL_CULL_FACE );
   glEnableVertexAttribArray( SC_POSITION );
   glEnable( GL_DEPTH_TEST );
   glDepthFunc( GL_ALWAYS );
   glDepthMask( GL_FALSE );
   glEnable( GL_SCISSOR_TEST );

   glDepthRange(0.0f, 1.0f);
   glPolygonOffset(-0.2f, -0.2f);
   glViewport(0, OGL_GetHeightOffset(), OGL_GetScreenWidth(), OGL_GetScreenHeight());

   glClearColor( 0.0f, 0.0f, 0.0f, 0.0f);
   glClear( GL_COLOR_BUFFER_BIT);

   srand( time(NULL) );
}

void OGL_UpdateScale(void)
{
   if (VI.width == 0 || VI.height == 0)
      return;
   OGL.scaleX = OGL_GetScreenWidth()  / (float)VI.width;
   OGL.scaleY = OGL_GetScreenHeight() / (float)VI.height;
}

uint32_t OGL_GetScreenWidth(void)
{
   return config.screen.width;
}

uint32_t OGL_GetScreenHeight(void)
{
   return config.screen.height;
}

float OGL_GetScaleX(void)
{
   return OGL.scaleX;
}

float OGL_GetScaleY(void)
{
   return OGL.scaleY;
}

uint32_t OGL_GetHeightOffset(void)
{
   return OGL.heightOffset;
}



void OGL_Stop(void)
{
   LOG(LOG_MINIMAL, "Stopping OpenGL\n");

   Combiner_Destroy();
   TextureCache_Destroy();
}

static void _updateCullFace(void)
{
   if (gSP.geometryMode & G_CULL_BOTH)
   {
      glEnable( GL_CULL_FACE );

      if (gSP.geometryMode & G_CULL_BACK)
         glCullFace(GL_BACK);
      else
         glCullFace(GL_FRONT);
   }
   else
      glDisable(GL_CULL_FACE);
}

/* TODO/FIXME - not complete yet */
static void _updateViewport(void)
{
   const uint32_t VI_height = VI.height;
   const float scaleX = OGL_GetScaleX();
   const float scaleY = OGL_GetScaleY();
   float Xf = gSP.viewport.vscale[0] < 0 ? (gSP.viewport.x + gSP.viewport.vscale[0] * 2.0f) : gSP.viewport.x;
   const GLint X = (GLint)(Xf * scaleX);
   const GLint Y = gSP.viewport.vscale[1] < 0 ? (GLint)((gSP.viewport.y + gSP.viewport.vscale[1] * 2.0f) * scaleY) : (GLint)((VI_height - (gSP.viewport.y + gSP.viewport.height)) * scaleY);

   glViewport(X,
         Y + OGL_GetHeightOffset(),
         MAX((GLint)(gSP.viewport.width * scaleX), 0),
         MAX((GLint)(gSP.viewport.height * scaleY), 0));

	gSP.changed &= ~CHANGED_VIEWPORT;
}

static void _updateDepthUpdate(void)
{
   if (gDP.otherMode.depthUpdate)
      glDepthMask(GL_TRUE);
   else
      glDepthMask(GL_FALSE);
}

static void _updateScissor(struct FrameBuffer *_pBuffer)
{
   uint32_t heightOffset, screenHeight;
   float scaleX, scaleY;
   float SX0 = gDP.scissor.ulx;
   float SX1 = gDP.scissor.lrx;

   if (_pBuffer == NULL)
   {
      scaleX       = OGL_GetScaleX();
      scaleY       = OGL_GetScaleY();
      heightOffset = OGL_GetHeightOffset();
      screenHeight = VI.height;
   }
   else
   {
      scaleX       = _pBuffer->m_scaleX;
      scaleY       = _pBuffer->m_scaleY;
      heightOffset = 0;
      screenHeight = (_pBuffer->m_height == 0) ? VI.height : _pBuffer->m_height;
   }

#if 0
	if (ogl.isAdjustScreen() && gSP.viewport.width < gDP.colorImage.width && gDP.colorImage.width > VI.width * 98 / 100)
		_adjustScissorX(SX0, SX1, ogl.getAdjustScale());
#endif

   glScissor(
         (GLint)(SX0 * scaleX),
         (GLint)((screenHeight - gDP.scissor.lry) * scaleY + heightOffset),
         MAX((GLint)((SX1 - gDP.scissor.ulx) * scaleX), 0),
         MAX((GLint)((gDP.scissor.lry - gDP.scissor.uly) * scaleY), 0)
         );
	gDP.changed &= ~CHANGED_SCISSOR;
}

static void _setBlendMode(void)
{
	const uint32_t blendmode = gDP.otherMode.l >> 16;
	// 0x7000 = CVG_X_ALPHA|ALPHA_CVG_SEL|FORCE_BL
	if (gDP.otherMode.alphaCvgSel != 0 && (gDP.otherMode.l & 0x7000) != 0x7000) {
		switch (blendmode) {
		case 0x4055: // Mario Golf
		case 0x5055: // Paper Mario intro clr_mem * a_in + clr_mem * a_mem
			glEnable(GL_BLEND);
			glBlendFunc(GL_ZERO, GL_ONE);
			break;
		default:
			glDisable(GL_BLEND);
		}
		return;
	}

	if (gDP.otherMode.forceBlender != 0 && gDP.otherMode.cycleType < G_CYC_COPY) {
		glEnable( GL_BLEND );

		switch (blendmode)
		{
			// Mace objects
			case 0x0382:
			// Mace special blend mode, see GLSLCombiner.cpp
			case 0x0091:
			// 1080 Sky
			case 0x0C08:
			// Used LOTS of places
			case 0x0F0A:
			//DK64 blue prints
			case 0x0302:
			// Bomberman 2 special blend mode, see GLSLCombiner.cpp
			case 0xA500:
			//Sin and Punishment
			case 0xCB02:
			// Battlezone
			// clr_in * a + clr_in * (1-a)
			case 0xC800:
			// Conker BFD
			// clr_in * a_fog + clr_fog * (1-a)
			// clr_in * 0 + clr_in * 1
			case 0x07C2:
			case 0x00C0:
			//ISS64
			case 0xC302:
			// Donald Duck
			case 0xC702:
				glBlendFunc(GL_ONE, GL_ZERO);
				break;

			case 0x0F1A:
				if (gDP.otherMode.cycleType == G_CYC_1CYCLE)
					glBlendFunc(GL_ONE, GL_ZERO);
				else
					glBlendFunc(GL_ZERO, GL_ONE);
				break;

			//Space Invaders
			case 0x0448: // Add
			case 0x055A:
				glBlendFunc( GL_ONE, GL_ONE );
				break;

			case 0xc712: // Pokemon Stadium?
			case 0xAF50: // LOT in Zelda: MM
			case 0x0F5A: // LOT in Zelda: MM
			case 0x0FA5: // Seems to be doing just blend color - maybe combiner can be used for this?
			case 0x5055: // Used in Paper Mario intro, I'm not sure if this is right...
				//clr_in * 0 + clr_mem * 1
				glBlendFunc( GL_ZERO, GL_ONE );
				break;

			case 0x5F50: //clr_mem * 0 + clr_mem * (1-a)
				glBlendFunc( GL_ZERO, GL_ONE_MINUS_SRC_ALPHA );
				break;

			case 0xF550: //clr_fog * a_fog + clr_mem * (1-a)
			case 0x0150: // spiderman
			case 0x0550: // bomberman 64
			case 0x0D18: //clr_in * a_fog + clr_mem * (1-a)
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				break;

			case 0xC912: //40 winks, clr_in * a_fog + clr_mem * 1
				glBlendFunc(GL_SRC_ALPHA, GL_ONE);
				break;

			case 0x0040: // Fzero
			case 0xC810: // Blends fog
			case 0xC811: // Blends fog
			case 0x0C18: // Standard interpolated blend
			case 0x0C19: // Used for antialiasing
			case 0x0050: // Standard interpolated blend
			case 0x0051: // Standard interpolated blend
			case 0x0055: // Used for antialiasing
				glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
				break;

         case 0x5000: // V8 explosions
            glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
            break;

			default:
				LOG(LOG_VERBOSE, "Unhandled blend mode=%x", gDP.otherMode.l >> 16);
				glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
				break;
		}
	}
   else if ((config.generalEmulation.hacks & hack_pilotWings) != 0 && (gDP.otherMode.l & 0x80) != 0) { //CLR_ON_CVG without FORCE_BL
		glEnable(GL_BLEND);
		glBlendFunc(GL_ZERO, GL_ONE);
	}
   /* TODO/FIXME: update */
   else if ((config.generalEmulation.hacks & hack_blastCorps) != 0 && gSP.texture.on == 0 /* && currentCombiner()->usesTex() */)
   {
      // Blast Corps
		glEnable(GL_BLEND);
		glBlendFunc(GL_ZERO, GL_ONE);
	}
   else
   {
		glDisable( GL_BLEND );
	}
}

static void _updateStates(void)
{
   if (gDP.otherMode.cycleType == G_CYC_COPY)
      Combiner_Set(EncodeCombineMode(0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0), -1);
   else if (gDP.otherMode.cycleType == G_CYC_FILL)
      Combiner_Set(EncodeCombineMode(0, 0, 0, SHADE, 0, 0, 0, 1, 0, 0, 0, SHADE, 0, 0, 0, 1), -1);
   else
      Combiner_Set(gDP.combine.mux, -1);

   if (gSP.changed & CHANGED_GEOMETRYMODE)
   {
      _updateCullFace();
		gSP.changed &= ~CHANGED_GEOMETRYMODE;
   }

   if (gDP.changed & CHANGED_RENDERMODE || gDP.changed & CHANGED_CYCLETYPE)
   {
      if (((gSP.geometryMode & G_ZBUFFER) || gDP.otherMode.depthSource == G_ZS_PRIM) && gDP.otherMode.cycleType <= G_CYC_2CYCLE)
      {
         if (gDP.otherMode.depthCompare != 0)
         {
            switch (gDP.otherMode.depthMode)
            {
               case ZMODE_OPA:
                  glDisable(GL_POLYGON_OFFSET_FILL);
                  glDepthFunc(GL_LEQUAL);
                  break;
               case ZMODE_INTER:
                  glDisable(GL_POLYGON_OFFSET_FILL);
                  glDepthFunc(GL_LEQUAL);
                  break;
               case ZMODE_XLU:
                  // Max || Infront;
                  glDisable(GL_POLYGON_OFFSET_FILL);
                  if (gDP.otherMode.depthSource == G_ZS_PRIM && gDP.primDepth.z == 1.0f)
                     // Max
                     glDepthFunc(GL_LEQUAL);
                  else
                     // Infront
                     glDepthFunc(GL_LESS);
                  break;
               case ZMODE_DEC:
                  glEnable(GL_POLYGON_OFFSET_FILL);
                  glDepthFunc(GL_LEQUAL);
                  break;
            }
         }
         else
         {
            glDisable(GL_POLYGON_OFFSET_FILL);
            glDepthFunc(GL_ALWAYS);
         }

         _updateDepthUpdate();

         glEnable(GL_DEPTH_TEST);
#ifdef NEW
         if (!GBI.isNoN())
            glDisable(GL_DEPTH_CLAMP);
#endif
      }
      else
      {
         glDisable(GL_DEPTH_TEST);
#ifdef NEW
         if (!GBI.isNoN())
            glEnable(GL_DEPTH_CLAMP);
#endif
      }
   }

   if ((gDP.changed & CHANGED_RENDERMODE))
      SC_SetUniform1f(uAlphaRef, (gDP.otherMode.cvgXAlpha) ? 0.5f : gDP.blendColor.a);

   if (gDP.changed & CHANGED_SCISSOR)
      _updateScissor(FrameBuffer_GetCurrent());

   if (gSP.changed & CHANGED_VIEWPORT)
      _updateViewport();

	if (gSP.changed & CHANGED_LIGHT)
		ShaderCombiner_UpdateLightParameters();

   if (gSP.changed & CHANGED_FOGPOSITION)
   {
      SC_SetUniform1f(uFogScale, (float) gSP.fog.multiplier / 255.0f);
      SC_SetUniform1f(uFogOffset, (float) gSP.fog.offset / 255.0f);
   }

   if ((gSP.changed & CHANGED_TEXTURE) || (gDP.changed & CHANGED_TILE) || (gDP.changed & CHANGED_TMEM))
   {
      //For some reason updating the texture cache on the first frame of LOZ:OOT causes a NULL Pointer exception...
      if (scProgramCurrent)
      {
         if (scProgramCurrent->usesT0)
         {
            TextureCache_Update(0);
            SC_ForceUniform2f(uTexOffset[0], gSP.textureTile[0]->fuls, gSP.textureTile[0]->fult);
            SC_ForceUniform2f(uCacheShiftScale[0], cache.current[0]->shiftScaleS, cache.current[0]->shiftScaleT);
            SC_ForceUniform2f(uCacheScale[0], cache.current[0]->scaleS, cache.current[0]->scaleT);
            SC_ForceUniform2f(uCacheOffset[0], cache.current[0]->offsetS, cache.current[0]->offsetT);
         }
         //else TextureCache_ActivateDummy(0);

         //Note: enabling dummies makes some F-zero X textures flicker.... strange.

         if (scProgramCurrent->usesT1)
         {
            TextureCache_Update(1);
            SC_ForceUniform2f(uTexOffset[1], gSP.textureTile[1]->fuls, gSP.textureTile[1]->fult);
            SC_ForceUniform2f(uCacheShiftScale[1], cache.current[1]->shiftScaleS, cache.current[1]->shiftScaleT);
            SC_ForceUniform2f(uCacheScale[1], cache.current[1]->scaleS, cache.current[1]->scaleT);
            SC_ForceUniform2f(uCacheOffset[1], cache.current[1]->offsetS, cache.current[1]->offsetT);
         }
         //else TextureCache_ActivateDummy(1);
      }
   }

   if ((gDP.changed & (CHANGED_RENDERMODE | CHANGED_CYCLETYPE)))
   {
      _setBlendMode();
      gDP.changed &= ~(CHANGED_RENDERMODE | CHANGED_CYCLETYPE);
   }

   gDP.changed &= CHANGED_TILE | CHANGED_TMEM;
   gSP.changed &= CHANGED_TEXTURE | CHANGED_MATRIX;
}

void OGL_DrawTriangle(struct SPVertex *vertices, int v0, int v1, int v2)
{
}

static void OGL_SetColorArray(void)
{
   if (scProgramCurrent->usesCol || gDP.otherMode.c1_m1b == 2)
      glEnableVertexAttribArray(SC_COLOR);
   else
      glDisableVertexAttribArray(SC_COLOR);
}

static void OGL_SetTexCoordArrays(void)
{
   if (OGL.renderState == RS_TRIANGLE)
   {
      glDisableVertexAttribArray(SC_TEXCOORD1);
      if (scProgramCurrent->usesT0 || scProgramCurrent->usesT1)
         glEnableVertexAttribArray(SC_TEXCOORD0);
      else
         glDisableVertexAttribArray(SC_TEXCOORD0);
   }
   else
   {
      if (scProgramCurrent->usesT0)
         glEnableVertexAttribArray(SC_TEXCOORD0);
      else
         glDisableVertexAttribArray(SC_TEXCOORD0);

      if (scProgramCurrent->usesT1)
         glEnableVertexAttribArray(SC_TEXCOORD1);
      else
         glDisableVertexAttribArray(SC_TEXCOORD1);
   }
}

static void OGL_prepareDrawTriangle(bool _dma)
{
   bool updateColorArrays, updateArrays;
   if (gSP.changed || gDP.changed)
      _updateStates();

   updateArrays = OGL.renderState != RS_TRIANGLE;

   if (updateArrays || scProgramChanged)
   {
      OGL.renderState = RS_TRIANGLE;
      OGL_SetColorArray();
      OGL_SetTexCoordArrays();
      glDisableVertexAttribArray(SC_TEXCOORD1);
      SC_ForceUniform1f(uRenderState, RS_TRIANGLE);
   }

   updateColorArrays = m_bFlatColors != (!__RSP.bLLE && (gSP.geometryMode & G_SHADING_SMOOTH) == 0);
   if (updateColorArrays)
      m_bFlatColors = !m_bFlatColors;

   if (updateArrays)
   {
      struct SPVertex *pVtx = (struct SPVertex*)&OGL.triangles.vertices[0];
      glVertexAttribPointer(SC_POSITION, 4, GL_FLOAT, GL_FALSE, sizeof(struct SPVertex), &pVtx->x);
      if (m_bFlatColors)
         glVertexAttribPointer(SC_COLOR, 4, GL_FLOAT, GL_FALSE, sizeof(struct SPVertex), &pVtx->flat_r);
      else
         glVertexAttribPointer(SC_COLOR, 4, GL_FLOAT, GL_FALSE, sizeof(struct SPVertex), &pVtx->r);
      glVertexAttribPointer(SC_TEXCOORD0, 2, GL_FLOAT, GL_FALSE, sizeof(struct SPVertex), &pVtx->s);
   }
   else if (updateColorArrays)
   {
      struct SPVertex *pVtx = (struct SPVertex*)&OGL.triangles.vertices[0];
      if (m_bFlatColors)
         glVertexAttribPointer(SC_COLOR, 4, GL_FLOAT, GL_FALSE, sizeof(struct SPVertex), &pVtx->flat_r);
      else
         glVertexAttribPointer(SC_COLOR, 4, GL_FLOAT, GL_FALSE, sizeof(struct SPVertex), &pVtx->r);
   }
}

void OGL_DrawLLETriangle(uint32_t _numVtx)
{
   struct FrameBuffer * pCurrentBuffer;
   float scaleX, scaleY;
   uint32_t i;
	if (_numVtx == 0)
		return;

	gSP.changed &= ~CHANGED_GEOMETRYMODE; // Don't update cull mode
	OGL_prepareDrawTriangle(false);
	glDisable(GL_CULL_FACE);

	pCurrentBuffer = FrameBuffer_GetCurrent();

	if (pCurrentBuffer == NULL)
   {
      glViewport( 0, OGL_GetHeightOffset(), OGL_GetScreenWidth(), OGL_GetScreenHeight());
   }
	else
		glViewport(0, 0, pCurrentBuffer->m_width * pCurrentBuffer->m_scaleX, pCurrentBuffer->m_height * pCurrentBuffer->m_scaleY);

	scaleX = pCurrentBuffer != NULL ? 1.0f / pCurrentBuffer->m_width : VI.rwidth;
	scaleY = pCurrentBuffer != NULL ? 1.0f / pCurrentBuffer->m_height : VI.rheight;

	for (i = 0; i < _numVtx; ++i)
   {
      struct SPVertex *vtx = (struct SPVertex*)&OGL.triangles.vertices[i];

      vtx->HWLight = 0;
      vtx->x  = vtx->x * (2.0f * scaleX) - 1.0f;
      vtx->x *= vtx->w;
      vtx->y  = vtx->y * (-2.0f * scaleY) + 1.0f;
      vtx->y *= vtx->w;
      vtx->z *= vtx->w;

      if (gDP.otherMode.texturePersp == 0)
      {
         vtx->s *= 2.0f;
         vtx->t *= 2.0f;
      }
   }

	glDrawArrays(GL_TRIANGLE_STRIP, 0, _numVtx);
	OGL.triangles.num = 0;

#if 0
	frameBufferList().setBufferChanged();
#endif
	gSP.changed |= CHANGED_VIEWPORT | CHANGED_GEOMETRYMODE;
}

void OGL_AddTriangle(int v0, int v1, int v2)
{
   uint32_t i;
   struct SPVertex *vtx = NULL;

   OGL.triangles.elements[OGL.triangles.num++] = v0;
   OGL.triangles.elements[OGL.triangles.num++] = v1;
   OGL.triangles.elements[OGL.triangles.num++] = v2;

	if ((gSP.geometryMode & G_SHADE) == 0)
   {
      /* Prim shading */
      for (i = OGL.triangles.num - 3; i < OGL.triangles.num; ++i)
      {
         vtx = (struct SPVertex*)&OGL.triangles.vertices[OGL.triangles.elements[i]];
         vtx->flat_r = gDP.primColor.r;
         vtx->flat_g = gDP.primColor.g;
         vtx->flat_b = gDP.primColor.b;
         vtx->flat_a = gDP.primColor.a;
      }
   }
   else if ((gSP.geometryMode & G_SHADING_SMOOTH) == 0)
   {
      /* Flat shading */
      struct SPVertex *vtx0 = (struct SPVertex*)&OGL.triangles.vertices[v0];

      for (i = OGL.triangles.num - 3; i < OGL.triangles.num; ++i)
      {
         vtx = (struct SPVertex*)&OGL.triangles.vertices[OGL.triangles.elements[i]];
         vtx->flat_r = vtx0->r;
         vtx->flat_g = vtx0->g;
         vtx->flat_b = vtx0->b;
         vtx->flat_a = vtx0->a;
      }
   }

	if (gDP.otherMode.depthSource == G_ZS_PRIM)
   {
		for (i = OGL.triangles.num - 3; i < OGL.triangles.num; ++i)
      {
			vtx = (struct SPVertex*)&OGL.triangles.vertices[OGL.triangles.elements[i]];
			vtx->z = gDP.primDepth.z * vtx->w;
		}
	}
}

void OGL_DrawDMATriangles(uint32_t _numVtx)
{
   if (_numVtx == 0)
      return;

   OGL_prepareDrawTriangle(true);
	glDrawArrays(GL_TRIANGLES, 0, _numVtx);
}

void OGL_DrawTriangles(void)
{
   if (OGL.triangles.num == 0)
      return;

   OGL_prepareDrawTriangle(false);

   glDrawElements(GL_TRIANGLES, OGL.triangles.num, GL_UNSIGNED_BYTE, OGL.triangles.elements);
   OGL.triangles.num = 0;
}

void OGL_DrawLine(int v0, int v1, float width )
{
   unsigned short elem[2];

   if (gSP.changed || gDP.changed)
      _updateStates();

   if (OGL.renderState != RS_LINE || scProgramChanged)
   {
      OGL_SetColorArray();
      glDisableVertexAttribArray(SC_TEXCOORD0);
      glDisableVertexAttribArray(SC_TEXCOORD1);
      glVertexAttribPointer(SC_POSITION, 4, GL_FLOAT, GL_FALSE, sizeof(struct SPVertex), &OGL.triangles.vertices[0].x);
      glVertexAttribPointer(SC_COLOR, 4, GL_FLOAT, GL_FALSE, sizeof(struct SPVertex), &OGL.triangles.vertices[0].r);

      SC_ForceUniform1f(uRenderState, RS_LINE);
      _updateCullFace();
      _updateViewport();
      OGL.renderState = RS_LINE;
   }

   elem[0] = v0;
   elem[1] = v1;
   glLineWidth( width * OGL.scaleX );
   glDrawElements(GL_LINES, 2, GL_UNSIGNED_SHORT, elem);
}

void OGL_DrawRect( int ulx, int uly, int lrx, int lry, float *color)
{
   struct FrameBuffer *pCurrentBuffer;
   float scaleX, scaleY, Z, W;
   bool updateArrays;

   gSP.changed &= ~CHANGED_GEOMETRYMODE; // Don't update cull mode
   if (gSP.changed || gDP.changed)
      _updateStates();

   updateArrays = OGL.renderState != RS_RECT;

   if (updateArrays || scProgramChanged)
   {
      glDisableVertexAttribArray(SC_COLOR);
      glDisableVertexAttribArray(SC_TEXCOORD0);
      glDisableVertexAttribArray(SC_TEXCOORD1);
      SC_ForceUniform1f(uRenderState, RS_RECT);
   }

   if (updateArrays)
   {
      glVertexAttrib4f(SC_POSITION, 0, 0, gSP.viewport.nearz, 1.0);
      glVertexAttribPointer(SC_POSITION, 2, GL_FLOAT, GL_FALSE, sizeof(GLVertex), &OGL.rect[0].x);
      OGL.renderState = RS_RECT;
   }

   pCurrentBuffer = FrameBuffer_GetCurrent();
   if (pCurrentBuffer == NULL)
      glViewport(0, OGL_GetHeightOffset(), OGL_GetScreenWidth(), OGL_GetScreenHeight());
   else
      glViewport(0, 0, pCurrentBuffer->m_width * pCurrentBuffer->m_scaleX, pCurrentBuffer->m_height * pCurrentBuffer->m_scaleY);

   glDisable(GL_CULL_FACE);

   scaleX = pCurrentBuffer != NULL ? 1.0f / pCurrentBuffer->m_width  : VI.rwidth;
   scaleY = pCurrentBuffer != NULL ? 1.0f / pCurrentBuffer->m_height : VI.rheight;
	Z      = (gDP.otherMode.depthSource == G_ZS_PRIM) ? gDP.primDepth.z : gSP.viewport.nearz;
	W      = 1.0f;

   OGL.rect[0].x = (float)ulx * (2.0f * scaleX) - 1.0f;
   OGL.rect[0].y = (float)uly * (-2.0f * scaleY) + 1.0f;
   OGL.rect[0].z = Z;
   OGL.rect[0].w = W;

   OGL.rect[1].x = (float)lrx * (2.0f * scaleX) - 1.0f;
   OGL.rect[1].y = OGL.rect[0].y;
   OGL.rect[1].z = Z;
   OGL.rect[1].w = W;

   OGL.rect[2].x = OGL.rect[0].x;
   OGL.rect[2].y = (float)lry * (-2.0f * scaleY) + 1.0f;
   OGL.rect[2].z = Z;
   OGL.rect[2].w = W;

   OGL.rect[3].x = OGL.rect[1].x;
   OGL.rect[3].y = OGL.rect[2].y;
   OGL.rect[3].z = Z;
   OGL.rect[3].w = W;

#if 0
	if (ogl.isAdjustScreen() && (gDP.colorImage.width > VI.width * 98 / 100) && (_lrx - _ulx < VI.width * 9 / 10)) {
		const float scale = ogl.getAdjustScale();
		for (uint32_t i = 0; i < 4; ++i)
			m_rect[i].x *= scale;
	}
#endif

	if (gDP.otherMode.cycleType == G_CYC_FILL)
		glVertexAttrib4fv(SC_COLOR, color);
   else
		glVertexAttrib4f(SC_COLOR, 0.0f, 0.0f, 0.0f, 0.0f);

   glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	gSP.changed |= CHANGED_GEOMETRYMODE | CHANGED_VIEWPORT;
}

static
bool texturedRectShadowMap(const struct TexturedRectParams *a)
{
	struct FrameBuffer *pCurrentBuffer = FrameBuffer_GetCurrent();
	if (pCurrentBuffer != NULL)
   {
      if (gDP.textureImage.size == 2 && gDP.textureImage.address >= gDP.depthImageAddress &&  gDP.textureImage.address < (gDP.depthImageAddress + gDP.colorImage.width*gDP.colorImage.width * 6 / 4))
      {
#ifdef NEW
         pCurrentBuffer->m_pDepthBuffer->activateDepthBufferTexture(pCurrentBuffer);
         SetDepthFogCombiner();
#endif
      }
   }
	return false;
}

static
bool texturedRectDepthBufferCopy(const struct TexturedRectParams *_params)
{
	// Copy one line from depth buffer into auxiliary color buffer with height = 1.
	// Data from depth buffer loaded into TMEM and then rendered to RDRAM by texrect.
	// Works only with depth buffer emulation enabled.
	// Load of arbitrary data to that area causes weird camera rotation in CBFD.
	const struct gDPTile *pTile = (const struct gDPTile*)gSP.textureTile[0];
	if (pTile->loadType == LOADTYPE_BLOCK && gDP.textureImage.size == 2
         && gDP.textureImage.address >= gDP.depthImageAddress
         &&  gDP.textureImage.address < (gDP.depthImageAddress + gDP.colorImage.width*gDP.colorImage.width * 6 / 4))
   {
      uint32_t x;
	  uint32_t width, ulx;
	  uint16_t *pSrc, *pDst;
      struct FrameBuffer *pBuffer = FrameBuffer_GetCurrent();
      if (config.frameBufferEmulation.enable == 0 || !pBuffer)
         return true;
      /* TODO/FIXMES */
#ifdef NEW
      pBuffer->m_cleared = true;
#endif
      if (config.frameBufferEmulation.copyDepthToRDRAM == 0)
         return true;
#ifdef NEW
      if (FrameBuffer_CopyDepthBuffer(gDP.colorImage.address))
         RDP_RepeatLastLoadBlock();
#endif


      width = (uint32_t)(_params->lrx - _params->ulx);
      ulx = (uint32_t)_params->ulx;
      pSrc = ((uint16_t*)TMEM) + (uint32_t)floorf(_params->uls + 0.5f);
      pDst = (uint16_t*)(gfx_info.RDRAM + gDP.colorImage.address);
      for (x = 0; x < width; ++x)
         pDst[(ulx + x) ^ 1] = swapword(pSrc[x]);

      return true;
   }
	return false;
}

static
bool texturedRectCopyToItself(const struct TexturedRectParams * _params)
{
   struct FrameBuffer *pCurrent = FrameBuffer_GetCurrent();
	if (gSP.textureTile[0]->frameBuffer == pCurrent)
		return true;
	return texturedRectDepthBufferCopy(_params);
}

static bool texturedRectBGCopy(const struct TexturedRectParams *_params)
{
   uint8_t *texaddr, *fbaddr;
   uint32_t y, width, tex_width, uly, lry;
   float flry;
	if (GBI_GetCurrentMicrocodeType() != S2DEX)
		return false;

	flry = _params->lry;
	if (flry > gDP.scissor.lry)
		flry = gDP.scissor.lry;

	width = (uint32_t)(_params->lrx - _params->ulx);
	tex_width = gSP.textureTile[0]->line << 3;
	uly = (uint32_t)_params->uly;
	lry = flry;

	texaddr = gfx_info.RDRAM + gDP.loadInfo[gSP.textureTile[0]->tmem].texAddress + tex_width*(uint32_t)_params->ult + (uint32_t)_params->uls;
	fbaddr = gfx_info.RDRAM + gDP.colorImage.address + (uint32_t)_params->ulx;

	for (y = uly; y < lry; ++y)
   {
		uint8_t *src = texaddr + (y - uly) * tex_width;
		uint8_t *dst = fbaddr + y * gDP.colorImage.width;
		memcpy(dst, src, width);
	}
	FrameBuffer_RemoveBuffer(gDP.colorImage.address);
	return true;
}

static bool texturedRectPaletteMod(const struct TexturedRectParams *_params)
{
   uint32_t i;
   uint16_t env16, prim16, *src, *dst;
   uint8_t envr, envg, envb, prmr, prmg, prmb;
	if (gDP.scissor.lrx != 16 || gDP.scissor.lry != 1 || _params->lrx != 16 || _params->lry != 1)
		return false;
	envr = (uint8_t)(gDP.envColor.r * 31.0f);
	envg = (uint8_t)(gDP.envColor.g * 31.0f);
	envb = (uint8_t)(gDP.envColor.b * 31.0f);
	env16 = (uint16_t)((envr << 11) | (envg << 6) | (envb << 1) | 1);
	prmr = (uint8_t)(gDP.primColor.r * 31.0f);
	prmg = (uint8_t)(gDP.primColor.g * 31.0f);
	prmb = (uint8_t)(gDP.primColor.b * 31.0f);
	prim16 = (uint16_t)((prmr << 11) | (prmg << 6) | (prmb << 1) | 1);
	src = (uint16_t*)&TMEM[256];
	dst = (uint16_t*)(gfx_info.RDRAM + gDP.colorImage.address);
	for (i = 0; i < 16; ++i)
		dst[i ^ 1] = (src[i<<2] & 0x100) ? prim16 : env16;
	return true;
}

static
bool texturedRectMonochromeBackground(const struct TexturedRectParams * _params)
{
	if (gDP.textureImage.address >= gDP.colorImage.address && gDP.textureImage.address <= (gDP.colorImage.address + gDP.colorImage.width*gDP.colorImage.height * 2)) {
#ifdef GL_IMAGE_TEXTURES_SUPPORT
		FrameBuffer * pCurrentBuffer = frameBufferList().getCurrent();
		if (pCurrentBuffer != NULL) {
			FrameBuffer_ActivateBufferTexture(0, pCurrentBuffer);
			SetMonochromeCombiner();
			return false;
		} else
#endif
			return true;
	}
	return false;
}

// Special processing of textured rect.
// Return true if actuial rendering is not necessary
bool(*texturedRectSpecial)(const struct TexturedRectParams * _params) = NULL;

void OGL_DrawTexturedRect(const struct TexturedRectParams *_params)
{
   struct FrameBuffer *pCurrentBuffer;
   float scaleX, scaleY, Z, W;
   bool updateArrays;

   if (gSP.changed || gDP.changed)
      _updateStates();

   updateArrays = OGL.renderState != RS_TEXTUREDRECT;
   if (updateArrays || scProgramChanged)
   {
      OGL.renderState = RS_TEXTUREDRECT;
      glDisableVertexAttribArray(SC_COLOR);
      OGL_SetTexCoordArrays();
      SC_ForceUniform1f(uRenderState, RS_TEXTUREDRECT);
   }

   if (updateArrays)
   {
      glVertexAttrib4f(SC_COLOR, 0, 0, 0, 0);
      glVertexAttribPointer(SC_POSITION, 2, GL_FLOAT, GL_FALSE, sizeof(GLVertex), &OGL.rect[0].x);
      glVertexAttribPointer(SC_TEXCOORD0, 2, GL_FLOAT, GL_FALSE, sizeof(GLVertex), &OGL.rect[0].s0);
      glVertexAttribPointer(SC_TEXCOORD1, 2, GL_FLOAT, GL_FALSE, sizeof(GLVertex), &OGL.rect[0].s1);
   }

	if (__RSP.cmd == 0xE4 && texturedRectSpecial != NULL && texturedRectSpecial(_params))
   {
      gSP.changed |= CHANGED_GEOMETRYMODE;
      return;
   }

   pCurrentBuffer = FrameBuffer_GetCurrent();

   if (pCurrentBuffer == NULL)
      glViewport(0, OGL_GetHeightOffset(), OGL_GetScreenWidth(), OGL_GetScreenHeight());
   else
      glViewport(0, 0, pCurrentBuffer->m_width * pCurrentBuffer->m_scaleX, pCurrentBuffer->m_height * pCurrentBuffer->m_scaleY);

   glDisable(GL_CULL_FACE);

   scaleX = pCurrentBuffer != NULL ? 1.0f / pCurrentBuffer->m_width  : VI.rwidth;
   scaleY = pCurrentBuffer != NULL ? 1.0f / pCurrentBuffer->m_height : VI.rheight;
	Z = (gDP.otherMode.depthSource == G_ZS_PRIM) ? gDP.primDepth.z : gSP.viewport.nearz;
	W = 1.0f;

   OGL.rect[0].x = (float) _params->ulx * (2.0f * scaleX) - 1.0f;
   OGL.rect[0].y = (float) _params->uly * (-2.0f * scaleY) + 1.0f;
   OGL.rect[0].z = Z;
   OGL.rect[0].w = W;

   OGL.rect[1].x = (float) (_params->lrx) * (2.0f * scaleX) - 1.0f;
   OGL.rect[1].y = OGL.rect[0].y;
   OGL.rect[1].z = Z;
   OGL.rect[1].w = W;

   OGL.rect[2].x = OGL.rect[0].x;
   OGL.rect[2].y = (float) (_params->lry) * (-2.0f * scaleY) + 1.0f;
   OGL.rect[2].z = Z;
   OGL.rect[2].w = W;

   OGL.rect[3].x = OGL.rect[1].x;
   OGL.rect[3].y = OGL.rect[2].y;
   OGL.rect[3].z = Z;
   OGL.rect[3].w = W;

   if (scProgramCurrent->usesT0 && cache.current[0] && gSP.textureTile[0])
   {
      OGL.rect[0].s0 = _params->uls * cache.current[0]->shiftScaleS - gSP.textureTile[0]->fuls;
      OGL.rect[0].t0 = _params->ult * cache.current[0]->shiftScaleT - gSP.textureTile[0]->fult;
      OGL.rect[3].s0 = (_params->lrs + 1.0f) * cache.current[0]->shiftScaleS - gSP.textureTile[0]->fuls;
      OGL.rect[3].t0 = (_params->lrt + 1.0f) * cache.current[0]->shiftScaleT - gSP.textureTile[0]->fult;

      if ((cache.current[0]->maskS) && !(cache.current[0]->mirrorS) && (fmod( OGL.rect[0].s0, cache.current[0]->width ) == 0.0f))
      {
         OGL.rect[3].s0 -= OGL.rect[0].s0;
         OGL.rect[0].s0 = 0.0f;
      }

      if ((cache.current[0]->maskT)  && !(cache.current[0]->mirrorT) && (fmod( OGL.rect[0].t0, cache.current[0]->height ) == 0.0f))
      {
         OGL.rect[3].t0 -= OGL.rect[0].t0;
         OGL.rect[0].t0 = 0.0f;
      }

      glActiveTexture( GL_TEXTURE0);

      if ((OGL.rect[0].s0 >= 0.0f) && (OGL.rect[3].s0 <= cache.current[0]->width))
         glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );

      if ((OGL.rect[0].t0 >= 0.0f) && (OGL.rect[3].t0 <= cache.current[0]->height))
         glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

      OGL.rect[0].s0 *= cache.current[0]->scaleS;
      OGL.rect[0].t0 *= cache.current[0]->scaleT;
      OGL.rect[3].s0 *= cache.current[0]->scaleS;
      OGL.rect[3].t0 *= cache.current[0]->scaleT;
   }

   if (scProgramCurrent->usesT1 && cache.current[1] && gSP.textureTile[1])
   {
      OGL.rect[0].s1 = _params->uls * cache.current[1]->shiftScaleS - gSP.textureTile[1]->fuls;
      OGL.rect[0].t1 = _params->ult * cache.current[1]->shiftScaleT - gSP.textureTile[1]->fult;
      OGL.rect[3].s1 = (_params->lrs + 1.0f) * cache.current[1]->shiftScaleS - gSP.textureTile[1]->fuls;
      OGL.rect[3].t1 = (_params->lrt + 1.0f) * cache.current[1]->shiftScaleT - gSP.textureTile[1]->fult;

      if ((cache.current[1]->maskS) && (fmod( OGL.rect[0].s1, cache.current[1]->width ) == 0.0f) && !(cache.current[1]->mirrorS))
      {
         OGL.rect[3].s1 -= OGL.rect[0].s1;
         OGL.rect[0].s1 = 0.0f;
      }

      if ((cache.current[1]->maskT) && (fmod( OGL.rect[0].t1, cache.current[1]->height ) == 0.0f) && !(cache.current[1]->mirrorT))
      {
         OGL.rect[3].t1 -= OGL.rect[0].t1;
         OGL.rect[0].t1 = 0.0f;
      }

      glActiveTexture( GL_TEXTURE1);
      if ((OGL.rect[0].s1 == 0.0f) && (OGL.rect[3].s1 <= cache.current[1]->width))
         glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );

      if ((OGL.rect[0].t1 == 0.0f) && (OGL.rect[3].t1 <= cache.current[1]->height))
         glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

      OGL.rect[0].s1 *= cache.current[1]->scaleS;
      OGL.rect[0].t1 *= cache.current[1]->scaleT;
      OGL.rect[3].s1 *= cache.current[1]->scaleS;
      OGL.rect[3].t1 *= cache.current[1]->scaleT;
   }

   if (gDP.otherMode.cycleType == G_CYC_COPY)
   {
      glActiveTexture(GL_TEXTURE0);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
   }

   if (_params->flip)
   {
      OGL.rect[1].s0 = OGL.rect[0].s0;
      OGL.rect[1].t0 = OGL.rect[3].t0;
      OGL.rect[1].s1 = OGL.rect[0].s1;
      OGL.rect[1].t1 = OGL.rect[3].t1;

      OGL.rect[2].s0 = OGL.rect[3].s0;
      OGL.rect[2].t0 = OGL.rect[0].t0;
      OGL.rect[2].s1 = OGL.rect[3].s1;
      OGL.rect[2].t1 = OGL.rect[0].t1;
   }
   else
   {
      OGL.rect[1].s0 = OGL.rect[3].s0;
      OGL.rect[1].t0 = OGL.rect[0].t0;
      OGL.rect[1].s1 = OGL.rect[3].s1;
      OGL.rect[1].t1 = OGL.rect[0].t1;

      OGL.rect[2].s0 = OGL.rect[0].s0;
      OGL.rect[2].t0 = OGL.rect[3].t0;
      OGL.rect[2].s1 = OGL.rect[0].s1;
      OGL.rect[2].t1 = OGL.rect[3].t1;
   }

#ifdef NEW
	if (ogl.isAdjustScreen() && (gDP.colorImage.width > VI.width * 98 / 100) && (_params.lrx - _params.ulx < VI.width * 9 / 10))
   {
		const float scale = ogl.getAdjustScale();
		for (uint32_t i = 0; i < 4; ++i)
			m_rect[i].x *= scale;
	}
#endif

   glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	gSP.changed |= CHANGED_GEOMETRYMODE | CHANGED_VIEWPORT;
}

void OGL_ClearDepthBuffer(bool _fullsize)
{
	if (config.frameBufferEmulation.enable && FrameBuffer_GetCurrent() == NULL)
		return;

#ifdef NEW
	depthBufferList().clearBuffer(_uly, _lry);
#endif

   glDisable( GL_SCISSOR_TEST );
   glDepthMask( GL_TRUE );
   glClear( GL_DEPTH_BUFFER_BIT );

   _updateDepthUpdate();

   glEnable( GL_SCISSOR_TEST );
}

void OGL_ClearColorBuffer(float *color)
{
	glDisable( GL_SCISSOR_TEST );

   glClearColor( color[0], color[1], color[2], color[3] );
   glClear( GL_COLOR_BUFFER_BIT );

	glEnable( GL_SCISSOR_TEST );
}

int OGL_CheckError(void)
{
   GLenum e = glGetError();
   if (e != GL_NO_ERROR)
   {
      printf("GL Error: ");
      switch(e)
      {
         case GL_INVALID_ENUM:   printf("INVALID ENUM"); break;
         case GL_INVALID_VALUE:  printf("INVALID VALUE"); break;
         case GL_INVALID_OPERATION:  printf("INVALID OPERATION"); break;
         case GL_OUT_OF_MEMORY:  printf("OUT OF MEMORY"); break;
      }
      printf("\n");
      return 1;
   }
   return 0;
}

void OGL_SwapBuffers(void)
{
   int retro_return(bool a);
   // if emulator defined a render callback function, call it before
   // buffer swap
   if (renderCallback)
      (*renderCallback)();
   retro_return(true);

   scProgramChanged = 0;
	gDP.otherMode.l = 0;
	gln64gDPSetTextureLUT(G_TT_NONE);
	++__RSP.DList;
}

void OGL_ReadScreen( void *dest, int *width, int *height )
{
   *width  = OGL_GetScreenWidth();
   *height = OGL_GetScreenHeight();

   dest = malloc(OGL_GetScreenHeight() * OGL_GetScreenWidth() * 3);
   if (dest == NULL)
      return;

   glReadPixels(0, OGL_GetHeightOffset(),
         OGL_GetScreenWidth(), OGL_GetScreenHeight(),
         GL_RGBA, GL_UNSIGNED_BYTE, dest );
}

void _setSpecialTexrect(void)
{
	const char * name = __RSP.romname;
	if (strstr(name, (const char *)"Beetle") || strstr(name, (const char *)"BEETLE") || strstr(name, (const char *)"HSV")
		|| strstr(name, (const char *)"DUCK DODGERS") || strstr(name, (const char *)"DAFFY DUCK"))
		texturedRectSpecial = texturedRectShadowMap;
	else if (strstr(name, (const char *)"Perfect Dark") || strstr(name, (const char *)"PERFECT DARK"))
		texturedRectSpecial = texturedRectDepthBufferCopy; // See comments to that function!
	else if (strstr(name, (const char *)"CONKER BFD"))
		texturedRectSpecial = texturedRectCopyToItself;
	else if (strstr(name, (const char *)"YOSHI STORY"))
		texturedRectSpecial = texturedRectBGCopy;
	else if (strstr(name, (const char *)"PAPER MARIO") || strstr(name, (const char *)"MARIO STORY"))
		texturedRectSpecial = texturedRectPaletteMod;
	else if (strstr(name, (const char *)"ZELDA"))
		texturedRectSpecial = texturedRectMonochromeBackground;
	else
		texturedRectSpecial = NULL;
}

bool OGL_Start(void)
{
   float f;
   _initStates();
	_setSpecialTexrect();

   //check extensions
   if ((config.texture.maxAnisotropy>0) && !OGL_IsExtSupported("GL_EXT_texture_filter_anistropic"))
   {
      LOG(LOG_WARNING, "Anistropic Filtering is not supported.\n");
      config.texture.maxAnisotropy = 0;
   }

   f = 0;
   glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &f);
   if (config.texture.maxAnisotropy > ((int)f))
   {
      LOG(LOG_WARNING, "Clamping max anistropy to %ix.\n", (int)f);
      config.texture.maxAnisotropy = (int)f;
   }

   //Print some info
   LOG(LOG_VERBOSE, "[gles2n64]: Enable Runfast... \n");

   //We must have a shader bound before binding any textures:
   Combiner_Init();
   Combiner_Set(EncodeCombineMode(0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0), -1);
   Combiner_Set(EncodeCombineMode(0, 0, 0, SHADE, 0, 0, 0, 1, 0, 0, 0, SHADE, 0, 0, 0, 1), -1);

   TextureCache_Init();

   OGL.renderState = RS_NONE;
   gSP.changed = gDP.changed = 0xFFFFFFFF;

   memset(OGL.triangles.vertices, 0, VERTBUFF_SIZE * sizeof(struct SPVertex));
   memset(OGL.triangles.elements, 0, ELEMBUFF_SIZE * sizeof(GLubyte));

   OGL.triangles.num = 0;

   return true;
}
