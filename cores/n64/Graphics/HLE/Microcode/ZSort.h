#ifndef _GRAPHICS_HLE_MICROCODE_ZSORT_H
#define _GRAPHICS_HLE_MICROCODE_ZSORT_H

#include <stdint.h>

#include <retro_inline.h>

#ifdef __cplusplus
extern "C" {
#endif

#define	GZM_USER0		0
#define	GZM_USER1		2
#define	GZM_MMTX		   4
#define	GZM_PMTX		   6
#define	GZM_MPMTX		8
#define	GZM_OTHERMODE	10
#define	GZM_VIEWPORT	12
#define	GZF_LOAD		   0
#define	GZF_SAVE		   1

#define	ZH_NULL		   0
#define	ZH_SHTRI	      1
#define	ZH_TXTRI	      2
#define	ZH_SHQUAD	   3
#define	ZH_TXQUAD	   4

typedef float M44[4][4];

typedef struct
{
   float view_scale[2];
   float view_trans[2];
   float scale_x;
   float scale_y;
} ZSORTRDP;

static INLINE int ZSort_Calc_invw (int _w)
{
   int count, neg;
   union
   {
      int32_t W;
      uint32_t UW;
      int16_t HW[2];
      uint16_t UHW[2];
   } Result;

   Result.W = _w;

   if (Result.UW == 0)
      Result.UW = 0x7FFFFFFF;
   else
   {
      if (Result.W < 0)
      {
         neg = true;
         if (Result.UHW[1] == 0xFFFF && Result.HW[0] < 0)
            Result.W = ~Result.W + 1;
         else
            Result.W = ~Result.W;
      }
      else
         neg = false;

      for (count = 31; count > 0; --count)
      {
         if ((Result.W & (1 << count)))
         {
            Result.W &= (0xFFC00000 >> (31 - count) );
            count = 0;
         }
      }

      Result.W = 0x7FFFFFFF / Result.W;
      for (count = 31; count > 0; --count)
      {
         if ((Result.W & (1 << count)))
         {
            Result.W &= (0xFFFF8000 >> (31 - count) );
            count = 0;
         }
      }

      if (neg == true)
         Result.W = ~Result.W;
   }
   return Result.W;
}

#ifdef __cplusplus
}
#endif

#endif
