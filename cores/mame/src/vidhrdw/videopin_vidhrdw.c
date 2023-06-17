/*************************************************************************

	Atari Video Pinball video emulation

*************************************************************************/

#include "driver.h"

UINT8* videopin_video_ram;

static int ball_x;
static int ball_y;

static struct tilemap* tilemap;


static UINT32 get_memory_offset(UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows)
{
	return num_rows * ((col + 16) % 48) + row;
}


static void get_tile_info(int tile_index)
{
	UINT8 code = videopin_video_ram[tile_index];

	SET_TILE_INFO(0, code, 0, (code & 0x40) ? TILE_FLIPY : 0)
}


VIDEO_START( videopin )
{
	tilemap = tilemap_create(get_tile_info, get_memory_offset, TILEMAP_OPAQUE, 8, 8, 48, 32);

	if (tilemap == NULL)
	{
		return 1;
	}

	return 0;
}


VIDEO_UPDATE( videopin )
{
	int col;
	int row;

	tilemap_set_scrollx(tilemap, 0, -8);   /* account for delayed loading of shift reg C6 */

	tilemap_draw(bitmap, cliprect, tilemap, 0, 0);

	for (row = 0; row < 32; row++)
	{
		for (col = 0; col < 48; col++)
		{
			UINT32 offset = get_memory_offset(col, row, 48, 32);

			if (videopin_video_ram[offset] & 0x80)   /* ball bit found */
			{
				struct rectangle rect;

				int x = 8 * col;
				int y = 8 * row;

				int i;
				int j;

				x += 4;   /* account for delayed loading of flip-flop C4 */

				rect.min_x = x;
				rect.min_y = y;
				rect.max_x = x + 15;
				rect.max_y = y + 15;

				if (rect.min_x < cliprect->min_x)
					rect.min_x = cliprect->min_x;
				if (rect.min_y < cliprect->min_y)
					rect.min_y = cliprect->min_y;
				if (rect.max_x > cliprect->max_x)
					rect.max_x = cliprect->max_x;
				if (rect.max_y > cliprect->max_y)
					rect.max_y = cliprect->max_y;

				x -= ball_x;
				y -= ball_y;

				/* ball placement is still 0.5 pixels off but don't tell anyone */

				for (i = 0; i < 2; i++)
				{
					for (j = 0; j < 2; j++)
					{
						drawgfx(bitmap, Machine->gfx[1],
							0, 0,
							0, 0,
							x + 16 * i,
							y + 16 * j,
							&rect, TRANSPARENCY_PEN, 0);
					}
				}

				return;   /* keep things simple and ignore the rest */
			}
		}
	}
}


WRITE_HANDLER( videopin_ball_w )
{
	ball_x = data & 15;
	ball_y = data >> 4;
}


WRITE_HANDLER( videopin_video_ram_w )
{
	if (videopin_video_ram[offset] != data)
	{
		tilemap_mark_tile_dirty(tilemap, offset);
	}

	videopin_video_ram[offset] = data;
}
