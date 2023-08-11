/*****************************************************************************
 *
 *	POKEY chip emulator 4.3
 *	Copyright (c) 2000 by The MAME Team
 *
 *	Based on original info found in Ron Fries' Pokey emulator,
 *	with additions by Brad Oliver, Eric Smith and Juergen Buchmueller.
 *	paddle (a/d conversion) details from the Atari 400/800 Hardware Manual.
 *  Polynome algorithms according to info supplied by Perry McFarlane.
 *
 *	This code is subject to the MAME license, which besides other
 *	things means it is distributed as is, no warranties whatsoever.
 *	For more details read the readme.txt that comes with MAME.
 *
 *****************************************************************************/

#ifndef _POKEYSOUND_H
#define _POKEYSOUND_H

#include "driver.h"

/* CONSTANT DEFINITIONS */

/* POKEY WRITE LOGICALS */
#define AUDF1_C     0x00
#define AUDC1_C     0x01
#define AUDF2_C     0x02
#define AUDC2_C     0x03
#define AUDF3_C     0x04
#define AUDC3_C     0x05
#define AUDF4_C     0x06
#define AUDC4_C     0x07
#define AUDCTL_C    0x08
#define STIMER_C    0x09
#define SKREST_C    0x0A
#define POTGO_C     0x0B
#define SEROUT_C    0x0D
#define IRQEN_C     0x0E
#define SKCTL_C     0x0F

/* POKEY READ LOGICALS */
#define POT0_C      0x00
#define POT1_C      0x01
#define POT2_C      0x02
#define POT3_C      0x03
#define POT4_C      0x04
#define POT5_C      0x05
#define POT6_C      0x06
#define POT7_C      0x07
#define ALLPOT_C    0x08
#define KBCODE_C    0x09
#define RANDOM_C    0x0A
#define SERIN_C     0x0D
#define IRQST_C     0x0E
#define SKSTAT_C    0x0F

/* exact 1.79 MHz clock freq (of the Atari 800 that is) */
#define FREQ_17_EXACT   1789790

/*
 * We can now handle the exact frequency as well as any other,
 * because aliasing effects are suppressed for pure tones.
 */
#define FREQ_17_APPROX  FREQ_17_EXACT

#define MAXPOKEYS	4	/* max number of emulated chips */

/*****************************************************************************
 * pot0_r to pot7_r:
 *	Handlers for reading the pot values. Some Atari games use
 *	ALLPOT to return dipswitch settings and other things.
 * serin_r, serout_w, interrupt_cb:
 *	New function pointers for serial input/output and a interrupt callback.
 *****************************************************************************/

struct POKEYinterface {
	INT32 num;    /* total number of pokeys in the machine */
	INT32 addtostream; /* 0 = clear stream 1 = mix previous stream w/ pokey_update */
    INT32 baseclock;
    INT32 mixing_level[MAXPOKEYS];
    INT32 (*pot0_r[MAXPOKEYS])(INT32 offset);
    INT32 (*pot1_r[MAXPOKEYS])(INT32 offset);
    INT32 (*pot2_r[MAXPOKEYS])(INT32 offset);
    INT32 (*pot3_r[MAXPOKEYS])(INT32 offset);
    INT32 (*pot4_r[MAXPOKEYS])(INT32 offset);
    INT32 (*pot5_r[MAXPOKEYS])(INT32 offset);
    INT32 (*pot6_r[MAXPOKEYS])(INT32 offset);
    INT32 (*pot7_r[MAXPOKEYS])(INT32 offset);
    INT32 (*allpot_r[MAXPOKEYS])(INT32 offset);
    INT32 (*serin_r[MAXPOKEYS])(INT32 offset);
    void (*serout_w[MAXPOKEYS])(INT32 offset, INT32 data);
    void (*interrupt_cb[MAXPOKEYS])(INT32 mask);
};


INT32 PokeyInit(INT32 clock, INT32 num, double vol, INT32 addtostream);
void PokeyExit();
void PokeyReset();
void PokeySetRoute(INT32 chip, double chipvol, INT32 nRouteDir);

void PokeySetTotalCyclesCB(INT32 (*pCPUCyclesCB)()); // see pre90s/d_tempest.cpp

void pokey_update(INT32 num, INT16 *buffer, INT32 length);
void pokey_update(INT16 *buffer, INT32 length);

void pokey_scan(INT32 nAction, INT32* pnMin);

void PokeyPotCallback(INT32 chip, INT32 potnum, INT32 (*pot_cb)(INT32 offs));
void PokeyAllPotCallback(INT32 chip, INT32 (*pot_cb)(INT32 offs));

void pokey_write(INT32 chip, INT32 offset, UINT8 data);
UINT8 pokey_read(INT32 chip, INT32 offset);
void pokey_serin_ready(INT32 after);
void pokey_break_write(INT32 chip, INT32 shift);
void pokey_kbcode_write(INT32 chip, INT32 kbcode, INT32 make);

INT32 pokey1_r (INT32 offset);
INT32 pokey2_r (INT32 offset);
INT32 pokey3_r (INT32 offset);
INT32 pokey4_r (INT32 offset);
INT32 quad_pokey_r (INT32 offset);

void pokey1_w (INT32 offset,INT32 data);
void pokey2_w (INT32 offset,INT32 data);
void pokey3_w (INT32 offset,INT32 data);
void pokey4_w (INT32 offset,INT32 data);
void quad_pokey_w (INT32 offset,INT32 data);

void pokey1_serin_ready (INT32 after);
void pokey2_serin_ready (INT32 after);
void pokey3_serin_ready (INT32 after);
void pokey4_serin_ready (INT32 after);

void pokey1_break_w (INT32 shift);
void pokey2_break_w (INT32 shift);
void pokey3_break_w (INT32 shift);
void pokey4_break_w (INT32 shift);

void pokey1_kbcode_w (INT32 kbcode, INT32 make);
void pokey2_kbcode_w (INT32 kbcode, INT32 make);
void pokey3_kbcode_w (INT32 kbcode, INT32 make);
void pokey4_kbcode_w (INT32 kbcode, INT32 make);

#endif	/* POKEYSOUND_H */
