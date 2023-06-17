/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/tms34061.h"
#include "vidhrdw/tlc34076.h"
#include "cpu/m6809/m6809.h"
#include "itech8.h"


/*************************************
 *
 *	Debugging
 *
 *************************************/

#define FULL_LOGGING			0
#define BLIT_LOGGING			0
#define INSTANT_BLIT			1



/*************************************
 *
 *	Blitter constants
 *
 *************************************/

#define BLITTER_ADDRHI			blitter_data[0]
#define BLITTER_ADDRLO			blitter_data[1]
#define BLITTER_FLAGS			blitter_data[2]
#define BLITTER_STATUS			blitter_data[3]
#define BLITTER_WIDTH			blitter_data[4]
#define BLITTER_HEIGHT			blitter_data[5]
#define BLITTER_MASK			blitter_data[6]
#define BLITTER_OUTPUT			blitter_data[7]
#define BLITTER_XSTART			blitter_data[8]
#define BLITTER_YCOUNT			blitter_data[9]
#define BLITTER_XSTOP			blitter_data[10]
#define BLITTER_YSKIP			blitter_data[11]

#define BLITFLAG_SHIFT			0x01
#define BLITFLAG_XFLIP			0x02
#define BLITFLAG_YFLIP			0x04
#define BLITFLAG_RLE			0x08
#define BLITFLAG_TRANSPARENT	0x10



/*************************************
 *
 *	Global variables
 *
 *************************************/

data8_t *itech8_grom_bank;
data8_t *itech8_display_page;


static UINT8 blitter_data[16];
static UINT8 blit_in_progress;

static UINT8 slikshot;

static struct tms34061_display tms_state;
static UINT8 *grom_base;
static UINT32 grom_size;



/*************************************
 *
 *	TMS34061 interfacing
 *
 *************************************/

static void generate_interrupt(int state)
{
	itech8_update_interrupts(-1, state, -1);

	if (FULL_LOGGING && state) log_cb(RETRO_LOG_DEBUG, LOGPRE "------------ DISPLAY INT (%d) --------------\n", cpu_getscanline());
}


static struct tms34061_interface tms34061intf =
{
	8,						/* VRAM address is (row << rowshift) | col */
	0x40000,				/* size of video RAM */
	0x100,					/* size of dirty chunks (must be power of 2) */
	generate_interrupt		/* interrupt gen callback */
};



/*************************************
 *
 *	Video start
 *
 *************************************/

VIDEO_START( itech8 )
{
	/* initialize TMS34061 emulation */
    if (tms34061_start(&tms34061intf))
		return 1;

	/* get the TMS34061 display state */
	tms34061_get_display_state(&tms_state);

	/* reset statics */
	slikshot = 0;

	/* fetch the GROM base */
	grom_base = memory_region(REGION_GFX1);
	grom_size = memory_region_length(REGION_GFX1);

	return 0;
}

VIDEO_START( slikshot )
{
	int result = video_start_itech8();
	slikshot = 1;
	return result;
}



/*************************************
 *
 *	Palette I/O
 *
 *************************************/

WRITE_HANDLER( itech8_palette_w )
{
	tlc34076_w(offset/2, data);
}



/*************************************
 *
 *	Low-level blitting primitives
 *
 *************************************/

static INLINE void draw_byte(offs_t addr, UINT8 val, UINT8 mask, UINT8 latch)
{
	tms_state.vram[addr] = val & mask;
	tms_state.latchram[addr] = latch;
}


static INLINE void draw_byte_trans4(offs_t addr, UINT8 val, UINT8 mask, UINT8 latch)
{
	if (!val)
		return;

	if (val & 0xf0)
	{
		if (val & 0x0f)
		{
			tms_state.vram[addr] = val & mask;
			tms_state.latchram[addr] = latch;
		}
		else
		{
			tms_state.vram[addr] = (tms_state.vram[addr] & 0x0f) | (val & mask & 0xf0);
			tms_state.latchram[addr] = (tms_state.latchram[addr] & 0x0f) | (latch & 0xf0);
		}
	}
	else
	{
		tms_state.vram[addr] = (tms_state.vram[addr] & 0xf0) | (val & mask & 0x0f);
		tms_state.latchram[addr] = (tms_state.latchram[addr] & 0xf0) | (latch & 0x0f);
	}
}


static INLINE void draw_byte_trans8(offs_t addr, UINT8 val, UINT8 mask, UINT8 latch)
{
	if (val) draw_byte(addr, val, mask, latch);
}



/*************************************
 *
 *	Low-level shifted blitting primitives
 *
 *************************************/

static INLINE void draw_byte_shift(offs_t addr, UINT8 val, UINT8 mask, UINT8 latch)
{
	tms_state.vram[addr] = (tms_state.vram[addr] & 0xf0) | ((val & mask) >> 4);
	tms_state.latchram[addr] = (tms_state.latchram[addr] & 0xf0) | (latch >> 4);
	tms_state.vram[addr + 1] = (tms_state.vram[addr + 1] & 0x0f) | ((val & mask) << 4);
	tms_state.latchram[addr + 1] = (tms_state.latchram[addr + 1] & 0x0f) | (latch << 4);
}


static INLINE void draw_byte_shift_trans4(offs_t addr, UINT8 val, UINT8 mask, UINT8 latch)
{
	if (!val)
		return;

	if (val & 0xf0)
	{
		tms_state.vram[addr] = (tms_state.vram[addr] & 0xf0) | ((val & mask) >> 4);
		tms_state.latchram[addr] = (tms_state.latchram[addr] & 0xf0) | (latch >> 4);
	}
	if (val & 0x0f)
	{
		tms_state.vram[addr + 1] = (tms_state.vram[addr + 1] & 0x0f) | ((val & mask) << 4);
		tms_state.latchram[addr + 1] = (tms_state.latchram[addr + 1] & 0x0f) | (latch << 4);
	}
}


static INLINE void draw_byte_shift_trans8(offs_t addr, UINT8 val, UINT8 mask, UINT8 latch)
{
	if (val) draw_byte_shift(addr, val, mask, latch);
}



/*************************************
 *
 *	Low-level flipped blitting primitives
 *
 *************************************/

static INLINE void draw_byte_xflip(offs_t addr, UINT8 val, UINT8 mask, UINT8 latch)
{
	val = (val >> 4) | (val << 4);
	draw_byte(addr, val, mask, latch);
}


static INLINE void draw_byte_trans4_xflip(offs_t addr, UINT8 val, UINT8 mask, UINT8 latch)
{
	val = (val >> 4) | (val << 4);
	draw_byte_trans4(addr, val, mask, latch);
}


static INLINE void draw_byte_shift_xflip(offs_t addr, UINT8 val, UINT8 mask, UINT8 latch)
{
	val = (val >> 4) | (val << 4);
	draw_byte_shift(addr, val, mask, latch);
}


static INLINE void draw_byte_shift_trans4_xflip(offs_t addr, UINT8 val, UINT8 mask, UINT8 latch)
{
	val = (val >> 4) | (val << 4);
	draw_byte_shift_trans4(addr, val, mask, latch);
}



/*************************************
 *
 *	Uncompressed blitter macro
 *
 *************************************/

#define DRAW_RAW_MACRO(NAME, TRANSPARENT, OPERATION) 										\
static void NAME(void)																		\
{																							\
	UINT8 *src = &grom_base[((*itech8_grom_bank << 16) | (BLITTER_ADDRHI << 8) | BLITTER_ADDRLO) % grom_size];\
	offs_t addr = tms_state.regs[TMS34061_XYADDRESS] | ((tms_state.regs[TMS34061_XYOFFSET] & 0x300) << 8);\
	int ydir = (BLITTER_FLAGS & BLITFLAG_YFLIP) ? -1 : 1;									\
	int xdir = (BLITTER_FLAGS & BLITFLAG_XFLIP) ? -1 : 1;									\
	int color = tms34061_latch_r(0);														\
	int width = BLITTER_WIDTH;																\
	int height = BLITTER_HEIGHT;															\
	UINT8 mask = BLITTER_MASK;																\
	UINT8 skip[3];																			\
	int x, y;																				\
																							\
	/* compute horiz skip counts */															\
	skip[0] = BLITTER_XSTART;																\
	skip[1] = (width <= BLITTER_XSTOP) ? 0 : width - 1 - BLITTER_XSTOP;						\
	if (xdir == -1) { int temp = skip[0]; skip[0] = skip[1]; skip[1] = temp; }				\
	width -= skip[0] + skip[1];																\
																							\
	/* compute vertical skip counts */														\
	if (ydir == 1)																			\
	{																						\
		skip[2] = (height <= BLITTER_YCOUNT) ? 0 : height - BLITTER_YCOUNT;					\
		if (BLITTER_YSKIP > 1) height -= BLITTER_YSKIP - 1;									\
	}																						\
	else																					\
	{																						\
		skip[2] = (height <= BLITTER_YSKIP) ? 0 : height - BLITTER_YSKIP;					\
		if (BLITTER_YCOUNT > 1) height -= BLITTER_YCOUNT - 1;								\
	}																						\
																							\
	/* skip top */																			\
	for (y = 0; y < skip[2]; y++)															\
	{																						\
		/* skip src and dest */																\
		addr += xdir * (width + skip[0] + skip[1]);											\
		src += width + skip[0] + skip[1];													\
																							\
		/* back up one and reverse directions */											\
		addr -= xdir;																		\
		addr += ydir * 256;																	\
		addr &= 0x3ffff;																	\
		xdir = -xdir;																		\
	}																						\
																							\
	/* loop over height */																	\
	for (y = skip[2]; y < height; y++)														\
	{																						\
		/* skip left */																		\
		addr += xdir * skip[y & 1];															\
		src += skip[y & 1];																	\
																							\
		/* loop over width */																\
		for (x = 0; x < width; x++)															\
		{																					\
			OPERATION(addr, *src++, mask, color);											\
			addr += xdir;																	\
		}																					\
																							\
		/* skip right */																	\
		addr += xdir * skip[~y & 1];														\
		src += skip[~y & 1];																\
																							\
		/* back up one and reverse directions */											\
		addr -= xdir;																		\
		addr += ydir * 256;																	\
		addr &= 0x3ffff;																	\
		xdir = -xdir;																		\
	}																						\
}



/*************************************
 *
 *	Compressed blitter macro
 *
 *************************************/

#define DRAW_RLE_MACRO(NAME, TRANSPARENT, OPERATION) 										\
static void NAME(void)																		\
{																							\
	UINT8 *src = &grom_base[((*itech8_grom_bank << 16) | (BLITTER_ADDRHI << 8) | BLITTER_ADDRLO) % grom_size];\
	offs_t addr = tms_state.regs[TMS34061_XYADDRESS] | ((tms_state.regs[TMS34061_XYOFFSET] & 0x300) << 8);\
	int ydir = (BLITTER_FLAGS & BLITFLAG_YFLIP) ? -1 : 1;									\
	int xdir = (BLITTER_FLAGS & BLITFLAG_XFLIP) ? -1 : 1;									\
	int count = 0, val = -1, innercount;													\
	int color = tms34061_latch_r(0);														\
	int width = BLITTER_WIDTH;																\
	int height = BLITTER_HEIGHT;															\
	UINT8 mask = BLITTER_MASK;																\
	UINT8 skip[3];																			\
	int xleft, y;																			\
																							\
	/* skip past the double-0's */															\
	src += 2;																				\
																							\
	/* compute horiz skip counts */															\
	skip[0] = BLITTER_XSTART;																\
	skip[1] = (width <= BLITTER_XSTOP) ? 0 : width - 1 - BLITTER_XSTOP;						\
	if (xdir == -1) { int temp = skip[0]; skip[0] = skip[1]; skip[1] = temp; }				\
	width -= skip[0] + skip[1];																\
																							\
	/* compute vertical skip counts */														\
	if (ydir == 1)																			\
	{																						\
		skip[2] = (height <= BLITTER_YCOUNT) ? 0 : height - BLITTER_YCOUNT;					\
		if (BLITTER_YSKIP > 1) height -= BLITTER_YSKIP - 1;									\
	}																						\
	else																					\
	{																						\
		skip[2] = (height <= BLITTER_YSKIP) ? 0 : height - BLITTER_YSKIP;					\
		if (BLITTER_YCOUNT > 1) height -= BLITTER_YCOUNT - 1;								\
	}																						\
																							\
	/* skip top */																			\
	for (y = 0; y < skip[2]; y++)															\
	{																						\
		/* skip dest */																		\
		addr += xdir * (width + skip[0] + skip[1]);											\
																							\
		/* scan RLE until done */															\
		for (xleft = width + skip[0] + skip[1]; xleft > 0; )								\
		{																					\
			/* load next RLE chunk if needed */												\
			if (!count)																		\
			{																				\
				count = *src++;																\
				val = (count & 0x80) ? -1 : *src++;											\
				count &= 0x7f;																\
			}																				\
																							\
			/* determine how much to bite off */											\
			innercount = (xleft > count) ? count : xleft;									\
			count -= innercount;															\
			xleft -= innercount;															\
																							\
			/* skip past the data */														\
			if (val == -1) src += innercount;												\
		}																					\
																							\
		/* back up one and reverse directions */											\
		addr -= xdir;																		\
		addr += ydir * 256;																	\
		addr &= 0x3ffff;																	\
		xdir = -xdir;																		\
	}																						\
																							\
	/* loop over height */																	\
	for (y = skip[2]; y < height; y++)														\
	{																						\
		/* skip left */																		\
		addr += xdir * skip[y & 1];															\
		for (xleft = skip[y & 1]; xleft > 0; )												\
		{																					\
			/* load next RLE chunk if needed */												\
			if (!count)																		\
			{																				\
				count = *src++;																\
				val = (count & 0x80) ? -1 : *src++;											\
				count &= 0x7f;																\
			}																				\
																							\
			/* determine how much to bite off */											\
			innercount = (xleft > count) ? count : xleft;									\
			count -= innercount;															\
			xleft -= innercount;															\
																							\
			/* skip past the data */														\
			if (val == -1) src += innercount;												\
		}																					\
																							\
		/* loop over width */																\
		for (xleft = width; xleft > 0; )													\
		{																					\
			/* load next RLE chunk if needed */												\
			if (!count)																		\
			{																				\
				count = *src++;																\
				val = (count & 0x80) ? -1 : *src++;											\
				count &= 0x7f;																\
			}																				\
																							\
			/* determine how much to bite off */											\
			innercount = (xleft > count) ? count : xleft;									\
			count -= innercount;															\
			xleft -= innercount;															\
																							\
			/* run of literals */															\
			if (val == -1)																	\
				for ( ; innercount--; addr += xdir)											\
					OPERATION(addr, *src++, mask, color);									\
																							\
			/* run of non-transparent repeats */											\
			else if (!TRANSPARENT || val)													\
				for ( ; innercount--; addr += xdir)											\
					OPERATION(addr, val, mask, color);										\
																							\
			/* run of transparent repeats */												\
			else																			\
				addr += xdir * innercount;													\
		}																					\
																							\
		/* skip right */																	\
		addr += xdir * skip[~y & 1];														\
		for (xleft = skip[~y & 1]; xleft > 0; )												\
		{																					\
			/* load next RLE chunk if needed */												\
			if (!count)																		\
			{																				\
				count = *src++;																\
				val = (count & 0x80) ? -1 : *src++;											\
				count &= 0x7f;																\
			}																				\
																							\
			/* determine how much to bite off */											\
			innercount = (xleft > count) ? count : xleft;									\
			count -= innercount;															\
			xleft -= innercount;															\
																							\
			/* skip past the data */														\
			if (val == -1) src += innercount;												\
		}																					\
																							\
		/* back up one and reverse directions */											\
		addr -= xdir;																		\
		addr += ydir * 256;																	\
		addr &= 0x3ffff;																	\
		xdir = -xdir;																		\
	}																						\
}



/*************************************
 *
 *	Blitter functions and tables
 *
 *************************************/

DRAW_RAW_MACRO(draw_raw,              0, draw_byte)
DRAW_RAW_MACRO(draw_raw_shift,        0, draw_byte_shift)
DRAW_RAW_MACRO(draw_raw_trans4,       1, draw_byte_trans4)
DRAW_RAW_MACRO(draw_raw_trans8,       1, draw_byte_trans8)
DRAW_RAW_MACRO(draw_raw_shift_trans4, 1, draw_byte_shift_trans4)
DRAW_RAW_MACRO(draw_raw_shift_trans8, 1, draw_byte_shift_trans8)

DRAW_RLE_MACRO(draw_rle,              0, draw_byte)
DRAW_RLE_MACRO(draw_rle_shift,        0, draw_byte_shift)
DRAW_RLE_MACRO(draw_rle_trans4,       1, draw_byte_trans4)
DRAW_RLE_MACRO(draw_rle_trans8,       1, draw_byte_trans8)
DRAW_RLE_MACRO(draw_rle_shift_trans4, 1, draw_byte_shift_trans4)
DRAW_RLE_MACRO(draw_rle_shift_trans8, 1, draw_byte_shift_trans8)

DRAW_RAW_MACRO(draw_raw_xflip,              0, draw_byte_xflip)
DRAW_RAW_MACRO(draw_raw_shift_xflip,        0, draw_byte_shift_xflip)
DRAW_RAW_MACRO(draw_raw_trans4_xflip,       1, draw_byte_trans4_xflip)
DRAW_RAW_MACRO(draw_raw_shift_trans4_xflip, 1, draw_byte_shift_trans4_xflip)

DRAW_RLE_MACRO(draw_rle_xflip,              0, draw_byte_xflip)
DRAW_RLE_MACRO(draw_rle_shift_xflip,        0, draw_byte_shift_xflip)
DRAW_RLE_MACRO(draw_rle_trans4_xflip,       1, draw_byte_trans4_xflip)
DRAW_RLE_MACRO(draw_rle_shift_trans4_xflip, 1, draw_byte_shift_trans4_xflip)


static void (*blit_table4[0x20])(void) =
{
	draw_raw,			draw_raw_shift,			draw_raw,			draw_raw_shift,
	draw_raw,			draw_raw_shift,			draw_raw,			draw_raw_shift,
	draw_rle,			draw_rle_shift,			draw_rle,			draw_rle_shift,
	draw_rle,			draw_rle_shift,			draw_rle,			draw_rle_shift,
	draw_raw_trans4,	draw_raw_shift_trans4,	draw_raw_trans4,	draw_raw_shift_trans4,
	draw_raw_trans4,	draw_raw_shift_trans4,	draw_raw_trans4,	draw_raw_shift_trans4,
	draw_rle_trans4,	draw_rle_shift_trans4,	draw_rle_trans4,	draw_rle_shift_trans4,
	draw_rle_trans4,	draw_rle_shift_trans4,	draw_rle_trans4,	draw_rle_shift_trans4
};

static void (*blit_table4_xflip[0x20])(void) =
{
	draw_raw_xflip,			draw_raw_shift_xflip,			draw_raw_xflip,			draw_raw_shift_xflip,
	draw_raw_xflip,			draw_raw_shift_xflip,			draw_raw_xflip,			draw_raw_shift_xflip,
	draw_rle_xflip,			draw_rle_shift_xflip,			draw_rle_xflip,			draw_rle_shift_xflip,
	draw_rle_xflip,			draw_rle_shift_xflip,			draw_rle_xflip,			draw_rle_shift_xflip,
	draw_raw_trans4_xflip,	draw_raw_shift_trans4_xflip,	draw_raw_trans4_xflip,	draw_raw_shift_trans4_xflip,
	draw_raw_trans4_xflip,	draw_raw_shift_trans4_xflip,	draw_raw_trans4_xflip,	draw_raw_shift_trans4_xflip,
	draw_rle_trans4_xflip,	draw_rle_shift_trans4_xflip,	draw_rle_trans4_xflip,	draw_rle_shift_trans4_xflip,
	draw_rle_trans4_xflip,	draw_rle_shift_trans4_xflip,	draw_rle_trans4_xflip,	draw_rle_shift_trans4_xflip
};

static void (*blit_table8[0x20])(void) =
{
	draw_raw,			draw_raw_shift,			draw_raw,			draw_raw_shift,
	draw_raw,			draw_raw_shift,			draw_raw,			draw_raw_shift,
	draw_rle,			draw_rle_shift,			draw_rle,			draw_rle_shift,
	draw_rle,			draw_rle_shift,			draw_rle,			draw_rle_shift,
	draw_raw_trans8,	draw_raw_shift_trans8,	draw_raw_trans8,	draw_raw_shift_trans8,
	draw_raw_trans8,	draw_raw_shift_trans8,	draw_raw_trans8,	draw_raw_shift_trans8,
	draw_rle_trans8,	draw_rle_shift_trans8,	draw_rle_trans8,	draw_rle_shift_trans8,
	draw_rle_trans8,	draw_rle_shift_trans8,	draw_rle_trans8,	draw_rle_shift_trans8
};



/*************************************
 *
 *	Blitter operations
 *
 *************************************/

static int perform_blit(void)
{
	/* debugging */
	if (FULL_LOGGING)
		logerror("Blit: scan=%d  src=%06x @ (%05x) for %dx%d ... flags=%02x\n",
				cpu_getscanline(),
				(*itech8_grom_bank << 16) | (BLITTER_ADDRHI << 8) | BLITTER_ADDRLO,
				0, BLITTER_WIDTH, BLITTER_HEIGHT, BLITTER_FLAGS);

	/* draw appropriately */
	if (BLITTER_OUTPUT & 0x40)
	{
		if (BLITTER_FLAGS & BLITFLAG_XFLIP)
			(*blit_table4_xflip[BLITTER_FLAGS & 0x1f])();
		else
			(*blit_table4[BLITTER_FLAGS & 0x1f])();
	}
	else
		(*blit_table8[BLITTER_FLAGS & 0x1f])();

	/* return the number of bytes processed */
	return BLITTER_WIDTH * BLITTER_HEIGHT;
}


static void blitter_done(int param)
{
	/* turn off blitting and generate an interrupt */
	blit_in_progress = 0;
	itech8_update_interrupts(-1, -1, 1);

	if (FULL_LOGGING) log_cb(RETRO_LOG_DEBUG, LOGPRE "------------ BLIT DONE (%d) --------------\n", cpu_getscanline());
}



/*************************************
 *
 *	Blitter I/O
 *
 *************************************/

READ_HANDLER( itech8_blitter_r )
{
	int result = blitter_data[offset / 2];

	/* debugging */
	if (FULL_LOGGING) log_cb(RETRO_LOG_DEBUG, LOGPRE "%04x:blitter_r(%02x)\n", activecpu_get_previouspc(), offset / 2);

	/* low bit seems to be ignored */
	offset /= 2;

	/* a read from offset 3 clears the interrupt and returns the status */
	if (offset == 3)
	{
		itech8_update_interrupts(-1, -1, 0);
		if (blit_in_progress)
			result |= 0x80;
		else
			result &= 0x7f;
	}

	return result;
}


WRITE_HANDLER( itech8_blitter_w )
{
	/* low bit seems to be ignored */
	offset /= 2;
	blitter_data[offset] = data;

	/* a write to offset 3 starts things going */
	if (offset == 3)
	{
		int pixels;

		/* log to the blitter file */
		if (BLIT_LOGGING)
		{
			static FILE *blitlog;
			if (!blitlog) blitlog = fopen("blitter.log", "w");
			if (blitlog) fprintf(blitlog, "Blit: XY=%1X%02X%02X SRC=%02X%02X%02X SIZE=%3dx%3d FLAGS=%02x",
						tms34061_r(14*4+2, 0, 0) & 0x0f, tms34061_r(15*4+2, 0, 0), tms34061_r(15*4+0, 0, 0),
						*itech8_grom_bank, blitter_data[0], blitter_data[1],
						blitter_data[4], blitter_data[5],
						blitter_data[2]);
			if (blitlog) fprintf(blitlog, "   %02X %02X %02X [%02X] %02X %02X %02X [%02X]-%02X %02X %02X %02X [%02X %02X %02X %02X]\n",
						blitter_data[0], blitter_data[1],
						blitter_data[2], blitter_data[3],
						blitter_data[4], blitter_data[5],
						blitter_data[6], blitter_data[7],
						blitter_data[8], blitter_data[9],
						blitter_data[10], blitter_data[11],
						blitter_data[12], blitter_data[13],
						blitter_data[14], blitter_data[15]);
		}

		/* perform the blit */
		pixels = perform_blit();
		blit_in_progress = 1;

		/* set a timer to go off when we're done */
		if (INSTANT_BLIT)
			blitter_done(0);
		else
			timer_set((double)pixels * TIME_IN_HZ(12000000), 0, blitter_done);
	}

	/* debugging */
	if (FULL_LOGGING) log_cb(RETRO_LOG_DEBUG, LOGPRE "%04x:blitter_w(%02x)=%02x\n", activecpu_get_previouspc(), offset, data);
}



/*************************************
 *
 *	TMS34061 I/O
 *
 *************************************/

WRITE_HANDLER( itech8_tms34061_w )
{
	int func = (offset >> 9) & 7;
	int col = offset & 0xff;

	/* Column address (CA0-CA8) is hooked up the A0-A7, with A1 being inverted
	   during register access. CA8 is ignored */
	if (func == 0 || func == 2)
		col ^= 2;

	/* Row address (RA0-RA8) is not dependent on the offset */
	tms34061_w(col, 0xff, func, data);
}


READ_HANDLER( itech8_tms34061_r )
{
	int func = (offset >> 9) & 7;
	int col = offset & 0xff;

	/* Column address (CA0-CA8) is hooked up the A0-A7, with A1 being inverted
	   during register access. CA8 is ignored */
	if (func == 0 || func == 2)
		col ^= 2;

	/* Row address (RA0-RA8) is not dependent on the offset */
	return tms34061_r(col, 0xff, func);
}



/*************************************
 *
 *	Main refresh
 *
 *************************************/

VIDEO_UPDATE( itech8 )
{
	int y, ty;

	/* first get the current display state */
	tms34061_get_display_state(&tms_state);

	/* if we're blanked, just fill with black */
	if (tms_state.blanked)
	{
		fillbitmap(bitmap, Machine->pens[0], cliprect);
		return;
	}

	/* perform one of two types of blitting; I'm not sure if bit 40 in */
	/* the blitter mode register really controls this type of behavior, but */
	/* it is set consistently enough that we can use it */

	/* blit mode one: 4bpp in the TMS34061 RAM, plus 4bpp of latched data */
	/* two pages are available, at 0x00000 and 0x20000 */
	/* pages are selected via the display page register */
	/* width can be up to 512 pixels */
	if (BLITTER_OUTPUT & 0x40)
	{
		int halfwidth = (Machine->visible_area.max_x + 2) / 2;
		UINT8 *base = &tms_state.vram[(~*itech8_display_page & 0x80) << 10];
		UINT8 *latch = &tms_state.latchram[(~*itech8_display_page & 0x80) << 10];

		base += (cliprect->min_y - Machine->visible_area.min_y) * 256;
		latch += (cliprect->min_y - Machine->visible_area.min_y) * 256;

		/* now regenerate the bitmap */
		for (ty = 0, y = cliprect->min_y; y <= cliprect->max_y; y++, ty++)
		{
			UINT8 scanline[512];
			int x;

			for (x = 0; x < halfwidth; x++)
			{
				scanline[x * 2 + 0] = (latch[256 * ty + x] & 0xf0) | (base[256 * ty + x] >> 4);
				scanline[x * 2 + 1] = (latch[256 * ty + x] << 4) | (base[256 * ty + x] & 0x0f);
			}
			draw_scanline8(bitmap, cliprect->min_x, y, cliprect->max_x - cliprect->min_x + 1, &scanline[cliprect->min_x], Machine->pens, -1);
		}
	}

	/* blit mode one: 8bpp in the TMS34061 RAM */
	/* two planes are available, at 0x00000 and 0x20000 */
	/* both planes are rendered; with 0x20000 transparent via color 0 */
	/* width can be up to 256 pixels */
	else
	{
		UINT8 *base = &tms_state.vram[tms_state.dispstart & ~0x30000];

		base += (cliprect->min_y - Machine->visible_area.min_y) * 256;

		/* now regenerate the bitmap */
		for (ty = 0, y = cliprect->min_y; y <= cliprect->max_y; y++, ty++)
		{
			draw_scanline8(bitmap, cliprect->min_x, y, cliprect->max_x - cliprect->min_x + 1, &base[0x20000 + 256 * ty + cliprect->min_x], Machine->pens, -1);
			draw_scanline8(bitmap, cliprect->min_x, y, cliprect->max_x - cliprect->min_x + 1, &base[0x00000 + 256 * ty + cliprect->min_x], Machine->pens, 0);
		}
	}

	/* extra rendering for slikshot */
	if (slikshot)
		slikshot_extra_draw(bitmap, cliprect);
}
