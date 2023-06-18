/***************************************************************************

Jailbreak - (c) 1986 Konami

Ernesto Corvi
ernesto@imagina.com

***************************************************************************/

/*

	TODO:

    - various unknown writes (NOPed out in the memory map)

*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m6809/m6809.h"


void konami1_decode(void);

extern UINT8 *jailbrek_scroll_x;
extern UINT8 *jailbrek_scroll_dir;

extern WRITE_HANDLER( jailbrek_videoram_w );
extern WRITE_HANDLER( jailbrek_colorram_w );

extern PALETTE_INIT( jailbrek );
extern VIDEO_START( jailbrek );
extern VIDEO_UPDATE( jailbrek );


static int irq_enable,nmi_enable;


static WRITE_HANDLER( ctrl_w )
{
	nmi_enable = data & 0x01;
	irq_enable = data & 0x02;
	flip_screen_set(data & 0x08);
}

static WRITE_HANDLER( coin_w )
{
	coin_counter_w(0, data & 0x01);
	coin_counter_w(1, data & 0x02);
}

static INTERRUPT_GEN( jb_interrupt )
{
	if (irq_enable)
		cpu_set_irq_line(0, 0, HOLD_LINE);
}

static INTERRUPT_GEN( jb_interrupt_nmi )
{
	if (nmi_enable)
		cpu_set_irq_line(0, IRQ_LINE_NMI, PULSE_LINE);
}


static READ_HANDLER( jailbrek_speech_r ) {
	return ( VLM5030_BSY() ? 1 : 0 );
}

static WRITE_HANDLER( jailbrek_speech_w ) {
	/* bit 0 could be latch direction like in yiear */
	VLM5030_ST( ( data >> 1 ) & 1 );
	VLM5030_RST( ( data >> 2 ) & 1 );
}

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x07ff, MRA_RAM },
	{ 0x0800, 0x0fff, MRA_RAM },
	{ 0x1000, 0x10bf, MRA_RAM }, /* sprites */
	{ 0x10c0, 0x14ff, MRA_RAM }, /* ??? */
	{ 0x1500, 0x1fff, MRA_RAM }, /* work ram */
	{ 0x2000, 0x203f, MRA_RAM }, /* scroll registers */
	{ 0x3100, 0x3100, input_port_4_r }, /* DSW1 */
	{ 0x3200, 0x3200, input_port_5_r }, /* DSW2 */
	{ 0x3300, 0x3300, input_port_0_r }, /* coins, start */
	{ 0x3301, 0x3301, input_port_1_r }, /* joy1 */
	{ 0x3302, 0x3302, input_port_2_r }, /* joy2 */
	{ 0x3303, 0x3303, input_port_3_r }, /* DSW0 */
	{ 0x6000, 0x6000, jailbrek_speech_r },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x07ff, jailbrek_colorram_w, &colorram },
	{ 0x0800, 0x0fff, jailbrek_videoram_w, &videoram },
	{ 0x1000, 0x10bf, MWA_RAM, &spriteram, &spriteram_size }, /* sprites */
	{ 0x10c0, 0x14ff, MWA_RAM }, /* ??? */
	{ 0x1500, 0x1fff, MWA_RAM }, /* work ram */
	{ 0x2000, 0x203f, MWA_RAM, &jailbrek_scroll_x }, /* scroll registers */
	{ 0x2040, 0x2040, MWA_NOP }, /* ??? */
	{ 0x2041, 0x2041, MWA_NOP }, /* ??? */
	{ 0x2042, 0x2042, MWA_RAM, &jailbrek_scroll_dir }, /* bit 2 = scroll direction */
	{ 0x2043, 0x2043, MWA_NOP }, /* ??? */
	{ 0x2044, 0x2044, ctrl_w }, /* irq, nmi enable, screen flip */
	{ 0x3000, 0x3000, coin_w }, /* coin counter */
	{ 0x3100, 0x3100, SN76496_0_w }, /* SN76496 data write */
	{ 0x3200, 0x3200, MWA_NOP },	/* mirror of the previous? */
	{ 0x3300, 0x3300, watchdog_reset_w }, /* watchdog */
	{ 0x4000, 0x4000, jailbrek_speech_w }, /* speech pins */
	{ 0x5000, 0x5000, VLM5030_data_w }, /* speech data */
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END



INPUT_PORTS_START( jailbrek )
	PORT_START	/* IN0 - $3300 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN1 - $3301 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )	/* shoot*/
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )	/* select*/
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 - $3302 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* DSW0  - $3303 */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, "Invalid" )

	PORT_START	/* DSW1  - $3100 */
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x08, "30K 70K" )
	PORT_DIPSETTING(    0x00, "40K 80K" )
	PORT_DIPNAME( 0x30, 0x10, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x30, "Easy" )
	PORT_DIPSETTING(    0x20, "Normal" )
	PORT_DIPSETTING(    0x10, "Difficult" )
	PORT_DIPSETTING(    0x00, "Very Difficult" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* DSW2  - $3200 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Upright Controls" )
	PORT_DIPSETTING(    0x02, "Single" )
	PORT_DIPSETTING(    0x00, "Dual" )
	PORT_BIT( 0x0c, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	1024,	/* 1024 characters */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },	/* the four bitplanes are packed in one nibble */
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8	/* every char takes 32 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	512,	/* 512 sprites */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },	/* the bitplanes are packed in one nibble */
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4,
			32*8+0*4, 32*8+1*4, 32*8+2*4, 32*8+3*4, 32*8+4*4, 32*8+5*4, 32*8+6*4, 32*8+7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			16*32, 17*32, 18*32, 19*32, 20*32, 21*32, 22*32, 23*32 },
	128*8	/* every sprite takes 128 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,   0, 16 }, /* characters */
	{ REGION_GFX2, 0, &spritelayout, 16*16, 16 }, /* sprites */
	{ -1 } /* end of array */
};



static struct SN76496interface sn76496_interface =
{
	1,	/* 1 chip */
	{ 1500000 },	/*  1.5 MHz ? (hand tuned) */
	{ 100 }
};

static struct VLM5030interface vlm5030_interface =
{
	3580000,    /* master clock */
	100,        /* volume       */
	REGION_SOUND1,	/* memory region of speech rom */
	0           /* memory size of speech rom */
};

static MACHINE_DRIVER_START( jailbrek )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6809, 3000000)        /* 3 MHz ??? */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(jb_interrupt,1)
	MDRV_CPU_PERIODIC_INT(jb_interrupt_nmi,500) /* ? */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(1*8, 31*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32)
	MDRV_COLORTABLE_LENGTH(512)

	MDRV_PALETTE_INIT(jailbrek)
	MDRV_VIDEO_START(jailbrek)
	MDRV_VIDEO_UPDATE(jailbrek)

	/* sound hardware */
	MDRV_SOUND_ADD(SN76496, sn76496_interface)
	MDRV_SOUND_ADD(VLM5030, vlm5030_interface)
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( jailbrek )
    ROM_REGION( 2*0x10000, REGION_CPU1, 0 )     /* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "jailb11d.bin", 0x8000, 0x4000, CRC(a0b88dfd) SHA1(f999e382b9d3b812fca41f4d0da3ea692fef6b19) )
	ROM_LOAD( "jailb9d.bin",  0xc000, 0x4000, CRC(444b7d8e) SHA1(c708b67c2d249448dae9a3d10c24d13ba6849597) )

    ROM_REGION( 0x08000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "jailb4f.bin",  0x00000, 0x4000, CRC(e3b7a226) SHA1(c19a02a2def65648bf198fccec98ebbd2fc7c0fb) )	/* characters */
    ROM_LOAD( "jailb5f.bin",  0x04000, 0x4000, CRC(504f0912) SHA1(b51a45dd5506bccdf0061dd6edd7f49ac86ed0f8) )

    ROM_REGION( 0x10000, REGION_GFX2, ROMREGION_DISPOSE )
    ROM_LOAD( "jailb3e.bin",  0x00000, 0x4000, CRC(0d269524) SHA1(a10ddb405e884bfec521a3c7a29d22f63e535b59) )	/* sprites */
    ROM_LOAD( "jailb4e.bin",  0x04000, 0x4000, CRC(27d4f6f4) SHA1(c42c064dbd7c5cf0b1d99651367e0bee1728a5b0) )
    ROM_LOAD( "jailb5e.bin",  0x08000, 0x4000, CRC(717485cb) SHA1(22609489186dcb3d7cd49b7ddfdc6f04d0739354) )
    ROM_LOAD( "jailb3f.bin",  0x0c000, 0x4000, CRC(e933086f) SHA1(c0fd1e8d23c0f7e14c0b75f629448034420cf8ef) )

	ROM_REGION( 0x0240, REGION_PROMS, 0 )
	ROM_LOAD( "jailbbl.cl2",  0x0000, 0x0020, CRC(f1909605) SHA1(91eaa865375b3bc052897732b64b1ff7df3f78f6) ) /* red & green */
	ROM_LOAD( "jailbbl.cl1",  0x0020, 0x0020, CRC(f70bb122) SHA1(bf77990260e8346faa3d3481718cbe46a4a27150) ) /* blue */
	ROM_LOAD( "jailbbl.bp2",  0x0040, 0x0100, CRC(d4fe5c97) SHA1(972e9dab6c53722545dd3a43e3ada7921e88708b) ) /* char lookup */
	ROM_LOAD( "jailbbl.bp1",  0x0140, 0x0100, CRC(0266c7db) SHA1(a8f21e86e6d974c9bfd92a147689d0e7316d66e2) ) /* sprites lookup */

	ROM_REGION( 0x2000, REGION_SOUND1, 0 ) /* speech rom */
	ROM_LOAD( "jailb8c.bin",  0x0000, 0x2000, CRC(d91d15e3) SHA1(475fe50aafbf8f2fb79880ef0e2c25158eda5270) )
ROM_END

ROM_START( manhatan )
    ROM_REGION( 2*0x10000, REGION_CPU1, 0 )     /* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "507n03.9d",    0x8000, 0x4000, CRC(e5039f7e) SHA1(0f12484ed40444d978e0405c27bdd027ae2e2a0b) )
	ROM_LOAD( "507n02.11d",   0xc000, 0x4000, CRC(143cc62c) SHA1(9520dbb1b6f1fa439e03d4caa9bed96ef8f805f2) )

    ROM_REGION( 0x08000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "507j08.4f",    0x00000, 0x4000, CRC(175e1b49) SHA1(4cfe982cdf7729bd05c6da803480571876320bf6) )	/* characters */
    ROM_LOAD( "jailb5f.bin",  0x04000, 0x4000, CRC(504f0912) SHA1(b51a45dd5506bccdf0061dd6edd7f49ac86ed0f8) )

    ROM_REGION( 0x10000, REGION_GFX2, ROMREGION_DISPOSE )
    ROM_LOAD( "jailb3e.bin",  0x00000, 0x4000, CRC(0d269524) SHA1(a10ddb405e884bfec521a3c7a29d22f63e535b59) )	/* sprites */
    ROM_LOAD( "jailb4e.bin",  0x04000, 0x4000, CRC(27d4f6f4) SHA1(c42c064dbd7c5cf0b1d99651367e0bee1728a5b0) )
    ROM_LOAD( "jailb5e.bin",  0x08000, 0x4000, CRC(717485cb) SHA1(22609489186dcb3d7cd49b7ddfdc6f04d0739354) )
    ROM_LOAD( "jailb3f.bin",  0x0c000, 0x4000, CRC(e933086f) SHA1(c0fd1e8d23c0f7e14c0b75f629448034420cf8ef) )

	ROM_REGION( 0x0240, REGION_PROMS, 0 )
	ROM_LOAD( "jailbbl.cl2",  0x0000, 0x0020, CRC(f1909605) SHA1(91eaa865375b3bc052897732b64b1ff7df3f78f6) ) /* red & green */
	ROM_LOAD( "jailbbl.cl1",  0x0020, 0x0020, CRC(f70bb122) SHA1(bf77990260e8346faa3d3481718cbe46a4a27150) ) /* blue */
	ROM_LOAD( "jailbbl.bp2",  0x0040, 0x0100, CRC(d4fe5c97) SHA1(972e9dab6c53722545dd3a43e3ada7921e88708b) ) /* char lookup */
	ROM_LOAD( "jailbbl.bp1",  0x0140, 0x0100, CRC(0266c7db) SHA1(a8f21e86e6d974c9bfd92a147689d0e7316d66e2) ) /* sprites lookup */

	ROM_REGION( 0x2000, REGION_SOUND1, 0 ) /* speech rom */
	ROM_LOAD( "507p01.8c",    0x0000, 0x2000, CRC(4a1da0b7) SHA1(e18987f0f7fa8740d5f91d1701ee11612c94e8e8) )
ROM_END


static DRIVER_INIT( jailbrek )
{
	konami1_decode();
}


GAME( 1986, jailbrek, 0,        jailbrek, jailbrek, jailbrek, ROT0, "Konami", "Jail Break" )
GAME( 1986, manhatan, jailbrek, jailbrek, jailbrek, jailbrek, ROT0, "Konami", "Manhattan 24 Bunsyo (Japan)" )
