/*********************************************************************

	artwork.h

	Second generation artwork implementation.

*********************************************************************/

#ifndef ARTWORK_H
#define ARTWORK_H


/***************************************************************************

	Constants

***************************************************************************/

/* the various types of built-in overlay primitives */
#define OVERLAY_TYPE_END			0
#define OVERLAY_TYPE_RECTANGLE			1
#define OVERLAY_TYPE_DISK			2
#define OVERLAY_TYPE_RIGHT_TRIANGLE		3

/* flags for the primitives */
#define OVERLAY_FLAG_NOBLEND			0x10
#define OVERLAY_FLAG_MASK			(OVERLAY_FLAG_NOBLEND)

/* the tag assigned to all the internal overlays by default */
#define OVERLAY_TAG				"overlay"



/***************************************************************************

	Macros

	Usually called without a tag, but a tag can be used to overide,
	for example, if having a special override for cocktail mode.

OVERLAY_RECT: This is used for building a rectangular overlay.
The arguments are coordinates of the top left and bottom right.

(l,t)
     X********************
     *********************
     *********************
     ********************X
                          (r,b)

OVERLAY_DISK: This is used for building a disk (circle) overlay.
The arguments are coordinates used to build the shape with
(x,y) the centre point, and r the radius.


  |      *****
r |   ***********
  | ***************
  | *******X******* X = (x,y)
    ***************
      ***********
         *****

OVERLAY_TRI: This is used for building a right-angled triangle overlay.
The arguments are coordinates representing points of each
end of the longest side. The order is important as they note
whether to fill the upper or lower part of the line to form a
triangle. If the coordinates used are (x,y) and (z,w), then the
following cases are used to fill the triangle.

(x,y)                     (z,w)
     X                         X********
     ***                         *******
     *****                         *****
     *******                         ***
     ********X                         X
              (z,w)                     (x,y)

              (z,w)                     (x,y)
             X                 ********X
           ***                 *******
         *****                 *****
       *******                 ***
     X********                 X
(x,y)                     (z,w)

NOBLEND VARIANTS: These are used to draw without blending
with underlying colours.

TAG VARIANTS: These are used to change the overlay tag to
something else. This can be used to have different variants
of a hardcoded overlay for different situations (for example,
upright and cocktail modes.

***************************************************************************/

#define OVERLAY_START(name)	\
	static const struct overlay_piece name[] = {

#define OVERLAY_END \
	{ OVERLAY_TYPE_END } };

#define OVERLAY_RECT(l,t,r,b,c) \
	{ (char *) OVERLAY_TAG, OVERLAY_TYPE_RECTANGLE, (c), (l), (t), (r), (b) },

#define OVERLAY_DISK(x,y,r,c) \
	{ (char *) OVERLAY_TAG, OVERLAY_TYPE_DISK, (c), (x), (y), (r), 0 },

#define OVERLAY_DISK_NOBLEND(x,y,r,c) \
	{ (char *) OVERLAY_TAG, OVERLAY_TYPE_DISK | OVERLAY_FLAG_NOBLEND, (c), (x), (y), (r), 0 },

#define OVERLAY_TRI(x,y,z,w,c) \
	{ (char *) OVERLAY_TAG, OVERLAY_TYPE_RIGHT_TRIANGLE, (c), (x), (y), (z), (w) },

#define OVERLAY_TRI_NOBLEND(x,y,z,w,c) \
	{ (char *) OVERLAY_TAG, OVERLAY_TYPE_RIGHT_TRIANGLE | OVERLAY_FLAG_NOBLEND, (c), (x), (y), (z), (w) },

#define OVERLAY_RECT_TAG(tag, l,t,r,b,c) \
	{ (char *) (tag), OVERLAY_TYPE_RECTANGLE, (c), (l), (t), (r), (b) },

#define OVERLAY_DISK_TAG(tag, x,y,r,c) \
	{ (char *) (tag), OVERLAY_TYPE_DISK, (c), (x), (y), (r), 0 },

#define OVERLAY_DISK_NOBLEND_TAG(tag, x,y,r,c) \
	{ (char *) (tag), OVERLAY_TYPE_DISK | OVERLAY_FLAG_NOBLEND, (c), (x), (y), (r), 0 },

#define OVERLAY_TRI_TAG(tag,x,y,z,w,c) \
	{ (char *) (tag), OVERLAY_TYPE_RIGHT_TRIANGLE, (c), (x), (y), (z), (w) },

#define OVERLAY_TRI_NOBLEND_TAG(tag,x,y,z,w,c) \
	{ (char *) (tag), OVERLAY_TYPE_RIGHT_TRIANGLE | OVERLAY_FLAG_NOBLEND, (c), (x), (y), (z), (w) },


/***************************************************************************

	Type definitions

***************************************************************************/

struct artwork_callbacks
{
	/* provides an additional way to activate artwork system; can be NULL */
	int (*activate_artwork)(struct osd_create_params *params);

	/* function to load an artwork file for a particular driver */
	mame_file *(*load_artwork)(const struct GameDriver **driver);
};

struct overlay_piece
{
	char *tag; /* Usually set to OVERLAY_TAG */
	UINT8 type;
	rgb_t color;
	float left, top, right, bottom;
};



/***************************************************************************

	Prototypes

***************************************************************************/

int artwork_create_display(struct osd_create_params *params, UINT32 *rgb_components, const struct artwork_callbacks *callbacks);
void artwork_update_video_and_audio(struct mame_display *display);
void artwork_override_screenshot_params(struct mame_bitmap **bitmap, struct rectangle *rect, UINT32 *rgb_components);

struct mame_bitmap *artwork_get_ui_bitmap(void);
void artwork_mark_ui_dirty(int minx, int miny, int maxx, int maxy);
void artwork_get_screensize(int *width, int *height);
void artwork_enable(int enable);

void artwork_set_overlay(const struct overlay_piece *overlist);
void artwork_show(const char *tag, int show);

mame_file *artwork_load_artwork_file(const struct GameDriver **driver);

#endif /* ARTWORK_H */

