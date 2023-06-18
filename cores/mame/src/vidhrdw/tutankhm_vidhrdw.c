/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/


/*  Stuff that work only in MS DOS (Color cycling)
 */

#include "driver.h"
#include "vidhrdw/generic.h"


unsigned char *tutankhm_scrollx;



static void videowrite(int offset,int data)
{
	unsigned char x1,x2,y1,y2;


	x1 = ( offset & 0x7f ) << 1;
	y1 = ( offset >> 7 );
	x2 = x1 + 1;
	y2 = y1;

	if (flip_screen_x)
	{
		x1 = 255 - x1;
		x2 = 255 - x2;
	}
	if (flip_screen_y)
	{
		y1 = 255 - y1;
		y2 = 255 - y2;
	}

	plot_pixel(tmpbitmap,x1,y1,Machine->pens[data & 0x0f]);
	plot_pixel(tmpbitmap,x2,y2,Machine->pens[data >> 4]);
}



WRITE_HANDLER( tutankhm_videoram_w )
{
	videoram[offset] = data;
	videowrite(offset,data);
}



/***************************************************************************

  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
VIDEO_UPDATE( tutankhm )
{
	if (get_vh_global_attribute_changed())
	{
		int offs;

		for (offs = 0;offs < videoram_size;offs++)
			tutankhm_videoram_w(offs,videoram[offs]);
	}

	/* copy the temporary bitmap to the screen */
	{
		int scroll[32], i;


		if (flip_screen_x)
		{
			for (i = 0;i < 8;i++)
				scroll[i] = 0;
			for (i = 8;i < 32;i++)
			{
				scroll[i] = -*tutankhm_scrollx;
				if (flip_screen_y) scroll[i] = -scroll[i];
			}
		}
		else
		{
			for (i = 0;i < 24;i++)
			{
				scroll[i] = -*tutankhm_scrollx;
				if (flip_screen_y) scroll[i] = -scroll[i];
			}
			for (i = 24;i < 32;i++)
				scroll[i] = 0;
		}

		copyscrollbitmap(bitmap,tmpbitmap,0,0,32,scroll,&Machine->visible_area,TRANSPARENCY_NONE,0);
	}
}



/* Juno First Blitter Hardware emulation

	Juno First can blit a 16x16 graphics which comes from un-memory mapped graphics roms

	$8070->$8071 specifies the destination NIBBLE address
	$8072->$8073 specifies the source NIBBLE address

	Depending on bit 0 of the source address either the source pixels will be copied to
	the destination address, or a zero will be written.
	This allows the game to quickly clear the sprites from the screen

	A lookup table is used to swap the source nibbles as they are the wrong way round in the
	source data.

	Bugs -

		Currently only the even pixels will be written to. This is to speed up the blit routine
		as it does not have to worry about shifting the source data.
		This means that all destination X values will be rounded to even values.
		In practice no one actaully notices this.

		The clear works properly.
*/

WRITE_HANDLER( junofrst_blitter_w )
{
	static unsigned char blitterdata[4];


	blitterdata[offset] = data;

	/* Blitter is triggered by $8073 */
	if (offset==3)
	{
		int i;
		unsigned long srcaddress;
		unsigned long destaddress;
		unsigned char srcflag;
		unsigned char destflag;
		unsigned char *JunoBLTRom = memory_region(REGION_GFX1);

		srcaddress = (blitterdata[0x2]<<8) | (blitterdata[0x3]);
		srcflag = srcaddress & 1;
		srcaddress >>= 1;
		srcaddress &= 0x7FFE;
		destaddress = (blitterdata[0x0]<<8)  | (blitterdata[0x1]);

		destflag = destaddress & 1;

		destaddress >>= 1;
		destaddress &= 0x7fff;

		if (srcflag) {
			for (i=0;i<16;i++) {

#define JUNOBLITPIXEL(x)									\
	if (JunoBLTRom[srcaddress+x])							\
		tutankhm_videoram_w( destaddress+x,					\
			((JunoBLTRom[srcaddress+x] & 0xf0) >> 4)		\
			| ((JunoBLTRom[srcaddress+x] & 0x0f) << 4));

				JUNOBLITPIXEL(0);
				JUNOBLITPIXEL(1);
				JUNOBLITPIXEL(2);
				JUNOBLITPIXEL(3);
				JUNOBLITPIXEL(4);
				JUNOBLITPIXEL(5);
				JUNOBLITPIXEL(6);
				JUNOBLITPIXEL(7);

				destaddress += 128;
				srcaddress += 8;
			}
		} else {
			for (i=0;i<16;i++) {

#define JUNOCLEARPIXEL(x) 						\
	if ((JunoBLTRom[srcaddress+x] & 0xF0)) 		\
		tutankhm_videoram_w( destaddress+x,		\
			videoram[destaddress+x] & 0xF0);	\
	if ((JunoBLTRom[srcaddress+x] & 0x0F))		\
		tutankhm_videoram_w( destaddress+x,		\
			videoram[destaddress+x] & 0x0F);

				JUNOCLEARPIXEL(0);
				JUNOCLEARPIXEL(1);
				JUNOCLEARPIXEL(2);
				JUNOCLEARPIXEL(3);
				JUNOCLEARPIXEL(4);
				JUNOCLEARPIXEL(5);
				JUNOCLEARPIXEL(6);
				JUNOCLEARPIXEL(7);
				destaddress += 128;
				srcaddress+= 8;
			}
		}
	}
}
