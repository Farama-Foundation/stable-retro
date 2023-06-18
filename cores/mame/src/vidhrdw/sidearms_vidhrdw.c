/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

extern int sidearms_gameid;

UINT8 *sidearms_bg_scrollx;
UINT8 *sidearms_bg_scrolly;

static UINT8 *tilerom;
static int bgon, objon, staron, charon, flipon;
static unsigned int hflop_74a_n, hcount_191, vcount_191, latch_374;

static struct tilemap *bg_tilemap, *fg_tilemap;

WRITE_HANDLER( sidearms_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(fg_tilemap, offset);
	}
}

WRITE_HANDLER( sidearms_colorram_w )
{
	if (colorram[offset] != data)
	{
		colorram[offset] = data;
		tilemap_mark_tile_dirty(fg_tilemap, offset);
	}
}

WRITE_HANDLER( sidearms_c804_w )
{
	/* bits 0 and 1 are coin counters */
	coin_counter_w(0, data & 0x01);
	coin_counter_w(1, data & 0x02);

	/* bit 2 and 3 lock the coin chutes */
	if (!sidearms_gameid || sidearms_gameid==3)
	{
		coin_lockout_w(0, !(data & 0x04));
		coin_lockout_w(1, !(data & 0x08));
	}
	else
	{
		coin_lockout_w(0, data & 0x04);
		coin_lockout_w(1, data & 0x08);
	}

	/* bit 4 resets the sound CPU */
	if (data & 0x10)
	{
		cpuint_reset_cpu(1);
	}

	/* bit 5 enables starfield */
	if (staron != (data & 0x20))
	{
		staron = data & 0x20;
		hflop_74a_n = 1;
		hcount_191 = vcount_191 = 0;
	}

	/* bit 6 enables char layer */
	charon = data & 0x40;

	/* bit 7 flips screen */
	if (flipon != (data & 0x80))
	{
		flipon = data & 0x80;
		flip_screen_set(flipon);
		tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
	}
}

WRITE_HANDLER( sidearms_gfxctrl_w )
{
	objon = data & 0x01;
	bgon = data & 0x02;
}

WRITE_HANDLER( sidearms_star_scrollx_w )
{
	unsigned int last_state = hcount_191;

	hcount_191++;
	hcount_191 &= 0x1ff;

	/* invert 74LS74A(flipflop) output on 74LS191(hscan counter) carry's rising edge*/
	if (hcount_191 & ~last_state & 0x100)
		hflop_74a_n ^= 1;
}

WRITE_HANDLER( sidearms_star_scrolly_w )
{
	vcount_191++;
	vcount_191 &= 0xff;
}


static INLINE void get_sidearms_bg_tile_info(int offs)
{
	int code, attr, color, flags;

	code = tilerom[offs];
	attr = tilerom[offs + 1];
	code |= attr<<8 & 0x100;
	color = attr>>3 & 0x1f;
	flags = attr>>1 & 0x03;

	SET_TILE_INFO(1, code, color, flags)
}

static INLINE void get_philko_bg_tile_info(int offs)
{
	int code, attr, color, flags;

	code = tilerom[offs];
	attr = tilerom[offs + 1];
	code |= (((attr>>6 & 0x02) | (attr & 0x01)) * 0x100);
	color = attr>>3 & 0x0f;
	flags = attr>>1 & 0x03;

	SET_TILE_INFO(1, code, color, flags)
}

static INLINE void get_fg_tile_info(int tile_index)
{
	int attr = colorram[tile_index];
	int code = videoram[tile_index] + (attr<<2 & 0x300);
	int color = attr & 0x3f;

	SET_TILE_INFO(0, code, color, 0)
}

static INLINE UINT32 sidearms_tilemap_scan( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows )
{
	/* logical (col,row) -> memory offset */
	int offset = ((row << 7) + col) << 1;

	/* swap bits 1-7 and 8-10 of the address to compensate for the funny layout of the ROM data */
	return ((offset & 0xf801) | ((offset & 0x0700) >> 7) | ((offset & 0x00fe) << 3)) & 0x7fff;
}

VIDEO_START( sidearms )
{
	tilerom = memory_region(REGION_GFX4);

	if (!sidearms_gameid)
	{
		bg_tilemap = tilemap_create(get_sidearms_bg_tile_info, sidearms_tilemap_scan,
			TILEMAP_TRANSPARENT, 32, 32, 128, 128);

		if ( !bg_tilemap ) return 1;

		tilemap_set_transparent_pen(bg_tilemap, 15);
	}
	else
	{
		bg_tilemap = tilemap_create(get_philko_bg_tile_info, sidearms_tilemap_scan,TILEMAP_OPAQUE, 32, 32, 128, 128);

		if ( !bg_tilemap ) return 1;
	}

	fg_tilemap = tilemap_create(get_fg_tile_info, tilemap_scan_rows,
		TILEMAP_TRANSPARENT, 8, 8, 64, 64);

	if ( !fg_tilemap ) return 1;

	tilemap_set_transparent_pen(fg_tilemap, 3);

	hflop_74a_n = 1;
	latch_374 = vcount_191 = hcount_191 = 0;

	flipon = charon = staron = objon = bgon = 0;

	return 0;
}

void sidearms_draw_sprites_region( struct mame_bitmap *bitmap, int start_offset, int end_offset )
{
	const struct GfxElement *gfx = Machine->gfx[2];
	struct rectangle *cliprect = &Machine->visible_area;
	int offs, attr, color, code, x, y, flipx, flipy;

	flipy = flipx = flipon;

	for (offs = end_offset - 32; offs >= start_offset; offs -= 32)
	{
		y = buffered_spriteram[offs + 2];
		if (!y || buffered_spriteram[offs + 5] == 0xc3) continue;

		attr = buffered_spriteram[offs + 1];
		color = attr & 0xf;
		code = buffered_spriteram[offs] + ((attr << 3) & 0x700);
		x = buffered_spriteram[offs + 3] + ((attr << 4) & 0x100);

		if (flipon)
		{
			x = (62 * 8) - x;
			y = (30 * 8) - y;
		}

		drawgfx(bitmap, gfx,
			code, color,
			flipx, flipy,
			x, y,
			cliprect,
			TRANSPARENCY_PEN, 15);
	}
}

static void sidearms_draw_starfield( struct mame_bitmap *bitmap )
{
	int x, y, i;
	unsigned int hadd_283, vadd_283, _hflop_74a_n, _hcount_191, _vcount_191;
	UINT8 *sf_rom;
	UINT16 *lineptr;
	int pixadv, lineadv;

	/* clear starfield background*/
	lineptr = (UINT16 *)bitmap->line[16] + 64;
	lineadv = bitmap->rowpixels;

	for (i=224; i; i--) { memset(lineptr, 0, 768); lineptr += lineadv; }

	/* bail if not Side Arms or the starfield has been disabled*/
	if (sidearms_gameid || !staron) return;

	/* init and cache some global vars in stack frame*/
	hadd_283 = 0;

	_hflop_74a_n = hflop_74a_n;
	_vcount_191 = vcount_191;
	_hcount_191 = hcount_191 & 0xff;

	sf_rom = memory_region(REGION_USER1);

#if 0 /* old loop (for reference; easier to read)*/
	if (!flipon)
	{
		lineptr = (UINT16 *)bitmap->line[0];
		pixadv  = 1;
		lineadv = lineadv - 512;
	}
	else
	{
		lineptr = (UINT16 *)bitmap->line[255] + 512 - 1;
		pixadv  = -1;
		lineadv = -lineadv + 512;
	}

	for (y=0; y<256; y++) /* 8-bit V-clock input*/
	{
		for (x=0; x<512; lineptr+=pixadv,x++) /* 9-bit H-clock input*/
		{
			i = hadd_283; /* store horizontal adder's previous state in i*/
			hadd_283 = _hcount_191 + (x & 0xff); /* add lower 8 bits and preserve carry*/

			if (x<64 || x>447 || y<16 || y>239) continue; /* clip rejection*/

			vadd_283 = _vcount_191 + y; /* add lower 8 bits and discard carry (later)*/

			if (!((vadd_283 ^ (x>>3)) & 4)) continue;		/* logic rejection 1*/
			if ((vadd_283 | (hadd_283>>1)) & 2) continue;	/* logic rejection 2*/

			/* latch data from starfield EPROM on rising edge of 74LS374's clock input*/
			if (!(~i & 0x1f))
			{
				i = vadd_283<<4 & 0xff0;				/* to starfield EPROM A04-A11 (8 bits)*/
				i |= (_hflop_74a_n^(hadd_283>>8)) << 3;	/* to starfield EPROM A03     (1 bit)*/
				i |= hadd_283>>5 & 7;					/* to starfield EPROM A00-A02 (3 bits)*/
				latch_374 = sf_rom[i + 0x3000];			/* lines A12-A13 are always high*/
			}

			if ((~((latch_374^hadd_283)^1) & 0x1f)) continue; /* logic rejection 3*/

			*lineptr = (UINT16)(latch_374>>5 | 0x378); /* to color mixer*/
		}
		lineptr += lineadv;
	}
#else /* optimized loop*/
	if (!flipon)
	{
		lineptr = (UINT16 *)bitmap->line[16] + 64;
		pixadv  = 1;
		lineadv = lineadv - 384;
	}
	else
	{
		lineptr = (UINT16 *)bitmap->line[239] + 512 - 64 - 1;
		pixadv  = -1;
		lineadv = -lineadv + 384;
	}

	for (y=16; y<240; y++) /* 8-bit V-clock input (clipped against vertical visible area)*/
	{
		/* inner loop pre-entry conditioning*/
		hadd_283 = (_hcount_191 + 64) & ~0x1f;
		vadd_283 = _vcount_191 + y;

		i = vadd_283<<4 & 0xff0;				/* to starfield EPROM A04-A11 (8 bits)*/
		i |= (_hflop_74a_n^(hadd_283>>8)) << 3;	/* to starfield EPROM A03     (1 bit)*/
		i |= hadd_283>>5 & 7;					/* to starfield EPROM A00-A02 (3 bits)*/
		latch_374 = sf_rom[i + 0x3000];			/* lines A12-A13 are always high*/

		hadd_283 = _hcount_191 + 63;

		for (x=64; x<448; lineptr+=pixadv,x++) /* 9-bit H-clock input (clipped against horizontal visible area)*/
		{
			i = hadd_283;							/* store horizontal adder's previous state in i*/
			hadd_283 = _hcount_191 + (x & 0xff);	/* add lower 8 bits and preserve carry*/
			vadd_283 = _vcount_191 + y;				/* add lower 8 bits and discard carry (later)*/

			if (!((vadd_283 ^ (x>>3)) & 4)) continue;		/* logic rejection 1*/
			if ((vadd_283 | (hadd_283>>1)) & 2) continue;	/* logic rejection 2*/

			/* latch data from starfield EPROM on rising edge of 74LS374's clock input*/
			if (!(~i & 0x1f))
			{
				i = vadd_283<<4 & 0xff0;				/* to starfield EPROM A04-A11 (8 bits)*/
				i |= (_hflop_74a_n^(hadd_283>>8)) << 3;	/* to starfield EPROM A03     (1 bit)*/
				i |= hadd_283>>5 & 7;					/* to starfield EPROM A00-A02 (3 bits)*/
				latch_374 = sf_rom[i + 0x3000];			/* lines A12-A13 are always high*/
			}

			if ((~((latch_374^hadd_283)^1) & 0x1f)) continue; /* logic rejection 3*/

			*lineptr = (UINT16)(latch_374>>5 | 0x378); /* to color mixer*/
		}
		lineptr += lineadv;
	}
#endif
}

static void sidearms_draw_sprites( struct mame_bitmap *bitmap )
{
	if (sidearms_gameid == 2 || sidearms_gameid == 3) /* Dyger and Whizz have simple front-to-back sprite priority*/
		sidearms_draw_sprites_region(bitmap, 0x0000, 0x1000);
	else
	{
		sidearms_draw_sprites_region(bitmap, 0x0700, 0x0800);
		sidearms_draw_sprites_region(bitmap, 0x0e00, 0x1000);
		sidearms_draw_sprites_region(bitmap, 0x0800, 0x0f00);
		sidearms_draw_sprites_region(bitmap, 0x0000, 0x0700);
	}
}

VIDEO_UPDATE( sidearms )
{
	sidearms_draw_starfield(bitmap);

	tilemap_set_scrollx(bg_tilemap, 0, sidearms_bg_scrollx[0] + (sidearms_bg_scrollx[1] << 8 & 0xf00));
	tilemap_set_scrolly(bg_tilemap, 0, sidearms_bg_scrolly[0] + (sidearms_bg_scrolly[1] << 8 & 0xf00));

	if (bgon)
		tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);

	if (objon)
		sidearms_draw_sprites(bitmap);

	if (charon)
		tilemap_draw(bitmap, cliprect, fg_tilemap, 0, 0);
}

VIDEO_EOF( sidearms )
{
	buffer_spriteram_w(0, 0);
}
