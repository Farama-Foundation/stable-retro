/***************************************************************************

  vidhrdw\cvs.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "vidhrdw/generic.h"
#include "vidhrdw/s2636.h"

#define MAX_STARS        250
#define STARS_COLOR_BASE 16

#ifdef MSB_FIRST
#define BL0 3
#define BL1 2
#define BL2 1
#define BL3 0
#define WL0 1
#define WL1 0
#else
#define BL0 0
#define BL1 1
#define BL2 2
#define BL3 3
#define WL0 0
#define WL1 1
#endif

struct star
{
	int x,y,code;
};

static struct star stars[MAX_STARS];
static int    total_stars;
static int    scroll[8];
static int    CollisionRegister=0;
static int    stars_on=0;
static int 	  character_mode=0;
static int    character_page=0;
static int    scroll_reg = 0;
static int    stars_scroll=0;

int  s2650_get_flag(void);

unsigned char *dirty_character;
unsigned char *character_1_ram;
unsigned char *character_2_ram;
unsigned char *character_3_ram;
unsigned char *bullet_ram;
unsigned char *cvs_s2636_1_ram;
unsigned char *cvs_s2636_2_ram;
unsigned char *cvs_s2636_3_ram;

struct mame_bitmap *s2636_1_bitmap;
struct mame_bitmap *s2636_2_bitmap;
struct mame_bitmap *s2636_3_bitmap;
static struct mame_bitmap *collision_bitmap;
static struct mame_bitmap *collision_background;
static struct mame_bitmap *scrolled_background;

static unsigned char s2636_1_dirty[4];
static unsigned char s2636_2_dirty[4];
static unsigned char s2636_3_dirty[4];

static int ModeOffset[4] = {223,191,255,127};

/******************************************************
 * Convert Colour prom to format for Mame Colour Map  *
 *                                                    *
 * There is a prom used for colour mapping and plane  *
 * priority. This is converted to a colour table here *
 *                                                    *
 * colours are taken from SRAM and are programmable   *
 ******************************************************/

PALETTE_INIT( cvs )
{
	int attr,col,map;

	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])

    /* Colour Mapping Prom */

    for(attr = 0;attr < 256; attr++)
    {
    	for(col = 0; col < 8; col++)
        {
          	map = color_prom[(col * 256) + attr];

            /* bits 1 and 3 are swopped over */

            COLOR(0,attr*8 + col) = ((map & 1) << 2) + (map & 2) + ((map & 4) >> 2);
        }
    }

    /* Background Collision Map */

    for(map=0;map<8;map++)
    {
    	COLOR(0,2048+map) = (map & 4) >> 2;
        COLOR(0,2056+map) = (map & 2) >> 1;
        COLOR(0,2064+map) = ((map & 2) >> 1) || ((map & 4) >> 2);
    }

    /* Sprites */

    for(map=0;map<8;map++)
    {
    	COLOR(0,map*2 + 2072) = 0;
    	COLOR(0,map*2 + 2073) = 8 + map;
    }

    /* Initialise Dirty Character Array */

	memset(dirty_character, 0, 256);
    memset(character_1_ram, 0, 1024);
    memset(character_2_ram, 0, 1024);
    memset(character_3_ram, 0, 1024);

    /* Set Sprite chip offsets */

	s2636_x_offset = -26;
	s2636_y_offset = 3;

    /* Set Scroll fixed areas */

    scroll[0]=0;
    scroll[6]=0;
    scroll[7]=0;
}

WRITE_HANDLER( cvs_video_fx_w )
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "%4x : Data Port = %2x\n",activecpu_get_pc(),data);

    /* Unimplemented */

    if(data & 2)   log_cb(RETRO_LOG_DEBUG, LOGPRE "       SHADE BRIGHTER TO RIGHT\n");
    if(data & 4)   log_cb(RETRO_LOG_DEBUG, LOGPRE "       SCREEN ROTATE\n");
    if(data & 8)   log_cb(RETRO_LOG_DEBUG, LOGPRE "       SHADE BRIGHTER TO LEFT\n");
    if(data & 64)  log_cb(RETRO_LOG_DEBUG, LOGPRE "       SHADE BRIGHTER TO BOTTOM\n");
    if(data & 128) log_cb(RETRO_LOG_DEBUG, LOGPRE "       SHADE BRIGHTER TO TOP\n");

    /* Implemented */

    stars_on = data & 1;
    set_led_status(1,data & 16);	/* Lamp 1 */
    set_led_status(2,data & 32);	/* Lamp 2 */
}

READ_HANDLER( cvs_character_mode_r )
{
	/* Really a write - uses address info */

    int value   = offset + 0x10;
    int newmode = (value >> 4) & 3;

    if(newmode != character_mode)
    {
	    character_mode = newmode;
        memset(dirtybuffer,1,videoram_size);
    }

    character_page = (value << 2) & 0x300;

    return 0;
}

READ_HANDLER( cvs_collision_r )
{
	return CollisionRegister;
}

READ_HANDLER( cvs_collision_clear )
{
	CollisionRegister=0;
    return 0;
}

WRITE_HANDLER( cvs_scroll_w )
{
	scroll_reg = 255 - data;

    scroll[1]=scroll_reg;
    scroll[2]=scroll_reg;
    scroll[3]=scroll_reg;
    scroll[4]=scroll_reg;
    scroll[5]=scroll_reg;
}

WRITE_HANDLER( cvs_videoram_w )
{
	if(!s2650_get_flag())
    {
    	/* Colour Map*/

        colorram_w(offset,data);
    }
    else
    {
    	/* Data*/

        videoram_w(offset,data);
    }
}

READ_HANDLER( cvs_videoram_r )
{
	if(!s2650_get_flag())
    {
    	/* Colour Map*/

        return colorram[offset];
    }
    else
    {
    	/* Data*/

        return videoram[offset];
    }
}

WRITE_HANDLER( cvs_bullet_w )
{
	if(!s2650_get_flag())
    {
    	/* Bullet Ram*/

        bullet_ram[offset] = data;
    }
    else
    {
    	/* Pallette Ram - Inverted ?*/

		paletteram_BBBGGGRR_w((offset & 0x0f),(data ^ 0xff));
    }
}

READ_HANDLER( cvs_bullet_r )
{
	if(!s2650_get_flag())
    {
    	/* Bullet Ram*/

        return bullet_ram[offset];
    }
    else
    {
    	/* Pallette Ram*/

        return (paletteram[offset] ^ 0xff);
    }
}

WRITE_HANDLER( cvs_2636_1_w )
{
	if(!s2650_get_flag())
    {
    	/* First 2636*/

        s2636_w(cvs_s2636_1_ram,offset,data,s2636_1_dirty);
    }
    else
    {
    	/* Character Ram 1*/

        if(character_1_ram[character_page + offset] != data)
        {
        	character_1_ram[character_page + offset] = data;
			dirty_character[128+((character_page + offset)>>3)] = 1;
        }
	}
}

READ_HANDLER( cvs_2636_1_r )
{
	if(!s2650_get_flag())
    {
    	/* First 2636*/

        return cvs_s2636_1_ram[offset];
    }
    else
    {
    	/* Character Ram 1*/

        return character_1_ram[character_page + offset];
    }
}

WRITE_HANDLER( cvs_2636_2_w )
{
	if(!s2650_get_flag())
    {
    	/* Second 2636*/

        s2636_w(cvs_s2636_2_ram,offset,data,s2636_2_dirty);
    }
    else
    {
    	/* Character Ram 2*/

        if(character_2_ram[character_page + offset] != data)
        {
        	character_2_ram[character_page + offset] = data;
			dirty_character[128+((character_page + offset)>>3)] = 1;
        }
    }
}

READ_HANDLER( cvs_2636_2_r )
{
	if(!s2650_get_flag())
    {
    	/* Second 2636*/

        return cvs_s2636_2_ram[offset];
    }
    else
    {
    	/* Character Ram 2*/

        return character_2_ram[character_page + offset];
    }
}

WRITE_HANDLER( cvs_2636_3_w )
{
	if(!s2650_get_flag())
    {
    	/* Third 2636*/

        s2636_w(cvs_s2636_3_ram,offset,data,s2636_3_dirty);
    }
    else
    {
    	/* Character Ram 3*/

        if(character_3_ram[character_page + offset] != data)
        {
        	character_3_ram[character_page + offset] = data;
			dirty_character[128+((character_page + offset)>>3)] = 1;
        }
    }
}

READ_HANDLER( cvs_2636_3_r )
{
	if(!s2650_get_flag())
    {
    	/* Third 2636*/

        return cvs_s2636_3_ram[offset];
    }
    else
    {
    	/* Character Ram 3*/

        return character_3_ram[character_page + offset];
    }
}

VIDEO_START( cvs )
{
	int generator = 0;
    int x,y;

	video_start_generic();

	/* precalculate the star background */

	total_stars = 0;

	for (y = 255;y >= 0;y--)
	{
		for (x = 511;x >= 0;x--)
		{
			int bit1,bit2;

			generator <<= 1;
			bit1 = (~generator >> 17) & 1;
			bit2 = (generator >> 5) & 1;

			if (bit1 ^ bit2) generator |= 1;

			if (((~generator >> 16) & 1) && (generator & 0xfe) == 0xfe)
			{
            	if(((~(generator >> 12)) & 0x01) && ((~(generator >> 13)) & 0x01))
                {
				    if (total_stars < MAX_STARS)
				    {
					    stars[total_stars].x = x;
					    stars[total_stars].y = y;
					    stars[total_stars].code = 1;

					    total_stars++;
				    }
                }
			}
		}
	}

    /* Need 3 bitmaps for 2636 chips */

	if ((s2636_1_bitmap = auto_bitmap_alloc_depth(Machine->drv->screen_width,Machine->drv->screen_height,8)) == 0)
		return 1;

	if ((s2636_2_bitmap = auto_bitmap_alloc_depth(Machine->drv->screen_width,Machine->drv->screen_height,8)) == 0)
		return 1;

	if ((s2636_3_bitmap = auto_bitmap_alloc_depth(Machine->drv->screen_width,Machine->drv->screen_height,8)) == 0)
		return 1;

    /* 3 bitmaps for collision detection */

	if ((collision_bitmap = auto_bitmap_alloc_depth(Machine->drv->screen_width,Machine->drv->screen_height,8)) == 0)
		return 1;

	if ((collision_background = auto_bitmap_alloc_depth(Machine->drv->screen_width,Machine->drv->screen_height,8)) == 0)
		return 1;

	if ((scrolled_background = auto_bitmap_alloc_depth(Machine->drv->screen_width,Machine->drv->screen_height,8)) == 0)
		return 1;

	return 0;
}

INTERRUPT_GEN( cvs_interrupt )
{
	stars_scroll++;

	cpu_irq_line_vector_w(0,0,0x03);
	cpu_set_irq_line(0,0,PULSE_LINE);
}

static INLINE void plot_star(struct mame_bitmap *bitmap, int x, int y)
{
	if (flip_screen_x)
	{
		x = 255 - x;
	}
	if (flip_screen_y)
	{
		y = 255 - y;
	}

	if (read_pixel(bitmap, x, y) == Machine->pens[0])
	{
		plot_pixel(bitmap, x, y, Machine->pens[7]);
	}
}

VIDEO_UPDATE( cvs )
{
	int offs,character;
	int sx,sy;

	if (get_vh_global_attribute_changed())
		memset(dirtybuffer, 1, videoram_size);

	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */

	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
        character = videoram[offs];

		if(dirtybuffer[offs] || dirty_character[character])
		{
            int character_bank;
            int forecolor;

			dirtybuffer[offs] = 0;

			sx = (offs % 32) * 8;
			sy = (offs / 32) * 8;

            /* Decide if RAM or ROM based character */

            if(character > ModeOffset[character_mode])
            {
            	/* re-generate character if dirty */

                if(dirty_character[character]==1)
                {
                	dirty_character[character]=2;
		   			decodechar(Machine->gfx[1],character,character_1_ram-1024,Machine->drv->gfxdecodeinfo[1].gfxlayout);
                }

            	character_bank=1;
            }
            else
            {
            	character_bank=0;
            }

            /* Main Screen */

 			drawgfx(tmpbitmap,Machine->gfx[character_bank],
				    character,
					colorram[offs],
				    0,0,
				    sx,sy,
				    0,TRANSPARENCY_NONE,0);


            /* Foreground for Collision Detection */

            forecolor = 0;
            if(colorram[offs] & 0x80)
            {
				forecolor=258;
            }
            else
			{
				if((colorram[offs] & 0x03) == 3) forecolor=256;
                else if((colorram[offs] & 0x01) == 0) forecolor=257;
            }

            if(forecolor)
 			    drawgfx(collision_background,Machine->gfx[character_bank],
				        character,
					    forecolor,
				        0,0,
				        sx,sy,
				        0,TRANSPARENCY_NONE,0);
		}
	}

    /* Tidy up dirty character map */

    for(offs=128;offs<256;offs++)
    	if(dirty_character[offs]==2) dirty_character[offs]=0;

    /* Update screen - 8 regions, fixed scrolling area */

	copyscrollbitmap(bitmap,tmpbitmap,0,0,8,scroll,&Machine->visible_area,TRANSPARENCY_NONE,0);
	copyscrollbitmap(scrolled_background,collision_background,0,0,8,scroll,&Machine->visible_area,TRANSPARENCY_NONE,0);

    /* 2636's */

	fillbitmap(s2636_1_bitmap,0,0);
	Update_Bitmap(s2636_1_bitmap,cvs_s2636_1_ram,s2636_1_dirty,2,collision_bitmap);

	fillbitmap(s2636_2_bitmap,0,0);
	Update_Bitmap(s2636_2_bitmap,cvs_s2636_2_ram,s2636_2_dirty,3,collision_bitmap);

	fillbitmap(s2636_3_bitmap,0,0);
	Update_Bitmap(s2636_3_bitmap,cvs_s2636_3_ram,s2636_3_dirty,4,collision_bitmap);

    /* Bullet Hardware */

    for (offs = 8; offs < 256; offs++ )
    {
        if(bullet_ram[offs] != 0)
        {
        	int ct;
            for(ct=0;ct<4;ct++)
            {
            	int bx=255-7-bullet_ram[offs]-ct;

            	/* Bullet/Object Collision */

                if((CollisionRegister & 8) == 0)
                {
                    if ((read_pixel(s2636_1_bitmap, bx, offs) != 0) ||
					    (read_pixel(s2636_2_bitmap, bx, offs) != 0) ||
					    (read_pixel(s2636_3_bitmap, bx, offs) != 0))
                        CollisionRegister |= 8;
                }

            	/* Bullet/Background Collision */

                if((CollisionRegister & 0x80) == 0)
                {
					if (read_pixel(scrolled_background, bx, offs) != Machine->pens[0])
                    	CollisionRegister |= 0x80;
                }

	            plot_pixel(bitmap,bx,offs,Machine->pens[7]);
            }
        }
    }

    /* Update 2636 images */

	if (bitmap->depth == 16)
    {
        UINT32 S1,S2,S3,SB,pen;

        for(sx=255;sx>7;sx--)
        {
        	UINT32 *sp1 = (UINT32 *)s2636_1_bitmap->line[sx];
	    	UINT32 *sp2 = (UINT32 *)s2636_2_bitmap->line[sx];
		    UINT32 *sp3 = (UINT32 *)s2636_3_bitmap->line[sx];
	        UINT64 *dst = (UINT64 *)bitmap->line[sx];
		    UINT8  *spb = (UINT8  *)scrolled_background->line[sx];

            for(offs=0;offs<62;offs++)
            {
        	     S1 = (*sp1++);
                 S2 = (*sp2++);
                 S3 = (*sp3++);

        	     pen = S1 | S2 | S3;

                 if(pen)
                 {
             	    UINT16 *address = (UINT16 *)dst;
				    if (pen & 0xff000000) address[BL3] = Machine->pens[(pen >> 24) & 15];
				    if (pen & 0x00ff0000) address[BL2] = Machine->pens[(pen >> 16) & 15];
				    if (pen & 0x0000ff00) address[BL1] = Machine->pens[(pen >>  8) & 15];
				    if (pen & 0x000000ff) address[BL0] = Machine->pens[(pen & 15)];

                    /* Collision Detection */

                    SB = 0;
				    if (spb[BL3] != Machine->pens[0]) SB =  0x08000000;
				    if (spb[BL2] != Machine->pens[0]) SB |= 0x00080000;
				    if (spb[BL1] != Machine->pens[0]) SB |= 0x00000800;
				    if (spb[BL0] != Machine->pens[0]) SB |= 0x00000008;

       	            if (S1 & S2) CollisionRegister |= 1;
       	            if (S2 & S3) CollisionRegister |= 2;
    			    if (S1 & S3) CollisionRegister |= 4;

                    if (SB)
                    {
    			        if (S1 & SB) CollisionRegister |= 16;
   			            if (S2 & SB) CollisionRegister |= 32;
       	                if (S3 & SB) CollisionRegister |= 64;
                    }
                 }

           	     dst++;
                 spb+=4;
            }
        }
    }
    else
	{
        for(sx=255;sx>7;sx--)
        {
	        UINT32 *sp1 = (UINT32 *)s2636_1_bitmap->line[sx];
	        UINT32 *sp2 = (UINT32 *)s2636_2_bitmap->line[sx];
	        UINT32 *sp3 = (UINT32 *)s2636_3_bitmap->line[sx];
            UINT32 *dst = (UINT32 *)bitmap->line[sx];
	        UINT8  *spb = (UINT8  *)scrolled_background->line[sx];

            UINT32 S1,S2,S3,SB,pen;

            for(offs=0;offs<62;offs++)
            {
        	     S1 = (*sp1++);
                 S2 = (*sp2++);
                 S3 = (*sp3++);

        	     pen = S1 | S2 | S3;

                 if(pen)
                 {
             	    UINT8 *address = (UINT8 *)dst;
				    if (pen & 0xff000000) address[BL3] = Machine->pens[(pen >> 24) & 15];
				    if (pen & 0x00ff0000) address[BL2] = Machine->pens[(pen >> 16) & 15];
				    if (pen & 0x0000ff00) address[BL1] = Machine->pens[(pen >>  8) & 15];
				    if (pen & 0x000000ff) address[BL0] = Machine->pens[(pen & 15)];

                    /* Collision Detection */

                    SB = 0;
				    if (spb[BL3] != Machine->pens[0]) SB =  0x08000000;
				    if (spb[BL2] != Machine->pens[0]) SB |= 0x00080000;
				    if (spb[BL1] != Machine->pens[0]) SB |= 0x00000800;
				    if (spb[BL0] != Machine->pens[0]) SB |= 0x00000008;

       	            if (S1 & S2) CollisionRegister |= 1;
       	            if (S2 & S3) CollisionRegister |= 2;
    			    if (S1 & S3) CollisionRegister |= 4;

                    if (SB)
                    {
    			        if (S1 & SB) CollisionRegister |= 16;
   			            if (S2 & SB) CollisionRegister |= 32;
       	                if (S3 & SB) CollisionRegister |= 64;
                    }
                 }

           	     dst++;
                 spb+=4;
            }
        }
    }

    /* Stars */

    if(stars_on)
    {
		for (offs = 0;offs < total_stars;offs++)
		{
			int x,y;


			x = ((stars[offs].x + stars_scroll) % 512) / 2;
			y = (stars[offs].y + (stars_scroll + stars[offs].x) / 512) % 256;

			if (y >= Machine->visible_area.min_y &&
				y <= Machine->visible_area.max_y)
			{
				if ((y & 1) ^ ((x >> 4) & 1))
				{
					plot_star(bitmap, x, y);
				}
			}
		}

    }
}
