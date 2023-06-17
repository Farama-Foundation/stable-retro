/* Ramtek M79 Ambush */

#include "driver.h"
#include "vidhrdw/generic.h"
/*
 * in
 * 8000 DIP SW
 * 8002 D0=VBlank
 * 8004
 * 8005
 *
 * out
 * 8000
 * 8001 Mask Sel
 * 8002
 * 8003 D0=SelfTest LED
 *
 */

WRITE_HANDLER( ramtek_videoram_w );

INTERRUPT_GEN( invaders_interrupt );
void ramtek_sh_update(void);
WRITE_HANDLER( ramtek_mask_w );

/*
 * since these functions aren't used anywhere else, i've made them
 * static, and included them here
 */
static const int ControllerTable[32] = {
    0  , 1  , 3  , 2  , 6  , 7  , 5  , 4  ,
    12 , 13 , 15 , 14 , 10 , 11 , 9  , 8  ,
    24 , 25 , 27 , 26 , 30 , 31 , 29 , 28 ,
    20 , 21 , 23 , 22 , 18 , 19 , 17 , 16
};

static READ_HANDLER( gray5bit_controller0_r )
{
    return (input_port_2_r(0) & 0xe0) | (~ControllerTable[input_port_2_r(0) & 0x1f] & 0x1f);
}

static READ_HANDLER( gray5bit_controller1_r )
{
    return (input_port_3_r(0) & 0xe0) | (~ControllerTable[input_port_3_r(0) & 0x1f] & 0x1f);
}

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x4000, 0x63ff, MRA_RAM },
	{ 0x8000, 0x8000, input_port_0_r},
	{ 0x8002, 0x8002, input_port_1_r},
	{ 0x8004, 0x8004, gray5bit_controller0_r},
	{ 0x8005, 0x8005, gray5bit_controller1_r},
	{ 0xC000, 0xC07f, MRA_RAM},			/* ?? */
	{ 0xC200, 0xC27f, MRA_RAM},			/* ?? */
MEMORY_END

WRITE_HANDLER( sound_w )
{
}

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x4000, 0x43ff, MWA_RAM },
    { 0x4400, 0x5fff, ramtek_videoram_w, &videoram },
    { 0x6000, 0x63ff, MWA_RAM },		/* ?? */
	{ 0x8001, 0x8001, ramtek_mask_w},
	{ 0x8000, 0x8000, sound_w },
	{ 0x8002, 0x8003, sound_w },
	{ 0xC000, 0xC07f, MWA_RAM},			/* ?? */
	{ 0xC200, 0xC27f, MWA_RAM},			/* ?? */
MEMORY_END


INPUT_PORTS_START( m79amb )
	PORT_START      /* 8000 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* dip switch */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START      /* 8002 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_VBLANK )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_COIN1  )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_TILT   )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNUSED )

	PORT_START		/* 8004 */
	PORT_ANALOG( 0x1f, 0x10, IPT_PADDLE, 25, 10, 0, 0x1f)
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START      /* 8005 */
	PORT_ANALOG( 0x1f, 0x10, IPT_PADDLE | IPF_PLAYER2, 25, 10, 0, 0x1f)
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )

INPUT_PORTS_END


static PALETTE_INIT( m79amb )
{
	palette_set_color(0,0x00,0x00,0x00); /* BLACK */
	palette_set_color(1,0xff,0xff,0xff); /* WHITE */
	palette_set_color(2,0xff,0x20,0x20); /* RED */
	palette_set_color(3,0x20,0xff,0x20); /* GREEN */
	palette_set_color(4,0xff,0xff,0x20); /* YELLOW */
	palette_set_color(5,0x20,0xff,0xff); /* CYAN */
	palette_set_color(6,0xff,0x20,0xff); /* PURPLE */
}

static INTERRUPT_GEN( M79_interrupt )
{
	cpu_set_irq_line_and_vector(0, 0, HOLD_LINE, 0xcf);  /* RST 08h */
}

static DRIVER_INIT( m79amb )
{
	unsigned char *rom = memory_region(REGION_CPU1);
	int i;

	/* PROM data is active low */
 	for (i = 0;i < 0x2000;i++)
		rom[i] = ~rom[i];
}

static MACHINE_DRIVER_START( m79amb )

	/* basic machine hardware */
	MDRV_CPU_ADD(8080, 1996800)
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(M79_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 28*8-1)
	MDRV_PALETTE_LENGTH(7)

	MDRV_PALETTE_INIT(m79amb)
	MDRV_VIDEO_START(generic_bitmapped)
	MDRV_VIDEO_UPDATE(generic_bitmapped)

	/* sound hardware */
MACHINE_DRIVER_END



ROM_START( m79amb )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "m79.10t",      0x0000, 0x0200, CRC(ccf30b1e) SHA1(c1a77f8dc81c491928f81121ca5c9b7f8753794f) )
	ROM_LOAD( "m79.9t",       0x0200, 0x0200, CRC(daf807dd) SHA1(16cd9d553bfb111c8380966cbde39dbddd5fe58c) )
	ROM_LOAD( "m79.8t",       0x0400, 0x0200, CRC(79fafa02) SHA1(440620f5be44febdd7c64014739dc71fb570cc92) )
	ROM_LOAD( "m79.7t",       0x0600, 0x0200, CRC(06f511f8) SHA1(a8dcaec0723b8ac9ad9474e3e931a23d7aa4ec23) )
	ROM_LOAD( "m79.6t",       0x0800, 0x0200, CRC(24634390) SHA1(bc5f2ae5a49904dde1bd5e6b134571bf1d912735) )
	ROM_LOAD( "m79.5t",       0x0a00, 0x0200, CRC(95252aa6) SHA1(e7ea598f864510557b511682a5d2223a512ff029) )
	ROM_LOAD( "m79.4t",       0x0c00, 0x0200, CRC(54cffb0f) SHA1(4a4ad921ef6324c927a2e4a9da624d8096b6d87b) )
	ROM_LOAD( "m79.3ta",      0x0e00, 0x0200, CRC(27db5ede) SHA1(890587181497ed6e1d45ed501790a6d4d3315f00) )
	ROM_LOAD( "m79.10u",      0x1000, 0x0200, CRC(e41d13d2) SHA1(cc2911f46a0465305e4c7bc08f55acd065f93534) )
	ROM_LOAD( "m79.9u",       0x1200, 0x0200, CRC(e35f5616) SHA1(394ad92ad7dd233ece17335cf20aef8861b41508) )
	ROM_LOAD( "m79.8u",       0x1400, 0x0200, CRC(14eafd7c) SHA1(ca2d17f6ae1c3ff461a1b2bc6f37622e70cdaae8) )
	ROM_LOAD( "m79.7u",       0x1600, 0x0200, CRC(b9864f25) SHA1(9330cf96b7bce13e0ee3ad746b00e82ef10c3989) )
	ROM_LOAD( "m79.6u",       0x1800, 0x0200, CRC(dd25197f) SHA1(13eaf40251de82e817f488a9de738aadd8f6715e) )
	ROM_LOAD( "m79.5u",       0x1a00, 0x0200, CRC(251545e2) SHA1(05a0d5e8f143ea376fb3c517cf5e9d0d3534b933) )
	ROM_LOAD( "m79.4u",       0x1c00, 0x0200, CRC(b5f55c75) SHA1(f478cde73ae961be7b58c769f035eef58fd45555) )
	ROM_LOAD( "m79.3u",       0x1e00, 0x0200, CRC(e968691a) SHA1(7024d10f2af195fc4050861706b1f3d22cb22a9c) )
ROM_END



GAMEX( 1977, m79amb, 0, m79amb, m79amb, m79amb, ROT0, "RamTek", "M79 Ambush", GAME_NO_SOUND )
