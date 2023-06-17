/*

Varia Metal

Notes:

It has Sega and Taito logos in the roms ?!

whats going on with the dipswitches
also has a strange sound chip + oki

scrolling behavior is incorrect, see background on second attract demo
tilemap priorities can change

spriteram clear or list markers? (i clear it after drawing each sprite at the moment)

cleanup


---

Excellent Systems 'Varia Metal'
board ID ES-9309B-B

main cpu 68000 @ 16Mhz

sound oki m6295 (rom VM8)
      es8712    (rom VM7)

program roms VM5 and VM6

graphics VM1-VM4

roms are 23C160 except for code and OKI 27C4001


*/

#include "driver.h"

int vmetal_es8712_start;
int vmetal_es8712_end;
data16_t *vmetal_texttileram;
data16_t *vmetal_mid1tileram;
data16_t *vmetal_mid2tileram;
data16_t *vmetal_tlookup;
data16_t *vmetal_videoregs;


static struct tilemap *vmetal_texttilemap;
static struct tilemap *vmetal_mid1tilemap;
static struct tilemap *vmetal_mid2tilemap;

static data16_t *varia_spriteram16;
static data16_t *vmetal_es8712;



MACHINE_INIT( vmetal_reset )
{
	vmetal_es8712_start = 0;
	vmetal_es8712_end = 0;
}


READ16_HANDLER ( varia_crom_read )
{
	/* game reads the cgrom, result is 7772, verified to be correct on the real board */

	data8_t *cgrom = memory_region(REGION_GFX1);
	data16_t retdat;
	offset = offset << 1;
	offset |= (vmetal_videoregs[0x0ab/2]&0x7f) << 16;
	retdat = (cgrom[offset] <<8)| (cgrom[offset+1]);

	return retdat;
}


READ16_HANDLER ( varia_random )
{
	return 0xffff;
}



static void get_vmetal_tlookup(UINT16 data, UINT16 *tileno, UINT16 *color)
{
	int idx = ((data & 0x7fff) >> 4)*2;
	UINT32 lookup = (vmetal_tlookup[idx]<<16) | vmetal_tlookup[idx+1];
	*tileno = (data & 0xf) | ((lookup>>2) & 0xfff0);
	*color = (lookup>>20) & 0xff;
}

/* sprite format

sprites are non-tile based

4 words per sprite

 -------- --------    -------- --------    -------- --------    -------- --------
       xx xxxxxxxx           y yyyyyyyy    Ff             oo    oooooooo oooooooo


  x = xpos
  y = ypos
  f = flipy
  F = flipx
  o = offset (64 byte sprite gfxdata boundaries)




*/

static void varia_drawsprites( struct mame_bitmap *bitmap, const struct rectangle *cliprect )
{

	data16_t *finish = varia_spriteram16;
	data16_t *source = finish + 0x1000/2;

	UINT16 *destline;

	const struct GfxElement *gfx = Machine->gfx[0];

	data8_t *srcgfx;

	int gfxoffs;

	while( source>finish )
	{
		int x,y,xloop,yloop;
		UINT16 height, width;
		int tile;
		int flipy, flipx;
		int colour;
		source -= 0x04;
		x = source[0]&0x3ff;
		y = source[1]& 0x1ff;
		flipy = source[2]&0x4000;
		flipx = source[2]&0x8000;
		gfxoffs = 0;
		tile=(source[3]) | ((source[2]&0x3)<<16);
		srcgfx= gfx->gfxdata+(64*tile);
		width = (((source[2]&0x3000) >> 12)+1)*16;
		height =(((source[2]&0x0600) >> 9)+1)*16;
		colour =((source[2]&0x00f0) >> 4);

		y-=64;
		x-=64;


		for (yloop=0; yloop<height; yloop++)
		{
			UINT16 drawypos;

			if (!flipy) {drawypos = y+yloop;} else {drawypos = (y+height-1)-yloop;}

			destline = (UINT16 *)(bitmap->line[drawypos]);


			for (xloop=0; xloop<width; xloop++)
			{
				UINT16 drawxpos;
				int pixdata;
				pixdata = srcgfx[gfxoffs];

				if (!flipx) { drawxpos = x+xloop; } else { drawxpos = (x+width-1)-xloop; }

				if ((drawxpos >= cliprect->min_x) && (drawxpos <= cliprect->max_x) && (drawypos >= cliprect->min_y) && (drawypos <= cliprect->max_y) && (pixdata!=15))
					destline[drawxpos] = pixdata + ((0x80+colour)*16);


				gfxoffs++;
			}


		}

		/* I see no end of list marker or register ... so I'm clearing the sprite ram after I draw each sprite */
	}
}

WRITE16_HANDLER( vmetal_texttileram_w )
{
	COMBINE_DATA(&vmetal_texttileram[offset]);
	tilemap_mark_tile_dirty(vmetal_texttilemap,offset);
}

WRITE16_HANDLER( vmetal_mid1tileram_w )
{
	COMBINE_DATA(&vmetal_mid1tileram[offset]);
	tilemap_mark_tile_dirty(vmetal_mid1tilemap,offset);
}
WRITE16_HANDLER( vmetal_mid2tileram_w )
{
	COMBINE_DATA(&vmetal_mid2tileram[offset]);
	tilemap_mark_tile_dirty(vmetal_mid2tilemap,offset);
}

WRITE16_HANDLER( paletteram16_GGGGGRRRRRBBBBBx_word_w )
{
	int r,g,b;
	UINT16 datax;
	COMBINE_DATA(&paletteram16[offset]);
	datax = paletteram16[offset];

	r = (datax >>  6) & 0x1f;
	g = (datax >> 11) & 0x1f;
	b = (datax >>  1) & 0x1f;

	r = (r << 3) | (r >> 2);
	g = (g << 3) | (g >> 2);
	b = (b << 3) | (b >> 2);

	palette_set_color(offset,r,g,b);
}

static READ16_HANDLER ( varia_dips_bit8_r ) { return ((readinputport(3) & 0x80) << 0) | ((readinputport(2) & 0x80) >> 1); }
static READ16_HANDLER ( varia_dips_bit7_r ) { return ((readinputport(3) & 0x40) << 1) | ((readinputport(2) & 0x40) >> 0); }
static READ16_HANDLER ( varia_dips_bit6_r ) { return ((readinputport(3) & 0x20) << 2) | ((readinputport(2) & 0x20) << 1); }
static READ16_HANDLER ( varia_dips_bit5_r ) { return ((readinputport(3) & 0x10) << 3) | ((readinputport(2) & 0x10) << 2); }
static READ16_HANDLER ( varia_dips_bit4_r ) { return ((readinputport(3) & 0x08) << 4) | ((readinputport(2) & 0x08) << 3); }
static READ16_HANDLER ( varia_dips_bit3_r ) { return ((readinputport(3) & 0x04) << 5) | ((readinputport(2) & 0x04) << 4); }
static READ16_HANDLER ( varia_dips_bit2_r ) { return ((readinputport(3) & 0x02) << 6) | ((readinputport(2) & 0x02) << 5); }
static READ16_HANDLER ( varia_dips_bit1_r ) { return ((readinputport(3) & 0x01) << 7) | ((readinputport(2) & 0x01) << 6); }


static WRITE16_HANDLER( vmetal_es8712_w )
{
	/* Many samples in the ADPCM ROM are actually not used.

	Snd			Offset Writes				  Sample Range
		 0000 0004 0002 0006 000a 0008 000c
	--   ----------------------------------   -------------
	00   006e 0001 00ab 003c 0002 003a 003a   01ab6e-023a3c
	01   003d 0002 003a 001d 0002 007e 007e   023a3d-027e1d
	02   00e2 0003 0005 002e 0003 00f3 00f3   0305e2-03f32e
	03   000a 0005 001e 00f6 0005 00ec 00ec   051e0a-05ecf6
	04   00f7 0005 00ec 008d 0006 0060 0060   05ecf7-06608d
	05   0016 0008 002e 0014 0009 0019 0019   082e16-091914
	06   0015 0009 0019 0094 000b 0015 0015   091915-0b1594
	07   0010 000d 0012 00bf 000d 0035 0035   0d1210-0d35bf
	08   00ce 000e 002f 0074 000f 0032 0032   0e2fce-0f3274
	09   0000 0000 0000 003a 0000 007d 007d   000000-007d3a
	0a   0077 0000 00fa 008d 0001 00b6 00b6   00fa77-01b68d
	0b   008e 0001 00b6 00b3 0002 0021 0021   01b68e-0221b3
	0c   0062 0002 00f7 0038 0003 00de 00de   02f762-03de38
	0d   00b9 0005 00ab 00ef 0006 0016 0016   05abb9-0616ef
	0e   00dd 0007 0058 00db 0008 001a 001a   0758dd-081adb
	0f   00dc 0008 001a 002e 0008 008a 008a   081adc-088a2e
	10   00db 0009 00d7 00ff 000a 0046 0046   09d7db-0a46ff
	11   0077 000c 0003 006d 000c 0080 0080   0c0377-0c806d
	12   006e 000c 0080 006c 000d 0002 0002   0c806e-0d026c
	13   006d 000d 0002 002b 000d 0041 0041   0d026d-0d412b
	14   002c 000d 0041 002a 000d 00be 00be   0d412c-0dbe2a
	15   002b 000d 00be 0029 000e 0083 0083   0dbe2b-0e8329
	16   002a 000e 0083 00ee 000f 0069 0069   0e832a-0f69ee
	*/

	COMBINE_DATA(&vmetal_es8712[offset]);
	log_cb(RETRO_LOG_DEBUG, LOGPRE "Writing %04x to ES8712 port %02x\n",data,offset);

	if ((offset == 0) && (ACCESSING_MSB))
	{
		ADPCM_stop(0);
		vmetal_es8712_start = 0;
		vmetal_es8712_end = 0;
	}

	if ((offset == 0x06) && ACCESSING_LSB)
	{
		vmetal_es8712_start  = ((vmetal_es8712[0] & 0x00ff) << 0);
		vmetal_es8712_start |= ((vmetal_es8712[1] & 0x00ff) << 8);
		vmetal_es8712_start |= ((vmetal_es8712[2] & 0x000f) << 16);


		vmetal_es8712_end    = ((vmetal_es8712[3] & 0x00ff) << 0);
		vmetal_es8712_end   |= ((vmetal_es8712[4] & 0x00ff) << 8);
		vmetal_es8712_end   |= ((vmetal_es8712[5] & 0x000f) << 16);

		/*	The MSB of offset 0x0000 is unreliable in setting the ADPCM bank.
			How is it really done? Let's do it manually for now */
		switch (vmetal_es8712_start)
		{
			case 0x000000:
			case 0x00fa77:
			case 0x01b68e:
			case 0x02f762:
			case 0x05abb9:
			case 0x0758dd:
			case 0x081adc:
			case 0x09d7db:
			case 0x0c0377:
			case 0x0c806e:
			case 0x0d026d:
			case 0x0d412c:
			case 0x0dbe2b:
			case 0x0e832a:	vmetal_es8712_start |= 0x100000; vmetal_es8712_end |= 0x100000; break;
			default:		break;
		}

		log_cb(RETRO_LOG_DEBUG, LOGPRE "Start=%08x  End=%08x  Length=%08x\n",vmetal_es8712_start,vmetal_es8712_end,vmetal_es8712_end - vmetal_es8712_start);

		if (vmetal_es8712_start < vmetal_es8712_end)
		{
			ADPCM_stop(0);
			ADPCM_play(0, vmetal_es8712_start, vmetal_es8712_end - vmetal_es8712_start);
		}
	}
}

static INTERRUPT_GEN( vmetal_interrupt )
{
	/* Loop any music playback - probably wrong */
	if (vmetal_es8712_end && (ADPCM_playing(0) == 0))
	{
		ADPCM_play(0, vmetal_es8712_start, vmetal_es8712_end - vmetal_es8712_start);
	}
}


static MEMORY_READ16_START( varia_readmem )
    { 0x000000, 0x0fffff, MRA16_RAM },
	{ 0x100000, 0x11ffff, MRA16_RAM },
	{ 0x120000, 0x13ffff, MRA16_RAM },
	{ 0x140000, 0x15ffff, MRA16_RAM },
	{ 0x160000, 0x16ffff, varia_crom_read },
	{ 0x170000, 0x171fff, MRA16_RAM },
	{ 0x172000, 0x173fff, MRA16_RAM }, 
	{ 0x174000, 0x174fff, MRA16_RAM },
	{ 0x175000, 0x177fff, MRA16_RAM },
	{ 0x178000, 0x1787ff, MRA16_RAM },
	{ 0x178800, 0x17ffff, MRA16_RAM },
	{ 0x200000, 0x200001, input_port_0_word_r },
	{ 0x200002, 0x200003, input_port_1_word_r },

	/* i have no idea whats meant to be going on here .. it seems to read one bit of the dips from some of them, protection ??? */
	{ 0x30fffe, 0x30ffff, varia_random }, 
	{ 0x317ffe, 0x317fff, varia_random }, 
	{ 0x31bffe, 0x31bfff, varia_random }, 
	{ 0x31dffe, 0x31dfff, varia_random }, 
	{ 0x31effe, 0x31efff, varia_random }, 
	{ 0x31f7fe, 0x31f7ff, varia_random }, 
	{ 0x31fbfe, 0x31fbff, varia_random }, 
	{ 0x31fdfe, 0x31fdff, varia_random }, 
	{ 0x31fefe, 0x31feff, varia_dips_bit8_r }, 
	{ 0x31ff7e, 0x31ff7f, varia_dips_bit7_r }, 
	{ 0x31ffbe, 0x31ffbf, varia_dips_bit6_r },
	{ 0x31ffde, 0x31ffdf, varia_dips_bit5_r },
	{ 0x31ffee, 0x31ffef, varia_dips_bit4_r }, 
	{ 0x31fff6, 0x31fff7, varia_dips_bit3_r },
	{ 0x31fffa, 0x31fffb, varia_dips_bit2_r },
	{ 0x31fffc, 0x31fffd, varia_dips_bit1_r },
	{ 0x31fffe, 0x31ffff, varia_random },
	{ 0x400000, 0x400001, OKIM6295_status_0_lsb_r },
	{ 0xff0000, 0xffffff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( varia_writemem )
    { 0x000000, 0x0fffff, MWA16_RAM },
	{ 0x100000, 0x11ffff, vmetal_texttileram_w, &vmetal_texttileram },
	{ 0x120000, 0x13ffff, vmetal_mid1tileram_w, &vmetal_mid1tileram },
	{ 0x140000, 0x15ffff, vmetal_mid2tileram_w, &vmetal_mid2tileram },
	{ 0x170000, 0x171fff, MWA16_RAM },
	{ 0x172000, 0x173fff, paletteram16_GGGGGRRRRRBBBBBx_word_w, &paletteram16 },
	{ 0x174000, 0x174fff, MWA16_RAM, &varia_spriteram16 },
	{ 0x175000, 0x177fff, MWA16_RAM },
	{ 0x178000, 0x1787ff, MWA16_RAM, &vmetal_tlookup },
	{ 0x178800, 0x17ffff, MWA16_RAM, &vmetal_videoregs },
	{ 0x200000, 0x200001, MWA16_NOP },
	{ 0x400000, 0x400001, OKIM6295_data_0_lsb_w  },
	{ 0x400002, 0x400003, OKIM6295_data_0_lsb_w },
	{ 0x500000, 0x50000d, vmetal_es8712_w, &vmetal_es8712 },
	{ 0xff0000, 0xffffff, MWA16_RAM },
MEMORY_END

INPUT_PORTS_START( varia )
	PORT_START	/* DSW */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START2  )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START		/* IN0 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_SERVICE2 ) 
	PORT_BIT( 0xffe0, IP_ACTIVE_LOW, IPT_UNKNOWN ) 

	PORT_START		/* Dips 1 */
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 3C_1C )  )
	PORT_DIPSETTING(      0x0006, DEF_STR( 2C_1C )  )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C )  )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_2C )  )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_3C )  )
	PORT_DIPSETTING(      0x0002, DEF_STR( 1C_4C )  )
	PORT_DIPSETTING(      0x0001, DEF_STR( 1C_5C )  )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C )  )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START		/* Dips 2 */
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x000c, 0x000c, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0008, "0"  )
	PORT_DIPSETTING(      0x0004, "1"  )
	PORT_DIPSETTING(      0x000c, "2"  )
	PORT_DIPSETTING(      0x0000, "3"  )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )
INPUT_PORTS_END



static struct GfxLayout char16x16layout =
{
	16,16,
	RGN_FRAC(1,4),
	4,
	{ 0,1,2,3 },
	{ RGN_FRAC(1,4)+4, RGN_FRAC(1,4)+0, RGN_FRAC(1,4)+12, RGN_FRAC(1,4)+8, RGN_FRAC(3,4)+4, RGN_FRAC(3,4)+0, RGN_FRAC(3,4)+12, RGN_FRAC(3,4)+8, 4, 0, 12, 8, RGN_FRAC(2,4)+4, RGN_FRAC(2,4)+0, RGN_FRAC(2,4)+12, RGN_FRAC(2,4)+8 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16, 8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	16*16
};

static struct GfxLayout char8x8layout =
{
	8,8,
	RGN_FRAC(1,4),
	4,
	{ 0,1,2,3 },
	{ 4, 0, 12, 8, RGN_FRAC(2,4)+4, RGN_FRAC(2,4)+0, RGN_FRAC(2,4)+12, RGN_FRAC(2,4)+8 },
	{ RGN_FRAC(1,4)+0*16,0*16, RGN_FRAC(1,4)+1*16, 1*16, RGN_FRAC(1,4)+2*16,  2*16, RGN_FRAC(1,4)+3*16, 3*16 },
	4*16
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &char16x16layout,   0x0, 512  }, /* bg tiles */
	{ REGION_GFX1, 0, &char8x8layout,   0x0, 512  }, /* bg tiles */
	{ -1 } /* end of array */
};


static struct OKIM6295interface okim6295_interface =
{
	1,					/* 1 chip */
	{ 10000 },			/* frequency (Hz) */
	{ REGION_SOUND1 },	/* memory region */
	{ 75 }
};

static struct ADPCMinterface adpcm_interface =
{
	1,
	12000,
	REGION_SOUND2,
	{ 75 }
};

static void get_vmetal_texttilemap_tile_info(int tile_index)
{
	UINT32 tile;
	UINT16 color, data = vmetal_texttileram[tile_index];
	int idx = ((data & 0x7fff) >> 4)*2;
	UINT32 lookup = (vmetal_tlookup[idx]<<16) | vmetal_tlookup[idx+1];
	tile = (data & 0xf) | (lookup & 0x7fff0);
	color = ((lookup>>20) & 0x1f)+0xe0;
	if (data & 0x8000) tile = 0;
	SET_TILE_INFO(1, tile, color, TILE_FLIPYX(0x0));
}


static void get_vmetal_mid1tilemap_tile_info(int tile_index)
{
	UINT16 tile, color, data = vmetal_mid1tileram[tile_index];
	get_vmetal_tlookup(data, &tile, &color);
	if (data & 0x8000) tile = 0;
	SET_TILE_INFO(0, tile, color, TILE_FLIPYX(0x0));
}
static void get_vmetal_mid2tilemap_tile_info(int tile_index)
{
	UINT16 tile, color, data = vmetal_mid2tileram[tile_index];
	get_vmetal_tlookup(data, &tile, &color);
	if (data & 0x8000) tile = 0;
	SET_TILE_INFO(0, tile, color, TILE_FLIPYX(0x0));
}

VIDEO_START(varia)
{
	vmetal_texttilemap = tilemap_create(get_vmetal_texttilemap_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,      8, 8, 256,256);
	vmetal_mid1tilemap = tilemap_create(get_vmetal_mid1tilemap_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,      16,16, 256,256);
	vmetal_mid2tilemap = tilemap_create(get_vmetal_mid2tilemap_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,      16,16, 256,256);
	tilemap_set_transparent_pen(vmetal_texttilemap,15);
	tilemap_set_transparent_pen(vmetal_mid1tilemap,15);
	tilemap_set_transparent_pen(vmetal_mid2tilemap,15);

	return 0;
}

VIDEO_UPDATE(varia)
{
	fillbitmap(bitmap, get_black_pen(), cliprect);

	tilemap_set_scrollx(vmetal_mid2tilemap,0, vmetal_videoregs[0x06a/2] /*+ vmetal_videoregs[0x066/2]*/);
	tilemap_set_scrollx(vmetal_mid1tilemap,0, vmetal_videoregs[0x07a/2] /*+ vmetal_videoregs[0x076/2]*/);


	tilemap_draw(bitmap,cliprect,vmetal_mid1tilemap,0,0);
	tilemap_draw(bitmap,cliprect,vmetal_mid2tilemap,0,0);
	varia_drawsprites (bitmap,cliprect);
	tilemap_draw(bitmap,cliprect,vmetal_texttilemap,0,0);
}

VIDEO_EOF(varia)
{
	memset(varia_spriteram16, 0, 0x1000);
}


static MACHINE_DRIVER_START( varia )
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_MEMORY(varia_readmem, varia_writemem)
	MDRV_CPU_VBLANK_INT(irq1_line_hold,1)
	MDRV_CPU_PERIODIC_INT(vmetal_interrupt, 240)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(vmetal_reset)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(2048, 2048)
	MDRV_VISIBLE_AREA(0, 319, 0, 223)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x1000)

	MDRV_VIDEO_START(varia)
	MDRV_VIDEO_UPDATE(varia)
	MDRV_VIDEO_EOF(varia)

	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
	MDRV_SOUND_ADD(ADPCM, adpcm_interface)
MACHINE_DRIVER_END



ROM_START( vmetal )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "vm5.bin", 0x00001, 0x80000, CRC(43ef844e) SHA1(c673f34fcc9e406282c9008795b52d01a240099a) )
	ROM_LOAD16_BYTE( "vm6.bin", 0x00000, 0x80000, CRC(cb292ab1) SHA1(41fdfe67e6cb848542fd5aa0dfde3b1936bb3a28) )

	ROM_REGION( 0x800000, REGION_GFX1, 0 )
	ROM_LOAD( "vm1.bin", 0x000000, 0x200000, CRC(b470c168) SHA1(c30462dc134da1e71a94b36ef96ecd65c325b07e) )
	ROM_LOAD( "vm2.bin", 0x200000, 0x200000, CRC(b36f8d60) SHA1(1676859d0fee4eb9897ce1601a2c9fd9a6dc4a43) )
	ROM_LOAD( "vm3.bin", 0x400000, 0x200000, CRC(00fca765) SHA1(ca9010bd7f59367e483868018db9a9abf871386e) )
	ROM_LOAD( "vm4.bin", 0x600000, 0x200000, CRC(5a25a49c) SHA1(c30781202ec882e1ec6adfb560b0a1075b3cce55) )

	ROM_REGION( 0x080000, REGION_SOUND1, 0 ) /* OKI6295 Samples */
	/* Second half is junk */
	ROM_LOAD( "vm8.bin", 0x00000, 0x80000, CRC(c14c001c) SHA1(bad96b5cd40d1c34ef8b702262168ecab8192fb6) )

	ROM_REGION( 0x200000, REGION_SOUND2, 0 ) /* Samples */
	ROM_LOAD( "vm7.bin", 0x00000, 0x200000, CRC(a88c52f1) SHA1(d74a5a11f84ba6b1042b33a2c156a1071b6fbfe1) )
ROM_END

GAMEX( 1995, vmetal, 0, varia, varia, 0, ROT270, "Excellent Systems", "Varia Metal", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
