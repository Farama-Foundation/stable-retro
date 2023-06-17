#include "driver.h"


static data8_t *vram[2],*unkram;
static int bankctrl,rambank,pmcbank,gfxrom_select;
static struct tilemap *tilemap[2];



/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

static INLINE void get_tile_info(int tile_index,data8_t *ram)
{
	tile_index *= 4;
	SET_TILE_INFO(
			0,
			ram[tile_index] + ((ram[tile_index+1] & 0x3f) << 8),
			ram[tile_index+2] & 0x0f,
			0)
}

static void get_tile_info0(int tile_index)
{
	get_tile_info(tile_index,vram[0]);
}

static void get_tile_info1(int tile_index)
{
	get_tile_info(tile_index,vram[1]);
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

VIDEO_START( hexion )
{
	tilemap[0] = tilemap_create(get_tile_info0,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,32);
	tilemap[1] = tilemap_create(get_tile_info1,tilemap_scan_rows,TILEMAP_OPAQUE,     8,8,64,32);

	if (!tilemap[0] || !tilemap[1])
		return 1;

	tilemap_set_transparent_pen(tilemap[0],0);
	tilemap_set_scrollx(tilemap[1],0,-4);
	tilemap_set_scrolly(tilemap[1],0,4);

	vram[0] = memory_region(REGION_CPU1) + 0x30000;
	vram[1] = vram[0] + 0x2000;
	unkram = vram[1] + 0x2000;

	return 0;
}



/***************************************************************************

  Memory handlers

***************************************************************************/

WRITE_HANDLER( hexion_bankswitch_w )
{
	unsigned char *rom = memory_region(REGION_CPU1) + 0x10000;

	/* bits 0-3 select ROM bank */
	cpu_setbank(1,rom + 0x2000 * (data & 0x0f));

	/* does bit 6 trigger the 052591? */
	if (data & 0x40)
	{
		int bank = unkram[0]&1;
		memset(vram[bank],unkram[1],0x2000);
		tilemap_mark_all_tiles_dirty(tilemap[bank]);
	}
	/* bit 7 = PMC-BK */
	pmcbank = (data & 0x80) >> 7;

	/* other bits unknown */
if (data & 0x30)
	usrintf_showmessage("bankswitch %02x",data&0xf0);

/*logerror("%04x: bankswitch_w %02x\n",activecpu_get_pc(),data);*/
}

READ_HANDLER( hexion_bankedram_r )
{
	if (gfxrom_select && offset < 0x1000)
	{
		return memory_region(REGION_GFX1)[((gfxrom_select & 0x7f) << 12) + offset];
	}
	else if (bankctrl == 0)
	{
		return vram[rambank][offset];
	}
	else if (bankctrl == 2 && offset < 0x800)
	{
		return unkram[offset];
	}
	else
	{
/*logerror("%04x: bankedram_r offset %04x, bankctrl = %02x\n",activecpu_get_pc(),offset,bankctrl);*/
		return 0;
	}
}

WRITE_HANDLER( hexion_bankedram_w )
{
	if (bankctrl == 3 && offset == 0 && (data & 0xfe) == 0)
	{
/*logerror("%04x: bankedram_w offset %04x, data %02x, bankctrl = %02x\n",activecpu_get_pc(),offset,data,bankctrl);*/
		rambank = data & 1;
	}
	else if (bankctrl == 0)
	{
		if (pmcbank)
		{
/*logerror("%04x: bankedram_w offset %04x, data %02x, bankctrl = %02x\n",activecpu_get_pc(),offset,data,bankctrl);*/
			if (vram[rambank][offset] != data)
			{
				vram[rambank][offset] = data;
				tilemap_mark_tile_dirty(tilemap[rambank],offset/4);
			}
		}
		else
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%04x pmc internal ram %04x = %02x\n",activecpu_get_pc(),offset,data);
	}
	else if (bankctrl == 2 && offset < 0x800)
	{
		if (pmcbank)
		{
/*logerror("%04x: unkram_w offset %04x, data %02x, bankctrl = %02x\n",activecpu_get_pc(),offset,data,bankctrl);*/
			unkram[offset] = data;
		}
		else
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%04x pmc internal ram %04x = %02x\n",activecpu_get_pc(),offset,data);
	}
	else
log_cb(RETRO_LOG_DEBUG, LOGPRE "%04x: bankedram_w offset %04x, data %02x, bankctrl = %02x\n",activecpu_get_pc(),offset,data,bankctrl);
}

WRITE_HANDLER( hexion_bankctrl_w )
{
/*logerror("%04x: bankctrl_w %02x\n",activecpu_get_pc(),data);*/
	bankctrl = data;
}

WRITE_HANDLER( hexion_gfxrom_select_w )
{
/*logerror("%04x: gfxrom_select_w %02x\n",activecpu_get_pc(),data);*/
	gfxrom_select = data;
}



/***************************************************************************

  Display refresh

***************************************************************************/

VIDEO_UPDATE( hexion )
{
	tilemap_draw(bitmap,cliprect,tilemap[1],0,0);
	tilemap_draw(bitmap,cliprect,tilemap[0],0,0);
}
