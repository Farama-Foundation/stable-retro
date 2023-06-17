/***************************************************************************
  vidhrdw.c

  Functions to emulate the video hardware of these machines.
  Video is 30x40 tiles. (200x320 for Twin Cobra/Flying shark)
  Video is 40x30 tiles. (320x200 for Wardner)

  Video has 3 scrolling tile layers (Background, Foreground and Text) and
  Sprites that have 4 (5?) priorities. Lowest priority is "Off".
  Wardner has an unusual sprite priority in the shop scenes, whereby a
  middle level priority Sprite appears over a high priority Sprite ?

***************************************************************************/

#include "driver.h"
#include "state.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/crtc6845.h"


static void twincobr_restore_screen(void);


static data16_t *twincobr_bgvideoram16;
static data16_t *twincobr_fgvideoram16;

int wardner_sprite_hack;	/* Required for weird sprite priority in wardner  */
							/* when hero is in shop. Hero should cover shop owner */

extern int toaplan_main_cpu;	/* Main CPU type.  0 = 68000, 1 = Z80 */

static size_t twincobr_bgvideoram_size,twincobr_fgvideoram_size;
static int txscrollx;
static int txscrolly;
static int fgscrollx;
static int fgscrolly;
static int bgscrollx;
static int bgscrolly;
int twincobr_fg_rom_bank;
int twincobr_bg_ram_bank;
int twincobr_display_on;
int twincobr_flip_screen;
int twincobr_flip_x_base;
int twincobr_flip_y_base;

static int txoffs = 0;
static int fgoffs = 0;
static int bgoffs = 0;
static int scroll_x = 0;
static int scroll_y = 0;

static int vidbaseaddr;
static int scroll_realign_x;


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

VIDEO_START( toaplan0 )
{
	/* the video RAM is accessed via ports, it's not memory mapped */
	videoram_size = 0x800;
	twincobr_bgvideoram_size = 0x2000;	/* banked two times 0x1000 */
	twincobr_fgvideoram_size = 0x1000;

	if ((videoram16 = auto_malloc(videoram_size*2)) == 0)
		return 1;
	memset(videoram16,0,videoram_size*2);

	if ((twincobr_fgvideoram16 = auto_malloc(twincobr_fgvideoram_size*2)) == 0)
		return 1;
	memset(twincobr_fgvideoram16,0,twincobr_fgvideoram_size*2);

	if ((twincobr_bgvideoram16 = auto_malloc(twincobr_bgvideoram_size*2)) == 0)
		return 1;
	memset(twincobr_bgvideoram16,0,twincobr_bgvideoram_size*2);

	if ((dirtybuffer = auto_malloc(twincobr_bgvideoram_size*2)) == 0)
		return 1;
	memset(dirtybuffer,1,twincobr_bgvideoram_size*2);

	if ((tmpbitmap = auto_bitmap_alloc(Machine->drv->screen_width,2*Machine->drv->screen_height)) == 0)
		return 1;

	state_save_register_UINT16("toaplan0", 0, "Text_Field", videoram16, videoram_size);
	state_save_register_UINT16("toaplan0", 0, "FG_PlayField", twincobr_fgvideoram16, twincobr_fgvideoram_size);
	state_save_register_UINT16("toaplan0", 0, "BG_PlayField", twincobr_bgvideoram16, twincobr_bgvideoram_size);
	state_save_register_int("toaplan0", 0, "txoffs", &txoffs);
	state_save_register_int("toaplan0", 0, "fgoffs", &fgoffs);
	state_save_register_int("toaplan0", 0, "bgoffs", &bgoffs);
	state_save_register_int("toaplan0", 0, "scroll_x", &scroll_x);
	state_save_register_int("toaplan0", 0, "scroll_y", &scroll_y);
	state_save_register_int("toaplan0", 0, "txscrollx", &txscrollx);
	state_save_register_int("toaplan0", 0, "fgscrollx", &fgscrollx);
	state_save_register_int("toaplan0", 0, "bgscrollx", &bgscrollx);
	state_save_register_int("toaplan0", 0, "txscrolly", &txscrolly);
	state_save_register_int("toaplan0", 0, "fgscrolly", &fgscrolly);
	state_save_register_int("toaplan0", 0, "bgscrolly", &bgscrolly);
	state_save_register_int("toaplan0", 0, "Display_On", &twincobr_display_on);
	state_save_register_int("toaplan0", 0, "twincobr_fg_rom_bank", &twincobr_fg_rom_bank);
	state_save_register_int("toaplan0", 0, "twincobr_bg_ram_bank", &twincobr_bg_ram_bank);
	state_save_register_int("toaplan0", 0, "twincobr_flip_screen", &twincobr_flip_screen);
	state_save_register_int("toaplan0", 0, "twincobr_flip_x_base", &twincobr_flip_x_base);
	state_save_register_int("toaplan0", 0, "twincobr_flip_y_base", &twincobr_flip_y_base);
	state_save_register_int("wardner", 0, "wardner_sprite_hack", &wardner_sprite_hack);
	state_save_register_func_postload(twincobr_restore_screen);	/* Restore the background */

	return 0;
}


static void twincobr_restore_screen(void)
{
	/* Mark the background layer cache as dirty, to force a redraw */
	memset(dirtybuffer,1,twincobr_bgvideoram_size*2);
}




WRITE16_HANDLER( twincobr_crtc_reg_sel_w )
{
	crtc6845_address_w(offset, data);
}

WRITE16_HANDLER( twincobr_crtc_data_w )
{
	crtc6845_register_w(offset, data);
}

WRITE16_HANDLER( twincobr_txoffs_w )
{
	COMBINE_DATA(&txoffs);
	txoffs %= videoram_size;
}
READ16_HANDLER( twincobr_txram_r )
{
	return videoram16[txoffs];
}
WRITE16_HANDLER( twincobr_txram_w )
{
	COMBINE_DATA(&videoram16[txoffs]);
}

WRITE16_HANDLER( twincobr_bgoffs_w )
{
	COMBINE_DATA(&bgoffs);
	bgoffs %= (twincobr_bgvideoram_size >> 1);
}
READ16_HANDLER( twincobr_bgram_r )
{
	return twincobr_bgvideoram16[bgoffs+twincobr_bg_ram_bank];
}
WRITE16_HANDLER( twincobr_bgram_w )
{
	COMBINE_DATA(&twincobr_bgvideoram16[bgoffs+twincobr_bg_ram_bank]);
	dirtybuffer[bgoffs] = 1;
}

WRITE16_HANDLER( twincobr_fgoffs_w )
{
	COMBINE_DATA(&fgoffs);
	fgoffs %= twincobr_fgvideoram_size;
}
READ16_HANDLER( twincobr_fgram_r )
{
	return twincobr_fgvideoram16[fgoffs];
}
WRITE16_HANDLER( twincobr_fgram_w )
{
	COMBINE_DATA(&twincobr_fgvideoram16[fgoffs]);
}


WRITE16_HANDLER( twincobr_txscroll_w )
{
	if (offset == 0) COMBINE_DATA(&txscrollx);
	else COMBINE_DATA(&txscrolly);
}

WRITE16_HANDLER( twincobr_bgscroll_w )
{
	if (offset == 0) COMBINE_DATA(&bgscrollx);
	else COMBINE_DATA(&bgscrolly);
}

WRITE16_HANDLER( twincobr_fgscroll_w )
{
	if (offset == 0) COMBINE_DATA(&fgscrollx);
	else COMBINE_DATA(&fgscrolly);
}

WRITE16_HANDLER( twincobr_exscroll_w )	/* Extra unused video layer */
{
	if (offset == 0) log_cb(RETRO_LOG_DEBUG, LOGPRE "PC - write %04x to unknown video scroll Y register\n",data);
	else log_cb(RETRO_LOG_DEBUG, LOGPRE "PC - write %04x to unknown video scroll X register\n",data);
}

/******************** Wardner interface to this hardware ********************/
WRITE_HANDLER( wardner_txlayer_w )
{
	int shift = 8 * (offset & 1);
	twincobr_txoffs_w(offset / 2, data << shift, 0xff00 >> shift);
}
WRITE_HANDLER( wardner_bglayer_w )
{
	int shift = 8 * (offset & 1);
	twincobr_bgoffs_w(offset / 2, data << shift, 0xff00 >> shift);
}
WRITE_HANDLER( wardner_fglayer_w )
{
	int shift = 8 * (offset & 1);
	twincobr_fgoffs_w(offset / 2, data << shift, 0xff00 >> shift);
}

WRITE_HANDLER( wardner_txscroll_w )
{
	int shift = 8 * (offset & 1);
	twincobr_txscroll_w(offset / 2, data << shift, 0xff00 >> shift);
}
WRITE_HANDLER( wardner_bgscroll_w )
{
	int shift = 8 * (offset & 1);
	twincobr_bgscroll_w(offset / 2, data << shift, 0xff00 >> shift);
}
WRITE_HANDLER( wardner_fgscroll_w )
{
	int shift = 8 * (offset & 1);
	twincobr_fgscroll_w(offset / 2, data << shift, 0xff00 >> shift);
}

WRITE_HANDLER( wardner_exscroll_w )	/* Extra unused video layer */
{
	if (offset == 0) log_cb(RETRO_LOG_DEBUG, LOGPRE "PC - write %04x to unknown video scroll Y register\n",data);
	else log_cb(RETRO_LOG_DEBUG, LOGPRE "PC - write %04x to unknown video scroll X register\n",data);
}

READ_HANDLER( wardner_videoram_r )
{
	int shift = 8 * (offset & 1);
	switch (offset/2) {
		case 0: return twincobr_txram_r(0,0) >> shift; break;
		case 1: return twincobr_bgram_r(0,0) >> shift; break;
		case 2: return twincobr_fgram_r(0,0) >> shift; break;
	}
	return 0;
}

WRITE_HANDLER( wardner_videoram_w )
{
	int shift = 8 * (offset & 1);
	switch (offset/2) {
		case 0: twincobr_txram_w(0,data << shift, 0xff00 >> shift); break;
		case 1: twincobr_bgram_w(0,data << shift, 0xff00 >> shift); break;
		case 2: twincobr_fgram_w(0,data << shift, 0xff00 >> shift); break;
	}
}

static void twincobr_draw_sprites(struct mame_bitmap *bitmap, int priority)
{
	int offs;

	for (offs = 0;offs < spriteram_size/2;offs += 4)
	{
		int attribute,sx,sy,flipx,flipy;
		int sprite, color;

		attribute = buffered_spriteram16[offs + 1];
		if ((attribute & 0x0c00) == priority) {	/* low priority */
			sy = buffered_spriteram16[offs + 3] >> 7;
			if (sy != 0x0100) {		/* sx = 0x01a0 or 0x0040*/
				sprite = buffered_spriteram16[offs] & 0x7ff;
				color  = attribute & 0x3f;
				sx = buffered_spriteram16[offs + 2] >> 7;
				flipx = attribute & 0x100;
				if (flipx) sx -= 14;		/* should really be 15 */
				flipy = attribute & 0x200;
				drawgfx(bitmap,Machine->gfx[3],
					sprite,
					color,
					flipx,flipy,
					sx-32,sy-16,
					&Machine->visible_area,TRANSPARENCY_PEN,0);
			}
		}
	}
}



VIDEO_UPDATE( toaplan0 )
{
	static int offs,code,tile,color;


	if (twincobr_display_on != 1)
	{
		fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area);
		return;
	}


	/* draw the background */
	for (offs = twincobr_bgvideoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy;

			dirtybuffer[offs] = 0;

			sx = offs % 64;
			sy = offs / 64;

			code = twincobr_bgvideoram16[offs+twincobr_bg_ram_bank];
			tile  = (code & 0x0fff);
			color = (code & 0xf000) >> 12;
			if (twincobr_flip_screen) { sx=63-sx; sy=63-sy; }
			drawgfx(tmpbitmap,Machine->gfx[2],
				tile,
				color,
				twincobr_flip_screen,twincobr_flip_screen,
				8*sx,8*sy,
				0,TRANSPARENCY_NONE,0);
		}
	}

	/* copy the background graphics */
	{
		if (twincobr_flip_screen) {
			scroll_x = (twincobr_flip_x_base + bgscrollx + 0x141) & 0x1ff;
			scroll_y = (twincobr_flip_y_base + bgscrolly + 0xf1) & 0x1ff;
		}
		else {
			scroll_x = (0x1c9 - bgscrollx) & 0x1ff;
			scroll_y = (- 0x1e - bgscrolly) & 0x1ff;
		}
		copyscrollbitmap(bitmap,tmpbitmap,1,&scroll_x,1,&scroll_y,&Machine->visible_area,TRANSPARENCY_NONE,0);
	}


	/* draw the sprites in low priority (Twin Cobra tanks under roofs) */
	twincobr_draw_sprites (bitmap, 0x0400);

	/* draw the foreground */
	scroll_x = (twincobr_flip_x_base + fgscrollx) & 0x01ff;
	scroll_y = (twincobr_flip_y_base + fgscrolly) & 0x01ff;
	vidbaseaddr = ((scroll_y>>3)*64) + (scroll_x>>3);
	scroll_realign_x = scroll_x >> 3;		/* realign video ram pointer */
	for (offs = (31*41)-1; offs >= 0; offs-- )
	{
		int xpos,ypos;
		unsigned char sx,sy;
		unsigned short int vidramaddr = 0;

		sx = offs % 41;
		sy = offs / 41;

		vidramaddr = vidbaseaddr + (sy*64) + sx;
		if ((scroll_realign_x + sx) > 63) vidramaddr -= 64;

		code  = twincobr_fgvideoram16[vidramaddr & 0xfff];
		tile  = (code & 0x0fff) | twincobr_fg_rom_bank;
		color = (code & 0xf000) >> 12;
		if (twincobr_flip_screen) { sx=40-sx; sy=30-sy; xpos=(sx*8) - (7-(scroll_x&7)); ypos=(sy*8) - (7-(scroll_y&7)); }
		else { xpos=(sx*8) - (scroll_x&7); ypos=(sy*8) - (scroll_y&7); }
		drawgfx(bitmap,Machine->gfx[1],
			tile,
			color,
			twincobr_flip_screen,twincobr_flip_screen,
			xpos,ypos,
			&Machine->visible_area,TRANSPARENCY_PEN,0);
	}

/*********  Begin ugly sprite hack for Wardner when hero is in shop *********/
	if ((wardner_sprite_hack) && (fgscrollx != bgscrollx)) {	/* Wardner ? */
		if ((fgscrollx==0x1c9) || (twincobr_flip_screen && (fgscrollx==0x17a))) {	/* in the shop ? */
			int wardner_hack = buffered_spriteram16[0x0b04/2];
		/* sprite position 0x6300 to 0x8700 -- hero on shop keeper (normal) */
		/* sprite position 0x3900 to 0x5e00 -- hero on shop keeper (flip) */
			if ((wardner_hack > 0x3900) && (wardner_hack < 0x8700)) {	/* hero at shop keeper ? */
				wardner_hack = buffered_spriteram16[0x0b02/2];
				wardner_hack |= 0x0400;			/* make hero top priority */
				buffered_spriteram16[0x0b02/2] = wardner_hack;
				wardner_hack = buffered_spriteram16[0x0b0a/2];
				wardner_hack |= 0x0400;
				buffered_spriteram16[0x0b0a/2] = wardner_hack;
				wardner_hack = buffered_spriteram16[0x0b12/2];
				wardner_hack |= 0x0400;
				buffered_spriteram16[0x0b12/2] = wardner_hack;
				wardner_hack = buffered_spriteram16[0x0b1a/2];
				wardner_hack |= 0x0400;
				buffered_spriteram16[0x0b1a/2] = wardner_hack;
			}
		}
	}
/**********  End ugly sprite hack for Wardner when hero is in shop **********/

	/* draw the sprites in normal priority */
	twincobr_draw_sprites (bitmap, 0x0800);

	/* draw the top layer */
	scroll_x = (twincobr_flip_x_base + txscrollx) & 0x01ff;
	scroll_y = (twincobr_flip_y_base + txscrolly) & 0x00ff;
	vidbaseaddr = ((scroll_y>>3)*64) + (scroll_x>>3);
	scroll_realign_x = scroll_x >> 3;
	for (offs = (31*41)-1; offs >= 0; offs-- )
	{
		int xpos,ypos;
		unsigned char sx,sy;
		unsigned short int vidramaddr = 0;

		sx = offs % 41;
		sy = offs / 41;

		vidramaddr = vidbaseaddr + (sy*64) + sx;
		if ((scroll_realign_x + sx) > 63) vidramaddr -= 64;

		code  = videoram16[vidramaddr & 0x7ff];
		tile  = (code & 0x07ff);
		color = (code & 0xf800) >> 11;
		if (twincobr_flip_screen) { sx=40-sx; sy=30-sy; xpos=(sx*8) - (7-(scroll_x&7)); ypos=(sy*8) - (7-(scroll_y&7)); }
		else { xpos=(sx*8) - (scroll_x&7); ypos=(sy*8) - (scroll_y&7); }
		drawgfx(bitmap,Machine->gfx[0],
			tile,
			color,
			twincobr_flip_screen,twincobr_flip_screen,
			xpos,ypos,
			&Machine->visible_area,TRANSPARENCY_PEN,0);
	}

	/* draw the sprites in high priority */
	twincobr_draw_sprites (bitmap, 0x0c00);
}

VIDEO_EOF( toaplan0 )
{
	/*  Spriteram is always 1 frame ahead, suggesting spriteram buffering.
		There are no CPU output registers that control this so we
		assume it happens automatically every frame, at the end of vblank */
	buffer_spriteram16_w(0,0,0);
}
