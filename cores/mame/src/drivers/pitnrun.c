/****************************************************
   Pit&Run - Taito 1984

 driver by  Tomasz Slanina and	Pierpaolo Prazzoli


TODO:

 - analog sound
   writes to $a8xx triggering analog sound :
   	$a800 - drivers are gettin into the cars
   	$a801 - collisions
   	$a802 - same as above
   	$a803 - slide on water
   	$a804 - accelerate
   	$a807 - analog sound reset


-----------------------------------------------------
$8101 B - course
$8102 B - trial
$8492 B - fuel
$84f6 B - lap
$84c1 W - time
-----------------------------------------------------

N4200374A

K1000232A
            A11_17     2128  PR9
           (68705P5)         PR10
                             PR11
     SW1                     PR12
                        Z80
                                      clr.1
                        PR8           clr.2

               PR6                    clr.3
               PR7
                              2114
                              2114

K1000231A

    2114 2114
    PR13
                Z80

          8910 8910
   5MHz

K1000233A

  2125      2125        2128
  2125      2125        2128
  2125      2125        PR4
  2125      2125        PR5
  2125      2125

                             2114
     PR1                     2114
     PR2
     PR3
*/

#include "driver.h"
#include "vidhrdw/generic.h"

WRITE_HANDLER (pitnrun_68705_portA_w);
WRITE_HANDLER (pitnrun_68705_portB_w);

READ_HANDLER (pitnrun_68705_portA_r);
READ_HANDLER (pitnrun_68705_portB_r);
READ_HANDLER (pitnrun_68705_portC_r);

MACHINE_INIT( pitnrun );

READ_HANDLER( pitnrun_mcu_data_r );
READ_HANDLER( pitnrun_mcu_status_r );
WRITE_HANDLER( pitnrun_mcu_data_w );

extern UINT8* videoram2;

WRITE_HANDLER( pitnrun_videoram_w );
WRITE_HANDLER( pitnrun_videoram2_w );
WRITE_HANDLER(pitnrun_ha_w);
WRITE_HANDLER(pitnrun_h_heed_w);
WRITE_HANDLER(pitnrun_v_heed_w);
WRITE_HANDLER(pitnrun_color_select_w);
WRITE_HANDLER( pitnrun_char_bank_select );
WRITE_HANDLER( pitnrun_scroll_w );
WRITE_HANDLER( pitnrun_scroll_y_w );
READ_HANDLER( pitnrun_videoram_r );
READ_HANDLER( pitnrun_videoram2_r );

PALETTE_INIT(pitnrun);
VIDEO_START(pitnrun);
VIDEO_UPDATE(pitnrun);

static int pitnrun_nmi;

static INTERRUPT_GEN( pitnrun_nmi_source )
{
	 if(pitnrun_nmi) cpu_set_irq_line(0,IRQ_LINE_NMI, PULSE_LINE)	;
}

static WRITE_HANDLER( nmi_enable_w )
{
        pitnrun_nmi=data&1;
}

static WRITE_HANDLER(pitnrun_hflip_w)
{
	flip_screen_x_set(data);
}

static WRITE_HANDLER(pitnrun_vflip_w)
{
	flip_screen_y_set(data);
}

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM},
	{ 0x8800, 0x8fff, pitnrun_videoram_r},
	{ 0x9000, 0x9fff, pitnrun_videoram2_r },
	{ 0xa000, 0xa0ff, spriteram_r },
	{ 0xa800, 0xa800, input_port_0_r },
	{ 0xb000, 0xb000, input_port_1_r },
	{ 0xb800, 0xb800, input_port_2_r },
	{ 0xd800, 0xd800, pitnrun_mcu_status_r},
	{ 0xd000, 0xd000, pitnrun_mcu_data_r },
	{ 0xf000, 0xf000, watchdog_reset_r},
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x8800, 0x8fff, pitnrun_videoram_w, &videoram ,&videoram_size },
	{ 0x9000, 0x9fff, pitnrun_videoram2_w, &videoram2 },
	{ 0xa000, 0xa0ff, spriteram_w, &spriteram, &spriteram_size },
	{ 0xa800, 0xa807, MWA_NOP }, /* Analog Sound */
	{ 0xb000, 0xb000, nmi_enable_w },
	{ 0xb001, 0xb001, pitnrun_color_select_w },
	{ 0xb004, 0xb004, MWA_NOP },/* COLOR SEL 2 - not used ?*/
	{ 0xb005, 0xb005, pitnrun_char_bank_select},
	{ 0xb006, 0xb006, pitnrun_hflip_w},
	{ 0xb007, 0xb007, pitnrun_vflip_w},
	{ 0xb800, 0xb800, soundlatch_w },
	{ 0xc800, 0xc801, pitnrun_scroll_w },
	{ 0xc802, 0xc802, pitnrun_scroll_y_w },
	{ 0xc804, 0xc804, pitnrun_mcu_data_w },
	{ 0xc805, 0xc805, pitnrun_h_heed_w },
 	{ 0xc806, 0xc806, pitnrun_v_heed_w },
	{ 0xc807, 0xc807, pitnrun_ha_w },
MEMORY_END

static PORT_WRITE_START( sound_writeport )
	{ 0x00, 0x00, soundlatch_clear_w },
	{ 0x8c, 0x8c, AY8910_control_port_1_w },
	{ 0x8d, 0x8d, AY8910_write_port_1_w   },
	{ 0x8e, 0x8e, AY8910_control_port_0_w },
	{ 0x8f, 0x8f, AY8910_write_port_0_w   },
	{ 0x90, 0x96, MWA_NOP },
	{ 0x97, 0x97, MWA_NOP },
	{ 0x98, 0x98, MWA_NOP },
PORT_END

static PORT_READ_START( sound_readport )
	{ 0x8f, 0x8f, AY8910_read_port_0_r },
PORT_END


static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x2fff, MRA_ROM },
	{ 0x3800, 0x3bff, MRA_RAM },

MEMORY_END


static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x2fff, MWA_ROM },
	{ 0x3800, 0x3bff, MWA_RAM },
MEMORY_END

static MEMORY_READ_START( mcu_readmem )
	{ 0x0000, 0x0000, pitnrun_68705_portA_r },
	{ 0x0001, 0x0001, pitnrun_68705_portB_r },
	{ 0x0002, 0x0002, pitnrun_68705_portC_r },
	{ 0x0003, 0x007f, MRA_RAM },
	{ 0x0080, 0x07ff, MRA_ROM },
MEMORY_END


static MEMORY_WRITE_START( mcu_writemem )
	{ 0x0000, 0x0000, pitnrun_68705_portA_w },
	{ 0x0001, 0x0001, pitnrun_68705_portB_w },
	{ 0x0003, 0x007f, MWA_RAM },
	{ 0x0080, 0x07ff, MWA_ROM },
MEMORY_END

INPUT_PORTS_START( pitnrun )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1  )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START		/* DSW0 */
	PORT_DIPNAME( 0x07, 0x01, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_7C ) )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0x20, 0x00, "Gasoline Count" )
	PORT_DIPSETTING(    0x00, "10 Up or 10 Down" )
	PORT_DIPSETTING(    0x20, "20 Up or 20 Down" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Cocktail ) )
	PORT_BITX(    0x80, 0x00, IPT_DIPSWITCH_NAME | IPF_CHEAT, "No Hit", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START	/* IN 1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT    | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( jumpkun )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1  )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	
	PORT_START
	PORT_DIPNAME( 0x07, 0x01, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_7C ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x00, "Invincibility (Cheat)")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END


static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,3),
	3,
	{ 0,RGN_FRAC(1,3), RGN_FRAC(2,3) },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
	  8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8
};

static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,2),
	4,
	{RGN_FRAC(1,2),RGN_FRAC(1,2)+4,0,4},
	{ STEP4(0,1), STEP4(8,1) },
	{ STEP8(0,8*2) },
	8*8*2
};

static struct AY8910interface ay8910_interface =
{
	2,
	18432000/12,
	{ 50, 50 },
	{ soundlatch_r, soundlatch_r },
	{ soundlatch_r, soundlatch_r },
	{ 0, 0 },
	{ 0, 0 }
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX3, 0, &charlayout,   64,2 },
	{ REGION_GFX2, 0, &charlayout,   32,2 },
	{ REGION_GFX1, 0, &spritelayout,   0, 4},
	{ -1 }
};

static MACHINE_DRIVER_START( pitnrun )
	MDRV_CPU_ADD(Z80,8000000/2)		 /* ? MHz */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(pitnrun_nmi_source,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_CPU_ADD(Z80, 10000000/4)     /* 2.5 MHz */

	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PORTS(sound_readport,sound_writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(M68705,2000000)
	MDRV_CPU_MEMORY(mcu_readmem,mcu_writemem)

	MDRV_MACHINE_INIT(pitnrun)

	MDRV_INTERLEAVE(100)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)

	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32*3)
	MDRV_PALETTE_INIT(pitnrun)
	MDRV_VIDEO_START(pitnrun)
	MDRV_VIDEO_UPDATE(pitnrun)
	MDRV_SOUND_ADD(AY8910, ay8910_interface)

MACHINE_DRIVER_END


ROM_START( pitnrun )
	ROM_REGION( 0x010000, REGION_CPU1, 0 )
	ROM_LOAD( "pr12",  0x00000, 0x02000, CRC(587a7b85) SHA1(f200ff9b706e13760a23e0187c6bffe496af0087) )
	ROM_LOAD( "pr11",  0x02000, 0x02000, CRC(270cd6dd) SHA1(ad42562e18aa30319fc55c201e5507e8734a5b4d) )
	ROM_LOAD( "pr10",  0x04000, 0x02000, CRC(65d92d89) SHA1(4030ccdb4d84e69c256e95431ee5a18cffeae5c0) )
	ROM_LOAD( "pr9",   0x06000, 0x02000, CRC(3155286d) SHA1(45af8cb81d70f2e30b52bbc7abd9f8d15231735f) )

	ROM_REGION( 0x010000, REGION_CPU2, 0 )
	ROM_LOAD( "pr13",  0x00000, 0x01000, CRC(fc8fd05c) SHA1(f40cc9c6fff6bda8411f4d638a0f5c5915aa3746) )

	ROM_REGION( 0x010000, REGION_CPU3, 0 )
	ROM_LOAD( "a11_17.3a",	0x0000, 0x800, CRC(e7d5d6e1) SHA1(c1131d6fcc36926e287be26090a3c89f22feaa35) )

	ROM_REGION( 0x06000, REGION_GFX1, 0 )
	ROM_LOAD( "pr1",  0x000000, 0x002000, CRC(c3b3131e) SHA1(ed0463e7eef452d7fbdcb031f9477825e9780943) )
	ROM_LOAD( "pr2",  0x002000, 0x002000, CRC(2fa1682a) SHA1(9daefb525fd69f0d9a45ff27e89865545e177a5a) )
	ROM_LOAD( "pr3",  0x004000, 0x002000, CRC(e678fe39) SHA1(134e36fd30bf3cf5884732f3455ca4d9dab6b665) )

	ROM_REGION( 0x4000, REGION_GFX2, 0 )
	ROM_LOAD( "pr4",  0x00000, 0x002000, CRC(fbae3504) SHA1(ce799dfd653462c0814e7530f3f8a686ab0ad7f4) )
	ROM_LOAD( "pr5",  0x02000, 0x002000, CRC(c9177180) SHA1(98c8f8f586b78b88dba254bd662642ee27f9b131) )

	ROM_REGION( 0x2000, REGION_GFX3, 0 )
	ROM_LOAD( "pr6",  0x00000, 0x001000, CRC(c53cb897) SHA1(81a73e6031b52fa45ec507ff4264b14474ef42a2) )
	ROM_LOAD( "pr7",  0x01000, 0x001000, CRC(7cdf9a55) SHA1(404dface7e09186e486945981e39063929599efc) )

	ROM_REGION( 0x2000, REGION_USER1, 0 )
	ROM_LOAD( "pr8",	0x0000, 0x2000, CRC(8e346d10) SHA1(1362ce4362c2d28c48fbd8a33da0cec5ef8e321f) )

	ROM_REGION( 0x0060, REGION_PROMS, 0 )
	ROM_LOAD( "clr.1",  0x0000, 0x0020, CRC(643012f4) SHA1(4a0c9766b9da456e39ce379ad62d695bf82413b0) )
	ROM_LOAD( "clr.2",  0x0020, 0x0020, CRC(50705f02) SHA1(a3d348678fd66f37c7a0d29af88f40740918b8d3) )
	ROM_LOAD( "clr.3",  0x0040, 0x0020, CRC(25e70e5e) SHA1(fdb9c69e9568a725dd0e3ac25835270fb4f49280) )
ROM_END

ROM_START( jumpkun )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "pr1.5d.2764", 0x00000, 0x02000, CRC(b0eabe9f) SHA1(e662f3946efe72b0bbf6c6934201163f765bb7aa) )
	ROM_LOAD( "pr2.5c.2764", 0x02000, 0x02000, CRC(d9240413) SHA1(f4d0491e125f1fe435b200b38fa125889784af0a) )
	ROM_LOAD( "pr3.5b.2764", 0x04000, 0x02000, CRC(105e3fec) SHA1(06ea902e6647fc37a603146324e3d0a067e1f649) )
	ROM_LOAD( "pr4.5a.2764", 0x06000, 0x02000, CRC(3a17ca88) SHA1(00516798d546098831e75547664c8fdaa2bbf050) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "snd1.2732", 0x00000, 0x01000, CRC(1290f316) SHA1(13e393860c1f7d1f97343b9f936c60adb7641efc) )
	ROM_LOAD( "snd2.2732", 0x01000, 0x01000, CRC(ec5e4489) SHA1(fc94fe798a1925e8e3dd15161648e9a960969fc4) )

	ROM_REGION( 0x0800, REGION_CPU3, 0 )
	/* not populated */

	ROM_REGION( 0x6000, REGION_GFX1, 0 )
	ROM_LOAD( "obj1.1k.2764", 0x00000, 0x02000, CRC(8929abfd) SHA1(978994af5816c20a8cd520263d04d1cc1e4df576) )
	ROM_LOAD( "obj2.1m.2764", 0x02000, 0x02000, CRC(c7bf5819) SHA1(15d8e1dd1c0911785237e9063a75a42a2dc1bd50) )
	ROM_LOAD( "obj3.1n.2764", 0x04000, 0x02000, CRC(5eeec986) SHA1(e58a0b98b90a1dd3971ed305100337aa2e5ec450) )

	ROM_REGION( 0x4000, REGION_GFX2, 0 )
	ROM_LOAD( "chr1.6d.2764", 0x00000, 0x02000, CRC(3c93d4ee) SHA1(003121c49bccbb95efb137e6d92d26eea1957fbd)  )
	ROM_LOAD( "chr2.6f.2764", 0x02000, 0x02000, CRC(154fad33) SHA1(7eddc794bd547053f185bb79a8220907bab13d85)  )

	ROM_REGION( 0x2000, REGION_GFX3, 0 )
	ROM_LOAD( "bsc2.3m.2764", 0x00000, 0x01000, CRC(25445f17) SHA1(b1ada95d8f02623bb4a4562d2d278a882414e57e) )
	ROM_LOAD( "bsc1.3p.2764", 0x01000, 0x01000, CRC(39ca2c37) SHA1(b8c71f443a0faf54df03ac5aca46ddd34c42d3a0) )

	ROM_REGION( 0x2000, REGION_USER1, 0 )
	/* not populated */

	ROM_REGION( 0x0060, REGION_PROMS, 0 )
	ROM_LOAD( "8h.82s123.bin", 0x0000, 0x0020, CRC(e54a6fe6) SHA1(c51da2cbf54b7abff7b0cdf0d6846c375b71edcd) )
	ROM_LOAD( "8l.82s123.bin", 0x0020, 0x0020, CRC(624830d5) SHA1(793b2f770ccef6c03bf3ecbf4debcc0531f62da1) )
	ROM_LOAD( "8j.82s123.bin", 0x0040, 0x0020, CRC(223a6990) SHA1(06e16de037c2c7ad5733390859fa7ec1ab1e2f69) )
ROM_END

GAMEX( 1984, pitnrun, 0, pitnrun, pitnrun, 0, ROT90, "Taito Corporation", "Pit and Run", GAME_IMPERFECT_SOUND )
GAMEX( 1984, jumpkun, 0, pitnrun, jumpkun, 0, ROT90, "Kaneko",            "Jump Kun (prototype)", GAME_IMPERFECT_SOUND )
