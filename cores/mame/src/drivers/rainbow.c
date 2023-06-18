/***************************************************************************

Rainbow Islands  (c) Taito 1987   + Jumping
===============

driver by Mike Coates

c-chip enhanced by Robert Gallagher, with many thanks to Tormod Tjaberg for providing
his PCB for dumping/collecting the c-chip data from Rainbow Islands Extra.

                            ***

Notes on Rainbow Islands romsets by Robert Gallagher
----------------------------------------------------

There are 3 code segments that differ between Rainbow Islands old and new
version. They are all related to secret rooms;

   The first code segment is entered at the start of a secret room.


         rainbowo                             rainbow
   $55EE lea    $C01308,a0              $55EE lea    $C00B08,a0
         move.w #$13,d0                       bsr    $561C
   $55F8 move.w #$15fa,2(a0)                  lea    $C00D08,a0
         move.w #$15fa,6(a0)                  bsr    $561C
         move.w #$15fa,$102(a0)               lea    $C00F08,a0
         move.w #$15fa,$106(a0)               bsr    $561C
         adda.l #8,a0                         lea    $C01108,a0
         dbf    d0,$55F8                      bsr    $561C
         rts                            $561C lea    $C01308,a0
                                              move.w #$13,d0
                                              ...

   The next code segment fixes the bonus related to entering ALL 10 Secret rooms.
   In Rainbow Islands, If you enter the last secret room, you receive a 10,000,000
   point bonus. If you have entered all 10 secret rooms, you receive a 50,000,000 point
   bonus. (BUG THAT'S NOT A BUG - the games says 1,000,000 and 5,000,000, but the code gives
   you 10mil and 50mil)
   The counter is in $10D05C.b, the (rainbowo) romset never increments this counter, making
   it impossible to achieve.

   $56A2 cmpi.w #$31,$11c6(a5)          $56CA addi.b #1,$105C(a5)   ;increment the secret room count
         bne                                  cmpi.w #$31,$11c6(a5) ;is it round 49 (50)?
                                              bne

   The final change fixes a Secret room bug that was noted by Stefan Jokisch.
   In (rainbowo) it is possible to scroll the screen inside a secret room.
   You can walk right off the top of the screen, and scroll the 'next' secret
   room into view. This is fixed in (rainbow) with the following code;

   $5F06 move.b #1,d0                   $5F34 tst.b  $11c4(a5)  ; in secret room?
         jsr    $1736                         bne    $600A      ; exit this routine, which is getting
                                              move.b #1,d0      ; the room height from c-chip bank 1
                                              jsr    $1736      ; which we don't need
                                              ...
                                        $600A rts

   The (jumping) bootleg is based on the (rainbowo) roms.

Notes on Rainbow Islands Extra by Robert Gallagher
--------------------------------------------------
   In Rainbow Islands Extra, there are more changes than just the differing enemies/rounds.

   In RIE if you end a level with the same 2 digits in your score as xxxx220, you will receive
   items at the "GOAL IN" worth 3000 points each (instead of 500pts) as:

   00 = french fries
   11 = neopolaton ice cream
   22 = creme caramel
   33 = hamburgers
   44 = cake slice
   55 = iced bun
   66 = mug of beer
   77 = hotdog on a stick
   88 = vanilla ice cream
   99 = blue popsicle

   There are two other possibilities:
   moneybags - worth 500pts if you don't have double digits, or worth 3000 points if you do (any)
               if you got the 'Money Coin' powerup in a secret room.
   hearts - worth 10,000 each, this is based on a timer in 0x0010DBA4 && 128 == 0, making this a
            1 in 128 chance.


   The items received in 'Secret Room' are not the same as Rainbow Islands. They are based on
   the LAST diamond you collect in the round before entering. That is; you must still collect
   the 7 diamonds in order, Red, Orange, Yellow... to enter a secret room, however, AFTER completing
   this, you can still collect diamonds. If you collect (last), you get:

   Red - Book of Continues - this allows you to continue after world 7
   Orange - Money Bag - this will make a 100,000pt Money bag fall from the sky on every round.
            it will also make all fruit at 'GOAL IN' into money bags.
   Yellow - Key - when taken, this will show the Secret room code completely for about 8 seconds.
   Green - Protection Fairy, gives you the protection fairy for about 8 seconds at the beginning
           of each round, or after you die. (NOT perm. like RI, code at $C528 in RIE handles this,
           and is distinctly different from the code at $BC8C in RI)
   Blue - Yellow Potion, gives you perm. fast rainbow power
   Indigo - Red Potion, gives you perm. 2X rainbow power
   Violet - Shoes, gives you perm. fast feet.

   Unlike the secret rooms in Rainbow Islands, in Rainbow Islands Extra, only 2 letters of the code
   for that room are revealed. The first letter, and the second letter will corespond to the last
   diamond collected before entering the secret room. (You can see this with the coloured hearts
   that remain). The idea is that you must enter the secret room 7X to get the full code, making the
   game harder. Otherwise, collecting a yellow diamond will give you the 'key' to the room, and reveal
   the code for about 5 seconds.

   In the secret rooms of RIE there is an added bonus; If you exit the room without collecting the
   power-up item, you will receive a 1,000,000pt bonus. The text in the room in Japanese reads:
   "KYOUKA-SOUBI WO TORAZUNI DERUTO 1000000 TEN HAIRUYO !!"

   If you enter all 10 secret rooms in Rainbow Islands Extra, and do NOT collect the powerups in
   _any_ of the rooms, you will receive "SPECIAL BIG BONUS !! 50000000 PTS."

   If you enter all 10 secret rooms, and do NOT collect the bubble-rainbow power in secret room 10,
   (though you may collect them in others) you will receive "SPECIAL BIG BONUS !! 10000000 PTS."

   If you collect the bubble-rainbow power in secret room 10, you will receive "ALL ROOM CLEAR
   5000000" bonus instead.

   In Rainbow Islands Extra (as in Rainbow Islands), if you collect 2 of any colour 'cane'...
     (canes are collected for collecting 7 of one colour of diamond, WITHOUT 'complete' happening, or
     if 'complete', you must collect 7 of the same colour diamond within one round. (the diamond
     counters are reset to one at the beginning of each round once 'complete' has occured).
   ...you will receive a 'Potion' (not Rainbow powerup potions)
   of the appropriate colour. Once collected, you will receive a
   100,000pts. large fruit item at 'GOAL IN' for that round. Although the code for this is present in
   Rainbow Islands, it is rarely seen. This is not a result of poor emulation, but rather poor coding;
   In both RI and RIE, there are tables that indicate which special item is deserved, and what actions
   are required to earn them. (the number of times the event must occur).
   In both RI and RIE, you receive a special item for every 3rd enemy you kill as:
   1. Shoes
   2. Red Potion
   3. Yellow Potion
   4. Red Potion
   5. Crystal Ball
   6. Yellow star
   7. Red Star
   8. - Special item - depending on what you deserve.

   The Special item table can be found at $ADC0 in RI, and $B5E8 in RIE. This table is checked in
   linear fashion until a special item requirement is met.
   items 24/25/26 are: Blue Ring/Violet Ring/Red Ring.
   To get these items you collect: Red Potion/Yellow Potion/Shoes. (note that each potion collect =+2)
   In Rainbow Islands, this table shows that we need only collect:
   3 red potions (but each one counts as +2 if you created it, or +1 if you 'found' it in a level),
   3 yellow potions ""
   3 red shoes (only count as one)
   The 'canes', and 'potions' occur -after- these entries (cane index 27-33, potion index 34-40)
   So the reason that you (rarely!) see the canes, and hence the potions in RI, is because you will
   almost ALWAYS satisfy at least one of these requirements: 2 RED potions are given before 1 'special'
   In Rainbow Islands Extra however, the canes are more common. This is because the index shows that
   we need to collect 12/12/12 of the above before we can get the rings.

   One more condition has been added to RIE regards this;
   if _no_ special item has been earned, you will receive a Potion that is the same colour as the
   LAST diamond you have collected in the round.



Bugs in Jumping
---------------

The bootleggers didn't defeat the protection completely: Secret
rooms are broken, some dying enemy sprites have obviously wrong
graphics, monsters are falling through platforms etc.


Secret rooms in Rainbow Islands
-------------------------------

Getting the small diamonds in order (red through to purple) opens
a secret door in the boss room. The trick is to turn an enemy into
a diamond of a specific color: It depends on its x position and
direction. There is a cheat code at the top of each secret room
that can be entered on the copyright screen:

	L -> left
	R -> right
	J -> jump
	B -> rainbow
	S -> start

				  |  regular   |  extra
	--------------+------------+------------
	Fast Feet     |  BLRBJSBJ  |  SLLSRJRR
	Red Potion    |  RJSBJLBR  |  JLSSSBRJ
	Yellow Potion |  SSSLLRRS  |  BRSLJSLJ
	Hint A        |  BJBJBJRS  |  BJBJBJRS
	Hint B        |  LJLSLBLS  |  LJLSLBLS
	Continue      |  LBSJRLJL  |  LJLRSJJJ
	Money Bags    |  RRLLBBJS  |  LLSBRRJB
	Money + Cont  |  RRRRSBSJ  |  RSJRLBRS
	Hint C        |  SJBLRJSR  |  SJBLRJSR
	100M Counter  |  SRBJSLSB  |  BBSSJJJJ
	Hint D        |  N/A       |  BBJJRLSL
	Hint E        |  N/A       |  LRSLRSJR
	Hint F        |  N/A       |  SBJLSBRR

***************************************************************************/

#include "driver.h"
#include "state.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/taitoic.h"
#include "sndhrdw/taitosnd.h"

VIDEO_START( rainbow );
VIDEO_START( jumping );

VIDEO_UPDATE( rainbow );
VIDEO_UPDATE( jumping );

WRITE16_HANDLER( jumping_spritectrl_w );
WRITE16_HANDLER( rainbow_spritectrl_w );
WRITE16_HANDLER( rainbow_cchip_ram_w );
WRITE16_HANDLER( rainbow_cchip_ctrl_w );
WRITE16_HANDLER( rainbow_cchip_bank_w );

READ16_HANDLER( rainbow_cchip_ram_r );
READ16_HANDLER( rainbow_cchip_ctrl_r );

void rainbow_cchip_init(int version);

static int jumping_latch = 0;


static WRITE16_HANDLER( jumping_sound_w )
{
	if (ACCESSING_LSB)
	{
		jumping_latch = data & 0xff; /*M68000 writes .b to $400007*/
		cpu_set_irq_line(1,0,HOLD_LINE);
	}
}


/***************************************************************************
                            MEMORY STRUCTURES
***************************************************************************/

static MEMORY_READ16_START( rainbow_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x10c000, 0x10ffff, MRA16_RAM },	/* main RAM */
	{ 0x200000, 0x200fff, MRA16_RAM },	/* palette */
	{ 0x201000, 0x203fff, MRA16_RAM },	/* read in initial checks */
	{ 0x390000, 0x390003, input_port_0_word_r },
	{ 0x3b0000, 0x3b0003, input_port_1_word_r },
	{ 0x3e0000, 0x3e0001, MRA16_NOP },
	{ 0x3e0002, 0x3e0003, taitosound_comm16_lsb_r },
	{ 0x800000, 0x8007ff, rainbow_cchip_ram_r },
	{ 0x800802, 0x800803, rainbow_cchip_ctrl_r },
	{ 0xc00000, 0xc0ffff, PC080SN_word_0_r },
	{ 0xd00000, 0xd03fff, PC090OJ_word_0_r },	/* sprite ram + other stuff */
MEMORY_END

static MEMORY_WRITE16_START( rainbow_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x10c000, 0x10ffff, MWA16_RAM },
	{ 0x200000, 0x200fff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
	{ 0x201000, 0x203fff, MWA16_RAM },	/* written in initial checks */
	{ 0x3a0000, 0x3a0001, rainbow_spritectrl_w },
	{ 0x3c0000, 0x3c0003, MWA16_NOP },	/* written very often, watchdog? */
	{ 0x3e0000, 0x3e0001, taitosound_port16_lsb_w },
	{ 0x3e0002, 0x3e0003, taitosound_comm16_lsb_w },
	{ 0x800000, 0x8007ff, rainbow_cchip_ram_w },
	{ 0x800802, 0x800803, rainbow_cchip_ctrl_w },
	{ 0x800c00, 0x800c01, rainbow_cchip_bank_w },
	{ 0xc00000, 0xc0ffff, PC080SN_word_0_w },
	{ 0xc20000, 0xc20003, PC080SN_yscroll_word_0_w },
	{ 0xc40000, 0xc40003, PC080SN_xscroll_word_0_w },
	{ 0xc50000, 0xc50003, PC080SN_ctrl_word_0_w },
	{ 0xd00000, 0xd03fff, PC090OJ_word_0_w },	/* sprite ram + other stuff */
MEMORY_END


static MEMORY_READ16_START( jumping_readmem )
	{ 0x000000, 0x09ffff, MRA16_ROM },
	{ 0x10c000, 0x10ffff, MRA16_RAM },	/* main RAM */
	{ 0x200000, 0x200fff, MRA16_RAM },	/* palette */
	{ 0x201000, 0x203fff, MRA16_RAM },	/* read in initial checks */
	{ 0x400000, 0x400001, input_port_0_word_r },
	{ 0x400002, 0x400003, input_port_1_word_r },
	{ 0x401000, 0x401001, input_port_2_word_r },
	{ 0x401002, 0x401003, input_port_3_word_r },
	{ 0x420000, 0x420001, MRA16_NOP },	/* read, but result not used */
	{ 0x440000, 0x4407ff, MRA16_RAM },
	{ 0xc00000, 0xc0ffff, PC080SN_word_0_r },
	{ 0xd00000, 0xd01fff, MRA16_RAM },	/* original spriteram location, needed for Attract Mode */
MEMORY_END

static MEMORY_WRITE16_START( jumping_writemem )
	{ 0x000000, 0x09ffff, MWA16_ROM },
	{ 0x10c000, 0x10ffff, MWA16_RAM },
	{ 0x200000, 0x200fff, paletteram16_xxxxBBBBGGGGRRRR_word_w , &paletteram16 },
	{ 0x201000, 0x203fff, MWA16_RAM },	/* written in initial checks */
	{ 0x3a0000, 0x3a0001, jumping_spritectrl_w },
	{ 0x3c0000, 0x3c0001, MWA16_NOP },	/* watchdog? */
	{ 0x400006, 0x400007, jumping_sound_w },
	{ 0x430000, 0x430003, PC080SN_yscroll_word_0_w },
	{ 0x440000, 0x4407ff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x800000, 0x80ffff, MWA16_NOP },	/* original c-chip location (not used) */
	{ 0xc00000, 0xc0ffff, PC080SN_word_0_w },
	{ 0xc20000, 0xc20003, MWA16_NOP },	/* seems it is a leftover from rainbow: scroll y written here too */
	{ 0xc40000, 0xc40003, PC080SN_xscroll_word_0_w },
	{ 0xd00000, 0xd01fff, MWA16_RAM }, 	/* original spriteram location, needed for Attract Mode */
MEMORY_END


/**********************************************************
                         SOUND

              Rainbow uses a YM2151 and YM3012
              Jumping uses two YM2203's
***********************************************************/

static WRITE_HANDLER( bankswitch_w )
{
	cpu_setbank(5, memory_region(REGION_CPU2) + ((data - 1) & 3) * 0x4000 + 0x10000);
}

static READ_HANDLER( jumping_latch_r )
{
	return jumping_latch;
}


static MEMORY_READ_START( rainbow_s_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x7fff, MRA_BANK5 },
	{ 0x8000, 0x8fff, MRA_RAM },
	{ 0x9001, 0x9001, YM2151_status_port_0_r },
	{ 0x9002, 0x9100, MRA_RAM },
	{ 0xa001, 0xa001, taitosound_slave_comm_r },
MEMORY_END

static MEMORY_WRITE_START( rainbow_s_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x8fff, MWA_RAM },
	{ 0x9000, 0x9000, YM2151_register_port_0_w },
	{ 0x9001, 0x9001, YM2151_data_port_0_w },
	{ 0xa000, 0xa000, taitosound_slave_port_w },
	{ 0xa001, 0xa001, taitosound_slave_comm_w },
MEMORY_END

static MEMORY_READ_START( jumping_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x8fff, MRA_RAM },
	{ 0xb000, 0xb000, YM2203_status_port_0_r },
	{ 0xb400, 0xb400, YM2203_status_port_1_r },
	{ 0xb800, 0xb800, jumping_latch_r },
	{ 0xc000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( jumping_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x8fff, MWA_RAM },
	{ 0xb000, 0xb000, YM2203_control_port_0_w },
	{ 0xb001, 0xb001, YM2203_write_port_0_w },
	{ 0xb400, 0xb400, YM2203_control_port_1_w },
	{ 0xb401, 0xb401, YM2203_write_port_1_w },
	{ 0xbc00, 0xbc00, MWA_NOP },	/* looks like a bankswitch, but sound works with or without it */
MEMORY_END


/***********************************************************
			 INPUT PORTS, DIPs
***********************************************************/

INPUT_PORTS_START( rainbow )
	PORT_START	/* DIP SWITCH A */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

	PORT_START	/* DIP SWITCH B */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Medium" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPSETTING(    0x04, "100k,1000k" )
	PORT_DIPNAME( 0x08, 0x08, "Complete Bonus" )
	PORT_DIPSETTING(    0x00, "100K Points" )
	PORT_DIPSETTING(    0x08, "1 Up" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x10, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPNAME( 0x40, 0x00, "Language" )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPSETTING(    0x40, "Japanese" )
	PORT_DIPNAME( 0x80, 0x00, "Coin Type" )
	PORT_DIPSETTING(    0x00, "Type 1" )
	PORT_DIPSETTING(    0x80, "Type 2" )

	PORT_START	/* 800007 */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )

	PORT_START /* 800009 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )

	PORT_START	/* 80000B */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 )

	PORT_START	/* 80000d */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
INPUT_PORTS_END

INPUT_PORTS_START( jumping )
	PORT_START	/* DIP SWITCH A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

	PORT_START	/* DIP SWITCH B */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Medium" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x04, "100k,1000k" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x08, 0x00, "Complete Bonus" )
	PORT_DIPSETTING(    0x08, "1 Up" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x10, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "Coin Type" )
	PORT_DIPSETTING(    0x00, "Type 1" )
	PORT_DIPSETTING(    0x80, "Type 2" )

	PORT_START  /* 401001 - Coins Etc. */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* 401003 - Player Controls */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY )
INPUT_PORTS_END


/**************************************************************
                         GFX DECODING
**************************************************************/

static struct GfxLayout tilelayout =
{
	8,8,    /* 8*8 tiles */
	RGN_FRAC(1,1),
	4,      /* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 8, 12, 0, 4, 24, 28, 16, 20 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8    /* every tile takes 32 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,  /* 16*16 sprites */
	RGN_FRAC(1,1),
	4,      /* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 8, 12, 0, 4, 24, 28, 16, 20, 40, 44, 32, 36, 56, 60, 48, 52 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
			8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	128*8   /* every sprite takes 128 consecutive bytes */
};

static struct GfxDecodeInfo rainbow_gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0x000000, &spritelayout, 0, 0x80 },	/* OBJ 16x16 */
	{ REGION_GFX1, 0x000000, &tilelayout,   0, 0x80 },	/* SCR 8x8 */
	{ -1 }	/* end of array */
};


static struct GfxLayout jumping_tilelayout =
{
	8,8,    /* 8*8 tiles */
	16384,  /* 16384 tiles */
	4,      /* 4 bits per pixel */
	{ 0, 0x20000*8, 0x40000*8, 0x60000*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8     /* every tile takes 8 consecutive bytes */
};

static struct GfxLayout jumping_spritelayout =
{
	16,16,  /* 16*16 sprites */
	5120,   /* 5120 sprites */
	4,      /* 4 bits per pixel */
	{ 0x78000*8,0x50000*8,0x28000*8,0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8*16+0, 8*16+1, 8*16+2, 8*16+3, 8*16+4, 8*16+5, 8*16+6, 8*16+7 },
	{ 0, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8    /* every sprite takes 32 consecutive bytes */
};

static struct GfxDecodeInfo jumping_gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0, &jumping_spritelayout, 0, 0x80 },	/* OBJ 16x16 */
	{ REGION_GFX1, 0, &jumping_tilelayout,   0, 0x80 },	/* SCR 8x8 */
	{ -1 }	/* end of array */
};


/**************************************************************
                   YM2151 & YM2203 (SOUND)
**************************************************************/

/* handler called by the YM2151 emulator when the internal timers cause an IRQ */

static void irqhandler(int irq)
{
	cpu_set_irq_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2151interface ym2151_interface =
{
	1,          /* 1 chip */
	4000000,    /* 4 MHz ? */
	{ YM3012_VOL(50,MIXER_PAN_LEFT,50,MIXER_PAN_RIGHT) },
	{ irqhandler },
	{ bankswitch_w }
};

static struct YM2203interface ym2203_interface =
{
	2,          /* 2 chips */
	3579545,    /* ?? MHz */
	{ YM2203_VOL(30,30), YM2203_VOL(30,30) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};


/***********************************************************
                      MACHINE DRIVERS
***********************************************************/

static MACHINE_DRIVER_START( rainbow )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 8000000)
	MDRV_CPU_MEMORY(rainbow_readmem,rainbow_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)

	MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_MEMORY(rainbow_s_readmem,rainbow_s_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)	/* 10 CPU slices per frame - enough for the sound CPU to read all commands */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 1*8, 31*8-1) /* is Y visible correct ? */
	MDRV_GFXDECODE(rainbow_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(8192)

	MDRV_VIDEO_START(rainbow)
	MDRV_VIDEO_UPDATE(rainbow)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( jumping )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 8000000)
	MDRV_CPU_MEMORY(jumping_readmem,jumping_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)

	MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_MEMORY(jumping_sound_readmem,jumping_sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)	/* 10 CPU slices per frame - enough ? */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 1*8, 31*8-1) /* is Y visible correct ? */
	MDRV_GFXDECODE(jumping_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(8192)

	MDRV_VIDEO_START(jumping)
	MDRV_VIDEO_UPDATE(jumping)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, ym2203_interface)
MACHINE_DRIVER_END


/***************************************************************************
                                  DRIVERS
***************************************************************************/

ROM_START( rainbow )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "b22-10-1.19",   0x00000, 0x10000, CRC(e34a50ca) SHA1(17a92cd7182db1e18000b1ae689758fcfd70fe16) )
	ROM_LOAD16_BYTE( "b22-11-1.20",   0x00001, 0x10000, CRC(6a31a093) SHA1(1e99ae47811c0d3774d138dab02ac50bc1b92173) )
	ROM_LOAD16_BYTE( "b22-08-1.21",   0x20000, 0x10000, CRC(15d6e17a) SHA1(7b0339180239e75adf1437aee276b652a1bfee51) )
	ROM_LOAD16_BYTE( "b22-09-1.22",   0x20001, 0x10000, CRC(454e66bc) SHA1(54d57dee90da04101f4b1473770b38001d63230f) )
	ROM_LOAD16_BYTE( "b22-03.23",     0x40000, 0x20000, CRC(3ebb0fb8) SHA1(1b41b305623d121255eb70cb992e4d9da13abd82) )
	ROM_LOAD16_BYTE( "b22-04.24",     0x40001, 0x20000, CRC(91625e7f) SHA1(765afd973d9b82bb496b04beca284bf2769d6e6f) )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )
	ROM_LOAD( "b22-14.43",            0x00000, 0x4000, CRC(113c1a5b) SHA1(effa2adf54a6be78b2d4baf3a47529342fb0d895) )
	ROM_CONTINUE(                     0x10000, 0xc000 )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b22-01.2",             0x00000, 0x80000, CRC(b76c9168) SHA1(e924be0c8294b930488bb04583784254a840a52e) )	/* tiles */

	ROM_REGION( 0xa0000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "b22-01.5",             0x00000, 0x80000, CRC(1b87ecf0) SHA1(37a463184f4064fe0565367236e289d57639614c) )	/* sprites */
	ROM_LOAD16_BYTE( "b22-12.7",      0x80000, 0x10000, CRC(67a76dc6) SHA1(626ee684eb3ea859c695ffe03344ccaa442da4af) )
	ROM_LOAD16_BYTE( "b22-13.6",      0x80001, 0x10000, CRC(2fda099f) SHA1(a1e27a4497f6733608be924d69d965b19f725b99) )
ROM_END

ROM_START( rainbowo )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "b22-10.19",     0x00000, 0x10000, CRC(3b013495) SHA1(fc89f401a80b9bde174df8a257bb7fad4937c838) )
	ROM_LOAD16_BYTE( "b22-11.20",     0x00001, 0x10000, CRC(80041a3d) SHA1(619d71a2bef5fd898a15d37d8016850f832428c3) )
	ROM_LOAD16_BYTE( "b22-08.21",     0x20000, 0x10000, CRC(962fb845) SHA1(1c5581e697902ee5cde0fb841ef05eade04a901b) )
	ROM_LOAD16_BYTE( "b22-09.22",     0x20001, 0x10000, CRC(f43efa27) SHA1(f92bcdbe9bbc1c200946c10605846bb406d8e963) )
	ROM_LOAD16_BYTE( "b22-03.23",     0x40000, 0x20000, CRC(3ebb0fb8) SHA1(1b41b305623d121255eb70cb992e4d9da13abd82) )
	ROM_LOAD16_BYTE( "b22-04.24",     0x40001, 0x20000, CRC(91625e7f) SHA1(765afd973d9b82bb496b04beca284bf2769d6e6f) )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )
	ROM_LOAD( "b22-14.43",            0x00000, 0x4000, CRC(113c1a5b) SHA1(effa2adf54a6be78b2d4baf3a47529342fb0d895) )
	ROM_CONTINUE(                     0x10000, 0xc000 )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b22-01.2",             0x00000, 0x80000, CRC(b76c9168) SHA1(e924be0c8294b930488bb04583784254a840a52e) )	/* tiles */

	ROM_REGION( 0xa0000, REGION_GFX2, ROMREGION_DISPOSE )
  	ROM_LOAD( "b22-01.5",             0x00000, 0x80000, CRC(1b87ecf0) SHA1(37a463184f4064fe0565367236e289d57639614c) )	/* sprites */
	ROM_LOAD16_BYTE( "b22-12.7",      0x80000, 0x10000, CRC(67a76dc6) SHA1(626ee684eb3ea859c695ffe03344ccaa442da4af) )
	ROM_LOAD16_BYTE( "b22-13.6",      0x80001, 0x10000, CRC(2fda099f) SHA1(a1e27a4497f6733608be924d69d965b19f725b99) )
ROM_END

ROM_START( rainbowe )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "b39-01.19",     0x00000, 0x10000, CRC(50690880) SHA1(88cd8739eaa6e4e5988be225c31d2a6605173d39) )
	ROM_LOAD16_BYTE( "b39-02.20",     0x00001, 0x10000, CRC(4dead71f) SHA1(03e9df33fc8fc64d6eeb1c3a763acac00b10c071) )
	ROM_LOAD16_BYTE( "b39-03.21",     0x20000, 0x10000, CRC(4a4cb785) SHA1(6ba7de3901a001cbb31713664ccb68b943a5578f) )
	ROM_LOAD16_BYTE( "b39-04.22",     0x20001, 0x10000, CRC(4caa53bd) SHA1(302c668b77de41ffef96a3ca9d94877b4affc466) )
	ROM_LOAD16_BYTE( "b22-03.23",     0x40000, 0x20000, CRC(3ebb0fb8) SHA1(1b41b305623d121255eb70cb992e4d9da13abd82) )
	ROM_LOAD16_BYTE( "b22-04.24",     0x40001, 0x20000, CRC(91625e7f) SHA1(765afd973d9b82bb496b04beca284bf2769d6e6f) )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )
	ROM_LOAD( "b22-14.43",            0x00000, 0x4000, CRC(113c1a5b) SHA1(effa2adf54a6be78b2d4baf3a47529342fb0d895) )
	ROM_CONTINUE(                     0x10000, 0xc000 )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b22-01.2",             0x00000, 0x80000, CRC(b76c9168) SHA1(e924be0c8294b930488bb04583784254a840a52e) )	/* tiles */

	ROM_REGION( 0xa0000, REGION_GFX2, ROMREGION_DISPOSE )
  	ROM_LOAD( "b22-01.5",             0x00000, 0x80000, CRC(1b87ecf0) SHA1(37a463184f4064fe0565367236e289d57639614c) )	/* sprites */
	ROM_LOAD16_BYTE( "b22-12.7",      0x80000, 0x10000, CRC(67a76dc6) SHA1(626ee684eb3ea859c695ffe03344ccaa442da4af) )
	ROM_LOAD16_BYTE( "b22-13.6",      0x80001, 0x10000, CRC(2fda099f) SHA1(a1e27a4497f6733608be924d69d965b19f725b99) )
ROM_END

ROM_START( jumping )
	ROM_REGION( 0xa0000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "jb1_h4",        0x00000, 0x10000, CRC(3fab6b31) SHA1(57803478949cb62f7eab2ef9be08b13aa2237dbc) )
	ROM_LOAD16_BYTE( "jb1_h8",        0x00001, 0x10000, CRC(8c878827) SHA1(4a54a217b7c442305c3ce9298aa36ae225382444) )
	ROM_LOAD16_BYTE( "jb1_i4",        0x20000, 0x10000, CRC(443492cf) SHA1(fc3809d784d611df4fd446ca2443eebdf4f0bfd7) )
	ROM_LOAD16_BYTE( "jb1_i8",        0x20001, 0x10000, CRC(ed33bae1) SHA1(24ade29f3fdd45aa772ccf9c046294baa6eae7a2) )
	ROM_LOAD16_BYTE( "b22-03.23",     0x40000, 0x20000, CRC(3ebb0fb8) SHA1(1b41b305623d121255eb70cb992e4d9da13abd82) )
	ROM_LOAD16_BYTE( "b22-04.24",     0x40001, 0x20000, CRC(91625e7f) SHA1(765afd973d9b82bb496b04beca284bf2769d6e6f) )
	ROM_LOAD16_BYTE( "jb1_f89",       0x80001, 0x10000, CRC(0810d327) SHA1(fe91ac02e617bde413dc8a20b7cbcaf3e20aeb28) )	/* c-chip substitute */

	ROM_REGION( 0x14000, REGION_CPU2, 0 )
	ROM_LOAD( "jb1_cd67",             0x00000, 0x8000, CRC(8527c00e) SHA1(86e3824caca39aca4ca4df63bb4474adacfc4c53) )
	ROM_CONTINUE(                     0x10000, 0x4000 )
	ROM_CONTINUE(                     0x0c000, 0x4000 )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "jb2_ic8",              0x00000, 0x10000, CRC(65b76309) SHA1(1e345726e137f4c56d4bf239651c986fd53a16c3) )	/* tiles */
	ROM_LOAD( "jb2_ic7",              0x10000, 0x10000, CRC(43a94283) SHA1(d6a05cbc7b996a8e7f1520563f6fada9a59021a4) )
	ROM_LOAD( "jb2_ic10",             0x20000, 0x10000, CRC(e61933fb) SHA1(02bc0e1a7a3ce9e15fb83b28ce8fafb0b8d80ebd) )
	ROM_LOAD( "jb2_ic9",              0x30000, 0x10000, CRC(ed031eb2) SHA1(905be4d890ff7bb8a4d8ad85b2a11483fb4d67eb) )
	ROM_LOAD( "jb2_ic12",             0x40000, 0x10000, CRC(312700ca) SHA1(c79edc9c25f364d0afd79aaa21cfe2fe46044314) )
	ROM_LOAD( "jb2_ic11",             0x50000, 0x10000, CRC(de3b0b88) SHA1(14b8871821e4c0abbb9967c5aa282cf4e67884fe) )
	ROM_LOAD( "jb2_ic14",             0x60000, 0x10000, CRC(9fdc6c8e) SHA1(ff4e1a98dc982bce2f9d235cac62c7166f477f64) )
	ROM_LOAD( "jb2_ic13",             0x70000, 0x10000, CRC(06226492) SHA1(834280ec49e61a0c9c6b6fe2033e1b20bd1bffbf) )

	ROM_REGION( 0xa0000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "jb2_ic62",             0x00000, 0x10000, CRC(8548db6c) SHA1(675cd301259d5ed16098a38ac58b27b5ccd91264) )	/* sprites */
	ROM_LOAD( "jb2_ic61",             0x10000, 0x10000, CRC(37c5923b) SHA1(c83ef45564c56ef62d7019aecbd79dccc671deee) )
	ROM_LOAD( "jb2_ic60",             0x20000, 0x08000, CRC(662a2f1e) SHA1(1c5e8b1f0623e64faf9cd60f9653fc5957191a9b) )
	ROM_LOAD( "jb2_ic78",             0x28000, 0x10000, CRC(925865e1) SHA1(457de50bc03e8b949ac7d46ae4188201e87574a8) )
	ROM_LOAD( "jb2_ic77",             0x38000, 0x10000, CRC(b09695d1) SHA1(e6d315f9befb7b47f42668d573a1102e52d78aea) )
	ROM_LOAD( "jb2_ic76",             0x48000, 0x08000, CRC(41937743) SHA1(890c832a7cf87e6fe749d4824b02d57e10872bdf) )
	ROM_LOAD( "jb2_ic93",             0x50000, 0x10000, CRC(f644eeab) SHA1(9d45e9dfb08e8c90b4b10f5dc383fa4732161a81) )
	ROM_LOAD( "jb2_ic92",             0x60000, 0x10000, CRC(3fbccd33) SHA1(137880a564573db1f9e6de5c730af6709ca301f3) )
	ROM_LOAD( "jb2_ic91",             0x70000, 0x08000, CRC(d886c014) SHA1(9327c332c98a81451e9e0624344d2601ef06e490) )
	ROM_LOAD( "jb2_i121",             0x78000, 0x10000, CRC(93df1e4d) SHA1(b100d265b973254ec9cd44b6c32f62b4bac3b732) )
	ROM_LOAD( "jb2_i120",             0x88000, 0x10000, CRC(7c4e893b) SHA1(eceecb38554157ee24d228a2c722dad750a6a07d) )
	ROM_LOAD( "jb2_i119",             0x98000, 0x08000, CRC(7e1d58d8) SHA1(d586a018c3ec3e6e6a39992170d324361e03c68a) )
ROM_END


static DRIVER_INIT( rainbow )
{
	rainbow_cchip_init(0);
}

static DRIVER_INIT( rainbowe )
{
	rainbow_cchip_init(1);
}

static DRIVER_INIT( jumping )
{
	int i;

	/* Sprite colour map is reversed - switch to normal */

	for (i = 0;i < memory_region_length(REGION_GFX2);i++)
		memory_region(REGION_GFX2)[i] ^= 0xff;

	state_save_register_int("jumping", 0, "sound", &jumping_latch);
}


GAME( 1987, rainbow,  0,       rainbow, rainbow, rainbow,  ROT0, "Taito Corporation", "Rainbow Islands (new version)" )
GAME( 1987, rainbowo, rainbow, rainbow, rainbow, rainbow,  ROT0, "Taito Corporation", "Rainbow Islands (old version)" )
GAME( 1988, rainbowe, rainbow, rainbow, rainbow, rainbowe, ROT0, "Taito Corporation", "Rainbow Islands (Extra)" )
GAME( 1989, jumping,  rainbow, jumping, jumping, jumping,  ROT0, "bootleg", "Jumping" )
