/****************************************************************************

Blockade/Comotion/Blasto/Hustle Memory MAP
Frank Palazzolo (palazzol@home.com)

CPU - Intel 8080A

Memory Address              (Upper/Lower)

0xxx 00aa aaaa aaaa     ROM     U2/U3    R       1K for Blockade/Comotion/Blasto
0xxx 01aa aaaa aaaa     ROM     U4/U5    R       1K for Comotion/Blasto/Hustle Only
xxx1 xxxx aaaa aaaa     RAM              R/W     256 bytes
1xx0 xxaa aaaa aaaa    VRAM                      1K playfield

                    CHAR ROM  U29/U43            256 bytes for Blockade/Comotion
                                                 512 for Blasto/Hustle

Ports    In            Out
1        Controls      bit 7 = Coin Latch Reset
                       bit 5 = Pin 19?
2        Controls      Square Wave Pitch Register
4        Controls      Noise On
8        N/A           Noise Off


Notes:  Support is complete with the exception of the square wave generator
        and noise generator.  I have not created a sample for the noise
		generator, but any BOOM sound as a sample will do for now for
		Blockade & Comotion, at least.

****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/* #define BLOCKADE_LOG 1 */

extern WRITE_HANDLER( blockade_videoram_w );

extern VIDEO_START( blockade );
extern VIDEO_UPDATE( blockade );

/* These are used to simulate coin latch circuitry */

static int coin_latch;  /* Active Low */
static int just_been_reset;

DRIVER_INIT( blockade )
{
	UINT8 *rom = memory_region(REGION_CPU1);
	int i;

	/* Merge nibble-wide roms together,
	   and load them into 0x0000-0x0400 */

	for (i = 0;i < 0x0400;i++)
	{
		rom[0x0000+i] = (rom[0x1000+i]<<4)+rom[0x1400+i];
	}

	/* Initialize the coin latch state here */
	coin_latch = 1;
	just_been_reset = 0;
}

DRIVER_INIT( comotion )
{
	UINT8 *rom = memory_region(REGION_CPU1);
	int i;

	/* Merge nibble-wide roms together,
	   and load them into 0x0000-0x0800 */

	for(i = 0;i < 0x0400;i++)
	{
		rom[0x0000+i] = (rom[0x1000+i]<<4)+rom[0x1400+i];
		rom[0x0400+i] = (rom[0x1800+i]<<4)+rom[0x1c00+i];
	}

	/* They also need to show up here for comotion/blasto */

	for(i = 0;i < 2048;i++)
	{
		rom[0x4000+i] = rom[0x0000+i];
	}

	/* Initialize the coin latch state here */
	coin_latch = 1;
	just_been_reset = 0;
}

/*************************************************************/
/*                                                           */
/* Inserting a coin should work like this:                   */
/*  1) Reset the CPU                                         */
/*  2) CPU Sets the coin latch                               */
/*  3) Finally the CPU coin latch is Cleared by the hardware */
/*     (by the falling coin..?)                              */
/*                                                           */
/*  I am faking this by keeping the CPU from Setting         */
/*  the coin latch if we have just been reset.               */
/*                                                           */
/*************************************************************/


/* Need to check for a coin on the interrupt, */
/* This will reset the cpu                    */

INTERRUPT_GEN( blockade_interrupt )
{
	timer_suspendcpu(0, 0, SUSPEND_ANY_REASON);

	if ((input_port_0_r(0) & 0x80) == 0)
	{
		just_been_reset = 1;
		cpu_set_reset_line(0,PULSE_LINE);
	}
}

READ_HANDLER( blockade_input_port_0_r )
{
    /* coin latch is bit 7 */

    int temp = (input_port_0_r(0)&0x7f);
    return (coin_latch<<7) | (temp);
}

WRITE_HANDLER( blockade_coin_latch_w )
{
    if (data & 0x80)
    {
    #ifdef BLOCKADE_LOG
        log_cb(RETRO_LOG_DEBUG, LOGPRE "Reset Coin Latch\n");
    #endif
        if (just_been_reset)
        {
            just_been_reset = 0;
            coin_latch = 0;
        }
        else
            coin_latch = 1;
    }

    if (data & 0x20)
    {
    #ifdef BLOCKADE_LOG
        log_cb(RETRO_LOG_DEBUG, LOGPRE "Pin 19 High\n");
    #endif
    }
    else
    {
    #ifdef BLOCKADE_LOG
        log_cb(RETRO_LOG_DEBUG, LOGPRE "Pin 19 Low\n");
    #endif
    }

    return;
}

WRITE_HANDLER( blockade_sound_freq_w )
{
#ifdef BLOCKADE_LOG
    log_cb(RETRO_LOG_DEBUG, LOGPRE "Sound Freq Write: %d\n",data);
#endif
    return;
}

WRITE_HANDLER( blockade_env_on_w )
{
#ifdef BLOCKADE_LOG
    log_cb(RETRO_LOG_DEBUG, LOGPRE "Boom Start\n");
#endif
    sample_start(0,0,0);
    return;
}

WRITE_HANDLER( blockade_env_off_w )
{
#ifdef BLOCKADE_LOG
    log_cb(RETRO_LOG_DEBUG, LOGPRE "Boom End\n");
#endif
    return;
}

static MEMORY_READ_START( readmem )
    { 0x0000, 0x07ff, MRA_ROM },
    { 0x4000, 0x47ff, MRA_ROM },  /* same image */
    { 0xe000, 0xe3ff, MRA_RAM },
    { 0xff00, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
    { 0x0000, 0x07ff, MWA_ROM },
    { 0x4000, 0x47ff, MWA_ROM },  /* same image */
    { 0xe000, 0xe3ff, blockade_videoram_w, &videoram },
    { 0xff00, 0xffff, MWA_RAM },
MEMORY_END

static PORT_READ_START( readport )
    { 0x01, 0x01, blockade_input_port_0_r },
    { 0x02, 0x02, input_port_1_r },
    { 0x04, 0x04, input_port_2_r },
PORT_END

static PORT_WRITE_START( writeport )
    { 0x01, 0x01, blockade_coin_latch_w },
    { 0x02, 0x02, blockade_sound_freq_w },
    { 0x04, 0x04, blockade_env_on_w },
    { 0x08, 0x08, blockade_env_off_w },
PORT_END

/* These are not dip switches, they are mapped to */
/* connectors on the board.  Different games had  */
/* different harnesses which plugged in here, and */
/* some pins were unused.                         */

INPUT_PORTS_START( blockade )
    PORT_START  /* IN0 */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_DIPNAME(    0x04, 0x04, "Boom Switch" )
    PORT_DIPSETTING( 0x00, DEF_STR( Off ) )
    PORT_DIPSETTING( 0x04, DEF_STR( On ) )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_DIPNAME(    0x70, 0x70, DEF_STR( Lives ) )
    PORT_DIPSETTING( 0x60, "3" )
    PORT_DIPSETTING( 0x50, "4" )
    PORT_DIPSETTING( 0x30, "5" )
    PORT_DIPSETTING( 0x70, "6" )
    PORT_BIT_IMPULSE( 0x80, IP_ACTIVE_LOW, IPT_COIN1, 1 )
                                /* this is really used for the coin latch,  */
                                /* see blockade_interrupt()                 */

    PORT_START  /* IN1 */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_PLAYER2 )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER2 )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_PLAYER2 )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_PLAYER2 )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_PLAYER1 )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER1 )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_PLAYER1 )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_PLAYER1 )

    PORT_START  /* IN2 */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN3 */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x7f, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( comotion )
    PORT_START  /* IN0 */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_DIPNAME(    0x04, 0x04, "Boom Switch" )
    PORT_DIPSETTING( 0x00, DEF_STR( Off ) )
    PORT_DIPSETTING( 0x04, DEF_STR( On ) )
    PORT_DIPNAME(    0x00, 0x08, DEF_STR( Lives ) )
    PORT_DIPSETTING( 0x00, "3" )
    PORT_DIPSETTING( 0x08, "4" )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT_IMPULSE( 0x80, IP_ACTIVE_LOW, IPT_COIN1, 1 )
                                /* this is really used for the coin latch,  */
                                /* see blockade_interrupt()                 */

    PORT_START  /* IN1 */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_PLAYER1 )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER1 )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_PLAYER1 )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_PLAYER1 )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_PLAYER3 )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER3 )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_PLAYER3 )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_PLAYER3 )

    PORT_START  /* IN2 */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_PLAYER2 )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER2 )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_PLAYER2 )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_PLAYER2 )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_PLAYER4 )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER4 )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_PLAYER4 )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_PLAYER4 )

	PORT_START	/* IN3 */
	PORT_BIT( 0x7f, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )
INPUT_PORTS_END

INPUT_PORTS_START( blasto )
    PORT_START  /* IN0 */
    PORT_DIPNAME(    0x03, 0x03, DEF_STR( Coinage ) )
    PORT_DIPSETTING( 0x00, DEF_STR( 4C_1C ) )
    PORT_DIPSETTING( 0x01, DEF_STR( 3C_1C ) )
    PORT_DIPSETTING( 0x02, DEF_STR( 2C_1C ) )
    PORT_DIPSETTING( 0x03, DEF_STR( 1C_1C ) )
    PORT_DIPNAME(    0x04, 0x04, "Boom Switch" )
    PORT_DIPSETTING( 0x00, DEF_STR( Off ) )
    PORT_DIPSETTING( 0x04, DEF_STR( On ) )
    PORT_DIPNAME(    0x08, 0x08, "Game Time" )
    PORT_DIPSETTING( 0x00, "70 Secs" )
    PORT_DIPSETTING( 0x08, "90 Secs" )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT_IMPULSE( 0x80, IP_ACTIVE_LOW, IPT_COIN1, 1 )
                                /* this is really used for the coin latch,  */
                                /* see blockade_interrupt()                 */

    PORT_START  /* IN1 */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )

    PORT_START  /* IN2 */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER2 )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_PLAYER2 )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_PLAYER2 )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_PLAYER2 )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER1 )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_PLAYER1 )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_PLAYER1 )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_PLAYER1 )

	PORT_START	/* IN3 */
	PORT_BIT( 0x7f, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )
INPUT_PORTS_END

INPUT_PORTS_START( hustle )
    PORT_START  /* IN0 */
    PORT_DIPNAME(    0x03, 0x03, DEF_STR( Coinage ) )
    PORT_DIPSETTING( 0x00, DEF_STR( 4C_1C ) )
    PORT_DIPSETTING( 0x01, DEF_STR( 3C_1C ) )
    PORT_DIPSETTING( 0x02, DEF_STR( 2C_1C ) )
    PORT_DIPSETTING( 0x03, DEF_STR( 1C_1C ) )
    PORT_DIPNAME(    0x04, 0x04, "Game Time" )
    PORT_DIPSETTING( 0x00, "1.5 mins" )
    PORT_DIPSETTING( 0x04, "2 mins" )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT_IMPULSE( 0x80, IP_ACTIVE_LOW, IPT_COIN1, 1 )
                                /* this is really used for the coin latch,  */
                                /* see blockade_interrupt()                 */

    PORT_START  /* IN1 */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_PLAYER2 )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER2 )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_PLAYER2 )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_PLAYER2 )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_PLAYER1 )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER1 )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_PLAYER1 )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_PLAYER1 )

    PORT_START  /* IN2 */
	PORT_DIPNAME( 0xf1, 0xf0, "Free Game" )
	PORT_DIPSETTING ( 0x71, "11000" )
    PORT_DIPSETTING ( 0xb1, "13000" )
	PORT_DIPSETTING ( 0xd1, "15000" )
	PORT_DIPSETTING ( 0xe1, "17000" )
    PORT_DIPSETTING ( 0xf0, "Disabled" )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN3 */
	PORT_BIT( 0x7f, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )
INPUT_PORTS_END



static struct GfxLayout blockade_layout =
{
	8,8,    /* 8*8 characters */
	32, /* 32 characters */
	1,  /* 1 bit per pixel */
	{ 0 },  /* no separation in 1 bpp */
	{ 4,5,6,7,256*8+4,256*8+5,256*8+6,256*8+7 },    /* Merge nibble-wide roms */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8 /* every char takes 8 consecutive bytes */
};

static struct GfxLayout blasto_layout =
{
	8,8,    /* 8*8 characters */
	64, /* 64 characters */
	1,  /* 1 bit per pixel */
	{ 0 },  /* no separation in 1 bpp */
	{ 4,5,6,7,512*8+4,512*8+5,512*8+6,512*8+7 },    /* Merge nibble-wide roms */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8 /* every char takes 8 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &blockade_layout, 0, 2 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo blasto_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &blasto_layout,   0, 2 },
	{ -1 } /* end of array */
};


static PALETTE_INIT( green )
{
	palette_set_color(0,0x00,0x00,0x00); /* BLACK */
	palette_set_color(1,0x00,0xff,0x00); /* GREEN */     /* overlay (Blockade) */
}

static PALETTE_INIT( yellow )
{
	palette_set_color(0,0x00,0x00,0x00); /* BLACK */
	palette_set_color(1,0xff,0xff,0x20); /* YELLOW */     /* overlay (Hustle) */
}
static PALETTE_INIT( bw )
{
	palette_set_color(0,0x00,0x00,0x00); /* BLACK */
	palette_set_color(1,0xff,0xff,0xff); /* WHITE */     /* Comotion/Blasto */
}



static const char *blockade_sample_names[] =
{
    "*blockade",
    "BOOM.wav",
    0   /* end of array */
};

static struct Samplesinterface samples_interface =
{
    1,	/* 1 channel */
	25,	/* volume */
	blockade_sample_names
};



static MACHINE_DRIVER_START( blockade )

	/* basic machine hardware */
	MDRV_CPU_ADD(8080, 2079000)
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_PORTS(readport,writeport)
	MDRV_CPU_VBLANK_INT(blockade_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2)
	MDRV_COLORTABLE_LENGTH(2)

	MDRV_PALETTE_INIT(green)
	MDRV_VIDEO_START(blockade)
	MDRV_VIDEO_UPDATE(blockade)

	/* sound hardware */
	MDRV_SOUND_ADD(SAMPLES, samples_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( comotion )
	MDRV_IMPORT_FROM(blockade)
	MDRV_PALETTE_INIT(bw)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( blasto )
	MDRV_IMPORT_FROM(blockade)
	MDRV_GFXDECODE(blasto_gfxdecodeinfo)
	MDRV_PALETTE_INIT(bw)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( hustle )
	MDRV_IMPORT_FROM(blockade)
	MDRV_GFXDECODE(blasto_gfxdecodeinfo)
	MDRV_PALETTE_INIT(yellow)
MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( blockade )
    ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
    /* Note: These are being loaded into a bogus location, */
    /*       They are nibble wide rom images which will be */
    /*       merged and loaded into the proper place by    */
    /*       blockade_rom_init()                           */
    ROM_LOAD( "316-04.u2", 0x1000, 0x0400, CRC(a93833e9) SHA1(e29e7b29900f8305effa700a53806a12bf9d37bd) )
    ROM_LOAD( "316-03.u3", 0x1400, 0x0400, CRC(85960d3b) SHA1(aabfe8f9c26126299d6c07a31ef1aac5300deff5) )

    ROM_REGION( 0x200, REGION_GFX1, ROMREGION_DISPOSE )
    ROM_LOAD( "316-02.u29", 0x0000, 0x0100, CRC(409f610f) SHA1(0c2253f4b72d8aa395f87cc0abe07f0b46fa538b) )
    ROM_LOAD( "316-01.u43", 0x0100, 0x0100, CRC(41a00b28) SHA1(2d0a90aac9d10a1ded240e5202fdf9cd7f70c4a7) )
ROM_END

ROM_START( comotion )
    ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
    /* Note: These are being loaded into a bogus location, */
    /*       They are nibble wide rom images which will be */
    /*       merged and loaded into the proper place by    */
    /*       comotion_rom_init()                           */
    ROM_LOAD( "316-07.u2", 0x1000, 0x0400, CRC(5b9bd054) SHA1(324b844788945e7bc82d096d6d375e79e3e1a634) )
    ROM_LOAD( "316-08.u3", 0x1400, 0x0400, CRC(1a856042) SHA1(91bdc260e8c88ce2b6ac05bfba043ed611bc30de) )
    ROM_LOAD( "316-09.u4", 0x1800, 0x0400, CRC(2590f87c) SHA1(95a7af04b610d79fb3f6d74dda322e66164b9484) )
    ROM_LOAD( "316-10.u5", 0x1c00, 0x0400, CRC(fb49a69b) SHA1(4009c3256a86508d981c1f77b65e6bff1face1e7) )

    ROM_REGION( 0x200, REGION_GFX1, ROMREGION_DISPOSE )
    ROM_LOAD( "316-06.u43", 0x0000, 0x0100, CRC(8f071297) SHA1(811471c87b77b4b9ab056cf0c0743fc2616b754c) )  /* Note: these are reversed */
    ROM_LOAD( "316-05.u29", 0x0100, 0x0100, CRC(53fb8821) SHA1(0a499aa4cf15f7ebea155aacd914de8851544215) )
ROM_END

ROM_START( blasto )
    ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
    /* Note: These are being loaded into a bogus location, */
    /*       They are nibble wide rom images which will be */
    /*       merged and loaded into the proper place by    */
    /*       comotion_rom_init()                           */
    ROM_LOAD( "blasto.u2", 0x1000, 0x0400, CRC(ec99d043) SHA1(10650e54bf55f3ace5c199215c2fce211916d3b7) )
    ROM_LOAD( "blasto.u3", 0x1400, 0x0400, CRC(be333415) SHA1(386cab720f0c2da16b9ec84f67ccebf23406c58d) )
    ROM_LOAD( "blasto.u4", 0x1800, 0x0400, CRC(1c889993) SHA1(e23c72d075cf3d209081bca5a953c33c8ae042ea) )
    ROM_LOAD( "blasto.u5", 0x1c00, 0x0400, CRC(efb640cb) SHA1(2dff5b249f876d7d13cc6dfad652ce7e5af10370) )

    ROM_REGION( 0x400, REGION_GFX1, ROMREGION_DISPOSE )
    ROM_LOAD( "blasto.u29", 0x0000, 0x0200, CRC(4dd69499) SHA1(34f097477a297bf5f986804e5967c92f9292be29) )
    ROM_LOAD( "blasto.u43", 0x0200, 0x0200, CRC(104051a4) SHA1(cae6b9d48e3eda5ba12ff5d9835ce2733e90f774) )
ROM_END

ROM_START( hustle )
    ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
    /* Note: These are being loaded into a bogus location, */
    /*       They are nibble wide rom images which will be */
    /*       merged and loaded into the proper place by    */
    /*       comotion_rom_init()                           */
    ROM_LOAD( "3160016.u2", 0x1000, 0x0400, CRC(d983de7c) SHA1(af6e0ea78449bfba4fe8affd724d7b0eb3d38706) )
    ROM_LOAD( "3160017.u3", 0x1400, 0x0400, CRC(edec9cb9) SHA1(548cc7b0a15a1c977b7ef4a99ff88101893f661a) )
    ROM_LOAD( "3160018.u4", 0x1800, 0x0400, CRC(f599b9c0) SHA1(c55ed33ac51b9cfbb2fe4321bbb1e0a16694f065) )
    ROM_LOAD( "3160019.u5", 0x1c00, 0x0400, CRC(7794bc7e) SHA1(b3d577291dea0e096b2ee56b0ef612f41b2e859c) )

    ROM_REGION( 0x400, REGION_GFX1, ROMREGION_DISPOSE )
    ROM_LOAD( "3160020.u29", 0x0000, 0x0200, CRC(541d2c67) SHA1(abdb918f302352693870b0a50eabaf95acf1cf63) )
    ROM_LOAD( "3160021.u43", 0x0200, 0x0200, CRC(b5083128) SHA1(d7e8242e9d12d09f3d69c08e373ede2bdd4deba9) )
ROM_END

ROM_START( mineswpr )
    ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
    /* Note: These are being loaded into a bogus location, */
    /*       They are nibble wide rom images which will be */
    /*       merged and loaded into the proper place by    */
    /*       blockade_rom_init()                           */
    ROM_LOAD( "mineswee.h0p", 0x1000, 0x0400, CRC(5850a4ba) SHA1(9f097d31428f4494573187049c53fbed2075ff32) )
    ROM_LOAD( "mineswee.l0p", 0x1400, 0x0400, CRC(05961379) SHA1(3d59341be8a663e8c54c1556442c992a6eb886ab) )

    ROM_REGION( 0x400, REGION_GFX1, ROMREGION_DISPOSE )
    ROM_LOAD( "mineswee.ums", 0x0000, 0x0200, CRC(0e1c5c37) SHA1(d4d56bd63307e387771c48304724dfc1ea1306d9) )
    ROM_LOAD( "mineswee.uls", 0x0200, 0x0200, CRC(3a4f66e1) SHA1(bd7f6c51d568a79fb06414b2a6ef245d0d983c3e) )
ROM_END

GAMEX( 1976, blockade, 0, blockade, blockade, blockade, ROT0, "Gremlin", "Blockade", GAME_IMPERFECT_SOUND )
GAMEX( 1976, comotion, 0, comotion, comotion, comotion, ROT0, "Gremlin", "Comotion", GAME_IMPERFECT_SOUND )
GAMEX( 1978, blasto,   0, blasto,   blasto,   comotion, ROT0, "Gremlin", "Blasto", GAME_IMPERFECT_SOUND )
GAMEX( 1977, hustle,   0, hustle,   hustle,   comotion, ROT0, "Gremlin", "Hustle", GAME_IMPERFECT_SOUND )
GAMEX( 1977, mineswpr, 0, blasto,   blockade, blockade, ROT0, "Amutech", "Minesweeper", GAME_IMPERFECT_SOUND )
