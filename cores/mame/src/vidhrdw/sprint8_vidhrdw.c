/***************************************************************************

Atari Sprint 8 video emulation

***************************************************************************/

#include "driver.h"

extern void sprint8_collision_callback(int n);

UINT8* sprint8_video_ram;
UINT8* sprint8_pos_h_ram;
UINT8* sprint8_pos_v_ram;
UINT8* sprint8_pos_d_ram;

static struct tilemap* tilemap1;
static struct tilemap* tilemap2;

static struct mame_bitmap* helper1;
static struct mame_bitmap* helper2;


static void get_tile_info1(int tile_index)
{
	UINT8 code = sprint8_video_ram[tile_index];

	int color = 0;

	if ((code & 0x30) != 0x30) /* ? */
	{
		color = 17;
	}
	else
	{
		if ((tile_index + 1) & 0x010)
		{
			color |= 1;
		}
		if (code & 0x80)
		{
			color |= 2;
		}
		if (tile_index & 0x200)
		{
			color |= 4;
		}
	}

	SET_TILE_INFO(code >> 7, code, color, (code & 0x40) ? (TILE_FLIPX | TILE_FLIPY) : 0)
}


static void get_tile_info2(int tile_index)
{
	UINT8 code = sprint8_video_ram[tile_index];

	int color = 0;

	if ((code & 0x38) != 0x28)
	{
		color = 16;
	}
	else
	{
		color = 17;
	}

	SET_TILE_INFO(code >> 7, code, color, (code & 0x40) ? (TILE_FLIPX | TILE_FLIPY) : 0)
}


WRITE_HANDLER( sprint8_video_ram_w )
{
	if (data != sprint8_video_ram[offset])
	{
		tilemap_mark_tile_dirty(tilemap1, offset);
		tilemap_mark_tile_dirty(tilemap2, offset);
	}

	sprint8_video_ram[offset] = data;
}


VIDEO_START( sprint8 )
{
	helper1 = auto_bitmap_alloc(Machine->drv->screen_width, Machine->drv->screen_height);
	helper2 = auto_bitmap_alloc(Machine->drv->screen_width, Machine->drv->screen_height);

	if (helper1 == NULL)
	{
		return 1;
	}
	if (helper2 == NULL)
	{
		return 1;
	}

	tilemap1 = tilemap_create(get_tile_info1, tilemap_scan_rows, TILEMAP_OPAQUE, 16, 8, 32, 32);
	tilemap2 = tilemap_create(get_tile_info2, tilemap_scan_rows, TILEMAP_OPAQUE, 16, 8, 32, 32);

	if (tilemap2 == NULL)
	{
		return 1;
	}
	if (tilemap1 == NULL)
	{
		return 1;
	}

	tilemap_set_scrolly(tilemap1, 0, +24);
	tilemap_set_scrolly(tilemap2, 0, +24);

	return 0;
}


static void draw_sprites(struct mame_bitmap* bitmap, const struct rectangle* rect)
{
	int i;

	for (i = 0; i < 16; i++)
	{
		UINT8 code = sprint8_pos_d_ram[i];

		int x = sprint8_pos_h_ram[i];
		int y = sprint8_pos_v_ram[i];

		if (code & 0x80)
		{
			x |= 0x100;
		}

		drawgfx(bitmap, Machine->gfx[2],
			code ^ 7,
			i,
			!(code & 0x10), !(code & 0x08),
			496 - x, y - 31,
			rect, TRANSPARENCY_PEN, 0);
	}
}


VIDEO_UPDATE( sprint8 )
{
	tilemap_draw(bitmap, cliprect, tilemap1, 0, 0);

	draw_sprites(bitmap, cliprect);
}


VIDEO_EOF( sprint8 )
{
	int x;
	int y;

	tilemap_draw(helper2, &Machine->visible_area, tilemap2, 0, 0);

	fillbitmap(helper1, 16, &Machine->visible_area);

	draw_sprites(helper1, &Machine->visible_area);

	for (y = Machine->visible_area.min_y; y <= Machine->visible_area.max_y; y++)
	{
		const UINT16* p1 = (UINT16*) helper1->line[y];
		const UINT16* p2 = (UINT16*) helper2->line[y];

		for (x = Machine->visible_area.min_x; x <= Machine->visible_area.max_x; x++)
		{
			if (p1[x] != 16 && p2[x] != 16)
			{
				double time = cpu_getscanlinetime(y + 24) + x / 11055000.0;

				timer_set(time, p1[x], sprint8_collision_callback);
			}
		}
	}
}
