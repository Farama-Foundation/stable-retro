/***************************************************************************

Block Out

driver by Nicola Salmoria

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"



extern data16_t *blockout_videoram;
extern data16_t *blockout_frontvideoram;
extern unsigned char *blockout_frontcolor;

WRITE16_HANDLER( blockout_videoram_w );
WRITE16_HANDLER( blockout_paletteram_w );
WRITE16_HANDLER( blockout_frontcolor_w );
VIDEO_START( blockout );
VIDEO_UPDATE( blockout );


static INTERRUPT_GEN( blockout_interrupt )
{
	/* interrupt 6 is vblank */
	/* interrupt 5 reads coin inputs - might have to be triggered only */
	/* when a coin is inserted */
	cpu_set_irq_line(0, 6 - cpu_getiloops(), HOLD_LINE);
}

static WRITE16_HANDLER( blockout_sound_command_w )
{
	if (ACCESSING_LSB)
	{
		soundlatch_w(offset,data & 0xff);
		cpu_set_irq_line(1, IRQ_LINE_NMI, PULSE_LINE);
	}
}


static MEMORY_READ16_START( readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x100000, 0x100001, input_port_0_word_r },
	{ 0x100002, 0x100003, input_port_1_word_r },
	{ 0x100004, 0x100005, input_port_2_word_r },
	{ 0x100006, 0x100007, input_port_3_word_r },
	{ 0x100008, 0x100009, input_port_4_word_r },
	{ 0x180000, 0x1bffff, MRA16_RAM },
	{ 0x1d4000, 0x1dffff, MRA16_RAM },
	{ 0x1f4000, 0x1fffff, MRA16_RAM },
	{ 0x200000, 0x207fff, MRA16_RAM },
	{ 0x208000, 0x21ffff, MRA16_RAM },
	{ 0x280200, 0x2805ff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x100014, 0x100015, blockout_sound_command_w },
	{ 0x100016, 0x100017, MWA16_NOP },	/* don't know, maybe reset sound CPU */
	{ 0x180000, 0x1bffff, blockout_videoram_w, &blockout_videoram },
	{ 0x1d4000, 0x1dffff, MWA16_RAM },	/* work RAM */
	{ 0x1f4000, 0x1fffff, MWA16_RAM },	/* work RAM */
	{ 0x200000, 0x207fff, MWA16_RAM, &blockout_frontvideoram },
	{ 0x208000, 0x21ffff, MWA16_RAM },	/* ??? */
	{ 0x280002, 0x280003, blockout_frontcolor_w },
	{ 0x280200, 0x2805ff, blockout_paletteram_w, &paletteram16 },
MEMORY_END

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0x8801, 0x8801, YM2151_status_port_0_r },
	{ 0x9800, 0x9800, OKIM6295_status_0_r },
	{ 0xa000, 0xa000, soundlatch_r },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x8800, 0x8800, YM2151_register_port_0_w },
	{ 0x8801, 0x8801, YM2151_data_port_0_w },
	{ 0x9800, 0x9800, OKIM6295_data_0_w },
MEMORY_END


INPUT_PORTS_START( blockout )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN3 )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	/* the following two are supposed to control Coin 2, but they don't work. */
	/* This happens on the original board too. */
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )	/* unused? */
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )	/* unused? */
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "1 Coin to Continue" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )	/* unused? */
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )	/* unused? */
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Very Hard" )
	PORT_DIPNAME( 0x04, 0x04, "Rotate Buttons" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
INPUT_PORTS_END

INPUT_PORTS_START( blckoutj )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN3 )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	/* the following two are supposed to control Coin 2, but they don't work. */
	/* This happens on the original board too. */
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )	/* unused? */
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )	/* unused? */
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "1 Coin to Continue" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )	/* unused? */
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )	/* unused? */
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Very Hard" )
	/* The following switch is 2/3 rotate buttons on the other sets, option doesn't exist in the Japan version */
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	/* these can still be used on the difficutly select even if they can't be used for rotating pieces in this version */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
INPUT_PORTS_END


/* handler called by the 2151 emulator when the internal timers cause an IRQ */
static void blockout_irq_handler(int irq)
{
	cpu_set_irq_line_and_vector(1,0,irq ? ASSERT_LINE : CLEAR_LINE,0xff);
}

static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	3579545,	/* 3.579545 MHz (?) */
	{ YM3012_VOL(60,MIXER_PAN_LEFT,60,MIXER_PAN_RIGHT) },
	{ blockout_irq_handler }
};

static struct OKIM6295interface okim6295_interface =
{
	1,                  /* 1 chip */
	{ 8000 },           /* 8000Hz frequency */
	{ REGION_SOUND1 },	/* memory region */
	{ 50 }
};



static MACHINE_DRIVER_START( blockout )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 8760000)       /* MRH - 8.76 makes gfx/adpcm samples sync better */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(blockout_interrupt,2)

	MDRV_CPU_ADD(Z80, 3579545)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 3.579545 MHz (?) */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 256)
	MDRV_VISIBLE_AREA(0, 319, 8, 247)
	MDRV_PALETTE_LENGTH(513)

	MDRV_VIDEO_START(blockout)
	MDRV_VIDEO_UPDATE(blockout)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( blockout )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 2*128k for 68000 code */
	ROM_LOAD16_BYTE( "bo29a0-2.bin", 0x00000, 0x20000, CRC(b0103427) SHA1(53cac2adc04783abbde21e9f3c0e655f22f68f69) )
	ROM_LOAD16_BYTE( "bo29a1-2.bin", 0x00001, 0x20000, CRC(5984d5a2) SHA1(4b350856d0313d40eaa3d8a8d9e310f74bc20398) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "bo29e3-0.bin", 0x0000, 0x8000, CRC(3ea01f78) SHA1(5fc4ad4d9f03d7c26d2afc3e7ede75589e40b0d8) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* 128k for ADPCM samples - sound chip is OKIM6295 */
	ROM_LOAD( "bo29e2-0.bin", 0x0000, 0x20000, CRC(15c5a99d) SHA1(89091eda454a028fd1f17501584bd589baf6d523) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "mb7114h.25",   0x0000, 0x0100, CRC(b25bbda7) SHA1(840f1470886bd0019db3cd29e3d1d80205a65f48) )	/* unknown */
ROM_END

ROM_START( blckout2 )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 2*128k for 68000 code */
	ROM_LOAD16_BYTE( "29a0",         0x00000, 0x20000, CRC(605f931e) SHA1(65fa7227dafde1fc8564e09fa949fe575b394d8a) )
	ROM_LOAD16_BYTE( "29a1",         0x00001, 0x20000, CRC(38f07000) SHA1(e4070e3067d77cc1b0d8d0c63786f2729c5c703a) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "bo29e3-0.bin", 0x0000, 0x8000, CRC(3ea01f78) SHA1(5fc4ad4d9f03d7c26d2afc3e7ede75589e40b0d8) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* 128k for ADPCM samples - sound chip is OKIM6295 */
	ROM_LOAD( "bo29e2-0.bin", 0x0000, 0x20000, CRC(15c5a99d) SHA1(89091eda454a028fd1f17501584bd589baf6d523) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "mb7114h.25",   0x0000, 0x0100, CRC(b25bbda7) SHA1(840f1470886bd0019db3cd29e3d1d80205a65f48) )	/* unknown */
ROM_END

ROM_START( blckoutj )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 2*128k for 68000 code */
	ROM_LOAD16_BYTE( "2.bin",         0x00000, 0x20000, CRC(e16cf065) SHA1(541b30b054cf08f10d6ca4746423759f4326c005) )
	ROM_LOAD16_BYTE( "1.bin",         0x00001, 0x20000, CRC(950b28a3) SHA1(7d1635ac2a3fc1efdd2f78cd6038bd7b4c907b1b) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "bo29e3-0.bin", 0x0000, 0x8000, CRC(3ea01f78) SHA1(5fc4ad4d9f03d7c26d2afc3e7ede75589e40b0d8) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* 128k for ADPCM samples - sound chip is OKIM6295 */
	ROM_LOAD( "bo29e2-0.bin", 0x0000, 0x20000, CRC(15c5a99d) SHA1(89091eda454a028fd1f17501584bd589baf6d523) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "mb7114h.25",   0x0000, 0x0100, CRC(b25bbda7) SHA1(840f1470886bd0019db3cd29e3d1d80205a65f48) )	/* unknown */
ROM_END

GAME( 1989, blockout, 0,        blockout, blockout, 0, ROT0, "Technos + California Dreams", "Block Out (set 1)" )
GAME( 1989, blckout2, blockout, blockout, blockout, 0, ROT0, "Technos + California Dreams", "Block Out (set 2)" )
GAME( 1989, blckoutj, blockout, blockout, blckoutj, 0, ROT0, "Technos + California Dreams", "Block Out (Japan)" )
