/***************************************************************************

  Capcom Baseball


  Somewhat similar to the "Mitchell hardware", but different enough to
  deserve its own driver.

TODO:
- understand what bit 6 of input port 0x12 is
- unknown bit 5 of bankswitch register

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/eeprom.h"


/* in machine/kabuki.c */
void pang_decode(void);


VIDEO_START( cbasebal );
WRITE_HANDLER( cbasebal_textram_w );
READ_HANDLER( cbasebal_textram_r );
WRITE_HANDLER( cbasebal_scrollram_w );
READ_HANDLER( cbasebal_scrollram_r );
WRITE_HANDLER( cbasebal_gfxctrl_w );
WRITE_HANDLER( cbasebal_scrollx_w );
WRITE_HANDLER( cbasebal_scrolly_w );
VIDEO_UPDATE( cbasebal );


static int rambank;

static WRITE_HANDLER( cbasebal_bankswitch_w )
{
	int bankaddress;
	unsigned char *RAM = memory_region(REGION_CPU1);

	/* bits 0-4 select ROM bank */
/*logerror("%04x: bankswitch %02x\n",activecpu_get_pc(),data);*/
	bankaddress = 0x10000 + (data & 0x1f) * 0x4000;
	cpu_setbank(1,&RAM[bankaddress]);

	/* bit 5 used but unknown */

	/* bits 6-7 select RAM bank */
	rambank = (data & 0xc0) >> 6;
}


static READ_HANDLER( bankedram_r )
{
	if (rambank == 2)
		return cbasebal_textram_r(offset);	/* VRAM */
	else if (rambank == 1)
	{
		if (offset < 0x800)
			return paletteram_r(offset);
		else return 0;
	}
	else
	{
		return cbasebal_scrollram_r(offset);	/* SCROLL */
	}
}

static WRITE_HANDLER( bankedram_w )
{
	if (rambank == 2)
		cbasebal_textram_w(offset,data);
	else if (rambank == 1)
	{
		if (offset < 0x800)
			paletteram_xxxxBBBBRRRRGGGG_w(offset,data);
	}
	else
		cbasebal_scrollram_w(offset,data);
}

static WRITE_HANDLER( cbasebal_coinctrl_w )
{
	coin_lockout_w(0,~data & 0x04);
	coin_lockout_w(1,~data & 0x08);
	coin_counter_w(0,data & 0x01);
	coin_counter_w(1,data & 0x02);
}



/***************************************************************************

  EEPROM

***************************************************************************/

static struct EEPROM_interface eeprom_interface =
{
	6,		/* address bits */
	16,		/* data bits */
	"0110",	/*  read command */
	"0101",	/* write command */
	"0111"	/* erase command */
};


static NVRAM_HANDLER( cbasebal )
{
	if (read_or_write)
		EEPROM_save(file);
	else
	{
		EEPROM_init(&eeprom_interface);

		if (file)
			EEPROM_load(file);
	}
}

static READ_HANDLER( eeprom_r )
{
	int bit;

	bit = EEPROM_read_bit() << 7;

	return (input_port_2_r(0) & 0x7f) | bit;
}

static WRITE_HANDLER( eeprom_cs_w )
{
	EEPROM_set_cs_line(data ? CLEAR_LINE : ASSERT_LINE);
}

static WRITE_HANDLER( eeprom_clock_w )
{
	EEPROM_set_clock_line(data ? CLEAR_LINE : ASSERT_LINE);
}

static WRITE_HANDLER( eeprom_serial_w )
{
	EEPROM_write_bit(data);
}



static MEMORY_READ_START( cbasebal_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xcfff, bankedram_r },
	{ 0xe000, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( cbasebal_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, bankedram_w, &paletteram },	/* palette + vram + scrollram */
	{ 0xe000, 0xfdff, MWA_RAM },			/* work RAM */
	{ 0xfe00, 0xffff, MWA_RAM, &spriteram, &spriteram_size },
MEMORY_END

static PORT_READ_START( cbasebal_readport )
	{ 0x10, 0x10, input_port_0_r },
	{ 0x11, 0x11, input_port_1_r },
	{ 0x12, 0x12, eeprom_r },
PORT_END

static PORT_WRITE_START( cbasebal_writeport )
	{ 0x00, 0x00, cbasebal_bankswitch_w },
	{ 0x01, 0x01, eeprom_cs_w },
	{ 0x02, 0x02, eeprom_clock_w },
	{ 0x03, 0x03, eeprom_serial_w },
	{ 0x05, 0x05, OKIM6295_data_0_w },
	{ 0x06, 0x06, YM2413_register_port_0_w },
	{ 0x07, 0x07, YM2413_data_port_0_w },
	{ 0x08, 0x09, cbasebal_scrollx_w },
	{ 0x0a, 0x0b, cbasebal_scrolly_w },
	{ 0x13, 0x13, cbasebal_gfxctrl_w },
	{ 0x14, 0x14, cbasebal_coinctrl_w },
PORT_END


INPUT_PORTS_START( cbasebal )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_SERVICE( 0x08, IP_ACTIVE_LOW )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_VBLANK )		/* ? */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* EEPROM data */
INPUT_PORTS_END




static struct GfxLayout cbasebal_textlayout =
{
	8,8,	/* 8*8 characters */
	4096,	/* 4096 characters */
	2,		/* 2 bits per pixel */
	{ 0, 4 },
	{ 8+3, 8+2, 8+1, 8+0, 3, 2, 1, 0 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8    /* every char takes 16 consecutive bytes */
};

static struct GfxLayout cbasebal_tilelayout =
{
	16,16,	/* 16*16 tiles */
	4096,	/* 4096 tiles */
	4,		/* 4 bits per pixel */
	{ 4096*64*8+4, 4096*64*8+0,4, 0 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3,
			16*16+0, 16*16+1, 16*16+2, 16*16+3, 16*16+8+0, 16*16+8+1, 16*16+8+2, 16*16+8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8    /* every tile takes 64 consecutive bytes */
};

static struct GfxLayout cbasebal_spritelayout =
{
	16,16,  /* 16*16 sprites */
	4096,   /* 2048 sprites */
	4,      /* 4 bits per pixel */
	{ 4096*64*8+4, 4096*64*8+0, 4, 0 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3,
			32*8+0, 32*8+1, 32*8+2, 32*8+3, 33*8+0, 33*8+1, 33*8+2, 33*8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8    /* every sprite takes 64 consecutive bytes */
};

static struct GfxDecodeInfo cbasebal_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &cbasebal_textlayout,   256,  8 }, /* colors 256- 287 */
	{ REGION_GFX2, 0, &cbasebal_tilelayout,   768, 16 }, /* colors 768-1023 */
	{ REGION_GFX3, 0, &cbasebal_spritelayout, 512,  8 }, /* colors 512- 639 */
	{ -1 } /* end of array */
};



static struct YM2413interface ym2413_interface=
{
	1,	/* 1 chip */
	3579545,	/* ???? */
	{ YM2413_VOL(100,MIXER_PAN_CENTER,100,MIXER_PAN_CENTER) }
};

static struct OKIM6295interface okim6295_interface =
{
	1,			/* 1 chip */
	{ 8000 },	/* 8000Hz ??? */
	{ REGION_SOUND1 },	/* memory region */
	{ 50 }
};



static MACHINE_DRIVER_START( cbasebal )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 6000000)	/* ??? */
	MDRV_CPU_MEMORY(cbasebal_readmem,cbasebal_writemem)
	MDRV_CPU_PORTS(cbasebal_readport,cbasebal_writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)	/* ??? */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(cbasebal)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(8*8, (64-8)*8-1, 2*8, 30*8-1 )
	MDRV_GFXDECODE(cbasebal_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(cbasebal)
	MDRV_VIDEO_UPDATE(cbasebal)

	/* sound hardware */
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
	MDRV_SOUND_ADD(YM2413, ym2413_interface)
MACHINE_DRIVER_END



ROM_START( cbasebal )
	ROM_REGION( 2*0x90000, REGION_CPU1, 0 )	/* 576k for code + 576k for decrypted opcodes */
	ROM_LOAD( "cbj10.11j",    0x00000, 0x08000, CRC(bbff0acc) SHA1(db9e2c89e030255851789caaf85f24dc73609d9b) )
	ROM_LOAD( "cbj07.16f",    0x10000, 0x20000, CRC(8111d13f) SHA1(264e21e824c87f55da326440c6ed71e1c287a63e) )
	ROM_LOAD( "cbj06.14f",    0x30000, 0x20000, CRC(9aaa0e37) SHA1(1a7b96b44c66b58f06707aafb1806520747b8c76) )
	ROM_LOAD( "cbj05.13f",    0x50000, 0x20000, CRC(d0089f37) SHA1(32354c3f4693a65e297791c4d8faac3aa9cff5a1) )
	/* 0x70000-0x8ffff empty (space for 04) */

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "cbj13.16m",    0x00000, 0x10000, CRC(2359fa0a) SHA1(3a37532ea43dd4b150c53a240d35a57a9b76d23d) )	/* text */

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "cbj02.1f",     0x00000, 0x20000, CRC(d6740535) SHA1(2ece885525718fd5fe52b8fa4c07930695b89659) )	/* tiles */
	ROM_LOAD( "cbj03.2f",     0x20000, 0x20000, CRC(88098dcd) SHA1(caddebeea581129d6a62fc9f7f354d61eef175c7) )
	ROM_LOAD( "cbj08.1j",     0x40000, 0x20000, CRC(5f3344bf) SHA1(1d3193078108e86e31bbfce15a8d2443cfbf2ff6) )
	ROM_LOAD( "cbj09.2j",     0x60000, 0x20000, CRC(aafffdae) SHA1(26e76b55fff49811df8e5b1f165be20ec8dd196a) )

	ROM_REGION( 0x80000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "cbj11.1m",     0x00000, 0x20000, CRC(bdc1507d) SHA1(efeaf3066acfb7186d73ad8e5b291d6e61965de2) )	/* sprites */
	ROM_LOAD( "cbj12.2m",     0x20000, 0x20000, CRC(973f3efe) SHA1(d776499d5ac4bc23eb5d1f28b88447cc07d8ac99) )
	ROM_LOAD( "cbj14.1n",     0x40000, 0x20000, CRC(765dabaa) SHA1(742d1c50b65f649f23eac7976fe26c2d7400e4e1) )
	ROM_LOAD( "cbj15.2n",     0x60000, 0x20000, CRC(74756de5) SHA1(791d6620cdb563f0b3a717432aa4647981b0a10e) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* OKIM */
	ROM_LOAD( "cbj01.1e",     0x00000, 0x20000, CRC(1d8968bd) SHA1(813e475d1d0c343e7dad516f1fe564d00c9c27fb) )
ROM_END


DRIVER_INIT( cbasebal )
{
	pang_decode();
}


GAME( 1989, cbasebal, 0, cbasebal, cbasebal, cbasebal, ROT0, "Capcom", "Capcom Baseball (Japan)" )
