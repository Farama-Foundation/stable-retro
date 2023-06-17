/****

New York, New York (c) 1980 Sigma


memory map main cpu (m6809)

fedcba98
--------
000xxxxx  we1   $0000 8k (bitmap)
100xxxxx  we1   $8000 8k (ram)

010xxxxx  we2   $4000 8k (bitmap)
110xxxxx  we2   $C000 8k (ram)

001xxxxx  we3   $2000 16k x 3bits (colour)

011xxxxx  we4   $6000 16k x 3bits (colour)

10100000  SRAM  $A000
10100001  CRTC  $A100
10100010  PIA   $A200
10100011  SOUND $A300 one latch for read one for write same address

10101xxx  ROM7  $A800
10110xxx  ROM6  $B000
10111xxx  ROM5  $B800

11100xxx  ROM4  $E000
11101xxx  ROM3  $E800
11110xxx  ROM2  $F000
11111xxx  ROM1  $F800

****/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/6821pia.h"
#include "vidhrdw/crtc6845.h"
#include "cpu/m6800/m6800.h"
#include "cpu/m6809/m6809.h"

PALETTE_INIT( nyny );
VIDEO_UPDATE( nyny );
VIDEO_START( nyny );

unsigned char *nyny_videoram ;
unsigned char *nyny_colourram ;


static unsigned char pia1_ca1 = 0 ;
static unsigned char dac_volume = 0 ;
static unsigned char dac_enable = 0 ;

READ_HANDLER( nyny_videoram0_r );
WRITE_HANDLER( nyny_videoram0_w );
READ_HANDLER( nyny_videoram1_r );
WRITE_HANDLER( nyny_videoram1_w );

READ_HANDLER( nyny_colourram0_r );
WRITE_HANDLER( nyny_colourram0_w );
READ_HANDLER( nyny_colourram1_r );
WRITE_HANDLER( nyny_colourram1_w );
WRITE_HANDLER( nyny_flipscreen_w ) ;



INTERRUPT_GEN( nyny_interrupt )
{
	/* this is not accurate */
	/* pia1_ca1 should be toggled by output of LS123 */
	pia1_ca1 ^= 0x80 ;

	/* update for coin irq */
	pia_0_ca1_w(0,input_port_5_r(0)&0x01);
	pia_0_ca2_w(0,input_port_6_r(0)&0x01);

	cpu_set_irq_line(0, 0, HOLD_LINE);
}

/***************************************************************************
	6821 PIA handlers
***************************************************************************/

void cpu0_irq(int state)
{
	cpu_set_irq_line(0,M6809_IRQ_LINE,state ? ASSERT_LINE : CLEAR_LINE);
}


static READ_HANDLER( pia1_ca1_r )
{
	return pia1_ca1;
}


static WRITE_HANDLER( pia1_porta_w )
{
	/* bits 0-7 control a timer (low 8 bits) - is this for a starfield? */
}

static WRITE_HANDLER( pia1_portb_w )
{
	/* bits 0-3 control a timer (high 4 bits) - is this for a starfield? */
	/* bit 4 enables the starfield? */

	/* bits 5-7 go to the music board */
	soundlatch2_w(0,(data & 0x60) >> 5);
	cpu_set_irq_line(2,M6802_IRQ_LINE,(data & 0x80) ? CLEAR_LINE : ASSERT_LINE);
}

static struct pia6821_interface pia0_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ input_port_0_r, input_port_1_r, input_port_5_r, 0, input_port_6_r, 0,
	/*outputs: A/B,CA/B2       */ 0, 0, 0, 0,
	/*irqs   : A/B             */ cpu0_irq, 0
};

static struct pia6821_interface pia1_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ 0, 0, pia1_ca1_r, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ pia1_porta_w, pia1_portb_w, nyny_flipscreen_w, 0,
	/*irqs   : A/B             */ 0, 0
};

MACHINE_INIT( nyny )
{
	pia_unconfig();
	pia_config(0, PIA_STANDARD_ORDERING, &pia0_intf);
	pia_config(1, PIA_ALTERNATE_ORDERING, &pia1_intf);
	pia_reset();
}



WRITE_HANDLER( ay8910_porta_w )
{
	/* dac sounds like crap most likely bad implementation */
	dac_volume = data ;
	DAC_1_data_w( 0, dac_enable * dac_volume ) ;
}

WRITE_HANDLER( ay8910_portb_w )
{
	int v = (data & 7) << 5 ;
	DAC_0_data_w( 0, v ) ;

	dac_enable = ( data & 8 ) >> 3 ;
	DAC_1_data_w( 0, dac_enable * dac_volume ) ;
}

WRITE_HANDLER( shared_w_irq )
{
	soundlatch_w(0,data);
	cpu_set_irq_line(1,M6802_IRQ_LINE,HOLD_LINE);
}


static unsigned char snd_w = 0;

READ_HANDLER( snd_answer_r )
{
	return snd_w;
}

WRITE_HANDLER( snd_answer_w )
{
	snd_w = data;
}



static MEMORY_READ_START( readmem )
	{ 0x0000, 0x1fff, nyny_videoram0_r }, /* WE1 8k*/
	{ 0x2000, 0x3fff, nyny_colourram0_r }, /* WE3*/
	{ 0x4000, 0x5fff, nyny_videoram1_r }, /* WE2*/
	{ 0x6000, 0x7fff, nyny_colourram1_r }, /* WE4*/
	{ 0x8000, 0x9fff, MRA_RAM }, /* WE1 8k*/
	{ 0xa000, 0xa007, MRA_RAM }, /* SRAM*/
	{ 0xa204, 0xa207, pia_0_r },
	{ 0xa208, 0xa20b, pia_1_r },
	{ 0xa300, 0xa300, snd_answer_r },
	{ 0xa800, 0xbfff, MRA_ROM },
	{ 0xc000, 0xdfff, MRA_RAM }, /* WE2*/
	{ 0xe000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x1fff, nyny_videoram0_w }, /* WE1*/
	{ 0x2000, 0x3fff, nyny_colourram0_w }, /* WE3*/
	{ 0x4000, 0x5fff, nyny_videoram1_w }, /* WE2*/
	{ 0x6000, 0x7fff, nyny_colourram1_w }, /* WE4*/
	{ 0x8000, 0x9fff, MWA_RAM }, /* WE1*/
	{ 0xa000, 0xa007, MWA_RAM, &generic_nvram, &generic_nvram_size }, /* SRAM (coin counter, shown when holding F2)*/
	{ 0xa204, 0xa207, pia_0_w },
	{ 0xa208, 0xa20b, pia_1_w },
	{ 0xa300, 0xa300, shared_w_irq },
	{ 0xa100, 0xa100, crtc6845_address_w },
	{ 0xa101, 0xa101, crtc6845_register_w },
	{ 0xa800, 0xbfff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM }, /* WE2*/
	{ 0xe000, 0xffff, MWA_ROM },
MEMORY_END


static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x007f, MRA_RAM },
	{ 0x9001, 0x9001, soundlatch_r },
	{ 0xa000, 0xa001, input_port_4_r },
	{ 0xb000, 0xb000, AY8910_read_port_0_r },
	{ 0xb002, 0xb002, AY8910_read_port_1_r },
	{ 0xd000, 0xffff, MRA_ROM },
MEMORY_END


static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x007f, MWA_RAM },
	{ 0x9001, 0x9001, snd_answer_w },
	{ 0xb000, 0xb000, AY8910_write_port_0_w },
	{ 0xb001, 0xb001, AY8910_control_port_0_w },
	{ 0xb002, 0xb002, AY8910_write_port_1_w },
	{ 0xb003, 0xb003, AY8910_control_port_1_w },
	{ 0xd000, 0xffff, MWA_ROM },
MEMORY_END

static MEMORY_READ_START( sound2_readmem )
	{ 0x0000, 0x007f, MRA_RAM },
	{ 0x9000, 0x9000, soundlatch2_r },
	{ 0xa000, 0xa000, AY8910_read_port_2_r },
	{ 0xf800, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( sound2_writemem )
	{ 0x0000, 0x007f, MWA_RAM },
	{ 0xa000, 0xa000, AY8910_write_port_2_w },
	{ 0xa001, 0xa001, AY8910_control_port_2_w },
	{ 0xf800, 0xffff, MWA_ROM },
MEMORY_END



INPUT_PORTS_START( nyny )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 ) 	/* PIA0 PA0 */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_SERVICE1 )	/* PIA0 PA1 */
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )	/* PIA0 PA2 */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 )	/* PIA0 PA3 */
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL ) /* PIA0 PA4 */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START1 )	/* PIA0 PA5 */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )	/* PIA0 PA6 */

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL ) /* PIA0 PB0 */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_COCKTAIL ) /* PIA0 PB1 */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_2WAY )	/* PIA0 PB2 */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )	/* PIA0 PB3 */

	PORT_START	/* SW1 - port 2*/
	PORT_DIPNAME( 0x03, 0x03, "Bombs from UFO (scr 3+) " )
	PORT_DIPSETTING(	0x03, "9" )
	PORT_DIPSETTING(	0x02, "12" )
	PORT_DIPSETTING(	0x01, "3" )
	PORT_DIPSETTING(	0x00, "6" )
	PORT_DIPNAME( 0x04, 0x04, "Bombs from UFO (scr 1-2) " )
	PORT_DIPSETTING(	0x04, "6" )
	PORT_DIPSETTING(	0x00, "9" )
	PORT_DIPNAME( 0x80, 0x80, "Voice Volume " )
	PORT_DIPSETTING(	0x80, "High" )
	PORT_DIPSETTING(	0x00, "Low" )

	PORT_START	/* SW2 - port 3*/
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x18, "No Replays" )
	PORT_DIPSETTING(	0x10, "5000 Points" )
	PORT_DIPSETTING(	0x00, "10000 Points" )
	PORT_DIPSETTING(	0x08, "15000 Points" )
	PORT_DIPNAME( 0x40, 0x40, "Extra Missile Base")
	PORT_DIPSETTING(	0x00, "3000 Points" )
	PORT_DIPSETTING(	0x40, "5000 Points" )
	PORT_DIPNAME( 0x80, 0x80, "Extra Missile Mode" )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )
	PORT_DIPSETTING(	0x00, "No Extra Base" )

	PORT_START	/* SW3 - port 4*/
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x1c, 0x00, "Vertical Screen Position" )
	PORT_DIPSETTING(	0x00, "Neutral" )
	PORT_DIPSETTING(	0x04, "+1" )
	PORT_DIPSETTING(	0x08, "+2" )
	PORT_DIPSETTING(	0x0c, "+3" )
	PORT_DIPSETTING(	0x1c, "-1" )
	PORT_DIPSETTING(	0x18, "-2" )
	PORT_DIPSETTING(	0x14, "-3" )
	PORT_DIPNAME( 0xe0, 0x00, "Horizontal Screen Position" )
	PORT_DIPSETTING(	0x00, "Neutral" )
	PORT_DIPSETTING(	0x60, "+1" )
	PORT_DIPSETTING(	0x40, "+2" )
	PORT_DIPSETTING(	0x20, "+3" )
	PORT_DIPSETTING(	0xe0, "-1" )
	PORT_DIPSETTING(	0xc0, "-2" )
	PORT_DIPSETTING(	0xa0, "-3" )

	PORT_START  /* Connected to PIA1 CA1 input - port 5 */
	PORT_BIT( 0xFF, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START  /* Connected to PIA1 CA2 input - port 6 */
	PORT_BIT( 0xFF, IP_ACTIVE_HIGH, IPT_SERVICE1 )
INPUT_PORTS_END



static struct AY8910interface ay8910_interface =
{
	3,	/* 3 chips */
	1000000,	/* 1 MHz */
	{ 25, 25, 3 },
	{ 0, input_port_2_r, 0 },
	{ 0, input_port_3_r, 0 },
	{ ay8910_porta_w, 0 },
	{ ay8910_portb_w, 0 }
};

static struct DACinterface dac_interface =
{
	2,
	{ 25, 25 }
};



static MACHINE_DRIVER_START( nyny )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6809, 1400000)	/* 1.40 MHz */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(nyny_interrupt,2) /* game doesn't use video based irqs it's polling based */

	MDRV_CPU_ADD(M6802,4000000/4)	/* 1 MHz */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_CPU_ADD(M6802,4000000/4)	/* 1 MHz */
	MDRV_CPU_MEMORY(sound2_readmem,sound2_writemem)

	MDRV_FRAMES_PER_SECOND(50)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(nyny)
	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 255, 4, 251)	/* visible_area - just a guess */
	MDRV_PALETTE_LENGTH(8)

	MDRV_PALETTE_INIT(nyny)
	MDRV_VIDEO_START(nyny)
	MDRV_VIDEO_UPDATE(nyny)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
	MDRV_SOUND_ADD(DAC, dac_interface)
MACHINE_DRIVER_END

/***************************************************************************
  Game driver(s)
***************************************************************************/


ROM_START( nyny )
	ROM_REGION(0x10000, REGION_CPU1, 0)	/* 64k for code for the first CPU (Video) */
	ROM_LOAD( "nyny01s.100",  0xa800, 0x800, CRC(a2b76eca) SHA1(e46717e6ad330be4c4e7d9fab4f055f89aa31bcc) )
	ROM_LOAD( "nyny02s.099",  0xb000, 0x800, CRC(ef2d4dae) SHA1(718c0ecf7770a780aebb1dc8bf4ca86ea0a5ea28) )
	ROM_LOAD( "nyny03s.098",  0xb800, 0x800, CRC(2734c229) SHA1(b028d057d26838bae50b8ddb90a3755b5315b4ee) )
	ROM_LOAD( "nyny04s.097",  0xe000, 0x800, CRC(bd94087f) SHA1(02dde604bb84097fcd95c434847c55198b4e4309) )
	ROM_LOAD( "nyny05s.096",  0xe800, 0x800, CRC(248b22c4) SHA1(d64d89bf78fa19d36e02720c296a60621ab8fe21) )
	ROM_LOAD( "nyny06s.095",  0xf000, 0x800, CRC(8c073052) SHA1(0ce103ac0e79124ac9f1e097dda1a0664b92b89b) )
	ROM_LOAD( "nyny07s.094",  0xf800, 0x800, CRC(d49d7429) SHA1(c12eaae7ba0b1d44c45a584232db03c5731c046a) )

	ROM_REGION(0x10000, REGION_CPU2, 0)	/* 64k for code for the second CPU (sound) */
	ROM_LOAD( "nyny08.093",   0xd000, 0x800, CRC(19ddb6c3) SHA1(0097fad542f9a33849565093c2fb106d90007b1a) )
	ROM_RELOAD(               0xd800, 0x800 ) /*  needed high bit not wired */
	ROM_LOAD( "nyny09.092",   0xe000, 0x800, CRC(a359c6f1) SHA1(1bc7b487581399908c3cec823733810fb6d944ce) )
	ROM_RELOAD(               0xe800, 0x800 )
	ROM_LOAD( "nyny10.091",   0xf000, 0x800, CRC(a72a70fa) SHA1(deed7dec9cc43fa1d6c4854ba18169c894c9a2f0) )
	ROM_RELOAD(               0xf800, 0x800 )

	ROM_REGION(0x10000, REGION_CPU3, 0) 	/* 64k for code for the third CPU (sound) */
	ROM_LOAD( "nyny11.snd",   0xf800, 0x800, CRC(650450fc) SHA1(214693df394ca05eff5dbe1e800107d326ba80f6) )
ROM_END

ROM_START( nynyg )
	ROM_REGION(0x10000, REGION_CPU1, 0)	/* 64k for code for the first CPU (Video) */
	ROM_LOAD( "gny1.cpu",     0xa800, 0x800, CRC(fb5b8f17) SHA1(2202325451dfd4e7c16cba93f0fade46929ffa72) )
	ROM_LOAD( "gny2.cpu",     0xb000, 0x800, CRC(d248dd93) SHA1(0c4579698f8917332041c08af6902b8f8acd7d62) )
	ROM_LOAD( "gny3.cpu",     0xb800, 0x800, CRC(223a9d09) SHA1(c2b12270d375587489208d6a1b37a4e3ec87bc20) )
	ROM_LOAD( "gny4.cpu",     0xe000, 0x800, CRC(7964ec1f) SHA1(dba3dc2e928fb3fc04a9dca12951343669a4ecbe) )
	ROM_LOAD( "gny5.cpu",     0xe800, 0x800, CRC(4799dcfc) SHA1(13dcc4a58a029c14a4e9acd0bf584c71d5302c03) )
	ROM_LOAD( "gny6.cpu",     0xf000, 0x800, CRC(4839d4d2) SHA1(cfd6f2f252ee2f6a4d881496a017c02d7dd77944) )
	ROM_LOAD( "gny7.cpu",     0xf800, 0x800, CRC(b7564c5b) SHA1(e1d8fe7f37aa7aa98f18c538fe6e688675cc2de1) )

	ROM_REGION(0x10000, REGION_CPU2, 0)	/* 64k for code for the second CPU (sound) */
	ROM_LOAD( "gny8.cpu",     0xd000, 0x800, CRC(e0bf7d00) SHA1(7afca3affa413179f4f59ce2cad89525cfa5efbc) )
	ROM_RELOAD(               0xd800, 0x800 ) /* reload needed high bit not wired */
	ROM_LOAD( "gny9.cpu",     0xe000, 0x800, CRC(639bc81a) SHA1(91819d49099e438ac8c70920a787aeaed3ed82e9) )
	ROM_RELOAD(               0xe800, 0x800 )
	ROM_LOAD( "gny10.cpu",    0xf000, 0x800, CRC(73764021) SHA1(bb2f62130142487afbd8d2540e2d4fe5bb67c4ee) )
	ROM_RELOAD(               0xf800, 0x800 )

	ROM_REGION(0x10000, REGION_CPU3, 0) 	/* 64k for code for the third CPU (sound) */
	/* The original dump of this ROM was bad [FIXED BITS (x1xxxxxx)] */
	/* Since what's left is identical to the Sigma version, I'm assuming it's the same. */
	ROM_LOAD( "nyny11.snd",   0xf800, 0x800, CRC(650450fc) SHA1(214693df394ca05eff5dbe1e800107d326ba80f6) )
ROM_END

ROM_START( arcadia )
	ROM_REGION(0x10000, REGION_CPU1, 0)	/* 64k for code for the first CPU (Video) */
	ROM_LOAD( "ar-01",        0xa800, 0x800, CRC(7b7e8f27) SHA1(2bb1d07d87ad5b952de9460c840d7e8b59ed1b4a) )
	ROM_LOAD( "ar-02",        0xb000, 0x800, CRC(81d9e172) SHA1(4279582f1edf54f0974fa277565d8ade6d9faa50) )
	ROM_LOAD( "ar-03",        0xb800, 0x800, CRC(2c5feb05) SHA1(6f8952e7744ba7d7b8b345d67f546b504f7a3b30) )
	ROM_LOAD( "ar-04",        0xe000, 0x800, CRC(66fcbd7f) SHA1(7b8c09593b7d0d25cbe0b28097d58772c32f13bb) )
	ROM_LOAD( "ar-05",        0xe800, 0x800, CRC(b2320e20) SHA1(977afc2d26ef500eff4499e6bc61f14314b19130) )
	ROM_LOAD( "ar-06",        0xf000, 0x800, CRC(27b79cc0) SHA1(2c5c3a9a09069751c5e9c23d0840ee4996006c0b) )
	ROM_LOAD( "ar-07",        0xf800, 0x800, CRC(be77a477) SHA1(817c069855634dd844f0068d64bfbf1862980d6b) )

	ROM_REGION(0x10000, REGION_CPU2, 0)	/* 64k for code for the second CPU (sound) */
	ROM_LOAD( "ar-08",        0xd000, 0x800, CRC(38569b25) SHA1(887a9afaa65d0961097f7fb5f1ae390d40e9c164) )
	ROM_RELOAD(               0xd800, 0x800 ) /*  needed high bit not wired */
	ROM_LOAD( "nyny09.092",   0xe000, 0x800, CRC(a359c6f1) SHA1(1bc7b487581399908c3cec823733810fb6d944ce) )
	ROM_RELOAD(               0xe800, 0x800 )
	ROM_LOAD( "nyny10.091",   0xf000, 0x800, CRC(a72a70fa) SHA1(deed7dec9cc43fa1d6c4854ba18169c894c9a2f0) )
	ROM_RELOAD(               0xf800, 0x800 )

	ROM_REGION(0x10000, REGION_CPU3, 0) 	/* 64k for code for the third CPU (sound) */
	ROM_LOAD( "ar-11",        0xf800, 0x800, CRC(208f4488) SHA1(533f8942e1c964cc88253e9dc4ec711f77607e4c) )
ROM_END


GAMEX( 1980, nyny,    0,    nyny, nyny, 0, ROT270, "Sigma Enterprises Inc.", "New York New York", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAMEX( 1980, nynyg,   nyny, nyny, nyny, 0, ROT270, "Sigma Enterprises Inc. (Gottlieb license)", "New York New York (Gottlieb)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAMEX( 1980, arcadia, nyny, nyny, nyny, 0, ROT270, "Sigma Enterprises Inc.", "Waga Seishun no Arcadia", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
