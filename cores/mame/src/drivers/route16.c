/***************************************************************************

 Route 16/Stratovox memory map (preliminary)

 driver by Zsolt Vasvari

 Notes: Route 16 and Stratovox use identical hardware with the following
        exceptions: Stratovox has a DAC for voice.
        Route 16 has the added ability to turn off each bitplane indiviaually.
        This looks like an afterthought, as one of the same bits that control
        the palette selection is doubly utilized as the bitmap enable bit.

 Space Echo:
        when all astronauts are taken the game over tune ends with 5 bad notes,
        this appears to be a bug in the rom from a changed instruction at 2EB3.

        service mode shows a garbled screen as most of the code for it has been
        replaced by other routines, however the sound tests still work. it's
        possible that the service switch isn't connected on the real hardware.

        the game hangs if it doesn't pass the startup test, a best guess is implemented
        rather than patching out the test. code for the same test is in stratvox but
        isn't called, speakres has a very similar test but doesn't care about the result.

        interrupts per frame for cpu1 is a best guess based on how stratvox uses the DAC,
        writing up to 195 times per frame with each byte from the rom written 4 times.
        spacecho writes one byte per interrupt so 195/4 or 48 is used. a lower number
        increases the chance of a sound interrupting itself, which for most sounds
        is buggy and causes the game to freeze until the first sound completes.

 CPU1

 0000-2fff ROM
 4000-43ff Shared RAM
 8000-bfff Video RAM

 I/O Read

 48xx IN0 - DIP Switches
 50xx IN1 - Input Port 1
 58xx IN2 - Input Port 2
 60xx IN3 - Unknown (Speak & Rescue/Space Echo only)

 I/O Write

 48xx OUT0 - D0-D4 color select for VRAM 0
             D5    coin counter
 50xx OUT1 - D0-D4 color select for VRAM 1
             D5    VIDEO I/II (Flip Screen)
 58xx OUT2 - Unknown (Speak & Rescue/Space Echo only)

 I/O Port Write

 6800 AY-8910 Write Port
 6900 AY-8910 Control Port


 CPU2

 0000-1fff ROM
 4000-43ff Shared RAM
 8000-bfff Video RAM

 I/O Write

 2800      DAC output (Stratovox only)

 ***************************************************************************/

#include "driver.h"

extern unsigned char *route16_sharedram;
extern unsigned char *route16_videoram1;
extern unsigned char *route16_videoram2;
extern size_t route16_videoram_size;
extern int route16_hardware;

DRIVER_INIT( route16 );
DRIVER_INIT( route16b );
DRIVER_INIT( stratvox );
PALETTE_INIT( route16 );
VIDEO_START( route16 );
WRITE_HANDLER( route16_out0_w );
WRITE_HANDLER( route16_out1_w );
WRITE_HANDLER( route16_videoram1_w );
WRITE_HANDLER( route16_videoram2_w );
READ_HANDLER( route16_videoram1_r );
READ_HANDLER( route16_videoram2_r );
WRITE_HANDLER( route16_sharedram_w );
READ_HANDLER( route16_sharedram_r );
VIDEO_UPDATE( route16 );
WRITE_HANDLER( stratvox_sn76477_w );
READ_HANDLER ( speakres_in3_r );
WRITE_HANDLER ( speakres_out2_w );

READ_HANDLER( routex_prot_read )
{
	if (activecpu_get_pc()==0x2f) return 0xFB;

	logerror ("cpu #%d (PC=%08X): unmapped prot read\n", cpu_getactivecpu(), activecpu_get_pc());
	return 0x00;

}

static MEMORY_READ_START( cpu1_readmem )
	{ 0x0000, 0x2fff, MRA_ROM },
  /*{ 0x3000, 0x3001, MRA_NOP },	 Route 16 protection device */
	{ 0x4000, 0x43ff, route16_sharedram_r },
	{ 0x4800, 0x4800, input_port_0_r },
	{ 0x5000, 0x5000, input_port_1_r },
	{ 0x5800, 0x5800, input_port_2_r },
	{ 0x8000, 0xbfff, route16_videoram1_r },
MEMORY_END

static MEMORY_WRITE_START( cpu1_writemem )
	{ 0x0000, 0x2fff, MWA_ROM },
 	/*{ 0x3001, 0x3001, MWA_NOP },	 Route 16 protection device */
	{ 0x4000, 0x43ff, route16_sharedram_w, &route16_sharedram },
	{ 0x4800, 0x4800, route16_out0_w },
	{ 0x5000, 0x5000, route16_out1_w },
	{ 0x8000, 0xbfff, route16_videoram1_w, &route16_videoram1, &route16_videoram_size },
	{ 0xc000, 0xc000, MWA_RAM }, /* Stratvox has an off by one error*/
                                 /* when clearing the screen*/
MEMORY_END

static MEMORY_READ_START( routex_cpu1_readmem )
	{ 0x0000, 0x37ff, MRA_ROM },
	{ 0x4000, 0x43ff, route16_sharedram_r },
	{ 0x4800, 0x4800, input_port_0_r },
	{ 0x5000, 0x5000, input_port_1_r },
	{ 0x5800, 0x5800, input_port_2_r },
 	{ 0x6400, 0x6400, routex_prot_read },
	{ 0x8000, 0xbfff, route16_videoram1_r },
MEMORY_END

static MEMORY_WRITE_START( routex_cpu1_writemem )
	{ 0x0000, 0x37ff, MWA_ROM },
	{ 0x4000, 0x43ff, route16_sharedram_w, &route16_sharedram },
	{ 0x4800, 0x4800, route16_out0_w },
	{ 0x5000, 0x5000, route16_out1_w },
	{ 0x8000, 0xbfff, route16_videoram1_w, &route16_videoram1, &route16_videoram_size },
	{ 0xc000, 0xc000, MWA_RAM }, /* Stratvox has an off by one error*/
                                 /* when clearing the screen*/
MEMORY_END

static MEMORY_READ_START( altcpu1_readmem )
	{ 0x0000, 0x2fff, MRA_ROM },
	{ 0x4000, 0x43ff, route16_sharedram_r },
	{ 0x4800, 0x4800, input_port_0_r },
	{ 0x5000, 0x5000, input_port_1_r },
	{ 0x5800, 0x5800, input_port_2_r },
	{ 0x6000, 0x6000, speakres_in3_r },
	{ 0x8000, 0xbfff, route16_videoram1_r },
MEMORY_END

static MEMORY_WRITE_START( altcpu1_writemem )
	{ 0x0000, 0x2fff, MWA_ROM },
	{ 0x4000, 0x43ff, route16_sharedram_w, &route16_sharedram },
	{ 0x4800, 0x4800, route16_out0_w },
	{ 0x5000, 0x5000, route16_out1_w },
	{ 0x5800, 0x5800, speakres_out2_w },
	{ 0x8000, 0xbfff, route16_videoram1_w, &route16_videoram1, &route16_videoram_size },
	{ 0xc000, 0xc000, MWA_RAM }, /* Speak & Rescue/Space Echo have same off by one error*/
MEMORY_END

static PORT_WRITE_START( cpu1_writeport )
	{ 0x6800, 0x6800, AY8910_write_port_0_w },
	{ 0x6900, 0x6900, AY8910_control_port_0_w },
PORT_END

static MEMORY_READ_START( cpu2_readmem )
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x4000, 0x43ff, route16_sharedram_r },
	{ 0x8000, 0xbfff, route16_videoram2_r },
MEMORY_END

static MEMORY_WRITE_START( cpu2_writemem )
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x2800, 0x2800, DAC_0_data_w }, /* Not used by Route 16*/
	{ 0x4000, 0x43ff, route16_sharedram_w },
	{ 0x8000, 0xbfff, route16_videoram2_w, &route16_videoram2 },
	{ 0xc000, 0xc1ff, MWA_NOP }, /* Route 16 sometimes writes outside of*/
MEMORY_END


INPUT_PORTS_START( route16 )
	PORT_START      /* DSW 1 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) ) /* Doesn't seem to*/
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )                    /* be referenced*/
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) ) /* Doesn't seem to*/
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )                    /* be referenced*/
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
/*	PORT_DIPSETTING(    0x18, DEF_STR( 2C_1C ) ) */ /* Same as 0x08*/
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START      /* Input Port 1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START      /* Input Port 2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START1 )
INPUT_PORTS_END



INPUT_PORTS_START( stratvox )
	PORT_START      /* IN0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPNAME( 0x02, 0x00, "Replenish Astronouts" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0c, 0x00, "2 Attackers At Wave" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x0c, "5" )
	PORT_DIPNAME( 0x10, 0x00, "Astronauts Kidnapped" )
	PORT_DIPSETTING(    0x00, "Less Often" )
	PORT_DIPSETTING(    0x10, "More Often" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "Demo Voices" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x0c, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0c, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START1 )
INPUT_PORTS_END

INPUT_PORTS_START( speakres )
	PORT_START      /* IN0 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x0c, 0x00, "2 Attackers At Wave" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x0c, "5" )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "5000" )
	PORT_DIPSETTING(    0x10, "8000" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "Demo Voices" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x0c, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0c, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START1 )
INPUT_PORTS_END

INPUT_PORTS_START( spacecho )
	PORT_START      /* IN0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPNAME( 0x02, 0x00, "Replenish Astronouts" )
	PORT_DIPSETTING(    0x02, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0c, 0x00, "2 Attackers At Wave" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x0c, "5" )
	PORT_DIPNAME( 0x10, 0x00, "Astronauts Kidnapped" )
	PORT_DIPSETTING(    0x00, "Less Often" )
	PORT_DIPSETTING(    0x10, "More Often" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "Demo Voices" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x0c, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0c, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START1 )
INPUT_PORTS_END


static struct AY8910interface ay8910_interface =
{
	1,	/* 1 chip */
	10000000/8,     /* 10MHz / 8 = 1.25MHz */
	{ 50 },
	{ 0 },
	{ 0 },
	{ stratvox_sn76477_w },  /* SN76477 commands (not used in Route 16?) */
	{ 0 }
};


static struct SN76477interface sn76477_interface =
{
	1,	/* 1 chip */
	{ 50 },  /* mixing level   pin description		 */
	{ RES_K( 47)   },		/*	4  noise_res		 */
	{ RES_K(150)   },		/*	5  filter_res		 */
	{ CAP_U(0.001) },		/*	6  filter_cap		 */
	{ RES_M(3.3)   },		/*	7  decay_res		 */
	{ CAP_U(1.0)   },		/*	8  attack_decay_cap  */
	{ RES_K(4.7)   },		/* 10  attack_res		 */
	{ RES_K(200)   },		/* 11  amplitude_res	 */
	{ RES_K( 55)   },		/* 12  feedback_res 	 */
	{ 5.0*2/(2+10) },		/* 16  vco_voltage		 */
	{ CAP_U(0.022) },		/* 17  vco_cap			 */
	{ RES_K(100)   },		/* 18  vco_res			 */
	{ 5.0		   },		/* 19  pitch_voltage	 */
	{ RES_K( 75)   },		/* 20  slf_res			 */
	{ CAP_U(1.0)   },		/* 21  slf_cap			 */
	{ CAP_U(2.2)   },		/* 23  oneshot_cap		 */
	{ RES_K(4.7)   }		/* 24  oneshot_res		 */
};


static struct DACinterface dac_interface =
{
	1,
	{ 50 }
};


static MACHINE_DRIVER_START( route16 )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("cpu1", Z80, 2500000)	/* 10MHz / 4 = 2.5MHz */
	MDRV_CPU_FLAGS(CPU_16BIT_PORT)
	MDRV_CPU_MEMORY(cpu1_readmem,cpu1_writemem)
	MDRV_CPU_PORTS(0,cpu1_writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD_TAG("cpu2", Z80, 2500000)	/* 10MHz / 4 = 2.5MHz */
	MDRV_CPU_MEMORY(cpu2_readmem,cpu2_writemem)

	MDRV_FRAMES_PER_SECOND(57)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)       /* frames per second, vblank duration */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 256-1, 0, 256-1)
	MDRV_PALETTE_LENGTH(8)

	MDRV_PALETTE_INIT(route16)
	MDRV_VIDEO_START(route16)
	MDRV_VIDEO_UPDATE(route16)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( routex )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(route16)
	MDRV_CPU_MODIFY("cpu1")
	MDRV_CPU_MEMORY(routex_cpu1_readmem,routex_cpu1_writemem)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( stratvox )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(route16)

	/* sound hardware */
	MDRV_SOUND_ADD(SN76477, sn76477_interface)
	MDRV_SOUND_ADD(DAC, dac_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( speakres )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(stratvox)
	MDRV_CPU_MODIFY("cpu1")
	MDRV_CPU_MEMORY(altcpu1_readmem,altcpu1_writemem)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( spacecho )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(speakres)
	MDRV_CPU_MODIFY("cpu2")
	MDRV_CPU_VBLANK_INT(irq0_line_hold,48)
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( route16 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )  /* 64k for the first CPU*/
	ROM_LOAD( "route16.a0",   0x0000, 0x0800, CRC(8f9101bd) SHA1(b2c0156d41e295282387fb85fc272b031a6d1b64) )
	ROM_LOAD( "route16.a1",   0x0800, 0x0800, CRC(389bc077) SHA1(b0606f6e647e81ceae7148bda96bd4673a51e823) )
	ROM_LOAD( "route16.a2",   0x1000, 0x0800, CRC(1065a468) SHA1(4a707a42fb5a718043c173cb98ff3523eb274ccc) )
	ROM_LOAD( "route16.a3",   0x1800, 0x0800, CRC(0b1987f3) SHA1(9b8abd6ec1ae15ca0d5e4de6b8a7ebf6c929d767) )
	ROM_LOAD( "route16.a4",   0x2000, 0x0800, CRC(f67d853a) SHA1(7479e84082e78f8670cc50858ce6a006d3063413) )
	ROM_LOAD( "route16.a5",   0x2800, 0x0800, CRC(d85cf758) SHA1(5af21250ee44ab1a43b844ede5a777a3d33b78b5) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )  /* 64k for the second CPU*/
	ROM_LOAD( "route16.b0",   0x0000, 0x0800, CRC(0f9588a7) SHA1(dfaffec4dbabd98cdc21a416bd2966d9d3ae6ad1) )
	ROM_LOAD( "route16.b1",   0x0800, 0x0800, CRC(2b326cf9) SHA1(c6602a9440a982c39f5836c6ab72283b6f9241be) )
	ROM_LOAD( "route16.b2",   0x1000, 0x0800, CRC(529cad13) SHA1(b533d20df1f2580e237c3d60bfe3483486ad9a48) )
	ROM_LOAD( "route16.b3",   0x1800, 0x0800, CRC(3bd8b899) SHA1(bc0c7909dbf5ea85eba5a1bb815fdd98c3aa794e) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	/* The upper 128 bytes are 0's, used by the hardware to blank the display */
	ROM_LOAD( "pr09",         0x0000, 0x0100, CRC(08793ef7) SHA1(bfc27aaf25d642cd57c0fbe73ab575853bd5f3ca) ) /* top bitmap */
	ROM_LOAD( "pr10",         0x0100, 0x0100, CRC(08793ef7) SHA1(bfc27aaf25d642cd57c0fbe73ab575853bd5f3ca) ) /* bottom bitmap */
ROM_END

ROM_START( route16a )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )  /* 64k for the first CPU*/
	ROM_LOAD( "vg-54",        0x0000, 0x0800, CRC(0c966319) SHA1(2f57e9a30dab864bbee2ccb0107c1b4212c5abaf) )
	ROM_LOAD( "vg-55",        0x0800, 0x0800, CRC(a6a8c212) SHA1(a4a695d401b1e495c863c6938296a99592df0e7d) )
	ROM_LOAD( "vg-56",        0x1000, 0x0800, CRC(5c74406a) SHA1(f106c27da6cac597afbabdef3ec7fa7d203905b0) )
	ROM_LOAD( "vg-57",        0x1800, 0x0800, CRC(313e68ab) SHA1(01fa83898123eb92a14bffc6fe774e00b083e86c) )
	ROM_LOAD( "vg-58",        0x2000, 0x0800, CRC(40824e3c) SHA1(bc157e6babf00d2119b389fdb9d5822e1c764f51) )
	ROM_LOAD( "vg-59",        0x2800, 0x0800, CRC(9313d2c2) SHA1(e08112f44ca454820752800d8b3b6408b73a4284) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )  /* 64k for the second CPU*/
	ROM_LOAD( "route16.b0",   0x0000, 0x0800, CRC(0f9588a7) SHA1(dfaffec4dbabd98cdc21a416bd2966d9d3ae6ad1) )
	ROM_LOAD( "vg-61",        0x0800, 0x0800, CRC(b216c88c) SHA1(d011ef9f3727f87ae3482e271a0c2496f76036b4) )
	ROM_LOAD( "route16.b2",   0x1000, 0x0800, CRC(529cad13) SHA1(b533d20df1f2580e237c3d60bfe3483486ad9a48) )
	ROM_LOAD( "route16.b3",   0x1800, 0x0800, CRC(3bd8b899) SHA1(bc0c7909dbf5ea85eba5a1bb815fdd98c3aa794e) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	/* The upper 128 bytes are 0's, used by the hardware to blank the display */
	ROM_LOAD( "pr09",         0x0000, 0x0100, CRC(08793ef7) SHA1(bfc27aaf25d642cd57c0fbe73ab575853bd5f3ca) ) /* top bitmap */
	ROM_LOAD( "pr10",         0x0100, 0x0100, CRC(08793ef7) SHA1(bfc27aaf25d642cd57c0fbe73ab575853bd5f3ca) ) /* bottom bitmap */
ROM_END

ROM_START( route16b )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )  /* 64k for the first CPU*/
	ROM_LOAD( "rt16.0",       0x0000, 0x0800, CRC(b1f0f636) SHA1(f21915ed40ebdf64970fb7e3cd8071ebfc4aa0b5) )
	ROM_LOAD( "rt16.1",       0x0800, 0x0800, CRC(3ec52fe5) SHA1(451969b5caedd665231ef78cf262679d6d4c8507) )
	ROM_LOAD( "rt16.2",       0x1000, 0x0800, CRC(a8e92871) SHA1(68a709c14309d2b617997b76ae9d7b80fd326f39) )
	ROM_LOAD( "rt16.3",       0x1800, 0x0800, CRC(a0fc9fc5) SHA1(7013750c1b3d403b12eac10282a930538ed9c73e) )
	ROM_LOAD( "rt16.4",       0x2000, 0x0800, CRC(6dcaf8c4) SHA1(27d84cc29f2b75280678e9c77f270ee39af50228) )
	ROM_LOAD( "rt16.5",       0x2800, 0x0800, CRC(63d7b05b) SHA1(d1e3473be283c92063674b9e69575081115bc456) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )  /* 64k for the second CPU*/
	ROM_LOAD( "rt16.6",       0x0000, 0x0800, CRC(fef605f3) SHA1(bfbffa0ded3e285c034f0ad832864021ef3f2256) )
	ROM_LOAD( "rt16.7",       0x0800, 0x0800, CRC(d0d6c189) SHA1(75cec891e20cf05aae354c8950857aea83c6dadc) )
	ROM_LOAD( "rt16.8",       0x1000, 0x0800, CRC(defc5797) SHA1(aec8179e647de70016e0e63b720f932752adacc1) )
	ROM_LOAD( "rt16.9",       0x1800, 0x0800, CRC(88d94a66) SHA1(163e952ada7c05110d1f1c681bd57d3b9ea8866e) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	/* The upper 128 bytes are 0's, used by the hardware to blank the display */
	ROM_LOAD( "pr09",         0x0000, 0x0100, CRC(08793ef7) SHA1(bfc27aaf25d642cd57c0fbe73ab575853bd5f3ca) ) /* top bitmap */
	ROM_LOAD( "pr10",         0x0100, 0x0100, CRC(08793ef7) SHA1(bfc27aaf25d642cd57c0fbe73ab575853bd5f3ca) ) /* bottom bitmap */
ROM_END

ROM_START( routex )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )  /* 64k for the first CPU*/
	ROM_LOAD( "routex01.a0",  0x0000, 0x0800, CRC(99b500e7) SHA1(2561c04a1425d7ac3309faf29fcfde63a0cda4da) )
	ROM_LOAD( "rt16.1",       0x0800, 0x0800, CRC(3ec52fe5) SHA1(451969b5caedd665231ef78cf262679d6d4c8507) )
	ROM_LOAD( "rt16.2",       0x1000, 0x0800, CRC(a8e92871) SHA1(68a709c14309d2b617997b76ae9d7b80fd326f39) )
	ROM_LOAD( "rt16.3",       0x1800, 0x0800, CRC(a0fc9fc5) SHA1(7013750c1b3d403b12eac10282a930538ed9c73e) )
	ROM_LOAD( "routex05.a4",  0x2000, 0x0800, CRC(2fef7653) SHA1(ba3477da249ca402d096704e57ea638fde6abe9c) )
	ROM_LOAD( "routex06.a5",  0x2800, 0x0800, CRC(a39ef648) SHA1(866095d9880b60b01f7ca66b332f5f6c4b41a5ac) )
	ROM_LOAD( "routex07.a6",  0x3000, 0x0800, CRC(89f80c1c) SHA1(dff37e0f2446a99890135891c59dc501866a25cc) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )  /* 64k for the second CPU*/
	ROM_LOAD( "routex11.b0",  0x0000, 0x0800, CRC(b51edd1d) SHA1(1ca10afd6851875c98b1d29aee457234c20ce0bf) )
	ROM_LOAD( "rt16.7",       0x0800, 0x0800, CRC(d0d6c189) SHA1(75cec891e20cf05aae354c8950857aea83c6dadc) )
	ROM_LOAD( "rt16.8",       0x1000, 0x0800, CRC(defc5797) SHA1(aec8179e647de70016e0e63b720f932752adacc1) )
	ROM_LOAD( "rt16.9",       0x1800, 0x0800, CRC(88d94a66) SHA1(163e952ada7c05110d1f1c681bd57d3b9ea8866e) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	/* The upper 128 bytes are 0's, used by the hardware to blank the display */
	ROM_LOAD( "pr09",         0x0000, 0x0100, CRC(08793ef7) SHA1(bfc27aaf25d642cd57c0fbe73ab575853bd5f3ca) ) /* top bitmap */
	ROM_LOAD( "pr10",         0x0100, 0x0100, CRC(08793ef7) SHA1(bfc27aaf25d642cd57c0fbe73ab575853bd5f3ca) ) /* bottom bitmap */
ROM_END

ROM_START( stratvox )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "ls01.bin",     0x0000, 0x0800, CRC(bf4d582e) SHA1(456f37e16d037a30dc4c1c460ebf9a248bf1a57c) )
	ROM_LOAD( "ls02.bin",     0x0800, 0x0800, CRC(16739dd4) SHA1(cd1f7d1b52ca1ab458d11b969f4f1f5af3ec7353) )
	ROM_LOAD( "ls03.bin",     0x1000, 0x0800, CRC(083c28de) SHA1(82e159f218f60e9c06ff78f2e52572f8f5a6c530) )
	ROM_LOAD( "ls04.bin",     0x1800, 0x0800, CRC(b0927e3b) SHA1(cc5f030dcbc93d5265dbf17a2425acdb921ab18b) )
	ROM_LOAD( "ls05.bin",     0x2000, 0x0800, CRC(ccd25c4e) SHA1(d6d5722d746dd22cecacfea407e798f4531eea99) )
	ROM_LOAD( "ls06.bin",     0x2800, 0x0800, CRC(07a907a7) SHA1(0c41eac01ac9fd67ef19752c47414c4bd90324b4) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for the second CPU */
	ROM_LOAD( "ls07.bin",     0x0000, 0x0800, CRC(4d333985) SHA1(371405b92b2ee8040e48ec7ad715d1a960746aac) )
	ROM_LOAD( "ls08.bin",     0x0800, 0x0800, CRC(35b753fc) SHA1(179e21f531e8be507f1754159590c111be1b44ff) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	/* The upper 128 bytes are 0's, used by the hardware to blank the display */
	ROM_LOAD( "pr09",         0x0000, 0x0100, CRC(08793ef7) SHA1(bfc27aaf25d642cd57c0fbe73ab575853bd5f3ca) ) /* top bitmap */
	ROM_LOAD( "pr10",         0x0100, 0x0100, CRC(08793ef7) SHA1(bfc27aaf25d642cd57c0fbe73ab575853bd5f3ca) ) /* bottom bitmap */
ROM_END

ROM_START( speakres )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "speakres.1",   0x0000, 0x0800, CRC(6026e4ea) SHA1(77975620b489f10e5b5de834e812c2802315e889) )
	ROM_LOAD( "speakres.2",   0x0800, 0x0800, CRC(93f0d4da) SHA1(bf3d2931d12a436bb4f0d0556806008ca722f070) )
	ROM_LOAD( "speakres.3",   0x1000, 0x0800, CRC(a3874304) SHA1(ca243364d077fa70d6c46b950ba6666617a56cc2) )
	ROM_LOAD( "speakres.4",   0x1800, 0x0800, CRC(f484be3a) SHA1(5befa61c5f3a3cde3d7d6cae2130021288ed8454) )
	ROM_LOAD( "speakres.5",   0x2000, 0x0800, CRC(61b12a67) SHA1(a1a636ecde16ffdc9f0bb460bd12f945ec66d36f) )
	ROM_LOAD( "speakres.6",   0x2800, 0x0800, CRC(220e0ab2) SHA1(9fb4abf50ff28995cb1f7ba807e15eb87127f520) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for the second CPU */
	ROM_LOAD( "speakres.7",   0x0000, 0x0800, CRC(d417be13) SHA1(6f1f76a911579b49bb0e1992296e7c3acf2bd517) )
	ROM_LOAD( "speakres.8",   0x0800, 0x0800, CRC(52485d60) SHA1(28b708a71d16428d1cd58f3b7aa326ccda85533c) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	/* The upper 128 bytes are 0's, used by the hardware to blank the display */
	ROM_LOAD( "pr09",         0x0000, 0x0100, CRC(08793ef7) SHA1(bfc27aaf25d642cd57c0fbe73ab575853bd5f3ca) ) /* top bitmap */
	ROM_LOAD( "pr10",         0x0100, 0x0100, CRC(08793ef7) SHA1(bfc27aaf25d642cd57c0fbe73ab575853bd5f3ca) ) /* bottom bitmap */
ROM_END

ROM_START( spacecho )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "rom.a0",       0x0000, 0x0800, CRC(40d74dce) SHA1(891d7fde1d4b0b66c38fa7f8933480e201c68113) )
	ROM_LOAD( "rom.a1",       0x0800, 0x0800, CRC(a5f0a34f) SHA1(359e7a9954dedb464f7456cd071db77b2219ab2c) )
	ROM_LOAD( "rom.a2",       0x1000, 0x0800, CRC(cbbb3acb) SHA1(3dc71683f31da39a544382b463ece39cca8124b3) )
	ROM_LOAD( "rom.a3",       0x1800, 0x0800, CRC(311050ca) SHA1(ed4a5cb7ec0306654178dae8f30b39b9c8db0ce3) )
	ROM_LOAD( "rom.a4",       0x2000, 0x0800, CRC(28943803) SHA1(4904e6d092494bfca064d25d094ab9e9049fa9ca) )
	ROM_LOAD( "rom.a5",       0x2800, 0x0800, CRC(851c9f28) SHA1(c7bb4e25b74eb71e8b394214f9cbd95f59a1fa58) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for the second CPU */
	ROM_LOAD( "rom.b0",       0x0000, 0x0800, CRC(db45689d) SHA1(057a8dc2629f57fdeebb6262de2bdd78b4e66dca) )
	ROM_LOAD( "rom.b2",       0x1000, 0x0800, CRC(1e074157) SHA1(cb2073415aff7804ac85e2137bef2005bf6cf239) )
	ROM_LOAD( "rom.b3",       0x1800, 0x0800, CRC(d50a8b20) SHA1(d733fa327d2e7dfe08c84015c6c326ed8ab39e3d) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	/* The upper 128 bytes are 0's, used by the hardware to blank the display */
	ROM_LOAD( "pr09",         0x0000, 0x0100, BAD_DUMP CRC(08793ef7) SHA1(bfc27aaf25d642cd57c0fbe73ab575853bd5f3ca)  ) /* top bitmap */
	ROM_LOAD( "pr10",         0x0100, 0x0100, BAD_DUMP CRC(08793ef7) SHA1(bfc27aaf25d642cd57c0fbe73ab575853bd5f3ca)  ) /* bottom bitmap */
ROM_END


/***************************************************************************

  Set hardware dependent flag.

***************************************************************************/
DRIVER_INIT( route16b )
{
    route16_hardware = 1;
}

DRIVER_INIT( route16 )
{
	unsigned char *rom = memory_region(REGION_CPU1);
	/* Is this actually a bootleg? some of the protection has
	   been removed */

	/* patch the protection */
	rom[0x00e9] = 0x3a;

	rom[0x0754] = 0xc3;
	rom[0x0755] = 0x63;
	rom[0x0756] = 0x07;

	init_route16b();
}

DRIVER_INIT( route16a )
{
	UINT8 *ROM = memory_region(REGION_CPU1);
	/* TO DO : Replace these patches with simulation of the protection device */

	/* patch the protection */
	ROM[0x00e9] = 0x3a;

	ROM[0x0105] = 0x00; /* jp nz,$4109 (nirvana) - NOP's in route16 */
	ROM[0x0106] = 0x00;
	ROM[0x0107] = 0x00;

	ROM[0x0731] = 0x00; /* jp nz,$4238 (nirvana) */
	ROM[0x0732] = 0x00;
	ROM[0x0733] = 0x00;

	ROM[0x0747] = 0xc3;
	ROM[0x0748] = 0x56;
	ROM[0x0749] = 0x07;

	init_route16b();
}


DRIVER_INIT( stratvox )
{
    route16_hardware = 0;
}



GAME( 1981, route16,  0,        route16,  route16,  route16,  ROT270, "Tehkan/Sun (Centuri license)", "Route 16" )
GAME( 1981, route16a, route16,  route16,  route16,  route16a, ROT270, "Tehkan/Sun (Centuri license)", "Route 16 (set 2)" )
GAME( 1981, route16b, route16,  route16,  route16,  route16b, ROT270, "bootleg", "Route 16 (bootleg)" )
GAME( 1981, routex,   route16,  routex,   route16,  route16b, ROT270, "bootleg", "Route X (bootleg)" )
GAME( 1980, speakres, 0,        speakres, speakres, stratvox, ROT270, "Sun Electronics", "Speak and Rescue" )
GAME( 1980, stratvox, speakres, stratvox, stratvox, stratvox, ROT270, "[Sun Electronics] (Taito license)", "Stratovox" )
GAME( 1980, spacecho, speakres, spacecho, spacecho, stratvox, ROT270, "bootleg", "Space Echo" )
