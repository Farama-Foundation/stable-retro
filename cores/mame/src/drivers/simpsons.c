/***************************************************************************

The Simpsons (c) 1991 Konami Co. Ltd

Preliminary driver by:
Ernesto Corvi
someone@secureshell.com

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/konami/konami.h" /* for the callback and the firq irq definition */
#include "cpu/z80/z80.h"
#include "vidhrdw/konamiic.h"

/* from vidhrdw */
VIDEO_START( simpsons );
WRITE_HANDLER( simpsons_priority_w );
VIDEO_UPDATE( simpsons );

/* from machine */
READ_HANDLER( simpsons_eeprom_r );
WRITE_HANDLER( simpsons_eeprom_w );
WRITE_HANDLER( simpsons_coin_counter_w );
READ_HANDLER( simpsons_sound_interrupt_r );
READ_HANDLER( simpsons_sound_r );
READ_HANDLER( simpsons_speedup1_r );
READ_HANDLER( simpsons_speedup2_r );
MACHINE_INIT( simpsons );
NVRAM_HANDLER( simpsons );
extern int simpsons_firq_enabled;

/***************************************************************************

  Memory Maps

***************************************************************************/

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x0fff, MRA_BANK3 },
	{ 0x1f80, 0x1f80, input_port_4_r },
	{ 0x1f81, 0x1f81, simpsons_eeprom_r },
	{ 0x1f90, 0x1f90, input_port_0_r },
	{ 0x1f91, 0x1f91, input_port_1_r },
	{ 0x1f92, 0x1f92, input_port_2_r },
	{ 0x1f93, 0x1f93, input_port_3_r },
	{ 0x1fc4, 0x1fc4, simpsons_sound_interrupt_r },
	{ 0x1fc6, 0x1fc7, simpsons_sound_r },	/* K053260 */
	{ 0x1fc8, 0x1fc9, K053246_r },
	{ 0x1fca, 0x1fca, watchdog_reset_r },
	{ 0x2000, 0x3fff, MRA_BANK4 },
	{ 0x0000, 0x3fff, K052109_r },
	{ 0x4856, 0x4856, simpsons_speedup2_r },
	{ 0x4942, 0x4942, simpsons_speedup1_r },
	{ 0x4000, 0x5fff, MRA_RAM },
	{ 0x6000, 0x7fff, MRA_BANK1 },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x0fff, MWA_BANK3 },
	{ 0x1fa0, 0x1fa7, K053246_w },
	{ 0x1fb0, 0x1fbf, K053251_w },
	{ 0x1fc0, 0x1fc0, simpsons_coin_counter_w },
	{ 0x1fc2, 0x1fc2, simpsons_eeprom_w },
	{ 0x1fc6, 0x1fc7, K053260_0_w },
	{ 0x2000, 0x3fff, MWA_BANK4 },
	{ 0x0000, 0x3fff, K052109_w },
	{ 0x4000, 0x5fff, MWA_RAM },
	{ 0x6000, 0x7fff, MWA_ROM },
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END

static WRITE_HANDLER( z80_bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_CPU2);

	offset = 0x10000 + ( ( ( data & 7 ) - 2 ) * 0x4000 );

	cpu_setbank( 2, &RAM[ offset ] );
}

#if 0
static int nmi_enabled;

static void sound_nmi_callback( int param )
{
	cpu_set_nmi_line( 1, ( nmi_enabled ) ? CLEAR_LINE : ASSERT_LINE );

	nmi_enabled = 0;
}
#endif

static void nmi_callback(int param)
{
	cpu_set_nmi_line(1,ASSERT_LINE);
}

static WRITE_HANDLER( z80_arm_nmi_w )
{
/*	sound_nmi_enabled = 1;*/
	cpu_set_nmi_line(1,CLEAR_LINE);
	timer_set(TIME_IN_USEC(50),0,nmi_callback);	/* kludge until the K053260 is emulated correctly */
}

static MEMORY_READ_START( z80_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK2 },
	{ 0xf000, 0xf7ff, MRA_RAM },
	{ 0xf801, 0xf801, YM2151_status_port_0_r },
	{ 0xfc00, 0xfc2f, K053260_0_r },
MEMORY_END

static MEMORY_WRITE_START( z80_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0xbfff, MWA_ROM },
	{ 0xf000, 0xf7ff, MWA_RAM },
	{ 0xf800, 0xf800, YM2151_register_port_0_w },
	{ 0xf801, 0xf801, YM2151_data_port_0_w },
	{ 0xfa00, 0xfa00, z80_arm_nmi_w },
	{ 0xfc00, 0xfc2f, K053260_0_w },
	{ 0xfe00, 0xfe00, z80_bankswitch_w },
MEMORY_END

/***************************************************************************

	Input Ports

***************************************************************************/

INPUT_PORTS_START( simpsons )
	PORT_START /* IN0 - Player 1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /*BUTTON3 Unused*/
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN1 - Player 2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /*BUTTON3 Unused*/
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* IN2 - Player 3 - Used on the 4p version */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /*BUTTON3 Unused*/
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START3 )

	PORT_START	/* IN3 - Player 4 - Used on the 4p version */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /*BUTTON3 Unused*/
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START4 )

	PORT_START /* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN ) /*SERVICE1 Unused*/
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN ) /*SERVICE2 Unused*/
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /*SERVICE3 Unused*/
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN ) /*SERVICE4 Unused*/

	PORT_START /* IN5 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0xfe, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( simpsn2p )
	PORT_START /* IN0 - Player 1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /*BUTTON3 Unused*/
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN1 - Player 2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /*BUTTON3 Unused*/
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* IN2 - Player 3 - Used on the 4p version */
/*	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )*/
/*	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )*/
/*	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )*/
/*	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )*/
/*	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )*/
/*	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )*/
/*	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) */ /*BUTTON3 Unused*/
/*	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START3 )*/

	PORT_START	/* IN3 - Player 4 - Used on the 4p version */
/*	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER4 )*/
/*	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )*/
/*	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER4 )*/
/*	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER4 )*/
/*	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )*/
/*	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )*/
/*	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) */ /*BUTTON3 Unused*/
/*	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START4 )*/

	PORT_START /* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN ) /*COIN3 Unused*/
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN ) /*COIN4 Unused*/
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN ) /*SERVICE2 Unused*/
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /*SERVICE3 Unused*/
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN ) /*SERVICE4 Unused*/

	PORT_START /* IN5 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0xfe, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END



/***************************************************************************

	Machine Driver

***************************************************************************/

static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	3579545,	/* 3.579545 MHz */
	{ YM3012_VOL(70,MIXER_PAN_CENTER,0,MIXER_PAN_CENTER) },	/* only left channel is connected */
	{ 0 }
};

static struct K053260_interface k053260_interface =
{
	1,
	{ 3579545 },
	{ REGION_SOUND1 }, /* memory region */
	{ { MIXER(75,MIXER_PAN_LEFT), MIXER(75,MIXER_PAN_RIGHT) } },
/*	{ nmi_callback }*/
};

static void simpsons_objdma(void)
{
	int counter, num_inactive;
	data16_t *src, *dst;

	K053247_export_config(&dst, 0, 0, 0, &counter);
	src = spriteram16;
	num_inactive = counter = 256;

	do {
		if ((*src & 0x8000) && (*src & 0xff))
		{
			memcpy(dst, src, 0x10);
			dst += 8;
			num_inactive--;
		}
		src += 8;
	}
	while (--counter);

	if (num_inactive) do { *dst = 0; dst += 8; } while (--num_inactive);
}

static void dmaend_callback(int data)
{
	if (simpsons_firq_enabled)
		cpu_set_irq_line(0, KONAMI_FIRQ_LINE, HOLD_LINE);
}

static INTERRUPT_GEN( simpsons_irq )
{
	if (K053246_is_IRQ_enabled())
	{
		simpsons_objdma();

		/* 32+256us delay at 8MHz dotclock; artificially shortened since actual V-blank length is unknown*/
		timer_set(TIME_IN_USEC(30), 0, dmaend_callback);
	}

	if (K052109_is_IRQ_enabled())
		cpu_set_irq_line(0, KONAMI_IRQ_LINE, HOLD_LINE);
}

static MACHINE_DRIVER_START( simpsons )

	/* basic machine hardware */
	MDRV_CPU_ADD(KONAMI, 3000000) /* ? */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(simpsons_irq,1)	/* IRQ triggered by the 052109, FIRQ by the sprite hardware */

	MDRV_CPU_ADD(Z80, 3579545)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(z80_readmem,z80_writemem)
								/* NMIs are generated by the 053260 */
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(simpsons)
	MDRV_NVRAM_HANDLER(simpsons)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_HAS_SHADOWS | VIDEO_HAS_HIGHLIGHTS | VIDEO_UPDATE_AFTER_VBLANK)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(14*8, (64-14)*8-1, 2*8, 30*8-1 )
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(simpsons)
	MDRV_VIDEO_UPDATE(simpsons)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(K053260, k053260_interface)
MACHINE_DRIVER_END


/***************************************************************************

  Game ROMs

***************************************************************************/

ROM_START( simpsons )
	ROM_REGION( 0x8b000, REGION_CPU1, 0 ) /* code + banked roms + banked ram */
	ROM_LOAD( "g02.16c",      0x10000, 0x20000, CRC(580ce1d6) SHA1(5b07fb8e8041e1663980aa35d853fdc13b22dac5) )
	ROM_LOAD( "g01.17c",      0x30000, 0x20000, CRC(9f843def) SHA1(858432b59101b0577c5cec6ac0c7c20ab0780c9a) )
	ROM_LOAD( "j13.13c",      0x50000, 0x20000, CRC(aade2abd) SHA1(10f178d5ed399b4866266e075d91ca3db26798f8) )
    ROM_LOAD( "j12.15c",      0x70000, 0x18000, CRC(479e12f2) SHA1(15a6cb12e68b4773a29ab463640a43f8e814de59) )
	ROM_CONTINUE(		      0x08000, 0x08000 )

	ROM_REGION( 0x28000, REGION_CPU2, 0 ) /* Z80 code + banks */
	ROM_LOAD( "e03.6g",       0x00000, 0x08000, CRC(866b7a35) SHA1(98905764eb4c7d968ccc17618a1f24ee12e33c0e) )
	ROM_CONTINUE(			  0x10000, 0x18000 )

	ROM_REGION( 0x100000, REGION_GFX1, 0 ) /* graphics ( dont dispose as the program can read them, 0 ) */
	ROM_LOAD( "simp_18h.rom", 0x000000, 0x080000, CRC(ba1ec910) SHA1(0805ccb641271dea43185dc0365732260db1763d) )	/* tiles */
	ROM_LOAD( "simp_16h.rom", 0x080000, 0x080000, CRC(cf2bbcab) SHA1(47afea47f9bc8cb5eb1c7b7fbafe954b3e749aeb) )

	ROM_REGION( 0x400000, REGION_GFX2, 0 ) /* graphics ( dont dispose as the program can read them, 0 ) */
	ROM_LOAD( "simp_3n.rom",  0x000000, 0x100000, CRC(7de500ad) SHA1(61b76b8f402e3bde1509679aaaa28ef08cafb0ab) )	/* sprites */
	ROM_LOAD( "simp_8n.rom",  0x100000, 0x100000, CRC(aa085093) SHA1(925239d79bf607021d371263352618876f59c1f8) )
	ROM_LOAD( "simp_12n.rom", 0x200000, 0x100000, CRC(577dbd53) SHA1(e603e03e3dcba766074561faa92afafa5761953d) )
	ROM_LOAD( "simp_16l.rom", 0x300000, 0x100000, CRC(55fab05d) SHA1(54db8559d71ed257de9a29c8808654eaea0df9e2) )

	ROM_REGION( 0x140000, REGION_SOUND1, 0 ) /* samples for the 053260 */
	ROM_LOAD( "simp_1f.rom", 0x000000, 0x100000, CRC(1397a73b) SHA1(369422c84cca5472967af54b8351e29fcd69f621) )
	ROM_LOAD( "simp_1d.rom", 0x100000, 0x040000, CRC(78778013) SHA1(edbd6d83b0d1a20df39bb160b92395586fa3c32d) )
ROM_END

ROM_START( simpsn2p )
	ROM_REGION( 0x8b000, REGION_CPU1, 0 ) /* code + banked roms + banked ram */
	ROM_LOAD( "g02.16c",      0x10000, 0x20000, CRC(580ce1d6) SHA1(5b07fb8e8041e1663980aa35d853fdc13b22dac5) )
	ROM_LOAD( "simp_p01.rom", 0x30000, 0x20000, CRC(07ceeaea) SHA1(c18255ae1d578c2d53de80d6323cdf41cbe47b57) )
	ROM_LOAD( "simp_013.rom", 0x50000, 0x20000, CRC(8781105a) SHA1(ef2f16f7a56d3715536511c674df4b3aab1be2bd) )
    ROM_LOAD( "simp_012.rom", 0x70000, 0x18000, CRC(244f9289) SHA1(eeda7f5c7340cbd1a1cd576af48cd5d1a629914a) )
	ROM_CONTINUE(		      0x08000, 0x08000 )

	ROM_REGION( 0x28000, REGION_CPU2, 0 ) /* Z80 code + banks */
	ROM_LOAD( "simp_g03.rom", 0x00000, 0x08000, CRC(76c1850c) SHA1(9047c6b26c4e33c74eb7400a807d3d9f206f7bbe) )
	ROM_CONTINUE(			  0x10000, 0x18000 )

	ROM_REGION( 0x100000, REGION_GFX1, 0 ) /* graphics ( dont dispose as the program can read them, 0 ) */
	ROM_LOAD( "simp_18h.rom", 0x000000, 0x080000, CRC(ba1ec910) SHA1(0805ccb641271dea43185dc0365732260db1763d) )	/* tiles */
	ROM_LOAD( "simp_16h.rom", 0x080000, 0x080000, CRC(cf2bbcab) SHA1(47afea47f9bc8cb5eb1c7b7fbafe954b3e749aeb) )

	ROM_REGION( 0x400000, REGION_GFX2, 0 ) /* graphics ( dont dispose as the program can read them, 0 ) */
	ROM_LOAD( "simp_3n.rom",  0x000000, 0x100000, CRC(7de500ad) SHA1(61b76b8f402e3bde1509679aaaa28ef08cafb0ab) )	/* sprites */
	ROM_LOAD( "simp_8n.rom",  0x100000, 0x100000, CRC(aa085093) SHA1(925239d79bf607021d371263352618876f59c1f8) )
	ROM_LOAD( "simp_12n.rom", 0x200000, 0x100000, CRC(577dbd53) SHA1(e603e03e3dcba766074561faa92afafa5761953d) )
	ROM_LOAD( "simp_16l.rom", 0x300000, 0x100000, CRC(55fab05d) SHA1(54db8559d71ed257de9a29c8808654eaea0df9e2) )

	ROM_REGION( 0x140000, REGION_SOUND1, 0 ) /* samples for the 053260 */
	ROM_LOAD( "simp_1f.rom", 0x000000, 0x100000, CRC(1397a73b) SHA1(369422c84cca5472967af54b8351e29fcd69f621) )
	ROM_LOAD( "simp_1d.rom", 0x100000, 0x040000, CRC(78778013) SHA1(edbd6d83b0d1a20df39bb160b92395586fa3c32d) )
ROM_END

ROM_START( simps2pa )
	ROM_REGION( 0x8b000, REGION_CPU1, 0 ) /* code + banked roms + banked ram */
        ROM_LOAD( "simp2.16c",    0x010000, 0x020000, CRC(580ce1d6) SHA1(5b07fb8e8041e1663980aa35d853fdc13b22dac5) )
        ROM_LOAD( "simp1.17c",    0x030000, 0x020000, CRC(07ceeaea) SHA1(c18255ae1d578c2d53de80d6323cdf41cbe47b57) )
        ROM_LOAD( "simp4.13c",    0x050000, 0x020000, CRC(54e6df66) SHA1(1b83ae56cf1deb51b04880fa421f06568c938a99) )
        ROM_LOAD( "simp3.15c",    0x070000, 0x018000, CRC(96636225) SHA1(5de95606e5c9337f18bc42f4df791cacafa20399) )
	ROM_CONTINUE(		      0x08000, 0x08000 )

	ROM_REGION( 0x28000, REGION_CPU2, 0 ) /* Z80 code + banks */
	ROM_LOAD( "simp5.6g",       0x00000, 0x08000, CRC(76c1850c) SHA1(9047c6b26c4e33c74eb7400a807d3d9f206f7bbe) )
	ROM_CONTINUE(			  0x10000, 0x18000 )

	ROM_REGION( 0x100000, REGION_GFX1, 0 ) /* graphics ( dont dispose as the program can read them, 0 ) */
	ROM_LOAD( "simp_18h.rom", 0x000000, 0x080000, CRC(ba1ec910) SHA1(0805ccb641271dea43185dc0365732260db1763d) )	/* tiles */
	ROM_LOAD( "simp_16h.rom", 0x080000, 0x080000, CRC(cf2bbcab) SHA1(47afea47f9bc8cb5eb1c7b7fbafe954b3e749aeb) )

	ROM_REGION( 0x400000, REGION_GFX2, 0 ) /* graphics ( dont dispose as the program can read them, 0 ) */
	ROM_LOAD( "simp_3n.rom",  0x000000, 0x100000, CRC(7de500ad) SHA1(61b76b8f402e3bde1509679aaaa28ef08cafb0ab) )	/* sprites */
	ROM_LOAD( "simp_8n.rom",  0x100000, 0x100000, CRC(aa085093) SHA1(925239d79bf607021d371263352618876f59c1f8) )
	ROM_LOAD( "simp_12n.rom", 0x200000, 0x100000, CRC(577dbd53) SHA1(e603e03e3dcba766074561faa92afafa5761953d) )
	ROM_LOAD( "simp_16l.rom", 0x300000, 0x100000, CRC(55fab05d) SHA1(54db8559d71ed257de9a29c8808654eaea0df9e2) )

	ROM_REGION( 0x140000, REGION_SOUND1, 0 ) /* samples for the 053260 */
	ROM_LOAD( "simp_1f.rom", 0x000000, 0x100000, CRC(1397a73b) SHA1(369422c84cca5472967af54b8351e29fcd69f621) )
	ROM_LOAD( "simp_1d.rom", 0x100000, 0x040000, CRC(78778013) SHA1(edbd6d83b0d1a20df39bb160b92395586fa3c32d) )
ROM_END

ROM_START( simps2pj )
	ROM_REGION( 0x8b000, REGION_CPU1, 0 ) /* code + banked roms + banked ram */
	ROM_LOAD( "072-s02.16c",  0x10000, 0x20000, CRC(265f7a47) SHA1(d39c19a5e303f822313409343b209947f4c47ae4) )
	ROM_LOAD( "072-t01.17c",  0x30000, 0x20000, CRC(91de5c2d) SHA1(1e18a5585ed821ec7cda69bdcdbfa4e6c71455c6) )
	ROM_LOAD( "072-213.13c",  0x50000, 0x20000, CRC(b326a9ae) SHA1(f222c33f2e8b306f2f0ef6f0da9febbf8219e1a4) )
    ROM_LOAD( "072-212.15c",  0x70000, 0x18000, CRC(584d9d37) SHA1(61b9df4dfb323b7284894e5e1eb9d713ebf64721) )
	ROM_CONTINUE(		      0x08000, 0x08000 )

	ROM_REGION( 0x28000, REGION_CPU2, 0 ) /* Z80 code + banks */
	ROM_LOAD( "simp_g03.rom", 0x00000, 0x08000, CRC(76c1850c) SHA1(9047c6b26c4e33c74eb7400a807d3d9f206f7bbe) )
	ROM_CONTINUE(			  0x10000, 0x18000 )

	ROM_REGION( 0x100000, REGION_GFX1, 0 ) /* graphics ( dont dispose as the program can read them, 0 ) */
	ROM_LOAD( "simp_18h.rom", 0x000000, 0x080000, CRC(ba1ec910) SHA1(0805ccb641271dea43185dc0365732260db1763d) )	/* tiles */
	ROM_LOAD( "simp_16h.rom", 0x080000, 0x080000, CRC(cf2bbcab) SHA1(47afea47f9bc8cb5eb1c7b7fbafe954b3e749aeb) )

	ROM_REGION( 0x400000, REGION_GFX2, 0 ) /* graphics ( dont dispose as the program can read them, 0 ) */
	ROM_LOAD( "simp_3n.rom",  0x000000, 0x100000, CRC(7de500ad) SHA1(61b76b8f402e3bde1509679aaaa28ef08cafb0ab) )	/* sprites */
	ROM_LOAD( "simp_8n.rom",  0x100000, 0x100000, CRC(aa085093) SHA1(925239d79bf607021d371263352618876f59c1f8) )
	ROM_LOAD( "simp_12n.rom", 0x200000, 0x100000, CRC(577dbd53) SHA1(e603e03e3dcba766074561faa92afafa5761953d) )
	ROM_LOAD( "simp_16l.rom", 0x300000, 0x100000, CRC(55fab05d) SHA1(54db8559d71ed257de9a29c8808654eaea0df9e2) )

	ROM_REGION( 0x140000, REGION_SOUND1, 0 ) /* samples for the 053260 */
	ROM_LOAD( "simp_1f.rom", 0x000000, 0x100000, CRC(1397a73b) SHA1(369422c84cca5472967af54b8351e29fcd69f621) )
	ROM_LOAD( "simp_1d.rom", 0x100000, 0x040000, CRC(78778013) SHA1(edbd6d83b0d1a20df39bb160b92395586fa3c32d) )
ROM_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

static DRIVER_INIT( simpsons )
{
	konami_rom_deinterleave_2(REGION_GFX1);
	konami_rom_deinterleave_4(REGION_GFX2);
}

GAME( 1991, simpsons, 0,        simpsons, simpsons, simpsons, ROT0, "Konami", "The Simpsons (4 Players)" )
GAME( 1991, simpsn2p, simpsons, simpsons, simpsn2p, simpsons, ROT0, "Konami", "The Simpsons (2 Players)" )
GAME( 1991, simps2pa, simpsons, simpsons, simpsons, simpsons, ROT0, "Konami", "The Simpsons (2 Players alt)" )
GAME( 1991, simps2pj, simpsons, simpsons, simpsn2p, simpsons, ROT0, "Konami", "The Simpsons (2 Players Japan)" )
