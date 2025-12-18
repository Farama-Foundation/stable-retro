#ifndef _GSP_STATE_H
#define _GSP_STATE_H

#include <stdint.h>

#include <boolean.h>

#ifdef __cplusplus
extern "C" {
#endif

struct SPVertex
{
	float x, y, z, w;
	float nx, ny, nz, __pad0;
	float r, g, b, a;
	float flat_r, flat_g, flat_b, flat_a;
	float s, t;
	uint8_t HWLight;
	int16_t flag;
	uint32_t clip;
};

struct SPLight
{
	float r, g, b;
	float x, y, z;
	float posx, posy, posz, posw;
	float ca, la, qa;
};

struct gSPInfo
{
	uint32_t segment[16];

	struct
	{
		uint32_t modelViewi, stackSize, billboard;
		float modelView[32][4][4];
		float projection[4][4];
		float combined[4][4];
	} matrix;

	struct
	{
		float A, B, C, D;
		float X, Y;
		float baseScaleX, baseScaleY;
	} objMatrix;

	uint32_t objRendermode;

	uint32_t vertexColorBase;
	uint32_t vertexi;

	struct SPLight lights[12];
	struct SPLight lookat[2];
	bool lookatEnable;

	struct
	{
		float scales, scalet;
      uint16_t org_scales, org_scalet;
		int32_t level, on, tile;
	} texture;

	struct gDPTile *textureTile[2];

	struct
	{
		float vscale[4];
		float vtrans[4];
		float x, y, width, height;
		float nearz, farz;
	} viewport;

	struct
	{
		int16_t multiplier, offset;
	} fog;

	struct
	{
		uint32_t address, width, height, format, size, palette;
		float imageX, imageY, scaleW, scaleH;
	} bgImage;

	uint32_t geometryMode;
	int32_t numLights;

	uint32_t changed;

	uint32_t status[4];

	struct
	{
		uint32_t vtx, mtx, tex_offset, tex_shift, tex_count;
	} DMAOffsets;

	/* CBFD */
	uint32_t vertexNormalBase;
	float vertexCoordMod[16];
};

extern struct gSPInfo gSP;

#ifdef __cplusplus
}
#endif

#endif
