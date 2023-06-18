#ifndef _WINDOW_H_
#define	_WINDOW_H_

#include "osd_cpu.h"
#include "mamedbg.h"

#ifdef  GNU
#define ARGFMT  __attribute__((format(printf,2,3)))
#else
#define ARGFMT
#endif

#ifndef DECL_SPEC
#define DECL_SPEC
#endif

#ifndef TRUE
#define TRUE    1
#endif

#ifndef FALSE
#define FALSE   0
#endif

#ifndef INVALID
#define INVALID 0xffffffff
#endif

#ifndef WIN_EMPTY
#define WIN_EMPTY   176 /* checkered pattern */
#endif
#ifndef CAPTION_L
#define CAPTION_L   174 /* >> */
#endif
#ifndef CAPTION_R
#define CAPTION_R   175 /* << */
#endif
#ifndef FRAME_TL
#define FRAME_TL    218 /* top, left border */
#endif
#ifndef FRAME_BL
#define FRAME_BL    192 /* bottom, left border */
#endif
#ifndef FRAME_TR
#define FRAME_TR    191 /* top, right border */
#endif
#ifndef FRAME_BR
#define FRAME_BR    217 /* bottom, right border */
#endif
#ifndef FRAME_V
#define FRAME_V     179 /* vertical line */
#endif
#ifndef FRAME_H
#define FRAME_H     196 /* horizontal line */
#endif

/* This is our window structure */

struct sWindow
{
	UINT8 filler;		/* Character */
	UINT8 prio; 		/* This window's priority */
	UINT32 x;			/* X Position (in characters) of our window */
	UINT32 y;			/* Y Position (in characters) of our window */
	UINT32 w;			/* X Size of our window (in characters) */
	UINT32 h;			/* Y Size (lines) of our window (in character lengths) */
	UINT32 cx;			/* Current cursor's X position */
	UINT32 cy;			/* Current cursor's Y position */
	UINT32 flags;		/* Window's attributes (below) */
	UINT8 co_text;		/* Default color */
	UINT8 co_frame; 	/* Frame color */
	UINT8 co_title; 	/* Title color */
	UINT8 saved_text;	/* Character under the cursor position */
	UINT8 saved_attr;	/* Attribute under the cursor position */

	/* Stuff that needs to be saved off differently */

	char	*title; /* Window title (if any) */
	UINT8	*text;	/* Pointer to video data - characters */
	UINT8	*attr;	/* Pointer to video data - attributes */

	/* These are the callbacks when certain things happen. All fields have been
	 * updated BEFORE the call. Return FALSE if the moves, resizes, closes,
	 * refocus aren't accepted.
	 */

	UINT32 (*Resize)(UINT32 idx, struct sWindow *);
	UINT32 (*Close)(UINT32 idx, struct sWindow *);
	UINT32 (*Move)(UINT32 idx, struct sWindow *);
	UINT32 (*Refocus)(UINT32 idx, struct sWindow *);  /* Bring it to the front */
};

/* These defines are for various aspects of the window */

#define BORDER_LEFT 		0x01	/* Border on left side of window */
#define BORDER_RIGHT		0x02	/* Border on right side of window */
#define BORDER_TOP			0x04	/* Border on top side of window */
#define BORDER_BOTTOM		0x08	/* Border on bottom side of window */
#define HIDDEN				0x10	/* Is it hidden currently? */
#define CURSOR_ON			0x20	/* Is the cursor on? */
#define NO_WRAP 			0x40	/* Do we actually wrap at the right side? */
#define NO_SCROLL			0x80	/* Do we actually scroll it? */
#define SHADOW				0x100	/* Do we cast a shadow? */
#define MOVEABLE			0x200	/* Is this Window moveable? */
#define RESIZEABLE			0x400	/* IS this Window resiable? */

#define MAX_WINDOWS 		32		/* Up to 32 windows active at once */
#define TAB_STOP			8		/* 8 Spaces for a tab stop! */

#define AUTO_FIX_XYWH		TRUE
#define NEWLINE_ERASE_EOL	TRUE	/* Shall newline also erase to end of line? */

/* Special characters */

#define CHAR_CURSORON		219 /* Cursor on character */
#define CHAR_CURSOROFF		32	/* Cursor off character */

/* Standard color set for IBM character set. DO NOT ALTER! */

#define WIN_BLACK			DBG_BLACK
#define WIN_BLUE			DBG_BLUE
#define WIN_GREEN			DBG_GREEN
#define WIN_CYAN			DBG_CYAN
#define WIN_RED 			DBG_RED
#define WIN_MAGENTA 		DBG_MAGENTA
#define WIN_BROWN			DBG_BROWN
#define WIN_WHITE			DBG_LIGHTGRAY
#define WIN_GRAY			DBG_GRAY
#define WIN_LIGHT_BLUE		DBG_LIGHTBLUE
#define WIN_LIGHT_GREEN 	DBG_LIGHTGREEN
#define WIN_LIGHT_CYAN		DBG_LIGHTCYAN
#define WIN_LIGHT_RED		DBG_LIGHTRED
#define WIN_LIGHT_MAGENTA	DBG_LIGHTMAGENTA
#define WIN_YELLOW			DBG_YELLOW
#define WIN_BRIGHT_WHITE	DBG_WHITE

#define	WIN_BRIGHT	0x08

/* Externs! */

extern UINT32 screen_w;
extern UINT32 screen_h;

extern void win_erase_eol(UINT32 idx, UINT8 bChar);
extern INT32 win_putc(UINT32 idx, UINT8 bChar);
extern UINT32 win_open(UINT32 idx, struct sWindow *psWin);
extern UINT32 win_init_engine(UINT32 w, UINT32 h);
extern UINT32 win_is_initalized(UINT32 idx);
extern void win_exit_engine(void);
extern void win_close(UINT32 idx);
extern INT32 win_vprintf(UINT32 idx, const char *pszString, va_list arg);
extern INT32 DECL_SPEC win_printf(UINT32 idx, const char *pszString, ...) ARGFMT;
extern UINT32 DECL_SPEC win_set_title(UINT32 idx, const char *pszTitle, ... ) ARGFMT;
extern UINT32 win_get_cx(UINT32 idx);
extern UINT32 win_get_cy(UINT32 idx);
extern UINT32 win_get_cx_abs(UINT32 idx);
extern UINT32 win_get_cy_abs(UINT32 idx);
extern UINT32 win_get_x_abs(UINT32 idx);
extern UINT32 win_get_y_abs(UINT32 idx);
extern UINT32 win_get_w(UINT32 idx);
extern UINT32 win_get_h(UINT32 idx);
extern void win_set_w(UINT32 idx, UINT32 w);
extern void win_set_h(UINT32 idx, UINT32 w);
extern void win_set_color(UINT32 idx, UINT32 color);
extern void win_set_title_color(UINT32 idx, UINT32 color);
extern void win_set_frame_color(UINT32 idx, UINT32 color);
extern void win_set_curpos(UINT32 idx, UINT32 x, UINT32 y);
extern void win_set_cursor(UINT32 idx, UINT32 dwCursorState);
extern void win_hide(UINT32 idx);
extern void win_show(UINT32 idx);
extern void win_update(UINT32 idx);
extern UINT8 win_get_prio(UINT32 idx);
extern void win_set_prio(UINT32 idx, UINT8 prio);
extern void win_move(UINT32 idx, UINT32 dwX, UINT32 dwY);
extern void win_invalidate_video(void);

#endif
