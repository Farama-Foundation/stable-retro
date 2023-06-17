/****************************************************************************

Tug Boat
6502 hooked up + preliminary video by MooglyGuy

TODO:
- verify connections of the two PIAs. I only hooked up a couple of ports but
  there are more.
- check how the score is displayed. I'm quite sure that tugboat_score_w is
  supposed to access videoram scanning it by columns (like btime_mirrorvideoram_w),
  but the current implementation is a big kludge, and it still looks wrong.
- colors might not be entirely accurate

the problem which caused the controls not to work
---
There's counter at $000b, counting up from $ff to 0 or from $fe to 0 (initial value depends
on game level). It's increased in main loop, and used for game flow control (scrolling speed , controls  etc).
Every interrupt, when (counter&3)!=0 , there's a check for left/right inputs .
But when init val was $ff (2nd level),  the condition 'counter&3!=0' was
always false - counter was reloaded and incremented before interrupt occurs

****************************************************************************/

#include "driver.h"
#include "machine/6821pia.h"


data8_t *tugboat_ram,*tugboat_score;


static UINT8 hd46505_0_reg[18],hd46505_1_reg[18];


/*  there isn't the usual resistor array anywhere near the color prom,
    just four 1k resistors. */
PALETTE_INIT( tugboat )
{
	int i;


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int r,g,b,brt;


		brt = ((color_prom[i] >> 3) & 0x01) ? 0xff : 0x80;

		r = brt * ((color_prom[i] >> 0) & 0x01);
		g = brt * ((color_prom[i] >> 1) & 0x01);
		b = brt * ((color_prom[i] >> 2) & 0x01);

		palette_set_color(i,r,g,b);
	}
}



/* see crtc6845.c. That file is only a placeholder, I process the writes here
   because I need the start_addr register to handle scrolling */
static WRITE_HANDLER( tugboat_hd46505_0_w )
{
	static int reg;
	if (offset == 0) reg = data & 0x0f;
	else if (reg < 18) hd46505_0_reg[reg] = data;
}
static WRITE_HANDLER( tugboat_hd46505_1_w )
{
	static int reg;
	if (offset == 0) reg = data & 0x0f;
	else if (reg < 18) hd46505_1_reg[reg] = data;
}



static WRITE_HANDLER( tugboat_score_w )
{
	tugboat_ram[0x291d + 32*offset] = data ^ 0x0f;	/* ???? */
}

static void draw_tilemap(struct mame_bitmap *bitmap,const struct rectangle *cliprect,
		int addr,int gfx0,int gfx1,int transparency)
{
	int x,y;

	for (y = 0;y < 32;y++)
	{
		for (x = 0;x < 32;x++)
		{
			int code = (tugboat_ram[addr + 0x400] << 8) | tugboat_ram[addr];
			int color = (code & 0x3c00) >> 10;
			int rgn;

			code &=0x3ff;
			rgn = gfx0;

			if (code > 0x1ff)
			{
				code &= 0x1ff;
				rgn = gfx1;
			}

			drawgfx(bitmap,Machine->gfx[rgn],
					code,
					color,
					0,0,
					8*x,8*y,
					cliprect,transparency,7);

			addr = (addr & 0xfc00) | ((addr + 1) & 0x03ff);
		}
	}
}

VIDEO_UPDATE( tugboat )
{
	int startaddr0 = hd46505_0_reg[0x0c]*256 + hd46505_0_reg[0x0d];
	int startaddr1 = hd46505_1_reg[0x0c]*256 + hd46505_1_reg[0x0d];


	draw_tilemap(bitmap,cliprect,startaddr0,0,1,TRANSPARENCY_NONE);
	draw_tilemap(bitmap,cliprect,startaddr1,2,3,TRANSPARENCY_PEN);
}





static int ctrl;

static READ_HANDLER( tugboat_input_r )
{
	if (~ctrl & 0x80)
		return readinputport(0);
	else if (~ctrl & 0x40)
		return readinputport(1);
	else if (~ctrl & 0x20)
		return readinputport(2);
	else if (~ctrl & 0x10)
		return readinputport(3);
	else
		return readinputport(4);
}

static READ_HANDLER( tugboat_ctrl_r )
{
	return ctrl;
}

static WRITE_HANDLER( tugboat_ctrl_w )
{
	ctrl = data;
}

static struct pia6821_interface pia0_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ tugboat_input_r, 0, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, 0, 0, 0,
	/*irqs   : A/B             */ 0, 0,
};

static struct pia6821_interface pia1_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ input_port_5_r, tugboat_ctrl_r, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0,              tugboat_ctrl_w, 0, 0,
	/*irqs   : A/B             */ 0, 0
};

static void interrupt_gen(int scanline)
{
	cpu_set_irq_line(0, 0, HOLD_LINE);
	timer_set(cpu_getscanlinetime(1), 0, interrupt_gen);
}

MACHINE_INIT( tugboat )
{
	pia_unconfig();
	pia_config(0, PIA_STANDARD_ORDERING, &pia0_intf);
	pia_config(1, PIA_STANDARD_ORDERING, &pia1_intf);
	pia_reset();
	timer_set(cpu_getscanlinetime(1), 0, interrupt_gen);
}


static MEMORY_READ_START( tugboat_readmem )
	{ 0x0000, 0x01ff, MRA_RAM },
	{ 0x11e4, 0x11e7, pia_0_r },
	{ 0x11e8, 0x11eb, pia_1_r },
	{ 0x2000, 0x2fff, MRA_RAM },
	{ 0x5000, 0x7fff, MRA_ROM },
	{ 0xfff0, 0xffff, MRA_ROM },	/* vectors */
MEMORY_END

static MEMORY_WRITE_START( tugboat_writemem )
	{ 0x0000, 0x01ff, MWA_RAM, &tugboat_ram },
	{ 0x1060, 0x1060, AY8910_control_port_0_w },
	{ 0x1061, 0x1061, AY8910_write_port_0_w },
	{ 0x10a0, 0x10a1, tugboat_hd46505_0_w },	/* scrolling is performed changing the start_addr register (0C/0D)*/
	{ 0x10c0, 0x10c1, tugboat_hd46505_1_w },
	{ 0x11e4, 0x11e7, pia_0_w },
	{ 0x11e8, 0x11eb, pia_1_w },
	{ 0x18e0, 0x18ef, tugboat_score_w },
	{ 0x2000, 0x2fff, MWA_RAM },	/* tilemap RAM */
    { 0x5000, 0x7fff, MWA_ROM },
MEMORY_END



INPUT_PORTS_START( tugboat )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT_IMPULSE( 0x01, IP_ACTIVE_LOW, IPT_COIN1, 1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x30, 0x10, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x40, "5" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(2,3), RGN_FRAC(1,3), RGN_FRAC(0,3) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0x80, 16 },
	{ REGION_GFX2, 0, &charlayout, 0x80, 16 },
	{ REGION_GFX3, 0, &charlayout, 0x00, 16 },
	{ REGION_GFX4, 0, &charlayout, 0x00, 16 },
	{ -1 }
};



static struct AY8910interface ay8910_interface =
{
	1,			/* 1 chip */
	2000000,	/* 2 MHz???? */
	{ 35 },		/* volume */
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};



static MACHINE_DRIVER_START( tugboat )
	MDRV_CPU_ADD_TAG("main", M6502, 2000000)	/* 2 MHz ???? */
	MDRV_CPU_MEMORY(tugboat_readmem,tugboat_writemem)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(tugboat)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8,64*8)
	MDRV_VISIBLE_AREA(1*8,31*8-1,2*8,30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)

	MDRV_PALETTE_INIT(tugboat)
	MDRV_VIDEO_UPDATE(tugboat)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END



ROM_START( tugboat )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "u7.bin", 0x5000, 0x1000, CRC(e81d7581) SHA1(c76327e3b027a5a2af69f8cfafa1f828ad0ebdb1) )
	ROM_LOAD( "u8.bin", 0x6000, 0x1000, CRC(7525de06) SHA1(0722c7a0b89c55162227173679ffbe398ca350a2) )
	ROM_LOAD( "u9.bin", 0x7000, 0x1000, CRC(aa4ae687) SHA1(a212eed5d04d6197aa3484ff36059fd7998604a6) )
	ROM_RELOAD(         0xf000, 0x1000 )	/* for the vectors */

	ROM_REGION( 0x1800, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT  )
	ROM_LOAD( "u67.bin",  0x0000, 0x0800, CRC(601c425b) SHA1(13ed54ba1307ba3f779293d88c19d0c0f2d91a96) )
	ROM_FILL(             0x0800, 0x0800, 0xff )
	ROM_FILL(             0x1000, 0x0800, 0xff )

	ROM_REGION( 0x3000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_INVERT  )
	ROM_LOAD( "u68.bin", 0x0000, 0x1000, CRC(d5835182) SHA1(f67c8f93e0d7dd1bf8e3a98756719d386c133d1c) )
	ROM_LOAD( "u69.bin", 0x1000, 0x1000, CRC(e6d25878) SHA1(de9096ef3108d031049be1e7f2c5e346d0bc0df1) )
	ROM_LOAD( "u70.bin", 0x2000, 0x1000, CRC(34ce2850) SHA1(8883126627ed8a1d2c3bed2a3d169ce35eafc8a3) )

	ROM_REGION( 0x1800, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "u168.bin", 0x0000, 0x0800, CRC(279042fd) SHA1(1361fff1bc532251bbd36b7b60776c2cc137cfba) )	/* labeled u-167 */
	ROM_FILL(             0x0800, 0x0800, 0x00 )
	ROM_FILL(             0x1000, 0x0800, 0x00 )

	ROM_REGION( 0x1800, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "u170.bin", 0x0000, 0x0800, CRC(64d9f4d7) SHA1(3ff7fc099023512c33ec4583e91e6cbab903e7a8) )	/* labeled u-168 */
	ROM_LOAD( "u169.bin", 0x0800, 0x0800, CRC(1a636296) SHA1(bcb18d714328ba3db2d16d74c47a985c16a0bbe2) )	/* labeled u-169 */
	ROM_LOAD( "u167.bin", 0x1000, 0x0800, CRC(b9c9b4f7) SHA1(6685d580ae150d7c67bac2786ee4b7a2c28eddc3) )	/* labeled u-170 */

	ROM_REGION( 0x0100, REGION_PROMS, ROMREGION_DISPOSE )
	ROM_LOAD( "nt2_u128.clr", 0x0000, 0x0100, CRC(236672bf) SHA1(57482d0a23223ef7b211045ad28d3e41e90f961e) )
ROM_END



GAMEX( 1982, tugboat, 0, tugboat, tugboat, 0, ROT90, "ETM", "Tugboat", GAME_IMPERFECT_GRAPHICS )
