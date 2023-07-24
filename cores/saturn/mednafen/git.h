#ifndef __MDFN_GIT_H
#define __MDFN_GIT_H

#include <algorithm>
#include <string>
#include <vector>
#include <map>
#include <libretro.h>

#include "video/surface.h"

#include "state.h"
#include "settings-common.h"

struct MemoryPatch;

struct CheatFormatStruct
{
 const char *FullName;		//"Game Genie", "GameShark", "Pro Action Catplay", etc.
 const char *Description;	// Whatever?

 bool (*DecodeCheat)(const std::string& cheat_string, MemoryPatch* patch);	// *patch should be left as initialized by MemoryPatch::MemoryPatch(), unless this is the
										// second(or third or whatever) part of a multipart cheat.
										//
										// Will throw an std::exception(or derivative) on format error.
										//
										// Will return true if this is part of a multipart cheat.
};

MDFN_HIDE extern const std::vector<CheatFormatStruct> CheatFormatInfo_Empty;

struct CheatInfoStruct
{
 //
 // InstallReadPatch and RemoveReadPatches should be non-NULL(even if only pointing to dummy functions) if the emulator module supports
 // read-substitution and read-substitution-with-compare style(IE Game Genie-style) cheats.
 //
 // See also "SubCheats" global stuff in mempatcher.h.
 //
 void (*InstallReadPatch)(uint32 address, uint8 value, int compare); // Compare is >= 0 when utilized.
 void (*RemoveReadPatches)(void);
 uint8 (*MemRead)(uint32 addr);
 void (*MemWrite)(uint32 addr, uint8 val);

 const std::vector<CheatFormatStruct>& CheatFormatInfo;

 bool BigEndian;	// UI default for cheat search and new cheats.
};

MDFN_HIDE extern const CheatInfoStruct CheatInfo_Empty;

struct EmulateSpecStruct
{
	// Pitch(32-bit) must be equal to width and >= the "fb_width" specified in the MDFNGI struct for the emulated system.
	// Height must be >= to the "fb_height" specified in the MDFNGI struct for the emulated system.
	// The framebuffer pointed to by surface->pixels is written to by the system emulation code.
	MDFN_Surface* surface = nullptr;

	// Set by the system emulation code every frame, to denote the horizontal and vertical offsets of the image, and the size
	// of the image.  If the emulated system sets the elements of LineWidths, then the width(w) of this structure
	// is ignored while drawing the image.
	MDFN_Rect DisplayRect = { 0, 0, 0, 0 };

	// Pointer to an array of int32, number of elements = fb_height, set by the driver code.  Individual elements written
	// to by system emulation code.  If the emulated system doesn't support multiple screen widths per frame, or if you handle
	// such a situation by outputting at a constant width-per-frame that is the least-common-multiple of the screen widths, then
	// you can ignore this.  If you do wish to use this, you must set all elements every frame.
	int32 *LineWidths = nullptr;

	// Set(optionally) by emulation code.  If InterlaceOn is true, then assume field height is 1/2 DisplayRect.h, and
	// only every other line in surface (with the start line defined by InterlacedField) has valid data
	// (it's up to internal Mednafen code to deinterlace it).
	bool InterlaceOn = false;
	bool InterlaceField = false;

	// Skip rendering this frame if true.  Set by the driver code.
	int skip = false;

	// Number of frames currently in internal sound buffer.  Set by the system emulation code, to be read by the driver code.
	int32 SoundBufSize = 0;

	// Number of cycles that this frame consumed, using MDFNGI::MasterClock as a time base.
	// Set by emulation code.
	// MasterCycles value at last MidSync(), 0 if mid sync isn't implemented for the emulation module in use.
	int64 MasterCycles = 0;
};

struct GameDB_Entry
{
 std::string GameID;
 bool GameIDIsHash = false;
 std::string Name;
 std::string Setting;
 std::string Purpose;
};

struct GameDB_Database
{
 std::string ShortName;
 std::string FullName;
 std::string Description;

 std::vector<GameDB_Entry> Entries;
};

//===========================================

typedef struct
{
 // Time base for EmulateSpecStruct::MasterCycles
 // MasterClock must be >= MDFN_MASTERCLOCK_FIXED(1.0)
 // All or part of the fractional component may be ignored in some timekeeping operations in the emulator to prevent integer overflow,
 // so it is unwise to have a fractional component when the integral component is very small(less than say, 10000).
 #define MDFN_MASTERCLOCK_FIXED(n)	((int64)((double)(n) * (1LL << 32)))
 int64 MasterClock;

 int lcm_width;
 int lcm_height;

 void *dummy_separator;	//

 int nominal_width;
 int nominal_height;

 int fb_width;		// Width of the framebuffer(not necessarily width of the image).  MDFN_Surface width should be >= this.
 int fb_height;		// Height of the framebuffer passed to the Emulate() function(not necessarily height of the image)

 uint8 MD5[16];

 // For mouse relative motion.
 double mouse_sensitivity;


 //
 // For absolute coordinates(IDIT_X_AXIS and IDIT_Y_AXIS), usually mapped to a mouse(hence the naming).
 //
 float mouse_scale_x, mouse_scale_y;
 float mouse_offs_x, mouse_offs_y; 
} MDFNGI;

//===========================================

int StateAction(StateMem *sm, int load, int data_only);

extern retro_log_printf_t log_cb;

#endif
