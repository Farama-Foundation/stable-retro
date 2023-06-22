#include "Turbo3D.h"
#include "N64.h"
#include "RSP.h"
#include "RDP.h"
#include "gSP.h"
#include "gDP.h"
#include "OpenGL.h"

#include "../../Graphics/RDP/gDP_state.h"
#include "../../Graphics/HLE/Microcode/turbo3d.h"

/******************Turbo3D microcode*************************/

static void Turbo3D_ProcessRDP(uint32_t _cmds)
{
   uint32_t addr = RSP_SegmentToPhysical(_cmds) >> 2;
   if (addr != 0)
   {
      uint32_t w0, w1;

      __RSP.bLLE = true;
      w0         = ((uint32_t*)gfx_info.RDRAM)[addr++];
      w1         = ((uint32_t*)gfx_info.RDRAM)[addr++];
      __RSP.cmd  = _SHIFTR( w0, 24, 8 );

      while (w0 + w1 != 0)
      {
         GBI.cmd[__RSP.cmd]( w0, w1 );
         w0 = ((uint32_t*)gfx_info.RDRAM)[addr++];
         w1 = ((uint32_t*)gfx_info.RDRAM)[addr++];
         __RSP.cmd = _SHIFTR( w0, 24, 8 );
         if (__RSP.cmd == 0xE4 || __RSP.cmd == 0xE5)
         {
            __RDP.w2 = ((uint32_t*)gfx_info.RDRAM)[addr++];
            __RDP.w3 = ((uint32_t*)gfx_info.RDRAM)[addr++];
         }
      }
      __RSP.bLLE = false;
   }
}

static
void Turbo3D_LoadGlobState(uint32_t pgstate)
{
   uint32_t s;
	const uint32_t addr = RSP_SegmentToPhysical(pgstate);
	struct T3DGlobState *gstate = (struct T3DGlobState*)&gfx_info.RDRAM[addr];
	const uint32_t w0 = gstate->othermode0;
	const uint32_t w1 = gstate->othermode1;
	gln64gDPSetOtherMode( _SHIFTR( w0, 0, 24 ),	// mode0
					 w1 );					// mode1

	for (s = 0; s < 16; ++s)
		gln64gSPSegment(s, gstate->segBases[s] & 0x00FFFFFF);

	gSPViewport(pgstate + 80);

	Turbo3D_ProcessRDP(gstate->rdpCmds);
}

static
void Turbo3D_LoadObject(uint32_t pstate, uint32_t pvtx, uint32_t ptri)
{
   uint32_t w0, w1;
	uint32_t addr           = RSP_SegmentToPhysical(pstate);
	struct T3DState *ostate = (struct T3DState*)&gfx_info.RDRAM[addr];
	const uint32_t tile = (ostate->textureState)&7;
	gSP.texture.tile    = tile;
	gSP.textureTile[0]  = &gDP.tiles[tile];
	gSP.textureTile[1]  = &gDP.tiles[(tile + 1) & 7];
	gSP.texture.scales  = 1.0f;
	gSP.texture.scalet  = 1.0f;

	w0   = ostate->othermode0;
	w1   = ostate->othermode1;
	gln64gDPSetOtherMode( _SHIFTR( w0, 0, 24 ),	// mode0
					 w1 );					// mode1

	gln64gSPSetGeometryMode(ostate->renderState);

	if ((ostate->flag&1) == 0) //load matrix
		gSPForceMatrix(pstate + sizeof(struct T3DState));

	gln64gSPClearGeometryMode(G_LIGHTING);
   gln64gSPClearGeometryMode(G_FOG);
	gln64gSPSetGeometryMode(G_SHADING_SMOOTH);
  
	if (pvtx != 0) //load vtx
		gln64gSPVertex(pvtx, ostate->vtxCount, ostate->vtxV0);

	Turbo3D_ProcessRDP(ostate->rdpCmds);

	if (ptri != 0)
   {
      unsigned t;
		addr = RSP_SegmentToPhysical(ptri);

		for (t = 0; t < ostate->triCount; ++t)
      {
			struct T3DTriN * tri = (struct T3DTriN*)&gfx_info.RDRAM[addr];
			addr += 4;
			gln64gSPTriangle(tri->v0, tri->v1, tri->v2);
		}
      OGL_DrawTriangles();
	}
}

void RunTurbo3D(void)
{
	do
   {
      uint32_t          addr = __RSP.PC[__RSP.PCi] >> 2;
      const uint32_t pgstate = ((uint32_t*)gfx_info.RDRAM)[addr++];
      uint32_t        pstate = ((uint32_t*)gfx_info.RDRAM)[addr++];
      const uint32_t    pvtx = ((uint32_t*)gfx_info.RDRAM)[addr++];
      const uint32_t    ptri = ((uint32_t*)gfx_info.RDRAM)[addr];

      if (pstate == 0)
      {
         __RSP.halt = 1;
         break;
      }
      if (pgstate != 0)
         Turbo3D_LoadGlobState(pgstate);
      Turbo3D_LoadObject(pstate, pvtx, ptri);

      /* Go to the next instruction */
      __RSP.PC[__RSP.PCi] += 16;
   }while (1);
}
