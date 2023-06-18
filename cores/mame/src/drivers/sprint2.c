/***************************************************************************

	Atari Sprint 2 hardware

	driver by Mike Balfour

	Games supported:
		* Sprint 1
		* Sprint 2
		* Dominos

	All three games run on the same PCB but with minor modifications (some
	chips removed, some wire-wrap connections added).

	If you have any questions about how this driver works, don't hesitate to
	ask.  - Mike Balfour (mab22@po.cwru.edu)

***************************************************************************/

#include "driver.h"

extern READ_HANDLER( sprint2_collision1_r );
extern READ_HANDLER( sprint2_collision2_r );

extern WRITE_HANDLER( sprint2_collision_reset1_w );
extern WRITE_HANDLER( sprint2_collision_reset2_w );
extern WRITE_HANDLER( sprint2_video_ram_w );

extern VIDEO_UPDATE( sprint2 );
extern VIDEO_START( sprint2 );
extern VIDEO_EOF( sprint2 );

extern UINT8* sprint2_video_ram;


static int attract;
static int steering[2];
static int gear[2];
static int game;


static DRIVER_INIT( sprint1 )
{
	game = 1;
}
static DRIVER_INIT( sprint2 )
{
	game = 2;
}
static DRIVER_INIT( dominos )
{
	game = 3;
}


static INTERRUPT_GEN( sprint2 )
{
	static UINT8 dial[2];

	/* handle steering wheels */

	if (game == 1 || game == 2)
	{
		int i;

		for (i = 0; i < 2; i++)
		{
			signed char delta = readinputport(6 + i) - dial[i];

			if (delta < 0)
			{
				steering[i] = 0x00;
			}
			if (delta > 0)
			{
				steering[i] = 0x40;
			}

			dial[i] += delta;

			switch (readinputport(4 + i) & 15)
			{
			case 1: gear[i] = 1; break;
			case 2: gear[i] = 2; break;
			case 4: gear[i] = 3; break;
			case 8: gear[i] = 4; break;
			}
		}
	}

	discrete_sound_w(2, sprint2_video_ram[0x394] & 15);
	discrete_sound_w(3, sprint2_video_ram[0x395] & 15);
	discrete_sound_w(4, sprint2_video_ram[0x396] & 15);

	cpu_set_nmi_line(0, PULSE_LINE);
}


static PALETTE_INIT( sprint2 )
{
	palette_set_color(0, 0x00, 0x00, 0x00);
	palette_set_color(1, 0x5b, 0x5b, 0x5b);
	palette_set_color(2, 0xa4, 0xa4, 0xa4);
	palette_set_color(3, 0xff, 0xff, 0xff);

	colortable[0x0] = 1;	/* black playfield */
	colortable[0x1] = 0;
	colortable[0x2] = 1;	/* white playfield */
	colortable[0x3] = 3;

	colortable[0x4] = 1;	/* car #1 */
	colortable[0x5] = 3;
	colortable[0x6] = 1;	/* car #2 */
	colortable[0x7] = 0;
	colortable[0x8] = 1;	/* car #3 */
	colortable[0x9] = 2;
	colortable[0xa] = 1;	/* car #4 */
	colortable[0xb] = 2;
}


static READ_HANDLER( sprint2_wram_r )
{
	return sprint2_video_ram[0x380 + offset % 0x80];
}


static READ_HANDLER( sprint2_dip_r )
{
	return (readinputport(0) << (2 * ((offset & 3) ^ 3))) & 0xc0;
}


static READ_HANDLER( sprint2_input_A_r )
{
	UINT8 val = readinputport(1);

	if (game == 2)
	{
		if (gear[0] == 1) val &= ~0x01;
		if (gear[1] == 1) val &= ~0x02;
		if (gear[0] == 2) val &= ~0x04;
		if (gear[1] == 2) val &= ~0x08;
		if (gear[0] == 3) val &= ~0x10;
		if (gear[1] == 3) val &= ~0x20;
	}

	return (val << (offset ^ 7)) & 0x80;
}


static READ_HANDLER( sprint2_input_B_r )
{
	UINT8 val = readinputport(2);

	if (game == 1)
	{
		if (gear[0] == 1) val &= ~0x01;
		if (gear[0] == 2) val &= ~0x02;
		if (gear[0] == 3) val &= ~0x04;
	}

	return (val << (offset ^ 7)) & 0x80;
}


static READ_HANDLER( sprint2_sync_r )
{
	UINT8 val = 0;

	if (attract != 0)
	{
		val |= 0x10;
	}
	if (cpu_getscanline() == 261)
	{
		val |= 0x20; /* VRESET */
	}
	if (cpu_getscanline() >= 224)
	{
		val |= 0x40; /* VBLANK */
	}
	if (cpu_getscanline() >= 131)
	{
		val |= 0x80; /* 60 Hz? */
	}

	return val;
}


static READ_HANDLER( sprint2_steering1_r )
{
	return steering[0];
}
static READ_HANDLER( sprint2_steering2_r )
{
	return steering[1];
}


static WRITE_HANDLER( sprint2_steering_reset1_w )
{
	steering[0] |= 0x80;
}
static WRITE_HANDLER( sprint2_steering_reset2_w )
{
	steering[1] |= 0x80;
}


static WRITE_HANDLER( sprint2_wram_w )
{
	sprint2_video_ram[0x380 + offset % 0x80] = data;
}


static WRITE_HANDLER( sprint2_attract_w )
{
	attract = offset & 1;

	discrete_sound_w(5, !attract);
}


static WRITE_HANDLER( sprint2_noise_reset_w )
{
	discrete_sound_w(6, 0);
}


static WRITE_HANDLER( sprint2_skid1_w )
{
	discrete_sound_w(0, offset & 1);
}
static WRITE_HANDLER( sprint2_skid2_w )
{
	discrete_sound_w(1, offset & 1);
}


static WRITE_HANDLER( sprint2_lamp1_w )
{
	set_led_status(0, offset & 1);
}
static WRITE_HANDLER( sprint2_lamp2_w )
{
	set_led_status(1, offset & 1);
}


static MEMORY_READ_START( readmem )
	{ 0x0000, 0x03ff, sprint2_wram_r },
	{ 0x0400, 0x07ff, MRA_RAM },
	{ 0x0818, 0x081f, sprint2_input_A_r },
	{ 0x0828, 0x082f, sprint2_input_B_r },
	{ 0x0830, 0x0837, sprint2_dip_r },
	{ 0x0840, 0x087f, input_port_3_r },
	{ 0x0880, 0x08bf, sprint2_steering1_r },
	{ 0x08c0, 0x08ff, sprint2_steering2_r },
	{ 0x0c00, 0x0fff, sprint2_sync_r },
	{ 0x1000, 0x13ff, sprint2_collision1_r },
	{ 0x1400, 0x17ff, sprint2_collision2_r },
	{ 0x1800, 0x1800, MRA_NOP },  /* debugger ROM location? */
	{ 0x2000, 0x3fff, MRA_ROM },
	{ 0xe000, 0xffff, MRA_ROM },
MEMORY_END


static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x03ff, sprint2_wram_w },
	{ 0x0400, 0x07ff, sprint2_video_ram_w, &sprint2_video_ram },
	{ 0x0c00, 0x0c0f, sprint2_attract_w },
	{ 0x0c10, 0x0c1f, sprint2_skid1_w },
	{ 0x0c20, 0x0c2f, sprint2_skid2_w },
	{ 0x0c30, 0x0c3f, sprint2_lamp1_w },
	{ 0x0c40, 0x0c4f, sprint2_lamp2_w },
	{ 0x0c60, 0x0c6f, MWA_NOP }, /* SPARE */
	{ 0x0c80, 0x0cff, MWA_NOP }, /* watchdog, disabled during service mode */
	{ 0x0d00, 0x0d7f, sprint2_collision_reset1_w },
	{ 0x0d80, 0x0dff, sprint2_collision_reset2_w },
	{ 0x0e00, 0x0e7f, sprint2_steering_reset1_w },
	{ 0x0e80, 0x0eff, sprint2_steering_reset2_w },
	{ 0x0f00, 0x0f7f, sprint2_noise_reset_w },
	{ 0x2000, 0x3fff, MWA_ROM },
	{ 0xe000, 0xffff, MWA_ROM },
MEMORY_END


INPUT_PORTS_START( sprint2 )
	PORT_START
	PORT_DIPNAME( 0x01, 0x00, "Tracks on Demo" )
	PORT_DIPSETTING(    0x00, "Easy Track Only" )
	PORT_DIPSETTING(    0x01, "Cycle 12 Tracks" )
	PORT_DIPNAME( 0x02, 0x00, "Oil Slicks" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unused) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "Extended Play" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0xc0, 0x00, "Play Time" )
	PORT_DIPSETTING(    0xc0, "60 seconds" )
	PORT_DIPSETTING(    0x80, "90 seconds" )
	PORT_DIPSETTING(    0x40, "120 seconds" )
	PORT_DIPSETTING(    0x00, "150 seconds" )

	PORT_START /* input A */
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_UNUSED ) /* P1 1st gear */
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_UNUSED ) /* P2 1st gear */
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_UNUSED ) /* P1 2nd gear */
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_UNUSED ) /* P2 2nd gear */
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_UNUSED ) /* P1 3rd gear */
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_UNUSED ) /* P2 3rd gear */

	PORT_START /* input B */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1, "Player 1 Gas", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2, "Player 2 Gas", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_BUTTON6, "Track Select", KEYCODE_SPACE, IP_JOY_DEFAULT )

	PORT_START
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER1, "Player 1 Gear 1", KEYCODE_Z, IP_JOY_DEFAULT )
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER1, "Player 1 Gear 2", KEYCODE_X, IP_JOY_DEFAULT )
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_BUTTON4 | IPF_PLAYER1, "Player 1 Gear 3", KEYCODE_C, IP_JOY_DEFAULT )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_BUTTON5 | IPF_PLAYER1, "Player 1 Gear 4", KEYCODE_V, IP_JOY_DEFAULT )

	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2, "Player 2 Gear 1", KEYCODE_Q, IP_JOY_DEFAULT )
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER2, "Player 2 Gear 2", KEYCODE_W, IP_JOY_DEFAULT )
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_BUTTON4 | IPF_PLAYER2, "Player 2 Gear 3", KEYCODE_E, IP_JOY_DEFAULT )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_BUTTON5 | IPF_PLAYER2, "Player 2 Gear 4", KEYCODE_R, IP_JOY_DEFAULT )

	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_PLAYER1, 100, 10, 0, 0 )

	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_PLAYER2, 100, 10, 0, 0 )
	PORT_START
	PORT_ADJUSTER( 20, "Motor 1 RPM" )

	PORT_START
	PORT_ADJUSTER( 30, "Motor 2 RPM" )
INPUT_PORTS_END


INPUT_PORTS_START( sprint1 )
	PORT_START
	PORT_DIPNAME( 0x01, 0x00, "Change Track" )
	PORT_DIPSETTING(    0x01, "Every Lap" )
	PORT_DIPSETTING(    0x00, "Every 2 Laps" )
	PORT_DIPNAME( 0x02, 0x00, "Oil Slicks" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unused) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "Extended Play" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0xc0, 0x00, "Play Time" )
	PORT_DIPSETTING(    0xc0, "60 seconds" )
	PORT_DIPSETTING(    0x80, "90 seconds" )
	PORT_DIPSETTING(    0x40, "120 seconds" )
	PORT_DIPSETTING(    0x00, "150 seconds" )

	PORT_START /* input A */

	PORT_START /* input B */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_UNUSED ) /* 1st gear */
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_UNUSED ) /* 2nd gear */
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_UNUSED ) /* 3rd gear */
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1, "Gas", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START
	PORT_BITX( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2, "Gear 1", KEYCODE_Z, IP_JOY_DEFAULT )
	PORT_BITX( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON3, "Gear 2", KEYCODE_X, IP_JOY_DEFAULT )
	PORT_BITX( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON4, "Gear 3", KEYCODE_C, IP_JOY_DEFAULT )
	PORT_BITX( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON5, "Gear 4", KEYCODE_V, IP_JOY_DEFAULT )

	PORT_START

	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL, 100, 10, 0, 0 )

	PORT_START

	PORT_START
	PORT_ADJUSTER( 20, "Motor RPM" )
INPUT_PORTS_END


INPUT_PORTS_START( dominos )
	PORT_START
	PORT_DIPNAME( 0x03, 0x01, "Points to Win" )
	PORT_DIPSETTING(	0x03, "6" )
	PORT_DIPSETTING(	0x02, "5" )
	PORT_DIPSETTING(	0x01, "4" )
	PORT_DIPSETTING(	0x00, "3" )
	PORT_DIPNAME( 0x0C, 0x08, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x08, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x0c, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	
	PORT_START /* input A */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_PLAYER2 )

	PORT_START /* input B */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_SERVICE( 0x40, IP_ACTIVE_LOW )

	PORT_START
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START
	PORT_ADJUSTER( 20, "Tone Freq" )
INPUT_PORTS_END


static struct GfxLayout tile_layout =
{
	16, 8,
	64,
	1,
	{ 0 },
	{
		0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7
	},
	{
		0x00, 0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38
	},
	0x40
};


static struct GfxLayout car_layout =
{
	16, 8,
	32,
	1,
	{ 0 },
	{
		0x7, 0x6, 0x5, 0x4, 0x3, 0x2, 0x1, 0x0,
		0xf, 0xe, 0xd, 0xc, 0xb, 0xa, 0x9, 0x8
	},
	{
		0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70
	},
	0x80
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tile_layout, 0, 2 },
	{ REGION_GFX2, 0, &car_layout, 4, 4 },
	{ -1 }
};


/************************************************************************/
/* sprint2 Sound System Analog emulation                                */
/************************************************************************/

const struct discrete_lfsr_desc sprint2_lfsr={
	16,                  /* Bit Length */
	0,                   /* Reset Value */
	0,                   /* Use Bit 0 as XOR input 0 */
	14,                  /* Use Bit 14 as XOR input 1 */
	DISC_LFSR_XNOR,      /* Feedback stage1 is XNOR */
	DISC_LFSR_OR,        /* Feedback stage2 is just stage 1 output OR with external feed */
	DISC_LFSR_REPLACE,   /* Feedback stage3 replaces the shifted register contents */
	0x000001,            /* Everything is shifted into the first bit only */
	0,                   /* Output is not inverted */
	15                   /* Output bit */
};

/* Nodes - Inputs */
#define SPRINT2_MOTORSND1_DATA     NODE_01
#define SPRINT2_MOTORSND2_DATA     NODE_02
#define SPRINT2_SKIDSND1_EN        NODE_03
#define SPRINT2_SKIDSND2_EN        NODE_04
#define SPRINT2_CRASHSND_DATA      NODE_05
#define SPRINT2_ATTRACT_EN         NODE_06
#define SPRINT2_NOISE_RESET        NODE_07
/* Nodes - Sounds */
#define SPRINT2_MOTORSND1          NODE_10
#define SPRINT2_MOTORSND2          NODE_11
#define SPRINT2_CRASHSND           NODE_12
#define SPRINT2_SKIDSND1           NODE_13
#define SPRINT2_SKIDSND2           NODE_14
#define SPRINT2_NOISE              NODE_15
#define SPRINT2_FINAL_MIX1         NODE_16
#define SPRINT2_FINAL_MIX2         NODE_17

static DISCRETE_SOUND_START(sprint2_sound_interface)
	/************************************************/
	/* Sprint2 sound system: 5 Sound Sources        */
	/*                     Relative Volume          */
	/*    1/2) Motor           84.69%               */
	/*      2) Crash          100.00%               */
	/*    4/5) Screech/Skid    40.78%               */
	/* Relative volumes calculated from resitor     */
	/* network in combiner circuit                  */
	/*                                              */
	/*  Discrete sound mapping via:                 */
	/*     discrete_sound_w($register,value)        */
	/*  $00 - Skid enable Car1                      */
	/*  $01 - Skid enable Car2                      */
	/*  $02 - Motorsound frequency Car1             */
	/*  $03 - Motorsound frequency Car2             */
	/*  $04 - Crash volume                          */
	/*  $05 - Attract                               */
	/*  $06 - Noise Reset                           */
	/*                                              */
	/************************************************/

	/************************************************/
	/* Input register mapping for sprint2           */
	/************************************************/
	/*                    NODE                       ADDR  MASK    GAIN      OFFSET  INIT */
	DISCRETE_INPUT      (SPRINT2_SKIDSND1_EN       , 0x00, 0x000f,                   0.0)
	DISCRETE_INPUT      (SPRINT2_SKIDSND2_EN       , 0x01, 0x000f,                   0.0)
	DISCRETE_INPUT      (SPRINT2_MOTORSND1_DATA    , 0x02, 0x000f,                   0.0)
	DISCRETE_INPUT      (SPRINT2_MOTORSND2_DATA    , 0x03, 0x000f,                   0.0)
	DISCRETE_INPUTX     (SPRINT2_CRASHSND_DATA     , 0x04, 0x000f, 1000.0/15.0, 0,   0.0)
	DISCRETE_INPUT      (SPRINT2_ATTRACT_EN        , 0x05, 0x000f,                   0.0)
	DISCRETE_INPUT_PULSE(SPRINT2_NOISE_RESET       , 0x06, 0x000f,                   1.0)

	/************************************************/
	/* Motor sound circuit is based on a 556 VCO    */
	/* with the input frequency set by the MotorSND */
	/* latch (4 bit). This freqency is then used to */
	/* driver a modulo 12 counter, with div6, 4 & 3 */
	/* summed as the output of the circuit.         */
	/* VCO Output is Sq wave = 27-382Hz             */
	/*  F1 freq - (Div6)                            */
	/*  F2 freq = (Div4)                            */
	/*  F3 freq = (Div3) 33.3% duty, 33.3 deg phase */
	/* To generate the frequency we take the freq.  */
	/* diff. and /15 to get all the steps between   */
	/* 0 - 15.  Then add the low frequency and send */
	/* that value to a squarewave generator.        */
	/* Also as the frequency changes, it ramps due  */
	/* to a 10uf capacitor on the R-ladder.         */
	/* Note the VCO freq. is controlled by a 250k   */
	/* pot.  The freq. used here is for the pot set */
	/* to 125k.  The low freq is allways the same.  */
	/* This adjusts the high end.                   */
	/* 0k = 214Hz.   250k = 4416Hz                  */
	/************************************************/
	DISCRETE_RCFILTER(NODE_20, 1, SPRINT2_MOTORSND1_DATA, 123000, 10e-6)
	DISCRETE_ADJUSTMENT(NODE_21, 1, (214.0-27.0)/12/15, (4416.0-27.0)/12/15, DISC_LOGADJ, 8)
	DISCRETE_MULTIPLY(NODE_22, 1, NODE_20, NODE_21)

	DISCRETE_MULTADD(NODE_23, 1, NODE_22, 2, 27.0/6)	/* F1 = /12*2 = /6 */
	DISCRETE_SQUAREWAVE(NODE_24, 1, NODE_23, (846.9/3), 50.0, 0, 0)
	DISCRETE_RCFILTER(NODE_25, 1, NODE_24, 10000, 1e-7)

	DISCRETE_MULTADD(NODE_26, 1, NODE_22, 3, 27.0/4)	/* F2 = /12*3 = /4 */
	DISCRETE_SQUAREWAVE(NODE_27, 1, NODE_26, (846.9/3), 50.0, 0, 0)
	DISCRETE_RCFILTER(NODE_28, 1, NODE_27, 10000, 1e-7)

	DISCRETE_MULTADD(NODE_29, 1, NODE_22, 4, 27.0/3)	/* F3 = /12*4 = /3 */
	DISCRETE_SQUAREWAVE(NODE_30, 1, NODE_29, (846.9/3), 100.0/3, 0, 360.0/3)
	DISCRETE_RCFILTER(NODE_31, 1, NODE_30, 10000, 1e-7)

	DISCRETE_ADDER3(SPRINT2_MOTORSND1, SPRINT2_ATTRACT_EN, NODE_25, NODE_28, NODE_31)

	/************************************************/
	/* Car2 motor sound is basically the same as    */
	/* Car1.  But I shifted the frequencies up for  */
	/* it to sound different from car1.             */
	/************************************************/
	DISCRETE_RCFILTER(NODE_40, 1, SPRINT2_MOTORSND2_DATA, 123000, 10e-6)
	DISCRETE_ADJUSTMENT(NODE_41, 1, (214.0-27.0)/12/15, (4416.0-27.0)/12/15, DISC_LOGADJ, 9)
	DISCRETE_MULTIPLY(NODE_42, 1, NODE_40, NODE_41)

	DISCRETE_MULTADD(NODE_43, 1, NODE_42, 2, 27.0/6)	/* F1 = /12*2 = /6 */
	DISCRETE_SQUAREWAVE(NODE_44, 1, NODE_43, (846.9/3), 50.0, 0, 0)
	DISCRETE_RCFILTER(NODE_45, 1, NODE_44, 10000, 1e-7)

	DISCRETE_MULTADD(NODE_46, 1, NODE_42, 3, 27.0/4)	/* F2 = /12*3 = /4 */
	DISCRETE_SQUAREWAVE(NODE_47, 1, NODE_46, (846.9/3), 50.0, 0, 0)
	DISCRETE_RCFILTER(NODE_48, 1, NODE_47, 10000, 1e-7)

	DISCRETE_MULTADD(NODE_49, 1, NODE_42, 4, 27.0/3)	/* F3 = /12*4 = /3 */
	DISCRETE_SQUAREWAVE(NODE_50, 1, NODE_49, (846.9/3), 100.0/3, 0, 360.0/3)
	DISCRETE_RCFILTER(NODE_51, 1, NODE_50, 10000, 1e-7)

	DISCRETE_ADDER3(SPRINT2_MOTORSND2, SPRINT2_ATTRACT_EN, NODE_45, NODE_48, NODE_51)

	/************************************************/
	/* Crash circuit is built around a noise        */
	/* generator built from 2 shift registers that  */
	/* are clocked by the 2V signal.                */
	/* 2V = HSYNC/4                                 */
	/*    = 15750/4                                 */
	/* Output is binary weighted with 4 bits of     */
	/* crash volume.                                */
	/************************************************/
	DISCRETE_LFSR_NOISE(SPRINT2_NOISE, 1, SPRINT2_NOISE_RESET, 15750.0/4, 1.0, 0, 0, &sprint2_lfsr)

	DISCRETE_MULTIPLY(NODE_60, 1, SPRINT2_NOISE, SPRINT2_CRASHSND_DATA)
	DISCRETE_RCFILTER(SPRINT2_CRASHSND, 1, NODE_60, 545, 1e-7)

	/************************************************/
	/* Skid circuits takes the noise output from    */
	/* the crash circuit and applies +ve feedback   */
	/* to cause oscillation. There is also an RC    */
	/* filter on the input to the feedback cct.     */
	/* RC is 1K & 10uF                              */
	/* Feedback cct is modelled by using the RC out */
	/* as the frequency input on a VCO,             */
	/* breadboarded freq range as:                  */
	/*  0 = 940Hz, 34% duty                         */
	/*  1 = 630Hz, 29% duty                         */
	/*  the duty variance is so small we ignore it  */
	/************************************************/
	DISCRETE_INVERT(NODE_70, SPRINT2_NOISE)
	DISCRETE_MULTADD(NODE_71, 1, NODE_70, 940.0-630.0, ((940.0-630.0)/2)+630.0)
	DISCRETE_RCFILTER(NODE_72, 1, NODE_71, 1000, 1e-5)
	DISCRETE_SQUAREWAVE(NODE_73, 1, NODE_72, 407.8, 31.5, 0, 0.0)
	DISCRETE_ONOFF(SPRINT2_SKIDSND1, SPRINT2_SKIDSND1_EN, NODE_73)
	DISCRETE_ONOFF(SPRINT2_SKIDSND2, SPRINT2_SKIDSND2_EN, NODE_73)

	/************************************************/
	/* Combine all 5 sound sources.                 */
	/* Add some final gain to get to a good sound   */
	/* level.                                       */
	/************************************************/
	DISCRETE_ADDER3(NODE_90, 1, SPRINT2_MOTORSND1, SPRINT2_CRASHSND, SPRINT2_SKIDSND1)
	DISCRETE_ADDER3(NODE_91, 1, SPRINT2_MOTORSND2, SPRINT2_CRASHSND, SPRINT2_SKIDSND2)
	DISCRETE_GAIN(SPRINT2_FINAL_MIX1, NODE_90, 65534.0/(846.9+1000.0+407.8))
	DISCRETE_GAIN(SPRINT2_FINAL_MIX2, NODE_91, 65534.0/(846.9+1000.0+407.8))

	DISCRETE_OUTPUT(SPRINT2_FINAL_MIX2, MIXER(100,MIXER_PAN_LEFT))
	DISCRETE_OUTPUT(SPRINT2_FINAL_MIX1, MIXER(100,MIXER_PAN_RIGHT))
DISCRETE_SOUND_END

static DISCRETE_SOUND_START(sprint1_sound_interface)
	/************************************************/
	/* Sprint1 sound system: 3 Sound Sources        */
	/*                     Relative Volume          */
	/*      1) Motor           84.69%               */
	/*      2) Crash          100.00%               */
	/*      3) Screech/Skid    40.78%               */
	/* Relative volumes calculated from resitor     */
	/* network in combiner circuit                  */
	/*                                              */
	/*  Discrete sound mapping via:                 */
	/*     discrete_sound_w($register,value)        */
	/*  $00 - Skid enable                           */
	/*  $02 - Motorsound frequency                  */
	/*  $04 - Crash volume                          */
	/*  $05 - Attract                               */
	/*  $06 - Noise Reset                           */
	/*                                              */
	/************************************************/

	/************************************************/
	/* Input register mapping for sprint1           */
	/************************************************/
	/*                    NODE                       ADDR  MASK    GAIN      OFFSET  INIT */
	DISCRETE_INPUT      (SPRINT2_SKIDSND1_EN       , 0x00, 0x000f,                 0.0)
	DISCRETE_INPUT      (SPRINT2_MOTORSND1_DATA    , 0x02, 0x000f,                 0.0)
	DISCRETE_INPUTX     (SPRINT2_CRASHSND_DATA     , 0x04, 0x000f, 1000.0/15.0, 0, 0.0)
	DISCRETE_INPUT      (SPRINT2_ATTRACT_EN        , 0x05, 0x000f,                 0.0)
	DISCRETE_INPUT_PULSE(SPRINT2_NOISE_RESET       , 0x06, 0x000f,                 1.0)

	/************************************************/
	/* Motor sound circuit is based on a 556 VCO    */
	/* with the input frequency set by the MotorSND */
	/* latch (4 bit). This freqency is then used to */
	/* driver a modulo 12 counter, with div6, 4 & 3 */
	/* summed as the output of the circuit.         */
	/* VCO Output is Sq wave = 27-382Hz             */
	/*  F1 freq - (Div6)                            */
	/*  F2 freq = (Div4)                            */
	/*  F3 freq = (Div3) 33.3% duty, 33.3 deg phase */
	/* To generate the frequency we take the freq.  */
	/* diff. and /15 to get all the steps between   */
	/* 0 - 15.  Then add the low frequency and send */
	/* that value to a squarewave generator.        */
	/* Also as the frequency changes, it ramps due  */
	/* to a 10uf capacitor on the R-ladder.         */
	/* Note the VCO freq. is controlled by a 250k   */
	/* pot.  The freq. used here is for the pot set */
	/* to 125k.  The low freq is allways the same.  */
	/* This adjusts the high end.                   */
	/* 0k = 214Hz.   250k = 4416Hz                  */
	/************************************************/
	DISCRETE_RCFILTER(NODE_20, 1, SPRINT2_MOTORSND1_DATA, 123000, 10e-6)
	DISCRETE_ADJUSTMENT(NODE_21, 1, (214.0-27.0)/12/15, (4416.0-27.0)/12/15, DISC_LOGADJ, 8)
	DISCRETE_MULTIPLY(NODE_22, 1, NODE_20, NODE_21)

	DISCRETE_MULTADD(NODE_23, 1, NODE_22, 2, 27.0/6)	/* F1 = /12*2 = /6 */
	DISCRETE_SQUAREWAVE(NODE_24, 1, NODE_23, (846.9/3), 50.0, 0, 0)
	DISCRETE_RCFILTER(NODE_25, 1, NODE_24, 10000, 1e-7)

	DISCRETE_MULTADD(NODE_26, 1, NODE_22, 3, 27.0/4)	/* F2 = /12*3 = /4 */
	DISCRETE_SQUAREWAVE(NODE_27, 1, NODE_26, (846.9/3), 50.0, 0, 0)
	DISCRETE_RCFILTER(NODE_28, 1, NODE_27, 10000, 1e-7)

	DISCRETE_MULTADD(NODE_29, 1, NODE_22, 4, 27.0/3)	/* F3 = /12*4 = /3 */
	DISCRETE_SQUAREWAVE(NODE_30, 1, NODE_29, (846.9/3), 100.0/3, 0, 360.0/3)
	DISCRETE_RCFILTER(NODE_31, 1, NODE_30, 10000, 1e-7)

	DISCRETE_ADDER3(SPRINT2_MOTORSND1, SPRINT2_ATTRACT_EN, NODE_25, NODE_28, NODE_31)

	/************************************************/
	/* Crash circuit is built around a noise        */
	/* generator built from 2 shift registers that  */
	/* are clocked by the 2V signal.                */
	/* 2V = HSYNC/4                                 */
	/*    = 15750/4                                 */
	/* Output is binary weighted with 4 bits of     */
	/* crash volume.                                */
	/************************************************/
	DISCRETE_LFSR_NOISE(SPRINT2_NOISE, 1, SPRINT2_NOISE_RESET, 15750.0/4, 1.0, 0, 0, &sprint2_lfsr)

	DISCRETE_MULTIPLY(NODE_60, SPRINT2_CRASHSND_DATA, SPRINT2_NOISE, SPRINT2_CRASHSND_DATA)
	DISCRETE_RCFILTER(SPRINT2_CRASHSND, SPRINT2_CRASHSND_DATA, NODE_60, 545, 1e-7)

	/************************************************/
	/* Skid circuit takes the noise output from     */
	/* the crash circuit and applies +ve feedback   */
	/* to cause oscillation. There is also an RC    */
	/* filter on the input to the feedback cct.     */
	/* RC is 1K & 10uF                              */
	/* Feedback cct is modelled by using the RC out */
	/* as the frequency input on a VCO,             */
	/* breadboarded freq range as:                  */
	/*  0 = 940Hz, 34% duty                         */
	/*  1 = 630Hz, 29% duty                         */
	/*  the duty variance is so small we ignore it  */
	/************************************************/
	DISCRETE_INVERT(NODE_70, SPRINT2_NOISE)
	DISCRETE_MULTADD(NODE_71, 1, NODE_70, 940.0-630.0, ((940.0-630.0)/2)+630.0)
	DISCRETE_RCFILTER(NODE_72, 1, NODE_71, 1000, 1e-5)
	DISCRETE_SQUAREWAVE(SPRINT2_SKIDSND1, SPRINT2_SKIDSND1_EN, NODE_72, 407.8, 31.5, 0, 0.0)

	/************************************************/
	/* Combine all 3 sound sources.                 */
	/* Add some final gain to get to a good sound   */
	/* level.                                       */
	/************************************************/
	DISCRETE_ADDER3(NODE_90, 1, SPRINT2_MOTORSND1, SPRINT2_CRASHSND, SPRINT2_SKIDSND1)
	DISCRETE_GAIN(SPRINT2_FINAL_MIX1, NODE_90, 65534.0/(846.9+1000.0+407.8))

	DISCRETE_OUTPUT(SPRINT2_FINAL_MIX1, 100)
DISCRETE_SOUND_END

/************************************************************************/
/* dominos Sound System Analog emulation                               */
/************************************************************************/

/* Nodes - Inputs */
#define DOMINOS_FREQ_DATA          NODE_01
#define DOMINOS_AMP_DATA           NODE_02
#define DOMINOS_TOPPLE_EN          NODE_03
#define DOMINOS_ATTRACT_EN         NODE_04
/* Nodes - Sounds */
#define DOMINOS_TONE_SND           NODE_10
#define DOMINOS_TOPPLE_SND         NODE_11

static DISCRETE_SOUND_START(dominos_sound_interface)
	/************************************************/
	/* Dominos Effects Relataive Gain Table         */
	/*                                              */
	/* Effect       V-ampIn   Gain ratio  Relative  */
	/* Tone          3.8      5/(5+10)     1000.0   */
	/* Topple        3.8      5/(33+5)      394.7   */
	/************************************************/

	/************************************************/
	/* Input register mapping for dominos           */
	/************************************************/
	/*              NODE                  ADDR  MASK    GAIN      OFFSET  INIT */
	DISCRETE_INPUT (DOMINOS_TOPPLE_EN,    0x00, 0x000f,                   0.0)
	DISCRETE_INPUT (DOMINOS_FREQ_DATA,    0x02, 0x000f,                   0.0)
	DISCRETE_INPUTX(DOMINOS_AMP_DATA,     0x04, 0x000f, 1000.0/15, 0,     0.0)
	DISCRETE_INPUT (DOMINOS_ATTRACT_EN,   0x05, 0x000f,                   0.0)

	/************************************************/
	/* Tone Sound                                   */
	/* Note: True freq range has not been tested    */
	/************************************************/
	DISCRETE_RCFILTER(NODE_20, 1, DOMINOS_FREQ_DATA, 123000, 1e-7)
	DISCRETE_ADJUSTMENT(NODE_21, 1, (289.0-95.0)/3/15, (4500.0-95.0)/3/15, DISC_LOGADJ, 4)
	DISCRETE_MULTIPLY(NODE_22, 1, NODE_20, NODE_21)

	DISCRETE_ADDER2(NODE_23, 1, NODE_22, 95.0/3)
	DISCRETE_SQUAREWAVE(NODE_24, DOMINOS_ATTRACT_EN, NODE_23, DOMINOS_AMP_DATA, 100.0/3, 0, 360.0/3)
	DISCRETE_RCFILTER(DOMINOS_TONE_SND, 1, NODE_24, 546, 1e-7)

	/************************************************/
	/* Topple sound is just the 4V source           */
	/* 4V = HSYNC/8                                 */
	/*    = 15750/8                                 */
	/************************************************/
	DISCRETE_SQUAREWFIX(DOMINOS_TOPPLE_SND, DOMINOS_TOPPLE_EN, 15750.0/8, 394.7, 50.0, 0, 0)

	/************************************************/
	/* Combine both sound sources.                  */
	/* Add some final gain to get to a good sound   */
	/* level.                                       */
	/************************************************/
	DISCRETE_ADDER2(NODE_90, 1, DOMINOS_TONE_SND, DOMINOS_TOPPLE_SND)
	DISCRETE_GAIN(NODE_91, NODE_90, 65534.0/(1000.0+394.7))

	DISCRETE_OUTPUT(NODE_91, 100)
DISCRETE_SOUND_END


static MACHINE_DRIVER_START( sprint2 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6502, 12096000 / 16)
	MDRV_CPU_MEMORY(readmem, writemem)
	MDRV_CPU_VBLANK_INT(sprint2, 1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(38 * 1000000 / 15750)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 262)
	MDRV_VISIBLE_AREA(0, 511, 0, 223)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4)
	MDRV_COLORTABLE_LENGTH(12)

	MDRV_PALETTE_INIT(sprint2)
	MDRV_VIDEO_START(sprint2)
	MDRV_VIDEO_UPDATE(sprint2)
	MDRV_VIDEO_EOF(sprint2)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD_TAG("discrete", DISCRETE, sprint2_sound_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( sprint1 )

	MDRV_IMPORT_FROM(sprint2)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(0)
	MDRV_SOUND_REPLACE("discrete", DISCRETE, sprint1_sound_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( dominos )

	MDRV_IMPORT_FROM(sprint2)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(0)
	MDRV_SOUND_REPLACE("discrete", DISCRETE, dominos_sound_interface)
MACHINE_DRIVER_END


ROM_START( sprint1 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "6290-01.b1", 0x2000, 0x0800, CRC(41fc985e) SHA1(7178846480cbf8d15955ccd987d0b0e902ab9f90) )
	ROM_RELOAD(             0xe000, 0x0800 )
	ROM_LOAD( "6291-01.c1", 0x2800, 0x0800, CRC(07f7a920) SHA1(845f65d2bd290eb295ca6bae2575f27aaa08c0dd) )
	ROM_RELOAD(             0xe800, 0x0800 )
	ROM_LOAD( "6442-01.d1", 0x3000, 0x0800, CRC(e9ff0124) SHA1(42fe028e2e595573ccc0821de3bb6970364c585d) )
	ROM_RELOAD(             0xf000, 0x0800 )
	ROM_LOAD( "6443-01.e1", 0x3800, 0x0800, CRC(d6bb00d0) SHA1(cdcd4bb7b32be7a11480d3312fcd8d536e2d0caf) )
	ROM_RELOAD(             0xf800, 0x0800 )

	ROM_REGION( 0x0200, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD_NIB_HIGH( "6396-01.p4", 0x0000, 0x0200, CRC(801b42dd) SHA1(1db58390d803f404253cbf36d562016441ca568d) )
	ROM_LOAD_NIB_LOW ( "6397-01.r4", 0x0000, 0x0200, CRC(135ba1aa) SHA1(0465259440f73e1a2c8d8101f29e99b4885420e4) )

	ROM_REGION( 0x0200, REGION_GFX2, ROMREGION_DISPOSE ) /* cars */
	ROM_LOAD_NIB_HIGH( "6399-01.j6", 0x0000, 0x0200, CRC(63d685b2) SHA1(608746163e25dbc14cde43c17aecbb9a14fac875) )
	ROM_LOAD_NIB_LOW ( "6398-01.k6", 0x0000, 0x0200, CRC(c9e1017e) SHA1(e7279a13e4a812d2e0218be0bc5162f2e56c6b66) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "6400-01.m2", 0x0000, 0x0100, CRC(b8094b4c) SHA1(82dc6799a19984f3b204ee3aeeb007e55afc8be3) )	/* SYNC */
	ROM_LOAD( "6401-01.e2", 0x0100, 0x0020, CRC(857df8db) SHA1(06313d5bde03220b2bc313d18e50e4bb1d0cfbbb) )	/* address */
ROM_END


ROM_START( sprint2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "6290-01.b1", 0x2000, 0x0800, CRC(41fc985e) SHA1(7178846480cbf8d15955ccd987d0b0e902ab9f90) )
	ROM_RELOAD(             0xe000, 0x0800 )
	ROM_LOAD( "6291-01.c1", 0x2800, 0x0800, CRC(07f7a920) SHA1(845f65d2bd290eb295ca6bae2575f27aaa08c0dd) )
	ROM_RELOAD(             0xe800, 0x0800 )
	ROM_LOAD( "6404.d1",    0x3000, 0x0800, CRC(d2878ff6) SHA1(b742a8896c1bf1cfacf48d06908920d88a2c9ea8) )
	ROM_RELOAD(             0xf000, 0x0800 )
	ROM_LOAD( "6405.e1",    0x3800, 0x0800, CRC(6c991c80) SHA1(c30a5b340f05dd702c7a186eb62607a48fa19f72) )
	ROM_RELOAD(             0xf800, 0x0800 )

	ROM_REGION( 0x0200, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD_NIB_HIGH( "6396-01.p4", 0x0000, 0x0200, CRC(801b42dd) SHA1(1db58390d803f404253cbf36d562016441ca568d) )
	ROM_LOAD_NIB_LOW ( "6397-01.r4", 0x0000, 0x0200, CRC(135ba1aa) SHA1(0465259440f73e1a2c8d8101f29e99b4885420e4) )

	ROM_REGION( 0x0200, REGION_GFX2, ROMREGION_DISPOSE ) /* cars */
	ROM_LOAD_NIB_HIGH( "6399-01.j6", 0x0000, 0x0200, CRC(63d685b2) SHA1(608746163e25dbc14cde43c17aecbb9a14fac875) )
	ROM_LOAD_NIB_LOW ( "6398-01.k6", 0x0000, 0x0200, CRC(c9e1017e) SHA1(e7279a13e4a812d2e0218be0bc5162f2e56c6b66) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "6400-01.m2", 0x0000, 0x0100, CRC(b8094b4c) SHA1(82dc6799a19984f3b204ee3aeeb007e55afc8be3) )	/* SYNC */
	ROM_LOAD( "6401-01.e2", 0x0100, 0x0020, CRC(857df8db) SHA1(06313d5bde03220b2bc313d18e50e4bb1d0cfbbb) )	/* address */
ROM_END


ROM_START( sprint2a )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "6290-01.b1", 0x2000, 0x0800, CRC(41fc985e) SHA1(7178846480cbf8d15955ccd987d0b0e902ab9f90) )
	ROM_RELOAD(             0xe000, 0x0800 )
	ROM_LOAD( "6291-01.c1", 0x2800, 0x0800, CRC(07f7a920) SHA1(845f65d2bd290eb295ca6bae2575f27aaa08c0dd) )
	ROM_RELOAD(             0xe800, 0x0800 )
	ROM_LOAD( "6404.d1",    0x3000, 0x0800, CRC(d2878ff6) SHA1(b742a8896c1bf1cfacf48d06908920d88a2c9ea8) )
	ROM_RELOAD(             0xf000, 0x0800 )
	ROM_LOAD( "6405-02.e1", 0x3800, 0x0800, CRC(e80fd249) SHA1(7bcf7dfd72ca83fdd80593eaf392570da1f71298) ) 
	ROM_RELOAD(             0xf800, 0x0800 )

	ROM_REGION( 0x0200, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD_NIB_HIGH( "6396-01.p4", 0x0000, 0x0200, CRC(801b42dd) SHA1(1db58390d803f404253cbf36d562016441ca568d) )
	ROM_LOAD_NIB_LOW ( "6397-01.r4", 0x0000, 0x0200, CRC(135ba1aa) SHA1(0465259440f73e1a2c8d8101f29e99b4885420e4) )

	ROM_REGION( 0x0200, REGION_GFX2, ROMREGION_DISPOSE ) /* cars */
	ROM_LOAD_NIB_HIGH( "6399-01.j6", 0x0000, 0x0200, CRC(63d685b2) SHA1(608746163e25dbc14cde43c17aecbb9a14fac875) )
	ROM_LOAD_NIB_LOW ( "6398-01.k6", 0x0000, 0x0200, CRC(c9e1017e) SHA1(e7279a13e4a812d2e0218be0bc5162f2e56c6b66) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "6400-01.m2", 0x0000, 0x0100, CRC(b8094b4c) SHA1(82dc6799a19984f3b204ee3aeeb007e55afc8be3) )	/* SYNC */
	ROM_LOAD( "6401-01.e2", 0x0100, 0x0020, CRC(857df8db) SHA1(06313d5bde03220b2bc313d18e50e4bb1d0cfbbb) )	/* address */
ROM_END


ROM_START( dominos )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "7352-02.d1",   0x3000, 0x0800, CRC(738b4413) SHA1(3a90ab25bb5f65504692f97da43f03e21392dcd8) )
	ROM_RELOAD(               0xf000, 0x0800 )
	ROM_LOAD( "7438-02.e1",   0x3800, 0x0800, CRC(c84e54e2) SHA1(383b388a1448a195f28352fc5e4ff1a2af80cc95) )
	ROM_RELOAD(               0xf800, 0x0800 )

	ROM_REGION( 0x200, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD_NIB_HIGH( "7439-01.p4",   0x0000, 0x0200, CRC(4f42fdd6) SHA1(f8ea4b582e26cad37b746174cdc9f1c7ae0819c3) )
	ROM_LOAD_NIB_LOW ( "7440-01.r4",   0x0000, 0x0200, CRC(957dd8df) SHA1(280457392f40cd66eae34d2fcdbd4d2142793402) )

	ROM_REGION( 0x200, REGION_GFX2, ROMREGION_DISPOSE ) /* sprites, not used */
	ROM_FILL( 0x0000, 0x0200, 0 )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "6400-01.m2", 0x0000, 0x0100, CRC(b8094b4c) SHA1(82dc6799a19984f3b204ee3aeeb007e55afc8be3) )	/* SYNC */
	ROM_LOAD( "6401-01.e2", 0x0100, 0x0020, CRC(857df8db) SHA1(06313d5bde03220b2bc313d18e50e4bb1d0cfbbb) )	/* address */
ROM_END


GAME( 1978, sprint1,  0,       sprint1, sprint1, sprint1, ROT0, "Atari", "Sprint 1" )
GAME( 1976, sprint2,  sprint1, sprint2, sprint2, sprint2, ROT0, "Atari", "Sprint 2 (set 1)" )
GAME( 1976, sprint2a, sprint1, sprint2, sprint2, sprint2, ROT0, "Atari", "Sprint 2 (set 2)" )
GAME( 1977, dominos,  0,       dominos, dominos, dominos, ROT0, "Atari", "Dominos" )
