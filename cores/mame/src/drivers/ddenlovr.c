/***************************************************************************

Some Dynax/Nakanihon games using the third version of their blitter

Driver by Nicola Salmoria based on preliminary work by Luca Elia

the driver is a complete mess ATM.

Hardware:
CPU: Z80 or 68000
Sound: (AY-3-8910) + YM2413 + MSM6295
Other: Real Time Clock (Oki MSM6242B or 72421B)

---------------------------------------------------------------------------------------------------------------------------
Year + Game					Board			CPU		Sound						Custom				Notes
---------------------------------------------------------------------------------------------------------------------------
92 Monkey Mole Panic						2xZ80	AY8910 +          M6295		Dynax NL-001		8251
93 Quiz Channel Question	N7311208L1-2	Z80		         YM2413 + M6295		NAKANIHON NL-002
94 Quiz 365									68000	AY8910 + YM2413 + M6295
94 Rong Rong								Z80		         YM2413 + M6295		NAKANIHON NL-002
95 Don Den Lover Vol 1		D11309208L1		68000	AY8910 + YM2413 + M6295		NAKANIHON NL-005
95 Nettoh Quiz Champion						68000	AY8910 + YM2413 + M6295

the following use a different blitter, will probably go in a separate driver
96 Hanakanzashi								Z80		         YM2413 + M6295
96 Hana Kagerou								Z80		         YM2413 + M6295		70C160F011
98 Mahjong Reach Ippatsu					Z80		         YM2413 + M6295		70C160F011
---------------------------------------------------------------------------------------------------------------------------

Notes:
- the zooming Dynax logo in ddenlovr would flicker because the palette is
  updated one frame after the bitmap. This is fixed using a framebuffer but I
  don't think it's correct.


TODO:
- a lot!

- ddenlovr: understand the extra commands for the blitter compressed data,
  used only by this game.

- ddenlovr: sometimes the colors of the girl in the presentation before the
  beginning of a stage are wrong, and they correct themselves when the board
  is drawn.
- The palette problems mentioned above happen in other games as well, e.g.
  quizchq attract mode.

- the registers right after the palette bank selectors (e00048-e0004f in ddenlovr)
  are not understood. They are related to the layer enable register and to the
  unknown blitter register 05.
  ddenlovr has a function at 001798 to initialize these four registers. It uses
  a table with 7 possible different layouts:
  0f 0f 3f cf
  4f 0f af 1f
  0f 0f 6f 9f
  0f 0f 0f ff
  0f 0f 7f 8f
  0f 0f cf 3f
  0f 0f 8f 7f
  the table is copied to e00048-e0004f and is also used to create a 16-entry
  array used to feed blitter register 05. Every element of the array is the OR
  of the values in the table above corresponding to bits set in the layer enable
  register. Note that in the table above the top 4 bits are split among the four
  entries.

- There's also a kludge to fix the zooming Dynax logo in ddenlovr, the palette
  base registers are all set to 0xff, so it would use the wrong palette otherwise.

- The meaning of blitter commands 43 and 8c is not understood.

- quizchq: some samples are played at the wrong pitch

- quiz365 crashes; protection?

- NVRAM

***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"


static UINT8 *pixmap[8];
static struct mame_bitmap *framebuffer;
static int extra_layers;

VIDEO_START(ddenlovr)
{
	int i;
	for (i = 0; i < 8; i++)
		if (!(pixmap[i] = auto_malloc(512*512)))	return 1;

	if (!(framebuffer = auto_bitmap_alloc(Machine->drv->screen_width,Machine->drv->screen_height))) return 1;

	extra_layers = 0;
	return 0;
}

VIDEO_START(mmpanic)
{
	if (video_start_ddenlovr())
		return 1;

	extra_layers = 1;
	return 0;
}


/***************************************************************************

						Blitter Data Format

The gfx data is a bitstream. Command size is always 3 bits, argument size
can be from 1 to 8 bits (up to 16 bits seem to be allowed, but not used).

Data starts with an 8 bit header:
7------- not used
-654---- size-1 of arguments indicating pen number (B)
----3210 size-1 of arguments indicating number of pixels (A)

The commands are:
000	Increment Y
001 Followed by A bits (N) and by B bits (P): draw N+1 pixels using pen P
010 Followed by A bits (N) and by (N+1)*B bits: copy N+1 pixels
011	Followed by A bits (N): skip N pixels
100	not used
101	Followed by 4 bits: change argument size
110	Followed by 3 bits: change pen size
111	Stop.

The drawing operation is verified (quiz365) to modify dynax_blit_y.

***************************************************************************/

static int dynax_dest_layer;
static int dynax_blit_flip;
static int dynax_blit_x;
static int dynax_blit_y;
static int dynax_blit_address;
static int dynax_blit_pen,dynax_blit_pen_mode;
static int dynax_blitter_irq_flag,dynax_blitter_irq_enable;
static int dynax_rect_width, dynax_rect_height;
static int dynax_line_length;
static int dynax_clip_ctrl,dynax_clip_x,dynax_clip_y;
static int dynax_scroll[8*2];
static int dynax_priority,dynax_priority2,dynax_bgcolor,dynax_bgcolor2;
static int dynax_layer_enable=0x0f, dynax_layer_enable2=0x0f;
static int dynax_palette_base[8];


static void do_plot(int x,int y,int pen)
{
	y &= 0x1ff;
	x &= 0x1ff;

	if (!(dynax_clip_ctrl & 1) && x < dynax_clip_x) return;
	if (!(dynax_clip_ctrl & 2) && x >= dynax_clip_x) return;
	if (!(dynax_clip_ctrl & 4) && y < dynax_clip_y) return;
	if (!(dynax_clip_ctrl & 8) && y >= dynax_clip_y) return;

	if (dynax_dest_layer & 0x0001) pixmap[0][512*y+x] = pen;
	if (dynax_dest_layer & 0x0002) pixmap[1][512*y+x] = pen;
	if (dynax_dest_layer & 0x0004) pixmap[2][512*y+x] = pen;
	if (dynax_dest_layer & 0x0008) pixmap[3][512*y+x] = pen;
	if (dynax_dest_layer & 0x0100) pixmap[4][512*y+x] = pen;
	if (dynax_dest_layer & 0x0200) pixmap[5][512*y+x] = pen;
	if (dynax_dest_layer & 0x0400) pixmap[6][512*y+x] = pen;
	if (dynax_dest_layer & 0x0800) pixmap[7][512*y+x] = pen;
}


static INLINE int fetch_bit(UINT8 *src_data,int src_len,int *bit_addr)
{
	int baddr = (*bit_addr)++;

	if (baddr/8 >= src_len)
	{
#ifdef MAME_DEBUG
		usrintf_showmessage("GFX ROM OVER %06x",baddr/8);
#endif
		return 1;
	}

	return (src_data[baddr / 8] >> (7 - (baddr & 7))) & 1;
}

static INLINE int fetch_word(UINT8 *src_data,int src_len,int *bit_addr,int word_len)
{
	int res = 0;

	while (word_len--)
	{
		res = (res << 1) | fetch_bit(src_data,src_len,bit_addr);
	}
	return res;
}



static int blit_draw(int src,int sx,int flags)
{
	UINT8 *src_data = memory_region(REGION_GFX1);
	int src_len = memory_region_length(REGION_GFX1);
	int bit_addr = src * 8;	/* convert to bit address */
	int pen_size, arg_size, cmd;
	int x;
	int xinc = (dynax_blit_flip & 1) ? -1 : 1;
	int yinc = (dynax_blit_flip & 2) ? -1 : 1;

/*if (src == 0x2a88ae) return src;*/
/*if (src == 0x2a0e7c) return src;*/
/*if (src == 0x279a4b) return src;*/
/*if (src == 0x276e11) return src;*/

	pen_size = fetch_word(src_data,src_len,&bit_addr,4) + 1;
	arg_size = fetch_word(src_data,src_len,&bit_addr,4) + 1;

#ifdef MAME_DEBUG
	if (pen_size > 4 || arg_size > 8)
		usrintf_showmessage("pen_size %d arg_size %d",pen_size,arg_size);
#endif

	x = sx;

	for (;;)
	{
		cmd = fetch_word(src_data,src_len,&bit_addr,3);
		switch (cmd)
		{
			case 0:	/* NEXT*/
				/* next line */
				dynax_blit_y += yinc;
				x = sx;
				break;

			default:
				usrintf_showmessage("%06x: unknown command %02x",src,cmd);
/*				return (bit_addr + 7) / 8;*/

			case 1:	/* LINE*/
				{
					int length = fetch_word(src_data,src_len,&bit_addr,arg_size);
					int pen = fetch_word(src_data,src_len,&bit_addr,pen_size);
					if (dynax_blit_pen_mode) pen = (dynax_blit_pen & 0x0f);
					pen |= dynax_blit_pen & 0xf0;
					while (length-- >= 0)
					{
						do_plot(x,dynax_blit_y,pen);
						x += xinc;
					}
				}
				break;

			case 2:	/* COPY*/
				{
					int length = fetch_word(src_data,src_len,&bit_addr,arg_size);
					while (length-- >= 0)
					{
						int pen = fetch_word(src_data,src_len,&bit_addr,pen_size);
						if (dynax_blit_pen_mode) pen = (dynax_blit_pen & 0x0f);
						pen |= dynax_blit_pen & 0xf0;
						do_plot(x,dynax_blit_y,pen);
						x += xinc;
					}
				}
				break;

			case 3:	/* SKIP*/
				x += fetch_word(src_data,src_len,&bit_addr,arg_size);
				break;

			case 5:	/* CHGA*/
				arg_size = fetch_word(src_data,src_len,&bit_addr,4) + 1;
				break;

			case 6:	/* CHGP*/
				pen_size = fetch_word(src_data,src_len,&bit_addr,3) + 1;
				break;

			case 7:	/* STOP*/
				return (bit_addr + 7) / 8;
		}
	}
}


static READ_HANDLER( rongrong_gfxrom_r )
{
	data8_t *rom	=	memory_region( REGION_GFX1 );
	size_t size		=	memory_region_length( REGION_GFX1 );
	int address	=	dynax_blit_address;

	if (address >= size)
	{
		address %= size;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU#0 PC %06X: Error, Blitter address %06X out of range\n", activecpu_get_pc(), address);
	}

	dynax_blit_address++;

	return rom[address];
}

static READ16_HANDLER( ddenlovr_gfxrom_r )
{
	return rongrong_gfxrom_r(offset);
}

static void blitter_w(int blitter, offs_t offset,data8_t data,int irq_vector)
{
	static int dynax_blit_reg[2];

profiler_mark(PROFILER_VIDEO);

	switch(offset)
	{
	case 0:
		dynax_blit_reg[blitter] = data;
		break;

	case 1:
		switch(dynax_blit_reg[blitter] & 0x3f)
		{
		case 0x00:
			if (blitter)	dynax_dest_layer = (dynax_dest_layer & 0x00ff) | (data<<8);
			else			dynax_dest_layer = (dynax_dest_layer & 0xff00) | (data<<0);
			break;

		case 0x02:
			dynax_blit_y = data | ((dynax_blit_reg[blitter] & 0xc0) << 2);
			break;

		case 0x03:
			dynax_blit_flip = data;
#ifdef MAME_DEBUG
if (dynax_blit_flip & 0xfc) usrintf_showmessage("dynax_blit_flip = %02x",dynax_blit_flip);
#endif
			break;

		case 0x04:
			dynax_blit_pen = data;
			break;

		case 0x05:
/*			unknown; related to the dest layer*/
			break;

		case 0x06:
			/* related to pen, can be 0 or 1 for 0x10 blitter command*/
			/* 0 = only bits 7-4 of dynax_blit_pen contain data*/
			/* 1 = bits 3-0 contain data as well*/
			dynax_blit_pen_mode = data;
			break;

		case 0x0a:
			dynax_rect_width = data | ((dynax_blit_reg[blitter] & 0xc0) << 2);
			break;

		case 0x0b:
			dynax_rect_height = data | ((dynax_blit_reg[blitter] & 0xc0) << 2);
			break;

		case 0x0c:
			dynax_line_length = data | ((dynax_blit_reg[blitter] & 0xc0) << 2);
			break;

		case 0x0d:
			dynax_blit_address = (dynax_blit_address & ~0x0000ff) | (data <<0);
			break;
		case 0x0e:
			dynax_blit_address = (dynax_blit_address & ~0x00ff00) | (data <<8);
			break;
		case 0x0f:
			dynax_blit_address = (dynax_blit_address & ~0xff0000) | (data<<16);
			break;

		case 0x14:
			dynax_blit_x = data | ((dynax_blit_reg[blitter] & 0xc0) << 2);
			break;

		case 0x16:
			dynax_clip_x = data | ((dynax_blit_reg[blitter] & 0xc0) << 2);
			break;

		case 0x17:
			dynax_clip_y = data | ((dynax_blit_reg[blitter] & 0xc0) << 2);
			break;

		case 0x18:
		case 0x19:
		case 0x1a:
		case 0x1b:
		case 0x1c:
		case 0x1d:
		case 0x1e:
		case 0x1f:
			dynax_scroll[blitter*8 + (dynax_blit_reg[blitter] & 7)] = data | ((dynax_blit_reg[blitter] & 0x40) << 2);
			break;

		case 0x20:
			dynax_clip_ctrl = data;
			break;

		case 0x24:
			{
logerror("%06x: blit src %06x x %03x y %03x flags %02x layer %02x pen %02x penmode %02x w %03x h %03x linelen %03x\n",
		activecpu_get_pc(),
		dynax_blit_address,dynax_blit_x,dynax_blit_y,data,
		dynax_dest_layer,dynax_blit_pen,dynax_blit_pen_mode,dynax_rect_width,dynax_rect_height,dynax_line_length);
				if (data == 0x1c)
				{
					/* draw a simple rectangle */
					int x,y;

#ifdef MAME_DEBUG
if (dynax_clip_ctrl != 0x0f)
	usrintf_showmessage("RECT clipx=%03x clipy=%03x ctrl=%x",dynax_clip_x,dynax_clip_y,dynax_clip_ctrl);
#endif

					for (y = 0;y <= dynax_rect_height;y++)
					{
						for (x = 0;x <= dynax_rect_width;x++)
						{
							do_plot(x+dynax_blit_x,y+dynax_blit_y,dynax_blit_pen);
						}
					}
				}
				else if (data == 0x43 || data == 0x8c)
				{
					/* unknown; these two are issued one after the other (43 then 8c)
					   8c is issued immediately after 43 has finished, without
					   changing any argument
					   initialized arguments are
					   00 dest layer
					   05 unknown, related to layer
					   14 X - always 0?
					   02 Y
					   0a width - always 0?
					   0b height
					   04 blit_pen
					   0c line_length - always 0?
					*/
if (data == 0x8c)
{
	int start = 512 * dynax_blit_y;
	int length = 512 * (dynax_rect_height+1);

#ifdef MAME_DEBUG
if (dynax_clip_ctrl != 0x0f)
	usrintf_showmessage("UNK8C clipx=%03x clipy=%03x ctrl=%x",dynax_clip_x,dynax_clip_y,dynax_clip_ctrl);
#endif

	if (start < 512*512)
	{
		if (start + length > 512*512)
			length = 512*512 - start;

		if (dynax_dest_layer & 0x0001) memset(pixmap[0] + start,dynax_blit_pen,length);
		if (dynax_dest_layer & 0x0002) memset(pixmap[1] + start,dynax_blit_pen,length);
		if (dynax_dest_layer & 0x0004) memset(pixmap[2] + start,dynax_blit_pen,length);
		if (dynax_dest_layer & 0x0008) memset(pixmap[3] + start,dynax_blit_pen,length);
		if (dynax_dest_layer & 0x0100) memset(pixmap[4] + start,dynax_blit_pen,length);
		if (dynax_dest_layer & 0x0200) memset(pixmap[5] + start,dynax_blit_pen,length);
		if (dynax_dest_layer & 0x0400) memset(pixmap[6] + start,dynax_blit_pen,length);
		if (dynax_dest_layer & 0x0800) memset(pixmap[7] + start,dynax_blit_pen,length);
	}
}
				}
				else if ((data == 0x04) || (data == 0x14))
				{
					/* fill from (X,Y) to end of pixmap
					   initialized arguments are
					   00 dest layer
					   05 unknown, related to layer
					   14 X
					   02 Y
					   04 blit_pen
					*/
					int start = (data == 0x04) ? 0 : (512*dynax_blit_y + dynax_blit_x);

#ifdef MAME_DEBUG
if (dynax_blit_x || dynax_blit_y)
	usrintf_showmessage("FILL command X %03x Y %03x",dynax_blit_x,dynax_blit_y);
#endif

					if (dynax_dest_layer & 0x0001) memset(pixmap[0] + start,dynax_blit_pen,512*512 - start);
					if (dynax_dest_layer & 0x0002) memset(pixmap[1] + start,dynax_blit_pen,512*512 - start);
					if (dynax_dest_layer & 0x0004) memset(pixmap[2] + start,dynax_blit_pen,512*512 - start);
					if (dynax_dest_layer & 0x0008) memset(pixmap[3] + start,dynax_blit_pen,512*512 - start);
					if (dynax_dest_layer & 0x0100) memset(pixmap[4] + start,dynax_blit_pen,512*512 - start);
					if (dynax_dest_layer & 0x0200) memset(pixmap[5] + start,dynax_blit_pen,512*512 - start);
					if (dynax_dest_layer & 0x0400) memset(pixmap[6] + start,dynax_blit_pen,512*512 - start);
					if (dynax_dest_layer & 0x0800) memset(pixmap[7] + start,dynax_blit_pen,512*512 - start);
				}
				else if (data == 0x13)
				{
					/* draw horizontal line
					   initialized arguments are
					   00 dest layer
					   05 unknown, related to layer
					   14 X
					   02 Y
					   0c line length
					   04 blit_pen
					   dynax_blit_x and dynax_blit_y are left pointing to the last pixel at the end of the command
					*/

#ifdef MAME_DEBUG
usrintf_showmessage("LINE X");
if (dynax_clip_ctrl != 0x0f)
	usrintf_showmessage("LINE X clipx=%03x clipy=%03x ctrl=%x",dynax_clip_x,dynax_clip_y,dynax_clip_ctrl);
if (dynax_blit_flip)
	usrintf_showmessage("LINE X flip=%x",dynax_blit_flip);
#endif

					while (dynax_line_length--)
						do_plot(dynax_blit_x++,dynax_blit_y,dynax_blit_pen);
				}
				else if (data == 0x1b)
				{
					/* draw vertical line
					   initialized arguments are
					   00 dest layer
					   05 unknown, related to layer
					   14 X
					   02 Y
					   0c line length
					   04 blit_pen
					   dynax_blit_x and dynax_blit_y are left pointing to the last pixel at the end of the command
					*/

#ifdef MAME_DEBUG
usrintf_showmessage("LINE Y");
if (dynax_clip_ctrl != 0x0f)
	usrintf_showmessage("LINE Y clipx=%03x clipy=%03x ctrl=%x",dynax_clip_x,dynax_clip_y,dynax_clip_ctrl);
#endif

					while (dynax_line_length--)
						do_plot(dynax_blit_x,dynax_blit_y++,dynax_blit_pen);
				}
				else if (data == 0x10)
				{
					/* copy from ROM
					   initialized arguments are
					   0D/0E/0F source data pointer
					   14 X
					   02 Y
					   00 dest layer
					   05 unknown, related to layer
					   04 blit_pen
					   06 blit_pen_mode (replace values stored in ROM)
					 */
					dynax_blit_address = blit_draw(dynax_blit_address,dynax_blit_x,data);
				}
				else
				{
#ifdef MAME_DEBUG
usrintf_showmessage("unknown blitter command %02x",data);
#endif
				}

				if (irq_vector)
				{
					/* quizchq */
					cpu_set_irq_line_and_vector(0, 0, HOLD_LINE, irq_vector);
				}
				else
				{
					/* ddenlovr */
					if (dynax_blitter_irq_enable)
					{
						dynax_blitter_irq_flag = 1;
						cpu_set_irq_line(0,1,HOLD_LINE);
					}
				}
			}
			break;

		default:
log_cb(RETRO_LOG_DEBUG, LOGPRE "%06x: Blitter %d reg %02x = %02x\n", activecpu_get_pc(), blitter, dynax_blit_reg[blitter], data);
			break;
		}
	}

profiler_mark(PROFILER_END);
}

static WRITE_HANDLER( rongrong_blitter_w )
{
	blitter_w(0,offset,data,0xf8);
}

static WRITE16_HANDLER( ddenlovr_blitter_w )
{
	if (ACCESSING_LSB)
		blitter_w(0,offset,data & 0xff,0);
}


static WRITE16_HANDLER( ddenlovr_blitter_irq_ack_w )
{
	if (ACCESSING_LSB)
	{
		if (data & 1)
		{
			dynax_blitter_irq_enable = 1;
		}
		else
		{
			dynax_blitter_irq_enable = 0;
			dynax_blitter_irq_flag = 0;
		}
	}
}


static WRITE_HANDLER( dynax_bgcolor_w )
{
	dynax_bgcolor = data;
}

static WRITE_HANDLER( dynax_bgcolor2_w )
{
	dynax_bgcolor2 = data;
}

static WRITE16_HANDLER( ddenlovr_bgcolor_w )
{
	if (ACCESSING_LSB)
		dynax_bgcolor_w(offset,data);
}


static WRITE_HANDLER( dynax_priority_w )
{
	dynax_priority = data;
}

static WRITE_HANDLER( dynax_priority2_w )
{
	dynax_priority2 = data;
}

static WRITE16_HANDLER( ddenlovr_priority_w )
{
	if (ACCESSING_LSB)
		dynax_priority_w(offset,data);
}


static WRITE_HANDLER( dynax_layer_enable_w )
{
	dynax_layer_enable = data;
}

static WRITE_HANDLER( dynax_layer_enable2_w )
{
	dynax_layer_enable2 = data;
}


static WRITE16_HANDLER( ddenlovr_layer_enable_w )
{
	if (ACCESSING_LSB)
		dynax_layer_enable_w(offset,data);
}



static void copylayer(struct mame_bitmap *bitmap,const struct rectangle *cliprect,int layer)
{
	int x,y;
	int scrollx = dynax_scroll[layer/4*8 + (layer%4) + 0];
	int scrolly = dynax_scroll[layer/4*8 + (layer%4) + 4];

	int palbase = dynax_palette_base[layer];

if (palbase == 0xff) palbase = 0;	/* workaround for ddenlovr dynax logo*/

	if (((dynax_layer_enable2 << 4) | dynax_layer_enable) & (1 << layer))
	{
		for (y = cliprect->min_y;y <= cliprect->max_y;y++)
		{
			for (x = cliprect->min_x;x <= cliprect->max_x;x++)
			{
				int pen = pixmap[layer][512 * ((y + scrolly) & 0x1ff) + ((x + scrollx) & 0x1ff)];
				if (pen & 0x0f)
				{
					((UINT16 *)bitmap->line[y])[x] = pen | palbase;
				}
			}
		}
	}
}

VIDEO_UPDATE(ddenlovr)
{
#if 0
	static int base = 0x24bbed;/*0x294a82;*/ /*0x27c47a;*/ /* = 0x279a4b;*/ /*0x2a0e7c;*/ /*0x2a88ae;*/ /*0x24bbed;*/

	int next;
	memset(pixmap[0],0,512*512);
	memset(pixmap[1],0,512*512);
	memset(pixmap[2],0,512*512);
	memset(pixmap[3],0,512*512);
	dynax_dest_layer = 8;
	dynax_blit_pen = 0;
	dynax_blit_pen_mode = 0;
	dynax_blit_y = 5;
	dynax_clip_ctrl = 0x0f;
	next = blit_draw(base,0,0);
if (keyboard_pressed(KEYCODE_Z))
	usrintf_showmessage("%06x",base);
	if (keyboard_pressed(KEYCODE_S)) base = next;
	if (keyboard_pressed_memory(KEYCODE_X)) base = next;
	if (keyboard_pressed(KEYCODE_C)) { base--; while ((memory_region(REGION_GFX1)[base] & 0xf0) != 0x30) base--; }
	if (keyboard_pressed(KEYCODE_V)) { base++; while ((memory_region(REGION_GFX1)[base] & 0xf0) != 0x30) base++; }
	if (keyboard_pressed_memory(KEYCODE_D)) { base--; while ((memory_region(REGION_GFX1)[base] & 0xf0) != 0x30) base--; }
	if (keyboard_pressed_memory(KEYCODE_F)) { base++; while ((memory_region(REGION_GFX1)[base] & 0xf0) != 0x30) base++; }
#endif

	copybitmap(bitmap,framebuffer,0,0,0,0,cliprect,TRANSPARENCY_NONE,0);

	/*
		technically, I should do the following in a eof handler, to avoid
		palette/gfx synchronization issues with frameskipping
	 */
	{
		static int order[24][4] =
		{
			{ 3,2,1,0 }, { 2,3,1,0 }, { 3,1,2,0 }, { 1,3,2,0 }, { 2,1,3,0 }, { 1,2,3,0 },
			{ 3,2,0,1 }, { 2,3,0,1 }, { 3,0,2,1 }, { 0,3,2,1 }, { 2,0,3,1 }, { 0,2,3,1 },
			{ 3,1,0,2 }, { 1,3,0,2 }, { 3,0,1,2 }, { 0,3,1,2 }, { 1,0,3,2 }, { 0,1,3,2 },
			{ 2,1,0,3 }, { 1,2,0,3 }, { 2,0,1,3 }, { 0,2,1,3 }, { 1,0,2,3 }, { 0,1,2,3 }
		};

		int pri = dynax_priority;

		if (pri >= 24)
		{
			usrintf_showmessage("priority = %02x",pri);
			pri = 0;
		}
		fillbitmap(framebuffer,dynax_bgcolor,&Machine->visible_area);

/*if (!keyboard_pressed(KEYCODE_Q))*/
		copylayer(framebuffer,&Machine->visible_area,order[pri][0]);
/*if (!keyboard_pressed(KEYCODE_W))*/
		copylayer(framebuffer,&Machine->visible_area,order[pri][1]);
/*if (!keyboard_pressed(KEYCODE_E))*/
		copylayer(framebuffer,&Machine->visible_area,order[pri][2]);
/*if (!keyboard_pressed(KEYCODE_R))*/
		copylayer(framebuffer,&Machine->visible_area,order[pri][3]);

		if (extra_layers)
		{

		pri = dynax_priority2;
		if (pri >= 24)
		{
			usrintf_showmessage("priority2 = %02x",pri);
			pri = 0;
		}
/*if (!keyboard_pressed(KEYCODE_A))*/
		copylayer(framebuffer,&Machine->visible_area,order[pri][0]+4);
/*if (!keyboard_pressed(KEYCODE_S))*/
		copylayer(framebuffer,&Machine->visible_area,order[pri][1]+4);
/*if (!keyboard_pressed(KEYCODE_D))*/
		copylayer(framebuffer,&Machine->visible_area,order[pri][2]+4);
/*if (!keyboard_pressed(KEYCODE_F))*/
		copylayer(framebuffer,&Machine->visible_area,order[pri][3]+4);
		}
	}
}



READ16_HANDLER( ddenlovr_special_r )
{
	int res = readinputport(2) | (dynax_blitter_irq_flag << 6);

	return res;
}

static WRITE16_HANDLER( ddenlovr_coincounter_0_w )
{
	if (ACCESSING_LSB)
		coin_counter_w(0, data & 1);
}
static WRITE16_HANDLER( ddenlovr_coincounter_1_w )
{
	if (ACCESSING_LSB)
		coin_counter_w(1, data & 1);
}


static data8_t palram[0x200];

static WRITE_HANDLER( rongrong_palette_w )
{
	int r,g,b,d1,d2,indx;

	palram[offset] = data;

	indx = ((offset & 0x1e0) >> 1) | (offset & 0x00f);
	d1 = palram[offset & ~0x10];
	d2 = palram[offset |  0x10];

	r = d1 & 0x1f;
	g = d2 & 0x1f;
	/* what were they smoking??? */
	b = ((d1 & 0xe0) >> 5) | (d2 & 0xc0) >> 3;

	r =  (r << 3) | (r >> 2);
	g =  (g << 3) | (g >> 2);
	b =  (b << 3) | (b >> 2);

	palette_set_color(indx,r,g,b);
}

static WRITE16_HANDLER( ddenlovr_palette_w )
{
	if (ACCESSING_LSB)
		rongrong_palette_w(offset,data & 0xff);
}


static WRITE_HANDLER( rongrong_palette_base_w )
{
	dynax_palette_base[offset] = data;
}

static WRITE_HANDLER( dynax_palette_base2_w )
{
	dynax_palette_base[offset+4] = data;
}


static WRITE16_HANDLER( ddenlovr_palette_base_w )
{
	if (ACCESSING_LSB)
		dynax_palette_base[offset] = data & 0xff;
}


static WRITE_HANDLER( quizchq_oki_bank_w )
{
	OKIM6295_set_bank_base(0, (data & 1) * 0x40000);
}

static WRITE16_HANDLER( ddenlovr_oki_bank_w )
{
	if (ACCESSING_LSB)
		OKIM6295_set_bank_base(0, (data & 7) * 0x40000);
}


static int okibank;

static WRITE16_HANDLER( quiz365_oki_bank1_w )
{
	if (ACCESSING_LSB)
	{
		okibank = (okibank & 2) | (data & 1);
		OKIM6295_set_bank_base(0, okibank * 0x40000);
	}
}

static WRITE16_HANDLER( quiz365_oki_bank2_w )
{
	if (ACCESSING_LSB)
	{
		okibank = (okibank & 1) | ((data & 1) << 1);
		OKIM6295_set_bank_base(0, okibank * 0x40000);
	}
}



#include <time.h>

static READ_HANDLER( rtc_r )
{
	time_t ltime;
	struct tm *today;
	time(&ltime);
	today = localtime(&ltime);

	switch (offset)
	{
		case 0x0: return today->tm_sec%10;
		case 0x1: return today->tm_sec/10;
		case 0x2: return today->tm_min%10;
		case 0x3: return today->tm_min/10;
		case 0x4: return today->tm_hour%10;
		case 0x5: return today->tm_hour/10;
		case 0x6: return today->tm_mday%10;
		case 0x7: return today->tm_mday/10;
		case 0x8: return (today->tm_mon+1)%10;
		case 0x9: return (today->tm_mon+1)/10;
		case 0xa: return today->tm_year%10;
		case 0xb: return (today->tm_year%100)/10;
		case 0xc: return today->tm_wday%10;
		default: return 0;
	}
}

static READ16_HANDLER( rtc16_r )
{
	return rtc_r(offset);
}


static READ_HANDLER( unk_r )
{
	return 0x78;
}

static READ16_HANDLER( unk16_r )
{
	return unk_r(offset);
}



static data8_t quiz365_select;

READ_HANDLER( quiz365_input_r )
{
	if (!(quiz365_select & 0x01))	return readinputport(3);
	if (!(quiz365_select & 0x02))	return readinputport(4);
	if (!(quiz365_select & 0x04))	return readinputport(5);
	if (!(quiz365_select & 0x08))	return 0xff;/*rand();*/
	if (!(quiz365_select & 0x10))	return 0xff;/*rand();*/
	return 0xff;
}

WRITE_HANDLER( quiz365_select_w )
{
	quiz365_select = data;
}


static data8_t rongrong_select2;

READ_HANDLER( rongrong_input2_r )
{
/*logerror("%04x: rongrong_input2_r offset %d select %x\n",activecpu_get_pc(),offset,rongrong_select2 );*/
	/* 0 and 1 are read from offset 1, 2 from offset 0... */
	switch( rongrong_select2 )
	{
		case 0x00:	return readinputport(0);
		case 0x01:	return readinputport(1);
		case 0x02:	return readinputport(2);
	}
	return 0xff;
}

WRITE_HANDLER( rongrong_select2_w )
{
	rongrong_select2 = data;
}


READ16_HANDLER( quiz365_input2_r )
{
/*logerror("%04x: rongrong_input2_r offset %d select %x\n",activecpu_get_pc(),offset,rongrong_select2 );*/
	/* 0 and 1 are read from offset 1, 2 from offset 0... */
	switch( rongrong_select2 )
	{
		case 0x10:	return readinputport(0);
		case 0x11:	return readinputport(1);
		case 0x12:	return readinputport(2) | (dynax_blitter_irq_flag << 6);
	}
	return 0xff;
}

WRITE16_HANDLER( quiz365_select2_w )
{
	if (ACCESSING_LSB)
		rongrong_select2 = data;
}

static WRITE16_HANDLER( quiz365_coincounter_w )
{
	if (ACCESSING_LSB)
	{
		if (rongrong_select2 == 0x1c)
		{
			coin_counter_w(0, ~data & 1);
			coin_counter_w(1, ~data & 4);
		}
	}
}


static MEMORY_READ16_START( quiz365_readmem )
	{ 0x000000, 0x17ffff, MRA16_ROM					},	/* ROM*/
	{ 0x300286, 0x300287, ddenlovr_gfxrom_r			},	/* Video Chip*/
	{ 0x300270, 0x300271, unk16_r		},	/* ? must be 78 on startup (not necessary in ddlover)*/
	{ 0x300204, 0x300207, quiz365_input2_r		},	/**/
	{ 0x300340, 0x30035f, rtc16_r				},	/* 6242RTC*/
	{ 0x300384, 0x300385, AY8910_read_port_0_lsb_r	},
	{ 0x3002c0, 0x3002c1, OKIM6295_status_0_lsb_r	},	/* Sound*/
	{ 0xff0000, 0xffffff, MRA16_RAM					},	/* RAM*/
MEMORY_END

static MEMORY_WRITE16_START( quiz365_writemem )
	{ 0x000000, 0x17ffff, MWA16_ROM						},	/* ROM*/
	{ 0x200000, 0x2003ff, ddenlovr_palette_w	},	/* Palette*/
/*	{ 0x201000, 0x2017ff, MWA16_RAM 					},	*/ /* ?*/
	{ 0x300240, 0x300247, ddenlovr_palette_base_w },	/* palette base for the 4 layers*/
/*	{ 0x300248, 0x30024f, 			},	*/ /* layer related? palette bank?*/
	{ 0x300200, 0x300201, quiz365_select2_w	},	/**/
	{ 0x300202, 0x300203, quiz365_coincounter_w		},	/* Coin Counters + more stuff written on startup*/
	{ 0x300268, 0x300269, ddenlovr_bgcolor_w },
	{ 0x30026a, 0x30026b, ddenlovr_priority_w },
	{ 0x30026c, 0x30026d, ddenlovr_layer_enable_w },
	{ 0x300280, 0x300283, ddenlovr_blitter_w			},
	{ 0x3003ca, 0x3003cb, ddenlovr_blitter_irq_ack_w	},	/* Blitter irq acknowledge*/
	{ 0x300300, 0x300301, YM2413_register_port_0_lsb_w	},	/* Sound*/
	{ 0x300302, 0x300303, YM2413_data_port_0_lsb_w		},	/**/
/*	{ 0x300340, 0x30035f,  					},	*/ /* 6242RTC*/
	{ 0x300380, 0x300381, AY8910_control_port_0_lsb_w	},
	{ 0x300382, 0x300383, AY8910_write_port_0_lsb_w		},
	{ 0x3002c0, 0x3002c1, OKIM6295_data_0_lsb_w 		},	/**/
	{ 0x3003c2, 0x3003c3, quiz365_oki_bank1_w },
	{ 0x3003cc, 0x3003cd, quiz365_oki_bank2_w },
	{ 0xff0000, 0xffffff, MWA16_RAM						},	/* RAM*/
MEMORY_END


static MEMORY_READ16_START( ddenlovr_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM					},	/* ROM*/
	{ 0xe00086, 0xe00087, ddenlovr_gfxrom_r			},	/* Video Chip*/
	{ 0xe00070, 0xe00071, unk16_r		},	/* ? must be 78 on startup (not necessary in ddlover)*/
	{ 0xe00100, 0xe00101, input_port_0_word_r		},	/* P1?*/
	{ 0xe00102, 0xe00103, input_port_1_word_r		},	/* P2?*/
	{ 0xe00104, 0xe00105, ddenlovr_special_r		},	/* Coins + ?*/
	{ 0xe00200, 0xe00201, input_port_3_word_r		},	/* DSW*/
	{ 0xe00500, 0xe0051f, rtc16_r 				},	/* 6242RTC*/
	{ 0xe00604, 0xe00605, AY8910_read_port_0_lsb_r	},
	{ 0xe00700, 0xe00701, OKIM6295_status_0_lsb_r	},	/* Sound*/
	{ 0xff0000, 0xffffff, MRA16_RAM					},	/* RAM*/
MEMORY_END

static MEMORY_WRITE16_START( ddenlovr_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM						},	/* ROM*/
	{ 0x300000, 0x300001, ddenlovr_oki_bank_w			},
	{ 0xd00000, 0xd003ff, ddenlovr_palette_w	},	/* Palette*/
/*	{ 0xd01000, 0xd017ff, MWA16_RAM 					},	*/ /* ? B0 on startup, then 00*/
	{ 0xe00040, 0xe00047, ddenlovr_palette_base_w },	/* palette base for the 4 layers*/
/*	{ 0xe00048, 0xe0004f, 			},	*/ /* layer related? palette bank? see notes at beginning of driver*/
	{ 0xe00068, 0xe00069, ddenlovr_bgcolor_w },
	{ 0xe0006a, 0xe0006b, ddenlovr_priority_w },
	{ 0xe0006c, 0xe0006d, ddenlovr_layer_enable_w },
	{ 0xe00080, 0xe00083, ddenlovr_blitter_w			},
	{ 0xe00302, 0xe00303, ddenlovr_blitter_irq_ack_w	},	/* Blitter irq acknowledge*/
	{ 0xe00308, 0xe00309, ddenlovr_coincounter_0_w		},	/* Coin Counters*/
	{ 0xe0030c, 0xe0030d, ddenlovr_coincounter_1_w		},	/**/
	{ 0xe00400, 0xe00401, YM2413_register_port_0_lsb_w	},	/* Sound*/
	{ 0xe00402, 0xe00403, YM2413_data_port_0_lsb_w		},	/**/
/*	{ 0xe00500, 0xe0051f, 					},	*/ /* 6242RTC*/
/*	{ 0xe00302, 0xe00303, MWA16_NOP						},	*/ /* ?*/
	{ 0xe00600, 0xe00601, AY8910_control_port_0_lsb_w	},
	{ 0xe00602, 0xe00603, AY8910_write_port_0_lsb_w		},
	{ 0xe00700, 0xe00701, OKIM6295_data_0_lsb_w 		},	/**/
	{ 0xff0000, 0xffffff, MWA16_RAM						},	/* RAM*/
MEMORY_END


static READ16_HANDLER( nettoqc_special_r )
{
	return readinputport(2) | (dynax_blitter_irq_flag ? 0x60 : 0x00 );
}

static data16_t nettoqc_select;

static WRITE16_HANDLER( nettoqc_select_w )
{
	COMBINE_DATA(&nettoqc_select);
}

static READ16_HANDLER( nettoqc_input_r )
{
	if (!(nettoqc_select & 0x01))	return readinputport(3);
	if (!(nettoqc_select & 0x02))	return readinputport(4);
	if (!(nettoqc_select & 0x04))	return readinputport(5);
	return 0xffff;
}

/*
	Protection:

	Writes 37 28 12 to 200e0b then 11 to 200e0d. Expects to read 88 from 200c03
	Writes 67 4c 3a to 200e0b then 19 to 200e0d. Expects to read 51 from 200c03
*/

static data16_t *nettoqc_protection_val;

static READ16_HANDLER( nettoqc_protection_r )
{
	switch( nettoqc_protection_val[0] & 0xff )
	{
		case 0x3a:	return 0x0051;
		default:	return 0x0088;
	}
}

static WRITE16_HANDLER( nettoqc_coincounter_w )
{
	if (ACCESSING_LSB)
	{
		coin_counter_w(0, data & 0x01);
		coin_counter_w(1, data & 0x04);
		/*                data & 0x80 ?*/
	}
}

static WRITE16_HANDLER( nettoqc_oki_bank_w )
{
	if (ACCESSING_LSB)
		OKIM6295_set_bank_base(0, (data & 3) * 0x40000);
}

static MEMORY_READ16_START( nettoqc_readmem )
	{ 0x000000, 0x17ffff, MRA16_ROM					},	/* ROM*/
	{ 0x200c02, 0x200c03, nettoqc_protection_r		},	/**/
	{ 0x300070, 0x300071, unk16_r					},	/* ? must be 78 on startup (not necessary in ddlover)*/
	{ 0x300086, 0x300087, ddenlovr_gfxrom_r			},	/* Video Chip*/
	{ 0x300100, 0x30011f, rtc16_r					},	/* 6242RTC*/
	{ 0x300180, 0x300181, input_port_0_word_r		},	/**/
	{ 0x300182, 0x300183, input_port_1_word_r		},	/**/
	{ 0x300184, 0x300185, nettoqc_special_r			},	/* Coins + ?*/
	{ 0x300186, 0x300187, nettoqc_input_r			},	/* DSW's*/
	{ 0x300240, 0x300241, OKIM6295_status_0_lsb_r	},	/* Sound*/
	{ 0xff0000, 0xffffff, MRA16_RAM					},	/* RAM*/
MEMORY_END

static MEMORY_WRITE16_START( nettoqc_writemem )
	{ 0x000000, 0x17ffff, MWA16_ROM								},	/* ROM*/
	{ 0x200000, 0x2003ff, ddenlovr_palette_w					},	/* Palette*/
	{ 0x200e0a, 0x200e0d, MWA16_RAM, &nettoqc_protection_val	},	/**/
	{ 0x201000, 0x2017ff, MWA16_RAM 							},	/* ?*/
	{ 0x300040, 0x300047, ddenlovr_palette_base_w				},	/* palette base for the 4 layers*/
	{ 0x300068, 0x300069, ddenlovr_bgcolor_w					},
	{ 0x30006a, 0x30006b, ddenlovr_priority_w					},
	{ 0x30006c, 0x30006d, ddenlovr_layer_enable_w				},
	{ 0x300080, 0x300083, ddenlovr_blitter_w					},
	{ 0x3000c0, 0x3000c1, YM2413_register_port_0_lsb_w			},	/* Sound*/
	{ 0x3000c2, 0x3000c3, YM2413_data_port_0_lsb_w				},	/**/
	{ 0x300140, 0x300141, AY8910_control_port_0_lsb_w			},	/**/
	{ 0x300142, 0x300143, AY8910_write_port_0_lsb_w				},	/**/
	{ 0x300188, 0x300189, nettoqc_coincounter_w					},	/* Coin Counters*/
	{ 0x30018a, 0x30018b, nettoqc_select_w						},	/**/
	{ 0x30018c, 0x30018d, nettoqc_oki_bank_w					},
	{ 0x300240, 0x300241, OKIM6295_data_0_lsb_w 				},	/**/
	{ 0x3001ca, 0x3001cb, ddenlovr_blitter_irq_ack_w			},	/* Blitter irq acknowledge*/
	{ 0xff0000, 0xffffff, MWA16_RAM								},	/* RAM*/
MEMORY_END


/***************************************************************************
								Rong Rong
***************************************************************************/

static data8_t rongrong_select;

READ_HANDLER( rongrong_input_r )
{
	if (!(rongrong_select & 0x01))	return readinputport(3);
	if (!(rongrong_select & 0x02))	return readinputport(4);
	if (!(rongrong_select & 0x04))	return 0xff;/*rand();*/
	if (!(rongrong_select & 0x08))	return 0xff;/*rand();*/
	if (!(rongrong_select & 0x10))	return readinputport(5);
	return 0xff;
}

WRITE_HANDLER( rongrong_select_w )
{
	UINT8 *rom = memory_region(REGION_CPU1);

/*logerror("%04x: rongrong_select_w %02x\n",activecpu_get_pc(),data);*/
	/* bits 0-4 = **both** ROM bank **AND** input select */
	cpu_setbank(1, &rom[0x10000 + 0x8000 * (data & 0x1f)]);
	rongrong_select = data;

	/* bits 5-7 = RAM bank */
	cpu_setbank(2, &rom[0x110000 + 0x1000 * ((data & 0xe0) >> 5)]);
}



static MEMORY_READ_START( quizchq_readmem )
	{ 0x0000, 0x5fff, MRA_ROM					},	/* ROM*/
	{ 0x6000, 0x6fff, MRA_RAM					},	/* RAM*/
	{ 0x7000, 0x7fff, MRA_BANK2					},	/* RAM (Banked)*/
	{ 0x8000, 0xffff, MRA_BANK1					},	/* ROM (Banked)*/
MEMORY_END

static MEMORY_WRITE_START( quizchq_writemem )
	{ 0x0000, 0x5fff, MWA_ROM					},	/* ROM*/
	{ 0x6000, 0x6fff, MWA_RAM					},	/* RAM*/
	{ 0x7000, 0x7fff, MWA_BANK2					},	/* RAM (Banked)*/
	{ 0x8000, 0x81ff, rongrong_palette_w	},	/* ROM (Banked)*/
	{ 0x8000, 0xffff, MWA_ROM					},	/* ROM (Banked)*/
MEMORY_END

static PORT_READ_START( quizchq_readport )
	{ 0x03, 0x03, rongrong_gfxrom_r			},	/* Video Chip*/
/*	{ 0x1b, 0x1b,	*/ /* bit 5 = busy flag?*/
	{ 0x1c, 0x1c, rongrong_input_r		},	/**/
	{ 0x22, 0x23, rongrong_input2_r		},	/**/
	{ 0x40, 0x40, OKIM6295_status_0_r	},	/**/
	{ 0x98, 0x98, unk_r		},	/* ? must be 78 on startup*/
	{ 0xa0, 0xaf, rtc_r },	/* 6242RTC*/
PORT_END

static PORT_WRITE_START( quizchq_writeport )
	{ 0x00, 0x01, rongrong_blitter_w	},
	{ 0x1e, 0x1e, rongrong_select_w		},	/**/
	{ 0x20, 0x20, rongrong_select2_w	},	/**/
	{ 0x40, 0x40, OKIM6295_data_0_w		},	/**/
	{ 0x60, 0x60, YM2413_register_port_0_w	},	/* Sound*/
	{ 0x61, 0x61, YM2413_data_port_0_w		},	/**/
	{ 0x80, 0x83, rongrong_palette_base_w },	/* palette base for the 4 layers*/
	{ 0x94, 0x94, dynax_bgcolor_w },
	{ 0x95, 0x95, dynax_priority_w },
	{ 0x96, 0x96, dynax_layer_enable_w },
/*	{ 0xa0, 0xaf, },	*/ /* 6242RTC*/
	{ 0xc0, 0xc0, quizchq_oki_bank_w },
	{ 0xc2, 0xc2, IOWP_NOP },	/* enables palette RAM at f000*/
PORT_END



static MEMORY_READ_START( rongrong_readmem )
	{ 0x0000, 0x5fff, MRA_ROM					},	/* ROM*/
	{ 0x6000, 0x6fff, MRA_RAM					},	/* RAM*/
	{ 0x7000, 0x7fff, MRA_BANK2					},	/* RAM (Banked)*/
	{ 0x8000, 0xffff, MRA_BANK1					},	/* ROM (Banked)*/
MEMORY_END

static MEMORY_WRITE_START( rongrong_writemem )
	{ 0x0000, 0x5fff, MWA_ROM					},	/* ROM*/
	{ 0x6000, 0x6fff, MWA_RAM					},	/* RAM*/
	{ 0x7000, 0x7fff, MWA_BANK2					},	/* RAM (Banked)*/
	{ 0xf000, 0xf1ff, rongrong_palette_w	},
	{ 0x8000, 0xffff, MWA_ROM					},	/* ROM (Banked)*/
MEMORY_END

static PORT_READ_START( rongrong_readport )
	{ 0x03, 0x03, rongrong_gfxrom_r			},	/* Video Chip*/
/*	{ 0x1b, 0x1b,	*/ /* bit 5 = busy flag?*/
	{ 0x1c, 0x1c, rongrong_input_r		},	/**/
	{ 0xa2, 0xa3, rongrong_input2_r		},	/**/
	{ 0x40, 0x40, OKIM6295_status_0_r	},	/**/
	{ 0x98, 0x98, unk_r		},	/* ? must be 78 on startup*/
	{ 0x20, 0x2f, rtc_r },	/* 6242RTC*/
PORT_END

static PORT_WRITE_START( rongrong_writeport )
	{ 0x00, 0x01, rongrong_blitter_w	},
	{ 0x1e, 0x1e, rongrong_select_w		},	/**/
	{ 0xa0, 0xa0, rongrong_select2_w	},	/**/
	{ 0x40, 0x40, OKIM6295_data_0_w		},	/**/
	{ 0x60, 0x60, YM2413_register_port_0_w	},	/* Sound*/
	{ 0x61, 0x61, YM2413_data_port_0_w		},	/**/
	{ 0x80, 0x83, rongrong_palette_base_w },	/* palette base for the 4 layers*/
	{ 0x94, 0x94, dynax_bgcolor_w },
	{ 0x95, 0x95, dynax_priority_w },
	{ 0x96, 0x96, dynax_layer_enable_w },
/*	{ 0x20, 0x2f, },	*/ /* 6242RTC*/
	{ 0xc2, 0xc2, IOWP_NOP },	/* enables palette RAM at f000, and protection device at f705/f706/f601*/
PORT_END
/*
1e input select,1c input read
	3e=dsw1	3d=dsw2
a0 input select,a2 input read (protection?)
	0=?	1=?	2=coins(from a3)
*/


/***************************************************************************
								Monkey Mole Panic
***************************************************************************/


static READ_HANDLER( magic_r )
{
	return 0x01;
}

static data8_t mmpanic_select;
static WRITE_HANDLER( mmpanic_select_w )
{
	mmpanic_select = data;
}

static WRITE_HANDLER( mmpanic_rombank_w )
{
	UINT8 *rom = memory_region(REGION_CPU1);
	cpu_setbank(1, &rom[0x10000 + 0x8000 * (data & 0x7)]);
	/* Bit 4? */
}

static WRITE_HANDLER( mmpanic_soundlatch_w )
{
	soundlatch_w(0,data);
	cpu_set_nmi_line(1, PULSE_LINE);
}

static WRITE_HANDLER( mmpanic_blitter_w )
{
	blitter_w(0,offset,data,0xdf);	/* RST 18*/
}
static WRITE_HANDLER( mmpanic_blitter2_w )
{
	blitter_w(1,offset,data,0xdf);	/* RST 18*/
}

/* A led for each of the 9 buttons */
static data16_t mmpanic_leds;

static void mmpanic_update_leds(void)
{
	set_led_status(0,mmpanic_leds);
}

/* leds 1-8 */
static WRITE_HANDLER( mmpanic_leds_w )
{
	mmpanic_leds = (mmpanic_leds & 0xff00) | data;
	mmpanic_update_leds();
}
/* led 9 */
static WRITE_HANDLER( mmpanic_leds2_w )
{
	mmpanic_leds = (mmpanic_leds & 0xfeff) | (data ? 0x0100 : 0);
	mmpanic_update_leds();
}


static WRITE_HANDLER( mmpanic_lockout_w )
{
	if (mmpanic_select == 0x0c)
	{
		coin_counter_w(0,(~data) & 0x01);
		coin_lockout_w(0,(~data) & 0x02);
		set_led_status(1,(~data) & 0x04);
	}
}

static READ_HANDLER( mmpanic_link_r )	{ return 0xff; }

/* Main CPU */

static MEMORY_READ_START( mmpanic_readmem )
	{ 0x0051, 0x0051, magic_r					},	/* ?*/
	{ 0x0000, 0x5fff, MRA_ROM					},	/* ROM*/
	{ 0x6000, 0x6fff, MRA_RAM					},	/* RAM*/
	{ 0x7000, 0x7fff, MRA_BANK2					},	/* RAM (Banked)*/
	{ 0x8000, 0xffff, MRA_BANK1					},	/* ROM (Banked)*/
MEMORY_END

static MEMORY_WRITE_START( mmpanic_writemem )
	{ 0x0000, 0x5fff, MWA_ROM					},	/* ROM*/
	{ 0x6000, 0x6fff, MWA_RAM					},	/* RAM*/
	{ 0x7000, 0x7fff, MWA_BANK2					},	/* RAM (Banked)*/
	{ 0x8000, 0x81ff, rongrong_palette_w		},	/* ROM (Banked)*/
	{ 0x8000, 0xffff, MWA_ROM					},	/* ROM (Banked)*/
MEMORY_END

static PORT_READ_START( mmpanic_readport )
	{ 0x00, 0x0f, rtc_r					},	/* 6242RTC*/
	{ 0x38, 0x38, unk_r					},	/* ? must be 78 on startup*/
	{ 0x58, 0x58, unk_r					},	/* ? must be 78 on startup*/
	{ 0x63, 0x63, rongrong_gfxrom_r		},	/* Video Chip*/
	{ 0x6a, 0x6a, input_port_0_r		},
	{ 0x6b, 0x6b, input_port_1_r		},
	{ 0x6c, 0x6d, mmpanic_link_r		},	/* Other cabinets?*/
	{ 0x7c, 0x7c, OKIM6295_status_0_r	},	/* Sound*/
	{ 0x94, 0x94, input_port_2_r		},	/* DSW 1*/
	{ 0x98, 0x98, input_port_3_r		},	/* DSW 2*/
	{ 0x9c, 0x9c, input_port_4_r		},	/* DSW 1&2 high bits*/
PORT_END

static PORT_WRITE_START( mmpanic_writeport )
	{ 0x00, 0x0f, IOWP_NOP					},	/* 6242RTC*/
	/* Layers 0-3:*/
	{ 0x20, 0x23, rongrong_palette_base_w	},
	{ 0x34, 0x34, dynax_bgcolor_w			},
	{ 0x35, 0x35, dynax_priority_w			},
	{ 0x36, 0x36, dynax_layer_enable_w		},
	/* Layers 4-7:*/
	{ 0x40, 0x43, dynax_palette_base2_w		},
	{ 0x54, 0x54, dynax_bgcolor2_w			},
	{ 0x55, 0x55, dynax_priority2_w			},
	{ 0x56, 0x56, dynax_layer_enable2_w		},

	{ 0x60, 0x61, mmpanic_blitter_w			},
	{ 0x64, 0x65, mmpanic_blitter2_w		},
	{ 0x68, 0x68, mmpanic_select_w			},
	{ 0x69, 0x69, mmpanic_lockout_w			},
	{ 0x74, 0x74, mmpanic_rombank_w			},

	{ 0x78, 0x78, IOWP_NOP					},	/* 0, during RST 08 (irq acknowledge?)*/

	{ 0x7c, 0x7c, OKIM6295_data_0_w			},	/* Sound*/
	{ 0x8c, 0x8c, mmpanic_soundlatch_w		},	/**/
	{ 0x88, 0x88, mmpanic_leds_w			},	/* Leds*/
	{ 0x90, 0x90, IOWP_NOP					},	/* written just before port 8c*/
	{ 0xa6, 0xa6, mmpanic_leds2_w			},	/**/
PORT_END

/* Sound CPU */

static MEMORY_READ_START( mmpanic_sound_readmem )
	{ 0x0000, 0x5fff, MRA_ROM					},	/* ROM*/
	{ 0x6000, 0x66ff, MRA_RAM					},	/* RAM*/
	{ 0x8000, 0xffff, MRA_ROM					},	/* ROM*/
MEMORY_END

static MEMORY_WRITE_START( mmpanic_sound_writemem )
	{ 0x0000, 0x5fff, MWA_ROM					},	/* ROM*/
	{ 0x6000, 0x66ff, MWA_RAM					},	/* RAM*/
	{ 0x8000, 0xffff, MWA_ROM					},	/* ROM*/
MEMORY_END

static PORT_READ_START( mmpanic_sound_readport )
	{ 0x00, 0x00, soundlatch_r		},
	{ 0x02, 0x02, IORP_NOP			},	/* read just before port 00*/
	{ 0x04, 0x04, IORP_NOP			},	/* read only once at the start*/
PORT_END

static PORT_WRITE_START( mmpanic_sound_writeport )
	{ 0x04, 0x04, IOWP_NOP					},	/* 0, during NMI*/
	{ 0x06, 0x06, IOWP_NOP					},	/* almost always 1, sometimes 0*/
	{ 0x08, 0x08, YM2413_register_port_0_w	},
	{ 0x09, 0x09, YM2413_data_port_0_w		},
	{ 0x0c, 0x0c, AY8910_write_port_0_w		},
	{ 0x0e, 0x0e, AY8910_control_port_0_w	},
PORT_END



INPUT_PORTS_START( ddenlovr )
	PORT_START	/* IN0 - Player 1*/
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER1 )

	PORT_START	/* IN1 - Player 2*/
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER2 )

	PORT_START	/* IN2 - Coins + ?*/
	PORT_BIT(  0x01, IP_ACTIVE_LOW,  IPT_COIN1    )
	PORT_BIT(  0x02, IP_ACTIVE_LOW,  IPT_COIN2    )
	PORT_BIT(  0x04, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BITX( 0x08, IP_ACTIVE_LOW,  IPT_SERVICE, "Test", KEYCODE_F1, IP_JOY_NONE )	/* Test */
	PORT_BIT(  0x10, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
	PORT_BIT(  0x20, IP_ACTIVE_LOW,  IPT_SPECIAL  )	/* ? quiz365*/
	PORT_BIT(  0x40, IP_ACTIVE_HIGH, IPT_SPECIAL  )	/* blitter irq flag*/
	PORT_BIT(  0x80, IP_ACTIVE_HIGH, IPT_SPECIAL  )	/* blitter busy flag*/

	/* one of the dips selects the nudity level */
	PORT_START	/* IN3 - DSW*/
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x02, 0x02, "Unknown 1-1" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Unknown 1-2" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Unknown 1-3" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 1-4" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Unknown 1-5*" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 1-6*" )	/* 6&7*/
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 1-7*" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END


INPUT_PORTS_START( nettoqc )
	PORT_START	/* IN0 - Player 1*/
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_BUTTON1  | IPF_PLAYER1 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_BUTTON2  | IPF_PLAYER1 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_BUTTON3  | IPF_PLAYER1 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON4  | IPF_PLAYER1 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN1 - Player 2*/
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_BUTTON1  | IPF_PLAYER2 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_BUTTON2  | IPF_PLAYER2 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_BUTTON3  | IPF_PLAYER2 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON4  | IPF_PLAYER2 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 - Coins + ?*/
	PORT_BIT(  0x01, IP_ACTIVE_LOW,  IPT_COIN1    )
	PORT_BIT(  0x02, IP_ACTIVE_LOW,  IPT_COIN2    )
	PORT_BIT(  0x04, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BITX( 0x08, IP_ACTIVE_LOW,  IPT_SERVICE, "Test", KEYCODE_F1, IP_JOY_NONE )	/* Test */
	PORT_BIT(  0x10, IP_ACTIVE_HIGH, IPT_SPECIAL  )	/* blitter busy flag*/
	PORT_BIT(  0x20, IP_ACTIVE_HIGH, IPT_SPECIAL  )	/* ?*/
	PORT_BIT(  0x40, IP_ACTIVE_HIGH, IPT_SPECIAL  )	/* blitter irq flag*/
	PORT_BIT(  0x80, IP_ACTIVE_HIGH, IPT_SPECIAL  )

	PORT_START	/* IN3 - DSW*/
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 1-6*" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 1-7*" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* IN4 - DSW*/
	PORT_DIPNAME( 0x03, 0x03, "Unknown 2-0&1*" )
	PORT_DIPSETTING(    0x02, "0" )
	PORT_DIPSETTING(    0x03, "1" )
	PORT_DIPSETTING(    0x01, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPNAME( 0x04, 0x04, "Unknown 2-2" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Unknown 2-3*" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 2-4*" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Unknown 2-5*" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 2-6*" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 2-7*" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* IN5 - DSW*/
	PORT_DIPNAME( 0x01, 0x01, "Unknown 1-8*" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x02, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x04, 0x04, "Unknown 2-8*" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Unknown 2-9*" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x70, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x80, 0x00, "Detailed Tests" )	/* menu "8 OPTION" in service mode*/
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END


INPUT_PORTS_START( quiz365 )
	PORT_START	/* IN0 - Player 1*/
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER1 )

	PORT_START	/* IN1 - Player 2*/
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER2 )

	PORT_START	/* IN2 - Coins + ?*/
	PORT_BIT(  0x01, IP_ACTIVE_LOW,  IPT_COIN1    )
	PORT_BIT(  0x02, IP_ACTIVE_LOW,  IPT_COIN2    )
	PORT_BIT(  0x04, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BITX( 0x08, IP_ACTIVE_LOW,  IPT_SERVICE, "Test", KEYCODE_F1, IP_JOY_NONE )	/* Test */
	PORT_BIT(  0x10, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
	PORT_BIT(  0x20, IP_ACTIVE_LOW,  IPT_SPECIAL  )	/* ? quiz365*/
	PORT_BIT(  0x40, IP_ACTIVE_HIGH, IPT_SPECIAL  )	/* blitter irq flag*/
	PORT_BIT(  0x80, IP_ACTIVE_HIGH, IPT_SPECIAL  )	/* blitter busy flag*/

	PORT_START	/* IN3 - DSW*/
	PORT_DIPNAME( 0x01, 0x01, "Unknown 1-0" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Unknown 1-1" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Unknown 1-2" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Unknown 1-3" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 1-4" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Unknown 1-5" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 1-6" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 1-7" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* IN4 - DSW*/
	PORT_DIPNAME( 0x01, 0x01, "Unknown 1-0" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Unknown 1-1" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Unknown 1-2" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Unknown 1-3" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 1-4" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Unknown 1-5" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 1-6" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 1-7" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* IN5 - DSW*/
	PORT_DIPNAME( 0x01, 0x01, "Unknown 1-0" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x02, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x04, 0x00, "Detailed Tests" )	/* menu "8 OPTION" in service mode*/
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Unknown 1-3" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


INPUT_PORTS_START( rongrong )
	PORT_START	/* IN0 - Player 1*/
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER1 )

	PORT_START	/* IN1 - Player 2*/
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER2 )

	PORT_START	/* IN2 - Coins + ?*/
	PORT_BIT(  0x01, IP_ACTIVE_LOW,  IPT_COIN1    )
	PORT_BIT(  0x02, IP_ACTIVE_LOW,  IPT_COIN2    )
	PORT_BIT(  0x04, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BITX( 0x08, IP_ACTIVE_LOW,  IPT_SERVICE, "Test", KEYCODE_F1, IP_JOY_NONE )	/* Test */
	PORT_BIT(  0x10, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
	PORT_BIT(  0x20, IP_ACTIVE_LOW,  IPT_SPECIAL  )	/* ? quiz365*/
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_SPECIAL  )	/* ? blitter irq flag ?*/
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_SPECIAL  )	/* ? blitter busy flag ?*/

	PORT_START	/* IN3 - DSW*/
	PORT_DIPNAME( 0x01, 0x01, "Unknown 1-0" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Unknown 1-1" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Unknown 1-2" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Unknown 1-3" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 1-4" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Unknown 1-5" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 1-6" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 1-7" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* IN4 - DSW*/
	PORT_DIPNAME( 0x01, 0x01, "Unknown 1-0" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Unknown 1-1" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Unknown 1-2" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Unknown 1-3" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 1-4" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Unknown 1-5" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 1-6" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 1-7" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* IN5 - DSW*/
	PORT_DIPNAME( 0x01, 0x01, "Unknown 1-0" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Unknown 1-1" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Unknown 1-2" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Unknown 1-3" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( quizchq )
	PORT_START	/* IN0 - Player 1*/
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN1 - Player 2*/
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 - Coins + ?*/
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_COIN1    )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_COIN2    )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_SERVICE, "Test", KEYCODE_F1, IP_JOY_NONE )	/* Test */
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_SPECIAL  )	/* ? quiz365*/
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_SPECIAL  )	/* ? blitter irq flag ?*/
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_SPECIAL  )	/* ? blitter busy flag ?*/

	PORT_START	/* IN3 - DSW*/
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 1-4" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ))
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 1-6" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Set Date" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* IN4 - DSW*/
	PORT_DIPNAME( 0x01, 0x01, "Unknown 2-0" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Unknown 2-1" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Unknown 2-2" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Unknown 2-3" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 2-4" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Unknown 2-5" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 2-6" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	/* IN5 - DSW*/
	PORT_DIPNAME( 0x01, 0x01, "Unknown 1-0" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Unknown 1-1" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Unknown 1-2" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Unknown 1-3" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( mmpanic )
	PORT_START	/* IN0 6a (68 = 1:used? 2:normal 3:goes to 69)*/
	PORT_BIT_IMPULSE( 0x01, IP_ACTIVE_LOW, IPT_COIN1, 2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* tested?*/
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_SERVICE, "Test", KEYCODE_F1, IP_JOY_NONE )	/* Test */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* tested?*/
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "8", KEYCODE_8_PAD, IP_JOY_NONE )
	PORT_BITX(0x40, IP_ACTIVE_LOW, 0, "9", KEYCODE_9_PAD, IP_JOY_NONE )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SPECIAL )	/* busy?*/

	PORT_START	/* IN1 6b (68 = 0 & 1)*/
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* tested*/
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "1", KEYCODE_1_PAD, IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "2", KEYCODE_2_PAD, IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "3", KEYCODE_3_PAD, IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "4", KEYCODE_4_PAD, IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "5", KEYCODE_5_PAD, IP_JOY_NONE )
	PORT_BITX(0x40, IP_ACTIVE_LOW, 0, "6", KEYCODE_6_PAD, IP_JOY_NONE )
	PORT_BITX(0x80, IP_ACTIVE_LOW, 0, "7", KEYCODE_7_PAD, IP_JOY_NONE )

	PORT_START	/* IN2 - DSW*/
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x1c, 0x10, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x1c, "0" )
	PORT_DIPSETTING(    0x18, "1" )
	PORT_DIPSETTING(    0x14, "2" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPSETTING(    0x0c, "4" )
	PORT_DIPSETTING(    0x08, "5" )
/*	PORT_DIPSETTING(    0x04, "5" )*/
/*	PORT_DIPSETTING(    0x00, "5" )*/
	PORT_DIPNAME( 0x20, 0x20, "Linked Cabinets" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 1-7*" )	/* 2-0 is related to the same thing (flip?)*/
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* IN3 - DSW*/
	PORT_DIPNAME( 0x01, 0x01, "Unknown 2-0*" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Unknown 2-1*" )	/* reg 1 of blitter*/
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, "Unknown 2-2&3*" )
	PORT_DIPSETTING(    0x0c, "10" )
	PORT_DIPSETTING(    0x08, "20" )
	PORT_DIPSETTING(    0x04, "40" )
	PORT_DIPSETTING(    0x00, "80" )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 2-4*" )	/* used?*/
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Unknown 2-5*" )	/* used?*/
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 2-6*" )	/* 6 & 7?*/
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 2-7*" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* IN4 - DSW*/
	PORT_DIPNAME( 0x01, 0x01, "Set Date" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x02, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x04, 0x04, "Unknown 3-2*" )	/* used?*/
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Unknown 3-3*" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 3-4" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Unknown 3-5" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 3-6*" )	/* used?*/
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 3-7*" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

/***************************************************************************
							Don Den Lover Vol.1
***************************************************************************/

static struct YM2413interface ym2413_interface =
{
	1,
	3579545,	/* ???? */
	{ YM2413_VOL(80,MIXER_PAN_CENTER,80,MIXER_PAN_CENTER) }
};

static struct AY8910interface ay8910_interface =
{
	1,			/* 1 chip */
	2000000,	/* ??? */
	{ 30 },
	{ quiz365_input_r },
	{ 0 },
	{ 0 },
	{ quiz365_select_w }
};

static struct OKIM6295interface okim6295_interface =
{
	1,
	{ 8000 },	/* ? */
	{ REGION_SOUND1 },
	{ 80 }
};

static MACHINE_DRIVER_START( ddenlovr )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main",M68000,24000000 / 2)
	MDRV_CPU_MEMORY(ddenlovr_readmem,ddenlovr_writemem)
	MDRV_CPU_VBLANK_INT(irq1_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(336, 256)
	MDRV_VISIBLE_AREA(0, 336-1, 5, 256-16+5-1)
	MDRV_PALETTE_LENGTH(0x100)

	MDRV_VIDEO_START(ddenlovr)
	MDRV_VIDEO_UPDATE(ddenlovr)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2413, ym2413_interface)
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( quiz365 )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(ddenlovr)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(quiz365_readmem,quiz365_writemem)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( nettoqc )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(ddenlovr)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(nettoqc_readmem,nettoqc_writemem)
MACHINE_DRIVER_END

/***************************************************************************
								Rong Rong
***************************************************************************/

/* the CPU is in Interrupt Mode 2
   vector can be 0xee, 0xf8 0xfa 0xfc
   rongrong: 0xf8 and 0xfa do nothing
   quizchq: 0xf8 and 0xfa are very similar, they should be treiggered by the blitter
   0xee is vblank
   0xfc is from the 6242RTC
 */
static INTERRUPT_GEN( quizchq_irq )
{
	static int count;

	/* I haven't found a irq ack register, so I need this kludge to
	   make sure I don't lose any interrupt generated by the blitter,
	   otherwise quizchq would lock up. */
	if (cpunum_get_reg(0,Z80_IRQ_STATE))
		return;

	if ((++count % 60) == 0)
		cpu_set_irq_line_and_vector(0, 0, HOLD_LINE, 0xfc);
	else
		cpu_set_irq_line_and_vector(0, 0, HOLD_LINE, 0xee);
}

static INTERRUPT_GEN( rtc_irq )
{
/*	cpu_set_irq_line_and_vector(0, 0, HOLD_LINE, 0xfc);*/
}

static MACHINE_DRIVER_START( quizchq )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 8000000)	/* ? */
	MDRV_CPU_MEMORY(quizchq_readmem,quizchq_writemem)
	MDRV_CPU_PORTS(quizchq_readport,quizchq_writeport)
	MDRV_CPU_VBLANK_INT(quizchq_irq,1)
/*	MDRV_CPU_PERIODIC_INT(rtc_irq,1)*/

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(336, 256)
	MDRV_VISIBLE_AREA(0, 336-1, 5, 256-16+5-1)
	MDRV_PALETTE_LENGTH(0x100)
	MDRV_COLORTABLE_LENGTH(0x100)

	MDRV_VIDEO_START(ddenlovr)
	MDRV_VIDEO_UPDATE(ddenlovr)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2413, ym2413_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( rongrong )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(quizchq)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(rongrong_readmem,rongrong_writemem)
	MDRV_CPU_PORTS(rongrong_readport,rongrong_writeport)
MACHINE_DRIVER_END

/***************************************************************************
***************************************************************************/

/*	the CPU is in Interrupt Mode 0:

	RST 08 is vblank
	RST 18 is from the blitter
	RST 20 is from the 6242RTC
 */
static INTERRUPT_GEN( mmpanic_irq )
{
	static int count;

	/* I haven't found a irq ack register, so I need this kludge to
	   make sure I don't lose any interrupt generated by the blitter,
	   otherwise the game would lock up. */
	if (cpunum_get_reg(0,Z80_IRQ_STATE))
		return;

	if ((++count % 60) == 0)
		cpu_set_irq_line_and_vector(0, 0, HOLD_LINE, 0xe7);	/* RST 20, clock*/
	else
		cpu_set_irq_line_and_vector(0, 0, HOLD_LINE, 0xcf);	/* RST 08, vblank*/
}

static MACHINE_DRIVER_START( mmpanic )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 8000000)	/* ? */
	MDRV_CPU_MEMORY(mmpanic_readmem,mmpanic_writemem)
	MDRV_CPU_PORTS(mmpanic_readport,mmpanic_writeport)
	MDRV_CPU_VBLANK_INT(mmpanic_irq,1)

	MDRV_CPU_ADD_TAG("sound", Z80, 8000000)	/* ? */
	MDRV_CPU_MEMORY(mmpanic_sound_readmem,mmpanic_sound_writemem)
	MDRV_CPU_PORTS(mmpanic_sound_readport,mmpanic_sound_writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)	/* NMI by main cpu*/

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(336, 256)
	MDRV_VISIBLE_AREA(0, 336-1, 5, 256-16+5-1)
	MDRV_PALETTE_LENGTH(0x100)
	MDRV_COLORTABLE_LENGTH(0x100)

	MDRV_VIDEO_START(mmpanic)	/* extra layers*/
	MDRV_VIDEO_UPDATE(ddenlovr)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2413, ym2413_interface)
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END


/***************************************************************************

Monkey Mole Panic
Nakanihon/Taito 1992
                      7001A  5563    6242
                      6295   7002
                             Z80
     8910                   5563   16MHz
     DynaX NL-001           7003              14.318MHz
                            Z80               24 MHz
          2018
                  DynaX   524256  524256       DynaX
                  1108    524256  524256       1427
                  DynaX   524256  524256       DynaX
                  1108    524256  524256       1427

     8251                      7006    7005   7004


The game asks players to slap buttons on a control panel and see mole-like creatures
get crunched on the eye-level video screen.

An on-screen test mode means the ticket dispenser can be adjusted from 1-99 tickets
and 15 score possibilities.

It also checks PCB EPROMs, switches and lamps, and the built-in income analyzer.

There are six levels of difficulty for one or two players.

The games are linkable (up to four) for competitive play.

***************************************************************************/

ROM_START( mmpanic )
	ROM_REGION( 0x50000, REGION_CPU1, 0 )	/* Z80 Code */
	ROM_LOAD( "nwc7002a",     0x00000, 0x40000, CRC(725b337f) SHA1(4d1f1ebc4de524d959dde60498d3f7038c7f3ed2) )
	ROM_RELOAD(               0x10000, 0x40000 )

	ROM_REGION( 0x30000, REGION_CPU2, 0 )	/* Z80 Code */
	ROM_LOAD( "nwc7003",      0x00000, 0x20000, CRC(4f02ce44) SHA1(9a3abd9c555d5863a2110d84d1a3f582ba9d56b9) )
	ROM_RELOAD(               0x10000, 0x20000 )

	ROM_REGION( 0x280000, REGION_GFX1, 0 )	/* blitter data */
	ROM_LOAD( "nwc7004",      0x000000, 0x100000, CRC(5b4ad8c5) SHA1(a92a0bef01c71e745597ec96e7b8aa0ec26dc59d) )
	ROM_LOAD( "nwc7005",      0x100000, 0x100000, CRC(9ec41956) SHA1(5a92d725cee7052e1c3cd671b58795125c6a4ea9) )
	ROM_LOAD( "nwc7006a",     0x200000, 0x080000, CRC(9099c571) SHA1(9762612f41384602d545d2ec6dabd5f077d5fe21) )

	ROM_REGION( 0x40000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "nwc7001a",     0x00000, 0x40000, CRC(1ae3660d) SHA1(c4711f00a30f7d2c80fe241d7e0a464f0bb2555f) )
ROM_END


/***************************************************************************

Quiz Channel Question (JPN ver.)
(c)1993 Nakanihon

N7311208L1-2
N73SUB

CPU:	TMPZ84C015BF-8
Sound:	YM2413
	M6295
OSC:	16MHz
	28.6363MHz
	32.768KHz ?
Custom:	NL-002 - Nakanihon
	(1108F0405) - Dynax
	(1427F0071) - Dynax
Others:	M6242B (RTC?)

***************************************************************************/

ROM_START( quizchq )
	ROM_REGION( 0x118000, REGION_CPU1, 0 )	/* Z80 Code + space for banked RAM */
	ROM_LOAD( "nwc7302.3e",   0x00000, 0x80000, CRC(14217f2d) SHA1(3cdffcf73e62586893bfaa7c47520b0698d3afda) )
	ROM_RELOAD(               0x10000, 0x80000 )
	ROM_LOAD( "nwc7303.4e",   0x90000, 0x80000, CRC(ffc77601) SHA1(b25c4a027e1fa4397dd86299dfe9251022b0d174) )

	ROM_REGION( 0x320000, REGION_GFX1, 0 )	/* blitter data */
	ROM_LOAD( "nwc7307.s4b",  0x000000, 0x80000, CRC(a09d1dbe) SHA1(f17af24293eea803ebb5c758bffb4519dcad3a71) )
	ROM_LOAD( "nwc7306.s3b",  0x080000, 0x80000, CRC(52d27aac) SHA1(3c38278a5ce757ca0c4a22e4de6052132edd7cbc) )
	ROM_LOAD( "nwc7305.s2b",  0x100000, 0x80000, CRC(5f50914e) SHA1(1fe5df146e028995c53a5aca896546898d7b5914) )
	ROM_LOAD( "nwc7304.s1b",  0x180000, 0x80000, CRC(72866919) SHA1(12b0c95f98c8c76a47e561e1d5035b62f1ec0789) )
	ROM_LOAD( "nwc7310.s4a",  0x200000, 0x80000, CRC(5939aeab) SHA1(6fcf63d6801cb506822a6d06b7bce45ecbb0b4dd) )
	ROM_LOAD( "nwc7309.s3a",  0x280000, 0x80000, CRC(88c863b2) SHA1(60e5098c84ffb302abce788a064c323bece9cc6b) )
	ROM_LOAD( "nwc7308.s2a",  0x300000, 0x20000, CRC(6eb5c81d) SHA1(c8e31e246e1235c045f5a881c6db43a2aff848ff) )

	ROM_REGION( 0x80000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "nwc7301.1f",   0x00000, 0x80000, CRC(52c672e8) SHA1(bc05155f4d9c711cc2ed187a4dd2207b886452f0) )	/* 2 banks*/
ROM_END

ROM_START( quizchql )
	ROM_REGION( 0x118000, REGION_CPU1, 0 )	/* Z80 Code + space for banked RAM */
	ROM_LOAD( "2.rom",        0x00000, 0x80000, CRC(1bf8fb25) SHA1(2f9a62654a018f19f6783be655d992c457551fc9) )
	ROM_RELOAD(               0x10000, 0x80000 )
	ROM_LOAD( "3.rom",        0x90000, 0x80000, CRC(6028198f) SHA1(f78c3cfc0663b44655cb75928941a5ec4a57c8ba) )

	ROM_REGION( 0x420000, REGION_GFX1, 0 )	/* blitter data */
	ROM_LOAD( "4.rom",        0x000000, 0x80000, CRC(e6bdea31) SHA1(cb39d1d5e367ad2623c2bd0b2966541aa41bbb9b) )
	ROM_LOAD( "5.rom",        0x080000, 0x80000, CRC(c243f10a) SHA1(22366a9441b8317780e85065accfa59fe1cd8258) )
	ROM_LOAD( "11.rom",       0x100000, 0x80000, CRC(c9ae5880) SHA1(1bbda7293178132797dd017d71b24aba5ce57022) )
	ROM_LOAD( "7.rom",        0x180000, 0x80000, CRC(a490aa4e) SHA1(05ff9982f0fb1062701063905aeeb50f37283e18) )
	ROM_LOAD( "6.rom",        0x200000, 0x80000, CRC(fbf713b6) SHA1(3ce73fa30dc020053b313dca1587ef6dd8ba1690) )
	ROM_LOAD( "8.rom",        0x280000, 0x80000, CRC(68d4b79f) SHA1(5937760495461dbe6a12670d631754c772171289) )
	ROM_LOAD( "10.rom",       0x300000, 0x80000, CRC(d56eaf0e) SHA1(56214de0b08c7db703a9af7dfd7e2deb74f36542) )
	ROM_LOAD( "9.rom",        0x380000, 0x80000, CRC(a11d535a) SHA1(5e95f07807cd2a5a0eae6cb5c70ccf4516d65124) )
	ROM_LOAD( "12.rom",       0x400000, 0x20000, CRC(43f8e5c7) SHA1(de4c8cc0948b0ce9e1ddf4bea434a7640db451e2) )

	ROM_REGION( 0x80000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "1snd.rom",     0x00000, 0x80000, CRC(cebb9220) SHA1(7a2ee750f2e608a37858b849914316dc778bcae2) )	/* 2 banks*/
ROM_END



ROM_START( quiz365 )
	ROM_REGION( 0x180000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "7805.rom", 0x000000, 0x080000, CRC(6db33222) SHA1(5f0cc9a15815252d8d5e85975ce8770717eb3ac8) )
	ROM_LOAD16_BYTE( "7804.rom", 0x000001, 0x080000, CRC(46d04ace) SHA1(b6489309d7704d2382802aa0f2f7526e367667ad) )
	ROM_LOAD16_BYTE( "7803.rom", 0x100000, 0x040000, CRC(5b7a78d3) SHA1(6ade16df301b57e4a7309834a47ca72300f50ffa) )
	ROM_LOAD16_BYTE( "7802.rom", 0x100001, 0x040000, CRC(c3238a9d) SHA1(6b4b2ab1315fc9e2667b4f8f394e00a27923f926) )

	ROM_REGION( 0x400000, REGION_GFX1, 0 )	/* blitter data */
	ROM_LOAD( "7810.rom", 0x000000, 0x100000, CRC(4b1a4984) SHA1(581ee032b396d65cd604f39846153a4dcb296aad) )
	ROM_LOAD( "7809.rom", 0x100000, 0x100000, CRC(139d52ab) SHA1(08d705301379fcb952cbb1add0e16a148e611bbb) )
	ROM_LOAD( "7808.rom", 0x200000, 0x080000, CRC(a09fd4a4) SHA1(016ecbf1d27a4890dee01e1966ec5efff6eb3afe) )
	ROM_LOAD( "7806.rom", 0x280000, 0x100000, CRC(75767c6f) SHA1(aef925dec3acfc01093d29f44e4a70f0fe28f66d) )
	ROM_LOAD( "7807.rom", 0x380000, 0x080000, CRC(60fb1dfe) SHA1(35317220b6401ccb03bb4ab7d3c0b6ab7637d82a) )

	ROM_REGION( 0x100000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "7801.rom", 0x080000, 0x080000, CRC(285cc62a) SHA1(7cb3bd0ead303787964bcf7a0ecf896b6a6bfa54) )	/* bank 2,3*/
	ROM_CONTINUE(         0x000000, 0x080000 )				/* bank 0,1*/
ROM_END


/***************************************************************************

								Rong Rong

Here are the proms for Nakanihon's Rong Rong
It's a quite nice Puzzle game.
The CPU don't have any numbers on it except for this:
Nakanihon
NL-002
3J3  JAPAN
For the sound it uses A YM2413

***************************************************************************/

ROM_START( rongrong )
	ROM_REGION( 0x118000, REGION_CPU1, 0 )	/* Z80 Code + space for banked RAM */
	ROM_LOAD( "rr_8002g.rom", 0x00000, 0x80000, CRC(9a5d2885) SHA1(9ca049085d14b1cfba6bd48adbb0b883494e7d29) )
	ROM_RELOAD(               0x10000, 0x80000 )
	/* 90000-10ffff empty */

	ROM_REGION( 0x280000, REGION_GFX1, 0 )	/* blitter data */
	ROM_LOAD( "rr_8003.rom",  0x000000, 0x80000, CRC(f57192e5) SHA1(e33f5243028520492cd876be3e4b6a76a9b20d46) )
	ROM_LOAD( "rr_8004.rom",  0x080000, 0x80000, CRC(c8c0b5cb) SHA1(d0c99908022b7d5d484e6d1990c00f15f7d8665a) )
	ROM_LOAD( "rr_8005g.rom", 0x100000, 0x80000, CRC(11c7a23c) SHA1(96d6b82db2555f7d0df661367a7a09bd4eaecba9) )
	ROM_LOAD( "rr_8006g.rom", 0x180000, 0x80000, CRC(f3de77e6) SHA1(13839837eab6acf6f8d6a9ca08fe56c872d50e6a) )
	ROM_LOAD( "rr_8007g.rom", 0x200000, 0x80000, CRC(38a8caa3) SHA1(41d6745bb340b7f8708a6b772f241989aa7fa09d) )

	ROM_REGION( 0x40000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "rr_8001w.rom", 0x00000, 0x40000, CRC(8edc87a2) SHA1(87e8ad50be025263e682cbfb5623f3a35b17118f) )
ROM_END


/***************************************************************************

Netto Quiz Champion (c) Nakanihon

CPU: 68HC000
Sound: OKI6295
Other: HN46505, unknown 68 pin, unknown 100 pin (x2), unknown 64 pin (part numbers scratched off).
PLDs: GAL16L8B (x2, protected)
RAM: TC524258BZ-10 (x5), TC55257BSPL-10 (x2), TC5588P-35
XTAL1: 16 MHz
XTAL2: 28.63636 MHz

***************************************************************************/

ROM_START( nettoqc )
	ROM_REGION( 0x180000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "10305.rom", 0x000000, 0x080000, CRC(ebb14a1f) SHA1(5e4511a878d0bcede79a287fb184e912c9eb7dc5) )
	ROM_LOAD16_BYTE( "10303.rom", 0x000001, 0x080000, CRC(30c114c3) SHA1(fa9c26d465d2d919e141bbc080a04ac0f87c7010) )
	ROM_LOAD16_BYTE( "10306.rom", 0x100000, 0x040000, CRC(f19fe827) SHA1(37907bf3206af5f4613dc80b6bd91c87dd6645ab) )
	ROM_LOAD16_BYTE( "10304.rom", 0x100001, 0x040000, CRC(da1f56e5) SHA1(76c865927ee8392dd77476a248816e04e60c784a) )
	ROM_CONTINUE(                 0x100001, 0x040000 )	/* 1ST AND 2ND HALF IDENTICAL*/

	ROM_REGION( 0x400000, REGION_GFX1, 0 )	/* blitter data */
	ROM_LOAD( "10307.rom", 0x000000, 0x100000, CRC(c7a3b05f) SHA1(c931670c5d14f8446404ad00d785fa73d97dedfc) )
	ROM_LOAD( "10308.rom", 0x100000, 0x100000, CRC(416807a1) SHA1(bccf746ddc9750e3956299fec5b3737a53b24c36) )
	ROM_LOAD( "10309.rom", 0x200000, 0x100000, CRC(81841272) SHA1(659c009c41ae54d330da41922c8afd1fb293d854) )
	ROM_LOAD( "10310.rom", 0x300000, 0x080000, CRC(0f790cda) SHA1(97c79b02ba95551514f8dee701bd71b53e41abf4) )
	ROM_LOAD( "10311.rom", 0x380000, 0x080000, CRC(41109231) SHA1(5e2f4684fd65dcdfb61a94099e0600c23a4740b2) )

	ROM_REGION( 0x100000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "10301.rom", 0x000000, 0x080000, CRC(52afd952) SHA1(3ed6d92b78552d390ee305bb216648dbf6d63daf) )
	ROM_LOAD( "10302.rom", 0x080000, 0x080000, CRC(6e2d4660) SHA1(d7924af8807f7238a7885b204a8c352ff75298b7) )
ROM_END


/***************************************************************************

Don Den Lover Vol 1
(C) Dynax Inc 1995

CPU: TMP68HC000N-12
SND: OKI M6295, YM2413 (18 pin DIL), YMZ284-D (16 pin DIL. This chip is in place where a 40 pin chip is marked on PCB,
                                     possibly a replacement for some other 40 pin YM chip?)
OSC: 28.636MHz (near large GFX chip), 24.000MHz (near CPU)
DIPS: 1 x 8 Position switch. DIP info is in Japanese !
RAM: 1 x Toshiba TC5588-35, 2 x Toshiba TC55257-10, 5 x OKI M514262-70

OTHER:
Battery
RTC 72421B   4382 (18 pin DIL)
3 X PAL's (2 on daughter-board at locations 2E & 2D, 1 on main board near CPU at location 4C)
GFX Chip - NAKANIHON NL-005 (208 pin, square, surface-mounted)

***************************************************************************/

ROM_START( ddenlovr )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "1134h.1a", 0x000000, 0x040000, CRC(43accdff) SHA1(3023d4a071fc877f8e4325e95e586739077ccb02) )
	ROM_LOAD16_BYTE( "1133h.1c", 0x000001, 0x040000, CRC(361bf7b6) SHA1(1727112284cd1dcc1ed17ccba214cb0f8993650a) )

	ROM_REGION( 0x480000, REGION_GFX1, 0 )	/* blitter data */
	/* 000000-1fffff empty */
	ROM_LOAD( "1135h.3h", 0x200000, 0x080000, CRC(ee143d8e) SHA1(61a36c64d450209071e996b418adf416dfa68fd9) )
	ROM_LOAD( "1136h.3f", 0x280000, 0x080000, CRC(58a662be) SHA1(3e2fc167bdee74ebfa63c3b1b0d822e3d898c30c) )
	ROM_LOAD( "1137h.3e", 0x300000, 0x080000, CRC(f96e0708) SHA1(e910970a4203b9b1943c853e3d869dd43cdfbc2d) )
	ROM_LOAD( "1138h.3d", 0x380000, 0x080000, CRC(633cff33) SHA1(aaf9ded832ae8889f413d3734edfcde099f9c319) )
	ROM_LOAD( "1139h.3c", 0x400000, 0x080000, CRC(be1189ca) SHA1(34b4102c6341ade03a1d44b6049ffa15666c6bb6) )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "1131h.1f", 0x080000, 0x080000, CRC(32f68241) SHA1(585b5e0d2d959af8b57ecc0a277aeda27e5cae9c) )	/* bank 2, 3*/
	ROM_LOAD( "1132h.1e", 0x100000, 0x080000, CRC(2de6363d) SHA1(2000328e41bc0261f19e02323434e9dfdc61013a) )	/* bank 4, 5*/
ROM_END


/***************************************************************************

HANAKANZASHI
(c)1996 DYNAX.INC
CPU : Z-80 (TMPZ84C015BF-8)
SOUND : MSM6295 YM2413
REAL TIME CLOCK : MSM6242

***************************************************************************/

ROM_START( hanakanz )
	ROM_REGION( 0x90000, REGION_CPU1, 0 )	/* Z80 Code */
	ROM_LOAD( "50720.5b",     0x00000, 0x80000, CRC(dc40fcfc) SHA1(32c8b3d23039ac47504c881552572f2c22afa585) )
	ROM_RELOAD(               0x10000, 0x80000 )

	ROM_REGION( 0x300000, REGION_GFX1, 0 )	/* blitter data */
	ROM_LOAD( "50730.8c",     0x000000, 0x80000, CRC(54e1731d) SHA1(c3f60c4412665b379b4b630ead576691d7b2a598) )
	ROM_LOAD( "50740.8b",     0x080000, 0x80000, CRC(999e70ce) SHA1(421c137b43522fbf9f3f5aa86692dc563af86880) )
	ROM_LOAD( "50750.10c",    0x100000, 0x80000, CRC(0e58bf9e) SHA1(5e04a637fc81fd48c6e1626ec06f2f1f4f52264a) )
	ROM_LOAD( "50760.10b",    0x180000, 0x80000, CRC(8fcb5da3) SHA1(86bd4f89e860cd476a026c21a87f34b7a208c539) )
	ROM_LOAD( "50770.12c",    0x200000, 0x80000, CRC(118e6baf) SHA1(8e14baa967af87a74558f80584b7d483c98112be) )
	ROM_LOAD( "50780.12b",    0x280000, 0x80000, CRC(6dfd8a86) SHA1(4d0c9f2028533ebe51f2963cb776bde5c802883e) )

	ROM_REGION( 0x80000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "50710.1c",     0x00000, 0x80000, CRC(72ae072f) SHA1(024af2ae6aa12b7f76d12a9c589f07ec7f47e395) )	/* 2 banks*/
ROM_END


static DRIVER_INIT( rongrong )
{
	/* Rong Rong seems to have a protection that works this way:
		- write 01 to port c2
		- write three times to f705 (a fixed command?)
		- write a parameter to f706
		- read the answer back from f601
		- write 00 to port c2
	   The parameter is read from RAM location 60d4, and the answer
	   is written back there. No matter what the protection device
	   does, it seems that making 60d4 always read 0 is enough to
	   bypass the protection. Actually, I'm wondering if this
	   version of the game might be a bootleg with the protection
	   patched.
	 */
	install_mem_read_handler(0, 0x60d4, 0x60d4, MRA_NOP);
}


GAMEX(1992, mmpanic,  0,       mmpanic,  mmpanic,  0,        ROT0, "Nakanihon + East Technology (Taito license)", "Monkey Mole Panic (USA)",                    GAME_NO_COCKTAIL )
GAMEX(1993, quizchq,  0,       quizchq,  quizchq,  0,        ROT0, "Nakanihon",                                   "Quiz Channel Question (Ver 1.00) (Japan)",   GAME_NO_COCKTAIL | GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAMEX(1993, quizchql, quizchq, quizchq,  quizchq,  0,        ROT0, "Nakanihon (Laxan license)",                   "Quiz Channel Question (Ver 1.23) (Taiwan[Q])", GAME_NO_COCKTAIL | GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAMEX(1994, quiz365,  0,       quiz365,  quiz365,  0,        ROT0, "Nakanihon",                                   "Quiz 365 (Hong Kong and Taiwan)",              GAME_NO_COCKTAIL | GAME_IMPERFECT_GRAPHICS | GAME_NOT_WORKING )
GAMEX(1994, rongrong, 0,       rongrong, rongrong, rongrong, ROT0, "Nakanihon",                                   "Rong Rong (Germany)",                        GAME_NO_COCKTAIL )
GAMEX(1995, nettoqc,  0,       nettoqc,  nettoqc,  0,        ROT0, "Nakanihon",                                   "Nettoh Quiz Champion (Japan)",               GAME_NO_COCKTAIL | GAME_IMPERFECT_COLORS )
GAMEX(1996, ddenlovr, 0,       ddenlovr, ddenlovr, 0,        ROT0, "Dynax",                                       "Don Den Lover Vol. 1 (Hong Kong)",           GAME_NO_COCKTAIL | GAME_IMPERFECT_COLORS )

GAMEX(1996, hanakanz, 0,       rongrong, rongrong, 0,        ROT0, "Dynax",     "Hanakanzashi (Japan)", GAME_NOT_WORKING )
