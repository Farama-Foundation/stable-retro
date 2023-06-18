/******************************************************************************

	Nintendo 2C03B PPU emulation.

	Written by Ernesto Corvi.
	This code is heavily based on Brad Oliver's MESS implementation.

******************************************************************************/

#include <math.h>
#include "vidhrdw/ppu2c03b.h"

/* constant definitions */
#define BOTTOM_VISIBLE_SCANLINE	239		/* The bottommost visible scanline */
#define NMI_SCANLINE     		244		/* 244 times Bayou Billy perfectly */
#define VISIBLE_SCREEN_WIDTH	(32*8)	/* Visible screen width */
#define VISIBLE_SCREEN_HEIGHT	(30*8)	/* Visible screen height */
#define VIDEORAM_SIZE			0x4000	/* videoram size */
#define SPRITERAM_SIZE			0x100	/* spriteram size */
#define CHARGEN_NUM_CHARS		512		/* max number of characters handled by the chargen */

/* default monochromatic colortable */
pen_t default_colortable_mono[] =
{
	0,1,2,3,
	0,1,2,3,
	0,1,2,3,
	0,1,2,3,
	0,1,2,3,
	0,1,2,3,
	0,1,2,3,
	0,1,2,3,
};

/* our chip state */
typedef struct {
	struct mame_bitmap		*bitmap;				/* target bitmap */
	UINT8					*videoram;				/* video ram */
	UINT8					*spriteram;				/* sprite ram */
	pen_t					*colortable_mono;		/* monochromatic color table modified at run time */
	UINT8					*dirtychar;				/* an array flagging dirty characters */
	int						chars_are_dirty;		/* master flag to check if theres any dirty character */
	void 					*scanline_timer;		/* scanline timer */
	int						scanline;				/* scanline count */
	ppu2c03b_scanline_cb	scanline_callback_proc;	/* optinal scanline callback */
	ppu2c03b_vidaccess_cb	vidaccess_callback_proc;/* optinal video access callback */
	int						has_videorom;			/* wether we access a video rom or not */
	int						videorom_banks;			/* number of banks in the videorom (if available) */
	int						regs[PPU_MAX_REG];		/* registers */
	int						refresh_data;			/* refresh-related */
	int						refresh_latch;			/* refresh-related */
	int						x_fine;					/* refresh-related */
	int						toggle;					/* used to latch hi-lo scroll */
	int						add;					/* vram increment amount */
	int						videoram_addr;			/* videoram address pointer */
	int						videoram_addr_latch;	/* videoram address latch */
	int						videoram_data_latch;	/* latched videoram data */
	int						tile_page;				/* current tile page */
	int						sprite_page;			/* current sprite page */
	int						back_color;				/* background color */
	UINT8					*ppu_page[4];			/* ppu pages */
	int						nes_vram[8];			/* keep track of 8 .5k vram pages to speed things up */
	int						scan_scale;				/* scan scale */
	int						scanlines_per_frame;	/* number of scanlines per frame */
} ppu2c03b_chip;

/* our local copy of the interface */
static struct ppu2c03b_interface *intf;

/* chips state - allocated at init time */
static ppu2c03b_chip *chips = 0;

static void scanline_callback( int num );

void (*ppu_latch)( offs_t offset );


/*************************************
 *
 *	PPU Palette Initialization
 *
 *************************************/
void ppu2c03b_init_palette( int first_entry ) {

#ifndef M_PI
#define M_PI 			(3.14159265358979323846L)
#endif

	/* This routine builds a palette using a transformation from */
	/* the YUV (Y, B-Y, R-Y) to the RGB color space */

	/* The NES has a 64 color palette                        */
	/* 16 colors, with 4 luminance levels for each color     */
	/* The 16 colors circle around the YUV color space,      */

	int i,j;

	double R, G, B;

	double tint = .6;
	double hue = 332.0;
	double bright_adjust = 1.0;

	double brightness[3][4] =
	{
		{ 0.50, 0.75, 1.0, 1.0 },
		{ 0.29, 0.45, 0.73, 0.9 },
		{ 0, 0.24, 0.47, 0.77 }
	};

	double angle[16] = { 0, 240, 210, 180, 150, 120, 90, 60, 30, 0, 330, 300, 270, 0, 0, 0 };

	/* loop through the 4 intensities */
	for (i = 0; i < 4; i++)
	{
		/* loop through the 16 colors */
		for (j = 0; j < 16; j++)
		{
			double sat;
			double y;
			double rad;

			switch (j)
			{
				case 0:
					sat = 0;
					y = brightness[0][i];
				break;

				case 13:
					sat = 0;
					y = brightness[2][i];
				break;

				case 14:
				case 15:
					sat = 0; y = 0;
				break;

				default:
					sat = tint;
					y = brightness[1][i];
				break;
			}

			rad = M_PI * ((angle[j] + hue) / 180.0);

			y *= bright_adjust;

			/* Transform to RGB */
			R = ( y + sat * sin( rad ) ) * 255.0;
			G = ( y - ( 27 / 53 ) * sat * sin( rad ) + ( 10 / 53 ) * sat * cos( rad ) ) * 255.0;
			B = ( y - sat * cos( rad ) ) * 255.0;

			/* Clipping, in case of saturation */
			if ( R < 0 )
				R = 0;
			if ( R > 255 )
				R = 255;
			if ( G < 0 )
				G = 0;
			if ( G > 255 )
				G = 255;
			if ( B < 0 )
				B = 0;
			if ( B > 255 )
				B = 255;

			/* Round, and set the value */
			palette_set_color(first_entry++, floor(R+.5), floor(G+.5), floor(B+.5));
		}
	}

	/* color tables are modified at run-time, and are initialized on 'ppu2c03b_reset' */
}

/* the charlayout we use for the chargen */
static struct GfxLayout ppu_charlayout =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters - modified at runtime */
	2,		/* 2 bits per pixel */
	{ 8*8, 0 },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8	/* every char takes 16 consecutive bytes */
};

/*************************************
 *
 *	PPU Initialization and Disposal
 *
 *************************************/
int ppu2c03b_init( struct ppu2c03b_interface *interface )
{
	int i;

	/* keep a local copy of the interface */
	intf = interface;

	/* safety check */
	if ( intf->num <= 0 )
		return -1;

	chips = auto_malloc( intf->num * sizeof( ppu2c03b_chip ) );

	if ( chips == 0 )
		return -1;

	/* intialize our virtual chips */
	for( i = 0; i < intf->num; i++ )
	{
		/* initialize the scanline handling portion */
		chips[i].scanline_timer = timer_alloc(scanline_callback);
		chips[i].scanline = 0;
		chips[i].scan_scale = 1;

		/* allocate a screen bitmap, videoram and spriteram, a dirtychar array and the monochromatic colortable */
		chips[i].bitmap = auto_bitmap_alloc( VISIBLE_SCREEN_WIDTH, VISIBLE_SCREEN_HEIGHT );
		chips[i].videoram = auto_malloc( VIDEORAM_SIZE );
		chips[i].spriteram = auto_malloc( SPRITERAM_SIZE );
		chips[i].dirtychar = auto_malloc( CHARGEN_NUM_CHARS );
		chips[i].colortable_mono = auto_malloc( sizeof( default_colortable_mono ) );

		/* see if it failed */
		if ( !chips[i].bitmap || !chips[i].videoram || !chips[i].spriteram || !chips[i].dirtychar || !chips[i].colortable_mono )
			return -1;

		/* clear videoram & spriteram */
		memset( chips[i].videoram, 0, VIDEORAM_SIZE );
		memset( chips[i].spriteram, 0, SPRITERAM_SIZE );

		/* set all characters dirty */
		memset( chips[i].dirtychar, 1, CHARGEN_NUM_CHARS );

		/* initialize the video ROM portion, if available */
		if ( ( intf->vrom_region[i] != REGION_INVALID ) && ( memory_region( intf->vrom_region[i] ) != 0 ) )
		{
			/* mark that we have a videorom */
			chips[i].has_videorom = 1;

			/* find out how many banks */
			chips[i].videorom_banks = memory_region_length( intf->vrom_region[i] ) / 0x2000;

			/* tweak the layout accordingly */
			ppu_charlayout.total = chips[i].videorom_banks * CHARGEN_NUM_CHARS;
		}
		else
		{
			chips[i].has_videorom = chips[i].videorom_banks = 0;

			/* we need to reset this in case of mame running multisession */
			ppu_charlayout.total = CHARGEN_NUM_CHARS;
		}

		/* now create the gfx region */
		{
			UINT8 *src = chips[i].has_videorom ? memory_region( intf->vrom_region[i] ) : chips[i].videoram;
			Machine->gfx[intf->gfx_layout_number[i]] = decodegfx( src, &ppu_charlayout );

			if ( Machine->gfx[intf->gfx_layout_number[i]] == 0 )
				return -1;

			if ( Machine->remapped_colortable )
				Machine->gfx[intf->gfx_layout_number[i]]->colortable = &Machine->remapped_colortable[intf->color_base[i]];

			Machine->gfx[intf->gfx_layout_number[i]]->total_colors = 8;
		}

		/* setup our videoram handlers based on mirroring */
		ppu2c03b_set_mirroring( i, intf->mirroring[i] );
	}

	/* success */
	return 0;
}

static void draw_background( const int num, UINT8 *line_priority )
{
	/* cache some values locally */
	struct mame_bitmap *bitmap = chips[num].bitmap;
	const int *ppu_regs = &chips[num].regs[0];
	const int scanline = chips[num].scanline;
	const int refresh_data = chips[num].refresh_data;
	const int gfx_bank = intf->gfx_layout_number[num];
	const int total_elements = Machine->gfx[gfx_bank]->total_elements;
	const int *nes_vram = &chips[num].nes_vram[0];
	const int tile_page = chips[num].tile_page;
	const int char_modulo = Machine->gfx[gfx_bank]->char_modulo;
	const int line_modulo = Machine->gfx[gfx_bank]->line_modulo;
	UINT8 *gfx_data = Machine->gfx[gfx_bank]->gfxdata;
	UINT8 **ppu_page = chips[num].ppu_page;
	int	start_x = ( chips[num].x_fine ^ 0x07 ) - 7;
	UINT16 back_pen;
	UINT16 *dest;

	UINT8 scroll_x_coarse, scroll_y_coarse, scroll_y_fine, color_mask;
	int x, tile_index, start, i;

	const pen_t *color_table;
	const pen_t *paldata;
	const UINT8 *sd;

	/* setup the color mask and colortable to use */
	if ( ppu_regs[PPU_CONTROL1] & PPU_CONTROL1_DISPLAY_MONO )
	{
		color_mask = 0xf0;
		color_table = chips[num].colortable_mono;
	}
	else
	{
		color_mask = 0xff;
		color_table = Machine->gfx[gfx_bank]->colortable;
	}

	/* cache the background pen */
	back_pen = Machine->pens[(chips[num].back_color & color_mask)+intf->color_base[num]];

	/* determine where in the nametable to start drawing from */
	/* based on the current scanline and scroll regs */
	scroll_x_coarse = refresh_data & 0x1f;
	scroll_y_coarse = ( refresh_data & 0x3e0 ) >> 5;
	scroll_y_fine = ( refresh_data & 0x7000 ) >> 12;

	x = scroll_x_coarse;

	/* get the tile index */
	tile_index = ( ( refresh_data & 0xc00 ) | 0x2000 ) + scroll_y_coarse * 32;

	/* set up dest */
	dest = ((UINT16 *) bitmap->base) + (bitmap->rowpixels * scanline) + start_x;

	/* draw the 32 or 33 tiles that make up a line */
	while ( start_x < VISIBLE_SCREEN_WIDTH )
	{
		int color_byte;
		int color_bits;
		int pos;
		int index1;
		int page, page2, address;
		int index2;
		UINT16 pen;

		index1 = tile_index + x;

		/* Figure out which byte in the color table to use */
		pos = ( ( index1 & 0x380 ) >> 4 ) | ( ( index1 & 0x1f ) >> 2 );
		page = (index1 & 0x0c00) >> 10;
		address = 0x3c0 + pos;
		color_byte = ppu_page[page][address];

		/* figure out which bits in the color table to use */
		color_bits = ( ( index1 & 0x40 ) >> 4 ) + ( index1 & 0x02 );

		address = index1 & 0x3ff;
		page2 = ppu_page[page][address];
		index2 = nes_vram[ ( page2 >> 6 ) | tile_page ] + ( page2 & 0x3f );

		/*27/12/2002*/
		if( ppu_latch )
		{
			(*ppu_latch)(( tile_page << 10 ) | ( page2 << 4 ));
		}

		paldata = &color_table[ 4 * ( ( ( color_byte >> color_bits ) & 0x03 ) ) ];
		start = ( index2 % total_elements ) * char_modulo + scroll_y_fine * line_modulo;
		sd = &gfx_data[start];

		/* render the pixel */
		for( i = 0; i < 8; i++ )
		{
			if ( ( start_x+i ) >= 0 && ( start_x+i ) < VISIBLE_SCREEN_WIDTH )
			{
				if ( sd[i] )
				{
					pen = paldata[sd[i]];
					line_priority[ start_x+i ] |= 0x02;
				}
				else
				{
					pen = back_pen;
				}
				*dest = pen;
			}
			dest++;
		}

		start_x += 8;

		/* move to next tile over and toggle the horizontal name table if necessary */
		x++;
		if ( x > 31 )
		{
			x = 0;
			tile_index ^= 0x400;
		}
	}

	/* if the left 8 pixels for the background are off, blank 'em */
	if ( !( ppu_regs[PPU_CONTROL1] & PPU_CONTROL1_BACKGROUND_L8 ) )
	{
		dest = ((UINT16 *) bitmap->base) + (bitmap->rowpixels * scanline);
		for( i = 0; i < 8; i++ )
			*(dest++) = back_pen;
	}

	/* done updating, whew */
}

static void draw_sprites( const int num, UINT8 *line_priority )
{
	/* cache some values locally */
	struct mame_bitmap *bitmap = chips[num].bitmap;
	const int scanline = chips[num].scanline;
	const int gfx_bank = intf->gfx_layout_number[num];
	const int total_elements = Machine->gfx[gfx_bank]->total_elements;
	const int sprite_page = chips[num].sprite_page;
	const int char_modulo = Machine->gfx[gfx_bank]->char_modulo;
	const int line_modulo = Machine->gfx[gfx_bank]->line_modulo;
	const UINT8 *sprites = chips[num].spriteram;
	pen_t *color_table = Machine->gfx[gfx_bank]->colortable;
	UINT8 *gfx_data = Machine->gfx[gfx_bank]->gfxdata;
	int *ppu_regs = &chips[num].regs[0];

	int x,y, i;
	int tile, index1, page;
	int pri;

	int flipx, flipy, color;
	int size;
	int spriteCount = 0;
	int sprite_line;
	int drawn;
	int start;

	const pen_t *paldata;
	const UINT8 *sd;

	/* determine if the sprites are 8x8 or 8x16 */
	size = ( ppu_regs[PPU_CONTROL0] & PPU_CONTROL0_SPRITE_SIZE ) ? 16 : 8;

	for( i = 0; i < SPRITERAM_SIZE; i += 4 ) {
		y = sprites[i] + 1;

		/* if the sprite isn't visible, skip it */
		if ( ( y + size <= scanline ) || ( y > scanline ) )
			continue;

		/* clear our drawn flag */
		drawn = 0;

		x = sprites[i+3];
		tile = sprites[i+1];
		color = ( sprites[i+2] & 0x03 ) + 4;
		pri = sprites[i+2] & 0x20;
		flipx = sprites[i+2] & 0x40;
		flipy = sprites[i+2] & 0x80;

		if ( size == 16 ) {
			/* if it's 8x16 and odd-numbered, draw the other half instead */
			if ( tile & 0x01 )
			{
				tile &= ~0x01;
				tile |= 0x100;
			}
			/* note that the sprite page value has no effect on 8x16 sprites */
			page = tile >> 6;
		} else
			page = ( tile >> 6 ) | sprite_page;


		index1 = chips[num].nes_vram[page] + ( tile & 0x3f );

		/*27/12/2002*/
		if ( ppu_latch )
			(*ppu_latch)(( sprite_page << 10 ) | ( (tile & 0xff) << 4 ));

		/* compute the character's line to draw */
		sprite_line = scanline - y;

		if ( flipy )
			sprite_line = ( size - 1 ) - sprite_line;

		paldata = &color_table[4 * color];
		start = ( index1 % total_elements ) * char_modulo + sprite_line * line_modulo;
		sd = &gfx_data[start];

		if ( pri )
		{
			/* draw the low-priority sprites */
			int j;

			if ( flipx )
			{
				for ( j = 0; j < 8; j++ )
				{
					/* is this pixel non-transparent? */
					if ( sd[7-j] )
					{
						/* has the background (or another sprite) already been drawn here? */
						if ( !line_priority[ x + j ] )
						{
							/* no, draw */
							if ( ( x + j ) < VISIBLE_SCREEN_WIDTH )
								plot_pixel( bitmap, x + j, scanline, paldata[ sd[7-j] ] );
							drawn = 1;
						}
						/* indicate that a sprite was drawn at this location, even if it's not seen */
						if ( ( x + j ) < VISIBLE_SCREEN_WIDTH )
							line_priority[ x + j ] |= 0x01;

						/* set the "sprite 0 hit" flag if appropriate */
						if ( i == 0 )
							ppu_regs[PPU_STATUS] |= PPU_STATUS_SPRITE0_HIT;
					}
				}
			}
			else
			{
				for ( j = 0; j < 8; j++ )
				{
					/* is this pixel non-transparent? */
					if ( sd[j] )
					{
						/* has the background (or another sprite) already been drawn here? */
						if ( !line_priority[ x + j ] )
						{
							/* no, draw */
							if ( ( x + j ) < VISIBLE_SCREEN_WIDTH )
								plot_pixel( bitmap, x + j, scanline, paldata[ sd[j] ] );
							drawn = 1;
						}
						/* indicate that a sprite was drawn at this location, even if it's not seen */
						if ( ( x + j ) < VISIBLE_SCREEN_WIDTH )
							line_priority[ x + j ] |= 0x01;

						/* set the "sprite 0 hit" flag if appropriate */
						if ( i == 0 )
							ppu_regs[PPU_STATUS] |= PPU_STATUS_SPRITE0_HIT;
					}
				}
			}
		}
		else
		{
			/* draw the high-priority sprites */
			int j;

			if ( flipx )
			{
				for ( j = 0; j < 8; j++ )
				{
					/* is this pixel non-transparent? */
					if ( sd[7-j] )
					{
						/* has another sprite been drawn here? */
						if ( !( line_priority[ x + j ] & 0x01 ) )
						{
							/* no, draw */
							if ( ( x + j ) < VISIBLE_SCREEN_WIDTH )
							{
								plot_pixel( bitmap, x + j, scanline, paldata[ sd[7-j] ] );
								line_priority[ x + j ] |= 0x01;
							}
							drawn = 1;
						}

						/* set the "sprite 0 hit" flag if appropriate */
						if ( ( i == 0 ) && ( line_priority[ x + j ] & 0x02 ) )
							ppu_regs[PPU_STATUS] |= PPU_STATUS_SPRITE0_HIT;
					}
				}
			}
			else
			{
				for ( j = 0; j < 8; j++ )
				{
					/* is this pixel non-transparent? */
					if ( sd[j] )
					{
						/* has another sprite been drawn here? */
						if ( !( line_priority[ x + j ] & 0x01 ) )
						{
							/* no, draw */
							if ( ( x + j ) < VISIBLE_SCREEN_WIDTH )
							{
								plot_pixel( bitmap, x + j, scanline, paldata[ sd[j] ] );
								line_priority[ x + j ] |= 0x01;
							}
							drawn = 1;
						}

						/* set the "sprite 0 hit" flag if appropriate */
						if ( ( i == 0 ) && ( line_priority[ x + j ] & 0x02 ) )
							ppu_regs[PPU_STATUS] |= PPU_STATUS_SPRITE0_HIT;
					}
				}
			}
		}

		if ( drawn )
		{
			/* if there are more than 8 sprites on this line, set the flag */
			spriteCount++;
			if ( spriteCount == 8 )
			{
				ppu_regs[PPU_STATUS] |= PPU_STATUS_8SPRITES;
				logerror ("> 8 sprites, scanline: %d\n", scanline);

				/* the real NES only draws up to 8 sprites - the rest should be invisible */
				break;
			}
		}
	}
}

/*************************************
 *
 *	Scanline Rendering and Update
 *
 *************************************/
static void render_scanline( int num )
{
	UINT8	line_priority[VISIBLE_SCREEN_WIDTH];
	int		*ppu_regs = &chips[num].regs[0];
	int 	i;
	int		refresh_data = chips[num].refresh_data;

	/* lets see how long it takes */
	profiler_mark(PROFILER_USER1+num);

	/* clear the line priority for this scanline */
	memset( line_priority, 0, VISIBLE_SCREEN_WIDTH );

	/* clear the sprite count for this line */
	ppu_regs[PPU_STATUS] &= ~PPU_STATUS_8SPRITES;

	/* see if we need to render the background */
	if ( ppu_regs[PPU_CONTROL1] & PPU_CONTROL1_BACKGROUND )
		draw_background( num, line_priority );

	/* if sprites are hidden in the leftmost column, fake a priority flag to mask them */
	if ( !( ppu_regs[PPU_CONTROL1] & PPU_CONTROL1_SPRITES_L8 ) )
	{
		for ( i = 0; i < 8; i++ )
			line_priority[i] |= 0x01;
	}

	/* if sprites are on, draw them */
	if ( ppu_regs[PPU_CONTROL1] & PPU_CONTROL1_SPRITES )
		draw_sprites( num, line_priority );

	/* increment the fine y-scroll */
	refresh_data += 0x1000;

	/* if it's rolled, increment the coarse y-scroll */
	if ( refresh_data & 0x8000 )
	{
		UINT16 tmp;
		tmp = ( refresh_data & 0x03e0 ) + 0x20;
		refresh_data &= 0x7c1f;
		/* handle bizarro scrolling rollover at the 30th (not 32nd) vertical tile */
		if ( tmp == 0x03c0 )
		{
			refresh_data ^= 0x0800;
		}
		else
		{
			refresh_data |= ( tmp & 0x03e0 );
		}
    }

    chips[num].refresh_data = refresh_data;

 	/* done updating, whew */
	profiler_mark(PROFILER_END);
}

static void update_scanline( int num )
{
	int scanline = chips[num].scanline;
	int *ppu_regs = &chips[num].regs[0];

	if ( scanline <= BOTTOM_VISIBLE_SCANLINE ) {
		/* If background or sprites are enabled, copy the ppu address latch */
		if ( ppu_regs[PPU_CONTROL1] & ( PPU_CONTROL1_BACKGROUND | PPU_CONTROL1_SPRITES ) )
		{
			/* Copy only the scroll x-coarse and the x-overflow bit */
			chips[num].refresh_data &= ~0x041f;
			chips[num].refresh_data |= ( chips[num].refresh_latch & 0x041f );
		}

#if 0	/* hmm... this only goes if we implement the osd_skip_this_frame functionality */

		/* check for sprite 0 hit */
		if ( ( scanline == spriteram[0] + 7 ) && ( ppu_regs[PPU_CONTROL1] & PPU_CONTROL1_SPRITES ) )
			ppu_regs[PPU_STATUS] |= PPU_STATUS_SPRITE0_HIT;

#endif

		/* Render this scanline if appropriate */
		if ( ppu_regs[PPU_CONTROL1] & ( PPU_CONTROL1_BACKGROUND | PPU_CONTROL1_SPRITES ) )
			render_scanline( num );

		return;
	}

	if ( scanline == BOTTOM_VISIBLE_SCANLINE+1 )
	{
		/* We just entered VBLANK */
		ppu_regs[PPU_STATUS] |= PPU_STATUS_VBLANK;
		return;
	}

	/* If NMI's are set to be triggered, go for it */
	if ( ( scanline == NMI_SCANLINE ) && ( ppu_regs[PPU_CONTROL0] & PPU_CONTROL0_NMI ) )
	{
		if ( intf->nmi_handler[num] )
			(*intf->nmi_handler[num])( num, ppu_regs );
	}
}

static void scanline_callback( int num )
{
	int *ppu_regs = &chips[num].regs[0];
	int i;
	int blanked = ( ppu_regs[PPU_CONTROL1] & ( PPU_CONTROL1_BACKGROUND | PPU_CONTROL1_SPRITES ) ) == 0;
	int vblank = ( ppu_regs[PPU_STATUS] & PPU_STATUS_VBLANK ) ? 1 : 0;

/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "SCANLINE CALLBACK %d\n",chips[num].scanline); */

	/* if a callback is available, call it */
	if ( chips[num].scanline_callback_proc )
		(*chips[num].scanline_callback_proc)( num, chips[num].scanline, vblank, blanked );

	/* update the scanline that just went by */
	update_scanline( num );

	/* increment our scanline count */
	chips[num].scanline++;

	/* decode any dirty chars if we're using vram */

	/* first, check the master dirty char flag */
	if ( !chips[num].has_videorom && chips[num].chars_are_dirty )
	{
		/* cache some values */
		UINT8 *dirtyarray = chips[num].dirtychar;
		UINT8 *vram = chips[num].videoram;
		struct GfxElement *gfx = Machine->gfx[intf->gfx_layout_number[num]];

		/* then iterate and decode */
		for( i = 0; i < CHARGEN_NUM_CHARS; i++ )
		{
			if ( dirtyarray[i] )
			{
				decodechar( gfx, i, vram, &ppu_charlayout);
				dirtyarray[i] = 0;
			}
		}

		chips[num].chars_are_dirty = 0;
	}

	/* see if we rolled */
	if ( chips[num].scanline >= chips[num].scanlines_per_frame )
	{
		/* clear the vblank & sprite hit flag */
		ppu_regs[PPU_STATUS] &= ~( PPU_STATUS_VBLANK | PPU_STATUS_SPRITE0_HIT );

		/* if background or sprites are enabled, copy the ppu address latch */
		if ( !blanked )
			chips[num].refresh_data = chips[num].refresh_latch;

		/* reset the scanline count */
		chips[num].scanline = 0;
	}

	/* setup our next stop here */
	timer_adjust(chips[num].scanline_timer, cpu_getscanlinetime( chips[num].scanline * chips[num].scan_scale ), num, 0);
}

/*************************************
 *
 *	PPU Reset
 *
 *************************************/
void ppu2c03b_reset( int num, int scan_scale )
{
	int i;

	/* check bounds */
	if ( num >= intf->num )
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE  "PPU(reset): Attempting to access an unmapped chip\n" );
		return;
	}

	/* reset the scanline count */
	chips[num].scanline = 0;

	/* set the scan scale (this is for dual monitor vertical setups) */
	chips[num].scan_scale = scan_scale;

	/* allocate the scanline timer - start at scanline 0 */
	timer_adjust(chips[num].scanline_timer, cpu_getscanlinetime(0), num, 0);

	/* reset the callbacks */
	chips[num].scanline_callback_proc = 0;
	chips[num].vidaccess_callback_proc = 0;

	for( i = 0; i < PPU_MAX_REG; i++ )
		chips[num].regs[i] = 0;

	/* initialize the rest of the members */
	chips[num].refresh_data = 0;
	chips[num].refresh_latch = 0;
	chips[num].x_fine = 0;
	chips[num].toggle = 0;
	chips[num].add = 1;
	chips[num].videoram_addr = 0;
	chips[num].videoram_addr_latch = 0;
	chips[num].videoram_data_latch = 0;
	chips[num].tile_page = 0;
	chips[num].sprite_page = 0;
	chips[num].back_color = 0;
	chips[num].chars_are_dirty = 1;
	chips[num].scanlines_per_frame = 262;	/* default */

	/* initialize the color tables */
	{
		int color_base = intf->color_base[num];

		for( i = 0; i < ( sizeof( default_colortable_mono ) / sizeof( default_colortable_mono[0] ) ); i++ )
		{
			/* monochromatic table */
			chips[num].colortable_mono[i] = Machine->pens[default_colortable_mono[i] + color_base];

			/* color table */
			Machine->gfx[intf->gfx_layout_number[num]]->colortable[i] = Machine->pens[default_colortable_mono[i] + color_base];
		}
	}

	/* set the vram bank-switch values to the default */
	for( i = 0; i < 8; i++ )
		chips[num].nes_vram[i] = i * 64;

	if ( chips[num].has_videorom )
		ppu2c03b_set_videorom_bank( num, 0, 8, 0, 512 );
}


/*************************************
 *
 *	PPU Registers Read
 *
 *************************************/
int ppu2c03b_r( int num, int offset )
{
	int ret = 0;

	/* check bounds */
	if ( num >= intf->num )
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE  "PPU %d(r): Attempting to access an unmapped chip\n", num );
		return 0;
	}

	if ( offset >= PPU_MAX_REG )
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE  "PPU %d(r): Attempting to read past the chip\n", num );

		offset &= PPU_MAX_REG - 1;
/*		return 0; */
	}

	/* now, see wich register to read */
	switch( offset )
	{
		case PPU_STATUS:
			ret = chips[num].regs[PPU_STATUS];

			/* this is necessary */
			chips[num].toggle = 0;

			/* note that we don't clear the vblank flag - this is correct. */
		break;

		case PPU_SPRITE_DATA:
			ret = chips[num].spriteram[chips[num].regs[PPU_SPRITE_ADDRESS]];
		break;

		case PPU_DATA:
			ret = chips[num].videoram_data_latch;

			/*27/12/2002*/
			if ( ppu_latch )
				(*ppu_latch)( chips[num].videoram_addr & 0x3fff );

			if ( ( chips[num].videoram_addr >= 0x2000 ) && ( chips[num].videoram_addr <= 0x3fef ) )
				chips[num].videoram_data_latch = chips[num].ppu_page[ ( chips[num].videoram_addr & 0xc00) >> 10][ chips[num].videoram_addr & 0x3ff ];
			else
				chips[num].videoram_data_latch = chips[num].videoram[ chips[num].videoram_addr & 0x3fff ];

			chips[num].videoram_addr += chips[num].add;
		break;

		default:
			/* ignore other register reads */
		break;
	}

	return ret;
}


/*************************************
 *
 *	PPU Registers Write
 *
 *************************************/
void ppu2c03b_w( int num, int offset, int data )
{

	/* check bounds */
	if ( num >= intf->num )
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE  "PPU(w): Attempting to access an unmapped chip\n" );
		return;
	}

	if ( offset >= PPU_MAX_REG )
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE  "PPU: Attempting to write past the chip\n" );

		offset &= PPU_MAX_REG - 1;
/*		return; */
	}

	switch( offset )
	{
		case PPU_CONTROL0:
			chips[num].regs[PPU_CONTROL0] = data;

			/* update the name table number on our refresh latches */
			chips[num].refresh_latch &= ~0x0c00;
			chips[num].refresh_latch |= ( data & 3 ) << 10;

			/* the char ram bank points either 0x0000 or 0x1000 (page 0 or page 4) */
			chips[num].tile_page = ( data & PPU_CONTROL0_CHR_SELECT ) >> 2;
			chips[num].sprite_page = ( data & PPU_CONTROL0_SPR_SELECT ) >> 1;

			chips[num].add = ( data & PPU_CONTROL0_INC ) ? 32 : 1;
		break;

		case PPU_CONTROL1:
			/* if color intensity has changed, change all the pens */
			if ( ( data & 0xe0 ) != ( chips[num].regs[PPU_CONTROL1] & 0xe0 ) )
			{
				/* TODO? : maybe add support for COLOR_INTENSITY later? */
			}

			chips[num].regs[PPU_CONTROL1] = data;
		break;

		case PPU_SPRITE_ADDRESS:
			chips[num].regs[PPU_SPRITE_ADDRESS] = data & 0xff;
		break;

		case PPU_SPRITE_DATA:
			chips[num].spriteram[chips[num].regs[PPU_SPRITE_ADDRESS]] = data;
			chips[num].regs[PPU_SPRITE_ADDRESS] = ( chips[num].regs[PPU_SPRITE_ADDRESS] + 1 ) & 0xff;
		break;

		case PPU_SCROLL:
			if ( chips[num].toggle )
			{
				/* second write */
				chips[num].refresh_latch &= ~0x03e0;
				chips[num].refresh_latch |= ( data & 0xf8 ) << 2;

				chips[num].refresh_latch &= ~0x7000;
				chips[num].refresh_latch |= ( data & 0x07 ) << 12;
			}
			else
			{
				/* first write */
				chips[num].refresh_latch &= ~0x1f;
				chips[num].refresh_latch |= (data & 0xf8) >> 3;

				chips[num].x_fine = data & 7;
			}

			chips[num].toggle ^= 1;
		break;

		case PPU_ADDRESS:
			if ( chips[num].toggle )
			{
				/* second write */
				chips[num].videoram_addr = ( chips[num].videoram_addr_latch << 8 ) | ( data & 0xff );

				chips[num].refresh_latch &= ~0x00ff;
				chips[num].refresh_latch |= data & 0xff;
				chips[num].refresh_data = chips[num].refresh_latch;
			}
			else
			{
				/* first write */
				chips[num].videoram_addr_latch = data & 0xff;

				if ( data != 0x3f ) /* TODO: remove this hack! */
				{
					chips[num].refresh_latch &= ~0xff00;
					chips[num].refresh_latch |= ( data & 0x3f ) << 8;
				}
			}

			chips[num].toggle ^= 1;
		break;

		case PPU_DATA:
			{
				int tempAddr = chips[num].videoram_addr & 0x3fff;

				/*27/12/2002*/
				if ( ppu_latch )
					(*ppu_latch)( tempAddr );

				/* if there's a callback, call it now */
				if ( chips[num].vidaccess_callback_proc )
					data = (*chips[num].vidaccess_callback_proc)( num, tempAddr, data );

				/* see if it's on the chargen portion */
				if ( tempAddr < 0x2000 )
				{
					/* if we have a videorom mapped there, dont write and log the problem */
					if ( chips[num].has_videorom )
					{
						/* if there is a vidaccess callback, assume it coped up with it */
						if ( chips[num].vidaccess_callback_proc == 0 )
							log_cb(RETRO_LOG_DEBUG, LOGPRE  "PPU: Attempting to write to the chargen, when there's a ROM there!\n" );
					}
					else
					{
						/* store the data */
						chips[num].videoram[tempAddr] = data;

						/* setup the master dirty switch */
						chips[num].chars_are_dirty = 1;

						/* mark the char dirty */
						chips[num].dirtychar[tempAddr >> 4] = 1;
					}

					/* increment the address */
					chips[num].videoram_addr += chips[num].add;

					/* and be gone */
					break;
				}

				/* the only valid background colors are writes to 0x3f00 and 0x3f10			*/
				/* and even then, they are mirrors of each other. as usual, some games		*/
				/* attempt to write values > the number of colors so we must mask the data. */
				if ( tempAddr >= 0x3f00 )
				{
					int color_base = intf->color_base[num];

					/* store the data */
					chips[num].videoram[tempAddr] = data;

					data &= 0x3f;

					if ( tempAddr & 0x03 )
					{
						Machine->gfx[intf->gfx_layout_number[num]]->colortable[ tempAddr & 0x1f ] = Machine->pens[color_base+data];
						chips[num].colortable_mono[tempAddr & 0x1f] = Machine->pens[color_base+(data & 0xf0)];
					}

					if ( ( tempAddr & 0x0f ) == 0 )
					{
						int i;

						chips[num].back_color = data;
						for( i = 0; i < 32; i += 4 )
						{
							Machine->gfx[intf->gfx_layout_number[num]]->colortable[ i ] = Machine->pens[color_base+data];
							chips[num].colortable_mono[i] = Machine->pens[color_base+(data & 0xf0)];
						}
					}

					/* increment the address */
					chips[num].videoram_addr += chips[num].add;

					/* and be gone */
					break;
				}

				/* everything else */
				/* writes to $3000-$3eff are mirrors of $2000-$2eff */
				{
					int page = ( tempAddr & 0x0c00) >> 10;
					int address = tempAddr & 0x3ff;

					chips[num].ppu_page[page][address] = data;

					/* increment the address */
					chips[num].videoram_addr += chips[num].add;

					/* fall through */
				}
			}
		break;

		default:
			/* ignore other registers writes */
		break;
	}
}

/*************************************
 *
 *	Sprite DMA
 *
 *************************************/
void ppu2c03b_spriteram_dma( int num, const UINT8 *source )
{
	/* check bounds */
	if ( num >= intf->num )
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE  "PPU(render): Attempting to access an unmapped chip\n" );
		return;
	}

	memcpy( chips[num].spriteram, source, SPRITERAM_SIZE );
}

/*************************************
 *
 *	PPU Rendering
 *
 *************************************/
void ppu2c03b_render( int num, struct mame_bitmap *bitmap, int flipx, int flipy, int sx, int sy )
{
	/* check bounds */
	if ( num >= intf->num )
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE  "PPU(render): Attempting to access an unmapped chip\n" );
		return;
	}

/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "PPU %d:VBLANK HIT (scanline %d)\n",num, chips[num].scanline); */

	copybitmap( bitmap, chips[num].bitmap, flipx, flipy, sx, sy, 0, TRANSPARENCY_NONE, 0 );
}

/*************************************
 *
 *	PPU VideoROM banking
 *
 *************************************/
void ppu2c03b_set_videorom_bank( int num, int start_page, int num_pages, int bank, int bank_size )
{
	int i;

	/* check bounds */
	if ( num >= intf->num )
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE  "PPU(set vrom bank): Attempting to access an unmapped chip\n" );
		return;
	}

	if ( !chips[num].has_videorom )
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE  "PPU(set vrom bank): Attempting to switch videorom banks and no rom is mapped\n" );
		return;
	}

	bank &= ( chips[num].videorom_banks * ( CHARGEN_NUM_CHARS / bank_size ) ) - 1;

	for( i = start_page; i < ( start_page + num_pages ); i++ )
		chips[num].nes_vram[i] = bank * bank_size + 64 * ( i - start_page );

	{
		int vram_start = start_page * 0x400;
		int count = num_pages * 0x400;
		int rom_start = bank * bank_size * 16;

		memcpy( &chips[num].videoram[vram_start], &memory_region( intf->vrom_region[num] )[rom_start], count );
	}
}

/*************************************
 *
 *	Utility functions
 *
 *************************************/
int ppu2c03b_get_pixel( int num, int x, int y )
{
	/* check bounds */
	if ( num >= intf->num )
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE  "PPU(get_pixel): Attempting to access an unmapped chip\n" );
		return 0;
	}

	if ( x >= VISIBLE_SCREEN_WIDTH )
		x = VISIBLE_SCREEN_WIDTH - 1;

	if ( y >= VISIBLE_SCREEN_HEIGHT )
		y = VISIBLE_SCREEN_HEIGHT - 1;

	return read_pixel(chips[num].bitmap, x, y);
}

int ppu2c03b_get_colorbase( int num )
{
	/* check bounds */
	if ( num >= intf->num )
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE  "PPU(get_colorbase): Attempting to access an unmapped chip\n" );
		return 0;
	}

	return intf->color_base[num];
}

void ppu2c03b_set_mirroring( int num, int mirroring )
{
	/* check bounds */
	if ( num >= intf->num )
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE  "PPU(get_colorbase): Attempting to access an unmapped chip\n" );
		return;
	}

	/* setup our videoram handlers based on mirroring */
	switch( mirroring )
	{
		case PPU_MIRROR_VERT:
			chips[num].ppu_page[0] = &(chips[num].videoram[0x2000]);
			chips[num].ppu_page[1] = &(chips[num].videoram[0x2400]);
			chips[num].ppu_page[2] = &(chips[num].videoram[0x2000]);
			chips[num].ppu_page[3] = &(chips[num].videoram[0x2400]);
		break;

		case PPU_MIRROR_HORZ:
			chips[num].ppu_page[0] = &(chips[num].videoram[0x2000]);
			chips[num].ppu_page[1] = &(chips[num].videoram[0x2000]);
			chips[num].ppu_page[2] = &(chips[num].videoram[0x2400]);
			chips[num].ppu_page[3] = &(chips[num].videoram[0x2400]);
		break;

		case PPU_MIRROR_HIGH:
			chips[num].ppu_page[0] = &(chips[num].videoram[0x2400]);
			chips[num].ppu_page[1] = &(chips[num].videoram[0x2400]);
			chips[num].ppu_page[2] = &(chips[num].videoram[0x2400]);
			chips[num].ppu_page[3] = &(chips[num].videoram[0x2400]);
		break;

		case PPU_MIRROR_LOW:
			chips[num].ppu_page[0] = &(chips[num].videoram[0x2000]);
			chips[num].ppu_page[1] = &(chips[num].videoram[0x2000]);
			chips[num].ppu_page[2] = &(chips[num].videoram[0x2000]);
			chips[num].ppu_page[3] = &(chips[num].videoram[0x2000]);
		break;

		case PPU_MIRROR_NONE:
		default:
			chips[num].ppu_page[0] = &(chips[num].videoram[0x2000]);
			chips[num].ppu_page[1] = &(chips[num].videoram[0x2400]);
			chips[num].ppu_page[2] = &(chips[num].videoram[0x2800]);
			chips[num].ppu_page[3] = &(chips[num].videoram[0x2c00]);
		break;
	}
}

void ppu2c03b_set_nmi_callback( int num, ppu2c03b_nmi_cb cb )
{
	/* check bounds */
	if ( num >= intf->num )
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE  "PPU(set_nmi_callback): Attempting to access an unmapped chip\n" );
		return;
	}

	intf->nmi_handler[num] = cb;
}

void ppu2c03b_set_scanline_callback( int num, ppu2c03b_scanline_cb cb )
{
	/* check bounds */
	if ( num >= intf->num )
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE  "PPU(set_scanline_callback): Attempting to access an unmapped chip\n" );
		return;
	}

	chips[num].scanline_callback_proc = cb;
}

void ppu2c03b_set_vidaccess_callback( int num, ppu2c03b_vidaccess_cb cb )
{
	/* check bounds */
	if ( num >= intf->num )
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE  "PPU(set_vidaccess_callback): Attempting to access an unmapped chip\n" );
		return;
	}

	chips[num].vidaccess_callback_proc = cb;
}

void ppu2c03b_set_scanlines_per_frame( int num, int scanlines )
{
	/* check bounds */
	if ( num >= intf->num )
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE  "PPU(set_scanlines_per_frame): Attempting to access an unmapped chip\n" );
		return;
	}

	chips[num].scanlines_per_frame = scanlines;
}

/*************************************
 *
 *	Accesors
 *
 *************************************/

READ_HANDLER( ppu2c03b_0_r )
{
	return ppu2c03b_r( 0, offset );
}

READ_HANDLER( ppu2c03b_1_r )
{
	return ppu2c03b_r( 1, offset );
}

WRITE_HANDLER( ppu2c03b_0_w )
{
	ppu2c03b_w( 0, offset, data );
}

WRITE_HANDLER( ppu2c03b_1_w )
{
	ppu2c03b_w( 1, offset, data );
}
