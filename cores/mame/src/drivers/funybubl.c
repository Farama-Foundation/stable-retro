/*

Funny Bubble ...

it a puzzloop rip-off .. but with a z80
the program roms say omega 1997
the gfx roms say 1999
title screen has no date

( a z80 as the main cpu in 1999 ??! )


banking might not be 100%

*/

#include "driver.h"

data8_t* banked_videoram;



static WRITE_HANDLER ( vidram_bank_w )
{
	if ((data&1) == 0)
		cpu_setbank(1,&banked_videoram[0x000000]);
	else
		cpu_setbank(1,&banked_videoram[0x001000]);
}

static WRITE_HANDLER ( bank2_w )
{
	unsigned char *rom = memory_region(REGION_CPU1);

		cpu_setbank(2,&rom[0x10000+0x4000*(data&0x3f)]);
}

static data8_t *funybubl_paletteram;


/* wrong i guess */
static WRITE_HANDLER ( funybubl_paldatawrite )
{
	int colchanged ;

	UINT32 coldat;
	int r,g,b;

	funybubl_paletteram[offset] = data;

	colchanged = offset >> 2;

	coldat = funybubl_paletteram[colchanged*4] | (funybubl_paletteram[colchanged*4+1] << 8) | (funybubl_paletteram[colchanged*4+2] << 16) | (funybubl_paletteram[colchanged*4+3] << 24);

	g = coldat & 0x003f;
	b = (coldat >> 6) & 0x3f;
	r = (coldat >> 12) & 0x3f;

	palette_set_color(colchanged,r<<2,g<<2,b<<2);

}


static READ_HANDLER ( unk_port_r )
{
	return 0xff;
}

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK2 }, /* banked port 1?*/
	{ 0xc400, 0xc7ff, MRA_RAM },
	{ 0xc800, 0xcfff, MRA_RAM },
	{ 0xd000, 0xdfff, MRA_BANK1 }, /* banked port 0?*/
	{ 0xe000, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0xbfff, MWA_ROM },
	{ 0xc400, 0xcfff, funybubl_paldatawrite, &funybubl_paletteram }, /* palette?*/
	{ 0xd000, 0xdfff, MWA_BANK1 }, /* banked port 0?*/
	{ 0xe000, 0xffff, MWA_RAM },
MEMORY_END

static PORT_READ_START( readport )
	{ 0x00, 0x00, input_port_0_r	},
	{ 0x01, 0x01, input_port_1_r	},
	{ 0x02, 0x02, input_port_2_r	},
	{ 0x03, 0x03, input_port_3_r	},

	{ 0x06, 0x06, input_port_4_r	},
PORT_END

WRITE_HANDLER( funybubl_soundcommand_w )
{
	soundlatch_w(0,data);
	cpu_set_irq_line(1,0, PULSE_LINE);
}

WRITE_HANDLER( funybubl_oki_bank_sw )
{
	OKIM6295_set_bank_base(0, ((data & 1) * 0x40000));
}

static PORT_WRITE_START( writeport )
	{ 0x00, 0x00, vidram_bank_w	},	/* vidram bank*/
	{ 0x01, 0x01, bank2_w }, /* rom bank?*/

	{ 0x03, 0x03, funybubl_soundcommand_w	},


PORT_END

/* sound cpu */


static MEMORY_READ_START( soundreadmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM }, /* ram?*/
	{ 0x9800, 0x9800, OKIM6295_status_0_r },
	{ 0xa000, 0xa000, soundlatch_r },
MEMORY_END

static MEMORY_WRITE_START( soundwritemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM }, /* ram?*/
	{ 0x9000, 0x9000, funybubl_oki_bank_sw },
	{ 0x9800, 0x9800, OKIM6295_data_0_w },
MEMORY_END



INPUT_PORTS_START( funybubl )
	PORT_START	/* System inputs */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */

	PORT_START	/* Player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP     | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN   | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT   | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */

	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP     | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN   | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT   | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */

	PORT_START	/* DSW 1 */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x30, "Very Easy" )
	PORT_DIPSETTING(    0x28, "Easy" )
	PORT_DIPSETTING(    0x38, "Normal" )
	PORT_DIPSETTING(    0x20, "Hard 1" )
	PORT_DIPSETTING(    0x18, "Hard 2" )
	PORT_DIPSETTING(    0x10, "Hard 3" )
	PORT_DIPSETTING(    0x08, "Hard 4" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "Nudity" )
	PORT_DIPSETTING(    0x80, "Semi" )
	PORT_DIPSETTING(    0x00, "Full" )

	/* Looks like the PCB shows an empty DSW 2 location */
INPUT_PORTS_END



static struct GfxLayout tiles8x8_layout =
{
	8,8,
	RGN_FRAC(1,8),
	8,
	{ RGN_FRAC(3,8),RGN_FRAC(2,8),RGN_FRAC(1,8),RGN_FRAC(0,8),RGN_FRAC(7,8),RGN_FRAC(6,8),RGN_FRAC(5,8), RGN_FRAC(4,8) },
	{ 0, 1, 2,  3,  4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static struct GfxLayout tiles8x8xx_layout =
{
	16,16,
	RGN_FRAC(1,4),
	8,
	{ RGN_FRAC(3,4)+4,RGN_FRAC(3,4)+0, RGN_FRAC(2,4)+4, RGN_FRAC(2,4)+0, RGN_FRAC(1,4)+4, RGN_FRAC(1,4)+0, RGN_FRAC(0,4)+4, RGN_FRAC(0,4)+0  },
	{ 0, 1,2,3, 8,9,10,11, 256,257,258,259, 264,265,266,267},
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
	  8*16, 9*16,10*16,11*16,12*16,13*16,14*16,15*16  },
	32*16
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tiles8x8_layout, 0, 16 },
	{ REGION_GFX2, 0, &tiles8x8xx_layout, 0, 16 },
	{ -1 }
};



static struct OKIM6295interface okim6295_interface =
{
	1,                  /* 1 chip */
	{ 8000 },           /* ? frequency */
	{ REGION_SOUND1 },	/* memory region */
	{ 100 }
};

DRIVER_INIT( funybubl )
{



	/* we allocate the memory here so its easier to share between cpus */
	banked_videoram = auto_malloc (0x2000);

	cpu_setbank(1,&banked_videoram[0x000000]);

}


VIDEO_START(funybubl)
{
	return 0;
}


static void funybubl_drawsprites( struct mame_bitmap *bitmap, const struct rectangle *cliprect )
{
  UINT8 *source = &banked_videoram[0x2000-0x20];
  UINT8 *finish = source - 0x1000;

  while( source>finish )
  {
    int xpos, ypos, tile;

    ypos = source[2];
    xpos = source[3];
    tile =  source[0] | ( (source[1] & 0x0f) <<8);
    if (source[1] & 0x80) tile += 0x1000;
    if (source[1] & 0x20) {	if (xpos < 0xe0) xpos += 0x100; }
    /* bits 0x40 and 0x10 not used?... */

    drawgfx(bitmap,Machine->gfx[1],tile,0,0,0,xpos,ypos,cliprect,TRANSPARENCY_PEN,255);

    source -= 0x20;
  }

}



VIDEO_UPDATE(funybubl)
{
	int x,y, offs;

	offs = 0;

	fillbitmap(bitmap, get_black_pen(), cliprect);


	/* tilemap .. convert it .. banking makes it slightly more annoying but still easy */
	for (y = 0; y < 32; y++)
	{
		for (x = 0; x< 64; x++)
		{
			int data;

			data = banked_videoram[offs] | (banked_videoram[offs+1] << 8);
			drawgfx(bitmap,Machine->gfx[0],data&0x7fff,(data&0x8000)?2:1,0,0,x*8,y*8,cliprect,TRANSPARENCY_PEN,0);
			offs+=2;
		}

	}

	funybubl_drawsprites(bitmap,cliprect);

/*
	if ( keyboard_pressed_memory(KEYCODE_W) )
	{
		FILE *fp;

		fp=fopen("funnybubsprites", "w+b");
		if (fp)
		{
			fwrite(&banked_videoram[0x1000], 0x1000, 1, fp);
			fclose(fp);
		}
	}
*/


}

static MACHINE_DRIVER_START( funybubl )
	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,8000000)		 /* ? MHz */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_PORTS(readport,writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80,8000000)		 /* ? MHz */
	MDRV_CPU_MEMORY(soundreadmem,soundwritemem)
/*	MDRV_CPU_PORTS(readport,writeport)*/
/*	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)*/

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE(512, 256)
	MDRV_VISIBLE_AREA(12*8, 512-12*8-1, 16, 256-16-1)
/*	MDRV_VISIBLE_AREA(0*8, 512-1, 0, 256-1)*/

	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x400)

	MDRV_VIDEO_START(funybubl)
	MDRV_VIDEO_UPDATE(funybubl)

	/* sound hardware */
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END



ROM_START( funybubl )
	ROM_REGION( 0x50000, REGION_CPU1, 0 ) /* main z80, lots of banked data */
	ROM_LOAD( "2.bin", 0x00000, 0x40000, CRC(d684c13f) SHA1(6a58b44dd775f374d6fd476a8fd175c28a83a495)  )
	ROM_RELOAD ( 0x10000, 0x40000 )


	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT  ) /* bg gfx 8x8x8*/
	ROM_LOAD( "7.bin", 0x000000, 0x40000, CRC(87603d7b) SHA1(21aec4cd011691f8608c3ddab83697bd89634fc8) )
	ROM_LOAD( "8.bin", 0x040000, 0x40000, CRC(ab6031bd) SHA1(557793817f98c07c82caab4293aed7dffa4dbf7b) )
	ROM_LOAD( "9.bin", 0x080000, 0x40000, CRC(0e8352ff) SHA1(29679a7ece2585e1a66296439b68bd56c937e313) )
	ROM_LOAD( "10.bin",0x0c0000, 0x40000, CRC(df7dd356) SHA1(13b9f40714dfa7b8cebc0191dcdde88b51f5e78c) )
	ROM_LOAD( "13.bin",0x100000, 0x40000, CRC(9f57bdd5) SHA1(6fd60da5f5eee0251e3a08957952ed9f037eeaec) )
	ROM_LOAD( "14.bin",0x140000, 0x40000, CRC(2ac15ea3) SHA1(de5be6378b4b6eee6faf532c9ef14bd609041cb3) )
	ROM_LOAD( "15.bin",0x180000, 0x40000, CRC(9a5e66a6) SHA1(cbe727e4f1e9a7072520d2e30eb0047cc67bff1b) )
	ROM_LOAD( "16.bin",0x1c0000, 0x40000, CRC(218060b3) SHA1(35124afce7f0f998b5c4761bbc888235de4e56ef) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "5.bin", 0x000000, 0x80000, CRC(b7ebbc00) SHA1(92520fda2f8f242b8cd49aeaac21b279f48276bf) )
	ROM_LOAD( "6.bin", 0x080000, 0x80000, CRC(28afc396) SHA1(555d51948ffb237311112dcfd0516a43f603ff03) )
	ROM_LOAD( "11.bin",0x100000, 0x80000, CRC(9e8687cd) SHA1(42fcba2532ae5028fcfc1df50750d99ad2586820) )
	ROM_LOAD( "12.bin",0x180000, 0x80000, CRC(63f0e810) SHA1(5c7ed32ee8dc1d9aabc8d136ec370471096356c2) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound z80 (not much code here ..) */
	ROM_LOAD( "1.bin", 0x00000,  0x10000, CRC(b8b5b675) SHA1(0a02ccd09bb2ae20efe49e3ca2006331aea0e2a7) )
	ROM_FILL(          0x08000,  0x08000, 0x00 )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "3.bin", 0x00000,  0x20000, CRC(a2d780f4) SHA1(bebba3db21ab9ddde8c6f19db3b67c869df582eb) )
	ROM_RELOAD(        0x40000,  0x20000 )
	ROM_LOAD( "4.bin", 0x20000,  0x20000, CRC(1f7e9269) SHA1(5c16b49a4e94aec7606d088c2d45a77842ab565b) )
	ROM_CONTINUE(      0x60000,  0x20000 )
ROM_END


GAME( 1999, funybubl, 0, funybubl, funybubl, funybubl, ROT0, "Comad", "Funny Bubble" )
