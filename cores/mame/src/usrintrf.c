/*********************************************************************

	usrintrf.c

	Functions used to handle MAME's user interface.

*********************************************************************/

#include "driver.h"
#include "vidhrdw/vector.h"
#include "datafile.h"
#include <stdarg.h>
#include <stdio.h>
#include <math.h>
#include "ui_text.h"
#include "fileio.h"
#include "info.h"
#include "log.h"
#include <libretro.h>
#include <string/stdstring.h>

#define MAX_MESSAGE_LENGTH 2014

static char message_buffer[MAX_MESSAGE_LENGTH];
static char messagetext[MAX_MESSAGE_LENGTH];
static int  messagecounter;
static bool generate_DAT;           /* allows us to display a UI message before and while the DAT is generated */


/***************************************************************************

	Externals

***************************************************************************/

/* Variables for stat menu */
extern unsigned int dispensed_tickets;
extern unsigned int coins[COIN_COUNTERS];
extern unsigned int coinlockedout[COIN_COUNTERS];

/* MARTINEZ.F 990207 Memory Card */
int 		memcard_menu(struct mame_bitmap *bitmap, int);
extern int	mcd_action;
extern int	mcd_number;
extern int	memcard_status;
extern int	memcard_number;
extern int	memcard_manager;

extern int neogeo_memcard_load(int);
extern void neogeo_memcard_save(void);
extern void neogeo_memcard_eject(void);
extern int neogeo_memcard_create(int);
/* MARTINEZ.F 990207 Memory Card End */



/***************************************************************************

	Local variables

***************************************************************************/

static struct GfxElement *uirotfont;

/* raw coordinates, relative to the real scrbitmap */
static struct rectangle uirawbounds;
static int uirawcharwidth, uirawcharheight;

/* rotated coordinates, easier to deal with */
static struct rectangle uirotbounds;
static int uirotwidth, uirotheight;
int uirotcharwidth, uirotcharheight;

static int setup_selected;
static int setup_via_menu = 0;

UINT8 ui_dirty;



/***************************************************************************

	Font data

***************************************************************************/

static const UINT8 uifontdata[] =
{
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* [ 0- 1] */
	0x7c,0x80,0x98,0x90,0x80,0xbc,0x80,0x7c,0xf8,0x04,0x64,0x44,0x04,0xf4,0x04,0xf8,	/* [ 2- 3] tape pos 1 */
	0x7c,0x80,0x98,0x88,0x80,0xbc,0x80,0x7c,0xf8,0x04,0x64,0x24,0x04,0xf4,0x04,0xf8,	/* [ 4- 5] tape pos 2 */
	0x7c,0x80,0x88,0x98,0x80,0xbc,0x80,0x7c,0xf8,0x04,0x24,0x64,0x04,0xf4,0x04,0xf8,	/* [ 6- 7] tape pos 3 */
	0x7c,0x80,0x90,0x98,0x80,0xbc,0x80,0x7c,0xf8,0x04,0x44,0x64,0x04,0xf4,0x04,0xf8,	/* [ 8- 9] tape pos 3 */
	0x30,0x48,0x84,0xb4,0xb4,0x84,0x48,0x30,0x30,0x48,0x84,0x84,0x84,0x84,0x48,0x30,	/* [10-11] */
	0x00,0xfc,0x84,0x8c,0xd4,0xa4,0xfc,0x00,0x00,0xfc,0x84,0x84,0x84,0x84,0xfc,0x00,	/* [12-13] */
	0x00,0x38,0x7c,0x7c,0x7c,0x38,0x00,0x00,0x00,0x30,0x68,0x78,0x78,0x30,0x00,0x00,	/* [14-15] circle & bullet */
	0x80,0xc0,0xe0,0xf0,0xe0,0xc0,0x80,0x00,0x04,0x0c,0x1c,0x3c,0x1c,0x0c,0x04,0x00,	/* [16-17] R/L triangle */
	0x20,0x70,0xf8,0x20,0x20,0xf8,0x70,0x20,0x48,0x48,0x48,0x48,0x48,0x00,0x48,0x00,
	0x00,0x00,0x30,0x68,0x78,0x30,0x00,0x00,0x00,0x30,0x68,0x78,0x78,0x30,0x00,0x00,
	0x70,0xd8,0xe8,0xe8,0xf8,0xf8,0x70,0x00,0x1c,0x7c,0x74,0x44,0x44,0x4c,0xcc,0xc0,
	0x20,0x70,0xf8,0x70,0x70,0x70,0x70,0x00,0x70,0x70,0x70,0x70,0xf8,0x70,0x20,0x00,
	0x00,0x10,0xf8,0xfc,0xf8,0x10,0x00,0x00,0x00,0x20,0x7c,0xfc,0x7c,0x20,0x00,0x00,
	0xb0,0x54,0xb8,0xb8,0x54,0xb0,0x00,0x00,0x00,0x28,0x6c,0xfc,0x6c,0x28,0x00,0x00,
	0x00,0x30,0x30,0x78,0x78,0xfc,0x00,0x00,0xfc,0x78,0x78,0x30,0x30,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x20,0x20,0x20,0x20,0x00,0x20,0x00,
	0x50,0x50,0x50,0x00,0x00,0x00,0x00,0x00,0x00,0x50,0xf8,0x50,0xf8,0x50,0x00,0x00,
	0x20,0x70,0xc0,0x70,0x18,0xf0,0x20,0x00,0x40,0xa4,0x48,0x10,0x20,0x48,0x94,0x08,
	0x60,0x90,0xa0,0x40,0xa8,0x90,0x68,0x00,0x10,0x20,0x40,0x00,0x00,0x00,0x00,0x00,
	0x20,0x40,0x40,0x40,0x40,0x40,0x20,0x00,0x10,0x08,0x08,0x08,0x08,0x08,0x10,0x00,
	0x20,0xa8,0x70,0xf8,0x70,0xa8,0x20,0x00,0x00,0x20,0x20,0xf8,0x20,0x20,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x30,0x30,0x60,0x00,0x00,0x00,0xf8,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x30,0x30,0x00,0x00,0x08,0x10,0x20,0x40,0x80,0x00,0x00,
	0x70,0x88,0x88,0x88,0x88,0x88,0x70,0x00,0x10,0x30,0x10,0x10,0x10,0x10,0x10,0x00,
	0x70,0x88,0x08,0x10,0x20,0x40,0xf8,0x00,0x70,0x88,0x08,0x30,0x08,0x88,0x70,0x00,
	0x10,0x30,0x50,0x90,0xf8,0x10,0x10,0x00,0xf8,0x80,0xf0,0x08,0x08,0x88,0x70,0x00,
	0x70,0x80,0xf0,0x88,0x88,0x88,0x70,0x00,0xf8,0x08,0x08,0x10,0x20,0x20,0x20,0x00,
	0x70,0x88,0x88,0x70,0x88,0x88,0x70,0x00,0x70,0x88,0x88,0x88,0x78,0x08,0x70,0x00,
	0x00,0x00,0x30,0x30,0x00,0x30,0x30,0x00,0x00,0x00,0x30,0x30,0x00,0x30,0x30,0x60,
	0x10,0x20,0x40,0x80,0x40,0x20,0x10,0x00,0x00,0x00,0xf8,0x00,0xf8,0x00,0x00,0x00,
	0x40,0x20,0x10,0x08,0x10,0x20,0x40,0x00,0x70,0x88,0x08,0x10,0x20,0x00,0x20,0x00,
	0x30,0x48,0x94,0xa4,0xa4,0x94,0x48,0x30,0x70,0x88,0x88,0xf8,0x88,0x88,0x88,0x00,
	0xf0,0x88,0x88,0xf0,0x88,0x88,0xf0,0x00,0x70,0x88,0x80,0x80,0x80,0x88,0x70,0x00,
	0xf0,0x88,0x88,0x88,0x88,0x88,0xf0,0x00,0xf8,0x80,0x80,0xf0,0x80,0x80,0xf8,0x00,
	0xf8,0x80,0x80,0xf0,0x80,0x80,0x80,0x00,0x70,0x88,0x80,0x98,0x88,0x88,0x70,0x00,
	0x88,0x88,0x88,0xf8,0x88,0x88,0x88,0x00,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x00,
	0x08,0x08,0x08,0x08,0x88,0x88,0x70,0x00,0x88,0x90,0xa0,0xc0,0xa0,0x90,0x88,0x00,
	0x80,0x80,0x80,0x80,0x80,0x80,0xf8,0x00,0x88,0xd8,0xa8,0x88,0x88,0x88,0x88,0x00,
	0x88,0xc8,0xa8,0x98,0x88,0x88,0x88,0x00,0x70,0x88,0x88,0x88,0x88,0x88,0x70,0x00,
	0xf0,0x88,0x88,0xf0,0x80,0x80,0x80,0x00,0x70,0x88,0x88,0x88,0x88,0x88,0x70,0x08,
	0xf0,0x88,0x88,0xf0,0x88,0x88,0x88,0x00,0x70,0x88,0x80,0x70,0x08,0x88,0x70,0x00,
	0xf8,0x20,0x20,0x20,0x20,0x20,0x20,0x00,0x88,0x88,0x88,0x88,0x88,0x88,0x70,0x00,
	0x88,0x88,0x88,0x88,0x88,0x50,0x20,0x00,0x88,0x88,0x88,0x88,0xa8,0xd8,0x88,0x00,
	0x88,0x50,0x20,0x20,0x20,0x50,0x88,0x00,0x88,0x88,0x88,0x50,0x20,0x20,0x20,0x00,
	0xf8,0x08,0x10,0x20,0x40,0x80,0xf8,0x00,0x30,0x20,0x20,0x20,0x20,0x20,0x30,0x00,
	0x40,0x40,0x20,0x20,0x10,0x10,0x08,0x08,0x30,0x10,0x10,0x10,0x10,0x10,0x30,0x00,
	0x20,0x50,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xfc,
	0x40,0x20,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x70,0x08,0x78,0x88,0x78,0x00,
	0x80,0x80,0xf0,0x88,0x88,0x88,0xf0,0x00,0x00,0x00,0x70,0x88,0x80,0x80,0x78,0x00,
	0x08,0x08,0x78,0x88,0x88,0x88,0x78,0x00,0x00,0x00,0x70,0x88,0xf8,0x80,0x78,0x00,
	0x18,0x20,0x70,0x20,0x20,0x20,0x20,0x00,0x00,0x00,0x78,0x88,0x88,0x78,0x08,0x70,
	0x80,0x80,0xf0,0x88,0x88,0x88,0x88,0x00,0x20,0x00,0x20,0x20,0x20,0x20,0x20,0x00,
	0x20,0x00,0x20,0x20,0x20,0x20,0x20,0xc0,0x80,0x80,0x90,0xa0,0xe0,0x90,0x88,0x00,
	0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x00,0x00,0x00,0xf0,0xa8,0xa8,0xa8,0xa8,0x00,
	0x00,0x00,0xb0,0xc8,0x88,0x88,0x88,0x00,0x00,0x00,0x70,0x88,0x88,0x88,0x70,0x00,
	0x00,0x00,0xf0,0x88,0x88,0xf0,0x80,0x80,0x00,0x00,0x78,0x88,0x88,0x78,0x08,0x08,
	0x00,0x00,0xb0,0xc8,0x80,0x80,0x80,0x00,0x00,0x00,0x78,0x80,0x70,0x08,0xf0,0x00,
	0x20,0x20,0x70,0x20,0x20,0x20,0x18,0x00,0x00,0x00,0x88,0x88,0x88,0x98,0x68,0x00,
	0x00,0x00,0x88,0x88,0x88,0x50,0x20,0x00,0x00,0x00,0xa8,0xa8,0xa8,0xa8,0x50,0x00,
	0x00,0x00,0x88,0x50,0x20,0x50,0x88,0x00,0x00,0x00,0x88,0x88,0x88,0x78,0x08,0x70,
	0x00,0x00,0xf8,0x10,0x20,0x40,0xf8,0x00,0x08,0x10,0x10,0x20,0x10,0x10,0x08,0x00,
	0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x40,0x20,0x20,0x10,0x20,0x20,0x40,0x00,
	0x00,0x68,0xb0,0x00,0x00,0x00,0x00,0x00,0x20,0x50,0x20,0x50,0xa8,0x50,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x20,0x20,0x40,0x0c,0x10,0x38,0x10,0x20,0x20,0xc0,0x00,
	0x00,0x00,0x00,0x00,0x00,0x28,0x28,0x50,0x00,0x00,0x00,0x00,0x00,0x00,0xa8,0x00,
	0x70,0xa8,0xf8,0x20,0x20,0x20,0x20,0x00,0x70,0xa8,0xf8,0x20,0x20,0xf8,0xa8,0x70,
	0x20,0x50,0x88,0x00,0x00,0x00,0x00,0x00,0x44,0xa8,0x50,0x20,0x68,0xd4,0x28,0x00,
	0x88,0x70,0x88,0x60,0x30,0x88,0x70,0x00,0x00,0x10,0x20,0x40,0x20,0x10,0x00,0x00,
	0x78,0xa0,0xa0,0xb0,0xa0,0xa0,0x78,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x20,0x20,0x00,0x00,0x00,0x00,0x00,
	0x10,0x10,0x20,0x00,0x00,0x00,0x00,0x00,0x28,0x50,0x50,0x00,0x00,0x00,0x00,0x00,
	0x28,0x28,0x50,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x30,0x78,0x78,0x30,0x00,0x00,
	0x00,0x00,0x00,0x78,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xfc,0x00,0x00,0x00,0x00,
	0x68,0xb0,0x00,0x00,0x00,0x00,0x00,0x00,0xf4,0x5c,0x54,0x54,0x00,0x00,0x00,0x00,
	0x88,0x70,0x78,0x80,0x70,0x08,0xf0,0x00,0x00,0x40,0x20,0x10,0x20,0x40,0x00,0x00,
	0x00,0x00,0x70,0xa8,0xb8,0xa0,0x78,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x50,0x88,0x88,0x50,0x20,0x20,0x20,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x20,0x20,0x20,0x20,0x20,0x00,
	0x00,0x20,0x70,0xa8,0xa0,0xa8,0x70,0x20,0x30,0x48,0x40,0xe0,0x40,0x48,0xf0,0x00,
	0x00,0x48,0x30,0x48,0x48,0x30,0x48,0x00,0x88,0x88,0x50,0xf8,0x20,0xf8,0x20,0x00,
	0x20,0x20,0x20,0x00,0x20,0x20,0x20,0x00,0x78,0x80,0x70,0x88,0x70,0x08,0xf0,0x00,
	0xd8,0xd8,0x00,0x00,0x00,0x00,0x00,0x00,0x30,0x48,0x94,0xa4,0xa4,0x94,0x48,0x30,
	0x60,0x10,0x70,0x90,0x70,0x00,0x00,0x00,0x00,0x28,0x50,0xa0,0x50,0x28,0x00,0x00,
	0x00,0x00,0x00,0xf8,0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x78,0x00,0x00,0x00,0x00,
	0x30,0x48,0xb4,0xb4,0xa4,0xb4,0x48,0x30,0x7c,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x60,0x90,0x90,0x60,0x00,0x00,0x00,0x00,0x20,0x20,0xf8,0x20,0x20,0x00,0xf8,0x00,
	0x60,0x90,0x20,0x40,0xf0,0x00,0x00,0x00,0x60,0x90,0x20,0x90,0x60,0x00,0x00,0x00,
	0x10,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x88,0x88,0x88,0xc8,0xb0,0x80,
	0x78,0xd0,0xd0,0xd0,0x50,0x50,0x50,0x00,0x00,0x00,0x00,0x30,0x30,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x10,0x20,0x00,0x20,0x60,0x20,0x20,0x70,0x00,0x00,0x00,
	0x20,0x50,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0xa0,0x50,0x28,0x50,0xa0,0x00,0x00,
	0x40,0x48,0x50,0x28,0x58,0xa8,0x38,0x08,0x40,0x48,0x50,0x28,0x44,0x98,0x20,0x3c,
	0xc0,0x28,0xd0,0x28,0xd8,0xa8,0x38,0x08,0x20,0x00,0x20,0x40,0x80,0x88,0x70,0x00,
	0x40,0x20,0x70,0x88,0xf8,0x88,0x88,0x00,0x10,0x20,0x70,0x88,0xf8,0x88,0x88,0x00,
	0x70,0x00,0x70,0x88,0xf8,0x88,0x88,0x00,0x68,0xb0,0x70,0x88,0xf8,0x88,0x88,0x00,
	0x50,0x00,0x70,0x88,0xf8,0x88,0x88,0x00,0x20,0x50,0x70,0x88,0xf8,0x88,0x88,0x00,
	0x78,0xa0,0xa0,0xf0,0xa0,0xa0,0xb8,0x00,0x70,0x88,0x80,0x80,0x88,0x70,0x08,0x70,
	0x40,0x20,0xf8,0x80,0xf0,0x80,0xf8,0x00,0x10,0x20,0xf8,0x80,0xf0,0x80,0xf8,0x00,
	0x70,0x00,0xf8,0x80,0xf0,0x80,0xf8,0x00,0x50,0x00,0xf8,0x80,0xf0,0x80,0xf8,0x00,
	0x40,0x20,0x70,0x20,0x20,0x20,0x70,0x00,0x10,0x20,0x70,0x20,0x20,0x20,0x70,0x00,
	0x70,0x00,0x70,0x20,0x20,0x20,0x70,0x00,0x50,0x00,0x70,0x20,0x20,0x20,0x70,0x00,
	0x70,0x48,0x48,0xe8,0x48,0x48,0x70,0x00,0x68,0xb0,0x88,0xc8,0xa8,0x98,0x88,0x00,
	0x40,0x20,0x70,0x88,0x88,0x88,0x70,0x00,0x10,0x20,0x70,0x88,0x88,0x88,0x70,0x00,
	0x70,0x00,0x70,0x88,0x88,0x88,0x70,0x00,0x68,0xb0,0x70,0x88,0x88,0x88,0x70,0x00,
	0x50,0x00,0x70,0x88,0x88,0x88,0x70,0x00,0x00,0x88,0x50,0x20,0x50,0x88,0x00,0x00,
	0x00,0x74,0x88,0x90,0xa8,0x48,0xb0,0x00,0x40,0x20,0x88,0x88,0x88,0x88,0x70,0x00,
	0x10,0x20,0x88,0x88,0x88,0x88,0x70,0x00,0x70,0x00,0x88,0x88,0x88,0x88,0x70,0x00,
	0x50,0x00,0x88,0x88,0x88,0x88,0x70,0x00,0x10,0xa8,0x88,0x50,0x20,0x20,0x20,0x00,
	0x00,0x80,0xf0,0x88,0x88,0xf0,0x80,0x80,0x60,0x90,0x90,0xb0,0x88,0x88,0xb0,0x00,
	0x40,0x20,0x70,0x08,0x78,0x88,0x78,0x00,0x10,0x20,0x70,0x08,0x78,0x88,0x78,0x00,
	0x70,0x00,0x70,0x08,0x78,0x88,0x78,0x00,0x68,0xb0,0x70,0x08,0x78,0x88,0x78,0x00,
	0x50,0x00,0x70,0x08,0x78,0x88,0x78,0x00,0x20,0x50,0x70,0x08,0x78,0x88,0x78,0x00,
	0x00,0x00,0xf0,0x28,0x78,0xa0,0x78,0x00,0x00,0x00,0x70,0x88,0x80,0x78,0x08,0x70,
	0x40,0x20,0x70,0x88,0xf8,0x80,0x70,0x00,0x10,0x20,0x70,0x88,0xf8,0x80,0x70,0x00,
	0x70,0x00,0x70,0x88,0xf8,0x80,0x70,0x00,0x50,0x00,0x70,0x88,0xf8,0x80,0x70,0x00,
	0x40,0x20,0x00,0x60,0x20,0x20,0x70,0x00,0x10,0x20,0x00,0x60,0x20,0x20,0x70,0x00,
	0x20,0x50,0x00,0x60,0x20,0x20,0x70,0x00,0x50,0x00,0x00,0x60,0x20,0x20,0x70,0x00,
	0x50,0x60,0x10,0x78,0x88,0x88,0x70,0x00,0x68,0xb0,0x00,0xf0,0x88,0x88,0x88,0x00,
	0x40,0x20,0x00,0x70,0x88,0x88,0x70,0x00,0x10,0x20,0x00,0x70,0x88,0x88,0x70,0x00,
	0x20,0x50,0x00,0x70,0x88,0x88,0x70,0x00,0x68,0xb0,0x00,0x70,0x88,0x88,0x70,0x00,
	0x00,0x50,0x00,0x70,0x88,0x88,0x70,0x00,0x00,0x20,0x00,0xf8,0x00,0x20,0x00,0x00,
	0x00,0x00,0x68,0x90,0xa8,0x48,0xb0,0x00,0x40,0x20,0x88,0x88,0x88,0x98,0x68,0x00,
	0x10,0x20,0x88,0x88,0x88,0x98,0x68,0x00,0x70,0x00,0x88,0x88,0x88,0x98,0x68,0x00,
	0x50,0x00,0x88,0x88,0x88,0x98,0x68,0x00,0x10,0x20,0x88,0x88,0x88,0x78,0x08,0x70,
	0x80,0xf0,0x88,0x88,0xf0,0x80,0x80,0x80,0x50,0x00,0x88,0x88,0x88,0x78,0x08,0x70
};

static const struct GfxLayout uifontlayout =
{
	6,8,
	256,
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};




#if 0
#pragma mark UTILITIES
#endif


/*-------------------------------------------------
	Private Functions
-------------------------------------------------*/

static void wordwrap_text_buffer (char *buffer, int maxwidth);
void display_warn_callback(int param);
bool generate_warning_list(void);
void generate_gameinfo(void);

/*-------------------------------------------------
	ui_markdirty - mark a raw rectangle dirty
-------------------------------------------------*/

static INLINE void ui_markdirty(const struct rectangle *rect)
{
	artwork_mark_ui_dirty(rect->min_x, rect->min_y, rect->max_x, rect->max_y);
	ui_dirty = 5;
}



/*-------------------------------------------------
	ui_raw2rot_rect - convert a rect from raw
	coordinates to rotated coordinates
-------------------------------------------------*/

static void ui_raw2rot_rect(struct rectangle *rect)
{
	int temp, w, h;

	/* get the effective screen size, including artwork */
	artwork_get_screensize(&w, &h);

	/* apply X flip */
	if (Machine->ui_orientation & ORIENTATION_FLIP_X)
	{
		temp = w - rect->min_x - 1;
		rect->min_x = w - rect->max_x - 1;
		rect->max_x = temp;
	}

	/* apply Y flip */
	if (Machine->ui_orientation & ORIENTATION_FLIP_Y)
	{
		temp = h - rect->min_y - 1;
		rect->min_y = h - rect->max_y - 1;
		rect->max_y = temp;
	}

	/* apply X/Y swap first */
	if (Machine->ui_orientation & ORIENTATION_SWAP_XY)
	{
		temp = rect->min_x; rect->min_x = rect->min_y; rect->min_y = temp;
		temp = rect->max_x; rect->max_x = rect->max_y; rect->max_y = temp;
	}
}



/*-------------------------------------------------
	ui_rot2raw_rect - convert a rect from rotated
	coordinates to raw coordinates
-------------------------------------------------*/

static void ui_rot2raw_rect(struct rectangle *rect)
{
	int temp, w, h;

	/* get the effective screen size, including artwork */
	artwork_get_screensize(&w, &h);

	/* apply X/Y swap first */
	if (Machine->ui_orientation & ORIENTATION_SWAP_XY)
	{
		temp = rect->min_x; rect->min_x = rect->min_y; rect->min_y = temp;
		temp = rect->max_x; rect->max_x = rect->max_y; rect->max_y = temp;
	}

	/* apply X flip */
	if (Machine->ui_orientation & ORIENTATION_FLIP_X)
	{
		temp = w - rect->min_x - 1;
		rect->min_x = w - rect->max_x - 1;
		rect->max_x = temp;
	}

	/* apply Y flip */
	if (Machine->ui_orientation & ORIENTATION_FLIP_Y)
	{
		temp = h - rect->min_y - 1;
		rect->min_y = h - rect->max_y - 1;
		rect->max_y = temp;
	}
}



/*-------------------------------------------------
	set_ui_visarea - called by the OSD code to
	set the UI's domain
-------------------------------------------------*/

void set_ui_visarea(int xmin, int ymin, int xmax, int ymax)
{
	/* fill in the rect */
	uirawbounds.min_x = xmin;
	uirawbounds.min_y = ymin;
	uirawbounds.max_x = xmax;
	uirawbounds.max_y = ymax;

	/* orient it */
	uirotbounds = uirawbounds;
	ui_raw2rot_rect(&uirotbounds);

	/* make some easier-to-access globals */
	uirotwidth = uirotbounds.max_x - uirotbounds.min_x + 1;
	uirotheight = uirotbounds.max_y - uirotbounds.min_y + 1;

	/* remove me */
	Machine->uiwidth = uirotbounds.max_x - uirotbounds.min_x + 1;
	Machine->uiheight = uirotbounds.max_y - uirotbounds.min_y + 1;
	Machine->uixmin = uirotbounds.min_x;
	Machine->uiymin = uirotbounds.min_y;

	/* rebuild the font */
	builduifont();
}



/*-------------------------------------------------
	erase_screen - erase the screen
-------------------------------------------------*/

static void erase_screen(struct mame_bitmap *bitmap)
{
	fillbitmap(bitmap, get_black_pen(), NULL);
	schedule_full_refresh();
}



#if 0
#pragma mark -
#pragma mark FONTS & TEXT
#endif

/*-------------------------------------------------
	builduifont - build the user interface fonts
-------------------------------------------------*/

struct GfxElement *builduifont(void)
{
	struct GfxLayout layout = uifontlayout;
	UINT32 tempoffset[MAX_GFX_SIZE];
	struct GfxElement *font;
	int temp, i;

	/* free any existing fonts */
	if (Machine->uifont)
		freegfx(Machine->uifont);
	if (uirotfont)
		freegfx(uirotfont);

	/* first decode a straight on version for games */
	Machine->uifont = font = decodegfx(uifontdata, &layout);
	Machine->uifontwidth = layout.width;
	Machine->uifontheight = layout.height;

	/* pixel double horizontally */
	if (uirotwidth >= 420)
	{
		memcpy(tempoffset, layout.xoffset, sizeof(tempoffset));
		for (i = 0; i < layout.width; i++)
			layout.xoffset[i*2+0] = layout.xoffset[i*2+1] = tempoffset[i];
		layout.width *= 2;
	}

	/* pixel double vertically */
	if (uirotheight >= 420)
	{
		memcpy(tempoffset, layout.yoffset, sizeof(tempoffset));
		for (i = 0; i < layout.height; i++)
			layout.yoffset[i*2+0] = layout.yoffset[i*2+1] = tempoffset[i];
		layout.height *= 2;
	}

	/* apply swappage */
	if (Machine->ui_orientation & ORIENTATION_SWAP_XY)
	{
		memcpy(tempoffset, layout.xoffset, sizeof(tempoffset));
		memcpy(layout.xoffset, layout.yoffset, sizeof(layout.xoffset));
		memcpy(layout.yoffset, tempoffset, sizeof(layout.yoffset));

		temp = layout.width;
		layout.width = layout.height;
		layout.height = temp;
	}

	/* apply xflip */
	if (Machine->ui_orientation & ORIENTATION_FLIP_X)
	{
		memcpy(tempoffset, layout.xoffset, sizeof(tempoffset));
		for (i = 0; i < layout.width; i++)
			layout.xoffset[i] = tempoffset[layout.width - 1 - i];
	}

	/* apply yflip */
	if (Machine->ui_orientation & ORIENTATION_FLIP_Y)
	{
		memcpy(tempoffset, layout.yoffset, sizeof(tempoffset));
		for (i = 0; i < layout.height; i++)
			layout.yoffset[i] = tempoffset[layout.height - 1 - i];
	}

	/* decode rotated font */
	uirotfont = decodegfx(uifontdata, &layout);

	/* set the raw and rotated character width/height */
	uirawcharwidth = layout.width;
	uirawcharheight = layout.height;
	uirotcharwidth = (Machine->ui_orientation & ORIENTATION_SWAP_XY) ? layout.height : layout.width;
	uirotcharheight = (Machine->ui_orientation & ORIENTATION_SWAP_XY) ? layout.width : layout.height;

	/* set up the bogus colortable */
	if (font)
	{
		static pen_t colortable[2*2];

		/* colortable will be set at run time */
		font->colortable = colortable;
		font->total_colors = 2;
		uirotfont->colortable = colortable;
		uirotfont->total_colors = 2;
	}

	return font;
}



/*-------------------------------------------------
	ui_drawchar - draw a rotated character
-------------------------------------------------*/

void ui_drawchar(struct mame_bitmap *dest, int ch, int color, int sx, int sy)
{
	struct rectangle bounds;

	/* construct a rectangle in rotated coordinates, then transform it */
	bounds.min_x = sx + uirotbounds.min_x;
	bounds.min_y = sy + uirotbounds.min_y;
	bounds.max_x = bounds.min_x + uirotcharwidth - 1;
	bounds.max_y = bounds.min_y + uirotcharheight - 1;
	ui_rot2raw_rect(&bounds);

	/* now render */
	drawgfx(dest, uirotfont, ch, color, 0, 0, bounds.min_x, bounds.min_y, &uirawbounds, TRANSPARENCY_NONE, 0);

	/* mark dirty */
	ui_markdirty(&bounds);
}



/*-------------------------------------------------
	ui_text_ex - draw a string to the screen
-------------------------------------------------*/

static void ui_text_ex(struct mame_bitmap *bitmap, const char *buf_begin, const char *buf_end, int x, int y, int color)
{
	for ( ; buf_begin != buf_end; ++buf_begin)
	{
		ui_drawchar(bitmap, *buf_begin, color, x, y);
		x += uirotcharwidth;
	}
}



/*-------------------------------------------------
	ui_text_ex - draw a string to the screen
-------------------------------------------------*/

void ui_text(struct mame_bitmap *bitmap, const char *buf, int x, int y)
{
	ui_text_ex(bitmap, buf, buf + strlen(buf), x, y, UI_COLOR_NORMAL);
}



/*-------------------------------------------------
	displaytext - display a series of text lines
-------------------------------------------------*/

void displaytext(struct mame_bitmap *bitmap, const struct DisplayText *dt)
{
   /* loop until we run out of descriptors */
   for ( ; dt->text; dt++)
   {
      ui_text_ex(bitmap, dt->text, dt->text + strlen(dt->text), dt->x, dt->y, dt->color);
   }
}



/*-------------------------------------------------
	multiline_extract - extract one line from a
	multiline buffer; return the number of
	characters in the line; pbegin points to the
	start of the next line
-------------------------------------------------*/

static unsigned multiline_extract(const char **pbegin, const char *end, unsigned maxchars)
{
	const char *begin = *pbegin;
	unsigned numchars = 0;

	/* loop until we hit the end or max out */
	while (begin != end && numchars < maxchars)
	{
		/* if we hit an EOL, strip it and return the current count */
		if (*begin == '\n')
		{
			*pbegin = begin + 1; /* strip final space */
			return numchars;
		}

		/* if we hit a space, word wrap */
		else if (*begin == ' ')
		{
			/* find the end of this word */
			const char* word_end = begin + 1;
			while (word_end != end && *word_end != ' ' && *word_end != '\n')
				++word_end;

			/* if that pushes us past the max, truncate here */
			if (numchars + word_end - begin > maxchars)
			{
				/* if we have at least one character, strip the space */
				if (numchars)
				{
					*pbegin = begin + 1;
					return numchars;
				}

				/* otherwise, take as much as we can */
				else
				{
					*pbegin = begin + maxchars;
					return maxchars;
				}
			}

			/* advance to the end of this word */
			numchars += word_end - begin;
			begin = word_end;
		}

		/* for all other chars, just increment */
		else
		{
			++numchars;
			++begin;
		}
	}

	/* make sure we always make forward progress */
	if (begin != end && (*begin == '\n' || *begin == ' '))
		++begin;
	*pbegin = begin;
	return numchars;
}



/*-------------------------------------------------
	multiline_size - compute the output size of a
	multiline string
-------------------------------------------------*/

static void multiline_size(int *dx, int *dy, const char *begin, const char *end, unsigned maxchars)
{
	unsigned rows = 0;
	unsigned cols = 0;

	/* extract lines until the end, counting rows and tracking the max columns */
	while (begin != end)
	{
		unsigned len;
		len = multiline_extract(&begin, end, maxchars);
		if (len > cols)
			cols = len;
		++rows;
	}

	/* return the final result scaled by the char size */
	*dx = cols * uirotcharwidth;
	*dy = (rows - 1) * 3*uirotcharheight/2 + uirotcharheight;
}



/*-------------------------------------------------
	multilinebox_size - compute the output size of
	a multiline string with box
-------------------------------------------------*/

static void multilinebox_size(int *dx, int *dy, const char *begin, const char *end, unsigned maxchars)
{
	/* standard computation, plus an extra char width and height */
	multiline_size(dx, dy, begin, end, maxchars);
	*dx += uirotcharwidth;
	*dy += uirotcharheight;
}



/*-------------------------------------------------
	ui_multitext_ex - display a multiline string
-------------------------------------------------*/

static void ui_multitext_ex(struct mame_bitmap *bitmap, const char *begin, const char *end, unsigned maxchars, int x, int y, int color)
{
	/* extract lines until the end */
	while (begin != end)
	{
		const char *line_begin = begin;
		unsigned len = multiline_extract(&begin, end, maxchars);
		ui_text_ex(bitmap, line_begin, line_begin + len, x, y, color);
		y += 3*uirotcharheight/2;
	}
}



/*-------------------------------------------------
	ui_multitextbox_ex - display a multiline
	string with box
-------------------------------------------------*/

static void ui_multitextbox_ex(struct mame_bitmap *bitmap, const char *begin, const char *end, unsigned maxchars, int x, int y, int dx, int dy, int color)
{
	/* draw the box first */
	ui_drawbox(bitmap, x, y, dx, dy);

	/* indent by half a character */
	x += uirotcharwidth/2;
	y += uirotcharheight/2;

	/* draw the text */
	ui_multitext_ex(bitmap, begin, end, maxchars, x, y, color);
}



#if 0
#pragma mark -
#pragma mark BOXES & LINES
#endif

/*-------------------------------------------------
	ui_drawbox - draw a black box with white border
-------------------------------------------------*/

void ui_drawbox(struct mame_bitmap *bitmap, int leftx, int topy, int width, int height)
{
	struct rectangle bounds, tbounds;
	pen_t black, white;

	/* make a rect and clip it */
	bounds.min_x = uirotbounds.min_x + leftx;
	bounds.min_y = uirotbounds.min_y + topy;
	bounds.max_x = bounds.min_x + width - 1;
	bounds.max_y = bounds.min_y + height - 1;
	sect_rect(&bounds, &uirotbounds);

	/* pick colors from the colortable */
	black = uirotfont->colortable[0];
	white = uirotfont->colortable[1];

	/* top edge */
	tbounds = bounds;
	tbounds.max_y = tbounds.min_y;
	ui_rot2raw_rect(&tbounds);
	fillbitmap(bitmap, white, &tbounds);

	/* bottom edge */
	tbounds = bounds;
	tbounds.min_y = tbounds.max_y;
	ui_rot2raw_rect(&tbounds);
	fillbitmap(bitmap, white, &tbounds);

	/* left edge */
	tbounds = bounds;
	tbounds.max_x = tbounds.min_x;
	ui_rot2raw_rect(&tbounds);
	fillbitmap(bitmap, white, &tbounds);

	/* right edge */
	tbounds = bounds;
	tbounds.min_x = tbounds.max_x;
	ui_rot2raw_rect(&tbounds);
	fillbitmap(bitmap, white, &tbounds);

	/* fill in the middle with black */
	tbounds = bounds;
	tbounds.min_x++;
	tbounds.min_y++;
	tbounds.max_x--;
	tbounds.max_y--;
	ui_rot2raw_rect(&tbounds);
	fillbitmap(bitmap, black, &tbounds);

	/* mark things dirty */
	ui_rot2raw_rect(&bounds);
	ui_markdirty(&bounds);
}


#if 0
#pragma mark -
#pragma mark BOXES & LINES
#endif

void ui_displaymenu(struct mame_bitmap *bitmap,const char **items,const char **subitems,char *flag,int selected,int arrowize_subitem)
{
	struct DisplayText dt[256];
	int curr_dt;
	const char *lefthilight = ui_getstring (UI_lefthilight);
	const char *righthilight = ui_getstring (UI_righthilight);
	const char *uparrow = ui_getstring (UI_uparrow);
	const char *downarrow = ui_getstring (UI_downarrow);
	const char *leftarrow = ui_getstring (UI_leftarrow);
	const char *rightarrow = ui_getstring (UI_rightarrow);
	int i,count,len,maxlen,highlen;
	int leftoffs,topoffs,visible,topitem;
	int selected_long;


	i = 0;
	maxlen = 0;
	highlen = uirotwidth / uirotcharwidth;
	while (items[i])
	{
		len = 3 + strlen(items[i]);
		if (subitems && subitems[i])
			len += 2 + strlen(subitems[i]);
		if (len > maxlen && len <= highlen)
			maxlen = len;
		i++;
	}
	count = i;

	visible = uirotheight / (3 * uirotcharheight / 2) - 1;
	topitem = 0;
	if (visible > count) visible = count;
	else
	{
		topitem = selected - visible / 2;
		if (topitem < 0) topitem = 0;
		if (topitem > count - visible) topitem = count - visible;
	}

	leftoffs = (uirotwidth - maxlen * uirotcharwidth) / 2;
	topoffs = (uirotheight - (3 * visible + 1) * uirotcharheight / 2) / 2;

	/* black background */
	ui_drawbox(bitmap,leftoffs,topoffs,maxlen * uirotcharwidth,(3 * visible + 1) * uirotcharheight / 2);

	selected_long = 0;
	curr_dt = 0;
	for (i = 0;i < visible;i++)
	{
		int item = i + topitem;

		if (i == 0 && item > 0)
		{
			dt[curr_dt].text = uparrow;
			dt[curr_dt].color = UI_COLOR_NORMAL;
			dt[curr_dt].x = (uirotwidth - uirotcharwidth * strlen(uparrow)) / 2;
			dt[curr_dt].y = topoffs + (3*i+1)*uirotcharheight/2;
			curr_dt++;
		}
		else if (i == visible - 1 && item < count - 1)
		{
			dt[curr_dt].text = downarrow;
			dt[curr_dt].color = UI_COLOR_NORMAL;
			dt[curr_dt].x = (uirotwidth - uirotcharwidth * strlen(downarrow)) / 2;
			dt[curr_dt].y = topoffs + (3*i+1)*uirotcharheight/2;
			curr_dt++;
		}
		else
		{
			if (subitems && subitems[item])
			{
				int sublen;
				len = strlen(items[item]);
				dt[curr_dt].text = items[item];
				dt[curr_dt].color = UI_COLOR_NORMAL;
				dt[curr_dt].x = leftoffs + 3*uirotcharwidth/2;
				dt[curr_dt].y = topoffs + (3*i+1)*uirotcharheight/2;
				curr_dt++;
				sublen = strlen(subitems[item]);
				if (sublen > maxlen-5-len)
				{
					dt[curr_dt].text = "...";
					sublen = strlen(dt[curr_dt].text);
					if (item == selected)
						selected_long = 1;
				} else {
					dt[curr_dt].text = subitems[item];
				}
				/* If this item is flagged, draw it in inverse print */
				dt[curr_dt].color = (flag && flag[item]) ? UI_COLOR_INVERSE : UI_COLOR_NORMAL;
				dt[curr_dt].x = leftoffs + uirotcharwidth * (maxlen-1-sublen) - uirotcharwidth/2;
				dt[curr_dt].y = topoffs + (3*i+1)*uirotcharheight/2;
				curr_dt++;
			}
			else
			{
				dt[curr_dt].text = items[item];
				dt[curr_dt].color = UI_COLOR_NORMAL;
				dt[curr_dt].x = (uirotwidth - uirotcharwidth * strlen(items[item])) / 2;
				dt[curr_dt].y = topoffs + (3*i+1)*uirotcharheight/2;
				curr_dt++;
			}
		}
	}

	i = selected - topitem;
	if (subitems && subitems[selected] && arrowize_subitem)
	{
		if (arrowize_subitem & 1)
		{
			int sublen;

			len = strlen(items[selected]);

			dt[curr_dt].text = leftarrow;
			dt[curr_dt].color = UI_COLOR_NORMAL;

			sublen = strlen(subitems[selected]);
			if (sublen > maxlen-5-len)
				sublen = strlen("...");

			dt[curr_dt].x = leftoffs + uirotcharwidth * (maxlen-2 - sublen) - uirotcharwidth/2 - 1;
			dt[curr_dt].y = topoffs + (3*i+1)*uirotcharheight/2;
			curr_dt++;
		}
		if (arrowize_subitem & 2)
		{
			dt[curr_dt].text = rightarrow;
			dt[curr_dt].color = UI_COLOR_NORMAL;
			dt[curr_dt].x = leftoffs + uirotcharwidth * (maxlen-1) - uirotcharwidth/2;
			dt[curr_dt].y = topoffs + (3*i+1)*uirotcharheight/2;
			curr_dt++;
		}
	}
	else
	{
		dt[curr_dt].text = righthilight;
		dt[curr_dt].color = UI_COLOR_NORMAL;
		dt[curr_dt].x = leftoffs + uirotcharwidth * (maxlen-1) - uirotcharwidth/2;
		dt[curr_dt].y = topoffs + (3*i+1)*uirotcharheight/2;
		curr_dt++;
	}
	dt[curr_dt].text = lefthilight;
	dt[curr_dt].color = UI_COLOR_NORMAL;
	dt[curr_dt].x = leftoffs + uirotcharwidth/2;
	dt[curr_dt].y = topoffs + (3*i+1)*uirotcharheight/2;
	curr_dt++;

	dt[curr_dt].text = 0;	/* terminate array */

	displaytext(bitmap,dt);

	if (selected_long)
	{
		int long_dx;
		int long_dy;
		int long_x;
		int long_y;
		unsigned long_max;

		long_max = (uirotwidth / uirotcharwidth) - 2;
		multilinebox_size(&long_dx,&long_dy,subitems[selected],subitems[selected] + strlen(subitems[selected]), long_max);

		long_x = uirotwidth - long_dx;
		long_y = topoffs + (i+1) * 3*uirotcharheight/2;

		/* if too low display up */
		if (long_y + long_dy > uirotheight)
			long_y = topoffs + i * 3*uirotcharheight/2 - long_dy;

		ui_multitextbox_ex(bitmap,subitems[selected],subitems[selected] + strlen(subitems[selected]), long_max, long_x,long_y,long_dx,long_dy, UI_COLOR_NORMAL);
	}
}


void ui_displaymessagewindow(struct mame_bitmap *bitmap,const char *text)
{
	struct DisplayText dt[256];
	int curr_dt;
	char *c,*c2;
	int i,len,maxlen,lines;
	char textcopy[2048];
	int leftoffs,topoffs;
	int maxcols,maxrows;

	maxcols = (uirotwidth / uirotcharwidth) - 1;
	maxrows = (2 * uirotheight - uirotcharheight) / (3 * uirotcharheight);

	/* copy text, calculate max len, count lines, wrap long lines and crop height to fit */
	maxlen = 0;
	lines = 0;
	c = (char *)text;
	c2 = textcopy;
	while (*c)
	{
		len = 0;
		while (*c && *c != '\n')
		{
			*c2++ = *c++;
			len++;
			if (len == maxcols && *c != '\n')
			{
				/* attempt word wrap */
				char *csave = c, *c2save = c2;
				int lensave = len;

				/* back up to last space or beginning of line */
				while (*c != ' ' && *c != '\n' && c > text)
					--c, --c2, --len;

				/* if no space was found, hard wrap instead */
				if (*c != ' ')
					c = csave, c2 = c2save, len = lensave;
				else
					c++;

				*c2++ = '\n'; /* insert wrap */
				break;
			}
		}

		if (*c == '\n')
			*c2++ = *c++;

		if (len > maxlen) maxlen = len;

		lines++;
		if (lines == maxrows)
			break;
	}
	*c2 = '\0';

	maxlen += 1;

	leftoffs = (uirotwidth - uirotcharwidth * maxlen) / 2;
	if (leftoffs < 0) leftoffs = 0;
	topoffs = (uirotheight - (3 * lines + 1) * uirotcharheight / 2) / 2;

	/* black background */
	ui_drawbox(bitmap,leftoffs,topoffs,maxlen * uirotcharwidth,(3 * lines + 1) * uirotcharheight / 2);

	curr_dt = 0;
	c = textcopy;
	i = 0;
	while (*c)
	{
		c2 = c;
		while (*c && *c != '\n')
			c++;

		if (*c == '\n')
		{
			*c = '\0';
			c++;
		}

		if (*c2 == '\t')    /* center text */
		{
			c2++;
			dt[curr_dt].x = (uirotwidth - uirotcharwidth * (c - c2)) / 2;
		}
		else
			dt[curr_dt].x = leftoffs + uirotcharwidth/2;

		dt[curr_dt].text = c2;
		dt[curr_dt].color = UI_COLOR_NORMAL;
		dt[curr_dt].y = topoffs + (3*i+1)*uirotcharheight/2;
		curr_dt++;

		i++;
	}

	dt[curr_dt].text = 0;	/* terminate array */

	displaytext(bitmap,dt);
}



static void showcharset(struct mame_bitmap *bitmap)
{
	int i;
	char buf[80];
	int mode,bank,color,firstdrawn;
	int palpage;
/*	int changed = 1;*/
	int total_colors = 0;
	pen_t *colortable = NULL;
	int cpx=0,cpy,skip_chars=0,skip_tmap=0;
	int tilemap_xpos = 0;
	int tilemap_ypos = 0;

	mode = 0;
	bank = 0;
	color = 0;
	firstdrawn = 0;
	palpage = 0;

	do
	{
		static const struct rectangle fullrect = { 0, 10000, 0, 10000 };

		/* mark the whole thing dirty */
		ui_markdirty(&fullrect);

		switch (mode)
		{
			case 0: /* palette or clut */
			{
				if (bank == 0)	/* palette */
				{
					total_colors = Machine->drv->total_colors;
					colortable = Machine->pens;
					strcpy(buf,"PALETTE");
				}
				else if (bank == 1)	/* clut */
				{
					total_colors = Machine->drv->color_table_len;
					colortable = Machine->remapped_colortable;
					strcpy(buf,"CLUT");
				}
				else
				{
					buf[0] = 0;
					total_colors = 0;
					colortable = 0;
				}

				/*if (changed) -- temporary */
				{
					erase_screen(bitmap);

					if (total_colors)
					{
						int sx,sy,colors;
						int column_heading_max;
						struct bounds;

						colors = total_colors - 256 * palpage;
						if (colors > 256) colors = 256;

						/* min(colors, 16) */
						if (colors < 16)
							column_heading_max = colors;
						else
							column_heading_max = 16;

						for (i = 0;i < column_heading_max;i++)
						{
							char bf[40];

							sx = 3*uirotcharwidth + (uirotcharwidth*4/3)*(i % 16);
							sprintf(bf,"%X",i);
							ui_text(bitmap,bf,sx,2*uirotcharheight);
							if (16*i < colors)
							{
								sy = 3*uirotcharheight + (uirotcharheight)*(i % 16);
								sprintf(bf,"%3X",i+16*palpage);
								ui_text(bitmap,bf,0,sy);
							}
						}

						for (i = 0;i < colors;i++)
						{
							struct rectangle bounds;
							bounds.min_x = uirotbounds.min_x + 3*uirotcharwidth + (uirotcharwidth*4/3)*(i % 16);
							bounds.min_y = uirotbounds.min_y + 2*uirotcharheight + (uirotcharheight)*(i / 16) + uirotcharheight;
							bounds.max_x = bounds.min_x + uirotcharwidth*4/3 - 1;
							bounds.max_y = bounds.min_y + uirotcharheight - 1;
							ui_rot2raw_rect(&bounds);
							fillbitmap(bitmap, colortable[i + 256*palpage], &bounds);
						}
					}
					else
						ui_text(bitmap,"N/A",3*uirotcharwidth,2*uirotcharheight);

					ui_text(bitmap,buf,0,0);
					/*changed = 0;*/
				}

				break;
			}
			case 1: /* characters */
			{
				int crotwidth = (Machine->ui_orientation & ORIENTATION_SWAP_XY) ? Machine->gfx[bank]->height : Machine->gfx[bank]->width;
				int crotheight = (Machine->ui_orientation & ORIENTATION_SWAP_XY) ? Machine->gfx[bank]->width : Machine->gfx[bank]->height;
				cpx = uirotwidth / crotwidth;
				if (cpx == 0) cpx = 1;
				cpy = (uirotheight - uirotcharheight) / crotheight;
				if (cpy == 0) cpy = 1;
				skip_chars = cpx * cpy;
				/*if (changed) -- temporary */
				{
					int flipx,flipy;
					int lastdrawn=0;

					erase_screen(bitmap);

					/* validity check after char bank change */
					if (firstdrawn >= Machine->gfx[bank]->total_elements)
					{
						firstdrawn = Machine->gfx[bank]->total_elements - skip_chars;
						if (firstdrawn < 0) firstdrawn = 0;
					}

					flipx = 0;
					flipy = 0;

					for (i = 0; i+firstdrawn < Machine->gfx[bank]->total_elements && i<cpx*cpy; i++)
					{
						struct rectangle bounds;
						bounds.min_x = (i % cpx) * crotwidth + uirotbounds.min_x;
						bounds.min_y = uirotcharheight + (i / cpx) * crotheight + uirotbounds.min_y;
						bounds.max_x = bounds.min_x + crotwidth - 1;
						bounds.max_y = bounds.min_y + crotheight - 1;
						ui_rot2raw_rect(&bounds);

						drawgfx(bitmap,Machine->gfx[bank],
								i+firstdrawn,color,  /*sprite num, color*/
								flipx,flipy,bounds.min_x,bounds.min_y,
								0,Machine->gfx[bank]->colortable ? TRANSPARENCY_NONE : TRANSPARENCY_NONE_RAW,0);

						lastdrawn = i+firstdrawn;
					}

					sprintf(buf,"GFXSET %d COLOR %2X CODE %X-%X",bank,color,firstdrawn,lastdrawn);
					ui_text(bitmap,buf,0,0);
					/*changed = 0;*/
				}

				break;
			}
			case 2: /* Tilemaps */
			{
				/*if (changed) -- temporary */
				{
					UINT32 tilemap_width, tilemap_height;
					tilemap_nb_size (bank, &tilemap_width, &tilemap_height);
					while (tilemap_xpos < 0)
						tilemap_xpos += tilemap_width;
					tilemap_xpos %= tilemap_width;

					while (tilemap_ypos < 0)
						tilemap_ypos += tilemap_height;
					tilemap_ypos %= tilemap_height;

					erase_screen(bitmap);
					tilemap_nb_draw (bitmap, bank, tilemap_xpos, tilemap_ypos);
					sprintf(buf, "TILEMAP %d (%dx%d)  X:%d  Y:%d", bank, tilemap_width, tilemap_height, tilemap_xpos, tilemap_ypos);
					ui_text(bitmap,buf,0,0);
					/*changed = 0;*/
					skip_tmap = 0;
				}
				break;
			}
		}

		update_video_and_audio();

		if (code_pressed(KEYCODE_LCONTROL) || code_pressed(KEYCODE_RCONTROL))
		{
			skip_chars = cpx;
			skip_tmap = 8;
		}
		if (code_pressed(KEYCODE_LSHIFT) || code_pressed(KEYCODE_RSHIFT))
		{
			skip_chars = 1;
			skip_tmap = 1;
		}


		if (input_ui_pressed_repeat(IPT_UI_RIGHT,8))
		{
			int next_bank, next_mode;
			int jumped;

			next_mode = mode;
			next_bank = bank+1;
			do {
				jumped = 0;
				switch (next_mode)
				{
					case 0:
						if (next_bank == 2 || Machine->drv->color_table_len == 0)
						{
							jumped = 1;
							next_mode++;
							next_bank = 0;
						}
						break;
					case 1:
						if (next_bank == MAX_GFX_ELEMENTS || !Machine->gfx[next_bank])
						{
							jumped = 1;
							next_mode++;
							next_bank = 0;
						}
						break;
					case 2:
						if (next_bank == tilemap_count())
							next_mode = -1;
						break;
				}
			}	while (jumped);
			if (next_mode != -1 )
			{
				bank = next_bank;
				mode = next_mode;
				/*firstdrawn = 0;*/
				/*changed = 1;*/
			}
		}

		if (input_ui_pressed_repeat(IPT_UI_LEFT,8))
		{
			int next_bank, next_mode;

			next_mode = mode;
			next_bank = bank-1;
			while(next_bank < 0 && next_mode >= 0)
			{
				next_mode = next_mode - 1;
				switch (next_mode)
				{
					case 0:
						if (Machine->drv->color_table_len == 0)
							next_bank = 0;
						else
							next_bank = 1;
						break;
					case 1:
						next_bank = MAX_GFX_ELEMENTS-1;
						while (next_bank >= 0 && !Machine->gfx[next_bank])
							next_bank--;
						break;
					case 2:
						next_bank = tilemap_count() - 1;
						break;
				}
			}
			if (next_mode != -1 )
			{
				bank = next_bank;
				mode = next_mode;
				/*firstdrawn = 0;*/
				/*changed = 1;*/
			}
		}

		if (code_pressed_memory_repeat(KEYCODE_PGDN,4))
		{
			switch (mode)
			{
				case 0:
				{
					if (256 * (palpage + 1) < total_colors)
					{
						palpage++;
						/*changed = 1;*/
					}
					break;
				}
				case 1:
				{
					if (firstdrawn + skip_chars < Machine->gfx[bank]->total_elements)
					{
						firstdrawn += skip_chars;
						/*changed = 1;*/
					}
					break;
				}
				case 2:
				{
					if (skip_tmap)
						tilemap_ypos -= skip_tmap;
					else
						tilemap_ypos -= bitmap->height/4;
					/*changed = 1;*/
					break;
				}
			}
		}

		if (code_pressed_memory_repeat(KEYCODE_PGUP,4))
		{
			switch (mode)
			{
				case 0:
				{
					if (palpage > 0)
					{
						palpage--;
						/*changed = 1;*/
					}
					break;
				}
				case 1:
				{
					firstdrawn -= skip_chars;
					if (firstdrawn < 0) firstdrawn = 0;
					/*changed = 1;*/
					break;
				}
				case 2:
				{
					if (skip_tmap)
						tilemap_ypos += skip_tmap;
					else
						tilemap_ypos += bitmap->height/4;
					/*changed = 1;*/
					break;
				}
			}
		}

		if (code_pressed_memory_repeat(KEYCODE_D,4))
		{
			switch (mode)
			{
				case 2:
				{
					if (skip_tmap)
						tilemap_xpos -= skip_tmap;
					else
						tilemap_xpos -= bitmap->width/4;
					/*changed = 1;*/
					break;
				}
			}
		}

		if (code_pressed_memory_repeat(KEYCODE_G,4))
		{
			switch (mode)
			{
				case 2:
				{
					if (skip_tmap)
						tilemap_xpos += skip_tmap;
					else
						tilemap_xpos += bitmap->width/4;
					/*changed = 1;*/
					break;
				}
			}
		}

		if (input_ui_pressed_repeat(IPT_UI_UP,6))
		{
			switch (mode)
			{
				case 1:
				{
					if (color < Machine->gfx[bank]->total_colors - 1)
					{
						color++;
						/*changed = 1;*/
					}
					break;
				}
			}
		}

		if (input_ui_pressed_repeat(IPT_UI_DOWN,6))
		{
			switch (mode)
			{
				case 0:
					break;
				case 1:
				{
					if (color > 0)
					{
						color--;
						/*changed = 1;*/
					}
				}
			}
		}
	} while (!input_ui_pressed(IPT_UI_SHOW_GFX) &&
			!input_ui_pressed(IPT_UI_CANCEL));

	schedule_full_refresh();
}



static int switchmenu(struct mame_bitmap *bitmap, int selected, UINT32 switch_name, UINT32 switch_setting)
{
	const char *menu_item[128];
	const char *menu_subitem[128];
	struct InputPort *entry[128];
	char flag[40];
	int i,sel;
	struct InputPort *in;
	int total;
	int arrowize;


	sel = selected - 1;


	in = Machine->input_ports;

	total = 0;
	while (in->type != IPT_END)
	{
		if ((in->type & ~IPF_MASK) == switch_name && input_port_name(in) != 0 &&
				(in->type & IPF_UNUSED) == 0 &&
				!(!options.cheat_input_ports && (in->type & IPF_CHEAT)))
		{
			entry[total] = in;
			menu_item[total] = input_port_name(in);

			total++;
		}

		in++;
	}

	if (total == 0) return 0;

	menu_item[total] = ui_getstring (UI_returntomain);
	menu_item[total + 1] = 0;	/* terminate array */
	total++;


	for (i = 0;i < total;i++)
	{
		flag[i] = 0; /* TODO: flag the dip if it's not the real default */
		if (i < total - 1)
		{
			in = entry[i] + 1;
			while ((in->type & ~IPF_MASK) == switch_setting &&
					in->default_value != entry[i]->default_value)
				in++;

			if ((in->type & ~IPF_MASK) != switch_setting)
				menu_subitem[i] = ui_getstring (UI_INVALID);
			else menu_subitem[i] = input_port_name(in);
		}
		else menu_subitem[i] = 0;	/* no subitem */
	}

	arrowize = 0;
	if (sel < total - 1)
	{
		in = entry[sel] + 1;
		while ((in->type & ~IPF_MASK) == switch_setting &&
				in->default_value != entry[sel]->default_value)
			in++;

		if ((in->type & ~IPF_MASK) != switch_setting)
			/* invalid setting: revert to a valid one */
			arrowize |= 1;
		else
		{
			if (((in-1)->type & ~IPF_MASK) == switch_setting &&
					!(!options.cheat_input_ports && ((in-1)->type & IPF_CHEAT)))
				arrowize |= 1;
		}
	}
	if (sel < total - 1)
	{
		in = entry[sel] + 1;
		while ((in->type & ~IPF_MASK) == switch_setting &&
				in->default_value != entry[sel]->default_value)
			in++;

		if ((in->type & ~IPF_MASK) != switch_setting)
			/* invalid setting: revert to a valid one */
			arrowize |= 2;
		else
		{
			if (((in+1)->type & ~IPF_MASK) == switch_setting &&
					!(!options.cheat_input_ports && ((in+1)->type & IPF_CHEAT)))
				arrowize |= 2;
		}
	}

	ui_displaymenu(bitmap,menu_item,menu_subitem,flag,sel,arrowize);

	if (input_ui_pressed_repeat(IPT_UI_DOWN,8))
		sel = (sel + 1) % total;

	if (input_ui_pressed_repeat(IPT_UI_UP,8))
		sel = (sel + total - 1) % total;

	if (input_ui_pressed_repeat(IPT_UI_RIGHT,8))
	{
		if (sel < total - 1)
		{
			in = entry[sel] + 1;
			while ((in->type & ~IPF_MASK) == switch_setting &&
					in->default_value != entry[sel]->default_value)
				in++;

			if ((in->type & ~IPF_MASK) != switch_setting)
				/* invalid setting: revert to a valid one */
				entry[sel]->default_value = (entry[sel]+1)->default_value & entry[sel]->mask;
			else
			{
				if (((in+1)->type & ~IPF_MASK) == switch_setting &&
						!(!options.cheat_input_ports && ((in+1)->type & IPF_CHEAT)))
					entry[sel]->default_value = (in+1)->default_value & entry[sel]->mask;
			}

			/* tell updatescreen() to clean after us (in case the window changes size) */
			schedule_full_refresh();
		}
	}

	if (input_ui_pressed_repeat(IPT_UI_LEFT,8))
	{
		if (sel < total - 1)
		{
			in = entry[sel] + 1;
			while ((in->type & ~IPF_MASK) == switch_setting &&
					in->default_value != entry[sel]->default_value)
				in++;

			if ((in->type & ~IPF_MASK) != switch_setting)
				/* invalid setting: revert to a valid one */
				entry[sel]->default_value = (entry[sel]+1)->default_value & entry[sel]->mask;
			else
			{
				if (((in-1)->type & ~IPF_MASK) == switch_setting &&
						!(!options.cheat_input_ports && ((in-1)->type & IPF_CHEAT)))
					entry[sel]->default_value = (in-1)->default_value & entry[sel]->mask;
			}

			/* tell updatescreen() to clean after us (in case the window changes size) */
			schedule_full_refresh();
		}
	}

	if (input_ui_pressed(IPT_UI_SELECT))
	{
		if (sel == total - 1) sel = -1;
	}

	if (input_ui_pressed(IPT_UI_CANCEL))
		sel = -1;

	if (input_ui_pressed(IPT_UI_CONFIGURE))
		sel = -2;

	if (sel == -1 || sel == -2)
	{
		schedule_full_refresh();
	}

	return sel + 1;
}



static int setdipswitches(struct mame_bitmap *bitmap, int selected)
{
	return switchmenu(bitmap, selected, IPT_DIPSWITCH_NAME, IPT_DIPSWITCH_SETTING);
}

/* This flag is used for record OR sequence of key/joy */
/* when is !=0 the first sequence is record, otherwise the first free */
/* it's used byt setdefkeysettings, setdefjoysettings, setkeysettings, setjoysettings */
static int record_first_insert = 1;

static char menu_subitem_buffer[500][96];

static int setdefcodesettings(struct mame_bitmap *bitmap,int selected)
{
	const char *menu_item[500];
	const char *menu_subitem[500];
	struct ipd *entry[500];
	char flag[500];
	int i,sel;
	struct ipd *in;
	int total;
	extern struct ipd inputport_defaults[];

	sel = selected - 1;


	if (Machine->input_ports == 0)
		return 0;

	in = inputport_defaults;

	total = 0;
	while (in->type != IPT_END)
	{
		if (in->name != 0  && (in->type & ~IPF_MASK) != IPT_UNKNOWN && (in->type & ~IPF_MASK) != IPT_OSD_DESCRIPTION && (in->type & IPF_UNUSED) == 0
			&& !(!options.cheat_input_ports && (in->type & IPF_CHEAT)))
		{
			entry[total] = in;
			menu_item[total] = in->name;

			total++;
		}

		in++;
	}

	if (total == 0) return 0;

	menu_item[total] = ui_getstring (UI_returntomain);
	menu_item[total + 1] = 0;	/* terminate array */
	total++;

	for (i = 0;i < total;i++)
	{
		if (i < total - 1)
		{
			seq_name(&entry[i]->seq,menu_subitem_buffer[i],sizeof(menu_subitem_buffer[0]));
			menu_subitem[i] = menu_subitem_buffer[i];
		} else
			menu_subitem[i] = 0;	/* no subitem */
		flag[i] = 0;
	}

	if (sel > SEL_MASK)   /* are we waiting for a new key? */
	{
		int ret;

		menu_subitem[sel & SEL_MASK] = "    ";
		ui_displaymenu(bitmap,menu_item,menu_subitem,flag,sel & SEL_MASK,3);

		ret = seq_read_async(&entry[sel & SEL_MASK]->seq,record_first_insert);

		if (ret >= 0)
		{
			sel &= SEL_MASK;

			if (ret > 0 || seq_get_1(&entry[sel]->seq) == CODE_NONE)
			{
				seq_set_1(&entry[sel]->seq,CODE_NONE);
				ret = 1;
			}

			/* tell updatescreen() to clean after us (in case the window changes size) */
			schedule_full_refresh();

			record_first_insert = ret != 0;
		}

		init_analog_seq();

		return sel + 1;
	}


	ui_displaymenu(bitmap,menu_item,menu_subitem,flag,sel,0);

	if (input_ui_pressed_repeat(IPT_UI_DOWN,8))
	{
		sel = (sel + 1) % total;
		record_first_insert = 1;
	}

	if (input_ui_pressed_repeat(IPT_UI_UP,8))
	{
		sel = (sel + total - 1) % total;
		record_first_insert = 1;
	}

	if (input_ui_pressed(IPT_UI_SELECT))
	{
		if (sel == total - 1) sel = -1;
		else
		{
			seq_read_async_start();

			sel |= 1 << SEL_BITS;	/* we'll ask for a key */

			/* tell updatescreen() to clean after us (in case the window changes size) */
			schedule_full_refresh();
		}
	}

	if (input_ui_pressed(IPT_UI_CANCEL))
		sel = -1;

	if (input_ui_pressed(IPT_UI_CONFIGURE))
		sel = -2;

	if (sel == -1 || sel == -2)
	{
		/* tell updatescreen() to clean after us */
		schedule_full_refresh();

		record_first_insert = 1;
	}

	return sel + 1;
}



static int setcodesettings(struct mame_bitmap *bitmap,int selected)
{
	const char *menu_item[500];
	const char *menu_subitem[500];
	struct InputPort *entry[500];
	char flag[500];
	int i,sel;
	struct InputPort *in;
	int total;


	sel = selected - 1;


	if (Machine->input_ports == 0)
		return 0;

	in = Machine->input_ports;

	total = 0;
	while (in->type != IPT_END)
	{
		if (input_port_name(in) != 0 && seq_get_1(&in->seq) != CODE_NONE && (in->type & ~IPF_MASK) != IPT_UNKNOWN && (in->type & ~IPF_MASK) != IPT_OSD_DESCRIPTION 
		 && !( !options.cheat_input_ports && (in->type & IPF_CHEAT) ) )
		{
			entry[total] = in;
			menu_item[total] = input_port_name(in);

			total++;
		}

		in++;
	}

	if (total == 0) return 0;

	menu_item[total] = ui_getstring (UI_returntomain);
	menu_item[total + 1] = 0;	/* terminate array */
	total++;

	for (i = 0;i < total;i++)
	{
		if (i < total - 1)
		{
			seq_name(input_port_seq(entry[i]),menu_subitem_buffer[i],sizeof(menu_subitem_buffer[0]));
			menu_subitem[i] = menu_subitem_buffer[i];

			/* If the key isn't the default, flag it */
			if (seq_get_1(&entry[i]->seq) != CODE_DEFAULT)
				flag[i] = 1;
			else
				flag[i] = 0;

		} else
			menu_subitem[i] = 0;	/* no subitem */
	}

	if (sel > SEL_MASK)   /* are we waiting for a new key? */
	{
		int ret;

		menu_subitem[sel & SEL_MASK] = "    ";
		ui_displaymenu(bitmap,menu_item,menu_subitem,flag,sel & SEL_MASK,3);

		ret = seq_read_async(&entry[sel & SEL_MASK]->seq,record_first_insert);

		if (ret >= 0)
		{
			sel &= SEL_MASK;

			if (ret > 0 || seq_get_1(&entry[sel]->seq) == CODE_NONE)
			{
				seq_set_1(&entry[sel]->seq, CODE_DEFAULT);
				ret = 1;
			}

			/* tell updatescreen() to clean after us (in case the window changes size) */
			schedule_full_refresh();

			record_first_insert = ret != 0;
		}

		init_analog_seq();

		return sel + 1;
	}


	ui_displaymenu(bitmap,menu_item,menu_subitem,flag,sel,0);

	if (input_ui_pressed_repeat(IPT_UI_DOWN,8))
	{
		sel = (sel + 1) % total;
		record_first_insert = 1;
	}

	if (input_ui_pressed_repeat(IPT_UI_UP,8))
	{
		sel = (sel + total - 1) % total;
		record_first_insert = 1;
	}

	if (input_ui_pressed(IPT_UI_SELECT))
	{
		if (sel == total - 1) sel = -1;
		else
		{
			seq_read_async_start();

			sel |= 1 << SEL_BITS;	/* we'll ask for a key */

			/* tell updatescreen() to clean after us (in case the window changes size) */
			schedule_full_refresh();
		}
	}

	if (input_ui_pressed(IPT_UI_CANCEL))
		sel = -1;

	if (input_ui_pressed(IPT_UI_CONFIGURE))
		sel = -2;

	if (sel == -1 || sel == -2)
	{
		schedule_full_refresh();

		record_first_insert = 1;
	}

	return sel + 1;
}


static int calibratejoysticks(struct mame_bitmap *bitmap,int selected)
{
	const char *msg;
	static char buf[2048];
	int sel;
	static int calibration_started = 0;

	sel = selected - 1;

	if (calibration_started == 0)
	{
		osd_joystick_start_calibration();
		calibration_started = 1;
		strcpy (buf, "");
	}

	if (sel > SEL_MASK) /* Waiting for the user to acknowledge joystick movement */
	{
		if (input_ui_pressed(IPT_UI_CANCEL))
		{
			calibration_started = 0;
			sel = -1;
		}
		else if (input_ui_pressed(IPT_UI_SELECT))
		{
			osd_joystick_calibrate();
			sel &= SEL_MASK;
		}

		ui_displaymessagewindow(bitmap,buf);
	}
	else
	{
		msg = osd_joystick_calibrate_next();
		schedule_full_refresh();
		if (msg == 0)
		{
			calibration_started = 0;
			osd_joystick_end_calibration();
			sel = -1;
		}
		else
		{
			strcpy (buf, msg);
			ui_displaymessagewindow(bitmap,buf);
			sel |= 1 << SEL_BITS;
		}
	}

	if (input_ui_pressed(IPT_UI_CONFIGURE))
		sel = -2;

	if (sel == -1 || sel == -2)
	{
		schedule_full_refresh();
	}

	return sel + 1;
}


static int settraksettings(struct mame_bitmap *bitmap,int selected)
{
	const char *menu_item[40];
	const char *menu_subitem[40];
	struct InputPort *entry[40];
	int i,sel;
	struct InputPort *in;
	int total,total2;
	int arrowize;
	char label[30][40];
	char setting[30][40];


	sel = selected - 1;


	if (Machine->input_ports == 0)
		return 0;

	in = Machine->input_ports;

	/* Count the total number of analog controls */
	total = 0;
	while (in->type != IPT_END)
	{
		if (((in->type & 0xff) > IPT_ANALOG_START) && ((in->type & 0xff) < IPT_ANALOG_END)
				&& !(!options.cheat_input_ports && (in->type & IPF_CHEAT)))
		{
			entry[total] = in;
			total++;
		}
		in++;
	}

	if (total == 0) return 0;

	/* Each analog control has 3 entries - key & joy delta, reverse, sensitivity */

#define ENTRIES 3

	total2 = total * ENTRIES;

	menu_item[total2] = ui_getstring (UI_returntomain);
	menu_item[total2 + 1] = 0;	/* terminate array */
	total2++;

	arrowize = 0;
	for (i = 0;i < total2;i++)
	{
		if (i < total2 - 1)
		{
			int sensitivity,delta;
			int reverse;

			strcpy (label[i], input_port_name(entry[i/ENTRIES]));
			sensitivity = IP_GET_SENSITIVITY(entry[i/ENTRIES]);
			delta = IP_GET_DELTA(entry[i/ENTRIES]);
			reverse = (entry[i/ENTRIES]->type & IPF_REVERSE);

			strcat (label[i], " ");
			switch (i%ENTRIES)
			{
				case 0:
					strcat (label[i], ui_getstring (UI_keyjoyspeed));
					sprintf(setting[i],"%d",delta);
					if (i == sel) arrowize = 3;
					break;
				case 1:
					strcat (label[i], ui_getstring (UI_reverse));
					if (reverse)
						strcpy(setting[i],ui_getstring (UI_on));
					else
						strcpy(setting[i],ui_getstring (UI_off));
					if (i == sel) arrowize = 3;
					break;
				case 2:
					strcat (label[i], ui_getstring (UI_sensitivity));
					sprintf(setting[i],"%3d%%",sensitivity);
					if (i == sel) arrowize = 3;
					break;
			}

			menu_item[i] = label[i];
			menu_subitem[i] = setting[i];

			in++;
		}
		else menu_subitem[i] = 0;	/* no subitem */
	}

	ui_displaymenu(bitmap,menu_item,menu_subitem,0,sel,arrowize);

	if (input_ui_pressed_repeat(IPT_UI_DOWN,8))
		sel = (sel + 1) % total2;

	if (input_ui_pressed_repeat(IPT_UI_UP,8))
		sel = (sel + total2 - 1) % total2;

	if (input_ui_pressed_repeat(IPT_UI_LEFT,8))
	{
		if(sel != total2 - 1)
		{
			if ((sel % ENTRIES) == 0)
			/* keyboard/joystick delta */
			{
				int val = IP_GET_DELTA(entry[sel/ENTRIES]);

				val --;
				if (val < 1) val = 1;
				IP_SET_DELTA(entry[sel/ENTRIES],val);
			}
			else if ((sel % ENTRIES) == 1)
			/* reverse */
			{
				int reverse = entry[sel/ENTRIES]->type & IPF_REVERSE;
				if (reverse)
					reverse=0;
				else
					reverse=IPF_REVERSE;
				entry[sel/ENTRIES]->type &= ~IPF_REVERSE;
				entry[sel/ENTRIES]->type |= reverse;
			}
			else if ((sel % ENTRIES) == 2)
			/* sensitivity */
			{
				int val = IP_GET_SENSITIVITY(entry[sel/ENTRIES]);

				val --;
				if (val < 1) val = 1;
				IP_SET_SENSITIVITY(entry[sel/ENTRIES],val);
			}
		}
	}

	if (input_ui_pressed_repeat(IPT_UI_RIGHT,8))
	{
		if(sel != total2 - 1)
		{
			if ((sel % ENTRIES) == 0)
			/* keyboard/joystick delta */
			{
				int val = IP_GET_DELTA(entry[sel/ENTRIES]);

				val ++;
				if (val > 255) val = 255;
				IP_SET_DELTA(entry[sel/ENTRIES],val);
			}
			else if ((sel % ENTRIES) == 1)
			/* reverse */
			{
				int reverse = entry[sel/ENTRIES]->type & IPF_REVERSE;
				if (reverse)
					reverse=0;
				else
					reverse=IPF_REVERSE;
				entry[sel/ENTRIES]->type &= ~IPF_REVERSE;
				entry[sel/ENTRIES]->type |= reverse;
			}
			else if ((sel % ENTRIES) == 2)
			/* sensitivity */
			{
				int val = IP_GET_SENSITIVITY(entry[sel/ENTRIES]);

				val ++;
				if (val > 255) val = 255;
				IP_SET_SENSITIVITY(entry[sel/ENTRIES],val);
			}
		}
	}

	if (input_ui_pressed(IPT_UI_SELECT))
	{
		if (sel == total2 - 1) sel = -1;
	}

	if (input_ui_pressed(IPT_UI_CANCEL))
		sel = -1;

	if (input_ui_pressed(IPT_UI_CONFIGURE))
		sel = -2;

	if (sel == -1 || sel == -2)
	{
		schedule_full_refresh();
	}

	return sel + 1;
}

static int mame_stats(struct mame_bitmap *bitmap,int selected)
{
	char temp[10];
	char buf[2048];
	int sel, i;


	sel = selected - 1;

	buf[0] = 0;

	if (dispensed_tickets)
	{
		strcat(buf, ui_getstring (UI_tickets));
		strcat(buf, ": ");
		sprintf(temp, "%d\n\n", dispensed_tickets);
		strcat(buf, temp);
	}

	for (i=0; i<COIN_COUNTERS; i++)
	{
		strcat(buf, ui_getstring (UI_coin));
		sprintf(temp, " %c: ", i+'A');
		strcat(buf, temp);
		if (!coins[i])
			strcat (buf, ui_getstring (UI_NA));
		else
		{
			sprintf (temp, "%d", coins[i]);
			strcat (buf, temp);
		}
		if (coinlockedout[i])
		{
			strcat(buf, " ");
			strcat(buf, ui_getstring (UI_locked));
			strcat(buf, "\n");
		}
		else
		{
			strcat(buf, "\n");
		}
	}

	{
		/* menu system, use the normal menu keys */
		strcat(buf,"\n\t");
		strcat(buf,ui_getstring (UI_lefthilight));
		strcat(buf," ");
		strcat(buf,ui_getstring (UI_returntomain));
		strcat(buf," ");
		strcat(buf,ui_getstring (UI_righthilight));

		ui_displaymessagewindow(bitmap,buf);

		if (input_ui_pressed(IPT_UI_SELECT))
			sel = -1;

		if (input_ui_pressed(IPT_UI_CANCEL))
			sel = -1;

		if (input_ui_pressed(IPT_UI_CONFIGURE))
			sel = -2;
	}

	if (sel == -1 || sel == -2)
	{
		schedule_full_refresh();
	}

	return sel + 1;
}


static int displaygameinfo(struct mame_bitmap *bitmap,int selected)
{
	int i;
	char buf[2048];
	char buf2[32];
	int sel;


	sel = selected - 1;


	sprintf(buf,"%s\n%s %s\n\n%s:\n",Machine->gamedrv->description,Machine->gamedrv->year,Machine->gamedrv->manufacturer,
		ui_getstring (UI_cpu));
	i = 0;
	while (i < MAX_CPU && Machine->drv->cpu[i].cpu_type)
	{

		if (Machine->drv->cpu[i].cpu_clock >= 1000000)
			sprintf(&buf[strlen(buf)],"%s %d.%06d MHz",
					cputype_name(Machine->drv->cpu[i].cpu_type),
					Machine->drv->cpu[i].cpu_clock / 1000000,
					Machine->drv->cpu[i].cpu_clock % 1000000);
		else
			sprintf(&buf[strlen(buf)],"%s %d.%03d kHz",
					cputype_name(Machine->drv->cpu[i].cpu_type),
					Machine->drv->cpu[i].cpu_clock / 1000,
					Machine->drv->cpu[i].cpu_clock % 1000);

		if (Machine->drv->cpu[i].cpu_flags & CPU_AUDIO_CPU)
		{
			sprintf (buf2, " (%s)", ui_getstring (UI_sound_lc));
			strcat(buf, buf2);
		}

		strcat(buf,"\n");

		i++;
	}

	sprintf (buf2, "\n%s", ui_getstring (UI_sound));
	strcat (buf, buf2);
	if (Machine->drv->sound_attributes & SOUND_SUPPORTS_STEREO)
		sprintf(&buf[strlen(buf)]," (%s)", ui_getstring (UI_stereo));
	strcat(buf,":\n");

	i = 0;
	while (i < MAX_SOUND && Machine->drv->sound[i].sound_type)
	{
		if (sound_num(&Machine->drv->sound[i]))
			sprintf(&buf[strlen(buf)],"%dx",sound_num(&Machine->drv->sound[i]));

		sprintf(&buf[strlen(buf)],"%s",sound_name(&Machine->drv->sound[i]));

		if (sound_clock(&Machine->drv->sound[i]))
		{
			if (sound_clock(&Machine->drv->sound[i]) >= 1000000)
				sprintf(&buf[strlen(buf)]," %d.%06d MHz",
						sound_clock(&Machine->drv->sound[i]) / 1000000,
						sound_clock(&Machine->drv->sound[i]) % 1000000);
			else
				sprintf(&buf[strlen(buf)]," %d.%03d kHz",
						sound_clock(&Machine->drv->sound[i]) / 1000,
						sound_clock(&Machine->drv->sound[i]) % 1000);
		}

		strcat(buf,"\n");

		i++;
	}

	if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
		sprintf(&buf[strlen(buf)],"\n%s\n", ui_getstring (UI_vectorgame));
	else
	{
		sprintf(&buf[strlen(buf)],"\n%s:\n", ui_getstring (UI_screenres));
		sprintf(&buf[strlen(buf)],"%d x %d (%s) %f Hz\n",
				Machine->visible_area.max_x - Machine->visible_area.min_x + 1,
				Machine->visible_area.max_y - Machine->visible_area.min_y + 1,
				(Machine->gamedrv->flags & ORIENTATION_SWAP_XY) ? "V" : "H",
				Machine->drv->frames_per_second);
#if 0
		{
			int pixelx,pixely,tmax,tmin,rem;

			pixelx = 4 * (Machine->visible_area.max_y - Machine->visible_area.min_y + 1);
			pixely = 3 * (Machine->visible_area.max_x - Machine->visible_area.min_x + 1);

			/* calculate MCD */
			if (pixelx >= pixely)
			{
				tmax = pixelx;
				tmin = pixely;
			}
			else
			{
				tmax = pixely;
				tmin = pixelx;
			}
			while ( (rem = tmax % tmin) )
			{
				tmax = tmin;
				tmin = rem;
			}
			/* tmin is now the MCD */

			pixelx /= tmin;
			pixely /= tmin;

			sprintf(&buf[strlen(buf)],"pixel aspect ratio %d:%d\n",
					pixelx,pixely);
		}
		sprintf(&buf[strlen(buf)],"%d colors ",Machine->drv->total_colors);
#endif
	}


	if (sel == -1)
	{
		/* startup info, print MAME version and ask for any key */

		sprintf (buf2, "\n\t%s ", ui_getstring (UI_mame));	/* \t means that the line will be centered */
		strcat(buf, buf2);

		strcat(buf, APPNAME);
		sprintf (buf2, "\n\t%s", ui_getstring (UI_anykey));
		strcat(buf,buf2);
		ui_drawbox(bitmap,0,0,uirotwidth,uirotheight);
		ui_displaymessagewindow(bitmap,buf);

		sel = 0;
		if (code_read_async() != CODE_NONE)
			sel = -1;
	}
	else
	{
		/* menu system, use the normal menu keys */
		strcat(buf,"\n\t");
		strcat(buf,ui_getstring (UI_lefthilight));
		strcat(buf," ");
		strcat(buf,ui_getstring (UI_returntomain));
		strcat(buf," ");
		strcat(buf,ui_getstring (UI_righthilight));

		ui_displaymessagewindow(bitmap,buf);

		if (input_ui_pressed(IPT_UI_SELECT))
			sel = -1;

		if (input_ui_pressed(IPT_UI_CANCEL))
			sel = -1;

		if (input_ui_pressed(IPT_UI_CONFIGURE))
			sel = -2;
	}

	if (sel == -1 || sel == -2)
	{
		schedule_full_refresh();
	}

	return sel + 1;
}


void generate_gameinfo(void)
{
	int i;
	char buf2[32];

  message_buffer[0] = '\0';

	sprintf(message_buffer,"CONTROLS: %s\n\nGAMEINFO: %s\n%s %s\n\n%s:\n",Machine->gamedrv->ctrl_dat->control_details, Machine->gamedrv->description, Machine->gamedrv->year, Machine->gamedrv->manufacturer,
		ui_getstring (UI_cpu));
	i = 0;
	while (i < MAX_CPU && Machine->drv->cpu[i].cpu_type)
	{

		if (Machine->drv->cpu[i].cpu_clock >= 1000000)
			sprintf(&message_buffer[strlen(message_buffer)],"%s %d.%06d MHz",
					cputype_name(Machine->drv->cpu[i].cpu_type),
					Machine->drv->cpu[i].cpu_clock / 1000000,
					Machine->drv->cpu[i].cpu_clock % 1000000);
		else
			sprintf(&message_buffer[strlen(message_buffer)],"%s %d.%03d kHz",
					cputype_name(Machine->drv->cpu[i].cpu_type),
					Machine->drv->cpu[i].cpu_clock / 1000,
					Machine->drv->cpu[i].cpu_clock % 1000);

		if (Machine->drv->cpu[i].cpu_flags & CPU_AUDIO_CPU)
		{
			sprintf (buf2, " (%s)", ui_getstring (UI_sound_lc));
			strcat(message_buffer, buf2);
		}

		strcat(message_buffer,"\n");

		i++;
	}

	sprintf (buf2, "\n%s", ui_getstring (UI_sound));
	strcat (message_buffer, buf2);
	if (Machine->drv->sound_attributes & SOUND_SUPPORTS_STEREO)
		sprintf(&message_buffer[strlen(message_buffer)]," (%s)", ui_getstring (UI_stereo));
	strcat(message_buffer,":\n");

	i = 0;
	while (i < MAX_SOUND && Machine->drv->sound[i].sound_type)
	{
		if (sound_num(&Machine->drv->sound[i]))
			sprintf(&message_buffer[strlen(message_buffer)],"%dx",sound_num(&Machine->drv->sound[i]));

		sprintf(&message_buffer[strlen(message_buffer)],"%s",sound_name(&Machine->drv->sound[i]));

		if (sound_clock(&Machine->drv->sound[i]))
		{
			if (sound_clock(&Machine->drv->sound[i]) >= 1000000)
				sprintf(&message_buffer[strlen(message_buffer)]," %d.%06d MHz",
						sound_clock(&Machine->drv->sound[i]) / 1000000,
						sound_clock(&Machine->drv->sound[i]) % 1000000);
			else
				sprintf(&message_buffer[strlen(message_buffer)]," %d.%03d kHz",
						sound_clock(&Machine->drv->sound[i]) / 1000,
						sound_clock(&Machine->drv->sound[i]) % 1000);
		}

		strcat(message_buffer,"\n");

		i++;
	}

	if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
		sprintf(&message_buffer[strlen(message_buffer)],"\n%s\n", ui_getstring (UI_vectorgame));
	else
	{
		sprintf(&message_buffer[strlen(message_buffer)],"\n%s:\n", ui_getstring (UI_screenres));
		sprintf(&message_buffer[strlen(message_buffer)],"%d x %d (%s) %f Hz\n",
				Machine->visible_area.max_x - Machine->visible_area.min_x + 1,
				Machine->visible_area.max_y - Machine->visible_area.min_y + 1,
				(Machine->gamedrv->flags & ORIENTATION_SWAP_XY) ? "V" : "H",
				Machine->drv->frames_per_second);
#if 0
		{
			int pixelx,pixely,tmax,tmin,rem;

			pixelx = 4 * (Machine->visible_area.max_y - Machine->visible_area.min_y + 1);
			pixely = 3 * (Machine->visible_area.max_x - Machine->visible_area.min_x + 1);

			/* calculate MCD */
			if (pixelx >= pixely)
			{
				tmax = pixelx;
				tmin = pixely;
			}
			else
			{
				tmax = pixely;
				tmin = pixelx;
			}
			while ( (rem = tmax % tmin) )
			{
				tmax = tmin;
				tmin = rem;
			}
			/* tmin is now the MCD */

			pixelx /= tmin;
			pixely /= tmin;

			sprintf(&message_buffer[strlen(message_buffer)],"pixel aspect ratio %d:%d\n",
					pixelx,pixely);
		}
		sprintf(&message_buffer[strlen(message_buffer)],"%d colors ",Machine->drv->total_colors);
#endif
	}
}


void ui_copyright_and_warnings(void)
{
  char buffer[MAX_MESSAGE_LENGTH];
  int i;
  char warning_buffer[MAX_MESSAGE_LENGTH];
  bool first_warning = true;

  warning_buffer[0] = '\0';
  buffer[0]='\0';
  if(!options.skip_disclaimer)
    snprintf(buffer, MAX_MESSAGE_LENGTH, "%s", ui_getstring(UI_copyright));

	if (Machine->gamedrv->flags &
			(GAME_NOT_WORKING | GAME_UNEMULATED_PROTECTION | GAME_WRONG_COLORS | GAME_IMPERFECT_COLORS |
			  GAME_NO_SOUND | GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS | GAME_NO_COCKTAIL ))
  {

    strcat(warning_buffer, ui_getstring(UI_knownproblems));

    if (Machine->gamedrv->flags & GAME_IMPERFECT_COLORS)
    {
      strcat(warning_buffer, ui_getstring(UI_imperfectcolors));
      first_warning = false;
    }

    if (Machine->gamedrv->flags & GAME_WRONG_COLORS)
    {
      if(!first_warning)
        strcat(warning_buffer, ", ");

      strcat(warning_buffer, ui_getstring (UI_wrongcolors));
      first_warning = false;
    }

    if (Machine->gamedrv->flags & GAME_IMPERFECT_GRAPHICS)
    {
      if(!first_warning)
        strcat(warning_buffer, ", ");

      strcat(warning_buffer, ui_getstring (UI_imperfectgraphics));
      first_warning = false;
    }

    if (Machine->gamedrv->flags & GAME_IMPERFECT_SOUND)
    {
      if(!first_warning)
        strcat(warning_buffer, ", ");

      strcat(warning_buffer, ui_getstring (UI_imperfectsound));
      first_warning = false;
    }

    if (Machine->gamedrv->flags & GAME_NO_SOUND)
    {
      if(!first_warning)
        strcat(warning_buffer, ", ");

      strcat(warning_buffer, ui_getstring (UI_nosound));
      first_warning = false;
    }

    if (Machine->gamedrv->flags & GAME_NO_COCKTAIL)
    {
      if(!first_warning)
        strcat(warning_buffer, ", ");

      strcat(warning_buffer, ui_getstring (UI_nococktail));
      first_warning = false;
    }

    if (Machine->gamedrv->flags & GAME_DOESNT_SERIALIZE)
    {
      if(!first_warning)
        strcat(warning_buffer, ", ");

      strcat(warning_buffer, ui_getstring (UI_no_serialization));
      first_warning = false;
    }

    if (Machine->gamedrv->flags & (GAME_NOT_WORKING | GAME_UNEMULATED_PROTECTION)) /* major problems */
    {
      const struct GameDriver *maindrv;
      int foundworking;

      if (Machine->gamedrv->flags & GAME_NOT_WORKING)
      {
        if(!first_warning)
          strcat(warning_buffer, ", ");

        strcpy(warning_buffer, ui_getstring (UI_brokengame));
        first_warning = false;
      }

      if (Machine->gamedrv->flags & GAME_UNEMULATED_PROTECTION)
      {
        if(!first_warning)
          strcat(warning_buffer, ", ");

        strcat(warning_buffer, ui_getstring (UI_brokenprotection));
        first_warning = false;
      }
      if(!options.skip_warnings) /* send the warnings to the frontend before looking up alternatives */
      {
        frontend_message_cb(warning_buffer, 180);
      }
      if (Machine->gamedrv->clone_of && !(Machine->gamedrv->clone_of->flags & NOT_A_DRIVER))
      {
        maindrv = Machine->gamedrv->clone_of;
      }
      else
      {
        maindrv = Machine->gamedrv;
      }

      foundworking = 0;
      i = 0;
      while (drivers[i])
      {
        if (drivers[i] == maindrv || drivers[i]->clone_of == maindrv)
        {
          if ((drivers[i]->flags & (GAME_NOT_WORKING | GAME_UNEMULATED_PROTECTION)) == 0)
          {
            if (foundworking == 0)
            {
              strcat(warning_buffer,"\n\n");
              strcat(warning_buffer, ui_getstring (UI_workingclones));
              strcat(warning_buffer,"\n\n");
            }
            foundworking = 1;

            sprintf(&warning_buffer[strlen(warning_buffer)],"%s\n",drivers[i]->name);
          }
        }
        i++;
      }
    }
    else /* there is not a GAME_NOT_WORKING or GAME_UNEMULATED_PROTECTION flag set */
    {
      if(!options.skip_warnings) /* send the warnings to the frontend */
      {
        frontend_message_cb(warning_buffer, 180);
      }
    }

    log_cb(RETRO_LOG_WARN, LOGPRE "\n\n%s\n", warning_buffer); /* log warning list to the console */

  }

  generate_gameinfo();
  log_cb(RETRO_LOG_INFO, LOGPRE "\n\n%s\n", message_buffer);

  if(strlen(buffer))
    usrintf_showmessage_secs(8, "%s", buffer);

}

/* Word-wraps the text in the specified buffer to fit in maxwidth characters per line.
   The contents of the buffer are modified.
   Known limitations: Words longer than maxwidth cause the function to fail. */
static void wordwrap_text_buffer (char *buffer, int maxwidth)
{
	int width = 0;

	while (*buffer)
	{
		if (*buffer == '\n')
		{
			buffer++;
			width = 0;
			continue;
		}

		width++;

		if (width > maxwidth)
		{
			/* backtrack until a space is found */
			while (*buffer != ' ')
			{
				buffer--;
				width--;
			}
			if (width < 1) return;	/* word too long */

			/* replace space with a newline */
			*buffer = '\n';
		}
		else
			buffer++;
	}
}

static int count_lines_in_buffer (char *buffer)
{
	int lines = 0;
	char c;

	while ( (c = *buffer++) )
		if (c == '\n') lines++;

	return lines;
}

/* Display lines from buffer, starting with line 'scroll', in a width x height text window */
static void display_scroll_message (struct mame_bitmap *bitmap, int *scroll, int width, int height, char *buf)
{
	struct DisplayText dt[256];
	int curr_dt = 0;
	const char *uparrow = ui_getstring (UI_uparrow);
	const char *downarrow = ui_getstring (UI_downarrow);
	char textcopy[2048];
	char *copy;
	int leftoffs,topoffs;
	int first = *scroll;
	int buflines,showlines;
	int i;


	/* draw box */
	leftoffs = (uirotwidth - uirotcharwidth * (width + 1)) / 2;
	if (leftoffs < 0) leftoffs = 0;
	topoffs = (uirotheight - (3 * height + 1) * uirotcharheight / 2) / 2;
	ui_drawbox(bitmap,leftoffs,topoffs,(width + 1) * uirotcharwidth,(3 * height + 1) * uirotcharheight / 2);

	buflines = count_lines_in_buffer (buf);
	if (first > 0)
	{
		if (buflines <= height)
			first = 0;
		else
		{
			height--;
			if (first > (buflines - height))
				first = buflines - height;
		}
		*scroll = first;
	}

	if (first != 0)
	{
		/* indicate that scrolling upward is possible */
		dt[curr_dt].text = uparrow;
		dt[curr_dt].color = UI_COLOR_NORMAL;
		dt[curr_dt].x = (uirotwidth - uirotcharwidth * strlen(uparrow)) / 2;
		dt[curr_dt].y = topoffs + (3*curr_dt+1)*uirotcharheight/2;
		curr_dt++;
	}

	if ((buflines - first) > height)
		showlines = height - 1;
	else
		showlines = height;

	/* skip to first line */
	while (first > 0)
	{
		char c;

		while ( (c = *buf++) )
		{
			if (c == '\n')
			{
				first--;
				break;
			}
		}
	}

	/* copy 'showlines' lines from buffer, starting with line 'first' */
	copy = textcopy;
	for (i = 0; i < showlines; i++)
	{
		char *copystart = copy;

		while (*buf && *buf != '\n')
		{
			*copy = *buf;
			copy++;
			buf++;
		}
		*copy = '\0';
		copy++;
		if (*buf == '\n')
			buf++;

		if (*copystart == '\t') /* center text */
		{
			copystart++;
			dt[curr_dt].x = (uirotwidth - uirotcharwidth * (copy - copystart)) / 2;
		}
		else
			dt[curr_dt].x = leftoffs + uirotcharwidth/2;

		dt[curr_dt].text = copystart;
		dt[curr_dt].color = UI_COLOR_NORMAL;
		dt[curr_dt].y = topoffs + (3*curr_dt+1)*uirotcharheight/2;
		curr_dt++;
	}

	if (showlines == (height - 1))
	{
		/* indicate that scrolling downward is possible */
		dt[curr_dt].text = downarrow;
		dt[curr_dt].color = UI_COLOR_NORMAL;
		dt[curr_dt].x = (uirotwidth - uirotcharwidth * strlen(downarrow)) / 2;
		dt[curr_dt].y = topoffs + (3*curr_dt+1)*uirotcharheight/2;
		curr_dt++;
	}

	dt[curr_dt].text = 0;	/* terminate array */

	displaytext(bitmap,dt);
}


/* Display text entry for current driver from history.dat and mameinfo.dat. */
static int displayhistory (struct mame_bitmap *bitmap, int selected)
{
	static int scroll = 0;
	static char *buf = 0;
	int maxcols,maxrows;
	int sel;
	int bufsize = 256 * 1024; /* 256KB of history.dat buffer, enough for everything */

	sel = selected - 1;


	maxcols = (uirotwidth / uirotcharwidth) - 1;
	maxrows = (2 * uirotheight - uirotcharheight) / (3 * uirotcharheight);
	maxcols -= 2;
	maxrows -= 8;

	if (!buf)
	{
		/* allocate a buffer for the text */
		buf = malloc (bufsize);

		if (buf)
		{
			/* try to load entry */
			if (load_driver_history (Machine->gamedrv, buf, bufsize) == 0)
			{
				scroll = 0;
				wordwrap_text_buffer (buf, maxcols);
				strcat(buf,"\n\t");
				strcat(buf,ui_getstring (UI_lefthilight));
				strcat(buf," ");
				strcat(buf,ui_getstring (UI_returntomain));
				strcat(buf," ");
				strcat(buf,ui_getstring (UI_righthilight));
				strcat(buf,"\n");
			}
			else
			{
				free (buf);
				buf = 0;
			}
		}
	}

	{
		if (buf)
			display_scroll_message (bitmap, &scroll, maxcols, maxrows, buf);
		else
		{
			char msg[80];

			strcpy(msg,"\t");
			strcat(msg,ui_getstring(UI_historymissing));
			strcat(msg,"\n\n\t");
			strcat(msg,ui_getstring (UI_lefthilight));
			strcat(msg," ");
			strcat(msg,ui_getstring (UI_returntomain));
			strcat(msg," ");
			strcat(msg,ui_getstring (UI_righthilight));
			ui_displaymessagewindow(bitmap,msg);
		}

		if ((scroll > 0) && input_ui_pressed_repeat(IPT_UI_UP,4))
		{
			if (scroll == 2) scroll = 0;	/* 1 would be the same as 0, but with arrow on top */
			else scroll--;
		}

		if (input_ui_pressed_repeat(IPT_UI_DOWN,4))
		{
			if (scroll == 0) scroll = 2;	/* 1 would be the same as 0, but with arrow on top */
			else scroll++;
		}


		if (input_ui_pressed(IPT_UI_SELECT))
			sel = -1;

		if (input_ui_pressed(IPT_UI_CANCEL))
			sel = -1;

		if (input_ui_pressed(IPT_UI_CONFIGURE))
			sel = -2;
	}

	if (sel == -1 || sel == -2)
	{
		schedule_full_refresh();

		/* force buffer to be recreated */
		if (buf)
		{
			free (buf);
			buf = 0;
		}
	}

	return sel + 1;

}

int memcard_menu(struct mame_bitmap *bitmap, int selection)
{
	int sel;
	int menutotal = 0;
	const char *menuitem[10];
	char buf[256];
	char buf2[256];

	sel = selection - 1 ;

	sprintf(buf, "%s %03d", ui_getstring (UI_loadcard), mcd_number);
	menuitem[menutotal++] = buf;
	menuitem[menutotal++] = ui_getstring (UI_ejectcard);
	menuitem[menutotal++] = ui_getstring (UI_createcard);
	menuitem[menutotal++] = ui_getstring (UI_returntomain);
	menuitem[menutotal] = 0;

	if (mcd_action!=0)
	{
		strcpy (buf2, "\n");

		switch(mcd_action)
		{
			case 1:
				strcat (buf2, ui_getstring (UI_loadfailed));
				break;
			case 2:
				strcat (buf2, ui_getstring (UI_loadok));
				break;
			case 3:
				strcat (buf2, ui_getstring (UI_cardejected));
				break;
			case 4:
				strcat (buf2, ui_getstring (UI_cardcreated));
				break;
			case 5:
				strcat (buf2, ui_getstring (UI_cardcreatedfailed));
				strcat (buf2, "\n");
				strcat (buf2, ui_getstring (UI_cardcreatedfailed2));
				break;
			default:
				strcat (buf2, ui_getstring (UI_carderror));
				break;
		}

		strcat (buf2, "\n\n");
		ui_displaymessagewindow(bitmap,buf2);
		if (input_ui_pressed(IPT_UI_SELECT))
			mcd_action = 0;
	}
	else
	{
		ui_displaymenu(bitmap,menuitem,0,0,sel,0);

		if (input_ui_pressed_repeat(IPT_UI_RIGHT,8))
			mcd_number = (mcd_number + 1) % 1000;

		if (input_ui_pressed_repeat(IPT_UI_LEFT,8))
			mcd_number = (mcd_number + 999) % 1000;

		if (input_ui_pressed_repeat(IPT_UI_DOWN,8))
			sel = (sel + 1) % menutotal;

		if (input_ui_pressed_repeat(IPT_UI_UP,8))
			sel = (sel + menutotal - 1) % menutotal;

		if (input_ui_pressed(IPT_UI_SELECT))
		{
			switch(sel)
			{
			case 0:
				neogeo_memcard_eject();
				if (neogeo_memcard_load(mcd_number))
				{
					memcard_status=1;
					memcard_number=mcd_number;
					mcd_action = 2;
				}
				else
					mcd_action = 1;
				break;
			case 1:
				neogeo_memcard_eject();
				mcd_action = 3;
				break;
			case 2:
				if (neogeo_memcard_create(mcd_number))
					mcd_action = 4;
				else
					mcd_action = 5;
				break;
			case 3:
				sel=-1;
				break;


			}
		}

		if (input_ui_pressed(IPT_UI_CANCEL))
			sel = -1;

		if (input_ui_pressed(IPT_UI_CONFIGURE))
			sel = -2;

		if (sel == -1 || sel == -2)
		{
			schedule_full_refresh();
		}
	}

	return sel + 1;
}


enum { UI_SWITCH = 0,UI_DEFCODE,UI_CODE,UI_FLUSH_CURRENT_CFG, UI_FLUSH_ALL_CFG, UI_ANALOG,UI_CALIBRATE,
		UI_STATS,UI_GAMEINFO, UI_HISTORY,
		UI_CHEAT,UI_RESET, UI_GENERATE_XML_DAT, UI_MEMCARD,UI_RAPIDFIRE,UI_EXIT };



#define MAX_SETUPMENU_ITEMS 20
static const char *menu_item[MAX_SETUPMENU_ITEMS];
static int menu_action[MAX_SETUPMENU_ITEMS];
static int menu_total;


void setup_menu_init(void)
{
	menu_total = 0;

  if(options.mame_remapping)
  {
	  menu_item[menu_total] = ui_getstring (UI_inputgeneral);      menu_action[menu_total++] = UI_DEFCODE;
    menu_item[menu_total] = ui_getstring (UI_inputspecific);     menu_action[menu_total++] = UI_CODE;
    /*menu_item[menu_total] = ui_getstring (UI_flush_current_cfg); menu_action[menu_total++] = UI_FLUSH_CURRENT_CFG; */
    /*menu_item[menu_total] = ui_getstring (UI_flush_all_cfg);     menu_action[menu_total++] = UI_FLUSH_ALL_CFG; */
  }

	/* Determine if there are any dip switches */
	{
		struct InputPort *in;
		int num;

		in = Machine->input_ports;

		num = 0;
		while (in->type != IPT_END)
		{
			if ((in->type & ~IPF_MASK) == IPT_DIPSWITCH_NAME && input_port_name(in) != 0 &&
					(in->type & IPF_UNUSED) == 0 &&	!(in->type & IPF_CHEAT))
				num++;
			in++;
		}

		if (num != 0)
		{
			menu_item[menu_total] = ui_getstring (UI_dipswitches); menu_action[menu_total++] = UI_SWITCH;
		}
	}

	/* Determine if there are any analog controls */
	{
		struct InputPort *in;
		int num;

		in = Machine->input_ports;

		num = 0;
		while (in->type != IPT_END)
		{
			if (((in->type & 0xff) > IPT_ANALOG_START) && ((in->type & 0xff) < IPT_ANALOG_END)
					&& !(in->type & IPF_CHEAT))
				num++;
			in++;
		}

		if ( num != 0)
		{
			menu_item[menu_total] = ui_getstring (UI_analogcontrols); menu_action[menu_total++] = UI_ANALOG;
		}
	}

	/* Joystick calibration possible? - not implemented in the libretro port as of May 2018*/
	if ( osd_joystick_needs_calibration() != 0)
	{
		menu_item[menu_total] = ui_getstring (UI_calibrate); menu_action[menu_total++] = UI_CALIBRATE;
	}

	menu_item[menu_total] = ui_getstring (UI_bookkeeping); menu_action[menu_total++] = UI_STATS;
	menu_item[menu_total] = ui_getstring (UI_gameinfo); menu_action[menu_total++] = UI_GAMEINFO;
	menu_item[menu_total] = ui_getstring (UI_history); menu_action[menu_total++] = UI_HISTORY;

	menu_item[menu_total] = ui_getstring (UI_cheat); menu_action[menu_total++] = UI_CHEAT;

	if (options.content_flags[CONTENT_NEOGEO])
	{
		menu_item[menu_total] = ui_getstring (UI_memorycard); menu_action[menu_total++] = UI_MEMCARD;
	}

#if !defined(SPLIT_CORE) && !defined(WIIU) && !defined(GEKKO) && !defined(__CELLOS_LV2__) && !defined(__SWITCH__) && !defined(PSP) && !defined(VITA) && !defined(__GCW0__) && !defined(__EMSCRIPTEN__) && !defined(_XBOX)
    /* don't offer to generate_xml_dat on consoles where it can't be used */
    menu_item[menu_total] = ui_getstring (UI_generate_xml_dat);   menu_action[menu_total++] = UI_GENERATE_XML_DAT;

#endif
  if(!options.display_setup)
  {
    menu_item[menu_total] = ui_getstring (UI_returntogame); menu_action[menu_total++] = UI_EXIT;
  }
	menu_item[menu_total] = 0; /* terminate array */
}


static int setup_menu(struct mame_bitmap *bitmap, int selected)
{
	int sel,res=-1;
	static int menu_lastselected = 0;

  if(generate_DAT)
  {
    print_mame_xml();
    generate_DAT = false;
  }

	if (selected == -1)
		sel = menu_lastselected;
	else sel = selected - 1;

	if (sel > SEL_MASK)
	{
		switch (menu_action[sel & SEL_MASK])
		{
			case UI_SWITCH:
				res = setdipswitches(bitmap, sel >> SEL_BITS);
				break;
			case UI_DEFCODE:
				res = setdefcodesettings(bitmap, sel >> SEL_BITS);
				break;
			case UI_CODE:
				res = setcodesettings(bitmap, sel >> SEL_BITS);
				break;
			case UI_ANALOG:
				res = settraksettings(bitmap, sel >> SEL_BITS);
				break;
			case UI_CALIBRATE:
				res = calibratejoysticks(bitmap, sel >> SEL_BITS);
				break;
			case UI_STATS:
				res = mame_stats(bitmap, sel >> SEL_BITS);
				break;
			case UI_GAMEINFO:
				res = displaygameinfo(bitmap, sel >> SEL_BITS);
				break;
			case UI_HISTORY:
				res = displayhistory(bitmap, sel >> SEL_BITS);
				break;
			case UI_CHEAT:
				res = cheat_menu(bitmap, sel >> SEL_BITS);
				break;
			case UI_MEMCARD:
				res = memcard_menu(bitmap, sel >> SEL_BITS);
				break;

		}

		if (res == -1)
		{
			menu_lastselected = sel;
			sel = -1;
		}
		else
			sel = (sel & SEL_MASK) | (res << SEL_BITS);

		return sel + 1;
	}


	ui_displaymenu(bitmap,menu_item,0,0,sel,0);

	if (input_ui_pressed_repeat(IPT_UI_DOWN,8))
		sel = (sel + 1) % menu_total;

	if (input_ui_pressed_repeat(IPT_UI_UP,8))
		sel = (sel + menu_total - 1) % menu_total;

	if (input_ui_pressed(IPT_UI_SELECT))
	{
		switch (menu_action[sel])
		{
			case UI_SWITCH:
			case UI_DEFCODE:
			case UI_CODE:
			case UI_ANALOG:
			case UI_CALIBRATE:
			case UI_STATS:
			case UI_GAMEINFO:
			case UI_HISTORY:
			case UI_CHEAT:
			case UI_MEMCARD:
				sel |= 1 << SEL_BITS;
				schedule_full_refresh();
				break;

      case UI_GENERATE_XML_DAT:
          frontend_message_cb("Generating XML DAT", 180);
          schedule_full_refresh();
          generate_DAT = true;
          break;

			case UI_EXIT:
				menu_lastselected = 0;
				sel = -1;
				break;
		}
	}

	if (input_ui_pressed(IPT_UI_CANCEL) ||
			input_ui_pressed(IPT_UI_CONFIGURE))
	{
		menu_lastselected = sel;
		sel = -1;
	}

	if (sel == -1)
	{
		schedule_full_refresh();
	}

	return sel + 1;
}


/*********************************************************************

  end of On Screen Display handling

*********************************************************************/


static void displaymessage(struct mame_bitmap *bitmap,const char *text)
{
	struct DisplayText dt[2];
	int avail;


	if (uirotwidth < uirotcharwidth * strlen(text))
	{
		ui_displaymessagewindow(bitmap,text);
		return;
	}

	avail = strlen(text)+2;

	ui_drawbox(bitmap,(uirotwidth - uirotcharwidth * avail) / 2,
			uirotheight - 3*uirotcharheight,
			avail * uirotcharwidth,
			2*uirotcharheight);

	dt[0].text = text;
	dt[0].color = UI_COLOR_NORMAL;
	dt[0].x = (uirotwidth - uirotcharwidth * strlen(text)) / 2;
	dt[0].y = uirotheight - 5*uirotcharheight/2;
	dt[1].text = 0; /* terminate array */
	displaytext(bitmap,dt);
}

void CLIB_DECL usrintf_showmessage(const char *text,...)
{
	va_list arg;
	va_start(arg,text);
	vsprintf(messagetext,text,arg);
	va_end(arg);
	messagecounter = 2 * Machine->drv->frames_per_second;
}

void CLIB_DECL usrintf_showmessage_secs(int seconds, const char *text,...)
{
	va_list arg;
	va_start(arg,text);
	vsprintf(messagetext,text,arg);
	va_end(arg);
	messagecounter = seconds * Machine->drv->frames_per_second;
}

int handle_user_interface(struct mame_bitmap *bitmap)
{

	DoCheat(bitmap);	/* This must be called once a frame */

	if (setup_selected == 0)
  {
    if(input_ui_pressed(IPT_UI_CONFIGURE))
    {
      setup_selected = -1;
    }
    else if(options.display_setup)
    {
      setup_selected = -1;
      setup_via_menu = 1;
	    setup_menu_init();
    }
  }

	if (setup_selected && setup_via_menu && !options.display_setup)
  {
    setup_selected = 0;
    setup_via_menu = 0;
    setup_menu_init();
    schedule_full_refresh();
  }
  else if(setup_selected)
  {
    setup_selected = setup_menu(bitmap, setup_selected);
  }

#ifdef MAME_DEBUG
	if (!mame_debug)
#endif

	/* show popup message if any */
	if (messagecounter > 0)
	{
    if ( /* a popup is on screen and the user presses one of these UI controls */
        input_ui_pressed(IPT_UI_CANCEL) || input_ui_pressed(IPT_UI_SELECT)
     || input_ui_pressed(IPT_UI_UP)     || input_ui_pressed(IPT_UI_DOWN)
     || input_ui_pressed(IPT_UI_LEFT)   || input_ui_pressed(IPT_UI_RIGHT) )
     {
      messagecounter = 1; /* decrease to trigger screen refresh */
     }
		else
      displaymessage(bitmap, messagetext);

		if (--messagecounter == 0)
			schedule_full_refresh();
	}

	/* if the user pressed IPT_UI_SHOW_GFX, show the character set */
	if (input_ui_pressed(IPT_UI_SHOW_GFX))
	{
		showcharset(bitmap);
	}

	return 0;
}


void init_user_interface(void)
{
	/* clear the input memory */
	while (code_read_async() != CODE_NONE) {};

	setup_menu_init();
	setup_selected = 0;

}

int setup_active(void)
{
	return setup_selected;
}
