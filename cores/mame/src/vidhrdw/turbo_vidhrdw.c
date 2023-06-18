/*************************************************************************

	Sega Z80-3D system

*************************************************************************/

#include "driver.h"
#include "turbo.h"
#include "vidhrdw/generic.h"


/* constants */
#define VIEW_WIDTH					(32*8)
#define VIEW_HEIGHT					(28*8)
#define END_OF_ROW_VALUE			0x12345678

/* globals definitions */
UINT8 *sega_sprite_position;
UINT8 turbo_collision;

/* internal data */
static UINT8 *sprite_priority, *sprite_expanded_priority;
static UINT8 *road_gfxdata, *road_palette, *road_enable_collide;
static UINT16 *road_expanded_palette;
static UINT8 *fore_palette, *fore_priority;
static UINT16 *fore_expanded_data;
static UINT8 *back_data;
static UINT8 *overall_priority, *collision_map;

/* sprite tracking */
struct sprite_params_data
{
	UINT32 *base;
	UINT8 *enable;
	int offset, rowbytes;
	int yscale, miny, maxy;
	int xscale, xoffs;
	int flip;
};
static struct sprite_params_data sprite_params[16];
static UINT32 *sprite_expanded_data;
static UINT8 *sprite_expanded_enable;

/* misc other stuff */
static UINT8 *buckrog_bitmap_ram;
static UINT8 drew_frame;
static UINT32 sprite_mask;


/***************************************************************************

	Convert the color PROMs into a more useable format.

***************************************************************************/

PALETTE_INIT( turbo )
{
	int i;

	for (i = 0; i < 512; i++, color_prom++)
	{
		int bit0, bit1, bit2, r, g, b;

		/* bits 4,5,6 of the index are inverted before being used as addresses */
		/* to save ourselves lots of trouble, we will undo the inversion when */
		/* generating the palette */
		int adjusted_index = i ^ 0x70;

		/* red component */
		bit0 = (*color_prom >> 0) & 1;
		bit1 = (*color_prom >> 1) & 1;
		bit2 = (*color_prom >> 2) & 1;
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		/* green component */
		bit0 = (*color_prom >> 3) & 1;
		bit1 = (*color_prom >> 4) & 1;
		bit2 = (*color_prom >> 5) & 1;
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		/* blue component */
		bit0 = 0;
		bit1 = (*color_prom >> 6) & 1;
		bit2 = (*color_prom >> 7) & 1;
		b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		palette_set_color(adjusted_index,r,g,b);
	}
}


PALETTE_INIT( subroc3d )
{
	int i;

	/* Subroc3D uses a common final color PROM with 512 entries */
	for (i = 0; i < 512; i++, color_prom++)
	{
		int bit0, bit1, bit2, r, g, b;

		/* red component */
		bit0 = (*color_prom >> 0) & 1;
		bit1 = (*color_prom >> 1) & 1;
		bit2 = (*color_prom >> 2) & 1;
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		/* green component */
		bit0 = (*color_prom >> 3) & 1;
		bit1 = (*color_prom >> 4) & 1;
		bit2 = (*color_prom >> 5) & 1;
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		/* blue component */
		bit0 = 0;
		bit1 = (*color_prom >> 6) & 1;
		bit2 = (*color_prom >> 7) & 1;
		b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		palette_set_color(i,r,g,b);
	}
}


PALETTE_INIT( buckrog )
{
	int i;

	/* Buck Rogers uses 1024 entries for the sprite color PROM */
	for (i = 0; i < 1024; i++, color_prom++)
	{
		int bit0, bit1, bit2, r, g, b;

		/* red component */
		bit0 = (*color_prom >> 0) & 1;
		bit1 = (*color_prom >> 1) & 1;
		bit2 = (*color_prom >> 2) & 1;
		r = 34 * bit0 + 68 * bit1 + 137 * bit2;

		/* green component */
		bit0 = (*color_prom >> 3) & 1;
		bit1 = (*color_prom >> 4) & 1;
		bit2 = (*color_prom >> 5) & 1;
		g = 34 * bit0 + 68 * bit1 + 137 * bit2;

		/* blue component */
		bit0 = 0;
		bit1 = (*color_prom >> 6) & 1;
		bit2 = (*color_prom >> 7) & 1;
		b = 34 * bit0 + 68 * bit1 + 137 * bit2;

		palette_set_color(i,r,g,b);
	}

	/* then another 512 entries for the character color PROM */
	for (i = 0; i < 512; i++, color_prom++)
	{
		int bit0, bit1, bit2, r, g, b;

		/* red component */
		bit0 =
		bit1 = (*color_prom >> 0) & 1;
		bit2 = (*color_prom >> 1) & 1;
		r = 34 * bit0 + 68 * bit1 + 137 * bit2;

		/* green component */
		bit0 =
		bit1 = (*color_prom >> 2) & 1;
		bit2 = (*color_prom >> 3) & 1;
		g = 34 * bit0 + 68 * bit1 + 137 * bit2;

		/* blue component */
		bit0 = 0;
		bit1 = (*color_prom >> 4) & 1;
		bit2 = (*color_prom >> 5) & 1;
		b = 34 * bit0 + 68 * bit1 + 137 * bit2;

		palette_set_color(i+1024,r,g,b);
	}

	/* finally, the gradient foreground gets its own set of 256 colors */
	for (i = 0; i < 256; i++)
	{
		int bit0, bit1, bit2, bit3, r, g, b;

		/* red component */
		bit0 = 0;
		bit1 = 0;
		bit2 = (i >> 0) & 1;
		r = 34 * bit0 + 68 * bit1 + 137 * bit2;

		/* green component */
		bit0 = (i >> 1) & 1;
		bit1 = (i >> 2) & 1;
		bit2 = (i >> 3) & 1;
		g = 34 * bit0 + 68 * bit1 + 137 * bit2;

		/* blue component */
		bit0 = (i >> 4) & 1;
		bit1 = (i >> 5) & 1;
		bit2 = (i >> 6) & 1;
		bit3 = (i >> 7) & 1;
		b = 16 * bit0 + 34 * bit1 + 68 * bit2 + 137 * bit3;

		palette_set_color(i+1024+512,r,g,b);
	}
}



/***************************************************************************

	Sprite startup/shutdown

***************************************************************************/

static int init_sprites(UINT32 sprite_expand[16], UINT8 sprite_enable[16], int expand_shift)
{
	UINT8 *sprite_gfxdata = memory_region(REGION_GFX1);
	int sprite_length = memory_region_length(REGION_GFX1);
	int sprite_bank_size = sprite_length / 8;
	UINT8 *src, *edst;
	UINT32 *dst;
	int i, j;

	/* allocate the expanded sprite data */
	sprite_expanded_data = auto_malloc(sprite_length * 2 * sizeof(UINT32));
	if (!sprite_expanded_data)
		return 1;

	/* allocate the expanded sprite enable array */
	sprite_expanded_enable = auto_malloc(sprite_length * 2 * sizeof(UINT8));
	if (!sprite_expanded_enable)
		return 1;

	/* expand the sprite ROMs */
	src = sprite_gfxdata;
	dst = sprite_expanded_data;
	edst = sprite_expanded_enable;
	for (i = 0; i < 8; i++)
	{
		/* expand this bank */
		for (j = 0; j < sprite_bank_size; j++)
		{
			int bits = *src++;
			*dst++ = sprite_expand[bits >> 4];
			*dst++ = sprite_expand[bits & 15];
			*edst++ = sprite_enable[bits >> 4];
			*edst++ = sprite_enable[bits & 15];
		}

		/* shift for the next bank */
		for (j = 0; j < 16; j++)
		{
			if (sprite_expand[j] != END_OF_ROW_VALUE)
				sprite_expand[j] <<= expand_shift;
			sprite_enable[j] <<= 1;
		}
	}

	/* success */
	return 0;
}



/***************************************************************************

	Foreground startup/shutdown

***************************************************************************/

static int init_fore(void)
{
	UINT8 *fore_gfxdata = memory_region(REGION_GFX2);
	int fore_length = memory_region_length(REGION_GFX2);
	UINT16 *dst;
	UINT8 *src;
	int i, j;

	/* allocate the expanded foreground data */
	fore_expanded_data = auto_malloc(fore_length);
	if (!fore_expanded_data)
		return 1;

	/* expand the foreground ROMs */
	src = fore_gfxdata;
	dst = fore_expanded_data;
	for (i = 0; i < fore_length / 2; i++, src++)
	{
		int bits1 = src[0];
		int bits2 = src[fore_length / 2];
		int newbits = 0;

		for (j = 0; j < 8; j++)
		{
			newbits |= ((bits1 >> (j ^ 7)) & 1) << (j * 2);
			newbits |= ((bits2 >> (j ^ 7)) & 1) << (j * 2 + 1);
		}
		*dst++ = newbits;
	}

	return 0;
}



/***************************************************************************

	Video startup

***************************************************************************/

VIDEO_START( turbo )
{
	UINT32 sprite_expand[16];
	UINT8 sprite_enable[16];
	UINT16 *dst;
	UINT8 *src;
	int i;

	/* determine ROM/PROM addresses */
	sprite_priority = memory_region(REGION_PROMS) + 0x0200;

	road_gfxdata = memory_region(REGION_GFX3);
	road_palette = memory_region(REGION_PROMS) + 0x0b00;
	road_enable_collide = memory_region(REGION_PROMS) + 0x0b40;

	fore_palette = memory_region(REGION_PROMS) + 0x0a00;

	overall_priority = memory_region(REGION_PROMS) + 0x0600;
	collision_map = memory_region(REGION_PROMS) + 0x0b60;

	/* compute the sprite expansion array */
	for (i = 0; i < 16; i++)
	{
		UINT32 value = 0;
		if (i & 1) value |= 0x00000001;
		if (i & 2) value |= 0x00000100;
		if (i & 4) value |= 0x00010000;

		/* special value for the end-of-row */
		if ((i & 0x0c) == 0x04) value = END_OF_ROW_VALUE;

		sprite_expand[i] = value;
		sprite_enable[i] = (i >> 3) & 1;
	}

	/* initialize the sprite data */
	if (init_sprites(sprite_expand, sprite_enable, 1))
		return 1;

	/* initialize the fore data */
	if (init_fore())
		return 1;

	/* allocate the expanded road palette */
	road_expanded_palette = auto_malloc(0x40 * sizeof(UINT16));
	if (!road_expanded_palette)
		return 1;

	/* expand the road palette */
	src = road_palette;
	dst = road_expanded_palette;
	for (i = 0; i < 0x20; i++, src++)
		*dst++ = src[0] | (src[0x20] << 8);

	/* other stuff */
	drew_frame = 0;
	sprite_mask = 0x7fff;

	/* return success */
	return 0;
}


VIDEO_START( subroc3d )
{
	UINT32 sprite_expand[16];
	UINT8 sprite_enable[16];
	int i;

	/* determine ROM/PROM addresses */
	sprite_priority = memory_region(REGION_PROMS) + 0x0500;
	fore_palette = memory_region(REGION_PROMS) + 0x0200;

	/* compute the sprite expansion array */
	for (i = 0; i < 16; i++)
	{
		sprite_expand[i] = (i == 0x03 || i == 0x0f) ? END_OF_ROW_VALUE : i;
		sprite_enable[i] = (i == 0x00 || i == 0x03 || i == 0x0c || i == 0x0f) ? 0 : 1;
	}

	/* initialize the sprite data */
	if (init_sprites(sprite_expand, sprite_enable, 4))
		return 1;

	/* initialize the fore data */
	if (init_fore())
		return 1;

	/* allocate the expanded sprite priority map */
	sprite_expanded_priority = auto_malloc(1 << 12);
	if (!sprite_expanded_priority)
		return 1;

	/* expand the sprite priority map */
	for (i = 0; i < (1 << 12); i++)
	{
		int plb = ~i & 0xff;
		int ply = i >> 8;
		int byteval = sprite_priority[(plb | ((ply & 0x0e) << 7)) & 0x1ff];
		sprite_expanded_priority[i] = (ply & 1) ? (byteval >> 4) : (byteval & 0x0f);
		sprite_expanded_priority[i] *= 4;
	}

	/* other stuff */
	sprite_mask = 0xffff;

	/* return success */
	return 0;
}


VIDEO_START( buckrog )
{
	UINT32 sprite_expand[16];
	UINT8 sprite_enable[16];
	int i;

	/* determine ROM/PROM addresses */
	fore_priority = memory_region(REGION_PROMS) + 0x400;
	back_data = memory_region(REGION_GFX3);

	/* compute the sprite expansion array */
	for (i = 0; i < 16; i++)
	{
		sprite_expand[i] = (i == 0x0f) ? END_OF_ROW_VALUE : i;
		sprite_enable[i] = (i == 0x00 || i == 0x0f) ? 0 : 1;
	}

	/* initialize the sprite data */
	if (init_sprites(sprite_expand, sprite_enable, 4))
		return 1;

	/* initialize the fore data */
	if (init_fore())
		return 1;

	/* allocate the expanded sprite priority map */
	sprite_expanded_priority = auto_malloc(1 << 8);
	if (!sprite_expanded_priority)
		return 1;

	/* expand the sprite priority map */
	for (i = 0; i < (1 << 8); i++)
	{
		if (i & 0x01) sprite_expanded_priority[i] = 0 | 8;
		else if (i & 0x02) sprite_expanded_priority[i] = 1 | 8;
		else if (i & 0x04) sprite_expanded_priority[i] = 2 | 8;
		else if (i & 0x08) sprite_expanded_priority[i] = 3 | 8;
		else if (i & 0x10) sprite_expanded_priority[i] = 4 | 8;
		else if (i & 0x20) sprite_expanded_priority[i] = 5 | 8;
		else if (i & 0x40) sprite_expanded_priority[i] = 6 | 8;
		else if (i & 0x80) sprite_expanded_priority[i] = 7 | 8;
		else sprite_expanded_priority[i] = 0;
		sprite_expanded_priority[i] *= 4;
	}

	/* allocate the bitmap RAM */
	buckrog_bitmap_ram = auto_malloc(0xe000);
	if (!buckrog_bitmap_ram)
		return 1;

	/* other stuff */
	sprite_mask = 0xffff;

	/* return success */
	return 0;
}



/***************************************************************************

	Sprite data gathering

***************************************************************************/

static void turbo_update_sprite_info(void)
{
	struct sprite_params_data *data = sprite_params;
	int i;

	/* first loop over all sprites and update those whose scanlines intersect ours */
	for (i = 0; i < 16; i++, data++)
	{
		UINT8 *sprite_base = spriteram + 16 * i;

		/* snarf all the data */
		data->base = sprite_expanded_data + (i & 7) * 0x8000;
		data->enable = sprite_expanded_enable + (i & 7) * 0x8000;
		data->offset = (sprite_base[6] + 256 * sprite_base[7]) & sprite_mask;
		data->rowbytes = (INT16)(sprite_base[4] + 256 * sprite_base[5]);
		data->miny = sprite_base[0];
		data->maxy = sprite_base[1];
		data->xscale = ((5 * 256 - 4 * sprite_base[2]) << 16) / (5 * 256);
		data->yscale = (4 << 16) / (sprite_base[3] + 4);
		data->xoffs = -1;
		data->flip = 0;
	}

	/* now find the X positions */
	for (i = 0; i < 0x200; i++)
	{
		int value = sega_sprite_position[i];
		if (value)
		{
			int base = (i & 0x100) >> 5;
			int which;
			for (which = 0; which < 8; which++)
				if (value & (1 << which))
					sprite_params[base + which].xoffs = i & 0xff;
		}
	}
}


static void subroc3d_update_sprite_info(void)
{
	struct sprite_params_data *data = sprite_params;
	int i;

	/* first loop over all sprites and update those whose scanlines intersect ours */
	for (i = 0; i < 16; i++, data++)
	{
		UINT8 *sprite_base = spriteram + 8 * i;

		/* snarf all the data */
		data->base = sprite_expanded_data + (i & 7) * 0x10000;
		data->enable = sprite_expanded_enable + (i & 7) * 0x10000;
		data->offset = ((sprite_base[6] + 256 * sprite_base[7]) * 2) & sprite_mask;
		data->rowbytes = (INT16)(sprite_base[4] + 256 * sprite_base[5]) * 2;
		data->miny = sprite_base[0] ^ 0xff;
		data->maxy = (sprite_base[1] ^ 0xff) - 1;
		data->xscale = 65536.0 * (1.0 - 0.004 * (double)(sprite_base[2] - 0x40));
		data->yscale = (4 << 16) / (sprite_base[3] + 4);
		data->xoffs = -1;
		data->flip = sprite_base[7]>>7;
	}

	/* now find the X positions */
	for (i = 0; i < 0x200; i++)
	{
		int value = sega_sprite_position[i];
		if (value)
		{
			int base = (i & 0x01) << 3;
			int which;
			for (which = 0; which < 8; which++)
				if (value & (1 << which))
					sprite_params[base + which].xoffs = ((i & 0x1fe) >> 1);
		}
	}
}



/***************************************************************************

	Sprite rendering

***************************************************************************/

static void draw_one_sprite(const struct sprite_params_data *data, UINT32 *dest, UINT8 *edest, int xclip, int scanline)
{
	int xstep = data->flip ? -data->xscale : data->xscale;
	int xoffs = data->xoffs;
	UINT32 xcurr;
	UINT32 *src;
	UINT8 *esrc;
	int offset;

	/* xoffs of -1 means don't draw */
	if (xoffs == -1 || data->xscale <= 0) return;

	/* compute the current data offset */
	scanline = ((scanline - data->miny) * data->yscale) >> 16;
	offset = data->offset + (scanline + 1) * data->rowbytes;

	/* clip to the road */
	xcurr = offset << 16;
	if (xoffs < xclip)
	{
		/* the pixel clock starts on xoffs regardless of clipping; take this into account */
		xcurr += ((xclip - xoffs) * xstep) & 0xffff;
		xoffs = xclip;
	}

	/* determine the bitmap location */
	src = data->base;
	esrc = data->enable;

	/* two cases: easy case is with xstep <= 0x10000 */
	if (xstep >= -0x10000 && xstep <= 0x10000)
	{
		/* loop over columns */
		while (xoffs < VIEW_WIDTH)
		{
			UINT32 srcval = src[(xcurr >> 16) & sprite_mask];
			UINT8 srcenable = esrc[(xcurr >> 16) & sprite_mask];

			/* stop on the end-of-row signal */
			if (srcval == END_OF_ROW_VALUE)
				break;

			/* OR in the bits from this pixel */
			dest[xoffs] |= srcval;
			edest[xoffs++] |= srcenable;
			xcurr += xstep;
		}
	}

	/* otherwise, we need to make sure we don't skip the end of row */
	else
	{
		int xdir = (xstep < 0) ? -1 : 1;

		/* loop over columns */
		while (xoffs < VIEW_WIDTH)
		{
			int xint = (xcurr >> 16) & sprite_mask, newxint;
			UINT32 srcval = src[xint];
			UINT8 srcenable = esrc[xint];

			/* stop on the end-of-row signal */
			if (srcval == END_OF_ROW_VALUE)
				break;

			/* OR in the bits from this pixel */
			dest[xoffs] |= srcval;
			edest[xoffs++] |= srcenable;
			xcurr += xstep;

			/* make sure we don't hit any end of rows along the way */
			newxint = (xcurr >> 16) & sprite_mask;
			while ((xint = (xint + xdir) & sprite_mask) != newxint)
				if (src[xint] == END_OF_ROW_VALUE)
					break;
		}
	}
}


static void draw_sprites(UINT32 *dest, UINT8 *edest, int scanline, UINT8 mask, int xclip)
{
	int i;

	for (i = 0; i < 8; i++)
	{
		const struct sprite_params_data *data;

		/* check the mask */
		if (mask & (1 << i))
		{
			/* if the sprite intersects this scanline, draw it */
			data = &sprite_params[i];
			if (scanline >= data->miny && scanline < data->maxy)
				draw_one_sprite(data, dest, edest, xclip, scanline);

			/* if the sprite intersects this scanline, draw it */
			data = &sprite_params[8 + i];
			if (scanline >= data->miny && scanline < data->maxy)
				draw_one_sprite(data, dest, edest, xclip, scanline);
		}
	}
}



/***************************************************************************

	Core drawing routines

***************************************************************************/

static void turbo_render(struct mame_bitmap *bitmap)
{
	UINT8 *overall_priority_base = &overall_priority[(turbo_fbpla & 8) << 6];
	UINT8 *sprite_priority_base = &sprite_priority[(turbo_fbpla & 7) << 7];
	UINT8 *road_gfxdata_base = &road_gfxdata[(turbo_opc << 5) & 0x7e0];
	UINT16 *road_palette_base = &road_expanded_palette[(turbo_fbcol & 1) << 4];
	pen_t *colortable;
	int x, y, i;

	/* suck up the sprite parameter data */
	turbo_update_sprite_info();

	/* determine the color offset */
	colortable = &Machine->pens[(turbo_fbcol & 6) << 6];

	/* loop over rows */
	for (y = 4; y < VIEW_HEIGHT - 4; y++)
	{
		int sel, coch, babit, slipar_acciar, area, area1, area2, area3, area4, area5, road = 0;
		UINT32 sprite_buffer[VIEW_WIDTH];
		UINT8 sprite_enable[VIEW_WIDTH];
		UINT8 scanline[VIEW_WIDTH];

		/* compute the Y sum between opa and the current scanline (p. 141) */
		int va = (y + turbo_opa) & 0xff;

		/* the upper bit of OPC inverts the road */
		if (!(turbo_opc & 0x80)) va ^= 0xff;

		/* clear the sprite buffer and draw the road sprites */
		memset(sprite_buffer, 0, VIEW_WIDTH * sizeof(UINT32));
		memset(sprite_enable, 0, VIEW_WIDTH * sizeof(UINT8));
		draw_sprites(sprite_buffer, sprite_enable, y, 0x07, 0);

		/* loop over 8-pixel chunks */
		for (x = 8; x < VIEW_WIDTH; x += 8)
		{
			int area5_buffer = road_gfxdata_base[0x4000 + (x >> 3)];
			UINT8 fore_data = videoram[(y / 8) * 32 + (x / 8) - 33];
			UINT16 forebits_buffer = fore_expanded_data[(fore_data << 3) | (y & 7)];

			/* loop over columns */
			for (i = 0; i < 8; i++)
			{
				UINT32 sprite = sprite_buffer[x + i];
				UINT8 enable = sprite_enable[x + i];

				/* compute the X sum between opb and the current column; only the carry matters (p. 141) */
				int carry = (x + i + turbo_opb) >> 8;

				/* the carry selects which inputs to use (p. 141) */
				if (carry)
				{
					sel	 = turbo_ipb;
					coch = turbo_ipc >> 4;
				}
				else
				{
					sel	 = turbo_ipa;
					coch = turbo_ipc & 15;
				}

				/* at this point we also compute area5 (p. 141) */
				area5 = (area5_buffer >> 3) & 0x10;
				area5_buffer <<= 1;

				/* now look up the rest of the road bits (p. 142) */
				area1 = road_gfxdata[0x0000 | ((sel & 15) << 8) | va];
				area1 = ((area1 + x + i) >> 8) & 0x01;
				area2 = road_gfxdata[0x1000 | ((sel & 15) << 8) | va];
				area2 = ((area2 + x + i) >> 7) & 0x02;
				area3 = road_gfxdata[0x2000 | ((sel >> 4) << 8) | va];
				area3 = ((area3 + x + i) >> 6) & 0x04;
				area4 = road_gfxdata[0x3000 | ((sel >> 4) << 8) | va];
				area4 = ((area4 + x + i) >> 5) & 0x08;

				/* compute the final area value and look it up in IC18/PR1115 (p. 144) */
				area = area5 | area4 | area3 | area2 | area1;
				babit = road_enable_collide[area] & 0x07;

				/* note: SLIPAR is 0 on the road surface only */
				/*		 ACCIAR is 0 on the road surface and the striped edges only */
				slipar_acciar = road_enable_collide[area] & 0x30;
				if (!road && (slipar_acciar & 0x20))
				{
					road = 1;
					draw_sprites(sprite_buffer, sprite_enable, y, 0xf8, x + i + 2);
				}

				/* perform collision detection here */
				turbo_collision |= collision_map[(enable & 7) | (slipar_acciar >> 1)];

				/* we only need to continue if we're actually drawing */
				if (bitmap)
				{
					int bacol, red, grn, blu, priority, forebits, mx;

					/* also use the coch value to look up color info in IC13/PR1114 and IC21/PR1117 (p. 144) */
					bacol = road_palette_base[coch & 15];

					/* at this point, do the character lookup */
					forebits = forebits_buffer & 3;
					forebits_buffer >>= 2;
					forebits = fore_palette[forebits | (fore_data & 0xfc)];

					/* look up the sprite priority in IC11/PR1122 */
					priority = sprite_priority_base[enable >> 1];

					/* use that to look up the overall priority in IC12/PR1123 */
					mx = overall_priority_base[(priority & 7) | ((enable << 3) & 8) | ((fore_data >> 3) & 0x10) | ((forebits << 2) & 0x20) | (babit << 6)];

					/* the input colors consist of a mix of sprite, road and 1's & 0's */
					red = 0x040000 | ((bacol & 0x001f) << 13) | ((forebits & 1) << 12) | ((sprite <<  4) & 0x0ff0);
					grn = 0x080000 | ((bacol & 0x03e0) <<  9) | ((forebits & 2) << 12) | ((sprite >>  3) & 0x1fe0);
					blu = 0x100000 | ((bacol & 0x7c00) <<  5) | ((forebits & 4) << 12) | ((sprite >> 10) & 0x3fc0);

					/* we then go through a muxer; normally these values are inverted, but */
					/* we've already taken care of that when we generated the palette */
					red = (red >> mx) & 0x10;
					grn = (grn >> mx) & 0x20;
					blu = (blu >> mx) & 0x40;
					scanline[x + i] = mx | red | grn | blu;
				}
			}
		}

		/* render the scanline */
		if (bitmap)
			draw_scanline8(bitmap, 8, y, VIEW_WIDTH - 8, &scanline[8], colortable, -1);
	}
}


static void subroc3d_render(struct mame_bitmap *bitmap)
{
	UINT8 *sprite_priority_base = &sprite_expanded_priority[(subroc3d_ply & 15) << 8];
	pen_t *colortable;
	int y;

	/* suck up the sprite parameter data */
	subroc3d_update_sprite_info();

	/* determine the color offset */
	colortable = &Machine->pens[(subroc3d_col & 15) << 5];

	/* loop over rows */
	for (y = 0; y < VIEW_HEIGHT; y++)
	{
		UINT32 sprite_buffer[VIEW_WIDTH];
		UINT8 sprite_enable[VIEW_WIDTH];
		UINT8 scanline[VIEW_WIDTH];
		int x;

		/* clear the sprite buffer and draw the road sprites */
		memset(sprite_buffer, 0, VIEW_WIDTH * sizeof(UINT32));
		memset(sprite_enable, 0, VIEW_WIDTH * sizeof(UINT8));
		draw_sprites(sprite_buffer, sprite_enable, y, 0xff, 0);

		/* loop over 8-pixel chunks */
		for (x = 0; x < VIEW_WIDTH; x += 8)
		{
			UINT8 fore_data = videoram[(y / 8) * 32 + (((x / 8) + subroc3d_chofs) % 32)];
			UINT16 forebits_buffer = fore_expanded_data[(fore_data << 3) | (y & 7)];
			int i;

			/* loop over columns */
			for (i = 0; i < 8; i++)
			{
				int bits, forebits, mux, mplb;

				/* at this point, do the character lookup */
				forebits = forebits_buffer & 3;
				forebits_buffer >>= 2;
				forebits = fore_palette[forebits | (fore_data & 0xfc)] & 0x0f;

				/* determine the value of mplb */
				mplb = (forebits == 0 || (fore_data & 0x80));

				/* look up the sprite priority in IC11/PR1122 */
				mux = mplb ? sprite_priority_base[sprite_enable[x + i]] : 0;

				/* mux3 selects either sprite or foreground */
				if (mux & 0x20)
					bits = (sprite_buffer[x + i] >> (mux & 0x1c)) & 0x0f;
				else
					bits = forebits;

				scanline[x + i] = ((mux & 0x20) >> 1) | bits;
			}
		}

		/* render the scanline */
		draw_scanline8(bitmap, 0, y, VIEW_WIDTH, scanline, colortable, -1);
	}
}


static void buckrog_render(struct mame_bitmap *bitmap)
{
	int y;

	/* suck up the sprite parameter data */
	subroc3d_update_sprite_info();

	/* loop over rows */
	for (y = 0; y < VIEW_HEIGHT; y++)
	{
		UINT32 sprite_buffer[VIEW_WIDTH];
		UINT8 sprite_enable[VIEW_WIDTH];
		UINT16 scanline[VIEW_WIDTH];
		int bgcolor;
		int x;

		/* determine background color for this scanline */
		bgcolor = 1024 | 512 | back_data[(buckrog_mov << 8) | y];

		/* clear the sprite buffer and draw the road sprites */
		memset(sprite_buffer, 0, VIEW_WIDTH * sizeof(UINT32));
		memset(sprite_enable, 0, VIEW_WIDTH * sizeof(UINT8));
		draw_sprites(sprite_buffer, sprite_enable, y, 0xff, 0);

		/* loop over 8-pixel chunks */
		for (x = 0; x < VIEW_WIDTH; x += 8)
		{
			UINT8 fore_data = videoram[(y / 8) * 32 + (x / 8)];
			UINT16 forebits_buffer = fore_expanded_data[(fore_data << 3) | (y & 7)];
			UINT16 forebits_upper = ((buckrog_fchg << 7) & 0x180) | ((fore_data >> 1) & 0x7c);
			UINT8 *stars = &buckrog_bitmap_ram[y * 256];
			int i;

			/* loop over columns */
			for (i = 0; i < 8; i++)
			{
				int bits, forebits, forepri, mux;

				/* at this point, do the character lookup */
				forebits = (forebits_buffer & 3) | forebits_upper;
				forebits_buffer >>= 2;

				/* look up the foreground priority */
				forepri = fore_priority[forebits];

				/* look up the sprite priority in IC11/PR1122 */
				mux = sprite_expanded_priority[sprite_enable[x + i]];

				/* final result is based on sprite/foreground/star priorities */
				if (!(forepri & 0x80))
					bits = 1024 | forebits;
				else if (mux & 0x20)
					bits = (buckrog_obch << 7) | ((mux & 0x1c) << 2) | ((sprite_buffer[x + i] >> (mux & 0x1c)) & 0x0f);
				else if (!(forepri & 0x40))
					bits = 1024 | forebits;
				else if (stars[x + i])
					bits = 1024 | 512 | 255;
				else
					bits = bgcolor;

				scanline[x + i] = bits;
			}
		}

		/* render the scanline */
		draw_scanline16(bitmap, 0, y, VIEW_WIDTH, scanline, Machine->pens, -1);
	}
}



/***************************************************************************

	Main refresh

***************************************************************************/

VIDEO_EOF( turbo )
{
	/* only do collision checking if we didn't draw */
	if (!drew_frame)
		turbo_render(NULL);
	drew_frame = 0;
}


VIDEO_UPDATE( turbo )
{
	/* perform the actual drawing */
	turbo_render(bitmap);

	/* draw the LEDs for the scores */
	turbo_update_segments();

	/* indicate that we drew this frame, so that the eof callback doesn't bother doing anything */
	drew_frame = 1;
}


VIDEO_UPDATE( subroc3d )
{
	/* perform the actual drawing */
	subroc3d_render(bitmap);

	/* draw the LEDs for the scores */
	turbo_update_segments();
}


VIDEO_UPDATE( buckrog )
{
	/* perform the actual drawing */
	buckrog_render(bitmap);

	/* draw the LEDs for the scores */
	turbo_update_segments();
}



/***************************************************************************

	Buck Rogers misc

***************************************************************************/

WRITE_HANDLER( buckrog_bitmap_w )
{
	buckrog_bitmap_ram[offset] = data & 1;
}
