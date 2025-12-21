#include <cassert>
#ifndef _WIN32
#include <dlfcn.h>
#endif
#include <fstream>
#include <map>
#include <sstream>
#include <unordered_map>
#include <vector>

#include "coreinfo.h"
#include "data.h"
#include "emulator.h"
#include "libretro.h"

#ifndef _WIN32
#define GETSYM dlsym
#else
#define GETSYM GetProcAddress
#endif

using namespace std;

namespace Retro {

static Emulator* s_loadedEmulator = nullptr;

static map<string, const char*> s_envVariables = {
	{ "genesis_plus_gx_bram", "per game" },
	{ "genesis_plus_gx_render", "single field" },
	{ "genesis_plus_gx_blargg_ntsc_filter", "disabled" },

	// Parallel-N64 defaults: force a CPU-rendered framebuffer so the frontend can read pixels.
	// If left on auto, the core may choose an OpenGL path and provide no CPU buffer.
	{ "parallel-n64-gfxplugin", "angrylion" },
};

static void (*retro_init)(void);
static void (*retro_deinit)(void);
static unsigned (*retro_api_version)(void);
static void (*retro_get_system_info)(struct retro_system_info* info);
static void (*retro_get_system_av_info)(struct retro_system_av_info* info);
static void (*retro_reset)(void);
static void (*retro_run)(void);
static size_t (*retro_serialize_size)(void);
static bool (*retro_serialize)(void* data, size_t size);
static bool (*retro_unserialize)(const void* data, size_t size);
static bool (*retro_load_game)(const struct retro_game_info* game);
static void (*retro_unload_game)(void);
static void* (*retro_get_memory_data)(unsigned id);
static size_t (*retro_get_memory_size)(unsigned id);
static void (*retro_cheat_reset)(void);
static void (*retro_cheat_set)(unsigned index, bool enabled, const char* code);
static void (*retro_set_environment)(retro_environment_t);
static void (*retro_set_video_refresh)(retro_video_refresh_t);
static void (*retro_set_audio_sample)(retro_audio_sample_t);
static void (*retro_set_audio_sample_batch)(retro_audio_sample_batch_t);
static void (*retro_set_input_poll)(retro_input_poll_t);
static void (*retro_set_input_state)(retro_input_state_t);

Emulator::Emulator() {
}

Emulator::~Emulator() {
	if (m_corePath) {
		free(m_corePath);
	}
	if (m_romLoaded) {
		unloadRom();
	}
	if (m_coreHandle) {
		unloadCore();
	}
}

bool Emulator::isLoaded() {
	return s_loadedEmulator;
}

bool Emulator::loadRom(const string& romPath) {
	if (m_romLoaded) {
		unloadRom();
	}

	auto core = coreForRom(romPath);
	if (core.size() == 0) {
		return false;
	}

	if (m_coreHandle && m_core != core) {
		unloadCore();
	}
	if (!m_coreHandle) {
		string lib = libForCore(core) + "_libretro.";
#ifdef __APPLE__
		lib += "dylib";
#elif defined(_WIN32)
		lib += "dll";
#else
		lib += "so";
#endif
		if (!loadCore(corePath() + "/" + lib)) {
			return false;
		}
		m_core = core;
	}

	retro_game_info gameInfo;
	ifstream in(romPath, ios::binary | ios::ate);
	if (in.fail()) {
		return false;
	}
	ostringstream out;
	gameInfo.size = in.tellg();
	if (in.fail()) {
		return false;
	}
	char* romData = new char[gameInfo.size];
	gameInfo.path = romPath.c_str();
	gameInfo.data = romData;
	in.seekg(0, ios::beg);
	in.read(romData, gameInfo.size);
	if (in.fail()) {
		delete[] romData;
		return false;
	}
	in.close();

	m_rotation = 0;
	auto res = retro_load_game(&gameInfo);
	delete[] romData;
	if (!res) {
		return false;
	}
	retro_get_system_av_info(&m_avInfo);
	fixScreenSize(romPath);

	// For some cores (notably some N64 cores), the initial AV info can be wrong.
	// Prefer the per-frame dimensions passed to cbVideoRefresh.
	{
		retro_system_info systemInfo;
		retro_get_system_info(&systemInfo);
		m_updateGeometryFromVideoRefresh =
			!strcmp(systemInfo.library_name, "ParaLLEl N64") ||
			!strcmp(systemInfo.library_name, "Mupen64Plus") ||
			!strcmp(systemInfo.library_name, "Mupen64Plus-Next");
	}

	if (m_serializationQuirks & RETRO_SERIALIZATION_QUIRK_MUST_INITIALIZE) {
		m_needsInitFrame = true;
	}

	m_romLoaded = true;
	m_romPath = romPath;
	return true;
}

void Emulator::run() {
	assert(s_loadedEmulator == this);
	m_audioData.clear();
	retro_run();
	if (m_serializationQuirks & RETRO_SERIALIZATION_QUIRK_MUST_INITIALIZE) {
		m_needsInitFrame = false;
	}
}

void Emulator::reset() {
	assert(s_loadedEmulator == this);

	memset(m_buttonMask, 0, sizeof(m_buttonMask));

	retro_system_info systemInfo;
	retro_get_system_info(&systemInfo);
	if (!strcmp(systemInfo.library_name, "Stella")) {
		// Stella does not properly clear everything when reseting or loading a savestate
		string romPath = m_romPath;

#ifdef _WIN32
		FreeLibrary(m_coreHandle);
#else
		dlclose(m_coreHandle);
#endif
		m_coreHandle = nullptr;
		s_loadedEmulator = nullptr;
		m_romLoaded = false;
		loadRom(m_romPath);
		if (m_addressSpace) {
			m_addressSpace->reset();
			m_addressSpace->addBlock(Retro::ramBase(m_core), retro_get_memory_size(RETRO_MEMORY_SYSTEM_RAM), retro_get_memory_data(RETRO_MEMORY_SYSTEM_RAM));
		}
	}

	retro_reset();

	if (m_serializationQuirks & RETRO_SERIALIZATION_QUIRK_MUST_INITIALIZE) {
		m_needsInitFrame = true;
	}
}

void Emulator::unloadCore() {
	if (!m_coreHandle) {
		return;
	}
	if (m_romLoaded) {
		unloadRom();
	}
	retro_deinit();
#ifdef _WIN32
	FreeLibrary(m_coreHandle);
#else
	dlclose(m_coreHandle);
#endif
	m_coreHandle = nullptr;
	s_loadedEmulator = nullptr;
}

void Emulator::unloadRom() {
	if (!m_romLoaded) {
		return;
	}
	retro_unload_game();
	m_romLoaded = false;
	m_romPath.clear();
	m_addressSpace = nullptr;
	m_map.clear();
}

bool Emulator::serialize(void* data, size_t size) {
	assert(s_loadedEmulator == this);
	ensureInitializedForSerialization();
	return retro_serialize(data, size);
}

bool Emulator::unserialize(const void* data, size_t size) {
	assert(s_loadedEmulator == this);
	try {
		retro_system_info systemInfo;
		retro_get_system_info(&systemInfo);
		if (!strcmp(systemInfo.library_name, "Stella")) {
			reset();
		}


			ensureInitializedForSerialization();
			bool ok = retro_unserialize(data, size);
			if (ok && (m_serializationQuirks & RETRO_SERIALIZATION_QUIRK_MUST_INITIALIZE)) {
				m_needsInitFrame = false;
			}
			return ok;
	} catch (...) {
		return false;
	}
}

size_t Emulator::serializeSize() {
	assert(s_loadedEmulator == this);
	return retro_serialize_size();
}

void Emulator::ensureInitializedForSerialization() {
	if ((m_serializationQuirks & RETRO_SERIALIZATION_QUIRK_MUST_INITIALIZE) && m_needsInitFrame) {
		// Run a single frame to satisfy cores that require initialization before (de)serialization
		run();
	}
}

void Emulator::clearCheats() {
	assert(s_loadedEmulator == this);
	retro_cheat_reset();
}

void Emulator::setCheat(unsigned index, bool enabled, const char* code) {
	assert(s_loadedEmulator == this);
	retro_cheat_set(index, enabled, code);
}

bool Emulator::loadCore(const string& corePath) {
	if (s_loadedEmulator) {
		return false;
	}

#ifdef _WIN32
	m_coreHandle = LoadLibrary(corePath.c_str());
#else
	m_coreHandle = dlopen(corePath.c_str(), RTLD_LAZY);
#endif
	if (!m_coreHandle) {
		return false;
	}

	retro_init = reinterpret_cast<void (*)()>(GETSYM(m_coreHandle, "retro_init"));
	retro_deinit = reinterpret_cast<void (*)()>(GETSYM(m_coreHandle, "retro_deinit"));
	retro_api_version = reinterpret_cast<unsigned int (*)()>(GETSYM(m_coreHandle, "retro_api_version"));
	retro_get_system_info = reinterpret_cast<void (*)(struct retro_system_info*)>(GETSYM(m_coreHandle, "retro_get_system_info"));
	retro_get_system_av_info = reinterpret_cast<void (*)(struct retro_system_av_info*)>(GETSYM(m_coreHandle, "retro_get_system_av_info"));
	retro_reset = reinterpret_cast<void (*)()>(GETSYM(m_coreHandle, "retro_reset"));
	retro_run = reinterpret_cast<void (*)()>(GETSYM(m_coreHandle, "retro_run"));
	retro_serialize_size = reinterpret_cast<size_t (*)()>(GETSYM(m_coreHandle, "retro_serialize_size"));
	retro_serialize = reinterpret_cast<bool (*)(void*, size_t)>(GETSYM(m_coreHandle, "retro_serialize"));
	retro_unserialize = reinterpret_cast<bool (*)(const void*, size_t)>(GETSYM(m_coreHandle, "retro_unserialize"));
	retro_load_game = reinterpret_cast<bool (*)(const struct retro_game_info*)>(GETSYM(m_coreHandle, "retro_load_game"));
	retro_unload_game = reinterpret_cast<void (*)()>(GETSYM(m_coreHandle, "retro_unload_game"));
	retro_get_memory_data = reinterpret_cast<void* (*) (unsigned int)>(GETSYM(m_coreHandle, "retro_get_memory_data"));
	retro_get_memory_size = reinterpret_cast<size_t (*)(unsigned int)>(GETSYM(m_coreHandle, "retro_get_memory_size"));
	retro_cheat_reset = reinterpret_cast<void (*)()>(GETSYM(m_coreHandle, "retro_cheat_reset"));
	retro_cheat_set = reinterpret_cast<void (*)(unsigned int, bool, const char*)>(GETSYM(m_coreHandle, "retro_cheat_set"));
	retro_set_environment = reinterpret_cast<void (*)(retro_environment_t)>(GETSYM(m_coreHandle, "retro_set_environment"));
	retro_set_video_refresh = reinterpret_cast<void (*)(retro_video_refresh_t)>(GETSYM(m_coreHandle, "retro_set_video_refresh"));
	retro_set_audio_sample = reinterpret_cast<void (*)(retro_audio_sample_t)>(GETSYM(m_coreHandle, "retro_set_audio_sample"));
	retro_set_audio_sample_batch = reinterpret_cast<void (*)(retro_audio_sample_batch_t)>(GETSYM(m_coreHandle, "retro_set_audio_sample_batch"));
	retro_set_input_poll = reinterpret_cast<void (*)(retro_input_poll_t)>(GETSYM(m_coreHandle, "retro_set_input_poll"));
	retro_set_input_state = reinterpret_cast<void (*)(short (*)(unsigned int, unsigned int, unsigned int, unsigned int))>(GETSYM(m_coreHandle, "retro_set_input_state"));

	// The default according to the docs
	m_imgDepth = 15;
	s_loadedEmulator = this;

	retro_set_environment(cbEnvironment);
	retro_set_video_refresh(cbVideoRefresh);
	retro_set_audio_sample(cbAudioSample);
	retro_set_audio_sample_batch(cbAudioSampleBatch);
	retro_set_input_poll(cbInputPoll);
	retro_set_input_state(cbInputState);
	retro_init();

		if (m_serializationQuirks & RETRO_SERIALIZATION_QUIRK_MUST_INITIALIZE) {
			m_needsInitFrame = true;
		}

	return true;
}

void Emulator::fixScreenSize(const string& romName) {
	retro_system_info systemInfo;
	retro_get_system_info(&systemInfo);
	if (!strcmp(systemInfo.library_name, "Genesis Plus GX")) {
		switch (romName.back()) {
		case 'd': // Mega Drive
			// Genesis Plus GX gives us too small a resolution initially
			m_avInfo.geometry.base_width = 320;
			m_avInfo.geometry.base_height = 224;
			break;
		case 's': // Master System
			// Genesis Plus GX gives us too small a resolution initially
			m_avInfo.geometry.base_width = 256;
			m_avInfo.geometry.base_height = 192;
			break;
		case 'g': // Game Gear
			m_avInfo.geometry.base_width = 160;
			m_avInfo.geometry.base_height = 144;
			break;
		}
	} else if (!strcmp(systemInfo.library_name, "Stella")) {
		// Stella gives confusing values to pretend the pixel width is 2x
		m_avInfo.geometry.base_width = 160;
	} else if (!strcmp(systemInfo.library_name, "Mednafen PCE Fast")) {
		m_avInfo.geometry.base_width = 256;
		m_avInfo.geometry.base_height = 242;
	} else if (!strcmp(systemInfo.library_name, "ParaLLEl N64") ||
			   !strcmp(systemInfo.library_name, "Mupen64Plus") ||
			   !strcmp(systemInfo.library_name, "Mupen64Plus-Next")) {
		// Some N64 libretro cores report a half-height (or otherwise unexpected)
		// base_height which causes the frontend to display only the top half
		// of the frame. Ensure we have at least a 480px height reported so the
		// image isn't vertically cropped.
		if (m_avInfo.geometry.base_height < 480) {
			m_avInfo.geometry.base_height = 480;
		}
	}
}

void Emulator::reconfigureAddressSpace() {
	if (!m_addressSpace) {
		return;
	}
	if (!m_map.empty() && m_addressSpace->blocks().empty()) {
		for (const auto& desc : m_map) {
			if (desc.flags & RETRO_MEMDESC_CONST) {
				continue;
			}
			size_t len = desc.len;
			if (desc.select) {
				len = ((~desc.select & ~desc.start) + 1) & desc.select;
			}
			if (desc.len && desc.len < len) {
				len = desc.len;
			}
			m_addressSpace->addBlock(desc.start, len, desc.ptr);
		}
	}
}

// callback for logging from emulator
// turned off by default to avoid spamming the log, only used for debugging issues within cores
static void cbLog(enum retro_log_level level, const char *fmt, ...) {
#if 0
	char buffer[4096] = {0};
	static const char * levelName[] = { "DEBUG", "INFO", "WARNING", "ERROR" };
	va_list va;

	va_start(va, fmt);
	std::vsnprintf(buffer, sizeof(buffer), fmt, va);
	va_end(va);

	if (level == 0)
		return;

	std::cerr << "[" << levelName[level] << "] " << buffer << std::flush;
#endif
}

bool Emulator::cbEnvironment(unsigned cmd, void* data) {
	assert(s_loadedEmulator);
	switch (cmd) {
	case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
		switch (*reinterpret_cast<retro_pixel_format*>(data)) {
		case RETRO_PIXEL_FORMAT_XRGB8888:
			s_loadedEmulator->m_imgDepth = 32;
			break;
		case RETRO_PIXEL_FORMAT_RGB565:
			s_loadedEmulator->m_imgDepth = 16;
			break;
		case RETRO_PIXEL_FORMAT_0RGB1555:
			s_loadedEmulator->m_imgDepth = 15;
			break;
		default:
			s_loadedEmulator->m_imgDepth = 0;
			break;
		}
		return true;
	case RETRO_ENVIRONMENT_GET_VARIABLE: {
		struct retro_variable* var = reinterpret_cast<struct retro_variable*>(data);
		if (s_envVariables.count(string(var->key))) {
			var->value = s_envVariables[string(var->key)];
			return true;
		}
		return false;
	}
	case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
		if (!s_loadedEmulator->m_corePath) {
			s_loadedEmulator->m_corePath = strdup(corePath().c_str());
		}
		*reinterpret_cast<const char**>(data) = s_loadedEmulator->m_corePath;
		return true;
	case RETRO_ENVIRONMENT_GET_CAN_DUPE:
		*reinterpret_cast<bool*>(data) = true;
		return true;
	case RETRO_ENVIRONMENT_SET_MEMORY_MAPS:
		s_loadedEmulator->m_map.clear();
		for (size_t i = 0; i < static_cast<const retro_memory_map*>(data)->num_descriptors; ++i) {
			s_loadedEmulator->m_map.emplace_back(static_cast<const retro_memory_map*>(data)->descriptors[i]);
		}
		s_loadedEmulator->reconfigureAddressSpace();
		return true;
	case RETRO_ENVIRONMENT_SET_ROTATION: {
		const unsigned* rotation = reinterpret_cast<const unsigned*>(data);
		if (rotation) {
			unsigned raw = *rotation % 4;
			if (s_loadedEmulator->m_core == "FBNeo") {
				raw = (4 - raw) % 4;
			}
			s_loadedEmulator->m_rotation = static_cast<int>(raw);
		}
		return true;
	}
	// Logs needs to be handled even when not used, otherwise some cores (ex: mame2003_plus) will crash
	// Also very useful when integrating new emulators to debug issues within the core itself
	case RETRO_ENVIRONMENT_GET_LOG_INTERFACE: {
		struct retro_log_callback *cb = (struct retro_log_callback *)data;
		cb->log = cbLog;
		return true;
	}
	case RETRO_ENVIRONMENT_SET_SERIALIZATION_QUIRKS: {
		s_loadedEmulator->m_serializationQuirks = *reinterpret_cast<const uint64_t*>(data);
		if (s_loadedEmulator->m_serializationQuirks & RETRO_SERIALIZATION_QUIRK_MUST_INITIALIZE) {
			s_loadedEmulator->m_needsInitFrame = true;
		}
		return true;
	}
	default:
		return false;
	}
	return false;
}

void Emulator::cbVideoRefresh(const void* data, unsigned width, unsigned height, size_t pitch) {
	assert(s_loadedEmulator);
	if (s_loadedEmulator->m_updateGeometryFromVideoRefresh && width && height) {
		s_loadedEmulator->m_avInfo.geometry.base_width = width;
		s_loadedEmulator->m_avInfo.geometry.base_height = height;
		if (s_loadedEmulator->m_avInfo.geometry.max_width < width) {
			s_loadedEmulator->m_avInfo.geometry.max_width = width;
		}
		if (s_loadedEmulator->m_avInfo.geometry.max_height < height) {
			s_loadedEmulator->m_avInfo.geometry.max_height = height;
		}
	}
	// Hardware rendering: the core is signaling that the framebuffer lives on the GPU.
	// We currently don't support GPU readback here; ignore and keep m_imgData null.
	if (data == RETRO_HW_FRAME_BUFFER_VALID) {
		s_loadedEmulator->m_imgData = nullptr;
		s_loadedEmulator->m_imgPitch = 0;
		return;
	}
	if (data) {
		s_loadedEmulator->m_imgData = data;
	}
	if (pitch) {
		s_loadedEmulator->m_imgPitch = pitch;
	}
}

void Emulator::cbAudioSample(int16_t left, int16_t right) {
	assert(s_loadedEmulator);
	s_loadedEmulator->m_audioData.push_back(left);
	s_loadedEmulator->m_audioData.push_back(right);
}

size_t Emulator::cbAudioSampleBatch(const int16_t* data, size_t frames) {
	assert(s_loadedEmulator);
	s_loadedEmulator->m_audioData.insert(s_loadedEmulator->m_audioData.end(), data, &data[frames * 2]);
	return frames;
}

void Emulator::cbInputPoll() {
	assert(s_loadedEmulator);
}

int16_t Emulator::cbInputState(unsigned port, unsigned, unsigned, unsigned id) {
	assert(s_loadedEmulator);
	return s_loadedEmulator->m_buttonMask[port][id];
}

void Emulator::configureData(GameData* data) {
	m_addressSpace = &data->addressSpace();
	m_addressSpace->reset();
	Retro::configureData(data, m_core);
	reconfigureAddressSpace();
	if (m_addressSpace->blocks().empty() && retro_get_memory_size(RETRO_MEMORY_SYSTEM_RAM)) {
		m_addressSpace->addBlock(Retro::ramBase(m_core), retro_get_memory_size(RETRO_MEMORY_SYSTEM_RAM), retro_get_memory_data(RETRO_MEMORY_SYSTEM_RAM));
	}
}

vector<string> Emulator::buttons() const {
	return Retro::buttons(m_core);
}

vector<string> Emulator::keybinds() const {
	return Retro::keybinds(m_core);
}
}
