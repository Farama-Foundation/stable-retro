/***************************************************************************

  Video Hardware for Championship V'ball by Paul Hampson
  Generally copied from China Gate by Paul Hampson
  "Mainly copied from Vidhrdw of Double Dragon (bootleg) & Double Dragon II"

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m6502/m6502.h"
#include <stdio.h>

int vb_scrollx_hi=0;
int vb_scrollx_lo=0;
int vb_scrolly_hi=0;

unsigned char *vb_scrolly_lo;
unsigned char *vb_videoram;
unsigned char *vb_attribram;
int vball_gfxset=0;
int vb_bgprombank=0xff;
int vb_spprombank=0xff;

static struct tilemap *bg_tilemap;
static int scrollx[31];

/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

static UINT32 background_scan(UINT32 col,UINT32 row,UINT32 num_cols,UINT32 num_rows)
{
	/* logical (col,row) -> memory offset */
	return (col & 0x1f) + ((row & 0x1f) << 5) + ((col & 0x20) << 5) + ((row & 0x20) <<6);
}

static void get_bg_tile_info(int tile_index)
{
	unsigned char code = vb_videoram[tile_index];
	unsigned char attr = vb_attribram[tile_index];
	SET_TILE_INFO(
			0,
			code + ((attr & 0x1f) << 8) + (vball_gfxset<<8),
			(attr >> 5) & 0x7,
			0)
}


VIDEO_START( vb )
{
	bg_tilemap = tilemap_create(get_bg_tile_info,background_scan,TILEMAP_OPAQUE, 8, 8,64,64);
	if( !bg_tilemap )
		return 1;

	tilemap_set_scroll_rows(bg_tilemap,32);

	return 0;
}

WRITE_HANDLER( vb_videoram_w )
{
	if (vb_videoram[offset] != data)
	{
		vb_videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap,offset);
	}
}

READ_HANDLER( vb_attrib_r )
{
	return vb_attribram[offset];
}

WRITE_HANDLER( vb_attrib_w )
{
	if( vb_attribram[offset] != data ){
		vb_attribram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap,offset);
	}
}

void vb_bgprombank_w( int bank )
{
	int i;
	UINT8* color_prom;

	if (bank==vb_bgprombank) return;

	color_prom = memory_region(REGION_PROMS) + bank*0x80;
	for (i=0;i<128;i++, color_prom++) {
		palette_set_color(i,pal4bit(color_prom[0] >> 0),pal4bit(color_prom[0] >> 4),
				       pal4bit(color_prom[0x800] >> 0));
	}
	vb_bgprombank=bank;
}

void vb_spprombank_w( int bank )
{

	int i;
	UINT8* color_prom;

	if (bank==vb_spprombank) return;

	color_prom = memory_region(REGION_PROMS)+0x400 + bank*0x80;
	for (i=128;i<256;i++,color_prom++)	{
		palette_set_color(i,pal4bit(color_prom[0] >> 0),pal4bit(color_prom[0] >> 4),
				       pal4bit(color_prom[0x800] >> 0));
	}
	vb_spprombank=bank;
}

void vb_mark_all_dirty( void )
{
	tilemap_mark_all_tiles_dirty(bg_tilemap);
}

#define DRAW_SPRITE( order, sx, sy ) drawgfx( bitmap, gfx, \
					(which+order),color,flipx,flipy,sx,sy, \
					cliprect,TRANSPARENCY_PEN,0);

static void draw_sprites( struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	const struct GfxElement *gfx = Machine->gfx[1];
	unsigned char *src = spriteram;
	int i;

/*	240-Y    S|X|CLR|WCH WHICH    240-X
	xxxxxxxx x|x|xxx|xxx xxxxxxxx xxxxxxxx
*/
	for (i = 0;i < spriteram_size;i += 4)
	{
		int attr = src[i+1];
		int which = src[i+2]+((attr & 0x07)<<8);
		int sx = ((src[i+3] + 8) & 0xff) - 7;
		int sy = 240 - src[i];
		int size = (attr & 0x80) >> 7;
		int color = (attr & 0x38) >> 3;
		int flipx = ~attr & 0x40;
		int flipy = 0;
		int dy = -16;

		if (flip_screen)
		{
			sx = 240 - sx;
			sy = 240 - sy;
			flipx = !flipx;
			flipy = !flipy;
			dy = -dy;
		}

		switch (size)
		{
			case 0: /* normal */
			DRAW_SPRITE(0,sx,sy);
			break;

			case 1: /* double y */
			DRAW_SPRITE(0,sx,sy + dy);
			DRAW_SPRITE(1,sx,sy);
			break;
		}
	}
}

#undef DRAW_SPRITE

VIDEO_UPDATE( vb )
{
	int i;

	tilemap_set_scrolly(bg_tilemap,0,vb_scrolly_hi + *vb_scrolly_lo);

	/*To get linescrolling to work properly, we must ignore the 1st two scroll values, no idea why! -SJE */
	for (i = 2;i < 32;i++) {
		tilemap_set_scrollx(bg_tilemap,i,scrollx[i-2]);
		/*logerror("scrollx[%d] = %d\n",i,scrollx[i]);*/
	}
	tilemap_draw(bitmap,cliprect,bg_tilemap,0,0);
	draw_sprites(bitmap,cliprect);
}


/*I don't really understand what the proper timing of this should be,
  but after TONS of testing, the tilemap individual line scrolling works as long as flip screen is not set -SJE
*/
INTERRUPT_GEN( vball_interrupt )
{
	int line = 31 - cpu_getiloops();
	if (line < 13)
		cpu_set_irq_line(0, M6502_IRQ_LINE, HOLD_LINE);
	else if (line == 13)
		cpu_set_irq_line(0, IRQ_LINE_NMI, PULSE_LINE);
	/*save the scroll x register value*/
	if(line<32) scrollx[31-line] = (vb_scrollx_hi + vb_scrollx_lo+4);
}
