/*
 * (C) Gražvydas "notaz" Ignotas, 2011
 *
 * This work is licensed under the terms of any of these licenses
 * (at your option):
 *  - GNU GPL, version 2 or later.
 *  - GNU LGPL, version 2.1 or later.
 * See the COPYING file in the top-level directory.
 */

#include <stdio.h>
#include <SDL.h>
#include <SDL_syswm.h>
#include "gpu.h"

static SDL_Surface *screen;
static Display *x11_display;

int vout_init(void)
{
  SDL_SysWMinfo wminfo;
  int ret;

  ret = SDL_Init(SDL_INIT_VIDEO);
  if (ret != 0) {
    fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
    return ret;
  }

  screen = SDL_SetVideoMode(1024, 512, 32, 0);
  if (screen == NULL) {
    fprintf(stderr, "SDL_SetVideoMode failed: %s\n", SDL_GetError());
    SDL_Quit();
    return -1;
  }

  SDL_VERSION(&wminfo.version);
  ret = SDL_GetWMInfo(&wminfo);
  if (ret == 1)
    x11_display = wminfo.info.x11.display;

  return 0;
}

int vout_finish(void)
{
  SDL_Quit();
  return 0;
}

int vout_update(void)
{
  uint32_t *d;
  int i;

  SDL_LockSurface(screen);
  if (gpu.status & PSX_GPU_STATUS_RGB24)
  {
    uint8_t *s;
    int y;
    for (y = 0; y < 512; y++) {
      s = (uint8_t *)gpu.vram + y * 2*1024;
      d = (uint32_t *)screen->pixels + y * 1024;
      for (i = 0; i < 1024 * 2 / 3; i++, s += 3)
        d[i] = (s[0] << 16) | (s[1] << 8) | s[2];
    }
  }
  else
  {
    uint16_t *s = gpu.vram;
    d = (uint32_t *)screen->pixels;
    for (i = 0; i < 1024 * 512; i++)
      d[i] = (((uint32_t)s[i] << 19) & 0xf80000) | ((s[i] << 6) & 0xf800) |
        ((s[i] >> 7) & 0xf8);
  }
  SDL_UnlockSurface(screen);
  SDL_UpdateRect(screen, 0, 0, 1024, 512);
  return 1;
}

void vout_blank(void)
{
}

long GPUopen(unsigned long *disp, char *cap, char *cfg)
{
  *disp = (long)x11_display;
  return 0;
}

long GPUclose(void)
{
  return 0;
}

void vout_set_config(const struct rearmed_cbs *cbs)
{
}

// vim:shiftwidth=2:expandtab
