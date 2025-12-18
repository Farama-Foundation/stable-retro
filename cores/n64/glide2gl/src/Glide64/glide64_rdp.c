/*
* Glide64 - Glide video plugin for Nintendo 64 emulators.
* Copyright (c) 2002  Dave2001
* Copyright (c) 2003-2009  Sergey 'Gonetz' Lipski
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or * any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

//****************************************************************
//
// Glide64 - Glide Plugin for Nintendo 64 emulators
// Project started on December 29th, 2001
//
// Authors:
// Dave2001, original author, founded the project in 2001, left it in 2002
// Gugaman, joined the project in 2002, left it in 2002
// Sergey 'Gonetz' Lipski, joined the project in 2002, main author since fall of 2002
// Hiroshi 'KoolSmoky' Morii, joined the project in 2007
//
//****************************************************************
//
// To modify Glide64:
// * Write your name and (optional)email, commented by your work, so I know who did it, and so that you can find which parts you modified when it comes time to send it to me.
// * Do NOT send me the whole project or file that you modified.  Take out your modified code sections, and tell me where to put them.  If people sent the whole thing, I would have many different versions, but no idea how to combine them all.
//
//****************************************************************

#include <math.h>
#include <encodings/crc32.h>
#include "Gfx_1.3.h"
#include "3dmath.h"
#include "Util.h"
#include "Combine.h"
#include "TexCache.h"
#include "Framebuffer_glide64.h"
#include "CRC.h"
#include "Glide64_UCode.h"
#include "GlideExtensions.h"
#include "rdp.h"
#include "../../libretro/libretro_private.h"
#include "../../Graphics/GBI.h"
#include "../../Graphics/HLE/Microcode/Fast3D.h"
#include "../../Graphics/RDP/gDP_funcs_C.h"
#include "../../Graphics/RSP/gSP_funcs_C.h"
#include "../../Graphics/RDP/RDP_state.h"
#include "../../Graphics/RDP/gDP_state.h"
#include "../../Graphics/RSP/RSP_state.h"

/* angrylion's macro, helps to cut overflowed values. */
#define SIGN16(x) (int16_t)(x)

/* positional and texel coordinate clipping */
#define CCLIP2(ux,lx,ut,lt,un,ln,uc,lc) \
		if (ux > lx || lx < uc || ux > lc) return; \
		if (ux < uc) { \
			float p = (uc-ux)/(lx-ux); \
			ut = p*(lt-ut)+ut; \
			un = p*(ln-un)+un; \
			ux = uc; \
		} \
		if (lx > lc) { \
			float p = (lc-ux)/(lx-ux); \
			lt = p*(lt-ut)+ut; \
			ln = p*(ln-un)+un; \
			lx = lc; \
		}

int dzdx   = 0;

const char *ACmp[] = { "NONE", "THRESHOLD", "UNKNOWN", "DITHER" };

const char *Mode0[] = { "COMBINED",    "TEXEL0",
            "TEXEL1",     "PRIMITIVE",
            "SHADE",      "ENVIORNMENT",
            "1",        "NOISE",
            "0",        "0",
            "0",        "0",
            "0",        "0",
            "0",        "0" };

const char *Mode1[] = { "COMBINED",    "TEXEL0",
            "TEXEL1",     "PRIMITIVE",
            "SHADE",      "ENVIORNMENT",
            "CENTER",     "K4",
            "0",        "0",
            "0",        "0",
            "0",        "0",
            "0",        "0" };

const char *Mode2[] = { "COMBINED",    "TEXEL0",
            "TEXEL1",     "PRIMITIVE",
            "SHADE",      "ENVIORNMENT",
            "SCALE",      "COMBINED_ALPHA",
            "T0_ALPHA",     "T1_ALPHA",
            "PRIM_ALPHA",   "SHADE_ALPHA",
            "ENV_ALPHA",    "LOD_FRACTION",
            "PRIM_LODFRAC",   "K5",
            "0",        "0",
            "0",        "0",
            "0",        "0",
            "0",        "0",
            "0",        "0",
            "0",        "0",
            "0",        "0",
            "0",        "0" };

const char *Mode3[] = { "COMBINED",    "TEXEL0",
            "TEXEL1",     "PRIMITIVE",
            "SHADE",      "ENVIORNMENT",
            "1",        "0" };

const char *Alpha0[] = { "COMBINED",   "TEXEL0",
            "TEXEL1",     "PRIMITIVE",
            "SHADE",      "ENVIORNMENT",
            "1",        "0" };

#define Alpha1 Alpha0
const char *Alpha2[] = { "LOD_FRACTION", "TEXEL0",
            "TEXEL1",     "PRIMITIVE",
            "SHADE",      "ENVIORNMENT",
            "PRIM_LODFRAC",   "0" };
#define Alpha3 Alpha0

const char *FBLa[] = { "G_BL_CLR_IN", "G_BL_CLR_MEM", "G_BL_CLR_BL", "G_BL_CLR_FOG" };
const char *FBLb[] = { "G_BL_A_IN", "G_BL_A_FOG", "G_BL_A_SHADE", "G_BL_0" };
const char *FBLc[] = { "G_BL_CLR_IN", "G_BL_CLR_MEM", "G_BL_CLR_BL", "G_BL_CLR_FOG"};
const char *FBLd[] = { "G_BL_1MA", "G_BL_A_MEM", "G_BL_1", "G_BL_0" };

const char *str_zs[] = { "G_ZS_PIXEL", "G_ZS_PRIM" };

const char *str_yn[] = { "NO", "YES" };
const char *str_offon[] = { "OFF", "ON" };

const char *str_cull[] = { "DISABLE", "FRONT", "BACK", "BOTH" };

// I=intensity probably
const char *str_format[] = { "RGBA", "YUV", "CI", "IA", "I", "?", "?", "?" };
const char *str_size[]   = { "4bit", "8bit", "16bit", "32bit" };
const char *str_cm[]     = { "WRAP/NO CLAMP", "MIRROR/NO CLAMP", "WRAP/CLAMP", "MIRROR/CLAMP" };
const char *str_lod[]    = { "1", "2", "4", "8", "16", "32", "64", "128", "256", "512", "1024", "2048" };
const char *str_aspect[] = { "1x8", "1x4", "1x2", "1x1", "2x1", "4x1", "8x1" };

const char *str_filter[] = { "Point Sampled", "Average (box)", "Bilinear" };

const char *str_tlut[]   = { "TT_NONE", "TT_UNKNOWN", "TT_RGBA_16", "TT_IA_16" };

const char *str_dither[] = { "Pattern", "~Pattern", "Noise", "None" };

const char *CIStatus[]   = { "ci_main", "ci_zimg", "ci_unknown",  "ci_useless",
                            "ci_old_copy", "ci_copy", "ci_copy_self",
                            "ci_zcopy", "ci_aux", "ci_aux_copy" };

//static variables

uint32_t frame_count;  // frame counter

int ucode_error_report = true;

uint8_t microcode[4096];
uint32_t uc_crc;

/* ** UCODE FUNCTIONS ** */
#include "ucode.h"
#include "glide64_gDP.h"
#include "glide64_gSP.h"
#include "ucode00.h"
#include "ucode01.h"
#include "ucode02.h"
#include "ucode03.h"
#include "ucode04.h"
#include "ucode05.h"
#include "ucode06.h"
#include "ucode07.h"
#include "ucode08.h"
#include "ucode09.h"
#include "ucode_f3dtexa.h"
#include "ucode09rdp.h"
#include "turbo3D.h"
#include "ucode_f3dex2acclaim.h"

static int reset = 0;
int old_ucode = -1;

void (*_gSPVertex)(uint32_t addr, uint32_t n, uint32_t v0);

void rdp_setfuncs(void)
{
   if (settings.hacks & hack_Makers)
   {
      if (log_cb)
         log_cb(RETRO_LOG_INFO, "Applying Mischief Makers function pointer table tweak...\n");
      gfx_instruction[0][191] = uc0_tri1_mischief;
   }
}

void rdp_new(void)
{
   unsigned i;
   unsigned      cpu = 0;
   rdp.vtx1          = (VERTEX*)calloc(256, sizeof(VERTEX));
   rdp.vtx2          = (VERTEX*)calloc(256, sizeof(VERTEX));
   rdp.vtx           = (VERTEX*)calloc(MAX_VTX, sizeof(VERTEX));
   rdp.frame_buffers = (COLOR_IMAGE*)calloc(NUMTEXBUF+2, sizeof(COLOR_IMAGE));

   rdp.vtxbuf        = 0;
   rdp.vtxbuf2       = 0;
   rdp.vtx_buffer    = 0;
   rdp.n_global      = 0;

   for (i = 0; i < MAX_TMU; i++)
   {
      rdp.cache[i] = (CACHE_LUT*)calloc(MAX_CACHE, sizeof(CACHE_LUT));
      rdp.cur_cache[i]   = 0;
   }

   if (perf_get_cpu_features_cb)
      cpu = perf_get_cpu_features_cb();

   _gSPVertex = glide64gSPVertex;

   gSP.objMatrix.A          = 1.0f;
   gSP.objMatrix.B          = 0.0f;
   gSP.objMatrix.C          = 0.0f;
   gSP.objMatrix.D          = 1.0f;
   gSP.objMatrix.X          = 0.0f;
   gSP.objMatrix.Y          = 0.0f;
   gSP.objMatrix.baseScaleX = 1.0f;
   gSP.objMatrix.baseScaleY = 1.0f;

   gSP.matrix.billboard  = 0;
}

void rdp_free(void)
{
   int i;

   if (rdp.vtx1)
      free(rdp.vtx1);
   rdp.vtx1           = NULL;

   if (rdp.vtx2)
      free(rdp.vtx2);
   rdp.vtx2           = NULL;

   for (i = 0; i < MAX_TMU; i++)
   {
      if (rdp.cache[i])
      {
         free(rdp.cache[i]);
         rdp.cache[i] = NULL;
      }
   }

   if (rdp.vtx)
      free(rdp.vtx);
   rdp.vtx           = NULL;

   if (rdp.frame_buffers)
      free(rdp.frame_buffers);
   rdp.frame_buffers = NULL;

}

void rdp_reset(void)
{
   int i;
   reset                = 1;

   if (!rdp.vtx)
      return;

   // set all vertex numbers
   for (i = 0; i < MAX_VTX; i++)
      rdp.vtx[i].number = i;

   gDPSetScissor(
         0,             /* mode */
         0,             /* ulx  */
         0,             /* uly  */
         320,           /* lrx  */
         240            /* lry  */
         );

   rdp.vi_org_reg       = *gfx_info.VI_ORIGIN_REG;
   gSP.viewport.vscale[2]    = 32.0f * 511.0f;
   gSP.viewport.vtrans[2]    = 32.0f * 511.0f;
   rdp.clip_ratio       = 1.0f;
   rdp.lookat[0][0]     = rdp.lookat[1][1] = 1.0f;
   rdp.allow_combine    = 1;
   g_gdp.flags          = UPDATE_SCISSOR | UPDATE_COMBINE | UPDATE_ZBUF_ENABLED | UPDATE_CULL_MODE;
   rdp.fog_mode         = FOG_MODE_ENABLED;
   rdp.maincimg[0].addr = rdp.maincimg[1].addr = rdp.last_drawn_ci_addr = 0x7FFFFFFF;
}

/******************************************************************
Function: ProcessDList
Purpose:  This function is called when there is a Dlist to be
processed. (High level GFX list)
input:    none
output:   none
*******************************************************************/
void DetectFrameBufferUsage(void);
uint32_t fbreads_front = 0;
uint32_t fbreads_back = 0;
int cpu_fb_read_called = false;
int cpu_fb_write_called = false;
int cpu_fb_write = false;
int cpu_fb_ignore = false;
int CI_SET = true;
uint32_t swapped_addr = 0;
int depth_buffer_fog;

extern bool frame_dupe;

void glide64ProcessDList(void)
{
  uint32_t dlist_start, dlist_length, a;

  no_dlist            = false;
  update_screen_count = 0;
  ChangeSize();

  if (reset)
  {
    reset = 0;
    if (settings.autodetect_ucode)
    {
      // Thanks to ZeZu for ucode autodetection!!!
      uint32_t startUcode = *(uint32_t*)(gfx_info.DMEM+0xFD0);
      memcpy (microcode, gfx_info.RDRAM+startUcode, 4096);
      microcheck ();
    }
    else
      memset (microcode, 0, 4096);
  }
  else if ( ((old_ucode == ucode_S2DEX) && (settings.ucode == ucode_F3DEX)) || settings.force_microcheck)
  {
    uint32_t startUcode = *(uint32_t*)(gfx_info.DMEM+0xFD0);
    memcpy (microcode, gfx_info.RDRAM+startUcode, 4096);
    microcheck ();
  }

  if (exception)
    return;

  /* Set states */
  if (settings.swapmode > 0)
    SwapOK                            = true;

  rdp.updatescreen                    = 1;

  gSP.matrix.modelViewi               = 0; /* 0 matrices so far in stack */
  //stack_size can be less then 32! Important for Silicon Vally. Thanks Orkin!
  gSP.matrix.stackSize                = MIN(32, (*(uint32_t*)(gfx_info.DMEM+0x0FE4))>>6);
  if (gSP.matrix.stackSize == 0)
    gSP.matrix.stackSize              = 32;
  rdp.fb_drawn                        = false;
  rdp.fb_drawn_front                  = false;
  g_gdp.flags                         = 0x7FFFFFFF;  /* All but clear cache */
  gSP.geometryMode                    = 0;
  rdp.maincimg[1]                     = rdp.maincimg[0];
  rdp.skip_drawing                    = false;
  rdp.s2dex_tex_loaded                = false;
  rdp.bg_image_height                 = 0xFFFF;
  fbreads_front                       = 0;
  fbreads_back                        = 0;
  gSP.fog.multiplier                  = 0;
  gSP.fog.offset                      = 0;

  if (rdp.vi_org_reg != *gfx_info.VI_ORIGIN_REG)		
     rdp.tlut_mode     = 0; /* is it correct? */

  rdp.scissor_set                     = false;
  gSP.DMAOffsets.tex_offset           = 0;
  gSP.DMAOffsets.tex_count            = 0;
  cpu_fb_write                        = false;
  cpu_fb_read_called                  = false;
  cpu_fb_write_called                 = false;
  cpu_fb_ignore                       = false;
  part_framebuf.d_ul_x                              = 0xffff;
  part_framebuf.d_ul_y                              = 0xffff;
  part_framebuf.d_lr_x                              = 0;
  part_framebuf.d_lr_y                              = 0;
  depth_buffer_fog                    = true;

  //analyze possible frame buffer usage
  if (fb_emulation_enabled)
    DetectFrameBufferUsage();
  if (!(settings.hacks&hack_Lego) || rdp.num_of_ci > 1)
    rdp.last_bg = 0;
  //* End of set states *//

  // Get the start of the display list and the length of it
  dlist_start = *(uint32_t*)(gfx_info.DMEM+0xFF0);
  dlist_length = *(uint32_t*)(gfx_info.DMEM+0xFF4);

  // Do nothing if dlist is empty
  if (dlist_start == 0)
     return;

  if (cpu_fb_write == true)
    DrawPartFrameBufferToScreen();
  if ((settings.hacks&hack_Tonic) && dlist_length < 16)
  {
    gdp_full_sync(__RSP.w0, __RSP.w1);
    FRDP_E("DLIST is too short!\n");
    return;
  }

  // Start executing at the start of the display list
  __RSP.PCi           = 0;
  __RSP.PC[__RSP.PCi] = dlist_start;
  __RSP.count         = -1;
  __RSP.halt          = 0;

  if (settings.ucode == ucode_Turbo3d)
     Turbo3D();
  else
  {
     /* MAIN PROCESSING LOOP */
     do
     {
        /* Get the address of the next command */
        a        = __RSP.PC[__RSP.PCi] & BMASK;

        /* Load the next command and its input */
        __RSP.w0 = ((uint32_t*)gfx_info.RDRAM)[a>>2];   // \ Current command, 64 bit
        __RSP.w1 = ((uint32_t*)gfx_info.RDRAM)[(a>>2)+1]; // /
        /* RDP state's w2 and w3 are filled only when needed, by the function that needs them */

#ifdef LOG_COMMANDS
        // Output the address before the command
        FRDP ("%08lx (c0:%08lx, c1:%08lx): ", a, __RSP.w0, __RSP.w1);
#endif

        // Go to the next instruction
        __RSP.PC[__RSP.PCi] = (a+8) & BMASK;

        // Process this instruction
        gfx_instruction[settings.ucode][__RSP.w0 >> 24](__RSP.w0, __RSP.w1);

        RSP_CheckDLCounter();
     } while (!__RSP.halt);
  }

  if (fb_emulation_enabled)
  {
    rdp.scale_x = rdp.scale_x_bak;
    rdp.scale_y = rdp.scale_y_bak;
  }
  if(settings.hacks & hack_OOT && !frame_dupe)
    copyWhiteToRDRAM(); /* Subscreen delay fix */
  else if (settings.frame_buffer & fb_ref)
    CopyFrameBuffer (GR_BUFFER_BACKBUFFER);

  if ((settings.hacks&hack_TGR2) && rdp.vi_org_reg != *gfx_info.VI_ORIGIN_REG && CI_SET)
  {
    newSwapBuffers ();
    CI_SET = false;
  }
}

static void colorimage_yoshis_story_memrect(uint16_t ul_x, uint16_t ul_y, 
      uint16_t lr_x, uint16_t lr_y, uint16_t tileno)
{
  uint32_t y;
  uint32_t off_x     = ((__RDP.w2 & 0xFFFF0000) >> 16) >> 5;
  uint32_t off_y     = ((__RDP.w2 & 0x0000FFFF) >> 5);
  uint32_t width     = lr_x - ul_x;
  uint32_t tex_width = g_gdp.tile[tileno].line << 3;
  uint8_t *texaddr   = (uint8_t*)(gfx_info.RDRAM + rdp.addr[g_gdp.tile[tileno].tmem] + tex_width*off_y + off_x);
  uint8_t *fbaddr    = (uint8_t*)(gfx_info.RDRAM + gDP.colorImage.address + ul_x);

  if (lr_y > g_gdp.__clip.yl)
    lr_y = g_gdp.__clip.yl;

  for (y = ul_y; y < lr_y; y++)
  {
    uint8_t *src = (uint8_t*)(texaddr + (y - ul_y) * tex_width);
    uint8_t *dst = (uint8_t*)(fbaddr + y * gDP.colorImage.width);
    memcpy (dst, src, width);
  }
}

static void colorimage_palette_modification(void)
{
   unsigned i;

   uint8_t envr        = (uint8_t)(g_gdp.env_color.r * 0.0039215689f * 31.0f);
   uint8_t envg        = (uint8_t)(g_gdp.env_color.g * 0.0039215689f * 31.0f);
   uint8_t envb        = (uint8_t)(g_gdp.env_color.b * 0.0039215689f * 31.0f);
   uint16_t env16      = (uint16_t)((envr<<11)|(envg<<6)|(envb<<1)|1);
   uint16_t prmr       = (uint8_t)(g_gdp.prim_color.r * 0.0039215689f * 31.0f);
   uint16_t prmg       = (uint8_t)(g_gdp.prim_color.g * 0.0039215689f * 31.0f);
   uint16_t prmb       = (uint8_t)(g_gdp.prim_color.b * 0.0039215689f * 31.0f);
   uint16_t prim16     = (uint16_t)((prmr << 11)|(prmg << 6)|(prmb << 1)|1);
   uint16_t *ptr_dst   = (uint16_t*)(gfx_info.RDRAM + gDP.colorImage.address);

   for (i = 0; i < 16; i++)
      ptr_dst[i^1] = (rdp.pal_8[i]&1) ? prim16 : env16;
}

static void colorimage_zbuffer_copy(uint32_t w0, uint32_t w1)
{
   unsigned x;
   uint16_t      ul_x = (uint16_t)((w1 & 0x00FFF000) >> 14);
   uint16_t      lr_x = (uint16_t)((w0 & 0x00FFF000) >> 14) + 1;
   uint16_t      ul_u = (uint16_t)((__RDP.w2 & 0xFFFF0000) >> 21) + 1;
   uint16_t     width = lr_x - ul_x;
   uint16_t  *ptr_src = ((uint16_t*)g_gdp.tmem)+ul_u;
   uint16_t  *ptr_dst = (uint16_t*)(gfx_info.RDRAM + gDP.colorImage.address);

   for (x = 0; x < width; x++)
   {
      uint16_t c = ptr_src[x];
      c = ((c << 8) & 0xFF00) | (c >> 8);
      ptr_dst[(ul_x+x)^1] = c;
   }
}

enum rdp_tex_rect_mode
{
   GSP_TEX_RECT = 0,
   GDP_TEX_RECT,
   HALF_TEX_RECT
};

static void rdp_getTexRectParams(uint32_t *w2, uint32_t *w3)
{
   enum rdp_tex_rect_mode texRectMode = GDP_TEX_RECT;
   uint32_t a, cmdHalf1, cmdHalf2;
   if (__RSP.bLLE)
   {
      *w2 = __RDP.w2;
      *w3 = __RDP.w3;
      return;
   }

   a = __RSP.PC[__RSP.PCi];
   cmdHalf1 = gfx_info.RDRAM[a+3];
   cmdHalf2 = gfx_info.RDRAM[a+11];
   a >>= 2;

   if (  (cmdHalf1 == 0xE1 && cmdHalf2 == 0xF1) || 
         (cmdHalf1 == 0xB4 && cmdHalf2 == 0xB3) || 
         (cmdHalf1 == 0xB3 && cmdHalf2 == 0xB2)
      )
   {
      texRectMode = GSP_TEX_RECT;
   }

   switch (texRectMode)
   {
      case GSP_TEX_RECT:
         //gSPTextureRectangle
         *w2 = ((uint32_t*)gfx_info.RDRAM)[a+1];
         __RSP.PC[__RSP.PCi] += 8;

         *w3 = ((uint32_t*)gfx_info.RDRAM)[a+3];
         __RSP.PC[__RSP.PCi] += 8;
         break;
      case GDP_TEX_RECT:
         //gDPTextureRectangle
         if (settings.hacks&hack_ASB ||
               settings.hacks & hack_Winback
               )
            *w2 = 0;
         else
            *w2 = ((uint32_t*)gfx_info.RDRAM)[a+0];

         if (settings.hacks & hack_Winback)
            *w3 = 0;
         else
            *w3 = ((uint32_t*)gfx_info.RDRAM)[a+1];
         __RSP.PC[__RSP.PCi] += 8;
         break;
      case HALF_TEX_RECT:
         break;
   }
}

static void rdp_texrect(uint32_t w0, uint32_t w1)
{
   float ul_x, ul_y, lr_x, lr_y;
   int i;
   int32_t off_x_i, off_y_i;
   uint32_t prev_tile;
   int32_t tilenum;
   float Z, dsdx, dtdy, s_ul_x, s_lr_x, s_ul_y, s_lr_y, off_size_x, off_size_y;
   struct {
      float ul_u, ul_v, lr_u, lr_v;
   } texUV[2]; //struct for texture coordinates
   VERTEX *vptr = NULL, vstd[4];
   
   rdp_getTexRectParams(&__RDP.w2, &__RDP.w3);

   if ((settings.hacks&hack_Yoshi) && settings.ucode == ucode_S2DEX)
   {
      colorimage_yoshis_story_memrect(
            (uint16_t)((w1 & 0x00FFF000) >> 14),   /* ul_x */
            (uint16_t)((w1 & 0x00000FFF) >> 2),    /* ul_y */
            (uint16_t)((w0 & 0x00FFF000) >> 14),   /* lr_x */
            (uint16_t)((w0 & 0x00000FFF) >> 2),    /* lr_y */
            (uint16_t)((w1 & 0x07000000) >> 24)    /* tileno */
            );
      return;
   }

   if (rdp.skip_drawing || (!fb_emulation_enabled && (gDP.colorImage.address == g_gdp.zb_address)) || __RDP.w3 == 0)
   {
      if ((settings.hacks&hack_PMario) && rdp.ci_status == CI_USELESS)
         colorimage_palette_modification();
      return;
   }

   if ((settings.ucode == ucode_PerfectDark) 
         && rdp.ci_count > 0 && (rdp.frame_buffers[rdp.ci_count-1].status == CI_ZCOPY))
   {
      colorimage_zbuffer_copy(w0, w1);
      LRDP("Depth buffer copied.\n");
      return;
   }

   if ((gDP.otherMode.l >> 16) == 0x3c18 && 
         rdp.cycle1 == 0x03ffffff && 
         rdp.cycle2 == 0x01ff1fff) //depth image based fog
   {
      if (!depth_buffer_fog)
         return;
      depth_buffer_fog = false;

      if (settings.fog)
         DrawDepthBufferFog();

      return;
   }

   // FRDP ("rdp.cycle1 %08lx, rdp.cycle2 %08lx\n", rdp.cycle1, rdp.cycle2);

   if (gDP.otherMode.cycleType == G_CYC_COPY)
   {
      ul_x = (int16_t)((w1 & 0x00FFF000) >> 14);
      ul_y = (int16_t)((w1 & 0x00000FFF) >> 2);
      lr_x = (int16_t)((w0 & 0x00FFF000) >> 14);
      lr_y = (int16_t)((w0 & 0x00000FFF) >> 2);
   }
   else
   {
      ul_x = _SHIFTR(w1, 12, 12) / 4.0f;
      ul_y = _SHIFTR(w1,  0, 12) / 4.0f;
      lr_x = _SHIFTR(w0, 12, 12) / 4.0f;
      lr_y = _SHIFTR(w0,  0, 12) / 4.0f;
   }

   if (ul_x >= lr_x)
   {
      FRDP("Wrong Texrect: ul_x: %f, lr_x: %f\n", ul_x, lr_x);
      return;
   }

   if (gDP.otherMode.cycleType > 1)
   {
      lr_x += 1.0f;
      lr_y += 1.0f;
   } else if (lr_y - ul_y < 1.0f)
      lr_y = ceilf(lr_y);

   if (settings.increase_texrect_edge)
   {
      if (floor(lr_x) != lr_x)
         lr_x = ceilf(lr_x);
      if (floor(lr_y) != lr_y)
         lr_y = ceilf(lr_y);
   }

   //*/
   // framebuffer workaround for Zelda: MM LOT
   if ((gDP.otherMode.l & 0xFFFF0000) == 0x0f5a0000)
      return;

   /*Gonetz*/
   //hack for Zelda MM. it removes black texrects which cover all geometry in "Link meets Zelda" cut scene
   if ((settings.hacks&hack_Zelda) && g_gdp.ti_address >= gDP.colorImage.address && g_gdp.ti_address < rdp.ci_end)
   {
      FRDP("Wrong Texrect. texaddr: %08lx, cimg: %08lx, cimg_end: %08lx\n", rdp.cur_cache[0]->addr, gDP.colorImage.address, gDP.colorImage.address + gDP.colorImage.width*gDP.colorImage.height*2);
      return;
   }

   //hack for Banjo2. it removes black texrects under Banjo
   if (settings.hacks&hack_Banjo2 && (!fb_hwfbe_enabled && ((rdp.cycle1 << 16) | (rdp.cycle2 & 0xFFFF)) == 0xFFFFFFFF && (gDP.otherMode.l & 0xFFFF0000) == 0x00500000))
      return;

   /*remove motion blur in night vision */
   if (
         (    settings.ucode == ucode_PerfectDark)
         && (rdp.maincimg[1].addr != rdp.maincimg[0].addr)
         && (g_gdp.ti_address >= rdp.maincimg[1].addr)
         && (g_gdp.ti_address < (rdp.maincimg[1].addr + gDP.colorImage.width * gDP.colorImage.height* g_gdp.fb_size))
      )
   {
      if (fb_emulation_enabled)
      {
         /* FRDP("Wrong Texrect. texaddr: %08lx, cimg: %08lx, cimg_end: %08lx\n",
          * g_gdp.ti_address, rdp.maincimg[1], rdp.maincimg[1] + gDP.colorImage.width * gDP.colorImage.height * g_gdp.fb_size); */
         if (rdp.ci_count > 0 && rdp.frame_buffers[rdp.ci_count-1].status == CI_COPY_SELF)
            return;
      }
   }

   tilenum        = (uint16_t)((w1 & 0x07000000) >> 24);

   rdp.texrecting = 1;

   prev_tile      = rdp.cur_tile;
   rdp.cur_tile   = tilenum;

   Z              = set_sprite_combine_mode ();

   rdp.texrecting = 0;

   if (!rdp.cur_cache[0])
   {
      rdp.cur_tile = prev_tile;
      return;
   }
   // ****
   // ** Texrect offset by Gugaman **
   //
   //integer representation of texture coordinate.
   //needed to detect and avoid overflow after shifting
   off_x_i = (__RDP.w2 >> 16) & 0xFFFF;
   off_y_i = __RDP.w2 & 0xFFFF;
   dsdx    = _FIXED2FLOAT((int16_t)((__RDP.w3 & 0xFFFF0000) >> 16), 10);
   dtdy    = _FIXED2FLOAT(((int16_t)(__RDP.w3 & 0x0000FFFF)), 10);
   if (off_x_i & 0x8000) //check for sign bit
      off_x_i |= ~0xffff; //make it negative
   //the same as for off_x_i
   if (off_y_i & 0x8000)
      off_y_i |= ~0xffff;

   if (gDP.otherMode.cycleType == G_CYC_COPY)
      dsdx /= 4.0f;

   s_ul_x = ul_x * rdp.scale_x + rdp.offset_x;
   s_lr_x = lr_x * rdp.scale_x + rdp.offset_x;
   s_ul_y = ul_y * rdp.scale_y + rdp.offset_y;
   s_lr_y = lr_y * rdp.scale_y + rdp.offset_y;

   if ( ((__RSP.w0 >> 24)&0xFF) == G_TEXRECTFLIP )
   {
      off_size_x = (lr_y - ul_y - 1) * dsdx;
      off_size_y = (lr_x - ul_x - 1) * dtdy;
   }
   else
   {
      off_size_x = (lr_x - ul_x - 1) * dsdx;
      off_size_y = (lr_y - ul_y - 1) * dtdy;
   }

   //calculate texture coordinates
   for (i = 0; i < 2; i++)
   {
      texUV[i].ul_u = texUV[i].ul_v = texUV[i].lr_u = texUV[i].lr_v = 0;

      if (rdp.cur_cache[i] && (rdp.tex & (i+1)))
      {
         unsigned tilenum = rdp.cur_tile + i;
         float maxs       = 1;
         float maxt       = 1;
         int x_i          = off_x_i;
         int y_i          = off_y_i;

         //shifting
         if (g_gdp.tile[tilenum].shift_s)
         {
            if (g_gdp.tile[tilenum].shift_s < 11)
            {
               x_i   = SIGN16(x_i);
               x_i >>= g_gdp.tile[tilenum].shift_s;
               maxs = 1.0f/(float)(1 << g_gdp.tile[tilenum].shift_s);
            }
            else
            {
               uint8_t iShift = (16 - g_gdp.tile[tilenum].shift_s);
               x_i <<= SIGN16(iShift);
               maxs = (float)(1 << iShift);
            }
         }

         if (g_gdp.tile[tilenum].shift_t)
         {
            if (g_gdp.tile[tilenum].shift_t < 11)
            {
               y_i   = SIGN16(y_i);
               y_i >>= g_gdp.tile[tilenum].shift_t;
               maxt = 1.0f/(float)(1 << g_gdp.tile[tilenum].shift_t);
            }
            else
            {
               uint8_t iShift = (16 - g_gdp.tile[tilenum].shift_t);
               y_i <<= SIGN16(iShift);
               maxt = (float)(1 << iShift);
            }
         }

         texUV[i].ul_u = x_i / 32.0f;
         texUV[i].ul_v = y_i / 32.0f;

         texUV[i].ul_u -= gDP.tiles[tilenum].fuls;
         texUV[i].ul_v -= gDP.tiles[tilenum].fult;

         texUV[i].lr_u = texUV[i].ul_u + off_size_x * maxs;
         texUV[i].lr_v = texUV[i].ul_v + off_size_y * maxt;

         texUV[i].ul_u = rdp.cur_cache[i]->c_off + rdp.cur_cache[i]->c_scl_x * texUV[i].ul_u;
         texUV[i].lr_u = rdp.cur_cache[i]->c_off + rdp.cur_cache[i]->c_scl_x * texUV[i].lr_u;
         texUV[i].ul_v = rdp.cur_cache[i]->c_off + rdp.cur_cache[i]->c_scl_y * texUV[i].ul_v;
         texUV[i].lr_v = rdp.cur_cache[i]->c_off + rdp.cur_cache[i]->c_scl_y * texUV[i].lr_v;
      }
   }
   rdp.cur_tile = prev_tile;

   // ****

   FRDP (" scissor: (%d, %d) -> (%d, %d)\n", rdp.scissor.ul_x, rdp.scissor.ul_y, rdp.scissor.lr_x, rdp.scissor.lr_y);

   CCLIP2 (s_ul_x, s_lr_x, texUV[0].ul_u, texUV[0].lr_u, texUV[1].ul_u, texUV[1].lr_u, (float)rdp.scissor.ul_x, (float)rdp.scissor.lr_x);
   CCLIP2 (s_ul_y, s_lr_y, texUV[0].ul_v, texUV[0].lr_v, texUV[1].ul_v, texUV[1].lr_v, (float)rdp.scissor.ul_y, (float)rdp.scissor.lr_y);

   vstd[0].x        = s_ul_x;
   vstd[0].y        = s_ul_y;
   vstd[0].z        = Z;
   vstd[0].q        = 1.0f;
   vstd[0].b        = 0;
   vstd[0].g        = 0;
   vstd[0].r        = 0;
   vstd[0].a        = 0;
   vstd[0].coord[0] = 0;
   vstd[0].coord[1] = 0;
   vstd[0].coord[2] = 0;
   vstd[0].coord[3] = 0;
   vstd[0].f        = 255.0f;
   vstd[0].u[0]     = texUV[0].ul_u;
   vstd[0].u[1]     = texUV[1].ul_u;
   vstd[0].v[0]     = texUV[0].ul_v;
   vstd[0].v[1]     = texUV[1].ul_v;
   vstd[0].w        = 0.0f;
   vstd[0].flags    = 0;
   vstd[0].vec[0]   = 0.0f;
   vstd[0].vec[1]   = 0.0f;
   vstd[0].vec[2]   = 0.0f;
   vstd[0].sx       = 0.0f;
   vstd[0].sy       = 0.0f;
   vstd[0].sz       = 0.0f;
   vstd[0].x_w      = 0.0f;
   vstd[0].y_w      = 0.0f;
   vstd[0].z_w      = 0.0f;
   vstd[0].oow      = 0.0f;
   vstd[0].u_w[0]   = 0.0f;
   vstd[0].u_w[1]   = 0.0f;
   vstd[0].v_w[0]            = 0.0f;
   vstd[0].v_w[1]            = 0.0f;
   vstd[0].not_zclipped      = 0;
   vstd[0].screen_translated = 0;
   vstd[0].uv_scaled         = 0;
   vstd[0].uv_calculated     = 0;
   vstd[0].shade_mod         = 0;
   vstd[0].color_backup      = 0;
   vstd[0].ou                = 0.0f;
   vstd[0].ov                = 0.0f;
   vstd[0].number            = 0;
   vstd[0].scr_off           = 0;
   vstd[0].z_off             = 0;

   vstd[1].x = s_lr_x;
   vstd[1].y = s_ul_y;
   vstd[1].z = Z;
   vstd[1].q = 1.0f;
   vstd[1].b        = 0.0f;
   vstd[1].g        = 0.0f;
   vstd[1].r        = 0.0f;
   vstd[1].a        = 0.0f;
   vstd[1].coord[0] = 0;
   vstd[1].coord[1] = 0;
   vstd[1].coord[2] = 0;
   vstd[1].coord[3] = 0;
   vstd[1].f        = 255.0f;
   vstd[1].u[0]     = texUV[0].lr_u;
   vstd[1].u[1]     = texUV[1].lr_u;
   vstd[1].v[0]     = texUV[0].ul_v;
   vstd[1].v[1]     = texUV[1].ul_v;
   vstd[1].w        = 0.0f;
   vstd[1].flags    = 0;
   vstd[1].vec[0]   = 0.0f;
   vstd[1].vec[1]   = 0.0f;
   vstd[1].vec[2]   = 0.0f;
   vstd[1].sx       = 0.0f;
   vstd[1].sy       = 0.0f;
   vstd[1].sz       = 0.0f;
   vstd[1].x_w      = 0.0f;
   vstd[1].y_w      = 0.0f;
   vstd[1].z_w      = 0.0f;
   vstd[1].oow      = 0.0f;
   vstd[1].u_w[0]   = 0.0f;
   vstd[1].u_w[1]   = 0.0f;
   vstd[1].v_w[0]            = 0.0f;
   vstd[1].v_w[1]            = 0.0f;
   vstd[1].not_zclipped      = 0;
   vstd[1].screen_translated = 0;
   vstd[1].uv_scaled         = 0;
   vstd[1].uv_calculated     = 0;
   vstd[1].shade_mod         = 0;
   vstd[1].color_backup      = 0;
   vstd[1].ou                = 0.0f;
   vstd[1].ov                = 0.0f;
   vstd[1].number            = 0;
   vstd[1].scr_off           = 0;
   vstd[1].z_off             = 0;

   vstd[2].x = s_ul_x;
   vstd[2].y = s_lr_y;
   vstd[2].z = Z;
   vstd[2].q = 1.0f;
   vstd[2].b        = 0.0f;
   vstd[2].g        = 0.0f;
   vstd[2].r        = 0.0f;
   vstd[2].a        = 0.0f;
   vstd[2].coord[0] = 0;
   vstd[2].coord[1] = 0;
   vstd[2].coord[2] = 0;
   vstd[2].coord[3] = 0;
   vstd[2].f = 255.0f;
   vstd[2].u[0] = texUV[0].ul_u;
   vstd[2].v[0] = texUV[0].lr_v;
   vstd[2].u[1] = texUV[1].ul_u;
   vstd[2].v[1] = texUV[1].lr_v;
   vstd[2].w        = 0.0f;
   vstd[2].flags    = 0;
   vstd[2].vec[0]   = 0.0f;
   vstd[2].vec[1]   = 0.0f;
   vstd[2].vec[2]   = 0.0f;
   vstd[2].sx       = 0.0f;
   vstd[2].sy       = 0.0f;
   vstd[2].sz       = 0.0f;
   vstd[2].x_w      = 0.0f;
   vstd[2].y_w      = 0.0f;
   vstd[2].z_w      = 0.0f;
   vstd[2].oow      = 0.0f;
   vstd[2].u_w[0]   = 0.0f;
   vstd[2].u_w[1]   = 0.0f;
   vstd[2].v_w[0]            = 0.0f;
   vstd[2].v_w[1]            = 0.0f;
   vstd[2].not_zclipped      = 0;
   vstd[2].screen_translated = 0;
   vstd[2].uv_scaled         = 0;
   vstd[2].uv_calculated     = 0;
   vstd[2].shade_mod         = 0;
   vstd[2].color_backup      = 0;
   vstd[2].ou                = 0.0f;
   vstd[2].ov                = 0.0f;
   vstd[2].number            = 0;
   vstd[2].scr_off           = 0;
   vstd[2].z_off             = 0;

   vstd[3].x = s_lr_x;
   vstd[3].y = s_lr_y;
   vstd[3].z = Z;
   vstd[3].q = 1.0f;
   vstd[3].b        = 0.0f;
   vstd[3].g        = 0.0f;
   vstd[3].r        = 0.0f;
   vstd[3].a        = 0.0f;
   vstd[3].coord[0] = 0;
   vstd[3].coord[1] = 0;
   vstd[3].coord[2] = 0;
   vstd[3].coord[3] = 0;
   vstd[3].f = 255.0f;
   vstd[3].u[0] = texUV[0].lr_u;
   vstd[3].v[0] = texUV[0].lr_v;
   vstd[3].u[1] = texUV[1].lr_u;
   vstd[3].v[1] = texUV[1].lr_v;
   vstd[3].w        = 0.0f;
   vstd[3].flags    = 0;
   vstd[3].vec[0]   = 0.0f;
   vstd[3].vec[1]   = 0.0f;
   vstd[3].vec[2]   = 0.0f;
   vstd[3].sx       = 0.0f;
   vstd[3].sy       = 0.0f;
   vstd[3].sz       = 0.0f;
   vstd[3].x_w      = 0.0f;
   vstd[3].y_w      = 0.0f;
   vstd[3].z_w      = 0.0f;
   vstd[3].oow      = 0.0f;
   vstd[3].u_w[0]   = 0.0f;
   vstd[3].u_w[1]   = 0.0f;
   vstd[3].v_w[0]            = 0.0f;
   vstd[3].v_w[1]            = 0.0f;
   vstd[3].not_zclipped      = 0;
   vstd[3].screen_translated = 0;
   vstd[3].uv_scaled         = 0;
   vstd[3].uv_calculated     = 0;
   vstd[3].shade_mod         = 0;
   vstd[3].color_backup      = 0;
   vstd[3].ou                = 0.0f;
   vstd[3].ov                = 0.0f;
   vstd[3].number            = 0;
   vstd[3].scr_off           = 0;
   vstd[3].z_off             = 0;

   if ( ((__RSP.w0 >> 24)&0xFF) == G_TEXRECTFLIP )
   {
      vstd[1].u[0] = texUV[0].ul_u;
      vstd[1].v[0] = texUV[0].lr_v;
      vstd[1].u[1] = texUV[1].ul_u;
      vstd[1].v[1] = texUV[1].lr_v;

      vstd[2].u[0] = texUV[0].lr_u;
      vstd[2].v[0] = texUV[0].ul_v;
      vstd[2].u[1] = texUV[1].lr_u;
      vstd[2].v[1] = texUV[1].ul_v;
   }


   vptr = (VERTEX*)vstd;

   apply_shading(vptr);

#if 0
   if (fullscreen)
#endif
   {
      if (rdp.fog_mode >= FOG_MODE_BLEND)
      {
         float fog = 1.0f/ g_gdp.fog_color.a;
         if (rdp.fog_mode != FOG_MODE_BLEND)
            fog = 1.0f / ((~g_gdp.fog_color.total)&0xFF);

         for (i = 0; i < 4; i++)
            vptr[i].f = fog;
         grFogMode (GR_FOG_WITH_TABLE_ON_FOGCOORD_EXT, g_gdp.fog_color.total);
      }

      ConvertCoordsConvert (vptr, 4);
      grDrawVertexArrayContiguous (GR_TRIANGLE_STRIP, 4, vptr);
   }
}

static void rdp_setscissor(uint32_t w0, uint32_t w1)
{
   gdp_set_scissor(w0, w1);

   /* clipper resolution is 320x240, scale based on computer resolution */
   /* TODO/FIXME - all these values are different from Angrylion's */
   gDPSetScissor(
         0,                                        /* mode */
         (uint32_t)(((w0 & 0x00FFF000) >> 14)),    /* ulx */
         (uint32_t)(((w0 & 0x00000FFF) >> 2)),     /* uly */
         (uint32_t)(((w1 & 0x00FFF000) >> 14)),    /* lrx */
         (uint32_t)(((w1 & 0x00000FFF) >> 2))      /* lry */
         );

   rdp.ci_upper_bound = g_gdp.__clip.yh;
   rdp.ci_lower_bound = g_gdp.__clip.yl;
   rdp.scissor_set    = true;

   if (gSP.viewport.vscale[0] == 0) //viewport is not set?
   {
      gSP.viewport.vscale[0] = (g_gdp.__clip.xl >> 1) * rdp.scale_x;
      gSP.viewport.vscale[1] = (g_gdp.__clip.yl >> 1) * -rdp.scale_y;
      gSP.viewport.vtrans[0] = gSP.viewport.vscale[0];
      gSP.viewport.vtrans[1] = -gSP.viewport.vscale[1];
      g_gdp.flags |= UPDATE_VIEWPORT;
   }
}

#define F3DEX2_SETOTHERMODE(cmd,sft,len,data) { \
   __RSP.w0 = (cmd<<24) | ((32-(sft)-(len))<<8) | (((len)-1)); \
   __RSP.w1 = data; \
   gfx_instruction[settings.ucode][cmd](__RSP.w0, __RSP.w1); \
}
#define SETOTHERMODE(cmd,sft,len,data) { \
   __RSP.w0 = (cmd<<24) | ((sft)<<8) | (len); \
   __RSP.w1 = data; \
   gfx_instruction[settings.ucode][cmd](__RSP.w0, __RSP.w1); \
}

static void rdp_setothermode(uint32_t w0, uint32_t w1)
{
   gdp_set_other_modes(w0, w1);

   if ((settings.ucode == ucode_F3DEX2) || (settings.ucode == ucode_CBFD))
   {
      int cmd0 = __RSP.w0;
      F3DEX2_SETOTHERMODE(0xE2, 0, 32, __RSP.w1); // SETOTHERMODE_L
      F3DEX2_SETOTHERMODE(0xE3, 0, 32, cmd0 & 0x00FFFFFF); // SETOTHERMODE_H
   }
   else
   {
      int cmd0 = __RSP.w0;
      SETOTHERMODE(0xB9, 0, 32, __RSP.w1); // SETOTHERMODE_L
      SETOTHERMODE(0xBA, 0, 32, cmd0 & 0x00FFFFFF); // SETOTHERMODE_H
   }
}

void load_palette (uint32_t addr, uint16_t start, uint16_t count)
{
   uint16_t i, p;
   uint16_t *dpal = (uint16_t*)(rdp.pal_8 + start);
   uint16_t end   = start+count;

   for (i=start; i<end; i++)
   {
      *(dpal++) = *(uint16_t *)(gfx_info.RDRAM + (addr^2));
      addr += 2;
   }

   start >>= 4;
   end = start + (count >> 4);
   if (end == start) // it can be if count < 16
      end = start + 1;
   for (p = start; p < end; p++)
      rdp.pal_8_crc[p] = encoding_crc32( 0xFFFFFFFF, (const uint8_t*)&rdp.pal_8[(p << 4)], 32 );
   gDP.paletteCRC256   = encoding_crc32( 0xFFFFFFFF, (const uint8_t*)rdp.pal_8_crc, 64 );
}

static void rdp_loadtlut(uint32_t w0, uint32_t w1)
{
   uint32_t tile  =  (w1 >> 24) & 0x07;
   uint16_t start = g_gdp.tile[tile].tmem - 256; // starting location in the palettes
   uint16_t count = ((uint16_t)(w1 >> 14) & 0x3FF) + 1;    // number to copy

   if (g_gdp.ti_address + (count<<1) > BMASK)
      count = (uint16_t)((BMASK - g_gdp.ti_address) >> 1);

   if (start+count > 256)
      count = 256-start;

   load_palette (g_gdp.ti_address, start, count);

   g_gdp.ti_address += count << 1;
}

static void rdp_settilesize(uint32_t w0, uint32_t w1)
{
   int tilenum               = gdp_set_tile_size_wrap(w0, w1);

   rdp.last_tile_size        = tilenum;

   /* TODO - should get rid of this eventually */
   gDP.tiles[tilenum].fuls = _FIXED2FLOAT(g_gdp.tile[tilenum].sl, 2);
   gDP.tiles[tilenum].fult = _FIXED2FLOAT(g_gdp.tile[tilenum].tl, 2);

   /* TODO - Wrong values being used by Glide64  - unify this
    * later so that it uses the same values as Angrylion */
   glide64gDPSetTileSize(
         tilenum,                            /* tile */
         (((uint16_t)(w0 >> 14)) & 0x03ff),  /* uls  */   
         (((uint16_t)(w0 >> 2 )) & 0x03ff),  /* ult  */
         (((uint16_t)(w1 >> 14)) & 0x03ff),  /* lrs  */
         (((uint16_t)(w1 >> 2 )) & 0x03ff)   /* lrt  */
         );

   /* handle wrapping */
   if (g_gdp.tile[tilenum].sl < g_gdp.tile[tilenum].sh)
      g_gdp.tile[tilenum].sl += 0x400;
   if (g_gdp.tile[tilenum].tl < g_gdp.tile[tilenum].th)
      g_gdp.tile[tilenum].tl += 0x400;
}

static void rdp_loadblock(uint32_t w0, uint32_t w1)
{
   /* lr_s specifies number of 64-bit words to copy
    * 10.2 format. */
   gDPLoadBlock(
         _SHIFTR(w1, 24, 3),     /* tile */ 
         (w0 >> 14) & 0x3FF,     /* ul_s */
         (w0 >>  2) & 0x3FF,     /* ul_t */
         (w1 >> 14) & 0x3FF,     /* lr_s */
         _SHIFTR(w1, 0, 12)      /* dxt */
         );
}

static void rdp_loadtile(uint32_t w0, uint32_t w1)
{
   glide64gDPLoadTile(
         (uint32_t)((w1 >> 24) & 0x07),      /* tile */
         (uint32_t)((w0 >> 14) & 0x03FF),    /* ul_s */
         (uint32_t)((w0 >> 2 ) & 0x03FF),    /* ul_t */
         (uint32_t)((w1 >> 14) & 0x03FF),    /* lr_s */
         (uint32_t)((w1 >> 2 ) & 0x03FF)     /* lr_t */
         );
}

static void rdp_settile(uint32_t w0, uint32_t w1)
{
   rdp.last_tile = gdp_set_tile(w0, w1);
}

static void rdp_fillrect(uint32_t w0, uint32_t w1)
{
	const uint32_t ul_x = _SHIFTR(w1, 14, 10);
	const uint32_t ul_y = _SHIFTR(w1, 2, 10);
	const uint32_t lr_x = _SHIFTR(w0, 14, 10) + 1;
	const uint32_t lr_y = _SHIFTR(w0, 2, 10)  + 1;

   /* Wrong coordinates. Skipped */
   if (lr_x < ul_x || lr_y < ul_y)
      return;

   glide64gDPFillRectangle(ul_x, ul_y, lr_x, lr_y);
}

static void rdp_setprimcolor(uint32_t w0, uint32_t w1)
{
   gdp_set_prim_color(w0, w1);
   /* TODO/FIXME - different value from the one Angrylion sets. */
   g_gdp.primitive_lod_min = (w0 >> 8) & 0xFF;
}

static void rdp_setcombine(uint32_t w0, uint32_t w1)
{
   gdp_set_combine(w0, w1);
   rdp.cycle1 = (g_gdp.combine.sub_a_rgb0 << 0) | (g_gdp.combine.sub_b_rgb0 << 4) | (g_gdp.combine.mul_rgb0 << 8) | (g_gdp.combine.add_rgb0 << 13) |
      (g_gdp.combine.sub_a_a0 << 16) | (g_gdp.combine.sub_b_a0 << 19)| (g_gdp.combine.mul_a0 << 22)| (g_gdp.combine.add_a0 << 25);
   rdp.cycle2 = (g_gdp.combine.sub_a_rgb1 << 0) | (g_gdp.combine.sub_b_rgb1 << 4) | (g_gdp.combine.mul_rgb1 << 8) | (g_gdp.combine.add_rgb1 << 13) |
      (g_gdp.combine.sub_a_a1 << 16)| (g_gdp.combine.sub_b_a1 << 19)| (g_gdp.combine.mul_a1 << 22)| (g_gdp.combine.add_a1 << 25);
}

static void rdp_settextureimage(uint32_t w0, uint32_t w1)
{
   gdp_set_texture_image(w0, w1);

   /* TODO/FIXME - all different values from the ones Angrylion sets. */
   glide64gDPSetTextureImage(
         _SHIFTR(w0, 21, 3),                 /* format */
         g_gdp.ti_size,                      /* size   */
         _SHIFTR(w0, 0, 12) + 1,             /* width */
         RSP_SegmentToPhysical(w1)           /* address */
         );

   if (gSP.DMAOffsets.tex_offset)
   {
      if (g_gdp.ti_format == G_IM_FMT_RGBA)
      {
         uint16_t * t               = (uint16_t*)(gfx_info.RDRAM + gSP.DMAOffsets.tex_offset);
         gSP.DMAOffsets.tex_shift   = t[gSP.DMAOffsets.tex_count ^ 1];
         g_gdp.ti_address          += gSP.DMAOffsets.tex_shift;
      }
      else
      {
         gSP.DMAOffsets.tex_offset  = 0;
         gSP.DMAOffsets.tex_shift   = 0;
         gSP.DMAOffsets.tex_count   = 0;
      }
   }

   rdp.s2dex_tex_loaded = true;

   if (rdp.ci_count > 0 && rdp.frame_buffers[rdp.ci_count-1].status == CI_COPY_SELF && (g_gdp.ti_address >= gDP.colorImage.address) && (g_gdp.ti_address < rdp.ci_end))
   {
      if (!rdp.fb_drawn)
      {
            CopyFrameBuffer(GR_BUFFER_BACKBUFFER);
         rdp.fb_drawn = true;
      }
   }
}

static void rdp_setdepthimage(uint32_t w0, uint32_t w1)
{
   gdp_set_mask_image(w0, w1);

   /* TODO/FIXME - different from Angrylion's value */
   g_gdp.zb_address = RSP_SegmentToPhysical(w1);

   rdp.zi_width = gDP.colorImage.width;
}

int SwapOK = true;

static void RestoreScale(void)
{
   rdp.scale_x        = rdp.scale_x_bak;
   rdp.scale_y        = rdp.scale_y_bak;
   gSP.viewport.vscale[0] *= rdp.scale_x;
   gSP.viewport.vscale[1] *= rdp.scale_y;
   gSP.viewport.vtrans[0] *= rdp.scale_x;
   gSP.viewport.vtrans[1] *= rdp.scale_y;

   g_gdp.flags       |= UPDATE_VIEWPORT | UPDATE_SCISSOR;
}

static void rdp_setcolorimage(uint32_t w0, uint32_t w1)
{
   gdp_set_color_image(w0, w1);

   if (fb_emulation_enabled && (rdp.num_of_ci < NUMTEXBUF))
   {
      COLOR_IMAGE *cur_fb  = (COLOR_IMAGE*)&rdp.frame_buffers[rdp.ci_count];
      COLOR_IMAGE *prev_fb = (COLOR_IMAGE*)&rdp.frame_buffers[rdp.ci_count?rdp.ci_count-1:0];
      COLOR_IMAGE *next_fb = (COLOR_IMAGE*)&rdp.frame_buffers[rdp.ci_count+1];

      switch (cur_fb->status)
      {
         case CI_MAIN:
            if (rdp.ci_count == 0)
            {
               if (rdp.ci_status == CI_AUX) //for PPL
               {
                  float sx    = rdp.scale_x;
                  float sy    = rdp.scale_y;
                  rdp.scale_x = 1.0f;
                  rdp.scale_y = 1.0f;

                  CopyFrameBuffer (GR_BUFFER_BACKBUFFER);
                  rdp.scale_x = sx;
                  rdp.scale_y = sy;
               }
               {
                  if ((rdp.num_of_ci > 1) &&
                        (next_fb->status == CI_AUX) &&
                        (next_fb->width >= cur_fb->width))
                  {
                     rdp.scale_x = 1.0f;
                     rdp.scale_y = 1.0f;
                  }
               }
            }
            //else if (rdp.ci_status == CI_AUX && !rdp.copy_ci_index)
            // CloseTextureBuffer();

            rdp.skip_drawing = false;
            break;
         case CI_COPY:
            if (!rdp.motionblur || (settings.frame_buffer&fb_motionblur))
            {
               if (cur_fb->width == gDP.colorImage.width)
               {
                  {
                     if (!rdp.fb_drawn || prev_fb->status == CI_COPY_SELF)
                     {
                        CopyFrameBuffer (GR_BUFFER_BACKBUFFER);
                        rdp.fb_drawn = true;
                     }
                     memcpy(gfx_info.RDRAM+cur_fb->addr,
                           gfx_info.RDRAM + gDP.colorImage.address,
                           (cur_fb->width*cur_fb->height)<<cur_fb->size>>1);
                  }
               }
            }
            else
               memset(gfx_info.RDRAM+cur_fb->addr, 0, cur_fb->width * cur_fb->height * g_gdp.fb_size);
            rdp.skip_drawing = true;
            break;
         case CI_AUX_COPY:
            rdp.skip_drawing = false;
            if (!rdp.fb_drawn)
            {
               CopyFrameBuffer (GR_BUFFER_BACKBUFFER);
               rdp.fb_drawn = true;
            }
            break;
         case CI_OLD_COPY:
            if (!rdp.motionblur || (settings.frame_buffer&fb_motionblur))
            {
               if (cur_fb->width == gDP.colorImage.width)
                  memcpy(gfx_info.RDRAM+cur_fb->addr,
                        gfx_info.RDRAM+rdp.maincimg[1].addr,
                        (cur_fb->width*cur_fb->height)<<cur_fb->size>>1);
               //rdp.skip_drawing = true;
            }
            else
               memset(gfx_info.RDRAM+cur_fb->addr, 0, (cur_fb->width*cur_fb->height) << g_gdp.fb_size >> 1);
            break;
            /*
               else if (rdp.frame_buffers[rdp.ci_count].status == ci_main_i)
               {
            // CopyFrameBuffer ();
            rdp.scale_x = rdp.scale_x_bak;
            rdp.scale_y = rdp.scale_y_bak;
            rdp.skip_drawing = false;
            }
            */
         case CI_AUX:
            rdp.skip_drawing = true;
            if (cur_fb->format == 0)
            {
               rdp.skip_drawing = false;
               if (rdp.ci_count == 0)
               {
                  // if (rdp.num_of_ci > 1)
                  // {
                  rdp.scale_x = 1.0f;
                  rdp.scale_y = 1.0f;
                  // }
               }
               else if (
                     (prev_fb->status == CI_MAIN) &&
                     (prev_fb->width == cur_fb->width)) // for Pokemon Stadium
                  CopyFrameBuffer (GR_BUFFER_BACKBUFFER);
            }
            cur_fb->status = CI_AUX;
            break;
         case CI_ZIMG:
            rdp.skip_drawing = true;
            break;
         case CI_ZCOPY:
            if (settings.ucode != ucode_PerfectDark)
               rdp.skip_drawing = true;
            break;
         case CI_USELESS:
            rdp.skip_drawing = true;
            break;
         case CI_COPY_SELF:
            rdp.skip_drawing = false;
            break;
         default:
            rdp.skip_drawing = false;
      }

      if ((rdp.ci_count > 0) && (prev_fb->status >= CI_AUX)) //for Pokemon Stadium
      {
         if (
               prev_fb->format == 0)
            CopyFrameBuffer (GR_BUFFER_BACKBUFFER);
         else if ((settings.hacks&hack_Knockout) && prev_fb->width < 100)
            CopyFrameBuffer (GR_BUFFER_TEXTUREBUFFER_EXT);
      }
      if (cur_fb->status == CI_COPY)
      {
         if (!rdp.motionblur && (rdp.num_of_ci > rdp.ci_count+1) && (next_fb->status != CI_AUX))
            RestoreScale();
      }

      if (cur_fb->status == CI_AUX)
      {
         if (cur_fb->format == 0)
         {
            if ((settings.hacks & hack_PPL) && (rdp.scale_x < 1.1f))  //need to put current image back to frame buffer
            {
               unsigned x, y;
               uint16_t c;
               int width         = cur_fb->width;
               int height        = cur_fb->height;
               uint16_t *ptr_dst = calloc(width * height, sizeof(uint16_t));
               uint16_t *ptr_src = (uint16_t*)(gfx_info.RDRAM + cur_fb->addr);

               for (y = 0; y < height; y++)
               {
                  for (x = 0; x < width; x++)
                  {
                     c = ((ptr_src[(x + y * width) ^ 1]) >> 1) | 0x8000;
                     ptr_dst[x + y * width] = c;
                  }
               }
               grLfbWriteRegion(GR_BUFFER_BACKBUFFER,
                     (uint32_t)rdp.offset_x,
                     (uint32_t)rdp.offset_y,
                     GR_LFB_SRC_FMT_555,
                     width,
                     height,
                     FXFALSE,
                     width << 1,
                     ptr_dst);
               free(ptr_dst);
            }
         }
      }

      if ((cur_fb->status == CI_MAIN) && (rdp.ci_count > 0))
      {
         int i;
         int to_org_res = true;

         for (i = rdp.ci_count + 1; i < rdp.num_of_ci; i++)
         {
            if ((rdp.frame_buffers[i].status != CI_MAIN) && (rdp.frame_buffers[i].status != CI_ZIMG) && (rdp.frame_buffers[i].status != CI_ZCOPY))
            {
               to_org_res = false;
               break;
            }
         }

         if (to_org_res)
         {
            LRDP("return to original scale\n");
            rdp.scale_x = rdp.scale_x_bak;
            rdp.scale_y = rdp.scale_y_bak;
         }
      }
      rdp.ci_status = cur_fb->status;
      rdp.ci_count++;
   }

   rdp.ocimg              = gDP.colorImage.address;
   gDP.colorImage.address = RSP_SegmentToPhysical(w1);
   gDP.colorImage.width           = (w0 & 0xFFF) + 1;

   if (fb_emulation_enabled && rdp.ci_count > 0)
      gDP.colorImage.height = rdp.frame_buffers[rdp.ci_count-1].height;
   else if (gDP.colorImage.width == 32)
      gDP.colorImage.height = 32;
   else
      gDP.colorImage.height = g_gdp.__clip.yl;

   if (g_gdp.zb_address == gDP.colorImage.address)
   {
      rdp.zi_width = gDP.colorImage.width;
      // int zi_height = MIN((int)rdp.zi_width*3/4, (int)rdp.vi_height);
      // rdp.zi_words = rdp.zi_width * zi_height;
   }
   rdp.ci_end = gDP.colorImage.address + ((gDP.colorImage.width * gDP.colorImage.height) << (g_gdp.fb_size-1));

   if (g_gdp.fb_format != G_IM_FMT_RGBA) //can't draw into non RGBA buffer
   {
      if (g_gdp.fb_format > 2)
         rdp.skip_drawing = true;
      return;
   }
   else
   {
      if (!fb_emulation_enabled)
         rdp.skip_drawing = false;
   }

   CI_SET = true;

   if (settings.swapmode > 0)
   {
      int viSwapOK;

      if (g_gdp.zb_address == gDP.colorImage.address)
         rdp.updatescreen = 1;

      viSwapOK = ((settings.swapmode == 2) && (rdp.vi_org_reg == *gfx_info.VI_ORIGIN_REG)) ? false : true;
      if ((g_gdp.zb_address != gDP.colorImage.address) && (rdp.ocimg != gDP.colorImage.address) && SwapOK && viSwapOK
            )
      {
         if (fb_emulation_enabled)
            rdp.maincimg[0] = rdp.frame_buffers[rdp.main_ci_index];
         else
            rdp.maincimg[0].addr = gDP.colorImage.address;
         rdp.last_drawn_ci_addr = (settings.swapmode == 2) ? swapped_addr : rdp.maincimg[0].addr;
         swapped_addr = gDP.colorImage.address;
         newSwapBuffers();
         rdp.vi_org_reg = *gfx_info.VI_ORIGIN_REG;
         SwapOK = false;
      }
   }
}

static void rsp_uc5_reserved0(uint32_t w0, uint32_t w1)
{
   glide64gSPSetDMATexOffset(w1);
}

/******************************************************************
Function: FrameBufferRead
Purpose:  This function is called to notify the dll that the
frame buffer memory is beening read at the given address.
DLL should copy content from its render buffer to the frame buffer
in N64 RDRAM
DLL is responsible to maintain its own frame buffer memory addr list
DLL should copy 4KB block content back to RDRAM frame buffer.
Emulator should not call this function again if other memory
is read within the same 4KB range
input:    addr          rdram address
val                     val
size            1 = uint8_t, 2 = uint16_t, 4 = uint32_t
output:   none
*******************************************************************/

void glide64FBRead(uint32_t addr)
{
  uint32_t a;
  if (cpu_fb_ignore)
    return;
  if (cpu_fb_write_called)
  {
    cpu_fb_ignore = true;
    cpu_fb_write = false;
    return;
  }
  cpu_fb_read_called = true;
  a = RSP_SegmentToPhysical(addr);

  if (!rdp.fb_drawn && (a >= gDP.colorImage.address) && (a < rdp.ci_end))
  {
    fbreads_back++;
    //if (fbreads_back > 2) //&& (gDP.colorImage.width <= 320))
    {
       CopyFrameBuffer (GR_BUFFER_BACKBUFFER);
      rdp.fb_drawn = true;
    }
  }
  if (!rdp.fb_drawn_front && (a >= rdp.maincimg[1].addr) && (a < rdp.maincimg[1].addr + gDP.colorImage.width * gDP.colorImage.height*2))
  {
    fbreads_front++;
    //if (fbreads_front > 2)//&& (gDP.colorImage.width <= 320))
    {
      uint32_t cimg = gDP.colorImage.address;
      gDP.colorImage.address = rdp.maincimg[1].addr;
      if (fb_emulation_enabled)
      {
        uint32_t h;
        gDP.colorImage.width = rdp.maincimg[1].width;
        rdp.ci_count = 0;
        h = rdp.frame_buffers[0].height;
        rdp.frame_buffers[0].height = rdp.maincimg[1].height;
        CopyFrameBuffer(GR_BUFFER_FRONTBUFFER);
        rdp.frame_buffers[0].height = h;
      }
      else
      {
        CopyFrameBuffer(GR_BUFFER_FRONTBUFFER);
      }
      gDP.colorImage.address = cimg;
      rdp.fb_drawn_front     = true;
    }
  }
}


/******************************************************************
Function: FrameBufferWrite
Purpose:  This function is called to notify the dll that the
frame buffer has been modified by CPU at the given address.
input:    addr          rdram address
val                     val
size            1 = uint8_t, 2 = uint16_t, 4 = uint32_t
output:   none
*******************************************************************/
void glide64FBWrite(uint32_t addr, uint32_t size)
{
  uint32_t a, shift_l, shift_r;

  if (cpu_fb_ignore)
    return;

  if (cpu_fb_read_called)
  {
    cpu_fb_ignore = true;
    cpu_fb_write  = false;
    return;
  }

  cpu_fb_write_called = true;
  a                   = RSP_SegmentToPhysical(addr);

  if (a < gDP.colorImage.address || a > rdp.ci_end)
    return;

  cpu_fb_write = true;
  shift_l      = (a - gDP.colorImage.address) >> 1;
  shift_r      = shift_l+2;

  part_framebuf.d_ul_x       = MIN(part_framebuf.d_ul_x, shift_l % gDP.colorImage.width);
  part_framebuf.d_ul_y       = MIN(part_framebuf.d_ul_y, shift_l / gDP.colorImage.width);
  part_framebuf.d_lr_x       = MAX(part_framebuf.d_lr_x, shift_r % gDP.colorImage.width);
  part_framebuf.d_lr_y       = MAX(part_framebuf.d_lr_y, shift_r / gDP.colorImage.width);
}


/************************************************************************
Function: glide64FBGetFrameBufferInfo
Purpose:  This function is called by the emulator core to retrieve frame
buffer information from the video plugin in order to be able
to notify the video plugin about CPU frame buffer read/write
operations

size:
= 1           byte
= 2           word (16 bit) <-- this is N64 default depth buffer format
= 4           dword (32 bit)

when frame buffer information is not available yet, set all values
in the FrameBufferInfo structure to 0

input:    FrameBufferInfo pinfo[6]
pinfo is pointed to a FrameBufferInfo structure which to be
filled in by this function
output:   Values are return in the FrameBufferInfo structure
Plugin can return up to 6 frame buffer info
************************************************************************/
///*
void glide64FBGetFrameBufferInfo(void *p)
{
   int i;
   FrameBufferInfo * pinfo = (FrameBufferInfo *)p;

   pinfo->addr   = 0;
   pinfo->size   = 0;
   pinfo->width  = 0;
   pinfo->height = 0;

   if (!(settings.frame_buffer&fb_get_info))
      return;

   if (fb_emulation_enabled)
   {
      int info_index;
      pinfo[0].addr   = rdp.maincimg[1].addr;
      pinfo[0].size   = rdp.maincimg[1].size;
      pinfo[0].width  = rdp.maincimg[1].width;
      pinfo[0].height = rdp.maincimg[1].height;
      info_index      = 1;

      for (i = 0; i < rdp.num_of_ci && info_index < 6; i++)
      {
         COLOR_IMAGE *cur_fb = (COLOR_IMAGE*)&rdp.frame_buffers[i];

         if (
                  cur_fb->status == CI_MAIN
               || cur_fb->status == CI_COPY_SELF
               || cur_fb->status == CI_OLD_COPY
            )
         {
            pinfo[info_index].addr   = cur_fb->addr;
            pinfo[info_index].size   = cur_fb->size;
            pinfo[info_index].width  = cur_fb->width;
            pinfo[info_index].height = cur_fb->height;
            info_index++;
         }
      }
   }
   else
   {
      pinfo[0].addr   = rdp.maincimg[0].addr;
      pinfo[0].size   = g_gdp.fb_size;
      pinfo[0].width  = gDP.colorImage.width;
      pinfo[0].height = gDP.colorImage.width*3/4;
      pinfo[1].addr   = rdp.maincimg[1].addr;
      pinfo[1].size   = g_gdp.fb_size;
      pinfo[1].width  = gDP.colorImage.width;
      pinfo[1].height = gDP.colorImage.width*3/4;
   }
}

#include "ucodeFB.h"

void DetectFrameBufferUsage(void)
{
   uint32_t ci, zi;
   int i, previous_ci_was_read, all_zimg;
   uint32_t dlist_start = *(uint32_t*)(gfx_info.DMEM+0xFF0);

   /* Do nothing if dlist is empty */
   if (dlist_start == 0)
      return;

   ci                   = gDP.colorImage.address;
   zi                   = g_gdp.zb_address;

   rdp.main_ci_last_tex_addr = 0;
   rdp.main_ci          = 0;
   rdp.main_ci_end      = 0;
   rdp.main_ci_bg       = 0;
   rdp.ci_count         = 0;
   rdp.main_ci_index    = 0;
   rdp.copy_ci_index    = 0;
   rdp.copy_zi_index    = 0;
   rdp.zimg_end         = 0;
   rdp.tmpzimg          = 0;
   rdp.motionblur       = false;
   previous_ci_was_read = rdp.read_previous_ci;
   rdp.read_previous_ci = false;
   rdp.read_whole_frame = false;
   rdp.swap_ci_index    = rdp.black_ci_index = -1;
   SwapOK               = true;

   // Start executing at the start of the display list
   __RSP.PCi            = 0;
   __RSP.PC[__RSP.PCi]  = dlist_start;
   __RSP.count          = -1;
   __RSP.halt           = 0;
   rdp.scale_x_bak      = rdp.scale_x;
   rdp.scale_y_bak      = rdp.scale_y;

   do
   {
      /* Get the address of the next command */
      uint32_t a = __RSP.PC[__RSP.PCi] & BMASK;

      /* Load the next command and its input */
      __RSP.w0   = ((uint32_t*)gfx_info.RDRAM)[a>>2];   // \ Current command, 64 bit
      __RSP.w1   = ((uint32_t*)gfx_info.RDRAM)[(a>>2)+1]; // /

      // Output the address before the command

      // Go to the next instruction
      __RSP.PC[__RSP.PCi] = (a+8) & BMASK;

      if ((uintptr_t)((void*)(gfx_instruction_lite[settings.ucode][__RSP.w0 >> 24])))
         gfx_instruction_lite[settings.ucode][__RSP.w0 >> 24](__RSP.w0, __RSP.w1);

      RSP_CheckDLCounter();
   }while (!__RSP.halt);

   SwapOK = true;

   if (rdp.ci_count > NUMTEXBUF) //overflow
   {
      gDP.colorImage.address = ci;
      g_gdp.zb_address       = zi;
      rdp.num_of_ci          = rdp.ci_count;
      rdp.scale_x            = rdp.scale_x_bak;
      rdp.scale_y            = rdp.scale_y_bak;
      return;
   }

   if (rdp.black_ci_index > 0 && rdp.black_ci_index < rdp.copy_ci_index)
      rdp.frame_buffers[rdp.black_ci_index].status = CI_MAIN;

   if ((rdp.ci_count-1) >= 0)
   {
      if (rdp.frame_buffers[rdp.ci_count-1].status == CI_UNKNOWN)
      {
         if (rdp.ci_count > 1)
            rdp.frame_buffers[rdp.ci_count-1].status = CI_AUX;
         else
            rdp.frame_buffers[rdp.ci_count-1].status = CI_MAIN;
      }
   }

   if ((rdp.ci_count > 0 && rdp.frame_buffers[rdp.ci_count-1].status == CI_AUX) &&
         (rdp.frame_buffers[rdp.main_ci_index].width < 320) &&
         (rdp.frame_buffers[rdp.ci_count-1].width > rdp.frame_buffers[rdp.main_ci_index].width))
   {
      for (i = 0; i < rdp.ci_count; i++)
      {
         if (rdp.frame_buffers[i].status == CI_MAIN)
            rdp.frame_buffers[i].status = CI_AUX;
         else if (rdp.frame_buffers[i].addr == rdp.frame_buffers[rdp.ci_count-1].addr)
            rdp.frame_buffers[i].status = CI_MAIN;
         //                        FRDP("rdp.frame_buffers[%d].status = %d\n", i, rdp.frame_buffers[i].status);
      }
      rdp.main_ci_index = rdp.ci_count-1;
   }

   all_zimg = true;
   for (i = 0; i < rdp.ci_count; i++)
   {
      if (rdp.frame_buffers[i].status != CI_ZIMG)
      {
         all_zimg = false;
         break;
      }
   }
   if (all_zimg)
   {
      for (i = 0; i < rdp.ci_count; i++)
         rdp.frame_buffers[i].status = CI_MAIN;
   }

#ifndef NDEBUG
   LRDP("detect fb final results: \n");
   for (i = 0; i < rdp.ci_count; i++)
   {
      FRDP("rdp.frame_buffers[%d].status = %s, addr: %08lx, height: %d\n", i, CIStatus[rdp.frame_buffers[i].status], rdp.frame_buffers[i].addr, rdp.frame_buffers[i].height);
   }
#endif

   gDP.colorImage.address = ci;
   g_gdp.zb_address       = zi;
   rdp.num_of_ci          = rdp.ci_count;

   if (rdp.read_previous_ci && previous_ci_was_read)
   {
      if (!fb_hwfbe_enabled || !rdp.copy_ci_index)
         rdp.motionblur = true;
   }
   if (rdp.motionblur || fb_hwfbe_enabled || (rdp.frame_buffers[rdp.copy_ci_index].status == CI_AUX_COPY))
   {
      rdp.scale_x = rdp.scale_x_bak;
      rdp.scale_y = rdp.scale_y_bak;
   }

   if ((rdp.read_previous_ci || previous_ci_was_read) && !rdp.copy_ci_index)
      rdp.read_whole_frame = true;
   if (rdp.read_whole_frame)
   {
      {
         if (rdp.motionblur)
         {
            if (settings.frame_buffer&fb_motionblur)
               CopyFrameBuffer (GR_BUFFER_BACKBUFFER);
            else
               memset(gfx_info.RDRAM + gDP.colorImage.address,
                     0,
                     gDP.colorImage.width * gDP.colorImage.height * g_gdp.fb_size);
         }
         else //if (ci_width == rdp.frame_buffers[rdp.main_ci_index].width)
         {
            if (rdp.maincimg[0].height > 65) //for 1080
            {
               uint32_t h;
               gDP.colorImage.address      = rdp.maincimg[0].addr;
               gDP.colorImage.width        = rdp.maincimg[0].width;
               rdp.ci_count                = 0;
               h                           = rdp.frame_buffers[0].height;
               rdp.frame_buffers[0].height = rdp.maincimg[0].height;

               CopyFrameBuffer (GR_BUFFER_BACKBUFFER);
               rdp.frame_buffers[0].height = h;
            }
            else //conker
            {
               CopyFrameBuffer (GR_BUFFER_BACKBUFFER);
            }
         }
      }
   }

   rdp.ci_count    = 0;
   rdp.maincimg[0] = rdp.frame_buffers[rdp.main_ci_index];
   //rdp.scale_x = rdp.scale_x_bak;
   //rdp.scale_y = rdp.scale_y_bak;
}

static void rdp_trifill(uint32_t w0, uint32_t w1)
{
   glide64gDPTriangle(w0, w1, 0, 0, 0);
}

static void rdp_trishade(uint32_t w0, uint32_t w1)
{
   glide64gDPTriangle(w0, w1, 1, 0, 0);
}

static void rdp_tritxtr(uint32_t w0, uint32_t w1)
{
   glide64gDPTriangle(w0, w1, 0, 1, 0);
}

static void rdp_trishadetxtr(uint32_t w0, uint32_t w1)
{
   glide64gDPTriangle(w0, w1, 1, 1, 0);
}

static void rdp_trifillz(uint32_t w0, uint32_t w1)
{
   glide64gDPTriangle(w0, w1, 0, 0, 1);
}

static void rdp_trishadez(uint32_t w0, uint32_t w1)
{
   glide64gDPTriangle(w0, w1, 1, 0, 1);
}

static void rdp_tritxtrz(uint32_t w0, uint32_t w1)
{
   glide64gDPTriangle(w0, w1, 0, 1, 1);
}

static void rdp_trishadetxtrz(uint32_t w0, uint32_t w1)
{
   glide64gDPTriangle(w0, w1, 1, 1, 1);
}


static rdp_instr rdp_command_table[64] =
{
   /* 0x00 */
   gdp_no_op,          gdp_invalid,                  gdp_invalid,                  gdp_invalid,
   gdp_invalid,              gdp_invalid,                  gdp_invalid,                  gdp_invalid,
   rdp_trifill,        rdp_trifillz,           rdp_tritxtr,            rdp_tritxtrz,
   rdp_trishade,       rdp_trishadez,          rdp_trishadetxtr,       rdp_trishadetxtrz,
   /* 0x10 */
   gdp_invalid,              gdp_invalid,                  gdp_invalid,                  gdp_invalid,
   gdp_invalid,              gdp_invalid,                  gdp_invalid,                  gdp_invalid,
   gdp_invalid,              gdp_invalid,                  gdp_invalid,                  gdp_invalid,
   gdp_invalid,              gdp_invalid,                  gdp_invalid,                  gdp_invalid,
   /* 0x20 */
   gdp_invalid,              gdp_invalid,                  gdp_invalid,                  gdp_invalid,
   rdp_texrect,        rdp_texrect,            gdp_load_sync,           gdp_pipe_sync,
   gdp_tile_sync,       gdp_full_sync,           gdp_set_key_gb,           gdp_set_key_r,
   gdp_set_convert,    rdp_setscissor,         gdp_set_prim_depth,       rdp_setothermode,
   /* 0x30 */
   rdp_loadtlut,           gdp_invalid,                  rdp_settilesize,        rdp_loadblock,
   rdp_loadtile,           rdp_settile,            rdp_fillrect,           gdp_set_fill_color,
   gdp_set_fog_color,      gdp_set_blend_color,      rdp_setprimcolor,       gdp_set_env_color,
   rdp_setcombine,         rdp_settextureimage,    rdp_setdepthimage,      rdp_setcolorimage
};

static const uint32_t rdp_command_length[64] =
{
   8,                      // 0x00, No Op
   8,                      // 0x01, ???
   8,                      // 0x02, ???
   8,                      // 0x03, ???
   8,                      // 0x04, ???
   8,                      // 0x05, ???
   8,                      // 0x06, ???
   8,                      // 0x07, ???
   32,                     // 0x08, Non-Shaded Triangle
   32+16,                  // 0x09, Non-Shaded, Z-Buffered Triangle
   32+64,                  // 0x0a, Textured Triangle
   32+64+16,               // 0x0b, Textured, Z-Buffered Triangle
   32+64,                  // 0x0c, Shaded Triangle
   32+64+16,               // 0x0d, Shaded, Z-Buffered Triangle
   32+64+64,               // 0x0e, Shaded+Textured Triangle
   32+64+64+16,            // 0x0f, Shaded+Textured, Z-Buffered Triangle
   8,                      // 0x10, ???
   8,                      // 0x11, ???
   8,                      // 0x12, ???
   8,                      // 0x13, ???
   8,                      // 0x14, ???
   8,                      // 0x15, ???
   8,                      // 0x16, ???
   8,                      // 0x17, ???
   8,                      // 0x18, ???
   8,                      // 0x19, ???
   8,                      // 0x1a, ???
   8,                      // 0x1b, ???
   8,                      // 0x1c, ???
   8,                      // 0x1d, ???
   8,                      // 0x1e, ???
   8,                      // 0x1f, ???
   8,                      // 0x20, ???
   8,                      // 0x21, ???
   8,                      // 0x22, ???
   8,                      // 0x23, ???
   16,                     // 0x24, Texture_Rectangle
   16,                     // 0x25, Texture_Rectangle_Flip
   8,                      // 0x26, Sync_Load
   8,                      // 0x27, Sync_Pipe
   8,                      // 0x28, Sync_Tile
   8,                      // 0x29, Sync_Full
   8,                      // 0x2a, Set_Key_GB
   8,                      // 0x2b, Set_Key_R
   8,                      // 0x2c, Set_Convert
   8,                      // 0x2d, Set_Scissor
   8,                      // 0x2e, Set_Prim_Depth
   8,                      // 0x2f, Set_Other_Modes
   8,                      // 0x30, Load_TLUT
   8,                      // 0x31, ???
   8,                      // 0x32, Set_Tile_Size
   8,                      // 0x33, Load_Block
   8,                      // 0x34, Load_Tile
   8,                      // 0x35, Set_Tile
   8,                      // 0x36, Fill_Rectangle
   8,                      // 0x37, Set_Fill_Color
   8,                      // 0x38, Set_Fog_Color
   8,                      // 0x39, Set_Blend_Color
   8,                      // 0x3a, Set_Prim_Color
   8,                      // 0x3b, Set_Env_Color
   8,                      // 0x3c, Set_Combine
   8,                      // 0x3d, Set_Texture_Image
   8,                      // 0x3e, Set_Mask_Image
   8                       // 0x3f, Set_Color_Image
};

static void rdphalf_1(uint32_t w0, uint32_t w1)
{
   if (RDP_Half1(w1))
      rdp_command_table[__RSP.cmd](__RSP.w0, __RSP.w1);
}

static void rdphalf_2(uint32_t w0, uint32_t w1)
{
}

static void rdphalf_cont(uint32_t w0, uint32_t w1)
{
}

static void glide64ProcessRDPList_restorestate(void)
{
   no_dlist                       = false;
   update_screen_count            = 0;
   ChangeSize();

   /* Set states */
   if (settings.swapmode > 0)
      SwapOK                      = true;
   rdp.updatescreen               = 1;

   gSP.matrix.modelViewi          = 0; /* 0 matrices so far in stack */

   /* stack_size can be less then 32! Important for Silicon Vally. Thanks Orkin! */
   gSP.matrix.stackSize           = MIN(32, (*(uint32_t*)(gfx_info.DMEM+0x0FE4))>>6);

   if (gSP.matrix.stackSize == 0)
      gSP.matrix.stackSize        = 32;

   rdp.fb_drawn                   = false;
   rdp.fb_drawn_front             = false;
   g_gdp.flags                    = 0x7FFFFFFF;  /* All but clear cache */
   gSP.geometryMode               = 0;
   rdp.maincimg[1]                = rdp.maincimg[0];
   rdp.skip_drawing               = false;
   rdp.s2dex_tex_loaded           = false;
   rdp.bg_image_height            = 0xFFFF;
   fbreads_front = fbreads_back   = 0;
   gSP.fog.multiplier             = 0;
   gSP.fog.offset                 = 0;

   if (rdp.vi_org_reg != *gfx_info.VI_ORIGIN_REG)		
      rdp.tlut_mode                     = 0; /* is it correct? */

   rdp.scissor_set      = false;
   gSP.DMAOffsets.tex_offset  = 0;
   gSP.DMAOffsets.tex_count   = 0;
   cpu_fb_write         = false;
   cpu_fb_read_called   = false;
   cpu_fb_write_called  = false;
   cpu_fb_ignore        = false;
   part_framebuf.d_ul_x = 0xffff;
   part_framebuf.d_ul_y = 0xffff;
   part_framebuf.d_lr_x = 0;
   part_framebuf.d_lr_y = 0;
   depth_buffer_fog     = true;
}

static void glide64ProcessRDPList_internal(void)
{
   uint32_t i;
   bool set_zero                  = true;
   const uint32_t length          = (*(uint32_t*)gfx_info.DPC_END_REG) - (*(uint32_t*)gfx_info.DPC_CURRENT_REG);

   (*(uint32_t*)gfx_info.DPC_STATUS_REG) &= ~0x0002;

   __RSP.bLLE = true;

   // load command data
   for (i = 0; i < length; i += 4)
   {
      __RDP.cmd_data[__RDP.cmd_ptr] = rdp_read_data((*(uint32_t*)gfx_info.DPC_CURRENT_REG) + i);
      __RDP.cmd_ptr = (__RDP.cmd_ptr + 1) & MAXCMD_MASK;
   }

   /* Load command data */
   while (__RDP.cmd_cur != __RDP.cmd_ptr)
   {
      uint32_t w0, w1;
      uint32_t cmd = (__RDP.cmd_data[__RDP.cmd_cur] >> 24) & 0x3f;

      if ((((__RDP.cmd_ptr - __RDP.cmd_cur)& MAXCMD_MASK) * 4) < rdp_command_length[cmd])
      {
         set_zero = false;
         break;
      }

      if (__RDP.cmd_cur + rdp_command_length[cmd] / 4 > MAXCMD)
         memcpy(__RDP.cmd_data + MAXCMD, __RDP.cmd_data,
               rdp_command_length[cmd] - (MAXCMD - __RDP.cmd_cur) * 4);

      /* execute the command */
      __RSP.w0 = __RDP.cmd_data[__RDP.cmd_cur + 0];
      __RSP.w1 = __RDP.cmd_data[__RDP.cmd_cur + 1];
      __RDP.w2 = __RDP.cmd_data[__RDP.cmd_cur + 2];
      __RDP.w3 = __RDP.cmd_data[__RDP.cmd_cur + 3];

      w0       = __RSP.w0;
      w1       = __RSP.w1;

      rdp_command_table[cmd](w0, w1);

      __RDP.cmd_cur = (__RDP.cmd_cur + rdp_command_length[cmd] / 4) & MAXCMD_MASK;
   }

   if (set_zero)
   {
      __RDP.cmd_cur = 0;
      __RDP.cmd_ptr = 0;
   }

   __RSP.bLLE = false;

   (*(uint32_t*)gfx_info.DPC_START_REG) = (*(uint32_t*)gfx_info.DPC_CURRENT_REG) = (*(uint32_t*)gfx_info.DPC_END_REG);
}

/******************************************************************
Function: ProcessRDPList
Purpose:  This function is called when there is a Dlist to be
processed. (Low level GFX list)
input:    none
output:   none
*******************************************************************/
void glide64ProcessRDPList(void)
{
   glide64ProcessRDPList_restorestate();
   glide64ProcessRDPList_internal();
}
