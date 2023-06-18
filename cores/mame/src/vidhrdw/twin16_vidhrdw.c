/*

	Konami Twin16 Hardware - Video

	TODO:

	- convert background to tilemap
	- clean up sprite drawing
	- sprite-background priorities
	- rogue sprites in devilw
	- corrupt sprites in vulcan end sequence
	- sprite lag in devilw
	- sprite Y axis lag in vulcan
	- add shadow sprites (alpha blending) to sprites in at least devilw

*/

#include "driver.h"
#include "vidhrdw/generic.h"

extern UINT16 twin16_custom_vidhrdw;
extern UINT16 *twin16_gfx_rom;
extern UINT16 *twin16_videoram2;
extern UINT16 *twin16_sprite_gfx_ram;
extern UINT16 *twin16_tile_gfx_ram;
extern int twin16_spriteram_process_enable( void );

static int need_process_spriteram;
static UINT16 gfx_bank;
static UINT16 scrollx[3], scrolly[3];
static UINT16 video_register;

enum {
	TWIN16_SCREEN_FLIPY		= 0x01,	/* ? breaks devils world text layer */
	TWIN16_SCREEN_FLIPX		= 0x02,	/* confirmed: Hard Puncher Intro */
	TWIN16_UNKNOWN1			= 0x04,	/* ?Hard Puncher uses this */
	TWIN16_PLANE_ORDER		= 0x08,	/* confirmed: Devil Worlds */
	TWIN16_TILE_FLIPY		= 0x20	/* confirmed? Vulcan Venture */
};

static struct tilemap *fg_tilemap;

WRITE16_HANDLER( twin16_videoram2_w )
{
	int oldword = twin16_videoram2[offset];

	COMBINE_DATA(&twin16_videoram2[offset]);

	if (oldword != twin16_videoram2[offset])
	{
		tilemap_mark_tile_dirty(fg_tilemap, offset);
	}
}

WRITE16_HANDLER( twin16_paletteram_word_w )
{ /* identical to tmnt_paletteram_w*/
	int r, g, b;

	COMBINE_DATA(paletteram16 + offset);
	offset &= ~1;

	data = ((paletteram16[offset] & 0xff) << 8) | (paletteram16[offset + 1] & 0xff);

	r = (data >>  0) & 0x1f;
	g = (data >>  5) & 0x1f;
	b = (data >> 10) & 0x1f;

	r = (r << 3) | (r >> 2);
	g = (g << 3) | (g >> 2);
	b = (b << 3) | (b >> 2);

	palette_set_color(offset / 2, r, g, b);
}

WRITE16_HANDLER( fround_gfx_bank_w )
{
	COMBINE_DATA(&gfx_bank);
}

WRITE16_HANDLER( twin16_video_register_w )
{
	switch (offset) {
	case 0: 
		COMBINE_DATA( &video_register );

		flip_screen_x_set(video_register & TWIN16_SCREEN_FLIPX);

		if (twin16_custom_vidhrdw)
			flip_screen_y_set(video_register & TWIN16_SCREEN_FLIPY);
		else
			flip_screen_y_set(~video_register & TWIN16_SCREEN_FLIPY);

		break;

	case 1: COMBINE_DATA( &scrollx[0] ); break;
	case 2: COMBINE_DATA( &scrolly[0] ); break;
	case 3: COMBINE_DATA( &scrollx[1] ); break;
	case 4: COMBINE_DATA( &scrolly[1] ); break;
	case 5: COMBINE_DATA( &scrollx[2] ); break;
	case 6: COMBINE_DATA( &scrolly[2] ); break;

	default:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "unknown video_register write:%d", data );
		break;
	}
}

static void draw_sprite( /* slow slow slow, but it's ok for now */
	struct mame_bitmap *bitmap,
	const UINT16 *pen_data,
	const pen_t *pal_data,
	int xpos, int ypos,
	int width, int height,
	int flipx, int flipy, int pri )
{

	int x,y,pval;
	if( xpos>=320 ) xpos -= 65536;
	if( ypos>=256 ) ypos -= 65536;

	if (pri) pval=2; else pval=8;

	{
		{
			for( y=0; y<height; y++ )
			{
				int sy = (flipy)?(ypos+height-1-y):(ypos+y);
				if( sy>=16 && sy<256-16 )
				{
					UINT16 *dest = (UINT16 *)bitmap->line[sy];
					UINT8 *pdest = (UINT8 *)priority_bitmap->line[sy];

					for( x=0; x<width; x++ )
					{
						int sx = (flipx)?(xpos+width-1-x):(xpos+x);
						if( sx>=0 && sx<320 )
						{
							UINT16 pen = pen_data[x/4];
							switch( x%4 )
							{
							case 0: pen = pen>>12; break;
							case 1: pen = (pen>>8)&0xf; break;
							case 2: pen = (pen>>4)&0xf; break;
							case 3: pen = pen&0xf; break;
							}
							if( pen && pdest[sx]<pval) { dest[sx] = pal_data[pen]; pdest[sx]|=0x10; }
						}
					}
				}
				pen_data += width/4;
			}
		}
	}
}

void twin16_spriteram_process( void )
{
	UINT16 dx = scrollx[0];
	UINT16 dy = scrolly[0];

	const UINT16 *source = &spriteram16[0x0000];
	const UINT16 *finish = &spriteram16[0x1800];

	memset( &spriteram16[0x1800], 0, 0x800 );
	while( source<finish ){
		UINT16 priority = source[0];
		if( priority & 0x8000 ){
			UINT16 *dest = &spriteram16[0x1800 + 4*(priority&0xff)];

			INT32 xpos = (0x10000*source[4])|source[5];
			INT32 ypos = (0x10000*source[6])|source[7];

			UINT16 attributes = source[2]&0x03ff; /* scale,size,color */
			if( priority & 0x0200 ) attributes |= 0x4000;
			/* Todo:  priority & 0x0100 is also used */
			attributes |= 0x8000;

			dest[0] = source[3]; /* gfx data */
			dest[1] = ((xpos>>8) - dx)&0xffff;
			dest[2] = ((ypos>>8) - dy)&0xffff;
			dest[3] = attributes;
		}
		source += 0x50/2;
	}
	need_process_spriteram = 0;
}

/*
 * Sprite Format
 * ----------------------------------
 *
 * Word | Bit(s)           | Use
 * -----+-fedcba9876543210-+----------------
 *   0  | --xxxxxxxxxxxxxx | code
 * -----+------------------+
 *   1  | -------xxxxxxxxx | ypos
 * -----+------------------+
 *   2  | -------xxxxxxxxx | xpos
 * -----+------------------+
 *   3  | x--------------- | enble
 *   3  | -x-------------- | priority?
 *   3  | ------x--------- | yflip?
 *   3  | -------x-------- | xflip
 *   3  | --------xx------ | height
 *   3  | ----------xx---- | width
 *   3  | ------------xxxx | color

shadow bit?

 */

static void draw_sprites( struct mame_bitmap *bitmap)
{
	int count = 0;

	const UINT16 *source = 0x1800+buffered_spriteram16 + 0x800 -4;
	const UINT16 *finish = 0x1800+buffered_spriteram16;

	while( source>=finish ){
		UINT16 attributes = source[3];
		UINT16 code = source[0];

		if( code!=0xffff && (attributes&0x8000)){
			int xpos = source[1];
			int ypos = source[2];

			const pen_t *pal_data = Machine->pens+((attributes&0xf)+0x10)*16;
			int height	= 16<<((attributes>>6)&0x3);
			int width	= 16<<((attributes>>4)&0x3);
			const UINT16 *pen_data = 0;
			int flipy = attributes&0x0200;
			int flipx = attributes&0x0100;

			if( twin16_custom_vidhrdw == 1 ){
				pen_data = twin16_gfx_rom + 0x80000;
			}
			else {
				switch( (code>>12)&0x3 ){ /* bank select */
					case 0:
					pen_data = twin16_gfx_rom;
					break;

					case 1:
					pen_data = twin16_gfx_rom + 0x40000;
					break;

					case 2:
					pen_data = twin16_gfx_rom + 0x80000;
					if( code&0x4000 ) pen_data += 0x40000;
					break;

					case 3:
					pen_data = twin16_sprite_gfx_ram;
					break;
				}
				code &= 0xfff;
			}
			pen_data += code*0x40;

			if( video_register&TWIN16_SCREEN_FLIPY ){
				if (ypos>65000) ypos=ypos-65536; /* Bit hacky */
				ypos = 256-ypos-height;
				flipy = !flipy;
			}
			if( video_register&TWIN16_SCREEN_FLIPX ){
				if (xpos>65000) xpos=xpos-65536; /* Bit hacky */
				xpos = 320-xpos-width;
				flipx = !flipx;
			}

			/*if( sprite_which==count || !keyboard_pressed( KEYCODE_B ) )*/
			draw_sprite( bitmap, pen_data, pal_data, xpos, ypos, width, height, flipx, flipy, (attributes&0x4000) );
		}

		count++;
		source -= 4;
	}
}

static void draw_layer( struct mame_bitmap *bitmap, int opaque ){
	const UINT16 *gfx_base;
	const UINT16 *source = videoram16;
	int i, y1, y2, yd;
	int bank_table[4];
	int dx, dy, palette;
	int tile_flipx = 0; /* video_register&TWIN16_TILE_FLIPX;*/
	int tile_flipy = video_register&TWIN16_TILE_FLIPY;

	if( ((video_register&TWIN16_PLANE_ORDER)?1:0) != opaque ){
		source += 0x1000;
		dx = scrollx[2];
		dy = scrolly[2];
		palette = 1;
	}
	else {
		source += 0x0000;
		dx = scrollx[1];
		dy = scrolly[1];
		palette = 0;
	}

	if( twin16_custom_vidhrdw == 1 ){
		gfx_base = twin16_gfx_rom;
		bank_table[3] = (gfx_bank>>(4*3))&0xf;
		bank_table[2] = (gfx_bank>>(4*2))&0xf;
		bank_table[1] = (gfx_bank>>(4*1))&0xf;
		bank_table[0] = (gfx_bank>>(4*0))&0xf;
	}
	else {
		gfx_base = twin16_tile_gfx_ram;
		bank_table[0] = 0;
		bank_table[1] = 1;
		bank_table[2] = 2;
		bank_table[3] = 3;
	}

	if( video_register&TWIN16_SCREEN_FLIPX ){
		dx = 256-dx-64;
		tile_flipx = !tile_flipx;
	}

	if( video_register&TWIN16_SCREEN_FLIPY ){
		dy = 256-dy;
		tile_flipy = !tile_flipy;
	}

	if( tile_flipy ){
		y1 = 7; y2 = -1; yd = -1;
	}
	else {
		y1 = 0; y2 = 8; yd = 1;
	}

	for( i=0; i<64*64; i++ ){
		int sx = (i%64)*8;
		int sy = (i/64)*8;
		int xpos,ypos;

		if( video_register&TWIN16_SCREEN_FLIPX ) sx = 63*8 - sx;
		if( video_register&TWIN16_SCREEN_FLIPY ) sy = 63*8 - sy;

		xpos = (sx-dx)&0x1ff;
		ypos = (sy-dy)&0x1ff;
		if( xpos>=320 ) xpos -= 512;
		if( ypos>=256 ) ypos -= 512;

		if(  xpos>-8 && ypos>8 && xpos<320 && ypos<256-16 ){
			int code = source[i];
			/*
				xxx-------------	color
				---xx-----------	tile bank
				-----xxxxxxxxxxx	tile number
			*/
			const UINT16 *gfx_data = gfx_base + (code&0x7ff)*16 + bank_table[(code>>11)&0x3]*0x8000;
			int color = (code>>13);
			pen_t *pal_data = Machine->pens + 16*(0x20+color+8*palette);

			{
				int y;
				UINT16 data;
				int pen;

				if( tile_flipx )
				{
					if( opaque )
					{
						{
							for( y=y1; y!=y2; y+=yd )
							{
								UINT16 *dest = ((UINT16 *)bitmap->line[ypos+y])+xpos;
								UINT8 *pdest = ((UINT8 *)priority_bitmap->line[ypos+y])+xpos;

								data = *gfx_data++;
								dest[7] = pal_data[(data>>4*3)&0xf];
								dest[6] = pal_data[(data>>4*2)&0xf];
								dest[5] = pal_data[(data>>4*1)&0xf];
								dest[4] = pal_data[(data>>4*0)&0xf];
								data = *gfx_data++;
								dest[3] = pal_data[(data>>4*3)&0xf];
								dest[2] = pal_data[(data>>4*2)&0xf];
								dest[1] = pal_data[(data>>4*1)&0xf];
								dest[0] = pal_data[(data>>4*0)&0xf];

								pdest[7]|=1;
								pdest[6]|=1;
								pdest[5]|=1;
								pdest[4]|=1;
								pdest[3]|=1;
								pdest[2]|=1;
								pdest[1]|=1;
								pdest[0]|=1;
							}
						}
					}
					else
					{
						{
							for( y=y1; y!=y2; y+=yd )
							{
								UINT16 *dest = ((UINT16 *)bitmap->line[ypos+y])+xpos;
								UINT8 *pdest = ((UINT8 *)priority_bitmap->line[ypos+y])+xpos;

								data = *gfx_data++;
								if( data )
								{
									pen = (data>>4*3)&0xf; if( pen ) { dest[7] = pal_data[pen]; pdest[7]|=4; }
									pen = (data>>4*2)&0xf; if( pen ) { dest[6] = pal_data[pen]; pdest[6]|=4; }
									pen = (data>>4*1)&0xf; if( pen ) { dest[5] = pal_data[pen]; pdest[5]|=4; }
									pen = (data>>4*0)&0xf; if( pen ) { dest[4] = pal_data[pen]; pdest[4]|=4; }
								}
								data = *gfx_data++;
								if( data )
								{
									pen = (data>>4*3)&0xf; if( pen ) { dest[3] = pal_data[pen]; pdest[3]|=4; }
									pen = (data>>4*2)&0xf; if( pen ) { dest[2] = pal_data[pen]; pdest[2]|=4; }
									pen = (data>>4*1)&0xf; if( pen ) { dest[1] = pal_data[pen]; pdest[1]|=4; }
									pen = (data>>4*0)&0xf; if( pen ) { dest[0] = pal_data[pen]; pdest[0]|=4; }
								}
							}
						}
					}
				}
				else
				{
					if( opaque )
					{
						{
							for( y=y1; y!=y2; y+=yd )
							{
								UINT16 *dest = ((UINT16 *)bitmap->line[ypos+y])+xpos;
								UINT8 *pdest = ((UINT8 *)priority_bitmap->line[ypos+y])+xpos;

								data = *gfx_data++;
								*dest++ = pal_data[(data>>4*3)&0xf];
								*dest++ = pal_data[(data>>4*2)&0xf];
								*dest++ = pal_data[(data>>4*1)&0xf];
								*dest++ = pal_data[(data>>4*0)&0xf];
								data = *gfx_data++;
								*dest++ = pal_data[(data>>4*3)&0xf];
								*dest++ = pal_data[(data>>4*2)&0xf];
								*dest++ = pal_data[(data>>4*1)&0xf];
								*dest++ = pal_data[(data>>4*0)&0xf];

								pdest[7]|=1;
								pdest[6]|=1;
								pdest[5]|=1;
								pdest[4]|=1;
								pdest[3]|=1;
								pdest[2]|=1;
								pdest[1]|=1;
								pdest[0]|=1;

							}
						}
					}
					else
					{
						{
							for( y=y1; y!=y2; y+=yd )
							{
								UINT16 *dest = ((UINT16 *)bitmap->line[ypos+y])+xpos;
								UINT8 *pdest = ((UINT8 *)priority_bitmap->line[ypos+y])+xpos;

								data = *gfx_data++;
								if( data )
								{
									pen = (data>>4*3)&0xf; if( pen ) { dest[0] = pal_data[pen]; pdest[0]|=4; }
									pen = (data>>4*2)&0xf; if( pen ) { dest[1] = pal_data[pen]; pdest[1]|=4; }
									pen = (data>>4*1)&0xf; if( pen ) { dest[2] = pal_data[pen]; pdest[2]|=4; }
									pen = (data>>4*0)&0xf; if( pen ) { dest[3] = pal_data[pen]; pdest[3]|=4; }
								}
								data = *gfx_data++;
								if( data )
								{
									pen = (data>>4*3)&0xf; if( pen ) { dest[4] = pal_data[pen]; pdest[4]|=4; }
									pen = (data>>4*2)&0xf; if( pen ) { dest[5] = pal_data[pen]; pdest[5]|=4; }
									pen = (data>>4*1)&0xf; if( pen ) { dest[6] = pal_data[pen]; pdest[6]|=4; }
									pen = (data>>4*0)&0xf; if( pen ) { dest[7] = pal_data[pen]; pdest[7]|=4; }
								}
							}
						}
					}
				}
			}
		}
	}
}

static void get_fg_tile_info(int tile_index)
{
	const UINT16 *source = twin16_videoram2;
	int attr = source[tile_index];
	int code = attr & 0x1ff;
	int color = (attr >> 9) & 0x0f;

	SET_TILE_INFO(0, code, color, 0)
}

VIDEO_START( twin16 )
{
	fg_tilemap = tilemap_create(get_fg_tile_info, tilemap_scan_rows_flip_y,
		TILEMAP_TRANSPARENT, 8, 8, 64, 32);

	if ( !fg_tilemap )
		return 1;

	tilemap_set_transparent_pen(fg_tilemap, 0);

	return 0;
}

VIDEO_START( fround )
{
	fg_tilemap = tilemap_create(get_fg_tile_info, tilemap_scan_rows,
		TILEMAP_TRANSPARENT, 8, 8, 64, 32);

	if ( !fg_tilemap )
		return 1;

	tilemap_set_transparent_pen(fg_tilemap, 0);

	return 0;
}

VIDEO_EOF( twin16 )
{
	if( twin16_spriteram_process_enable() && need_process_spriteram )
		twin16_spriteram_process();

	need_process_spriteram = 1;

	buffer_spriteram16_w(0,0,0);
}

VIDEO_UPDATE( twin16 )
{
	fillbitmap(priority_bitmap,0,cliprect);
	draw_layer( bitmap,1 );
	draw_layer( bitmap,0 );
	draw_sprites( bitmap );
	tilemap_draw(bitmap, cliprect, fg_tilemap, 0, 0);
}
