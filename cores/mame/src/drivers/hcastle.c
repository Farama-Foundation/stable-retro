/***************************************************************************

	Haunted Castle

	Emulation by Bryan McPhail, mish@tendril.co.uk

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/konami/konami.h"
#include "cpu/z80/z80.h"

PALETTE_INIT( hcastle );
VIDEO_UPDATE( hcastle );
VIDEO_START( hcastle );

extern data8_t *hcastle_pf1_videoram,*hcastle_pf2_videoram;

WRITE_HANDLER( hcastle_pf1_video_w );
WRITE_HANDLER( hcastle_pf2_video_w );
READ_HANDLER( hcastle_gfxbank_r );
WRITE_HANDLER( hcastle_gfxbank_w );
WRITE_HANDLER( hcastle_pf1_control_w );
WRITE_HANDLER( hcastle_pf2_control_w );

static WRITE_HANDLER( hcastle_bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	int bankaddress;

	bankaddress = 0x10000 + (data & 0x1f) * 0x2000;
	cpu_setbank(1,&RAM[bankaddress]);
}

static WRITE_HANDLER( hcastle_soundirq_w )
{
	cpu_set_irq_line( 1, 0, HOLD_LINE );
}

static WRITE_HANDLER( hcastle_coin_w )
{
	coin_counter_w(0,data & 0x40);
	coin_counter_w(1,data & 0x80);
}

static READ_HANDLER( speedup_r )
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	int data = ( RAM[0x18dc] << 8 ) | RAM[0x18dd];

	if ( data < memory_region_length(REGION_CPU1) )
	{
		data = ( RAM[data] << 8 ) | RAM[data + 1];

		if ( data == 0xffff )
			cpu_spinuntil_int();
	}

	return RAM[0x18dc];
}



static MEMORY_READ_START( readmem )
	{ 0x0020, 0x003f, MRA_RAM },
	{ 0x0220, 0x023f, MRA_RAM },
	{ 0x0410, 0x0410, input_port_0_r },
	{ 0x0411, 0x0411, input_port_1_r },
	{ 0x0412, 0x0412, input_port_2_r },
	{ 0x0413, 0x0413, input_port_5_r }, /* Dip 3 */
	{ 0x0414, 0x0414, input_port_4_r }, /* Dip 2 */
	{ 0x0415, 0x0415, input_port_3_r }, /* Dip 1 */
	{ 0x0418, 0x0418, hcastle_gfxbank_r },
	{ 0x0600, 0x06ff, paletteram_r },
	{ 0x18dc, 0x18dc, speedup_r },
	{ 0x0700, 0x5fff, MRA_RAM },
	{ 0x6000, 0x7fff, MRA_BANK1 },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x0007, hcastle_pf1_control_w },
	{ 0x0020, 0x003f, MWA_RAM },	/* rowscroll? */
	{ 0x0200, 0x0207, hcastle_pf2_control_w },
	{ 0x0220, 0x023f, MWA_RAM },	/* rowscroll? */
	{ 0x0400, 0x0400, hcastle_bankswitch_w },
	{ 0x0404, 0x0404, soundlatch_w },
	{ 0x0408, 0x0408, hcastle_soundirq_w },
	{ 0x040c, 0x040c, watchdog_reset_w },
	{ 0x0410, 0x0410, hcastle_coin_w },
	{ 0x0418, 0x0418, hcastle_gfxbank_w },
	{ 0x0600, 0x06ff, paletteram_xBBBBBGGGGGRRRRR_swap_w, &paletteram },
	{ 0x0700, 0x1fff, MWA_RAM },
	{ 0x2000, 0x2fff, hcastle_pf1_video_w, &hcastle_pf1_videoram },
	{ 0x3000, 0x3fff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x4000, 0x4fff, hcastle_pf2_video_w, &hcastle_pf2_videoram },
	{ 0x5000, 0x5fff, MWA_RAM, &spriteram_2, &spriteram_2_size },
 	{ 0x6000, 0xffff, MWA_ROM },
MEMORY_END

/*****************************************************************************/

static WRITE_HANDLER( sound_bank_w )
{
	int bank_A=(data&0x3);
	int bank_B=((data>>2)&0x3);
	K007232_set_bank( 0, bank_A, bank_B );
}

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0xa000, 0xa000, YM3812_status_port_0_r },
	{ 0xb000, 0xb00d, K007232_read_port_0_r },
	{ 0xd000, 0xd000, soundlatch_r },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x9800, 0x987f, K051649_waveform_w },
	{ 0x9880, 0x9889, K051649_frequency_w },
	{ 0x988a, 0x988e, K051649_volume_w },
	{ 0x988f, 0x988f, K051649_keyonoff_w },
	{ 0xa000, 0xa000, YM3812_control_port_0_w },
	{ 0xa001, 0xa001, YM3812_write_port_0_w },
	{ 0xb000, 0xb00d, K007232_write_port_0_w },
	{ 0xc000, 0xc000, sound_bank_w }, /* 7232 bankswitch */
MEMORY_END

/*****************************************************************************/

INPUT_PORTS_START( hcastle )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Unused SW 1-0" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Unused SW 1-1" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x18, 0x10, DEF_STR( Difficulty ) )	/* "Difficulty 1"*/
	PORT_DIPSETTING(    0x18, "Easy" )				/* "Easy"*/
	PORT_DIPSETTING(    0x10, "Normal" )			/* "Nomal" !*/
	PORT_DIPSETTING(    0x08, "Hard" )				/* "Difficult"*/
	PORT_DIPSETTING(    0x00, "Hardest" )			/* "Very Difficult"*/
	PORT_DIPNAME( 0x60, 0x40, "Damage" )			/* "Difficulty 2"*/
	PORT_DIPSETTING(    0x60, "Small" )				/* "Strong"*/
	PORT_DIPSETTING(    0x40, "Normal" )			/* "Nomal" !*/
	PORT_DIPSETTING(    0x20, "Big" )				/* "Weak"*/
	PORT_DIPSETTING(    0x00, "Biggest" )			/* "Very Weak"*/
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
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
/*	PORT_DIPSETTING(    0x00, "Invalidity" )			*/ /* Disables the 2 coin slots*/

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Upright Controls" )
	PORT_DIPSETTING(    0x02, "Single" )
	PORT_DIPSETTING(    0x00, "Dual" )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Yes ) )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

/*****************************************************************************/

static struct GfxLayout charlayout =
{
	8,8,
	32768,
	4,
	{ 0, 1, 2, 3 },
	{ 2*4, 3*4, 0*4, 1*4, 6*4, 7*4, 4*4, 5*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,       0, 8*16 },	/* 007121 #0 */
	{ REGION_GFX2, 0, &charlayout, 8*16*16, 8*16 },	/* 007121 #1 */
	{ -1 } /* end of array */
};

/*****************************************************************************/

static void irqhandler(int linestate)
{
/*	cpu_set_irq_line(1,0,linestate);*/
}

static void volume_callback(int v)
{
	K007232_set_volume(0,0,(v >> 4) * 0x11,0);
	K007232_set_volume(0,1,0,(v & 0x0f) * 0x11);
}

static struct K007232_interface k007232_interface =
{
	1,		/* number of chips */
	3579545,	/* clock */
	{ REGION_SOUND1 },	/* memory regions */
	{ K007232_VOL(44,MIXER_PAN_CENTER,50,MIXER_PAN_CENTER) },	/* volume */
	{ volume_callback }	/* external port callback */
};

static struct YM3812interface ym3812_interface =
{
	1,
	3579545,
	{ 70 },
	{ irqhandler },
};

static struct k051649_interface k051649_interface =
{
	3579545/2,	/* Clock */
	45,			/* Volume */
};

static MACHINE_DRIVER_START( hcastle )

	/* basic machine hardware */
	MDRV_CPU_ADD(KONAMI, 3000000)	/* Derived from 24 MHz clock */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, 3579545)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(59)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)	/* frames per second verified by comparison with real board */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_BUFFERS_SPRITERAM)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(128)
	MDRV_COLORTABLE_LENGTH(2*8*16*16)

	MDRV_PALETTE_INIT(hcastle)
	MDRV_VIDEO_START(hcastle)
	MDRV_VIDEO_UPDATE(hcastle)

	/* sound hardware */
	MDRV_SOUND_ADD(K007232, k007232_interface)
	MDRV_SOUND_ADD(YM3812, ym3812_interface)
	MDRV_SOUND_ADD(K051649, k051649_interface)
MACHINE_DRIVER_END

/***************************************************************************/

ROM_START( hcastle )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )
	ROM_LOAD( "m03.k12",      0x08000, 0x08000, CRC(d85e743d) SHA1(314e2a2bbe650540306b85c8b89ec5bcaef11a0d) )
	ROM_LOAD( "b06.k8",       0x10000, 0x20000, CRC(abd07866) SHA1(a261d0cd90f5909abd06e8b691669e63d890c3be) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "768.e01",      0x00000, 0x08000, CRC(b9fff184) SHA1(c55f468c0da6afdaa2af65a111583c0c42868bd1) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "768c09.g21",   0x000000, 0x80000, CRC(e3be3fdd) SHA1(01a686af33a0a700066b1a5334d8552454ff186f) )
	ROM_LOAD( "768c08.g19",   0x080000, 0x80000, CRC(9633db8b) SHA1(fe1b117c2566288b88f000106c649c2fa5648ddc) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "768c04.j5",    0x000000, 0x80000, CRC(2960680e) SHA1(72e1f025496c907de8516e3b5f1781e73d5b2c6c) )
	ROM_LOAD( "768c05.j6",    0x080000, 0x80000, CRC(65a2f227) SHA1(43f368e533d6a164dc68d54130b81883e0d1bafe) )

	ROM_REGION( 0x0500, REGION_PROMS, 0 )
	ROM_LOAD( "768c13.j21",   0x0000, 0x0100, CRC(f5de80cb) SHA1(e8cc3e14a5d23b25fb7bf790e64786c6aa2df8b7) )	/* 007121 #0 sprite lookup table */
	ROM_LOAD( "768c14.j22",   0x0100, 0x0100, CRC(b32071b7) SHA1(09a699a3f20c155eae1e63429f03ed91abc54784) )	/* 007121 #0 char lookup table */
	ROM_LOAD( "768c11.i4",    0x0200, 0x0100, CRC(f5de80cb) SHA1(e8cc3e14a5d23b25fb7bf790e64786c6aa2df8b7) )	/* 007121 #1 sprite lookup table (same) */
	ROM_LOAD( "768c10.i3",    0x0300, 0x0100, CRC(b32071b7) SHA1(09a699a3f20c155eae1e63429f03ed91abc54784) )	/* 007121 #1 char lookup table (same) */
	ROM_LOAD( "768b12.d20",   0x0400, 0x0100, CRC(362544b8) SHA1(744c8d2ccfa980fc9a7354b4d241c569b3c1fffe) )	/* priority encoder (not used) */

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* 512k for the samples */
	ROM_LOAD( "768c07.e17",   0x00000, 0x80000, CRC(01f9889c) SHA1(01252d2ce7b14cfbe39ac8d7a5bd7417f1c2fc22) )
ROM_END

ROM_START( hcastleo )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )
	ROM_LOAD( "768.k03",      0x08000, 0x08000, CRC(40ce4f38) SHA1(1ab6d62a75c818b2ccbbb714373d6c7418500eb7) )
	ROM_LOAD( "768.g06",      0x10000, 0x20000, CRC(cdade920) SHA1(e15b7458ded4e4c811a737575ec3f16e5eec4121) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "768.e01",      0x00000, 0x08000, CRC(b9fff184) SHA1(c55f468c0da6afdaa2af65a111583c0c42868bd1) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "768c09.g21",   0x000000, 0x80000, CRC(e3be3fdd) SHA1(01a686af33a0a700066b1a5334d8552454ff186f) )
	ROM_LOAD( "768c08.g19",   0x080000, 0x80000, CRC(9633db8b) SHA1(fe1b117c2566288b88f000106c649c2fa5648ddc) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "768c04.j5",    0x000000, 0x80000, CRC(2960680e) SHA1(72e1f025496c907de8516e3b5f1781e73d5b2c6c) )
	ROM_LOAD( "768c05.j6",    0x080000, 0x80000, CRC(65a2f227) SHA1(43f368e533d6a164dc68d54130b81883e0d1bafe) )

	ROM_REGION( 0x0500, REGION_PROMS, 0 )
	ROM_LOAD( "768c13.j21",   0x0000, 0x0100, CRC(f5de80cb) SHA1(e8cc3e14a5d23b25fb7bf790e64786c6aa2df8b7) )	/* 007121 #0 sprite lookup table */
	ROM_LOAD( "768c14.j22",   0x0100, 0x0100, CRC(b32071b7) SHA1(09a699a3f20c155eae1e63429f03ed91abc54784) )	/* 007121 #0 char lookup table */
	ROM_LOAD( "768c11.i4",    0x0200, 0x0100, CRC(f5de80cb) SHA1(e8cc3e14a5d23b25fb7bf790e64786c6aa2df8b7) )	/* 007121 #1 sprite lookup table (same) */
	ROM_LOAD( "768c10.i3",    0x0300, 0x0100, CRC(b32071b7) SHA1(09a699a3f20c155eae1e63429f03ed91abc54784) )	/* 007121 #1 char lookup table (same) */
	ROM_LOAD( "768b12.d20",   0x0400, 0x0100, CRC(362544b8) SHA1(744c8d2ccfa980fc9a7354b4d241c569b3c1fffe) )	/* priority encoder (not used) */

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* 512k for the samples */
	ROM_LOAD( "768c07.e17",   0x00000, 0x80000, CRC(01f9889c) SHA1(01252d2ce7b14cfbe39ac8d7a5bd7417f1c2fc22) )
ROM_END

ROM_START( hcastlej )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )
	ROM_LOAD( "768p03.k12",0x08000, 0x08000, CRC(d509e340) SHA1(3a8078bd89a80ab9529e4ee8658fcafb8dd65258) )
	ROM_LOAD( "768j06.k8", 0x10000, 0x20000, CRC(42283c3e) SHA1(565a2eb607e262484f48919536c045d515cff89f) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "768.e01",   0x00000, 0x08000, CRC(b9fff184) SHA1(c55f468c0da6afdaa2af65a111583c0c42868bd1) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "768c09.g21",   0x000000, 0x80000, CRC(e3be3fdd) SHA1(01a686af33a0a700066b1a5334d8552454ff186f) )
	ROM_LOAD( "768c08.g19",   0x080000, 0x80000, CRC(9633db8b) SHA1(fe1b117c2566288b88f000106c649c2fa5648ddc) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "768c04.j5",    0x000000, 0x80000, CRC(2960680e) SHA1(72e1f025496c907de8516e3b5f1781e73d5b2c6c) )
	ROM_LOAD( "768c05.j6",    0x080000, 0x80000, CRC(65a2f227) SHA1(43f368e533d6a164dc68d54130b81883e0d1bafe) )

	ROM_REGION( 0x0500, REGION_PROMS, 0 )
	ROM_LOAD( "768c13.j21",   0x0000, 0x0100, CRC(f5de80cb) SHA1(e8cc3e14a5d23b25fb7bf790e64786c6aa2df8b7) )	/* 007121 #0 sprite lookup table */
	ROM_LOAD( "768c14.j22",   0x0100, 0x0100, CRC(b32071b7) SHA1(09a699a3f20c155eae1e63429f03ed91abc54784) )	/* 007121 #0 char lookup table */
	ROM_LOAD( "768c11.i4",    0x0200, 0x0100, CRC(f5de80cb) SHA1(e8cc3e14a5d23b25fb7bf790e64786c6aa2df8b7) )	/* 007121 #1 sprite lookup table (same) */
	ROM_LOAD( "768c10.i3",    0x0300, 0x0100, CRC(b32071b7) SHA1(09a699a3f20c155eae1e63429f03ed91abc54784) )	/* 007121 #1 char lookup table (same) */
	ROM_LOAD( "768b12.d20",   0x0400, 0x0100, CRC(362544b8) SHA1(744c8d2ccfa980fc9a7354b4d241c569b3c1fffe) )	/* priority encoder (not used) */

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* 512k for the samples */
	ROM_LOAD( "768c07.e17",   0x00000, 0x80000, CRC(01f9889c) SHA1(01252d2ce7b14cfbe39ac8d7a5bd7417f1c2fc22) )
ROM_END

ROM_START( hcastljo )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )
	ROM_LOAD( "768n03.k12",0x08000, 0x08000, CRC(3e4dca2a) SHA1(cd70fdc42b970b89ae16ab6c81d1a5003fa53dbd) )
	ROM_LOAD( "768j06.k8", 0x10000, 0x20000, CRC(42283c3e) SHA1(565a2eb607e262484f48919536c045d515cff89f) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "768.e01",   0x00000, 0x08000, CRC(b9fff184) SHA1(c55f468c0da6afdaa2af65a111583c0c42868bd1) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "768c09.g21",   0x000000, 0x80000, CRC(e3be3fdd) SHA1(01a686af33a0a700066b1a5334d8552454ff186f) )
	ROM_LOAD( "768c08.g19",   0x080000, 0x80000, CRC(9633db8b) SHA1(fe1b117c2566288b88f000106c649c2fa5648ddc) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "768c04.j5",    0x000000, 0x80000, CRC(2960680e) SHA1(72e1f025496c907de8516e3b5f1781e73d5b2c6c) )
	ROM_LOAD( "768c05.j6",    0x080000, 0x80000, CRC(65a2f227) SHA1(43f368e533d6a164dc68d54130b81883e0d1bafe) )

	ROM_REGION( 0x0500, REGION_PROMS, 0 )
	ROM_LOAD( "768c13.j21",   0x0000, 0x0100, CRC(f5de80cb) SHA1(e8cc3e14a5d23b25fb7bf790e64786c6aa2df8b7) )	/* 007121 #0 sprite lookup table */
	ROM_LOAD( "768c14.j22",   0x0100, 0x0100, CRC(b32071b7) SHA1(09a699a3f20c155eae1e63429f03ed91abc54784) )	/* 007121 #0 char lookup table */
	ROM_LOAD( "768c11.i4",    0x0200, 0x0100, CRC(f5de80cb) SHA1(e8cc3e14a5d23b25fb7bf790e64786c6aa2df8b7) )	/* 007121 #1 sprite lookup table (same) */
	ROM_LOAD( "768c10.i3",    0x0300, 0x0100, CRC(b32071b7) SHA1(09a699a3f20c155eae1e63429f03ed91abc54784) )	/* 007121 #1 char lookup table (same) */
	ROM_LOAD( "768b12.d20",   0x0400, 0x0100, CRC(362544b8) SHA1(744c8d2ccfa980fc9a7354b4d241c569b3c1fffe) )	/* priority encoder (not used) */

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* 512k for the samples */
	ROM_LOAD( "768c07.e17",   0x00000, 0x80000, CRC(01f9889c) SHA1(01252d2ce7b14cfbe39ac8d7a5bd7417f1c2fc22) )
ROM_END



GAME( 1988, hcastle,  0,       hcastle, hcastle, 0, ROT0, "Konami", "Haunted Castle (version M)" )
GAME( 1988, hcastleo, hcastle, hcastle, hcastle, 0, ROT0, "Konami", "Haunted Castle (version K)" )
GAME( 1988, hcastlej, hcastle, hcastle, hcastle, 0, ROT0, "Konami", "Akuma-Jou Dracula (Japan version P)" )
GAME( 1988, hcastljo, hcastle, hcastle, hcastle, 0, ROT0, "Konami", "Akuma-Jou Dracula (Japan version N)" )
