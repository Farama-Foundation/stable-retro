/* Copyright (c) 2013-2015 Jeffrey Pfau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <mgba/core/blip_buf.h>
#include <mgba/core/core.h>
#include <mgba/core/serialize.h>
#ifdef M_CORE_GBA
#include <mgba/internal/gba/gba.h>
#include <mgba/internal/gba/input.h>
#include <mgba/internal/gba/video.h>
#endif
#ifdef M_CORE_GB
#include <mgba/internal/gb/gb.h>
#endif
#include "feature/gui/gui-runner.h"
#include <mgba-util/gui.h>
#include <mgba-util/gui/file-select.h>
#include <mgba-util/gui/font.h>
#include <mgba-util/gui/menu.h>
#include <mgba-util/memory.h>

#include <mgba-util/platform/3ds/3ds-vfs.h>
#include <mgba-util/threading.h>
#include "ctr-gpu.h"

#include <3ds.h>
#include <3ds/gpu/gx.h>

mLOG_DECLARE_CATEGORY(GUI_3DS);
mLOG_DEFINE_CATEGORY(GUI_3DS, "3DS", "gui.3ds");

static enum ScreenMode {
	SM_PA_BOTTOM,
	SM_AF_BOTTOM,
	SM_SF_BOTTOM,
	SM_PA_TOP,
	SM_AF_TOP,
	SM_SF_TOP,
	SM_MAX
} screenMode = SM_PA_TOP;

static enum FilterMode {
	FM_NEAREST,
	FM_LINEAR_1x,
	FM_LINEAR_2x,
	FM_MAX
} filterMode = FM_LINEAR_2x;

static enum DarkenMode {
	DM_NATIVE,
	DM_MULT,
	DM_MULT_SCALE,
	DM_MULT_SCALE_BIAS,
	DM_MAX
} darkenMode = DM_NATIVE;

#define _3DS_INPUT 0x3344534B

#define AUDIO_SAMPLES 384
#define AUDIO_SAMPLE_BUFFER (AUDIO_SAMPLES * 16)
#define DSP_BUFFERS 4

static struct m3DSRotationSource {
	struct mRotationSource d;
	accelVector accel;
	angularRate gyro;
} rotation;

static struct m3DSImageSource {
	struct mImageSource d;
	Handle handles[2];
	u32 bufferSize;
	u32 transferSize;
	void* buffer;
	unsigned cam;
} camera;

static enum {
	NO_SOUND,
	DSP_SUPPORTED
} hasSound;

// TODO: Move into context
static void* outputBuffer;
static struct mAVStream stream;
static int16_t* audioLeft = 0;
static int16_t* audioRight = 0;
static size_t audioPos = 0;
static C3D_Tex outputTexture;
static ndspWaveBuf dspBuffer[DSP_BUFFERS];
static int bufferId = 0;
static bool frameLimiter = true;
static u64 tickCounter;

static C3D_RenderTarget* topScreen[2];
static C3D_RenderTarget* bottomScreen[2];
static int doubleBuffer = 0;
static bool frameStarted = false;

static C3D_RenderTarget* upscaleBuffer;
static C3D_Tex upscaleBufferTex;

static aptHookCookie cookie;

extern bool allocateRomBuffer(void);

static bool _initGpu(void) {
	if (!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE)) {
		return false;
	}

	topScreen[0] = C3D_RenderTargetCreate(240, 400, GPU_RB_RGB8, 0);
	topScreen[1] = C3D_RenderTargetCreate(240, 400, GPU_RB_RGB8, 0);
	bottomScreen[0] = C3D_RenderTargetCreate(240, 320, GPU_RB_RGB8, 0);
	bottomScreen[1] = C3D_RenderTargetCreate(240, 320, GPU_RB_RGB8, 0);
	if (!topScreen[0] || !topScreen[1] || !bottomScreen[0] || !bottomScreen[1]) {
		return false;
	}

	if (!C3D_TexInitVRAM(&upscaleBufferTex, 512, 512, GPU_RB_RGB8)) {
		return false;
	}
	upscaleBuffer = C3D_RenderTargetCreateFromTex(&upscaleBufferTex, GPU_TEXFACE_2D, 0, 0);
	if (!upscaleBuffer) {
		return false;
	}

	C3D_RenderTargetSetClear(upscaleBuffer, C3D_CLEAR_COLOR, 0, 0);

	return ctrInitGpu();
}

static void _cleanup(void) {
	ctrDeinitGpu();

	if (outputBuffer) {
		linearFree(outputBuffer);
	}

	C3D_RenderTargetDelete(topScreen[0]);
	C3D_RenderTargetDelete(topScreen[1]);
	C3D_RenderTargetDelete(bottomScreen[0]);
	C3D_RenderTargetDelete(bottomScreen[1]);
	C3D_RenderTargetDelete(upscaleBuffer);
	C3D_TexDelete(&upscaleBufferTex);
	C3D_TexDelete(&outputTexture);
	C3D_Fini();

	gfxExit();

	if (hasSound != NO_SOUND) {
		linearFree(audioLeft);
	}

	if (hasSound == DSP_SUPPORTED) {
		ndspExit();
	}

	camExit();
	ndspExit();
	ptmuExit();
}

static void _aptHook(APT_HookType hook, void* user) {
	UNUSED(user);
	switch (hook) {
	case APTHOOK_ONEXIT:
		_cleanup();
		exit(0);
		break;
	default:
		break;
	}
}

static void _map3DSKey(struct mInputMap* map, int ctrKey, enum GBAKey key) {
	mInputBindKey(map, _3DS_INPUT, __builtin_ctz(ctrKey), key);
}

static void _postAudioBuffer(struct mAVStream* stream, blip_t* left, blip_t* right);

static void _drawStart(void) {
}

static void _frameStart(void) {
	if (frameStarted) {
		return;
	}
	frameStarted = true;
	u8 flags = 0;
	if (!frameLimiter) {
		if (tickCounter + 4481000 > svcGetSystemTick()) {
			flags = C3D_FRAME_NONBLOCK;
		} else {
			tickCounter = svcGetSystemTick();
		}
	}
	C3D_FrameBegin(flags);
	// Mark both buffers used to make sure they get cleared
	C3D_FrameDrawOn(topScreen[doubleBuffer]);
	C3D_FrameDrawOn(bottomScreen[doubleBuffer]);
	ctrStartFrame();
}

static void _drawEnd(void) {
	if (!frameStarted) {
		return;
	}
	ctrEndFrame();
	C3D_RenderTargetSetOutput(topScreen[doubleBuffer], GFX_TOP, GFX_LEFT, GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGB8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8));
	C3D_RenderTargetSetOutput(bottomScreen[doubleBuffer], GFX_BOTTOM, GFX_LEFT, GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGB8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8));
	C3D_FrameEnd(GX_CMDLIST_FLUSH);
	frameStarted = false;

	doubleBuffer ^= 1;
	C3D_FrameBufClear(&bottomScreen[doubleBuffer]->frameBuf, C3D_CLEAR_COLOR, 0, 0);
	C3D_FrameBufClear(&topScreen[doubleBuffer]->frameBuf, C3D_CLEAR_COLOR, 0, 0);
}

static int _batteryState(void) {
	u8 charge;
	u8 adapter;
	PTMU_GetBatteryLevel(&charge);
	PTMU_GetBatteryChargeState(&adapter);
	int state = 0;
	if (adapter) {
		state |= BATTERY_CHARGING;
	}
	if (charge > 0) {
		--charge;
	}
	return state | charge;
}

static void _guiPrepare(void) {
	_frameStart();
	C3D_FrameDrawOn(bottomScreen[doubleBuffer]);
	ctrSetViewportSize(320, 240, true);
}

static void _guiFinish(void) {
	ctrFlushBatch();
}

static void _resetCamera(struct m3DSImageSource* imageSource) {
	if (!imageSource->cam) {
		return;
	}
	CAMU_SetSize(imageSource->cam, SIZE_QCIF, CONTEXT_A);
	CAMU_SetOutputFormat(imageSource->cam, OUTPUT_RGB_565, CONTEXT_A);
	CAMU_SetFrameRate(imageSource->cam, FRAME_RATE_30);
	CAMU_FlipImage(imageSource->cam, FLIP_NONE, CONTEXT_A);

	CAMU_SetNoiseFilter(imageSource->cam, true);
	CAMU_SetAutoExposure(imageSource->cam, false);
	CAMU_SetAutoWhiteBalance(imageSource->cam, false);
}

static void _setup(struct mGUIRunner* runner) {
	uint8_t mask;
	if (R_SUCCEEDED(svcGetProcessAffinityMask(&mask, CUR_PROCESS_HANDLE, 4)) && mask >= 4) {
		mCoreConfigSetDefaultIntValue(&runner->config, "threadedVideo", 1);
		mCoreLoadForeignConfig(runner->core, &runner->config);
	}

	runner->core->setPeripheral(runner->core, mPERIPH_ROTATION, &rotation.d);
	runner->core->setPeripheral(runner->core, mPERIPH_IMAGE_SOURCE, &camera.d);
	if (hasSound != NO_SOUND) {
		runner->core->setAVStream(runner->core, &stream);
	}

	_map3DSKey(&runner->core->inputMap, KEY_A, GBA_KEY_A);
	_map3DSKey(&runner->core->inputMap, KEY_B, GBA_KEY_B);
	_map3DSKey(&runner->core->inputMap, KEY_START, GBA_KEY_START);
	_map3DSKey(&runner->core->inputMap, KEY_SELECT, GBA_KEY_SELECT);
	_map3DSKey(&runner->core->inputMap, KEY_UP, GBA_KEY_UP);
	_map3DSKey(&runner->core->inputMap, KEY_DOWN, GBA_KEY_DOWN);
	_map3DSKey(&runner->core->inputMap, KEY_LEFT, GBA_KEY_LEFT);
	_map3DSKey(&runner->core->inputMap, KEY_RIGHT, GBA_KEY_RIGHT);
	_map3DSKey(&runner->core->inputMap, KEY_L, GBA_KEY_L);
	_map3DSKey(&runner->core->inputMap, KEY_R, GBA_KEY_R);

	outputBuffer = linearMemAlign(256 * 224 * 2, 0x80);
	runner->core->setVideoBuffer(runner->core, outputBuffer, 256);

	unsigned mode;
	if (mCoreConfigGetUIntValue(&runner->config, "screenMode", &mode) && mode < SM_MAX) {
		screenMode = mode;
	}
	if (mCoreConfigGetUIntValue(&runner->config, "filterMode", &mode) && mode < FM_MAX) {
		filterMode = mode;
		if (filterMode == FM_NEAREST) {
			C3D_TexSetFilter(&upscaleBufferTex, GPU_NEAREST, GPU_NEAREST);
		} else {
			C3D_TexSetFilter(&upscaleBufferTex, GPU_LINEAR, GPU_LINEAR);
		}
	}
	if (mCoreConfigGetUIntValue(&runner->config, "darkenMode", &mode) && mode < DM_MAX) {
		darkenMode = mode;
	}
	frameLimiter = true;

	runner->core->setAudioBufferSize(runner->core, AUDIO_SAMPLES);
}

static void _gameLoaded(struct mGUIRunner* runner) {
	switch (runner->core->platform(runner->core)) {
#ifdef M_CORE_GBA
		// TODO: Move these to callbacks
	case PLATFORM_GBA:
		if (((struct GBA*) runner->core->board)->memory.hw.devices & HW_TILT) {
			HIDUSER_EnableAccelerometer();
		}
		if (((struct GBA*) runner->core->board)->memory.hw.devices & HW_GYRO) {
			HIDUSER_EnableGyroscope();
		}
		break;
#endif
#ifdef M_CORE_GB
	case PLATFORM_GB:
		if (((struct GB*) runner->core->board)->memory.mbcType == GB_MBC7) {
			HIDUSER_EnableAccelerometer();
		}
		break;
#endif
	default:
		break;
	}
	osSetSpeedupEnable(true);

	double ratio = GBAAudioCalculateRatio(1, 59.8260982880808, 1);
	blip_set_rates(runner->core->getAudioChannel(runner->core, 0), runner->core->frequency(runner->core), 32768 * ratio);
	blip_set_rates(runner->core->getAudioChannel(runner->core, 1), runner->core->frequency(runner->core), 32768 * ratio);
	if (hasSound != NO_SOUND) {
		audioPos = 0;
	}
	if (hasSound == DSP_SUPPORTED) {
		memset(audioLeft, 0, AUDIO_SAMPLE_BUFFER * 2 * sizeof(int16_t));
	}
	unsigned mode;
	if (mCoreConfigGetUIntValue(&runner->config, "screenMode", &mode) && mode < SM_MAX) {
		screenMode = mode;
	}
	if (mCoreConfigGetUIntValue(&runner->config, "filterMode", &mode) && mode < FM_MAX) {
		filterMode = mode;
		if (filterMode == FM_NEAREST) {
			C3D_TexSetFilter(&upscaleBufferTex, GPU_NEAREST, GPU_NEAREST);
		} else {
			C3D_TexSetFilter(&upscaleBufferTex, GPU_LINEAR, GPU_LINEAR);
		}
	}
	if (mCoreConfigGetUIntValue(&runner->config, "darkenMode", &mode) && mode < DM_MAX) {
		darkenMode = mode;
	}
	if (mCoreConfigGetUIntValue(&runner->config, "camera", &mode)) {
		switch (mode) {
		case 0:
		default:
			mode = SELECT_NONE;
			break;
		case 1:
			mode = SELECT_IN1;
			break;
		case 2:
			mode = SELECT_OUT1;
			break;
		}
		if (mode != camera.cam) {
			camera.cam = mode;
			if (camera.buffer) {
				_resetCamera(&camera);
				CAMU_Activate(camera.cam);
			}
		}
	}
}

static void _gameUnloaded(struct mGUIRunner* runner) {
	osSetSpeedupEnable(false);
	frameLimiter = true;

	switch (runner->core->platform(runner->core)) {
#ifdef M_CORE_GBA
		// TODO: Move these to callbacks
	case PLATFORM_GBA:
		if (((struct GBA*) runner->core->board)->memory.hw.devices & HW_TILT) {
			HIDUSER_DisableAccelerometer();
		}
		if (((struct GBA*) runner->core->board)->memory.hw.devices & HW_GYRO) {
			HIDUSER_DisableGyroscope();
		}
		break;
#endif
#ifdef M_CORE_GB
	case PLATFORM_GB:
		if (((struct GB*) runner->core->board)->memory.mbcType == GB_MBC7) {
			HIDUSER_DisableAccelerometer();
		}
		break;
#endif
	default:
		break;
	}
}

static void _drawTex(struct mCore* core, bool faded) {
	_frameStart();
	unsigned screen_w, screen_h;
	switch (screenMode) {
	case SM_PA_BOTTOM:
		C3D_FrameDrawOn(bottomScreen[doubleBuffer]);
		screen_w = 320;
		screen_h = 240;
		break;
	case SM_PA_TOP:
		C3D_FrameDrawOn(topScreen[doubleBuffer]);
		screen_w = 400;
		screen_h = 240;
		break;
	default:
		C3D_FrameDrawOn(upscaleBuffer);
		screen_w = 512;
		screen_h = 512;
		break;
	}

	unsigned corew, coreh;
	core->desiredVideoDimensions(core, &corew, &coreh);

	int w = corew;
	int h = coreh;
	// Get greatest common divisor
	while (w != 0) {
		int temp = h % w;
		h = w;
		w = temp;
	}
	int gcd = h;
	unsigned aspectw = corew / gcd;
	unsigned aspecth = coreh / gcd;
	int x = 0;
	int y = 0;

	switch (screenMode) {
	case SM_PA_TOP:
	case SM_PA_BOTTOM:
		w = corew;
		h = coreh;
		x = (screen_w - w) / 2;
		y = (screen_h - h) / 2;
		ctrSetViewportSize(screen_w, screen_h, true);
		break;
	case SM_AF_TOP:
	case SM_AF_BOTTOM:
	case SM_SF_TOP:
	case SM_SF_BOTTOM:
	default:
		if (filterMode == FM_LINEAR_1x) {
			w = corew;
			h = coreh;
		} else {
			w = corew * 2;
			h = coreh * 2;
		}
		ctrSetViewportSize(screen_w, screen_h, false);
		break;
	}

	ctrActivateTexture(&outputTexture);
	u32 color;
	if (!faded) {
		color = 0xFFFFFFFF;
		switch (darkenMode) {
		case DM_NATIVE:
		case DM_MAX:
			break;
		case DM_MULT_SCALE_BIAS:
			ctrTextureBias(0x070707);
			// Fall through
		case DM_MULT_SCALE:
			color = 0xFF707070;
			// Fall through
		case DM_MULT:
			ctrTextureMultiply();
			break;
		}
	} else {
		color = 0xFF484848;
		switch (darkenMode) {
		case DM_NATIVE:
		case DM_MAX:
			break;
		case DM_MULT_SCALE_BIAS:
			ctrTextureBias(0x030303);
			// Fall through
		case DM_MULT_SCALE:
			color = 0xFF303030;
			// Fall through
		case DM_MULT:
			ctrTextureMultiply();
			break;
		}

	}
	ctrAddRectEx(color, x, y, w, h, 0, 0, corew, coreh, 0);
	ctrFlushBatch();

	corew = w;
	coreh = h;
	screen_h = 240;
	if (screenMode < SM_PA_TOP) {
		C3D_FrameDrawOn(bottomScreen[doubleBuffer]);
		screen_w = 320;
	} else {
		C3D_FrameDrawOn(topScreen[doubleBuffer]);
		screen_w = 400;
	}
	ctrSetViewportSize(screen_w, screen_h, true);

	switch (screenMode) {
	default:
		return;
	case SM_AF_TOP:
	case SM_AF_BOTTOM:
		w = screen_w / aspectw;
		h = screen_h / aspecth;
		if (w * aspecth > screen_h) {
			w = aspectw * h;
			h = aspecth * h;
		} else {
			h = aspecth * w;
			w = aspectw * w;
		}
		break;
	case SM_SF_TOP:
	case SM_SF_BOTTOM:
		w = screen_w;
		h = screen_h;
		break;
	}

	x = (screen_w - w) / 2;
	y = (screen_h - h) / 2;
	ctrActivateTexture(&upscaleBufferTex);
	ctrAddRectEx(0xFFFFFFFF, x, y, w, h, 0, 0, corew, coreh, 0);
	ctrFlushBatch();
}

static void _drawFrame(struct mGUIRunner* runner, bool faded) {
	UNUSED(runner);

	C3D_Tex* tex = &outputTexture;

	GSPGPU_FlushDataCache(outputBuffer, 256 * VIDEO_VERTICAL_PIXELS * 2);
	C3D_SafeDisplayTransfer(
			outputBuffer, GX_BUFFER_DIM(256, VIDEO_VERTICAL_PIXELS),
			tex->data, GX_BUFFER_DIM(256, 256),
			GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGB565) |
				GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB565) |
				GX_TRANSFER_OUT_TILED(1) | GX_TRANSFER_FLIP_VERT(1));

	if (hasSound == NO_SOUND) {
		blip_clear(runner->core->getAudioChannel(runner->core, 0));
		blip_clear(runner->core->getAudioChannel(runner->core, 1));
	}

	gspWaitForPPF();
	_drawTex(runner->core, faded);
}

static void _drawScreenshot(struct mGUIRunner* runner, const color_t* pixels, unsigned width, unsigned height, bool faded) {

	C3D_Tex* tex = &outputTexture;

	color_t* newPixels = linearMemAlign(256 * height * sizeof(color_t), 0x100);

	unsigned y;
	for (y = 0; y < height; ++y) {
		memcpy(&newPixels[y * 256], &pixels[y * width], width * sizeof(color_t));
		memset(&newPixels[y * 256 + width], 0, (256 - width) * sizeof(color_t));
	}

	GSPGPU_FlushDataCache(newPixels, 256 * height * sizeof(u32));
	C3D_SafeDisplayTransfer(
			(u32*) newPixels, GX_BUFFER_DIM(256, height),
			tex->data, GX_BUFFER_DIM(256, 256),
			GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGB565) |
				GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB565) |
				GX_TRANSFER_OUT_TILED(1) | GX_TRANSFER_FLIP_VERT(1));
	gspWaitForPPF();
	linearFree(newPixels);

	_drawTex(runner->core, faded);
}

static uint16_t _pollGameInput(struct mGUIRunner* runner) {
	UNUSED(runner);

	hidScanInput();
	uint32_t activeKeys = hidKeysHeld();
	uint16_t keys = mInputMapKeyBits(&runner->core->inputMap, _3DS_INPUT, activeKeys, 0);
	keys |= (activeKeys >> 24) & 0xF0;
	return keys;
}

static void _incrementScreenMode(struct mGUIRunner* runner) {
	UNUSED(runner);
	screenMode = (screenMode + 1) % SM_MAX;
	mCoreConfigSetUIntValue(&runner->config, "screenMode", screenMode);

	C3D_FrameBufClear(&bottomScreen[doubleBuffer]->frameBuf, C3D_CLEAR_COLOR, 0, 0);
	C3D_FrameBufClear(&topScreen[doubleBuffer]->frameBuf, C3D_CLEAR_COLOR, 0, 0);
}

static void _setFrameLimiter(struct mGUIRunner* runner, bool limit) {
	UNUSED(runner);
	if (frameLimiter == limit) {
		return;
	}
	frameLimiter = limit;
	tickCounter = svcGetSystemTick();
}

static bool _running(struct mGUIRunner* runner) {
	UNUSED(runner);
	return aptMainLoop();
}

static uint32_t _pollInput(const struct mInputMap* map) {
	hidScanInput();
	int activeKeys = hidKeysHeld();
	return mInputMapKeyBits(map, _3DS_INPUT, activeKeys, 0);
}

static enum GUICursorState _pollCursor(unsigned* x, unsigned* y) {
	hidScanInput();
	if (!(hidKeysHeld() & KEY_TOUCH)) {
		return GUI_CURSOR_NOT_PRESENT;
	}
	touchPosition pos;
	hidTouchRead(&pos);
	*x = pos.px;
	*y = pos.py;
	return GUI_CURSOR_DOWN;
}

static void _sampleRotation(struct mRotationSource* source) {
	struct m3DSRotationSource* rotation = (struct m3DSRotationSource*) source;
	// Work around ctrulib getting the entries wrong
	rotation->accel = *(accelVector*) &hidSharedMem[0x48];
	rotation->gyro = *(angularRate*) &hidSharedMem[0x5C];
}

static int32_t _readTiltX(struct mRotationSource* source) {
	struct m3DSRotationSource* rotation = (struct m3DSRotationSource*) source;
	return rotation->accel.x << 18L;
}

static int32_t _readTiltY(struct mRotationSource* source) {
	struct m3DSRotationSource* rotation = (struct m3DSRotationSource*) source;
	return rotation->accel.y << 18L;
}

static int32_t _readGyroZ(struct mRotationSource* source) {
	struct m3DSRotationSource* rotation = (struct m3DSRotationSource*) source;
	return rotation->gyro.y << 18L; // Yes, y
}

static void _startRequestImage(struct mImageSource* source, unsigned w, unsigned h, int colorFormats) {
	UNUSED(colorFormats);
	struct m3DSImageSource* imageSource = (struct m3DSImageSource*) source;

	_resetCamera(imageSource);

	CAMU_SetTrimming(PORT_CAM1, true);
	CAMU_SetTrimmingParamsCenter(PORT_CAM1, w, h, 176, 144);
	CAMU_GetBufferErrorInterruptEvent(&imageSource->handles[1], PORT_CAM1);

	if (imageSource->bufferSize != w * h * 2 && imageSource->buffer) {
		free(imageSource->buffer);
		imageSource->buffer = NULL;
	}
	imageSource->bufferSize = w * h * 2;
	if (!imageSource->buffer) {
		imageSource->buffer = malloc(imageSource->bufferSize);
	}
	CAMU_GetMaxBytes(&imageSource->transferSize, w, h);
	CAMU_SetTransferBytes(PORT_CAM1, imageSource->transferSize, w, h);
	CAMU_Activate(imageSource->cam);
	CAMU_ClearBuffer(PORT_CAM1);
	CAMU_StartCapture(PORT_CAM1);

	if (imageSource->cam) {
		CAMU_SetReceiving(&imageSource->handles[0], imageSource->buffer, PORT_CAM1, imageSource->bufferSize, imageSource->transferSize);
	}
}

static void _stopRequestImage(struct mImageSource* source) {
	struct m3DSImageSource* imageSource = (struct m3DSImageSource*) source;

	free(imageSource->buffer);
	imageSource->buffer = NULL;
	svcCloseHandle(imageSource->handles[0]);
	svcCloseHandle(imageSource->handles[1]);

	CAMU_StopCapture(PORT_CAM1);
	CAMU_Activate(SELECT_NONE);
}


static void _requestImage(struct mImageSource* source, const void** buffer, size_t* stride, enum mColorFormat* colorFormat) {
	struct m3DSImageSource* imageSource = (struct m3DSImageSource*) source;

	if (!imageSource->cam) {
		memset(imageSource->buffer, 0, imageSource->bufferSize);
		*buffer = imageSource->buffer;
		*stride = 128;
		*colorFormat = mCOLOR_RGB565;
		return;
	}

	s32 i;
	svcWaitSynchronizationN(&i, imageSource->handles, 2, false, U64_MAX);

	if (i == 0) {
		*buffer = imageSource->buffer;
		*stride = 128;
		*colorFormat = mCOLOR_RGB565;
	} else {
		CAMU_ClearBuffer(PORT_CAM1);
		CAMU_StartCapture(PORT_CAM1);
	}

	svcCloseHandle(imageSource->handles[0]);
	CAMU_SetReceiving(&imageSource->handles[0], imageSource->buffer, PORT_CAM1, imageSource->bufferSize, imageSource->transferSize);
}

static void _postAudioBuffer(struct mAVStream* stream, blip_t* left, blip_t* right) {
	UNUSED(stream);
	if (hasSound == DSP_SUPPORTED) {
		int startId = bufferId;
		while (dspBuffer[bufferId].status == NDSP_WBUF_QUEUED || dspBuffer[bufferId].status == NDSP_WBUF_PLAYING) {
			bufferId = (bufferId + 1) & (DSP_BUFFERS - 1);
			if (bufferId == startId) {
				blip_clear(left);
				blip_clear(right);
				return;
			}
		}
		void* tmpBuf = dspBuffer[bufferId].data_pcm16;
		memset(&dspBuffer[bufferId], 0, sizeof(dspBuffer[bufferId]));
		dspBuffer[bufferId].data_pcm16 = tmpBuf;
		dspBuffer[bufferId].nsamples = AUDIO_SAMPLES;
		blip_read_samples(left, dspBuffer[bufferId].data_pcm16, AUDIO_SAMPLES, true);
		blip_read_samples(right, dspBuffer[bufferId].data_pcm16 + 1, AUDIO_SAMPLES, true);
		DSP_FlushDataCache(dspBuffer[bufferId].data_pcm16, AUDIO_SAMPLES * 2 * sizeof(int16_t));
		ndspChnWaveBufAdd(0, &dspBuffer[bufferId]);
	}
}

int main() {
	rotation.d.sample = _sampleRotation;
	rotation.d.readTiltX = _readTiltX;
	rotation.d.readTiltY = _readTiltY;
	rotation.d.readGyroZ = _readGyroZ;

	stream.videoDimensionsChanged = 0;
	stream.postVideoFrame = 0;
	stream.postAudioFrame = 0;
	stream.postAudioBuffer = _postAudioBuffer;

	camera.d.startRequestImage = _startRequestImage;
	camera.d.stopRequestImage = _stopRequestImage;
	camera.d.requestImage = _requestImage;
	camera.buffer = NULL;
	camera.bufferSize = 0;
	camera.cam = SELECT_IN1;

	if (!allocateRomBuffer()) {
		return 1;
	}

	aptHook(&cookie, _aptHook, 0);

	ptmuInit();
	camInit();

	hasSound = NO_SOUND;
	if (!ndspInit()) {
		hasSound = DSP_SUPPORTED;
		ndspSetOutputMode(NDSP_OUTPUT_STEREO);
		ndspSetOutputCount(1);
		ndspChnReset(0);
		ndspChnSetFormat(0, NDSP_FORMAT_STEREO_PCM16);
		ndspChnSetInterp(0, NDSP_INTERP_NONE);
		ndspChnSetRate(0, 0x8000);
		ndspChnWaveBufClear(0);
		audioLeft = linearMemAlign(AUDIO_SAMPLES * DSP_BUFFERS * 2 * sizeof(int16_t), 0x80);
		memset(dspBuffer, 0, sizeof(dspBuffer));
		int i;
		for (i = 0; i < DSP_BUFFERS; ++i) {
			dspBuffer[i].data_pcm16 = &audioLeft[AUDIO_SAMPLES * i * 2];
			dspBuffer[i].nsamples = AUDIO_SAMPLES;
		}
	}

	gfxInit(GSP_BGR8_OES, GSP_BGR8_OES, true);

	if (!_initGpu()) {
		outputTexture.data = 0;
		_cleanup();
		return 1;
	}

	if (!C3D_TexInitVRAM(&outputTexture, 256, 256, GPU_RGB565)) {
		_cleanup();
		return 1;
	}
	C3D_TexSetWrap(&outputTexture, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
	C3D_TexSetFilter(&outputTexture, GPU_NEAREST, GPU_NEAREST);
	C3D_TexSetFilter(&upscaleBufferTex, GPU_LINEAR, GPU_LINEAR);
	void* outputTextureEnd = (u8*)outputTexture.data + 256 * 256 * 2;

	// Zero texture data to make sure no garbage around the border interferes with filtering
	GX_MemoryFill(
			outputTexture.data, 0x0000, outputTextureEnd, GX_FILL_16BIT_DEPTH | GX_FILL_TRIGGER,
			NULL, 0, NULL, 0);
	gspWaitForPSC0();

	struct GUIFont* font = GUIFontCreate();

	if (!font) {
		_cleanup();
		return 1;
	}

	struct mGUIRunner runner = {
		.params = {
			320, 240,
			font, "/",
			_drawStart, _drawEnd,
			_pollInput, _pollCursor,
			_batteryState,
			_guiPrepare, _guiFinish,
		},
		.keySources = (struct GUIInputKeys[]) {
			{
				.name = "3DS Input",
				.id = _3DS_INPUT,
				.keyNames = (const char*[]) {
					"A",
					"B",
					"Select",
					"Start",
					"D-Pad Right",
					"D-Pad Left",
					"D-Pad Up",
					"D-Pad Down",
					"R",
					"L",
					"X",
					"Y",
					0,
					0,
					"ZL",
					"ZR",
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					"C-Stick Right",
					"C-Stick Left",
					"C-Stick Up",
					"C-Stick Down",
				},
				.nKeys = 28
			},
			{ .id = 0 }
		},
		.configExtra = (struct GUIMenuItem[]) {
			{
				.title = "Screen mode",
				.data = "screenMode",
				.submenu = 0,
				.state = SM_PA_TOP,
				.validStates = (const char*[]) {
					"Pixel-Accurate/Bottom",
					"Aspect-Ratio Fit/Bottom",
					"Stretched/Bottom",
					"Pixel-Accurate/Top",
					"Aspect-Ratio Fit/Top",
					"Stretched/Top",
				},
				.nStates = 6
			},
			{
				.title = "Filtering",
				.data = "filterMode",
				.submenu = 0,
				.state = FM_LINEAR_2x,
				.validStates = (const char*[]) {
					NULL, // Disable choosing nearest neighbor; it always looks bad
					"Bilinear (smoother)",
					"Bilinear (pixelated)",
				},
				.nStates = 3
			},
			{
				.title = "Screen darkening",
				.data = "darkenMode",
				.submenu = 0,
				.state = DM_NATIVE,
				.validStates = (const char*[]) {
					"None",
					"Dark",
					"Very dark",
					"Grayed",
				},
				.nStates = 4
			},
			{
				.title = "Camera",
				.data = "camera",
				.submenu = 0,
				.state = 1,
				.validStates = (const char*[]) {
					"None",
					"Inner",
					"Outer",
				},
				.nStates = 3
			}
		},
		.nConfigExtra = 4,
		.setup = _setup,
		.teardown = 0,
		.gameLoaded = _gameLoaded,
		.gameUnloaded = _gameUnloaded,
		.prepareForFrame = 0,
		.drawFrame = _drawFrame,
		.drawScreenshot = _drawScreenshot,
		.paused = _gameUnloaded,
		.unpaused = _gameLoaded,
		.incrementScreenMode = _incrementScreenMode,
		.setFrameLimiter = _setFrameLimiter,
		.pollGameInput = _pollGameInput,
		.running = _running
	};

	runner.autosave.running = true;
	MutexInit(&runner.autosave.mutex);
	ConditionInit(&runner.autosave.cond);

	APT_SetAppCpuTimeLimit(20);
	runner.autosave.thread = threadCreate(mGUIAutosaveThread, &runner.autosave, 0x4000, 0x1F, 1, true);

	mGUIInit(&runner, "3ds");

	_map3DSKey(&runner.params.keyMap, KEY_X, GUI_INPUT_CANCEL);
	_map3DSKey(&runner.params.keyMap, KEY_Y, mGUI_INPUT_SCREEN_MODE);
	_map3DSKey(&runner.params.keyMap, KEY_B, GUI_INPUT_BACK);
	_map3DSKey(&runner.params.keyMap, KEY_A, GUI_INPUT_SELECT);
	_map3DSKey(&runner.params.keyMap, KEY_UP, GUI_INPUT_UP);
	_map3DSKey(&runner.params.keyMap, KEY_DOWN, GUI_INPUT_DOWN);
	_map3DSKey(&runner.params.keyMap, KEY_LEFT, GUI_INPUT_LEFT);
	_map3DSKey(&runner.params.keyMap, KEY_RIGHT, GUI_INPUT_RIGHT);
	_map3DSKey(&runner.params.keyMap, KEY_CSTICK_UP, mGUI_INPUT_INCREASE_BRIGHTNESS);
	_map3DSKey(&runner.params.keyMap, KEY_CSTICK_DOWN, mGUI_INPUT_DECREASE_BRIGHTNESS);

	mGUIRunloop(&runner);
	mGUIDeinit(&runner);

	_cleanup();
	return 0;
}
