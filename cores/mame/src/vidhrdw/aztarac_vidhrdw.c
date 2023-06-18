/*************************************************************************

	Centuri Aztarac hardware

*************************************************************************/

#include "driver.h"
#include "vidhrdw/vector.h"
#include "aztarac.h"

#define AVECTOR(x, y, color, intensity) \
vector_add_point (xcenter + ((x) << 16), ycenter - ((y) << 16), color, intensity)

data16_t *aztarac_vectorram;

static int xcenter, ycenter;

static INLINE void read_vectorram (int addr, int *x, int *y, int *c)
{
    *c = aztarac_vectorram[addr] & 0xffff;
    *x = aztarac_vectorram[addr + 0x800] & 0x03ff;
    *y = aztarac_vectorram[addr + 0x1000] & 0x03ff;
    if (*x & 0x200) *x |= 0xfffffc00;
    if (*y & 0x200) *y |= 0xfffffc00;
}

WRITE16_HANDLER( aztarac_ubr_w )
{
    int x, y, c, intensity, xoffset, yoffset, color;
    int defaddr, objaddr=0, ndefs;

    if (data) /* data is the global intensity (always 0xff in Aztarac). */
    {
        vector_clear_list();

        while (1)
        {
            read_vectorram (objaddr, &xoffset, &yoffset, &c);
            objaddr++;

            if (c & 0x4000)
                break;

            if ((c & 0x2000) == 0)
            {
                defaddr = (c >> 1) & 0x7ff;
                AVECTOR (xoffset, yoffset, 0, 0);

                read_vectorram (defaddr, &x, &ndefs, &c);
				ndefs++;

                if (c & 0xff00)
                {
                    /* latch color only once */
                    intensity = (c >> 8);
					color = VECTOR_COLOR222(c & 0x3f);
                    while (ndefs--)
                    {
                        defaddr++;
                        read_vectorram (defaddr, &x, &y, &c);
                        if ((c & 0xff00) == 0)
                            AVECTOR (x + xoffset, y + yoffset, 0, 0);
                        else
                            AVECTOR (x + xoffset, y + yoffset, color, intensity);
                    }
                }
                else
                {
                    /* latch color for every definition */
                    while (ndefs--)
                    {
                        defaddr++;
                        read_vectorram (defaddr, &x, &y, &c);
						color = VECTOR_COLOR222(c & 0x3f);
                        AVECTOR (x + xoffset, y + yoffset, color, c >> 8);
                    }
                }
            }
        }
    }
}


VIDEO_START( aztarac )
{
	int xmin = Machine->visible_area.min_x;
	int ymin = Machine->visible_area.min_y;
	int xmax = Machine->visible_area.max_x;
	int ymax = Machine->visible_area.max_y;

	xcenter=((xmax + xmin) / 2) << 16;
	ycenter=((ymax + ymin) / 2) << 16;

	return video_start_vector();
}
