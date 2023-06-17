/******************************************************************************

	Nintendo 2C03B PPU emulation.

	Written by Ernesto Corvi.
	This code is heavily based on Brad Oliver's MESS implementation.

******************************************************************************/
#ifndef __PPU_2C03B_H__
#define __PPU_2C03B_H__

#include "driver.h"

/* increment to use more chips */
#define MAX_PPU 2

/* mirroring types */
#define PPU_MIRROR_NONE		0
#define PPU_MIRROR_VERT		1
#define PPU_MIRROR_HORZ		2
#define PPU_MIRROR_HIGH		3
#define PPU_MIRROR_LOW		4

/* registers definition */
enum {
	PPU_CONTROL0 = 0,
	PPU_CONTROL1,
	PPU_STATUS,
	PPU_SPRITE_ADDRESS,
	PPU_SPRITE_DATA,
	PPU_SCROLL,
	PPU_ADDRESS,
	PPU_DATA,
	PPU_MAX_REG
};

/* bit definitions for (some of) the registers */
enum {
	PPU_CONTROL0_INC			= 0x04,
	PPU_CONTROL0_SPR_SELECT		= 0x08,
	PPU_CONTROL0_CHR_SELECT		= 0x10,
	PPU_CONTROL0_SPRITE_SIZE	= 0x20,
	PPU_CONTROL0_NMI			= 0x80,

	PPU_CONTROL1_DISPLAY_MONO	= 0x01,
	PPU_CONTROL1_BACKGROUND_L8	= 0x02,
	PPU_CONTROL1_SPRITES_L8		= 0x04,
	PPU_CONTROL1_BACKGROUND		= 0x08,
	PPU_CONTROL1_SPRITES		= 0x10,

	PPU_STATUS_8SPRITES			= 0x20,
	PPU_STATUS_SPRITE0_HIT		= 0x40,
	PPU_STATUS_VBLANK			= 0x80
};

/* callback datatypes */
typedef void (*ppu2c03b_scanline_cb)( int num, int scanline, int vblank, int blanked );
typedef void (*ppu2c03b_nmi_cb)( int num, int *ppu_regs );
typedef int  (*ppu2c03b_vidaccess_cb)( int num, int address, int data );

struct ppu2c03b_interface
{
	int				num;							/* number of chips ( 1 to MAX_PPU ) */
	int				vrom_region[MAX_PPU];			/* region id of gfx vrom (or REGION_INVALID if none) */
	int				gfx_layout_number[MAX_PPU];		/* gfx layout number used by each chip */
	int				color_base[MAX_PPU];			/* color base to use per ppu */
	int				mirroring[MAX_PPU];				/* mirroring options (PPU_MIRROR_* flag) */
	ppu2c03b_nmi_cb	nmi_handler[MAX_PPU];			/* NMI handler */
};

/* routines */
void ppu2c03b_init_palette( int first_entry );
int ppu2c03b_init( struct ppu2c03b_interface *interface );

void ppu2c03b_reset( int num, int scan_scale );
void ppu2c03b_set_videorom_bank( int num, int start_page, int num_pages, int bank, int bank_size );
void ppu2c03b_spriteram_dma( int num, const UINT8 *source );
void ppu2c03b_render( int num, struct mame_bitmap *bitmap, int flipx, int flipy, int sx, int sy );
int ppu2c03b_get_pixel( int num, int x, int y );
int ppu2c03b_get_colorbase( int num );
void ppu2c03b_set_mirroring( int num, int mirroring );
void ppu2c03b_set_irq_callback( int num, ppu2c03b_nmi_cb cb );
void ppu2c03b_set_scanline_callback( int num, ppu2c03b_scanline_cb cb );
void ppu2c03b_set_vidaccess_callback( int num, ppu2c03b_vidaccess_cb cb );
void ppu2c03b_set_scanlines_per_frame( int num, int scanlines );

/*27/12/2002*/
extern void (*ppu_latch)( offs_t offset );

void ppu2c03b_w( int num, int offset, int data );
int ppu2c03b_r( int num, int offset );

/* accesors */
READ_HANDLER( ppu2c03b_0_r );
READ_HANDLER( ppu2c03b_1_r );

WRITE_HANDLER( ppu2c03b_0_w );
WRITE_HANDLER( ppu2c03b_1_w );

#endif /* __PPU_2C03B_H__ */
