#ifndef __VECTOR__
#define __VECTOR__

#include "artwork.h"
#include "mame2003.h"

#define VECTOR_TEAM \
	"-* Vector Heads *-\n" \
	"Brad Oliver\n" \
	"Aaron Giles\n" \
	"Bernd Wiebelt\n" \
	"Allard van der Bas\n" \
	"Al Kossow (VECSIM)\n" \
	"Hedley Rainnie (VECSIM)\n" \
	"Eric Smith (VECSIM)\n" \
	"Neil Bradley (technical advice)\n" \
	"Andrew Caldwell (anti-aliasing)\n" \
	"- *** -\n"

#define MAX_POINTS 10000	/* Maximum # of points we can queue in a vector list */

#define MAX_PIXELS 850000  /* Maximum of pixels we can remember */

#define VECTOR_COLOR111(c) \
	MAKE_RGB((((c) >> 2) & 1) * 0xff, (((c) >> 1) & 1) * 0xff, (((c) >> 0) & 1) * 0xff)

#define VECTOR_COLOR222(c) \
	MAKE_RGB((((c) >> 4) & 3) * 0x55, (((c) >> 2) & 3) * 0x55, (((c) >> 0) & 3) * 0x55)

#define VECTOR_COLOR444(c) \
	MAKE_RGB((((c) >> 8) & 15) * 0x11, (((c) >> 4) & 15) * 0x11, (((c) >> 0) & 15) * 0x11)

#define Tinten(intensity, col) \
	MAKE_RGB((RGB_RED(col) * (intensity)) >> 8, (RGB_GREEN(col) * (intensity)) >> 8, (RGB_BLUE(col) * (intensity)) >> 8)

typedef UINT32 vector_pixel_t;
#define VECTOR_PIXEL_END	0xffffffff
#define VECTOR_PIXEL(x,y)	((x) | ((y) << 16))
#define VECTOR_PIXEL_X(p)	((p) & 0xffff)
#define VECTOR_PIXEL_Y(p)	((p) >> 16)

extern vector_pixel_t *vector_dirty_list;

extern unsigned char *vectorram;
extern size_t vectorram_size;

VIDEO_START( vector );
VIDEO_UPDATE( vector );

#define VCLEAN  0
#define VDIRTY  1
#define VCLIP   2

/* The vertices are buffered here */
typedef struct
{
	int x; int y;
	rgb_t col;
	int intensity;
	int arg1; int arg2; /* start/end in pixel array or clipping info */
	int status;         /* for dirty and clipping handling */
	rgb_t (*callback)(void);
} point;

void vector_register_aux_renderer(int (*aux_renderer)(point *start, int num_points));

void vector_clear_list (void);
void vector_draw_to (int x2, int y2, rgb_t col, int intensity, int dirty, rgb_t (*color_callback)(void));
void vector_add_point (int x, int y, rgb_t color, int intensity);
void vector_add_point_callback (int x, int y, rgb_t (*color_callback)(void), int intensity);
void vector_add_clip (int minx, int miny, int maxx, int maxy);
void vector_set_gamma(float _gamma);
float vector_get_gamma(void);

#endif
