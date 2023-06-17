#define NEW_DRAWSPRITE 1
/* System 32 Video Hardware */

/* todo:

add linezoom, clipping window effects on bg tilemaps
fix sprite clipping effect? (outside area clip)
fix / improve alphablending
fix alphablending enable, amount etc. (sonic almost certainly shouldn't have it enabled ?)
fix / add row-select, linescroll
fix priorities properly (will need vmixer)
find rad rally title screen background
remaining colour problems (sonic?)
solid flag on tiles? (rad rally..)
background colour
any remaining glitches

*/

#include "driver.h"
#define MAX_COLOURS (16384)

/* Debugging flags and kludges*/
extern int system32_temp_kludge;
int priloop;

extern int multi32;

extern data16_t *sys32_spriteram16;
data8_t  *sys32_spriteram8; /* I maintain this to make drawing ram based sprites easier */
extern data16_t *system32_mixerregs[2];		/* mixer registers*/
data16_t *sys32_videoram;
data32_t *multi32_videoram;
data8_t sys32_ramtile_dirty[0x1000];
extern data16_t sys32_displayenable;
extern data16_t sys32_tilebank_external;
data16_t sys32_old_tilebank_external;

int sys32_tilebank_internal;
int sys32_old_tilebank_internal;

int sys32_paletteshift[4];
int sys32_palettebank[4];
int sys32_old_paletteshift[4];
int sys32_old_palettebank[4];

extern int system32_mixerShift;
int system32_screen_mode;
int system32_screen_old_mode;
int system32_allow_high_resolution;
static int sys32_old_brightness[2][3];
int sys32_brightness[2][3];

data8_t system32_dirty_window[0x100];
data8_t system32_windows[4][4];
data8_t system32_old_windows[4][4];

/* these are the various attributes a sprite can have, will decide which need to be global later, maybe put them in a struct */

static int sys32sprite_indirect_palette;
static int sys32sprite_indirect_interleave;
static int sys32sprite_is_shadow;
static int sys32sprite_rambasedgfx;
static int sys32sprite_8bpp;
static int sys32sprite_draw_colour_f;
static int sys32sprite_yflip;
static int sys32sprite_xflip;
static int sys32sprite_use_yoffset;
static int sys32sprite_use_xoffset;
static int sys32sprite_yalign;
static int sys32sprite_xalign;
static int sys32sprite_rom_height;
static int sys32sprite_rom_width;
static int sys32sprite_rom_bank_low;
static int sys32sprite_unknown_1;
static int sys32sprite_unknown_2;
/*static int sys32sprite_solid;*/
static int sys32sprite_screen_height;
static int sys32sprite_unknown_3;
static int sys32sprite_rom_bank_high;
static int sys32sprite_unknown_4;
static int sys32sprite_unknown_5;
static int sys32sprite_rom_bank_mid;
static int sys32sprite_screen_width;
static int sys32sprite_ypos;
static int sys32sprite_xpos;
static int sys32sprite_rom_offset;
static int sys32sprite_palette;
static int sys32sprite_monitor_select; /* multi32*/
static int sys32sprite_priority;

static data16_t *sys32sprite_table;

static int spritenum; /* used to go through the sprite list */
static int jump_x, jump_y; /* these are set during a jump command and sometimes used by the sprites afterwards */
static data16_t *spritedata_source; /* a pointer into spriteram */

#if !NEW_DRAWSPRITE
static UINT32 sys32sprite_x_zoom;
static UINT32 sys32sprite_y_zoom;
#else
static int sys32mon_old4, sys32mon_old8;
#endif

/*

this actually draws the sprite, and could probably be optimized quite a bit ;-)
currently zooming isn't supported etc.

*/
#if NEW_DRAWSPRITE

/** AT050703 new drawsprite (unproven, general testing required)*/
static INLINE void system32_draw_sprite ( struct mame_bitmap *bitmap, const struct rectangle *cliprect )
{
#define FP     20
#define FPONE  (1<<FP)
#define FPHALF (1<<(FP-1))

/* FP entry vaule(FPENT) should normally be 0.5(FPHALF) but it causes sprite gaps occationally.*/
#define FPENT  0

	static UINT32 idp_cache8[256];
	static UINT32 idp_cache4[16];
	static data16_t *idp_base, *idb_old=0;
	static int idi_old=-1;

	/* one-time*/
	int src_fw, src_fh;
	int dst_minx, dst_maxx, dst_miny, dst_maxy;
	int dst_skipx, dst_skipy, dst_x, dst_y, dst_lastx, dst_lasty; /* Buy Warren Spector's Deus Ex2. It's cool.*/
	int flipx, flipy;

	/* inner loop*/
	UINT8 *src_ptr;
	register int edx, eax, ecx;
	int src_fx, src_fdx, transparent_pen;
	UINT32 *pal_base;
	UINT32 *dst_ptr;
	UINT32 *dst_end;

	/* outter loop*/
	int src_fby, src_fdy;
	int dst_pitch;
	int src_pitch, src_fbx;
	UINT8 *src_base;
	UINT8 *src_end;
	int dst_w, dst_h;


	/* fill internal data structure with default values*/
	src_base  = memory_region(REGION_GFX2);
	src_end   = src_base + memory_region_length(REGION_GFX2);
	src_pitch = sys32sprite_rom_width;
	src_fw    = sys32sprite_rom_width;
	src_fh    = sys32sprite_rom_height;

	idp_base  = sys32sprite_table;
	pal_base  = Machine->gfx[0]->colortable;

	dst_ptr   = bitmap->base;
	/* Should be (bitmap->height + (2 * BITMAP_SAFETY))
	 * but BITMAP_SAFETY is defined in src/common.c,
	 * so inaccessible here...
	 * > BITMAP_SAFETY == 16 */
	dst_end   = dst_ptr + (bitmap->width * (bitmap->height + 32));
	dst_pitch = bitmap->rowpixels;
	dst_minx  = cliprect->min_x;
	dst_maxx  = cliprect->max_x;
	dst_miny  = cliprect->min_y;
	dst_maxy  = cliprect->max_y;
	dst_x     = sys32sprite_xpos;
	dst_y     = sys32sprite_ypos;
	dst_w     = sys32sprite_screen_width;
	dst_h     = sys32sprite_screen_height;

	flipx     = sys32sprite_xflip;
	flipy     = sys32sprite_yflip;
	transparent_pen   = 0;

	/* cull zero dimension and off-screen objects*/
	if (!src_fw || !src_fh || !dst_w || !dst_h) return;
	if (dst_x > dst_maxx || dst_y > dst_maxy) return;
	dst_lastx = dst_x + dst_w - 1;
	if (dst_lastx < dst_minx) return;
	dst_lasty = dst_y + dst_h - 1;
	if (dst_lasty < dst_miny) return;

	/* calculate zoom factors*/
	src_fw <<= FP;
	src_fh <<= FP;
	src_fdx = src_fw / dst_w;
	src_fdy = src_fh / dst_h;

	/* clip destination*/
	dst_skipx = 0;
	eax = dst_minx;  if ((eax -= dst_x) > 0) { dst_skipx = eax;  dst_w -= eax;  dst_x = dst_minx; }
	eax = dst_lastx; if ((eax -= dst_maxx) > 0) dst_w -= eax;
	dst_skipy = 0;
	eax = dst_miny;  if ((eax -= dst_y) > 0) { dst_skipy = eax;  dst_h -= eax;  dst_y = dst_miny; }
	eax = dst_lasty; if ((eax -= dst_maxy) > 0) dst_h -= eax;

	/* clip source (precision loss from MUL after DIV is intentional to maintain pixel consistency)*/
	if (flipx)
	{
		src_fbx = src_fw - FPENT - 1;
		src_fdx = -src_fdx;
	}
	else src_fbx = FPENT;
	src_fbx += dst_skipx * src_fdx;
	if (flipy)
	{
		src_fby = src_fh - FPENT - 1;
		src_fdy = -src_fdy;
	}
	else src_fby = FPENT;
	src_fby += dst_skipy * src_fdy;


	/* modify oddities*/
	/* if the gfx data is coming from RAM instead of ROM change the pointer*/
	if (sys32sprite_rambasedgfx)
	{
		src_base = sys32_spriteram8;
		src_end  = src_base + 0x20000; /* size of sys32_spriteram8 buffer */
		sys32sprite_rom_offset &= 0x1ffff; /* right mask?*/
	}

	if (sys32sprite_monitor_select)
	{
		pal_base += MAX_COLOURS;
		dst_x += system32_screen_mode ? 52*8 : 40*8;
	}

	if (!sys32sprite_8bpp)
	{
		src_pitch >>= 1;
		if (!sys32sprite_draw_colour_f) transparent_pen = 0x0f;
	}
	else
		if (!sys32sprite_draw_colour_f) transparent_pen = 0xff;

	if (!sys32sprite_is_shadow)
	{
		if (sys32sprite_indirect_palette)
		{
			/* update indirect palette cache if necessary*/
			if (!sys32sprite_8bpp)
			{
				if (idb_old != idp_base || sys32mon_old4 != sys32sprite_monitor_select)
				{
					idb_old = idp_base;
					sys32mon_old4 = sys32sprite_monitor_select;

					for (ecx=0; ecx<0x10; ecx+=2)
					{
						eax = idp_base[ecx];   edx = idp_base[ecx+1];
						eax &= 0x0fff;         edx &= 0x0fff; /* no apparent side-effect observed*/
						eax = pal_base[eax];   edx = pal_base[edx];
						idp_cache4[ecx] = eax; idp_cache4[ecx+1] = edx;
					}
				}
			}
			else
			{
				edx = *idp_base & 0xfff;
				if (idi_old != edx || sys32mon_old8 != sys32sprite_monitor_select)
				{
					idi_old = edx;
					sys32mon_old8 = sys32sprite_monitor_select;
					pal_base += edx;

					for (ecx=0; ecx<0x100; ecx+=2)
					{
						eax = pal_base[ecx];   edx = pal_base[ecx+1];
						idp_cache8[ecx] = eax; idp_cache8[ecx+1] = edx;
					}
				}
			}
		}
		else
			pal_base += sys32sprite_palette<<4;
	}
	else
		sys32sprite_indirect_palette = 0; /* make sure full-shadows and IDP's are mutually exclusive*/


	/* adjust insertion points and pre-entry constants*/
	src_base += sys32sprite_rom_offset;
	dst_ptr += dst_y * dst_pitch + dst_x + dst_w;
	dst_w = -dst_w;

	ecx = src_fby;
	src_fby += src_fdy;
	ecx >>= FP;
	src_ptr = src_base;
	ecx *= src_pitch;
	src_fx = src_fbx;
	edx    = src_fbx;
	src_ptr += ecx;
	ecx = dst_w;

	if (!sys32sprite_8bpp)
	{
		/* 4bpp*/
		edx >>= FP+1;

		if (sys32sprite_indirect_palette)
		{
			do {
				do {
					if ((src_ptr + edx) >= src_end) return;

					eax = src_ptr[edx];
					edx = src_fx;
					if (src_fx & FPONE) eax &= 0xf; else eax >>= 4;
					edx += src_fdx;
					src_fx += src_fdx;
					edx >>= FP+1;

					if (!eax || eax == transparent_pen) continue;
					if ((dst_ptr + ecx) >= dst_end) return;

					if (eax != 0x0e)
						dst_ptr[ecx] = idp_cache4[eax];
					else
					{
						eax = dst_ptr[ecx];
						eax = (eax>>9&0x7c00) | (eax>>6&0x03e0) | (eax>>3&0x001f);
						dst_ptr[ecx] = ((UINT32*)palette_shadow_table)[eax];
					}

				} while (++ecx);

				ecx = src_fby;      src_fby += src_fdy;
				ecx >>= FP;         dst_ptr += dst_pitch;
				ecx *= src_pitch;   src_fx = src_fbx;
				edx = src_fbx;
				src_ptr = src_base; edx >>= FP+1;
				src_ptr += ecx;     ecx = dst_w;

			} while (--dst_h);
		}
		else if (!sys32sprite_is_shadow)
		{
			do {
				do {
					if ((src_ptr + edx) >= src_end) return;

					eax = src_ptr[edx];
					edx = src_fx;
					if (src_fx & FPONE) eax &= 0xf; else eax >>= 4;
					edx += src_fdx;
					src_fx += src_fdx;
					edx >>= (FP+1);

					if (!eax || eax == transparent_pen) continue;
					if ((dst_ptr + ecx) >= dst_end) return;

					dst_ptr[ecx] = pal_base[eax];
				} while (++ecx);

				ecx = src_fby;      src_fby += src_fdy;
				ecx >>= FP;         dst_ptr += dst_pitch;
				ecx *= src_pitch;   src_fx = src_fbx;
				edx = src_fbx;
				src_ptr = src_base; edx >>= FP+1;
				src_ptr += ecx;     ecx = dst_w;

			} while (--dst_h);
		}
		else
		{
			do {
				do {
					if ((src_ptr + edx) >= src_end) return;

					eax = src_ptr[edx];
					edx = src_fx;
					if (src_fx & FPONE) eax &= 0xf; else eax >>= 4;
					edx += src_fdx;
					src_fx += src_fdx;
					edx >>= (FP+1);

					if (!eax || eax == transparent_pen) continue;
					if ((dst_ptr + ecx) >= dst_end) return;

					eax = dst_ptr[ecx];
					eax = (eax>>9&0x7c00) | (eax>>6&0x03e0) | (eax>>3&0x001f);

					dst_ptr[ecx] = ((UINT32*)palette_shadow_table)[eax];

				} while (++ecx);

				ecx = src_fby;      src_fby += src_fdy;
				ecx >>= FP;         dst_ptr += dst_pitch;
				ecx *= src_pitch;   src_fx = src_fbx;
				edx = src_fbx;
				src_ptr = src_base; edx >>= FP+1;
				src_ptr += ecx;     ecx = dst_w;

			} while (--dst_h);
		}
	}
	else
	{
		/* 8bpp*/
		edx >>= FP;
		src_fx += src_fdx;

		if (sys32sprite_indirect_palette)
		{
			do {
				do {
					if ((src_ptr + edx) >= src_end) return;

					eax = src_ptr[edx];
					edx = src_fx;
					src_fx += src_fdx;
					edx >>= FP;

					if (!eax || eax == 0xe0 || eax == transparent_pen) continue;
					if ((dst_ptr + ecx) >= dst_end) return;

					if (eax != 0xf0)
						dst_ptr[ecx] = idp_cache8[eax];
					else
					{
						eax = dst_ptr[ecx];
						eax = (eax>>9&0x7c00) | (eax>>6&0x03e0) | (eax>>3&0x001f);
						dst_ptr[ecx] = ((UINT32*)palette_shadow_table)[eax];
					}

				} while (++ecx);

				ecx = src_fby;      src_fby += src_fdy;
				ecx >>= FP;         dst_ptr += dst_pitch;
				ecx *= src_pitch;   src_fx = src_fbx;
				edx = src_fbx;      src_fx += src_fdx;
				src_ptr = src_base; edx >>= FP;
				src_ptr += ecx;     ecx = dst_w;

			} while (--dst_h);
		}
		else if (!sys32sprite_is_shadow)
		{
			do {
				do {
					if ((src_ptr + edx) >= src_end) return;

					eax = src_ptr[edx];
					edx = src_fx;
					src_fx += src_fdx;
					edx >>= FP;

					if (!eax || eax == transparent_pen) continue;
					if ((dst_ptr + ecx) >= dst_end) return;

					dst_ptr[ecx] = pal_base[eax];

				} while (++ecx);

				ecx = src_fby;      src_fby += src_fdy;
				ecx >>= FP;         dst_ptr += dst_pitch;
				ecx *= src_pitch;   src_fx = src_fbx;
				edx = src_fbx;      src_fx += src_fdx;
				src_ptr = src_base; edx >>= FP;
				src_ptr += ecx;     ecx = dst_w;

			} while (--dst_h);
		}
		else
		{
			do {
				do {
					if ((src_ptr + edx) >= src_end) return;

					eax = src_ptr[edx];
					edx = src_fx;
					src_fx += src_fdx;
					edx >>= FP;

					if (!eax || eax == transparent_pen) continue;
					if ((dst_ptr + ecx) >= dst_end) return;

					eax = dst_ptr[ecx];
					eax = (eax>>9&0x7c00) | (eax>>6&0x03e0) | (eax>>3&0x001f);

					dst_ptr[ecx] = ((UINT32*)palette_shadow_table)[eax];
				} while (++ecx);

				ecx = src_fby;      src_fby += src_fdy;
				ecx >>= FP;         dst_ptr += dst_pitch;
				ecx *= src_pitch;   src_fx = src_fbx;
				edx = src_fbx;      src_fx += src_fdx;
				src_ptr = src_base; edx >>= FP;
				src_ptr += ecx;     ecx = dst_w;

			} while (--dst_h);
		}
	}
#undef FP
#undef FPONE
#undef FPHALF
#undef FPENT
}

#else

/* old drawsprite (working and proven)*/
void system32_draw_sprite ( struct mame_bitmap *bitmap, const struct rectangle *cliprect ) {
	data8_t *sprite_gfxdata = memory_region ( REGION_GFX2 );
	UINT32 xsrc,ysrc;
	UINT32 xdst,ydst;
	/* um .. probably a better way to do this */
	struct GfxElement *gfx=Machine->gfx[0];
	const pen_t *paldata = &gfx->colortable[0];

	/* if the gfx data is coming from RAM instead of ROM change the pointer */
	if ( sys32sprite_rambasedgfx ) {
		sprite_gfxdata = sys32_spriteram8;
		sys32sprite_rom_offset &= 0x1ffff; /* right mask? */
	}

	ysrc = 0;
	ydst = 0;

	while ( ysrc < (sys32sprite_rom_height<<16) ) {
		int drawypos;
		xsrc = 0;
		xdst = 0;

		if (!sys32sprite_yflip) {
			drawypos = sys32sprite_ypos+ydst; /* no flip*/
			if (drawypos > cliprect->max_y) ysrc = sys32sprite_rom_height<<16; /* quit drawing if we've gone off the right*/
		}
		else {
			drawypos = sys32sprite_ypos+((sys32sprite_screen_height-1)-ydst); /* y flip*/
			if (drawypos < cliprect->min_y) ysrc = sys32sprite_rom_height<<16; /* quit drawing if we've gone off the left on a flipped sprite*/
		}

		if ((drawypos >= cliprect->min_y) && (drawypos <= cliprect->max_y)) {
			UINT32 *destline = (bitmap->line[drawypos]);

			while ( xsrc < (sys32sprite_rom_width<<16) ) {

				int drawxpos;

				if (!sys32sprite_xflip) {
					drawxpos = sys32sprite_xpos+xdst; /* no flip*/
					if (drawxpos > cliprect->max_x) xsrc = sys32sprite_rom_width<<16; /* quit drawing if we've gone off the right*/
				}
				else {
					drawxpos = sys32sprite_xpos+((sys32sprite_screen_width-1)-xdst); /* x flip*/
					if (drawxpos < cliprect->min_x) xsrc = sys32sprite_rom_width<<16; /* quit drawing if we've gone off the left on a flipped sprite*/
				}

				if ((drawxpos >= cliprect->min_x) && (drawxpos <= cliprect->max_x)) {
					int gfxdata;
					int data;
					int r,g,b;

					if (sys32sprite_monitor_select) drawxpos+=system32_screen_mode?52*8:40*8;
					if (!sys32sprite_8bpp) { /* 4bpp*/
						gfxdata = (sprite_gfxdata[sys32sprite_rom_offset+((xsrc>>16)/2)+(ysrc>>16)*(sys32sprite_rom_width/2)]);

						if (xsrc & 0x10000) gfxdata = gfxdata & 0x0f;
						else gfxdata = (gfxdata & 0xf0) >> 4;

						if ( (!sys32sprite_draw_colour_f && gfxdata == 0x0f) ) gfxdata = 0;
						if (sys32sprite_indirect_palette) {
							switch (gfxdata) {
							case 0x00:
								break;

							case 0x0f: /* Transparent*/
								break;

							case 0x0e: /* Shadow*/
								data=destline[drawxpos];

								r = ((data >> 16) & 0xff)*0.5;
								g = ((data >> 8) & 0xff)*0.5;
								b = ((data >> 0) & 0xff)*0.5;

								destline[drawxpos] =  MAKE_RGB(r,g,b);
								break;

							default:
								destline[drawxpos] =  paldata[(sys32sprite_table[gfxdata] & 0xfff)+(sys32sprite_monitor_select*MAX_COLOURS)];
								break;
							}
						}
						else {
							if (sys32sprite_is_shadow) {
								data=destline[drawxpos];

								r = ((data >> 16) & 0xff)*0.5;
								g = ((data >> 8) & 0xff)*0.5;
								b = ((data >> 0) & 0xff)*0.5;

								if (gfxdata) destline[drawxpos] =  MAKE_RGB(r,g,b);
							}
							else {

								switch (gfxdata) {
								case 0x00:
									break;
								default:
									destline[drawxpos] =  paldata[gfxdata + (sys32sprite_palette * 16)+(sys32sprite_monitor_select*MAX_COLOURS)];
									break;
								}

							}
						}
					}
					else { /* 8bpp*/
						gfxdata = (sprite_gfxdata[sys32sprite_rom_offset+(xsrc>>16)+(ysrc>>16)*(sys32sprite_rom_width)]);

						if ( (!sys32sprite_draw_colour_f) && (gfxdata == 0xff) ) gfxdata = 0;

						if (sys32sprite_indirect_palette) {
							switch (gfxdata) {
							case 0x00:
								break;
							case 0xe0: /* Transparent*/
								break;
							case 0xf0: /* Shadow*/
								data=destline[drawxpos];

								r = ((data >> 16) & 0xff)*0.5;
								g = ((data >> 8) & 0xff)*0.5;
								b = ((data >> 0) & 0xff)*0.5;

								destline[drawxpos] =  MAKE_RGB(r,g,b);
								break;
							default:
								destline[drawxpos] =  paldata[gfxdata+(sys32sprite_table[0] & 0xfff)+sys32sprite_monitor_select*MAX_COLOURS];
								break;
							}
						}
						else {
							if (sys32sprite_is_shadow) {
								data=destline[drawxpos];

								r = ((data >> 16) & 0xff)*0.5;
								g = ((data >> 8) & 0xff)*0.5;
								b = ((data >> 0) & 0xff)*0.5;

								if (gfxdata) destline[drawxpos] =  MAKE_RGB(r,g,b);
							}
							else {
								if (gfxdata) destline[drawxpos] =  paldata[gfxdata + (sys32sprite_palette * 16)+sys32sprite_monitor_select*MAX_COLOURS];
							}
						}
					} /* bpp */
				} /* xclipping */
				xsrc+=sys32sprite_x_zoom;
				xdst++;
			}
		}
		ysrc+=sys32sprite_y_zoom;
		ydst++;
	}
}

#endif

/* system32_get_sprite_info

this function is used to get information on the sprite from spriteram and call the
drawing functions

	spriteram Sprite Entry layout

	0:  ffffffff ffffffff  1:  HHHHHHHH WWWWWWWW  2:  hhhhhhhh hhhhhhhh  3:  wwwwwwww wwwwwwww
	4:  yyyyyyyy yyyyyyyy  5:  xxxxxxxx xxxxxxxx  6:  rrrrrrrr rrrrrrrr  7:  pppppppp pppppppp

	f = various flags
		xx------ -------- (0xc000) :  Command (00 for a sprite, other values would mean this isn't a sprite)
		--x----- -------- (0x2000) :  Sprite uses Indirect Palette (TRUSTED)
		---x---- -------- (0x1000) :  Sprite uses Indirect Palette which is Interleaved in Spritelist (GA2?)
		----x--- -------- (0x0800) :  Sprite is a shadow.  Uses upper 16 values of the sprite priority table
		-----x-- -------- (0x0400) :  Sprite GFX data comes from Spriteram, not ROM (TRUSTED)
		------x- -------- (0x0200) :  Sprite is 8bpp not 4bpp (TRUSTED)
		-------x -------- (0x0100) :  If NOT set colour in palette 0x0f is transparent (TRUSTED)
		-------- x------- (0x0080) :  Sprite Y-Flip (TRUSTED)
		-------- -x------ (0x0040) :  Sprite X-Flip (TRUSTED)
		-------- --x----- (0x0020) :  Use Y offset (offset set in last jump) (not trusted)
		-------- ---x---- (0x0010) :  Use X offset (offset set in last jump) (TRUSTED)
		-------- ----xx-- (0x000c) :  Y alignment. 00=Center, 10=Start, 01=End (TRUSTED)
		-------- ------xx (0x0003) :  X alignment. 00=Center, 10=Start, 01=End (TRUSTED)

	H = height of sprite in ROM
	W = width  of sprite in ROM (multiply by 4 to get screen width)

	System32:
	w = width to draw on SCREEN + extra attributes
		x------- -------- (0x8000) :  unknown
		-x------ -------- (0x4000) :  Bit 5 of Sprite ROM Bank (TRUSTED)
		--x----- -------- (0x2000) :  unknown
		---x---- -------- (0x1000) :  unknown
		----x--- -------- (0x0800) :  Bit 4 of Sprite ROM Bank (TRUSTED)
		-----xxx xxxxxxxx (0x07ff) :  Width to draw on screen (TRUSTED)

	Multi32:
	w = width to draw on SCREEN + extra attributes
		x------- -------- (0x8000) :  bit 5 of the sprite bank (TRUSTED)
		-x------ -------- (0x4000) :  unknown
		--x----- -------- (0x2000) :  Bit 4 of the sprite bank (TRUSTED)
		---x---- -------- (0x1000) :  unknown
		----x--- -------- (0x0800) :  Monitor selection for this sprite (TRUSTED)
		-----xxx xxxxxxxx (0x07ff) :  Width to draw on screen (TRUSTED)
	y = y-position (12-bit?, high bit = sign bit?)

	x = x-position (12-bit, high bit = sign bit)

	r = ROM Offset of GFX data (multiply by 4 to get real offset)

	p = Palette & Priority bits, I think these change depending on the mode (Direct or Indirect)
		DIRECT MODE *probably wrong, holoseum needed a kludge to work
		xxxxx--- -------- (0xf800) :  unknown
		-----xxx xxxx---- (0x07f0) :  palette #
		-------- ----xxxx (0x000f) :  unknown

*/

/** AT050703: minor clean-up's*/
static INLINE void system32_get_sprite_info ( struct mame_bitmap *bitmap, const struct rectangle *cliprect ) {
	/* get attributes */
	int mixerinput, sprite_palette_mask, sprite_priority_levels, sys32sprite_priority_lookup;

	sys32sprite_indirect_palette		= (spritedata_source[0]&0x2000) >> 13;
	sys32sprite_indirect_interleave		= (spritedata_source[0]&0x1000) >> 12;
	sys32sprite_is_shadow				= (spritedata_source[0]&0x0800) >> 11;
	sys32sprite_rambasedgfx				= (spritedata_source[0]&0x0400) >> 10;
	sys32sprite_8bpp					= (spritedata_source[0]&0x0200) >> 9;
	sys32sprite_draw_colour_f			= (spritedata_source[0]&0x0100) >> 8;
	sys32sprite_yflip					= (spritedata_source[0]&0x0080) >> 7;
	sys32sprite_xflip					= (spritedata_source[0]&0x0040) >> 6;
	sys32sprite_use_yoffset				= (spritedata_source[0]&0x0020) >> 5;
	sys32sprite_use_xoffset				= (spritedata_source[0]&0x0010) >> 4;
	sys32sprite_yalign					= (spritedata_source[0]&0x000c) >> 2;
	sys32sprite_xalign					= (spritedata_source[0]&0x0003) >> 0;

	sys32sprite_rom_height				= (spritedata_source[1]&0xff00) >> 8;
	sys32sprite_rom_width				= (spritedata_source[1]&0x00ff) >> 0;

	sys32sprite_rom_bank_low			= (spritedata_source[2]&0xf000) >> 12;
	sys32sprite_unknown_1				= (spritedata_source[2]&0x0800) >> 11;
	sys32sprite_unknown_2				= (spritedata_source[2]&0x0400) >> 10;
	sys32sprite_screen_height			= (spritedata_source[2]&0x03ff) >> 0;

	if (multi32) {
		sys32sprite_rom_bank_high			= (spritedata_source[3]&0x8000) >> 15;
		sys32sprite_unknown_3				= (spritedata_source[3]&0x4000) >> 14;
		sys32sprite_rom_bank_mid			= (spritedata_source[3]&0x2000) >> 13;
		sys32sprite_unknown_4				= (spritedata_source[3]&0x1000) >> 12;
		sys32sprite_monitor_select			= (spritedata_source[3]&0x0800) >> 11;
	}
	else {
		sys32sprite_unknown_3				= (spritedata_source[3]&0x8000) >> 15;
		sys32sprite_rom_bank_high			= (spritedata_source[3]&0x4000) >> 14;
		sys32sprite_unknown_4				= (spritedata_source[3]&0x2000) >> 13;
		sys32sprite_unknown_5				= (spritedata_source[3]&0x1000) >> 12;
		sys32sprite_rom_bank_mid			= (spritedata_source[3]&0x0800) >> 11;
		sys32sprite_monitor_select			= 0;
	}
	sys32sprite_screen_width			= (spritedata_source[3]&0x07ff) >> 0;

	sys32sprite_ypos					= (spritedata_source[4]&0xffff) >> 0;

	sys32sprite_xpos					= (spritedata_source[5]&0xffff) >> 0;

	sys32sprite_rom_offset				= (spritedata_source[6]&0xffff) >> 0;

	sprite_palette_mask=(1<<(system32_mixerShift+4))-1;
	sprite_priority_levels=system32_mixerregs[sys32sprite_monitor_select][0x4d/2]&2?15:3;
	mixerinput = (spritedata_source[7] >> (system32_mixerShift + 8)) & 0xf;
	sys32sprite_palette = (spritedata_source[7] >> 4) & sprite_palette_mask;
	sys32sprite_palette += (system32_mixerregs[sys32sprite_monitor_select][mixerinput] & 0x30)<<2;

	/* process attributes */

	sys32sprite_rom_width = sys32sprite_rom_width << 2;
	sys32sprite_rom_offset = sys32sprite_rom_offset | (sys32sprite_rom_bank_low << 16) | (sys32sprite_rom_bank_mid << 20) | (sys32sprite_rom_bank_high << 21);
	sys32sprite_rom_offset = sys32sprite_rom_offset << 2;

	/* Determine the sprites palette and priority.  The actual priority of the sprite is found by looking up
	   the sprite priority table in the mixer registers.  The lookup value is found by reading the first colour
	   in the sprites palette in the case of indirect sprites.  For direct sprites, the lookup value is found by
	   reading the sprite priority data.
	*/
	if (sys32sprite_indirect_palette) {
		if (sys32sprite_indirect_interleave) /* indirect mode where the table is included in the display list */
		{
			sys32sprite_table = spritedata_source+8;
			spritenum+=2;
		}
		else /* indirect mode where the display list contains an offset to the table */
		{
			sys32sprite_table = sys32_spriteram16 + ((spritedata_source[7] & ((1<<(8+system32_mixerShift))-1))*8);
		}
		if (sys32sprite_table[0]==0xffff) sys32sprite_priority_lookup=1;
		else sys32sprite_priority_lookup = (sys32sprite_table[0]>>(8+system32_mixerShift))&0xf;
	}
	else {
		/* If all of the palette bits are set, the sprite is a shadow.  This is a secondary
		   method to define sprite shadows alongside the sys32sprite_is_shadow bit.
		   Direct palette shadow sprites use the upper 16 values in the sprite priority lookup table. */
		if (sprite_palette_mask==((spritedata_source[7]>>4)&sprite_palette_mask)) sys32sprite_is_shadow=1;
		sys32sprite_priority_lookup = (spritedata_source[7]>>(system32_mixerShift+8))&0xf;
	}

	sys32sprite_priority = system32_mixerregs[sys32sprite_monitor_select][sys32sprite_priority_lookup&sprite_priority_levels]&0xf;
	if (sys32sprite_is_shadow && ((!strcmp(Machine->gamedrv->name,"f1en")) || (!strcmp(Machine->gamedrv->name,"f1lap")))) sys32sprite_is_shadow=0;  /* f1en turns this flag on the car sprites?*/

	if (sys32sprite_use_yoffset) sys32sprite_ypos += jump_y;
	if (sys32sprite_use_xoffset) sys32sprite_xpos += jump_x;

	/* adjust positions according to offsets if used (radm, radr, alien3, darkedge etc.) */

	/* adjust sprite positions based on alignment, pretty much straight from modeler */
	switch (sys32sprite_xalign) {
	case 0: /* centerX*/
	case 3:
		sys32sprite_xpos -= (sys32sprite_screen_width-1) / 2; /* this is trusted again spiderman truck door*/
		break;
	case 1: /* rightX*/
		sys32sprite_xpos -= sys32sprite_screen_width - 1;
		break;
	case 2: /* leftX*/
		break;
	}

	switch (sys32sprite_yalign) {
	case 0: /* centerY*/
	case 3:
		sys32sprite_ypos -= (sys32sprite_screen_height-1) / 2; /* this is trusted against alien3 energy bars*/
		break;
	case 1: /* bottomY*/
		sys32sprite_ypos -= sys32sprite_screen_height - 1;
		break;
	case 2: /* topY*/
		break;
	}

	sys32sprite_xpos &= 0x0fff;
	sys32sprite_ypos &= 0x0fff;

	/* sprite positions are signed */
	if (sys32sprite_ypos & 0x0800) sys32sprite_ypos -= 0x1000;
	if (sys32sprite_xpos & 0x0800) sys32sprite_xpos -= 0x1000;

	/* Inefficient sprite priority hack to get things working for now.  Will change to arrays later.
		Currently, draw_sprite is a lot more processor intensive and has a greater need for optimisation. */
	if (priloop==sys32sprite_priority)
		if (!multi32 || (multi32 && (readinputport(0xf)&(sys32sprite_monitor_select+1))>>sys32sprite_monitor_select))
			system32_draw_sprite ( bitmap, cliprect );
}

/* Sprite RAM

each entry in the sprite list is 16 bytes (8 words)
the sprite list itself consists of 4 main different types of entry
 a normal sprite

	0:  00------ --------  1:  -------- --------  2:  -------- --------  3:  -------- --------
	4:  -------- --------  5:  -------- --------  6:  -------- --------  7:  -------- --------

		(See Above for bit usage)

 a command to set the clipping area

	0:  01------ --------  1:  -------- --------  2:  -------- --------  3:  -------- --------
	4:  -------- --------  5:  -------- --------  6:  -------- --------  7:  -------- --------

		(to be filled in later)

 a jump command

	0:  10ujjjjj jjjjjjjj  1:  yyyyyyyy yyyyyyyy  2:  xxxxxxxx xxxxxxxx  3:  -------- --------
	4:  -------- --------  5:  -------- --------  6:  -------- --------  7:  -------- --------

		u = set sprite offset positions with this jump (alien3 proves this test is needed)
		j = sprite number to jump to
		y = sprite y offset to use (? bits) (only set if u = 1)
		x = sprite x offset to use (? bits) (only set if u = 1)

		other bits unused / unknown

 a terminate list command

	0:  11------ --------  1:  -------- --------  2:  -------- --------  3:  -------- --------
	4:  -------- --------  5:  -------- --------  6:  -------- --------  7:  -------- --------

		(other bits unused, list is terminated)

sprite ram can also contain palette look up data for the special indirect
palette modes, as well as sprite gfx data which is used instead of the gfx
in the roms if a bit in the sprite entry is set.

*/

void system32_process_spritelist ( struct mame_bitmap *bitmap, const struct rectangle *cliprect ) {
	int processed;
	int command;
	struct rectangle clip;

	/* set clipping defaults */
	clip.min_x = Machine->visible_area.min_x;
	clip.max_x = Machine->visible_area.max_x;
	clip.min_y = Machine->visible_area.min_y;
	clip.max_y = Machine->visible_area.max_y;

	processed = 0;
	spritenum = 0;

	while (spritenum < 0x20000/16) {
		spritedata_source = sys32_spriteram16 + 8 * spritenum;

		command = (spritedata_source[0] & 0xc000) >> 14;

		switch (command) {
		case 0x3: /* end of sprite list */
			/*				logerror ("SPRITELIST: terminated at sprite %06x\n", spritenum*16);*/
			spritenum = 60000; /* just set a high sprite number so we stop processing */
			break;
		case 0x2: /* jump to position in sprite list*/
			/*				logerror ("SPRITELIST: jump at sprite %06x to %06x extra data 0 %04x 1 %04x, 2 %04x 3 %04x 4 %04x 5 %04x 6 %04x 7 %04x\n", spritenum*16, (spritedata_source[0] & 0x1fff)*16, spritedata_source[0] & 0x2000, spritedata_source[1], spritedata_source[2], spritedata_source[3] , spritedata_source[4] , spritedata_source[5] ,spritedata_source[6] , spritedata_source[7] );*/
			spritenum = spritedata_source[0] & 0x1fff;
			if (spritedata_source[0] & 0x2000) {
				jump_y = spritedata_source[1];
				jump_x = spritedata_source[2];
			}
			break;
		case 0x1: /* set clipping registers */
			/*				logerror ("SPRITELIST: set clip regs at %06x extra data 0 %04x 1 %04x 2 %04x 3 %04x 4 %04x 5 %04x 6 %04x 7 %04x\n", spritenum*16, spritedata_source[0], spritedata_source[1],spritedata_source[2],spritedata_source[3],spritedata_source[4],spritedata_source[5],spritedata_source[6],spritedata_source[7]  );*/
			{

				if (spritedata_source[0] & 0x3000) /* alien 3 needs something like this ... */
				{
					clip.min_y = spritedata_source[0]& 0x0fff;
					clip.max_y = spritedata_source[1]& 0x0fff;
					clip.min_x = spritedata_source[2]& 0x0fff;
					clip.max_x = spritedata_source[3]& 0x0fff;

					if  (clip.max_y > Machine->visible_area.max_y) clip.max_y = Machine->visible_area.max_y;
					if  (clip.max_x > Machine->visible_area.max_x) clip.max_x = Machine->visible_area.max_x;
				}
				else {
					clip.min_x = Machine->visible_area.min_x;
					clip.max_x = Machine->visible_area.max_x;
					clip.min_y = Machine->visible_area.min_y;
					clip.max_y = Machine->visible_area.max_y;
				}

			}

			spritenum ++;
			break;
		case 0x0: /* draw sprite */
			/*				logerror ("SPRITELIST: draw sprite at %06x\n", spritenum*16 );*/
			system32_get_sprite_info (bitmap, &clip);
			spritenum ++;
			break;
		}

		processed++;
		if (processed > 0x20000/16) /* its dead ;-) */
		{
			/*			logerror ("SPRITELIST: terminated due to infinite loop\n");*/
			spritenum = 16384;
		};
	}
}

/* 0x31ff00 - 0x31ffff are video registers

tile banking is controlled by a register in here as well as a register external to the tilemap chip
which is mapped at 0xc0000e

	00 | rR-- -b--  ---- ----    |  b = tile bank low bit ( | 0x2000 ), not multi-32  r = screen resolution R also resolution?
	02 | ---- ----  ---- dddd    |  d = tilemap disable registers
	04 | bbbb bbbb  ???? SsRr       S = layer 3 rowselect enable
									s = layer 2 rowselect enable
									R = layer 3 rowscroll enable
									r = layer 2 rowscroll enable
									b = table bases
									jpark sets one of the ?
	06 |
	08 |
	0a |
	0c |
	0e |
	10 |
	12 | scroll x for tilemap 0
	14 |
	16 | scroll y for tilemap 0
	18 |
	1a | scroll x for tilemap 1
	1c |
	1e | scroll y for tilemap 1
	20 |
	22 | scroll x for tilemap 2
	24 |
	26 | scroll y for tilemap 2
	28 |
	2a | scroll x for tilemap 3
	2c |
	2e | scroll y for tilemap 3
	30 | scroll x offset tilemap 0
	32 | scroll y offset tilemap 0
	34 | scroll x offset tilemap 1
	36 | scroll y offset tilemap 1
	38 | scroll x offset tilemap 2
	3a | scroll y offset tilemap 2
	3c | scroll x offset tilemap 3
	3e | scroll y offset tilemap 3
	40 | pages 0 + 1 of tilemap 0
	42 | pages 2 + 3 of tilemap 0
	44 | pages 0 + 1 of tilemap 1
	46 | pages 2 + 3 of tilemap 1
	48 | pages 0 + 1 of tilemap 2
	4a | pages 2 + 3 of tilemap 2
	4c | pages 0 + 1 of tilemap 3
	4e | pages 2 + 3 of tilemap 4
	50 |
	52 |
	54 |
	56 |
	58 |
	5a |
	5c |
	5e |
	60 |
	62 |
	64 |
	66 |
	.... etc.. fill the rest in later

*/


/* Mixer Registers

00 ---- ---- ---- pppp  p = Sprite Priority Table
02 ---- ---- ---- pppp  p = Sprite Priority Table
04 ---- ---- ---- pppp  p = Sprite Priority Table
06 ---- ---- ---- pppp  p = Sprite Priority Table
08 ---- ---- ---- pppp  p = Sprite Priority Table
0a ---- ---- ---- pppp  p = Sprite Priority Table
0c ---- ---- ---- pppp  p = Sprite Priority Table
0e ---- ---- ---- pppp  p = Sprite Priority Table
10 ---- ---- ---- pppp  p = Sprite shadow? Priority Table
12 ---- ---- ---- pppp  p = Sprite shadow? Priority Table
14 ---- ---- ---- pppp  p = Sprite shadow? Priority Table
16 ---- ---- ---- pppp  p = Sprite shadow? Priority Table
18 ---- ---- ---- pppp  p = Sprite shadow? Priority Table
1a ---- ---- ---- pppp  p = Sprite shadow? Priority Table
1c ---- ---- ---- pppp  p = Sprite shadow? Priority Table
1e ---- ---- ---- pppp  p = Sprite shadow? Priority Table
20 ---- ---- ssss ssss  s = Mixershift?
22 ---- ssss bbbb pppp  Tilemap Palette Base + Shifting, b = bank, s = shift p = priority 0
24 ---- ssss bbbb pppp  p = priority 1
26 ---- ssss bbbb pppp  p = priority 2
28 ---- ssss bbbb pppp  p = priority 3
2a ---- ---- ---- f---  f = tilemap flip x 0
2c ---- ---- ---- f---  f = tilemap flip x 1
2e ---- ---- ---- f---  f = tilemap flip x 2
30 ---- ---- ---- f---  f = tilemap flip x 3
32 ---e ---- ---e ----  e = alpha enable 0
34 ---e ---- ---e ----  e = alpha enable 1
36 ---e ---- ---e ----  e = alpha enable 2
38 ---e ---- ---e ----  e = alpha enable 3
3a
3c
3e ---- ---- ---- ---w  w = tilemap wrap disable?
40 bbbb bbbb bbbb bbbb  b = brightness (red)
42 bbbb bbbb bbbb bbbb  b = brightness (green)
44 bbbb bbbb bbbb bbbb  b = brightness (blue)
46 bbbb bbbb bbbb bbbb  b = brightness? (layer?) 2?     or r ? (jpark)
48 bbbb bbbb bbbb bbbb  b = brightness? (layer?) 3?     or g ? (jpark)
4a bbbb bbbb bbbb bbbb  b = brightness? (layer?)        or b ? (jpark)
4c ---- ---- ---- --l-   l = number of sprite layers?16:4
4e bbbb bbbb ---- ----   b = alpha blend amount?

*/

/** AT050703: minor code shufflings*/
void system32_draw_text_layer ( struct mame_bitmap *bitmap, const struct rectangle *cliprect ) /* using this for now to save me tilemap system related headaches */
{
	int x,y;
	int textbank = sys32_videoram[0x01ff5c/2] & 0x0007;
	int tmaddress = (sys32_videoram[0x01ff5c/2] & 0x00f0) >> 4;

	int monitor_select, monitor_offset;
	struct GfxElement *gfx = Machine->gfx[1];
	struct GfxLayout *gfxlayout = Machine->drv->gfxdecodeinfo[1].gfxlayout;
	data8_t *txtile_gfxregion = memory_region(REGION_GFX3);
	data16_t* tx_tilemapbase = sys32_videoram + ((0x10000+tmaddress*0x1000) /2);

	if (multi32)
	{
		monitor_select = readinputport(0xf) & 3;
		monitor_offset = system32_screen_mode ? 52*8 : 40*8;
	}
	else
	{
		monitor_select = 1;
		monitor_offset = 0;
	}

	/* this register is like this

	 ---- ----  tttt -bbb

	 t = address of tilemap data (used by dbzvrvs)
	 b = address of tile gfx data (used by radmobile / radrally ingame, jpark)

	 */

	for (y = 0; y < 32 ; y++) {
		for (x = 0; x < 64 ; x++) {
			int data=tx_tilemapbase[x+y*64];
			int code = data & 0x01ff;
			int pal = (data>>9) & 0x7f;
			int drawypos, flip;

			pal += (((system32_mixerregs[0][0x10] & 0xf0) >> 4) * 0x40);

			code += textbank * 0x200;

			if (sys32_ramtile_dirty[code]) {
				decodechar(gfx, code, (data8_t*)txtile_gfxregion, gfxlayout);
				sys32_ramtile_dirty[code] = 0;
			}

			if (system32_temp_kludge != 1) {
				drawypos = y*8;
				flip = 0;
			}
			else /* holoseum, actually probably requires the sprites to be globally flipped + game ROT180 not the tilemap */
			{
				drawypos = 215-y*8;
				flip = 1;
			}

			if (monitor_select & 1)
				drawgfx(bitmap,gfx,code,pal,0,flip,(x<<3),drawypos,cliprect,TRANSPARENCY_PEN,0);

			/* Multi32: Draw the same text on Monitor B*/
			if (monitor_select & 2)
				drawgfx(bitmap,gfx,code,pal,0,flip,(x<<3)+monitor_offset,drawypos,cliprect,TRANSPARENCY_PEN,0);
		}
	}
}

READ16_HANDLER ( sys32_videoram_r ) {
	return sys32_videoram[offset];
}

WRITE16_HANDLER ( sys32_videoram_w ) {
	data8_t *txtile_gfxregion = memory_region(REGION_GFX3);

	COMBINE_DATA(&sys32_videoram[offset]);


	/* also write it to another region so its easier (imo) to work with the ram based tiles */
	if (ACCESSING_MSB)
		txtile_gfxregion[offset*2+1] = (data & 0xff00) >> 8;

	if (ACCESSING_LSB)
		txtile_gfxregion[offset*2] = (data & 0x00ff );

	/* each tile is 0x10 words */
	sys32_ramtile_dirty[offset / 0x10] = 1;

	system32_dirty_window[offset>>9]=1;

}

WRITE16_HANDLER ( sys32_spriteram_w ) {

	COMBINE_DATA(&sys32_spriteram16[offset]);

	/* also write it to another region so its easier to work with when drawing sprites with RAM based gfx */
	if (ACCESSING_MSB)
		sys32_spriteram8[offset*2+1] = (data & 0xff00) >> 8;

	if (ACCESSING_LSB)
		sys32_spriteram8[offset*2] = (data & 0x00ff );
}

/*

Tilemaps are made of 4 windows

each window is 32x16 in size

*/

UINT32 sys32_bg_map( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows ) {
	int page = 0;
	if( row<16 ) { /* top */
		if( col<32 ) page = 0; else page = 1;
	}
	else { /* bottom */
		if( col<32 ) page = 2; else page = 3;
	}

	return ((col & 31) + (row & 15) * 32) + page * 0x200;

}


static struct tilemap *system32_layer_tilemap[4];

static void get_system32_tile_info ( int tile_index, int layer ) {
	int tileno, s32palette;
	int page;
	int yxflip;
	int monitor=multi32?layer%2:0;

	page = tile_index >> 9;

	tileno = sys32_videoram[(tile_index&0x1ff)+system32_windows[layer][page]*0x200];
	s32palette = ((tileno & 0x1ff0) >> (sys32_paletteshift[layer]+4));
	yxflip = (tileno & 0xc000)>>14;

	tileno &= 0x1fff;

	if (multi32) {

		/*
		External tilebank register (0xc0000e)

		-------- x-------  Tilemap Layer 3 bank += 0x4000
		-------- -x------  Tilemap Layer 3 bank += 0x2000
		-------- --x-----  Tilemap Layer 2 bank += 0x4000
		-------- ---x----  Tilemap Layer 2 bank += 0x2000
		-------- ----x---  Tilemap Layer 1 bank += 0x4000
		-------- -----x--  Tilemap Layer 1 bank += 0x2000
		-------- ------x-  Tilemap Layer 0 bank += 0x4000
		-------- -------x  Tilemap Layer 0 bank += 0x2000
		*/

		tileno|=(sys32_tilebank_external>>(layer*2)&3)*0x2000;
	}
	else {
		if (sys32_tilebank_internal) tileno |= 0x2000;
		if (sys32_tilebank_external&1) tileno |= 0x4000;
	}

	/* Multi32: use palette_b for monitor 2*/
	SET_TILE_INFO(0,tileno,sys32_palettebank[layer]+s32palette+(monitor*MAX_COLOURS/0x10),TILE_FLIPYX(yxflip))
}

static void get_system32_layer0_tile_info(int tile_index) {
	get_system32_tile_info(tile_index,0);
}
static void get_system32_layer1_tile_info(int tile_index) {
	get_system32_tile_info(tile_index,1);
}
static void get_system32_layer2_tile_info(int tile_index) {
	get_system32_tile_info(tile_index,2);
}
static void get_system32_layer3_tile_info(int tile_index) {
	get_system32_tile_info(tile_index,3);
}

VIDEO_START( system32 ) {
	int i;

	system32_layer_tilemap[0] = tilemap_create(get_system32_layer0_tile_info,sys32_bg_map,TILEMAP_TRANSPARENT, 16, 16,64,32);
	tilemap_set_transparent_pen(system32_layer_tilemap[0],0);
	system32_layer_tilemap[1] = tilemap_create(get_system32_layer1_tile_info,sys32_bg_map,TILEMAP_TRANSPARENT, 16, 16,64,32);
	tilemap_set_transparent_pen(system32_layer_tilemap[1],0);
	system32_layer_tilemap[2] = tilemap_create(get_system32_layer2_tile_info,sys32_bg_map,TILEMAP_TRANSPARENT, 16, 16,64,32);
	tilemap_set_transparent_pen(system32_layer_tilemap[2],0);
	system32_layer_tilemap[3] = tilemap_create(get_system32_layer3_tile_info,sys32_bg_map,TILEMAP_TRANSPARENT, 16, 16,64,32);
	tilemap_set_transparent_pen(system32_layer_tilemap[3],0);

	sys32_spriteram8 = auto_malloc ( 0x20000 ); /* for ram sprites*/
	sys32_videoram = auto_malloc ( 0x20000 );

	for (i=0; i <= multi32; i++) {
		sys32_old_brightness[i][0] = 0;
		sys32_old_brightness[i][1] = 0;
		sys32_old_brightness[i][2] = 0;
		sys32_brightness[i][0] = 0xff;
		sys32_brightness[i][1] = 0xff;
		sys32_brightness[i][2] = 0xff;
	}

	for (i = 0; i < 0x100; i++)
		system32_dirty_window[i] = 1;

	return 0;
}

void system32_set_colour (int offset);
void multi32_set_colour (int offset, int monitor);

static void system32_recalc_palette( int monitor ) {
	int i;
	for (i = 0; i < MAX_COLOURS; i++) {
		if (multi32) multi32_set_colour (i,monitor);
		else system32_set_colour(i);
	}
}

void system32_draw_bg_layer ( struct mame_bitmap *bitmap, const struct rectangle *cliprect, int layer ) {
	int trans = 0;
	int alphaamount = 0;
	int rowscroll=0, rowselect=0;
	int monitor = multi32?layer%2:0;
	int monitor_res = 0;
	struct rectangle clip;

	if ((system32_mixerregs[monitor][(0x32+2*layer)/2] & 0x1010) == 0x1010) {
		trans = TILEMAP_ALPHA;
		alphaamount = 255-((((system32_mixerregs[monitor][0x4e/2])>>8) & 7) <<5); /*umm this is almost certainly wrong*/
		alpha_set_level(alphaamount);
	}

	/* rowselect / rowscroll

	outrunners road - works ok
	svf pitch - works ok with tilemap flip / clipping hack
	brival floor - seems ok
	arabfgt floor - seems ok, bit shakey
	rad rally mirror - reasonable but doesn't scroll smoothly
	rad mobile backgrounds - wrong?
	sonic title screen background - ok

	what effect does alien3 use? zooming instead?
	jurassic park enables rowscroll on one of the levels in the attract but its hard to see what for

	*/

	if (layer == 2) {
		rowscroll = (sys32_videoram[0x01FF04/2] & 0x0001);
		rowselect = (sys32_videoram[0x01FF04/2] & 0x0004)>>2;
	}

	if (layer == 3) {
		rowscroll = (sys32_videoram[0x01FF04/2] & 0x0002)>>1;
		rowselect = (sys32_videoram[0x01FF04/2] & 0x0008)>>3;
	}

	/* Switch to Machine->visible_area.max_x later*/
	monitor_res=system32_screen_mode?52*8:40*8;

	if (multi32) {
		/*			clip.min_x = Machine->visible_area.min_x;*/
		/*			clip.max_x = Machine->visible_area.max_x;*/
		clip.min_x = (layer%2)*monitor_res;
		clip.max_x = (layer%2+1)*monitor_res;
		clip.min_y = 0;
		clip.max_y = 28*8;
	}
	else {
		clip.min_x = Machine->visible_area.min_x;
		clip.max_x = Machine->visible_area.max_x;
		clip.min_y = Machine->visible_area.min_y;
		clip.max_y = Machine->visible_area.max_y;
	}

	if (rowscroll || rowselect) {
		int line;

		int tableaddress = sys32_videoram[0x01FF04/2]>>8;

		tableaddress = (tableaddress * 0x200);

		/* workaround for issue with system32_mixerregs. */
		/* sonic  - trips this during attract mode on the ice level demo, messes up layer 2 bg. */
		/* alien3 - trips this during the enter initials high score screen, flips the letters. */
		if ( strcmp(Machine->gamedrv->name,"sonic") && strcmp(Machine->gamedrv->name,"alien3") )
		{
			if ((system32_mixerregs[monitor][(0x32+layer*2)/2]&8)>>3) {
				if (layer==2) tilemap_set_flip(system32_layer_tilemap[layer], TILEMAP_FLIPX);
			}
		}

		for (line = 0; line < 224;line++) {
			int xscroll = (sys32_videoram[(0x01FF12+8*layer)/2]);
			int yscroll = (sys32_videoram[(0x01FF16+8*layer)/2]);

			clip.min_y = clip.max_y = line;

			if (rowscroll) xscroll+=(sys32_videoram[((tableaddress+(layer-2)*0x200)/2)+line]);
			if (rowselect) yscroll+=(sys32_videoram[((tableaddress+0x400+(layer-2)*0x200)/2)+line])-line;


			if ((system32_mixerregs[monitor][(0x32+layer*2)/2]&8)>>3) {
				/* disable wrap on this tilemap, should be done on the other too but its less important
				   this is a bit messy because mame has no core functionality for this without resorting
				   to tilemap_draw_roz which I can't do because of RGB_DIRECT, it might be wrong anyway,
				   maybe its using the system32 clipping windows somehow */
				if (layer == 3) {
					int x2;
					x2 =xscroll&0x7ff;
					x2 = 0x7ff-x2;
					if (x2 > 0x3ff) clip.min_x = 0;
					else clip.min_x = x2;
					clip.max_x = 320-1;
				}
			}
			/* Multi32: Shift layer 3's rowscroll left one screen so that it lines up*/
			tilemap_set_scrollx(system32_layer_tilemap[layer],0, (xscroll & 0x3ff));
			tilemap_set_scrolly(system32_layer_tilemap[layer],0, (yscroll & 0x1ff));
			tilemap_set_scrolldx(system32_layer_tilemap[layer], (sys32_videoram[(0x01FF30+layer*4)/2]&0x1ff)+monitor*monitor_res, -(sys32_videoram[(0x01FF30+layer*4)/2]&0x1ff)-monitor*monitor_res);
			tilemap_set_scrolldy(system32_layer_tilemap[layer], sys32_videoram[(0x01FF32+layer*4)/2]&0x1ff, -sys32_videoram[(0x01FF32+layer*4)/2]&0x1ff);
			tilemap_draw(bitmap,&clip,system32_layer_tilemap[layer],trans,0);
		}
	}
	else {
		/* Multi32: Shift layer 3's rowscroll left one screen so that it lines up*/
		tilemap_set_scrollx(system32_layer_tilemap[layer],0,((sys32_videoram[(0x01FF12+8*layer)/2]) & 0x3ff));
		tilemap_set_scrolly(system32_layer_tilemap[layer],0,((sys32_videoram[(0x01FF16+8*layer)/2]) & 0x1ff));
		tilemap_set_scrolldx(system32_layer_tilemap[layer], (sys32_videoram[(0x01FF30+layer*4)/2]&0x1ff)+monitor*monitor_res, -(sys32_videoram[(0x01FF30+layer*4)/2]&0x1ff)-monitor*monitor_res);
		tilemap_set_scrolldy(system32_layer_tilemap[layer], sys32_videoram[(0x01FF32+layer*4)/2]&0x1ff, -sys32_videoram[(0x01FF32+layer*4)/2]&0x1ff);
		tilemap_draw(bitmap,&clip,system32_layer_tilemap[layer],trans,0);
	}
}

VIDEO_UPDATE( system32 ) {
	int i;

	int monitor_setting;
	int monitor_display_start;
	int monitor_display_width;
	int monitor_vertical_offset;

	int sys32_tmap_disabled = sys32_videoram[0x1FF02/2] & 0x000f;

	int priority0 = (system32_mixerregs[0][0x22/2] & 0x000f);
	int priority1 = (system32_mixerregs[multi32][0x24/2] & 0x000f);
	int priority2 = (system32_mixerregs[0][0x26/2] & 0x000f);
	int priority3 = (system32_mixerregs[multi32][0x28/2] & 0x000f);
	int sys32_palette_dirty[2] = {0, 0};

	/* -------------------------------------- experimental wip code --------------------------------*/
	int tm,ii;

	#if NEW_DRAWSPRITE
		/** force IDP recache*/
		sys32mon_old8 = sys32mon_old4 = -1;
	#endif

	/* if the windows number used by a tilemap use change then that window of the tilemap needs to be considered dirty*/
	for (tm = 0; tm < 4; tm++) {
		system32_windows[tm][0] = (sys32_videoram[(0x01FF40+4*tm)/2] & 0x007f);
		system32_windows[tm][1] = (sys32_videoram[(0x01FF40+4*tm)/2] & 0x7f00)>>8;
		system32_windows[tm][2] = (sys32_videoram[(0x01FF42+4*tm)/2] & 0x007f);
		system32_windows[tm][3] = (sys32_videoram[(0x01FF42+4*tm)/2] & 0x7f00)>>8;

		if (system32_windows[tm][0] != system32_old_windows[tm][0]) {
			for (ii = 0x000 ; ii < 0x200 ; ii++) tilemap_mark_tile_dirty(system32_layer_tilemap[tm],ii);
		}
		if (system32_windows[tm][1] != system32_old_windows[tm][1]) {
			for (ii = 0x200 ; ii < 0x400 ; ii++) tilemap_mark_tile_dirty(system32_layer_tilemap[tm],ii);
		}
		if (system32_windows[tm][2] != system32_old_windows[tm][2]) {
			for (ii = 0x400 ; ii < 0x600 ; ii++) tilemap_mark_tile_dirty(system32_layer_tilemap[tm],ii);
		}
		if (system32_windows[tm][3] != system32_old_windows[tm][3]) {
			for (ii = 0x600 ; ii < 0x800 ; ii++) tilemap_mark_tile_dirty(system32_layer_tilemap[tm],ii);
		}

		/* if the actual windows are dirty we also need to mark them dirty in the tilemap*/
		if (system32_dirty_window [ system32_windows[tm][0] ]) {
			for (ii = 0x000 ; ii < 0x200 ; ii++) tilemap_mark_tile_dirty(system32_layer_tilemap[tm],ii);
		}
		if (system32_dirty_window [ system32_windows[tm][1] ]) {
			for (ii = 0x200 ; ii < 0x400 ; ii++) tilemap_mark_tile_dirty(system32_layer_tilemap[tm],ii);
		}
		if (system32_dirty_window [ system32_windows[tm][2] ]) {
			for (ii = 0x400 ; ii < 0x600 ; ii++) tilemap_mark_tile_dirty(system32_layer_tilemap[tm],ii);
		}
		if (system32_dirty_window [ system32_windows[tm][3] ]) {
			for (ii = 0x600 ; ii < 0x800 ; ii++) tilemap_mark_tile_dirty(system32_layer_tilemap[tm],ii);
		}

		system32_old_windows[tm][0] = system32_windows[tm][0];
		system32_old_windows[tm][1] = system32_windows[tm][1];
		system32_old_windows[tm][2] = system32_windows[tm][2];
		system32_old_windows[tm][3] = system32_windows[tm][3];
	}

	/* we can clean the dirty window markers now*/
	for (ii = 0; ii < 0x100; ii++)
		system32_dirty_window[ii] = 0;

	/* if the internal tilebank changed everything is dirty*/
	sys32_tilebank_internal = sys32_videoram[0x01FF00/2] & 0x0400;
	if (sys32_tilebank_internal != sys32_old_tilebank_internal) {
		tilemap_mark_all_tiles_dirty(system32_layer_tilemap[0]);
		tilemap_mark_all_tiles_dirty(system32_layer_tilemap[1]);
		tilemap_mark_all_tiles_dirty(system32_layer_tilemap[2]);
		tilemap_mark_all_tiles_dirty(system32_layer_tilemap[3]);
	}
	sys32_old_tilebank_internal = sys32_tilebank_internal;

	/* if the external tilebank changed everything is dirty*/

	if  ( (sys32_tilebank_external) != sys32_old_tilebank_external ) {
		tilemap_mark_all_tiles_dirty(system32_layer_tilemap[0]);
		tilemap_mark_all_tiles_dirty(system32_layer_tilemap[1]);
		tilemap_mark_all_tiles_dirty(system32_layer_tilemap[2]);
		tilemap_mark_all_tiles_dirty(system32_layer_tilemap[3]);
	}
	sys32_old_tilebank_external = sys32_tilebank_external;

	/* if the palette shift /bank registers changed the tilemap is dirty, not sure these are regs 100% correct some odd colours in sonic / jpark*/
	for (tm = 0; tm < 4; tm++) {
		int monitor=multi32?tm%2:0;
		sys32_paletteshift[tm] = (system32_mixerregs[monitor][(0x22+tm*2)/2] & 0x0f00)>>8;
		if (sys32_paletteshift[tm] != sys32_old_paletteshift[tm]) {
			tilemap_mark_all_tiles_dirty(system32_layer_tilemap[tm]);
			sys32_old_paletteshift[tm] = sys32_paletteshift[tm];
		}

		sys32_palettebank[tm] = ((system32_mixerregs[monitor][(0x22+tm*2)/2] & 0x00f0)>>4)*0x40;
		if (sys32_palettebank[tm] != sys32_old_palettebank[tm]) {
			tilemap_mark_all_tiles_dirty(system32_layer_tilemap[tm]);
			sys32_old_palettebank[tm] = sys32_palettebank[tm];
		}
	}
	/*---------------------------------------- end wip code -----------------------------------------------*/

	/* palette dirty check */

	for (i=0; i <= multi32; i++) {
		sys32_brightness[i][0] = (system32_mixerregs[i][0x40/2]);
		sys32_brightness[i][1] = (system32_mixerregs[i][0x42/2]);
		sys32_brightness[i][2] = (system32_mixerregs[i][0x44/2]);

		if (sys32_brightness[i][0] != sys32_old_brightness[i][0]) {
			sys32_old_brightness[i][0] = sys32_brightness[i][0]; sys32_palette_dirty[i] = 1;
		}
		if (sys32_brightness[i][1] != sys32_old_brightness[i][1]) {
			sys32_old_brightness[i][1] = sys32_brightness[i][1]; sys32_palette_dirty[i] = 1;
		}
		if (sys32_brightness[i][2] != sys32_old_brightness[i][2]) {
			sys32_old_brightness[i][2] = sys32_brightness[i][2]; sys32_palette_dirty[i] = 1;
		}

		if (sys32_palette_dirty[i]) {
			sys32_palette_dirty[i] = 0;
			system32_recalc_palette(i);
		}
	}

	/* end palette dirty*/

	system32_screen_mode = sys32_videoram[0x01FF00/2] & 0xc000;  /* this should be 0x8000 according to modeler but then brival is broken?  this way alien3 and arabfgt try to change when they shouldn't .. wrong register?*/

	if (multi32) {
		monitor_setting=readinputport(0xf);
		monitor_vertical_offset=1;
		monitor_display_start=0;
		if (monitor_setting==2) monitor_display_start=1;
		if (monitor_setting==3) {
			monitor_vertical_offset=2;
			monitor_display_width=2;
		}
		else
			monitor_display_width=1+monitor_display_start;
	}
	else {
		monitor_display_start=0;
		monitor_display_width=1;
		monitor_vertical_offset=1;
	}

	fillbitmap(bitmap, 0, 0);

	if (system32_screen_mode && system32_allow_high_resolution) {
		set_visible_area(52*monitor_display_start*8, 52*8*monitor_display_width-1, 0, 28*8*monitor_vertical_offset-1);
	}
	else {
		set_visible_area(40*monitor_display_start*8, 40*8*monitor_display_width-1, 0, 28*8*monitor_vertical_offset-1);
	}

	fillbitmap(bitmap, 0, 0);

	/* Rad Rally (title screen) and Rad Mobile (Winners don't use drugs) use a bitmap ... */
	/* i think this is wrong tho, rad rally enables it on the 2nd title screen when the
	   data isn't complete */

	if (sys32_videoram[0x01FF00/2] & 0x0800)  /* wrong? */
	{
		int xcnt, ycnt;
		static UINT32 *destline;
		struct GfxElement *gfx=Machine->gfx[0];

		const pen_t *paldata = &gfx->colortable[0];
		static pen_t palcopy[MAX_COLOURS];
		static int copy_videoram[57505];

		if (paldata[0] == 16316664 && palcopy[0] != 16316664) /* copy palette and videoram for radr in ready state */
		{
			int i;
			for(i = 0; i < MAX_COLOURS; i++)
				 palcopy[i] = paldata[i];

			for(i = 0; i < 57505; i++)
				copy_videoram[i] = sys32_videoram[i];
		}
    
		for ( ycnt = 0 ; ycnt < 224 ; ycnt ++ )
		{
			destline = (UINT32 *)(bitmap->line[ycnt]);


			for ( xcnt = 0 ; xcnt < 160 ; xcnt ++ )
			{
				int data2;

				if (!strcmp(Machine->gamedrv->name,"radr")) /* replace with copies */
				{
					data2 = copy_videoram[256*ycnt+xcnt];
					destline[xcnt*2+1] = palcopy[(data2 >> 8)+(0x100*0x1d)]; /* 1d00 */
					destline[xcnt*2] = palcopy[(data2 &0xff)+(0x100*0x1d)];
				}
				else if (!strcmp(Machine->gamedrv->name,"radm"))
				{
					data2 = sys32_videoram[256*ycnt+xcnt];
					destline[xcnt*2+1] = paldata[(data2 >> 8)+(0x100*0x1d)]; /* 1d00 */
					destline[xcnt*2] = paldata[(data2 &0xff)+(0x100*0x1d)];
				}
			}

		}
	}



	/* Priority loop.  Draw layers 1 and 3 on Multi32's Monitor B */
	if (sys32_displayenable & 0x0002) {
		for (priloop=0; priloop < 0x10; priloop++) {
			if (priloop == priority0 && (!multi32 || (multi32 && (readinputport(0xf)&1)))) {
				if (!(sys32_tmap_disabled & 0x1)) system32_draw_bg_layer (bitmap,cliprect,0);
			}
			if (priloop == priority1 && (!multi32 || (multi32 && (readinputport(0xf)&2)>>1))) {
				if (!(sys32_tmap_disabled & 0x2)) system32_draw_bg_layer (bitmap,cliprect,1);
			}
			if (priloop == priority2 && (!multi32 || (multi32 && (readinputport(0xf)&1)))) {
				if (!(sys32_tmap_disabled & 0x4)) {
          if ((!strcmp(Machine->gamedrv->name,"jpark")) && priloop==0xe ) system32_draw_bg_layer (bitmap,cliprect,1); /* mix jeep to both layers */
          system32_draw_bg_layer (bitmap,cliprect,2);
        }
			}
			if (priloop == priority3 && (!multi32 || (multi32 && (readinputport(0xf)&2)>>1))) {
				if (!(sys32_tmap_disabled & 0x8)) system32_draw_bg_layer (bitmap,cliprect,3);
			}
			system32_process_spritelist (bitmap, cliprect);
		}
	}
	system32_draw_text_layer (bitmap, cliprect);

#if 0
	{

		/* custom log */

		static FILE *sys32_logfile;

		/* provide errorlog from here on */
		sys32_logfile = fopen("sys32vid.log","wa");

		int x;

	  /*	x = rand(); */

		fprintf(sys32_logfile,"Video Regs 0x31ff00 - 0x31ffff\n");
		for (x = 0x1ff00; x< 0x20000; x+=2)
		{
			fprintf(sys32_logfile, "%04x\n", sys32_videoram[x/2] ) ;

		}
		fprintf(sys32_logfile,"Mixer Regs 0x610000 - 0x6100ff\n");
		for (x = 0x00; x< 0x100; x+=2)
		{
			fprintf(sys32_logfile, "%04x\n", system32_mixerregs[0][x/2] ) ;

		}


		fclose (sys32_logfile);
	}
#endif

}
