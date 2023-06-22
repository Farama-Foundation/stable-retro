#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <stdint.h>

#include "DepthBuffer.h"
#include "Textures.h"

struct FrameBuffer
{
	struct FrameBuffer *higher, *lower;

	CachedTexture *texture;

	uint32_t m_startAddress, m_endAddress;
	uint32_t m_size, m_width, m_height, m_fillcolor, m_validityChecked;
	float m_scaleX, m_scaleY;

	bool m_copiedToRdram;
	bool m_cleared;
	bool m_changed;
	bool m_cfb;
	bool m_isDepthBuffer;
	bool m_isPauseScreen;
	bool m_isOBScreen;
	bool m_needHeightCorrection;
	bool m_postProcessed;

	GLuint m_FBO;
	struct gDPTile *m_pLoadTile;
	CachedTexture *m_pTexture;
	struct DepthBuffer *m_pDepthBuffer;
	// multisampling
	CachedTexture *m_pResolveTexture;
	GLuint m_resolveFBO;
	bool m_resolved;
};

struct FrameBufferInfo
{
	struct FrameBuffer *top, *bottom, *current;
	int numBuffers;
};

extern struct FrameBufferInfo frameBuffer;

void FrameBuffer_Init(void);
void FrameBuffer_Destroy(void);
void FrameBuffer_CopyToRDRAM( uint32_t _address );
void FrameBuffer_CopyFromRDRAM( uint32_t _address, bool _bUseAlpha );
void FrameBuffer_CopyDepthBuffer( uint32_t _address );
void FrameBuffer_ActivateBufferTexture( int16_t t, struct FrameBuffer *buffer);
void FrameBuffer_ActivateBufferTextureBG(int16_t t, struct FrameBuffer *buffer);

void FrameBuffer_SaveBuffer( uint32_t address, uint16_t format, uint16_t size, uint16_t width, uint16_t height, bool unknown );
void FrameBuffer_RenderBuffer( uint32_t address );
void FrameBuffer_RestoreBuffer( uint32_t address, uint16_t size, uint16_t width );
void FrameBuffer_RemoveBuffer( uint32_t address );
struct FrameBuffer *FrameBuffer_FindBuffer( uint32_t address );
struct FrameBuffer *FrameBuffer_GetCurrent(void);

#endif
