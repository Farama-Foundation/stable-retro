/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

  CHANGES:
  MAB 05 MAR 99 - changed overlay support to use artwork functions
  AAT 12 MAY 02 - rewrote Ripcord and added pixel-wise collision

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "artwork.h"

static int clown_x=0,clown_y=0,clown_z=0;

static struct tilemap *bg_tilemap;

/***************************************************************************
***************************************************************************/

static const char *circus_sample_names[] =
{
	"*circus",
	"pop.wav",
	"miss.wav",
	"bounce.wav",
	0       /* end of array */
};

struct Samplesinterface circus_samples_interface =
{
	3,	/* 3 channels */
	25,	/* volume */
	circus_sample_names
};

WRITE_HANDLER( circus_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

WRITE_HANDLER( circus_clown_x_w )
{
	clown_x = 240-data;
}

WRITE_HANDLER( circus_clown_y_w )
{
	clown_y = 240-data;
}

/* This register controls the clown image currently displayed */
/* and also is used to enable the amplifier and trigger the   */
/* discrete circuitry that produces sound effects and music   */

WRITE_HANDLER( circus_clown_z_w )
{
	clown_z = (data & 0x0f);
*(memory_region(REGION_CPU1)+0x8000)=data; log_cb(RETRO_LOG_DEBUG, LOGPRE "Z:%02x\n",data); /*DEBUG*/
	/* Bits 4-6 enable/disable trigger different events */
	/* descriptions are based on Circus schematics      */

	switch ((data & 0x70) >> 4)
	{
		case 0 : /* All Off */
			DAC_data_w (0,0);
			break;

		case 1 : /* Music */
			DAC_data_w (0,0x7f);
			break;

		case 2 : /* Pop */
			sample_start (0, 0, 0);
			break;

		case 3 : /* Normal Video */
			break;

		case 4 : /* Miss */
			sample_start (1, 1, 0);
			break;

		case 5 : /* Invert Video */
			break;

		case 6 : /* Bounce */
			sample_start (2, 2, 0);
			break;

		case 7 : /* Don't Know */
			break;
	};

	/* Bit 7 enables amplifier (1 = on) */

/*  log_cb(RETRO_LOG_DEBUG, LOGPRE "clown Z = %02x\n",data);*/
}

static void get_bg_tile_info(int tile_index)
{
	int code = videoram[tile_index];

	SET_TILE_INFO(0, code, 0, 0)
}

VIDEO_START( circus )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows,
		TILEMAP_OPAQUE, 8, 8, 32, 32);

	if ( !bg_tilemap )
		return 1;

	return 0;
}

static void draw_line(struct mame_bitmap *bitmap, int x1, int y1, int x2, int y2, int dotted)
{
	/* Draws horizontal and Vertical lines only! */
	int col = Machine->pens[1];

	int count, skip;

	/* Draw the Line */

	if (dotted > 0)
		skip = 2;
	else
		skip = 1;

	if (x1 == x2)
	{
		for (count = y2; count >= y1; count -= skip)
		{
			plot_pixel(bitmap, x1, count, col);
		}
	}
	else
	{
		for (count = x2; count >= x1; count -= skip)
		{
			plot_pixel(bitmap, count, y1, col);
		}
	}
}

static void draw_robot_box (struct mame_bitmap *bitmap, int x, int y)
{
	/* Box */

	int ex = x + 24;
	int ey = y + 26;

	draw_line(bitmap,x,y,ex,y,0);       /* Top */
	draw_line(bitmap,x,ey,ex,ey,0);     /* Bottom */
	draw_line(bitmap,x,y,x,ey,0);       /* Left */
	draw_line(bitmap,ex,y,ex,ey,0);     /* Right */

	/* Score Grid */

	ey = y + 10;
	draw_line(bitmap,x+8,ey,ex,ey,0);   /* Horizontal Divide Line */
	draw_line(bitmap,x+8,y,x+8,ey,0);
	draw_line(bitmap,x+16,y,x+16,ey,0);
}

static void circus_draw_fg( struct mame_bitmap *bitmap )
{
	/* The sync generator hardware is used to   */
	/* draw the border and diving boards        */

	draw_line (bitmap,0,18,255,18,0);
	draw_line (bitmap,0,249,255,249,1);
	draw_line (bitmap,0,18,0,248,0);
	draw_line (bitmap,247,18,247,248,0);

	draw_line (bitmap,0,137,17,137,0);
	draw_line (bitmap,231,137,248,137,0);
	draw_line (bitmap,0,193,17,193,0);
	draw_line (bitmap,231,193,248,193,0);

	drawgfx(bitmap,Machine->gfx[1],
			clown_z,
			0,
			0,0,
			clown_y,clown_x,
			&Machine->visible_area,TRANSPARENCY_PEN,0);
}

VIDEO_UPDATE( circus )
{
	tilemap_draw(bitmap, &Machine->visible_area, bg_tilemap, 0, 0);
	circus_draw_fg(bitmap);
}

static void robotbwl_draw_scoreboard( struct mame_bitmap *bitmap )
{
	int offs;

	/* The sync generator hardware is used to   */
	/* draw the bowling alley & scorecards      */

	for(offs=15;offs<=63;offs+=24)
	{
		draw_robot_box(bitmap, offs, 31);
		draw_robot_box(bitmap, offs, 63);
		draw_robot_box(bitmap, offs, 95);

		draw_robot_box(bitmap, offs+152, 31);
		draw_robot_box(bitmap, offs+152, 63);
		draw_robot_box(bitmap, offs+152, 95);
	}

	draw_robot_box(bitmap, 39, 127);                  /* 10th Frame */
	draw_line(bitmap, 39,137,47,137,0);          /* Extra digit box */

	draw_robot_box(bitmap, 39+152, 127);
	draw_line(bitmap, 39+152,137,47+152,137,0);
}

static void robotbwl_draw_bowling_alley( struct mame_bitmap *bitmap )
{
	draw_line(bitmap, 103,17,103,205,0);
	draw_line(bitmap, 111,17,111,203,1);
	draw_line(bitmap, 152,17,152,205,0);
	draw_line(bitmap, 144,17,144,203,1);
}

static void robotbwl_draw_ball( struct mame_bitmap *bitmap )
{
	drawgfx(bitmap,Machine->gfx[1],
			clown_z,
			0,
			0,0,
			clown_y+8,clown_x+8, /* Y is horizontal position */
			&Machine->visible_area,TRANSPARENCY_PEN,0);
}

VIDEO_UPDATE( robotbwl )
{
	tilemap_draw(bitmap, &Machine->visible_area, bg_tilemap, 0, 0);
	robotbwl_draw_scoreboard(bitmap);
	robotbwl_draw_bowling_alley(bitmap);
	robotbwl_draw_ball(bitmap);
}

static void crash_draw_car( struct mame_bitmap *bitmap )
{
	drawgfx(bitmap,Machine->gfx[1],
		clown_z,
		0,
		0,0,
		clown_y,clown_x, /* Y is horizontal position */
		&Machine->visible_area,TRANSPARENCY_PEN,0);
}

VIDEO_UPDATE( crash )
{
	tilemap_draw(bitmap, &Machine->visible_area, bg_tilemap, 0, 0);
	crash_draw_car(bitmap);
}

static void ripcord_draw_skydiver( struct mame_bitmap *bitmap )
{
	const struct GfxElement *gfx;
	const struct rectangle *clip;
	pen_t *pal_ptr;
	UINT8  *src_lineptr, *src_pixptr;
	UINT16 *dst_lineptr, *dst_lineend;
	unsigned int code, color;
	int sx, sy;
	int src_pitch, dst_width, dst_height, dst_pitch, dst_pixoffs, dst_pixend;
	int collision, eax, edx;

	gfx = Machine->gfx[0];
	clip = &Machine->visible_area;

	code = clown_z;
	color = 0;

	sx = clown_y;
	sy = clown_x - 1;
	dst_width = 16;
	dst_height = 16;
	edx = 1;

	gfx = Machine->gfx[1];
	pal_ptr = gfx->colortable + color * gfx->color_granularity;
	src_lineptr = gfx->gfxdata + code * gfx->char_modulo;
	src_pitch = gfx->line_modulo;
	dst_pitch = bitmap->rowpixels;

	if (Machine->orientation & ORIENTATION_FLIP_X)
	{
		sx = bitmap->width - sx;
		dst_width = -dst_width;
		edx = -edx;
	}

	if (Machine->orientation & ORIENTATION_FLIP_Y)
	{
		sy = bitmap->height - sy;
		dst_pitch = -dst_pitch;
	}

	dst_lineptr = (UINT16*)bitmap->line[sy];
	dst_pixend = (sx + dst_width) & 0xff;
	dst_lineend = dst_lineptr + dst_pitch * dst_height;

	/* draw sky diver and check collision on a pixel basis*/
	collision = 0;
	do
	{
		src_pixptr = src_lineptr;
		dst_pixoffs = sx;

		do
		{
			eax = *src_pixptr;
			src_pixptr ++;
			if (eax)
			{
				eax = pal_ptr[eax];
				collision |= dst_lineptr[dst_pixoffs];
				dst_lineptr[dst_pixoffs] = eax;
			}
			dst_pixoffs += edx;

		} while((dst_pixoffs &= 0xff) != dst_pixend);

		src_lineptr += src_pitch;

	} while((dst_lineptr += dst_pitch) != dst_lineend);

	/* report collision only when the character is not blank and within display area*/
	if (collision && code!=0xf && clown_x>0 && clown_x<240 && clown_y>-12 && clown_y<240)
	{
		cpu_set_irq_line(0, 0, ASSERT_LINE); /* interrupt accuracy is critical in Ripcord*/
		cpu_set_irq_line(0, 0, CLEAR_LINE);
	}
}

VIDEO_UPDATE( ripcord )
{
	tilemap_draw(bitmap, &Machine->visible_area, bg_tilemap, 0, 0);
	ripcord_draw_skydiver(bitmap);
}
