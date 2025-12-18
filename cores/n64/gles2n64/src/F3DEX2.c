#include <assert.h>

#include <retro_miscellaneous.h>

#include "gles2N64.h"
#include "Debug.h"
#include "F3D.h"
#include "F3DEX.h"
#include "F3DEX2.h"
#include "N64.h"
#include "RSP.h"
#include "RDP.h"
#include "gSP.h"
#include "gDP.h"
#include "GBI.h"
#include "OpenGL.h"

#include "Config.h"

#include "../../Graphics/RSP/gSP_funcs_C.h"

void F3DEX2_Mtx( uint32_t w0, uint32_t w1 )
{
   gln64gSPMatrix( w1, _SHIFTR( w0, 0, 8 ) ^ G_MTX_PUSH );
}

void F3DEX2_MoveMem( uint32_t w0, uint32_t w1 )
{
   switch (_SHIFTR( w0, 0, 8 ))
   {
      case F3DEX2_MV_VIEWPORT:
         gSPViewport( w1 );
         break;

      case G_MV_MATRIX:
         gSPForceMatrix( w1 );

         /* force matrix takes two commands */
         __RSP.PC[__RSP.PCi] += 8;
         break;

      case G_MV_LIGHT:
         {
            const uint32_t offset = (_SHIFTR( w0, 5, 11)) & 0x7F8;
            const uint32_t n = offset / 24;

            if (n < 2)
               gSPLookAt(w1, n);
            else
               gSPLight(w1, n - 1);
         }
         break;
   }
}

void F3DEX2_Vtx( uint32_t w0, uint32_t w1 )
{
   uint32_t n = _SHIFTR( w0, 12, 8 );

   gln64gSPVertex( w1, n, _SHIFTR( w0, 1, 7 ) - n );
}

void F3DEX2_Reserved1( uint32_t w0, uint32_t w1 )
{
}

void F3DEX2_Tri1( uint32_t w0, uint32_t w1 )
{
   gln64gSP1Triangle( _SHIFTR( w0, 17, 7 ),
         _SHIFTR( w0, 9, 7 ),
         _SHIFTR( w0, 1, 7 ), 0);
}

void F3DEX2_Line3D( uint32_t w0, uint32_t w1 )
{
}

void F3DEX2_PopMtx( uint32_t w0, uint32_t w1 )
{
   gln64gSPPopMatrixN( 0, w1 >> 6 );
}

void F3DEX2_MoveWord( uint32_t w0, uint32_t w1 )
{
   switch (_SHIFTR( w0, 16, 8 ))
   {
      case G_MW_FORCEMTX:
         /* Handled in movemem */
         break;
      case G_MW_MATRIX:
         gln64gSPInsertMatrix( _SHIFTR( w0, 0, 16 ), w1 );
         break;
      case G_MW_NUMLIGHT:
         gln64gSPNumLights( w1 / 24 );
         break;
      case G_MW_CLIP:
         gln64gSPClipRatio( w1 );
         break;
      case G_MW_SEGMENT:
         gln64gSPSegment( _SHIFTR( w0, 0, 16 ) >> 2, w1 & 0x00FFFFFF );
         break;
      case G_MW_FOG:
         gln64gSPFogFactor( (int16_t)_SHIFTR( w1, 16, 16 ), (int16_t)_SHIFTR( w1, 0, 16 ) );
			//offset must be 0 for move_fog, but it can be non zero in Nushi Zuri 64 - Shiokaze ni Notte
			//low-level display list has setothermode commands in this place, so this is obviously not move_fog.
			if (_SHIFTR(w0, 0, 16) == 0x04)
				gln64gDPSetTextureLUT((w1 == 0xffffffff) ? G_TT_NONE : G_TT_RGBA16);
         break;
      case G_MW_LIGHTCOL:
         gln64gSPLightColor((_SHIFTR( w0, 0, 16 ) / 24) + 1, w1 );
         break;
      case G_MW_PERSPNORM:
         gln64gSPPerspNormalize( w1 );
         break;
   }
}

void F3DEX2_Texture( uint32_t w0, uint32_t w1 )
{
   gln64gSPTexture( _FIXED2FLOAT( _SHIFTR( w1, 16, 16 ), 16 ),
         _FIXED2FLOAT( _SHIFTR( w1, 0, 16 ), 16 ),
         _SHIFTR( w0, 11, 3 ),
         _SHIFTR( w0, 8, 3 ),
         _SHIFTR( w0, 1, 7 ) );
}

void F3DEX2_SetOtherMode_H( uint32_t w0, uint32_t w1 )
{
	const uint32_t length = _SHIFTR(w0, 0, 8) + 1;
	const uint32_t shift = MAX(0, (int32_t)(32 - _SHIFTR(w0, 8, 8) - length));
	gln64gSPSetOtherMode_H(length, shift, w1);
}

void F3DEX2_SetOtherMode_L( uint32_t w0, uint32_t w1 )
{
	const uint32_t length = _SHIFTR(w0, 0, 8) + 1;
	const uint32_t shift = MAX(0, (int32_t)(32 - _SHIFTR(w0, 8, 8) - length));
	gln64gSPSetOtherMode_L(length, shift, w1);
}

void F3DEX2_GeometryMode( uint32_t w0, uint32_t w1 )
{
   gln64gSPGeometryMode( ~_SHIFTR( w0, 0, 24 ), w1 );
}

void F3DEX2_DMAIO( uint32_t w0, uint32_t w1 )
{
}

void F3DEX2_Special_1( uint32_t w0, uint32_t w1 )
{
	gln64gSPDlistCount(_SHIFTR( w0, 0, 8 ), w1);
}

void F3DEX2_Special_2( uint32_t w0, uint32_t w1 )
{
}

void F3DEX2_Special_3( uint32_t w0, uint32_t w1 )
{
}

void F3DEX2_Quad( uint32_t w0, uint32_t w1 )
{
   gln64gSP2Triangles( _SHIFTR( w0, 17, 7 ),
         _SHIFTR( w0, 9, 7 ),
         _SHIFTR( w0, 1, 7 ),
         0,
         _SHIFTR( w1, 17, 7 ),
         _SHIFTR( w1, 9, 7 ),
         _SHIFTR( w1, 1, 7 ),
         0 );
}

void F3DEX2_Init(void)
{
	gSPSetupFunctions();
   // Set GeometryMode flags
   GBI_InitFlags( F3DEX2 );

   GBI.PCStackSize = 18;

   // GBI Command                      Command Value               Command Function
   GBI_SetGBI( G_RDPHALF_2,            F3DEX2_RDPHALF_2,           F3D_RDPHalf_2 );
   GBI_SetGBI( G_SETOTHERMODE_H,       F3DEX2_SETOTHERMODE_H,      F3DEX2_SetOtherMode_H );
   GBI_SetGBI( G_SETOTHERMODE_L,       F3DEX2_SETOTHERMODE_L,      F3DEX2_SetOtherMode_L );
   GBI_SetGBI( G_RDPHALF_1,            F3DEX2_RDPHALF_1,           F3D_RDPHalf_1 );
   GBI_SetGBI( G_SPNOOP,               F3DEX2_SPNOOP,              F3D_SPNoOp );
   GBI_SetGBI( G_ENDDL,                F3DEX2_ENDDL,               F3D_EndDL );
   GBI_SetGBI( G_DL,                   F3DEX2_DL,                  F3D_DList );
   GBI_SetGBI( G_LOAD_UCODE,           F3DEX2_LOAD_UCODE,          F3DEX_Load_uCode );
   GBI_SetGBI( G_MOVEMEM,              F3DEX2_MOVEMEM,             F3DEX2_MoveMem );
   GBI_SetGBI( G_MOVEWORD,             F3DEX2_MOVEWORD,            F3DEX2_MoveWord );
   GBI_SetGBI( G_MTX,                  F3DEX2_MTX,                 F3DEX2_Mtx );
   GBI_SetGBI( G_GEOMETRYMODE,         F3DEX2_GEOMETRYMODE,        F3DEX2_GeometryMode );
   GBI_SetGBI( G_POPMTX,               F3DEX2_POPMTX,              F3DEX2_PopMtx );
   GBI_SetGBI( G_TEXTURE,              F3DEX2_TEXTURE,             F3DEX2_Texture );
   GBI_SetGBI( G_DMA_IO,               F3DEX2_DMA_IO,              F3DEX2_DMAIO );
   GBI_SetGBI( G_SPECIAL_1,            F3DEX2_SPECIAL_1,           F3DEX2_Special_1 );
   GBI_SetGBI( G_SPECIAL_2,            F3DEX2_SPECIAL_2,           F3DEX2_Special_2 );
   GBI_SetGBI( G_SPECIAL_3,            F3DEX2_SPECIAL_3,           F3DEX2_Special_3 );

   GBI_SetGBI( G_VTX,                  F3DEX2_VTX,                 F3DEX2_Vtx );
   GBI_SetGBI( G_MODIFYVTX,            F3DEX2_MODIFYVTX,           F3DEX_ModifyVtx );
   GBI_SetGBI( G_CULLDL,               F3DEX2_CULLDL,              F3DEX_CullDL );
   GBI_SetGBI( G_BRANCH_Z,             F3DEX2_BRANCH_Z,            F3DEX_Branch_Z );
   GBI_SetGBI( G_TRI1,                 F3DEX2_TRI1,                F3DEX2_Tri1 );
   GBI_SetGBI( G_TRI2,                 F3DEX2_TRI2,                F3DEX_Tri2 );
   GBI_SetGBI( G_QUAD,                 F3DEX2_QUAD,                F3DEX2_Quad );
   GBI_SetGBI( G_LINE3D,               F3DEX2_LINE3D,              F3DEX2_Line3D );
}
