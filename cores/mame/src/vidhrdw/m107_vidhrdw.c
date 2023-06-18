/****************************************************************************

	Irem M107 video hardware, Bryan McPhail, mish@tendril.co.uk

	Close to M92 hardware, but with 4 playfields, not 3.
	Twice as many colours, twice as many sprites.

*****************************************************************************

	Port:
		0x80: pf1 Y scroll
		0x82: pf1 X scroll
		0x84: pf2 Y scroll
		0x86: pf2 X scroll
		0x88: pf3 Y scroll
		0x8a: pf3 X scroll
		0x8c: pf4 Y scroll
		0x8e: pf4 X scroll

		0x90: pf1 control
		0x92: pf2 control
		0x94: pf3 control
		0x96: pf4 control

		0x98: Priority?
		0x9a:
		0x9c:
		0x9e: Raster interrupt value

	Playfield control:
		Bit  0x0f00: 	Playfield location in VRAM (in steps of 0x1000)
		Bit  0x0080:	0 = Playfield enable, 1 = disable
		Bit  0x0002:	1 = Rowscroll enable, 0 = disable

*****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

static struct tilemap *pf4_layer,*pf3_layer,*pf2_layer,*pf1_layer;
static int m107_control[0x20];
static unsigned char *m107_spriteram;
unsigned char *m107_vram_data;
int m107_raster_irq_position,m107_sprite_list;
int m107_spritesystem;

static int pf1_vram_ptr,pf2_vram_ptr,pf3_vram_ptr,pf4_vram_ptr;
static int pf1_enable,pf2_enable,pf3_enable,pf4_enable;
static int pf1_rowscroll,pf2_rowscroll,pf3_rowscroll,pf4_rowscroll;

/*****************************************************************************/

static void get_pf1_tile_info(int tile_index)
{
	int tile,color;
	tile_index = 4*tile_index + pf1_vram_ptr;

	tile=m107_vram_data[tile_index]+(m107_vram_data[tile_index+1]<<8);
	if (m107_vram_data[tile_index+3] & 0x10) tile+=0x10000;
	color=m107_vram_data[tile_index+2];

	SET_TILE_INFO(
			0,
			tile,
			color&0x7f,
			TILE_FLIPYX((m107_vram_data[tile_index+3] & 0xc)>>2))

	/* Priority 1 = tile appears above sprites */
	tile_info.priority = ((m107_vram_data[tile_index+3]&2)>>1);
}

static void get_pf2_tile_info(int tile_index)
{
	int tile,color;
	tile_index = 4*tile_index + pf2_vram_ptr;

	tile=m107_vram_data[tile_index]+(m107_vram_data[tile_index+1]<<8);
	if (m107_vram_data[tile_index+3] & 0x10) tile+=0x10000;
	color=m107_vram_data[tile_index+2];

	SET_TILE_INFO(
			0,
			tile,
			color&0x7f,
			TILE_FLIPYX((m107_vram_data[tile_index+3] & 0xc)>>2))

	tile_info.priority = ((m107_vram_data[tile_index+3]&2)>>1);
}

static void get_pf3_tile_info(int tile_index)
{
	int tile,color;
	tile_index = 4*tile_index + pf3_vram_ptr;

	tile=m107_vram_data[tile_index]+(m107_vram_data[tile_index+1]<<8);
	if (m107_vram_data[tile_index+3] & 0x10) tile+=0x10000;
	color=m107_vram_data[tile_index+2];

	SET_TILE_INFO(
			0,
			tile,
			color&0x7f,
			TILE_FLIPYX((m107_vram_data[tile_index+3] & 0xc)>>2))
}

static void get_pf4_tile_info(int tile_index)
{
	int tile,color;
	tile_index = 4*tile_index + pf4_vram_ptr;

	tile=m107_vram_data[tile_index]+(m107_vram_data[tile_index+1]<<8);
	if (m107_vram_data[tile_index+3] & 0x10) tile+=0x10000;
	color=m107_vram_data[tile_index+2];

	SET_TILE_INFO(
			0,
			tile,
			color&0x7f,
			TILE_FLIPYX((m107_vram_data[tile_index+3] & 0xc)>>2))
}

/*****************************************************************************/

READ_HANDLER( m107_vram_r )
{
	return m107_vram_data[offset];
}

WRITE_HANDLER( m107_vram_w )
{
	int a;

	m107_vram_data[offset]=data;

	/* Work out what area to dirty, potentially more than 1 */
	a=offset&0xc000;
	offset&=0x3fff;

	if (a==pf1_vram_ptr)
		tilemap_mark_tile_dirty( pf1_layer,offset/4);

	if (a==pf2_vram_ptr)
		tilemap_mark_tile_dirty( pf2_layer,offset/4);

	if (a==pf3_vram_ptr)
		tilemap_mark_tile_dirty( pf3_layer,offset/4);

	if (a==pf4_vram_ptr)
		tilemap_mark_tile_dirty( pf4_layer,offset/4);
}

/*****************************************************************************/

WRITE_HANDLER( m107_control_w )
{
	static int last_pf1,last_pf2,last_pf3,last_pf4;

	m107_control[offset]=data;

	switch (offset) {
		case 0x10: /* Playfield 1 (top layer) */
		case 0x11:
			if ((m107_control[0x10]&0x80)==0x80) pf1_enable=0; else pf1_enable=1;
			if ((m107_control[0x10]&0x02)==0x02) pf1_rowscroll=1; else pf1_rowscroll=0;
			tilemap_set_enable(pf1_layer,pf1_enable);
			pf1_vram_ptr=(m107_control[0x11]&0xf)*0x1000;
			/* Have to dirty tilemaps if vram pointer has changed */
			if (last_pf1!=pf1_vram_ptr)
				tilemap_mark_all_tiles_dirty(pf1_layer);
			last_pf1=pf1_vram_ptr;
			break;

		case 0x12: /* Playfield 2 */
		case 0x13:
			if ((m107_control[0x12]&0x80)==0x80) pf2_enable=0; else pf2_enable=1;
			if ((m107_control[0x12]&0x02)==0x02) pf2_rowscroll=1; else pf2_rowscroll=0;
			tilemap_set_enable(pf2_layer,pf2_enable);
			pf2_vram_ptr=(m107_control[0x13]&0xf)*0x1000;
			/* Have to dirty tilemaps if vram pointer has changed */
			if (last_pf2!=pf2_vram_ptr)
				tilemap_mark_all_tiles_dirty(pf2_layer);
			last_pf2=pf2_vram_ptr;
			break;

		case 0x14: /* Playfield 3 */
		case 0x15:
			if ((m107_control[0x14]&0x80)==0x80) pf3_enable=0; else pf3_enable=1;
			if ((m107_control[0x14]&0x02)==0x02) pf3_rowscroll=1; else pf3_rowscroll=0;
			tilemap_set_enable(pf3_layer,pf3_enable);
			pf3_vram_ptr=(m107_control[0x15]&0xf)*0x1000;
			/* Have to dirty tilemaps if vram pointer has changed */
			if (last_pf3!=pf3_vram_ptr)
				tilemap_mark_all_tiles_dirty(pf3_layer);
			last_pf3=pf3_vram_ptr;
			break;

		case 0x16: /* Playfield 4 */
		case 0x17:
			if ((m107_control[0x16]&0x80)==0x80) pf4_enable=0; else pf4_enable=1;
			if ((m107_control[0x16]&0x02)==0x02) pf4_rowscroll=1; else pf4_rowscroll=0;
			tilemap_set_enable(pf4_layer,pf4_enable);
			pf4_vram_ptr=(m107_control[0x17]&0xf)*0x1000;
			/* Have to dirty tilemaps if vram pointer has changed */
			if (last_pf4!=pf4_vram_ptr)
				tilemap_mark_all_tiles_dirty(pf4_layer);
			last_pf4=pf4_vram_ptr;
			break;

		case 0x1e:
			m107_raster_irq_position=(m107_control[0x1e])-128; /*gamezfan */
			break;
	}
}

/*****************************************************************************/

VIDEO_START( m107 )
{
	pf1_layer = tilemap_create(
		get_pf1_tile_info,tilemap_scan_rows,
		TILEMAP_TRANSPARENT,
		8,8,
		64,64
	);

	pf2_layer = tilemap_create(
		get_pf2_tile_info,tilemap_scan_rows,
		TILEMAP_TRANSPARENT,
		8,8,
		64,64
	);

	pf3_layer = tilemap_create(
		get_pf3_tile_info,tilemap_scan_rows,
		TILEMAP_TRANSPARENT,
		8,8,
		64,64
	);

	pf4_layer = tilemap_create(
		get_pf4_tile_info,tilemap_scan_rows,
		0,
		8,8,
		64,64
	);

	if (!pf1_layer || !pf2_layer || !pf3_layer || !pf4_layer)
		return 1;

	tilemap_set_transparent_pen(pf1_layer,0);
	tilemap_set_transparent_pen(pf2_layer,0);
	tilemap_set_transparent_pen(pf3_layer,0);

	pf1_vram_ptr=pf2_vram_ptr=pf3_vram_ptr=pf4_vram_ptr=0;
	pf1_enable=pf2_enable=pf3_enable=pf4_enable=0;
	pf1_rowscroll=pf2_rowscroll=pf3_rowscroll=pf4_rowscroll=0;

	m107_spriteram = auto_malloc(0x1000);
	if (!m107_spriteram)
		return 1;
	memset(m107_spriteram,0,0x1000);

	m107_sprite_list=0;

	return 0;
}

/*****************************************************************************/

static void m107_drawsprites(struct mame_bitmap *bitmap, const struct rectangle *cliprect, int pri)
{
	int offs;

	for (offs = 0x1000-8;offs >= 0;offs -= 8) {
		int x,y,sprite,colour,fx,fy,y_multi,i,s_ptr;

		if (((m107_spriteram[offs+4]&0x80)==0x80) && pri==0) continue;
		if (((m107_spriteram[offs+4]&0x80)==0x00) && pri==1) continue;

		y=m107_spriteram[offs+0] | (m107_spriteram[offs+1]<<8);
		x=m107_spriteram[offs+6] | (m107_spriteram[offs+7]<<8);
		x&=0x1ff;
		y&=0x1ff;

		if (x==0 || y==0) continue; /* offscreen */

	    sprite=(m107_spriteram[offs+2] | (m107_spriteram[offs+3]<<8))&0x7fff;

		x = x - 16;
		y = 512 - 16 - y;

		colour=m107_spriteram[offs+4]&0x7f;
		fx=m107_spriteram[offs+5]&1;
		fy=m107_spriteram[offs+5]&2;
		y_multi=(m107_spriteram[offs+1]>>3)&0x3;

		if (m107_spritesystem == 0)
		{
			y_multi=1 << y_multi; /* 1, 2, 4 or 8 */

			s_ptr = 0;
			if (!fy) s_ptr+=y_multi-1;

			for (i=0; i<y_multi; i++)
			{
				drawgfx(bitmap,Machine->gfx[1],
						sprite + s_ptr,
						colour,
						fx,fy,
						x,y-i*16,
						cliprect,TRANSPARENCY_PEN,0);
				if (fy) s_ptr++; else s_ptr--;
			}
		}
		else
		{
			UINT8 *rom = memory_region(REGION_USER1);
			int rom_offs = sprite*8;

			if (rom[rom_offs+1] || rom[rom_offs+3] || rom[rom_offs+5] || rom[rom_offs+7])
			{
				while (rom_offs < 0x40000)	/* safety check */
				{
					int xdisp = rom[rom_offs+6]+256*rom[rom_offs+7];
					int ydisp = rom[rom_offs+2]+256*rom[rom_offs+3];
					int ffx=fx^(rom[rom_offs+1]&1);
					int ffy=fy^(rom[rom_offs+1]&2);
					sprite=rom[rom_offs+4]+256*rom[rom_offs+5];
					y_multi=1<<((rom[rom_offs+3]>>1)&0x3);
					if (fx) xdisp = -xdisp;
					if (fy) ydisp = -ydisp - (16*y_multi-1);
					if (!ffy) sprite+=y_multi-1;
					for (i=0; i<y_multi; i++)
					{
						drawgfx(bitmap,Machine->gfx[1],
								sprite+(ffy?i:-i),
								colour,
								ffx,ffy,
								(x+xdisp)&0x1ff,(y-ydisp-16*i)&0x1ff,
								cliprect,TRANSPARENCY_PEN,0);
					}

					if (rom[rom_offs+1]&0x80) break;	/* end of block */

 					rom_offs += 8;
				}
			}
		}
	}
}

/*****************************************************************************/

VIDEO_UPDATE( m107 )
{
	/* Nothing - screen refresh is handled by raster interrupt routine */
}

/*****************************************************************************/

static void m107_update_scroll_positions(void)
{
	int i;

	/*	Playfield 4 rowscroll data is 0xde800 - 0xdebff???
		Playfield 3 rowscroll data is 0xdf800 - 0xdfbff
		Playfield 2 rowscroll data is 0xdf400 - 0xdf7ff
		Playfield 1 rowscroll data is 0xde800 - 0xdebff		??
	*/

	if (pf1_rowscroll) {
		tilemap_set_scroll_rows(pf1_layer,1024);
		for (i=0; i<1024; i+=2)
			tilemap_set_scrollx( pf1_layer,i/2, (m107_vram_data[0xe800+i]+(m107_vram_data[0xe801+i]<<8)));
	} else {
		tilemap_set_scroll_rows(pf1_layer,1);
		tilemap_set_scrollx( pf1_layer,0, (m107_control[3]<<8)+m107_control[2]+3 );
	}
	if (pf2_rowscroll) {
		tilemap_set_scroll_rows(pf2_layer,512);
		for (i=0; i<1024; i+=2)
			tilemap_set_scrollx( pf2_layer,i/2, (m107_vram_data[0xf400+i]+(m107_vram_data[0xf401+i]<<8)));
	} else {
		tilemap_set_scroll_rows(pf2_layer,1);
		tilemap_set_scrollx( pf2_layer,0, (m107_control[7]<<8)+m107_control[6]+1 );
	}
	if (pf3_rowscroll) {
		tilemap_set_scroll_rows(pf3_layer,512);
		for (i=0; i<1024; i+=2)
			tilemap_set_scrollx( pf3_layer,i/2, (m107_vram_data[0xf800+i]+(m107_vram_data[0xf801+i]<<8)));
	} else {
		tilemap_set_scroll_rows(pf3_layer,1);
		tilemap_set_scrollx( pf3_layer,0, (m107_control[11]<<8)+m107_control[10]-1 );
	}
	if (pf4_rowscroll) {
		tilemap_set_scroll_rows(pf4_layer,512);
		for (i=0; i<1024; i+=2)
			tilemap_set_scrollx( pf4_layer,i/2, (m107_vram_data[0xfc00+i]+(m107_vram_data[0xfc01+i]<<8)));
	} else {
		tilemap_set_scroll_rows(pf4_layer,1);
		tilemap_set_scrollx( pf4_layer,0, (m107_control[15]<<8)+m107_control[14]-3 );
	}

	tilemap_set_scrolly( pf1_layer,0, (m107_control[1]<<8)+m107_control[0] );
	tilemap_set_scrolly( pf2_layer,0, (m107_control[5]<<8)+m107_control[4] );
	tilemap_set_scrolly( pf3_layer,0, (m107_control[9]<<8)+m107_control[8] );
	tilemap_set_scrolly( pf4_layer,0, (m107_control[13]<<8)+m107_control[12] );

/*	pf4_layer->scrolled=1;*/
/*	pf3_layer->scrolled=1;*/
/*	pf2_layer->scrolled=1;*/
/*	pf1_layer->scrolled=1;*/
}

/*****************************************************************************/

void m107_screenrefresh(struct mame_bitmap *bitmap,const struct rectangle *cliprect)
{
	if (pf4_enable)
		tilemap_draw(bitmap,cliprect,pf4_layer,0,0);
	else
		fillbitmap(bitmap,Machine->pens[0],cliprect);

	tilemap_draw(bitmap,cliprect,pf3_layer,0,0);
	tilemap_draw(bitmap,cliprect,pf2_layer,0,0);
	tilemap_draw(bitmap,cliprect,pf1_layer,0,0);

	m107_drawsprites(bitmap,cliprect,0);
	tilemap_draw(bitmap,cliprect,pf2_layer,1,0);
	tilemap_draw(bitmap,cliprect,pf1_layer,1,0);

	m107_drawsprites(bitmap,cliprect,1);

	/* This hardware probably has more priority values - but I haven't found
		any used yet */
}

void m107_vh_raster_partial_refresh(struct mame_bitmap *bitmap,int start_line,int end_line)
{
	struct rectangle clip;

	clip.min_x = Machine->visible_area.min_x;
	clip.max_x = Machine->visible_area.max_x;
	clip.min_y = start_line+128;
	clip.max_y = end_line+128;
	if (clip.min_y < Machine->visible_area.min_y)
		clip.min_y = Machine->visible_area.min_y;
	if (clip.max_y > Machine->visible_area.max_y)
		clip.max_y = Machine->visible_area.max_y;

	if (clip.max_y > clip.min_y)
	{
		m107_update_scroll_positions();
		m107_screenrefresh(bitmap,&clip);
	}
}

/*****************************************************************************/

WRITE_HANDLER( m107_spritebuffer_w )
{
	if (offset==0) {
/*		log_cb(RETRO_LOG_DEBUG, LOGPRE "%04x: buffered spriteram\n",activecpu_get_pc());*/
		memcpy(m107_spriteram,spriteram,0x1000);
	}
}
