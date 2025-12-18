#ifndef _RDP_STATE_H
#define _RDP_STATE_H

#include <stdint.h>

#include <boolean.h>
#include <retro_inline.h>

#include "m64p_plugin.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAXCMD 0x100000

#ifndef DP_INTERRUPT
#define DP_INTERRUPT    0x20
#endif

typedef struct
{
   /* Next command */
	uint32_t w2;
   uint32_t w3;

	uint32_t cmd_ptr;
	uint32_t cmd_cur;
	uint32_t cmd_data[MAXCMD + 32];
} RDPInfo;

extern RDPInfo __RDP;

/* Useful macros for decoding GBI command's parameters */

#ifndef _SHIFTL
#define _SHIFTL( v, s, w )  (((uint32_t)v & ((0x01 << w) - 1)) << s)
#endif

#ifndef _SHIFTR
#define _SHIFTR( v, s, w )  (((uint32_t)v >> s) & ((0x01 << w) - 1))
#endif

#ifndef SRA
#define SRA(exp, sa)    ((signed)(exp) >> (sa))
#endif

#ifndef SIGN
#define SIGN(i, b)      SRA((i) << (32 - (b)), (32 - (b)))
#endif

#ifndef GET_LOW
#define GET_LOW(x)      (((x) & 0x003E) << 2)
#endif

#ifndef GET_MED
#define GET_MED(x)      (((x) & 0x07C0) >> 3)
#endif

#ifndef GET_HI
#define GET_HI(x)       (((x) >> 8) & 0x00F8)
#endif

/* Update flags */
#define UPDATE_ZBUF_ENABLED   0x00000001
#define UPDATE_TEXTURE        0x00000002  /* Same thing */
#define UPDATE_COMBINE        0x00000002  /* Same thing */

#define UPDATE_CULL_MODE      0x00000004
#define UPDATE_LIGHTS         0x00000010
#define UPDATE_BIASLEVEL      0x00000020
#define UPDATE_ALPHA_COMPARE  0x00000040
#define UPDATE_VIEWPORT       0x00000080
#define UPDATE_MULT_MAT       0x00000100
#define UPDATE_SCISSOR        0x00000200
#define UPDATE_FOG_ENABLED    0x00010000

typedef struct
{
   uint32_t total;
   uint32_t z;
   uint16_t dz;
   int32_t r, g, b, a, l;
} gdp_color;

typedef struct
{
   int32_t xl, yl, xh, yh;
} gdp_rectangle;

typedef struct
{
    int32_t clampdiffs, clampdifft;
    int32_t clampens, clampent;
    int32_t masksclamped, masktclamped;
    int32_t notlutswitch, tlutswitch;
} gdp_faketile;

typedef struct
{
    int32_t format;        /* Controls the output format of the rasterized image:
                              
                              000 - RGBA
                              001 - YUV
                              010 - Color Index
                              011 - Intensity Alpha
                              011 - Intensity
                           */
    int32_t size;          /* Size of an individual pixel in terms of bits:
                              
                              00 - 4 bits
                              01 - 8 bits (Color Index)
                              10 - 16 bits (RGBA)
                              11 - 32 bits (RGBA)
                           */
    int32_t line;          /* size of one row (x axis) in 64 bit words */
    int32_t tmem;          /* location in texture memory (in 64 bit words, max 512 (4MB)) */
    int32_t palette;       /* palette # to use */
    int32_t ct;            /* clamp_t - Enable clamping in the T direction when texturing
                              primitives. */
    int32_t mt;            /* mirror_t - Enable mirroring in the T direction when texturing
                              primitives.*/
    int32_t cs;            /* clamp_s  - Enable clamping in the S direction when texturing
                              primitives.*/
    int32_t ms;            /* mirror_s - Enable mirroring in the T direction when texturing
                              primitives.*/
    int32_t mask_t;        /* mask to wrap around (y axis) */
    int32_t shift_t;       /* level of detail shifting in the t direction (scaling) */
    int32_t mask_s;        /* mask to wrap around (x axis) */
    int32_t shift_s;       /* level of detail shift in the s direction (scaling) */
    int32_t sl;            /* lr_s - Lower right s coordinate (10.5 fixed point format) */
    int32_t tl;            /* lr_t - Lower right t coordinate (10.5 fixed point format) */
    int32_t sh;            /* ul_s - Upper left  s coordinate (10.5 fixed point format) */
    int32_t th;            /* ul_t - Upper left  t coordinate (10.5 fixed point format) */
    gdp_faketile f;
} gdp_tile;

typedef struct
{
    int32_t sub_a_rgb0;     /* c_a0  */
    int32_t sub_b_rgb0;     /* c_b0  */
    int32_t mul_rgb0;       /* c_c0  */
    int32_t add_rgb0;       /* c_d0  */
    int32_t sub_a_a0;       /* c_Aa0 */
    int32_t sub_b_a0;       /* c_Ab0 */
    int32_t mul_a0;         /* c_Ac0 */
    int32_t add_a0;         /* c_Ad0 */

    int32_t sub_a_rgb1;     /* c_a1  */
    int32_t sub_b_rgb1;     /* c_b1  */
    int32_t mul_rgb1;       /* c_c1  */
    int32_t add_rgb1;       /* c_d1  */
    int32_t sub_a_a1;       /* c_Aa1 */
    int32_t sub_b_a1;       /* c_Ab1 */
    int32_t mul_a1;         /* c_Ac1 */
    int32_t add_a1;         /* c_Ad1 */
} gdp_combine_modes;

typedef struct
{
    int stalederivs;
    int dolod;
    int partialreject_1cycle; 
    int partialreject_2cycle;
    int special_bsel0; 
    int special_bsel1;
    int rgb_alpha_dither;
} GDP_MODEDERIVS;

typedef struct
{
#if 0
   int atomic_primitive_enable;  /* Force primitive to be written to the
                                    framebuffer before processing next primitive.
                                 */
#endif
   int cycle_type;               /* Pipeline rasterization mode:
                                    00 - 1 Cycle
                                    01 - 2 Cycle
                                    10 - Copy
                                    11 - Fill
                                 */
   int persp_tex_en;             /* Enable perspective correction on textures. */
   int detail_tex_en;            /* Enable detail texture. */
   int sharpen_tex_en;           /* Enable sharpen texture. */
   int tex_lod_en;               /* Enable texture level of detail (mipmapping). */
   int en_tlut;                  /* Enable texture lookup table. This is useful when
                                    textures are color mapped but the desired output
                                    to the frame buffer is RGB. */
   int tlut_type;                /* Type of texels (texture color values) in TLUT table:

                                    0 - 16 bit RGBA (0bRRRRRGGGGGBBBBBA)
                                    1 - 16 bit IA   (0xIIAA)
                                 */
   int sample_type;              /* Type of texture sampling:

                                    0 - 1x1 (point sample)
                                    1 - 2x2
                                 */
   int mid_texel;                /* Enable 2x2 half-texel sampling for texture filter. */
   int bi_lerp0;                 /* Enables bilinear interpolation for cycle 0 when 1 Cycle
                                    or 2 Cycle mode is enabled. */
   int bi_lerp1;                 /* Enables bilinear interpolation for cycle 1 when 1 Cycle
                                    or 2 Cycle mode is enabled. */
   int convert_one;              /* Color convert the texel outputted by the texture filter
                                    on cycle 0. */
   int key_en;                   /* Enable chroma keying. */
   int rgb_dither_sel;           /* Type of dithering is done to RGB values in 1 Cycle
                                    or 2 Cycle modes:

                                    00 - Magic square matrix
                                    01 - Bayer matrix
                                    10 - Noise
                                    11 - No dither
                                  */
   int alpha_dither_sel;         /* Type of dithering done to alpha values in 1 Cycle
                                    or 2 Cycle modes:

                                    00 - Pattern
                                    01 - ~Pattern
                                    10 - Noise
                                    11 - No dither
                                 */
   int blend_m1a_0;              /* Multiply blend 1a input in cycle 0. */
   int blend_m1a_1;              /* Multiply blend 1a input in cycle 1. */
   int blend_m1b_0;              /* Multiply blend 1b input in cycle 0. */
   int blend_m1b_1;              /* Multiply blend 1b input in cycle 1. */
   int blend_m2a_0;              /* Multiply blend 2a input in cycle 0. */
   int blend_m2a_1;              /* Multiply blend 2a input in cycle 1. */
   int blend_m2b_0;              /* Multiply blend 2b input in cycle 0. */
   int blend_m2b_1;              /* Multiply blend 2b input in cycle 1. */
   int force_blend;              /* Enable force blend. */
   int alpha_cvg_select;         /* Enable use of coverage bits in alpha calculation. */
   int cvg_times_alpha;          /* Enable multiplying coverage bits by alpha value
                                    for final pixel alpha:

                                    0 - Alpha = CVG
                                    1 - Alpha = CVG * A
                                 */
   int z_mode;                   /* Mode select for Z buffer:

                                    00 - Opaque
                                    01 - Interpenetrating
                                    10 - Transparent
                                    11 - Decal
                                 */
   int cvg_dest;                 /* Mode select for handling coverage values:

                                    00 - Clamp
                                    01 - Wrap
                                    10 - Force to full coverage
                                    11 - Don't write back
                                 */
   int color_on_cvg;             /* Only update color on coverage overflow. Useful
                                    for transparent surfaces. */
   int image_read_en;            /* Enable coverage read/modify/write access to
                                    frame buffer. */
   int z_update_en;              /* Enable writing new Z value if color write is
                                    enabled. */
   int z_compare_en;             /* Enable conditional color write based on depth
                                    comparison. */
   int antialias_en;             /* Enable anti-aliasing based on coverage bits if 
                                    force blend is not enabled. */
   int z_source_sel;             /* Select the source of the Z value:

                                    0 - Pixel Z
                                    1 - Primitive Z
                                 */
   int dither_alpha_en;          /* Select source for alpha compare:

                                    0 - Random noise
                                    1 - Blend alpha
                                 */
   int alpha_compare_en;         /* Enable conditional color write based on alpha compare. */
   GDP_MODEDERIVS f;
} gdp_other_modes;

struct gdp_global
{
   uint32_t flags;

   int32_t ti_format;         /* format: ARGB, IA, ... */
   int32_t ti_size;           /* size: 4, 8, 16, or 32-bit */
   int32_t ti_width;          /* used in rdp_settextureimage */
   uint32_t ti_address;     /* address in RDRAM to load the texture from */

   int32_t primitive_lod_min;
   int32_t primitive_lod_frac;
   int32_t lod_frac;
   gdp_color texel0_color;
   gdp_color texel1_color;
   gdp_color combined_color;
   gdp_color prim_color;
   gdp_color fill_color;
   gdp_color blend_color;
   gdp_color fog_color;
   gdp_color env_color;
   gdp_color key_width;
   gdp_color key_scale;
   gdp_color key_center;
   int32_t k0, k1, k2, k3, k4, k5;
   gdp_tile tile[8];
   gdp_combine_modes combine;
   gdp_other_modes other_modes;
   gdp_rectangle __clip;
   int scfield;
   int sckeepodd;
   uint8_t tmem[0x1000];
   uint32_t zb_address;
   int32_t fb_format;
   int32_t fb_size;
   int32_t fb_width;
   uint32_t fb_address;
};

static INLINE void gdp_calculate_clamp_diffs(struct gdp_global *g_gdp, uint32_t i)
{
   g_gdp->tile[i].f.clampdiffs = ((g_gdp->tile[i].sh >> 2) - (g_gdp->tile[i].sl >> 2)) & 0x3ff;
   g_gdp->tile[i].f.clampdifft = ((g_gdp->tile[i].th >> 2) - (g_gdp->tile[i].tl >> 2)) & 0x3ff;
}

void gdp_set_prim_color(uint32_t w0, uint32_t w1);

void gdp_set_prim_depth(uint32_t w0, uint32_t w1);

void gdp_set_env_color(uint32_t w0, uint32_t w1);

void gdp_set_fill_color(uint32_t w0, uint32_t w1);

void gdp_set_fog_color(uint32_t w0, uint32_t w1);

void gdp_set_blend_color(uint32_t w0, uint32_t w1);

void gdp_set_convert(uint32_t w0, uint32_t w1);

void gdp_set_key_r(uint32_t w0, uint32_t w1);

void gdp_set_key_gb(uint32_t w0, uint32_t w1);

int32_t gdp_set_tile(uint32_t w0, uint32_t w1);

int32_t gdp_set_tile_size_wrap(uint32_t w0, uint32_t w1);

void gdp_set_tile_size(uint32_t w0, uint32_t w1);

void gdp_set_combine(uint32_t w0, uint32_t w1);

void gdp_set_texture_image(uint32_t w0, uint32_t w1);

void gdp_set_scissor(uint32_t w0, uint32_t w1);

void gdp_set_other_modes(uint32_t w0, uint32_t w1);

void gdp_full_sync(uint32_t w0, uint32_t w1);

void gdp_pipe_sync(uint32_t w0, uint32_t w1);

void gdp_tile_sync(uint32_t w0, uint32_t w1);

void gdp_load_sync(uint32_t w0, uint32_t w1);

void gdp_no_op(uint32_t w0, uint32_t w1);

void gdp_invalid(uint32_t w0, uint32_t w1);

void gdp_set_mask_image(uint32_t w0, uint32_t w1);

void gdp_set_color_image(uint32_t w0, uint32_t w1);

static INLINE uint32_t rdp_read_data(uint32_t address)
{
   if ((*(uint32_t*)gfx_info.DPC_STATUS_REG) & 0x1) /* XBUS_DMEM_DMA enabled */
      return ((uint32_t*)gfx_info.DMEM)[(address & 0xfff) >> 2];
   return ((uint32_t*)gfx_info.RDRAM)[address >> 2];
}

bool RDP_Half1(uint32_t _c);

extern struct gdp_global g_gdp;

#ifdef __cplusplus
}
#endif

#endif
