#include "driver.h"
#include "state.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/taitoic.h"

static struct mame_bitmap* pixel_layer = 0;

static UINT16* video_ram = NULL;

static UINT16 video_ctrl = 0;
static UINT16 video_mask = 0;

static UINT8* line_dirty = NULL;

static void mark_all_dirty(void)
{
	memset(line_dirty, 1, 256);
}


/******************************************************
          INITIALISATION AND CLEAN-UP
******************************************************/

VIDEO_START( volfied )
{
	pixel_layer = auto_bitmap_alloc(Machine->drv->screen_width, Machine->drv->screen_height);
	if (pixel_layer == NULL)
		return 1;

	line_dirty = auto_malloc(256);
	if (line_dirty == NULL)
		return 1;

	video_ram = auto_malloc(0x40000 * sizeof (UINT16));
	if (video_ram == NULL)
		return 1;

	state_save_register_UINT16("volfied", 0, "video_ram",   video_ram,    0x40000);
	state_save_register_UINT16("volfied", 0, "video_ctrl",  &video_ctrl,  1);
	state_save_register_UINT16("volfied", 0, "video_mask",  &video_mask,  1);

	state_save_register_func_postload (mark_all_dirty);

	return PC090OJ_vh_start(0, 0, 0, 0);
}


/*******************************************************
          READ AND WRITE HANDLERS
*******************************************************/

READ16_HANDLER( volfied_video_ram_r )
{
	return video_ram[offset];
}

WRITE16_HANDLER( volfied_video_ram_w )
{
	mem_mask |= ~video_mask;

	line_dirty[(offset >> 9) & 0xff] = 1;

	COMBINE_DATA(&video_ram[offset]);
}

WRITE16_HANDLER( volfied_video_ctrl_w )
{
	if (ACCESSING_LSB && (data & 1) != (video_ctrl & 1))
	{
		mark_all_dirty();    /* screen switch */
	}

	COMBINE_DATA(&video_ctrl);
}

READ16_HANDLER( volfied_video_ctrl_r )
{
	/* Could this be some kind of hardware collision detection? If bit 6 is
	   set the game will check for collisions with the large enemy, whereas
	   bit 5 does the same for small enemies. Bit 7 is also used although
	   its purpose is unclear. This register is usually read during a VBI
	   and stored in work RAM for later use. */

	return 0x60;
}

WRITE16_HANDLER( volfied_video_mask_w )
{
	COMBINE_DATA(&video_mask);
}

WRITE16_HANDLER( volfied_sprite_ctrl_w )
{
	PC090OJ_sprite_ctrl = (data & 0x3c) >> 2;
}


/*******************************************************
				SCREEN REFRESH
*******************************************************/

static void refresh_pixel_layer(void)
{
	int x, y;

	/*********************************************************

	VIDEO RAM has 2 screens x 256 rows x 512 columns x 16 bits

	x---------------  select image
	-x--------------  ?             (used for 3-D corners)
	--x-------------  ?             (used for 3-D walls)
	---xxxx---------  image B
	-------xxx------  palette index bits #8 to #A
	----------x-----  ?
	-----------x----  ?
	------------xxxx  image A

	*********************************************************/

	UINT16* p = video_ram;

	if (video_ctrl & 1)
	{
		p += 0x20000;
	}

	for (y = 0; y < Machine->drv->screen_height; y++)
	{
		if (line_dirty[y])
		{
			for (x = 1; x < Machine->drv->screen_width + 1; x++) /* Hmm, 1 pixel offset is needed to align properly with sprites */
			{
				int color = (p[x] << 2) & 0x700;

				if (p[x] & 0x8000)
				{
					color |= 0x800 | ((p[x] >> 9) & 0xf);

					if (p[x] & 0x2000)
					{
						color &= ~0xf;	  /* hack */
					}
				}
				else
				{
					color |= p[x] & 0xf;
				}

				plot_pixel(pixel_layer, x - 1, y, Machine->pens[color]);
			}

			line_dirty[y] = 0;
		}

		p += 512;
	}
}

VIDEO_UPDATE( volfied )
{
	fillbitmap(priority_bitmap, 0, cliprect);

	refresh_pixel_layer();

	copybitmap(bitmap, pixel_layer, 0, 0, 0, 0, cliprect, TRANSPARENCY_NONE, 0);

	PC090OJ_draw_sprites(bitmap, cliprect, 0);
}
