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
#include <tslib.h>

#include "libpicofe/input.h"
#include "pl_gun_ts.h"
#include "in_tsbutton.h"

#define IN_TSBUTTON_PREFIX "tsbutton:"
#define IN_TSBUTTON_COUNT 4
static int tsbutton_down_id;
static int last_tsbutton_id;

#define TS_WIDTH 320
#define TS_HEIGHT 240

// HACK: stealing this from plugin_lib
extern void *tsdev;

static const char * const in_tsbutton_keys[IN_TSBUTTON_COUNT] = {
	"TS1", "TS2", "TS3", "TS4",
};

static void in_tsbutton_probe(const in_drv_t *drv)
{
	struct tsdev *dev = tsdev;
	if (dev == NULL) {
		fprintf(stderr, "in_tsbutton_probe: missing tsdev\n");
		return;
	}

	in_register(IN_TSBUTTON_PREFIX "touchscreen as buttons",
		pl_gun_ts_get_fd(dev), NULL, IN_TSBUTTON_COUNT, in_tsbutton_keys, 0);
}

static const char * const *
in_tsbutton_get_key_names(const in_drv_t *drv, int *count)
{
	*count = IN_TSBUTTON_COUNT;
	return in_tsbutton_keys;
}

static int update_button(void)
{
	struct tsdev *dev = tsdev;
	int sx = 0, sy = 0, sp = 0;

	if (dev == NULL)
		return -1;

	if (pl_gun_ts_update_raw(dev, &sx, &sy, &sp)) {
		if (sp == 0)
			tsbutton_down_id = -1;
		else {
			// 0 1
			// 2 3
			tsbutton_down_id = 0;
			if (sx > TS_WIDTH / 2)
				tsbutton_down_id++;
			if (sy > TS_HEIGHT / 2)
				tsbutton_down_id += 2;
		}
	}

	return 0;
}

static int in_tsbutton_update(void *drv_data, const int *binds, int *result)
{
	int ret, t;
	
	ret = update_button();
	if (ret != 0)
		return ret;

	if (tsbutton_down_id >= 0)
		for (t = 0; t < IN_BINDTYPE_COUNT; t++)
			result[t] |= binds[IN_BIND_OFFS(tsbutton_down_id, t)];

	return 0;
}

static int in_tsbutton_update_keycode(void *data, int *is_down)
{
	int ret, ret_kc = -1, ret_down = 0;

	ret = update_button();
	if (ret != 0)
		return ret;

	if (tsbutton_down_id == last_tsbutton_id)
		return -1;

	if (tsbutton_down_id >= 0) {
		if (last_tsbutton_id >= 0) {
			ret_kc = last_tsbutton_id;
			last_tsbutton_id = -1;
		}
		else {
			ret_down = 1;
			ret_kc = tsbutton_down_id;
			last_tsbutton_id = tsbutton_down_id;
		}
	}
	else {
		ret_kc = last_tsbutton_id;
		last_tsbutton_id = -1;
	}

	if (is_down != NULL)
		*is_down = ret_down;

	return ret_kc;
}

static const in_drv_t in_tsbutton_drv = {
	.prefix         = IN_TSBUTTON_PREFIX,
	.probe          = in_tsbutton_probe,
	.get_key_names  = in_tsbutton_get_key_names,
	.update         = in_tsbutton_update,
	.update_keycode = in_tsbutton_update_keycode,
};

void in_tsbutton_init(void)
{
	tsbutton_down_id = last_tsbutton_id = -1;
	in_register_driver(&in_tsbutton_drv, NULL, NULL);
}

