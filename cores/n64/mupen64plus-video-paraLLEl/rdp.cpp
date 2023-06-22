#include "rdp.hpp"
#include "Gfx #1.3.h"
#include "parallel.h"
#include "z64.h"
#include <assert.h>

using namespace Vulkan;
using namespace std;

extern retro_log_printf_t log_cb;
extern retro_environment_t environ_cb;

namespace RDP
{
const struct retro_hw_render_interface_vulkan *vulkan;

static int cmd_cur;
static int cmd_ptr;
static uint32_t cmd_data[0x00040000 >> 2];
static uint64_t pending_timeline_value, timeline_value;

static unique_ptr<CommandProcessor> frontend;
static unique_ptr<Device> device;
static unique_ptr<Context> context;
static QueryPoolHandle begin_ts, end_ts;

static vector<retro_vulkan_image> retro_images;
static vector<ImageHandle> retro_image_handles;
unsigned width, height;
unsigned overscan;
unsigned upscaling = 1;
unsigned downscaling_steps = 0;
bool native_texture_lod = false;
bool native_tex_rect = true;
bool synchronous, divot_filter, gamma_dither, vi_aa, vi_scale, dither_filter, interlacing;

static const unsigned cmd_len_lut[64] = {
	1, 1, 1, 1, 1, 1, 1, 1, 4, 6, 12, 14, 12, 14, 20, 22,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  1,  1,  1,  1,  1,
	1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 1,  1,  1,  1,  1,  1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  1,  1,  1,  1,  1,
};

void process_commands()
{
	const uint32_t DP_CURRENT = *GET_GFX_INFO(DPC_CURRENT_REG) & 0x00FFFFF8;
	const uint32_t DP_END = *GET_GFX_INFO(DPC_END_REG) & 0x00FFFFF8;
	*GET_GFX_INFO(DPC_STATUS_REG) &= ~DP_STATUS_FREEZE;

	int length = DP_END - DP_CURRENT;
	if (length <= 0)
		return;

	length = unsigned(length) >> 3;
	if ((cmd_ptr + length) & ~(0x0003FFFF >> 3))
		return;

	uint32_t offset = DP_CURRENT;
	if (*GET_GFX_INFO(DPC_STATUS_REG) & DP_STATUS_XBUS_DMA)
	{
		do
		{
			offset &= 0xFF8;
			cmd_data[2 * cmd_ptr + 0] = *reinterpret_cast<const uint32_t *>(SP_DMEM + offset);
			cmd_data[2 * cmd_ptr + 1] = *reinterpret_cast<const uint32_t *>(SP_DMEM + offset + 4);
			offset += sizeof(uint64_t);
			cmd_ptr++;
		} while (--length > 0);
	}
	else
	{
		if (DP_END > 0x7ffffff || DP_CURRENT > 0x7ffffff)
		{
			return;
		}
		else
		{
			do
			{
				offset &= 0xFFFFF8;
				cmd_data[2 * cmd_ptr + 0] = *reinterpret_cast<const uint32_t *>(DRAM + offset);
				cmd_data[2 * cmd_ptr + 1] = *reinterpret_cast<const uint32_t *>(DRAM + offset + 4);
				offset += sizeof(uint64_t);
				cmd_ptr++;
			} while (--length > 0);
		}
	}

	while (cmd_cur - cmd_ptr < 0)
	{
		uint32_t w1 = cmd_data[2 * cmd_cur];
		uint32_t command = (w1 >> 24) & 63;
		int cmd_length = cmd_len_lut[command];

		if (cmd_ptr - cmd_cur - cmd_length < 0)
		{
			*GET_GFX_INFO(DPC_START_REG) = *GET_GFX_INFO(DPC_CURRENT_REG) = *GET_GFX_INFO(DPC_END_REG);
			return;
		}

		if (command >= 8 && frontend)
			frontend->enqueue_command(cmd_length * 2, &cmd_data[2 * cmd_cur]);

		if (RDP::Op(command) == RDP::Op::SyncFull)
		{
			// For synchronous RDP:
			if (synchronous && frontend)
				frontend->wait_for_timeline(frontend->signal_timeline());
			*gfx_info.MI_INTR_REG |= DP_INTERRUPT;
			gfx_info.CheckInterrupts();
		}

		cmd_cur += cmd_length;
	}

	cmd_ptr = 0;
	cmd_cur = 0;
	*GET_GFX_INFO(DPC_START_REG) = *GET_GFX_INFO(DPC_CURRENT_REG) = *GET_GFX_INFO(DPC_END_REG);
}

static QueryPoolHandle refresh_begin_ts;

void profile_refresh_begin()
{
	if (device)
		refresh_begin_ts = device->write_calibrated_timestamp();
}

void profile_refresh_end()
{
	if (device)
	{
		device->register_time_interval("Emulation", refresh_begin_ts, device->write_calibrated_timestamp(), "refresh");
		refresh_begin_ts.reset();
	}
}

void begin_frame()
{
	unsigned mask = vulkan->get_sync_index_mask(vulkan->handle);
	unsigned num_frames = 0;
	for (unsigned i = 0; i < 32; i++)
		if (mask & (1u << i))
			num_frames = i + 1;

	if (num_frames != retro_images.size())
	{
		retro_images.resize(num_frames);
		retro_image_handles.resize(num_frames);
	}

	vulkan->wait_sync_index(vulkan->handle);
	if (!begin_ts)
		begin_ts = device->write_calibrated_timestamp();

	//frontend->wait_for_timeline(pending_timeline_value);
	//pending_timeline_value = timeline_value;
}

bool init()
{
	if (!context || !vulkan)
		return false;

	unsigned mask = vulkan->get_sync_index_mask(vulkan->handle);
	unsigned num_frames = 0;
	unsigned num_sync_frames = 0;
	for (unsigned i = 0; i < 32; i++)
	{
		if (mask & (1u << i))
		{
			num_frames = i + 1;
			num_sync_frames++;
		}
	}

	retro_images.resize(num_frames);
	retro_image_handles.resize(num_frames);

	device.reset(new Device);
	device->set_context(*context);
	device->init_frame_contexts(num_sync_frames);
	log_cb(RETRO_LOG_INFO, "Using %u sync frames for parallel-RDP.\n", num_sync_frames);
	device->set_queue_lock(
			[]() { vulkan->lock_queue(vulkan->handle); },
			[]() { vulkan->unlock_queue(vulkan->handle); });

	uintptr_t aligned_rdram = reinterpret_cast<uintptr_t>(gfx_info.RDRAM);
	uintptr_t offset = 0;

	if (device->get_device_features().supports_external_memory_host)
	{
		size_t align = device->get_device_features().host_memory_properties.minImportedHostPointerAlignment;
		offset = aligned_rdram & (align - 1);
		aligned_rdram -= offset;
	}
	else
		log_cb(RETRO_LOG_WARN, "VK_EXT_external_memory_host is not supported by this device. Application might run slower because of this.\n");

	CommandProcessorFlags flags = 0;
	switch (upscaling)
	{
		case 2:
			flags |= COMMAND_PROCESSOR_FLAG_UPSCALING_2X_BIT;
			log_cb(RETRO_LOG_INFO, "Using 2x upscaling!\n");
			break;

		case 4:
			flags |= COMMAND_PROCESSOR_FLAG_UPSCALING_4X_BIT;
			log_cb(RETRO_LOG_INFO, "Using 4x upscaling!\n");
			break;

		case 8:
			flags |= COMMAND_PROCESSOR_FLAG_UPSCALING_8X_BIT;
			log_cb(RETRO_LOG_INFO, "Using 8x upscaling!\n");
			break;

		default:
			break;
	}

	frontend.reset(new CommandProcessor(*device, reinterpret_cast<void *>(aligned_rdram),
				offset, 8 * 1024 * 1024, 4 * 1024 * 1024, flags));

	if (!frontend->device_is_supported())
	{
		log_cb(RETRO_LOG_ERROR, "This device probably does not support 8/16-bit storage. Make sure you're using up-to-date drivers!\n");
		frontend.reset();
		return false;
	}

	RDP::Quirks quirks;
	quirks.set_native_texture_lod(native_texture_lod);
	quirks.set_native_resolution_tex_rect(native_tex_rect);
	frontend->set_quirks(quirks);

	timeline_value = 0;
	pending_timeline_value = 0;
	width = 0;
	height = 0;
	return true;
}

void deinit()
{
	begin_ts.reset();
	end_ts.reset();
	retro_image_handles.clear();
	retro_images.clear();
	frontend.reset();
	device.reset();
	context.reset();
}

static void complete_frame_error()
{
	static const char error_tex[] =
		"ooooooooooooooooooooooooo"
		"ooXXXXXoooXXXXXoooXXXXXoo"
		"ooXXooooooXoooXoooXoooXoo"
		"ooXXXXXoooXXXXXoooXXXXXoo"
		"ooXXXXXoooXoXoooooXoXoooo"
		"ooXXooooooXooXooooXooXooo"
		"ooXXXXXoooXoooXoooXoooXoo"
		"ooooooooooooooooooooooooo";

	auto info = Vulkan::ImageCreateInfo::immutable_2d_image(50, 16, VK_FORMAT_R8G8B8A8_UNORM, false);
	info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	info.misc = IMAGE_MISC_MUTABLE_SRGB_BIT;

	Vulkan::ImageInitialData data = {};

	uint32_t tex_data[16][50];
	for (unsigned y = 0; y < 16; y++)
		for (unsigned x = 0; x < 50; x++)
			tex_data[y][x] = error_tex[25 * (y >> 1) + (x >> 1)] != 'o' ? 0xffffffffu : 0u;
	data.data = tex_data;
	auto image = device->create_image(info, &data);

	unsigned index = vulkan->get_sync_index(vulkan->handle);
	assert(index < retro_images.size());

	retro_images[index].image_view = image->get_view().get_view();
	retro_images[index].image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	retro_images[index].create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	retro_images[index].create_info.image = image->get_image();
	retro_images[index].create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	retro_images[index].create_info.format = VK_FORMAT_R8G8B8A8_UNORM;
	retro_images[index].create_info.subresourceRange.baseMipLevel = 0;
	retro_images[index].create_info.subresourceRange.baseArrayLayer = 0;
	retro_images[index].create_info.subresourceRange.levelCount = 1;
	retro_images[index].create_info.subresourceRange.layerCount = 1;
	retro_images[index].create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	retro_images[index].create_info.components.r = VK_COMPONENT_SWIZZLE_R;
	retro_images[index].create_info.components.g = VK_COMPONENT_SWIZZLE_G;
	retro_images[index].create_info.components.b = VK_COMPONENT_SWIZZLE_B;
	retro_images[index].create_info.components.a = VK_COMPONENT_SWIZZLE_A;

	vulkan->set_image(vulkan->handle, &retro_images[index], 0, nullptr, VK_QUEUE_FAMILY_IGNORED);
	width = image->get_width();
	height = image->get_height();
	retro_image_handles[index] = image;

	device->flush_frame();
}

void complete_frame()
{
	if (!frontend)
	{
		complete_frame_error();
		device->next_frame_context();
		return;
	}

	timeline_value = frontend->signal_timeline();

	frontend->set_vi_register(VIRegister::Control, *GET_GFX_INFO(VI_STATUS_REG));
	frontend->set_vi_register(VIRegister::Origin, *GET_GFX_INFO(VI_ORIGIN_REG));
	frontend->set_vi_register(VIRegister::Width, *GET_GFX_INFO(VI_WIDTH_REG));
	frontend->set_vi_register(VIRegister::Intr, *GET_GFX_INFO(VI_INTR_REG));
	frontend->set_vi_register(VIRegister::VCurrentLine, *GET_GFX_INFO(VI_V_CURRENT_LINE_REG));
	frontend->set_vi_register(VIRegister::Timing, *GET_GFX_INFO(VI_V_BURST_REG));
	frontend->set_vi_register(VIRegister::VSync, *GET_GFX_INFO(VI_V_SYNC_REG));
	frontend->set_vi_register(VIRegister::HSync, *GET_GFX_INFO(VI_H_SYNC_REG));
	frontend->set_vi_register(VIRegister::Leap, *GET_GFX_INFO(VI_LEAP_REG));
	frontend->set_vi_register(VIRegister::HStart, *GET_GFX_INFO(VI_H_START_REG));
	frontend->set_vi_register(VIRegister::VStart, *GET_GFX_INFO(VI_V_START_REG));
	frontend->set_vi_register(VIRegister::VBurst, *GET_GFX_INFO(VI_V_BURST_REG));
	frontend->set_vi_register(VIRegister::XScale, *GET_GFX_INFO(VI_X_SCALE_REG));
	frontend->set_vi_register(VIRegister::YScale, *GET_GFX_INFO(VI_Y_SCALE_REG));

	ScanoutOptions opts;
	opts.persist_frame_on_invalid_input = true;
	opts.vi.aa = vi_aa;
	opts.vi.scale = vi_scale;
	opts.vi.dither_filter = dither_filter;
	opts.vi.divot_filter = divot_filter;
	opts.vi.gamma_dither = gamma_dither;
	opts.downscale_steps = downscaling_steps;
	opts.crop_overscan_pixels = overscan;
	auto image = frontend->scanout(opts);
	unsigned index = vulkan->get_sync_index(vulkan->handle);

	if (!image)
	{
		auto info = Vulkan::ImageCreateInfo::immutable_2d_image(1, 1, VK_FORMAT_R8G8B8A8_UNORM);
		info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
			VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		info.misc = IMAGE_MISC_MUTABLE_SRGB_BIT;
		info.initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
		image = device->create_image(info);

		auto cmd = device->request_command_buffer();
		cmd->image_barrier(*image,
				VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT);
		cmd->clear_image(*image, {});
		cmd->image_barrier(*image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT);
		device->submit(cmd);
	}

	assert(index < retro_images.size());

	retro_images[index].image_view = image->get_view().get_view();
	retro_images[index].image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	retro_images[index].create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	retro_images[index].create_info.image = image->get_image();
	retro_images[index].create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	retro_images[index].create_info.format = VK_FORMAT_R8G8B8A8_UNORM;
	retro_images[index].create_info.subresourceRange.baseMipLevel = 0;
	retro_images[index].create_info.subresourceRange.baseArrayLayer = 0;
	retro_images[index].create_info.subresourceRange.levelCount = 1;
	retro_images[index].create_info.subresourceRange.layerCount = 1;
	retro_images[index].create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	retro_images[index].create_info.components.r = VK_COMPONENT_SWIZZLE_R;
	retro_images[index].create_info.components.g = VK_COMPONENT_SWIZZLE_G;
	retro_images[index].create_info.components.b = VK_COMPONENT_SWIZZLE_B;
	retro_images[index].create_info.components.a = VK_COMPONENT_SWIZZLE_A;

	vulkan->set_image(vulkan->handle, &retro_images[index], 0, nullptr, VK_QUEUE_FAMILY_IGNORED);
	width = image->get_width();
	height = image->get_height();
	retro_image_handles[index] = image;

	end_ts = device->write_calibrated_timestamp();
	device->register_time_interval("Emulation", begin_ts, end_ts, "frame");
	begin_ts.reset();
	end_ts.reset();

	RDP::Quirks quirks;
	quirks.set_native_texture_lod(native_texture_lod);
	quirks.set_native_resolution_tex_rect(native_tex_rect);
	frontend->set_quirks(quirks);

	frontend->begin_frame_context();
}
}

bool parallel_create_device(struct retro_vulkan_context *frontend_context, VkInstance instance, VkPhysicalDevice gpu,
                            VkSurfaceKHR surface, PFN_vkGetInstanceProcAddr get_instance_proc_addr,
                            const char **required_device_extensions, unsigned num_required_device_extensions,
                            const char **required_device_layers, unsigned num_required_device_layers,
                            const VkPhysicalDeviceFeatures *required_features)
{
	if (!Vulkan::Context::init_loader(get_instance_proc_addr))
		return false;

	::RDP::context.reset(new Vulkan::Context);
	if (!::RDP::context->init_device_from_instance(
				instance, gpu, surface, required_device_extensions, num_required_device_extensions,
				required_device_layers, num_required_device_layers, required_features, Vulkan::CONTEXT_CREATION_DISABLE_BINDLESS_BIT))
	{
		::RDP::context.reset();
		return false;
	}

	frontend_context->gpu = ::RDP::context->get_gpu();
	frontend_context->device = ::RDP::context->get_device();
	frontend_context->queue = ::RDP::context->get_graphics_queue();
	frontend_context->queue_family_index = ::RDP::context->get_graphics_queue_family();
	frontend_context->presentation_queue = ::RDP::context->get_graphics_queue();
	frontend_context->presentation_queue_family_index = ::RDP::context->get_graphics_queue_family();

	// Frontend owns the device.
	::RDP::context->release_device();
	return true;
}

static const VkApplicationInfo parallel_app_info = {
	VK_STRUCTURE_TYPE_APPLICATION_INFO,
	nullptr,
	"paraLLEl-RDP",
	0,
	"Granite",
	0,
	VK_API_VERSION_1_1,
};

const VkApplicationInfo *parallel_get_application_info(void)
{
	return &parallel_app_info;
}

