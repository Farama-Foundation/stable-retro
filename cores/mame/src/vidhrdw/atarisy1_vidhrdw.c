/***************************************************************************

	Atari System 1 hardware

****************************************************************************/

#include "driver.h"
#include "machine/atarigen.h"
#include "atarisy1.h"



/*************************************
 *
 *	Constants
 *
 *************************************/

/* the color and remap PROMs are mapped as follows */
#define PROM1_BANK_4			0x80		/* active low */
#define PROM1_BANK_3			0x40		/* active low */
#define PROM1_BANK_2			0x20		/* active low */
#define PROM1_BANK_1			0x10		/* active low */
#define PROM1_OFFSET_MASK		0x0f		/* postive logic */

#define PROM2_BANK_6_OR_7		0x80		/* active low */
#define PROM2_BANK_5			0x40		/* active low */
#define PROM2_PLANE_5_ENABLE	0x20		/* active high */
#define PROM2_PLANE_4_ENABLE	0x10		/* active high */
#define PROM2_PF_COLOR_MASK		0x0f		/* negative logic */
#define PROM2_BANK_7			0x08		/* active low, plus PROM2_BANK_6_OR_7 low as well */
#define PROM2_MO_COLOR_MASK		0x07		/* negative logic */



/*************************************
 *
 *	Globals we own
 *
 *************************************/

data16_t *atarisy1_bankselect;



/*************************************
 *
 *	Statics
 *
 *************************************/

/* playfield parameters */
static UINT16 playfield_lookup[256];
static UINT8 playfield_tile_bank;
static data16_t playfield_priority_pens;
static void *yscroll_reset_timer;

/* INT3 tracking */
static int next_timer_scanline;
static void *scanline_timer;
static void *int3off_timer;

/* graphics bank tracking */
static UINT8 bank_gfx[3][8];
static UINT8 bank_color_shift[MAX_GFX_ELEMENTS];

/* basic form of a graphics bank */
static struct GfxLayout objlayout =
{
	8,8,	/* 8*8 sprites */
	4096,	/* 4096 of them */
	6,		/* 6 bits per pixel */
	{ 5*8*0x08000, 4*8*0x08000, 3*8*0x08000, 2*8*0x08000, 1*8*0x08000, 0*8*0x08000 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8		/* every sprite takes 8 consecutive bytes */
};



/*************************************
 *
 *	Prototypes
 *
 *************************************/

static void update_timers(int scanline);
static int decode_gfx(UINT16 *pflookup, UINT16 *molookup);
static int get_bank(UINT8 prom1, UINT8 prom2, int bpp);
static void int3_callback(int scanline);
static void int3off_callback(int param);
static void reset_yscroll_callback(int param);



/*************************************
 *
 *	Tilemap callbacks
 *
 *************************************/

static void get_alpha_tile_info(int tile_index)
{
	UINT16 data = atarigen_alpha[tile_index];
	int code = data & 0x3ff;
	int color = (data >> 10) & 0x07;
	int opaque = data & 0x2000;
	SET_TILE_INFO(0, code, color, opaque ? TILE_IGNORE_TRANSPARENCY : 0);
}


static void get_playfield_tile_info(int tile_index)
{
	UINT16 data = atarigen_playfield[tile_index];
	UINT16 lookup = playfield_lookup[((data >> 8) & 0x7f) | (playfield_tile_bank << 7)];
	int gfxindex = (lookup >> 8) & 15;
	int code = ((lookup & 0xff) << 8) | (data & 0xff);
	int color = 0x20 + (((lookup >> 12) & 15) << bank_color_shift[gfxindex]);
	SET_TILE_INFO(gfxindex, code, color, (data >> 15) & 1);
}



/*************************************
 *
 *	Generic video system start
 *
 *************************************/

VIDEO_START( atarisy1 )
{
	static const struct atarimo_desc modesc =
	{
		0,					/* index to which gfx system */
		8,					/* number of motion object banks */
		1,					/* are the entries linked? */
		1,					/* are the entries split? */
		0,					/* render in reverse order? */
		0,					/* render in swapped X/Y order? */
		0,					/* does the neighbor bit affect the next object? */
		0,					/* pixels per SLIP entry (0 for no-slip) */
		0,					/* pixel offset for SLIPs */
		0x38,				/* maximum number of links to visit/scanline (0=all) */

		0x100,				/* base palette entry */
		0x100,				/* maximum number of colors */
		0,					/* transparent pen index */

		{{ 0,0,0,0x003f }},	/* mask for the link */
		{{ 0,0xff00,0,0 }},	/* mask for the graphics bank */
		{{ 0,0xffff,0,0 }},	/* mask for the code index */
		{{ 0 }},			/* mask for the upper code index */
		{{ 0,0xff00,0,0 }},	/* mask for the color */
		{{ 0,0,0x3fe0,0 }},	/* mask for the X position */
		{{ 0x3fe0,0,0,0 }},	/* mask for the Y position */
		{{ 0 }},			/* mask for the width, in tiles*/
		{{ 0x000f,0,0,0 }},	/* mask for the height, in tiles */
		{{ 0x8000,0,0,0 }},	/* mask for the horizontal flip */
		{{ 0 }},			/* mask for the vertical flip */
		{{ 0,0,0x8000,0 }},	/* mask for the priority */
		{{ 0 }},			/* mask for the neighbor */
		{{ 0 }},			/* mask for absolute coordinates */

		{{ 0,0xffff,0,0 }},	/* mask for the special value */
		0xffff,				/* resulting value to indicate "special" */
		0					/* callback routine for special entries */
	};

	UINT16 motable[256];
	UINT16 *codelookup;
	UINT8 *colorlookup, *gfxlookup;
	int i, size;

	/* first decode the graphics */
	if (!decode_gfx(playfield_lookup, motable))
		return 1;

	/* initialize the playfield */
	atarigen_playfield_tilemap = tilemap_create(get_playfield_tile_info, tilemap_scan_rows, TILEMAP_OPAQUE, 8,8, 64,64);
	if (!atarigen_playfield_tilemap)
		return 1;

	/* initialize the motion objects */
	if (!atarimo_init(0, &modesc))
		return 1;

	/* initialize the alphanumerics */
	atarigen_alpha_tilemap = tilemap_create(get_alpha_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8,8, 64,32);
	if (!atarigen_alpha_tilemap)
		return 1;
	tilemap_set_transparent_pen(atarigen_alpha_tilemap, 0);

	/* modify the motion object code lookup */
	codelookup = atarimo_get_code_lookup(0, &size);
	for (i = 0; i < size; i++)
		codelookup[i] = (i & 0xff) | ((motable[i >> 8] & 0xff) << 8);

	/* modify the motion object color and gfx lookups */
	colorlookup = atarimo_get_color_lookup(0, &size);
	gfxlookup = atarimo_get_gfx_lookup(0, &size);
	for (i = 0; i < size; i++)
	{
		colorlookup[i] = ((motable[i] >> 12) & 15) << 1;
		gfxlookup[i] = (motable[i] >> 8) & 15;
	}

	/* reset the statics */
	atarimo_set_yscroll(0, 256);
	next_timer_scanline = -1;
	scanline_timer = timer_alloc(int3_callback);
	int3off_timer = timer_alloc(int3off_callback);
	yscroll_reset_timer = timer_alloc(reset_yscroll_callback);
	return 0;
}



/*************************************
 *
 *	Graphics bank selection
 *
 *************************************/

WRITE16_HANDLER( atarisy1_bankselect_w )
{
	data16_t oldselect = *atarisy1_bankselect;
	data16_t newselect = oldselect, diff;
	int scanline = cpu_getscanline();

	/* update memory */
	COMBINE_DATA(&newselect);
	diff = oldselect ^ newselect;

	/* sound CPU reset */
	if (diff & 0x0080)
	{
		cpu_set_reset_line(1, (newselect & 0x0080) ? CLEAR_LINE : ASSERT_LINE);
		if (!(newselect & 0x0080)) atarigen_sound_reset();
	}

	/* if MO or playfield banks change, force a partial update */
	if (diff & 0x003c)
		force_partial_update(scanline);

	/* motion object bank select */
	atarimo_set_bank(0, (newselect >> 3) & 7);
	update_timers(scanline);

	/* playfield bank select */
	if (diff & 0x0004)
	{
		playfield_tile_bank = (newselect >> 2) & 1;
		tilemap_mark_all_tiles_dirty(atarigen_playfield_tilemap);
	}

	/* stash the new value */
	*atarisy1_bankselect = newselect;
}



/*************************************
 *
 *	Playfield priority pens
 *
 *************************************/

WRITE16_HANDLER( atarisy1_priority_w )
{
	data16_t oldpens = playfield_priority_pens;
	data16_t newpens = oldpens;

	/* force a partial update in case this changes mid-screen */
	COMBINE_DATA(&newpens);
	if (oldpens != newpens)
		force_partial_update(cpu_getscanline());
	playfield_priority_pens = newpens;
}



/*************************************
 *
 *	Playfield horizontal scroll
 *
 *************************************/

WRITE16_HANDLER( atarisy1_xscroll_w )
{
	data16_t oldscroll = *atarigen_xscroll;
	data16_t newscroll = oldscroll;

	/* force a partial update in case this changes mid-screen */
	COMBINE_DATA(&newscroll);
	if (oldscroll != newscroll)
		force_partial_update(cpu_getscanline());

	/* set the new scroll value */
	tilemap_set_scrollx(atarigen_playfield_tilemap, 0, newscroll);

	/* update the data */
	*atarigen_xscroll = newscroll;
}



/*************************************
 *
 *	Playfield vertical scroll
 *
 *************************************/

static void reset_yscroll_callback(int newscroll)
{
	tilemap_set_scrolly(atarigen_playfield_tilemap, 0, newscroll);
}


WRITE16_HANDLER( atarisy1_yscroll_w )
{
	data16_t oldscroll = *atarigen_yscroll;
	data16_t newscroll = oldscroll;
	int scanline = cpu_getscanline();
	int adjusted_scroll;

	/* force a partial update in case this changes mid-screen */
	COMBINE_DATA(&newscroll);
	force_partial_update(scanline);

	/* because this latches a new value into the scroll base,
	   we need to adjust for the scanline */
	adjusted_scroll = newscroll;
	if (scanline <= Machine->visible_area.max_y)
		adjusted_scroll -= (scanline + 1);
	tilemap_set_scrolly(atarigen_playfield_tilemap, 0, adjusted_scroll);

	/* but since we've adjusted it, we must reset it to the normal value
	   once we hit scanline 0 again */
	timer_adjust(yscroll_reset_timer, cpu_getscanlinetime(0), newscroll, 0);

	/* update the data */
	*atarigen_yscroll = newscroll;
}



/*************************************
 *
 *	Sprite RAM write handler
 *
 *************************************/

WRITE16_HANDLER( atarisy1_spriteram_w )
{
	int active_bank = atarimo_get_bank(0);
	int oldword = atarimo_0_spriteram[offset];
	int newword = oldword;
	COMBINE_DATA(&newword);

	/* if the data changed, and it modified the live sprite bank, do some extra work */
	if (oldword != newword && (offset >> 8) == active_bank)
	{
		/* if modifying a timer, beware */
		if (((offset & 0xc0) == 0x00 && atarimo_0_spriteram[offset | 0x40] == 0xffff) ||
		    ((offset & 0xc0) == 0x40 && (newword == 0xffff || oldword == 0xffff)))
		{
			/* if the timer is in the active bank, update the display list */
			atarimo_0_spriteram_w(offset, data, 0);
			update_timers(cpu_getscanline());
		}
		
		/* if we're about to modify data in the active sprite bank, make sure the video is up-to-date */
		/* Road Runner needs this to work; note the +2 kludge -- +1 would be correct since the video */
		/* renders the next scanline's sprites to the line buffers, but Road Runner still glitches */
		/* without the extra +1 */
		else
			force_partial_update(cpu_getscanline() + 2);
	}

	/* let the MO handler do the basic work */
	atarimo_0_spriteram_w(offset, data, 0);
}



/*************************************
 *
 *	MO interrupt handlers
 *
 *************************************/

static void int3off_callback(int param)
{
	/* clear the state */
	atarigen_scanline_int_ack_w(0, 0, 0);
}


static void int3_callback(int scanline)
{
	/* update the state */
	atarigen_scanline_int_gen();

	/* set a timer to turn it off */
	timer_adjust(int3off_timer, cpu_getscanlineperiod(), 0, 0);

	/* determine the time of the next one */
	next_timer_scanline = -1;
	update_timers(scanline);
}



/*************************************
 *
 *	MO interrupt state read
 *
 *************************************/

READ16_HANDLER( atarisy1_int3state_r )
{
	return atarigen_scanline_int_state ? 0x0080 : 0x0000;
}



/*************************************
 *
 *	Timer updater
 *
 *************************************/

static void update_timers(int scanline)
{
	UINT16 *base = &atarimo_0_spriteram[atarimo_get_bank(0) * 64 * 4];
	int link = 0, best = scanline, found = 0;
	UINT8 spritevisit[64];

	/* track which ones we've visited */
	memset(spritevisit, 0, sizeof(spritevisit));

	/* walk the list until we loop */
	while (!spritevisit[link])
	{
		/* timers are indicated by 0xffff in entry 2 */
		if (base[link + 0x40] == 0xffff)
		{
			int data = base[link];
			int vsize = (data & 15) + 1;
			int ypos = (256 - (data >> 5) - vsize * 8 - 1) & 0x1ff;

			/* note that we found something */
			found = 1;

			/* is this a better entry than the best so far? */
			if (best <= scanline)
			{
				if ((ypos <= scanline && ypos < best) || ypos > scanline)
					best = ypos;
			}
			else
			{
				if (ypos < best)
					best = ypos;
			}
		}

		/* link to the next */
		spritevisit[link] = 1;
		link = base[link + 0xc0] & 0x3f;
	}

	/* if nothing was found, use scanline -1 */
	if (!found)
		best = -1;

	/* update the timer */
	if (best != next_timer_scanline)
	{
		next_timer_scanline = best;

		/* set a new one */
		if (best != -1)
			timer_adjust(scanline_timer, cpu_getscanlinetime(best), best, 0);
		else
			timer_adjust(scanline_timer, TIME_NEVER, 0, 0);
	}
}



/*************************************
 *
 *	Main refresh
 *
 *************************************/

VIDEO_UPDATE( atarisy1 )
{
	struct atarimo_rect_list rectlist;
	struct mame_bitmap *mobitmap;
	int x, y, r;

	/* draw the playfield */
	tilemap_draw(bitmap, cliprect, atarigen_playfield_tilemap, 0, 0);

	/* draw and merge the MO */
	mobitmap = atarimo_render(0, cliprect, &rectlist);
	for (r = 0; r < rectlist.numrects; r++, rectlist.rect++)
		for (y = rectlist.rect->min_y; y <= rectlist.rect->max_y; y++)
		{
			UINT16 *mo = (UINT16 *)mobitmap->base + mobitmap->rowpixels * y;
			UINT16 *pf = (UINT16 *)bitmap->base + bitmap->rowpixels * y;
			for (x = rectlist.rect->min_x; x <= rectlist.rect->max_x; x++)
				if (mo[x])
				{
					/* high priority MO? */
					if (mo[x] & ATARIMO_PRIORITY_MASK)
					{
						/* only gets priority if MO pen is not 1 */
						if ((mo[x] & 0x0f) != 1)
							pf[x] = 0x300 + ((pf[x] & 0x0f) << 4) + (mo[x] & 0x0f);
					}

					/* low priority */
					else
					{
						/* priority pens for playfield color 0 */
						if ((pf[x] & 0xf8) != 0 || !(playfield_priority_pens & (1 << (pf[x] & 0x07))))
							pf[x] = mo[x];
					}

					/* erase behind ourselves */
					mo[x] = 0;
				}
		}

	/* add the alpha on top */
	tilemap_draw(bitmap, cliprect, atarigen_alpha_tilemap, 0, 0);
}



/*************************************
 *
 *	Graphics decoding
 *
 *************************************/

static int decode_gfx(UINT16 *pflookup, UINT16 *molookup)
{
	UINT8 *prom1 = &memory_region(REGION_PROMS)[0x000];
	UINT8 *prom2 = &memory_region(REGION_PROMS)[0x200];
	int obj, i;

	/* reset the globals */
	memset(&bank_gfx[0][0], 0, sizeof(bank_gfx));

	/* loop for two sets of objects */
	for (obj = 0; obj < 2; obj++)
	{
		/* loop for 256 objects in the set */
		for (i = 0; i < 256; i++, prom1++, prom2++)
		{
			int bank, bpp, color, offset;

			/* determine the bpp */
			bpp = 4;
			if (*prom2 & PROM2_PLANE_4_ENABLE)
			{
				bpp = 5;
				if (*prom2 & PROM2_PLANE_5_ENABLE)
					bpp = 6;
			}

			/* determine the offset */
			offset = *prom1 & PROM1_OFFSET_MASK;

			/* determine the bank */
			bank = get_bank(*prom1, *prom2, bpp);
			if (bank < 0)
				return 0;

			/* set the value */
			if (obj == 0)
			{
				/* playfield case */
				color = (~*prom2 & PROM2_PF_COLOR_MASK) >> (bpp - 4);
				if (bank == 0)
				{
					bank = 1;
					offset = color = 0;
				}
				pflookup[i] = offset | (bank << 8) | (color << 12);
			}
			else
			{
				/* motion objects (high bit ignored) */
				color = (~*prom2 & PROM2_MO_COLOR_MASK) >> (bpp - 4);
				molookup[i] = offset | (bank << 8) | (color << 12);
			}
		}
	}
	return 1;
}



/*************************************
 *
 *	Graphics bank mapping
 *
 *************************************/

static int get_bank(UINT8 prom1, UINT8 prom2, int bpp)
{
	int bank_offset[8] = { 0, 0x00000, 0x30000, 0x60000, 0x90000, 0xc0000, 0xe0000, 0x100000 };
	int bank_index, i, gfx_index;

	/* determine the bank index */
	if ((prom1 & PROM1_BANK_1) == 0)
		bank_index = 1;
	else if ((prom1 & PROM1_BANK_2) == 0)
		bank_index = 2;
	else if ((prom1 & PROM1_BANK_3) == 0)
		bank_index = 3;
	else if ((prom1 & PROM1_BANK_4) == 0)
		bank_index = 4;
	else if ((prom2 & PROM2_BANK_5) == 0)
		bank_index = 5;
	else if ((prom2 & PROM2_BANK_6_OR_7) == 0)
	{
		if ((prom2 & PROM2_BANK_7) == 0)
			bank_index = 7;
		else
			bank_index = 6;
	}
	else
		return 0;

	/* find the bank */
	if (bank_gfx[bpp - 4][bank_index])
		return bank_gfx[bpp - 4][bank_index];

	/* if the bank is out of range, call it 0 */
	if (bank_offset[bank_index] >= memory_region_length(REGION_GFX2))
		return 0;

	/* don't have one? let's make it ... first find any empty slot */
	for (gfx_index = 0; gfx_index < MAX_GFX_ELEMENTS; gfx_index++)
		if (Machine->gfx[gfx_index] == NULL)
			break;
	if (gfx_index == MAX_GFX_ELEMENTS)
		return -1;

	/* tweak the structure for the number of bitplanes we have */
	objlayout.planes = bpp;
	for (i = 0; i < bpp; i++)
		objlayout.planeoffset[i] = (bpp - i - 1) * 0x8000 * 8;

	/* decode the graphics */
	Machine->gfx[gfx_index] = decodegfx(&memory_region(REGION_GFX2)[bank_offset[bank_index]], &objlayout);
	if (!Machine->gfx[gfx_index])
		return -1;

	/* set the color information */
	Machine->gfx[gfx_index]->colortable = &Machine->remapped_colortable[256];
	Machine->gfx[gfx_index]->color_granularity = 8;
	Machine->gfx[gfx_index]->total_colors = 0x40;
	bank_color_shift[gfx_index] = bpp - 3;

	/* set the entry and return it */
	return bank_gfx[bpp - 4][bank_index] = gfx_index;
}
