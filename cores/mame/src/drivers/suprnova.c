/* Super Kaneko Nova System */
/* Original Driver by Sylvain Glaize */
/* taken to pieces and attempted reconstruction by David Haywood */

/* Credits (in no particular order):
   Olivier Galibert for all the assistance and information he's provided
   R.Belmont for working on the SH2 timers so sound worked
   Nicola Salmoria for hooking up the Roz and improving the dirty tile handling
   Paul Priest for a lot of things
   Stephh for spotting what was wrong with Puzz Loop's inputs
*/

/*
ToDo:
   Priorities
   Transparency effects (each pen has a transparent flag, there are seperate r,g,b alpha values to be used for these pens)
   Idle skipping on other games + bios ..
   Fix gaps between zoomed sprites (See Gals Panic 4 continue screen and timer)
   Fix broken games

Alpha pens:
   galpanis uses them to white in/out the bg's during attract? Solid white atm.

Priorities:
   Puzzloop appears to use the priorities, the other games might use alpha effects to disable a layer instead ..
     *they don't appear to use alpha, so how should it work, senknow intro, galpanis attract,  cyvern level 4?

Ram-Based Tiles:
   The front layer in galpanic games
   The kaneko logo in galpanis
   The Bios

Sprite Zoom:
   Used all over the place in the Gals Panic games, check the bit in galpani4 attract where if you hammer the button the ballon expands until it bursts.
   Puzz Loop main logo

Roz:
   Should be used on galpani4 bg's, but for some reason incxx,incxy,incyx,incyy are always 1,0,0,1 resp. Maybe a timer problem, input problem or core problem. Who knows?
      Update, rand() was returning a 15-bit number under mingw. Top bit seemed to determine whether it rotated or not.
   To zoom the skull in the senknow attract mode
   To zoom the girl at the end of a level in galpanis
   To zoom the kaneko logo in galpanis

Linescroll:
   Puzzloop intro, the screen covered in 'puzz loop' of varying sizes, the little ones go left and the big ones right.
   Puzzloop title screen uses it just to scroll the bg up.
   Also used behind the hi-score table on puzzloop...
   Galpani4/galpanis use it for 2P mode on both bg A and bg B.

R,G,B Brightness:
   The Electro Design games have this set darker than 0xff for sprites and bg's anyway.
   Galpanis uses it to dim the bg's in attract on the stats screens for the characters (bg's hidden by white pen atm).
Lots of games use them to fade, haven't seen r,g,b used independantly yet but you never know.

etc.etc.

-pjp/dh

gutsn Crash Reason
the routine at 401d8c8 calls 40249e0 with r14 as a parameter, which points where the return pr on the stack is
param is 60ffbc0, pr is at 60ffbcc, but 40249e0 writes more than 12 bytes
Gutsn now works in 0.69, timing/sh2 fixes??

Motherboard Layout
------------------

SUPER-KANEKO-NOVA-SYSTEM
MAIN-BOARD-A
NEP-16


   BATT                                      HM514260
   CR2032                       SKNSA1       HM514260       SH2
            W24257                          LH5168
YMZ280B-F   W24257   BABY004  LATTICE  SW1  M62X42
                              pLSI1016
  HITACHI          28.636MHz 33.3333MHz
  DF1                        21.504MHz
J D49307     ALTAIR                   DENEB
             BY006-224                BY007-32F
A            (QFP208)                 (QFP208)

M                  W24257  W24257           HM514260
            /-     W24257  W24257                                 /-
M VEGA      ||             W24257  SPCII-B  HM514260              ||
  BY005-197 ||             W24257  JH-6186                        ||
A (QFP144)  ||                     (QFP208)                       ||
            ||                               W24257               ||
  DSW1(8)   ||      VIEWIII-A      SPCII-A   W24257 LH540202      ||
            ||      BL-001         JH-4181   W24257               ||
            \-      (QFP240)       (QFP208)  W24257               \-
  OKIM6253


Notes:
      SKNSA1 is BIOS (Asia)
      HM514260, W24257, LH5168, M62X42 all smt SRAM
      LH540202 is DIP SRAM


Cart Layout
-----------

SUPER-KANEKO-NOVA-SYSTEM
ROM-BOARD
NEP-16


 /-                                                    /-
 ||SK300-00  *  PAL PAL D431000 D431000  *       *     ||
 ||                                                    ||
 ||                                                    ||
 ||                        *   SK-200-00               ||
 ||          SK01A  SK01A                              ||
 ||          U8     U10    *      *    SK-101 SK-100-00||
 \-                                                    \-

 Notes:
       *: unpopulated position for surface mounted 16MBit SOP44 MASK ROM
       U8 and U10 are socketed 27C040 EPROM
       All other ROMs are surface mounted SOP44 MASK ROM

Cart Layout
-----------

SUPER KANEKO NOVA SYSTEM
ROM 4 BOARD
NEP-16

Top Side:
 /-                                                    /-
 ||SS300-00 SS210-00     SS102-00 SS103-00   SS201-00  ||
 ||U1       U3           U8       U32        U9        ||
 ||                                                    ||
 ||                                                    ||
 || 082*    SG01A  SG01A  #      #                     ||
 ||          U4     U6    U29    U30                   ||
 \-                                                    \-

Bottom Side:
 /-                                                    /-
 ||SS200-00 SS000-00     SS101-00       ^       ^      ||
 ||U17      U21          U20            U24     U26    ||
 ||                                                    ||
 ||                                                    ||
 ||                                  NEC     NEC       ||
 ||                                  D431000 D43100    ||
 \-                                                    \-

* Kaneko (208 pin PQFP)    # Empty sockets for 27C4001 / 27C040
  ROM0                     ^ Empty sockets for uPD23C32000
  082                        Full NEC ram number: D431000AGW-70LL
  9709PK002


Cart Layout
-----------

SUPER-KANEKO-NOVA-SYSTEM
ROM-2-BOARD
NEP-16


 /-                                                    /-
 ||PZL-300-00 * PAL PAL  *   PZL-200-00 PZL-210-00  *  ||
 ||                                                    ||
 ||                             D431000 D431000        ||
 ||                                                    ||
 ||          PZ01U  PZ01U  #    #                      ||
 ||          U8     U10    U43  U44   *   * PZL-100-00 ||
 \-                                                    \-


*/

#include "driver.h"
#include "machine/random.h"
#include "vidhrdw/generic.h"
#include <time.h>

#define BIOS_SKIP 1 /* Skip Bios as it takes too long and doesn't complete atm.*/

/* Defined in vidhrdw*/
extern void skns_sprite_kludge(int x, int y);

data32_t *skns_tilemapA_ram, *skns_tilemapB_ram, *skns_v3slc_ram;
data32_t *skns_palette_ram, *skns_v3t_ram, *skns_main_ram, *skns_cache_ram;
data32_t *skns_pal_regs, *skns_v3_regs, *skns_spc_regs;

data32_t skns_v3t_dirty[0x4000]; /* allocate this elsewhere?*/
data32_t skns_v3t_4bppdirty[0x8000]; /* allocate this elsewhere?*/
int skns_v3t_somedirty,skns_v3t_4bpp_somedirty;

static UINT8 bright_spc_b=0x00, bright_spc_g=0x00, bright_spc_r=0x00;
static UINT8 bright_v3_b=0x00,  bright_v3_g=0x00,  bright_v3_r=0x00;
static int use_spc_bright, use_v3_bright;

WRITE32_HANDLER ( skns_tilemapA_w );
WRITE32_HANDLER ( skns_tilemapB_w );
WRITE32_HANDLER ( skns_v3_regs_w );
VIDEO_START(skns);
VIDEO_EOF(skns);
VIDEO_UPDATE(skns);

/* hit.c */

static struct {
	UINT16 x1p, y1p, z1p, x1s, y1s, z1s;
	UINT16 x2p, y2p, z2p, x2s, y2s, z2s;
	UINT16 org;

	UINT16 x1_p1, x1_p2, y1_p1, y1_p2, z1_p1, z1_p2;
	UINT16 x2_p1, x2_p2, y2_p1, y2_p2, z2_p1, z2_p2;
	UINT16 x1tox2, y1toy2, z1toz2;
	INT16 x_in, y_in, z_in;
	UINT16 flag;

	UINT8 disconnect;
} hit;

static void hit_calc_orig(UINT16 p, UINT16 s, UINT16 org, UINT16 *l, UINT16 *r)
{
	switch(org & 3) {
	case 0:
		*l = p;
		*r = p+s;
	break;
	case 1:
		*l = p-s/2;
		*r = *l+s;
	break;
	case 2:
		*l = p-s;
		*r = p;
	break;
	case 3:
		*l = p-s;
		*r = p+s;
	break;
	}
}

static void hit_calc_axis(UINT16 x1p, UINT16 x1s, UINT16 x2p, UINT16 x2s, UINT16 org,
			  UINT16 *x1_p1, UINT16 *x1_p2, UINT16 *x2_p1, UINT16 *x2_p2,
			  INT16 *x_in, UINT16 *x1tox2)
{
	UINT16 x1l, x1r, x2l, x2r;
	hit_calc_orig(x1p, x1s, org,      &x1l, &x1r);
	hit_calc_orig(x2p, x2s, org >> 8, &x2l, &x2r);

	*x1tox2 = x2p-x1p;
	*x1_p1 = x1p;
	*x2_p1 = x2p;
	*x1_p2 = x1r;
	*x2_p2 = x2l;
	*x_in = x1r-x2l;
}

static void hit_recalc(void)
{
	hit_calc_axis(hit.x1p, hit.x1s, hit.x2p, hit.x2s, hit.org,
		&hit.x1_p1, &hit.x1_p2, &hit.x2_p1, &hit.x2_p2,
		&hit.x_in, &hit.x1tox2);
	hit_calc_axis(hit.y1p, hit.y1s, hit.y2p, hit.y2s, hit.org,
		&hit.y1_p1, &hit.y1_p2, &hit.y2_p1, &hit.y2_p2,
		&hit.y_in, &hit.y1toy2);
	hit_calc_axis(hit.z1p, hit.z1s, hit.z2p, hit.z2s, hit.org,
		&hit.z1_p1, &hit.z1_p2, &hit.z2_p1, &hit.z2_p2,
		&hit.z_in, &hit.z1toz2);

	hit.flag = 0;
	hit.flag |= hit.y2p > hit.y1p ? 0x8000 : hit.y2p == hit.y1p ? 0x4000 : 0x2000;
	hit.flag |= hit.y_in >= 0 ? 0 : 0x1000;
	hit.flag |= hit.x2p > hit.x1p ? 0x0800 : hit.x2p == hit.x1p ? 0x0400 : 0x0200;
	hit.flag |= hit.x_in >= 0 ? 0 : 0x0100;
	hit.flag |= hit.z2p > hit.z1p ? 0x0080 : hit.z2p == hit.z1p ? 0x0040 : 0x0020;
	hit.flag |= hit.z_in >= 0 ? 0 : 0x0010;
	hit.flag |= hit.x_in >= 0 && hit.y_in >= 0 && hit.z_in >= 0 ? 8 : 0;
	hit.flag |= hit.z_in >= 0 && hit.x_in >= 0                  ? 4 : 0;
	hit.flag |= hit.y_in >= 0 && hit.z_in >= 0                  ? 2 : 0;
	hit.flag |= hit.x_in >= 0 && hit.y_in >= 0                  ? 1 : 0;
/*  if(0)
    log_event("HIT", "Recalc, (%d,%d)-(%d,%d)-(%d,%d):(%d,%d)-(%d,%d)-(%d,%d):%04x, (%d,%d,%d), %04x",
	      hit.x1p, hit.x1s, hit.y1p, hit.y1s, hit.z1p, hit.z1s,
	      hit.x2p, hit.x2s, hit.y2p, hit.y2s, hit.z2p, hit.z2s,
	      hit.org,
	      hit.x_in, hit.y_in, hit.z_in, hit.flag);
*/
}

WRITE32_HANDLER ( skns_hit_w )
/*void hit_w(UINT32 adr, UINT32 data, int type)*/
{
	int adr = offset * 4;

	switch(adr) {
	case 0x00:
	case 0x28:
		hit.x1p = data;
	break;
	case 0x08:
	case 0x30:
		hit.y1p = data;
	break;
	case 0x38:
	case 0x50:
		hit.z1p = data;
	break;
	case 0x04:
	case 0x2c:
		hit.x1s = data;
	break;
	case 0x0c:
	case 0x34:
		hit.y1s = data;
	break;
	case 0x3c:
	case 0x54:
		hit.z1s = data;
	break;
	case 0x10:
	case 0x58:
		hit.x2p = data;
	break;
	case 0x18:
	case 0x60:
		hit.y2p = data;
	break;
	case 0x20:
	case 0x68:
		hit.z2p = data;
	break;
	case 0x14:
	case 0x5c:
		hit.x2s = data;
	break;
	case 0x1c:
	case 0x64:
		hit.y2s = data;
	break;
	case 0x24:
	case 0x6c:
		hit.z2s = data;
	break;
	case 0x70:
		hit.org = data;
	break;
	default:
/*		log_write("HIT", adr, data, type);*/
	break;
	}
	hit_recalc();
}

WRITE32_HANDLER ( skns_hit2_w )
{
/*	log_event("HIT", "Set disconnect to %02x", data);*/
	hit.disconnect = data;
}


READ32_HANDLER( skns_hit_r )
/*UINT32 hit_r(UINT32 adr, int type)*/
{
	int adr = offset *4;

/*	log_read("HIT", adr, type);*/

	if(hit.disconnect)
		return 0x0000;
	switch(adr) {
	case 0x28:
	case 0x2a:
		return (UINT16)mame_rand();
	case 0x00:
	case 0x10:
		return (UINT16)hit.x_in;
	case 0x04:
	case 0x14:
		return (UINT16)hit.y_in;
	case 0x18:
		return (UINT16)hit.z_in;
	case 0x08:
	case 0x1c:
		return hit.flag;
	case 0x40:
		return hit.x1p;
	case 0x48:
		return hit.y1p;
	case 0x50:
		return hit.z1p;
	case 0x44:
		return hit.x1s;
	case 0x4c:
		return hit.y1s;
	case 0x54:
		return hit.z1s;
	case 0x58:
		return hit.x2p;
	case 0x60:
		return hit.y2p;
	case 0x68:
		return hit.z2p;
	case 0x5c:
		return hit.x2s;
	case 0x64:
		return hit.y2s;
	case 0x6c:
		return hit.z2s;
	case 0x70:
		return hit.org;
	case 0x80:
		return hit.x1tox2;
	case 0x84:
		return hit.y1toy2;
	case 0x88:
		return hit.z1toz2;
	case 0x90:
		return hit.x1_p1;
	case 0xa0:
		return hit.y1_p1;
	case 0xb0:
		return hit.z1_p1;
	case 0x98:
		return hit.x1_p2;
	case 0xa8:
		return hit.y1_p2;
	case 0xb8:
		return hit.z1_p2;
	case 0x94:
		return hit.x2_p1;
	case 0xa4:
		return hit.y2_p1;
	case 0xb4:
		return hit.z2_p1;
	case 0x9c:
		return hit.x2_p2;
	case 0xac:
		return hit.y2_p2;
	case 0xbc:
		return hit.z2_p2;
	default:
/*		log_read("HIT", adr, type);*/
	return 0;
	}
}

/* end hit.c */


/* start old driver code */


static void interrupt_callback(int param)
{
	cpu_set_irq_line(0,param,HOLD_LINE);
}

static MACHINE_INIT(skns)
{
	timer_pulse(TIME_IN_MSEC(2), 15, interrupt_callback);
	timer_pulse(TIME_IN_MSEC(8), 11, interrupt_callback);
	timer_pulse(TIME_IN_CYCLES(0,1824), 9, interrupt_callback);

	cpu_setbank(1,memory_region(REGION_USER1));
}


static INTERRUPT_GEN(skns_interrupt)
{
	UINT8 interrupt = 5;
	switch(cpu_getiloops())
	{
		case 0:
			interrupt = 5; /* VBLANK*/
			break;
		case 1:
			interrupt = 1; /* SPC*/
			break;
	}
	cpu_set_irq_line(0,interrupt,HOLD_LINE);
}

INPUT_PORTS_START( skns )

	PORT_START  /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )

	PORT_START  /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )

	PORT_START  /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START  /* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START  /* Paddle A */
	PORT_ANALOG( 0xFF, 0x00, IPT_DIAL | IPF_REVERSE | IPF_PLAYER1, 100, 15, 0, 0 )

	PORT_START  /* Paddle B */
	PORT_ANALOG( 0xFF, 0x00, IPT_DIAL | IPF_REVERSE | IPF_PLAYER2, 100, 15, 0, 0 )

	PORT_START  /* Paddle C */
	PORT_ANALOG( 0xFF, 0x00, IPT_DIAL | IPF_REVERSE | IPF_PLAYER3, 100, 15, 0, 0 )

	PORT_START  /* DSW */
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR(Flip_Screen) ) /* This port affects 0x040191c8 function*/
	PORT_DIPSETTING(    0x02, DEF_STR(Off) )
	PORT_DIPSETTING(    0x00, DEF_STR(On) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR(Unknown) )
	PORT_DIPSETTING(    0x04, DEF_STR(Off) )
	PORT_DIPSETTING(    0x00, DEF_STR(On) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR(Unknown) )
	PORT_DIPSETTING(    0x08, DEF_STR(Off) )
	PORT_DIPSETTING(    0x00, DEF_STR(On) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR(Unknown) )
	PORT_DIPSETTING(    0x10, DEF_STR(Off) )
	PORT_DIPSETTING(    0x00, DEF_STR(On) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR(Unknown) )
	PORT_DIPSETTING(    0x20, DEF_STR(Off) )
	PORT_DIPSETTING(    0x00, DEF_STR(On) )
	PORT_DIPNAME( 0x40, 0x40, "Use Backup Ram" )
	PORT_DIPSETTING(    0x00, DEF_STR(No) )
	PORT_DIPSETTING(    0x40, DEF_STR(Yes) )
	PORT_DIPNAME( 0x80, 0x80, "Freeze" )
	PORT_DIPSETTING(    0x00, "Freezes the game")
	PORT_DIPSETTING(    0x80, "Right value")

	PORT_START  /* Paddle D */
	PORT_ANALOG( 0xFF, 0x00, IPT_DIAL | IPF_REVERSE | IPF_PLAYER4, 100, 15, 0, 0 )

INPUT_PORTS_END

INPUT_PORTS_START( jjparads )

	PORT_START  /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )

	PORT_START  /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )

	PORT_START  /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START  /* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START  /* Paddle A */
	PORT_ANALOG( 0xFF, 0x00, IPT_DIAL | IPF_REVERSE | IPF_PLAYER1, 100, 15, 0, 0 )

	PORT_START  /* Paddle B */
	PORT_ANALOG( 0xFF, 0x00, IPT_DIAL | IPF_REVERSE | IPF_PLAYER2, 100, 15, 0, 0 )

	PORT_START  /* Paddle C */
	PORT_ANALOG( 0xFF, 0x00, IPT_DIAL | IPF_REVERSE | IPF_PLAYER3, 100, 15, 0, 0 )

	PORT_START  /* DSW */
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR(Flip_Screen) ) /* This port affects 0x040191c8 function*/
	PORT_DIPSETTING(    0x02, DEF_STR(Off) )
	PORT_DIPSETTING(    0x00, DEF_STR(On) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR(Unknown) )
	PORT_DIPSETTING(    0x04, DEF_STR(Off) )
	PORT_DIPSETTING(    0x00, DEF_STR(On) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR(Unknown) )
	PORT_DIPSETTING(    0x08, DEF_STR(Off) )
	PORT_DIPSETTING(    0x00, DEF_STR(On) )
	PORT_DIPNAME( 0x10, 0x10, "Test Mode" )
	PORT_DIPSETTING(    0x10, DEF_STR(Off) )
	PORT_DIPSETTING(    0x00, DEF_STR(On) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR(Unknown) )
	PORT_DIPSETTING(    0x20, DEF_STR(Off) )
	PORT_DIPSETTING(    0x00, DEF_STR(On) )
	PORT_DIPNAME( 0x40, 0x40, "Use Backup Ram" )
	PORT_DIPSETTING(    0x00, DEF_STR(No) )
	PORT_DIPSETTING(    0x40, DEF_STR(Yes) )
	PORT_DIPNAME( 0x80, 0x80, "Freeze" )
	PORT_DIPSETTING(    0x00, "Freezes the game")
	PORT_DIPSETTING(    0x80, "Right value")

	PORT_START  /* Paddle D */
	PORT_ANALOG( 0xFF, 0x00, IPT_DIAL | IPF_REVERSE | IPF_PLAYER4, 100, 15, 0, 0 )

INPUT_PORTS_END


static READ32_HANDLER( nova_input_port_0_r )
{
	return readinputport(0)<<24 | readinputport(1)<<16 | readinputport(2)<<8 | readinputport(3);
}

static READ32_HANDLER( nova_input_port_3_r )
{
	return readinputport(8) | 0xffffff00;
}

static READ32_HANDLER( nova_input_port_dip_r )
{
	return readinputport(4)<<24 | readinputport(5)<<16 | readinputport(6)<<8 | readinputport(7);
}

static UINT32 timer_0_temp[4];

static WRITE32_HANDLER( msm6242_w )
{
	COMBINE_DATA(&timer_0_temp[offset]);

	if(offset>=4)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Timer 0 outbound\n");
		return;
	}
}

static WRITE32_HANDLER( skns_io_w )
{
	switch(offset) {
	case 2:
		if(((mem_mask & 0xff000000) == 0))
		{ /* Coin Lock/Count */
/*			coin_counter_w(0, data & 0x01000000);*/
/*			coin_counter_w(1, data & 0x02000000);*/
/*			coin_lockout_w(0, ~data & 0x04000000);*/
/*			coin_lockout_w(1, ~data & 0x08000000); */ /* Works in puzzloop, others behave strange.*/
		}
		if(((mem_mask & 0x00ff0000) == 0))
		{ /* Analogue Input Select */
		}
		if(((mem_mask & 0x0000ff00) == 0))
		{ /* Extended Output - Port A, Mahjong inputs, Comms etc. */
		}
		if(((mem_mask & 0x000000ff) == 0))
		{ /* Extended Output - Port B */
		}
	break;
	case 3:
		if(((mem_mask & 0x0000ff00) == 0))
		{ /* Interrupt Clear, do we need these? */
/*			if(data&0x01)
				cpu_set_irq_line(0,1,CLEAR_LINE);
			if(data&0x02)
				cpu_set_irq_line(0,3,CLEAR_LINE);
			if(data&0x04)
				cpu_set_irq_line(0,5,CLEAR_LINE);
			if(data&0x08)
				cpu_set_irq_line(0,7,CLEAR_LINE);
			if(data&0x10)
				cpu_set_irq_line(0,9,CLEAR_LINE);
			if(data&0x20)
				cpu_set_irq_line(0,0xb,CLEAR_LINE);
			if(data&0x40)
				cpu_set_irq_line(0,0xd,CLEAR_LINE);
			if(data&0x80)
				cpu_set_irq_line(0,0xf,CLEAR_LINE);*/

			/* idle skip for vblokbrk/sarukani, i can't find a better place to put it :-( but i think it works ok unless its making the game too fast */
			if (activecpu_get_pc()==0x04013B44)
			{
				if (!strcmp(Machine->gamedrv->name,"vblokbrk") ||
					!strcmp(Machine->gamedrv->name,"sarukani"))
					cpu_spinuntil_int();
			}

		}
		else
		{
			log_cb(RETRO_LOG_DEBUG, LOGPRE "Unk IO Write memmask:%08x offset:%08x data:%08x\n", mem_mask, offset, data);
		}
	break;
	default:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Unk IO Write memmask:%08x offset:%08x data:%08x\n", mem_mask, offset, data);
	break;
	}
}

extern void *record;
extern void *playback;

static READ32_HANDLER( msm6242_r )
{
	struct tm *tm;
	time_t tms;
	long value;

	if(record != 0 || playback != 0)
		return 0;

	time(&tms);
	tm = localtime(&tms);
	/* The clock is not y2k-compatible, wrap back 10 years, screw the leap years*/
	/*  tm->tm_year -= 10;*/

	switch(offset) {
	case 0:
		value  = (tm->tm_sec % 10)<<24;
		value |= (tm->tm_sec / 10)<<16;
		value |= (tm->tm_min % 10)<<8;
		value |= (tm->tm_min / 10);
		break;
	case 1:
		value  = (tm->tm_hour % 10)<<24;
		value |= ((tm->tm_hour / 10) /*| (tm->tm_hour >= 12 ? 4 : 0)*/)<<16;
		value |= (tm->tm_mday % 10)<<8;
		value |= (tm->tm_mday / 10);
		break;
	case 2:
		value  = ((tm->tm_mon + 1) % 10)<<24;
		value |= ((tm->tm_mon + 1) / 10)<<16;
		value |= (tm->tm_year % 10)<<8;
		value |= ((tm->tm_year / 10) % 10);
		break;
	case 3:
	default:
		value  = (tm->tm_wday)<<24;
		value |= (1)<<16;
		value |= (6)<<8;
		value |= (4);
		break;
	}
	return value;
}

/* end old driver code */
static void palette_set_rgb_brightness (int offset, UINT8 brightness_r, UINT8 brightness_g, UINT8 brightness_b)
{
	int use_bright, r, g, b, alpha;

	b = ((skns_palette_ram[offset] >> 0  ) & 0x1f);
	g = ((skns_palette_ram[offset] >> 5  ) & 0x1f);
	r = ((skns_palette_ram[offset] >> 10  ) & 0x1f);

	alpha = ((skns_palette_ram[offset] >> 15  ) & 0x1);

	if(offset<(0x40*256)) { /* 1st half is for Sprites*/
		use_bright = use_spc_bright;
	} else { /* V3 bg's*/
		use_bright = use_v3_bright;
	}

	if(use_bright) {
		if(brightness_b) b = ((b<<3) * (brightness_b+1))>>8;
		else b = 0;
		if(brightness_g) g = ((g<<3) * (brightness_g+1))>>8;
		else g = 0;
		if(brightness_r) r = ((r<<3) * (brightness_r+1))>>8;
		else r = 0;
	} else {
		b <<= 3;
		g <<= 3;
		r <<= 3;
	}

	palette_set_color(offset,r,g,b);
}

/* This ignores the alpha values atm.*/
static int spc_changed=0, v3_changed=0, palette_updated=0;

static WRITE32_HANDLER ( skns_pal_regs_w )
{
	COMBINE_DATA(&skns_pal_regs[offset]);
	palette_updated =1;

	switch ( offset )
	{
	case (0x00/4): /* RWRA0*/
		if( use_spc_bright != (data&1) ) {
			use_spc_bright = data&1;
			spc_changed = 1;
		}
		break;
	case (0x04/4): /* RWRA1*/
		if( bright_spc_g != (data&0xff) ) {
			bright_spc_g = data&0xff;
			spc_changed = 1;
		}
		break;
	case (0x08/4): /* RWRA2*/
		if( bright_spc_r != (data&0xff) ) {
			bright_spc_r = data&0xff;
			spc_changed = 1;
		}
		break;
	case (0x0C/4): /* RWRA3*/
		if( bright_spc_b != (data&0xff) ) {
			bright_spc_b = data&0xff;
			spc_changed = 1;
		}
		break;

	case (0x10/4): /* RWRB0*/
		if( use_v3_bright != (data&1) ) {
			use_v3_bright = data&1;
			v3_changed = 1;
		}
		break;
	case (0x14/4): /* RWRB1*/
		if( bright_v3_g != (data&0xff) ) {
			bright_v3_g = data&0xff;
			v3_changed = 1;
		}
		break;
	case (0x18/4): /* RWRB2*/
		if( bright_v3_r != (data&0xff) ) {
			bright_v3_r = data&0xff;
			v3_changed = 1;
		}
		break;
	case (0x1C/4): /* RWRB3*/
		if( bright_v3_b != (data&0xff) ) {
			bright_v3_b = data&0xff;
			v3_changed = 1;
		}
		break;
	}
}

void skns_palette_update(void)
{
	int i;
	
	if (palette_updated)
	{
		if(spc_changed)
			for(i=0; i<=((0x40*256)-1); i++)
				palette_set_rgb_brightness (i, bright_spc_r, bright_spc_g, bright_spc_b);
			
		if(v3_changed)
			for(i=(0x40*256); i<=((0x80*256)-1); i++)
				palette_set_rgb_brightness (i, bright_v3_r, bright_v3_g, bright_v3_b);
		palette_updated =0;
	}

}


static WRITE32_HANDLER ( skns_palette_ram_w )
{
	int r,g,b;
	int brightness_r, brightness_g, brightness_b, alpha;
	int use_bright;

	COMBINE_DATA(&skns_palette_ram[offset]);

	b = ((skns_palette_ram[offset] >> 0  ) & 0x1f);
	g = ((skns_palette_ram[offset] >> 5  ) & 0x1f);
	r = ((skns_palette_ram[offset] >> 10  ) & 0x1f);

	alpha = ((skns_palette_ram[offset] >> 15  ) & 0x1);

	if(offset<(0x40*256)) { /* 1st half is for Sprites*/
		use_bright = use_spc_bright;
		brightness_b = bright_spc_b;
		brightness_g = bright_spc_g;
		brightness_r = bright_spc_r;
	} else { /* V3 bg's*/
		use_bright = use_v3_bright;
		brightness_b = bright_v3_b;
		brightness_g = bright_v3_g;
		brightness_r = bright_v3_r;
	}

	if(use_bright) {
		if(brightness_b) b = ((b<<3) * (brightness_b+1))>>8;
		else b = 0;
		if(brightness_g) g = ((g<<3) * (brightness_g+1))>>8;
		else g = 0;
		if(brightness_r) r = ((r<<3) * (brightness_r+1))>>8;
		else r = 0;
	} else {
		b <<= 3;
		g <<= 3;
		r <<= 3;
	}

	palette_set_color(offset,r,g,b);
}

static WRITE32_HANDLER( skns_v3t_w )
{
	data8_t *btiles = memory_region(REGION_GFX3);

	COMBINE_DATA(&skns_v3t_ram[offset]);

	skns_v3t_dirty[offset/0x40] = 1;
	skns_v3t_somedirty = 1;
	skns_v3t_4bppdirty[offset/0x20] = 1;
	skns_v3t_4bpp_somedirty = 1;

	data = skns_v3t_ram[offset];
/* i think we need to swap around to decode .. endian issues?*/
	btiles[offset*4+0] = (data & 0xff000000) >> 24;
	btiles[offset*4+1] = (data & 0x00ff0000) >> 16;
	btiles[offset*4+2] = (data & 0x0000ff00) >> 8;
	btiles[offset*4+3] = (data & 0x000000ff) >> 0;
}

static WRITE32_HANDLER( skns_ymz280_w )
{
	if ((mem_mask & 0xff000000) == 0)
		YMZ280B_register_0_w(offset,(data >> 24) & 0xff);
	if ((mem_mask & 0x00ff0000) == 0)
		YMZ280B_data_0_w(offset,(data >> 16) & 0xff);
}

static MEMORY_READ32_START( skns_readmem )
	{ 0x00000000, 0x0007ffff, MRA32_ROM       }, /* BIOS ROM */
	{ 0x00400000, 0x00400003, nova_input_port_0_r } ,
	{ 0x00400004, 0x00400007, nova_input_port_dip_r } ,
	{ 0x00400008, 0x0040000b, MRA32_RAM } ,
	/* In between is write only */
	{ 0x0040000c, 0x0040000f, nova_input_port_3_r } ,
	{ 0x00800000, 0x00801fff, MRA32_RAM       }, /* 'backup' RAM */
/*	{ 0x00c00000, 0x00c00003, skns_ymz280_r   },  // ymz280 (sound) /*/
	{ 0x01000000, 0x0100000f, msm6242_r       } ,
	{ 0x02000000, 0x02003fff, MRA32_RAM       }, /* 'spc' RAM */
	{ 0x02100000, 0x0210003f, MRA32_RAM       }, /* 'spc' */
	{ 0x02400000, 0x0240007f, MRA32_RAM       }, /* 'v3' */
    { 0x02500000, 0x02507fff, MRA32_RAM       }, /* 'v3tbl' RAM */
    { 0x02600000, 0x02607fff, MRA32_RAM       }, /* 'v3slc' RAM */
	{ 0x02a00000, 0x02a0001f, MRA32_RAM       }, /* skns_pal_regs */
    { 0x02a40000, 0x02a5ffff, MRA32_RAM       }, /* 'palette' RAM */
	{ 0x02f00000, 0x02f000ff, skns_hit_r      }, /* hit */
	{ 0x04000000, 0x041fffff, MRA32_BANK1     }, /* GAME ROM */
	{ 0x04800000, 0x0483ffff, MRA32_RAM       }, /* 'v3t' RAM */
	{ 0x06000000, 0x060fffff, MRA32_RAM       }, /* 'main' RAM */
	{ 0xc0000000, 0xc0000fff, MRA32_RAM       }, /* 'cache' RAM */
MEMORY_END

static MEMORY_WRITE32_START( skns_writemem )
	{ 0x00000000, 0x0007ffff, MWA32_ROM }, /* BIOS ROM */
	{ 0x00400000, 0x0040000f, skns_io_w }, /* I/O Write */
	{ 0x00800000, 0x00801fff, MWA32_RAM, (data32_t **)&generic_nvram, &generic_nvram_size }, /* 'backup' RAM */
	{ 0x00c00000, 0x00c00003, skns_ymz280_w }, /* ymz280_w (sound) */
	{ 0x01000000, 0x0100000f, msm6242_w } ,
	{ 0x01800000, 0x01800003, skns_hit2_w },
	{ 0x02000000, 0x02003fff, MWA32_RAM, &spriteram32, &spriteram_size }, /* sprite ram */
	{ 0x02100000, 0x0210003f, MWA32_RAM, &skns_spc_regs  }, /* sprite registers */
	{ 0x02400000, 0x0240007f, skns_v3_regs_w, &skns_v3_regs }, /* tilemap registers */
	{ 0x02500000, 0x02503fff, skns_tilemapA_w, &skns_tilemapA_ram }, /* tilemap A */
	{ 0x02504000, 0x02507fff, skns_tilemapB_w, &skns_tilemapB_ram }, /* tilemap B */
	{ 0x02600000, 0x02607fff, MWA32_RAM, &skns_v3slc_ram }, /* tilemap linescroll */
	{ 0x02a00000, 0x02a0001f, skns_pal_regs_w, &skns_pal_regs },
	{ 0x02a40000, 0x02a5ffff, skns_palette_ram_w, &skns_palette_ram },
	{ 0x02f00000, 0x02f000ff, skns_hit_w },
	{ 0x04000000, 0x041fffff, MWA32_ROM }, /* GAME ROM */
	{ 0x04800000, 0x0483ffff, skns_v3t_w, &skns_v3t_ram }, /* tilemap b ram based tiles */
	{ 0x06000000, 0x060fffff, MWA32_RAM, &skns_main_ram },
	{ 0xc0000000, 0xc0000fff, MWA32_RAM, &skns_cache_ram }, /* 'cache' RAM */
MEMORY_END

/***** GFX DECODE *****/

static struct GfxLayout skns_tilemap_layout =
{
	16,16,
	RGN_FRAC(1,1),
	8,
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	{ 0*128, 1*128, 2*128, 3*128, 4*128, 5*128, 6*128, 7*128,
	  8*128, 9*128, 10*128, 11*128, 12*128, 13*128, 14*128, 15*128 },
	16*16*8
};

static struct GfxLayout skns_4bpptilemap_layout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3  },
	{ 1*4, 0*4, 3*4, 2*4, 5*4, 4*4, 7*4, 6*4,
	 9*4, 8*4, 11*4, 10*4, 13*4, 12*4, 15*4, 14*4 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
	  8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	16*16*4
};

static struct GfxDecodeInfo skns_bg_decode[] =
{
   /* REGION_GFX1 is sprites, RLE encoded */
	{ REGION_GFX2, 0, &skns_tilemap_layout, 0x000, 128 },
	{ REGION_GFX3, 0, &skns_tilemap_layout, 0x000, 128 },
	{ REGION_GFX2, 0, &skns_4bpptilemap_layout, 0x000, 128 },
	{ REGION_GFX3, 0, &skns_4bpptilemap_layout, 0x000, 128 },
	{ -1 }
};

/***** MACHINE DRIVER *****/

static struct YMZ280Binterface ymz280b_intf =
{
	1,
	{ 33333333/2 },
	{ REGION_SOUND1 },
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) },
	{ 0 }	/* irq ?*/
};

static MACHINE_DRIVER_START(skns)
	MDRV_CPU_ADD(SH2,28638000)
	MDRV_CPU_MEMORY(skns_readmem,skns_writemem)
	MDRV_CPU_VBLANK_INT(skns_interrupt,2)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(skns)
	MDRV_NVRAM_HANDLER(generic_1fill)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_BUFFERS_SPRITERAM)
	MDRV_SCREEN_SIZE(64*16,64*16)
	MDRV_VISIBLE_AREA(0,319,0,239)
	MDRV_PALETTE_LENGTH(32768)
	MDRV_GFXDECODE(skns_bg_decode)

	MDRV_VIDEO_START(skns)
	MDRV_VIDEO_EOF(skns)
	MDRV_VIDEO_UPDATE(skns)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YMZ280B, ymz280b_intf)
MACHINE_DRIVER_END

/***** BIOS SKIPPING *****/

static READ32_HANDLER( bios_skip_r )
{
#if BIOS_SKIP
	if (activecpu_get_pc()==0x6fa) cpu_writemem32bedw(0x06000029,1);
#endif
	return skns_main_ram[0x00028/4];
}

/***** IDLE SKIPPING *****/

static READ32_HANDLER( gutsn_speedup_r )
{
/*
	0402206A: MOV.L   @($8C,PC),R5
	0402206C: MOV.L   @($8C,PC),R1
	0402206E: MOV.L   @R1,R2       // R1 == 600C780
	04022070: MOV.L   @R5,R3       // R5 == 6000078
	04022072: CMP/EQ  R2,R3
	04022074: BT      $0402206C
*/
	if (activecpu_get_pc()==0x4022070)
	{
		if(skns_main_ram[0x00078/4] == skns_main_ram[0x0c780/4])
			cpu_spinuntil_int();
	}
	return skns_main_ram[0x0c780/4];
}

static READ32_HANDLER( cyvern_speedup_r )
{
	if (activecpu_get_pc()==0x402ebd4) cpu_spinuntil_int();
	return skns_main_ram[0x4d3c8/4];
}

static READ32_HANDLER( puzloopj_speedup_r )
{
	if (activecpu_get_pc()==0x401dca2) cpu_spinuntil_int();
	return skns_main_ram[0x86714/4];
}

static READ32_HANDLER( puzloopu_speedup_r )
{
	if (activecpu_get_pc()==0x401dab2) cpu_spinuntil_int();
	return skns_main_ram[0x85cec/4];
}

static READ32_HANDLER( puzzloop_speedup_r )
{
/*
	0401DA12: MOV.L   @($80,PC),R1
	0401DA14: MOV.L   @R1,R0 (R1=0x6081d38)
	0401DA16: TST     R0,R0
	0401DA18: BF      $0401DA26
	0401DA26: BRA     $0401DA12
*/
	if (activecpu_get_pc()==0x401da16) cpu_spinuntil_int();
	return skns_main_ram[0x81d38/4];
}

static READ32_HANDLER( senknow_speedup_r )
{
	if (activecpu_get_pc()==0x4017dd0) cpu_spinuntil_int();
	return skns_main_ram[0x0000dc/4];
}

static READ32_HANDLER( teljan_speedup_r )
{
	if (activecpu_get_pc()==0x401ba34) cpu_spinuntil_int();
	return skns_main_ram[0x002fb4/4];
}

static READ32_HANDLER( jjparads_speedup_r )
{
	if (activecpu_get_pc()==0x4015e86) cpu_spinuntil_int();
	return skns_main_ram[0x000994/4];
}

static READ32_HANDLER( jjparad2_speedup_r )
{
	if (activecpu_get_pc()==0x401620c) cpu_spinuntil_int();
	return skns_main_ram[0x000984/4];
}

static READ32_HANDLER( ryouran_speedup_r )
{
	if (activecpu_get_pc()==0x40182d0) cpu_spinuntil_int();
	return skns_main_ram[0x000a14/4];
}

static READ32_HANDLER( galpans2_speedup_r )
{
	if (activecpu_get_pc()==0x4049ae4) cpu_spinuntil_int();
	return skns_main_ram[0x0fb6bc/4];
}

static READ32_HANDLER( panicstr_speedup_r )
{
	if (activecpu_get_pc()==0x404e68c) cpu_spinuntil_int();
	return skns_main_ram[0x0f19e4/4];
}

static READ32_HANDLER( sengekis_speedup_r ) /* 60006ee  600308e*/
{
	if (activecpu_get_pc()==0x60006ee) cpu_spinuntil_int();
	return skns_main_ram[0xb74bc/4];
}

static READ32_HANDLER( sengekij_speedup_r ) /* 60006ee  600308e*/
{
	if (activecpu_get_pc()==0x60006ee) cpu_spinuntil_int();
	return skns_main_ram[0xb7380/4];
}

static DRIVER_INIT( skns )     { install_mem_read32_handler(0, 0x6000028, 0x600002b, bios_skip_r );  }
static DRIVER_INIT( galpani4 ) { skns_sprite_kludge(-5,-1); init_skns();  } /* Idle Loop caught by sh-2 core*/
static DRIVER_INIT( galpanis ) { skns_sprite_kludge(-5,-1); init_skns();  } /* Idle Loop caught by sh-2 core*/
static DRIVER_INIT( galpandx ) { skns_sprite_kludge(-5,-1); init_skns();  } /* Idle Loop caught by sh-2 core*/
static DRIVER_INIT( cyvern )   { skns_sprite_kludge(+0,+2); init_skns(); install_mem_read32_handler(0, 0x604d3c8, 0x604d3cb, cyvern_speedup_r );   }
static DRIVER_INIT( galpans2 ) { skns_sprite_kludge(-1,-1); init_skns(); install_mem_read32_handler(0, 0x60fb6bc, 0x60fb6bf, galpans2_speedup_r );  }
static DRIVER_INIT( gutsn )    { skns_sprite_kludge(+0,+0); init_skns(); install_mem_read32_handler(0, 0x600c780, 0x600c783, gutsn_speedup_r );  }
static DRIVER_INIT( panicstr ) { skns_sprite_kludge(-1,-1); init_skns(); install_mem_read32_handler(0, 0x60f19e4, 0x60f19e7, panicstr_speedup_r );  }
static DRIVER_INIT( senknow )  { skns_sprite_kludge(+1,+1); init_skns(); install_mem_read32_handler(0, 0x60000dc, 0x60000df, senknow_speedup_r );  }
static DRIVER_INIT( puzzloop ) { skns_sprite_kludge(-9,-1); init_skns(); install_mem_read32_handler(0, 0x6081d38, 0x6081d3b, puzzloop_speedup_r ); }
static DRIVER_INIT( puzloopj ) { skns_sprite_kludge(-9,-1); init_skns(); install_mem_read32_handler(0, 0x6086714, 0x6086717, puzloopj_speedup_r ); }
static DRIVER_INIT( puzloopu ) { skns_sprite_kludge(-9,-1); init_skns(); install_mem_read32_handler(0, 0x6085cec, 0x6085cef, puzloopu_speedup_r ); }
static DRIVER_INIT( jjparads ) { skns_sprite_kludge(+5,+1); init_skns(); install_mem_read32_handler(0, 0x6000994, 0x6000997, jjparads_speedup_r );  }
static DRIVER_INIT( jjparad2 ) { skns_sprite_kludge(+5,+1); init_skns(); install_mem_read32_handler(0, 0x6000984, 0x6000987, jjparad2_speedup_r );  }
static DRIVER_INIT( ryouran )  { skns_sprite_kludge(+5,+1); init_skns(); install_mem_read32_handler(0, 0x6000a14, 0x6000a17, ryouran_speedup_r );  }
static DRIVER_INIT( teljan )   { skns_sprite_kludge(+5,+1); init_skns(); install_mem_read32_handler(0, 0x6002fb4, 0x6002fb7, teljan_speedup_r );  }
static DRIVER_INIT( sengekis ) { skns_sprite_kludge(-192,-272); init_skns(); install_mem_read32_handler(0, 0x60b74bc, 0x60b74bf, sengekis_speedup_r ); }
static DRIVER_INIT( sengekij ) { skns_sprite_kludge(-192,-272); init_skns(); install_mem_read32_handler(0, 0x60b7380, 0x60b7383, sengekij_speedup_r ); }
static DRIVER_INIT( sarukani ) { skns_sprite_kludge(-1,-1); init_skns();  } /* Speedup is in skns_io_w()*/



/***** ROM LOADING *****/

ROM_START( skns )
	ROM_REGION( 0x0100000, REGION_CPU1, 0 ) /* SH-2 Code */
	ROM_LOAD       ( "sknsj1.u10", 0x000000, 0x080000, CRC(7e2b836c) SHA1(92c5a7a2472496028bff0e5980d41dd294f42144) ) /* Japan BIOS */
	ROM_LOAD       ( "sknse1.u10", 0x000000, 0x080000, CRC(e2b9d7d1) SHA1(b530a3bb9dedc8cfafcba9f1f10277590be04a15) ) /* Europ BIOS */
	ROM_LOAD       ( "sknsa1.u10", 0x000000, 0x080000, CRC(745e5212) SHA1(caba649ab2d83b2d7e007eecee0fc582c019df38) ) /* Asia  BIOS */
	ROM_LOAD       ( "sknsu1.u10", 0x000000, 0x080000, CRC(384d21ec) SHA1(a27e8a18099d9cea64fa32db28d01101c2a78815) ) /* USA   BIOS */
ROM_END

ROM_START( cyvern )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH-2 Code */
	ROM_LOAD       ( "sknsj1.u10",   0x000000, 0x080000, CRC(7e2b836c) SHA1(92c5a7a2472496028bff0e5980d41dd294f42144) ) /* Japan BIOS */

	ROM_REGION32_BE( 0x200000, REGION_USER1, 0 ) /* SH-2 Code mapped at 0x04000000 */
	ROM_LOAD16_BYTE( "cvj-even.u10", 0x000000, 0x100000, CRC(802fadb4) SHA1(cbfac3a87a4863466117c61f2ecaf63d506352f6) )
	ROM_LOAD16_BYTE( "cvj-odd.u8",   0x000001, 0x100000, CRC(f8a0fbdd) SHA1(5cc8c12c13b5eb3456083e70100450ba041de76e) )

	ROM_REGION( 0x800000, REGION_GFX1, 0 ) /* Sprites */
	ROM_LOAD( "cv100-00.u24", 0x000000, 0x400000, CRC(cd4ae88a) SHA1(925f4ae01a6ad3633be2a61be69e163f05401cf6) )
	ROM_LOAD( "cv101-00.u20", 0x400000, 0x400000, CRC(a6cb3f0b) SHA1(8d83f44a096ca0a70962ca4c602c4331874c8560) )

	ROM_REGION( 0x800000, REGION_GFX2, ROMREGION_DISPOSE ) /* Tiles Plane A */
	ROM_LOAD( "cv200-00.u16", 0x000000, 0x400000, CRC(ddc8c67e) SHA1(9b99e87e69e88011e6d693d19ac5e115b4fa50b0) )
	ROM_LOAD( "cv201-00.u13", 0x400000, 0x400000, CRC(65863321) SHA1(b8b75f50406068ffc3fca3887d2f0a653ca491c9) )

	ROM_REGION( 0x800000, REGION_GFX3, 0 ) /* Tiles Plane B */
	/* First 0x040000 bytes (0x03ff Tiles) are RAM Based Tiles */
	/* 0x040000 - 0x3fffff empty? */
	ROM_LOAD( "cv210-00.u18", 0x400000, 0x400000, CRC(7486bf3a) SHA1(3b4285ca570e9c5ad396c615bfc054372d1b0162) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "cv300-00.u4", 0x000000, 0x400000, CRC(fbeda465) SHA1(4d5066a22f4589b6b7f85b3e77c348d900ac4bdd) )
ROM_END

ROM_START( galpani4 )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH-2 Code */
	ROM_LOAD       ( "sknsj1.u10",   0x000000, 0x080000, CRC(7e2b836c) SHA1(92c5a7a2472496028bff0e5980d41dd294f42144) ) /* Japan BIOS */

	ROM_REGION32_BE( 0x200000, REGION_USER1, 0 ) /* SH-2 Code mapped at 0x04000000 */
    ROM_LOAD16_BYTE( "gp4j1.u10",    0x000000, 0x080000, CRC(919a3893) SHA1(83b89a9e628a1f46f8a56ea512fc8ad641d5e239) )
    ROM_LOAD16_BYTE( "gp4j1.u08",    0x000001, 0x080000, CRC(94cb1fb7) SHA1(ac90103dd43cdce6a287ffc13631c1de477a9a71) )

	ROM_REGION( 0x400000, REGION_GFX1, 0 )
    ROM_LOAD( "gp410000.u24", 0x000000, 0x200000, CRC(1df61f01) SHA1(a9e95bbb3013e8f2fd01243b1b392ff07b4f7d02) )
    ROM_LOAD( "gp410100.u20", 0x200000, 0x100000, CRC(8e2c9349) SHA1(a58fa9bcc9684ed4558e3395d592b64a1978a902) )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE )
    ROM_LOAD( "gp420000.u16", 0x000000, 0x200000, CRC(f0781376) SHA1(aeab9553a9af922524e528eb2d019cf36b6e2094) )
    ROM_LOAD( "gp420100.u18", 0x200000, 0x200000, CRC(10c4b183) SHA1(80e05f3932495ad4fc9bf928fa66e6d2931bbb06) )

	ROM_REGION( 0x800000, REGION_GFX3, 0 ) /* Tiles Plane B */
	/* First 0x040000 bytes (0x03ff Tiles) are RAM Based Tiles */
	/* 0x040000 - 0x3fffff empty? */

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Samples */
    ROM_LOAD( "gp430000.u4", 0x000000, 0x200000, CRC(8374663a) SHA1(095512564f4de25dc3752d9fbd254b9dabd16d1b) )
ROM_END

ROM_START( galpandx )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH-2 Code */
	ROM_LOAD  ( "sknsa1.u10", 0x000000, 0x080000, CRC(745e5212) SHA1(caba649ab2d83b2d7e007eecee0fc582c019df38) ) /* Asia BIOS */

	ROM_REGION32_BE( 0x200000, REGION_USER1, 0 ) /* SH-2 Code mapped at 0x04000000 */
	ROM_LOAD16_BYTE( "gpdx.u10", 0x000000, 0x100000, CRC(8eca883d) SHA1(f5a102ac21aeebc44afcd0ef7f655de21d4442b2) )
	ROM_LOAD16_BYTE( "gpdx.u8",  0x000001, 0x100000, CRC(b0088d8f) SHA1(2363620265d3fc53bae1c5889ea761444f80735a) )

	ROM_REGION( 0x400000, REGION_GFX1, 0 )
    ROM_LOAD( "gp410000.u24", 0x000000, 0x200000, CRC(1df61f01) SHA1(a9e95bbb3013e8f2fd01243b1b392ff07b4f7d02) )
    ROM_LOAD( "gp410100.u20", 0x200000, 0x100000, CRC(8e2c9349) SHA1(a58fa9bcc9684ed4558e3395d592b64a1978a902) )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE )
    ROM_LOAD( "gp420000.u16", 0x000000, 0x200000, CRC(f0781376) SHA1(aeab9553a9af922524e528eb2d019cf36b6e2094) )
    ROM_LOAD( "gp420100.u18", 0x200000, 0x200000, CRC(10c4b183) SHA1(80e05f3932495ad4fc9bf928fa66e6d2931bbb06) )

	ROM_REGION( 0x800000, REGION_GFX3, 0 ) /* Tiles Plane B */
	/* First 0x040000 bytes (0x03ff Tiles) are RAM Based Tiles */
	/* 0x040000 - 0x3fffff empty? */
	
	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "gp430000.u4",   0x000000, 0x200000, CRC(8374663a) SHA1(095512564f4de25dc3752d9fbd254b9dabd16d1b) ) /* Doesn't seem to use these samples at all */
	ROM_LOAD( "gp4-301-01.u7", 0x200000, 0x200000, CRC(886ef77f) SHA1(047d5fecf2034339c69b2cb605b623a814a18f0d) ) /* Changed some samples when compared to U4 rom  */
ROM_END

ROM_START( galpanis )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH-2 Code */
	ROM_LOAD       ( "sknsj1.u10",   0x000000, 0x080000, CRC(7e2b836c) SHA1(92c5a7a2472496028bff0e5980d41dd294f42144) ) /* Japan BIOS */

	ROM_REGION32_BE( 0x200000, REGION_USER1, 0 ) /* SH-2 Code mapped at 0x04000000 */
    ROM_LOAD16_BYTE( "gps-e.u10",    0x000000, 0x100000, CRC(c6938c3f) SHA1(05853ee6a44a55702788a75580b04a4be45e9bcb) )
    ROM_LOAD16_BYTE( "gps-o.u8",     0x000001, 0x100000, CRC(e764177a) SHA1(3a1333eb1022ed1a275b9c3d44b5f4ab81618fb6) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
    ROM_LOAD( "gps10000.u24", 0x000000, 0x400000, CRC(a1a7acf2) SHA1(52c86ae907f0c0236808c19f652955b09e90ec5a) )
    ROM_LOAD( "gps10100.u20", 0x400000, 0x400000, CRC(49f764b6) SHA1(9f4289858c3dac625ef623cc381a47b45aa5d8e2) )
    ROM_LOAD( "gps10200.u17", 0x800000, 0x400000, CRC(51980272) SHA1(6c0706d913b33995579aaf0688c4bf26d6d35a78) )

	ROM_REGION( 0x800000, REGION_GFX2, ROMREGION_DISPOSE )
    ROM_LOAD( "gps20000.u16", 0x000000, 0x400000, CRC(c146a09e) SHA1(5af5a7b9d9a55ec7aba3fd85a3a0211b92b1b84f) )
    ROM_LOAD( "gps20100.u13", 0x400000, 0x400000, CRC(9dfa2dc6) SHA1(a058c42fd76c23c0e5c8c11f5617fd29e056be7d) )

	ROM_REGION( 0x800000, REGION_GFX3, 0 ) /* Tiles Plane B */
	/* First 0x040000 bytes (0x03ff Tiles) are RAM Based Tiles */
	/* 0x040000 - 0x3fffff empty? */

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Samples */
    ROM_LOAD( "gps30000.u4", 0x000000, 0x400000, CRC(9e4da8e3) SHA1(6506d9300a442883357003a05fd2c78d364c35bb) )
ROM_END

ROM_START( galpans2 )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH-2 Code */
	ROM_LOAD       ( "sknsj1.u10",   0x000000, 0x080000, CRC(7e2b836c) SHA1(92c5a7a2472496028bff0e5980d41dd294f42144) ) /* Japan BIOS */

	ROM_REGION32_BE( 0x200000, REGION_USER1, 0 ) /* SH-2 Code mapped at 0x04000000 */
    ROM_LOAD16_BYTE( "gps2j.u6",     0x000000, 0x100000, CRC(6e74005b) SHA1(a57e8307062e262c2e7a84e2c58f7dfe03fc0f78) )
    ROM_LOAD16_BYTE( "gps2j.u4",     0x000001, 0x100000, CRC(9b4b2304) SHA1(0b481f4d71d92bf23f38ed22979efd4409004857) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
    ROM_LOAD( "gs210000.u21", 0x000000, 0x400000, CRC(294b2f14) SHA1(90cbd0acdaa2d89d208c28aae33ab57c03624089) )
    ROM_LOAD( "gs210100.u20", 0x400000, 0x400000, CRC(f75c5a9a) SHA1(3919643cee6c88185a1aa3c58c5bc80599bf734e) )
    ROM_LOAD( "gs210200.u8",  0x800000, 0x400000, CRC(25b4f56b) SHA1(f9a33d5ed54a04ecece3035e75508d191bbe74b1) )
    ROM_LOAD( "gs210300.u32", 0xc00000, 0x400000, CRC(db6d4424) SHA1(0a88dafd0ee2490ff2ef39ce8eb1931c41bdda42) )

	ROM_REGION( 0x800000, REGION_GFX2, ROMREGION_DISPOSE )
    ROM_LOAD( "gs220000.u17", 0x000000, 0x400000, CRC(5caae1c0) SHA1(8f77e4cf018d7290b2d804cbff9fccf0bf4d2404) )
    ROM_LOAD( "gs220100.u9",  0x400000, 0x400000, CRC(8d51f197) SHA1(19d2afab823ea179918e7bcbf4df2283e77570f0) )

	ROM_REGION( 0x800000, REGION_GFX3, 0 ) /* Tiles Plane B */
	/* First 0x040000 bytes (0x03ff Tiles) are RAM Based Tiles */
	/* 0x040000 - 0x3fffff empty? */
    ROM_LOAD( "gs221000.u3",  0x400000, 0x400000, CRC(58800a18) SHA1(5e6d55ecd12275662d6f59559e137b759f23fff6) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Samples */
      ROM_LOAD( "gs230000.u1",  0x000000, 0x400000, CRC(0348e8e1) SHA1(8a21c7e5cea0bc08a2595213d689c58c0251fdb5) )
ROM_END

ROM_START( gutsn )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH-2 Code */
	ROM_LOAD       ( "sknsj1.u10",   0x000000, 0x080000, CRC(7e2b836c) SHA1(92c5a7a2472496028bff0e5980d41dd294f42144) ) /* Japan BIOS */

	ROM_REGION32_BE( 0x200000, REGION_USER1, 0 ) /* SH-2 Code mapped at 0x04000000 */
    ROM_LOAD16_BYTE( "gts000j0.u6"  ,0x000000, 0x080000, CRC(8ee91310) SHA1(8dd918189fe445d79c7f028168862b852f70a6f2) )
    ROM_LOAD16_BYTE( "gts001j0.u4",  0x000001, 0x080000, CRC(80b8ee66) SHA1(4faf5f358ceee866f09bd81e63ba3ebd21bde835) )

	ROM_REGION( 0x400000, REGION_GFX1, 0 )
    ROM_LOAD( "gts10000.u24", 0x000000, 0x400000, CRC(1959979e) SHA1(92a68784664dd833ca6fcca1b15cd46b9365d081) )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE )
    ROM_LOAD( "gts20000.u16", 0x000000, 0x400000, CRC(c443aac3) SHA1(b0416a09ead26077e9276bae98d94eeb1cf86877) )

	ROM_REGION( 0x400000, REGION_GFX3, 0 ) /* Tiles Plane B */
	/* First 0x040000 bytes (0x03ff Tiles) are RAM Based Tiles */
	/* 0x040000 - 0x3fffff empty? */

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "gts30000.u4", 0x000000, 0x400000, CRC(8c169141) SHA1(41caea6fa644515f7417c84bdac599b13ad07e8c) )
ROM_END

ROM_START( panicstr )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH-2 Code */
	ROM_LOAD       ( "sknsj1.u10",   0x000000, 0x080000, CRC(7e2b836c) SHA1(92c5a7a2472496028bff0e5980d41dd294f42144) ) /* Japan BIOS */

	ROM_REGION32_BE( 0x200000, REGION_USER1, 0 ) /* SH-2 Code mapped at 0x04000000 */
    ROM_LOAD16_BYTE( "ps1000j0.u10", 0x000000, 0x100000, CRC(59645f89) SHA1(8da205c6e38899d6c637941700dd7eea56011c10) )
    ROM_LOAD16_BYTE( "ps1001j0.u8",  0x000001, 0x100000, CRC(c4722be9) SHA1(7009d320a80cfa7d80efc5fc915081914bc3c827) )

  	ROM_REGION( 0x800000, REGION_GFX1, 0 )
    ROM_LOAD( "ps-10000.u24", 0x000000, 0x400000, CRC(294b2f14) SHA1(90cbd0acdaa2d89d208c28aae33ab57c03624089) )
    ROM_LOAD( "ps110100.u20", 0x400000, 0x400000, CRC(e292f393) SHA1(b0914f7f0abf9f821f2592c289ea4e3b3e7f819a) )

  	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE )
    ROM_LOAD( "ps120000.u16", 0x000000, 0x400000, CRC(d772ac15) SHA1(6bf7b9bfccdcb7481b21fa2ab9b683d79033a192) )

  	ROM_REGION( 0x400000, REGION_GFX3, 0 ) /* Tiles Plane B */
	/* First 0x040000 bytes (0x03ff Tiles) are RAM Based Tiles */
	/* 0x040000 - 0x3fffff empty? */

  	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Samples */
    ROM_LOAD( "ps-30000.u4",  0x000000, 0x400000, CRC(2262e263) SHA1(73443e5f40f5c5c9bd41c6207fa6376072f0f65e) )
ROM_END

ROM_START( puzzloop )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH-2 Code */
	ROM_LOAD       ( "sknse1.u10", 0x000000, 0x080000, CRC(e2b9d7d1) SHA1(b530a3bb9dedc8cfafcba9f1f10277590be04a15) ) /* Europe BIOS */

	ROM_REGION32_BE( 0x200000, REGION_USER1, 0 ) /* SH-2 Code mapped at 0x04000000 */
	ROM_LOAD16_BYTE( "pl00e1.u6",  0x000000, 0x080000, CRC(273adc38) SHA1(37ca873342ba9fb9951114048a9cd255f73fe19c) )
	ROM_LOAD16_BYTE( "pl00e1.u4",  0x000001, 0x080000, CRC(14ac2870) SHA1(d1abcfd64d7c0ead67e879c40e1010453fd4da13) )

	ROM_REGION( 0x800000, REGION_GFX1, 0 )
	ROM_LOAD( "pzl10000.u24", 0x000000, 0x400000, CRC(35bf6897) SHA1(8a1f1f5234a61971a62401633de1dec1920fc4da) )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "pzl20000.u16", 0x000000, 0x400000, CRC(ff558e68) SHA1(69a50c8100edbf2d5d92ce14b3f079f76c544bdd) )

	ROM_REGION( 0x800000, REGION_GFX3, 0 ) /* Tiles Plane B */
	/* First 0x040000 bytes (0x03ff Tiles) are RAM Based Tiles */
	/* 0x040000 - 0x3fffff empty? */
	ROM_LOAD( "pzl21000.u18", 0x400000, 0x400000, CRC(c8b3be64) SHA1(6da9ca8b963ebf10df6bc02bd1bdc66392e2fa60) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Samples */
    ROM_LOAD( "pzl30000.u4", 0x000000, 0x400000, CRC(38604b8d) SHA1(1191cf48a6a7baa58e51509442b40ea67f5252d2) )
ROM_END

ROM_START( puzloopj )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH-2 Code */
	ROM_LOAD       ( "sknsj1.u10",   0x000000, 0x080000, CRC(7e2b836c) SHA1(92c5a7a2472496028bff0e5980d41dd294f42144) ) /* Japan BIOS */

	ROM_REGION32_BE( 0x200000, REGION_USER1, 0 ) /* SH-2 Code mapped at 0x04000000 */
	ROM_LOAD16_BYTE( "pl0j2u6.u10",  0x000000, 0x080000, CRC(23c3bf97) SHA1(77ea1f32bed5709a6ad5b250370f08cfe8036867) )
	ROM_LOAD16_BYTE( "pl0j2u4.u8",   0x000001, 0x080000, CRC(55b2a3cb) SHA1(d4cbe143fe2ad622af808cbd9eedffeff3b77e0d) )

	ROM_REGION( 0x800000, REGION_GFX1, 0 )
	ROM_LOAD( "pzl10000.u24", 0x000000, 0x400000, CRC(35bf6897) SHA1(8a1f1f5234a61971a62401633de1dec1920fc4da) )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "pzl20000.u16", 0x000000, 0x400000, CRC(ff558e68) SHA1(69a50c8100edbf2d5d92ce14b3f079f76c544bdd) )

	ROM_REGION( 0x800000, REGION_GFX3, 0 ) /* Tiles Plane B */
	/* First 0x040000 bytes (0x03ff Tiles) are RAM Based Tiles */
	/* 0x040000 - 0x3fffff empty? */
	ROM_LOAD( "pzl21000.u18", 0x400000, 0x400000, CRC(c8b3be64) SHA1(6da9ca8b963ebf10df6bc02bd1bdc66392e2fa60) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Samples */
    ROM_LOAD( "pzl30000.u4", 0x000000, 0x400000, CRC(38604b8d) SHA1(1191cf48a6a7baa58e51509442b40ea67f5252d2) )
ROM_END

ROM_START( puzloopu )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH-2 Code */
	ROM_LOAD       ( "sknsu1.u10",   0x000000, 0x080000, CRC(384d21ec) SHA1(a27e8a18099d9cea64fa32db28d01101c2a78815) ) /* US BIOS */

	ROM_REGION32_BE( 0x200000, REGION_USER1, 0 ) /* SH-2 Code mapped at 0x04000000 */
	ROM_LOAD16_BYTE( "puzloopu.u10",  0x000000, 0x080000, CRC(e6f3f82f) SHA1(ac61dc22fa3c1b1c2f3a41d3a8fb43938b77ca68) )
	ROM_LOAD16_BYTE( "puzloopu.u8",   0x000001, 0x080000, CRC(0d081d30) SHA1(ec0cdf120126104b9bb706f68c9ba9c3777dd69c) )

	ROM_REGION( 0x800000, REGION_GFX1, 0 )
	ROM_LOAD( "pzl10000.u24", 0x000000, 0x400000, CRC(35bf6897) SHA1(8a1f1f5234a61971a62401633de1dec1920fc4da) )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "pzl20000.u16", 0x000000, 0x400000, CRC(ff558e68) SHA1(69a50c8100edbf2d5d92ce14b3f079f76c544bdd) )

	ROM_REGION( 0x800000, REGION_GFX3, 0 ) /* Tiles Plane B */
	/* First 0x040000 bytes (0x03ff Tiles) are RAM Based Tiles */
	/* 0x040000 - 0x3fffff empty? */
	ROM_LOAD( "pzl21000.u18", 0x400000, 0x400000, CRC(c8b3be64) SHA1(6da9ca8b963ebf10df6bc02bd1bdc66392e2fa60) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Samples */
    ROM_LOAD( "pzl30000.u4", 0x000000, 0x400000, CRC(38604b8d) SHA1(1191cf48a6a7baa58e51509442b40ea67f5252d2) )
ROM_END

/* haven't even tried to run the ones below yet */

ROM_START( jjparads )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH-2 Code */
	ROM_LOAD       ( "sknsj1.u10",   0x000000, 0x080000, CRC(7e2b836c) SHA1(92c5a7a2472496028bff0e5980d41dd294f42144) ) /* Japan BIOS */

	ROM_REGION32_BE( 0x200000, REGION_USER1, 0 ) /* SH-2 Code mapped at 0x04000000 */
    ROM_LOAD16_BYTE( "jp1j1.u10",    0x000000, 0x080000, CRC(de2fb669) SHA1(229ff1ae0ec5bc77fc17642964e0bb0146594e86) )
    ROM_LOAD16_BYTE( "jp1j1.u8",     0x000001, 0x080000, CRC(7276efb1) SHA1(3edc265b5c02da7d21a2494a6dc2878fbad93f87) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
    ROM_LOAD( "jp100-00.u24", 0x000000, 0x400000, CRC(f31b2e95) SHA1(7e5bb518d4f6423785d3f9f2752a624a66b42469) )
    ROM_LOAD( "jp101-00.u20", 0x400000, 0x400000, CRC(70cc8c24) SHA1(a4805ce19f512b047829548b635e68690d714175) )
    ROM_LOAD( "jp102-00.u17", 0x800000, 0x400000, CRC(35401c1e) SHA1(38fe86a08555bb823b8d64ac043330aaaa6b8892) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
    ROM_LOAD( "jp200-00.u16", 0x000000, 0x200000, CRC(493d63db) SHA1(4b8fe7ff1ae14a914a675ce4072a4d9e5cfc08b0) )

	ROM_REGION( 0x400000, REGION_GFX3, 0 ) /* Tiles Plane B */
	/* First 0x040000 bytes (0x03ff Tiles) are RAM Based Tiles */
	/* 0x040000 - 0x3fffff empty? */

	ROM_REGION( 0x200000, REGION_SOUND1, 0 ) /* Samples */
    ROM_LOAD( "jp300-00.u4", 0x000000, 0x200000, CRC(7023fe46) SHA1(24a92133bc664d63b3be67c2ef11cd7b605ee7e8) )
ROM_END

ROM_START( jjparad2 )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH-2 Code */
	ROM_LOAD       ( "sknsj1.u10",   0x000000, 0x080000, CRC(7e2b836c) SHA1(92c5a7a2472496028bff0e5980d41dd294f42144) ) /* Japan BIOS */

	ROM_REGION32_BE( 0x200000, REGION_USER1, 0 ) /* SH-2 Code mapped at 0x04000000 */
    ROM_LOAD16_BYTE( "jp2000j1.u6",  0x000000, 0x080000, CRC(5d75e765) SHA1(33bcd8f929f6025b00df2ea783b13a391a28a5c3) )
    ROM_LOAD16_BYTE( "jp2001j1.u4",  0x000001, 0x080000, CRC(1771910a) SHA1(7ca9584d379d7b41f303a3ba861f943c570ad97c) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
    ROM_LOAD( "jp210000.u21", 0x000000, 0x400000, CRC(79a7e3d7) SHA1(bd0f8d01971e5895395f97f2520bcd03ab19d229) )
    ROM_LOAD( "jp210100.u20", 0x400000, 0x400000, CRC(42415e0c) SHA1(f7bff86d55fa9002fbd14e4c62f9d3df8faaf7d0) )
    ROM_LOAD( "jp210200.u8",  0x800000, 0x400000, CRC(26731745) SHA1(8939d36b82b10b1010e4b924e6b9fdd4742efe48) )

	ROM_REGION( 0x800000, REGION_GFX2, ROMREGION_DISPOSE )
    ROM_LOAD( "jp220000.u17", 0x000000, 0x400000, CRC(d0e71873) SHA1(c6ffba3624e6d4c2d4e12ef7d88a02cbc3867b18) )
    ROM_LOAD( "jp220100.u9",  0x400000, 0x400000, CRC(4c7d964d) SHA1(3352cd866a64466f4f5a990c2c5e3e28e7028a99) )

	ROM_REGION( 0x400000, REGION_GFX3, 0 ) /* Tiles Plane B */
	/* First 0x040000 bytes (0x03ff Tiles) are RAM Based Tiles */
	/* 0x040000 - 0x3fffff empty? */

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Samples */
    ROM_LOAD( "jp230000.u1", 0x000000, 0x400000, CRC(73e30d7f) SHA1(af5b16cec722dbbf0e03d73edfa133dbf10ac4f3) )
ROM_END

ROM_START( sengekis )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH-2 Code */
	ROM_LOAD       ( "sknsa1.u10", 0x000000, 0x080000, CRC(745e5212) SHA1(caba649ab2d83b2d7e007eecee0fc582c019df38) ) /* Asia BIOS */

	ROM_REGION32_BE( 0x200000, REGION_USER1, 0 ) /* SH-2 Code mapped at 0x04000000 */
    ROM_LOAD16_BYTE( "ss01a.u6",     0x000000, 0x080000, CRC(962fe857) SHA1(3df74c5efff11333dea9316a063129dcec0d7bdd) )
    ROM_LOAD16_BYTE( "ss01a.u4",     0x000001, 0x080000, CRC(ee853c23) SHA1(ddbf7f7cf509788ee3daf7b4d8ae1482e6e31a03) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
    ROM_LOAD( "ss100-00.u21", 0x000000, 0x400000, CRC(bc7b3dfa) SHA1(dff10a7aef548abda48470293382057a2ca9557e) )
    ROM_LOAD( "ss101-00.u20", 0x400000, 0x400000, CRC(ab2df280) SHA1(e456c578a36f585b24379d74def1bcab276c2b1b) )
    ROM_LOAD( "ss102-00.u8",  0x800000, 0x400000, CRC(0845eafe) SHA1(663b163bf4e87c7df0030e791f95b1a5827de315) )
    ROM_LOAD( "ss103-00.u32", 0xc00000, 0x400000, CRC(ee451ac9) SHA1(01cc6b6f371c0090a6a7f4c33d05f4b9a6c59fee) )

	ROM_REGION( 0x800000, REGION_GFX2, ROMREGION_DISPOSE )
    ROM_LOAD( "ss200-00.u17", 0x000000, 0x400000, CRC(cd773976) SHA1(38b8df5e685be65c3fde09f9e585591f678632d4) )
    ROM_LOAD( "ss201-00.u9",  0x400000, 0x400000, CRC(301fad4c) SHA1(15faf37eeec5cc46afcb4bd236345b5c3dd647ac) )

	ROM_REGION( 0x600000, REGION_GFX3, 0 ) /* Tiles Plane B */
	/* First 0x040000 bytes (0x03ff Tiles) are RAM Based Tiles */
	/* 0x040000 - 0x3fffff empty? */
    ROM_LOAD( "ss210-00.u3",  0x400000, 0x200000, CRC(c3697805) SHA1(bd41064e3527cdc4b9a4ab9c423c916309b3f057) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Samples */
    ROM_LOAD( "ss300-00.u1", 0x000000, 0x400000, CRC(35b04b18) SHA1(b69f33fc6a50ec20382329317d20b3c1e7f01b87) )
ROM_END

ROM_START( sengekij )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH-2 Code */
	ROM_LOAD       ( "sknsj1.u10",   0x000000, 0x080000, CRC(7e2b836c) SHA1(92c5a7a2472496028bff0e5980d41dd294f42144) ) /* Japan BIOS */

	ROM_REGION32_BE( 0x200000, REGION_USER1, 0 ) /* SH-2 Code mapped at 0x04000000 */
    ROM_LOAD16_BYTE( "ss01j.u6",     0x000000, 0x080000, CRC(9efdcd5a) SHA1(66cca04d07999dc8ca0bcf19db925996b34d0390) )
    ROM_LOAD16_BYTE( "ss01j.u4",     0x000001, 0x080000, CRC(92c3f45e) SHA1(60c647e66b0126fb7749874be39938972481b957) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
    ROM_LOAD( "ss100-00.u21", 0x000000, 0x400000, CRC(bc7b3dfa) SHA1(dff10a7aef548abda48470293382057a2ca9557e) )
    ROM_LOAD( "ss101-00.u20", 0x400000, 0x400000, CRC(ab2df280) SHA1(e456c578a36f585b24379d74def1bcab276c2b1b) )
    ROM_LOAD( "ss102-00.u8",  0x800000, 0x400000, CRC(0845eafe) SHA1(663b163bf4e87c7df0030e791f95b1a5827de315) )
    ROM_LOAD( "ss103-00.u32", 0xc00000, 0x400000, CRC(ee451ac9) SHA1(01cc6b6f371c0090a6a7f4c33d05f4b9a6c59fee) )

	ROM_REGION( 0x800000, REGION_GFX2, ROMREGION_DISPOSE )
    ROM_LOAD( "ss200-00.u17", 0x000000, 0x400000, CRC(cd773976) SHA1(38b8df5e685be65c3fde09f9e585591f678632d4) )
    ROM_LOAD( "ss201-00.u9",  0x400000, 0x400000, CRC(301fad4c) SHA1(15faf37eeec5cc46afcb4bd236345b5c3dd647ac) )

	ROM_REGION( 0x600000, REGION_GFX3, 0 ) /* Tiles Plane B */
	/* First 0x040000 bytes (0x03ff Tiles) are RAM Based Tiles */
	/* 0x040000 - 0x3fffff empty? */
    ROM_LOAD( "ss210-00.u3",  0x400000, 0x200000, CRC(c3697805) SHA1(bd41064e3527cdc4b9a4ab9c423c916309b3f057) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Samples */
    ROM_LOAD( "ss300-00.u1", 0x000000, 0x400000, CRC(35b04b18) SHA1(b69f33fc6a50ec20382329317d20b3c1e7f01b87) )
ROM_END

ROM_START( senknow )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH-2 Code */
	ROM_LOAD       ( "sknsj1.u10",   0x000000, 0x080000, CRC(7e2b836c) SHA1(92c5a7a2472496028bff0e5980d41dd294f42144) ) /* Japan BIOS */

	ROM_REGION32_BE( 0x200000, REGION_USER1, 0 ) /* SH-2 Code mapped at 0x04000000 */
    ROM_LOAD16_BYTE( "snw000j1.u6",  0x000000, 0x080000, CRC(0d6136f6) SHA1(eedd011cfe03577bfaf386723502d03f6e5dbd8c) )
    ROM_LOAD16_BYTE( "snw001j1.u4",  0x000001, 0x080000, CRC(4a10ec3d) SHA1(bbec4fc53bd61d06ffe5a53debada5785b124fdd) )

	ROM_REGION( 0x0800000, REGION_GFX1, 0 )
    ROM_LOAD( "snw10000.u21", 0x000000, 0x400000, CRC(5133c69c) SHA1(d279df3ffd005dbf0930a8e40eaf2467f8653284) )
    ROM_LOAD( "snw10100.u20", 0x400000, 0x400000, CRC(9dafe03f) SHA1(978b4597ff2a54ac5049fd64798e8173b29dd363) )

	ROM_REGION( 0x800000, REGION_GFX2, ROMREGION_DISPOSE )
    ROM_LOAD( "snw20000.u17", 0x000000, 0x400000, CRC(d5fe5f8c) SHA1(817d8d0a5fbc0c50dc3c592f938150f82df97cec) )
    ROM_LOAD( "snw20100.u9",  0x400000, 0x400000, CRC(c0037846) SHA1(3267b142ebce47e1717250239d98fdb4af7964f8) )

	ROM_REGION( 0x800000, REGION_GFX3, 0 ) /* Tiles Plane B */
	/* First 0x040000 bytes (0x03ff Tiles) are RAM Based Tiles */
	/* 0x040000 - 0x3fffff empty? */
    ROM_LOAD( "snw21000.u3",  0x400000, 0x400000, CRC(f5c23e79) SHA1(b509680001c3205b289f43d4f44aaaa7f896419b) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Samples */
    ROM_LOAD( "snw30000.u1",  0x000000, 0x400000, CRC(ec9eef40) SHA1(8f74ec9cb6054a77227c0505094f0ef8bc371429) )
ROM_END

ROM_START( teljan )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH-2 Code */
	ROM_LOAD       ( "sknsj1.u10",   0x000000, 0x080000, CRC(7e2b836c) SHA1(92c5a7a2472496028bff0e5980d41dd294f42144) ) /* Japan BIOS */

	ROM_REGION32_BE( 0x200000, REGION_USER1, 0 ) /* SH-2 Code mapped at 0x04000000 */
    ROM_LOAD16_BYTE( "tel1j.u10",    0x000000, 0x080000, CRC(09b552fe) SHA1(2f315fd09eb22fa8c81faa1e926038f20daa845f) )
    ROM_LOAD16_BYTE( "tel1j.u8",     0x000001, 0x080000, CRC(070b4345) SHA1(5743f12a351b89593c6adfaeb8a5a2ab7bc8b424) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
    ROM_LOAD( "tj100-00.u24", 0x000000, 0x400000, CRC(810144f1) SHA1(1c90e71e5f34ee05771ab4a673329f78f17791df) )
    ROM_LOAD( "tj101-00.u20", 0x400000, 0x400000, CRC(82f570e1) SHA1(3ba9d1775f897052aca5cff2edbf575399101c5c) )
    ROM_LOAD( "tj102-00.u17", 0x800000, 0x400000, CRC(ace875dc) SHA1(be97c895beeac979c5704986e818d4f3cfa00e49) )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE )
    ROM_LOAD( "tj200-00.u16", 0x000000, 0x400000, CRC(be0f90b2) SHA1(1848a65f244e1e8a3ff7ab38e76f86cabca8b47e) )

	ROM_REGION( 0x400000, REGION_GFX3, 0 ) /* Tiles Plane B */
	/* First 0x040000 bytes (0x03ff Tiles) are RAM Based Tiles */
	/* 0x040000 - 0x3fffff empty? */

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Samples */
    ROM_LOAD( "tj300-00.u4", 0x000000, 0x400000, CRC(685495c4) SHA1(3853c0583b84ed3163370ae48e4b3912cbeb986e) )
ROM_END

ROM_START( ryouran )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH-2 Code */
	ROM_LOAD       ( "sknsj1.u10",   0x000000, 0x080000, CRC(7e2b836c) SHA1(92c5a7a2472496028bff0e5980d41dd294f42144) ) /* Japan BIOS */

	ROM_REGION32_BE( 0x200000, REGION_USER1, 0 ) /* SH-2 Code mapped at 0x04000000 */
    ROM_LOAD16_BYTE( "or000j1.u10",  0x000000, 0x080000, CRC(d93aa491) SHA1(dc01707f1e80d81f28d6b685d08fc6c0d2bf7330) )
    ROM_LOAD16_BYTE( "or001j1.u8",   0x000001, 0x080000, CRC(f466e5e9) SHA1(65d699f6f9e299333e51a6a52cb13a0f1a902fe1) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
    ROM_LOAD( "or100-00.u24", 0x000000, 0x400000, CRC(e9c7695b) SHA1(0a104d4e4e0c933d2eaaf410a8c243db6673786a) )
    ROM_LOAD( "or101-00.u20", 0x400000, 0x400000, CRC(fe06bf12) SHA1(f3a2f88aed65bcc1c16f37fd4c0011e3538128f7) )
    ROM_LOAD( "or102-00.u17", 0x800000, 0x400000, CRC(f2a5237b) SHA1(b8871f9c0f3864c334ec9a8146cf7dd1961ecb94) )

	ROM_REGION( 0x800000, REGION_GFX2, ROMREGION_DISPOSE )
    ROM_LOAD( "or200-00.u16", 0x000000, 0x400000, CRC(4c4701a8) SHA1(7b397b553ba86bba2ee82228cabdf2179e878d69) )
    ROM_LOAD( "or201-00.u13", 0x400000, 0x400000, CRC(a94064aa) SHA1(5d736f810ffdbb6ada5c5efcb5fb29eedafc3e2f) )

	ROM_REGION( 0x400000, REGION_GFX3, 0 ) /* Tiles Plane B */
	/* First 0x040000 bytes (0x03ff Tiles) are RAM Based Tiles */
	/* 0x040000 - 0x3fffff empty? */

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Samples */
    ROM_LOAD( "or300-00.u4", 0x000000, 0x400000, CRC(a3f64b79) SHA1(6ecb2b4c0d213fe5384b19d6bfdb86871f21fd9f) )
ROM_END

ROM_START( vblokbrk )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH-2 Code */
	ROM_LOAD       ( "sknsa1.u10", 0x000000, 0x080000, CRC(745e5212) SHA1(caba649ab2d83b2d7e007eecee0fc582c019df38) ) /* Asia BIOS */

	ROM_REGION32_BE( 0x200000, REGION_USER1, 0 ) /* SH-2 Code mapped at 0x04000000 */
    ROM_LOAD16_BYTE( "sk01a.u10",    0x000000, 0x080000, CRC(4d1be53e) SHA1(3d28b73a67530147962b8df6244af8bea2ab080f) )
    ROM_LOAD16_BYTE( "sk01a.u8",     0x000001, 0x080000, CRC(461e0197) SHA1(003573a4abdbecc6dd234a13c61ef07a25d980e2) )

	ROM_REGION( 0x0400000, REGION_GFX1, 0 )
    ROM_LOAD( "sk100-00.u24", 0x000000, 0x200000, CRC(151dd88a) SHA1(87bb1039a9883f721a315760eb2c4abe4a94046f) )
    ROM_LOAD( "sk-101.u20",   0x200000, 0x100000, CRC(779cce23) SHA1(70147b36d982524ba9921823e481ce8fbb5daa26) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
    ROM_LOAD( "sk200-00.u16", 0x000000, 0x200000, CRC(2e297c61) SHA1(4071b945a1294fbc3d18fab1f144bf09af4349e8) )

	ROM_REGION( 0x400000, REGION_GFX3, 0 ) /* Tiles Plane B */
	/* First 0x040000 bytes (0x03ff Tiles) are RAM Based Tiles */
	/* 0x040000 - 0x3fffff empty? */

	ROM_REGION( 0x200000, REGION_SOUND1, 0 ) /* Samples */
    ROM_LOAD( "sk300-00.u4", 0x000000, 0x200000, CRC(e6535c05) SHA1(8895b7c326e0261691cb184887ac1ca637302460) )
ROM_END

ROM_START( sarukani )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH-2 Code */
	ROM_LOAD       ( "sknsj1.u10",   0x000000, 0x080000, CRC(7e2b836c) SHA1(92c5a7a2472496028bff0e5980d41dd294f42144) ) /* Japan BIOS */

	ROM_REGION32_BE( 0x200000, REGION_USER1, 0 ) /* SH-2 Code mapped at 0x04000000 */
    ROM_LOAD16_BYTE( "sk1j1.u10",    0x000000, 0x080000, CRC(fcc131b6) SHA1(5e3e71ee1f736b6098e671e6f57b1fb313c81adb) )
    ROM_LOAD16_BYTE( "sk1j1.u8",     0x000001, 0x080000, CRC(3b6aa343) SHA1(a969b20b1170d82351024cab9e37f2fbfd01ddeb) )

	ROM_REGION( 0x0400000, REGION_GFX1, 0 )
    ROM_LOAD( "sk100-00.u24", 0x000000, 0x200000, CRC(151dd88a) SHA1(87bb1039a9883f721a315760eb2c4abe4a94046f) )
    ROM_LOAD( "sk-101.u20",   0x200000, 0x100000, CRC(779cce23) SHA1(70147b36d982524ba9921823e481ce8fbb5daa26) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
    ROM_LOAD( "sk200-00.u16", 0x000000, 0x200000, CRC(2e297c61) SHA1(4071b945a1294fbc3d18fab1f144bf09af4349e8) )

	ROM_REGION( 0x400000, REGION_GFX3, 0 ) /* Tiles Plane B */
	/* First 0x040000 bytes (0x03ff Tiles) are RAM Based Tiles */
	/* 0x040000 - 0x3fffff empty? */

	ROM_REGION( 0x200000, REGION_SOUND1, 0 ) /* Samples */
    ROM_LOAD( "sk300-00.u4", 0x000000, 0x200000, CRC(e6535c05) SHA1(8895b7c326e0261691cb184887ac1ca637302460) )
ROM_END


/***** GAME DRIVERS *****/

GAMEX( 1996, skns,     0,    skns, skns, 0,        ROT0,  "Kaneko", "Super Kaneko Nova System BIOS", NOT_A_DRIVER )

GAMEX( 1996, galpani4, skns,    skns, skns,     galpani4, ROT0,  "Kaneko", "Gals Panic 4 (Japan)", GAME_IMPERFECT_GRAPHICS )
GAMEX( 2001, galpandx, skns,    skns, skns,     galpandx, ROT0,  "Kaneko", "Gals Panic DX (Asia)", GAME_IMPERFECT_GRAPHICS ) /* copyright 2001, re-release for the Asian market? */
GAMEX( 1997, galpanis, skns,    skns, skns,     galpanis, ROT0,  "Kaneko", "Gals Panic S - Extra Edition (Japan)", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1998, cyvern,   skns,    skns, skns,     cyvern,   ROT90, "Kaneko", "Cyvern (Japan)", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1999, galpans2, skns,    skns, skns,     galpans2, ROT0,  "Kaneko", "Gals Panic S2 (Japan)", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1999, panicstr, skns,    skns, skns,     panicstr, ROT0,  "Kaneko", "Panic Street (Japan)", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1999, senknow , skns,    skns, skns,     senknow,  ROT0,  "Kaneko / Kouyousha", "Sen-Know (Japan)", GAME_IMPERFECT_GRAPHICS )
GAMEX( 2000, gutsn,    skns,    skns, skns,     gutsn,    ROT0,  "Kaneko / Kouyousha", "Guts'n (Japan)", GAME_IMPERFECT_GRAPHICS ) /* quite fragile, started working of it's own accord in 0.69 :)*/
GAMEX( 1998, puzzloop, skns,    skns, skns,     puzzloop, ROT0,  "Mitchell", "Puzz Loop (Europe)", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1998, puzloopj, puzzloop,skns, skns,     puzloopj, ROT0,  "Mitchell", "Puzz Loop (Japan)", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1998, puzloopu, puzzloop,skns, skns,     puzloopu, ROT0,  "Mitchell", "Puzz Loop (USA)", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1996, jjparads, skns,    skns, jjparads, jjparads, ROT0,  "Electro Design", "Jan Jan Paradise", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1997, jjparad2, skns,    skns, jjparads, jjparad2, ROT0,  "Electro Design", "Jan Jan Paradise 2", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1998, ryouran , skns,    skns, jjparads, ryouran,  ROT0,  "Electro Design", "VS Mahjong Otome Ryouran", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1999, teljan  , skns,    skns, jjparads, teljan,   ROT0,  "Electro Design", "Tel Jan", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1997, sengekis, skns,    skns, skns,     sengekis, ROT90, "Kaneko / Warashi", "Sengeki Striker (Asia)", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1997, sengekij, sengekis,skns, skns,     sengekij, ROT90, "Kaneko / Warashi", "Sengeki Striker (Japan)", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1997, vblokbrk, skns,    skns, skns,     sarukani, ROT0,  "Kaneko / Mediaworks", "VS Block Breaker (Asia)", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1997, sarukani, vblokbrk,skns, skns,     sarukani, ROT0,  "Kaneko / Mediaworks", "Saru-Kani-Hamu-Zou (Japan)", GAME_IMPERFECT_GRAPHICS )
