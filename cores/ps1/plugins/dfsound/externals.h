/***************************************************************************
                         externals.h  -  description
                             -------------------
    begin                : Wed May 15 2002
    copyright            : (C) 2002 by Pete Bernert
    email                : BlackDove@addcom.de
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version. See also the license.txt file for *
 *   additional informations.                                              *
 *                                                                         *
 ***************************************************************************/

#ifndef __P_SOUND_EXTERNALS_H__
#define __P_SOUND_EXTERNALS_H__

#include <stdint.h>

/////////////////////////////////////////////////////////
// generic defines
/////////////////////////////////////////////////////////

//#define log_unhandled printf
#define log_unhandled(...)

#ifdef __GNUC__
#define noinline __attribute__((noinline))
#define unlikely(x) __builtin_expect((x), 0)
#else
#define noinline
#define unlikely(x) x
#endif
#if defined(__GNUC__) && !defined(_TMS320C6X)
#define preload __builtin_prefetch
#else
#define preload(...)
#endif

#define PSE_LT_SPU                  4
#define PSE_SPU_ERR_SUCCESS         0
#define PSE_SPU_ERR                 -60
#define PSE_SPU_ERR_NOTCONFIGURED   PSE_SPU_ERR - 1
#define PSE_SPU_ERR_INIT            PSE_SPU_ERR - 2
#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

////////////////////////////////////////////////////////////////////////
// spu defines
////////////////////////////////////////////////////////////////////////

// num of channels
#define MAXCHAN     24

// note: must be even due to the way reverb works now
#define NSSIZE ((44100 / 50 + 32) & ~1)

///////////////////////////////////////////////////////////
// struct defines
///////////////////////////////////////////////////////////

enum ADSR_State {
 ADSR_ATTACK = 0,
 ADSR_DECAY = 1,
 ADSR_SUSTAIN = 2,
 ADSR_RELEASE = 3,
};

// ADSR INFOS PER CHANNEL
typedef struct
{
 unsigned char  State:2;                               // ADSR_State
 unsigned char  AttackModeExp:1;
 unsigned char  SustainModeExp:1;
 unsigned char  SustainIncrease:1;
 unsigned char  ReleaseModeExp:1;
 unsigned char  AttackRate;
 unsigned char  DecayRate;
 unsigned char  SustainLevel;
 unsigned char  SustainRate;
 unsigned char  ReleaseRate;
 int            EnvelopeVol;
} ADSRInfoEx;
              
///////////////////////////////////////////////////////////

// MAIN CHANNEL STRUCT
typedef struct
{
 int               iSBPos;                             // mixing stuff
 int               spos;
 int               sinc;
 int               sinc_inv;

 unsigned char *   pCurr;                              // current pos in sound mem
 unsigned char *   pLoop;                              // loop ptr in sound mem

 unsigned int      bReverb:1;                          // can we do reverb on this channel? must have ctrl register bit, to get active
 unsigned int      bRVBActive:1;                       // reverb active flag
 unsigned int      bNoise:1;                           // noise active flag
 unsigned int      bFMod:2;                            // freq mod (0=off, 1=sound channel, 2=freq channel)
 unsigned int      prevflags:3;                        // flags from previous block
 unsigned int      bIgnoreLoop:1;                      // Ignore loop
 unsigned int      bStarting:1;                        // starting after keyon
 union {
  struct {
   int             iLeftVolume;                        // left volume
   int             iRightVolume;                       // right volume
  };
  int              iVolume[2];
 };
 ADSRInfoEx        ADSRX;
 int               iRawPitch;                          // raw pitch (0...3fff)
} SPUCHAN;

///////////////////////////////////////////////////////////

typedef struct
{
 int StartAddr;      // reverb area start addr in samples
 int CurrAddr;       // reverb area curr addr in samples

 int VolLeft;
 int VolRight;

 // directly from nocash docs
 //int dAPF1; // 1DC0 disp    Reverb APF Offset 1
 //int dAPF2; // 1DC2 disp    Reverb APF Offset 2
 int vIIR;    // 1DC4 volume  Reverb Reflection Volume 1
 int vCOMB1;  // 1DC6 volume  Reverb Comb Volume 1
 int vCOMB2;  // 1DC8 volume  Reverb Comb Volume 2
 int vCOMB3;  // 1DCA volume  Reverb Comb Volume 3
 int vCOMB4;  // 1DCC volume  Reverb Comb Volume 4
 int vWALL;   // 1DCE volume  Reverb Reflection Volume 2
 int vAPF1;   // 1DD0 volume  Reverb APF Volume 1
 int vAPF2;   // 1DD2 volume  Reverb APF Volume 2
 int mLSAME;  // 1DD4 src/dst Reverb Same Side Reflection Address 1 Left
 int mRSAME;  // 1DD6 src/dst Reverb Same Side Reflection Address 1 Right
 int mLCOMB1; // 1DD8 src     Reverb Comb Address 1 Left
 int mRCOMB1; // 1DDA src     Reverb Comb Address 1 Right
 int mLCOMB2; // 1DDC src     Reverb Comb Address 2 Left
 int mRCOMB2; // 1DDE src     Reverb Comb Address 2 Right
 int dLSAME;  // 1DE0 src     Reverb Same Side Reflection Address 2 Left
 int dRSAME;  // 1DE2 src     Reverb Same Side Reflection Address 2 Right
 int mLDIFF;  // 1DE4 src/dst Reverb Different Side Reflect Address 1 Left
 int mRDIFF;  // 1DE6 src/dst Reverb Different Side Reflect Address 1 Right
 int mLCOMB3; // 1DE8 src     Reverb Comb Address 3 Left
 int mRCOMB3; // 1DEA src     Reverb Comb Address 3 Right
 int mLCOMB4; // 1DEC src     Reverb Comb Address 4 Left
 int mRCOMB4; // 1DEE src     Reverb Comb Address 4 Right
 int dLDIFF;  // 1DF0 src     Reverb Different Side Reflect Address 2 Left
 int dRDIFF;  // 1DF2 src     Reverb Different Side Reflect Address 2 Right
 int mLAPF1;  // 1DF4 src/dst Reverb APF Address 1 Left
 int mRAPF1;  // 1DF6 src/dst Reverb APF Address 1 Right
 int mLAPF2;  // 1DF8 src/dst Reverb APF Address 2 Left
 int mRAPF2;  // 1DFA src/dst Reverb APF Address 2 Right
 int vLIN;    // 1DFC volume  Reverb Input Volume Left
 int vRIN;    // 1DFE volume  Reverb Input Volume Right

 // subtracted offsets
 int mLAPF1_dAPF1, mRAPF1_dAPF1, mLAPF2_dAPF2, mRAPF2_dAPF2;

 int dirty;   // registers changed
} REVERBInfo;

///////////////////////////////////////////////////////////

// psx buffers / addresses

typedef union
{
 int SB[28 + 4 + 4];
 int SB_rvb[2][4*2]; // for reverb filtering
 struct {
  int sample[28];
  union {
   struct {
    int pos;
    int val[4];
   } gauss;
   int simple[5]; // 28-32
  } interp;
  int sinc_old;
 };
} sample_buf;

typedef struct
{
 unsigned short  spuCtrl;
 unsigned short  spuStat;

 unsigned int    spuAddr;

 unsigned int    cycles_played;
 unsigned int    cycles_dma_end;
 int             decode_pos;
 int             decode_dirty_ch;
 unsigned int    bSpuInit:1;
 unsigned int    bSPUIsOpen:1;
 unsigned int    bMemDirty:1;          // had external write to SPU RAM

 unsigned int    dwNoiseVal;           // global noise generator
 unsigned int    dwNoiseCount;
 unsigned int    dwNewChannel;         // flags for faster testing, if new channel starts
 unsigned int    dwChannelsAudible;    // not silent channels
 unsigned int    dwChannelDead;        // silent+not useful channels

 unsigned int    XARepeat;
 unsigned int    XALastVal;

 int             iLeftXAVol;
 int             iRightXAVol;

 int             cdClearSamples;       // extra samples to clear the capture buffers
 struct {                              // channel volume in the cd controller
  unsigned char  ll, lr, rl, rr;       // see cdr.Attenuator* in cdrom.c
 } cdv;                                // applied on spu side for easier emulation

 unsigned int    last_keyon_cycles;

 union {
  unsigned char  *spuMemC;
  unsigned short *spuMem;
 };
 unsigned char * pSpuIrq;

 unsigned char * pSpuBuffer;
 short         * pS;

 SPUCHAN       * s_chan;
 REVERBInfo    * rvb;

 int           * SSumLR;

 void (CALLBACK *irqCallback)(int);
 //void (CALLBACK *cddavCallback)(short, short);
 void (CALLBACK *scheduleCallback)(unsigned int);

 const xa_decode_t * xapGlobal;
 unsigned int  * XAFeed;
 unsigned int  * XAPlay;
 unsigned int  * XAStart;
 unsigned int  * XAEnd;

 unsigned int  * CDDAFeed;
 unsigned int  * CDDAPlay;
 unsigned int  * CDDAStart;
 unsigned int  * CDDAEnd;

 unsigned short  regArea[0x400];

 sample_buf      sb[MAXCHAN+1]; // last entry is used for reverb filter
 int             interpolation;

#if P_HAVE_PTHREAD || defined(WANT_THREAD_CODE)
 sample_buf    * sb_thread;
 sample_buf      sb_thread_[MAXCHAN+1];
#endif
} SPUInfo;

#define regAreaRef(offset) \
  spu.regArea[((offset) - 0xc00) >> 1]
#define regAreaGet(offset) \
  regAreaRef(offset)
#define regAreaGetCh(ch, offset) \
  spu.regArea[(((ch) << 4) | (offset)) >> 1]

///////////////////////////////////////////////////////////
// SPU.C globals
///////////////////////////////////////////////////////////

#ifndef _IN_SPU

extern SPUInfo spu;

void do_samples(unsigned int cycles_to, int force_no_thread);
void schedule_next_irq(void);
void check_irq_io(unsigned int addr);
void do_irq_io(int cycles_after);

#define do_samples_if_needed(c, no_thread, samples) \
 do { \
  if ((no_thread) || (int)((c) - spu.cycles_played) >= (samples) * 768) \
   do_samples(c, no_thread); \
 } while (0)

#endif

void FeedXA(const xa_decode_t *xap);
void FeedCDDA(unsigned char *pcm, int nBytes);

#endif /* __P_SOUND_EXTERNALS_H__ */
