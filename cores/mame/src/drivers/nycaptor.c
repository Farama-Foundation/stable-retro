/***************************************************************************
    N.Y. Captor - Taito '85

     Driver by Tomasz Slanina  dox@space.pl
****************************************************************************
  Hardware similar to Fairyland Story
  Cycle Shooting (Taito '86) is running on (almost) the same hardware
  as NY Captor, but MCU dump is missing.

What's new :
 - added bootleg - Bronx
 - Cycle Shooting added (bigger VRAM than nycpator, dfferent (unknwn yet) banking and
   gfx control , ROMs probably are wrong mapped, gfx too)
 - Sub CPU halt (cpu #0 ,$d001)
 - Improved communication between main cpu and sound cpu ($d400 cpu#0 , $d000 sound cpu)


To do :
 - REAL bg/sprite priority system
 - cpu #0 , $d806 - only bits 0,1,2 used , something with sound control
                    (code at $62x)
 - better sound emulation (mixing ?)
 - unknown R/W
 - 13th bit in color defs (priority? bit is set for all colors in spot1,
                           for 3/4 screen cols in spot2 etc)
Notes :

 $d000 - MCU data read/write
 $d001 - Watchdog ?
 $d002 - generic control  ----x--- ROM Bank

 $d800 - Dip 0            ------xx Bonus life
                                00 150000 300000 300000
                                01 100000 300000 300000
                                10  20000  80000  80000
                                11  50000 150000 200000
                          -----x-- Unknown
                          ---xx--- Lives
                             00    2
                             01    1
                             10    3
                             11    5
                          --x----- Free Play
                            0      Yes
                            1      No
                          -x------ Allow Continue
                           0       No
                           1       Yes
                          x------- Demo Sounds
                          0        Off
                          1        On

 $d801 - Dip 1            Coinage

 $d802 - Dip 2            -------x Freeze
                                 0 On
                                 1 Off
                          ------x- Training Spot
                                0  No
                                1  Yes
                          ----xx-- Difficulty
                              00   Hardest
                              01   Normal
                              10   Hard
                              11   Easy
                          ---x---- Display Coinage
                             0     No
                             1     Yes
                          --x----- Reset Damage
                            0      Every 4 Stages
                            1      Every Stage
                          -x------ No Hit
                           0       Yes
                           1       No
                          x------- Coin Slots
                          0        1
                          1        2

 $d803 - Input Port 1     -------x Start1
                          ------x- Unused
                          -----x-- Service1
                          ----x--- Tilt
                          ---x---- Coin1
                          --x----- Coin2
                          xx------ Unknown

 $d804 - Input Port 2     -------x Button1
                          xxxxxxx- Unused

 $d805 - MCU Status read  ------x- MCU has sent data to the main cpu

 $d806 - Unknown

 $d807 - MCU Status read  -------x MCU is ready to receive data from main cpu


RAM :

 $e18e - sync between 1st and 2nd Z80
 $e18d - same as a bove

 $e19d - coinage A (Word)
 $e19f - coinage B (Word)

 $e298 - spot
 $e299 - lives
 $e2a1 - damage
 $e2a2 - stage

 $e39c - video control byte stored here
 $e39d - generic control value
 $e39f - gun position update
 $e397 - same as above ?

 $e3a0 - gun Y position ($18 - $f0 = visible area)
 $e3a1 - gun X position ($08 - $c0 = visible area)
 $e3a2 - shot trigger

 $e3a8 - timer for bullets reload
 $e3aa - bullets

 $df00 R - x - beam counter
 $df01 R - y - beam counter
 $df02 R - bit 0 - beam detector

 $df03 W - video control byte


Stephh's additional notes (based on the game Z80 code and some tests) :

  - You need to press SERVICE1 while resetting the game to enter the "test mode".
    Note that the grid is always displayed, so it's IMPOSSIBLE to see what's in it 8(

  - When "Infinite Bullets" is set to ON, there is no timer to reload the bullets.
  - When "No Hit" Dip Switch is ON, damage is still incremented, but there are
    no test made if it reaches the limit (0x0a).

  - Bit 2 of 0xd802 also determines the precision of a shot
    (= range around a bullet where the enemies can be killed) :
      * 0 : small range (0x0a)
      * 1 : high  range (0x0c)

***************************************************************************/

/*#define USE_MCU*/


#include <math.h>
#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"

VIDEO_START( nycaptor );
VIDEO_UPDATE( nycaptor );


extern UINT8 *nycaptor_scrlram;
UINT8 *nycaptor_sharedram;
static int generic_control_reg = 0;
static int sound_nmi_enable=0,pending_nmi=0;
int nyc_gametype=0;


WRITE_HANDLER( nycaptor_videoram_w );
WRITE_HANDLER( nycaptor_spriteram_w );
WRITE_HANDLER( nycaptor_palette_w );
WRITE_HANDLER( nycaptor_gfxctrl_w );
WRITE_HANDLER( nycaptor_scrlram_w );
WRITE_HANDLER( nycaptor_68705_portA_w );
WRITE_HANDLER( nycaptor_68705_portB_w );
WRITE_HANDLER( nycaptor_68705_portC_w );
WRITE_HANDLER( nycaptor_68705_ddrA_w );
WRITE_HANDLER( nycaptor_68705_ddrB_w );
WRITE_HANDLER( nycaptor_68705_ddrC_w );
WRITE_HANDLER( nycaptor_mcu_w );

READ_HANDLER( nycaptor_mcu_r );
READ_HANDLER( nycaptor_mcu_status_r1 );
READ_HANDLER( nycaptor_mcu_status_r2 );
READ_HANDLER( nycaptor_spriteram_r );
READ_HANDLER( nycaptor_palette_r );
READ_HANDLER( nycaptor_gfxctrl_r );
READ_HANDLER( nycaptor_scrlram_r );
READ_HANDLER( nycaptor_68705_portC_r );
READ_HANDLER( nycaptor_68705_portB_r );
READ_HANDLER( nycaptor_68705_portA_r );
READ_HANDLER( nycaptor_videoram_r );


static WRITE_HANDLER( sub_cpu_halt_w )
{
	cpu_set_halt_line(1, (data )? ASSERT_LINE : CLEAR_LINE);
}

static UINT8 snd_data;

READ_HANDLER( from_snd_r )
{
	return snd_data;
}

WRITE_HANDLER( to_main_w )
{
	snd_data = data;
}


READ_HANDLER(nycaptor_sharedram_r)
{
	return nycaptor_sharedram[offset];
}

WRITE_HANDLER(nycaptor_sharedram_w)
{
	nycaptor_sharedram[offset]=data;
}


static READ_HANDLER( nycaptor_b_r )
{
		return 1;
}

static READ_HANDLER( nycaptor_by_r )
{
	int port=readinputport(6);
	if(nyc_gametype==1)
			port=255-port;
		return port-8;
}

static READ_HANDLER( nycaptor_bx_r )
{
		return (readinputport(5)+0x27)|1;
}


static WRITE_HANDLER( sound_cpu_reset_w )
{
	cpu_set_reset_line(2, (data&1 )? ASSERT_LINE : CLEAR_LINE);
}

static int vol_ctrl[16];

static MACHINE_INIT( ta7630 )
{
	int i;

	double db			= 0.0;
	double db_step		= 0.50;	/* 0.50 dB step (at least, maybe more) */
	double db_step_inc	= 0.275;
	for (i=0; i<16; i++)
	{
		double max = 100.0 / pow(10.0, db/20.0 );
		vol_ctrl[ 15-i ] = max;
		/*logerror("vol_ctrl[%x] = %i (%f dB)\n",15-i,vol_ctrl[ 15-i ],db);*/
		db += db_step;
		db_step += db_step_inc;
	}
}

static void nmi_callback(int param)
{
	if (sound_nmi_enable) cpu_set_irq_line(2,IRQ_LINE_NMI,PULSE_LINE);
	else pending_nmi = 1;
}

static WRITE_HANDLER( sound_command_w )
{
	soundlatch_w(0,data);
	timer_set(TIME_NOW,data,nmi_callback);
}

static WRITE_HANDLER( nmi_disable_w )
{
	sound_nmi_enable = 0;
}

static WRITE_HANDLER( nmi_enable_w )
{
	sound_nmi_enable = 1;
	if (pending_nmi)
	{
		cpu_set_irq_line(2,IRQ_LINE_NMI,PULSE_LINE);
		pending_nmi = 0;
	}
}

static WRITE_HANDLER(unk_w)
{

}

static struct AY8910interface ay8910_interface =
{
	2,
	8000000/4,
	{ 15, 15 },
	{ 0,0 },
	{ 0,0 },
	{ unk_w,unk_w },
	{ unk_w,unk_w }
};

static struct MSM5232interface msm5232_interface =
{
	1, /* number of chips */
	2000000, /* 2 MHz ??? */
	{ { 0.65e-6, 0.65e-6, 0.65e-6, 0.65e-6, 0.65e-6, 0.65e-6, 0.65e-6, 0.65e-6 } },	/* 0.65 (???) uF capacitors (match the sample, not verified) */
	{ 100 }	/* mixing level ??? */
};


static READ_HANDLER ( nycaptor_generic_control_r )
{
	return generic_control_reg;
}

static WRITE_HANDLER( nycaptor_generic_control_w )
{
	generic_control_reg = data;
	cpu_setbank(1, memory_region(REGION_CPU1) + 0x10000 + ((data&0x08)>>3)*0x4000 );
}

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xc7ff, nycaptor_videoram_r },
	{ 0xd000, 0xd000, nycaptor_mcu_r },
	{ 0xd002, 0xd002, nycaptor_generic_control_r },
	{ 0xd400, 0xd400, from_snd_r },
	{ 0xd401, 0xd401, MRA_NOP },
	{ 0xd800, 0xd800, input_port_0_r },
	{ 0xd801, 0xd801, input_port_1_r },
	{ 0xd802, 0xd802, input_port_2_r },
	{ 0xd803, 0xd803, input_port_3_r },
	{ 0xd804, 0xd804, input_port_4_r },
	{ 0xd805, 0xd805, nycaptor_mcu_status_r1 },
	{ 0xd806, 0xd806, MRA_NOP }, /* unknown ?sound? */
	{ 0xd807, 0xd807, nycaptor_mcu_status_r2 },
	{ 0xdc00, 0xdc9f, nycaptor_spriteram_r},
	{ 0xdca0, 0xdcbf, nycaptor_scrlram_r },
	{ 0xdd00, 0xdeff, nycaptor_palette_r },
	{ 0xdf03, 0xdf03, nycaptor_gfxctrl_r },
	{ 0xe000, 0xffff, nycaptor_sharedram_r },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc7ff, nycaptor_videoram_w, &videoram, &videoram_size },
	{ 0xd000, 0xd000, nycaptor_mcu_w },
	{ 0xd001, 0xd001, sub_cpu_halt_w },
	{ 0xd002, 0xd002, nycaptor_generic_control_w },	/* bit 3 - memory bank at 0x8000-0xbfff */
	{ 0xd400, 0xd400, sound_command_w },
	{ 0xd403, 0xd403, sound_cpu_reset_w },
	{ 0xdc00, 0xdc9f, nycaptor_spriteram_w},
	{ 0xdca0, 0xdcbf, nycaptor_scrlram_w, &nycaptor_scrlram },
	{ 0xdce1, 0xdce1, MWA_NOP},
	{ 0xdd00, 0xdeff, nycaptor_palette_w },
	{ 0xdf03, 0xdf03, nycaptor_gfxctrl_w },
	{ 0xe000, 0xffff, nycaptor_sharedram_w,&nycaptor_sharedram },
MEMORY_END


static MEMORY_READ_START( readmem_sub )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xd800, 0xd800, input_port_0_r },
	{ 0xd801, 0xd801, input_port_1_r },
	{ 0xd802, 0xd802, input_port_2_r },
	{ 0xd803, 0xd803, input_port_3_r },
	{ 0xd804, 0xd804, input_port_4_r },
	{ 0xdc00, 0xdc9f, nycaptor_spriteram_r},
	{ 0xdd00, 0xdeff, nycaptor_palette_r },
	{ 0xdf00, 0xdf00, nycaptor_bx_r },
	{ 0xdf01, 0xdf01, nycaptor_by_r },
	{ 0xdf02, 0xdf02, nycaptor_b_r },
	{ 0xdf03, 0xdf03, nycaptor_gfxctrl_r },
	{ 0xe000, 0xffff, nycaptor_sharedram_r },
MEMORY_END


static MEMORY_WRITE_START( writemem_sub )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc7ff, nycaptor_videoram_w, &videoram, &videoram_size },
	{ 0xdc00, 0xdc9f, nycaptor_spriteram_w},
	{ 0xdca0, 0xdcbf, nycaptor_scrlram_w, &nycaptor_scrlram },
	{ 0xdd00, 0xdeff, nycaptor_palette_w },
	{ 0xdf03, 0xdf03, MWA_NOP },/* ? gfx control ? */
	{ 0xe000, 0xffff, nycaptor_sharedram_w },
MEMORY_END

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xd000, 0xd000, soundlatch_r },
	{ 0xd200, 0xd200, MRA_NOP },
	{ 0xe000, 0xefff, MRA_NOP },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xc800, 0xc800, AY8910_control_port_0_w },
	{ 0xc801, 0xc801, AY8910_write_port_0_w },
	{ 0xc802, 0xc802, AY8910_control_port_1_w },
	{ 0xc803, 0xc803, AY8910_write_port_1_w },
	{ 0xc900, 0xc90d, MSM5232_0_w },
	{ 0xca00, 0xca00, MWA_NOP},
	{ 0xcb00, 0xcb00, MWA_NOP},
	{ 0xcc00, 0xcc00, MWA_NOP},
	{ 0xd000, 0xd000, to_main_w },
	{ 0xd200, 0xd200, nmi_enable_w },
	{ 0xd400, 0xd400, nmi_disable_w },
	{ 0xd600, 0xd600, MWA_NOP},
	{ 0xe000, 0xefff, MWA_NOP },
MEMORY_END

static MEMORY_READ_START( m68705_readmem )
	{ 0x0000, 0x0000, nycaptor_68705_portA_r },
	{ 0x0001, 0x0001, nycaptor_68705_portB_r },
	{ 0x0002, 0x0002, nycaptor_68705_portC_r },
	{ 0x0010, 0x007f, MRA_RAM },
	{ 0x0080, 0x07ff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( m68705_writemem )
	{ 0x0000, 0x0000, nycaptor_68705_portA_w },
	{ 0x0001, 0x0001, nycaptor_68705_portB_w },
	{ 0x0002, 0x0002, nycaptor_68705_portC_w },
	{ 0x0004, 0x0004, nycaptor_68705_ddrA_w },
	{ 0x0005, 0x0005, nycaptor_68705_ddrB_w },
	{ 0x0006, 0x0006, nycaptor_68705_ddrC_w },
	{ 0x0010, 0x007f, MWA_RAM },
	{ 0x0080, 0x07ff, MWA_ROM },
MEMORY_END


/* Cycle Shooting */


static READ_HANDLER(cyclshtg_mcu_status_r)
{
  return 0xff;
}

static READ_HANDLER(cyclshtg_mcu_r)
{
  return 7;
}

static WRITE_HANDLER(cyclshtg_mcu_w)
{

}

static READ_HANDLER(cyclshtg_mcu_status_r1)
{
  return rand();
}

static WRITE_HANDLER( cyclshtg_generic_control_w )
{
	int bank=(data>>2)&3;
	generic_control_reg = data;
	cpu_setbank(1, memory_region(REGION_CPU1) + 0x10000 + bank*0x4000 );
}

static struct DACinterface dac_interface =
{
	1,
	{ 50 }
};

static MEMORY_READ_START( cyclshtg_readmem )
    { 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
    { 0xc000, 0xcfff, nycaptor_videoram_r },
    { 0xd000, 0xd000, cyclshtg_mcu_r },
	{ 0xd002, 0xd002, nycaptor_generic_control_r },
    { 0xd400, 0xd400, from_snd_r },
	{ 0xd800, 0xd800, input_port_0_r },
	{ 0xd801, 0xd801, input_port_1_r },
	{ 0xd802, 0xd802, input_port_2_r },
	{ 0xd803, 0xd803, input_port_3_r },
	{ 0xd804, 0xd804, input_port_4_r },
	{ 0xd805, 0xd805, cyclshtg_mcu_status_r },
	{ 0xd806, 0xd806, MRA_NOP },
	{ 0xd807, 0xd807, cyclshtg_mcu_status_r },
	{ 0xdc00, 0xdc9f, nycaptor_spriteram_r},
	{ 0xdca0, 0xdcbf, nycaptor_scrlram_r },
	{ 0xdd00, 0xdeff, nycaptor_palette_r },
	{ 0xdf03, 0xdf03, nycaptor_gfxctrl_r },
    { 0xe000, 0xffff, nycaptor_sharedram_r },
MEMORY_END

static MEMORY_WRITE_START( cyclshtg_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, nycaptor_videoram_w, &videoram, &videoram_size },
	{ 0xd000, 0xd000, cyclshtg_mcu_w },
	{ 0xd001, 0xd001, sub_cpu_halt_w },
	{ 0xd002, 0xd002, cyclshtg_generic_control_w },
	{ 0xd400, 0xd400, sound_command_w },
	{ 0xd403, 0xd403, sound_cpu_reset_w },
	{ 0xdc00, 0xdc9f, nycaptor_spriteram_w},
	{ 0xdca0, 0xdcbf, nycaptor_scrlram_w, &nycaptor_scrlram },
	{ 0xdce1, 0xdce1, MWA_NOP},
	{ 0xdd00, 0xdeff, nycaptor_palette_w },
	{ 0xdf03, 0xdf03, nycaptor_gfxctrl_w },
	{ 0xe000, 0xffff, nycaptor_sharedram_w,&nycaptor_sharedram },
MEMORY_END


static MEMORY_READ_START( cyclshtg_readmem_sub )
  { 0x0000, 0xbfff, MRA_ROM },
  { 0xd800, 0xd800, input_port_0_r },
	{ 0xd801, 0xd801, input_port_1_r },
	{ 0xd802, 0xd802, input_port_2_r },
	{ 0xd803, 0xd803, input_port_3_r },
	{ 0xd804, 0xd804, input_port_4_r },
	{ 0xdc00, 0xdc9f, nycaptor_spriteram_r},
	{ 0xdd00, 0xdeff, nycaptor_palette_r },
	{ 0xdf00, 0xdf00, nycaptor_bx_r },
	{ 0xdf01, 0xdf01, nycaptor_by_r },
	{ 0xdf02, 0xdf02, nycaptor_b_r },
	{ 0xdf03, 0xdf03, nycaptor_gfxctrl_r },
  { 0xe000, 0xffff, nycaptor_sharedram_r },
MEMORY_END

static MEMORY_WRITE_START( cyclshtg_writemem_sub )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc7ff, nycaptor_videoram_w, &videoram, &videoram_size },
	{ 0xdc00, 0xdc9f, nycaptor_spriteram_w},
	{ 0xdca0, 0xdcbf, nycaptor_scrlram_w, &nycaptor_scrlram },
	{ 0xdd00, 0xdeff, nycaptor_palette_w },
	{ 0xdf03, 0xdf03, MWA_NOP },
	{ 0xe000, 0xffff, nycaptor_sharedram_w },
MEMORY_END

static READ_HANDLER(unk_r)
{
  return rand();
}

static MEMORY_READ_START( bronx_readmem )
    { 0x0000, 0x7fff, MRA_ROM },
    { 0x8000, 0xbfff, MRA_BANK1 },
    { 0xc000, 0xcfff, nycaptor_videoram_r },
    { 0xd000, 0xd000, cyclshtg_mcu_r },
    { 0xd002, 0xd002, nycaptor_generic_control_r },
    { 0xd400, 0xd400, from_snd_r },
    { 0xd401, 0xd401, unk_r },
    { 0xd800, 0xd800, input_port_0_r },
	{ 0xd801, 0xd801, input_port_1_r },
	{ 0xd802, 0xd802, input_port_2_r },
	{ 0xd803, 0xd803, input_port_3_r },
	{ 0xd804, 0xd804, input_port_4_r },
	{ 0xd805, 0xd805, cyclshtg_mcu_status_r },
	{ 0xd806, 0xd806, MRA_NOP },
	{ 0xd807, 0xd807, cyclshtg_mcu_status_r },
	{ 0xdc00, 0xdc9f, nycaptor_spriteram_r },
	{ 0xdca0, 0xdcbf, nycaptor_scrlram_r },
	{ 0xdd00, 0xdeff, nycaptor_palette_r },
	{ 0xdf03, 0xdf03, nycaptor_gfxctrl_r },
    { 0xe000, 0xffff, nycaptor_sharedram_r },
MEMORY_END

static MEMORY_WRITE_START( bronx_writemem )
    { 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, nycaptor_videoram_w, &videoram, &videoram_size },
	{ 0xd000, 0xd000, MWA_NOP },
	{ 0xd001, 0xd001, sub_cpu_halt_w },
	{ 0xd002, 0xd002, cyclshtg_generic_control_w },
	{ 0xd400, 0xd400, sound_command_w },
	{ 0xd403, 0xd403, sound_cpu_reset_w },
	{ 0xdc00, 0xdc9f, nycaptor_spriteram_w },
	{ 0xdca0, 0xdcbf, nycaptor_scrlram_w, &nycaptor_scrlram },
	{ 0xdd00, 0xdeff, nycaptor_palette_w },
    { 0xdf03, 0xdf03, nycaptor_gfxctrl_w },
    { 0xe000, 0xffff, nycaptor_sharedram_w, &nycaptor_sharedram },
MEMORY_END

static MEMORY_READ_START( bronx_readmem_sub )
    { 0x0000, 0x7fff, MRA_ROM },
    { 0xd800, 0xd800, input_port_0_r },
    { 0xd801, 0xd801, input_port_1_r },
	{ 0xd802, 0xd802, input_port_2_r },
	{ 0xd803, 0xd803, input_port_3_r },
	{ 0xd804, 0xd804, input_port_4_r },
	{ 0xd805, 0xd805, cyclshtg_mcu_status_r1 },
	{ 0xd807, 0xd807, cyclshtg_mcu_status_r },
	{ 0xdc00, 0xdc9f, nycaptor_spriteram_r },
	{ 0xdd00, 0xdeff, nycaptor_palette_r },
	{ 0xdf00, 0xdf00, nycaptor_bx_r },
	{ 0xdf01, 0xdf01, nycaptor_by_r },
	{ 0xdf02, 0xdf02, nycaptor_b_r },
	{ 0xdf03, 0xdf03, nycaptor_gfxctrl_r },
    { 0xe000, 0xffff, nycaptor_sharedram_r },
MEMORY_END

static MEMORY_WRITE_START( bronx_writemem_sub )
    { 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc7ff, nycaptor_videoram_w, &videoram, &videoram_size },
	{ 0xdc00, 0xdc9f, nycaptor_spriteram_w },
	{ 0xdca0, 0xdcbf, nycaptor_scrlram_w, &nycaptor_scrlram },
	{ 0xdd00, 0xdeff, nycaptor_palette_w },
	{ 0xdf03, 0xdf03, nycaptor_gfxctrl_w },
	{ 0xe000, 0xffff, nycaptor_sharedram_w },
MEMORY_END

static READ_HANDLER( port_fetch_r )
{
	return memory_region(REGION_USER1)[offset];
}

static PORT_READ_START( bronx_readport )
	{ 0x0000, 0x7fff, port_fetch_r },
PORT_END


static MEMORY_WRITE_START( bronx_sound_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xc800, 0xc800, AY8910_control_port_0_w },
	{ 0xc801, 0xc801, AY8910_write_port_0_w },
	{ 0xc802, 0xc802, AY8910_control_port_1_w },
	{ 0xc803, 0xc803, AY8910_write_port_1_w },
	{ 0xc900, 0xc90d, MSM5232_0_w },
	{ 0xca00, 0xca00, MWA_NOP},
	{ 0xcb00, 0xcb00, MWA_NOP},
	{ 0xcc00, 0xcc00, MWA_NOP},
	{ 0xd000, 0xd000, to_main_w },
	{ 0xd200, 0xd200, nmi_enable_w },
	{ 0xd400, 0xd400, nmi_disable_w },
	{ 0xd600, 0xd600, DAC_0_signed_data_w },
	{ 0xe000, 0xefff, MWA_NOP },
MEMORY_END


/* verified from Z80 code */
INPUT_PORTS_START( bronx )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW ) 
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) ) 
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) ) 
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) ) 
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) ) 
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) ) 
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) ) 
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) ) 
	/* coinage B isn't mentionned in the manual */
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) ) 
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) ) 
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) ) 
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) ) 
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

	PORT_START
    PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) ) 
	PORT_DIPSETTING(    0x02, "Easy" ) 
	PORT_DIPSETTING(    0x03, "Medium" ) 
	PORT_DIPSETTING(    0x01, "Hard" ) 
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x08, DEF_STR( Bonus_Life ) )       /* table at 0x100f - see notes for 'bronx' */
	PORT_DIPSETTING(	0x0c, "150k 350k 200k+" )
	PORT_DIPSETTING(	0x08, "200k 500k 300k+" )
	PORT_DIPSETTING(	0x04, "300k 700k 400k+" )
	PORT_DIPSETTING(	0x00, "400k 900k 500k+" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )            /* see notes */
	PORT_DIPSETTING(	0x00, "1" )
	PORT_DIPSETTING(	0x30, "2" )
	PORT_DIPSETTING(	0x10, "4" )
	PORT_DIPSETTING(	0x20, "5" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x80, 0x80, "Reset Damage (Cheat)" )      /* see notes */
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x10, 0x10, "Infinite Bullets" )          /* see notes */
	PORT_DIPSETTING(	0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )            /* IPT_START2 is some similar Taito games (eg: 'flstory') */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0xfe, IP_ACTIVE_LOW, IPT_UNUSED )

 	PORT_START
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_Y | IPF_PLAYER1, 25, 15, 0x00, 0xff)

	PORT_START
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_X | IPF_PLAYER1, 25, 15, 0x00, 0xff)
INPUT_PORTS_END


INPUT_PORTS_START( nycaptor )
	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x02, "20k, 80k then every 80k" )
	PORT_DIPSETTING(    0x03, "50k, 150k then every 200k" )
	PORT_DIPSETTING(    0x01, "100k, 300k then every 300k" )
	PORT_DIPSETTING(    0x00, "150k, 300k then every 300k" )
	PORT_BITX(    0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Infinite Bullets", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x18, "3" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x0f, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 9C_1C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 8C_1C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 7C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_8C ) )
	PORT_DIPNAME( 0xf0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 9C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 8C_1C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 7C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_8C ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Freeze" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Training Spot" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x0c, "Easy" )
	PORT_DIPSETTING(    0x04, "Normal" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x10, 0x10, "Coinage Display" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x20, "Reset Damage" )
	PORT_DIPSETTING(    0x20, "Every Stage" )
	PORT_DIPSETTING(    0x00, "Every 4 Stages" )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "No Hit", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Coin Slots" )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x80, "2" )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* IPT_START2 is some similar Taito games (eg: 'flstory') */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* "I/O ERROR" if active */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* "I/O ERROR" if active */

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0xfe, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_X | IPF_PLAYER1, 25, 15, 0x00, 0xff)

	PORT_START
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_Y | IPF_PLAYER1, 25, 15, 0x00, 0xff)
INPUT_PORTS_END

static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+0, RGN_FRAC(1,2)+4, 0, 4 },
	{ 3, 2, 1, 0, 8+3, 8+2, 8+1, 8+0 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8
};

static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+0, RGN_FRAC(1,2)+4, 0, 4 },
	{ 3, 2, 1, 0, 8+3, 8+2, 8+1, 8+0,
			16*8+3, 16*8+2, 16*8+1, 16*8+0, 16*8+8+3, 16*8+8+2, 16*8+8+1, 16*8+8+0 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			16*16, 17*16, 18*16, 19*16, 20*16, 21*16, 22*16, 23*16 },
	64*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,     0, 16 },/*16 kolorow*/
	{ REGION_GFX1, 0, &spritelayout, 256, 16 },/*paleta 2, 16 kolorow*/
	{ -1 }	/* end of array */
};


static MACHINE_DRIVER_START( nycaptor )
	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,8000000/2)		/* ??? */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80,8000000/2)
	MDRV_CPU_MEMORY(readmem_sub,writemem_sub)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)	/* IRQ generated by ??? */

	MDRV_CPU_ADD(Z80,8000000/2)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,2)	/* IRQ generated by ??? */

	MDRV_CPU_ADD(M68705,2000000)
	MDRV_CPU_MEMORY(m68705_readmem,m68705_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)	/* 100 CPU slices per frame - an high value to ensure proper */

	/* synchronization of the CPUs */
	MDRV_MACHINE_INIT(ta7630)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_VIDEO_START(nycaptor)
	MDRV_VIDEO_UPDATE(nycaptor)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
	MDRV_SOUND_ADD(MSM5232, msm5232_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( cyclshtg )
	MDRV_CPU_ADD(Z80,8000000/2)

	MDRV_CPU_MEMORY(cyclshtg_readmem,cyclshtg_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80,8000000/2)
	MDRV_CPU_MEMORY(cyclshtg_readmem_sub,cyclshtg_writemem_sub)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80,8000000/2)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem,bronx_sound_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,2)

#ifdef USE_MCU
	MDRV_CPU_ADD(M68705,2000000)
	MDRV_CPU_MEMORY(m68705_readmem,m68705_writemem)
#endif

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)
	MDRV_MACHINE_INIT(ta7630)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_VIDEO_START(nycaptor)
	MDRV_VIDEO_UPDATE(nycaptor)

	MDRV_SOUND_ADD(AY8910, ay8910_interface)
	MDRV_SOUND_ADD(MSM5232, msm5232_interface)
	MDRV_SOUND_ADD(DAC, dac_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( bronx )
	MDRV_CPU_ADD(Z80,8000000/2)

	MDRV_CPU_MEMORY(bronx_readmem, bronx_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80,8000000/2)
	MDRV_CPU_MEMORY(bronx_readmem_sub, bronx_writemem_sub)
	MDRV_CPU_PORTS(bronx_readport,0)
	MDRV_CPU_FLAGS(CPU_16BIT_PORT)

	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80,8000000/2)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem,bronx_sound_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,2)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(2)
	MDRV_MACHINE_INIT(ta7630)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_VIDEO_START(nycaptor)
	MDRV_VIDEO_UPDATE(nycaptor)

	MDRV_SOUND_ADD(AY8910, ay8910_interface)
	MDRV_SOUND_ADD(MSM5232, msm5232_interface)
	MDRV_SOUND_ADD(DAC, dac_interface)
MACHINE_DRIVER_END



/***************************************************************************
  Game driver(s)
***************************************************************************/

ROM_START( nycaptor )
	ROM_REGION( 0x18000, REGION_CPU1, 0 )
	ROM_LOAD( "a50_04",   0x00000, 0x4000, CRC(33d971a3) SHA1(8bf6cb8d799739dc6f115d352453af278d58de9a) )
	ROM_LOAD( "a50_03",   0x04000, 0x4000, CRC(8557fa44) SHA1(5639ec2ac21ae94c416c01bd7c0dae722cc14598) )
	ROM_LOAD( "a50_02",   0x10000, 0x4000, CRC(9697b898) SHA1(28b92963264b867ca4452dad7dcbbb8c8247d2f5) )
	ROM_LOAD( "a50_01",   0x14000, 0x4000, CRC(0965f84a) SHA1(22698446f52f5d29632aa982b9be87a9bf86fbef) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "a50_05-2", 0x0000, 0x4000, CRC(7796655a) SHA1(74cf84bf6f4dfee57ef6236280c0d64c1a156ffe) )
	ROM_LOAD( "a50_06",   0x4000, 0x4000, CRC(450d6783) SHA1(e652bf83fb6bdd8152dbafb05bb695259c2619cc) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )
	ROM_LOAD( "a50_15",   0x0000, 0x4000, CRC(f8a604e5) SHA1(8fae920fd09584b5e5ccd0db8b8934b393a23d50) )
	ROM_LOAD( "a50_16",   0x4000, 0x4000, CRC(fc24e11d) SHA1(ce1a1d7b809fa0f5f5e7a462047374b1b3f621c6) )

	ROM_REGION( 0x0800, REGION_CPU4, 0 )
	ROM_LOAD( "a50_17",   0x0000, 0x0800, CRC(69fe08dc) SHA1(9bdac3e835f63bbb8806892169d89f43d447df21) )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "a50_07",   0x00000, 0x4000, CRC(b3b330cf) SHA1(8330e4831b8d0068c0367417db2e27fded7c56fe) )
	ROM_LOAD( "a50_09",   0x04000, 0x4000, CRC(22e2232e) SHA1(3b43114a5511b00e45cd67073c3f833fcf098fb6) )
	ROM_LOAD( "a50_11",   0x08000, 0x4000, CRC(2c04ad4f) SHA1(a6272f91f583d46dd4a3fe863482b39406c70e97) )
	ROM_LOAD( "a50_13",   0x0c000, 0x4000, CRC(9940e78d) SHA1(8607808e6ff76808ae155d671c3179bfe7d2639b) )
	ROM_LOAD( "a50_08",   0x10000, 0x4000, CRC(3c31be14) SHA1(a1ed382fbd374609022535bd0d78bc9e84edd63b) )
	ROM_LOAD( "a50_10",   0x14000, 0x4000, CRC(ce84dc5a) SHA1(8723310f3e25820ef7490c16759ebcb8354dde85) )
	ROM_LOAD( "a50_12",   0x18000, 0x4000, CRC(3fb4cfa3) SHA1(b1c5f7b0297c59dc93420d31e0ef2b1125dbb9db) )
	ROM_LOAD( "a50_14",   0x1c000, 0x4000, CRC(24b2f1bf) SHA1(4757aec2e4b99ce33d993ce1e19ee46a4eb76e86) )
ROM_END

ROM_START( cyclshtg )
	ROM_REGION( 0x18000, REGION_CPU1, 0 )
	ROM_LOAD( "a97_01.i17",   0x00000, 0x4000, CRC(686fac1a) SHA1(46d17cb98f064413bb76c5d869f8061d2771cda0) )
	ROM_LOAD( "a97_02.i16",   0x04000, 0x4000, CRC(48a812f9) SHA1(8ab18cb8d6a8b7ce1ed1a4009f5435ce4b0937b4) )
	ROM_LOAD( "a97_03.u15",   0x10000, 0x4000, CRC(67ad3067) SHA1(2e355653e91c093abe7db0a3d55d5a3f95c4a2e3) )
	ROM_LOAD( "a97_04.u14",   0x14000, 0x4000, CRC(804e6445) SHA1(5b6771c5729faf62d5002d090c0b9c5ca5cb9ad6) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "a97_05.u22",   0x0000, 0x4000, CRC(fdc36c4f) SHA1(cae2d3f07c5bd6de9d40ff7d385b999e7dc9ce82) )
	ROM_LOAD( "a80_06.u23",   0x4000, 0x4000, CRC(2769c5ab) SHA1(b8f5a4a8c70c8d37d5e92b37faa0e25b287b3fb2) )
	ROM_LOAD( "a97_06.i24",   0x8000, 0x4000, CRC(c0473a54) SHA1(06fa7345a44a72995146e973c2cd7a14499f4310) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )
	ROM_LOAD( "a80_16.i26",   0x0000, 0x4000, CRC(ce171a48) SHA1(e5ae9bb22f58c8857737bc6f5317866819a4e4d1) )
	ROM_LOAD( "a80_17.i25",   0x4000, 0x4000, CRC(a90b7bbc) SHA1(bd5c96861a59a1f84bb5032775b1c70efdb7066f) )

	ROM_REGION( 0x0800, REGION_CPU4, 0 )
	ROM_LOAD( "a80_18",       0x0000, 0x0800, NO_DUMP ) /* Missing */

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "a80_11.u11",   0x00000, 0x4000, CRC(29e1293b) SHA1(106204ec46fae5d3b5e4a4d423bc0e886a637c61) )
	ROM_LOAD( "a80_10.u10",   0x04000, 0x4000, CRC(345f576c) SHA1(fee5b2167bcd0fdc21c0a7b22ffdf7506a24baee) )
	ROM_LOAD( "a80_09.u9",    0x08000, 0x4000, CRC(3ef06dff) SHA1(99bbd32ae89a6becac9e1bb8a34a834d81890444) )
	ROM_LOAD( "a97_07.u8",    0x0c000, 0x4000, CRC(8f2baf57) SHA1(bb4e5b69477c51536dfd95ea59c7281159f3bcc7) )
	ROM_LOAD( "a80_15.u39",   0x10000, 0x4000, CRC(2cefb47d) SHA1(3bef20c9c0c4f9237a327da3cbc9a7bbf63771ea) )
	ROM_LOAD( "a80_14.u34",   0x14000, 0x4000, CRC(91642de8) SHA1(531974fc147d25e9feada89bc82d5df62ec9d446) )
	ROM_LOAD( "a80_13.u33",   0x18000, 0x4000, CRC(96a67c6b) SHA1(3bed4313d6b8d554b35abb1515ab94e78c2718c5) )
	ROM_LOAD( "a80_12.u23",   0x1c000, 0x4000, CRC(9ff04c85) SHA1(a5edc50bbe6e2c976895c97400c75088bc90a1fc) )
ROM_END

/*
Bronx (Cycle Shooting bootleg)
Taito, 1986

The hardware is an almost exact copy of the original Taito hardware,
minus the 68705 Microcontroller :-)
*/

ROM_START( bronx )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )
	ROM_LOAD( "1.bin",   0x00000, 0x4000, CRC(399b5063) SHA1(286e0d1a0c1a41060e87f2dadf199f38c787d40f) )
	ROM_LOAD( "2.bin",   0x04000, 0x4000, CRC(3b5f9756) SHA1(7df0e4808799ca6a7809bd9ac3a1689a18ae1cdb) )
	ROM_LOAD( "3.bin",   0x10000, 0x8000, CRC(d2ffd3ce) SHA1(06d237a4aa46e37192bd94e2db361c62c35d469c) )
	ROM_LOAD( "4.bin",   0x18000, 0x8000, CRC(20cf148d) SHA1(49f49f9e58d7aa5690ff828b746ab856c71b0d9c) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "5.bin",   0x0000, 0x4000, CRC(19a1c665) SHA1(38f5f47f22740a75cf0ce6d0406368c2c86e0021) )
	ROM_LOAD( "6.bin",   0x4000, 0x4000, CRC(2769c5ab) SHA1(b8f5a4a8c70c8d37d5e92b37faa0e25b287b3fb2) )

	ROM_REGION( 0x08000, REGION_USER1, 0 )
	ROM_LOAD( "7.bin",   0x00000, 0x8000, CRC(463f9f62) SHA1(8e1fa8f78d230d32502422078599e9e9af889a92) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )
	ROM_LOAD( "a80_16.i26",   0x0000, 0x4000, CRC(ce171a48) SHA1(e5ae9bb22f58c8857737bc6f5317866819a4e4d1) )
	ROM_LOAD( "a80_17.i25",   0x4000, 0x4000, CRC(a90b7bbc) SHA1(bd5c96861a59a1f84bb5032775b1c70efdb7066f) )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "a80_11.u11",   0x00000, 0x4000, CRC(29e1293b) SHA1(106204ec46fae5d3b5e4a4d423bc0e886a637c61) )
	ROM_LOAD( "a80_10.u10",   0x04000, 0x4000, CRC(345f576c) SHA1(fee5b2167bcd0fdc21c0a7b22ffdf7506a24baee) )
	ROM_LOAD( "a80_09.u9",    0x08000, 0x4000, CRC(3ef06dff) SHA1(99bbd32ae89a6becac9e1bb8a34a834d81890444) )
	ROM_LOAD( "8.bin",    	  0x0c000, 0x4000, CRC(2b778d24) SHA1(caca7a18743a4bb657a7c5691d93de0ccb867003) )

	ROM_LOAD( "a80_15.u39",   0x10000, 0x4000, CRC(2cefb47d) SHA1(3bef20c9c0c4f9237a327da3cbc9a7bbf63771ea) )
	ROM_LOAD( "a80_14.u34",   0x14000, 0x4000, CRC(91642de8) SHA1(531974fc147d25e9feada89bc82d5df62ec9d446) )
	ROM_LOAD( "a80_13.u33",   0x18000, 0x4000, CRC(96a67c6b) SHA1(3bed4313d6b8d554b35abb1515ab94e78c2718c5) )
	ROM_LOAD( "a80_12.u23",   0x1c000, 0x4000, CRC(9ff04c85) SHA1(a5edc50bbe6e2c976895c97400c75088bc90a1fc) )
ROM_END


static DRIVER_INIT(bronx)
{
	int i;
	for(i=0;i<0x20000;i++)
		memory_region(REGION_CPU1)[i]=BITSWAP8(memory_region(REGION_CPU1)[i],0,1,2,3,4,5,6,7);
	nyc_gametype=1;
	
}

static DRIVER_INIT(nycaptor)
{
	nyc_gametype=0;
}

static DRIVER_INIT(cyclshtg)
{
	nyc_gametype=1;
}

GAMEX(1985, nycaptor, 0,        nycaptor,  nycaptor, nycaptor, ROT0,  "Taito", "N.Y. Captor", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAMEX(1986, cyclshtg, 0,        cyclshtg,  bronx,    cyclshtg, ROT90, "Taito", "Cycle Shooting", GAME_NOT_WORKING )
GAMEX(1986, bronx,    cyclshtg, bronx,     bronx,    bronx,    ROT90, "bootleg", "Bronx", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
