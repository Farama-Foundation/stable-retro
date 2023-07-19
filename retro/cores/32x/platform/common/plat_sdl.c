/*
 * PicoDrive
 * (C) notaz, 2013
 * (C) kub, 2020-2022
 *
 * This work is licensed under the terms of MAME license.
 * See COPYING file in the top-level directory.
 */

#include <stdio.h>

#include "../libpicofe/input.h"
#include "../libpicofe/plat.h"
#include "../libpicofe/plat_sdl.h"
#include "../libpicofe/in_sdl.h"
#include "../libpicofe/gl.h"
#include "emu.h"
#include "menu_pico.h"
#include "input_pico.h"
#include "plat_sdl.h"
#include "version.h"

#include <pico/pico_int.h>

static void *shadow_fb;
static int shadow_size;
static struct area { int w, h; } area;

static struct in_pdata in_sdl_platform_data = {
	.defbinds = in_sdl_defbinds,
	.key_map = in_sdl_key_map,
	.joy_map = in_sdl_joy_map,
};

/* YUV stuff */
static int yuv_ry[32], yuv_gy[32], yuv_by[32];
static unsigned char yuv_u[32 * 2], yuv_v[32 * 2];
static unsigned char yuv_y[256];
static struct uyvy { uint32_t y:8; uint32_t vyu:24; } yuv_uyvy[65536];

void bgr_to_uyvy_init(void)
{
  int i, v;

  /* init yuv converter:
    y0 = (int)((0.299f * r0) + (0.587f * g0) + (0.114f * b0));
    y1 = (int)((0.299f * r1) + (0.587f * g1) + (0.114f * b1));
    u = (int)(8 * 0.565f * (b0 - y0)) + 128;
    v = (int)(8 * 0.713f * (r0 - y0)) + 128;
  */
  for (i = 0; i < 32; i++) {
    yuv_ry[i] = (int)(0.299f * i * 65536.0f + 0.5f);
    yuv_gy[i] = (int)(0.587f * i * 65536.0f + 0.5f);
    yuv_by[i] = (int)(0.114f * i * 65536.0f + 0.5f);
  }
  for (i = -32; i < 32; i++) {
    v = (int)(8 * 0.565f * i) + 128;
    if (v < 0)
      v = 0;
    if (v > 255)
      v = 255;
    yuv_u[i + 32] = v;
    v = (int)(8 * 0.713f * i) + 128;
    if (v < 0)
      v = 0;
    if (v > 255)
      v = 255;
    yuv_v[i + 32] = v;
  }
  // valid Y range seems to be 16..235
  for (i = 0; i < 256; i++) {
    yuv_y[i] = 16 + 219 * i / 32;
  }
  // everything combined into one large array for speed
  for (i = 0; i < 65536; i++) {
     int r = (i >> 11) & 0x1f, g = (i >> 6) & 0x1f, b = (i >> 0) & 0x1f;
     int y = (yuv_ry[r] + yuv_gy[g] + yuv_by[b]) >> 16;
     yuv_uyvy[i].y = yuv_y[y];
#if CPU_IS_LE
     yuv_uyvy[i].vyu = (yuv_v[r-y + 32] << 16) | (yuv_y[y] << 8) | yuv_u[b-y + 32];
#else
     yuv_uyvy[i].vyu = (yuv_v[b-y + 32] << 16) | (yuv_y[y] << 8) | yuv_u[r-y + 32];
#endif
  }
}

void rgb565_to_uyvy(void *d, const void *s, int w, int h, int pitch, int dpitch, int x2)
{
  uint32_t *dst = d;
  const uint16_t *src = s;
  int i;

  if (x2) while (h--) {
    for (i = w; i >= 4; src += 4, dst += 4, i -= 4)
    {
      struct uyvy *uyvy0 = yuv_uyvy + src[0], *uyvy1 = yuv_uyvy + src[1];
      struct uyvy *uyvy2 = yuv_uyvy + src[2], *uyvy3 = yuv_uyvy + src[3];
#if CPU_IS_LE
      dst[0] = (uyvy0->y << 24) | uyvy0->vyu;
      dst[1] = (uyvy1->y << 24) | uyvy1->vyu;
      dst[2] = (uyvy2->y << 24) | uyvy2->vyu;
      dst[3] = (uyvy3->y << 24) | uyvy3->vyu;
#else
      dst[0] = uyvy0->y | (uyvy0->vyu << 8);
      dst[1] = uyvy1->y | (uyvy1->vyu << 8);
      dst[2] = uyvy2->y | (uyvy2->vyu << 8);
      dst[3] = uyvy3->y | (uyvy3->vyu << 8);
#endif
    }
    src += pitch - (w-i);
    dst += (dpitch - 2*(w-i))/2;
  } else while (h--) {
    for (i = w; i >= 4; src += 4, dst += 2, i -= 4)
    {
      struct uyvy *uyvy0 = yuv_uyvy + src[0], *uyvy1 = yuv_uyvy + src[1];
      struct uyvy *uyvy2 = yuv_uyvy + src[2], *uyvy3 = yuv_uyvy + src[3];
#if CPU_IS_LE
      dst[0] = (uyvy1->y << 24) | uyvy0->vyu;
      dst[1] = (uyvy3->y << 24) | uyvy2->vyu;
#else
      dst[0] = uyvy1->y | (uyvy0->vyu << 8);
      dst[1] = uyvy3->y | (uyvy2->vyu << 8);
#endif
    }
    src += pitch - (w-i);
    dst += (dpitch - (w-i))/2;
  }
}

static int clear_buf_cnt, clear_stat_cnt;

static void resize_buffers(void)
{
	// make sure the shadow buffers are big enough in case of resize
	if (shadow_size < g_menuscreen_w * g_menuscreen_h * 2) {
		shadow_size = g_menuscreen_w * g_menuscreen_h * 2;
		shadow_fb = realloc(shadow_fb, shadow_size);
		g_menubg_ptr = realloc(g_menubg_ptr, shadow_size);
	}
}

void plat_video_set_size(int w, int h)
{
	if (area.w != w || area.h != h) {
		area = (struct area) { w, h };
		if (plat_sdl_change_video_mode(w, h, 0) < 0) {
			// failed, revert to original resolution
			area = (struct area) { g_screen_width,g_screen_height };
			plat_sdl_change_video_mode(g_screen_width, g_screen_height, 0);
		}
		if (!plat_sdl_overlay && !plat_sdl_gl_active) {
			g_screen_width = plat_sdl_screen->w;
			g_screen_height = plat_sdl_screen->h;
			g_screen_ppitch = plat_sdl_screen->pitch/2;
			g_screen_ptr = plat_sdl_screen->pixels;
		} else {
			g_screen_width = w;
			g_screen_height = h;
			g_screen_ppitch = w;
		}
	}
}

void plat_video_set_shadow(int w, int h)
{
	g_screen_width = w;
	g_screen_height = h;
	g_screen_ppitch = w;
	g_screen_ptr = shadow_fb;
}

void plat_video_flip(void)
{
	resize_buffers();

	if (plat_sdl_overlay != NULL) {
		SDL_Rect dstrect =
			{ 0, 0, plat_sdl_screen->w, plat_sdl_screen->h };
		SDL_LockYUVOverlay(plat_sdl_overlay);
		if (area.w <= plat_sdl_overlay->w && area.h <= plat_sdl_overlay->h)
			rgb565_to_uyvy(plat_sdl_overlay->pixels[0], shadow_fb,
					area.w, area.h, g_screen_ppitch,
					plat_sdl_overlay->pitches[0]/2,
					plat_sdl_overlay->w >= 2*area.w);
		SDL_UnlockYUVOverlay(plat_sdl_overlay);
		SDL_DisplayYUVOverlay(plat_sdl_overlay, &dstrect);
	}
	else if (plat_sdl_gl_active) {
		gl_flip(shadow_fb, g_screen_ppitch, g_screen_height);
	}
	else {
		if (SDL_MUSTLOCK(plat_sdl_screen)) {
			SDL_UnlockSurface(plat_sdl_screen);
			SDL_Flip(plat_sdl_screen);
			SDL_LockSurface(plat_sdl_screen);
		} else
			SDL_Flip(plat_sdl_screen);
		g_screen_ppitch = plat_sdl_screen->pitch/2;
		g_screen_ptr = plat_sdl_screen->pixels;
		plat_video_set_buffer(g_screen_ptr);
		if (clear_buf_cnt) {
			memset(g_screen_ptr, 0, plat_sdl_screen->pitch*plat_sdl_screen->h);
			clear_buf_cnt--;
		}
	}

	// for overlay/gl modes buffer ptr may change on resize
	if ((plat_sdl_overlay || plat_sdl_gl_active) &&
	    (g_screen_ptr != shadow_fb || g_screen_ppitch != g_screen_width)) {
		g_screen_ppitch = g_screen_width;
		g_screen_ptr = shadow_fb;
		plat_video_set_buffer(g_screen_ptr);
	}
	if (clear_stat_cnt) {
		unsigned short *d = (unsigned short *)g_screen_ptr + g_screen_ppitch * g_screen_height;
		int l = g_screen_ppitch * 8;
		memset((int *)(d - l), 0, l * 2);
		clear_stat_cnt--;
	}
}

void plat_video_wait_vsync(void)
{
}

void plat_video_clear_status(void)
{
	clear_stat_cnt = 3; // do it thrice in case of triple buffering
}

void plat_video_clear_buffers(void)
{
	if (plat_sdl_overlay || plat_sdl_gl_active)
		memset(shadow_fb, 0, g_menuscreen_w * g_menuscreen_h * 2);
	else {
		memset(g_screen_ptr, 0, plat_sdl_screen->pitch*plat_sdl_screen->h);
		clear_buf_cnt = 3; // do it thrice in case of triple buffering
	}
}

void plat_video_menu_enter(int is_rom_loaded)
{
	if (SDL_MUSTLOCK(plat_sdl_screen))
		SDL_UnlockSurface(plat_sdl_screen);
}

void plat_video_menu_begin(void)
{
	plat_sdl_change_video_mode(g_menuscreen_w, g_menuscreen_h, 1);
	resize_buffers();
	if (plat_sdl_overlay || plat_sdl_gl_active) {
		g_menuscreen_pp = g_menuscreen_w;
		g_menuscreen_ptr = shadow_fb;
	}
	else {
		if (SDL_MUSTLOCK(plat_sdl_screen))
			SDL_LockSurface(plat_sdl_screen);
		g_menuscreen_pp = plat_sdl_screen->pitch / 2;
		g_menuscreen_ptr = plat_sdl_screen->pixels;
	}
}

void plat_video_menu_end(void)
{
	if (plat_sdl_overlay != NULL) {
		SDL_Rect dstrect =
			{ 0, 0, plat_sdl_screen->w, plat_sdl_screen->h };

		SDL_LockYUVOverlay(plat_sdl_overlay);
		if (g_menuscreen_w <= plat_sdl_overlay->w && g_menuscreen_h <= plat_sdl_overlay->h)
			rgb565_to_uyvy(plat_sdl_overlay->pixels[0], shadow_fb,
				g_menuscreen_w, g_menuscreen_h, g_menuscreen_pp,
				plat_sdl_overlay->pitches[0]/2,
				plat_sdl_overlay->w >= 2 * g_menuscreen_w);
		SDL_UnlockYUVOverlay(plat_sdl_overlay);

		SDL_DisplayYUVOverlay(plat_sdl_overlay, &dstrect);
	}
	else if (plat_sdl_gl_active) {
		gl_flip(g_menuscreen_ptr, g_menuscreen_pp, g_menuscreen_h);
	}
	else {
		if (SDL_MUSTLOCK(plat_sdl_screen))
			SDL_UnlockSurface(plat_sdl_screen);
		SDL_Flip(plat_sdl_screen);
	}
	g_menuscreen_ptr = NULL;
}

void plat_video_menu_leave(void)
{
}

void plat_video_loop_prepare(void)
{
	// take over any new vout settings
	plat_sdl_change_video_mode(0, 0, 0);
	area.w = g_menuscreen_w, area.h = g_menuscreen_h;
	resize_buffers();

	// switch over to scaled output if available, but keep the aspect ratio
	if (plat_sdl_overlay || plat_sdl_gl_active) {
		if (g_menuscreen_w * 240 >= g_menuscreen_h * 320) {
			g_screen_width = (240 * g_menuscreen_w/g_menuscreen_h) & ~1;
			g_screen_height= 240;
		} else {
			g_screen_width = 320;
			g_screen_height= (320 * g_menuscreen_h/g_menuscreen_w) & ~1;
		}
		g_screen_ppitch = g_screen_width;
		g_screen_ptr = shadow_fb;
	}
	else {
		g_screen_width = plat_sdl_screen->w;
		g_screen_height = plat_sdl_screen->h;
		g_screen_ppitch = plat_sdl_screen->pitch/2;
		if (SDL_MUSTLOCK(plat_sdl_screen))
			SDL_LockSurface(plat_sdl_screen);
		g_screen_ptr = plat_sdl_screen->pixels;
	}

	plat_video_set_size(g_screen_width, g_screen_height);
	plat_video_set_buffer(g_screen_ptr);
}

void plat_early_init(void)
{
}

static void plat_sdl_resize(int w, int h)
{
	// take over new settings
	if (plat_sdl_screen->w != area.w || plat_sdl_screen->h != area.h) {
		g_menuscreen_h = plat_sdl_screen->h;
		g_menuscreen_w = plat_sdl_screen->w;
		resize_buffers();
		rendstatus_old = -1;
	}
}

static void plat_sdl_quit(void)
{
	// for now..
	exit(1);
}

void plat_init(void)
{
	int ret;

	ret = plat_sdl_init();
	if (ret != 0)
		exit(1);
#if defined(__RG350__) || defined(__GCW0__) || defined(__OPENDINGUX__) || defined(__RG99__)
	// opendingux on JZ47x0 may falsely report a HW overlay, fix to window
	plat_target.vout_method = 0;
#endif

	plat_sdl_quit_cb = plat_sdl_quit;
	plat_sdl_resize_cb = plat_sdl_resize;

	SDL_ShowCursor(0);
	SDL_WM_SetCaption("PicoDrive " VERSION, NULL);

	g_menuscreen_pp = g_menuscreen_w;
	g_menuscreen_ptr = NULL;

	shadow_size = g_menuscreen_w * g_menuscreen_h * 2;
	if (shadow_size < 320 * 480 * 2)
		shadow_size = 320 * 480 * 2;

	shadow_fb = calloc(1, shadow_size);
	g_menubg_ptr = calloc(1, shadow_size);
	if (shadow_fb == NULL || g_menubg_ptr == NULL) {
		fprintf(stderr, "OOM\n");
		exit(1);
	}

	g_screen_width = 320;
	g_screen_height = 240;
	g_screen_ppitch = 320;
	g_screen_ptr = shadow_fb;

	in_sdl_platform_data.kmap_size = in_sdl_key_map_sz,
	in_sdl_platform_data.jmap_size = in_sdl_joy_map_sz,
	in_sdl_platform_data.key_names = *in_sdl_key_names,
	in_sdl_init(&in_sdl_platform_data, plat_sdl_event_handler);
	in_probe();

#if defined(__RG99__)
	// do not use the default resolution
	plat_sdl_change_video_mode(320, 240, 1);
#endif

	bgr_to_uyvy_init();
}

void plat_finish(void)
{
	free(shadow_fb);
	shadow_fb = NULL;
	free(g_menubg_ptr);
	g_menubg_ptr = NULL;
	plat_sdl_finish();
}
