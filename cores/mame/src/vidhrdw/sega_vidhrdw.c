/*************************************************************************

	Sega vector hardware

*************************************************************************/

/*
 * History:
 *
 * 97???? Converted Al Kossow's G80 sources. LBO
 * 970807 Scaling support and dynamic sin/cos tables. ASG
 * 980124 Suport for antialiasing. .ac
 * 980203 cleaned up and interfaced to generic vector routines. BW
 *
 * TODO: use floating point math instead of fixed point.
 */

#include "driver.h"
#include "avgdvg.h"
#include "vector.h"
#include <math.h>

static int width, height, cent_x, cent_y, min_x, min_y, max_x, max_y;
static long *sinTable, *cosTable;
static int intensity;

void sega_generate_vector_list (void)
{
	int deltax, deltay;
	int currentX, currentY;

	int vectorIndex;
	int symbolIndex;

	int rotate, scale;
	int attrib;

	int angle, length;
	int color;

	int draw;

	vector_clear_list();

	symbolIndex = 0;	/* Reset vector PC to 0 */

	/* Sega games are all clipped to this region at the DAC */
	vector_add_clip((512-min_x)<<16, (max_y-1536)<<16,
  	              (1536-min_x)<<16, (max_y-512)<<16);

	/*
	 * walk the symbol list until 'last symbol' set
	 */

	do {
		draw = vectorram[symbolIndex++];

		if (draw & 1)	/* if symbol active */
		{
			currentX    = vectorram[symbolIndex + 0] | (vectorram[symbolIndex + 1] << 8);
			currentY    = vectorram[symbolIndex + 2] | (vectorram[symbolIndex + 3] << 8);
			vectorIndex = vectorram[symbolIndex + 4] | (vectorram[symbolIndex + 5] << 8);
			rotate      = vectorram[symbolIndex + 6] | (vectorram[symbolIndex + 7] << 8);
			scale       = vectorram[symbolIndex + 8];

			currentX = ((currentX & 0x7ff) - min_x) << 16;
			currentY = (max_y - (currentY & 0x7ff)) << 16;
			vector_add_point ( currentX, currentY, 0, 0);
			vectorIndex &= 0xfff;

			/* walk the vector list until 'last vector' bit */
			/* is set in attributes */

			do
			{
				attrib = vectorram[vectorIndex + 0];
				length = vectorram[vectorIndex + 1];
				angle  = vectorram[vectorIndex + 2] | (vectorram[vectorIndex + 3] << 8);

				vectorIndex += 4;

				/* calculate deltas based on len, angle(s), and scale factor */

				angle = (angle + rotate) & 0x3ff;
				deltax = sinTable[angle] * scale * length;
				deltay = cosTable[angle] * scale * length;

				currentX += deltax >> 6;
				currentY -= deltay >> 6;

				color = VECTOR_COLOR222((attrib >> 1) & 0x3f);
				if ((attrib & 1) && color)
				{
					if (options.translucency)
						intensity = 0xa0; /* leave room for translucency */
					else
						intensity = 0xff;
				}
				else
					intensity = 0;
				vector_add_point ( currentX, currentY, color, intensity );

			} while (!(attrib & 0x80));
		}

		symbolIndex += 9;
		if (symbolIndex >= vectorram_size)
			break;

	} while (!(draw & 0x80));
}


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

VIDEO_START( sega )
{
	int i;

	if (vectorram_size == 0)
		return 1;
	min_x =Machine->visible_area.min_x;
	min_y =Machine->visible_area.min_y;
	max_x =Machine->visible_area.max_x;
	max_y =Machine->visible_area.max_y;
	width =max_x-min_x;
	height=max_y-min_y;
	cent_x=(max_x+min_x)/2;
	cent_y=(max_y+min_y)/2;

	/* allocate memory for the sine and cosine lookup tables ASG 080697 */
	sinTable = auto_malloc (0x400 * sizeof (long));
	if (!sinTable)
		return 1;
	cosTable = auto_malloc (0x400 * sizeof (long));
	if (!cosTable)
		return 1;

	/* generate the sine/cosine lookup tables */
	for (i = 0; i < 0x400; i++)
	{
		double angle = ((2. * M_PI) / (double)0x400) * (double)i;
		double temp;

		temp = sin (angle);
		if (temp < 0)
			sinTable[i] = (long)(temp * (double)(1 << 15) - 0.5);
		else
			sinTable[i] = (long)(temp * (double)(1 << 15) + 0.5);

		temp = cos (angle);
		if (temp < 0)
			cosTable[i] = (long)(temp * (double)(1 << 15) - 0.5);
		else
			cosTable[i] = (long)(temp * (double)(1 << 15) + 0.5);
	}

	return video_start_vector();
}



/***************************************************************************

  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/

VIDEO_UPDATE( sega )
{
	sega_generate_vector_list();
	video_update_vector(bitmap,0);
}
