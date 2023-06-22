#ifndef _GRAPHICS_HLE_MICROCODE_TURBO3D_H
#define _GRAPHICS_HLE_MICROCODE_TURBO3D_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************Turbo3D microcode*************************/

struct T3DGlobState
{
	uint16_t pad0;
	uint16_t perspNorm;
	uint32_t flag;
	uint32_t othermode0;
	uint32_t othermode1;
	uint32_t segBases[16];
	/* the viewport to use */
	int16_t vsacle1;
	int16_t vsacle0;
	int16_t vsacle3;
	int16_t vsacle2;
	int16_t vtrans1;
	int16_t vtrans0;
	int16_t vtrans3;
	int16_t vtrans2;
	uint32_t rdpCmds;
};

struct T3DState
{
	uint32_t renderState;	/* render state */
	uint32_t textureState;	/* texture state */
	uint8_t flag;
	uint8_t triCount;	      /* how many triangles? */
	uint8_t vtxV0;		      /* where to load vertices? */
	uint8_t vtxCount;	      /* how many vertices? */
	uint32_t rdpCmds;	      /* ptr (segment address) to RDP DL */
	uint32_t othermode0;
	uint32_t othermode1;
};

struct T3DTriN
{
	uint8_t	flag, v2, v1, v0;	/* flag is which one for flat shade */
};

#ifdef __cplusplus
}
#endif

#endif
