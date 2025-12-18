#include "RSP_state.h"

RSPInfo		__RSP;

void RSP_CheckDLCounter(void)
{
   if (__RSP.count != -1)
   {
      --__RSP.count;
      if (__RSP.count == 0)
      {
         __RSP.count = -1;
         --__RSP.PCi;
      }
   }
}
