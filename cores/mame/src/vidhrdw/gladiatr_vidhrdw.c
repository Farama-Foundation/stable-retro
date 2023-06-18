

/***************************************************************************
	Video Hardware description for Taito Gladiator

***************************************************************************/
#include "driver.h"
#include "vidhrdw/generic.h"

static int video_attributes;
static int base_scroll;
static int background_scroll;
static int sprite_bank;

unsigned char *gladiatr_scroll;
unsigned char *gladiator_text;


static void update_color(int offset)
{
	int r,g,b;


	r = (paletteram[offset] >> 0) & 0x0f;
	g = (paletteram[offset] >> 4) & 0x0f;
	b = (paletteram_2[offset] >> 0) & 0x0f;

	r = (r << 1) + ((paletteram_2[offset] >> 4) & 0x01);
	g = (g << 1) + ((paletteram_2[offset] >> 5) & 0x01);
	b = (b << 1) + ((paletteram_2[offset] >> 6) & 0x01);

	r = (r << 3) | (r >> 2);
	g = (g << 3) | (g >> 2);
	b = (b << 3) | (b >> 2);

	palette_set_color(offset,r,g,b);

	/* the text layer might use the other 512 entries in the palette RAM */
	/* (which are all set to 0x07ff = white). I don't know, so I just set */
	/* it to white. */
	palette_set_color(512,0x00,0x00,0x00);
	palette_set_color(513,0xff,0xff,0xff);
}

WRITE_HANDLER( gladiatr_paletteram_rg_w )
{
	paletteram[offset] = data;
	update_color(offset);
}

WRITE_HANDLER( gladiatr_paletteram_b_w )
{
	paletteram_2[offset] = data;
	update_color(offset);
}


WRITE_HANDLER( gladiatr_spritebank_w );
WRITE_HANDLER( gladiatr_spritebank_w ){
	sprite_bank = (data)?4:2;
}

READ_HANDLER( gladiatr_video_registers_r );
READ_HANDLER( gladiatr_video_registers_r ){
	switch( offset ){
		case 0x080: return video_attributes;
		case 0x100: return base_scroll;
		case 0x300: return background_scroll;
	}
	return 0;
}

WRITE_HANDLER( gladiatr_video_registers_w );
WRITE_HANDLER( gladiatr_video_registers_w ){
	switch( offset ){
		case 0x000: break;
		case 0x080: video_attributes = data; break;
		case 0x100: base_scroll = data; break;
		case 0x200: break;
		case 0x300: background_scroll = data; break;
	}
}


VIDEO_START( gladiatr );
VIDEO_START( gladiatr ){
	sprite_bank = 2;

	dirtybuffer = auto_malloc(64*32);
	if( !dirtybuffer )
		return 1;
	tmpbitmap = auto_bitmap_alloc(512,256);
	if( !tmpbitmap )
		return 1;
	memset(dirtybuffer,1,64*32);
	return 0;
}


static void render_background( struct mame_bitmap *bitmap );
static void render_background( struct mame_bitmap *bitmap ){
	int i;
	static int tile_bank_select = 0;

	int scrollx = - background_scroll;

	if( base_scroll < 0xd0 ){
		scrollx += 256-(0xd0)-64-32;

		if( video_attributes&0x04 ){
			scrollx += 256;
		}
	}
	else {
		if( video_attributes&0x04 ){
			scrollx += base_scroll;
		}
		else {
			scrollx += 256-(0xd0)-64-32;
		}
	}

	{
		int old_bank_select = tile_bank_select;
		if( video_attributes & 0x10 ){
			tile_bank_select = 256*8;
		}
		else {
			tile_bank_select = 0;
		}
		if( old_bank_select != tile_bank_select )
			memset(dirtybuffer,1,64*32);
	}

	for( i=0; i<64*32; i++ ){
		if( dirtybuffer[i] ){
			int sx = (i%64)*8;
			int sy = (i/64)*8;

			int attributes = colorram[i];
			int color = 0x1F - (attributes>>3);
			int tile_number = videoram[i] + 256*(attributes&0x7) + tile_bank_select;

			drawgfx(tmpbitmap,Machine->gfx[1+(tile_number/512)],
				tile_number%512,
				color,
				0,0, /* no flip */
				sx,sy,
				0, /* no need to clip */
				TRANSPARENCY_NONE,0);

			dirtybuffer[i] = 0;
		}
	}

	copyscrollbitmap(bitmap,tmpbitmap,
		1,&scrollx,
		0,0,
		&Machine->visible_area,TRANSPARENCY_NONE,0);
}

static void render_text( struct mame_bitmap *bitmap );
static void render_text( struct mame_bitmap *bitmap ){
	const struct rectangle *clip = &Machine->visible_area;
	const struct GfxElement *gfx = Machine->gfx[0];

	int tile_bank_offset = (video_attributes&3)*256;

	unsigned char *source = gladiator_text;

	int sx,sy;

	int dx;

	if( base_scroll < 0xd0 ){ /* panning text */
		dx = 256-(0xd0)-64-32- background_scroll;
		if( video_attributes&0x04 ){ dx += 256; }
	}
	else { /* fixed text */
		dx = 0;
		if( (video_attributes&0x08)==0 ) source += 32; /* page 2 */
	}

	for( sy=0; sy<256; sy+=8 ){
		for( sx=0; sx<256; sx+=8 ){
			drawgfx( bitmap,gfx,
				tile_bank_offset + *source++,
				0, /* color */
				0,0, /* no flip */
				sx+dx,sy,
				clip,TRANSPARENCY_PEN,0);
		}
		source += 32; /* skip to next row */
	}
}

static void draw_sprite( struct mame_bitmap *bitmap, int tile_number, int color, int sx, int sy, int xflip, int yflip, int big );
static void draw_sprite( struct mame_bitmap *bitmap, int tile_number, int color, int sx, int sy, int xflip, int yflip, int big ){
	const struct rectangle *clip = &Machine->visible_area;

	static int tile_offset[4][4] = {
		{0x0,0x1,0x4,0x5},
		{0x2,0x3,0x6,0x7},
		{0x8,0x9,0xC,0xD},
		{0xA,0xB,0xE,0xF}
	};

	int x,y;

	int size = big?4:2;

	for( y=0; y<size; y++ ){
		for( x=0; x<size; x++ ){
			int ex = xflip?(size-1-x):x;
			int ey = yflip?(size-1-y):y;

			int t = tile_offset[ey][ex] + tile_number;

			drawgfx(bitmap,Machine->gfx[1+8+((t/512)%12)],
				t%512,

				color,
				xflip,yflip,
				sx+x*8,sy+y*8,
				clip,TRANSPARENCY_PEN,0);
		}
	}
}

static void render_sprites(struct mame_bitmap *bitmap);
static void render_sprites(struct mame_bitmap *bitmap){
	unsigned char *source = spriteram;
	unsigned char *finish = source+0x400;

	do{
		int attributes = source[0x800];
		int big = attributes&0x10;
		int bank = (attributes&0x1) + ((attributes&2)?sprite_bank:0);
		int tile_number = (source[0]+256*bank)*4;
		int sx = source[0x400+1] + 256*(source[0x801]&1);
		int sy = 240-source[0x400] - (big?16:0);
		int xflip = attributes&0x04;
		int yflip = attributes&0x08;
		int color = 0x20 + (source[1]&0x1F);

		if( (video_attributes & 0x04) && (base_scroll < 0xd0) )
			sx += 256-64+8-0xD0-64+8;
		else
			sx += base_scroll-0xD0-64+8;

		draw_sprite( bitmap, tile_number, color, sx,sy, xflip,yflip, big );

		source+=2;
	}while( source<finish );
}



VIDEO_UPDATE( gladiatr )
{
	if (video_attributes & 0x20)	/* screen refresh enable? */
	{
		render_background( bitmap );
		render_sprites( bitmap );
		render_text( bitmap );
	}
}
