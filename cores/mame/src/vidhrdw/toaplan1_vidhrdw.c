/***************************************************************************

  Functions to emulate the video hardware of some Toaplan games,
  which use the BCU-2 tile controller, and the FCU-2 Sprite controller -
  and SCU Sprite controller (Only Rally Bike uses the SCU controller).


  There are 4 scrolling layers of graphics, stored in planes of 64x64 tiles.
  Each tile in each plane is assigned a priority between 1 and 15, higher
  numbers have greater priority.

 BCU controller. Each tile takes up 32 bits - the format is:

  0         1         2         3
  ---- ---- ---- ---- -ttt tttt tttt tttt = Tile number (0 - $7fff)
  ---- ---- ---- ---- h--- ---- ---- ---- = Hidden
  ---- ---- --cc cccc ---- ---- ---- ---- = Color (0 - $3f)
  pppp ---- ---- ---- ---- ---- ---- ---- = Priority (0-$f)
  ---- ???? ??-- ---- ---- ---- ---- ---- = Unknown / Unused

  Scroll Reg

  0         1         2         3
  xxxx xxxx x--- ---- ---- ---- ---- ---- = X position
  ---- ---- ---- ---- yyyy yyyy y--- ---- = Y position
  ---- ---- -??? ???? ---- ---- -??? ???? = Unknown / Unused



 FCU controller. Sprite RAM format  (except Rally Bike)

  0         1         2         3
  -sss ssss ssss ssss ---- ---- ---- ---- = Sprite number (0 - $7fff)
  h--- ---- ---- ---- ---- ---- ---- ---- = Hidden
  ---- ---- ---- ---- ---- ---- --cc cccc = Color (0 - $3f)
  ---- ---- ---- ---- ---- dddd dd-- ---- = Dimension (pointer to Size RAM)
  ---- ---- ---- ---- pppp ---- ---- ---- = Priority (0-$f)

  4         5         6         7
  ---- ---- ---- ---- xxxx xxxx x--- ---- = X position
  yyyy yyyy y--- ---- ---- ---- ---- ---- = Y position
  ---- ---- -??? ???? ---- ---- -??? ???? = Unknown



 SCU controller. Sprite RAM format  (Rally Bike)

  0         1         2         3
  ---- -sss ssss ssss ---- ---- ---- ---- = Sprite number (0 - $7FF)
  ---- ---- ---- ---- ---- ---- --cc cccc = Color (0 - $3F)
  ---- ---- ---- ---- ---- ---x ---- ---- = Flip X
  ---- ---- ---- ---- ---- --y- ---- ---- = Flip Y
  ---- ---- ---- ---- ---- pp-- ---- ---- = Priority (0h,4h,8h,Ch (shifted < 2 places))
  ???? ?--- ---- ---- ???? ---- ??-- ---- = Unknown / Unused

  4         5         6         7
  xxxx xxxx x--- ---- ---- ---- ---- ---- = X position
  ---- ---- ---- ---- yyyy yyyy y--- ---- = Y position
  ---- ---- -??? ???? ---- ---- -??? ???? = Unknown



  The tiles use a palette of 1024 colors, the sprites use a different palette
  of 1024 colors.


           BCU Controller writes                Tile Offsets
 Game      reg0  reg1  reg2  reg3         X     Y     flip-X  flip-Y
RallyBik   41e0  2e1e  148c  0f09        01e6  00fc     <- same --
Truxton    41e0  2717  0e86  0c06        01b7  00f2     0188  01fd
HellFire   41e0  2717  0e86  0c06        01b7  0102     0188  000d
ZeroWing   41e0  2717  0e86  0c06        01b7  0102     0188  000d
DemonWld   41e0  2e1e  148c  0f09        01a9  00fc     0196  0013
FireShrk   41e0  2717  0e86  0c06        01b7  00f2     0188  01fd
Out-Zone   41e0  2e1e  148c  0f09        01a9  00ec     0196  0003
Vimana     41e0  2717  0e86  0c06        01b7  00f2     0188  01fd


Sprites are of varying sizes between 8x8 and 128x128 with any variation
in between, in multiples of 8 either way.
Here we draw the first 8x8 part of the sprite, then by using the sprite
dimensions, we draw the rest of the 8x8 parts to produce the complete
sprite.


Abnormalities:
 How/when do priority 0 Tile layers really get displayed ?

 What are the video PROMs for ? Priority maybe ?

***************************************************************************/


#include "driver.h"
#include "state.h"
#include "toaplan1.h"
#include "tilemap.h"
#include "palette.h"
#include "vidhrdw/generic.h"
#include "cpu/m68000/m68000.h"

#define TOAPLAN1_TILEVRAM_SIZE       0x4000	/* each tile layer ram (word size) */
#define TOAPLAN1_SPRITERAM_SIZE      0x800	/* sprite ram (word size) */
#define TOAPLAN1_SPRITESIZERAM_SIZE  0x80	/* sprite size ram (word size) */

#define TOAPLAN1_RENDER_TYPE_ZEROWING	0
#define TOAPLAN1_RENDER_TYPE_DEMONWLD	1

static data16_t *toaplan1_tileram16;
static data16_t *toaplan1_spritesizeram16;
static data16_t *toaplan1_buffered_spritesizeram16;

size_t toaplan1_colorram1_size;
size_t toaplan1_colorram2_size;
data16_t *toaplan1_colorram1;
data16_t *toaplan1_colorram2;

static int bcu_flipscreen;		/* Tile   controller flip flag */
static int fcu_flipscreen;		/* Sprite controller flip flag */

static unsigned int tileram_offs;
static unsigned int spriteram_offs;

static unsigned int scrollregs[8];
static unsigned int num_tiles;

static int layer_scrollx[4];
static int layer_scrolly[4];
static int layer_offsetx[4];
static int layer_offsety[4];

static int scrollx_offs1;
static int scrollx_offs2;
static int scrollx_offs3;
static int scrollx_offs4;
static int scrolly_offs;

static int flip_y_offs;

static int tiles_offsetx;
static int tiles_offsety;

static int toaplan1_reset;		/* Hack! See toaplan1_bcu_control below */


typedef struct
	{
	UINT16 tile_num;
	UINT16 color;
	char priority;
	int xpos;
	int ypos;
	} tile_struct;

tile_struct *bg_list[4];

tile_struct *tile_list[32];
tile_struct *temp_list;
static int max_list_size[32];
static int tile_count[32];

		struct	mame_bitmap *tmpbitmap1;
static	struct	mame_bitmap *tmpbitmap2;
static	struct	mame_bitmap *tmpbitmap3;


#undef BGDBG

#ifdef BGDBG
int	toaplan_dbg_sprite_only = 0;
int	toaplan_dbg_priority = 0;
int	toaplan_dbg_layer[4] = {1,1,1,1};
#endif

static int toaplan1_tile_buffers_alloc(void)
{
	int i;

	if ( (toaplan1_tileram16 = (data16_t *)auto_malloc(TOAPLAN1_TILEVRAM_SIZE * 4)) == 0){
		return 1;
	}
	memset(toaplan1_tileram16,0,TOAPLAN1_TILEVRAM_SIZE * 4);

	log_cb(RETRO_LOG_DEBUG, LOGPRE "colorram_size: %08x\n", toaplan1_colorram1_size + toaplan1_colorram2_size);
	if ( (paletteram16 = (data16_t *)auto_malloc(toaplan1_colorram1_size + toaplan1_colorram2_size)) == 0){
		return 1;
	}
	memset(paletteram16,0,toaplan1_colorram1_size + toaplan1_colorram2_size);

	for (i=0; i<4; i++)
	{
		if ((bg_list[i]=(tile_struct *)auto_malloc( 33 * 44 * sizeof(tile_struct))) == 0){
			return 1;
		}
		memset(bg_list[i], 0, 33 * 44 * sizeof(tile_struct));
	}

	for (i=0; i<16; i++)
	{
		max_list_size[i] = 8192;
		if ((tile_list[i]=(tile_struct *)auto_malloc(max_list_size[i]*sizeof(tile_struct))) == 0){
			return 1;
		}
		memset(tile_list[i],0,max_list_size[i]*sizeof(tile_struct));
	}

	max_list_size[16] = 65536;
	if ((tile_list[16]=(tile_struct *)auto_malloc(max_list_size[16]*sizeof(tile_struct))) == 0){
		return 1;
	}
	memset(tile_list[16],0,max_list_size[16]*sizeof(tile_struct));

	return 0;
}



VIDEO_START( rallybik )
{

	if( toaplan1_tile_buffers_alloc() ){
		return 1;
	}

	num_tiles = (Machine->drv->screen_width/8+1)*(Machine->drv->screen_height/8);

	spriteram_offs = tileram_offs = 0;

	scrollx_offs1 = 0x0d + 6;
	scrollx_offs2 = 0x0d + 4;
	scrollx_offs3 = 0x0d + 2;
	scrollx_offs4 = 0x0d + 0;
	scrolly_offs  = 0x111;

	bcu_flipscreen = 0;
	toaplan1_reset = 0;

	return 0;

}

VIDEO_START( toaplan1 )
{
	tmpbitmap1 = auto_bitmap_alloc(Machine->drv->screen_width,Machine->drv->screen_height);
	tmpbitmap2 = auto_bitmap_alloc(Machine->drv->screen_width,Machine->drv->screen_height);
	tmpbitmap3 = auto_bitmap_alloc(Machine->drv->screen_width,Machine->drv->screen_height);

	if ( (spriteram16 = (data16_t *)auto_malloc(TOAPLAN1_SPRITERAM_SIZE)) == 0){
		return 1;
	}
	memset(spriteram16,0,TOAPLAN1_SPRITERAM_SIZE);

	if ( (buffered_spriteram16 = (data16_t *)auto_malloc(TOAPLAN1_SPRITERAM_SIZE)) == 0){
		return 1;
	}
	memset(buffered_spriteram16,0,TOAPLAN1_SPRITERAM_SIZE);

	if ( (toaplan1_spritesizeram16 = (data16_t *)auto_malloc(TOAPLAN1_SPRITESIZERAM_SIZE)) == 0){
		return 1;
	}
	memset(toaplan1_spritesizeram16,0,TOAPLAN1_SPRITESIZERAM_SIZE);

	if ( (toaplan1_buffered_spritesizeram16 = (data16_t *)auto_malloc(TOAPLAN1_SPRITESIZERAM_SIZE)) == 0){
		return 1;
	}
	memset(toaplan1_buffered_spritesizeram16,0,TOAPLAN1_SPRITESIZERAM_SIZE);

	if( toaplan1_tile_buffers_alloc() ){
		return 1;
	}

	num_tiles = (Machine->drv->screen_width/8+1)*(Machine->drv->screen_height/8);

	spriteram_offs = tileram_offs = 0;

	scrollx_offs1 = 0x1ef + 6;
	scrollx_offs2 = 0x1ef + 4;
	scrollx_offs3 = 0x1ef + 2;
	scrollx_offs4 = 0x1ef + 0;
	scrolly_offs  = 0x101;

	bcu_flipscreen = 0;
	fcu_flipscreen = 0;
	toaplan1_reset = 1;

	return 0;
}



void toaplan1_set_scrolls(void)
{

	layer_scrollx[0] = (((scrollregs[0]) >> 7) + (scrollx_offs1 - tiles_offsetx)) & 0x1ff;
	layer_scrollx[1] = (((scrollregs[2]) >> 7) + (scrollx_offs2 - tiles_offsetx)) & 0x1ff;
	layer_scrollx[2] = (((scrollregs[4]) >> 7) + (scrollx_offs3 - tiles_offsetx)) & 0x1ff;
	layer_scrollx[3] = (((scrollregs[6]) >> 7) + (scrollx_offs4 - tiles_offsetx)) & 0x1ff;

	layer_scrolly[0] = (((scrollregs[1]) >> 7) + scrolly_offs - tiles_offsety) & 0x1ff;
	layer_scrolly[1] = (((scrollregs[3]) >> 7) + scrolly_offs - tiles_offsety) & 0x1ff;
	layer_scrolly[2] = (((scrollregs[5]) >> 7) + scrolly_offs - tiles_offsety) & 0x1ff;
	layer_scrolly[3] = (((scrollregs[7]) >> 7) + scrolly_offs - tiles_offsety) & 0x1ff;

}

/***************************************************************************

  Video I/O port hardware.

***************************************************************************/

READ16_HANDLER( toaplan1_frame_done_r )
{
	return cpu_getvblank();
}

WRITE16_HANDLER( toaplan1_tile_offsets_w )
{
	if ( offset == 0 )
	{
		COMBINE_DATA(&tiles_offsetx);
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Tiles_offsetx now = %08x\n",tiles_offsetx);
	}
	else
	{
		COMBINE_DATA(&tiles_offsety);
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Tiles_offsety now = %08x\n",tiles_offsety);
	}
	toaplan1_reset = 1;
	toaplan1_set_scrolls();
}


WRITE16_HANDLER( rallybik_bcu_flipscreen_w )
{
	if (ACCESSING_LSB)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Setting BCU controller flipscreen port to %04x\n",data);
		bcu_flipscreen = data & 0x01;		/* 0x0001 = flip, 0x0000 = no flip */
		if (bcu_flipscreen)
		{
			scrollx_offs1 = 0x080 - 6;
			scrollx_offs2 = 0x080 - 4;
			scrollx_offs3 = 0x080 - 2;
			scrollx_offs4 = 0x080 - 0;
			scrolly_offs  = 0x1f8;
		}
		else
		{
			scrollx_offs1 = 0x0d + 6;
			scrollx_offs2 = 0x0d + 4;
			scrollx_offs3 = 0x0d + 2;
			scrollx_offs4 = 0x0d + 0;
			scrolly_offs  = 0x111;
		}
		toaplan1_set_scrolls();
	}
}

WRITE16_HANDLER( toaplan1_bcu_flipscreen_w )
{
	if (ACCESSING_LSB)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Setting BCU controller flipscreen port to %04x\n",data);
		bcu_flipscreen = data & 0x01;		/* 0x0001 = flip, 0x0000 = no flip */
		if (bcu_flipscreen)
		{
			scrollx_offs1 = 0x011 - 6;
			scrollx_offs2 = 0x011 - 4;
			scrollx_offs3 = 0x011 - 2;
			scrollx_offs4 = 0x011 - 0;
			scrolly_offs  = 0xff;
			if (1)
			{
				scrolly_offs  += 16;
				flip_y_offs = -16;
			}
		}
		else
		{
			scrollx_offs1 = 0x1ef + 6;
			scrollx_offs2 = 0x1ef + 4;
			scrollx_offs3 = 0x1ef + 2;
			scrollx_offs4 = 0x1ef + 0;
			scrolly_offs  = 0x101;
			flip_y_offs = 0;
		}
		toaplan1_set_scrolls();
	}
}

WRITE16_HANDLER( toaplan1_fcu_flipscreen_w )
{
	if (ACCESSING_MSB)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Setting FCU controller flipscreen port to %04x\n",data);
		fcu_flipscreen = data & 0x8000;	/* 0x8000 = flip, 0x0000 = no flip */
	}
}

WRITE16_HANDLER( toaplan1_bcu_control_w )
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "BCU tile controller register:%02x now = %04x\n",offset,data);

	/*** Hack for Zero Wing and OutZone, to reset the sound system on */
	/*** soft resets. These two games don't have a sound reset port,  */
	/*** unlike the other games */

	if (toaplan1_unk_reset_port && toaplan1_reset)
	{
		toaplan1_reset = 0;
		toaplan1_reset_sound(0,0,0);
	}

}



READ16_HANDLER( toaplan1_spriteram_offs_r )
{
	return spriteram_offs;
}

WRITE16_HANDLER( toaplan1_spriteram_offs_w )
{
	COMBINE_DATA(&spriteram_offs);
}


/* tile palette */
READ16_HANDLER( toaplan1_colorram1_r )
{
	return toaplan1_colorram1[offset];
}

WRITE16_HANDLER( toaplan1_colorram1_w )
{
	COMBINE_DATA(&toaplan1_colorram1[offset]);
	paletteram16_xBBBBBGGGGGRRRRR_word_w(offset, data, 0);
}

/* sprite palette */
READ16_HANDLER( toaplan1_colorram2_r )
{
	return toaplan1_colorram2[offset];
}

WRITE16_HANDLER( toaplan1_colorram2_w )
{
	COMBINE_DATA(&toaplan1_colorram2[offset]);
	paletteram16_xBBBBBGGGGGRRRRR_word_w(offset+(toaplan1_colorram1_size/2), data, 0);
}

READ16_HANDLER( toaplan1_spriteram16_r )
{
	return spriteram16[spriteram_offs & ((TOAPLAN1_SPRITERAM_SIZE/2)-1)];
}

WRITE16_HANDLER( toaplan1_spriteram16_w )
{
	COMBINE_DATA(&spriteram16[spriteram_offs & ((TOAPLAN1_SPRITERAM_SIZE/2)-1)]);

#ifdef MAME_DEBUG
	if (spriteram_offs >= (TOAPLAN1_SPRITERAM_SIZE/2))
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Sprite_RAM_word_w, %08x out of range !\n", spriteram_offs);
		return;
	}
#endif

	spriteram_offs++;
}

READ16_HANDLER( toaplan1_spritesizeram16_r )
{
	return toaplan1_spritesizeram16[spriteram_offs & ((TOAPLAN1_SPRITESIZERAM_SIZE/2)-1)];
}

WRITE16_HANDLER( toaplan1_spritesizeram16_w )
{
	COMBINE_DATA(&toaplan1_spritesizeram16[spriteram_offs & ((TOAPLAN1_SPRITESIZERAM_SIZE/2)-1)]);

#ifdef MAME_DEBUG
	if (spriteram_offs >= (TOAPLAN1_SPRITESIZERAM_SIZE/2))
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Sprite_Size_RAM_word_w, %08x out of range !\n", spriteram_offs);
		return;
	}
#endif

	spriteram_offs++;	/* really ? shouldn't happen on the sizeram */
}

READ16_HANDLER( toaplan1_tileram_offs_r )
{
	return tileram_offs;
}

WRITE16_HANDLER( toaplan1_tileram_offs_w )
{
	COMBINE_DATA(&tileram_offs);
	toaplan1_reset = 1;
}

READ16_HANDLER( rallybik_tileram16_r )
{
	data16_t data = toaplan1_tileram16[((tileram_offs * 2) + offset) & (((TOAPLAN1_TILEVRAM_SIZE*4)/2)-1)];

	if (offset == 0)	/* some bit lines may be stuck to others */
	{
		data |= ((data & 0xf000) >> 4);
		data |= ((data & 0x0030) << 2);
	}
	return data;
}

READ16_HANDLER( toaplan1_tileram16_r )
{
	data16_t data = toaplan1_tileram16[((tileram_offs * 2) + offset) & (((TOAPLAN1_TILEVRAM_SIZE*4)/2)-1)];

	return data;
}

WRITE16_HANDLER( toaplan1_tileram16_w )
{
	COMBINE_DATA(&toaplan1_tileram16[((tileram_offs * 2) + offset) & (((TOAPLAN1_TILEVRAM_SIZE*4)/2)-1)]);

#ifdef MAME_DEBUG
	if ( ((tileram_offs * 2) + offset) >= ((TOAPLAN1_TILEVRAM_SIZE*4)/2) )
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Tile_RAM_w, %08x out of range !\n", tileram_offs);
		return;
	}
#endif

	if ( offset == 1 ) tileram_offs++;
}

READ16_HANDLER( toaplan1_scroll_regs_r )
{
	offset = offset & 7;
	return scrollregs[offset];
}

WRITE16_HANDLER( toaplan1_scroll_regs_w )
{
	offset = offset & 7;
	COMBINE_DATA(&scrollregs[offset]);
	toaplan1_set_scrolls();
}

/***************************************************************************

  Draw the game screen in the given mame_bitmap.

***************************************************************************/


static void toaplan1_find_tiles( void )
{
	int priority;
	int layer;
	tile_struct *tinfo;
	data16_t *t_info;

	for ( priority = 0; priority < 16; priority++ )
	{
		tile_count[priority] = 0;
	}

	for ( layer = 3; layer >= 0; layer-- )
	{
		int scrolly,scrollx,offsetx,offsety;
		int sx,sy,tattr;
		int i;

#ifdef BGDBG
		if( toaplan_dbg_layer[layer] == 1 ){
#endif

		t_info = toaplan1_tileram16 + (layer * (TOAPLAN1_TILEVRAM_SIZE/2));
		scrollx = layer_scrollx[layer];
		offsetx = scrollx / 8;
		scrolly = layer_scrolly[layer];
		offsety = scrolly / 8;
		layer_offsetx[layer] = scrollx & 0x7;
		layer_offsety[layer] = scrolly & 0x7;

		for ( sy = 0; sy < 33; sy++ )
	{
			for ( sx = 0; sx <= 40; sx++ )
			{
				i = ((sy+offsety)&0x3f)*128 + ((sx+offsetx)&0x3f)*2;
				tattr = t_info[i];
				priority = (tattr >> 12);

				tinfo = (tile_struct *)&(bg_list[layer][sy*41+sx]);
				tinfo->tile_num = t_info[i+1];
				tinfo->priority = priority;
				tinfo->color = tattr & 0x3f;
				tinfo->color |= layer<<8;
				tinfo->xpos = (sx*8)-(scrollx&0x7);
				tinfo->ypos = (sy*8)-(scrolly&0x7);

				if ( (priority) || (layer == 0) )	/* if priority 0 draw layer 0 only */
				{
					tinfo = (tile_struct *)&(tile_list[priority][tile_count[priority]]);
					tinfo->tile_num = t_info[i+1];
					if ( !((priority) && (tinfo->tile_num & 0x8000)) )
/*					if ( (tinfo->tile_num & 0x8000) == 0 ) */
					{
						tinfo->priority = priority;
						tinfo->color = tattr & 0x3f;
						tinfo->color |= layer<<8;
						tinfo->xpos = (sx*8)-(scrollx&0x7);
						tinfo->ypos = (sy*8)-(scrolly&0x7);
						tile_count[priority]++;
						if(tile_count[priority]==max_list_size[priority]){
							log_cb(RETRO_LOG_DEBUG, LOGPRE " Tile buffer over flow !! %08x\n",priority);
						}
					}
				}
			}
		}
#ifdef BGDBG
		}
#endif
	}
}


static void toaplan1_find_sprites (void)
{
	int priority;
	int sprite;
	data16_t *s_info;
	data16_t *s_size;


	tile_count[16] = 0;

	s_size = toaplan1_buffered_spritesizeram16;	/* sprite block size */
	s_info = buffered_spriteram16;				/* start of sprite ram */

	for ( sprite = 0; sprite < 256; sprite++ )
	{
		int tattr,tchar;

		tchar = s_info[0];
		tattr = s_info[1];

		if ( (tchar & 0x8000) == 0 )
		{
			int sx,sy,dx,dy,s_sizex,s_sizey;
			int sprite_size_ptr;

			sx=s_info[2];
			sx >>= 7;
			if ( sx > 416 ) sx -= 512;

			sy=s_info[3];
			sy >>= 7;
			if ( sy > 416 ) sy -= 512;

			priority = (tattr >> 12);

			sprite_size_ptr = (tattr>>6)&0x3f;
			s_sizey = (s_size[sprite_size_ptr]>>4)&0xf;
			s_sizex =  s_size[sprite_size_ptr]    &0xf;

			for ( dy = s_sizey; dy > 0; dy-- )
			{
				for ( dx = s_sizex; dx > 0; dx-- )
	{
					tile_struct *tinfo;

					tinfo = (tile_struct *)&(tile_list[16][tile_count[16]]);
					tinfo->priority = priority;
					tinfo->tile_num = tchar;
					tinfo->color = 0x80 | (tattr & 0x3f);
					tinfo->xpos = sx-dx*8+s_sizex*8;
					tinfo->ypos = sy-dy*8+s_sizey*8 + flip_y_offs;
					tile_count[16]++;
					if(tile_count[16]==max_list_size[16]){
						log_cb(RETRO_LOG_DEBUG, LOGPRE " Tile buffer over flow !! %08x\n",priority);
					}
					tchar++;
				}
			}
		}
		s_info += 4;
	}
}

static void rallybik_find_sprites (void)
{
	int offs;
	int tattr;
	int sx,sy,tchar;
	int priority;
	tile_struct *tinfo;

	for (offs = 0;offs < (spriteram_size/2);offs += 4)
	{
		tattr = buffered_spriteram16[offs+1];
		if ( tattr )	/* no need to render hidden sprites */
		{
			sx=buffered_spriteram16[offs+2];
			sx >>= 7;
			sx &= 0x1ff;
			if ( sx > 416 ) sx -= 512;

			sy=buffered_spriteram16[offs+3];
			sy >>= 7;
			sy &= 0x1ff;
			if ( sy > 416 ) sy -= 512;

			priority = (tattr>>8) & 0xc;
			tchar = buffered_spriteram16[offs];
			tinfo = (tile_struct *)&(tile_list[priority][tile_count[priority]]);
			tinfo->tile_num = tchar & 0x7ff;
			tinfo->color = 0x80 | (tattr&0x3f);
			tinfo->color |= (tattr & 0x0100);
			tinfo->color |= (tattr & 0x0200);
			if (tinfo->color & 0x0100) sx -= 15;

			tinfo->xpos = sx-31;
			tinfo->ypos = sy-16;
			tile_count[priority]++;
			if(tile_count[priority]==max_list_size[priority]){
				log_cb(RETRO_LOG_DEBUG, LOGPRE " Tile buffer over flow !! %08x\n",priority);
	}
		}  /* if tattr */
	}  /* for sprite */
}



void toaplan1_sprite_mask
	(
	struct mame_bitmap *dest_bmp,
	struct mame_bitmap *src_bmp,
	const struct rectangle *clip
	)
{
	struct rectangle myclip;
	int sx=0;
	int sy=0;
	int transparent_color;

	transparent_color = Machine->pens[0];

	if (0) 
	{
		int temp;

		/* clip and myclip might be the same, so we need a temporary storage */
		temp = clip->min_x;
		myclip.min_x = clip->min_y;
		myclip.min_y = temp;
		temp = clip->max_x;
		myclip.max_x = clip->max_y;
		myclip.max_y = temp;
		clip = &myclip;
	}
	if (0)
		{
		int temp;

		sx = -sx;

		/* clip and myclip might be the same, so we need a temporary storage */
		temp = clip->min_x;
		myclip.min_x = dest_bmp->width-1 - clip->max_x;
		myclip.max_x = dest_bmp->width-1 - temp;
		myclip.min_y = clip->min_y;
		myclip.max_y = clip->max_y;
		clip = &myclip;
	}
	if (0)
			{
		int temp;

		sy = -sy;

		myclip.min_x = clip->min_x;
		myclip.max_x = clip->max_x;
		/* clip and myclip might be the same, so we need a temporary storage */
		temp = clip->min_y;
		myclip.min_y = dest_bmp->height-1 - clip->max_y;
		myclip.max_y = dest_bmp->height-1 - temp;
		clip = &myclip;
			}

	{
		int ex = sx+src_bmp->width;
		int ey = sy+src_bmp->height;

		if( sx < clip->min_x)
		{ /* clip left */
			sx = clip->min_x;
		}
		if( sy < clip->min_y )
		{ /* clip top */
			sy = clip->min_y;
		}
		if( ex > clip->max_x+1 )
		{ /* clip right */
			ex = clip->max_x + 1;
		}
		if( ey > clip->max_y+1 )
		{ /* clip bottom */
			ey = clip->max_y + 1;
		}

		if( ex>sx )
		{ /* skip if inner loop doesn't draw anything */
			int y;

			for( y=sy; y<ey; y++ )
		{
				unsigned short *dest = (unsigned short *)dest_bmp->line[y];
				unsigned short *source = (unsigned short *)src_bmp->line[y];
				int x;

				for( x=sx; x<ex; x++ )
			{
					int c = source[x];
					if( c != transparent_color )
						dest[x] = transparent_color;
				}
			}
		}
	}
}



void toaplan1_sprite_copy
	(
	struct mame_bitmap *dest_bmp,
	struct mame_bitmap *src_bmp,
	struct mame_bitmap *look_bmp,
	const struct rectangle *clip
	)
{
	struct rectangle myclip;
	int sx=0;
	int sy=0;
	int transparent_color;

	transparent_color = Machine->pens[0];

	if (0)
	{
		int temp;

		/* clip and myclip might be the same, so we need a temporary storage */
		temp = clip->min_x;
		myclip.min_x = clip->min_y;
		myclip.min_y = temp;
		temp = clip->max_x;
		myclip.max_x = clip->max_y;
		myclip.max_y = temp;
		clip = &myclip;
	}
	if (0)
	{
		int temp;

		sx = -sx;

		/* clip and myclip might be the same, so we need a temporary storage */
		temp = clip->min_x;
		myclip.min_x = dest_bmp->width-1 - clip->max_x;
		myclip.max_x = dest_bmp->width-1 - temp;
		myclip.min_y = clip->min_y;
		myclip.max_y = clip->max_y;
		clip = &myclip;
	}
	if (0)
		{
		int temp;

		sy = -sy;

		myclip.min_x = clip->min_x;
		myclip.max_x = clip->max_x;
		/* clip and myclip might be the same, so we need a temporary storage */
		temp = clip->min_y;
		myclip.min_y = dest_bmp->height-1 - clip->max_y;
		myclip.max_y = dest_bmp->height-1 - temp;
		clip = &myclip;
	}

			{
		int ex = sx+src_bmp->width;
		int ey = sy+src_bmp->height;

		if( sx < clip->min_x)
		{ /* clip left */
			sx = clip->min_x;
		}
		if( sy < clip->min_y )
		{ /* clip top */
			sy = clip->min_y;
			}
		if( ex > clip->max_x+1 )
		{ /* clip right */
			ex = clip->max_x + 1;
		}
		if( ey > clip->max_y+1 )
		{ /* clip bottom */
			ey = clip->max_y + 1;
	}

		if( ex>sx )
		{ /* skip if inner loop doesn't draw anything */
			int y;

			for( y=sy; y<ey; y++ )
	{
				unsigned short *dest = (unsigned short *)dest_bmp->line[y];
				unsigned short *source = (unsigned short *)src_bmp->line[y];
				unsigned short *look = (unsigned short *)look_bmp->line[y];
				int x;

				for( x=sx; x<ex; x++ )
		{
					if( look[x] != transparent_color )
						dest[x] = source[x];
		}
	}
		}
	}
}



void toaplan1_sprite_0_copy
	(
	struct mame_bitmap *dest_bmp,
	struct mame_bitmap *look_bmp,
	const struct rectangle *clip
	)
{
	struct rectangle myclip;
	int sx=0;
	int sy=0;
	int transparent_color;

	transparent_color = Machine->pens[0];

	if (0)
	{
		int temp;

		/* clip and myclip might be the same, so we need a temporary storage */
		temp = clip->min_x;
		myclip.min_x = clip->min_y;
		myclip.min_y = temp;
		temp = clip->max_x;
		myclip.max_x = clip->max_y;
		myclip.max_y = temp;
		clip = &myclip;
	}
	if (0)
	{
		int temp;

		sx = -sx;

		/* clip and myclip might be the same, so we need a temporary storage */
		temp = clip->min_x;
		myclip.min_x = dest_bmp->width-1 - clip->max_x;
		myclip.max_x = dest_bmp->width-1 - temp;
		myclip.min_y = clip->min_y;
		myclip.max_y = clip->max_y;
		clip = &myclip;
	}
	if (0)
	{
		int temp;

		sy = -sy;

		myclip.min_x = clip->min_x;
		myclip.max_x = clip->max_x;
		/* clip and myclip might be the same, so we need a temporary storage */
		temp = clip->min_y;
		myclip.min_y = dest_bmp->height-1 - clip->max_y;
		myclip.max_y = dest_bmp->height-1 - temp;
		clip = &myclip;
	}

	{
		int ex = sx+look_bmp->width;
		int ey = sy+look_bmp->height;

		if( sx < clip->min_x)
		{ /* clip left */
			sx = clip->min_x;
	}
		if( sy < clip->min_y )
		{ /* clip top */
			sy = clip->min_y;
	}
		if( ex > clip->max_x+1 )
		{ /* clip right */
			ex = clip->max_x + 1;
	}
		if( ey > clip->max_y+1 )
		{ /* clip bottom */
			ey = clip->max_y + 1;
	}

		if( ex>sx )
		{ /* skip if inner loop doesn't draw anything */
			int y;

			for( y=sy; y<ey; y++ )
	{
				unsigned short *dest = (unsigned short *)dest_bmp->line[y];
				unsigned short *look = (unsigned short *)look_bmp->line[y];
				int x;

				for( x=sx; x<ex; x++ )
	{
					if( look[x] != transparent_color )
						dest[x] = transparent_color;
	}
	}
	}
	}
}



static void toaplan1_render (struct mame_bitmap *bitmap)
{
	int i;
	int priority,pen;
	int flip;
	tile_struct *tinfo;

	fillbitmap (bitmap, Machine->pens[0x120], &Machine->visible_area);

#ifdef BGDBG

if (keyboard_pressed(KEYCODE_Q)) { toaplan_dbg_priority = 0; }
if (keyboard_pressed(KEYCODE_W)) { toaplan_dbg_priority = 1; }
if (keyboard_pressed_memory(KEYCODE_E)) { toaplan_dbg_priority = 2; }
if (keyboard_pressed_memory(KEYCODE_R)) { toaplan_dbg_priority = 3; }
if (keyboard_pressed_memory(KEYCODE_T)) { toaplan_dbg_priority = 4; }
if (keyboard_pressed_memory(KEYCODE_Y)) { toaplan_dbg_priority = 5; }
if (keyboard_pressed_memory(KEYCODE_U)) { toaplan_dbg_priority = 6; }
if (keyboard_pressed_memory(KEYCODE_I)) { toaplan_dbg_priority = 7; }
if (keyboard_pressed_memory(KEYCODE_A)) { toaplan_dbg_priority = 8; }
if (keyboard_pressed_memory(KEYCODE_S)) { toaplan_dbg_priority = 9; }
if (keyboard_pressed_memory(KEYCODE_D)) { toaplan_dbg_priority = 10; }
if (keyboard_pressed_memory(KEYCODE_F)) { toaplan_dbg_priority = 11; }
if (keyboard_pressed_memory(KEYCODE_G)) { toaplan_dbg_priority = 12; }
if (keyboard_pressed_memory(KEYCODE_H)) { toaplan_dbg_priority = 13; }
if (keyboard_pressed_memory(KEYCODE_J)) { toaplan_dbg_priority = 14; }
if (keyboard_pressed_memory(KEYCODE_K)) { toaplan_dbg_priority = 15; }

if (keyboard_pressed_memory(KEYCODE_Z)) { toaplan_dbg_sprite_only ^= 1; }
if (keyboard_pressed_memory(KEYCODE_X)) { toaplan_dbg_layer[0] ^= 1; }
if (keyboard_pressed_memory(KEYCODE_C)) { toaplan_dbg_layer[1] ^= 1; }
if (keyboard_pressed_memory(KEYCODE_V)) { toaplan_dbg_layer[2] ^= 1; }
if (keyboard_pressed_memory(KEYCODE_B)) { toaplan_dbg_layer[3] ^= 1; }

if( toaplan_dbg_priority != 0 ){

	priority = toaplan_dbg_priority;
	{
		tinfo = (tile_struct *)&(tile_list[priority][0]);
		pen = TRANSPARENCY_NONE;
		for ( i = 0; i < tile_count[priority]; i++ ) /* draw only tiles in list */
		{
			drawgfx(bitmap,Machine->gfx[0],
				tinfo->tile_num,
				(tinfo->color&0x3f),
				0,0,						/* flipx,flipy */
				tinfo->xpos,tinfo->ypos,
				&Machine->visible_area,pen,0);
			tinfo++;
		}
	}

}else{

#endif

	if (bcu_flipscreen)
		flip = 1;
	else
		flip = 0;


#ifdef BGDBG
if ( toaplan_dbg_sprite_only == 0 ){
#endif

	priority = 0;
	while ( priority < 16 )			/* draw priority layers in order */
	{
		int layer;

		tinfo = (tile_struct *)&(tile_list[priority][0]);
		layer = (tinfo->color >> 8);

		if ( (layer == 0) && (priority >= 8 ) )
	{
			pen = TRANSPARENCY_NONE;
		}
		else{
			pen = TRANSPARENCY_PEN;
		}

		for ( i = 0; i < tile_count[priority]; i++ )	/* draw only tiles in list */
		{
			int xpos,ypos;

			if ( flip ){
				xpos = (512 - tinfo->xpos) - 8 - (512 - Machine->drv->screen_width);
				ypos = (512 - tinfo->ypos) - 8 - (512 - Machine->drv->screen_height);
		}
			else{
				xpos = tinfo->xpos;
				ypos = tinfo->ypos;
	}

			drawgfx(bitmap,Machine->gfx[0],
				tinfo->tile_num,
				(tinfo->color&0x3f),
				flip,flip,							/* flipx,flipy */
				xpos,ypos,
				&Machine->visible_area,pen,0);
			tinfo++;
		}
		priority++;
	}
#ifdef BGDBG
}
#endif

#ifdef BGDBG
}
#endif

}



static void zerowing_render (struct mame_bitmap *bitmap)
{
	int i;
	int priority,pen;
	int flip;
	tile_struct *tinfo;

	fillbitmap (bitmap, Machine->pens[0x120], &Machine->visible_area);

	if (bcu_flipscreen)
		flip = 1;
	else
		flip = 0;

	priority = 0;
	while ( priority < 16 )			/* draw priority layers in order */
	{
		int layer;

		tinfo = (tile_struct *)&(tile_list[priority][0]);
		layer = (tinfo->color >> 8);
		pen = TRANSPARENCY_PEN;

		for ( i = 0; i < tile_count[priority]; i++ ) /* draw only tiles in list */
		{
			int xpos,ypos;

			if ( flip ){
				xpos = (512 - tinfo->xpos) - 8 - (512 - Machine->drv->screen_width);
				ypos = (512 - tinfo->ypos) - 8 - (512 - Machine->drv->screen_height);
			}
			else{
				xpos = tinfo->xpos;
				ypos = tinfo->ypos;
			}

			drawgfx(bitmap,Machine->gfx[0],
				tinfo->tile_num,
				(tinfo->color&0x3f),
				flip,flip,							/* flipx,flipy */
				xpos,ypos,
				&Machine->visible_area,pen,0);
			tinfo++;
		}
		priority++;
	}
}


static void demonwld_render (struct mame_bitmap *bitmap)
{
	int i;
	int priority,pen;
	int flip;
	tile_struct *tinfo;

	fillbitmap (bitmap, Machine->pens[0], &Machine->visible_area);

#ifdef BGDBG

if (keyboard_pressed_memory(KEYCODE_Q)) { toaplan_dbg_priority = 0; }
if (keyboard_pressed_memory(KEYCODE_W)) { toaplan_dbg_priority = 1; }
if (keyboard_pressed_memory(KEYCODE_E)) { toaplan_dbg_priority = 2; }
if (keyboard_pressed_memory(KEYCODE_R)) { toaplan_dbg_priority = 3; }
if (keyboard_pressed_memory(KEYCODE_T)) { toaplan_dbg_priority = 4; }
if (keyboard_pressed_memory(KEYCODE_Y)) { toaplan_dbg_priority = 5; }
if (keyboard_pressed_memory(KEYCODE_U)) { toaplan_dbg_priority = 6; }
if (keyboard_pressed_memory(KEYCODE_I)) { toaplan_dbg_priority = 7; }
if (keyboard_pressed_memory(KEYCODE_O)) { toaplan_dbg_priority = 8; }
if (keyboard_pressed_memory(KEYCODE_A)) { toaplan_dbg_priority = 9; }
if (keyboard_pressed_memory(KEYCODE_S)) { toaplan_dbg_priority = 10; }
if (keyboard_pressed_memory(KEYCODE_D)) { toaplan_dbg_priority = 11; }
if (keyboard_pressed_memory(KEYCODE_F)) { toaplan_dbg_priority = 12; }
if (keyboard_pressed_memory(KEYCODE_G)) { toaplan_dbg_priority = 13; }
if (keyboard_pressed_memory(KEYCODE_H)) { toaplan_dbg_priority = 14; }
if (keyboard_pressed_memory(KEYCODE_J)) { toaplan_dbg_priority = 15; }

if (keyboard_pressed_memory(KEYCODE_Z)) { toaplan_dbg_sprite_only ^= 1; }
if (keyboard_pressed_memory(KEYCODE_X)) { toaplan_dbg_layer[0] ^= 1; }
if (keyboard_pressed_memory(KEYCODE_C)) { toaplan_dbg_layer[1] ^= 1; }
if (keyboard_pressed_memory(KEYCODE_V)) { toaplan_dbg_layer[2] ^= 1; }
if (keyboard_pressed_memory(KEYCODE_B)) { toaplan_dbg_layer[3] ^= 1; }

if( toaplan_dbg_priority != 0 ){

	palette_set_color(0x120,0xf,0xf,0xf);
	fillbitmap (bitmap, Machine->pens[0x120], &Machine->visible_area);
	priority = toaplan_dbg_priority - 1;
				{
		tinfo = (tile_struct *)&(tile_list[priority][0]);
		/* hack to fix black blobs in Demon's World sky */
		pen = TRANSPARENCY_NONE;
		for ( i = 0; i < tile_count[priority]; i++ ) /* draw only tiles in list */
				{
			drawgfx(bitmap,Machine->gfx[0],
				tinfo->tile_num,
				(tinfo->color&0x3f),
				0,0,						/* flipx,flipy */
				tinfo->xpos,tinfo->ypos,
				&Machine->visible_area,pen,0);
			tinfo++;
				}
			}

}else{

	if ( toaplan_dbg_sprite_only == 0 ){

#endif

	if (bcu_flipscreen)
		flip = 1;
	else
		flip = 0;

	priority = 0;
	while ( priority < 16 )			/* draw priority layers in order */
				{
		int layer;

		tinfo = (tile_struct *)&(tile_list[priority][0]);
		layer = (tinfo->color >> 8);

		if ( priority == 1 )
			pen = TRANSPARENCY_NONE;
		else
			pen = TRANSPARENCY_PEN;

		for ( i = 0; i < tile_count[priority]; i++ ) /* draw only tiles in list */
		{
			int xpos,ypos;

			if ( flip ){
				xpos = (512 - tinfo->xpos) - 8 - (512 - Machine->drv->screen_width);
				ypos = (512 - tinfo->ypos) - 8 - (512 - Machine->drv->screen_height);
			}
			else{
				xpos = tinfo->xpos;
				ypos = tinfo->ypos;
				}

			drawgfx(bitmap,Machine->gfx[0],
				tinfo->tile_num,
				(tinfo->color&0x3f),
				flip,flip,							/* flipx,flipy */
				xpos,ypos,
				&Machine->visible_area,pen,0);
			tinfo++;
			}
		priority++;
		}

#ifdef BGDBG
	}
}
#endif

}


static void rallybik_render (struct mame_bitmap *bitmap)
{
	int i;
	int priority,pen;
	int flip;
	tile_struct *tinfo;

	fillbitmap (bitmap, Machine->pens[0], &Machine->visible_area);

	if (bcu_flipscreen)
		flip = 1;
	else
		flip = 0;

	priority = 0;
	while ( priority < 16 )			/* draw priority layers in order */
	{
		tinfo = (tile_struct *)&(tile_list[priority][0]);

		if ( priority <= 1 )
			pen = TRANSPARENCY_NONE;
		else
			pen = TRANSPARENCY_PEN;

		for ( i = 0; i < tile_count[priority]; i++ )	/* draw only tiles in list */
		{
			int xpos,ypos;

			if( tinfo->color & 0x80 ){
				/* sprite */
				drawgfx(bitmap,Machine->gfx[1],
					tinfo->tile_num,
					(tinfo->color&0x3f), 				/* bit 7 not for colour */
					(tinfo->color & 0x0100),(tinfo->color & 0x0200),	/* flipx,flipy */
					tinfo->xpos,tinfo->ypos,
					&Machine->visible_area,pen,0);
			}else{
				/* BG */
				if ( flip ){
					xpos = (512 - tinfo->xpos) - 8 - (512 - Machine->drv->screen_width);
					ypos = (512 - tinfo->ypos) - 8 - (512 - Machine->drv->screen_height);
				}
				else{
					xpos = tinfo->xpos;
					ypos = tinfo->ypos;
				}
				drawgfx(bitmap,Machine->gfx[0],
					tinfo->tile_num,
					(tinfo->color&0x3f),
					flip,flip,							/* flipx,flipy */
					xpos,ypos,
					&Machine->visible_area,pen,0);
			}
			tinfo++;
		}
		priority++;
	}
}



static void toaplan1_sprite_render (struct mame_bitmap *bitmap)
{
	int i;
	int j;
	int priority;
	int flip;
	tile_struct *tinfo;
	tile_struct *tinfo_sp;
	struct rectangle sp_rect;

	fillbitmap (tmpbitmap1, Machine->pens[0],&Machine->visible_area);

	flip = 0;

	tinfo_sp = (tile_struct *)&(tile_list[16][0]);
	for ( i = 0; i < tile_count[16]; i++ )	/* draw sprite No. in order */
	{
		int	flipx,flipy;

		sp_rect.min_x = tinfo_sp->xpos;
		sp_rect.min_y = tinfo_sp->ypos;
		sp_rect.max_x = tinfo_sp->xpos + 7;
		sp_rect.max_y = tinfo_sp->ypos + 7;


		flipx = (tinfo_sp->color & 0x0100);
		flipy = (tinfo_sp->color & 0x0200);

		fillbitmap (tmpbitmap2, Machine->pens[0], &sp_rect);

		drawgfx(tmpbitmap2,Machine->gfx[1],
			tinfo_sp->tile_num,
			(tinfo_sp->color&0x3f),			/* bit 7 not for colour */
			flipx,flipy,					/* flipx,flipy */
			tinfo_sp->xpos,tinfo_sp->ypos,
			&Machine->visible_area,TRANSPARENCY_PEN,0
		);

		fillbitmap (tmpbitmap3, Machine->pens[0], &sp_rect);

		priority = tinfo_sp->priority;
		{
		int ix0,ix1,ix2,ix3;
		int dirty;

		dirty = 0;
		for ( j = 0; j < 4; j++ )
		{
			int x,y;

			y = tinfo_sp->ypos+layer_offsety[j];
			x = tinfo_sp->xpos+layer_offsetx[j];
			ix0 = ( y   /8) * 41 +  x   /8;
			ix1 = ( y   /8) * 41 + (x+7)/8;
			ix2 = ((y+7)/8) * 41 +  x   /8;
			ix3 = ((y+7)/8) * 41 + (x+7)/8;

			if(	(ix0 >= 0) && (ix0 < 32*41) ){
				tinfo = (tile_struct *)&(bg_list[j][ix0]);
				if( tinfo->priority >= priority ){
					drawgfx(tmpbitmap3,Machine->gfx[0],
						tinfo->tile_num,
						(tinfo->color&0x3f),
						flip,flip,
						tinfo->xpos,tinfo->ypos,
						&Machine->visible_area,TRANSPARENCY_PEN,0
					);
					dirty=1;
				}
			}
			if(	(ix0 != ix1) && (ix1 >= 0) && (ix1 < 32*41) ){
				tinfo = (tile_struct *)&(bg_list[j][ix1]);
				if( tinfo->priority >= priority ){
					drawgfx(tmpbitmap3,Machine->gfx[0],
						tinfo->tile_num,
						(tinfo->color&0x3f),
						flip,flip,
						tinfo->xpos,tinfo->ypos,
						&Machine->visible_area,TRANSPARENCY_PEN,0
					);
					dirty=1;
				}
			}
			if(	(ix1 != ix2) && (ix2 >= 0) && (ix2 < 32*41) ){
				tinfo = (tile_struct *)&(bg_list[j][ix2]);
				if( tinfo->priority >= priority ){
					drawgfx(tmpbitmap3,Machine->gfx[0],
						tinfo->tile_num,
						(tinfo->color&0x3f),
						flip,flip,
						tinfo->xpos,tinfo->ypos,
						&Machine->visible_area,TRANSPARENCY_PEN,0
					);
					dirty=1;
				}
			}
			if( (ix2 != ix3) && (ix3 >= 0) && (ix3 < 32*41) ){
				tinfo = (tile_struct *)&(bg_list[j][ix3]);
				if( tinfo->priority >= priority ){
					drawgfx(tmpbitmap3,Machine->gfx[0],
						tinfo->tile_num,
						(tinfo->color&0x3f),
						flip,flip,
						tinfo->xpos,tinfo->ypos,
						&Machine->visible_area,TRANSPARENCY_PEN,0
					);
					dirty=1;
				}
			}
		}
		if( dirty != 0 )
		{
			toaplan1_sprite_mask(
				tmpbitmap2,
				tmpbitmap3,
				&sp_rect
			);
			fillbitmap (tmpbitmap3, Machine->pens[0], &sp_rect);
			drawgfx(tmpbitmap3,Machine->gfx[1],
				tinfo_sp->tile_num,
				(tinfo_sp->color&0x3f),
				flipx,flipy,
				tinfo_sp->xpos,tinfo_sp->ypos,
				&Machine->visible_area,TRANSPARENCY_PEN,0
			);
			if ( priority == 0 ){			/* demonwld : sprite mask effect in BOSS dying */
				toaplan1_sprite_0_copy(
					tmpbitmap1,
					tmpbitmap3,
					&sp_rect
				);
			}else{
				toaplan1_sprite_copy(
					tmpbitmap1,
					tmpbitmap2,
					tmpbitmap3,
					&sp_rect
				);
			}

		}else{
			drawgfx(tmpbitmap1,Machine->gfx[1],
				tinfo_sp->tile_num,
				(tinfo_sp->color&0x3f),
				flipx,flipy,
				tinfo_sp->xpos,tinfo_sp->ypos,
				&Machine->visible_area,TRANSPARENCY_PEN,0
			);
		}
		}
		tinfo_sp++;
	}

	copybitmap(bitmap, tmpbitmap1, 
		fcu_flipscreen, fcu_flipscreen,
		0, 0,
		&Machine->visible_area, TRANSPARENCY_PEN, 0
	);

}



/* void toaplan1_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh) */
VIDEO_UPDATE( toaplan1 )
{
	
	memcpy(buffered_spriteram16, spriteram16, TOAPLAN1_SPRITERAM_SIZE);	/* ghetto fix 4x */
	memcpy(toaplan1_buffered_spritesizeram16, toaplan1_spritesizeram16, TOAPLAN1_SPRITESIZERAM_SIZE);
	/* discover what data will be drawn */
	toaplan1_find_sprites();
	toaplan1_find_tiles();

	toaplan1_render(bitmap);
	toaplan1_sprite_render(bitmap);
}

/* void zerowing_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh) */
VIDEO_UPDATE( zerowing )
{
	memcpy(buffered_spriteram16, spriteram16, TOAPLAN1_SPRITERAM_SIZE);
	memcpy(toaplan1_buffered_spritesizeram16, toaplan1_spritesizeram16, TOAPLAN1_SPRITESIZERAM_SIZE);
	/* discover what data will be drawn */
	toaplan1_find_sprites();
	toaplan1_find_tiles();

	zerowing_render(bitmap);
	toaplan1_sprite_render(bitmap);
}

/* void demonwld_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh) */
VIDEO_UPDATE( demonwld )
{
	memcpy(buffered_spriteram16, spriteram16, TOAPLAN1_SPRITERAM_SIZE);
	memcpy(toaplan1_buffered_spritesizeram16, toaplan1_spritesizeram16, TOAPLAN1_SPRITESIZERAM_SIZE);
	/* discover what data will be drawn */
	toaplan1_find_sprites();
	toaplan1_find_tiles();

	demonwld_render(bitmap);
	toaplan1_sprite_render(bitmap);
}

/* void rallybik_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh) */
VIDEO_UPDATE( rallybik )
{
	buffer_spriteram16_w(0, 0, 0);
	/* discover what data will be drawn */
	toaplan1_find_tiles();
	rallybik_find_sprites();

	rallybik_render(bitmap);
}

/****************************************************************************
    Spriteram is always 1 frame ahead, suggesting spriteram buffering.
    There are no CPU output registers that control this so we
    assume it happens automatically every frame, at the end of vblank
****************************************************************************/

/* void toaplan1_eof_callback(void) */
VIDEO_EOF( toaplan1 )
{
	memcpy(buffered_spriteram16, spriteram16, TOAPLAN1_SPRITERAM_SIZE);
	memcpy(toaplan1_buffered_spritesizeram16, toaplan1_spritesizeram16, TOAPLAN1_SPRITESIZERAM_SIZE);
}

/* void rallybik_eof_callback(void) */
VIDEO_EOF( rallybik )
{
	buffer_spriteram16_w(0, 0, 0);
}

/* void samesame_eof_callback(void) */
VIDEO_EOF( samesame )
{
	memcpy(buffered_spriteram16, spriteram16, TOAPLAN1_SPRITERAM_SIZE);
	memcpy(toaplan1_buffered_spritesizeram16, toaplan1_spritesizeram16, TOAPLAN1_SPRITESIZERAM_SIZE);
	cpu_set_irq_line(0, MC68000_IRQ_2, HOLD_LINE);	/* Frame done */
}
