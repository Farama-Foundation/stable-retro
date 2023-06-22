#include "gSP_funcs.h"
#include "../plugin.h"

void GSPCombineMatrices(void)
{
   switch (gfx_plugin)
   {
      case GFX_GLIDE64:
#ifdef HAVE_GLIDE64
         glide64gSPCombineMatrices();
#endif
         break;
      case GFX_GLN64:
#if defined(HAVE_GLN64) || defined(HAVE_GLIDEN64)
         gln64gSPCombineMatrices();
#endif
         break;
      case GFX_RICE:
#if defined(HAVE_RICE)
         /* TODO/FIXME */
#endif
         break;
      case GFX_ANGRYLION:
      case GFX_PARALLEL:
         /* Stub, not HLE */
         break;
   }
}

void GSPDlistCount(uint32_t count, uint32_t v)
{
   switch (gfx_plugin)
   {
      case GFX_GLIDE64:
#ifdef HAVE_GLIDE64
         glide64gSPDlistCount(count, v);
#endif
         break;
      case GFX_GLN64:
#if defined(HAVE_GLN64) || defined(HAVE_GLIDEN64)
         gln64gSPDlistCount(count, v);
#endif
         break;
      case GFX_RICE:
#ifdef HAVE_RICE
         /* TODO/FIXME */
#endif
         break;
      case GFX_ANGRYLION:
      case GFX_PARALLEL:
         /* Stub, not HLE */
         break;
   }
}

void GSPClipVertex(uint32_t v)
{
   switch (gfx_plugin)
   {
      case GFX_GLIDE64:
#ifdef HAVE_GLIDE64
         glide64gSPClipVertex(v);
#endif
         break;
      case GFX_GLN64:
#if defined(HAVE_GLN64) || defined(HAVE_GLIDEN64)
         gln64gSPClipVertex(v);
#endif
         break;
      case GFX_RICE:
#ifdef HAVE_RICE
         /* TODO/FIXME */
#endif
         break;
      case GFX_ANGRYLION:
      case GFX_PARALLEL:
         /* Stub, not HLE */
         break;
   }
}

/* Loads a LookAt structure in the RSP for specular highlighting
 * and projection mapping.
 *
 * l             - The lookat structure address.
 */
void GSPLookAt(uint32_t l, uint32_t n)
{
   switch (gfx_plugin)
   {
      case GFX_GLIDE64:
#ifdef HAVE_GLIDE64
         glide64gSPLookAt(l, n);
#endif
         break;
      case GFX_GLN64:
#if defined(HAVE_GLN64) || defined(HAVE_GLIDEN64)
         gln64gSPLookAt(l, n);
#endif
         break;
      case GFX_RICE:
#ifdef HAVE_RICE
         /* TODO/FIXME */
#endif
         break;
      case GFX_ANGRYLION:
      case GFX_PARALLEL:
         /* Stub, not HLE */
         break;
   }
}

/* Loads one light structure to the RSP.
 *
 * l             - The pointer to the light structure.
 * n             - The light number that is replaced (1~8)
 */
void GSPLight(uint32_t l, int32_t n)
{
   switch (gfx_plugin)
   {
      case GFX_GLIDE64:
#ifdef HAVE_GLIDE64
         glide64gSPLight(l, n);
#endif
         break;
      case GFX_GLN64:
#if defined(HAVE_GLN64) || defined(HAVE_GLIDEN64)
         gln64gSPLight(l, n);
#endif
         break;
      case GFX_RICE:
#ifdef HAVE_RICE
         /* TODO/FIXME */
#endif
         break;
      case GFX_ANGRYLION:
      case GFX_PARALLEL:
         /* Stub, not HLE */
         break;
   }
}

/* Quickly changes the light color in the RSP.
 *
 * lightNum     - The light number whose color is being modified:
 *                LIGHT_1 (First light)
 *                LIGHT_2 (Second light)
 *                :
 *                LIGHT_u (Eighth light)
 *
 * packedColor - The new light color (32-bit value 0xRRGGBB??)
 *               (?? is ignored)
 * */

void GSPLightColor(uint32_t lightNum, uint32_t packedColor )
{
   switch (gfx_plugin)
   {
      case GFX_GLIDE64:
#ifdef HAVE_GLIDE64
         glide64gSPLightColor(lightNum, packedColor);
#endif
         break;
      case GFX_GLN64:
#ifdef HAVE_GLN64
         gln64gSPLightColor(lightNum, packedColor);
#endif
         break;
      case GFX_RICE:
#ifdef HAVE_RICE
         /* TODO/FIXME */
#endif
         break;
      case GFX_ANGRYLION:
      case GFX_PARALLEL:
         /* Stub, not HLE */
         break;
   }
}

/* Loads the viewport projection parameters.
 *
 * v           - v is the segment address to the viewport
 *               structure "Vp".
 * */
void GSPViewport(uint32_t v)
{
   switch (gfx_plugin)
   {
      case GFX_GLIDE64:
#ifdef HAVE_GLIDE64
         glide64gSPViewport(v);
#endif
         break;
      case GFX_GLN64:
#if defined(HAVE_GLN64) || defined(HAVE_GLIDEN64)
         gln64gSPViewport(v);
#endif
         break;
      case GFX_RICE:
#ifdef HAVE_RICE
         /* TODO/FIXME */
#endif
         break;
      case GFX_ANGRYLION:
      case GFX_PARALLEL:
         /* Stub, not HLE */
         break;
   }
}

void GSPForceMatrix(uint32_t mptr)
{
   switch (gfx_plugin)
   {
      case GFX_GLIDE64:
#ifdef HAVE_GLIDE64
         glide64gSPForceMatrix(mptr);
#endif
         break;
      case GFX_GLN64:
#if defined(HAVE_GLN64) || defined(HAVE_GLIDEN64)
         gln64gSPForceMatrix(mptr);
#endif
         break;
      case GFX_RICE:
#ifdef HAVE_RICE
         /* TODO/FIXME */
#endif
         break;
      case GFX_ANGRYLION:
      case GFX_PARALLEL:
         /* Stub, not HLE */
         break;
   }
}

extern "C" void GSPEndDisplayListC(void);

void GSPEndDisplayList(void)
{
   GSPEndDisplayListC();
}
