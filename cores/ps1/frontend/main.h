/*  Pcsx - Pc Psx Emulator
 *  Copyright (C) 1999-2002  Pcsx Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02111-1307 USA
 */

#ifndef __FRONTEND_MAIN_H__
#define __FRONTEND_MAIN_H__

#include <stdlib.h>
#include "config.h"

#define PCSX_DOT_DIR "/.pcsx/"
#define DEFAULT_MEM_CARD_1 PCSX_DOT_DIR "memcards/card1.mcd"
#define DEFAULT_MEM_CARD_2 PCSX_DOT_DIR "memcards/card2.mcd"
#define MEMCARD_DIR        PCSX_DOT_DIR "memcards/"
#define STATES_DIR         PCSX_DOT_DIR "sstates/"
#define CHEATS_DIR         PCSX_DOT_DIR "cheats/"
#define PATCHES_DIR        PCSX_DOT_DIR "patches/"
#define CFG_DIR            PCSX_DOT_DIR "cfg/"
#ifndef PANDORA
#define BIOS_DIR           PCSX_DOT_DIR "bios/"
#define SCREENSHOTS_DIR    PCSX_DOT_DIR "screenshots/"
#else
#define BIOS_DIR           "/bios/"
#define SCREENSHOTS_DIR    "/screenshots/"
#endif

extern char cfgfile_basename[MAXPATHLEN];

extern int state_slot;

/* emu_core_preinit - must be the very first call
 * emu_core_init - to be called after platform-specific setup */
int emu_core_preinit(void);
int emu_core_init(void);

void emu_core_ask_exit(void);

void emu_set_default_config(void);
void emu_on_new_cd(int show_hud_msg);

void emu_make_path(char *buf, size_t size, const char *dir, const char *fname);
void emu_make_data_path(char *buff, const char *end, int size);

int get_state_filename(char *buf, int size, int i);
int emu_check_state(int slot);
int emu_save_state(int slot);
int emu_load_state(int slot);

void set_cd_image(const char *fname);

extern unsigned long gpuDisp;
extern int ready_to_go, g_emu_want_quit, g_emu_resetting;

extern char hud_msg[64];
extern int hud_new_msg;

enum sched_action {
	SACTION_NONE,
	SACTION_ENTER_MENU,
	SACTION_LOAD_STATE,
	SACTION_SAVE_STATE,
	SACTION_NEXT_SSLOT,
	SACTION_PREV_SSLOT,
	SACTION_TOGGLE_FSKIP,
	SACTION_SWITCH_DISPMODE,
	SACTION_FAST_FORWARD,
	SACTION_SCREENSHOT,
	SACTION_VOLUME_UP,	// 10
	SACTION_VOLUME_DOWN,
	SACTION_MINIMIZE,
	SACTION_TOGGLE_FPS,
	SACTION_TOGGLE_FULLSCREEN,
	SACTION_GUN_TRIGGER = 16,
	SACTION_GUN_A,
	SACTION_GUN_B,
	SACTION_GUN_TRIGGER2,
	SACTION_ANALOG_TOGGLE,
};

#define SACTION_GUN_MASK (0x0f << SACTION_GUN_TRIGGER)

#endif /* __FRONTEND_MAIN_H__ */
