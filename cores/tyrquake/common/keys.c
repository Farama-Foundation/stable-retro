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

#include "compat/strl.h"
#include "client.h"
#include "cmd.h"
#include "common.h"
#include "console.h"
#include "keys.h"
#include "menu.h"
#include "quakedef.h"
#include "screen.h"
#include "shell.h"
#include "sys.h"
#include "zone.h"

#if defined(_WIN32) && !defined(_XBOX)
#include <windows.h>
#include <winuser.h>
#endif

#include <streams/file_stream.h>

/* forward declarations */
int rfprintf(RFILE * stream, const char * format, ...);

/*

key up events are sent even if in console mode

*/

#define MAXCMDLINE 256

char key_lines[32][MAXCMDLINE];
int key_linepos;
int key_lastpress;

static qboolean lshift_down = false;
static qboolean rshift_down = false;

int edit_line = 0;
int history_line = 0;

keydest_t key_dest;

const char *keybindings[K_LAST];
qboolean consolekeys[K_LAST];	/* if true, can't be rebound while in console */
qboolean menubound[K_LAST];	/* if true, can't be rebound while in menu */
int keyshift[K_LAST];		/* key to map to if shift held down in console */
int key_repeats[K_LAST];		/* if > 1, it is autorepeating */
qboolean keydown[K_LAST];

typedef struct {
    const char *name;
    int keynum;
} keyname_t;

keyname_t keynames[] = {
    {"TAB", K_TAB},
    {"ENTER", K_ENTER},
    {"ESCAPE", K_ESCAPE},
    {"SPACE", K_SPACE},
    {"BACKSPACE", K_BACKSPACE},
    {"UPARROW", K_UPARROW},
    {"DOWNARROW", K_DOWNARROW},
    {"LEFTARROW", K_LEFTARROW},
    {"RIGHTARROW", K_RIGHTARROW},

    {"LALT", K_LALT},
    {"RALT", K_RALT},
    {"LCTRL", K_LCTRL},
    {"RCTRL", K_RCTRL},
    {"LSHIFT", K_LSHIFT},
    {"RSHIFT", K_RSHIFT},

    {"F1", K_F1},
    {"F2", K_F2},
    {"F3", K_F3},
    {"F4", K_F4},
    {"F5", K_F5},
    {"F6", K_F6},
    {"F7", K_F7},
    {"F8", K_F8},
    {"F9", K_F9},
    {"F10", K_F10},
    {"F11", K_F11},
    {"F12", K_F12},

    {"INS", K_INS},
    {"DEL", K_DEL},
    {"PGDN", K_PGDN},
    {"PGUP", K_PGUP},
    {"HOME", K_HOME},
    {"END", K_END},

    {"MOUSE1", K_MOUSE1},
    {"MOUSE2", K_MOUSE2},
    {"MOUSE3", K_MOUSE3},
    {"MOUSE4", K_MOUSE4},
    {"MOUSE5", K_MOUSE5},
    {"MOUSE6", K_MOUSE6},
    {"MOUSE7", K_MOUSE7},
    {"MOUSE8", K_MOUSE8},

    {"JOY1", K_JOY1},
    {"JOY2", K_JOY2},
    {"JOY3", K_JOY3},
    {"JOY4", K_JOY4},
    /* Follows retroarch RETRO_DEVICE order */
    {"JOY_B", K_JOY_B},
    {"JOY_Y", K_JOY_Y},
    {"JOY_SELECT", K_JOY_SELECT},
    {"JOY_START", K_JOY_START},
    {"JOY_UP", K_JOY_UP},
    {"JOY_DOWN", K_JOY_DOWN},
    {"JOY_LEFT", K_JOY_LEFT},
    {"JOY_RIGHT", K_JOY_RIGHT},
    {"JOY_A", K_JOY_A},
    {"JOY_X", K_JOY_X},
    {"JOY_L", K_JOY_L},
    {"JOY_R", K_JOY_R},
    {"JOY_L2", K_JOY_L2},
    {"JOY_R2", K_JOY_R2},
    {"JOY_L3", K_JOY_L3},
    {"JOY_R3", K_JOY_R3},

    {"AUX1", K_AUX1},
    {"AUX2", K_AUX2},
    {"AUX3", K_AUX3},
    {"AUX4", K_AUX4},
    {"AUX5", K_AUX5},
    {"AUX6", K_AUX6},
    {"AUX7", K_AUX7},
    {"AUX8", K_AUX8},
    {"AUX9", K_AUX9},
    {"AUX10", K_AUX10},
    {"AUX11", K_AUX11},
    {"AUX12", K_AUX12},
    {"AUX13", K_AUX13},
    {"AUX14", K_AUX14},
    {"AUX15", K_AUX15},
    {"AUX16", K_AUX16},
    {"AUX17", K_AUX17},
    {"AUX18", K_AUX18},
    {"AUX19", K_AUX19},
    {"AUX20", K_AUX20},
    {"AUX21", K_AUX21},
    {"AUX22", K_AUX22},
    {"AUX23", K_AUX23},
    {"AUX24", K_AUX24},
    {"AUX25", K_AUX25},
    {"AUX26", K_AUX26},
    {"AUX27", K_AUX27},
    {"AUX28", K_AUX28},
    {"AUX29", K_AUX29},
    {"AUX30", K_AUX30},
    {"AUX31", K_AUX31},
    {"AUX32", K_AUX32},

    {"PAUSE", K_PAUSE},

    {"MWHEELUP", K_MWHEELUP},
    {"MWHEELDOWN", K_MWHEELDOWN},

    {"SEMICOLON", ';'},		/* because a raw semicolon seperates commands */

    {NULL, 0}
};

/*
==============================================================================

			LINE TYPING INTO THE CONSOLE

==============================================================================
*/

/*
 * Given a command buffer, return a pointer to the start of the current
 * command string. Only simple for now (i.e. search backwards for a command
 * delimiter), but proper parsing of quotation, etc needed later...
 */
static char *GetCommandPos(char *buf)
{
    char *pos = strrchr(buf, ';');
    if (pos) {
	pos++;
	while (*pos == ' ')
	    pos++;
    } else
	pos = buf;

    if (*pos == '\\' || *pos == '/')
	pos++;

    return pos;
}

static qboolean CheckForCommand(void)
{
    int i;
    char cmd[128];
    char *s = key_lines[edit_line] + 1;	/* skip the ] */

    for (i = 0; i < sizeof(cmd) - 1; i++)
	if (s[i] <= ' ')
	    break;
	else
	    cmd[i] = s[i];
    cmd[i] = 0;

    return Cmd_Exists(cmd) || Cvar_FindVar(cmd) || Cmd_Alias_Exists(cmd);
}


static void
CompleteCommand(void)
{
    const char *cmd, *completion;
    char *s, *newcmd;
    int len;

    s = GetCommandPos(key_lines[edit_line] + 1);
    cmd = Cmd_CommandComplete(s);
    if (cmd) {
	size_t remaining;
	key_linepos = s - key_lines[edit_line];
	if (s == key_lines[edit_line] + 1) {
	    *s++ = '/';
	    key_linepos++;
	}
	remaining = MAXCMDLINE - (s - key_lines[edit_line]);
	strlcpy(s, cmd, remaining);
	key_linepos += strlen(cmd);
	if (key_linepos >= MAXCMDLINE)
	    key_linepos = MAXCMDLINE - 1;
	key_lines[edit_line][key_linepos] = 0;
	Z_Free((void *)cmd);
    } else {
	/* Try argument completion? */
	cmd = strchr(s, ' ');
	if (cmd) {
	    len = cmd - s;
	    newcmd = (char*)Z_Malloc(len + 1);
	    strlcpy(newcmd, s, len + 1);

	    completion = NULL;
	    if (Cmd_Exists(newcmd)) {
		s += len;
		while (*s == ' ')
		    s++;
		completion = Cmd_ArgComplete(newcmd, s);
	    } else if (Cvar_FindVar(newcmd)) {
		s += len;
		while (*s == ' ')
		    s++;
		completion = Cvar_ArgComplete(newcmd, s);
	    }
	    if (completion) {
		size_t remaining = MAXCMDLINE - (s - key_lines[edit_line]);
		key_linepos = s - key_lines[edit_line];
		strlcpy(s, completion, remaining);
		key_linepos += strlen(completion);
		if (key_linepos >= MAXCMDLINE)
		    key_linepos = MAXCMDLINE - 1;
		Z_Free((void *)completion);
	    }
	    Z_Free(newcmd);
	}
    }
}

static void
ShowCompletions(void)
{
    const char *s;
    struct stree_root *root;
    unsigned int len;

    s = GetCommandPos(key_lines[edit_line] + 1);

    root = Cmd_CommandCompletions(s);
    if (root && root->entries) {
	Con_Printf("%s\n", key_lines[edit_line]);
	/* Con_Printf("%u possible completions:\n", root->entries); */
	Con_ShowTree(root);
	Z_Free(root);
    } else {
	char *cmd = (char*)strchr(s, ' ');
	if (cmd) {
	    len = cmd - s;
	    cmd = (char*)Z_Malloc(len + 1);
	    strlcpy(cmd, s, len + 1);

	    if (Cmd_Exists(cmd)) {
		s += len;
		while (*s == ' ')
		    s++;
		root = Cmd_ArgCompletions(cmd, s);
		if (root && root->entries) {
		    Con_Printf("%s\n", key_lines[edit_line]);
		    Con_ShowTree(root);
		    Z_Free(root);
		}
	    } else if (Cvar_FindVar(cmd)) {
		s += len;
		while (*s == ' ')
		    s++;
		root = Cvar_ArgCompletions(cmd, s);
		if (root && root->entries) {
		    Con_Printf("%s\n", key_lines[edit_line]);
		    Con_ShowTree(root);
		    Z_Free(root);
		}
	    }
	    Z_Free(cmd);
	}
    }
}

static void
EnterCommand(const char *buf)
{
    /*
     * Slash text are always commands.
     * Without the slash, we test for a valid command/alias/cvar.
     * If the check fails, we send the text as a chat message.
     */
    if (buf[0] == '\\' || buf[0] == '/')
	Cbuf_AddText("%s", buf + 1);
    else if (CheckForCommand())
	Cbuf_AddText("%s", buf);
    else {
	/* doesn't look like a command, convert to a chat message */
	if (cls.state >= ca_connected)
	    Cbuf_AddText("say ");
	Cbuf_AddText("%s", buf);
    }
    Cbuf_AddText("\n");
}

/*
====================
Key_Console

Interactive line editing and console scrollback
====================
*/
static void
Key_Console(int key)
{
    /* detect double presses of tab key */
    static qboolean tab_once = false;

    if (key != K_TAB)
	tab_once = false;

    if (key == K_ENTER) {
	EnterCommand(key_lines[edit_line] + 1);

	Con_Printf("%s\n", key_lines[edit_line]);
	edit_line = (edit_line + 1) & 31;
	history_line = edit_line;
	key_lines[edit_line][0] = ']';
	key_linepos = 1;
	if (cls.state == ca_disconnected)
	    /* force an update, because the command may take some time */
	    SCR_UpdateScreen();
	return;
    }

    if (key == K_TAB) {		/* command completion */
	if (tab_once) {
	    /* double tab */
	    ShowCompletions();
	    tab_once = false;
	    return;
	}
	tab_once = true;
	CompleteCommand();
	return;
    }

    if (key == K_BACKSPACE || key == K_LEFTARROW) {
	if (key_linepos > 1)
	    key_linepos--;
	return;
    }

    if (key == K_UPARROW) {
	do {
	    history_line = (history_line - 1) & 31;
	} while (history_line != edit_line && !key_lines[history_line][1]);
	if (history_line == edit_line)
	    history_line = (edit_line + 1) & 31;
	/* memmove + explicit NUL: history_line != edit_line is guaranteed
	 * by the check above, but GCC -Wrestrict cannot prove it from the
	 * & 31 indexing. memmove is well-defined regardless. */
	{
	    size_t n = strlen(key_lines[history_line]);
	    if (n >= MAXCMDLINE) n = MAXCMDLINE - 1;
	    memmove(key_lines[edit_line], key_lines[history_line], n);
	    key_lines[edit_line][n] = '\0';
	    key_linepos = (int)n;
	}
	return;
    }

    if (key == K_DOWNARROW) {
	if (history_line == edit_line)
	    return;
	do {
	    history_line = (history_line + 1) & 31;
	}
	while (history_line != edit_line && !key_lines[history_line][1]);
	if (history_line == edit_line) {
	    key_lines[edit_line][0] = ']';
	    key_linepos = 1;
	} else {
	    /* See K_UPARROW: memmove silences -Wrestrict that GCC cannot
	     * disprove from the & 31 array indexing. */
	    size_t n = strlen(key_lines[history_line]);
	    if (n >= MAXCMDLINE) n = MAXCMDLINE - 1;
	    memmove(key_lines[edit_line], key_lines[history_line], n);
	    key_lines[edit_line][n] = '\0';
	    key_linepos = (int)n;
	}
	return;
    }

    if (key == K_PGUP || key == K_MWHEELUP) {
	con->display -= 2;
	return;
    }

    if (key == K_PGDN || key == K_MWHEELDOWN) {
	con->display += 2;
	if (con->display > con->current)
	    con->display = con->current;
	return;
    }

    /* FIXME - only scroll back over _used_ lines (and 10 is arbitrary...) */
    if (key == K_HOME) {
	con->display = con->current - con_totallines + 10;
	return;
    }

    if (key == K_END) {
	con->display = con->current;
	return;
    }

    if (key < 32 || key > 127)
	return;			/* non printable */

    if (key_linepos < MAXCMDLINE - 1) {
	key_lines[edit_line][key_linepos] = key;
	key_linepos++;
	key_lines[edit_line][key_linepos] = 0;
    }
}

/* ============================================================================ */

qboolean chat_team;
char chat_buffer[MAXCMDLINE];
int chat_bufferlen = 0;

static void
Key_Message(int key)
{
    if (key == K_ENTER) {
	if (chat_team)
	    Cbuf_AddText("say_team \"%s\"\n", chat_buffer);
	else
	    Cbuf_AddText("say \"%s\"\n", chat_buffer);

	key_dest = key_game;
	chat_bufferlen = 0;
	chat_buffer[0] = 0;
	return;
    }

    if (key == K_ESCAPE) {
	key_dest = key_game;
	chat_bufferlen = 0;
	chat_buffer[0] = 0;
	return;
    }

    if (key < 32 || key > 127)
	return;			/* non printable */

    if (key == K_BACKSPACE) {
	if (chat_bufferlen) {
	    chat_bufferlen--;
	    chat_buffer[chat_bufferlen] = 0;
	}
	return;
    }

    if (chat_bufferlen == sizeof(chat_buffer) - 1)
	return;			/* all full */

    chat_buffer[chat_bufferlen++] = key;
    chat_buffer[chat_bufferlen] = 0;
}

/* ============================================================================ */


/*
===================
Key_StringToKeynum

Returns a key number to be used to index keybindings[] by looking at
the given string.  Single ascii characters return themselves, while
the K_* names are matched up.
===================
*/
static int
Key_StringToKeynum(const char *str)
{
    keyname_t *kn;

    if (!str || !str[0])
	return -1;
    if (!str[1])
	return str[0];

    for (kn = keynames; kn->name; kn++) {
	if (!strcasecmp(str, kn->name))
	    return kn->keynum;
    }
    return -1;
}

/*
===================
Key_KeynumToString

Returns a string (either a single ascii char, or a K_* name) for the
given keynum.
FIXME: handle quote special (general escape sequence?)
===================
*/
const char *
Key_KeynumToString(int keynum)
{
    keyname_t *kn;
    static char tinystr[2];

    if (keynum == -1)
	return "<KEY NOT FOUND>";
    if (keynum > 32 && keynum < 127) {	/* printable ascii */
	tinystr[0] = keynum;
	tinystr[1] = 0;
	return tinystr;
    }

    for (kn = keynames; kn->name; kn++)
	if (keynum == kn->keynum)
	    return kn->name;

    return "<UNKNOWN KEYNUM>";
}


/*
===================
Key_SetBinding
===================
*/
void
Key_SetBinding(knum_t keynum, const char *binding)
{
    char *newbinding;

    if (keynum == -1)
	return;

    /* free old bindings */
    if (keybindings[keynum]) {
	Z_Free((void *)keybindings[keynum]);
	keybindings[keynum] = NULL;
    }

    if (binding) {
	/* allocate memory for new binding */
	size_t bindlen = strlen(binding) + 1;
	newbinding = (char*)Z_Malloc(bindlen);
	memcpy(newbinding, binding, bindlen);
	keybindings[keynum] = newbinding;
    }
}

/*
===================
Key_Unbind_f
===================
*/
static void
Key_Unbind_f(void)
{
    int b;

    if (Cmd_Argc() != 2) {
	Con_Printf("unbind <key> : remove commands from a key\n");
	return;
    }

    b = Key_StringToKeynum(Cmd_Argv(1));
    if (b == -1) {
	Con_Printf("\"%s\" isn't a valid key\n", Cmd_Argv(1));
	return;
    }

    Key_SetBinding((knum_t)b, NULL);
}

static void
Key_Unbindall_f(void)
{
    int i;

    for (i = 0; i < K_LAST; i++)
	if (keybindings[i])
	    Key_SetBinding((knum_t)i, NULL);
}


/*
===================
Key_Bind_f
===================
*/
static void
Key_Bind_f(void)
{
    int i, argc, keynum, len;
    char cmd[1024];

    /* FIXME - allow arguments bound with the commands? */
    argc = Cmd_Argc();
    if (argc != 2 && argc != 3) {
	Con_Printf("bind <key> [command] : attach a command to a key\n");
	return;
    }

    keynum = Key_StringToKeynum(Cmd_Argv(1));
    if (keynum == -1) {
	Con_Printf("\"%s\" isn't a valid key\n", Cmd_Argv(1));
	return;
    }

    if (argc == 2) {
	if (keybindings[keynum])
	    Con_Printf("\"%s\" = \"%s\"\n", Cmd_Argv(1), keybindings[keynum]);
	else
	    Con_Printf("\"%s\" is not bound\n", Cmd_Argv(1));
	return;
    }

    cmd[0] = 0;
    len = 0;

    for (i = 2; i < argc; i++) {
	len += strlen(Cmd_Argv(i)) + ((i > 2) ? 1 : 0);
	if (len >= sizeof(cmd)) {
	    Con_Printf("bind command too long (MAX = %d)", (int)sizeof(cmd));
	    return;
	}
	if (i > 2)
	    strlcat(cmd, " ", sizeof(cmd));
	strlcat(cmd, Cmd_Argv(i), sizeof(cmd));
    }

    Key_SetBinding((knum_t)keynum, cmd);
}

/*
============
Key_WriteBindings

Writes lines containing "bind key value"
============
*/
void
Key_WriteBindings(RFILE *f)
{
    int i;

    for (i = 0; i < K_LAST; i++)
	if (keybindings[i])
	    rfprintf(f, "bind \"%s\" \"%s\"\n",
		    Key_KeynumToString(i), keybindings[i]);
}


/*
===================
Key_Init
===================
*/
void
Key_Init(void)
{
    int i;

    for (i = 0; i < 32; i++) {
	key_lines[i][0] = ']';
	key_lines[i][1] = 0;
    }
    key_linepos = 1;

/**/
/* init ascii characters in console mode */
/**/
    for (i = 32; i < 128; i++)
	consolekeys[i] = true;
    consolekeys[K_ENTER] = true;
    consolekeys[K_TAB] = true;
    consolekeys[K_LEFTARROW] = true;
    consolekeys[K_RIGHTARROW] = true;
    consolekeys[K_UPARROW] = true;
    consolekeys[K_DOWNARROW] = true;
    consolekeys[K_BACKSPACE] = true;
    consolekeys[K_HOME] = true;
    consolekeys[K_END] = true;
    consolekeys[K_PGUP] = true;
    consolekeys[K_PGDN] = true;
    consolekeys[K_LSHIFT] = true;
    consolekeys[K_RSHIFT] = true;
    consolekeys[K_MWHEELUP] = true;
    consolekeys[K_MWHEELDOWN] = true;

    for (i = 0; i < K_LAST; i++)
        keyshift[i] = i;
    for (i = 'a'; i <= 'z'; i++)
        keyshift[i] = i - 'a' + 'A';

    keyshift[K_BACKQUOTE] = K_ASCIITILDE;
    keyshift[K_1] = K_EXCLAIM;
    keyshift[K_2] = K_AT;
    keyshift[K_3] = K_HASH;
    keyshift[K_4] = K_DOLLAR;
    keyshift[K_5] = K_PERCENT;
    keyshift[K_6] = K_CARET;
    keyshift[K_7] = K_AMPERSAND;
    keyshift[K_8] = K_ASTERISK;
    keyshift[K_9] = K_LEFTPAREN;
    keyshift[K_0] = K_RIGHTPAREN;
    keyshift[K_MINUS] = K_UNDERSCORE;
    keyshift[K_EQUALS] = K_PLUS;
    keyshift[K_BACKSLASH] = K_BAR;
    keyshift[K_LEFTBRACKET] = K_BRACELEFT;
    keyshift[K_RIGHTBRACKET] = K_BRACERIGHT;
    keyshift[K_SEMICOLON] = K_COLON;
    keyshift[K_QUOTE] = K_QUOTEDBL;
    keyshift[K_COMMA] = K_LESS;
    keyshift[K_PERIOD] = K_GREATER;
    keyshift[K_SLASH] = K_QUESTION;

    menubound[K_ESCAPE] = true;
    for (i = K_F1; i <= K_F15; i++)
	menubound[i] = true;

/**/
/* register our functions */
/**/
    Cmd_AddCommand("bind", Key_Bind_f);
    Cmd_AddCommand("unbind", Key_Unbind_f);
    Cmd_AddCommand("unbindall", Key_Unbindall_f);
}

/*
===================
Key_Event

Called by the system between frames for both key up and key down events
Should NOT be called during an interrupt!
===================
*/
void
Key_Event(knum_t key, qboolean down)
{
    const char *kb;

    keydown[key] = down;

    if (!down)
	key_repeats[key] = 0;

    key_lastpress = key;

/* update auto-repeat status */
    if (down) {
	key_repeats[key]++;
	if (key != K_BACKSPACE
	    && key != K_PAUSE
	    && key != K_PGUP && key != K_PGDN && key_repeats[key] > 1)
	    return;		/* ignore most autorepeats */

	if (key >= K_MOUSE1 && !keybindings[key])
	    Con_Printf("%s is unbound, hit F4 to set.\n",
		       Key_KeynumToString(key));
    }

    if (key == K_LSHIFT)
	lshift_down = down;
    if (key == K_RSHIFT)
	rshift_down = down;

/**/
/* handle escape specialy, so the user can never unbind it */
/**/
    if (key == K_ESCAPE) {
	if (!down)
	    return;
	switch (key_dest) {
	case key_message:
	    Key_Message(key);
	    break;
	case key_menu:
	    M_Keydown(key);
	    break;
	case key_game:
	case key_console:
	    M_ToggleMenu_f();
	    break;
	default:
	    Sys_Error("Bad key_dest");
	}
	return;
    }
/**/
/* key up events only generate commands if the game key binding is */
/* a button command (leading + sign).  These will occur even in console mode, */
/* to keep the character from continuing an action started before a console */
/* switch.  Button commands include the kenum as a parameter, so multiple */
/* downs can be matched with ups */
/**/
    if (!down) {
	kb = keybindings[key];
	if (kb && kb[0] == '+')
	    Cbuf_AddText("-%s %i\n", kb + 1, key);
	if (keyshift[key] != key) {
	    kb = keybindings[keyshift[key]];
	    if (kb && kb[0] == '+')
		Cbuf_AddText("-%s %i\n", kb + 1, key);
	}
	return;
    }
/**/
/* during demo playback, most keys bring up the main menu */
/**/
    if (cls.demoplayback && consolekeys[key] && key_dest == key_game) {
	/* Don't override the console key */
	if (!keybindings[key] || strcmp(keybindings[key], "toggleconsole")) {
	    M_ToggleMenu_f();
	    return;
	}
    }
/**/
/* if not a consolekey, send to the interpreter no matter what mode is */
/**/
    if ((key_dest == key_menu && menubound[key])
	|| (key_dest == key_console && !consolekeys[key])
	|| (key_dest == key_game
	    && (!con_forcedup || !consolekeys[key]))) {
	kb = keybindings[key];
	if (kb) {
	    if (kb[0] == '+')
		/* button commands add keynum as a parm */
		Cbuf_AddText("%s %i\n", kb, key);
	    else
		Cbuf_AddText("%s\n", kb);
	}
	return;
    }

    if (!down)
	return;			/* other systems only care about key down events */

    if (lshift_down || rshift_down)
	key = (knum_t)keyshift[key];

    switch (key_dest) {
    case key_message:
	Key_Message(key);
	break;
    case key_menu:
	M_Keydown(key);
	break;

    case key_game:
    case key_console:
	Key_Console(key);
	break;
    default:
	Sys_Error("Bad key_dest");
    }
}

/*
===================
Key_ClearStates
===================
*/
void
Key_ClearStates(void)
{
    int i;

    for (i = 0; i < 256; i++) {
	keydown[i] = false;
	key_repeats[i] = 0;
    }
}

/*
===================
Key_ClearTyping
===================
*/
void
Key_ClearTyping(void)
{
    key_lines[edit_line][1] = 0;
    key_linepos = 1;
}
