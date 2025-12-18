#ifndef __GLIDE_H__
#define __GLIDE_H__

#define GL_GLEXT_PROTOTYPES
#include <boolean.h>
#include <glsm/glsmsym.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
** -----------------------------------------------------------------------
** TYPE DEFINITIONS
** -----------------------------------------------------------------------
*/

/*
** fundamental types
*/
#define FXTRUE    1
#define FXFALSE   0

/*
** helper macros
*/
#define FXBIT( i )    ( 1L << (i) )

typedef int (*GrProc)();

/*
** -----------------------------------------------------------------------
** CONSTANTS AND TYPES
** -----------------------------------------------------------------------
*/
#define GR_MIPMAPLEVELMASK_EVEN  FXBIT(0)
#define GR_MIPMAPLEVELMASK_ODD  FXBIT(1)
#define GR_MIPMAPLEVELMASK_BOTH (GR_MIPMAPLEVELMASK_EVEN | GR_MIPMAPLEVELMASK_ODD )

#define GR_LODBIAS_BILINEAR     0.5
#define GR_LODBIAS_TRILINEAR    0.0

#define GR_TMU0         0x0
#define GR_TMU1         0x1
#define GR_TMU2         0x2

#define GR_COMBINE_FUNCTION_ZERO        0x0
#define GR_COMBINE_FUNCTION_NONE        GR_COMBINE_FUNCTION_ZERO
#define GR_COMBINE_FUNCTION_LOCAL       0x1
#define GR_COMBINE_FUNCTION_LOCAL_ALPHA 0x2
#define GR_COMBINE_FUNCTION_SCALE_OTHER 0x3
#define GR_COMBINE_FUNCTION_BLEND_OTHER GR_COMBINE_FUNCTION_SCALE_OTHER
#define GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL 0x4
#define GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL_ALPHA 0x5 
#define GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL 0x6
#define GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL_ADD_LOCAL 0x7
#define GR_COMBINE_FUNCTION_BLEND GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL_ADD_LOCAL
#define GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL_ADD_LOCAL_ALPHA 0x8
#define GR_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL 0x9
#define GR_COMBINE_FUNCTION_BLEND_LOCAL GR_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL
#define GR_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL_ALPHA 0x10

#define GR_COMBINE_FACTOR_ZERO          0x0
#define GR_COMBINE_FACTOR_NONE          GR_COMBINE_FACTOR_ZERO
#define GR_COMBINE_FACTOR_LOCAL         0x1
#define GR_COMBINE_FACTOR_OTHER_ALPHA   0x2
#define GR_COMBINE_FACTOR_LOCAL_ALPHA   0x3
#define GR_COMBINE_FACTOR_TEXTURE_ALPHA 0x4
#define GR_COMBINE_FACTOR_TEXTURE_RGB   0x5
#define GR_COMBINE_FACTOR_DETAIL_FACTOR GR_COMBINE_FACTOR_TEXTURE_ALPHA
#define GR_COMBINE_FACTOR_LOD_FRACTION  0x5
#define GR_COMBINE_FACTOR_ONE           0x8
#define GR_COMBINE_FACTOR_ONE_MINUS_LOCAL 0x9
#define GR_COMBINE_FACTOR_ONE_MINUS_OTHER_ALPHA 0xa
#define GR_COMBINE_FACTOR_ONE_MINUS_LOCAL_ALPHA 0xb
#define GR_COMBINE_FACTOR_ONE_MINUS_TEXTURE_ALPHA 0xc
#define GR_COMBINE_FACTOR_ONE_MINUS_DETAIL_FACTOR GR_COMBINE_FACTOR_ONE_MINUS_TEXTURE_ALPHA
#define GR_COMBINE_FACTOR_ONE_MINUS_LOD_FRACTION 0xd


#define GR_COMBINE_LOCAL_ITERATED 0x0
#define GR_COMBINE_LOCAL_CONSTANT 0x1
#define GR_COMBINE_LOCAL_NONE GR_COMBINE_LOCAL_CONSTANT
#define GR_COMBINE_LOCAL_DEPTH  0x2

#define GR_COMBINE_OTHER_ITERATED 0x0
#define GR_COMBINE_OTHER_TEXTURE 0x1
#define GR_COMBINE_OTHER_CONSTANT 0x2
#define GR_COMBINE_OTHER_NONE GR_COMBINE_OTHER_CONSTANT

#define GR_ALPHASOURCE_CC_ALPHA 0x0
#define GR_ALPHASOURCE_ITERATED_ALPHA 0x1
#define GR_ALPHASOURCE_TEXTURE_ALPHA 0x2
#define GR_ALPHASOURCE_TEXTURE_ALPHA_TIMES_ITERATED_ALPHA 0x3

#define GR_COLORCOMBINE_ZERO 0x0
#define GR_COLORCOMBINE_CCRGB 0x1
#define GR_COLORCOMBINE_ITRGB 0x2
#define GR_COLORCOMBINE_ITRGB_DELTA0 0x3
#define GR_COLORCOMBINE_DECAL_TEXTURE 0x4
#define GR_COLORCOMBINE_TEXTURE_TIMES_CCRGB 0x5
#define GR_COLORCOMBINE_TEXTURE_TIMES_ITRGB 0x6
#define GR_COLORCOMBINE_TEXTURE_TIMES_ITRGB_DELTA0 0x7
#define GR_COLORCOMBINE_TEXTURE_TIMES_ITRGB_ADD_ALPHA 0x8
#define GR_COLORCOMBINE_TEXTURE_TIMES_ALPHA 0x9
#define GR_COLORCOMBINE_TEXTURE_TIMES_ALPHA_ADD_ITRGB 0xa
#define GR_COLORCOMBINE_TEXTURE_ADD_ITRGB 0xb
#define GR_COLORCOMBINE_TEXTURE_SUB_ITRGB 0xc
#define GR_COLORCOMBINE_CCRGB_BLEND_ITRGB_ON_TEXALPHA 0xd
#define GR_COLORCOMBINE_DIFF_SPEC_A 0xe
#define GR_COLORCOMBINE_DIFF_SPEC_B 0xf
#define GR_COLORCOMBINE_ONE 0x10

#define GR_BLEND_ZERO GL_ZERO
#define GR_BLEND_SRC_ALPHA GL_SRC_ALPHA
#define GR_BLEND_SRC_COLOR 0x2
#define GR_BLEND_DST_COLOR GR_BLEND_SRC_COLOR
#define GR_BLEND_DST_ALPHA 0x3 
#define GR_BLEND_ONE GL_ONE
#define GR_BLEND_ONE_MINUS_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
#define GR_BLEND_ONE_MINUS_SRC_COLOR 0x6
#define GR_BLEND_ONE_MINUS_DST_COLOR GR_BLEND_ONE_MINUS_SRC_COLOR 
#define GR_BLEND_ONE_MINUS_DST_ALPHA 0x7
#define GR_BLEND_RESERVED_8 0x8
#define GR_BLEND_RESERVED_9 0x9
#define GR_BLEND_RESERVED_A 0xa
#define GR_BLEND_RESERVED_B 0xb
#define GR_BLEND_RESERVED_C 0xc
#define GR_BLEND_RESERVED_D 0xd
#define GR_BLEND_RESERVED_E 0xe
#define GR_BLEND_ALPHA_SATURATE 0xf
#define GR_BLEND_PREFOG_COLOR GR_BLEND_ALPHA_SATURATE

#define GR_ASPECT_LOG2_8x1        3       /* 8W x 1H */
#define GR_ASPECT_LOG2_4x1        2       /* 4W x 1H */
#define GR_ASPECT_LOG2_2x1        1       /* 2W x 1H */
#define GR_ASPECT_LOG2_1x1        0       /* 1W x 1H */
#define GR_ASPECT_LOG2_1x2       -1       /* 1W x 2H */
#define GR_ASPECT_LOG2_1x4       -2       /* 1W x 4H */
#define GR_ASPECT_LOG2_1x8       -3       /* 1W x 8H */

#define GR_BUFFER_FRONTBUFFER   0x0
#define GR_BUFFER_BACKBUFFER    0x1
#define GR_BUFFER_AUXBUFFER     0x2
#define GR_BUFFER_DEPTHBUFFER   0x3
#define GR_BUFFER_ALPHABUFFER   0x4
#define GR_BUFFER_TRIPLEBUFFER  0x5

#define GR_CHROMAKEY_DISABLE    0x0
#define GR_CHROMAKEY_ENABLE     0x1

#define GR_CHROMARANGE_RGB_ALL_EXT  0x0

#define GR_CHROMARANGE_DISABLE_EXT  0x00
#define GR_CHROMARANGE_ENABLE_EXT   0x01

#define GR_TEXCHROMA_DISABLE_EXT               0x0
#define GR_TEXCHROMA_ENABLE_EXT                0x1

#define GR_TEXCHROMARANGE_RGB_ALL_EXT  0x0

#define GR_CMP_NEVER    GL_NEVER
#define GR_CMP_LESS     GL_LESS
#define GR_CMP_EQUAL    GL_EQUAL
#define GR_CMP_LEQUAL   GL_LEQUAL
#define GR_CMP_GREATER  GL_GREATER
#define GR_CMP_NOTEQUAL GL_NOTEQUAL
#define GR_CMP_GEQUAL   GL_EQUAL
#define GR_CMP_ALWAYS   GL_ALWAYS

#define GR_COLORFORMAT_ARGB     0x0
#define GR_COLORFORMAT_ABGR     0x1

#define GR_COLORFORMAT_RGBA     0x2
#define GR_COLORFORMAT_BGRA     0x3

#define GR_CULL_DISABLE         0x0
#define GR_CULL_NEGATIVE        0x1
#define GR_CULL_POSITIVE        0x2

#define GR_DEPTHBUFFER_DISABLE                  0x0
#define GR_DEPTHBUFFER_ZBUFFER                  0x1
#define GR_DEPTHBUFFER_WBUFFER                  0x2
#define GR_DEPTHBUFFER_ZBUFFER_COMPARE_TO_BIAS  0x3
#define GR_DEPTHBUFFER_WBUFFER_COMPARE_TO_BIAS  0x4

#define GR_DITHER_DISABLE       0x0
#define GR_DITHER_2x2           0x1
#define GR_DITHER_4x4           0x2

#define GR_STIPPLE_DISABLE	0x0
#define GR_STIPPLE_PATTERN	0x1
#define GR_STIPPLE_ROTATE	0x2

#define GR_FOG_DISABLE                     0x0
#define GR_FOG_WITH_TABLE_ON_Q             0x1
#define GR_FOG_WITH_TABLE_ON_FOGCOORD_EXT  0x2

#define GR_LFB_READ_ONLY  0x00
#define GR_LFB_WRITE_ONLY 0x01
#define GR_LFB_IDLE       0x00
#define GR_LFB_NOIDLE     0x10

#define GR_LFB_WRITE_ONLY_EXPLICIT_EXT	0x02 /* explicitly not allow reading from the lfb pointer */

#define GR_LFBBYPASS_DISABLE    0x0
#define GR_LFBBYPASS_ENABLE     0x1

#define GR_LFBWRITEMODE_565        0x0 /* RGB:RGB */
#define GR_LFBWRITEMODE_555        0x1 /* RGB:RGB */
#define GR_LFBWRITEMODE_1555       0x2 /* ARGB:ARGB */
#define GR_LFBWRITEMODE_RESERVED1  0x3
#define GR_LFBWRITEMODE_888        0x4 /* RGB */
#define GR_LFBWRITEMODE_8888       0x5 /* ARGB */
#define GR_LFBWRITEMODE_RESERVED2  0x6
#define GR_LFBWRITEMODE_RESERVED3  0x7
#define GR_LFBWRITEMODE_RESERVED4  0x8
#define GR_LFBWRITEMODE_RESERVED5  0x9
#define GR_LFBWRITEMODE_RESERVED6  0xa
#define GR_LFBWRITEMODE_RESERVED7  0xb
#define GR_LFBWRITEMODE_565_DEPTH  0xc /* RGB:DEPTH */
#define GR_LFBWRITEMODE_555_DEPTH  0xd /* RGB:DEPTH */
#define GR_LFBWRITEMODE_1555_DEPTH 0xe /* ARGB:DEPTH */
#define GR_LFBWRITEMODE_ZA16       0xf /* DEPTH:DEPTH */
#define GR_LFBWRITEMODE_ANY        0xFF


#define GR_ORIGIN_UPPER_LEFT    0x0
#define GR_ORIGIN_LOWER_LEFT    0x1
#define GR_ORIGIN_ANY           0xFF

typedef struct {
    int                size;
    void               *lfbPtr;
    uint32_t              strideInBytes;        
    int32_t   writeMode;
    int32_t origin;
} GrLfbInfo_t;

#define GR_LOD_LOG2_256         0x8
#define GR_LOD_LOG2_128         0x7
#define GR_LOD_LOG2_64          0x6
#define GR_LOD_LOG2_32          0x5
#define GR_LOD_LOG2_16          0x4
#define GR_LOD_LOG2_8           0x3
#define GR_LOD_LOG2_4           0x2
#define GR_LOD_LOG2_2           0x1
#define GR_LOD_LOG2_1           0x0

#define GR_MIPMAP_DISABLE               0x0 /* no mip mapping  */
#define GR_MIPMAP_NEAREST               0x1 /* use nearest mipmap */
#define GR_MIPMAP_NEAREST_DITHER        0x2 /* GR_MIPMAP_NEAREST + LOD dith */

#define GR_SMOOTHING_DISABLE    0x0
#define GR_SMOOTHING_ENABLE     0x1

#define GR_TEXTURECLAMP_WRAP        GL_REPEAT
#define GR_TEXTURECLAMP_CLAMP       GL_CLAMP_TO_EDGE
#define GR_TEXTURECLAMP_MIRROR_EXT  GL_MIRRORED_REPEAT

#define GR_TEXTUREFILTER_POINT_SAMPLED  0x0
#define GR_TEXTUREFILTER_3POINT_LINEAR  0x1
#define GR_TEXTUREFILTER_BILINEAR       0x2

/* KoolSmoky - */
#define GR_TEXFMT_8BIT                  0x0
#define GR_TEXFMT_RGB_332               GR_TEXFMT_8BIT
#define GR_TEXFMT_YIQ_422               0x1
#define GR_TEXFMT_ALPHA_8               0x2 /* (0..0xFF) alpha     */
#define GR_TEXFMT_INTENSITY_8           0x3 /* (0..0xFF) intensity */
#define GR_TEXFMT_ALPHA_INTENSITY_44    0x4
#define GR_TEXFMT_P_8                   0x5 /* 8-bit palette */
#define GR_TEXFMT_RSVD0                 0x6 /* GR_TEXFMT_P_8_RGBA */
#define GR_TEXFMT_P_8_6666              GR_TEXFMT_RSVD0
#define GR_TEXFMT_P_8_6666_EXT          GR_TEXFMT_RSVD0
#define GR_TEXFMT_RSVD1                 0x7
#define GR_TEXFMT_16BIT                 0x8
#define GR_TEXFMT_ARGB_8332             GR_TEXFMT_16BIT
#define GR_TEXFMT_AYIQ_8422             0x9
#define GR_TEXFMT_RGB_565               0xa
#define GR_TEXFMT_ARGB_1555             0xb
#define GR_TEXFMT_ARGB_4444             0xc
#define GR_TEXFMT_ALPHA_INTENSITY_88    0xd
#define GR_TEXFMT_AP_88                 0xe /* 8-bit alpha 8-bit palette */
#define GR_TEXFMT_RSVD2                 0xf
#define GR_TEXFMT_RSVD4                 GR_TEXFMT_RSVD2

#define GR_MODE_DISABLE     0x0
#define GR_MODE_ENABLE      0x1

/* Types of data in strips */
#define GR_FLOAT        0
#define GR_U8           1

/*
** grDrawVertexArray/grDrawVertexArrayContiguous primitive type
*/
#define GR_POINTS                        0
#define GR_LINE_STRIP                    1
#define GR_LINES                         2
#define GR_POLYGON                       3
#define GR_TRIANGLE_STRIP                GL_TRIANGLE_STRIP
#define GR_TRIANGLE_FAN                  GL_TRIANGLE_FAN
#define GR_TRIANGLES                     GL_TRIANGLES

/*
** grGetString types
*/
#define GR_EXTENSION                    0xa0
#define GR_HARDWARE                     0xa1
#define GR_RENDERER                     0xa2
#define GR_VENDOR                       0xa3
#define GR_VERSION                      0xa4

/*
** -----------------------------------------------------------------------
** STRUCTURES
** -----------------------------------------------------------------------
*/

typedef struct {
    int32_t           smallLodLog2;
    int32_t           largeLodLog2;
    int32_t   aspectRatioLog2;
    int32_t format;
    int               width;
    int               height;
    void              *data;
} GrTexInfo;

#define GR_LFB_SRC_FMT_565          0x00
#define GR_LFB_SRC_FMT_555          0x01
#define GR_LFB_SRC_FMT_1555         0x02
#define GR_LFB_SRC_FMT_888          0x04
#define GR_LFB_SRC_FMT_8888         0x05
#define GR_LFB_SRC_FMT_565_DEPTH    0x0c
#define GR_LFB_SRC_FMT_555_DEPTH    0x0d
#define GR_LFB_SRC_FMT_1555_DEPTH   0x0e
#define GR_LFB_SRC_FMT_ZA16         0x0f
#define GR_LFB_SRC_FMT_RLE16        0x80

/*
** -----------------------------------------------------------------------
** FUNCTION PROTOTYPES
** -----------------------------------------------------------------------
*/

/*
** rendering functions
*/
void grDrawPoint( const void *pt );

void grDrawLine( const void *v1, const void *v2 );

void grVertexLayout(uint32_t param, int32_t offset, uint32_t mode);

void grDrawVertexArrayContiguous(uint32_t mode, uint32_t Count, void *pointers);

void grBufferClear(uint32_t color, uint32_t alpha, uint32_t depth);

void grBufferSwap(uint32_t swap_interval);


/*
 * Vertex Cache functions
 *
 * these are public just to allow reuse of the vertex cache vbo in some
 * specific places. you probably do not want to use them. */
void vbo_enable();
void vbo_disable();
void vbo_bind();
void vbo_unbind();
void vbo_buffer_data(void *data, size_t size);

/*
** error management
*/
typedef void (*GrErrorCallbackFnc_t)( const char *string, int32_t fatal );

void grErrorSetCallback( GrErrorCallbackFnc_t fnc );

/*
** SST routines
*/
uint32_t grSstWinOpen(void);

 int32_t 
grSstWinClose( uint32_t context );

/*
** Glide configuration and special effect maintenance functions
*/

void grAlphaBlendFunction(GLenum rgb_sf, GLenum rgb_df, GLenum alpha_sf, GLenum alpha_df);

void grAlphaCombine(
               int32_t function, int32_t factor,
               int32_t local, int32_t other,
               int32_t invert
               );

void  grAlphaTestFunction( int32_t function, uint8_t value, int set_alpha_ref);

void  grAlphaTestReferenceValue( uint8_t value );

void  grChromakeyMode( int32_t mode );

void grChromakeyValue( uint32_t value );

void grClipWindow(uint32_t minx, uint32_t miny, uint32_t maxx, uint32_t maxy);

void grColorCombine(
               int32_t function, int32_t factor,
               int32_t local, int32_t other,
               int32_t invert );

void grColorMask(bool rgb, bool a);

void grCullMode( int32_t mode );

void grConstantColorValue( uint32_t value );

void grDepthBiasLevel( int32_t level );

void grDepthBufferFunction(GLenum func);

#define grDepthBufferMode(mode) \
{ \
   if (mode == GR_DEPTHBUFFER_DISABLE) \
      glDisable(GL_DEPTH_TEST); \
   else \
      glEnable(GL_DEPTH_TEST); \
}

void grDepthMask(bool mask);

void grFogMode( int32_t mode, uint32_t fogcolor );

void grLoadGammaTable( uint32_t nentries, uint32_t *red, uint32_t *green, uint32_t *blue);

void grDepthRange( float n, float f );

void grStippleMode( int32_t mode );

void grStipplePattern( uint32_t mode );

void grViewport( int32_t x, int32_t y, int32_t width, int32_t height );

/*
** texture mapping control functions
*/
 uint32_t  
grTexCalcMemRequired(
                    int32_t lodmax,
                     int32_t aspect, int32_t fmt);

#define TMU_SIZE (8 * 2048 * 2048)

#define grTexMaxAddress(tmu) ((TMU_SIZE * 2) - 1)

void grTexSource( int32_t tmu,
             uint32_t      startAddress,
             uint32_t      evenOdd,
             GrTexInfo  *info,
             int do_download);

void grTexFilterClampMode(
               int32_t tmu,
               int32_t s_clampmode,
               int32_t t_clampmode,
               int32_t minfilter_mode,
               int32_t magfilter_mode
               );

void grTexCombine(
             int32_t tmu,
             int32_t rgb_function,
             int32_t rgb_factor, 
             int32_t alpha_function,
             int32_t alpha_factor,
             int32_t rgb_invert,
             int32_t alpha_invert
             );

void grTexDetailControl(
                   int32_t tmu,
                   int lod_bias,
                   uint8_t detail_scale,
                   float detail_max
                   );

void grTexMipMapMode( int32_t     tmu, 
                 int32_t mode,
                 int32_t         lodBlend );

/*
** linear frame buffer functions
*/

 int32_t 
grLfbLock( int32_t type, int32_t buffer, int32_t writeMode,
           int32_t origin, int32_t pixelPipeline, 
           GrLfbInfo_t *info );

#define grLfbUnlock(type, buffer) (true)

void grLfbConstantAlpha( uint8_t alpha );

void grLfbConstantDepth( uint32_t depth );

void grLfbWriteColorSwizzle(int32_t swizzleBytes, int32_t swapWords);

void grLfbWriteColorFormat(int32_t colorFormat);

 int32_t 
grLfbWriteRegion( int32_t dst_buffer, 
                  uint32_t dst_x, uint32_t dst_y, 
                  uint32_t src_format, 
                  uint32_t src_width, uint32_t src_height, 
                  int32_t pixelPipeline,
                  int32_t src_stride, void *src_data );

 int32_t 
grLfbReadRegion( int32_t src_buffer,
                 uint32_t src_x, uint32_t src_y,
                 uint32_t src_width, uint32_t src_height,
                 uint32_t dst_stride, void *dst_data );

#define guFogGenerateLinear(nearZ, farZ) \
{ \
   fogStart = (nearZ) / 255.0f; \
   fogEnd = (farZ) / 255.0f; \
}

extern unsigned int BUFFERSWAP;

#define NUM_TMU 2

// Vertex structure
typedef struct
{
  float x, y, z, q;

  uint8_t  b;  // These values are arranged like this so that *(uint32_t*)(VERTEX+?) is
  uint8_t  g;  // ARGB format that glide can use.
  uint8_t  r;
  uint8_t  a;

  float coord[4];

  float f; //fog

  float u[2];
  float v[2];
  float w;
  uint16_t  flags;

  float vec[3]; // normal vector

  float sx, sy, sz;
  float x_w, y_w, z_w, oow;
  float u_w[2];
  float v_w[2];
  uint8_t  not_zclipped;
  uint8_t  screen_translated;
  uint8_t  uv_scaled;
  uint32_t uv_calculated;  // like crc
  uint32_t shade_mod;
  uint32_t color_backup;

  float ou, ov;

  int   number;   // way to identify it
  int   scr_off, z_off; // off the screen?
} VERTEX;

// ZIGGY framebuffer copy extension
// allow to copy the depth or color buffer from back/front to front/back
#define GR_FBCOPY_MODE_DEPTH 0
#define GR_FBCOPY_MODE_COLOR 1
#define GR_FBCOPY_BUFFER_BACK 0
#define GR_FBCOPY_BUFFER_FRONT 1

// COMBINE extension

#define GR_FUNC_MODE_ZERO                 0x00
#define GR_FUNC_MODE_X                    0x01
#define GR_FUNC_MODE_ONE_MINUS_X          0x02
#define GR_FUNC_MODE_NEGATIVE_X           0x03
#define GR_FUNC_MODE_X_MINUS_HALF         0x04

#define GR_CMBX_ZERO                      0x00
#define GR_CMBX_TEXTURE_ALPHA             0x01
#define GR_CMBX_ALOCAL                    0x02
#define GR_CMBX_AOTHER                    0x03
#define GR_CMBX_B                         0x04
#define GR_CMBX_CONSTANT_ALPHA            0x05
#define GR_CMBX_CONSTANT_COLOR            0x06
#define GR_CMBX_DETAIL_FACTOR             0x07
#define GR_CMBX_ITALPHA                   0x08
#define GR_CMBX_ITRGB                     0x09
#define GR_CMBX_LOCAL_TEXTURE_ALPHA       0x0a
#define GR_CMBX_LOCAL_TEXTURE_RGB         0x0b
#define GR_CMBX_LOD_FRAC                  0x0c
#define GR_CMBX_OTHER_TEXTURE_ALPHA       0x0d
#define GR_CMBX_OTHER_TEXTURE_RGB         0x0e
#define GR_CMBX_TEXTURE_RGB               0x0f
#define GR_CMBX_TMU_CALPHA                0x10
#define GR_CMBX_TMU_CCOLOR                0x11


void grColorCombineExt(uint32_t a, uint32_t a_mode,
				  uint32_t b, uint32_t b_mode,
                  uint32_t c, int32_t c_invert,
				  uint32_t d, int32_t d_invert,
				  uint32_t shift, int32_t invert);

void grAlphaCombineExt(uint32_t a, uint32_t a_mode,
				  uint32_t b, uint32_t b_mode,
				  uint32_t c, int32_t c_invert,
				  uint32_t d, int32_t d_invert,
				  uint32_t shift, int32_t invert);

void 
grTexColorCombineExt(int32_t       tmu,
                     uint32_t a, uint32_t a_mode,
                     uint32_t b, uint32_t b_mode,
                     uint32_t c, int32_t c_invert,
                     uint32_t d, int32_t d_invert,
                     uint32_t shift, int32_t invert);

void 
grTexAlphaCombineExt(int32_t       tmu,
                     uint32_t a, uint32_t a_mode,
                     uint32_t b, uint32_t b_mode,
                     uint32_t c, int32_t c_invert,
                     uint32_t d, int32_t d_invert,
                     uint32_t shift, int32_t invert,
                     uint32_t ccolor_value);

void  
grColorCombineExt(uint32_t a, uint32_t a_mode,
                  uint32_t b, uint32_t b_mode,
                  uint32_t c, int32_t c_invert,
                  uint32_t d, int32_t d_invert,
                  uint32_t shift, int32_t invert);

 void 
grAlphaCombineExt(uint32_t a, uint32_t a_mode,
                  uint32_t b, uint32_t b_mode,
                  uint32_t c, int32_t c_invert,
                  uint32_t d, int32_t d_invert,
                  uint32_t shift, int32_t invert);

extern void grChromaRangeExt(uint32_t color0, uint32_t color1, uint32_t mode);
extern void grChromaRangeModeExt(int32_t mode);
extern void grTexChromaRangeExt(int32_t tmu, uint32_t color0, uint32_t color1, int32_t mode);
extern void grTexChromaModeExt(int32_t tmu, int32_t mode);

extern int width, height;
extern float fogStart, fogEnd;

#ifdef __cplusplus
}
#endif

#endif /* __GLIDE_H__ */
