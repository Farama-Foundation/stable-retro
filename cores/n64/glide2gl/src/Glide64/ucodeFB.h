/*
* Glide64 - Glide video plugin for Nintendo 64 emulators.
* Copyright (c) 2002  Dave2001
* Copyright (c) 2003-2009  Sergey 'Gonetz' Lipski
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* any later version.
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
//
// Creation 13 August 2003               Gonetz
//
//****************************************************************

static void fb_uc0_moveword(uint32_t w0, uint32_t w1)
{
   if ((w0 & 0xFF) == 0x06) // segment
      glide64gSPSegment((w0 >> 10) & 0x0F, w1);
}

static void fb_uc2_moveword(uint32_t w0, uint32_t w1)
{
   if (((w0 >> 16) & 0xFF) == 0x06) // segment
      glide64gSPSegment(((w0 & 0xFFFF) >> 2)&0xF, w1);
}

static void fb_bg_copy(uint32_t w0, uint32_t w1)
{
   CI_STATUS status;
   uint32_t addr, imagePtr;
   if (rdp.main_ci == 0)
      return;
   status = rdp.frame_buffers[rdp.ci_count-1].status;
   if (status == CI_COPY)
      return;

   addr     = RSP_SegmentToPhysical(w1) >> 1;
   imagePtr	= RSP_SegmentToPhysical(((uint32_t*)gfx_info.RDRAM)[(addr+8)>>1]);

   if (status == CI_MAIN)
   {
      uint16_t frameW	= ((uint16_t *)gfx_info.RDRAM)[(addr+3)^1] >> 2;
      uint16_t frameH	= ((uint16_t *)gfx_info.RDRAM)[(addr+7)^1] >> 2;
      if ( (frameW == rdp.frame_buffers[rdp.ci_count-1].width) && (frameH == rdp.frame_buffers[rdp.ci_count-1].height) )
         rdp.main_ci_bg = imagePtr;
   }
   else if (imagePtr >= rdp.main_ci && imagePtr < rdp.main_ci_end) //addr within main frame buffer
   {
      rdp.copy_ci_index = rdp.ci_count-1;
      rdp.frame_buffers[rdp.copy_ci_index].status = CI_COPY;
      FRDP("rdp.frame_buffers[%d].status = CI_COPY\n", rdp.copy_ci_index);

      if (rdp.frame_buffers[rdp.copy_ci_index].addr != rdp.main_ci_bg)
      {
         rdp.scale_x = 1.0f;
         rdp.scale_y = 1.0f;
      }
      else
      {
         rdp.motionblur = true;
      }

      FRDP ("Detect FB usage. texture addr is inside framebuffer: %08lx - %08lx \n", imagePtr, rdp.main_ci);
   }
   else if (imagePtr == g_gdp.zb_address)
   {
      if (status == CI_UNKNOWN)
      {
         rdp.frame_buffers[rdp.ci_count-1].status = CI_ZCOPY;
         rdp.tmpzimg = rdp.frame_buffers[rdp.ci_count-1].addr;
         if (!rdp.copy_zi_index)
            rdp.copy_zi_index = rdp.ci_count-1;
         FRDP("rdp.frame_buffers[%d].status = CI_ZCOPY\n", rdp.copy_ci_index);
      }
   }
}

static void fb_setscissor(uint32_t w0, uint32_t w1)
{
    gDPSetScissor( _SHIFTR( w1, 24, 2 ),                        // mode
                   _FIXED2FLOAT( _SHIFTR( w0, 12, 12 ), 2 ),    // ulx
                   _FIXED2FLOAT( _SHIFTR( w0,  0, 12 ), 2 ),    // uly
                   _FIXED2FLOAT( _SHIFTR( w1, 12, 12 ), 2 ),    // lrx
                   _FIXED2FLOAT( _SHIFTR( w1,  0, 12 ), 2 ) );  // lry

   if (rdp.ci_count)
   {
      COLOR_IMAGE *cur_fb = (COLOR_IMAGE*)&rdp.frame_buffers[rdp.ci_count-1];
      if (g_gdp.__clip.xl - g_gdp.__clip.xh > (uint32_t)(cur_fb->width >> 1))
      {
         if (cur_fb->height == 0 || (cur_fb->width >= g_gdp.__clip.xl - 1 && cur_fb->width <= g_gdp.__clip.xl + 1))
            cur_fb->height = g_gdp.__clip.yl;
      }
   }
}

static void fb_uc2_movemem(uint32_t w0, uint32_t w1)
{
   if ((w0 & 0xFF) == 8)
   {
      uint32_t a           = RSP_SegmentToPhysical(w1) >> 1;
      int16_t scale_x      = ((int16_t*)gfx_info.RDRAM)[(a+0)^1] >> 2;
      int16_t trans_x      = ((int16_t*)gfx_info.RDRAM)[(a+4)^1] >> 2;
      COLOR_IMAGE *cur_fb  = (COLOR_IMAGE*)&rdp.frame_buffers[rdp.ci_count-1];

      if ( abs((int)(scale_x + trans_x - cur_fb->width)) < 3)
      {
         int16_t scale_y   = ((int16_t*)gfx_info.RDRAM)[(a+1)^1] >> 2;
         int16_t trans_y   = ((int16_t*)gfx_info.RDRAM)[(a+5)^1] >> 2;
         uint32_t height   = scale_y + trans_y;
         
         if (height < g_gdp.__clip.yl)
            cur_fb->height = height;
      }
   }
}

static void fb_rect(uint32_t w0, uint32_t w1)
{
   int ul_x, lr_x, width, diff;

   if (rdp.frame_buffers[rdp.ci_count-1].width == 32)
      return;

   ul_x    = ((w1 & 0x00FFF000) >> 14);
   lr_x    = ((w0 & 0x00FFF000) >> 14);
   width   = lr_x-ul_x;
   diff    = abs((int)rdp.frame_buffers[rdp.ci_count-1].width - width);

   if (diff < 4)
   {
      uint32_t lr_y = MIN(g_gdp.__clip.yl, (w0 & 0xFFF) >> 2);
      if (rdp.frame_buffers[rdp.ci_count-1].height < lr_y)
      {
         FRDP("fb_rect. ul_x: %d, lr_x: %d, fb_height: %d -> %d\n", ul_x, lr_x, rdp.frame_buffers[rdp.ci_count-1].height, lr_y);
         rdp.frame_buffers[rdp.ci_count-1].height = lr_y;
      }
   }
}

static void fb_rdphalf_1(uint32_t w0, uint32_t w1)
{
   gDP.half_1 = w1;
}

static void fb_settextureimage(uint32_t w0, uint32_t w1)
{
   COLOR_IMAGE *cur_fb;
   if (rdp.main_ci == 0)
      return;
   cur_fb = (COLOR_IMAGE*)&rdp.frame_buffers[rdp.ci_count-1];
   if ( cur_fb->status >= CI_COPY)
      return;
   if (((w0 >> 19) & 0x03) >= 2)  //check that texture is 16/32bit
   {
      int tex_format = ((w0 >> 21) & 0x07);
      uint32_t addr  = RSP_SegmentToPhysical(w1);

      if ( tex_format == 0 )
      {
         FRDP ("fb_settextureimage. fmt: %d, size: %d, imagePtr %08lx, main_ci: %08lx, cur_ci: %08lx \n", ((w0 >> 21) & 0x07), ((w0 >> 19) & 0x03), addr, rdp.main_ci, rdp.frame_buffers[rdp.ci_count-1].addr);
         if (cur_fb->status == CI_MAIN)
         {
            rdp.main_ci_last_tex_addr = addr;
            if (cur_fb->height == 0)
            {
               cur_fb->height = g_gdp.__clip.yl;
               rdp.main_ci_end = cur_fb->addr + ((cur_fb->width * cur_fb->height) << cur_fb->size >> 1);
            }
         }
         if ((addr >= rdp.main_ci) && (addr < rdp.main_ci_end)) //addr within main frame buffer
         {
            if (cur_fb->status == CI_MAIN)
            {
               rdp.copy_ci_index = rdp.ci_count-1;
               cur_fb->status    = CI_COPY_SELF;
               rdp.scale_x       = rdp.scale_x_bak;
               rdp.scale_y       = rdp.scale_y_bak;

               FRDP("rdp.frame_buffers[%d].status = CI_COPY_SELF\n", rdp.ci_count-1);
            }
            else
            {
               if (cur_fb->width == rdp.frame_buffers[rdp.main_ci_index].width)
               {
                  rdp.copy_ci_index = rdp.ci_count-1;
                  cur_fb->status    = CI_COPY;

                  FRDP("rdp.frame_buffers[%d].status = CI_COPY\n", rdp.copy_ci_index);

                  if ((rdp.main_ci_last_tex_addr >= cur_fb->addr) &&
                        (rdp.main_ci_last_tex_addr < (cur_fb->addr + cur_fb->width*cur_fb->height*cur_fb->size)))
                  {
                     rdp.motionblur = true;
                  }
                  else
                  {
                     rdp.scale_x = 1.0f;
                     rdp.scale_y = 1.0f;
                  }
               }
               else if (!(settings.frame_buffer & fb_ignore_aux_copy) && cur_fb->width < rdp.frame_buffers[rdp.main_ci_index].width)
               {
                  rdp.copy_ci_index = rdp.ci_count-1;
                  cur_fb->status    = CI_AUX_COPY;
                  FRDP("rdp.frame_buffers[%d].status = ci_aux_copy\n", rdp.copy_ci_index);
                  rdp.scale_x       = 1.0f;
                  rdp.scale_y       = 1.0f;
               }
               else
               {
                  cur_fb->status = CI_AUX;
                  FRDP("rdp.frame_buffers[%d].status = ci_aux\n", rdp.copy_ci_index);
               }
            }
            FRDP ("Detect FB usage. texture addr is inside framebuffer: %08lx - %08lx \n", addr, rdp.main_ci);
         }
         ///*
         else if ((cur_fb->status != CI_MAIN) && (addr >= g_gdp.zb_address && addr < rdp.zimg_end))
         {
            cur_fb->status = CI_ZCOPY;
            if (!rdp.copy_zi_index)
               rdp.copy_zi_index = rdp.ci_count-1;
            FRDP("fb_settextureimage. rdp.frame_buffers[%d].status = CI_ZCOPY\n", rdp.ci_count-1);
         }
         //*/
         else if ((rdp.maincimg[0].width > 64) && (addr >= rdp.maincimg[0].addr) && (addr < (rdp.maincimg[0].addr + rdp.maincimg[0].width*rdp.maincimg[0].height*2)))
         {
            if (cur_fb->status != CI_MAIN)
            {
               cur_fb->status = CI_OLD_COPY;
               FRDP("rdp.frame_buffers[%d].status = CI_OLD_COPY 1, addr:%08lx\n", rdp.ci_count-1, rdp.last_drawn_ci_addr);
            }
            rdp.read_previous_ci = true;
         }
         else if ((addr >= rdp.last_drawn_ci_addr) && (addr < (rdp.last_drawn_ci_addr + rdp.maincimg[0].width*rdp.maincimg[0].height*2)))
         {
            if (cur_fb->status != CI_MAIN)
            {
               cur_fb->status = CI_OLD_COPY;
               FRDP("rdp.frame_buffers[%d].status = CI_OLD_COPY 2, addr:%08lx\n", rdp.ci_count-1, rdp.last_drawn_ci_addr);
            }
            rdp.read_previous_ci = true;
         }
      }
      else if (fb_hwfbe_enabled && (cur_fb->status == CI_MAIN))
      {
         if ((addr >= rdp.main_ci) && (addr < rdp.main_ci_end)) //addr within main frame buffer
         {
            rdp.copy_ci_index  = rdp.ci_count-1;
            rdp.black_ci_index = rdp.ci_count-1;
            cur_fb->status     = CI_COPY_SELF;
            FRDP("rdp.frame_buffers[%d].status = CI_COPY_SELF\n", rdp.ci_count-1);
         }
      }
   }
   if (cur_fb->status == CI_UNKNOWN)
   {
      cur_fb->status = CI_AUX;
      FRDP("fb_settextureimage. rdp.frame_buffers[%d].status = ci_aux\n", rdp.ci_count-1);
   }
}

static void fb_loadtxtr(uint32_t w0, uint32_t w1)
{
   if (rdp.frame_buffers[rdp.ci_count-1].status == CI_UNKNOWN)
   {
      rdp.frame_buffers[rdp.ci_count-1].status = CI_AUX;
      FRDP("rdp.frame_buffers[%d].status = ci_aux\n", rdp.ci_count-1);
   }
}

static void fb_setdepthimage(uint32_t w0, uint32_t w1)
{
   int i;
   g_gdp.zb_address = RSP_SegmentToPhysical(w1);
   rdp.zimg_end     = g_gdp.zb_address + gDP.colorImage.width * gDP.colorImage.height * 2;

   if (g_gdp.zb_address == rdp.main_ci)  //strange, but can happen
   {
      rdp.frame_buffers[rdp.main_ci_index].status = CI_UNKNOWN;

      if (rdp.main_ci_index < rdp.ci_count)
      {
         rdp.frame_buffers[rdp.main_ci_index].status = CI_ZIMG;
         FRDP("rdp.frame_buffers[%d].status = CI_ZIMG\n", rdp.main_ci_index);
         rdp.main_ci_index++;
         rdp.frame_buffers[rdp.main_ci_index].status = CI_MAIN;
         FRDP("rdp.frame_buffers[%d].status = CI_MAIN\n", rdp.main_ci_index);
         rdp.main_ci     = rdp.frame_buffers[rdp.main_ci_index].addr;
         rdp.main_ci_end = rdp.main_ci + (rdp.frame_buffers[rdp.main_ci_index].width 
               * rdp.frame_buffers[rdp.main_ci_index].height
               * rdp.frame_buffers[rdp.main_ci_index].size);
        
         for (i = rdp.main_ci_index+1; i < rdp.ci_count; i++)
         {
            COLOR_IMAGE *fb = (COLOR_IMAGE*)&rdp.frame_buffers[i];
            if (fb->addr == rdp.main_ci)
            {
               fb->status = CI_MAIN;
               FRDP("rdp.frame_buffers[%d].status = CI_MAIN\n", i);
            }
         }
      }
      else
         rdp.main_ci = 0;
   }
   for (i = 0; i < rdp.ci_count; i++)
   {
      COLOR_IMAGE *fb = (COLOR_IMAGE*)&rdp.frame_buffers[i];
      if ((fb->addr == g_gdp.zb_address) && (fb->status == CI_AUX || fb->status == CI_USELESS))
      {
         fb->status = CI_ZIMG;
         FRDP("rdp.frame_buffers[%d].status = CI_ZIMG\n", i);
      }
   }
}

static void fb_setcolorimage(uint32_t w0, uint32_t w1)
{
   COLOR_IMAGE *cur_fb    = NULL;
   rdp.ocimg              = gDP.colorImage.address;
   gDP.colorImage.address = RSP_SegmentToPhysical(w1);

   cur_fb                 = (COLOR_IMAGE*)&rdp.frame_buffers[rdp.ci_count];
   cur_fb->width          = (w0 & 0xFFF) + 1;
   if (cur_fb->width == 32 )
      cur_fb->height      = 32;
   else if (cur_fb->width == 16 )
      cur_fb->height      = 16;
   else if (rdp.ci_count > 0)
      cur_fb->height      = g_gdp.__clip.yl;
   else
      cur_fb->height      = 0;

   cur_fb->format         = (w0 & 0x00E00000) >> (53 - 32);
   cur_fb->size           = (w0 & 0x00180000) >> (51 - 32);
   cur_fb->addr           = gDP.colorImage.address;
   cur_fb->changed        = 1;

   /*
      if (rdp.ci_count > 0)
      if (rdp.frame_buffers[0].addr == gDP.colorImage.address)
      rdp.frame_buffers[0].height = g_gdp.__clip.yl;
      */
   //FRDP ("fb_setcolorimage. width: %d,  height: %d,  fmt: %d, size: %d, addr %08lx\n", cur_fb->width, cur_fb->height, cur_fb->format, cur_fb->size, cur_fb->addr);
   if (gDP.colorImage.address == g_gdp.zb_address)
   {
      cur_fb->status = CI_ZIMG;
      rdp.zimg_end = g_gdp.zb_address + cur_fb->width * g_gdp.__clip.yl * 2;
      //FRDP("rdp.frame_buffers[%d].status = CI_ZIMG\n", rdp.ci_count);
   }
   else if (gDP.colorImage.address == rdp.tmpzimg)
   {
      cur_fb->status = CI_ZCOPY;
      if (!rdp.copy_zi_index)
         rdp.copy_zi_index = rdp.ci_count-1;
      //FRDP("rdp.frame_buffers[%d].status = CI_ZCOPY\n", rdp.ci_count);
   }
   else if (rdp.main_ci != 0)
   {
      if (gDP.colorImage.address == rdp.main_ci) //switched to main fb again
      {
         cur_fb->height    = MAX(cur_fb->height, rdp.frame_buffers[rdp.main_ci_index].height);
         rdp.main_ci_index = rdp.ci_count;
         rdp.main_ci_end   = gDP.colorImage.address + ((cur_fb->width * cur_fb->height) << cur_fb->size >> 1);
         cur_fb->status    = CI_MAIN;
         //FRDP("rdp.frame_buffers[%d].status = CI_MAIN\n", rdp.ci_count);
      }
      else // status is not known yet
         cur_fb->status    = CI_UNKNOWN;
   }
   else
   {
      if ((g_gdp.zb_address != gDP.colorImage.address))//&& (rdp.ocimg != gDP.colorImage.address))
      {
         rdp.main_ci       = gDP.colorImage.address;
         rdp.main_ci_end   = gDP.colorImage.address + ((cur_fb->width * cur_fb->height) << cur_fb->size >> 1);
         rdp.main_ci_index = rdp.ci_count;
         cur_fb->status    = CI_MAIN;
         //FRDP("rdp.frame_buffers[%d].status = CI_MAIN\n", rdp.ci_count);
      }
      else
         cur_fb->status = CI_UNKNOWN;

   }
   if (rdp.ci_count > 0 && rdp.frame_buffers[rdp.ci_count-1].status == CI_UNKNOWN) //status of previous fb was not changed - it is useless
   {
      if (fb_hwfbe_enabled && !(settings.frame_buffer & fb_useless_is_useless))
      {
         rdp.frame_buffers[rdp.ci_count-1].status  = CI_AUX;
         rdp.frame_buffers[rdp.ci_count-1].changed = 0;
         FRDP("rdp.frame_buffers[%d].status = ci_aux\n", rdp.ci_count-1);
      }
      else
      {
         rdp.frame_buffers[rdp.ci_count-1].status = CI_USELESS;
         /*
            uint32_t addr = rdp.frame_buffers[rdp.ci_count-1].addr;
            for (i = 0; i < rdp.ci_count - 1; i++)
            {
            if (rdp.frame_buffers[i].addr == addr)
            {
            rdp.frame_buffers[rdp.ci_count-1].status = rdp.frame_buffers[i].status;
            break;
            }
            }
         //*/
         //FRDP("rdp.frame_buffers[%d].status = %s\n", rdp.ci_count-1, CIStatus[rdp.frame_buffers[rdp.ci_count-1].status]);
      }
   }
   if (cur_fb->status == CI_MAIN)
   {
      int viSwapOK = ((settings.swapmode == 2) && (rdp.vi_org_reg == *gfx_info.VI_ORIGIN_REG)) ? false : true;
      if ((rdp.maincimg[0].addr != cur_fb->addr) && SwapOK && viSwapOK)
      {
         SwapOK            = false;
         rdp.swap_ci_index = rdp.ci_count;
      }
   }
   rdp.ci_count++;
   if (rdp.ci_count > NUMTEXBUF) //overflow
      __RSP.halt = 1;
}

// RDP graphic instructions pointer table used in DetectFrameBufferUsage

static rdp_instr gfx_instruction_lite[9][256] =
{
   {
      // uCode 0 - RSP SW 2.0X
      // 00-3f
      // games: Super Mario 64, Tetrisphere, Demos
      0,                     0,             0,              0,
      0,             0,              uc0_displaylist,        0,
      0,              0,           0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      // 40-7f: Unused
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      // 80-bf: Immediate commands
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      F3D_EndDL,              0,     					0,    					0,
      fb_uc0_moveword,           0,          			uc0_culldl,             0,
      // c0-ff: RDP commands
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                  0,                  0,                  0,
      0,                  0,                  0,                  0,
      0,                  0,                  0,                  0,
      0,                  0,                  0,                  0,
      0,                  0,                  0,                  0,
      fb_rect,         fb_rect,         	  0,                  0,
      0,                  0,                  0,                  0,
      0,         fb_setscissor,         0,       0,
      0,                  0,                  0,                  0,
      0,                  0,            	fb_rect,              0,
      0,                  0,                  0,                  0,
      0,         fb_settextureimage,    fb_setdepthimage,      fb_setcolorimage
   },

   // uCode 1 - F3DEX 1.XX
   // 00-3f
   // games: Mario Kart, Star Fox
   {
      0,                      0,                      0,                      0,
      0,             0,              uc0_displaylist,        0,
      0,              0,           0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      // 40-7f: unused
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      // 80-bf: Immediate commands
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      uc6_loaducode,
      uc1_branch_z,           0,               0,		   0,
      fb_rdphalf_1,          0,             0,  0,
      F3D_EndDL,              0,     0,     0,
      fb_uc0_moveword,           0,          uc2_culldl,             0,
      // c0-ff: RDP commands
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                  0,                  0,                  0,
      0,                  0,                  0,                  0,
      0,                  0,                  0,                  0,
      0,                  0,                  0,                  0,
      0,                  0,                  0,                  0,
      fb_rect,         fb_rect,         	  0,                  0,
      0,                  0,                  0,                  0,
      0,         fb_setscissor,         0,       0,
      0,                  0,                  0,                  0,
      0,                  0,            	fb_rect,              0,
      0,                  0,                  0,                  0,
      0,         fb_settextureimage,    fb_setdepthimage,      fb_setcolorimage
   },

   // uCode 2 - F3DEX 2.XX
   // games: Zelda 64
   {
      // 00-3f
      0,					0,				0,			uc2_culldl,
      uc1_branch_z,			0,				0,			0,
      0,				fb_bg_copy,			fb_bg_copy,			0,
      0,					0,					0,					0,
      0,                  0,                  0,                  0,
      0,                  0,                  0,                  0,
      0,                  0,                  0,                  0,
      0,                  0,                  0,                  0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,

      // 40-7f: unused
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,

      // 80-bf: unused
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,

      // c0-ff: RDP commands mixed with uc2 commands
      0,                  0,                  0,                  0,
      0,                  0,                  0,                  0,
      0,                  0,                  0,                  0,
      0,                  0,                  0,                  0,
      0,                  0,                  0,                  0,
      0,			uc2_dlist_cnt,				0,					0,
      0,			0,			0,				fb_uc2_moveword,
      fb_uc2_movemem,			uc2_load_ucode,			uc0_displaylist,	F3D_EndDL,
      0,					fb_rdphalf_1,			0, 		0,
      fb_rect,         fb_rect,         	  0,                  0,
      0,                  0,                  0,                  0,
      0,         fb_setscissor,         0,       0,
      0,                  0,                  0,                  0,
      0,                  0,            	fb_rect,              0,
      0,                  0,                  0,                  0,
      0,         fb_settextureimage,    fb_setdepthimage,      fb_setcolorimage
   },

   // uCode 3 - "RSP SW 2.0D", but not really
   // 00-3f
   // games: Wave Race
   // ** Added by Gonetz **
   {
      0,                  0,                  0,                  0,
      0,                  0,                  uc0_displaylist,                  0,
      0,                  0,                  0,                  0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      // 40-7f: unused
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      // 80-bf: Immediate commands
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                  0,                  0,                  0,
      0,                  0,                  0,                  0,
      F3D_EndDL,              0,     0,     0,
      fb_uc0_moveword,           0,          uc0_culldl,             0,
      // c0-ff: RDP commands
      0,                  0,                  0,                  0,
      0,                  0,                  0,                  0,
      0,                  0,                  0,                  0,
      0,                  0,                  0,                  0,
      0,                  0,                  0,                  0,
      0,                  0,                  0,                  0,
      0,                  0,                  0,                  0,
      0,                  0,                  0,                  0,
      0,                  0,                  0,                  0,
      fb_rect,         fb_rect,         	  0,                  0,
      0,                  0,                  0,                  0,
      0,         fb_setscissor,         0,       0,
      0,                  0,                  0,                  0,
      0,                  0,            	fb_rect,              0,
      0,                  0,                  0,                  0,
      0,         fb_settextureimage,    fb_setdepthimage,      fb_setcolorimage
   },

   {
      // uCode 4 - RSP SW 2.0D EXT
      // 00-3f
      // games: Star Wars: Shadows of the Empire
      0,                  0,                  0,                  0,
      0,             0,              uc0_displaylist,        0,
      0,                  0,                  0,                  0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      // 40-7f: Unused
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      // 80-bf: Immediate commands
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                  0,                  0,                  0,
      0,                  0,                  0,                  0,
      F3D_EndDL,              0,     0,     0,
      fb_uc0_moveword,           0,          uc0_culldl,             0,
      // c0-ff: RDP commands
      gdp_no_op,               0,                  0,                  0,
      0,                  0,                  0,                  0,
      0,                  0,                  0,                  0,
      0,                  0,                  0,                  0,
      0,                  0,                  0,                  0,
      0,                  0,                  0,                  0,
      0,                  0,                  0,                  0,
      0,                  0,                  0,                  0,
      0,                  0,                  0,                  0,
      fb_rect,         fb_rect,         	  0,                  0,
      0,                  0,                  0,                  0,
      0,         fb_setscissor,         0,       0,
      0,                  0,                  0,                  0,
      0,                  0,            	fb_rect,              0,
      0,                  0,                  0,                  0,
      0,         fb_settextureimage,    fb_setdepthimage,      fb_setcolorimage
   },

   {
      // uCode 5 - RSP SW 2.0 Diddy
      // 00-3f
      // games: Diddy Kong Racing
      0,         				0,         				0,       				0,
      0,						0,			       uc0_displaylist,			 uc5_dl_in_mem,
      0,              		0,           			0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      // 40-7f: Unused
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      // 80-bf: Immediate commands
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                  	0,                  	0,                  	0,
      F3D_EndDL,              0,     					0,     					0,
      fb_uc0_moveword,        0,          			uc0_culldl,             0,
      // c0-ff: RDP commands
      0,                  0,                  0,                  0,
      0,                  0,                  0,                  0,
      0,                  0,                  0,                  0,
      0,                  0,                  0,                  0,
      0,                  0,                  0,                  0,
      0,                  0,                  0,                  0,
      0,                  0,                  0,                  0,
      0,                  0,                  0,                  0,
      0,                  0,                  0,                  0,
      fb_rect,         fb_rect,         	  0,                  0,
      0,                  0,                  0,                  0,
      0,         fb_setscissor,         0,       0,
      0,                  0,                  0,                  0,
      0,                  0,            	fb_rect,              0,
      0,                  0,                  0,                  0,
      0,         fb_settextureimage,    fb_setdepthimage,      fb_setcolorimage
   },

   // uCode 6 - S2DEX 1.XX
   // games: Yoshi's Story
   {
      0,                  0,                  0,                  0,
      0,             0,         uc0_displaylist,        0,
      0,                  0,                  0,                  0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      // 40-7f: unused
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      // 80-bf: Immediate commands
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      uc6_loaducode,
      uc6_select_dl,          0,         0,		0,
      0,                  0,                  0,                  0,
      F3D_EndDL,              0,     0,     0,
      fb_uc0_moveword,           0,          uc2_culldl,             0,
      // c0-ff: RDP commands
      0,               fb_loadtxtr,       fb_loadtxtr,    fb_loadtxtr,
      fb_loadtxtr,        0,                  0,                  0,
      0,                  0,                  0,                  0,
      0,                  0,                  0,                  0,
      0,                  0,                  0,                  0,
      0,                  0,                  0,                  0,
      0,                  0,                  0,                  0,
      0,                  0,                  0,                  0,
      0,                  0,                  0,                  0,
      fb_rect,         fb_rect,         	  0,                  0,
      0,                  0,                  0,                  0,
      0,         fb_setscissor,         0,       0,
      0,                  0,                  0,                  0,
      0,                  0,            	fb_rect,              0,
      0,                  0,                  0,                  0,
      0,         fb_settextureimage,    fb_setdepthimage,      fb_setcolorimage
   },

   {
      0,                      0,                      0,                      0,
      0,             			0,              		uc0_displaylist,        0,
      0,              		0,           			0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      // 40-7f: unused
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      // 80-bf: Immediate commands
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,           			0,               		0,		   				0,
      0,          			0,             			0,  					0,
      F3D_EndDL,              0,     					0,     					0,
      fb_uc0_moveword,        0,          			uc0_culldl,             0,
      // c0-ff: RDP commands
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                      0,                      0,                      0,
      0,                  0,                  0,                  0,
      0,                  0,                  0,                  0,
      0,                  0,                  0,                  0,
      0,                  0,                  0,                  0,
      0,                  0,                  0,                  0,
      fb_rect,         fb_rect,         	  0,                  0,
      0,                  0,                  0,                  0,
      0,         fb_setscissor,         0,       0,
      0,                  0,                  0,                  0,
      0,                  0,            	fb_rect,              0,
      0,                  0,                  0,                  0,
      0,         fb_settextureimage,    fb_setdepthimage,      fb_setcolorimage
   },

   {
      // 00-3f
      0,					0,				0,			uc2_culldl,
      uc1_branch_z,			0,				0,			0,
      0,				fb_bg_copy,			fb_bg_copy,			0,
      0,					0,					0,					0,
      0,                  0,                  0,                  0,
      0,                  0,                  0,                  0,
      0,                  0,                  0,                  0,
      0,                  0,                  0,                  0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,

      // 40-7f: unused
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,

      // 80-bf: unused
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,
      0,					0,					0,					0,

      // c0-ff: RDP commands mixed with uc2 commands
      0,                  0,                  0,                  0,
      0,                  0,                  0,                  0,
      0,                  0,                  0,                  0,
      0,                  0,                  0,                  0,
      0,                  0,                  0,                  0,
      0,			uc2_dlist_cnt,				0,					0,
      0,			0,			0,				fb_uc2_moveword,
      0,			uc2_load_ucode,			uc0_displaylist,	F3D_EndDL,
      0,			0,			  0, 				  0,
      fb_rect,         fb_rect,         	  0,                  0,
      0,                  0,                  0,                  0,
      0,         fb_setscissor,         0,       0,
      0,                  0,                  0,                  0,
      0,                  0,            	fb_rect,              0,
      0,                  0,                  0,                  0,
      0,         fb_settextureimage,    fb_setdepthimage,      fb_setcolorimage
   }
};
