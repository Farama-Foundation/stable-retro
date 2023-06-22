#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include <retro_miscellaneous.h>

#include "Common.h"
#include "gles2N64.h"
#include "Debug.h"
#include "RSP.h"
#include "GBI.h"
#include "gSP.h"
#include "gDP.h"
#include "3DMath.h"
#include "OpenGL.h"
#include "CRC.h"
#include <string.h>
#include "convert.h"
#include "S2DEX.h"
#include "VI.h"
#include "FrameBuffer.h"
#include "DepthBuffer.h"
#include "Config.h"

#include "../../Graphics/3dmath.h"
#include "../../Graphics/RDP/gDP_state.h"
#include "../../Graphics/RSP/gSP_state.h"
#include "../../Graphics/image_convert.h"

//Note: 0xC0 is used by 1080 alot, its an unknown command.

float identityMatrix[4][4] =
{
    { 1.0f, 0.0f, 0.0f, 0.0f },
    { 0.0f, 1.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f }
};


static INLINE void gln64gSPFlushTriangles(void)
{
   if ((gSP.geometryMode & G_SHADING_SMOOTH) == 0)
   {
      OGL_DrawTriangles();
      return;
   }

   if (
         (__RSP.nextCmd != G_TRI1) &&
         (__RSP.nextCmd != G_TRI2) &&
         (__RSP.nextCmd != G_TRI4) &&
         (__RSP.nextCmd != G_QUAD)
      ) 
         OGL_DrawTriangles();
}


void gln64gSPCopyVertex( struct SPVertex *dest, struct SPVertex *src )
{
	dest->x = src->x;
	dest->y = src->y;
	dest->z = src->z;
	dest->w = src->w;
	dest->r = src->r;
	dest->g = src->g;
	dest->b = src->b;
	dest->a = src->a;
	dest->s = src->s;
	dest->t = src->t;
}

void gln64gSPInterpolateVertex( struct SPVertex *dest, float percent, struct SPVertex *first, struct SPVertex *second )
{
	dest->x = first->x + percent * (second->x - first->x);
	dest->y = first->y + percent * (second->y - first->y);
	dest->z = first->z + percent * (second->z - first->z);
	dest->w = first->w + percent * (second->w - first->w);
	dest->r = first->r + percent * (second->r - first->r);
	dest->g = first->g + percent * (second->g - first->g);
	dest->b = first->b + percent * (second->b - first->b);
	dest->a = first->a + percent * (second->a - first->a);
	dest->s = first->s + percent * (second->s - first->s);
	dest->t = first->t + percent * (second->t - first->t);
}

void gln64gSPTriangle(int32_t v0, int32_t v1, int32_t v2)
{
   /* TODO/FIXME - update with gliden64 code */
   if ((v0 < INDEXMAP_SIZE) && (v1 < INDEXMAP_SIZE) && (v2 < INDEXMAP_SIZE))
   {
      OGL_AddTriangle(v0, v1, v2);
   }

   if (depthBuffer.current) depthBuffer.current->cleared = false;
   gDP.colorImage.height = (uint32_t)(MAX( gDP.colorImage.height, (uint32_t)gDP.scissor.lry ));
}

void gln64gSP1Triangle( int32_t v0, int32_t v1, int32_t v2, int flags)
{
   gln64gSPTriangle( v0, v1, v2);
   gln64gSPFlushTriangles();
}

void gln64gSP2Triangles(const int32_t v00, const int32_t v01, const int32_t v02, const int32_t flag0,
                    const int32_t v10, const int32_t v11, const int32_t v12, const int32_t flag1 )
{
   gln64gSPTriangle( v00, v01, v02);
   gln64gSPTriangle( v10, v11, v12);
   gln64gSPFlushTriangles();
}

void gln64gSP4Triangles(const int32_t v00, const int32_t v01, const int32_t v02,
                    const int32_t v10, const int32_t v11, const int32_t v12,
                    const int32_t v20, const int32_t v21, const int32_t v22,
                    const int32_t v30, const int32_t v31, const int32_t v32 )
{
   gln64gSPTriangle(v00, v01, v02);
   gln64gSPTriangle(v10, v11, v12);
   gln64gSPTriangle(v20, v21, v22);
   gln64gSPTriangle(v30, v31, v32);
   gln64gSPFlushTriangles();
}

static void gln64gSPTransformVertex_default(float vtx[4], float mtx[4][4])
{
   float x, y, z, w;
   x = vtx[0];
   y = vtx[1];
   z = vtx[2];
   w = vtx[3];

   vtx[0] = x * mtx[0][0] + y * mtx[1][0] + z * mtx[2][0] + mtx[3][0];
   vtx[1] = x * mtx[0][1] + y * mtx[1][1] + z * mtx[2][1] + mtx[3][1];
   vtx[2] = x * mtx[0][2] + y * mtx[1][2] + z * mtx[2][2] + mtx[3][2];
   vtx[3] = x * mtx[0][3] + y * mtx[1][3] + z * mtx[2][3] + mtx[3][3];
}

static void gln64gSPLightVertex_default(void *data)
{
   struct SPVertex * _vtx = (struct SPVertex*)data;
	if (!config.generalEmulation.enableHWLighting)
   {
      unsigned i;

      _vtx->HWLight = 0;
      _vtx->r = gSP.lights[gSP.numLights].r;
      _vtx->g = gSP.lights[gSP.numLights].g;
      _vtx->b = gSP.lights[gSP.numLights].b;

      for (i = 0; i < gSP.numLights; i++)
      {
         float intensity = DotProduct( &_vtx->nx, &gSP.lights[i].x );
         if (intensity < 0.0f)
            intensity = 0.0f;
         _vtx->r += gSP.lights[i].r * intensity;
         _vtx->g += gSP.lights[i].g * intensity;
         _vtx->b += gSP.lights[i].b * intensity;
      }

      _vtx->r = MIN(1.0f, _vtx->r);
      _vtx->g = MIN(1.0f, _vtx->g);
      _vtx->b = MIN(1.0f, _vtx->b);
   }
   else
   {
      /* TODO/FIXME - update with gliden64 code */
   }

}

static void gln64gSPPointLightVertex_default(void *data, float * _vPos)
{
   uint32_t l;
   float light_intensity = 0.0f;
   struct SPVertex *_vtx = (struct SPVertex*)data;

   assert(_vPos != NULL);
   _vtx->HWLight = 0;
   _vtx->r = gSP.lights[gSP.numLights].r;
   _vtx->g = gSP.lights[gSP.numLights].g;
   _vtx->b = gSP.lights[gSP.numLights].b;

   for (l = 0; l < gSP.numLights; ++l)
   {
      float light_len2, light_len, at;
      float lvec[3] = {gSP.lights[l].posx, gSP.lights[l].posy, gSP.lights[l].posz};
      lvec[0] -= _vPos[0];
      lvec[1] -= _vPos[1];
      lvec[2] -= _vPos[2];
      light_len2 = lvec[0]*lvec[0] + lvec[1]*lvec[1] + lvec[2]*lvec[2];
      light_len = sqrtf(light_len2);
      at = gSP.lights[l].ca + light_len/65535.0f*gSP.lights[l].la + light_len2/65535.0f*gSP.lights[l].qa;

      if (at > 0.0f)
         light_intensity = 1/at;//DotProduct (lvec, nvec) / (light_len * normal_len * at);
      else
         light_intensity = 0.0f;

      if (light_intensity > 0.0f)
      {
         _vtx->r += gSP.lights[l].r * light_intensity;
         _vtx->g += gSP.lights[l].g * light_intensity;
         _vtx->b += gSP.lights[l].b * light_intensity;
      }
   }
   if (_vtx->r > 1.0f)  _vtx->r = 1.0f;
   if (_vtx->g > 1.0f)  _vtx->g = 1.0f;
   if (_vtx->b > 1.0f)  _vtx->b = 1.0f;
}

static void gln64gSPLightVertex_CBFD(void *data)
{
   struct SPVertex *_vtx = (struct SPVertex*)data;
   uint32_t l;
	float r = gSP.lights[gSP.numLights].r;
	float g = gSP.lights[gSP.numLights].g;
	float b = gSP.lights[gSP.numLights].b;

	for (l = 0; l < gSP.numLights; ++l)
   {
      const struct SPLight *light = (const struct SPLight*)&gSP.lights[l];
      const float vx = (_vtx->x + gSP.vertexCoordMod[ 8])*gSP.vertexCoordMod[12] - light->posx;
      const float vy = (_vtx->y + gSP.vertexCoordMod[ 9])*gSP.vertexCoordMod[13] - light->posy;
      const float vz = (_vtx->z + gSP.vertexCoordMod[10])*gSP.vertexCoordMod[14] - light->posz;
      const float vw = (_vtx->w + gSP.vertexCoordMod[11])*gSP.vertexCoordMod[15] - light->posw;
      const float len = (vx*vx+vy*vy+vz*vz+vw*vw)/65536.0f;
      float intensity = light->ca / len;
      if (intensity > 1.0f)
         intensity = 1.0f;
      r += light->r * intensity;
      g += light->g * intensity;
      b += light->b * intensity;
   }

	r = MIN(1.0f, r);
	g = MIN(1.0f, g);
	b = MIN(1.0f, b);

	_vtx->r *= r;
	_vtx->g *= g;
	_vtx->b *= b;
	_vtx->HWLight = 0;
}

static void gln64gSPPointLightVertex_CBFD(void *data, float * _vPos)
{
   uint32_t l;
   const struct SPLight *light = NULL;
   struct SPVertex *_vtx = (struct SPVertex*)data;
	float r = gSP.lights[gSP.numLights].r;
	float g = gSP.lights[gSP.numLights].g;
	float b = gSP.lights[gSP.numLights].b;

	float intensity = 0.0f;

	for (l = 0; l < gSP.numLights-1; ++l)
   {
		light = (struct SPLight*)&gSP.lights[l];
		intensity = DotProduct( &_vtx->nx, &light->x );

		if (intensity < 0.0f)
			continue;

		if (light->ca > 0.0f)
      {
			const float vx = (_vtx->x + gSP.vertexCoordMod[ 8])*gSP.vertexCoordMod[12] - light->posx;
			const float vy = (_vtx->y + gSP.vertexCoordMod[ 9])*gSP.vertexCoordMod[13] - light->posy;
			const float vz = (_vtx->z + gSP.vertexCoordMod[10])*gSP.vertexCoordMod[14] - light->posz;
			const float vw = (_vtx->w + gSP.vertexCoordMod[11])*gSP.vertexCoordMod[15] - light->posw;
			const float len = (vx*vx+vy*vy+vz*vz+vw*vw)/65536.0f;
			float p_i = light->ca / len;
			if (p_i > 1.0f) p_i = 1.0f;
			intensity *= p_i;
		}
		r += light->r * intensity;
		g += light->g * intensity;
		b += light->b * intensity;
	}

	light = (struct SPLight*)&gSP.lights[gSP.numLights-1];

	intensity = DotProduct( &_vtx->nx, &light->x );

	if (intensity > 0.0f)
   {
		r += light->r * intensity;
		g += light->g * intensity;
		b += light->b * intensity;
	}

	r = MIN(1.0f, r);
	g = MIN(1.0f, g);
	b = MIN(1.0f, b);

	_vtx->r *= r;
	_vtx->g *= g;
	_vtx->b *= b;
	_vtx->HWLight = 0;
}

static void gln64gSPBillboardVertex_default(uint32_t v, uint32_t i)
{
   struct SPVertex *vtx0 = (struct SPVertex*)&OGL.triangles.vertices[i];
   struct SPVertex *vtx  = (struct SPVertex*)&OGL.triangles.vertices[v];

   vtx->x += vtx0->x;
   vtx->y += vtx0->y;
   vtx->z += vtx0->z;
   vtx->w += vtx0->w;
}

void gln64gSPClipVertex(uint32_t v)
{
   struct SPVertex *vtx = &OGL.triangles.vertices[v];
   vtx->clip = 0;
   if (vtx->x > +vtx->w)   vtx->clip |= CLIP_POSX;
   if (vtx->x < -vtx->w)   vtx->clip |= CLIP_NEGX;
   if (vtx->y > +vtx->w)   vtx->clip |= CLIP_POSY;
   if (vtx->y < -vtx->w)   vtx->clip |= CLIP_NEGY;
   if (vtx->w < 0.01f)      vtx->clip |= CLIP_Z;
}

void gln64gSPProcessVertex(uint32_t v)
{
   
   float intensity, r, g, b;
   struct SPVertex *vtx = (struct SPVertex*)&OGL.triangles.vertices[v];

   if (gSP.changed & CHANGED_MATRIX)
      gln64gSPCombineMatrices();

   gln64gSPTransformVertex( &vtx->x, gSP.matrix.combined );

   if (gSP.viewport.vscale[0] < 0)
		vtx->x = -vtx->x;

   if (gSP.matrix.billboard)
   {
      int i = 0;

      gln64gSPBillboardVertex(v, i);
   }

   gln64gSPClipVertex(v);

   if (gSP.geometryMode & G_LIGHTING)
   {
      float vPos[3];
      vPos[0] = (float)vtx->x;
      vPos[1] = (float)vtx->y;
      vPos[2] = (float)vtx->z;

      TransformVectorNormalize( &vtx->nx, gSP.matrix.modelView[gSP.matrix.modelViewi] );
		if (gSP.geometryMode & G_POINT_LIGHTING)
			gln64gSPPointLightVertex(vtx, vPos);
      else
			gln64gSPLightVertex(vtx);

      if (/* GBI.isTextureGen() && */ gSP.geometryMode & G_TEXTURE_GEN)
      {
			float fLightDir[3] = {vtx->nx, vtx->ny, vtx->nz};
			float x, y;

			if (gSP.lookatEnable)
         {
				x = DotProduct(&gSP.lookat[0].x, fLightDir);
				y = DotProduct(&gSP.lookat[1].x, fLightDir);
			}
         else
         {
				x = fLightDir[0];
				y = fLightDir[1];
			}

         if (gSP.geometryMode & G_TEXTURE_GEN_LINEAR)
         {
            vtx->s = acosf(x) * 325.94931f;
            vtx->t = acosf(y) * 325.94931f;
         }
         else /* G_TEXTURE_GEN */
         {
            vtx->s = (x + 1.0f) * 512.0f;
            vtx->t = (y + 1.0f) * 512.0f;
         }
      }
   }
   else
		vtx->HWLight = 0;
}

void gln64gSPLoadUcodeEx( uint32_t uc_start, uint32_t uc_dstart, uint16_t uc_dsize )
{
   MicrocodeInfo *ucode;
   __RSP.PCi = 0;
   gSP.matrix.modelViewi = 0;
   gSP.changed |= CHANGED_MATRIX;
   gSP.status[0] = gSP.status[1] = gSP.status[2] = gSP.status[3] = 0;

   if ((((uc_start & 0x1FFFFFFF) + 4096) > RDRAMSize) || (((uc_dstart & 0x1FFFFFFF) + uc_dsize) > RDRAMSize))
      return;

   ucode = (MicrocodeInfo*)GBI_DetectMicrocode( uc_start, uc_dstart, uc_dsize );

   __RSP.uc_start  = uc_start;
   __RSP.uc_dstart = uc_dstart;

   /* TODO/FIXME - remove this maybe? for gliden64 */
   if (ucode->type != 0xFFFFFFFF)
      last_good_ucode = ucode->type;

   if (ucode->type != NONE)
      GBI_MakeCurrent( ucode );
   else
   {
      LOG(LOG_WARNING, "Unknown Ucode\n");
   }
}

void gln64gSPCombineMatrices(void)
{
   MultMatrix(gSP.matrix.projection, gSP.matrix.modelView[gSP.matrix.modelViewi], gSP.matrix.combined);
   gSP.changed &= ~CHANGED_MATRIX;
}

void gln64gSPNoOp(void)
{
   gln64gSPFlushTriangles();
}

void gln64gSPMatrix( uint32_t matrix, uint8_t param )
{
   float mtx[4][4];
   uint32_t address = RSP_SegmentToPhysical( matrix );

   if (address + 64 > RDRAMSize)
      return;

   RSP_LoadMatrix( mtx, address );

   if (param & G_MTX_PROJECTION)
   {
      if (param & G_MTX_LOAD)
         CopyMatrix( gSP.matrix.projection, mtx );
      else
         MultMatrix2( gSP.matrix.projection, mtx );
   }
   else
   {
      if ((param & G_MTX_PUSH) && (gSP.matrix.modelViewi < (gSP.matrix.stackSize - 1)))
      {
         CopyMatrix( gSP.matrix.modelView[gSP.matrix.modelViewi + 1], gSP.matrix.modelView[gSP.matrix.modelViewi] );
         gSP.matrix.modelViewi++;
      }
      if (param & G_MTX_LOAD)
         CopyMatrix( gSP.matrix.modelView[gSP.matrix.modelViewi], mtx );
      else
         MultMatrix2( gSP.matrix.modelView[gSP.matrix.modelViewi], mtx );
   }

   gSP.changed |= CHANGED_MATRIX;
}

void gln64gSPDMAMatrix( uint32_t matrix, uint8_t index, uint8_t multiply )
{
   float mtx[4][4];
   uint32_t address = gSP.DMAOffsets.mtx + RSP_SegmentToPhysical( matrix );

   if (address + 64 > RDRAMSize)
      return;

   RSP_LoadMatrix( mtx, address );

   gSP.matrix.modelViewi = index;

   if (multiply)
      MultMatrix(gSP.matrix.modelView[0], mtx, gSP.matrix.modelView[gSP.matrix.modelViewi]);
   else
      CopyMatrix( gSP.matrix.modelView[gSP.matrix.modelViewi], mtx );

   CopyMatrix( gSP.matrix.projection, identityMatrix );

   gSP.changed |= CHANGED_MATRIX;
}

void gln64gSPViewport(uint32_t v)
{
   uint32_t address = RSP_SegmentToPhysical( v );

   if ((address + 16) > RDRAMSize)
      return;

   gSP.viewport.vscale[0] = _FIXED2FLOAT( *(int16_t*)&gfx_info.RDRAM[address +  2], 2 );
   gSP.viewport.vscale[1] = _FIXED2FLOAT( *(int16_t*)&gfx_info.RDRAM[address     ], 2 );
   gSP.viewport.vscale[2] = _FIXED2FLOAT( *(int16_t*)&gfx_info.RDRAM[address +  6], 10 ); /* * 0.00097847357f; */
   gSP.viewport.vscale[3] = *(int16_t*)&gfx_info.RDRAM[address +  4];
   gSP.viewport.vtrans[0] = _FIXED2FLOAT( *(int16_t*)&gfx_info.RDRAM[address + 10], 2 );
   gSP.viewport.vtrans[1] = _FIXED2FLOAT( *(int16_t*)&gfx_info.RDRAM[address +  8], 2 );
   gSP.viewport.vtrans[2] = _FIXED2FLOAT( *(int16_t*)&gfx_info.RDRAM[address + 14], 10 ); /* * 0.00097847357f; */
   gSP.viewport.vtrans[3] = *(int16_t*)&gfx_info.RDRAM[address + 12];

   gSP.viewport.x      = gSP.viewport.vtrans[0] - gSP.viewport.vscale[0];
   gSP.viewport.y      = gSP.viewport.vtrans[1] - gSP.viewport.vscale[1];
   gSP.viewport.width  = fabs(gSP.viewport.vscale[0]) * 2;
   gSP.viewport.height = fabs(gSP.viewport.vscale[1]) * 2;
   gSP.viewport.nearz  = gSP.viewport.vtrans[2] - gSP.viewport.vscale[2];
   gSP.viewport.farz   = (gSP.viewport.vtrans[2] + gSP.viewport.vscale[2]) ;

   gSP.changed |= CHANGED_VIEWPORT;
}

void gln64gSPForceMatrix( uint32_t mptr )
{
   uint32_t address = RSP_SegmentToPhysical( mptr );

   if (address + 64 > RDRAMSize)
      return;

   RSP_LoadMatrix( gSP.matrix.combined, address);

   gSP.changed &= ~CHANGED_MATRIX;
}

void gln64gSPLight( uint32_t l, int32_t n )
{
   uint32_t addrByte;
   Light *light = NULL;

	--n;
	addrByte = RSP_SegmentToPhysical( l );

	if ((addrByte + sizeof( Light )) > RDRAMSize)
		return;

	light = (Light*)&gfx_info.RDRAM[addrByte];

	if (n < 8)
   {
      uint32_t addrShort;

		gSP.lights[n].r = light->r * 0.0039215689f;
		gSP.lights[n].g = light->g * 0.0039215689f;
		gSP.lights[n].b = light->b * 0.0039215689f;

		gSP.lights[n].x = light->x;
		gSP.lights[n].y = light->y;
		gSP.lights[n].z = light->z;

		NormalizeVector( &gSP.lights[n].x );
		addrShort = addrByte >> 1;
		gSP.lights[n].posx = (float)(((short*)gfx_info.RDRAM)[(addrShort+4)^1]);
		gSP.lights[n].posy = (float)(((short*)gfx_info.RDRAM)[(addrShort+5)^1]);
		gSP.lights[n].posz = (float)(((short*)gfx_info.RDRAM)[(addrShort+6)^1]);
		gSP.lights[n].ca = (float)(gfx_info.RDRAM[(addrByte + 3) ^ 3]) / 16.0f;
		gSP.lights[n].la = (float)(gfx_info.RDRAM[(addrByte + 7) ^ 3]);
		gSP.lights[n].qa = (float)(gfx_info.RDRAM[(addrByte + 14) ^ 3]) / 8.0f;
	}

	if (config.generalEmulation.enableHWLighting != 0)
		gSP.changed |= CHANGED_LIGHT;
}

void gln64gSPLightCBFD( uint32_t l, int32_t n )
{
   Light *light = NULL;
	uint32_t addrByte = RSP_SegmentToPhysical( l );

	if ((addrByte + sizeof( Light )) > RDRAMSize)
		return;

	light = (Light*)&gfx_info.RDRAM[addrByte];

	if (n < 12)
   {
	   uint32_t addrShort;

		gSP.lights[n].r = light->r * 0.0039215689f;
		gSP.lights[n].g = light->g * 0.0039215689f;
		gSP.lights[n].b = light->b * 0.0039215689f;

		gSP.lights[n].x = light->x;
		gSP.lights[n].y = light->y;
		gSP.lights[n].z = light->z;

		NormalizeVector( &gSP.lights[n].x );
		addrShort = addrByte >> 1;
		gSP.lights[n].posx = (float)(((int16_t*)gfx_info.RDRAM)[(addrShort+16)^1]);
		gSP.lights[n].posy = (float)(((int16_t*)gfx_info.RDRAM)[(addrShort+17)^1]);
		gSP.lights[n].posz = (float)(((int16_t*)gfx_info.RDRAM)[(addrShort+18)^1]);
		gSP.lights[n].posw = (float)(((int16_t*)gfx_info.RDRAM)[(addrShort+19)^1]);
		gSP.lights[n].ca = (float)(gfx_info.RDRAM[(addrByte + 12) ^ 3]) / 16.0f;
	}

	if (config.generalEmulation.enableHWLighting != 0)
		gSP.changed |= CHANGED_LIGHT;
}

void gln64gSPLookAt( uint32_t _l, uint32_t _n )
{
	Light *light;
   uint32_t address = RSP_SegmentToPhysical(_l);

   if ((address + sizeof(Light)) > RDRAMSize)
      return;

   assert(_n < 2);

   light = (Light*)&gfx_info.RDRAM[address];

   gSP.lookat[_n].x = light->x;
   gSP.lookat[_n].y = light->y;
   gSP.lookat[_n].z = light->z;

   gSP.lookatEnable = (_n == 0) || (_n == 1 && (light->x != 0 || light->y != 0));

   NormalizeVector(&gSP.lookat[_n].x);
}

void gln64gSPVertex( uint32_t v, uint32_t n, uint32_t v0 )
{
   unsigned int i;
   Vertex *vertex;
   uint32_t address = RSP_SegmentToPhysical( v );

   if ((address + sizeof( Vertex ) * n) > RDRAMSize)
      return;

   vertex = (Vertex*)&gfx_info.RDRAM[address];

   if ((n + v0) <= INDEXMAP_SIZE)
   {
      for (i = v0; i < n + v0; i++)
      {
         uint32_t v = i;
         struct SPVertex *vtx = (struct SPVertex*)&OGL.triangles.vertices[v];

         vtx->x = vertex->x;
         vtx->y = vertex->y;
         vtx->z = vertex->z;
         vtx->s = _FIXED2FLOAT( vertex->s, 5 );
         vtx->t = _FIXED2FLOAT( vertex->t, 5 );

         if (gSP.geometryMode & G_LIGHTING)
         {
            vtx->nx = vertex->normal.x;
            vtx->ny = vertex->normal.y;
            vtx->nz = vertex->normal.z;
            vtx->a = vertex->color.a * 0.0039215689f;
         }
         else
         {
            vtx->r = vertex->color.r * 0.0039215689f;
            vtx->g = vertex->color.g * 0.0039215689f;
            vtx->b = vertex->color.b * 0.0039215689f;
            vtx->a = vertex->color.a * 0.0039215689f;
         }

         gln64gSPProcessVertex(v);
         vertex++;
      }
   }
}

void gln64gSPCIVertex( uint32_t v, uint32_t n, uint32_t v0 )
{
   unsigned int i;
   PDVertex *vertex;

   uint32_t address = RSP_SegmentToPhysical( v );

   if ((address + sizeof( PDVertex ) * n) > RDRAMSize)
      return;

   vertex = (PDVertex*)&gfx_info.RDRAM[address];

   if ((n + v0) <= INDEXMAP_SIZE)
   {
      for(i = v0; i < n + v0; i++)
      {
         uint8_t *color;
         uint32_t v = i;
         struct SPVertex *vtx = (struct SPVertex*)&OGL.triangles.vertices[v];
         vtx->x = vertex->x;
         vtx->y = vertex->y;
         vtx->z = vertex->z;
         vtx->s = _FIXED2FLOAT( vertex->s, 5 );
         vtx->t = _FIXED2FLOAT( vertex->t, 5 );

         color = (uint8_t*)&gfx_info.RDRAM[gSP.vertexColorBase + (vertex->ci & 0xff)];

         if (gSP.geometryMode & G_LIGHTING)
         {
            vtx->nx = (int8_t)color[3];
            vtx->ny = (int8_t)color[2];
            vtx->nz = (int8_t)color[1];
            vtx->a = color[0] * 0.0039215689f;
         }
         else
         {
            vtx->r = color[3] * 0.0039215689f;
            vtx->g = color[2] * 0.0039215689f;
            vtx->b = color[1] * 0.0039215689f;
            vtx->a = color[0] * 0.0039215689f;
         }

         gln64gSPProcessVertex(v);
         vertex++;
      }
   }
}

void gln64gSPDMAVertex( uint32_t v, uint32_t n, uint32_t v0 )
{
   unsigned int i;
   uint32_t address = gSP.DMAOffsets.vtx + RSP_SegmentToPhysical( v );

   if ((address + 10 * n) > RDRAMSize)
      return;

   if ((n + v0) <= INDEXMAP_SIZE)
   {
      for (i = v0; i < n + v0; i++)
      {
         uint32_t v = i;
         struct SPVertex *vtx = (struct SPVertex*)&OGL.triangles.vertices[v];

         vtx->x = *(int16_t*)&gfx_info.RDRAM[address ^ 2];
         vtx->y = *(int16_t*)&gfx_info.RDRAM[(address + 2) ^ 2];
         vtx->z = *(int16_t*)&gfx_info.RDRAM[(address + 4) ^ 2];

         if (gSP.geometryMode & G_LIGHTING)
         {
            vtx->nx = *(int8_t*)&gfx_info.RDRAM[(address + 6) ^ 3];
            vtx->ny = *(int8_t*)&gfx_info.RDRAM[(address + 7) ^ 3];
            vtx->nz = *(int8_t*)&gfx_info.RDRAM[(address + 8) ^ 3];
            vtx->a = *(uint8_t*)&gfx_info.RDRAM[(address + 9) ^ 3] * 0.0039215689f;
         }
         else
         {
            vtx->r = *(uint8_t*)&gfx_info.RDRAM[(address + 6) ^ 3] * 0.0039215689f;
            vtx->g = *(uint8_t*)&gfx_info.RDRAM[(address + 7) ^ 3] * 0.0039215689f;
            vtx->b = *(uint8_t*)&gfx_info.RDRAM[(address + 8) ^ 3] * 0.0039215689f;
            vtx->a = *(uint8_t*)&gfx_info.RDRAM[(address + 9) ^ 3] * 0.0039215689f;
         }

         gln64gSPProcessVertex(v);
         address += 10;
      }
   }
}

void gln64gSPCBFDVertex( uint32_t a, uint32_t n, uint32_t v0 )
{
	Vertex *vertex;
	uint32_t address = RSP_SegmentToPhysical(a);

	if ((address + sizeof( Vertex ) * n) > RDRAMSize)
		return;

	vertex = (Vertex*)&gfx_info.RDRAM[address];

	if ((n + v0) <= INDEXMAP_SIZE)
   {
		unsigned int i = v0;
		for (; i < n + v0; ++i)
      {
			uint32_t v = i;
         struct SPVertex *vtx = (struct SPVertex*)&OGL.triangles.vertices[v];

			vtx->x = vertex->x;
			vtx->y = vertex->y;
			vtx->z = vertex->z;
			vtx->s = _FIXED2FLOAT( vertex->s, 5 );
			vtx->t = _FIXED2FLOAT( vertex->t, 5 );
			if (gSP.geometryMode & G_LIGHTING)
         {
				const uint32_t normaleAddrOffset = (v<<1);
				vtx->nx = (float)(((int8_t*)gfx_info.RDRAM)[(gSP.vertexNormalBase + normaleAddrOffset + 0)^3]);
				vtx->ny = (float)(((int8_t*)gfx_info.RDRAM)[(gSP.vertexNormalBase + normaleAddrOffset + 1)^3]);
				vtx->nz = (float)((int8_t)(vertex->flag&0xFF));
			}
			vtx->r = vertex->color.r * 0.0039215689f;
			vtx->g = vertex->color.g * 0.0039215689f;
			vtx->b = vertex->color.b * 0.0039215689f;
			vtx->a = vertex->color.a * 0.0039215689f;
			gln64gSPProcessVertex(v);
			vertex++;
		}
	} else {
		LOG(LOG_ERROR, "Using Vertex outside buffer v0=%i, n=%i\n", v0, n);
	}
}

void gln64gSPDisplayList( uint32_t dl )
{
   uint32_t address = RSP_SegmentToPhysical( dl );

   if ((address + 8) > RDRAMSize)
      return;

   if (__RSP.PCi < (GBI.PCStackSize - 1))
   {
      __RSP.PCi++;
      __RSP.PC[__RSP.PCi] = address;
      __RSP.nextCmd = _SHIFTR( *(uint32_t*)&gfx_info.RDRAM[address], 24, 8 );
   }
}

void gln64gSPBranchList( uint32_t dl )
{
   uint32_t address = RSP_SegmentToPhysical( dl );

   if ((address + 8) > RDRAMSize)
      return;

   __RSP.PC[__RSP.PCi] = address;
   __RSP.nextCmd = _SHIFTR( *(uint32_t*)&gfx_info.RDRAM[address], 24, 8 );
}

void gln64gSPBranchLessZ( uint32_t branchdl, uint32_t vtx, float zval )
{
   float zTest;
   struct SPVertex      *v = NULL;
   uint32_t address = RSP_SegmentToPhysical( branchdl );

   if ((address + 8) > RDRAMSize)
      return;

   v = (struct SPVertex*)&OGL.triangles.vertices[vtx];
   zTest = v->z / v->w;

   if (zTest > 1.0f || zTest <= zval)
      __RSP.PC[__RSP.PCi] = address;
}

void gln64gSPDlistCount(uint32_t count, uint32_t v)
{
	uint32_t address = RSP_SegmentToPhysical( v );
	if (address == 0 || (address + 8) > RDRAMSize)
		return;

	if (__RSP.PCi >= 9)
		return;

	++__RSP.PCi;  /* go to the next PC in the stack */
	__RSP.PC[__RSP.PCi] = address;  /* jump to the address */
	__RSP.nextCmd = _SHIFTR( *(uint32_t*)&gfx_info.RDRAM[address], 24, 8 );
	__RSP.count = count + 1;
}

void gln64gSPSetDMAOffsets( uint32_t mtxoffset, uint32_t vtxoffset )
{
   gSP.DMAOffsets.mtx = mtxoffset;
   gSP.DMAOffsets.vtx = vtxoffset;
}

void gln64gSPSetDMATexOffset(uint32_t _addr)
{
	gSP.DMAOffsets.tex_offset = RSP_SegmentToPhysical(_addr);
	gSP.DMAOffsets.tex_shift = 0;
	gSP.DMAOffsets.tex_count = 0;
}

void gln64gSPSetVertexColorBase( uint32_t base )
{
   gSP.vertexColorBase = RSP_SegmentToPhysical( base );
}

void gln64gSPSetVertexNormaleBase( uint32_t base )
{
	gSP.vertexNormalBase = RSP_SegmentToPhysical( base );
}

void gln64gSPDMATriangles( uint32_t tris, uint32_t n )
{
   unsigned int i;
   int32_t v0, v1, v2;
   struct SPVertex *pVtx;
   DKRTriangle *triangles;
   uint32_t address = RSP_SegmentToPhysical( tris );

   if (address + sizeof( DKRTriangle ) * n > RDRAMSize)
      return;

   triangles = (DKRTriangle*)&gfx_info.RDRAM[address];

   for (i = 0; i < n; i++)
   {
      int mode = 0;
      if (!(triangles->flag & 0x40))
      {
         if (gSP.viewport.vscale[0] > 0)
            mode |= G_CULL_BACK;
         else
            mode |= G_CULL_FRONT;
      }

      if ((gSP.geometryMode&G_CULL_BOTH) != mode)
      {
         gSP.geometryMode &= ~G_CULL_BOTH;
         gSP.geometryMode |= mode;
         gSP.changed |= CHANGED_GEOMETRYMODE;
      }

      v0 = triangles->v0;
      v1 = triangles->v1;
      v2 = triangles->v2;
      OGL.triangles.vertices[v0].s = _FIXED2FLOAT( triangles->s0, 5 );
      OGL.triangles.vertices[v0].t = _FIXED2FLOAT( triangles->t0, 5 );
      OGL.triangles.vertices[v1].s = _FIXED2FLOAT( triangles->s1, 5 );
      OGL.triangles.vertices[v1].t = _FIXED2FLOAT( triangles->t1, 5 );
      OGL.triangles.vertices[v2].s = _FIXED2FLOAT( triangles->s2, 5 );
      OGL.triangles.vertices[v2].t = _FIXED2FLOAT( triangles->t2, 5 );
      triangles++;
   }

   OGL_DrawTriangles();
}

void gln64gSP1Quadrangle( int32_t v0, int32_t v1, int32_t v2, int32_t v3)
{
   gln64gSPTriangle( v0, v1, v2);
   gln64gSPTriangle( v0, v2, v3);
   gln64gSPFlushTriangles();
}

bool gln64gSPCullVertices( uint32_t v0, uint32_t vn )
{
   unsigned int i;
   uint32_t clip = 0;
	if (vn < v0)
   {
      // Aidyn Chronicles - The First Mage seems to pass parameters in reverse order.
      const uint32_t v = v0;
      v0 = vn;
      vn = v;
   }

   for (i = v0 + 1; i <= vn; i++)
   {
      clip |= (~OGL.triangles.vertices[i].clip) & CLIP_ALL;
      if (clip == CLIP_ALL)
         return false;
   }
   return true;
}

static void _loadSpriteImage(const struct uSprite *_pSprite)
{
	gSP.bgImage.address = RSP_SegmentToPhysical( _pSprite->imagePtr );

	gSP.bgImage.width = _pSprite->stride;
	gSP.bgImage.height = _pSprite->imageY + _pSprite->imageH;
	gSP.bgImage.format = _pSprite->imageFmt;
	gSP.bgImage.size = _pSprite->imageSiz;
	gSP.bgImage.palette = 0;
	gDP.tiles[0].textureMode = TEXTUREMODE_BGIMAGE;
	gSP.bgImage.imageX = _pSprite->imageX;
	gSP.bgImage.imageY = _pSprite->imageY;
	gSP.bgImage.scaleW = gSP.bgImage.scaleH = 1.0f;

	if (config.frameBufferEmulation.enable != 0)
	{
		struct FrameBuffer *pBuffer = FrameBuffer_FindBuffer(gSP.bgImage.address);
		if (pBuffer != NULL)
      {
			gDP.tiles[0].frameBuffer = pBuffer;
			gDP.tiles[0].textureMode = TEXTUREMODE_FRAMEBUFFER_BG;
			gDP.tiles[0].loadType = LOADTYPE_TILE;
			gDP.changed |= CHANGED_TMEM;
		}
	}
}

void gln64gSPSprite2DBase( uint32_t _base )
{
	float z, w, scaleX, scaleY;
	uint32_t flipX, flipY;
   //assert(RSP.nextCmd == 0xBE);
   const uint32_t address = RSP_SegmentToPhysical( _base );
   struct uSprite *pSprite = (struct uSprite*)&gfx_info.RDRAM[address];

   if (pSprite->tlutPtr != 0)
   {
      gln64gDPSetTextureImage( 0, 2, 1, pSprite->tlutPtr );
      gln64gDPSetTile( 0, 2, 0, 256, 7, 0, 0, 0, 0, 0, 0, 0 );
      gln64gDPLoadTLUT( 7, 0, 0, 1020, 0 );

      if (pSprite->imageFmt != G_IM_FMT_RGBA)
         gDP.otherMode.textureLUT = G_TT_RGBA16;
      else
         gDP.otherMode.textureLUT = G_TT_NONE;
   } else
      gDP.otherMode.textureLUT = G_TT_NONE;

   _loadSpriteImage(pSprite);
   gln64gSPTexture( 1.0f, 1.0f, 0, 0, true );
   gDP.otherMode.texturePersp = 1;

   z = (gDP.otherMode.depthSource == G_ZS_PRIM) ? gDP.primDepth.z : gSP.viewport.nearz;
   w = 1.0f;

   scaleX = 1.0f;
   scaleY = 1.0f;
   flipX = 0;
   flipY = 0;

   do
   {
	  float uls, ult, lrs, lrt;
      float ulx, uly, lrx, lry;
      float frameX, frameY, frameW, frameH;
      int32_t v0 = 0, v1 = 1, v2 = 2, v3 = 3;
      struct SPVertex *vtx0, *vtx1, *vtx2, *vtx3;
      uint32_t w0 = *(uint32_t*)&gfx_info.RDRAM[__RSP.PC[__RSP.PCi]];
      uint32_t w1 = *(uint32_t*)&gfx_info.RDRAM[__RSP.PC[__RSP.PCi] + 4];
      __RSP.cmd = _SHIFTR( w0, 24, 8 );

      __RSP.PC[__RSP.PCi] += 8;
      __RSP.nextCmd = _SHIFTR( *(uint32_t*)&gfx_info.RDRAM[__RSP.PC[__RSP.PCi]], 24, 8 );

      if (__RSP.cmd == 0xBE )
      {
         /* gSPSprite2DScaleFlip */
         scaleX  = _FIXED2FLOAT( _SHIFTR(w1, 16, 16), 10 );
         scaleY  = _FIXED2FLOAT( _SHIFTR(w1,  0, 16), 10 );
         flipX = _SHIFTR(w0, 8, 8);
         flipY = _SHIFTR(w0, 0, 8);
         continue;
      }

      /* gSPSprite2DDraw */
      frameX = _FIXED2FLOAT(((int16_t)_SHIFTR(w1, 16, 16)), 2);
      frameY = _FIXED2FLOAT(((int16_t)_SHIFTR(w1,  0, 16)), 2);
      frameW = pSprite->imageW / scaleX;
      frameH = pSprite->imageH / scaleY;

      if (flipX != 0)
      {
         ulx = frameX + frameW;
         lrx = frameX;
      }
      else
      {
         ulx = frameX;
         lrx = frameX + frameW;
      }

      if (flipY != 0)
      {
         uly = frameY + frameH;
         lry = frameY;
      }
      else
      {
         uly = frameY;
         lry = frameY + frameH;
      }

      uls = pSprite->imageX;
      ult = pSprite->imageY;
      lrs = uls + pSprite->imageW - 1;
      lrt = ult + pSprite->imageH - 1;

      /* Hack for WCW Nitro. TODO : activate it later.
         if (WCW_NITRO) {
         gSP.bgImage.height /= scaleY;
         gSP.bgImage.imageY /= scaleY;
         ult /= scaleY;
         lrt /= scaleY;
         gSP.bgImage.width *= scaleY;
         }
         */


      vtx0 = (struct SPVertex*)&OGL.triangles.vertices[v0];
      vtx0->x = ulx;
      vtx0->y = uly;
      vtx0->z = z;
      vtx0->w = w;
      vtx0->s = uls;
      vtx0->t = ult;
      vtx1 = (struct SPVertex*)&OGL.triangles.vertices[v1];
      vtx1->x = lrx;
      vtx1->y = uly;
      vtx1->z = z;
      vtx1->w = w;
      vtx1->s = lrs;
      vtx1->t = ult;
      vtx2 = (struct SPVertex*)&OGL.triangles.vertices[v2];
      vtx2->x = ulx;
      vtx2->y = lry;
      vtx2->z = z;
      vtx2->w = w;
      vtx2->s = uls;
      vtx2->t = lrt;
      vtx3 = (struct SPVertex*)&OGL.triangles.vertices[v3];
      vtx3->x = lrx;
      vtx3->y = lry;
      vtx3->z = z;
      vtx3->w = w;
      vtx3->s = lrs;
      vtx3->t = lrt;

      OGL_DrawLLETriangle(4);
   } while (__RSP.nextCmd == 0xBD || __RSP.nextCmd == 0xBE);
}

void gln64gSPCullDisplayList( uint32_t v0, uint32_t vn )
{
   if (gln64gSPCullVertices( v0, vn ))
      gSPEndDisplayList();
}

void gln64gSPPopMatrixN( uint32_t param, uint32_t num )
{
   if (gSP.matrix.modelViewi > num - 1)
   {
      gSP.matrix.modelViewi -= num;

      gSP.changed |= CHANGED_MATRIX;
   }
}

void gln64gSPPopMatrix( uint32_t param )
{
   switch (param)
   {
      case 0: // modelview
         if (gSP.matrix.modelViewi > 0)
         {
            gSP.matrix.modelViewi--;

            gSP.changed |= CHANGED_MATRIX;
         }
         break;
      case 1: // projection, can't
         break;
      default:
         break;
   }
}

void gln64gSPSegment( int32_t seg, int32_t base )
{
    gSP.segment[seg] = base;
}

void gln64gSPClipRatio( uint32_t r )
{
}

void gln64gSPInsertMatrix( uint32_t where, uint32_t num )
{
   float fraction, integer;

   if (gSP.changed & CHANGED_MATRIX)
      gln64gSPCombineMatrices();

   if ((where & 0x3) || (where > 0x3C))
      return;

   if (where < 0x20)
   {
      fraction = modff( gSP.matrix.combined[0][where >> 1], &integer );
      gSP.matrix.combined[0][where >> 1]
       = (float)_SHIFTR( num, 16, 16 ) + abs( (int)fraction );

      fraction = modff( gSP.matrix.combined[0][(where >> 1) + 1], &integer );
      gSP.matrix.combined[0][(where >> 1) + 1]
       = (float)_SHIFTR( num, 0, 16 ) + abs( (int)fraction );
   }
   else
   {
      float newValue;

      fraction = modff( gSP.matrix.combined[0][(where - 0x20) >> 1], &integer );
      newValue = integer + _FIXED2FLOAT( _SHIFTR( num, 16, 16 ), 16);

      // Make sure the sign isn't lost
      if ((integer == 0.0f) && (fraction != 0.0f))
         newValue = newValue * (fraction / abs( (int)fraction ));

      gSP.matrix.combined[0][(where - 0x20) >> 1] = newValue;

      fraction = modff( gSP.matrix.combined[0][((where - 0x20) >> 1) + 1], &integer );
      newValue = integer + _FIXED2FLOAT( _SHIFTR( num, 0, 16 ), 16 );

      // Make sure the sign isn't lost
      if ((integer == 0.0f) && (fraction != 0.0f))
         newValue = newValue * (fraction / abs( (int)fraction ));

      gSP.matrix.combined[0][((where - 0x20) >> 1) + 1] = newValue;
   }
}

void gln64gSPModifyVertex( uint32_t vtx, uint32_t where, uint32_t val )
{
   int32_t v = vtx;
   struct SPVertex *vtx0 = (struct SPVertex*)&OGL.triangles.vertices[v];

   switch (where)
   {
      case G_MWO_POINT_RGBA:
         vtx0->r = _SHIFTR( val, 24, 8 ) * 0.0039215689f;
         vtx0->g = _SHIFTR( val, 16, 8 ) * 0.0039215689f;
         vtx0->b = _SHIFTR( val, 8, 8 ) * 0.0039215689f;
         vtx0->a = _SHIFTR( val, 0, 8 ) * 0.0039215689f;
         break;
      case G_MWO_POINT_ST:
         vtx0->s = _FIXED2FLOAT( (int16_t)_SHIFTR( val, 16, 16 ), 5 ) / gSP.texture.scales;
         vtx0->t = _FIXED2FLOAT( (int16_t)_SHIFTR( val, 0, 16 ), 5 ) / gSP.texture.scalet;
         break;
      case G_MWO_POINT_XYSCREEN:
         {
            float scrX = _FIXED2FLOAT( (int16_t)_SHIFTR( val, 16, 16 ), 2 );
            float scrY = _FIXED2FLOAT( (int16_t)_SHIFTR( val, 0, 16 ), 2 );
            vtx0->x = (scrX - gSP.viewport.vtrans[0]) / gSP.viewport.vscale[0];
            vtx0->x *= vtx0->w;
            vtx0->y = -(scrY - gSP.viewport.vtrans[1]) / gSP.viewport.vscale[1];
            vtx0->y *= vtx0->w;
            vtx0->clip &= ~(CLIP_POSX | CLIP_NEGX | CLIP_POSY | CLIP_NEGY);
         }
         break;
      case G_MWO_POINT_ZSCREEN:
         {
            float scrZ = _FIXED2FLOAT((int16_t)_SHIFTR(val, 16, 16), 15);
            vtx0->z = (scrZ - gSP.viewport.vtrans[2]) / (gSP.viewport.vscale[2]);
            vtx0->z *= vtx0->w;
            vtx0->clip &= ~CLIP_Z;
         }
         break;
   }
}

void gln64gSPNumLights( int32_t n )
{
   if (n <= 12)
   {
      gSP.numLights = n;
      if (config.generalEmulation.enableHWLighting != 0)
         gSP.changed |= CHANGED_LIGHT;
   }
}

void gln64gSPLightColor( uint32_t lightNum, uint32_t packedColor )
{
   lightNum--;

   if (lightNum < 8)
   {
      gSP.lights[lightNum].r = _SHIFTR( packedColor, 24, 8 ) * 0.0039215689f;
      gSP.lights[lightNum].g = _SHIFTR( packedColor, 16, 8 ) * 0.0039215689f;
      gSP.lights[lightNum].b = _SHIFTR( packedColor, 8, 8 ) * 0.0039215689f;
		if (config.generalEmulation.enableHWLighting != 0)
			gSP.changed |= CHANGED_LIGHT;
   }
}

void gln64gSPFogFactor( int16_t fm, int16_t fo )
{
   gSP.fog.multiplier = fm;
   gSP.fog.offset     = fo;

   gSP.changed |= CHANGED_FOGPOSITION;
}

void gln64gSPPerspNormalize( uint16_t scale )
{
}

void gln64gSPCoordMod(uint32_t _w0, uint32_t _w1)
{
   uint32_t idx, pos;

	if ((_w0&8) != 0)
		return;
	idx = _SHIFTR(_w0, 1, 2);
	pos = _w0&0x30;
	if (pos == 0)
   {
		gSP.vertexCoordMod[0+idx] = (float)(int16_t)_SHIFTR(_w1, 16, 16);
		gSP.vertexCoordMod[1+idx] = (float)(int16_t)_SHIFTR(_w1, 0, 16);
	}
   else if (pos == 0x10)
   {
		assert(idx < 3);
		gSP.vertexCoordMod[4+idx] = _SHIFTR(_w1, 16, 16)/65536.0f;
		gSP.vertexCoordMod[5+idx] = _SHIFTR(_w1, 0, 16)/65536.0f;
		gSP.vertexCoordMod[12+idx] = gSP.vertexCoordMod[0+idx] + gSP.vertexCoordMod[4+idx];
		gSP.vertexCoordMod[13+idx] = gSP.vertexCoordMod[1+idx] + gSP.vertexCoordMod[5+idx];
	} else if (pos == 0x20) {
		gSP.vertexCoordMod[8+idx] = (float)(int16_t)_SHIFTR(_w1, 16, 16);
		gSP.vertexCoordMod[9+idx] = (float)(int16_t)_SHIFTR(_w1, 0, 16);
	}
}

void gln64gSPTexture( float sc, float tc, int32_t level, int32_t tile, int32_t on )
{
   gSP.texture.on = on;
   if (on == 0)
      return;

   gSP.texture.scales = sc;
   gSP.texture.scalet = tc;

   if (gSP.texture.scales == 0.0f)
      gSP.texture.scales = 1.0f;
   if (gSP.texture.scalet == 0.0f)
      gSP.texture.scalet = 1.0f;

   gSP.texture.level = level;

   gSP.texture.tile = tile;
   gSP.textureTile[0] = &gDP.tiles[tile];
   gSP.textureTile[1] = &gDP.tiles[(tile + 1) & 7];

   gSP.changed |= CHANGED_TEXTURE;
}

void gln64gSPGeometryMode( uint32_t clear, uint32_t set )
{
   gSP.geometryMode = (gSP.geometryMode & ~clear) | set;
   gSP.changed |= CHANGED_GEOMETRYMODE;
}

void gln64gSPSetGeometryMode( uint32_t mode )
{
   gSP.geometryMode |= mode;
   gSP.changed |= CHANGED_GEOMETRYMODE;
}

void gln64gSPClearGeometryMode( uint32_t mode )
{
   gSP.geometryMode &= ~mode;
   gSP.changed |= CHANGED_GEOMETRYMODE;
}

void gln64gSPSetOtherMode_H(uint32_t _length, uint32_t _shift, uint32_t _data)
{
	const uint32_t mask = (((uint64_t)1 << _length) - 1) << _shift;
	gDP.otherMode.h = (gDP.otherMode.h&(~mask)) | _data;

	if (mask & 0x00300000)  // cycle type
		gDP.changed |= CHANGED_CYCLETYPE;
}

void gln64gSPSetOtherMode_L(uint32_t _length, uint32_t _shift, uint32_t _data)
{
	const uint32_t mask = (((uint64_t)1 << _length) - 1) << _shift;
	gDP.otherMode.l = (gDP.otherMode.l&(~mask)) | _data;

	if (mask & 0xFFFFFFF8)  // rendermode / blender bits
		gDP.changed |= CHANGED_RENDERMODE;
}

void gln64gSPLine3D( int32_t v0, int32_t v1, int32_t flag )
{
   OGL_DrawLine(v0, v1, 1.5f );
}

void gln64gSPLineW3D( int32_t v0, int32_t v1, int32_t wd, int32_t flag )
{
   OGL_DrawLine(v0, v1, 1.5f + wd * 0.5f );
}

static
void _loadBGImage(const struct uObjScaleBg * _bgInfo, bool _loadScale)
{
	uint32_t imageW, imageH;
	gSP.bgImage.address = RSP_SegmentToPhysical( _bgInfo->imagePtr );

	imageW = _bgInfo->imageW >> 2;
	gSP.bgImage.width = imageW - imageW%2;
	imageH = _bgInfo->imageH >> 2;
	gSP.bgImage.height = imageH - imageH%2;
	gSP.bgImage.format = _bgInfo->imageFmt;
	gSP.bgImage.size = _bgInfo->imageSiz;
	gSP.bgImage.palette = _bgInfo->imagePal;
	gDP.tiles[0].textureMode = TEXTUREMODE_BGIMAGE;
	gSP.bgImage.imageX = _FIXED2FLOAT( _bgInfo->imageX, 5 );
	gSP.bgImage.imageY = _FIXED2FLOAT( _bgInfo->imageY, 5 );
	if (_loadScale) {
		gSP.bgImage.scaleW = _FIXED2FLOAT( _bgInfo->scaleW, 10 );
		gSP.bgImage.scaleH = _FIXED2FLOAT( _bgInfo->scaleH, 10 );
	} else
		gSP.bgImage.scaleW = gSP.bgImage.scaleH = 1.0f;

	if (config.frameBufferEmulation.enable)
   {
		struct FrameBuffer *pBuffer = FrameBuffer_FindBuffer(gSP.bgImage.address);
      /* TODO/FIXME */
		if ((pBuffer != NULL) && pBuffer->m_size == gSP.bgImage.size && (/* !pBuffer->m_isDepthBuffer || */ pBuffer->m_changed))
      {
			gDP.tiles[0].frameBuffer = pBuffer;
			gDP.tiles[0].textureMode = TEXTUREMODE_FRAMEBUFFER_BG;
			gDP.tiles[0].loadType = LOADTYPE_TILE;
			gDP.changed |= CHANGED_TMEM;
		}
	}
}

struct ObjCoordinates
{
	float ulx, uly, lrx, lry;
	float uls, ult, lrs, lrt;
	float z, w;
};

struct ObjData
{
	float scaleW;
	float scaleH;
	uint32_t imageW;
	uint32_t imageH;
	float X0;
	float X1;
	float Y0;
	float Y1;
	bool flipS, flipT;
};

void ObjData_new(struct ObjData *obj, const struct uObjSprite *_pObjSprite)
{
   obj->scaleW = _FIXED2FLOAT(_pObjSprite->scaleW, 10);
   obj->scaleH = _FIXED2FLOAT(_pObjSprite->scaleH, 10);
   obj->imageW = _pObjSprite->imageW >> 5;
   obj->imageH = _pObjSprite->imageH >> 5;
   obj->X0     = _FIXED2FLOAT(_pObjSprite->objX, 2);
   obj->X1     = obj->X0 + obj->imageW / obj->scaleW;
   obj->Y0     = _FIXED2FLOAT(_pObjSprite->objY, 2);
   obj->Y1     = obj->Y0 + obj->imageH / obj->scaleH;
   obj->flipS  = (_pObjSprite->imageFlags & 0x01) != 0;
   obj->flipT  = (_pObjSprite->imageFlags & 0x10) != 0;
}

void ObjCoordinates_new(struct ObjCoordinates *obj, const struct uObjSprite *_pObjSprite, bool _useMatrix)
{
   struct ObjData data;
   
   ObjData_new(&data, _pObjSprite);

   obj->ulx = data.X0;
   obj->lrx = data.X1;
   obj->uly = data.Y0;
   obj->lry = data.Y1;

   if (_useMatrix)
   {
      obj->ulx = obj->ulx / gSP.objMatrix.baseScaleX + gSP.objMatrix.X;
      obj->lrx = obj->lrx / gSP.objMatrix.baseScaleX + gSP.objMatrix.X;
      obj->uly = obj->uly / gSP.objMatrix.baseScaleY + gSP.objMatrix.Y;
      obj->lry = obj->lry / gSP.objMatrix.baseScaleY + gSP.objMatrix.Y;
   }

   obj->uls = obj->ult = 0;
   obj->lrs = data.imageW - 1;
   obj->lrt = data.imageH - 1;
   if (data.flipS)
   {
      obj->uls = obj->lrs;
      obj->lrs = 0;
   }
   if (data.flipT)
   {
      obj->ult = obj->lrt;
      obj->lrt = 0;
   }

   obj->z = (gDP.otherMode.depthSource == G_ZS_PRIM) ? gDP.primDepth.z : gSP.viewport.nearz;
   obj->w = 1.0f;
}

void ObjCoordinates2_new(struct ObjCoordinates *obj, const struct uObjScaleBg * _pObjScaleBg)
{
   const float frameX = _FIXED2FLOAT(_pObjScaleBg->frameX, 2);
   const float frameY = _FIXED2FLOAT(_pObjScaleBg->frameY, 2);
   const float frameW = _FIXED2FLOAT(_pObjScaleBg->frameW, 2);
   const float frameH = _FIXED2FLOAT(_pObjScaleBg->frameH, 2);
   const float imageX = gSP.bgImage.imageX;
   const float imageY = gSP.bgImage.imageY;
   const float imageW = (float)(_pObjScaleBg->imageW>>2);
   const float imageH = (float)(_pObjScaleBg->imageH >> 2);
   //		const float imageW = (float)gSP.bgImage.width;
   //		const float imageH = (float)gSP.bgImage.height;
   const float scaleW = gSP.bgImage.scaleW;
   const float scaleH = gSP.bgImage.scaleH;

   obj->ulx = frameX;
   obj->uly = frameY;
   obj->lrx = frameX + MIN(imageW/scaleW, frameW) - 1.0f;
   obj->lry = frameY + MIN(imageH/scaleH, frameH) - 1.0f;
   if (gDP.otherMode.cycleType == G_CYC_COPY)
   {
      obj->lrx += 1.0f;
      obj->lry += 1.0f;;
   }

   obj->uls = imageX;
   obj->ult = imageY;
   obj->lrs = obj->uls + (obj->lrx - obj->ulx) * scaleW;
   obj->lrt = obj->ult + (obj->lry - obj->uly) * scaleH;
   if (gDP.otherMode.cycleType != G_CYC_COPY)
   {
      if ((gSP.objRendermode&G_OBJRM_SHRINKSIZE_1) != 0)
      {
         obj->lrs -= 1.0f / scaleW;
         obj->lrt -= 1.0f / scaleH;
      }
      else if ((gSP.objRendermode&G_OBJRM_SHRINKSIZE_2) != 0)
      {
         obj->lrs -= 1.0f;
         obj->lrt -= 1.0f;
      }
   }

   if ((_pObjScaleBg->imageFlip & 0x01) != 0)
   {
      obj->ulx = obj->lrx;
      obj->lrx = frameX;
   }

   obj->z = (gDP.otherMode.depthSource == G_ZS_PRIM) ? gDP.primDepth.z : gSP.viewport.nearz;
   obj->w = 1.0f;
}

static void gln64gSPDrawObjRect(const struct ObjCoordinates *_coords)
{
   struct SPVertex *vtx0, *vtx1, *vtx2, *vtx3;
	uint32_t v0 = 0, v1 = 1, v2 = 2, v3 = 3;
   vtx0 = (struct SPVertex*)&OGL.triangles.vertices[v0];
	vtx0->x = _coords->ulx;
	vtx0->y = _coords->uly;
	vtx0->z = _coords->z;
	vtx0->w = _coords->w;
	vtx0->s = _coords->uls;
	vtx0->t = _coords->ult;
	vtx1 = (struct SPVertex*)&OGL.triangles.vertices[v1];
	vtx1->x = _coords->lrx;
	vtx1->y = _coords->uly;
	vtx1->z = _coords->z;
	vtx1->w = _coords->w;
	vtx1->s = _coords->lrs;
	vtx1->t = _coords->ult;
	vtx2 = (struct SPVertex*)&OGL.triangles.vertices[v2];
	vtx2->x = _coords->ulx;
	vtx2->y = _coords->lry;
	vtx2->z = _coords->z;
	vtx2->w = _coords->w;
	vtx2->s = _coords->uls;
	vtx2->t = _coords->lrt;
	vtx3 = (struct SPVertex*)&OGL.triangles.vertices[v3];
	vtx3->x = _coords->lrx;
	vtx3->y = _coords->lry;
	vtx3->z = _coords->z;
	vtx3->w = _coords->w;
	vtx3->s = _coords->lrs;
	vtx3->t = _coords->lrt;

   OGL_DrawLLETriangle(4);
	gDP.colorImage.height = (uint32_t)(MAX(gDP.colorImage.height, (uint32_t)gDP.scissor.lry));
}

void gln64gSPBgRect1Cyc( uint32_t _bg )
{
   struct ObjCoordinates objCoords;
	const uint32_t address = RSP_SegmentToPhysical( _bg );
	struct uObjScaleBg *objScaleBg = (struct uObjScaleBg*)&gfx_info.RDRAM[address];
	_loadBGImage(objScaleBg, true);

#ifdef GL_IMAGE_TEXTURES_SUPPORT
	if (gSP.bgImage.address == gDP.depthImageAddress || depthBufferList().findBuffer(gSP.bgImage.address) != NULL)
		_copyDepthBuffer();
	// Zelda MM uses depth buffer copy in LoT and in pause screen.
	// In later case depth buffer is used as temporal color buffer, and usual rendering must be used.
	// Since both situations are hard to distinguish, do the both depth buffer copy and bg rendering.
#endif // GL_IMAGE_TEXTURES_SUPPORT

	gDP.otherMode.cycleType = G_CYC_1CYCLE;
	gDP.changed |= CHANGED_CYCLETYPE;
	gln64gSPTexture(1.0f, 1.0f, 0, 0, true);
	gDP.otherMode.texturePersp = 1;

	ObjCoordinates2_new(&objCoords, objScaleBg);
	gln64gSPDrawObjRect(&objCoords);
}

void gln64gSPBgRectCopy( uint32_t _bg )
{
   struct ObjCoordinates objCoords;
	const uint32_t address = RSP_SegmentToPhysical( _bg );
	struct uObjScaleBg *objBg = (struct uObjScaleBg*)&gfx_info.RDRAM[address];
	_loadBGImage(objBg, false);

#ifdef GL_IMAGE_TEXTURES_SUPPORT
	if (gSP.bgImage.address == gDP.depthImageAddress || depthBufferList().findBuffer(gSP.bgImage.address) != NULL)
		_copyDepthBuffer();
	// See comment to gSPBgRect1Cyc
#endif // GL_IMAGE_TEXTURES_SUPPORT

	gln64gSPTexture( 1.0f, 1.0f, 0, 0, true );
	gDP.otherMode.texturePersp = 1;

   ObjCoordinates2_new(&objCoords, objBg);
	gln64gSPDrawObjRect(&objCoords);
}

void gln64gSPObjLoadTxtr( uint32_t tx )
{
   uint32_t address = RSP_SegmentToPhysical( tx );
   uObjTxtr *objTxtr = (uObjTxtr*)&gfx_info.RDRAM[address];

   if ((gSP.status[objTxtr->block.sid >> 2] & objTxtr->block.mask) != objTxtr->block.flag)
   {
      switch (objTxtr->block.type)
      {
         case G_OBJLT_TXTRBLOCK:
            gln64gDPSetTextureImage( 0, 1, 0, objTxtr->block.image );
            gln64gDPSetTile( 0, 1, 0, objTxtr->block.tmem, 7, 0, 0, 0, 0, 0, 0, 0 );
            gln64gDPLoadBlock( 7, 0, 0, ((objTxtr->block.tsize + 1) << 3) - 1, objTxtr->block.tline );
            break;
         case G_OBJLT_TXTRTILE:
            gln64gDPSetTextureImage( 0, 1, (objTxtr->tile.twidth + 1) << 1, objTxtr->tile.image );
            gln64gDPSetTile( 0, 1, (objTxtr->tile.twidth + 1) >> 2, objTxtr->tile.tmem, 7, 0, 0, 0, 0, 0, 0, 0 );
            gln64gDPLoadTile( 7, 0, 0, (((objTxtr->tile.twidth + 1) << 1) - 1) << 2, (((objTxtr->tile.theight + 1) >> 2) - 1) << 2 );
            break;
         case G_OBJLT_TLUT:
            gln64gDPSetTextureImage( 0, 2, 1, objTxtr->tlut.image );
            gln64gDPSetTile( 0, 2, 0, objTxtr->tlut.phead, 7, 0, 0, 0, 0, 0, 0, 0 );
            gln64gDPLoadTLUT( 7, 0, 0, objTxtr->tlut.pnum << 2, 0 );
            break;
      }
      gSP.status[objTxtr->block.sid >> 2] = (gSP.status[objTxtr->block.sid >> 2] & ~objTxtr->block.mask) | (objTxtr->block.flag & objTxtr->block.mask);
   }
}

static void gln64gSPSetSpriteTile(const struct uObjSprite *_pObjSprite)
{
	const uint32_t w = MAX(_pObjSprite->imageW >> 5, 1);
	const uint32_t h = MAX(_pObjSprite->imageH >> 5, 1);

	gln64gDPSetTile( _pObjSprite->imageFmt, _pObjSprite->imageSiz, _pObjSprite->imageStride, _pObjSprite->imageAdrs, 0, _pObjSprite->imagePal, G_TX_CLAMP, G_TX_CLAMP, 0, 0, 0, 0 );
	gln64gDPSetTileSize( 0, 0, 0, (w - 1) << 2, (h - 1) << 2 );
	gln64gSPTexture( 1.0f, 1.0f, 0, 0, true );
	gDP.otherMode.texturePersp = 1;
}

static void _drawYUVImageToFrameBuffer(const struct ObjCoordinates *_objCoords)
{
   uint16_t h, w;
   uint32_t width, height, *mb, *dst;
   struct FrameBuffer *pBuffer;
	const uint32_t ulx = (uint32_t)_objCoords->ulx;
	const uint32_t uly = (uint32_t)_objCoords->uly;
	const uint32_t lrx = (uint32_t)_objCoords->lrx;
	const uint32_t lry = (uint32_t)_objCoords->lry;
	const uint32_t ci_width = gDP.colorImage.width;
	const uint32_t ci_height = gDP.colorImage.height;
	if (ulx >= ci_width)
		return;
	if (uly >= ci_height)
		return;
	width = 16;
	height = 16;
	if (lrx > ci_width)
		width = ci_width - ulx;
	if (lry > ci_height)
		height = ci_height - uly;
	mb = (uint32_t*)(gfx_info.RDRAM + gDP.textureImage.address); //pointer to the first macro block
	dst = (uint32_t*)(gfx_info.RDRAM + gDP.colorImage.address);
	dst += ulx + uly * ci_width;

	/* YUV macro block contains 16x16 texture. 
    * we need to put it in the proper place inside cimg */
	for (h = 0; h < 16; h++)
   {
		for (w = 0; w < 16; w += 2)
      {
         uint32_t t = *(mb++); //each uint32_t contains 2 pixels
         if ((h < height) && (w < width)) //clipping. texture image may be larger than color image
         {
            uint8_t y0 = (uint8_t)t & 0xFF;
            uint8_t v = (uint8_t)(t >> 8) & 0xFF;
            uint8_t y1 = (uint8_t)(t >> 16) & 0xFF;
            uint8_t u = (uint8_t)(t >> 24) & 0xFF;
            *(dst++) = YUVtoRGBA16(y0, u, v);
            *(dst++) = YUVtoRGBA16(y1, u, v);
         }
      }
		dst += ci_width - 16;
	}

	pBuffer = FrameBuffer_GetCurrent();
	if (pBuffer != NULL)
   {
#if 0
		pBuffer->m_isOBScreen = true;
#endif
   }
}

void gln64gSPObjRectangle(uint32_t _sp)
{
   struct ObjCoordinates objCoords;
	const uint32_t address = RSP_SegmentToPhysical(_sp);
	struct uObjSprite *objSprite = (struct uObjSprite*)&gfx_info.RDRAM[address];

	gln64gSPSetSpriteTile(objSprite);
   ObjCoordinates_new(&objCoords, objSprite, false);
	gln64gSPDrawObjRect(&objCoords);
}

void gln64gSPObjRectangleR(uint32_t _sp)
{
   struct ObjCoordinates objCoords;
   const uint32_t address = RSP_SegmentToPhysical(_sp);
   const struct uObjSprite *objSprite = (struct uObjSprite*)&gfx_info.RDRAM[address];

   gln64gSPSetSpriteTile(objSprite);
   ObjCoordinates_new(&objCoords, objSprite, true);

   if (objSprite->imageFmt == G_IM_FMT_YUV && (config.generalEmulation.hacks & hack_Ogre64)) //Ogre Battle needs to copy YUV texture to frame buffer
      _drawYUVImageToFrameBuffer(&objCoords);
   gln64gSPDrawObjRect(&objCoords);
}

void gln64gSPObjSprite( uint32_t _sp )
{
   struct SPVertex *vtx0, *vtx1, *vtx2, *vtx3;
   float uls, lrs, ult, lrt;
	float z;
	int32_t v0, v1, v2, v3;
   float ulx, uly, lrx, lry;
   struct ObjData data;
	const uint32_t address = RSP_SegmentToPhysical( _sp );
	struct uObjSprite *objSprite = (struct uObjSprite*)&gfx_info.RDRAM[address];

	gln64gSPSetSpriteTile(objSprite);
	ObjData_new(&data, objSprite);

	ulx = data.X0;
	uly = data.Y0;
	lrx = data.X1;
	lry = data.Y1;

	uls = 0;
   lrs =  data.imageW - 1;
   ult = 0;
   lrt = data.imageH - 1;

	if (objSprite->imageFlags & 0x01) { // flipS
		uls = lrs;
		lrs = 0;
	}
	if (objSprite->imageFlags & 0x10) { // flipT
		ult = lrt;
		lrt = 0;
	}
	z = (gDP.otherMode.depthSource == G_ZS_PRIM) ? gDP.primDepth.z : gSP.viewport.nearz;

	v0 = 0;
	v1 = 1;
	v2 = 2;
	v3 = 3;

   vtx0 = (struct SPVertex*)&OGL.triangles.vertices[v0];
	vtx0->x = gSP.objMatrix.A * ulx + gSP.objMatrix.B * uly + gSP.objMatrix.X;
	vtx0->y = gSP.objMatrix.C * ulx + gSP.objMatrix.D * uly + gSP.objMatrix.Y;
	vtx0->z = z;
	vtx0->w = 1.0f;
	vtx0->s = uls;
	vtx0->t = ult;
   vtx1 = (struct SPVertex*)&OGL.triangles.vertices[v1];
	vtx1->x = gSP.objMatrix.A * lrx + gSP.objMatrix.B * uly + gSP.objMatrix.X;
	vtx1->y = gSP.objMatrix.C * lrx + gSP.objMatrix.D * uly + gSP.objMatrix.Y;
	vtx1->z = z;
	vtx1->w = 1.0f;
	vtx1->s = lrs;
	vtx1->t = ult;
   vtx2 = (struct SPVertex*)&OGL.triangles.vertices[v2];
	vtx2->x = gSP.objMatrix.A * ulx + gSP.objMatrix.B * lry + gSP.objMatrix.X;
	vtx2->y = gSP.objMatrix.C * ulx + gSP.objMatrix.D * lry + gSP.objMatrix.Y;
	vtx2->z = z;
	vtx2->w = 1.0f;
	vtx2->s = uls;
	vtx2->t = lrt;
   vtx3 = (struct SPVertex*)&OGL.triangles.vertices[v3];
	vtx3->x = gSP.objMatrix.A * lrx + gSP.objMatrix.B * lry + gSP.objMatrix.X;
	vtx3->y = gSP.objMatrix.C * lrx + gSP.objMatrix.D * lry + gSP.objMatrix.Y;
	vtx3->z = z;
	vtx3->w = 1.0f;
	vtx3->s = lrs;
	vtx3->t = lrt;

   OGL_DrawLLETriangle(4);

#ifdef NEW
   /* TODO/FIXME */
	frameBufferList().setBufferChanged();
#endif
	gDP.colorImage.height = (uint32_t)(MAX( gDP.colorImage.height, (uint32_t)gDP.scissor.lry ));
}

void gln64gSPObjLoadTxSprite( uint32_t txsp )
{
   gln64gSPObjLoadTxtr( txsp );
   gln64gSPObjSprite( txsp + sizeof( uObjTxtr ) );
}

void gln64gSPObjLoadTxRect(uint32_t txsp)
{
	gln64gSPObjLoadTxtr(txsp);
	gln64gSPObjRectangle(txsp + sizeof(uObjTxtr));
}

void gln64gSPObjLoadTxRectR( uint32_t txsp )
{
	gln64gSPObjLoadTxtr( txsp );
	gln64gSPObjRectangleR( txsp + sizeof( uObjTxtr ) );
}

void gln64gSPObjMatrix( uint32_t mtx )
{
   uint32_t address = RSP_SegmentToPhysical( mtx );
   uObjMtx *objMtx = (uObjMtx*)&gfx_info.RDRAM[address];

   gSP.objMatrix.A = _FIXED2FLOAT( objMtx->A, 16 );
   gSP.objMatrix.B = _FIXED2FLOAT( objMtx->B, 16 );
   gSP.objMatrix.C = _FIXED2FLOAT( objMtx->C, 16 );
   gSP.objMatrix.D = _FIXED2FLOAT( objMtx->D, 16 );
   gSP.objMatrix.X = _FIXED2FLOAT( objMtx->X, 2 );
   gSP.objMatrix.Y = _FIXED2FLOAT( objMtx->Y, 2 );
   gSP.objMatrix.baseScaleX = _FIXED2FLOAT( objMtx->BaseScaleX, 10 );
   gSP.objMatrix.baseScaleY = _FIXED2FLOAT( objMtx->BaseScaleY, 10 );
}

void gln64gSPObjSubMatrix( uint32_t mtx )
{
	uint32_t address = RSP_SegmentToPhysical(mtx);
	uObjSubMtx *objMtx = (uObjSubMtx*)&gfx_info.RDRAM[address];
	gSP.objMatrix.X = _FIXED2FLOAT(objMtx->X, 2);
	gSP.objMatrix.Y = _FIXED2FLOAT(objMtx->Y, 2);
	gSP.objMatrix.baseScaleX = _FIXED2FLOAT(objMtx->BaseScaleX, 10);
	gSP.objMatrix.baseScaleY = _FIXED2FLOAT(objMtx->BaseScaleY, 10);
}

void gln64gSPObjRendermode(uint32_t _mode)
{
	gSP.objRendermode = _mode;
}

void (*gln64gSPTransformVertex)(float vtx[4], float mtx[4][4])  = gln64gSPTransformVertex_default;
void (*gln64gSPLightVertex)(void *data)                         = gln64gSPLightVertex_default;
void (*gln64gSPPointLightVertex)(void *data, float * _vPos)     = gln64gSPPointLightVertex_default;
void (*gln64gSPBillboardVertex)(uint32_t v, uint32_t i)         = gln64gSPBillboardVertex_default;

void gSPSetupFunctions(void)
{
   if (GBI_GetCurrentMicrocodeType() != F3DEX2CBFD)
   {
      gln64gSPLightVertex      = gln64gSPLightVertex_default;
      gln64gSPPointLightVertex = gln64gSPPointLightVertex_default;
      return;
   }

   gln64gSPLightVertex      = gln64gSPLightVertex_CBFD;
   gln64gSPPointLightVertex = gln64gSPPointLightVertex_CBFD;
}
