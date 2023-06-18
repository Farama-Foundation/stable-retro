/***************************************************************************

Find Out    (c) 1987

driver by Nicola Salmoria

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/8255ppi.h"

static UINT8 drawctrl[3];
static UINT8 color[8];


VIDEO_UPDATE( findout )
{
	copybitmap(bitmap,tmpbitmap,0,0,0,0,cliprect,TRANSPARENCY_NONE,0);
}

static WRITE_HANDLER( findout_drawctrl_w )
{
	drawctrl[offset] = data;
	if (offset == 2)
	{
		int i;
		for (i = 0; i < 8; i++)
			if (BIT(drawctrl[1],i)) color[i] = drawctrl[0] & 7;
	}
}

static WRITE_HANDLER( findout_bitmap_w )
{
	int sx,sy;
	int mask;

	mask = 0xff;

	sx = 8*(offset % 64);
	sy = offset / 64;

	if (mask & 0x80) plot_pixel(tmpbitmap,sx+0,sy, color[8-0-1]);
	if (mask & 0x40) plot_pixel(tmpbitmap,sx+1,sy, color[8-1-1]);
	if (mask & 0x20) plot_pixel(tmpbitmap,sx+2,sy, color[8-2-1]);
	if (mask & 0x10) plot_pixel(tmpbitmap,sx+3,sy, color[8-3-1]);
	if (mask & 0x08) plot_pixel(tmpbitmap,sx+4,sy, color[8-4-1]);
	if (mask & 0x04) plot_pixel(tmpbitmap,sx+5,sy, color[8-5-1]);
	if (mask & 0x02) plot_pixel(tmpbitmap,sx+6,sy, color[8-6-1]);
	if (mask & 0x01) plot_pixel(tmpbitmap,sx+7,sy, color[8-7-1]);
}

PALETTE_INIT( findout )
{
	int i;

	for (i = 0; i < 8; i++ )
	{
		palette_set_color(i, pal1bit(i >> 2), pal1bit(i), pal1bit(i >> 1));
	}
}

static READ_HANDLER( portC_r )
{
	return 4;
/*	return (rand()&2);*/
}

static WRITE_HANDLER( lamps_w )
{
	set_led_status(0,data & 0x01);
	set_led_status(1,data & 0x02);
	set_led_status(2,data & 0x04);
	set_led_status(3,data & 0x08);
	set_led_status(4,data & 0x10);
}

static WRITE_HANDLER( sound_w )
{
	/* bit 3 used but unknown */

	/* bit 6 enables NMI */
	interrupt_enable_w(0,data & 0x40);

	/* bit 7 goes directly to the sound amplifier */
	DAC_data_w(0,((data & 0x80) >> 7) * 255);

/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "%04x: sound_w %02x\n",activecpu_get_pc(),data);*/
/*	usrintf_showmessage("%02x",data);*/
}

static ppi8255_interface ppi8255_intf =
{
	2, 									/* 2 chips */
	{ input_port_0_r, input_port_2_r },	/* Port A read */
	{ input_port_1_r, NULL },			/* Port B read */
	{ NULL,           portC_r },		/* Port C read */
	{ NULL,           NULL },			/* Port A write */
	{ NULL,           lamps_w },		/* Port B write */
	{ sound_w,        NULL },			/* Port C write */
};

MACHINE_INIT( findout )
{
	ppi8255_init(&ppi8255_intf);
}


static READ_HANDLER( catchall )
{
	int pc = activecpu_get_pc();

	if (pc != 0x3c74 && pc != 0x0364 && pc != 0x036d)	/* weed out spurious blit reads */
		log_cb(RETRO_LOG_DEBUG, LOGPRE "%04x: unmapped memory read from %04x\n",pc,offset);

	return 0xff;
}

static WRITE_HANDLER( banksel_main_w )
{
	cpu_setbank(1,memory_region(REGION_CPU1) + 0x8000);
}
static WRITE_HANDLER( banksel_1_w )
{
	cpu_setbank(1,memory_region(REGION_CPU1) + 0x10000);
}
static WRITE_HANDLER( banksel_2_w )
{
	cpu_setbank(1,memory_region(REGION_CPU1) + 0x18000);
}
static WRITE_HANDLER( banksel_3_w )
{
	cpu_setbank(1,memory_region(REGION_CPU1) + 0x20000);
}
static WRITE_HANDLER( banksel_4_w )
{
	cpu_setbank(1,memory_region(REGION_CPU1) + 0x28000);
}
static WRITE_HANDLER( banksel_5_w )
{
	cpu_setbank(1,memory_region(REGION_CPU1) + 0x30000);
}


/* This signature is used to validate the question ROMs. Simple protection check? */
static int signature_answer,signature_pos;

static READ_HANDLER( signature_r )
{
	return signature_answer;
}

static WRITE_HANDLER( signature_w )
{
	if (data == 0) signature_pos = 0;
	else
	{
		static data8_t signature[8] = { 0xff, 0x01, 0xfd, 0x05, 0xf5, 0x15, 0xd5, 0x55 };

		signature_answer = signature[signature_pos++];

		signature_pos &= 7;	/* safety; shouldn't happen */
	}
}



static MEMORY_READ_START( readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x47ff, MRA_RAM },
	{ 0x4800, 0x4803, ppi8255_0_r },
	{ 0x5000, 0x5003, ppi8255_1_r },
	{ 0x6400, 0x6400, signature_r },
	{ 0x7800, 0x7fff, MRA_ROM },
	{ 0x8000, 0xffff, MRA_BANK1 },
	{ 0x0000, 0xffff, catchall },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x47ff, MWA_RAM, &generic_nvram, &generic_nvram_size },
	{ 0x4800, 0x4803, ppi8255_0_w },
	{ 0x5000, 0x5003, ppi8255_1_w },
	/* banked ROMs are enabled by low 6 bits of the address */
	{ 0x603e, 0x603e, banksel_1_w },
	{ 0x603d, 0x603d, banksel_2_w },
	{ 0x603b, 0x603b, banksel_3_w },
	{ 0x6037, 0x6037, banksel_4_w },
	{ 0x602f, 0x602f, banksel_5_w },
	{ 0x601f, 0x601f, banksel_main_w },
	{ 0x6200, 0x6200, signature_w },
	{ 0x7800, 0x7fff, MWA_ROM },	/* space for diagnostic ROM? */
	{ 0x8000, 0x8002, findout_drawctrl_w },
	{ 0xc000, 0xffff, findout_bitmap_w },
	{ 0x8000, 0xffff, MWA_ROM },	/* overlapped by the above */
MEMORY_END



INPUT_PORTS_START( findout )
	PORT_START
	PORT_DIPNAME( 0x07, 0x01, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 7C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x08, 0x00, "Game Repetition" )
	PORT_DIPSETTING( 0x08, DEF_STR( No ) )
	PORT_DIPSETTING( 0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x10, 0x10, "Orientation" )
	PORT_DIPSETTING( 0x10, "Horizontal" )
	PORT_DIPSETTING( 0x00, "Vertical" )
	PORT_DIPNAME( 0x20, 0x20, "Buy Letter" )
	PORT_DIPSETTING( 0x20, DEF_STR( No ) )
	PORT_DIPSETTING( 0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x40, 0x40, "Starting Letter" )
	PORT_DIPSETTING( 0x40, DEF_STR( No ) )
	PORT_DIPSETTING( 0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, "Bonus Letter" )
	PORT_DIPSETTING( 0x80, DEF_STR( No ) )
	PORT_DIPSETTING( 0x00, DEF_STR( Yes ) )

	PORT_START
	PORT_BIT_IMPULSE( 0x01, IP_ACTIVE_LOW, IPT_COIN1, 2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_SERVICE( 0x08, IP_ACTIVE_LOW )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON5 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END



static struct DACinterface dac_interface =
{
	1,
	{ 100 }
};



static MACHINE_DRIVER_START( findout )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,4000000)	/* 4 MHz ?????? (affects sound pitch) */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(findout)
	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER|VIDEO_PIXEL_ASPECT_RATIO_1_2)
	MDRV_SCREEN_SIZE(512, 256)
	MDRV_VISIBLE_AREA(48, 511-48, 16, 255-16)
	MDRV_PALETTE_LENGTH(8)
	MDRV_PALETTE_INIT(findout)

	MDRV_VIDEO_START(generic_bitmapped)
	MDRV_VIDEO_UPDATE(findout)

	/* sound hardware */
	MDRV_SOUND_ADD(DAC, dac_interface)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( findout )
	ROM_REGION( 0x38000, REGION_CPU1, 0 )
	ROM_LOAD( "12.bin",       0x00000, 0x4000, CRC(21132d4c) SHA1(e3562ee2f46b3f022a852a0e0b1c8fb8164f64a3) )
	ROM_LOAD( "11.bin",       0x08000, 0x2000, CRC(0014282c) SHA1(c6792f2ff712ba3759ff009950d78750df844d01) )	/* banked */
	ROM_LOAD( "13.bin",       0x10000, 0x8000, CRC(cea91a13) SHA1(ad3b395ab0362f3decf178824b1feb10b6335bb3) )	/* banked ROMs for solution data */
	ROM_LOAD( "14.bin",       0x18000, 0x8000, CRC(2a433a40) SHA1(4132d81256db940789a40aa1162bf1b3997cb23f) )
	ROM_LOAD( "15.bin",       0x20000, 0x8000, CRC(d817b31e) SHA1(11e6e1042ee548ce2080127611ce3516a0528ae0) )
	ROM_LOAD( "16.bin",       0x28000, 0x8000, CRC(143f9ac8) SHA1(4411e8ba853d7d5c032115ce23453362ab82e9bb) )
	ROM_LOAD( "17.bin",       0x30000, 0x8000, CRC(dd743bc7) SHA1(63f7e01ac5cda76a1d3390b6b83f4429b7d3b781) )

	ROM_REGION( 0x0200, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "82s147.bin",   0x0000, 0x0200, CRC(f3b663bb) SHA1(5a683951c8d3a2baac4b49e379d6e10e35465c8a) )	/* unknown */
ROM_END



GAMEX( 1987, findout, 0, findout, findout, 0, ROT0, "Elettronolo", "Find Out", GAME_IMPERFECT_SOUND )
