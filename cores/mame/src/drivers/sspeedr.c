/***************************************************************************

Taito Super Speed Race driver

***************************************************************************/

#include "driver.h"
#include "artwork.h"

extern WRITE_HANDLER( sspeedr_driver_horz_w );
extern WRITE_HANDLER( sspeedr_driver_horz_2_w );
extern WRITE_HANDLER( sspeedr_driver_vert_w );
extern WRITE_HANDLER( sspeedr_driver_pic_w );

extern WRITE_HANDLER( sspeedr_drones_horz_w );
extern WRITE_HANDLER( sspeedr_drones_horz_2_w );
extern WRITE_HANDLER( sspeedr_drones_vert_w );
extern WRITE_HANDLER( sspeedr_drones_mask_w );

extern WRITE_HANDLER( sspeedr_track_horz_w );
extern WRITE_HANDLER( sspeedr_track_horz_2_w );
extern WRITE_HANDLER( sspeedr_track_vert_w );
extern WRITE_HANDLER( sspeedr_track_ice_w );

extern VIDEO_START( sspeedr );
extern VIDEO_UPDATE( sspeedr );
extern VIDEO_EOF( sspeedr );

static UINT8 led_TIME[2];
static UINT8 led_SCORE[24];


static PALETTE_INIT( sspeedr )
{
	int i;

	for (i = 0; i < 16; i++)
	{
		int r = (i & 1) ? 0xb0 : 0x20;
		int g = (i & 2) ? 0xb0 : 0x20;
		int b = (i & 4) ? 0xb0 : 0x20;

		if (i & 8)
		{
			r += 0x4f;
			g += 0x4f;
			b += 0x4f;
		}

		palette_set_color(i, r, g, b);
	}
}


static READ_HANDLER( sspeedr_steering_r )
{
	UINT8 val = readinputport(0);

	return 0x3f ^ (val >> 2) ^ (val >> 3);
}


static WRITE_HANDLER( sspeedr_int_ack_w )
{
	cpu_set_irq_line(0, 0, CLEAR_LINE);
}


static WRITE_HANDLER( sspeedr_lamp_w )
{
	artwork_show("lampGO",
		data & 1);
	artwork_show("lampEP",
		data & 2);

	coin_counter_w(0, data & 8);
}


static WRITE_HANDLER( sspeedr_time_w )
{
	UINT8 prev = led_TIME[offset];

	char buf_old[8];
	char buf_new[8];

	data = data & 15;

	sprintf(buf_old, "LEDT%d-%c", offset, prev >= 10 ? 'X' : '0' + prev);
	sprintf(buf_new, "LEDT%d-%c", offset, data >= 10 ? 'X' : '0' + data);

	artwork_show(buf_old, 0);
	artwork_show(buf_new, 1);

	led_TIME[offset] = data;
}


static WRITE_HANDLER( sspeedr_score_w )
{
	UINT8 prev = led_SCORE[offset];

	char buf_old[8];
	char buf_new[8];

	data = ~data & 15;

	sprintf(buf_old, "LED%02d-%c", offset, prev >= 10 ? 'X' : '0' + prev);
	sprintf(buf_new, "LED%02d-%c", offset, data >= 10 ? 'X' : '0' + data);

	artwork_show(buf_old, 0);
	artwork_show(buf_new, 1);

	led_SCORE[offset] = data;
}


static WRITE_HANDLER( sspeedr_sound_w )
{
	/* not implemented */
}


static MEMORY_READ_START( readmem )
	{ 0x0000, 0x0fff, MRA_ROM },
	{ 0x2000, 0x21ff, MRA_RAM },
MEMORY_END


static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x0fff, MWA_ROM },
	{ 0x2000, 0x21ff, MWA_RAM },
	{ 0x7f00, 0x7f17, sspeedr_score_w },
MEMORY_END


static PORT_READ_START( readport )
	{ 0x00, 0x00, sspeedr_steering_r },
	{ 0x01, 0x01, input_port_1_r },
	{ 0x03, 0x03, input_port_2_r },
	{ 0x04, 0x04, input_port_3_r },
PORT_END


static PORT_WRITE_START( writeport )
	{ 0x00, 0x01, sspeedr_sound_w },
	{ 0x02, 0x02, sspeedr_lamp_w },
	{ 0x04, 0x05, sspeedr_time_w },
	{ 0x06, 0x06, watchdog_reset_w },
	{ 0x10, 0x10, sspeedr_driver_horz_w },
	{ 0x11, 0x11, sspeedr_driver_pic_w },
	{ 0x12, 0x12, sspeedr_driver_horz_2_w },
	{ 0x13, 0x13, sspeedr_drones_horz_w },
	{ 0x14, 0x14, sspeedr_drones_horz_2_w },
	{ 0x15, 0x15, sspeedr_drones_mask_w },
	{ 0x16, 0x16, sspeedr_driver_vert_w },
	{ 0x17, 0x18, sspeedr_track_vert_w },
	{ 0x19, 0x19, sspeedr_track_horz_w },
	{ 0x1a, 0x1a, sspeedr_track_horz_2_w },
	{ 0x1b, 0x1b, sspeedr_track_ice_w },
	{ 0x1c, 0x1e, sspeedr_drones_vert_w },
	{ 0x1f, 0x1f, sspeedr_int_ack_w },
PORT_END


INPUT_PORTS_START( sspeedr )

	PORT_START
	PORT_ANALOG( 0xff, 0x80, IPT_DIAL, 25, 10, 0x00, 0xff )

	PORT_START
	PORT_ANALOG( 0x1f, 0x00, IPT_PEDAL | IPF_REVERSE, 25, 20, 0x00, 0x1f )

	PORT_START
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x0C, 0x08, "Play Time" )
	PORT_DIPSETTING(    0x00, "60 seconds")
	PORT_DIPSETTING(    0x04, "70 seconds")
	PORT_DIPSETTING(    0x08, "80 seconds")
	PORT_DIPSETTING(    0x0C, "90 seconds")
	PORT_DIPNAME( 0x10, 0x00, "Extended Play" )
	PORT_DIPSETTING(    0x00, "20 seconds" )
	PORT_DIPSETTING(    0x10, "30 seconds" )
	PORT_DIPNAME( 0xE0, 0x20, DEF_STR( Service_Mode ) )
	PORT_DIPSETTING(    0x20, "Play Mode" )
	PORT_DIPSETTING(    0xA0, "RAM/ROM Test" )
	PORT_DIPSETTING(    0xE0, "Accelerator Adjustment" )

	PORT_START
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_TOGGLE ) /* gear shift lever */
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )

INPUT_PORTS_END


static struct GfxLayout car_layout =
{
	32, 16,
	16,
	4,
	{ 0, 1, 2, 3 },
	{
		0x04, 0x04, 0x00, 0x00, 0x0c, 0x0c, 0x08, 0x08,
		0x14, 0x14, 0x10, 0x10, 0x1c, 0x1c, 0x18, 0x18,
		0x24, 0x24, 0x20, 0x20, 0x2c, 0x2c, 0x28, 0x28,
		0x34, 0x34, 0x30, 0x30, 0x3c, 0x3c, 0x38, 0x38
	},
	{
		0x000, 0x040, 0x080, 0x0c0, 0x100, 0x140, 0x180, 0x1c0,
		0x200, 0x240, 0x280, 0x2c0, 0x300, 0x340, 0x380, 0x3c0
	},
	0x400
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &car_layout, 0, 1 },
	{ REGION_GFX2, 0, &car_layout, 0, 1 },
	{ -1 }
};


static MACHINE_DRIVER_START( sspeedr )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 19968000 / 8)
	MDRV_CPU_MEMORY(readmem, writemem)
	MDRV_CPU_PORTS(readport, writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_assert, 1)

	MDRV_FRAMES_PER_SECOND(59.39)
	MDRV_VBLANK_DURATION(16 * 1000000 / 15680)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(376, 256)
	MDRV_VISIBLE_AREA(0, 375, 0, 247)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(16)

	MDRV_PALETTE_INIT(sspeedr)
	MDRV_VIDEO_START(sspeedr)
	MDRV_VIDEO_UPDATE(sspeedr)
	MDRV_VIDEO_EOF(sspeedr)

	/* sound hardware */
MACHINE_DRIVER_END


ROM_START( sspeedr )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "ssr0000.pgm", 0x0000, 0x0800, CRC(bfc7069a) SHA1(2f7aa3d3c7cfd804ba4b625c6a8338534a204855) )
	ROM_LOAD( "ssr0800.pgm", 0x0800, 0x0800, CRC(ec46b59a) SHA1(d5727efecb32ad3d034b885e4a57d7373368ca9e) )

	ROM_REGION( 0x0800, REGION_GFX1, ROMREGION_DISPOSE ) /* driver */
	ROM_LOAD( "ssrm762a.f3", 0x0000, 0x0800, CRC(de4653a9) SHA1(a6bbffb7eb60581eee43c74d20ca00b50c9a6e07) )

	ROM_REGION( 0x0800, REGION_GFX2, ROMREGION_DISPOSE ) /* drone */
	ROM_LOAD( "ssrm762b.j3", 0x0000, 0x0800, CRC(ef6a1cd6) SHA1(77c31f14783e5ba90849bdc930b099c8360aeba7) )

	ROM_REGION( 0x0800, REGION_GFX3, 0 ) /* track */
	ROM_LOAD( "ssrm762c.l3", 0x0000, 0x0800, CRC(ebaad3ee) SHA1(54ac994b505d20c75cf07a4f68da12360ee00153) )
ROM_END


GAMEX( 1979, sspeedr, 0, sspeedr, sspeedr, 0, ROT270, "Midway", "Super Speed Race", GAME_NO_SOUND )
