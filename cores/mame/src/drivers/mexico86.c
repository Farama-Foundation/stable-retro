/***************************************************************************

Kick & Run - (c) 1987 Taito

Ernesto Corvi
ernesto@imagina.com

Notes:
- 4 players mode is not emulated. This involves some shared RAM and a subboard.
  There is additional code for a third Z80 in the bootleg version, I don't
  know if it's related or if its just a replacement for the 68705.
  

  
- Kiki Kaikai suffers from random lock-up's. It happens when the sound
  CPU misses CTS from YM2203. The processor will loop infinitely and the main
  CPU will in turn wait forever. It's difficult to meet the required level
  of synchronization. THis is kludged by filtering the 2203's busy signal.
  
- KiKi KaiKai uses a custom 68701 MCU which isn't dumped. The bootleg Knight Boy
  replaces it with a 68705. The bootleg is NOT 100% equivalent to the original
  (a situation similar to Bubble Bobble): collision detection is imperfect, the
  player can't be killed by some enemies.
  I think the bootleggers put the custom mcu in a test rig, examined its bus
  activity and replicated the behaviour inaccurately because they coudln't
  figure it all out. Indeed, the 68705 code reads all the memory locations
  related to the missing collision detection, but does nothing with them.
  
- In the KiKi KaiKai MCU simulation, I don't bother supporting the coinage dip
  switch settings. Therefore, it's hardwired to be 1 coin / 1 credit.
  
***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"

/* in machine/mexico86.c */
extern unsigned char *mexico86_protection_ram;
extern unsigned char *kicknrun_sharedram;
WRITE_HANDLER( mexico86_f008_w );
WRITE_HANDLER( kicknrun_f008_w );
INTERRUPT_GEN( kikikai_interrupt );
INTERRUPT_GEN( kicknrun_interrupt );
INTERRUPT_GEN( mexico86_m68705_interrupt );
READ_HANDLER( mexico86_68705_portA_r );
WRITE_HANDLER( mexico86_68705_portA_w );
WRITE_HANDLER( mexico86_68705_ddrA_w );
READ_HANDLER( mexico86_68705_portB_r );
WRITE_HANDLER( mexico86_68705_portB_w );
WRITE_HANDLER( mexico86_68705_ddrB_w );

/* in vidhrdw/mexico86.c */
extern unsigned char *mexico86_videoram,*mexico86_objectram;
extern size_t mexico86_objectram_size;
WRITE_HANDLER( mexico86_bankswitch_w );
VIDEO_UPDATE( mexico86 );
VIDEO_UPDATE( kikikai );

/* kicknrun mcu hookup similar to Bubble Bobble
   will be fine for Kiki Kai Kai also if/when the mcu
   is dumped
*/
static int ddr1, ddr2, ddr3, ddr4;
static int port1_in, port2_in, port3_in, port4_in;
static int port1_out, port2_out, port3_out, port4_out;

extern unsigned char *kicknrun_sharedram;
extern READ_HANDLER( kicknrun_sharedram_r );
extern WRITE_HANDLER( kicknrun_sharedram_w );


READ_HANDLER( kicknrun_mcu_ddr1_r )
{
	return ddr1;
}

WRITE_HANDLER( kicknrun_mcu_ddr1_w )
{
	ddr1 = data;
}

READ_HANDLER( kicknrun_mcu_ddr2_r )
{
	return ddr2;
}

WRITE_HANDLER( kicknrun_mcu_ddr2_w )
{
	ddr2 = data;
}

READ_HANDLER( kicknrun_mcu_ddr3_r )
{
	return ddr3;
}

WRITE_HANDLER( kicknrun_mcu_ddr3_w )
{
	ddr3 = data;
}

READ_HANDLER( kicknrun_mcu_ddr4_r )
{
	return ddr4;
}

WRITE_HANDLER( kicknrun_mcu_ddr4_w )
{
	ddr4 = data;
}

READ_HANDLER( kicknrun_mcu_port1_r )
{
	port1_in = readinputport(0);
	return (port1_out & ddr1) | (port1_in & ~ddr1);
}

WRITE_HANDLER( kicknrun_mcu_port1_w )
{
	/* bit 0, 1: coin counters (?) */
	if (data & 0x01 && ~port1_out & 0x01) {
		coin_counter_w(0,data & 0x01);
	}

	if (data & 0x02 && ~port1_out & 0x02) {
		coin_counter_w(1,data & 0x02);
	}
	
	/* bit 4, 5: coin lockouts */
	coin_lockout_w( 0, (~data & 0x10) );
	coin_lockout_w( 1, (~data & 0x10) );

	/* bit 7: ? (set briefly while MCU boots) */

	port1_out = data;
}

READ_HANDLER( kicknrun_mcu_port2_r )
{
	return (port2_out & ddr2) | (port2_in & ~ddr2);
}

WRITE_HANDLER( kicknrun_mcu_port2_w )
{
	/* bit 2: clock
	   latch on high->low transition
	*/   
	if ((port2_out & 0x04) && (~data & 0x04))
	{
		int address = port4_out;

		if (data & 0x10)
		{
			/* read */
			if (data & 0x01)
				port3_in = kicknrun_sharedram[address];
			else
				port3_in = readinputport((address & 1) + 1);
		}
		else
		{
			/* write */
			kicknrun_sharedram[address] = port3_out;
		}
	}

	port2_out = data;
}

READ_HANDLER( kicknrun_mcu_port3_r )
{
	return (port3_out & ddr3) | (port3_in & ~ddr3);
}

WRITE_HANDLER( kicknrun_mcu_port3_w )
{
	port3_out = data;
}

READ_HANDLER( kicknrun_mcu_port4_r )
{
	return (port4_out & ddr4) | (port4_in & ~ddr4);
}

WRITE_HANDLER( kicknrun_mcu_port4_w )
{
	port4_out = data;
}


/*AT*/
static READ_HANDLER( kiki_2203_r )
{
	return(YM2203Read(0,0) & 0x7f);
}
/*ZT*/

static unsigned char *shared;

static READ_HANDLER( shared_r )
{
	return shared[offset];
}

static WRITE_HANDLER( shared_w )
{
	shared[offset] = data;
}


static MEMORY_READ_START( readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },  /* banked roms */
	{ 0xc000, 0xe7ff, shared_r },   /* shared with sound cpu */
	{ 0xe800, 0xe8ff, MRA_RAM },    /* protection ram */
	{ 0xe900, 0xefff, MRA_RAM },
	{ 0xf010, 0xf010, input_port_5_r },
	{ 0xf800, 0xffff, MRA_RAM },    /* communication ram - to connect 4 players's subboard */
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xe7ff, shared_w, &shared },  /* shared with sound cpu */
	/*{ 0xc000, 0xcfff, MWA_RAM, &mexico86_videoram },*/
	{ 0xc000, 0xd4ff, MWA_RAM, &mexico86_videoram }, /*AT: corrected size*/
	{ 0xd500, 0xd7ff, MWA_RAM, &mexico86_objectram, &mexico86_objectram_size },
	{ 0xe800, 0xe8ff, MWA_RAM, &mexico86_protection_ram },  /* shared with mcu */
	{ 0xe900, 0xefff, MWA_RAM },
	{ 0xf000, 0xf000, mexico86_bankswitch_w },  /* program and gfx ROM banks */
	{ 0xf008, 0xf008, mexico86_f008_w },    /* cpu reset lines + other unknown stuff */
	{ 0xf018, 0xf018, MWA_NOP },    /* watchdog_reset_w }, */
	{ 0xf800, 0xffff, MWA_RAM },    /* communication ram */
MEMORY_END

static MEMORY_READ_START( kicknrun_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },  /* banked roms */
	{ 0xc000, 0xe7ff, shared_r },   /* shared with sound cpu */
	{ 0xe800, 0xe8ff, kicknrun_sharedram_r },    /* shared with mcu */
	{ 0xe900, 0xefff, MRA_RAM },
	{ 0xf010, 0xf010, input_port_5_r },
	{ 0xf800, 0xffff, MRA_RAM },    /* communication ram - to connect 4 players's subboard */
MEMORY_END

static MEMORY_WRITE_START( kicknrun_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xe7ff, shared_w, &shared },  /* shared with sound cpu */
	/* { 0xc000, 0xcfff, MWA_RAM, &mexico86_videoram }, */
	{ 0xc000, 0xd4ff, MWA_RAM, &mexico86_videoram }, /* AT: corrected size */
	{ 0xd500, 0xd7ff, MWA_RAM, &mexico86_objectram, &mexico86_objectram_size },
	{ 0xe800, 0xe8ff, kicknrun_sharedram_w, &kicknrun_sharedram },  /* shared with mcu */
	{ 0xe900, 0xefff, MWA_RAM },
	{ 0xf000, 0xf000, mexico86_bankswitch_w },  /* program and gfx ROM banks */
	{ 0xf008, 0xf008, kicknrun_f008_w },    /* cpu reset lines + other unknown stuff */
	{ 0xf018, 0xf018, MWA_NOP },    /* watchdog_reset_w }, */
	{ 0xf800, 0xffff, MWA_RAM },    /* communication ram */
MEMORY_END

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xa7ff, shared_r },
	{ 0xa800, 0xbfff, MRA_RAM },
	/*{ 0xc000, 0xc000, YM2203_status_port_0_r },*/
	{ 0xc000, 0xc000, kiki_2203_r }, /*AT*/
	{ 0xc001, 0xc001, YM2203_read_port_0_r },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0xa7ff, shared_w },
	{ 0xa800, 0xbfff, MWA_RAM },
	{ 0xc000, 0xc000, YM2203_control_port_0_w },
	{ 0xc001, 0xc001, YM2203_write_port_0_w },
MEMORY_END

static MEMORY_READ_START( m68705_readmem )
	{ 0x0000, 0x0000, mexico86_68705_portA_r },
	{ 0x0001, 0x0001, mexico86_68705_portB_r },
	{ 0x0002, 0x0002, input_port_0_r }, /* COIN */
	{ 0x0010, 0x007f, MRA_RAM },
	{ 0x0080, 0x07ff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( m68705_writemem )
	{ 0x0000, 0x0000, mexico86_68705_portA_w },
	{ 0x0001, 0x0001, mexico86_68705_portB_w },
	{ 0x0004, 0x0004, mexico86_68705_ddrA_w },
	{ 0x0005, 0x0005, mexico86_68705_ddrB_w },
	{ 0x000a, 0x000a, MWA_NOP },    /* looks like a bug in the code, writes to */
									/* 0x0a (=10dec) instead of 0x10 */
	{ 0x0010, 0x007f, MWA_RAM },
	{ 0x0080, 0x07ff, MWA_ROM },
MEMORY_END

static MEMORY_READ_START( m6801_readmem )
    { 0x0000, 0x0000, kicknrun_mcu_ddr1_r },
	{ 0x0001, 0x0001, kicknrun_mcu_ddr2_r },
	{ 0x0002, 0x0002, kicknrun_mcu_port1_r },
	{ 0x0003, 0x0003, kicknrun_mcu_port2_r },
	{ 0x0004, 0x0004, kicknrun_mcu_ddr3_r },
	{ 0x0005, 0x0005, kicknrun_mcu_ddr4_r },
	{ 0x0006, 0x0006, kicknrun_mcu_port3_r },
	{ 0x0007, 0x0007, kicknrun_mcu_port4_r },
	{ 0x0040, 0x00ff, MRA_RAM },
	{ 0xf000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( m6801_writemem )
    { 0x0000, 0x0000, kicknrun_mcu_ddr1_w },
	{ 0x0001, 0x0001, kicknrun_mcu_ddr2_w },
	{ 0x0002, 0x0002, kicknrun_mcu_port1_w },
	{ 0x0003, 0x0003, kicknrun_mcu_port2_w },
	{ 0x0004, 0x0004, kicknrun_mcu_ddr3_w },
	{ 0x0005, 0x0005, kicknrun_mcu_ddr4_w },
	{ 0x0006, 0x0006, kicknrun_mcu_port3_w },
	{ 0x0007, 0x0007, kicknrun_mcu_port4_w },
	{ 0x0040, 0x00ff, MWA_RAM },
	{ 0xf000, 0xffff, MWA_ROM },
MEMORY_END

INPUT_PORTS_START( kicknrun )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE )    /* service 2 */

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	/* When Bit 1 is On, the machine waits a signal from another one */
	/* Seems like if you can join two cabinets, one as master */
	/* and the other as slave, probably to play four players */
	PORT_DIPNAME( 0x01, 0x01, "System Selection" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) ) /* Screen ? */
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) ) /* Demo Sounds only play every 8th Demo */
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, "Easy" )
	PORT_DIPSETTING(    0x02, "Normal" )
	PORT_DIPSETTING(    0x01, "Medium" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x0c, 0x08, "Playing Time" )
	PORT_DIPSETTING(    0x00, "40 Seconds" )
	PORT_DIPSETTING(    0x0c, "One Minute" )
	PORT_DIPSETTING(    0x08, "One Minute and 20 Sec." )
	PORT_DIPSETTING(    0x04, "One Minute and 40 Sec." )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	/* The following dip seems to be related with the first one */
	PORT_DIPNAME( 0x20, 0x20, "System Selection" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Number of Matches" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x40, "6" )
	PORT_DIPNAME( 0x80, 0x80, "Players" )
	PORT_DIPSETTING(    0x80, "2" )
	PORT_DIPSETTING(    0x00, "4" )

	PORT_START
	/* the following is actually service coin 1 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_SERVICE, "Advance", KEYCODE_F1, IP_JOY_NONE )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( mexico86 )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE )    /* service 2 */

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	/* When Bit 1 is On, the machine waits a signal from another one */
	/* Seems like if you can join two cabinets, one as master */
	/* and the other as slave, probably to play four players */
	PORT_DIPNAME( 0x01, 0x01, "System Selection" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) ) /* Screen ? */
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) ) /* this should be Demo Sounds, but doesn't work? */
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, "Easy" )
	PORT_DIPSETTING(    0x02, "Normal" )
	PORT_DIPSETTING(    0x01, "Medium" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x0c, 0x08, "Playing Time" )
	PORT_DIPSETTING(    0x00, "40 Seconds" )
	PORT_DIPSETTING(    0x0c, "One Minute" )
	PORT_DIPSETTING(    0x08, "One Minute and 20 Sec." )
	PORT_DIPSETTING(    0x04, "One Minute and 40 Sec." )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	/* The following dip seems to be related with the first one */
	PORT_DIPNAME( 0x20, 0x20, "System Selection" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Number of Matches" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x40, "6" )
	PORT_DIPNAME( 0x80, 0x80, "Players" )
	PORT_DIPSETTING(    0x80, "2" )
	PORT_DIPSETTING(    0x00, "4" )

	PORT_START
	/* the following is actually service coin 1 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_SERVICE, "Advance", KEYCODE_F1, IP_JOY_NONE )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( kikikai )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

/*AT*/
	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

#if 0 /* old coinage settings*/
	PORT_DIPNAME( 0x30, 0x30, "Coin 1" )
	PORT_DIPSETTING(    0x30, "A:1C/1C B:1C/1C" )
	PORT_DIPSETTING(    0x20, "A:1C/2C B:2C/1C" )
	PORT_DIPSETTING(    0x10, "A:2C/1C B:3C/1C" )
	PORT_DIPSETTING(    0x00, "A:2C/3C B:4C/1C" )
	PORT_DIPNAME( 0xc0, 0xc0, "Coin 2" )
	PORT_DIPSETTING(    0xc0, "A:1C/1C B:1C/2C" )
	PORT_DIPSETTING(    0x80, "A:1C/2C B:1C/3C" )
	PORT_DIPSETTING(    0x40, "A:2C/1C B:1C/4C" )
	PORT_DIPSETTING(    0x00, "A:2C/3C B:1C/6C" )
#endif

	/* coinage copied from Japanese manual but type B doesn't work*/
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "50000 100000" )
	PORT_DIPSETTING(    0x0c, "70000 150000" )
	PORT_DIPSETTING(    0x08, "70000 200000" )
	PORT_DIPSETTING(    0x04, "100000 300000" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x40, "A" )
	PORT_DIPSETTING(    0x00, "B" )
	PORT_DIPNAME( 0x80, 0x00, "Number Match" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
/*ZT*/

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,
	4*2048,
	4,
	{ 0x20000*8, 0x20000*8+4, 0, 4 },
	{ 3, 2, 1, 0, 8+3, 8+2, 8+1, 8+0 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,   0, 16 },
	{ -1 } /* end of array */
};



static struct YM2203interface ym2203_interface =
{
	1,          /* 1 chip */
	3000000,    /* 3 MHz ??? */
	{ YM2203_VOL(40,40) },
	{ input_port_3_r },
	{ input_port_4_r },
	{ 0 },
	{ 0 }
};

static MACHINE_DRIVER_START( kicknrun )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 6000000)      /* 6 MHz??? */
	MDRV_CPU_MEMORY(kicknrun_readmem,kicknrun_writemem)
	MDRV_CPU_VBLANK_INT(kicknrun_interrupt, 1) /* IRQs triggered by the MCU */

	MDRV_CPU_ADD(Z80, 6000000)      /* 6 MHz??? */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(M6801,4000000/2)	/* xtal is 4MHz, I think it's divided by 2 internally */
	MDRV_CPU_MEMORY(m6801_readmem,m6801_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_pulse,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)  /* frames per second, vblank duration */
	MDRV_INTERLEAVE(100)    /* 100 CPU slices per frame - an high value to ensure proper */
							/* synchronization of the CPUs */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)

	MDRV_PALETTE_INIT(RRRR_GGGG_BBBB)
	MDRV_VIDEO_UPDATE(mexico86)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, ym2203_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( mexico86 )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 6000000)      /* 6 MHz??? */
	MDRV_CPU_MEMORY(readmem,writemem)

	MDRV_CPU_ADD(Z80, 6000000)      /* 6 MHz??? */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(M68705, 4000000/2) /* xtal is 4MHz (????) I think it's divided by 2 internally */
	MDRV_CPU_MEMORY(m68705_readmem,m68705_writemem)
	MDRV_CPU_VBLANK_INT(mexico86_m68705_interrupt,2)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)  /* frames per second, vblank duration */
	MDRV_INTERLEAVE(100)    /* 100 CPU slices per frame - an high value to ensure proper */
							/* synchronization of the CPUs */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)

	MDRV_PALETTE_INIT(RRRR_GGGG_BBBB)
	MDRV_VIDEO_UPDATE(mexico86)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, ym2203_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( kikikai )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 6000000)      /* 6 MHz??? */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(kikikai_interrupt, 1) /* IRQs should be triggered by the MCU, but we don't have it */

	MDRV_CPU_ADD(Z80, 6000000)      /* 6 MHz??? */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)  /* frames per second, vblank duration */
	MDRV_INTERLEAVE(100)    /* 100 CPU slices per frame - an high value to ensure proper */
							/* synchronization of the CPUs */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)

	MDRV_PALETTE_INIT(RRRR_GGGG_BBBB)
	MDRV_VIDEO_UPDATE(kikikai)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, ym2203_interface)
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( kikikai )
	ROM_REGION( 0x28000, REGION_CPU1, 0 )    /* 196k for code */
	ROM_LOAD( "a85-17.rom", 0x00000, 0x08000, CRC(c141d5ab) SHA1(fe3622ba283e514416c43a44f83f922a958b27cd) ) /* 1st half, main code        */
	ROM_CONTINUE(           0x20000, 0x08000 )             /* 2nd half, banked at 0x8000 */
	ROM_LOAD( "a85-16.rom", 0x10000, 0x10000, CRC(4094d750) SHA1(05e0ad177a3eb144b203784ecb6242a0fc5c4d4d) ) /* banked at 0x8000           */
	ROM_COPY(  REGION_CPU1, 0x10000, 0x08000, 0x04000 ) /*AT: set as default to avoid banking problems*/

	ROM_REGION( 0x10000, REGION_CPU2, 0 )    /* 64k for the audio cpu */
	ROM_LOAD( "a85-11.rom", 0x0000, 0x8000, CRC(cc3539db) SHA1(4239a40fdee65cba613e4b4ec54cf7899480e366) )

	ROM_REGION( 0x0800, REGION_CPU3, 0 )    /* 2k for the microcontroller */
	/* ROM_LOAD( "68701.bin",    0x0000, 0x0800, NO_DUMP ) */

	ROM_REGION( 0x40000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "a85-15.rom", 0x00000, 0x10000, CRC(aebc8c32) SHA1(77347cf5780f084a77123eb636cd0bad672a39e8) )
	ROM_LOAD( "a85-14.rom", 0x10000, 0x10000, CRC(a9df0453) SHA1(a5e9cd6266ab3ae46cd1b35a4603e13a2ca023fb) )
	ROM_LOAD( "a85-13.rom", 0x20000, 0x10000, CRC(3eeaf878) SHA1(f8ae8938a8358d1222e9fdf7bc0094ac13faf404) )
	ROM_LOAD( "a85-12.rom", 0x30000, 0x10000, CRC(91e58067) SHA1(c7eb9bf650039254fb7664758938b1012eacc597) )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "a85-08.rom", 0x0000, 0x0100, CRC(d15f61a8) SHA1(945c8aa26c85269c10373218bef13e04e25eb1e4) )
	ROM_LOAD( "a85-10.rom", 0x0100, 0x0100, CRC(8fc3fa86) SHA1(d4d86f8e147bbf2a370de428ac20a28b0f146782) )
	ROM_LOAD( "a85-09.rom", 0x0200, 0x0100, CRC(b931c94d) SHA1(fb554084f34c602d1ff7806fb945a06cf14332af) )
ROM_END

ROM_START( kicknrun )
	ROM_REGION( 0x28000, REGION_CPU1, 0 )    /* 196k for code */
	ROM_LOAD( "a87-08.bin", 0x00000, 0x08000, CRC(715e1b04) SHA1(60b7259758ec73f1cc945556e9c2b25766b745a8) ) /* 1st half, main code        */
	ROM_CONTINUE(           0x20000, 0x08000 )             /* 2nd half, banked at 0x8000 */
	ROM_LOAD( "a87-07.bin", 0x10000, 0x10000, CRC(6cb6ebfe) SHA1(fca61fc2ad8fadc1e15b9ff84c7469b68d16e885) ) /* banked at 0x8000           */
	ROM_COPY(  REGION_CPU1, 0x10000, 0x08000, 0x04000 ) /*AT: set as default to avoid banking problems*/

	ROM_REGION( 0x10000, REGION_CPU2, 0 )    /* 64k for the audio cpu */
	ROM_LOAD( "a87-06.bin", 0x0000, 0x8000, CRC(1625b587) SHA1(7336384e13c114915de5e439df5731ce3fc2054a) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )    /* 2k for the microcontroller */
	ROM_LOAD( "a87-01_jph1021p.h8", 0xf000, 0x1000, CRC(9451e880) SHA1(e9a505296108645f99449d391d0ebe9ac1b9984e) ) /* MCU labeled TAITO A78 01,  JPH1021P, 185, PS4 */	

	ROM_REGION( 0x40000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "a87-05.bin", 0x08000, 0x08000, CRC(4eee3a8a) SHA1(2f0e4c2fb6cba48d0e2b95927fc14f0038557371) )
	ROM_CONTINUE(           0x00000, 0x08000 )
	ROM_LOAD( "a87-04.bin", 0x10000, 0x08000, CRC(8b438d20) SHA1(12e615f34b7e732157f893b97c9b7e99e9ef7d62) )
	ROM_RELOAD(             0x18000, 0x08000 )
	ROM_LOAD( "a87-03.bin", 0x28000, 0x08000, CRC(f42e8a88) SHA1(db2702141981ba368bdc665443a8a0662266e6d9) )
	ROM_CONTINUE(           0x20000, 0x08000 )
	ROM_LOAD( "a87-02.bin", 0x30000, 0x08000, CRC(64f1a85f) SHA1(04fb9824450812b08f7e6fc57e0af828be9bd575) )
	ROM_RELOAD(             0x38000, 0x08000 )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "a87-10.bin", 0x0000, 0x0100, CRC(be6eb1f0) SHA1(f4d00e9b12bf116bf84edb2ff6caab158094b668) )
	ROM_LOAD( "a87-12.bin", 0x0100, 0x0100, CRC(3e953444) SHA1(e9c84ca9390fd7c73738a8b681a02e87fbd51bb4) )
	ROM_LOAD( "a87-11.bin", 0x0200, 0x0100, CRC(14f6c28d) SHA1(8c60974e4607906a3f77260bdd0704af60d596fc) )
ROM_END

ROM_START( mexico86 )
	ROM_REGION( 0x28000, REGION_CPU1, 0 )    /* 196k for code */
	ROM_LOAD( "2_g.bin",    0x00000, 0x08000, CRC(2bbfe0fb) SHA1(8f047e001ea8e49d28f73e546c82812af1c2533c) ) /* 1st half, main code        */
	ROM_CONTINUE(           0x20000, 0x08000 )             /* 2nd half, banked at 0x8000 */
	ROM_LOAD( "1_f.bin",    0x10000, 0x10000, CRC(0b93e68e) SHA1(c6fbcce83103e3e71a7a1ef9f18a10622ed6b951) ) /* banked at 0x8000           */
	ROM_COPY(  REGION_CPU1, 0x10000, 0x08000, 0x04000 ) /*AT: set as default to avoid banking problems*/

	ROM_REGION( 0x10000, REGION_CPU2, 0 )    /* 64k for the audio cpu */
	ROM_LOAD( "a87-06.bin", 0x0000, 0x8000, CRC(1625b587) SHA1(7336384e13c114915de5e439df5731ce3fc2054a) )

	ROM_REGION( 0x0800, REGION_CPU3, 0 )    /* 2k for the microcontroller */
	ROM_LOAD( "68_h.bin",   0x0000, 0x0800, CRC(ff92f816) SHA1(0015c3f2ed014052b3fa376409e3a7cca36fac72) )

	ROM_REGION( 0x40000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "4_d.bin",    0x08000, 0x08000, CRC(57cfdbca) SHA1(89c305c380c3de14a956ee4bc85d3a0d343b638e) )
	ROM_CONTINUE(           0x00000, 0x08000 )
	ROM_LOAD( "5_c.bin",    0x10000, 0x08000, CRC(e42fa143) SHA1(02d7e0e01af1cecc3952f6355987118098d346c3) )
	ROM_RELOAD(             0x18000, 0x08000 )
	ROM_LOAD( "6_b.bin",    0x28000, 0x08000, CRC(a4607989) SHA1(6832147603a146c34cc1809e839c8e034d0dacc5) )
	ROM_CONTINUE(           0x20000, 0x08000 )
	ROM_LOAD( "7_a.bin",    0x30000, 0x08000, CRC(245036b1) SHA1(108d9959de869b4fdf766abeade1486acec13bf2) )
	ROM_RELOAD(             0x38000, 0x08000 )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "a87-10.bin", 0x0000, 0x0100, CRC(be6eb1f0) SHA1(f4d00e9b12bf116bf84edb2ff6caab158094b668) )
	ROM_LOAD( "a87-12.bin", 0x0100, 0x0100, CRC(3e953444) SHA1(e9c84ca9390fd7c73738a8b681a02e87fbd51bb4) )
	ROM_LOAD( "a87-11.bin", 0x0200, 0x0100, CRC(14f6c28d) SHA1(8c60974e4607906a3f77260bdd0704af60d596fc) )
ROM_END


GAME( 1986, kikikai,  0,        kikikai,  kikikai,   0,  ROT90, "Taito Corporation", "KiKi KaiKai" )
GAME( 1986, kicknrun, 0,        kicknrun, kicknrun,  0,  ROT0,  "Taito Corporation", "Kick and Run" )
GAME( 1986, mexico86, kicknrun, mexico86, mexico86,  0,  ROT0,  "bootleg", "Mexico 86" )

