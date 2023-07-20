/* Copyright (c) 2013-2017 Jeffrey Pfau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <mgba/internal/gba/renderers/proxy.h>

#include <mgba/core/cache-set.h>
#include <mgba/internal/gba/gba.h>
#include <mgba/internal/gba/io.h>

static void GBAVideoProxyRendererInit(struct GBAVideoRenderer* renderer);
static void GBAVideoProxyRendererReset(struct GBAVideoRenderer* renderer);
static void GBAVideoProxyRendererDeinit(struct GBAVideoRenderer* renderer);
static uint16_t GBAVideoProxyRendererWriteVideoRegister(struct GBAVideoRenderer* renderer, uint32_t address, uint16_t value);
static void GBAVideoProxyRendererWriteVRAM(struct GBAVideoRenderer* renderer, uint32_t address);
static void GBAVideoProxyRendererWritePalette(struct GBAVideoRenderer* renderer, uint32_t address, uint16_t value);
static void GBAVideoProxyRendererWriteOAM(struct GBAVideoRenderer* renderer, uint32_t oam);
static void GBAVideoProxyRendererDrawScanline(struct GBAVideoRenderer* renderer, int y);
static void GBAVideoProxyRendererFinishFrame(struct GBAVideoRenderer* renderer);
static void GBAVideoProxyRendererGetPixels(struct GBAVideoRenderer* renderer, size_t* stride, const void** pixels);
static void GBAVideoProxyRendererPutPixels(struct GBAVideoRenderer* renderer, size_t stride, const void* pixels);

static bool _parsePacket(struct mVideoLogger* logger, const struct mVideoLoggerDirtyInfo* packet);
static uint16_t* _vramBlock(struct mVideoLogger* logger, uint32_t address);

void GBAVideoProxyRendererCreate(struct GBAVideoProxyRenderer* renderer, struct GBAVideoRenderer* backend) {
	renderer->d.init = GBAVideoProxyRendererInit;
	renderer->d.reset = GBAVideoProxyRendererReset;
	renderer->d.deinit = GBAVideoProxyRendererDeinit;
	renderer->d.writeVideoRegister = GBAVideoProxyRendererWriteVideoRegister;
	renderer->d.writeVRAM = GBAVideoProxyRendererWriteVRAM;
	renderer->d.writeOAM = GBAVideoProxyRendererWriteOAM;
	renderer->d.writePalette = GBAVideoProxyRendererWritePalette;
	renderer->d.drawScanline = GBAVideoProxyRendererDrawScanline;
	renderer->d.finishFrame = GBAVideoProxyRendererFinishFrame;
	renderer->d.getPixels = GBAVideoProxyRendererGetPixels;
	renderer->d.putPixels = GBAVideoProxyRendererPutPixels;

	renderer->d.disableBG[0] = false;
	renderer->d.disableBG[1] = false;
	renderer->d.disableBG[2] = false;
	renderer->d.disableBG[3] = false;
	renderer->d.disableOBJ = false;

	renderer->logger->context = renderer;
	renderer->logger->parsePacket = _parsePacket;
	renderer->logger->vramBlock = _vramBlock;
	renderer->logger->paletteSize = SIZE_PALETTE_RAM;
	renderer->logger->vramSize = SIZE_VRAM;
	renderer->logger->oamSize = SIZE_OAM;

	renderer->backend = backend;
}

static void _init(struct GBAVideoProxyRenderer* proxyRenderer) {
	mVideoLoggerRendererInit(proxyRenderer->logger);

	if (proxyRenderer->logger->block) {
		proxyRenderer->backend->palette = proxyRenderer->logger->palette;
		proxyRenderer->backend->vram = proxyRenderer->logger->vram;
		proxyRenderer->backend->oam = (union GBAOAM*) proxyRenderer->logger->oam;
		proxyRenderer->backend->cache = NULL;
	}
}

static void _reset(struct GBAVideoProxyRenderer* proxyRenderer) {
	memcpy(proxyRenderer->logger->oam, &proxyRenderer->d.oam->raw, SIZE_OAM);
	memcpy(proxyRenderer->logger->palette, proxyRenderer->d.palette, SIZE_PALETTE_RAM);
	memcpy(proxyRenderer->logger->vram, proxyRenderer->d.vram, SIZE_VRAM);

	mVideoLoggerRendererReset(proxyRenderer->logger);
}

void GBAVideoProxyRendererShim(struct GBAVideo* video, struct GBAVideoProxyRenderer* renderer) {
	if ((renderer->backend && video->renderer != renderer->backend) || video->renderer == &renderer->d) {
		return;
	}
	renderer->backend = video->renderer;
	video->renderer = &renderer->d;
	renderer->d.cache = renderer->backend->cache;
	renderer->d.palette = video->palette;
	renderer->d.vram = video->vram;
	renderer->d.oam = &video->oam;
	_init(renderer);
	_reset(renderer);
}

void GBAVideoProxyRendererUnshim(struct GBAVideo* video, struct GBAVideoProxyRenderer* renderer) {
	if (video->renderer != &renderer->d) {
		return;
	}
	renderer->backend->cache = video->renderer->cache;
	video->renderer = renderer->backend;
	renderer->backend->palette = video->palette;
	renderer->backend->vram = video->vram;
	renderer->backend->oam = &video->oam;

	mVideoLoggerRendererDeinit(renderer->logger);
}

void GBAVideoProxyRendererInit(struct GBAVideoRenderer* renderer) {
	struct GBAVideoProxyRenderer* proxyRenderer = (struct GBAVideoProxyRenderer*) renderer;

	_init(proxyRenderer);

	proxyRenderer->backend->init(proxyRenderer->backend);
}

void GBAVideoProxyRendererReset(struct GBAVideoRenderer* renderer) {
	struct GBAVideoProxyRenderer* proxyRenderer = (struct GBAVideoProxyRenderer*) renderer;

	_reset(proxyRenderer);

	proxyRenderer->backend->reset(proxyRenderer->backend);
}

void GBAVideoProxyRendererDeinit(struct GBAVideoRenderer* renderer) {
	struct GBAVideoProxyRenderer* proxyRenderer = (struct GBAVideoProxyRenderer*) renderer;

	proxyRenderer->backend->deinit(proxyRenderer->backend);

	mVideoLoggerRendererDeinit(proxyRenderer->logger);
}

static bool _parsePacket(struct mVideoLogger* logger, const struct mVideoLoggerDirtyInfo* item) {
	struct GBAVideoProxyRenderer* proxyRenderer = logger->context;
	switch (item->type) {
	case DIRTY_REGISTER:
		proxyRenderer->backend->writeVideoRegister(proxyRenderer->backend, item->address, item->value);
		break;
	case DIRTY_PALETTE:
		if (item->address < SIZE_PALETTE_RAM) {
			logger->palette[item->address >> 1] = item->value;
			proxyRenderer->backend->writePalette(proxyRenderer->backend, item->address, item->value);
		}
		break;
	case DIRTY_OAM:
		if (item->address < SIZE_PALETTE_RAM) {
			logger->oam[item->address] = item->value;
			proxyRenderer->backend->writeOAM(proxyRenderer->backend, item->address);
		}
		break;
	case DIRTY_VRAM:
		if (item->address <= SIZE_VRAM - 0x1000) {
			logger->readData(logger, &logger->vram[item->address >> 1], 0x1000, true);
			proxyRenderer->backend->writeVRAM(proxyRenderer->backend, item->address);
		}
		break;
	case DIRTY_SCANLINE:
		if (item->address < VIDEO_VERTICAL_PIXELS) {
			proxyRenderer->backend->drawScanline(proxyRenderer->backend, item->address);
		}
		break;
	case DIRTY_FRAME:
		proxyRenderer->backend->finishFrame(proxyRenderer->backend);
		break;
	case DIRTY_FLUSH:
		return false;
	default:
		return false;
	}
	return true;
}

static uint16_t* _vramBlock(struct mVideoLogger* logger, uint32_t address) {
	struct GBAVideoProxyRenderer* proxyRenderer = logger->context;
	return &proxyRenderer->d.vram[address >> 1];
}

uint16_t GBAVideoProxyRendererWriteVideoRegister(struct GBAVideoRenderer* renderer, uint32_t address, uint16_t value) {
	struct GBAVideoProxyRenderer* proxyRenderer = (struct GBAVideoProxyRenderer*) renderer;
	switch (address) {
	case REG_DISPCNT:
		value &= 0xFFF7;
		break;
	case REG_BG0CNT:
	case REG_BG1CNT:
		value &= 0xDFFF;
		break;
	case REG_BG2CNT:
	case REG_BG3CNT:
		value &= 0xFFFF;
		break;
	case REG_BG0HOFS:
	case REG_BG0VOFS:
	case REG_BG1HOFS:
	case REG_BG1VOFS:
	case REG_BG2HOFS:
	case REG_BG2VOFS:
	case REG_BG3HOFS:
	case REG_BG3VOFS:
		value &= 0x01FF;
		break;
	}
	if (address > REG_BLDY) {
		return value;
	}

	mVideoLoggerRendererWriteVideoRegister(proxyRenderer->logger, address, value);
	if (!proxyRenderer->logger->block) {
		proxyRenderer->backend->writeVideoRegister(proxyRenderer->backend, address, value);
	}
	return value;
}

void GBAVideoProxyRendererWriteVRAM(struct GBAVideoRenderer* renderer, uint32_t address) {
	struct GBAVideoProxyRenderer* proxyRenderer = (struct GBAVideoProxyRenderer*) renderer;
	mVideoLoggerRendererWriteVRAM(proxyRenderer->logger, address);
	if (!proxyRenderer->logger->block) {
		proxyRenderer->backend->writeVRAM(proxyRenderer->backend, address);
	}
	if (renderer->cache) {
		mCacheSetWriteVRAM(renderer->cache, address);
	}
}

void GBAVideoProxyRendererWritePalette(struct GBAVideoRenderer* renderer, uint32_t address, uint16_t value) {
	struct GBAVideoProxyRenderer* proxyRenderer = (struct GBAVideoProxyRenderer*) renderer;
	mVideoLoggerRendererWritePalette(proxyRenderer->logger, address, value);
	if (!proxyRenderer->logger->block) {
		proxyRenderer->backend->writePalette(proxyRenderer->backend, address, value);
	}
	if (renderer->cache) {
		mCacheSetWritePalette(renderer->cache, address >> 1, mColorFrom555(value));
	}
}

void GBAVideoProxyRendererWriteOAM(struct GBAVideoRenderer* renderer, uint32_t oam) {
	struct GBAVideoProxyRenderer* proxyRenderer = (struct GBAVideoProxyRenderer*) renderer;
	if (!proxyRenderer->logger->block) {
		proxyRenderer->backend->writeOAM(proxyRenderer->backend, oam);
	}
	mVideoLoggerRendererWriteOAM(proxyRenderer->logger, oam, proxyRenderer->d.oam->raw[oam]);
}

void GBAVideoProxyRendererDrawScanline(struct GBAVideoRenderer* renderer, int y) {
	struct GBAVideoProxyRenderer* proxyRenderer = (struct GBAVideoProxyRenderer*) renderer;
	if (!proxyRenderer->logger->block) {
		proxyRenderer->backend->drawScanline(proxyRenderer->backend, y);
	}
	mVideoLoggerRendererDrawScanline(proxyRenderer->logger, y);
	if (proxyRenderer->logger->block && proxyRenderer->logger->wake) {
		proxyRenderer->logger->wake(proxyRenderer->logger, y);
	}
}

void GBAVideoProxyRendererFinishFrame(struct GBAVideoRenderer* renderer) {
	struct GBAVideoProxyRenderer* proxyRenderer = (struct GBAVideoProxyRenderer*) renderer;
	if (proxyRenderer->logger->block && proxyRenderer->logger->wait) {
		proxyRenderer->logger->lock(proxyRenderer->logger);
		proxyRenderer->logger->wait(proxyRenderer->logger);
	}
	proxyRenderer->backend->finishFrame(proxyRenderer->backend);
	mVideoLoggerRendererFinishFrame(proxyRenderer->logger);
	mVideoLoggerRendererFlush(proxyRenderer->logger);
	if (proxyRenderer->logger->block && proxyRenderer->logger->wait) {
		proxyRenderer->logger->unlock(proxyRenderer->logger);
	}
}

static void GBAVideoProxyRendererGetPixels(struct GBAVideoRenderer* renderer, size_t* stride, const void** pixels) {
	struct GBAVideoProxyRenderer* proxyRenderer = (struct GBAVideoProxyRenderer*) renderer;
	if (proxyRenderer->logger->block && proxyRenderer->logger->wait) {
		proxyRenderer->logger->lock(proxyRenderer->logger);
		// Insert an extra item into the queue to make sure it gets flushed
		mVideoLoggerRendererFlush(proxyRenderer->logger);
		proxyRenderer->logger->wait(proxyRenderer->logger);
	}
	proxyRenderer->backend->getPixels(proxyRenderer->backend, stride, pixels);
	if (proxyRenderer->logger->block && proxyRenderer->logger->wait) {
		proxyRenderer->logger->unlock(proxyRenderer->logger);
	}
}

static void GBAVideoProxyRendererPutPixels(struct GBAVideoRenderer* renderer, size_t stride, const void* pixels) {
	struct GBAVideoProxyRenderer* proxyRenderer = (struct GBAVideoProxyRenderer*) renderer;
	if (proxyRenderer->logger->block && proxyRenderer->logger->wait) {
		proxyRenderer->logger->lock(proxyRenderer->logger);
		// Insert an extra item into the queue to make sure it gets flushed
		mVideoLoggerRendererFlush(proxyRenderer->logger);
		proxyRenderer->logger->wait(proxyRenderer->logger);
	}
	proxyRenderer->backend->putPixels(proxyRenderer->backend, stride, pixels);
	if (proxyRenderer->logger->block && proxyRenderer->logger->wait) {
		proxyRenderer->logger->unlock(proxyRenderer->logger);
	}
}
