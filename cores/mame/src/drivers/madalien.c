/***************************************************************************

Mad Alien
(c) 1980 Data East Corporation

Driver by Norbert Kehrer (February 2004).

****************************************************************************

Hardware:
---------

Main CPU	6502
Sound CPU	6502
Sound chip	AY-3-8910


Memory map of main board:
-------------------------

$0000 - $03ff	Program RAM 
$6000 - $63ff	Video RAM   
$6800 - $7fff	Character generator RAM
$8004		Flip screen in cocktail mode (?)
$8006		Write: send command to sound board, read: input from sound board
$8008		Write: set counter for shifter circuitry, read: low byte of shift reg. after shift operation
$8009		Write: set data to be shifted, read: high byte of shift reg. after shift operation
$8008		Video register (control of headlight, color, and landscape)
$800d		Background horiz. scroll register left part
$800e		Background horiz. scroll register right part
$800f		Background vertical scroll register
$9000		Input port 1: player 1 controls
$9001		DIP switch port
$9002		Input port 2: player 2 controls
$c000 - $ffff	Program ROM

Main board interrupts: NMI triggered, when coin is inserted


Memory map of sound board:
--------------------------

$0000 - $03ff	Sound board program RAM 
$6500		Sound board register, just to write in someting (?)
$6800		Sound board register, just to write in and read out (?)
$8000		Write: AY-3-8910 control port, read: input from master CPU
$8001		AY-3-8910 data port
$8006		Output to master CPU
$f800 - $ffff	Sound board program ROM

Sound board interrupts: NMI triggered by VBlank signal


Input ports:
------------

Input port 1, mapped to memory address $9000:
7654 3210
1          Unused
 1         Start 2 players game
  1        Start 1 player game
   1       Player 1 fire button
     1     Player 1 down (not used in Mad Alien)
      1    Player 1 up (not used in Mad Alien)
       1   Player 1 left
        1  Player 1 right

DIP switch port, mapped to memory address $9001:
7654 3210
1          VBlank (0 = off, 1 = on)
 1         Game screen (0 = prevent turning, 1 = to turn) (?)
  00       Bonus points (0 = 3000, 1 = 7000, 2 = 5000, 3 = nil)
     00    Game charge (0 = 1 coin/1 play, 1 = 1 coin/2 plays, 2 and 3 = 2 coins 1 play)
       00  Number of cars (0 = 3 cars, 1 = 4 cars, 2 = 5 cars, 3 = 6cars) 

Input port 2, mapped to memory address $9002:
7654 3210
111        Unused
   1       Player 2 fire button
     1     Player 2 down (not used in Mad Alien)
      1    Player 2 up (not used in Mad Alien)
       1   Player 2 left
        1  Player 2 right

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

static struct tilemap *fg_tilemap, *bg_tilemap_l, *bg_tilemap_r;

static struct rectangle bg_tilemap_l_clip;
static struct rectangle bg_tilemap_r_clip;

static struct mame_bitmap *headlight_bitmap, *flip_bitmap;

static UINT8 *madalien_videoram;
static UINT8 *madalien_charram;
static UINT8 *madalien_bgram;
static UINT8 *madalien_shift_data;

static UINT8 madalien_scroll_v;
static UINT8 madalien_scroll_l;
static UINT8 madalien_scroll_r;
static UINT8 madalien_scroll_light;

static UINT8 madalien_shift_counter;
static UINT8 madalien_shift_reg_lo;
static UINT8 madalien_shift_reg_hi;

static UINT8 madalien_video_register;

static UINT8 madalien_bg_map_selector;

static int madalien_select_color_1;
static int madalien_select_color_2;
static int madalien_swap_colors;

static int madalien_headlight_on;

static int madalien_flip_screen;

static int madalien_headlight_source[128][128];

static UINT8 madalien_sound_reg;


PALETTE_INIT( madalien )
{
	int i, j, n, bit0, bit1, r, g, b;

	n = Machine->drv->total_colors / 2;

	for (i = 0; i < n; i++)
	{
		/* red component */
		bit0 = (color_prom[i] >> 0) & 0x01;
		bit1 = (color_prom[i] >> 1) & 0x01;
		r = 0x40 * bit0 + 0x80 * bit1;
		/* green component */
		bit0 = (color_prom[i] >> 2) & 0x01;
		bit1 = (color_prom[i] >> 3) & 0x01;
		g = 0x40 * bit0 + 0x80 * bit1;
		/* blue component */
		bit0 = (color_prom[i] >> 4) & 0x01;
		bit1 = (color_prom[i] >> 5) & 0x01;
		b = 0x40 * bit0 + 0x80 * bit1;

		palette_set_color(i, r, g, b);

		/* Colors for bits 1 and 2 swapped */
		j = i;
		switch (j & 6) {
			case 2:
				j &= 0xf9;
				j |= 0x04;
				break;
			case 4:
				j &= 0xf9;
				j |= 0x02;
				break;
		};
		palette_set_color(j+n, r, g, b);
	}
}

static void get_fg_tile_info(int tile_index)
{
	int code, color;

	code = madalien_videoram[tile_index];
	color = 2 * madalien_select_color_1;

	SET_TILE_INFO(0, code, color, 0)
}

static void get_bg_tile_info_l(int tile_index)
{
	int x, y, code, color, bg_base;

	UINT8 *bgrom = memory_region(REGION_GFX2);

	x = tile_index & 0x0f;
	y = (tile_index / 16) & 0x0f;

	bg_base = madalien_bg_map_selector * 0x0100;

	if (x < 8)
		code = bgrom[bg_base + 8*y + x];
	else
		code = 16;

	color = madalien_swap_colors * 4;

	SET_TILE_INFO(1, code, color, 0)
}

static void get_bg_tile_info_r(int tile_index)
{
	int x, y, code, color, bg_base;

	UINT8 *bgrom = memory_region(REGION_GFX2);

	x = tile_index & 0x0f;
	y = (tile_index / 16) & 0x0f;

	bg_base = madalien_bg_map_selector * 0x0100;

	if (x < 8)
		code = bgrom[bg_base + 0x80 + 8*y + x];
	else 
		code = 16;

	color = madalien_swap_colors * 4;

	SET_TILE_INFO(1, code, color, 0)
}




VIDEO_START( madalien )
{
	int x, y, data1, data2, bit;
	const UINT8 *headlight_rom;

	fg_tilemap = tilemap_create(get_fg_tile_info, tilemap_scan_cols_flip_x, 
		TILEMAP_TRANSPARENT, 8, 8, 32, 32);

	if ( !fg_tilemap )
		return 1;

	bg_tilemap_l = tilemap_create(get_bg_tile_info_l, tilemap_scan_cols_flip_x, 
		TILEMAP_OPAQUE, 16, 16, 16, 16);

	if ( !bg_tilemap_l )
		return 1;

	bg_tilemap_r = tilemap_create(get_bg_tile_info_r, tilemap_scan_cols_flip_x, 
		TILEMAP_OPAQUE, 16, 16, 16, 16);

	if ( !bg_tilemap_r )
		return 1;

	tilemap_set_transparent_pen( fg_tilemap, 0 );

	bg_tilemap_l_clip = Machine->visible_area;
	bg_tilemap_l_clip.max_y = Machine->drv->screen_height / 2;

	bg_tilemap_r_clip = Machine->visible_area;
	bg_tilemap_r_clip.min_y = Machine->drv->screen_height / 2;

	tilemap_set_flip(bg_tilemap_r, TILEMAP_FLIPY);

	headlight_bitmap = auto_bitmap_alloc(128, 128);
	if( !headlight_bitmap )
		return 1;

	flip_bitmap = auto_bitmap_alloc(Machine->drv->screen_width,Machine->drv->screen_height);
	if (!flip_bitmap)
		return 1;

	madalien_bgram = auto_malloc(0x1000);	/* ficticiuos background RAM for empty tile */
	if (!madalien_bgram)
		return 1;

	memset(madalien_bgram, 0, 0x1000);

	madalien_scroll_v = madalien_scroll_l = madalien_scroll_r = 0;

	madalien_shift_data = memory_region( REGION_GFX4 );

	/* Generate headlight shape data: */
	headlight_rom = memory_region( REGION_GFX3 );
	for( x=0; x<64; x++ )
		for( y=0; y<64; y+=8 )
		{
			data1 = headlight_rom[x*16 + (y/8)];
			data2 = headlight_rom[x*16 + (y/8) + 8];
			for( bit=0; bit<8; bit++ )
			{
				madalien_headlight_source[127-(y+bit)][x] = (data1 & 0x01);
				madalien_headlight_source[63-(y+bit)][x] = (data2 & 0x01);
				data1 >>= 1;
				data2 >>= 1;
			}
		}
	for( x=0; x<64; x++ )
		for( y=0; y<128; y++ )
			madalien_headlight_source[y][64+x] = madalien_headlight_source[y][63-x];

	return 0;
}



VIDEO_UPDATE( madalien )
{
	struct rectangle clip;
	int i, yh, x, y, xp, yp, color;

	for (i=0; i<256; i++)
		decodechar(Machine->gfx[0], i, madalien_charram, Machine->drv->gfxdecodeinfo[0].gfxlayout);

	decodechar(Machine->gfx[1], 16, madalien_bgram, Machine->drv->gfxdecodeinfo[1].gfxlayout); /* empty tile */

	tilemap_set_scrolly( bg_tilemap_l, 0, madalien_scroll_l );
	tilemap_set_scrollx( bg_tilemap_l, 0, madalien_scroll_v );

	tilemap_set_scrolly( bg_tilemap_r, 0, madalien_scroll_r );
	tilemap_set_scrollx( bg_tilemap_r, 0, madalien_scroll_v );

	clip = bg_tilemap_l_clip;
	sect_rect(&clip, cliprect);
	tilemap_draw(bitmap, &clip, bg_tilemap_l, 0, 0);

	clip = bg_tilemap_r_clip;
	sect_rect(&clip, cliprect);
	tilemap_draw(bitmap, &clip, bg_tilemap_r, 0, 0);

	tilemap_draw(bitmap, &Machine->visible_area, fg_tilemap, 0, 0);

	/* Draw headlight area using lighter colors: */
	if (madalien_headlight_on && (madalien_bg_map_selector & 1))
	{
		yh = (256 - madalien_scroll_light) & 0xff;
		if (yh >= 192)
			yh = -(255-yh);

		copybitmap(
			headlight_bitmap,	/* dest */
			bitmap,			/* source */
			0, 0, 			/* flipx, flipy */
			0, -yh,			/* scroll x, scroll y */
			cliprect, 		/* clip */
			TRANSPARENCY_NONE, 0	);

		for( x=0; x<128; x++ )
			for( y=0; y<128; y++ )
				if (madalien_headlight_source[x][y])
				{
					xp = x;
					yp = yh + y;
					if( xp >= Machine->visible_area.min_x &&
					    yp >= Machine->visible_area.min_y && 
					    xp <= Machine->visible_area.max_x &&
					    yp <= Machine->visible_area.max_y )
					{
						color = read_pixel(headlight_bitmap, x, y);
						plot_pixel( bitmap, xp, yp, Machine->pens[color+8] );
					}
				}
	};

	/* Flip screen (cocktail mode): */
	if (madalien_flip_screen) {
		copybitmap(flip_bitmap, bitmap, 1, 1, 0, 0, &Machine->visible_area, TRANSPARENCY_NONE, 0);
		copybitmap(bitmap, flip_bitmap, 0, 0, 0, 0, &Machine->visible_area, TRANSPARENCY_NONE, 0);
	};
}



INTERRUPT_GEN( madalien_interrupt )
{
	static int coin;

	int port = readinputport(3) & 0x01;

	if (port != 0x00)    /* Coin insertion triggers an NMI */
	{
		if (coin == 0)
		{
			coin = 1;
			cpu_set_irq_line(0, IRQ_LINE_NMI, PULSE_LINE);
		}
	}
	else
		coin = 0;
}


READ_HANDLER( madalien_shift_reg_lo_r )
{
	return madalien_shift_reg_lo;
}

READ_HANDLER( madalien_shift_reg_hi_r )
{
	return madalien_shift_reg_hi;
}

READ_HANDLER( madalien_videoram_r )
{
	return madalien_videoram[offset];
}

READ_HANDLER( madalien_charram_r )
{
	return madalien_charram[offset];
}

WRITE_HANDLER(madalien_video_register_w)
{
	int bit0, bit2, bit3;

	if ( madalien_video_register != data )
	{
		madalien_video_register = data;

		bit0 = data & 0x01;
		bit2 = ((data & 0x04) >> 2);
		bit3 = ((data & 0x08) >> 3);

		/* Headlight on, if bit 0 is on */
		madalien_headlight_on = bit0;

		/* Swap color bits 1 and 2 of background */ 
		if (bit2 != madalien_swap_colors)
		{
			madalien_swap_colors = bit2;
			tilemap_mark_all_tiles_dirty( fg_tilemap );
		};

		/* Select map (0=landscape A, 1=tunnel, 2=landscape B, 3=tunnel) */
		madalien_bg_map_selector &= 0x01;
		madalien_bg_map_selector |= (bit3 << 1);

		tilemap_mark_all_tiles_dirty( bg_tilemap_l );
		tilemap_mark_all_tiles_dirty( bg_tilemap_r );
	};
}

WRITE_HANDLER( madalien_charram_w )
{
	if (madalien_charram[offset] != data)
	{
		madalien_charram[offset] = data;
		tilemap_mark_all_tiles_dirty(fg_tilemap);
	}
}

WRITE_HANDLER( madalien_scroll_light_w )
{
	madalien_scroll_light = data;
}

WRITE_HANDLER( madalien_scroll_v_w )
{
	madalien_scroll_v = (255 - (data & 0xfc));

	madalien_bg_map_selector &= 0x02;
	madalien_bg_map_selector |= (data & 0x01);

	madalien_select_color_1 = (data & 0x01);

	tilemap_mark_all_tiles_dirty( bg_tilemap_l );
	tilemap_mark_all_tiles_dirty( bg_tilemap_r );
	tilemap_mark_all_tiles_dirty( fg_tilemap );
}

WRITE_HANDLER( madalien_scroll_l_w )
{
	madalien_scroll_l = data;
}

WRITE_HANDLER( madalien_scroll_r_w )
{
	madalien_scroll_r = data;
}

WRITE_HANDLER( madalien_shift_counter_w )
{
	madalien_shift_counter = data & 0x07;
}

UINT8 reverse_bits( int x )	/* bit reversal by wiring in Mad Alien hardware */
{
	int bit, n;
	n = 0;
	for (bit=0; bit<8; bit++) 
		if (x & (1 << bit))
			n |= (1 << (7-bit));
	return n;
}

UINT8 swap_bits( int x )	/* special bit swap by wiring in Mad Alien hardware */
{
	int n = 0;
	if (x & 0x40) n |= 0x01;
	if (x & 0x20) n |= 0x02;
	if (x & 0x10) n |= 0x04;
	if (x & 0x08) n |= 0x08;
	if (x & 0x04) n |= 0x10;
	if (x & 0x02) n |= 0x20;
	if (x & 0x01) n |= 0x40;
	return n;
}

WRITE_HANDLER( madalien_shift_reg_w )
{
	int rom_addr_0, rom_addr_1;
	rom_addr_0 = madalien_shift_counter * 256 + data;
	rom_addr_1 = ((madalien_shift_counter^0x07) & 0x07) * 256 + reverse_bits(data);
	madalien_shift_reg_lo = madalien_shift_data[rom_addr_0];
	madalien_shift_reg_hi = swap_bits( madalien_shift_data[rom_addr_1] );
}

WRITE_HANDLER( madalien_videoram_w )
{
	if (madalien_videoram[offset] != data)
	{
		madalien_videoram[offset] = data;
		tilemap_mark_tile_dirty(fg_tilemap, offset);
	}
}

WRITE_HANDLER( madalien_flip_screen_w )
{
	if (readinputport(1) & 0x40)	/* hack for screen flipping in cocktail mode - main board schematics needed */
		madalien_flip_screen = (data & 1);
	else
		madalien_flip_screen = 0;
}

WRITE_HANDLER( madalien_sound_command_w )
{
	soundlatch_w(offset, data);
	cpu_set_irq_line(1, 0, HOLD_LINE);
}

WRITE_HANDLER( madalien_soundreg_w )
{
	madalien_sound_reg = data;
}

READ_HANDLER( madalien_soundreg_r )
{
	return madalien_sound_reg;
}


static struct GfxLayout charlayout_memory =
{
	8,8,    /* 8*8 characters */
	256,	/* 256 characters */
	3,      /* 3 bits per pixel */
	{ 2*256*8*8, 256*8*8, 0 },  /* the 3 bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8     /* every char takes 8 consecutive bytes */
};

static struct GfxLayout tilelayout =
{
	16,16,  /* 16*16 tiles */
	16+1,	/* 16 tiles + 1 empty tile */
	3,      /* 3 bits per pixel */
	{ 4*16*16*16+4, 2*16*16*16+4, 4 }, 
	{ 3*16*8+0, 3*16*8+1, 3*16*8+2, 3*16*8+3, 2*16*8+0, 2*16*8+1, 2*16*8+2, 2*16*8+3,
	  16*8+0, 16*8+1, 16*8+2, 16*8+3, 0, 1, 2, 3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	64*8    /* every tile takes 64 consecutive bytes */
};


static struct GfxDecodeInfo madalien_gfxdecodeinfo[] =
{
	{ 0, 0, &charlayout_memory,	0, 8 }, /* characters (the game dynamically modifies them) */
	{ REGION_GFX1, 0, &tilelayout,	0, 8 },	/* background tiles */
	{ -1 } /* end of array */
};


static struct AY8910interface ay8910_interface =
{
	1,		/* 1 chip */
	500000,		/* 500 kHz like sound CPU (?) */
	{ 23, 23 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};


static MEMORY_READ_START( madalien_readmem )
    { 0x0000, 0x03ff, MRA_RAM },			/* Program RAM */
	{ 0x6000, 0x63ff, madalien_videoram_r },		/* Video RAM   */
	{ 0x6800, 0x7fff, madalien_charram_r },		/* Character generator RAM */
	{ 0x8006, 0x8006, soundlatch2_r },			/* Input from sound board */
	{ 0x8008, 0x8008, madalien_shift_reg_lo_r },	/* Low byte of shift reg. after shift operation */
	{ 0x8009, 0x8009, madalien_shift_reg_hi_r },	/* Low byte of shift reg. after shift operation */
	{ 0x9000, 0x9000, input_port_0_r },   		/* Input ports */
	{ 0x9001, 0x9001, input_port_1_r },    
	{ 0x9002, 0x9002, input_port_2_r },    
	{ 0xc000, 0xffff, MRA_ROM },			/* Program ROM */
MEMORY_END



static MEMORY_WRITE_START( madalien_writemem )
    { 0x0000, 0x03ff, MWA_RAM },						/* Normal RAM */
	{ 0x6000, 0x63ff, madalien_videoram_w, &madalien_videoram },	/* Video RAM   */
	{ 0x6800, 0x7fff, madalien_charram_w, &madalien_charram },	/* Character generator RAM */
	{ 0x8004, 0x8004, madalien_flip_screen_w },		/* Flip screen in cocktail mode */
	{ 0x8006, 0x8006, madalien_sound_command_w },	/* Command to sound board */
	{ 0x8008, 0x8008, madalien_shift_counter_w },		/* Set counter for shifter circuitry */
	{ 0x8009, 0x8009, madalien_shift_reg_w }, 		/* Put data to be shifted */
	{ 0x800b, 0x800b, madalien_video_register_w },		/* Video register (light/color/landscape) */
	{ 0x800c, 0x800c, madalien_scroll_light_w },		/* Car headlight horiz. scroll register */
	{ 0x800d, 0x800d, madalien_scroll_l_w },		/* Background horiz. scroll register left part */
	{ 0x800e, 0x800e, madalien_scroll_r_w },		/* Background horiz. scroll register right part */
	{ 0x800f, 0x800f, madalien_scroll_v_w },		/* Background vertical scroll register */
	{ 0xc000, 0xffff, MWA_ROM },			/* Program ROM */
MEMORY_END


static MEMORY_READ_START( sound_readmem )
    { 0x0000, 0x03ff, MRA_RAM },		/* Sound board RAM */
	{ 0x6800, 0x6800, madalien_soundreg_r },	/* Sound board register, just to write in and read out */
	{ 0x8000, 0x8000, soundlatch_r },		/* Sound board input from master CPU */
	{ 0xf800, 0xffff, MRA_ROM },		/* Sound board program ROM */
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
    { 0x0000, 0x03ff, MWA_RAM },			/* Sound board RAM */
	{ 0x6500, 0x6500, madalien_soundreg_w },		/* Sound board register */
	{ 0x8000, 0x8000, AY8910_control_port_0_w },	/* AY-3-8910 control port */
	{ 0x8001, 0x8001, AY8910_write_port_0_w },	/* AY-3-8910 data port */
	{ 0x8006, 0x8006, soundlatch2_w },		/* Sound board output to master CPU */
	{ 0xf800, 0xffff, MWA_ROM },			/* Sound board program ROM */
MEMORY_END



INPUT_PORTS_START( madalien )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x00, "3" )
	PORT_DIPSETTING(	0x01, "4" )
	PORT_DIPSETTING(	0x02, "5" )
	PORT_DIPSETTING(	0x03, "6" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x0c, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x30, 0x30, "Bonus_points" )
	PORT_DIPSETTING(    0x00, "3000" )
	PORT_DIPSETTING(    0x20, "5000" )
	PORT_DIPSETTING(    0x10, "7000"  )
	PORT_DIPSETTING(    0x30, "nil"  )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Cocktail ) )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK  )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* Fake port or coin: coin insertion triggers an NMI */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0xfe, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END



static MACHINE_DRIVER_START( madalien )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M6502, 750000)    /* 750 kHz ? */
	MDRV_CPU_MEMORY(madalien_readmem, madalien_writemem)
	MDRV_CPU_VBLANK_INT(madalien_interrupt, 1)

	MDRV_CPU_ADD_TAG("sound", M6502, 500000)   /* 500 kHz ? */
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem, sound_writemem)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse, 16)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(3072)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 32*8-1) 
	MDRV_GFXDECODE(madalien_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2*32)
	MDRV_COLORTABLE_LENGTH(2*32)
	MDRV_PALETTE_INIT(madalien)
	MDRV_VIDEO_START(madalien)
	MDRV_VIDEO_UPDATE(madalien)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)

MACHINE_DRIVER_END


ROM_START( madalien )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) 			 /* 64k for 6502 code of main CPU */
	ROM_LOAD( "m7",	0xc000, 0x0800, CRC(4d12f89d) SHA1(e155f9135bc2bea56e211052f2b74d25e76308c8) )
	ROM_LOAD( "m6",	0xc800, 0x0800, CRC(1bc4a57b) SHA1(02252b868d0c07c0a18240e9d831c303cdcfa9a6) )
	ROM_LOAD( "m5",	0xd000, 0x0800, CRC(8db99572) SHA1(f8cf22f8c134b47756b7f02c5ca0217100466744) )
	ROM_LOAD( "m4",	0xd800, 0x0800, CRC(fba671af) SHA1(dd74bd357c82d525948d836a7f860bbb3182c825) )
	ROM_LOAD( "m3",	0xe000, 0x0800, CRC(1aad640d) SHA1(9ace7d2c5ef9e789c2b8cc65420b19ce72cd95fa) )
	ROM_LOAD( "m2",	0xe800, 0x0800, CRC(cbd533a0) SHA1(d3be81fb9ba40e30e5ff0171efd656b11dd20f2b) )
	ROM_LOAD( "m1",	0xf000, 0x0800, CRC(ad654b1d) SHA1(f8b365dae3801e97e04a10018a790d3bdb5d9439) )
	ROM_LOAD( "m0",	0xf800, 0x0800, CRC(cf7aa787) SHA1(f852cc806ecc582661582326747974a14f50174a) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     		/* 64k for 6502 code of audio CPU */
	ROM_LOAD( "m8", 0xf800, 0x0400, CRC(cfd19dab) SHA1(566dc84ffe9bcaeb112250a9e1882bf62f47b579) )
	ROM_LOAD( "m9", 0xfc00, 0x0400, CRC(48f30f24) SHA1(9c0bf6e43b143d6af1ebb9dad2bdc2b53eb2e48e) )

	ROM_REGION( 0x6000, REGION_GFX1, ROMREGION_DISPOSE )   /* Background tiles */
	ROM_LOAD( "mc", 0x0000, 0x0400, CRC(2daadfb7) SHA1(8be084a39b256e538fd57111e92d47115cb142cd) )
	ROM_LOAD( "md", 0x0400, 0x0400, CRC(3ee1287a) SHA1(33bc59a8d09d22f3db80f881c2f37aa788718138) )
	ROM_LOAD( "me", 0x0800, 0x0400, CRC(45a5c201) SHA1(ac600afeabf494634c3189d8e96644bd0deb45f3) )

	ROM_REGION( 0x6000, REGION_GFX2, 0 )  			/* Background tile maps */
	ROM_LOAD( "mf", 0x0000, 0x0400, CRC(e9cba773) SHA1(356c7edb1b412a9e04f0747e780c945af8791c55) )

	ROM_REGION( 0x0400, REGION_GFX3, 0 )			/* Car headlight */
	ROM_LOAD( "ma", 0x0000, 0x0400, CRC(aab16446) SHA1(d2342627cc2766004343f27515d8a7989d5fe932) )

	ROM_REGION( 0x0800, REGION_GFX4, 0 )			/* Shifting data */
	ROM_LOAD( "mb", 0x0000, 0x0800, CRC(cb801e49) SHA1(7444c4af7cf07e5fdc54044d62ea4fcb201b2b8b) )

	ROM_REGION( 0x0060, REGION_PROMS, 0 ) 			/* Color PROM */
	ROM_LOAD( "mg",	0x0000, 0x0020, CRC(3395b31f) SHA1(26235fb448a4180c58f0887e53a29c17857b3b34) )
ROM_END


DRIVER_INIT( madalien )	
{
	madalien_shift_counter = 0;
	madalien_shift_reg_lo  = 0;
	madalien_shift_reg_hi  = 0;
	madalien_bg_map_selector = 0;
	madalien_scroll_light = 0;
	madalien_select_color_1 = 0;
	madalien_select_color_2 = 0;
	madalien_headlight_on = 0;
	madalien_swap_colors = 0;
	madalien_video_register = 0;
	madalien_flip_screen = 0;
}


/*          rom       parent  machine   inp       init */
GAME( 1980, madalien, 0,      madalien, madalien, madalien, ROT270, "Data East Corporation", "Mad Alien" )
