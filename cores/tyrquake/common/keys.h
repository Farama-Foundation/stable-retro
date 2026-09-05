/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#ifndef KEYS_H
#define KEYS_H

#include <streams/file_stream.h>

#include "qtypes.h"

/*
 * These are the key numbers that should be passed to Key_Event().
 * The keyboard syms have been chosed to map to ASCII.
 * Keysym names map closely to those defined in SDL
 * Numbering scheme taken from QuakeForge
 */
typedef enum {
    K_UNKNOWN = 0,
    K_BACKSPACE = 8,
    K_TAB = 9,
    K_CLEAR = 12,
    K_RETURN = 13,
    K_ENTER = 13,
    K_PAUSE = 19,
    K_ESCAPE = 27,
    K_SPACE = 32,
    K_EXCLAIM = 33,
    K_QUOTEDBL = 34,
    K_HASH = 35,
    K_DOLLAR = 36,
    K_PERCENT = 37,
    K_AMPERSAND = 38,
    K_QUOTE = 39,
    K_LEFTPAREN = 40,
    K_RIGHTPAREN = 41,
    K_ASTERISK = 42,
    K_PLUS = 43,
    K_COMMA = 44,
    K_MINUS = 45,
    K_PERIOD = 46,
    K_SLASH = 47,
    K_0 = 48,
    K_1 = 49,
    K_2 = 50,
    K_3 = 51,
    K_4 = 52,
    K_5 = 53,
    K_6 = 54,
    K_7 = 55,
    K_8 = 56,
    K_9 = 57,
    K_COLON = 58,
    K_SEMICOLON = 59,
    K_LESS = 60,
    K_EQUALS = 61,
    K_GREATER = 62,
    K_QUESTION = 63,
    K_AT = 64,
    /*
     * Skip uppercase, alpha keys passed as lowercase.
     */
    K_LEFTBRACKET = 91,
    K_BACKSLASH = 92,
    K_RIGHTBRACKET = 93,
    K_CARET = 94,
    K_UNDERSCORE = 95,
    K_BACKQUOTE = 96,
    K_a = 97,
    K_b = 98,
    K_c = 99,
    K_d = 100,
    K_e = 101,
    K_f = 102,
    K_g = 103,
    K_h = 104,
    K_i = 105,
    K_j = 106,
    K_k = 107,
    K_l = 108,
    K_m = 109,
    K_n = 110,
    K_o = 111,
    K_p = 112,
    K_q = 113,
    K_r = 114,
    K_s = 115,
    K_t = 116,
    K_u = 117,
    K_v = 118,
    K_w = 119,
    K_x = 120,
    K_y = 121,
    K_z = 122,
    K_BRACELEFT = 123,
    K_BAR = 124,
    K_BRACERIGHT = 125,
    K_ASCIITILDE = 126,
    K_DEL = 127,
    /* End of ASCII mapped keysyms */

    /* Numeric keypad */
    K_KP0 = 256,
    K_KP1 = 257,
    K_KP2 = 258,
    K_KP3 = 259,
    K_KP4 = 260,
    K_KP5 = 261,
    K_KP6 = 262,
    K_KP7 = 263,
    K_KP8 = 264,
    K_KP9 = 265,
    K_KP_PERIOD = 266,
    K_KP_DIVIDE = 267,
    K_KP_MULTIPLY = 268,
    K_KP_MINUS = 269,
    K_KP_PLUS = 270,
    K_KP_ENTER = 271,
    K_KP_EQUALS = 272,

    /* Arrows + Home/End pad */
    K_UPARROW = 273,
    K_DOWNARROW = 274,
    /* both these keys were inverted in-game, maybe it only affects a few
     * input drivers? */
    K_RIGHTARROW = 275,
    K_LEFTARROW = 276,
    K_INS = 277,
    K_HOME = 278,
    K_END = 279,
    K_PGUP = 280,
    K_PGDN = 281,

    /* Function Keys */
    K_F1 = 282,
    K_F2 = 283,
    K_F3 = 284,
    K_F4 = 285,
    K_F5 = 286,
    K_F6 = 287,
    K_F7 = 288,
    K_F8 = 289,
    K_F9 = 290,
    K_F10 = 291,
    K_F11 = 292,
    K_F12 = 293,
    K_F13 = 294,
    K_F14 = 295,
    K_F15 = 296,

    /* Modifier Keys */
    K_NUMLOCK = 300,
    K_CAPSLOCK = 301,
    K_SCROLLOCK = 302,
    K_RSHIFT = 303,
    K_LSHIFT = 304,
    K_RCTRL = 305,
    K_LCTRL = 306,
    K_RALT = 307,
    K_LALT = 308,
    K_RMETA = 309,
    K_LMETA = 310,
    K_LSUPER = 311,	/* Left "Windows" key */
    K_RSUPER = 312,	/* Right "Windows" key */
    K_MODE = 313,	/* "Alt Gr" key */
    K_COMPOSE = 314,	/* Multi-key compose key */

    /* Misc. function keys */
    K_HELP = 315,
    K_PRINT = 316,
    K_SYSREQ = 317,
    K_BREAK = 318,
    K_MENU = 319,
    K_POWER = 320,
    K_EURO = 321,
    K_UNDO = 322,

    /* Japanese keys */

    /* Some multi-media/browser keys */

    /* Add any other keys here */

    /*
     * mouse buttons generate virtual keys
     * (mouse buttons 4-8 added below)
     */
    K_MOUSE1,
    K_MOUSE2,
    K_MOUSE3,
    K_MOUSE4,
    K_MOUSE5,
#define K_MWHEELUP K_MOUSE4
#define K_MWHEELDOWN K_MOUSE5
    K_MOUSE6,
    K_MOUSE7,
    K_MOUSE8,

    /*
     * joystick buttons
     */
    K_JOY1,
    K_JOY2,
    K_JOY3,
    K_JOY4,
    K_JOY_B,
    K_JOY_Y,
    K_JOY_SELECT,
    K_JOY_START,
    K_JOY_UP,
    K_JOY_DOWN,
    K_JOY_LEFT,
    K_JOY_RIGHT,
    K_JOY_A,
    K_JOY_X,
    K_JOY_L,
    K_JOY_R,
    K_JOY_L2,
    K_JOY_R2,
    K_JOY_L3,
    K_JOY_R3,

    /*
     * aux keys are for multi-buttoned joysticks to generate so they can use
     * the normal binding process
     */
    K_AUX1,
    K_AUX2,
    K_AUX3,
    K_AUX4,
    K_AUX5,
    K_AUX6,
    K_AUX7,
    K_AUX8,
    K_AUX9,
    K_AUX10,
    K_AUX11,
    K_AUX12,
    K_AUX13,
    K_AUX14,
    K_AUX15,
    K_AUX16,
    K_AUX17,
    K_AUX18,
    K_AUX19,
    K_AUX20,
    K_AUX21,
    K_AUX22,
    K_AUX23,
    K_AUX24,
    K_AUX25,
    K_AUX26,
    K_AUX27,
    K_AUX28,
    K_AUX29,
    K_AUX30,
    K_AUX31,
    K_AUX32,
    K_LAST,

    ENSURE_INT_KNUM = 0x70000000
} knum_t;

typedef enum {
    key_game,
    key_console,
    key_message,
    key_menu,
    key_none,
    ENSURE_INT_KEYDEST = 0x70000000
} keydest_t;

extern keydest_t key_dest;
extern const char *keybindings[K_LAST];
extern int key_repeats[K_LAST];
extern int key_lastpress;

extern char chat_buffer[];
extern int chat_bufferlen;
extern qboolean chat_team;

void Key_Event(knum_t key, qboolean down);
void Key_Init(void);
void Key_WriteBindings(RFILE *f);
void Key_SetBinding(knum_t keynum, const char *binding);
void Key_ClearStates(void);
void Key_ClearTyping(void);

const char *Key_KeynumToString(int keynum);

#define MAXCMDLINE 256
extern char key_lines[32][MAXCMDLINE];
extern int edit_line;
extern int key_linepos;

#endif /* KEYS_H */
