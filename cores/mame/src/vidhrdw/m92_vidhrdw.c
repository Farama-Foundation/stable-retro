/*****************************************************************************

	Irem M92 video hardware, Bryan McPhail, mish@tendril.co.uk

	Brief Overview:

		3 scrolling playfields, 512 by 512.
		Each playfield can enable rowscroll, change shape (to 1024 by 512),
		be enabled/disabled and change position in VRAM.

		Tiles can have several priority values:
			0 = standard
			1 = Top 8 pens appear over sprites (split tilemap)
			2 = Whole tile appears over sprites
			3 = ?  Seems to be the whole tile is over sprites (as 2).

		Sprites have 2 priority values:
			0 = standard
			1 = Sprite appears over all tiles, including high priority pf1

		Raster interrupts can be triggered at any line of the screen redraw,
		typically used in games like R-Type Leo to multiplex the top playfield.

*****************************************************************************

	Master Control registers:

		Word 0:	Playfield 1 control
			Bit  0x40:  1 = Rowscroll enable, 0 = disable
			Bit  0x10:	0 = Playfield enable, 1 = disable
			Bit  0x04:  0 = 512 x 512 playfield, 1 = 1024 x 512 playfield
			Bits 0x03:  Playfield location in VRAM (0, 0x4000, 0x8000, 0xc000)
		Word 1: Playfield 2 control (as above)
		Word 2: Playfield 3 control (as above)
		Word 3: Raster IRQ position.

	The raster IRQ position is offset by 128+8 from the first visible line,
	suggesting there are 8 lines before the first visible one.

*****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "state.h"

static struct tilemap *pf3_wide_layer,*pf3_layer,*pf2_layer,*pf1_wide_layer,*pf1_layer,*pf1_hlayer;
static int pf1_control[8],pf2_control[8],pf3_control[8],pf4_control[8];
static int pf1_vram_ptr,pf2_vram_ptr,pf3_vram_ptr;
static int pf1_enable,pf2_enable,pf3_enable;
static int pf1_rowscroll,pf2_rowscroll,pf3_rowscroll;
static int pf1_shape,pf2_shape,pf3_shape;
static int m92_sprite_list;

int m92_raster_irq_position,m92_raster_enable;
unsigned char *m92_vram_data,*m92_spritecontrol;
int m92_game_kludge;

/* This is a kludgey speedup that I consider to be pretty ugly..  But -
it gets a massive speed increase (~40fps -> ~65fps).  It works by avoiding
the need to dirty the top playfield twice a frame */
#define RYPELEO_SPEEDUP m92_game_kludge==1

extern void m92_sprite_interrupt(void);
int m92_sprite_buffer_busy;
static int m92_palette_bank;

/*****************************************************************************/

static void spritebuffer_callback (int dummy)
{
	m92_sprite_buffer_busy=0x80;
	if (m92_game_kludge!=2) /* Major Title 2 doesn't like this interrupt!? */
		m92_sprite_interrupt();
}

WRITE_HANDLER( m92_spritecontrol_w )
{
	static int sprite_extent;

	m92_spritecontrol[offset]=data;

	/* Sprite list size register - used in spriteroutine */
	if (offset==0)
		sprite_extent=data;

	/* Sprite control - display all sprites, or partial list */
	if (offset==4) {
		if (data==8)
			m92_sprite_list=(((0x100 - sprite_extent)&0xff)*8);
		else
			m92_sprite_list=0x800;

		/* Bit 0 is also significant */
	}

	/* Sprite buffer - the data written doesn't matter (confirmed by several games) */
	if (offset==8) {
		buffer_spriteram_w(0,0);
		m92_sprite_buffer_busy=0;

		/* Pixel clock is 26.6666 MHz, we have 0x800 bytes, or 0x400 words
		to copy from spriteram to the buffer.  It seems safe to assume 1
		word can be copied per clock.  So:

			1 MHz clock would be 1 word every 0.000,001s = 1000ns
			26.6666MHz clock would be 1 word every 0.000,000,037 = 37 ns
			Buffer should copy in about 37888 ns.
		*/
		timer_set (TIME_IN_NSEC(37 * 0x400), 0, spritebuffer_callback);
	}
/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "%04x: m92_spritecontrol_w %08x %08x\n",activecpu_get_pc(),offset,data);*/
}

WRITE_HANDLER( m92_videocontrol_w )
{
	/*
		Many games write:
			0x2000
			0x201b in alternate frames.

		Some games write to this both before and after the sprite buffer
		register - perhaps some kind of acknowledge bit is in there?

		Lethal Thunder fails it's RAM test with the upper palette bank
		enabled.  This was one of the earlier games and could actually
		be a different motherboard revision (most games use M92-A-B top
		pcb, a M92-A-A revision could exist...).
	*/
	if (offset==0)
	{
		/* Access to upper palette bank */
    if ((data & 0x2) == 0x2 && (m92_game_kludge!=3 && m92_game_kludge!=1)) m92_palette_bank = 1;
		else                     m92_palette_bank = 0;
	}
/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "%04x: m92_videocontrol_w %d = %02x\n",activecpu_get_pc(),offset,data);*/
}

READ_HANDLER( m92_paletteram_r )
{
	return paletteram_r(offset + 0x800*m92_palette_bank);
}

WRITE_HANDLER( m92_paletteram_w )
{
	paletteram_xBBBBBGGGGGRRRRR_w(offset + 0x800*m92_palette_bank,data);
}

/*****************************************************************************/

static void get_pf1_tile_info(int tile_index)
{
	int tile,color,pri;
	tile_index = 4*tile_index+pf1_vram_ptr;

	tile=m92_vram_data[tile_index]+(m92_vram_data[tile_index+1]<<8)+((m92_vram_data[tile_index+3]&0x80)<<9);
	color=m92_vram_data[tile_index+2];
	if (m92_vram_data[tile_index+3]&1) pri = 2;
	else if (color&0x80) pri = 1;
	else pri = 0;

	SET_TILE_INFO(
			0,
			tile,
			color&0x7f,
			TILE_FLIPYX((m92_vram_data[tile_index+3] & 0x6)>>1) | TILE_SPLIT(pri))
}

static void get_pf1_htile_info(int tile_index)
{
	int tile,color,pri;
	tile_index = 4*tile_index + 0xc000;

	tile=m92_vram_data[tile_index]+(m92_vram_data[tile_index+1]<<8)+((m92_vram_data[tile_index+3]&0x80)<<9);
	color=m92_vram_data[tile_index+2];
	if (m92_vram_data[tile_index+3]&1) pri = 2;
	else if (color&0x80) pri = 1;
	else pri = 0;

	SET_TILE_INFO(
			0,
			tile,
			color&0x7f,
			TILE_FLIPYX((m92_vram_data[tile_index+3] & 0x6)>>1) | TILE_SPLIT(pri))
}

static void get_pf1_ltile_info(int tile_index)
{
	int tile,color,pri;
	tile_index *= 4;
	if (pf1_vram_ptr==0x4000) tile_index += 0x4000;
	else if (pf1_vram_ptr==0x8000) tile_index += 0x8000;

	tile=m92_vram_data[tile_index]+(m92_vram_data[tile_index+1]<<8)+((m92_vram_data[tile_index+3]&0x80)<<9);
	color=m92_vram_data[tile_index+2];
	if (m92_vram_data[tile_index+3]&1) pri = 2;
	else if (color&0x80) pri = 1;
	else pri = 0;

	SET_TILE_INFO(
			0,
			tile,
			color&0x7f,
			TILE_FLIPYX((m92_vram_data[tile_index+3] & 0x6)>>1) | TILE_SPLIT(pri))
}

static void get_pf2_tile_info(int tile_index)
{
	int tile,color,pri;
	tile_index = 4*tile_index + pf2_vram_ptr;

	tile=m92_vram_data[tile_index]+(m92_vram_data[tile_index+1]<<8)+((m92_vram_data[tile_index+3]&0x80)<<9);
	color=m92_vram_data[tile_index+2];
	if (m92_vram_data[tile_index+3]&1) pri = 2;
	else if (color&0x80) pri = 1;
	else pri = 0;

	SET_TILE_INFO(
			0,
			tile,
			color&0x7f,
			TILE_FLIPYX((m92_vram_data[tile_index+3] & 0x6)>>1) | TILE_SPLIT(pri))
}

static void get_pf3_tile_info(int tile_index)
{
	int tile,color,pri;
	tile_index = 4*tile_index + pf3_vram_ptr;
	tile=m92_vram_data[tile_index]+(m92_vram_data[tile_index+1]<<8)+((m92_vram_data[tile_index+3]&0x80)<<9);
	color=m92_vram_data[tile_index+2];

	if (color&0x80) pri = 1;
	else pri = 0;

	SET_TILE_INFO(
			0,
			tile,
			color&0x7f,
			TILE_FLIPYX((m92_vram_data[tile_index+3] & 0x6)>>1) | TILE_SPLIT(pri))
}

static void get_pf1_wide_tile_info(int tile_index)
{
	int tile,color,pri;
	tile_index = 4*tile_index + pf1_vram_ptr;

	tile=m92_vram_data[tile_index]+(m92_vram_data[tile_index+1]<<8)+((m92_vram_data[tile_index+3]&0x80)<<9);
	color=m92_vram_data[tile_index+2];
	if (m92_vram_data[tile_index+3]&1) pri = 2;
	else if (color&0x80) pri = 1;
	else pri = 0;

	SET_TILE_INFO(
			0,
			tile,
			color&0x7f,
			TILE_FLIPYX((m92_vram_data[tile_index+3] & 0x6)>>1) | TILE_SPLIT(pri))
}

static void get_pf3_wide_tile_info(int tile_index)
{
	int tile,color,pri;
	tile_index = 4*tile_index + pf3_vram_ptr;

	tile=m92_vram_data[tile_index]+(m92_vram_data[tile_index+1]<<8)+((m92_vram_data[tile_index+3]&0x80)<<9);
	color=m92_vram_data[tile_index+2];

	if (color&0x80) pri = 1;
	else pri = 0;

	SET_TILE_INFO(
			0,
			tile,
			color&0x7f,
			TILE_FLIPYX((m92_vram_data[tile_index+3] & 0x6)>>1) | TILE_SPLIT(pri))
}

/*****************************************************************************/

READ_HANDLER( m92_vram_r )
{
	return m92_vram_data[offset];
}

WRITE_HANDLER( m92_vram_w )
{
	int a,wide;

	m92_vram_data[offset]=data;

	/* Work out what area to dirty, potentially more than 1 */
	a=offset&0xc000;
	wide=offset&0x7fff;
	offset&=0x3fff;

	if (RYPELEO_SPEEDUP) {
		if (a==0xc000) {
			tilemap_mark_tile_dirty(pf1_hlayer,offset/4);
			return;
		}
		tilemap_mark_tile_dirty(pf1_layer,offset/4);
	}
	else
	if (a==pf1_vram_ptr || (a==pf1_vram_ptr+0x4000)) {
		tilemap_mark_tile_dirty(pf1_layer,offset/4);
		tilemap_mark_tile_dirty(pf1_wide_layer,wide/4);
	}

	if (a==pf2_vram_ptr)
		tilemap_mark_tile_dirty(pf2_layer,offset/4);

	if (a==pf3_vram_ptr || (a==pf3_vram_ptr+0x4000)) {
		tilemap_mark_tile_dirty(pf3_layer,offset/4);
		tilemap_mark_tile_dirty(pf3_wide_layer,wide/4);
	}
}

/*****************************************************************************/

WRITE_HANDLER( m92_pf1_control_w )
{
	pf1_control[offset]=data;
}

WRITE_HANDLER( m92_pf2_control_w )
{
	pf2_control[offset]=data;
}

WRITE_HANDLER( m92_pf3_control_w )
{
	pf3_control[offset]=data;
}

WRITE_HANDLER( m92_master_control_w )
{
	static int last_pf1_ptr,last_pf2_ptr,last_pf3_ptr;

	pf4_control[offset]=data;

	switch (offset) {
		case 0: /* Playfield 1 (top layer) */
			if ((pf4_control[0]&0x10)==0x10) pf1_enable=0; else pf1_enable=1;
			if ((pf4_control[0]&0x40)==0x40) pf1_rowscroll=1; else pf1_rowscroll=0;
			pf1_vram_ptr=(pf4_control[0]&3)*0x4000;
			pf1_shape=(pf4_control[0]&4)>>2;

			if (RYPELEO_SPEEDUP) tilemap_set_enable(pf1_hlayer,pf1_enable);
			if (pf1_shape) {
				tilemap_set_enable(pf1_layer,0);
				tilemap_set_enable(pf1_wide_layer,pf1_enable);
			}
			else {
				tilemap_set_enable(pf1_layer,pf1_enable);
				tilemap_set_enable(pf1_wide_layer,0);
			}
			/* Have to dirty tilemaps if vram pointer has changed */
			if (m92_game_kludge!=1 && last_pf1_ptr!=pf1_vram_ptr) {
				tilemap_mark_all_tiles_dirty(pf1_layer);
				tilemap_mark_all_tiles_dirty(pf1_wide_layer);
			}
			last_pf1_ptr=pf1_vram_ptr;
			break;
		case 2: /* Playfield 2 (middle layer) */
			if ((pf4_control[2]&0x10)==0x10) pf2_enable=0; else pf2_enable=1;
			if ((pf4_control[2]&0x40)==0x40) pf2_rowscroll=1; else pf2_rowscroll=0;
			tilemap_set_enable(pf2_layer,pf2_enable);
			pf2_vram_ptr=(pf4_control[2]&3)*0x4000;
			pf2_shape=(pf4_control[2]&4)>>2;
			if (last_pf2_ptr!=pf2_vram_ptr)
				tilemap_mark_all_tiles_dirty(pf2_layer);
			last_pf2_ptr=pf2_vram_ptr;
			break;
		case 4: /* Playfield 3 (bottom layer) */
			if ((pf4_control[4]&0x10)==0x10) pf3_enable=0; else pf3_enable=1;
			if ((pf4_control[4]&0x40)==0x40) pf3_rowscroll=1; else pf3_rowscroll=0;
			pf3_shape=(pf4_control[4]&4)>>2;

			if (pf3_shape) {
				tilemap_set_enable(pf3_layer,0);
				tilemap_set_enable(pf3_wide_layer,pf3_enable);
			}
			else {
				tilemap_set_enable(pf3_layer,pf3_enable);
				tilemap_set_enable(pf3_wide_layer,0);
			}
			pf3_vram_ptr=(pf4_control[4]&3)*0x4000;
			if (last_pf3_ptr!=pf3_vram_ptr) {
				tilemap_mark_all_tiles_dirty(pf3_layer);
				tilemap_mark_all_tiles_dirty(pf3_wide_layer);
			}
			last_pf3_ptr=pf3_vram_ptr;
			break;
		case 6:
		case 7:
/*			if (flip_screen)*/
/*				m92_raster_irq_position=256-(((pf4_control[7]<<8) | pf4_control[6])-128);*/
/*			else*/
				m92_raster_irq_position=((pf4_control[7]<<8) | pf4_control[6])-128;
/*			if (offset==7)*/
/*				log_cb(RETRO_LOG_DEBUG, LOGPRE "%06x: Raster %d %d\n",activecpu_get_pc(),offset, m92_raster_irq_position);*/
			break;
	}
}

/*****************************************************************************/

VIDEO_START( m92 )
{
	if (RYPELEO_SPEEDUP) {
		pf1_hlayer = tilemap_create(
			get_pf1_htile_info,tilemap_scan_rows,
			TILEMAP_SPLIT,
			8,8,
			64,64
		);
		pf1_layer = tilemap_create(
				get_pf1_ltile_info,tilemap_scan_rows,
				TILEMAP_SPLIT,
				8,8,
				64,64
			);
		tilemap_set_transmask(pf1_hlayer,0,0xffff,0x0001);
		tilemap_set_transmask(pf1_hlayer,1,0x00ff,0xff01);
		tilemap_set_transmask(pf1_hlayer,2,0x0001,0xffff);
	}
	else
		pf1_layer = tilemap_create(
			get_pf1_tile_info,tilemap_scan_rows,
			TILEMAP_SPLIT,
			8,8,
			64,64
		);

	pf2_layer = tilemap_create(
		get_pf2_tile_info,tilemap_scan_rows,
		TILEMAP_SPLIT,
		8,8,
		64,64
	);

	pf3_layer = tilemap_create(
		get_pf3_tile_info,tilemap_scan_rows,
		TILEMAP_SPLIT,
		8,8,
		64,64
	);

	pf1_wide_layer = tilemap_create(
		get_pf1_wide_tile_info,tilemap_scan_rows,
		TILEMAP_SPLIT,
		8,8,
		128,64
	);

	pf3_wide_layer = tilemap_create(
		get_pf3_wide_tile_info,tilemap_scan_rows,
		TILEMAP_SPLIT,
		8,8,
		128,64
	);

	paletteram = auto_malloc(0x1000);

	if (!pf1_layer || !pf2_layer || !pf3_layer || !pf1_wide_layer || !pf3_wide_layer || !paletteram)
		return 1;

	/* split type 0 - totally transparent in front half */
	tilemap_set_transmask(pf1_layer,0,0xffff,0x0001);
	tilemap_set_transmask(pf2_layer,0,0xffff,0x0001);
	tilemap_set_transmask(pf3_layer,0,0xffff,0x0000);
	tilemap_set_transmask(pf1_wide_layer,0,0xffff,0x0001);
	tilemap_set_transmask(pf3_wide_layer,0,0xffff,0x0000);
	/* split type 1 - pens 0-7 transparent in front half */
	tilemap_set_transmask(pf1_layer,1,0x00ff,0xff01);
	tilemap_set_transmask(pf2_layer,1,0x00ff,0xff01);
	tilemap_set_transmask(pf3_layer,1,0x00ff,0xff00);
	tilemap_set_transmask(pf1_wide_layer,1,0x00ff,0xff01);
	tilemap_set_transmask(pf3_wide_layer,1,0x00ff,0xff00);
	/* split type 2 - pen 0 transparent in front half */
	tilemap_set_transmask(pf1_layer,2,0x0001,0xffff);
	tilemap_set_transmask(pf2_layer,2,0x0001,0xffff);
	tilemap_set_transmask(pf3_layer,2,0x0001,0xfffe);
	tilemap_set_transmask(pf1_wide_layer,2,0x0001,0xffff);
	tilemap_set_transmask(pf3_wide_layer,2,0x0001,0xfffe);

	pf1_vram_ptr=pf2_vram_ptr=pf3_vram_ptr=0;
	pf1_enable=pf2_enable=pf3_enable=0;
	pf1_rowscroll=pf2_rowscroll=pf3_rowscroll=0;
	pf1_shape=pf2_shape=pf3_shape=0;

	memset(spriteram,0,0x800);
	memset(buffered_spriteram,0,0x800);

	state_save_register_int  ("video", 0, "pf1_vram_ptr",            &pf1_vram_ptr);
	state_save_register_int  ("video", 0, "pf1_shape",               &pf1_shape);
	state_save_register_int  ("video", 0, "pf1_enable",              &pf1_enable);
	state_save_register_int  ("video", 0, "pf1_rowscroll",           &pf1_rowscroll);
	state_save_register_int  ("video", 0, "pf1_vram_ptr",            &pf1_vram_ptr);
	state_save_register_UINT8("video", 0, "pf1_control",    (UINT8*) pf1_control, sizeof(pf1_control));

	state_save_register_int  ("video", 0, "pf2_vram_ptr",            &pf2_vram_ptr);
	state_save_register_int  ("video", 0, "pf2_shape",               &pf2_shape);
	state_save_register_int  ("video", 0, "pf2_enable",              &pf2_enable);
	state_save_register_int  ("video", 0, "pf2_rowscroll",           &pf2_rowscroll);
	state_save_register_int  ("video", 0, "pf2_vram_ptr",            &pf2_vram_ptr);
	state_save_register_UINT8("video", 0, "pf2_control",    (UINT8*) pf2_control, sizeof(pf2_control));

	state_save_register_int  ("video", 0, "pf3_vram_ptr",            &pf3_vram_ptr);
	state_save_register_int  ("video", 0, "pf3_shape",               &pf3_shape);
	state_save_register_int  ("video", 0, "pf3_enable",              &pf3_enable);
	state_save_register_int  ("video", 0, "pf3_rowscroll",           &pf3_rowscroll);
	state_save_register_int  ("video", 0, "pf3_vram_ptr",            &pf3_vram_ptr);
	state_save_register_UINT8("video", 0, "pf3_control",    (UINT8*) pf3_control, sizeof(pf3_control));

	state_save_register_UINT8("video", 0, "pf4_control",    (UINT8*) pf4_control, sizeof(pf4_control));

	state_save_register_int  ("video", 0, "m92_sprite_list",         &m92_sprite_list);
	state_save_register_int  ("video", 0, "m92_raster_irq_position", &m92_raster_irq_position);
	state_save_register_int  ("video", 0, "m92_sprite_buffer_busy",  &m92_sprite_buffer_busy);
	state_save_register_int  ("video", 0, "m92_palette_bank",        &m92_palette_bank);

	state_save_register_UINT8("video", 0, "paletteram",              paletteram, 0x1000);

	return 0;
}

/*****************************************************************************/

static void m92_drawsprites(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	int offs=0;

	while (offs<m92_sprite_list) {
		int x,y,sprite,colour,fx,fy,x_multi,y_multi,i,j,s_ptr,pri;

		y=(buffered_spriteram[offs+0] | (buffered_spriteram[offs+1]<<8))&0x1ff;
		x=(buffered_spriteram[offs+6] | (buffered_spriteram[offs+7]<<8))&0x1ff;

		if ((buffered_spriteram[offs+4]&0x80)==0x80) pri=0; else pri=2;

		x = x - 16;
		y = 512 - 16 - y;

	    sprite=(buffered_spriteram[offs+2] | (buffered_spriteram[offs+3]<<8));
		colour=buffered_spriteram[offs+4]&0x7f;

		fx=buffered_spriteram[offs+5]&1;
		fy=(buffered_spriteram[offs+5]&2)>>1;
		y_multi=(buffered_spriteram[offs+1]>>1)&0x3;
		x_multi=(buffered_spriteram[offs+1]>>3)&0x3;

		y_multi=1 << y_multi; /* 1, 2, 4 or 8 */
		x_multi=1 << x_multi; /* 1, 2, 4 or 8 */

		if (fx) x+=16 * (x_multi - 1);
		for (j=0; j<x_multi; j++)
		{
			s_ptr=8 * j;
			if (!fy) s_ptr+=y_multi-1;

			for (i=0; i<y_multi; i++)
			{
				if (flip_screen) {
					int ffx=fx,ffy=fy;
					if (ffx) ffx=0; else ffx=1;
					if (ffy) ffy=0; else ffy=1;
					pdrawgfx(bitmap,Machine->gfx[1],
							sprite + s_ptr,
							colour,
							ffx,ffy,
							496-x,496-(y-i*16),
							cliprect,TRANSPARENCY_PEN,0,pri);
				} else {
					pdrawgfx(bitmap,Machine->gfx[1],
							sprite + s_ptr,
							colour,
							fx,fy,
							x,y-i*16,
							cliprect,TRANSPARENCY_PEN,0,pri);
				}
				if (fy) s_ptr++; else s_ptr--;
			}
			if (fx) x-=16; else x+=16;
			offs+=8;
		}
	}
}

/*****************************************************************************/

VIDEO_UPDATE( m92 )
{
#ifdef MAME_DEBUG
	/* Screen refresh is handled by raster interrupt routine, here
		we just check the keyboard */
	if (keyboard_pressed_memory(KEYCODE_F1)) {
		m92_raster_enable ^= 1;
		if (m92_raster_enable)
			usrintf_showmessage("Raster IRQ enabled");
		else
			usrintf_showmessage("Raster IRQ disabled");
	}
#endif

	/* Flipscreen appears hardwired to the dipswitch - strange */
	if (readinputport(5)&1)
		flip_screen_set(0);
	else
		flip_screen_set(1);
}

static void m92_update_scroll_positions(void)
{
	int i,pf1_off,pf2_off,pf3_off;

	/*	Playfield 3 rowscroll data is 0xdfc00 - 0xdffff
		Playfield 2 rowscroll data is 0xdf800 - 0xdfbff
		Playfield 1 rowscroll data is 0xdf400 - 0xdf7ff

		It appears to be hardwired to those locations.

		In addition, each playfield is staggered 2 pixels horizontally from the
		previous one.  This is most obvious in Hook & Blademaster.

	*/

	if (flip_screen) {
		pf1_off=-25;
		pf2_off=-27;
		pf3_off=-29;
	} else {
		pf1_off=0;
		pf2_off=2;
		pf3_off=4;
	}

	if (pf1_rowscroll) {
		tilemap_set_scroll_rows(pf1_layer,512);
		tilemap_set_scroll_rows(pf1_wide_layer,512);
		for (i=0; i<1024; i+=2)
			tilemap_set_scrollx( pf1_layer,i/2, (m92_vram_data[0xf400+i]+(m92_vram_data[0xf401+i]<<8))-pf1_off);
		for (i=0; i<1024; i+=2)
			tilemap_set_scrollx( pf1_wide_layer,i/2, (m92_vram_data[0xf400+i]+(m92_vram_data[0xf401+i]<<8))-pf1_off+256);
	} else {
		tilemap_set_scroll_rows(pf1_layer,1);
		tilemap_set_scroll_rows(pf1_wide_layer,1);
		tilemap_set_scrollx( pf1_layer,0, (pf1_control[5]<<8)+pf1_control[4]-pf1_off );
		tilemap_set_scrollx( pf1_wide_layer,0, (pf1_control[5]<<8)+pf1_control[4]+256-pf1_off );
	}

	if (pf2_rowscroll) {
		tilemap_set_scroll_rows(pf2_layer,512);
		for (i=0; i<1024; i+=2)
			tilemap_set_scrollx( pf2_layer,i/2, (m92_vram_data[0xf800+i]+(m92_vram_data[0xf801+i]<<8))-pf2_off);
	} else {
		tilemap_set_scroll_rows(pf2_layer,1);
		tilemap_set_scrollx( pf2_layer,0, (pf2_control[5]<<8)+pf2_control[4]-pf2_off );
	}

	if (pf3_rowscroll) {
		tilemap_set_scroll_rows(pf3_layer,512);
		for (i=0; i<1024; i+=2)
			tilemap_set_scrollx( pf3_layer,i/2, (m92_vram_data[0xfc00+i]+(m92_vram_data[0xfc01+i]<<8))-pf3_off);
		tilemap_set_scroll_rows(pf3_wide_layer,512);
		for (i=0; i<1024; i+=2)
			tilemap_set_scrollx( pf3_wide_layer,i/2, (m92_vram_data[0xfc00+i]+(m92_vram_data[0xfc01+i]<<8))-pf3_off+256);
	} else {
		tilemap_set_scroll_rows(pf3_layer,1);
		tilemap_set_scrollx( pf3_layer,0, (pf3_control[5]<<8)+pf3_control[4]-pf3_off );
		tilemap_set_scroll_rows(pf3_wide_layer,1);
		tilemap_set_scrollx( pf3_wide_layer,0, (pf3_control[5]<<8)+pf3_control[4]-pf3_off+256 );
	}

	tilemap_set_scrolly( pf1_layer,0, (pf1_control[1]<<8)+pf1_control[0] );
	tilemap_set_scrolly( pf2_layer,0, (pf2_control[1]<<8)+pf2_control[0] );
	tilemap_set_scrolly( pf3_layer,0, (pf3_control[1]<<8)+pf3_control[0] );
	tilemap_set_scrolly( pf1_wide_layer,0, (pf1_control[1]<<8)+pf1_control[0] );
	tilemap_set_scrolly( pf3_wide_layer,0, (pf3_control[1]<<8)+pf3_control[0] );

	if (RYPELEO_SPEEDUP) {
		tilemap_set_scroll_rows(pf1_hlayer,1);
		tilemap_set_scrollx( pf1_hlayer,0, (pf1_control[5]<<8)+pf1_control[4] );
		tilemap_set_scrolly( pf1_hlayer,0, (pf1_control[1]<<8)+pf1_control[0] );
	}
}

/*****************************************************************************/

static void m92_screenrefresh(struct mame_bitmap *bitmap,const struct rectangle *cliprect)
{
	fillbitmap(priority_bitmap,0,cliprect);

	if (pf3_enable) {
		tilemap_draw(bitmap,cliprect,pf3_wide_layer,TILEMAP_BACK,0);
		tilemap_draw(bitmap,cliprect,pf3_layer,TILEMAP_BACK,0);
	}
	else
		fillbitmap(bitmap,Machine->pens[0],cliprect);

	tilemap_draw(bitmap,cliprect,pf2_layer,TILEMAP_BACK,0);
	tilemap_draw(bitmap,cliprect,pf1_wide_layer,TILEMAP_BACK,0);
	if (RYPELEO_SPEEDUP && pf1_vram_ptr==0xc000)
		tilemap_draw(bitmap,cliprect,pf1_hlayer,TILEMAP_BACK,0);
	else
		tilemap_draw(bitmap,cliprect,pf1_layer,TILEMAP_BACK,0);

	tilemap_draw(bitmap,cliprect,pf3_wide_layer,TILEMAP_FRONT,1);
	tilemap_draw(bitmap,cliprect,pf3_layer,TILEMAP_FRONT,1);
	tilemap_draw(bitmap,cliprect,pf2_layer,TILEMAP_FRONT,1);
	tilemap_draw(bitmap,cliprect,pf1_wide_layer,TILEMAP_FRONT,1);
	if (RYPELEO_SPEEDUP && pf1_vram_ptr==0xc000)
		tilemap_draw(bitmap,cliprect,pf1_hlayer,TILEMAP_FRONT,1);
	else
		tilemap_draw(bitmap,cliprect,pf1_layer,TILEMAP_FRONT,1);

	m92_drawsprites(bitmap,cliprect);
}

void m92_vh_raster_partial_refresh(struct mame_bitmap *bitmap,int start_line,int end_line)
{
	struct rectangle clip;

	clip.min_x = 0;/*Machine->visible_area.min_x;*/
	clip.max_x = 511;/*Machine->visible_area.max_x;*/
	clip.min_y = start_line+128;
	clip.max_y = end_line+128;
	if (clip.min_y < Machine->visible_area.min_y)
		clip.min_y = Machine->visible_area.min_y;
	if (clip.max_y > Machine->visible_area.max_y)
		clip.max_y = Machine->visible_area.max_y;

	if (clip.max_y > clip.min_y)
	{
		m92_update_scroll_positions();
		m92_screenrefresh(bitmap,&clip);
	}
}
