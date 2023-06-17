/***************************************************************************

	Dark Seal (Rev 3)    (c) 1990 Data East Corporation (World version)
	Dark Seal (Rev 1)    (c) 1990 Data East Corporation (World version)
	Dark Seal            (c) 1990 Data East Corporation (Japanese version)
	Gate Of Doom (Rev 4) (c) 1990 Data East Corporation (USA version)
	Gate of Doom (Rev 1) (c) 1990 Data East Corporation (USA version)

	Emulation by Bryan McPhail, mish@tendril.co.uk

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/h6280/h6280.h"

VIDEO_START( darkseal );
VIDEO_UPDATE( darkseal );

WRITE16_HANDLER( darkseal_pf1_data_w );
WRITE16_HANDLER( darkseal_pf2_data_w );
WRITE16_HANDLER( darkseal_pf3_data_w );
WRITE16_HANDLER( darkseal_pf3b_data_w );
WRITE16_HANDLER( darkseal_control_0_w );
WRITE16_HANDLER( darkseal_control_1_w );
WRITE16_HANDLER( darkseal_palette_24bit_rg_w );
WRITE16_HANDLER( darkseal_palette_24bit_b_w );
extern data16_t *darkseal_pf12_row, *darkseal_pf34_row;
extern data16_t *darkseal_pf1_data,*darkseal_pf2_data,*darkseal_pf3_data;
static data16_t *darkseal_ram;

/******************************************************************************/

static WRITE16_HANDLER( darkseal_control_w )
{
	switch (offset<<1) {
    case 6: /* DMA flag */
		buffer_spriteram16_w(0,0,0);
		return;
    case 8: /* Sound CPU write */
		soundlatch_w(0,data & 0xff);
		cpu_set_irq_line(1,0,HOLD_LINE);
    	return;
  	case 0xa: /* IRQ Ack (VBL) */
		return;
	}
}

static READ16_HANDLER( darkseal_control_r )
{
	switch (offset<<1)
	{
		case 0: /* Dip Switches */
			return (readinputport(3) + (readinputport(4) << 8));

		case 2: /* Player 1 & Player 2 joysticks & fire buttons */
			return (readinputport(0) + (readinputport(1) << 8));

		case 4: /* Credits */
			return readinputport(2);
	}

	return ~0;
}

/******************************************************************************/

static MEMORY_READ16_START( darkseal_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x100000, 0x103fff, MRA16_RAM },
	{ 0x120000, 0x1207ff, MRA16_RAM },
	{ 0x140000, 0x140fff, MRA16_RAM },
	{ 0x141000, 0x141fff, MRA16_RAM },
	{ 0x180000, 0x18000f, darkseal_control_r },
	{ 0x220000, 0x220fff, MRA16_RAM },
	{ 0x222000, 0x222fff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( darkseal_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x100000, 0x103fff, MWA16_RAM, &darkseal_ram },
	{ 0x120000, 0x1207ff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x140000, 0x140fff, darkseal_palette_24bit_rg_w, &paletteram16 },
	{ 0x141000, 0x141fff, darkseal_palette_24bit_b_w, &paletteram16_2 },
	{ 0x180000, 0x18000f, darkseal_control_w },
 	{ 0x200000, 0x200fff, darkseal_pf3b_data_w }, /* 2nd half of pf3, only used on last level */
	{ 0x202000, 0x203fff, darkseal_pf3_data_w, &darkseal_pf3_data },
	{ 0x220000, 0x220fff, MWA16_RAM, &darkseal_pf12_row },
	{ 0x222000, 0x222fff, MWA16_RAM, &darkseal_pf34_row },
	{ 0x240000, 0x24000f, darkseal_control_0_w },
	{ 0x260000, 0x261fff, darkseal_pf2_data_w, &darkseal_pf2_data },
	{ 0x262000, 0x263fff, darkseal_pf1_data_w, &darkseal_pf1_data },
	{ 0x2a0000, 0x2a000f, darkseal_control_1_w },
MEMORY_END

/******************************************************************************/

static WRITE_HANDLER( YM2151_w )
{
	switch (offset) {
	case 0:
		YM2151_register_port_0_w(0,data);
		break;
	case 1:
		YM2151_data_port_0_w(0,data);
		break;
	}
}

static WRITE_HANDLER( YM2203_w )
{
	switch (offset) {
	case 0:
		YM2203_control_port_0_w(0,data);
		break;
	case 1:
		YM2203_write_port_0_w(0,data);
		break;
	}
}

static MEMORY_READ_START( sound_readmem )
	{ 0x000000, 0x00ffff, MRA_ROM },
	{ 0x100000, 0x100001, YM2203_status_port_0_r },
	{ 0x110000, 0x110001, YM2151_status_port_0_r },
	{ 0x120000, 0x120001, OKIM6295_status_0_r },
	{ 0x130000, 0x130001, OKIM6295_status_1_r },
	{ 0x140000, 0x140001, soundlatch_r },
	{ 0x1f0000, 0x1f1fff, MRA_BANK8 },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x000000, 0x00ffff, MWA_ROM },
	{ 0x100000, 0x100001, YM2203_w },
	{ 0x110000, 0x110001, YM2151_w },
	{ 0x120000, 0x120001, OKIM6295_data_0_w },
	{ 0x130000, 0x130001, OKIM6295_data_1_w },
	{ 0x1f0000, 0x1f1fff, MWA_BANK8 },
	{ 0x1fec00, 0x1fec01, H6280_timer_w },
	{ 0x1ff402, 0x1ff403, H6280_irq_status_w },
MEMORY_END

/******************************************************************************/

INPUT_PORTS_START( darkseal )
	PORT_START	/* Player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )	/* button 3 - unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )	/* button 3 - unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* Credits */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Dip switch bank 1 */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x01, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, "Easy" )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x04, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x30, 0x30, "Energy" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x10, "2.5" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

/******************************************************************************/

static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 chars */
	4096,
	4,		/* 4 bits per pixel  */
	{ 0x00000*8, 0x10000*8, 0x8000*8, 0x18000*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout seallayout =
{
	16,16,
	4096,
	4,
	{ 8, 0,  0x40000*8+8, 0x40000*8,},
	{ 32*8+0, 32*8+1, 32*8+2, 32*8+3, 32*8+4, 32*8+5, 32*8+6, 32*8+7,
		0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8
};

static struct GfxLayout seallayout2 =
{
	16,16,
	4096*2,
	4,
	{ 8, 0, 0x80000*8+8, 0x80000*8 },
	{ 32*8+0, 32*8+1, 32*8+2, 32*8+3, 32*8+4, 32*8+5, 32*8+6, 32*8+7,
		0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,    0, 16 },	/* Characters 8x8 */
	{ REGION_GFX2, 0, &seallayout,  768, 16 },	/* Tiles 16x16 */
	{ REGION_GFX3, 0, &seallayout, 1024, 16 },	/* Tiles 16x16 */
	{ REGION_GFX4, 0, &seallayout2, 256, 32 },	/* Sprites 16x16 */
	{ -1 } /* end of array */
};

/******************************************************************************/

static struct OKIM6295interface okim6295_interface =
{
	2,              /* 2 chips */
	{ 32220000/32/132, 32220000/16/132 },/* Frequency */
	{ REGION_SOUND1, REGION_SOUND2 },
	{ 75, 60 } /* Note!  Keep chip 1 (voices) louder than chip 2 */
};

static struct YM2203interface ym2203_interface =
{
	1,
	32220000/8, /* Accurate, audio section crystal is 32.220 MHz */
	{ YM2203_VOL(60,60) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static void sound_irq(int state)
{
	cpu_set_irq_line(1,1,state); /* IRQ 2 */
}

static struct YM2151interface ym2151_interface =
{
	1,
	32220000/9, /* Accurate, audio section crystal is 32.220 MHz */
	{ YM3012_VOL(45,MIXER_PAN_LEFT,45,MIXER_PAN_RIGHT) },
	{ sound_irq }
};

static MACHINE_DRIVER_START( darkseal )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,12000000) /* Custom chip 59 */
	MDRV_CPU_MEMORY(darkseal_readmem,darkseal_writemem)
	MDRV_CPU_VBLANK_INT(irq6_line_hold,1)/* VBL */

	MDRV_CPU_ADD(H6280, 32220000/8) /* Custom chip 45, Audio section crystal is 32.220 MHz */
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(58)
	MDRV_VBLANK_DURATION(529)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_BUFFERS_SPRITERAM)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(darkseal)
	MDRV_VIDEO_UPDATE(darkseal)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, ym2203_interface)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END

/******************************************************************************/

ROM_START( darkseal )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "ga04-3.rom",   0x00000, 0x20000, CRC(bafad556) SHA1(5bd8a787d41a33919701ced29212bc11301e31d9) )
	ROM_LOAD16_BYTE( "ga01-3.rom",   0x00001, 0x20000, CRC(f409050e) SHA1(4653094aeb5dd7ba1e490c04897a23ba8990df3c) )
	ROM_LOAD16_BYTE( "ga-00.rom",    0x40000, 0x20000, CRC(fbf3ac63) SHA1(51af581ee951eedeb4aa413ecbebe8bf4d30613b) )
	ROM_LOAD16_BYTE( "ga-05.rom",    0x40001, 0x20000, CRC(d5e3ae3f) SHA1(12f6e92af115422c6ab6ef1d33675d1e1cd58e10) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* Sound CPU */
	ROM_LOAD( "fz-06.rom",    0x00000, 0x10000, CRC(c4828a6d) SHA1(fbfd0c85730bbe18401879cd68c19aaec9d482d8) )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "fz-02.rom",    0x000000, 0x10000, CRC(3c9c3012) SHA1(086c2123725d4aa32838c0b6c82317d9c789c465) )	/* chars */
	ROM_LOAD( "fz-03.rom",    0x010000, 0x10000, CRC(264b90ed) SHA1(0bb1557673107c2d732a9374d5601a6eaf229473) )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mac-03.rom",   0x000000, 0x80000, CRC(9996f3dc) SHA1(fffd9ecfe142a0c7c3c9c521778ff9c55ea8b225) ) /* tiles 1 */

	ROM_REGION( 0x080000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "mac-02.rom",   0x000000, 0x80000, CRC(49504e89) SHA1(6da4733a650b9040abb2a81a49476368b514b5ab) ) /* tiles 2 */

	ROM_REGION( 0x100000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "mac-00.rom",   0x000000, 0x80000, CRC(52acf1d6) SHA1(a7b68782417baafc86371b106fd31c5317f5b3d8) ) /* sprites */
	ROM_LOAD( "mac-01.rom",   0x080000, 0x80000, CRC(b28f7584) SHA1(e02ddd45130a7b50f80b6dd049059dba8071d768) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "fz-08.rom",    0x00000, 0x20000, CRC(c9bf68e1) SHA1(c81e2534a814fe44c8787946a9fbe18f1743c3b4) )

	ROM_REGION( 0x20000, REGION_SOUND2, 0 )	/* ADPCM samples */
	ROM_LOAD( "fz-07.rom",    0x00000, 0x20000, CRC(588dd3cb) SHA1(16c4e7670a4967768ddbfd52939d4e6e42268441) )
ROM_END

ROM_START( darksea1 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "ga-04.rom",    0x00000, 0x20000, CRC(a1a985a9) SHA1(eac3f43ff4016dcc21fe34b6bfed36e0d4b86959) )
	ROM_LOAD16_BYTE( "ga-01.rom",    0x00001, 0x20000, CRC(98bd2940) SHA1(88ac727c3797e646834266320a71aa159e2b2541) )
	ROM_LOAD16_BYTE( "ga-00.rom",    0x40000, 0x20000, CRC(fbf3ac63) SHA1(51af581ee951eedeb4aa413ecbebe8bf4d30613b) )
	ROM_LOAD16_BYTE( "ga-05.rom",    0x40001, 0x20000, CRC(d5e3ae3f) SHA1(12f6e92af115422c6ab6ef1d33675d1e1cd58e10) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* Sound CPU */
	ROM_LOAD( "fz-06.rom",    0x00000, 0x10000, CRC(c4828a6d) SHA1(fbfd0c85730bbe18401879cd68c19aaec9d482d8) )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "fz-02.rom",    0x000000, 0x10000, CRC(3c9c3012) SHA1(086c2123725d4aa32838c0b6c82317d9c789c465) )	/* chars */
	ROM_LOAD( "fz-03.rom",    0x010000, 0x10000, CRC(264b90ed) SHA1(0bb1557673107c2d732a9374d5601a6eaf229473) )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mac-03.rom",   0x000000, 0x80000, CRC(9996f3dc) SHA1(fffd9ecfe142a0c7c3c9c521778ff9c55ea8b225) ) /* tiles 1 */

	ROM_REGION( 0x080000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "mac-02.rom",   0x000000, 0x80000, CRC(49504e89) SHA1(6da4733a650b9040abb2a81a49476368b514b5ab) ) /* tiles 2 */

	ROM_REGION( 0x100000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "mac-00.rom",   0x000000, 0x80000, CRC(52acf1d6) SHA1(a7b68782417baafc86371b106fd31c5317f5b3d8) ) /* sprites */
	ROM_LOAD( "mac-01.rom",   0x080000, 0x80000, CRC(b28f7584) SHA1(e02ddd45130a7b50f80b6dd049059dba8071d768) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "fz-08.rom",    0x00000, 0x20000, CRC(c9bf68e1) SHA1(c81e2534a814fe44c8787946a9fbe18f1743c3b4) )

	ROM_REGION( 0x20000, REGION_SOUND2, 0 )	/* ADPCM samples */
	ROM_LOAD( "fz-07.rom",    0x00000, 0x20000, CRC(588dd3cb) SHA1(16c4e7670a4967768ddbfd52939d4e6e42268441) )
ROM_END

ROM_START( darkseaj )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "fz-04.bin",    0x00000, 0x20000, CRC(817faa2c) SHA1(8a79703f0e3aeb2ceeb098466561ab604baef301) )
	ROM_LOAD16_BYTE( "fz-01.bin",    0x00001, 0x20000, CRC(373caeee) SHA1(5cfa0c7672c439e9d011d9ec93da32c2377dce19) )
	ROM_LOAD16_BYTE( "fz-00.bin",    0x40000, 0x20000, CRC(1ab99aa7) SHA1(1da51f3ee0d15094911d4090264b945090d51242) )
	ROM_LOAD16_BYTE( "fz-05.bin",    0x40001, 0x20000, CRC(3374ef8c) SHA1(4144e71e452e281078bcd9b9a996db9f5dccc346) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* Sound CPU */
	ROM_LOAD( "fz-06.rom",    0x00000, 0x10000, CRC(c4828a6d) SHA1(fbfd0c85730bbe18401879cd68c19aaec9d482d8) )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "fz-02.rom",    0x000000, 0x10000, CRC(3c9c3012) SHA1(086c2123725d4aa32838c0b6c82317d9c789c465) )	/* chars */
	ROM_LOAD( "fz-03.rom",    0x010000, 0x10000, CRC(264b90ed) SHA1(0bb1557673107c2d732a9374d5601a6eaf229473) )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mac-03.rom",   0x000000, 0x80000, CRC(9996f3dc) SHA1(fffd9ecfe142a0c7c3c9c521778ff9c55ea8b225) ) /* tiles 1 */

	ROM_REGION( 0x080000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "mac-02.rom",   0x000000, 0x80000, CRC(49504e89) SHA1(6da4733a650b9040abb2a81a49476368b514b5ab) ) /* tiles 2 */

	ROM_REGION( 0x100000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "mac-00.rom",   0x000000, 0x80000, CRC(52acf1d6) SHA1(a7b68782417baafc86371b106fd31c5317f5b3d8) ) /* sprites */
	ROM_LOAD( "mac-01.rom",   0x080000, 0x80000, CRC(b28f7584) SHA1(e02ddd45130a7b50f80b6dd049059dba8071d768) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "fz-08.rom",    0x00000, 0x20000, CRC(c9bf68e1) SHA1(c81e2534a814fe44c8787946a9fbe18f1743c3b4) )

	ROM_REGION( 0x20000, REGION_SOUND2, 0 )	/* ADPCM samples */
	ROM_LOAD( "fz-07.rom",    0x00000, 0x20000, CRC(588dd3cb) SHA1(16c4e7670a4967768ddbfd52939d4e6e42268441) )
ROM_END

ROM_START( gatedoom )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "gb04-4",       0x00000, 0x20000, CRC(8e3a0bfd) SHA1(1d20bd678a630e2006c7f50f4d13656136db3721) )
	ROM_LOAD16_BYTE( "gb01-4",       0x00001, 0x20000, CRC(8d0fd383) SHA1(797e3cf43c9315b4195232eb1787a2292af4901b) )
	ROM_LOAD16_BYTE( "ga-00.rom",    0x40000, 0x20000, CRC(fbf3ac63) SHA1(51af581ee951eedeb4aa413ecbebe8bf4d30613b) )
	ROM_LOAD16_BYTE( "ga-05.rom",    0x40001, 0x20000, CRC(d5e3ae3f) SHA1(12f6e92af115422c6ab6ef1d33675d1e1cd58e10) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* Sound CPU */
	ROM_LOAD( "fz-06.rom",    0x00000, 0x10000, CRC(c4828a6d) SHA1(fbfd0c85730bbe18401879cd68c19aaec9d482d8) )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "fz-02.rom",    0x000000, 0x10000, CRC(3c9c3012) SHA1(086c2123725d4aa32838c0b6c82317d9c789c465) )	/* chars */
	ROM_LOAD( "fz-03.rom",    0x010000, 0x10000, CRC(264b90ed) SHA1(0bb1557673107c2d732a9374d5601a6eaf229473) )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mac-03.rom",   0x000000, 0x80000, CRC(9996f3dc) SHA1(fffd9ecfe142a0c7c3c9c521778ff9c55ea8b225) ) /* tiles 1 */

	ROM_REGION( 0x080000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "mac-02.rom",   0x000000, 0x80000, CRC(49504e89) SHA1(6da4733a650b9040abb2a81a49476368b514b5ab) ) /* tiles 2 */

	ROM_REGION( 0x100000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "mac-00.rom",   0x000000, 0x80000, CRC(52acf1d6) SHA1(a7b68782417baafc86371b106fd31c5317f5b3d8) ) /* sprites */
	ROM_LOAD( "mac-01.rom",   0x080000, 0x80000, CRC(b28f7584) SHA1(e02ddd45130a7b50f80b6dd049059dba8071d768) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "fz-08.rom",    0x00000, 0x20000, CRC(c9bf68e1) SHA1(c81e2534a814fe44c8787946a9fbe18f1743c3b4) )

	ROM_REGION( 0x20000, REGION_SOUND2, 0 )	/* ADPCM samples */
	ROM_LOAD( "fz-07.rom",    0x00000, 0x20000, CRC(588dd3cb) SHA1(16c4e7670a4967768ddbfd52939d4e6e42268441) )
ROM_END

ROM_START( gatedom1 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "gb04.bin",     0x00000, 0x20000, CRC(4c3bbd2b) SHA1(e74a532edd01a559d0c388b37a2ead98c19fe5de) )
	ROM_LOAD16_BYTE( "gb01.bin",     0x00001, 0x20000, CRC(59e367f4) SHA1(f88fa23b8e91f312103eb8a1d9a91d8171ec3ad4) )
	ROM_LOAD16_BYTE( "gb00.bin",     0x40000, 0x20000, CRC(a88c16a1) SHA1(e02d5470692f23afa658b9bda933bb20be64602f) )
	ROM_LOAD16_BYTE( "gb05.bin",     0x40001, 0x20000, CRC(252d7e14) SHA1(b2f27cd9686dfc697f3faca74d20b298a59efab2) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* Sound CPU */
	ROM_LOAD( "fz-06.rom",    0x00000, 0x10000, CRC(c4828a6d) SHA1(fbfd0c85730bbe18401879cd68c19aaec9d482d8) )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "fz-02.rom",    0x000000, 0x10000, CRC(3c9c3012) SHA1(086c2123725d4aa32838c0b6c82317d9c789c465) )	/* chars */
	ROM_LOAD( "fz-03.rom",    0x010000, 0x10000, CRC(264b90ed) SHA1(0bb1557673107c2d732a9374d5601a6eaf229473) )

  	/* the following four have not been verified on a real Gate of Doom */
	/* board - might be different from Dark Seal! */

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mac-03.rom",   0x000000, 0x80000, CRC(9996f3dc) SHA1(fffd9ecfe142a0c7c3c9c521778ff9c55ea8b225) ) /* tiles 1 */

	ROM_REGION( 0x080000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "mac-02.rom",   0x000000, 0x80000, CRC(49504e89) SHA1(6da4733a650b9040abb2a81a49476368b514b5ab) ) /* tiles 2 */

	ROM_REGION( 0x100000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "mac-00.rom",   0x000000, 0x80000, CRC(52acf1d6) SHA1(a7b68782417baafc86371b106fd31c5317f5b3d8) ) /* sprites */
	ROM_LOAD( "mac-01.rom",   0x080000, 0x80000, CRC(b28f7584) SHA1(e02ddd45130a7b50f80b6dd049059dba8071d768) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "fz-08.rom",    0x00000, 0x20000, CRC(c9bf68e1) SHA1(c81e2534a814fe44c8787946a9fbe18f1743c3b4) )

	ROM_REGION( 0x20000, REGION_SOUND2, 0 )	/* ADPCM samples */
	ROM_LOAD( "fz-07.rom",    0x00000, 0x20000, CRC(588dd3cb) SHA1(16c4e7670a4967768ddbfd52939d4e6e42268441) )
ROM_END

/******************************************************************************/

static DRIVER_INIT( darkseal )
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	int i;

	for (i=0x00000; i<0x80000; i++)
		RAM[i]=(RAM[i] & 0xbd) | ((RAM[i] & 0x02) << 5) | ((RAM[i] & 0x40) >> 5);

}

/******************************************************************************/

GAME( 1990, darkseal, 0,        darkseal, darkseal, darkseal, ROT0, "Data East Corporation", "Dark Seal (World revision 3)" )
GAME( 1990, darksea1, darkseal, darkseal, darkseal, darkseal, ROT0, "Data East Corporation", "Dark Seal (World revision 1)" )
GAME( 1990, darkseaj, darkseal, darkseal, darkseal, darkseal, ROT0, "Data East Corporation", "Dark Seal (Japan)" )
GAME( 1990, gatedoom, darkseal, darkseal, darkseal, darkseal, ROT0, "Data East Corporation", "Gate of Doom (US revision 4)" )
GAME( 1990, gatedom1, darkseal, darkseal, darkseal, darkseal, ROT0, "Data East Corporation", "Gate of Doom (US revision 1)" )
