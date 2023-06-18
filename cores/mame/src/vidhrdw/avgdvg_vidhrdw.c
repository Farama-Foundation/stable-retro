/*************************************************************************

	avgdvg.c: Atari DVG and AVG simulators

	Copyright 1991, 1992, 1996 Eric Smith

	Modified for the MAME project 1997 by
	Brad Oliver, Bernd Wiebelt, Aaron Giles, Andrew Caldwell

	971108 Disabled vector timing routines, introduced an ugly (but fast!)
			busy flag hack instead. BW
	980202 New anti aliasing code by Andrew Caldwell (.ac)
	980206 New (cleaner) busy flag handling.
			Moved LBO's buffered point into generic vector code. BW
	980212 Introduced timing code based on Aaron timer routines. BW
	980318 Better color handling, Bzone and MHavoc clipping. BW

	Battlezone uses a red overlay for the top of the screen and a green one
	for the rest. There is a circuit to clip color 0 lines extending to the
	red zone. This is emulated now. Thanks to Neil Bradley for the info. BW

	Frame and interrupt rates (Neil Bradley) BW
		~60 fps/4.0ms: Asteroid, Asteroid Deluxe
		~40 fps/4.0ms: Lunar Lander
		~40 fps/4.1ms: Battle Zone
		~45 fps/5.4ms: Space Duel, Red Baron
		~30 fps/5.4ms: StarWars

	Games with self adjusting framerate
		4.1ms: Black Widow, Gravitar
		4.1ms: Tempest
		Major Havoc
		Quantum

	TODO: accurate vector timing (need timing diagramm)

************************************************************************/

#include "driver.h"
#include "avgdvg.h"
#include "vector.h"
#include "log.h"


/*************************************
 *
 *	Constants
 *
 *************************************/

#define BRIGHTNESS	12

#define MAXSTACK	8		/* Tempest needs more than 4 */

/* AVG commands */
#define VCTR		0
#define HALT		1
#define SVEC		2
#define STAT		3
#define CNTR		4
#define JSRL		5
#define RTSL		6
#define JMPL		7
#define SCAL		8

/* DVG commands */
#define DVCTR		0x01
#define DLABS		0x0a
#define DHALT		0x0b
#define DJSRL		0x0c
#define DRTSL		0x0d
#define DJMPL		0x0e
#define DSVEC		0x0f



/*************************************
 *
 *	Static variables
 *
 *************************************/

static UINT8 vector_engine;
static UINT8 flipword;
static UINT8 busy;
static rgb_t colorram[32];

static int width, height;
static int xcenter, ycenter;
static int xmin, xmax;
static int ymin, ymax;

static int flip_x, flip_y, swap_xy;

int vector_updates; /* avgdvg_go_w()'s per Mame frame, should be 1 */


#define BANK_SIZE (0x2000)
#define NUM_BANKS (2)
static unsigned char *vectorbank[NUM_BANKS];

static rgb_t sparkle_callback(void);



/*************************************
 *
 *	Compute 2's complement value
 *
 *************************************/

static INLINE int twos_comp_val(int num, int bits)
{
	return (INT32)(num << (32 - bits)) >> (32 - bits);
}



/*************************************
 *
 *	Vector RAM accesses
 *
 *************************************/

static INLINE UINT16 vector_word(UINT16 offset)
{
	UINT8 *base;

	/* convert from word offset to byte */
	offset *= 2;

	/* get address of the word */
	base = &vectorbank[offset / BANK_SIZE][offset % BANK_SIZE];

	/* result is based on flipword */
	if (flipword)
		return base[1] | (base[0] << 8);
	else
		return base[0] | (base[1] << 8);
}



/*************************************
 *
 *	Vector timing
 *
 *************************************/

static INLINE int vector_timer(int deltax, int deltay)
{
	deltax = abs(deltax);
	deltay = abs(deltay);
	if (deltax > deltay)
		return deltax >> 16;
	else
		return deltay >> 16;
}


static INLINE int dvg_vector_timer(int scale)
{
	return scale;
}



/*************************************
 *
 *	AVG brightness computation
 *
 *************************************/

static INLINE int effective_z(int z, int statz)
{
	/* Star Wars blends Z and an 8-bit STATZ */
	/* STATZ of 128 should give highest intensity */
	if (vector_engine == USE_AVG_SWARS)
	{
		z = (z * statz) / (options.translucency ? 12 : 8);
		if (z > 0xff)
			z = 0xff;
	}

	/* everyone else uses this */
	else
	{
		/* special case for Alpha One -- no idea if this is right */
		if (vector_engine == USE_AVG_ALPHAONE)
		{
			if (z)
				z ^= 0x15;
		}

		/* Z == 2 means use the value from STATZ */
		else if (z == 2)
			z = statz;

		z *= (options.translucency) ? BRIGHTNESS : 16;
	}

	return z;
}



/*************************************
 *
 *	DVG vector generator
 *
 *************************************/

static int dvg_generate_vector_list(void)
{
	static const char *dvg_mnem[] =
	{
		"????", "vct1", "vct2", "vct3",
		"vct4", "vct5", "vct6", "vct7",
		"vct8", "vct9", "labs", "halt",
		"jsrl", "rtsl", "jmpl", "svec"
	};

	int stack[MAXSTACK];
	int pc = 0;
	int sp = 0;
	int scale = 0;
	int currentx = 0, currenty = 0;
	int total_length = 1;
	int done = 0;

	int firstwd, secondwd = 0;
	int opcode;
	int x, y, z, temp, a;
	int deltax, deltay;

	/* reset the vector list */
	vector_clear_list();

	/* loop until finished */
	while (!done)
	{
		/* fetch the first word and get its opcode */
		firstwd = vector_word(pc++);
		opcode = firstwd >> 12;

		/* the DVCTR and DLABS opcodes take two words */
		if (opcode >= 0 && opcode <= DLABS)
			secondwd = vector_word(pc++);

		/* debugging */
		log_cb(RETRO_LOG_DEBUG, LOGPRE "%4x: %4x ", pc, firstwd);
		if (opcode <= DLABS)
		{
			(void)dvg_mnem;
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%s ", dvg_mnem[opcode]);
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%4x  ", secondwd);
		}

		/* switch off the opcode */
		switch (opcode)
		{
			/* 0 is an invalid opcode */
			case 0:
	 			log_cb(RETRO_LOG_DEBUG, LOGPRE "Error: DVG opcode 0!  Addr %4x Instr %4x %4x\n", pc-2, firstwd, secondwd);
#ifdef VG_DEBUG
				done = 1;
				break;
#endif

			/* 1-9 are DVCTR ops: draw a vector */
			case 1:
			case 2:
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
			case 9:

				/* compute raw X and Y values */
	  			y = firstwd & 0x03ff;
				x = secondwd & 0x3ff;
				if (firstwd & 0x400)
					y = -y;
				if (secondwd & 0x400)
					x = -x;

				/* determine the brightness */
				z = secondwd >> 12;
				log_cb(RETRO_LOG_DEBUG, LOGPRE "(%d,%d) z: %d scal: %d", x, y, z, opcode);

				/* compute the effective brightness */
				z = effective_z(z, z);

				/* determine the scale factor; scale 9 means -1 */
	  			temp = ((scale + opcode) & 0x0f);
	  			if (temp > 9)
					temp = -1;

				/* compute the deltas */
	  			deltax = (x << 16) >> (9-temp);
				deltay = (y << 16) >> (9-temp);

				/* adjust the current position and compute timing */
	  			currentx += deltax;
				currenty -= deltay;
				total_length += dvg_vector_timer(temp);

				/* add the new point */
				vector_add_point(currentx, currenty, colorram[1], z);
				break;

			/* DSVEC: draw a short vector */
			case DSVEC:

				/* compute raw X and Y values */
				y = firstwd & 0x0300;
				x = (firstwd & 0x03) << 8;
				if (firstwd & 0x0400)
					y = -y;
				if (firstwd & 0x04)
					x = -x;

				/* determine the brightness */
				z = (firstwd >> 4) & 0x0f;

				/* compute the effective brightness */
				z = effective_z(z, z);

				/* determine the scale factor; scale 9 means -1 */
				temp = 2 + ((firstwd >> 2) & 0x02) + ((firstwd >> 11) & 0x01);
	  			temp = (scale + temp) & 0x0f;
				if (temp > 9)
					temp = -1;
				log_cb(RETRO_LOG_DEBUG, LOGPRE "(%d,%d) z: %d scal: %d", x, y, z, temp);

				/* compute the deltas */
				deltax = (x << 16) >> (9 - temp);
				deltay = (y << 16) >> (9 - temp);

				/* adjust the current position and compute timing */
	  			currentx += deltax;
				currenty -= deltay;
				total_length += dvg_vector_timer(temp);

				/* add the new point */
				vector_add_point(currentx, currenty, colorram[1], z);
				break;

			/* DLABS: move to an absolute location */
			case DLABS:

				/* extract the new X,Y coordinates */
				y = twos_comp_val(firstwd, 12);
				x = twos_comp_val(secondwd, 12);

				/* global scale comes from upper 4 bits of second word */
	  			scale = secondwd >> 12;

	  			/* set the current X,Y */
				currentx = (x - xmin) << 16;
				currenty = (ymax - y) << 16;
				log_cb(RETRO_LOG_DEBUG, LOGPRE "(%d,%d) scal: %d", x, y, secondwd >> 12);
				break;

			/* DRTSL: return from subroutine */
			case DRTSL:

				/* handle stack underflow */
				if (sp == 0)
	    		{
					log_cb(RETRO_LOG_DEBUG, LOGPRE "\n*** Vector generator stack underflow! ***\n");
					done = 1;
					sp = MAXSTACK - 1;
				}
				else
					sp--;

				/* pull the new PC from the stack */
				pc = stack[sp];

				/* debugging */
				if (firstwd & 0x1fff)
					log_cb(RETRO_LOG_DEBUG, LOGPRE "(%d?)", firstwd & 0x1fff);
				break;

			/* DHALT: all done! */
			case DHALT:
				done = 1;

				/* debugging */
				if (firstwd & 0x1fff)
      				log_cb(RETRO_LOG_DEBUG, LOGPRE "(%d?)", firstwd & 0x0fff);
				break;

			/* DJMPL: jump to a new program location */
			case DJMPL:
				a = firstwd & 0x0fff;
				log_cb(RETRO_LOG_DEBUG, LOGPRE "%4x", a);
				pc = a;

				if (!pc)
					done=1;
				break;

			/* DJSRL: jump to a new program location as subroutine */
			case DJSRL:
				a = firstwd & 0x0fff;
				log_cb(RETRO_LOG_DEBUG, LOGPRE "%4x", a);

				/* push the next PC on the stack */
				stack[sp] = pc;

				/* check for stack overflows */
				if (sp == (MAXSTACK - 1))
	    		{
					log_cb(RETRO_LOG_DEBUG, LOGPRE "\n*** Vector generator stack overflow! ***\n");
					done = 1;
					sp = 0;
				}
				else
					sp++;

				/* jump to the new location */
				pc = a;
				break;

			/* anything else gets caught here */
			default:
				log_cb(RETRO_LOG_DEBUG, LOGPRE "Unknown DVG opcode found\n");
				done = 1;
		}
   		log_cb(RETRO_LOG_DEBUG, LOGPRE "\n");
	}

	/* return the total length of everything drawn */
	return total_length;
}

void avg_set_flip_x(int flip)
{
	flip_x = flip;
}

void avg_set_flip_y(int flip)
{
	flip_y = flip;
}

void avg_apply_flipping_and_swapping(int *x, int *y)
{
	if (flip_x)
		*x += (xcenter-*x)<<1;
	if (flip_y)
		*y += (ycenter-*y)<<1;

	if (swap_xy)
	{
		int temp = *x;
		*x = *y - ycenter + xcenter;
		*y = temp - xcenter + ycenter;
	}
}

void avg_add_point(int x, int y, rgb_t color, int intensity)
{
	avg_apply_flipping_and_swapping(&x, &y);
	vector_add_point(x, y, color, intensity);
}

void avg_add_point_callback(int x, int y, rgb_t (*color_callback)(void), int intensity)
{
	avg_apply_flipping_and_swapping(&x, &y);
	vector_add_point_callback(x, y, color_callback, intensity);
}

/*************************************
 *
 *	AVG vector generator
 *
 *************************************

	Atari Analog Vector Generator Instruction Set

	Compiled from Atari schematics and specifications
	Eric Smith  7/2/92
	---------------------------------------------

	NOTE: The vector generator is little-endian.  The instructions are 16 bit
	      words, which need to be stored with the least significant byte in the
	      lower (even) address.  They are shown here with the MSB on the left.

	The stack allows four levels of subroutine calls in the TTL version, but only
	three levels in the gate array version.

	inst  bit pattern          description
	----  -------------------  -------------------
	VCTR  000- yyyy yyyy yyyy  normal vector
	      zzz- xxxx xxxx xxxx
	HALT  001- ---- ---- ----  halt - does CNTR also on newer hardware
	SVEC  010y yyyy zzzx xxxx  short vector - don't use zero length
	STAT  0110 ---- zzzz cccc  status
	SCAL  0111 -bbb llll llll  scaling
	CNTR  100- ---- dddd dddd  center
	JSRL  101a aaaa aaaa aaaa  jump to subroutine
	RTSL  110- ---- ---- ----  return
	JMPL  111a aaaa aaaa aaaa  jump

	-     unused bits
	x, y  relative x and y coordinates in two's complement (5 or 13 bit,
	      5 bit quantities are scaled by 2, so x=1 is really a length 2 vector.
	z     intensity, 0 = blank, 1 means use z from STAT instruction,  2-7 are
	      doubled for actual range of 4-14
	c     color
	b     binary scaling, multiplies all lengths by 2**(1-b), 0 is double size,
	      1 is normal, 2 is half, 3 is 1/4, etc.
	l     linear scaling, multiplies all lengths by 1-l/256, don't exceed $80
	d     delay time, use $40
	a     address (word address relative to base of vector memory)

	Notes:

	Quantum:
	        the VCTR instruction has a four bit Z field, that is not
	        doubled.  The value 2 means use Z from STAT instruction.

	        the SVEC instruction can't be used

	Major Havoc:
	        SCAL bit 11 is used for setting a Y axis window.

	        STAT bit 11 is used to enable "sparkle" color.
	        STAT bit 10 inverts the X axis of vectors.
	        STAT bits 9 and 8 are the Vector ROM bank select.

	Star Wars:
	        STAT bits 10, 9, and 8 are used directly for R, G, and B.

 *************************************/

static int avg_generate_vector_list(void)
{
	static const char *avg_mnem[] =
	{
		"vctr", "halt", "svec", "stat", "cntr",
		"jsrl", "rtsl", "jmpl", "scal"
	};

	int stack[MAXSTACK];
	int pc = 0;
	int sp = 0;
	int scale = 0;
	int statz = 0;
	int sparkle = 0;
	int xflip = 0;
	int color = 0;
	int ywindow = 1;
	int currentx = xcenter;
	int currenty = ycenter;
	int total_length = 1;
	int done = 0;

	int firstwd, secondwd = 0;
	int opcode;
	int x, y, z = 0, b, l, d, a;
	int deltax, deltay;

	/* check for zeroed vector RAM */
	if (vector_word(pc) == 0 && vector_word(pc + 1) == 0)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "VGO with zeroed vector memory\n");
		return total_length;
	}

	/* kludge to bypass Major Havoc's empty frames */
	if ((vector_engine == USE_AVG_MHAVOC || vector_engine == USE_AVG_ALPHAONE) && vector_word(pc) == 0xafe2)
		return total_length;

	/* reset the vector list */
	vector_clear_list();

	/* loop until finished... */
	while (!done)
	{
		/* fetch the first word and get its opcode */
		firstwd = vector_word(pc++);
		opcode = firstwd >> 13;

		/* the VCTR opcode takes two words */
		if (opcode == VCTR)
			secondwd = vector_word(pc++);

		/* SCAL is a variant of STAT; convert it here */
		else if (opcode == STAT && (firstwd & 0x1000))
			opcode = SCAL;

		/* debugging */
		(void)avg_mnem;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "%4x: %4x ", pc, firstwd);
		if (opcode == VCTR)
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%4x  ", secondwd);
		else
			log_cb(RETRO_LOG_DEBUG, LOGPRE "      ");
		log_cb(RETRO_LOG_DEBUG, LOGPRE "%s ", avg_mnem[opcode]);

		/* switch off the opcode */
		switch (opcode)
		{
			/* VCTR: draw a long vector */
			case VCTR:

				/* Quantum uses 12-bit vectors and a 4-bit Z value */
				if (vector_engine == USE_AVG_QUANTUM)
				{
					x = twos_comp_val(secondwd, 12);
					y = twos_comp_val(firstwd, 12);
					z = (secondwd >> 12) & 0x0f;
				}

				/* everyone else uses 13-bit vectors and a 3-bit Z value */
				else
				{
					x = twos_comp_val(secondwd, 13);
					y = twos_comp_val(firstwd, 13);
					z = (secondwd >> 12) & 0x0e;
				}

				/* compute the effective brightness */
				z = effective_z(z, statz);

				/* compute the deltas */
				deltax = x * scale;
				deltay = y * scale;
				if (xflip) deltax = -deltax;

				/* adjust the current position and compute timing */
				currentx += deltax;
				currenty -= deltay;
				total_length += vector_timer(deltax, deltay);

				/* add the new point */
				if (sparkle)
					avg_add_point_callback(currentx, currenty, sparkle_callback, z);
				else
					avg_add_point(currentx, currenty, colorram[color], z);
				log_cb(RETRO_LOG_DEBUG, LOGPRE "VCTR x:%d y:%d z:%d statz:%d", x, y, z, statz);
				break;

			/* SVEC: draw a short vector */
			case SVEC:

				/* Quantum doesn't support this */
				if (vector_engine == USE_AVG_QUANTUM)
					break;

				/* two 5-bit vectors plus a 3-bit Z value */
				x = twos_comp_val(firstwd, 5) * 2;
				y = twos_comp_val(firstwd >> 8, 5) * 2;
				z = (firstwd >> 4) & 0x0e;

				/* compute the effective brightness */
				z = effective_z(z, statz);

				/* compute the deltas */
				deltax = x * scale;
				deltay = y * scale;
				if (xflip) deltax = -deltax;

				/* adjust the current position and compute timing */
				currentx += deltax;
				currenty -= deltay;
				total_length += vector_timer(deltax, deltay);

				/* add the new point */
				if (sparkle)
					avg_add_point_callback(currentx, currenty, sparkle_callback, z);
				else
					avg_add_point(currentx, currenty, colorram[color], z);
				log_cb(RETRO_LOG_DEBUG, LOGPRE "SVEC x:%d y:%d z:%d statz:%d", x, y, z, statz);
				break;

			/* STAT: control colors, clipping, sparkling, and flipping */
			case STAT:

				/* Star Wars takes RGB directly and has an 8-bit brightness */
				if (vector_engine == USE_AVG_SWARS)
				{
					color = (firstwd >> 8) & 7;
					statz = firstwd & 0xff;
				}

				/* everyone else has a 4-bit color and 4-bit brightness */
				else
				{
					color = firstwd & 0x0f;
					statz = (firstwd >> 4) & 0x0f;
				}

				/* Tempest has the sparkle bit in bit 11 */
				if (vector_engine == USE_AVG_TEMPEST)
					sparkle = !(firstwd & 0x0800);

				/* Major Havoc/Alpha One have sparkle bit, xflip, and banking */
				else if (vector_engine == USE_AVG_MHAVOC || vector_engine == USE_AVG_ALPHAONE)
				{
					sparkle = firstwd & 0x0800;
					xflip = firstwd & 0x0400;
					vectorbank[1] = &memory_region(REGION_CPU1)[0x18000 + ((firstwd >> 8) & 3) * 0x2000];
				}

				/* BattleZone has a clipping circuit */
				else if (vector_engine == USE_AVG_BZONE)
				{
					int newymin = (color == 0) ? 0x0050 : ymin;
					vector_add_clip(xmin << 16, newymin << 16,
									xmax << 16, ymax << 16);
				}

				/* debugging */
				log_cb(RETRO_LOG_DEBUG, LOGPRE "STAT: statz: %d color: %d", statz, color);
				if (xflip || sparkle)
					log_cb(RETRO_LOG_DEBUG, LOGPRE "xflip: %02x  sparkle: %02x\n", xflip, sparkle);
				break;

			/* SCAL: set the scale factor */
			case SCAL:
				b = ((firstwd >> 8) & 7) + 8;
				l = ~firstwd & 0xff;
				scale = (l << 16) >> b;

				/* Y-Window toggle for Major Havoc */
				if (vector_engine == USE_AVG_MHAVOC || vector_engine == USE_AVG_ALPHAONE)
					if (firstwd & 0x0800)
					{
						int newymin = ymin;
						log_cb(RETRO_LOG_DEBUG, LOGPRE "CLIP %d\n", firstwd & 0x0800);

						/* toggle the current state */
						ywindow = !ywindow;

						/* adjust accordingly */
						if (ywindow)
							newymin = (vector_engine == USE_AVG_MHAVOC) ? 0x0048 : 0x0083;
						vector_add_clip(xmin << 16, newymin << 16,
										xmax << 16, ymax << 16);
					}

				/* debugging */
				log_cb(RETRO_LOG_DEBUG, LOGPRE "bin: %d, lin: ", b);
				if (l > 0x80)
					log_cb(RETRO_LOG_DEBUG, LOGPRE "(%d?)", l);
				else
					log_cb(RETRO_LOG_DEBUG, LOGPRE "%d", l);
				log_cb(RETRO_LOG_DEBUG, LOGPRE " scale: %f", (scale/(float)(1<<16)));
				break;

			/* CNTR: center the beam */
			case CNTR:

				/* delay stored in low 8 bits; normally is 0x40 */
				d = firstwd & 0xff;
				if (d != 0x40) log_cb(RETRO_LOG_DEBUG, LOGPRE "%d", d);

				/* move back to the middle */
				currentx = xcenter;
				currenty = ycenter;
				avg_add_point(currentx, currenty, 0, 0);
				break;

			/* RTSL: return from subroutine */
			case RTSL:

				/* handle stack underflow */
				if (sp == 0)
				{
					log_cb(RETRO_LOG_DEBUG, LOGPRE "\n*** Vector generator stack underflow! ***\n");
					done = 1;
					sp = MAXSTACK - 1;
				}
				else
					sp--;

				/* pull the new PC from the stack */
				pc = stack[sp];

				/* debugging */
				if (firstwd & 0x1fff)
					log_cb(RETRO_LOG_DEBUG, LOGPRE "(%d?)", firstwd & 0x1fff);
				break;

			/* HALT: all done! */
			case HALT:
				done = 1;

				/* debugging */
				if (firstwd & 0x1fff)
					log_cb(RETRO_LOG_DEBUG, LOGPRE "(%d?)", firstwd & 0x1fff);
				break;

			/* JMPL: jump to a new program location */
			case JMPL:
				a = firstwd & 0x1fff;
				log_cb(RETRO_LOG_DEBUG, LOGPRE "%4x", a);

				/* if a = 0x0000, treat as HALT */
				if (a == 0x0000)
					done = 1;
				else
					pc = a;
				break;

			/* JSRL: jump to a new program location as subroutine */
			case JSRL:
				a = firstwd & 0x1fff;
				log_cb(RETRO_LOG_DEBUG, LOGPRE "%4x", a);

				/* if a = 0x0000, treat as HALT */
				if (a == 0x0000)
					done = 1;
				else
				{
					/* push the next PC on the stack */
					stack[sp] = pc;

					/* check for stack overflows */
					if (sp == (MAXSTACK - 1))
					{
						log_cb(RETRO_LOG_DEBUG, LOGPRE "\n*** Vector generator stack overflow! ***\n");
						done = 1;
						sp = 0;
					}
					else
						sp++;

					/* jump to the new location */
					pc = a;
				}
				break;

			/* anything else gets caught here */
			default:
				log_cb(RETRO_LOG_DEBUG, LOGPRE "internal error\n");
		}
		log_cb(RETRO_LOG_DEBUG, LOGPRE "\n");
	}

	/* return the total length of everything drawn */
	return total_length;
}



/*************************************
 *
 *	AVG execution/busy detection
 *
 ************************************/

int avgdvg_done(void)
{
	return !busy;
}


static void avgdvg_clr_busy(int dummy)
{
	busy = 0;
}


WRITE_HANDLER( avgdvg_go_w )
{
	int total_length;

	/* skip if already busy */
	if (busy)
		return;

	/* count vector updates and mark ourselves busy */
	vector_updates++;
	busy = 1;

	/* DVG case */
	if (vector_engine == USE_DVG)
	{
		total_length = dvg_generate_vector_list();
		timer_set(TIME_IN_NSEC(4500) * total_length, 0, avgdvg_clr_busy);
	}

	/* AVG case */
	else
	{
		total_length = avg_generate_vector_list();

		/* for Major Havoc, we need to look for empty frames */
		if (total_length > 1)
			timer_set(TIME_IN_NSEC(1500) * total_length, 0, avgdvg_clr_busy);
		else
		{
			vector_updates--;
			busy = 0;
		}
	}
}


WRITE16_HANDLER( avgdvg_go_word_w )
{
	avgdvg_go_w(offset, data);
}



/*************************************
 *
 *	AVG reset
 *
 ************************************/

WRITE_HANDLER( avgdvg_reset_w )
{
	avgdvg_clr_busy(0);
}


WRITE16_HANDLER( avgdvg_reset_word_w )
{
	avgdvg_clr_busy(0);
}



/*************************************
 *
 *	Vector generator init
 *
 ************************************/

int avgdvg_init(int vector_type)
{
	int i;

	/* 0 vector RAM size is invalid */
	if (vectorram_size == 0)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Error: vectorram_size not initialized\n");
		return 1;
	}

	/* initialize the pages */
	for (i = 0; i < NUM_BANKS; i++)
		vectorbank[i] = vectorram + i * BANK_SIZE;
	if (vector_type == USE_AVG_MHAVOC || vector_type == USE_AVG_ALPHAONE)
		vectorbank[1] = &memory_region(REGION_CPU1)[0x18000];

	/* set the engine type and validate it */
	vector_engine = vector_type;
	if (vector_engine < AVGDVG_MIN || vector_engine > AVGDVG_MAX)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Error: unknown Atari Vector Game Type\n");
		return 1;
	}

	/* Star Wars is reverse-endian */
	if (vector_engine == USE_AVG_SWARS)
		flipword = 1;

	/* Quantum may be reverse-endian depending on the platform */
#ifdef MSB_FIRST
	else if (vector_engine==USE_AVG_QUANTUM)
		flipword = 1;
#endif

	/* everyone else is standard */
	else
		flipword = 0;

	/* clear the busy state */
	busy = 0;

	/* compute the min/max values */
	xmin = Machine->visible_area.min_x;
	ymin = Machine->visible_area.min_y;
	xmax = Machine->visible_area.max_x;
	ymax = Machine->visible_area.max_y;
	width = xmax - xmin;
	height = ymax - ymin;

	/* determine the center points */
	xcenter = ((xmax + xmin) / 2) << 16;
	ycenter = ((ymax + ymin) / 2) << 16;

	/* initialize to no avg flipping */
	flip_x = flip_y = 0;

	/* Tempest and Quantum have X and Y swapped */
	if ((vector_type == USE_AVG_TEMPEST) ||
		(vector_type == USE_AVG_QUANTUM))
		swap_xy = 1;
	else
		swap_xy = 0;

	return video_start_vector();
}



/*************************************
 *
 *	Video startup
 *
 ************************************/

VIDEO_START( dvg )
{
	return avgdvg_init(USE_DVG);
}


VIDEO_START( avg )
{
	return avgdvg_init(USE_AVG);
}


VIDEO_START( avg_starwars )
{
	return avgdvg_init(USE_AVG_SWARS);
}


VIDEO_START( avg_tempest )
{
	return avgdvg_init(USE_AVG_TEMPEST);
}


VIDEO_START( avg_mhavoc )
{
	return avgdvg_init(USE_AVG_MHAVOC);
}


VIDEO_START( avg_alphaone )
{
	return avgdvg_init(USE_AVG_ALPHAONE);
}


VIDEO_START( avg_bzone )
{
	return avgdvg_init(USE_AVG_BZONE);
}


VIDEO_START( avg_quantum )
{
	return avgdvg_init(USE_AVG_QUANTUM);
}


VIDEO_START( avg_redbaron )
{
	return avgdvg_init(USE_AVG_RBARON);
}



/*************************************
 *
 *	Palette generation
 *
 ************************************/

/* Black and White vector colors for Asteroids, Lunar Lander, Omega Race */
PALETTE_INIT( avg_white )
{
	int i;
	for (i = 0; i < 32; i++)
		colorram[i] = MAKE_RGB(0xff, 0xff, 0xff);
}


/* Basic 8 rgb vector colors for Tempest, Gravitar, Major Havoc etc. */
PALETTE_INIT( avg_multi )
{
	int i;
	for (i = 0; i < 32; i++)
		colorram[i] = VECTOR_COLOR111(i);
}



/*************************************
 *
 *	Color RAM handling
 *
 ************************************/

WRITE_HANDLER( tempest_colorram_w )
{
	int bit3 = (~data >> 3) & 1;
	int bit2 = (~data >> 2) & 1;
	int bit1 = (~data >> 1) & 1;
	int bit0 = (~data >> 0) & 1;
	int r = bit1 * 0xee + bit0 * 0x11;
	int g = bit3 * 0xee;
	int b = bit2 * 0xee;

	colorram[offset] = MAKE_RGB(r, g, b);
}


WRITE_HANDLER( mhavoc_colorram_w )
{
	int bit3 = (~data >> 3) & 1;
	int bit2 = (~data >> 2) & 1;
	int bit1 = (~data >> 1) & 1;
	int bit0 = (~data >> 0) & 1;
	int r = bit3 * 0xee + bit2 * 0x11;
	int g = bit1 * 0xee;
	int b = bit0 * 0xee;

	colorram[offset] = MAKE_RGB(r, g, b);
}


WRITE16_HANDLER( quantum_colorram_w )
{
	if (ACCESSING_LSB)
	{
		int bit3 = (~data >> 3) & 1;
		int bit2 = (~data >> 2) & 1;
		int bit1 = (~data >> 1) & 1;
		int bit0 = (~data >> 0) & 1;
		int r = bit3 * 0xee;
		int g = bit1 * 0xee + bit0 * 0x11;
		int b = bit2 * 0xee;

		colorram[offset & 0x0f] = MAKE_RGB(r, g, b);
	}
}


static rgb_t sparkle_callback(void)
{
	return colorram[16 + ((rand() >> 8) & 15)];
}
