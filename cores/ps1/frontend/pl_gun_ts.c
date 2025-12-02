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
#include <stdlib.h>
#include <dlfcn.h>
#include <tslib.h>
#include "plugin_lib.h"
#include "pl_gun_ts.h"
#include "menu.h"

#ifdef MAEMO
#define N900_TSMAX_X 4096
#define N900_TSOFFSET_X 0
#define N900_TSMAX_Y 4096
#define N900_TSOFFSET_Y 0
#endif

static int gun_x, gun_y, gun_in;
static int ts_multiplier_x, ts_multiplier_y, ts_offs_x, ts_offs_y;
static int (*pts_read)(struct tsdev *dev, struct ts_sample *sample, int nr);
static int (*pts_fd)(struct tsdev *dev);

#define limit(v, min, max) \
	if (v < min) v = min; \
	else if (v > max) v = max

int pl_gun_ts_update_raw(struct tsdev *ts, int *x, int *y, int *p)
{
	struct ts_sample sample;
	int sx = 0, sy = 0, sp = 0, updated = 0;

	if (ts != NULL) {
		while (pts_read(ts, &sample, 1) > 0) {
			sx = sample.x;
#ifdef MAEMO
			sy = N900_TSMAX_Y - sample.y;
#else
			sy = sample.y;
#endif
			sp = sample.pressure;
			updated = 1;
		}

		if (updated) {
			gun_x = (sx - ts_offs_x) * ts_multiplier_x >> 10;
			gun_y = (sy - ts_offs_y) * ts_multiplier_y >> 10;
			limit(gun_x, 0, 1023);
			limit(gun_y, 0, 1023);
			if (sp)
				gun_in |= 1;
			else
				gun_in &= ~1;
		}
	}

	if (updated) {
		if (x) *x = sx;
		if (y) *y = sy;
		if (p) *p = sp;
		return 1;
	}

	return 0;
}

/* returns x, y in range 0..1023 (normalized to visible layer) */
void pl_gun_ts_update(struct tsdev *ts, int *x, int *y, int *in)
{
	pl_gun_ts_update_raw(ts, NULL, NULL, NULL);

	*x = gun_x;
	*y = gun_y;
	*in = gun_in;
}

void pl_set_gun_rect(int x, int y, int w, int h)
{
	ts_offs_x = x;
	ts_offs_y = y;
	ts_multiplier_x = (1<<20) / w;
	ts_multiplier_y = (1<<20) / h;
}

int pl_gun_ts_get_fd(struct tsdev *ts)
{
	if (ts != NULL && pts_fd != NULL)
		return pts_fd(ts);

	return -1;
}

struct tsdev *pl_gun_ts_init(void)
{
	struct tsdev *(*pts_open)(const char *dev_name, int nonblock) = NULL;
	int (*pts_config)(struct tsdev *) = NULL;
	int (*pts_close)(struct tsdev *) = NULL;
	const char *tsdevname;
	struct tsdev *ts;
	void *ltsh;

#ifdef MAEMO
	tsdevname = "/dev/input/ts";
#else
	tsdevname = getenv("TSLIB_TSDEVICE");
	if (tsdevname == NULL)
		tsdevname = "/dev/input/touchscreen0";
#endif

	// avoid hard dep on tslib
	ltsh = dlopen("/usr/lib/libts-1.0.so.0", RTLD_NOW|RTLD_GLOBAL);
	if (ltsh == NULL)
		ltsh = dlopen("/usr/lib/libts-0.0.so.0", RTLD_NOW|RTLD_GLOBAL);
	if (ltsh == NULL)
		ltsh = dlopen("/lib/libts-0.0.so.0", RTLD_NOW|RTLD_GLOBAL);
	if (ltsh == NULL) {
		fprintf(stderr, "%s\n", dlerror());
		goto fail;
	}

	pts_open = dlsym(ltsh, "ts_open");
	pts_config = dlsym(ltsh, "ts_config");
	pts_read = dlsym(ltsh, "ts_read");
	pts_fd = dlsym(ltsh, "ts_fd");
	pts_close = dlsym(ltsh, "ts_close");
	if (pts_open == NULL || pts_config == NULL || pts_read == NULL
	    || pts_fd == NULL || pts_close == NULL) {
		fprintf(stderr, "%s\n", dlerror());
		goto fail_dlsym;
	}

	ts = pts_open(tsdevname, 1);
	if (ts == NULL){
		printf("Failed pts_open, check permission on %s\n", tsdevname);
		goto fail_open;
	}
	if (pts_config(ts) != 0){
		printf("Failed pts_config\n");
		goto fail_config;
	}

	// FIXME: we should be able to get this somewhere
	// the problem is this doesn't always match resolution due to different display modes
#ifdef MAEMO
	pl_set_gun_rect(N900_TSOFFSET_X, N900_TSOFFSET_Y, N900_TSMAX_X, N900_TSMAX_Y);
#else
#ifdef __ARM_ARCH_7A__
	pl_set_gun_rect(0, 0, 800, 480);
#else
	pl_set_gun_rect(0, 0, 320, 240);
#endif
#endif
	printf("Touchscreen configured, device=%s\n", tsdevname);
	return ts;

fail_config:
	pts_close(ts);
fail_open:
fail_dlsym:
	dlclose(ltsh);
	ltsh = NULL;
fail:
	fprintf(stderr, "Could not open touchscreen\n");
	return NULL;
}

