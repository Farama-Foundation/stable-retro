/*
** File: tms9928a.c -- software implementation of the Texas Instruments
**                     TMS9918(A), TMS9928(A) and TMS9929(A), used by the Coleco, MSX and
**                     TI99/4(A).
**
** All undocumented features as described in the following file
** should be emulated.
**
** http://www.msxnet.org/tech/tms9918a.txt
**
** By Sean Young 1999 (sean@msxnet.org).
** Based on code by Mike Balfour. Features added:
** - read-ahead
** - single read/write address
** - AND mask for mode 2
** - multicolor mode
** - undocumented screen modes
** - illegal sprites (max 4 on one line)
** - vertical coordinate corrected -- was one to high (255 => 0, 0 => 1)
** - errors in interrupt emulation
** - back drop correctly emulated.
**
** 19 feb 2000, Sean:
** - now uses plot_pixel (..), so -ror works properly
** - fixed bug in tms.patternmask
**
** 3 nov 2000, Raphael Nabet:
** - fixed a nasty bug in _TMS9928A_sprites. A transparent sprite caused
**   sprites at lower levels not to be displayed, which is wrong.
**
** 3 jan 2001, Sean Young:
** - A few minor cleanups
** - Changed TMS9928A_vram_[rw] and  TMS9928A_register_[rw] to READ_HANDLER
**   and WRITE_HANDLER.
** - Got rid of the color table, unused. Also got rid of the old colors,
**   which were commented out anyway.
**
**
** Todo:
** - The screen image is rendered in `one go'. Modifications during
**   screen build up are not shown.
** - Correctly emulate 4,8,16 kb VRAM if needed.
** - uses plot_pixel (...) in TMS_sprites (...), which is rended in
**   in a back buffer created with malloc (). Hmm..
** - Colours are incorrect. [fixed by R Nabet ?]
*/

#include "driver.h"
#include "state.h"
#include "vidhrdw/generic.h"
#include "tms9928a.h"


/*
	New palette (R. Nabet).

	First 3 columns from TI datasheet (in volts).
	Next 3 columns based on formula :
		Y = .299*R + .587*G + .114*B (NTSC)
	(the coefficients are likely to be slightly different with PAL, but who cares ?)
	I assumed the "zero" for R-Y and B-Y was 0.47V.
	Last 3 coeffs are the 8-bit values.

	Color            Y  	R-Y 	B-Y 	R   	G   	B   	R	G	B
	0 Transparent
	1 Black         0.00	0.47	0.47	0.00	0.00	0.00	  0	  0	  0
	2 Medium green  0.53	0.07	0.20	0.13	0.79	0.26	 33	200	 66
	3 Light green   0.67	0.17	0.27	0.37	0.86	0.47	 94	220	120
	4 Dark blue     0.40	0.40	1.00	0.33	0.33	0.93	 84	 85	237
	5 Light blue    0.53	0.43	0.93	0.49	0.46	0.99	125	118	252
	6 Dark red      0.47	0.83	0.30	0.83	0.32	0.30	212	 82	 77
	7 Cyan          0.73	0.00	0.70	0.26	0.92	0.96	 66	235	245
	8 Medium red    0.53	0.93	0.27	0.99	0.33	0.33	252	 85	 84
	9 Light red     0.67	0.93	0.27	1.13(!)	0.47	0.47	255	121	120
	A Dark yellow   0.73	0.57	0.07	0.83	0.76	0.33	212	193	 84
	B Light yellow  0.80	0.57	0.17	0.90	0.81	0.50	230	206	128
	C Dark green    0.47	0.13	0.23	0.13	0.69	0.23	 33	176	 59
	D Magenta       0.53	0.73	0.67	0.79	0.36	0.73	201	 91	186
	E Gray          0.80	0.47	0.47	0.80	0.80	0.80	204	204	204
	F White         1.00	0.47	0.47	1.00	1.00	1.00	255	255	255
*/
static unsigned char TMS9928A_palette[16*3] =
{
	0, 0, 0,
	0, 0, 0,
	33, 200, 66,
	94, 220, 120,
	84, 85, 237,
	125, 118, 252,
	212, 82, 77,
	66, 235, 245,
	252, 85, 84,
	255, 121, 120,
	212, 193, 84,
	230, 206, 128,
	33, 176, 59,
	201, 91, 186,
	204, 204, 204,
	255, 255, 255
};

/*
** Defines for `dirty' optimization
*/

#define MAX_DIRTY_COLOUR        (256*3)
#define MAX_DIRTY_PATTERN       (256*3)
#define MAX_DIRTY_NAME          (40*24)

/*
** Forward declarations of internal functions.
*/
static void _TMS9928A_mode0 (struct mame_bitmap*);
static void _TMS9928A_mode1 (struct mame_bitmap*);
static void _TMS9928A_mode2 (struct mame_bitmap*);
static void _TMS9928A_mode12 (struct mame_bitmap*);
static void _TMS9928A_mode3 (struct mame_bitmap*);
static void _TMS9928A_mode23 (struct mame_bitmap*);
static void _TMS9928A_modebogus (struct mame_bitmap*);
static void _TMS9928A_sprites (struct mame_bitmap*);
static void _TMS9928A_change_register (int reg, UINT8 data);
static void _TMS9928A_set_dirty (char);

static void (*ModeHandlers[])(struct mame_bitmap*) = {
        _TMS9928A_mode0, _TMS9928A_mode1, _TMS9928A_mode2,  _TMS9928A_mode12,
        _TMS9928A_mode3, _TMS9928A_modebogus, _TMS9928A_mode23,
        _TMS9928A_modebogus };

#define IMAGE_SIZE (256*192)        /* size of rendered image        */

#define LEFT_BORDER			15		/* a bit less for 9918a??? */
#define RIGHT_BORDER		15		/* 13 for 9929a */
#define TOP_BORDER_60HZ		27
#define BOTTOM_BORDER_60HZ	24
#define TOP_BORDER_50HZ		51		/* unknown (102 for top+bottom?) */
#define BOTTOM_BORDER_50HZ	51		/* unknown (102 for top+bottom?) */
#define TOP_BORDER			tms.top_border
#define BOTTOM_BORDER		tms.bottom_border

#define TMS_SPRITES_ENABLED ((tms.Regs[1] & 0x50) == 0x40)
#define TMS_50HZ ((tms.model == TMS9929) || (tms.model == TMS9929A))
#define TMS_REVA ((tms.model == TMS99x8A) || (tms.model == TMS9929A))
#define TMS_MODE ( (TMS_REVA ? (tms.Regs[0] & 2) : 0) | \
	((tms.Regs[1] & 0x10)>>4) | ((tms.Regs[1] & 8)>>1))

typedef struct {
    /* TMS9928A internal settings */
    UINT8 ReadAhead,Regs[8],StatusReg,oldStatusReg,FirstByte,latch,INT;
    int Addr,BackColour,Change,mode;
    int colour,pattern,nametbl,spriteattribute,spritepattern;
    int colourmask,patternmask;
    void (*INTCallback)(int);
    /* memory */
    UINT8 *vMem, *dBackMem;
    struct mame_bitmap *tmpbmp;
    int vramsize, model;
    /* emulation settings */
    int LimitSprites; /* max 4 sprites on a row, like original TMS9918A */
    int top_border, bottom_border;
    /* dirty tables */
    char anyDirtyColour, anyDirtyName, anyDirtyPattern;
    char *DirtyColour, *DirtyName, *DirtyPattern;
} TMS9928A;

static TMS9928A tms;

/*
** initialize the palette
*/
static PALETTE_INIT( tms9928a )
{
	palette_set_colors(0, TMS9928A_palette, TMS9928A_PALETTE_SIZE);
}


/*
** The init, reset and shutdown functions
*/
void TMS9928A_reset () {
    int  i;

    for (i=0;i<8;i++) tms.Regs[i] = 0;
    tms.StatusReg = 0;
    tms.nametbl = tms.pattern = tms.colour = 0;
    tms.spritepattern = tms.spriteattribute = 0;
    tms.colourmask = tms.patternmask = 0;
    tms.Addr = tms.ReadAhead = tms.INT = 0;
    tms.mode = tms.BackColour = 0;
    tms.Change = 1;
    tms.FirstByte = 0;
	tms.latch = 0;
    _TMS9928A_set_dirty (1);
}

static int TMS9928A_start (const TMS9928a_interface *intf) {
    /* 4, 8 or 16 kB vram please */
    if (! ((intf->vram == 0x1000) || (intf->vram == 0x2000) || (intf->vram == 0x4000)) )
        return 1;

    tms.model = intf->model;

	tms.top_border = TMS_50HZ ? TOP_BORDER_50HZ : TOP_BORDER_60HZ;
	tms.bottom_border = TMS_50HZ ? BOTTOM_BORDER_50HZ : BOTTOM_BORDER_60HZ;

	tms.INTCallback = intf->int_callback;

    /* Video RAM */
    tms.vramsize = intf->vram;
    tms.vMem = (UINT8*) auto_malloc (intf->vram);
    if (!tms.vMem)
		return 1;
    memset (tms.vMem, 0, intf->vram);

    /* Sprite back buffer */
    tms.dBackMem = (UINT8*)auto_malloc (IMAGE_SIZE);
    if (!tms.dBackMem)
        return 1;

    /* dirty buffers */
    tms.DirtyName = (char*)auto_malloc (MAX_DIRTY_NAME);
    if (!tms.DirtyName)
        return 1;

    tms.DirtyPattern = (char*)auto_malloc (MAX_DIRTY_PATTERN);
    if (!tms.DirtyPattern)
        return 1;

    tms.DirtyColour = (char*)auto_malloc (MAX_DIRTY_COLOUR);
    if (!tms.DirtyColour)
        return 1;

    /* back bitmap */
    tms.tmpbmp = auto_bitmap_alloc (256, 192);
    if (!tms.tmpbmp)
        return 1;

    TMS9928A_reset ();
    tms.LimitSprites = 1;

	state_save_register_UINT8 ("tms9928a", 0, "R0", &tms.Regs[0], 1);
	state_save_register_UINT8 ("tms9928a", 0, "R1", &tms.Regs[1], 1);
	state_save_register_UINT8 ("tms9928a", 0, "R2", &tms.Regs[2], 1);
	state_save_register_UINT8 ("tms9928a", 0, "R3", &tms.Regs[3], 1);
	state_save_register_UINT8 ("tms9928a", 0, "R4", &tms.Regs[4], 1);
	state_save_register_UINT8 ("tms9928a", 0, "R5", &tms.Regs[5], 1);
	state_save_register_UINT8 ("tms9928a", 0, "R6", &tms.Regs[6], 1);
	state_save_register_UINT8 ("tms9928a", 0, "R7", &tms.Regs[7], 1);
	state_save_register_UINT8 ("tms9928a", 0, "S", &tms.StatusReg, 1);
	state_save_register_UINT8 ("tms9928a", 0, "read_ahead", &tms.ReadAhead, 1);
	state_save_register_UINT8 ("tms9928a", 0, "first_byte", &tms.FirstByte, 1);
	state_save_register_UINT8 ("tms9928a", 0, "latch", &tms.latch, 1);
	state_save_register_UINT16 ("tms9928a", 0, "vram_latch", (UINT16*)&tms.Addr, 1);
	state_save_register_UINT8 ("tms9928a", 0, "interrupt_line", &tms.INT, 1);
	state_save_register_UINT8 ("tms9928a", 0, "VRAM", tms.vMem, intf->vram);

    return 0;
}

void TMS9928A_post_load (void) {
	int i;

	/* mark the screen as dirty */
	_TMS9928A_set_dirty (1);

	/* all registers need to be re-written, so tables are recalculated */
	for (i=0;i<8;i++)
		_TMS9928A_change_register (i, tms.Regs[i]);

	/* make sure the back ground colour is reset */
	tms.BackColour = -1;

	/* make sure the interrupt request is set properly */
	if (tms.INTCallback) tms.INTCallback (tms.INT);
}

/*
** Set all dirty / clean
*/
static void _TMS9928A_set_dirty (char dirty) {
    tms.anyDirtyColour = tms.anyDirtyName = tms.anyDirtyPattern = dirty;
    memset (tms.DirtyName, dirty, MAX_DIRTY_NAME);
    memset (tms.DirtyColour, dirty, MAX_DIRTY_COLOUR);
    memset (tms.DirtyPattern, dirty, MAX_DIRTY_PATTERN);
}

/*
** The I/O functions.
*/
READ_HANDLER (TMS9928A_vram_r) {
    UINT8 b;
    b = tms.ReadAhead;
    tms.ReadAhead = tms.vMem[tms.Addr];
    tms.Addr = (tms.Addr + 1) & (tms.vramsize - 1);
    tms.latch = 0;
    return b;
}

WRITE_HANDLER (TMS9928A_vram_w) {
    int i;

    if (tms.vMem[tms.Addr] != data) {
        tms.vMem[tms.Addr] = data;
        tms.Change = 1;
        /* dirty optimization */
        if ( (tms.Addr >= tms.nametbl) &&
            (tms.Addr < (tms.nametbl + MAX_DIRTY_NAME) ) ) {
            tms.DirtyName[tms.Addr - tms.nametbl] = 1;
            tms.anyDirtyName = 1;
        }

        i = (tms.Addr - tms.colour) >> 3;
        if ( (i >= 0) && (i < MAX_DIRTY_COLOUR) ) {
            tms.DirtyColour[i] = 1;
            tms.anyDirtyColour = 1;
        }

        i = (tms.Addr - tms.pattern) >> 3;
        if ( (i >= 0) && (i < MAX_DIRTY_PATTERN) ) {
            tms.DirtyPattern[i] = 1;
            tms.anyDirtyPattern = 1;
        }
    }
    tms.Addr = (tms.Addr + 1) & (tms.vramsize - 1);
    tms.ReadAhead = data;
    tms.latch = 0;
}

READ_HANDLER (TMS9928A_register_r) {
    UINT8 b;
    b = tms.StatusReg;
    tms.StatusReg = 0x1f;
    if (tms.INT) {
        tms.INT = 0;
        if (tms.INTCallback) tms.INTCallback (tms.INT);
    }
    tms.latch = 0;
    return b;
}

WRITE_HANDLER (TMS9928A_register_w) {
	int reg;

    if (tms.latch) {
        if (data & 0x80) {
            /* register write */
			reg = data & 7;
			/*if (tms.FirstByte != tms.Regs[reg])*/ /* Removed to fix ColecoVision MESS Driver*/
	            _TMS9928A_change_register (reg, tms.FirstByte);
        } else {
            /* set read/write address */
            tms.Addr = ((UINT16)data << 8 | tms.FirstByte) & (tms.vramsize - 1);
            if ( !(data & 0x40) ) {
				/* read ahead */
				TMS9928A_vram_r	(0);
            }
        }
        tms.latch = 0;
    } else {
        tms.FirstByte = data;
		tms.latch = 1;
    }
}

static void _TMS9928A_change_register (int reg, UINT8 val) {
    static const UINT8 Mask[8] =
        { 0x03, 0xfb, 0x0f, 0xff, 0x07, 0x7f, 0x07, 0xff };
    static const char *modes[] = {
        "Mode 0 (GRAPHIC 1)", "Mode 1 (TEXT 1)", "Mode 2 (GRAPHIC 2)",
        "Mode 1+2 (TEXT 1 variation)", "Mode 3 (MULTICOLOR)",
        "Mode 1+3 (BOGUS)", "Mode 2+3 (MULTICOLOR variation)",
        "Mode 1+2+3 (BOGUS)" };
    UINT8 b;
    int mode;

    val &= Mask[reg];
    tms.Regs[reg] = val;

    log_cb(RETRO_LOG_DEBUG, LOGPRE "TMS9928A: Reg %d = %02xh\n", reg, (int)val);
    tms.Change = 1;
    switch (reg) {
    case 0:
        if (tms.mode != TMS_MODE) {
            /* re-calculate masks and pattern generator & colour */
            if (val & 2) {
                tms.colour = ((tms.Regs[3] & 0x80) * 64) & (tms.vramsize - 1);
                tms.colourmask = (tms.Regs[3] & 0x7f) * 8 | 7;
                tms.pattern = ((tms.Regs[4] & 4) * 2048) & (tms.vramsize - 1);
                tms.patternmask = (tms.Regs[4] & 3) * 256 |
				    (tms.colourmask & 255);
            } else {
                tms.colour = (tms.Regs[3] * 64) & (tms.vramsize - 1);
                tms.pattern = (tms.Regs[4] * 2048) & (tms.vramsize - 1);
            }
            tms.mode = TMS_MODE;
            log_cb(RETRO_LOG_DEBUG, LOGPRE "TMS9928A: %s\n", modes[tms.mode]);
            _TMS9928A_set_dirty (1);
        }
        break;
    case 1:
        /* check for changes in the INT line */
        b = (val & 0x20) && (tms.StatusReg & 0x80) ;
        if (b != tms.INT) {
            tms.INT = b;
            if (tms.INTCallback) tms.INTCallback (tms.INT);
        }
        mode = TMS_MODE;
        if (tms.mode != mode) {
            tms.mode = mode;
            _TMS9928A_set_dirty (1);
            log_cb(RETRO_LOG_DEBUG, LOGPRE "TMS9928A: %s\n", modes[tms.mode]);
        }
        break;
    case 2:
        tms.nametbl = (val * 1024) & (tms.vramsize - 1);
        tms.anyDirtyName = 1;
        memset (tms.DirtyName, 1, MAX_DIRTY_NAME);
        break;
    case 3:
        if (tms.Regs[0] & 2) {
            tms.colour = ((val & 0x80) * 64) & (tms.vramsize - 1);
            tms.colourmask = (val & 0x7f) * 8 | 7;
         } else {
            tms.colour = (val * 64) & (tms.vramsize - 1);
        }
        tms.anyDirtyColour = 1;
        memset (tms.DirtyColour, 1, MAX_DIRTY_COLOUR);
        break;
    case 4:
        if (tms.Regs[0] & 2) {
            tms.pattern = ((val & 4) * 2048) & (tms.vramsize - 1);
            tms.patternmask = (val & 3) * 256 | 255;
        } else {
            tms.pattern = (val * 2048) & (tms.vramsize - 1);
        }
        tms.anyDirtyPattern = 1;
        memset (tms.DirtyPattern, 1, MAX_DIRTY_PATTERN);
        break;
    case 5:
        tms.spriteattribute = (val * 128) & (tms.vramsize - 1);
        break;
    case 6:
        tms.spritepattern = (val * 2048) & (tms.vramsize - 1);
        break;
    case 7:
        /* The backdrop is updated at TMS9928A_refresh() */
        tms.anyDirtyColour = 1;
        memset (tms.DirtyColour, 1, MAX_DIRTY_COLOUR);
        break;
    }
}

/*
** Interface functions
*/

/*void TMS9928A_int_callback (void (*callback)(int)) {
    tms.INTCallback = callback;
}*/

void TMS9928A_set_spriteslimit (int limit) {
    tms.LimitSprites = limit;
}

/*
** Updates the screen (the dMem memory area).
*/
VIDEO_UPDATE( tms9928a )
{
    int c;

    if (tms.Change) {
        c = tms.Regs[7] & 15; if (!c) c=1;
        if (tms.BackColour != c) {
            tms.BackColour = c;
            palette_set_color (0,
                TMS9928A_palette[c * 3], TMS9928A_palette[c * 3 + 1],
                TMS9928A_palette[c * 3 + 2]);
        }
    }

	if (tms.Change)
	{
		if (tms.Regs[1] & 0x40)
			(*ModeHandlers[tms.mode])(tms.tmpbmp);
	}
	else
		tms.StatusReg = tms.oldStatusReg;

	if (! (tms.Regs[1] & 0x40))
		fillbitmap(bitmap, Machine->pens[tms.BackColour], &Machine->visible_area);
	else
	{
		copybitmap(bitmap, tms.tmpbmp, 0, 0, LEFT_BORDER, TOP_BORDER, &Machine->visible_area, TRANSPARENCY_NONE, 0);
		{
			struct rectangle rt;

			/* set borders */
			rt.min_x = 0; rt.max_x = LEFT_BORDER+256+RIGHT_BORDER-1;
			rt.min_y = 0; rt.max_y = TOP_BORDER-1;
			fillbitmap (bitmap, tms.BackColour, &rt);
			rt.min_y = TOP_BORDER+192; rt.max_y = TOP_BORDER+192+BOTTOM_BORDER-1;
			fillbitmap (bitmap, tms.BackColour, &rt);

			rt.min_y = TOP_BORDER; rt.max_y = TOP_BORDER+192-1;
			rt.min_x = 0; rt.max_x = LEFT_BORDER-1;
			fillbitmap (bitmap, tms.BackColour, &rt);
			rt.min_x = LEFT_BORDER+256; rt.max_x = LEFT_BORDER+256+RIGHT_BORDER-1;
			fillbitmap (bitmap, tms.BackColour, &rt);
	    }
		if (TMS_SPRITES_ENABLED)
			_TMS9928A_sprites(bitmap);
	}

    /* store Status register, so it can be restored at the next frame
       if there are no changes (sprite collision bit is lost) */
    tms.oldStatusReg = tms.StatusReg;
    tms.Change = 0;
	return;
}

int TMS9928A_interrupt () {
    int b;

    /* when skipping frames, calculate sprite collision */
    if (osd_skip_this_frame() ) {
        if (tms.Change) {
            if (TMS_SPRITES_ENABLED) {
                _TMS9928A_sprites (NULL);
            }
        } else {
	    	tms.StatusReg = tms.oldStatusReg;
		}
    }

    tms.StatusReg |= 0x80;
    b = (tms.Regs[1] & 0x20) != 0;
    if (b != tms.INT) {
        tms.INT = b;
        if (tms.INTCallback) tms.INTCallback (tms.INT);
    }

    return b;
}

static void _TMS9928A_mode1 (struct mame_bitmap *bmp) {
    int pattern,x,y,yy,xx,name,charcode;
    UINT8 fg,bg,*patternptr;
	struct rectangle rt;

    if ( !(tms.anyDirtyColour || tms.anyDirtyName || tms.anyDirtyPattern) )
         return;

    fg = Machine->pens[tms.Regs[7] / 16];
    bg = Machine->pens[tms.Regs[7] & 15];

    if (tms.anyDirtyColour) {
		/* colours at sides must be reset */
		rt.min_y = 0; rt.max_y = 191;
		rt.min_x = 0; rt.max_x = 7;
		fillbitmap (bmp, bg, &rt);
		rt.min_y = 0; rt.max_y = 191;
		rt.min_x = 248; rt.max_x = 255;
		fillbitmap (bmp, bg, &rt);
    }

    name = 0;
    for (y=0;y<24;y++) {
        for (x=0;x<40;x++) {
            charcode = tms.vMem[tms.nametbl+name];
            if ( !(tms.DirtyName[name++] || tms.DirtyPattern[charcode]) &&
				!tms.anyDirtyColour)
                continue;
            patternptr = tms.vMem + tms.pattern + (charcode*8);
            for (yy=0;yy<8;yy++) {
                pattern = *patternptr++;
                for (xx=0;xx<6;xx++) {
		    		plot_pixel (bmp, 8+x*6+xx, y*8+yy,
						(pattern & 0x80) ? fg : bg);
                    pattern *= 2;
                }
            }
        }
    }
    _TMS9928A_set_dirty (0);
}

static void _TMS9928A_mode12 (struct mame_bitmap *bmp) {
    int pattern,x,y,yy,xx,name,charcode;
    UINT8 fg,bg,*patternptr;
	struct rectangle rt;

    if ( !(tms.anyDirtyColour || tms.anyDirtyName || tms.anyDirtyPattern) )
         return;

    fg = Machine->pens[tms.Regs[7] / 16];
    bg = Machine->pens[tms.Regs[7] & 15];

    if (tms.anyDirtyColour) {
		/* colours at sides must be reset */
		rt.min_y = 0; rt.max_y = 191;
		rt.min_x = 0; rt.max_x = 7;
		fillbitmap (bmp, bg, &rt);
		rt.min_y = 0; rt.max_y = 191;
		rt.min_x = 248; rt.max_x = 255;
		fillbitmap (bmp, bg, &rt);
    }

    name = 0;
    for (y=0;y<24;y++) {
        for (x=0;x<40;x++) {
            charcode = (tms.vMem[tms.nametbl+name]+(y/8)*256)&tms.patternmask;
            if ( !(tms.DirtyName[name++] || tms.DirtyPattern[charcode]) &&
					!tms.anyDirtyColour)
                continue;
            patternptr = tms.vMem + tms.pattern + (charcode*8);
            for (yy=0;yy<8;yy++) {
                pattern = *patternptr++;
                for (xx=0;xx<6;xx++) {
		    		plot_pixel (bmp, 8+x*6+xx, y*8+yy,
                        (pattern & 0x80) ? fg : bg);
                    pattern *= 2;
                }
            }
        }
    }
    _TMS9928A_set_dirty (0);
}

static void _TMS9928A_mode0 (struct mame_bitmap *bmp) {
    int pattern,x,y,yy,xx,name,charcode,colour;
    UINT8 fg,bg,*patternptr;

    name = 0;
    for (y=0;y<24;y++) {
        for (x=0;x<32;x++) {
            charcode = tms.vMem[tms.nametbl+name];
            if ( !(tms.DirtyName[name++] || tms.DirtyPattern[charcode] ||
                tms.DirtyColour[charcode/64]) )
                continue;
            patternptr = tms.vMem + tms.pattern + charcode*8;
            colour = tms.vMem[tms.colour+charcode/8];
            fg = Machine->pens[colour / 16];
            bg = Machine->pens[colour & 15];
            for (yy=0;yy<8;yy++) {
                pattern=*patternptr++;
                for (xx=0;xx<8;xx++) {
		    		plot_pixel (bmp, x*8+xx, y*8+yy,
						(pattern & 0x80) ? fg : bg);
                    pattern *= 2;
                }
            }
        }
    }
    _TMS9928A_set_dirty (0);
}

static void _TMS9928A_mode2 (struct mame_bitmap *bmp) {
    int colour,name,x,y,yy,pattern,xx,charcode;
    UINT8 fg,bg;
    UINT8 *colourptr,*patternptr;

    if ( !(tms.anyDirtyColour || tms.anyDirtyName || tms.anyDirtyPattern) )
         return;

    name = 0;
    for (y=0;y<24;y++) {
        for (x=0;x<32;x++) {
            charcode = tms.vMem[tms.nametbl+name]+(y/8)*256;
            colour = (charcode&tms.colourmask);
            pattern = (charcode&tms.patternmask);
            if ( !(tms.DirtyName[name++] || tms.DirtyPattern[pattern] ||
                tms.DirtyColour[colour]) )
                continue;
            patternptr = tms.vMem+tms.pattern+colour*8;
            colourptr = tms.vMem+tms.colour+pattern*8;
            for (yy=0;yy<8;yy++) {
                pattern = *patternptr++;
                colour = *colourptr++;
                fg = Machine->pens[colour / 16];
                bg = Machine->pens[colour & 15];
                for (xx=0;xx<8;xx++) {
		    		plot_pixel (bmp, x*8+xx, y*8+yy,
						(pattern & 0x80) ? fg : bg);
                    pattern *= 2;
                }
            }
        }
    }
    _TMS9928A_set_dirty (0);
}

static void _TMS9928A_mode3 (struct mame_bitmap *bmp) {
    int x,y,yy,yyy,name,charcode;
    UINT8 fg,bg,*patternptr;

    if ( !(tms.anyDirtyColour || tms.anyDirtyName || tms.anyDirtyPattern) )
         return;

    name = 0;
    for (y=0;y<24;y++) {
        for (x=0;x<32;x++) {
            charcode = tms.vMem[tms.nametbl+name];
            if ( !(tms.DirtyName[name++] || tms.DirtyPattern[charcode]) &&
					!tms.anyDirtyColour)
                continue;
            patternptr = tms.vMem+tms.pattern+charcode*8+(y&3)*2;
            for (yy=0;yy<2;yy++) {
                fg = Machine->pens[(*patternptr / 16)];
                bg = Machine->pens[((*patternptr++) & 15)];
                for (yyy=0;yyy<4;yyy++) {
		    plot_pixel (bmp, x*8+0, y*8+yy*4+yyy, fg);
		    plot_pixel (bmp, x*8+1, y*8+yy*4+yyy, fg);
		    plot_pixel (bmp, x*8+2, y*8+yy*4+yyy, fg);
		    plot_pixel (bmp, x*8+3, y*8+yy*4+yyy, fg);
		    plot_pixel (bmp, x*8+4, y*8+yy*4+yyy, bg);
		    plot_pixel (bmp, x*8+5, y*8+yy*4+yyy, bg);
		    plot_pixel (bmp, x*8+6, y*8+yy*4+yyy, bg);
		    plot_pixel (bmp, x*8+7, y*8+yy*4+yyy, bg);
                }
            }
        }
    }
    _TMS9928A_set_dirty (0);
}

static void _TMS9928A_mode23 (struct mame_bitmap *bmp) {
    int x,y,yy,yyy,name,charcode;
    UINT8 fg,bg,*patternptr;

    if ( !(tms.anyDirtyColour || tms.anyDirtyName || tms.anyDirtyPattern) )
         return;

    name = 0;
    for (y=0;y<24;y++) {
        for (x=0;x<32;x++) {
            charcode = tms.vMem[tms.nametbl+name];
            if ( !(tms.DirtyName[name++] || tms.DirtyPattern[charcode]) &&
		!tms.anyDirtyColour)
                continue;
            patternptr = tms.vMem + tms.pattern +
                ((charcode+(y&3)*2+(y/8)*256)&tms.patternmask)*8;
            for (yy=0;yy<2;yy++) {
                fg = Machine->pens[(*patternptr / 16)];
                bg = Machine->pens[((*patternptr++) & 15)];
                for (yyy=0;yyy<4;yyy++) {
		    plot_pixel (bmp, x*8+0, y*8+yy*4+yyy, fg);
		    plot_pixel (bmp, x*8+1, y*8+yy*4+yyy, fg);
		    plot_pixel (bmp, x*8+2, y*8+yy*4+yyy, fg);
		    plot_pixel (bmp, x*8+3, y*8+yy*4+yyy, fg);
		    plot_pixel (bmp, x*8+4, y*8+yy*4+yyy, bg);
		    plot_pixel (bmp, x*8+5, y*8+yy*4+yyy, bg);
		    plot_pixel (bmp, x*8+6, y*8+yy*4+yyy, bg);
		    plot_pixel (bmp, x*8+7, y*8+yy*4+yyy, bg);
                }
            }
        }
    }
    _TMS9928A_set_dirty (0);
}

static void _TMS9928A_modebogus (struct mame_bitmap *bmp) {
    UINT8 fg,bg;
    int x,y,n,xx;

    if ( !(tms.anyDirtyColour || tms.anyDirtyName || tms.anyDirtyPattern) )
         return;

    fg = Machine->pens[tms.Regs[7] / 16];
    bg = Machine->pens[tms.Regs[7] & 15];

    for (y=0;y<192;y++) {
        xx=0;
        n=8; while (n--) plot_pixel (bmp, xx++, y, bg);
        for (x=0;x<40;x++) {
            n=4; while (n--) plot_pixel (bmp, xx++, y, fg);
            n=2; while (n--) plot_pixel (bmp, xx++, y, bg);
        }
        n=8; while (n--) plot_pixel (bmp, xx++, y, bg);
    }

    _TMS9928A_set_dirty (0);
}

/*
** This function renders the sprites. Sprite collision is calculated in
** in a back buffer (tms.dBackMem), because sprite collision detection
** is rather complicated (transparent sprites also cause the sprite
** collision bit to be set, and ``illegal'' sprites do not count
** (they're not displayed)).
**
** This code should be optimized. One day.
*/
static void _TMS9928A_sprites (struct mame_bitmap *bmp) {
    UINT8 *attributeptr,*patternptr,c;
    int p,x,y,size,i,j,large,yy,xx,limit[192],
        illegalsprite,illegalspriteline;
    UINT16 line,line2;

    attributeptr = tms.vMem + tms.spriteattribute;
    size = (tms.Regs[1] & 2) ? 16 : 8;
    large = (int)(tms.Regs[1] & 1);

    for (x=0;x<192;x++) limit[x] = 4;
    tms.StatusReg = 0x80;
    illegalspriteline = 255;
    illegalsprite = 0;

    memset (tms.dBackMem, 0, IMAGE_SIZE);
    for (p=0;p<32;p++) {
        y = *attributeptr++;
        if (y == 208) break;
        if (y > 208) {
            y=-(~y&255);
        } else {
            y++;
        }
        x = *attributeptr++;
        patternptr = tms.vMem + tms.spritepattern +
            ((size == 16) ? *attributeptr & 0xfc : *attributeptr) * 8;
        attributeptr++;
        c = (*attributeptr & 0x0f);
        if (*attributeptr & 0x80) x -= 32;
        attributeptr++;

        if (!large) {
            /* draw sprite (not enlarged) */
            for (yy=y;yy<(y+size);yy++) {
                if ( (yy < 0) || (yy > 191) ) continue;
                if (limit[yy] == 0) {
                    /* illegal sprite line */
                    if (yy < illegalspriteline) {
                        illegalspriteline = yy;
                        illegalsprite = p;
                    } else if (illegalspriteline == yy) {
                        if (illegalsprite > p) {
                            illegalsprite = p;
                        }
                    }
                    if (tms.LimitSprites) continue;
                } else limit[yy]--;
                line = 256*patternptr[yy-y] + patternptr[yy-y+16];
                for (xx=x;xx<(x+size);xx++) {
                    if (line & 0x8000) {
                        if ((xx >= 0) && (xx < 256)) {
                            if (tms.dBackMem[yy*256+xx]) {
                                tms.StatusReg |= 0x20;
                            } else {
                                tms.dBackMem[yy*256+xx] = 0x01;
                            }
                            if (c && ! (tms.dBackMem[yy*256+xx] & 0x02))
                            {
                            	tms.dBackMem[yy*256+xx] |= 0x02;
                            	if (bmp)
									plot_pixel (bmp, LEFT_BORDER+xx, TOP_BORDER+yy, Machine->pens[c]);
							}
                        }
                    }
                    line *= 2;
                }
            }
        } else {
            /* draw enlarged sprite */
            for (i=0;i<size;i++) {
                yy=y+i*2;
                line2 = 256*patternptr[i] + patternptr[i+16];
                for (j=0;j<2;j++) {
                    if ( (yy >= 0) && (yy <= 191) ) {
                        if (limit[yy] == 0) {
                            /* illegal sprite line */
                            if (yy < illegalspriteline) {
                                illegalspriteline = yy;
                                 illegalsprite = p;
                            } else if (illegalspriteline == yy) {
                                if (illegalsprite > p) {
                                    illegalsprite = p;
                                }
                            }
                            if (tms.LimitSprites) continue;
                        } else limit[yy]--;
                        line = line2;
                        for (xx=x;xx<(x+size*2);xx+=2) {
                            if (line & 0x8000) {
                                if ((xx >=0) && (xx < 256)) {
                                    if (tms.dBackMem[yy*256+xx]) {
                                        tms.StatusReg |= 0x20;
                                    } else {
                                        tms.dBackMem[yy*256+xx] = 0x01;
                                    }
		                            if (c && ! (tms.dBackMem[yy*256+xx] & 0x02))
        		                    {
                		            	tms.dBackMem[yy*256+xx] |= 0x02;
                                        if (bmp)
                                        	plot_pixel (bmp, LEFT_BORDER+xx, TOP_BORDER+yy, Machine->pens[c]);
                		            }
                                }
                                if (((xx+1) >=0) && ((xx+1) < 256)) {
                                    if (tms.dBackMem[yy*256+xx+1]) {
                                        tms.StatusReg |= 0x20;
                                    } else {
                                        tms.dBackMem[yy*256+xx+1] = 0x01;
                                    }
		                            if (c && ! (tms.dBackMem[yy*256+xx+1] & 0x02))
        		                    {
                		            	tms.dBackMem[yy*256+xx+1] |= 0x02;
                                        if (bmp)
                                        	plot_pixel (bmp, LEFT_BORDER+xx+1, TOP_BORDER+yy, Machine->pens[c]);
									}
                                }
                            }
                            line *= 2;
                        }
                    }
                    yy++;
                }
            }
        }
    }
    if (illegalspriteline == 255) {
        tms.StatusReg |= (p > 31) ? 31 : p;
    } else {
        tms.StatusReg |= 0x40 + illegalsprite;
    }
}

static TMS9928a_interface sIntf;

static VIDEO_START ( TMS9928A_hack )
{
	return TMS9928A_start(&sIntf);
}

void mdrv_tms9928a(struct InternalMachineDriver *machine, const TMS9928a_interface *intf)
{
	int top_border = ((intf->model == TMS9929) || (intf->model == TMS9929A)) ? TOP_BORDER_50HZ : TOP_BORDER_60HZ;
	int bottom_border = ((intf->model == TMS9929) || (intf->model == TMS9929A)) ? BOTTOM_BORDER_50HZ : BOTTOM_BORDER_60HZ;

	sIntf = *intf;

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_UPDATE_BEFORE_VBLANK | VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(LEFT_BORDER+32*8+RIGHT_BORDER, top_border+24*8+bottom_border)
	/* display no border */
	/*MDRV_VISIBLE_AREA(LEFT_BORDER+0*8, LEFT_BORDER+32*8-1, top_border+0*8, top_border+24*8-1)*/
	/* display max borders */
	/*MDRV_VISIBLE_AREA(0, LEFT_BORDER+32*8+RIGHT_BORDER-1, 0, top_border+24*8+bottom_border-1)*/
	/* display fixed (12+12)*(9+9) borders */
	MDRV_VISIBLE_AREA(LEFT_BORDER-12, LEFT_BORDER+32*8+12-1, top_border-9, top_border+24*8+9-1)
	MDRV_PALETTE_LENGTH(TMS9928A_PALETTE_SIZE)
	MDRV_COLORTABLE_LENGTH(0)
	MDRV_PALETTE_INIT(tms9928a)

	MDRV_VIDEO_START(TMS9928A_hack)
	MDRV_VIDEO_UPDATE(tms9928a)
}
