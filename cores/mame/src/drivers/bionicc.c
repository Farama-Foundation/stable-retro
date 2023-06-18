/********************************************************************

			  Bionic Commando



ToDo:
- finish video driver
	Some attributes are unknown. I don't remember the original game but
	seems there are some problems:
	- misplaced sprites ? ( see beginning of level 1 or 2 for example )
	- sprite / sprite priority ? ( see level 2 the reflectors )
	- sprite / background priority ? ( see level 1: birds walk through
		branches of different trees )
	- see the beginning of level 3: is the background screwed ?

- get rid of input port hack

	Controls appear to be mapped at 0xFE4000, alongside dip switches, but there
	is something strange going on that I can't (yet) figure out.
	Player controls and coin inputs are supposed to magically appear at
	0xFFFFFB (coin/start)
	0xFFFFFD (player 2)
	0xFFFFFF (player 1)
	This is probably done by an MPU on the board (whose ROM is not
	available).

	The MPU also takes care of the commands for the sound CPU, which are stored
	at FFFFF9.

	IRQ4 seems to be control related.
	On each interrupt, it reads 0xFE4000 (coin/start), shift the bits around
	and move the resulting byte into a dword RAM location. The dword RAM location
	is rotated by 8 bits each time this happens.
	This is probably done to be pedantic about coin insertions (might be protection
	related). In fact, currently coin insertions are not consistently recognized.

********************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


WRITE16_HANDLER( bionicc_fgvideoram_w );
WRITE16_HANDLER( bionicc_bgvideoram_w );
WRITE16_HANDLER( bionicc_txvideoram_w );
WRITE16_HANDLER( bionicc_paletteram_w );
WRITE16_HANDLER( bionicc_scroll_w );
WRITE16_HANDLER( bionicc_gfxctrl_w );

extern data16_t *bionicc_bgvideoram;
extern data16_t *bionicc_fgvideoram;
extern data16_t *bionicc_txvideoram;

VIDEO_START( bionicc );
VIDEO_UPDATE( bionicc );
VIDEO_EOF( bionicc );

void bionicc_readinputs(void);
void bionicc_sound_cmd(int data);



static data16_t bionicc_inp[3];

WRITE16_HANDLER( hacked_controls_w )
{
log_cb(RETRO_LOG_DEBUG, LOGPRE "%06x: hacked_controls_w %04x %02x\n",activecpu_get_pc(),offset,data);
	COMBINE_DATA(&bionicc_inp[offset]);
}

static READ16_HANDLER( hacked_controls_r )
{
log_cb(RETRO_LOG_DEBUG, LOGPRE "%06x: hacked_controls_r %04x %04x\n",activecpu_get_pc(),offset,bionicc_inp[offset]);
	return bionicc_inp[offset];
}

static WRITE16_HANDLER( bionicc_mpu_trigger_w )
{
	data = readinputport(0) >> 12;
	bionicc_inp[0] = data ^ 0x0f;

	data = readinputport(3); /* player 2 controls */
	bionicc_inp[1] = data ^ 0xff;

	data = readinputport(2); /* player 1 controls */
	bionicc_inp[2] = data ^ 0xff;
}


static data16_t soundcommand;

WRITE16_HANDLER( hacked_soundcommand_w )
{
	COMBINE_DATA(&soundcommand);
	soundlatch_w(0,soundcommand & 0xff);
}

static READ16_HANDLER( hacked_soundcommand_r )
{
	return soundcommand;
}


/********************************************************************

  INTERRUPT

  The game runs on 2 interrupts.

  IRQ 2 drives the game
  IRQ 4 processes the input ports

  The game is very picky about timing. The following is the only
  way I have found it to work.

********************************************************************/

INTERRUPT_GEN( bionicc_interrupt )
{
	if (cpu_getiloops() == 0) 
		cpu_set_irq_line(0, 2, HOLD_LINE);
	else
		cpu_set_irq_line(0, 4, HOLD_LINE);
}

static MEMORY_READ16_START( readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },                /* 68000 ROM */
	{ 0xfe0000, 0xfe07ff, MRA16_RAM },                /* RAM? */
	{ 0xfe0800, 0xfe0cff, MRA16_RAM },                /* sprites */
	{ 0xfe0d00, 0xfe3fff, MRA16_RAM },                /* RAM? */
	{ 0xfe4000, 0xfe4001, input_port_0_word_r },
	{ 0xfe4002, 0xfe4003, input_port_1_word_r },
	{ 0xfec000, 0xfecfff, MRA16_RAM },
	{ 0xff0000, 0xff3fff, MRA16_RAM },
	{ 0xff4000, 0xff7fff, MRA16_RAM },
	{ 0xff8000, 0xff87ff, MRA16_RAM },
	{ 0xffc000, 0xfffff7, MRA16_RAM },                /* working RAM */
	{ 0xfffff8, 0xfffff9, hacked_soundcommand_r },      /* hack */
	{ 0xfffffa, 0xffffff, hacked_controls_r },      /* hack */
MEMORY_END

static MEMORY_WRITE16_START( writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0xfe0000, 0xfe07ff, MWA16_RAM },	/* RAM? */
	{ 0xfe0800, 0xfe0cff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0xfe0d00, 0xfe3fff, MWA16_RAM },              /* RAM? */
	{ 0xfe4000, 0xfe4001, bionicc_gfxctrl_w },	/* + coin counters */
	{ 0xfe8010, 0xfe8017, bionicc_scroll_w },
	{ 0xfe801a, 0xfe801b, bionicc_mpu_trigger_w },	/* ??? not sure, but looks like it */
	{ 0xfec000, 0xfecfff, bionicc_txvideoram_w, &bionicc_txvideoram },
	{ 0xff0000, 0xff3fff, bionicc_fgvideoram_w, &bionicc_fgvideoram },
	{ 0xff4000, 0xff7fff, bionicc_bgvideoram_w, &bionicc_bgvideoram },
	{ 0xff8000, 0xff87ff, bionicc_paletteram_w, &paletteram16 },
	{ 0xffc000, 0xfffff7, MWA16_RAM },	/* working RAM */
	{ 0xfffff8, 0xfffff9, hacked_soundcommand_w },      /* hack */
	{ 0xfffffa, 0xffffff, hacked_controls_w },	/* hack */
MEMORY_END


static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8001, 0x8001, YM2151_status_port_0_r },
	{ 0xa000, 0xa000, soundlatch_r },
	{ 0xc000, 0xc7ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x8000, YM2151_register_port_0_w },
	{ 0x8001, 0x8001, YM2151_data_port_0_w },
	{ 0xc000, 0xc7ff, MWA_RAM },
MEMORY_END



INPUT_PORTS_START( bionicc )
	PORT_START
	PORT_BIT( 0x0fff, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_6C ) )
	PORT_SERVICE( 0x0040, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0300, "3" )
	PORT_DIPSETTING(      0x0200, "4" )
	PORT_DIPSETTING(      0x0100, "5" )
	PORT_DIPSETTING(      0x0000, "7" )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Upright ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x1800, 0x1800, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(      0x1800, "20K, 40K, every 60K")
	PORT_DIPSETTING(      0x1000, "30K, 50K, every 70K" )
	PORT_DIPSETTING(      0x0800, "20K and 60K only")
	PORT_DIPSETTING(      0x0000, "30K and 70K only" )
	PORT_DIPNAME( 0x6000, 0x4000, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x4000, "Easy" )
	PORT_DIPSETTING(      0x6000, "Medium")
	PORT_DIPSETTING(      0x2000, "Hard")
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x8000, 0x8000, "Freeze" )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END



/********************************************************************

  GRAPHICS

********************************************************************/


static struct GfxLayout spritelayout_bionicc=
{
	16,16,  /* 16*16 sprites */
	2048,   /* 2048 sprites */
	4,      /* 4 bits per pixel */
	{ 0x30000*8,0x20000*8,0x10000*8,0 },
	{
		0,1,2,3,4,5,6,7,
		(16*8)+0,(16*8)+1,(16*8)+2,(16*8)+3,
		(16*8)+4,(16*8)+5,(16*8)+6,(16*8)+7
	},
	{
		0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
		8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8,
	},
	256   /* every sprite takes 256 consecutive bytes */
};

static struct GfxLayout vramlayout_bionicc=
{
	8,8,    /* 8*8 characters */
	1024,   /* 1024 character */
	2,      /* 2 bitplanes */
	{ 4,0 },
	{ 0,1,2,3,8,9,10,11 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	128   /* every character takes 128 consecutive bytes */
};

static struct GfxLayout scroll2layout_bionicc=
{
	8,8,    /* 8*8 tiles */
	2048,   /* 2048 tiles */
	4,      /* 4 bits per pixel */
	{ (0x08000*8)+4,0x08000*8,4,0 },
	{ 0,1,2,3, 8,9,10,11 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	128   /* every tile takes 128 consecutive bytes */
};

static struct GfxLayout scroll1layout_bionicc=
{
	16,16,  /* 16*16 tiles */
	2048,   /* 2048 tiles */
	4,      /* 4 bits per pixel */
	{ (0x020000*8)+4,0x020000*8,4,0 },
	{
		0,1,2,3, 8,9,10,11,
		(8*4*8)+0,(8*4*8)+1,(8*4*8)+2,(8*4*8)+3,
		(8*4*8)+8,(8*4*8)+9,(8*4*8)+10,(8*4*8)+11
	},
	{
		0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
		8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16
	},
	512   /* each tile takes 512 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo_bionicc[] =
{
	{ REGION_GFX1, 0, &vramlayout_bionicc,    768, 64 },	/* colors 768-1023 */
	{ REGION_GFX2, 0, &scroll2layout_bionicc,   0,  4 },	/* colors   0-  63 */
	{ REGION_GFX3, 0, &scroll1layout_bionicc, 256,  4 },	/* colors 256- 319 */
	{ REGION_GFX4, 0, &spritelayout_bionicc,  512, 16 },	/* colors 512- 767 */
	{ -1 }
};


static struct YM2151interface ym2151_interface =
{
	1,                      /* 1 chip */
	3579545,                /* 3.579545 MHz ? */
	{ YM3012_VOL(60,MIXER_PAN_LEFT,60,MIXER_PAN_RIGHT) },
	{ 0 }
};


static MACHINE_DRIVER_START( bionicc )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000) /* ?? MHz ? */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(bionicc_interrupt,8)

	MDRV_CPU_ADD(Z80, 3579545)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)  /* 4 MHz ??? TODO: find real FRQ */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,4)	/* ??? */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_BUFFERS_SPRITERAM)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo_bionicc)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(bionicc)
	MDRV_VIDEO_EOF(bionicc)
	MDRV_VIDEO_UPDATE(bionicc)

	MDRV_SOUND_ADD(YM2151, ym2151_interface)
MACHINE_DRIVER_END



ROM_START( bionicc )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_BYTE( "tsu_02b.rom",  0x00000, 0x10000, CRC(cf965a0a) SHA1(ab88742a3225a0b82ee2dfef6ed0058d3e11c38c) ) /* 68000 code */
	ROM_LOAD16_BYTE( "tsu_04b.rom",  0x00001, 0x10000, CRC(c9884bfb) SHA1(7d10cedff0a62847f8deb61a9611cc6661efb037) ) /* 68000 code */
	ROM_LOAD16_BYTE( "tsu_03b.rom",  0x20000, 0x10000, CRC(4e157ae2) SHA1(cc02931376d22a7fcfc320e6fd4129e03a461a49) ) /* 68000 code */
	ROM_LOAD16_BYTE( "tsu_05b.rom",  0x20001, 0x10000, CRC(e66ca0f9) SHA1(a503badf2fed38786d38c313d1dc315f3175d6de) ) /* 68000 code */

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the audio CPU */
	ROM_LOAD( "tsu_01b.rom",  0x00000, 0x8000, CRC(a9a6cafa) SHA1(55e0a0e6ca11e8e73339d5b4604e130031211291) )

	ROM_REGION( 0x08000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tsu_08.rom",   0x00000, 0x8000, CRC(9bf0b7a2) SHA1(1361335c3c2c8a9c6a7d99566048d8aac99e7c8f) )	/* VIDEORAM (text layer) tiles */

	ROM_REGION( 0x10000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "tsu_07.rom",   0x00000, 0x8000, CRC(9469efa4) SHA1(53c70361e8d9e54825f61b87a10df42438aaf5b0) )	/* SCROLL2 Layer Tiles */
	ROM_LOAD( "tsu_06.rom",   0x08000, 0x8000, CRC(40bf0eb4) SHA1(fcb186c31747e2c9872de01e34b3e713dc74df82) )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "ts_12.rom",    0x00000, 0x8000, CRC(e4b4619e) SHA1(3bec8399ffb28fd50ce6ae88d90b091eadf8bda1) )	/* SCROLL1 Layer Tiles */
	ROM_LOAD( "ts_11.rom",    0x08000, 0x8000, CRC(ab30237a) SHA1(ea6c07df992ba48f9eca7daa4ea775faa94358d2) )
	ROM_LOAD( "ts_17.rom",    0x10000, 0x8000, CRC(deb657e4) SHA1(b36b468f9bbb7a4937286230d3f6caa14c61d4dd) )
	ROM_LOAD( "ts_16.rom",    0x18000, 0x8000, CRC(d363b5f9) SHA1(1dd3991d99db2d6bcbdb12879ba50a01fef95004) )
	ROM_LOAD( "ts_13.rom",    0x20000, 0x8000, CRC(a8f5a004) SHA1(36ab0cb8ec9ce0519876f7461ccc5020c9c5b597) )
	ROM_LOAD( "ts_18.rom",    0x28000, 0x8000, CRC(3b36948c) SHA1(d85fcc0265ba1729c587b046cc5a7ba6f25363dd) )
	ROM_LOAD( "ts_23.rom",    0x30000, 0x8000, CRC(bbfbe58a) SHA1(9b1d5672b6f3c5c0952f8dcd0da71acc68a97a5e) )
	ROM_LOAD( "ts_24.rom",    0x38000, 0x8000, CRC(f156e564) SHA1(a6cad05bcc6d9ded6294f9b5aa856d05641aed02) )

	ROM_REGION( 0x40000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "tsu_10.rom",   0x00000, 0x8000, CRC(f1180d02) SHA1(312626af48235a1f726ab596f296ef4739785ca0) )	/* Sprites */
	ROM_LOAD( "tsu_09.rom",   0x08000, 0x8000, CRC(6a049292) SHA1(525c862061f426d679b539b6926af4c9f14b47b5) )
	ROM_LOAD( "tsu_15.rom",   0x10000, 0x8000, CRC(ea912701) SHA1(106336c63a1c8a0b13236268bc533a8263285cad) )
	ROM_LOAD( "tsu_14.rom",   0x18000, 0x8000, CRC(46b2ad83) SHA1(21ebd5691a544323fdfcf330b9a37bbe0428e3e3) )
	ROM_LOAD( "tsu_20.rom",   0x20000, 0x8000, CRC(17857ad2) SHA1(9f45cea6e9ce82bfc9ee6896a30257d20fb38bca) )
	ROM_LOAD( "tsu_19.rom",   0x28000, 0x8000, CRC(b5c82722) SHA1(969f9159f7d59e4e4c9ef9ddbdc27cbfa531eabf) )
	ROM_LOAD( "tsu_22.rom",   0x30000, 0x8000, CRC(5ee1ae6a) SHA1(76ca53d847c940c4176d79ba49b0c10efd6342e8) )
	ROM_LOAD( "tsu_21.rom",   0x38000, 0x8000, CRC(98777006) SHA1(bcc2058b639e9b71d16af05f63df298bcce91fdc) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "63s141.18f",   0x0000, 0x0100, CRC(b58d0023) SHA1(e8a4a2e2951bf73b3d9eed6957e9ee1e61c9c58a) )	/* priority (not used) */
ROM_END

ROM_START( bionicc2 )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_BYTE( "02",      0x00000, 0x10000, CRC(f2528f08) SHA1(04c793837c86d83312fd44b46a6a94378c90113b) ) /* 68000 code */
	ROM_LOAD16_BYTE( "04",      0x00001, 0x10000, CRC(38b1c7e4) SHA1(14bf743726c214bd00177e7b410c272dd7ab3d3f) ) /* 68000 code */
	ROM_LOAD16_BYTE( "03",      0x20000, 0x10000, CRC(72c3b76f) SHA1(f7f71eae7617e3348b727775088b496e86d51e38) ) /* 68000 code */
	ROM_LOAD16_BYTE( "05",      0x20001, 0x10000, CRC(70621f83) SHA1(0a77c2827a5c50457d90ccc62e463508d83d2f20) ) /* 68000 code */

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the audio CPU */
	ROM_LOAD( "tsu_01b.rom",  0x00000, 0x8000, CRC(a9a6cafa) SHA1(55e0a0e6ca11e8e73339d5b4604e130031211291) )

	ROM_REGION( 0x08000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tsu_08.rom",   0x00000, 0x8000, CRC(9bf0b7a2) SHA1(1361335c3c2c8a9c6a7d99566048d8aac99e7c8f) )	/* VIDEORAM (text layer) tiles */

	ROM_REGION( 0x10000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "tsu_07.rom",   0x00000, 0x8000, CRC(9469efa4) SHA1(53c70361e8d9e54825f61b87a10df42438aaf5b0) )	/* SCROLL2 Layer Tiles */
	ROM_LOAD( "tsu_06.rom",   0x08000, 0x8000, CRC(40bf0eb4) SHA1(fcb186c31747e2c9872de01e34b3e713dc74df82) )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "ts_12.rom",    0x00000, 0x8000, CRC(e4b4619e) SHA1(3bec8399ffb28fd50ce6ae88d90b091eadf8bda1) )	/* SCROLL1 Layer Tiles */
	ROM_LOAD( "ts_11.rom",    0x08000, 0x8000, CRC(ab30237a) SHA1(ea6c07df992ba48f9eca7daa4ea775faa94358d2) )
	ROM_LOAD( "ts_17.rom",    0x10000, 0x8000, CRC(deb657e4) SHA1(b36b468f9bbb7a4937286230d3f6caa14c61d4dd) )
	ROM_LOAD( "ts_16.rom",    0x18000, 0x8000, CRC(d363b5f9) SHA1(1dd3991d99db2d6bcbdb12879ba50a01fef95004) )
	ROM_LOAD( "ts_13.rom",    0x20000, 0x8000, CRC(a8f5a004) SHA1(36ab0cb8ec9ce0519876f7461ccc5020c9c5b597) )
	ROM_LOAD( "ts_18.rom",    0x28000, 0x8000, CRC(3b36948c) SHA1(d85fcc0265ba1729c587b046cc5a7ba6f25363dd) )
	ROM_LOAD( "ts_23.rom",    0x30000, 0x8000, CRC(bbfbe58a) SHA1(9b1d5672b6f3c5c0952f8dcd0da71acc68a97a5e) )
	ROM_LOAD( "ts_24.rom",    0x38000, 0x8000, CRC(f156e564) SHA1(a6cad05bcc6d9ded6294f9b5aa856d05641aed02) )

	ROM_REGION( 0x40000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "tsu_10.rom",   0x00000, 0x8000, CRC(f1180d02) SHA1(312626af48235a1f726ab596f296ef4739785ca0) )	/* Sprites */
	ROM_LOAD( "tsu_09.rom",   0x08000, 0x8000, CRC(6a049292) SHA1(525c862061f426d679b539b6926af4c9f14b47b5) )
	ROM_LOAD( "tsu_15.rom",   0x10000, 0x8000, CRC(ea912701) SHA1(106336c63a1c8a0b13236268bc533a8263285cad) )
	ROM_LOAD( "tsu_14.rom",   0x18000, 0x8000, CRC(46b2ad83) SHA1(21ebd5691a544323fdfcf330b9a37bbe0428e3e3) )
	ROM_LOAD( "tsu_20.rom",   0x20000, 0x8000, CRC(17857ad2) SHA1(9f45cea6e9ce82bfc9ee6896a30257d20fb38bca) )
	ROM_LOAD( "tsu_19.rom",   0x28000, 0x8000, CRC(b5c82722) SHA1(969f9159f7d59e4e4c9ef9ddbdc27cbfa531eabf) )
	ROM_LOAD( "tsu_22.rom",   0x30000, 0x8000, CRC(5ee1ae6a) SHA1(76ca53d847c940c4176d79ba49b0c10efd6342e8) )
	ROM_LOAD( "tsu_21.rom",   0x38000, 0x8000, CRC(98777006) SHA1(bcc2058b639e9b71d16af05f63df298bcce91fdc) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "63s141.18f",   0x0000, 0x0100, CRC(b58d0023) SHA1(e8a4a2e2951bf73b3d9eed6957e9ee1e61c9c58a) )	/* priority (not used) */
ROM_END

ROM_START( topsecrt )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_BYTE( "ts_02.rom",  0x00000, 0x10000, CRC(b2fe1ddb) SHA1(892f19124993add96edabdba3aafeecc6668c5d9) ) /* 68000 code */
	ROM_LOAD16_BYTE( "ts_04.rom",  0x00001, 0x10000, CRC(427a003d) SHA1(5a379fe2942e5565810939d5eb843003226222cc) ) /* 68000 code */
	ROM_LOAD16_BYTE( "ts_03.rom",  0x20000, 0x10000, CRC(27f04bb6) SHA1(41d17b84b34dc8b2e5dfa67794a8df3e898b740b) ) /* 68000 code */
	ROM_LOAD16_BYTE( "ts_05.rom",  0x20001, 0x10000, CRC(c01547b1) SHA1(563bf6be4f10f5e6eb5b562266accf168f62bf30) ) /* 68000 code */

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the audio CPU */
	ROM_LOAD( "ts_01.rom",    0x00000, 0x8000, CRC(8ea07917) SHA1(e9ace70d89482fc3669860450a41aacacbee9083) )

	ROM_REGION( 0x08000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ts_08.rom",    0x00000, 0x8000, CRC(96ad379e) SHA1(accd3a560b259c186bc28cdc004ed8de0b12f9d5) )	/* VIDEORAM (text layer) tiles */

	ROM_REGION( 0x10000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ts_07.rom",    0x00000, 0x8000, CRC(25cdf8b2) SHA1(316f6acc46878682dabeab12722e6a64504d23bd) )	/* SCROLL2 Layer Tiles */
	ROM_LOAD( "ts_06.rom",    0x08000, 0x8000, CRC(314fb12d) SHA1(dab0519a49b64fe7a837b3c6383f6147e1ab6ffd) )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "ts_12.rom",    0x00000, 0x8000, CRC(e4b4619e) SHA1(3bec8399ffb28fd50ce6ae88d90b091eadf8bda1) )	/* SCROLL1 Layer Tiles */
	ROM_LOAD( "ts_11.rom",    0x08000, 0x8000, CRC(ab30237a) SHA1(ea6c07df992ba48f9eca7daa4ea775faa94358d2) )
	ROM_LOAD( "ts_17.rom",    0x10000, 0x8000, CRC(deb657e4) SHA1(b36b468f9bbb7a4937286230d3f6caa14c61d4dd) )
	ROM_LOAD( "ts_16.rom",    0x18000, 0x8000, CRC(d363b5f9) SHA1(1dd3991d99db2d6bcbdb12879ba50a01fef95004) )
	ROM_LOAD( "ts_13.rom",    0x20000, 0x8000, CRC(a8f5a004) SHA1(36ab0cb8ec9ce0519876f7461ccc5020c9c5b597) )
	ROM_LOAD( "ts_18.rom",    0x28000, 0x8000, CRC(3b36948c) SHA1(d85fcc0265ba1729c587b046cc5a7ba6f25363dd) )
	ROM_LOAD( "ts_23.rom",    0x30000, 0x8000, CRC(bbfbe58a) SHA1(9b1d5672b6f3c5c0952f8dcd0da71acc68a97a5e) )
	ROM_LOAD( "ts_24.rom",    0x38000, 0x8000, CRC(f156e564) SHA1(a6cad05bcc6d9ded6294f9b5aa856d05641aed02) )

	ROM_REGION( 0x40000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "ts_10.rom",    0x00000, 0x8000, CRC(c3587d05) SHA1(ad0898a5d4cf110783ef092bf8e65b6ef31a8ae0) )	/* Sprites */
	ROM_LOAD( "ts_09.rom",    0x08000, 0x8000, CRC(6b63eef2) SHA1(5d1580db7f49c5994c2a08a36c2d05f3e246930d) )
	ROM_LOAD( "ts_15.rom",    0x10000, 0x8000, CRC(db8cebb0) SHA1(1cc9eac14851cde95fb2d69d6f5ffb08bc9c0d93) )
	ROM_LOAD( "ts_14.rom",    0x18000, 0x8000, CRC(e2e41abf) SHA1(d002d0d8fdbb9ec3e2eac218f6338f733953ca82) )
	ROM_LOAD( "ts_20.rom",    0x20000, 0x8000, CRC(bfd1a695) SHA1(bf93486b96bfa1a1d5015189043b07e6130e6df1) )
	ROM_LOAD( "ts_19.rom",    0x28000, 0x8000, CRC(928b669e) SHA1(98ea9d23a46b0700490fd2fa7ab4fb0988dd5ca6) )
	ROM_LOAD( "ts_22.rom",    0x30000, 0x8000, CRC(3fe05d9a) SHA1(32e28ef03fb82785019d1ae8b3859215b5368c2b) )
	ROM_LOAD( "ts_21.rom",    0x38000, 0x8000, CRC(27a9bb7c) SHA1(bb60332c0ecde4d7797960dec39c1079498175c3) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "63s141.18f",   0x0000, 0x0100, CRC(b58d0023) SHA1(e8a4a2e2951bf73b3d9eed6957e9ee1e61c9c58a) )	/* priority (not used) */
ROM_END



GAME( 1987, bionicc,  0,       bionicc, bionicc, 0, ROT0, "Capcom", "Bionic Commando (US set 1)" )
GAME( 1987, bionicc2, bionicc, bionicc, bionicc, 0, ROT0, "Capcom", "Bionic Commando (US set 2)" )
GAME( 1987, topsecrt, bionicc, bionicc, bionicc, 0, ROT0, "Capcom", "Top Secret (Japan)" )
