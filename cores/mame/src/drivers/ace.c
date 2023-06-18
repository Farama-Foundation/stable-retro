/****************************************************************************

Ace by Allied Leisure

Driver by Jarek Burczynski
2002.09.19



Allied Leisure 1976
"MAJOR MFG. INC. SUNNYVALE, CA" in PCB etch

18MHz
                                                          5MHz

8080


2101
2101


A5               3106          3106         3106
A4
A3                                                      3622.K4
A2                                   2101
A1                   2101            2101

                                                         [ RANGE ] [ TIME ]
                                                        (two 0-9 thumbwheel switches)


5x2101 - SRAM 256x4
3x3106 - SRAM 256x1
1x3622 - ROM 512x4


****************************************************************************/

#include "driver.h"


static unsigned char *ace_scoreram;
static unsigned char *ace_ram2;
static unsigned char *ace_characterram;


static int objpos[8];

static WRITE_HANDLER( ace_objpos_w )
{
	objpos[offset]=data;
}

#if 0
static READ_HANDLER( ace_objpos_r )
{
	return objpos[offset];
}
#endif

VIDEO_UPDATE( ace )
{
	int offs;

	decodechar(Machine->gfx[1], 0, ace_characterram, Machine->drv->gfxdecodeinfo[1].gfxlayout);
	decodechar(Machine->gfx[2], 0, ace_characterram, Machine->drv->gfxdecodeinfo[2].gfxlayout);
	decodechar(Machine->gfx[3], 0, ace_characterram, Machine->drv->gfxdecodeinfo[3].gfxlayout);

	for (offs = 0; offs < 8; offs++)
	{
		decodechar(Machine->gfx[4], offs, ace_scoreram, Machine->drv->gfxdecodeinfo[4].gfxlayout);
	}

	/* first of all, fill the screen with the background color */
	fillbitmap(bitmap, Machine->pens[0], &Machine->visible_area);


		drawgfx(bitmap,Machine->gfx[1],
				0,
				0,
				0,0,
				objpos[0],objpos[1],
				&Machine->visible_area,TRANSPARENCY_NONE,0);

		drawgfx(bitmap,Machine->gfx[2],
				0,
				0,
				0,0,
				objpos[2],objpos[3],
				&Machine->visible_area,TRANSPARENCY_NONE,0);

		drawgfx(bitmap,Machine->gfx[3],
				0,
				0,
				0,0,
				objpos[4],objpos[5],
				&Machine->visible_area,TRANSPARENCY_NONE,0);

	for (offs = 0; offs < 8; offs++)
	{
		drawgfx(bitmap,Machine->gfx[4],
				offs,
				0,
				0,0,
				10*8+offs*16,256-16, /* ?? */
				&Machine->visible_area,TRANSPARENCY_NONE,0);
	}
}


static PALETTE_INIT( ace )
{
	palette_set_color(0,0x10,0x20,0xd0); /* light bluish */
	palette_set_color(1,0xff,0xff,0xff); /* white */
}


static READ_HANDLER( ace_characterram_r )
{
	return ace_characterram[offset];
}

static WRITE_HANDLER( ace_characterram_w )
{
	if (ace_characterram[offset] != data)
	{
		if (data&(~0x07))
		{
			log_cb(RETRO_LOG_DEBUG, LOGPRE "write to %04x data=%02x\n", 0x8000+offset, data);
			usrintf_showmessage("write to %04x data=%02x\n", 0x8000+offset, data);
		}
		ace_characterram[offset] = data;
	}
}


static READ_HANDLER( unk_r )
{
	return rand()&0xff;
}


static MEMORY_READ_START( readmem )

	{ 0x0000, 0x09ff, MRA_ROM },

	{ 0x2000, 0x20ff, MRA_RAM },
	{ 0x8300, 0x83ff, MRA_RAM },
	{ 0x8000, 0x80ff, ace_characterram_r },

	/* players inputs */
	{ 0xc008, 0xc008, input_port_0_r },
	{ 0xc009, 0xc009, input_port_1_r },
	{ 0xc00a, 0xc00a, input_port_2_r },
	{ 0xc00b, 0xc00b, input_port_3_r },
	{ 0xc00c, 0xc00c, input_port_4_r },
	{ 0xc00d, 0xc00d, input_port_5_r },
	{ 0xc00e, 0xc00e, input_port_6_r },
	{ 0xc00f, 0xc00f, input_port_7_r },
	{ 0xc010, 0xc010, input_port_8_r },
	{ 0xc011, 0xc011, input_port_9_r },

	{ 0xc012, 0xc012, unk_r },

	/* vblank */
	{ 0xc014, 0xc014, input_port_10_r },

	/* coin */
	{ 0xc015, 0xc015, input_port_11_r },

	/* start (must read 1 at least once to make the game run) */
	{ 0xc016, 0xc016, input_port_12_r },

	{ 0xc017, 0xc017, unk_r },
	{ 0xc018, 0xc018, unk_r },
	{ 0xc019, 0xc019, unk_r },

	{ 0xc020, 0xc020, unk_r },
	{ 0xc021, 0xc021, unk_r },
	{ 0xc022, 0xc022, unk_r },
	{ 0xc023, 0xc023, unk_r },
	{ 0xc024, 0xc024, unk_r },
	{ 0xc025, 0xc025, unk_r },
	{ 0xc026, 0xc026, unk_r },

MEMORY_END

/* 5x2101 - SRAM 256x4 */
/* 3x3106 - SRAM 256x1 */
/* 1x3622 - ROM 512x4  - doesn't seem to be used ????????????*/

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x09ff, MWA_ROM },

	{ 0x2000, 0x20ff, MWA_RAM, &ace_scoreram },	/* 2x2101 */
	{ 0x8300, 0x83ff, MWA_RAM, &ace_ram2 },		/* 2x2101 */
	{ 0x8000, 0x80ff, ace_characterram_w, &ace_characterram },	/* 3x3101 (3bits: 0, 1, 2) */

	{ 0xc000, 0xc005, ace_objpos_w },
MEMORY_END


INPUT_PORTS_START( ace )

	PORT_START	/* player thrust c008 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_PLAYER1 )

	PORT_START	/* player slowdown c009 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )

	PORT_START	/* player left c00a */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )

	PORT_START	/* player right c00b */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )

	PORT_START	/* player fire c00c */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )

	PORT_START	/* enemy thrust c00d */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_PLAYER2 )

	PORT_START	/* enemy slowdown c00e */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )

	PORT_START	/* enemy left c00f */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )

	PORT_START	/* enemy right c010 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )

	PORT_START	/* enemy fire c011 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )

/*c012*/


	PORT_START	/* VBLANK??? read from 0xc014 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START	/* coin input c015 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START	/* game start c016 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )

INPUT_PORTS_END


static struct GfxLayout charlayout =
{
	16,16,	/* 16*16 chars */
	8,	/* 8 characters */
	1,		/* 1 bit per pixel */
	{ 4 },	/* character rom is 512x4 bits (3622 type)*/
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3, 16+0, 16+1, 16+2, 16+3, 24+0, 24+1, 24+2, 24+3 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32, 8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	64*8	/* every char takes 64 consecutive bytes */
};

static struct GfxLayout charlayout0 =
{
	16,16,	/* 16*16 chars */
	1,	/* 1 characters */
	1,		/* 1 bit per pixel */
	{ 7 },	/* bit 0 in character ram */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	{ 0*128, 1*128, 2*128, 3*128, 4*128, 5*128, 6*128, 7*128, 8*128, 9*128, 10*128, 11*128, 12*128, 13*128, 14*128, 15*128 },
	256*8	/* every char takes 256 consecutive bytes */
};

static struct GfxLayout charlayout1 =
{
	16,16,	/* 16*16 chars */
	1,	/* 1 characters */
	1,		/* 1 bit per pixel */
	{ 6 },	/* bit 1 in character ram */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	{ 0*128, 1*128, 2*128, 3*128, 4*128, 5*128, 6*128, 7*128, 8*128, 9*128, 10*128, 11*128, 12*128, 13*128, 14*128, 15*128 },
	256*8	/* every char takes 256 consecutive bytes */
};

static struct GfxLayout charlayout2 =
{
	16,16,	/* 16*16 chars */
	1,	/* 1 characters */
	1,		/* 1 bit per pixel */
	{ 5 },	/* bit 2 in character ram */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	{ 0*128, 1*128, 2*128, 3*128, 4*128, 5*128, 6*128, 7*128, 8*128, 9*128, 10*128, 11*128, 12*128, 13*128, 14*128, 15*128 },
	256*8	/* every char takes 256 consecutive bytes */
};

static struct GfxLayout scorelayout =
{
	16,16,	/* 16*16 chars */
	8,	/* 8 characters */
	1,		/* 1 bit per pixel */
	{ 0 },	/*  */
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16, 8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	32*8	/* every char takes 32 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0     , &charlayout,  0, 2 },
	{ 0          , 0x8000, &charlayout0, 0, 2 },    /* the game dynamically modifies this */
	{ 0          , 0x8000, &charlayout1, 0, 2 },    /* the game dynamically modifies this */
	{ 0          , 0x8000, &charlayout2, 0, 2 },    /* the game dynamically modifies this */
	{ 0          , 0x8000, &scorelayout, 0, 2 },    /* the game dynamically modifies this */
	{ -1 } /* end of array */
};



static MACHINE_DRIVER_START( ace )

	/* basic machine hardware */
	MDRV_CPU_ADD(8080, 18000000 / 9)	/* 2 MHz ? */
	MDRV_CPU_MEMORY(readmem,writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(4*8, 32*8-1, 2*8, 32*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2)
	MDRV_COLORTABLE_LENGTH(2*2)

	MDRV_PALETTE_INIT(ace)
	MDRV_VIDEO_UPDATE(ace)

	/* sound hardware */
	/* ???? */

MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( ace )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for main CPU */
	ROM_LOAD( "ace.a1",		0x0000, 0x0200, CRC(16811834) SHA1(5502812dd161908eea3fa8851d7e5c1e22b0f8ff) )
	ROM_LOAD( "ace.a2",		0x0200, 0x0200, CRC(f9eae80e) SHA1(8865b86c7b5d57c76312c16f8a614bf35ffaf532) )
	ROM_LOAD( "ace.a3",		0x0400, 0x0200, CRC(c5c63b8c) SHA1(2079dd12ff0c4aafec19aeb9baa70fc9b6788356) )
	ROM_LOAD( "ace.a4",		0x0600, 0x0200, CRC(ea4503aa) SHA1(fea610124b9f7ea18d29b4e4599253ba1ee067e1) )
	ROM_LOAD( "ace.a5",		0x0800, 0x0200, CRC(623c58e7) SHA1(a92418bc323a1ae76eae8e094e4d6ebd1e8da14e) )

	/* not used - I couldn't guess when this should be displayed */
	ROM_REGION( 0x0200, REGION_GFX1, 0 )
	ROM_LOAD( "ace.k4",		0x0000, 0x0200, CRC(daa05ec6) SHA1(8b71ffb802293dc93f6b492ff128a704e676a5fd) )

ROM_END

GAMEX( 1976, ace, 0, ace, ace, 0, ROT0, "Allied Leisure", "Ace", GAME_NO_SOUND | GAME_IMPERFECT_COLORS )
