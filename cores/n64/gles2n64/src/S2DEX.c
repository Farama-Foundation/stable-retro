#include <stdint.h>

#include "OpenGL.h"
#include "S2DEX.h"
#include "F3D.h"
#include "F3DEX.h"
#include "GBI.h"
#include "gSP.h"
#include "gDP.h"
#include "RSP.h"

void S2DEX_BG_1Cyc( uint32_t w0, uint32_t w1 )
{
   gln64gSPBgRect1Cyc( w1 );
}

void S2DEX_BG_Copy( uint32_t w0, uint32_t w1 )
{
   gln64gSPBgRectCopy( w1 );
}

void S2DEX_Obj_Rectangle( uint32_t w0, uint32_t w1 )
{
   gln64gSPObjRectangle( w1 );
}

void S2DEX_Obj_Sprite( uint32_t w0, uint32_t w1 )
{
   gln64gSPObjSprite( w1 );
}

void S2DEX_Obj_MoveMem( uint32_t w0, uint32_t w1 )
{
	switch (_SHIFTR( w0, 0, 16 )) {
		case S2DEX_MV_MATRIX:
			gln64gSPObjMatrix( w1 );
			break;
		case S2DEX_MV_SUBMATRIX:
			gln64gSPObjSubMatrix( w1 );
			break;
		case S2DEX_MV_VIEWPORT:
			gln64gSPViewport( w1 );
			break;
	}
}

void S2DEX_Select_DL( uint32_t w0, uint32_t w1 )
{
#ifdef DEBUG
	LOG(LOG_WARNING, "S2DEX_Select_DL unimplemented\n");
#endif
}

void S2DEX_Obj_RenderMode( uint32_t w0, uint32_t w1 )
{
	gln64gSPObjRendermode(w1);
}

void S2DEX_Obj_Rectangle_R( uint32_t w0, uint32_t w1 )
{
	gln64gSPObjRectangleR(w1);
}

void S2DEX_Obj_LoadTxtr( uint32_t w0, uint32_t w1 )
{
   gln64gSPObjLoadTxtr( w1 );
}

void S2DEX_Obj_LdTx_Sprite( uint32_t w0, uint32_t w1 )
{
   gln64gSPObjLoadTxSprite( w1 );
}

void S2DEX_Obj_LdTx_Rect( uint32_t w0, uint32_t w1 )
{
	gln64gSPObjLoadTxSprite( w1 );
}

void S2DEX_Obj_LdTx_Rect_R( uint32_t w0, uint32_t w1 )
{
   gln64gSPObjLoadTxRectR( w1 );
}

void S2DEX_Init(void)
{
   // Set GeometryMode flags
   GBI_InitFlags( F3DEX );

   GBI.PCStackSize = 18;

   //          GBI Command             Command Value           Command Function
   GBI_SetGBI( G_SPNOOP,               F3D_SPNOOP,             F3D_SPNoOp );
   GBI_SetGBI( G_BG_1CYC,              S2DEX_BG_1CYC,          S2DEX_BG_1Cyc );
   GBI_SetGBI( G_BG_COPY,              S2DEX_BG_COPY,          S2DEX_BG_Copy );
   GBI_SetGBI( G_OBJ_RECTANGLE,        S2DEX_OBJ_RECTANGLE,    S2DEX_Obj_Rectangle );
   GBI_SetGBI( G_OBJ_SPRITE,           S2DEX_OBJ_SPRITE,       S2DEX_Obj_Sprite );
   GBI_SetGBI( G_OBJ_MOVEMEM,          S2DEX_OBJ_MOVEMEM,      S2DEX_Obj_MoveMem );
   GBI_SetGBI( G_DL,                   F3D_DL,                 F3D_DList );
   GBI_SetGBI( G_SELECT_DL,            S2DEX_SELECT_DL,        S2DEX_Select_DL );
   GBI_SetGBI( G_OBJ_RENDERMODE,       S2DEX_OBJ_RENDERMODE,   S2DEX_Obj_RenderMode );
   GBI_SetGBI( G_OBJ_RECTANGLE_R,      S2DEX_OBJ_RECTANGLE_R,  S2DEX_Obj_Rectangle_R );
   GBI_SetGBI( G_OBJ_LOADTXTR,         S2DEX_OBJ_LOADTXTR,     S2DEX_Obj_LoadTxtr );
   GBI_SetGBI( G_OBJ_LDTX_SPRITE,      S2DEX_OBJ_LDTX_SPRITE,  S2DEX_Obj_LdTx_Sprite );
   GBI_SetGBI( G_OBJ_LDTX_RECT,        S2DEX_OBJ_LDTX_RECT,    S2DEX_Obj_LdTx_Rect );
   GBI_SetGBI( G_OBJ_LDTX_RECT_R,      S2DEX_OBJ_LDTX_RECT_R,  S2DEX_Obj_LdTx_Rect_R );
   GBI_SetGBI( G_MOVEWORD,             F3D_MOVEWORD,           F3D_MoveWord );
   GBI_SetGBI( G_SETOTHERMODE_H,       F3D_SETOTHERMODE_H,     F3D_SetOtherMode_H );
   GBI_SetGBI( G_SETOTHERMODE_L,       F3D_SETOTHERMODE_L,     F3D_SetOtherMode_L );
   GBI_SetGBI( G_ENDDL,                F3D_ENDDL,              F3D_EndDL );
   GBI_SetGBI( G_RDPHALF_1,            F3D_RDPHALF_1,          F3D_RDPHalf_1 );
   GBI_SetGBI( G_RDPHALF_2,            F3D_RDPHALF_2,          F3D_RDPHalf_2 );
   GBI_SetGBI( G_LOAD_UCODE,           S2DEX_LOAD_UCODE,       F3DEX_Load_uCode );
}
