/*
 * (C) notaz, 2010-2011
 *
 * This work is licensed under the terms of the GNU GPLv2 or later.
 * See the COPYING file in the top-level directory.
 */

#define _IN_DRAW

#include "gpu.h"

#include "../../frontend/plugin_lib.h"
#include "pcnt.h"

// misc globals
long           lLowerpart;
BOOL           bCheckMask = FALSE;
unsigned short sSetMask;
unsigned long  lSetMask;

static void blit(void)
{
 int px = PSXDisplay.DisplayPosition.x & ~1; // XXX: align needed by bgr*_to_...
 int py = PSXDisplay.DisplayPosition.y;
 int w = PreviousPSXDisplay.Range.x1;
 int h = PreviousPSXDisplay.DisplayMode.y;
 unsigned short *srcs = psxVuw + py * 1024 + px;

 if (w <= 0)
   return;

 // account for centering
 h -= PreviousPSXDisplay.Range.y0;

 #error out of date
 rcbs->pl_vout_flip(srcs, 1024, PSXDisplay.RGB24, w, h);
}

void DoBufferSwap(void)
{
 static int fbw, fbh, fb24bpp;

 if (PreviousPSXDisplay.DisplayMode.x == 0 || PreviousPSXDisplay.DisplayMode.y == 0)
  return;

 /* careful if rearranging this code, we try to set mode and flip
  * to get the hardware apply both changes at the same time */
 if (PreviousPSXDisplay.DisplayMode.x != fbw || PreviousPSXDisplay.DisplayMode.y != fbh
     || PSXDisplay.RGB24 != fb24bpp) {
  fbw = PreviousPSXDisplay.DisplayMode.x;
  fbh = PreviousPSXDisplay.DisplayMode.y;
  fb24bpp = PSXDisplay.RGB24;
  rcbs->pl_vout_set_mode(fbw, fbh, fbw, fbh, fb24bpp ? 24 : 16);
 }

 pcnt_start(PCNT_BLIT);
 blit();
 pcnt_end(PCNT_BLIT);
}

void DoClearScreenBuffer(void)
{
}

unsigned long ulInitDisplay(void)
{
 if (rcbs->pl_vout_open() != 0)
  return 0;

 return 1; /* ok */
}

void CloseDisplay(void)
{
 rcbs->pl_vout_close();
}
