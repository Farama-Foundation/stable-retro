/***************************************************************************

	mame.c

	Controls execution of the core MAME system.

****************************************************************************

    libretro manages:
        - platform-specific init
        - calls init_game() from retro_load_game()
        - calls run_game() from retro_load_game

	mame.c manages:
		init_game()
			- constructs the machine driver
			- calls init_game_options()

			init_game_options()
				- determines color depth from the options
				- computes orientation from the options

			- initializes the savegame system
			- calls init_machine()

			init_machine()
				- initializes the localized strings
				- initializes the input system
				- parses and allocates the game's input ports
				- initializes the hard disk system
				- loads the game's ROMs
				- resets the timer system
				- starts the refresh timer
				- initializes the CPUs
				- loads the configuration file
				- initializes the memory system for the game
				- calls the driver's DRIVER_INIT callback

			- calls run_machine()

			run_machine()
				- calls vh_open()

				vh_open()
					- allocates the palette
					- decodes the graphics
					- computes vector game resolution
					- sets up the artwork
					- calls osd_create_display() to init the display
					- allocates the scrbitmap
					- sets the initial visible_area
					- sets up buffered spriteram
					- creates the user interface font
					- creates the debugger bitmap and font
					- finishes palette initialization

				- initializes the tilemap system
				- calls the driver's VIDEO_START callback
				- starts the audio system
				- disposes of regions marked as disposable
				- calls ui_copyright_and_warnings()

				pause_action_start_emulator()
					- initializes the user interface
					- initializes the cheat system
					- calls the driver's NVRAM_HANDLER

	--------------( at this point, we're up and running )---------------------------

					- calls the driver's NVRAM_HANDLER
					- tears down the cheat system
					- saves the game's configuration

				- stops the audio system
				- calls the driver's VIDEO_STOP callback
				- tears down the tilemap system
				- calls vh_close()

				vh_close()
					- frees the decoded graphics
					- frees the fonts
					- calls osd_close_display() to shut down the display
					- tears down the artwork
					- tears down the palette system

			- calls shutdown_machine()

			shutdown_machine()
				- tears down the memory system
				- frees all the memory regions
				- tears down the hard disks
				- tears down the CPU system
				- releases the input ports
				- tears down the input system
				- tears down the localized strings
				- resets the saved state system

			- calls osd_exit() to do platform-specific cleanup

		- exits the program

***************************************************************************/

#include <ctype.h>
#include <stdarg.h>
#include <file/file_path.h>
#include "ui_text.h"
#include "mamedbg.h"
#include "artwork.h"
#include "state.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/vector.h"
#include "palette.h"
#include "harddisk.h"
#include "driver.h"
#include "mame.h"
#include "bootstrap.h"


/***************************************************************************

	Global variables

***************************************************************************/

/* handy globals for other parts of the system */
int framerate_test = 0;
void *record;	/* for -record */
void *playback; /* for -playback */
int mame_debug; /* !0 when -debug option is specified */
int bailing;	/* set to 1 if the startup is aborted to prevent multiple error messages */

extern int16_t XsoundBuffer[2048];
extern void (*pause_action)(void);

/* the active machine */
static struct RunningMachine active_machine;
struct RunningMachine *Machine = &active_machine;

/* the active game driver */
static const struct GameDriver *gamedrv;
static struct InternalMachineDriver internal_drv;

/* various game options filled in by the OSD */
struct GameOptions options;

/* the active video display */
static struct mame_display current_display;
static UINT8 visible_area_changed;

/* video updating */
static UINT8 full_refresh_pending;
static int last_partial_scanline;

/* speed computation */
static struct performance_info performance;

/* misc other statics */
static int settingsloaded;
static int leds_status;

/* artwork callbacks */
static struct artwork_callbacks mame_artwork_callbacks =
{
	NULL,
	artwork_load_artwork_file
};

static int game_loaded;



/***************************************************************************

	Hard disk interface prototype

***************************************************************************/

static struct chd_interface_file *mame_chd_open(const char *filename, const char *mode);
static void mame_chd_close(struct chd_interface_file *file);
static UINT32 mame_chd_read(struct chd_interface_file *file, UINT64 offset, UINT32 count, void *buffer);
static UINT32 mame_chd_write(struct chd_interface_file *file, UINT64 offset, UINT32 count, const void *buffer);
static UINT64 mame_chd_length(struct chd_interface_file *file);

static struct chd_interface mame_chd_interface =
{
	mame_chd_open,
	mame_chd_close,
	mame_chd_read,
	mame_chd_write,
	mame_chd_length
};



/***************************************************************************

	Other function prototypes

***************************************************************************/

static int init_machine(void);
static void shutdown_machine(void);
static int run_machine(void);
void pause_action_start_emulator(void);

#ifdef MAME_DEBUG
static int validitychecks(void);
#endif

static int vh_open(void);
static void vh_close(void);
static void init_game_options(void);
static int decode_graphics(const struct GfxDecodeInfo *gfxdecodeinfo);
static void compute_aspect_ratio(const struct InternalMachineDriver *drv, int *aspect_x, int *aspect_y);
static void scale_vectorgames(int gfx_width, int gfx_height, int *width, int *height);
static int init_buffered_spriteram(void);

/***************************************************************************

	Inline functions

***************************************************************************/

/*-------------------------------------------------
	bail_and_print - set the bailing flag and
	print a message if one hasn't already been
	printed
-------------------------------------------------*/

static INLINE void bail_and_print(const char *message)
{
	if (!bailing)
	{
		bailing = 1;
		log_cb(RETRO_LOG_ERROR, LOGPRE "%s\n", message);
	}
}




/***************************************************************************

	Core system management

***************************************************************************/

bool init_game(int game)
{
	begin_resource_tracking();

#ifdef MAME_DEBUG
	/* validity checks -- debug build only */
	if (validitychecks())
		return false;
#endif

	/* first give the machine a good cleaning */
	memset(Machine, 0, sizeof(*Machine));

	/* initialize the driver-related variables in the Machine */
	Machine->gamedrv = gamedrv = drivers[game];
	expand_machine_driver(gamedrv->drv, &internal_drv);
	Machine->drv = &internal_drv;
  return true;
}


/*-------------------------------------------------
	run_game - run the given game in a session
-------------------------------------------------*/

bool run_game(int game)
{
	init_game_options();

	/* here's the meat of it all */
	bailing = 0;

  begin_resource_tracking();

  /* finish setting up our local machine */
  if (init_machine())
      bail_and_print("Unable to initialize machine emulation");
  else
  {
  /* then run it */
      if (run_machine())
          bail_and_print("Unable to start machine emulation");
      else
      {
         game_loaded = 1;
         return 0;
      }

      /* shutdown the local machine */
      shutdown_machine();
  }
  /* stop tracking resources and exit the OSD layer */
  end_resource_tracking();

	return 1;
}

void run_game_done(void)
{
	shutdown_machine();
	end_resource_tracking();
}

/*-------------------------------------------------
	init_machine - initialize the emulated machine
-------------------------------------------------*/

static int init_machine(void)
{

	/* load the localization file */
	if (uistring_init(options.language_file) != 0)
	{
		log_cb(RETRO_LOG_ERROR, LOGPRE "uistring_init failed\n");
		goto cant_load_language_file;
	}

	/* initialize the input system */
	if (code_init() != 0)
	{
		log_cb(RETRO_LOG_ERROR, LOGPRE "code_init failed\n");
		goto cant_init_input;
	}

	/* if we have inputs, process them now */
	if (gamedrv->input_ports)
	{
		/* allocate input ports */
		Machine->input_ports = input_port_allocate(gamedrv->input_ports);
		if (!Machine->input_ports)
		{
			log_cb(RETRO_LOG_ERROR, LOGPRE "could not allocate Machine->input_ports\n");
			goto cant_allocate_input_ports;
		}

		/* allocate default input ports */
		Machine->input_ports_default = input_port_allocate(gamedrv->input_ports);
		if (!Machine->input_ports_default)
		{
			log_cb(RETRO_LOG_ERROR, LOGPRE "could not allocate Machine->input_ports_default\n");
			goto cant_allocate_input_ports_default;
		}
	}

	/* init the hard drive interface now, before attempting to load */
	chd_set_interface(&mame_chd_interface);

	/* load the ROMs if we have some */
	if (gamedrv->rom && rom_load(gamedrv->rom) != 0)
	{
		log_cb(RETRO_LOG_ERROR, LOGPRE "readroms failed\n");
		goto cant_load_roms;
	}

	/* first init the timers; some CPUs have built-in timers and will need */
	/* to allocate them up front */
	timer_init();
	cpu_init_refresh_timer();

	/* now set up all the CPUs */
	cpu_init();

	/* load input ports settings (keys, dip switches, and so on) */
	settingsloaded = load_input_port_settings();

	/* multi-session safety - set spriteram size to zero before memory map is set up */
	spriteram_size = spriteram_2_size = 0;

	/* initialize the memory system for this game */
	if (!memory_init())
	{
		log_cb(RETRO_LOG_ERROR, LOGPRE "memory_init failed\n");
		goto cant_init_memory;
	}

	/* call the game driver's init function */
	if (gamedrv->driver_init)
		(*gamedrv->driver_init)();

	return 0;

cant_init_memory:
cant_load_roms:
	input_port_free(Machine->input_ports_default);
	Machine->input_ports_default = 0;
cant_allocate_input_ports_default:
	input_port_free(Machine->input_ports);
	Machine->input_ports = 0;
cant_allocate_input_ports:
	code_close();
cant_init_input:
cant_load_language_file:
	return 1;
}



/*-------------------------------------------------
	run_machine - start the various subsystems
	and the CPU emulation; returns non zero in
	case of error
-------------------------------------------------*/

static int run_machine(void)
{
	int res = 1;

	/* start the video hardware */
	if (vh_open())
		bail_and_print("Unable to start video emulation");
	else
	{
		/* initialize tilemaps */
		tilemap_init();

		/* start up the driver's video */
		if (Machine->drv->video_start && (*Machine->drv->video_start)())
			bail_and_print("Unable to start video emulation");
		else
		{
			/* start the audio system */
			if (sound_start())
				bail_and_print("Unable to start audio emulation");
			else
			{
				int region;

				/* free memory regions allocated with REGIONFLAG_DISPOSE (typically gfx roms) */
				for (region = 0; region < MAX_MEMORY_REGIONS; region++)
					if (Machine->memory_region[region].flags & ROMREGION_DISPOSE)
					{
						int i;

						/* invalidate contents to avoid subtle bugs */
						for (i = 0; i < memory_region_length(region); i++)
							memory_region(region)[i] = rand();
						free(Machine->memory_region[region].base);
						Machine->memory_region[region].base = 0;
					}

				ui_copyright_and_warnings();
				pause_action_start_emulator(); /* this needs call before retrorun else serialization can fail on different sizes */
				return 0;
			}

			/* shut down the driver's video and kill and artwork */
			if (Machine->drv->video_stop)
				(*Machine->drv->video_stop)();
		}

		/* close down the tilemap and video systems */
		tilemap_close();
		vh_close();
	}

	return res;
}

void run_machine_done(void)
{
	sound_stop();

    /* shut down the driver's video and kill and artwork */
    if (Machine->drv->video_stop)
        (*Machine->drv->video_stop)();

    /* close down the tilemap and video systems */
    tilemap_close();
    vh_close();
}

void pause_action_start_emulator(void)
{
  init_user_interface();
  artwork_enable(1);
  InitCheat();

  /* load the NVRAM now */
  if (Machine->drv->nvram_handler)
  {
    mame_file *nvram_file = mame_fopen(Machine->gamedrv->name, 0, FILETYPE_NVRAM, 0);

    if(!nvram_file)
      log_cb(RETRO_LOG_INFO, LOGPRE "First run: NVRAM handler found for %s but no existing NVRAM file found.\n", Machine->gamedrv->name);

    log_cb(RETRO_LOG_INFO, LOGPRE "options.nvram_bootstrap: %i \n", options.nvram_bootstrap);
    if(!nvram_file && (Machine->gamedrv->bootstrap != NULL))
    {
      if(options.nvram_bootstrap)
      {
        log_cb(RETRO_LOG_INFO, LOGPRE "Spwaning NVRAM bootstrap as the initial NVRAM image.\n");
        nvram_file = spawn_bootstrap_nvram(Machine->gamedrv->bootstrap->data, Machine->gamedrv->bootstrap->length);
      }
      else
        log_cb(RETRO_LOG_INFO, LOGPRE "NVRAM bootstrap available, but disabled via core option.\n");
    }
    else
      log_cb(RETRO_LOG_INFO, LOGPRE "Delegating population of initial NVRAM to emulated system.\n");

    (*Machine->drv->nvram_handler)(nvram_file, 0);
    if (nvram_file)
        mame_fclose(nvram_file);
  }

  /* run the emulation! */
  cpu_run();

  /* Unpause */
  pause_action = 0;
}

void run_machine_core_done(void)
{
    /* save the NVRAM */
    if (Machine->drv->nvram_handler)
    {
        mame_file *nvram_file = mame_fopen(Machine->gamedrv->name, 0, FILETYPE_NVRAM, 1);
        if (nvram_file != NULL)
        {
            (*Machine->drv->nvram_handler)(nvram_file, 1);
            mame_fclose(nvram_file);
        }
    }

    StopCheat();

    /* save input ports settings */
    save_input_port_settings();
}

/*-------------------------------------------------
	shutdown_machine - tear down the emulated
	machine
-------------------------------------------------*/

static void shutdown_machine(void)
{
	int i;

	/* release any allocated memory */
	memory_shutdown();

	/* free the memory allocated for various regions */
	for (i = 0; i < MAX_MEMORY_REGIONS; i++)
		free_memory_region(i);

	/* close all hard drives */
	chd_close_all();

	/* reset the CPU system */
	cpu_exit();

	/* free the memory allocated for input ports definition */
	input_port_free(Machine->input_ports);
	input_port_free(Machine->input_ports_default);

	/* close down the input system */
	code_close();

	/* reset the saved states */
	state_save_reset();
}

/*-------------------------------------------------
	expand_machine_driver - construct a machine
	driver from the macroized state
-------------------------------------------------*/

void expand_machine_driver(void (*constructor)(struct InternalMachineDriver *), struct InternalMachineDriver *output)
{
	/* keeping this function allows us to pre-init the driver before constructing it */
	memset(output, 0, sizeof(*output));
	(*constructor)(output);
}



/*-------------------------------------------------
	vh_open - start up the video system
-------------------------------------------------*/

static int vh_open(void)
{
	struct osd_create_params params;
	struct artwork_callbacks *artcallbacks;
	int bmwidth = Machine->drv->screen_width;
	int bmheight = Machine->drv->screen_height;

	/* first allocate the necessary palette structures */
	if (palette_start())
		goto cant_start_palette;

	/* convert the gfx ROMs into character sets. This is done BEFORE calling the driver's */
	/* palette_init() routine because it might need to check the Machine->gfx[] data */
	if (Machine->drv->gfxdecodeinfo)
		if (decode_graphics(Machine->drv->gfxdecodeinfo))
			goto cant_decode_graphics;

	/* if we're a vector game, override the screen width and height */
	if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
    {
       scale_vectorgames(options.vector_width, options.vector_height, &bmwidth, &bmheight);
    }
	/* compute the visible area for raster games */
	if (!(Machine->drv->video_attributes & VIDEO_TYPE_VECTOR))
	{
		params.width = Machine->drv->default_visible_area.max_x - Machine->drv->default_visible_area.min_x + 1;
		params.height = Machine->drv->default_visible_area.max_y - Machine->drv->default_visible_area.min_y + 1;
	}
	else
	{
		params.width = bmwidth;
		params.height = bmheight;
	}

	/* fill in the rest of the display parameters */
	compute_aspect_ratio(Machine->drv, &params.aspect_x, &params.aspect_y);
	params.depth = Machine->color_depth;
	params.colors = palette_get_total_colors_with_ui();
	params.fps = Machine->drv->frames_per_second;
	params.video_attributes = Machine->drv->video_attributes;
	params.orientation = Machine->orientation;
	artcallbacks = &mame_artwork_callbacks;

	/* initialize the display through the artwork (and eventually the OSD) layer */
	if (artwork_create_display(&params, direct_rgb_components, artcallbacks))
		goto cant_create_display;

	/* the create display process may update the vector width/height, so recompute */
	if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
    {
        scale_vectorgames(options.vector_width, options.vector_height, &bmwidth, &bmheight);
    }


	/* now allocate the screen bitmap */
	Machine->scrbitmap = auto_bitmap_alloc_depth(bmwidth, bmheight, Machine->color_depth);
	if (!Machine->scrbitmap)
		goto cant_create_scrbitmap;

	/* set the default visible area */
	set_visible_area(0,1,0,1);	/* make sure everything is recalculated on multiple runs */
	set_visible_area(
			Machine->drv->default_visible_area.min_x,
			Machine->drv->default_visible_area.max_x,
			Machine->drv->default_visible_area.min_y,
			Machine->drv->default_visible_area.max_y);

	/* create spriteram buffers if necessary */
	if (Machine->drv->video_attributes & VIDEO_BUFFERS_SPRITERAM)
		if (init_buffered_spriteram())
			goto cant_init_buffered_spriteram;

	/* build our private user interface font */
	/* This must be done AFTER osd_create_display() so the function knows the */
	/* resolution we are running at and can pick a different font depending on it. */
	/* It must be done BEFORE palette_init() because that will also initialize */
	/* (through osd_allocate_colors()) the uifont colortable. */
	Machine->uifont = builduifont();
	if (Machine->uifont == NULL)
		goto cant_build_uifont;

#ifdef MAME_DEBUG
	/* if the debugger is enabled, initialize its bitmap and font */
	if (mame_debug)
	{
		int depth = options.debug_depth ? options.debug_depth : Machine->color_depth;

		/* first allocate the debugger bitmap */
		Machine->debug_bitmap = auto_bitmap_alloc_depth(options.debug_width, options.debug_height, depth);
		if (!Machine->debug_bitmap)
			goto cant_create_debug_bitmap;

		/* then create the debugger font */
		Machine->debugger_font = build_debugger_font();
		if (Machine->debugger_font == NULL)
			goto cant_build_debugger_font;
	}
#endif

	/* initialize the palette - must be done after osd_create_display() */
	if (palette_init())
		goto cant_init_palette;

	/* force the first update to be full */
	set_vh_global_attribute(NULL, 0);

	/* reset video statics and get out of here */
	pdrawgfx_shadow_lowpri = 0;
	leds_status = 0;

	return 0;

cant_init_palette:

#ifdef MAME_DEBUG
cant_build_debugger_font:
cant_create_debug_bitmap:
#endif

cant_build_uifont:
cant_init_buffered_spriteram:
cant_create_scrbitmap:
cant_create_display:
cant_decode_graphics:
cant_start_palette:
	vh_close();
	return 1;
}



/*-------------------------------------------------
	vh_close - close down the video system
-------------------------------------------------*/

static void vh_close(void)
{
	int i;

	/* free all the graphics elements */
	for (i = 0; i < MAX_GFX_ELEMENTS; i++)
	{
		freegfx(Machine->gfx[i]);
		Machine->gfx[i] = 0;
	}

	/* free the font elements */
	if (Machine->uifont)
	{
		freegfx(Machine->uifont);
		Machine->uifont = NULL;
	}
	if (Machine->debugger_font)
	{
		freegfx(Machine->debugger_font);
		Machine->debugger_font = NULL;
	}

	/* close down the OSD layer's display */
	osd_close_display();
}



/*-------------------------------------------------
	compute_aspect_ratio - determine the aspect
	ratio encoded in the video attributes
-------------------------------------------------*/

static void compute_aspect_ratio(const struct InternalMachineDriver *drv, int *aspect_x, int *aspect_y)
{
	/* if it's explicitly specified, use it */
	if (drv->aspect_x && drv->aspect_y)
	{
		*aspect_x = drv->aspect_x;
		*aspect_y = drv->aspect_y;
	}

	/* otherwise, attempt to deduce the result */
	else if (!(drv->video_attributes & VIDEO_DUAL_MONITOR))
	{
		*aspect_x = 4;
		*aspect_y = (drv->video_attributes & VIDEO_DUAL_MONITOR) ? 6 : 3;
	}
}



/*-------------------------------------------------
	init_game_options - initialize the various
	game options
-------------------------------------------------*/

static void init_game_options(void)
{
  /* copy some settings into easier-to-handle variables */
  record	   = options.record;
  playback   = options.playback;
  mame_debug = options.mame_debug;

  /* determine the color depth */
  Machine->color_depth = 16;
  alpha_active = 0;
  if (Machine->drv->video_attributes & VIDEO_RGB_DIRECT)
  {
    /* first pick a default */
    if (Machine->drv->video_attributes & VIDEO_NEEDS_6BITS_PER_GUN)
      Machine->color_depth = 32;
    else
      Machine->color_depth = 15;

    /* use 32-bit color output as default to skip color conversions */
	if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR) 	Machine->color_depth = 32;

    /* now allow overrides */
    if (options.color_depth == 15 || options.color_depth == 32)
      Machine->color_depth = options.color_depth;

    /* enable alpha for direct video modes */
    alpha_active = 1;
    alpha_init();
  }

  /* update the vector width/height with libretro settings or use the default */
  if (options.vector_width  == 0) options.vector_width  = Machine->drv->screen_width;
  if (options.vector_height == 0) options.vector_height = Machine->drv->screen_height;



  /* get orientation right */
  Machine->orientation    = ROT0;
  Machine->ui_orientation = options.ui_orientation;

  Machine->sample_rate = options.samplerate;
 
}



/*-------------------------------------------------
	decode_graphics - decode the graphics
-------------------------------------------------*/

static int decode_graphics(const struct GfxDecodeInfo *gfxdecodeinfo)
{
	int i;

	/* loop over all elements */
	for (i = 0; i < MAX_GFX_ELEMENTS && gfxdecodeinfo[i].memory_region != -1; i++)
	{
		int region_length = 8 * memory_region_length(gfxdecodeinfo[i].memory_region);
		UINT8 *region_base = memory_region(gfxdecodeinfo[i].memory_region);
		struct GfxLayout glcopy;
		int j;

		/* make a copy of the layout */
		glcopy = *gfxdecodeinfo[i].gfxlayout;

		/* if the character count is a region fraction, compute the effective total */
		if (IS_FRAC(glcopy.total))
			glcopy.total = region_length / glcopy.charincrement * FRAC_NUM(glcopy.total) / FRAC_DEN(glcopy.total);

		/* loop over all the planes, converting fractions */
		for (j = 0; j < MAX_GFX_PLANES; j++)
		{
			int value = glcopy.planeoffset[j];
			if (IS_FRAC(value))
				glcopy.planeoffset[j] = FRAC_OFFSET(value) + region_length * FRAC_NUM(value) / FRAC_DEN(value);
		}

		/* loop over all the X/Y offsets, converting fractions */
		for (j = 0; j < MAX_GFX_SIZE; j++)
		{
			int value = glcopy.xoffset[j];
			if (IS_FRAC(value))
				glcopy.xoffset[j] = FRAC_OFFSET(value) + region_length * FRAC_NUM(value) / FRAC_DEN(value);

			value = glcopy.yoffset[j];
			if (IS_FRAC(value))
				glcopy.yoffset[j] = FRAC_OFFSET(value) + region_length * FRAC_NUM(value) / FRAC_DEN(value);
		}

		/* some games increment on partial tile boundaries; to handle this without reading */
		/* past the end of the region, we may need to truncate the count */
		/* an example is the games in metro.c */
		if (glcopy.planeoffset[0] == GFX_RAW)
		{
			int base = gfxdecodeinfo[i].start;
			int end = region_length/8;
			while (glcopy.total > 0)
			{
				int elementbase = base + (glcopy.total - 1) * glcopy.charincrement / 8;
				int lastpixelbase = elementbase + glcopy.height * glcopy.yoffset[0] / 8 - 1;
				if (lastpixelbase < end)
					break;
				glcopy.total--;
			}
		}

		/* now decode the actual graphics */
		if ((Machine->gfx[i] = decodegfx(region_base + gfxdecodeinfo[i].start, &glcopy)) == 0)
		{
			bailing = 1;
			log_cb(RETRO_LOG_ERROR, LOGPRE "Out of memory decoding gfx\n");
			return 1;
		}

		/* if we have a remapped colortable, point our local colortable to it */
		if (Machine->remapped_colortable)
			Machine->gfx[i]->colortable = &Machine->remapped_colortable[gfxdecodeinfo[i].color_codes_start];
		Machine->gfx[i]->total_colors = gfxdecodeinfo[i].total_color_codes;
	}
	return 0;
}



/*-------------------------------------------------
	scale_vectorgames - scale the vector games
	to a given resolution
-------------------------------------------------*/

static void scale_vectorgames(int gfx_width, int gfx_height, int *width, int *height)
{
	double x_scale, y_scale, scale;

	/* compute the scale values */
	x_scale = (double)gfx_width / (double)(*width);
	y_scale = (double)gfx_height / (double)(*height);

	/* pick the smaller scale factor */
	scale = (x_scale < y_scale) ? x_scale : y_scale;

	/* compute the new size */
	*width = (int)((double)*width * scale);
	*height = (int)((double)*height * scale);

	/* round to the nearest 4 pixel value */

}



/*-------------------------------------------------
	init_buffered_spriteram - initialize the
	double-buffered spriteram
-------------------------------------------------*/

static int init_buffered_spriteram(void)
{
	/* make sure we have a valid size */
	if (spriteram_size == 0)
	{
		log_cb(RETRO_LOG_ERROR, LOGPRE "vh_open():  Video buffers spriteram but spriteram_size is 0\n");
		return 0;
	}

	/* allocate memory for the back buffer */
	buffered_spriteram = auto_malloc(spriteram_size);
	if (!buffered_spriteram)
		return 1;

	/* register for saving it */
	state_save_register_UINT8("generic_video", 0, "buffered_spriteram", buffered_spriteram, spriteram_size);

	/* do the same for the secon back buffer, if present */
	if (spriteram_2_size)
	{
		/* allocate memory */
		buffered_spriteram_2 = auto_malloc(spriteram_2_size);
		if (!buffered_spriteram_2)
			return 1;

		/* register for saving it */
		state_save_register_UINT8("generic_video", 0, "buffered_spriteram_2", buffered_spriteram_2, spriteram_2_size);
	}

	/* make 16-bit and 32-bit pointer variants */
	buffered_spriteram16 = (data16_t *)buffered_spriteram;
	buffered_spriteram32 = (data32_t *)buffered_spriteram;
	buffered_spriteram16_2 = (data16_t *)buffered_spriteram_2;
	buffered_spriteram32_2 = (data32_t *)buffered_spriteram_2;
	return 0;
}



/***************************************************************************

	Screen rendering and management.

***************************************************************************/

/*-------------------------------------------------
	set_visible_area - adjusts the visible portion
	of the bitmap area dynamically
-------------------------------------------------*/

void set_visible_area(int min_x, int max_x, int min_y, int max_y)
{
	if (       Machine->visible_area.min_x == min_x
			&& Machine->visible_area.max_x == max_x
			&& Machine->visible_area.min_y == min_y
			&& Machine->visible_area.max_y == max_y)
		return;

	/* "dirty" the area for the next display update */
	visible_area_changed = 1;

	/* set the new values in the Machine struct */
	Machine->visible_area.min_x = min_x;
	Machine->visible_area.max_x = max_x;
	Machine->visible_area.min_y = min_y;
	Machine->visible_area.max_y = max_y;

	/* vector games always use the whole bitmap */
	if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
	{
		Machine->absolute_visible_area.min_x = 0;
		Machine->absolute_visible_area.max_x = Machine->scrbitmap->width - 1;
		Machine->absolute_visible_area.min_y = 0;
		Machine->absolute_visible_area.max_y = Machine->scrbitmap->height - 1;
	}

	/* raster games need to use the visible area */
	else
		Machine->absolute_visible_area = Machine->visible_area;

	/* recompute scanline timing */
	cpu_compute_scanline_timing();
}



/*-------------------------------------------------
	schedule_full_refresh - force a full erase
	and refresh the next frame
-------------------------------------------------*/

void schedule_full_refresh(void)
{
	full_refresh_pending = 1;
}



/*-------------------------------------------------
	reset_partial_updates - reset the partial
	updating mechanism for a new frame
-------------------------------------------------*/

void reset_partial_updates(void)
{
	last_partial_scanline = 0;
	performance.partial_updates_this_frame = 0;
}



/*-------------------------------------------------
	force_partial_update - perform a partial
	update from the last scanline up to and
	including the specified scanline
-------------------------------------------------*/

void force_partial_update(int scanline)
{
	struct rectangle clip = Machine->visible_area;

	/* if skipping this frame, bail */
	if (osd_skip_this_frame())
		return;

	/* skip if less than the lowest so far */
	if (scanline < last_partial_scanline)
		return;

	/* if there's a dirty bitmap and we didn't do any partial updates yet, handle it now */
	if (full_refresh_pending && last_partial_scanline == 0)
	{
		fillbitmap(Machine->scrbitmap, get_black_pen(), NULL);
		full_refresh_pending = 0;
	}

	/* set the start/end scanlines */
	if (last_partial_scanline > clip.min_y)
		clip.min_y = last_partial_scanline;
	if (scanline < clip.max_y)
		clip.max_y = scanline;

	/* render if necessary */
	if (clip.min_y <= clip.max_y)
	{
		profiler_mark(PROFILER_VIDEO);
		(*Machine->drv->video_update)(Machine->scrbitmap, &clip);
		performance.partial_updates_this_frame++;
		profiler_mark(PROFILER_END);
	}

	/* remember where we left off */
	last_partial_scanline = scanline + 1;
}



/*-------------------------------------------------
	draw_screen - render the final screen bitmap
	and update any artwork
-------------------------------------------------*/
int gbPriorityBitmapIsDirty;

void draw_screen(void)
{
	/* finish updating the screen */
	force_partial_update(Machine->visible_area.max_y);
	if( gbPriorityBitmapIsDirty )
	{
		fillbitmap( priority_bitmap, 0x00, NULL );
		gbPriorityBitmapIsDirty = 0;
	}
}


/*-------------------------------------------------
	update_video_and_audio - actually call the
	OSD layer to perform an update
-------------------------------------------------*/

void update_video_and_audio(void)
{
	int skipped_it = osd_skip_this_frame();

#ifdef MAME_DEBUG
	debug_trace_delay = 0;
#endif

	/* fill in our portion of the display */
	current_display.changed_flags = 0;

	/* set the main game bitmap */
	current_display.game_bitmap = Machine->scrbitmap;
	current_display.game_bitmap_update = Machine->absolute_visible_area;
	if (!skipped_it)
		current_display.changed_flags |= GAME_BITMAP_CHANGED;

	/* set the visible area */
	current_display.game_visible_area = Machine->absolute_visible_area;
	if (visible_area_changed)
		current_display.changed_flags |= GAME_VISIBLE_AREA_CHANGED;

	/* set the vector dirty list */
	if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
		if (!full_refresh_pending && !ui_dirty && !skipped_it)
		{
			current_display.vector_dirty_pixels = vector_dirty_list;
			current_display.changed_flags |= VECTOR_PIXELS_CHANGED;
		}

#ifdef MAME_DEBUG
	/* set the debugger bitmap */
	current_display.debug_bitmap = Machine->debug_bitmap;
	if (debugger_bitmap_changed)
		current_display.changed_flags |= DEBUG_BITMAP_CHANGED;
	debugger_bitmap_changed = 0;

	/* adjust the debugger focus */
	if (debugger_focus != current_display.debug_focus)
	{
		current_display.debug_focus = debugger_focus;
		current_display.changed_flags |= DEBUG_FOCUS_CHANGED;
	}
#endif

	/* set the LED status */
	if (leds_status != current_display.led_state)
	{
		current_display.led_state = leds_status;
		current_display.changed_flags |= LED_STATE_CHANGED;
	}

	/* update with data from other parts of the system */
	palette_update_display(&current_display);

	/* render */
	artwork_update_video_and_audio(&current_display);

	/* reset dirty flags */
	visible_area_changed = 0;
	if (ui_dirty) ui_dirty--;
}



/*-------------------------------------------------
	updatescreen - handle frameskipping and UI,
	plus updating the screen during normal
	operations
-------------------------------------------------*/

int updatescreen(void)
{
	/* update sound */
	sound_update();

	/* if we're not skipping this frame, draw the screen */
	if (osd_skip_this_frame() == 0)
	{
		profiler_mark(PROFILER_VIDEO);
		draw_screen();
		profiler_mark(PROFILER_END);
	}

	/* the user interface must be called between vh_update() and osd_update_video_and_audio(), */
	/* to allow it to overlay things on the game display. We must call it even */
	/* if the frame is skipped, to keep a consistent timing. */
	if (handle_user_interface(artwork_get_ui_bitmap()))
		/* quit if the user asked to */
		return 1;

	/* blit to the screen */
	update_video_and_audio();

	/* call the end-of-frame callback */
	if (Machine->drv->video_eof)
	{
		profiler_mark(PROFILER_VIDEO);
		(*Machine->drv->video_eof)();
		profiler_mark(PROFILER_END);
	}

	return 0;
}



/*-------------------------------------------------
	set_led_status - set the state of a given LED
-------------------------------------------------*/

void set_led_status(int num, int on)
{
	if (on)
		leds_status |=	(1 << num);
	else
		leds_status &= ~(1 << num);
}



/*-------------------------------------------------
	mame_get_performance_info - return performance
	info
-------------------------------------------------*/

const struct performance_info *mame_get_performance_info(void)
{
	return &performance;
}



/*-------------------------------------------------
	mame_find_cpu_index - return the index of the
	given CPU, or -1 if not found
-------------------------------------------------*/

int mame_find_cpu_index(const char *tag)
{
	int cpunum;

	for (cpunum = 0; cpunum < MAX_CPU; cpunum++)
		if (Machine->drv->cpu[cpunum].tag && strcmp(Machine->drv->cpu[cpunum].tag, tag) == 0)
			return cpunum;

	return -1;
}



/*-------------------------------------------------
	machine_add_cpu - add a CPU during machine
	driver expansion
-------------------------------------------------*/

struct MachineCPU *machine_add_cpu(struct InternalMachineDriver *machine, const char *tag, int type, int cpuclock)
{
	int cpunum;

	for (cpunum = 0; cpunum < MAX_CPU; cpunum++)
		if (machine->cpu[cpunum].cpu_type == 0)
		{
			machine->cpu[cpunum].tag = tag;
			machine->cpu[cpunum].cpu_type = type;
			machine->cpu[cpunum].cpu_clock = cpuclock;
			return &machine->cpu[cpunum];
		}

	log_cb(RETRO_LOG_ERROR, LOGPRE "Out of CPU's!\n");
	return NULL;
}



/*-------------------------------------------------
	machine_find_cpu - find a tagged CPU during
	machine driver expansion
-------------------------------------------------*/

struct MachineCPU *machine_find_cpu(struct InternalMachineDriver *machine, const char *tag)
{
	int cpunum;

	for (cpunum = 0; cpunum < MAX_CPU; cpunum++)
		if (machine->cpu[cpunum].tag && strcmp(machine->cpu[cpunum].tag, tag) == 0)
			return &machine->cpu[cpunum];

	log_cb(RETRO_LOG_ERROR, LOGPRE "Can't find CPU '%s'!\n", tag);
	return NULL;
}



/*-------------------------------------------------
	machine_remove_cpu - remove a tagged CPU
	during machine driver expansion
-------------------------------------------------*/

void machine_remove_cpu(struct InternalMachineDriver *machine, const char *tag)
{
	int cpunum;

	for (cpunum = 0; cpunum < MAX_CPU; cpunum++)
		if (machine->cpu[cpunum].tag && strcmp(machine->cpu[cpunum].tag, tag) == 0)
		{
			memmove(&machine->cpu[cpunum], &machine->cpu[cpunum + 1], sizeof(machine->cpu[0]) * (MAX_CPU - cpunum - 1));
			memset(&machine->cpu[MAX_CPU - 1], 0, sizeof(machine->cpu[0]));
			return;
		}

	log_cb(RETRO_LOG_ERROR, LOGPRE "Can't find CPU '%s'!\n", tag);
}



/*-------------------------------------------------
	machine_add_sound - add a sound system during
	machine driver expansion
-------------------------------------------------*/

struct MachineSound *machine_add_sound(struct InternalMachineDriver *machine, const char *tag, int type, void *sndintf)
{
	int soundnum;

	for (soundnum = 0; soundnum < MAX_SOUND; soundnum++)
		if (machine->sound[soundnum].sound_type == 0)
		{
			machine->sound[soundnum].tag = tag;
			machine->sound[soundnum].sound_type = type;
			machine->sound[soundnum].sound_interface = sndintf;
			return &machine->sound[soundnum];
		}

	log_cb(RETRO_LOG_ERROR, LOGPRE "Out of sounds!\n");
	return NULL;

}



/*-------------------------------------------------
	machine_find_sound - find a tagged sound
	system during machine driver expansion
-------------------------------------------------*/

struct MachineSound *machine_find_sound(struct InternalMachineDriver *machine, const char *tag)
{
	int soundnum;

	for (soundnum = 0; soundnum < MAX_SOUND; soundnum++)
		if (machine->sound[soundnum].tag && strcmp(machine->sound[soundnum].tag, tag) == 0)
			return &machine->sound[soundnum];

	log_cb(RETRO_LOG_ERROR, LOGPRE "Can't find sound '%s'!\n", tag);
	return NULL;
}



/*-------------------------------------------------
	machine_remove_sound - remove a tagged sound
	system during machine driver expansion
-------------------------------------------------*/

void machine_remove_sound(struct InternalMachineDriver *machine, const char *tag)
{
	int soundnum;

	for (soundnum = 0; soundnum < MAX_SOUND; soundnum++)
		if (machine->sound[soundnum].tag && strcmp(machine->sound[soundnum].tag, tag) == 0)
		{
			memmove(&machine->sound[soundnum], &machine->sound[soundnum + 1], sizeof(machine->sound[0]) * (MAX_SOUND - soundnum - 1));
			memset(&machine->sound[MAX_SOUND - 1], 0, sizeof(machine->sound[0]));
			return;
		}

	log_cb(RETRO_LOG_ERROR, LOGPRE "Can't find sound '%s'!\n", tag);
}



/*-------------------------------------------------
	mame_chd_open - interface for opening
	a hard disk image
-------------------------------------------------*/

struct chd_interface_file *mame_chd_open(const char *filename, const char *mode)
{
	/* look for read-only drives first in the ROM path */
	if (mode[0] == 'r' && !strchr(mode, '+'))
	{
		const struct GameDriver *drv;

		/* attempt reading up the chain through the parents */
		for (drv = Machine->gamedrv; drv != NULL; drv = drv->clone_of)
		{
			void* file = mame_fopen(drv->name, filename, FILETYPE_IMAGE, 0);

			if (file != NULL)
				return file;
		}

		return NULL;
	}

	/* look for read/write drives in the diff area */
	return (struct chd_interface_file *)mame_fopen(NULL, filename, FILETYPE_IMAGE_DIFF, 1);
}



/*-------------------------------------------------
	mame_chd_close - interface for closing
	a hard disk image
-------------------------------------------------*/

void mame_chd_close(struct chd_interface_file *file)
{
	mame_fclose((mame_file *)file);
}



/*-------------------------------------------------
	mame_chd_read - interface for reading
	from a hard disk image
-------------------------------------------------*/

UINT32 mame_chd_read(struct chd_interface_file *file, UINT64 offset, UINT32 count, void *buffer)
{
	mame_fseek((mame_file *)file, offset, SEEK_SET);
	return mame_fread((mame_file *)file, buffer, count);
}



/*-------------------------------------------------
	mame_chd_write - interface for writing
	to a hard disk image
-------------------------------------------------*/

UINT32 mame_chd_write(struct chd_interface_file *file, UINT64 offset, UINT32 count, const void *buffer)
{
	mame_fseek((mame_file *)file, offset, SEEK_SET);
	return mame_fwrite((mame_file *)file, buffer, count);
}


/*-------------------------------------------------
	mame_chd_length - interface for getting
	the length a hard disk image
-------------------------------------------------*/

UINT64 mame_chd_length(struct chd_interface_file *file)
{
	return mame_fsize((mame_file *)file);
}



/***************************************************************************

	Huge bunch of validity checks for the debug build

***************************************************************************/

#ifdef MAME_DEBUG

static int validitychecks(void)
{
	int i,j,cpu;
	UINT8 a,b;
	int error = 0;


	a = 0xff;
	b = a + 1;
	if (b > a)	{ log_cb(RETRO_LOG_ERROR, LOGPRE "UINT8 must be 8 bits\n"); error = 1; }

	if (sizeof(INT8)   != 1)	{ log_cb(RETRO_LOG_ERROR, LOGPRE "INT8 must be 8 bits\n"); error = 1; }
	if (sizeof(UINT8)  != 1)	{ log_cb(RETRO_LOG_ERROR, LOGPRE "UINT8 must be 8 bits\n"); error = 1; }
	if (sizeof(INT16)  != 2)	{ log_cb(RETRO_LOG_ERROR, LOGPRE "INT16 must be 16 bits\n"); error = 1; }
	if (sizeof(UINT16) != 2)	{ log_cb(RETRO_LOG_ERROR, LOGPRE "UINT16 must be 16 bits\n"); error = 1; }
	if (sizeof(INT32)  != 4)	{ log_cb(RETRO_LOG_ERROR, LOGPRE "INT32 must be 32 bits\n"); error = 1; }
	if (sizeof(UINT32) != 4)	{ log_cb(RETRO_LOG_ERROR, LOGPRE "UINT32 must be 32 bits\n"); error = 1; }
	if (sizeof(INT64)  != 8)	{ log_cb(RETRO_LOG_ERROR, LOGPRE "INT64 must be 64 bits\n"); error = 1; }
	if (sizeof(UINT64) != 8)	{ log_cb(RETRO_LOG_ERROR, LOGPRE "UINT64 must be 64 bits\n"); error = 1; }

	for (i = 0;drivers[i];i++)
	{
		struct InternalMachineDriver drv;
		const struct RomModule *romp;
		const struct InputPortTiny *inp;

		expand_machine_driver(drivers[i]->drv, &drv);

		if (drivers[i]->clone_of == drivers[i])
		{
			log_cb(RETRO_LOG_ERROR, LOGPRE "%s: %s is set as a clone of itself\n",drivers[i]->source_file,drivers[i]->name);
			error = 1;
		}

		if (drivers[i]->clone_of && drivers[i]->clone_of->clone_of)
		{
			if ((drivers[i]->clone_of->clone_of->flags & NOT_A_DRIVER) == 0)
			{
				log_cb(RETRO_LOG_ERROR, LOGPRE "%s: %s is a clone of a clone\n",drivers[i]->source_file,drivers[i]->name);
				error = 1;
			}
		}

#if 0
/*		if (drivers[i]->drv->color_table_len == drivers[i]->drv->total_colors && */
		if (drivers[i]->drv->color_table_len && drivers[i]->drv->total_colors &&
				drivers[i]->drv->vh_init_palette == 0)
		{
			log_cb(RETRO_LOG_ERROR, LOGPRE "%s: %s could use color_table_len = 0\n",drivers[i]->source_file,drivers[i]->name);
			error = 1;
		}
#endif

		for (j = i+1;drivers[j];j++)
		{
			if (!strcmp(drivers[i]->name,drivers[j]->name))
			{
				log_cb(RETRO_LOG_ERROR, LOGPRE "%s: %s is a duplicate name (%s, %s)\n",drivers[i]->source_file,drivers[i]->name,drivers[i]->source_file,drivers[j]->source_file);
				error = 1;
			}
			if (!strcmp(drivers[i]->description,drivers[j]->description))
			{
				log_cb(RETRO_LOG_ERROR, LOGPRE "%s: %s is a duplicate description (%s, %s)\n",drivers[i]->description,drivers[i]->source_file,drivers[i]->name,drivers[j]->name);
				error = 1;
			}
			if (drivers[i]->rom && drivers[i]->rom == drivers[j]->rom
					&& (drivers[i]->flags & NOT_A_DRIVER) == 0
					&& (drivers[j]->flags & NOT_A_DRIVER) == 0)
			{
				log_cb(RETRO_LOG_ERROR, LOGPRE "%s: %s and %s use the same ROM set\n",drivers[i]->source_file,drivers[i]->name,drivers[j]->name);
				error = 1;
			}
		}

		if ((drivers[i]->flags & NOT_A_DRIVER) == 0)
		{
			if (drv.sound[0].sound_type == 0 && (drivers[i]->flags & GAME_NO_SOUND) == 0 &&
					strcmp(drivers[i]->name,"minivadr"))
			{
				log_cb(RETRO_LOG_ERROR, LOGPRE "%s: %s missing GAME_NO_SOUND flag\n",drivers[i]->source_file,drivers[i]->name);
				error = 1;
			}
		}

		romp = drivers[i]->rom;

		if (romp)
		{
			int region_type_used[REGION_MAX];
			int region_length[REGION_MAX];
			const char *last_name = 0;
			int count = -1;

			for (j = 0;j < REGION_MAX;j++)
			{
				region_type_used[j] = 0;
				region_length[j] = 0;
			}

			while (!ROMENTRY_ISEND(romp))
			{
				const char *c;

				if (ROMENTRY_ISREGION(romp))
				{
					int type = ROMREGION_GETTYPE(romp);

					count++;
					if (type && (type >= REGION_MAX || type <= REGION_INVALID))
					{
						log_cb(RETRO_LOG_ERROR, LOGPRE "%s: %s has invalid ROM_REGION type %x\n",drivers[i]->source_file,drivers[i]->name,type);
						error = 1;
					}

					region_type_used[type]++;
					region_length[type] = region_length[count] = ROMREGION_GETLENGTH(romp);
				}
				if (ROMENTRY_ISFILE(romp))
				{
					const char *hash;

					last_name = c = ROM_GETNAME(romp);
					while (*c)
					{
						if (tolower(*c) != *c)
						{
							log_cb(RETRO_LOG_ERROR, LOGPRE "%s: %s has upper case ROM name %s\n",drivers[i]->source_file,drivers[i]->name,ROM_GETNAME(romp));
							error = 1;
						}
						c++;
					}

					hash = ROM_GETHASHDATA(romp);
					if (!hash_verify_string(hash))
					{
						log_cb(RETRO_LOG_ERROR, LOGPRE "%s: rom '%s' has an invalid hash string '%s'\n", drivers[i]->name, ROM_GETNAME(romp), hash);
						error = 1;
					}
				}
				if (!ROMENTRY_ISREGIONEND(romp))						/* ROM_LOAD_XXX() */
				{
					if (ROM_GETOFFSET(romp) + ROM_GETLENGTH(romp) > region_length[count])
					{
						log_cb(RETRO_LOG_ERROR, LOGPRE "%s: %s has ROM %s extending past the defined memory region\n",drivers[i]->source_file,drivers[i]->name,last_name);
						error = 1;
					}
				}
				romp++;
			}

			for (j = 1;j < REGION_MAX;j++)
			{
				if (region_type_used[j] > 1)
				{
					log_cb(RETRO_LOG_ERROR, LOGPRE "%s: %s has duplicated ROM_REGION type %x\n",drivers[i]->source_file,drivers[i]->name,j);
					error = 1;
				}
			}


			for (cpu = 0;cpu < MAX_CPU;cpu++)
			{
				if (drv.cpu[cpu].cpu_type)
				{
					int alignunit,databus_width;


					alignunit = cputype_align_unit(drv.cpu[cpu].cpu_type);
					databus_width = cputype_databus_width(drv.cpu[cpu].cpu_type);

					if (drv.cpu[cpu].memory_read)
					{
						const struct Memory_ReadAddress *mra = drv.cpu[cpu].memory_read;

						if (!IS_MEMPORT_MARKER(mra) || (mra->end & MEMPORT_DIRECTION_MASK) != MEMPORT_DIRECTION_READ)
						{
							log_cb(RETRO_LOG_ERROR, LOGPRE "%s: %s wrong MEMPORT_READ_START\n",drivers[i]->source_file,drivers[i]->name);
							error = 1;
						}

						switch (databus_width)
						{
							case 8:
								if ((mra->end & MEMPORT_WIDTH_MASK) != MEMPORT_WIDTH_8)
								{
									log_cb(RETRO_LOG_ERROR, LOGPRE "%s: %s cpu #%d uses wrong data width memory handlers! (width = %d, memory = %08x)\n",drivers[i]->source_file,drivers[i]->name,cpu,databus_width,mra->end);
									error = 1;
								}
								break;
							case 16:
								if ((mra->end & MEMPORT_WIDTH_MASK) != MEMPORT_WIDTH_16)
								{
									log_cb(RETRO_LOG_ERROR, LOGPRE "%s: %s cpu #%d uses wrong data width memory handlers! (width = %d, memory = %08x)\n",drivers[i]->source_file,drivers[i]->name,cpu,databus_width,mra->end);
									error = 1;
								}
								break;
							case 32:
								if ((mra->end & MEMPORT_WIDTH_MASK) != MEMPORT_WIDTH_32)
								{
									log_cb(RETRO_LOG_ERROR, LOGPRE "%s: %s cpu #%d uses wrong data width memory handlers! (width = %d, memory = %08x)\n",drivers[i]->source_file,drivers[i]->name,cpu,databus_width,mra->end);
									error = 1;
								}
								break;
						}

						while (!IS_MEMPORT_END(mra))
						{
							if (!IS_MEMPORT_MARKER(mra))
							{
								if (mra->end < mra->start)
								{
									log_cb(RETRO_LOG_ERROR, LOGPRE "%s: %s wrong memory read handler start = %08x > end = %08x\n",drivers[i]->source_file,drivers[i]->name,mra->start,mra->end);
									error = 1;
								}
								if ((mra->start & (alignunit-1)) != 0 || (mra->end & (alignunit-1)) != (alignunit-1))
								{
									log_cb(RETRO_LOG_ERROR, LOGPRE "%s: %s wrong memory read handler start = %08x, end = %08x ALIGN = %d\n",drivers[i]->source_file,drivers[i]->name,mra->start,mra->end,alignunit);
									error = 1;
								}
							}
							mra++;
						}
					}
					if (drv.cpu[cpu].memory_write)
					{
						const struct Memory_WriteAddress *mwa = drv.cpu[cpu].memory_write;

						if (mwa->start != MEMPORT_MARKER ||
								(mwa->end & MEMPORT_DIRECTION_MASK) != MEMPORT_DIRECTION_WRITE)
						{
							log_cb(RETRO_LOG_ERROR, LOGPRE "%s: %s wrong MEMPORT_WRITE_START\n",drivers[i]->source_file,drivers[i]->name);
							error = 1;
						}

						switch (databus_width)
						{
							case 8:
								if ((mwa->end & MEMPORT_WIDTH_MASK) != MEMPORT_WIDTH_8)
								{
									log_cb(RETRO_LOG_ERROR, LOGPRE "%s: %s cpu #%d uses wrong data width memory handlers! (width = %d, memory = %08x)\n",drivers[i]->source_file,drivers[i]->name,cpu,databus_width,mwa->end);
									error = 1;
								}
								break;
							case 16:
								if ((mwa->end & MEMPORT_WIDTH_MASK) != MEMPORT_WIDTH_16)
								{
									log_cb(RETRO_LOG_ERROR, LOGPRE "%s: %s cpu #%d uses wrong data width memory handlers! (width = %d, memory = %08x)\n",drivers[i]->source_file,drivers[i]->name,cpu,databus_width,mwa->end);
									error = 1;
								}
								break;
							case 32:
								if ((mwa->end & MEMPORT_WIDTH_MASK) != MEMPORT_WIDTH_32)
								{
									log_cb(RETRO_LOG_ERROR, LOGPRE "%s: %s cpu #%d uses wrong data width memory handlers! (width = %d, memory = %08x)\n",drivers[i]->source_file,drivers[i]->name,cpu,databus_width,mwa->end);
									error = 1;
								}
								break;
						}

						while (!IS_MEMPORT_END(mwa))
						{
							if (!IS_MEMPORT_MARKER(mwa))
							{
								if (mwa->end < mwa->start)
								{
									log_cb(RETRO_LOG_ERROR, LOGPRE "%s: %s wrong memory write handler start = %08x > end = %08x\n",drivers[i]->source_file,drivers[i]->name,mwa->start,mwa->end);
									error = 1;
								}
								if ((mwa->start & (alignunit-1)) != 0 || (mwa->end & (alignunit-1)) != (alignunit-1))
								{
									log_cb(RETRO_LOG_ERROR, LOGPRE "%s: %s wrong memory write handler start = %08x, end = %08x ALIGN = %d\n",drivers[i]->source_file,drivers[i]->name,mwa->start,mwa->end,alignunit);
									error = 1;
								}
							}
							mwa++;
						}
					}

					if (drv.cpu[cpu].port_read)
					{
						const struct IO_ReadPort *pra = drv.cpu[cpu].port_read;

						if (!IS_MEMPORT_MARKER(pra) || (pra->end & MEMPORT_DIRECTION_MASK) != MEMPORT_DIRECTION_READ)
						{
							log_cb(RETRO_LOG_ERROR, LOGPRE "%s: %s wrong PORT_READ_START\n",drivers[i]->source_file,drivers[i]->name);
							error = 1;
						}

						switch (databus_width)
						{
							case 8:
								if ((pra->end & MEMPORT_WIDTH_MASK) != MEMPORT_WIDTH_8)
								{
									log_cb(RETRO_LOG_ERROR, LOGPRE "%s: %s cpu #%d uses wrong data width port handlers! (width = %d, port = %08x)\n",drivers[i]->source_file,drivers[i]->name,cpu,databus_width,pra->end);
									error = 1;
								}
								break;
							case 16:
								if ((pra->end & MEMPORT_WIDTH_MASK) != MEMPORT_WIDTH_16)
								{
									log_cb(RETRO_LOG_ERROR, LOGPRE "%s: %s cpu #%d uses wrong data width port handlers! (width = %d, port = %08x)\n",drivers[i]->source_file,drivers[i]->name,cpu,databus_width,pra->end);
									error = 1;
								}
								break;
							case 32:
								if ((pra->end & MEMPORT_WIDTH_MASK) != MEMPORT_WIDTH_32)
								{
									log_cb(RETRO_LOG_ERROR, LOGPRE "%s: %s cpu #%d uses wrong data width port handlers! (width = %d, port = %08x)\n",drivers[i]->source_file,drivers[i]->name,cpu,databus_width,pra->end);
									error = 1;
								}
								break;
						}

						while (!IS_MEMPORT_END(pra))
						{
							if (!IS_MEMPORT_MARKER(pra))
							{
								if (pra->end < pra->start)
								{
									log_cb(RETRO_LOG_ERROR, LOGPRE "%s: %s wrong port read handler start = %08x > end = %08x\n",drivers[i]->source_file,drivers[i]->name,pra->start,pra->end);
									error = 1;
								}
								if ((pra->start & (alignunit-1)) != 0 || (pra->end & (alignunit-1)) != (alignunit-1))
								{
									log_cb(RETRO_LOG_ERROR, LOGPRE "%s: %s wrong port read handler start = %08x, end = %08x ALIGN = %d\n",drivers[i]->source_file,drivers[i]->name,pra->start,pra->end,alignunit);
									error = 1;
								}

							}
							pra++;
						}
					}

					if (drv.cpu[cpu].port_write)
					{
						const struct IO_WritePort *pwa = drv.cpu[cpu].port_write;

						if (pwa->start != MEMPORT_MARKER ||
								(pwa->end & MEMPORT_DIRECTION_MASK) != MEMPORT_DIRECTION_WRITE)
						{
							log_cb(RETRO_LOG_ERROR, LOGPRE "%s: %s wrong PORT_WRITE_START\n",drivers[i]->source_file,drivers[i]->name);
							error = 1;
						}

						switch (databus_width)
						{
							case 8:
								if ((pwa->end & MEMPORT_WIDTH_MASK) != MEMPORT_WIDTH_8)
								{
									log_cb(RETRO_LOG_ERROR, LOGPRE "%s: %s cpu #%d uses wrong data width port handlers! (width = %d, port = %08x)\n",drivers[i]->source_file,drivers[i]->name,cpu,databus_width,pwa->end);
									error = 1;
								}
								break;
							case 16:
								if ((pwa->end & MEMPORT_WIDTH_MASK) != MEMPORT_WIDTH_16)
								{
									log_cb(RETRO_LOG_ERROR, LOGPRE "%s: %s cpu #%d uses wrong data width port handlers! (width = %d, port = %08x)\n",drivers[i]->source_file,drivers[i]->name,cpu,databus_width,pwa->end);
									error = 1;
								}
								break;
							case 32:
								if ((pwa->end & MEMPORT_WIDTH_MASK) != MEMPORT_WIDTH_32)
								{
									log_cb(RETRO_LOG_ERROR, LOGPRE "%s: %s cpu #%d uses wrong data width port handlers! (width = %d, port = %08x)\n",drivers[i]->source_file,drivers[i]->name,cpu,databus_width,pwa->end);
									error = 1;
								}
								break;
						}

						while (!IS_MEMPORT_END(pwa))
						{
							if (!IS_MEMPORT_MARKER(pwa))
							{
								if (pwa->end < pwa->start)
								{
									log_cb(RETRO_LOG_ERROR, LOGPRE "%s: %s wrong port write handler start = %08x > end = %08x\n",drivers[i]->source_file,drivers[i]->name,pwa->start,pwa->end);
									error = 1;
								}
								if ((pwa->start & (alignunit-1)) != 0 || (pwa->end & (alignunit-1)) != (alignunit-1))
								{
									log_cb(RETRO_LOG_ERROR, LOGPRE "%s: %s wrong port write handler start = %08x, end = %08x ALIGN = %d\n",drivers[i]->source_file,drivers[i]->name,pwa->start,pwa->end,alignunit);
									error = 1;
								}

							}
							pwa++;
						}
					}

				}
			}


			if (drv.gfxdecodeinfo)
			{
				for (j = 0;j < MAX_GFX_ELEMENTS && drv.gfxdecodeinfo[j].memory_region != -1;j++)
				{
					int len,avail,k,start;
					int type = drv.gfxdecodeinfo[j].memory_region;


/*
					if (type && (type >= REGION_MAX || type <= REGION_INVALID))
					{
						log_cb(RETRO_LOG_ERROR, LOGPRE "%s: %s has invalid memory region for gfx[%d]\n",drivers[i]->source_file,drivers[i]->name,j);
						error = 1;
					}
*/

					if (!IS_FRAC(drv.gfxdecodeinfo[j].gfxlayout->total))
					{
						start = 0;
						for (k = 0;k < MAX_GFX_PLANES;k++)
						{
							if (drv.gfxdecodeinfo[j].gfxlayout->planeoffset[k] > start)
								start = drv.gfxdecodeinfo[j].gfxlayout->planeoffset[k];
						}
						start &= ~(drv.gfxdecodeinfo[j].gfxlayout->charincrement-1);
						len = drv.gfxdecodeinfo[j].gfxlayout->total *
								drv.gfxdecodeinfo[j].gfxlayout->charincrement;
						avail = region_length[type]
								- (drv.gfxdecodeinfo[j].start & ~(drv.gfxdecodeinfo[j].gfxlayout->charincrement/8-1));
						if ((start + len) / 8 > avail)
						{
							log_cb(RETRO_LOG_ERROR, LOGPRE "%s: %s has gfx[%d] extending past allocated memory\n",drivers[i]->source_file,drivers[i]->name,j);
							error = 1;
						}
					}
				}
			}
		}


		inp = drivers[i]->input_ports;

		if (inp)
		{
			while (inp->type != IPT_END)
			{
				if (inp->name && inp->name != IP_NAME_DEFAULT)
				{
					j = 0;

					for (j = 0;j < STR_TOTAL;j++)
					{
						if (inp->name == ipdn_defaultstrings[j]) break;
						else if (!strcasecmp(inp->name,ipdn_defaultstrings[j]))
						{
							log_cb(RETRO_LOG_ERROR, LOGPRE "%s: %s must use DEF_STR( %s )\n",drivers[i]->source_file,drivers[i]->name,inp->name);
							error = 1;
						}
					}

					if (inp->name == DEF_STR( On ) && (inp+1)->name == DEF_STR( Off ))
					{
						log_cb(RETRO_LOG_ERROR, LOGPRE "%s: %s has inverted Off/On dipswitch order\n",drivers[i]->source_file,drivers[i]->name);
						error = 1;
					}

					if (inp->name == DEF_STR( Yes ) && (inp+1)->name == DEF_STR( No ))
					{
						log_cb(RETRO_LOG_ERROR, LOGPRE "%s: %s has inverted No/Yes dipswitch order\n",drivers[i]->source_file,drivers[i]->name);
						error = 1;
					}

					if (!strcasecmp(inp->name,"table"))
					{
						log_cb(RETRO_LOG_ERROR, LOGPRE "%s: %s must use DEF_STR( Cocktail ), not %s\n",drivers[i]->source_file,drivers[i]->name,inp->name);
						error = 1;
					}

					if (inp->name == DEF_STR( Cabinet ) && (inp+1)->name == DEF_STR( Upright )
							&& inp->default_value != (inp+1)->default_value)
					{
						log_cb(RETRO_LOG_ERROR, LOGPRE "%s: %s Cabinet must default to Upright\n",drivers[i]->source_file,drivers[i]->name);
						error = 1;
					}

					if (inp->name == DEF_STR( Cocktail ) && (inp+1)->name == DEF_STR( Upright ))
					{
						log_cb(RETRO_LOG_ERROR, LOGPRE "%s: %s has inverted Upright/Cocktail dipswitch order\n",drivers[i]->source_file,drivers[i]->name);
						error = 1;
					}

					if (inp->name >= DEF_STR( 9C_1C ) && inp->name <= DEF_STR( Free_Play )
							&& (inp+1)->name >= DEF_STR( 9C_1C ) && (inp+1)->name <= DEF_STR( Free_Play )
							&& inp->name >= (inp+1)->name)
					{
						log_cb(RETRO_LOG_ERROR, LOGPRE "%s: %s has unsorted coinage %s > %s\n",drivers[i]->source_file,drivers[i]->name,inp->name,(inp+1)->name);
						error = 1;
					}

					if (inp->name == DEF_STR( Flip_Screen ) && (inp+1)->name != DEF_STR( Off ))
					{
						log_cb(RETRO_LOG_ERROR, LOGPRE "%s: %s has wrong Flip Screen option %s\n",drivers[i]->source_file,drivers[i]->name,(inp+1)->name);
						error = 1;
					}

					if (inp->name == DEF_STR( Demo_Sounds ) && (inp+2)->name == DEF_STR( On )
							&& inp->default_value != (inp+2)->default_value)
					{
						log_cb(RETRO_LOG_ERROR, LOGPRE "%s: %s Demo Sounds must default to On\n",drivers[i]->source_file,drivers[i]->name);
						error = 1;
					}

					if (inp->name == DEF_STR( Demo_Sounds ) && (inp+1)->name == DEF_STR( No ))
					{
						log_cb(RETRO_LOG_ERROR, LOGPRE "%s: %s has wrong Demo Sounds option No instead of Off\n",drivers[i]->source_file,drivers[i]->name);
						error = 1;
					}
				}

				inp++;
			}
		}
	}

	return error;
}
#endif

void cpu_run_done(void);
void mame_done(void)
{
    if(game_loaded)
    {
        cpu_run_done();
        run_machine_core_done();
        run_machine_done();
        run_game_done();
    }

    game_loaded = 0;
}
