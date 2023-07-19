/*
 * (C) Gražvydas "notaz" Ignotas, 2012
 *
 * This work is licensed under the terms of any of these licenses
 * (at your option):
 *  - GNU GPL, version 2 or later.
 *  - GNU LGPL, version 2.1 or later.
 *  - MAME license.
 * See the COPYING file in the top-level directory.
 */

#include <stdio.h>
#include <SDL.h>
#include <SDL_syswm.h>

#include "menu.h"
#include "plat.h"
#include "gl.h"
#include "plat_sdl.h"

// XXX: maybe determine this instead..
#define WM_DECORATION_H 32

SDL_Surface *plat_sdl_screen;
SDL_Overlay *plat_sdl_overlay;
int plat_sdl_gl_active;
void (*plat_sdl_resize_cb)(int w, int h);
void (*plat_sdl_quit_cb)(void);

static char vid_drv_name[32];
static int window_w, window_h, window_b;
static int fs_w, fs_h;
static int old_fullscreen;
static int screen_flags;
static int vout_mode_overlay = -1, vout_mode_overlay2x = -1, vout_mode_gl = -1;
static void *display, *window;
static int gl_quirks;

/* w, h is layer resolution */
int plat_sdl_change_video_mode(int w, int h, int force)
{
  static int prev_w, prev_h;

  // skip GL recreation if window doesn't change - avoids flicker
  if (plat_target.vout_method == vout_mode_gl && plat_sdl_gl_active
      && plat_target.vout_fullscreen == old_fullscreen
      && w == prev_w && h == prev_h && !force)
  {
    return 0;
  }

  if (w == 0)
    w = prev_w;
  else
    prev_w = w;
  if (h == 0)
    h = prev_h;
  else
    prev_h = h;

  // invalid method might come from config..
  if (plat_target.vout_method != 0
      && plat_target.vout_method != vout_mode_overlay
      && plat_target.vout_method != vout_mode_overlay2x
      && plat_target.vout_method != vout_mode_gl)
  {
    fprintf(stderr, "invalid vout_method: %d\n", plat_target.vout_method);
    plat_target.vout_method = 0;
  }

  if (plat_sdl_overlay != NULL) {
    SDL_FreeYUVOverlay(plat_sdl_overlay);
    plat_sdl_overlay = NULL;
  }
  if (plat_sdl_gl_active) {
    gl_finish();
    plat_sdl_gl_active = 0;
  }

  if (plat_target.vout_method != 0) {
    Uint32 flags = SDL_RESIZABLE | SDL_SWSURFACE;
    int win_w = window_w;
    int win_h = window_h;

    if (plat_target.vout_fullscreen) {
      flags |= SDL_FULLSCREEN;
      win_w = fs_w;
      win_h = fs_h;
    }

    // XXX: workaround some occasional mysterious deadlock in SDL_SetVideoMode
    // (seen on r-pi)
    SDL_PumpEvents();

    if (!plat_sdl_screen || screen_flags != flags ||
        plat_sdl_screen->w != win_w || plat_sdl_screen->h != win_h)
      plat_sdl_screen = SDL_SetVideoMode(win_w, win_h, 0, flags);
    screen_flags = flags;
    if (plat_sdl_screen == NULL) {
      fprintf(stderr, "SDL_SetVideoMode failed: %s\n", SDL_GetError());
      plat_target.vout_method = 0;
    }
  }

  if (plat_target.vout_method == vout_mode_overlay
      || plat_target.vout_method == vout_mode_overlay2x) {
    int W = plat_target.vout_method == vout_mode_overlay2x && w < 640 ? 2*w : w;
    plat_sdl_overlay = SDL_CreateYUVOverlay(W, h, SDL_UYVY_OVERLAY, plat_sdl_screen);
    if (plat_sdl_overlay != NULL && SDL_LockYUVOverlay(plat_sdl_overlay) == 0) {
      if ((long)plat_sdl_overlay->pixels[0] & 3)
        fprintf(stderr, "warning: overlay pointer is unaligned\n");

      plat_sdl_overlay_clear();

      SDL_UnlockYUVOverlay(plat_sdl_overlay);
    }
    else {
      fprintf(stderr, "warning: could not create overlay.\n");
      plat_target.vout_method = 0;
    }
  }
  else if (plat_target.vout_method == vout_mode_gl) {
    int sw = plat_sdl_screen->w, sh = plat_sdl_screen->h;
    plat_sdl_gl_active = (gl_init(display, window, &gl_quirks, sw, sh) == 0);
    if (!plat_sdl_gl_active) {
      fprintf(stderr, "warning: could not init GL.\n");
      plat_target.vout_method = 0;
    }
  }

  if (plat_target.vout_method == 0) {
    Uint32 flags;
    int win_w = w;
    int win_h = h;

#if defined SDL_SURFACE_SW
    flags = SDL_SWSURFACE;
#elif defined(SDL_TRIPLEBUF) && defined(SDL_BUFFER_3X)
    flags = SDL_HWSURFACE | SDL_TRIPLEBUF;
#else
    flags = SDL_HWSURFACE | SDL_DOUBLEBUF;
#endif
    if (plat_target.vout_fullscreen && fs_w && fs_h) {
      flags |= SDL_FULLSCREEN;
      win_w = fs_w;
      win_h = fs_h;
    }

    SDL_PumpEvents();

    if (!plat_sdl_screen || screen_flags != flags ||
        plat_sdl_screen->w != win_w || plat_sdl_screen->h != win_h)
      plat_sdl_screen = SDL_SetVideoMode(win_w, win_h, 16, flags);
    screen_flags = flags;
    if (plat_sdl_screen == NULL) {
      fprintf(stderr, "SDL_SetVideoMode failed: %s\n", SDL_GetError());
      return -1;
    }
  }

  old_fullscreen = plat_target.vout_fullscreen;
  if (plat_sdl_resize_cb != NULL)
    plat_sdl_resize_cb(plat_sdl_screen->w, plat_sdl_screen->h);

  return 0;
}

void plat_sdl_event_handler(void *event_)
{
  static int was_active;
  SDL_Event *event = event_;

  switch (event->type) {
  case SDL_VIDEORESIZE:
    //printf("resize %dx%d\n", event->resize.w, event->resize.h);
    if (plat_target.vout_method != 0
        && !plat_target.vout_fullscreen && !old_fullscreen)
    {
      window_w = event->resize.w & ~3;
      window_h = event->resize.h & ~3;
      plat_sdl_change_video_mode(0, 0, 1);
    }
    break;
  case SDL_ACTIVEEVENT:
    if (event->active.gain && !was_active) {
      if (plat_sdl_overlay != NULL) {
        SDL_Rect dstrect = { 0, 0, plat_sdl_screen->w, plat_sdl_screen->h };
        SDL_DisplayYUVOverlay(plat_sdl_overlay, &dstrect);
      }
      else if (plat_sdl_gl_active) {
        if (gl_quirks & GL_QUIRK_ACTIVATE_RECREATE) {
          int sw = plat_sdl_screen->w, sh = plat_sdl_screen->h;
          gl_finish();
          plat_sdl_gl_active = (gl_init(display, window, &gl_quirks, sw, sh) == 0);
        }
        gl_flip(NULL, 0, 0);
      }
      // else SDL takes care of it
    }
    was_active = event->active.gain;
    break;
  case SDL_QUIT:
    if (plat_sdl_quit_cb != NULL)
      plat_sdl_quit_cb();
    break;
  }
}

int plat_sdl_init(void)
{
  static const char *vout_list[] = { NULL, NULL, NULL, NULL, NULL };
  const SDL_VideoInfo *info;
  SDL_SysWMinfo wminfo;
  const char *env;
  int overlay_works = 0;
  int gl_works = 0;
  int i, ret, h;
  int try_gl;

  ret = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE);
  if (ret != 0) {
    fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
    return -1;
  }

  info = SDL_GetVideoInfo();
  if (info != NULL) {
    fs_w = info->current_w;
    fs_h = info->current_h;
    if (info->wm_available)
      window_b = WM_DECORATION_H;
    printf("plat_sdl: using %dx%d as fullscreen resolution\n", fs_w, fs_h);
  }

  g_menuscreen_w = 640;
  if (fs_w != 0 && g_menuscreen_w > fs_w)
    g_menuscreen_w = fs_w;
  g_menuscreen_h = 480;
  if (fs_h != 0) {
    h = fs_h;
    if (window_b && h > window_b)
      h -= window_b;
    if (g_menuscreen_h > h)
      g_menuscreen_h = h;
  }

  plat_sdl_screen = SDL_SetVideoMode(g_menuscreen_w, g_menuscreen_h, 16, SDL_HWSURFACE);
  if (plat_sdl_screen == NULL) {
    plat_sdl_screen = SDL_SetVideoMode(0, 0, 16, SDL_SWSURFACE);
    if (plat_sdl_screen == NULL) {
      fprintf(stderr, "SDL_SetVideoMode failed: %s\n", SDL_GetError());
      goto fail;
    }

    if (plat_sdl_screen->w < 320 || plat_sdl_screen->h < 240) {
      fprintf(stderr, "resolution %dx%d is too small, sorry.\n",
              plat_sdl_screen->w, plat_sdl_screen->h);
      goto fail;
    }
  }
  g_menuscreen_w = window_w = plat_sdl_screen->w;
  g_menuscreen_h = window_h = plat_sdl_screen->h;
  g_menuscreen_pp = g_menuscreen_w;

  // overlay/gl require native bpp in some cases..
  plat_sdl_screen = SDL_SetVideoMode(g_menuscreen_w, g_menuscreen_h,
    0, plat_sdl_screen->flags);
  if (plat_sdl_screen == NULL) {
    fprintf(stderr, "SDL_SetVideoMode failed: %s\n", SDL_GetError());
    goto fail;
  }

  plat_sdl_overlay = SDL_CreateYUVOverlay(plat_sdl_screen->w, plat_sdl_screen->h,
    SDL_UYVY_OVERLAY, plat_sdl_screen);
  if (plat_sdl_overlay != NULL) {
    printf("plat_sdl: overlay: fmt %x, planes: %d, pitch: %d, hw: %d\n",
      plat_sdl_overlay->format, plat_sdl_overlay->planes, *plat_sdl_overlay->pitches,
      plat_sdl_overlay->hw_overlay);

    if (plat_sdl_overlay->hw_overlay)
      overlay_works = 1;
    else
      fprintf(stderr, "warning: video overlay is not hardware accelerated, "
                      "not going to use it.\n");
    SDL_FreeYUVOverlay(plat_sdl_overlay);
    plat_sdl_overlay = NULL;
  }
  else
    fprintf(stderr, "overlay is not available.\n");

  // get x11 display/window for GL
  SDL_VideoDriverName(vid_drv_name, sizeof(vid_drv_name));
#ifdef SDL_VIDEO_DRIVER_X11
  if (strcmp(vid_drv_name, "x11") == 0) {
    SDL_VERSION(&wminfo.version);
    ret = SDL_GetWMInfo(&wminfo);
    if (ret > 0) {
      display = wminfo.info.x11.display;
      window = (void *)wminfo.info.x11.window;
    }
  }
#else
  (void)wminfo;
#endif

  ret = -1;
  try_gl = 1;
  env = getenv("DISPLAY");
  if (env && env[0] != ':') {
    fprintf(stderr, "looks like a remote DISPLAY, "
      "not trying GL (use PICOFE_GL=1 to override)\n");
    // because some drivers just kill the program with no way to recover
    try_gl = 0;
  }
  env = getenv("PICOFE_GL");
  if (env)
    try_gl = atoi(env);
  if (try_gl)
    ret = gl_init(display, window, &gl_quirks, g_menuscreen_w, g_menuscreen_h);
  if (ret == 0) {
    gl_announce();
    gl_works = 1;
    gl_finish();
  }

  i = 0;
  vout_list[i++] = "SDL Window";
  if (overlay_works) {
    plat_target.vout_method = vout_mode_overlay = i;
    vout_list[i++] = "Video Overlay";
#ifdef SDL_OVERLAY_2X
    vout_mode_overlay2x = i;
    vout_list[i++] = "Video Overlay 2x";
#endif
  }
  if (gl_works) {
    plat_target.vout_method = vout_mode_gl = i;
    vout_list[i++] = "OpenGL";
  }
  plat_target.vout_methods = vout_list;

  plat_sdl_change_video_mode(g_menuscreen_w, g_menuscreen_h, 1);
  return 0;

fail:
  SDL_Quit();
  return -1;
}

void plat_sdl_finish(void)
{
  if (plat_sdl_overlay != NULL) {
    SDL_FreeYUVOverlay(plat_sdl_overlay);
    plat_sdl_overlay = NULL;
  }
  if (plat_sdl_gl_active) {
    gl_finish();
    plat_sdl_gl_active = 0;
  }
  // restore back to initial resolution
  // resolves black screen issue on R-Pi
  if (strcmp(vid_drv_name, "x11") != 0)
    SDL_SetVideoMode(fs_w, fs_h, 16, SDL_SWSURFACE);
  SDL_Quit();
}

void plat_sdl_overlay_clear(void)
{
  int pixels = plat_sdl_overlay->w * plat_sdl_overlay->h;
  int *dst = (int *)plat_sdl_overlay->pixels[0];
  int v = 0x10801080;

  for (; pixels > 7; dst += 4, pixels -= 2 * 4)
    dst[0] = dst[1] = dst[2] = dst[3] = v;

  for (; pixels > 1; dst++, pixels -= 2)
    *dst = v;
}

// vim:shiftwidth=2:expandtab
