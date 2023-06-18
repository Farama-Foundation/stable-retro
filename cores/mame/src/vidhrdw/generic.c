/***************************************************************************

  vidhrdw/generic.c

  Some general purpose functions used by many video drivers.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "state.h"


data8_t *videoram;
data16_t *videoram16;
data32_t *videoram32;
size_t videoram_size;
data8_t *colorram;
data16_t *colorram16;
data32_t *colorram32;
data8_t *spriteram;			/* not used in this module... */
data16_t *spriteram16;		/* ... */
data32_t *spriteram32;		/* ... */
data8_t *spriteram_2;
data16_t *spriteram16_2;
data32_t *spriteram32_2;
data8_t *spriteram_3;
data16_t *spriteram16_3;
data32_t *spriteram32_3;
data8_t *buffered_spriteram;
data16_t *buffered_spriteram16;
data32_t *buffered_spriteram32;
data8_t *buffered_spriteram_2;
data16_t *buffered_spriteram16_2;
data32_t *buffered_spriteram32_2;
size_t spriteram_size;		/* ... here just for convenience */
size_t spriteram_2_size;
size_t spriteram_3_size;
data8_t *dirtybuffer;
data16_t *dirtybuffer16;
data32_t *dirtybuffer32;
struct mame_bitmap *tmpbitmap;

int flip_screen_x, flip_screen_y;
static int global_attribute_changed;

void video_generic_postload(void)
{
	memset(dirtybuffer,1,videoram_size);
}

/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
VIDEO_START( generic )
{
	dirtybuffer = 0;
	tmpbitmap = 0;

	if (videoram_size == 0)
	{
    log_cb(RETRO_LOG_DEBUG, LOGPRE "Error: video_start_generic() called but videoram_size not initialized\n");
		return 1;
	}

	if ((dirtybuffer = auto_malloc(videoram_size)) == 0)
		return 1;
	memset(dirtybuffer,1,videoram_size);

	if ((tmpbitmap = auto_bitmap_alloc(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
		return 1;

	state_save_register_func_postload(video_generic_postload);

	return 0;
}


VIDEO_START( generic_bitmapped )
{
	if ((tmpbitmap = auto_bitmap_alloc(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
		return 1;

	return 0;
}


/***************************************************************************

  Draw the game screen in the given mame_bitmap.
  To be used by bitmapped games not using sprites.

***************************************************************************/
VIDEO_UPDATE( generic_bitmapped )
{
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);
}


READ_HANDLER( videoram_r )
{
	return videoram[offset];
}

READ_HANDLER( colorram_r )
{
	return colorram[offset];
}

WRITE_HANDLER( videoram_w )
{
	if (videoram[offset] != data)
	{
		dirtybuffer[offset] = 1;

		videoram[offset] = data;
	}
}

WRITE_HANDLER( colorram_w )
{
	if (colorram[offset] != data)
	{
		dirtybuffer[offset] = 1;

		colorram[offset] = data;
	}
}



READ_HANDLER( spriteram_r )
{
	return spriteram[offset];
}

WRITE_HANDLER( spriteram_w )
{
	spriteram[offset] = data;
}

READ16_HANDLER( spriteram16_r )
{
	return spriteram16[offset];
}

WRITE16_HANDLER( spriteram16_w )
{
	COMBINE_DATA(spriteram16+offset);
}

READ_HANDLER( spriteram_2_r )
{
	return spriteram_2[offset];
}

WRITE_HANDLER( spriteram_2_w )
{
	spriteram_2[offset] = data;
}

/* Mish:  171099

	'Buffered spriteram' is where the graphics hardware draws the sprites
from private ram that the main CPU cannot access.  The main CPU typically
prepares sprites for the next frame in it's own sprite ram as the graphics
hardware renders sprites for the current frame from private ram.  Main CPU
sprite ram is usually copied across to private ram by setting some flag
in the VBL interrupt routine.

	The reason for this is to avoid sprite flicker or lag - if a game
is unable to prepare sprite ram within a frame (for example, lots of sprites
on screen) then it doesn't trigger the buffering hardware - instead the
graphics hardware will use the sprites from the last frame. An example is
Dark Seal - the buffer flag is only written to if the CPU is idle at the time
of the VBL interrupt.  If the buffering is not emulated the sprites flicker
at busy scenes.

	Some games seem to use buffering because of hardware constraints -
Capcom games (Cps1, Last Duel, etc) render spriteram _1 frame ahead_ and
buffer this spriteram at the end of a frame, so the _next_ frame must be drawn
from the buffer.  Presumably the graphics hardware and the main cpu cannot
share the same spriteram for whatever reason.

	Sprite buffering & Mame:

	To use sprite buffering in a driver use VIDEO_BUFFERS_SPRITERAM in the
machine driver.  This will automatically create an area for buffered spriteram
equal to the size of normal spriteram.

	Spriteram size _must_ be declared in the memory map:

	{ 0x120000, 0x1207ff, MWA_BANK2, &spriteram, &spriteram_size },

	Then the video driver must draw the sprites from the buffered_spriteram
pointer.  The function buffer_spriteram_w() is used to simulate hardware
which buffers the spriteram from a memory location write.  The function
buffer_spriteram(unsigned char *ptr, int length) can be used where
more control is needed over what is buffered.

	For examples see darkseal.c, contra.c, lastduel.c, bionicc.c etc.

*/

WRITE_HANDLER( buffer_spriteram_w )
{
	memcpy(buffered_spriteram,spriteram,spriteram_size);
}

WRITE16_HANDLER( buffer_spriteram16_w )
{
	memcpy(buffered_spriteram16,spriteram16,spriteram_size);
}

WRITE32_HANDLER( buffer_spriteram32_w )
{
	memcpy(buffered_spriteram32,spriteram32,spriteram_size);
}

WRITE_HANDLER( buffer_spriteram_2_w )
{
	memcpy(buffered_spriteram_2,spriteram_2,spriteram_2_size);
}

WRITE16_HANDLER( buffer_spriteram16_2_w )
{
	memcpy(buffered_spriteram16_2,spriteram16_2,spriteram_2_size);
}

WRITE32_HANDLER( buffer_spriteram32_2_w )
{
	memcpy(buffered_spriteram32_2,spriteram32_2,spriteram_2_size);
}

void buffer_spriteram(unsigned char *ptr,int length)
{
	memcpy(buffered_spriteram,ptr,length);
}

void buffer_spriteram_2(unsigned char *ptr,int length)
{
	memcpy(buffered_spriteram_2,ptr,length);
}


/***************************************************************************

	Global video attribute handling code

***************************************************************************/

/*-------------------------------------------------
	updateflip - handle global flipping
-------------------------------------------------*/

static void updateflip(void)
{
	int min_x,max_x,min_y,max_y;

	tilemap_set_flip(ALL_TILEMAPS,(TILEMAP_FLIPX & flip_screen_x) | (TILEMAP_FLIPY & flip_screen_y));

	min_x = Machine->drv->default_visible_area.min_x;
	max_x = Machine->drv->default_visible_area.max_x;
	min_y = Machine->drv->default_visible_area.min_y;
	max_y = Machine->drv->default_visible_area.max_y;

	if (flip_screen_x)
	{
		int temp;

		temp = Machine->drv->screen_width - min_x - 1;
		min_x = Machine->drv->screen_width - max_x - 1;
		max_x = temp;
	}
	if (flip_screen_y)
	{
		int temp;

		temp = Machine->drv->screen_height - min_y - 1;
		min_y = Machine->drv->screen_height - max_y - 1;
		max_y = temp;
	}

	set_visible_area(min_x,max_x,min_y,max_y);
}


/*-------------------------------------------------
	flip_screen_set - set global flip
-------------------------------------------------*/

void flip_screen_set(int on)
{
	flip_screen_x_set(on);
	flip_screen_y_set(on);
}


/*-------------------------------------------------
	flip_screen_x_set - set global horizontal flip
-------------------------------------------------*/

void flip_screen_x_set(int on)
{
	if (on) on = ~0;
	if (flip_screen_x != on)
	{
		set_vh_global_attribute(&flip_screen_x,on);
		updateflip();
	}
}


/*-------------------------------------------------
	flip_screen_y_set - set global vertical flip
-------------------------------------------------*/

void flip_screen_y_set(int on)
{
	if (on) on = ~0;
	if (flip_screen_y != on)
	{
		set_vh_global_attribute(&flip_screen_y,on);
		updateflip();
	}
}


/*-------------------------------------------------
	set_vh_global_attribute - set an arbitrary
	global video attribute
-------------------------------------------------*/

void set_vh_global_attribute( int *addr, int data )
{
	if (!addr || *addr != data)
	{
		global_attribute_changed = 1;
		if (addr)
			*addr = data;
	}
}


/*-------------------------------------------------
	get_vh_global_attribute - set an arbitrary
	global video attribute
-------------------------------------------------*/

int get_vh_global_attribute_changed(void)
{
	int result = global_attribute_changed;
	global_attribute_changed = 0;
	return result;
}
