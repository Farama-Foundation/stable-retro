#include <stdint.h>

#include "glide64_gDP.h"
#include "Combine.h"
#include "Util.h"

#include "../../Graphics/RDP/gDP_state.h"

void apply_shading(void *data);

// STANDARD DRAWIMAGE - draws a 2d image based on the following structure

float set_sprite_combine_mode(void)
{
  float Z;
  if (((gDP.otherMode.h & RDP_CYCLE_TYPE) >> 20) == G_CYC_COPY)
  {
    int32_t color_source;

    rdp.tex           = 1;
    rdp.allow_combine = 0;

    /* Now actually combine ! */
    color_source      = GR_COMBINE_FUNCTION_LOCAL;
    cmb.tmu1_func     = cmb.tmu0_func = color_source;
    cmb.tmu1_fac      = cmb.tmu0_fac = GR_COMBINE_FACTOR_NONE;
    cmb.tmu1_a_func   = cmb.tmu0_a_func = GR_COMBINE_FUNCTION_LOCAL;
    cmb.tmu1_a_fac    = cmb.tmu0_a_fac = GR_COMBINE_FACTOR_NONE;
    cmb.tmu1_invert   = cmb.tmu0_invert = FXFALSE;
    cmb.tmu1_a_invert = cmb.tmu0_a_invert = FXFALSE;
  }

  g_gdp.flags |= UPDATE_COMBINE;
  update ();

  rdp.allow_combine = 1;

  // set z buffer mode
  Z = 0.0f;
  if ((gDP.otherMode.l & 0x00000030) && (((gDP.otherMode.h & RDP_CYCLE_TYPE) >> 20) < G_CYC_COPY))
  {
    if (g_gdp.other_modes.z_source_sel == G_ZS_PRIM)
      Z = g_gdp.prim_color.z;
    FRDP ("prim_depth = %d, prim_dz = %d\n", g_gdp.prim_color.z, g_gdp.prim_color.dz);
    Z = ScaleZ(Z);

    if (gDP.otherMode.l & 0x00000400)
      grDepthBiasLevel(g_gdp.prim_color.dz);
  }

  grCullMode (GR_CULL_DISABLE);
  grFogMode (GR_FOG_DISABLE, g_gdp.fog_color.total);
  g_gdp.flags |= UPDATE_CULL_MODE | UPDATE_FOG_ENABLED;

  if (((gDP.otherMode.h & RDP_CYCLE_TYPE) >> 20) == G_CYC_COPY)
  {
    grColorCombine (GR_COMBINE_FUNCTION_SCALE_OTHER,
      GR_COMBINE_FACTOR_ONE,
      GR_COMBINE_LOCAL_NONE,
      GR_COMBINE_OTHER_TEXTURE,
      FXFALSE);
    grAlphaCombine (GR_COMBINE_FUNCTION_SCALE_OTHER,
      GR_COMBINE_FACTOR_ONE,
      GR_COMBINE_LOCAL_NONE,
      GR_COMBINE_OTHER_TEXTURE,
      FXFALSE);
    grAlphaBlendFunction (GR_BLEND_ONE,
      GR_BLEND_ZERO,
      GR_BLEND_ZERO,
      GR_BLEND_ZERO);
    grAlphaTestFunction ((gDP.otherMode.l & 1) ? GR_CMP_GEQUAL : GR_CMP_ALWAYS, 0x80, (gDP.otherMode.l & 1) ? 1 : 0);
    g_gdp.flags |= UPDATE_ALPHA_COMPARE | UPDATE_COMBINE;
  }
  return Z;
}

#define XSCALE(x) ((float)(x)/(1<<18))
#define YSCALE(y) ((float)(y)/(1<<2))
#define ZSCALE(z) ((g_gdp.other_modes.z_source_sel == G_ZS_PRIM) ? (float)(g_gdp.prim_color.z) : (float)((uint32_t)(z))/0xffff0000)
#define PERSP_EN  ((gDP.otherMode.texturePersp))
#define WSCALE(z) 1.0f/(PERSP_EN? ((float)((uint32_t)(z) + 0x10000)/0xffff0000) : 1.0f)
#define CSCALE(c) (((c)>0x3ff0000? 0x3ff0000:((c)<0? 0 : (c)))>>18)
#define _PERSP(w) ( w )
#define PERSP(s, w) ( ((int64_t)(s) << 20) / (_PERSP(w)? _PERSP(w):1) )
#define SSCALE(s, _w) (PERSP_EN ? (float)(PERSP(s, _w))/(1 << 10) : (float)(s)/(1<<21))
#define TSCALE(s, w)  (PERSP_EN ? (float)(PERSP(s, w))/(1 << 10) : (float)(s)/(1<<21))

static void glide64gDPLLETriangle(uint32_t w0, uint32_t w1, int shade, int texture, int zbuffer, uint32_t *rdp_cmd)
{
   int j;
   int xleft, xright, xleft_inc, xright_inc;
   VERTEX vtxbuf[12], *vtx;

   int32_t vn;
   int32_t yl, ym, yh;          /* triangle edge y-coordinates */
   int32_t xl, xh, xm;          /* triangle edge x-coordinates */
   int32_t dxldy, dxhdy, dxmdy; /* triangle edge inverse-slopes */

   uint32_t w2, w3, w4, w5, w6, w7;
   int r = 0xff;
   int g = 0xff;
   int b = 0xff;
   int a = 0xff;
   int z = 0xffff0000;
   int s = 0;
   int t = 0;
   int w = 0x30000;
   int drdx = 0, dgdx = 0, dbdx = 0, dadx = 0, dzdx = 0, dsdx = 0, dtdx = 0, dwdx = 0;
   int drde = 0, dgde = 0, dbde = 0, dade = 0, dzde = 0, dsde = 0, dtde = 0, dwde = 0;
   int flip = (w0 & 0x800000) ? 1 : 0;
   uint32_t * shade_base = rdp_cmd + 8;
   uint32_t * texture_base = rdp_cmd + 8;
   uint32_t * zbuffer_base = rdp_cmd + 8;

   gDP.otherMode.textureDetail = _SHIFTR(w1, 19, 3);

   if (shade)
   {
      texture_base += 16;
      zbuffer_base += 16;
   }
   if (texture)
   {
      zbuffer_base += 16;
   }

   w2 = rdp_cmd[2];
   w3 = rdp_cmd[3];
   w4 = rdp_cmd[4];
   w5 = rdp_cmd[5];
   w6 = rdp_cmd[6];
   w7 = rdp_cmd[7];

   /* triangle edge y-coordinates */
   yl = (w0 & 0x3fff);
   ym = ((w1 >> 16) & 0x3fff);
   yh = ((w1 >> 0) & 0x3fff);

   /* triangle edge x-coordinates */
   xl = (int32_t)(w2);
   xh = (int32_t)(w4);
   xm = (int32_t)(w6);

   /* triangle edge inverse-slopes */
   dxldy = (int32_t)(w3);
   dxhdy = (int32_t)(w5);
   dxmdy = (int32_t)(w7);

   rdp.cur_tile = (w0 >> 16) & 0x7;


   if (yl & (0x800<<2)) yl |= 0xfffff000<<2;
   if (ym & (0x800<<2)) ym |= 0xfffff000<<2;
   if (yh & (0x800<<2)) yh |= 0xfffff000<<2;

   yh &= ~3;


   if (shade)
   {
      r = (shade_base[0] & 0xffff0000) | ((shade_base[+4 ] >> 16) & 0x0000ffff);
      g = ((shade_base[0 ] << 16) & 0xffff0000) | (shade_base[4 ] & 0x0000ffff);
      b = (shade_base[1 ] & 0xffff0000) | ((shade_base[5 ] >> 16) & 0x0000ffff);
      a = ((shade_base[1 ] << 16) & 0xffff0000) | (shade_base[5 ] & 0x0000ffff);
      drdx = (shade_base[2 ] & 0xffff0000) | ((shade_base[6 ] >> 16) & 0x0000ffff);
      dgdx = ((shade_base[2 ] << 16) & 0xffff0000) | (shade_base[6 ] & 0x0000ffff);
      dbdx = (shade_base[3 ] & 0xffff0000) | ((shade_base[7 ] >> 16) & 0x0000ffff);
      dadx = ((shade_base[3 ] << 16) & 0xffff0000) | (shade_base[7 ] & 0x0000ffff);
      drde = (shade_base[8 ] & 0xffff0000) | ((shade_base[12] >> 16) & 0x0000ffff);
      dgde = ((shade_base[8 ] << 16) & 0xffff0000) | (shade_base[12] & 0x0000ffff);
      dbde = (shade_base[9 ] & 0xffff0000) | ((shade_base[13] >> 16) & 0x0000ffff);
      dade = ((shade_base[9 ] << 16) & 0xffff0000) | (shade_base[13] & 0x0000ffff);
   }
   if (texture)
   {
      s = (texture_base[0 ] & 0xffff0000) | ((texture_base[4 ] >> 16) & 0x0000ffff);
      t = ((texture_base[0 ] << 16) & 0xffff0000) | (texture_base[4 ] & 0x0000ffff);
      w = (texture_base[1 ] & 0xffff0000) | ((texture_base[5 ] >> 16) & 0x0000ffff);
      // w = abs(w);
      dsdx = (texture_base[2 ] & 0xffff0000) | ((texture_base[6 ] >> 16) & 0x0000ffff);
      dtdx = ((texture_base[2 ] << 16) & 0xffff0000) | (texture_base[6 ] & 0x0000ffff);
      dwdx = (texture_base[3 ] & 0xffff0000) | ((texture_base[7 ] >> 16) & 0x0000ffff);
      dsde = (texture_base[8 ] & 0xffff0000) | ((texture_base[12] >> 16) & 0x0000ffff);
      dtde = ((texture_base[8 ] << 16) & 0xffff0000) | (texture_base[12] & 0x0000ffff);
      dwde = (texture_base[9 ] & 0xffff0000) | ((texture_base[13] >> 16) & 0x0000ffff);
   }
   if (zbuffer)
   {
      z = zbuffer_base[0];
      dzdx = zbuffer_base[1];
      dzde = zbuffer_base[2];
   }

   xh <<= 2;
   xm <<= 2;
   xl <<= 2;
   r <<= 2;
   g <<= 2;
   b <<= 2;
   a <<= 2;
   dsde >>= 2;
   dtde >>= 2;
   dsdx >>= 2;
   dtdx >>= 2;
   dzdx >>= 2;
   dzde >>= 2;
   dwdx >>= 2;
   dwde >>= 2;

   vn = 0;
   vtx = (VERTEX*)&vtxbuf[vn++];
   memset(vtxbuf, 0, sizeof(vtxbuf));

   xleft      = xm;
   xright     = xh;
   xleft_inc  = dxmdy;
   xright_inc = dxhdy;

   while (yh<ym &&
         !((!flip && xleft < xright+0x10000) ||
            (flip && xleft > xright-0x10000))) {
      xleft += xleft_inc;
      xright += xright_inc;
      s += dsde;
      t += dtde;
      w += dwde;
      r += drde;
      g += dgde;
      b += dbde;
      a += dade;
      z += dzde;
      yh++;
   }

   j = ym-yh;
   if (j > 0)
   {
      int dx = (xleft-xright)>>16;
      if ((!flip && xleft < xright) ||
            (flip/* && xleft > xright*/))
      {
         if (shade) {
            vtx->r = CSCALE(r+drdx*dx);
            vtx->g = CSCALE(g+dgdx*dx);
            vtx->b = CSCALE(b+dbdx*dx);
            vtx->a = CSCALE(a+dadx*dx);
         }
         if (texture) {
            vtx->ou = SSCALE(s+dsdx*dx, w+dwdx*dx);
            vtx->ov = TSCALE(t+dtdx*dx, w+dwdx*dx);
         }
         vtx->x = XSCALE(xleft);
         vtx->y = YSCALE(yh);
         vtx->z = ZSCALE(z+dzdx*dx);
         vtx->w = WSCALE(w+dwdx*dx);
         vtx = &vtxbuf[vn++];
      }
      if ((!flip/* && xleft < xright*/) ||
            (flip && xleft > xright))
      {
         if (shade) {
            vtx->r = CSCALE(r);
            vtx->g = CSCALE(g);
            vtx->b = CSCALE(b);
            vtx->a = CSCALE(a);
         }
         if (texture) {
            vtx->ou = SSCALE(s, w);
            vtx->ov = TSCALE(t, w);
         }
         vtx->x = XSCALE(xright);
         vtx->y = YSCALE(yh);
         vtx->z = ZSCALE(z);
         vtx->w = WSCALE(w);
         vtx = &vtxbuf[vn++];
      }
      xleft += xleft_inc*j;
      xright += xright_inc*j;
      s += dsde*j;
      t += dtde*j;
      if (w + dwde*j)
         w += dwde*j;
      else
         w += dwde*(j-1);
      r += drde*j;
      g += dgde*j;
      b += dbde*j;
      a += dade*j;
      z += dzde*j;
      // render ...
   }

   if (xl != xh)
      xleft = xl;

   //if (yl-ym > 0)
   {
      int dx = (xleft-xright)>>16;
      if ((!flip && xleft <= xright) ||
            (flip/* && xleft >= xright*/))
      {
         if (shade) {
            vtx->r = CSCALE(r+drdx*dx);
            vtx->g = CSCALE(g+dgdx*dx);
            vtx->b = CSCALE(b+dbdx*dx);
            vtx->a = CSCALE(a+dadx*dx);
         }
         if (texture) {
            vtx->ou = SSCALE(s+dsdx*dx, w+dwdx*dx);
            vtx->ov = TSCALE(t+dtdx*dx, w+dwdx*dx);
         }
         vtx->x = XSCALE(xleft);
         vtx->y = YSCALE(ym);
         vtx->z = ZSCALE(z+dzdx*dx);
         vtx->w = WSCALE(w+dwdx*dx);
         vtx = &vtxbuf[vn++];
      }
      if ((!flip/* && xleft <= xright*/) ||
            (flip && xleft >= xright))
      {
         if (shade) {
            vtx->r = CSCALE(r);
            vtx->g = CSCALE(g);
            vtx->b = CSCALE(b);
            vtx->a = CSCALE(a);
         }
         if (texture) {
            vtx->ou = SSCALE(s, w);
            vtx->ov = TSCALE(t, w);
         }
         vtx->x = XSCALE(xright);
         vtx->y = YSCALE(ym);
         vtx->z = ZSCALE(z);
         vtx->w = WSCALE(w);
         vtx = &vtxbuf[vn++];
      }
   }
   xleft_inc = dxldy;
   xright_inc = dxhdy;

   j = yl-ym;
   //j--; // ?
   xleft += xleft_inc*j;
   xright += xright_inc*j;
   s += dsde*j;
   t += dtde*j;
   w += dwde*j;
   r += drde*j;
   g += dgde*j;
   b += dbde*j;
   a += dade*j;
   z += dzde*j;

   while (yl>ym &&
         !((!flip && xleft < xright+0x10000) ||
            (flip && xleft > xright-0x10000)))
   {
      xleft -= xleft_inc;
      xright -= xright_inc;
      s -= dsde;
      t -= dtde;
      w -= dwde;
      r -= drde;
      g -= dgde;
      b -= dbde;
      a -= dade;
      z -= dzde;
      j--;
      yl--;
   }

   // render ...
   if (j >= 0) {
      int dx = (xleft-xright)>>16;
      if ((!flip && xleft <= xright) ||
            (flip/* && xleft >= xright*/))
      {
         if (shade) {
            vtx->r = CSCALE(r+drdx*dx);
            vtx->g = CSCALE(g+dgdx*dx);
            vtx->b = CSCALE(b+dbdx*dx);
            vtx->a = CSCALE(a+dadx*dx);
         }
         if (texture) {
            vtx->ou = SSCALE(s+dsdx*dx, w+dwdx*dx);
            vtx->ov = TSCALE(t+dtdx*dx, w+dwdx*dx);
         }
         vtx->x = XSCALE(xleft);
         vtx->y = YSCALE(yl);
         vtx->z = ZSCALE(z+dzdx*dx);
         vtx->w = WSCALE(w+dwdx*dx);
         vtx = &vtxbuf[vn++];
      }
      if ((!flip/* && xleft <= xright*/) ||
            (flip && xleft >= xright))
      {
         if (shade) {
            vtx->r = CSCALE(r);
            vtx->g = CSCALE(g);
            vtx->b = CSCALE(b);
            vtx->a = CSCALE(a);
         }
         if (texture) {
            vtx->ou = SSCALE(s, w);
            vtx->ov = TSCALE(t, w);
         }
         vtx->x = XSCALE(xright);
         vtx->y = YSCALE(yl);
         vtx->z = ZSCALE(z);
         vtx->w = WSCALE(w);
         vtx = &vtxbuf[vn++];
      }
   }

   //if (fullscreen)
   {
      int k;

      update ();

      for (k = 0; k < vn-1; k++)
      {
         VERTEX * v = (VERTEX*)&vtxbuf[k];
         v->x       = v->x * rdp.scale_x + rdp.offset_x;
         v->y       = v->y * rdp.scale_y + rdp.offset_y;
         // v->z = 1.0f;///v->w;
         v->q       = 1.0f/v->w;
         v->u[1]    = v->u[0] = v->ou;
         v->v[1]    = v->v[0] = v->ov;

         if (rdp.tex >= 1 && rdp.cur_cache[0])
         {
            draw_tri_uv_calculation_update_shift(rdp.cur_tile, 0, v);
            v->u[0] /= v->w;
            v->v[0] /= v->w;
         }

         if (rdp.tex >= 2 && rdp.cur_cache[1])
         {
            draw_tri_uv_calculation_update_shift(rdp.cur_tile, 1, v);
            v->u[1] /= v->w;
            v->v[1] /= v->w;
         }
         apply_shade_modulation(v);
      }
      grCullMode(GR_CULL_DISABLE);
      ConvertCoordsConvert (vtxbuf, vn);
      grDrawVertexArrayContiguous (GR_TRIANGLE_STRIP, vn-1, vtxbuf);
   }
}

void glide64gDPTriangle(uint32_t w0, uint32_t w1, int shade, int texture, int zbuffer)
{
   glide64gDPLLETriangle(w0, w1, shade, texture, zbuffer, __RDP.cmd_data + __RDP.cmd_cur);
}

void glide64gDPSetTextureImage(int32_t fmt, int32_t siz,
   int32_t width, int32_t addr)
{
   g_gdp.ti_format  = fmt;
   g_gdp.ti_size    = siz;
   g_gdp.ti_address = addr;
   g_gdp.ti_width   = width;
}

void glide64gDPSetTile(
      uint32_t fmt,
      uint32_t siz,
      uint32_t line,
      uint32_t tmem,
      uint32_t tile,
      uint32_t palette,
      uint32_t cmt,
      uint32_t maskt,
      uint32_t shiftt,
      uint32_t cms,
      uint32_t masks,
      uint32_t shifts )
{
   g_gdp.tile[tile].format   = fmt;
   g_gdp.tile[tile].size     = siz;
   g_gdp.tile[tile].line     = line;
   g_gdp.tile[tile].tmem     = tmem;
   g_gdp.tile[tile].palette  = palette;
   g_gdp.tile[tile].ct       = 1;
   g_gdp.tile[tile].mt       = cmt;
   g_gdp.tile[tile].mask_t   = maskt;
   g_gdp.tile[tile].shift_t  = shiftt;
   g_gdp.tile[tile].cs       = 1;
   g_gdp.tile[tile].ms       = cms;
   g_gdp.tile[tile].mask_s   = masks;
   g_gdp.tile[tile].shift_s  = shifts;
}

void glide64gDPSetTileSize(uint32_t tile, uint32_t uls, uint32_t ult,
      uint32_t lrs, uint32_t lrt)
{
   g_gdp.tile[tile].sh = uls;
   g_gdp.tile[tile].th = ult;
   g_gdp.tile[tile].sl = lrs;
   g_gdp.tile[tile].tl = lrt;
}

void glide64gDPSetScissor( uint32_t mode, float ulx, float uly, float lrx, float lry )
{
   g_gdp.__clip.xh = (uint32_t)ulx;
   g_gdp.__clip.yh = (uint32_t)uly;
   g_gdp.__clip.xl = (uint32_t)lrx;
   g_gdp.__clip.yl = (uint32_t)lry;
}

void glide64gDPLoadBlock( uint32_t tile, uint32_t ul_s, uint32_t ul_t,
      uint32_t lr_s, uint32_t dxt )
{
   uint32_t _dxt, addr, off, cnt;
   uint8_t *dst;

   if (rdp.skip_drawing)
      return;

   if (gSP.DMAOffsets.tex_offset)
   {
      if (gSP.DMAOffsets.tex_shift % ((lr_s+1)<<3))
      {
         g_gdp.ti_address -= gSP.DMAOffsets.tex_shift;
         gSP.DMAOffsets.tex_offset = 0;
         gSP.DMAOffsets.tex_shift  = 0;
         gSP.DMAOffsets.tex_count  = 0;
      }
      else
         gSP.DMAOffsets.tex_count++;
   }

   rdp.addr[g_gdp.tile[tile].tmem] = g_gdp.ti_address;

   // ** DXT is used for swapping every other line
   /* double fdxt = (double)0x8000000F/(double)((uint32_t)(2047/(dxt-1))); // F for error
      uint32_t _dxt = (uint32_t)fdxt;*/

   // 0x00000800 -> 0x80000000 (so we can check the sign bit instead of the 11th bit)
   _dxt = dxt << 20;
   addr = RSP_SegmentToPhysical(g_gdp.ti_address);

   g_gdp.tile[tile].sh = ul_s;
   g_gdp.tile[tile].th = ul_t;
   g_gdp.tile[tile].sl = lr_s;

   rdp.timg.set_by = 0; /* load block */

   /* do a quick boundary check before copying 
    * to eliminate the possibility for exception */
   if (ul_s >= 512)
   {
      lr_s = 1; /* 1 so that it doesn't die on memcpy */
      ul_s = 511;
   }
   if (ul_s+lr_s > 512)
      lr_s = 512-ul_s;

   if (addr+(lr_s<<3) > BMASK+1)
      lr_s = (uint16_t)((BMASK-addr)>>3);

   /* angrylion's advice to use ul_s in texture image offset 
    * and cnt calculations.
    *
    * Helps to fix Vigilante 8 JPEG backgrounds and logos */
   off = g_gdp.ti_address + (ul_s << g_gdp.tile[tile].size >> 1);
   dst = ((uint8_t*)g_gdp.tmem) + (g_gdp.tile[tile].tmem<<3);
   cnt = lr_s-ul_s+1;
   if (g_gdp.tile[tile].size == 3)
      cnt <<= 1;

   if (((g_gdp.tile[tile].tmem + cnt) << 3) > sizeof(g_gdp.tmem))
   {
      //WriteLog(M64MSG_INFO, "rdp_loadblock wanted to write %u bytes after the end of tmem", ((g_gdp.tile[tile].tmem + cnt) << 3) - sizeof(g_gdp.tmem));
      cnt = (sizeof(g_gdp.tmem) >> 3) - (g_gdp.tile[tile].tmem);
   }

   if (g_gdp.ti_size == G_IM_SIZ_32b)
      LoadBlock32b(tile, ul_s, ul_t, lr_s, dxt);
   else
      loadBlock((uint32_t *)gfx_info.RDRAM, (uint32_t *)dst, off, _dxt, cnt);

   g_gdp.ti_address += cnt << 3;
   g_gdp.tile[tile].tl = ul_t + ((dxt*cnt)>>11);

   g_gdp.flags |= UPDATE_TEXTURE;
}

void glide64gDPLoadTile(uint32_t tile, uint32_t ul_s, uint32_t ul_t,
      uint32_t lr_s, uint32_t lr_t)
{
   uint32_t offs, height, width;
   int line_n;

   if (rdp.skip_drawing)
      return;

   rdp.timg.set_by                 = 1; /* load tile */

   rdp.addr[g_gdp.tile[tile].tmem] = g_gdp.ti_address;

   if (lr_s < ul_s || lr_t < ul_t)
      return;

   if ((settings.hacks&hack_Tonic) && tile == 7)
      glide64gDPSetTileSize(
            0,          /* tile */
            ul_s,       /* ulx  */
            ul_t,       /* uly  */
            lr_s,       /* lrx  */
            lr_t        /* lry  */
            );

   height = lr_t - ul_t + 1; // get height
   width = lr_s - ul_s + 1;

#ifdef TEXTURE_FILTER
   LOAD_TILE_INFO &info = rdp.load_info[g_gdp.tile[tile].tmem];
   info.tile_ul_s       = ul_s;
   info.tile_ul_t       = ul_t;
   info.tile_width      = (g_gdp.tile[tile].ms ? MIN((uint16_t)width,   1 << g_gdp.tile[tile].ms) : (uint16_t)width);
   info.tile_height     = (g_gdp.tile[tile].mt ? MIN((uint16_t)height, 1 << g_gdp.tile[tile].mt) : (uint16_t)height);

   if (settings.hacks&hack_MK64) {
      if (info.tile_width%2)
         info.tile_width--;
      if (info.tile_height%2)
         info.tile_height--;
   }
   info.tex_width       = g_gdp.ti_width;
   info.tex_size        = g_gdp.ti_size;
#endif


   line_n = g_gdp.ti_width << g_gdp.tile[tile].size >> 1;
   offs   = ul_t * line_n;
   offs  += ul_s << g_gdp.tile[tile].size >> 1;
   offs  += g_gdp.ti_address;
   if (offs >= BMASK)
      return;

   if (g_gdp.ti_size == G_IM_SIZ_32b)
   {
      LoadTile32b(tile, ul_s, ul_t, width, height);
   }
   else
   {
      uint8_t *dst, *end;
      uint32_t wid_64;

      // check if points to bad location
      if (offs + line_n*height > BMASK)
         height = (BMASK - offs) / line_n;
      if (height == 0)
         return;

      wid_64 = g_gdp.tile[tile].line;
      dst    = ((uint8_t*)g_gdp.tmem) + (g_gdp.tile[tile].tmem << 3);
      end    = ((uint8_t*)g_gdp.tmem) + 4096 - (wid_64<<3);
      loadTile((uint32_t *)gfx_info.RDRAM, (uint32_t *)dst, wid_64, height, line_n, offs, (uint32_t *)end);
   }
}

void glide64gDPFillRectangle(uint32_t ul_x, uint32_t ul_y, uint32_t lr_x, uint32_t lr_y)
{
   int32_t s_ul_x, s_lr_x, s_ul_y, s_lr_y;
   int pd_multiplayer = (settings.ucode == 7)  /* ucode_PerfectDark */
      && (((gDP.otherMode.h & RDP_CYCLE_TYPE) >> 20) == 3)
      && (g_gdp.fill_color.total == 0xFFFCFFFC);

   if (
         (gDP.colorImage.address == g_gdp.zb_address) || 
         (fb_emulation_enabled && rdp.ci_count > 0 && rdp.frame_buffers[rdp.ci_count-1].status == CI_ZIMG) ||
         pd_multiplayer
      )
   {
      /* FillRect - cleared the depth buffer */

      //if (fullscreen)
      {
         /* do not clear main depth buffer for aux depth buffers */
         if (!(settings.hacks&hack_Hyperbike) || gDP.colorImage.width > 64) 
         {
            update_scissor(false);
            grDepthMask (FXTRUE);
            grColorMask (FXFALSE, FXFALSE);
            grBufferClear (0, 0, g_gdp.fill_color.total ? g_gdp.fill_color.total & 0xFFFF : 0xFFFF);
            grColorMask (FXTRUE, FXTRUE);
            g_gdp.flags |= UPDATE_ZBUF_ENABLED;
         }
         //if (settings.frame_buffer&fb_depth_clear)
         {
            uint32_t zi_width_in_dwords, *dst, x, y;
            ul_x = MIN(MAX(ul_x, g_gdp.__clip.xh),  g_gdp.__clip.xl);
            lr_x = MIN(MAX(lr_x, g_gdp.__clip.xh),  g_gdp.__clip.xl);
            ul_y = MIN(MAX(ul_y, g_gdp.__clip.yh),  g_gdp.__clip.yl);
            lr_y = MIN(MAX(lr_y, g_gdp.__clip.yh),  g_gdp.__clip.yl);
            zi_width_in_dwords = gDP.colorImage.width >> 1;
            ul_x >>= 1;
            lr_x >>= 1;
            dst = (uint32_t*)(gfx_info.RDRAM + gDP.colorImage.address);
            dst += ul_y * zi_width_in_dwords;
            for (y = ul_y; y < lr_y; y++)
            {
               for (x = ul_x; x < lr_x; x++)
                  dst[x] = g_gdp.fill_color.total;
               dst += zi_width_in_dwords;
            }
         }
      }
      return;
   }

   /* Fillrect skipped */
   if (rdp.skip_drawing)
      return;

   /* Update scissor */
   //if (fullscreen)
   update_scissor(false);

   if (settings.decrease_fillrect_edge && ((gDP.otherMode.h & RDP_CYCLE_TYPE) >> 20) == 0)
   {
      lr_x--; lr_y--;
   }

   s_ul_x = (uint32_t)(ul_x * rdp.scale_x + rdp.offset_x);
   s_lr_x = (uint32_t)(lr_x * rdp.scale_x + rdp.offset_x);
   s_ul_y = (uint32_t)(ul_y * rdp.scale_y + rdp.offset_y);
   s_lr_y = (uint32_t)(lr_y * rdp.scale_y + rdp.offset_y);

   if (s_lr_x < 0)
      s_lr_x = 0;
   if (s_lr_y < 0)
      s_lr_y = 0;
   if ((uint32_t)s_ul_x > settings.res_x)
      s_ul_x = settings.res_x;
   if ((uint32_t)s_ul_y > settings.res_y)
      s_ul_y = settings.res_y;

   FRDP (" - %d, %d, %d, %d\n", s_ul_x, s_ul_y, s_lr_x, s_lr_y);

   {
      VERTEX v[4], vout[4];
      float Z = 0;

      grFogMode (GR_FOG_DISABLE, g_gdp.fog_color.total);

      if (((gDP.otherMode.h & RDP_CYCLE_TYPE) >> 20) != G_CYC_FILL)
         Z = set_sprite_combine_mode();

      // Draw the vertices
      v[0].x = (float)s_ul_x;
      v[0].y = (float)s_ul_y;
      v[0].z = Z;
      v[0].q = 1.0f;
      v[0].b        = 0;
      v[0].g        = 0;
      v[0].r        = 0;
      v[0].a        = 0;
      v[0].coord[0] = 0;
      v[0].coord[1] = 0;
      v[0].coord[2] = 0;
      v[0].coord[3] = 0;
      v[0].f        = 0.0f;
      v[0].u[0]     = 0.0f;
      v[0].u[1]     = 0.0f;
      v[0].v[0]     = 0.0f;
      v[0].v[1]     = 0.0f;
      v[0].w        = 0.0f;
      v[0].flags    = 0;
      v[0].vec[0]   = 0.0f;
      v[0].vec[1]   = 0.0f;
      v[0].vec[2]   = 0.0f;
      v[0].sx       = 0.0f;
      v[0].sy       = 0.0f;
      v[0].sz       = 0.0f;
      v[0].x_w      = 0.0f;
      v[0].y_w      = 0.0f;
      v[0].z_w      = 0.0f;
      v[0].oow      = 0.0f;
      v[0].u_w[0]   = 0.0f;
      v[0].u_w[1]   = 0.0f;
      v[0].v_w[0]            = 0.0f;
      v[0].v_w[1]            = 0.0f;
      v[0].not_zclipped      = 0;
      v[0].screen_translated = 0;
      v[0].uv_scaled         = 0;
      v[0].uv_calculated     = 0;
      v[0].shade_mod         = 0;
      v[0].color_backup      = 0;
      v[0].ou                = 0.0f;
      v[0].ov                = 0.0f;
      v[0].number            = 0;
      v[0].scr_off           = 0;
      v[0].z_off             = 0;

      v[1].x = (float)s_lr_x;
      v[1].y = (float)s_ul_y;
      v[1].z = Z;
      v[1].q = 1.0f;
      v[1].b        = 0;
      v[1].g        = 0;
      v[1].r        = 0;
      v[1].a        = 0;
      v[1].coord[0] = 0;
      v[1].coord[1] = 0;
      v[1].coord[2] = 0;
      v[1].coord[3] = 0;
      v[1].f        = 0.0f;
      v[1].u[0]     = 0.0f;
      v[1].u[1]     = 0.0f;
      v[1].v[0]     = 0.0f;
      v[1].v[1]     = 0.0f;
      v[1].w        = 0.0f;
      v[1].flags    = 0;
      v[1].vec[0]   = 0.0f;
      v[1].vec[1]   = 0.0f;
      v[1].vec[2]   = 0.0f;
      v[1].sx       = 0.0f;
      v[1].sy       = 0.0f;
      v[1].sz       = 0.0f;
      v[1].x_w      = 0.0f;
      v[1].y_w      = 0.0f;
      v[1].z_w      = 0.0f;
      v[1].oow      = 0.0f;
      v[1].u_w[0]   = 0.0f;
      v[1].u_w[1]   = 0.0f;
      v[1].v_w[0]            = 0.0f;
      v[1].v_w[1]            = 0.0f;
      v[1].not_zclipped      = 0;
      v[1].screen_translated = 0;
      v[1].uv_scaled         = 0;
      v[1].uv_calculated     = 0;
      v[1].shade_mod         = 0;
      v[1].color_backup      = 0;
      v[1].ou                = 0.0f;
      v[1].ov                = 0.0f;
      v[1].number            = 0;
      v[1].scr_off           = 0;
      v[1].z_off             = 0;

      v[2].x = (float)s_ul_x;
      v[2].y = (float)s_lr_y;
      v[2].z = Z;
      v[2].q = 1.0f;
      v[2].b        = 0;
      v[2].g        = 0;
      v[2].r        = 0;
      v[2].a        = 0;
      v[2].coord[0] = 0;
      v[2].coord[1] = 0;
      v[2].coord[2] = 0;
      v[2].coord[3] = 0;
      v[2].f        = 0.0f;
      v[2].u[0]     = 0.0f;
      v[2].u[1]     = 0.0f;
      v[2].v[0]     = 0.0f;
      v[2].v[1]     = 0.0f;
      v[2].w        = 0.0f;
      v[2].flags    = 0;
      v[2].vec[0]   = 0.0f;
      v[2].vec[1]   = 0.0f;
      v[2].vec[2]   = 0.0f;
      v[2].sx       = 0.0f;
      v[2].sy       = 0.0f;
      v[2].sz       = 0.0f;
      v[2].x_w      = 0.0f;
      v[2].y_w      = 0.0f;
      v[2].z_w      = 0.0f;
      v[2].oow      = 0.0f;
      v[2].u_w[0]   = 0.0f;
      v[2].u_w[1]   = 0.0f;
      v[2].v_w[0]            = 0.0f;
      v[2].v_w[1]            = 0.0f;
      v[2].not_zclipped      = 0;
      v[2].screen_translated = 0;
      v[2].uv_scaled         = 0;
      v[2].uv_calculated     = 0;
      v[2].shade_mod         = 0;
      v[2].color_backup      = 0;
      v[2].ou                = 0.0f;
      v[2].ov                = 0.0f;
      v[2].number            = 0;
      v[2].scr_off           = 0;
      v[2].z_off             = 0;
      
      v[3].x = (float)s_lr_x;
      v[3].y = (float)s_lr_y;
      v[3].z = Z;
      v[3].q = 1.0f;
      v[3].b        = 0;
      v[3].g        = 0;
      v[3].r        = 0;
      v[3].a        = 0;
      v[3].coord[0] = 0;
      v[3].coord[1] = 0;
      v[3].coord[2] = 0;
      v[3].coord[3] = 0;
      v[3].f        = 0.0f;
      v[3].u[0]     = 0.0f;
      v[3].u[1]     = 0.0f;
      v[3].v[0]     = 0.0f;
      v[3].v[1]     = 0.0f;
      v[3].w        = 0.0f;
      v[3].flags    = 0;
      v[3].vec[0]   = 0.0f;
      v[3].vec[1]   = 0.0f;
      v[3].vec[2]   = 0.0f;
      v[3].sx       = 0.0f;
      v[3].sy       = 0.0f;
      v[3].sz       = 0.0f;
      v[3].x_w      = 0.0f;
      v[3].y_w      = 0.0f;
      v[3].z_w      = 0.0f;
      v[3].oow      = 0.0f;
      v[3].u_w[0]   = 0.0f;
      v[3].u_w[1]   = 0.0f;
      v[3].v_w[0]            = 0.0f;
      v[3].v_w[1]            = 0.0f;
      v[3].not_zclipped      = 0;
      v[3].screen_translated = 0;
      v[3].uv_scaled         = 0;
      v[3].uv_calculated     = 0;
      v[3].shade_mod         = 0;
      v[3].color_backup      = 0;
      v[3].ou                = 0.0f;
      v[3].ov                = 0.0f;
      v[3].number            = 0;
      v[3].scr_off           = 0;
      v[3].z_off             = 0;

      if (((gDP.otherMode.h & RDP_CYCLE_TYPE) >> 20) == G_CYC_FILL)
      {
         uint32_t color = g_gdp.fill_color.total;

         if ((settings.hacks&hack_PMario) && rdp.ci_count > 0 && rdp.frame_buffers[rdp.ci_count-1].status == CI_AUX)
         {
            /* background of auxiliary frame buffers must have zero alpha.
             * make it black, set 0 alpha to black pixels on frame buffer read */
            color = 0;
         }
         else if (g_gdp.fb_size < G_IM_SIZ_32b)
         {
            color = ((color&1)?0xFF:0) |
               ((uint32_t)((float)((color&0xF800) >> 11) / 31.0f * 255.0f) << 24) |
               ((uint32_t)((float)((color&0x07C0) >> 6) / 31.0f * 255.0f) << 16) |
               ((uint32_t)((float)((color&0x003E) >> 1) / 31.0f * 255.0f) << 8);
         }

         grConstantColorValue (color);

         grColorCombine (GR_COMBINE_FUNCTION_LOCAL,
               GR_COMBINE_FACTOR_NONE,
               GR_COMBINE_LOCAL_CONSTANT,
               GR_COMBINE_OTHER_NONE,
               FXFALSE);

         grAlphaCombine (GR_COMBINE_FUNCTION_LOCAL,
               GR_COMBINE_FACTOR_NONE,
               GR_COMBINE_LOCAL_CONSTANT,
               GR_COMBINE_OTHER_NONE,
               FXFALSE);

         grAlphaBlendFunction (GR_BLEND_ONE, GR_BLEND_ZERO, GR_BLEND_ONE, GR_BLEND_ZERO);

         grAlphaTestFunction (GR_CMP_ALWAYS, 0x00, 0);
         grStippleMode(GR_STIPPLE_DISABLE);

         grCullMode(GR_CULL_DISABLE);
         grFogMode (GR_FOG_DISABLE, g_gdp.fog_color.total);
         grDepthBufferFunction (GR_CMP_ALWAYS);
         grDepthMask (FXFALSE);

         g_gdp.flags |= UPDATE_COMBINE | UPDATE_CULL_MODE | UPDATE_FOG_ENABLED | UPDATE_ZBUF_ENABLED;
      }
      else
      {
         uint32_t cmb_mode_c = (rdp.cycle1 << 16) | (rdp.cycle2 & 0xFFFF);
         uint32_t cmb_mode_a = (rdp.cycle1 & 0x0FFF0000) | ((rdp.cycle2 >> 16) & 0x00000FFF);

         if (cmb_mode_c == 0x9fff9fff || cmb_mode_a == 0x09ff09ff) //shade
            apply_shading(v);

         /* special blender mode for Bomberman64 */
         if (gDP.otherMode.forceBlender && ((gDP.otherMode.l >> 16) == 0x0550))
         {
            grAlphaCombine (GR_COMBINE_FUNCTION_LOCAL,
                  GR_COMBINE_FACTOR_NONE,
                  GR_COMBINE_LOCAL_CONSTANT,
                  GR_COMBINE_OTHER_NONE,
                  FXFALSE);
            grConstantColorValue((cmb.ccolor & 0xFFFFFF00) | g_gdp.fog_color.a);
            g_gdp.flags |= UPDATE_COMBINE;
         }
      }

      vout[0] = v[0];
      vout[1] = v[2];
      vout[2] = v[1];
      vout[3] = v[3];

      grDrawVertexArrayContiguous(GR_TRIANGLE_STRIP, 4, &vout[0]);
   }
}
