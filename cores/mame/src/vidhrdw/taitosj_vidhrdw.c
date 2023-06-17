/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

#ifndef MIN
#define MIN(x,y) (x)<(y)?(x):(y)
#endif


unsigned char *taitosj_videoram2,*taitosj_videoram3;
unsigned char *taitosj_characterram;
unsigned char *taitosj_scroll;
unsigned char *taitosj_colscrolly;
unsigned char *taitosj_gfxpointer;
unsigned char *taitosj_colorbank,*taitosj_video_priority;
unsigned char *kikstart_scrollram;
unsigned char *taitosj_spritebank;
static unsigned char taitosj_collision_reg[4];
static unsigned char *dirtybuffer2,*dirtybuffer3;
static struct mame_bitmap *taitosj_tmpbitmap[3];
static struct mame_bitmap *sprite_sprite_collbitmap1,*sprite_sprite_collbitmap2;
static struct mame_bitmap *sprite_plane_collbitmap1;
static struct mame_bitmap *sprite_plane_collbitmap2[3];
static unsigned char dirtycharacter1[256],dirtycharacter2[256];
static unsigned char dirtysprite1[64],dirtysprite2[64];
static int taitosj_video_enable;
static int flipscreen[2];
static struct rectangle spritearea[32]; /*areas on bitmap (sprite locations)*/
static int spriteon[32]; /* 1 if sprite is active */

static int playfield_enable_mask[3] = { 0x10, 0x20, 0x40 };


/***************************************************************************

  I call the three planes with the conventional names "front", "middle" and
  "back", because that's their default order, but they can be arranged,
  together with the sprites, in any order. The priority is selected by
  register 0xd300, which works as follow:

  bits 0-3 go to A4-A7 of a 256x4 PROM
  bit 4 selects D0/D1 or D2/D3 of the PROM
  bit 5-7 n.c.
  A0-A3 of the PROM is fed with a mask of the inactive planes
  (i.e. all-zero) in the order sprites-front-middle-back
  the 2-bit code which comes out from the PROM selects the plane
  to display.

  Here is a dump of one of these PROMs; on the right is the resulting order
  (s = sprites f = front m = middle b = back). Note that, in theory, the
  PROM could encode some really funky priority schemes which couldn't be
  reconducted to the simple layer order given here. Luckily, none of the
  games seem to do that. Actually, all of them seem to use the same PROM,
  with the exception of Wild Western.

                                                        d300 pri    d300 pri
  00: 08 09 08 0A 00 05 00 0F 08 09 08 0A 00 05 00 0F |  00  sfmb    10  msfb
  10: 08 09 08 0B 00 0D 00 0F 08 09 08 0A 00 05 00 0F |  01  sfbm    11  msbf
  20: 08 0A 08 0A 04 05 00 0F 08 0A 08 0A 04 05 00 0F |  02  smfb    12  mfsb
  30: 08 0A 08 0A 04 07 0C 0F 08 0A 08 0A 04 05 00 0F |  03  smbf    13  mfbs
  40: 08 0B 08 0B 0C 0F 0C 0F 08 09 08 0A 00 05 00 0F |  04  sbfm    14  mbsf
  50: 08 0B 08 0B 0C 0F 0C 0F 08 0A 08 0A 04 05 00 0F |  05  sbmf    15  mbfs
  60: 0D 0D 0C 0E 0D 0D 0C 0F 01 05 00 0A 01 05 00 0F |  06  fsmb    16  bsfm
  70: 0D 0D 0C 0F 0D 0D 0C 0F 01 09 00 0A 01 05 00 0F |  07  fsbm    17  bsmf
  80: 0D 0D 0E 0E 0D 0D 0C 0F 05 05 02 0A 05 05 00 0F |  08  fmsb    18  bfsm
  90: 0D 0D 0E 0E 0D 0D 0F 0F 05 05 0A 0A 05 05 00 0F |  09  fmbs    19  bfms
  A0: 0D 0D 0F 0F 0D 0D 0F 0F 09 09 08 0A 01 05 00 0F |  0A  fbsm    1A  bmsf
  B0: 0D 0D 0F 0F 0D 0D 0F 0F 09 09 0A 0A 05 05 00 0F |  0B  fbms    1B  bmfs
  C0: 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F |  0C   -      1C   -
  D0: 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F |  0D   -      1D   -
  E0: 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F |  0E   -      1E   -
  F0: 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F |  0F   -      1F   -

***************************************************************************/

static int draworder[32][4];


/***************************************************************************

  Convert the color PROMs into a more useable format.

  The Taito games don't have a color PROM. They use RAM to dynamically
  create the palette. The resolution is 9 bit (3 bits per gun).

  The RAM is connected to the RGB output this way:

  bit 0 -- inverter -- 220 ohm resistor  -- RED
  bit 7 -- inverter -- 470 ohm resistor  -- RED
        -- inverter -- 1  kohm resistor  -- RED
        -- inverter -- 220 ohm resistor  -- GREEN
        -- inverter -- 470 ohm resistor  -- GREEN
        -- inverter -- 1  kohm resistor  -- GREEN
        -- inverter -- 220 ohm resistor  -- BLUE
        -- inverter -- 470 ohm resistor  -- BLUE
  bit 0 -- inverter -- 1  kohm resistor  -- BLUE

***************************************************************************/

PALETTE_INIT( taitosj )
{
	int i;
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])

	/* all gfx elements use the same palette */
	for (i = 0;i < 64;i++)
	{
		COLOR(0,i) = i;
		/* we create both a "normal" lookup table and one where pen 0 is */
		/* always mapped to color 0. This is needed for transparency. */
		if (i % 8 == 0) COLOR(0,i + 64) = 0;
		else COLOR(0,i + 64) = i;
	}


	/* do a simple conversion of the PROM into layer priority order. Note that */
	/* this is a simplification, which assumes the PROM encodes a sensible priority */
	/* scheme. */
	color_prom = memory_region(REGION_PROMS);
	for (i = 0;i < 32;i++)
	{
		int j,mask;

		mask = 0;	/* start with all four layers active, so we'll get the highest */
					/* priority one in the first loop */

		for (j = 3;j >= 0;j--)
		{
			int data;

			data = color_prom[0x10 * (i & 0x0f) + mask];
			if (i & 0x10) data >>= 2;
			data &= 0x03;
			mask |= (1 << data);	/* in next loop, we'll see which of the remaining */
									/* layers has top priority when this one is transparent */
			draworder[i][j] = data;
		}
	}
}



WRITE_HANDLER( taitosj_paletteram_w )
{
	int bit0,bit1,bit2;
	int r,g,b,val;


	paletteram[offset] = data;

	/* red component */
	val = paletteram[offset | 1];
	bit0 = (~val >> 6) & 0x01;
	bit1 = (~val >> 7) & 0x01;
	val = paletteram[offset & ~1];
	bit2 = (~val >> 0) & 0x01;
	r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

	/* green component */
	val = paletteram[offset | 1];
	bit0 = (~val >> 3) & 0x01;
	bit1 = (~val >> 4) & 0x01;
	bit2 = (~val >> 5) & 0x01;
	g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

	/* blue component */
	val = paletteram[offset | 1];
	bit0 = (~val >> 0) & 0x01;
	bit1 = (~val >> 1) & 0x01;
	bit2 = (~val >> 2) & 0x01;
	b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

	palette_set_color(offset / 2,r,g,b);
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
VIDEO_START( taitosj )
{
	int i;


	taitosj_tmpbitmap[0] = taitosj_tmpbitmap[1] = taitosj_tmpbitmap[2] = 0;
	sprite_sprite_collbitmap2 = sprite_sprite_collbitmap1 = 0;
	sprite_plane_collbitmap1 = 0;
	sprite_plane_collbitmap2[0] = sprite_plane_collbitmap2[1] = sprite_plane_collbitmap2[2] = 0;
	dirtybuffer3  = dirtybuffer2 = 0;


	if (video_start_generic() != 0)
		return 1;

	if ((dirtybuffer2 = auto_malloc(videoram_size)) == 0)
		return 1;
	memset(dirtybuffer2,1,videoram_size);

	if ((dirtybuffer3 = auto_malloc(videoram_size)) == 0)
		return 1;
	memset(dirtybuffer3,1,videoram_size);

	if ((sprite_plane_collbitmap1 = auto_bitmap_alloc(16,16)) == 0)
		return 1;

	for (i = 0; i < 3; i++)
	{
		if ((taitosj_tmpbitmap[i] = auto_bitmap_alloc(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
			return 1;

		if ((sprite_plane_collbitmap2[i] = auto_bitmap_alloc(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
			return 1;
	}

	if ((sprite_sprite_collbitmap1 = auto_bitmap_alloc(32,32)) == 0)
		return 1;

	if ((sprite_sprite_collbitmap2 = auto_bitmap_alloc(32,32)) == 0)
		return 1;

	flipscreen[0] = flipscreen[1] = 0;

	taitosj_spritebank = spriteram;

	return 0;
}



READ_HANDLER( taitosj_gfxrom_r )
{
	int offs;


	offs = taitosj_gfxpointer[0]+taitosj_gfxpointer[1]*256;

	taitosj_gfxpointer[0]++;
	if (taitosj_gfxpointer[0] == 0) taitosj_gfxpointer[1]++;

	if (offs < 0x8000)
		return memory_region(REGION_GFX1)[offs];
	else return 0;
}


WRITE_HANDLER( taitosj_videoram2_w )
{
	if (taitosj_videoram2[offset] != data)
	{
		dirtybuffer2[offset] = 1;

		taitosj_videoram2[offset] = data;
	}
}



WRITE_HANDLER( taitosj_videoram3_w )
{
	if (taitosj_videoram3[offset] != data)
	{
		dirtybuffer3[offset] = 1;

		taitosj_videoram3[offset] = data;
	}
}



WRITE_HANDLER( taitosj_colorbank_w )
{
	if (taitosj_colorbank[offset] != data)
	{
log_cb(RETRO_LOG_DEBUG, LOGPRE "colorbank %d = %02x\n",offset,data);
		memset(dirtybuffer,1,videoram_size);
		memset(dirtybuffer2,1,videoram_size);
		memset(dirtybuffer3,1,videoram_size);

		taitosj_colorbank[offset] = data;
	}
}



WRITE_HANDLER( taitosj_videoenable_w )
{
	if (taitosj_video_enable != data)
	{
log_cb(RETRO_LOG_DEBUG, LOGPRE "videoenable = %02x\n",data);

		if ((taitosj_video_enable & 3) != (data & 3))
		{
			flipscreen[0] = data & 1;
			flipscreen[1] = data & 2;

			memset(dirtybuffer,1,videoram_size);
			memset(dirtybuffer2,1,videoram_size);
			memset(dirtybuffer3,1,videoram_size);
		}

		/* OBJEX sprite bank select */
		if( data & 4 )
			taitosj_spritebank = spriteram_2;
		else
			taitosj_spritebank = spriteram;

		if ((taitosj_video_enable & 4) != (data & 4))
			log_cb(RETRO_LOG_DEBUG, LOGPRE "sprite bank[%d]\n", (data & 4) >> 2 );

		taitosj_video_enable = data;
	}
}



WRITE_HANDLER( taitosj_characterram_w )
{
	if (taitosj_characterram[offset] != data)
	{
		if (offset < 0x1800)
		{
			dirtycharacter1[(offset / 8) & 0xff] = 1;
			dirtysprite1[(offset / 32) & 0x3f] = 1;
		}
		else
		{
			dirtycharacter2[(offset / 8) & 0xff] = 1;
			dirtysprite2[(offset / 32) & 0x3f] = 1;
		}

		taitosj_characterram[offset] = data;
	}
}

WRITE_HANDLER( junglhbr_characterram_w )
{
	taitosj_characterram_w(offset, data ^ 0xfc);
}

/***************************************************************************

  As if the hardware weren't complicated enough, it also has built-in
  collision detection.

***************************************************************************/
READ_HANDLER( taitosj_collision_reg_r )
{
	return taitosj_collision_reg[offset];
}

WRITE_HANDLER( taitosj_collision_reg_clear_w )
{
	taitosj_collision_reg[0] = 0;
	taitosj_collision_reg[1] = 0;
	taitosj_collision_reg[2] = 0;
	taitosj_collision_reg[3] = 0;
}

static INLINE int get_sprite_xy(UINT8 num, UINT8* sx, UINT8* sy)
{
	int offs = num * 4;


	*sx = taitosj_spritebank[offs] - 1;
	*sy = 240 - taitosj_spritebank[offs + 1];
	return (*sy < 240);
}


static int check_sprite_sprite_bitpattern(int sx1, int sy1, int num1,
                                          int sx2, int sy2, int num2)
{
	int x,y,minx,miny,maxx = 16,maxy = 16;
	int offs1 = num1 * 4;
	int offs2 = num2 * 4;

	/* normalize coordinates to (0,0) and compute overlap */
	if (sx1 < sx2)
	{
		sx2 -= sx1;
		sx1 = 0;
		minx = sx2;
	}
	else
	{
		sx1 -= sx2;
		sx2 = 0;
		minx = sx1;
	}

	if (sy1 < sy2)
	{
		sy2 -= sy1;
		sy1 = 0;
		miny = sy2;
	}
	else
	{
		sy1 -= sy2;
		sy2 = 0;
		miny = sy1;
	}

	/* draw the sprites into seperate bitmaps and check overlapping region */
	drawgfx(sprite_sprite_collbitmap1,Machine->gfx[(taitosj_spritebank[offs1 + 3] & 0x40) ? 3 : 1],
			taitosj_spritebank[offs1 + 3] & 0x3f,
			0,
			taitosj_spritebank[offs1 + 2] & 1, taitosj_spritebank[offs1 + 2] & 2,
			sx1,sy1,
			0,TRANSPARENCY_NONE,0);

	drawgfx(sprite_sprite_collbitmap2,Machine->gfx[(taitosj_spritebank[offs2 + 3] & 0x40) ? 3 : 1],
			taitosj_spritebank[offs2 + 3] & 0x3f,
			0,
			taitosj_spritebank[offs2 + 2] & 1, taitosj_spritebank[offs2 + 2] & 2,
			sx2,sy2,
			0,TRANSPARENCY_NONE,0);

	for (y = miny;y < maxy;y++)
	{
		for(x = minx;x < maxx;x++)
		{
			if ((read_pixel(sprite_sprite_collbitmap1, x, y) != Machine->pens[0]) &&
			    (read_pixel(sprite_sprite_collbitmap2, x, y) != Machine->pens[0]))
			{
				return 1;  /* collided */
			}
		}
	}

	return 0;
}


static void check_sprite_sprite_collision(void)
{
	UINT8 i,j,sx1,sx2,sy1,sy2;


	if (taitosj_video_enable & 0x80)	/* check OBJOFF */
	{
		/* chech each pair of sprites */
		for (i = 0x00; i < 0x20; i++)
		{
			if ((i >= 0x10) && (i <= 0x17)) continue;	/* no sprites here */

			if (get_sprite_xy(i, &sx1, &sy1))
			{
				for (j = i+1; j < 0x20; j++)
				{
					if ((j >= 0x10) && (j <= 0x17)) continue;	  /* no sprites here */

					if (get_sprite_xy(j, &sx2, &sy2))
					{
						/* rule out any pairs that cannot be touching */
						if ((abs((INT8)sx1 - (INT8)sx2) < 16) &&
							(abs((INT8)sy1 - (INT8)sy2) < 16))
						{
							if (check_sprite_sprite_bitpattern(sx1, sy1, i, sx2, sy2, j))
							{
								/* mark sprite as collided */
								/* note that only the sprite with the higher number is marked */
								/* as collided. This is how the hardware works and required */
								/* by Pirate Pete to be able to finish the last round. */
								int reg;

								/* Here I am mimicking what I do in the sprite drawing code. */
								/* The last sprite has to be moved at the start of the list. */
								if (j == 0x1f)
								{
									reg = i >> 3;
									if (reg == 3)  reg = 2;

									taitosj_collision_reg[reg] |= (1 << (i & 0x07));
								}
								else
								{
									reg = j >> 3;
									if (reg == 3)  reg = 2;

									taitosj_collision_reg[reg] |= (1 << (j & 0x07));
								}
							}
						}
					}
				}
			}
		}
	}
}


static void calculate_sprites_areas(void)
{
	UINT8 sx,sy;
	int i,minx,miny,maxx,maxy;

	for (i = 0x00; i < 0x20; i++)
	{
		if ((i >= 0x10) && (i <= 0x17)) continue;	/* no sprites here */

		if (get_sprite_xy(i, &sx, &sy))
		{
			minx = sx;
			miny = sy;

			maxx = minx+15;
			maxy = miny+15;

			/* check for bitmap bounds to avoid illegal memory access */
			if (minx < 0) minx = 0;
			if (miny < 0) miny = 0;
			if (maxx >= Machine->drv->screen_width - 1)
				maxx = Machine->drv->screen_width - 1;
			if (maxy >= Machine->drv->screen_height - 1)
				maxy = Machine->drv->screen_height - 1;

			spritearea[i].min_x = minx;
			spritearea[i].max_x = maxx;
			spritearea[i].min_y = miny;
			spritearea[i].max_y = maxy;
			spriteon[i] = 1;
		}
		else /* sprite is off */
		{
			spriteon[i] = 0;
		}
	}
}

static int check_sprite_plane_bitpattern(int num)
{
	int x,y,flipx,flipy,minx,miny,maxx,maxy;
	int offs = num * 4;
	int result = 0;  /* no collisions */

	int	check_playfield1 = taitosj_video_enable & playfield_enable_mask[0];
	int	check_playfield2 = taitosj_video_enable & playfield_enable_mask[1];
	int	check_playfield3 = taitosj_video_enable & playfield_enable_mask[2];

	minx = spritearea[num].min_x;
	miny = spritearea[num].min_y;
	maxx = spritearea[num].max_x + 1;
	maxy = spritearea[num].max_y + 1;


	flipx = taitosj_spritebank[offs + 2] & 1;
	flipy = taitosj_spritebank[offs + 2] & 2;

	if (flipscreen[0])
	{
		flipx = !flipx;
		minx = MIN(minx + 2, Machine->drv->screen_width);
		maxx = MIN(maxx + 2, Machine->drv->screen_width);
	}
	if (flipscreen[1])
	{
		flipy = !flipy;
		miny = MIN(miny + 2, Machine->drv->screen_height);
		maxy = MIN(maxy + 2, Machine->drv->screen_height);
	}

	/* draw sprite into a bitmap and check if playfields collide */
	drawgfx(sprite_plane_collbitmap1, Machine->gfx[(taitosj_spritebank[offs + 3] & 0x40) ? 3 : 1],
			taitosj_spritebank[offs + 3] & 0x3f,
			0,
			flipx, flipy,
			0,0,
			0,TRANSPARENCY_NONE,0);

	for (y = miny;y < maxy;y++)
	{
		for (x = minx;x < maxx;x++)
		{
			if (read_pixel(sprite_plane_collbitmap1, x-minx, y-miny) != Machine->pens[0]) /* is there anything to check for ? */
			{
				if (check_playfield1 && (read_pixel(sprite_plane_collbitmap2[0], x, y) != Machine->pens[0]))
				{
					result |= 1;  /* collided */
					if (result == 7)  goto done;
					check_playfield1 = 0;
				}
				if (check_playfield2 && (read_pixel(sprite_plane_collbitmap2[1], x, y) != Machine->pens[0]))
				{
					result |= 2;  /* collided */
					if (result == 7)  goto done;
					check_playfield2 = 0;

				}
				if (check_playfield3 && (read_pixel(sprite_plane_collbitmap2[2], x, y) != Machine->pens[0]))
				{
					result |= 4;  /* collided */
					if (result == 7)  goto done;
					check_playfield3 = 0;
				}
			}
		}
	}

done:
	return result;
}

static void check_sprite_plane_collision(void)
{
	UINT8 i;


	if (taitosj_video_enable & 0x80)	/* check OBJOFF */
	{
		/* check each sprite */
		for (i = 0x00; i < 0x20; i++)
		{
			if ((i >= 0x10) && (i <= 0x17)) continue;	/* no sprites here */

			if (spriteon[i])
			{
				taitosj_collision_reg[3] |= check_sprite_plane_bitpattern(i);
			}
		}
	}
}


static void drawsprites(struct mame_bitmap *bitmap)
{
	/* sprite visibility area is missing 4 pixels from the sides, surely to reduce
       wraparound side effects. This was verified on a real Elevator Action.
       Note that the clipping is asymmetrical. This matches the real thing.
       I'm not sure of what should happen when the screen is flipped, though.
     */
	static struct rectangle spritevisiblearea =
	{
		0*8+3, 32*8-1-1,
		2*8, 30*8-1
	};
	static struct rectangle spritevisibleareaflip =
	{
		0*8+1, 32*8-3-1,
		2*8, 30*8-1
	};

	if (taitosj_video_enable & 0x80)	/* check OBJOFF */
	{
		int sprite;


		/* drawing order is a bit strange. The last sprite has to be moved at the start of the list. */
		for (sprite = 0x1f;sprite >= 0;sprite--)
		{
			UINT8 sx,sy,flipx,flipy;
			int offs = ((sprite - 1) & 0x1f) * 4;	/* move last sprite at the head of the list */


			if ((offs >= 0x40) && (offs <= 0x5f))  continue;	/* no sprites here */


			if (get_sprite_xy(offs / 4, &sx, &sy))
			{
				flipx = taitosj_spritebank[offs + 2] & 1;
				flipy = taitosj_spritebank[offs + 2] & 2;
				if (flipscreen[0])
				{
					sx = 238 - sx;
					flipx = !flipx;
				}
				if (flipscreen[1])
				{
					sy = 242 - sy;
					flipy = !flipy;
				}

				drawgfx(bitmap,Machine->gfx[(taitosj_spritebank[offs + 3] & 0x40) ? 3 : 1],
						taitosj_spritebank[offs + 3] & 0x3f,
						2 * ((taitosj_colorbank[1] >> 4) & 0x03) + ((taitosj_spritebank[offs + 2] >> 2) & 1),
						flipx,flipy,
						sx,sy,
						flipscreen[0] ? &spritevisibleareaflip : &spritevisiblearea,TRANSPARENCY_PEN,0);

				/* draw with wrap around. The horizontal games (eg. sfposeid) need this */
				drawgfx(bitmap,Machine->gfx[(taitosj_spritebank[offs + 3] & 0x40) ? 3 : 1],
						taitosj_spritebank[offs + 3] & 0x3f,
						2 * ((taitosj_colorbank[1] >> 4) & 0x03) + ((taitosj_spritebank[offs + 2] >> 2) & 1),
						flipx,flipy,
						sx - 0x100,sy,
						flipscreen[0] ? &spritevisibleareaflip : &spritevisiblearea,TRANSPARENCY_PEN,0);
			}
		}
	}
}


static void drawplayfield(int n, struct mame_bitmap *bitmap)
{
	static int fudge1[3] = { 3,  1, -1 };
	static int fudge2[3] = { 8, 10, 12 };

	if (taitosj_video_enable & playfield_enable_mask[n])
	{
		int i,scrollx,scrolly[32];


		scrollx = taitosj_scroll[2*n];
		if (flipscreen[0])
			scrollx =  (scrollx & 0xf8) + ((scrollx + fudge1[n]) & 7) + fudge2[n];
		else
			scrollx = -(scrollx & 0xf8) + ((scrollx + fudge1[n]) & 7) + fudge2[n];

		if (flipscreen[1])
		{
			for (i = 0;i < 32;i++)
				scrolly[31-i] =  taitosj_colscrolly[32*n+i] + taitosj_scroll[2*n+1];
		}
		else
		{
			for (i = 0;i < 32;i++)
				scrolly[i]    = -taitosj_colscrolly[32*n+i] - taitosj_scroll[2*n+1];
		}

		copyscrollbitmap(bitmap,taitosj_tmpbitmap[n],1,&scrollx,32,scrolly,&Machine->visible_area,TRANSPARENCY_COLOR,0);

		/* store parts covered with sprites for sprites/playfields collision detection */
		for (i=0x00; i<0x20; i++)
		{
			if ((i >= 0x10) && (i <= 0x17)) continue; /* no sprites here */
			if (spriteon[i])
				copyscrollbitmap( sprite_plane_collbitmap2[n],taitosj_tmpbitmap[n],1,&scrollx,32,scrolly,&spritearea[i],TRANSPARENCY_NONE,0);
		}
	}
}

static void kikstart_drawplayfield(int n, struct mame_bitmap *bitmap)
{
	if (taitosj_video_enable & playfield_enable_mask[n])
	{
		int i,scrolly,scrollx[32*8];

		for (i = 1;i < 32*8;i++) /* 1-255 ! */
		{
			if(flipscreen[1])
			{
				switch(n)
				{
					case 0:	scrollx[32*8-i] = 0 ;break;
					case 1:	scrollx[32*8-i] = kikstart_scrollram[i]+((taitosj_scroll[2*n]+0xa)&0xff);break;
					case 2:	scrollx[32*8-i] = kikstart_scrollram[0x100+i]+((taitosj_scroll[2*n]+0xc)&0xff);break;
				}
			}
			else
			{
				switch(n)
				{
					case 0:	scrollx[i] = 0 ;break;
					case 1:	scrollx[i] = 0xff-kikstart_scrollram[i-1]-((taitosj_scroll[2*n]-0x10)&0xff);break;
					case 2:	scrollx[i] = 0xff-kikstart_scrollram[0x100+i-1]-((taitosj_scroll[2*n]-0x12)&0xff);break;
				}
			}
		}
		scrolly=taitosj_scroll[2*n+1]; /*always 0 ? */
		copyscrollbitmap(bitmap,taitosj_tmpbitmap[n],32*8,scrollx,1,&scrolly,&Machine->visible_area,TRANSPARENCY_COLOR,0);
		/* store parts covered with sprites for sprites/playfields collision detection */
		for (i=0x00; i<0x20; i++)
		{
			if ((i >= 0x10) && (i <= 0x17)) continue; /* no sprites here */
			if (spriteon[i])
				copyscrollbitmap( sprite_plane_collbitmap2[n],taitosj_tmpbitmap[n],32*8,scrollx,1,&scrolly,&spritearea[i],TRANSPARENCY_NONE,0);
		}
	}
}


static void drawplane(int n,struct mame_bitmap *bitmap)
{
	switch (n)
	{
	case 0:
		drawsprites(bitmap);
		break;
	case 1:
	case 2:
	case 3:
		if(!strcmp(Machine->gamedrv->name,"kikstart"))
			kikstart_drawplayfield(n-1,bitmap); /* line scroll */
		else
			drawplayfield(n-1,bitmap); /* tile scroll */
		break;
	}
}


VIDEO_UPDATE( taitosj )
{
	int offs,i,alldirty = 0;


	/* decode modified characters */
	for (offs = 0;offs < 256;offs++)
	{
		if (dirtycharacter1[offs] == 1)
		{
			decodechar(Machine->gfx[0],offs,taitosj_characterram,Machine->drv->gfxdecodeinfo[0].gfxlayout);
			dirtycharacter1[offs] = 0;
			alldirty = 1;
		}
		if (dirtycharacter2[offs] == 1)
		{
			decodechar(Machine->gfx[2],offs,taitosj_characterram + 0x1800,Machine->drv->gfxdecodeinfo[2].gfxlayout);
			dirtycharacter2[offs] = 0;
			alldirty = 1;
		}
	}

	/* if characters changed, redraw everything */
	if (alldirty)
	{
		memset(dirtybuffer, 1,videoram_size);
		memset(dirtybuffer2,1,videoram_size);
		memset(dirtybuffer3,1,videoram_size);
	}

	/* decode modified sprites */
	for (offs = 0;offs < 64;offs++)
	{
		if (dirtysprite1[offs] == 1)
		{
			decodechar(Machine->gfx[1],offs,taitosj_characterram,Machine->drv->gfxdecodeinfo[1].gfxlayout);
			dirtysprite1[offs] = 0;
		}
		if (dirtysprite2[offs] == 1)
		{
			decodechar(Machine->gfx[3],offs,taitosj_characterram + 0x1800,Machine->drv->gfxdecodeinfo[3].gfxlayout);
			dirtysprite2[offs] = 0;
		}
	}


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy;


			dirtybuffer[offs] = 0;

			sx = offs % 32;
			sy = offs / 32;
			if (flipscreen[0]) sx = 31 - sx;
			if (flipscreen[1]) sy = 31 - sy;

			drawgfx(taitosj_tmpbitmap[0],Machine->gfx[taitosj_colorbank[0] & 0x08 ? 2 : 0],
					videoram[offs],
					(taitosj_colorbank[0] & 0x07) + 8,	/* use transparent pen 0 */
					flipscreen[0],flipscreen[1],
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}

		if (dirtybuffer2[offs])
		{
			int sx,sy;


			dirtybuffer2[offs] = 0;

			sx = offs % 32;
			sy = offs / 32;
			if (flipscreen[0]) sx = 31 - sx;
			if (flipscreen[1]) sy = 31 - sy;

			drawgfx(taitosj_tmpbitmap[1],Machine->gfx[taitosj_colorbank[0] & 0x80 ? 2 : 0],
					taitosj_videoram2[offs],
					((taitosj_colorbank[0] >> 4) & 0x07) + 8,	/* use transparent pen 0 */
					flipscreen[0],flipscreen[1],
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}

		if (dirtybuffer3[offs])
		{
			int sx,sy;


			dirtybuffer3[offs] = 0;

			sx = offs % 32;
			sy = offs / 32;
			if (flipscreen[0]) sx = 31 - sx;
			if (flipscreen[1]) sy = 31 - sy;

			drawgfx(taitosj_tmpbitmap[2],Machine->gfx[taitosj_colorbank[1] & 0x08 ? 2 : 0],
					taitosj_videoram3[offs],
					(taitosj_colorbank[1] & 0x07) + 8,	/* use transparent pen 0 */
					flipscreen[0],flipscreen[1],
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	/*called here because drawplayfield() uses its output (spritearea[32]) */
	calculate_sprites_areas();

	/* first of all, fill the screen with the background color */
	fillbitmap(bitmap,Machine->pens[8 * (taitosj_colorbank[1] & 0x07)],
			&Machine->visible_area);

	for (i = 0;i < 4;i++)
		drawplane(draworder[*taitosj_video_priority & 0x1f][i],bitmap);


	check_sprite_sprite_collision();

	/*check_sprite_plane_collision() uses drawn bitmaps, so it must me called _AFTER_ drawplane() */
	check_sprite_plane_collision();

	/*check_plane_plane_collision();*/	/*not implemented !!!*/
}
