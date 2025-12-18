#include <boolean.h>

#include "RDP_state.h"
#include "../RSP/RSP_state.h"

#include "../GBI.h"
#include "../plugin/plugin.h"

RDPInfo		      __RDP;
struct gdp_global g_gdp;

void gdp_set_texture_image(uint32_t w0, uint32_t w1)
{
   g_gdp.ti_format  = (w0 & 0x00E00000) >> (53 - 32);
   g_gdp.ti_size    = (w0 & 0x00180000) >> (51 - 32);
   g_gdp.ti_width   = (w0 & 0x000003FF) >> (32 - 32);
   g_gdp.ti_address = (w1 & 0x03FFFFFF) >> ( 0 -  0);
   /* g_gdp.ti_address &= 0x00FFFFFF; // physical memory limit, enforced later */
   ++g_gdp.ti_width;

   g_gdp.flags |= UPDATE_TEXTURE;
}

void gdp_set_prim_color(uint32_t w0, uint32_t w1)
{
   g_gdp.prim_color.total   =  w1;
   g_gdp.prim_color.r       = (w1 & 0xFF000000) >> 24;
   g_gdp.prim_color.g       = (w1 & 0x00FF0000) >> 16;
   g_gdp.prim_color.b       = (w1 & 0x0000FF00) >>  8;
   g_gdp.prim_color.a       = (w1 & 0x000000FF) >>  0;

   /* Level of Detail fraction for primitive, used primarily
    * in multi-tile operations for rectangle primitives. */
   g_gdp.primitive_lod_frac = (w0 & 0x000000FF) >> (32-32); 

   /* Minimum clamp for LOD fraction when in detail or sharpen
    * texture modes, fixed point 0.5. */
   g_gdp.primitive_lod_min  = (w0 & 0x00001F00) >> (40-32);
   
   g_gdp.flags |= UPDATE_COMBINE;
}

void gdp_set_env_color(uint32_t w0, uint32_t w1)
{
   g_gdp.env_color.total =  w1;
   g_gdp.env_color.r       = (w1 & 0xFF000000) >> 24;
   g_gdp.env_color.g       = (w1 & 0x00FF0000) >> 16;
   g_gdp.env_color.b       = (w1 & 0x0000FF00) >>  8;
   g_gdp.env_color.a       = (w1 & 0x000000FF) >>  0;

   g_gdp.flags |= UPDATE_COMBINE;
}

void gdp_set_prim_depth(uint32_t w0, uint32_t w1)
{
   g_gdp.prim_color.z     = (w1 & 0xFFFF0000) >> 16;
   g_gdp.prim_color.dz    = (w1 & 0x0000FFFF) >>  0;
}

void gdp_set_fog_color(uint32_t w0, uint32_t w1)
{
   g_gdp.fog_color.total =  w1;
   g_gdp.fog_color.r       = (w1 & 0xFF000000) >> 24;
   g_gdp.fog_color.g       = (w1 & 0x00FF0000) >> 16;
   g_gdp.fog_color.b       = (w1 & 0x0000FF00) >>  8;
   g_gdp.fog_color.a       = (w1 & 0x000000FF) >>  0;

   g_gdp.flags |= UPDATE_COMBINE;
}

void gdp_set_blend_color(uint32_t w0, uint32_t w1)
{
   g_gdp.blend_color.total = w1;
   g_gdp.blend_color.r       = (w1 & 0xFF000000) >> 24;
   g_gdp.blend_color.g       = (w1 & 0x00FF0000) >> 16;
   g_gdp.blend_color.b       = (w1 & 0x0000FF00) >>  8;
   g_gdp.blend_color.a       = (w1 & 0x000000FF) >>  0;

   g_gdp.flags |= UPDATE_COMBINE;
}

void gdp_set_fill_color(uint32_t w0, uint32_t w1)
{
   g_gdp.fill_color.total = w1;
   g_gdp.fill_color.r       = (w1 & 0xFF000000) >> 24;
   g_gdp.fill_color.g       = (w1 & 0x00FF0000) >> 16;
   g_gdp.fill_color.b       = (w1 & 0x0000FF00) >>  8;
   g_gdp.fill_color.a       = (w1 & 0x000000FF) >>  0;
   g_gdp.fill_color.z   = _SHIFTR( w1,  2, 14 );
   g_gdp.fill_color.dz  = _SHIFTR( w1,  0,  2 );

   g_gdp.flags |= UPDATE_ALPHA_COMPARE | UPDATE_COMBINE;
}

/* This command updates the coefficients for converting YUV
 * pixels to RGB. */

void gdp_set_convert(uint32_t w0, uint32_t w1)
{
   const uint64_t fifo_word = ((uint64_t)w0 << 32) | ((uint64_t)w1 <<  0);

   /* K0 term of YUV-RGB conversion matrix. */
   g_gdp.k0 = (fifo_word >> 45) & 0x1FF;
   /* K1 term of YUV-RGB conversion matrix. */
   g_gdp.k1 = (fifo_word >> 36) & 0x1FF;
   /* K2 term of YUV-RGB conversion matrix. */
   g_gdp.k2 = (fifo_word >> 27) & 0x1FF;
   /* K3 term of YUV-RGB conversion matrix. */
   g_gdp.k3 = (fifo_word >> 18) & 0x1FF;
   /* K4 term of YUV-RGB conversion matrix. */
   g_gdp.k4 = (fifo_word >>  9) & 0x1FF;
   /* K5 term of YUV-RGB conversion matrix. */
   g_gdp.k5 = (fifo_word >>  0) & 0x1FF;

/*
 * All the graphics plugins emulating the RDP need to process k0, k1, k2, k3
 * as 9-bit sign-extended integers.  Especially with angrylion's plugin, we
 * can save on constantly sign-extending k0-k3 at the ninth bit by doing it
 * right now, as only in this function do those values ever get updated.
 */
   g_gdp.k0 = SIGN(g_gdp.k0, 9);
   g_gdp.k1 = SIGN(g_gdp.k1, 9);
   g_gdp.k2 = SIGN(g_gdp.k2, 9);
   g_gdp.k3 = SIGN(g_gdp.k3, 9);
}

void gdp_set_key_gb(uint32_t w0, uint32_t w1)
{
   g_gdp.key_scale.total  = (g_gdp.key_scale.total & 0xFF0000FF) | (((w1 >> 16) & 0xFF) << 16)   | (((w1 & 0xFF)) << 8);
   g_gdp.key_center.total = (g_gdp.key_center.total & 0xFF0000FF) | (((w1 >> 24) & 0xFF) << 16) | (((w1 >> 8) & 0xFF) << 8);

   g_gdp.key_width.g      = (w0 & 0x00FFF000) >> 12;
   g_gdp.key_width.b      = (w0 & 0x00000FFF) >>  0;
   g_gdp.key_center.g     = (w1 & 0xFF000000) >> 24;
   g_gdp.key_scale.g      = (w1 & 0x00FF0000) >> 16;
   g_gdp.key_center.b     = (w1 & 0x0000FF00) >>  8;
   g_gdp.key_scale.b      = (w1 & 0x000000FF) >>  0;
}

/* This command sets the coefficients used for Red keying. */
void gdp_set_key_r(uint32_t w0, uint32_t w1)
{
   g_gdp.key_scale.total  = (g_gdp.key_scale.total & 0x00FFFFFF)  | ((w1 & 0xFF) << 24);
   g_gdp.key_center.total = (g_gdp.key_center.total & 0x00FFFFFF) | (((w1 >> 8) & 0xFF) << 24);

   /* Defines color or intensity at which key is active, 0-255. */
   g_gdp.key_center.r     = (w1 & 0x0000FF00) >>  8;
   /* (Size of half the key window including the soft edge) * scale. If width > 1.0,
    * then keying is disabled for that channel. */
   g_gdp.key_width.r      = (w1 & 0x0FFF0000) >> 16;
   /* (1.0 / (size of soft edge). For hard ege keying, set scale to 255. */
   g_gdp.key_scale.r      = (w1 & 0x000000FF) >>  0;
}

int32_t gdp_set_tile(uint32_t w0, uint32_t w1)
{
   int32_t tilenum             = (w1 & 0x07000000) >> 24;

   /* Image data format, 0 = RGBA, 1 = YUV, 2 = Color Index, 3 = IA, 4 = I */
   g_gdp.tile[tilenum].format  = (w0 & 0x00E00000) >> (53 - 32);
   /* Size of pixel/texel color element, 0 = 4b, 1 = 8b, 2 = 16b, 3 = 32b, 4 = Other */
   g_gdp.tile[tilenum].size    = (w0 & 0x00180000) >> (51 - 32);
   /* Size of tile line in 64bit words, max of 4KB. */
   g_gdp.tile[tilenum].line    = (w0 & 0x0003FE00) >> (41 - 32);
   /* Starting TMEM address for this tile in words (64bits), 4KB range. */
   g_gdp.tile[tilenum].tmem    = (w0 & 0x000001FF) >> (32 - 32);
   /* Palette number for 4b Color Indexed texels. This number is
    * used as the MS 4b of an 8b index. */
   g_gdp.tile[tilenum].palette = (w1 & 0x00F00000) >> (20 -  0);
   /* Clamp enable for T direction. */
   g_gdp.tile[tilenum].ct      = (w1 & 0x00080000) >> (19 -  0);
   /* Mirror enable for T direction. */
   g_gdp.tile[tilenum].mt      = (w1 & 0x00040000) >> (18 -  0);
   /* Mask for wrapping/mirroring in T direction. If this field
    * is zero then clamp, otherwise pass (mask) LSBs of T address. */
   g_gdp.tile[tilenum].mask_t  = (w1 & 0x0003C000) >> (14 -  0);
   /* Level of Detail shift for T addresses. */
   g_gdp.tile[tilenum].shift_t = (w1 & 0x00003C00) >> (10 -  0);
   /* Clamp enable bit for S direction. */
   g_gdp.tile[tilenum].cs      = (w1 & 0x00000200) >> ( 9 -  0);
   /* Mirror enable bit for S direction. */
   g_gdp.tile[tilenum].ms      = (w1 & 0x00000100) >> ( 8 -  0);
   /* Mask for wrapping/mirroring in S direction. If this field is
    * zero then clamp, otherwise pass (mask) LSBs of S address. */
   g_gdp.tile[tilenum].mask_s  = (w1 & 0x000000F0) >> ( 4 -  0);
   /* Level of Detail shift for S addresses. */
   g_gdp.tile[tilenum].shift_s = (w1 & 0x0000000F) >> ( 0 -  0);

   g_gdp.flags |= UPDATE_TEXTURE;

   return tilenum;
}

int32_t gdp_set_tile_size_wrap(uint32_t w0, uint32_t w1)
{
   int32_t tilenum = (w1 & 0x07000000) >> (24 -  0);

   /* Low S coordinate of tile in image. */
   g_gdp.tile[tilenum].sl      = (w0 & 0x00FFF000) >> (44 - 32);
   /* Low T coordinate of tile in image. */
   g_gdp.tile[tilenum].tl      = (w0 & 0x00000FFF) >> (32 - 32);
   /* High S coordinate of tile in image. */
   g_gdp.tile[tilenum].sh      = (w1 & 0x00FFF000) >> (12 -  0);
   /* High T coordinate of tile in image. */
   g_gdp.tile[tilenum].th      = (w1 & 0x00000FFF) >> ( 0 -  0);

   gdp_calculate_clamp_diffs(&g_gdp, tilenum);

   g_gdp.flags |= UPDATE_TEXTURE;

   return tilenum;
}

void gdp_set_tile_size(uint32_t w0, uint32_t w1)
{
   gdp_set_tile_size_wrap(w0, w1);
}

/* The Color Combiner implements the 
 * following equation on each color:
 * (A - B) * C + D
 *
 * RGB and Alpha channels have separate
 * mux selects. In addition, there are separate
 * mux selects for cycle 0 and cycle 1. */
void gdp_set_combine(uint32_t w0, uint32_t w1)
{
   /* subtract A input, RGB components, cycle 0. */
   g_gdp.combine.sub_a_rgb0 = (w0 & 0x00F00000) >> 20;
   /* multiply input,   RGB components, cycle 0. */
   g_gdp.combine.mul_rgb0   = (w0 & 0x000F8000) >> 15;
   /* subtract A input, alpha component, cycle 0. */
   g_gdp.combine.sub_a_a0   = (w0 & 0x00007000) >> 12;
   /* multiply input,   alpha component, cycle 0. */
   g_gdp.combine.mul_a0     = (w0 & 0x00000E00) >>  9;
   /* subtract A input, RGB components,  cycle 1. */
   g_gdp.combine.sub_a_rgb1 = (w0 & 0x000001E0) >>  5;
   /* multiply input,   RGB components, cycle 1. */
   g_gdp.combine.mul_rgb1   = (w0 & 0x0000001F) >>  0;
   /* subtract B input, RGB components, cycle 0. */
   g_gdp.combine.sub_b_rgb0 = (w1 & 0xF0000000) >> 28;
   /* subtract B input, RGB components, cycle 1. */
   g_gdp.combine.sub_b_rgb1 = (w1 & 0x0F000000) >> 24;
   /* subtract A input, alpha component, cycle 1. */
   g_gdp.combine.sub_a_a1   = (w1 & 0x00E00000) >> 21;
   g_gdp.combine.mul_a1     = (w1 & 0x001C0000) >> 18;
   g_gdp.combine.add_rgb0   = (w1 & 0x00038000) >> 15;
   g_gdp.combine.sub_b_a0   = (w1 & 0x00007000) >> 12;
   g_gdp.combine.add_a0     = (w1 & 0x00000E00) >>  9;
   g_gdp.combine.add_rgb1   = (w1 & 0x000001C0) >>  6;
   g_gdp.combine.sub_b_a1   = (w1 & 0x00000038) >>  3;
   g_gdp.combine.add_a1     = (w1 & 0x00000007) >>  0;

   g_gdp.flags |= UPDATE_COMBINE;
}

void gdp_set_other_modes(uint32_t w0, uint32_t w1)
{
   /* K:  atomic_prim              = (w0 & 0x0080000000000000) >> 55; */
   /* j:  reserved for future use -- (w0 & 0x0040000000000000) >> 54 */

   /* Display pipeline cycle control mode: 0 = 1 cycle, 1 = 2 cycle,
    * 2 = Copy, 3 = Fill */
   g_gdp.other_modes.cycle_type       = (w0 & 0x00300000) >> (52 - 32);

   /* Enable perspective correction on texture. */
   g_gdp.other_modes.persp_tex_en     = !!(w0 & 0x00080000); /* 51 */

   /* Enable detail texture. */
   g_gdp.other_modes.detail_tex_en    = !!(w0 & 0x00040000); /* 50 */

   /* Enable sharpened texture. */
   g_gdp.other_modes.sharpen_tex_en   = !!(w0 & 0x00020000); /* 49 */

   /* Enable texture Level of Detail (LOD). */
   g_gdp.other_modes.tex_lod_en       = !!(w0 & 0x00010000); /* 48 */

   /* Enable lookup of texel values from TLUT. Meaningful if texture
    * type is index, tile is in low TMEM, TLUT is in high TMEM, and
    * color image is RGB. */
   g_gdp.other_modes.en_tlut          = !!(w0 & 0x00008000); /* 47 */

   /* Type of texels in table, 0 = 16b RGBA (5/5/5/1), I = IA (8/8) */
   g_gdp.other_modes.tlut_type        = !!(w0 & 0x00004000); /* 46 */

   /* Determines how textures are sampled: 0 = 1x1 (Point Sample), 1 = 2x2.
    * Note that copy (point sample 4 horizontally adjacent texels) mode
    * is indicated by cycle_type. */
   g_gdp.other_modes.sample_type      = !!(w0 & 0x00002000); /* 45 */

   /* Indicates Texture Filter should do a 2x2x half texel interpolation,
    * primarily used for MPEG motion compensation processing. */
   g_gdp.other_modes.mid_texel        = !!(w0 & 0x00001000); /* 44 */

   /* color convert operation in Texture Filter. Used in cycle 0. */
   g_gdp.other_modes.bi_lerp0         = !!(w0 & 0x00000800); /* 43 */

   /* Color convert operation in Texture Filter. Used in cycle 1. */
   g_gdp.other_modes.bi_lerp1         = !!(w0 & 0x00000400); /* 42 */

   /* Color convert texel that was the output of the texture filter
    * on cycle0, used to qualify bi_lerp_1. */
   g_gdp.other_modes.convert_one      = !!(w0 & 0x00000200); /* 41 */

   /* Enable chroma keying. */
   g_gdp.other_modes.key_en           = !!(w0 & 0x00000100); /* 40 */

   /* 0 = magic square matrix (preferred if filtered)
    * 1 = "standard" bayer matrix (preferred if not filtered)
    * 2 = noise (as before)
    * 3 = no dither.
    */
   g_gdp.other_modes.rgb_dither_sel   = (w0 & 0x000000C0) >> (38 - 32);

   /* 0 = pattern
    * 1 = ~pattern
    * 2 = noise
    * 3 = noise dither
    */
   g_gdp.other_modes.alpha_dither_sel = (w0 & 0x00000030) >> (36 - 32);

   /* reserved for future, def:15 -- (w1 & 0x0000000F00000000) >> 32 */
   g_gdp.other_modes.blend_m1a_0      = (w1 & 0xC0000000) >> (30 -  0);
   g_gdp.other_modes.blend_m1a_1      = (w1 & 0x30000000) >> (28 -  0);
   g_gdp.other_modes.blend_m1b_0      = (w1 & 0x0C000000) >> (26 -  0);
   g_gdp.other_modes.blend_m1b_1      = (w1 & 0x03000000) >> (24 -  0);
   g_gdp.other_modes.blend_m2a_0      = (w1 & 0x00C00000) >> (22 -  0);
   g_gdp.other_modes.blend_m2a_1      = (w1 & 0x00300000) >> (20 -  0);
   g_gdp.other_modes.blend_m2b_0      = (w1 & 0x000C0000) >> (18 -  0);
   g_gdp.other_modes.blend_m2b_1      = (w1 & 0x00030000) >> (16 -  0);
   /* N:  reserved for future use -- (w1 & 0x0000000000008000) >> 15 */

   /* Force blend enable */
   g_gdp.other_modes.force_blend      = !!(w1 & 0x00004000); /* 14 */

   /* Use cvg (or cvg * alpha) for pixel alpha. */
   g_gdp.other_modes.alpha_cvg_select = !!(w1 & 0x00002000); /* 13 */

   /* Use cvg times alpha for pixel alpha and coverage. */
   g_gdp.other_modes.cvg_times_alpha  = !!(w1 & 0x00001000); /* 12 */

   /* 0 : opaque, 1 : interpenetrating, 2 : transparent, 3 : decal */
   g_gdp.other_modes.z_mode           = (w1 & 0x00000C00) >> (10 -  0);

   /* 0 : clamp (normal), 1 : wrap (was assume full cvg),
    * 2 : zap (force to full cvg), 3: save (don't overwrite memory cvg). */
   g_gdp.other_modes.cvg_dest         = (w1 & 0x00000300) >> ( 8 -  0);

   /* Only update color on coverage overflow (transparent surfaces). */
   g_gdp.other_modes.color_on_cvg     = !!(w1 & 0x00000080); /*  7 */

   /* Enable color/cvg read/modify/write memory access. */
   g_gdp.other_modes.image_read_en    = !!(w1 & 0x00000040); /*  6 */

   /* Enable writing of Z if color write enabled. */
   g_gdp.other_modes.z_update_en      = !!(w1 & 0x00000020); /*  5 */

   /* Conditional color write enable on depth comparison. */
   g_gdp.other_modes.z_compare_en     = !!(w1 & 0x00000010); /*  4 */

   /* If not force blend, allow blend enable - use cvg bits */
   g_gdp.other_modes.antialias_en     = !!(w1 & 0x00000008); /*  3 */

   /* Use random noise in alpha compare, otherwise use blend alpha
    * in alpha compare. */
   g_gdp.other_modes.dither_alpha_en  = !!(w1 & 0x00000002); /*  1 */

   /* Conditional color write on alpha compare. */
   g_gdp.other_modes.alpha_compare_en = !!(w1 & 0x00000001); /*  0 */
}

void gdp_set_scissor(uint32_t w0, uint32_t w1)
{
   g_gdp.__clip.xh   = (w0 & 0x00FFF000) >> (44 - 32);
   g_gdp.__clip.yh   = (w0 & 0x00000FFF) >> (32 - 32);
   g_gdp.scfield     = (w1 & 0x02000000) >> (25 -  0);
   g_gdp.sckeepodd   = (w1 & 0x01000000) >> (24 -  0);
   g_gdp.__clip.xl   = (w1 & 0x00FFF000) >> (12 -  0);
   g_gdp.__clip.yl   = (w1 & 0x00000FFF) >> ( 0 -  0);

   g_gdp.flags |= UPDATE_SCISSOR;
}

/* This command stalls the RDP until the last DRAM buffer is read
 * or written from any preceding primitive. It is typically only needed
 * if the memory data is to be reused, like switching display buffers,
 * or writing a color_image to be used as a texture_image, or for
 * consistent read/write access to an RDP write/read image from the CPU. */
void gdp_full_sync(uint32_t w0, uint32_t w1)
{
   *gfx_info.MI_INTR_REG |= DP_INTERRUPT;
   gfx_info.CheckInterrupts();
}

void gdp_pipe_sync(uint32_t w0, uint32_t w1)
{
}

void gdp_tile_sync(uint32_t w0, uint32_t w1)
{
}

void gdp_load_sync(uint32_t w0, uint32_t w1)
{
}

void gdp_no_op(uint32_t w0, uint32_t w1)
{
}

void gdp_set_mask_image(uint32_t w0, uint32_t w1)
{
   g_gdp.zb_address = w1 & 0x03FFFFFF;
   /* g_gdp.zb_address &= 0x00FFFFFF; */
}

static char invalid_command[] = "00\nDP reserved command.";

void gdp_invalid(uint32_t w0, uint32_t w1)
{
   const unsigned int command = (w0 & 0x3F000000) >> 24;

   invalid_command[0] = '0' | command >> 3;
   invalid_command[1] = '0' | (command & 07);

   //DisplayError(invalid_command);
}

void gdp_set_color_image(uint32_t w0, uint32_t w1)
{
   g_gdp.fb_format  = (w0 & 0x00E00000) >> (53 - 32);
   g_gdp.fb_size    = (w0 & 0x00180000) >> (51 - 32);
   g_gdp.fb_width   = (w0 & 0x000003FF) >> (32 - 32);
   g_gdp.fb_address = (w1 & 0x03FFFFFF) >> ( 0 -  0);
   ++g_gdp.fb_width;
   /* g_gdp.fb_address &= 0x00FFFFFF; */
}

bool RDP_Half1(uint32_t _c)
{
   uint32_t cmd = _SHIFTR( _c, 24, 8 );
   if (cmd >= G_TRI_FILL && cmd <= G_TRI_SHADE_TXTR_ZBUFF) /* Triangle command */
   {
      __RDP.cmd_ptr = 0;
      __RDP.cmd_cur = 0;

      do
      {
         __RDP.cmd_data[__RDP.cmd_ptr++] = __RSP.w1;
         RSP_CheckDLCounter();

         /* Load the next command and its input */
         __RSP.w0 = *(uint32_t*)&gfx_info.RDRAM[__RSP.PC[__RSP.PCi]];
         __RSP.w1 = *(uint32_t*)&gfx_info.RDRAM[__RSP.PC[__RSP.PCi] + 4];
         __RSP.cmd = _SHIFTR(__RSP.w0, 24, 8);
         /* Go to the next instruction */
         __RSP.PC[__RSP.PCi] += 8;
      }while ((__RSP.w0 >> 24) != 0xb3);

      __RDP.cmd_data[__RDP.cmd_ptr++] = __RSP.w1;
      __RSP.cmd                       = (__RDP.cmd_data[__RDP.cmd_cur] >> 24) & 0x3f;
      __RSP.w0                        = __RDP.cmd_data[__RDP.cmd_cur+0];
      __RSP.w1                        = __RDP.cmd_data[__RDP.cmd_cur+1];

      return true;
   }

   return false;
}
