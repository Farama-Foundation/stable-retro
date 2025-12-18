#include <stdint.h>

#include "../../GBI.h"
#include "../../RSP/gSP_funcs_C.h"
#include "../../RSP/RSP_state.h"
#include "../../plugin.h"

void F3D_MoveMem(uint32_t w0, uint32_t w1)
{
   /* Check the command */
   switch (_SHIFTR( w0, 16, 8))
   {
      case F3D_MV_VIEWPORT:
         gSPViewport( w1 );
         break;
      case G_MV_MATRIX_1:
         gSPForceMatrix(w1);
         /* force matrix takes four commands */
         __RSP.PC[__RSP.PCi] += 24; 
         break;
      case G_MV_L0:
         gSPLight( w1, LIGHT_1 );
         break;
      case G_MV_L1:
         gSPLight( w1, LIGHT_2 );
         break;
      case G_MV_L2:
         gSPLight( w1, LIGHT_3 );
         break;
      case G_MV_L3:
         gSPLight( w1, LIGHT_4 );
         break;
      case G_MV_L4:
         gSPLight( w1, LIGHT_5 );
         break;
      case G_MV_L5:
         gSPLight( w1, LIGHT_6 );
         break;
      case G_MV_L6:
         gSPLight( w1, LIGHT_7 );
         break;
      case G_MV_L7:
         gSPLight( w1, LIGHT_8 );
         break;
      case G_MV_LOOKATX:
         gSPLookAt(w1, 0);
         break;
      case G_MV_LOOKATY:
         gSPLookAt(w1, 1);
         break;

   }
}

void F3D_Reserved0( uint32_t w0, uint32_t w1 )
{
}

void F3D_Reserved1( uint32_t w0, uint32_t w1 )
{
}

void F3D_Reserved2( uint32_t w0, uint32_t w1 )
{
}

void F3D_Reserved3( uint32_t w0, uint32_t w1 )
{
}

void F3D_EndDL( uint32_t w0, uint32_t w1 )
{
   gSPEndDisplayList();
}
