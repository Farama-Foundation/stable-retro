#ifndef MAME2003_H
#define MAME2003_H

#include <stdio.h>
#include <libretro.h>
#include <file/file_path.h>
#include <compat/posix_string.h>
#include "osd_cpu.h"
#include "inptport.h"

/*
 * we can't #include <retro_miscellaneous.h> to bring in PATH_MAX_LENGTH due to namespace conflicts
 * involing the DWORD define so we'll include only some useful bits for now
 */
#include <retro_inline.h>

#if defined(__CELLOS_LV2__)
#include <sys/fs_external.h>
#endif

#include <limits.h>

#ifndef PATH_MAX_LENGTH
#if defined(__CELLOS_LV2__)
#define PATH_MAX_LENGTH CELL_FS_MAX_FS_PATH_LENGTH
#elif defined(_XBOX1) || defined(_3DS) || defined(PSP) || defined(PS2) || defined(GEKKO) || defined(WIIU) || defined(ORBIS)
#define PATH_MAX_LENGTH 512
#else
#define PATH_MAX_LENGTH 4096
#endif
#endif

/*
 * end of retro_miscellaneous.h bits
 */

#ifdef __cplusplus
extern "C" {
#endif

/* The Win32 port requires this constant for variable arg routines. */
#ifndef CLIB_DECL
#define CLIB_DECL
#endif

#ifdef __LP64__
#define FPTR unsigned long   /* 64bit: sizeof(void *) is sizeof(long)  */
#else
#define FPTR unsigned int
#endif

/***************************************************************************

	Parameters

***************************************************************************/

#define APPNAME           "mame2003-plus"

#define FRAMES_PER_FPS_UPDATE         12
#define MAX_GFX_ELEMENTS              32
#define MAX_MEMORY_REGIONS            32

#define LIBRETRO_ANALOG_MIN       -32768
#define LIBRETRO_ANALOG_MAX        32767
#define MAME_ANALOG_MIN             -128
#define MAME_ANALOG_MAX              128

#define INPUT_BUTTON_AXIS_THRESHOLD   64

enum
{
  X_AXIS = 0,
  Y_AXIS,
  Z_AXIS,
  PEDAL_AXIS,
  MAX_ANALOG_AXES
};

enum
{
  IDX_CLASSIC = 0,
  IDX_FIGHTSTICK,
  IDX_8BUTTON,
  IDX_6BUTTON,
  IDX_NUMBER_OF_INPUT_TYPES
};

enum /* the "display numbers" for each player, as opposed to their array index */
{
  DISP_PLAYER1 = 1,
  DISP_PLAYER2,
  DISP_PLAYER3,
  DISP_PLAYER4,
  DISP_PLAYER5,
  DISP_PLAYER6,
  DISP_PLAYER7,
  DISP_PLAYER8
};

#define MAX_PLAYER_COUNT  DISP_PLAYER8   /* We currently support a maximum of eight simultaneous players */

/******************************************************************************

    The following is a set of OS joystick codes (also including buttons and controls
    on mice, lightguns, etc). In MAME 2003+, the libretro API takes the role of the
    MAME OSD and these codes are used to represent the full range of input states
    that can exist among any of the libretro API abstractions that can be used.

    The names for elements of the enum reflect the fact that these codes parallel
    input codes in libretro.h; because each of the libretro input abstractions uses
    independent, overlapping code ranges, we cannot simply reuse the libretro codes.

******************************************************************************/

enum
{
  OSD_JOYPAD_B = 0,
  OSD_JOYPAD_Y,
  OSD_JOYPAD_SELECT,
  OSD_JOYPAD_START,
  OSD_JOYPAD_UP,
  OSD_JOYPAD_DOWN,
  OSD_JOYPAD_LEFT,
  OSD_JOYPAD_RIGHT,
  OSD_JOYPAD_A,
  OSD_JOYPAD_X,
  OSD_JOYPAD_L,
  OSD_JOYPAD_R,
  OSD_JOYPAD_L2,
  OSD_JOYPAD_R2,
  OSD_JOYPAD_L3,
  OSD_JOYPAD_R3,
  OSD_MOUSE_BUTTON_1,
  OSD_MOUSE_BUTTON_2,
  OSD_MOUSE_BUTTON_3,
  OSD_MOUSE_BUTTON_4,
  OSD_MOUSE_BUTTON_5,
  OSD_ANALOG_LEFT_NEGATIVE_X,
  OSD_ANALOG_LEFT_POSITIVE_X,
  OSD_ANALOG_LEFT_NEGATIVE_Y,
  OSD_ANALOG_LEFT_POSITIVE_Y,
  OSD_ANALOG_RIGHT_NEGATIVE_X,
  OSD_ANALOG_RIGHT_POSITIVE_X,
  OSD_ANALOG_RIGHT_NEGATIVE_Y,
  OSD_ANALOG_RIGHT_POSITIVE_Y,
  OSD_LIGHTGUN_IS_OFFSCREEN,
  OSD_LIGHTGUN_IS_TRIGGER,      /*Status Check*/
  OSD_LIGHTGUN_RELOAD,          /*Forced off-screen shot*/
  OSD_LIGHTGUN_AUX_A,
  OSD_LIGHTGUN_AUX_B,
  OSD_LIGHTGUN_START,
  OSD_LIGHTGUN_SELECT,
  OSD_LIGHTGUN_AUX_C,
  OSD_LIGHTGUN_DPAD_UP,
  OSD_LIGHTGUN_DPAD_DOWN,
  OSD_LIGHTGUN_DPAD_LEFT,
  OSD_LIGHTGUN_DPAD_RIGHT,
  OSD_INPUT_CODES_PER_PLAYER
};

/******************************************************************************

	Shared libretro log interface
    set in mame2003.c

******************************************************************************/
extern retro_log_printf_t log_cb;


/******************************************************************************

	frontend message interface
    implemented in mame2003.c

******************************************************************************/
extern void frontend_message_cb(const char *message_string, unsigned frames_to_display);


/******************************************************************************

  Core options

******************************************************************************/
void set_content_flags(void);
void init_core_options(void);
void update_variables(bool first_time);

enum CORE_OPTIONS  /* controls the order in which core options appear. common, important, and content-specific options should go earlier on the list */
{
  OPT_SKIP_DISCLAIMER = 0,
  OPT_SKIP_WARNINGS,
  OPT_DISPLAY_SETUP,
  OPT_MAME_REMAPPING,
  OPT_AUTOSAVE_HISCORE,
  OPT_NVRAM_BOOTSTRAP,
  OPT_CORE_SYS_SUBFOLDER,
  OPT_CORE_SAVE_SUBFOLDER,
  OPT_XY_DEVICE,
  OPT_INPUT_INTERFACE,
  OPT_4WAY,
  OPT_CROSSHAIR_ENABLED,
  OPT_CROSSHAIR_APPEARANCE,
  OPT_SHARE_DIAL,
  OPT_DIAL_SWAP_XY,
  OPT_CHEAT_INPUT_PORTS,
  OPT_OVERRIDE_AD_STICK,
  OPT_INPUT_TOGGLE,
  OPT_DIGITAL_JOY_CENTERING,
  OPT_USE_SAMPLES,
  OPT_USE_ALT_SOUND,
  OPT_SAMPLE_RATE,
  OPT_BRIGHTNESS,
  OPT_GAMMA,
  OPT_TATE_MODE,
  OPT_FRAMESKIP,
  OPT_ARTWORK,
  OPT_ART_RESOLUTION,
  OPT_ART_OVERLAY_OPACITY,
  OPT_VECTOR_RESOLUTION,
  OPT_VECTOR_ANTIALIAS,
  OPT_VECTOR_BEAM,
  OPT_VECTOR_TRANSLUCENCY,
  OPT_VECTOR_FLICKER,
  OPT_VECTOR_INTENSITY,
  OPT_NEOGEO_BIOS,
  OPT_STV_BIOS,
  OPT_CPU_CLOCK_SCALE,
#if (HAS_CYCLONE || HAS_DRZ80)
  OPT_CYCLONE_MODE,
#endif
  OPT_end /* dummy last entry */
};


/******************************************************************************

	Display

******************************************************************************/

/* mame_bitmap used to be declared here, but has moved to common.c */
/* sadly, the include order requires that at least this forward declaration is here */
struct mame_bitmap;
struct mame_display;
struct performance_info;
struct rectangle;
struct rom_load_data;


/* these are the parameters passed into osd_create_display */
struct osd_create_params
{
	int width, height;			/* width and height */
	int aspect_x, aspect_y;		/* aspect ratio X:Y */
	int depth;					/* depth, either 16(palette), 15(RGB) or 32(RGB) */
	int colors;					/* colors in the palette (including UI) */
	float fps;					/* frame rate */
	int video_attributes;		/* video flags from driver */
	int orientation;			/* orientation requested by the user */
};



/*
  Create a display screen, or window, of the given dimensions (or larger). It is
  acceptable to create a smaller display if necessary, in that case the user must
  have a way to move the visibility window around.

  The params contains all the information the
  Attributes are the ones defined in driver.h, they can be used to perform
  optimizations, e.g. dirty rectangle handling if the game supports it, or faster
  blitting routines with fixed palette if the game doesn't change the palette at
  run time. The VIDEO_PIXEL_ASPECT_RATIO flags should be honored to produce a
  display of correct proportions.
  Orientation is the screen orientation (as defined in driver.h) which will be done
  by the core. This can be used to select thinner screen modes for vertical games
  (ORIENTATION_SWAP_XY set), or even to ask the user to rotate the monitor if it's
  a pivot model. Note that the OS dependent code must NOT perform any rotation,
  this is done entirely in the core.
  Depth can be 8 or 16 for palettized modes, meaning that the core will store in the
  bitmaps logical pens which will have to be remapped through a palette at blit time,
  and 15 or 32 for direct mapped modes, meaning that the bitmaps will contain RGB
  triplets (555 or 888). For direct mapped modes, the VIDEO_RGB_DIRECT flag is set
  in the attributes field.

  Returns 0 on success.
*/
int osd_create_display(const struct osd_create_params *params, UINT32 *rgb_components);



/* osd_close_display is implemented in video.c */
void osd_close_display(void);


/* defined in video.c */
void mame2003_video_get_geometry(struct retro_game_geometry *geom);


/*
  osd_skip_this_frame() must return 0 if the current frame will be displayed.
  This can be used by drivers to skip cpu intensive processing for skipped
  frames, so the function must return a consistent result throughout the
  current frame. The function MUST NOT check timers and dynamically determine
  whether to display the frame: such calculations must be done in
  osd_update_video_and_audio(), and they must affect the FOLLOWING frames, not
  the current one. At the end of osd_update_video_and_audio(), the code must
  already know exactly whether the next frame will be skipped or not.
*/
int osd_skip_this_frame(void);


/*
  Update video and audio. game_bitmap contains the game display, while
  debug_bitmap an image of the debugger window (if the debugger is active; NULL
  otherwise). They can be shown one at a time, or in two separate windows,
  depending on the OS limitations. If only one is shown, the user must be able
  to toggle between the two by pressing IPT_UI_TOGGLE_DEBUG; moreover,
  osd_debugger_focus() will be used by the core to force the display of a
  specific bitmap, e.g. the debugger one when the debugger becomes active.

  leds_status is a bitmask of lit LEDs, usually player start lamps. They can be
  simulated using the keyboard LEDs, or in other ways e.g. by placing graphics
  on the window title bar.
*/
void osd_update_video_and_audio(struct mame_display *display);


/******************************************************************************

	Sound

******************************************************************************/

/*
  osd_start_audio_stream() is called at the start of the emulation to initialize
  the output stream, then osd_update_audio_stream() is called every frame to
  feed new data. osd_stop_audio_stream() is called when the emulation is stopped.

  The sample rate is fixed at Machine->sample_rate. Samples are 16-bit, signed.
  When the stream is stereo, left and right samples are alternated in the
  stream.

  osd_start_audio_stream() and osd_update_audio_stream() must return the number
  of samples (or couples of samples, when using stereo) required for next frame.
  This will be around Machine->sample_rate / Machine->drv->frames_per_second,
  the code may adjust it by SMALL AMOUNTS to keep timing accurate and to
  maintain audio and video in sync when using vsync. Note that sound emulation,
  especially when DACs are involved, greatly depends on the number of samples
  per frame to be roughly constant, so the returned value must always stay close
  to the reference value of Machine->sample_rate / Machine->drv->frames_per_second.
  Of course that value is not necessarily an integer so at least a +/- 1
  adjustment is necessary to avoid drifting over time.
*/
int osd_start_audio_stream(int stereo);
int osd_update_audio_stream(INT16 *buffer);
void osd_stop_audio_stream(void);

/******************************************************************************

	Keyboard

******************************************************************************/
/*
  return a list of all available keys (see input.h)
*/
const struct KeyboardInfo *osd_get_key_list(void);

/*
  tell whether the specified key is pressed or not. keycode is the OS dependent
  code specified in the list returned by osd_get_key_list().
*/
int osd_is_key_pressed(int keycode);

/*
  Return the Unicode value of the most recently pressed key. This
  function is used only by text-entry routines in the user interface and should
  not be used by drivers. The value returned is in the range of the first 256
  bytes of Unicode, e.g. ISO-8859-1. A return value of 0 indicates no key down.

  Set flush to 1 to clear the buffer before entering text. This will avoid
  having prior UI and game keys leak into the text entry.
*/
int osd_readkey_unicode(int flush);


/******************************************************************************

	Joystick

******************************************************************************/

/*
  return a list of all available joystick inputs (see input.h)
*/
const struct JoystickInfo *osd_get_joy_list(void);

/*
  tell whether the specified joystick direction/button is pressed or not.
  joycode is the OS dependent code specified in the list returned by
  osd_get_joy_list().
*/
int osd_is_joy_pressed(int joycode);

/* added for building joystick seq for analog inputs */
int osd_is_joystick_axis_code(unsigned joycode);

/* osd_analogjoy_read returns in the range -128 .. 128 (yes, 128, not 127) */
void osd_analogjoy_read(  int player,
                          int analog_axis[MAX_ANALOG_AXES],
                          InputCode analogjoy_input[MAX_ANALOG_AXES]  );

/******************************************************************************
 *
 * Legacy joystick calibration functions
 *
 * As of March 2021: these MAME functions should not actually be used and will not be invoked
 * as long as needs_calibration always returns 0. The libretro frontend is reponsible for
 * providing calibrated position data.
 ******************************************************************************/

/* Joystick calibration routines BW 19981216 */
int osd_joystick_needs_calibration(void);

/* Preprocessing for joystick calibration. Returns 0 on success */
void osd_joystick_start_calibration(void);

/* Prepare the next calibration step. Return a description of this step. */
/* (e.g. "move to upper left") */
const char *osd_joystick_calibrate_next(void);

/* Get the actual joystick calibration data for the current position */
void osd_joystick_calibrate(void);

/* Postprocessing (e.g. saving joystick data to config) */
void osd_joystick_end_calibration(void);


/******************************************************************************

	Trackball, Spinner, Mouse, Pointer, Lightgun

******************************************************************************/

/* Returns relative or absolute positions for various X-Y coordinate devices */
void osd_xy_device_read(int player, int *deltax, int *deltay, const char* type);


/******************************************************************************

	Utility functions

******************************************************************************/

/* inptport.c defines general purpose defaults for key and joystick bindings which
 * may be further adjusted by the OS dependent code to better match the available
 * keyboard, e.g. one could map pause to the Pause key instead of P, or snapshot
 * to PrtScr instead of F12. Of course the user can further change the settings
 * to anything they like.
 *
 * osd_customize_inputport_defaults is called on startup, before reading the
 * configuration from disk. Scan the list, and change the keys/joysticks you want.
 */
void osd_customize_inputport_defaults(struct ipd *defaults);


/******************************************************************************

	Timing

  As of March 2021, these functions are not implemented in the libretro port.

******************************************************************************/

typedef INT64 cycles_t;

/* return the current number of cycles, or some other high-resolution timer */
cycles_t osd_cycles(void);

/* return the number of cycles per second */
cycles_t osd_cycles_per_second(void);

/* return the current number of cycles, or some other high-resolution timer.
   This call must be the fastest possible because it is called by the profiler;
   it isn't necessary to know the number of ticks per seconds. */
cycles_t osd_profiling_ticks(void);

#ifdef __cplusplus
}
#endif

#endif /* MAME2003_H */
