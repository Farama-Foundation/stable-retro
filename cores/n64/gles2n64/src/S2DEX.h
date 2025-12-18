#ifndef S2DEX_H
#define S2DEX_H

#include "../../Graphics/HLE/Microcode/S2DEX.h"

#ifdef __cplusplus
extern "C" {
#endif

void S2DEX_BG_1Cyc( uint32_t w0, uint32_t w1 );
void S2DEX_BG_Copy( uint32_t w0, uint32_t w1 );
void S2DEX_Obj_Rectangle( uint32_t w0, uint32_t w1 );
void S2DEX_Obj_Sprite( uint32_t w0, uint32_t w1 );
void S2DEX_Obj_MoveMem( uint32_t w0, uint32_t w1 );
void S2DEX_Select_DL( uint32_t w0, uint32_t w1 );
void S2DEX_Obj_RenderMode( uint32_t w0, uint32_t w1 );
void S2DEX_Obj_Rectangle_R( uint32_t w0, uint32_t w1 );
void S2DEX_Obj_LoadTxtr( uint32_t w0, uint32_t w1 );
void S2DEX_Obj_LdTx_Sprite( uint32_t w0, uint32_t w1 );
void S2DEX_Obj_LdTx_Rect( uint32_t w0, uint32_t w1 );
void S2DEX_Obj_LdTx_Rect_R( uint32_t w0, uint32_t w1 );
void S2DEX_Init(void);

#ifdef __cplusplus
}
#endif

#endif

