/* Super Slam (c)1993 Playmark */

/*

Still some unknown reads / writes (it writes all over the place ...)
Inputs / DSW's need finishing / verifying.

The 87C751 MCU for sound has not had its internal program ROM dumped.
So the sound MCU is simulated here - and therefore not 100% correct.

Update 12/03/2005 - Pierpaolo Prazzoli
- Fixed sprites
- Fixed text tilemap colors
- Fixed text tilemap scrolls
- Fixed VSync
- Fixed middle tilemap removing wraparound in the title screen (24/03/2005)

*/

/*

Here's the info about this dump:

Name:            Super Slam
Manufacturer:    PlayMark
Year:            1993
Date Dumped:     18-07-2002

CPU:             ST TS68000CP12, Signetics 87C751
SOUND:           OKIM6295
GFX:             TI TPC1020AFN-084C
CRYSTALS:        12MHz, 26MHz
DIPSW:           Two 8-Way
Country:         Italy

About the game:

This is a Tennis game :) You can play with some boys and girls, an old man,
a small kid and even with a dog! And remember, Winners don't use Drugs ;)

*/

#include "driver.h"


#define oki_time_base 0x08

static int sslam_sound;
static int sslam_melody;
static int sslam_melody_loop;
static int sslam_snd_bank;
data16_t *sslam_bg_tileram, *sslam_tx_tileram, *sslam_md_tileram;
data16_t *sslam_spriteram, *sslam_regs;



/**************************************************************************
   This table converts commands sent from the main CPU, into sample numbers
   played back by the sound processor.
   All commentry and most sound effects are correct, however the music
   tracks may be playing at the wrong times.
   Accordingly, the commands for playing the below samples is just a guess:
   1A, 1B, 1C, 1D, 1E, 60, 61, 62, 65, 66, 67, 68, 69, 6B, 6C.
   Samples 63, 64 and 6A are currently not fitted anywhere :-(
   Note: that samples 60, 61 and 62 combine to form a music track.
   Ditto for samples 65, 66, 67 and 68.
   Command 29 is fired when the game is completed successfully. It requires
   a melody from bank one to be playing, but we're already using the bank
   one melody during game play.
   The sound CPU simulation can only be perfected once it can be compared
   against a real game board.
*/

static const data8_t sslam_cmd_snd[128] =
{
/*00*/	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
/*08*/	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x70, 0x71,
/*10*/	0x72, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14,
/*18*/	0x15, 0x16, 0x17, 0x18, 0x19, 0x73, 0x74, 0x75,
/*20*/	0x76, 0x1a, 0x1b, 0x1c, 0x1d, 0x00, 0x1f, 0x6c,
/*28*/	0x1e, 0x65, 0x00, 0x00, 0x60, 0x20, 0x69, 0x65,
/*30*/	0x00, 0x00, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
/*38*/	0x29, 0x2a, 0x2b, 0x00, 0x6b, 0x00, 0x00, 0x00
};

/* Sound numbers in the sample ROM
01 "Out"
02 "Fault"
03 "Deuce"
04 "Let"
05 "Net"
06 "Let's Play"
07 "Play"
08 "Love"
09 "All"
0a "15"
0b "30"
0c "40"
0d "Change Court"
0e "Game"
0f "0"
10 "1"
11 "2"
12 "3"
13 "4"
14 "5"
15 "6"
16 "7"
17 "8"
18 "9"
19 "10"
1a Ball Hit
1b Ball Hit
1c Ball Bounce
1d Swing
1e Crowd Cheer
1f Crowd Clap
20 Horns
21 Ball Smash
22 Ball Hit
23 "Oww!"
24 "Ooh"
25 "Set Point"
26 "Match Point"
27 Boy "Ouch"
28 Girl "Ouch"
29 Synth
2a Three step rising synth
2b Ball Hit

60 Melody A1 -------------
61 Melody A2
62 Melody A3        Bank 0
63 Melody B
64 Melody C  -------------

65 Melody D1 -------------
66 Melody D2        Bank 1
67 Melody D3
68 Melody D4 -------------

69 Melody E --------------
6a Melody F         Bank 2
6b Melody G
6c Melody H --------------

70 "Tie Break"          -------------
71 "Advantage Server"
72 "Advantage Receiver"
73 "Game and First Set"        Bank 1
74 "Game and Second Set"
75 "Game and Third Set"
76 "Game Set and Match" -------------


Super Slam
Playmark, 1993

PCB Layout
----------

----------------------------------------------------
|      6295   3              62256             4   |
|          87C751            62256                 |
|      1MHz                                    5   |
|       6116                                       |
|       6116                                   6   |
|     DSW1(8)                                      |
|J                                             7   |
|A                                 TPC1020         |
|M    DSW2(8)                                  8   |
|M                                                 |
|A                                             9   |
|                                                  |
|                                              10  |
|                                                  |
|                                              11  |
|12MHz                          2018  2018         |
|       68000P12 62256                             |
|                 2             2018  2018         |
|                62256                             |
|                 1     26MHz                      |
----------------------------------------------------

Notes:
      68k clock: 12MHz
          VSync: 58Hz
          HSync: 14.62kHz
   87c751 clock: 12MHz (87c751 is 8051 based DIP24 MCU with)
                       (2Kx8 OTP EPROM and 64Kx8 SRAM)
                       (unfortunately, it's protected)


*/


/* vidhrdw/playmark.c */
WRITE16_HANDLER( bigtwin_paletteram_w );

/* vidhrdw/sslam.c */
WRITE16_HANDLER( sslam_tx_tileram_w );
WRITE16_HANDLER( sslam_md_tileram_w );
WRITE16_HANDLER( sslam_bg_tileram_w );
VIDEO_START(sslam);
VIDEO_UPDATE(sslam);


static void sslam_play(int melody, int data)
{
	int status = OKIM6295_status_0_r(0);

	log_cb(RETRO_LOG_DEBUG, LOGPRE "Playing sample %01x:%02x from command %02x\n",sslam_snd_bank,sslam_sound,data);
	if (sslam_sound == 0) usrintf_showmessage("Unknown sound command %02x",sslam_sound);

	if (melody) {
		if (sslam_melody != sslam_sound) {
			sslam_melody      = sslam_sound;
			sslam_melody_loop = sslam_sound;
			if (status & 0x08)
				OKIM6295_data_0_w(0,0x40);
			OKIM6295_data_0_w(0,(0x80 | sslam_melody));
			OKIM6295_data_0_w(0,0x81);
		}
	}
	else {
		if ((status & 0x01) == 0) {
		OKIM6295_data_0_w(0,(0x80 | sslam_sound));
			OKIM6295_data_0_w(0,0x11);
		}
		else if ((status & 0x02) == 0) {
		OKIM6295_data_0_w(0,(0x80 | sslam_sound));
			OKIM6295_data_0_w(0,0x21);
		}
		else if ((status & 0x04) == 0) {
		OKIM6295_data_0_w(0,(0x80 | sslam_sound));
			OKIM6295_data_0_w(0,0x41);
		}
	}
}

WRITE16_HANDLER( sslam_snd_w )
{
	if (ACCESSING_LSB)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "PC:%06x Writing %04x to Sound CPU\n",activecpu_get_previouspc(),data);
		if (data >= 0x40) {
			if (data == 0xfe) {
				OKIM6295_data_0_w(0,0x40);	/* Stop playing the melody */
				sslam_melody      = 0x00;
				sslam_melody_loop = 0x00;
			}
			else {
				log_cb(RETRO_LOG_DEBUG, LOGPRE "Unknown command (%02x) sent to the Sound controller\n",data);
			}
		}
		else if (data == 0) {
			OKIM6295_data_0_w(0,0x38);		/* Stop playing effects */
		}
		else {
			sslam_sound = sslam_cmd_snd[data];

			if (sslam_sound >= 0x70) {
				if (sslam_snd_bank != 1)
					OKIM6295_set_bank_base(0, (1 * 0x40000));
				sslam_snd_bank = 1;
				sslam_play(0, data);
			}
			else if (sslam_sound >= 0x69) {
				if (sslam_snd_bank != 2)
					OKIM6295_set_bank_base(0, (2 * 0x40000));
				sslam_snd_bank = 2;
				sslam_play(4, data);
			}
			else if (sslam_sound >= 0x65) {
				if (sslam_snd_bank != 1)
					OKIM6295_set_bank_base(0, (1 * 0x40000));
				sslam_snd_bank = 1;
				sslam_play(4, data);
			}
			else if (sslam_sound >= 0x60) {
				sslam_snd_bank = 0;
					OKIM6295_set_bank_base(0, (0 * 0x40000));
				sslam_snd_bank = 0;
				sslam_play(4, data);
			}
			else {
				sslam_play(0, data);
			}
		}
	}
}


static INTERRUPT_GEN( sslam_interrupt )
{
	if ((OKIM6295_status_0_r(0) & 0x08) == 0)
	{
		switch(sslam_melody_loop)
		{
			case 0x060:	sslam_melody_loop = 0x061; break;
			case 0x061:	sslam_melody_loop = 0x062; break;
			case 0x062:	sslam_melody_loop = 0x060; break;

			case 0x065:	sslam_melody_loop = 0x165; break;
			case 0x165:	sslam_melody_loop = 0x265; break;
			case 0x265:	sslam_melody_loop = 0x365; break;
			case 0x365:	sslam_melody_loop = 0x066; break;
			case 0x066:	sslam_melody_loop = 0x067; break;
			case 0x067:	sslam_melody_loop = 0x068; break;
			case 0x068:	sslam_melody_loop = 0x065; break;

			case 0x063:	sslam_melody_loop = 0x063; break;
			case 0x064:	sslam_melody_loop = 0x064; break;
			case 0x069:	sslam_melody_loop = 0x069; break;
			case 0x06a:	sslam_melody_loop = 0x06a; break;
			case 0x06b:	sslam_melody_loop = 0x06b; break;
			case 0x06c:	sslam_melody_loop = 0x06c; break;

			default:	sslam_melody_loop = 0x00; break;
		}

		if (sslam_melody_loop)
		{
/*			log_cb(RETRO_LOG_DEBUG, LOGPRE "Changing to sample %02x\n",sslam_melody_loop); */
			OKIM6295_data_0_w(0,((0x80 | sslam_melody_loop) & 0xff));
			OKIM6295_data_0_w(0,0x81);
		}
	}
}



/* Memory Maps */

/* these will need verifying .. the game writes all over the place ... */

static MEMORY_READ16_START( sslam_readmem )
    { 0x000400, 0x07ffff, MRA16_RAM },
	{ 0x100000, 0x103fff, MRA16_RAM },
	{ 0x104000, 0x107fff, MRA16_RAM },
	{ 0x108000, 0x10ffff, MRA16_RAM },
	{ 0x110000, 0x11000b, MRA16_RAM },
	{ 0x280000, 0x280fff, MRA16_RAM },
	{ 0x201000, 0x220fff, MRA16_RAM },
	{ 0x300010, 0x300011, input_port_0_word_r },
	{ 0x300012, 0x300013, input_port_1_word_r },
	{ 0x300014, 0x300015, input_port_2_word_r },
	{ 0x300016, 0x300017, input_port_3_word_r },
	{ 0x300018, 0x300019, input_port_4_word_r },
	{ 0x30001a, 0x30001b, input_port_5_word_r },
	{ 0x30001c, 0x30001d, input_port_6_word_r },
	{ 0xf00000, 0xffffff, MRA16_RAM	}, /* Main RAM */
	{ 0x000000, 0xffffff, MRA16_ROM }, /* I don't honestly know where the rom is mirrored .. so all unmapped reads / writes go to rom */
MEMORY_END

static MEMORY_WRITE16_START( sslam_writemem )
    { 0x000400, 0x07ffff, MWA16_RAM },
	{ 0x100000, 0x103fff, sslam_bg_tileram_w, &sslam_bg_tileram },
	{ 0x104000, 0x107fff, sslam_md_tileram_w, &sslam_md_tileram },
	{ 0x108000, 0x10ffff, sslam_tx_tileram_w, &sslam_tx_tileram },
	{ 0x110000, 0x11000b, MWA16_RAM, &sslam_regs },
	{ 0x11000c, 0x11000d, MWA16_NOP },
	{ 0x200000, 0x200001, MWA16_NOP },
	{ 0x280000, 0x280fff, bigtwin_paletteram_w, &paletteram16 },
	{ 0x201000, 0x220fff, MWA16_RAM, &sslam_spriteram}, /* probably not all of it .. */
	{ 0x304000, 0x304001, MWA16_NOP },
	{ 0x30001e, 0x30001f, sslam_snd_w },
	{ 0xf00000, 0xffffff, MWA16_RAM	}, /* Main RAM */
	{ 0x000000, 0xffffff, MWA16_ROM }, /* I don't honestly know where the rom is mirrored .. so all unmapped reads / writes go to rom */
MEMORY_END

/* Input Ports */

INPUT_PORTS_START( sslam )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN4 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START3 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER4 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START4 )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Unknown ) )		/* 0x000522 = 0x00400e */
	PORT_DIPSETTING(    0x03, "0" )
	PORT_DIPSETTING(    0x02, "1" )
	PORT_DIPSETTING(    0x01, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPNAME( 0x04, 0x04, "Singles Game Time" )
	PORT_DIPSETTING(    0x04, "180 Seconds" )
	PORT_DIPSETTING(    0x00, "120 Seconds" )
	PORT_DIPNAME( 0x08, 0x08, "Doubles Game Time" )
	PORT_DIPSETTING(    0x08, "180 Seconds" )
	PORT_DIPSETTING(    0x00, "120 Seconds" )
	PORT_DIPNAME( 0x30, 0x30, "Starting Score" )
	PORT_DIPSETTING(    0x30, "4-4" )
	PORT_DIPSETTING(    0x20, "3-4" )
	PORT_DIPSETTING(    0x10, "3-3" )
	PORT_DIPSETTING(    0x00, "0-0" )
	PORT_DIPNAME( 0x40, 0x40, "Max Players" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x40, "4" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x07, 0x07, "Coin(s) per Player" )
	PORT_DIPSETTING(    0x07, "1" )
	PORT_DIPSETTING(    0x06, "2" )
	PORT_DIPSETTING(    0x05, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPSETTING(    0x02, "6" )
	PORT_DIPSETTING(    0x01, "7" )
	PORT_DIPSETTING(    0x00, "8" )
	PORT_DIPNAME( 0x38, 0x38, "Coin Multiplicator" )
	PORT_DIPSETTING(    0x38, "*1" )
	PORT_DIPSETTING(    0x30, "*2" )
	PORT_DIPSETTING(    0x28, "*3" )
	PORT_DIPSETTING(    0x20, "*4" )
	PORT_DIPSETTING(    0x18, "*5" )
	PORT_DIPSETTING(    0x10, "*6" )
	PORT_DIPSETTING(    0x08, "*7" )
	PORT_DIPSETTING(    0x00, "*8" )
	PORT_DIPNAME( 0x40, 0x00, "On Time Up" )
	PORT_DIPSETTING(    0x00, "End After Point" )
	PORT_DIPSETTING(    0x40, "End After Game" )
	PORT_DIPNAME( 0x80, 0x80, "Coin Slots" )
	PORT_DIPSETTING(    0x80, "Common" )
	PORT_DIPSETTING(    0x00, "Individual" )
INPUT_PORTS_END

/* GFX Decodes */

static struct GfxLayout tiles8x8_layout =
{
	8,8,
	RGN_FRAC(1,4),
	4,
	{ 0, RGN_FRAC(1,4), RGN_FRAC(2,4), RGN_FRAC(3,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static struct GfxLayout tiles16x16_layout =
{
	16,16,
	RGN_FRAC(1,4),
	4,
	{ 0, RGN_FRAC(1,4), RGN_FRAC(2,4), RGN_FRAC(3,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
		16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
		8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	16*16
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tiles16x16_layout, 0    , 16 }, /* bg */
	{ REGION_GFX1, 0, &tiles16x16_layout, 0x100, 16 }, /* mid */
	{ REGION_GFX1, 0, &tiles8x8_layout,   0x200, 16 }, /* tx */
	{ REGION_GFX2, 0, &tiles8x8_layout,   0x300, 16 }, /* spr */
	{ -1 }
};

static struct OKIM6295interface okim6295_interface =
{
	1,							/* 1 chip */
	{ (26000000/16)/165 },		/* frequency 9849Hz ??? */
	{ REGION_SOUND1 },			/* memory region */
	{ 80 }
};

/* Machine Driver */

static MACHINE_DRIVER_START( sslam )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)	/* 12 MHz */
	MDRV_CPU_MEMORY(sslam_readmem, sslam_writemem)
	MDRV_CPU_VBLANK_INT(irq2_line_hold,1)
	MDRV_CPU_PERIODIC_INT(sslam_interrupt, 240)

	MDRV_FRAMES_PER_SECOND(58)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(1*8, 39*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x800)

	MDRV_VIDEO_START(sslam)
	MDRV_VIDEO_UPDATE(sslam)
	
	/* sound hardware */
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END


/* maybe one dump is bad .. which? -> 2nd set was verified good from 2 pcbs */

ROM_START( sslam )
	ROM_REGION( 0x1000000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "2.u67", 0x00000, 0x80000, CRC(1ce52917) SHA1(b9b1d14ea44c248ce6e615c5c553c0d485c1302b) )
	ROM_RELOAD ( 0x100000, 0x80000 )
	ROM_RELOAD ( 0x200000, 0x80000 )
	ROM_RELOAD ( 0x300000, 0x80000 )
	ROM_RELOAD ( 0x400000, 0x80000 )
	ROM_RELOAD ( 0x500000, 0x80000 )
	ROM_RELOAD ( 0x600000, 0x80000 )
	ROM_RELOAD ( 0x700000, 0x80000 )
	ROM_RELOAD ( 0x800000, 0x80000 )
	ROM_RELOAD ( 0x900000, 0x80000 )
	ROM_RELOAD ( 0xa00000, 0x80000 )
	ROM_RELOAD ( 0xb00000, 0x80000 )
	ROM_RELOAD ( 0xc00000, 0x80000 )
	ROM_RELOAD ( 0xd00000, 0x80000 )
	ROM_RELOAD ( 0xe00000, 0x80000 )
	ROM_RELOAD ( 0xf00000, 0x80000 )
	ROM_LOAD16_BYTE( "it_22.bin", 0x00001, 0x80000, CRC(51c56828) SHA1(d71d64b0268c156456bed64b4c13b98181fa3e0f) )
	ROM_RELOAD ( 0x100001, 0x80000 )
	ROM_RELOAD ( 0x200001, 0x80000 )
	ROM_RELOAD ( 0x300001, 0x80000 )
	ROM_RELOAD ( 0x400001, 0x80000 )
	ROM_RELOAD ( 0x500001, 0x80000 )
	ROM_RELOAD ( 0x600001, 0x80000 )
	ROM_RELOAD ( 0x700001, 0x80000 )
	ROM_RELOAD ( 0x800001, 0x80000 )
	ROM_RELOAD ( 0x900001, 0x80000 )
	ROM_RELOAD ( 0xa00001, 0x80000 )
	ROM_RELOAD ( 0xb00001, 0x80000 )
	ROM_RELOAD ( 0xc00001, 0x80000 )
	ROM_RELOAD ( 0xd00001, 0x80000 )
	ROM_RELOAD ( 0xe00001, 0x80000 )
	ROM_RELOAD ( 0xf00001, 0x80000 )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE  ) /* Bg */
	ROM_LOAD( "7.u45",     0x000000, 0x80000, CRC(64ecdde9) SHA1(576ba1169d90970622249e532baa4209bf12de5a) )
	ROM_LOAD( "6.u39",     0x080000, 0x80000, CRC(6928065c) SHA1(ad5b1889bebf0358df0295d6041b798ac53ac625) )
	ROM_LOAD( "5.u42",     0x100000, 0x80000, CRC(8d18bdc6) SHA1(cacc4f475f85438a00ead4911730202e995983a7) )
	ROM_LOAD( "4.u36",     0x180000, 0x80000, CRC(8e15fb9d) SHA1(47917d8aac1bce2e15f36904f5c2534e5b80236b) )
	
	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE  ) /* Sprites */
	ROM_LOAD( "8.u83",     0x000000, 0x80000, CRC(19bb89dd) SHA1(c2a0c32d350a193d366b5086502998281fd0bec4) )
	ROM_LOAD( "9.u84",     0x080000, 0x80000, CRC(d50d86c7) SHA1(7ecbcc03851a8174610f7f5ad889e40543da928e) )
	ROM_LOAD( "10.u85",    0x100000, 0x80000, CRC(681b8ac8) SHA1(ebfeffc091f53af246311574b9c5d83d2716a7be) )
	ROM_LOAD( "11.u86",    0x180000, 0x80000, CRC(e41f89e3) SHA1(e4b39411a4cea6aa6c01564f74bb8e432d382a73) )

	/* $00000-$20000 stays the same in all sound banks, */
	/* the second half of the bank is the area that gets switched */
	ROM_REGION( 0xc0000, REGION_SOUND1, 0 ) /* OKI Samples */
	ROM_LOAD( "3.u13",       0x00000, 0x40000, CRC(d0a9245f) SHA1(2e840cdd7bdfe7c6f986daf88576de0559597499) )
	ROM_CONTINUE(            0x60000, 0x20000 )
	ROM_CONTINUE(            0xa0000, 0x20000 )
	ROM_COPY( REGION_SOUND1, 0x00000, 0x40000, 0x20000)
	ROM_COPY( REGION_SOUND1, 0x00000, 0x80000, 0x20000)
ROM_END

ROM_START( sslama )
	ROM_REGION( 0x1000000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "2.u67", 0x00000, 0x80000, CRC(1ce52917) SHA1(b9b1d14ea44c248ce6e615c5c553c0d485c1302b) )
	ROM_RELOAD ( 0x100000, 0x80000 )
	ROM_RELOAD ( 0x200000, 0x80000 )
	ROM_RELOAD ( 0x300000, 0x80000 )
	ROM_RELOAD ( 0x400000, 0x80000 )
	ROM_RELOAD ( 0x500000, 0x80000 )
	ROM_RELOAD ( 0x600000, 0x80000 )
	ROM_RELOAD ( 0x700000, 0x80000 )
	ROM_RELOAD ( 0x800000, 0x80000 )
	ROM_RELOAD ( 0x900000, 0x80000 )
	ROM_RELOAD ( 0xa00000, 0x80000 )
	ROM_RELOAD ( 0xb00000, 0x80000 )
	ROM_RELOAD ( 0xc00000, 0x80000 )
	ROM_RELOAD ( 0xd00000, 0x80000 )
	ROM_RELOAD ( 0xe00000, 0x80000 )
	ROM_RELOAD ( 0xf00000, 0x80000 )
	ROM_LOAD16_BYTE( "1.u56", 0x00001, 0x80000,  CRC(59bec8ae) SHA1(2d53213a1d335184384b2138d18d496b602dc3fb) )
	ROM_RELOAD ( 0x100001, 0x80000 )
	ROM_RELOAD ( 0x200001, 0x80000 )
	ROM_RELOAD ( 0x300001, 0x80000 )
	ROM_RELOAD ( 0x400001, 0x80000 )
	ROM_RELOAD ( 0x500001, 0x80000 )
	ROM_RELOAD ( 0x600001, 0x80000 )
	ROM_RELOAD ( 0x700001, 0x80000 )
	ROM_RELOAD ( 0x800001, 0x80000 )
	ROM_RELOAD ( 0x900001, 0x80000 )
	ROM_RELOAD ( 0xa00001, 0x80000 )
	ROM_RELOAD ( 0xb00001, 0x80000 )
	ROM_RELOAD ( 0xc00001, 0x80000 )
	ROM_RELOAD ( 0xd00001, 0x80000 )
	ROM_RELOAD ( 0xe00001, 0x80000 )
	ROM_RELOAD ( 0xf00001, 0x80000 )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE  ) /* Bg */
	ROM_LOAD( "7.u45",     0x000000, 0x80000, CRC(64ecdde9) SHA1(576ba1169d90970622249e532baa4209bf12de5a) )
	ROM_LOAD( "6.u39",     0x080000, 0x80000, CRC(6928065c) SHA1(ad5b1889bebf0358df0295d6041b798ac53ac625) )
	ROM_LOAD( "5.u42",     0x100000, 0x80000, CRC(8d18bdc6) SHA1(cacc4f475f85438a00ead4911730202e995983a7) )
	ROM_LOAD( "4.u36",     0x180000, 0x80000, CRC(8e15fb9d) SHA1(47917d8aac1bce2e15f36904f5c2534e5b80236b) )
	
	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE  ) /* Sprites */
	ROM_LOAD( "8.u83",     0x000000, 0x80000, CRC(19bb89dd) SHA1(c2a0c32d350a193d366b5086502998281fd0bec4) )
	ROM_LOAD( "9.u84",     0x080000, 0x80000, CRC(d50d86c7) SHA1(7ecbcc03851a8174610f7f5ad889e40543da928e) )
	ROM_LOAD( "10.u85",    0x100000, 0x80000, CRC(681b8ac8) SHA1(ebfeffc091f53af246311574b9c5d83d2716a7be) )
	ROM_LOAD( "11.u86",    0x180000, 0x80000, CRC(e41f89e3) SHA1(e4b39411a4cea6aa6c01564f74bb8e432d382a73) )

	/* $00000-$20000 stays the same in all sound banks, */
	/* the second half of the bank is the area that gets switched */
	ROM_REGION( 0xc0000, REGION_SOUND1, 0 ) /* OKI Samples */
	ROM_LOAD( "3.u13",       0x00000, 0x40000, CRC(d0a9245f) SHA1(2e840cdd7bdfe7c6f986daf88576de0559597499) )
	ROM_CONTINUE(            0x60000, 0x20000 )
	ROM_CONTINUE(            0xa0000, 0x20000 )
	ROM_COPY( REGION_SOUND1, 0x00000, 0x40000, 0x20000)
	ROM_COPY( REGION_SOUND1, 0x00000, 0x80000, 0x20000)
ROM_END

GAMEX(1993, sslam,  0,     sslam, sslam, 0, ROT0, "Playmark", "Super Slam (set 1)", GAME_IMPERFECT_SOUND )
GAMEX(1993, sslama, sslam, sslam, sslam, 0, ROT0, "Playmark", "Super Slam (set 2)", GAME_IMPERFECT_SOUND )
