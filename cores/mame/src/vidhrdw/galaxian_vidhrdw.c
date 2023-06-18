/***************************************************************************

  Galaxian hardware family

***************************************************************************/

#include "driver.h"

static struct rectangle _spritevisiblearea =
{
	2*8+1, 32*8-1,
	2*8,   30*8-1
};
static struct rectangle _spritevisibleareaflipx =
{
	0*8, 30*8-2,
	2*8, 30*8-1
};

static struct rectangle* spritevisiblearea;
static struct rectangle* spritevisibleareaflipx;


#define STARS_COLOR_BASE 		(memory_region_length(REGION_PROMS))
#define BULLETS_COLOR_BASE		(STARS_COLOR_BASE + 64)
#define BACKGROUND_COLOR_BASE	(BULLETS_COLOR_BASE + 2)


data8_t *galaxian_videoram;
data8_t *galaxian_spriteram;
data8_t *galaxian_spriteram2;
data8_t *galaxian_attributesram;
data8_t *galaxian_bulletsram;
data8_t *rockclim_videoram;
size_t galaxian_spriteram_size;
size_t galaxian_spriteram2_size;
size_t galaxian_bulletsram_size;


static void get_tile_info(int tile_index);
static void rockclim_get_tile_info(int tile_index);
static struct tilemap *tilemap;
static struct tilemap *rockclim_tilemap;
static int mooncrst_gfxextend;
static int gfxbank[5];
static int spriteram2_present;
static int flip_screen_x;
static int flip_screen_y;
static int color_mask;
static void (*modify_charcode)(UINT16 *code,UINT8 x);		/* function to call to do character banking */
static void  gmgalax_modify_charcode(UINT16 *code,UINT8 x);
static void mooncrst_modify_charcode(UINT16 *code,UINT8 x);
static void mooncrgx_modify_charcode(UINT16 *code,UINT8 x);
static void  moonqsr_modify_charcode(UINT16 *code,UINT8 x);
static void mshuttle_modify_charcode(UINT16 *code,UINT8 x);
static void   pisces_modify_charcode(UINT16 *code,UINT8 x);
static void mimonkey_modify_charcode(UINT16 *code,UINT8 x);
static void  batman2_modify_charcode(UINT16 *code,UINT8 x);
static void  mariner_modify_charcode(UINT16 *code,UINT8 x);
static void  jumpbug_modify_charcode(UINT16 *code,UINT8 x);

static void (*modify_spritecode)(data8_t *spriteram,int*,int*,int*,int);	/* function to call to do sprite banking */
static void  gmgalax_modify_spritecode(data8_t *spriteram,int *code,int *flipx,int *flipy,int offs);
static void mooncrst_modify_spritecode(data8_t *spriteram,int *code,int *flipx,int *flipy,int offs);
static void mooncrgx_modify_spritecode(data8_t *spriteram,int *code,int *flipx,int *flipy,int offs);
static void  moonqsr_modify_spritecode(data8_t *spriteram,int *code,int *flipx,int *flipy,int offs);
static void mshuttle_modify_spritecode(data8_t *spriteram,int *code,int *flipx,int *flipy,int offs);
static void  calipso_modify_spritecode(data8_t *spriteram,int *code,int *flipx,int *flipy,int offs);
static void   pisces_modify_spritecode(data8_t *spriteram,int *code,int *flipx,int *flipy,int offs);
static void mimonkey_modify_spritecode(data8_t *spriteram,int *code,int *flipx,int *flipy,int offs);
static void  batman2_modify_spritecode(data8_t *spriteram,int *code,int *flipx,int *flipy,int offs);
static void  jumpbug_modify_spritecode(data8_t *spriteram,int *code,int *flipx,int *flipy,int offs);
static void dkongjrm_modify_spritecode(data8_t *spriteram,int *code,int *flipx,int *flipy,int offs);

static void (*modify_color)(UINT8 *color);	/* function to call to do modify how the color codes map to the PROM */
static void frogger_modify_color(UINT8 *color);
static void gmgalax_modify_color(UINT8 *color);
static void drivfrcg_modify_color(UINT8 *color);

static void (*modify_ypos)(UINT8*);	/* function to call to do modify how vertical positioning bits are connected */
static void frogger_modify_ypos(UINT8 *sy);

static void stars_blink_callback(int param);
static void stars_scroll_callback(int param);

static void (*tilemap_set_scroll)( struct tilemap *, int col, int value );

/* star circuit */
#define STAR_COUNT  252
struct star
{
	int x,y,color;
};
static struct star stars[STAR_COUNT];
static int stars_colors_start;
       int galaxian_stars_on;
static int stars_scrollpos;
static int stars_blink_state;
static void *stars_blink_timer;
static void *stars_scroll_timer;
static int timer_adjusted;
       void galaxian_init_stars(int colors_offset);
static void (*draw_stars)(struct mame_bitmap *);		/* function to call to draw the star layer */
static void     noop_draw_stars(struct mame_bitmap *bitmap);
       void galaxian_draw_stars(struct mame_bitmap *bitmap);
	   void scramble_draw_stars(struct mame_bitmap *bitmap);
static void   rescue_draw_stars(struct mame_bitmap *bitmap);
static void  mariner_draw_stars(struct mame_bitmap *bitmap);
static void  jumpbug_draw_stars(struct mame_bitmap *bitmap);
static void start_stars_blink_timer(double ra, double rb, double c);
static void start_stars_scroll_timer(void);

/* bullets circuit */
static int darkplnt_bullet_color;
static void (*draw_bullets)(struct mame_bitmap *,int,int,int);	/* function to call to draw a bullet */
static void galaxian_draw_bullets(struct mame_bitmap *bitmap, int offs, int x, int y);
static void gteikob2_draw_bullets(struct mame_bitmap *bitmap, int offs, int x, int y);
static void scramble_draw_bullets(struct mame_bitmap *bitmap, int offs, int x, int y);
static void   theend_draw_bullets(struct mame_bitmap *bitmap, int offs, int x, int y);
static void darkplnt_draw_bullets(struct mame_bitmap *bitmap, int offs, int x, int y);

/* background circuit */
static int background_enable;
static int background_red, background_green, background_blue;
static void (*draw_background)(struct mame_bitmap *);	/* function to call to draw the background */
static void galaxian_draw_background(struct mame_bitmap *bitmap);
static void scramble_draw_background(struct mame_bitmap *bitmap);
static void  turtles_draw_background(struct mame_bitmap *bitmap);
static void  mariner_draw_background(struct mame_bitmap *bitmap);
static void  frogger_draw_background(struct mame_bitmap *bitmap);
static void stratgyx_draw_background(struct mame_bitmap *bitmap);
static void  minefld_draw_background(struct mame_bitmap *bitmap);
static void   rescue_draw_background(struct mame_bitmap *bitmap);


/***************************************************************************

  Convert the color PROMs into a more useable format.

  Galaxian has one 32 bytes palette PROM, connected to the RGB output this way:

  bit 7 -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 220 ohm resistor  -- GREEN
        -- 470 ohm resistor  -- GREEN
        -- 1  kohm resistor  -- GREEN
        -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
  bit 0 -- 1  kohm resistor  -- RED

  The output of the background star generator is connected this way:

  bit 5 -- 100 ohm resistor  -- BLUE
        -- 150 ohm resistor  -- BLUE
        -- 100 ohm resistor  -- GREEN
        -- 150 ohm resistor  -- GREEN
        -- 100 ohm resistor  -- RED
  bit 0 -- 150 ohm resistor  -- RED

  The blue background in Scramble and other games goes through a 390 ohm
  resistor.

  The bullet RGB outputs go through 100 ohm resistors.

  The RGB outputs have a 470 ohm pull-down each.

***************************************************************************/
PALETTE_INIT( galaxian )
{
	int i;


	/* first, the character/sprite palette */

	for (i = 0;i < memory_region_length(REGION_PROMS);i++)
	{
		int bit0,bit1,bit2,r,g,b;

		/* red component */
		bit0 = BIT(*color_prom,0);
		bit1 = BIT(*color_prom,1);
		bit2 = BIT(*color_prom,2);
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = BIT(*color_prom,3);
		bit1 = BIT(*color_prom,4);
		bit2 = BIT(*color_prom,5);
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = BIT(*color_prom,6);
		bit1 = BIT(*color_prom,7);
		b = 0x4f * bit0 + 0xa8 * bit1;

		palette_set_color(i,r,g,b);
		color_prom++;
	}


	galaxian_init_stars(STARS_COLOR_BASE);


	/* bullets - yellow and white */
	palette_set_color(BULLETS_COLOR_BASE+0,0xef,0xef,0x00);
	palette_set_color(BULLETS_COLOR_BASE+1,0xef,0xef,0xef);
}

PALETTE_INIT( scramble )
{
	palette_init_galaxian(colortable, color_prom);


	/* blue background - 390 ohm resistor */
	palette_set_color(BACKGROUND_COLOR_BASE,0,0,0x56);
}

PALETTE_INIT( moonwar )
{
	palette_init_scramble(colortable, color_prom);


	/* wire mod to connect the bullet blue output to the 220 ohm resistor */
	palette_set_color(BULLETS_COLOR_BASE+0,0xef,0xef,0x97);
}

PALETTE_INIT( turtles )
{
	int i;


	palette_init_galaxian(colortable, color_prom);


	/*  The background color generator is connected this way:

		RED   - 390 ohm resistor
		GREEN - 470 ohm resistor
		BLUE  - 390 ohm resistor */

	for (i = 0; i < 8; i++)
	{
		int r = BIT(i,0) * 0x55;
		int g = BIT(i,1) * 0x47;
		int b = BIT(i,2) * 0x55;

		palette_set_color(BACKGROUND_COLOR_BASE+i,r,g,b);
	}
}

PALETTE_INIT( stratgyx )
{
	int i;


	palette_init_galaxian(colortable, color_prom);


	/*  The background color generator is connected this way:

		RED   - 270 ohm resistor
		GREEN - 560 ohm resistor
		BLUE  - 470 ohm resistor */

	for (i = 0; i < 8; i++)
	{
		int r = BIT(i,0) * 0x7c;
		int g = BIT(i,1) * 0x3c;
		int b = BIT(i,2) * 0x47;

		palette_set_color(BACKGROUND_COLOR_BASE+i,r,g,b);
	}
}

PALETTE_INIT( frogger )
{
	palette_init_galaxian(colortable, color_prom);


	/* blue background - 470 ohm resistor */
	palette_set_color(BACKGROUND_COLOR_BASE,0,0,0x47);
}

PALETTE_INIT( rockclim )
{
	int i;


	/* first, the character/sprite palette */

	for (i = 0;i < memory_region_length(REGION_PROMS);i++)
	{
		int bit0,bit1,bit2,r,g,b;

		/* red component */
		bit0 = BIT(*color_prom,0);
		bit1 = BIT(*color_prom,1);
		bit2 = BIT(*color_prom,2);
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = BIT(*color_prom,3);
		bit1 = BIT(*color_prom,4);
		bit2 = BIT(*color_prom,5);
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = BIT(*color_prom,6);
		bit1 = BIT(*color_prom,7);
		b = 0x4f * bit0 + 0xa8 * bit1;

		palette_set_color(i,r,g,b);
		color_prom++;
	}
}
/***************************************************************************

  Convert the color PROMs into a more useable format.

  Dark Planet has one 32 bytes palette PROM, connected to the RGB output this way:

  bit 5 -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 1  kohm resistor  -- BLUE
        -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
  bit 0 -- 1  kohm resistor  -- RED

  The bullet RGB outputs go through 100 ohm resistors.

  The RGB outputs have a 470 ohm pull-down each.

***************************************************************************/
PALETTE_INIT( darkplnt )
{
	int i;


	/* first, the character/sprite palette */

	for (i = 0;i < 32;i++)
	{
		int bit0,bit1,bit2,r,g,b;

		/* red component */
		bit0 = BIT(*color_prom,0);
		bit1 = BIT(*color_prom,1);
		bit2 = BIT(*color_prom,2);
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		g = 0x00;
		/* blue component */
		bit0 = BIT(*color_prom,3);
		bit1 = BIT(*color_prom,4);
		bit2 = BIT(*color_prom,5);
		b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		palette_set_color(i,r,g,b);
		color_prom++;
	}


	/* bullets - red and blue */
	palette_set_color(BULLETS_COLOR_BASE+0,0xef,0x00,0x00);
	palette_set_color(BULLETS_COLOR_BASE+1,0x00,0x00,0xef);
}

PALETTE_INIT( minefld )
{
	int i;


	palette_init_galaxian(colortable, color_prom);


	/* set up background colors */

	/* graduated blue */

	for (i = 0; i < 128; i++)
	{
		int r = 0;
		int g = i;
		int b = i * 2;
		palette_set_color(BACKGROUND_COLOR_BASE+i,r,g,b);
	}

	/* graduated brown */

	for (i = 0; i < 128; i++)
	{
		int r = i * 1.5;
		int g = i * 0.75;
		int b = i / 2;
		palette_set_color(BACKGROUND_COLOR_BASE+128+i,r,g,b);
	}
}

PALETTE_INIT( rescue )
{
	int i;


	palette_init_galaxian(colortable, color_prom);


	/* set up background colors */

	/* graduated blue */

	for (i = 0; i < 128; i++)
	{
		int r = 0;
		int g = i;
		int b = i * 2;
		palette_set_color(BACKGROUND_COLOR_BASE+i,r,g,b);
	}
}

PALETTE_INIT( mariner )
{
	int i;


	palette_init_galaxian(colortable, color_prom);


	/* set up background colors */

	/* 16 shades of blue - the 4 bits are connected to the following resistors:

		bit 0 -- 4.7 kohm resistor
			  -- 2.2 kohm resistor
			  -- 1   kohm resistor
		bit 0 -- .47 kohm resistor */

	for (i = 0; i < 16; i++)
	{
		int r,g,b;

		r = 0;
		g = 0;
		b = 0x0e * BIT(i,0) + 0x1f * BIT(i,1) + 0x43 * BIT(i,2) + 0x8f * BIT(i,3);

		palette_set_color(BACKGROUND_COLOR_BASE+i,r,g,b);
	}
}


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

static int video_start_common(UINT32 (*get_memory_offset)(UINT32,UINT32,UINT32,UINT32))
{
	tilemap = tilemap_create(get_tile_info,get_memory_offset,TILEMAP_TRANSPARENT,8,8,32,32);

	if (!tilemap)
		return 1;

	tilemap_set_transparent_pen(tilemap,0);


	modify_charcode = 0;
	modify_spritecode = 0;
	modify_color = 0;
	modify_ypos = 0;

	mooncrst_gfxextend = 0;

	draw_bullets = 0;

	draw_background = galaxian_draw_background;
	background_enable = 0;
	background_blue = 0;
	background_red = 0;
	background_green = 0;

	draw_stars = noop_draw_stars;

	flip_screen_x = 0;
	flip_screen_y = 0;

	spriteram2_present = 0;

	spritevisiblearea      = &_spritevisiblearea;
	spritevisibleareaflipx = &_spritevisibleareaflipx;

	color_mask = (Machine->gfx[0]->color_granularity == 4) ? 7 : 3;

	return 0;
}

VIDEO_START( galaxian_plain )
{
	int ret = video_start_common(tilemap_scan_rows);

	tilemap_set_scroll_cols(tilemap, 32);
	tilemap_set_scroll = tilemap_set_scrolly;

	return ret;
}

VIDEO_START( galaxian )
{
	int ret = video_start_galaxian_plain();

	draw_stars = galaxian_draw_stars;

	draw_bullets = galaxian_draw_bullets;

	return ret;
}

VIDEO_START( gmgalax )
{
	int ret = video_start_galaxian();

	modify_charcode   = gmgalax_modify_charcode;
	modify_spritecode = gmgalax_modify_spritecode;
	modify_color      = gmgalax_modify_color;

	return ret;
}

VIDEO_START( mooncrst )
{
	int ret = video_start_galaxian();

	modify_charcode   = mooncrst_modify_charcode;
	modify_spritecode = mooncrst_modify_spritecode;

	return ret;
}

VIDEO_START( mooncrgx )
{
	int ret = video_start_galaxian();

	modify_charcode   = mooncrgx_modify_charcode;
	modify_spritecode = mooncrgx_modify_spritecode;

	return ret;
}

VIDEO_START( moonqsr )
{
	int ret = video_start_galaxian();

	modify_charcode   = moonqsr_modify_charcode;
	modify_spritecode = moonqsr_modify_spritecode;

	return ret;
}

VIDEO_START( mshuttle )
{
	int ret = video_start_galaxian();

	modify_charcode   = mshuttle_modify_charcode;
	modify_spritecode = mshuttle_modify_spritecode;

	return ret;
}

VIDEO_START( pisces )
{
	int ret = video_start_galaxian();

	modify_charcode   = pisces_modify_charcode;
	modify_spritecode = pisces_modify_spritecode;

	return ret;
}

VIDEO_START( gteikob2 )
{
	int ret = video_start_pisces();

	draw_bullets = gteikob2_draw_bullets;

	return ret;
}

VIDEO_START( batman2 )
{
	int ret = video_start_galaxian();

	modify_charcode   = batman2_modify_charcode;
	modify_spritecode = batman2_modify_spritecode;

	return ret;
}

VIDEO_START( scramble )
{
	int ret = video_start_galaxian_plain();

	draw_stars = scramble_draw_stars;

	draw_bullets = scramble_draw_bullets;

	draw_background = scramble_draw_background;

	return ret;
}

VIDEO_START( sfx )
{
	int ret = video_start_common(tilemap_scan_cols);

	tilemap_set_scroll_rows(tilemap, 32);
	tilemap_set_scroll = tilemap_set_scrollx;

	draw_stars = scramble_draw_stars;

	draw_bullets = scramble_draw_bullets;

	draw_background = turtles_draw_background;

	return ret;
}

VIDEO_START( turtles )
{
	int ret = video_start_galaxian_plain();

	draw_background = turtles_draw_background;

	return ret;
}

VIDEO_START( theend )
{
	int ret = video_start_galaxian();

	draw_bullets = theend_draw_bullets;

	return ret;
}

VIDEO_START( darkplnt )
{
	int ret = video_start_galaxian_plain();

	draw_bullets = darkplnt_draw_bullets;

	return ret;
}

VIDEO_START( rescue )
{
	int ret = video_start_scramble();

	draw_stars = rescue_draw_stars;

	draw_background = rescue_draw_background;

	return ret;
}

VIDEO_START( minefld )
{
	int ret = video_start_scramble();

	draw_stars = rescue_draw_stars;

	draw_background = minefld_draw_background;

	return ret;
}

VIDEO_START( stratgyx )
{
	int ret = video_start_galaxian_plain();

	draw_background = stratgyx_draw_background;

	return ret;
}

VIDEO_START( ckongs )
{
	int ret = video_start_scramble();

	modify_spritecode = mshuttle_modify_spritecode;

	return ret;
}

VIDEO_START( calipso )
{
	int ret = video_start_galaxian_plain();

	draw_bullets = scramble_draw_bullets;

	draw_background = scramble_draw_background;

	modify_spritecode = calipso_modify_spritecode;

	return ret;
}

VIDEO_START( mariner )
{
	int ret = video_start_galaxian_plain();

	draw_stars = mariner_draw_stars;

	draw_bullets = scramble_draw_bullets;

	draw_background = mariner_draw_background;

	modify_charcode = mariner_modify_charcode;

	return ret;
}

VIDEO_START( froggers )
{
	int ret = video_start_galaxian_plain();

	draw_background = frogger_draw_background;

	return ret;
}

VIDEO_START( frogger )
{
	int ret = video_start_froggers();

	modify_color = frogger_modify_color;
	modify_ypos = frogger_modify_ypos;

	return ret;
}

VIDEO_START( froggrmc )
{
	int ret = video_start_froggers();

	modify_color = frogger_modify_color;

	return ret;
}

VIDEO_START( jumpbug )
{
	int ret = video_start_scramble();

	draw_stars = jumpbug_draw_stars;

	modify_charcode   = jumpbug_modify_charcode;
	modify_spritecode = jumpbug_modify_spritecode;

	return ret;
}

VIDEO_START( mimonkey )
{
	int ret = video_start_scramble();

	modify_charcode   = mimonkey_modify_charcode;
	modify_spritecode = mimonkey_modify_spritecode;

	return ret;
}

VIDEO_START( dkongjrm )
{
	int ret = video_start_galaxian_plain();

	modify_charcode   = pisces_modify_charcode;
	modify_spritecode = dkongjrm_modify_spritecode;

	spriteram2_present= 1;

	return ret;
}

VIDEO_START( newsin7 )
{
	int ret = video_start_scramble();

	spritevisiblearea      = &_spritevisibleareaflipx;
	spritevisibleareaflipx = &_spritevisiblearea;

	return ret;
}


static void rockclim_draw_background(struct mame_bitmap *bitmap)
{
	tilemap_draw(bitmap,0,rockclim_tilemap, 0,0);
}

static void rockclim_modify_spritecode(data8_t *spriteram,int *code,int *flipx,int *flipy,int offs)
{
	if (gfxbank[2])	*code|=0x40;
}

VIDEO_START( rockclim )
{
	int ret = video_start_galaxian();
	rockclim_tilemap = tilemap_create(rockclim_get_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,8,8,64,32);
	draw_background = rockclim_draw_background;
	modify_charcode = mooncrst_modify_charcode;
	modify_spritecode = rockclim_modify_spritecode;
	return ret;
}

static void drivfrcg_get_tile_info(int tile_index)
{
	int code = galaxian_videoram[tile_index];
	UINT8 x = tile_index & 0x1f;
	UINT8 color = galaxian_attributesram[(x << 1) | 1] & 7;
	UINT8 bank = galaxian_attributesram[(x << 1) | 1] & 0x30;

	code |= (bank << 4);
	color |= ((galaxian_attributesram[(x << 1) | 1] & 0x40) >> 3);

	SET_TILE_INFO(0, code, color, 0)
}

VIDEO_START( drivfrcg )
{
	tilemap = tilemap_create(drivfrcg_get_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,32,32);

	if (!tilemap)
		return 1;

	tilemap_set_transparent_pen(tilemap,0);
	tilemap_set_scroll_cols(tilemap, 32);
	tilemap_set_scroll = tilemap_set_scrolly;

	modify_charcode = 0;
	modify_spritecode = mshuttle_modify_spritecode;
	modify_color = drivfrcg_modify_color;
	modify_ypos = 0;

	mooncrst_gfxextend = 0;

	draw_bullets = 0;

	draw_background = galaxian_draw_background;
	background_enable = 0;
	background_blue = 0;
	background_red = 0;
	background_green = 0;

	draw_stars = noop_draw_stars;

	flip_screen_x = 0;
	flip_screen_y = 0;

	spriteram2_present = 0;

	spritevisiblearea      = &_spritevisiblearea;
	spritevisibleareaflipx = &_spritevisibleareaflipx;

	color_mask = 0xff;

	return 0;
}


WRITE_HANDLER( galaxian_videoram_w )
{
	if (galaxian_videoram[offset] != data)
	{
		galaxian_videoram[offset] = data;
		tilemap_mark_tile_dirty(tilemap, offset);
	}
}

READ_HANDLER( galaxian_videoram_r )
{
	return galaxian_videoram[offset];
}


WRITE_HANDLER( galaxian_attributesram_w )
{
	if (galaxian_attributesram[offset] != data)
	{
		if (offset & 0x01)
		{
			/* color change */
			int i;

			for (i = offset >> 1; i < 0x0400; i += 32)
				tilemap_mark_tile_dirty(tilemap, i);
		}
		else
		{
			if (modify_ypos)
			{
				modify_ypos(&data);
			}

			tilemap_set_scroll(tilemap, offset >> 1, data);
		}

		galaxian_attributesram[offset] = data;
	}
}


WRITE_HANDLER( galaxian_flip_screen_x_w )
{
	if (flip_screen_x != (data & 0x01))
	{
		flip_screen_x = data & 0x01;

		tilemap_set_flip(tilemap, (flip_screen_x ? TILEMAP_FLIPX : 0) | (flip_screen_y ? TILEMAP_FLIPY : 0));
	}
}

WRITE_HANDLER( galaxian_flip_screen_y_w )
{
	if (flip_screen_y != (data & 0x01))
	{
		flip_screen_y = data & 0x01;

		tilemap_set_flip(tilemap, (flip_screen_x ? TILEMAP_FLIPX : 0) | (flip_screen_y ? TILEMAP_FLIPY : 0));
	}
}


WRITE_HANDLER( gteikob2_flip_screen_x_w )
{
	galaxian_flip_screen_x_w(offset, ~data);
}

WRITE_HANDLER( gteikob2_flip_screen_y_w )
{
	galaxian_flip_screen_y_w(offset, ~data);
}


WRITE_HANDLER( hotshock_flip_screen_w )
{
	galaxian_flip_screen_x_w(offset, data);
	galaxian_flip_screen_y_w(offset, data);
}


WRITE_HANDLER( scramble_background_enable_w )
{
	background_enable = data & 0x01;
}

WRITE_HANDLER( scramble_background_red_w )
{
	background_red = data & 0x01;
}

WRITE_HANDLER( scramble_background_green_w )
{
	background_green = data & 0x01;
}

WRITE_HANDLER( scramble_background_blue_w )
{
	background_blue = data & 0x01;
}


WRITE_HANDLER( galaxian_stars_enable_w )
{
	galaxian_stars_on = data & 0x01;

	if (!galaxian_stars_on)
	{
		stars_scrollpos = 0;
	}
}


WRITE_HANDLER( darkplnt_bullet_color_w )
{
	darkplnt_bullet_color = data & 0x01;
}



WRITE_HANDLER( galaxian_gfxbank_w )
{
	if (gfxbank[offset] != data)
	{
		gfxbank[offset] = data;

		tilemap_mark_all_tiles_dirty(tilemap);
	}
}

WRITE_HANDLER( rockclim_videoram_w )
{
	if (rockclim_videoram[offset] != data)
	{
		rockclim_videoram[offset] = data;
		tilemap_mark_tile_dirty(rockclim_tilemap, offset);
	}
}

static int rockclim_v=0;
static int rockclim_h=0;

WRITE_HANDLER( rockclim_scroll_w )
{


	switch(offset&3)
	{
		case 0: rockclim_h=(rockclim_h&0xff00)|data;tilemap_set_scrollx(rockclim_tilemap , 0, rockclim_h );break;
		case 1:	rockclim_h=(rockclim_h&0xff)|(data<<8);tilemap_set_scrollx(rockclim_tilemap , 0, rockclim_h );break;
		case 2:	rockclim_v=(rockclim_v&0xff00)|data;tilemap_set_scrolly(rockclim_tilemap , 0, rockclim_v );break;
		case 3:	rockclim_v=(rockclim_v&0xff)|(data<<8);tilemap_set_scrolly(rockclim_tilemap , 0, rockclim_v );break;
	}

}


READ_HANDLER( rockclim_videoram_r )
{
	return rockclim_videoram[offset];
}



/* character banking functions */

static void gmgalax_modify_charcode(UINT16 *code,UINT8 x)
{
	*code |= (gfxbank[0] << 9);
}

static void mooncrst_modify_charcode(UINT16 *code,UINT8 x)
{
	if (gfxbank[2] && ((*code & 0xc0) == 0x80))
	{
		*code = (*code & 0x3f) | (gfxbank[0] << 6) | (gfxbank[1] << 7) | 0x0100;
	}
}

static void mooncrgx_modify_charcode(UINT16 *code,UINT8 x)
{
	if (gfxbank[2] && ((*code & 0xc0) == 0x80))
	{
		*code = (*code & 0x3f) | (gfxbank[1] << 6) | (gfxbank[0] << 7) | 0x0100;
	}
}

static void moonqsr_modify_charcode(UINT16 *code,UINT8 x)
{
	*code |= ((galaxian_attributesram[(x << 1) | 1] & 0x20) << 3);
}

static void mshuttle_modify_charcode(UINT16 *code,UINT8 x)
{
	*code |= ((galaxian_attributesram[(x << 1) | 1] & 0x30) << 4);
}

static void pisces_modify_charcode(UINT16 *code,UINT8 x)
{
	*code |= (gfxbank[0] << 8);
}

static void mimonkey_modify_charcode(UINT16 *code,UINT8 x)
{
	*code |= (gfxbank[0] << 8) | (gfxbank[2] << 9);
}

static void batman2_modify_charcode(UINT16 *code,UINT8 x)
{
	if (*code & 0x80)
	{
		*code |= (gfxbank[0] << 8);
	}
}

static void mariner_modify_charcode(UINT16 *code,UINT8 x)
{
	UINT8 *prom;


	/* bit 0 of the PROM controls character banking */

	prom = memory_region(REGION_USER2);

	*code |= ((prom[x] & 0x01) << 8);
}

static void jumpbug_modify_charcode(UINT16 *code,UINT8 x)
{
	if (((*code & 0xc0) == 0x80) &&
		 (gfxbank[2] & 0x01))
	{
		*code += 128 + (( gfxbank[0] & 0x01) << 6) +
					   (( gfxbank[1] & 0x01) << 7) +
					   ((~gfxbank[4] & 0x01) << 8);
	}
}


/* sprite banking functions */

static void gmgalax_modify_spritecode(data8_t *spriteram,int *code,int *flipx,int *flipy,int offs)
{
	*code |= (gfxbank[0] << 7) | 0x40;
}

static void mooncrst_modify_spritecode(data8_t *spriteram,int *code,int *flipx,int *flipy,int offs)
{
	if (gfxbank[2] && ((*code & 0x30) == 0x20))
	{
		*code = (*code & 0x0f) | (gfxbank[0] << 4) | (gfxbank[1] << 5) | 0x40;
	}
}

static void mooncrgx_modify_spritecode(data8_t *spriteram,int *code,int *flipx,int *flipy,int offs)
{
	if (gfxbank[2] && ((*code & 0x30) == 0x20))
	{
		*code = (*code & 0x0f) | (gfxbank[1] << 4) | (gfxbank[0] << 5) | 0x40;
	}
}

static void moonqsr_modify_spritecode(data8_t *spriteram,int *code,int *flipx,int *flipy,int offs)
{
	*code |= ((spriteram[offs + 2] & 0x20) << 1);
}

static void mshuttle_modify_spritecode(data8_t *spriteram,int *code,int *flipx,int *flipy,int offs)
{
	*code |= ((spriteram[offs + 2] & 0x30) << 2);
}

static void calipso_modify_spritecode(data8_t *spriteram,int *code,int *flipx,int *flipy,int offs)
{
	/* No flips */
	*code = spriteram[offs + 1];
	*flipx = 0;
	*flipy = 0;
}

static void pisces_modify_spritecode(data8_t *spriteram,int *code,int *flipx,int *flipy,int offs)
{
	*code |= (gfxbank[0] << 6);
}

static void mimonkey_modify_spritecode(data8_t *spriteram,int *code,int *flipx,int *flipy,int offs)
{
	*code |= (gfxbank[0] << 6) | (gfxbank[2] << 7);
}

static void batman2_modify_spritecode(data8_t *spriteram,int *code,int *flipx,int *flipy,int offs)
{
	/* only the upper 64 sprites are used */
	*code |= 0x40;
}

static void jumpbug_modify_spritecode(data8_t *spriteram,int *code,int *flipx,int *flipy,int offs)
{
	if (((*code & 0x30) == 0x20) &&
		 (gfxbank[2] & 0x01) != 0)
	{
		*code += 32 + (( gfxbank[0] & 0x01) << 4) +
					  (( gfxbank[1] & 0x01) << 5) +
					  ((~gfxbank[4] & 0x01) << 6);
	}
}

static void dkongjrm_modify_spritecode(data8_t *spriteram,int *code,int *flipx,int *flipy,int offs)
{
	/* No x flip */
	*code = (spriteram[offs + 1] & 0x7f) | 0x80;
	*flipx = 0;
}


/* color PROM mapping functions */

static void frogger_modify_color(UINT8 *color)
{
	*color = ((*color >> 1) & 0x03) | ((*color << 2) & 0x04);
}

static void gmgalax_modify_color(UINT8 *color)
{
	*color |= (gfxbank[0] << 3);
}

static void drivfrcg_modify_color(UINT8 *color)
{
	*color = ((*color & 0x40) >> 3) | (*color & 7);
}


/* y position mapping functions */

static void frogger_modify_ypos(UINT8 *sy)
{
	*sy = (*sy << 4) | (*sy >> 4);
}


/* bullet drawing functions */

static void galaxian_draw_bullets(struct mame_bitmap *bitmap, int offs, int x, int y)
{
	int i;


	for (i = 0; i < 4; i++)
	{
		x--;

		if (x >= Machine->visible_area.min_x &&
			x <= Machine->visible_area.max_x)
		{
			int color;


			/* yellow missile, white shells (this is the terminology on the schematics) */
			color = ((offs == 7*4) ? BULLETS_COLOR_BASE : BULLETS_COLOR_BASE + 1);

			plot_pixel(bitmap, x, y, Machine->pens[color]);
		}
	}
}

static void gteikob2_draw_bullets(struct mame_bitmap *bitmap, int offs, int x, int y)
{
	galaxian_draw_bullets(bitmap, offs, 260 - x, y);
}

static void scramble_draw_bullets(struct mame_bitmap *bitmap, int offs, int x, int y)
{
	if (flip_screen_x)  x++;

	x = x - 6;

	if (x >= Machine->visible_area.min_x &&
		x <= Machine->visible_area.max_x)
	{
		/* yellow bullets */
		plot_pixel(bitmap, x, y, Machine->pens[BULLETS_COLOR_BASE]);
	}
}

static void darkplnt_draw_bullets(struct mame_bitmap *bitmap, int offs, int x, int y)
{
	if (flip_screen_x)  x++;

	x = x - 6;

	if (x >= Machine->visible_area.min_x &&
		x <= Machine->visible_area.max_x)
	{
		plot_pixel(bitmap, x, y, Machine->pens[32 + darkplnt_bullet_color]);
	}
}

static void theend_draw_bullets(struct mame_bitmap *bitmap, int offs, int x, int y)
{
	int i;


	/* same as Galaxian, but all bullets are yellow */
	for (i = 0; i < 4; i++)
	{
		x--;

		if (x >= Machine->visible_area.min_x &&
			x <= Machine->visible_area.max_x)
		{
			plot_pixel(bitmap, x, y, Machine->pens[BULLETS_COLOR_BASE]);
		}
	}
}


/* background drawing functions */

static void galaxian_draw_background(struct mame_bitmap *bitmap)
{
	/* plain black background */
	fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area);
}

static void scramble_draw_background(struct mame_bitmap *bitmap)
{
	if (background_enable)
	{
		fillbitmap(bitmap,Machine->pens[BACKGROUND_COLOR_BASE],&Machine->visible_area);
	}
	else
	{
		fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area);
	}
}

static void turtles_draw_background(struct mame_bitmap *bitmap)
{
	int color = (background_blue << 2) | (background_green << 1) | background_red;

	fillbitmap(bitmap,Machine->pens[BACKGROUND_COLOR_BASE + color],&Machine->visible_area);
}

static void frogger_draw_background(struct mame_bitmap *bitmap)
{
	/* color split point verified on real machine */
	if (flip_screen_x)
	{
		plot_box(bitmap,   0, 0, 128, 256, Machine->pens[0]);
		plot_box(bitmap, 128, 0, 128, 256, Machine->pens[BACKGROUND_COLOR_BASE]);
	}
	else
	{
		plot_box(bitmap,   0, 0, 128, 256, Machine->pens[BACKGROUND_COLOR_BASE]);
		plot_box(bitmap, 128, 0, 128, 256, Machine->pens[0]);
	}
}

static void stratgyx_draw_background(struct mame_bitmap *bitmap)
{
	UINT8 x;
	UINT8 *prom;


	/* the background PROM is connected the following way:

	   bit 0 = 0 enables the blue gun if BCB is asserted
	   bit 1 = 0 enables the red gun if BCR is asserted and
	             the green gun if BCG is asserted
	   bits 2-7 are unconnected */

	prom = memory_region(REGION_USER1);

	for (x = 0; x < 32; x++)
	{
		int sx,color;


		color = 0;

		if ((~prom[x] & 0x02) && background_red)   color |= 0x01;
		if ((~prom[x] & 0x02) && background_green) color |= 0x02;
		if ((~prom[x] & 0x01) && background_blue)  color |= 0x04;

		if (flip_screen_x)
		{
			sx = 8 * (31 - x);
		}
		else
		{
			sx = 8 * x;
		}

		plot_box(bitmap, sx, 0, 8, 256, Machine->pens[BACKGROUND_COLOR_BASE + color]);
	}
}

static void minefld_draw_background(struct mame_bitmap *bitmap)
{
	if (background_enable)
	{
		int x;


		for (x = 0; x < 128; x++)
		{
			plot_box(bitmap, x,       0, 1, 256, Machine->pens[BACKGROUND_COLOR_BASE + x]);
		}

		for (x = 0; x < 120; x++)
		{
			plot_box(bitmap, x + 128, 0, 1, 256, Machine->pens[BACKGROUND_COLOR_BASE + x + 128]);
		}

		plot_box(bitmap, 248, 0, 16, 256, Machine->pens[BACKGROUND_COLOR_BASE]);
	}
	else
	{
		fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area);
	}
}

static void rescue_draw_background(struct mame_bitmap *bitmap)
{
	if (background_enable)
	{
		int x;


		for (x = 0; x < 128; x++)
		{
			plot_box(bitmap, x,       0, 1, 256, Machine->pens[BACKGROUND_COLOR_BASE + x]);
		}

		for (x = 0; x < 120; x++)
		{
			plot_box(bitmap, x + 128, 0, 1, 256, Machine->pens[BACKGROUND_COLOR_BASE + x + 8]);
		}

		plot_box(bitmap, 248, 0, 16, 256, Machine->pens[BACKGROUND_COLOR_BASE]);
	}
	else
	{
		fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area);
	}
}

static void mariner_draw_background(struct mame_bitmap *bitmap)
{
	UINT8 x;
	UINT8 *prom;


	/* the background PROM contains the color codes for each 8 pixel
	   line (column) of the screen.  The first 0x20 bytes for unflipped,
	   and the 2nd 0x20 bytes for flipped screen. */

	prom = memory_region(REGION_USER1);

	if (flip_screen_x)
	{
		for (x = 0; x < 32; x++)
		{
			int color;


			if (x == 0)
				color = 0;
			else
				color = prom[0x20 + x - 1];

			plot_box(bitmap, 8 * (31 - x), 0, 8, 256, Machine->pens[BACKGROUND_COLOR_BASE + color]);
		}
	}
	else
	{
		for (x = 0; x < 32; x++)
		{
			int color;


			if (x == 31)
				color = 0;
			else
				color = prom[x + 1];

			plot_box(bitmap, 8 * x, 0, 8, 256, Machine->pens[BACKGROUND_COLOR_BASE + color]);
		}
	}
}


/* star drawing functions */

void galaxian_init_stars(int colors_offset)
{
	int i;
	int total_stars;
	UINT32 generator;
	int x,y;


	galaxian_stars_on = 0;
	stars_blink_state = 0;
	stars_blink_timer = timer_alloc(stars_blink_callback);
	stars_scroll_timer = timer_alloc(stars_scroll_callback);
	timer_adjusted = 0;
	stars_colors_start = colors_offset;


	for (i = 0;i < 64;i++)
	{
		int bits,r,g,b;
		int map[4] = { 0x00, 0x88, 0xcc, 0xff };


		bits = (i >> 0) & 0x03;
		r = map[bits];
		bits = (i >> 2) & 0x03;
		g = map[bits];
		bits = (i >> 4) & 0x03;
		b = map[bits];
		palette_set_color(colors_offset+i,r,g,b);
	}


	/* precalculate the star background */

	total_stars = 0;
	generator = 0;

	for (y = 0;y < 256;y++)
	{
		for (x = 0;x < 512;x++)
		{
			UINT32 bit0;


			bit0 = ((~generator >> 16) & 0x01) ^ ((generator >> 4) & 0x01);

			generator = (generator << 1) | bit0;

			if (((~generator >> 16) & 0x01) && (generator & 0xff) == 0xff)
			{
				int color;


				color = (~(generator >> 8)) & 0x3f;
				if (color)
				{
					stars[total_stars].x = x;
					stars[total_stars].y = y;
					stars[total_stars].color = color;

					total_stars++;
				}
			}
		}
	}

	if (total_stars != STAR_COUNT)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "total_stars = %d, STAR_COUNT = %d\n",total_stars,STAR_COUNT);
		exit(1);
	}
}

static void plot_star(struct mame_bitmap *bitmap, int x, int y, int color)
{
	if (y < Machine->visible_area.min_y ||
		y > Machine->visible_area.max_y ||
		x < Machine->visible_area.min_x ||
		x > Machine->visible_area.max_x)
		return;


	if (flip_screen_x)
	{
		x = 255 - x;
	}
	if (flip_screen_y)
	{
		y = 255 - y;
	}

	plot_pixel(bitmap, x, y, Machine->pens[stars_colors_start + color]);
}

static void noop_draw_stars(struct mame_bitmap *bitmap)
{
}

void galaxian_draw_stars(struct mame_bitmap *bitmap)
{
	int offs;


	if (!timer_adjusted)
	{
		start_stars_scroll_timer();
		timer_adjusted = 1;
	}


	for (offs = 0;offs < STAR_COUNT;offs++)
	{
		int x,y;


		x = ((stars[offs].x +   stars_scrollpos) & 0x01ff) >> 1;
		y = ( stars[offs].y + ((stars_scrollpos + stars[offs].x) >> 9)) & 0xff;

		if ((y & 0x01) ^ ((x >> 3) & 0x01))
		{
			plot_star(bitmap, x, y, stars[offs].color);
		}
	}
}

void scramble_draw_stars(struct mame_bitmap *bitmap)
{
	int offs;


	if (!timer_adjusted)
	{
		start_stars_blink_timer(100000, 10000, 0.00001);
		timer_adjusted = 1;
	}


	for (offs = 0;offs < STAR_COUNT;offs++)
	{
		int x,y;


		x = stars[offs].x >> 1;
		y = stars[offs].y;

		if ((y & 0x01) ^ ((x >> 3) & 0x01))
		{
			/* determine when to skip plotting */
			switch (stars_blink_state & 0x03)
			{
			case 0:
				if (!(stars[offs].color & 0x01))  continue;
				break;
			case 1:
				if (!(stars[offs].color & 0x04))  continue;
				break;
			case 2:
				if (!(stars[offs].y & 0x02))  continue;
				break;
			case 3:
				/* always plot */
				break;
			}

			plot_star(bitmap, x, y, stars[offs].color);
		}
	}
}

static void rescue_draw_stars(struct mame_bitmap *bitmap)
{
	int offs;


	/* same as Scramble, but only top (left) half of screen */

	if (!timer_adjusted)
	{
		start_stars_blink_timer(100000, 10000, 0.00001);
		timer_adjusted = 1;
	}


	for (offs = 0;offs < STAR_COUNT;offs++)
	{
		int x,y;


		x = stars[offs].x >> 1;
		y = stars[offs].y;

		if ((x < 128) && ((y & 0x01) ^ ((x >> 3) & 0x01)))
		{
			/* determine when to skip plotting */
			switch (stars_blink_state & 0x03)
			{
			case 0:
				if (!(stars[offs].color & 0x01))  continue;
				break;
			case 1:
				if (!(stars[offs].color & 0x04))  continue;
				break;
			case 2:
				if (!(stars[offs].y & 0x02))  continue;
				break;
			case 3:
				/* always plot */
				break;
			}

			plot_star(bitmap, x, y, stars[offs].color);
		}
	}
}

static void mariner_draw_stars(struct mame_bitmap *bitmap)
{
	int offs;
	UINT8 *prom;


	if (!timer_adjusted)
	{
		start_stars_scroll_timer();
		timer_adjusted = 1;
	}


	/* bit 2 of the PROM controls star visibility */

	prom = memory_region(REGION_USER2);

	for (offs = 0;offs < STAR_COUNT;offs++)
	{
		int x,y;


		x = ((stars[offs].x +   -stars_scrollpos) & 0x01ff) >> 1;
		y = ( stars[offs].y + ((-stars_scrollpos + stars[offs].x) >> 9)) & 0xff;

		if ((y & 0x01) ^ ((x >> 3) & 0x01))
		{
			if (prom[(x/8 + 1) & 0x1f] & 0x04)
			{
				plot_star(bitmap, x, y, stars[offs].color);
			}
		}
	}
}

static void jumpbug_draw_stars(struct mame_bitmap *bitmap)
{
	int offs;


	if (!timer_adjusted)
	{
		start_stars_blink_timer(100000, 10000, 0.00001);
		start_stars_scroll_timer();
		timer_adjusted = 1;
	}


	for (offs = 0;offs < STAR_COUNT;offs++)
	{
		int x,y;


		x = stars[offs].x >> 1;
		y = stars[offs].y;

		/* determine when to skip plotting */
		if ((y & 0x01) ^ ((x >> 3) & 0x01))
		{
			switch (stars_blink_state & 0x03)
			{
			case 0:
				if (!(stars[offs].color & 0x01))  continue;
				break;
			case 1:
				if (!(stars[offs].color & 0x04))  continue;
				break;
			case 2:
				if (!(stars[offs].y & 0x02))  continue;
				break;
			case 3:
				/* always plot */
				break;
			}

			x = ((stars[offs].x +   stars_scrollpos) & 0x01ff) >> 1;
			y = ( stars[offs].y + ((stars_scrollpos + stars[offs].x) >> 9)) & 0xff;

			/* no stars in the status area */
			if (x >= 240)  continue;

			plot_star(bitmap, x, y, stars[offs].color);
		}
	}
}


static void stars_blink_callback(int param)
{
	stars_blink_state++;
}

static void start_stars_blink_timer(double ra, double rb, double c)
{
	/* calculate the period using the formula given in the 555 datasheet */

	double period = 0.693 * (ra + 2.0 * rb) * c;

	timer_adjust(stars_blink_timer, TIME_IN_SEC(period), 0, TIME_IN_SEC(period));
}


static void stars_scroll_callback(int param)
{
	if (galaxian_stars_on)
	{
		stars_scrollpos++;
	}
}

static void start_stars_scroll_timer()
{
	timer_adjust(stars_scroll_timer, TIME_IN_HZ(Machine->drv->frames_per_second), 0, TIME_IN_HZ(Machine->drv->frames_per_second));
}


/***************************************************************************

  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/

static void get_tile_info(int tile_index)
{
	UINT8 x = tile_index & 0x1f;

	UINT16 code = galaxian_videoram[tile_index];
	UINT8 color = galaxian_attributesram[(x << 1) | 1] & color_mask;

	if (modify_charcode)
	{
		modify_charcode(&code, x);
	}

	if (modify_color)
	{
		modify_color(&color);
	}

	SET_TILE_INFO(0, code, color, 0)
}

static void rockclim_get_tile_info(int tile_index)
{
	UINT16 code = rockclim_videoram[tile_index];
	SET_TILE_INFO(2, code, 0, 0)
}

static void draw_bullets_common(struct mame_bitmap *bitmap)
{
	int offs;


	for (offs = 0;offs < galaxian_bulletsram_size;offs += 4)
	{
		UINT8 sx,sy;

		sy = 255 - galaxian_bulletsram[offs + 1];
		sx = 255 - galaxian_bulletsram[offs + 3];

		if (sy < Machine->visible_area.min_y ||
			sy > Machine->visible_area.max_y)
			continue;

		if (flip_screen_y)  sy = 255 - sy;

		draw_bullets(bitmap, offs, sx, sy);
	}
}


static void draw_sprites(struct mame_bitmap *bitmap, data8_t *spriteram, size_t spriteram_size)
{
	int offs;


	for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
	{
		UINT8 sx,sy,color;
		int flipx,flipy,code;


		sx = spriteram[offs + 3] + 1;	/* the existence of +1 is supported by a LOT of games */
		sy = spriteram[offs];			/* Anteater, Mariner, for example */
		flipx = spriteram[offs + 1] & 0x40;
		flipy = spriteram[offs + 1] & 0x80;
		code = spriteram[offs + 1] & 0x3f;
		color = spriteram[offs + 2] & color_mask;

		if (modify_spritecode)
		{
			modify_spritecode(spriteram, &code, &flipx, &flipy, offs);
		}

		if (modify_color)
		{
			modify_color(&color);
		}

		if (modify_ypos)
		{
			modify_ypos(&sy);
		}

		if (flip_screen_x)
		{
			sx = 240 - sx;
			flipx = !flipx;
		}

		if (flip_screen_y)
		{
			flipy = !flipy;
		}
		else
		{
			sy = 240 - sy;
		}


		/* In at least Amidar Turtles, sprites #0, #1 and #2 need to be moved */
		/* down (left) one pixel to be positioned correctly. */
		/* Note that the adjustment must be done AFTER handling flipscreen, thus */
		/* proving that this is a hardware related "feature" */

		if (offs < 3*4)  sy++;


		drawgfx(bitmap,Machine->gfx[1],
				code,color,
				flipx,flipy,
				sx,sy,
				flip_screen_x ? spritevisibleareaflipx : spritevisiblearea,TRANSPARENCY_PEN,0);
	}
}


VIDEO_UPDATE( galaxian )
{
	draw_background(bitmap);


	if (galaxian_stars_on)
	{
		draw_stars(bitmap);
	}


	tilemap_draw(bitmap, 0, tilemap, 0, 0);


	if (draw_bullets)
	{
		draw_bullets_common(bitmap);
	}


	draw_sprites(bitmap, galaxian_spriteram, galaxian_spriteram_size);

	if (spriteram2_present)
	{
		draw_sprites(bitmap, galaxian_spriteram2, galaxian_spriteram2_size);
	}
}
