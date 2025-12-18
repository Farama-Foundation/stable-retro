#ifndef _3DMATH_H
#define _3DMATH_H

#ifdef __cplusplus
extern "C" {
#endif

enum gfx_plugin_type
{
   GFX_GLIDE64 = 0,
   GFX_RICE,
   GFX_GLN64,
   GFX_ANGRYLION,
   GFX_PARALLEL
};

enum rsp_plugin_type
{
   RSP_HLE = 0,
   RSP_CXD4,
   RSP_PARALLEL
};

extern enum gfx_plugin_type gfx_plugin;

#ifdef __cplusplus
}
#endif

#endif
