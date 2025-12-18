#include <string.h>
#include <stdlib.h>
#include "OpenGL.h"
#include "Combiner.h"
#include "Common.h"
#include "Textures.h"
#include "Config.h"

//(sa - sb) * m + a
static const uint32_t saRGBExpanded[] =
{
    COMBINED,           TEXEL0,             TEXEL1,             PRIMITIVE,
    SHADE,              ENVIRONMENT,        ONE,                NOISE,
    ZERO,               ZERO,               ZERO,               ZERO,
    ZERO,               ZERO,               ZERO,               ZERO
};

static const uint32_t sbRGBExpanded[] =
{
    COMBINED,           TEXEL0,             TEXEL1,             PRIMITIVE,
    SHADE,              ENVIRONMENT,        CENTER,             K4,
    ZERO,               ZERO,               ZERO,               ZERO,
    ZERO,               ZERO,               ZERO,               ZERO
};

static const uint32_t mRGBExpanded[] =
{
    COMBINED,           TEXEL0,             TEXEL1,             PRIMITIVE,
    SHADE,              ENVIRONMENT,        SCALE,              COMBINED_ALPHA,
    TEXEL0_ALPHA,       TEXEL1_ALPHA,       PRIMITIVE_ALPHA,    SHADE_ALPHA,
    ENV_ALPHA,          LOD_FRACTION,       PRIM_LOD_FRAC,      K5,
    ZERO,               ZERO,               ZERO,               ZERO,
    ZERO,               ZERO,               ZERO,               ZERO,
    ZERO,               ZERO,               ZERO,               ZERO,
    ZERO,               ZERO,               ZERO,               ZERO
};

static const uint32_t aRGBExpanded[] =
{
    COMBINED,           TEXEL0,             TEXEL1,             PRIMITIVE,
    SHADE,              ENVIRONMENT,        ONE,                ZERO
};

static const uint32_t saAExpanded[] =
{
    COMBINED,           TEXEL0_ALPHA,       TEXEL1_ALPHA,       PRIMITIVE_ALPHA,
    SHADE_ALPHA,        ENV_ALPHA,          ONE,                ZERO
};

static const uint32_t sbAExpanded[] =
{
    COMBINED,           TEXEL0_ALPHA,       TEXEL1_ALPHA,       PRIMITIVE_ALPHA,
    SHADE_ALPHA,        ENV_ALPHA,          ONE,                ZERO
};

static const uint32_t mAExpanded[] =
{
    LOD_FRACTION,       TEXEL0_ALPHA,       TEXEL1_ALPHA,       PRIMITIVE_ALPHA,
    SHADE_ALPHA,        ENV_ALPHA,          PRIM_LOD_FRAC,      ZERO,
};

static const uint32_t aAExpanded[] =
{
    COMBINED,           TEXEL0_ALPHA,       TEXEL1_ALPHA,       PRIMITIVE_ALPHA,
    SHADE_ALPHA,        ENV_ALPHA,          ONE,                ZERO
};

static bool mux_find(DecodedMux *dmux, int index, int src)
{
      if (dmux->decode[index].sa == src) return true;
      if (dmux->decode[index].sb == src) return true;
      if (dmux->decode[index].m == src) return true;
      if (dmux->decode[index].a == src) return true;
   return false;
}

static bool mux_swap(DecodedMux *dmux, int cycle, int src0, int src1)
{
   int i, r;
   r = false;
   for(i = 0; i < 2; i++)
   {
      int ii = (cycle == 0) ? i : (2+i);
      {
         if (dmux->decode[ii].sa == src0) {dmux->decode[ii].sa = src1; r=true;}
         else if (dmux->decode[ii].sa == src1) {dmux->decode[ii].sa = src0; r=true;}

         if (dmux->decode[ii].sb == src0) {dmux->decode[ii].sb = src1; r=true;}
         else if (dmux->decode[ii].sb == src1) {dmux->decode[ii].sb = src0; r=true;}

         if (dmux->decode[ii].m == src0) {dmux->decode[ii].m = src1; r=true;}
         else if (dmux->decode[ii].m == src1) {dmux->decode[ii].m = src0; r=true;}

         if (dmux->decode[ii].a == src0) {dmux->decode[ii].a = src1; r=true;}
         else if (dmux->decode[ii].a == src1) {dmux->decode[ii].a = src0; r=true;}
      }
   }
   return r;
}

static bool mux_replace(DecodedMux *dmux, int cycle, int src, int dest)
{
   int i, r;
   r = false;

   for(i = 0; i < 2; i++)
   {
      int ii = (cycle == 0) ? i : (2+i);
      if (dmux->decode[ii].sa == src) {dmux->decode[ii].sa = dest; r=true;}
      if (dmux->decode[ii].sb == src) {dmux->decode[ii].sb = dest; r=true;}
      if (dmux->decode[ii].m  == src) {dmux->decode[ii].m  = dest; r=true;}
      if (dmux->decode[ii].a  == src) {dmux->decode[ii].a  = dest; r=true;}
   }
   return r;
}

static void *mux_new(uint64_t dmux, bool cycle2)
{
   int i;
   DecodedMux *mux = malloc(sizeof(DecodedMux)); 

   mux->combine.mux = dmux;
   mux->flags = 0;

   //set to ZERO.
   for(i = 0; i < 4;i++)
   {
      mux->decode[i].sa = ZERO;
      mux->decode[i].sb = ZERO;
      mux->decode[i].m  = ZERO;
      mux->decode[i].a  = ZERO;
   }

   //rgb cycle 0
   mux->decode[0].sa = saRGBExpanded[mux->combine.saRGB0];
   mux->decode[0].sb = sbRGBExpanded[mux->combine.sbRGB0];
   mux->decode[0].m  = mRGBExpanded[mux->combine.mRGB0];
   mux->decode[0].a  = aRGBExpanded[mux->combine.aRGB0];
   mux->decode[1].sa = saAExpanded[mux->combine.saA0];
   mux->decode[1].sb = sbAExpanded[mux->combine.sbA0];
   mux->decode[1].m  = mAExpanded[mux->combine.mA0];
   mux->decode[1].a  = aAExpanded[mux->combine.aA0];

   if (cycle2)
   {
      //rgb cycle 1
      mux->decode[2].sa = saRGBExpanded[mux->combine.saRGB1];
      mux->decode[2].sb = sbRGBExpanded[mux->combine.sbRGB1];
      mux->decode[2].m  = mRGBExpanded[mux->combine.mRGB1];
      mux->decode[2].a  = aRGBExpanded[mux->combine.aRGB1];
      mux->decode[3].sa = saAExpanded[mux->combine.saA1];
      mux->decode[3].sb = sbAExpanded[mux->combine.sbA1];
      mux->decode[3].m  = mAExpanded[mux->combine.mA1];
      mux->decode[3].a  = aAExpanded[mux->combine.aA1];

      //texel 0/1 are swapped in 2nd cycle.
      mux_swap(mux, 1, TEXEL0, TEXEL1);
      mux_swap(mux, 1, TEXEL0_ALPHA, TEXEL1_ALPHA);
   }

   //simplifying mux:
   if (mux_replace(mux, G_CYC_1CYCLE, LOD_FRACTION, ZERO) || mux_replace(mux, G_CYC_2CYCLE, LOD_FRACTION, ZERO))
      LOG(LOG_VERBOSE, "SC Replacing LOD_FRACTION with ZERO\n");
#if 1
   if (mux_replace(mux, G_CYC_1CYCLE, K4, ZERO) || mux_replace(mux, G_CYC_2CYCLE, K4, ZERO))
      LOG(LOG_VERBOSE, "SC Replacing K4 with ZERO\n");

   if (mux_replace(mux, G_CYC_1CYCLE, K5, ZERO) || mux_replace(mux, G_CYC_2CYCLE, K5, ZERO))
      LOG(LOG_VERBOSE, "SC Replacing K5 with ZERO\n");
#endif

   if (mux_replace(mux, G_CYC_1CYCLE, CENTER, ZERO) || mux_replace(mux, G_CYC_2CYCLE, CENTER, ZERO))
      LOG(LOG_VERBOSE, "SC Replacing CENTER with ZERO\n");

   if (mux_replace(mux, G_CYC_1CYCLE, SCALE, ZERO) || mux_replace(mux, G_CYC_2CYCLE, SCALE, ZERO))
      LOG(LOG_VERBOSE, "SC Replacing SCALE with ZERO\n");

   //Combiner has initial value of zero in cycle 0
   if (mux_replace(mux, G_CYC_1CYCLE, COMBINED, ZERO))
      LOG(LOG_VERBOSE, "SC Setting CYCLE1 COMBINED to ZERO\n");

   if (mux_replace(mux, G_CYC_1CYCLE, COMBINED_ALPHA, ZERO))
      LOG(LOG_VERBOSE, "SC Setting CYCLE1 COMBINED_ALPHA to ZERO\n");

   if (!config.enableNoise)
   {
      if (mux_replace(mux, G_CYC_1CYCLE, NOISE, ZERO))
         LOG(LOG_VERBOSE, "SC Setting CYCLE1 NOISE to ZERO\n");

      if (mux_replace(mux, G_CYC_2CYCLE, NOISE, ZERO))
         LOG(LOG_VERBOSE, "SC Setting CYCLE2 NOISE to ZERO\n");

   }

   //mutiplying by zero: (A-B)*0 + C = C
   for(i = 0 ; i < 4; i++)
   {
      if (mux->decode[i].m == ZERO)
      {
         mux->decode[i].sa = ZERO;
         mux->decode[i].sb = ZERO;
      }
   }

   //(A1-B1)*C1 + D1
   //(A2-B2)*C2 + D2
   //1. ((A1-B1)*C1 + D1 - B2)*C2 + D2 = A1*C1*C2 - B1*C1*C2 + D1*C2 - B2*C2 + D2
   //2. (A2 - (A1-B1)*C1 - D1)*C2 + D2 = A2*C2 - A1*C1*C2 + B1*C1*C2 - D1*C2 + D2
   //3. (A2 - B2)*((A1-B1)*C1 + D1) + D2 = A2*A1*C1 - A2*B1*C1 + A2*D1 - B2*A1*C1 + B2*B1*C1 - B2*D1 + D2
   //4. (A2-B2)*C2 + (A1-B1)*C1 + D1 = A2*C2 - B2*C2 + A1*C1 - B1*C1 + D1

   if (cycle2)
   {

      if (!mux_find(mux, 2, COMBINED))
         mux->flags |= SC_IGNORE_RGB0;

      if (!(mux_find(mux, 2, COMBINED_ALPHA) || mux_find(mux, 3, COMBINED_ALPHA) || mux_find(mux, 3, COMBINED)))
         mux->flags |= SC_IGNORE_ALPHA0;

      if (mux->decode[2].sa == ZERO && mux->decode[2].sb == ZERO && mux->decode[2].m == ZERO && mux->decode[2].a == COMBINED)
         mux->flags |= SC_IGNORE_RGB1;

      if (mux->decode[3].sa == ZERO && mux->decode[3].sb == ZERO && mux->decode[3].m == ZERO &&
            (mux->decode[3].a == COMBINED_ALPHA || mux->decode[3].a == COMBINED))
         mux->flags |= SC_IGNORE_ALPHA1;
   }

   return mux;
}

void Combiner_Init(void)
{
   ShaderCombiner_Init();
}

void Combiner_Destroy(void)
{
   ShaderCombiner_Destroy();
}

void Combiner_Set(uint64_t mux, int flags)
{
   DecodedMux *dmux;

   //determine flags
   if (flags == -1)
   {
      flags = 0;
      if ((gSP.geometryMode & G_FOG))
         flags |= SC_FOGENABLED;

      if ((gDP.otherMode.alphaCompare == G_AC_THRESHOLD) && !(gDP.otherMode.alphaCvgSel))
      {
         flags |= SC_ALPHAENABLED;
         if (gDP.blendColor.a > 0.0f)
            flags |= SC_ALPHAGREATER;
      }
      else if (gDP.otherMode.cvgXAlpha)
      {
         flags |= SC_ALPHAENABLED;
         flags |= SC_ALPHAGREATER;
      }

      if (gDP.otherMode.cycleType == G_CYC_2CYCLE)
         flags |= SC_2CYCLE;
   }

   dmux = (DecodedMux*)mux_new(mux, flags & SC_2CYCLE);

   ShaderCombiner_Set(dmux, flags);

   if (dmux)
      free(dmux);
}
