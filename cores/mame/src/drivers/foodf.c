/***************************************************************************

	Atari Food Fight hardware

	driver by Aaron Giles

	Games supported:
		* Food Fight

	Known bugs:
		* none at this time

****************************************************************************

	Memory map

****************************************************************************

	========================================================================
	MAIN CPU
	========================================================================
	000000-00FFFF   R     xxxxxxxx xxxxxxxx   Program ROM
	014000-01BFFF   R/W   xxxxxxxx xxxxxxxx   Program RAM
	01C000-01CFFF   R/W   xxxxxxxx xxxxxxxx   Motion object RAM (1024 entries x 2 words)
	                R/W   x------- --------      (0: Horizontal flip)
	                R/W   -x------ --------      (0: Vertical flip)
	                R/W   ---xxxxx --------      (0: Palette select)
	                R/W   -------- xxxxxxxx      (0: Tile index)
	                R/W   xxxxxxxx --------      (1: X position)
	                R/W   -------- xxxxxxxx      (1: Y position)
	800000-8007FF   R/W   xxxxxxxx xxxxxxxx   Playfield RAM (32x32 tiles)
	                R/W   x------- --------      (Tile index MSB)
	                R/W   --xxxxxx --------      (Palette select)
	                R/W   -------- xxxxxxxx      (Tile index LSBs)
	900000-9001FF   R/W   -------- ----xxxx   NVRAM
	940000-940007   R     -------- xxxxxxxx   Analog input read
	944000-944007     W   -------- --------   Analog input select
	948000          R     -------- xxxxxxxx   Digital inputs
	                R     -------- x-------      (Self test)
	                R     -------- -x------      (Player 2 throw)
	                R     -------- --x-----      (Player 1 throw)
	                R     -------- ---x----      (Aux coin)
	                R     -------- ----x---      (2 player start)
	                R     -------- -----x--      (1 player start)
	                R     -------- ------x-      (Right coin)
	                R     -------- -------x      (Left coin)
	948000            W   -------- xxxxxxxx   Digital outputs
	                  W   -------- x-------      (Right coin counter)
	                  W   -------- -x------      (Left coin counter)
	                  W   -------- --x-----      (LED 2)
	                  W   -------- ---x----      (LED 1)
	                  W   -------- ----x---      (INT2 reset)
	                  W   -------- -----x--      (INT1 reset)
	                  W   -------- ------x-      (Update)
	                  W   -------- -------x      (Playfield flip)
	94C000            W   -------- --------   Unknown
	950000-9501FF     W   -------- xxxxxxxx   Palette RAM (256 entries)
	                  W   -------- xx------      (Blue)
	                  W   -------- --xxx---      (Green)
	                  W   -------- -----xxx      (Red)
	954000            W   -------- --------   NVRAM recall
	958000            W   -------- --------   Watchdog
	A40000-A4001F   R/W   -------- xxxxxxxx   POKEY 2
	A80000-A8001F   R/W   -------- xxxxxxxx   POKEY 1
	AC0000-AC001F   R/W   -------- xxxxxxxx   POKEY 3
	========================================================================
	Interrupts:
		IRQ1 = 32V
		IRQ2 = VBLANK
	========================================================================


***************************************************************************/


#include "driver.h"
#include "machine/atarigen.h"
#include "vidhrdw/generic.h"
#include "foodf.h"
#include "bootstrap.h"
#include "inptport.h"



/*************************************
 *
 *	Statics
 *
 *************************************/

static UINT8 whichport = 0;



/*************************************
 *
 *	NVRAM
 *
 *************************************/

static READ16_HANDLER( nvram_r )
{
	return ((data16_t *)generic_nvram)[offset] | 0xfff0;
}



/*************************************
 *
 *	Interrupts
 *
 *************************************/

static void update_interrupts(void)
{
	int newstate = 0;

	if (atarigen_scanline_int_state)
		newstate |= 1;
	if (atarigen_video_int_state)
		newstate |= 2;

	if (newstate)
		cpu_set_irq_line(0, newstate, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 7, CLEAR_LINE);
}


static void scanline_update(int scanline)
{
	/* INT 1 is on 32V */
	if (scanline & 32)
		atarigen_scanline_int_gen();
}


static MACHINE_INIT( foodf )
{
	atarigen_interrupt_reset(update_interrupts);
	atarigen_scanline_timer_reset(scanline_update, 32);
}



/*************************************
 *
 *	Digital outputs
 *
 *************************************/

static WRITE16_HANDLER( digital_w )
{
	if (ACCESSING_LSB)
	{
		foodf_set_flip(data & 0x01);

		if (!(data & 0x04))
			atarigen_scanline_int_ack_w(0,0,0);
		if (!(data & 0x08))
			atarigen_video_int_ack_w(0,0,0);

		coin_counter_w(0, (data >> 6) & 1);
		coin_counter_w(1, (data >> 7) & 1);
	}
}



/*************************************
 *
 *	Analog inputs
 *
 *************************************/

static UINT8 position_update ( UINT8 temp )
{
  if (temp > 127) {
    if (temp > 190) return 190;
    else return temp; }

  else {
    if (temp < 65) return 65;
    else return temp; }
}

static READ16_HANDLER( analog_r )
{

 /****************************************************
  * Live Center - rev 5 - by mahoneyt944 & grant2258 *
  ****************************************************/

  static UINT8 currentx  = 0x80; /* upright */
  static UINT8 currenty  = 0x7F;
  static UINT8 currentx2 = 0x80; /* cocktail */
  static UINT8 currenty2 = 0x7F;
  static UINT8 delay     = 0;    /* debounce counter */
  static UINT8 t         = 0;    /* debounce count to reach */
  UINT8  center          = 0;    /* reset center checks */
  UINT8  center2         = 0;


  /* live center dip switch set -> On */
  if (readinputport(6) == 0x01)
  {
    /* user set debounce delay from dip menu */
    if (readinputport(7) == 0xFF) t = 0;
    else t = readinputport(7);
    if (delay > t) delay = 0;

    /* check for centers */
    if (readinputport(0) == 0x7F && readinputport(2) == 0x7F) center = 1;
    if (readinputport(1) == 0x7F && readinputport(3) == 0x7F) center2 = 1;

    if (delay == t) /* debounce protection */
    {
      delay = 0;

      if (center == 0)
      { /* update upright stopping positions */
        currentx = position_update(readinputport(0));
        currenty = position_update(readinputport(2));
      }

      if (center2 == 0)
      { /* update cocktail stopping positions */
        currentx2 = position_update(readinputport(1));
        currenty2 = position_update(readinputport(3));
      }
    }
    else delay++;

    /* return upright x stopping position */
    if (whichport == 0)
    { if (center) return currentx; }

    /* return upright y stopping position */
    else if (whichport == 2)
    { if (center) return currenty; }

    /* return cocktail x stopping position */
    else if (whichport == 1)
    { if (center2) return currentx2; }

    /* return cocktail y stopping position */
    else if (whichport == 3)
    { if (center2) return currenty2; }
  }

  /* return actual input  */
  return readinputport(whichport);
}


static WRITE16_HANDLER( analog_w )
{
	whichport = offset ^ 3;
}



/*************************************
 *
 *	POKEY I/O
 *
 *************************************/

static READ16_HANDLER( pokey1_word_r ) { return pokey1_r(offset); }
static READ16_HANDLER( pokey2_word_r ) { return pokey2_r(offset); }
static READ16_HANDLER( pokey3_word_r ) { return pokey3_r(offset); }

static WRITE16_HANDLER( pokey1_word_w ) { if (ACCESSING_LSB) pokey1_w(offset, data & 0xff); }
static WRITE16_HANDLER( pokey2_word_w ) { if (ACCESSING_LSB) pokey2_w(offset, data & 0xff); }
static WRITE16_HANDLER( pokey3_word_w ) { if (ACCESSING_LSB) pokey3_w(offset, data & 0xff); }



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ16_START( readmem )
	{ 0x000000, 0x00ffff, MRA16_ROM },
	{ 0x014000, 0x01cfff, MRA16_RAM },
	{ 0x800000, 0x8007ff, MRA16_RAM },
	{ 0x900000, 0x9001ff, nvram_r },
	{ 0x940000, 0x940007, analog_r },
	{ 0x948000, 0x948001, input_port_4_word_r },
	{ 0x94c000, 0x94c001, MRA16_NOP }, /* Used from PC 0x776E */
	{ 0x958000, 0x958001, watchdog_reset16_r },
	{ 0xa40000, 0xa4001f, pokey2_word_r },
	{ 0xa80000, 0xa8001f, pokey1_word_r },
	{ 0xac0000, 0xac001f, pokey3_word_r },
MEMORY_END


static MEMORY_WRITE16_START( writemem )
	{ 0x000000, 0x00ffff, MWA16_ROM },
	{ 0x014000, 0x01bfff, MWA16_RAM },
	{ 0x01c000, 0x01cfff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x800000, 0x8007ff, atarigen_playfield_w, &atarigen_playfield },
	{ 0x900000, 0x9001ff, MWA16_RAM, (data16_t **)&generic_nvram, &generic_nvram_size },
	{ 0x944000, 0x944007, analog_w },
	{ 0x948000, 0x948001, digital_w },
	{ 0x950000, 0x9501ff, foodf_paletteram_w, &paletteram16 },
	{ 0x954000, 0x954001, MWA16_NOP },
	{ 0x958000, 0x958001, watchdog_reset16_w },
	{ 0xa40000, 0xa4001f, pokey2_word_w },
	{ 0xa80000, 0xa8001f, pokey1_word_w },
	{ 0xac0000, 0xac001f, pokey3_word_w },
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( foodf )
	PORT_START	/* IN0 */
	PORT_ANALOG( 0xff, 0x7f, IPT_AD_STICK_X | IPF_PLAYER1 | IPF_REVERSE, 100, 100, 0, 255 )

	PORT_START	/* IN1 */
	PORT_ANALOG( 0xff, 0x7f, IPT_AD_STICK_X | IPF_PLAYER2 | IPF_REVERSE | IPF_COCKTAIL, 100, 100, 0, 255 )

	PORT_START	/* IN2 */
	PORT_ANALOG( 0xff, 0x7f, IPT_AD_STICK_Y | IPF_PLAYER1 | IPF_REVERSE, 100, 100, 0, 255 )

	PORT_START	/* IN3 */
	PORT_ANALOG( 0xff, 0x7f, IPT_AD_STICK_Y | IPF_PLAYER2 | IPF_REVERSE | IPF_COCKTAIL, 100, 100, 0, 255 )

	PORT_START	/* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	/* SW1 */
	PORT_DIPNAME( 0x07, 0x00, "Bonus Coins" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPSETTING(    0x05, "1 for every 2" )
	PORT_DIPSETTING(    0x02, "1 for every 4" )
	PORT_DIPSETTING(    0x01, "1 for every 5" )
	PORT_DIPSETTING(    0x06, "2 for every 4" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Coin_A ))
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ))
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ))
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Coin_B ))
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ))
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ))
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_5C ))
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_6C ))
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coinage ))
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ))
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ))
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ))
	PORT_DIPSETTING(    0x40, DEF_STR( Free_Play ))
	
	PORT_START	/* Port #6 - Toggle for Live Center */
	PORT_DIPNAME( 0x01, 0x00, "Live Center" )
	PORT_DIPSETTING(    0x01, "On" )
	PORT_DIPSETTING(    0x00, "Off" )
	
	PORT_START	/* Port #7 - Settings for Debounce Delay */
	PORT_DIPNAME( 0xFF, 0xFF, "Debounce Delay" )
	PORT_DIPSETTING(    0xFF, "0" )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x05, "5" )
	PORT_DIPSETTING(    0x06, "6" )
	PORT_DIPSETTING(    0x07, "7" )
	PORT_DIPSETTING(    0x08, "8" )
	PORT_DIPSETTING(    0x09, "9" )
	PORT_DIPSETTING(    0x0A, "10")

INPUT_PORTS_END



/*************************************
 *
 *	Graphics definitions
 *
 *************************************/

static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	2,
	{ 0, 4 },
	{ 8*8+0, 8*8+1, 8*8+2, 8*8+3, 0, 1, 2, 3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*16
};


static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,2),
	2,
	{ RGN_FRAC(1,2), 0 },
	{ 8*16+0, 8*16+1, 8*16+2, 8*16+3, 8*16+4, 8*16+5, 8*16+6, 8*16+7, 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	8*32
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,   0, 64 },	/* characters 8x8 */
	{ REGION_GFX2, 0, &spritelayout, 0, 64 },	/* sprites & playfield */
	{ -1 }
};



/*************************************
 *
 *	Sound definitions
 *
 *************************************/

static READ_HANDLER( pot_r )
{
	return (readinputport(5) >> offset) << 7;
}

static struct POKEYinterface pokey_interface =
{
	3,
	600000,
	{ 33, 33, 33 },
	/* The 8 pot handlers */
	{ pot_r, 0, 0 },
	{ pot_r, 0, 0 },
	{ pot_r, 0, 0 },
	{ pot_r, 0, 0 },
	{ pot_r, 0, 0 },
	{ pot_r, 0, 0 },
	{ pot_r, 0, 0 },
	{ pot_r, 0, 0 },
	/* The allpot handler */
	{ 0, 0, 0 }
};



/*************************************
 *
 *	Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( foodf )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 6000000)
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(atarigen_video_int_gen,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(foodf)
	MDRV_NVRAM_HANDLER(generic_1fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_START(foodf)
	MDRV_VIDEO_UPDATE(foodf)

	/* sound hardware */
	MDRV_SOUND_ADD(POKEY, pokey_interface)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definition(s)
 *
 *************************************/

ROM_START( foodf )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for 68000 code */
	ROM_LOAD16_BYTE( "301-8c.020",   0x000001, 0x002000, CRC(dfc3d5a8) SHA1(7abe5e9c27098bd8c93cc06f1b9e3db0744019e9) )
	ROM_LOAD16_BYTE( "302-9c.020",   0x000000, 0x002000, CRC(ef92dc5c) SHA1(eb41291615165f549a68ebc6d4664edef1a04ac5) )
	ROM_LOAD16_BYTE( "303-8d.020",   0x004001, 0x002000, CRC(64b93076) SHA1(efa4090d96aa0ffd4192a045f174ac5960810bca) )
	ROM_LOAD16_BYTE( "304-9d.020",   0x004000, 0x002000, CRC(ea596480) SHA1(752aa33a8e8045650dd32ec7c7026e00d7896e0f) )
	ROM_LOAD16_BYTE( "305-8e.020",   0x008001, 0x002000, CRC(e6cff1b1) SHA1(7c7ad2dcdff60fc092e8a825c5a6de6b506523de) )
	ROM_LOAD16_BYTE( "306-9e.020",   0x008000, 0x002000, CRC(95159a3e) SHA1(f180126671776f62242ec9fd4a82a581c551ffce) )
	ROM_LOAD16_BYTE( "307-8f.020",   0x00c001, 0x002000, CRC(17828dbb) SHA1(9d8e29a5e56a8a9c5db8561e4c20ff22f69b46ca) )
	ROM_LOAD16_BYTE( "308-9f.020",   0x00c000, 0x002000, CRC(608690c9) SHA1(419020c69ce6fded0d9af44ead8ec4727468d58b) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "109-6lm.020",  0x000000, 0x002000, CRC(c13c90eb) SHA1(ebd2bbbdd7e184851d1ab4b5648481d966c78cc2) )

	ROM_REGION( 0x4000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "110-4d.020",   0x000000, 0x002000, CRC(8870e3d6) SHA1(702007d3d543f872b5bf5d00b49f6e05b46d6600) )
	ROM_LOAD( "111-4e.020",   0x002000, 0x002000, CRC(84372edf) SHA1(9beef3ff3b28405c45d691adfbc233921073be47) )
ROM_END


ROM_START( foodf2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for 68000 code */
	ROM_LOAD16_BYTE( "201-8c.020",   0x000001, 0x002000, CRC(4ee52d73) SHA1(ff4ab8169a9b260bbd1f49023a30064e2f0b6686) )
	ROM_LOAD16_BYTE( "202-9c.020",   0x000000, 0x002000, CRC(f8c4b977) SHA1(824d33baa413b2ee898c75157624ea007c92032f) )
	ROM_LOAD16_BYTE( "203-8d.020",   0x004001, 0x002000, CRC(0e9f99a3) SHA1(37bba66957ee19e7d05fcc3e4583e909809075ed) )
	ROM_LOAD16_BYTE( "204-9d.020",   0x004000, 0x002000, CRC(f667374c) SHA1(d7be70b56500e2071b7f8c810f7a3e2a6743c6bd) )
	ROM_LOAD16_BYTE( "205-8e.020",   0x008001, 0x002000, CRC(1edd05b5) SHA1(cc712a11946c103eaa808c86e15676fde8610ad9) )
	ROM_LOAD16_BYTE( "206-9e.020",   0x008000, 0x002000, CRC(bb8926b3) SHA1(95c6ba8ac6b56d1a67a47758b71712d55a959cd0) )
	ROM_LOAD16_BYTE( "207-8f.020",   0x00c001, 0x002000, CRC(c7383902) SHA1(f76e2c95fccd0cafff9346a32e0c041c291a6696) )
	ROM_LOAD16_BYTE( "208-9f.020",   0x00c000, 0x002000, CRC(608690c9) SHA1(419020c69ce6fded0d9af44ead8ec4727468d58b) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "109-6lm.020",  0x000000, 0x002000, CRC(c13c90eb) SHA1(ebd2bbbdd7e184851d1ab4b5648481d966c78cc2) )

	ROM_REGION( 0x4000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "110-4d.020",   0x000000, 0x002000, CRC(8870e3d6) SHA1(702007d3d543f872b5bf5d00b49f6e05b46d6600) )
	ROM_LOAD( "111-4e.020",   0x002000, 0x002000, CRC(84372edf) SHA1(9beef3ff3b28405c45d691adfbc233921073be47) )
ROM_END


ROM_START( foodfc )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for 68000 code */
	ROM_LOAD16_BYTE( "113-8c.020",   0x000001, 0x002000, CRC(193a299f) SHA1(58bbf714eff22d8a47b174e4b121f14a8dcb4ef9) )
	ROM_LOAD16_BYTE( "114-9c.020",   0x000000, 0x002000, CRC(33ed6bbe) SHA1(5d80fb092d2964b851e6c5982572d4ffc5078c55) )
	ROM_LOAD16_BYTE( "115-8d.020",   0x004001, 0x002000, CRC(64b93076) SHA1(efa4090d96aa0ffd4192a045f174ac5960810bca) )
	ROM_LOAD16_BYTE( "116-9d.020",   0x004000, 0x002000, CRC(ea596480) SHA1(752aa33a8e8045650dd32ec7c7026e00d7896e0f) )
	ROM_LOAD16_BYTE( "117-8e.020",   0x008001, 0x002000, CRC(12a55db6) SHA1(508f02c72074a0e3300ec32c181e4f72cbc4245f) )
	ROM_LOAD16_BYTE( "118-9e.020",   0x008000, 0x002000, CRC(e6d451d4) SHA1(03bfa932ed419572c08942ad159288b38d24d90f) )
	ROM_LOAD16_BYTE( "119-8f.020",   0x00c001, 0x002000, CRC(455cc891) SHA1(9f7764c15dea7568326860b910686fec644c42c2) )
	ROM_LOAD16_BYTE( "120-9f.020",   0x00c000, 0x002000, CRC(34173910) SHA1(19e6032c22d20410386516ffc1a809ae50431c65) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "109-6lm.020",  0x000000, 0x002000, CRC(c13c90eb) SHA1(ebd2bbbdd7e184851d1ab4b5648481d966c78cc2) )

	ROM_REGION( 0x4000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "110-4d.020",   0x000000, 0x002000, CRC(8870e3d6) SHA1(702007d3d543f872b5bf5d00b49f6e05b46d6600) )
	ROM_LOAD( "111-4e.020",   0x002000, 0x002000, CRC(84372edf) SHA1(9beef3ff3b28405c45d691adfbc233921073be47) )
ROM_END



/*************************************
 *
 *	Game driver(s)
 *
 *************************************/

GAMEC( 1982, foodf,  0,     foodf, foodf, 0, ROT0, "Atari", "Food Fight (rev 3)",    &foodf_ctrl, &foodf_bootstrap  )
GAMEC( 1982, foodf2, foodf, foodf, foodf, 0, ROT0, "Atari", "Food Fight (rev 2)",    &foodf_ctrl, &foodf_bootstrap  )
GAMEC( 1982, foodfc, foodf, foodf, foodf, 0, ROT0, "Atari", "Food Fight (cocktail)", &foodf_ctrl, &foodfc_bootstrap )
