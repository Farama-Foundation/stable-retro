/*
 *	Thunder Ceptor board
 *	emulate video hardware
 */

#include "driver.h"
#include "state.h"
#include "namcoic.h"

#ifdef MAME_DEBUG
extern int debug_key_pressed;
#endif

#define TX_TILE_OFFSET_CENTER	(32 * 2)
#define TX_TILE_OFFSET_RIGHT	(32 * 0 + 2)
#define TX_TILE_OFFSET_LEFT	(32 * 31 + 2)

#define SPR_TRANS_COLOR		(0xff + 768)
#define SPR_MASK_COLOR		(0xfe + 768)


data8_t *tceptor_tile_ram;
data8_t *tceptor_tile_attr;
data8_t *tceptor_bg_ram;
data16_t *tceptor_sprite_ram;

static int sprite16;
static int sprite32;
static int bg;

static struct tilemap *tx_tilemap;

static struct tilemap *bg1_tilemap;
static struct tilemap *bg2_tilemap;

static int bg1_scroll_x, bg1_scroll_y;
static int bg2_scroll_x, bg2_scroll_y;

static struct mame_bitmap *temp_bitmap;

static int is_mask_spr[1024/16];

/*******************************************************************/

PALETTE_INIT( tceptor )
{
	int totcolors, totlookup;
	int i;

	totcolors = Machine->drv->total_colors;
	totlookup = Machine->drv->color_table_len;

	for (i = 0; i < totcolors; i++)
	{
		int bit0, bit1, bit2, bit3, r, g, b;

		bit0 = (color_prom[0] >> 0) & 0x01;
		bit1 = (color_prom[0] >> 1) & 0x01;
		bit2 = (color_prom[0] >> 2) & 0x01;
		bit3 = (color_prom[0] >> 3) & 0x01;
		r = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[totcolors] >> 0) & 0x01;
		bit1 = (color_prom[totcolors] >> 1) & 0x01;
		bit2 = (color_prom[totcolors] >> 2) & 0x01;
		bit3 = (color_prom[totcolors] >> 3) & 0x01;
		g = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[2*totcolors] >> 0) & 0x01;
		bit1 = (color_prom[2*totcolors] >> 1) & 0x01;
		bit2 = (color_prom[2*totcolors] >> 2) & 0x01;
		bit3 = (color_prom[2*totcolors] >> 3) & 0x01;
		b = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		palette_set_color(i, r, g, b);
		color_prom++;
	}

	color_prom += 2 * totcolors;
	/* color_prom now points to the beginning of the lookup table */

	/*
	  color lookup table:
		0-    +1024 ( 4 * 256) colors: text   (use 0-   256 colors)
		1024- +1024 (16 *  64) colors: sprite (use 768- 256 colors)
		2048-  +512 ( 8 *  64) colors: bg     (use 0-   512 colors)
		3840-  +256 ( 4 *  64) colors: road   (use 512- 256 colors)
	*/

	/* tiles lookup table (1024 colors) */
	for (i = 0; i < 1024;i++)
		colortable[i] = *color_prom++;

	/* road lookup table (256 colors) */
	for (i = 0; i < 256; i++)
		colortable[i + 0xf00] = *(color_prom++) + 512;

	/* sprites lookup table (1024 colors) */
	for (i = 0;i < 1024; i++)
		colortable[i + 1024] = *(color_prom++) + 768;

	/* background: lookup prom is not presented, use prom directly (512 colors) */
	for (i = 0;i < 512; i++)
		colortable[i + 2048] = i;

	/* setup sprite mask color map */
	/* tceptor2: only 0x23 */
	memset(is_mask_spr, 0, sizeof is_mask_spr);
	for (i = 0; i < 1024; i++)
		if (colortable[i + 1024] == SPR_MASK_COLOR)
			is_mask_spr[i / 16] = 1;
}


/*******************************************************************/

static INLINE int get_tile_addr(int tile_index)
{
	int x = tile_index / 28;
	int y = tile_index % 28;

	switch (x)
	{
	case 0:
		return TX_TILE_OFFSET_LEFT + y;
	case 33:
		return TX_TILE_OFFSET_RIGHT + y;
	}

	return TX_TILE_OFFSET_CENTER + (x - 1) + y * 32;
}

static void get_tx_tile_info(int tile_index)
{
	int offset = get_tile_addr(tile_index);
	int code = tceptor_tile_ram[offset];
	int color = tceptor_tile_attr[offset];

	SET_TILE_INFO(0, code, color, 0);
}

static void tile_mark_dirty(int offset)
{
	int x = -1;
	int y = -1;

	if (offset >= TX_TILE_OFFSET_LEFT && offset < TX_TILE_OFFSET_LEFT + 28)
	{
		x = 0;
		y = offset - TX_TILE_OFFSET_LEFT;
	}
	else if (offset >= TX_TILE_OFFSET_RIGHT && offset < TX_TILE_OFFSET_RIGHT + 28)
	{
		x = 33;
		y = offset - TX_TILE_OFFSET_RIGHT;
	}
	else if (offset >= TX_TILE_OFFSET_CENTER && offset < TX_TILE_OFFSET_CENTER + 32 * 28)
	{
		offset -= TX_TILE_OFFSET_CENTER;
		x = (offset % 32) + 1;
		y = offset / 32;
	}

	if (x >= 0)
		tilemap_mark_tile_dirty(tx_tilemap, x * 28 + y);
}


READ_HANDLER( tceptor_tile_ram_r )
{
	return tceptor_tile_ram[offset];
}

WRITE_HANDLER( tceptor_tile_ram_w )
{
	if (tceptor_tile_ram[offset] != data)
	{
		tceptor_tile_ram[offset] = data;
		tile_mark_dirty(offset);
	}
}

READ_HANDLER( tceptor_tile_attr_r )
{
	return tceptor_tile_attr[offset];
}

WRITE_HANDLER( tceptor_tile_attr_w )
{
	if (tceptor_tile_attr[offset] != data)
	{
		tceptor_tile_attr[offset] = data;
		tile_mark_dirty(offset);
	}
}


/*******************************************************************/

static void get_bg1_tile_info(int tile_index)
{
	UINT16 data = tceptor_bg_ram[tile_index * 2] | (tceptor_bg_ram[tile_index * 2 + 1] << 8);
	int code = (data & 0x3ff) | 0x000;
	int color = (data & 0xfc00) >> 10;

	SET_TILE_INFO(bg, code, color, 0);
}

static void get_bg2_tile_info(int tile_index)
{
	UINT16 data = tceptor_bg_ram[tile_index * 2 + 0x1000] | (tceptor_bg_ram[tile_index * 2 + 1 + 0x1000] << 8);
	int code = (data & 0x3ff) | 0x400;
	int color = (data & 0xfc00) >> 10;

	SET_TILE_INFO(bg, code, color, 0);
}

READ_HANDLER( tceptor_bg_ram_r )
{
	return tceptor_bg_ram[offset];
}

WRITE_HANDLER( tceptor_bg_ram_w )
{
	if (tceptor_bg_ram[offset] != data)
	{
		tceptor_bg_ram[offset] = data;

		offset /= 2;
		if (offset < 0x800)
			tilemap_mark_tile_dirty(bg1_tilemap, offset);
		else
			tilemap_mark_tile_dirty(bg2_tilemap, offset - 0x800);
	}
}

WRITE_HANDLER( tceptor_bg_scroll_w )
{
	switch (offset)
	{
	case 0:
		bg1_scroll_x &= 0xff;
		bg1_scroll_x |= data << 8;
		break;
	case 1:
		bg1_scroll_x &= 0xff00;
		bg1_scroll_x |= data;
		break;
	case 2:
		bg1_scroll_y = data;
		break;

	case 4:
		bg2_scroll_x &= 0xff;
		bg2_scroll_x |= data << 8;
		break;
	case 5:
		bg2_scroll_x &= 0xff00;
		bg2_scroll_x |= data;
		break;
	case 6:
		bg2_scroll_y = data;
		break;
	}
}


/*******************************************************************/

static int decode_bg(int region)
{
	static struct GfxLayout bg_layout =
	{
		8, 8,
		2048,
		3,
		{ 0x40000+4, 0, 4 },
		{ 0, 1, 2, 3, 8, 9, 10, 11 },
		{ 0, 16, 32, 48, 64, 80, 96, 112 },
		128
	};

	int gfx_index = bg;
	data8_t *src = memory_region(region) + 0x8000;
	unsigned char *buffer;
	int len = 0x8000;
	int i;

	if (!(buffer = malloc(len)))
		return 1;

	/* expand rom tc2-19.10d */
	for (i = 0; i < len / 2; i++)
	{
		buffer[i*2+1] = src[i] & 0x0f;
		buffer[i*2] = (src[i] & 0xf0) >> 4;
	}

	memcpy(src, buffer, len);
	free(buffer);

	/* decode the graphics */
	Machine->gfx[gfx_index] = decodegfx(memory_region(region), &bg_layout);
	if (!Machine->gfx[gfx_index])
		return 1;

	/* set the color information */
	Machine->gfx[gfx_index]->colortable = &Machine->remapped_colortable[2048];
	Machine->gfx[gfx_index]->total_colors = 64;

	return 0;
}

static int decode_sprite(int gfx_index, struct GfxLayout *layout, const void *data)
{
	/* decode the graphics */
	Machine->gfx[gfx_index] = decodegfx(data, layout);
	if (!Machine->gfx[gfx_index])
		return 1;

	/* set the color information */
	Machine->gfx[gfx_index]->colortable = &Machine->remapped_colortable[1024];
	Machine->gfx[gfx_index]->total_colors = 64;

	return 0;
}

/* fix sprite order*/
static int decode_sprite16(int region)
{
	static struct GfxLayout spr16_layout =
	{
		16, 16,
		512,
		4,
		{ 0x00000, 0x00004, 0x40000, 0x40004 },
		{
			0*8, 0*8+1, 0*8+2, 0*8+3, 1*8, 1*8+1, 1*8+2, 1*8+3,
			2*8, 2*8+1, 2*8+2, 2*8+3, 3*8, 3*8+1, 3*8+2, 3*8+3
		},
		{
			 0*2*16,  1*2*16,  2*2*16,  3*2*16,  4*2*16,  5*2*16,  6*2*16,  7*2*16,
			 8*2*16,  9*2*16, 10*2*16, 11*2*16, 12*2*16, 13*2*16, 14*2*16, 15*2*16
		},
		2*16*16
	};

	data8_t *src = memory_region(region);
	int len = memory_region_length(region);
	data8_t *dst;
	int i, y;

	dst = (data8_t *)malloc(len);
	if (!src || !dst)
		return 1;

	for (i = 0; i < len / (4*4*16); i++)
		for (y = 0; y < 16; y++)
		{
			memcpy(&dst[(i*4 + 0) * (2*16*16/8) + y * (2*16/8)],
			       &src[i * (2*32*32/8) + y * (2*32/8)],
			       4);
			memcpy(&dst[(i*4 + 1) * (2*16*16/8) + y * (2*16/8)],
			       &src[i * (2*32*32/8) + y * (2*32/8) + (4*8/8)],
			       4);
			memcpy(&dst[(i*4 + 2) * (2*16*16/8) + y * (2*16/8)],
			       &src[i * (2*32*32/8) + y * (2*32/8) + (16*2*32/8)],
			       4);
			memcpy(&dst[(i*4 + 3) * (2*16*16/8) + y * (2*16/8)],
			       &src[i * (2*32*32/8) + y * (2*32/8) + (4*8/8) + (16*2*32/8)],
			       4);
		}

	if (decode_sprite(sprite16, &spr16_layout, dst))
		return 1;

	free(dst);

	return 0;
}

/* fix sprite order*/
static int decode_sprite32(int region)
{
	static struct GfxLayout spr32_layout =
	{
		32, 32,
		1024,
		4,
		{ 0x000000, 0x000004, 0x200000, 0x200004 },
		{
			0*8, 0*8+1, 0*8+2, 0*8+3, 1*8, 1*8+1, 1*8+2, 1*8+3,
			2*8, 2*8+1, 2*8+2, 2*8+3, 3*8, 3*8+1, 3*8+2, 3*8+3,
			4*8, 4*8+1, 4*8+2, 4*8+3, 5*8, 5*8+1, 5*8+2, 5*8+3,
			6*8, 6*8+1, 6*8+2, 6*8+3, 7*8, 7*8+1, 7*8+2, 7*8+3
		},
		{
			 0*2*32,  1*2*32,  2*2*32,  3*2*32,  4*2*32,  5*2*32,  6*2*32,  7*2*32,
			 8*2*32,  9*2*32, 10*2*32, 11*2*32, 12*2*32, 13*2*32, 14*2*32, 15*2*32,
			16*2*32, 17*2*32, 18*2*32, 19*2*32, 20*2*32, 21*2*32, 22*2*32, 23*2*32,
			24*2*32, 25*2*32, 26*2*32, 27*2*32, 28*2*32, 29*2*32, 30*2*32, 31*2*32
		},
		2*32*32
	};

	data8_t *src = memory_region(region);
	int len = memory_region_length(region);
	int total = spr32_layout.total;
	int size = spr32_layout.charincrement / 8;
	data8_t *dst;
	int i;

	dst = (data8_t *)malloc(len);
	if (!src || !dst)
		return 1;

	memset(dst, 0, len);

	for (i = 0; i < total; i++)
	{
		int code;

		code = (i & 0x07f) | ((i & 0x180) << 1) | 0x80;
		code &= ~((i & 0x200) >> 2);

		memcpy(&dst[size * (i + 0)],     &src[size * (code + 0)],     size);
		memcpy(&dst[size * (i + total)], &src[size * (code + total)], size);
	}

	if (decode_sprite(sprite32, &spr32_layout, dst))
		return 1;

	free(dst);

	return 0;
}

static void mark_all_tiles_dirty(void)
{
	tilemap_mark_all_tiles_dirty(tx_tilemap);
	tilemap_mark_all_tiles_dirty(bg1_tilemap);
	tilemap_mark_all_tiles_dirty(bg2_tilemap);
}

VIDEO_START( tceptor )
{
	int gfx_index;

	/* find first empty slot to decode gfx */
	for (gfx_index = 0; gfx_index < MAX_GFX_ELEMENTS; gfx_index++)
		if (Machine->gfx[gfx_index] == 0)
			break;
	if (gfx_index + 4 > MAX_GFX_ELEMENTS)
		return 1;

	bg = gfx_index++;
	if (decode_bg(REGION_GFX2))
		return 1;

	sprite16 = gfx_index++;
	if (decode_sprite16(REGION_GFX3))
		return 1;

	sprite32 = gfx_index++;
	if (decode_sprite32(REGION_GFX4))
		return 1;

	/* allocate temp bitmaps */
	temp_bitmap = auto_bitmap_alloc(Machine->drv->screen_width, Machine->drv->screen_height);
	if (!temp_bitmap)
		return 1;

	if (namco_road_init(gfx_index))
		return 1;

	namco_road_set_transparent_color(Machine->remapped_colortable[0xfff]);

	tx_tilemap = tilemap_create(get_tx_tile_info, tilemap_scan_cols, TILEMAP_TRANSPARENT_COLOR, 8, 8, 34, 28);
	if (!tx_tilemap)
		return 1;

	tilemap_set_scrollx(tx_tilemap, 0, -2*8);
	tilemap_set_scrolly(tx_tilemap, 0, 0);
	tilemap_set_transparent_pen(tx_tilemap, 7);

	bg1_tilemap = tilemap_create(get_bg1_tile_info, tilemap_scan_rows, TILEMAP_OPAQUE, 8, 8, 64, 32);
	bg2_tilemap = tilemap_create(get_bg2_tile_info, tilemap_scan_rows, TILEMAP_OPAQUE, 8, 8, 64, 32);
	if (!bg1_tilemap || !bg2_tilemap)
		return 1;

	tilemap_set_transparent_pen(bg1_tilemap, 0);
	tilemap_set_transparent_pen(bg2_tilemap, 0);

	state_save_register_int   ("tceptor", 0, "bg1_scroll_x",      &bg1_scroll_x);
	state_save_register_int   ("tceptor", 0, "bg1_scroll_y",      &bg1_scroll_y);
	state_save_register_int   ("tceptor", 0, "bg2_scroll_x",      &bg2_scroll_x);
	state_save_register_int   ("tceptor", 0, "bg2_scroll_y",      &bg2_scroll_y);

	state_save_register_func_postload(mark_all_tiles_dirty);

	return 0;
}


/*******************************************************************/

/*
	Sprite data format

	000: zzzzzzBB BTTTTTTT
	002: ZZZZZZPP PPCCCCCC
	100: fFL---YY YYYYYYYY
	102: ------XX XXXXXXXX

	B: bank
	T: number
	P: priority
	C: color
	X: x
	Y: y
	L: large sprite
	F: flip x
	f: flip y
	Z: zoom x
	z: zoom y
*/

static void draw_sprites(struct mame_bitmap *bitmap, const struct rectangle *cliprect, int sprite_priority)
{
	data16_t *mem1 = &tceptor_sprite_ram[0x000/2];
	data16_t *mem2 = &tceptor_sprite_ram[0x100/2];
	int need_mask = 0;
	int i;

	for (i = 0; i < 0x100; i += 2)
	{
		int scalex = (mem1[1 + i] & 0xfc00) << 1;
		int scaley = (mem1[0 + i] & 0xfc00) << 1;
		int pri = 7 - ((mem1[1 + i] & 0x3c0) >> 6);

		if (pri == sprite_priority && scalex && scaley)
		{
			int x = mem2[1 + i] & 0x3ff;
			int y = 512 - (mem2[0 + i] & 0x3ff);
			int flipx = mem2[0 + i] & 0x4000;
			int flipy = mem2[0 + i] & 0x8000;
			int color = mem1[1 + i] & 0x3f;
			int gfx;
			int code;

			if (mem2[0 + i] & 0x2000)
			{
				gfx = sprite32;
				code = mem1[0 + i] & 0x3ff;

			}
			else
			{
				gfx = sprite16;
				code = mem1[0 + i] & 0x1ff;
				scaley *= 2;
			}

			if (is_mask_spr[color])
			{
				if (!need_mask)
				{
					/* backup previous bitmap*/
					copybitmap(temp_bitmap, bitmap, 0, 0, 0, 0, cliprect, TRANSPARENCY_NONE, 0);
				}

				need_mask = 1;
			}

			/* round off*/
			scalex += 0x800;
			scaley += 0x800;

			x -= 64;
			y -= 78;

			drawgfxzoom(bitmap,
			            Machine->gfx[gfx],
			            code,
			            color,
			            flipx, flipy,
			            x, y,
			            cliprect,
			            TRANSPARENCY_COLOR, SPR_TRANS_COLOR,
			            scalex,
			            scaley);
		}
	}

	/* if SPR_MASK_COLOR pen is used, restore pixels from previous bitmap */
	if (need_mask)
	{
		int x, y;

		for (x = cliprect->min_x; x <= cliprect->max_x; x++)
			for (y = cliprect->min_y; y <= cliprect->max_y; y++)
				if (read_pixel(bitmap, x, y) == SPR_MASK_COLOR)
				{
					int color = read_pixel(temp_bitmap, x, y);

					/* restore pixel*/
					plot_pixel(bitmap, x, y, color);
				}
	}
}


VIDEO_UPDATE( tceptor )
{
	struct rectangle rect;
	int pri;
	int bg_center = 144 - ((((bg1_scroll_x + bg2_scroll_x ) & 0x1ff) - 288) / 2);

	/* left background*/
	rect = *cliprect;
	rect.max_x = bg_center;
	tilemap_set_scrollx(bg1_tilemap, 0, bg1_scroll_x + 12);
	tilemap_set_scrolly(bg1_tilemap, 0, bg1_scroll_y + 20); /*32?*/
	tilemap_draw(bitmap, &rect, bg1_tilemap, 0, 0);

	/* right background*/
	rect.min_x = bg_center;
	rect.max_x = cliprect->max_x;
	tilemap_set_scrollx(bg2_tilemap, 0, bg2_scroll_x + 20);
	tilemap_set_scrolly(bg2_tilemap, 0, bg2_scroll_y + 20); /* 32?*/
	tilemap_draw(bitmap, &rect, bg2_tilemap, 0, 0);

	for (pri = 0; pri < 8; pri++)
	{
		namco_road_draw(bitmap, cliprect, pri * 2);
		namco_road_draw(bitmap, cliprect, pri * 2 + 1);
		draw_sprites(bitmap, cliprect, pri);
	}

	tilemap_draw(bitmap, cliprect, tx_tilemap, 0, 0);
}
