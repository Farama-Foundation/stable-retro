/***************************************************************************

							-= Jaleco Mega System 1 =-

					driver by	Luca Elia (l.elia@tin.it)



**********  There are 3 scrolling layers, 1 word per tile:

* Note: MS1-Z has 2 layers only.

  A page is 256x256, approximately the visible screen size. Each layer is
  made up of 8 pages (8x8 tiles) or 32 pages (16x16 tiles). The number of
  horizontal  pages and the tiles size  is selectable, using the  layer's
  control register. I think that when tiles are 16x16 a layer can be made
  of 16x2, 8x4, 4x8 or 2x16 pages (see below). When tile size is 8x8 we
  have two examples to guide the choice:

  the copyright screen of p47j (0x12) should be 4x2 (unless it's been hacked :)
  the ending sequence of 64th street (0x13) should be 2x4.

  I don't see a relation.


MS1-A MS1-B MS1-C
-----------------

					Scrolling layers:

90000 50000 e0000	Scroll 0
94000 54000 e8000	Scroll 1
98000 58000 f0000	Scroll 2					* Note: missing on MS1-Z

Tile format:	fedc------------	Palette
				----ba9876543210	Tile Number



84000 44000 c2208	Layers Enable				* Note: missing on MS1-Z?

	fedc ---- ---- ---- unused
	---- ba98 ---- ----	Priority Code
	---- ---- 7654 ----	unused
	---- ---- ---- 3---	Enable Sprites
	---- ---- ---- -210	Enable Layer 210

	(Note that the bottom layer can't be disabled)


84200 44200 c2000	Scroll 0 Control
84208 44208 c2008	Scroll 1 Control
84008 44008 c2100	Scroll 2 Control		* Note: missing on MS1-Z

Offset:		00						Scroll X
			02						Scroll Y
			04 fedc ba98 765- ----	? (unused?)
			   ---- ---- ---4 ----	0<->16x16 Tiles	1<->8x8 Tiles
			   ---- ---- ---- 32--	? (used, by p47!)
			   ---- ---- ---- --10	N: Layer H pages = 16 / (2^N)



84300 44300 c2308	Screen Control

	fed- ---- ---- ---- 	? (unused?)
	---c ---- ---- ---- 	? (on, troughout peekaboo)
	---- ba9- ---- ---- 	? (unused?)
	---- ---8 ---- ---- 	Portrait F/F (?FullFill?)
	---- ---- 765- ---- 	? (unused?)
	---- ---- ---4 ---- 	Reset Sound CPU (1->0 Transition)
	---- ---- ---- 321- 	? (unused?)
	---- ---- ---- ---0		Flip Screen



**********  There are 256*4 colors (256*3 for MS1-Z):

Colors		MS1-A/C			MS1-Z

000-0ff		Scroll 0		Scroll 0
100-1ff		Scroll 1		Sprites
200-2ff		Scroll 2		Scroll 1
300-3ff		Sprites			-

88000 48000 f8000	Palette

	fedc--------3---	Red
	----ba98-----2--	Blue
	--------7654--1-	Green
	---------------0	? (used, not RGB! [not changed in fades])


**********  There are 256 sprites (128 for MS1-Z):

&RAM[8000]	Sprite Data	(16 bytes/entry. 128? entries)

Offset:		0-6						? (used, but as normal RAM, I think)
			08 	fed- ---- ---- ----	?
				---c ---- ---- ----	mosaic sol.	(?)
				---- ba98 ---- ----	mosaic		(?)
				---- ---- 7--- ----	y flip
				---- ---- -6-- ----	x flip
				---- ---- --45 ----	?
				---- ---- ---- 3210	color code (* bit 3 = priority *)
			0A						H position
			0C						V position
			0E	fedc ---- ---- ----	? (used by p47j, 0-8!)
				---- ba98 7654 3210	Number



Object RAM tells the hw how to use Sprite Data (missing on MS1-Z).
This makes it possible to group multiple small sprite, up to 256,
into one big virtual sprite (up to a whole screen):

8e000 4e000 d2000	Object RAM (8 bytes/entry. 256*4 entries)

Offset:		00	Index into Sprite Data RAM
			02	H	Displacement
			04	V	Displacement
			06	Number	Displacement

Only one of these four 256 entries is used to see if the sprite is to be
displayed, according to this latter's flipx/y state:

Object RAM entries:		Used by sprites with:

000-0ff					No Flip
100-1ff					Flip X
200-2ff					Flip Y
300-3ff					Flip X & Y




No? No? c2108	Sprite Bank

	fedc ba98 7654 321- ? (unused?)
	---- ---- ---- ---0 Sprite Bank



84100 44100 c2200 Sprite Control

			fedc ba9- ---- ---- ? (unused?)
			---- ---8 ---- ---- Enable Sprite Splitting In 2 Groups:
								Some Sprite Appear Over, Some Below The Layers
			---- ---- 765- ---- ? (unused?)
			---- ---- ---4 ----	Enable Effect (?)
			---- ---- ---- 3210	Effect Number (?)

I think bit 4 enables some sort of color cycling for sprites having priority
bit set. See code of p7j at 6488,  affecting the rotating sprites before the
Jaleco logo is shown: values 11-1f, then 0. I fear the Effect Number is an
offset to be applied over the pens used by those sprites. As for bit 8, it's
not used during game, but it is turned on when sprite/foreground priority is
tested, along with Effect Number being 1, so ...


**********  Priorities (ouch!)

[ Sprite / Sprite order ]

	[MS1-A,B,C]		From first in Object RAM (frontmost) to last.
	[MS1-Z]			From last in Sprite RAM (frontmost) to first.

[ Layer / Layer & Sprite / Layer order ]

Controlled by:

	* bits 7-4 (16 values) of the Layer Control register
	* bit 4 of the Sprite Control register

		Layer Control	Sprite Control
MS1-Z	-
MS1-A	84000			84100
MS1-B	44000			44100
MS1-C	c2208			c2200

When bit 4 of the Sprite Contol register is set, sprites with color
code 0-7 and sprites with color 8-f form two groups. Each group can
appear over or below some layers.

The 16 values in the Layer Control register determine the order of
the layers, and of the groups of sprites.

There is a PROM that translates the values in the register to the
actual code sent to the hardware.


***************************************************************************/

#include "driver.h"
#include "megasys1.h"
#include "vidhrdw/generic.h"

/* Variables defined here, that have to be shared: */
struct tilemap *megasys1_tmap[3];

data16_t *megasys1_scrollram_0, *megasys1_scrollram_1, *megasys1_scrollram_2;
data16_t *megasys1_objectram, *megasys1_vregs, *megasys1_ram;

int megasys1_scroll_flag[3], megasys1_scrollx[3], megasys1_scrolly[3], megasys1_pages_per_tmap_x[3], megasys1_pages_per_tmap_y[3];
int megasys1_active_layers, megasys1_sprite_bank;
int megasys1_screen_flag, megasys1_sprite_flag;
int megasys1_bits_per_color_code;
int megasys1_8x8_scroll_0_factor, megasys1_16x16_scroll_0_factor;
int megasys1_8x8_scroll_1_factor, megasys1_16x16_scroll_1_factor;
int megasys1_8x8_scroll_2_factor, megasys1_16x16_scroll_2_factor;


/* Variables defined in driver: */
static int hardware_type_z;
static UINT16 *megasys1_buffer_objectram,*megasys1_buffer2_objectram,*megasys1_buffer_spriteram16,*megasys1_buffer2_spriteram16;


#ifdef MAME_DEBUG

#define SHOW_WRITE_ERROR(_format_,_offset_,_data_)\
{ \
	usrintf_showmessage(_format_,_offset_,_data_);\
	log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #0 PC %06X : Warning, ",activecpu_get_pc()); \
	log_cb(RETRO_LOG_DEBUG, LOGPRE _format_,_offset_,_data_);\
	log_cb(RETRO_LOG_DEBUG, LOGPRE "\n");\
}

#else

#define SHOW_WRITE_ERROR(_format_,_offset_,_data_)\
{\
	log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #0 PC %06X : Warning, ",activecpu_get_pc()); \
	log_cb(RETRO_LOG_DEBUG, LOGPRE _format_,_offset_,_data_); \
	log_cb(RETRO_LOG_DEBUG, LOGPRE "\n");\
}

#endif



#define MEGASYS1_VREG_FLAG(_n_) \
		megasys1_scroll_##_n_##_flag_w(new_data); \
		if (megasys1_tmap[_n_] == 0) SHOW_WRITE_ERROR("vreg %04X <- %04X NO MEMORY FOR SCREEN",offset*2,data);





extern struct GameDriver driver_lomakai;
extern struct GameDriver driver_soldamj;


VIDEO_START( megasys1 )
{
	int i;

	spriteram16 = &megasys1_ram[0x8000/2];
	megasys1_buffer_objectram = auto_malloc(0x2000);
	megasys1_buffer_spriteram16 = auto_malloc(0x2000);
	megasys1_buffer2_objectram = auto_malloc(0x2000);
	megasys1_buffer2_spriteram16 = auto_malloc(0x2000);
	
	megasys1_tmap[0] = megasys1_tmap[1] = megasys1_tmap[2] = NULL;

	megasys1_active_layers = megasys1_sprite_bank = megasys1_screen_flag = megasys1_sprite_flag = 0;

 	for (i = 0; i < 3; i ++)
	{
		megasys1_scroll_flag[i] = megasys1_scrollx[i] = megasys1_scrolly[i] = 0;
		megasys1_pages_per_tmap_x[i] = megasys1_pages_per_tmap_y[i] = 0;
	}

 	megasys1_bits_per_color_code = 4;

/*
	The tile code of a specific layer is multiplied for a constant
	depending on the tile mode (8x8 or 16x16)

	The most reasonable arrangement seems a 1:1 mapping (meaning we
	must multiply by 4 the tile code in 16x16 mode, since we decode
	the graphics like 8x8)

	However, this is probably a game specific thing, as Soldam uses
	layer 1 in both modes, and even with 8x8 tiles the tile code must
	be multiplied by 4!

	AFAIK, the other games use a layer in one mode only (always 8x8 or
	16x16) so it could be that the multiplication factor is constant
	for each layer and hardwired to 1x or 4x for both tile sizes
*/

	megasys1_8x8_scroll_0_factor = 1;	megasys1_16x16_scroll_0_factor = 4;
	megasys1_8x8_scroll_1_factor = 1;	megasys1_16x16_scroll_1_factor = 4;
	megasys1_8x8_scroll_2_factor = 1;	megasys1_16x16_scroll_2_factor = 4;

	if (Machine->gamedrv == &driver_soldamj)
	{
		megasys1_8x8_scroll_1_factor = 4;	megasys1_16x16_scroll_1_factor = 4;
	}

	hardware_type_z = 0;
	if (Machine->gamedrv			==	&driver_lomakai ||
		Machine->gamedrv->clone_of	==	&driver_lomakai )
		hardware_type_z = 1;

 	return 0;
}

/***************************************************************************

							Layers declarations:

					* Read and write handlers for the layer
					* Callbacks for the TileMap code

***************************************************************************/

#define TILES_PER_PAGE_X (0x20)
#define TILES_PER_PAGE_Y (0x20)
#define TILES_PER_PAGE (TILES_PER_PAGE_X * TILES_PER_PAGE_Y)

#define MEGASYS1_GET_TILE_INFO(_n_) \
void megasys1_get_scroll_##_n_##_tile_info_8x8(int tile_index) \
{ \
	data16_t code = megasys1_scrollram_##_n_[tile_index]; \
	SET_TILE_INFO( _n_ , (code & 0xfff) * megasys1_8x8_scroll_##_n_##_factor, code >> (16 - megasys1_bits_per_color_code), 0 ) \
} \
\
void megasys1_get_scroll_##_n_##_tile_info_16x16(int tile_index) \
{ \
	data16_t code = megasys1_scrollram_##_n_[tile_index/4]; \
	SET_TILE_INFO( _n_ , (code & 0xfff) * megasys1_16x16_scroll_##_n_##_factor + (tile_index & 3), code >> (16-megasys1_bits_per_color_code), 0 ); \
} \
\
UINT32 megasys1_##_n_##_scan_8x8(UINT32 col,UINT32 row,UINT32 num_cols,UINT32 num_rows) \
{ \
	return 	(col * TILES_PER_PAGE_Y) + \
\
			(row / TILES_PER_PAGE_Y) * TILES_PER_PAGE * megasys1_pages_per_tmap_x[_n_] + \
			(row % TILES_PER_PAGE_Y); \
}\
\
UINT32 megasys1_##_n_##_scan_16x16(UINT32 col,UINT32 row,UINT32 num_cols,UINT32 num_rows) \
{ \
	return	( ((col / 2) * (TILES_PER_PAGE_Y / 2)) + \
\
			  ((row / 2) / (TILES_PER_PAGE_Y / 2)) * (TILES_PER_PAGE / 4) * megasys1_pages_per_tmap_x[_n_] + \
			  ((row / 2) % (TILES_PER_PAGE_Y / 2)) )*4 + (row&1) + (col&1)*2; \
}


#define MEGASYS1_SCROLLRAM_W(_n_) \
WRITE16_HANDLER( megasys1_scrollram_##_n_##_w ) \
{ \
	data16_t old_data = megasys1_scrollram_##_n_[offset]; \
	data16_t new_data = COMBINE_DATA(&megasys1_scrollram_##_n_[offset]); \
	if (old_data != new_data) \
	{ \
		if ( (offset < 0x40000/2) && (megasys1_tmap[_n_]) ) \
		{\
			int tile_index = offset; \
			if (megasys1_scroll_flag[_n_] & 0x10)	/* tiles are 8x8 */ \
			{ \
				tilemap_mark_tile_dirty(megasys1_tmap[_n_], tile_index ); \
			} \
			else \
			{ \
				tilemap_mark_tile_dirty(megasys1_tmap[_n_], tile_index*4 + 0); \
				tilemap_mark_tile_dirty(megasys1_tmap[_n_], tile_index*4 + 1); \
				tilemap_mark_tile_dirty(megasys1_tmap[_n_], tile_index*4 + 2); \
				tilemap_mark_tile_dirty(megasys1_tmap[_n_], tile_index*4 + 3); \
			} \
		}\
	}\
}

MEGASYS1_GET_TILE_INFO(0)
MEGASYS1_GET_TILE_INFO(1)
MEGASYS1_GET_TILE_INFO(2)

MEGASYS1_SCROLLRAM_W(0)
MEGASYS1_SCROLLRAM_W(1)
MEGASYS1_SCROLLRAM_W(2)


/***************************************************************************

							Video registers access

***************************************************************************/


/*		Tilemap Size (PagesX x PagesY)

		Reg. Value			16			8		<- Tile Size

			0				16 x  2		8 x 1
			1	 			 8 x  4		4 x 2
			2	 			 4 x  8		4 x 2
			3	 			 2 x 16		2 x 4
*/

#define MEGASYS1_SCROLL_FLAG_W(_n_) \
void megasys1_scroll_##_n_##_flag_w(int data) \
{ \
	if ((megasys1_scroll_flag[_n_] != data) || (megasys1_tmap[_n_] == 0) ) \
	{ \
		megasys1_scroll_flag[_n_] = data; \
\
		if (megasys1_tmap[_n_]) \
			tilemap_dispose(megasys1_tmap[_n_]); \
\
		/* number of pages when tiles are 16x16 */ \
		megasys1_pages_per_tmap_x[_n_] = 16 / ( 1 << (megasys1_scroll_flag[_n_] & 0x3) ); \
		megasys1_pages_per_tmap_y[_n_] = 32 / megasys1_pages_per_tmap_x[_n_]; \
\
		/* when tiles are 8x8, divide the number of total pages by 4 */ \
		if (megasys1_scroll_flag[_n_] & 0x10) \
		{ \
			if (megasys1_pages_per_tmap_y[_n_] > 4)	{ megasys1_pages_per_tmap_x[_n_] /= 1;	megasys1_pages_per_tmap_y[_n_] /= 4;} \
			else									{ megasys1_pages_per_tmap_x[_n_] /= 2;	megasys1_pages_per_tmap_y[_n_] /= 2;} \
		} \
\
		if ((megasys1_tmap[_n_] = \
				tilemap_create \
					(	(megasys1_scroll_flag[_n_] & 0x10) ? \
							megasys1_get_scroll_##_n_##_tile_info_8x8 : \
							megasys1_get_scroll_##_n_##_tile_info_16x16, \
						(megasys1_scroll_flag[_n_] & 0x10) ? \
							megasys1_##_n_##_scan_8x8 : \
							megasys1_##_n_##_scan_16x16, \
						TILEMAP_TRANSPARENT, \
						8,8, \
						TILES_PER_PAGE_X * megasys1_pages_per_tmap_x[_n_], \
						TILES_PER_PAGE_Y * megasys1_pages_per_tmap_y[_n_]  \
					) \
			)) tilemap_set_transparent_pen(megasys1_tmap[_n_],15); \
	} \
}

MEGASYS1_SCROLL_FLAG_W(0)
MEGASYS1_SCROLL_FLAG_W(1)
MEGASYS1_SCROLL_FLAG_W(2)


/* Used by MS1-A/Z, B */
WRITE16_HANDLER( megasys1_vregs_A_w )
{
	data16_t new_data = COMBINE_DATA(&megasys1_vregs[offset]);

	switch (offset)
	{
		case 0x000/2   :	megasys1_active_layers = new_data;	break;

		case 0x008/2+0 :	megasys1_scrollx[2] = new_data;	break;
		case 0x008/2+1 :	megasys1_scrolly[2] = new_data;	break;
		case 0x008/2+2 :	MEGASYS1_VREG_FLAG(2)		break;

		case 0x200/2+0 :	megasys1_scrollx[0] = new_data;	break;
		case 0x200/2+1 :	megasys1_scrolly[0] = new_data;	break;
		case 0x200/2+2 :	MEGASYS1_VREG_FLAG(0)		break;

		case 0x208/2+0 :	megasys1_scrollx[1] = new_data;	break;
		case 0x208/2+1 :	megasys1_scrolly[1] = new_data;	break;
		case 0x208/2+2 :	MEGASYS1_VREG_FLAG(1)		break;

		case 0x100/2   :	megasys1_sprite_flag = new_data;		break;

		case 0x300/2   :	megasys1_screen_flag = new_data;
							if (new_data & 0x10)
								cpu_set_reset_line(1,ASSERT_LINE);
							else
								cpu_set_reset_line(1,CLEAR_LINE);
							break;

		case 0x308/2   :	soundlatch_word_w(0,new_data,0);
							cpu_set_irq_line(1,4,HOLD_LINE);
							break;

		default		 :	SHOW_WRITE_ERROR("vreg %04X <- %04X",offset*2,data);
	}

}




/* Used by MS1-C only */
READ16_HANDLER( megasys1_vregs_C_r )
{
	switch (offset)
	{
		case 0x8000/2:	return soundlatch2_word_r(0,0);
		default:		return megasys1_vregs[offset];
	}
}

WRITE16_HANDLER( megasys1_vregs_C_w )
{
	data16_t new_data = COMBINE_DATA(&megasys1_vregs[offset]);

	switch (offset)
	{
		case 0x2000/2+0 :	megasys1_scrollx[0] = new_data;	break;
		case 0x2000/2+1 :	megasys1_scrolly[0] = new_data;	break;
		case 0x2000/2+2 :	MEGASYS1_VREG_FLAG(0)		break;

		case 0x2008/2+0 :	megasys1_scrollx[1] = new_data;	break;
		case 0x2008/2+1 :	megasys1_scrolly[1] = new_data;	break;
		case 0x2008/2+2 :	MEGASYS1_VREG_FLAG(1)		break;

		case 0x2100/2+0 :	megasys1_scrollx[2] = new_data;	break;
		case 0x2100/2+1 :	megasys1_scrolly[2] = new_data;	break;
		case 0x2100/2+2 :	MEGASYS1_VREG_FLAG(2)		break;

		case 0x2108/2   :	megasys1_sprite_bank   = new_data;	break;
		case 0x2200/2   :	megasys1_sprite_flag   = new_data;	break;
		case 0x2208/2   :	megasys1_active_layers = new_data;	break;

		case 0x2308/2   :	megasys1_screen_flag = new_data;
							if (new_data & 0x10)
								cpu_set_reset_line(1,ASSERT_LINE);
							else
								cpu_set_reset_line(1,CLEAR_LINE);
							break;

		case 0x8000/2   :	/* Cybattler reads sound latch on irq 2 */
							soundlatch_word_w(0,new_data,0);
							cpu_set_irq_line(1,2,HOLD_LINE);
							break;

		default:		SHOW_WRITE_ERROR("vreg %04X <- %04X",offset*2,data);
	}
}



/* Used by MS1-D only */
WRITE16_HANDLER( megasys1_vregs_D_w )
{
	data16_t new_data = COMBINE_DATA(&megasys1_vregs[offset]);

	switch (offset)
	{
		case 0x2000/2+0 :	megasys1_scrollx[0] = new_data;	break;
		case 0x2000/2+1 :	megasys1_scrolly[0] = new_data;	break;
		case 0x2000/2+2 :	MEGASYS1_VREG_FLAG(0)		break;

		case 0x2008/2+0 :	megasys1_scrollx[1] = new_data;	break;
		case 0x2008/2+1 :	megasys1_scrolly[1] = new_data;	break;
		case 0x2008/2+2 :	MEGASYS1_VREG_FLAG(1)		break;

/*		case 0x2100/2+0 :	megasys1_scrollx[2] = new_data;	break;*/
/*		case 0x2100/2+1 :	megasys1_scrolly[2] = new_data;	break;*/
/*		case 0x2100/2+2 :	MEGASYS1_VREG_FLAG(2)		break;*/

		case 0x2108/2   :	megasys1_sprite_bank	=	new_data;		break;
		case 0x2200/2   :	megasys1_sprite_flag	=	new_data;		break;
		case 0x2208/2   :	megasys1_active_layers	=	new_data;		break;
		case 0x2308/2   :	megasys1_screen_flag	=	new_data;		break;

		default:		SHOW_WRITE_ERROR("vreg %04X <- %04X",offset*2,data);
	}
}



/***************************************************************************

							Sprites Drawing

***************************************************************************/


/*	 Draw sprites in the given bitmap.

 Sprite Data:

	Offset		Data

 	00-07						?
	08 		fed- ---- ---- ----	?
			---c ---- ---- ----	mosaic sol.	(?)
			---- ba98 ---- ----	mosaic		(?)
			---- ---- 7--- ----	y flip
			---- ---- -6-- ----	x flip
			---- ---- --45 ----	?
			---- ---- ---- 3210	color code (bit 3 = priority)
	0A		X position
	0C		Y position
	0E		Code											*/

static void draw_sprites(struct mame_bitmap *bitmap,const struct rectangle *cliprect)
{
	int color,code,sx,sy,flipx,flipy,attr,sprite,offs,color_mask;

/* objram: 0x100*4 entries		spritedata: 0x80 entries */

	/* sprite order is from first in Sprite Data RAM (frontmost) to last */

	if (hardware_type_z == 0)	/* standard sprite hardware */
	{
		color_mask = (megasys1_sprite_flag & 0x100) ? 0x07 : 0x0f;

		for (offs = (0x800-8)/2;offs >= 0;offs -= 8/2)
		{
			for (sprite = 0; sprite < 4 ; sprite ++)
			{
				data16_t *objectdata = &megasys1_buffer2_objectram[offs + (0x800/2) * sprite];
				data16_t *spritedata = &megasys1_buffer2_spriteram16[ (objectdata[ 0 ] & 0x7f) * 0x10/2];

				attr = spritedata[ 8/2 ];
				if (((attr & 0xc0)>>6) != sprite)	continue;	/* flipping*/

				/* apply the position displacements */
				sx = ( spritedata[0x0A/2] + objectdata[0x02/2] ) % 512;
				sy = ( spritedata[0x0C/2] + objectdata[0x04/2] ) % 512;

				if (sx > 256-1) sx -= 512;
				if (sy > 256-1) sy -= 512;

				flipx = attr & 0x40;
				flipy = attr & 0x80;

				if (megasys1_screen_flag & 1)
				{
					flipx = !flipx;		flipy = !flipy;
					sx = 240-sx;		sy = 240-sy;
				}

				/* sprite code is displaced as well */
				code  = spritedata[0x0E/2] + objectdata[0x06/2];
				color = (attr & color_mask);

				pdrawgfx(bitmap,Machine->gfx[3],
						(code & 0xfff ) + ((megasys1_sprite_bank & 1) << 12),
						color,
						flipx, flipy,
						sx, sy,
						cliprect,
						TRANSPARENCY_PEN,15,
						(attr & 0x08) ? 0x0c : 0x0a);
			}	/* sprite */
		}	/* offs */
	}	/* non Z hw */
	else
	{

		/* MS1-Z just draws Sprite Data, and in reverse order */

		for (sprite = 0x80-1;sprite >= 0;sprite--)
		{
			data16_t *spritedata = &spriteram16[ sprite * 0x10/2];

			attr = spritedata[ 8/2 ];

			sx = spritedata[0x0A/2] % 512;
			sy = spritedata[0x0C/2] % 512;

			if (sx > 256-1) sx -= 512;
			if (sy > 256-1) sy -= 512;

			code  = spritedata[0x0E/2];
			color = (attr & 0x0F);

			flipx = attr & 0x40;
			flipy = attr & 0x80;

			if (megasys1_screen_flag & 1)
			{
				flipx = !flipx;		flipy = !flipy;
				sx = 240-sx;		sy = 240-sy;
			}

			pdrawgfx(bitmap,Machine->gfx[2],
					code,
					color,
					flipx, flipy,
					sx, sy,
					cliprect,
					TRANSPARENCY_PEN,15,
					(attr & 0x08) ? 0x0c : 0x0a);
		}	/* sprite */
	}	/* Z hw */

}




/***************************************************************************
						Convert the Priority Prom
***************************************************************************/

struct priority
{
	struct GameDriver *driver;
	int priorities[16];
};

int megasys1_layers_order[16];


/*
	Layers order encoded as an int like: 0x01234, where

	0:	Scroll 0
	1:	Scroll 1
	2:	Scroll 2
	3:	Sprites with color 0-7
		(*every sprite*, if sprite splitting is not active)
	4:	Sprites with color 8-f
	f:	empty placeholder (we can't use 0!)

	and the bottom layer is on the left (e.g. 0).

	The special value 0xfffff means that the order is either unknown
	or no simple stack of layers can account for the values in the prom.
	(the default value, 0x04132, will be used in those cases)

*/


/*
	Convert the 512 bytes in the Priority Prom into 16 ints, encoding
	the layers order for 16 selectable priority schemes.

	INPUT (to the video chip):

		4 pixels: 3 layers(012) + 1 sprite (3)
		(there are low and high priority sprites which
		are split when the "split sprites" bit is set)

	addr =	( (low pri sprite & split sprites ) << 0 ) +
			( (pixel 0 is enabled and opaque )  << 1 ) +
			( (pixel 1 is enabled and opaque )  << 2 ) +
			( (pixel 2 is enabled and opaque )  << 3 ) +
			( (pixel 3 is enabled and opaque )  << 4 ) +
			( (layers_enable bits 11-8  )       << 5 )

	OUTPUT (to video):
		1 pixel, the one from layer: PROM[addr] (layer can be 0-3)

	This scheme can generate a wealth of funky priority schemes
	while we can account for just a simple stack of transparent
	layers like: 01324. That is: bottom layer is 0, then 1, then
	sprites (low priority sprites if sprite splitting is active,
	every sprite if not) then layer 2 and high priority sprites
	(only if sprite splitting is active).

	Hence, during the conversion process we make sure each of the
	16 priority scheme in the prom is a "simple" one like the above
	and log a warning otherwise. The feasibility criterion is such:

	the opaque pens of the top layer must be above any other layer.
	The transparent pens of the top layer must be either totally
	opaque or totally transparent with respects to the other layers:
	when the bit relative to the top layer is not set, the top layer's
	code must be either always absent (transparent case) or always
	present (opaque case) in the prom.

	NOTE: This can't account for orders starting like: 030..
	as found in peekaboo's prom. That's where sprites go below
	the bottom layer's opaque pens, but above its transparent
	pens.
*/

PALETTE_INIT( megasys1 )
{
	int pri_code, offset, i, order;

	/* convert PROM to something we can use */

	for (pri_code = 0; pri_code < 0x10 ; pri_code++)	/* 16 priority codes*/
	{
		int layers_order[2];	/* 2 layers orders (split sprites on/off)*/

		for (offset = 0; offset < 2; offset ++)
		{
			int enable_mask = 0xf;	/* start with every layer enabled*/

			layers_order[offset] = 0xfffff;

			do
			{
				int top = color_prom[pri_code * 0x20 + offset + enable_mask * 2] & 3;	/* this must be the top layer*/
				int top_mask = 1 << top;

				int	result = 0;		/* result of the feasibility check for this layer*/

				for (i = 0; i < 0x10 ; i++)	/* every combination of opaque and transparent pens*/
				{
					int opacity	=	i & enable_mask;	/* only consider active layers*/
					int layer	=	color_prom[pri_code * 0x20 + offset + opacity * 2];

					if (opacity)
					{
						if (opacity & top_mask)
						{
							if (layer != top )	result |= 1; 	/* error: opaque pens aren't always opaque!*/
						}
						else
						{
							if (layer == top)	result |= 2;	/* transparent pen is opaque*/
							else				result |= 4;	/* transparent pen is transparent*/
						}
					}
				}

				/*	note: 3210 means that layer 0 is the bottom layer
					(the order is reversed in the hand-crafted data) */

				layers_order[offset] = ( (layers_order[offset] << 4) | top ) & 0xfffff;
				enable_mask &= ~top_mask;

				if (result & 1)
				{
					log_cb(RETRO_LOG_DEBUG, LOGPRE "WARNING, pri $%X split %d - layer %d's opaque pens not totally opaque\n",pri_code,offset,top);

					layers_order[offset] = 0xfffff;
					break;
				}

				if  ((result & 6) == 6)
				{
					log_cb(RETRO_LOG_DEBUG, LOGPRE "WARNING, pri $%X split %d - layer %d's transparent pens aren't always transparent nor always opaque\n",pri_code,offset,top);

					layers_order[offset] = 0xfffff;
					break;
				}

				if (result == 2)	enable_mask = 0; /* totally opaque top layer*/

			}	while (enable_mask);

        }	/* offset*/

		/* merge the two layers orders */

		order = 0xfffff;

		for (i = 5; i > 0 ; )	/* 5 layers to write*/
		{
			int layer;
			int layer0 = layers_order[0] & 0x0f;
			int layer1 = layers_order[1] & 0x0f;

			if (layer0 != 3)	/* 0,1,2 or f*/
			{
				if (layer1 == 3)
				{
					layer = 4;
					layers_order[0] <<= 4;	/* layer1 won't change next loop*/
				}
				else
				{
					layer = layer0;
					if (layer0 != layer1)
					{
						log_cb(RETRO_LOG_DEBUG, LOGPRE "WARNING, pri $%X - 'sprite splitting' does not simply split sprites\n",pri_code);

						order = 0xfffff;
						break;
					}

				}
			}
			else	/* layer0 = 3;*/
			{
				if (layer1 == 3)
				{
					layer = 0x43;			/* 4 must always be present*/
					order <<= 4;
					i --;					/* 2 layers written at once*/
				}
				else
				{
					layer = 3;
					layers_order[1] <<= 4;	/* layer1 won't change next loop*/
				}
			}

			/* reverse the order now */
			order = (order << 4 ) | layer;

			i --;		/* layer written*/

			layers_order[0] >>= 4;
			layers_order[1] >>= 4;

		}	/* merging*/

		megasys1_layers_order[pri_code] = order & 0xfffff;	/* at last!*/

	}	/* pri_code*/



#if 0
	/* log the priority schemes */
	for (i = 0; i < 16; i++)
		log_cb(RETRO_LOG_DEBUG, LOGPRE "PROM %X] %05x\n", i, megasys1_layers_order[i]);
#endif


}





/***************************************************************************
			  Draw the game screen in the given mame_bitmap.
***************************************************************************/


VIDEO_UPDATE( megasys1 )
{
	int i,flag,pri,primask;
	int active_layers;

	if (hardware_type_z)
	{
		/* no layer 2 and fixed layers order? */
		active_layers = 0x000b;
		pri = 0x0314f;
	}
	else
	{
		int reallyactive = 0;

		/* get layers order */
		pri = megasys1_layers_order[(megasys1_active_layers & 0x0f0f) >> 8];

#ifdef MAME_DEBUG
		if (pri == 0xfffff)
		{
			usrintf_showmessage("Pri: %04X - Flag: %04X", megasys1_active_layers, megasys1_sprite_flag);
		}
#endif

		if (pri == 0xfffff) pri = 0x04132;

		/* see what layers are really active (layers 4 & f will do no harm) */
		for (i = 0;i < 5;i++)
			reallyactive |= 1 << ((pri >> (4*i)) & 0x0f);

		active_layers = megasys1_active_layers & reallyactive;
		active_layers |= 1 << ((pri & 0xf0000) >> 16);	/* bottom layer can't be disabled*/
	}

	tilemap_set_flip( ALL_TILEMAPS, (megasys1_screen_flag & 1) ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0 );

	for (i = 0;i < 3;i++)
	{
		if (megasys1_tmap[i])
		{
			tilemap_set_enable(megasys1_tmap[i],active_layers & (1 << i));

			tilemap_set_scrollx(megasys1_tmap[i],0,megasys1_scrollx[i]);
			tilemap_set_scrolly(megasys1_tmap[i],0,megasys1_scrolly[i]);
		}
	}

	fillbitmap(priority_bitmap,0,cliprect);

	flag = TILEMAP_IGNORE_TRANSPARENCY;
	primask = 0;

	for (i = 0;i < 5;i++)
	{
		int layer = (pri & 0xf0000) >> 16;
		pri <<= 4;

		switch (layer)
		{
			case 0:
			case 1:
			case 2:
				if ( (megasys1_tmap[layer]) && (active_layers & (1 << layer) ) )
				{
					tilemap_draw(bitmap,cliprect,megasys1_tmap[layer],flag,primask);
					flag = 0;
				}
				break;
			case 3:
			case 4:
				if (flag != 0)
				{
					flag = 0;
					fillbitmap(bitmap,Machine->pens[0],cliprect);
				}

				if (megasys1_sprite_flag & 0x100)	/* sprites are split */
				{
					/* following tilemaps will obscure this sprites layer */
					primask |= 1 << (layer-3);
				}
				else
					/* following tilemaps will obscure all sprites */
					if (layer == 3)	primask |= 3;

				break;
		}
	}

	if (active_layers & 0x08)
		draw_sprites(bitmap,cliprect);
}

VIDEO_EOF( megasys1 )
{
	/* Sprite are TWO frames ahead, like NMK16 HW. */
/* megasys1_objectram */
	memcpy(megasys1_buffer2_objectram,megasys1_buffer_objectram, 0x2000);
	memcpy(megasys1_buffer_objectram, megasys1_objectram, 0x2000);
/* spriteram16 */
	memcpy(megasys1_buffer2_spriteram16, megasys1_buffer_spriteram16, 0x2000);
	memcpy(megasys1_buffer_spriteram16, spriteram16, 0x2000);

}

