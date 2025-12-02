/***************************************************************************
                          fps.c  -  description
                             -------------------
    begin                : Sun Mar 08 2009
    copyright            : (C) 1999-2009 by Pete Bernert
    web                  : www.pbernert.com   
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

//*************************************************************************// 
// History of changes:
//
// 2009/03/08 - Pete  
// - generic cleanup for the Peops release
//
//*************************************************************************// 

#ifdef _WINDOWS
#include "Stdafx.h"
#include "Externals.h"
#include "plugin.h"
#include "Fps.h"
#include "Prim.h"
#else
#include "gpuStdafx.h"
#include "gpuExternals.h"
//#include "plugins.h"
#include "gpuFps.h"
#include "gpuPrim.h"
#endif

#define _IN_FPS

#define CALLBACK

////////////////////////////////////////////////////////////////////////
// FPS stuff
////////////////////////////////////////////////////////////////////////
#ifdef _WINDOWS
LARGE_INTEGER  liCPUFrequency;
#endif

////////////////////////////////////////////////////////////////////////
// FPS skipping / limit
////////////////////////////////////////////////////////////////////////

BOOL           bIsPerformanceCounter;
float          fFrameRateHz;
DWORD          dwFrameRateTicks;
float          fFrameRate;
int            iFrameLimit;
BOOL           bUseFrameLimit;
BOOL           bUseFrameSkip;
DWORD          dwLaceCnt;

BOOL         bInitCap; 
float        fps_skip;
float        fps_cur;

#ifdef _WINDOWS

void FrameCap (void)
{
 static DWORD curticks, lastticks, _ticks_since_last_update;
 static DWORD TicksToWait = 0;
 static LARGE_INTEGER  CurrentTime;
 static LARGE_INTEGER  LastTime;
 static BOOL SkipNextWait = FALSE;
 BOOL Waiting = TRUE;

 //---------------------------------------------------------
 // init some static vars... 
 // bInitCap is TRUE on startup and everytime the user
 // is toggling the frame limit
 //---------------------------------------------------------

 if(bInitCap)
  {
   bInitCap=FALSE;
   if (bIsPerformanceCounter)
    QueryPerformanceCounter(&LastTime);
   lastticks = timeGetTime();
   TicksToWait=0;
   return;
  }

 //---------------------------------------------------------

 if(bIsPerformanceCounter)
  {
   QueryPerformanceCounter(&CurrentTime);
   _ticks_since_last_update = CurrentTime.LowPart - LastTime.LowPart;

   //---------------------------------------------------------
   // check if diff > 1/2 sec, if yes: take mm timer value
   //---------------------------------------------------------

   curticks = timeGetTime();
   if(_ticks_since_last_update>(liCPUFrequency.LowPart>>1))   
    {
     if(curticks < lastticks)
          _ticks_since_last_update = dwFrameRateTicks+TicksToWait+1;
     else _ticks_since_last_update = (liCPUFrequency.LowPart * (curticks - lastticks))/1000;
    }

   //---------------------------------------------------------

   if ((_ticks_since_last_update > TicksToWait) || 
       (CurrentTime.LowPart < LastTime.LowPart))
    {
     LastTime.HighPart = CurrentTime.HighPart;
     LastTime.LowPart  = CurrentTime.LowPart;

     lastticks=curticks;

     if((_ticks_since_last_update-TicksToWait) > dwFrameRateTicks)
          TicksToWait=0;
     else TicksToWait=dwFrameRateTicks-(_ticks_since_last_update-TicksToWait);
    }
   else
    {
     while (Waiting)
      {
       QueryPerformanceCounter(&CurrentTime);
       _ticks_since_last_update = CurrentTime.LowPart - LastTime.LowPart;

       //---------------------------------------------------------
       // check if diff > 1/2 sec, if yes: take mm timer value
       //---------------------------------------------------------
       curticks = timeGetTime();
       if(_ticks_since_last_update>(liCPUFrequency.LowPart>>1))   
        {
         if(curticks < lastticks)
              _ticks_since_last_update = TicksToWait+1;
         else _ticks_since_last_update = (liCPUFrequency.LowPart * (curticks - lastticks))/1000;
        }
       //---------------------------------------------------------

       if ((_ticks_since_last_update > TicksToWait) || 
           (CurrentTime.LowPart < LastTime.LowPart))
        {
         Waiting = FALSE;

         lastticks=curticks;

         LastTime.HighPart = CurrentTime.HighPart;
         LastTime.LowPart = CurrentTime.LowPart;
         TicksToWait = dwFrameRateTicks;
        }
      }
    }
  }
 else
  {
   curticks = timeGetTime();
   _ticks_since_last_update = curticks - lastticks;

   if ((_ticks_since_last_update > TicksToWait) || 
       (curticks < lastticks))
    {
     lastticks = curticks;

     if((_ticks_since_last_update-TicksToWait) > dwFrameRateTicks)
          TicksToWait=0;
     else TicksToWait=dwFrameRateTicks-(_ticks_since_last_update-TicksToWait);
    }
   else
    {
     while (Waiting)
      {
       curticks = timeGetTime();
       _ticks_since_last_update = curticks - lastticks;
       if ((_ticks_since_last_update > TicksToWait) ||
           (curticks < lastticks))
        {
         Waiting = FALSE;
         lastticks = curticks;
         TicksToWait = dwFrameRateTicks;
        }
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////

#define MAXSKIP 120
#define MAXLACE 16

void FrameSkip(void)
{
 static int   iNumSkips=0,iAdditionalSkip=0;           // number of additional frames to skip
 static DWORD dwLastLace=0;                            // helper var for frame limitation
 static DWORD curticks, lastticks, _ticks_since_last_update;
 static LARGE_INTEGER  CurrentTime;
 static LARGE_INTEGER  LastTime;

 if(!dwLaceCnt) return;                                // important: if no updatelace happened, we ignore it completely

 if(iNumSkips)                                         // we are in skipping mode?
  {
   dwLastLace+=dwLaceCnt;                              // -> calc frame limit helper (number of laces)
   bSkipNextFrame = TRUE;                              // -> we skip next frame
   iNumSkips--;                                        // -> ok, one done
  }
 else                                                  // ok, no additional skipping has to be done... 
  {                                                    // we check now, if some limitation is needed, or a new skipping has to get started
   DWORD dwWaitTime;

   if(bInitCap || bSkipNextFrame)                      // first time or we skipped before?
    {
     if(bUseFrameLimit && !bInitCap)                   // frame limit wanted and not first time called?
      {
       DWORD dwT=_ticks_since_last_update;             // -> that's the time of the last drawn frame
       dwLastLace+=dwLaceCnt;                          // -> and that's the number of updatelace since the start of the last drawn frame

       if(bIsPerformanceCounter)                       // -> now we calc the time of the last drawn frame + the time we spent skipping
        {
         QueryPerformanceCounter(&CurrentTime);
         _ticks_since_last_update= dwT+CurrentTime.LowPart - LastTime.LowPart;
        }
       else
        {
         curticks = timeGetTime();
         _ticks_since_last_update= dwT+curticks - lastticks;
        }

       dwWaitTime=dwLastLace*dwFrameRateTicks;         // -> and now we calc the time the real psx would have needed

       if(_ticks_since_last_update<dwWaitTime)         // -> we were too fast?
        {                                    
         if((dwWaitTime-_ticks_since_last_update)>     // -> some more security, to prevent
            (60*dwFrameRateTicks))                     //    wrong waiting times
          _ticks_since_last_update=dwWaitTime;

         while(_ticks_since_last_update<dwWaitTime)    // -> loop until we have reached the real psx time
          {                                            //    (that's the additional limitation, yup)
           if(bIsPerformanceCounter)
            {
             QueryPerformanceCounter(&CurrentTime);
             _ticks_since_last_update = dwT+CurrentTime.LowPart - LastTime.LowPart;
            }
           else
            {
             curticks = timeGetTime();
             _ticks_since_last_update = dwT+curticks - lastticks;
            }
          }
        }
       else                                            // we were still too slow ?!!?
        {
         if(iAdditionalSkip<MAXSKIP)                   // -> well, somewhen we really have to stop skipping on very slow systems
          {
           iAdditionalSkip++;                          // -> inc our watchdog var
           dwLaceCnt=0;                                // -> reset lace count
           if(bIsPerformanceCounter)                   // -> ok, start time of the next frame
            QueryPerformanceCounter(&LastTime);
           lastticks = timeGetTime();
           return;                                     // -> done, we will skip next frame to get more speed
          } 
        }
      }

     bInitCap=FALSE;                                   // -> ok, we have inited the frameskip func
     iAdditionalSkip=0;                                // -> init additional skip
     bSkipNextFrame=FALSE;                              // -> we don't skip the next frame
     if(bIsPerformanceCounter)                         // -> we store the start time of the next frame
      QueryPerformanceCounter(&LastTime);
     lastticks = timeGetTime();
     dwLaceCnt=0;                                      // -> and we start to count the laces 
     dwLastLace=0;      
     _ticks_since_last_update=0;
     return;                                           // -> done, the next frame will get drawn
    }

   bSkipNextFrame=FALSE;                                // init the frame skip signal to 'no skipping' first

   if(bIsPerformanceCounter)                           // get the current time (we are now at the end of one drawn frame)
    {
     QueryPerformanceCounter(&CurrentTime);
     _ticks_since_last_update = CurrentTime.LowPart - LastTime.LowPart;
    }
   else
    {
     curticks = timeGetTime();
     _ticks_since_last_update = curticks - lastticks;
    }

   dwLastLace=dwLaceCnt;                               // store curr count (frame limitation helper)
   dwWaitTime=dwLaceCnt*dwFrameRateTicks;              // calc the 'real psx lace time'

   if(_ticks_since_last_update>dwWaitTime)             // hey, we needed way too long for that frame...
    {
     if(bUseFrameLimit)                                // if limitation, we skip just next frame,
      {                                                // and decide after, if we need to do more
       iNumSkips=0;
      }
     else
      {
       iNumSkips=_ticks_since_last_update/dwWaitTime;  // -> calc number of frames to skip to catch up
       iNumSkips--;                                    // -> since we already skip next frame, one down
       if(iNumSkips>MAXSKIP) iNumSkips=MAXSKIP;        // -> well, somewhere we have to draw a line
      }
     bSkipNextFrame = TRUE;                            // -> signal for skipping the next frame
    }
   else                                                // we were faster than real psx? fine :)
   if(bUseFrameLimit)                                  // frame limit used? so we wait til the 'real psx time' has been reached
    {
     if(dwLaceCnt>MAXLACE)                             // -> security check
      _ticks_since_last_update=dwWaitTime;

     while(_ticks_since_last_update<dwWaitTime)        // just do a waiting loop...
      {
       if(bIsPerformanceCounter)
        {
         QueryPerformanceCounter(&CurrentTime);
         _ticks_since_last_update = CurrentTime.LowPart - LastTime.LowPart;
        }
       else
        {
         curticks = timeGetTime();
         _ticks_since_last_update = curticks - lastticks;
        }
      }
    }

   if(bIsPerformanceCounter)                           // ok, start time of the next frame
    QueryPerformanceCounter(&LastTime);
   lastticks = timeGetTime();
  }

 dwLaceCnt=0;                                          // init lace counter
}

////////////////////////////////////////////////////////////////////////

void calcfps(void)
{
 static DWORD curticks,_ticks_since_last_update,lastticks;
 static long   fps_cnt = 0;
 static DWORD  fps_tck = 1;
 static LARGE_INTEGER  CurrentTime;
 static LARGE_INTEGER  LastTime;
 static long   fpsskip_cnt = 0;
 static DWORD  fpsskip_tck = 1;

 if(bIsPerformanceCounter)
  {
   QueryPerformanceCounter(&CurrentTime);
   _ticks_since_last_update=CurrentTime.LowPart-LastTime.LowPart;
                                                        
   //--------------------------------------------------//
   curticks = timeGetTime();
   if(_ticks_since_last_update>(liCPUFrequency.LowPart>>1))
    _ticks_since_last_update = (liCPUFrequency.LowPart * (curticks - lastticks))/1000;
   lastticks=curticks;
   //--------------------------------------------------//

   if(bUseFrameSkip && !bUseFrameLimit && _ticks_since_last_update)                                    
    fps_skip=min(fps_skip,(((float)liCPUFrequency.LowPart) / ((float)_ticks_since_last_update) +1.0f));

   LastTime.HighPart = CurrentTime.HighPart;
   LastTime.LowPart = CurrentTime.LowPart;
  }
 else
  {
   curticks = timeGetTime();
   _ticks_since_last_update=curticks-lastticks;

   if(bUseFrameSkip && !bUseFrameLimit && _ticks_since_last_update)
    fps_skip=min(fps_skip,((float)1000/(float)_ticks_since_last_update+1.0f));

   lastticks = curticks;
  }

 if(bUseFrameSkip && bUseFrameLimit)
  {
   fpsskip_tck += _ticks_since_last_update;

   if(++fpsskip_cnt==2)
    {
     if(bIsPerformanceCounter)
      fps_skip = ((float)liCPUFrequency.LowPart) / ((float)fpsskip_tck) *2.0f;
     else
      fps_skip = (float)2000/(float)fpsskip_tck;

     fps_skip +=6.0f;

     fpsskip_cnt = 0;
     fpsskip_tck = 1;
    }
  }

 fps_tck += _ticks_since_last_update;

 if(++fps_cnt==10)
  {
   if(bIsPerformanceCounter)
    fps_cur = ((float)liCPUFrequency.LowPart) / ((float)fps_tck) *10.0f;
   else
    fps_cur = (float)10000/(float)fps_tck;

   fps_cnt = 0;
   fps_tck = 1;

   if(bUseFrameLimit && fps_cur>fFrameRateHz)            // optical adjust ;) avoids flickering fps display
    fps_cur=fFrameRateHz;
  }
}

////////////////////////////////////////////////////////////////////////
// PC FPS skipping / limit
////////////////////////////////////////////////////////////////////////

void PCFrameCap(void)
{
 static DWORD curticks, lastticks, _ticks_since_last_update;
 static DWORD TicksToWait = 0;
 static LARGE_INTEGER  CurrentTime;
 static LARGE_INTEGER  LastTime;
 BOOL Waiting = TRUE;

 while (Waiting)
  {
   if(bIsPerformanceCounter)
    {
     QueryPerformanceCounter(&CurrentTime);
     _ticks_since_last_update = CurrentTime.LowPart - LastTime.LowPart;

     //------------------------------------------------//
     curticks = timeGetTime();
     if(_ticks_since_last_update>(liCPUFrequency.LowPart>>1))   
      {
       if(curticks < lastticks)
            _ticks_since_last_update = TicksToWait+1;
       else _ticks_since_last_update = (liCPUFrequency.LowPart * (curticks - lastticks))/1000;
      }
     //------------------------------------------------//

     if ((_ticks_since_last_update > TicksToWait) ||
         (CurrentTime.LowPart < LastTime.LowPart))
      {
       Waiting = FALSE;

       lastticks=curticks; 

       LastTime.HighPart = CurrentTime.HighPart;
       LastTime.LowPart = CurrentTime.LowPart;
       TicksToWait = (liCPUFrequency.LowPart / fFrameRateHz);
      }
    }
   else
    {
     curticks = timeGetTime();
     _ticks_since_last_update = curticks - lastticks;
     if ((_ticks_since_last_update > TicksToWait) || 
         (curticks < lastticks))
      {
       Waiting = FALSE;
       lastticks = curticks;
       TicksToWait = (1000 / (DWORD)fFrameRateHz);
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////

void PCcalcfps(void)
{
 static DWORD curticks,_ticks_since_last_update,lastticks;
 static long  fps_cnt = 0;
 static float fps_acc = 0;
 static LARGE_INTEGER  CurrentTime;
 static LARGE_INTEGER  LastTime;
 float CurrentFPS=0;    
 
 if(bIsPerformanceCounter)
  {
   QueryPerformanceCounter(&CurrentTime);
   _ticks_since_last_update=CurrentTime.LowPart-LastTime.LowPart;

   //--------------------------------------------------//
   curticks = timeGetTime();
   if(_ticks_since_last_update>(liCPUFrequency.LowPart>>1))   
    _ticks_since_last_update = (liCPUFrequency.LowPart * (curticks - lastticks))/1000;
   lastticks=curticks;
   //--------------------------------------------------//

   if(_ticks_since_last_update)
    {
     CurrentFPS = ((float)liCPUFrequency.LowPart) / ((float)_ticks_since_last_update);
    }
   else CurrentFPS = 0;
   LastTime.HighPart = CurrentTime.HighPart;
   LastTime.LowPart = CurrentTime.LowPart;
  }
 else
  {
   curticks = timeGetTime();
   if(_ticks_since_last_update=curticks-lastticks)
        CurrentFPS=(float)1000/(float)_ticks_since_last_update;
   else CurrentFPS = 0;
   lastticks = curticks;
  }

 fps_acc += CurrentFPS;

 if(++fps_cnt==10)
  {
   fps_cur = fps_acc / 10;
   fps_acc = 0;
   fps_cnt = 0;
  }
 
 fps_skip=CurrentFPS+1.0f;
}

////////////////////////////////////////////////////////////////////////

void SetAutoFrameCap(void)
{
 if(iFrameLimit==1)
  {
   fFrameRateHz = fFrameRate;
   if(bIsPerformanceCounter)
        dwFrameRateTicks=(liCPUFrequency.LowPart / fFrameRateHz);
   else dwFrameRateTicks=(1000 / (DWORD)fFrameRateHz);
   return;
  }

 if(dwActFixes&128)
  {
   if (PSXDisplay.Interlaced)
        fFrameRateHz = PSXDisplay.PAL?50.0f:60.0f;
   else fFrameRateHz = PSXDisplay.PAL?25.0f:30.0f;
  }
 else
  {
   //fFrameRateHz = PSXDisplay.PAL?50.0f:59.94f;

   if(PSXDisplay.PAL)
    {
     if (STATUSREG&GPUSTATUS_INTERLACED)
           fFrameRateHz=33868800.0f/677343.75f;        // 50.00238
      else fFrameRateHz=33868800.0f/680595.00f;        // 49.76351
    }                                                     
   else
    {
     if (STATUSREG&GPUSTATUS_INTERLACED)
           fFrameRateHz=33868800.0f/565031.25f;        // 59.94146
      else fFrameRateHz=33868800.0f/566107.50f;        // 59.82750
    }

   if(bIsPerformanceCounter)
        dwFrameRateTicks=(liCPUFrequency.LowPart / fFrameRateHz);
   else dwFrameRateTicks=(1000 / (DWORD)fFrameRateHz);
  }
}

////////////////////////////////////////////////////////////////////////

void InitFrameCap(void)                                // inits cpu frequency info (on gpu startup)
{
 if (QueryPerformanceFrequency (&liCPUFrequency))
      bIsPerformanceCounter = TRUE;
 else bIsPerformanceCounter = FALSE;
}

////////////////////////////////////////////////////////////////////////

void ReInitFrameCap(void)
{
 BOOL bOldPerformanceCounter=bIsPerformanceCounter;    // store curr timer mode

 if(dwActFixes&0x10000)                                // check game fix... high performance counters are bad on some mb chipsets
      bIsPerformanceCounter=FALSE;
 else 
  {
   if (QueryPerformanceFrequency (&liCPUFrequency))
        bIsPerformanceCounter = TRUE;
   else bIsPerformanceCounter = FALSE;
  }
           
 if(bOldPerformanceCounter!=bIsPerformanceCounter)     // changed?
  {
   bInitCap = TRUE;
   SetAutoFrameCap();
  }
}

////////////////////////////////////////////////////////////////////////

void SetFrameRateConfig(void)
{
 if(fFrameRateHz==0) 
  {
   if(iFrameLimit==2) fFrameRateHz=59.94f;           // auto framerate? set some init val (no pal/ntsc known yet)
   else               fFrameRateHz=fFrameRate;       // else set user framerate
  }

 if(bIsPerformanceCounter)
      dwFrameRateTicks=(liCPUFrequency.LowPart / fFrameRateHz);
 else dwFrameRateTicks=(1000 / (DWORD)fFrameRateHz);
}

////////////////////////////////////////////////////////////////////////

// LINUX ---------------------------------------------

#else

#define TIMEBASE 100000

// hehehe... using same func name as with win32 ;) wow, are we genius ;)
unsigned long timeGetTime()
{
 struct timeval tv;
 gettimeofday(&tv, 0);                                // well, maybe there are better ways
 return tv.tv_sec * 100000 + tv.tv_usec/10;           // to do that in linux, but at least it works
}

void FrameCap(void)
{
 static unsigned long curticks, lastticks, _ticks_since_last_update;
 static unsigned long TicksToWait = 0;
 bool Waiting = TRUE;

  {
   curticks = timeGetTime();
   _ticks_since_last_update = curticks - lastticks;

    if((_ticks_since_last_update > TicksToWait) ||
       (curticks <lastticks))
    {
     lastticks = curticks;

     if((_ticks_since_last_update-TicksToWait) > dwFrameRateTicks)
          TicksToWait=0;
     else TicksToWait=dwFrameRateTicks-(_ticks_since_last_update-TicksToWait);
    }
   else
    {
     while (Waiting) 
      {
       curticks = timeGetTime(); 
       _ticks_since_last_update = curticks - lastticks; 
       if ((_ticks_since_last_update > TicksToWait) ||
           (curticks < lastticks)) 
        { 
         Waiting = FALSE;
         lastticks = curticks;
         TicksToWait = dwFrameRateTicks; 
        } 
      } 
    } 
  } 
} 
 
////////////////////////////////////////////////////////////////////////

#define MAXSKIP 120
#define MAXLACE 16

void FrameSkip(void)
{
 static int   iNumSkips=0,iAdditionalSkip=0;           // number of additional frames to skip
 static DWORD dwLastLace=0;                            // helper var for frame limitation
 static DWORD curticks, lastticks, _ticks_since_last_update;

 if(!dwLaceCnt) return;                                // important: if no updatelace happened, we ignore it completely

 if(iNumSkips)                                         // we are in skipping mode?
  {
   dwLastLace+=dwLaceCnt;                              // -> calc frame limit helper (number of laces)
   bSkipNextFrame = TRUE;                              // -> we skip next frame
   iNumSkips--;                                        // -> ok, one done
  }
 else                                                  // ok, no additional skipping has to be done... 
  {                                                    // we check now, if some limitation is needed, or a new skipping has to get started
   DWORD dwWaitTime;

   if(bInitCap || bSkipNextFrame)                      // first time or we skipped before?
    {
     if(bUseFrameLimit && !bInitCap)                   // frame limit wanted and not first time called?
      {
       DWORD dwT=_ticks_since_last_update;             // -> that's the time of the last drawn frame
       dwLastLace+=dwLaceCnt;                          // -> and that's the number of updatelace since the start of the last drawn frame

       curticks = timeGetTime();
       _ticks_since_last_update= dwT+curticks - lastticks;

       dwWaitTime=dwLastLace*dwFrameRateTicks;         // -> and now we calc the time the real psx would have needed

       if(_ticks_since_last_update<dwWaitTime)         // -> we were too fast?
        {                                    
         if((dwWaitTime-_ticks_since_last_update)>     // -> some more security, to prevent
            (60*dwFrameRateTicks))                     //    wrong waiting times
          _ticks_since_last_update=dwWaitTime;

         while(_ticks_since_last_update<dwWaitTime)    // -> loop until we have reached the real psx time
          {                                            //    (that's the additional limitation, yup)
           curticks = timeGetTime();
           _ticks_since_last_update = dwT+curticks - lastticks;
          }
        }
       else                                            // we were still too slow ?!!?
        {
         if(iAdditionalSkip<MAXSKIP)                   // -> well, somewhen we really have to stop skipping on very slow systems
          {
           iAdditionalSkip++;                          // -> inc our watchdog var
           dwLaceCnt=0;                                // -> reset lace count
           lastticks = timeGetTime();
           return;                                     // -> done, we will skip next frame to get more speed
          }
        }
      }

     bInitCap=FALSE;                                   // -> ok, we have inited the frameskip func
     iAdditionalSkip=0;                                // -> init additional skip
     bSkipNextFrame=FALSE;                             // -> we don't skip the next frame
     lastticks = timeGetTime();
     dwLaceCnt=0;                                      // -> and we start to count the laces 
     dwLastLace=0;      
     _ticks_since_last_update=0;
     return;                                           // -> done, the next frame will get drawn
    }

   bSkipNextFrame=FALSE;                               // init the frame skip signal to 'no skipping' first

   curticks = timeGetTime();
   _ticks_since_last_update = curticks - lastticks;

   dwLastLace=dwLaceCnt;                               // store curr count (frame limitation helper)
   dwWaitTime=dwLaceCnt*dwFrameRateTicks;              // calc the 'real psx lace time'

   if(_ticks_since_last_update>dwWaitTime)             // hey, we needed way too long for that frame...
    {
     if(bUseFrameLimit)                                // if limitation, we skip just next frame,
      {                                                // and decide after, if we need to do more
       iNumSkips=0;
      }
     else
      {
       iNumSkips=_ticks_since_last_update/dwWaitTime;  // -> calc number of frames to skip to catch up
       iNumSkips--;                                    // -> since we already skip next frame, one down
       if(iNumSkips>MAXSKIP) iNumSkips=MAXSKIP;        // -> well, somewhere we have to draw a line
      }
     bSkipNextFrame = TRUE;                            // -> signal for skipping the next frame
    }
   else                                                // we were faster than real psx? fine :)
   if(bUseFrameLimit)                                  // frame limit used? so we wait til the 'real psx time' has been reached
    {
     if(dwLaceCnt>MAXLACE)                             // -> security check
      _ticks_since_last_update=dwWaitTime;

     while(_ticks_since_last_update<dwWaitTime)        // just do a waiting loop...
      {
       curticks = timeGetTime();
       _ticks_since_last_update = curticks - lastticks;
      }
    }

   lastticks = timeGetTime();
  }

 dwLaceCnt=0;                                          // init lace counter
}

//////////////////////////////////////////////////////////////////////// 
 
void calcfps(void) 
{ 
 static unsigned long curticks,_ticks_since_last_update,lastticks; 
 static long   fps_cnt = 0;
 static unsigned long  fps_tck = 1; 
 static long           fpsskip_cnt = 0;
 static unsigned long  fpsskip_tck = 1;
 
  { 
   curticks = timeGetTime(); 
   _ticks_since_last_update=curticks-lastticks; 
 
   if(bUseFrameSkip && !bUseFrameLimit && _ticks_since_last_update) 
    fps_skip=min(fps_skip,((float)TIMEBASE/(float)_ticks_since_last_update+1.0f));
 
   lastticks = curticks; 
  } 
 
 if(bUseFrameSkip && bUseFrameLimit)
  {
   fpsskip_tck += _ticks_since_last_update;

   if(++fpsskip_cnt==2)
    {
     fps_skip = (float)2000/(float)fpsskip_tck;

     fps_skip +=6.0f;

     fpsskip_cnt = 0;
     fpsskip_tck = 1;
    }
  }

 fps_tck += _ticks_since_last_update; 
 
 if(++fps_cnt==10) 
  { 
   fps_cur = (float)(TIMEBASE*10)/(float)fps_tck; 

   fps_cnt = 0; 
   fps_tck = 1; 
 
   if(bUseFrameLimit && fps_cur>fFrameRateHz)            // optical adjust ;) avoids flickering fps display 
    fps_cur=fFrameRateHz; 
  } 
} 

void PCFrameCap (void) 
{
 static unsigned long curticks, lastticks, _ticks_since_last_update;
 static unsigned long TicksToWait = 0;
 bool Waiting = TRUE; 
 
 while (Waiting) 
  {
   curticks = timeGetTime(); 
   _ticks_since_last_update = curticks - lastticks; 
   if ((_ticks_since_last_update > TicksToWait) ||  
       (curticks < lastticks)) 
    { 
     Waiting = FALSE; 
     lastticks = curticks; 
     TicksToWait = (TIMEBASE / (unsigned long)fFrameRateHz); 
    } 
  } 
} 
 
//////////////////////////////////////////////////////////////////////// 
 
void PCcalcfps(void) 
{ 
 static unsigned long curticks,_ticks_since_last_update,lastticks; 
 static long  fps_cnt = 0; 
 static float fps_acc = 0;
 float CurrentFPS=0;     
  
 curticks = timeGetTime(); 
 _ticks_since_last_update=curticks-lastticks;
 if(_ticks_since_last_update) 
      CurrentFPS=(float)TIMEBASE/(float)_ticks_since_last_update;
 else CurrentFPS = 0;
 lastticks = curticks; 
 
 fps_acc += CurrentFPS;

 if(++fps_cnt==10)
  {
   fps_cur = fps_acc / 10;
   fps_acc = 0;
   fps_cnt = 0;
  }

 fps_skip=CurrentFPS+1.0f;
}

////////////////////////////////////////////////////////////////////////

void SetAutoFrameCap(void)
{
 if(iFrameLimit==1)
  {
   fFrameRateHz = fFrameRate;
   dwFrameRateTicks=(TIMEBASE / (unsigned long)fFrameRateHz);
   return;
  }

 if(dwActFixes&128)
  {
   if (PSXDisplay.Interlaced)
        fFrameRateHz = PSXDisplay.PAL?50.0f:60.0f;
   else fFrameRateHz = PSXDisplay.PAL?25.0f:30.0f;
  }
 else
  {
   //fFrameRateHz = PSXDisplay.PAL?50.0f:59.94f;

   if(PSXDisplay.PAL)
    {
     if (STATUSREG&GPUSTATUS_INTERLACED)
           fFrameRateHz=33868800.0f/677343.75f;        // 50.00238
      else fFrameRateHz=33868800.0f/680595.00f;        // 49.76351
    }                                                     
   else
    {
     if (STATUSREG&GPUSTATUS_INTERLACED)
           fFrameRateHz=33868800.0f/565031.25f;        // 59.94146
      else fFrameRateHz=33868800.0f/566107.50f;        // 59.82750
    }

   dwFrameRateTicks=(TIMEBASE / (unsigned long)fFrameRateHz);
  }
}

////////////////////////////////////////////////////////////////////////

void SetFrameRateConfig(void)
{
 if(!fFrameRate) fFrameRate=200.0f;

 if(fFrameRateHz==0) 
  {                                                    
   if(iFrameLimit==2) fFrameRateHz=59.94f;             // auto framerate? set some init val (no pal/ntsc known yet)
   else               fFrameRateHz=fFrameRate;         // else set user framerate
  }

 dwFrameRateTicks=(TIMEBASE / (unsigned long)fFrameRateHz);

 if(iFrameLimit==2) SetAutoFrameCap();
}

////////////////////////////////////////////////////////////////////////

void InitFrameCap(void)
{
 // nothing on linux
}

void ReInitFrameCap(void)
{
 // nothing on linux
}

////////////////////////////////////////////////////////////////////////

#endif

////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////

void CheckFrameRate(void)                              // called in updatelace (on every emulated psx vsync)
{
 if(bUseFrameSkip) 
  {
   if(!(dwActFixes&0x100))
    {
     dwLaceCnt++;                                      // -> and store cnt of vsync between frames
     if(dwLaceCnt>=MAXLACE && bUseFrameLimit) 
      {
       if(dwLaceCnt==MAXLACE) bInitCap=TRUE;
       FrameCap();
      }
    }
   else if(bUseFrameLimit) FrameCap();
   calcfps();                                          // -> calc fps display in skipping mode
  }                                                  
 else                                                  // -> non-skipping mode:
  {
   if(bUseFrameLimit) FrameCap();
   calcfps();  
  }
}

////////////////////////////////////////////////////////////////////////

void CALLBACK GPUsetframelimit(unsigned long option)   // new EPSXE interface func: main emu can enable/disable fps limitation this way
{
 bInitCap = TRUE;

 if(option==1)                                         // emu says: limit
  {
   bUseFrameLimit=TRUE;bUseFrameSkip=FALSE;iFrameLimit=2;
   SetAutoFrameCap();
  }
 else                                                  // emu says: no limit
  {
   bUseFrameLimit=FALSE;
  }
}

////////////////////////////////////////////////////////////////////////
