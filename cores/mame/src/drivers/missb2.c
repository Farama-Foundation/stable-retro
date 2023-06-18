/* Miss Bubble 2

A rather odd bootleg of Bubble Bobble with level select, redesigned levels,
redesigned (8bpp!) graphics and different sound hardware... Crazy

-- where is oki?

*/

#include "driver.h"
#include "vidhrdw/generic.h"

static data8_t *bg_paletteram,*bg_vram;

/* vidhrdw/bublbobl.c */
extern unsigned char *bublbobl_objectram;
extern size_t bublbobl_objectram_size;
VIDEO_UPDATE( bublbobl );

/* machine/bublbobl.c */
extern unsigned char *bublbobl_sharedram1,*bublbobl_sharedram2;
READ_HANDLER( bublbobl_sharedram1_r );
READ_HANDLER( bublbobl_sharedram2_r );
WRITE_HANDLER( bublbobl_sharedram1_w );
WRITE_HANDLER( bublbobl_sharedram2_w );
INTERRUPT_GEN( bublbobl_m68705_interrupt );
READ_HANDLER( bublbobl_68705_portA_r );
WRITE_HANDLER( bublbobl_68705_portA_w );
WRITE_HANDLER( bublbobl_68705_ddrA_w );
READ_HANDLER( bublbobl_68705_portB_r );
WRITE_HANDLER( bublbobl_68705_portB_w );
WRITE_HANDLER( bublbobl_68705_ddrB_w );
WRITE_HANDLER( bublbobl_bankswitch_w );
WRITE_HANDLER( tokio_bankswitch_w );
WRITE_HANDLER( tokio_videoctrl_w );
WRITE_HANDLER( bublbobl_nmitrigger_w );
READ_HANDLER( tokio_fake_r );
WRITE_HANDLER( bublbobl_sound_command_w );
WRITE_HANDLER( bublbobl_sh_nmi_disable_w );
WRITE_HANDLER( bublbobl_sh_nmi_enable_w );
extern int bublbobl_video_enable;


VIDEO_UPDATE( missb2 )
{
	int offs;
	int sx,sy,xc,yc;
	int gfx_num,gfx_attr,gfx_offs;
	const UINT8 *prom_line;
	UINT16 bg_offs;

	/* Bubble Bobble doesn't have a real video RAM. All graphics (characters */
	/* and sprites) are stored in the same memory region, and information on */
	/* the background character columns is stored in the area dd00-dd3f */

	/* This clears & redraws the entire screen each pass */
	fillbitmap(bitmap,Machine->pens[255],&Machine->visible_area);

	if (!bublbobl_video_enable) return;

	/* background map register */
	/*usrintf_showmessage("%02x",(*bg_vram) & 0x1f);*/
	for(bg_offs = ((*bg_vram) << 4);bg_offs<(((*bg_vram)<< 4)|0xf);bg_offs++)
	{
		drawgfx(bitmap,Machine->gfx[1],
				bg_offs,
				1,
				0,0,
				0,(bg_offs & 0xf) * 0x10,
				&Machine->visible_area,TRANSPARENCY_NONE,0xff);
	}


	sx = 0;

	for (offs = 0;offs < bublbobl_objectram_size;offs += 4)
    {
		/* skip empty sprites */
		/* this is dword aligned so the UINT32 * cast shouldn't give problems */
		/* on any architecture */
		if (*(UINT32 *)(&bublbobl_objectram[offs]) == 0)
			continue;

		gfx_num = bublbobl_objectram[offs + 1];
		gfx_attr = bublbobl_objectram[offs + 3];
		prom_line = memory_region(REGION_PROMS) + 0x80 + ((gfx_num & 0xe0) >> 1);

		gfx_offs = ((gfx_num & 0x1f) * 0x80);
		if ((gfx_num & 0xa0) == 0xa0)
			gfx_offs |= 0x1000;

		sy = -bublbobl_objectram[offs + 0];

		for (yc = 0;yc < 32;yc++)
		{
			if (prom_line[yc/2] & 0x08)	continue;	/* NEXT */

			if (!(prom_line[yc/2] & 0x04))	/* next column */
			{
				sx = bublbobl_objectram[offs + 2];
				if (gfx_attr & 0x40) sx -= 256;
			}

			for (xc = 0;xc < 2;xc++)
			{
				int goffs,code,color,flipx,flipy,x,y;

				goffs = gfx_offs + xc * 0x40 + (yc & 7) * 0x02 +
						(prom_line[yc/2] & 0x03) * 0x10;
				code = videoram[goffs] + 256 * (videoram[goffs + 1] & 0x03) + 1024 * (gfx_attr & 0x0f);
				color = (videoram[goffs + 1] & 0x3c) >> 2;
				flipx = videoram[goffs + 1] & 0x40;
				flipy = videoram[goffs + 1] & 0x80;
				x = sx + xc * 8;
				y = (sy + yc * 8) & 0xff;

				if (flip_screen)
				{
					x = 248 - x;
					y = 248 - y;
					flipx = !flipx;
					flipy = !flipy;
				}

				drawgfx(bitmap,Machine->gfx[0],
						code,
						0,
						flipx,flipy,
						x,y,
						&Machine->visible_area,TRANSPARENCY_PEN,0xff);
			}
		}

		sx += 16;
	}
}

static INLINE void bg_changecolor_RRRRGGGGBBBBxxxx(pen_t color,int data)
{
	int r,g,b;


	r = (data >> 12) & 0x0f;
	g = (data >>  8) & 0x0f;
	b = (data >>  4) & 0x0f;

	r = (r << 4) | r;
	g = (g << 4) | g;
	b = (b << 4) | b;

	palette_set_color(color+256,r,g,b);
}

WRITE_HANDLER( bg_paletteram_RRRRGGGGBBBBxxxx_swap_w )
{
	bg_paletteram[offset] = data;
	bg_changecolor_RRRRGGGGBBBBxxxx(offset / 2,bg_paletteram[offset | 1] | (bg_paletteram[offset & ~1] << 8));
}

WRITE_HANDLER( bg_bank_w )
{
	int bankaddress;
	unsigned char *RAM = memory_region(REGION_CPU2);

	/*I don't know how this is really connected,bit 1 is always high afaik...*/
	bankaddress = ((data & 2) ? 0x1000 : 0x0000) | ((data & 1) ? 0x4000 : 0x0000) | (0x8000);
	cpu_setbank(2,&RAM[bankaddress]);
}



static MEMORY_READ_START( missb2_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xf7ff, bublbobl_sharedram1_r },
	{ 0xf800, 0xf9ff, MRA_RAM },
	{ 0xfc00, 0xfcff, bublbobl_sharedram2_r },

	{ 0xfe00, 0xfe03, MRA_RAM }, /* ?*/
	{ 0xfe80, 0xfe83, MRA_RAM }, /* ?*/

	{ 0xff00, 0xff00, input_port_0_r },
	{ 0xff01, 0xff01, input_port_1_r },
	{ 0xff02, 0xff02, input_port_2_r },
	{ 0xff03, 0xff03, input_port_3_r },
	{ 0xfd00, 0xfdff, MRA_RAM }, /* ? */
MEMORY_END

static MEMORY_WRITE_START( missb2_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xdcff, MWA_RAM, &videoram, &videoram_size },
	{ 0xdd00, 0xdfff, MWA_RAM, &bublbobl_objectram, &bublbobl_objectram_size },
	{ 0xe000, 0xf7ff, bublbobl_sharedram1_w, &bublbobl_sharedram1 },
	{ 0xf800, 0xf9ff, paletteram_RRRRGGGGBBBBxxxx_swap_w, &paletteram  },
	{ 0xfa00, 0xfa00, bublbobl_sound_command_w },
	{ 0xfa80, 0xfa80, MWA_NOP },
	{ 0xfb00, 0xfb00, bublbobl_nmitrigger_w },	/* not used by Bubble Bobble, only by Tokio */
	{ 0xfb40, 0xfb40, bublbobl_bankswitch_w },
	{ 0xfc00, 0xfcff, bublbobl_sharedram2_w, &bublbobl_sharedram2 },
	{ 0xfd00, 0xfdff, MWA_RAM }, /* ? */

	{ 0xfe00, 0xfe03, MWA_RAM }, /* ?*/
	{ 0xfe80, 0xfe83, MWA_RAM }, /* ?*/

	{ 0xff94, 0xff94, MWA_NOP }, /* ?*/
MEMORY_END

/*READ_HANDLER ( missb_random )*/
/*{*/
/*	return rand();*/
/*}*/

static MEMORY_READ_START( missb2_readmem2 )
	{ 0x0000, 0x7fff, MRA_ROM },

	{ 0x9000, 0xafff, MRA_BANK2 }, /* ROM data for the background palette ram*/
	{ 0xb000, 0xb1ff, MRA_ROM }, /* ? banked ?*/

	{ 0xc800, 0xcfff, MRA_RAM }, /* main? */
	{ 0xe000, 0xf7ff, bublbobl_sharedram1_r },
MEMORY_END

static MEMORY_WRITE_START( missb2_writemem2 )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xc1ff, bg_paletteram_RRRRGGGGBBBBxxxx_swap_w,&bg_paletteram  },
	{ 0xc800, 0xcfff, MWA_RAM }, /* main? */

	{ 0xd000, 0xd000, bg_bank_w },
	{ 0xd002, 0xd002, MWA_NOP },
	{ 0xd003, 0xd003, MWA_RAM,&bg_vram },
	{ 0xe000, 0xf7ff, bublbobl_sharedram1_w },
MEMORY_END

/* does it actually use the original sound hw or is it just leftover code .. */

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x8fff, MRA_RAM },
	{ 0x9000, 0x9000, YM2203_status_port_0_r },
	{ 0x9001, 0x9001, YM2203_read_port_0_r },
	{ 0xa000, 0xa000, YM3526_status_port_0_r },
	{ 0xb000, 0xb000, soundlatch_r },
	{ 0xb001, 0xb001, MRA_NOP },	/* bit 0: message pending for main cpu */
									/* bit 1: message pending for sound cpu */
	{ 0xe000, 0xefff, MRA_ROM },	/* space for diagnostic ROM? */
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x8fff, MWA_RAM },
	{ 0x9000, 0x9000, YM2203_control_port_0_w },
	{ 0x9001, 0x9001, YM2203_write_port_0_w },
	{ 0xa000, 0xa000, YM3526_control_port_0_w },
	{ 0xa001, 0xa001, YM3526_write_port_0_w },
	{ 0xb000, 0xb000, MWA_NOP },	/* message for main cpu */
	{ 0xb001, 0xb001, bublbobl_sh_nmi_enable_w },
	{ 0xb002, 0xb002, bublbobl_sh_nmi_disable_w },
	{ 0xe000, 0xefff, MWA_ROM },	/* space for diagnostic ROM? */
MEMORY_END



INPUT_PORTS_START( missb2 )
	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, "Language" )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPSETTING(    0x01, "Japanese" )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Medium" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x08, "20000 80000" )
	PORT_DIPSETTING(    0x0c, "30000 100000" )
	PORT_DIPSETTING(    0x04, "40000 200000" )
	PORT_DIPSETTING(    0x00, "50000 250000" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x10, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_DIPNAME( 0xc0, 0x00, "Monster Speed" )
	PORT_DIPSETTING(    0x00, "Normal" )
	PORT_DIPSETTING(    0x40, "Medium" )
	PORT_DIPSETTING(    0x80, "High" )
	PORT_DIPSETTING(    0xc0, "Very High" )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_TILT ) /* ?????*/
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,4),
	8,
	{ RGN_FRAC(0,4)+0, RGN_FRAC(0,4)+4, RGN_FRAC(1,4)+0, RGN_FRAC(1,4)+4, RGN_FRAC(2,4)+0, RGN_FRAC(2,4)+4, RGN_FRAC(3,4)+0, RGN_FRAC(3,4)+4 },
	{ 3, 2, 1, 0, 8+3, 8+2, 8+1, 8+0 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8
};





static struct GfxLayout bglayout =
{
	256,16,
	RGN_FRAC(1,1),
	8,
	{ 0,1,2,3,4,5,6,7 },
	{
		0*8,      1*8, 2048*8, 2049*8,    8*8,    9*8, 2056*8, 2057*8,
		4*8,      5*8, 2052*8, 2053*8,   12*8,   13*8, 2060*8, 2061*8,
		256*8 , 257*8, 2304*8, 2305*8,  264*8,  265*8, 2312*8, 2313*8,
		260*8 , 261*8, 2308*8, 2309*8,  268*8,  269*8, 2316*8, 2317*8,
       1024*8, 1025*8, 3072*8, 3073*8, 1032*8, 1033*8, 3080*8, 3081*8,
	   1028*8, 1029*8, 3076*8, 3077*8, 1036*8, 1037*8, 3084*8, 3085*8,
	   1280*8, 1281*8, 3328*8, 3329*8, 1288*8, 1289*8, 3336*8, 3337*8,
       1284*8, 1285*8, 3332*8, 3333*8, 1292*8, 1293*8, 3340*8, 3341*8,
		512*8,  513*8, 2560*8, 2561*8,  520*8,  521*8, 2568*8, 2569*8,
		516*8,  517*8, 2564*8, 2565*8,	524*8,  525*8, 2572*8, 2573*8,
        768*8,  769*8, 2816*8, 2817*8,  776*8,  777*8, 2824*8, 2825*8,
        772*8,  773*8, 2820*8, 2821*8,  780*8,  781*8, 2828*8, 2829*8,
	   1536*8, 1537*8, 3584*8, 3585*8, 1544*8, 1545*8, 3592*8, 3593*8,
       1540*8, 1541*8, 3588*8, 3589*8, 1548*8, 1549*8, 3596*8, 3597*8,
	   1792*8, 1793*8, 3840*8, 3841*8, 1800*8, 1801*8, 3848*8, 3849*8,
	   1796*8, 1797*8, 3844*8, 3845*8, 1804*8, 1805*8, 3852*8, 3853*8,
          2*8,    3*8, 2050*8, 2051*8,   10*8,   11*8, 2058*8, 2059*8,
		  6*8,    7*8, 2054*8, 2055*8,	 14*8,   15*8, 2062*8, 2063*8,
	    258*8,  259*8, 2306*8, 2307*8,  266*8,  267*8, 2314*8, 2315*8,
		262*8,  263*8, 2310*8, 2311*8,	270*8,  271*8, 2318*8, 2319*8,
       1026*8, 1027*8, 3074*8, 3075*8, 1034*8, 1035*8, 3082*8, 3083*8,
       1030*8, 1031*8, 3078*8, 3079*8, 1038*8, 1039*8, 3086*8, 3087*8,
       1282*8, 1283*8, 3330*8, 3331*8, 1290*8, 1291*8, 3338*8, 3339*8,
       1286*8, 1287*8, 3334*8, 3335*8, 1294*8, 1295*8, 3342*8, 3343*8,
        514*8,  515*8, 2562*8, 2563*8, 	522*8,  523*8, 2570*8, 2571*8,
		518*8,  519*8, 2566*8, 2567*8,  526*8,  527*8, 2574*8, 2575*8,
		770*8,  771*8, 2818*8, 2819*8,  778*8,  779*8, 2826*8, 2827*8,
        774*8,  775*8, 2822*8, 2823*8,  782*8,  783*8, 2830*8, 2831*8,
	   1538*8, 1539*8, 3586*8, 3587*8, 1546*8, 1547*8, 3594*8, 3595*8,
	   1542*8, 1543*8, 3590*8, 3591*8, 1550*8, 1551*8, 3598*8, 3599*8,
	   1794*8, 1795*8, 3842*8, 3843*8, 1802*8, 1803*8, 3850*8, 3851*8,
	   1798*8, 1799*8, 3846*8, 3847*8, 1806*8, 1807*8, 3854*8, 3855*8
	  },
	{ 0*128, 1*128, 2*128, 3*128, 4*128, 5*128, 6*128, 7*128, 8*128, 9*128, 10*128, 11*128, 12*128, 13*128, 14*128, 15*128 },
	256*128
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x00000, &charlayout, 0, 1 },
	{ REGION_GFX2, 0x00000, &bglayout, 0, 2 },
	{ -1 }	/* end of array */
};

/* ?? not sure for this*/
#define MAIN_XTAL 24000000

/* handler called by the 2203 emulator when the internal timers cause an IRQ */
static void irqhandler(int irq)
{
	cpu_set_irq_line(2,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2203interface ym2203_interface =
{
	1,			/* 1 chip */
	MAIN_XTAL/8,	/* 3 MHz */
	{ YM2203_VOL(25,25) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler }
};


static struct YM3526interface ym3526_interface =
{
	1,			/* 1 chip (no more supported) */
	MAIN_XTAL/8,	/* 3 MHz */
	{ 50 }		/* volume */
};


static struct OKIM6295interface okim6295_interface =
{
	1,                  /* 1 chip */
	{ 8000 },           /* ? frequency */
	{ REGION_SOUND1 },	/* memory region */
	{ 100 }
};


static MACHINE_DRIVER_START( missb2 )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, MAIN_XTAL/4)	/* 6 MHz */
	MDRV_CPU_MEMORY(missb2_readmem,missb2_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, MAIN_XTAL/4)	/* 6 MHz */
	MDRV_CPU_MEMORY(missb2_readmem2,missb2_writemem2)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, MAIN_XTAL/8)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 3 MHz */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
								/* IRQs are triggered by the YM2203 */
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)	/* 100 CPU slices per frame - an high value to ensure proper */
							/* synchronization of the CPUs */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_VIDEO_UPDATE(missb2)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, ym2203_interface) /* ?*/
	MDRV_SOUND_ADD(YM3526, ym3526_interface) /* ?*/
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END



ROM_START( missb2 )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )
	ROM_LOAD( "msbub2-u.204", 0x00000, 0x10000, CRC(b633bdde) SHA1(29a389c52ff06718f1c4c39f6a854856c237356b) ) /* FIRST AND SECOND HALF IDENTICAL */
	/* ROMs banked at 8000-bfff */
	ROM_LOAD( "msbub2-u.203", 0x10000, 0x10000, CRC(29fd8afe) SHA1(94ead80d20cd3974dd4fb0358915e3bd8b793158) )
	/* 20000-2ffff empty */

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the second CPU */
	ROM_LOAD( "msbub2-u.11",  0x0000, 0x10000, CRC(003dc092) SHA1(dff3c2b31d0804a308e5c42cf9705cd3d6144ad7) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* 64k for the third CPU */
	ROM_LOAD( "msbub2-u.211", 0x0000, 0x08000, CRC(08e5d846) SHA1(8509a71df984f0348bdc6ab60eb2ba7ceb9b1246) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "msbub2-u.14",  0x00000, 0x40000, CRC(b3164b47) SHA1(083a63010515b0aa43b482938ae302b2df985312) )
	ROM_LOAD( "msbub2-u.126", 0x40000, 0x40000, CRC(b0a9a353) SHA1(40d7f4c970d8571de319231c295fa0d2836efcf7) )
	ROM_LOAD( "msbub2-u.124", 0x80000, 0x40000, CRC(4b0d8e5b) SHA1(218da3edcfea228e6df1ac59bc24217713d79410) )
	ROM_LOAD( "msbub2-u.125", 0xc0000, 0x40000, CRC(77b710e2) SHA1(f6f46804a23de6c930bc40a3f45ac70e160f0645) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 ) /* background images */
	ROM_LOAD16_BYTE( "msbub2-u.ic1", 0x100001, 0x80000, CRC(d621cbc3) SHA1(36343d85bdde0e40dfe0f0e4e646546f175903f8) )
	ROM_LOAD16_BYTE( "msbub2-u.ic3", 0x100000, 0x80000, CRC(90e56035) SHA1(8fa18d97a05890178c52b97ff75aed300344a93e) )
	ROM_LOAD16_BYTE( "msbub2-u.ic2", 0x000001, 0x80000, CRC(694c2783) SHA1(401dc8713a02130289f364786c38e70c4c4f9b2e) )
	ROM_LOAD16_BYTE( "msbub2-u.ic4", 0x000000, 0x80000, CRC(be71c9f0) SHA1(1961e931017f644486cea0ce431d50973679c848) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* samples */
	ROM_LOAD( "msbub2-u.13", 0x00000, 0x20000, CRC(14f07386) SHA1(097897d92226f900e11dbbdd853aff3ac46ff016) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "a71-25.bin",  0x0000, 0x0100, CRC(2d0f8545) SHA1(089c31e2f614145ef2743164f7b52ae35bc06808) )	/* video timing - taken from bublbobl */
ROM_END

static DRIVER_INIT( missb2 )
{
	unsigned char *ROM = memory_region(REGION_CPU1);

	/* in Bubble Bobble, bank 0 has code falling from 7fff to 8000, */
	/* so I have to copy it there because bank switching wouldn't catch it */
	memcpy(ROM+0x08000,ROM+0x10000,0x4000);

}

GAMEX( 1996, missb2, bublbobl, missb2, missb2, missb2, ROT0,  "Alpha Co", "Miss Bubble 2", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
