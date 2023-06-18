/***************************************************************************

  Atari Tournament Table driver

  Hardware is identical to the VCS2600 except for an extra 6532 chip.

***************************************************************************/

#include "driver.h"
#include "machine/6532riot.h"
#include "cpu/m6502/m6502.h"
#include "sound/tiaintf.h"


extern PALETTE_INIT( tia_NTSC );
extern PALETTE_INIT( tia_PAL );

extern VIDEO_START( tia );
extern VIDEO_UPDATE( tia );

extern READ_HANDLER( tia_r );
extern WRITE_HANDLER( tia_w );

extern void tia_init(void);


static UINT8* r6532_0_ram;
static UINT8* r6532_1_ram;



static WRITE_HANDLER( tourtabl_led_w )
{
	set_led_status(0, data & 0x40); /* start 1 */
	set_led_status(1, data & 0x20); /* start 2 */
	set_led_status(2, data & 0x10); /* start 4 */
	set_led_status(3, data & 0x80); /* select game */

	coin_lockout_global_w(!(data & 0x80));
}


static WRITE_HANDLER( r6532_0_ram_w )
{
	r6532_0_ram[offset] = data;
}
static WRITE_HANDLER( r6532_1_ram_w )
{
	r6532_1_ram[offset] = data;
}


static READ_HANDLER( r6532_0_ram_r )
{
	return r6532_0_ram[offset];
}
static READ_HANDLER( r6532_1_ram_r )
{
	return r6532_1_ram[offset];
}


static MEMORY_READ_START( readmem )
	{ 0x0000, 0x007F, tia_r },
	{ 0x0080, 0x00FF, r6532_0_ram_r },
	{ 0x0100, 0x017F, tia_r },
	{ 0x0180, 0x01FF, r6532_0_ram_r },
	{ 0x0280, 0x029F, r6532_0_r },
	{ 0x0400, 0x047F, r6532_1_ram_r },
	{ 0x0500, 0x051F, r6532_1_r },
	{ 0x0800, 0x1FFF, MRA_ROM },
	{ 0xE800, 0xFFFF, MRA_ROM },
MEMORY_END


static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x007F, tia_w },
	{ 0x0080, 0x00FF, r6532_0_ram_w, &r6532_0_ram },
	{ 0x0100, 0x017F, tia_w },
	{ 0x0180, 0x01FF, r6532_0_ram_w },
	{ 0x0280, 0x029F, r6532_0_w },
	{ 0x0400, 0x047F, r6532_1_ram_w, &r6532_1_ram },
	{ 0x0500, 0x051F, r6532_1_w },
	{ 0x0800, 0x1FFF, MWA_ROM },
	{ 0xE800, 0xFFFF, MWA_ROM },
MEMORY_END


static const struct R6532interface r6532_interface_0 =
{
	input_port_6_r,
	input_port_7_r,
	NULL,
	watchdog_reset_w
};


static const struct R6532interface r6532_interface_1 =
{
	input_port_8_r,
	input_port_9_r,
	NULL,
	tourtabl_led_w
};


static struct TIAinterface tia_interface =
{
    31400, 255, TIA_DEFAULT_GAIN
};


static MACHINE_INIT( tourtabl )
{
	r6532_init(0, &r6532_interface_0);
	r6532_init(1, &r6532_interface_1);

	tia_init();
}


INPUT_PORTS_START( tourtabl )

	PORT_START
	PORT_ANALOG( 0xff, 0x80, IPT_PADDLE | IPF_PLAYER1 | IPF_REVERSE, 40, 10, 0, 255 )

	PORT_START
	PORT_ANALOG( 0xff, 0x80, IPT_PADDLE | IPF_PLAYER2 | IPF_REVERSE, 40, 10, 0, 255 )

	PORT_START
	PORT_ANALOG( 0xff, 0x80, IPT_PADDLE | IPF_PLAYER3 | IPF_REVERSE, 40, 10, 0, 255 )

	PORT_START
	PORT_ANALOG( 0xff, 0x80, IPT_PADDLE | IPF_PLAYER4 | IPF_REVERSE, 40, 10, 0, 255 )

	PORT_START /* TIA INPT4 */
	PORT_DIPNAME( 0x80, 0x80, "Breakout Replay" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x80, DEF_STR( On ))

	PORT_START /* TIA INPT5 */
	PORT_DIPNAME( 0x80, 0x80, "Game Length" )
	PORT_DIPSETTING(    0x00, "11 points (3 balls)" )
	PORT_DIPSETTING(    0x80, "15 points (5 balls)" )

	PORT_START /* RIOT #0 SWCHA */
	PORT_DIPNAME( 0x0F, 0x0E, "Replay Level" )
	PORT_DIPSETTING(    0x0B, "200 points" )
	PORT_DIPSETTING(    0x0C, "250 points" )
	PORT_DIPSETTING(    0x0D, "300 points" )
	PORT_DIPSETTING(    0x0E, "400 points" )
	PORT_DIPSETTING(    0x0F, "450 points" )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )

	PORT_START /* RIOT #0 SWCHB */
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_SERVICE, "Game Select", KEYCODE_SPACE, IP_JOY_DEFAULT )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START /* RIOT #1 SWCHA */
	PORT_DIPNAME( 0x0F, 0x07, DEF_STR( Coinage ))
	PORT_DIPSETTING(    0x00, "Mode A" )
	PORT_DIPSETTING(    0x01, "Mode B" )
	PORT_DIPSETTING(    0x02, "Mode C" )
	PORT_DIPSETTING(    0x03, "Mode D" )
	PORT_DIPSETTING(    0x04, "Mode E" )
	PORT_DIPSETTING(    0x05, "Mode F" )
	PORT_DIPSETTING(    0x06, "Mode G" )
	PORT_DIPSETTING(    0x07, "Mode H" )
	PORT_DIPSETTING(    0x08, "Mode I" )
	PORT_DIPSETTING(    0x09, "Mode J" )
	PORT_DIPSETTING(    0x0A, "Mode K" )
	PORT_DIPSETTING(    0x0B, "Mode L" )
	PORT_DIPSETTING(    0x0C, "Mode M" )
	PORT_DIPSETTING(    0x0D, "Mode N" )
	PORT_DIPSETTING(    0x0E, "Mode O" )
	PORT_DIPSETTING(    0x0F, "Mode P" )
	PORT_DIPNAME( 0x30, 0x00, "Language" )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPSETTING(    0x10, "French" )
	PORT_DIPSETTING(    0x20, "German" )
	PORT_DIPSETTING(    0x30, "Spanish" )
	PORT_SERVICE( 0x40, IP_ACTIVE_HIGH )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START /* RIOT #1 SWCHB */
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_START1 )

INPUT_PORTS_END


static MACHINE_DRIVER_START( tourtabl )
	/* basic machine hardware */
	MDRV_CPU_ADD(M6502, 3579575 / 3)	/* actually M6507 */
	MDRV_CPU_MEMORY(readmem, writemem)

	MDRV_FRAMES_PER_SECOND(60)

	MDRV_MACHINE_INIT(tourtabl)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(160, 262)
	MDRV_VISIBLE_AREA(0, 159, 46, 245)
	MDRV_PALETTE_LENGTH(128)
	MDRV_PALETTE_INIT(tia_NTSC)

	MDRV_VIDEO_START(tia)
	MDRV_VIDEO_UPDATE(tia)

	/* sound hardware */
	MDRV_SOUND_ADD(TIA, tia_interface)
MACHINE_DRIVER_END


ROM_START( tourtabl )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "030751.ab2", 0x0800, 0x0800, CRC(4479a6f7) SHA1(bf3fd859614533a592f831e3539ea0a9d1964c82) )
	ROM_RELOAD(             0xE800, 0x0800 )
	ROM_LOAD( "030752.ab3", 0x1000, 0x0800, CRC(c92c49dc) SHA1(cafcf13e1b1087b477a667d1e785f5e2be187b0d) )
	ROM_RELOAD(             0xF000, 0x0800 )
	ROM_LOAD( "030753.ab4", 0x1800, 0x0800, CRC(3978b269) SHA1(4fa05c655bb74711eb99428f36df838ec70da699) )
	ROM_RELOAD(             0xF800, 0x0800 )
ROM_END


ROM_START( tourtab2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "030929.ab2", 0x0800, 0x0800, CRC(fcdfafa2) SHA1(f35ab83366a334a110fbba0cef09f4db950dbb68) )
	ROM_RELOAD(             0xE800, 0x0800 )
	ROM_LOAD( "030752.ab3", 0x1000, 0x0800, CRC(c92c49dc) SHA1(cafcf13e1b1087b477a667d1e785f5e2be187b0d) )
	ROM_RELOAD(             0xF000, 0x0800 )
	ROM_LOAD( "030753.ab4", 0x1800, 0x0800, CRC(3978b269) SHA1(4fa05c655bb74711eb99428f36df838ec70da699) )
	ROM_RELOAD(             0xF800, 0x0800 )
ROM_END


GAME( 1978, tourtabl, 0,        tourtabl, tourtabl, NULL, ROT0, "Atari", "Tournament Table (set 1)" )
GAME( 1978, tourtab2, tourtabl, tourtabl, tourtabl, NULL, ROT0, "Atari", "Tournament Table (set 2)" )
