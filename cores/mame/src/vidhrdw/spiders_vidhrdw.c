/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/crtc6845.h"

extern int spiders_video_flip;

static int bitflip[256];
static int *screenbuffer;

#define SCREENBUFFER_SIZE	0x2000
#define SCREENBUFFER_MASK	0x1fff

/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
VIDEO_START( spiders )
{
	int loop;

	if ((tmpbitmap = auto_bitmap_alloc(Machine->drv->screen_width,Machine->drv->screen_height)) == 0) return 1;

	for(loop=0;loop<256;loop++)
	{
		bitflip[loop]=(loop&0x01)?0x80:0x00;
		bitflip[loop]|=(loop&0x02)?0x40:0x00;
		bitflip[loop]|=(loop&0x04)?0x20:0x00;
		bitflip[loop]|=(loop&0x08)?0x10:0x00;
		bitflip[loop]|=(loop&0x10)?0x08:0x00;
		bitflip[loop]|=(loop&0x20)?0x04:0x00;
		bitflip[loop]|=(loop&0x40)?0x02:0x00;
		bitflip[loop]|=(loop&0x80)?0x01:0x00;
	}

	if ((screenbuffer = auto_malloc(SCREENBUFFER_SIZE*sizeof(int))) == 0) return 1;
	memset(screenbuffer,1,SCREENBUFFER_SIZE*sizeof(int));

	return 0;
}


/***************************************************************************

  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
VIDEO_UPDATE( spiders )
{
	int loop,data0,data1,data2,col;

	size_t crtc6845_mem_size;
	int video_addr,increment;

	unsigned char *RAM = memory_region(REGION_CPU1);


	crtc6845_mem_size=crtc6845_horiz_disp*crtc6845_vert_disp*8;

	if(spiders_video_flip)
	{
		video_addr=crtc6845_start_addr+(crtc6845_mem_size-1);
		if((video_addr&0xff)==0x80) video_addr-=0x80;	/* Fudge factor!!! */
		increment=-1;
	}
	else
	{
		video_addr=crtc6845_start_addr;
		increment=1;
	}

	video_addr&=0xfbff;	/* Fudge factor: sometimes this bit gets set and */
				/* I've no idea how it maps to the hardware but  */
				/* everything works OK if we do this             */

	if(crtc6845_page_flip) video_addr+=0x2000;

	for(loop=0;loop<crtc6845_mem_size;loop++)
	{
		int i,x,y,combo;

		if(spiders_video_flip)
		{
			data0=bitflip[RAM[0x0000+video_addr]];
			data1=bitflip[RAM[0x4000+video_addr]];
			data2=bitflip[RAM[0x8000+video_addr]];
		}
		else
		{
			data0=RAM[0x0000+video_addr];
			data1=RAM[0x4000+video_addr];
			data2=RAM[0x8000+video_addr];
		}

		combo=data0|(data1<<8)|(data2<<16);

		if(screenbuffer[video_addr&SCREENBUFFER_MASK]!=combo)
		{

			y=loop/0x20;

			for(i=0;i<8;i++)
			{
				x=((loop%0x20)<<3)+i;
				col=((data2&0x01)<<2)+((data1&0x01)<<1)+(data0&0x01);

				plot_pixel(tmpbitmap, x, y, Machine->pens[col]);

				data0 >>= 1;
				data1 >>= 1;
				data2 >>= 1;
			}
			screenbuffer[video_addr&SCREENBUFFER_MASK]=combo;
		}
		video_addr+=increment;
		video_addr&=0x3fff;
	}

	/* Now copy the temp bitmap to the screen */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);
}
