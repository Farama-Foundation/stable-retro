/***************************************************************************

Legionnaire (c) Tad 1992
-----------

David Graves

Made from MAME D-con and Toki drivers (by Bryan McPhail, Jarek Parchanski)


Heated Barrel looks like a minor revision of the Legionnaire
hardware. It has a graphics banking facility, which doubles the 0xfff
different tiles available for use in the foreground layer.


Legionnaire BK3 charsets
------------------------

The GFX roms contain two odd sections of 256 16x16 tiles marked as BK3.
These need to be brought together and decoded as a single section of
0x200 tiles.

The 0x104000 area appears to be extra paletteram?


TODO
----

Unemulated protection messes up both games.

Seibu sound system mentioned in log.


Legionnaire
-----------

Foreground tiles screwy (screen after character selection screen).

Need 16 px off top of vis area?


Heated Barrel
-------------

Big problems with layers not being cleared, especially the text
layer. There may be a write to the COP controlling layer clearance?

Ends with Access violation when you die on round 1 boss. A lot of
non-existent area reads in log - maybe because of bad reads from
the COP.


Preliminary COP MCU memory map
------------------------------

0x400-0x5ff   Protection related
0x600-0x6ff   Includes standard screen control words
0x700-0x7ff   Includes standard Seibu sound system


***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"
#include "sndhrdw/seibu.h"

extern WRITE16_HANDLER( legionna_background_w );
extern WRITE16_HANDLER( legionna_foreground_w );
extern WRITE16_HANDLER( legionna_midground_w );
extern WRITE16_HANDLER( legionna_text_w );
extern WRITE16_HANDLER( legionna_control_w );

extern VIDEO_START( legionna );
extern VIDEO_START( cupsoc );
extern VIDEO_UPDATE( legionna );
extern VIDEO_UPDATE( godzilla );
extern VIDEO_UPDATE( sdgndmrb );
void heatbrl_setgfxbank(UINT16 data);

extern data16_t *legionna_back_data,*legionna_fore_data,*legionna_mid_data,*legionna_scrollram16,*legionna_textram;
static data16_t *mcu_ram;

static WRITE16_HANDLER( legionna_paletteram16_w )	/* xBBBBxRRRRxGGGGx */
{
	int a,r,g,b;
	COMBINE_DATA(&paletteram16[offset]);

	a = paletteram16[offset];

	r = (a >> 1) & 0x0f;
	g = (a >> 6) & 0x0f;
	b = (a >> 11) & 0x0f;

	r = (r << 4) | r;
	g = (g << 4) | g;
	b = (b << 4) | b;

	palette_set_color(offset,r,g,b);
}

/* Mcu reads in attract in Legionnaire game demo

Guess the 0x400-0x5ff area of the COP is protection related.

CPU0 PC 0032a2 unknown MCU write offset: 0260 data: 9c6c
CPU0 PC 0032a8 unknown MCU write offset: 0250 data: 0010
CPU0 PC 0032c8 unknown MCU write offset: 0261 data: 987c
CPU0 PC 0032ce unknown MCU write offset: 0251 data: 0010
CPU0 PC 003546 unknown MCU write offset: 0262 data: 02c4
CPU0 PC 00354a unknown MCU write offset: 0252 data: 0004
CPU0 PC 00355c unknown MCU write offset: 0263 data: 0000
CPU0 PC 003560 unknown MCU write offset: 0253 data: 0004
CPU0 PC 003568 unknown MCU write offset: 0280 data: a180
CPU0 PC 00356e unknown MCU write offset: 0280 data: a980
CPU0 PC 003574 unknown MCU write offset: 0280 data: b100
CPU0 PC 00357a unknown MCU write offset: 0280 data: b900
CPU0 PC 003580 unknown MCU read offset: 02c4
CPU0 PC 003588 unknown MCU read offset: 02c2
CPU0 PC 003594 unknown MCU read offset: 02c1
CPU0 PC 0035a0 unknown MCU read offset: 02c3
CPU0 PC 0032a2 unknown MCU write offset: 0260 data: 9c6c
CPU0 PC 0032a8 unknown MCU write offset: 0250 data: 0010
CPU0 PC 0032c8 unknown MCU write offset: 0261 data: 987c
CPU0 PC 0032ce unknown MCU write offset: 0251 data: 0010
CPU0 PC 003422 unknown MCU write offset: 0280 data: 138e
CPU0 PC 003428 unknown MCU read offset: 02da
CPU0 PC 00342e unknown MCU read offset: 02d8
CPU0 PC 00346c unknown MCU write offset: 0280 data: 3bb0
CPU0 PC 0032a2 unknown MCU write offset: 0260 data: 987c
CPU0 PC 0032a8 unknown MCU write offset: 0250 data: 0010
CPU0 PC 003306 unknown MCU write offset: 0280 data: 8100
CPU0 PC 00330c unknown MCU write offset: 0280 data: 8900


Mcu reads in attract in Heated Barrel game demo (note
partial similarity)

(i) This sequence repeats a number of times early on:

CPU0 PC 0085b4 unknown MCU write offset: 0210 data: 0064
CPU0 PC 0085ba unknown MCU write offset: 0211 data: 0000
CPU0 PC 0085be unknown MCU read offset: 02ca
CPU0 PC 0085ee unknown MCU read offset: 02c9
CPU0 PC 008622 unknown MCU read offset: 02c8

(ii) This happens a few times:

CPU0 PC 0017ac unknown MCU write offset: 0260 data: b6cc
CPU0 PC 0017b2 unknown MCU write offset: 0250 data: 0010
CPU0 PC 0017d2 unknown MCU write offset: 0261 data: babc
CPU0 PC 0017d8 unknown MCU write offset: 0251 data: 0010
CPU0 PC 00192c unknown MCU write offset: 0280 data: 138e
CPU0 PC 001932 unknown MCU read offset: 02da
CPU0 PC 001938 unknown MCU read offset: 02d8
CPU0 PC 001976 unknown MCU write offset: 0280 data: 3bb0
CPU0 PC 0017ac unknown MCU write offset: 0260 data: b6cc
CPU0 PC 0017b2 unknown MCU write offset: 0250 data: 0010
CPU0 PC 0017d2 unknown MCU write offset: 0261 data: bb9c
CPU0 PC 0017d8 unknown MCU write offset: 0251 data: 0010
CPU0 PC 00192c unknown MCU write offset: 0280 data: 138e
CPU0 PC 001932 unknown MCU read offset: 02da
CPU0 PC 001938 unknown MCU read offset: 02d8
CPU0 PC 001976 unknown MCU write offset: 0280 data: 3bb0

(iii) Later on this happens a lot:

CPU0 PC 0017ac unknown MCU write offset: 0260 data: c61c
CPU0 PC 0017b2 unknown MCU write offset: 0250 data: 0010
CPU0 PC 0017d2 unknown MCU write offset: 0261 data: bb9c
CPU0 PC 0017d8 unknown MCU write offset: 0251 data: 0010
CPU0 PC 001a5c unknown MCU write offset: 0262 data: aa48
CPU0 PC 001a62 unknown MCU write offset: 0252 data: 0003
CPU0 PC 001a7c unknown MCU write offset: 0263 data: a0c8
CPU0 PC 001a82 unknown MCU write offset: 0253 data: 0003
CPU0 PC 001a86 unknown MCU write offset: 0280 data: a100
CPU0 PC 001a8c unknown MCU write offset: 0280 data: b080
CPU0 PC 001a92 unknown MCU write offset: 0280 data: a900
CPU0 PC 001a98 unknown MCU write offset: 0280 data: b880
CPU0 PC 001a9e unknown MCU read offset: 02c0
CPU0 PC 001aa6 unknown MCU read offset: 02c2
CPU0 PC 001ab2 unknown MCU read offset: 02c1

*/

static READ16_HANDLER( mcu_r )
{
	switch (offset)
	{
		/* Protection is not understood */

		case (0x470/2):	/* read PC $110a, could be some sort of control word:
				sometimes a bit is changed then it's poked back in... */
			return (rand() &0xffff);

		case (0x582/2):	/* read PC $3594 */
			return (rand() &0xffff);

		case (0x584/2):	/* read PC $3588 */
			return (rand() &0xffff);

		case (0x586/2):	/* read PC $35a0 */
			return (rand() &0xffff);

		case (0x588/2):	/* read PC $3580 */
			return (rand() &0xffff);

		case (0x5b0/2):	/* bit 15 is branched on a few times in the $3300 area */
			return (rand() &0xffff);

		case (0x5b4/2):	/* read and stored in ram before +0x5b0 bit 15 tested */
			return (rand() &0xffff);

		/* Non-protection reads */

		case (0x708/2):	/* seibu sound: these three around $b10 on */
			return seibu_main_word_r(2,0);

		case (0x70c/2):
			return seibu_main_word_r(3,0);

		case (0x714/2):
			return seibu_main_word_r(5,0);

		/* Inputs */

		case (0x740/2):	/* code at $b00 sticks waiting for bit 6 hi */
			return input_port_1_word_r(0,0);

		case (0x744/2):
			return input_port_2_word_r(0,0);

		case (0x748/2):	/* code at $f4a reads this 4 times in _weird_ fashion */
			return input_port_0_word_r(0,0);

		case (0x74c/2):
			return input_port_3_word_r(0,0);

	}
log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU0 PC %06x unknown MCU read offset: %04x\n",activecpu_get_previouspc(),offset);

	return mcu_ram[offset];
}

static WRITE16_HANDLER( mcu_w )
{
	COMBINE_DATA(&mcu_ram[offset]);

	switch (offset)
	{
		/* 61a bit 0 is flipscreen*/
		/* 61c probably layer disables, like Dcon*/
		/* 620 - 62a scroll control;  is there a layer priority switch...?*/

		case (0x620/2):
		{
			legionna_scrollram16[0] = mcu_ram[offset];
			break;
		}
		case (0x622/2):
		{
			legionna_scrollram16[1] = mcu_ram[offset];
			break;
		}
		case (0x624/2):
		{
			legionna_scrollram16[2] = mcu_ram[offset];
			break;
		}
		case (0x626/2):
		{
			legionna_scrollram16[3] = mcu_ram[offset];
			break;
		}
		case (0x628/2):
		{
			legionna_scrollram16[4] = mcu_ram[offset];
			break;
		}
		case (0x62a/2):
		{
			legionna_scrollram16[5] = mcu_ram[offset];
			break;
		}
		case (0x700/2):	/* seibu(0) */
		{
			seibu_main_word_w(0,mcu_ram[offset],0xff00);
			break;
		}
		case (0x704/2):	/* seibu(1) */
		{
			seibu_main_word_w(1,mcu_ram[offset],0xff00);
			break;
		}
		case (0x710/2):	/* seibu(4) */
		{
			seibu_main_word_w(4,mcu_ram[offset],0xff00);
			break;
		}
		case (0x718/2):	/* seibu(6) */
		{
			seibu_main_word_w(6,mcu_ram[offset],0xff00);
			break;
		}
		default:
log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU0 PC %06x unknown MCU write offset: %04x data: %04x\n",activecpu_get_previouspc(),offset,data);
	}
}

static READ16_HANDLER( cop2_mcu_r )
{
	switch (offset)
	{
		/* Protection is not understood */

		case (0x580/2):	/* read PC $1a9e */
			return (rand() &0xffff);

		case (0x582/2):	/* read PC $1ab2 */
			return (rand() &0xffff);

		case (0x584/2):	/* read PC $1aa6 */
			return (rand() &0xffff);

		case (0x590/2):	/* read PC $8622 */
			return (rand() &0xffff);

		case (0x592/2):	/* read PC $85ee */
			return (rand() &0xffff);

		case (0x594/2):	/* read PC $85be */
			return (rand() &0xffff);

		case (0x5b0/2):	/* bit 15 is branched on a few times in the $1938 area */
			return (rand() &0xffff);

		case (0x5b4/2):	/* read at $1932 and stored in ram before +0x5b0 bit 15 tested */
			return (rand() &0xffff);

		/* Non-protection reads */

		case (0x7c8/2):	/* seibu sound */
			return seibu_main_word_r(2,0);

		case (0x7cc/2):
			return seibu_main_word_r(3,0);

		case (0x7d4/2):
			return seibu_main_word_r(5,0);

		/* Inputs */

		case (0x740/2):
			return input_port_1_word_r(0,0);

		case (0x744/2):
			return input_port_2_word_r(0,0);

		case (0x748/2):
			return input_port_4_word_r(0,0);

		case (0x74c/2):
			return input_port_3_word_r(0,0);

	}
log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU0 PC %06x unknown MCU read offset: %04x\n",activecpu_get_previouspc(),offset);

	return mcu_ram[offset];
}

static WRITE16_HANDLER( cop2_mcu_w )
{
	COMBINE_DATA(&mcu_ram[offset]);

	switch (offset)
	{
		case (0x470/2):
		{
			heatbrl_setgfxbank( mcu_ram[offset] );
			break;
		}

		/* 65a bit 0 is flipscreen*/
		/* 65c probably layer disables, like Dcon? Used on screen when you press P1-4 start (values 13, 11, 0 seen)*/
		/* 660 - 66a scroll control;  is there a layer priority switch...?*/

		case (0x660/2):
		{
			legionna_scrollram16[0] = mcu_ram[offset];
			break;
		}
		case (0x662/2):
		{
			legionna_scrollram16[1] = mcu_ram[offset];
			break;
		}
		case (0x664/2):
		{
			legionna_scrollram16[2] = mcu_ram[offset];
			break;
		}
		case (0x666/2):
		{
			legionna_scrollram16[3] = mcu_ram[offset];
			break;
		}
		case (0x668/2):
		{
			legionna_scrollram16[4] = mcu_ram[offset];
			break;
		}
		case (0x66a/2):
		{
			legionna_scrollram16[5] = mcu_ram[offset];
			break;
		}
		case (0x7c0/2):	/* seibu(0) */
		{
			seibu_main_word_w(0,mcu_ram[offset],0xff00);
			break;
		}
		case (0x7c4/2):	/* seibu(1) */
		{
			seibu_main_word_w(1,mcu_ram[offset],0xff00);
			break;
		}
		case (0x7d0/2):	/* seibu(4) */
		{
			seibu_main_word_w(4,mcu_ram[offset],0xff00);
			break;
		}
		case (0x7d8/2):	/* seibu(6) */
		{
			seibu_main_word_w(6,mcu_ram[offset],0xff00);
			break;
		}
		default:
log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU0 PC %06x unknown MCU write offset: %04x data: %04x\n",activecpu_get_previouspc(),offset,data);
	}
}

static READ16_HANDLER( godzilla_cop_mcu_r )
{
	switch (offset)
	{
		/* Protection is not understood */

		/* Non-protection reads */

		case (0x7c8/2):	/* seibu sound */
			return seibu_main_word_r(2,0);

		case (0x7cc/2):
			return seibu_main_word_r(3,0);

		case (0x7d4/2):
			return seibu_main_word_r(5,0);

		/* Inputs */

		case (0x740/2):
			return input_port_1_word_r(0,0);

		case (0x744/2):
			return input_port_2_word_r(0,0);

		case (0x748/2):
			return input_port_4_word_r(0,0);

		case (0x74c/2):
			return input_port_3_word_r(0,0);

	}
/*logerror("CPU0 PC %06x unknown MCU read offset: %04x\n",activecpu_get_previouspc(),offset);*/

	return mcu_ram[offset];
}

static WRITE16_HANDLER( godzilla_cop_mcu_w )
{
	COMBINE_DATA(&mcu_ram[offset]);

	switch (offset)
	{
		case (0x7c0/2):	/* seibu(0) */
		{
			seibu_main_word_w(0,mcu_ram[offset],0xff00);
			break;
		}
		case (0x7c4/2):	/* seibu(1) */
		{
			seibu_main_word_w(1,mcu_ram[offset],0xff00);
			break;
		}
		case (0x7d0/2):	/* seibu(4) */
		{
			seibu_main_word_w(4,mcu_ram[offset],0xff00);
			break;
		}
		case (0x7d8/2):	/* seibu(6) */
		{
			seibu_main_word_w(6,mcu_ram[offset],0xff00);
			break;
		}
/*		default:*/
/*		log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU0 PC %06x MCU write offset: %04x data: %04x\n",activecpu_get_previouspc(),offset*2,data);*/
	}
}

static READ16_HANDLER( sdgndmrb_cop_mcu_r )
{
	switch (offset)
	{
		/* Non-protection reads */

		case (0x708/2):	/* seibu sound */
			return seibu_main_word_r(2,0);

		case (0x70c/2):
			return seibu_main_word_r(3,0);

		case (0x714/2):
			return seibu_main_word_r(5,0);

		/* Inputs */

		case (0x740/2):
			return input_port_1_word_r(0,0);

		case (0x744/2):
			return input_port_2_word_r(0,0);

		case (0x748/2):
			return input_port_4_word_r(0,0);

		case (0x74c/2):
			return input_port_3_word_r(0,0);

		case (0x75c/2):
			return input_port_5_word_r(0,0);
	}
/*	return rand();*/
/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU0 PC %06x MCU read offset: %04x\n",activecpu_get_previouspc(),offset*2);*/
/*	usrintf_showmessage("CPU0 PC %06x MCU read offset: %04x",activecpu_get_previouspc(),offset*2);*/

	return mcu_ram[offset];
}

static WRITE16_HANDLER( sdgndmrb_cop_mcu_w )
{
	COMBINE_DATA(&mcu_ram[offset]);

	switch (offset)
	{

		/* TODO: tilemaps x-axis are offset,we use a temporary kludge for now */
		case (0x620/2):
		{
			legionna_scrollram16[0] = 0x10 + mcu_ram[offset];
			break;
		}
		case (0x622/2):
		{
			legionna_scrollram16[1] = mcu_ram[offset];
			break;
		}
		case (0x624/2):
		{
			legionna_scrollram16[2] = 0x10 + mcu_ram[offset];
			break;
		}
		case (0x626/2):
		{
			legionna_scrollram16[3] = mcu_ram[offset];
			break;
		}
		case (0x628/2):
		{
			legionna_scrollram16[4] = 0x10 + mcu_ram[offset];
			break;
		}
		case (0x62a/2):
		{
			legionna_scrollram16[5] = mcu_ram[offset];
			break;
		}
		/* scroll mirrors? */
		case (0x62c/2):
		case (0x62e/2):
		case (0x630/2):
		case (0x632/2):
		case (0x634/2):
		case (0x636/2):
			break;
		/* Text Layer scroll registers */
		case (0x638/2):
		{
			legionna_scrollram16[6] = 0x38 + mcu_ram[offset];
			break;
		}
		case (0x63a/2):
		{
			legionna_scrollram16[7] = mcu_ram[offset];
			break;
		}
		/* OBJ scroll register,seems unused by this game. */
		case (0x644/2):
			break;

		/* Seems a mirror for the choices in the test menu... */
		case (0x67c/2):
		case (0x680/2):
		case (0x6fc/2):
			break;

		case (0x700/2):	/* seibu(0) */
		{
			seibu_main_word_w(0,mcu_ram[offset],0xff00);
			break;
		}
		case (0x704/2):	/* seibu(1) */
		{
			seibu_main_word_w(1,mcu_ram[offset],0xff00);
			break;
		}
		case (0x710/2):	/* seibu(4) */
		{
			seibu_main_word_w(4,mcu_ram[offset],0xff00);
			break;
		}
		case (0x718/2):	/* seibu(6) */
		{
			seibu_main_word_w(6,mcu_ram[offset],0xff00);
			break;
		}

/*		default:*/
/*		log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU0 PC %06x MCU write offset: %04x data: %04x\n",activecpu_get_previouspc(),offset*2,data);*/
/*		usrintf_showmessage("CPU0 PC %06x MCU write offset: %04x data: %04x",activecpu_get_previouspc(),offset*2,data);*/
	}
}

/*****************************************************************************/

static MEMORY_READ16_START( legionna_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x100000, 0x1007ff, mcu_r },	/* COP mcu */
	{ 0x101000, 0x1017ff, MRA16_RAM },	/* 32x16 bg layer, 16x16 tiles */
	{ 0x101800, 0x101fff, MRA16_RAM },	/* 32x16 bg layer, 16x16 tiles */
	{ 0x102000, 0x1027ff, MRA16_RAM },	/* 32x16 bg layer, 16x16 tiles */
	{ 0x102800, 0x1037ff, MRA16_RAM },	/* 64x32 text/front layer, 8x8 tiles */

	/* The 4000-4fff area contains PALETTE words and may be extra paletteram? */
	{ 0x104000, 0x104fff, MRA16_RAM },	/* palette mirror ? */
/*	{ 0x104000, 0x10401f, MRA16_RAM },	 // debugging... /*/
/*	{ 0x104200, 0x1043ff, MRA16_RAM },	 // ??? /*/
/*	{ 0x104600, 0x1047ff, MRA16_RAM },	 // ??? /*/
/*	{ 0x104800, 0x10481f, MRA16_RAM },	 // ??? /*/

	{ 0x105000, 0x105fff, MRA16_RAM },	/* spriteram */
	{ 0x106000, 0x106fff, MRA16_RAM },
	{ 0x107000, 0x107fff, MRA16_RAM },	/* palette */
	{ 0x108000, 0x113fff, MRA16_RAM },	/* main ram */
MEMORY_END

static MEMORY_WRITE16_START( legionna_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x100000, 0x1007ff, mcu_w, &mcu_ram },	/* COP mcu */
	{ 0x101000, 0x1017ff, legionna_background_w, &legionna_back_data },
	{ 0x101800, 0x101fff, legionna_foreground_w, &legionna_fore_data },
	{ 0x102000, 0x1027ff, legionna_midground_w,  &legionna_mid_data },
	{ 0x102800, 0x1037ff, legionna_text_w, &legionna_textram },

	/* The 4000-4fff area contains PALETTE words and may be extra paletteram? */
	{ 0x104000, 0x104fff, MWA16_RAM },
/*	{ 0x104000, 0x104fff, legionna_paletteram16_w },*/
/*	{ 0x104000, 0x10401f, MWA16_RAM },*/
/*	{ 0x104200, 0x1043ff, MWA16_RAM },*/
/*	{ 0x104600, 0x1047ff, MWA16_RAM },*/
/*	{ 0x104800, 0x10481f, MWA16_RAM },*/

	{ 0x105000, 0x105fff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x106000, 0x106fff, MWA16_RAM },	/* is this used outside inits ?? */
	{ 0x107000, 0x107fff, legionna_paletteram16_w, &paletteram16 },	/* palette xRRRRxGGGGxBBBBx ? */
	{ 0x108000, 0x113fff, MWA16_RAM },
MEMORY_END

static MEMORY_READ16_START( heatbrl_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x100000, 0x1007ff, cop2_mcu_r },	/* COP mcu */
	{ 0x100800, 0x100fff, MRA16_RAM },	/* 32x16 bg layer, 16x16 tiles */
	{ 0x101000, 0x1017ff, MRA16_RAM },	/* 32x16 bg layer, 16x16 tiles */
	{ 0x101800, 0x101fff, MRA16_RAM },	/* 32x16 bg layer, 16x16 tiles */
	{ 0x102000, 0x102fff, MRA16_RAM },	/* 64x32 text/front layer, 8x8 tiles */
	{ 0x103000, 0x103fff, MRA16_RAM },	/* spriteram */
	{ 0x104000, 0x104fff, MRA16_RAM },	/* palette */
	{ 0x108000, 0x11ffff, MRA16_RAM },	/* main ram */
MEMORY_END

static MEMORY_WRITE16_START( heatbrl_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x100000, 0x1007ff, cop2_mcu_w, &mcu_ram },	/* COP mcu */
	{ 0x100800, 0x100fff, legionna_background_w, &legionna_back_data },
	{ 0x101000, 0x1017ff, legionna_foreground_w, &legionna_fore_data },
	{ 0x101800, 0x101fff, legionna_midground_w,  &legionna_mid_data },
	{ 0x102000, 0x102fff, legionna_text_w, &legionna_textram },
	{ 0x103000, 0x103fff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x104000, 0x104fff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
	{ 0x108000, 0x11ffff, MWA16_RAM },
MEMORY_END

static MEMORY_READ16_START( godzilla_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x100000, 0x1007ff, godzilla_cop_mcu_r },	/* COP mcu */
	{ 0x101000, 0x1017ff, MRA16_RAM },
	{ 0x101800, 0x101fff, MRA16_RAM },
	{ 0x102000, 0x102fff, MRA16_RAM },
	{ 0x103000, 0x103fff, MRA16_RAM },
	{ 0x104000, 0x104fff, MRA16_RAM },
	{ 0x105000, 0x107fff, MRA16_RAM },
	{ 0x108000, 0x11ffff, MRA16_RAM },	/* main ram */
MEMORY_END

static MEMORY_WRITE16_START( godzilla_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x100000, 0x1007ff, godzilla_cop_mcu_w, &mcu_ram },	/* COP mcu */
	{ 0x101000, 0x1017ff, legionna_background_w, &legionna_back_data },
	{ 0x101800, 0x101fff, MWA16_RAM },
	{ 0x102000, 0x102fff, legionna_text_w, &legionna_textram },
	{ 0x103000, 0x1037ff, legionna_midground_w,  &legionna_mid_data },
	{ 0x103800, 0x103fff, MWA16_RAM },
	{ 0x104000, 0x104fff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
	{ 0x105000, 0x105fff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x106000, 0x1067ff, legionna_foreground_w, &legionna_fore_data },
	{ 0x106800, 0x106fff, MWA16_RAM },
	{ 0x107000, 0x107fff, MWA16_RAM },
	{ 0x108000, 0x11ffff, MWA16_RAM },
MEMORY_END

static MEMORY_READ16_START( sdgndmrb_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM },
	{ 0x100000, 0x1007ff, sdgndmrb_cop_mcu_r },	/* COP mcu */
	{ 0x100800, 0x100fff, MRA16_RAM },	/* 32x16 bg layer, 16x16 tiles */
	{ 0x101000, 0x1017ff, MRA16_RAM },	/* 32x16 bg layer, 16x16 tiles */
	{ 0x101800, 0x101fff, MRA16_RAM },	/* 32x16 bg layer, 16x16 tiles */
	{ 0x102000, 0x102fff, MRA16_RAM },	/* 64x32 text/front layer, 8x8 tiles */
	{ 0x103000, 0x103fff, MRA16_RAM },	/* spriteram */
	{ 0x104000, 0x104fff, MRA16_RAM },	/* palette */
	{ 0x105000, 0x105fff, MRA16_RAM },
	{ 0x107000, 0x107fff, MRA16_RAM },	/* extra spriteram? */
	{ 0x108000, 0x11ffff, MRA16_RAM },	/* main ram */
MEMORY_END

static MEMORY_WRITE16_START( sdgndmrb_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM },
	{ 0x100000, 0x1007ff, sdgndmrb_cop_mcu_w, &mcu_ram },	/* COP mcu */
	{ 0x100800, 0x100fff, legionna_background_w, &legionna_back_data },
	{ 0x101000, 0x1017ff, legionna_foreground_w, &legionna_fore_data },
	{ 0x101800, 0x101fff, legionna_midground_w,  &legionna_mid_data },
	{ 0x102000, 0x102fff, legionna_text_w, &legionna_textram },
	{ 0x103000, 0x103fff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
	{ 0x104000, 0x104fff, paletteram16_xBBBBBGGGGGRRRRR_word_w },
	{ 0x105000, 0x105fff, MWA16_RAM },
	{ 0x106000, 0x106fff, MWA16_RAM },
	{ 0x107000, 0x107fff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x108000, 0x11ffff, MWA16_RAM },
MEMORY_END

static MEMORY_READ16_START( cupsoc_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM },
	{ 0x100000, 0x1007ff, sdgndmrb_cop_mcu_r },	/* COP mcu */
	{ 0x100800, 0x100fff, MRA16_RAM },
	{ 0x101000, 0x101fff, MRA16_RAM },
	{ 0x102000, 0x102fff, MRA16_RAM },
	{ 0x103000, 0x103fff, MRA16_RAM },
	{ 0x104000, 0x104fff, MRA16_RAM },
	{ 0x105000, 0x105fff, MRA16_RAM },
	{ 0x106000, 0x106fff, MRA16_RAM },
	{ 0x107000, 0x107fff, MRA16_RAM },
	{ 0x108000, 0x11ffff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( cupsoc_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM },
	{ 0x100000, 0x1007ff, sdgndmrb_cop_mcu_w, &mcu_ram },	/* COP mcu */
	{ 0x100800, 0x100fff, legionna_background_w, &legionna_back_data },
	{ 0x101000, 0x1017ff, legionna_foreground_w, &legionna_fore_data },
	{ 0x101800, 0x101fff, legionna_midground_w,  &legionna_mid_data },
	{ 0x102000, 0x102fff, legionna_text_w, &legionna_textram },
	{ 0x103000, 0x103fff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 }, 
	{ 0x104000, 0x104fff, MWA16_RAM },
	{ 0x105000, 0x105fff, MWA16_RAM },
	{ 0x106000, 0x106fff, MWA16_RAM },
	{ 0x107000, 0x107fff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x108000, 0x11ffff, MWA16_RAM },	
MEMORY_END

/*****************************************************************************/

INPUT_PORTS_START( legionna )
	SEIBU_COIN_INPUTS	/* Must be port 0: coin inputs read through sound cpu */

	PORT_START
	PORT_DIPNAME( 0x001f, 0x001f, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0015, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(      0x0017, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0019, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x001b, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 8C_3C ) )
	PORT_DIPSETTING(      0x001d, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 5C_3C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x001f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0009, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0013, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0011, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x000f, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x000d, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x000b, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x001e, "A 1/1 B 1/2" )
	PORT_DIPSETTING(      0x0014, "A 2/1 B 1/3" )
	PORT_DIPSETTING(      0x000a, "A 3/1 B 1/5" )
	PORT_DIPSETTING(      0x0000, "A 5/1 B 1/6" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Freeze" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0200, "1" )
	PORT_DIPSETTING(      0x0300, "2" )
	PORT_DIPSETTING(      0x0100, "3" )
	PORT_BITX( 0,         0x0000, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x0400, 0x0400, "Extend" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x3000, 0x3000, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x2000, "Easy" )
	PORT_DIPSETTING(      0x3000, "Medium" )
	PORT_DIPSETTING(      0x1000, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x4000, 0x4000, "Allow Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW,  IPT_BUTTON1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW,  IPT_BUTTON2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
/*	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )*/
INPUT_PORTS_END


INPUT_PORTS_START( heatbrl )
	SEIBU_COIN_INPUTS	/* Must be port 0: coin inputs read through sound cpu */

	PORT_START
	PORT_DIPNAME( 0x001f, 0x001f, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0015, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(      0x0017, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0019, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x001b, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 8C_3C ) )
	PORT_DIPSETTING(      0x001d, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 5C_3C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x001f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0009, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0013, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0011, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x000f, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x000d, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x000b, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x001e, "A 1/1 B 1/2" )
	PORT_DIPSETTING(      0x0014, "A 2/1 B 1/3" )
	PORT_DIPSETTING(      0x000a, "A 3/1 B 1/5" )
	PORT_DIPSETTING(      0x0000, "A 5/1 B 1/6" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Freeze" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0200, "1" )
	PORT_DIPSETTING(      0x0300, "2" )
	PORT_DIPSETTING(      0x0100, "3" )
	PORT_BITX( 0,         0x0000, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x0400, 0x0400, "Extend" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x3000, 0x3000, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x2000, "Easy" )
	PORT_DIPSETTING(      0x3000, "Medium" )
	PORT_DIPSETTING(      0x1000, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x4000, 0x4000, "Allow Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN4 )	/* haven't found coin4, maybe it doesn't exist*/
/*	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )*/

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER3  )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_PLAYER3  )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
INPUT_PORTS_END


INPUT_PORTS_START( godzilla )
	SEIBU_COIN_INPUTS	/* Must be port 0: coin inputs read through sound cpu */

	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Service_Mode ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER3  )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_PLAYER3  )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( sdgndmrb )
	SEIBU_COIN_INPUTS	/* Must be port 0: coin inputs read through sound cpu */

	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Service_Mode ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER3  )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_PLAYER3  )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( cupsoc )
	SEIBU_COIN_INPUTS	/* Must be port 0: coin inputs read through sound cpu */

	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER3  )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_PLAYER3  )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	  0x0003, "Normal" )
	PORT_DIPSETTING(      0x0002, "Easy" )
	PORT_DIPSETTING(      0x0001, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x000c, 0x0000, "Players / Coin Mode" )
	PORT_DIPSETTING(      0x0000, "4 Players / 1 Coin Slot" )
	PORT_DIPSETTING(      0x0004, "4 Players / 4 Coin Slots" )
	PORT_DIPSETTING(      0x0008, "4 Players / 2 Coin Slots" )
	PORT_DIPSETTING(      0x000c, "2 Players" )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	  0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Service_Mode ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0xffc0, 0xffc0, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0xffc0, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END

/*****************************************************************************/


static struct GfxLayout legionna_charlayout =
{
	8,8,
	RGN_FRAC(1,4),	/* other half is BK3, decoded in char2layout */
	4,
	{ 0, 4, 4096*16*8+0, 4096*16*8+4 },
	{ 3, 2, 1, 0, 8+3, 8+2, 8+1, 8+0 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8
};

static struct GfxLayout heatbrl_charlayout =
{
	8,8,
	RGN_FRAC(1,2),	/* second half is junk, like legionna we may need a different decode */
	4,
	{ 0, 4, 4096*16*8+0, 4096*16*8+4 },
	{ 3, 2, 1, 0, 8+3, 8+2, 8+1, 8+0 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8
};


static struct GfxLayout legionna_char2layout =
{
	16,16,
	256,	/* Can't use RGN_FRAC as (1,16) not supported */
	4,
	{ 0, 4, 4096*16*8+0, 4096*16*8+4 },
	{ 3, 2, 1, 0, 11, 10, 9, 8,
	  1024*16*8 +3,  1024*16*8 +2,  1024*16*8 +1, 1024*16*8 +0,
	  1024*16*8 +11, 1024*16*8 +10, 1024*16*8 +9, 1024*16*8 +8 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
	  512*16*8 +0*16, 512*16*8 +1*16, 512*16*8 +2*16, 512*16*8 +3*16,
	  512*16*8 +4*16, 512*16*8 +5*16, 512*16*8 +6*16, 512*16*8 +7*16 },
	16*8
};

static struct GfxLayout legionna_tilelayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 2*4, 3*4, 0*4, 1*4 },
	{ 3, 2, 1, 0, 16+3, 16+2, 16+1, 16+0,
	  64*8+3, 64*8+2, 64*8+1, 64*8+0, 64*8+16+3, 64*8+16+2, 64*8+16+1, 64*8+16+0 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
	  8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	128*8
};

static struct GfxLayout legionna_spritelayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 2*4, 3*4, 0*4, 1*4 },
	{ 3, 2, 1, 0, 16+3, 16+2, 16+1, 16+0,
	  64*8+3, 64*8+2, 64*8+1, 64*8+0, 64*8+16+3, 64*8+16+2, 64*8+16+1, 64*8+16+0 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
	  8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	128*8
};

static struct GfxDecodeInfo legionna_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &legionna_charlayout,   48*16, 16 },
	{ REGION_GFX3, 0, &legionna_tilelayout,    0*16, 16 },
	{ REGION_GFX4, 0, &legionna_char2layout,  32*16, 16 },	/* example BK3 decode */
	{ REGION_GFX2, 0, &legionna_spritelayout,  0*16, 8*16 },
	{ REGION_GFX5, 0, &legionna_tilelayout,   32*16, 16 },	/* this should be the BK3 decode */
	{ REGION_GFX6, 0, &legionna_tilelayout,   16*16, 16 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo heatbrl_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &heatbrl_charlayout,    48*16, 16 },
	{ REGION_GFX3, 0, &legionna_tilelayout,    0*16, 16 },
	{ REGION_GFX4, 0, &legionna_char2layout,  32*16, 16 },	/* unused */
	{ REGION_GFX2, 0, &legionna_spritelayout,  0*16, 8*16 },
	{ REGION_GFX5, 0, &legionna_tilelayout,   32*16, 16 },
	{ REGION_GFX6, 0, &legionna_tilelayout,   16*16, 16 },
	{ -1 } /* end of array */
};

/*****************************************************************************/

/* Parameters: YM3812 frequency, Oki frequency, Oki memory region */
SEIBU_SOUND_SYSTEM_YM3812_HARDWARE(14318180/4,8000,REGION_SOUND1);

SEIBU_SOUND_SYSTEM_YM2151_HARDWARE(14318180/4,8000,REGION_SOUND1);

/*****************************************************************************/

static MACHINE_DRIVER_START( legionna )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,20000000/2) 	/* ??? */
	MDRV_CPU_MEMORY(legionna_readmem,legionna_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)/* VBL */

	SEIBU_SOUND_SYSTEM_CPU(14318180/4)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(seibu_sound_1)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_BUFFERS_SPRITERAM)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(legionna_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(128*16)

	MDRV_VIDEO_START(legionna)
	MDRV_VIDEO_UPDATE(legionna)

	/* sound hardware */
	SEIBU_SOUND_SYSTEM_YM3812_INTERFACE
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( heatbrl )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,20000000/2) 	/* ??? */
	MDRV_CPU_MEMORY(heatbrl_readmem,heatbrl_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)/* VBL */

	SEIBU_SOUND_SYSTEM_CPU(14318180/4)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(seibu_sound_1)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_BUFFERS_SPRITERAM)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 32*8-1)
	MDRV_GFXDECODE(heatbrl_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(128*16)

	MDRV_VIDEO_START(legionna)
	MDRV_VIDEO_UPDATE(legionna)

	/* sound hardware */
	SEIBU_SOUND_SYSTEM_YM3812_INTERFACE
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( godzilla )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 20000000/2)
	MDRV_CPU_MEMORY(godzilla_readmem,godzilla_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)

	SEIBU2_SOUND_SYSTEM_CPU(14318180/4)

	MDRV_FRAMES_PER_SECOND(61)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(seibu_sound_1)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_BUFFERS_SPRITERAM)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(heatbrl_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(128*16)

	MDRV_VIDEO_START(legionna)
	MDRV_VIDEO_UPDATE(godzilla)

	/* sound hardware */
	SEIBU_SOUND_SYSTEM_YM2151_INTERFACE
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( sdgndmrb )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 20000000/2)
	MDRV_CPU_MEMORY(sdgndmrb_readmem,sdgndmrb_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)

	SEIBU2_SOUND_SYSTEM_CPU(14318180/4)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(seibu_sound_1)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_BUFFERS_SPRITERAM)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(heatbrl_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(128*16)

	MDRV_VIDEO_START(legionna)
	MDRV_VIDEO_UPDATE(sdgndmrb)

	/* sound hardware */
	SEIBU_SOUND_SYSTEM_YM2151_INTERFACE
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( cupsoc )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,20000000/2)
	MDRV_CPU_MEMORY(cupsoc_readmem,cupsoc_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)/* VBL */

	SEIBU_SOUND_SYSTEM_CPU(14318180/4)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(seibu_sound_1)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_BUFFERS_SPRITERAM)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(heatbrl_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(128*16)

	MDRV_VIDEO_START(cupsoc)
	MDRV_VIDEO_UPDATE(sdgndmrb)

	/* sound hardware */
	SEIBU_SOUND_SYSTEM_YM3812_INTERFACE
MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( legionna )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD32_BYTE( "1",           0x00000, 0x20000, CRC(9e2d3ec8) SHA1(8af9ca349389cbbd2b541aafa09de57f87f6fd72) )
	ROM_LOAD32_BYTE( "2",           0x00001, 0x20000, CRC(35c8a28f) SHA1(31a1f2f9e04dfcab4b3357d6d27c24b434a8c14b) )
	ROM_LOAD32_BYTE( "3",           0x00002, 0x20000, CRC(553fc7c0) SHA1(b12a2eea6b2c9bd76c0c74ddf2765d58510f586a) )
	ROM_LOAD32_BYTE( "legion4.bin", 0x00003, 0x20000, CRC(2cc36c98) SHA1(484fc6eeeed89386ec69df0f92919b742cfdd89f) )

	ROM_REGION( 0x20000*2, REGION_CPU2, 0 )	/* Z80 code, banked data */
	ROM_LOAD( "6",   0x00000, 0x08000, CRC(fe7b8d06) SHA1(1e5b52ea4b4042940e2ee2db75c7c0f24973422a) )
	ROM_CONTINUE(    0x10000, 0x08000 )	/* banked stuff */

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "7",   0x000000, 0x10000, CRC(88e26809) SHA1(40ee55d3b5329b6f657e0621d93c4caf6a035fdf) )	/* chars, some BK3 tiles too */
	ROM_LOAD( "8",   0x010000, 0x10000, CRC(06e35407) SHA1(affeeb97b7f3cfa9b65a584ebe25c16a5b2c9a89) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "obj1",     0x000000, 0x100000, CRC(d35602f5) SHA1(79379abf1c8131df47f81f42b2dc6876926a4e9d) )	/* sprites */
	ROM_LOAD( "obj2",     0x100000, 0x100000, CRC(351d3917) SHA1(014562ac55c09227c08275df3129df19d81af164) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "back",     0x000000, 0x100000, CRC(58280989) SHA1(e3eef1f52829a91b8f87cfe27776a1f12679b3ca) )	/* 3 sets of tiles ('MBK','LBK','BK3') */

	ROM_REGION( 0x020000, REGION_GFX4, ROMREGION_DISPOSE )	/* example BK3 decode */
	ROM_COPY( REGION_GFX1, 0x00000, 0x00000, 0x20000 )

	ROM_REGION( 0x020000, REGION_GFX5, ROMREGION_DISPOSE )	/* we _should_ decode all BK3 tiles here */

	ROM_REGION( 0x080000, REGION_GFX6, ROMREGION_DISPOSE )	/* LBK tiles (plus BK3 at end) */
	ROM_COPY( REGION_GFX3, 0x80000, 0x00000, 0x80000 )

	ROM_REGION( 0x020000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "5",   0x00000, 0x20000, CRC(21d09bde) SHA1(8dce5011e083706ac7b57c5aee4b79d30fa8d4cb) )
ROM_END

ROM_START( legionnu )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD32_BYTE( "1",   0x00000, 0x20000, CRC(9e2d3ec8) SHA1(8af9ca349389cbbd2b541aafa09de57f87f6fd72) )
	ROM_LOAD32_BYTE( "2",   0x00001, 0x20000, CRC(35c8a28f) SHA1(31a1f2f9e04dfcab4b3357d6d27c24b434a8c14b) )
	ROM_LOAD32_BYTE( "3",   0x00002, 0x20000, CRC(553fc7c0) SHA1(b12a2eea6b2c9bd76c0c74ddf2765d58510f586a) )
	ROM_LOAD32_BYTE( "4",   0x00003, 0x20000, CRC(91fd4648) SHA1(8ad6d0512996b88d3c0c7a96912eebaae2333424) )

	ROM_REGION( 0x20000*2, REGION_CPU2, 0 )	/* Z80 code, banked data */
	ROM_LOAD( "6",   0x00000, 0x08000, CRC(fe7b8d06) SHA1(1e5b52ea4b4042940e2ee2db75c7c0f24973422a) )
	ROM_CONTINUE(    0x10000, 0x08000 )	/* banked stuff */

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "7",   0x000000, 0x10000, CRC(88e26809) SHA1(40ee55d3b5329b6f657e0621d93c4caf6a035fdf) )	/* chars, some BK3 tiles too */
	ROM_LOAD( "8",   0x010000, 0x10000, CRC(06e35407) SHA1(affeeb97b7f3cfa9b65a584ebe25c16a5b2c9a89) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "obj1",     0x000000, 0x100000, CRC(d35602f5) SHA1(79379abf1c8131df47f81f42b2dc6876926a4e9d) )	/* sprites */
	ROM_LOAD( "obj2",     0x100000, 0x100000, CRC(351d3917) SHA1(014562ac55c09227c08275df3129df19d81af164) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "back",     0x000000, 0x100000, CRC(58280989) SHA1(e3eef1f52829a91b8f87cfe27776a1f12679b3ca) )	/* 3 sets of tiles ('MBK','LBK','BK3') */

	ROM_REGION( 0x020000, REGION_GFX4, ROMREGION_DISPOSE )	/* example BK3 decode */
	ROM_COPY( REGION_GFX1, 0x00000, 0x00000, 0x20000 )

	ROM_REGION( 0x020000, REGION_GFX5, ROMREGION_DISPOSE )	/* we _should_ decode all BK3 tiles here */

	ROM_REGION( 0x080000, REGION_GFX6, ROMREGION_DISPOSE )	/* LBK tiles (plus BK3 at end) */
	ROM_COPY( REGION_GFX3, 0x80000, 0x00000, 0x80000 )

	ROM_REGION( 0x020000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "5",   0x00000, 0x20000, CRC(21d09bde) SHA1(8dce5011e083706ac7b57c5aee4b79d30fa8d4cb) )
ROM_END

ROM_START( heatbrl )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD32_BYTE( "1e_ver2.9k",   0x00000, 0x20000, CRC(b30bd632) SHA1(8684dd4787929886b0bce283301e492206ade9d9) )
	ROM_LOAD32_BYTE( "2e_ver2.9m",   0x00001, 0x20000, CRC(f3a23056) SHA1(d8840468535ac59fede60ea5a2928410d9c7a33a) )
	ROM_LOAD32_BYTE( "3e_ver2.9f",   0x00002, 0x20000, CRC(a2c41715) SHA1(a15b7a35ae0792ed00c47426d2e07c445acd8b8d) )
	ROM_LOAD32_BYTE( "4e_ver2.9h",   0x00003, 0x20000, CRC(a50f4f08) SHA1(f468e4a016a53803b8404bacdef5712311c6f0ac) )

	ROM_REGION( 0x20000*2, REGION_CPU2, 0 )	/* Z80 code, banked data */
	ROM_LOAD( "barrel.7",   0x00000, 0x08000, CRC(0784dbd8) SHA1(bdf7f8a3a3eb346eb2aeaf4f9bfc49af059d04c9) )
	ROM_CONTINUE(    0x10000, 0x08000 )	/* banked stuff */

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "barrel.6",   0x000000, 0x10000, CRC(bea3c581) SHA1(7f7f0a74bf106acaf57c182d47f0c707da2011bd) )	/* chars */
	ROM_LOAD( "barrel.5",   0x010000, 0x10000, CRC(5604d155) SHA1(afc30347b1e1316ec25056c0c1576f78be5f1a72) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "obj1",     0x000000, 0x100000, CRC(f7a7c31c) SHA1(683e5c7a0732ff5fd56167dd82035ca050de0507) )	/* sprites */
	ROM_LOAD( "obj2",     0x100000, 0x100000, CRC(24236116) SHA1(b27bd771cacd1587d4927e3f489c4f54b5dec110) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )	/* MBK tiles */
	ROM_LOAD( "bg-1",     0x000000, 0x100000, CRC(2f5d8baa) SHA1(0bf687c46c603150eadb304adcd78d53a338e615) )

	ROM_REGION( 0x020000, REGION_GFX4, ROMREGION_DISPOSE )	/* not used */

	ROM_REGION( 0x080000, REGION_GFX5, ROMREGION_DISPOSE )	/* BK3 tiles */
	ROM_LOAD( "bg-3",     0x000000, 0x080000, CRC(83850e2d) SHA1(cdc2df8e3bc58319c50768ea2a05b9c7ddc2a652) )

	ROM_REGION( 0x080000, REGION_GFX6, ROMREGION_DISPOSE )	/* LBK tiles */
	ROM_LOAD( "bg-2",     0x000000, 0x080000, CRC(77ee4c6f) SHA1(a0072331bc970ba448ac5bb1ae5caa0332c82a99) )

	ROM_REGION( 0x020000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "barrel.8",  0x00000, 0x20000, CRC(489e5b1d) SHA1(ecd69d87ed354d1d08dbe6c2890af5f05d9d67d0) )
ROM_END

ROM_START( heatbrlo )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD32_BYTE( "barrel.1h",   0x00000, 0x20000, CRC(d5a85c36) SHA1(421a42863faa940057ed5637748f791152a15502) )
	ROM_LOAD32_BYTE( "barrel.2h",   0x00001, 0x20000, CRC(5104d463) SHA1(f65ee824508da431567661804f6235b61425b2dd) )
	ROM_LOAD32_BYTE( "barrel.3h",   0x00002, 0x20000, CRC(823373a0) SHA1(1bb7f811df4f85db8ca10e59fe22137a09470def) )
	ROM_LOAD32_BYTE( "barrel.4h",   0x00003, 0x20000, CRC(19a8606b) SHA1(6e950212c532e46bb6645c3c1f8205c2a4ea2c87) )

	ROM_REGION( 0x20000*2, REGION_CPU2, 0 )	/* Z80 code, banked data */
	ROM_LOAD( "barrel.7",   0x00000, 0x08000, CRC(0784dbd8) SHA1(bdf7f8a3a3eb346eb2aeaf4f9bfc49af059d04c9) )
	ROM_CONTINUE(    0x10000, 0x08000 )	/* banked stuff */

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "barrel.6",   0x000000, 0x10000, CRC(bea3c581) SHA1(7f7f0a74bf106acaf57c182d47f0c707da2011bd) )	/* chars */
	ROM_LOAD( "barrel.5",   0x010000, 0x10000, CRC(5604d155) SHA1(afc30347b1e1316ec25056c0c1576f78be5f1a72) )

/* Sprite + tilemap gfx roms not dumped, for now we use ones from heatbrlu
Readme mentions as undumped:
barrel1,2,3,4.OBJ
barrel1,2,3,4.BG */

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "obj1",     0x000000, 0x100000, CRC(f7a7c31c) SHA1(683e5c7a0732ff5fd56167dd82035ca050de0507) )	/* sprites */
	ROM_LOAD( "obj2",     0x100000, 0x100000, CRC(24236116) SHA1(b27bd771cacd1587d4927e3f489c4f54b5dec110) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )	/* MBK tiles */
	ROM_LOAD( "bg-1",     0x000000, 0x100000, CRC(2f5d8baa) SHA1(0bf687c46c603150eadb304adcd78d53a338e615) )

	ROM_REGION( 0x020000, REGION_GFX4, ROMREGION_DISPOSE )	/* not used */

	ROM_REGION( 0x080000, REGION_GFX5, ROMREGION_DISPOSE )	/* BK3 tiles */
	ROM_LOAD( "bg-3",     0x000000, 0x080000, CRC(83850e2d) SHA1(cdc2df8e3bc58319c50768ea2a05b9c7ddc2a652) )

	ROM_REGION( 0x080000, REGION_GFX6, ROMREGION_DISPOSE )	/* LBK tiles */
	ROM_LOAD( "bg-2",     0x000000, 0x080000, CRC(77ee4c6f) SHA1(a0072331bc970ba448ac5bb1ae5caa0332c82a99) )

	ROM_REGION( 0x020000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "barrel.8",  0x00000, 0x20000, CRC(489e5b1d) SHA1(ecd69d87ed354d1d08dbe6c2890af5f05d9d67d0) )
ROM_END

ROM_START( heatbrlu )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD32_BYTE( "1e_ver2.9k",   0x00000, 0x20000, CRC(b30bd632) SHA1(8684dd4787929886b0bce283301e492206ade9d9) )
	ROM_LOAD32_BYTE( "2u",           0x00001, 0x20000, CRC(289dd629) SHA1(fb379e067ffee4e54d55da638e45e22d6b2ef788) )
	ROM_LOAD32_BYTE( "3e_ver2.9f",   0x00002, 0x20000, CRC(a2c41715) SHA1(a15b7a35ae0792ed00c47426d2e07c445acd8b8d) )
	ROM_LOAD32_BYTE( "4e_ver2.9h",   0x00003, 0x20000, CRC(a50f4f08) SHA1(f468e4a016a53803b8404bacdef5712311c6f0ac) )

	ROM_REGION( 0x20000*2, REGION_CPU2, 0 )	/* Z80 code, banked data */
	ROM_LOAD( "barrel.7",   0x00000, 0x08000, CRC(0784dbd8) SHA1(bdf7f8a3a3eb346eb2aeaf4f9bfc49af059d04c9) )
	ROM_CONTINUE(    0x10000, 0x08000 )	/* banked stuff */

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "barrel.6",   0x000000, 0x10000, CRC(bea3c581) SHA1(7f7f0a74bf106acaf57c182d47f0c707da2011bd) )	/* chars */
	ROM_LOAD( "barrel.5",   0x010000, 0x10000, CRC(5604d155) SHA1(afc30347b1e1316ec25056c0c1576f78be5f1a72) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "obj1",     0x000000, 0x100000, CRC(f7a7c31c) SHA1(683e5c7a0732ff5fd56167dd82035ca050de0507) )	/* sprites */
	ROM_LOAD( "obj2",     0x100000, 0x100000, CRC(24236116) SHA1(b27bd771cacd1587d4927e3f489c4f54b5dec110) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )	/* MBK tiles */
	ROM_LOAD( "bg-1",     0x000000, 0x100000, CRC(2f5d8baa) SHA1(0bf687c46c603150eadb304adcd78d53a338e615) )

	ROM_REGION( 0x020000, REGION_GFX4, ROMREGION_DISPOSE )	/* not used */

	ROM_REGION( 0x080000, REGION_GFX5, ROMREGION_DISPOSE )	/* BK3 tiles */
	ROM_LOAD( "bg-3",     0x000000, 0x080000, CRC(83850e2d) SHA1(cdc2df8e3bc58319c50768ea2a05b9c7ddc2a652) )

	ROM_REGION( 0x080000, REGION_GFX6, ROMREGION_DISPOSE )	/* LBK tiles */
	ROM_LOAD( "bg-2",     0x000000, 0x080000, CRC(77ee4c6f) SHA1(a0072331bc970ba448ac5bb1ae5caa0332c82a99) )

	ROM_REGION( 0x020000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "barrel.8",  0x00000, 0x20000, CRC(489e5b1d) SHA1(ecd69d87ed354d1d08dbe6c2890af5f05d9d67d0) )
ROM_END

/*

Godzilla
Banpresto 1993

This game runs on Seibu hardware, similar to Legionairre.

PCB Layout
|----------------------------------------------------|
|   YM2151  M6295  PCM.922   8    Z80          PAL   |
|     YM3014                                   PAL   |
|           YM3931  SEI0220  14.31818MHz  6116       |
|         (SEI0100)                                  |
|                                OBJ1.648   OBJ3.743 |
|                      SEI0211                       |
|J                               OBJ2.756   OBJ4.757 |
|A                                                   |
|M     DSW1-8                 2.025  4.026  62256    |
|M     DSW2-8                               62256    |
|A           S68E08.844 PAL   1.024  3.023  62256    |
|      6264                   PAL  PAL      62256    |
|      6264                                          |
| SEI0200                   COPX-D2.313   SEI0300    |
| TC110G21AF                              TC25SC900AF|
|                                                    |
|BK1.618             11.620  20MHz   PAL   PAL       |
|          BK3.619   10.615          PAL   68000     |
|----------------------------------------------------|

Notes:
      Z80 clock    : 3.579545MHz
      68000 clock  : 10.000MHz
      YM2151 clock : 3.579545MHz
      M6295 clock  : 1.000MHz, sample rate = clk /132
      VSync        : 61Hz
      HSync        : 15.74kHz
      S68E08.844   : 82S147 bipolar PROM
      BK & OBJ     : 8M MASK (read as 238000)
      Main PRG 1-4 : 27C010 EPROM
      ROMs 8,10,11 : 27C512 EPROM
      COPX-D2.313  : 4M MASK (read as 234200)
      PCM          : 4M MASK (read as 27C040)
      
      Custom SEIBU chips:
                         SEI0211 and SEI0220: connected to OBJ ROMs
                         SEI300             : connected to PRG ROMs and 68000
                         SEI0200            : connected to BG ROMs

*/

ROM_START( godzilla )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD32_BYTE( "2.025",        0x000000, 0x020000, CRC(be9c6e5a) SHA1(9a7e49ac9cdbcc02b13b3448544cee5fe398ec16) )
	ROM_LOAD32_BYTE( "1.024",        0x000001, 0x020000, CRC(0d6b663d) SHA1(01e02999cffd2642f7a37e492fe7f83770cddd67) )
	ROM_LOAD32_BYTE( "4.026",        0x000002, 0x020000, CRC(bb8c0132) SHA1(fa8b049f590be710b3cf82f27deade63656db730) )
	ROM_LOAD32_BYTE( "3.023",        0x000003, 0x020000, CRC(bb16e5d0) SHA1(31d8941e6e297b1f410944f0063a4c9219d23f23) )

	ROM_REGION( 0x20000*2, REGION_CPU2, 0 )	/* Z80 code, banked data */
	ROM_LOAD( "8.016",        0x000000, 0x08000, CRC(4ab76e43) SHA1(40c34fade03161c4b50f9f6a2ae61078b8d8ea6d) )
	ROM_CONTINUE(			  0x010000, 0x08000 )	/* banked stuff */
	
	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "11.620",       0x000000, 0x010000, CRC(58e0e41f) SHA1(563c633eb3d4df41e467c93957c74b540a0ae43c) )
	ROM_LOAD( "10.615",       0x010000, 0x010000, CRC(9c22bc13) SHA1(a94d9ed63ee1f5e358ebcaf517e6a1c986fa5d96) )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "obj1.748",     0x000000, 0x100000, CRC(146bacb0) SHA1(1331f04f3d9e6236cec7524e9da1782ed1916ff7) )
	ROM_LOAD( "obj2.756",     0x100000, 0x100000, CRC(91c2a6a5) SHA1(0e9d9d94c3d99a54c6f9f99270e65682eb0a8b6a) )
	ROM_LOAD( "obj3.743",     0x200000, 0x100000, CRC(5af0114e) SHA1(9362de9ade6db67ab0e3a2dfea580e688bbf7729) )
	ROM_LOAD( "obj4.757",     0x300000, 0x100000, CRC(7448b054) SHA1(5c08319329eb8c90b63e5393c0011bc39911ebbb) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )	/* MBK tiles */
	ROM_LOAD( "bg1.618",      0x000000, 0x100000, CRC(78fbbb84) SHA1(b1f5d4041bb88c5b2a561949239b11c3fd7c5fbc) )

	ROM_REGION( 0x020000, REGION_GFX4, ROMREGION_DISPOSE )	/* not used */

	ROM_REGION( 0x100000, REGION_GFX5, ROMREGION_DISPOSE )	/* BK3 tiles */
	ROM_LOAD( "bg2.619",      0x000000, 0x100000, CRC(8ac192a5) SHA1(54b557e81a704c70a651e6b8da70207a2a70530f) )

	ROM_REGION( 0x100000, REGION_GFX6, ROMREGION_DISPOSE )	/* LBK tiles */
	ROM_COPY( REGION_GFX3, 0x00000, 0x00000, 0x100000 )

	ROM_REGION( 0x080000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "pcm.922",      0x000000, 0x080000, CRC(59cbef10) SHA1(6b89b7286f80f9c903dfb81dc93a03c38dff707c) )

	ROM_REGION( 0x200, REGION_PROMS, 0 )
	ROM_LOAD( "s68e08.844",   0x000000, 0x000200, CRC(96f7646e) SHA1(400a831b83d6ac4d2a46ef95b97b1ee237099e44) ) /* Priority */

	ROM_REGION( 0x080000, REGION_USER1, 0 )
	ROM_LOAD( "copx-d2.313",  0x000000, 0x080000, CRC(7c52581b) SHA1(7e668476f886806b0c06fa0bcf4bbc955878c87c) )
ROM_END

/*

SD Gundam Sangokushi Rainbow Tairiku Senki
(c)1993 Banpresto
TYPE-R
Board made by Seibu?

CPU  : MC68000P10
Sound: Z80A YM2151 M6295 Y3014B
OSC  : 20.0000MHz (X11), 14.31818MHz (X71)

ROMs:
rb-p1.25 - Main programs (27c020)
rb-p2.24 |
rb-p3.26 |
rb-p4.23 /

rb-s.016 - Sound program (27c512)
rb-ad.922 - Sound data (27c1001)

rb-bg-01.618 - Background (TC538200AP)
rb-bg-2.619  |
rb-f1.620    | (27c512)
rb-f2.615    /

rb-spr01.748 - Sprites (TC538200AP)
rb-spr23.756 /

copx-d2.313 - ? (2M-16bit)

s68e08.844 - (N82S147N)

PALs:
s68e01.122
s68e02.310 (16L8ACN)
s68e03.322 (16L8ACN)
s68e04.551
s68e05.552
s68e06r.741 (18CV8)
s68e07.842
s68e09.015 (16L8ACN)


Custom chips:
SEI0100BU YM3931 9149 EALA
SEI0220BP JAPAN S 9208 U ("S" for "Sharp")
SEI0211 9215 ABBB
SEI0200 TC110G21AF 0076 9324EAI JAPAN
SEI300 TC25SC900AF 001 9211EAI JAPAN

*/

ROM_START( sdgndmrb )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD32_BYTE( "rb-p1.25",     0x000000, 0x040000, CRC(0995c511) SHA1(97fb2bd7d26720552ace25e655fce09ad9a7afd7) )
	ROM_LOAD32_BYTE( "rb-p2.24",     0x000001, 0x040000, CRC(c9eb756f) SHA1(88d784a71bfab4f321d3320aed1b6b2648529979) )
	ROM_LOAD32_BYTE( "rb-p3.26",     0x000002, 0x040000, CRC(fe2f08a8) SHA1(bb95e5c113a0343b6da43c5dca1292601dec00eb) )
	ROM_LOAD32_BYTE( "rb-p4.23",     0x000003, 0x040000, CRC(f558962a) SHA1(fcfb6f2cba59effd14c76602b0f87f564235d8ef) )

	ROM_REGION( 0x20000, REGION_CPU2, 0 ) /* 64k code for sound Z80 */
	ROM_LOAD( "rb-s.016",     0x000000, 0x08000, CRC(8439bf5b) SHA1(089009b91768d64edef6639e7694723d2d1c46ff) )
	ROM_CONTINUE(             0x010000, 0x08000 )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rb-f1.620",    0x000000, 0x010000, CRC(792c403d) SHA1(3c606af696fe8f3d6edefdab3940bd5eb341bca9) )
	ROM_LOAD( "rb-f2.615",    0x010000, 0x010000, CRC(a30e0903) SHA1(b9e7646da1ccab6dadaca6beda08125b34946653) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "rb-spr01.748", 0x000000, 0x100000, CRC(11a3479d) SHA1(4d2d06d62da02c6e9884735de8c319f37ca1715c) )
	ROM_LOAD( "rb-spr23.756", 0x100000, 0x100000, CRC(fd08a761) SHA1(3297a2bfaabef17ed9320e24e9a4ffa2f3eb3a44) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "rb-bg-01.618", 0x000000, 0x100000, CRC(6a4ca7e7) SHA1(13612d29f8f04cf62b4357b69b81240dd1eceae4) )

	ROM_REGION( 0x040000, REGION_GFX4, ROMREGION_DISPOSE )	/* not used */

	ROM_REGION( 0x100000, REGION_GFX5, ROMREGION_DISPOSE )
	ROM_LOAD( "rb-bg-2.619",  0x000000, 0x100000, CRC(a9b5c85e) SHA1(0ae044e05730e8080d94f1f6758f8dd051b03c41) )

	ROM_REGION( 0x100000, REGION_GFX6, ROMREGION_DISPOSE )
	ROM_COPY( REGION_GFX5, 0x00000, 0x00000, 0x100000 )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	 /* ADPCM samples */
	ROM_LOAD( "rb-ad.922",    0x000000, 0x020000, CRC(a364cb42) SHA1(c527b39a1627ecee20a2c4df4cf2b5f2ba729081) )

	ROM_REGION( 0x040000, REGION_USER1, 0 )
	ROM_LOAD( "copx-d2.313",  0x0000, 0x040000, CRC(a6732ff9) SHA1(c4856ec77869d9098da24b1bb3d7d58bb74b4cda) )
ROM_END

ROM_START( cupsoc )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD32_BYTE( "seibu1.10n",   0x000000, 0x040000, CRC(e91fdc95) SHA1(71c56fffabca79e73dfc61aad17bc58e09a28680) )
	ROM_LOAD32_BYTE( "seibu2.10q",   0x000001, 0x040000, CRC(7816df3c) SHA1(d5cfbf493cc00c47406b314c08e9cbf159a7f98c) )
	ROM_LOAD32_BYTE( "seibu3.10f",   0x000002, 0x040000, CRC(3be8a330) SHA1(f821080acd29c5801abc36da3341aabaea82ceb0) )
	ROM_LOAD32_BYTE( "seibu4.10k",   0x000003, 0x040000, CRC(f30167ea) SHA1(5431296e3245631c90362373027c54166f8fba16) )
	
	ROM_REGION( 0x20000*2, REGION_CPU2, 0 )	/* Z80 code, banked data */
	ROM_LOAD( "seibu7.8a",    0x000000, 0x08000, CRC(f63329f9) SHA1(51736de48efc14415cfdf169b43623d4c95fde2b) )
	ROM_CONTINUE(			  0x010000, 0x08000 )	/* banked stuff */

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "seibu6.7x",    0x000000, 0x010000, CRC(21c1e1b8) SHA1(30928c8ef98bf32ba0bf795ddadba1c95fcffe9d) )
	ROM_LOAD( "seibu5.7y",    0x010000, 0x010000, CRC(955d9fd7) SHA1(782451e8e85f7ba285d6cacd9d3fdcf48bde60bc) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "obj.8c",       0x000000, 0x100000, CRC(e2377895) SHA1(1d1c7f31a08a464139cdaf383a5e1ade0717dc9f) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )	/* MBK tiles */
	ROM_LOAD( "back-1.4y",    0x000000, 0x100000, CRC(3dfea0ec) SHA1(8f41d267e488e07831946ef898d593897f10bfe2) )

	ROM_REGION( 0x020000, REGION_GFX4, ROMREGION_DISPOSE )	/* not used */

	ROM_REGION( 0x080000, REGION_GFX5, ROMREGION_DISPOSE )	/* BK3 tiles */
	ROM_LOAD( "back-2.6y",    0x000000, 0x080000, CRC(e07712af) SHA1(2a0285d6a1e0141838e898252b8d922a6263b05f) )

	ROM_REGION( 0x080000, REGION_GFX6, ROMREGION_DISPOSE )	/* LBK tiles */
	ROM_COPY( REGION_GFX5, 0x00000, 0x00000, 0x080000 )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "seibu8.7a",    0x000000, 0x040000, CRC(6f594808) SHA1(218aa12068aa587c7656355f6a6b86d97c868774) )
ROM_END

ROM_START( cupsoc2 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD32_BYTE( "scc_01.bin",   0x000000, 0x040000, CRC(c122203c) SHA1(93c0ae90c0ed3889b9159774ba89536108c9b259) )
	ROM_LOAD32_BYTE( "scc_02.bin",   0x000001, 0x040000, CRC(105511b4) SHA1(f2ebe95a10f5928f57d4f532e2d2432f13b774b2) )
	ROM_LOAD32_BYTE( "scc_03.bin",   0x000002, 0x040000, CRC(2d23d78f) SHA1(c479ded8782f2d23e123b7d00ec57c18a8f80578) )
	ROM_LOAD32_BYTE( "scc_04.bin",   0x000003, 0x040000, CRC(e8877461) SHA1(3be44459699fd455b0daaac10e8a37d1b7985607) )

	ROM_REGION( 0x20000*2, REGION_CPU2, 0 )	/* Z80 code, banked data */
	ROM_LOAD( "seibu7.8a",    0x000000, 0x08000, CRC(f63329f9) SHA1(51736de48efc14415cfdf169b43623d4c95fde2b) )
	ROM_CONTINUE(			  0x010000, 0x08000 )	/* banked stuff */

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "scc_06.bin",   0x000000, 0x010000, CRC(f1a18ec6) SHA1(43f8ec3fc541b8dc2a17533329dd3448afadcb3b) )
	ROM_LOAD( "scc_05.bin",   0x010000, 0x010000, CRC(c0358503) SHA1(e87991c6a6f3e060a1b03b4899fa891510fca15f) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "obj.8c",       0x000000, 0x100000, CRC(e2377895) SHA1(1d1c7f31a08a464139cdaf383a5e1ade0717dc9f) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )	/* MBK tiles */
	ROM_LOAD( "back-1.4y",    0x000000, 0x100000, CRC(3dfea0ec) SHA1(8f41d267e488e07831946ef898d593897f10bfe2) )

	ROM_REGION( 0x020000, REGION_GFX4, ROMREGION_DISPOSE )	/* not used */

	ROM_REGION( 0x080000, REGION_GFX5, ROMREGION_DISPOSE )	/* BK3 tiles */
	ROM_LOAD( "back-2.6y",    0x000000, 0x080000, CRC(e07712af) SHA1(2a0285d6a1e0141838e898252b8d922a6263b05f) )

	ROM_REGION( 0x080000, REGION_GFX6, ROMREGION_DISPOSE )	/* LBK tiles */
	ROM_COPY( REGION_GFX5, 0x00000, 0x00000, 0x080000 )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "seibu8.7a",    0x000000, 0x040000, CRC(6f594808) SHA1(218aa12068aa587c7656355f6a6b86d97c868774) )

	ROM_REGION( 0x080000, REGION_USER1, 0 )
	ROM_LOAD( "copx-d1.bin",  0x000000, 0x080000, CRC(029bc402) SHA1(0f64e4c32d95abfa3920b39ed3cf0cc6eb50191b) )
ROM_END

ROM_START( olysoc92 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD32_BYTE( "u025.1",       0x000000, 0x040000, CRC(a94e7780) SHA1(abbe328be425b4529e6b75ffa723c6771e4b6fcf) )
	ROM_LOAD32_BYTE( "u024.2",       0x000001, 0x040000, CRC(cb5f0748) SHA1(e11bf11a3766ab33c60a143867496887c6238b11) )
	ROM_LOAD32_BYTE( "u026.3",       0x000002, 0x040000, CRC(f71cc626) SHA1(7f66031509063d5fac33a3b5873b616c7ad0c25b) )
	ROM_LOAD32_BYTE( "u023.4",       0x000003, 0x040000, CRC(2ba10e6c) SHA1(d682d97426a749cfdbaf728edb219dbf84e9eef8) )
	
	ROM_REGION( 0x20000*2, REGION_CPU2, 0 )	/* Z80 code, banked data */
	ROM_LOAD( "seibu7.8a",    0x000000, 0x08000, CRC(f63329f9) SHA1(51736de48efc14415cfdf169b43623d4c95fde2b) )
	ROM_CONTINUE(			  0x010000, 0x08000 )	/* banked stuff */

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "seibu6.7x",    0x000000, 0x010000, CRC(21c1e1b8) SHA1(30928c8ef98bf32ba0bf795ddadba1c95fcffe9d) )
	ROM_LOAD( "seibu5.7y",    0x010000, 0x010000, CRC(955d9fd7) SHA1(782451e8e85f7ba285d6cacd9d3fdcf48bde60bc) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "obj.8c",       0x000000, 0x100000, CRC(e2377895) SHA1(1d1c7f31a08a464139cdaf383a5e1ade0717dc9f) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )	/* MBK tiles */
	ROM_LOAD( "back-1.4y",    0x000000, 0x100000, CRC(3dfea0ec) SHA1(8f41d267e488e07831946ef898d593897f10bfe2) )

	ROM_REGION( 0x020000, REGION_GFX4, ROMREGION_DISPOSE )	/* not used */

	ROM_REGION( 0x080000, REGION_GFX5, ROMREGION_DISPOSE )	/* BK3 tiles */
	ROM_LOAD( "back-2.6y",    0x000000, 0x080000, CRC(e07712af) SHA1(2a0285d6a1e0141838e898252b8d922a6263b05f) )

	ROM_REGION( 0x080000, REGION_GFX6, ROMREGION_DISPOSE )	/* LBK tiles */
	ROM_COPY( REGION_GFX5, 0x00000, 0x00000, 0x080000 )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "seibu8.7a",    0x000000, 0x040000, CRC(6f594808) SHA1(218aa12068aa587c7656355f6a6b86d97c868774) )
ROM_END

static DRIVER_INIT( legionna )
{
	/* Unscramble gfx: quarters 1&2 swapped, quarters 3&4 swapped */

	data8_t *gfx = memory_region(REGION_GFX1);
	int len = memory_region_length(REGION_GFX1)/2;
	int a,i;

	for (i = 0; i < len/2; i++)
	{
		a = gfx[i];
		gfx[i] = gfx[i + len/2];
		gfx[i+len/2] = a;

		a = gfx[i+len];
		gfx[i+len] = gfx[i + len/2 + len];
		gfx[i + len/2 +len] = a;
	}
}



GAMEX( 1992, legionna, 0,        legionna, legionna, legionna, ROT0, "Tad", "Legionnaire (World)", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX( 1992, legionnu, legionna, legionna, legionna, legionna, ROT0, "Tad (Fabtek license)", "Legionnaire (US)", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )

GAMEX( 1992, heatbrl,  0,        heatbrl,  heatbrl,  0,        ROT0, "Tad", "Heated Barrel (World)", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX( 1992, heatbrlo, heatbrl,  heatbrl,  heatbrl,  0,        ROT0, "Tad", "Heated Barrel (World old version)", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX( 1992, heatbrlu, heatbrl,  heatbrl,  heatbrl,  0,        ROT0, "Tad", "Heated Barrel (US)", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )

GAMEX( 1993, godzilla, 0,        godzilla, godzilla, 0,        ROT0, "Banpresto", "Godzilla", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )

GAMEX( 1993, sdgndmrb, 0,		 sdgndmrb, sdgndmrb, 0,		   ROT0, "Banpresto", "SD Gundam Sangokushi Rainbow Tairiku Senki", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )

GAMEX( 1992, cupsoc,   0,		 cupsoc,  cupsoc,	 0,        ROT0, "Seibu", "Seibu Cup Soccer", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX( 1992, cupsoc2,  cupsoc,   cupsoc,  cupsoc,    0,        ROT0, "Seibu", "Seibu Cup Soccer (set 2)", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX( 1992, olysoc92, cupsoc,   cupsoc,  cupsoc,    0,        ROT0, "Seibu", "Olympic Soccer '92", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
