#include "driver.h"
#include "cpu/m6809/m6809.h"
#include "machine/6522via.h"
#include "vidhrdw/generic.h"

static int scanline=0;

INTERRUPT_GEN( beezer_interrupt )
{
	scanline = (scanline + 1) % 0x80;
	via_0_ca2_w (0, scanline & 0x10);
	if ((scanline & 0x78) == 0x78)
		cpu_set_irq_line(0, M6809_FIRQ_LINE, ASSERT_LINE);
	else
		cpu_set_irq_line(0, M6809_FIRQ_LINE, CLEAR_LINE);
}

VIDEO_UPDATE( beezer )
{
	int x, y;

	if (get_vh_global_attribute_changed())
		for (y = Machine->visible_area.min_y; y <= Machine->visible_area.max_y; y+=2)
		{
			for (x = Machine->visible_area.min_x; x <= Machine->visible_area.max_x; x++)
			{
				plot_pixel (tmpbitmap, x, y+1, Machine->pens[videoram[0x80*y+x] & 0x0f]);
				plot_pixel (tmpbitmap, x, y, Machine->pens[(videoram[0x80*y+x] >> 4)& 0x0f]);
			}
		}
	
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);
}

WRITE_HANDLER( beezer_map_w )
{
	/*
	  bit 7 -- 330  ohm resistor  -- BLUE
	        -- 560  ohm resistor  -- BLUE
			-- 330	ohm resistor  -- GREEN
			-- 560	ohm resistor  -- GREEN
			-- 1.2 kohm resistor  -- GREEN
			-- 330	ohm resistor  -- RED
			-- 560	ohm resistor  -- RED
	  bit 0 -- 1.2 kohm resistor  -- RED
	*/

	int r, g, b, bit0, bit1, bit2;;

	/* red component */
	bit0 = (data >> 0) & 0x01;
	bit1 = (data >> 1) & 0x01;
	bit2 = (data >> 2) & 0x01;
	r = 0x26 * bit0 + 0x50 * bit1 + 0x89 * bit2;
	/* green component */
	bit0 = (data >> 3) & 0x01;
	bit1 = (data >> 4) & 0x01;
	bit2 = (data >> 5) & 0x01;
	g = 0x26 * bit0 + 0x50 * bit1 + 0x89 * bit2;
	/* blue component */
	bit0 = (data >> 6) & 0x01;
	bit1 = (data >> 7) & 0x01;
	b = 0x5f * bit0 + 0xa0 * bit1;

	palette_set_color(offset, r, g, b);
}

WRITE_HANDLER( beezer_ram_w )
{
	int x, y;
	x = offset % 0x100;
	y = (offset / 0x100) * 2;

	if( (y >= Machine->visible_area.min_y) && (y <= Machine->visible_area.max_y) )
	{
		plot_pixel (tmpbitmap, x, y+1, Machine->pens[data & 0x0f]);
		plot_pixel (tmpbitmap, x, y, Machine->pens[(data >> 4)& 0x0f]);
	}

	videoram[offset] = data;
}

READ_HANDLER( beezer_line_r )
{
	return (scanline & 0xfe) << 1;
}
