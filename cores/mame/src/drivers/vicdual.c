/***************************************************************************

VIC Dual Game board

Seems like there are 2 types of boards. One uses an 8080A and 2 control PROMs
and the other is a Z80 with 1 control PROM.


0000-3fff ROM
4000-7fff ROM mirror image (used in most games)

the following have mirror images throughout the address space
e000-e3ff Video RAM + work RAM
e400-e7ff RAM
e800-efff Character generator RAM

I/O ports:

The memory map is the same for many games, but the I/O ports change. The
following ones are for Carnival, and apply to many other games as well.

read:
00        IN0
          bit 0 = connector
          bit 1 = connector
          bit 2 = dsw
          bit 3 = dsw
          bit 4 = connector
          bit 5 = connector
          bit 6 = seems unused
          bit 7 = seems unused

01        IN1
          bit 0 = connector
          bit 1 = connector
          bit 2 = dsw
          bit 3 = vblank
          bit 4 = connector
          bit 5 = connector
          bit 6 = seems unused
          bit 7 = seems unused

02        IN2
          bit 0 = connector
          bit 1 = connector
          bit 2 = dsw
          bit 3 = timer? is this used?
          bit 4 = connector
          bit 5 = connector
          bit 6 = seems unused
          bit 7 = seems unused

03        IN3
          bit 0 = connector
          bit 1 = connector
          bit 2 = dsw
          bit 3 = COIN (must reset the CPU to make the game acknowledge it)
          bit 4 = connector
          bit 5 = connector
          bit 6 = seems unused
          bit 7 = seems unused

write:
	(ports 1 and 2: see definitions in sound driver)

08        ?

40        palette bank

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/i8039/i8039.h"
#include "artwork.h"



#define	PSG_CLOCK_CARNIVAL	( 3579545 / 3 )	/* Hz */


static unsigned char *vicdual_ram;

extern unsigned char *vicdual_characterram;
PALETTE_INIT( vicdual );
WRITE_HANDLER( vicdual_characterram_w );
READ_HANDLER( vicdual_characterram_r );
WRITE_HANDLER( vicdual_palette_bank_w );
VIDEO_UPDATE( vicdual );

/* Carnival sound handlers */
extern const char *carnival_sample_names[];
WRITE_HANDLER( carnival_sh_port1_w );
WRITE_HANDLER( carnival_sh_port2_w );
READ_HANDLER( carnival_music_port_t1_r );
WRITE_HANDLER( carnival_music_port_1_w );
WRITE_HANDLER( carnival_music_port_2_w );

/* Depth Charge sound handlers */
extern const char *depthch_sample_names[];
WRITE_HANDLER( depthch_sh_port1_w );

/* Invinco sound handlers */
extern const char *invinco_sample_names[];
WRITE_HANDLER( invinco_sh_port2_w );

/* Pulsar sound handlers */
extern const char *pulsar_sample_names[];
WRITE_HANDLER( pulsar_sh_port1_w );
WRITE_HANDLER( pulsar_sh_port2_w );

/* Frogs sound handlers */
struct Samplesinterface frogs_samples_interface;
WRITE_HANDLER( frogs_sh_port2_w );
void croak_callback(int param);
mame_timer *croak_timer;

#define OVERLAY_LTR_BLUE        MAKE_ARGB(0x04,0x2f,0x6d,0xa5)

/* Overlay based on sources: */
/* https://flyers.arcade-museum.com/?page=flyer&db=videodb&id=1701&image=1 */
OVERLAY_START( depthch_overlay )
    OVERLAY_RECT(  0, 0,  256, 224, OVERLAY_LTR_BLUE )
OVERLAY_END

static MACHINE_INIT( frogs )
{
	croak_timer = timer_alloc(croak_callback);
}

static const char *frogs_sample_names[] =
{
	"*frogs",
	"boing.wav",
	"buzzz.wav",
	"croak.wav",
	"hop.wav",
	"splash.wav",
	"zip.wav",
	0       /* end of array */
};

struct Samplesinterface frogs_samples_interface =
{
	5,	/* 5 channels */
	35,	/* volume */
	frogs_sample_names
};

void croak_callback(int param)
{
	sample_stop(2);
}

WRITE_HANDLER( frogs_sh_port2_w )
{
	static int last_croak = 0;
	static int last_buzzz = 0;
	int new_croak = data & 0x08;
	int new_buzzz = data & 0x10;


	if (data & 0x01)
		sample_start (3, 3, 0);	/* Hop */
if (data & 0x02)
		sample_start (0, 0, 0);	/* Boing */
	if (new_croak)
		sample_start (2, 2, 0);	/* Croak */
	else
	{
		if (last_croak)
		{
			/* The croak will keep playing until .429s after being disabled */
			timer_adjust(croak_timer, 1.1 * RES_K(390) * CAP_U(1), 0, 0);
		}
	}
	if (new_buzzz)
	{
		/* The Buzzz sound starts off a little louder in volume then
		 * settles down to a steady buzzz.  Whenever the trigger goes
		 * low, the sound is disabled.  If it then goes high, the buzzz
		 * then starts off louder again.  The games does this every time
		 * the fly moves.
		 * So I made the sample start with the louder effect and then play
		 * for 12 seconds.  A fly should move before this.  If not the
		 * sample loops, adding the loud part as if the fly moved.
		 * This is obviously incorrect, but a fly never stands still for
		 * 12 seconds.
		 */
		if (!last_buzzz)
			sample_start (1, 1, 1);	/* Buzzz */
	}
	else
		sample_stop(1);
	if (data & 0x80)
		sample_start (4, 4, 0);	/* Splash */

	last_croak = new_croak;
	last_buzzz = new_buzzz;
}


static int protection_data;

static WRITE_HANDLER( samurai_protection_w )
{
	protection_data = data;
}

static READ_HANDLER( samurai_input_r )
{
	int answer = 0;

	if (protection_data == 0xab) answer = 0x02;
	else if (protection_data == 0x1d) answer = 0x0c;

	return (readinputport(1 + offset) & 0xfd) | ((answer >> offset) & 0x02);
}


static WRITE_HANDLER( vicdual_ram_w )
{
	vicdual_ram[offset] = data;
}

static READ_HANDLER( vicdual_ram_r )
{
	return vicdual_ram[offset];
}


static READ_HANDLER( depthch_input_port_1_r )
{
	/* bit 0 is 64V according to the schematics */
	return (input_port_1_r(0) & 0xfe) | ((cpu_getscanline() >> 6) & 0x01);
}


static MEMORY_READ_START( vicdual_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x83ff, videoram_r },
	{ 0x8400, 0x87ff, vicdual_ram_r },
	{ 0x8800, 0x8fff, vicdual_characterram_r },
	{ 0x9000, 0x93ff, videoram_r },
	{ 0x9400, 0x97ff, vicdual_ram_r },
	{ 0x9800, 0x9fff, vicdual_characterram_r },
	{ 0xa000, 0xa3ff, videoram_r },
	{ 0xa400, 0xa7ff, vicdual_ram_r },
	{ 0xa800, 0xafff, vicdual_characterram_r },
	{ 0xb000, 0xb3ff, videoram_r },
	{ 0xb400, 0xb7ff, vicdual_ram_r },
	{ 0xb800, 0xbfff, vicdual_characterram_r },
	{ 0xc000, 0xc3ff, videoram_r },
	{ 0xc400, 0xc7ff, vicdual_ram_r },
	{ 0xc800, 0xcfff, vicdual_characterram_r },
	{ 0xd000, 0xd3ff, videoram_r },
	{ 0xd400, 0xd7ff, vicdual_ram_r },
	{ 0xd800, 0xdfff, vicdual_characterram_r },
	{ 0xe000, 0xe3ff, videoram_r },
	{ 0xe400, 0xe7ff, vicdual_ram_r },
	{ 0xe800, 0xefff, vicdual_characterram_r },
	{ 0xf000, 0xf3ff, videoram_r },
	{ 0xf400, 0xf7ff, vicdual_ram_r },
	{ 0xf800, 0xffff, vicdual_characterram_r },
MEMORY_END

static MEMORY_WRITE_START( vicdual_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x83ff, videoram_w, &videoram, &videoram_size },
	{ 0x8400, 0x87ff, vicdual_ram_w, &vicdual_ram },
	{ 0x8800, 0x8fff, vicdual_characterram_w, &vicdual_characterram },
	{ 0x9000, 0x93ff, videoram_w, &videoram, &videoram_size },
	{ 0x9400, 0x97ff, vicdual_ram_w, &vicdual_ram },
	{ 0x9800, 0x9fff, vicdual_characterram_w, &vicdual_characterram},
	{ 0xa000, 0xa3ff, videoram_w },
	{ 0xa400, 0xa7ff, vicdual_ram_w },
	{ 0xa800, 0xafff, vicdual_characterram_w },
	{ 0xb000, 0xb3ff, videoram_w },
	{ 0xb400, 0xb7ff, vicdual_ram_w },
	{ 0xb800, 0xbfff, vicdual_characterram_w },
	{ 0xc000, 0xc3ff, videoram_w },
	{ 0xc400, 0xc7ff, vicdual_ram_w },
	{ 0xc800, 0xcfff, vicdual_characterram_w },
	{ 0xd000, 0xd3ff, videoram_w },
	{ 0xd400, 0xd7ff, vicdual_ram_w },
	{ 0xd800, 0xdfff, vicdual_characterram_w },
	{ 0xe000, 0xe3ff, videoram_w },
	{ 0xe400, 0xe7ff, vicdual_ram_w },
	{ 0xe800, 0xefff, vicdual_characterram_w },
	{ 0xf000, 0xf3ff, videoram_w },
	{ 0xf400, 0xf7ff, vicdual_ram_w },
	{ 0xf800, 0xffff, vicdual_characterram_w },
MEMORY_END


/* Safari has extra RAM */
static MEMORY_READ_START( safari_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0xe000, 0xe3ff, videoram_r },
	{ 0xe400, 0xe7ff, vicdual_ram_r },
	{ 0xe800, 0xefff, vicdual_characterram_r },
MEMORY_END

static MEMORY_WRITE_START( safari_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0xe000, 0xe3ff, videoram_w, &videoram, &videoram_size },
	{ 0xe400, 0xe7ff, vicdual_ram_w, &vicdual_ram },
	{ 0xe800, 0xefff, vicdual_characterram_w, &vicdual_characterram },
MEMORY_END


static PORT_READ_START( readport_2ports )
	{ 0x01, 0x01, input_port_0_r },
	{ 0x08, 0x08, input_port_1_r },
PORT_END

static PORT_READ_START( readport_3ports )
	{ 0x01, 0x01, input_port_0_r },
	{ 0x04, 0x04, input_port_1_r },
	{ 0x08, 0x08, input_port_2_r },
PORT_END

static PORT_READ_START( readport_4ports )
	{ 0x00, 0x00, input_port_0_r },
	{ 0x01, 0x01, input_port_1_r },
	{ 0x02, 0x02, input_port_2_r },
	{ 0x03, 0x03, input_port_3_r },
PORT_END

static PORT_READ_START( readport_safari )
	{ 0x03, 0x03, input_port_0_r },
	{ 0x08, 0x08, input_port_1_r },
PORT_END


static PORT_WRITE_START( writeport )
	{ 0x40, 0x40, vicdual_palette_bank_w },
PORT_END


static MEMORY_READ_START( i8039_readmem )
	{ 0x0000, 0x07ff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( i8039_writemem )
	{ 0x0000, 0x07ff, MWA_ROM },
MEMORY_END


static PORT_READ_START( i8039_readport )
	{ I8039_t1, I8039_t1, carnival_music_port_t1_r },
PORT_END

static PORT_WRITE_START( i8039_writeport )
	{ I8039_p1, I8039_p1, carnival_music_port_1_w },
	{ I8039_p2, I8039_p2, carnival_music_port_2_w },
PORT_END



INPUT_PORTS_START( depthch )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_DIPNAME (0x30, 0x30, DEF_STR( Coinage ) )
	PORT_DIPSETTING (   0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING (   0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING (   0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING (   0x30, DEF_STR( 1C_1C ) )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* probably unused */

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )  /* 64V */
	PORT_BIT( 0x7e, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* probably unused */
	PORT_BIT_IMPULSE( 0x80, IP_ACTIVE_LOW, IPT_COIN1 | IPF_RESETCPU, 30 )
INPUT_PORTS_END

INPUT_PORTS_START( safari )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_BUTTON2, "Aim Up", KEYCODE_A, IP_JOY_DEFAULT )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_BUTTON3, "Aim Down", KEYCODE_Z, IP_JOY_DEFAULT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* probably unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x0e, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* probably unused */
	PORT_DIPNAME (0x30, 0x30, DEF_STR( Coinage ) )
	PORT_DIPSETTING (   0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING (   0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING (   0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING (   0x30, DEF_STR( 1C_1C ) )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* probably unused */
	PORT_BIT_IMPULSE( 0x80, IP_ACTIVE_LOW, IPT_COIN1 | IPF_RESETCPU, 30 )
INPUT_PORTS_END

INPUT_PORTS_START( frogs )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )	/* The original joystick was a 3-way */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )	/* stick, of which Mame's 4-way does */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )	/* a fine simulation */
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Allow Free Game" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x20, "Time" )
	PORT_DIPSETTING(    0x00, "60" )
	PORT_DIPSETTING(    0x20, "90" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_1C ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x7e, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* probably unused */
	PORT_BIT_IMPULSE( 0x80, IP_ACTIVE_LOW, IPT_COIN1 | IPF_RESETCPU, 30 )
INPUT_PORTS_END

INPUT_PORTS_START( sspacaho )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_DIPNAME( 0x04, 0x04, "S.A. Lives (1/2)" )
	PORT_DIPSETTING(    0x00, "+0" )
	PORT_DIPSETTING(    0x04, "+1" )
	PORT_DIPNAME( 0x08, 0x00, "H.O. Lives" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x04, 0x00, "S.A. Lives (2/2)" )
	PORT_DIPSETTING(    0x00, "+0" )
	PORT_DIPSETTING(    0x04, "+2" )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x04, 0x00, "S.A. Bonus Life" )
	PORT_DIPSETTING(    0x00, "10000" )
 	PORT_DIPSETTING(    0x04, "15000" )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x04, 0x00, "S.A. Bonus Life For Final UFO" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT_IMPULSE( 0x08, IP_ACTIVE_LOW, IPT_COIN1 | IPF_RESETCPU, 30 )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_TOGGLE, "Game Select", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( sspaceat )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x01, 0x00, "Bonus Life For Final UFO" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0e, 0x0e, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x0e, "3" )
	PORT_DIPSETTING(    0x0c, "4" )
	PORT_DIPSETTING(    0x0a, "5" )
	PORT_DIPSETTING(    0x06, "6" )
/* the following are duplicates
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x02, "5" ) */
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "10000" )
	PORT_DIPSETTING(    0x10, "15000" )
	PORT_BIT( 0x60, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x80, 0x00, "Credits Display" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x7e, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT_IMPULSE( 0x80, IP_ACTIVE_LOW, IPT_COIN1 | IPF_RESETCPU, 30 )
INPUT_PORTS_END

INPUT_PORTS_START( headon )
	PORT_START	/* IN0 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x7e, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* probably unused */
	PORT_BIT_IMPULSE( 0x80, IP_ACTIVE_LOW, IPT_COIN1 | IPF_RESETCPU, 30 )
INPUT_PORTS_END

INPUT_PORTS_START( headon2 )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* probably unused */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )

	PORT_START	/* IN1 */
	PORT_BIT( 0x07, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x18, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x00, "6" )
/*	PORT_DIPSETTING(    0x08, "5" )*/
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* probably unused */

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_BIT( 0x7c, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* probably unused */
	PORT_BIT_IMPULSE( 0x80, IP_ACTIVE_LOW, IPT_COIN1 | IPF_RESETCPU, 30 )
INPUT_PORTS_END

INPUT_PORTS_START( invho2 )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x04, 0x04, "Head On Lives (1/2)" )
	PORT_DIPSETTING(    0x04, "+0" )
	PORT_DIPSETTING(    0x00, "+1" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x04, 0x00, "Head On Lives (2/2)" )
	PORT_DIPSETTING(    0x04, "+0" )
	PORT_DIPSETTING(    0x00, "+1" )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x04, 0x00, "Invinco Lives" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPSETTING(    0x04, "6" )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* timer - unused */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )	/* probably unused */

	PORT_START	/* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	/* There's probably a bug in the code: this would likely be the second */
	/* bit of the Invinco Lives setting, but the game reads bit 3 instead */
	/* of bit 2. */
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT_IMPULSE( 0x08, IP_ACTIVE_LOW, IPT_COIN1 | IPF_RESETCPU, 30 )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_TOGGLE, "Game Select", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( samurai )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* probably unused */
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_BITX(    0x08, 0x08, IPT_DIPSWITCH_NAME, "Infinite Lives", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* protection, see samurai_input_r() */
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) ) /* unknown, but used */
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_VBLANK ) /* seems to be on port 2 instead */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* protection, see samurai_input_r() */
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_VBLANK ) /* either vblank, or a timer. In the */
                                            /* Carnival schematics, it's a timer. */
/*	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )*/  /* timer */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* protection, see samurai_input_r() */
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT_IMPULSE( 0x08, IP_ACTIVE_LOW, IPT_COIN1 | IPF_RESETCPU, 30 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* probably unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( invinco )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* probably unused */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* probably unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* probably unused */

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x60, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x7e, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT_IMPULSE( 0x80, IP_ACTIVE_LOW, IPT_COIN1 | IPF_RESETCPU, 30 )
INPUT_PORTS_END

INPUT_PORTS_START( invds )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x04, 0x00, "Invinco Lives (1/2)" )
	PORT_DIPSETTING(    0x00, "+0" )
	PORT_DIPSETTING(    0x04, "+1" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x04, 0x00, "Invinco Lives (2/2)" )
	PORT_DIPSETTING(    0x00, "+0" )
	PORT_DIPSETTING(    0x04, "+2" )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x04, 0x00, "Deep Scan Lives (1/2)" )
	PORT_DIPSETTING(    0x00, "+0" )
	PORT_DIPSETTING(    0x04, "+1" )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* timer - unused */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	/* +1 and +2 gives 2 lives instead of 6 */
	PORT_DIPNAME( 0x04, 0x00, "Deep Scan Lives (2/2)" )
	PORT_DIPSETTING(    0x04, "+0" )
	PORT_DIPSETTING(    0x00, "+2" )
	PORT_BIT_IMPULSE( 0x08, IP_ACTIVE_LOW, IPT_COIN1 | IPF_RESETCPU, 30 )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_TOGGLE, "Game Select", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( tranqgun )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* timer */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT_IMPULSE( 0x08, IP_ACTIVE_LOW, IPT_COIN1 | IPF_RESETCPU, 30 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( spacetrk )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) ) /* unknown, but used */
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* must be high for bonus life to work */
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* timer - unused */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* must be high for bonus life to work */
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT_IMPULSE( 0x08, IP_ACTIVE_LOW, IPT_COIN1 | IPF_RESETCPU, 30 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( sptrekct )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) ) /* unknown, but used */
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* must be high for bonus life to work */
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* timer - unused */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* must be high for bonus life to work */
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT_IMPULSE( 0x08, IP_ACTIVE_LOW, IPT_COIN1 | IPF_RESETCPU, 30 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( carnival )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* probably unused */
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* probably unused */
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* probably unused */
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* timer - unused */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT_IMPULSE( 0x08, IP_ACTIVE_LOW, IPT_COIN1 | IPF_RESETCPU, 30 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( carnvckt )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT    | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* timer - unused */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT_IMPULSE( 0x08, IP_ACTIVE_LOW, IPT_COIN1 | IPF_RESETCPU, 30 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( brdrline )
	PORT_START	/* IN0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* probably unused */
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* timer - unused */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT_IMPULSE( 0x08, IP_ACTIVE_LOW, IPT_COIN1 | IPF_RESETCPU, 30 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( digger )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x60, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x7e, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT_IMPULSE( 0x80, IP_ACTIVE_LOW, IPT_COIN1 | IPF_RESETCPU, 30 )
INPUT_PORTS_END

INPUT_PORTS_START( pulsar )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x04, 0x04, "Lives (1/2)" )
	PORT_DIPSETTING(    0x04, "+0" )
	PORT_DIPSETTING(    0x00, "+2" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x04, 0x04, "Lives (2/2)" )
	PORT_DIPSETTING(    0x04, "+0" )
	PORT_DIPSETTING(    0x00, "+1" )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* probably unused */
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* timer - unused */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* probably unused */
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT_IMPULSE( 0x08, IP_ACTIVE_LOW, IPT_COIN1 | IPF_RESETCPU, 30 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( heiankyo )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) ) /* bonus life? */
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "2 Players Mode" )
	PORT_DIPSETTING(    0x08, "Alternating" )
	PORT_DIPSETTING(    0x00, "Simultaneous" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) ) /* bonus life? */
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) ) /* bonus life? */
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* timer - unused */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )	/* probably unused */

	PORT_START	/* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_BIT_IMPULSE( 0x08, IP_ACTIVE_LOW, IPT_COIN1 | IPF_RESETCPU, 30 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( alphaho )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_DIPNAME( 0x04, 0x00, "Alpha Fighter Lives (1/2)" )
	PORT_DIPSETTING(    0x00, "+0" )
	PORT_DIPSETTING(    0x04, "+1" )
	PORT_DIPNAME( 0x08, 0x00, "Head On Lives" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x04, 0x00, "Alpha Fighter Lives (2/2)" )
	PORT_DIPSETTING(    0x00, "+0" )
	PORT_DIPSETTING(    0x04, "+2" )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */

	PORT_START	/* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x04, 0x00, "Alpha Fighter Unknown" )	/* related to soccer frequency (code at 0x4950)*/
	PORT_DIPSETTING(    0x00, DEF_STR ( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR ( On ) )
	PORT_BIT_IMPULSE( 0x08, IP_ACTIVE_LOW, IPT_COIN1 | IPF_RESETCPU, 30 )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_TOGGLE, "Game Select", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	1,	/* 1 bit per pixel */
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },	/* pretty straightforward layout */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 0, 0xe800, &charlayout, 0, 32 },	/* the game dynamically modifies this */
	{ -1 }	/* end of array */
};



static struct Samplesinterface samples_interface_carnival =
{
 	12,	/* 12 channels */
 	50,	/* volume */
	carnival_sample_names
};

static struct Samplesinterface samples_interface_depthch =
{
	12,	/* 12 channels */
	50,	/* volume */
	depthch_sample_names
};

static struct Samplesinterface samples_interface_invinco3 =
{
	12,	/* 12 channels */
	50,	/* volume */
	invinco_sample_names
};

static struct Samplesinterface samples_interface_pulsar =
{
	12,	/* 12 channels */
	50,	/* volume */
	pulsar_sample_names
};


static MACHINE_DRIVER_START( 2ports )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 15468480/8)
	MDRV_CPU_MEMORY(vicdual_readmem,vicdual_writemem)
	MDRV_CPU_PORTS(readport_2ports,writeport)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(5000)	/* frames per second, vblank duration */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(64)

	MDRV_PALETTE_INIT(vicdual)
	MDRV_VIDEO_START(generic)
	MDRV_VIDEO_UPDATE(vicdual)

	/* sound hardware */
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( 3ports )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(2ports)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PORTS(readport_3ports,writeport)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( 4ports )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(2ports)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PORTS(readport_4ports,writeport)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( safari )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(2ports)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(safari_readmem,safari_writemem)
	MDRV_CPU_PORTS(readport_safari,writeport)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( depthch )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(2ports)

	/* sound hardware */
	MDRV_SOUND_ADD(SAMPLES, samples_interface_depthch)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( invinco3 )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(3ports)

	/* sound hardware */
	MDRV_SOUND_ADD(SAMPLES, samples_interface_invinco3)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( invinco4 )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(4ports)

	/* sound hardware */
	MDRV_SOUND_ADD(SAMPLES, samples_interface_invinco3)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( pulsar )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(4ports)

	/* sound hardware */
	MDRV_SOUND_ADD(SAMPLES, samples_interface_pulsar)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( frogs )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(2ports)
	MDRV_MACHINE_INIT(frogs)

	/* sound hardware */
	MDRV_SOUND_ADD(SAMPLES, frogs_samples_interface)
MACHINE_DRIVER_END

static struct AY8910interface carnival_ay8910_interface =
{
	1,	/* 1 chips */
	PSG_CLOCK_CARNIVAL,
	{ 35 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

/* don't know if any of the other games use the 8048 music board */
/* so, we won't burden those drivers with the extra music handling */
static MACHINE_DRIVER_START( carnival )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,15468480/8)
	MDRV_CPU_MEMORY(vicdual_readmem,vicdual_writemem)
	MDRV_CPU_PORTS(readport_4ports,writeport)

	MDRV_CPU_ADD(I8039,( ( 3579545 / 5 ) / 3 ))
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(i8039_readmem,i8039_writemem)
	MDRV_CPU_PORTS(i8039_readport,i8039_writeport)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(5000)
	MDRV_INTERLEAVE(10)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(64)

	MDRV_PALETTE_INIT(vicdual)
	MDRV_VIDEO_START(generic)
	MDRV_VIDEO_UPDATE(vicdual)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, carnival_ay8910_interface)
	MDRV_SOUND_ADD(SAMPLES, samples_interface_carnival)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( depthch )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "50a",          0x0000, 0x0400, CRC(56c5ffed) SHA1(f1e6cc322da93615d59850b3225a50f06fe58259) )
	ROM_LOAD( "51a",          0x0400, 0x0400, CRC(695eb81f) SHA1(f2491b8b9ce2dbb6d2606dcfaeb8658671f25400) )
	ROM_LOAD( "52",           0x0800, 0x0400, CRC(aed0ba1b) SHA1(cb7473e6b3c192953ae1832ab444545ddd85babb) )
	ROM_LOAD( "53",           0x0c00, 0x0400, CRC(2ccbd2d0) SHA1(76d8459bbad709666ce0c0be51f1d09e091983a2) )
	ROM_LOAD( "54a",          0x1000, 0x0400, CRC(1b7f6a43) SHA1(08d7864378b012a735eac4968f4dd86e36dc9d8d) )
	ROM_LOAD( "55a",          0x1400, 0x0400, CRC(9fc2eb41) SHA1(95a1684da346709908cd66bec06acfaeead596cf) )

	ROM_REGION( 0x0040, REGION_USER1, 0 )	/* misc PROMs, but no color so don't use REGION_PROMS! */
	ROM_LOAD( "316-0043.u87", 0x0000, 0x0020, CRC(e60a7960) SHA1(b8b8716e859c57c35310efc4594262afedb84823) )	/* control PROM */
	ROM_LOAD( "316-0042.u88", 0x0020, 0x0020, CRC(a1506b9d) SHA1(037c3db2ea40eca459e8acba9d1506dd28d72d10) )	/* sequence PROM */
ROM_END

ROM_START( depthv1 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD_NIB_LOW ( "316025.u63",  0x0000, 0x0400, CRC(bec75b9c) SHA1(8abe8b63be892e6abb7a886222b9eab40c5fcda0) )
	ROM_LOAD_NIB_HIGH( "316022.u51",  0x0000, 0x0400, CRC(977b7889) SHA1(dc1e874c2fd44709117474c5b210d67130ac361f) )
	ROM_LOAD_NIB_LOW ( "316030.u89",  0x0400, 0x0400, CRC(9e2bbb45) SHA1(be60d7330a160e15a8a822aa791aed3060b3b1db) )
	ROM_LOAD_NIB_HIGH( "316028.u77",  0x0400, 0x0400, CRC(597ae441) SHA1(8d3af5e64e838a57057d46f97a7b1c1037c1a0cf) )
	ROM_LOAD_NIB_LOW ( "316026.u64",  0x0800, 0x0400, CRC(61cc0802) SHA1(7173920f38188a1d1637cb2cb48e31cdd03a194e) )
	ROM_LOAD_NIB_HIGH( "316023.u52",  0x0800, 0x0400, CRC(9244b613) SHA1(6587035ec22d90194cdc3efaed3571a1ab975e1c) )
	ROM_LOAD_NIB_LOW ( "316031.u90",  0x0c00, 0x0400, CRC(861ffed1) SHA1(14e0a6a13726052000477c3586d99486167b8812) )
	ROM_LOAD_NIB_HIGH( "316029.u78",  0x0c00, 0x0400, CRC(53178634) SHA1(d8c4b70c3ab5144938ca0989300ad68e48391490) )
	ROM_LOAD_NIB_LOW ( "3160027.u65", 0x1000, 0x0400, CRC(4eecfc70) SHA1(1a1f6cc5da6df91e9e9016def65184201c3d2672) )
	ROM_LOAD_NIB_HIGH( "316024.u53",  0x1000, 0x0400, CRC(a9f55883) SHA1(78bfbc76f84657d32eb1b8072186b403729ea614) )
	ROM_LOAD_NIB_LOW ( "316049.u91",  0x1400, 0x0400, CRC(dc7eff35) SHA1(1915e92c09cba5868bd2e73ad395e19ddf47a3de) )
	ROM_LOAD_NIB_HIGH( "316048.u79",  0x1400, 0x0400, CRC(6e700621) SHA1(2b8db1cbbaf7808d4bf446435bbbbfc4d7761db8) )

	ROM_REGION( 0x0040, REGION_USER1, 0 )	/* misc PROMs, but no color so don't use REGION_PROMS! */
	ROM_LOAD( "316013.u27", 0x0000, 0x0020, CRC(690ef530) SHA1(6c0de3fa87a341cd378fefb8e06bf7918db9a074) )	/* control PROM */
	ROM_LOAD( "316014.u28", 0x0020, 0x0020, CRC(7b7a8492) SHA1(6ba8d891cc6eb0dd80051377b6b832e8894655e7) )	/* sequence PROM */
ROM_END

ROM_START( subhunt )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD_NIB_LOW ( "dp04",       0x0000, 0x0400, CRC(0ace1aef) SHA1(071256dd63e2e449093a65a4c9b006be5e17b786) )
	ROM_LOAD_NIB_HIGH( "dp01",       0x0000, 0x0400, CRC(da9e835b) SHA1(505c969b479aeab11bb6a21ef06837280846d90a) )
	ROM_LOAD_NIB_LOW ( "dp10",       0x0400, 0x0400, CRC(de752f20) SHA1(513a92554d14a09d6b80ba8017d161c7cda9ed8c) )
	ROM_LOAD_NIB_HIGH( "316028.u77", 0x0400, 0x0400, CRC(597ae441) SHA1(8d3af5e64e838a57057d46f97a7b1c1037c1a0cf) ) /* dp07*/
	ROM_LOAD_NIB_LOW ( "dp05",       0x0800, 0x0400, CRC(1c0530cf) SHA1(b1f2b1038ee063533669341f1a71755eecc2e1a9) )
	ROM_LOAD_NIB_HIGH( "316023.u52", 0x0800, 0x0400, CRC(9244b613) SHA1(6587035ec22d90194cdc3efaed3571a1ab975e1c) ) /* dp02*/
	ROM_LOAD_NIB_LOW ( "dp11",       0x0c00, 0x0400, CRC(0007044a) SHA1(c8d7c693e3059ff020563336fe712c234e94b8f9) )
	ROM_LOAD_NIB_HIGH( "dp08",       0x0c00, 0x0400, CRC(4d4e3ec8) SHA1(a0d5392fe5795cc6bf7373f194186506283c947c) )
	ROM_LOAD_NIB_LOW ( "dp06",       0x1000, 0x0400, CRC(63e1184b) SHA1(91934cb041365dabdc58a831312577fdb0dc923b) )
	ROM_LOAD_NIB_HIGH( "dp03",       0x1000, 0x0400, CRC(d70dbfd8) SHA1(0183a6b1ffd87a9e28588a7a9aa18aeb003560f0) )
	ROM_LOAD_NIB_LOW ( "dp12",       0x1400, 0x0400, CRC(170d7718) SHA1(4348e4e2dbb1edd9a4228fd3ccef58c50f1ae129) )
	ROM_LOAD_NIB_HIGH( "dp09",       0x1400, 0x0400, CRC(97466803) SHA1(f04ba4a1a960836974a85832596fc3a92a711094) )

	ROM_REGION( 0x0040, REGION_USER1, 0 )
	ROM_LOAD( "dp13", 0x0000, 0x0020, CRC(690ef530) SHA1(6c0de3fa87a341cd378fefb8e06bf7918db9a074) )
	ROM_LOAD( "dp14", 0x0020, 0x0020, CRC(7b7a8492) SHA1(6ba8d891cc6eb0dd80051377b6b832e8894655e7) )
ROM_END


ROM_START( safari )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "3160066.u48",  0x0000, 0x0400, CRC(2a26b098) SHA1(a16b04110fb142cec01c10460b14ec0c4e8d99af) )
	ROM_LOAD( "3160065.u47",  0x0400, 0x0400, CRC(b776f7db) SHA1(7332d1b18e1b199d87367182f185abafd9ad0bb1) )
	ROM_LOAD( "3160064.u46",  0x0800, 0x0400, CRC(19d8c196) SHA1(219dca308a4f917617cfe291580eb23fc2cb4687) )
	ROM_LOAD( "3160063.u45",  0x0c00, 0x0400, CRC(028bad25) SHA1(94120f197c15705d9447d4615b82e31b61672f89) )
	ROM_LOAD( "3160062.u44",  0x1000, 0x0400, CRC(504e0575) SHA1(069390941a0d79d623dce816fefef4d52b6e929f) )
	ROM_LOAD( "3160061.u43",  0x1400, 0x0400, CRC(d4c528e0) SHA1(8b28b70f4cdb12189bb7526d70e4df849a4b9c42) )
	ROM_LOAD( "3160060.u42",  0x1800, 0x0400, CRC(48c7b0cc) SHA1(26f757927212a01b2682ab520dd3b26a5524bdc3) )
	ROM_LOAD( "3160059.u41",  0x1c00, 0x0400, CRC(3f7baaff) SHA1(5f935cb2212718226cca10f4bcb28a5fdde109c7) )
	ROM_LOAD( "3160058.u40",  0x2000, 0x0400, CRC(0d5058f1) SHA1(00fd39a058e206b1bc5669438ab9670fa4db1921) )
	ROM_LOAD( "3160057.u39",  0x2400, 0x0400, CRC(298e8c41) SHA1(b9b6bc84d2531c85e4529c910d6e97ea83650ce3) )

	ROM_REGION( 0x0040, REGION_USER1, 0 )	/* misc PROMs, but no color so don't use REGION_PROMS! */
	ROM_LOAD( "316-0043.u87", 0x0000, 0x0020, CRC(e60a7960) SHA1(b8b8716e859c57c35310efc4594262afedb84823) )	/* control PROM */
	ROM_LOAD( "316-0042.u88", 0x0020, 0x0020, CRC(a1506b9d) SHA1(037c3db2ea40eca459e8acba9d1506dd28d72d10) )	/* sequence PROM */
ROM_END

ROM_START( frogs )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "119a.u48",     0x0000, 0x0400, CRC(b1d1fce4) SHA1(572015bede39b14526e93919b63b6d01ae38a09a) )
	ROM_LOAD( "118a.u47",     0x0400, 0x0400, CRC(12fdcc05) SHA1(06c6d17edec9fb03f46514c1f6c5d8c420ef4d05) )
	ROM_LOAD( "117a.u46",     0x0800, 0x0400, CRC(8a5be424) SHA1(ed8a09b4318929b83118f87e2da601028349f2bd) )
	ROM_LOAD( "116b.u45",     0x0c00, 0x0400, CRC(09b82619) SHA1(1063e268138b4ff6a8037d8d1a0816c34bbac690) )
	ROM_LOAD( "115a.u44",     0x1000, 0x0400, CRC(3d4e4fa8) SHA1(4655c4922328837af410cb298e0c296ae0099591) )
	ROM_LOAD( "114a.u43",     0x1400, 0x0400, CRC(04a21853) SHA1(1e84eb84d5770f54925055b748ab9ca2aa72c1cc) )
	ROM_LOAD( "113a.u42",     0x1800, 0x0400, CRC(02786692) SHA1(8a8937fd92beecf1119fe3f6b41a700725412aa1) )
	ROM_LOAD( "112a.u41",     0x1c00, 0x0400, CRC(0be2a058) SHA1(271f3b60cba422fff7e782fda198c3897c275b46) )
ROM_END

ROM_START( sspaceat )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "155.u27",      0x0000, 0x0400, CRC(ba7bb86f) SHA1(030e6f69d3ae00456fc02d1dc0fb915a81689df4) )
	ROM_LOAD( "156.u26",      0x0400, 0x0400, CRC(0b3a491c) SHA1(19cef304bb91f745797f27adbb9d334876d4fb78) )
	ROM_LOAD( "157.u25",      0x0800, 0x0400, CRC(3d3fac3b) SHA1(b22c2517af7c7077032d1b83e4628173d168e3ca) )
	ROM_LOAD( "158.u24",      0x0c00, 0x0400, CRC(843b80f6) SHA1(b61466d3546f1e0759ec84e841664cbe4d2a0a4d) )
	ROM_LOAD( "159.u23",      0x1000, 0x0400, CRC(1eacf60d) SHA1(52d5bfad4357619a9bdc7435e66ed5accadc6401) )
	ROM_LOAD( "160.u22",      0x1400, 0x0400, CRC(e61d482f) SHA1(d41550af1cb244adddff5c151fe2c140591c58ff) )
	ROM_LOAD( "161.u21",      0x1800, 0x0400, CRC(eb5e0993) SHA1(745135f50e50a8516a529a3caff27ee2580227f1) )
	ROM_LOAD( "162.u20",      0x1c00, 0x0400, CRC(5f84d550) SHA1(4fa2c48f843ad49b55598b2757e0e4e1e117aacb) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "316-0138.u44", 0x0000, 0x0020, CRC(67104ea9) SHA1(26b6bd2a1973b83bb9af4e3385d8cb14cb3f62f2) )
ROM_END

ROM_START( sspacat2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "81.u48",       0x0000, 0x0400, CRC(3e4b29f6) SHA1(ec99b7e156bad1f9f900fdebb289f0c9abf08647) )
	ROM_LOAD( "58.u47",       0x0400, 0x0400, CRC(176adb80) SHA1(9798d3b2d59fe4b7d26927b444746f135f0f0d8e) )
	ROM_LOAD( "59.u46",       0x0800, 0x0400, CRC(b2400d05) SHA1(12011fd91bbdfc94b02f9089be54d7cbb8dedece) )
	ROM_LOAD( "150.u45",      0x0c00, 0x0400, CRC(cf9bfa65) SHA1(3521bd2608705a83bd8d3daa0239708d2a8755e3) )
	ROM_LOAD( "151.u44",      0x1000, 0x0400, CRC(064530f1) SHA1(8278b271ae7d67e0b5433aefb150fd743ce6558a) )
	ROM_LOAD( "152.u43",      0x1400, 0x0400, CRC(c65c30fe) SHA1(849a0d46575ad0c72aceef28daa27911ec35181a) )
	ROM_LOAD( "153.u42",      0x1800, 0x0400, CRC(ea70c7f6) SHA1(656d113636224ec9c30982daa6a43877bc6ee58f) )
	ROM_LOAD( "156a.u41",     0x1c00, 0x0400, CRC(9029d2ce) SHA1(57d21650bfe9d76d874661443768213321acc56b) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "316-0138.u44", 0x0000, 0x0020, CRC(67104ea9) SHA1(26b6bd2a1973b83bb9af4e3385d8cb14cb3f62f2) )
ROM_END

ROM_START( sspacatc )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "139.u27",      0x0000, 0x0400, CRC(9f2112fc) SHA1(89c129ef1a95c5934a7c775994aafc91911b0051) )
	ROM_LOAD( "140.u26",      0x0400, 0x0400, CRC(ddbeed35) SHA1(48b33d7b35457675b545ca42c8afd79b86ce6035) )
	ROM_LOAD( "141.u25",      0x0800, 0x0400, CRC(b159924d) SHA1(320bbe156493f30a573ff548398f8f469e261e21) )
	ROM_LOAD( "142.u24",      0x0c00, 0x0400, CRC(f2ebfce9) SHA1(4792bca4a331bc41fd850760e6260e933063398f) )
	ROM_LOAD( "143.u23",      0x1000, 0x0400, CRC(bff34a66) SHA1(8a7490a13b9526c45f3afee1eee59d2b0096105f) )
	ROM_LOAD( "144.u22",      0x1400, 0x0400, CRC(fa062d58) SHA1(76e15e1d29b0e22ab310381d3d9faddf8912b205) )
	ROM_LOAD( "145.u21",      0x1800, 0x0400, CRC(7e950614) SHA1(0fe4de728ca550f0ef904b2d7d84fb2d56648401) )
	ROM_LOAD( "146.u20",      0x1c00, 0x0400, CRC(8ba94fbc) SHA1(371702c2d489fa0f1959734ebd35af45006712fa) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "316-0138.u44", 0x0000, 0x0020, CRC(67104ea9) SHA1(26b6bd2a1973b83bb9af4e3385d8cb14cb3f62f2) )
ROM_END

ROM_START( headon )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "163a",         0x0000, 0x0400, CRC(4bb51259) SHA1(43411ffda3fe03b1d694f70791b0bab5786759c0) )
	ROM_LOAD( "164a",         0x0400, 0x0400, CRC(aeac8c5f) SHA1(ef9ad63d13076a559ba12c6421ad61de21dd4c90) )
	ROM_LOAD( "165a",         0x0800, 0x0400, CRC(f1a0cb72) SHA1(540b30225ef176c416ea5b142fe7dbb67b7a78fb) )
	ROM_LOAD( "166c",         0x0c00, 0x0400, CRC(65d12951) SHA1(25fb0da2ea62a2b1ec214ce5c599a183e121b98a) )
	ROM_LOAD( "167c",         0x1000, 0x0400, CRC(2280831e) SHA1(128e7f7444440f113b3395dcb333281e0e8bef93) )
	ROM_LOAD( "192a",         0x1400, 0x0400, CRC(ed4666f2) SHA1(a12c22bfbb027eab3181627804b69129e89bd22c) )
	ROM_LOAD( "193a",         0x1800, 0x0400, CRC(37a1df4c) SHA1(45e1670351f0ef92ef4d9100b0e60ae598df4275) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "316-0138.u44", 0x0000, 0x0020, CRC(67104ea9) SHA1(26b6bd2a1973b83bb9af4e3385d8cb14cb3f62f2) )
ROM_END

ROM_START( headonb )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "163a",         0x0000, 0x0400, CRC(4bb51259) SHA1(43411ffda3fe03b1d694f70791b0bab5786759c0) )
	ROM_LOAD( "164a",         0x0400, 0x0400, CRC(aeac8c5f) SHA1(ef9ad63d13076a559ba12c6421ad61de21dd4c90) )
	ROM_LOAD( "165a",         0x0800, 0x0400, CRC(f1a0cb72) SHA1(540b30225ef176c416ea5b142fe7dbb67b7a78fb) )
	ROM_LOAD( "166b",         0x0c00, 0x0400, CRC(1c59008a) SHA1(430ecc3c2422d61af35eab77b96a480254572cc6) )
	ROM_LOAD( "167a",         0x1000, 0x0400, CRC(069e839e) SHA1(e1ed68573c13c0c88a2bb7b2096860523de952c0) )
	ROM_LOAD( "192a",         0x1400, 0x0400, CRC(ed4666f2) SHA1(a12c22bfbb027eab3181627804b69129e89bd22c) )
	ROM_LOAD( "193a-1",       0x1800, 0x0400, CRC(d3782c1d) SHA1(340782374b7015a0aaf98aeb6503b759e199a58a) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "316-0138.u44", 0x0000, 0x0020, CRC(67104ea9) SHA1(26b6bd2a1973b83bb9af4e3385d8cb14cb3f62f2) )
ROM_END

ROM_START( headon2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "u27.bin",      0x0000, 0x0400, CRC(fa47d2fb) SHA1(b3208f5bce228c453bdafbc9c1f2c8e1bd920d32) )
	ROM_LOAD( "u26.bin",      0x0400, 0x0400, CRC(61c47b15) SHA1(47619bd51fcaf47dd72e940c474f310c9287f2f4) )
	ROM_LOAD( "u25.bin",      0x0800, 0x0400, CRC(bb16db92) SHA1(f57dfbe52b0e545c7c889ac846dc7281d28f2698) )
	ROM_LOAD( "u24.bin",      0x0c00, 0x0400, CRC(17a09f24) SHA1(0cb40ec185f2ee3a26e943d84e8e2834d5f9d3ed) )
	ROM_LOAD( "u23.bin",      0x1000, 0x0400, CRC(0024895e) SHA1(60f81c383f1541555c26f7cf111a12a34f7f4f3e) )
	ROM_LOAD( "u22.bin",      0x1400, 0x0400, CRC(f798304d) SHA1(55526c6daead9b74a88e8bc0311155aa41a93210) )
	ROM_LOAD( "u21.bin",      0x1800, 0x0400, CRC(4c19dd40) SHA1(0bdfed47594c7aa5ff655b507350fc6a912b6855) )
	ROM_LOAD( "u20.bin",      0x1c00, 0x0400, CRC(25887ff2) SHA1(67cfb5ac93902b4c603f02c876c021ff453e5f0e) )

	ROM_REGION( 0x0060, REGION_PROMS, 0 )
	ROM_LOAD( "316-0138.u44", 0x0000, 0x0020, CRC(67104ea9) SHA1(26b6bd2a1973b83bb9af4e3385d8cb14cb3f62f2) )
	ROM_LOAD( "u65.bin",      0x0020, 0x0020, CRC(e60a7960) SHA1(b8b8716e859c57c35310efc4594262afedb84823) )	/* control PROM */
	ROM_LOAD( "u66.bin",      0x0040, 0x0020, CRC(a1506b9d) SHA1(037c3db2ea40eca459e8acba9d1506dd28d72d10) )	/* sequence PROM */
ROM_END

ROM_START( invho2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "271b.u33",     0x0000, 0x0400, CRC(44356a73) SHA1(6ff1050d84b6b7a006762c35e0b3d2befb0f90d6) )
	ROM_LOAD( "272b.u32",     0x0400, 0x0400, CRC(bd251265) SHA1(134f081c62173fab80b46918e4a073cf5f72df77) )
	ROM_LOAD( "273b.u31",     0x0800, 0x0400, CRC(2fc80cd9) SHA1(2790fb45233c8d6e74a56fcdfde1c468926d44d2) )
	ROM_LOAD( "274b.u30",     0x0c00, 0x0400, CRC(4fac4210) SHA1(9bb2fe888edab7e52a180d7f4d7fdab17392a736) )
	ROM_LOAD( "275b.u29",     0x1000, 0x0400, CRC(85af508e) SHA1(4b71c9583fe3b16a24f05d685d1edbd53ff81f81) )
	ROM_LOAD( "276b.u28",     0x1400, 0x0400, CRC(e305843a) SHA1(bb5113d3e0a4ca81e055da9c03755d0e6270d927) )
	ROM_LOAD( "277b.u27",     0x1800, 0x0400, CRC(b6b4221e) SHA1(8cfeff5ca7d29d973409df7f422428411462eab6) )
	ROM_LOAD( "278b.u26",     0x1c00, 0x0400, CRC(74d42250) SHA1(023227d314ec91c9b508b7fd60f163414165c25b) )
	ROM_LOAD( "279b.u8",      0x2000, 0x0400, CRC(8d30a3e0) SHA1(ac8cfca1b334d95e209bcfceeeeca31c03faecc8) )
	ROM_LOAD( "280b.u7",      0x2400, 0x0400, CRC(b5ee60ec) SHA1(dbc682b5770755fed8c04ef0f0311b2850228236) )
	ROM_LOAD( "281b.u6",      0x2800, 0x0400, CRC(21a6d4f2) SHA1(e8c8b263ffe53d5af50a561d038bfa96136767ad) )
	ROM_LOAD( "282b.u5",      0x2c00, 0x0400, CRC(07d54f8a) SHA1(f99af42ec24938cdd19c0ff7ac2b9e9882dc3655) )
	ROM_LOAD( "283b.u4",      0x3000, 0x0400, CRC(bdbe7ec1) SHA1(45e08c533acc538d88a0580535325b9ff1a60f2f) )
	ROM_LOAD( "284b.u3",      0x3400, 0x0400, CRC(ae9e9f16) SHA1(7f708ce49f34582d53abad0d811265dd28af899f) )
	ROM_LOAD( "285b.u2",      0x3800, 0x0400, CRC(8dc3ec34) SHA1(1f70027ad3b781244b672ee77225e32589c61e46) )
	ROM_LOAD( "286b.u1",      0x3c00, 0x0400, CRC(4bab9ba2) SHA1(a74881acf222466deb9c9e35dff532af6b10a7fc) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "316-0287.u49", 0x0000, 0x0020, CRC(d4374b01) SHA1(85ea0915f23571358e2e0c2b66b968e7b93f4bd6) )
ROM_END

ROM_START( sspacaho )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "epr00001.bin", 0x0000, 0x0800, CRC(ba62f57a) SHA1(7cfc079c6afe317b6c389c06802fdf1f83858510) )
	ROM_LOAD( "epr00002.bin", 0x0800, 0x0800, CRC(94b3c59c) SHA1(e6ee1c25fb45d03d514421c231d794f9da05f47f) )
	ROM_LOAD( "epr00003.bin", 0x1000, 0x0800, CRC(df13aef2) SHA1(61d210eeb59fe132e14fdd7eb6a39ebc55168097) )
	ROM_LOAD( "epr00004.bin", 0x1800, 0x0800, CRC(8431e15e) SHA1(b028e718ee90f37c848e5f83494be61cb90338e2) )
	ROM_LOAD( "epr00005.bin", 0x2000, 0x0800, CRC(eec2b6e7) SHA1(4ed830755b4d1da6111afc6c16c7c633521ccb9c) )
	ROM_LOAD( "epr00006.bin", 0x2800, 0x0800, CRC(780e47ed) SHA1(533879b8abf0e69644fd8b784dbe9bf10cde6d9f) )
	ROM_LOAD( "epr00007.bin", 0x3000, 0x0800, CRC(8189a2fa) SHA1(3c13a394f48adb8f7c6b8203bd0749921461ea06) )
	ROM_LOAD( "epr00008.bin", 0x3800, 0x0800, CRC(34a64a80) SHA1(a588fae0ecaa80677887d8c95ef8896a4bdd77ee) )

	ROM_REGION( 0x0060, REGION_PROMS, 0 )
	ROM_LOAD( "316-0138.u44", 0x0000, 0x0020, CRC(67104ea9) SHA1(26b6bd2a1973b83bb9af4e3385d8cb14cb3f62f2) )
	ROM_LOAD( "316-0043.u87", 0x0020, 0x0020, CRC(e60a7960) SHA1(b8b8716e859c57c35310efc4594262afedb84823) )
	ROM_LOAD( "316-0042.u88", 0x0040, 0x0020, CRC(a1506b9d) SHA1(037c3db2ea40eca459e8acba9d1506dd28d72d10) )
ROM_END

ROM_START( samurai )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "epr289.u33",   0x0000, 0x0400, CRC(a1a9cb03) SHA1(1875a86ad5938295dd5db6bb045be46eba8638ba) )
	ROM_LOAD( "epr290.u32",   0x0400, 0x0400, CRC(49fede51) SHA1(58ab1779d555281ec436ae90dcdf4ada42625892) )
	ROM_LOAD( "epr291.u31",   0x0800, 0x0400, CRC(6503dd72) SHA1(e0a3f42418a13f38314ec6e7951cd45c686fecbc) )
	ROM_LOAD( "epr292.u30",   0x0c00, 0x0400, CRC(179c224f) SHA1(0a202718a9c2f5f4f1553d1dccd99bebb511363f) )
	ROM_LOAD( "epr366.u29",   0x1000, 0x0400, CRC(3df2abec) SHA1(f52182c93026de0f5e7a3c36fe4a35e386c95d0c) )
	ROM_LOAD( "epr355.u28",   0x1400, 0x0400, CRC(b24517a4) SHA1(51d613ac21e33c70705f1731b905af72dc561dbf) )
	ROM_LOAD( "epr367.u27",   0x1800, 0x0400, CRC(992a6e5a) SHA1(45ae4bf297d7ce52d879d3af5e26960c0dd5034c) )
	ROM_LOAD( "epr368.u26",   0x1c00, 0x0400, CRC(403c72ce) SHA1(eaa7a56a28b393db97fd6f1a62b7592cd47060f0) )
	ROM_LOAD( "epr369.u8",    0x2000, 0x0400, CRC(3cfd115b) SHA1(58d1d34605b75f0078eb7e61eca8d5897a8b8294) )
	ROM_LOAD( "epr370.u7",    0x2400, 0x0400, CRC(2c30db12) SHA1(9dcf8aa5cfdda5b350fc6b70524a15f970d98d91) )
	ROM_LOAD( "epr299.u6",    0x2800, 0x0400, CRC(87c71139) SHA1(10c273c2f6a58bba8b5891dfd851e18898b45fd1) )
	ROM_LOAD( "epr371.u5",    0x2c00, 0x0400, CRC(761f56cf) SHA1(0d63e8b0ac7bfe9f3cd08cea68eb941b1a5a536c) )
	ROM_LOAD( "epr301.u4",    0x3000, 0x0400, CRC(23de1ff7) SHA1(51bf437a62ee770918524c8d0e8b0e007a800021) )
	ROM_LOAD( "epr372.u3",    0x3400, 0x0400, CRC(292cfd89) SHA1(a8131f03e8e9f5009508813445bbea559bc27726) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "pr55.clr",     0x0000, 0x0020, CRC(975f5fb0) SHA1(d5917d68ad5549fe5cc997521c3b0a5a279d2231) )

	ROM_REGION( 0x0040, REGION_USER1, 0 )	/* misc PROMs */
	ROM_LOAD( "316-0043.u87", 0x0000, 0x0020, CRC(e60a7960) SHA1(b8b8716e859c57c35310efc4594262afedb84823) )	/* control PROM */
	ROM_LOAD( "316-0042.u88", 0x0020, 0x0020, CRC(a1506b9d) SHA1(037c3db2ea40eca459e8acba9d1506dd28d72d10) )	/* sequence PROM */
ROM_END

ROM_START( invinco )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "310a.u27",     0x0000, 0x0400, CRC(e3931365) SHA1(e34083004515ad45ddbf9ab89c34473b6c5d46fb) )
	ROM_LOAD( "311a.u26",     0x0400, 0x0400, CRC(de1a6c4a) SHA1(ca7ab7b4c77319f7923d56ad8b60d16211af19bc) )
	ROM_LOAD( "312a.u25",     0x0800, 0x0400, CRC(e3c08f39) SHA1(13c177980722559ec885565d3f889b830322f25e) )
	ROM_LOAD( "313a.u24",     0x0c00, 0x0400, CRC(b680b306) SHA1(b03a5621bdf6a0bdc79af9361a2c0cc339b50b4b) )
	ROM_LOAD( "314a.u23",     0x1000, 0x0400, CRC(790f07d9) SHA1(222e1c392c92fc547a67d96410f9a4277f3bb6cb) )
	ROM_LOAD( "315a.u22",     0x1400, 0x0400, CRC(0d13bed2) SHA1(c23d937acad2171aae8f76671843a841f62d147f) )
	ROM_LOAD( "316a.u21",     0x1800, 0x0400, CRC(88d7eab8) SHA1(272b099414b68a428f9538f0219f92569525b87c) )
	ROM_LOAD( "317a.u20",     0x1c00, 0x0400, CRC(75389463) SHA1(277ba127f322caf2486735b82bf7b96bb9e34a56) )
	ROM_LOAD( "318a.uxx",     0x2000, 0x0400, CRC(0780721d) SHA1(580aefb382702babdf8248e36330c4b22e8579c8) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "316-246.u44",  0x0000, 0x0020, CRC(fe4406cb) SHA1(92e2459420a7f7412f02cfaf68604fc233b0a245) )
ROM_END

ROM_START( invds )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "367.u33",      0x0000, 0x0400, CRC(e6a33eae) SHA1(16de70e8fcd093964a448a86bc89b1c607152ead) )
	ROM_LOAD( "368.u32",      0x0400, 0x0400, CRC(421554a8) SHA1(efb6759998e36322258c172aa7e8ba6416b3235f) )
	ROM_LOAD( "369.u31",      0x0800, 0x0400, CRC(531e917a) SHA1(f1d48e18f51de36d01bafab5bfa68d16a8c0192b) )
	ROM_LOAD( "370.u30",      0x0c00, 0x0400, CRC(2ad68f8c) SHA1(0ceee2d04e239856ecf18f93b5ee75d72e031f92) )
	ROM_LOAD( "371.u29",      0x1000, 0x0400, CRC(1b98dc5c) SHA1(ac8a1226405cab9d6049304163d658c5ae611c1a) )
	ROM_LOAD( "372.u28",      0x1400, 0x0400, CRC(3a72190a) SHA1(f2852b3ca5fce274ed5452d28d06a3a27fe8a94f) )
	ROM_LOAD( "373.u27",      0x1800, 0x0400, CRC(3d361520) SHA1(bf8441cd5050f643535229b4761310f7dc8a2997) )
	ROM_LOAD( "374.u26",      0x1c00, 0x0400, CRC(e606e7d9) SHA1(66efa2beda9fcc2f3cd8fc77c99c196de30d1a30) )
	ROM_LOAD( "375.u8",       0x2000, 0x0400, CRC(adbe8d32) SHA1(5c883f0053c4f8124b81c49847b00301edb9f654) )
	ROM_LOAD( "376.u7",       0x2400, 0x0400, CRC(79409a46) SHA1(12772cf3e06d5127adb16396c4589cd9a06cbce5) )
	ROM_LOAD( "377.u6",       0x2800, 0x0400, CRC(3f021a71) SHA1(ba4a3833c6f7d28c3033de77882ff7ee0b96b190) )
	ROM_LOAD( "378.u5",       0x2c00, 0x0400, CRC(49a542b0) SHA1(5aa90df413ac0498cadaff9ca805896066e0f8a8) )
	ROM_LOAD( "379.u4",       0x3000, 0x0400, CRC(ee140e49) SHA1(5c29e34afdf91b68554e678699179802616c1f94) )
	ROM_LOAD( "380.u3",       0x3400, 0x0400, CRC(688ba831) SHA1(24f977c99714a08bf14c7753d42e2744aa86c396) )
	ROM_LOAD( "381.u2",       0x3800, 0x0400, CRC(798ba0c7) SHA1(7867d107f95077c539263619155c7f3ce4ef0968) )
	ROM_LOAD( "382.u1",       0x3c00, 0x0400, CRC(8d195c24) SHA1(5c314947ba13112b4154d3be069892cca4f5da42) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "316-246",      0x0000, 0x0020, CRC(fe4406cb) SHA1(92e2459420a7f7412f02cfaf68604fc233b0a245) )

	ROM_REGION( 0x0020, REGION_USER1, 0 )	/* misc PROM */
	ROM_LOAD( "316-0206.u14", 0x0000, 0x0020, CRC(9617d796) SHA1(7cff2741866095ff42eadd8022bea349ec8d2f39) )	/* control PROM */
ROM_END

ROM_START( tranqgun )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "u33.bin",      0x0000, 0x0400, CRC(6d50e902) SHA1(1d14c0b28cb3650bb57b9ef61265fe94c453d648) )
	ROM_LOAD( "u32.bin",      0x0400, 0x0400, CRC(f0ba0e60) SHA1(fcdd4355ccc1893a8a0450403f466bee916793dc) )
	ROM_LOAD( "u31.bin",      0x0800, 0x0400, CRC(9fe440d3) SHA1(a0be6caf3ea821c84be83351a0b80c240485218f) )
	ROM_LOAD( "u30.bin",      0x0c00, 0x0400, CRC(1041608e) SHA1(9f9667a4ad98f43c606b534157488fb7dd7b18ba) )
	ROM_LOAD( "u29.bin",      0x1000, 0x0400, CRC(fb5de95f) SHA1(7ff57c2e7b074e74936fde808f455a118dc495c2) )
	ROM_LOAD( "u28.bin",      0x1400, 0x0400, CRC(03fd8727) SHA1(d334c50adcaea02f9b55b4ec1b4c7dda3a0298ca) )
	ROM_LOAD( "u27.bin",      0x1800, 0x0400, CRC(3d93239b) SHA1(211748992f5735cf4b8a69f035aa89b40c814d75) )
	ROM_LOAD( "u26.bin",      0x1c00, 0x0400, CRC(20f64a7f) SHA1(e61e7dd0f418c17954b72d443a1de206ef8cf4d0) )
	ROM_LOAD( "u8.bin",       0x2000, 0x0400, CRC(5121c695) SHA1(6b8ae30f2f17ff886e6c61783613a60b5c3d9b82) )
	ROM_LOAD( "u7.bin",       0x2400, 0x0400, CRC(b13d21f7) SHA1(f624e96c1dada3222b6b5a83f0bdf52b821077d6) )
	ROM_LOAD( "u6.bin",       0x2800, 0x0400, CRC(603cee59) SHA1(7c1f55cfe36cf52687f1c60d9277b46bd12054d4) )
	ROM_LOAD( "u5.bin",       0x2c00, 0x0400, CRC(7f25475f) SHA1(f519d59872d9429300040cb0ce940e8103087763) )
	ROM_LOAD( "u4.bin",       0x3000, 0x0400, CRC(57dc3123) SHA1(db57cb77d840bc97d0fa9de28cc4eba90b0319d0) )
	ROM_LOAD( "u3.bin",       0x3400, 0x0400, CRC(7aa7829b) SHA1(ef9835d83d4468a28aa2a837b3c23760d9e5480e) )
	ROM_LOAD( "u2.bin",       0x3800, 0x0400, CRC(a9b10df5) SHA1(f74e83c5f51ecb53e3190c1ae83fa59b60806f62) )
	ROM_LOAD( "u1.bin",       0x3c00, 0x0400, CRC(431a7449) SHA1(67a142c084078ebebe9ed91b577fffe1ab0f39a2) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "u49.bin",      0x0000, 0x0020, CRC(6481445b) SHA1(2136408f25a95ed73882deaa5a174d4a1a7ba438) )

	ROM_REGION( 0x0040, REGION_USER1, 0 )	/* misc PROMs */
	ROM_LOAD( "316-0043.u87", 0x0000, 0x0020, CRC(e60a7960) SHA1(b8b8716e859c57c35310efc4594262afedb84823) )	/* control PROM */
	ROM_LOAD( "316-0042.u88", 0x0020, 0x0020, CRC(a1506b9d) SHA1(037c3db2ea40eca459e8acba9d1506dd28d72d10) )	/* sequence PROM */
ROM_END

ROM_START( spacetrk )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "u33.bin",      0x0000, 0x0400, CRC(9033fe50) SHA1(0a9b86af03956575403d8b494963f55887fc4dc3) )
	ROM_LOAD( "u32.bin",      0x0400, 0x0400, CRC(08f61f0d) SHA1(f206b18959e2cb6d4f6962415695eca0412da739) )
	ROM_LOAD( "u31.bin",      0x0800, 0x0400, CRC(1088a8c4) SHA1(ca40098137f0d90548d62a4daead3c6bc488fde0) )
	ROM_LOAD( "u30.bin",      0x0c00, 0x0400, CRC(55560cc8) SHA1(751585e261088cc740aa293a1e71f13b928c37f9) )
	ROM_LOAD( "u29.bin",      0x1000, 0x0400, CRC(71713958) SHA1(85399d3cbe5ed265b59b4441c17247b09c4c34d0) )
	ROM_LOAD( "u28.bin",      0x1400, 0x0400, CRC(7bcf5ca3) SHA1(05a6d31c0229e840081cad7fb17bdebc0fd6484f) )
	ROM_LOAD( "u27.bin",      0x1800, 0x0400, CRC(ad7a2065) SHA1(04a98b4578c7770f69525c614c402ccf2f3c99ed) )
	ROM_LOAD( "u26.bin",      0x1c00, 0x0400, CRC(6060fe77) SHA1(d7606daabdb0212daef6724b0ba6abab13731080) )
	ROM_LOAD( "u8.bin",       0x2000, 0x0400, CRC(75a90624) SHA1(61f10dd66ba1daa1b37c407d22d2ea3da2c8fec8) )
	ROM_LOAD( "u7.bin",       0x2400, 0x0400, CRC(7b31a2ab) SHA1(58ebf8ccb2e807aacc24c0017cb8c094124c1776) )
	ROM_LOAD( "u6.bin",       0x2800, 0x0400, CRC(94135b33) SHA1(deb836034ab1de016a26d45e7176f292aa34038c) )
	ROM_LOAD( "u5.bin",       0x2c00, 0x0400, CRC(cfbf2538) SHA1(d200ae1a866a6e4587226e449a3d09d330a79cf4) )
	ROM_LOAD( "u4.bin",       0x3000, 0x0400, CRC(b4b95129) SHA1(04c4ced1b8d0bf2cbb2533d9870574395a5f0793) )
	ROM_LOAD( "u3.bin",       0x3400, 0x0400, CRC(03ca1d70) SHA1(66ed7664ee1ddbadd6993d44c3684e6cb47912b3) )
	ROM_LOAD( "u2.bin",       0x3800, 0x0400, CRC(a968584b) SHA1(4ea4b83e116627daec4d226591924f84740e7786) )
	ROM_LOAD( "u1.bin",       0x3c00, 0x0400, CRC(e6e300e8) SHA1(4e198915c4f2b904dcc9ad81e5106bd711546e4e) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "u49.bin",      0x0000, 0x0020, CRC(aabae4cd) SHA1(6748d20318aed1c9949a3373166ebdca13eae965) )

	ROM_REGION( 0x0040, REGION_USER1, 0 )	/* misc PROMs */
	ROM_LOAD( "316-0043.u87", 0x0000, 0x0020, CRC(e60a7960) SHA1(b8b8716e859c57c35310efc4594262afedb84823) )	/* control PROM */
	ROM_LOAD( "316-0042.u88", 0x0020, 0x0020, CRC(a1506b9d) SHA1(037c3db2ea40eca459e8acba9d1506dd28d72d10) )	/* sequence PROM */
ROM_END

ROM_START( sptrekct )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "u33c.bin",     0x0000, 0x0400, CRC(b056b928) SHA1(1bbf5c30b226c5ca3c09fcff36a1b21132a524b6) )
	ROM_LOAD( "u32c.bin",     0x0400, 0x0400, CRC(dffb11d9) SHA1(5c95b7e493ac9e8714d91d19b6f01967559ce55c) )
	ROM_LOAD( "u31c.bin",     0x0800, 0x0400, CRC(9b25d46f) SHA1(29362fe4587f6425cc55c6f5592f853caf8bc1e2) )
	ROM_LOAD( "u30c.bin",     0x0c00, 0x0400, CRC(3a612bfe) SHA1(914a9a1192bfb255ee8b72671d33b5ae17b3a1b7) )
	ROM_LOAD( "u29c.bin",     0x1000, 0x0400, CRC(d8bb6e0c) SHA1(6bf57dde384a447350deb7125e287af147d51878) )
	ROM_LOAD( "u28c.bin",     0x1400, 0x0400, CRC(0e367740) SHA1(5140244d4cd64727ddfe052b7b4880e9599998f9) )
	ROM_LOAD( "u27c.bin",     0x1800, 0x0400, CRC(d59fec86) SHA1(8f0163aa61fb35a9977e24c36f7c01c3fb0bc156) )
	ROM_LOAD( "u26c.bin",     0x1c00, 0x0400, CRC(9deefa0f) SHA1(d109743e737931227bb504c47ba3cc959ae24756) )
	ROM_LOAD( "u8c.bin",      0x2000, 0x0400, CRC(613116c5) SHA1(23ad643a3df0d3823b776422454cba2e590a49c6) )
	ROM_LOAD( "u7c.bin",      0x2400, 0x0400, CRC(3bdf2464) SHA1(a19a863da8fb33650483ba5b4ea133f1c6393620) )
	ROM_LOAD( "u6c.bin",      0x2800, 0x0400, CRC(039d73fa) SHA1(08498e76b4f29f22078dfbe7ec52f47528f6db42) )
	ROM_LOAD( "u5c.bin",      0x2c00, 0x0400, CRC(1638344f) SHA1(0f8488763f3f3776ab09eb5cf1d4db8ebd384345) )
	ROM_LOAD( "u4c.bin",      0x3000, 0x0400, CRC(e34443cd) SHA1(8f2302f48aaceae70d8e576f318f44e96f92d4f3) )
	ROM_LOAD( "u3c.bin",      0x3400, 0x0400, CRC(6f16cbd7) SHA1(ef32558731d374a1417bb58ba24c1aa79d71ecd8) )
	ROM_LOAD( "u2c.bin",      0x3800, 0x0400, CRC(94da3cdc) SHA1(082f96c6fad0ccb314e80efd3a2d7e156a55c48e) )
	ROM_LOAD( "u1c.bin",      0x3c00, 0x0400, CRC(2a228bf4) SHA1(7c39700c7a63ca3202134acefa8e6f04f7145dfd) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "u49.bin",      0x0000, 0x0020, CRC(aabae4cd) SHA1(6748d20318aed1c9949a3373166ebdca13eae965) )

	ROM_REGION( 0x0040, REGION_USER1, 0 )	/* misc PROMs */
	ROM_LOAD( "316-0043.u87", 0x0000, 0x0020, CRC(e60a7960) SHA1(b8b8716e859c57c35310efc4594262afedb84823) )	/* control PROM */
	ROM_LOAD( "316-0042.u88", 0x0020, 0x0020, CRC(a1506b9d) SHA1(037c3db2ea40eca459e8acba9d1506dd28d72d10) )	/* sequence PROM */
ROM_END

ROM_START( carnival )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "651u33.cpu",   0x0000, 0x0400, CRC(9f2736e6) SHA1(c3fb9197b5e83dc7d5335de2268e0acb30cf8328) )
	ROM_LOAD( "652u32.cpu",   0x0400, 0x0400, CRC(a1f58beb) SHA1(e027beca7bf3ef5ef67e2195f909332fd194b5dc) )
	ROM_LOAD( "653u31.cpu",   0x0800, 0x0400, CRC(67b17922) SHA1(46cdfd0371dec61a5440c2111660729c0f0ecdb8) )
	ROM_LOAD( "654u30.cpu",   0x0c00, 0x0400, CRC(befb09a5) SHA1(da44b6a869b5eb0705e01fee4478696f6bef9de8) )
	ROM_LOAD( "655u29.cpu",   0x1000, 0x0400, CRC(623fcdad) SHA1(35890964f5cf799c141002916641089ccec0fcc9) )
	ROM_LOAD( "656u28.cpu",   0x1400, 0x0400, CRC(53040332) SHA1(ff7a06d93cb890abf0616770774668396d128ba3) )
	ROM_LOAD( "657u27.cpu",   0x1800, 0x0400, CRC(f2537467) SHA1(262b859098f4f7e5e9bf2f83bda833044824226e) )
	ROM_LOAD( "658u26.cpu",   0x1c00, 0x0400, CRC(fcc3854e) SHA1(7adbd6ca6f636dec75fa6eccdf3381686e074bc6) )
	ROM_LOAD( "659u8.cpu",    0x2000, 0x0400, CRC(28be8d69) SHA1(2d9ac9a53f00fe2282e4317585e6bddadb676c0f) )
	ROM_LOAD( "660u7.cpu",    0x2400, 0x0400, CRC(3873ccdb) SHA1(56be81fdee8947758ba966915c0739e5560a7f94) )
	ROM_LOAD( "661u6.cpu",    0x2800, 0x0400, CRC(d9a96dff) SHA1(0366acf3418901bfeeda59d4cd51fe8ceaad4577) )
	ROM_LOAD( "662u5.cpu",    0x2c00, 0x0400, CRC(d893ca72) SHA1(564176ab7f3757d51db8eef9fbc4228fa2ce328f) )
	ROM_LOAD( "663u4.cpu",    0x3000, 0x0400, CRC(df8c63c5) SHA1(e8d0632b5cb5bd7f698485531f3edeb13efdc685) )
	ROM_LOAD( "664u3.cpu",    0x3400, 0x0400, CRC(689a73e8) SHA1(b4134e8d892df7ba3352e4d3f581923decae6e54) )
	ROM_LOAD( "665u2.cpu",    0x3800, 0x0400, CRC(28e7b2b6) SHA1(57eb5dd0f11da8ff8001e76036264246d6bc27d2) )
	ROM_LOAD( "666u1.cpu",    0x3c00, 0x0400, CRC(4eec7fae) SHA1(cdc858165136c55b01511805c9d4dc6bc598fe1f) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "316-633",      0x0000, 0x0020, CRC(f0084d80) SHA1(95ec912ac2c64cd58a50c68afc0993746841a531) )

	ROM_REGION( 0x0800, REGION_CPU2, 0 )	/* sound ROM */
	ROM_LOAD( "crvl.snd",     0x0000, 0x0400, CRC(0dbaa2b0) SHA1(eae7fc362a0ff8f908c42e093c7dbb603659373c) )
ROM_END

ROM_START( carnvckt )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "epr501",       0x0000, 0x0400, CRC(688503d2) SHA1(a1fe03c23276d458ba74f7473524918eb9b7c7e5) )
	ROM_LOAD( "652u32.cpu",   0x0400, 0x0400, CRC(a1f58beb) SHA1(e027beca7bf3ef5ef67e2195f909332fd194b5dc) )
	ROM_LOAD( "653u31.cpu",   0x0800, 0x0400, CRC(67b17922) SHA1(46cdfd0371dec61a5440c2111660729c0f0ecdb8) )
	ROM_LOAD( "654u30.cpu",   0x0c00, 0x0400, CRC(befb09a5) SHA1(da44b6a869b5eb0705e01fee4478696f6bef9de8) )
	ROM_LOAD( "655u29.cpu",   0x1000, 0x0400, CRC(623fcdad) SHA1(35890964f5cf799c141002916641089ccec0fcc9) )
	ROM_LOAD( "epr506",       0x1400, 0x0400, CRC(ba916e97) SHA1(87e1ac8bd21bafb815b702048c6be24f410a8998) )
	ROM_LOAD( "epr507",       0x1800, 0x0400, CRC(d0bda4a5) SHA1(faf3f3c2a8f962c6c2901e5a4a31b452b506ee22) )
	ROM_LOAD( "epr508",       0x1c00, 0x0400, CRC(f0258cad) SHA1(065c90835dadeda7085422295cde09491c94b6e0) )
	ROM_LOAD( "epr509",       0x2000, 0x0400, CRC(dcc8a530) SHA1(2d1cac3b40f5afab5423d45ecc3f5565266f9c03) )
	ROM_LOAD( "epr510",       0x2400, 0x0400, CRC(92c2ba51) SHA1(c0c0c836d2aa6943bd808acc12161adf23cd0d21) )
	ROM_LOAD( "epr511",       0x2800, 0x0400, CRC(3af899a0) SHA1(540cbdc6a912cb325bbea33935bfa06e13f0021a) )
	ROM_LOAD( "epr512",       0x2c00, 0x0400, CRC(09f7b3e6) SHA1(18a58680500148e7a031e91230d73b2ce4dc712d) )
	ROM_LOAD( "epr513",       0x3000, 0x0400, CRC(8f41974c) SHA1(65e9c2378ad99f804590de36ffba4c60bfa1bfe3) )
	ROM_LOAD( "epr514",       0x3400, 0x0400, CRC(2788d140) SHA1(7341c44fb1f7eb8c4d25d3e1e75bcf3bfdb9a89c) )
	ROM_LOAD( "epr515",       0x3800, 0x0400, CRC(10decaa9) SHA1(4c980164dde275e1488ae74a7ad61e6acc75e608) )
	ROM_LOAD( "epr516",       0x3c00, 0x0400, CRC(7c32b352) SHA1(8cb472a7f71a301417c6a8e4a26a9bdcd43b6062) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "316-633",      0x0000, 0x0020, CRC(f0084d80) SHA1(95ec912ac2c64cd58a50c68afc0993746841a531) )

	ROM_REGION( 0x0800, REGION_CPU2, 0 )	/* sound ROM */
	ROM_LOAD( "crvl.snd",     0x0000, 0x0400, CRC(0dbaa2b0) SHA1(eae7fc362a0ff8f908c42e093c7dbb603659373c) )
ROM_END

ROM_START( brdrlinb )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "border1.33",   0x0000, 0x0800, CRC(48387706) SHA1(b4db2f05e722812370b0b24cd15061d6fc578560) ) /* karateco*/
	ROM_LOAD( "border2.30",   0x0800, 0x0800, CRC(1d669b60) SHA1(47ef5141591e177419fa352968be26ecc6fafd89) )
	ROM_LOAD( "border3.29",   0x1000, 0x0800, CRC(6e4d6fb3) SHA1(4747359c4f09e93c563b93ee1189743894332b47) )
	ROM_LOAD( "border4.27",   0x1800, 0x0800, CRC(718446d8) SHA1(cc15cb37ebb51970fcdebf74ab4308a25b40af2a) )
	ROM_LOAD( "border5.08",   0x2000, 0x0800, CRC(a0584337) SHA1(1fd9c60dc42a178c88d4e4e4b4d5de495ea906c6) )
	ROM_LOAD( "border6.06",   0x2800, 0x0800, CRC(cb30fb98) SHA1(96ea83111e8ccf409fe40f19ce200a50ea9ea288) )
	ROM_LOAD( "border7.04",   0x3000, 0x0800, CRC(200c5321) SHA1(f9faa6125dd2adc69c4ba9d962c0998f815ccd1c) )
	ROM_LOAD( "border8.02",   0x3800, 0x0800, CRC(735e140d) SHA1(1c0b6cf2d8c88601084dfcb8d7490b85ef1a86d5) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "borderc.49",      0x0000, 0x0020, CRC(bc6be94e) SHA1(34e113ec25e19212b74907d35be5cb8714a8249c) )

	ROM_REGION( 0x0800, REGION_CPU2, 0 )	/* sound ROM */
	ROM_LOAD( "bords.bin",     0x0000, 0x0400, CRC(a23e1d9f) SHA1(ce209571f6341aa6f036a015e666673098bc98ea) )

	ROM_REGION( 0x0020, REGION_USER1, 0 )	/* misc PROM */
	ROM_LOAD( "border.32", 0x0000, 0x0020, CRC(c128d0ba) SHA1(0ce9febbb7e2f5388ed999a479e3d385dba0b342) )
	ROM_LOAD( "bordera.15",0x0000, 0x0020, CRC(6449e678) SHA1(421c45c8fba3c2bc2a7ebbea2c837c8fa1a5a2f3) )
	ROM_LOAD( "borderb.14",0x0000, 0x0020, CRC(55dcdef1) SHA1(6fbd041edc258b7e1b99bbe9526612cfb1b541f8) )
ROM_END

ROM_START( digger )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "684.u27",      0x0000, 0x0400, CRC(bba0d7c2) SHA1(1e55dd95b07b562dcc1e52ecf9460d302b14ee60) )
	ROM_LOAD( "685.u26",      0x0400, 0x0400, CRC(85210d8b) SHA1(8260ca809a3a20a52b146d357253aa958d08887e) )
	ROM_LOAD( "686.u25",      0x0800, 0x0400, CRC(2d87238c) SHA1(de8dbe56c4c71b5d75e77c39cfbd771b91c0db0f) )
	ROM_LOAD( "687.u24",      0x0c00, 0x0400, CRC(0dd0604e) SHA1(d357e195a6e61615b0e8cfb027628c2ce5f2b1c5) )
	ROM_LOAD( "688.u23",      0x1000, 0x0400, CRC(2f649667) SHA1(f4c08c7c8f1e9cee84bf844810f006d27bd35025) )
	ROM_LOAD( "689.u22",      0x1400, 0x0400, CRC(89fd63d9) SHA1(413f5df7eedb67848119c675e4edd1ce211ded24) )
	ROM_LOAD( "690.u21",      0x1800, 0x0400, CRC(a86622a6) SHA1(44c0712cc1e11255dc0e27f4754984885c9fa8ad) )
	ROM_LOAD( "691.u20",      0x1c00, 0x0400, CRC(8aca72d8) SHA1(59306dbc350dc64a17ad2f9909ef00e9860c56e7) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "316-507",      0x0000, 0x0020, CRC(fdb22e8f) SHA1(b09241b532cfe7679e837e9f3e5956cfc588a0be) )

	ROM_REGION( 0x0020, REGION_USER1, 0 )	/* misc PROM */
	ROM_LOAD( "316-0206.u14", 0x0000, 0x0020, CRC(9617d796) SHA1(7cff2741866095ff42eadd8022bea349ec8d2f39) )	/* control PROM */
ROM_END

ROM_START( pulsar )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "790.u33",      0x0000, 0x0400, CRC(5e3816da) SHA1(83f019fa3598e383310b4c21441e4f8ef0c9d4fb) )
	ROM_LOAD( "791.u32",      0x0400, 0x0400, CRC(ce0aee83) SHA1(f3755592a9aaa2d493d017c8da19354fd5598860) )
	ROM_LOAD( "792.u31",      0x0800, 0x0400, CRC(72d78cf1) SHA1(d292d80826081073d279d163a107926fa80f02d0) )
	ROM_LOAD( "793.u30",      0x0c00, 0x0400, CRC(42155dd4) SHA1(d3032b0509eb3aa9c49b92ae69971ec019310b16) )
	ROM_LOAD( "794.u29",      0x1000, 0x0400, CRC(11c7213a) SHA1(417a608913e84c456f3181e94e496b05599dfb14) )
	ROM_LOAD( "795.u28",      0x1400, 0x0400, CRC(d2f02e29) SHA1(2b4333ecb89abd80634686b9ef667213423bc4a8) )
	ROM_LOAD( "796.u27",      0x1800, 0x0400, CRC(67737a2e) SHA1(6e8616bb0fba603fec9d97a1852986390db67b0a) )
	ROM_LOAD( "797.u26",      0x1c00, 0x0400, CRC(ec250b24) SHA1(b8bbdc888008d072672024fced55fcf3da668532) )
	ROM_LOAD( "798.u8",       0x2000, 0x0400, CRC(1d34912d) SHA1(32b10bc9617fa14b9e98bd572c86bba495f64d18) )
	ROM_LOAD( "799.u7",       0x2400, 0x0400, CRC(f5695e4c) SHA1(77251f917802d01d76a9a6cf8bb6c12919803388) )
	ROM_LOAD( "800.u6",       0x2800, 0x0400, CRC(bf91ad92) SHA1(ebadcb9a6d2ee5e4b432f132095fac8d72e05a34) )
	ROM_LOAD( "801.u5",       0x2c00, 0x0400, CRC(1e9721dc) SHA1(ddb94275c8fd26f7323b89d234b34883737f8738) )
	ROM_LOAD( "802.u4",       0x3000, 0x0400, CRC(d32d2192) SHA1(334c10dd418fe320fc9a689c798e90787aaae7bf) )
	ROM_LOAD( "803.u3",       0x3400, 0x0400, CRC(3ede44d5) SHA1(3967fc6dc25b5872ae1ad389a31257b39879e13d) )
	ROM_LOAD( "804.u2",       0x3800, 0x0400, CRC(62847b01) SHA1(79763623df6eb7d189a0b0583ecc12c88602a44d) )
	ROM_LOAD( "805.u1",       0x3c00, 0x0400, CRC(ab418e86) SHA1(ad0dfd982b3bd3943aaf670d48485218f1cc4998) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "316-0789.u49", 0x0000, 0x0020, CRC(7fc1861f) SHA1(e005a8afd6b9e6b7d4ddf362c204e472b80582c6) )

	ROM_REGION( 0x0020, REGION_USER1, 0 )	/* misc PROM */
	ROM_LOAD( "316-0206.u14", 0x0000, 0x0020, CRC(9617d796) SHA1(7cff2741866095ff42eadd8022bea349ec8d2f39) )	/* control PROM */
ROM_END

ROM_START( heiankyo )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "ha16.u33",     0x0000, 0x0400, CRC(1eec8b36) SHA1(55644cfeb7a9d64e52f11611c91c6186038772a3) )
	ROM_LOAD( "ha15.u32",     0x0400, 0x0400, CRC(c1b9a1a5) SHA1(068ad2da4852a50c948c4f9b3e1b1aa5c5bf5ca5) )
	ROM_LOAD( "ha14.u31",     0x0800, 0x0400, CRC(5b7b582e) SHA1(078b8b7d1836cc31cee244a58fb6a6a50135f833) )
	ROM_LOAD( "ha13.u30",     0x0c00, 0x0400, CRC(4aa67e01) SHA1(5539a028cb1935bb4d6ab747c92792f5462add1f) )
	ROM_LOAD( "ha12.u29",     0x1000, 0x0400, CRC(75889ca6) SHA1(552bcf976f31d7b634b79175c0470978b6b82433) )
	ROM_LOAD( "ha11.u28",     0x1400, 0x0400, CRC(d469226a) SHA1(dfca01d956e12162fab261a017c727a756b67206) )
	ROM_LOAD( "ha10.u27",     0x1800, 0x0400, CRC(4e203074) SHA1(1a80c396ceb9a2b1c737e1af791dbab2bee10ce5) )
	ROM_LOAD( "ha9.u26",      0x1c00, 0x0400, CRC(9c3a3dd2) SHA1(afcd85ec0174bdcab31135b4e271cec1eb75fd02) )
	ROM_LOAD( "ha8.u8",       0x2000, 0x0400, CRC(6cc64878) SHA1(4d03ff925d80835c27512b3bd04ea57f91b4491f) )
	ROM_LOAD( "ha7.u7",       0x2400, 0x0400, CRC(6d2f9527) SHA1(4e51c5404d0302547c1ae85b27ffe4de11d68224) )
	ROM_LOAD( "ha6.u6",       0x2800, 0x0400, CRC(e467c353) SHA1(a76b4f6d9702f760f287b5285f76ea4206c6934a) )
	ROM_LOAD( "ha3.u3",       0x2c00, 0x0400, CRC(6a55eda8) SHA1(f526ebf18a33271b798e53cfcadb27e4c3a03466) )
	/* 3000-37ff empty */
	ROM_LOAD( "ha2.u2",       0x3800, 0x0400, CRC(056b3b8b) SHA1(3cce6c928599604ffdcdb767caa7b32d8ec1e03d) )
	ROM_LOAD( "ha1.u1",       0x3c00, 0x0400, CRC(b8da2b5e) SHA1(70d97b89cb3162bd479203c53148319179a9873f) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "316-138.u49",  0x0000, 0x0020, CRC(67104ea9) SHA1(26b6bd2a1973b83bb9af4e3385d8cb14cb3f62f2) )

	ROM_REGION( 0x0040, REGION_USER1, 0 )	/* misc PROMs */
	ROM_LOAD( "316-0043.u87", 0x0000, 0x0020, CRC(e60a7960) SHA1(b8b8716e859c57c35310efc4594262afedb84823) )	/* control PROM */
	ROM_LOAD( "316-0042.u88", 0x0020, 0x0020, CRC(a1506b9d) SHA1(037c3db2ea40eca459e8acba9d1506dd28d72d10) )	/* sequence PROM */
ROM_END

ROM_START( alphaho )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "c0.bin",       0x0000, 0x0400, CRC(db774c23) SHA1(c5042872110ae8d0c5c7629892a16b87e8f19d96) )
	ROM_LOAD( "c1.bin",       0x0400, 0x0400, CRC(b63f4695) SHA1(95b3ca96ca48f2c525eaf2b49956248e46686688) )
	ROM_LOAD( "c2.bin",       0x0800, 0x0400, CRC(4ebf0ba4) SHA1(b6651f7424fd5e62422b45ccce118db64dab50bf) )
	ROM_LOAD( "c3.bin",       0x0c00, 0x0400, CRC(126f17ec) SHA1(a78b9f62d76305903ad56ebf995ba0745774e6f2) )
	ROM_LOAD( "c4.bin",       0x1000, 0x0400, CRC(52798c61) SHA1(c01c11b99f56ca7f5d8824a3b9f795b2075d21a0) )
	ROM_LOAD( "c5.bin",       0x1400, 0x0400, CRC(4827cb36) SHA1(a5f56aa73147503fb6c5a3f38489754d8d27f257) )
	ROM_LOAD( "c6.bin",       0x1800, 0x0400, CRC(8b2ff47e) SHA1(43c84fe40180c42ade3c77790aeadeeb28e5c5cd) )
	ROM_LOAD( "c7.bin",       0x1c00, 0x0400, CRC(44921df4) SHA1(3ece06f20330aebe2770bf74142cd5e7bc5297ec) )
	ROM_LOAD( "c8.bin",       0x2000, 0x0400, CRC(9fb12fca) SHA1(a5b49ddd86be44b9220cc4ceb84b32c55e5d432b) )
	ROM_LOAD( "c9.bin",       0x2400, 0x0400, CRC(e5f622f7) SHA1(57858b6abbf34fc4ab2b19a469cbd945a0e14a0e) )
	ROM_LOAD( "ca.bin",       0x2800, 0x0400, CRC(82b28e77) SHA1(c4f425daa02acbc19d4ecee8bd38d8bb338e085f) )
	ROM_LOAD( "cb.bin",       0x2c00, 0x0400, CRC(94fba0ad) SHA1(c623368c710ddf8ff05e09c963eeb863c9951df3) )
	ROM_LOAD( "cc.bin",       0x3000, 0x0400, CRC(de338b6d) SHA1(a465aaac041e4aa017afb9821dd049ac4950ba92) )
	ROM_LOAD( "cd.bin",       0x3400, 0x0400, CRC(be76baac) SHA1(4b6a46c9484cfc90fa405f8568df44cfc96b1d7a) )
	ROM_LOAD( "ce.bin",       0x3800, 0x0400, CRC(3c409d57) SHA1(fda00b4eaaa187f489a8957952005e08b2b7ba40) )
	ROM_LOAD( "cf.bin",       0x3c00, 0x0400, CRC(d03c5a09) SHA1(eebc1d2302bd2bae28c25187bb099ae618c5cd05) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "alphaho.col",  0x0000, 0x0020, NO_DUMP )
ROM_END



static void vicdual_decode(void)
{
	unsigned char *RAM = memory_region(REGION_CPU1);


	/* copy the ROMs to the mirror image */
	memcpy(&RAM[0x4000],&RAM[0x0000],0x4000);
}



static DRIVER_INIT( nosamples )
{
	vicdual_decode();
}

static DRIVER_INIT( depthch )
{
	install_port_read_handler(0, 0x08, 0x08, depthch_input_port_1_r);

	/* install sample trigger */
	install_port_write_handler(0, 0x04, 0x04, depthch_sh_port1_w);

	vicdual_decode();

	artwork_set_overlay(depthch_overlay);

}

static DRIVER_INIT( samurai )
{
	/* install protection handlers */
	install_mem_write_handler(0, 0x7f00, 0x7f00, samurai_protection_w);
	install_port_read_handler(0, 0x01, 0x03, samurai_input_r);

	vicdual_decode();
}

static DRIVER_INIT( carnival )
{
	/* install sample triggers */
	install_port_write_handler(0, 0x01, 0x01, carnival_sh_port1_w);
	install_port_write_handler(0, 0x02, 0x02, carnival_sh_port2_w);

	vicdual_decode();
}

static DRIVER_INIT( invinco )
{
	/* install sample trigger */
	install_port_write_handler(0, 0x02, 0x02, invinco_sh_port2_w);

	vicdual_decode();
}

static DRIVER_INIT( invho2 )
{
	/* install sample trigger */
	install_port_write_handler(0, 0x02, 0x02, invinco_sh_port2_w);

	vicdual_decode();
}

static DRIVER_INIT( invds )
{
	/* install sample trigger */
	install_port_write_handler(0, 0x01, 0x01, invinco_sh_port2_w);

	vicdual_decode();
}

static DRIVER_INIT( pulsar )
{
	/* install sample triggers */
	install_port_write_handler(0, 0x01, 0x01, pulsar_sh_port1_w);
	install_port_write_handler(0, 0x02, 0x02, pulsar_sh_port2_w);

	vicdual_decode();
}

#if 0
static DRIVER_INIT( alphaho )
{
	/* install sample trigger */
	install_port_write_handler(0, 0x01, 0x01, invinco_sh_port2_w);
	install_port_write_handler(0, 0x02, 0x02, invinco_sh_port2_w);

	vicdual_decode();
}
#endif

static DRIVER_INIT( frogs )
{
	install_port_write_handler(0, 0x02, 0x02, frogs_sh_port2_w);
	vicdual_decode();
}


GAME( 1977, depthch,  0,        depthch,  depthch,  depthch,   ROT0,   "Gremlin", "Depthcharge" )
GAME( 1977, depthv1,  depthch,  depthch,  depthch,  depthch,   ROT0,   "Gremlin", "Depthcharge (older)" )
GAME( 1977, subhunt,  depthch,  depthch,  depthch,  depthch,   ROT0,   "Taito", "Sub Hunter" )
GAMEX(1977, safari,   0,        safari,   safari,   nosamples, ROT0,   "Gremlin", "Safari", GAME_NO_SOUND )
GAMEX(1978, frogs,    0,        frogs,    frogs,    frogs,     ROT0,   "Gremlin", "Frogs", GAME_IMPERFECT_SOUND )
GAMEX(1979, sspaceat, 0,        3ports,   sspaceat, nosamples, ROT270, "Sega", "Space Attack (upright)", GAME_NO_SOUND )
GAMEX(1979, sspacat2, sspaceat, 3ports,   sspaceat, nosamples, ROT270, "Sega", "Space Attack (upright, older)", GAME_NO_SOUND )
GAMEX(1979, sspacatc, sspaceat, 3ports,   sspaceat, nosamples, ROT270, "Sega", "Space Attack (cocktail)", GAME_NO_SOUND )
GAMEX(1979, sspacaho, 0,        4ports,   sspacaho, nosamples, ROT270, "Sega", "Space Attack - Head On", GAME_NO_SOUND )
GAMEX(1979, headon,   0,        2ports,   headon,   nosamples, ROT0,   "Gremlin", "Head On (2 players)", GAME_NO_SOUND )
GAMEX(1979, headonb,  headon,   2ports,   headon,   nosamples, ROT0,   "Gremlin", "Head On (1 player)", GAME_NO_SOUND )
GAMEX(1979, headon2,  0,        3ports,   headon2,  nosamples, ROT0,   "Sega", "Head On 2", GAME_NO_SOUND )
GAMEX(1979, invho2,   0,        invinco4, invho2,   invho2,    ROT270, "Sega", "Invinco - Head On 2", GAME_IMPERFECT_SOUND )
GAMEX(1980, samurai,  0,        4ports,   samurai,  samurai,   ROT270, "Sega", "Samurai", GAME_NO_SOUND )
GAME( 1979, invinco,  0,        invinco3, invinco,  invinco,   ROT270, "Sega", "Invinco" )
GAMEX(1979, invds,    0,        invinco4, invds,    invds,     ROT270, "Sega", "Invinco - Deep Scan", GAME_IMPERFECT_SOUND )
GAMEX(1980, tranqgun, 0,        4ports,   tranqgun, nosamples, ROT270, "Sega", "Tranquilizer Gun", GAME_NO_SOUND )
GAMEX(1980, spacetrk, 0,        4ports,   spacetrk, nosamples, ROT270, "Sega", "Space Trek (upright)", GAME_NO_SOUND )
GAMEX(1980, sptrekct, spacetrk, 4ports,   sptrekct, nosamples, ROT270, "Sega", "Space Trek (cocktail)", GAME_NO_SOUND )
GAME( 1980, carnival, 0,        carnival, carnival, carnival,  ROT270, "Sega", "Carnival (upright)" )
GAME( 1980, carnvckt, carnival, carnival, carnvckt, carnival,  ROT270, "Sega", "Carnival (cocktail)" )
GAMEX(1981, brdrlinb, 0,        4ports,   brdrline, carnival,  ROT270, "bootleg", "Borderline (bootleg)", GAME_NO_SOUND )
GAMEX(1980, digger,   0,        3ports,   digger,   nosamples, ROT270, "Sega", "Digger", GAME_NO_SOUND )
GAME( 1981, pulsar,   0,        pulsar,   pulsar,   pulsar,    ROT270, "Sega", "Pulsar" )
GAMEX(1979, heiankyo, 0,        4ports,   heiankyo, nosamples, ROT270, "Denki Onkyo", "Heiankyo Alien", GAME_NO_SOUND )
GAMEX(19??, alphaho,  0,        invinco4, alphaho,  invho2,    ROT270, "Data East Corporation", "Alpha Fighter - Head On", GAME_WRONG_COLORS )
