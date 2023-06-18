/***************************************************************************

There is strong evidence that the following Namco custom chips are all
instances of the same 4-bit MCU, the Fujitsu MB8851 (42-pin DIP package)
and MB8852 (28-pin DIP), differently programmed.

chip pins function
---- ---- --------
50XX  28  player score handling (protection)
51XX  42  I/O (coin management built-in)
52XX  42  sample playback
53XX  42  I/O (steering wheel support)
54XX  28  explosion (noise) generator
56XX  42  I/O (coin management built-in)
58XX  42  I/O (coin management built-in)
62XX  28  I/O and explosion (noise) generator

06XX interface:
---------------
Galaga                  51XX  ----  ----  54XX
Bosconian (CPU board)   51XX  ----  50XX  54XX
Bosconian (Video board) 50XX  52XX  ----  ----
Xevious                 51XX  ----  50XX  54XX
Dig Dug                 51XX  53XX  ----  ----
Pole Position / PP II   51XX  53XX  52XX  54XX

16XX interface:
---------------
Super Pac Man           56XX  56XX  ----  ----
Pac & Pal               56XX  ????  ----  ----
Mappy                   58XX  58XX  ----  ----
Phozon                  58XX  56XX  ----  ----
The Tower of Druaga     58XX  56XX  ----  ----
Grobda                  58XX  56XX  ----  ----
Dig Dug II              58XX  56XX  ----  ----
Motos                   56XX  56XX  ----  ----
Gaplus                  56XX  58XX  62XX  ----
Gaplus (alt.)           58XX  56XX  62XX  ----
Libble Rabble           58XX  56XX? 56XX? ----
Toy Pop                 58XX  56XX  56XX  ----


Pinouts:

        MB8851                      MB8852
       +------+                    +------+
  EXTAL|1   42|Vcc            EXTAL|1   28|Vcc
   XTAL|2   41|K3        XTAL /STBY|2   27|K3
 /RESET|3   40|K2            /RESET|3   26|K2
   /IRQ|4   39|K1                O0|4   25|K1
     SO|5   38|K0                O1|5   24|K0
     SI|6   37|R15 /STBY         O2|6   23|R10 /IRQ
/SC /TO|7   36|R14               O3|7   22|R9 /TC
    /TC|8   35|R13               O4|8   21|R8
     P0|9   34|R12               O5|9   20|R7
     P1|10  33|R11               O6|10  19|R6
     P2|11  32|R10               O7|11  18|R5
     P3|12  31|R9                R0|12  17|R4
     O0|13  30|R8                R1|13  16|R3
     O1|14  29|R7               GND|14  15|R2
     O2|15  28|R6                  +------+
     O3|16  27|R5
     O4|17  26|R4
     O5|18  25|R3
     O6|19  24|R2
     O7|20  23|R1
    GND|21  22|R0
       +------+


      O  O  R  R  R  K
50XX  O  O  I     I  I
54XX  O  O  I  O     I
62XX  O  O  IO O     I

      P  O  O  R  R  R  R  K
51XX  O  O  O  I  I  I  I  I
52XX  O  O  O  I  I  O  O  I
53XX  O? O  O  I  I  I  I  I
56XX  O  O  O  I  I  I  IO I
58XX  O  O  O  I  I  I  IO I


For the 52XX, see sound/namco52.c

For the 54XX, see sound/namco54.c


Namco custom I/O chips 56XX and 58XX
(plus an unknown one used only by Pac & Pal - could be "57XX", I guess).
(56XX mode 7 is used only by liblrabl, it could be a different chip as well)

These chips work together with a 16XX, that interfaces them with the buffer
RAM. Each chip uses 16 nibbles of memory; the 16XX supports up to 4 chips,
but most games use only 2.

The 56XX and 58XX are pin-to-pin compatible, but not functionally equivalent:
they provide the same functions, but the command codes and memory addresses
are different, so they cannot be exchanged.

The devices have 42 pins. There are 16 input lines and 8 output lines to be
used for I/O.

It is very likely that the chips are standard 4-bit microcontrollers with
embedded ROM, but it hasn't been determined which type. The closest match
so far is the Oki MSM6408/MSM6422, but the pinout doesn't match 100%.
Since the Namco customs appear to be manufactured by Fujitsu, they might be
MB8851, but I haven't found data sheets nor pinouts for those devices.


pin   description
---   -----------
1     clock (Mappy, Super Pac-Man)
2     clock (Gaplus; equivalent to the above?)
3     reset
4     irq
5-6   (to/from 16XX) (this is probably a normal I/O port used to synchronize with the 16XX)
7-8   ?
9-12  address to r/w from RAM; 12 also goes to the 16XX and acts as r/w line, so
      the chip can only read from addresses 0-7 and only write to addresses 8-F
	  (this is probably a normal I/O port used for that purpose)
13-16 out port A
17-20 out port B
21    GND
22-25 in port B
26-29 in port C
30-33 in port D
34-37 (to 16XX) probably data to r/w from RAM
	  (this is probably a normal I/O port used for that purpose)
38-41 in port A
42    Vcc

TODO:
- It's likely that the 56XX and 58XX chips, when in "coin mode", also internally
  handle outputs for start lamps, coin counters and coin lockout, like the 51XX.
  Such lines are NOT present in the Mappy and Super Pacman schematics, so they
  were probably not used for those games, but they might have been used in
  others (most likely Gaplus).

***************************************************************************/

#include "driver.h"
#include "machine/namcoio.h"

#define VERBOSE 0


struct namcoio
{
	int type;
	read8_handler in[4];
	write8_handler out[2];
	int reset;
	int lastcoins,lastbuttons;
	int credits;
	int coins[2];
	int coins_per_cred[2];
	int creds_per_coin[2];
	int in_count;
	int mode,coincred_mode,remap_joy;	/* for 51XX */
};

static struct namcoio io[MAX_NAMCOIO];
static int nmi_cpu[MAX_06XX];
static void *nmi_timer[MAX_06XX];

static READ_HANDLER( nop_r ) { return 0x0f; }
static WRITE_HANDLER( nop_w ) { }

#define READ_PORT(n)	(io[chip].in[n](0) & 0x0f)
#define WRITE_PORT(n,d)	io[chip].out[n](0,(d) & 0x0f)



/*
50XX
Bosconian scoring info

I/O controller command 64h update Score[Player[chip]][chip] values: (set by game code)

60h = switch to player 1
68h = switch to player 2

80h =    5  (Xevious startup check)
81h =   10  Asteroid
82h =   15? (Battles Xevious bootleg)
83h =   20  Cosmo-Mine
84h =   25? (Battles Xevious bootleg)
85h =   30? (Battles Xevious bootleg)
86h =   40? (Battles Xevious bootleg)
87h =   50  I-Type
88h =   60  P-Type
89h =   70  E-Type
8ah =   80? (Battles Xevious bootleg)
8bh =   90? (Battles Xevious bootleg)
8ch =  100? (Battles Xevious bootleg)
8Dh =  200  Spy Ship
8eh =  300? (Battles Xevious bootleg)
8fh =  500? (Battles Xevious bootleg)

90h =   50? (guess)
91h =  100          2nd CPU protection check
92h =  150? (guess)
93h =  200  Bonus & 2nd CPU protection check
94h =  250? (guess)
95h =  300  Bonus & 2nd CPU protection check
96h =  400  Bonus & 2nd CPU protection check
97h =  500          2nd CPU protection check
98h =  600  Bonus & 2nd CPU protection check
99h =  700          2nd CPU protection check
9Ah =  800  Bonus & 2nd CPU protection check
9bh =  900          2nd CPU protection check
9ch = 1000? (guess)
9dh = 2000? (guess)
9eh = 3000? (guess)
9fh = 5000? (guess)

A0h =  500  I-Type Formation
A1h = 1000  P-Type Formation
A2h = 1500  E-Type Formation
A3h = 2000  Bonus
A4h = 2500? (guess)
A5h = 3000  Bonus
A6h = 4000  Bonus
A7h = 5000  Bonus
A8h = 6000  Bonus
A9h = 7000  Bonus
Aah = 8000? (guess)
Abh = 9000? (guess)
Ach =10000? (guess)
Adh =20000? (guess)
Aeh =30000? (guess)
Afh =50000? (guess)

b0h =   10? (guess)
b1h =   20? (guess)
b2h =   30? (guess)
b3h =   40? (guess)
b4h =   50? (guess)
b5h =   60? (guess)
b6h =   80? (guess)
B7h =  100  I-Type Leader
B8h =  120  P-Type Leader
B9h =  140  E-Type Leader
bah =  160? (guess)
bbh =  180? (guess)
bch =  200? (guess)
bdh =  400? (guess)
beh =  600? (guess)
bfh = 1000? (guess)

E0h =   15? (Battles Xevious bootleg)
E1h =   30? (Battles Xevious bootleg)
E2h =   45? (Battles Xevious bootleg)
E3h =   60? (Battles Xevious bootleg)
E4h =   75? (Battles Xevious bootleg)
E5h =   90  (Xevious startup check)
E6h =  120? (Battles Xevious bootleg)
E7h =  150? (Battles Xevious bootleg)
E8h =  180? (Battles Xevious bootleg)
E9h =  210? (Battles Xevious bootleg)
Eah =  240? (Battles Xevious bootleg)
Ebh =  270? (Battles Xevious bootleg)
Ech =  300? (Battles Xevious bootleg)
Edh =  600? (Battles Xevious bootleg)
Eeh =  900? (Battles Xevious bootleg)
Efh = 1000? (Battles Xevious bootleg)

Bonuses are given at the end of a round if the game is set to auto
difficulty and the round is completed on one life. Bonus values are:

 100x3  95h
 100x4  96h
 100x8  9Ah
 200x4  96h,96h
 200x8  9Ah,9Ah
 300x8  A3h,96h
 400x8  A5h,93h
 500x8  A3h,A3h
 600x8  A6h,9Ah
 700x8  A7h,98h
 800x8  A8h,96h
 900x8  A9h,93h
1000x8  A6h,A6h


I/O controller command 84h set bonus values: (set by game code)

Byte 0: always 10h
Byte 1: indicator (20h=first bonus, 30h=interval bonus, others=unknown)
Byte 2: BCD Score[Player[chip]][chip] (--ss----)
Byte 3: BCD Score[Player[chip]][chip] (----ss--)
Byte 4: BCD Score[Player[chip]][chip] (------ss)

Indicator values 20h and 30h are sent once during startup based upon
the dip switch settings, other values are sent during gameplay.
The default bonus setting is 20000, 70000, and every 70000.


I/O controller command 94h read Score[Player[chip]][chip] returned value: (read by game code)

Byte 0: BCD Score[Player[chip]][chip] (fs------) and flags
Byte 1: BCD Score[Player[chip]][chip] (--ss----)
Byte 2: BCD Score[Player[chip]][chip] (----ss--)
Byte 3: BCD Score[Player[chip]][chip] (------ss)

Flags: 80h=high score, 40h=first bonus, 20h=interval bonus
*/

#define MAX_50XX 2

static int		HiScore[MAX_50XX];
int		Score[2][MAX_50XX];
int		NextBonus[2][MAX_50XX];
int		FirstBonus[MAX_50XX],IntervalBonus[MAX_50XX];
int Player[MAX_50XX];

static int in_count_50XX[MAX_50XX];

void namcoio_50XX_write(int chipnum,int data)
{
	int chip = (chipnum < 4) ? 0 : 1;
	static int fetch[MAX_50XX];
	static int fetchmode[MAX_50XX];

#if VERBOSE
	logerror("%04x: custom 50XX #%d write %02x\n",activecpu_get_pc(),chip,data);
#endif

	if (fetch[chip])
	{
		switch (fetch[chip]--)
		{
			case 3:
				if (fetchmode[chip] == 0x20)
					FirstBonus[chip] = ((data / 16) * 100000) + ((data % 16) * 10000);
				else if (fetchmode[chip] == 0x30)
					IntervalBonus[chip] = ((data / 16) * 100000) + ((data % 16) * 10000);
				else if (fetchmode[chip] == 0x50)
					HiScore[chip] = ((data / 16) * 100000) + ((data % 16) * 10000);

				NextBonus[0][chip] = FirstBonus[chip];
				NextBonus[1][chip] = FirstBonus[chip];
				break;
			case 2:
				if (fetchmode[chip] == 0x20)
					FirstBonus[chip] = FirstBonus[chip] + ((data / 16) * 1000) + ((data % 16) * 100);
				else if (fetchmode[chip] == 0x30)
					IntervalBonus[chip] = IntervalBonus[chip] + ((data / 16) * 1000) + ((data % 16) * 100);
				else if (fetchmode[chip] == 0x50)
					HiScore[chip] = HiScore[chip] + ((data / 16) * 1000) + ((data % 16) * 100);

				NextBonus[0][chip] = FirstBonus[chip];
				NextBonus[1][chip] = FirstBonus[chip];
				break;
			case 1:
				if (fetchmode[chip] == 0x20)
					FirstBonus[chip] = FirstBonus[chip] + ((data / 16) * 10) + ((data % 16) * 1);
				else if (fetchmode[chip] == 0x30)
					IntervalBonus[chip] = IntervalBonus[chip] + ((data / 16) * 10) + ((data % 16) * 1);
				else if (fetchmode[chip] == 0x50)
					HiScore[chip] = HiScore[chip] + ((data / 16) * 10) + ((data % 16) * 1);

				NextBonus[0][chip] = FirstBonus[chip];
				NextBonus[1][chip] = FirstBonus[chip];
				break;
		}
	}
	else
	{
		switch(data)
		{
			case 0x10:
				Score[0][chip] = 0;
				Score[1][chip] = 0;
				in_count_50XX[chip] = 0;
				break;

			case 0x20:
			case 0x30:
			case 0x50:
				fetch[chip] = 3;
				fetchmode[chip] = data;
				break;

			case 0x60:	/* 1P Score */
				Player[chip] = 0;
				break;
			case 0x68:	/* 2P Score */
				Player[chip] = 1;
				break;
/*			case 0x70: */
/*				break; */
			case 0x80:
				Score[Player[chip]][chip] += 5;
				break;
			case 0x81:
				Score[Player[chip]][chip] += 10;
				break;
			case 0x83:
				Score[Player[chip]][chip] += 20;
				break;
			case 0x87:
				Score[Player[chip]][chip] += 50;
				break;
			case 0x88:
				Score[Player[chip]][chip] += 60;
				break;
			case 0x89:
				Score[Player[chip]][chip] += 70;
				break;
			case 0x8D:
				Score[Player[chip]][chip] += 200;
				break;
			case 0x91:
				Score[Player[chip]][chip] += 100;
				break;
			case 0x93:
				Score[Player[chip]][chip] += 200;
				break;
			case 0x95:
				Score[Player[chip]][chip] += 300;
				break;
			case 0x96:
				Score[Player[chip]][chip] += 400;
				break;
			case 0x97:
				Score[Player[chip]][chip] += 500;
				break;
			case 0x98:
				Score[Player[chip]][chip] += 600;
				break;
			case 0x99:
				Score[Player[chip]][chip] += 700;
				break;
			case 0x9A:
				Score[Player[chip]][chip] += 800;
				break;
			case 0x9b:
				Score[Player[chip]][chip] += 900;
				break;
			case 0xA0:
				Score[Player[chip]][chip] += 500;
				break;
			case 0xA1:
				Score[Player[chip]][chip] += 1000;
				break;
			case 0xA2:
				Score[Player[chip]][chip] += 1500;
				break;
			case 0xA3:
				Score[Player[chip]][chip] += 2000;
				break;
			case 0xA5:
				Score[Player[chip]][chip] += 3000;
				break;
			case 0xA6:
				Score[Player[chip]][chip] += 4000;
				break;
			case 0xA7:
				Score[Player[chip]][chip] += 5000;
				break;
			case 0xA8:
				Score[Player[chip]][chip] += 6000;
				break;
			case 0xA9:
				Score[Player[chip]][chip] += 7000;
				break;
			case 0xB7:
				Score[Player[chip]][chip] += 100;
				break;
			case 0xB8:
				Score[Player[chip]][chip] += 120;
				break;
			case 0xB9:
				Score[Player[chip]][chip] += 140;
				break;
			case 0xE5:
				Score[Player[chip]][chip] += 90;
				break;
			default:
				logerror("unknown Score: %02x\n",data);
			break;
		}
	}
}

data8_t namcoio_50XX_read(int chipnum)
{
	int chip = (chipnum < 4) ? 0 : 1;
#if VERBOSE
	logerror("%04x: custom 50XX #%d read\n",activecpu_get_pc(),chip);
#endif

	switch ((in_count_50XX[chip]++) % 4)
	{
		default:
		case 0:
			{
				int flags = 0;
				int lo = (Score[Player[chip]][chip] / 1000000) % 10;
				if (Score[Player[chip]][chip] >= HiScore[chip])
				{
					HiScore[chip] = Score[Player[chip]][chip];
					flags |= 0x80;
				}
				if (Score[Player[chip]][chip] >= NextBonus[Player[chip]][chip])
				{
					if (NextBonus[Player[chip]][chip] == FirstBonus[chip])
					{
						NextBonus[Player[chip]][chip] = IntervalBonus[chip];
						flags |= 0x40;
					}
					else
					{
						NextBonus[Player[chip]][chip] += IntervalBonus[chip];
						flags |= 0x20;
					}
				}
				return lo | flags;
			}

		case 1:
			{
				int hi = (Score[Player[chip]][chip] / 100000) % 10;
				int lo = (Score[Player[chip]][chip] / 10000) % 10;
				return (hi * 16) + lo;
			}

		case 2:
			{
				int hi = (Score[Player[chip]][chip] / 1000) % 10;
				int lo = (Score[Player[chip]][chip] / 100) % 10;
				return (hi * 16) + lo;
			}

		case 3:
			{
				int hi = (Score[Player[chip]][chip] / 10) % 10;
				int lo = Score[Player[chip]][chip] % 10;
				return (hi * 16) + lo;
			}
	}
}



/*
51XX

commands:
00: nop
01 + 4 arguments: set coinage (xevious, possibly because of a bug, is different)
02: go in "credit" mode and enable start buttons
03: disable joystick remapping
04: enable joystick remapping
05: go in "switch" mode
06: nop
07: nop
*/

void namcoio_51XX_write(int chip,int data)
{
	data &= 0x07;

#if VERBOSE
	logerror("%04x: custom 51XX write %02x\n",activecpu_get_pc(),data);
#endif

	if (io[chip].coincred_mode)
	{
		switch (io[chip].coincred_mode--)
		{
			case 4: io[chip].coins_per_cred[0] = data; break;
			case 3: io[chip].creds_per_coin[0] = data; break;
			case 2: io[chip].coins_per_cred[1] = data; break;
			case 1: io[chip].creds_per_coin[1] = data; break;
		}
	}
	else
	{
		switch (data)
		{
			case 0:	/* nop */
				break;

			case 1:	/* set coinage */
				io[chip].coincred_mode = 4;
				/* this is a good time to reset the credits counter */
				io[chip].credits = 0;

				{
					/* kludge for a possible bug in Xevious */
					extern const struct GameDriver driver_xevious;

					if (Machine->gamedrv == &driver_xevious || Machine->gamedrv->clone_of == &driver_xevious)
					{
						io[chip].coincred_mode = 6;
						io[chip].remap_joy = 1;
					}
				}
				break;

			case 2:	/* go in "credits" mode and enable start buttons */
				io[chip].mode = 1;
				io[chip].in_count = 0;
				break;

			case 3:	/* disable joystick remapping */
				io[chip].remap_joy = 0;
				break;

			case 4:	/* enable joystick remapping */
				io[chip].remap_joy = 1;
				break;

			case 5:	/* go in "switch" mode */
				io[chip].mode = 0;
				io[chip].in_count = 0;
				break;

			default:
				logerror("unknown 51XX command %02x\n",data);
				break;
		}
	}
}


/* joystick input mapping

  The joystick is parsed and a number corresponding to the direction is returned,
  according to the following table:

	      0
	    7	1
	  6   8   2
	    5	3
	      4

  The values for directions impossible to obtain on a joystick have not been
  verified on Namco original hardware, but they are the same in all the bootlegs,
  so we can assume they are right.
*/
static int joy_map[16] =
/*  LDRU, LDR, LDU,  LD, LRU, LR,  LU,    L, DRU,  DR,  DU,   D,  RU,   R,   U, center */
{	 0xf, 0xe, 0xd, 0x5, 0xc, 0x9, 0x7, 0x6, 0xb, 0x3, 0xa, 0x4, 0x1, 0x2, 0x0, 0x8 };


data8_t namcoio_51XX_read(int chip)
{
#if VERBOSE
	logerror("%04x: custom 51XX read\n",activecpu_get_pc());
#endif

	if (io[chip].mode == 0)	/* switch mode */
	{
		switch ((io[chip].in_count++) % 3)
		{
			default:
			case 0: return READ_PORT(0) | (READ_PORT(1) << 4);
			case 1: return READ_PORT(2) | (READ_PORT(3) << 4);
			case 2: return 0;	/* nothing? */
		}
	}
	else	/* credits mode */
	{
		switch ((io[chip].in_count++) % 3)
		{
			default:
			case 0:	/* number of credits in BCD format */
				{
					int in,toggle;

					in = ~(READ_PORT(0) | (READ_PORT(1) << 4));
					toggle = in ^ io[chip].lastcoins;
					io[chip].lastcoins = in;

					if (io[chip].coins_per_cred[0] > 0)
					{
						if (io[chip].credits >= 9)
						{
							WRITE_PORT(1,1);	/* coin lockout */
						}
						else
						{
							WRITE_PORT(1,0);	/* coin lockout */
							/* check if the user inserted a coin */
							if (toggle & in & 0x10)
							{
								io[chip].coins[0]++;
								WRITE_PORT(0,0x04);	/* coin counter */
								WRITE_PORT(0,0x0c);
								if (io[chip].coins[0] >= io[chip].coins_per_cred[0])
								{
									io[chip].credits += io[chip].creds_per_coin[0];
									io[chip].coins[0] -= io[chip].coins_per_cred[0];
								}
							}
							if (toggle & in & 0x20)
							{
								io[chip].coins[1]++;
								WRITE_PORT(0,0x08);	/* coin counter */
								WRITE_PORT(0,0x0c);
								if (io[chip].coins[1] >= io[chip].coins_per_cred[1])
								{
									io[chip].credits += io[chip].creds_per_coin[1];
									io[chip].coins[1] -= io[chip].coins_per_cred[1];
								}
							}
							if (toggle & in & 0x40)
							{
								io[chip].credits++;
							}
						}
					}
					else io[chip].credits = 100;	/* free play */

					if (io[chip].mode == 1)
					{
						int on = (cpu_getcurrentframe() & 0x10) >> 4;

						if (io[chip].credits >= 2)
							WRITE_PORT(0,0x0c | 3*on);	/* lamps */
						else if (io[chip].credits >= 1)
							WRITE_PORT(0,0x0c | 2*on);	/* lamps */
						else
							WRITE_PORT(0,0x0c);	/* lamps off */

						/* check for 1 player start button */
						if (toggle & in & 0x04)
						{
							if (io[chip].credits >= 1)
							{
								io[chip].credits--;
								io[chip].mode = 2;
								WRITE_PORT(0,0x0c);	/* lamps off */
							}
						}
						/* check for 2 players start button */
						else if (toggle & in & 0x08)
						{
							if (io[chip].credits >= 2)
							{
								io[chip].credits -= 2;
								io[chip].mode = 2;
								WRITE_PORT(0,0x0c);	/* lamps off */
							}
						}
					}
				}

				if (~readinputport(0) & 0x80)	/* check test mode switch */
					return 0xbb;

				return (io[chip].credits / 10) * 16 + io[chip].credits % 10;

			case 1:
				{
					int joy = READ_PORT(2) & 0x0f;
					int in,toggle;

					in = ~READ_PORT(0);
					toggle = in ^ io[chip].lastbuttons;
					io[chip].lastbuttons = (io[chip].lastbuttons & 2) | (in & 1);

					/* remap joystick */
					if (io[chip].remap_joy) joy = joy_map[joy];

					/* fire */
					joy |= ((toggle & in & 0x01)^1) << 4;
					joy |= ((in & 0x01)^1) << 5;

					return joy;
				}

			case 2:
				{
					int joy = READ_PORT(3) & 0x0f;
					int in,toggle;

					in = ~READ_PORT(0);
					toggle = in ^ io[chip].lastbuttons;
					io[chip].lastbuttons = (io[chip].lastbuttons & 1) | (in & 2);

					/* remap joystick */
					if (io[chip].remap_joy) joy = joy_map[joy];

					/* fire */
					joy |= ((toggle & in & 0x02)^2) << 3;
					joy |= ((in & 0x02)^2) << 4;

					return joy;
				}
		}
	}
}


/***************************************************************************/


data8_t namcoio_ram[MAX_NAMCOIO * 16];

#define IORAM_READ(offset) (namcoio_ram[chip * 0x10 + (offset)] & 0x0f)
#define IORAM_WRITE(offset,data) {namcoio_ram[chip * 0x10 + (offset)] = (data) & 0x0f;}


static void handle_coins(int chip,int swap)
{
	int val,toggled;
	int credit_add = 0;
	int credit_sub = 0;
	int button;

/*usrintf_showmessage("%x %x %x %x %x %x %x %x",IORAM_READ(8),IORAM_READ(9),IORAM_READ(10),IORAM_READ(11),IORAM_READ(12),IORAM_READ(13),IORAM_READ(14),IORAM_READ(15)); */

	val = ~READ_PORT(0);	/* pins 38-41 */
	toggled = val ^ io[chip].lastcoins;
	io[chip].lastcoins = val;

	/* check coin insertion */
	if (val & toggled & 0x01)
	{
		io[chip].coins[0]++;
		if (io[chip].coins[0] >= (io[chip].coins_per_cred[0] & 7))
		{
			credit_add = io[chip].creds_per_coin[0] - (io[chip].coins_per_cred[0] >> 3);
			io[chip].coins[0] -= io[chip].coins_per_cred[0] & 7;
		}
		else if (io[chip].coins_per_cred[0] & 8)
			credit_add = 1;
	}
	if (val & toggled & 0x02)
	{
		io[chip].coins[1]++;
		if (io[chip].coins[1] >= (io[chip].coins_per_cred[1] & 7))
		{
			credit_add = io[chip].creds_per_coin[1] - (io[chip].coins_per_cred[1] >> 3);
			io[chip].coins[1] -= io[chip].coins_per_cred[1] & 7;
		}
		else if (io[chip].coins_per_cred[1] & 8)
			credit_add = 1;
	}
	if (val & toggled & 0x08)
	{
		credit_add = 1;
	}

	val = ~READ_PORT(3);	/* pins 30-33 */
	toggled = val ^ io[chip].lastbuttons;
	io[chip].lastbuttons = val;

	/* check start buttons, only if the game allows */
	if (IORAM_READ(9) == 0)
	/* the other argument is IORAM_READ(10) = 1, meaning unknown */
	{
		if (val & toggled & 0x04)
		{
			if (io[chip].credits >= 1) credit_sub = 1;
		}
		else if (val & toggled & 0x08)
		{
			if (io[chip].credits >= 2) credit_sub = 2;
		}
	}

	io[chip].credits += credit_add - credit_sub;

	IORAM_WRITE(0 ^ swap, io[chip].credits / 10);	/* BCD credits */
	IORAM_WRITE(1 ^ swap, io[chip].credits % 10);	/* BCD credits */
	IORAM_WRITE(2 ^ swap, credit_add);	/* credit increment (coin inputs) */
	IORAM_WRITE(3 ^ swap, credit_sub);	/* credit decrement (start buttons) */
	IORAM_WRITE(4, ~READ_PORT(1));	/* pins 22-25 */
	button = ((val & 0x05) << 1) | (val & toggled & 0x05);
	IORAM_WRITE(5, button);	/* pins 30 & 32 normal and impulse */
	IORAM_WRITE(6, ~READ_PORT(2));	/* pins 26-29 */
	button = (val & 0x0a) | ((val & toggled & 0x0a) >> 1);
	IORAM_WRITE(7, button);	/* pins 31 & 33 normal and impulse */
}



static void namco_customio_56XX_run(int chip)
{
#if VERBOSE
	logerror("execute 56XX %d mode %d\n",chip,IORAM_READ(8));
#endif

	switch (IORAM_READ(8))
	{
		case 0:	/* nop? */
			break;

		case 1:	/* read switch inputs */
			IORAM_WRITE(0, ~READ_PORT(0));	/* pins 38-41 */
			IORAM_WRITE(1, ~READ_PORT(1));	/* pins 22-25 */
			IORAM_WRITE(2, ~READ_PORT(2));	/* pins 26-29 */
			IORAM_WRITE(3, ~READ_PORT(3));	/* pins 30-33 */

/*usrintf_showmessage("%x %x %x %x %x %x %x %x",IORAM_READ(8),IORAM_READ(9),IORAM_READ(10),IORAM_READ(11),IORAM_READ(12),IORAM_READ(13),IORAM_READ(14),IORAM_READ(15)); */

			WRITE_PORT(0,IORAM_READ(9));	/* output to pins 13-16 (motos, pacnpal, gaplus) */
			WRITE_PORT(1,IORAM_READ(10));	/* output to pins 17-20 (gaplus) */
			break;

		case 2:	/* initialize coinage settings */
			io[chip].coins_per_cred[0] = IORAM_READ(9);
			io[chip].creds_per_coin[0] = IORAM_READ(10);
			io[chip].coins_per_cred[1] = IORAM_READ(11);
			io[chip].creds_per_coin[1] = IORAM_READ(12);
			/* IORAM_READ(13) = 1; meaning unknown - possibly a 3rd coin input? (there's a IPT_UNUSED bit in port A) */
			/* IORAM_READ(14) = 1; meaning unknown - possibly a 3rd coin input? (there's a IPT_UNUSED bit in port A) */
			/* IORAM_READ(15) = 0; meaning unknown */
			break;

		case 4:	/* druaga, digdug chip #1: read dip switches and inputs */
				/* superpac chip #0: process coin and start inputs, read switch inputs */
			handle_coins(chip,0);
			break;

		case 7:	/* bootup check (liblrabl only) */
			{
				/* liblrabl chip #1: 9-15 = f 1 2 3 4 0 0, expects 2 = e */
				/* liblrabl chip #2: 9-15 = 0 1 4 5 5 0 0, expects 7 = 6 */
				IORAM_WRITE(2,0xe);
				IORAM_WRITE(7,0x6);
			}
			break;

		case 8:	/* bootup check */
			{
				int i,sum;

				/* superpac: 9-15 = f f f f f f f, expects 0-1 = 6 9. 0x69 = f+f+f+f+f+f+f. */
				/* motos:    9-15 = f f f f f f f, expects 0-1 = 6 9. 0x69 = f+f+f+f+f+f+f. */
				/* phozon:   9-15 = 1 2 3 4 5 6 7, expects 0-1 = 1 c. 0x1c = 1+2+3+4+5+6+7 */
				sum = 0;
				for (i = 9;i < 16;i++)
					sum += IORAM_READ(i);
				IORAM_WRITE(0,sum >> 4);
				IORAM_WRITE(1,sum & 0xf);
			}
			break;

		case 9:	/* read dip switches and inputs */
			WRITE_PORT(0,0);	/* set pin 13 = 0 */
			IORAM_WRITE(0, ~READ_PORT(0));	/* pins 38-41, pin 13 = 0 */
			IORAM_WRITE(2, ~READ_PORT(1));	/* pins 22-25, pin 13 = 0 */
			IORAM_WRITE(4, ~READ_PORT(2));	/* pins 26-29, pin 13 = 0 */
			IORAM_WRITE(6, ~READ_PORT(3));	/* pins 30-33, pin 13 = 0 */
			WRITE_PORT(0,1);	/* set pin 13 = 1 */
			IORAM_WRITE(1, ~READ_PORT(0));	/* pins 38-41, pin 13 = 1 */
			IORAM_WRITE(3, ~READ_PORT(1));	/* pins 22-25, pin 13 = 1 */
			IORAM_WRITE(5, ~READ_PORT(2));	/* pins 26-29, pin 13 = 1 */
			IORAM_WRITE(7, ~READ_PORT(3));	/* pins 30-33, pin 13 = 1 */
			break;

		default:
			logerror("Namco I/O %d: unknown I/O mode %d\n",chip,IORAM_READ(8));
	}
}



static void namco_customio_pacnpal_run(int chip)
{
#if VERBOSE
	logerror("execute PACNPAL %d mode %d\n",chip,IORAM_READ(8));
#endif

	switch (IORAM_READ(8))
	{
		case 0:	/* nop? */
			break;

		case 3:	/* pacnpal chip #1: read dip switches and inputs */
			IORAM_WRITE(4, ~READ_PORT(0));	/* pins 38-41, pin 13 = 0 ? */
			IORAM_WRITE(5, ~READ_PORT(2));	/* pins 26-29 ? */
			IORAM_WRITE(6, ~READ_PORT(1));	/* pins 22-25 ? */
			IORAM_WRITE(7, ~READ_PORT(3));	/* pins 30-33 */
			break;

		default:
			logerror("Namco I/O %d: unknown I/O mode %d\n",chip,IORAM_READ(8));
	}
}



static void namco_customio_58XX_run(int chip)
{
#if VERBOSE
	logerror("execute 58XX %d mode %d\n",chip,IORAM_READ(8));
#endif

	switch (IORAM_READ(8))
	{
		case 0:	/* nop? */
			break;

		case 1:	/* read switch inputs */
			IORAM_WRITE(4, ~READ_PORT(0));	/* pins 38-41 */
			IORAM_WRITE(5, ~READ_PORT(1));	/* pins 22-25 */
			IORAM_WRITE(6, ~READ_PORT(2));	/* pins 26-29 */
			IORAM_WRITE(7, ~READ_PORT(3));	/* pins 30-33 */

/*usrintf_showmessage("%x %x %x %x %x %x %x %x",IORAM_READ(8),IORAM_READ(9),IORAM_READ(10),IORAM_READ(11),IORAM_READ(12),IORAM_READ(13),IORAM_READ(14),IORAM_READ(15)); */

			WRITE_PORT(0,IORAM_READ(9));	/* output to pins 13-16 (toypop) */
			WRITE_PORT(1,IORAM_READ(10));	/* output to pins 17-20 (toypop) */
			break;

		case 2:	/* initialize coinage settings */
			io[chip].coins_per_cred[0] = IORAM_READ(9);
			io[chip].creds_per_coin[0] = IORAM_READ(10);
			io[chip].coins_per_cred[1] = IORAM_READ(11);
			io[chip].creds_per_coin[1] = IORAM_READ(12);
			/* IORAM_READ(13) = 1; meaning unknown - possibly a 3rd coin input? (there's a IPT_UNUSED bit in port A) */
			/* IORAM_READ(14) = 0; meaning unknown - possibly a 3rd coin input? (there's a IPT_UNUSED bit in port A) */
			/* IORAM_READ(15) = 0; meaning unknown */
			break;

		case 3:	/* process coin and start inputs, read switch inputs */
			handle_coins(chip,2);
			break;

		case 4:	/* read dip switches and inputs */
			WRITE_PORT(0,0);	/* set pin 13 = 0 */
			IORAM_WRITE(0, ~READ_PORT(0));	/* pins 38-41, pin 13 = 0 */
			IORAM_WRITE(2, ~READ_PORT(1));	/* pins 22-25, pin 13 = 0 */
			IORAM_WRITE(4, ~READ_PORT(2));	/* pins 26-29, pin 13 = 0 */
			IORAM_WRITE(6, ~READ_PORT(3));	/* pins 30-33, pin 13 = 0 */
			WRITE_PORT(0,1);	/* set pin 13 = 1 */
			IORAM_WRITE(1, ~READ_PORT(0));	/* pins 38-41, pin 13 = 1 */
			IORAM_WRITE(3, ~READ_PORT(1));	/* pins 22-25, pin 13 = 1 */
			IORAM_WRITE(5, ~READ_PORT(2));	/* pins 26-29, pin 13 = 1 */
			IORAM_WRITE(7, ~READ_PORT(3));	/* pins 30-33, pin 13 = 1 */
			break;

		case 5:	/* bootup check */
			/* mode 5 values are checked against these numbers during power up
			   mappy:  9-15 = 3 6 5 f a c e, expects 1-7 =   8 4 6 e d 9 d
			   grobda: 9-15 = 2 3 4 5 6 7 8, expects 2 = f and 6 = c
			   phozon: 9-15 = 0 1 2 3 4 5 6, expects 0-7 = 0 2 3 4 5 6 c a
			   gaplus: 9-15 = f f f f f f f, expects 0-1 = f f

			   This has been determined to be the result of repeated XORs,
			   controlled by a 7-bit LFSR. The following algorithm should be
			   equivalent to the original one (though probably less efficient).
			   The first nibble of the result however is uncertain. It is usually
			   0, but in some cases it toggles between 0 and F. We use a kludge
			   to give Gaplus the F it expects.
			*/
			{
				int i,n,rng,seed;
				#define NEXT(n) ((((n) & 1) ? (n) ^ 0x90 : (n)) >> 1)

				/* initialize the LFSR depending on the first two arguments */
				n = (IORAM_READ(9) * 16 + IORAM_READ(10)) & 0x7f;
				seed = 0x22;
				for (i = 0;i < n;i++)
					seed = NEXT(seed);

				/* calculate the answer */
				for (i = 1;i < 8;i++)
				{
					n = 0;
					rng = seed;
					if (rng & 1) { n ^= ~IORAM_READ(11); }
					rng = NEXT(rng);
					seed = rng;	/* save state for next loop */
					if (rng & 1) { n ^= ~IORAM_READ(10); }
					rng = NEXT(rng);
					if (rng & 1) { n ^= ~IORAM_READ(9); }
					rng = NEXT(rng);
					if (rng & 1) { n ^= ~IORAM_READ(15); }
					rng = NEXT(rng);
					if (rng & 1) { n ^= ~IORAM_READ(14); }
					rng = NEXT(rng);
					if (rng & 1) { n ^= ~IORAM_READ(13); }
					rng = NEXT(rng);
					if (rng & 1) { n ^= ~IORAM_READ(12); }

					IORAM_WRITE(i,~n);
				}
				IORAM_WRITE(0,0x0);
				/* kludge for gaplus */
				if (IORAM_READ(9) == 0xf) IORAM_WRITE(0,0xf);
			}
			break;

		default:
			logerror("Namco I/O %d: unknown I/O mode %d\n",chip,IORAM_READ(8));
	}
}



READ_HANDLER( namcoio_r )
{
	/* RAM is 4-bit wide; Pac & Pal requires the | 0xf0 otherwise Easter egg doesn't work */
	offset &= 0x3f;

#if VERBOSE
	logerror("%04x: I/O read %d: mode %d, offset %d = %02x\n", activecpu_get_pc(), offset / 16, namcoio_ram[(offset & 0x30) + 8], offset & 0x0f, namcoio_ram[offset]&0x0f);
#endif

	return 0xf0 | namcoio_ram[offset];
}

WRITE_HANDLER( namcoio_w )
{
	offset &= 0x3f;
	data &= 0x0f;	/* RAM is 4-bit wide */

#if VERBOSE
	logerror("%04x: I/O write %d: offset %d = %02x\n", activecpu_get_pc(), offset / 16, offset & 0x0f, data);
#endif

	namcoio_ram[offset] = data;
}

void namcoio_set_reset_line(int chipnum, int state)
{
	io[chipnum].reset = (state == ASSERT_LINE) ? 1 : 0;
	if (state != CLEAR_LINE)
	{
		/* reset internal registers */
		io[chipnum].credits = 0;
		io[chipnum].coins[0] = 0;
		io[chipnum].coins_per_cred[0] = 1;
		io[chipnum].creds_per_coin[0] = 1;
		io[chipnum].coins[1] = 0;
		io[chipnum].coins_per_cred[1] = 1;
		io[chipnum].creds_per_coin[1] = 1;
		io[chipnum].in_count = 0;
	}
}

static void namcoio_run(int param)
{
	switch (io[param].type)
	{
		case NAMCOIO_56XX:
			namco_customio_56XX_run(param);
			break;
		case NAMCOIO_58XX:
			namco_customio_58XX_run(param);
			break;
		case NAMCOIO_PACNPAL:
			namco_customio_pacnpal_run(param);
			break;
	}
}

void namcoio_set_irq_line(int chipnum, int state)
{
	if (chipnum < MAX_NAMCOIO && state != CLEAR_LINE && !io[chipnum].reset)
	{
		/* give the cpu a tiny bit of time to write the command before processing it */
		timer_set(TIME_IN_USEC(50), chipnum, namcoio_run);
	}
}

void namcoio_init(int chipnum, int type, const struct namcoio_interface *intf)
{
	if (chipnum < MAX_NAMCOIO)
	{
		io[chipnum].type = type;
		io[chipnum].in[0] = (intf && intf->in[0]) ? intf->in[0] : nop_r;
		io[chipnum].in[1] = (intf && intf->in[1]) ? intf->in[1] : nop_r;
		io[chipnum].in[2] = (intf && intf->in[2]) ? intf->in[2] : nop_r;
		io[chipnum].in[3] = (intf && intf->in[3]) ? intf->in[3] : nop_r;
		io[chipnum].out[0] = (intf && intf->out[0]) ? intf->out[0] : nop_w;
		io[chipnum].out[1] = (intf && intf->out[1]) ? intf->out[1] : nop_w;
		namcoio_set_reset_line(chipnum,PULSE_LINE);
	}
}







void nmi_generate(int param)
{
	if (!cpunum_is_suspended(param, SUSPEND_REASON_HALT | SUSPEND_REASON_RESET | SUSPEND_REASON_DISABLE))
	{
#if VERBOSE
	logerror("NMI cpu %d\n",nmi_cpu[param]);
#endif

		cpu_set_irq_line(nmi_cpu[param], IRQ_LINE_NMI, PULSE_LINE);
	}
}


int customio_command[MAX_06XX];


void namco_06xx_init(int chipnum, int cpu,
	int type0, const struct namcoio_interface *intf0,
	int type1, const struct namcoio_interface *intf1,
	int type2, const struct namcoio_interface *intf2,
	int type3, const struct namcoio_interface *intf3)
{
	if (chipnum < MAX_06XX)
	{
		namcoio_init(4*chipnum + 0, type0, intf0);
		namcoio_init(4*chipnum + 1, type1, intf1);
		namcoio_init(4*chipnum + 2, type2, intf2);
		namcoio_init(4*chipnum + 3, type3, intf3);
		nmi_cpu[chipnum] = cpu;
		nmi_timer[chipnum] = timer_alloc(nmi_generate);
	}
}



static data8_t namcoio_53XX_digdug_read(int chip)
{
#if VERBOSE
	logerror("%04x: custom 53XX read\n",activecpu_get_pc());
#endif

	switch ((io[chip].in_count++) % 2)
	{
		default:
		case 0: return READ_PORT(0) | (READ_PORT(1) << 4);
		case 1: return READ_PORT(2) | (READ_PORT(3) << 4);
	}
}


static data8_t namcoio_53XX_polepos_read(int chip)
{
#if VERBOSE
	logerror("%04x: custom 53XX read\n",activecpu_get_pc());
#endif

	switch ((io[chip].in_count++) % 8)
	{
		case 0: return READ_PORT(0) | (READ_PORT(1) << 4);	/* steering */
		case 4: return READ_PORT(2) | (READ_PORT(3) << 4);	/* dip switches */
		default: return 0xff;	/* polepos2 hangs if 0 is returned */
	}
}


static data8_t namco_06xx_data_read(int chipnum)
{
	switch (io[chipnum].type)
	{
		case NAMCOIO_50XX: return namcoio_50XX_read(chipnum);
		case NAMCOIO_51XX: return namcoio_51XX_read(chipnum);
		case NAMCOIO_53XX_DIGDUG:  return namcoio_53XX_digdug_read(chipnum);
		case NAMCOIO_53XX_POLEPOS: return namcoio_53XX_polepos_read(chipnum);
		default:
			logerror("%04x: custom IO type %d unsupported read\n",activecpu_get_pc(),io[chipnum].type);
			return 0xff;
	}
}

static void namco_06xx_data_write(int chipnum,data8_t data)
{
	switch (io[chipnum].type)
	{
		case NAMCOIO_50XX: namcoio_50XX_write(chipnum,data); break;
		case NAMCOIO_51XX: namcoio_51XX_write(chipnum,data); break;
		case NAMCOIO_52XX: namcoio_52XX_write(data); break;
		case NAMCOIO_54XX: namcoio_54XX_write(data); break;
		default:
			logerror("%04x: custom IO type %d unsupported write\n",activecpu_get_pc(),io[chipnum].type);
			break;
	}
}

static data8_t namco_06xx_data_r(int chip,int offset)
{
#if VERBOSE
	logerror("%04x: custom IO read offset %d\n",activecpu_get_pc(),offset);
#endif

	if (!(customio_command[chip] & 0x10))
	{
		logerror("%04x: custom IO read in write mode %02x\n",activecpu_get_pc(),customio_command[chip]);
		return 0;
	}

	switch (customio_command[chip] & 0xf)
	{
		case 0x1: return namco_06xx_data_read(4*chip + 0); break;
		case 0x2: return namco_06xx_data_read(4*chip + 1); break;
		case 0x4: return namco_06xx_data_read(4*chip + 2); break;
		case 0x8: return namco_06xx_data_read(4*chip + 3); break;
		default:
			logerror("%04x: custom IO read in unsupported mode %02x\n",activecpu_get_pc(),customio_command[chip]);
			return 0xff;
	}
}

static void namco_06xx_data_w(int chip,int offset,int data)
{
#if VERBOSE
	logerror("%04x: custom IO write offset %d = %02x\n",activecpu_get_pc(),offset,data);
#endif

	if (customio_command[chip] & 0x10)
	{
		logerror("%04x: custom IO write in read mode %02x\n",activecpu_get_pc(),customio_command[chip]);
		return;
	}

	switch (customio_command[chip] & 0xf)
	{
		case 0x1: namco_06xx_data_write(4*chip + 0,data); break;
		case 0x2: namco_06xx_data_write(4*chip + 1,data); break;
		case 0x4: namco_06xx_data_write(4*chip + 2,data); break;
		case 0x8: namco_06xx_data_write(4*chip + 3,data); break;
		default:
			logerror("%04x: custom IO write in unsupported mode %02x\n",activecpu_get_pc(),customio_command[chip]);
			break;
	}
}


static data8_t namco_06xx_ctrl_r(int chip)
{
	return customio_command[chip];
}

static void namco_06xx_ctrl_w(int chip,int data)
{
#if VERBOSE
	logerror("%04x: custom IO command %02x\n",activecpu_get_pc(),data);
#endif

	customio_command[chip] = data;

	if ((customio_command[chip] & 0x0f) == 0)
		timer_adjust(nmi_timer[chip], TIME_NEVER, chip, 0);
	else
		timer_adjust(nmi_timer[chip], TIME_IN_USEC(200), chip, TIME_IN_USEC(200));
}



READ_HANDLER( namco_06xx_0_data_r )		{ return namco_06xx_data_r(0,offset); }
READ_HANDLER( namco_06xx_1_data_r )		{ return namco_06xx_data_r(1,offset); }
WRITE_HANDLER( namco_06xx_0_data_w )	{ namco_06xx_data_w(0,offset,data); }
WRITE_HANDLER( namco_06xx_1_data_w )	{ namco_06xx_data_w(1,offset,data); }
READ_HANDLER( namco_06xx_0_ctrl_r )		{ return namco_06xx_ctrl_r(0); }
READ_HANDLER( namco_06xx_1_ctrl_r )		{ return namco_06xx_ctrl_r(1); }
WRITE_HANDLER( namco_06xx_0_ctrl_w )	{ namco_06xx_ctrl_w(0,data); }
WRITE_HANDLER( namco_06xx_1_ctrl_w )	{ namco_06xx_ctrl_w(1,data); }
