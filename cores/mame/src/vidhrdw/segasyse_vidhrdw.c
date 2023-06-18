/*******************************************************************************
 Sega System E (834-5803) Driver (vidhrdw/segasyse.c)
********************************************************************************
 driver by David Haywood

 see (drivers/segasyse.c) for additional notes

 todo:

 clean up megatech code, covert to rgb_direct? might be needed for megatech
 some megatech games are also sms based so we'll need a way of supporting that
 for now support has been quickly added while the menu system is worked out
 megaplay has an sms vdp too

*******************************************************************************/

#include "driver.h"
#include "state.h"

/*-- Variables --*/

#define CHIPS 2							/* There are 2 VDP Chips */

UINT8  segae_vdp_cmdpart[CHIPS];		/* VDP Command Part Counter */
UINT16 segae_vdp_command[CHIPS];		/* VDP Command Word */

UINT8  segae_vdp_accessmode[CHIPS];		/* VDP Access Mode (VRAM, CRAM)	*/
UINT16 segae_vdp_accessaddr[CHIPS];		/* VDP Access Address */
UINT8  segae_vdp_readbuffer[CHIPS];		/* VDP Read Buffer */

UINT8 *segae_vdp_vram[CHIPS];			/* Pointer to VRAM */
UINT8 *segae_vdp_cram[CHIPS];			/* Pointer to the VDP's CRAM */
UINT8 *segae_vdp_regs[CHIPS];			/* Pointer to the VDP's Registers */

UINT8 segae_vdp_vrambank[CHIPS];		/* Current VRAM Bank number (from writes to Port 0xf7) */

UINT8 *cache_bitmap;					/* 8bpp bitmap with raw pen values */

static int segasyse_palettebase; /* needed for megatech for now..*/

/*- in (drivers/segasyse.c) -*/

extern UINT8 vintpending;
extern UINT8 hintpending;

/*-- Prototypes --*/

int	segae_vdp_start( UINT8 chip );
void segae_vdp_stop( UINT8 chip );

void segae_vdp_processcmd ( UINT8 chip, UINT16 cmd );
void segae_vdp_setregister ( UINT8 chip, UINT16 cmd );

void segae_drawtilesline(UINT8 *dest, int line, UINT8 chip, UINT8 pri);
void segae_drawspriteline(UINT8 *dest, UINT8 chip, UINT8 line);
void segae_drawscanline(int line, int chips, int blank);

static void segae_draw8pix_solid16(UINT8 *dest, UINT8 chip, UINT16 tile, UINT8 line, UINT8 flipx, UINT8 col);
static void segae_draw8pix(UINT8 *dest, UINT8 chip, UINT16 tile, UINT8 line, UINT8 flipx, UINT8 col);
static void segae_draw8pixsprite(UINT8 *dest, UINT8 chip, UINT16 tile, UINT8 line);

/*******************************************************************************
 vhstart, vhstop and vhrefresh functions
*******************************************************************************/

VIDEO_START( segae )
{
	UINT8 temp;

	segasyse_palettebase = 0;

	for (temp=0;temp<CHIPS;temp++)
		if (segae_vdp_start(temp)) return 1;

	cache_bitmap = auto_malloc( (16+256+16) * 192); /* 16 pixels either side to simplify drawing */

	if (!cache_bitmap) return 1;

	return 0;
}

VIDEO_UPDATE( segae )
{
	int i;

	/*- Draw from cache_bitmap to screen -*/

	for (i = 0;i < 192;i++)
		draw_scanline8(bitmap,0,i,256,&cache_bitmap[i * (16+256+16) +16],&Machine->pens[segasyse_palettebase],15);
}

VIDEO_UPDATE( megrescu )
{
	int i;

	/*- Draw from cache_bitmap to screen -*/

	for (i = 0;i < 192;i++)
		draw_scanline8(bitmap,0,i,256,&cache_bitmap[i * (16+256+16) +16],&Machine->pens[segasyse_palettebase],-1);
}

/* these are used by megatech */

/* starts vdp for bios screen only */
int start_megatech_video_normal(void)
{
	segasyse_palettebase = 0x800;

	if (segae_vdp_start(0)) return 1;

	cache_bitmap = auto_malloc( (16+256+16) * 192); /* 16 pixels either side to simplify drawing */

	if (!cache_bitmap) return 1;

	return 0;
}

void update_megatech_video_normal(struct mame_bitmap *bitmap, const struct rectangle *cliprect )
{
	int i;

	/*- Draw from cache_bitmap to screen -*/

	for (i = 0; i < 192;i++)
		segae_drawscanline(i,0,0);

	for (i = 0;i < 192;i++)
		draw_scanline8(bitmap,32,i,256,&cache_bitmap[i * (16+256+16) +16],&Machine->pens[segasyse_palettebase],-1);

}

void update_megaplay_video_normal(struct mame_bitmap *bitmap, const struct rectangle *cliprect )
{
	int i;

	/*- Draw from cache_bitmap to screen -*/

	for (i = 0; i < 192;i++)
		segae_drawscanline(i,0,0);

	for (i = 0;i < 192;i++)
		draw_scanline8(bitmap,32,i+16,256,&cache_bitmap[i * (16+256+16) +24],&Machine->pens[segasyse_palettebase],0);

}

/*******************************************************************************
 VDP Start / Stop Functions
********************************************************************************
 note: we really should check after each allocation to make sure it was
	   successful then if one allocation fails we can free up the previous ones
*******************************************************************************/

int	segae_vdp_start( UINT8 chip )
{
	UINT8 temp;

	/*- VRAM -*/

	segae_vdp_vram[chip] = auto_malloc(0x8000); /* 32kb (2 banks) */
	segae_vdp_vrambank[chip] = 0;

	/*- CRAM -*/

	segae_vdp_cram[chip] = auto_malloc(0x20);

	/*- VDP Registers -*/

	segae_vdp_regs[chip] = auto_malloc(0x20);

	/*- Check Allocation was Successful -*/

	if (!segae_vdp_vram[chip] || !segae_vdp_cram[chip] || !segae_vdp_regs[chip])
		return 1;

	/*- Clear Memory -*/

	memset(segae_vdp_vram[chip], 0, 0x8000);
	memset(segae_vdp_cram[chip], 0, 0x20);
	memset(segae_vdp_regs[chip], 0, 0x20);

	/*- Set Up Some Default Values */

	segae_vdp_accessaddr[chip] = 0;
	segae_vdp_accessmode[chip] = 0;
	segae_vdp_cmdpart[chip] = 0;
	segae_vdp_command[chip] = 0;

	/*- Black the Palette -*/

	for (temp=0;temp<32;temp++)
		palette_set_color(temp + 32*chip+segasyse_palettebase, 0, 0, 0);

	/* Save State Stuff (based on vidhrdw/taitoic.c) */

	{
		char buf[20];	/* we need different labels for every item of save data */
			sprintf(buf,"SEGASYSE-VDP-%01x",chip);	/* so we add chip # as a suffix */
			state_save_register_UINT8 ( buf, 0, "Video RAM",		segae_vdp_vram[chip], 0x8000);
			state_save_register_UINT8 ( buf, 0, "Colour RAM",		segae_vdp_cram[chip], 0x20);
			state_save_register_UINT8 ( buf, 0, "Registers",		segae_vdp_regs[chip], 0x20);
			state_save_register_UINT8 ( buf, 0, "Command Part",		&segae_vdp_cmdpart[chip], 1);
			state_save_register_UINT16( buf, 0, "Command Word",		&segae_vdp_command[chip], 1);
			state_save_register_UINT8 ( buf, 0, "Access Mode",		&segae_vdp_accessmode[chip], 1);
			state_save_register_UINT16( buf, 0, "Access Address",	&segae_vdp_accessaddr[chip], 1);
			state_save_register_UINT8 ( buf, 0, "VRAM Bank",		&segae_vdp_vrambank[chip], 1);
	}


	return 0;
}

/*******************************************************************************
 Core VDP Functions
*******************************************************************************/

/*-- Reads --*/

/***************************************
 segae_vdp_ctrl_r ( UINT8 chip )
****************************************
 reading the vdp control port will
 return the following

 bit:
  7	- vert int pending
  6	- line int pending
  5	- sprite collision (non 0 pixels) *not currently emulated (not needed by these games)*
  4 - always 0
  3 - always 0
  2 - always 0
  1 - always 0
  0 - always 0

  bits 5,6,7 are cleared after a read
***************************************/

unsigned char segae_vdp_ctrl_r ( UINT8 chip )
{
	UINT8 temp;

	if(strcmp(Machine->gamedrv->name, "opaopan") == 0)
	{
     if (chip == 0) return 0; /* slave vdp (chip==0) doesn't get to clear hint/vint status. dink */
	} 

	temp = 0;

	temp |= (vintpending << 7);
	temp |= (hintpending << 6);

	cpu_set_irq_line(0, 0, CLEAR_LINE);

	hintpending = vintpending = 0;

	return temp;
}

unsigned char segae_vdp_data_r ( UINT8 chip )
{
	UINT8 temp;

	segae_vdp_cmdpart[chip] = 0;

	temp = segae_vdp_readbuffer[chip];

	if (segae_vdp_accessmode[chip]==0x03) { /* CRAM Access */
		/* error CRAM can't be read!! */
	} else { /* VRAM */
		segae_vdp_readbuffer[chip] = segae_vdp_vram[chip][ segae_vdp_vrambank[chip]*0x4000 + segae_vdp_accessaddr[chip] ];
		segae_vdp_accessaddr[chip] += 1;
		segae_vdp_accessaddr[chip] &= 0x3fff;
	}
	return temp;
}

/*-- Writes --*/

void segae_vdp_ctrl_w ( UINT8 chip, UINT8 data )
{
	if (!segae_vdp_cmdpart[chip]) {
		segae_vdp_cmdpart[chip] = 1;
		segae_vdp_command[chip] = data;
	} else {
		segae_vdp_cmdpart[chip] = 0;
		segae_vdp_command[chip] |= (data << 8);
		segae_vdp_processcmd (chip, segae_vdp_command[chip]);
	}
}

void segae_vdp_data_w ( UINT8 chip, UINT8 data )
{
	segae_vdp_cmdpart[chip] = 0;

	if (segae_vdp_accessmode[chip]==0x03) { /* CRAM Access */
		UINT8 r,g,b, temp;

		temp = segae_vdp_cram[chip][segae_vdp_accessaddr[chip]];

		segae_vdp_cram[chip][segae_vdp_accessaddr[chip]] = data;

		if (temp != data) {
			r = (segae_vdp_cram[chip][segae_vdp_accessaddr[chip]] & 0x03) << 6;
			g = (segae_vdp_cram[chip][segae_vdp_accessaddr[chip]] & 0x0c) << 4;
			b = (segae_vdp_cram[chip][segae_vdp_accessaddr[chip]] & 0x30) << 2;

			palette_set_color(segae_vdp_accessaddr[chip] + 32*chip+segasyse_palettebase, r, g, b);
		}

		segae_vdp_accessaddr[chip] += 1;
		segae_vdp_accessaddr[chip] &= 0x1f;
	} else if (segae_vdp_accessmode[chip]==0x01) { /* VRAM Accesses */
		segae_vdp_vram[chip][ segae_vdp_vrambank[chip]*0x4000 + segae_vdp_accessaddr[chip] ] = data;
		segae_vdp_accessaddr[chip] += 1;
		segae_vdp_accessaddr[chip] &= 0x3fff;
	}
}

/*-- Associated Functions --*/

/***************************************
 segae_vdp_processcmd
****************************************

general command format

 M M A A A A A A A A A A A A A A  M=Mode, A=Address

 the command will be one of 3 things according to the upper
 4 bits

 0 0 - - - - - - - - - - - - - -  VRAM Acess Mode (Special Read)

 0 1 - - - - - - - - - - - - - -  VRAM Acesss Mode

 1 0 0 0 - - - - - - - - - - - -  VDP Register Set (current mode & address _not_ changed)

 1 0 x x - - - - - - - - - - - -  VRAM Access Mode (0x1000 - 0x3FFF only, x x is anything but 0 0)

 1 1 - - - - - - - - - - - - - -  CRAM Access Mode

***************************************/

void segae_vdp_processcmd ( UINT8 chip, UINT16 cmd )
{
	if ( (cmd & 0xf000) == 0x8000 ) { /*  1 0 0 0 - - - - - - - - - - - -  VDP Register Set */
		segae_vdp_setregister (chip, cmd);
	} else { /* Anything Else */
		segae_vdp_accessmode[chip] = (cmd & 0xc000) >> 14;
		segae_vdp_accessaddr[chip] = (cmd & 0x3fff);

		if ((segae_vdp_accessmode[chip]==0x03) && (segae_vdp_accessaddr[chip] > 0x1f) ) { /* Check Address is valid for CRAM */
			/* Illegal, CRAM isn't this large! */
			segae_vdp_accessaddr[chip] &= 0x1f;
		}

		if (segae_vdp_accessmode[chip] == 0x00) { /*  0 0 - - - - - - - - - - - - - -  VRAM Acess Mode (Special Read) */
			segae_vdp_readbuffer[chip] = segae_vdp_vram[chip][ segae_vdp_vrambank[chip]*0x4000 + segae_vdp_accessaddr[chip] ];
			segae_vdp_accessaddr[chip] += 1;
			segae_vdp_accessaddr[chip] &= 0x3fff;
		}
	}
}

/***************************************
 segae_vdp_setregister

 general command format

 1 0 0 0 R R R R D D D D D D D D  1/0 = Fixed Values, R = Register # / Address, D = Data

***************************************/

void segae_vdp_setregister ( UINT8 chip, UINT16 cmd )
{
	UINT8 regnumber;
	UINT8 regdata;

	regnumber = (cmd & 0x0f00) >> 8;
	regdata   = (cmd & 0x00ff);

	if (regnumber < 11) {
		segae_vdp_regs[chip][regnumber] = regdata;
		if (regnumber == 1 && chip == 1) {
			if ((segae_vdp_regs[chip][0x1]&0x20) && vintpending) {
				cpu_set_irq_line(0, 0, HOLD_LINE);
			} else {
				cpu_set_irq_line(0, 0, CLEAR_LINE);
			}
		}
		if (regnumber == 0 && chip == 1) {
			if ((segae_vdp_regs[chip][0x0]&0x10) && hintpending) { /* dink */
				cpu_set_irq_line(0, 0, HOLD_LINE);
			} else {
				cpu_set_irq_line(0, 0, CLEAR_LINE);
			}
		}
	} else {
		/* Illegal, there aren't this many registers! */
	}
}

/*******************************************************************************
 System E Drawing Capabilities Notes
********************************************************************************
 Display Consists of

 VDP0 Backdrop Color (?)
 VDP0 Tiles (Low)
 VDP0 Sprites
 VDP0 Tiles (High)
 VDP1 Tiles (Low)
 VDP1 Sprites
 VDP1 Tiles (High)

 each vdp has its on vram, etc etc.

 the tilemaps are 256x224 in size, 256x192 of this is visible, the tiles
 are 8x8 pixels, so 32x28 of 8x8 tiles make a 256x224 tilemap.

 the tiles are 4bpp (16 colours), video ram can hold upto 512 tile gfx

 tile references are 16 bits (3 bits unused, 1 bit priority, 1 bit palette,
 2 bits for flip, 9 bits for tile number)

 tilemaps can be scrolled horizontally, the top 16 lines of the display can
 have horinzontal scrolling disabled

 tilemaps can be scrolled vertically, the right 64 lines of the display can
 have the vertical scrolling disabled

 the leftmost 8 line of the display can be blanked

*******************************************************************************/

void segae_drawscanline(int line, int chips, int blank)
{

	UINT8* dest;

	if (osd_skip_this_frame())
		return;

	dest = cache_bitmap + (16+256+16) * line;

	/* This should be cleared to bg colour, but which vdp determines that !, neither seems to be right, maybe its always the same col? */
	memset(dest, 0, 16+256+16);

	if (segae_vdp_regs[0][1] & 0x40) {
		segae_drawtilesline (dest+16, line, 0,0);
		segae_drawspriteline(dest+16, 0, line);
		segae_drawtilesline (dest+16, line, 0,1);
	}

	if (chips>0) /* we don't want to do this on megatech */
	{
		if (segae_vdp_regs[1][1] & 0x40) {
			segae_drawtilesline (dest+16, line, 1,0);
			segae_drawspriteline(dest+16, 1, line);
			segae_drawtilesline (dest+16, line, 1,1);
		}
	}

	/* FIX ME!! */
	if (blank)
	{
		if (strcmp(Machine->gamedrv->name,"tetrisse")) /* and we really don't want to do it on tetrise */
			if (strcmp(Machine->gamedrv->name,"opaopan")) /* and we really don't want to do it on opaopa */
               if (strcmp(Machine->gamedrv->name,"megrescu")) /* and we really don't want to do it on opaopa */
			memset(dest+16, 32+16, 8); /* Clear Leftmost column, there should be a register for this like on the SMS i imagine    */
							   			  /* on the SMS this is bit 5 of register 0 (according to CMD's SMS docs) for system E this  */							   			  /* appears to be incorrect, most games need it blanked 99% of the time so we blank it      */
	}

}

/*-- Drawing a line of tiles --*/

void segae_drawtilesline(UINT8 *dest, int line, UINT8 chip, UINT8 pri)
{

	UINT8 hscroll;
	UINT8 vscroll, vscroll_line;
	UINT16 tmbase;
	UINT8 tilesline, tilesline2;
	UINT8 coloffset, coloffset2;
	UINT8 loopcount;

	hscroll = (256-segae_vdp_regs[chip][8]);
	vscroll = segae_vdp_regs[chip][9];
	vscroll_line = (line + vscroll) % 224;

	tmbase =  (segae_vdp_regs[chip][2] & 0x0e) << 10;
	tmbase += (segae_vdp_vrambank[chip] * 0x4000);

	tilesline = vscroll_line >> 3;
	tilesline2= vscroll_line % 8;


	coloffset = (hscroll >> 3);
	coloffset2= (hscroll % 8);

	dest -= coloffset2;

	for (loopcount=0;loopcount<33;loopcount++) {

		UINT16 vram_offset, vram_word;
		UINT16 tile_no;
		UINT8  palette, priority, flipx, flipy;

		vram_offset = tmbase
					+ (2 * (32*tilesline + ((coloffset+loopcount)&0x1f) ) );
		vram_word = segae_vdp_vram[chip][vram_offset] | (segae_vdp_vram[chip][vram_offset+1] << 8);

		tile_no = (vram_word & 0x01ff);
		flipx =   (vram_word & 0x0200) >> 9;
		flipy =   (vram_word & 0x0400) >> 10;
		palette = (vram_word & 0x0800) >> 11;
		priority= (vram_word & 0x1000) >> 12;

		tilesline2= vscroll_line % 8;
		if (flipy) tilesline2 = 7-tilesline2;

		if (priority == pri) {
			if (chip == 0) segae_draw8pix_solid16(dest, chip, tile_no,tilesline2,flipx,palette);
			else segae_draw8pix(dest, chip, tile_no,tilesline2,flipx,palette);
		}
		dest+=8;
	}
}

void segae_drawspriteline(UINT8 *dest, UINT8 chip, UINT8 line)
{
	/* todo: figure out what riddle of pythagoras hates about this */

	int nosprites;
	int loopcount;

	UINT16 spritebase;

	nosprites = 0;

	if (segae_vdp_regs[chip][1] & 0x1) {
		usrintf_showmessage("double-size spr. not supported. ");
		return;
	}

	spritebase =  (segae_vdp_regs[chip][5] & 0x7e) << 7;
	spritebase += (segae_vdp_vrambank[chip] * 0x4000);

	/*- find out how many sprites there are -*/

	for (loopcount=0;loopcount<64;loopcount++) {
		UINT8 ypos;

		ypos = segae_vdp_vram[chip][spritebase+loopcount];

		if (ypos==208) {
			nosprites=loopcount;
			break;
		}
	}

	if (!strcmp(Machine->gamedrv->name,"ridleofp")) nosprites = 63; /* why, there must be a bug elsewhere i guess ?! */
        if (!strcmp(Machine->gamedrv->name,"slapshtr")) nosprites = 63; /* why, there must be a bug elsewhere i guess ?! */
         if (!strcmp(Machine->gamedrv->name,"megrescu")) nosprites = 63; /* why, there must be a bug elsewhere i guess ?! */
			
	/*- draw sprites IN REVERSE ORDER -*/

	for (loopcount = nosprites; loopcount >= 0;loopcount--) {
		int ypos;
		UINT8 sheight;

		ypos = segae_vdp_vram[chip][spritebase+loopcount] +1;

		if (segae_vdp_regs[chip][1] & 0x02) sheight=16; else sheight=8;

		if ( (line >= ypos) && (line < ypos+sheight) ) {
			int xpos;
			UINT16 sprnum;
			UINT8 spline;

			spline = line - ypos;

			xpos   = segae_vdp_vram[chip][spritebase+0x80+ (2*loopcount)];
			sprnum = segae_vdp_vram[chip][spritebase+0x81+ (2*loopcount)];

			if (segae_vdp_regs[chip][6] & 0x04)
			    sprnum |= 0x100;
			if (segae_vdp_regs[chip][1] & 0x02)
				sprnum &= 0x01FE;

			segae_draw8pixsprite(dest+xpos, chip, sprnum, spline);

		}
	}
}

static void segae_draw8pix_solid16(UINT8 *dest, UINT8 chip, UINT16 tile, UINT8 line, UINT8 flipx, UINT8 col)
{

	UINT32 pix8 = *(UINT32 *)&segae_vdp_vram[chip][(32)*tile + (4)*line + (0x4000) * segae_vdp_vrambank[chip]];
	UINT8  pix, coladd;

	if (!pix8 && !col) return; /*note only the colour 0 of each vdp is transparent NOT colour 16???, fixes sky in HangonJr */

	coladd = 16*col;

	if (flipx)	{
		pix = ((pix8 >> 0) & 0x01) | ((pix8 >> 7) & 0x02) | ((pix8 >> 14) & 0x04) | ((pix8 >> 21) & 0x08) ; pix+= coladd ; if (pix) dest[0] = pix+ 32*chip;
		pix = ((pix8 >> 1) & 0x01) | ((pix8 >> 8) & 0x02) | ((pix8 >> 15) & 0x04) | ((pix8 >> 22) & 0x08) ; pix+= coladd ; if (pix) dest[1] = pix+ 32*chip;
		pix = ((pix8 >> 2) & 0x01) | ((pix8 >> 9) & 0x02) | ((pix8 >> 16) & 0x04) | ((pix8 >> 23) & 0x08) ; pix+= coladd ; if (pix) dest[2] = pix+ 32*chip;
		pix = ((pix8 >> 3) & 0x01) | ((pix8 >>10) & 0x02) | ((pix8 >> 17) & 0x04) | ((pix8 >> 24) & 0x08) ; pix+= coladd ; if (pix) dest[3] = pix+ 32*chip;
		pix = ((pix8 >> 4) & 0x01) | ((pix8 >>11) & 0x02) | ((pix8 >> 18) & 0x04) | ((pix8 >> 25) & 0x08) ; pix+= coladd ; if (pix) dest[4] = pix+ 32*chip;
		pix = ((pix8 >> 5) & 0x01) | ((pix8 >>12) & 0x02) | ((pix8 >> 19) & 0x04) | ((pix8 >> 26) & 0x08) ; pix+= coladd ; if (pix) dest[5] = pix+ 32*chip;
		pix = ((pix8 >> 6) & 0x01) | ((pix8 >>13) & 0x02) | ((pix8 >> 20) & 0x04) | ((pix8 >> 27) & 0x08) ; pix+= coladd ; if (pix) dest[6] = pix+ 32*chip;
		pix = ((pix8 >> 7) & 0x01) | ((pix8 >>14) & 0x02) | ((pix8 >> 21) & 0x04) | ((pix8 >> 28) & 0x08) ; pix+= coladd ; if (pix) dest[7] = pix+ 32*chip;
	} else {
		pix = ((pix8 >> 7) & 0x01) | ((pix8 >>14) & 0x02) | ((pix8 >> 21) & 0x04) | ((pix8 >> 28) & 0x08) ; pix+= coladd ; if (pix) dest[0] = pix+ 32*chip;
		pix = ((pix8 >> 6) & 0x01) | ((pix8 >>13) & 0x02) | ((pix8 >> 20) & 0x04) | ((pix8 >> 27) & 0x08) ; pix+= coladd ; if (pix) dest[1] = pix+ 32*chip;
		pix = ((pix8 >> 5) & 0x01) | ((pix8 >>12) & 0x02) | ((pix8 >> 19) & 0x04) | ((pix8 >> 26) & 0x08) ; pix+= coladd ; if (pix) dest[2] = pix+ 32*chip;
		pix = ((pix8 >> 4) & 0x01) | ((pix8 >>11) & 0x02) | ((pix8 >> 18) & 0x04) | ((pix8 >> 25) & 0x08) ; pix+= coladd ; if (pix) dest[3] = pix+ 32*chip;
		pix = ((pix8 >> 3) & 0x01) | ((pix8 >>10) & 0x02) | ((pix8 >> 17) & 0x04) | ((pix8 >> 24) & 0x08) ; pix+= coladd ; if (pix) dest[4] = pix+ 32*chip;
		pix = ((pix8 >> 2) & 0x01) | ((pix8 >> 9) & 0x02) | ((pix8 >> 16) & 0x04) | ((pix8 >> 23) & 0x08) ; pix+= coladd ; if (pix) dest[5] = pix+ 32*chip;
		pix = ((pix8 >> 1) & 0x01) | ((pix8 >> 8) & 0x02) | ((pix8 >> 15) & 0x04) | ((pix8 >> 22) & 0x08) ; pix+= coladd ; if (pix) dest[6] = pix+ 32*chip;
		pix = ((pix8 >> 0) & 0x01) | ((pix8 >> 7) & 0x02) | ((pix8 >> 14) & 0x04) | ((pix8 >> 21) & 0x08) ; pix+= coladd ; if (pix) dest[7] = pix+ 32*chip;
	}
}

static void segae_draw8pix(UINT8 *dest, UINT8 chip, UINT16 tile, UINT8 line, UINT8 flipx, UINT8 col)
{

	UINT32 pix8 = *(UINT32 *)&segae_vdp_vram[chip][(32)*tile + (4)*line + (0x4000) * segae_vdp_vrambank[chip]];
	UINT8  pix, coladd;

	if (!pix8) return;

	coladd = 16*col+32*chip;

	if (flipx)	{
		pix = ((pix8 >> 0) & 0x01) | ((pix8 >> 7) & 0x02) | ((pix8 >> 14) & 0x04) | ((pix8 >> 21) & 0x08) ; if (pix) dest[0] = pix+ coladd;
		pix = ((pix8 >> 1) & 0x01) | ((pix8 >> 8) & 0x02) | ((pix8 >> 15) & 0x04) | ((pix8 >> 22) & 0x08) ; if (pix) dest[1] = pix+ coladd;
		pix = ((pix8 >> 2) & 0x01) | ((pix8 >> 9) & 0x02) | ((pix8 >> 16) & 0x04) | ((pix8 >> 23) & 0x08) ; if (pix) dest[2] = pix+ coladd;
		pix = ((pix8 >> 3) & 0x01) | ((pix8 >>10) & 0x02) | ((pix8 >> 17) & 0x04) | ((pix8 >> 24) & 0x08) ; if (pix) dest[3] = pix+ coladd;
		pix = ((pix8 >> 4) & 0x01) | ((pix8 >>11) & 0x02) | ((pix8 >> 18) & 0x04) | ((pix8 >> 25) & 0x08) ; if (pix) dest[4] = pix+ coladd;
		pix = ((pix8 >> 5) & 0x01) | ((pix8 >>12) & 0x02) | ((pix8 >> 19) & 0x04) | ((pix8 >> 26) & 0x08) ; if (pix) dest[5] = pix+ coladd;
		pix = ((pix8 >> 6) & 0x01) | ((pix8 >>13) & 0x02) | ((pix8 >> 20) & 0x04) | ((pix8 >> 27) & 0x08) ; if (pix) dest[6] = pix+ coladd;
		pix = ((pix8 >> 7) & 0x01) | ((pix8 >>14) & 0x02) | ((pix8 >> 21) & 0x04) | ((pix8 >> 28) & 0x08) ; if (pix) dest[7] = pix+ coladd;
	} else {
		pix = ((pix8 >> 7) & 0x01) | ((pix8 >>14) & 0x02) | ((pix8 >> 21) & 0x04) | ((pix8 >> 28) & 0x08) ; if (pix) dest[0] = pix+ coladd;
		pix = ((pix8 >> 6) & 0x01) | ((pix8 >>13) & 0x02) | ((pix8 >> 20) & 0x04) | ((pix8 >> 27) & 0x08) ; if (pix) dest[1] = pix+ coladd;
		pix = ((pix8 >> 5) & 0x01) | ((pix8 >>12) & 0x02) | ((pix8 >> 19) & 0x04) | ((pix8 >> 26) & 0x08) ; if (pix) dest[2] = pix+ coladd;
		pix = ((pix8 >> 4) & 0x01) | ((pix8 >>11) & 0x02) | ((pix8 >> 18) & 0x04) | ((pix8 >> 25) & 0x08) ; if (pix) dest[3] = pix+ coladd;
		pix = ((pix8 >> 3) & 0x01) | ((pix8 >>10) & 0x02) | ((pix8 >> 17) & 0x04) | ((pix8 >> 24) & 0x08) ; if (pix) dest[4] = pix+ coladd;
		pix = ((pix8 >> 2) & 0x01) | ((pix8 >> 9) & 0x02) | ((pix8 >> 16) & 0x04) | ((pix8 >> 23) & 0x08) ; if (pix) dest[5] = pix+ coladd;
		pix = ((pix8 >> 1) & 0x01) | ((pix8 >> 8) & 0x02) | ((pix8 >> 15) & 0x04) | ((pix8 >> 22) & 0x08) ; if (pix) dest[6] = pix+ coladd;
		pix = ((pix8 >> 0) & 0x01) | ((pix8 >> 7) & 0x02) | ((pix8 >> 14) & 0x04) | ((pix8 >> 21) & 0x08) ; if (pix) dest[7] = pix+ coladd;
	}
}

static void segae_draw8pixsprite(UINT8 *dest, UINT8 chip, UINT16 tile, UINT8 line)
{

	UINT32 pix8 = *(UINT32 *)&segae_vdp_vram[chip][(((32)*tile + (4)*line)&0x3fff) + (0x4000) * segae_vdp_vrambank[chip]];
	UINT8  pix;

	if (!pix8) return; /*note only the colour 0 of each vdp is transparent NOT colour 16, fixes sky in HangonJr */

	pix = ((pix8 >> 7) & 0x01) | ((pix8 >>14) & 0x02) | ((pix8 >> 21) & 0x04) | ((pix8 >> 28) & 0x08) ; if (pix) dest[0] = pix+16+32*chip;
	pix = ((pix8 >> 6) & 0x01) | ((pix8 >>13) & 0x02) | ((pix8 >> 20) & 0x04) | ((pix8 >> 27) & 0x08) ; if (pix) dest[1] = pix+16+32*chip;
	pix = ((pix8 >> 5) & 0x01) | ((pix8 >>12) & 0x02) | ((pix8 >> 19) & 0x04) | ((pix8 >> 26) & 0x08) ; if (pix) dest[2] = pix+16+32*chip;
	pix = ((pix8 >> 4) & 0x01) | ((pix8 >>11) & 0x02) | ((pix8 >> 18) & 0x04) | ((pix8 >> 25) & 0x08) ; if (pix) dest[3] = pix+16+32*chip;
	pix = ((pix8 >> 3) & 0x01) | ((pix8 >>10) & 0x02) | ((pix8 >> 17) & 0x04) | ((pix8 >> 24) & 0x08) ; if (pix) dest[4] = pix+16+32*chip;
	pix = ((pix8 >> 2) & 0x01) | ((pix8 >> 9) & 0x02) | ((pix8 >> 16) & 0x04) | ((pix8 >> 23) & 0x08) ; if (pix) dest[5] = pix+16+32*chip;
	pix = ((pix8 >> 1) & 0x01) | ((pix8 >> 8) & 0x02) | ((pix8 >> 15) & 0x04) | ((pix8 >> 22) & 0x08) ; if (pix) dest[6] = pix+16+32*chip;
	pix = ((pix8 >> 0) & 0x01) | ((pix8 >> 7) & 0x02) | ((pix8 >> 14) & 0x04) | ((pix8 >> 21) & 0x08) ; if (pix) dest[7] = pix+16+32*chip;

}
