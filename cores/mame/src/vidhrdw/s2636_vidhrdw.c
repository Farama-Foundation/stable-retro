/*************************************************************
 *                                                           *
 * Signetics 2636 video chip                                 *
 *                                                           *
 *************************************************************

 PVI REGISTER DESCRIPTION
 ------------------------

       |              bit              |R/W| description
 byte  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |   |
       |                               |   |
 FC0   | size 4| size 3| size 2| size 1| W | size of the 4 objects(=sprites)
       |                               |   |
 FC1   |       |C1 |C2 |C3 |C1 |C2 |C3 | W | colours of the 4 objects
       |       |  colour 1 |  colour 2 |   |
 FC2   |       |C1 |C2 |C3 |C1 |C2 |C3 | W |
       |       |  colour 3 |  colour 4 |   |
       |                               |   |
 FC3   |                       |sh |pos| W | 1=shape 0=position
       |                               |   | display format and position
 FC4   |            (free)             |   |
 FC5   |            (free)             |   |
       |                               |   |
 FC6   |   |C1 |C2 |C3 |BG |scrn colr  | W | background lock and colour
       |   |backg colr |enb|C1 |C2 |C3 |   | 3="enable"
       |                               |   |
 FC7   |            sound              | W | squarewave output
       |                               |   |
 FC8   |       N1      |      N2       | W | range of the 4 display digits
 FC9   |       N3      |      N4       | W |
       |                               |   |
       |obj/backgrnd   |complete object| R |
 FCA   | 1 | 2 | 3 | 4 | 1 | 2 | 3 | 4 |   |
       |                               |   |
 FCB   |   |VR-|   object collisions   | R | Composition of object and back-
       |   |LE |1/2|1/3|1/3|1/4|2/4|3/4|   | ground,collision detection and
       |                               |   | object display as a state display
       |                               |   | for the status register.Set VRLE.
       |                               |   | wait for VRST.Read out or transmit
       |                               |   | [copy?] all bits until reset by
       |                               |   | VRST.
       |                               |   |
 FCC   |            PORT1              | R | PORT1 and PORT2 for the range of
 FCD   |            PORT2              |   | the A/D conversion.Cleared by VRST
 FCE   |            (free)             |   |
 FCF   |            (free)             |   |


 Size control by byte FC0

  bit  matrix
 |0|0|  8x10
 |0|1| 16x20
 |1|0| 32x40
 |1|1| 64x80

 CE1 and not-CE2 are outputs from the PVI.$E80..$EFF also controls the
 analogue multiplexer.


 SPRITES
 -------

 each object field: (=sprite data structure)

 0 \ 10 bytes of bitmap (Each object is 8 pixels wide.)
 9 /
 A   HC  horizontal object coordinate
 B   HCB horizontal dublicate coordinate
 C   VC  vertical object coordinate
 D   VCB vertical dublicate coordinate

 *************************************************************/

#include "driver.h"

static int SpriteOffset[4] = {0,0x10,0x20,0x40};

/* To adjust sprites against bitmap */

int s2636_x_offset=0;
int s2636_y_offset=0;

void s2636_w(unsigned char *workram,int offset,int data,unsigned char *dirty)
{
	if (workram[offset] != data)
    {
        workram[offset] = data;

        if(offset < 10)
			dirty[0]=1;
        else
        {
	        if((offset > 15) && (offset < 26))
				dirty[1]=1;
            else
            {
		        if((offset > 31) && (offset < 42))
					dirty[2]=1;
                else
                {
			        if((offset > 63) && (offset < 74))
						dirty[3]=1;
                }
            }
        }
    }
}

/*****************************************/
/* Check for Collision between 2 sprites */
/*****************************************/

static int SpriteCheck(int first,int second,unsigned char *workram,int Graphics_Bank,struct mame_bitmap *collision_bitmap)
{
	int Checksum=0;
	int x,y;

    /* Does not check shadow sprites yet*/

    if((workram[SpriteOffset[first] + 10] != 0xff) && (workram[SpriteOffset[second] + 10] != 0xff))
    {
    	int fx1 = workram[SpriteOffset[first] + 10] + s2636_x_offset;
        int fy1 = workram[SpriteOffset[first] + 12] + s2636_y_offset;
		int fx2 = workram[SpriteOffset[second] + 10] + s2636_x_offset;
		int fy2 = workram[SpriteOffset[second] + 12] + s2636_y_offset;

        if((fx1>=0) && (fy1>=0) && (fx2>=0) && (fy2>=0))
		{
  		    int expand1 = 1 << (16+((workram[0xC0]>>(first<<1)) & 3));
  		    int expand2 = 1 << (16+((workram[0xC0]>>(second<<1)) & 3));

    	    int char1   = SpriteOffset[first]>>4;
    	    int char2   = SpriteOffset[second]>>4;

            /* Draw first sprite */

		    drawgfxzoom(collision_bitmap,Machine->gfx[Graphics_Bank],
		                char1,
			            1,
		                0,0,
		                fx1,fy1,
		                &Machine->visible_area, TRANSPARENCY_PEN, 0,
				        expand1,expand1);

            /* Get fingerprint */

	        for (x = fx1; x < fx1 + Machine->gfx[Graphics_Bank]->width; x++)
	        {
		        for (y = fy1; y < fy1 + Machine->gfx[Graphics_Bank]->height; y++)
                {
			        if ((x < Machine->visible_area.min_x) ||
			            (x > Machine->visible_area.max_x) ||
			            (y < Machine->visible_area.min_y) ||
			            (y > Machine->visible_area.max_y))
			        {
				        continue;
			        }

        	        Checksum += read_pixel(collision_bitmap, x, y);
                }
	        }

            /* Blackout second sprite */

		    drawgfxzoom(collision_bitmap,Machine->gfx[Graphics_Bank],
		                char2,
			            0,
		                0,0,
				        fx2,fy2,
		                &Machine->visible_area, TRANSPARENCY_PEN, 0,
				        expand2,expand2);

            /* Remove fingerprint */

	        for (x = fx1; x < fx1 + Machine->gfx[Graphics_Bank]->width; x++)
	        {
		        for (y = fy1; y < fy1 + Machine->gfx[Graphics_Bank]->height; y++)
                {
			        if ((x < Machine->visible_area.min_x) ||
			            (x > Machine->visible_area.max_x) ||
			            (y < Machine->visible_area.min_y) ||
			            (y > Machine->visible_area.max_y))
			        {
				        continue;
			        }

        	        Checksum -= read_pixel(collision_bitmap, x, y);
                }
	        }

            /* Zero bitmap */

		    drawgfxzoom(collision_bitmap,Machine->gfx[Graphics_Bank],
		                char1,
			            0,
		                0,0,
		                fx1,fy1,
		                &Machine->visible_area, TRANSPARENCY_PEN, 0,
				        expand1,expand1);
            }
    }

	return Checksum;
}

void Update_Bitmap(struct mame_bitmap *bitmap,unsigned char *workram,unsigned char *dirty,int Graphics_Bank,struct mame_bitmap *collision_bitmap)
{
	int CollisionSprite = 0;
    int spriteno;
    int offs;

    for(spriteno=0;spriteno<4;spriteno++)
    {
    	offs = SpriteOffset[spriteno];

    	if(workram[offs+10]!=0xFF)
		{
			int charno   = offs>>4;
	  		int expand   = 1 << (16+((workram[0xC0]>>(spriteno<<1)) & 3));
            int bx       = workram[offs+10] + s2636_x_offset;
            int by       = workram[offs+12] + s2636_y_offset;

            if((bx >= 0) && (by >= 0))
            {
                /* Get colour and mask correct bits */

                int colour   = workram[0xC1 + (spriteno >> 1)];

                if((spriteno & 1)==0) colour >>= 3;

                colour = (colour & 7) + 7;

                if(dirty[spriteno])
                {
	   			    decodechar(Machine->gfx[Graphics_Bank],charno,workram,Machine->drv->gfxdecodeinfo[Graphics_Bank].gfxlayout);
                    dirty[spriteno] = 0;
                }

		        drawgfxzoom(bitmap,Machine->gfx[Graphics_Bank],
			                charno,
				            colour,
			                0,0,
			                bx,by,
			                &Machine->visible_area,
							TRANSPARENCY_BLEND_RAW, 0,
					        expand,expand);

                /* Shadow Sprites */

                if((workram[offs+11]!=0xff) && (workram[offs+13]!=0xfe))
                {
            	    bx=workram[offs+11] + s2636_x_offset;

                    if(bx >= 0)
                    {
	            	    for(;by < 255;)
					    {
						    by=by+10+workram[offs+13];

				            drawgfxzoom(bitmap,Machine->gfx[Graphics_Bank],
					                    charno,
						                colour,
				    	                0,0,
				        	            bx,by,
				            	        &Machine->visible_area,
										TRANSPARENCY_BLEND_RAW, 0,
						        	    expand,expand);
	                    }
                    }
                }
            }
        }
    }

    /* Sprite->Sprite collision detection */

    if(SpriteCheck(0,1,workram,Graphics_Bank,collision_bitmap)) CollisionSprite |= 0x20;
    if(SpriteCheck(0,2,workram,Graphics_Bank,collision_bitmap)) CollisionSprite |= 0x10;
    if(SpriteCheck(0,3,workram,Graphics_Bank,collision_bitmap)) CollisionSprite |= 0x08;
    if(SpriteCheck(1,2,workram,Graphics_Bank,collision_bitmap)) CollisionSprite |= 0x04;
    if(SpriteCheck(1,3,workram,Graphics_Bank,collision_bitmap)) CollisionSprite |= 0x02;
    if(SpriteCheck(2,3,workram,Graphics_Bank,collision_bitmap)) CollisionSprite |= 0x01;

    workram[0xCB] = CollisionSprite;
}
