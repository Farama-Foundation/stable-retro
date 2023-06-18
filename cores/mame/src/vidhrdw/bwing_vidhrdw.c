/*****************************************************************************

B-Wings  (c) 1984 Data East Corporation
Zaviga   (c) 1984 Data East Corporation

drivers by Acho A. Tang

*****************************************************************************/
/* Directives*/

#include "driver.h"
#include "vidhrdw/generic.h"

#define BW_DEBUG 0

#define BW_NTILES_L2 7
#define BW_NTILES (1<<BW_NTILES_L2)

/******************************************************************************/
/* Local Vars*/

static struct tilemap *scrollmap[2], *charmap, *fgmap, *bgmap;
static struct GfxElement *fgfx, *bgfx;
static data8_t *srbase[4], *fgdata, *bgdata;
static int *srxlat;
static unsigned sreg[8], palatch=0, srbank=0, mapmask=0, mapflip=0;

/******************************************************************************/
/* Local Functions*/

static void fill_srxlat(int *xlat)
{
	unsigned base, offset, i;

	for(base=0; base<0x2000; base+=0x400)
	{
		for(i=0; i<0x100; i++)
		{
			offset = base + (i<<2 & ~0x3f) + (i & 0x0f);

			xlat[base+i] = offset;
			xlat[base+i+0x100] = offset + 0x10;
			xlat[base+i+0x200] = offset + 0x20;
			xlat[base+i+0x300] = offset + 0x30;
		}
	}
}

/******************************************************************************/
/* Exports*/

struct GfxLayout bwing_tilelayout =
{
	16, 16,
	BW_NTILES,
	3,
	{ 0x4000*8, 0x2000*8, 0 },
	{ 7, 6, 5, 4, 3, 2, 1, 0, 128+7, 128+6, 128+5, 128+4, 128+3, 128+2, 128+1, 128+0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  8*8, 9*8,10*8,11*8,12*8,13*8,14*8,15*8 },
	32*8
};


WRITE_HANDLER( bwing_spriteram_w ) { buffered_spriteram[offset] = data; }
WRITE_HANDLER( bwing_videoram_w )  { videoram[offset] = data; tilemap_mark_tile_dirty(charmap, offset); }


READ_HANDLER ( bwing_scrollram_r )
{
	if (!srbank) offset = srxlat[offset];

	return((srbase[srbank])[offset]);
}


WRITE_HANDLER( bwing_scrollram_w )
{
	if (!srbank)
	{
		offset = srxlat[offset];

		tilemap_mark_tile_dirty(scrollmap[offset>>12], offset & 0xfff);
	}

	(srbase[srbank])[offset] = data;
}


WRITE_HANDLER( bwing_scrollreg_w )
{
	static unsigned bp_ready=0;
	unsigned i;
	data8_t *src;

	sreg[offset] = data;

	switch (offset)
	{
		case 6: palatch = data; break; /* one of the palette components is latched through I/O(yike)*/

		case 7:
			/* tile graphics are decoded in RAM on the fly and tile codes are banked + interleaved(ouch)*/
			mapmask = data;
			srbank = data >> 6;

			if (srbank) bp_ready |= 1<<(srbank-1) & 7;

			if (bp_ready == 7 && !srbank)
			{
				src = srbase[1];
				for (i=0; i<BW_NTILES; i++) decodechar(fgfx, i, src, &bwing_tilelayout);

				src += 0x1000;
				for (i=0; i<BW_NTILES; i++) decodechar(bgfx, i, src, &bwing_tilelayout);

				bp_ready = 0;
			}

			#if BW_DEBUG
				log_cb(RETRO_LOG_DEBUG, LOGPRE "(%1d)%04x: w=%02x a=%04x f=%d\n",cpu_getactivecpu(),activecpu_get_pc(),data,0x1b00+offset,cpu_getcurrentframe());
			#endif
		break;
	}

	#if BW_DEBUG
		(memory_region(REGION_CPU1))[0x1b10 + offset] = data;
	#endif
}


WRITE_HANDLER( bwing_paletteram_w )
{
	const float rgb[4][3]={{0.85,0.95,1.00},{0.90,1.00,1.00},{0.80,1.00,1.00},{0.75,0.90,1.10}};
	int r, g, b, i;

	paletteram[offset] = data;

	r = ~data & 7;
	g = ~data>>4 & 7;
	b = ~palatch & 7;

	r = ((r<<5) + (r<<2) + (r>>1));
	g = ((g<<5) + (g<<2) + (g>>1));
	b = ((b<<5) + (b<<2) + (b>>1));

	if ((i = readinputport(7)) < 4)
	{
		r = (float)r * rgb[i][0];
		g = (float)g * rgb[i][1];
		b = (float)b * rgb[i][2];
		if (r > 0xff) r = 0xff;
		if (g > 0xff) g = 0xff;
		if (b > 0xff) b = 0xff;
	}

	palette_set_color(offset, r, g, b);

	#if BW_DEBUG
		paletteram[offset+0x40] = palatch;
	#endif
}

/******************************************************************************/
/* Initializations*/

#define BW_SET_TILE_INFO(GFX, CODE, COLOR) { \
	tile_info.tile_number = (CODE); \
	tile_info.pen_data = GFX->gfxdata + (CODE) * GFX->char_modulo; \
	tile_info.pal_data = &GFX->colortable[(COLOR) << 3]; \
	tile_info.pen_usage = GFX->pen_usage[(CODE)]; }

static INLINE void get_fgtileinfo(int i)
{
	unsigned code = fgdata[i];
	BW_SET_TILE_INFO(fgfx, code & (BW_NTILES-1), code >> 7)
}

static INLINE void get_bgtileinfo(int i)
{
	unsigned code = bgdata[i];
	BW_SET_TILE_INFO(bgfx, code & (BW_NTILES-1), code >> 7)
}

static INLINE void get_charinfo(int i)
{
	SET_TILE_INFO(0, videoram[i], 0, 0)
}

static INLINE UINT32 bwing_scan_cols(UINT32 col, UINT32 row, UINT32 nc, UINT32 nr)
{
	return((col<<6) + row);
}


VIDEO_START( bwing )
{
	UINT32 *dwptr;
	int i;

	if (!(charmap = tilemap_create(get_charinfo,tilemap_scan_cols,TILEMAP_TRANSPARENT, 8, 8,32,32))) return(1);
	if (!(fgmap = tilemap_create(get_fgtileinfo,bwing_scan_cols,TILEMAP_TRANSPARENT,16,16,64,64))) return(1);
	if (!(bgmap = tilemap_create(get_bgtileinfo,bwing_scan_cols,TILEMAP_OPAQUE,16,16,64,64))) return(1);
	if (!(srxlat = (int *)auto_malloc(0x8000))) return(1);

	scrollmap[0] = fgmap;
	scrollmap[1] = bgmap;
	tilemap_set_transparent_pen(charmap, 0);
	tilemap_set_transparent_pen(fgmap, 0);

	fill_srxlat(srxlat);

	fgdata = memory_region(REGION_USER1);
	bgdata = fgdata + 0x1000;

	for (i=0; i<4; i++) srbase[i] = fgdata + i * 0x2000;
	for (i=0; i<8; i++) sreg[i] = 0;

	fgfx = Machine->gfx[2];
	bgfx = Machine->gfx[3];

	if ((dwptr = fgfx->pen_usage))
	{
		dwptr[0] = 0;
		for(i=1; i<BW_NTILES; i++) dwptr[i] = -1;
	}

	return(0);
}

/******************************************************************************/
/* Realtime*/

static void bwing_drawsprites(struct mame_bitmap *bmp, const struct rectangle *clip, data8_t *ram, int pri)
{
	int attrib, fx, fy, code, x, y, color, i;
	struct GfxElement *gfx = Machine->gfx[1];

	for (i=0; i<0x200; i+=4)
	{
		attrib = ram[i];   if (!(attrib & 1) || (color = attrib>>3 & 1) != pri) continue;
		code   = ram[i+1]; code += attrib<<3 & 0x100;
		y      = ram[i+2]; y -= attrib<<1 & 0x100;
		x      = ram[i+3]; x -= attrib<<2 & 0x100;
		fx = attrib & 0x04;
		fy = ~attrib & 0x02;

		/* normal/cocktail*/
		if (mapmask & 0x20) { fx = !fx; fy = !fy; x = 240 - x; y = 240 - y; }

		/* single/double*/
		if (!(attrib & 0x10))
			drawgfx(bmp, gfx, code, color, fx, fy, x, y, clip, TRANSPARENCY_PEN, 0);
		else
			drawgfxzoom(bmp, gfx, code, color, fx, fy, x, y, clip, TRANSPARENCY_PEN, 0, 1<<16, 2<<16);
	}
}


VIDEO_UPDATE( bwing )
{
	unsigned x, y, shiftx;

	if (mapmask & 0x20)
		{ mapflip = TILEMAP_FLIPX; shiftx = -8; }
	else
		{ mapflip = TILEMAP_FLIPY; shiftx = 8; }

	/* draw background*/
	if (!(mapmask & 1))
	{
		tilemap_set_flip(bgmap, mapflip);
		x = ((sreg[1]<<2 & 0x300) + sreg[2] + shiftx) & 0x3ff;
		tilemap_set_scrollx(bgmap, 0, x);
		y = (sreg[1]<<4 & 0x300) + sreg[3];
		tilemap_set_scrolly(bgmap, 0, y);
		tilemap_draw(bitmap, cliprect, bgmap, 0, 0);
	}
	else
		fillbitmap(bitmap, get_black_pen(), cliprect);

	/* draw low priority sprites*/
	bwing_drawsprites(bitmap, cliprect, buffered_spriteram, 0);

	/* draw foreground*/
	if (!(mapmask & 2))
	{
		tilemap_set_flip(fgmap, mapflip);
		x = ((sreg[1]<<6 & 0x300) + sreg[4] + shiftx) & 0x3ff;
		tilemap_set_scrollx(fgmap, 0, x);
		y = (sreg[1]<<8 & 0x300) + sreg[5];
		tilemap_set_scrolly(fgmap, 0, y);
		tilemap_draw(bitmap, cliprect, fgmap, 0, 0);
	}

	/* draw high priority sprites*/
	bwing_drawsprites(bitmap, cliprect, buffered_spriteram, 1);

	/* draw text layer*/
/*	if (mapmask & 4)*/
	{
		tilemap_set_flip(charmap, mapflip);
		tilemap_draw(bitmap, cliprect, charmap, 0, 0);
	}
}

/******************************************************************************/
