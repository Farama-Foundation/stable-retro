/***************************************************************************

	Cinematronics vector hardware

***************************************************************************/

#include "driver.h"
#include "vector.h"
#include "cpu/ccpu/ccpu.h"
#include "cinemat.h"

#define RED   0x04
#define GREEN 0x02
#define BLUE  0x01
#define WHITE RED|GREEN|BLUE

static UINT8 color_display;
static int cinemat_screenh;

void CinemaVectorData(int fromx, int fromy, int tox, int toy, int color)
{
    static int lastx, lasty;

	fromy = cinemat_screenh - fromy;
	toy = cinemat_screenh - toy;

	if (fromx != lastx || fromx != lasty)
		vector_add_point (fromx << 16, fromy << 16, 0, 0);

    if (color_display)
        vector_add_point (tox << 16, toy << 16, VECTOR_COLOR111(color & 0x07), color & 0x08 ? 0x80: 0x40);
    else
        vector_add_point (tox << 16, toy << 16, VECTOR_COLOR111(WHITE), color * 12);

	lastx = tox;
	lasty = toy;
}

PALETTE_INIT( cinemat )
{
    color_display = 0;
}


PALETTE_INIT( cinemat_color )
{
    color_display = 1;
}


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

VIDEO_START( cinemat )
{
	cinemat_screenh = Machine->visible_area.max_y - Machine->visible_area.min_y;
	return video_start_vector();
}


VIDEO_EOF( cinemat )
{
	vector_clear_list();
}


VIDEO_UPDATE( spacewar )
{
	int sw_option = readinputport(1);

	video_update_vector(bitmap, 0);

	/* set the state of the artwork */
	artwork_show("pressed3", (~sw_option >> 0) & 1);
	artwork_show("pressed8", (~sw_option >> 1) & 1);
	artwork_show("pressed4", (~sw_option >> 2) & 1);
	artwork_show("pressed9", (~sw_option >> 3) & 1);
	artwork_show("pressed1", (~sw_option >> 4) & 1);
	artwork_show("pressed6", (~sw_option >> 5) & 1);
	artwork_show("pressed2", (~sw_option >> 6) & 1);
	artwork_show("pressed7", (~sw_option >> 7) & 1);
	artwork_show("pressed5", (~sw_option >> 10) & 1);
	artwork_show("pressed0", (~sw_option >> 11) & 1);
}
