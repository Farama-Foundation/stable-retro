/****************************************************************************
 *	window.c
 *	Text mode window engine
 *
 *	Written by Neil Bradley (neil@synthcom.com)
 *	Heavily modified by Juergen Buchmueller (pullmoll@t-online.de)
 *
 *  Designed to fit the needs of the new MAME debugger (a bit more ;)
 *
 *	Warning: This code is still buggy!
 *	Some of the changes I made were contrary to the original design,
 *	so expect 'assertions' for every non common case (ie. window too
 *	big, hanging out of the screen etc.)
 ****************************************************************************/
#include <stdio.h>

#ifdef MAME_DEBUG

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include "window.h"

/*
 * These standard definition functions are macro'd so they can easily be
 * redefined to other custom procedures.
 */

#define ASSERT(expr)	assert(expr)

static INLINE void *MyMalloc( UINT32 size, const char *function )
{
	void *p = malloc( size );
    ASSERT( p );
	memset( p, 0, size );
	return p;
}

static INLINE void *MyReAlloc( void *p, UINT32 size, const char *function )
{
	p = realloc( p, size );
    ASSERT( p );
	return p;
}

static INLINE void MyFree( void **p, const char *function )
{
	if( *p )
	{
		free(*p);
		*p = NULL;
	}
}

/* Forward references for circular function calls */

void win_set_cursor_char(UINT32 idx, UINT32 state, UINT32 state_old);

/* Global windowing data we need */

UINT32 screen_w = 80;	/* Our screen's size in characters X */
UINT32 screen_h = 50;	/* Our screen's size in characters Y */

static struct sWindow *p_windows = NULL;
static UINT8 *p_prio_map = NULL;
static UINT8 *p_shadow_map = NULL;
static UINT8 *p_text = NULL;
static UINT8 *p_attr = NULL;

/************************************************************************
 *
 * Name : win_out
 *
 * Entry: Character, attribute and X/Y position of char to place
 *
 * Exit : None
 *
 * Description:
 *
 * This routine will place a character at a given video position.
 *
 ************************************************************************/

static INLINE void win_out(UINT8 bChar, UINT8 bAttr, UINT32 x, UINT32 y, UINT32 idx)
{
	UINT32 offs = (y * screen_w) + x;

	ASSERT(idx < MAX_WINDOWS);

	if( x >= screen_w || y >= screen_h )
		return;

	idx = p_windows[idx].prio;

	/* If we're under the influence of a Window's shadow, change the attribute */
	if( idx > p_shadow_map[offs] )
		bAttr = ( bAttr & 0x08 ) ? bAttr & 0x07 : 0x08;

	/* If it's different */
	if( bChar != p_text[offs] || bAttr != p_attr[offs] )
	{
		/* Put it in our video map */
		p_text[offs] = bChar;
		p_attr[offs] = bAttr;

		/* Here's where we draw the character */
		dbg_put_screen_char(bChar, bAttr, x, y);
	}
}

/************************************************************************
 *
 * Name : win_update_map
 *
 * Entry: Nothing
 *
 * Exit : Nothing
 *
 * Description:
 *
 * This routine will update the window priority map and
 * will recompute all windows.
 *
 ************************************************************************/

static void win_update_map(void)
{
	INT32 prio, i;
	UINT32 yadd, xadd, x, y;
	struct sWindow *pwin;

	ASSERT(p_prio_map); /* This had better not be null */
	ASSERT(p_windows);	/* This either */

	memset(p_prio_map, 0xff, screen_w * screen_h);

	for( prio = 0xff; prio >= 0; prio-- )
	{
		for( i = 0, pwin = p_windows; i < MAX_WINDOWS; i++, pwin++ )
		{
			if ( pwin->prio == prio && pwin->text && !(pwin->flags & HIDDEN) )
				break;
		}

		if( i != MAX_WINDOWS && pwin->x < screen_w )
		{
			UINT32 w;
			xadd = 0;
			yadd = 0;

			if( pwin->flags & BORDER_LEFT )   ++xadd;
			if( pwin->flags & BORDER_RIGHT )  ++xadd;
			if( pwin->flags & BORDER_TOP )	  ++yadd;
			if( pwin->flags & BORDER_BOTTOM ) ++yadd;

			w = pwin->w + xadd;
			if( w > screen_w )
				w = screen_w - pwin->x;

			for( y = pwin->y; y < screen_h && y < pwin->y + pwin->h + yadd; y++ )
				memset(&p_prio_map[y * screen_w + pwin->x], prio, w);
		}
	}

	/* Now create a shadow region (if applicable) */

	memset(p_shadow_map, 0xff, screen_w * screen_h);

	for( pwin = &p_windows[MAX_WINDOWS-1], i = MAX_WINDOWS-1; i >= 0; --i, --pwin )
	{
		/* Let's figure out if we should be doing a shadow */

		if( pwin->text && (pwin->flags & SHADOW) && !(pwin->flags & HIDDEN) )
		{
			yadd = pwin->y + pwin->h;
			xadd = pwin->x + pwin->w;

			/* If we have additional borders, extend it! */
			if( pwin->flags & BORDER_TOP )	  ++yadd;
			if( pwin->flags & BORDER_BOTTOM ) ++yadd;
			if( pwin->flags & BORDER_LEFT )   ++xadd;
			if( pwin->flags & BORDER_RIGHT )  ++xadd;

			/* If the line is still within range, go figure it! */

			if( (yadd + 1) < screen_h )
			{
				for( x = pwin->x + 1; x < screen_w && x < (xadd + 1); x++ )
				{
					if( pwin->prio < p_shadow_map[x + (yadd * screen_w)] )
						p_shadow_map[x + (yadd * screen_w)] = pwin->prio;
				}
			}

			/* Now let's draw us a vertical shadow line */
			if( (xadd + 1) < screen_w )
			{
				for( y = pwin->y + 1; y < yadd; y++ )
				{
					if( pwin->prio < p_shadow_map[xadd + (y * screen_w)] )
						p_shadow_map[xadd + (y * screen_w)] = pwin->prio;
				}
			}
		}
	}
}

/************************************************************************
 *
 * Name : win_update_shadow
 *
 * Entry: Window #, pointer to an array of bytes for affected windows
 *
 * Exit : Nothing
 *
 * Description:
 *
 * This routine will find all windows that are affected by a
 * shadow of a given window. Not actual updates are done.
 *
 ************************************************************************/

static void win_update_shadow(UINT32 idx, UINT8 *affected)
{
	struct sWindow *pwin = &p_windows[idx];
	UINT32 x, y, xadd, yadd;
	UINT8 bMap = 0;

	xadd = pwin->w;
	yadd = pwin->h;

	if( pwin->flags & BORDER_LEFT )   ++xadd;
	if( pwin->flags & BORDER_RIGHT )  ++xadd;
	if( pwin->flags & BORDER_TOP )	  ++yadd;
	if( pwin->flags & BORDER_BOTTOM ) ++yadd;

	/* Now check to see if we need to update anything because of the shadow */

	if( pwin->flags & SHADOW )
	{
		/* Check out the shadow on the bottom and see if we need */
		/* to update a window below. */
		y = yadd + pwin->y;
		if( y < screen_h )
		{
			for( x = pwin->x + 2; x < (pwin->x + xadd + 2); x++ )
			{
				if( x < screen_w )
				{
					bMap = p_prio_map[(y * screen_w) + x];
					if( 0xff != bMap )
						affected[bMap] = 1;
				}
			}
		}

		/* And now down the right side */
		for( y = pwin->y + 1; y < screen_h && y < pwin->y + yadd + 1; y++ )
		{
			for( x = xadd + pwin->x; x < (xadd + pwin->x + 2); x++ )
			{
				if( x < screen_w )
				{
					bMap = p_prio_map[(y * screen_w) + x];
					if( 0xff != bMap )
						affected[bMap] = 1;
				}
			}
		}
	}
}

/************************************************************************
 *
 * Name : win_update
 *
 * Entry: Window # to update
 *
 * Exit : Nothing
 *
 * Description:
 *
 * This routine will update a given window on the screen.
 *
 ************************************************************************/

void win_update(UINT32 idx)
{
	struct sWindow *pwin = &p_windows[idx];
	UINT8 affected[MAX_WINDOWS];
	UINT32 x0, y0, x, y, win_offs, scr_offs;
	UINT8 ch, color;

	ASSERT(idx < MAX_WINDOWS);	/* This had better be in range */
	ASSERT(p_windows);			/* And this had better be initialized */

	memset( affected, 0, sizeof(affected) );

	/* If this isn't a valid window or it's hidden, just bail out */

	if( NULL == pwin->text || (pwin->flags & HIDDEN) )
		return;

	/* Let's whip through and draw the whole stinking window, shall we? */

	x0 = pwin->x;
	y0 = pwin->y;

	if( pwin->flags & BORDER_LEFT ) ++x0;
	if( pwin->flags & BORDER_TOP )	++y0;

	color = pwin->co_frame;

	/* See if we need a character in the upper left hand corner of our window */
	if( (pwin->flags & BORDER_TOP) && (pwin->flags & BORDER_LEFT) )
	{
		if( pwin->x < screen_w && pwin->y < screen_h )
			if( p_prio_map[pwin->x + (pwin->y * screen_w)] >= pwin->prio )
				win_out(FRAME_TL, color, pwin->x, pwin->y, idx);
	}

	/* See if we need a character in the lower left hand corner of our window */
	if( (pwin->flags & BORDER_BOTTOM) && (pwin->flags & BORDER_LEFT) )
	{
		if( pwin->x < screen_w && pwin->y + y0 < screen_h )
			if( p_prio_map[pwin->x + ((y0 + pwin->h) * screen_w)] >= pwin->prio )
				win_out(FRAME_BL, color, pwin->x, pwin->h + y0, idx);
	}

	/* See if we need a character in the upper right hand corner of our window */
	if( (pwin->flags & BORDER_TOP) && (pwin->flags & BORDER_RIGHT) )
	{
		if( pwin->x + x0 < screen_w && pwin->y < screen_h )
			if( p_prio_map[pwin->w + x0 + (pwin->y * screen_w)] >= pwin->prio )
				win_out(FRAME_TR, color, pwin->w + x0, pwin->y, idx);
	}

	/* See if we need a character in the lower right hand corner of our window */
	if( (pwin->flags & BORDER_BOTTOM) && (pwin->flags & BORDER_RIGHT) )
	{
		if( pwin->x + x0 < screen_w && pwin->y + y0 < screen_h )
			if( p_prio_map[pwin->w + x0 + ((pwin->h + y0) * screen_w)] >= pwin->prio )
				win_out(FRAME_BR, color, pwin->w + x0, pwin->h + y0, idx);
    }

	/* Let's go through and draw the frame for the window (if any) */
	if( pwin->flags & BORDER_LEFT )    /* Here we have a left border */
	{
        for( y = y0; y < (y0 + pwin->h); y++ )
		{
			if( p_prio_map[pwin->x + (y * screen_w)] >= pwin->prio )
				win_out(FRAME_V, color, pwin->x, y, idx);
		}
	}

	/* Let's draw the right side of the window (if any) */
	if( pwin->flags & BORDER_RIGHT ) /* Here we have a right border */
	{
		if( pwin->w + x0 < screen_w )
			for( y = y0; y < screen_h && y < (y0 + pwin->h); y++ )
			{
				if( p_prio_map[pwin->w + x0 + (y * screen_w)] >= pwin->prio )
					win_out(FRAME_V, color, x0 + pwin->w, y, idx);
			}
	}

	/* Let's draw the bottom side of the window (if any) */
	if( pwin->flags & BORDER_BOTTOM )
	{
		if( pwin->h + y0 < screen_h )
			for( x = x0; x < (x0 + pwin->w); x++ )
			{
				if( p_prio_map[((y0 + pwin->h) * screen_w) + x] >= pwin->prio )
					win_out(FRAME_H, color, x, y0 + pwin->h, idx);
			}
	}

	/* And now the top! */
	if( pwin->flags & BORDER_TOP )
	{
		/* If we've got a title, let's put that in, too... */
		if( pwin->title )
        {
			UINT32 i, j, length1, length2;
			char *p = strchr(pwin->title, '\t');

			/* If the title contains a tab, split it into two parts */
            if( p )
			{
                i = 0;
                j = 1;
				length1 = (UINT32)(p - pwin->title);
				length2 = strlen(p + j);

				for( x = x0; x < screen_w && x < (x0 + pwin->w); x++ )
				{
					if( p_prio_map[x + (pwin->y * screen_w)] < pwin->prio )
						continue;
					color = pwin->co_frame;
                    if( x < (x0 + 1) )
					{
						ch = FRAME_H;
					}
                    else
					if( x > (x0 + 1 + length1 + 1) )
					{
						if( x < (x0 + pwin->w - 1 - length2 - 1) )
						{
							ch = FRAME_H;
						}
						else
						{
							if( x == (x0 + pwin->w - 1) )
								ch = CAPTION_R;
							else
							if( x == (x0 + pwin->w - 1 - length2 - 1) )
								ch = CAPTION_L;
							else
							{
                                color = pwin->co_title;
								ch = p[j++];
							}
						}
					}
					else
					{
						if( x == (x0 + 1) )
							ch = CAPTION_L;
						else
						if( x == (x0 + 1 + length1 + 1) )
							ch = CAPTION_R;
						else
						{
                            color = pwin->co_title;
							ch = pwin->title[i++];
						}
					}
					win_out(ch, color, x, pwin->y, idx);
                }
            }
			else
			{
				/* Draw a top border with a title in the left part */
				length1 = strlen(pwin->title);
				i = 0;
				for( x = x0; x < screen_w && x < (x0 + pwin->w); x++ )
				{
					if( p_prio_map[x + (pwin->y * screen_w)] < pwin->prio )
						continue;
					color = pwin->co_frame;
                    if( x < (x0 + 1) || x > (x0 + 1 + length1 + 1) )
					{
						ch = FRAME_H;
					}
					else
					{
						if( x == (x0 + 1) )
							ch = CAPTION_L;
						else
						if( x == (x0 + 1 + length1 + 1) )
							ch = CAPTION_R;
						else
						{
                            color = pwin->co_title;
							ch = pwin->title[i++];
						}
					}
					win_out(ch, color, x, pwin->y, idx);
				}
			}
        }
		else
		{
			for( x = x0; x < screen_w && x < (x0 + pwin->w); x++ )
			{
				if( p_prio_map[(pwin->y * screen_w) + x] >= pwin->prio )
					win_out(FRAME_H, color, x, pwin->y, idx);
			}
		}

	}

	/* Loop through our existing window and update it on the screen */
	for( y = 0; y < pwin->h; y++ )
	{
		win_offs = y * screen_w;
		scr_offs = (y0 + y) * screen_w + x0;
		for( x = 0; x < pwin->w; x++ )
		{
			if( p_prio_map[scr_offs] >= pwin->prio )
			{
				ch = pwin->text[win_offs];
				color = pwin->attr[win_offs];
				win_out(ch, color, x0 + x, y0 + y, idx);
			}
			win_offs++;
			scr_offs++;
		}
	}
}

/************************************************************************
 *
 * Name : win_erase
 *
 * Entry: Window index # to erase
 *
 * Exit : Nothing
 *
 * Description:
 *
 * This routine will erase a window from the physical display (including
 * borders if applicable)
 *
 ************************************************************************/

void win_erase(UINT32 idx)
{
	struct sWindow *pwin = &p_windows[idx];
	UINT8 affected[MAX_WINDOWS];
	UINT32 i = 0;
	UINT32 flags_old;
	UINT8 bMap = 0;
	UINT32 x, y;
	UINT32 xadd = 0;
	UINT32 yadd = 0;

	ASSERT(idx < MAX_WINDOWS);	/* This had better be in range */
	ASSERT(p_windows);			/* And this had better be initialized */

	if( NULL == pwin->text )
		return;

	memset( affected, 0, sizeof(affected) );

	/* Let's fake like the window is gone */

	flags_old = pwin->flags;
	pwin->flags |= HIDDEN;

	/* Recompute the display matrix */

	win_update_map();

	xadd = pwin->w;
	yadd = pwin->h;

	if( pwin->flags & BORDER_LEFT )   ++xadd;
	if( pwin->flags & BORDER_RIGHT )  ++xadd;
	if( pwin->flags & BORDER_TOP )	  ++yadd;
	if( pwin->flags & BORDER_BOTTOM ) ++yadd;

	win_set_cursor_char(idx, 0, pwin->flags & CURSOR_ON);

	/* Go see if we need to do anything about our shadows */
	for( y = pwin->y; y < screen_h && y < (pwin->y + yadd); y++ )
	{
		for( x = pwin->x; x < screen_w && x < (pwin->x + xadd); x++ )
		{
			bMap = p_prio_map[(y * screen_w) + x];
			if( 0xff == bMap )
				win_out(WIN_EMPTY, WIN_WHITE, x, y, idx);
			else
				affected[bMap] = 1;
		}
	}

	win_update_shadow(idx, affected);

	/* Now that we've erased the residuals, let's go update */
	/* the windows that need it */

	for( i = 0; i < MAX_WINDOWS; i++ )
	{
		if( affected[i] || (i != idx) )
		{
			win_update(i);
			win_set_cursor_char(i, p_windows[i].flags & CURSOR_ON, 0);
		}
	}

	pwin->flags = flags_old;
}

/************************************************************************
 *
 * Name : win_set_cursor_char
 *
 * Entry: Window # to set cursor state, and state of cursor
 *
 * Exit : Nothing
 *
 * Description:
 *
 * This routine will either turn on/off the cursor in a given window.
 *
 ************************************************************************/

void win_set_cursor_char(UINT32 idx, UINT32 state, UINT32 state_old)
{
	struct sWindow *pwin = &p_windows[idx];
	UINT32 x0, y0, win_offs;

	if( INVALID == idx )
		return;

	if( NULL == pwin->text )
		return;

    if( pwin->flags & HIDDEN )
		return;

	if( pwin->flags & NO_WRAP )
	{
		if( pwin->x >= pwin->w )
			return;
		if( pwin->y >= pwin->h )
			return;
	}

    x0 = pwin->x + pwin->cx;
	y0 = pwin->y + pwin->cy;

	if( pwin->flags & BORDER_LEFT ) ++x0;
	if( pwin->flags & BORDER_TOP )	++y0;

	/* If we are outside of the physical screen, just exit */
    if( x0 >= screen_w || y0 >= screen_h )
		return;

	win_offs = y0 * screen_w + x0;

	if( p_prio_map[win_offs] < pwin->prio )
		return;

	if( state )
	{
		pwin->saved_text = p_text[win_offs];
		pwin->saved_attr = p_attr[win_offs];
		win_out(CHAR_CURSORON, WIN_BRIGHT_WHITE, x0, y0, idx);
	}
	else
	{
		if( 0 != state_old )
			win_out(pwin->saved_text, pwin->saved_attr, x0, y0, idx);
	}
}

/************************************************************************
 *
 * Name : win_is_initalized()
 *
 * Entry: Nothing
 *
 * Exit : TRUE if a window is already initialized
 *
 * Description:
 *
 * This routine returns the initialization status of a window
 *
 ************************************************************************/

UINT32 win_is_initalized(UINT32 idx)
{
	struct sWindow *pwin = &p_windows[idx];

	if( !p_windows )
		return FALSE;

    if( idx >= MAX_WINDOWS )
		return FALSE;
	if( pwin->text == NULL )
		return FALSE;
	if( pwin->attr == NULL )
        return FALSE;

    return TRUE;
}

/************************************************************************
 *
 * Name : win_init_engine()
 *
 * Entry: Nothing
 *
 * Exit : Nothing
 *
 * Description:
 *
 * This routine will initialize all variables needed for the windowing
 * engine to start.
 *
 ************************************************************************/

UINT32 win_init_engine(UINT32 w, UINT32 h)
{
	UINT32 x, y;

	/* Uninitialize if we are currently initialized */

	win_exit_engine();	/* Just in case! */

	screen_w = w;
	screen_h = h;

	/* Allocate memory for some things */

	p_text = MyMalloc(screen_w * screen_h, "win_init_engine()");
	p_attr = MyMalloc(screen_w * screen_h, "win_init_engine()");
	p_windows = MyMalloc(sizeof(struct sWindow) * MAX_WINDOWS, "win_init_engine()");
	p_prio_map = MyMalloc(screen_w * screen_h, "win_init_engine()");
	p_shadow_map = MyMalloc(screen_w * screen_h, "win_init_engine()");

	win_update_map();

	for (y = 0; y < screen_h; y++)
		for (x = 0; x < screen_w; x++)
			win_out(WIN_EMPTY, WIN_WHITE, x, y, 0);

	return(TRUE);
}

/************************************************************************
 *
 * Name : win_exit_engine
 *
 * Entry: Nothing
 *
 * Exit : Nothing
 *
 * Description:
 *
 * This routine will shut down the windowing engine
 *
 ************************************************************************/

void win_exit_engine(void)
{
	UINT32 x, y, i;

	/* Clear the screen. This *MUST* be before the shadow map is freed! */

	if( p_windows )
	{
		if( p_text )
		{
			for (y = 0; y < screen_h; y++)
				for (x = 0; x < screen_w; x++)
					win_out(' ', WIN_WHITE, x, y, 0);
		}

		for (i = 0; i < MAX_WINDOWS; i++)
		{
			if( p_windows[i].text )
				MyFree((void **) &p_windows[i].text, "win_exit_engine()");
			if( p_windows[i].attr )
				MyFree((void **) &p_windows[i].attr, "win_exit_engine()");
        }
	}

	if( p_windows )
		MyFree((void **) &p_windows, "InitWindowEngine()");
	if(  p_prio_map )
		MyFree((void **) &p_prio_map, "InitWindowEngine()");
	if( p_shadow_map )
		MyFree((void **) &p_shadow_map, "InitWindowEngine()");
	if( p_text )
		MyFree((void **) &p_text, "InitWindowEngine()");
	if( p_attr )
		MyFree((void **) &p_attr, "InitWindowEngine()");
}

/************************************************************************
 *
 * Name : win_open
 *
 * Entry: Window structure, and priority desired
 *
 * Exit : FALSE If couldn't be opened, or TRUE if successful
 *
 * Description:
 *
 * This routine will allow one to open a window.
 *
 ************************************************************************/

UINT32 win_open(UINT32 idx, struct sWindow *psWin)
{
	struct sWindow *pwin = &p_windows[idx];
	UINT8 affected[MAX_WINDOWS];
	UINT32 xadd, yadd, i;
	UINT8 ch = 0, color = 0;

	ASSERT(idx < MAX_WINDOWS);	/* This had better be in range */
	ASSERT(p_windows);			/* And this had better be initialized */
	ASSERT(psWin);				/* This, too. */

	memset( affected, 0, sizeof(affected) );

	/* Is the window already open? Return FALSE if so */

	if( pwin->text )
		return(FALSE);

	xadd = 0;
	yadd = 0;

	if( psWin->flags & BORDER_LEFT )
		++xadd;
	if( psWin->flags & BORDER_RIGHT )
		++xadd;
	if( psWin->flags & BORDER_TOP )
		++yadd;
	if( psWin->flags & BORDER_BOTTOM )
		++yadd;

/*	ASSERT((psWin->x + psWin->w + xadd) <= screen_w);*/
/*	ASSERT((psWin->y + psWin->h + yadd) <= screen_h);*/

	psWin->text = MyMalloc( screen_w * (yadd + psWin->h), "win_open()" );
	psWin->attr = MyMalloc( screen_w * (yadd + psWin->h), "win_open()" );
	psWin->title = MyMalloc( screen_w + 1, "win_open()" );

	/* This is our fill character */
	ch = psWin->filler;
	color = psWin->co_text;

	for (i = 0; i < screen_w * (yadd + psWin->h); i++)
	{
		psWin->text[i] = ch;
		psWin->attr[i] = color;
	}

    psWin->cx = 0;
	psWin->cy = 0;

	/* Copy it into the regular structure, update the window map and show */
	/* the window (if it's not hidden) */

	memcpy(pwin, psWin, sizeof(struct sWindow));

	win_update_map();
	win_update_shadow(idx, affected);

	/* Now that we've erased the residuals, let's go update the windows */
	/* that need it */

	for (i = 0; i < MAX_WINDOWS; i++)
	{
		if( affected[i] || (i == idx) )
		{
			win_update(i);
			win_set_cursor_char(i, p_windows[i].flags & CURSOR_ON, 0);
		}
	}

	pwin->saved_text = ch;
	pwin->saved_attr = color;
	win_set_cursor_char(idx, pwin->flags & CURSOR_ON, 0);

	/* Now that the window is opened, if there's a resize handler available, */
	/* call it! */

	if( pwin->Resize )
		pwin->Resize(idx, pwin);

	return(TRUE);
}

/************************************************************************
 *
 * Name : win_close
 *
 * Entry: Window #
 *
 * Exit : Nothing
 *
 * Description:
 *
 * This routine will close down a currently opened window and erase it from
 * the visual screen.
 *
 ************************************************************************/

void win_close(UINT32 idx)
{
	struct sWindow *pwin = &p_windows[idx];

	ASSERT(idx < MAX_WINDOWS);	/* This had better be in range */
	ASSERT(p_windows);			/* And this had better be initialized */

	if( NULL == pwin->text )
		return;

	/* Call the shutdown client if applicable */

	if( pwin->Close )
	{
		if( FALSE == pwin->Close(idx, pwin) )
			return;
	}

	/* Erase the window from the screen */

	win_erase(idx);

	/* Delete all we've allocated */

	MyFree((void **) &pwin->text, "win_close()"); /* Free our video data */
	MyFree((void **) &pwin->attr, "win_close()"); /* Free our video data */
	MyFree((void **) &pwin->title, "win_close()"); /* Free our video data */
}

/************************************************************************
 *
 * Name : win_scroll
 *
 * Entry: Window # to scroll
 *
 * Exit : Window is scrolled and updated (if currently visible)
 *
 * Description:
 *
 * This routine will scroll a window
 *
 ************************************************************************/

UINT32 win_scroll(UINT32 idx)
{
	struct sWindow *pwin = &p_windows[idx];
	UINT32 i;
	UINT8 *ptext, *pattr, color;

	ASSERT(idx < MAX_WINDOWS);	/* This had better be in range */
	ASSERT(p_windows);			/* And this had better be initialized */

	/* Is the window already open? Return FALSE if so */

	if( NULL == pwin->text )
		return(FALSE);

	/* If we don't scroll it, don't scroll it! */

	if( pwin->flags & NO_SCROLL )
		return(FALSE);

	/* Let's do the scroll */

	ptext = pwin->text;
	pattr = pwin->attr;

	if( pwin->h != 1 )
	{
		memcpy( ptext, ptext + screen_w, screen_w * (pwin->h - 1) );
		memcpy( pattr, pattr + screen_w, screen_w * (pwin->h - 1) );
	}

	/* Now that we've done the scroll, let's clear out the bottom line */

	color = pwin->co_text;
	for (i = 0; i < pwin->w; i++)
	{
		*ptext++ = ' ';
		*pattr++ = color;
	}

	win_update(idx);
	return(TRUE);
}

/************************************************************************
 *
 * Name : win_internal_putchar
 *
 * Entry: Character to display
 *
 * Exit : relative offset of the cursor after the output
 *
 * Description:
 *
 * This routine will put a character to the currently active window. This
 * routine does not take into account the cursor! Use win_putc or win_printf
 * instead!
 *
 ************************************************************************/

INT32 win_internal_putchar(UINT32 idx, UINT8 ch)
{
	struct sWindow *pwin = &p_windows[idx];
	INT32 rel = 0;
	UINT32 x0, y0;
	UINT8 color;

	if( NULL == pwin->text )
		return 0;

	color = pwin->co_text;

    switch( ch )
	{
	case '\b':  /* Backspace? */
		if( pwin->cx )
		{
			pwin->cx--;
			rel--;
		}
		break;

	case '\r':  /* Carriage return */
		rel = - pwin->cx;
		pwin->cx = 0;
		break;

	case '\n':  /* Newline or linefeed? */
#if NEWLINE_ERASE_EOL
		win_erase_eol( idx, ' ' );
#endif
		rel = - pwin->cx;
		pwin->cx = 0;
		pwin->cy++;
		if( pwin->cy >= pwin->h )
		{
			pwin->cy--;
			win_scroll(idx);
		}
		break;

	case '\t':  /* Tab? */
		do
		{
			rel += win_internal_putchar( idx, ' ');
		} while( pwin->cx % TAB_STOP );
		break;

    default:
        x0 = pwin->x + pwin->cx;
		y0 = pwin->y + pwin->cy;

		if( pwin->flags & BORDER_LEFT )
			++x0;
		if( pwin->flags & BORDER_TOP )
			++y0;

		/* Sanity check */
        if( x0 < screen_w && y0 < screen_h )
		{
			pwin->text[pwin->cy * screen_w + pwin->cx] = ch;
            pwin->attr[pwin->cy * screen_w + pwin->cx] = color;
			if( pwin->cx < pwin->w )
			{
				if( p_prio_map[y0 * screen_w + x0] >= pwin->prio && !(pwin->flags & HIDDEN) )
					win_out(ch, color, x0, y0, idx);
			}
		}

		rel++;
		pwin->cx++;
		if( pwin->cx >= pwin->w )
		{
			/* If we do not wrap at the right side, just exit */
            if( pwin->flags & NO_WRAP )
				return rel;
            pwin->cx = 0;
			pwin->cy++;
			if( pwin->cy >= pwin->h )
			{
				win_scroll(idx);
                pwin->cy--;
			}
		}
    }
	return rel;
}

/************************************************************************
 *
 * Name : win_putc
 *
 * Entry: Character to print
 *
 * Exit : relative offset of the cursor after the output
 *
 * Description:
 *
 * This routine will put a character to the currently defined window.
 *
 ************************************************************************/

INT32 win_putc(UINT32 idx, UINT8 bChar)
{
	struct sWindow *pwin = &p_windows[idx];
	INT32 rel;

	ASSERT(idx < MAX_WINDOWS);	/* This had better be in range */
	ASSERT(p_windows);			/* And this had better be initialized */

    if( NULL == pwin->text )
		return 0;

	if( pwin->flags & CURSOR_ON )
		win_set_cursor_char(idx, 0, pwin->flags);

	rel = win_internal_putchar(idx, bChar);

	if( pwin->flags & CURSOR_ON )
		win_set_cursor_char(idx, 1, 0);

	return rel;
}

/************************************************************************
 *
 * Name : win_erase_eol
 *
 * Entry: Window # and character to fill with
 *
 * Exit : None
 *
 * Description:
 *
 * This routine will fill a character to the end of the line
 *
 ************************************************************************/

void win_erase_eol(UINT32 idx, UINT8 ch)
{
	struct sWindow *pwin = &p_windows[idx];
	UINT32 i, x, y;
	UINT32 flags_old;

	ASSERT(idx < MAX_WINDOWS);	/* This had better be in range */
	ASSERT(p_windows);			/* And this had better be initialized */

    if( NULL == pwin->text )
		return;

	flags_old = pwin->flags;
	pwin->flags |= NO_SCROLL;

	if( pwin->flags & CURSOR_ON )
		win_set_cursor_char(idx, 0, pwin->flags);

	/* Do the fill! */

	x = pwin->cx;
	y = pwin->cy;
	for( i = x; i < pwin->w ; i++ )
		win_internal_putchar(idx, ch);

	pwin->cx = x;
	pwin->cy = y;
	pwin->flags = flags_old;

	if( pwin->flags & CURSOR_ON )
		win_set_cursor_char(idx, 1, 0);
}

/************************************************************************
 *
 * Name : win_set_curpos
 *
 * Entry: Window #	and x/y coordinates within the window
 *
 * Exit : Nothing
 *
 * Description:
 *
 * This routine will position the cursor within the Window.
 *
 ************************************************************************/

void win_set_curpos(UINT32 idx, UINT32 x, UINT32 y)
{
	struct sWindow *pwin = &p_windows[idx];

	ASSERT(idx < MAX_WINDOWS);	/* This had better be in range */
	ASSERT(p_windows);			/* And this had better be initialized */

    if( NULL == pwin->text )
		return;

	/* Make sure we're in range */

	/* If we do not wrap at the right side, just exit */
	if( !(pwin->flags & NO_WRAP) )
	{
		if( x >= pwin->w )
			return;
		if( y >= pwin->h )
			return;
	}

	win_set_cursor_char(idx, 0, pwin->flags & CURSOR_ON);

	/* Now put the cursor there */

	pwin->cx = x;
	pwin->cy = y;
	win_set_cursor_char(idx, pwin->flags & CURSOR_ON, 0);
}

static char tmp_text[450];

/************************************************************************
 *
 * Name : win_vprintf
 *
 * Entry: window # to and format (with optional arguments) to print
 *
 * Exit : Nothing
 *
 * Description:
 *
 * This routine will send a null terminated string to a window
 *
 ************************************************************************/

INT32 win_vprintf(UINT32 idx, const char *fmt, va_list arg)
{
    struct sWindow *pwin = &p_windows[idx];
    char *src = tmp_text;
    int length = 0;

	ASSERT(idx < MAX_WINDOWS);	/* This had better be in range */
	ASSERT(p_windows);			/* And this had better be initialized */

    if( NULL == pwin->text )
        return length;

    if( pwin->flags & CURSOR_ON )
        win_set_cursor_char(idx, 0, pwin->flags & CURSOR_ON);

	length = vsprintf(tmp_text, fmt, arg);

    while( *src )
        win_internal_putchar( idx, *src++ );

    if( pwin->flags & CURSOR_ON )
        win_set_cursor_char(idx, 1, 0);

    return length;
}


/************************************************************************
 *
 * Name : win_printf
 *
 * Entry: window # to and format (with optional arguments) to print
 *
 * Exit : Nothing
 *
 * Description:
 *
 * This routine will send a null terminated string to a window
 *
 ************************************************************************/

INT32 DECL_SPEC win_printf(UINT32 idx, const char *fmt, ...)
{
	struct sWindow *pwin = &p_windows[idx];
	char *src = tmp_text;
	int length = 0;
	va_list arg;

	ASSERT(idx < MAX_WINDOWS);	/* This had better be in range */
	ASSERT(p_windows);			/* And this had better be initialized */

    if( NULL == pwin->text )
		return length;

	if( pwin->flags & CURSOR_ON )
		win_set_cursor_char(idx, 0, pwin->flags & CURSOR_ON);

	va_start(arg, fmt);
	length = vsprintf(tmp_text, fmt, arg);
	va_end(arg);

	while( *src )
		win_internal_putchar( idx, *src++ );

	if( pwin->flags & CURSOR_ON )
		win_set_cursor_char(idx, 1, 0);

	return length;
}

/************************************************************************
 *
 * Name : win_set_color
 *
 * Entry: Window # and color
 *
 * Exit : Nothing
 *
 * Description:
 *
 * This routine sets the fore- and background colors for
 * subsequent text outputs (win_putc and win_printf)
 *
 ************************************************************************/

void win_set_color(UINT32 idx, UINT32 color)
{
	struct sWindow *pwin = &p_windows[idx];

	ASSERT(idx < MAX_WINDOWS);	/* This had better be in range */
	ASSERT(p_windows);			/* And this had better be initialized */

    if( NULL == pwin->text )
		return;

	pwin->co_text = color;
}

/************************************************************************
 *
 * Name : win_set_title_color
 *
 * Entry: Window # and new title color
 *
 * Exit : Nothing
 *
 * Description:
 *
 * This routine sets the color for title of a window
 *
 ************************************************************************/

void win_set_title_color(UINT32 idx, UINT32 color)
{
	struct sWindow *pwin = &p_windows[idx];

	ASSERT(idx < MAX_WINDOWS);	/* This had better be in range */
	ASSERT(p_windows);			/* And this had better be initialized */

    if( NULL == pwin->text )
		return;

	pwin->co_title = color;
	win_update( idx );
}

/************************************************************************
 *
 * Name : win_set_frame_color
 *
 * Entry: Window # and new frame color
 *
 * Exit : Nothing
 *
 * Description:
 *
 * This routine sets the color for title of a window
 *
 ************************************************************************/

void win_set_frame_color(UINT32 idx, UINT32 color)
{
	struct sWindow *pwin = &p_windows[idx];

	ASSERT(idx < MAX_WINDOWS);	/* This had better be in range */
	ASSERT(p_windows);			/* And this had better be initialized */

    if( NULL == pwin->text )
		return;

	pwin->co_frame = color;
	win_update( idx );
}

/************************************************************************
 *
 * Name : win_set_cursor
 *
 * Entry: Window # and cursor state
 *
 * Exit : Nothing
 *
 * Description:
 *
 * This routine sets the cursor state to on (non zero) or off (zero)
 *
 ************************************************************************/

void win_set_cursor(UINT32 idx, UINT32 state)
{
	struct sWindow *pwin = &p_windows[idx];
	UINT32 cursor;

	ASSERT(idx < MAX_WINDOWS);	/* This had better be in range */
	ASSERT(p_windows);			/* And this had better be initialized */

    if( NULL == pwin->text )
		return;

	if( state )
		cursor = pwin->flags |= CURSOR_ON;
	else
		cursor = pwin->flags &= ~CURSOR_ON;

	win_set_cursor_char(idx, state, pwin->flags & CURSOR_ON);

	pwin->flags = cursor;
}

/************************************************************************
 *
 * Name : win_hide
 *
 * Entry: Window # to hide
 *
 * Exit : Nothing
 *
 * Description:
 *
 * This routine will hide a window.
 *
 ************************************************************************/

void win_hide(UINT32 idx)
{
	struct sWindow *pwin = &p_windows[idx];

	ASSERT(idx < MAX_WINDOWS);	/* This had better be in range */
	ASSERT(p_windows);			/* And this had better be initialized */

	/* If it's already hidden, don't hide it again! */

	if( pwin->flags & HIDDEN )
		return;

	/* Otherwise, hide it */

	win_erase(idx);
	pwin->flags |= HIDDEN;
}

/************************************************************************
 *
 * Name : win_show
 *
 * Entry: Window # to show
 *
 * Exit : Nothing
 *
 * Description:
 *
 * This routine will show a window.
 *
 ************************************************************************/

void win_show(UINT32 idx)
{
	struct sWindow *pwin = &p_windows[idx];

	ASSERT(idx < MAX_WINDOWS);	/* This had better be in range */
	ASSERT(p_windows);			/* And this had better be initialized */

	/* Don't redraw the window if it's already shown */

	if( pwin->flags & HIDDEN )
	{
		pwin->flags &= ~HIDDEN;
		win_update_map();
	}

	/* But update them */
	win_update(idx);
}

/************************************************************************
 *
 * Name : win_set_title
 *
 * Entry: Window # and new title
 *
 * Exit :
 *
 * Description:
 *
 * Changes the title of a window and updates the screen
 *
 ************************************************************************/

UINT32 DECL_SPEC win_set_title(UINT32 idx, const char *fmt, ... )
{
	struct sWindow *pwin = &p_windows[idx];
	va_list arg;

	ASSERT(idx < MAX_WINDOWS);	/* This had better be in range */
	ASSERT(p_windows);			/* And this had better be initialized */

	/* If BORDER_TOP is not set, changing a title makes no sense */
	if( (pwin->flags & BORDER_TOP) == 0 )
		return FALSE;

	va_start(arg, fmt);
	vsprintf(tmp_text, fmt, arg);
	va_end(arg);

	/* If a title is there and did not change, just exit */
	if( pwin->title && !strcmp(pwin->title, tmp_text) )
		return TRUE;

	strncpy( pwin->title, tmp_text, screen_w );

	win_update(idx);
	return TRUE;
}

/************************************************************************
 *
 * Name : win_get_cx
 *
 * Entry: Window #
 *
 * Exit : Cursor X
 *
 * Description:
 *
 * Returns the cursor X coordinate for a window
 *
 ************************************************************************/

UINT32 win_get_cx(UINT32 idx)
{
	struct sWindow *pwin = &p_windows[idx];

	ASSERT(idx < MAX_WINDOWS);	/* This had better be in range */
	ASSERT(p_windows);			/* And this had better be initialized */

    return pwin->cx;
}

/************************************************************************
 *
 * Name : win_get_cy
 *
 * Entry: Window #
 *
 * Exit : Cursor Y
 *
 * Description:
 *
 * Returns the cursor Y coordinate for a window
 *
 ************************************************************************/

UINT32 win_get_cy(UINT32 idx)
{
	struct sWindow *pwin = &p_windows[idx];

	ASSERT(idx < MAX_WINDOWS);	/* This had better be in range */
	ASSERT(p_windows);			/* And this had better be initialized */

	return pwin->cy;
}

/************************************************************************
 *
 * Name : win_get_cx_abs
 *
 * Entry: Window #
 *
 * Exit : Cursor X
 *
 * Description:
 *
 * Returns the screen (absolute) Cursor X coordinate for a window
 *
 ************************************************************************/

UINT32 win_get_cx_abs(UINT32 idx)
{
	struct sWindow *pwin = &p_windows[idx];
	UINT32 x;

	ASSERT(idx < MAX_WINDOWS);	/* This had better be in range */
	ASSERT(p_windows);			/* And this had better be initialized */

	x = pwin->x + pwin->cx;
	if( pwin->flags & BORDER_LEFT )
		x++;

	return x;
}

/************************************************************************
 *
 * Name : win_get_cy_abs
 *
 * Entry: Window #
 *
 * Exit : Cursor Y
 *
 * Description:
 *
 * Returns the screen (absolute) Cursor Y coordinate for a window
 *
 ************************************************************************/

UINT32 win_get_cy_abs(UINT32 idx)
{
	struct sWindow *pwin = &p_windows[idx];
	UINT32 y;

	ASSERT(idx < MAX_WINDOWS);	/* This had better be in range */
	ASSERT(p_windows);			/* And this had better be initialized */

	y = pwin->y + pwin->cy;
	if( pwin->flags & BORDER_TOP )
		y++;

	return y;
}

/************************************************************************
 *
 * Name : win_get_x_abs
 *
 * Entry: Window #
 *
 * Exit : AbsX
 *
 * Description:
 *
 * Returns the screen (absolute) X coordinate for a window
 *
 ************************************************************************/

UINT32 win_get_x_abs(UINT32 idx)
{
	struct sWindow *pwin = &p_windows[idx];
	UINT32 x;

	ASSERT(idx < MAX_WINDOWS);	/* This had better be in range */
	ASSERT(p_windows);			/* And this had better be initialized */

	x = pwin->x;
	if( pwin->flags & BORDER_LEFT )
		x++;

	return x;
}

/************************************************************************
 *
 * Name : win_get_y_abs
 *
 * Entry: Window #
 *
 * Exit : AbsY
 *
 * Description:
 *
 * Returns the screen (absolute) Y coordinate for a window
 *
 ************************************************************************/

UINT32 win_get_y_abs(UINT32 idx)
{
	struct sWindow *pwin = &p_windows[idx];
	UINT32 y;

	ASSERT(idx < MAX_WINDOWS);	/* This had better be in range */
	ASSERT(p_windows);			/* And this had better be initialized */

	y = pwin->y;
	if( pwin->flags & BORDER_TOP )
		y++;

	return y;
}

/************************************************************************
 *
 * Name : win_get_w
 *
 * Entry: Window #
 *
 * Exit : Width
 *
 * Description:
 *
 * Returns the width of a window
 *
 ************************************************************************/

UINT32 win_get_w(UINT32 idx)
{
	struct sWindow *pwin = &p_windows[idx];

	ASSERT(idx < MAX_WINDOWS);	/* This had better be in range */
	ASSERT(p_windows);			/* And this had better be initialized */

	return pwin->w;
}

/************************************************************************
 *
 * Name : win_get_h
 *
 * Entry: Window #
 *
 * Exit : Height
 *
 * Description:
 *
 * Returns the height of a window
 *
 ************************************************************************/

UINT32 win_get_h(UINT32 idx)
{
	struct sWindow *pwin = &p_windows[idx];

	ASSERT(idx < MAX_WINDOWS);	/* This had better be in range */
	ASSERT(p_windows);			/* And this had better be initialized */

	return pwin->h;
}

/************************************************************************
 *
 * Name : win_set_w
 *
 * Entry: Window # and new width
 *
 * Exit : Nothing
 *
 * Description:
 *
 * Changes the width of a window
 *
 ************************************************************************/

void win_set_w(UINT32 idx, UINT32 w)
{
    struct sWindow *pwin = &p_windows[idx];

	ASSERT(idx < MAX_WINDOWS);	/* This had better be in range */
	ASSERT(p_windows);			/* And this had better be initialized */

    win_erase(idx);
    pwin->w = w;
    win_update_map();
    win_update(idx);
}

/************************************************************************
 *
 * Name : win_set_h
 *
 * Entry: Window # and new height
 *
 * Exit : Nothing
 *
 * Description:
 *
 * Changes the height of a window
 *
 ************************************************************************/

void win_set_h(UINT32 idx, UINT32 h)
{
    struct sWindow *pwin = &p_windows[idx];
    UINT32 yadd;
	ASSERT(idx < MAX_WINDOWS);	/* This had better be in range */
	ASSERT(p_windows);			/* And this had better be initialized */

    yadd = 0;

	if( pwin->flags & BORDER_TOP )
        ++yadd;
	if( pwin->flags & BORDER_BOTTOM )
        ++yadd;

    win_erase(idx);
    pwin->text = MyReAlloc(pwin->text, screen_w * (h + yadd), "win_set_h()");
    pwin->attr = MyReAlloc(pwin->attr, screen_w * (h + yadd), "win_set_h()");
    pwin->h = h;
    if( pwin->cy >= pwin->h )
        pwin->cy = pwin->h - 1;
    win_update_map();
    win_update(idx);
}

/************************************************************************
 *
 * Name : win_get_prio
 *
 * Entry: Window #
 *
 * Exit : Priority of window
 *
 * Description:
 *
 * Returns the current priority of a window
 *
 ************************************************************************/

UINT8 win_get_prio(UINT32 idx)
{
	struct sWindow *pwin = &p_windows[idx];

	ASSERT(idx < MAX_WINDOWS);	/* This had better be in range */
	ASSERT(p_windows);			/* And this had better be initialized */

	return pwin->prio;
}

/************************************************************************
 *
 * Name : win_set_prio
 *
 * Entry: Window # and new priority
 *
 * Exit : Nothing
 *
 * Description:
 *
 * Changes the priority of a window
 *
 ************************************************************************/

void win_set_prio(UINT32 idx, UINT8 prio)
{
	struct sWindow *pwin = &p_windows[idx];

	ASSERT(idx < MAX_WINDOWS);	/* This had better be in range */
	ASSERT(p_windows);			/* And this had better be initialized */

    if( pwin->prio == prio )
		return;

	win_erase(idx);
    pwin->prio = prio;
	win_update_map();
	win_update(idx);
}

/************************************************************************
 *
 * Name : win_move
 *
 * Entry: Window #, new x and y coordinates
 *
 * Exit :
 *
 * Description:
 *
 * Moves a window to a new position
 *
 ************************************************************************/

void win_move(UINT32 idx, UINT32 x, UINT32 y)
{
	struct sWindow *pwin = &p_windows[idx];

	ASSERT(idx < MAX_WINDOWS);	/* This had better be in range */
	ASSERT(p_windows);			/* And this had better be initialized */
    win_erase(idx);
	pwin->x = x;
	pwin->y = y;
	win_update_map();
    win_update(idx);
}

/************************************************************************
 *
 * Name : win_invalidate_video
 *
 * Entry: Nothing
 *
 * Exit : Nothing
 *
 * Description:
 *
 * Invalidates the video memory (eg. after a switch to graphics mode)
 *
 ************************************************************************/

void win_invalidate_video(void)
{

    if( p_text == NULL || p_attr == NULL )
		return;

	memset( p_text, 0xff, screen_w * screen_h );
	memset( p_attr, 0xff, screen_w * screen_h );
}

#endif
