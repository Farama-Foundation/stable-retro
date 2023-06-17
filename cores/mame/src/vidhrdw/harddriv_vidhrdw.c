/***************************************************************************

	Hard Drivin' video hardware

****************************************************************************/

#include "vidhrdw/generic.h"
#include "cpu/tms34010/tms34010.h"
#include "cpu/tms34010/34010ops.h"
#include "harddriv.h"



/*************************************
 *
 *	Constants and macros
 *
 *************************************/

#define DISPLAY_SPEEDUPS			0

#ifdef MSB_FIRST
#define MASK(n)			(0xff000000UL >> (((n) ^ 1) * 8))
#else
#define MASK(n)			(0x000000ffUL << ((n) * 8))
#endif


/*************************************
 *
 *	External definitions
 *
 *************************************/

/* externally accessible */
UINT8 hdgsp_multisync;
UINT8 *hdgsp_vram;
data16_t *hdgsp_control_lo;
data16_t *hdgsp_control_hi;
data16_t *hdgsp_paletteram_lo;
data16_t *hdgsp_paletteram_hi;
size_t hdgsp_vram_size;



/*************************************
 *
 *	Static globals
 *
 *************************************/

static offs_t vram_mask;

static UINT8 shiftreg_enable;

static UINT32 *mask_table;
static UINT8 *gsp_shiftreg_source;

static offs_t gfx_offset;
static offs_t gfx_rowbytes;
static int gfx_offsetscan;
static INT8 gfx_finescroll;
static UINT8 gfx_palettebank;



/*************************************
 *
 *	Start/stop routines
 *
 *************************************/

VIDEO_START( harddriv )
{
	UINT32 *destmask, mask;
	int i;

	shiftreg_enable = 0;

	gfx_offset = 0;
	gfx_rowbytes = 0;
	gfx_offsetscan = 0;
	gfx_finescroll = 0;
	gfx_palettebank = 0;

	/* allocate the mask table */
	mask_table = auto_malloc(sizeof(UINT32) * 4 * 65536);
	if (!mask_table)
		return 1;

	/* fill in the mask table */
	destmask = mask_table;
	for (i = 0; i < 65536; i++)
		if (hdgsp_multisync)
		{
			mask = 0;
			if (i & 0x0001) mask |= MASK(0);
			if (i & 0x0004) mask |= MASK(1);
			if (i & 0x0010) mask |= MASK(2);
			if (i & 0x0040) mask |= MASK(3);
			*destmask++ = mask;

			mask = 0;
			if (i & 0x0100) mask |= MASK(0);
			if (i & 0x0400) mask |= MASK(1);
			if (i & 0x1000) mask |= MASK(2);
			if (i & 0x4000) mask |= MASK(3);
			*destmask++ = mask;
		}
		else
		{
			mask = 0;
			if (i & 0x0001) mask |= MASK(0);
			if (i & 0x0002) mask |= MASK(1);
			if (i & 0x0004) mask |= MASK(2);
			if (i & 0x0008) mask |= MASK(3);
			*destmask++ = mask;

			mask = 0;
			if (i & 0x0010) mask |= MASK(0);
			if (i & 0x0020) mask |= MASK(1);
			if (i & 0x0040) mask |= MASK(2);
			if (i & 0x0080) mask |= MASK(3);
			*destmask++ = mask;

			mask = 0;
			if (i & 0x0100) mask |= MASK(0);
			if (i & 0x0200) mask |= MASK(1);
			if (i & 0x0400) mask |= MASK(2);
			if (i & 0x0800) mask |= MASK(3);
			*destmask++ = mask;

			mask = 0;
			if (i & 0x1000) mask |= MASK(0);
			if (i & 0x2000) mask |= MASK(1);
			if (i & 0x4000) mask |= MASK(2);
			if (i & 0x8000) mask |= MASK(3);
			*destmask++ = mask;
		}

	/* init VRAM pointers */
	vram_mask = hdgsp_vram_size - 1;

	return 0;
}



/*************************************
 *
 *	Shift register access
 *
 *************************************/

void hdgsp_write_to_shiftreg(UINT32 address, UINT16 *shiftreg)
{
	/* access to the 1bpp/2bpp area */
	if (address >= 0x02000000 && address <= 0x020fffff)
	{
		address -= 0x02000000;
		address >>= hdgsp_multisync;
		address &= vram_mask;
		address &= ~((512*8 >> hdgsp_multisync) - 1);
		gsp_shiftreg_source = &hdgsp_vram[address];
	}

	/* access to normal VRAM area */
	else if (address >= 0xff800000 && address <= 0xffffffff)
	{
		address -= 0xff800000;
		address /= 8;
		address &= vram_mask;
		address &= ~511;
		gsp_shiftreg_source = &hdgsp_vram[address];
	}
	else
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Unknown shiftreg write %08X\n", address);
}


void hdgsp_read_from_shiftreg(UINT32 address, UINT16 *shiftreg)
{
	if (!shiftreg_enable)
		return;

	/* access to the 1bpp/2bpp area */
	if (address >= 0x02000000 && address <= 0x020fffff)
	{
		address -= 0x02000000;
		address >>= hdgsp_multisync;
		address &= vram_mask;
		address &= ~((512*8 >> hdgsp_multisync) - 1);
		memmove(&hdgsp_vram[address], gsp_shiftreg_source, 512*8 >> hdgsp_multisync);
	}

	/* access to normal VRAM area */
	else if (address >= 0xff800000 && address <= 0xffffffff)
	{
		address -= 0xff800000;
		address /= 8;
		address &= vram_mask;
		address &= ~511;
		memmove(&hdgsp_vram[address], gsp_shiftreg_source, 512);
	}
	else
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Unknown shiftreg read %08X\n", address);
}



/*************************************
 *
 *	TMS34010 update callback
 *
 *************************************/

void hdgsp_display_update(UINT32 offs, int rowbytes, int scanline)
{
	if (scanline == 0) scanline--;
	force_partial_update(scanline);
	gfx_offset = offs >> hdgsp_multisync;
	gfx_offsetscan = scanline + 1;
	gfx_rowbytes = rowbytes >> hdgsp_multisync;
}



/*************************************
 *
 *	Palette bank updating
 *
 *************************************/

static void update_palette_bank(int newbank)
{
	if (gfx_palettebank != newbank)
	{
		force_partial_update(cpu_getscanline());
		gfx_palettebank = newbank;
	}
}



/*************************************
 *
 *	Video control registers (lo)
 *
 *************************************/

READ16_HANDLER( hdgsp_control_lo_r )
{
	return hdgsp_control_lo[offset];
}


WRITE16_HANDLER( hdgsp_control_lo_w )
{
	int oldword = hdgsp_control_lo[offset];
	int newword;

	COMBINE_DATA(&hdgsp_control_lo[offset]);
	newword = hdgsp_control_lo[offset];

	if (oldword != newword && offset != 0)
		log_cb(RETRO_LOG_DEBUG, LOGPRE "GSP:hdgsp_control_lo(%X)=%04X\n", offset, newword);
}



/*************************************
 *
 *	Video control registers (hi)
 *
 *************************************/

READ16_HANDLER( hdgsp_control_hi_r )
{
	return hdgsp_control_hi[offset];
}


WRITE16_HANDLER( hdgsp_control_hi_w )
{
	int val = (offset >> 3) & 1;

	int oldword = hdgsp_control_hi[offset];
	int newword;

	COMBINE_DATA(&hdgsp_control_hi[offset]);
	newword = hdgsp_control_hi[offset];

	switch (offset & 7)
	{
		case 0x00:
			shiftreg_enable = val;
			break;

		case 0x01:
			data = data & (15 >> hdgsp_multisync);
			if (gfx_finescroll != data)
			{
				force_partial_update(cpu_getscanline() - 1);
				gfx_finescroll = data;
			}
			break;

		case 0x02:
			update_palette_bank((gfx_palettebank & ~1) | val);
			break;

		case 0x03:
			update_palette_bank((gfx_palettebank & ~2) | (val << 1));
			break;

		case 0x04:
			if (Machine->drv->total_colors >= 256 * 8)
				update_palette_bank((gfx_palettebank & ~4) | (val << 2));
			break;

		case 0x07:
			/* LED */
			break;

		default:
			if (oldword != newword)
				log_cb(RETRO_LOG_DEBUG, LOGPRE "GSP:hdgsp_control_hi_w(%X)=%04X\n", offset, newword);
			break;
	}
}



/*************************************
 *
 *	Video RAM expanders
 *
 *************************************/

READ16_HANDLER( hdgsp_vram_2bpp_r )
{
	return 0;
}


WRITE16_HANDLER( hdgsp_vram_1bpp_w )
{
	UINT32 *dest = (UINT32 *)&hdgsp_vram[offset * 16];
	UINT32 *mask = &mask_table[data * 4];
	UINT32 color = hdgsp_control_lo[0] & 0xff;
	UINT32 curmask;

	color |= color << 8;
	color |= color << 16;

	curmask = *mask++;
	*dest = (*dest & ~curmask) | (color & curmask);
	dest++;

	curmask = *mask++;
	*dest = (*dest & ~curmask) | (color & curmask);
	dest++;

	curmask = *mask++;
	*dest = (*dest & ~curmask) | (color & curmask);
	dest++;

	curmask = *mask++;
	*dest = (*dest & ~curmask) | (color & curmask);
	dest++;
}


WRITE16_HANDLER( hdgsp_vram_2bpp_w )
{
	UINT32 *dest = (UINT32 *)&hdgsp_vram[offset * 8];
	UINT32 *mask = &mask_table[data * 2];
	UINT32 color = hdgsp_control_lo[0];
	UINT32 curmask;

	color |= color << 16;

	curmask = *mask++;
	*dest = (*dest & ~curmask) | (color & curmask);
	dest++;

	curmask = *mask++;
	*dest = (*dest & ~curmask) | (color & curmask);
	dest++;
}



/*************************************
 *
 *	Palette registers (lo)
 *
 *************************************/

static INLINE void gsp_palette_change(int offset)
{
	int red = (hdgsp_paletteram_lo[offset] >> 8) & 0xff;
	int green = hdgsp_paletteram_lo[offset] & 0xff;
	int blue = hdgsp_paletteram_hi[offset] & 0xff;
	palette_set_color(offset, red, green, blue);
}


READ16_HANDLER( hdgsp_paletteram_lo_r )
{
	/* note that the palette is only accessed via the first 256 entries */
	/* others are selected via the palette bank */
	offset = gfx_palettebank * 0x100 + (offset & 0xff);

	return hdgsp_paletteram_lo[offset];
}


WRITE16_HANDLER( hdgsp_paletteram_lo_w )
{
	/* note that the palette is only accessed via the first 256 entries */
	/* others are selected via the palette bank */
	offset = gfx_palettebank * 0x100 + (offset & 0xff);

	COMBINE_DATA(&hdgsp_paletteram_lo[offset]);
	gsp_palette_change(offset);
}



/*************************************
 *
 *	Palette registers (hi)
 *
 *************************************/

READ16_HANDLER( hdgsp_paletteram_hi_r )
{
	/* note that the palette is only accessed via the first 256 entries */
	/* others are selected via the palette bank */
	offset = gfx_palettebank * 0x100 + (offset & 0xff);

	return hdgsp_paletteram_hi[offset];
}


WRITE16_HANDLER( hdgsp_paletteram_hi_w )
{
	/* note that the palette is only accessed via the first 256 entries */
	/* others are selected via the palette bank */
	offset = gfx_palettebank * 0x100 + (offset & 0xff);

	COMBINE_DATA(&hdgsp_paletteram_hi[offset]);
	gsp_palette_change(offset);
}



/*************************************
 *
 *	End of frame routine
 *
 *************************************/

VIDEO_EOF( harddriv )
{
	/* reset the display offset */
	gfx_offsetscan = 0;
}



/*************************************
 *
 *	Core refresh routine
 *
 *************************************/

VIDEO_UPDATE( harddriv )
{
	pen_t *pens = &Machine->pens[gfx_palettebank * 256];
	pen_t black = get_black_pen();
	offs_t adjusted_offs;
	int start, end, x, y;
	int lzero, rzero, draw;

	/* check for disabled video */
	if (tms34010_io_display_blanked(1))
	{
		fillbitmap(bitmap, black, cliprect);
		return;
	}

	/* gross! they use start/end of HBLANK to adjust the position of screen elements */
	cpuintrf_push_context(hdcpu_gsp);
	start = tms34010_io_register_r(REG_HEBLNK, 0) * (4 >> hdgsp_multisync) - gfx_finescroll;
	end = tms34010_io_register_r(REG_HSBLNK, 0) * (4 >> hdgsp_multisync) - (15 >> hdgsp_multisync);
	cpuintrf_pop_context();

	/* compute how many pixels to blank and render */
	lzero = start - cliprect->min_x;
	draw = end - start;
	rzero = cliprect->max_x + 1 - end;
	adjusted_offs = gfx_offset;

	/* clip as necessary */
	if (lzero < 0)
	{
		draw += lzero;
		adjusted_offs -= lzero;
		lzero = 0;
	}
	if (rzero < 0)
	{
		draw += rzero;
		rzero = 0;
	}

	/* loop over scanlines */
	for (y = cliprect->min_y; y <= cliprect->max_y; y++)
	{
		UINT32 offset = adjusted_offs + gfx_rowbytes * (y - gfx_offsetscan);
		UINT16 *dest = (UINT16 *)bitmap->base + y * bitmap->rowpixels + cliprect->min_x;

		/* render */
		for (x = 0; x < lzero; x++)
			*dest++ = black;
		for (x = 0; x < draw; x++)
			*dest++ = pens[hdgsp_vram[BYTE_XOR_LE(offset++) & vram_mask]];
		for (x = 0; x < rzero; x++)
			*dest++ = black;
	}

	/* update the speedup counts */
#if DISPLAY_SPEEDUPS
if (cliprect->min_y < 30)
{
	char temp[200];
	sprintf(temp, "GSP:%d/%d/%d/%d",
			gsp_speedup_count[0], gsp_speedup_count[1],
			gsp_speedup_count[2], gsp_speedup_count[3]);
	ui_text(bitmap, temp, 0, 0);
	sprintf(temp, "MSP:%d/%d/%d/%d",
			msp_speedup_count[0], msp_speedup_count[1],
			msp_speedup_count[2], msp_speedup_count[3]);
	ui_text(bitmap, temp, 0, 10);
	sprintf(temp, "ADSP:%d/%d/%d/%d",
			adsp_speedup_count[0], adsp_speedup_count[1],
			adsp_speedup_count[2], adsp_speedup_count[3]);
	ui_text(bitmap, temp, 0, 20);
}
#endif
}
