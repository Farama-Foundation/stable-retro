#if !defined(NEON_BUILD) || defined(SIMD_BUILD)

#ifndef zip_4x32b

#define vector_cast(vec_to, source) source

#define zip_4x32b(dest, source_a, source_b) {                                  \
  u32 _i; for(_i = 0; _i < 4; _i++) {                                          \
    (dest).e[_i * 2 + 0] = (source_a).e[_i];                                   \
    (dest).e[_i * 2 + 1] = (source_b).e[_i];                                   \
  }                                                                            \
}

#endif

void setup_sprite_16bpp_4x(psx_gpu_struct *psx_gpu, s32 x, s32 y, s32 u,
 s32 v, s32 width, s32 height, u32 color)
{
  u32 left_offset = u & 0x7;
  u32 width_rounded = width + left_offset + 7;

  u16 *fb_ptr = psx_gpu->vram_out_ptr + (y * 1024) + (s32)(x - left_offset * 2);
  u32 right_width = width_rounded & 0x7;
  u32 block_width = width_rounded / 8;
  u32 fb_ptr_pitch = (2048 + 16) - (block_width * 16);

  u32 left_mask_bits = ~(0xFFFF << (left_offset * 2));
  u32 right_mask_bits = 0xFFFC << (right_width * 2);

  u32 texture_offset_base = u + (v * 1024);
  u32 texture_mask =
   psx_gpu->texture_mask_width | (psx_gpu->texture_mask_height * 1024);

  u32 blocks_remaining;
  u32 num_blocks = psx_gpu->num_blocks;
  block_struct *block = psx_gpu->blocks + num_blocks;

  u16 *texture_page_ptr = psx_gpu->texture_page_ptr;
  u16 *texture_block_ptr;

  texture_offset_base &= ~0x7;

  sprites_16bpp++;

  if(block_width == 1)
  {
    u32 mask_bits = left_mask_bits | right_mask_bits;
    u32 mask_bits_a = mask_bits & 0xFF;
    u32 mask_bits_b = mask_bits >> 8;
    
    vec_8x16u texels;
    vec_8x16u texels_wide;

    while(height)
    {
      num_blocks += 4;
      sprite_blocks += 4;

      if(num_blocks > MAX_BLOCKS)
      {
        flush_render_block_buffer(psx_gpu);
        num_blocks = 4;
        block = psx_gpu->blocks;
      }
      
      texture_block_ptr =
       texture_page_ptr + (texture_offset_base & texture_mask);

      //load_128b(texels, texture_block_ptr);
      texels = *(vec_8x16u *)texture_block_ptr;
      
      zip_4x32b(vector_cast(vec_4x32u, texels_wide), texels.low, texels.low);
      block->texels = texels_wide;
      block->draw_mask_bits = mask_bits_a;
      block->fb_ptr = fb_ptr;          
      block++;
      
      block->texels = texels_wide;
      block->draw_mask_bits = mask_bits_a;
      block->fb_ptr = fb_ptr + 1024;          
      block++;
      
      zip_4x32b(vector_cast(vec_4x32u, texels_wide), texels.high, texels.high);
      block->texels = texels_wide;
      block->draw_mask_bits = mask_bits_b;
      block->fb_ptr = fb_ptr + 8;
      block++;
      
      block->texels = texels_wide;
      block->draw_mask_bits = mask_bits_b;
      block->fb_ptr = fb_ptr + 8 + 1024;          
      block++;      

      texture_offset_base += 1024;
      fb_ptr += 2048;

      height--;
      psx_gpu->num_blocks = num_blocks;
    }
  }
  else
  {
    u32 texture_offset;
    
    u32 left_mask_bits_a = left_mask_bits & 0xFF;
    u32 left_mask_bits_b = left_mask_bits >> 8;
    u32 right_mask_bits_a = right_mask_bits & 0xFF;
    u32 right_mask_bits_b = right_mask_bits >> 8;
    
    vec_8x16u texels;
    vec_8x16u texels_wide;    

    while(height)
    {
      blocks_remaining = block_width - 2;
      num_blocks += block_width * 4;
      sprite_blocks += block_width * 4;

      if(num_blocks > MAX_BLOCKS)
      {
        flush_render_block_buffer(psx_gpu);
        num_blocks = block_width * 4;
        block = psx_gpu->blocks;
      }

      texture_offset = texture_offset_base;
      texture_offset_base += 1024;

      texture_block_ptr = texture_page_ptr + (texture_offset & texture_mask);
      
      //load_128b(texels, texture_block_ptr);
      texels = *(vec_8x16u *)texture_block_ptr;

      zip_4x32b(vector_cast(vec_4x32u, texels_wide), texels.low, texels.low);
      block->texels = texels_wide;
      block->draw_mask_bits = left_mask_bits_a;
      block->fb_ptr = fb_ptr;
      block++;
      
      block->texels = texels_wide;
      block->draw_mask_bits = left_mask_bits_a;
      block->fb_ptr = fb_ptr + 1024;
      block++;      

      zip_4x32b(vector_cast(vec_4x32u, texels_wide), texels.high, texels.high);
      block->texels = texels_wide;
      block->draw_mask_bits = left_mask_bits_b;
      block->fb_ptr = fb_ptr + 8;
      block++;  
      
      block->texels = texels_wide;
      block->draw_mask_bits = left_mask_bits_b;
      block->fb_ptr = fb_ptr + 8 + 1024;
      block++;  
      
      texture_offset += 8;
      fb_ptr += 16;

      while(blocks_remaining)
      {
        texture_block_ptr = texture_page_ptr + (texture_offset & texture_mask);
        //load_128b(texels, texture_block_ptr);
        texels = *(vec_8x16u *)texture_block_ptr;

        zip_4x32b(vector_cast(vec_4x32u, texels_wide), texels.low, texels.low);
        block->texels = texels_wide;
        block->draw_mask_bits = 0;
        block->fb_ptr = fb_ptr;
        block++;
        
        block->texels = texels_wide;
        block->draw_mask_bits = 0;
        block->fb_ptr = fb_ptr + 1024;
        block++;      

        zip_4x32b(vector_cast(vec_4x32u, texels_wide), texels.high, texels.high);
        block->texels = texels_wide;
        block->draw_mask_bits = 0;
        block->fb_ptr = fb_ptr + 8;
        block++;
        
        block->texels = texels_wide;
        block->draw_mask_bits = 0;
        block->fb_ptr = fb_ptr + 8 + 1024;
        block++;
        
        texture_offset += 8;
        fb_ptr += 16;

        blocks_remaining--;
      }

      texture_block_ptr = texture_page_ptr + (texture_offset & texture_mask);
      //load_128b(texels, texture_block_ptr);
      texels = *(vec_8x16u *)texture_block_ptr;
      
      zip_4x32b(vector_cast(vec_4x32u, texels_wide), texels.low, texels.low);
      block->texels = texels_wide;
      block->draw_mask_bits = right_mask_bits_a;
      block->fb_ptr = fb_ptr;
      block++;
      
      block->texels = texels_wide;
      block->draw_mask_bits = right_mask_bits_a;
      block->fb_ptr = fb_ptr + 1024;
      block++;      

      zip_4x32b(vector_cast(vec_4x32u, texels_wide), texels.high, texels.high);
      block->texels = texels_wide;
      block->draw_mask_bits = right_mask_bits_b;
      block->fb_ptr = fb_ptr + 8;
      block++;

      block->texels = texels_wide;
      block->draw_mask_bits = right_mask_bits_b;
      block->fb_ptr = fb_ptr + 8 + 1024;      
      block++;

      fb_ptr += fb_ptr_pitch;

      height--;
      psx_gpu->num_blocks = num_blocks;
    }
  }
}

#endif

static void setup_sprite_untextured_4x(psx_gpu_struct *psx_gpu, s32 x, s32 y,
 s32 u, s32 v, s32 width, s32 height, u32 color)
{
  width *= 2;
  height *= 2;
  if (width > 1024)
    width = 1024;
  setup_sprite_untextured(psx_gpu, x, y, u, v, width, height, color);
}

#define setup_sprite_blocks_switch_textured_4x(texture_mode)                   \
  setup_sprite_##texture_mode##_4x                                             \

#define setup_sprite_blocks_switch_untextured_4x(texture_mode)                 \
  setup_sprite_untextured_4x                                                   \

#define setup_sprite_blocks_switch_4x(texturing, texture_mode)                 \
  setup_sprite_blocks_switch_##texturing##_4x(texture_mode)                    \

  
#define render_sprite_blocks_switch_block_modulation_4x(texture_mode,          \
 blend_mode, mask_evaluate, shading, dithering, texturing, blending,           \
 modulation)                                                                   \
{                                                                              \
  setup_sprite_blocks_switch_4x(texturing, texture_mode),                      \
  texture_sprite_blocks_switch_##texturing(texture_mode),                      \
  shade_blocks_switch(unshaded, texturing, modulation, undithered, blending,   \
   mask_evaluate),                                                             \
  blend_blocks_switch(texturing, blending, blend_mode, mask_evaluate)          \
}                                                                              \

#define render_sprite_blocks_switch_block_blending_4x(texture_mode,            \
 blend_mode, mask_evaluate, shading, dithering, texturing, blending)           \
  render_sprite_blocks_switch_block_modulation_4x(texture_mode, blend_mode,    \
   mask_evaluate, shading, dithering, texturing, blending, modulated),         \
  render_sprite_blocks_switch_block_modulation_4x(texture_mode, blend_mode,    \
   mask_evaluate, shading, dithering, texturing, blending, unmodulated)        \

#define render_sprite_blocks_switch_block_texturing_4x(texture_mode,           \
 blend_mode, mask_evaluate, shading, dithering, texturing)                     \
  render_sprite_blocks_switch_block_blending_4x(texture_mode, blend_mode,      \
   mask_evaluate, shading, dithering, texturing, unblended),                   \
  render_sprite_blocks_switch_block_blending_4x(texture_mode, blend_mode,      \
   mask_evaluate, shading, dithering, texturing, blended)                      \

#define render_sprite_blocks_switch_block_dithering_4x(texture_mode,           \
 blend_mode, mask_evaluate, shading, dithering)                                \
  render_sprite_blocks_switch_block_texturing_4x(texture_mode, blend_mode,     \
   mask_evaluate, shading, dithering, untextured),                             \
  render_sprite_blocks_switch_block_texturing_4x(texture_mode, blend_mode,     \
   mask_evaluate, shading, dithering, textured)                                \

#define render_sprite_blocks_switch_block_shading_4x(texture_mode, blend_mode, \
 mask_evaluate, shading)                                                       \
  render_sprite_blocks_switch_block_dithering_4x(texture_mode, blend_mode,     \
   mask_evaluate, shading, undithered),                                        \
  render_sprite_blocks_switch_block_dithering_4x(texture_mode, blend_mode,     \
   mask_evaluate, shading, dithered)                                           \

#define render_sprite_blocks_switch_block_mask_evaluate_4x(texture_mode,       \
 blend_mode, mask_evaluate)                                                    \
  render_sprite_blocks_switch_block_shading_4x(texture_mode, blend_mode,       \
   mask_evaluate, unshaded),                                                   \
  render_sprite_blocks_switch_block_shading_4x(texture_mode, blend_mode,       \
   mask_evaluate, shaded)                                                      \

#define render_sprite_blocks_switch_block_blend_mode_4x(texture_mode,          \
 blend_mode)                                                                   \
  render_sprite_blocks_switch_block_mask_evaluate_4x(texture_mode, blend_mode, \
   off),                                                                       \
  render_sprite_blocks_switch_block_mask_evaluate_4x(texture_mode, blend_mode, \
   on)                                                                         \

#define render_sprite_blocks_switch_block_texture_mode_4x(texture_mode)        \
  render_sprite_blocks_switch_block_blend_mode_4x(texture_mode, average),      \
  render_sprite_blocks_switch_block_blend_mode_4x(texture_mode, add),          \
  render_sprite_blocks_switch_block_blend_mode_4x(texture_mode, subtract),     \
  render_sprite_blocks_switch_block_blend_mode_4x(texture_mode, add_fourth)    \

#define render_sprite_blocks_switch_block_4x()                                 \
  render_sprite_blocks_switch_block_texture_mode_4x(4bpp),                     \
  render_sprite_blocks_switch_block_texture_mode_4x(8bpp),                     \
  render_sprite_blocks_switch_block_texture_mode_4x(16bpp),                    \
  render_sprite_blocks_switch_block_texture_mode_4x(16bpp)                     \


render_block_handler_struct render_sprite_block_handlers_4x[] =
{
  render_sprite_blocks_switch_block_4x()
};

void render_sprite_4x(psx_gpu_struct *psx_gpu, s32 x, s32 y, u32 u, u32 v,
 s32 width, s32 height, u32 flags, u32 color)
{
  s32 x_right = x + width - 1;
  s32 y_bottom = y + height - 1;
  s16 end_x;

#ifdef PROFILE
  sprites++;
#endif

  if(x < psx_gpu->viewport_start_x)
  {
    u32 clip = psx_gpu->viewport_start_x - x;
    x += clip;
    u += clip;
    width -= clip;
  }

  if(y < psx_gpu->viewport_start_y)
  {
    s32 clip = psx_gpu->viewport_start_y - y;
    y += clip;
    v += clip;
    height -= clip;
  }

  end_x = psx_gpu->viewport_end_x;
  if (end_x - psx_gpu->viewport_start_x + 1 > 512)
    end_x = psx_gpu->viewport_start_x + 511;

  if(x_right > end_x)
    width -= x_right - end_x;

  if(y_bottom > psx_gpu->viewport_end_y)
    height -= y_bottom - psx_gpu->viewport_end_y;

  if((width <= 0) || (height <= 0))
    return;

  if (!psx_gpu->enhancement_current_buf_ptr)
    return;
  psx_gpu->vram_out_ptr = psx_gpu->enhancement_current_buf_ptr;

  x *= 2;
  y *= 2;

#ifdef PROFILE
  span_pixels += width * height;
  spans += height;
#endif

  u32 render_state = flags &
   (RENDER_FLAGS_MODULATE_TEXELS | RENDER_FLAGS_BLEND |
   RENDER_FLAGS_TEXTURE_MAP);
  render_state |=
   (psx_gpu->render_state_base & ~RENDER_STATE_DITHER);

  if((psx_gpu->render_state != render_state) ||
   (psx_gpu->primitive_type != PRIMITIVE_TYPE_SPRITE))
  {
    psx_gpu->render_state = render_state;
    flush_render_block_buffer(psx_gpu);
#ifdef PROFILE
    state_changes++;
#endif
  }

  psx_gpu->primitive_type = PRIMITIVE_TYPE_SPRITE;

  color &= 0xFFFFFF;

  if(psx_gpu->triangle_color != color)
  {
    flush_render_block_buffer(psx_gpu);
    psx_gpu->triangle_color = color;
  }

  if(color == 0x808080)
    render_state |= RENDER_FLAGS_MODULATE_TEXELS;

  render_block_handler_struct *render_block_handler =
   &(render_sprite_block_handlers_4x[render_state]);
  psx_gpu->render_block_handler = render_block_handler;

  ((setup_sprite_function_type *)render_block_handler->setup_blocks)
   (psx_gpu, x, y, u, v, width, height, color);
}

