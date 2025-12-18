#include "gles2N64.h"
#include "Debug.h"
#include "F3D.h"
#include "N64.h"
#include "RSP.h"
#include "RDP.h"
#include "gSP.h"
#include "gDP.h"
#include "GBI.h"
#include "OpenGL.h"
#include "DepthBuffer.h"

#include "Config.h"

#include "../../Graphics/RDP/gDP_state.h"
#include "../../Graphics/RSP/gSP_funcs_C.h"

void F3D_SPNoOp( uint32_t w0, uint32_t w1 )
{
   gln64gSPNoOp();
}

void F3D_Mtx( uint32_t w0, uint32_t w1 )
{
   if (_SHIFTR( w0, 0, 16 ) != 64)
      return;

   gln64gSPMatrix( w1, _SHIFTR( w0, 16, 8 ) );
}

void F3D_Vtx( uint32_t w0, uint32_t w1 )
{
   gln64gSPVertex( w1, _SHIFTR( w0, 20, 4 ) + 1, _SHIFTR( w0, 16, 4 ) );
}

void F3D_DList( uint32_t w0, uint32_t w1 )
{
   switch (_SHIFTR( w0, 16, 8 ))
   {
      case G_DL_PUSH:
         gln64gSPDisplayList( w1 );
         break;
      case G_DL_NOPUSH:
         gln64gSPBranchList( w1 );
         break;
   }
}

void F3D_Sprite2D_Base( uint32_t w0, uint32_t w1 )
{
	gln64gSPSprite2DBase( w1 );
}


void F3D_Tri1( uint32_t w0, uint32_t w1 )
{
   gln64gSP1Triangle( _SHIFTR( w1, 16, 8 ) / 10,
         _SHIFTR( w1, 8, 8 ) / 10,
         _SHIFTR( w1, 0, 8 ) / 10, 0);
}

void F3D_CullDL( uint32_t w0, uint32_t w1 )
{
   gln64gSPCullDisplayList( _SHIFTR( w0, 0, 24 ) / 40, (w1 / 40) - 1 );
}

void F3D_PopMtx( uint32_t w0, uint32_t w1 )
{
   gln64gSPPopMatrix( w1 );
}

void F3D_MoveWord( uint32_t w0, uint32_t w1 )
{
   switch (_SHIFTR( w0, 0, 8 ))
   {
      case G_MW_MATRIX:
         gln64gSPInsertMatrix( _SHIFTR( w0, 8, 16 ), w1 );
         break;

      case G_MW_NUMLIGHT:
         gln64gSPNumLights( ((w1 - 0x80000000) >> 5) - 1 );
         break;

      case G_MW_CLIP:
         gln64gSPClipRatio( w1 );
         break;

      case G_MW_SEGMENT:
			gln64gSPSegment( _SHIFTR( w0, 10, 4 ), w1 & 0x00FFFFFF );
         break;

      case G_MW_FOG:
			gln64gSPFogFactor( (int16_t)_SHIFTR( w1, 16, 16 ), (int16_t)_SHIFTR( w1, 0, 16 ) );
         break;

      case G_MW_LIGHTCOL:
         switch (_SHIFTR( w0, 8, 16 ))
         {
            case F3D_MWO_aLIGHT_1:
               gln64gSPLightColor( LIGHT_1, w1 );
               break;
            case F3D_MWO_aLIGHT_2:
               gln64gSPLightColor( LIGHT_2, w1 );
               break;
            case F3D_MWO_aLIGHT_3:
               gln64gSPLightColor( LIGHT_3, w1 );
               break;
            case F3D_MWO_aLIGHT_4:
               gln64gSPLightColor( LIGHT_4, w1 );
               break;
            case F3D_MWO_aLIGHT_5:
               gln64gSPLightColor( LIGHT_5, w1 );
               break;
            case F3D_MWO_aLIGHT_6:
               gln64gSPLightColor( LIGHT_6, w1 );
               break;
            case F3D_MWO_aLIGHT_7:
               gln64gSPLightColor( LIGHT_7, w1 );
               break;
            case F3D_MWO_aLIGHT_8:
               gln64gSPLightColor( LIGHT_8, w1 );
               break;
         }
         break;
      case G_MW_POINTS:
         {
            const uint32_t val = _SHIFTR(w0, 8, 16);
            gln64gSPModifyVertex(val / 40, val % 40, w1);
         }
         break;
      case G_MW_PERSPNORM:
         gln64gSPPerspNormalize( w1 );
         break;
   }
}

void F3D_Texture( uint32_t w0, uint32_t w1 )
{
   gln64gSPTexture( _FIXED2FLOAT( _SHIFTR( w1, 16, 16 ), 16 ),
         _FIXED2FLOAT( _SHIFTR( w1, 0, 16 ), 16 ),
         _SHIFTR( w0, 11, 3 ),
         _SHIFTR( w0, 8, 3 ),
         _SHIFTR( w0, 0, 8 ) );
}

void F3D_SetOtherMode_H( uint32_t w0, uint32_t w1 )
{
	const uint32_t length = _SHIFTR(w0, 0, 8);
	const uint32_t shift  = _SHIFTR(w0, 8, 8);
	gln64gSPSetOtherMode_H(length, shift, w1);
}

void F3D_SetOtherMode_L( uint32_t w0, uint32_t w1 )
{
	const uint32_t length = _SHIFTR(w0, 0, 8);
	const uint32_t shift = _SHIFTR(w0, 8, 8);
	gln64gSPSetOtherMode_L(length, shift, w1);
}

void F3D_SetGeometryMode( uint32_t w0, uint32_t w1 )
{
   gln64gSPSetGeometryMode( w1 );
}

void F3D_ClearGeometryMode( uint32_t w0, uint32_t w1 )
{
   gln64gSPClearGeometryMode( w1 );
}

void F3D_Quad( uint32_t w0, uint32_t w1 )
{
	gln64gSP1Quadrangle( _SHIFTR( w1, 24, 8 ) / 10, _SHIFTR( w1, 16, 8 ) / 10, _SHIFTR( w1, 8, 8 ) / 10, _SHIFTR( w1, 0, 8 ) / 10 );
}

void F3D_RDPHalf_1( uint32_t w0, uint32_t w1 )
{
   gDP.half_1 = w1;
	RDP_Half_1(w1);
}

void F3D_RDPHalf_2( uint32_t w0, uint32_t w1 )
{
   gDP.half_2 = w1;
}

void F3D_RDPHalf_Cont( uint32_t w0, uint32_t w1 )
{
}

void F3D_Tri4( uint32_t w0, uint32_t w1 )
{
	gln64gSP4Triangles( _SHIFTR( w1, 28, 4 ), _SHIFTR( w0, 12, 4 ), _SHIFTR( w1, 24, 4 ),
				   _SHIFTR( w1, 20, 4 ), _SHIFTR( w0,  8, 4 ), _SHIFTR( w1, 16, 4 ),
				   _SHIFTR( w1, 12, 4 ), _SHIFTR( w0,  4, 4 ), _SHIFTR( w1,  8, 4 ),
				   _SHIFTR( w1,  4, 4 ), _SHIFTR( w0,  0, 4 ), _SHIFTR( w1,  0, 4 ) );
}

void F3D_Init(void)
{
	gSPSetupFunctions();
   // Set GeometryMode flags
   GBI_InitFlags( F3D );

   GBI.PCStackSize = 10;

   //          GBI Command             Command Value           Command Function
   GBI_SetGBI( G_SPNOOP,               F3D_SPNOOP,             F3D_SPNoOp );
   GBI_SetGBI( G_MTX,                  F3D_MTX,                F3D_Mtx );
   GBI_SetGBI( G_RESERVED0,            F3D_RESERVED0,          F3D_Reserved0 );
   GBI_SetGBI( G_MOVEMEM,              F3D_MOVEMEM,            F3D_MoveMem );
   GBI_SetGBI( G_VTX,                  F3D_VTX,                F3D_Vtx );
   GBI_SetGBI( G_RESERVED1,            F3D_RESERVED1,          F3D_Reserved1 );
   GBI_SetGBI( G_DL,                   F3D_DL,                 F3D_DList );
   GBI_SetGBI( G_RESERVED2,            F3D_RESERVED2,          F3D_Reserved2 );
   GBI_SetGBI( G_RESERVED3,            F3D_RESERVED3,          F3D_Reserved3 );
   GBI_SetGBI( G_SPRITE2D_BASE,        F3D_SPRITE2D_BASE,      F3D_Sprite2D_Base );

   GBI_SetGBI( G_TRI1,                 F3D_TRI1,               F3D_Tri1 );
   GBI_SetGBI( G_CULLDL,               F3D_CULLDL,             F3D_CullDL );
   GBI_SetGBI( G_POPMTX,               F3D_POPMTX,             F3D_PopMtx );
   GBI_SetGBI( G_MOVEWORD,             F3D_MOVEWORD,           F3D_MoveWord );
   GBI_SetGBI( G_TEXTURE,              F3D_TEXTURE,            F3D_Texture );
   GBI_SetGBI( G_SETOTHERMODE_H,       F3D_SETOTHERMODE_H,     F3D_SetOtherMode_H );
   GBI_SetGBI( G_SETOTHERMODE_L,       F3D_SETOTHERMODE_L,     F3D_SetOtherMode_L );
   GBI_SetGBI( G_ENDDL,                F3D_ENDDL,              F3D_EndDL );
   GBI_SetGBI( G_SETGEOMETRYMODE,      F3D_SETGEOMETRYMODE,    F3D_SetGeometryMode );
   GBI_SetGBI( G_CLEARGEOMETRYMODE,    F3D_CLEARGEOMETRYMODE,  F3D_ClearGeometryMode );
   GBI_SetGBI( G_QUAD,                 F3D_QUAD,               F3D_Quad );
   GBI_SetGBI( G_RDPHALF_1,            F3D_RDPHALF_1,          F3D_RDPHalf_1 );
   GBI_SetGBI( G_RDPHALF_2,            F3D_RDPHALF_2,          F3D_RDPHalf_2 );
   GBI_SetGBI( G_RDPHALF_CONT,         F3D_RDPHALF_CONT,       F3D_RDPHalf_Cont );
   GBI_SetGBI( G_TRI4,                 F3D_TRI4,               F3D_Tri4 );

}
