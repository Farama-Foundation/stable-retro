/*** Video *******************************************************************/
/* see drivers/pgm.c for notes on where improvements can be made */

#include "driver.h"

extern data16_t *pgm_mainram, *pgm_bg_videoram, *pgm_tx_videoram, *pgm_videoregs, *pgm_rowscrollram;
static struct tilemap *pgm_tx_tilemap, *pgm_bg_tilemap;
static UINT16 *sprite_bitmap;
static data16_t *pgm_spritebufferram; /* buffered spriteram*/

extern data8_t *pgm_sprite_a_region;   /* = memory_region       ( REGION_GFX4 ); */
extern size_t	pgm_sprite_a_region_allocate;

/* Sprites - These are a pain! */


static void pgm_drawsprite(int wide, int high, int xpos, int ypos, int palt, int boffset, int flip)
{
	int xcnt, ycnt, offset, xdrawpos, ydrawpos;

	data8_t *bdata    = memory_region       ( REGION_GFX4 );
	size_t  bdatasize = memory_region_length( REGION_GFX4 )-1;
	data8_t *adata    = pgm_sprite_a_region;
	size_t  adatasize = pgm_sprite_a_region_allocate-1;

	data16_t *dest;
	data16_t msk;

	data32_t aoffset;
	int draw;

	aoffset = (bdata[(boffset+3) & bdatasize] << 24) | (bdata[(boffset+2) & bdatasize] << 16) | (bdata[(boffset+1) & bdatasize] << 8) | (bdata[(boffset+0) & bdatasize] << 0);
	aoffset = aoffset >> 2; aoffset *= 3;

/* 	if(aoffset)	logerror ("aoffset %08x boffset %08x\n",aoffset,boffset);*/

	boffset += 4; /* because the first dword is the a data offset */
	if (!(flip & 0x02)) { /* NO Y FLIP */
		for (ycnt = 0 ; ycnt < high ; ycnt++) {
			ydrawpos = ypos + ycnt;

			if (!(flip & 0x01)) { /* NO FLIPPING */
				for (xcnt = 0 ; xcnt < wide ; xcnt++) {
					xdrawpos = xpos + (xcnt*16)+32;
					draw = 0; if ( (xdrawpos > 0) && (xdrawpos < 448+32) && (ydrawpos >= 0) && (ydrawpos < 224) ) draw = 1;
					/* Draw Code */
					{
						offset = xdrawpos + ((448+32+32) * ydrawpos);
						dest = &sprite_bitmap[offset];
						msk = (( bdata[(boffset+0) & bdatasize] << 8) |( bdata[(boffset+1) & bdatasize] << 0) );
						if (!(msk & 0x0100)) { if (draw)  dest[0]  = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x0200)) { if (draw)  dest[1]  = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x0400)) { if (draw)  dest[2]  = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x0800)) { if (draw)  dest[3]  = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x1000)) { if (draw)  dest[4]  = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x2000)) { if (draw)  dest[5]  = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x4000)) { if (draw)  dest[6]  = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x8000)) { if (draw)  dest[7]  = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x0001)) { if (draw)  dest[8]  = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x0002)) { if (draw)  dest[9]  = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x0004)) { if (draw)  dest[10] = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x0008)) { if (draw)  dest[11] = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x0010)) { if (draw)  dest[12] = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x0020)) { if (draw)  dest[13] = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x0040)) { if (draw)  dest[14] = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x0080)) { if (draw)  dest[15] = adata[aoffset & adatasize]+ palt*32;aoffset++; }
					}
					boffset+=2;
				}
			} else { /* X FLIPPING */
				for (xcnt = wide-1 ; xcnt >= 0 ; xcnt--) {
					xdrawpos = xpos + (xcnt*16)+32;
					draw = 0; if ( (xdrawpos > 0) && (xdrawpos < 448+32) && (ydrawpos >= 0) && (ydrawpos < 224) ) draw = 1;
					/* Draw Code */
					{
						offset = xdrawpos + ((448+32+32) * ydrawpos);
						dest = &sprite_bitmap[offset];
						msk = (( bdata[(boffset+0) & bdatasize] << 8) |( bdata[(boffset+1) & bdatasize] << 0) );
						if (!(msk & 0x0100)) { if (draw)  dest[15]  = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x0200)) { if (draw)  dest[14]  = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x0400)) { if (draw)  dest[13]  = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x0800)) { if (draw)  dest[12]  = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x1000)) { if (draw)  dest[11]  = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x2000)) { if (draw)  dest[10]  = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x4000)) { if (draw)  dest[9]  = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x8000)) { if (draw)  dest[8]  = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x0001)) { if (draw)  dest[7]  = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x0002)) { if (draw)  dest[6]  = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x0004)) { if (draw)  dest[5] = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x0008)) { if (draw)  dest[4] = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x0010)) { if (draw)  dest[3] = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x0020)) { if (draw)  dest[2] = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x0040)) { if (draw)  dest[1] = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x0080)) { if (draw)  dest[0] = adata[aoffset & adatasize]+ palt*32;aoffset++; }
					}
					boffset+=2;
				}
			}
		}
	} else {	/* Y FLIPPING */
		for (ycnt = high-1 ; ycnt >= 0 ; ycnt--) {
			ydrawpos = ypos + ycnt;

			if (!(flip & 0x01)) { /* Y FLIP */
				for (xcnt = 0 ; xcnt < wide ; xcnt++) {
					xdrawpos = xpos + (xcnt*16)+32;
					draw = 0; if ( (xdrawpos > 0) && (xdrawpos < 448+32) && (ydrawpos >= 0) && (ydrawpos < 224) ) draw = 1;
					/* Draw Code */
					{
						offset = xdrawpos + ((448+32+32) * ydrawpos);
						dest = &sprite_bitmap[offset];
						msk = (( bdata[(boffset+0) & bdatasize] << 8) |( bdata[(boffset+1) & bdatasize] << 0) );
						if (!(msk & 0x0100)) { if (draw)  dest[0]  = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x0200)) { if (draw)  dest[1]  = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x0400)) { if (draw)  dest[2]  = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x0800)) { if (draw)  dest[3]  = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x1000)) { if (draw)  dest[4]  = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x2000)) { if (draw)  dest[5]  = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x4000)) { if (draw)  dest[6]  = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x8000)) { if (draw)  dest[7]  = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x0001)) { if (draw)  dest[8]  = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x0002)) { if (draw)  dest[9]  = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x0004)) { if (draw)  dest[10] = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x0008)) { if (draw)  dest[11] = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x0010)) { if (draw)  dest[12] = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x0020)) { if (draw)  dest[13] = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x0040)) { if (draw)  dest[14] = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x0080)) { if (draw)  dest[15] = adata[aoffset & adatasize]+ palt*32;aoffset++; }
					}
					boffset+=2;
				}
			} else { /* XY FLIP */
				for (xcnt = wide-1 ; xcnt >= 0 ; xcnt--) {
					xdrawpos = xpos + (xcnt*16)+32;
					draw = 0; if ( (xdrawpos > 0) && (xdrawpos < 448+32) && (ydrawpos >= 0) && (ydrawpos < 224) ) draw = 1;
					/* Draw Code */
					{
						offset = xdrawpos + ((448+32+32) * ydrawpos);
						dest = &sprite_bitmap[offset];
						msk = (( bdata[(boffset+0) & bdatasize] << 8) |( bdata[(boffset+1) & bdatasize] << 0) );
						if (!(msk & 0x0100)) { if (draw)  dest[15]  = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x0200)) { if (draw)  dest[14]  = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x0400)) { if (draw)  dest[13]  = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x0800)) { if (draw)  dest[12]  = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x1000)) { if (draw)  dest[11]  = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x2000)) { if (draw)  dest[10]  = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x4000)) { if (draw)  dest[9]  = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x8000)) { if (draw)  dest[8]  = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x0001)) { if (draw)  dest[7]  = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x0002)) { if (draw)  dest[6]  = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x0004)) { if (draw)  dest[5] = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x0008)) { if (draw)  dest[4] = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x0010)) { if (draw)  dest[3] = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x0020)) { if (draw)  dest[2] = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x0040)) { if (draw)  dest[1] = adata[aoffset & adatasize]+ palt*32;aoffset++; }
						if (!(msk & 0x0080)) { if (draw)  dest[0] = adata[aoffset & adatasize]+ palt*32;aoffset++; }
					}
					boffset+=2;
				}
			}
		}
	}
}

static UINT16 *pgm_sprite_source;

static void pgm_drawsprites(int priority)
{
	/* ZZZZ Zxxx xxxx xxxx
	   zzzz z-yy yyyy yyyy
	   -ffp pppp Pvvv vvvv
	   vvvv vvvv vvvv vvvv
	   wwww wwwh hhhh hhhh
	*/

	/* const UINT16 *finish = pgm_spritebufferram+0xa00; */
	const UINT16 *finish = pgm_spritebufferram+(0xa00/2);
	int y;

	/* clear the sprite bitmap */
	for (y = 0; y < 224*(448+32+32); y++)
		sprite_bitmap[y] = 0x400;

	while( pgm_sprite_source<finish )
	{
		int xpos = pgm_sprite_source[0] & 0x07ff;
		int ypos = pgm_sprite_source[1] & 0x03ff;
/*		int xzom = (pgm_sprite_source[0] & 0xf800) >> 11;*/
/*		int yzom = (pgm_sprite_source[1] & 0xf800) >> 11;*/
		int palt = (pgm_sprite_source[2] & 0x1f00) >> 8;
		int flip = (pgm_sprite_source[2] & 0x6000) >> 13;
		int boff = ((pgm_sprite_source[2] & 0x007f) << 16) | (pgm_sprite_source[3] & 0xffff);
		int wide = (pgm_sprite_source[4] & 0x7e00) >> 9;/* Killing Blade*/
		int high = pgm_sprite_source[4] & 0x01ff;
		int pri = (pgm_sprite_source[2] & 0x0080) >>  7;

		boff *= 2;
		if (xpos > 0x3ff) xpos -=0x800;
		if (ypos > 0x1ff) ypos -=0x400;

		if (high == 0) break; /* is this right? */

		if ((priority == 1) && (pri == 0)) break;

		pgm_drawsprite(wide, high, xpos, ypos, palt, boff, flip);

		pgm_sprite_source += 5;
	}
}

/* TX Layer */

WRITE16_HANDLER( pgm_tx_videoram_w )
{
	if (pgm_tx_videoram[offset] != data)
	{
		pgm_tx_videoram[offset] = data;
		tilemap_mark_tile_dirty(pgm_tx_tilemap,offset/2);
	}
}

static void get_pgm_tx_tilemap_tile_info(int tile_index)
{

/* 0x904000 - 0x90ffff is the Text Overlay Ram (pgm_tx_videoram)
	each tile uses 4 bytes, the tilemap is 64x128?

   the layer uses 4bpp 8x8 tiles from the 'T' roms
   colours from 0xA01000 - 0xA017FF

   scroll registers are at 0xB05000 (Y) and 0xB06000 (X)

	---- ---- ffpp ppp- nnnn nnnn nnnn nnnn

	n = tile number
	p = palette
	f = flip
*/
	int tileno,colour,flipyx; /*,game;*/

	tileno = pgm_tx_videoram[tile_index *2] & 0xffff;
	colour = (pgm_tx_videoram[tile_index*2+1] & 0x3e) >> 1;
	flipyx = (pgm_tx_videoram[tile_index*2+1] & 0xc0) >> 6;;

	if (tileno > 0xbfff) { tileno -= 0xc000 ; tileno += 0x20000; } /* not sure about this */

	SET_TILE_INFO(0,tileno,colour,TILE_FLIPYX(flipyx))
}

/* BG Layer */

WRITE16_HANDLER( pgm_bg_videoram_w )
{
	if (pgm_bg_videoram[offset] != data)
	{
		pgm_bg_videoram[offset] = data;
		tilemap_mark_tile_dirty(pgm_bg_tilemap,offset/2);
	}
}

static void get_pgm_bg_tilemap_tile_info(int tile_index)
{
	/* pretty much the same as tx layer */

	int tileno,colour,flipyx;

	tileno = pgm_bg_videoram[tile_index *2] & 0xffff;
	if (tileno > 0x7ff) tileno+=0x1000; /* Tiles 0x800+ come from the GAME Roms */
	colour = (pgm_bg_videoram[tile_index*2+1] & 0x3e) >> 1;
	flipyx = (pgm_bg_videoram[tile_index*2+1] & 0xc0) >> 6;;

	SET_TILE_INFO(1,tileno,colour,TILE_FLIPYX(flipyx))
}



/*** Video - Start / Update ****************************************************/

VIDEO_START( pgm )
{
	pgm_tx_tilemap= tilemap_create(get_pgm_tx_tilemap_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT, 8, 8,64,32);
	tilemap_set_transparent_pen(pgm_tx_tilemap,15);

/*	pgm_bg_tilemap = tilemap_create(get_pgm_bg_tilemap_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT, 32, 32,64,64); */
	pgm_bg_tilemap = tilemap_create(get_pgm_bg_tilemap_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT, 32, 32,64,16);
	tilemap_set_transparent_pen(pgm_bg_tilemap,31);
/*	tilemap_set_scroll_rows(pgm_bg_tilemap,64*32); */
	tilemap_set_scroll_rows(pgm_bg_tilemap,16*32);

	pgm_spritebufferram = auto_malloc (0xa00);

	sprite_bitmap		= auto_malloc((448+32+32) * 224 * sizeof(UINT16));
	if (!sprite_bitmap) return 1;




	return 0;
}

VIDEO_UPDATE( pgm )
{
	int y;

	fillbitmap(bitmap,get_black_pen(),&Machine->visible_area);

	pgm_sprite_source = pgm_spritebufferram;
	pgm_drawsprites(1);

	/* copy the sprite bitmap to the screen */
	for (y = 0; y < 224; y++)
		draw_scanline16(bitmap, 0, y, 448, &sprite_bitmap[y * (448+32+32)+32], Machine->pens, 0x400);

	tilemap_set_scrolly(pgm_bg_tilemap,0, pgm_videoregs[0x2000/2]);

	for (y = 0; y < 224; y++)
		/*tilemap_set_scrollx(pgm_bg_tilemap,(y+pgm_videoregs[0x2000/2])&0x7ff, pgm_videoregs[0x3000/2]+pgm_rowscrollram[y]); */
        tilemap_set_scrollx(pgm_bg_tilemap,(y+pgm_videoregs[0x2000/2])&0x1ff, pgm_videoregs[0x3000/2]+pgm_rowscrollram[y]);
	
	tilemap_draw(bitmap,cliprect,pgm_bg_tilemap,0,0);

	pgm_drawsprites(0);
	/* copy the sprite bitmap to the screen */
	for (y = 0; y < 224; y++)
		draw_scanline16(bitmap, 0, y, 448, &sprite_bitmap[y * (448+32+32)+32], Machine->pens, 0x400);

	tilemap_set_scrolly(pgm_tx_tilemap,0, pgm_videoregs[0x5000/2]);
	tilemap_set_scrollx(pgm_tx_tilemap,0, pgm_videoregs[0x6000/2]); /* Check*/
	tilemap_draw(bitmap,cliprect,pgm_tx_tilemap,0,0);
}

VIDEO_EOF( pgm )
{
	/* first 0xa00 of main ram = sprites, seems to be buffered, DMA? */
	memcpy(pgm_spritebufferram,pgm_mainram,0xa00);
}
