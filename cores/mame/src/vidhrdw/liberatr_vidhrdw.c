/***************************************************************************

  liberator.c - 'vidhrdw.c'

  Functions to emulate the video hardware of the machine.

   Liberator's screen is 256 pixels by 256 pixels.  The
     round planet in the middle of the screen is 128 pixels
     tall by 96 equivalent (192 at double pixel rate).  The
     emulator needs to account for the aspect ratio of 4/3
     from the arcade video system in order to make the planet
     appear round.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "liberatr.h"


UINT8 *liberatr_bitmapram;


/*
	The following structure describes the (up to 32) line segments
	that make up one horizontal line (latitude) for one display frame of the planet.
	Note: this and the following structure is only used to collect the
	data before it is packed for actual use.
*/
typedef struct
{
	UINT8	segment_count;		/* the number of segments on this line */
	UINT8	max_x;				/* the maximum value of x_array for this line */
	UINT8	color_array[32];	/* the color values  */
	UINT8	x_array[32];		/* and maximum x values for each segment  */
} Liberator_Segs;

/*
	The following structure describes the lines (latitudes)
	that make up one complete display frame of the planet.
	Note: this and the previous structure is only used to collect the
	data before it is packed for actual use.
*/
typedef struct
{
	Liberator_Segs line[ 0x80 ];
} Liberator_Frame;


/*
	The following structure collects the 256 frames of the
	planet (one per value of longitude).
	The data is packed segment_count,segment_start,color,length,color,length,...  then
                       segment_count,segment_start,color,length,color,length...  for the next line, etc
	for the 128 lines.
*/
typedef struct
{
	UINT8 *frame[256];
} Liberator_Planet;


UINT8 *liberatr_base_ram;
UINT8 *liberatr_planet_frame;
UINT8 *liberatr_planet_select;
UINT8 *liberatr_x;
UINT8 *liberatr_y;

static UINT8 *liberatr_videoram;

/*
	The following array collects the 2 different planet
	descriptions, which are selected by liberatr_planetbit
*/
static Liberator_Planet *liberatr_planet_segs[2];


static INLINE void bitmap_common_w(UINT8 x, UINT8 y, int data)
{
	int pen;

	liberatr_videoram[(y<<8) | x] = data & 0xe0;

	pen = Machine->pens[(data >> 5) + 0x10];
	plot_pixel(tmpbitmap, x, y, pen);
}

WRITE_HANDLER( liberatr_bitmap_xy_w )
{
	bitmap_common_w(*liberatr_x, *liberatr_y, data);
}

WRITE_HANDLER( liberatr_bitmap_w )
{
	UINT8 x = (offset & 0x3f) << 2;
	UINT8 y = (offset >> 6);

	liberatr_bitmapram[offset] = data;

    bitmap_common_w(x , y, data);
    bitmap_common_w(x+1, y, data);
    bitmap_common_w(x+2, y, data);
    bitmap_common_w(x+3, y, data);
}


READ_HANDLER( liberatr_bitmap_xy_r )
{
	return liberatr_videoram[((*liberatr_y)<<8) | (*liberatr_x)];
}


WRITE_HANDLER( liberatr_colorram_w )
{
	UINT8 r,g,b;

	/* handle the hardware flip of the bit order from 765 to 576 that
	   hardware does between vram and color ram */
	static UINT8 penmap[] = {0x10,0x12,0x14,0x16,0x11,0x13,0x15,0x17};


	/* scale it from 0x00-0xff */
	r = ((~data >> 3) & 0x07) * 0x24 + 3;  if (r == 3)  r = 0;
	g = ((~data     ) & 0x07) * 0x24 + 3;  if (g == 3)  g = 0;
	b = ((~data >> 5) & 0x06) * 0x24 + 3;  if (b == 3)  b = 0;

	if (offset & 0x10)
	{
		/* bitmap colorram values */
		offset = penmap[offset & 0x07];
 	}
	else
	{
		offset ^= 0x0f;
	}

	palette_set_color(offset,r,g,b);
}


/********************************************************************************************
  liberatr_init_planet()

  The data for the planet is stored in ROM using a run-length type of encoding.  This
  function does the conversion to the above structures and then a smaller
  structure which is quicker to use in real time.

  Its a multi-step process, reflecting the history of the code.  Not quite as efficient
  as it might be, but this is not realtime stuff, so who cares...
 ********************************************************************************************/
static int liberatr_init_planet(int planet_select)
{
	UINT16 longitude;
	UINT8 *planet_rom;

	const UINT8* latitude_scale = memory_region(REGION_USER1);
	const UINT8* longitude_scale = memory_region(REGION_USER2);

	planet_rom = memory_region(REGION_GFX1);

	/* for each starting longitude */
	for (longitude = 0; longitude < 0x100; longitude++)
	{
		UINT8  i, latitude, start_segment, segment_count;
		UINT16 total_segment_count;
		UINT8  *buffer;
		Liberator_Frame frame;
		Liberator_Segs *line = 0;


		total_segment_count = 0;

		/* for each latitude */
		for (latitude = 0; latitude < 0x80; latitude++)
		{
			UINT8 segment, longitude_scale_factor, latitude_scale_factor, color, x=0;
			UINT8 x_array[32], color_array[32], visible_array[32];


			/* point to the structure which will hold the data for this line */
			line = &frame.line[ latitude ];

			latitude_scale_factor = latitude_scale[ latitude ];

			/* for this latitude, load the 32 segments into the arrays */
			for (segment = 0; segment < 0x20; segment++)
			{
				UINT16 length, planet_data, address;


				/*
				   read the planet picture ROM and get the
				   latitude and longitude scaled from the scaling PROMS
				*/
				address = (latitude << 5) + segment;
				if (planet_select)
					planet_data = (planet_rom[0x0000+address] << 8) + planet_rom[0x1000+address];
				else
					planet_data = (planet_rom[0x2000+address] << 8) + planet_rom[0x3000+address];

				color  =  (planet_data >> 8) & 0x0f;
				length = ((planet_data << 1) & 0x1fe) + ((planet_data >> 15) & 0x01);


				/* scale the longitude limit (adding the starting longitude) */
				address = longitude + ( length >> 1 ) + ( length & 1 );		/* shift with rounding */
				visible_array[segment] = (( address & 0x100 ) ? 1 : 0);
				if (address & 0x80)
				{
					longitude_scale_factor = 0xff;
				}
				else
				{
					address = ((address & 0x7f) << 1) + (((length & 1) || visible_array[segment]) ? 0 : 1);
					longitude_scale_factor = longitude_scale[ address ];
				}

				x_array[segment] = (((UINT16)latitude_scale_factor * (UINT16)longitude_scale_factor) + 0x80) >> 8;	/* round it */
				color_array[segment] = color;
			}

			/*
			   determine which segment is the western horizon and
			     leave 'segment' indexing it.
			*/
			for (segment = 0; segment < 0x1f; segment++)	/* if not found, 'segment' = 0x1f */
				if (visible_array[segment]) break;

			/* transfer from the temporary arrays to the structure */
			line->max_x = (latitude_scale_factor * 0xc0) >> 8;
			if (line->max_x & 1)
				line->max_x += 1; 				/* make it even */

			/*
			   as part of the quest to reduce memory usage (and to a lesser degree
			     execution time), stitch together segments that have the same color
			*/
			segment_count = 0;
			i = 0;
			start_segment = segment;
			do
			{
				color = color_array[segment];
				while (color == color_array[segment])
				{
					x = x_array[segment];
					segment = (segment+1) & 0x1f;
					if (segment == start_segment)
						break;
				}
				line->color_array[ i ] = color;
				line->x_array[ i ]     = (x > line->max_x) ? line->max_x : x;
				i++;
				segment_count++;
			} while ((i < 32) && (x <= line->max_x));

			total_segment_count += segment_count;
			line->segment_count = segment_count;
		}

		/* now that the all the lines have been processed, and we know how
		   many segments it will take to store the description, allocate the
		   space for it and copy the data to it.
		*/
		if ((buffer = (UINT8 *)auto_malloc(2*(128 + total_segment_count))) == 0)
			return 1;

		liberatr_planet_segs[ planet_select ]->frame[ longitude ] = buffer;

		for (latitude = 0; latitude < 0x80; latitude++)
		{
			UINT8 last_x;


			line = &frame.line[ latitude ];
			segment_count = line->segment_count;
			*buffer++ = segment_count;
			last_x = 0;

			/* calculate the bitmap's x coordinate for the western horizon
			   center of bitmap - (the number of planet pixels) / 4 */
			*buffer++ = Machine->drv->screen_width/2 - (line->max_x + 2) / 4;

			for (i = 0; i < segment_count; i++)
			{
				UINT8 current_x = (line->x_array[ i ] + 1) / 2;
				*buffer++ = line->color_array[ i ];
				*buffer++ = current_x - last_x;
				last_x = current_x;
			}
		}
	}

	return 0;
}


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

VIDEO_START( liberatr )
{
    liberatr_videoram = 0;
    liberatr_planet_segs[0] = 0;
    liberatr_planet_segs[1] = 0;

    /* allocate a tmpbitmap */
    if ((tmpbitmap = auto_bitmap_alloc(Machine->drv->screen_width, Machine->drv->screen_height)) == 0)
    	return 1;

	if ((liberatr_videoram = auto_malloc(Machine->drv->screen_width * Machine->drv->screen_height)) == 0)
		return 1;

	/* allocate the planet descriptor structure */
	if (((liberatr_planet_segs[0] = auto_malloc(sizeof(Liberator_Planet))) == 0) ||
	    ((liberatr_planet_segs[1] = auto_malloc(sizeof(Liberator_Planet))) == 0))
		return 1;

	memset(liberatr_planet_segs[0], 0, sizeof(Liberator_Planet));
	memset(liberatr_planet_segs[1], 0, sizeof(Liberator_Planet));

	/* for each planet in the planet ROMs */
	if (liberatr_init_planet(0) || liberatr_init_planet(1))
		return 1;

	return 0;
}


/***************************************************************************

  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/

static void liberatr_draw_planet(struct mame_bitmap *bitmap)
{
	UINT8 latitude;
	UINT8 *buffer;


	buffer = liberatr_planet_segs[ (*liberatr_planet_select >> 4) & 0x01 ]->frame[ *liberatr_planet_frame ];

	/* for each latitude */
	for (latitude = 0; latitude < 0x80; latitude++)
	{
		UINT8 base_color, segment, segment_count, x, y;

		/* grab the color value for the base (if any) at this latitude */
		base_color = liberatr_base_ram[latitude>>3] ^ 0x0f;

		segment_count = *buffer++;
		x             = *buffer++;
		y             = 64 + latitude;

		/* run through the segments, drawing its color until its x_array value comes up. */
		for (segment = 0; segment < segment_count; segment++)
		{
			UINT8 color, segment_length, i;
			UINT16 pen;

			color = *buffer++;
			if ((color & 0x0c) == 0x0c)
				color = base_color;

			pen = Machine->pens[color];

			segment_length = *buffer++;

			for (i = 0; i < segment_length; i++, x++)
			{
				/* only plot pixels that don't cover the foreground up */
				if (!liberatr_videoram[(y<<8) | x])
					plot_pixel(bitmap, x, y, pen);
			}
		}
	}
}


VIDEO_UPDATE( liberatr )
{
	if (get_vh_global_attribute_changed())
	{
		UINT8 liberatr_y_save = *liberatr_y;
		UINT8 liberatr_x_save = *liberatr_x;

		/* redraw bitmap */
		for (*liberatr_y = Machine->visible_area.min_y; *liberatr_y < Machine->visible_area.max_y; (*liberatr_y)++)
		{
			for (*liberatr_x = Machine->visible_area.min_x; *liberatr_x < Machine->visible_area.max_x; (*liberatr_x)++)
			{
				liberatr_bitmap_xy_w(0, liberatr_bitmap_xy_r(0));
			}
		}

		*liberatr_y = liberatr_y_save;
		*liberatr_x = liberatr_x_save;
	}
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);

	/* draw the planet */
	liberatr_draw_planet(bitmap);
}
