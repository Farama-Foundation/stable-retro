/******************************************************************************

	palette.c

	Palette handling functions.

	There are several levels of abstraction in the way MAME handles the palette,
	and several display modes which can be used by the drivers.

	Palette
	-------
	Note: in the following text, "color" refers to a color in the emulated
	game's virtual palette. For example, a game might be able to display 1024
	colors at the same time. If the game uses RAM to change the available
	colors, the term "palette" refers to the colors available at any given time,
	not to the whole range of colors which can be produced by the hardware. The
	latter is referred to as "color space".
	The term "pen" refers to one of the maximum MAX_PENS colors that can be
	used to generate the display.

	So, to summarize, the three layers of palette abstraction are:

	P1) The game virtual palette (the "colors")
	P2) MAME's MAX_PENS colors palette (the "pens")
	P3) The OS specific hardware color registers (the "OS specific pens")

	The array Machine->pens[] is a lookup table which maps game colors to OS
	specific pens (P1 to P3). When you are working on bitmaps at the pixel level,
	*always* use Machine->pens to map the color numbers. *Never* use constants.
	For example if you want to make pixel (x,y) of color 3, do:
	bitmap->line[y][x] = Machine->pens[3];


	Lookup table
	------------
	Palettes are only half of the story. To map the gfx data to colors, the
	graphics routines use a lookup table. For example if we have 4bpp tiles,
	which can have 256 different color codes, the lookup table for them will have
	256 * 2^4 = 4096 elements. For games using a palette RAM, the lookup table is
	usually a 1:1 map. For games using PROMs, the lookup table is often larger
	than the palette itself so for example the game can display 256 colors out
	of a palette of 16.

	The palette and the lookup table are initialized to default values by the
	main core, but can be initialized by the driver using the function
	MachineDriver->vh_init_palette(). For games using palette RAM, that
	function is usually not needed, and the lookup table can be set up by
	properly initializing the color_codes_start and total_color_codes fields in
	the GfxDecodeInfo array.
	When vh_init_palette() initializes the lookup table, it maps gfx codes
	to game colors (P1 above). The lookup table will be converted by the core to
	map to OS specific pens (P3 above), and stored in Machine->remapped_colortable.


	Display modes
	-------------
	The available display modes can be summarized in three categories:
	1) Static palette. Use this for games which use PROMs for color generation.
		The palette is initialized by palette_init(), and never changed
		again.
	2) Dynamic palette. Use this for games which use RAM for color generation.
		The palette can be dynamically modified by the driver using the function
		palette_set_color().
	3) Direct mapped 16-bit or 32-bit color. This should only be used in special
		cases, e.g. to support alpha blending.
		MachineDriver->video_attributes must contain VIDEO_RGB_DIRECT.

******************************************************************************/

#ifndef PALETTE_H
#define PALETTE_H

#ifdef __cplusplus
extern "C" {
#endif


/*-------------------------------------------------
	TYPE DEFINITIONS
-------------------------------------------------*/

struct mame_display;		/* declared elsewhere */

typedef UINT32 pen_t;
typedef UINT32 rgb_t;
typedef UINT16 rgb15_t;



/*-------------------------------------------------
	CONSTANTS
-------------------------------------------------*/

#define PALETTE_DEFAULT_SHADOW_FACTOR (0.6)
#define PALETTE_DEFAULT_HIGHLIGHT_FACTOR (1/PALETTE_DEFAULT_SHADOW_FACTOR)

#define PALETTE_DEFAULT_SHADOW_FACTOR32 (0.6)
#define PALETTE_DEFAULT_HIGHLIGHT_FACTOR32 (1/PALETTE_DEFAULT_SHADOW_FACTOR32)



/*-------------------------------------------------
	MACROS
-------------------------------------------------*/

#define MAKE_RGB(r,g,b) 	((((r) & 0xff) << 16) | (((g) & 0xff) << 8) | ((b) & 0xff))
#define MAKE_ARGB(a,r,g,b)	(MAKE_RGB(r,g,b) | (((a) & 0xff) << 24))
#define RGB_ALPHA(rgb)		(((rgb) >> 24) & 0xff)
#define RGB_RED(rgb)		(((rgb) >> 16) & 0xff)
#define RGB_GREEN(rgb)		(((rgb) >> 8) & 0xff)
#define RGB_BLUE(rgb)		((rgb) & 0xff)



/*-------------------------------------------------
	GLOBAL VARIABLES
-------------------------------------------------*/

extern UINT32 direct_rgb_components[3];
extern UINT16 *palette_shadow_table;

extern data8_t *paletteram;
extern data8_t *paletteram_2;	/* use when palette RAM is split in two parts */
extern data16_t *paletteram16;
extern data16_t *paletteram16_2;
extern data32_t *paletteram32;



/*-------------------------------------------------
	PROTOTYPES
-------------------------------------------------*/

int palette_start(void);
int palette_init(void);
int palette_get_total_colors_with_ui(void);

void palette_update_display(struct mame_display *display);

void palette_set_color(pen_t pen, UINT8 r, UINT8 g, UINT8 b);
void palette_get_color(pen_t pen, UINT8 *r, UINT8 *g, UINT8 *b);
void palette_set_colors(pen_t color_base, const UINT8 *colors, int color_count);

int palette_get_total_colors(void);
void palette_normalize_range( UINT32 start, UINT32 end, int lum_min, int lum_max);
void res_palette_set_colors(pen_t color_base, rgb_t *colors, int color_count);

void palette_set_brightness(pen_t pen, double bright);
void palette_set_shadow_factor(double factor);
void palette_set_highlight_factor(double factor);

/*
	Shadows(Highlights) Quick Reference
	-----------------------------------

	1) declare MDRV_VIDEO_ATTRIBUTES( ... VIDEO_HAS_SHADOWS | VIDEO_HAS_HIGHLIGHTS ... )

	2) set gfx_drawmode_table[0-n] to DRAWMODE_NONE, DRAWMODE_SOURCE or DRAWMODE_SHADOW

	3) (optional) set shadow darkness or highlight brightness by
		palette_set_shadow_factor32(0.0-1.0) or
		palette_set_highlight_factor32(1.0-n.n)

	4) before calling drawgfx use
		palette_set_shadow_mode(0) to arm shadows or
		palette_set_shadow_mode(1) to arm highlights

	5) call drawgfx with the TRANSPARENCY_PEN_TABLE flag
		drawgfx( ..., cliprect, TRANSPARENCY_PEN_TABLE, transparent_color )
*/
void palette_set_shadow_mode(int mode);
void palette_set_shadow_factor32(double factor);
void palette_set_highlight_factor32(double factor);
void palette_set_shadow_dRGB32(int mode, int dr, int dg, int db, int noclip);
void palette_set_highlight_method(int method);
/*0=default, 1=multiplication with flooding, 2=addition */

void palette_set_global_gamma(double _gamma);
double palette_get_global_gamma(void);

void palette_set_global_brightness(double brightness);
void palette_set_global_brightness_adjust(double adjustment);
double palette_get_global_brightness(void);

pen_t get_black_pen(void);


/* here are some functions to handle commonly used palette layouts, so you don't
   have to write your own paletteram_w() function. */

READ_HANDLER( paletteram_r );
READ_HANDLER( paletteram_2_r );
READ16_HANDLER( paletteram16_word_r );
READ16_HANDLER( paletteram16_2_word_r );
READ32_HANDLER( paletteram32_r );

WRITE_HANDLER( paletteram_BBGGGRRR_w );
WRITE_HANDLER( paletteram_RRRGGGBB_w );
WRITE_HANDLER( paletteram_BBBGGGRR_w );
WRITE_HANDLER( paletteram_IIBBGGRR_w );
WRITE_HANDLER( paletteram_BBGGRRII_w );

/* _w       least significant byte first */
/* _swap_w  most significant byte first */
/* _split_w least and most significant bytes are not consecutive */
/* _word_w  use with 16 bit CPU */
/* R, G, B are bits, r, g, b are bytes */
/*                        MSB          LSB */
WRITE_HANDLER( paletteram_xxxxBBBBGGGGRRRR_w );
WRITE_HANDLER( paletteram_xxxxBBBBGGGGRRRR_swap_w );
WRITE_HANDLER( paletteram_xxxxBBBBGGGGRRRR_split1_w );	/* uses paletteram[] */
WRITE_HANDLER( paletteram_xxxxBBBBGGGGRRRR_split2_w );	/* uses paletteram_2[] */
WRITE16_HANDLER( paletteram16_xxxxBBBBGGGGRRRR_word_w );
WRITE_HANDLER( paletteram_xxxxBBBBRRRRGGGG_w );
WRITE_HANDLER( paletteram_xxxxBBBBRRRRGGGG_swap_w );
WRITE_HANDLER( paletteram_xxxxBBBBRRRRGGGG_split1_w );	/* uses paletteram[] */
WRITE_HANDLER( paletteram_xxxxBBBBRRRRGGGG_split2_w );	/* uses paletteram_2[] */
WRITE16_HANDLER( paletteram16_xxxxBBBBRRRRGGGG_word_w );
WRITE_HANDLER( paletteram_xxxxRRRRBBBBGGGG_split1_w );	/* uses paletteram[] */
WRITE_HANDLER( paletteram_xxxxRRRRBBBBGGGG_split2_w );	/* uses paletteram_2[] */
WRITE_HANDLER( paletteram_xxxxRRRRGGGGBBBB_w );
WRITE_HANDLER( paletteram_xxxxRRRRGGGGBBBB_swap_w );
WRITE16_HANDLER( paletteram16_xxxxRRRRGGGGBBBB_word_w );
WRITE_HANDLER( paletteram_RRRRGGGGBBBBxxxx_swap_w );
WRITE_HANDLER( paletteram_RRRRGGGGBBBBxxxx_split1_w );	/* uses paletteram[] */
WRITE_HANDLER( paletteram_RRRRGGGGBBBBxxxx_split2_w );	/* uses paletteram_2[] */
WRITE16_HANDLER( paletteram16_RRRRGGGGBBBBxxxx_word_w );
WRITE_HANDLER( paletteram_BBBBGGGGRRRRxxxx_swap_w );
WRITE_HANDLER( paletteram_BBBBGGGGRRRRxxxx_split1_w );	/* uses paletteram[] */
WRITE_HANDLER( paletteram_BBBBGGGGRRRRxxxx_split2_w );	/* uses paletteram_2[] */
WRITE16_HANDLER( paletteram16_BBBBGGGGRRRRxxxx_word_w );
WRITE_HANDLER( paletteram_xBBBBBGGGGGRRRRR_w );
WRITE_HANDLER( paletteram_xBBBBBGGGGGRRRRR_swap_w );
WRITE_HANDLER( paletteram_xBBBBBGGGGGRRRRR_split1_w );	/* uses paletteram[] */
WRITE_HANDLER( paletteram_xBBBBBGGGGGRRRRR_split2_w );	/* uses paletteram_2[] */
WRITE16_HANDLER( paletteram16_xBBBBBGGGGGRRRRR_word_w );
WRITE_HANDLER( paletteram_xRRRRRGGGGGBBBBB_w );
WRITE16_HANDLER( paletteram16_xRRRRRGGGGGBBBBB_word_w );
WRITE16_HANDLER( paletteram16_xGGGGGRRRRRBBBBB_word_w );
WRITE16_HANDLER( paletteram16_xGGGGGBBBBBRRRRR_word_w );
WRITE_HANDLER( paletteram_RRRRRGGGGGBBBBBx_w );
WRITE16_HANDLER( paletteram16_RRRRRGGGGGBBBBBx_word_w );
WRITE16_HANDLER( paletteram16_IIIIRRRRGGGGBBBB_word_w );
WRITE16_HANDLER( paletteram16_RRRRGGGGBBBBIIII_word_w );
WRITE16_HANDLER( paletteram16_xrgb_word_w );
WRITE16_HANDLER( paletteram16_xbgr_word_w );
WRITE16_HANDLER( paletteram16_RRRRGGGGBBBBRGBx_word_w );


/******************************************************************************

 Commonly used color PROM handling functions

******************************************************************************/

void palette_init_black_and_white(UINT16 *colortable, const UINT8 *color_prom);
void palette_init_RRRR_GGGG_BBBB(UINT16 *colortable, const UINT8 *color_prom);



/***************************************************************************
    static INLINE FUNCTIONS
***************************************************************************/

/*-------------------------------------------------
    rgb_to_rgb15 - convert an RGB triplet to
    a 15-bit OSD-specified RGB value
-------------------------------------------------*/

static INLINE rgb15_t rgb_to_rgb15(rgb_t rgb)
{
	return ((RGB_RED(rgb) >> 3) << 10) | ((RGB_GREEN(rgb) >> 3) << 5) | ((RGB_BLUE(rgb) >> 3) << 0);
}


/*-------------------------------------------------
    rgb_clamp - clamp an RGB component to 0-255
-------------------------------------------------*/

static INLINE UINT8 rgb_clamp(INT32 value)
{
	if (value < 0)
		return 0;
	if (value > 255)
		return 255;
	return value;
}


/*-------------------------------------------------
    pal1bit - convert a 1-bit value to 8 bits
-------------------------------------------------*/

static INLINE UINT8 pal1bit(UINT8 bits)
{
	return (bits & 1) ? 0xff : 0x00;
}


/*-------------------------------------------------
    pal2bit - convert a 2-bit value to 8 bits
-------------------------------------------------*/

static INLINE UINT8 pal2bit(UINT8 bits)
{
	bits &= 3;
	return (bits << 6) | (bits << 4) | (bits << 2) | bits;
}


/*-------------------------------------------------
    pal3bit - convert a 3-bit value to 8 bits
-------------------------------------------------*/

static INLINE UINT8 pal3bit(UINT8 bits)
{
	bits &= 7;
	return (bits << 5) | (bits << 2) | (bits >> 1);
}


/*-------------------------------------------------
    pal4bit - convert a 4-bit value to 8 bits
-------------------------------------------------*/

static INLINE UINT8 pal4bit(UINT8 bits)
{
	bits &= 0xf;
	return (bits << 4) | bits;
}


/*-------------------------------------------------
    pal5bit - convert a 5-bit value to 8 bits
-------------------------------------------------*/

static INLINE UINT8 pal5bit(UINT8 bits)
{
	bits &= 0x1f;
	return (bits << 3) | (bits >> 2);
}


/*-------------------------------------------------
    pal6bit - convert a 6-bit value to 8 bits
-------------------------------------------------*/

static INLINE UINT8 pal6bit(UINT8 bits)
{
	bits &= 0x3f;
	return (bits << 2) | (bits >> 4);
}


/*-------------------------------------------------
    pal7bit - convert a 7-bit value to 8 bits
-------------------------------------------------*/

static INLINE UINT8 pal7bit(UINT8 bits)
{
	bits &= 0x7f;
	return (bits << 1) | (bits >> 6);
}

#ifdef __cplusplus
}
#endif

#endif
