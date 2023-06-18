/*
    (Some) Data East 32 bit 156 CPU ARM based games:

    Heavy Smash
    World Cup Volleyball 95
	World Cup Volleyball 95 Extra Version

    See also deco32.c, deco_mlc.c, backfire.c

    Emulation by Bryan McPhail, mish@tendril.co.uk
*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "decocrpt.h"
#include "deco32.h"
#include "machine/eeprom.h"
#include "cpu/arm/arm.h"
#include "deco16ic.h"

extern void deco156_decrypt(void);

static int wcvol95_bank_callback(const int bank)
{
	return ((bank>>4)&0x7) * 0x1000;
}

static VIDEO_START( wcvol95 )
{
	/* allocate the ram as 16-bit (we do it here because the CPU is 32-bit) */
	deco16_pf1_data      = (UINT16*)auto_malloc(0x2000);
	deco16_pf2_data      = (UINT16*)auto_malloc(0x2000);
	deco16_pf1_rowscroll = (UINT16*)auto_malloc(0x800);
	deco16_pf2_rowscroll = (UINT16*)auto_malloc(0x800);
	deco16_pf12_control  = (UINT16*)auto_malloc(0x10);
	paletteram16         = (UINT16*)auto_malloc(0x1000);

	deco16_1_video_init();

	deco16_set_tilemap_bank_callback(0, wcvol95_bank_callback);
	deco16_set_tilemap_bank_callback(1, wcvol95_bank_callback);

	return 0;
}

/* spriteram is really 16-bit.. this can be changed to use 16-bit ram like the tilemaps
 its the same sprite chip Data East used on many, many 16-bit era titles */
static void draw_sprites(struct mame_bitmap *bitmap,const struct rectangle *cliprect)
{
	int offs;

	for (offs = (0x1400/4)-4;offs >= 0;offs -= 4) /* 0x1400 for charlien */
	{
		int x,y,sprite,colour,multi,fx,fy,inc,flash,mult, pri;

		sprite = spriteram32[offs+1]&0xffff;

		y = spriteram32[offs]&0xffff;
		flash=y&0x1000;
		if (flash && (cpu_getcurrentframe() & 1)) continue;

		x = spriteram32[offs+2]&0xffff;
		colour = (x >>9) & 0x1f;

		pri = (x&0xc000); /* 2 bits or 1? */

		switch (pri&0xc000) {
		case 0x0000: pri=0; break;
		case 0x4000: pri=0xf0; break;
		case 0x8000: pri=0xf0|0xcc; break;
		case 0xc000: pri=0xf0|0xcc; break; /*  or 0xf0|0xcc|0xaa ? */
		}

		fx = y & 0x2000;
		fy = y & 0x4000;
		multi = (1 << ((y & 0x0600) >> 9)) - 1;	/* 1x, 2x, 4x, 8x height */

		x = x & 0x01ff;
		y = y & 0x01ff;
		if (x >= 320) x -= 512;
		if (y >= 256) y -= 512;
		y = 240 - y;
		x = 304 - x;

		if (x>320) continue;

		sprite &= ~multi;
		if (fy)
			inc = -1;
		else
		{
			sprite += multi;
			inc = 1;
		}

		if (1)
		{
			y=240-y;
			x=304-x;
			if (fx) fx=0; else fx=1;
			if (fy) fy=0; else fy=1;
			mult=16;
		}
		else mult=-16;

		while (multi >= 0)
		{
			pdrawgfx(bitmap,Machine->gfx[2],
				sprite - multi * inc,
				colour,
				fx,fy,
				x,y + mult * multi,
				cliprect,TRANSPARENCY_PEN,0,pri);

			multi--;
		}
	}
}

static VIDEO_UPDATE( wcvol95 )
{
	fillbitmap(priority_bitmap,0,cliprect); 
	fillbitmap(bitmap,0,cliprect);

	deco16_pf12_update(deco16_pf1_rowscroll,deco16_pf2_rowscroll);

	deco16_tilemap_2_draw(bitmap,cliprect,0,0);
	draw_sprites(bitmap,cliprect);
	deco16_tilemap_1_draw(bitmap,cliprect,0,0);
}

/***************************************************************************/

static WRITE32_HANDLER(hvysmsh_eeprom_w)
{
	{
		OKIM6295_set_bank_base(1, 0x40000 * (data & 0x7) );

		EEPROM_set_clock_line((data & 0x20) ? ASSERT_LINE : CLEAR_LINE);
		EEPROM_write_bit(data & 0x10);
		EEPROM_set_cs_line((data & 0x40) ? CLEAR_LINE : ASSERT_LINE);
	}
}

static WRITE32_HANDLER( hvysmsh_oki_0_bank_w )
{
	OKIM6295_set_bank_base(0, (data & 1) * 0x40000);
}

static WRITE32_HANDLER(wcvol95_eeprom_w)
{
	{
		EEPROM_set_clock_line((data & 0x2) ? ASSERT_LINE : CLEAR_LINE);
		EEPROM_write_bit(data & 0x1);
		EEPROM_set_cs_line((data & 0x4) ? CLEAR_LINE : ASSERT_LINE);
	}
}


static WRITE32_HANDLER(hvysmsh_nonbuffered_palette_w)
{
	int r,g,b;

	COMBINE_DATA(&paletteram32[offset]);

	b = (paletteram32[offset] >>16) & 0xff;
	g = (paletteram32[offset] >> 8) & 0xff;
	r = (paletteram32[offset] >> 0) & 0xff;

	palette_set_color(offset,r,g,b);
}

static WRITE32_HANDLER(wcvol95_nonbuffered_palette_w)
{
	int r,g,b;

	COMBINE_DATA(&paletteram32[offset]);

	b = (paletteram32[offset] >>10) & 0x1f;
	g = (paletteram32[offset] >> 5) & 0x1f;
	r = (paletteram32[offset] >> 0) & 0x1f;

	palette_set_color(offset,r<<3,g<<3,b<<3);
}


/***************************************************************************/
static READ32_HANDLER( wcvol95_pf1_rowscroll_r ) { return deco16_pf1_rowscroll[offset]^0xffff0000; }
static READ32_HANDLER( wcvol95_pf2_rowscroll_r ) { return deco16_pf2_rowscroll[offset]^0xffff0000; }
static WRITE32_HANDLER( wcvol95_pf1_rowscroll_w ) { data &=0x0000ffff; mem_mask &=0x0000ffff; COMBINE_DATA(&deco16_pf1_rowscroll[offset]); }
static WRITE32_HANDLER( wcvol95_pf2_rowscroll_w ) { data &=0x0000ffff; mem_mask &=0x0000ffff; COMBINE_DATA(&deco16_pf2_rowscroll[offset]); }
static READ32_HANDLER ( wcvol95_pf12_control_r ) { return deco16_pf12_control[offset]^0xffff0000; }
static WRITE32_HANDLER( wcvol95_pf12_control_w ) { data &=0x0000ffff; mem_mask &=0x0000ffff; COMBINE_DATA(&deco16_pf12_control[offset]); }
static READ32_HANDLER( wcvol95_pf1_data_r ) {	return deco16_pf1_data[offset]^0xffff0000; }
static READ32_HANDLER( wcvol95_pf2_data_r ) {	return deco16_pf2_data[offset]^0xffff0000; }
static WRITE32_HANDLER( wcvol95_pf1_data_w ) { data &=0x0000ffff; mem_mask &=0x0000ffff; deco16_pf1_data_w(offset,data,mem_mask); }
static WRITE32_HANDLER( wcvol95_pf2_data_w ) { data &=0x0000ffff; mem_mask &=0x0000ffff; deco16_pf2_data_w(offset,data,mem_mask); }

static READ32_HANDLER( input_r )
{
	UINT32 ret = 0;
	static int vblank = 0;

	ret  = (readinputport(1) << 16) | readinputport(0);
	ret &= ~0x01100000; /* eeprom & vblank */
	ret |= vblank;
	ret |= EEPROM_read_bit() << 24;

	vblank ^= 0x00100000;

	return ret;
}

static WRITE32_HANDLER( hvysmsh_sound_0_w )
{
	OKIM6295_data_0_w(0, data & 0xff);
}

static READ32_HANDLER( hvysmsh_sound_0_r )
{
	return OKIM6295_status_0_r(0);
}

static WRITE32_HANDLER( hvysmsh_sound_1_w )
{
	OKIM6295_data_1_w(0, data & 0xff);
}

static READ32_HANDLER( hvysmsh_sound_1_r )
{
	return OKIM6295_status_1_r(0);
}

static WRITE32_HANDLER( wcvol95_sound_w )
{
	if (mem_mask==0xffffff00) {
		if (offset)
			YMZ280B_data_0_w(0,data & 0xff);
		else
			YMZ280B_register_0_w(0,data & 0xff);
	} else {
		log_cb(RETRO_LOG_DEBUG, LOGPRE "%08x:  non-byte written to sound %08x mask %08x\n",activecpu_get_pc(),data,mem_mask);
	}
}

static READ32_HANDLER( wcvol95_sound_r )
{
	if (mem_mask==0xffffff00) {
		return YMZ280B_status_0_r(0);
	} else {
		log_cb(RETRO_LOG_DEBUG, LOGPRE "%08x:  non-byte read from sound mask %08x\n",activecpu_get_pc(),mem_mask);
	}

	return 0;
}

static MEMORY_READ32_START( hvysmsh_readmem )
    { 0x000000, 0x0fffff, MRA32_ROM },
	{ 0x100000, 0x107fff, MRA32_RAM },
	{ 0x120000, 0x120003, input_r },
	{ 0x140000, 0x140003, hvysmsh_sound_0_r },
	{ 0x160000, 0x160003, hvysmsh_sound_1_r },
	{ 0x180000, 0x18001f, wcvol95_pf12_control_r },
	{ 0x190000, 0x191fff, wcvol95_pf1_data_r },
	{ 0x194000, 0x195fff, wcvol95_pf2_data_r },
	{ 0x1a0000, 0x1a0fff, wcvol95_pf1_rowscroll_r },
	{ 0x1a4000, 0x1a4fff, wcvol95_pf2_rowscroll_r },
	{ 0x1c0000, 0x1c0fff, MRA32_RAM },
	{ 0x1d0010, 0x1d002f, MRA32_NOP }, /* Check for DMA complete? */
	{ 0x1e0000, 0x1e1fff, MRA32_RAM },
MEMORY_END

static MEMORY_WRITE32_START( hvysmsh_writemem )
    { 0x000000, 0x0fffff, MWA32_ROM },
	{ 0x100000, 0x107fff, MWA32_RAM },
	{ 0x120000, 0x120003, MWA32_NOP }, /* Volume control in low byte */
	{ 0x120004, 0x120007, hvysmsh_eeprom_w },
	{ 0x120008, 0x12000b, MWA32_NOP }, /* IRQ ack? */
	{ 0x12000c, 0x12000f, hvysmsh_oki_0_bank_w },
	{ 0x140000, 0x140003, hvysmsh_sound_0_w },
	{ 0x160000, 0x160003, hvysmsh_sound_1_w },
	{ 0x180000, 0x18001f, wcvol95_pf12_control_w },
	{ 0x190000, 0x191fff, wcvol95_pf1_data_w },
	{ 0x194000, 0x195fff, wcvol95_pf2_data_w },
	{ 0x1a0000, 0x1a0fff, wcvol95_pf1_rowscroll_w },
	{ 0x1a4000, 0x1a4fff, wcvol95_pf2_rowscroll_w },
	{ 0x1c0000, 0x1c0fff, hvysmsh_nonbuffered_palette_w, &paletteram32 },
	{ 0x1e0000, 0x1e1fff, MWA32_RAM, &spriteram32, &spriteram_size },
MEMORY_END

static MEMORY_READ32_START( wcvol95_readmem )
    { 0x000000, 0x0fffff, MRA32_ROM },
	{ 0x100000, 0x10001f, wcvol95_pf12_control_r },
	{ 0x110000, 0x111fff, wcvol95_pf1_data_r },
	{ 0x114000, 0x115fff, wcvol95_pf2_data_r },
	{ 0x120000, 0x120fff, wcvol95_pf1_rowscroll_r },
	{ 0x124000, 0x124fff, wcvol95_pf2_rowscroll_r },
	{ 0x130000, 0x137fff, MRA32_RAM },
	{ 0x140000, 0x140003, input_r },
	{ 0x160000, 0x161fff, MRA32_RAM },
	{ 0x170000, 0x170003, MRA32_NOP }, /* Irq ack? */
	{ 0x180000, 0x180fff, MRA32_RAM },
	{ 0x1a0000, 0x1a0007, wcvol95_sound_r },
MEMORY_END

static MEMORY_WRITE32_START( wcvol95_writemem )
    { 0x000000, 0x0fffff, MWA32_ROM },
	{ 0x100000, 0x10001f, wcvol95_pf12_control_w },
	{ 0x110000, 0x111fff, wcvol95_pf1_data_w },
	{ 0x114000, 0x115fff, wcvol95_pf2_data_w },
	{ 0x120000, 0x120fff, wcvol95_pf1_rowscroll_w },
	{ 0x124000, 0x124fff, wcvol95_pf2_rowscroll_w },
	{ 0x130000, 0x137fff, MWA32_RAM },
	{ 0x150000, 0x150003, wcvol95_eeprom_w },
	{ 0x160000, 0x161fff, MWA32_RAM, &spriteram32, &spriteram_size },
	{ 0x170000, 0x170003, MWA32_NOP }, /* Irq ack? */
{ 0x180000, 0x180fff, wcvol95_nonbuffered_palette_w, &paletteram32 },
	{ 0x1a0000, 0x1a0007, wcvol95_sound_w },
MEMORY_END

/***************************************************************************/

INPUT_PORTS_START( hvysmsh )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_SERVICE_NO_TOGGLE( 0x0008, IP_ACTIVE_LOW )
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_SPECIAL )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON5| IPF_PLAYER1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON6| IPF_PLAYER1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON4| IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON5| IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON6| IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( wcvol95 )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_SERVICE_NO_TOGGLE( 0x0008, IP_ACTIVE_LOW )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNUSED ) /* 'soundmask' */
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_SPECIAL )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


/**********************************************************************************/

static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+8, RGN_FRAC(1,2), 8, 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 16, 0, 24, 8 },
	{ 64*8+0, 64*8+1, 64*8+2, 64*8+3, 64*8+4, 64*8+5, 64*8+6, 64*8+7,
		0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	128*8
};

static struct GfxLayout tilelayout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+8, RGN_FRAC(1,2), 8, 0 },
	{ 32*8+0, 32*8+1, 32*8+2, 32*8+3, 32*8+4, 32*8+5, 32*8+6, 32*8+7,
		0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,      0, 32 },
	{ REGION_GFX1, 0, &tilelayout,      0, 32 },
	{ REGION_GFX2, 0, &spritelayout,  512, 32 },
	{ -1 }	/* end of array */
};


/**********************************************************************************/

static void sound_irq_gen(int state)
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "sound irq\n");
}

static INTERRUPT_GEN( deco32_vbl_interrupt )
{
	cpu_set_irq_line(0, ARM_IRQ_LINE, HOLD_LINE);
}

static struct OKIM6295interface m6295_intf =
{
	2,
	{ 28000000/28/132, 28000000/14/132 },		/* Appears to be spot on for both interfaces. */
	{ REGION_SOUND1, REGION_SOUND2 },
	{ 100, 35 }
};

static MACHINE_DRIVER_START( hvysmsh )

	/* basic machine hardware */
	MDRV_CPU_ADD(ARM,28000000 / 4)
	MDRV_CPU_MEMORY(hvysmsh_readmem, hvysmsh_writemem)
	MDRV_CPU_VBLANK_INT(deco32_vbl_interrupt, 1)

	MDRV_FRAMES_PER_SECOND(58)
	MDRV_VBLANK_DURATION(TIME_IN_USEC(529))

	MDRV_NVRAM_HANDLER(93C46) /* Actually 93c45 */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_RGB_DIRECT | VIDEO_BUFFERS_SPRITERAM | VIDEO_NEEDS_6BITS_PER_GUN)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(wcvol95)
	MDRV_VIDEO_UPDATE(wcvol95)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(OKIM6295, m6295_intf)
MACHINE_DRIVER_END


static struct YMZ280Binterface ymz280b_intf =
{
	1,
	{ 28000000 / 2 }, /* Confirmed on real board */
	{ REGION_SOUND1 },
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) },
	{ sound_irq_gen }
};

static MACHINE_DRIVER_START( wcvol95 )

	/* basic machine hardware */
	MDRV_CPU_ADD(ARM,28000000 / 3)
	MDRV_CPU_MEMORY(wcvol95_readmem, wcvol95_writemem)
	MDRV_CPU_VBLANK_INT(deco32_vbl_interrupt, 1)

	MDRV_FRAMES_PER_SECOND(58)
	MDRV_VBLANK_DURATION(TIME_IN_USEC(529))

	MDRV_NVRAM_HANDLER(93C46) /* Actually 93c45 */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_RGB_DIRECT | VIDEO_BUFFERS_SPRITERAM | VIDEO_NEEDS_6BITS_PER_GUN)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(wcvol95)
	MDRV_VIDEO_UPDATE(wcvol95)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YMZ280B, ymz280b_intf)
MACHINE_DRIVER_END


/**********************************************************************************/

ROM_START( hvysmsh ) /* Europe -2  1993/06/30 */
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* DE156 code (encrypted) */
	ROM_LOAD32_WORD( "lt00-2.2j", 0x000002, 0x080000, CRC(f6e10fc0) SHA1(76189260ca0a79500d62c4aa8e3aed6cfca3e102) )
	ROM_LOAD32_WORD( "lt01-2.3j", 0x000000, 0x080000, CRC(ce2a75e2) SHA1(4119a3175d7c394041197f01523a6eaa3d9ba398) )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )
	ROM_LOAD( "mbg-00.9a",  0x000000, 0x080000, CRC(7d94eb16) SHA1(31cf5302eba37e935865822aebd76c700bc51eaf) )
	ROM_CONTINUE( 0x100000, 0x080000)
	ROM_CONTINUE( 0x080000, 0x080000)
	ROM_CONTINUE( 0x180000, 0x080000)

	ROM_REGION( 0x800000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "mbg-01.10a", 0x000000, 0x200000, CRC(bcd7fb29) SHA1(a54a813b5adcb4df0bfdd58285b1f8e17fbbb7a2) )
	ROM_LOAD16_BYTE( "mbg-02.11a", 0x000001, 0x200000, CRC(0cc16440) SHA1(1cbf620a9d875ec87dd28a97a256584b6ef277cd) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "mbg-03.10k", 0x00000, 0x80000,  CRC(4b809420) SHA1(ad0278745002320804a31af0b772f9ab5f075027) )

	ROM_REGION( 0x200000, REGION_SOUND2, 0 ) /* Oki samples */
	ROM_LOAD( "mbg-04.11k", 0x00000, 0x200000, CRC(2281c758) SHA1(934691b4002ecd6bc9a09b8970ff18a09451d492) )
ROM_END

ROM_START( hvysmshj ) /* Japan -2  1993/06/30 */
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* DE156 code (encrypted) */
	ROM_LOAD32_WORD( "lp00-2.2j", 0x000002, 0x080000, CRC(3f8fd724) SHA1(8efb27b96dbdc58715eb44c7846f30d485e1ded4) )
	ROM_LOAD32_WORD( "lp01-2.3j", 0x000000, 0x080000, CRC(a6fe282a) SHA1(10295b740ced35b3bb1f48ca3af2e985912405ec) )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )
	ROM_LOAD( "mbg-00.9a",  0x000000, 0x080000, CRC(7d94eb16) SHA1(31cf5302eba37e935865822aebd76c700bc51eaf) )
	ROM_CONTINUE( 0x100000, 0x080000)
	ROM_CONTINUE( 0x080000, 0x080000)
	ROM_CONTINUE( 0x180000, 0x080000)

	ROM_REGION( 0x800000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "mbg-01.10a", 0x000000, 0x200000, CRC(bcd7fb29) SHA1(a54a813b5adcb4df0bfdd58285b1f8e17fbbb7a2) )
	ROM_LOAD16_BYTE( "mbg-02.11a", 0x000001, 0x200000, CRC(0cc16440) SHA1(1cbf620a9d875ec87dd28a97a256584b6ef277cd) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "mbg-03.10k", 0x00000, 0x80000,  CRC(4b809420) SHA1(ad0278745002320804a31af0b772f9ab5f075027) )

	ROM_REGION( 0x200000, REGION_SOUND2, 0 ) /* Oki samples */
	ROM_LOAD( "mbg-04.11k", 0x00000, 0x200000, CRC(2281c758) SHA1(934691b4002ecd6bc9a09b8970ff18a09451d492) )
ROM_END

ROM_START( hvysmsha ) /* Asia -4  1993/09/06 */
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* DE156 code (encrypted) */
	ROM_LOAD32_WORD( "xx00-4.2j", 0x000002, 0x080000, CRC(333a92c1) SHA1(b7e174ea081febb765298aa1c6533b2f9f162bce) ) /* "xx" is NOT the correct region code, this needs */
	ROM_LOAD32_WORD( "xx01-4.3j", 0x000000, 0x080000, CRC(8c24c5ed) SHA1(ab9689530f4f4a6015ce0a6f8e0d796b0618cd79) ) /* to be verified and corrected at some point */

	ROM_REGION( 0x200000, REGION_GFX1, 0 )
	ROM_LOAD( "mbg-00.9a",  0x000000, 0x080000, CRC(7d94eb16) SHA1(31cf5302eba37e935865822aebd76c700bc51eaf) )
	ROM_CONTINUE( 0x100000, 0x080000)
	ROM_CONTINUE( 0x080000, 0x080000)
	ROM_CONTINUE( 0x180000, 0x080000)

	ROM_REGION( 0x800000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "mbg-01.10a", 0x000000, 0x200000, CRC(bcd7fb29) SHA1(a54a813b5adcb4df0bfdd58285b1f8e17fbbb7a2) )
	ROM_LOAD16_BYTE( "mbg-02.11a", 0x000001, 0x200000, CRC(0cc16440) SHA1(1cbf620a9d875ec87dd28a97a256584b6ef277cd) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "mbg-03.10k", 0x00000, 0x80000,  CRC(4b809420) SHA1(ad0278745002320804a31af0b772f9ab5f075027) )

	ROM_REGION( 0x200000, REGION_SOUND2, 0 ) /* Oki samples */
	ROM_LOAD( "mbg-04.11k", 0x00000, 0x200000, CRC(2281c758) SHA1(934691b4002ecd6bc9a09b8970ff18a09451d492) )
ROM_END

ROM_START( wcvol95 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* DE156 code (encrypted) */
	ROM_LOAD32_WORD( "pn00-0.2f",    0x000002, 0x080000, CRC(c9ed2006) SHA1(cee93eafc42c4de7a1453c85e7d6bca8d62cdc7b) )
	ROM_LOAD32_WORD( "pn01-0.4f",    0x000000, 0x080000, CRC(1c3641c3) SHA1(60dddc3585e4dedb485f7505fee03495f615c0c0) )

	ROM_REGION( 0x080000, REGION_GFX1, 0 )
	ROM_LOAD( "mbx-00.9a",    0x000000, 0x080000, CRC(a0b24204) SHA1(cec8089c6c635f23b3a4aeeef2c43f519568ad70) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "mbx-01.12a",    0x000000, 0x100000, CRC(73deb3f1) SHA1(c0cabecfd88695afe0f27c5bb115b4973907207d) )
	ROM_LOAD16_BYTE( "mbx-02.13a",    0x000001, 0x100000, CRC(3204d324) SHA1(44102f71bae44bf3a9bd2de7e5791d959a2c9bdd) )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 ) /* YMZ280B-F samples */
	ROM_LOAD( "mbx-03.13j",    0x00000, 0x200000,  CRC(061632bc) SHA1(7900ac56e59f4a4e5768ce72f4a4b7c5875f5ae8) )
ROM_END

ROM_START( wcvol95x )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* DE156 code (encrypted) */
	/* no label markings were present */
	ROM_LOAD32_WORD( "2f.bin",    0x000002, 0x080000, CRC(ac06633d) SHA1(5d37ca3050f35d5fc06f70e91b1522e325471585) )
	ROM_LOAD32_WORD( "4f.bin",    0x000000, 0x080000, CRC(e211f67a) SHA1(d008c2b809482f17ada608134357fa1205d767d4) )

	ROM_REGION( 0x080000, REGION_GFX1, 0 )
	ROM_LOAD( "mbx-00.9a",    0x000000, 0x080000, CRC(a0b24204) SHA1(cec8089c6c635f23b3a4aeeef2c43f519568ad70) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "mbx-01.12a",    0x000000, 0x100000, CRC(73deb3f1) SHA1(c0cabecfd88695afe0f27c5bb115b4973907207d) )
	ROM_LOAD16_BYTE( "mbx-02.13a",    0x000001, 0x100000, CRC(3204d324) SHA1(44102f71bae44bf3a9bd2de7e5791d959a2c9bdd) )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 ) /* YMZ280B-F samples */
	ROM_LOAD( "mbx-03.13j",    0x00000, 0x200000,  CRC(061632bc) SHA1(7900ac56e59f4a4e5768ce72f4a4b7c5875f5ae8) )
ROM_END


/**********************************************************************************/

static void descramble_sound(UINT8 *rom, int length)
{
	UINT8 *buf1 = (UINT8*)malloc(length);
	UINT32 x;

	for (x=0;x<length;x++)
	{
		UINT32 addr;

		addr = BITSWAP24 (x,23,22,21,0, 20,
		                    19,18,17,16,
		                    15,14,13,12,
		                    11,10,9, 8,
		                    7, 6, 5, 4,
		                    3, 2, 1 );

		buf1[addr] = rom[x];
	}

	memcpy(rom,buf1,length);

	free (buf1);
}

static DRIVER_INIT( hvysmsh )
{
	deco56_decrypt(REGION_GFX1); /* 141 */
	deco156_decrypt();
	descramble_sound(memory_region(REGION_SOUND2), memory_region_length(REGION_SOUND2));
}

static DRIVER_INIT( wcvol95 )
{
	deco56_decrypt(REGION_GFX1); /* 141 */
	deco156_decrypt();
	descramble_sound(memory_region(REGION_SOUND1), memory_region_length(REGION_SOUND1));
}


/**********************************************************************************/
GAME( 1993, hvysmsh,  0,       hvysmsh, hvysmsh, hvysmsh,  ROT0, "Data East Corporation", "Heavy Smash (Europe version -2)" )
GAME( 1993, hvysmsha, hvysmsh, hvysmsh, hvysmsh, hvysmsh,  ROT0, "Data East Corporation", "Heavy Smash (Asia version -4)" )
GAME( 1993, hvysmshj, hvysmsh, hvysmsh, hvysmsh, hvysmsh,  ROT0, "Data East Corporation", "Heavy Smash (Japan version -2)" )
GAME( 1995, wcvol95,  0,       wcvol95, wcvol95, wcvol95,  ROT0, "Data East Corporation", "World Cup Volley 95 (Japan v1.0)" )
GAME( 1995, wcvol95x, wcvol95, wcvol95, wcvol95, wcvol95,  ROT0, "Data East Corporation", "World Cup Volley 95 Extra Version (Asia v2.0B)" )
