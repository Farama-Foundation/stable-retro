/*************************************************************************

	Williams/Midway Y/Z-unit system

**************************************************************************/

#include "driver.h"
#include "cpu/tms34010/tms34010.h"
#include "cpu/tms34010/34010ops.h"
#include "midyunit.h"


/* compile-time options */
#define FAST_DMA			1		/* DMAs complete immediately; reduces number of CPU switches */
#define LOG_DMA				0		/* DMAs are logged if the 'L' key is pressed */


/* constants for the DMA chip */
enum
{
	DMA_COMMAND = 0,
	DMA_ROWBYTES,
	DMA_OFFSETLO,
	DMA_OFFSETHI,
	DMA_XSTART,
	DMA_YSTART,
	DMA_WIDTH,
	DMA_HEIGHT,
	DMA_PALETTE,
	DMA_COLOR
};



/* graphics-related variables */
       UINT8 *	midyunit_gfx_rom;
       size_t	midyunit_gfx_rom_size;
static UINT8	autoerase_enable;

/* palette-related variables */
static UINT32	palette_mask;
static pen_t *	pen_map;

/* videoram-related variables */
static UINT16 *	local_videoram;
static UINT8	videobank_select;

/* update-related variables */
static int		last_update_scanline;

/* DMA-related variables */
static data16_t dma_register[16];
static struct
{
	UINT32		offset;			/* source offset, in bits */
	INT32 		rowbytes;		/* source bytes to skip each row */
	INT32 		xpos;			/* x position, clipped */
	INT32		ypos;			/* y position, clipped */
	INT32		width;			/* horizontal pixel count */
	INT32		height;			/* vertical pixel count */
	UINT16		palette;		/* palette base */
	UINT16		color;			/* current foreground color with palette */
} dma_state;



/* prototypes */
static void update_partial(int scanline, int render);
static void scanline0_callback(int param);



/*************************************
 *
 *	Video startup
 *
 *************************************/

static VIDEO_START( common )
{
	/* allocate memory */
	midyunit_cmos_ram = auto_malloc(0x2000 * 4);
	local_videoram = auto_malloc(0x80000);
	pen_map = auto_malloc(65536 * sizeof(pen_map[0]));

	/* handle failure */
	if (!midyunit_cmos_ram || !local_videoram || !pen_map)
		return 1;

	/* we have to erase this since we rely on upper bits being 0 */
	memset(local_videoram, 0, 0x80000);

	/* reset all the globals */
	midyunit_cmos_page = 0;
	autoerase_enable = 0;
	last_update_scanline = 0;

	/* reset DMA state */
	memset(dma_register, 0, sizeof(dma_register));
	memset(&dma_state, 0, sizeof(dma_state));

	/* set up scanline 0 timer */
	timer_set(cpu_getscanlinetime(0), 0, scanline0_callback);
	return 0;
}


VIDEO_START( midyunit_4bit )
{
	int result = video_start_common();
	int i;

	/* handle failure */
	if (result)
		return result;

	/* init for 4-bit */
	for (i = 0; i < 65536; i++)
		pen_map[i] = ((i & 0xf000) >> 8) | (i & 0x000f);
	palette_mask = 0x00ff;

	return 0;
}


VIDEO_START( midyunit_6bit )
{
	int result = video_start_common();
	int i;

	/* handle failure */
	if (result)
		return result;

	/* init for 6-bit */
	for (i = 0; i < 65536; i++)
		pen_map[i] = ((i & 0xc000) >> 8) | (i & 0x0f3f);
	palette_mask = 0x0fff;

	return 0;
}


VIDEO_START( midzunit )
{
	int result = video_start_common();
	int i;

	/* handle failure */
	if (result)
		return result;

	/* init for 8-bit */
	for (i = 0; i < 65536; i++)
		pen_map[i] = i & 0x1fff;
	palette_mask = 0x1fff;

	return 0;
}



/*************************************
 *
 *	Banked graphics ROM access
 *
 *************************************/

READ16_HANDLER( midyunit_gfxrom_r )
{
	offset *= 2;
	if (palette_mask == 0x00ff)
		return midyunit_gfx_rom[offset] | (midyunit_gfx_rom[offset] << 4) |
				(midyunit_gfx_rom[offset + 1] << 8) | (midyunit_gfx_rom[offset + 1] << 12);
	else
		return midyunit_gfx_rom[offset] | (midyunit_gfx_rom[offset + 1] << 8);
}



/*************************************
 *
 *	Video/color RAM read/write
 *
 *************************************/

WRITE16_HANDLER( midyunit_vram_w )
{
	offset *= 2;
	if (videobank_select)
	{
		if (ACCESSING_LSB)
			local_videoram[offset] = (data & 0x00ff) | (dma_register[DMA_PALETTE] << 8);
		if (ACCESSING_MSB)
			local_videoram[offset + 1] = (data >> 8) | (dma_register[DMA_PALETTE] & 0xff00);
	}
	else
	{
		if (ACCESSING_LSB)
			local_videoram[offset] = (local_videoram[offset] & 0x00ff) | (data << 8);
		if (ACCESSING_MSB)
			local_videoram[offset + 1] = (local_videoram[offset + 1] & 0x00ff) | (data & 0xff00);
	}
}


READ16_HANDLER( midyunit_vram_r )
{
	offset *= 2;
	if (videobank_select)
		return (local_videoram[offset] & 0x00ff) | (local_videoram[offset + 1] << 8);
	else
		return (local_videoram[offset] >> 8) | (local_videoram[offset + 1] & 0xff00);
}



/*************************************
 *
 *	Shift register read/write
 *
 *************************************/

void midyunit_to_shiftreg(UINT32 address, UINT16 *shiftreg)
{
	memcpy(shiftreg, &local_videoram[address >> 3], 2 * 512 * sizeof(UINT16));
}


void midyunit_from_shiftreg(UINT32 address, UINT16 *shiftreg)
{
	memcpy(&local_videoram[address >> 3], shiftreg, 2 * 512 * sizeof(UINT16));
}



/*************************************
 *
 *	Y/Z-unit control register
 *
 *************************************/

WRITE16_HANDLER( midyunit_control_w )
{
	/*
	 * Narc system register
	 * ------------------
	 *
	 *   | Bit              | Use
	 * --+-FEDCBA9876543210-+------------
	 *   | xxxxxxxx-------- |   7 segment led on CPU board
	 *   | --------xx------ |   CMOS page
	 *   | ----------x----- | - OBJ PAL RAM select
	 *   | -----------x---- | - autoerase enable
	 *   | ---------------- | - watchdog
	 *
	 */

	if (ACCESSING_LSB)
	{
		/* CMOS page is bits 6-7 */
		midyunit_cmos_page = ((data >> 6) & 3) * 0x1000;

		/* video bank select is bit 5 */
		videobank_select = (data >> 5) & 1;

		/* handle autoerase disable (bit 4) */
		if (data & 0x10)
		{
			if (autoerase_enable)
			{
				log_cb(RETRO_LOG_DEBUG, LOGPRE "autoerase off @ %d\n", cpu_getscanline());
				update_partial(cpu_getscanline() - 1, 1);
			}
			autoerase_enable = 0;
		}

		/* handle autoerase enable (bit 4)  */
		else
		{
			if (!autoerase_enable)
			{
				log_cb(RETRO_LOG_DEBUG, LOGPRE "autoerase on @ %d\n", cpu_getscanline());
				update_partial(cpu_getscanline() - 1, 1);
			}
			autoerase_enable = 1;
		}
	}
}



/*************************************
 *
 *	Palette handlers
 *
 *************************************/

WRITE16_HANDLER( midyunit_paletteram_w )
{
	COMBINE_DATA(&paletteram16[offset]);
	palette_set_color(offset & palette_mask, pal5bit(data >> 10), pal5bit(data >> 5), pal5bit(data >> 0));
}



/*************************************
 *
 *	DMA drawing routines
 *
 *************************************/

/*** constant definitions ***/
#define	PIXEL_SKIP		0
#define PIXEL_COLOR		1
#define PIXEL_COPY		2

#define XFLIP_NO		0
#define XFLIP_YES		1


typedef void (*dma_draw_func)(void);


/*** core blitter routine macro ***/
#define DMA_DRAW_FUNC_BODY(name, xflip, zero, nonzero)				 			\
{																				\
	int height = dma_state.height;												\
	int width = dma_state.width;												\
	UINT8 *base = midyunit_gfx_rom;													\
	UINT32 offset = dma_state.offset >> 3;										\
	UINT16 pal = dma_state.palette;												\
	UINT16 color = pal | dma_state.color;										\
	int x, y;																	\
																				\
	/* loop over the height */													\
	for (y = 0; y < height; y++)												\
	{																			\
		int tx = dma_state.xpos;												\
		int ty = dma_state.ypos;												\
		UINT32 o = offset;														\
		UINT16 *dest;															\
																				\
		/* determine Y position */												\
		ty = (ty + y) & 0x1ff;													\
		offset += dma_state.rowbytes;											\
																				\
		/* determine destination pointer */										\
		dest = &local_videoram[ty * 512];										\
																				\
		/* loop until we draw the entire width */								\
		for (x = 0; x < width; x++, o++)										\
		{																		\
			/* special case similar handling of zero/non-zero */				\
			if (zero == nonzero)												\
			{																	\
				if (zero == PIXEL_COLOR)										\
					dest[tx] = color;											\
				else if (zero == PIXEL_COPY)									\
					dest[tx] = base[o] | pal;									\
			}																	\
																				\
			/* otherwise, read the pixel and look */							\
			else																\
			{																	\
				int pixel = base[o];											\
																				\
				/* non-zero pixel case */										\
				if (pixel)														\
				{																\
					if (nonzero == PIXEL_COLOR)									\
						dest[tx] = color;										\
					else if (nonzero == PIXEL_COPY)								\
						dest[tx] = pixel | pal;									\
				}																\
																				\
				/* zero pixel case */											\
				else															\
				{																\
					if (zero == PIXEL_COLOR)									\
						dest[tx] = color;										\
					else if (zero == PIXEL_COPY)								\
						dest[tx] = pal;											\
				}																\
			}																	\
																				\
			/* update pointers */												\
			if (xflip) tx--;													\
			else tx++;															\
		}																		\
	}																			\
}

/*** slightly simplified one for most blitters ***/
#define DMA_DRAW_FUNC(name, xflip, zero, nonzero)						\
static void name(void)													\
{																		\
	DMA_DRAW_FUNC_BODY(name, xflip, zero, nonzero)						\
}

/*** empty blitter ***/
static void dma_draw_none(void)
{
}

/*** super macro for declaring an entire blitter family ***/
#define DECLARE_BLITTER_SET(prefix)										\
DMA_DRAW_FUNC(prefix##_p0,      XFLIP_NO,  PIXEL_COPY,  PIXEL_SKIP)		\
DMA_DRAW_FUNC(prefix##_p1,      XFLIP_NO,  PIXEL_SKIP,  PIXEL_COPY)		\
DMA_DRAW_FUNC(prefix##_c0,      XFLIP_NO,  PIXEL_COLOR, PIXEL_SKIP)		\
DMA_DRAW_FUNC(prefix##_c1,      XFLIP_NO,  PIXEL_SKIP,  PIXEL_COLOR)	\
DMA_DRAW_FUNC(prefix##_p0p1,    XFLIP_NO,  PIXEL_COPY,  PIXEL_COPY)		\
DMA_DRAW_FUNC(prefix##_c0c1,    XFLIP_NO,  PIXEL_COLOR, PIXEL_COLOR)	\
DMA_DRAW_FUNC(prefix##_c0p1,    XFLIP_NO,  PIXEL_COLOR, PIXEL_COPY)		\
DMA_DRAW_FUNC(prefix##_p0c1,    XFLIP_NO,  PIXEL_COPY,  PIXEL_COLOR)	\
																		\
DMA_DRAW_FUNC(prefix##_p0_xf,   XFLIP_YES, PIXEL_COPY,  PIXEL_SKIP)		\
DMA_DRAW_FUNC(prefix##_p1_xf,   XFLIP_YES, PIXEL_SKIP,  PIXEL_COPY)		\
DMA_DRAW_FUNC(prefix##_c0_xf,   XFLIP_YES, PIXEL_COLOR, PIXEL_SKIP)		\
DMA_DRAW_FUNC(prefix##_c1_xf,   XFLIP_YES, PIXEL_SKIP,  PIXEL_COLOR)	\
DMA_DRAW_FUNC(prefix##_p0p1_xf, XFLIP_YES, PIXEL_COPY,  PIXEL_COPY)		\
DMA_DRAW_FUNC(prefix##_c0c1_xf, XFLIP_YES, PIXEL_COLOR, PIXEL_COLOR)	\
DMA_DRAW_FUNC(prefix##_c0p1_xf, XFLIP_YES, PIXEL_COLOR, PIXEL_COPY)		\
DMA_DRAW_FUNC(prefix##_p0c1_xf, XFLIP_YES, PIXEL_COPY,  PIXEL_COLOR)	\
																												\
static dma_draw_func prefix[32] =																				\
{																												\
/*	B0:N / B1:N			B0:Y / B1:N			B0:N / B1:Y			B0:Y / B1:Y */									\
	dma_draw_none,		prefix##_p0,		prefix##_p1,		prefix##_p0p1,		/* no color */ 				\
	prefix##_c0,		prefix##_c0,		prefix##_c0p1,		prefix##_c0p1,		/* color 0 pixels */		\
	prefix##_c1,		prefix##_p0c1,		prefix##_c1,		prefix##_p0c1,		/* color non-0 pixels */	\
	prefix##_c0c1,		prefix##_c0c1,		prefix##_c0c1,		prefix##_c0c1,		/* fill */ 					\
																												\
	dma_draw_none,		prefix##_p0_xf,		prefix##_p1_xf,		prefix##_p0p1_xf,	/* no color */ 				\
	prefix##_c0_xf,		prefix##_c0_xf,		prefix##_c0p1_xf,	prefix##_c0p1_xf,	/* color 0 pixels */ 		\
	prefix##_c1_xf,		prefix##_p0c1_xf,	prefix##_c1_xf,		prefix##_p0c1_xf,	/* color non-0 pixels */	\
	prefix##_c0c1_xf,	prefix##_c0c1_xf,	prefix##_c0c1_xf,	prefix##_c0c1_xf	/* fill */ 					\
};

/* allow for custom blitters */
#ifdef MIDYUNIT_CUSTOM_BLITTERS
#include "midyblit.c"
#endif

/*** blitter family declarations ***/
DECLARE_BLITTER_SET(dma_draw)



/*************************************
 *
 *	DMA finished callback
 *
 *************************************/

static int temp_irq_callback(int irqline)
{
	tms34010_set_irq_line(0, CLEAR_LINE);
	return 0;
}


static void dma_callback(int is_in_34010_context)
{
	dma_register[DMA_COMMAND] &= ~0x8000; /* tell the cpu we're done */
	if (is_in_34010_context)
	{
		tms34010_set_irq_callback(temp_irq_callback);
		tms34010_set_irq_line(0, ASSERT_LINE);
	}
	else
		cpu_set_irq_line(0, 0, HOLD_LINE);
}



/*************************************
 *
 *	DMA reader
 *
 *************************************/

READ16_HANDLER( midyunit_dma_r )
{
	int result = dma_register[offset];

#if !FAST_DMA
	/* see if we're done */
	if (offset == DMA_COMMAND && (result & 0x8000))
		switch (activecpu_get_pc())
		{
			case 0xfff7aa20: /* narc */
			case 0xffe1c970: /* trog */
			case 0xffe1c9a0: /* trog3 */
			case 0xffe1d4a0: /* trogp */
			case 0xffe07690: /* smashtv */
			case 0xffe00450: /* hiimpact */
			case 0xffe14930: /* strkforc */
			case 0xffe02c20: /* strkforc */
			case 0xffc79890: /* mk */
			case 0xffc7a5a0: /* mk */
			case 0xffc063b0: /* term2 */
			case 0xffc00720: /* term2 */
			case 0xffc07a60: /* totcarn/totcarnp */
			case 0xff805200: /* mk2 */
			case 0xff8044e0: /* mk2 */
			case 0xff82e200: /* nbajam */
				cpu_spinuntil_int();
				break;
		}
#endif

	return result;
}



/*************************************
 *
 *	DMA write handler
 *
 *************************************/

/*
 * DMA registers
 * ------------------
 *
 *  Register | Bit              | Use
 * ----------+-FEDCBA9876543210-+------------
 *     0     | x--------------- | trigger write (or clear if zero)
 *           | ---184-1-------- | unknown
 *           | ----------x----- | flip y
 *           | -----------x---- | flip x
 *           | ------------x--- | blit nonzero pixels as color
 *           | -------------x-- | blit zero pixels as color
 *           | --------------x- | blit nonzero pixels
 *           | ---------------x | blit zero pixels
 *     1     | xxxxxxxxxxxxxxxx | width offset
 *     2     | xxxxxxxxxxxxxxxx | source address low word
 *     3     | xxxxxxxxxxxxxxxx | source address high word
 *     4     | xxxxxxxxxxxxxxxx | detination x
 *     5     | xxxxxxxxxxxxxxxx | destination y
 *     6     | xxxxxxxxxxxxxxxx | image columns
 *     7     | xxxxxxxxxxxxxxxx | image rows
 *     8     | xxxxxxxxxxxxxxxx | palette
 *     9     | xxxxxxxxxxxxxxxx | color
 */

WRITE16_HANDLER( midyunit_dma_w )
{
	UINT32 gfxoffset;
	int command;

	/* blend with the current register contents */
	COMBINE_DATA(&dma_register[offset]);

	/* only writes to DMA_COMMAND actually cause actions */
	if (offset != DMA_COMMAND)
		return;

	/* high bit triggers action */
	command = dma_register[DMA_COMMAND];
	if (!(command & 0x8000))
	{
		tms34010_set_irq_line(0, CLEAR_LINE);
		return;
	}

#if LOG_DMA
	if (keyboard_pressed(KEYCODE_L))
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "----\n");
		logerror("DMA command %04X: (xflip=%d yflip=%d)\n",
				command, (command >> 4) & 1, (command >> 5) & 1);
		logerror("  offset=%08X pos=(%d,%d) w=%d h=%d\n",
				dma_register[DMA_OFFSETLO] | (dma_register[DMA_OFFSETHI] << 16),
				(INT16)dma_register[DMA_XSTART], (INT16)dma_register[DMA_YSTART],
				dma_register[DMA_WIDTH], dma_register[DMA_HEIGHT]);
		logerror("  palette=%04X color=%04X\n",
				dma_register[DMA_PALETTE], dma_register[DMA_COLOR]);
	}
#endif

	profiler_mark(PROFILER_USER1);

	/* fill in the basic data */
	dma_state.rowbytes = (INT16)dma_register[DMA_ROWBYTES];
	dma_state.xpos = (INT16)dma_register[DMA_XSTART];
	dma_state.ypos = (INT16)dma_register[DMA_YSTART];
	dma_state.width = dma_register[DMA_WIDTH];
	dma_state.height = dma_register[DMA_HEIGHT];
	dma_state.palette = dma_register[DMA_PALETTE] << 8;
	dma_state.color = dma_register[DMA_COLOR] & 0xff;

	/* determine the offset and adjust the rowbytes */
	gfxoffset = dma_register[DMA_OFFSETLO] | (dma_register[DMA_OFFSETHI] << 16);
	if (command & 0x10)
	{
		gfxoffset -= (dma_state.width - 1) * 8;
		dma_state.rowbytes = (dma_state.rowbytes - dma_state.width + 3) & ~3;
		dma_state.xpos += dma_state.width - 1;
	}
	else
		dma_state.rowbytes = (dma_state.rowbytes + dma_state.width + 3) & ~3;

	/* apply Y clipping */
	if (dma_state.ypos < 0)
	{
		dma_state.height -= -dma_state.ypos;
		dma_state.offset += (-dma_state.ypos * dma_state.rowbytes) << 3;
		dma_state.ypos = 0;
	}
	if (dma_state.ypos + dma_state.height > 512)
		dma_state.height = 512 - dma_state.ypos;

	/* apply X clipping */
	if (!(command & 0x10))
	{
		if (dma_state.xpos < 0)
		{
			dma_state.width -= -dma_state.xpos;
			dma_state.offset += -dma_state.xpos << 3;
			dma_state.xpos = 0;
		}
		if (dma_state.xpos + dma_state.width > 512)
			dma_state.width = 512 - dma_state.xpos;
	}
	else
	{
		if (dma_state.xpos >= 512)
		{
			dma_state.width -= dma_state.xpos - 511;
			dma_state.offset += (dma_state.xpos - 511) << 3;
			dma_state.xpos = 511;
		}
		if (dma_state.xpos - dma_state.width < 0)
			dma_state.width = dma_state.xpos;
	}

	/* special case: drawing mode C doesn't need to know about any pixel data */
	/* shimpact relies on this behavior */
	if ((command & 0x0f) == 0x0c)
		gfxoffset = 0;

	/* determine the location and draw */
	if (gfxoffset < 0x02000000)
		gfxoffset += 0x02000000;
	if (gfxoffset < 0x06000000)
	{
		dma_state.offset = gfxoffset - 0x02000000;
		(*dma_draw[command & 0x1f])();
	}

	/* signal we're done */
	if (FAST_DMA)
		dma_callback(1);
	else
		timer_set(TIME_IN_NSEC(41 * dma_state.width * dma_state.height), 0, dma_callback);

	profiler_mark(PROFILER_END);
}



/*************************************
 *
 *	Partial screen updater
 *
 *************************************/

static void update_partial(int scanline, int render)
{
	/* force a partial refresh */
	if (render)
		force_partial_update(scanline);

	/* if no autoerase is enabled, quit now */
	if (autoerase_enable)
	{
		int starty, stopy;
		int v, width, xoffs;
		UINT32 offset;

		/* autoerase the lines here */
		starty = (last_update_scanline > Machine->visible_area.min_y) ? last_update_scanline : Machine->visible_area.min_y;
		stopy = (scanline < Machine->visible_area.max_y) ? scanline : Machine->visible_area.max_y;

		/* determine the base of the videoram */
		offset = (~tms34010_get_DPYSTRT(0) & 0x1ff0) << 5;
		offset += 512 * (starty - Machine->visible_area.min_y);

		/* determine how many pixels to copy */
		xoffs = Machine->visible_area.min_x;
		width = Machine->visible_area.max_x - xoffs + 1;
		offset += xoffs;

		/* loop over rows */
		for (v = starty; v <= stopy; v++)
		{
			memcpy(&local_videoram[offset & 0x3ffff], &local_videoram[510 * 512], width * sizeof(UINT16));
			offset += 512;
		}
	}

	/* remember where we left off */
	last_update_scanline = scanline + 1;
}



/*************************************
 *
 *	34010 display address callback
 *
 *************************************/

void midyunit_display_addr_changed(UINT32 offs, int rowbytes, int scanline)
{
}



/*************************************
 *
 *	34010 interrupt callback
 *
 *************************************/

void midyunit_display_interrupt(int scanline)
{
	update_partial(scanline - 1, 1);
}



/*************************************
 *
 *	34010 I/O register kludge
 *
 *************************************/

WRITE16_HANDLER( midyunit_io_register_w )
{
	/* if the HEBLNK or HSBLNK are changing, force an update */
	if (offset == REG_HSBLNK || offset == REG_HEBLNK)
	{
		int oldval = tms34010_io_register_r(offset, mem_mask);
		if ((data & ~mem_mask) != (oldval & ~mem_mask))
			update_partial(cpu_getscanline(), 1);
	}

	/* pass the write through */
	tms34010_io_register_w(offset, data, mem_mask);
}



/*************************************
 *
 *	End-of-frame update
 *
 *************************************/

VIDEO_EOF( midyunit )
{
	/* finish updating/autoerasing, even if we skipped a frame */
	update_partial(Machine->visible_area.max_y, 0);
}


static void scanline0_callback(int param)
{
	last_update_scanline = 0;
	timer_set(cpu_getscanlinetime(0), 0, scanline0_callback);
}



/*************************************
 *
 *	Core refresh routine
 *
 *************************************/

VIDEO_UPDATE( midyunit )
{
	int hsblnk, heblnk, leftscroll;
	int v, width, xoffs;
	UINT32 offset;

	/* get the current blanking values */
	cpuintrf_push_context(0);
	heblnk = tms34010_io_register_r(REG_HEBLNK, 0);
	hsblnk = tms34010_io_register_r(REG_HSBLNK, 0);
	leftscroll = (Machine->visible_area.max_x + 1 - Machine->visible_area.min_x) - (hsblnk - heblnk) * 2;
	if (leftscroll < 0)
		leftscroll = 0;
	cpuintrf_pop_context();

	/* determine the base of the videoram */
	offset = (~tms34010_get_DPYSTRT(0) & 0x1ff0) << 5;
	offset += 512 * (cliprect->min_y - Machine->visible_area.min_y);

	/* determine how many pixels to copy */
	xoffs = cliprect->min_x;
	width = cliprect->max_x - xoffs + 1;
	offset += xoffs;

	/* adjust for the left scroll hack */
	xoffs += leftscroll;
	width -= leftscroll;

	/* loop over rows */
	for (v = cliprect->min_y; v <= cliprect->max_y; v++)
	{
		draw_scanline16(bitmap, xoffs, v, width, &local_videoram[offset & 0x3ffff], pen_map, -1);
		offset += 512;
	}

	/* blank the left side */
	if (leftscroll > 0)
	{
		struct rectangle erase = *cliprect;
		erase.max_x = leftscroll - 1;
		fillbitmap(bitmap, get_black_pen(), &erase);
	}
}
