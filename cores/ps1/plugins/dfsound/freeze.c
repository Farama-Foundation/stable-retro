/***************************************************************************
                          freeze.c  -  description
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

#include <stddef.h>
#include <assert.h>
#include "stdafx.h"

#define _IN_FREEZE

#include "externals.h"
#include "registers.h"
#include "spu.h"

////////////////////////////////////////////////////////////////////////
// freeze structs
////////////////////////////////////////////////////////////////////////

typedef struct
{
 int            AttackModeExp;
 int            AttackTime;
 int            DecayTime;
 int            SustainLevel;
 int            SustainModeExp;
 int            SustainModeDec;
 int            SustainTime;
 int            ReleaseModeExp;
 unsigned int   ReleaseVal;
 int            ReleaseTime;
 int            ReleaseStartTime; 
 int            ReleaseVol; 
 int            lTime;
 int            lVolume;
} ADSRInfo;

typedef struct
{
 int            State;
 int            AttackModeExp;
 int            AttackRate;
 int            DecayRate;
 int            SustainLevel;
 int            SustainModeExp;
 int            SustainIncrease;
 int            SustainRate;
 int            ReleaseModeExp;
 int            ReleaseRate;
 int            EnvelopeVol;
 int            lVolume;
 int            lDummy1;
 int            lDummy2;
} ADSRInfoEx_orig;

typedef struct
{
 // no mutexes used anymore... don't need them to sync access
 //HANDLE            hMutex;

 int               bNew;                               // start flag

 int               iSBPos;                             // mixing stuff
 int               spos;
 int               sinc;
 int               SB[32+32];                          // Pete added another 32 dwords in 1.6 ... prevents overflow issues with gaussian/cubic interpolation (thanx xodnizel!), and can be used for even better interpolations, eh? :)
 int               sval;

 int               iStart;                             // start ptr into sound mem
 int               iCurr;                              // current pos in sound mem
 int               iLoop;                              // loop ptr in sound mem

 int               bOn;                                // is channel active (sample playing?)
 int               bStop;                              // is channel stopped (sample _can_ still be playing, ADSR Release phase)
 int               bReverb;                            // can we do reverb on this channel? must have ctrl register bit, to get active
 int               iActFreq;                           // current psx pitch
 int               iUsedFreq;                          // current pc pitch
 int               iLeftVolume;                        // left volume
 int               iLeftVolRaw;                        // left psx volume value
 int               bIgnoreLoop;                        // ignore loop bit, if an external loop address is used
 int               iMute;                              // mute mode
 int               iRightVolume;                       // right volume
 int               iRightVolRaw;                       // right psx volume value
 int               iRawPitch;                          // raw pitch (0...3fff)
 int               iIrqDone;                           // debug irq done flag
 int               s_1;                                // last decoding infos
 int               s_2;
 int               bRVBActive;                         // reverb active flag
 int               iRVBOffset;                         // reverb offset
 int               iRVBRepeat;                         // reverb repeat
 int               bNoise;                             // noise active flag
 int               bFMod;                              // freq mod (0=off, 1=sound channel, 2=freq channel)
 int               iRVBNum;                            // another reverb helper
 int               iOldNoise;                          // old noise val for this channel   
 ADSRInfo          ADSR;                               // active ADSR settings
 ADSRInfoEx_orig   ADSRX;                              // next ADSR settings (will be moved to active on sample start)
} SPUCHAN_orig;

typedef struct SPUFreeze
{
 char          szSPUName[8];
 uint32_t ulFreezeVersion;
 uint32_t ulFreezeSize;
 unsigned char cSPUPort[0x200];
 unsigned char cSPURam[0x80000];
 xa_decode_t   xaS;     
} SPUFreeze_t;

typedef struct
{
 unsigned short  spuIrq;
 unsigned short  decode_pos;
 uint32_t   pSpuIrq;
 uint32_t   spuAddr;
 uint32_t   rvb_cur;
 uint16_t   xa_left;
 uint16_t   cdda_left;
 uint32_t   cycles_played;

 SPUCHAN_orig s_chan[MAXCHAN];   

 uint32_t   cycles_dma_end;
 uint32_t   decode_dirty_ch;
 uint32_t   dwNoiseVal;
 uint32_t   dwNoiseCount;
 uint32_t   XARepeat;
 uint32_t   XALastVal;
 uint32_t   last_keyon_cycles;
 uint32_t   rvb_sb[2][4];
 int32_t    interpolation; // which interpolation's data is in SPUCHAN_orig::SB

} SPUOSSFreeze_t;

////////////////////////////////////////////////////////////////////////

static SPUOSSFreeze_t * LoadStateV5(SPUFreeze_t * pF, uint32_t cycles);
static void LoadStateUnknown(SPUFreeze_t * pF, uint32_t cycles); // unknown format

// we want to retain compatibility between versions,
// so use original channel struct
static void save_channel(SPUCHAN_orig *d, const SPUCHAN *s, int ch)
{
 memset(d, 0, sizeof(*d));
 d->bNew = !!(spu.dwNewChannel & (1<<ch));
 d->iSBPos = s->iSBPos;
 d->spos = s->spos;
 d->sinc = s->sinc;
 assert(sizeof(d->SB) >= sizeof(spu.sb[ch]));
 memcpy(d->SB, &spu.sb[ch], sizeof(spu.sb[ch]));
 d->iStart = (regAreaGetCh(ch, 6) & ~1) << 3;
 d->iCurr = 0; // set by the caller
 d->iLoop = 0; // set by the caller
 d->bOn = !!(spu.dwChannelsAudible & (1<<ch));
 d->bStop = s->ADSRX.State == ADSR_RELEASE;
 d->bReverb = s->bReverb;
 d->iActFreq = 1;
 d->iUsedFreq = 2;
 d->iLeftVolume = s->iLeftVolume;
 // this one is nasty but safe, save compat is important
 d->bIgnoreLoop = (s->prevflags ^ 2) << 1;
 d->iRightVolume = s->iRightVolume;
 d->iRawPitch = s->iRawPitch;
 d->s_1 = spu.sb[ch].SB[27]; // yes it's reversed
 d->s_2 = spu.sb[ch].SB[26];
 d->bRVBActive = s->bRVBActive;
 d->bNoise = s->bNoise;
 d->bFMod = s->bFMod;
 d->ADSRX.State = s->ADSRX.State;
 d->ADSRX.AttackModeExp = s->ADSRX.AttackModeExp;
 d->ADSRX.AttackRate = s->ADSRX.AttackRate;
 d->ADSRX.DecayRate = s->ADSRX.DecayRate;
 d->ADSRX.SustainLevel = s->ADSRX.SustainLevel;
 d->ADSRX.SustainModeExp = s->ADSRX.SustainModeExp;
 d->ADSRX.SustainIncrease = s->ADSRX.SustainIncrease;
 d->ADSRX.SustainRate = s->ADSRX.SustainRate;
 d->ADSRX.ReleaseModeExp = s->ADSRX.ReleaseModeExp;
 d->ADSRX.ReleaseRate = s->ADSRX.ReleaseRate;
 d->ADSRX.EnvelopeVol = s->ADSRX.EnvelopeVol;
 d->ADSRX.lVolume = d->bOn; // hmh
}

static void load_channel(SPUCHAN *d, const SPUCHAN_orig *s, int ch)
{
 memset(d, 0, sizeof(*d));
 if (s->bNew) spu.dwNewChannel |= 1<<ch;
 d->iSBPos = s->iSBPos;
 if ((uint32_t)d->iSBPos >= 28) d->iSBPos = 27;
 d->spos = s->spos;
 d->sinc = s->sinc;
 d->sinc_inv = 0;
 memcpy(&spu.sb[ch], s->SB, sizeof(spu.sb[ch]));
 d->pCurr = (void *)((uintptr_t)s->iCurr & 0x7fff0);
 d->pLoop = (void *)((uintptr_t)s->iLoop & 0x7fff0);
 d->bReverb = s->bReverb;
 d->iLeftVolume = s->iLeftVolume;
 d->iRightVolume = s->iRightVolume;
 d->iRawPitch = s->iRawPitch;
 d->bRVBActive = s->bRVBActive;
 d->bNoise = s->bNoise;
 d->bFMod = s->bFMod;
 d->prevflags = (s->bIgnoreLoop >> 1) ^ 2;
 d->ADSRX.State = s->ADSRX.State;
 if (s->bStop) d->ADSRX.State = ADSR_RELEASE;
 d->ADSRX.AttackModeExp = s->ADSRX.AttackModeExp;
 d->ADSRX.AttackRate = s->ADSRX.AttackRate;
 d->ADSRX.DecayRate = s->ADSRX.DecayRate;
 d->ADSRX.SustainLevel = s->ADSRX.SustainLevel;
 d->ADSRX.SustainModeExp = s->ADSRX.SustainModeExp;
 d->ADSRX.SustainIncrease = s->ADSRX.SustainIncrease;
 d->ADSRX.SustainRate = s->ADSRX.SustainRate;
 d->ADSRX.ReleaseModeExp = s->ADSRX.ReleaseModeExp;
 d->ADSRX.ReleaseRate = s->ADSRX.ReleaseRate;
 d->ADSRX.EnvelopeVol = s->ADSRX.EnvelopeVol;
 if (s->bOn) spu.dwChannelsAudible |= 1<<ch;
 else d->ADSRX.EnvelopeVol = 0;
}

// force load from regArea to variables
static void load_register(unsigned long reg, unsigned int cycles)
{
 unsigned short *r = &spu.regArea[((reg & 0xfff) - 0xc00) >> 1];
 *r ^= 1;
 SPUwriteRegister(reg, *r ^ 1, cycles);
}

////////////////////////////////////////////////////////////////////////
// SPUFREEZE: called by main emu on savestate load/save
////////////////////////////////////////////////////////////////////////

long DoFreeze(unsigned int ulFreezeMode, SPUFreeze_t * pF,
 unsigned int cycles)
{
 SPUOSSFreeze_t * pFO = NULL;
 sample_buf *sb_rvb = &spu.sb[MAXCHAN];
 int i, j;

 if(!pF) return 0;                                     // first check

#if P_HAVE_PTHREAD || defined(WANT_THREAD_CODE)
 sb_rvb = &spu.sb_thread[MAXCHAN];
#endif
 if(ulFreezeMode)                                      // info or save?
  {//--------------------------------------------------//
   int xa_left = 0, cdda_left = 0;
   do_samples(cycles, 1);

   if(ulFreezeMode==1)                                 
    memset(pF,0,sizeof(SPUFreeze_t)+sizeof(SPUOSSFreeze_t));

   strcpy(pF->szSPUName,"PBOSS");
   pF->ulFreezeVersion=5;
   pF->ulFreezeSize=sizeof(SPUFreeze_t)+sizeof(SPUOSSFreeze_t);

   if(ulFreezeMode==2) return 1;                       // info mode? ok, bye
                                                       // save mode:
   regAreaGet(H_SPUctrl) = spu.spuCtrl;
   regAreaGet(H_SPUstat) = spu.spuStat;
   memcpy(pF->cSPURam,spu.spuMem,0x80000);             // copy common infos
   memcpy(pF->cSPUPort,spu.regArea,0x200);

   if(spu.xapGlobal && spu.XAPlay!=spu.XAFeed)         // some xa
    {
     xa_left = spu.XAFeed - spu.XAPlay;
     if (xa_left < 0)
      xa_left = spu.XAEnd - spu.XAPlay + spu.XAFeed - spu.XAStart;
     pF->xaS = *spu.xapGlobal;
    }
   else if (spu.CDDAPlay != spu.CDDAFeed)
    {
     // abuse the xa struct to store leftover cdda samples
     unsigned int *p = spu.CDDAPlay;
     cdda_left = spu.CDDAFeed - spu.CDDAPlay;
     if (cdda_left < 0)
      cdda_left = spu.CDDAEnd - spu.CDDAPlay + spu.CDDAFeed - spu.CDDAStart;
     if (cdda_left > sizeof(pF->xaS.pcm) / 4)
      cdda_left = sizeof(pF->xaS.pcm) / 4;
     if (p + cdda_left <= spu.CDDAEnd)
      memcpy(pF->xaS.pcm, p, cdda_left * 4);
     else {
      memcpy(pF->xaS.pcm, p, (spu.CDDAEnd - p) * 4);
      memcpy((char *)pF->xaS.pcm + (spu.CDDAEnd - p) * 4, spu.CDDAStart,
             (cdda_left - (spu.CDDAEnd - p)) * 4);
     }
     pF->xaS.nsamples = 0;
    }
   else
    memset(&pF->xaS,0,sizeof(xa_decode_t));            // or clean xa

   pFO=(SPUOSSFreeze_t *)(pF+1);                       // store special stuff

   pFO->spuIrq = spu.regArea[(H_SPUirqAddr - 0x0c00) / 2];
   if(spu.pSpuIrq) pFO->pSpuIrq = spu.pSpuIrq - spu.spuMemC;

   pFO->spuAddr=spu.spuAddr;
   if(pFO->spuAddr==0) pFO->spuAddr=0xbaadf00d;
   pFO->decode_pos = spu.decode_pos;
   pFO->rvb_cur = spu.rvb->CurrAddr;
   pFO->xa_left = xa_left;
   pFO->cdda_left = cdda_left;
   pFO->cycles_played = spu.cycles_played;
   pFO->cycles_dma_end = spu.cycles_dma_end;
   pFO->decode_dirty_ch = spu.decode_dirty_ch;
   pFO->dwNoiseVal = spu.dwNoiseVal;
   pFO->dwNoiseCount = spu.dwNoiseCount;
   pFO->XARepeat = spu.XARepeat;
   pFO->XALastVal = spu.XALastVal;
   pFO->last_keyon_cycles = spu.last_keyon_cycles;
   for (i = 0; i < 2; i++)
    memcpy(&pFO->rvb_sb[i], sb_rvb->SB_rvb[i], sizeof(pFO->rvb_sb[i]));
   pFO->interpolation = spu.interpolation;

   for(i=0;i<MAXCHAN;i++)
    {
     save_channel(&pFO->s_chan[i],&spu.s_chan[i],i);
     if(spu.s_chan[i].pCurr)
      pFO->s_chan[i].iCurr=spu.s_chan[i].pCurr-spu.spuMemC;
     if(spu.s_chan[i].pLoop)
      pFO->s_chan[i].iLoop=spu.s_chan[i].pLoop-spu.spuMemC;
    }

   return 1;
   //--------------------------------------------------//
  }
                                                       
 if(ulFreezeMode!=0) return 0;                         // bad mode? bye

 memcpy(spu.spuMem,pF->cSPURam,0x80000);               // get ram
 memcpy(spu.regArea,pF->cSPUPort,0x200);
 spu.bMemDirty = 1;
 spu.spuCtrl = regAreaGet(H_SPUctrl);
 spu.spuStat = regAreaGet(H_SPUstat);

 if (!strcmp(pF->szSPUName,"PBOSS") && pF->ulFreezeVersion==5)
   pFO = LoadStateV5(pF, cycles);
 else LoadStateUnknown(pF, cycles);

 spu.XAPlay = spu.XAFeed = spu.XAStart;
 spu.CDDAPlay = spu.CDDAFeed = spu.CDDAStart;
 spu.cdClearSamples = 512;
 if (pFO && pFO->xa_left && pF->xaS.nsamples) {        // start xa again
  FeedXA(&pF->xaS);
  spu.XAPlay = spu.XAFeed - pFO->xa_left;
  if (spu.XAPlay < spu.XAStart)
   spu.XAPlay = spu.XAStart;
 }
 else if (pFO && pFO->cdda_left) {                     // start cdda again
  FeedCDDA((void *)pF->xaS.pcm, pFO->cdda_left * 4);
 }

 // not in old savestates
 spu.cycles_dma_end = 0;
 spu.decode_dirty_ch = spu.dwChannelsAudible & 0x0a;
 spu.dwNoiseVal = 0;
 spu.dwNoiseCount = 0;
 spu.XARepeat = 0;
 spu.XALastVal = 0;
 spu.last_keyon_cycles = cycles - 16*786u;
 spu.interpolation = -1;
 if (pFO && pF->ulFreezeSize >= sizeof(*pF) + offsetof(SPUOSSFreeze_t, rvb_sb)) {
  spu.cycles_dma_end = pFO->cycles_dma_end;
  spu.decode_dirty_ch = pFO->decode_dirty_ch;
  spu.dwNoiseVal = pFO->dwNoiseVal;
  spu.dwNoiseCount = pFO->dwNoiseCount;
  spu.XARepeat = pFO->XARepeat;
  spu.XALastVal = pFO->XALastVal;
  spu.last_keyon_cycles = pFO->last_keyon_cycles;
 }
 if (pFO && pF->ulFreezeSize >= sizeof(*pF) + sizeof(*pFO)) {
  for (i = 0; i < 2; i++)
   for (j = 0; j < 2; j++)
    memcpy(&sb_rvb->SB_rvb[i][j*4], pFO->rvb_sb[i], 4 * sizeof(sb_rvb->SB_rvb[i][0]));
  spu.interpolation = pFO->interpolation;
 }
 for (i = 0; i <= 2; i += 2)
  if (!regAreaGet(H_SPUcmvolL+i) && regAreaGet(H_SPUmvolL+i) < 0x8000u)
   regAreaRef(H_SPUcmvolL+i) = regAreaGet(H_SPUmvolL+i) << 1;

 // repair some globals
 for(i=0;i<=62;i+=2)
  load_register(H_Reverb+i, cycles);
 load_register(H_SPUReverbAddr, cycles);
 load_register(H_SPUrvolL, cycles);
 load_register(H_SPUrvolR, cycles);
 load_register(H_CDLeft, cycles);
 load_register(H_CDRight, cycles);

 // reverb
 spu.rvb->StartAddr = regAreaGet(H_SPUReverbAddr) << 2;
 if (spu.rvb->CurrAddr < spu.rvb->StartAddr)
  spu.rvb->CurrAddr = spu.rvb->StartAddr;

 ClearWorkingState();

 if (spu.spuCtrl & CTRL_IRQ)
  schedule_next_irq();

 return 1;
}

////////////////////////////////////////////////////////////////////////

static SPUOSSFreeze_t * LoadStateV5(SPUFreeze_t * pF, uint32_t cycles)
{
 int i;SPUOSSFreeze_t * pFO;

 pFO=(SPUOSSFreeze_t *)(pF+1);

 spu.pSpuIrq = spu.spuMemC + ((spu.regArea[(H_SPUirqAddr - 0x0c00) / 2] << 3) & ~0xf);

 if(pFO->spuAddr)
  {
   if (pFO->spuAddr == 0xbaadf00d) spu.spuAddr = 0;
   else spu.spuAddr = pFO->spuAddr & 0x7fffe;
  }
 spu.decode_pos = pFO->decode_pos & 0x1ff;
 spu.rvb->CurrAddr = pFO->rvb_cur;
 spu.cycles_played = pFO->cycles_played ? pFO->cycles_played : cycles;

 spu.dwNewChannel=0;
 spu.dwChannelsAudible=0;
 spu.dwChannelDead=0;
 for(i=0;i<MAXCHAN;i++)
  {
   load_channel(&spu.s_chan[i],&pFO->s_chan[i],i);

   spu.s_chan[i].pCurr+=(uintptr_t)spu.spuMemC;
   spu.s_chan[i].pLoop+=(uintptr_t)spu.spuMemC;
  }

 return pFO;
}

////////////////////////////////////////////////////////////////////////

static void LoadStateUnknown(SPUFreeze_t * pF, uint32_t cycles)
{
 int i;

 for(i=0;i<MAXCHAN;i++)
  {
   spu.s_chan[i].pLoop=spu.spuMemC;
  }

 spu.dwNewChannel=0;
 spu.dwChannelsAudible=0;
 spu.dwChannelDead=0;
 spu.pSpuIrq=spu.spuMemC;
 spu.cycles_played = cycles;

 for(i=0;i<0xc0;i++)
  {
   load_register(0x1f801c00 + i*2, cycles);
  }
}

////////////////////////////////////////////////////////////////////////
// vim:shiftwidth=1:expandtab
