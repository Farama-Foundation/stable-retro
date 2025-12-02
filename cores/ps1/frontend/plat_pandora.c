/*
 * (C) notaz, 2011
 *
 * This work is licensed under the terms of the GNU GPLv2 or later.
 * See the COPYING file in the top-level directory.
 */

#include <stdio.h>
#include <linux/input.h>

#include "libpicofe/input.h"
#include "libpicofe/linux/in_evdev.h"
#include "libpicofe/plat.h"
#include "plugin_lib.h"
#include "plat_omap.h"
#include "main.h"
#include "menu.h"

static const struct in_default_bind in_evdev_defbinds[] = {
	{ KEY_UP,	IN_BINDTYPE_PLAYER12, DKEY_UP },
	{ KEY_DOWN,	IN_BINDTYPE_PLAYER12, DKEY_DOWN },
	{ KEY_LEFT,	IN_BINDTYPE_PLAYER12, DKEY_LEFT },
	{ KEY_RIGHT,	IN_BINDTYPE_PLAYER12, DKEY_RIGHT },
	{ KEY_PAGEUP,	IN_BINDTYPE_PLAYER12, DKEY_TRIANGLE },
	{ KEY_PAGEDOWN,	IN_BINDTYPE_PLAYER12, DKEY_CROSS },
	{ KEY_END,	IN_BINDTYPE_PLAYER12, DKEY_CIRCLE },
	{ KEY_HOME,	IN_BINDTYPE_PLAYER12, DKEY_SQUARE },
	{ KEY_LEFTALT,	IN_BINDTYPE_PLAYER12, DKEY_START },
	{ KEY_LEFTCTRL,	IN_BINDTYPE_PLAYER12, DKEY_SELECT },
	{ KEY_RIGHTSHIFT,IN_BINDTYPE_PLAYER12, DKEY_L1 },
	{ KEY_RIGHTCTRL, IN_BINDTYPE_PLAYER12, DKEY_R1 },
	{ KEY_Q,	IN_BINDTYPE_PLAYER12, DKEY_L2 },
	{ KEY_P,	IN_BINDTYPE_PLAYER12, DKEY_R2 },
	{ KEY_MENU,	IN_BINDTYPE_EMU, SACTION_MINIMIZE },
	{ KEY_SPACE,    IN_BINDTYPE_EMU, SACTION_ENTER_MENU },
	{ KEY_1,        IN_BINDTYPE_EMU, SACTION_SAVE_STATE },
	{ KEY_2,        IN_BINDTYPE_EMU, SACTION_LOAD_STATE },
	{ KEY_3,        IN_BINDTYPE_EMU, SACTION_PREV_SSLOT },
	{ KEY_4,        IN_BINDTYPE_EMU, SACTION_NEXT_SSLOT },
	{ KEY_5,        IN_BINDTYPE_EMU, SACTION_TOGGLE_FSKIP },
	{ KEY_6,        IN_BINDTYPE_EMU, SACTION_SCREENSHOT },
	{ KEY_7,        IN_BINDTYPE_EMU, SACTION_TOGGLE_FPS },
	{ KEY_8,        IN_BINDTYPE_EMU, SACTION_SWITCH_DISPMODE },
	{ KEY_BACKSPACE,IN_BINDTYPE_EMU, SACTION_FAST_FORWARD },
	{ 0, 0, 0 }
};

static const struct menu_keymap key_pbtn_map[] =
{
	{ KEY_UP,	PBTN_UP },
	{ KEY_DOWN,	PBTN_DOWN },
	{ KEY_LEFT,	PBTN_LEFT },
	{ KEY_RIGHT,	PBTN_RIGHT },
	/* Pandora */
	{ KEY_END,	PBTN_MOK },
	{ KEY_PAGEDOWN,	PBTN_MBACK },
	{ KEY_HOME,	PBTN_MA2 },
	{ KEY_PAGEUP,	PBTN_MA3 },
	{ KEY_LEFTCTRL,   PBTN_MENU },
	{ KEY_RIGHTSHIFT, PBTN_L },
	{ KEY_RIGHTCTRL,  PBTN_R },
	/* "normal" keyboards */
	{ KEY_ENTER,	PBTN_MOK },
	{ KEY_ESC,	PBTN_MBACK },
	{ KEY_SEMICOLON,  PBTN_MA2 },
	{ KEY_APOSTROPHE, PBTN_MA3 },
	{ KEY_BACKSLASH,  PBTN_MENU },
	{ KEY_LEFTBRACE,  PBTN_L },
	{ KEY_RIGHTBRACE, PBTN_R },
};

static const struct in_pdata pandora_evdev_pdata = {
	.defbinds = in_evdev_defbinds,
	.key_map = key_pbtn_map,
	.kmap_size = sizeof(key_pbtn_map) / sizeof(key_pbtn_map[0]),
};

int plat_init(void)
{
	plat_omap_init();
	plat_target_init();

	in_evdev_init(&pandora_evdev_pdata);
	in_probe();
	plat_target_setup_input();

	in_adev[0] = in_name_to_id("evdev:nub0");
	in_adev[1] = in_name_to_id("evdev:nub1");
	in_adev_is_nublike[0] = in_adev_is_nublike[1] = 1;

	return 0;
}

void plat_finish(void)
{
	plat_omap_finish();
	plat_target_finish();
}

void plat_gvideo_open(int is_pal)
{
	plat_target_lcdrate_set(is_pal);
	plat_target_hwfilter_set(plat_target.hwfilter);
	plat_target_gamma_set(g_gamma, 0);

	plat_omap_gvideo_open();
}

void plat_trigger_vibrate(int pad, int low, int high)
{
}
