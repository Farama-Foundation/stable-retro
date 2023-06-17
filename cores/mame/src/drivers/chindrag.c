/* Dragon World / Virtua Bowling */


#include "driver.h"
#include "vidhrdw/generic.h"
#include "sound/ics2115.h"


UINT8 *layer[8];

static UINT16 igs_priority, *igs_priority_ram;

static UINT8 chmplst2_pen_hi;	/* high 3 bits of pens (chmplst2 only) */


static WRITE16_HANDLER( igs_priority_w )
{
	COMBINE_DATA(&igs_priority);

	if (data & ~0x7)
		log_cb(RETRO_LOG_DEBUG, LOGPRE "%06x: warning, unknown bits written to igs_priority = %02x\n", activecpu_get_pc(), igs_priority);
}


VIDEO_START(igs)
{
	int i;

	for (i = 0; i < 8; i++)
	{
		layer[i] = auto_malloc(512 * 256);
	}

	chmplst2_pen_hi = 0;

	return 0;
}

VIDEO_UPDATE(igs)
{
#ifdef MAME_DEBUG
	int layer_enable = -1;
#endif

	int x,y,l,scr_addr,pri_addr;
	UINT16 *pri_ram;

#ifdef MAME_DEBUG
	if (code_pressed(KEYCODE_Z))
	{
		int mask = 0;
		if (code_pressed(KEYCODE_Q))	mask |= 0x01;
		if (code_pressed(KEYCODE_W))	mask |= 0x02;
		if (code_pressed(KEYCODE_E))	mask |= 0x04;
		if (code_pressed(KEYCODE_R))	mask |= 0x08;
		if (code_pressed(KEYCODE_A))	mask |= 0x10;
		if (code_pressed(KEYCODE_S))	mask |= 0x20;
		if (code_pressed(KEYCODE_D))	mask |= 0x40;
		if (code_pressed(KEYCODE_F))	mask |= 0x80;
		if (mask)	layer_enable &= mask;
	}
#endif

	pri_ram = &igs_priority_ram[(igs_priority & 7) * 512/2];

	for (y = cliprect->min_y; y <= cliprect->max_y; y++)
	{
		for (x = cliprect->min_x; x <= cliprect->max_x; x++)
		{
			scr_addr = x + y * 512;
			pri_addr = 0xff;

			for (l = 0; l < 8; l++)
			{
				if (	(layer[l][scr_addr] != 0xff)
#ifdef MAME_DEBUG
						&& (layer_enable & (1 << l))
#endif
					)
					pri_addr &= ~(1 << l);
			}


			l	=	pri_ram[pri_addr] & 7;

#ifdef MAME_DEBUG
			if ((layer_enable != -1) && (pri_addr == 0xff))
				plot_pixel( bitmap, x,y, get_black_pen() );
			else
#endif
				plot_pixel( bitmap, x,y, layer[l][scr_addr] | (l << 8) );
		}
	}
}

static READ16_HANDLER( igs_layers_r )
{
	int layer0 = ((offset & (0x80000/2)) ? 4 : 0) + ((offset & 1) ? 0 : 2);

	UINT8 *l0 = layer[layer0];
	UINT8 *l1 = layer[layer0+1];

	offset >>= 1;
	offset &= 0x1ffff;

	return (l0[offset] << 8) | l1[offset];
}

static WRITE16_HANDLER( igs_layers_w )
{
	UINT16 word;

	int layer0 = ((offset & (0x80000/2)) ? 4 : 0) + ((offset & 1) ? 0 : 2);

	UINT8 *l0 = layer[layer0];
	UINT8 *l1 = layer[layer0+1];

	offset >>= 1;
	offset &= 0x1ffff;

	word = (l0[offset] << 8) | l1[offset];
	COMBINE_DATA(&word);
	l0[offset] = word >> 8;
	l1[offset] = word;
}

/***************************************************************************

    Blitter

***************************************************************************/

static struct
{

	UINT16	x, y, w, h,
			gfx_lo, gfx_hi,
			depth,
			pen,
			flags;

}	blitter;


static WRITE16_HANDLER( igs_blit_x_w )		{	COMBINE_DATA(&blitter.x);		}
static WRITE16_HANDLER( igs_blit_y_w )		{	COMBINE_DATA(&blitter.y);		}
static WRITE16_HANDLER( igs_blit_gfx_lo_w )	{	COMBINE_DATA(&blitter.gfx_lo);	}
static WRITE16_HANDLER( igs_blit_gfx_hi_w )	{	COMBINE_DATA(&blitter.gfx_hi);	}
static WRITE16_HANDLER( igs_blit_w_w )		{	COMBINE_DATA(&blitter.w);		}
static WRITE16_HANDLER( igs_blit_h_w )		{	COMBINE_DATA(&blitter.h);		}
static WRITE16_HANDLER( igs_blit_depth_w )	{	COMBINE_DATA(&blitter.depth);	}
static WRITE16_HANDLER( igs_blit_pen_w )	{	COMBINE_DATA(&blitter.pen);		}


static WRITE16_HANDLER( igs_blit_flags_w )
{
	int x, xstart, xend, xinc, flipx;
	int y, ystart, yend, yinc, flipy;
	int depth4, clear, opaque, z;
	UINT8 trans_pen, clear_pen, pen_hi, *dest;
	UINT8 pen = 0;

	UINT8 *gfx		=	memory_region(REGION_GFX1);
	UINT8 *gfx2		=	memory_region(REGION_GFX2);
	int gfx_size	=	memory_region_length(REGION_GFX1);
	int gfx2_size	=	memory_region_length(REGION_GFX2);

    struct	rectangle clip	=	Machine->visible_area;

	COMBINE_DATA(&blitter.flags);

	log_cb(RETRO_LOG_DEBUG, LOGPRE "%06x: blit x %03x, y %03x, w %03x, h %03x, gfx %03x%04x, depth %02x, pen %02x, flags %03x\n", activecpu_get_pc(),
					blitter.x,blitter.y,blitter.w,blitter.h,blitter.gfx_hi,blitter.gfx_lo,blitter.depth,blitter.pen,blitter.flags);

	dest	=	layer[	   blitter.flags & 0x0007	];
	opaque	=			 !(blitter.flags & 0x0008);
	clear	=			   blitter.flags & 0x0010;
	flipx	=			   blitter.flags & 0x0020;
	flipy	=			   blitter.flags & 0x0040;
	if					(!(blitter.flags & 0x0400)) return;

	pen_hi	=	(chmplst2_pen_hi & 0x07) << 5;

	/* pixel address */
	z		=	blitter.gfx_lo  + (blitter.gfx_hi << 16);

	/* what were they smoking??? */
	depth4	=	!((blitter.flags & 0x7) < (4 - (blitter.depth & 0x7))) ||
				(z & 0x800000);		/* see chmplst2 */

	z &= 0x7fffff;

	if (depth4)
	{
		z	*=	2;
		if (gfx2 && (blitter.gfx_hi & 0x80))	trans_pen = 0x1f;	/* chmplst2 */
		else									trans_pen = 0x0f;

		clear_pen = blitter.pen | 0xf0;
	}
	else
	{
		if (gfx2)	trans_pen = 0x1f;	/* vbowl */
		else		trans_pen = 0xff;

		clear_pen = blitter.pen;
	}

	xstart = (blitter.x & 0x1ff) - (blitter.x & 0x200);
	ystart = (blitter.y & 0x0ff) - (blitter.y & 0x100);

	if (flipx)	{ xend = xstart - (blitter.w & 0x1ff) - 1;	xinc = -1; }
	else		{ xend = xstart + (blitter.w & 0x1ff) + 1;	xinc =  1; }

	if (flipy)	{ yend = ystart - (blitter.h & 0x0ff) - 1;	yinc = -1; }
	else		{ yend = ystart + (blitter.h & 0x0ff) + 1;	yinc =  1; }

	for (y = ystart; y != yend; y += yinc)
	{
		for (x = xstart; x != xend; x += xinc)
		{
			/* fetch the pixel */
			if (!clear)
			{
				if (depth4)		pen = (gfx[(z/2)%gfx_size] >> ((z&1)?4:0)) & 0x0f;
				else			pen = gfx[z%gfx_size];

				if ( gfx2 )
				{
					pen &= 0x0f;
					if ( gfx2[(z/8)%gfx2_size] & (1 << (z & 7)) )
						pen |= 0x10;
				}
			}

			/* plot it */
			if (x >= clip.min_x && x <= clip.max_x && y >= clip.min_y && y <= clip.max_y)
			{
				if      (clear)				dest[x + y * 512] = clear_pen;
				else if (pen != trans_pen)	dest[x + y * 512] = pen | pen_hi;
				else if (opaque)			dest[x + y * 512] = 0xff;
			}

			z++;
		}
	}

	#ifdef MAME_DEBUG
#if 1
	if (code_pressed(KEYCODE_Z))
	{	char buf[20];
		sprintf(buf, "%02X%02X",blitter.depth,blitter.flags&0xff);
	}
#endif
	#endif
}

/***************************************************************************

    Common functions

***************************************************************************/

/* Inputs */

static UINT16 igs_input_sel,igs_input_sel2;

#define IGS_INPUT_RW( NUM )						\
												\
static WRITE16_HANDLER( igs_##NUM##_input_w )	\
{												\
	COMBINE_DATA(&igs_input_sel);				\
												\
	if ( (igs_input_sel & 0xff00) || ((~igs_input_sel) & ((1 << NUM)-1)) )	\
		log_cb(RETRO_LOG_DEBUG, LOGPRE "%06x: warning, unknown bits written in igs_input_sel = %02x\n", activecpu_get_pc(), igs_input_sel);	\
}												\
												\
static READ16_HANDLER( igs_##NUM##_input_r )	\
{												\
	int i;										\
	UINT16 ret=0;								\
												\
	for (i = 0; i < NUM; i++)					\
		if ((~igs_input_sel) & (1 << i) )		\
			ret = readinputport(i);				\
												\
	/* 0x0100 is blitter busy */				\
	return 	(ret & 0xff) | 0x0000;				\
}

IGS_INPUT_RW( 3 )
IGS_INPUT_RW( 4 )
IGS_INPUT_RW( 5 )



/* Palette r5g5b5
   offset+0x000: xRRRRRGG
   offset+0x800: GGGBBBBB
*/

static WRITE16_HANDLER( igs_palette_w )
{
	int r,g,b, rgb;

	COMBINE_DATA(&paletteram16[offset]);

	rgb = (paletteram16[offset & 0x7ff] & 0xff) | ((paletteram16[offset | 0x800] & 0xff) << 8);

	r	=	(rgb >>  0) & 0x1f;
	g	=	(rgb >>  5) & 0x1f;
	b	=	(rgb >> 10) & 0x1f;

	r	=	(r << 3) | (r >> 2);
	g	=	(g << 3) | (g >> 2);
	b	=	(b << 3) | (b >> 2);

	palette_set_color(offset & 0x7ff,r,g,b);
}



/***************************************************************************

    Code Decryption

***************************************************************************/

void grtwall_decrypt(void)
{
	int i;
	UINT16 *src = (UINT16 *) (memory_region(REGION_CPU1));

	int rom_size = 0x80000;

	for (i=0; i<rom_size/2; i++)
	{
		UINT16 x = src[i];

    	if ((i & 0x2000) == 0x0000 || (i & 0x0004) == 0x0000 || (i & 0x0090) == 0x0000)
			x ^= 0x0004;
    	if ((i & 0x0100) == 0x0100 || (i & 0x0040) == 0x0040 || (i & 0x0012) == 0x0012)
			x ^= 0x0020;
    	if ((i & 0x2400) == 0x0000 || (i & 0x4100) == 0x4100 || ((i & 0x2000) == 0x2000 && (i & 0x0c00) != 0x0000))
			x ^= 0x0200;
    	if ((x & 0x0024) == 0x0004 || (x & 0x0024) == 0x0020)
			x ^= 0x0024;
		src[i] = x;
	}
}


void lhb_decrypt(void)
{
	int i;
	UINT16 *src = (UINT16 *) (memory_region(REGION_CPU1));

	int rom_size = 0x80000;

	for (i=0; i<rom_size/2; i++)
	{
		UINT16 x = src[i];

		if ((i & 0x1100) != 0x0100)
			x ^= 0x0002;

		if ((i & 0x0150) != 0x0000 && (i & 0x0152) != 0x0010)
			x ^= 0x0400;

		if ((i & 0x2084) != 0x2084 && (i & 0x2094) != 0x2014)
			x ^= 0x2000;

		src[i] = (x >> 8) | (x <<8);
	}
}


void chindrag_decrypt(void)
{
	int i;
	UINT16 *src = (UINT16 *) (memory_region(REGION_CPU1));

	int rom_size = 0x80000;

	for (i=0; i<rom_size/2; i++)
	{
		UINT16 x = src[i];

		if ((i & 0x2000) == 0x0000 || (i & 0x0004) == 0x0000 || (i & 0x0090) == 0x0000)
			x ^= 0x0004;

		if ((i & 0x0100) == 0x0100 || (i & 0x0040) == 0x0040 || (i & 0x0012) == 0x0012)
			x ^= 0x0020;

		if ((((i & 0x1000) == 0x1000) ^ ((i & 0x0100) == 0x0100))
			|| (i & 0x0880) == 0x0800 || (i & 0x0240) == 0x0240)
				x ^= 0x0200;

		if ((x & 0x0024) == 0x0004 || (x & 0x0024) == 0x0020)
			x ^= 0x0024;

		src[i] = x;
	}
}


void drgnwrld_decrypt(void)
{
	int i;
	UINT16 *src = (UINT16 *) (memory_region(REGION_CPU1));

	int rom_size = 0x80000;

	for (i=0; i<rom_size/2; i++)
	{
		UINT16 x = src[i];

		if ((i & 0x2000) == 0x0000 || (i & 0x0004) == 0x0000 || (i & 0x0090) == 0x0000)
			x ^= 0x0004;

		if ((i & 0x0100) == 0x0100 || (i & 0x0040) == 0x0040 || (i & 0x0012) == 0x0012)
			x ^= 0x0020;
/*
        if ((((i & 0x1000) == 0x1000) ^ ((i & 0x0100) == 0x0100))
            || (i & 0x0880) == 0x0800 || (i & 0x0240) == 0x0240)
                x ^= 0x0200;
*/
		if ((x & 0x0024) == 0x0004 || (x & 0x0024) == 0x0020)
			x ^= 0x0024;

		src[i] = x;
	}
}


void chmplst2_decrypt(void)
{
	int i,j;
	int rom_size = 0x80000;
	UINT16 *src = (UINT16 *) (memory_region(REGION_CPU1));
	UINT16 *result_data = malloc(rom_size);

 	for (i=0; i<rom_size/2; i++)
	{
		UINT16 x = src[i];

		if ((i & 0x0054) != 0x0000 && (i & 0x0056) != 0x0010)
			x ^= 0x0400;

		if ((i & 0x0204) == 0x0000)
 			x ^= 0x0800;

		if ((i & 0x3080) != 0x3080 && (i & 0x3090) != 0x3010)
			x ^= 0x2000;

		j = BITSWAP24(i, 23,22,21,20,19,18,17,16,15,14,13, 8, 11,10, 9, 2, 7,6,5,4,3, 12, 1,0);

		result_data[j] = (x >> 8) | (x << 8);
	}

	memcpy(src,result_data,rom_size);

	free(result_data);
}



void vbowlj_decrypt(void)
{
	int i;
	UINT16 *src = (UINT16 *) (memory_region(REGION_CPU1));

	int rom_size = 0x80000;

	for(i=0; i<rom_size/2; i++)
	{
		UINT16 x = src[i];

		if((i & 0x4100) == 0x0100)
			x ^= 0x0002;

		if((i & 0x4000) == 0x4000 && (i & 0x0300) != 0x0100)
			x ^= 0x0002;

		if((i & 0x5700) == 0x5100)
			x ^= 0x0002;

		if((i & 0x5500) == 0x1000)
			x ^= 0x0002;

		if((i & 0x0140) != 0x0000 || (i & 0x0012) == 0x0012)
			x ^= 0x0400;

		if((i & 0x2004) != 0x2004 || (i & 0x0090) == 0x0000)
			x ^= 0x2000;

	    src[i] = (x << 8) | (x >> 8);
	  }
}


void vbowl_decrypt(void)
{
	int i;
	UINT16 *src = (UINT16 *) (memory_region(REGION_CPU1));

	int rom_size = 0x80000;

	for(i=0; i<rom_size/2; i++)
	{
		UINT16 x = src[i];

				if( i & 0x1000/2 )
		{
			if( ~i & 0x400/2 )
				x ^= 0x0002;
		}

		if( i & 0x4000/2 )
		{
			if( i & 0x100/2 )
			{
				if( ~i & 0x08/2 )
					x ^= 0x2000;
			}
			else
			{
				if( ~i & 0x28/2 )
					x ^= 0x2000;
			}
		}
		else
		{
			x ^= 0x2000;
		}

		if( i & 0x200/2 )
		{
			x ^= 0x0400;
		}
		else
		{
			if( (i & 0x80/2) == 0x80/2 || (i & 0x24/2) == 0x24/2 )
				x ^= 0x0400;
		}

		src[i] = (x << 8) | (x >> 8);
	}
}



/***************************************************************************

    Gfx Decryption

***************************************************************************/


void chmplst2_decrypt_gfx(void)
{
	int i;
	unsigned rom_size = 0x200000;
	UINT8 *src = (UINT8 *) (memory_region(REGION_GFX1));
	UINT8 *result_data = malloc(rom_size);

	for (i=0; i<rom_size; i++)
    	result_data[i] = src[BITSWAP24(i, 23,22,21,20, 19, 17,16,15, 13,12, 10,9,8,7,6,5,4, 2,1, 3, 11, 14, 18, 0)];

	memcpy(src,result_data,rom_size);

	free(result_data);
}

void chindrag_gfx_decrypt(void)
{
	int i;
	unsigned rom_size = 0x400000;
	UINT8 *src = (UINT8 *) (memory_region(REGION_GFX1));
	UINT8 *result_data = malloc(rom_size);

 	for (i=0; i<rom_size; i++)
    	result_data[i] = src[BITSWAP24(i, 23,22,21,20,19,18,17,16,15, 12, 13, 14, 11,10,9,8,7,6,5,4,3,2,1,0)];

	memcpy(src,result_data,rom_size);

	free(result_data);
}



/***************************************************************************

    Protection & I/O

***************************************************************************/

static UINT16 igs_magic[2];

static WRITE16_HANDLER( chmplst2_magic_w )
{
	COMBINE_DATA(&igs_magic[offset]);

	if (offset == 0)
		return;

	switch(igs_magic[0])
	{
		case 0x00:
			COMBINE_DATA(&igs_input_sel2);

			if (ACCESSING_LSB)
			{
				coin_counter_w(0,	data & 0x20);
				/*  coin out        data & 0x40 */
			}

			if ( igs_input_sel2 & ~0x7f )
				log_cb(RETRO_LOG_DEBUG, LOGPRE "%06x: warning, unknown bits written in igs_input_sel2 = %02x\n", activecpu_get_pc(), igs_input_sel2);

			break;

		case 0x02:
			if (ACCESSING_LSB)
			{
				chmplst2_pen_hi = data & 0x07;

				OKIM6295_set_bank_base(0, (data & 0x08) ? 0x40000 : 0);
			}

			if ( chmplst2_pen_hi & ~0xf )
				log_cb(RETRO_LOG_DEBUG, LOGPRE "%06x: warning, unknown bits written in chmplst2_pen_hi = %02x\n", activecpu_get_pc(), chmplst2_pen_hi);

			break;

		default:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%06x: warning, writing to igs_magic %02x = %02x\n", activecpu_get_pc(), igs_magic[0], data);
	}
}

static READ16_HANDLER( chmplst2_magic_r )
{
	switch(igs_magic[0])
	{
		case 0x01:
			if (~igs_input_sel2 & 0x01)	return readinputport(4);
			if (~igs_input_sel2 & 0x02)	return readinputport(5);
			if (~igs_input_sel2 & 0x04)	return readinputport(6);
			if (~igs_input_sel2 & 0x08)	return readinputport(7);
			if (~igs_input_sel2 & 0x10)	return readinputport(8);
			/* fall through */
		default:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%06x: warning, reading with igs_magic = %02x\n", activecpu_get_pc(), igs_magic[0]);
			break;

		case 0x03:	return 0xff;

		/* Protection:
		   0544FE: 20 21 22 24 25 26 27 28 2A 2B 2C 2D 2E 30 31 32 33 34
		   0544EC: 49 47 53 41 41 7F 41 41 3E 41 49 F9 0A 26 49 49 49 32
		*/

		case 0x20:	return 0x49;
		case 0x21:	return 0x47;
		case 0x22:	return 0x53;

		case 0x24:	return 0x41;
		case 0x25:	return 0x41;
		case 0x26:	return 0x7f;
		case 0x27:	return 0x41;
		case 0x28:	return 0x41;

		case 0x2a:	return 0x3e;
		case 0x2b:	return 0x41;
		case 0x2c:	return 0x49;
		case 0x2d:	return 0xf9;
		case 0x2e:	return 0x0a;

		case 0x30:	return 0x26;
		case 0x31:	return 0x49;
		case 0x32:	return 0x49;
		case 0x33:	return 0x49;
		case 0x34:	return 0x32;
	}

	return 0;
}



static WRITE16_HANDLER( chindrag_magic_w )
{
	COMBINE_DATA(&igs_magic[offset]);

	if (offset == 0)
		return;

	switch(igs_magic[0])
	{

		case 0x00:
			if (ACCESSING_LSB)
				coin_counter_w(0,data & 2);

			if (data & ~0x2)
				log_cb(RETRO_LOG_DEBUG, LOGPRE "%06x: warning, unknown bits written in coin counter = %02x\n", activecpu_get_pc(), data);

			break;

		default:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%06x: warning, writing to igs_magic %02x = %02x\n", activecpu_get_pc(), igs_magic[0], data);
	}
}

static READ16_HANDLER( chindrag_magic_r )
{
	switch(igs_magic[0])
	{
		case 0x00:	return readinputport(4);
		case 0x01:	return readinputport(5);
		case 0x02:	return readinputport(6);

		case 0x20:	return 0x49;
		case 0x21:	return 0x47;
		case 0x22:	return 0x53;

		case 0x24:	return 0x41;
		case 0x25:	return 0x41;
		case 0x26:	return 0x7f;
		case 0x27:	return 0x41;
		case 0x28:	return 0x41;

		case 0x2a:	return 0x3e;
		case 0x2b:	return 0x41;
		case 0x2c:	return 0x49;
		case 0x2d:	return 0xf9;
		case 0x2e:	return 0x0a;

		case 0x30:	return 0x26;
		case 0x31:	return 0x49;
		case 0x32:	return 0x49;
		case 0x33:	return 0x49;
		case 0x34:	return 0x32;

		default:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%06x: warning, reading with igs_magic = %02x\n", activecpu_get_pc(), igs_magic[0]);
	}

	return 0;
}



static WRITE16_HANDLER( grtwall_magic_w )
{
	COMBINE_DATA(&igs_magic[offset]);

	if (offset == 0)
		return;

	switch(igs_magic[0])
	{
		case 0x02:
			if (ACCESSING_LSB)
			{
				coin_counter_w(0,data & 0x01);

				OKIM6295_set_bank_base(0, (data & 0x10) ? 0x40000 : 0);
			}

			if (data & ~0x11)
				log_cb(RETRO_LOG_DEBUG, LOGPRE "%06x: warning, unknown bits written in coin counter = %02x\n", activecpu_get_pc(), data);

			break;

		default:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%06x: warning, writing to igs_magic %02x = %02x\n", activecpu_get_pc(), igs_magic[0], data);
	}
}

static READ16_HANDLER( grtwall_magic_r )
{
	switch(igs_magic[0])
	{
		case 0x00:	return readinputport(5);

		case 0x20:	return 0x49;
		case 0x21:	return 0x47;
		case 0x22:	return 0x53;

		case 0x24:	return 0x41;
		case 0x25:	return 0x41;
		case 0x26:	return 0x7f;
		case 0x27:	return 0x41;
		case 0x28:	return 0x41;

		case 0x2a:	return 0x3e;
		case 0x2b:	return 0x41;
		case 0x2c:	return 0x49;
		case 0x2d:	return 0xf9;
		case 0x2e:	return 0x0a;

		case 0x30:	return 0x26;
		case 0x31:	return 0x49;
		case 0x32:	return 0x49;
		case 0x33:	return 0x49;
		case 0x34:	return 0x32;

		default:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%06x: warning, reading with igs_magic = %02x\n", activecpu_get_pc(), igs_magic[0]);
	}

	return 0;
}



static WRITE16_HANDLER( lhb_okibank_w )
{
	if (ACCESSING_MSB)
	{
		OKIM6295_set_bank_base(0, (data & 0x200) ? 0x40000 : 0);
	}

	if ( data & (~0x200) )
		log_cb(RETRO_LOG_DEBUG, LOGPRE "%06x: warning, unknown bits written in oki bank = %02x\n", activecpu_get_pc(), data);

}

static WRITE16_HANDLER( lhb_input2_w )
{
	COMBINE_DATA(&igs_input_sel2);

	if (ACCESSING_LSB)
	{
		coin_counter_w(0,			 data & 0x20	);
		/*  coin out                 data & 0x40
		    pay out?                 data & 0x80
		*/
	}

	if ( igs_input_sel2 & (~0x7f) )
		log_cb(RETRO_LOG_DEBUG, LOGPRE "%06x: warning, unknown bits written in igs_input_sel2 = %02x\n", activecpu_get_pc(), igs_input_sel2);

}

static READ16_HANDLER( lhb_input2_r )
{
	switch(offset)
	{
		case 0:		return igs_input_sel2;

		case 1:
			if (~igs_input_sel2 & 0x01)	return readinputport(6);
			if (~igs_input_sel2 & 0x02)	return readinputport(7);
			if (~igs_input_sel2 & 0x04)	return readinputport(8);
			if (~igs_input_sel2 & 0x08)	return readinputport(9);
			if (~igs_input_sel2 & 0x10)	return readinputport(10);

			log_cb(RETRO_LOG_DEBUG, LOGPRE "%06x: warning, reading with igs_input_sel2 = %02x\n", activecpu_get_pc(), igs_input_sel2);
			break;
	}
	return 0;
}



static WRITE16_HANDLER( vbowl_magic_w )
{
	COMBINE_DATA(&igs_magic[offset]);

	if (offset == 0)
		return;

	switch(igs_magic[0])
	{
		case 0x02:
			if (ACCESSING_LSB)
			{
				coin_counter_w(0,data & 1);
				coin_counter_w(1,data & 2);
			}

			if (data & ~0x3)
				log_cb(RETRO_LOG_DEBUG, LOGPRE "%06x: warning, unknown bits written in coin counter = %02x\n", activecpu_get_pc(), data);

			break;

		default:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%06x: warning, writing to igs_magic %02x = %02x\n", activecpu_get_pc(), igs_magic[0], data);
	}
}

static READ16_HANDLER( vbowl_magic_r )
{
	switch(igs_magic[0])
	{
		case 0x00:	return readinputport(5);
		case 0x01:	return readinputport(6);

		case 0x20:	return 0x49;
		case 0x21:	return 0x47;
		case 0x22:	return 0x53;

		case 0x24:	return 0x41;
		case 0x25:	return 0x41;
		case 0x26:	return 0x7f;
		case 0x27:	return 0x41;
		case 0x28:	return 0x41;

		case 0x2a:	return 0x3e;
		case 0x2b:	return 0x41;
		case 0x2c:	return 0x49;
		case 0x2d:	return 0xf9;
		case 0x2e:	return 0x0a;

		case 0x30:	return 0x26;
		case 0x31:	return 0x49;
		case 0x32:	return 0x49;
		case 0x33:	return 0x49;
		case 0x34:	return 0x32;

		default:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%06x: warning, reading with igs_magic = %02x\n", activecpu_get_pc(), igs_magic[0]);
	}

	return 0;
}



static WRITE16_HANDLER( xymg_magic_w )
{
	COMBINE_DATA(&igs_magic[offset]);

	if (offset == 0)
		return;

	switch(igs_magic[0])
	{
		case 0x01:
			COMBINE_DATA(&igs_input_sel2);

			if (ACCESSING_LSB)
			{
				coin_counter_w(0,	data & 0x20);
				/*  coin out        data & 0x40 */
			}

			if ( igs_input_sel2 & ~0x3f )
				log_cb(RETRO_LOG_DEBUG, LOGPRE "%06x: warning, unknown bits written in igs_input_sel2 = %02x\n", activecpu_get_pc(), igs_input_sel2);

			break;

		default:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%06x: warning, writing to igs_magic %02x = %02x\n", activecpu_get_pc(), igs_magic[0], data);
	}
}

static READ16_HANDLER( xymg_magic_r )
{
	switch(igs_magic[0])
	{
		case 0x00:	return readinputport(3);

		case 0x02:
			if (~igs_input_sel2 & 0x01)	return readinputport(4);
			if (~igs_input_sel2 & 0x02)	return readinputport(5);
			if (~igs_input_sel2 & 0x04)	return readinputport(6);
			if (~igs_input_sel2 & 0x08)	return readinputport(7);
			if (~igs_input_sel2 & 0x10)	return readinputport(8);
			/* fall through */
		default:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%06x: warning, reading with igs_magic = %02x\n", activecpu_get_pc(), igs_magic[0]);
			break;
	}

	return 0;
}



/***************************************************************************

    Memory Maps

***************************************************************************/

static WRITE16_HANDLER( igs_YM3812_control_port_0_w )
{
	if (ACCESSING_LSB)
		YM3812_control_port_0_w(0,data);
}

static WRITE16_HANDLER( igs_YM3812_write_port_0_w )
{
	if (ACCESSING_LSB)
		YM3812_write_port_0_w(0,data);
}

static MEMORY_READ16_START( chindrag_readmem)
    { 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x100000, 0x103fff, MRA16_RAM },
	{ 0x200000, 0x200fff, MRA16_RAM },
	{ 0x400000, 0x401fff, MRA16_RAM },
	{ 0x500000, 0x500001, input_port_3_word_r },
	{ 0x600000, 0x600001, OKIM6295_status_0_lsb_r },
	{ 0x800002, 0x800003, chindrag_magic_r },
	{ 0xa88000, 0xa88001, igs_3_input_r },
MEMORY_END

static MEMORY_WRITE16_START( chindrag_writemem )
    { 0x100000, 0x103fff, MWA16_RAM, (data16_t **)&generic_nvram, &generic_nvram_size },
	{ 0x200000, 0x200fff, MWA16_RAM, &igs_priority_ram },
	{ 0x400000, 0x401fff, igs_palette_w, &paletteram16 },
	{ 0x600000, 0x600001, OKIM6295_data_0_lsb_w },
	{ 0x700000, 0x700001, igs_YM3812_control_port_0_w },
	{ 0x700002, 0x700003, igs_YM3812_write_port_0_w },
	{ 0x800000, 0x800003, chindrag_magic_w },
	{ 0xa20000, 0xa20001, igs_priority_w },
	{ 0xa40000, 0xa40001, igs_3_input_w },
	{ 0xa58000, 0xa58001, igs_blit_x_w },
	{ 0xa58800, 0xa58801, igs_blit_y_w },
	{ 0xa59000, 0xa59001, igs_blit_w_w },
	{ 0xa59800, 0xa59801, igs_blit_h_w },
	{ 0xa5a000, 0xa5a001, igs_blit_gfx_lo_w },
	{ 0xa5a800, 0xa5a801, igs_blit_gfx_hi_w },
	{ 0xa5b000, 0xa5b001, igs_blit_flags_w },
	{ 0xa5b800, 0xa5b801, igs_blit_pen_w },
	{ 0xa5c000, 0xa5c001, igs_blit_depth_w },
MEMORY_END



static READ16_HANDLER( ics2115_0_word_r )
{
	switch(offset)
	{
		case 0:	return ics2115_r(0);
		case 1:	return ics2115_r(1);
		case 2:	return (ics2115_r(3) << 8) | ics2115_r(2);
	}
	return 0xff;
}

static WRITE16_HANDLER( ics2115_0_word_w )
{
	switch(offset)
	{
		case 1:
			if (ACCESSING_LSB)	ics2115_w(1,data);
			break;
		case 2:
			if (ACCESSING_LSB)	ics2115_w(2,data);
			if (ACCESSING_MSB)	ics2115_w(3,data>>8);
			break;
	}
}

static WRITE16_HANDLER( vbowl_unk_w )
{
}

static READ16_HANDLER( vbowl_unk_r )
{
	return 0xffff;
}

static UINT16 *vbowl_trackball;
static VIDEO_EOF( vbowl )
{
	vbowl_trackball[0] = vbowl_trackball[1];
	vbowl_trackball[1] = (readinputport(8) << 8) | readinputport(7);
}

static WRITE16_HANDLER( vbowl_pen_hi_w )
{
	if (ACCESSING_LSB)
	{
		chmplst2_pen_hi = data & 0x07;
	}

	if (data & ~0x7)
		log_cb(RETRO_LOG_DEBUG, LOGPRE "%06x: warning, unknown bits written to pen_hi = %04x\n", activecpu_get_pc(), igs_priority);
}

static WRITE16_HANDLER( vbowl_link_0_w )	{ }
static WRITE16_HANDLER( vbowl_link_1_w )	{ }
static WRITE16_HANDLER( vbowl_link_2_w )	{ }
static WRITE16_HANDLER( vbowl_link_3_w )	{ }

static MEMORY_READ16_START( vbowl_readmem )
    { 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x100000, 0x103fff, MRA16_RAM },
	{ 0x200000, 0x200fff, MRA16_RAM },
	{ 0x300000, 0x3fffff, igs_layers_r },
	{ 0x400000, 0x401fff, MRA16_RAM },
	{ 0x520000, 0x520001, input_port_4_word_r },
	{ 0x600000, 0x600007, ics2115_0_word_r },
	{ 0x700000, 0x700003, MRA16_RAM },
	{ 0x800002, 0x800003, vbowl_magic_r },
	{ 0xa88000, 0xa88001, igs_4_input_r },
	{ 0xa80000, 0xa80001, vbowl_unk_r },
	{ 0xa90000, 0xa90001, vbowl_unk_r },
	{ 0xa98000, 0xa98001, vbowl_unk_r },
MEMORY_END

static MEMORY_WRITE16_START( vbowl_writemem )
    { 0x100000, 0x103fff, MWA16_RAM, (data16_t **)&generic_nvram, &generic_nvram_size },
	{ 0x200000, 0x200fff, MWA16_RAM, &igs_priority_ram },
	{ 0x300000, 0x3fffff, igs_layers_w },
	{ 0x400000, 0x401fff, igs_palette_w, &paletteram16 },
	{ 0x600000, 0x600007, ics2115_0_word_w },
	{ 0x700000, 0x700003, MWA16_RAM, &vbowl_trackball },
	{ 0x700004, 0x700005, vbowl_pen_hi_w },
	{ 0x800000, 0x800003, vbowl_magic_w },
	{ 0xa00000, 0xa00001, vbowl_link_0_w },
	{ 0xa08000, 0xa08001, vbowl_link_1_w },
	{ 0xa10000, 0xa10001, vbowl_link_2_w },
	{ 0xa18000, 0xa18001, vbowl_link_3_w },
	{ 0xa20000, 0xa20001, igs_priority_w },
	{ 0xa40000, 0xa40001, igs_4_input_w },
	{ 0xa58000, 0xa58001, igs_blit_x_w },
	{ 0xa58800, 0xa58801, igs_blit_y_w },
	{ 0xa59000, 0xa59001, igs_blit_w_w },
	{ 0xa59800, 0xa59801, igs_blit_h_w },
	{ 0xa5a000, 0xa5a001, igs_blit_gfx_lo_w },
	{ 0xa5a800, 0xa5a801, igs_blit_gfx_hi_w },
	{ 0xa5b000, 0xa5b001, igs_blit_flags_w },
	{ 0xa5b800, 0xa5b801, igs_blit_pen_w },
	{ 0xa5c000, 0xa5c001, igs_blit_depth_w },
MEMORY_END


/***************************************************************************

    Input Ports

***************************************************************************/

INPUT_PORTS_START( drgnwrld )
	PORT_START	/* IN0 - DSW1 */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x18, "Normal" )
	PORT_DIPSETTING(    0x10, "Hard" )
	PORT_DIPSETTING(    0x08, "Harder" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* IN1 - DSW2 */
	PORT_DIPNAME( 0x01, 0x01, "Open Girl?" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Background" )
	PORT_DIPSETTING(    0x02, "Girl" )
	PORT_DIPSETTING(    0x00, "Landscape" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Bang Turtle?" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Send Boom?" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Test?" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

  	PORT_START	/* IN2 - DSW3 */
	PORT_DIPNAME( 0xff, 0xff, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0xff, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE ) /* keep pressed while booting */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )	/* press in girl test to pause, button 3 advances */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN5 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )

	PORT_START	/* IN6 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
INPUT_PORTS_END

INPUT_PORTS_START( vbowl )
	PORT_START  /* IN0 - DSW1 */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Sexy Interlude" ) /* Sexy Interlude pics */
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x20, "Open Picture" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x40, 0x40, "Controls" )
	PORT_DIPSETTING(    0x40, "Joystick" )
	PORT_DIPSETTING(    0x00, "Trackball" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START  /* IN1 - DSW2 */
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, "Easy" )
	PORT_DIPSETTING(    0x02, "Normal" )
	PORT_DIPSETTING(    0x01, "Medium" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x04, 0x04, "Spares To Win (Frames 1-5)" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x18, 0x18, "Points To Win (Frames 6-10)" )
	PORT_DIPSETTING(    0x18, "160" )
	PORT_DIPSETTING(    0x10, "170" )
	PORT_DIPSETTING(    0x08, "180" )
	PORT_DIPSETTING(    0x00, "190" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

  	PORT_START  /* IN2 - DSW3 */
	PORT_DIPNAME( 0x03, 0x03, "Cabinet ID" )
	PORT_DIPSETTING(    0x03, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x04, 0x04, "Linked Cabinets" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START  /* IN3 - DSW4 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START   /* IN4 */ 
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START   /* IN5 */ 
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON3 )

	PORT_START   /* IN6 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )

	PORT_START   /* IN7 */ 
    PORT_ANALOG( 0xff, 0x00, IPT_TRACKBALL_X | IPF_PLAYER1, 30, 30, 0xff, 0x00)

	PORT_START   /* IN8 */
    PORT_ANALOG( 0xff, 0x00, IPT_TRACKBALL_Y | IPF_PLAYER1, 30, 30, 0xff, 0x00)
INPUT_PORTS_END


INPUT_PORTS_START( vbowlj )
	PORT_START	/* IN0 - DSW1 */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Sexy Interlude" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x20, "Controls" )
	PORT_DIPSETTING(    0x20, "Joystick" )
	PORT_DIPSETTING(    0x00, "Trackball" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* IN1 - DSW2 */
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, "Easy" )
	PORT_DIPSETTING(    0x02, "Normal" )
	PORT_DIPSETTING(    0x01, "Medium" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x04, 0x04, "Spares To Win (Frames 1-5)" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x18, 0x18, "Points To Win (Frames 6-10)" )
	PORT_DIPSETTING(    0x18, "160" )
	PORT_DIPSETTING(    0x10, "170" )
	PORT_DIPSETTING(    0x08, "180" )
	PORT_DIPSETTING(    0x00, "190" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

  	PORT_START	/* IN2 - DSW3 */
	PORT_DIPNAME( 0x03, 0x03, "Cabinet ID" )
	PORT_DIPSETTING(    0x03, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x04, 0x04, "Linked Cabinets" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	/*  IN3 - DSW4 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN5 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON3 )

	PORT_START	/* IN6 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )

	PORT_START	/* IN7 */
    PORT_ANALOG( 0xff, 0x00, IPT_TRACKBALL_X | IPF_PLAYER1, 30, 30, 0xff, 0x00)

	PORT_START	/* IN8 */
    PORT_ANALOG( 0xff, 0x00, IPT_TRACKBALL_Y | IPF_PLAYER1, 30, 30, 0xff, 0x00)
INPUT_PORTS_END



/***************************************************************************

    Machine Drivers

***************************************************************************/

/* for debugging

static const gfx_layout gfxlayout_8x8x4 =
{
    8,8,
    RGN_FRAC(1,1),
    4,
    { STEP4(0,1) },
    { 4, 0, 12,  8, 20,16, 28,24 },
    { STEP8(0,8*4) },
    8*8*4
};
static const gfx_layout gfxlayout_16x16x4 =
{
    16,16,
    RGN_FRAC(1,1),
    4,
    { STEP4(0,1) },
    {  4, 0, 12, 8, 20,16, 28,24,
      36,32, 44,40, 52,48, 60,56    },
    { STEP16(0,16*4) },
    16*16*4
};
static const gfx_layout gfxlayout_8x8x8 =
{
    8,8,
    RGN_FRAC(1,1),
    8,
    { STEP8(0,1) },
    { STEP8(0,8) },
    { STEP8(0,8*8) },
    8*8*8
};
static const gfx_layout gfxlayout_16x16x8 =
{
    16,16,
    RGN_FRAC(1,1),
    8,
    { STEP8(0,1) },
    { STEP16(0,8) },
    { STEP16(0,16*8) },
    16*16*8
};
static const gfx_layout gfxlayout_16x16x1 =
{
    16,16,
    RGN_FRAC(1,1),
    1,
    { 0 },
    { STEP16(15,-1) },
    { STEP16(0,16*1) },
    16*16*1
};

static const gfx_decode gfxdecodeinfo[] =
{
    { REGION_GFX1, 0, &gfxlayout_8x8x4,   0, 0x80  },
    { REGION_GFX1, 0, &gfxlayout_16x16x4, 0, 0x80  },
    { REGION_GFX1, 0, &gfxlayout_8x8x8,   0, 0x08  },
    { REGION_GFX1, 0, &gfxlayout_16x16x8, 0, 0x08  },
    { -1 }
};
static const gfx_decode gfxdecodeinfo_chmplst2[] =
{
    { REGION_GFX1, 0, &gfxlayout_8x8x4,   0, 0x80  },
    { REGION_GFX1, 0, &gfxlayout_16x16x4, 0, 0x80  },
    { REGION_GFX1, 0, &gfxlayout_8x8x8,   0, 0x08  },
    { REGION_GFX1, 0, &gfxlayout_16x16x8, 0, 0x08  },
    { REGION_GFX2, 0, &gfxlayout_16x16x1, 0, 0x80  },
    { -1 }
};
*/

static struct OKIM6295interface okim6295_interface =
{
	1,              /* 1 chips */
	{ 1047600/132 },/* Frequency */
	{ REGION_SOUND1 },
	{ 60 },
};


static INTERRUPT_GEN( chmplst2_interrupt )
{
	switch (cpu_getiloops())
	{
		case 0:	cpu_set_irq_line(0, 6, HOLD_LINE);	break;
		default:
		case 1:	cpu_set_irq_line(0, 5, HOLD_LINE);	break;
	}
}



static struct YM3812interface ym3812_interface =
{
	1,				/* 1 chips */
	3579545,		/* ? */
	{ 100 },
	{ 0 },
};


static void sound_irq(int state)
{
/*  cpu_set_irq_line(0, 3, state); */
} 


static struct ics2115_interface pgm_ics2115_interface = {
	{ MIXER(100, MIXER_PAN_LEFT), MIXER(100, MIXER_PAN_RIGHT) },
	REGION_SOUND1,
	/* sound_irq */
};



static MACHINE_DRIVER_START( chindrag )
	MDRV_CPU_ADD(M68000, 22000000/3)
	MDRV_CPU_MEMORY(chindrag_readmem,chindrag_writemem)
	MDRV_CPU_VBLANK_INT(chmplst2_interrupt,1+4)	/* lev5 frequency drives the music tempo */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_NVRAM_HANDLER(generic_0fill)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 256)
	MDRV_VISIBLE_AREA(0, 512-1, 0, 240-1)
	/*  MDRV_GFXDECODE(gfxdecodeinfo) */
	MDRV_PALETTE_LENGTH(0x800)

	MDRV_VIDEO_START( igs )
	MDRV_VIDEO_UPDATE( igs )

	/* sound hardware */
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
    MDRV_SOUND_ADD(YM3812, ym3812_interface)
MACHINE_DRIVER_END

	
	

static INTERRUPT_GEN( vbowl_interrupt )
{
	switch (cpu_getiloops())
	{
		case 0:	cpu_set_irq_line(0, 4, HOLD_LINE);	break;
		case 1:	cpu_set_irq_line(0, 5, HOLD_LINE);	break;
		case 2:	cpu_set_irq_line(0, 6, HOLD_LINE);	break;
		default:
		case 3:	cpu_set_irq_line(0, 3, HOLD_LINE);	break;	/* sound */
	}
}


static MACHINE_DRIVER_START( vbowl )
	MDRV_CPU_ADD(M68000, 22000000/3)
	MDRV_CPU_MEMORY(vbowl_readmem,vbowl_writemem)
	MDRV_CPU_VBLANK_INT(vbowl_interrupt,3+4)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_NVRAM_HANDLER(generic_0fill)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 256)
	MDRV_VISIBLE_AREA(0, 512-1, 0, 240-1)
	/*  MDRV_GFXDECODE(gfxdecodeinfo) */
	MDRV_PALETTE_LENGTH(0x800)

	MDRV_VIDEO_START( igs )
	MDRV_VIDEO_UPDATE( igs )
	MDRV_VIDEO_EOF(vbowl)	/* trackball */

	/* sound hardware */
	MDRV_SOUND_ADD(ICS2115, pgm_ics2115_interface)
MACHINE_DRIVER_END


static DRIVER_INIT( chindrag )
{
	UINT16 *rom = (UINT16 *) memory_region(REGION_CPU1);

	chindrag_decrypt();
	chindrag_gfx_decrypt();

	/* PROTECTION CHECKS */
	rom[0x033d2/2]	=	0x606c;
	rom[0x11c74/2]	=	0x606c;
	rom[0x23d2a/2]	=	0x606c;
	rom[0x23f68/2]	=	0x606c;
	rom[0x240d4/2]	=	0x606c;
	rom[0x242ac/2]	=	0x606c;
	rom[0x244b2/2]	=	0x606c;
	rom[0x24630/2]	=	0x606c;
	rom[0x24886/2]	=	0x606c;
	rom[0x24aca/2]	=	0x606c;
	rom[0x24d46/2]	=	0x606c;
	rom[0x24f8e/2]	=	0x606c;
	rom[0x254ba/2]	=	0x6000;
	rom[0x26a52/2]	=	0x606c;
	rom[0x276aa/2]	=	0x606c;
	rom[0x2a870/2]	=	0x606c;
}

static DRIVER_INIT( drgnwrld )
{
	UINT16 *rom = (UINT16 *) memory_region(REGION_CPU1);

	drgnwrld_decrypt();
	chindrag_gfx_decrypt();

	/* PROTECTION CHECKS */
	rom[0x032ee/2]	=	0x606c;
	rom[0x23d5e/2]	=	0x606c;
	rom[0x23fd0/2]	=	0x606c;
	rom[0x24170/2]	=	0x606c;
	rom[0x24348/2]	=	0x606c;
	rom[0x2454e/2]	=	0x606c;
	rom[0x246cc/2]	=	0x606c;
	rom[0x24922/2]	=	0x606c;
	rom[0x24b66/2]	=	0x606c;
	rom[0x24de2/2]	=	0x606c;
	rom[0x2502a/2]	=	0x606c;
	rom[0x25556/2]	=	0x6000;
	rom[0x2a16c/2]	=	0x606c;
}



DRIVER_INIT( vbowl )
{
    UINT16 *rom = (UINT16 *) memory_region(REGION_CPU1);
	UINT8  *gfx = (UINT8 *)  memory_region(REGION_GFX1);
	int i;

	vbowlj_decrypt();

	for (i = 0x400000-1; i >= 0; i--)
	{
		gfx[i * 2 + 1] = (gfx[i] & 0xf0) >> 4;
		gfx[i * 2 + 0] = (gfx[i] & 0x0f) >> 0;
	}

	/* Patch the bad dump so that it doesn't reboot at the end of a game (the patched value is from vbowlj) */
	rom[0x080e0/2] = 0xe549;

	
	/* PROTECTION CHECKS */
	rom[0x3764/2] = 0x4e75;

}

DRIVER_INIT( vbowlj )
{
	UINT16 *rom = (UINT16 *) memory_region(REGION_CPU1);
	UINT8  *gfx = (UINT8 *)  memory_region(REGION_GFX1);
	int i;

	vbowlj_decrypt();

	for (i = 0x400000-1; i >= 0; i--)
	{
		gfx[i * 2 + 1] = (gfx[i] & 0xf0) >> 4;
		gfx[i * 2 + 0] = (gfx[i] & 0x0f) >> 0;
	}

	/* PROTECTION CHECKS */
	rom[0x37b4/2]	=	0x4e75;
}



/***************************************************************************

    ROMs Loading

***************************************************************************/

ROM_START( drgnwrld )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "chinadr-v0400.bin", 0x00000, 0x80000, CRC(a6daa2b8) SHA1(0cbfd001c1fd82a6385453d1c2a808add67746af) )

	ROM_REGION( 0x420000, REGION_GFX1, 0 )
	ROM_LOAD( "d0301", 0x000000, 0x400000, CRC(78ab45d9) SHA1(c326ee9f150d766edd6886075c94dea3691b606d) )
	ROM_LOAD( "cg",    0x400000, 0x020000, CRC(2dda0be3) SHA1(587b7cab747d4336515c98eb3365341bb6c7e5e4) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )
	ROM_LOAD( "sp", 0x00000, 0x40000, CRC(fde63ce1) SHA1(cc32d2cace319fe4d5d0aa96d7addb2d1def62f2) )
ROM_END


ROM_START( vbowl )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )
	ROM_LOAD( "bowlingv101xcm.u45", 0x00000, 0x80000, CRC(ab8e3f1f) SHA1(69159e22559d6a26fe2afafd770aa640c192ba4b) )

	ROM_REGION( 0x800000, REGION_GFX1, 0)
	ROM_LOAD( "vrbowlng.u69", 0x000000, 0x400000, CRC(b0d339e8) SHA1(a26a5e0202a78e8cdc562b10d64e14eadfa4e115) )
	/* extra space to expand every 4 bits to 8 */

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_INVERT )
	ROM_LOAD( "vrbowlng.u68", 0x000000, 0x100000, CRC(b0ce27e7) SHA1(6d3ef97edd606f384b1e05b152fbea12714887b7) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )
	ROM_LOAD( "vrbowlng.u67", 0x00000, 0x80000, CRC(53000936) SHA1(e50c6216f559a9248c095bdfae05c3be4be79ff3) )	/* 8 bit signed mono & u-law */
	ROM_LOAD( "vrbowlng.u66", 0x80000, 0x80000, CRC(f62cf8ed) SHA1(c53e47e2c619ed974ad40ee4aaa4a35147ea8311) )	/* 8 bit signed mono */
	ROM_COPY( REGION_SOUND1, 0, 0x100000,0x100000)
	ROM_COPY( REGION_SOUND1, 0, 0x200000,0x100000)
	ROM_COPY( REGION_SOUND1, 0, 0x300000,0x100000)
ROM_END

ROM_START( vbowlj )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )
	ROM_LOAD( "vrbowlng.u45", 0x00000, 0x80000, CRC(091c19c1) SHA1(5a7bfbee357122e9061b38dfe988c3853b0984b0) ) /* second half all 00 */

	ROM_REGION( 0x800000, REGION_GFX1, 0)
	ROM_LOAD( "vrbowlng.u69", 0x000000, 0x400000, CRC(b0d339e8) SHA1(a26a5e0202a78e8cdc562b10d64e14eadfa4e115) )
	/* extra space to expand every 4 bits to 8 */

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_INVERT )
	ROM_LOAD( "vrbowlng.u68", 0x000000, 0x100000, CRC(b0ce27e7) SHA1(6d3ef97edd606f384b1e05b152fbea12714887b7) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )
	ROM_LOAD( "vrbowlng.u67", 0x00000, 0x80000, CRC(53000936) SHA1(e50c6216f559a9248c095bdfae05c3be4be79ff3) )	/* 8 bit signed mono & u-law */
	ROM_LOAD( "vrbowlng.u66", 0x80000, 0x80000, CRC(f62cf8ed) SHA1(c53e47e2c619ed974ad40ee4aaa4a35147ea8311) )	/* 8 bit signed mono */
	ROM_COPY( REGION_SOUND1, 0, 0x100000,0x100000)
	ROM_COPY( REGION_SOUND1, 0, 0x200000,0x100000)
	ROM_COPY( REGION_SOUND1, 0, 0x300000,0x100000)
ROM_END



/***************************************************************************

    Game Drivers

***************************************************************************/

GAME(1997, drgnwrld, 0,       chindrag, drgnwrld, drgnwrld, ROT0, "IGS", "Dragon World (World, V040O)" )
GAME(1996, vbowl,    0,       vbowl,    vbowl,    vbowl,    ROT0, "Alta / IGS", "Virtua Bowling (World, V101XCM )" )
GAME(1996, vbowlj,   vbowl,   vbowl,    vbowlj,   vbowlj,   ROT0, "Alta / IGS", "Virtua Bowling (Japan, V100JCM)" )
