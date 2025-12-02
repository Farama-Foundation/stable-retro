/*
 * Copyright (C) 2011 Gilead Kutnick "Exophase" <exophase@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include <stdio.h>
#include <stdlib.h>

#include "SDL.h"
#include "common.h"

#include "../../gpulib/gpu.h"
#include "psx_gpu.c"
#include "psx_gpu_parse.c"

#pragma GCC diagnostic ignored "-Wunused-result"

extern u32 span_pixels;
extern u32 span_pixel_blocks;
extern u32 spans;
extern u32 triangles;
extern u32 sprites;
extern u32 sprites_4bpp;
extern u32 sprites_8bpp;
extern u32 sprites_16bpp;
extern u32 sprites_untextured;
extern u32 sprite_blocks;
extern u32 lines;
extern u32 texels_4bpp;
extern u32 texels_8bpp;
extern u32 texels_16bpp;
extern u32 texel_blocks_4bpp;
extern u32 texel_blocks_8bpp;
extern u32 texel_blocks_16bpp;
extern u32 texel_blocks_untextured;
extern u32 blend_blocks;
extern u32 render_buffer_flushes;
extern u32 state_changes;
extern u32 trivial_rejects;
extern u32 left_split_triangles;
extern u32 flat_triangles;
extern u32 clipped_triangles;
extern u32 zero_block_spans;
extern u32 texture_cache_loads;
extern u32 false_modulated_blocks;

static u32 mismatches;

typedef struct
{
	u16 vram[1024 * 512];
	u32 gpu_register[15];
	u32 status;
} gpu_dump_struct;

static gpu_dump_struct state;

psx_gpu_struct __attribute__((aligned(256))) _psx_gpu;
u16 __attribute__((aligned(256))) _vram[(1024 * 512) + 1024];

#define percent_of(numerator, denominator)                                     \
  ((((double)(numerator)) / (denominator)) * 100.0)                            \

void clear_stats(void)
{
  triangles = 0;
  sprites = 0;
  sprites_4bpp = 0;
  sprites_8bpp = 0;
  sprites_16bpp = 0;
  sprites_untextured = 0;
  sprite_blocks = 0;
  lines = 0;
  span_pixels = 0;
  span_pixel_blocks = 0;
  spans = 0;
  texels_4bpp = 0;
  texels_8bpp = 0;
  texels_16bpp = 0;
  texel_blocks_untextured = 0;
  texel_blocks_4bpp = 0;
  texel_blocks_8bpp = 0;
  texel_blocks_16bpp = 0;
  blend_blocks = 0;
  render_buffer_flushes = 0;
  state_changes = 0;
  trivial_rejects = 0;
  left_split_triangles = 0;
  flat_triangles = 0;
  clipped_triangles = 0;
  zero_block_spans = 0;
  texture_cache_loads = 0;
  false_modulated_blocks = 0;
}

void update_screen(psx_gpu_struct *psx_gpu, SDL_Surface *screen)
{
  u32 x, y;

  for(y = 0; y < 512; y++)
  {
    for(x = 0; x < 1024; x++)
    {
      u32 pixel = psx_gpu->vram_ptr[(y * 1024) + x];
      ((u32 *)screen->pixels)[(y * 1024) + x] =
       ((pixel & 0x1F) << (16 + 3)) |
       (((pixel >> 5) & 0x1F) << (8 + 3)) |
       (((pixel >> 10) & 0x1F) << 3);
    }
  }

  SDL_Flip(screen);
}

#ifdef NEON_BUILD

#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
  
#endif

int main(int argc, char *argv[])
{
  psx_gpu_struct *psx_gpu = &_psx_gpu;
  SDL_Surface *screen = NULL;
  SDL_Event event;

  u32 *list;
  int size;
  FILE *state_file;
  FILE *list_file;
  u32 no_display = 0;
  s32 dummy0 = 0;
  s32 dummy1 = 0;
  u32 dummy2 = 0;

  if((argc != 3) && (argc != 4))
  {
    printf("usage:\n%s <state> <list>\n", argv[0]);
    return 1;
  }

  if((argc == 4) && !strcmp(argv[3], "-n"))
    no_display = 1;
  
  state_file = fopen(argv[1], "rb");
  fread(&state, 1, sizeof(gpu_dump_struct), state_file);
  fclose(state_file);
  
  list_file = fopen(argv[2], "rb");
  
  fseek(list_file, 0, SEEK_END);
  size = ftell(list_file);
  fseek(list_file, 0, SEEK_SET);
  //size = 0;

  list = malloc(size);
  fread(list, 1, size, list_file);
  fclose(list_file);
 
  if(no_display == 0) 
  {
    SDL_Init(SDL_INIT_EVERYTHING);
    screen = SDL_SetVideoMode(1024, 512, 32, 0);
    if (screen == 0)
    {
      printf("can't set video mode: %s\n", SDL_GetError());
      return 1;
    }
  }

#ifdef NEON_BUILD
  u16 *vram_ptr;
#if 0
  system("ofbset -fb /dev/fb1 -mem 6291456 -en 0");
  u32 fbdev_handle = open("/dev/fb1", O_RDWR);
  vram_ptr = (mmap((void *)0x50000000, 1024 * 1024 * 2, PROT_READ | PROT_WRITE,
   MAP_SHARED | 0xA0000000, fbdev_handle, 0));
#elif 0
  #ifndef MAP_HUGETLB
  #define MAP_HUGETLB 0x40000 /* arch specific */
  #endif
  vram_ptr = (mmap((void *)0x50000000, 1024 * 1024 * 2, PROT_READ | PROT_WRITE,
   MAP_ANONYMOUS | MAP_PRIVATE | MAP_HUGETLB, -1, 0));
#else
  vram_ptr = (mmap((void *)0x50000000, 1024 * 1024 * 2, PROT_READ | PROT_WRITE,
   MAP_ANONYMOUS | MAP_PRIVATE, -1, 0));
#endif
  if (vram_ptr == MAP_FAILED)
  {
    perror("mmap");
    return 1;
  }
  vram_ptr += 64;

  initialize_psx_gpu(psx_gpu, vram_ptr);
#else
  initialize_psx_gpu(psx_gpu, _vram + 64);
#endif

#ifdef NEON_BUILD
  //triangle_benchmark(psx_gpu);
  //return 0;
#endif

  memcpy(psx_gpu->vram_ptr, state.vram, 1024 * 512 * 2);

  clear_stats();

#ifdef CYCLE_COUNTER
  init_counter();
#endif

  gpu_parse(psx_gpu, list, size, &dummy0, &dummy1, &dummy2);
  flush_render_block_buffer(psx_gpu);

  clear_stats();

#ifdef CYCLE_COUNTER
  u32 cycles = get_counter();
#endif

  gpu_parse(psx_gpu, list, size, &dummy0, &dummy1, &dummy2);
  flush_render_block_buffer(psx_gpu);

#ifdef CYCLE_COUNTER
  u32 cycles_elapsed = get_counter() - cycles;

  printf("%-64s: %d\n", argv[1], cycles_elapsed);
#else
  printf("%-64s: ", argv[1]);
#endif

#if 1
  u32 i;

  for(i = 0; i < 1024 * 512; i++)
  {
    if((psx_gpu->vram_ptr[i] & 0x7FFF) != (state.vram[i] & 0x7FFF))
    {
      printf("(%d %d %d) vs (%d %d %d) at (%d %d)\n",
       psx_gpu->vram_ptr[i] & 0x1F,
       (psx_gpu->vram_ptr[i] >> 5) & 0x1F,
       (psx_gpu->vram_ptr[i] >> 10) & 0x1F,
       state.vram[i] & 0x1F,
       (state.vram[i] >> 5) & 0x1F,
       (state.vram[i] >> 10) & 0x1F, i % 1024, i / 1024);

      mismatches++;
    }
    else
    {
      psx_gpu->vram_ptr[i] =
       ((psx_gpu->vram_ptr[i] & 0x1F) / 4) |
       ((((psx_gpu->vram_ptr[i] >> 5) & 0x1F) / 4) << 5) |
       ((((psx_gpu->vram_ptr[i] >> 10) & 0x1F) / 4) << 10);
    }
  }
#endif

#if 0
  printf("\n");
  printf("  %d pixels, %d pixel blocks, %d spans\n"
   "   (%lf pixels per block, %lf pixels per span),\n"
   "   %lf blocks per span (%lf per non-zero span), %lf overdraw)\n\n",
   span_pixels, span_pixel_blocks, spans,
   (double)span_pixels / span_pixel_blocks,
   (double)span_pixels / spans,
   (double)span_pixel_blocks / spans, 
   (double)span_pixel_blocks / (spans - zero_block_spans),
   (double)span_pixels / 
   ((psx_gpu->viewport_end_x - psx_gpu->viewport_start_x) * 
   (psx_gpu->viewport_end_y - psx_gpu->viewport_start_y)));

  printf("  %d triangles\n"
   "   (%d trivial rejects, %lf%% flat, %lf%% left split, %lf%% clipped)\n"
   "   (%lf pixels per triangle, %lf rows per triangle)\n\n",
   triangles, trivial_rejects,
   percent_of(flat_triangles, triangles),
   percent_of(left_split_triangles, triangles),
   percent_of(clipped_triangles, triangles),
   (double)span_pixels / triangles,
   (double)spans / triangles);

  printf("  Block data:\n");
  printf("   %7d 4bpp texel blocks  (%lf%%)\n", texel_blocks_4bpp,
   percent_of(texel_blocks_4bpp, span_pixel_blocks));
  printf("   %7d 8bpp texel blocks  (%lf%%)\n", texel_blocks_8bpp,
   percent_of(texel_blocks_8bpp, span_pixel_blocks));
  printf("   %7d 16bpp texel blocks (%lf%%)\n", texel_blocks_16bpp,
   percent_of(texel_blocks_16bpp, span_pixel_blocks));
  printf("   %7d untextured blocks  (%lf%%)\n", texel_blocks_untextured,
   percent_of(texel_blocks_untextured, span_pixel_blocks));
  printf("   %7d sprite blocks      (%lf%%)\n", sprite_blocks,  
   percent_of(sprite_blocks, span_pixel_blocks));
  printf("   %7d blended blocks     (%lf%%)\n", blend_blocks,
   percent_of(blend_blocks, span_pixel_blocks));
  printf("   %7d false-mod blocks   (%lf%%)\n", false_modulated_blocks,
   percent_of(false_modulated_blocks, span_pixel_blocks));
  printf("\n");
  printf("  %lf blocks per render buffer flush\n", (double)span_pixel_blocks /
   render_buffer_flushes);
  printf("  %d zero block spans\n", zero_block_spans);
  printf("  %d state changes, %d texture cache loads\n", state_changes,
   texture_cache_loads);
  if(sprites)
  {
    printf("  %d sprites\n"
     "    4bpp:       %lf%%\n"
     "    8bpp:       %lf%%\n"
     "    16bpp:      %lf%%\n"
     "    untextured: %lf%%\n",
     sprites, percent_of(sprites_4bpp, sprites),
     percent_of(sprites_8bpp, sprites), percent_of(sprites_16bpp, sprites),
     percent_of(sprites_untextured, sprites));
  }
  printf("  %d lines\n", lines);
  printf("\n");
  printf("  %d mismatches\n\n\n", mismatches);
#endif

  fflush(stdout);

  if(no_display == 0)
  {
    while(1)
    {
      update_screen(psx_gpu, screen);
  
      if(SDL_PollEvent(&event))
      {
        if((event.type == SDL_QUIT) ||
         ((event.type == SDL_KEYDOWN) &&
         (event.key.keysym.sym == SDLK_ESCAPE)))
        {
          break;
        }      
      }
  
      SDL_Delay(20);
    }
  }

  return (mismatches != 0);
}
