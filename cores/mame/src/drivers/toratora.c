/***************************************************************************

Tora Tora (c) 1980 GamePlan

driver by Nicola Salmoria

TODO:
- The game doesn't seem to work right. It also reads some unmapped memory
  addresses, are the two things related? Missing ROMs? There's an empty
  socket for U3 on the board, which should map at 5000-57ff, however the
  game reads mostly from 4800-4fff, which would be U6 according to the
  schematics.

- The manual mentions dip switch settings and the schematics show the switches,
  the game reads them but ignores them, forcing 1C/1C and 3 lives.
  Maybe the dump is from a proto?

- Hook up sound (two SN76477 connected to PIA 1 and 2)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/6821pia.h"




WRITE_HANDLER( toratora_videoram_w )
{
	if (videoram[offset] != data)
	{
		int i,x,y;

		videoram[offset] = data;

		y = offset / 32;
		x = 8 * (offset % 32);

		for (i = 0; i < 8; i++)
		{
			plot_pixel(tmpbitmap, x, y, Machine->pens[(data & 0x80) ? 1 : 0]);

			x++;
			data <<= 1;
		}
	}
}

WRITE_HANDLER( toratora_clear_tv_w )
{
	for (offset = 0;offset < 0x2000;offset++)
		toratora_videoram_w(offset,0);
}




INTERRUPT_GEN( toratora_interrupt )
{
	/* for simplicity, I generate an IRQ every vblank. In reality, the IRQ
	   should be generated every time the status of an input
	   (buttons + coins) changes. */
	cpu_set_irq_line(0, 0, HOLD_LINE);
}



static READ_HANDLER( porta_0_r )
{
	return readinputport(0) & 0x0f;
}

static READ_HANDLER( ca1_0_r )
{
	return (readinputport(0) & 0x10) >> 4;	/* coin A */
}

static READ_HANDLER( ca2_0_r )
{
	return (readinputport(0) & 0x20) >> 5;	/* coin B */
}

static WRITE_HANDLER( portb_0_w )
{
	/* this is the coin counter output, however it is controlled by changing
	   the PIA DDR (FF/DF) so we don't have a way to know which is which
	   because we always get a 00 write. */
}



static READ_HANDLER( portb_1_r )
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "%04x: read DIP\n",activecpu_get_pc());
	return readinputport(1);
}

static WRITE_HANDLER( ca2_1_w )
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "76477 #0 VCO SEL = %d\n",data & 1);
}

static WRITE_HANDLER( cb2_1_w )
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "DIP tristate %sactive\n",(data & 1) ? "in" : "");
}

static struct pia6821_interface pia0_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ porta_0_r, 0, ca1_0_r, 0, ca2_0_r, 0,
	/*outputs: A/B,CA/B2       */ 0, portb_0_w, 0, 0,
	/*irqs   : A/B             */ 0, 0,
};

static struct pia6821_interface pia1_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ 0, portb_1_r, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, 0, ca2_1_w, cb2_1_w,
	/*irqs   : A/B             */ 0, 0,
};

static struct pia6821_interface pia2_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, 0, 0, 0,
	/*irqs   : A/B             */ 0, 0,
};

MACHINE_INIT( toratora )
{
	pia_unconfig();
	pia_config(0, PIA_STANDARD_ORDERING, &pia0_intf);
	pia_config(1, PIA_STANDARD_ORDERING, &pia1_intf);
	pia_config(2, PIA_STANDARD_ORDERING, &pia2_intf);
	pia_reset();
}


static int timer;

INTERRUPT_GEN( toratora_timer )
{
	timer++;	/* timer counting at 16 Hz */

	/* also, when the timer overflows (16 seconds) watchdog would kick in */
	if (timer & 0x100) usrintf_showmessage("watchdog!");
}

static READ_HANDLER( toratora_timer_r )
{
	return timer;
}

static WRITE_HANDLER( toratora_clear_timer_w )
{
	timer = 0;
}



static MEMORY_READ_START( readmem )
	{ 0x0000, 0x0fff, MRA_RAM },
	{ 0x1000, 0x2fff, MRA_ROM },
	{ 0x8000, 0x9fff, MRA_RAM },
	{ 0xf04b, 0xf04b, toratora_timer_r },
	{ 0xf0a0, 0xf0a3, pia_0_r },
	{ 0xf0a4, 0xf0a7, pia_1_r },
	{ 0xf0a8, 0xf0ab, pia_2_r },
	{ 0xf800, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x0fff, MWA_RAM },
	{ 0x1000, 0x2fff, MWA_ROM },
	{ 0x8000, 0x9fff, toratora_videoram_w, &videoram },
	{ 0xf04a, 0xf04a, toratora_clear_tv_w },
	{ 0xf04b, 0xf04b, toratora_clear_timer_w },
	{ 0xf0a0, 0xf0a3, pia_0_w },
	{ 0xf0a4, 0xf0a7, pia_1_w },
	{ 0xf0a8, 0xf0ab, pia_2_w },
	{ 0xf800, 0xffff, MWA_ROM },
MEMORY_END



INPUT_PORTS_START( toratora )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "1" )
	PORT_DIPSETTING(    0x02, "0" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x04, "1" )
	PORT_DIPSETTING(    0x02, "0" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x20, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END



static MACHINE_DRIVER_START( toratora )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6800,500000)	/* ?????? game speed is entirely controlled by this */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(toratora_interrupt,1)
	MDRV_CPU_PERIODIC_INT(toratora_timer,16)	/* timer counting at 16 Hz */

	MDRV_MACHINE_INIT(toratora)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0,256-1,8,248-1)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)

	MDRV_VIDEO_START(generic_bitmapped)
	MDRV_VIDEO_UPDATE(generic_bitmapped)

	/* sound hardware */
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( toratora )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "tora.u1",      0x1000, 0x0800, CRC(413c743a) SHA1(a887dfaaee557327a1699bb424488b934dab8612) )
	ROM_LOAD( "tora.u10",     0x1800, 0x0800, CRC(dc771b1c) SHA1(1bd81decb4d0a854878227c52d45ac0eea0602ec) )
	ROM_LOAD( "tora.u2",      0x2000, 0x0800, CRC(c574c664) SHA1(9f41a53ca51d04e5bec7525fe83c5f4bdfcf128d) )
	ROM_LOAD( "tora.u9",      0x2800, 0x0800, CRC(b67aa11f) SHA1(da9e77255640a4b32eed2be89b686b98a248bd72) )
	ROM_LOAD( "tora.u11",     0xf800, 0x0800, CRC(55135d6f) SHA1(c48f180a9d6e894aafe87b2daf74e9a082f4600e) )
ROM_END



GAMEX( 1980, toratora, 0, toratora, toratora, 0, ROT90, "GamePlan", "Tora Tora", GAME_NO_SOUND )
