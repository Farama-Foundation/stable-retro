#include "rdp.hpp"
#include "Gfx #1.3.h"
#include "m64p_config.h"
#include "m64p_types.h"
#include <stdlib.h>
#include <string.h>
#include <boolean.h>

extern "C" {
extern int retro_return(bool just_flipping);

void parallelChangeWindow(void)
{
}

void parallelReadScreen2(void *dest, int *width, int *height, int front)
{
}

void parallelDrawScreen(void)
{
}

void parallelGetDllInfo(PLUGIN_INFO *PluginInfo)
{
	PluginInfo->Version = 0x0001;
	PluginInfo->Type = 2;
	strcpy(PluginInfo->Name, "paraLLEl-RDP");
	PluginInfo->NormalMemory = true;
	PluginInfo->MemoryBswaped = true;
}

void parallelSetRenderingCallback(void (*callback)(int))
{
}

int parallelInitiateGFX(GFX_INFO Gfx_Info)
{
	return true;
}

void parallelMoveScreen(int xpos, int ypos)
{
}

void parallelProcessDList(void)
{
}

void parallelProcessRDPList(void)
{
	RDP::process_commands();
}

void parallelRomClosed(void)
{
}

int parallelRomOpen(void)
{
	return 1;
}

void parallelUpdateScreen(void)
{
	RDP::complete_frame();
	retro_return(true);
}

void parallelShowCFB(void)
{
	parallelUpdateScreen();
}

void parallelViStatusChanged(void)
{
}

void parallelViWidthChanged(void)
{
}

void parallelFBWrite(unsigned int addr, unsigned int size)
{
}

void parallelFBRead(unsigned int addr)
{
}

void parallelFBGetFrameBufferInfo(void *pinfo)
{
}

m64p_error parallelPluginGetVersion(m64p_plugin_type *PluginType, int *PluginVersion, int *APIVersion,
		const char **PluginNamePtr, int *Capabilities)
{
	/* set version info */
	if (PluginType != NULL)
		*PluginType = M64PLUGIN_GFX;

	if (PluginVersion != NULL)
		*PluginVersion = 0x016304;

	if (APIVersion != NULL)
		*APIVersion = 0x020100;

	if (PluginNamePtr != NULL)
		*PluginNamePtr = "paraLLEl-RDP";

	if (Capabilities != NULL)
		*Capabilities = 0;

	return M64ERR_SUCCESS;
}

bool parallel_init(const struct retro_hw_render_interface_vulkan *vulkan)
{
	RDP::vulkan = vulkan;
	return RDP::init();
}

void parallel_deinit()
{
	RDP::deinit();
	RDP::vulkan = nullptr;
}

unsigned parallel_frame_width()
{
	return RDP::width;
}

unsigned parallel_frame_height()
{
	return RDP::height;
}

bool parallel_frame_is_valid()
{
	return true;
}

void parallel_begin_frame()
{
	RDP::begin_frame();
}

void parallel_set_synchronous_rdp(bool enable)
{
	RDP::synchronous = enable;
}

void parallel_set_divot_filter(bool enable)
{
	RDP::divot_filter = enable;
}

void parallel_set_gamma_dither(bool enable)
{
	RDP::gamma_dither = enable;
}

void parallel_set_vi_aa(bool enable)
{
	RDP::vi_aa = enable;
}

void parallel_set_vi_scale(bool enable)
{
	RDP::vi_scale = enable;
}

void parallel_set_dither_filter(bool enable)
{
	RDP::dither_filter = enable;
}

void parallel_set_interlacing(bool enable)
{
	RDP::interlacing = enable;
}

void parallel_profile_video_refresh_begin(void)
{
	RDP::profile_refresh_begin();
}

void parallel_profile_video_refresh_end(void)
{
	RDP::profile_refresh_end();
}

void parallel_set_upscaling(unsigned factor)
{
	RDP::upscaling = factor;
}

void parallel_set_downscaling_steps(unsigned steps)
{
	RDP::downscaling_steps = steps;
}

void parallel_set_native_texture_lod(bool enable)
{
	RDP::native_texture_lod = enable;
}

void parallel_set_native_tex_rect(bool enable)
{
	RDP::native_tex_rect = enable;
}

void parallel_set_overscan_crop(unsigned pixels)
{
	RDP::overscan = pixels;
}
}
