/***************************************************************************

	Exidy Car Polo hardware

	driver by Zsolt Vasvari

****************************************************************************/

#include "driver.h"
#include "carpolo.h"


data8_t *carpolo_alpharam;
data8_t *carpolo_spriteram;


/* the screen elements' priorties determine their color */
#define BACKGROUND_COLOR	0
#define FIELD_COLOR			1
#define CAR1_COLOR			2
#define LINE_COLOR			3
#define CAR4_COLOR			4
#define CAR3_COLOR			5
#define CAR2_COLOR			6
#define BALL_COLOR			7
#define NET_COLOR			8
#define LEFT_GOAL_COLOR		9
#define RIGHT_GOAL_COLOR	10
#define SPECIAL_CHAR_COLOR	11
#define ALPHA0_COLOR		12
#define ALPHA1_COLOR		13
#define ALPHA2_COLOR		14
#define ALPHA3_COLOR		15

#define SCORE_COLOR			16	/* this is only used for collision detection purposes */

#define SPRITE_WIDTH		16
#define SPRITE_HEIGHT		16
#define GOAL_WIDTH			16
#define GOAL_HEIGHT			64

#define LEFT_GOAL_X			2*16-8
#define RIGHT_GOAL_X		13*16-8
#define GOAL_Y				7*16

#define TOP_BORDER			16
#define BOTTOM_BORDER		255
#define LEFT_BORDER			0
#define RIGHT_BORDER		239


static struct mame_bitmap *sprite_sprite_collision_bitmap1;
static struct mame_bitmap *sprite_sprite_collision_bitmap2;
static struct mame_bitmap *sprite_goal_collision_bitmap1;
static struct mame_bitmap *sprite_goal_collision_bitmap2;
static struct mame_bitmap *sprite_border_collision_bitmap;


/***************************************************************************
 *
 *	Palette generation
 *
 *  The palette PROM is connected to the RGB output this way.
 *
 *  bit 0 -- 220 ohm resistor  -- BLUE (probably an error on schematics)
 *        -- 470 ohm resistor  -- BLUE (probably an error on schematics)
 *        -- 220 ohm resistor  -- GREEN
 *        -- 470 ohm resistor  -- GREEN
 *        -- 1  kohm resistor  -- GREEN
 *        -- 220 ohm resistor  -- RED
 *        -- 470 ohm resistor  -- RED
 *  bit 7 -- 1  kohm resistor  -- RED
 *
 **************************************************************************/

PALETTE_INIT( carpolo )
{
	/* thanks to Jarek Burczynski for analyzing the circuit */
/*	const static float MAX_VOLTAGE = 6.9620;*/
	const static float MIN_VOLTAGE = 1.7434;
	const static float MAX_VOLTAGE = 5.5266;

	const static float r_voltage[] = { 1.7434, 2.1693, 2.5823, 3.0585,
									   3.4811, 4.0707, 4.7415, 5.4251 };

	const static float g_voltage[] = { 1.7434, 2.1693, 2.5823, 3.0585,
									   3.4811, 4.0707, 4.7415, 5.4251 };
/*	const static float g_voltage[] = { 4.7871, 5.0613, 5.3079, 5.6114,*/
/*									   5.7940, 6.1608, 6.5436, 6.9620 };*/

	const static float b_voltage[] = { 1.9176, 2.8757, 3.9825, 5.5266 };


	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	/* the -1 is for the fake score color */
	for (i = 0; i < Machine->drv->total_colors - 1; i++)
	{
		UINT8 r,g,b;
		int bit0,bit1,bit2;

		/* red component */
		bit0 = (*color_prom >> 7) & 0x01;
		bit1 = (*color_prom >> 6) & 0x01;
		bit2 = (*color_prom >> 5) & 0x01;
		/*r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;*/
		r = ((r_voltage[*color_prom >> 5         ] - MIN_VOLTAGE) / (MAX_VOLTAGE - MIN_VOLTAGE)) * 255.;
		/* green component */
		bit0 = (*color_prom >> 4) & 0x01;
		bit1 = (*color_prom >> 3) & 0x01;
		bit2 = (*color_prom >> 2) & 0x01;
		/*g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;*/
		g = ((g_voltage[(*color_prom >> 2) & 0x07] - MIN_VOLTAGE) / (MAX_VOLTAGE - MIN_VOLTAGE)) * 255.;
		/* blue component */
		bit0 = (*color_prom >> 1) & 0x01;
		bit1 = (*color_prom >> 0) & 0x01;
		/*b = 0x4f * bit0 + 0xa8 * bit1;*/
		b = ((b_voltage[*color_prom & 0x03       ] - MIN_VOLTAGE) / (MAX_VOLTAGE - MIN_VOLTAGE)) * 255.;


		palette_set_color(i,r,g,b);

		/* score color is same as net color*/
		if (i == NET_COLOR)
		{
			palette_set_color(SCORE_COLOR,r,g,b);
		}

		color_prom++;
	}


	/* sprite colors */
	for (i = 0; i < TOTAL_COLORS(0) / 2; i++)
	{
		COLOR(0,i*2+1) = i;
	}


	/* the bits in the goal gfx PROM are hooked as follows (all active LO):
	   Bit 3 - goal post
	   Bit 2 - scoring area
	   Bit 1 - net
	   Bit 0 - n/c

	   Below I am only filling in the colors actually used. */

	/* left goal */
	COLOR(1, (0x07 ^ 0x0f)) = LEFT_GOAL_COLOR;
	COLOR(1, (0x0d ^ 0x0f)) = NET_COLOR;
	COLOR(1, (0x09 ^ 0x0f)) = SCORE_COLOR;

	/* right goal */
	COLOR(1, (16 + (0x07 ^ 0x0f))) = RIGHT_GOAL_COLOR;
	COLOR(1, (16 + (0x0d ^ 0x0f))) = NET_COLOR;
	COLOR(1, (16 + (0x09 ^ 0x0f))) = SCORE_COLOR;

	/* alpha layer */
	COLOR(2, 0*2+1) = ALPHA0_COLOR;
	COLOR(2, 1*2+1) = ALPHA1_COLOR;
	COLOR(2, 2*2+1) = ALPHA2_COLOR;
	COLOR(2, 3*2+1) = ALPHA3_COLOR;
}


/*************************************
 *
 *	Video system start
 *
 *************************************/

VIDEO_START( carpolo )
{
	if ((sprite_sprite_collision_bitmap1 = auto_bitmap_alloc(SPRITE_WIDTH*2,SPRITE_HEIGHT*2)) == 0)
		return 1;

	if ((sprite_sprite_collision_bitmap2 = auto_bitmap_alloc(SPRITE_WIDTH*2,SPRITE_HEIGHT*2)) == 0)
		return 1;

	if ((sprite_goal_collision_bitmap1 = auto_bitmap_alloc(SPRITE_WIDTH+GOAL_WIDTH,SPRITE_HEIGHT+GOAL_HEIGHT)) == 0)
		return 1;

	if ((sprite_goal_collision_bitmap2 = auto_bitmap_alloc(SPRITE_WIDTH+GOAL_WIDTH,SPRITE_HEIGHT+GOAL_HEIGHT)) == 0)
		return 1;

	if ((sprite_border_collision_bitmap = auto_bitmap_alloc(SPRITE_WIDTH,SPRITE_HEIGHT)) == 0)
		return 1;

	return 0;
}


/*************************************
 *
 *	Core video refresh
 *
 *************************************/

static void draw_alpha_line(struct mame_bitmap *bitmap, const struct rectangle *cliprect,
					   		int alpha_line, int video_line)
{
	int x;


	for (x = 0; x < 32; x++)
	{
		data8_t code, col;

		code = carpolo_alpharam[alpha_line * 32 + x] >> 2;
		col  = carpolo_alpharam[alpha_line * 32 + x] & 0x03;

		drawgfx(bitmap,Machine->gfx[2],
				code,col,
				0,0,
				x*8,video_line*8,
				cliprect,TRANSPARENCY_PEN,0);
	}
}


static void remap_sprite_code(int bank, int code, int *remapped_code, int *flipy)
{
	UINT8 *PROM;


	PROM = memory_region(REGION_USER1);

	code = (bank << 4) | code;
	*remapped_code = PROM[code] & 0x0f;
	*flipy = (PROM[code] & 0x10) >> 4;
}


static void draw_sprite(struct mame_bitmap *bitmap, const struct rectangle *cliprect,
						UINT8 x, UINT8 y, int bank, int code, int col)
{
	int remapped_code, flipy;


	remap_sprite_code(bank, code, &remapped_code, &flipy);

	x = 240 - x;
	y = 240 - y;

	drawgfx(bitmap,Machine->gfx[0],
			remapped_code, col,
			0, flipy,
			x, y,
			cliprect,TRANSPARENCY_PEN,0);

	/* draw with wrap around */
	drawgfx(bitmap,Machine->gfx[0],
			remapped_code, col,
			0, flipy,
			(INT16)x - 256, y,
			cliprect,TRANSPARENCY_PEN,0);
}


VIDEO_UPDATE( carpolo )
{
	/* draw the playfield elements, in the correct priority order */

	/* score area - position determined by bit 4 of the vertical timing PROM */
	plot_box(bitmap,0,0,RIGHT_BORDER+1,TOP_BORDER,Machine->pens[BACKGROUND_COLOR]);

	/* field */
	plot_box(bitmap,0,TOP_BORDER,RIGHT_BORDER+1,BOTTOM_BORDER-TOP_BORDER+1,Machine->pens[FIELD_COLOR]);

	/* car 1 */
	draw_sprite(bitmap, cliprect,
				carpolo_spriteram[0x00], carpolo_spriteram[0x01],
				0, carpolo_spriteram[0x0c] & 0x0f, CAR1_COLOR);

	/* border - position determined by bit 4 and 7 of the vertical timing PROM */
	plot_box(bitmap,0,TOP_BORDER,   RIGHT_BORDER+1,1,Machine->pens[LINE_COLOR]);
	plot_box(bitmap,0,BOTTOM_BORDER,RIGHT_BORDER+1,1,Machine->pens[LINE_COLOR]);
	plot_box(bitmap,LEFT_BORDER,TOP_BORDER, 1,BOTTOM_BORDER-TOP_BORDER+1,Machine->pens[LINE_COLOR]);
	plot_box(bitmap,RIGHT_BORDER,TOP_BORDER,1,BOTTOM_BORDER-TOP_BORDER+1,Machine->pens[LINE_COLOR]);

	/* car 4 */
	draw_sprite(bitmap, cliprect,
				carpolo_spriteram[0x06], carpolo_spriteram[0x07],
				0, carpolo_spriteram[0x0d] >> 4, CAR4_COLOR);

	/* car 3 */
	draw_sprite(bitmap, cliprect,
				carpolo_spriteram[0x04], carpolo_spriteram[0x05],
				0, carpolo_spriteram[0x0d] & 0x0f, CAR3_COLOR);

	/* car 2 */
	draw_sprite(bitmap, cliprect,
				carpolo_spriteram[0x02], carpolo_spriteram[0x03],
				0, carpolo_spriteram[0x0c] >> 4, CAR2_COLOR);

	/* ball */
	draw_sprite(bitmap, cliprect,
				carpolo_spriteram[0x08], carpolo_spriteram[0x09],
				1, carpolo_spriteram[0x0e] & 0x0f, BALL_COLOR);

	/* left goal - position determined by bit 6 of the
	   horizontal and vertical timing PROMs */
	drawgfxzoom(bitmap,Machine->gfx[1],
				0,0,
				0,0,
				LEFT_GOAL_X,GOAL_Y,
				cliprect,TRANSPARENCY_PEN,0,
				0x20000,0x20000);

	/* right goal */
	drawgfxzoom(bitmap,Machine->gfx[1],
				0,1,
				1,0,
				RIGHT_GOAL_X,GOAL_Y,
				cliprect,TRANSPARENCY_PEN,0,
				0x20000,0x20000);

	/* special char - bit 0 of 0x0f enables it,
					  bit 1 marked as WIDE, but never appears to be set */
	if (carpolo_spriteram[0x0f] & 0x02)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "WIDE!\n");
	}

	if (carpolo_spriteram[0x0f] & 0x01)
	{
		draw_sprite(bitmap, cliprect,
					carpolo_spriteram[0x0a], carpolo_spriteram[0x0b],
					1, carpolo_spriteram[0x0e] >> 4, SPECIAL_CHAR_COLOR);
	}


	/* draw the alpha layer */

	/* there are only 8 lines of text repeated 4 times
	   and bit 3 of the vertical timing PROM controls in
	   which quadrant the line will actually appear */

	draw_alpha_line(bitmap, cliprect, 0, (0*4+0)*2  );
	draw_alpha_line(bitmap, cliprect, 1, (0*4+0)*2+1);
	draw_alpha_line(bitmap, cliprect, 2, (3*4+1)*2  );
	draw_alpha_line(bitmap, cliprect, 3, (3*4+1)*2+1);
	draw_alpha_line(bitmap, cliprect, 4, (1*4+2)*2  );
	draw_alpha_line(bitmap, cliprect, 5, (1*4+2)*2+1);
	draw_alpha_line(bitmap, cliprect, 6, (0*4+3)*2  );
	draw_alpha_line(bitmap, cliprect, 7, (0*4+3)*2+1);
}


/*************************************
 *
 *	End of frame callback
 *
 *************************************/

static void normalize_coordinates(int *x1, int *y1, int *x2, int *y2)
{
	if (*x1 < *x2)
	{
		*x2 = *x2 - *x1;
		*x1 = 0;
	}
	else
	{
		*x1 = *x1 - *x2;
		*x2 = 0;
	}

	if (*y1 < *y2)
	{
		*y2 = *y2 - *y1;
		*y1 = 0;
	}
	else
	{
		*y1 = *y1 - *y2;
		*y2 = 0;
	}
}


static int check_sprite_sprite_collision(int x1, int y1, int code1, int flipy1,
										 int x2, int y2, int code2, int flipy2,
										 int *col_x, int *col_y)
{
	int collided = 0;


	x1 = 240 - x1;
	y1 = 240 - y1;
	x2 = 240 - x2;
	y2 = 240 - y2;


	/* check if the two sprites are within collision range*/
	if ((abs(x1 - x2) < SPRITE_WIDTH) && (abs(y1 - y2) < SPRITE_HEIGHT))
	{
		int x,y;


		normalize_coordinates(&x1, &y1, &x2, &y2);

		fillbitmap(sprite_sprite_collision_bitmap1, Machine->pens[0], 0);
		fillbitmap(sprite_sprite_collision_bitmap2, Machine->pens[0], 0);

		drawgfx(sprite_sprite_collision_bitmap1,Machine->gfx[0],
				code1,1,
				0,flipy1,
				x1,y1,
				0,TRANSPARENCY_PEN,0);

		drawgfx(sprite_sprite_collision_bitmap2,Machine->gfx[0],
				code2,1,
				0,flipy2,
				x2,y2,
				0,TRANSPARENCY_PEN,0);


		for (x = x1; x < x1 + SPRITE_WIDTH; x++)
		{
			for (y = y1; y < y1 + SPRITE_HEIGHT; y++)
			{
				if ((read_pixel(sprite_sprite_collision_bitmap1, x, y) == Machine->pens[1]) &&
				    (read_pixel(sprite_sprite_collision_bitmap2, x, y) == Machine->pens[1]))
				{
					*col_x = (x1 + x) & 0x0f;
					*col_y = (y1 + y) & 0x0f;

					collided = 1;

					break;
				}
			}
		}
	}

	return collided;
}


/* returns 1 for collision with goal post,
   2 for collision with scoring area */
static int check_sprite_left_goal_collision(int x1, int y1, int code1, int flipy1, int goalpost_only)
{
	int collided = 0;


	x1 = 240 - x1;
	y1 = 240 - y1;


	/* check if the sprites is within the range of the goal*/
	if (((y1 + 16) > GOAL_Y) && (y1 < (GOAL_Y + GOAL_HEIGHT)) &&
	    ((x1 + 16) > LEFT_GOAL_X) && (x1 < (LEFT_GOAL_X + GOAL_WIDTH)))
	{
		int x,y;
		int x2,y2;


		x2 = LEFT_GOAL_X;
		y2 = GOAL_Y;

		normalize_coordinates(&x1, &y1, &x2, &y2);

		fillbitmap(sprite_goal_collision_bitmap1, Machine->pens[0], 0);
		fillbitmap(sprite_goal_collision_bitmap2, Machine->pens[0], 0);

		drawgfx(sprite_goal_collision_bitmap1,Machine->gfx[0],
				code1,1,
				0,flipy1,
				x1,y1,
				0,TRANSPARENCY_PEN,0);

		drawgfxzoom(sprite_goal_collision_bitmap2,Machine->gfx[1],
					0,0,
					0,0,
					x2,y2,
					0,TRANSPARENCY_PEN,0,
					0x20000,0x20000);

		for (x = x1; x < x1 + SPRITE_WIDTH; x++)
		{
			for (y = y1; y < y1 + SPRITE_HEIGHT; y++)
			{
				if ((read_pixel(sprite_goal_collision_bitmap1, x, y) == Machine->pens[1]))
				{
					pen_t pix = read_pixel(sprite_goal_collision_bitmap2, x, y);

					if (pix == Machine->pens[LEFT_GOAL_COLOR])
					{
						collided = 1;
						break;
					}

					if (!goalpost_only && (pix == Machine->pens[SCORE_COLOR]))
					{
						collided = 2;
						break;
					}
				}
			}
		}
	}

	return collided;
}


static int check_sprite_right_goal_collision(int x1, int y1, int code1, int flipy1, int goalpost_only)
{
	int collided = 0;


	x1 = 240 - x1;
	y1 = 240 - y1;


	/* check if the sprites is within the range of the goal*/
	if (((y1 + 16) > GOAL_Y) && (y1 < (GOAL_Y + GOAL_HEIGHT)) &&
	    ((x1 + 16) > RIGHT_GOAL_X) && (x1 < (RIGHT_GOAL_X + GOAL_WIDTH)))
	{
		int x,y;
		int x2,y2;


		x2 = RIGHT_GOAL_X;
		y2 = GOAL_Y;

		normalize_coordinates(&x1, &y1, &x2, &y2);

		fillbitmap(sprite_goal_collision_bitmap1, Machine->pens[0], 0);
		fillbitmap(sprite_goal_collision_bitmap2, Machine->pens[0], 0);

		drawgfx(sprite_goal_collision_bitmap1,Machine->gfx[0],
				code1,1,
				0,flipy1,
				x1,y1,
				0,TRANSPARENCY_PEN,0);

		drawgfxzoom(sprite_goal_collision_bitmap2,Machine->gfx[1],
					0,1,
					1,0,
					x2,y2,
					0,TRANSPARENCY_PEN,0,
					0x20000,0x20000);

		for (x = x1; x < x1 + SPRITE_WIDTH; x++)
		{
			for (y = y1; y < y1 + SPRITE_HEIGHT; y++)
			{
				if ((read_pixel(sprite_goal_collision_bitmap1, x, y) == Machine->pens[1]))
				{
					pen_t pix = read_pixel(sprite_goal_collision_bitmap2, x, y);

					if (pix == Machine->pens[RIGHT_GOAL_COLOR])
					{
						collided = 1;
						break;
					}

					if (!goalpost_only && (pix == Machine->pens[SCORE_COLOR]))
					{
						collided = 2;
						break;
					}
				}
			}
		}
	}

	return collided;
}


/* returns 1 for collision with vertical border,
   2 for collision with horizontal border */
static int check_sprite_border_collision(data8_t x1, data8_t y1, int code1, int flipy1)
{
	data8_t x,y;
	int collided = 0;


	x1 = 240 - x1;
	y1 = 240 - y1;


	fillbitmap(sprite_border_collision_bitmap, Machine->pens[0], 0);

	drawgfx(sprite_border_collision_bitmap,Machine->gfx[0],
			code1,1,
			0,flipy1,
			0,0,
			0,TRANSPARENCY_PEN,0);

	for (x = 0; x < SPRITE_WIDTH; x++)
	{
		for (y = 0; y < SPRITE_HEIGHT; y++)
		{
			if ((read_pixel(sprite_border_collision_bitmap, x, y) == Machine->pens[1]))
			{
				if (((data8_t)(x1 + x) == LEFT_BORDER) ||
					((data8_t)(x1 + x) == RIGHT_BORDER))
				{
					collided = 1;
					break;
				}

				if (((data8_t)(y1 + y) == TOP_BORDER) ||
					((data8_t)(y1 + y) == BOTTOM_BORDER))
				{
					collided = 2;
					break;
				}
			}
		}
	}

	return collided;
}


VIDEO_EOF( carpolo )
{
	int col_x, col_y;
	int car1_x, car2_x, car3_x, car4_x, ball_x;
	int car1_y, car2_y, car3_y, car4_y, ball_y;
	int car1_code, car2_code, car3_code, car4_code, ball_code;
	int car1_flipy, car2_flipy, car3_flipy, car4_flipy, ball_flipy;


	/* check car-car collision first*/

	car1_x = carpolo_spriteram[0x00];
	car1_y = carpolo_spriteram[0x01];
	remap_sprite_code(0, carpolo_spriteram[0x0c] & 0x0f, &car1_code, &car1_flipy);

	car2_x = carpolo_spriteram[0x02];
	car2_y = carpolo_spriteram[0x03];
	remap_sprite_code(0, carpolo_spriteram[0x0c] >> 4,   &car2_code, &car2_flipy);

	car3_x = carpolo_spriteram[0x04];
	car3_y = carpolo_spriteram[0x05];
	remap_sprite_code(0, carpolo_spriteram[0x0d] & 0x0f, &car3_code, &car3_flipy);

	car4_x = carpolo_spriteram[0x06];
	car4_y = carpolo_spriteram[0x07];
	remap_sprite_code(0, carpolo_spriteram[0x0d] >> 4,   &car4_code, &car4_flipy);

	ball_x = carpolo_spriteram[0x08];
	ball_y = carpolo_spriteram[0x09];
	remap_sprite_code(1, carpolo_spriteram[0x0e] & 0x0f, &ball_code, &ball_flipy);


	/* cars 1 and 2*/
	if (check_sprite_sprite_collision(car1_x, car1_y, car1_code, car1_flipy,
									  car2_x, car2_y, car2_code, car2_flipy,
									  &col_x, &col_y))
	{
		carpolo_generate_car_car_interrupt(0, 1);
	}

	/* cars 1 and 3*/
	else if (check_sprite_sprite_collision(car1_x, car1_y, car1_code, car1_flipy,
										   car3_x, car3_y, car3_code, car3_flipy,
										   &col_x, &col_y))
	{
		carpolo_generate_car_car_interrupt(0, 2);
	}

	/* cars 1 and 4*/
	else if (check_sprite_sprite_collision(car1_x, car1_y, car1_code, car1_flipy,
										   car4_x, car4_y, car4_code, car4_flipy,
										   &col_x, &col_y))
	{
		carpolo_generate_car_car_interrupt(0, 3);
	}

	/* cars 2 and 3*/
	else if (check_sprite_sprite_collision(car2_x, car2_y, car2_code, car2_flipy,
										   car3_x, car3_y, car3_code, car3_flipy,
										   &col_x, &col_y))
	{
		carpolo_generate_car_car_interrupt(1, 2);
	}

	/* cars 2 and 4*/
	else if (check_sprite_sprite_collision(car2_x, car2_y, car2_code, car2_flipy,
										   car4_x, car4_y, car4_code, car4_flipy,
										   &col_x, &col_y))
	{
		carpolo_generate_car_car_interrupt(1, 3);
	}

	/* cars 3 and 4*/
	else if (check_sprite_sprite_collision(car3_x, car3_y, car3_code, car3_flipy,
										   car4_x, car4_y, car4_code, car4_flipy,
										   &col_x, &col_y))
	{
		carpolo_generate_car_car_interrupt(2, 3);
	}



	/* check car-ball collision*/
	if (check_sprite_sprite_collision(car1_x, car1_y, car1_code, car1_flipy,
									  ball_x, ball_y, ball_code, ball_flipy,
									  &col_x, &col_y))
	{
		carpolo_generate_car_ball_interrupt(0, col_x, col_y);
	}
	else if (check_sprite_sprite_collision(car2_x, car2_y, car2_code, car2_flipy,
									  	   ball_x, ball_y, ball_code, ball_flipy,
										   &col_x, &col_y))
	{
		carpolo_generate_car_ball_interrupt(1, col_x, col_y);
	}
	else if (check_sprite_sprite_collision(car3_x, car3_y, car3_code, car3_flipy,
									  	   ball_x, ball_y, ball_code, ball_flipy,
										   &col_x, &col_y))
	{
		carpolo_generate_car_ball_interrupt(2, col_x, col_y);
	}
	else if (check_sprite_sprite_collision(car4_x, car4_y, car4_code, car4_flipy,
									  	   ball_x, ball_y, ball_code, ball_flipy,
										   &col_x, &col_y))
	{
		carpolo_generate_car_ball_interrupt(3, col_x, col_y);
	}


	/* check car-goal collision*/
	if (check_sprite_left_goal_collision(car1_x, car1_y, car1_code, car1_flipy, 1))
	{
		carpolo_generate_car_goal_interrupt(0, 0);
	}
	else if (check_sprite_right_goal_collision(car1_x, car1_y, car1_code, car1_flipy, 1))
	{
		carpolo_generate_car_goal_interrupt(0, 1);
	}
	else if (check_sprite_left_goal_collision(car2_x, car2_y, car2_code, car2_flipy, 1))
	{
		carpolo_generate_car_goal_interrupt(1, 0);
	}
	else if (check_sprite_right_goal_collision(car2_x, car2_y, car2_code, car2_flipy, 1))
	{
		carpolo_generate_car_goal_interrupt(1, 1);
	}
	else if (check_sprite_left_goal_collision(car3_x, car3_y, car3_code, car3_flipy, 1))
	{
		carpolo_generate_car_goal_interrupt(2, 0);
	}
	else if (check_sprite_right_goal_collision(car3_x, car3_y, car3_code, car3_flipy, 1))
	{
		carpolo_generate_car_goal_interrupt(2, 1);
	}
	else if (check_sprite_left_goal_collision(car4_x, car4_y, car4_code, car4_flipy, 1))
	{
		carpolo_generate_car_goal_interrupt(3, 0);
	}
	else if (check_sprite_right_goal_collision(car4_x, car4_y, car4_code, car4_flipy, 1))
	{
		carpolo_generate_car_goal_interrupt(3, 1);
	}


	/* check ball collision with static screen elements*/
	{
	int col;

	col = check_sprite_left_goal_collision(ball_x, ball_y, ball_code, ball_flipy, 0);

	if (col == 1)  carpolo_generate_ball_screen_interrupt(0x05);
	if (col == 2)  carpolo_generate_ball_screen_interrupt(0x03);


	col = check_sprite_right_goal_collision(ball_x, ball_y, ball_code, ball_flipy, 0);

	if (col == 1)  carpolo_generate_ball_screen_interrupt(0x05 | 0x08);
	if (col == 2)  carpolo_generate_ball_screen_interrupt(0x03 | 0x08);


	if (check_sprite_border_collision(ball_x, ball_y, ball_code, ball_flipy))
	{
		carpolo_generate_ball_screen_interrupt(0x06);
	}
	}


	/* check car-border collision*/
	{
	int col;

	col = check_sprite_border_collision(car1_x, car1_y, car1_code, car1_flipy);

	if (col)
	{
		carpolo_generate_car_border_interrupt(0, (col == 2));
	}
	else
	{
		col = check_sprite_border_collision(car2_x, car2_y, car2_code, car2_flipy);

		if (col)
		{
			carpolo_generate_car_border_interrupt(1, (col == 2));
		}
		else
		{
			col = check_sprite_border_collision(car3_x, car3_y, car3_code, car3_flipy);

			if (col)
			{
				carpolo_generate_car_border_interrupt(2, (col == 2));
			}
			else
			{
				col = check_sprite_border_collision(car4_x, car4_y, car4_code, car4_flipy);

				if (col)
				{
					carpolo_generate_car_border_interrupt(3, (col == 2));
				}
			}
		}
	}
	}
}
