#include <stdlib.h>
#include <SDL_keysym.h>

#include "../libpicofe/input.h"
#include "../libpicofe/in_sdl.h"
#include "../common/input_pico.h"
#include "../common/plat_sdl.h"

const struct in_default_bind in_sdl_defbinds[] = {
	{ SDLK_UP,	IN_BINDTYPE_PLAYER12, GBTN_UP },
	{ SDLK_DOWN,	IN_BINDTYPE_PLAYER12, GBTN_DOWN },
	{ SDLK_LEFT,	IN_BINDTYPE_PLAYER12, GBTN_LEFT },
	{ SDLK_RIGHT,	IN_BINDTYPE_PLAYER12, GBTN_RIGHT },
	{ SDLK_LSHIFT,	IN_BINDTYPE_PLAYER12, GBTN_A },
	{ SDLK_LALT,	IN_BINDTYPE_PLAYER12, GBTN_B },
	{ SDLK_LCTRL,	IN_BINDTYPE_PLAYER12, GBTN_C },
	{ SDLK_RETURN,	IN_BINDTYPE_PLAYER12, GBTN_START },
	{ SDLK_ESCAPE,	IN_BINDTYPE_EMU, PEVB_MENU },
	{ SDLK_TAB,		IN_BINDTYPE_EMU, PEVB_PICO_PPREV },
	{ SDLK_BACKSPACE,	IN_BINDTYPE_EMU, PEVB_PICO_PNEXT },
	{ SDLK_BACKSPACE,	IN_BINDTYPE_EMU, PEVB_STATE_SAVE },
	{ SDLK_TAB,		IN_BINDTYPE_EMU, PEVB_STATE_LOAD },
	{ SDLK_SPACE,	IN_BINDTYPE_EMU, PEVB_FF },
	{ 0, 0, 0 }
};

const struct menu_keymap in_sdl_key_map[] = {
	{ SDLK_UP,	PBTN_UP },
	{ SDLK_DOWN,	PBTN_DOWN },
	{ SDLK_LEFT,	PBTN_LEFT },
	{ SDLK_RIGHT,	PBTN_RIGHT },
#if defined(__MIYOO__)
	{ SDLK_LALT,	PBTN_MOK },
	{ SDLK_LCTRL,	PBTN_MBACK },
#else
	{ SDLK_LCTRL,	PBTN_MOK },
	{ SDLK_LALT,	PBTN_MBACK },
#endif
	{ SDLK_SPACE,	PBTN_MA2 },
	{ SDLK_LSHIFT,	PBTN_MA3 },
	{ SDLK_TAB,	PBTN_L },
	{ SDLK_BACKSPACE,	PBTN_R },
};
const int in_sdl_key_map_sz = sizeof(in_sdl_key_map) / sizeof(in_sdl_key_map[0]);

const struct menu_keymap in_sdl_joy_map[] = {
	{ SDLK_UP,	PBTN_UP },
	{ SDLK_DOWN,	PBTN_DOWN },
	{ SDLK_LEFT,	PBTN_LEFT },
	{ SDLK_RIGHT,	PBTN_RIGHT },
	/* joystick */
	{ SDLK_WORLD_0,	PBTN_MOK },
	{ SDLK_WORLD_1,	PBTN_MBACK },
	{ SDLK_WORLD_2,	PBTN_MA2 },
	{ SDLK_WORLD_3,	PBTN_MA3 },
};
const int in_sdl_joy_map_sz = sizeof(in_sdl_joy_map) / sizeof(in_sdl_joy_map[0]);

const char * const _in_sdl_key_names[SDLK_LAST] = {
	[SDLK_UP] = "UP",
	[SDLK_DOWN] = "DOWN",
	[SDLK_LEFT] = "LEFT",
	[SDLK_RIGHT] = "RIGHT",
#if defined(__MIYOO__)
	[SDLK_LALT] = "A",
	[SDLK_LCTRL] = "B",
#else
	[SDLK_LCTRL] = "A",
	[SDLK_LALT] = "B",
#endif
#if defined(__GCW0__) || defined(__MIYOO__)
	[SDLK_LSHIFT] = "X",
	[SDLK_SPACE] = "Y",
#else
	[SDLK_LSHIFT] = "Y",
	[SDLK_SPACE] = "X",
#endif
	[SDLK_RETURN] = "START",
	[SDLK_ESCAPE] = "SELECT",

#if defined(__RG350__) || defined(__OPENDINGUX__) || defined(__RG99__)
	[SDLK_HOME] = "POWER",

	[SDLK_TAB] = "L1",
	[SDLK_BACKSPACE] = "R1",
	[SDLK_PAGEUP] = "L2",
	[SDLK_PAGEDOWN] = "R2",
	[SDLK_KP_DIVIDE] = "L3",
	[SDLK_KP_PERIOD] = "R3",
#elif defined(__MIYOO__)
	[SDLK_TAB] = "L1",
	[SDLK_BACKSPACE] = "R1",
	[SDLK_RALT] = "L2",
	[SDLK_RSHIFT] = "R2",
	[SDLK_RCTRL] = "R",
#else
	[SDLK_TAB] = "L",
	[SDLK_BACKSPACE] = "R",
	[SDLK_POWER] = "POWER",
	[SDLK_PAUSE] = "LOCK",
#endif
};
const char * const (*in_sdl_key_names)[SDLK_LAST] = &_in_sdl_key_names;
