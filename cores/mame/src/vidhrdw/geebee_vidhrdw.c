/****************************************************************************
 *
 * geebee.c
 *
 * video driver
 * juergen buchmueller <pullmoll@t-online.de>, jan 2000
 *
 * TODO:
 * backdrop support for lamps? (player1, player2 and serve)
 * what is the counter output anyway?
 *
 ****************************************************************************/

#include "driver.h"
#include "artwork.h"
#include "vidhrdw/generic.h"

/* from machine/geebee.c */
extern int geebee_ball_h;
extern int geebee_ball_v;
extern int geebee_lamp1;
extern int geebee_lamp2;
extern int geebee_lamp3;
extern int geebee_counter;
extern int geebee_lock_out_coil;
extern int geebee_bgw;
extern int geebee_ball_on;
extern int geebee_inv;

#ifdef MAME_DEBUG
char geebee_msg[32+1];
int geebee_cnt;
#endif


static unsigned char geebee_palette[] =
{
	0x00,0x00,0x00, /* black */
	0xff,0xff,0xff, /* white */
	0x7f,0x7f,0x7f  /* grey  */
};

static unsigned short geebee_colortable[] =
{
	 0, 1,
	 0, 2,
	 1, 0,
	 2, 0
};

static unsigned short navalone_colortable[] =
{
	 0, 1,
	 0, 2,
	 0, 1,
	 0, 2
};

#define COCKTAIL_ONLY		"cocktail"
#define UPRIGHT_ONLY		"upright"

VIDEO_START( geebee )
{
	if( video_start_generic() )
		return 1;

	/* Change overlay based on mode */
	artwork_show(UPRIGHT_ONLY, (readinputport(2) & 0x01) == 0);
	artwork_show(COCKTAIL_ONLY, (readinputport(2) & 0x01) != 0);

	return 0;
}

VIDEO_START( navalone )
{
	if( video_start_generic() )
		return 1;

	/* Change overlay based on mode */
	artwork_show(UPRIGHT_ONLY, (readinputport(2) & 0x01) == 0);
	artwork_show(COCKTAIL_ONLY, (readinputport(2) & 0x01) != 0);

	return 0;
}

VIDEO_START( sos )
{
	if( video_start_generic() )
		return 1;

	return 0;
}

VIDEO_START( kaitei )
{
	if( video_start_generic() )
	return 1;

	/* Change overlay based on mode */
	artwork_show(COCKTAIL_ONLY, (readinputport(2) & 0x01) == 0);
	artwork_show(UPRIGHT_ONLY, (readinputport(2) & 0x01) != 0);

	return 0;
}

VIDEO_START( kaitein )
{
	if( video_start_generic() )
	return 1;

	/* No change in overlay, as this is always in cocktail mode */
	artwork_show(COCKTAIL_ONLY, true);
	artwork_show(UPRIGHT_ONLY, false);

	return 0;
}

/* Initialise the palette */
PALETTE_INIT( geebee )
{
	int i;
	for (i = 0; i < sizeof(geebee_palette)/3; i++)
		palette_set_color(i,geebee_palette[i*3+0],geebee_palette[i*3+1],geebee_palette[i*3+2]);
	memcpy(colortable, geebee_colortable, sizeof (geebee_colortable));
}

/* Initialise the palette */
PALETTE_INIT( navalone )
{
	int i;
	for (i = 0; i < sizeof(geebee_palette)/3; i++)
		palette_set_color(i,geebee_palette[i*3+0],geebee_palette[i*3+1],geebee_palette[i*3+2]);
	memcpy(colortable, navalone_colortable, sizeof (navalone_colortable));
}


static INLINE void geebee_plot(struct mame_bitmap *bitmap, const struct rectangle *cliprect, int x, int y)
{
	if (x >= cliprect->min_x && x <= cliprect->max_x && y >= cliprect->min_y && y <= cliprect->max_y)
		plot_pixel(bitmap,x,y,Machine->pens[1]);
}

VIDEO_UPDATE( geebee )
{
	int offs;

#ifdef MAME_DEBUG
	if( geebee_cnt > 0 )
	{
		ui_text(Machine->scrbitmap, geebee_msg, Machine->visible_area.min_y, Machine->visible_area.max_x - 8);
    }
#endif

	if (get_vh_global_attribute_changed())
        memset(dirtybuffer, 1, videoram_size);

	for( offs = 0; offs < videoram_size; offs++ )
	{
		if( dirtybuffer[offs] )
		{
			int mx,my,sx,sy,code,color;

			dirtybuffer[offs] = 0;

			mx = offs % 32;
			my = offs / 32;

			if (my == 0)
			{
				sx = 8*33;
				sy = 8*mx;
			}
			else if (my == 1)
			{
				sx = 0;
				sy = 8*mx;
			}
			else
			{
				sx = 8*(mx+1);
				sy = 8*my;
			}

			if (geebee_inv)
			{
				sx = 33*8 - sx;
				sy = 31*8 - sy;
			}

			code = videoram[offs];
			color = ((geebee_bgw & 1) << 1) | ((code & 0x80) >> 7);
			drawgfx(tmpbitmap,Machine->gfx[0],
					code,color,
					geebee_inv,geebee_inv,sx,sy,
					&Machine->visible_area,TRANSPARENCY_NONE,0);
		}
	}
	copybitmap(bitmap,tmpbitmap,0,0,0,0,cliprect,TRANSPARENCY_NONE,0);

	if( geebee_ball_on )
	{
		int x, y;

		for( y = 0; y < 4; y++ )
			for( x = 0; x < 4; x++ )
				geebee_plot(bitmap,cliprect,geebee_ball_h+x+5,geebee_ball_v+y-2);
	}
}
