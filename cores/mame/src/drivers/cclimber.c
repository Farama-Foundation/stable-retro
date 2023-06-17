/***************************************************************************

Crazy Climber memory map (preliminary)
as described by Lionel Theunissen (lionelth@ozemail.com.au)

Crazy Kong is very similar to Crazy Climber, there is an additional ROM at
5000-5fff and RAM is at 6000-6bff. Dip switches and input connections are
different as well.

Swimmer is similar but also different (e.g. it has two CPUs and two 8910,
graphics are 3bpp instead of 2)

0000h-4fffh ;20k program ROMs. ROM11=0000h
                               ROM10=1000h
                               ROM09=2000h
                               ROM08=3000h
                               ROM07=4000h

8000h-83ffh ;1k scratchpad RAM.
8800h-88ffh ;256 bytes Bigsprite RAM.
9000h-93ffh ;1k screen RAM.
9800h-981fh ;Column smooth scroll position. Corresponds to each char
             column.

9880h-989fh ;Sprite controls. 8 groups of 4 bytes:
  1st byte; code/attribute.
            Bits 0-5: sprite code.
            Bit    6: x invert.
            Bit    7: y invert.
  2nd byte ;color.
            Bits 0-3: colour. (palette scheme 0-15)
            Bit    4: 0=charset1, 1 =charset 2.
  3rd byte ;y position
  4th byte ;x position

98dc        bit 0  big sprite priority over sprites? (1 = less priority)
98ddh ;Bigsprite colour/attribute.
            Bit 0-2: Big sprite colour.
            bit 3  ??
            Bit   4: x invert.
            Bit   5: y invert.
98deh ;Bigsprite y position.
98dfh ;Bigsprite x position.

9c00h-9fffh ;1/2k colour RAM: Bits 0-3: colour. (palette scheme 0-15)
                              Bit    4: 0=charset1, 1=charset2.
                              Bit    5: (not used by CC)
                              Bit    6: x invert.
                              Bit    7: y invert. (not used by CC)

a000h ;RD: Player 1 controls.
            Bit 0: Left up
                1: Left down
                2: Left left
                3: Left right
                4: Right up
                5: Right down
                6: Right left
                7: Right right

a000h ;WR: Non Maskable interrupt.
            Bit 0: 0=NMI disable, 1=NMI enable.

a001h ;WR: Horizontal video direction (Crazy Kong sets it to 1).
            Bit 0: 0=Normal, 1=invert.

a002h ;WR: Vertical video direction (Crazy Kong sets it to 1).
            Bit 0: 0=Normal, 1=invert.

a004h ;WR: Sample trigger.
            Bit 0: 0=Trigger.

a800h ;RD: Player 2 controls (table model only).
            Bit 0: Left up
                1: Left down
                2: Left left
                3: Left right
                4: Right up
                5: Right down
                6: Right left
                7: Right right


a800h ;WR: Sample rate speed.
              Full byte value (0-255).

b000h ;RD: DIP switches.
            Bit 1,0: Number of climbers.
                     00=3, 01=4, 10=5, 11=6.
            Bit   2: Extra climber bonus.
                     0=30000, 1=50000.
            Bit   3: 1=Test Pattern
            Bit 5,4: Coins per credit.
                     00=1, 01=2, 10=3 11=4.
            Bit 7,6: Plays per credit.
                     00=1, 01=2, 10=3, 11=Freeplay.

b000h ;WR: Sample volume.
            Bits 0-5: Volume (0-31).

b800h ;RD: Machine switches.
            Bit 0: Coin 1.
            Bit 1: Coin 2.
            Bit 2: 1 Player start.
            Bit 3: 2 Player start.
            Bit 4: Upright/table select.
                   0=table, 1=upright.


I/O 8  ;AY-3-8910 Control Reg.
I/O 9  ;AY-3-8910 Data Write Reg.
I/O C  ;AY-3-8910 Data Read Reg.
        Port A of the 8910 selects the digital sample to play

Changes:
25 Jan 98 LBO
        * Added support for the real Swimmer bigsprite ROMs, courtesy of Gary Walton.
        * Increased the IRQs for the Swimmer audio CPU to 4 to make it more "jaunty".
          Not sure if this is accurate, but it should be closer.
3 Mar 98 LBO
        * Added alternate version of Swimmer.

TODO:
        * Verify timings of sound/music on Swimmer.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



extern unsigned char *cclimber_bsvideoram;
extern size_t cclimber_bsvideoram_size;
extern unsigned char *cclimber_bigspriteram;
extern unsigned char *cclimber_column_scroll;
WRITE_HANDLER( cclimber_colorram_w );
WRITE_HANDLER( cclimber_bigsprite_videoram_w );
PALETTE_INIT( cclimber );
VIDEO_UPDATE( cclimber );

extern struct AY8910interface cclimber_ay8910_interface;
extern struct AY8910interface swimmer_ay8910_interface;
extern struct CustomSound_interface cclimber_custom_interface;
WRITE_HANDLER( cclimber_sample_trigger_w );
WRITE_HANDLER( cclimber_sample_rate_w );
WRITE_HANDLER( cclimber_sample_volume_w );


static WRITE_HANDLER( flip_screen_x_w )
{
	flip_screen_x_set(data);
}

static WRITE_HANDLER( flip_screen_y_w )
{
	flip_screen_y_set(data);
}


static MACHINE_INIT( cclimber )
{
	/* Disable interrupts, River Patrol / Silver Land needs this */
	cpu_interrupt_enable(0,0);
}



/* Note that River Patrol reads/writes to a000-a4f0. This is a bug in the code.
   The instruction at 0x0593 should say LD DE,$8000 */

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0x6000, 0x6bff, MRA_RAM },	/* Crazy Kong only */
	{ 0x8000, 0x83ff, MRA_RAM },
	{ 0x8800, 0x8bff, MRA_RAM },
	{ 0x9000, 0x93ff, MRA_RAM },	/* video RAM */
	{ 0x9800, 0x9bff, MRA_RAM },	/* column scroll registers */
	{ 0x9c00, 0x9fff, MRA_RAM },	/* color RAM */
	{ 0xa000, 0xa000, input_port_0_r },     /* IN0 */
	{ 0xa800, 0xa800, input_port_1_r },     /* IN1 */
	{ 0xb000, 0xb000, input_port_2_r },     /* DSW */
	{ 0xb800, 0xb800, input_port_3_r },     /* IN2 */
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x5fff, MWA_ROM },
	{ 0x6000, 0x6bff, MWA_RAM },    /* Crazy Kong only */
	{ 0x8000, 0x83ff, MWA_RAM },
	{ 0x8800, 0x88ff, cclimber_bigsprite_videoram_w, &cclimber_bsvideoram, &cclimber_bsvideoram_size },
	{ 0x8900, 0x8bff, MWA_RAM },  /* not used, but initialized */
	{ 0x9000, 0x93ff, videoram_w, &videoram, &videoram_size },
	{ 0x9400, 0x97ff, videoram_w }, /* mirror address, used by Crazy Climber to draw windows */
	/* 9800-9bff and 9c00-9fff share the same RAM, interleaved */
	/* (9800-981f for scroll, 9c20-9c3f for color RAM, and so on) */
	{ 0x9800, 0x981f, MWA_RAM, &cclimber_column_scroll },
	{ 0x9880, 0x989f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x98dc, 0x98df, MWA_RAM, &cclimber_bigspriteram },
	{ 0x9800, 0x9bff, MWA_RAM },  /* not used, but initialized */
	{ 0x9c00, 0x9fff, cclimber_colorram_w, &colorram },
	{ 0xa000, 0xa000, interrupt_enable_w },
	{ 0xa001, 0xa001, flip_screen_x_w },
	{ 0xa002, 0xa002, flip_screen_y_w },
	{ 0xa004, 0xa004, cclimber_sample_trigger_w },
	{ 0xa800, 0xa800, cclimber_sample_rate_w },
	{ 0xb000, 0xb000, cclimber_sample_volume_w },
MEMORY_END

static MEMORY_READ_START( cannonb_readmem )
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0x6000, 0x6bff, MRA_RAM },	/* Crazy Kong only, Cannon Ball also*/
	{ 0x8000, 0x83ff, MRA_RAM },

	{ 0x8800, 0x8800, MRA_NOP }, /* must not return what's written (game will reset after coin insert if it returns 0xff)*/

/*{ 0x8800, 0x8bff, MRA_RAM },*/

	{ 0x9000, 0x93ff, MRA_RAM },	/* video RAM */
	{ 0x9800, 0x981f, MRA_RAM },	/* column scroll registers */
	{ 0x9820, 0x9bff, MRA_RAM },	/* */
	{ 0x9c00, 0x9fff, MRA_RAM },	/* color RAM */

	{ 0xa000, 0xa000, input_port_0_r },     /* IN0 */
	{ 0xa800, 0xa800, input_port_1_r },     /* IN1 */
	{ 0xb000, 0xb000, input_port_2_r },     /* DSW */
	{ 0xb800, 0xb800, input_port_3_r },     /* IN2 */
MEMORY_END

static MEMORY_WRITE_START( cannonb_writemem )

	{ 0x5045, 0x505f, MWA_NOP},	/* do not errorlog this */

	{ 0x0000, 0x5fff, MWA_ROM },

	{ 0x6000, 0x6bff, MWA_RAM },    /* Crazy Kong only, Cannon Ball also */
	{ 0x8000, 0x83ff, MWA_RAM },

	{ 0x8800, 0x88ff, cclimber_bigsprite_videoram_w, &cclimber_bsvideoram, &cclimber_bsvideoram_size },
/*{ 0x8900, 0x8bff, MWA_RAM },   // not used, but initialized /*/
	{ 0x9000, 0x93ff, videoram_w, &videoram, &videoram_size },
	{ 0x9400, 0x97ff, videoram_w }, /* mirror address, used by Crazy Climber to draw windows */
	/* 9800-9bff and 9c00-9fff share the same RAM, interleaved */
	/* (9800-981f for scroll, 9c20-9c3f for color RAM, and so on) */
	{ 0x9800, 0x981f, MWA_RAM, &cclimber_column_scroll },
	{ 0x9880, 0x989f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x98dc, 0x98df, MWA_RAM, &cclimber_bigspriteram },
	{ 0x9800, 0x9bff, MWA_RAM },  /* not used, but initialized */
	{ 0x9c00, 0x9fff, cclimber_colorram_w, &colorram },
	{ 0xa000, 0xa000, interrupt_enable_w },
	{ 0xa001, 0xa001, flip_screen_x_w },
	{ 0xa002, 0xa002, flip_screen_y_w },
	{ 0xa004, 0xa004, cclimber_sample_trigger_w },
	{ 0xa800, 0xa800, cclimber_sample_rate_w },
	{ 0xb000, 0xb000, cclimber_sample_volume_w },
MEMORY_END


static PORT_READ_START( readport )
	{ 0x0c, 0x0c, AY8910_read_port_0_r },
PORT_END

static PORT_WRITE_START( writeport )
	{ 0x08, 0x08, AY8910_control_port_0_w },
	{ 0x09, 0x09, AY8910_write_port_0_w },
PORT_END



INPUT_PORTS_START( cclimber )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_UP     | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_DOWN   | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_LEFT   | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_RIGHT  | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_UP    | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_DOWN  | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_LEFT  | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_RIGHT | IPF_8WAY )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_UP     | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_DOWN   | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_LEFT   | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_RIGHT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_RIGHT | IPF_8WAY | IPF_COCKTAIL )

	PORT_START      /* DSW */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )		/* Look code at 0x03c4 : 0x8076 is never tested !*/
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_BITX(    0x08, 0x00, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Rack Test", KEYCODE_F1, IP_JOY_NONE )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coin_B ) )		/* Also "Bonus Life" due to code at 0x03d4*/
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )		/* Bonus life : 30000 points*/
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_2C ) )		/* Bonus life : 50000 points*/
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )		/* Bonus life : 30000 points*/
	PORT_DIPSETTING(    0xc0, DEF_STR( Free_Play ) )	/* Bonus life : 50000 points*/

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_BIT( 0xe0, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

/* Same as 'cclimber' but correct "Bonus Life" Dip Switch */
INPUT_PORTS_START( cclimbrj )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_UP     | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_DOWN   | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_LEFT   | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_RIGHT  | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_UP    | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_DOWN  | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_LEFT  | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_RIGHT | IPF_8WAY )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_UP     | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_DOWN   | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_LEFT   | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_RIGHT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_RIGHT | IPF_8WAY | IPF_COCKTAIL )

	PORT_START      /* DSW */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "30000" )
	PORT_DIPSETTING(    0x04, "50000" )
	PORT_BITX(    0x08, 0x00, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Rack Test", KEYCODE_F1, IP_JOY_NONE )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( Free_Play ) )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_BIT( 0xe0, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END


/* copy of ckong with few changes */
INPUT_PORTS_START( cannonb )
	PORT_START      /* IN0 */
	PORT_BIT( 0x07, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_4WAY )

	PORT_START      /* IN1 */
	PORT_BIT( 0x07, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_COCKTAIL )

	PORT_START      /* DSW */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x00, "Player title" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Bonus_Life ) ) /*there is cocktail mode around here*/
	PORT_DIPSETTING(    0x00, "7000" )/*on*/
	PORT_DIPSETTING(    0x04, "10000" )/*cocktail off*/
	PORT_DIPSETTING(    0x08, "15000" )/*on*/
	PORT_DIPSETTING(    0x0c, "20000" )/*cocktail off*/
	PORT_DIPNAME( 0x70, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x80, 0x00, "Unknown 1" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START      /* IN2 - verified */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


/* several differences with cclimber: note that IN2 bits are ACTIVE_LOW, while in */
/* cclimber they are ACTIVE_HIGH. */
INPUT_PORTS_START( ckong )
	PORT_START      /* IN0 */
	PORT_BIT( 0x07, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY )

	PORT_START      /* IN1 */
	PORT_BIT( 0x07, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )

	PORT_START      /* DSW */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "7000" )
	PORT_DIPSETTING(    0x04, "10000" )
	PORT_DIPSETTING(    0x08, "15000" )
	PORT_DIPSETTING(    0x0c, "20000" )
	PORT_DIPNAME( 0x70, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( rpatrolb )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x3e, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x3e, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )

	PORT_START      /* DSW */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x08, "5" )
	PORT_DIPSETTING(    0x0c, "6" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x20, 0x00, "Unknown 1" )  /* Probably unused */
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "Unknown 2" )  /* Probably unused */
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Memory Test" )
	PORT_DIPSETTING(    0x00, "Retry on Error" )
	PORT_DIPSETTING(    0x80, "Stop on Error" )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,    /* 8*8 characters */
	512,    /* 512 characters (256 in Crazy Climber) */
	2,      /* 2 bits per pixel */
	{ 0, 512*8*8 }, /* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },     /* pretty straightforward layout */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8     /* every char takes 8 consecutive bytes */
};
static struct GfxLayout bscharlayout =
{
	8,8,    /* 8*8 characters */
	256,    /* 256 characters */
	2,      /* 2 bits per pixel */
	{ 0, 256*8*8 }, /* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },     /* pretty straightforward layout */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8     /* every char takes 8 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,  /* 16*16 sprites */
	128,    /* 128 sprites (64 in Crazy Climber) */
	2,      /* 2 bits per pixel */
	{ 0, 128*16*16 },       /* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,       /* pretty straightforward layout */
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8    /* every sprite takes 32 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &charlayout,      0, 16 }, /* char set #1 */
	{ REGION_GFX1, 0x2000, &charlayout,      0, 16 }, /* char set #2 */
	{ REGION_GFX2, 0x0000, &bscharlayout, 16*4,  8 }, /* big sprite char set */
	{ REGION_GFX1, 0x0000, &spritelayout,    0, 16 }, /* sprite set #1 */
	{ REGION_GFX1, 0x2000, &spritelayout,    0, 16 }, /* sprite set #2 */
	{ -1 } /* end of array */
};



static MACHINE_DRIVER_START( cclimber )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 3072000)	/* 3.072 MHz */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_PORTS(readport,writeport)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_INIT(cclimber)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(96)
	MDRV_COLORTABLE_LENGTH(16*4+8*4)

	MDRV_PALETTE_INIT(cclimber)
	MDRV_VIDEO_START(generic)
	MDRV_VIDEO_UPDATE(cclimber)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, cclimber_ay8910_interface)
	MDRV_SOUND_ADD(CUSTOM, cclimber_custom_interface)
MACHINE_DRIVER_END


/* copy of cclimber except for readmem and writemem */
static MACHINE_DRIVER_START( cannonb )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 3072000)	/* 3.072 MHz */
	MDRV_CPU_MEMORY(cannonb_readmem,cannonb_writemem)
	MDRV_CPU_PORTS(readport,writeport)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_INIT(cclimber)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(96)
	MDRV_COLORTABLE_LENGTH(16*4+8*4)

	MDRV_PALETTE_INIT(cclimber)
	MDRV_VIDEO_START(generic)
	MDRV_VIDEO_UPDATE(cclimber)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, cclimber_ay8910_interface)
	MDRV_SOUND_ADD(CUSTOM, cclimber_custom_interface)
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( cclimber )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 )	/* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "cc11",         0x0000, 0x1000, CRC(217ec4ff) SHA1(334604c3a051d57440a9d0bfc34b809418ef1d2d) )
	ROM_LOAD( "cc10",         0x1000, 0x1000, CRC(b3c26cef) SHA1(f52cb5482c12a9c5fb56e2e2aec7cab0ed23e5a5) )
	ROM_LOAD( "cc09",         0x2000, 0x1000, CRC(6db0879c) SHA1(c0ba1976c1dcd6edadd78073173a26851ae8dd4f) )
	ROM_LOAD( "cc08",         0x3000, 0x1000, CRC(f48c5fe3) SHA1(79072bbbf37387998ffd031afe8eb569a16fa9bd) )
	ROM_LOAD( "cc07",         0x4000, 0x1000, CRC(3e873baf) SHA1(8870dc5948cdd3c8d2fe9e54a20cf6c311c94e53) )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "cc06",         0x0000, 0x0800, CRC(481b64cc) SHA1(3f35c545fc784ed4f969aba2d7be6e13a5ae32b7) )
	/* empty hole - Crazy Kong has an additional ROM here */
	ROM_LOAD( "cc04",         0x1000, 0x0800, CRC(332347cb) SHA1(4115ca32af73f1791635b7d9e093bf77088a8222) )
	/* empty hole - Crazy Kong has an additional ROM here */
	ROM_LOAD( "cc05",         0x2000, 0x0800, CRC(2c33b760) SHA1(2edea8fe13376fbd51a5586d97aba3b30d78e94b) )
	/* empty hole - Crazy Kong has an additional ROM here */
	ROM_LOAD( "cc03",         0x3000, 0x0800, CRC(4e4b3658) SHA1(0d39a8cb5cd6cf06008be60707f9b277a8a32a2d) )
	/* empty hole - Crazy Kong has an additional ROM here */

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "cc02",         0x0000, 0x0800, CRC(14f3ecc9) SHA1(a1b5121abfbe8f07580eb3fa6384352d239a3d75) )
	ROM_LOAD( "cc01",         0x0800, 0x0800, CRC(21c0f9fb) SHA1(44fad56d302a439257216ddac9fd62b3666589f1) )

	ROM_REGION( 0x0060, REGION_PROMS, 0 )
	ROM_LOAD( "cclimber.pr1", 0x0000, 0x0020, CRC(751c3325) SHA1(edce2bc883996c1d72dc6c1c9f62799b162d415a) )
	ROM_LOAD( "cclimber.pr2", 0x0020, 0x0020, CRC(ab1940fa) SHA1(8d98e05cbaa6f55770c12e0a9a8ed9c73cc54423) )
	ROM_LOAD( "cclimber.pr3", 0x0040, 0x0020, CRC(71317756) SHA1(1195f0a037e379cc1a3c0314cb746f5cd2bffe50) )

	ROM_REGION( 0x2000, REGION_SOUND1, 0 )	/* samples */
	ROM_LOAD( "cc13",         0x0000, 0x1000, CRC(e0042f75) SHA1(86cb31b110742a0f7ae33052c88f42d00deb5468) )
	ROM_LOAD( "cc12",         0x1000, 0x1000, CRC(5da13aaa) SHA1(b2d41e69435d09c456648a10e33f5e1fbb0bc64c) )
ROM_END

ROM_START( cclimbrj )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 )	/* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "cc11j.bin",    0x0000, 0x1000, CRC(89783959) SHA1(948fa88fcb9e3797b9c10934d36cf6a55cb590fe) )
	ROM_LOAD( "cc10j.bin",    0x1000, 0x1000, CRC(14eda506) SHA1(4bc55b4c4ec197952b05ad32584f15f0383cc2df) )
	ROM_LOAD( "cc09j.bin",    0x2000, 0x1000, CRC(26489069) SHA1(9be4d4a22dd334e619416e6c846a05003c0d687e) )
	ROM_LOAD( "cc08j.bin",    0x3000, 0x1000, CRC(b33c96f8) SHA1(3974f4a60f37bed9e4faee7dafb565f553b9c201) )
	ROM_LOAD( "cc07j.bin",    0x4000, 0x1000, CRC(fbc9626c) SHA1(32be2d06321b2943718d0bec77ec9ebb806e4b93) )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "cc06",         0x0000, 0x0800, CRC(481b64cc) SHA1(3f35c545fc784ed4f969aba2d7be6e13a5ae32b7) )
	/* empty hole - Crazy Kong has an additional ROM here */
	ROM_LOAD( "cc04",         0x1000, 0x0800, CRC(332347cb) SHA1(4115ca32af73f1791635b7d9e093bf77088a8222) )
	/* empty hole - Crazy Kong has an additional ROM here */
	ROM_LOAD( "cc05",         0x2000, 0x0800, CRC(2c33b760) SHA1(2edea8fe13376fbd51a5586d97aba3b30d78e94b) )
	/* empty hole - Crazy Kong has an additional ROM here */
	ROM_LOAD( "cc03",         0x3000, 0x0800, CRC(4e4b3658) SHA1(0d39a8cb5cd6cf06008be60707f9b277a8a32a2d) )
	/* empty hole - Crazy Kong has an additional ROM here */

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "cc02",         0x0000, 0x0800, CRC(14f3ecc9) SHA1(a1b5121abfbe8f07580eb3fa6384352d239a3d75) )
	ROM_LOAD( "cc01",         0x0800, 0x0800, CRC(21c0f9fb) SHA1(44fad56d302a439257216ddac9fd62b3666589f1) )

	ROM_REGION( 0x0060, REGION_PROMS, 0 )
	ROM_LOAD( "cclimber.pr1", 0x0000, 0x0020, CRC(751c3325) SHA1(edce2bc883996c1d72dc6c1c9f62799b162d415a) )
	ROM_LOAD( "cclimber.pr2", 0x0020, 0x0020, CRC(ab1940fa) SHA1(8d98e05cbaa6f55770c12e0a9a8ed9c73cc54423) )
	ROM_LOAD( "cclimber.pr3", 0x0040, 0x0020, CRC(71317756) SHA1(1195f0a037e379cc1a3c0314cb746f5cd2bffe50) )

	ROM_REGION( 0x2000, REGION_SOUND1, 0 )	/* samples */
	ROM_LOAD( "cc13j.bin",    0x0000, 0x1000, CRC(5f0bcdfb) SHA1(7f79bf6de117348f606696ed7ea1937bbf926612) )
	ROM_LOAD( "cc12j.bin",    0x1000, 0x1000, CRC(9003ffbd) SHA1(fd016056aabc23957643f37230f03842294f795e) )
ROM_END

ROM_START( ccboot )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 )	/* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "m11.bin",      0x0000, 0x1000, CRC(5efbe180) SHA1(e0c24f21d563da075eb5019d0e76cb01c2598c7a) )
	ROM_LOAD( "m10.bin",      0x1000, 0x1000, CRC(be2748c7) SHA1(ae66bc4e5e02bf9944a3ee4b0d2dec073f732260) )
	ROM_LOAD( "cc09j.bin",    0x2000, 0x1000, CRC(26489069) SHA1(9be4d4a22dd334e619416e6c846a05003c0d687e) )
	ROM_LOAD( "m08.bin",      0x3000, 0x1000, CRC(e3c542d6) SHA1(645cc4c94d1b1601c0083b156de67ec47fe2449f) )
	ROM_LOAD( "cc07j.bin",    0x4000, 0x1000, CRC(fbc9626c) SHA1(32be2d06321b2943718d0bec77ec9ebb806e4b93) )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "cc06",         0x0000, 0x0800, CRC(481b64cc) SHA1(3f35c545fc784ed4f969aba2d7be6e13a5ae32b7) )
	/* empty hole - Crazy Kong has an additional ROM here */
	ROM_LOAD( "m04.bin",      0x1000, 0x0800, CRC(6fb80538) SHA1(6ba5add5c0190e79191b3fa749a1b94e766e3950) )
	/* empty hole - Crazy Kong has an additional ROM here */
	ROM_LOAD( "m05.bin",      0x2000, 0x0800, CRC(056af36b) SHA1(756a295bbf7ede201b2e4cb106ce67a127e008de) )
	/* empty hole - Crazy Kong has an additional ROM here */
	ROM_LOAD( "m03.bin",      0x3000, 0x0800, CRC(67127253) SHA1(e27556ed74e73644a2578ce6645c312d64f484c6) )
	/* empty hole - Crazy Kong has an additional ROM here */

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "m02.bin",      0x0000, 0x0800, CRC(7f4877de) SHA1(c9aa9ff1b6cf907917fedfbd419b15ac337cf7bb) )
	ROM_LOAD( "m01.bin",      0x0800, 0x0800, CRC(49fab908) SHA1(9665d6e26f390afcbf0ed9fe8fea9be94fbb3a84) )

	ROM_REGION( 0x0160, REGION_PROMS, 0 )
	ROM_LOAD( "cclimber.pr1", 0x0000, 0x0020, CRC(751c3325) SHA1(edce2bc883996c1d72dc6c1c9f62799b162d415a) )
	ROM_LOAD( "cclimber.pr2", 0x0020, 0x0020, CRC(ab1940fa) SHA1(8d98e05cbaa6f55770c12e0a9a8ed9c73cc54423) )
	ROM_LOAD( "cclimber.pr3", 0x0040, 0x0020, CRC(71317756) SHA1(1195f0a037e379cc1a3c0314cb746f5cd2bffe50) )
	ROM_LOAD( "ccboot.prm",   0x0060, 0x0100, CRC(9e11550d) SHA1(b8cba8e16e10e23fba1f11551102ab77b680bdf0) )	/* decryption table (not used) */

	ROM_REGION( 0x2000, REGION_SOUND1, 0 )	/* samples */
	ROM_LOAD( "cc13j.bin",    0x0000, 0x1000, CRC(5f0bcdfb) SHA1(7f79bf6de117348f606696ed7ea1937bbf926612) )
	ROM_LOAD( "cc12j.bin",    0x1000, 0x1000, CRC(9003ffbd) SHA1(fd016056aabc23957643f37230f03842294f795e) )
ROM_END

ROM_START( ccboot2 )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 )	/* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "11.4k",        0x0000, 0x1000, CRC(b2b17e24) SHA1(1242d64242b3a6fe099457d155ebc508e5482818) )
	ROM_LOAD( "10.4j",        0x1000, 0x1000, CRC(8382bc0f) SHA1(2390ee2ec08a074c7bc4b9c7750b979a1d3a8a67) )
	ROM_LOAD( "cc09j.bin",    0x2000, 0x1000, CRC(26489069) SHA1(9be4d4a22dd334e619416e6c846a05003c0d687e) )
	ROM_LOAD( "m08.bin",      0x3000, 0x1000, CRC(e3c542d6) SHA1(645cc4c94d1b1601c0083b156de67ec47fe2449f) )
	ROM_LOAD( "cc07j.bin",    0x4000, 0x1000, CRC(fbc9626c) SHA1(32be2d06321b2943718d0bec77ec9ebb806e4b93) )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "cc06",         0x0000, 0x0800, CRC(481b64cc) SHA1(3f35c545fc784ed4f969aba2d7be6e13a5ae32b7) )
	/* empty hole - Crazy Kong has an additional ROM here */
	ROM_LOAD( "cc04",         0x1000, 0x0800, CRC(332347cb) SHA1(4115ca32af73f1791635b7d9e093bf77088a8222) )
	/* empty hole - Crazy Kong has an additional ROM here */
	ROM_LOAD( "cc05",         0x2000, 0x0800, CRC(2c33b760) SHA1(2edea8fe13376fbd51a5586d97aba3b30d78e94b) )
	/* empty hole - Crazy Kong has an additional ROM here */
	ROM_LOAD( "cc03",         0x3000, 0x0800, CRC(4e4b3658) SHA1(0d39a8cb5cd6cf06008be60707f9b277a8a32a2d) )
	/* empty hole - Crazy Kong has an additional ROM here */

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "cc02",         0x0000, 0x0800, CRC(14f3ecc9) SHA1(a1b5121abfbe8f07580eb3fa6384352d239a3d75) )
	ROM_LOAD( "cc01",         0x0800, 0x0800, CRC(21c0f9fb) SHA1(44fad56d302a439257216ddac9fd62b3666589f1) )

	ROM_REGION( 0x0160, REGION_PROMS, 0 )
	ROM_LOAD( "cclimber.pr1", 0x0000, 0x0020, CRC(751c3325) SHA1(edce2bc883996c1d72dc6c1c9f62799b162d415a) )
	ROM_LOAD( "cclimber.pr2", 0x0020, 0x0020, CRC(ab1940fa) SHA1(8d98e05cbaa6f55770c12e0a9a8ed9c73cc54423) )
	ROM_LOAD( "cclimber.pr3", 0x0040, 0x0020, CRC(71317756) SHA1(1195f0a037e379cc1a3c0314cb746f5cd2bffe50) )
	ROM_LOAD( "ccboot.prm",   0x0060, 0x0100, CRC(9e11550d) SHA1(b8cba8e16e10e23fba1f11551102ab77b680bdf0) )	/* decryption table (not used) */

	ROM_REGION( 0x2000, REGION_SOUND1, 0 )	/* samples */
	ROM_LOAD( "cc13j.bin",    0x0000, 0x1000, CRC(5f0bcdfb) SHA1(7f79bf6de117348f606696ed7ea1937bbf926612) )
	ROM_LOAD( "cc12j.bin",    0x1000, 0x1000, CRC(9003ffbd) SHA1(fd016056aabc23957643f37230f03842294f795e) )
ROM_END

ROM_START( ckong )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "d05-07.bin",   0x0000, 0x1000, CRC(b27df032) SHA1(57f9be139c610405e3c2fddd7093dfb1277e450e) )
	ROM_LOAD( "f05-08.bin",   0x1000, 0x1000, CRC(5dc1aaba) SHA1(42b9e5946ffce7c156d114bde68f37c2c34853c4) )
	ROM_LOAD( "h05-09.bin",   0x2000, 0x1000, CRC(c9054c94) SHA1(1aa08d2501ee620759fd5c111e12f6d432c25294) )
	ROM_LOAD( "k05-10.bin",   0x3000, 0x1000, CRC(069c4797) SHA1(03be185e6914ec7f3770ce3da4eb49cdb97adc85) )
	ROM_LOAD( "l05-11.bin",   0x4000, 0x1000, CRC(ae159192) SHA1(d467256a3a366e246243e7828ff4a45d4c146e2c) )
	ROM_LOAD( "n05-12.bin",   0x5000, 0x1000, CRC(966bc9ab) SHA1(4434fc620169ffea1b1f227b61674e1daf79b54b) )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "n11-06.bin",   0x0000, 0x1000, CRC(2dcedd12) SHA1(dfdcfc21bcba7c8e148ee54daae511ca78c58e70) )
	ROM_LOAD( "k11-04.bin",   0x1000, 0x1000, CRC(3375b3bd) SHA1(a00b3c31cff123aab6ac0833aabfdd663302971a) )
	ROM_LOAD( "l11-05.bin",   0x2000, 0x1000, CRC(fa7cbd91) SHA1(0208d2ebc59f3600005476b6987472685bc99d67) )
	ROM_LOAD( "h11-03.bin",   0x3000, 0x1000, CRC(5655cc11) SHA1(5195e9b2a60c54280b48b32ee8248090904dbc51) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "c11-02.bin",   0x0000, 0x0800, CRC(d1352c31) SHA1(da726a63a8be830d695afeddc1717749af8c9d47) )
	ROM_LOAD( "a11-01.bin",   0x0800, 0x0800, CRC(a7a2fdbd) SHA1(529865f8bbfbdbbf34ac39c70ef17e6d5bd0f845) )

	ROM_REGION( 0x0060, REGION_PROMS, 0 )
	ROM_LOAD( "prom.v6",      0x0000, 0x0020, CRC(b3fc1505) SHA1(5b94adde0428a26b815c7eb9b3f3716470d349c7) )
	ROM_LOAD( "prom.u6",      0x0020, 0x0020, CRC(26aada9e) SHA1(f59645e606ea4f0dd0fc4ea47dd03f526c534941) )
	ROM_LOAD( "prom.t6",      0x0040, 0x0020, CRC(676b3166) SHA1(29b9434cd34d43ea5664e436e2a24b54f8d88aac) )

	ROM_REGION( 0x2000, REGION_SOUND1, 0 )	/* samples */
	ROM_LOAD( "cc13j.bin",    0x0000, 0x1000, CRC(5f0bcdfb) SHA1(7f79bf6de117348f606696ed7ea1937bbf926612) )
	ROM_LOAD( "cc12j.bin",    0x1000, 0x1000, CRC(9003ffbd) SHA1(fd016056aabc23957643f37230f03842294f795e) )
ROM_END

ROM_START( ckonga )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "d05-07.bin",   0x0000, 0x1000, CRC(b27df032) SHA1(57f9be139c610405e3c2fddd7093dfb1277e450e) )
	ROM_LOAD( "f05-08.bin",   0x1000, 0x1000, CRC(5dc1aaba) SHA1(42b9e5946ffce7c156d114bde68f37c2c34853c4) )
	ROM_LOAD( "h05-09.bin",   0x2000, 0x1000, CRC(c9054c94) SHA1(1aa08d2501ee620759fd5c111e12f6d432c25294) )
	ROM_LOAD( "10.dat",       0x3000, 0x1000, CRC(c3beb501) SHA1(14f49c45fc7c91799034c5a51fca310f0a66b1d7) )
	ROM_LOAD( "l05-11.bin",   0x4000, 0x1000, CRC(ae159192) SHA1(d467256a3a366e246243e7828ff4a45d4c146e2c) )
	ROM_LOAD( "n05-12.bin",   0x5000, 0x1000, CRC(966bc9ab) SHA1(4434fc620169ffea1b1f227b61674e1daf79b54b) )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "n11-06.bin",   0x0000, 0x1000, CRC(2dcedd12) SHA1(dfdcfc21bcba7c8e148ee54daae511ca78c58e70) )
	ROM_LOAD( "k11-04.bin",   0x1000, 0x1000, CRC(3375b3bd) SHA1(a00b3c31cff123aab6ac0833aabfdd663302971a) )
	ROM_LOAD( "l11-05.bin",   0x2000, 0x1000, CRC(fa7cbd91) SHA1(0208d2ebc59f3600005476b6987472685bc99d67) )
	ROM_LOAD( "h11-03.bin",   0x3000, 0x1000, CRC(5655cc11) SHA1(5195e9b2a60c54280b48b32ee8248090904dbc51) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "c11-02.bin",   0x0000, 0x0800, CRC(d1352c31) SHA1(da726a63a8be830d695afeddc1717749af8c9d47) )
	ROM_LOAD( "a11-01.bin",   0x0800, 0x0800, CRC(a7a2fdbd) SHA1(529865f8bbfbdbbf34ac39c70ef17e6d5bd0f845) )

	ROM_REGION( 0x0060, REGION_PROMS, 0 )
	ROM_LOAD( "prom.v6",      0x0000, 0x0020, CRC(b3fc1505) SHA1(5b94adde0428a26b815c7eb9b3f3716470d349c7) )
	ROM_LOAD( "prom.u6",      0x0020, 0x0020, CRC(26aada9e) SHA1(f59645e606ea4f0dd0fc4ea47dd03f526c534941) )
	ROM_LOAD( "prom.t6",      0x0040, 0x0020, CRC(676b3166) SHA1(29b9434cd34d43ea5664e436e2a24b54f8d88aac) )

	ROM_REGION( 0x2000, REGION_SOUND1, 0 )	/* samples */
	ROM_LOAD( "cc13j.bin",    0x0000, 0x1000, CRC(5f0bcdfb) SHA1(7f79bf6de117348f606696ed7ea1937bbf926612) )
	ROM_LOAD( "cc12j.bin",    0x1000, 0x1000, CRC(9003ffbd) SHA1(fd016056aabc23957643f37230f03842294f795e) )
ROM_END

ROM_START( ckongjeu )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "d05-07.bin",   0x0000, 0x1000, CRC(b27df032) SHA1(57f9be139c610405e3c2fddd7093dfb1277e450e) )
	ROM_LOAD( "f05-08.bin",   0x1000, 0x1000, CRC(5dc1aaba) SHA1(42b9e5946ffce7c156d114bde68f37c2c34853c4) )
	ROM_LOAD( "h05-09.bin",   0x2000, 0x1000, CRC(c9054c94) SHA1(1aa08d2501ee620759fd5c111e12f6d432c25294) )
	ROM_LOAD( "ckjeu10.dat",  0x3000, 0x1000, CRC(7e6eeec4) SHA1(98b283ea22bedc46710a24e65cfae48b87a57605) )
	ROM_LOAD( "l05-11.bin",   0x4000, 0x1000, CRC(ae159192) SHA1(d467256a3a366e246243e7828ff4a45d4c146e2c) )
	ROM_LOAD( "ckjeu12.dat",  0x5000, 0x1000, CRC(0532f270) SHA1(a73680bd7939097bd821fb6834e8763cf1572b55) )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "n11-06.bin",   0x0000, 0x1000, CRC(2dcedd12) SHA1(dfdcfc21bcba7c8e148ee54daae511ca78c58e70) )
	ROM_LOAD( "k11-04.bin",   0x1000, 0x1000, CRC(3375b3bd) SHA1(a00b3c31cff123aab6ac0833aabfdd663302971a) )
	ROM_LOAD( "l11-05.bin",   0x2000, 0x1000, CRC(fa7cbd91) SHA1(0208d2ebc59f3600005476b6987472685bc99d67) )
	ROM_LOAD( "h11-03.bin",   0x3000, 0x1000, CRC(5655cc11) SHA1(5195e9b2a60c54280b48b32ee8248090904dbc51) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "c11-02.bin",   0x0000, 0x0800, CRC(d1352c31) SHA1(da726a63a8be830d695afeddc1717749af8c9d47) )
	ROM_LOAD( "a11-01.bin",   0x0800, 0x0800, CRC(a7a2fdbd) SHA1(529865f8bbfbdbbf34ac39c70ef17e6d5bd0f845) )

	ROM_REGION( 0x0060, REGION_PROMS, 0 )
	ROM_LOAD( "prom.v6",      0x0000, 0x0020, CRC(b3fc1505) SHA1(5b94adde0428a26b815c7eb9b3f3716470d349c7) )
	ROM_LOAD( "prom.u6",      0x0020, 0x0020, CRC(26aada9e) SHA1(f59645e606ea4f0dd0fc4ea47dd03f526c534941) )
	ROM_LOAD( "prom.t6",      0x0040, 0x0020, CRC(676b3166) SHA1(29b9434cd34d43ea5664e436e2a24b54f8d88aac) )

	ROM_REGION( 0x2000, REGION_SOUND1, 0 )	/* samples */
	ROM_LOAD( "cc13j.bin",    0x0000, 0x1000, CRC(5f0bcdfb) SHA1(7f79bf6de117348f606696ed7ea1937bbf926612) )
	ROM_LOAD( "cc12j.bin",    0x1000, 0x1000, CRC(9003ffbd) SHA1(fd016056aabc23957643f37230f03842294f795e) )
ROM_END

ROM_START( ckongo )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "o55a-1",       0x0000, 0x1000, CRC(8bfb4623) SHA1(1b8e12d1f337756bbfa9c3d736db7513d571c1b3) )
	ROM_LOAD( "o55a-2",       0x1000, 0x1000, CRC(9ae8089b) SHA1(e50864bb77dce24ba6d10c4fc16ccaa593962442) )
	ROM_LOAD( "o55a-3",       0x2000, 0x1000, CRC(e82b33c8) SHA1(27befba696cd1a9453fb49e8e4ddd46eab41b30d) )
	ROM_LOAD( "o55a-4",       0x3000, 0x1000, CRC(f038f941) SHA1(02be92ef3bf8d36c9916b40109c738965a652a76) )
	ROM_LOAD( "o55a-5",       0x4000, 0x1000, CRC(5182db06) SHA1(f3e981dc3744aff7756f8e0bfd4d92583a02417d) )
	/* no ROM at 5000 */

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
	/* same as ckong but with halves switched */
	ROM_LOAD( "o50b-1",       0x0000, 0x0800, CRC(cae9e2bf) SHA1(bc62d98840b8b5b296de0f1379baabb1b4d25df0) )
	ROM_CONTINUE(             0x2000, 0x0800 )
	ROM_LOAD( "o50b-2",       0x0800, 0x0800, CRC(fba82114) SHA1(36b7c124edf73b01681f5d63867fafa38a31abbf) )
	ROM_CONTINUE(             0x2800, 0x0800 )
	ROM_LOAD( "o50b-3",       0x1000, 0x0800, CRC(1714764b) SHA1(b025fcc03d45b1ec29be7e292622745544ba891d) )
	ROM_CONTINUE(             0x3000, 0x0800 )
	ROM_LOAD( "o50b-4",       0x1800, 0x0800, CRC(b7008b57) SHA1(9328ff79947dbebdc3e2dd8bcc362667b8201476) )
	ROM_CONTINUE(             0x3800, 0x0800 )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "c11-02.bin",   0x0000, 0x0800, CRC(d1352c31) SHA1(da726a63a8be830d695afeddc1717749af8c9d47) )
	ROM_LOAD( "a11-01.bin",   0x0800, 0x0800, CRC(a7a2fdbd) SHA1(529865f8bbfbdbbf34ac39c70ef17e6d5bd0f845) )

	ROM_REGION( 0x0060, REGION_PROMS, 0 )
	ROM_LOAD( "prom.v6",      0x0000, 0x0020, CRC(b3fc1505) SHA1(5b94adde0428a26b815c7eb9b3f3716470d349c7) )
	ROM_LOAD( "prom.u6",      0x0020, 0x0020, CRC(26aada9e) SHA1(f59645e606ea4f0dd0fc4ea47dd03f526c534941) )
	ROM_LOAD( "prom.t6",      0x0040, 0x0020, CRC(676b3166) SHA1(29b9434cd34d43ea5664e436e2a24b54f8d88aac) )

	ROM_REGION( 0x2000, REGION_SOUND1, 0 )	/* samples */
	ROM_LOAD( "cc13j.bin",    0x0000, 0x1000, CRC(5f0bcdfb) SHA1(7f79bf6de117348f606696ed7ea1937bbf926612) )
	ROM_LOAD( "cc12j.bin",    0x1000, 0x1000, CRC(9003ffbd) SHA1(fd016056aabc23957643f37230f03842294f795e) )
ROM_END

ROM_START( ckongalc )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "ck7.bin",      0x0000, 0x1000, CRC(2171cac3) SHA1(7b18bfe44c32fb64b675bbbe2136344522c79b09) )
	ROM_LOAD( "ck8.bin",      0x1000, 0x1000, CRC(88b83ff7) SHA1(4afc494cc264aaa4614da6aed02ce062d9c20850) )
	ROM_LOAD( "ck9.bin",      0x2000, 0x1000, CRC(cff2af47) SHA1(1757428cefad13855a623162101ec01c04006c94) )
	ROM_LOAD( "ck10.bin",     0x3000, 0x1000, CRC(520fa4de) SHA1(6edbaf727756cd33bde94492d72654aa12dbd7e1) )
	ROM_LOAD( "ck11.bin",     0x4000, 0x1000, CRC(327dcadf) SHA1(17b2d3b9e2a82b5278a01cc972cb49705d113127) )
	/* no ROM at 5000 */

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ck6.bin",      0x0000, 0x1000, CRC(a8916dc8) SHA1(472520aae3837e6026f2a7577d3b2aff371a316c) )
	ROM_LOAD( "ck4.bin",      0x1000, 0x1000, CRC(b62a0367) SHA1(8c285cbc714d7e6589bd63b3cef7c841ed1c2a4e) )
	ROM_LOAD( "ck5.bin",      0x2000, 0x1000, CRC(cd3b5dde) SHA1(2319a2be04d70989b01f4fc703756ba6e1c1f388) )
	ROM_LOAD( "ck3.bin",      0x3000, 0x1000, CRC(61122c5e) SHA1(978b6dbec35f3adc651fddf332db17625099a92e) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ck2.bin",      0x0000, 0x0800, CRC(f67c80f1) SHA1(d1fbcce1b6242f810e106ff50812636e3168ebc1) )
	ROM_LOAD( "ck1.bin",      0x0800, 0x0800, CRC(80eb517d) SHA1(fef4111f656c58b28e7eac5aa5b5cc7e07ccb2fd) )

	ROM_REGION( 0x0060, REGION_PROMS, 0 )
	ROM_LOAD( "ck6v.bin",     0x0000, 0x0020, CRC(751c3325) SHA1(edce2bc883996c1d72dc6c1c9f62799b162d415a) )
	ROM_LOAD( "ck6u.bin",     0x0020, 0x0020, CRC(ab1940fa) SHA1(8d98e05cbaa6f55770c12e0a9a8ed9c73cc54423) )
	ROM_LOAD( "ck6t.bin",     0x0040, 0x0020, CRC(b4e827a5) SHA1(31a5a5ad54417a474d22bb16c473415d99a2b6f1) )

	ROM_REGION( 0x2000, REGION_SOUND1, 0 )	/* samples */
	ROM_LOAD( "cc13j.bin",    0x0000, 0x1000, CRC(5f0bcdfb) SHA1(7f79bf6de117348f606696ed7ea1937bbf926612) )
	ROM_LOAD( "ck12.bin",     0x1000, 0x1000, CRC(2eb23b60) SHA1(c9e7dc584562aceb374193655fbacb7df6c9c731) )
ROM_END

ROM_START( monkeyd )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "ck7.bin",      0x0000, 0x1000, CRC(2171cac3) SHA1(7b18bfe44c32fb64b675bbbe2136344522c79b09) )
	ROM_LOAD( "ck8.bin",      0x1000, 0x1000, CRC(88b83ff7) SHA1(4afc494cc264aaa4614da6aed02ce062d9c20850) )
	ROM_LOAD( "ck9.bin",      0x2000, 0x1000, CRC(cff2af47) SHA1(1757428cefad13855a623162101ec01c04006c94) )
	ROM_LOAD( "ck10.bin",     0x3000, 0x1000, CRC(520fa4de) SHA1(6edbaf727756cd33bde94492d72654aa12dbd7e1) )
	ROM_LOAD( "md5l.bin",     0x4000, 0x1000, CRC(d1db1bb0) SHA1(fe7d700c7f9eca9c389be3717ebebf3e7dc63aa2) )
	/* no ROM at 5000 */

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ck6.bin",      0x0000, 0x1000, CRC(a8916dc8) SHA1(472520aae3837e6026f2a7577d3b2aff371a316c) )
	ROM_LOAD( "ck4.bin",      0x1000, 0x1000, CRC(b62a0367) SHA1(8c285cbc714d7e6589bd63b3cef7c841ed1c2a4e) )
	ROM_LOAD( "ck5.bin",      0x2000, 0x1000, CRC(cd3b5dde) SHA1(2319a2be04d70989b01f4fc703756ba6e1c1f388) )
	ROM_LOAD( "ck3.bin",      0x3000, 0x1000, CRC(61122c5e) SHA1(978b6dbec35f3adc651fddf332db17625099a92e) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ck2.bin",      0x0000, 0x0800, CRC(f67c80f1) SHA1(d1fbcce1b6242f810e106ff50812636e3168ebc1) )
	ROM_LOAD( "ck1.bin",      0x0800, 0x0800, CRC(80eb517d) SHA1(fef4111f656c58b28e7eac5aa5b5cc7e07ccb2fd) )

	ROM_REGION( 0x0060, REGION_PROMS, 0 )
	ROM_LOAD( "ck6v.bin",     0x0000, 0x0020, BAD_DUMP CRC(751c3325) SHA1(edce2bc883996c1d72dc6c1c9f62799b162d415a)  )
	ROM_LOAD( "ck6u.bin",     0x0020, 0x0020, BAD_DUMP CRC(ab1940fa) SHA1(8d98e05cbaa6f55770c12e0a9a8ed9c73cc54423)  )
	ROM_LOAD( "ck6t.bin",     0x0040, 0x0020, BAD_DUMP CRC(b4e827a5) SHA1(31a5a5ad54417a474d22bb16c473415d99a2b6f1)  )

	ROM_REGION( 0x2000, REGION_SOUND1, 0 )	/* samples */
	ROM_LOAD( "cc13j.bin",    0x0000, 0x1000, CRC(5f0bcdfb) SHA1(7f79bf6de117348f606696ed7ea1937bbf926612) )
	ROM_LOAD( "ck12.bin",     0x1000, 0x1000, CRC(2eb23b60) SHA1(c9e7dc584562aceb374193655fbacb7df6c9c731) )
ROM_END

ROM_START( rpatrolb )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "rp1.4l",       0x0000, 0x1000, CRC(bfd7ae7a) SHA1(a06d1cc2674ed40d0bfa67dd6d724964c1e40600) )
	ROM_LOAD( "rp2.4j",       0x1000, 0x1000, CRC(03f53340) SHA1(35336945f4b634fc4c7791ac9c9e6643c8cd8006) )
	ROM_LOAD( "rp3.4f",       0x2000, 0x1000, CRC(8fa300df) SHA1(5c3ba1ef6c1ce8df437b4fa464293208630b5e8d) )
	ROM_LOAD( "rp4.4e",       0x3000, 0x1000, CRC(74a8f1f4) SHA1(6bbc4944e4b31425a6b82f370b6760e5a4b36f56) )
	ROM_LOAD( "rp5.4c",       0x4000, 0x1000, CRC(d7ef6c87) SHA1(38e3b44b355907824919acc4f5064dcb98ebb1d0) )
	/* no ROM at 5000 */

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rp6.6n",       0x0000, 0x0800, CRC(19f18e9e) SHA1(a5500ac36bcda772f3ba79d9e9d37b1eec7bfd13) )
	/* 0800-0fff empty */
	ROM_LOAD( "rp8.6k",       0x1000, 0x0800, CRC(008738c7) SHA1(a66d9daf31b0d9cf087b591c74f0aaee3d7607b5) )
	/* 1800-1fff empty */
	ROM_LOAD( "rp7.6l",       0x2000, 0x0800, CRC(07f2070d) SHA1(39df286fda9e48eba6e770fe23a603b5e10d88b6) )
	/* 2800-2fff empty */
	ROM_LOAD( "rp9.6h",       0x3000, 0x0800, CRC(ea5aafca) SHA1(d8f8fe270680ae261d63bd4702107961cd904699) )
	/* 3800-3fff empty */

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "rp10.6a",      0x0000, 0x0800, CRC(59747c31) SHA1(92acf07489f3e17f0c1769a0df15b6ddb117830f) )
	ROM_LOAD( "rp11.6c",      0x0800, 0x0800, CRC(065651a5) SHA1(5c2f9b44d8819d2f792525c06b5c341fe07329c0) )

	ROM_REGION( 0x0060, REGION_PROMS, 0 )
	ROM_LOAD( "bprom1.9n",    0x0000, 0x0020, CRC(f9a2383b) SHA1(4d88c177740efdb27708474c9ee0fcdca5a78c36) )
	ROM_LOAD( "bprom2.9p",    0x0020, 0x0020, CRC(1743bd26) SHA1(9bb50f6e24a7ac3c9ddf3923e57c5532603009e5) )
	ROM_LOAD( "bprom3.9c",    0x0040, 0x0020, CRC(ee03bc96) SHA1(45e33e750a536a904f30136d84dd7993d97e8e54) )

	/* no samples */
ROM_END

ROM_START( silvland )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "7.2r",         0x0000, 0x1000, CRC(57e6be62) SHA1(c1d47970f8209256c9ccd6512b921dec6c276998) )
	ROM_LOAD( "8.1n",         0x1000, 0x1000, CRC(bbb2b287) SHA1(93cd4ebe238c189c80be8b8ab1ec2649256dd6ea) )
	ROM_LOAD( "rp3.4f",       0x2000, 0x1000, CRC(8fa300df) SHA1(5c3ba1ef6c1ce8df437b4fa464293208630b5e8d) )
	ROM_LOAD( "10.2n",        0x3000, 0x1000, CRC(5536a65d) SHA1(0bf2b9ea76fd6fd8c0475bf6f49a42f1c96d3906) )
	ROM_LOAD( "11.1r",        0x4000, 0x1000, CRC(6f23f66f) SHA1(3ca8075c28956ec473ccb0e9f05e9ad8669f743d) )
	ROM_LOAD( "12.2k",        0x5000, 0x1000, CRC(26f1537c) SHA1(0468352d49edec3a52e32612856735b78e11079b) )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "6.6n",         0x0000, 0x0800, CRC(affb804f) SHA1(9fc77804690e91773787e06f3329accef075f9f3) )
	/* 0800-0fff empty */
	ROM_LOAD( "4.6k",         0x1000, 0x0800, CRC(e487579d) SHA1(aed59f15dbc904d73e19d914ccd0a86fda859085) )
	/* 1800-1fff empty */
	ROM_LOAD( "5.6l",         0x2000, 0x0800, CRC(ad4642e5) SHA1(f4de2d9ed0e69c002be07f47247e95167a3ffffb) )
	/* 2800-2fff empty */
	ROM_LOAD( "3.6h",         0x3000, 0x0800, CRC(59125a1a) SHA1(37638fb690d6b4f11585f6a13586271c2f0e3743) )
	/* 3800-3fff empty */

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "2.6c",         0x0000, 0x0800, CRC(c8d32b8e) SHA1(7d655d243ed13cf2537f3fdfde5bf34229f7cb84) )
	ROM_LOAD( "1.6a",         0x0800, 0x0800, CRC(ee333daf) SHA1(b02998dccec9a4f841838874221caabae8380fcc) )

	ROM_REGION( 0x0060, REGION_PROMS, 0 )
	ROM_LOAD( "mb7051.1v",    0x0000, 0x0020, CRC(1d2343b1) SHA1(294f22178af4532abf767c1ffe2dc831bbe683bf) )
	ROM_LOAD( "mb7051.1u",    0x0020, 0x0020, CRC(c174753c) SHA1(303bfb1f470b525ccaeafa81a38a4bc3a7de5dbb) )
	ROM_LOAD( "mb7051.1t",    0x0040, 0x0020, CRC(04a1be01) SHA1(9c270c04d374d46752ec99bd4e79fed1e2896bc0) )

	/* no samples */
ROM_END


ROM_START( cannonb )
	ROM_REGION( 0x11000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "canballs.5d", 0x10000, 0x1000, CRC(43ad0d16) SHA1(682f1ee15e41bb5a161287536bb97704c0d3be9c) ) /* only this one ROM is encrypted */
	ROM_LOAD( "canballs.5f",  0x1000, 0x1000, CRC(3e0dacdd) SHA1(cdd3684a6962f2fb582b8a415383c06a5e5059dd) )
	ROM_LOAD( "canballs.5h",  0x2000, 0x1000, CRC(e18a836b) SHA1(19b90a55db82914c5db18486e05d9f59aba1b442) )
	ROM_LOAD( "canballs.5k",  0x3000, 0x0800, CRC(6ed3cbf4) SHA1(070ba61dc97df6be8004f7e052a4cef836234888) )
	ROM_LOAD( "canballs.5m",  0x4000, 0x1000, CRC(4f0088ab) SHA1(a8009f5b8517ba4d84fbc483b199f2514f24eae8) )
	ROM_LOAD( "canballs.5n",  0x5000, 0x1000, CRC(91570033) SHA1(7cd7fe9541da36c3919324bc65e6db1d1ca635e0) )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "canballs.11n", 0x0000, 0x1000, CRC(a95b8e03) SHA1(e78125023e1af6de292292b875b45401b2173ca9) )
	ROM_LOAD( "canballs.11k", 0x1000, 0x1000, CRC(dbbe8263) SHA1(efe4bba25a03261bc8309e6d83d5600def875b0c) )
	ROM_LOAD( "canballs.11l", 0x2000, 0x1000, CRC(060b044c) SHA1(3121f07adb661663a2303085eea1b662968f8f98) )
	ROM_LOAD( "canballs.11h", 0x3000, 0x1000, CRC(8043bc1a) SHA1(bd2f3dfe26cf8d987d9ecaa41eac4bdc4e16a692) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "canballs.11c", 0x0000, 0x0800, CRC(d1352c31) SHA1(da726a63a8be830d695afeddc1717749af8c9d47) )
	ROM_LOAD( "canballs.11a", 0x0800, 0x0800, CRC(a7a2fdbd) SHA1(529865f8bbfbdbbf34ac39c70ef17e6d5bd0f845) )

	ROM_REGION( 0x0060, REGION_PROMS, 0 )
	ROM_LOAD( "prom.v6",      0x0000, 0x0020, CRC(b3fc1505) SHA1(5b94adde0428a26b815c7eb9b3f3716470d349c7) )
	ROM_LOAD( "prom.u6",      0x0020, 0x0020, CRC(26aada9e) SHA1(f59645e606ea4f0dd0fc4ea47dd03f526c534941) )
	ROM_LOAD( "prom.t6",      0x0040, 0x0020, CRC(676b3166) SHA1(29b9434cd34d43ea5664e436e2a24b54f8d88aac) )

	ROM_REGION( 0x2000, REGION_SOUND1, 0 )	/* samples */
	ROM_LOAD( "canballs.5s",  0x0000, 0x1000, CRC(5f0bcdfb) SHA1(7f79bf6de117348f606696ed7ea1937bbf926612) )
	ROM_LOAD( "canballs.5p",  0x1000, 0x1000, CRC(9003ffbd) SHA1(fd016056aabc23957643f37230f03842294f795e) )
ROM_END

static DRIVER_INIT( cannonb )
{
	int A;
	unsigned char *rom = memory_region(REGION_CPU1);


	for (A = 0x0000;A < 0x1000;A++) /* only first ROM is encrypted */
	{
		unsigned char src;
		int i;
		unsigned char xor_tab[4] ={0x92, 0x82, 0x12, 0x10};

		src = rom[A+0x10000];

		i = ((A&0x200)>>8) | ((A&0x80)>>7);

		src ^= xor_tab[i];

		rom[A] = src;
	}
}



/***************************************************************************

  Swimmer driver

***************************************************************************/

WRITE_HANDLER( swimmer_bgcolor_w );
WRITE_HANDLER( swimmer_palettebank_w );
PALETTE_INIT( swimmer );
VIDEO_UPDATE( swimmer );
WRITE_HANDLER( swimmer_sidepanel_enable_w );



WRITE_HANDLER( swimmer_sh_soundlatch_w )
{
	soundlatch_w(offset,data);
	cpu_set_irq_line_and_vector(1,0,HOLD_LINE,0xff);
}



static MEMORY_READ_START( swimmer_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0x9000, 0x93ff, MRA_RAM },
	{ 0x9400, 0x97ff, videoram_r }, /* mirror address (used by Swimmer) */
	{ 0x9c00, 0x9fff, MRA_RAM },
	{ 0xa000, 0xa000, input_port_0_r },
	{ 0xa800, 0xa800, input_port_1_r },
	{ 0xb000, 0xb000, input_port_2_r },
	{ 0xb800, 0xb800, input_port_3_r },
	{ 0xb880, 0xb880, input_port_4_r },
	{ 0xc000, 0xc7ff, MRA_RAM },    /* ??? used by Guzzler */
	{ 0xe000, 0xffff, MRA_ROM },    /* Guzzler only */
MEMORY_END

static MEMORY_WRITE_START( swimmer_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x8800, 0x88ff, cclimber_bigsprite_videoram_w, &cclimber_bsvideoram, &cclimber_bsvideoram_size },
	{ 0x8900, 0x89ff, cclimber_bigsprite_videoram_w },      /* mirror for the above (Guzzler writes to both) */
	{ 0x9000, 0x93ff, videoram_w, &videoram, &videoram_size },
	{ 0x9400, 0x97ff, videoram_w }, /* mirror address (used by Guzzler) */
	{ 0x9800, 0x981f, MWA_RAM, &cclimber_column_scroll },
	{ 0x9880, 0x989f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x98fc, 0x98ff, MWA_RAM, &cclimber_bigspriteram },
	{ 0x9c00, 0x9fff, cclimber_colorram_w, &colorram },
	{ 0xa000, 0xa000, interrupt_enable_w },
	{ 0xa001, 0xa001, flip_screen_x_w },
	{ 0xa002, 0xa002, flip_screen_y_w },
	{ 0xa003, 0xa003, swimmer_sidepanel_enable_w },
	{ 0xa004, 0xa004, swimmer_palettebank_w },
	{ 0xa800, 0xa800, swimmer_sh_soundlatch_w },
	{ 0xb800, 0xb800, swimmer_bgcolor_w },  /* river color in Swimmer */
	{ 0xc000, 0xc7ff, MWA_RAM },    /* ??? used by Guzzler */
	{ 0xe000, 0xffff, MWA_ROM },    /* Guzzler only */
MEMORY_END


static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x0fff, MRA_ROM },
	{ 0x2000, 0x23ff, MRA_RAM },
	{ 0x3000, 0x3000, soundlatch_r },
	{ 0x4000, 0x4001, MRA_RAM },    /* ??? */
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x0fff, MWA_ROM },
	{ 0x2000, 0x23ff, MWA_RAM },
	{ 0x4000, 0x4000, MWA_RAM },    /* ??? */
MEMORY_END

static PORT_WRITE_START( sound_writeport )
	{ 0x00, 0x00, AY8910_write_port_0_w },
	{ 0x01, 0x01, AY8910_control_port_0_w },
	{ 0x80, 0x80, AY8910_write_port_1_w },
	{ 0x81, 0x81, AY8910_control_port_1_w },
PORT_END



INPUT_PORTS_START( swimmer )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0xe0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0xe0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_BITX( 0,       0x03, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "10000" )
	PORT_DIPSETTING(    0x04, "20000" )
	PORT_DIPSETTING(    0x08, "30000" )
	PORT_DIPSETTING(    0x0c, "None" )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_6C ) )

	PORT_START      /* IN3/DSW2 */
	PORT_BIT( 0x03, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Easy" )		/* labeled this way for similarities with 'swimmerb'*/
	PORT_DIPSETTING(    0x40, "Hard" )		/* labeled this way for similarities with 'swimmerb'*/
	PORT_DIPSETTING(    0x80, "Harder" )
	PORT_DIPSETTING(    0xc0, "Hardest" )

	PORT_START      /* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0xfc, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

/* Same as 'swimmer' but different "Difficulty" Dip Switch */
INPUT_PORTS_START( swimmerb )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0xe0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0xe0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_BITX( 0,       0x03, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "10000" )
	PORT_DIPSETTING(    0x04, "20000" )
	PORT_DIPSETTING(    0x08, "30000" )
	PORT_DIPSETTING(    0x0c, "None" )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_6C ) )

	PORT_START      /* IN3/DSW2 */
	PORT_BIT( 0x03, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START      /* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0xfc, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( guzzler )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0xe0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0xe0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_BITX( 0,       0x03, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x04, "20K, every 50K" )
	PORT_DIPSETTING(    0x00, "30K, every 100K" )
	PORT_DIPSETTING(    0x08, "30K only" )
	PORT_DIPSETTING(    0x0c, "None" )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_6C ) )

	PORT_START      /* DSW1 */
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x20, 0x00, "High Score Names" )
	PORT_DIPSETTING(    0x20, "3 Letters" )
	PORT_DIPSETTING(    0x00, "10 Letters" )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x40, "Medium" )
	PORT_DIPSETTING(    0x80, "Hard" )
	PORT_DIPSETTING(    0xc0, "Hardest" )

	PORT_START      /* coin */
	PORT_BIT_IMPULSE( 0x01, IP_ACTIVE_HIGH, IPT_COIN1, 2)
	PORT_BIT_IMPULSE( 0x02, IP_ACTIVE_HIGH, IPT_COIN2, 2)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END



static struct GfxLayout swimmer_charlayout =
{
	8,8,    /* 8*8 characters */
	512,    /* 512 characters */
	3,      /* 3 bits per pixel */
	{ 0, 512*8*8, 512*2*8*8 },      /* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },	     /* characters are upside down */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8     /* every char takes 8 consecutive bytes */
};

static struct GfxLayout swimmer_spritelayout =
{
	16,16,  /* 16*16 sprites */
	128,    /* 128 sprites */
	3,	      /* 3 bits per pixel */
	{ 0, 128*16*16, 128*2*16*16 },  /* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,       /* pretty straightforward layout */
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8    /* every sprite takes 32 consecutive bytes */
};

static struct GfxDecodeInfo swimmer_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &swimmer_charlayout,      0, 64 }, /* characters */
	{ REGION_GFX1, 0, &swimmer_spritelayout,    0, 32 }, /* sprite set #1 */
	{ REGION_GFX2, 0, &swimmer_charlayout,   64*8, 4 },  /* big sprite set */
	{ -1 } /* end of array */
};




static MACHINE_DRIVER_START( swimmer )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 3072000)	/* 3.072 MHz */
	MDRV_CPU_MEMORY(swimmer_readmem,swimmer_writemem)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,1)

	MDRV_CPU_ADD(Z80,4000000/2)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 2 MHz */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PORTS(0,sound_writeport)
	MDRV_CPU_PERIODIC_INT(nmi_line_pulse,4000000/16384) /* IRQs are triggered by the main CPU */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(swimmer_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256+32+2)
	MDRV_COLORTABLE_LENGTH(64*8+4*8)

	MDRV_PALETTE_INIT(swimmer)
	MDRV_VIDEO_START(generic)
	MDRV_VIDEO_UPDATE(swimmer)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, swimmer_ay8910_interface)
MACHINE_DRIVER_END



ROM_START( swimmer )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "sw1",          0x0000, 0x1000, CRC(f12481e7) SHA1(4e8ee509043fd57ec1579594f0b2c543f270bead) )
	ROM_LOAD( "sw2",          0x1000, 0x1000, CRC(a0b6fdd2) SHA1(7d3603de6c282224869824c7572868fc85599ea2) )
	ROM_LOAD( "sw3",          0x2000, 0x1000, CRC(ec93d7de) SHA1(e225c6b98eb3c32825c1cc1fcf69dec7e340460c) )
	ROM_LOAD( "sw4",          0x3000, 0x1000, CRC(0107927d) SHA1(419aeca37c7604f71f49e3dee36f477eee0ba53a) )
	ROM_LOAD( "sw5",          0x4000, 0x1000, CRC(ebd8a92c) SHA1(65401f8d39250f6ec61841e58ce4c21ddfe99842) )
	ROM_LOAD( "sw6",          0x5000, 0x1000, CRC(f8539821) SHA1(82f43ecbbb0a3771632eb26e10bc5453d74b65b1) )
	ROM_LOAD( "sw7",          0x6000, 0x1000, CRC(37efb64e) SHA1(0ed4d678895c17b37df605990acd096c538e3675) )
	ROM_LOAD( "sw8",          0x7000, 0x1000, CRC(33d6001e) SHA1(749b746d018e74e364fd6974e4522c8a18915774) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for sound board */
	ROM_LOAD( "sw12.4k",      0x0000, 0x1000, CRC(2eee9bcb) SHA1(ceafdf750a8af0c1c9abbbf437c3e9d9ae09f72b) )

	ROM_REGION( 0x3000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "sw15.18k",     0x0000, 0x1000, CRC(4f3608cb) SHA1(cebcad69c4ad5dacc0bf597fdaed6f8950ffdfe1) )  /* chars */
	ROM_LOAD( "sw14.18l",     0x1000, 0x1000, CRC(7181c8b4) SHA1(b22fa0ebac884002cf6f5651e4366f30d0ab09f5) )
	ROM_LOAD( "sw13.18m",     0x2000, 0x1000, CRC(2eb1af5c) SHA1(0105d03adfc5ce9ca478e678a1e1d8bae7c516e0) )

	ROM_REGION( 0x3000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "sw23.6c",      0x0000, 0x0800, CRC(9ca67e24) SHA1(86f561abc1a1c6b0c29c6017246d805c5a48b999) )  /* bigsprite data */
	ROM_RELOAD(               0x0800, 0x0800 )	/* Guzzler has larger ROMs */
	ROM_LOAD( "sw22.5c",      0x1000, 0x0800, CRC(02c10992) SHA1(8c383fdcd83aa9997e5802a58419b9d993a9b38d) )
	ROM_RELOAD(               0x1800, 0x0800 )	/* Guzzler has larger ROMs */
	ROM_LOAD( "sw21.4c",      0x2000, 0x0800, CRC(7f4993c1) SHA1(a5884b3af707109e810cf1f38bee3cb642e619f6) )
	ROM_RELOAD(               0x2800, 0x0800 )	/* Guzzler has larger ROMs */

	ROM_REGION( 0x0220, REGION_PROMS, 0 )
	ROM_LOAD( "24s10.13b",    0x0000, 0x100, CRC(8e35b97d) SHA1(2e2c254574660e01b9983f795a2adb5b9911d7f0) )
	ROM_LOAD( "24s10.13a",    0x0100, 0x100, CRC(c5f24909) SHA1(27f2c967d440f6387841aa3f7b116c64bb812af1) )
	ROM_LOAD( "18s030.12c",   0x0200, 0x020, CRC(3b2deb3a) SHA1(bb7b5c662454f5b355cc59cbdf8879e4664bed1d) )
ROM_END

ROM_START( swimmera )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "swa1",         0x0000, 0x1000, CRC(42c2b6c5) SHA1(13688e1ee08308b13ead5af7b4f65043dae4e40f) )
	ROM_LOAD( "swa2",         0x1000, 0x1000, CRC(49bac195) SHA1(a5d2cc2cdd10003f69014c4799f5f59e47a44260) )
	ROM_LOAD( "swa3",         0x2000, 0x1000, CRC(a6d8cb01) SHA1(80ab0ffaee6e0edf19b767229865722c2af6112c) )
	ROM_LOAD( "swa4",         0x3000, 0x1000, CRC(7be75182) SHA1(4fe7bc6382ea7311be1225fb0715aa2ff4ec084c) )
	ROM_LOAD( "swa5",         0x4000, 0x1000, CRC(78f79573) SHA1(6124fae47b3fa2e5965dffdfe9cbeb96acf08314) )
	ROM_LOAD( "swa6",         0x5000, 0x1000, CRC(fda9b311) SHA1(d9c914ad27f5988d0d4da5c942fb12bb5728cdfb) )
	ROM_LOAD( "swa7",         0x6000, 0x1000, CRC(7090e5ee) SHA1(d1e0ca38c3d1e4a7b7efa3696e47fb36ad3f8aa0) )
	ROM_LOAD( "swa8",         0x7000, 0x1000, CRC(ab86efa9) SHA1(5b5a80ae285c7e9f4c51e646116edf789d4dba39) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for sound board */
	ROM_LOAD( "sw12.4k",      0x0000, 0x1000, CRC(2eee9bcb) SHA1(ceafdf750a8af0c1c9abbbf437c3e9d9ae09f72b) )

	ROM_REGION( 0x3000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "sw15.18k",     0x0000, 0x1000, CRC(4f3608cb) SHA1(cebcad69c4ad5dacc0bf597fdaed6f8950ffdfe1) )  /* chars */
	ROM_LOAD( "sw14.18l",     0x1000, 0x1000, CRC(7181c8b4) SHA1(b22fa0ebac884002cf6f5651e4366f30d0ab09f5) )
	ROM_LOAD( "sw13.18m",     0x2000, 0x1000, CRC(2eb1af5c) SHA1(0105d03adfc5ce9ca478e678a1e1d8bae7c516e0) )

	ROM_REGION( 0x3000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "sw23.6c",      0x0000, 0x0800, CRC(9ca67e24) SHA1(86f561abc1a1c6b0c29c6017246d805c5a48b999) )  /* bigsprite data */
	ROM_RELOAD(               0x0800, 0x0800 )	/* Guzzler has larger ROMs */
	ROM_LOAD( "sw22.5c",      0x1000, 0x0800, CRC(02c10992) SHA1(8c383fdcd83aa9997e5802a58419b9d993a9b38d) )
	ROM_RELOAD(               0x1800, 0x0800 )	/* Guzzler has larger ROMs */
	ROM_LOAD( "sw21.4c",      0x2000, 0x0800, CRC(7f4993c1) SHA1(a5884b3af707109e810cf1f38bee3cb642e619f6) )
	ROM_RELOAD(               0x2800, 0x0800 )	/* Guzzler has larger ROMs */

	ROM_REGION( 0x0220, REGION_PROMS, 0 )
	ROM_LOAD( "24s10.13b",    0x0000, 0x100, CRC(8e35b97d) SHA1(2e2c254574660e01b9983f795a2adb5b9911d7f0) )
	ROM_LOAD( "24s10.13a",    0x0100, 0x100, CRC(c5f24909) SHA1(27f2c967d440f6387841aa3f7b116c64bb812af1) )
	ROM_LOAD( "18s030.12c",   0x0200, 0x020, CRC(3b2deb3a) SHA1(bb7b5c662454f5b355cc59cbdf8879e4664bed1d) )
ROM_END

ROM_START( swimmerb )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "sw1.9l",       0x0000, 0x1000, CRC(b045be08) SHA1(52187e1daebec521a98157f22960637393e40e62) )
	ROM_LOAD( "sw2.9k",       0x1000, 0x1000, CRC(163d65e5) SHA1(b505f05af96f241285f0f7082ed03fa07bbde7de) )
	ROM_LOAD( "sw3.9j",       0x2000, 0x1000, CRC(631d74e9) SHA1(b6adba9445264de80f5daf33dad1c90b23617648) )
	ROM_LOAD( "sw4.9f",       0x3000, 0x1000, CRC(d62634db) SHA1(c6d0d2cf7a3a19fac1752a30189f31eb3df8fa42) )
	ROM_LOAD( "sw5.9e",       0x4000, 0x1000, CRC(922d5d87) SHA1(e5f111d82a072e59c00b759eaada195f1fc06532) )
	ROM_LOAD( "sw6.9d",       0x5000, 0x1000, CRC(85478209) SHA1(df3c79ca25229fef2fe0f48d3c173e389628a68d) )
	ROM_LOAD( "sw7.9c",       0x6000, 0x1000, CRC(88266f2e) SHA1(4ad15f9ba7b45a6c1c3637f8d0fd8be9c04b495f) )
	ROM_LOAD( "sw8.9a",       0x7000, 0x1000, CRC(191a16e4) SHA1(75d3f49e2f4ea04d3a7cc88662c023768bf48365) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for sound board */
	ROM_LOAD( "sw12.4k",      0x0000, 0x1000, CRC(2eee9bcb) SHA1(ceafdf750a8af0c1c9abbbf437c3e9d9ae09f72b) )

	ROM_REGION( 0x3000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "sw15.18k",     0x0000, 0x1000, CRC(4f3608cb) SHA1(cebcad69c4ad5dacc0bf597fdaed6f8950ffdfe1) )  /* chars */
	ROM_LOAD( "sw14.18l",     0x1000, 0x1000, CRC(7181c8b4) SHA1(b22fa0ebac884002cf6f5651e4366f30d0ab09f5) )
	ROM_LOAD( "sw13.18m",     0x2000, 0x1000, CRC(2eb1af5c) SHA1(0105d03adfc5ce9ca478e678a1e1d8bae7c516e0) )

	ROM_REGION( 0x3000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "sw23.6c",      0x0000, 0x0800, CRC(9ca67e24) SHA1(86f561abc1a1c6b0c29c6017246d805c5a48b999) )  /* bigsprite data */
	ROM_RELOAD(               0x0800, 0x0800 )	/* Guzzler has larger ROMs */
	ROM_LOAD( "sw22.5c",      0x1000, 0x0800, CRC(02c10992) SHA1(8c383fdcd83aa9997e5802a58419b9d993a9b38d) )
	ROM_RELOAD(               0x1800, 0x0800 )	/* Guzzler has larger ROMs */
	ROM_LOAD( "sw21.4c",      0x2000, 0x0800, CRC(7f4993c1) SHA1(a5884b3af707109e810cf1f38bee3cb642e619f6) )
	ROM_RELOAD(               0x2800, 0x0800 )	/* Guzzler has larger ROMs */

	ROM_REGION( 0x0220, REGION_PROMS, 0 )
	ROM_LOAD( "24s10.13b",    0x0000, 0x100, CRC(8e35b97d) SHA1(2e2c254574660e01b9983f795a2adb5b9911d7f0) )
	ROM_LOAD( "24s10.13a",    0x0100, 0x100, CRC(c5f24909) SHA1(27f2c967d440f6387841aa3f7b116c64bb812af1) )
	ROM_LOAD( "18s030.12c",   0x0200, 0x020, CRC(3b2deb3a) SHA1(bb7b5c662454f5b355cc59cbdf8879e4664bed1d) )
ROM_END

ROM_START( guzzler )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "guzz-01.bin",  0x0000, 0x2000, CRC(58aaa1e9) SHA1(4ea9c85670a0d71483ac79564093043762a24b2c) )
	ROM_LOAD( "guzz-02.bin",  0x2000, 0x2000, CRC(f80ceb17) SHA1(eedff7355fb5aa18b82f0a3e39bba5521c359791) )
	ROM_LOAD( "guzz-03.bin",  0x4000, 0x2000, CRC(e63c65a2) SHA1(e2b888911330690faa3a041e1a17d838b46e6bbd) )
	ROM_LOAD( "guzz-04.bin",  0x6000, 0x2000, CRC(45be42f5) SHA1(578943afdb6ceca34ca7c19c2fd1164ca3aa57bd) )
	ROM_LOAD( "guzz-16.bin",  0xe000, 0x2000, CRC(61ee00b7) SHA1(ea8516c8dfb2de32a8034f94c7d0c086e3596740) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for sound board */
	ROM_LOAD( "guzz-12.bin",  0x0000, 0x1000, CRC(f3754d9e) SHA1(bb30832aba4e82ab0ecce40fc1223d9771ff7dd2) )

	ROM_REGION( 0x3000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "guzz-13.bin",  0x0000, 0x1000, CRC(afc464e2) SHA1(61730b5e5add24ba3d4e8903c5d71cf4df9b77e0) )   /* chars */
	ROM_LOAD( "guzz-14.bin",  0x1000, 0x1000, CRC(acbdfe1f) SHA1(ab7abe4bb321fc7dc4e73acab4b1a7133e6bcf20) )
	ROM_LOAD( "guzz-15.bin",  0x2000, 0x1000, CRC(66978c05) SHA1(2c8d5545f8b1d3cd7cd63448f8064fd3712d6fee) )

	ROM_REGION( 0x3000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "guzz-11.bin",  0x0000, 0x1000, CRC(ec2e9d86) SHA1(2fc631229e78db68777e74a03f98f660f324a885) )   /* big sprite */
	ROM_LOAD( "guzz-10.bin",  0x1000, 0x1000, CRC(bd3f0bf7) SHA1(c57aff05812801c22104a4afc8a8a6bca33dda96) )
	ROM_LOAD( "guzz-09.bin",  0x2000, 0x1000, CRC(18927579) SHA1(414676193ef1f6ce79a4cba73e4d017312f766f4) )

	ROM_REGION( 0x0220, REGION_PROMS, 0 )
	ROM_LOAD( "guzzler.003",  0x0000, 0x100, CRC(f86930c1) SHA1(58efc8cbef05e1612d12e2f0babddf15571d42bb) )
	ROM_LOAD( "guzzler.002",  0x0100, 0x100, CRC(b566ea9e) SHA1(345078af6a339fbe6cd966046acd9d04c8926b5c) )
	ROM_LOAD( "guzzler.001",  0x0200, 0x020, CRC(69089495) SHA1(96b067b22be14536bac748f8d61e5587a8a04e92) )
ROM_END



void cclimber_decode(const unsigned char convtable[8][16])
{
	int A;
	unsigned char *rom = memory_region(REGION_CPU1);
	int diff = memory_region_length(REGION_CPU1) / 2;


	memory_set_opcode_base(0,rom+diff);

	for (A = 0x0000;A < 0x10000;A++)
	{
		int i,j;
		unsigned char src;


		src = rom[A];

		/* pick the translation table from bit 0 of the address */
		/* and from bits 1 7 of the source data */
		i = (A & 1) | (src & 0x02) | ((src & 0x80) >> 5);

		/* pick the offset in the table from bits 0 2 4 6 of the source data */
		j = (src & 0x01) | ((src & 0x04) >> 1) | ((src & 0x10) >> 2) | ((src & 0x40) >> 3);

		/* decode the opcodes */
		rom[A + diff] = (src & 0xaa) | convtable[i][j];
	}
}

static DRIVER_INIT( cclimber )
{
	static const unsigned char convtable[8][16] =
	{
		/* -1 marks spots which are unused and therefore unknown */
		{ 0x44,0x14,0x54,0x10,0x11,0x41,0x05,0x50,0x51,0x00,0x40,0x55,0x45,0x04,0x01,0x15 },
		{ 0x44,0x10,0x15,0x55,0x00,0x41,0x40,0x51,0x14,0x45,0x11,0x50,0x01,0x54,0x04,0x05 },
		{ 0x45,0x10,0x11,0x44,0x05,0x50,0x51,0x04,0x41,0x14,0x15,0x40,0x01,0x54,0x55,0x00 },
		{ 0x04,0x51,0x45,0x00,0x44,0x10,  -1,0x55,0x11,0x54,0x50,0x40,0x05,  -1,0x14,0x01 },
		{ 0x54,0x51,0x15,0x45,0x44,0x01,0x11,0x41,0x04,0x55,0x50,  -1,0x00,0x10,0x40,  -1 },
		{   -1,0x54,0x14,0x50,0x51,0x01,  -1,0x40,0x41,0x10,0x00,0x55,0x05,0x44,0x11,0x45 },
		{ 0x51,0x04,0x10,  -1,0x50,0x40,0x00,  -1,0x41,0x01,0x05,0x15,0x11,0x14,0x44,0x54 },
		{   -1,  -1,0x54,0x01,0x15,0x40,0x45,0x41,0x51,0x04,0x50,0x05,0x11,0x44,0x10,0x14 }
	};

	cclimber_decode(convtable);
}

DRIVER_INIT( cclimbrj )
{
	static const unsigned char convtable[8][16] =
	{
		{ 0x41,0x54,0x51,0x14,0x05,0x10,0x01,0x55,0x44,0x11,0x00,0x50,0x15,0x40,0x04,0x45 },
		{ 0x50,0x11,0x40,0x55,0x51,0x14,0x45,0x04,0x54,0x15,0x10,0x05,0x44,0x01,0x00,0x41 },
		{ 0x44,0x11,0x00,0x50,0x41,0x54,0x04,0x14,0x15,0x40,0x51,0x55,0x05,0x10,0x01,0x45 },
		{ 0x10,0x50,0x54,0x55,0x01,0x44,0x40,0x04,0x14,0x11,0x00,0x41,0x45,0x15,0x51,0x05 },
		{ 0x14,0x41,0x01,0x44,0x04,0x50,0x51,0x45,0x11,0x40,0x54,0x15,0x10,0x00,0x55,0x05 },
		{ 0x01,0x05,0x41,0x45,0x54,0x50,0x55,0x10,0x11,0x15,0x51,0x14,0x44,0x40,0x04,0x00 },
		{ 0x05,0x55,0x00,0x50,0x11,0x40,0x54,0x14,0x45,0x51,0x10,0x04,0x44,0x01,0x41,0x15 },
		{ 0x55,0x50,0x15,0x10,0x01,0x04,0x41,0x44,0x45,0x40,0x05,0x00,0x11,0x14,0x51,0x54 },
	};

	cclimber_decode(convtable);
}



GAME( 1980, cclimber, 0,        cclimber, cclimber, cclimber, ROT0,   "Nichibutsu", "Crazy Climber (US)" )
GAME( 1980, cclimbrj, cclimber, cclimber, cclimbrj, cclimbrj, ROT0,   "Nichibutsu", "Crazy Climber (Japan)" )
GAME( 1980, ccboot,   cclimber, cclimber, cclimber, cclimbrj, ROT0,   "bootleg", "Crazy Climber (bootleg set 1)" )
GAME( 1980, ccboot2,  cclimber, cclimber, cclimber, cclimbrj, ROT0,   "bootleg", "Crazy Climber (bootleg set 2)" )
GAME( 1981, ckong,    0,        cclimber, ckong,    0,        ROT270, "Falcon", "Crazy Kong (set 1)" )
GAME( 1981, ckonga,   ckong,    cclimber, ckong,    0,        ROT270, "Falcon", "Crazy Kong (set 2)" )
GAME( 1981, ckongjeu, ckong,    cclimber, ckong,    0,        ROT270, "bootleg", "Crazy Kong (Jeutel bootleg)" )
GAME( 1981, ckongo,   ckong,    cclimber, ckong,    0,        ROT270, "bootleg", "Crazy Kong (Orca bootleg)" )
GAME( 1981, ckongalc, ckong,    cclimber, ckong,    0,        ROT270, "bootleg", "Crazy Kong (Alca bootleg)" )
GAME( 1981, monkeyd,  ckong,    cclimber, ckong,    0,        ROT270, "bootleg", "Monkey Donkey" )
GAME( 1982?,rpatrolb, 0,        cclimber, rpatrolb, 0,        ROT0,   "bootleg", "River Patrol (bootleg)" )
GAME( 1982?,silvland, rpatrolb, cclimber, rpatrolb, 0,        ROT0,   "Falcon", "Silver Land" )
GAMEX( 1985, cannonb,  0,        cannonb,  cannonb,  cannonb,  ROT90,  "Soft", "Cannon Ball (Crazy Climber hardware)" , GAME_IMPERFECT_GRAPHICS )

GAME( 1982, swimmer,  0,        swimmer,  swimmer,  0,        ROT0,   "Tehkan", "Swimmer (set 1)" )
GAME( 1982, swimmera, swimmer,  swimmer,  swimmer,  0,        ROT0,   "Tehkan", "Swimmer (set 2)" )
GAME( 1982, swimmerb, swimmer,  swimmer,  swimmerb, 0,        ROT0,   "Tehkan", "Swimmer (set 3)" )
GAME( 1983, guzzler,  0,        swimmer,  guzzler,  0,        ROT90,  "Tehkan", "Guzzler" )
