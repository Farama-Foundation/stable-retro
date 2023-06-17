/*
   GI SP0250 digital LPC sound synthesizer

   By O. Galibert.

   Unknown:
   - Exact noise algorithm
   - Exact noise pitch (probably ok)
   - Exact main frequency (probably ok)
   - Exact amplitude decoding (aka mantissa:exponent)
   - 7 bits output mapping
   - Whether the pitch starts counting from 0 or 1

   Sound quite reasonably already though.
*/

#include <math.h>
#include "driver.h"
#include "cpuintrf.h"

enum { MAIN_CLOCK = 10000 };

static struct {
	INT16 amp;
	UINT8 pitch;
	UINT8 repeat;
	UINT8 pcount, rcount;
	UINT8 pcounto, rcounto;
	UINT32 RNG;
	int stream;
	int voiced;
	UINT8 fifo[15];
	int fifo_pos;

	void (*drq)(int state);

	struct {
		INT16 F, B;
		INT16 z1, z2;
	} filter[6];
} sp0250;

/* Internal ROM to the chip, cf. manual*/

static UINT16 coefs[128] = {
	  0,   9,  17,  25,  33,  41,  49,  57,  65,  73,  81,  89,  97, 105, 113, 121,
	129, 137, 145, 153, 161, 169, 177, 185, 193, 201, 203, 217, 225, 233, 241, 249,
	257, 265, 273, 281, 289, 297, 301, 305, 309, 313, 317, 321, 325, 329, 333, 337,
	341, 345, 349, 353, 357, 361, 365, 369, 373, 377, 381, 385, 389, 393, 397, 401,
	405, 409, 413, 417, 421, 425, 427, 429, 431, 433, 435, 437, 439, 441, 443, 445,
	447, 449, 451, 453, 455, 457, 459, 461, 463, 465, 467, 469, 471, 473, 475, 477,
	479, 481, 482, 483, 484, 485, 486, 487, 488, 489, 490, 491, 492, 493, 494, 495,
	496, 497, 498, 499, 500, 501, 502, 503, 504, 505, 506, 507, 508, 509, 510, 511
};

/* To be checked, somehow*/

static UINT16 sp0250_ga(UINT8 v)
{
	return (v & 0x1f) << (v>>5);
}

static INT16 sp0250_gc(UINT8 v)
{
	INT16 res = coefs[v & 0x7f];
	if(!(v & 0x80))
		res = -res;
	return res;
}

static void sp0250_load_values(void)
{
	sp0250.filter[0].B = sp0250_gc(sp0250.fifo[ 0]);
	sp0250.filter[0].F = sp0250_gc(sp0250.fifo[ 1]);
	sp0250.amp         = sp0250_ga(sp0250.fifo[ 2]);
	sp0250.filter[1].B = sp0250_gc(sp0250.fifo[ 3]);
	sp0250.filter[1].F = sp0250_gc(sp0250.fifo[ 4]);
	sp0250.pitch       =           sp0250.fifo[ 5];
	sp0250.filter[2].B = sp0250_gc(sp0250.fifo[ 6]);
	sp0250.filter[2].F = sp0250_gc(sp0250.fifo[ 7]);
	sp0250.repeat      =           sp0250.fifo[ 8] & 0x3f;
	sp0250.voiced      =           sp0250.fifo[ 8] & 0x40;
	sp0250.filter[3].B = sp0250_gc(sp0250.fifo[ 9]);
	sp0250.filter[3].F = sp0250_gc(sp0250.fifo[10]);
	sp0250.filter[4].B = sp0250_gc(sp0250.fifo[11]);
	sp0250.filter[4].F = sp0250_gc(sp0250.fifo[12]);
	sp0250.filter[5].B = sp0250_gc(sp0250.fifo[13]);
	sp0250.filter[5].F = sp0250_gc(sp0250.fifo[14]);
	sp0250.fifo_pos = 0;
	sp0250.drq(ASSERT_LINE);
}

static void sp0250_timer_tick(int param)
{
	sp0250.pcount++;
	if(sp0250.pcount >= sp0250.pitch) {
		sp0250.pcount = 0;
		sp0250.rcount++;
		if(sp0250.rcount >= sp0250.repeat) {
			sp0250.rcount = 0;
			stream_update(sp0250.stream, 0);

			/* The synchronisation between the update callback and the*/
			/* timer tick is crap.  Specifically, the timer tick goes*/
			/* a little faster.  So compensate.*/

			sp0250.pcount = sp0250.pcounto;
			sp0250.rcount = sp0250.rcounto;
			if(sp0250.pcount || sp0250.rcount)
				return;

			if(sp0250.fifo_pos == 15)
				sp0250_load_values();
			else {
				sp0250.amp = 0;
				sp0250.pitch = 0;
				sp0250.repeat = 0;
			}
		}
	}
}

static void sp0250_update(int num, INT16 *output, int length)
{
	int i;
	for(i=0; i<length; i++) {
		INT16 z0 = 0;
		int f;

		if(sp0250.voiced)
			if(!sp0250.pcounto)
				z0 = sp0250.amp;
			else
				z0 = 0;
		else {
			/* Borrowing the ay noise generation LFSR*/

			if(sp0250.RNG & 1) {
				z0 = sp0250.amp;
				sp0250.RNG ^= 0x24000;
			} else
				z0 = -sp0250.amp;
			sp0250.RNG >>= 1;
		}
		for(f=0; f<6; f++) {
			z0 += ((sp0250.filter[f].z1 * sp0250.filter[f].F) >> 8)
				+ ((sp0250.filter[f].z2 * sp0250.filter[f].B) >> 9);
			sp0250.filter[f].z2 = sp0250.filter[f].z1;
			sp0250.filter[f].z1 = z0;
		}

		/* Physical resolution is only 7 bits, but heh*/
		output[i] = z0;

		sp0250.pcounto++;
		if(sp0250.pcounto >= sp0250.pitch) {
			sp0250.pcounto = 0;
			sp0250.rcounto++;
			if(sp0250.rcounto >= sp0250.repeat)
				sp0250.rcounto = 0;
		}
	}
}


int sp0250_sh_start( const struct MachineSound *msound )
{
	struct sp0250_interface *intf = msound->sound_interface;
	memset(&sp0250, 0, sizeof(sp0250));
	sp0250.RNG = 1;
	sp0250.drq = intf->drq_callback;
	sp0250.drq(ASSERT_LINE);
	timer_pulse(TIME_IN_HZ(MAIN_CLOCK), 0, sp0250_timer_tick);

	sp0250.stream = stream_init("SP0250", intf->volume, MAIN_CLOCK, 0, sp0250_update);

	return 0;
}

void sp0250_sh_stop( void )
{
}


WRITE_HANDLER( sp0250_w )
{
	if(sp0250.fifo_pos != 15) {
		sp0250.fifo[sp0250.fifo_pos++] = data;
		if(sp0250.fifo_pos == 15)
			sp0250.drq(CLEAR_LINE);
	}
}
