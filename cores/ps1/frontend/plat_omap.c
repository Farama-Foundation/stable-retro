/*
 * (C) Gražvydas "notaz" Ignotas, 2010-2012
 *
 * This work is licensed under the terms of the GNU GPLv2 or later.
 * See the COPYING file in the top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/omapfb.h>

#include "libpicofe/menu.h"
#include "libpicofe/input.h"
#include "libpicofe/linux/fbdev.h"
#include "libpicofe/linux/xenv.h"
#include "plugin_lib.h"
#include "pl_gun_ts.h"
#include "plat.h"
#include "plat_omap.h"
#include "menu.h"

static struct vout_fbdev *main_fb, *layer_fb;

static int omap_setup_layer_(int fd, int enabled, int x, int y, int w, int h)
{
	struct omapfb_plane_info pi = { 0, };
	struct omapfb_mem_info mi = { 0, };
	int ret;

	ret = ioctl(fd, OMAPFB_QUERY_PLANE, &pi);
	if (ret != 0) {
		perror("QUERY_PLANE");
		return -1;
	}

	ret = ioctl(fd, OMAPFB_QUERY_MEM, &mi);
	if (ret != 0) {
		perror("QUERY_MEM");
		return -1;
	}

	/* must disable when changing stuff */
	if (pi.enabled) {
		pi.enabled = 0;
		ret = ioctl(fd, OMAPFB_SETUP_PLANE, &pi);
		if (ret != 0)
			perror("SETUP_PLANE");
	}

	// upto 1024x512 (2x resolution enhancement)
	if (mi.size < 1024*512*2 * 3) {
		mi.size = 1024*512*2 * 3;
		ret = ioctl(fd, OMAPFB_SETUP_MEM, &mi);
		if (ret != 0) {
			perror("SETUP_MEM");
			return -1;
		}
	}

	pi.pos_x = x;
	pi.pos_y = y;
	pi.out_width = w;
	pi.out_height = h;
	pi.enabled = enabled;

	ret = ioctl(fd, OMAPFB_SETUP_PLANE, &pi);
	if (ret != 0) {
		perror("SETUP_PLANE");
		return -1;
	}

	return 0;
}

static int omap_enable_layer(int enabled)
{
	int x = g_layer_x, y = g_layer_y;
	int w = g_layer_w, h = g_layer_h;

	// it's not allowed for the layer to be partially offscreen,
	// instead it is faked by plat_gvideo_set_mode()
	if (x < 0) { w += x; x = 0; }
	if (y < 0) { h += y; y = 0; }
	if (x + w > 800) w = 800 - x;
	if (y + h > 480) h = 480 - y;

	if (enabled)
		pl_set_gun_rect(x, y, w, h);

	return omap_setup_layer_(vout_fbdev_get_fd(layer_fb),
		enabled, x, y, w, h);
}

void plat_omap_gvideo_open(void)
{
	omap_enable_layer(1);

	// try to align redraws to vsync
	vout_fbdev_wait_vsync(layer_fb);
}

void *plat_gvideo_set_mode(int *w_in, int *h_in, int *bpp)
{
	int l = 0, r = 0, t = 0, b = 0;
	int w = *w_in, h = *h_in;
	void *buf;

	if (g_scaler == SCALE_1_1 || g_scaler == SCALE_2_2) {
		if (w > g_menuscreen_w)
			l = r = (w - g_menuscreen_w) / 2;
		if (h > g_menuscreen_h)
			t = b = (h - g_menuscreen_h) / 2;
	}
	else if (g_scaler == SCALE_CUSTOM) {
		int right = g_layer_x + g_layer_w;
		int bottom = g_layer_y + g_layer_h;
		if (g_layer_x < 0)
			l = -g_layer_x * w / g_menuscreen_w;
		if (g_layer_y < 0)
			t = -g_layer_y * h / g_menuscreen_h;
		if (right > g_menuscreen_w)
			r = (right - g_menuscreen_w) * w / g_menuscreen_w;
		if (bottom > g_menuscreen_h)
			b = (bottom - g_menuscreen_h) * h / g_menuscreen_h;
	}
	w -= l + r;
	h -= t + b;

	buf = vout_fbdev_resize(layer_fb, w, h, *bpp,
		l, r, t, b, 3, 1);

	vout_fbdev_clear(layer_fb);

	omap_enable_layer(1);

	return buf;
}

void *plat_gvideo_flip(void)
{
	return vout_fbdev_flip(layer_fb);
}

void plat_gvideo_close(void)
{
	omap_enable_layer(0);
}

void plat_video_menu_enter(int is_rom_loaded)
{
	g_menuscreen_ptr = vout_fbdev_resize(main_fb,
		g_menuscreen_w, g_menuscreen_h, 16, 0, 0, 0, 0, 3, 0);
	if (g_menuscreen_ptr == NULL)
		fprintf(stderr, "warning: vout_fbdev_resize failed\n");
	vout_fbdev_clear(main_fb);

	xenv_update(NULL, NULL, NULL, NULL);
}

void plat_video_menu_begin(void)
{
}

void plat_video_menu_end(void)
{
	g_menuscreen_ptr = vout_fbdev_flip(main_fb);
}

void plat_video_menu_leave(void)
{
	/* have to get rid of panning so that plugins that
	 * use fb0 and don't ever pan can work. */
	g_menuscreen_ptr = vout_fbdev_resize(main_fb,
		g_menuscreen_w, g_menuscreen_h, 16, 0, 0, 0, 0, 1, 0);
	if (g_menuscreen_ptr == NULL)
		fprintf(stderr, "warning: vout_fbdev_resize failed\n");
	vout_fbdev_clear(main_fb);
}

void plat_minimize(void)
{
	int ret;

	ret = vout_fbdev_save(layer_fb);
	if (ret != 0) {
		printf("minimize: layer/fb handling failed\n");
		return;
	}

	xenv_minimize();

	in_set_config_int(0, IN_CFG_BLOCKING, 0); /* flush event queue */
	omap_enable_layer(0); /* restore layer mem */
	vout_fbdev_restore(layer_fb);
}

void *plat_prepare_screenshot(int *w, int *h, int *bpp)
{
	return NULL;
}

void plat_omap_init(void)
{
	const char *main_fb_name, *layer_fb_name;
	int fd, ret, w, h;

	main_fb_name = getenv("FBDEV_MAIN");
	if (main_fb_name == NULL)
		main_fb_name = "/dev/fb0";

	layer_fb_name = getenv("FBDEV_LAYER");
	if (layer_fb_name == NULL)
		layer_fb_name = "/dev/fb1";

	// must set the layer up first to be able to use it
	fd = open(layer_fb_name, O_RDWR);
	if (fd == -1) {
		fprintf(stderr, "%s: ", layer_fb_name);
		perror("open");
		exit(1);
	}

	g_layer_x = 80, g_layer_y = 0;
	g_layer_w = 640, g_layer_h = 480;

	ret = omap_setup_layer_(fd, 0, g_layer_x, g_layer_y, g_layer_w, g_layer_h);
	close(fd);
	if (ret != 0) {
		fprintf(stderr, "failed to set up layer, exiting.\n");
		exit(1);
	}

	xenv_init(NULL, "PCSX-ReARMed");

	w = h = 0;
	main_fb = vout_fbdev_init(main_fb_name, &w, &h, 16, 2);
	if (main_fb == NULL) {
		fprintf(stderr, "couldn't init fb: %s\n", main_fb_name);
		exit(1);
	}

	g_menuscreen_w = g_menuscreen_pp = w;
	g_menuscreen_h = h;
	g_menuscreen_ptr = vout_fbdev_flip(main_fb);
	pl_rearmed_cbs.screen_w = w;
	pl_rearmed_cbs.screen_h = h;

	w = 640;
	h = 512;
	layer_fb = vout_fbdev_init(layer_fb_name, &w, &h, 16, 3);
	if (layer_fb == NULL) {
		fprintf(stderr, "couldn't init fb: %s\n", layer_fb_name);
		goto fail0;
	}

	return;

fail0:
	vout_fbdev_finish(main_fb);
	exit(1);
}

void plat_omap_finish(void)
{
	omap_enable_layer(0);
	vout_fbdev_finish(layer_fb);
	vout_fbdev_finish(main_fb);
	xenv_finish();
}

