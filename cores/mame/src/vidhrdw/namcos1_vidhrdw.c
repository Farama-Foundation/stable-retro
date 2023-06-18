#include "driver.h"
#include "vidhrdw/generic.h"

#define get_gfx_pointer(gfxelement,c,line) (gfxelement->gfxdata + (c*gfxelement->height+line) * gfxelement->line_modulo)

#define SPRITECOLORS 2048
#define TILECOLORS 1536
#define BACKGROUNDCOLOR (SPRITECOLORS+2*TILECOLORS)

#define FG_OFFSET 0x7000
#define MAX_PLAYFIELDS 6
#define MAX_SPRITES 127

#define UPDATE_SUBLEAD 0x01
#define UPDATE_TIED 0x02
#define USE_SP_BUFFER 0x04
#define UPDATE_OVERRIDE 0x08
#define MAIN_COMPLETE 0x10
#define SUB_COMPLETE 0x20

/*#define TRY_PDRAWGFX 1*/
WRITE_HANDLER( namcos1_main_update_w );
WRITE_HANDLER( namcos1_sub_update_w );

struct playfield
{
	int color;
};

/*
  video ram map
  0000-1fff : scroll playfield (0) : 64*64*2
  2000-3fff : scroll playfield (1) : 64*64*2
  4000-5fff : scroll playfield (2) : 64*64*2
  6000-6fff : scroll playfield (3) : 64*32*2
  7000-700f : ?
  7010-77ef : fixed playfield (4)  : 36*28*2
  77f0-77ff : ?
  7800-780f : ?
  7810-7fef : fixed playfield (5)  : 36*28*2
  7ff0-7fff : ?
*/
static unsigned char *namcos1_videoram;

/*
  paletteram map (s1ram  0x0000-0x7fff)
  0000-17ff : palette page0 : sprite
  2000-37ff : palette page1 : playfield
  4000-57ff : palette page2 : playfield (shadow)
  6000-7fff : work ram ?
*/
static unsigned char *namcos1_paletteram;

/*
  controlram map (s1ram 0x8000-0x9fff)
  0000-07ff : work ram
  0800-0fef : sprite ram    : 0x10 * 127
  0ff0-0fff : display control register
  1000-1fff : playfield control register
*/
static data8_t *namcos1_controlram;

/* palette dirty information */
static unsigned char sprite_palette_state[MAX_SPRITES+1];
static unsigned char tilemap_palette_state[MAX_PLAYFIELDS];

/* per game scroll adjustment */
static int scrolloffsX[4];
static int scrolloffsY[4];

static data8_t namcos1_playfield_control[0x100];
struct playfield playfields[MAX_PLAYFIELDS];

static struct tilemap *tilemap[MAX_PLAYFIELDS];
static UINT8 *mpMaskData;

static int sprite_fixed_sx;
static int sprite_fixed_sy;
static int flipscreen;

static unsigned priority_xlat[8] = {0,1,2,3,4,5,6,7};
static int update_status, idle_counter, idle_threshold;
static data8_t *sp_updatebuffer, *sp_backbuffer;
extern int namcos1_game_id;

static void namcos1_set_flipscreen(int flip)
{
	int i;

	int pos_x[] = {0x0b0,0x0b2,0x0b3,0x0b4};
	int pos_y[] = {0x108,0x108,0x108,0x008};
	int neg_x[] = {0x1d0,0x1d2,0x1d3,0x1d4};
	int neg_y[] = {0x1e8,0x1e8,0x1e8,0x0e8};

	flipscreen = flip;
	if(!flip)
	{
		for ( i = 0; i < 4; i++ ) {
			scrolloffsX[i] = pos_x[i];
			scrolloffsY[i] = pos_y[i];
		}
	}
	else
	{
		for ( i = 0; i < 4; i++ ) {
			scrolloffsX[i] = neg_x[i];
			scrolloffsY[i] = neg_y[i];
		}
	}
	tilemap_set_flip(ALL_TILEMAPS,flipscreen ? TILEMAP_FLIPX|TILEMAP_FLIPY : 0);
}

static WRITE_HANDLER( namcos1_playfield_control_w )
{
	namcos1_playfield_control[offset] = data;
	/* 0-15 : scrolling */
	if ( offset < 16 )
	{
	}
	/* 16-21 : priority */
	else if ( offset < 22 )
	{
	}
	/* 22,23 unused */
	else if (offset < 24)
	{
	}
	/* 24-29 palette */
	else
	if ( offset < 30 )
	{
		int whichone = offset - 24;
		if (playfields[whichone].color != (data & 7))
		{
			playfields[whichone].color = data & 7;
			tilemap_palette_state[whichone] = 1;
		}
	}
}

READ_HANDLER( namcos1_videoram_r )
{
	return namcos1_videoram[offset];
}

WRITE_HANDLER( namcos1_videoram_w )
{
	if (namcos1_videoram[offset] != data)
	{
		namcos1_videoram[offset] = data;
		if(offset < FG_OFFSET)
		{   /* background 0-3 */
			int layer = offset>>13;
			int num = (offset &= 0x1fff)>>1;
			tilemap_mark_tile_dirty(tilemap[layer],num);
		}
		else
		{   /* foreground 4-5 */
			int layer = (offset>>11 & 1) + 4;
			int num = ((offset&0x7ff)-0x10)>>1;
			if (num >= 0 && num < 0x3f0)
				tilemap_mark_tile_dirty(tilemap[layer],num);
		}
	}
}

READ_HANDLER( namcos1_paletteram_r )
{
	return namcos1_paletteram[offset];
}

WRITE_HANDLER( namcos1_paletteram_w )
{
	namcos1_paletteram[offset] = data;
	if ((offset&0x1fff) < 0x1800)
	{
		if (offset < 0x2000)
		{
			sprite_palette_state[(offset&0x7f0)>>4] = 1;
		}
		else
		{
			int i,color;

			color = (offset&0x700)>>8;
			for(i=0;i<MAX_PLAYFIELDS;i++)
			{
				if (playfields[i].color == color)
					tilemap_palette_state[i] = 1;
			}
		}
	}
}

static void namcos1_palette_refresh(int start,int offset,int num)
{
	int color;

	offset = (offset/0x800)*0x2000 + (offset&0x7ff);

	for (color = start; color < start + num; color++)
	{
		int r,g,b;
		r = namcos1_paletteram[offset];
		g = namcos1_paletteram[offset + 0x0800];
		b = namcos1_paletteram[offset + 0x1000];
		palette_set_color(color,r,g,b);

		if (offset >= 0x2000)
		{
			r = namcos1_paletteram[offset + 0x2000];
			g = namcos1_paletteram[offset + 0x2800];
			b = namcos1_paletteram[offset + 0x3000];
			palette_set_color(color+TILECOLORS,r,g,b);
		}
		offset++;
	}
}

/* display control block write */
/*
0-3  unknown
4-5  sprite offset x
6    flip screen
7    sprite offset y
8-15 unknown
*/
static WRITE_HANDLER( namcos1_displaycontrol_w )
{
	unsigned char *disp_reg = &namcos1_controlram[0xff0];
	int newflip;

	switch(offset)
	{
	case 0x02: /* ?? */
		break;
	case 0x04: /* sprite offset X */
	case 0x05:
		sprite_fixed_sx = disp_reg[4]*256+disp_reg[5] - 151;
		if( sprite_fixed_sx > 480 ) sprite_fixed_sx -= 512;
		if( sprite_fixed_sx < -32 ) sprite_fixed_sx += 512;
		break;
	case 0x06: /* flip screen */
		newflip = (disp_reg[6]&1)^0x01;
		if(flipscreen != newflip)
		{
			namcos1_set_flipscreen(newflip);
		}
		break;
	case 0x07: /* sprite offset Y */
		sprite_fixed_sy = 239 - disp_reg[7];
		break;
	case 0x0a: /* ?? */
		/* 00 : blazer,dspirit,quester */
		/* 40 : others */
		break;
	case 0x0e: /* ?? */
		/* 00 : blazer,dangseed,dspirit,pacmania,quester */
		/* 06 : others */
	case 0x0f: /* ?? */
		/* 00 : dangseed,dspirit,pacmania */
		/* f1 : blazer */
		/* f8 : galaga88,quester */
		/* e7 : others */
		break;
	}
#if 0
	{
		char buf[80];
		sprintf(buf,"%02x:%02x:%02x:%02x:%02x%02x,%02x,%02x,%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
		disp_reg[0],disp_reg[1],disp_reg[2],disp_reg[3],
		disp_reg[4],disp_reg[5],disp_reg[6],disp_reg[7],
		disp_reg[8],disp_reg[9],disp_reg[10],disp_reg[11],
		disp_reg[12],disp_reg[13],disp_reg[14],disp_reg[15]);
		usrintf_showmessage(buf);
	}
#endif
}

WRITE_HANDLER( namcos1_videocontrol_w )
{
	int olddata = olddata = namcos1_controlram[offset];
	namcos1_controlram[offset] = data;

	/* 0000-07ff work ram */
	if (offset < 0x800)
		return;
	/* 0800-0fef sprite ram */
	else if (offset < 0xff0) switch (namcos1_game_id)
	{
		case 0x0182: /* only Rompers use it but this behavior may not be unique*/
			if ((offset & 0x0f) == 7 && abs(olddata - data) == 0xff)
				namcos1_controlram[offset-1] |= 1;
		break;
	}
	/* 0ff0-0fff display control ram */
	else if (offset < 0x1000)
		namcos1_displaycontrol_w(offset & 0x0f, data);
	/* 1000-1fff control ram */
	else
		namcos1_playfield_control_w(offset & 0xff, data);
}

/* tilemap callback */
static INLINE void background_get_info(int tile_index,int info_color,data8_t *info_vram)
{
	int code;

	tile_index <<= 1;
	code = info_vram[tile_index+1]+((info_vram[tile_index]&0x3f)<<8);
	SET_TILE_INFO(0,code,info_color,0);
	tile_info.mask_data = &mpMaskData[code<<3];
}

static INLINE void foreground_get_info(int tile_index,int info_color,data8_t *info_vram)
{
	int code;

	tile_index <<= 1;
	code = info_vram[tile_index+1]+((info_vram[tile_index]&0x3f)<<8);
	SET_TILE_INFO(0,code,info_color,0);
	tile_info.mask_data = &mpMaskData[code<<3];
}

static void background_get_info0(int tile_index) { background_get_info(tile_index,0,&namcos1_videoram[0x0000]); }
static void background_get_info1(int tile_index) { background_get_info(tile_index,1,&namcos1_videoram[0x2000]); }
static void background_get_info2(int tile_index) { background_get_info(tile_index,2,&namcos1_videoram[0x4000]); }
static void background_get_info3(int tile_index) { background_get_info(tile_index,3,&namcos1_videoram[0x6000]); }
static void foreground_get_info4(int tile_index) { foreground_get_info(tile_index,4,&namcos1_videoram[FG_OFFSET+0x010]); }
static void foreground_get_info5(int tile_index) { foreground_get_info(tile_index,5,&namcos1_videoram[FG_OFFSET+0x810]); }

#if 0
static void update_playfield( int layer )
{
	/* for background , set scroll position */
	if( layer < 4 )
	{
		int scrollx = -(namcos1_playfield_control[layer*4+1] + 256*namcos1_playfield_control[layer*4+0]) + scrolloffsX[layer];
		int scrolly = -(namcos1_playfield_control[layer*4+3] + 256*namcos1_playfield_control[layer*4+2]) + scrolloffsY[layer];

		if ( flipscreen ) {
			scrollx = -scrollx;
			scrolly = -scrolly;
		}
		/* set scroll */
		tilemap_set_scrollx(tilemap[layer],0,scrollx);
		tilemap_set_scrolly(tilemap[layer],0,scrolly);
	}
}
#endif

#ifdef TRY_PDRAWGFX
static void draw_sprites(struct mame_bitmap *bitmap, const struct rectangle *cliprect, data8_t *namcos1_spriteram)
{
	static const int sprite_sizemap[4] = {16,8,32,4};
	struct rectangle rect;
	struct GfxElement *gfx;
	int sx,sy,code,color,flipx,flipy;
	int width,height,left,top;
	int offs, flags;
	unsigned primask;

	gfx = Machine->gfx[1];

	/* the last 0x10 bytes are control registers, not a sprite */
	for (offs=0x7e0; offs>=0; offs-=0x10)
	{
		width =  sprite_sizemap[(namcos1_spriteram[offs + 4]>>6)&3];
		height = sprite_sizemap[(namcos1_spriteram[offs + 8]>>1)&3];
		flipx = ((namcos1_spriteram[offs + 4]>>5)&1) ^ flipscreen;
		flipy = (namcos1_spriteram[offs + 8]&1) ^ flipscreen;
		left = (namcos1_spriteram[offs + 4]&0x18) & (~(width-1));
		top =  (namcos1_spriteram[offs + 8]&0x18) & (~(height-1));
		code = ((namcos1_spriteram[offs + 4]&7)<<8) + namcos1_spriteram[offs + 5];
		color = namcos1_spriteram[offs + 6]>>1;
		flags = (color != 0x7f) ? TRANSPARENCY_PEN : TRANSPARENCY_PEN_TABLE;
		primask = priority_xlat[namcos1_spriteram[offs + 8]>>5&7];

		/* sx */
		sx = ((namcos1_spriteram[offs + 6]&1)<<8) + namcos1_spriteram[offs + 7];
		sx += sprite_fixed_sx;

		if(flipscreen) sx = 210 - sx - width;

		if( sx > 480  ) sx -= 512;
		if( sx < -32  ) sx += 512;
		if( sx < -224 ) sx += 512;

		/* sy */
		sy = sprite_fixed_sy - namcos1_spriteram[offs + 9];

		if(flipscreen) sy = 222 - sy;
		else sy = sy - height;

		if( sy > 224 ) sy -= 256;
		if( sy < -32 ) sy += 256;

		rect.min_x=sx;
		rect.max_x=sx+(width-1);
		rect.min_y=sy;
		rect.max_y=sy+(height-1);

		if (cliprect->min_x > rect.min_x) rect.min_x = cliprect->min_x;
		if (cliprect->max_x < rect.max_x) rect.max_x = cliprect->max_x;
		if (cliprect->min_y > rect.min_y) rect.min_y = cliprect->min_y;
		if (cliprect->max_y < rect.max_y) rect.max_y = cliprect->max_y;

		if (flipx) sx -= 32-width-left;
		else sx -= left;
		if (flipy) sy -= 32-height-top;
		else sy -= top;

		/* Doesn't work. Can pdrawgfx handle more than 4 layers?*/
		pdrawgfx(bitmap, gfx,
				code,
				color,
				flipx, flipy,
				sx, sy,
				&rect, flags, 0x0f, primask);
	}
}
#else
static void draw_sprites(struct mame_bitmap *bitmap,const struct rectangle *cliprect,data8_t *namcos1_spriteram,int priority)
{
	static const int sprite_sizemap[4] = {16,8,32,4};
	struct rectangle rect;
	struct GfxElement *gfx;
	int sx,sy,code,color,flipx,flipy;
	int width,height,left,top;
	int offs, flags;

	gfx = Machine->gfx[1];

	/* the last 0x10 bytes are control registers, not a sprite */
	for (offs = 0;offs < 0x800-0x10;offs += 0x10)
	{
		if (priority_xlat[namcos1_spriteram[offs + 8]>>5&7] != priority) continue;

		width =  sprite_sizemap[(namcos1_spriteram[offs + 4]>>6)&3];
		height = sprite_sizemap[(namcos1_spriteram[offs + 8]>>1)&3];
		flipx = ((namcos1_spriteram[offs + 4]>>5)&1) ^ flipscreen;
		flipy = (namcos1_spriteram[offs + 8]&1) ^ flipscreen;
		left = (namcos1_spriteram[offs + 4]&0x18) & (~(width-1));
		top =  (namcos1_spriteram[offs + 8]&0x18) & (~(height-1));
		code = ((namcos1_spriteram[offs + 4]&7)<<8) + namcos1_spriteram[offs + 5];
		color = namcos1_spriteram[offs + 6]>>1;
		flags = (color != 0x7f) ? TRANSPARENCY_PEN : TRANSPARENCY_PEN_TABLE;

		/* sx */
		sx = ((namcos1_spriteram[offs + 6]&1)<<8) + namcos1_spriteram[offs + 7];
		sx += sprite_fixed_sx;

		if(flipscreen) sx = 210 - sx - width;

		if( sx > 480  ) sx -= 512;
		if( sx < -32  ) sx += 512;
		if( sx < -224 ) sx += 512;

		/* sy */
		sy = sprite_fixed_sy - namcos1_spriteram[offs + 9];

		if(flipscreen) sy = 222 - sy;
		else sy = sy - height;

		if( sy > 224 ) sy -= 256;
		if( sy < -32 ) sy += 256;

		rect.min_x=sx;
		rect.max_x=sx+(width-1);
		rect.min_y=sy;
		rect.max_y=sy+(height-1);

		if (cliprect->min_x > rect.min_x) rect.min_x = cliprect->min_x;
		if (cliprect->max_x < rect.max_x) rect.max_x = cliprect->max_x;
		if (cliprect->min_y > rect.min_y) rect.min_y = cliprect->min_y;
		if (cliprect->max_y < rect.max_y) rect.max_y = cliprect->max_y;

		if (flipx) sx -= 32-width-left;
		else sx -= left;
		if (flipy) sy -= 32-height-top;
		else sy -= top;

		drawgfx(bitmap,gfx,
				code,
				color,
				flipx,flipy,
				sx,sy,
				&rect,flags,0x0f);
	}
}
#endif

static void namcos1_draw_screen(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	int i, j, scrollx, scrolly, priority;

	for (i=0; i<128; i++)
	{
		j = i << 4;
		if (sprite_palette_state[i])
		{
			sprite_palette_state[i] = 0;
			namcos1_palette_refresh(j, j, 15);
		}
	}

	for (i=0; i<MAX_PLAYFIELDS; i++)
	{
		if (tilemap_palette_state[i])
		{
			tilemap_palette_state[i] = 0;
			namcos1_palette_refresh((i<<8)+2048, (playfields[i].color<<8)+2048, 256);
		}
	}

	for (i=0; i<4; i++)
	{
		j = i << 2;
		scrollx = -( namcos1_playfield_control[j+1] + (namcos1_playfield_control[j+0]<<8) ) + scrolloffsX[i];
		scrolly = -( namcos1_playfield_control[j+3] + (namcos1_playfield_control[j+2]<<8) ) + scrolloffsY[i];

		if (flipscreen)
		{
			scrollx = -scrollx;
			scrolly = -scrolly;
		}

		tilemap_set_scrollx(tilemap[i],0,scrollx);
		tilemap_set_scrolly(tilemap[i],0,scrolly);
	}

#ifndef TRY_PDRAWGFX
	fillbitmap(priority_bitmap, 0, cliprect);
#endif

	/* background color */
	fillbitmap(bitmap, Machine->pens[BACKGROUNDCOLOR], cliprect);

	/* bit 0-2 priority */
	/* bit 3   disable	*/
	for (i=0, priority=0; priority<=7; priority++)
	{
#ifdef TRY_PDRAWGFX
		i = 1 << priority;
#endif
		/* scroll maps are refreshed when f600 is called*/
		if (namcos1_playfield_control[16] == priority) tilemap_draw(bitmap,cliprect,tilemap[0],0,i);
		if (namcos1_playfield_control[17] == priority) tilemap_draw(bitmap,cliprect,tilemap[1],0,i);
		if (namcos1_playfield_control[18] == priority) tilemap_draw(bitmap,cliprect,tilemap[2],0,i);
		if (namcos1_playfield_control[19] == priority) tilemap_draw(bitmap,cliprect,tilemap[3],0,i);

		/* text maps should be refreshed independently regardless of interrupt status but due to*/
		/* priority constraints they be drawn here*/
		if (namcos1_playfield_control[20] == priority) tilemap_draw(bitmap,cliprect,tilemap[4],0,i);
		if (namcos1_playfield_control[21] == priority) tilemap_draw(bitmap,cliprect,tilemap[5],0,i);

#ifndef TRY_PDRAWGFX
		draw_sprites(bitmap, cliprect, sp_updatebuffer, priority);
#endif
	}

#ifdef TRY_PDRAWGFX
		draw_sprites(bitmap, cliprect, sp_updatebuffer);
#endif
}


VIDEO_START( namcos1 )
{
	int i;

	mpMaskData = (UINT8 *)memory_region( REGION_GFX1 );

	/* set table for sprite color == 0x7f */
	for(i=0;i<=15;i++)
		gfx_drawmode_table[i] = DRAWMODE_SHADOW;

	/* set static memory points */
	namcos1_paletteram = memory_region(REGION_USER2);
	namcos1_controlram = memory_region(REGION_USER2) + 0x8000;

	/* allocate videoram */
	namcos1_videoram = auto_malloc(0x9000);
	memset(namcos1_videoram, 0, 0x9000);

	/* initialize playfields */
	tilemap[0] = tilemap_create(background_get_info0,tilemap_scan_rows,TILEMAP_BITMASK,8,8,64,64);
	tilemap[1] = tilemap_create(background_get_info1,tilemap_scan_rows,TILEMAP_BITMASK,8,8,64,64);
	tilemap[2] = tilemap_create(background_get_info2,tilemap_scan_rows,TILEMAP_BITMASK,8,8,64,64);
	tilemap[3] = tilemap_create(background_get_info3,tilemap_scan_rows,TILEMAP_BITMASK,8,8,64,32);
	tilemap[4] = tilemap_create(foreground_get_info4,tilemap_scan_rows,TILEMAP_BITMASK,8,8,36,28);
	tilemap[5] = tilemap_create(foreground_get_info5,tilemap_scan_rows,TILEMAP_BITMASK,8,8,36,28);

	if (!tilemap[0] || !tilemap[1] || !tilemap[2] || !tilemap[3] || !tilemap[4] || !tilemap[5] || !namcos1_videoram)
		return 1;

	namcos1_set_flipscreen(0);

	/* initialize sprites and display controller */
	for(i=0;i<0xf;i++)
		namcos1_displaycontrol_w(i,0);
	for(i=0;i<0xff;i++)
		namcos1_playfield_control_w(i,0);

	for (i = 0;i < TILECOLORS;i++)
	{
		palette_shadow_table[Machine->pens[i+SPRITECOLORS]] = Machine->pens[i+SPRITECOLORS+TILECOLORS];
	}

	update_status = 0 | USE_SP_BUFFER;
	idle_counter = 0;
	idle_threshold = Machine->drv->frames_per_second / 2;

	switch (namcos1_game_id) /* custom settings*/
	{
/* use default */
		/*case 0x0487:*/ /* Youkai Douchuuki*/
		/*case 0x0152:*/ /* Marchen Maze*/
		/*case 0x0154:*/ /* World Stadium*/
		/*case 0x0184:*/ /* World Stadium 89*/
		/*case 0x0310:*/ /* World Stadium 90*/
		/*case 0x0143:*/ /* World Court*/
		/*case 0x0181:*/ /* Splatter House*/
		/*case 0x1288:*/ /* Face Off*/
		/*case 0xff90:*/ /* Puzzle Club*/
		/*break; */

		case 0x0588: /* Berabohm*/
		case 0x0787: /* Blazer*/
		case 0x0311: /* Soukoban DX*/
			update_status |= UPDATE_TIED;
		break;

		case 0x0151: /* Pacmania*/
		case 0x0155: /* Bakutotu*/
		case 0x0153: /* Galaga88*/
		case 0x0987: /* Quester*/
		case 0x0183: /* Blast Off*/
		case 0x0308: /* Dangerous Seed*/
		case 0x0309: /* Pistol Daimyo*/
		case 0x0185: /* Tank Force*/
			update_status |= UPDATE_SUBLEAD;
			update_status &= ~USE_SP_BUFFER;
		break;

		case 0x0136: /* Dragon Spirit*/
			priority_xlat[2] = 4;
		break;

		case 0x0182: /* Rompers*/
			priority_xlat[3] = 2;
		break;
	}

	if (!(update_status & UPDATE_OVERRIDE))
	{
		if (!(update_status & UPDATE_SUBLEAD))
		{
			install_mem_write_handler(0, 0xf600, 0xf600, namcos1_main_update_w);
			if (!(update_status & UPDATE_TIED) && update_status & USE_SP_BUFFER)
				install_mem_write_handler(1, 0xf600, 0xf600, namcos1_sub_update_w);
		}
		else
		{
			install_mem_write_handler(1, 0xf600, 0xf600, namcos1_main_update_w);
			if (!(update_status & UPDATE_TIED) && update_status & USE_SP_BUFFER)
				install_mem_write_handler(0, 0xf600, 0xf600, namcos1_sub_update_w);
		}
	}

	if (update_status & USE_SP_BUFFER)
	{
		sp_updatebuffer = namcos1_videoram + 0x8000;
		sp_backbuffer = namcos1_videoram + 0x8800;
	}
	else
		sp_updatebuffer = sp_backbuffer = &namcos1_controlram[0x800];

#ifdef TRY_PDRAWGFX /* there's not much information on priority masks and I'm not sure if this is right*/
	for (i=0; i<8; i++) priority_xlat[i] = ~( ( 1 << (i+1) ) - 1 );
#endif

	return 0;
}

WRITE_HANDLER( namcos1_main_update_w )
{
	idle_counter = 0;

	if (update_status & MAIN_COMPLETE) return;
	update_status |= MAIN_COMPLETE;

	if (update_status & UPDATE_TIED && update_status & USE_SP_BUFFER)
		memcpy(sp_backbuffer, &namcos1_controlram[0x800], 0x7f0); /* take a snapshot of current sprite RAM*/

	namcos1_draw_screen(Machine->scrbitmap, &Machine->visible_area);
}

WRITE_HANDLER( namcos1_sub_update_w )
{
	if (update_status & SUB_COMPLETE) return;
	update_status |= SUB_COMPLETE;

	memcpy(sp_backbuffer, &namcos1_controlram[0x800], 0x7f0); /* take a snapshot of current sprite RAM*/
}

VIDEO_UPDATE( namcos1 )
{
	data8_t *temp;

	if (idle_counter < idle_threshold && !(update_status & UPDATE_OVERRIDE))
	{
		idle_counter++;
		update_status &= ~(MAIN_COMPLETE | SUB_COMPLETE);
	}
	else
	{
		/* draw screen unconditionally if video goes idle for too long(required unless priority bitmap is used)*/
		if (update_status & USE_SP_BUFFER) memcpy(sp_backbuffer, &namcos1_controlram[0x800], 0x7f0);
		namcos1_draw_screen(bitmap, cliprect);
	}

	temp = sp_backbuffer; sp_backbuffer = sp_updatebuffer; sp_updatebuffer = temp; /* mature backbuffer*/
}
