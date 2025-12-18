#include <stdint.h>

#include "3dmath.h"
#include "../../../Graphics/3dmath.h"
#include "../../../Graphics/RDP/gDP_state.h"
#include "../../../Graphics/RSP/gSP_state.h"

#include "glide64_gDP.h"
#include "glide64_gSP.h"

int vtx_last = 0;

typedef struct 
{
   int16_t y;
   int16_t x;
   uint16_t idx;

   int16_t z;

   int16_t t;
   int16_t s;
} vtx_uc7;

void uc6_obj_sprite(uint32_t w0, uint32_t w1);

static INLINE void calculateVertexFog(VERTEX *v)
{
   if (rdp.flags & FOG_ENABLED)
   {
      if (v->w < 0.0f)
         v->f = 0.0f;
      else
         v->f = MIN(255.0f, MAX(0.0f, v->z_w * gSP.fog.multiplier + gSP.fog.offset));
      v->a = (uint8_t)v->f;
   }
   else
      v->f = 1.0f;
}

void glide64gSPLightVertex(void *data)
{
   uint32_t i;
   float color[3];
   VERTEX *v = (VERTEX*)data;

   color[0] = rdp.light[gSP.numLights].col[0];
   color[1] = rdp.light[gSP.numLights].col[1];
   color[2] = rdp.light[gSP.numLights].col[2];

   for (i = 0; i < gSP.numLights; i++)
   {
      float intensity = DotProduct (rdp.light_vector[i], v->vec);

      if (intensity < 0.0f) 
         intensity = 0.0f;

      color[0] += rdp.light[i].col[0] * intensity;
      color[1] += rdp.light[i].col[1] * intensity;
      color[2] += rdp.light[i].col[2] * intensity;
   }

   v->r = (uint8_t)(255.0f * clamp_float(color[0], 0.0, 1.0));
   v->g = (uint8_t)(255.0f * clamp_float(color[1], 0.0, 1.0));
   v->b = (uint8_t)(255.0f * clamp_float(color[2], 0.0, 1.0));
}

void glide64gSPPointLightVertex(void *data, float * vpos)
{
   uint32_t l;
   float color[3];
   VERTEX *v      = (VERTEX*)data;

   color[0] = rdp.light[gSP.numLights].col[0];
   color[1] = rdp.light[gSP.numLights].col[1];
   color[2] = rdp.light[gSP.numLights].col[2];

   for (l = 0; l < gSP.numLights; l++)
   {
      float light_intensity = 0.0f;

      if (rdp.light[l].nonblack)
      {
         float light_len2, light_len, at;
         float lvec[3] = {rdp.light[l].x, rdp.light[l].y, rdp.light[l].z};
         lvec[0] -= vpos[0];
         lvec[1] -= vpos[1];
         lvec[2] -= vpos[2];

         light_len2 = lvec[0] * lvec[0] + lvec[1] * lvec[1] + lvec[2] * lvec[2];
         light_len = sqrtf(light_len2);
         at = rdp.light[l].ca + light_len/65535.0f*rdp.light[l].la + light_len2/65535.0f*rdp.light[l].qa;

         if (at > 0.0f)
            light_intensity = 1/at;//DotProduct (lvec, nvec) / (light_len * normal_len * at);
      }

      if (light_intensity > 0.0f)
      {
         color[0] += rdp.light[l].col[0] * light_intensity;
         color[1] += rdp.light[l].col[1] * light_intensity;
         color[2] += rdp.light[l].col[2] * light_intensity;
      }
   }

   if (color[0] > 1.0f) color[0] = 1.0f;
   if (color[1] > 1.0f) color[1] = 1.0f;
   if (color[2] > 1.0f) color[2] = 1.0f;

   v->r = (uint8_t)(color[0]*255.0f);
   v->g = (uint8_t)(color[1]*255.0f);
   v->b = (uint8_t)(color[2]*255.0f);
}

void load_matrix (float m[4][4], uint32_t addr)
{
   int x,y;  // matrix index
   uint16_t *src = (uint16_t*)gfx_info.RDRAM;
   addr >>= 1;

   // Adding 4 instead of one, just to remove mult. later
   for (x = 0; x < 16; x += 4)
   {
      for (y=0; y<4; y++)
         m[x>>2][y] = (float)((((int32_t)src[(addr+x+y)^1]) << 16) | src[(addr+x+y+16)^1]) / 65536.0f;
   }
}

void glide64gSPCombineMatrices(void)
{
   MulMatrices(rdp.model, rdp.proj, rdp.combined);
   g_gdp.flags ^= UPDATE_MULT_MAT;
}

void glide64gSPSegment(int32_t seg, int32_t base)
{
   gSP.segment[seg] = base;
}

void glide64gSPClipRatio(uint32_t r)
{
   rdp.clip_ratio = (float)vi_integer_sqrt(r);
   g_gdp.flags |= UPDATE_VIEWPORT;
}

void glide64gSPClipVertex(uint32_t v)
{
   VERTEX *vtx = (VERTEX*)&rdp.vtx[v];

   vtx->scr_off = 0;
   if (vtx->x > +vtx->w)   vtx->scr_off |= 2;
   if (vtx->x < -vtx->w)   vtx->scr_off |= 1;
   if (vtx->y > +vtx->w)   vtx->scr_off |= 8;
   if (vtx->y < -vtx->w)   vtx->scr_off |= 4;
   if (vtx->w < 0.1f)      vtx->scr_off |= 16;
}

void glide64gSPLookAt(uint32_t l, uint32_t n)
{
   int8_t  *rdram_s8  = (int8_t*) (gfx_info.RDRAM  + RSP_SegmentToPhysical(l));
   int8_t dir_x = rdram_s8[11];
   int8_t dir_y = rdram_s8[10];
   int8_t dir_z = rdram_s8[9];
   rdp.lookat[n][0] = (float)(dir_x) / 127.0f;
   rdp.lookat[n][1] = (float)(dir_y) / 127.0f;
   rdp.lookat[n][2] = (float)(dir_z) / 127.0f;
   gSP.lookatEnable = (n == 0) || (n == 1 && (dir_x || dir_y));
}

void glide64gSPLight(uint32_t l, int32_t n)
{
   int16_t *rdram     = (int16_t*)(gfx_info.RDRAM  + RSP_SegmentToPhysical(l));
   uint8_t *rdram_u8  = (uint8_t*)(gfx_info.RDRAM  + RSP_SegmentToPhysical(l));
   int8_t  *rdram_s8  = (int8_t*) (gfx_info.RDRAM  + RSP_SegmentToPhysical(l));

   --n;

   if (n < 8)
   {
      /* Get the data */
      rdp.light[n].nonblack  = rdram_u8[3];
      rdp.light[n].nonblack += rdram_u8[2];
      rdp.light[n].nonblack += rdram_u8[1];

      rdp.light[n].col[0]    = rdram_u8[3] / 255.0f;
      rdp.light[n].col[1]    = rdram_u8[2] / 255.0f;
      rdp.light[n].col[2]    = rdram_u8[1] / 255.0f;
      rdp.light[n].col[3]    = 1.0f;

      rdp.light[n].dir[0]    = (float)rdram_s8[11] / 127.0f;
      rdp.light[n].dir[1]    = (float)rdram_s8[10] / 127.0f;
      rdp.light[n].dir[2]    = (float)rdram_s8[9] / 127.0f;

      rdp.light[n].x         = (float)rdram[5];
      rdp.light[n].y         = (float)rdram[4];
      rdp.light[n].z         = (float)rdram[7];
      rdp.light[n].ca        = (float)rdram[0] / 16.0f;
      rdp.light[n].la        = (float)rdram[4];
      rdp.light[n].qa        = (float)rdram[13] / 8.0f;

      //g_gdp.flags |= UPDATE_LIGHTS;
   }
}

void glide64gSPLightColor( uint32_t lightNum, uint32_t packedColor )
{
   lightNum--;

   if (lightNum < 8)
   {
      rdp.light[lightNum].col[0] = _SHIFTR( packedColor, 24, 8 ) * 0.0039215689f;
      rdp.light[lightNum].col[1] = _SHIFTR( packedColor, 16, 8 ) * 0.0039215689f;
      rdp.light[lightNum].col[2] = _SHIFTR( packedColor, 8, 8 )  * 0.0039215689f;
      rdp.light[lightNum].col[3] = 255;
   }
}

void glide64gSP1Triangle( int32_t v0, int32_t v1, int32_t v2, int32_t flag )
{
   VERTEX *v[3];

   v[0] = &rdp.vtx[v0];
   v[1] = &rdp.vtx[v1];
   v[2] = &rdp.vtx[v2];

   cull_trianglefaces(v, 1, true, true, flag);
}

void glide64gSP1Quadrangle(int32_t v0, int32_t v1, int32_t v2, int32_t v3)
{
   VERTEX *v[6];

   if (rdp.skip_drawing)
      return;

   v[0] = &rdp.vtx[v0];
   v[1] = &rdp.vtx[v1];
   v[2] = &rdp.vtx[v2];
   v[3] = &rdp.vtx[v3];
   v[4] = &rdp.vtx[v0];
   v[5] = &rdp.vtx[v2];

   cull_trianglefaces(v, 2, true, true, 0);
}

void glide64gSP2Triangles(const int32_t v00, const int32_t v01, const int32_t v02, const int32_t flag0,
                    const int32_t v10, const int32_t v11, const int32_t v12, const int32_t flag1 )
{
   VERTEX *v[6];

   if (rdp.skip_drawing)
      return;

   v[0] = &rdp.vtx[v00];
   v[1] = &rdp.vtx[v01];
   v[2] = &rdp.vtx[v02];
   v[3] = &rdp.vtx[v10];
   v[4] = &rdp.vtx[v11];
   v[5] = &rdp.vtx[v12];

   cull_trianglefaces(v, 2, true, true, 0);
}

void glide64gSP4Triangles( int32_t v00, int32_t v01, int32_t v02,
                    int32_t v10, int32_t v11, int32_t v12,
                    int32_t v20, int32_t v21, int32_t v22,
                    int32_t v30, int32_t v31, int32_t v32 )
{
   VERTEX *v[12];

   if (rdp.skip_drawing)
      return;

   v[0]  = &rdp.vtx[v00];
   v[1]  = &rdp.vtx[v01];
   v[2]  = &rdp.vtx[v02];
   v[3]  = &rdp.vtx[v10];
   v[4]  = &rdp.vtx[v11];
   v[5]  = &rdp.vtx[v12];
   v[6]  = &rdp.vtx[v20];
   v[7]  = &rdp.vtx[v21];
   v[8]  = &rdp.vtx[v22];
   v[9]  = &rdp.vtx[v30];
   v[10] = &rdp.vtx[v31];
   v[11] = &rdp.vtx[v32];

   cull_trianglefaces(v, 4, true, true, 0);
}

void glide64gSPViewport(uint32_t v)
{
   int16_t *rdram     = (int16_t*)(gfx_info.RDRAM  + RSP_SegmentToPhysical( v ));

   int16_t scale_y = rdram[0] >> 2;
   int16_t scale_x = rdram[1] >> 2;
   int16_t scale_z = rdram[3];
   int16_t trans_x = rdram[5] >> 2;
   int16_t trans_y = rdram[4] >> 2;
   int16_t trans_z = rdram[7];
   if (settings.correct_viewport)
   {
      scale_x = abs(scale_x);
      scale_y = abs(scale_y);
   }
   gSP.viewport.vscale[0] = scale_x * rdp.scale_x;
   gSP.viewport.vscale[1] = -scale_y * rdp.scale_y;
   gSP.viewport.vscale[2] = 32.0f * scale_z;
   gSP.viewport.vtrans[0] = trans_x * rdp.scale_x;
   gSP.viewport.vtrans[1] = trans_y * rdp.scale_y;
   gSP.viewport.vtrans[2] = 32.0f * trans_z;

   g_gdp.flags |= UPDATE_VIEWPORT;
}

void glide64gSPForceMatrix( uint32_t mptr )
{
   uint32_t address = RSP_SegmentToPhysical( mptr );

   load_matrix(rdp.combined, address);

   g_gdp.flags &= ~UPDATE_MULT_MAT;
}

void glide64gSPObjMatrix( uint32_t mtx )
{
   uint32_t addr         = RSP_SegmentToPhysical(mtx) >> 1;

   gSP.objMatrix.A          = ((int*)gfx_info.RDRAM)[(addr+0)>>1] / 65536.0f;
   gSP.objMatrix.B          = ((int*)gfx_info.RDRAM)[(addr+2)>>1] / 65536.0f;
   gSP.objMatrix.C          = ((int*)gfx_info.RDRAM)[(addr+4)>>1] / 65536.0f;
   gSP.objMatrix.D          = ((int*)gfx_info.RDRAM)[(addr+6)>>1] / 65536.0f;
   gSP.objMatrix.X          = ((int16_t*)gfx_info.RDRAM)[(addr+8)^1] / 4.0f;
   gSP.objMatrix.Y          = ((int16_t*)gfx_info.RDRAM)[(addr+9)^1] / 4.0f;
   gSP.objMatrix.baseScaleX = ((uint16_t*)gfx_info.RDRAM)[(addr+10)^1] / 1024.0f;
   gSP.objMatrix.baseScaleY = ((uint16_t*)gfx_info.RDRAM)[(addr+11)^1] / 1024.0f;
}

void glide64gSPObjSubMatrix( uint32_t mtx )
{
   uint32_t addr         = RSP_SegmentToPhysical(mtx) >> 1;

   gSP.objMatrix.X          = ((int16_t*)gfx_info.RDRAM)[(addr+0)^1] / 4.0f;
   gSP.objMatrix.Y          = ((int16_t*)gfx_info.RDRAM)[(addr+1)^1] / 4.0f;
   gSP.objMatrix.baseScaleX = ((uint16_t*)gfx_info.RDRAM)[(addr+2)^1] / 1024.0f;
   gSP.objMatrix.baseScaleY = ((uint16_t*)gfx_info.RDRAM)[(addr+3)^1] / 1024.0f;
}

void glide64gSPObjLoadTxtr(uint32_t tx)
{
   uint32_t addr = RSP_SegmentToPhysical(tx) >> 1;
   uint32_t type = ((uint32_t*)gfx_info.RDRAM)[(addr + 0) >> 1]; // 0, 1

   switch (type)
   {
      case G_OBJLT_TLUT:
         {
            uint32_t image = RSP_SegmentToPhysical(((uint32_t*)gfx_info.RDRAM)[(addr + 2) >> 1]); // 2, 3
            uint16_t phead = ((uint16_t *)gfx_info.RDRAM)[(addr + 4) ^ 1] - 256; // 4
            uint16_t pnum  = ((uint16_t *)gfx_info.RDRAM)[(addr + 5) ^ 1] + 1; // 5

            //FRDP ("palette addr: %08lx, start: %d, num: %d\n", image, phead, pnum);
            load_palette (image, phead, pnum);
         }
         break;
      case G_OBJLT_TXTRBLOCK:
         {
            uint32_t w0, w1;
            uint32_t image = RSP_SegmentToPhysical(((uint32_t*)gfx_info.RDRAM)[(addr + 2) >> 1]); // 2, 3
            uint16_t tmem  = ((uint16_t *)gfx_info.RDRAM)[(addr + 4) ^ 1]; // 4
            uint16_t tsize = ((uint16_t *)gfx_info.RDRAM)[(addr + 5) ^ 1]; // 5
            uint16_t tline = ((uint16_t *)gfx_info.RDRAM)[(addr + 6) ^ 1]; // 6

            glide64gDPSetTextureImage(
                  g_gdp.ti_format,        /* format */
                  G_IM_SIZ_8b,            /* siz */
                  1,                      /* width */
                  image                   /* address */
                  );

            g_gdp.tile[7].tmem = tmem;
            g_gdp.tile[7].size = 1;
            w0 = __RSP.w0           = 0;
            w1 = __RSP.w1           = 0x07000000 | (tsize << 14) | tline;

            glide64gDPLoadBlock(
                  ((w1 >> 24) & 0x07), 
                  (w0 >> 14) & 0x3FF, /* ul_s */
                  (w0 >>  2) & 0x3FF, /* ul_t */
                  (w1 >> 14) & 0x3FF, /* lr_s */
                  (w1 & 0x0FFF) /* dxt */
                  );
         }
         break;
      case G_OBJLT_TXTRTILE:
         {
            int line;
            uint32_t w0, w1;
            uint32_t image   = RSP_SegmentToPhysical(((uint32_t*)gfx_info.RDRAM)[(addr + 2) >> 1]); // 2, 3
            uint16_t tmem    = ((uint16_t *)gfx_info.RDRAM)[(addr + 4) ^ 1]; // 4
            uint16_t twidth  = ((uint16_t *)gfx_info.RDRAM)[(addr + 5) ^ 1]; // 5
            uint16_t theight = ((uint16_t *)gfx_info.RDRAM)[(addr + 6) ^ 1]; // 6
#if 0
            FRDP ("tile addr: %08lx, tmem: %08lx, twidth: %d, theight: %d\n", image, tmem, twidth, theight);
#endif
            line             = (twidth + 1) >> 2;

            glide64gDPSetTextureImage(
                  g_gdp.ti_format,        /* format */
                  G_IM_SIZ_8b,            /* siz */
                  line << 3,              /* width */
                  image                   /* address */
                  );

            g_gdp.tile[7].tmem = tmem;
            g_gdp.tile[7].line = line;
            g_gdp.tile[7].size = 1;

            w0 = __RSP.w0 = 0;
            w1 = __RSP.w1 = 0x07000000 | (twidth << 14) | (theight << 2);

            glide64gDPLoadTile(
                  (uint32_t)((w1 >> 24) & 0x07),      /* tile */
                  (uint32_t)((w0 >> 14) & 0x03FF),    /* ul_s */
                  (uint32_t)((w0 >> 2 ) & 0x03FF),    /* ul_t */
                  (uint32_t)((w1 >> 14) & 0x03FF),    /* lr_s */
                  (uint32_t)((w1 >> 2 ) & 0x03FF)     /* lr_t */
                  );
         }
         break;
   }
}

static INLINE void pre_update(void)
{
   /* This is special, not handled in update(), but here
    * Matrix Pre-multiplication idea */
   if (g_gdp.flags & UPDATE_MULT_MAT)
      gSPCombineMatrices();

   if (g_gdp.flags & UPDATE_LIGHTS)
   {
      uint32_t l;
      g_gdp.flags ^= UPDATE_LIGHTS;

      /* Calculate light vectors */
      for (l = 0; l < gSP.numLights; l++)
      {
         InverseTransformVector(&rdp.light[l].dir[0], rdp.light_vector[l], rdp.model);
         NormalizeVector (rdp.light_vector[l]);
      }
   }
}

/*
 * Loads into the RSP vertex buffer the vertices that will be used by the 
 * gSP1Triangle commands to generate polygons.
 *
 * v  - Segment address of the vertex list  pointer to a list of vertices.
 * n  - Number of vertices (1 - 32).
 * v0 - Starting index in vertex buffer where vertices are to be loaded into.
 */
void glide64gSPVertex(uint32_t v, uint32_t n, uint32_t v0)
{
   unsigned int i;
   float x, y, z;
   uint32_t iter = 16;
   void   *vertex  = (void*)(gfx_info.RDRAM + v);

   pre_update();

   for (i=0; i < (n * iter); i+= iter)
   {
      VERTEX *vtx = (VERTEX*)&rdp.vtx[v0 + (i / iter)];
      int16_t *rdram    = (int16_t*)vertex;
      uint8_t *rdram_u8 = (uint8_t*)vertex;
      uint8_t *color = (uint8_t*)(rdram_u8 + 12);
      y                 = (float)rdram[0];
      x                 = (float)rdram[1];
      vtx->flags        = (uint16_t)rdram[2];
      z                 = (float)rdram[3];
      vtx->ov           = (float)rdram[4];
      vtx->ou           = (float)rdram[5];
      vtx->uv_scaled    = 0;
      vtx->a            = color[0];

      vtx->x = x*rdp.combined[0][0] + y*rdp.combined[1][0] + z*rdp.combined[2][0] + rdp.combined[3][0];
      vtx->y = x*rdp.combined[0][1] + y*rdp.combined[1][1] + z*rdp.combined[2][1] + rdp.combined[3][1];
      vtx->z = x*rdp.combined[0][2] + y*rdp.combined[1][2] + z*rdp.combined[2][2] + rdp.combined[3][2];
      vtx->w = x*rdp.combined[0][3] + y*rdp.combined[1][3] + z*rdp.combined[2][3] + rdp.combined[3][3];

      vtx->uv_calculated = 0xFFFFFFFF;
      vtx->screen_translated = 0;
      vtx->shade_mod = 0;

      if (fabs(vtx->w) < 0.001)
         vtx->w = 0.001f;
      vtx->oow = 1.0f / vtx->w;
      vtx->x_w = vtx->x * vtx->oow;
      vtx->y_w = vtx->y * vtx->oow;
      vtx->z_w = vtx->z * vtx->oow;
      calculateVertexFog (vtx);

      gSPClipVertex(v0 + (i / iter));

      if (gSP.geometryMode & G_LIGHTING)
      {
         vtx->vec[0] = (int8_t)color[3];
         vtx->vec[1] = (int8_t)color[2];
         vtx->vec[2] = (int8_t)color[1];

         if (settings.ucode == 2 && gSP.geometryMode & G_POINT_LIGHTING)
         {
            float tmpvec[3] = {x, y, z};
            glide64gSPPointLightVertex(vtx, tmpvec);
         }
         else
         {
            NormalizeVector (vtx->vec);
            glide64gSPLightVertex(vtx);
         }

         if (gSP.geometryMode & G_TEXTURE_GEN)
         {
            if (gSP.geometryMode & G_TEXTURE_GEN_LINEAR)
               calc_linear (vtx);
            else
               calc_sphere (vtx);
         }

      }
#if 0
      else if (gSP.geometryMode & G_ACCLAIM_LIGHTING)
      {
         glide64gSPPointLightVertex_Acclaim(vtx);
      }
#endif
      else
      {
         vtx->r = color[3];
         vtx->g = color[2];
         vtx->b = color[1];
      }
      vertex = (char*)vertex + iter;
   }
}

void glide64gSPFogFactor(int16_t fm, int16_t fo )
{
   gSP.fog.multiplier = fm;
   gSP.fog.offset     = fo;
}

void glide64gSPSetGeometryMode (uint32_t mode)
{
   gSP.geometryMode |= mode;
}

void glide64gSPClearGeometryMode (uint32_t mode)
{
    gSP.geometryMode &= ~mode;
}

void glide64gSPNumLights(int32_t n)
{
   if (n > 12)
      return;

   gSP.numLights = n;
   g_gdp.flags |= UPDATE_LIGHTS;
}

void glide64gSPPopMatrixN(uint32_t param, uint32_t num )

{
   if (gSP.matrix.modelViewi > num - 1)
      gSP.matrix.modelViewi -= num;
   CopyMatrix(rdp.model, gSP.matrix.modelView[gSP.matrix.modelViewi]);
   g_gdp.flags |= UPDATE_MULT_MAT;
}

void glide64gSPPopMatrix(uint32_t param)
{
   switch (param)
   {
      case 0: // modelview
         if (gSP.matrix.modelViewi > 0)
         {
            gSP.matrix.modelViewi--;
            CopyMatrix(rdp.model, gSP.matrix.modelView[gSP.matrix.modelViewi]);
            g_gdp.flags |= UPDATE_MULT_MAT;
         }
         break;
      case 1: // projection, can't
         break;
      default:
#ifdef DEBUG
         DebugMsg( DEBUG_HIGH | DEBUG_ERROR | DEBUG_MATRIX, "// Attempting to pop matrix stack below 0\n" );
         DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_MATRIX, "gSPPopMatrix( %s );\n",
               (param == G_MTX_MODELVIEW) ? "G_MTX_MODELVIEW" :
               (param == G_MTX_PROJECTION) ? "G_MTX_PROJECTION" : "G_MTX_INVALID" );
#endif
         break;
   }
}

void glide64gSPDlistCount(uint32_t count, uint32_t v)
{
   uint32_t address = RSP_SegmentToPhysical(v);

   if (__RSP.PCi >= 9 || address == 0)
      return;

   __RSP.PCi ++;  // go to the next PC in the stack
   __RSP.PC[__RSP.PCi] = address;  // jump to the address
   __RSP.count = count + 1;
}

void glide64gSPCullDisplayList( uint32_t v0, uint32_t vn )
{
	if (glide64gSPCullVertices( v0, vn ))
      gSPEndDisplayList();
}

void glide64gSPModifyVertex( uint32_t vtx, uint32_t where, uint32_t val )
{
   VERTEX *v = (VERTEX*)&rdp.vtx[vtx];

   switch (where)
   {
      case 0:
         uc6_obj_sprite(__RSP.w0, __RSP.w1);
         break;

      case G_MWO_POINT_RGBA:
         v->r = (uint8_t)(val >> 24);
         v->g = (uint8_t)((val >> 16) & 0xFF);
         v->b = (uint8_t)((val >> 8) & 0xFF);
         v->a = (uint8_t)(val & 0xFF);
         v->shade_mod = 0;
         break;

      case G_MWO_POINT_ST:
         {
            float scale = gDP.otherMode.texturePersp ? 0.03125f : 0.015625f;
            v->ou = (float)((int16_t)(val>>16)) * scale;
            v->ov = (float)((int16_t)(val&0xFFFF)) * scale;
            v->uv_calculated = 0xFFFFFFFF;
            v->uv_scaled = 1;
         }
         break;

      case G_MWO_POINT_XYSCREEN:
         {
            float scr_x = (float)((int16_t)(val>>16)) / 4.0f;
            float scr_y = (float)((int16_t)(val&0xFFFF)) / 4.0f;
            v->screen_translated = 2;
            v->sx = scr_x * rdp.scale_x + rdp.offset_x;
            v->sy = scr_y * rdp.scale_y + rdp.offset_y;
            if (v->w < 0.01f)
            {
               v->w   = 1.0f;
               v->oow = 1.0f;
               v->z_w = 1.0f;
            }
            v->sz = gSP.viewport.vtrans[2] + v->z_w * gSP.viewport.vscale[2];

            v->scr_off = 0;
            if (scr_x < 0)
               v->scr_off |= X_CLIP_MAX;
            if (scr_x > rdp.vi_width)
               v->scr_off |= X_CLIP_MIN;
            if (scr_y < 0)
               v->scr_off |= Y_CLIP_MAX;
            if (scr_y > rdp.vi_height)
               v->scr_off |= Y_CLIP_MIN;
            if (v->w < 0.1f)
               v->scr_off |= Z_CLIP_MAX;
         }
         break;
      case G_MWO_POINT_ZSCREEN:
         {
            float scr_z = _FIXED2FLOAT((int16_t)_SHIFTR(val, 16, 16), 15);
            v->z_w      = (scr_z - gSP.viewport.vtrans[2]) / gSP.viewport.vscale[2];
            v->z        = v->z_w * v->w;
         }
         break;
   }
}

bool glide64gSPCullVertices( uint32_t v0, uint32_t vn )
{
   unsigned i;
   uint32_t clip = 0;

	if (vn < v0)
   {
      // Aidyn Chronicles - The First Mage seems to pass parameters in reverse order.
      const uint32_t v = v0;
      v0 = vn;
      vn = v;
   }

   /* Wipeout 64 passes vn = 512, increasing MAX_VTX to 512+ doesn't fix. */
   if (vn > MAX_VTX)
      return false;

   for (i = v0; i <= vn; i++)
   {
      VERTEX *v = (VERTEX*)&rdp.vtx[i];

      /* Check if completely off the screen 
       * (quick frustrum clipping for 90 FOV) */
      if (v->x >= -v->w)
         clip |= X_CLIP_MAX;
      if (v->x <= v->w)
         clip |= X_CLIP_MIN;
      if (v->y >= -v->w)
         clip |= Y_CLIP_MAX;
      if (v->y <= v->w)
         clip |= Y_CLIP_MIN;
      if (v->w >= 0.1f)
         clip |= Z_CLIP_MAX;
      if (clip == 0x1F)
         return false;
   }
   return true;
}

void glide64gSPSetVertexColorBase(uint32_t base)
{
   gSP.vertexColorBase = RSP_SegmentToPhysical(base);
}

void glide64gSPCIVertex(uint32_t v, uint32_t n, uint32_t v0)
{
   unsigned int i;
   uint32_t addr   = RSP_SegmentToPhysical(v);
   vtx_uc7 *vertex = (vtx_uc7*)&gfx_info.RDRAM[addr];
   uint32_t iter   = 1;

   pre_update();

   for (i = 0; i < (n * iter); i += iter)
   {
      VERTEX *vert    = (VERTEX*)&rdp.vtx[v0 + (i / iter)];
      uint8_t *color  = (uint8_t*)&gfx_info.RDRAM[gSP.vertexColorBase + (vertex->idx & 0xff)];
      float x         = (float)vertex->x;
      float y         = (float)vertex->y;
      float z         = (float)vertex->z;

      vert->flags     = 0;
      vert->ou        = (float)vertex->s;
      vert->ov        = (float)vertex->t;
      vert->uv_scaled = 0;
      vert->a         = color[0];

      vert->x         = x*rdp.combined[0][0] + y*rdp.combined[1][0] + z*rdp.combined[2][0] + rdp.combined[3][0];
      vert->y         = x*rdp.combined[0][1] + y*rdp.combined[1][1] + z*rdp.combined[2][1] + rdp.combined[3][1];
      vert->z         = x*rdp.combined[0][2] + y*rdp.combined[1][2] + z*rdp.combined[2][2] + rdp.combined[3][2];
      vert->w         = x*rdp.combined[0][3] + y*rdp.combined[1][3] + z*rdp.combined[2][3] + rdp.combined[3][3];

      vert->uv_calculated     = 0xFFFFFFFF;
      vert->screen_translated = 0;

      if (fabs(vert->w) < 0.001)
         vert->w = 0.001f;
      vert->oow  = 1.0f / vert->w;
      vert->x_w  = vert->x * vert->oow;
      vert->y_w  = vert->y * vert->oow;
      vert->z_w  = vert->z * vert->oow;
      calculateVertexFog (vert);

      vert->scr_off = 0;
      if (vert->x < -vert->w)
         vert->scr_off |= X_CLIP_MAX;
      if (vert->x > vert->w)
         vert->scr_off |= X_CLIP_MIN;
      if (vert->y < -vert->w)
         vert->scr_off |= Y_CLIP_MAX;
      if (vert->y > vert->w)
         vert->scr_off |= Y_CLIP_MIN;
      if (vert->w < 0.1f)
         vert->scr_off |= Z_CLIP_MAX;
#if 0
      if (vert->z_w > 1.0f)
         vert->scr_off |= Z_CLIP_MIN; 
#endif

      if (gSP.geometryMode & G_LIGHTING)
      {
         vert->vec[0] = (int8_t)color[3];
         vert->vec[1] = (int8_t)color[2];
         vert->vec[2] = (int8_t)color[1];

         if (gSP.geometryMode & G_TEXTURE_GEN_LINEAR) 
            calc_linear(vert);
         else if (gSP.geometryMode & G_TEXTURE_GEN) 
            calc_sphere(vert);

         NormalizeVector (vert->vec);

         glide64gSPLightVertex(vert);
      }
      else
      {
         vert->r = color[3];
         vert->g = color[2];
         vert->b = color[1];
      }
      vertex++;
   }
}

void glide64gSPSetVertexNormaleBase(uint32_t base)
{
   gSP.vertexNormalBase = RSP_SegmentToPhysical(base);
}

void glide64gSPCBFDVertex(uint32_t a, uint32_t n, uint32_t v0)
{
   uint32_t i;
   uint32_t addr        = RSP_SegmentToPhysical(a);
   void   *membase_ptr  = (void*)(gfx_info.RDRAM + addr);
   uint32_t iter        = 16;

   if (v0 < 0)
      return;

   pre_update();

   for (i=0; i < (n * iter); i+= iter)
   {
      VERTEX *vert = (VERTEX*)&rdp.vtx[v0 + (i / iter)];
      int16_t *rdram    = (int16_t*)membase_ptr;
      uint8_t *rdram_u8 = (uint8_t*)membase_ptr;
      uint8_t *color = (uint8_t*)(rdram_u8 + 12);

      float x                 = (float)rdram[1];
      float y                 = (float)rdram[0];
      float z                 = (float)rdram[3];

      vert->flags       = (uint16_t)rdram[2];
      vert->ov          = (float)rdram[4];
      vert->ou          = (float)rdram[5];
      vert->uv_scaled   = 0;
      vert->a           = color[0];

      vert->x = x*rdp.combined[0][0] + y*rdp.combined[1][0] + z*rdp.combined[2][0] + rdp.combined[3][0];
      vert->y = x*rdp.combined[0][1] + y*rdp.combined[1][1] + z*rdp.combined[2][1] + rdp.combined[3][1];
      vert->z = x*rdp.combined[0][2] + y*rdp.combined[1][2] + z*rdp.combined[2][2] + rdp.combined[3][2];
      vert->w = x*rdp.combined[0][3] + y*rdp.combined[1][3] + z*rdp.combined[2][3] + rdp.combined[3][3];

      vert->uv_calculated     = 0xFFFFFFFF;
      vert->screen_translated = 0;
      vert->shade_mod         = 0;

      if (fabs(vert->w) < 0.001)
         vert->w = 0.001f;
      vert->oow = 1.0f / vert->w;
      vert->x_w = vert->x * vert->oow;
      vert->y_w = vert->y * vert->oow;
      vert->z_w = vert->z * vert->oow;

      vert->scr_off = 0;
      if (vert->x < -vert->w)
         vert->scr_off |= X_CLIP_MAX;
      if (vert->x > vert->w)
         vert->scr_off |= X_CLIP_MIN;
      if (vert->y < -vert->w)
         vert->scr_off |= Y_CLIP_MAX;
      if (vert->y > vert->w)
         vert->scr_off |= Y_CLIP_MIN;
      if (vert->w < 0.1f)
         vert->scr_off |= Z_CLIP_MAX;

      vert->r = color[3];
      vert->g = color[2];
      vert->b = color[1];

      if ((gSP.geometryMode & G_LIGHTING))
      {
         uint32_t l;
         float light_intensity, color[3];
         uint32_t shift = v0 << 1;

         vert->vec[0]   = ((int8_t*)gfx_info.RDRAM)[(gSP.vertexNormalBase + (i>>3) + shift + 0)^3];
         vert->vec[1]   = ((int8_t*)gfx_info.RDRAM)[(gSP.vertexNormalBase + (i>>3) + shift + 1)^3];
         vert->vec[2]   = (int8_t)(vert->flags & 0xff);

         if (gSP.geometryMode & G_TEXTURE_GEN_LINEAR)
            calc_linear (vert);
         else if (gSP.geometryMode & G_TEXTURE_GEN)
            calc_sphere (vert);

         color[0] = rdp.light[gSP.numLights].col[0];
         color[1] = rdp.light[gSP.numLights].col[1];
         color[2] = rdp.light[gSP.numLights].col[2];

         light_intensity = 0.0f;

         if (gSP.geometryMode & G_POINT_LIGHTING)
         {
            NormalizeVector (vert->vec);
            for (l = 0; l < gSP.numLights-1; l++)
            {
               if (!rdp.light[l].nonblack)
                  continue;
               light_intensity = DotProduct (rdp.light_vector[l], vert->vec);
               FRDP("light %d, intensity : %f\n", l, light_intensity);
               if (light_intensity < 0.0f)
                  continue;

               if (rdp.light[l].ca > 0.0f)
               {
                  float vx = (vert->x + gSP.vertexCoordMod[8])*gSP.vertexCoordMod[12] - rdp.light[l].x;
                  float vy = (vert->y + gSP.vertexCoordMod[9])*gSP.vertexCoordMod[13] - rdp.light[l].y;
                  float vz = (vert->z + gSP.vertexCoordMod[10])*gSP.vertexCoordMod[14] - rdp.light[l].z;
                  float vw = (vert->w + gSP.vertexCoordMod[11])*gSP.vertexCoordMod[15] - rdp.light[l].w;
                  float len = (vx*vx+vy*vy+vz*vz+vw*vw)/65536.0f;
                  float p_i = rdp.light[l].ca / len;
                  if (p_i > 1.0f)
                     p_i = 1.0f;
                  light_intensity *= p_i;
               }

               color[0] += rdp.light[l].col[0] * light_intensity;
               color[1] += rdp.light[l].col[1] * light_intensity;
               color[2] += rdp.light[l].col[2] * light_intensity;
            }
            light_intensity = DotProduct (rdp.light_vector[l], vert->vec);
            if (light_intensity > 0.0f)
            {
               color[0] += rdp.light[l].col[0] * light_intensity;
               color[1] += rdp.light[l].col[1] * light_intensity;
               color[2] += rdp.light[l].col[2] * light_intensity;
            }
         }
         else
         {
            for (l = 0; l < gSP.numLights; l++)
            {
               if (rdp.light[l].nonblack && rdp.light[l].nonzero)
               {
                  float vx = (vert->x + gSP.vertexCoordMod[8])*gSP.vertexCoordMod[12] - rdp.light[l].x;
                  float vy = (vert->y + gSP.vertexCoordMod[9])*gSP.vertexCoordMod[13] - rdp.light[l].y;
                  float vz = (vert->z + gSP.vertexCoordMod[10])*gSP.vertexCoordMod[14] - rdp.light[l].z;
                  float vw = (vert->w + gSP.vertexCoordMod[11])*gSP.vertexCoordMod[15] - rdp.light[l].w;
                  float len = (vx*vx+vy*vy+vz*vz+vw*vw)/65536.0f;
                  light_intensity = rdp.light[l].ca / len;
                  if (light_intensity > 1.0f)
                     light_intensity = 1.0f;
                  color[0] += rdp.light[l].col[0] * light_intensity;
                  color[1] += rdp.light[l].col[1] * light_intensity;
                  color[2] += rdp.light[l].col[2] * light_intensity;
               }
            }
         }
         if (color[0] > 1.0f) color[0] = 1.0f;
         if (color[1] > 1.0f) color[1] = 1.0f;
         if (color[2] > 1.0f) color[2] = 1.0f;
         vert->r = (uint8_t)(((float)vert->r)*color[0]);
         vert->g = (uint8_t)(((float)vert->g)*color[1]);
         vert->b = (uint8_t)(((float)vert->b)*color[2]);
      }
      membase_ptr = (char*)membase_ptr + iter;
   }
}

void glide64gSPCoordMod(uint32_t w0, uint32_t w1)
{
   uint32_t idx, pos;
   uint16_t offset = (uint16_t)(w0 & 0xFFFF);
   uint8_t n       = offset >> 2;

   if (w0 & 8)
      return;
   idx = (w0 >> 1)&3;
   pos = w0 & 0x30;
   if (pos == 0)
   {
      gSP.vertexCoordMod[0+idx] = (int16_t)(w1 >> 16);
      gSP.vertexCoordMod[1+idx] = (int16_t)(w1 & 0xffff);
   }
   else if (pos == 0x10)
   {
      gSP.vertexCoordMod[4+idx]  = (w1 >> 16) / 65536.0f;
      gSP.vertexCoordMod[5+idx]  = (w1 & 0xffff) / 65536.0f;
      gSP.vertexCoordMod[12+idx] = gSP.vertexCoordMod[0+idx] + gSP.vertexCoordMod[4+idx];
      gSP.vertexCoordMod[13+idx] = gSP.vertexCoordMod[1+idx] + gSP.vertexCoordMod[5+idx];

   }
   else if (pos == 0x20)
   {
      gSP.vertexCoordMod[8+idx] = (int16_t)(w1 >> 16);
      gSP.vertexCoordMod[9+idx] = (int16_t)(w1 & 0xffff);
   }
}

void glide64gSPLightCBFD(uint32_t l, int32_t n)
{
   uint32_t a;
   uint32_t addr = RSP_SegmentToPhysical(l);

   rdp.light[n].nonblack  = gfx_info.RDRAM[(addr+0)^3];
   rdp.light[n].nonblack += gfx_info.RDRAM[(addr+1)^3];
   rdp.light[n].nonblack += gfx_info.RDRAM[(addr+2)^3];

   rdp.light[n].col[0]    = (((uint8_t*)gfx_info.RDRAM)[(addr+0)^3]) / 255.0f;
   rdp.light[n].col[1]    = (((uint8_t*)gfx_info.RDRAM)[(addr+1)^3]) / 255.0f;
   rdp.light[n].col[2]    = (((uint8_t*)gfx_info.RDRAM)[(addr+2)^3]) / 255.0f;
   rdp.light[n].col[3]    = 1.0f;

   rdp.light[n].dir[0]    = (float)(((int8_t*)gfx_info.RDRAM)[(addr+8)^3]) / 127.0f;
   rdp.light[n].dir[1]    = (float)(((int8_t*)gfx_info.RDRAM)[(addr+9)^3]) / 127.0f;
   rdp.light[n].dir[2]    = (float)(((int8_t*)gfx_info.RDRAM)[(addr+10)^3]) / 127.0f;

   a = addr >> 1;

   rdp.light[n].x         = (float)(((int16_t*)gfx_info.RDRAM)[(a+16)^1]);
   rdp.light[n].y         = (float)(((int16_t*)gfx_info.RDRAM)[(a+17)^1]);
   rdp.light[n].z         = (float)(((int16_t*)gfx_info.RDRAM)[(a+18)^1]);
   rdp.light[n].w         = (float)(((int16_t*)gfx_info.RDRAM)[(a+19)^1]);
   rdp.light[n].nonzero   = gfx_info.RDRAM[(addr+12)^3];
   rdp.light[n].ca        = (float)rdp.light[n].nonzero / 16.0f;
#if 0
   rdp.light[n].la        = rdp.light[n].ca * 1.0f;
#endif
}

void glide64gSPSetDMAOffsets(uint32_t mtxoffset, uint32_t vtxoffset)
{
  gSP.DMAOffsets.mtx = mtxoffset;
  gSP.DMAOffsets.vtx = vtxoffset;
  vtx_last = 0;
}

void glide64gSPDMAMatrix(uint32_t matrix, uint8_t index, uint8_t multiply)
{
   uint32_t address = gSP.DMAOffsets.mtx + RSP_SegmentToPhysical(matrix);

   gSP.matrix.modelViewi = index;

   if (multiply)
   {
      DECLAREALIGN16VAR(mtx[4][4]);
      DECLAREALIGN16VAR(m_src[4][4]);

      load_matrix(mtx, address);
      CopyMatrix(m_src, rdp.dkrproj[0]);
      MulMatrices(mtx, m_src, rdp.dkrproj[index]);
   }
   else
      load_matrix(rdp.dkrproj[index], address);

   g_gdp.flags |= UPDATE_MULT_MAT;
}

void glide64gSPDMAVertex(uint32_t v, uint32_t n, uint32_t v0)
{
   unsigned int i;
   uint32_t addr = gSP.DMAOffsets.vtx + RSP_SegmentToPhysical(v);

   // | cccc cccc 1111 1??? 0000 0002 2222 2222 | cmd1 = address |
   // c = vtx command
   // 1 = method #1 of getting count
   // 2 = method #2 of getting count
   // ? = unknown, but used
   // 0 = unused
   
   int prj = gSP.matrix.modelViewi;

   for (i = v0; i < n + v0; i++)
   {
      VERTEX *v = (VERTEX*)&rdp.vtx[i];
      int start = (i-v0) * 10;
      float x   = (float)((int16_t*)gfx_info.RDRAM)[(((addr+start) >> 1) + 0)^1];
      float y   = (float)((int16_t*)gfx_info.RDRAM)[(((addr+start) >> 1) + 1)^1];
      float z   = (float)((int16_t*)gfx_info.RDRAM)[(((addr+start) >> 1) + 2)^1];

      v->x = x*rdp.dkrproj[prj][0][0] + y*rdp.dkrproj[prj][1][0] + z*rdp.dkrproj[prj][2][0] + rdp.dkrproj[prj][3][0];
      v->y = x*rdp.dkrproj[prj][0][1] + y*rdp.dkrproj[prj][1][1] + z*rdp.dkrproj[prj][2][1] + rdp.dkrproj[prj][3][1];
      v->z = x*rdp.dkrproj[prj][0][2] + y*rdp.dkrproj[prj][1][2] + z*rdp.dkrproj[prj][2][2] + rdp.dkrproj[prj][3][2];
      v->w = x*rdp.dkrproj[prj][0][3] + y*rdp.dkrproj[prj][1][3] + z*rdp.dkrproj[prj][2][3] + rdp.dkrproj[prj][3][3];

      if (gSP.matrix.billboard)
      {
         v->x += rdp.vtx[0].x;
         v->y += rdp.vtx[0].y;
         v->z += rdp.vtx[0].z;
         v->w += rdp.vtx[0].w;
      }

      if (fabs(v->w) < 0.001)
         v->w = 0.001f;

      v->oow = 1.0f / v->w;
      v->x_w = v->x * v->oow;
      v->y_w = v->y * v->oow;
      v->z_w = v->z * v->oow;

      v->uv_calculated = 0xFFFFFFFF;
      v->screen_translated = 0;
      v->shade_mod = 0;

      v->scr_off = 0;
      if (v->x < -v->w)
         v->scr_off |= X_CLIP_MAX;
      if (v->x > v->w)
         v->scr_off |= X_CLIP_MIN;
      if (v->y < -v->w)
         v->scr_off |= Y_CLIP_MAX;
      if (v->y > v->w)
         v->scr_off |= Y_CLIP_MIN;
      if (v->w < 0.1f)
         v->scr_off |= Z_CLIP_MAX;
      if (fabs(v->z_w) > 1.0)
         v->scr_off |= Z_CLIP_MIN;

      v->r = ((uint8_t*)gfx_info.RDRAM)[(addr+start + 6)^3];
      v->g = ((uint8_t*)gfx_info.RDRAM)[(addr+start + 7)^3];
      v->b = ((uint8_t*)gfx_info.RDRAM)[(addr+start + 8)^3];
      v->a = ((uint8_t*)gfx_info.RDRAM)[(addr+start + 9)^3];
      calculateVertexFog (v);
   }
}

void glide64gSPDMATriangles(uint32_t tris, uint32_t n)
{
   int i;
   uint32_t addr = RSP_SegmentToPhysical(tris);

   vtx_last = 0;    // we've drawn something, so the vertex index needs resetting

   // | cccc cccc 2222 0000 1111 1111 1111 0000 | cmd1 = address |
   // c = tridma command
   // 1 = method #1 of getting count
   // 2 = method #2 of getting count
   // 0 = unused


   for (i = 0; i < n; i++)
   {
      VERTEX *v[3];
      unsigned cull_mode = GR_CULL_NEGATIVE;
      int start          = i << 4;
      int v0             = gfx_info.RDRAM[addr+start];
      int v1             = gfx_info.RDRAM[addr+start+1];
      int v2             = gfx_info.RDRAM[addr+start+2];
      int flags          = gfx_info.RDRAM[addr+start+3];

      rdp.flags &= ~G_CULL_BOTH;

      if (flags & 0x40)
      {
         /* no cull */
         cull_mode  = GR_CULL_DISABLE;
      }
      else
      {
         /* front cull */
         if (gSP.viewport.vscale[0] >= 0)
            rdp.flags |= CULL_FRONT;
         else
         {
            /* agh, backwards culling */
            rdp.flags |= CULL_BACK;   
            cull_mode  = GR_CULL_POSITIVE;
         }
      }

      grCullMode(cull_mode);

      start   += 4;

      v[0]     = &rdp.vtx[v0];
      v[1]     = &rdp.vtx[v1];
      v[2]     = &rdp.vtx[v2];

      v[0]->ou = _FIXED2FLOAT(((int16_t*)gfx_info.RDRAM)[((addr+start) >> 1) + 5], 5);
      v[0]->ov = _FIXED2FLOAT(((int16_t*)gfx_info.RDRAM)[((addr+start) >> 1) + 4], 5);
      v[1]->ou = _FIXED2FLOAT(((int16_t*)gfx_info.RDRAM)[((addr+start) >> 1) + 3], 5);
      v[1]->ov = _FIXED2FLOAT(((int16_t*)gfx_info.RDRAM)[((addr+start) >> 1) + 2], 5);
      v[2]->ou = _FIXED2FLOAT(((int16_t*)gfx_info.RDRAM)[((addr+start) >> 1) + 1], 5);
      v[2]->ov = _FIXED2FLOAT(((int16_t*)gfx_info.RDRAM)[((addr+start) >> 1) + 0], 5);

      v[0]->uv_calculated = 0xFFFFFFFF;
      v[1]->uv_calculated = 0xFFFFFFFF;
      v[2]->uv_calculated = 0xFFFFFFFF;

      cull_trianglefaces(v, 1, true, true, 0);
   }
}

void glide64gSPDisplayList(uint32_t dl)
{
   uint32_t addr = RSP_SegmentToPhysical(dl);

   if (__RSP.PCi >= 9)
      return;                  /* DL stack overflow */
   __RSP.PCi++;                /* go to the next PC in the stack */
   __RSP.PC[__RSP.PCi] = addr; /* jump to the address */
}

void glide64gSPBranchList(uint32_t dl)
{
   uint32_t addr = RSP_SegmentToPhysical(dl);
   __RSP.PC[__RSP.PCi] = addr; /* jump to the address */
}

void glide64gSPSetDMATexOffset(uint32_t addr)
{
   gSP.DMAOffsets.tex_offset = RSP_SegmentToPhysical(addr);
   gSP.DMAOffsets.tex_count  = 0;
}

void glide64gSPTexture(int32_t sc, int32_t tc, int32_t level, 
      int32_t tile, int32_t on)
{
   if (tile == 7 && (settings.hacks&hack_Supercross))
      tile = 0; /* fix for supercross 2000 */

   gDP.otherMode.textureDetail    = level;
   rdp.cur_tile                   = tile;
   gSP.texture.on                 = 0;

   if (on)
   {
      gSP.texture.on              = 1;
      gSP.texture.org_scales      = sc;
      gSP.texture.org_scalet      = tc;
      gSP.texture.scales          = _FIXED2FLOAT(((sc+1)/65536.0f), 5);
      gSP.texture.scalet          = _FIXED2FLOAT(((tc+1)/65536.0f), 5);

      g_gdp.flags |= UPDATE_TEXTURE;
   }
}

void modelview_load (float m[4][4])
{
   CopyMatrix(rdp.model, m);
   g_gdp.flags |= UPDATE_MULT_MAT | UPDATE_LIGHTS;
}

void modelview_mul (float m[4][4])
{
   MulMatrices(m, rdp.model, rdp.model);
   g_gdp.flags |= UPDATE_MULT_MAT | UPDATE_LIGHTS;
}

void modelview_push(void)
{
   if (gSP.matrix.modelViewi == gSP.matrix.stackSize)
      return;

   CopyMatrix(gSP.matrix.modelView[gSP.matrix.modelViewi++], rdp.model);
}

void modelview_load_push (float m[4][4])
{
   modelview_push();
   modelview_load(m);
}

void modelview_mul_push (float m[4][4])
{
   modelview_push();
   modelview_mul (m);
}

void projection_load (float m[4][4])
{
   CopyMatrix(rdp.proj, m);
   g_gdp.flags |= UPDATE_MULT_MAT;
}

void projection_mul (float m[4][4])
{
   MulMatrices(m, rdp.proj, rdp.proj);
   g_gdp.flags |= UPDATE_MULT_MAT;
}

void glide64gSPMatrix( uint32_t matrix, uint8_t param )
{
   DECLAREALIGN16VAR(m[4][4]);
   uint32_t addr   = RSP_SegmentToPhysical(matrix);

   load_matrix(m, addr);

   switch (param)
   {
      case G_MTX_NOPUSH: // modelview mul nopush
         modelview_mul (m);
         break;

      case 1: // projection mul nopush
      case 5: // projection mul push, can't push projection
         projection_mul(m);
         break;

      case G_MTX_LOAD: // modelview load nopush
         modelview_load(m);
         break;

      case 3: // projection load nopush
      case 7: // projection load push, can't push projection
         projection_load(m);
         break;

      case 4: // modelview mul push
         modelview_mul_push(m);
         break;

      case 6: // modelview load push
         modelview_load_push(m);
         break;
   }
}

void glide64gSPBranchLessZ(uint32_t branchdl, uint32_t vtx, float zval)
{
   uint32_t address = RSP_SegmentToPhysical(branchdl);
   float zTest = fabs(rdp.vtx[vtx].z);

   if(zTest > 1.0f || zTest <= zval)
      __RSP.PC[__RSP.PCi] = address;
}
