/*********************************************************************

  mame2003.c

  an updated port of Xmame 0.78 to the libretro API

*********************************************************************/

#include <stdint.h>
#include <string/stdstring.h>
#include <libretro.h>
#include <file/file_path.h>
#include <streams/file_stream.h>
#include <math.h>

#if (HAS_DRZ80 || HAS_CYCLONE)
#include "frontend_list.h"
#endif

#include "mame.h"
#include "driver.h"
#include "state.h"
#include "log.h"
#include "input.h"
#include "inptport.h"
#include "fileio.h"
#include "controls.h"
#include "usrintrf.h"


const struct GameDriver  *game_driver;

int            retro_running = 0;
int            gotFrame;
static float   delta_samples;
int            samples_per_frame = 0;
int            orig_samples_per_frame = 0;
short*         samples_buffer;
short*         conversion_buffer;
int            usestereo = 1;


/* MAME data structures to store and translate keyboard state */
const struct KeyboardInfo  retroKeys[];

retro_log_printf_t                         log_cb;
static struct retro_message                frontend_message;

struct retro_perf_callback                 perf_cb;
retro_environment_t                        environ_cb         = NULL;
retro_video_refresh_t                      video_cb           = NULL;
static retro_input_poll_t                  poll_cb            = NULL;
static retro_input_state_t                 input_cb           = NULL;
static retro_audio_sample_batch_t          audio_batch_cb     = NULL;
retro_set_led_state_t                      led_state_cb       = NULL;
struct retro_audio_buffer_status_callback  buf_status_cb;

#ifdef _MSC_VER
#if _MSC_VER < 1800
double round(double number)
{
  return (number >= 0) ? (int)(number + 0.5) : (int)(number - 0.5);
}
#endif
#endif


/******************************************************************************

  private function prototypes

******************************************************************************/

static void   check_system_specs(void);
       void   retro_describe_controls(void);
   unsigned   get_device_parent(unsigned device_id);
        int   get_retro_code(const char* type, unsigned osd_code);
   unsigned   get_ctrl_ipt_code(unsigned player_number, unsigned standard_code);
   unsigned   encode_osd_joycode(unsigned player_number, unsigned osd_code);
   unsigned   decode_osd_joycode(unsigned joycode);
   unsigned   calc_player_number(unsigned joycode);
        int   rescale_analog(int libretro_coordinate);
static void   remove_slash (char* temp);
static void   configure_cyclone_mode (int driverIndex);


/******************************************************************************

  Data structures for libretro controllers

*******************************************************************************/

/* the first of our controllers can use the base retropad type and rename it,
 * while any layout variations must subclass the type.
 */

#define PAD_CLASSIC       RETRO_DEVICE_JOYPAD
#define PAD_FIGHTSTICK    RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 0)
#define PAD_8BUTTON       RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 1)
#define PAD_6BUTTON       RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 2)

const struct retro_controller_description controllers[] = {
  { "RetroPad",   PAD_CLASSIC    },
  { "Fightstick", PAD_FIGHTSTICK },
  { "8-Button",   PAD_8BUTTON    },
  { "6-Button",   PAD_6BUTTON    },
};

/******************************************************************************

  frontend message interface

******************************************************************************/

void frontend_message_cb(const char *message_string, unsigned frames_to_display)
{
  frontend_message.msg    = message_string;
  frontend_message.frames = frames_to_display;
  environ_cb(RETRO_ENVIRONMENT_SET_MESSAGE, &frontend_message);
}

/******************************************************************************

  implementation of key libretro functions

******************************************************************************/
bool retro_audio_buff_active        = false;
unsigned retro_audio_buff_occupancy = 0;
bool retro_audio_buff_underrun      = false;

static void retro_audio_buff_status_cb(bool active, unsigned occupancy, bool underrun_likely)
{
   retro_audio_buff_active    = active;
   retro_audio_buff_occupancy = occupancy;
   retro_audio_buff_underrun  = underrun_likely;
}

void retro_set_audio_buff_status_cb(void)
{
  if (options.frameskip > 0 && options.frameskip >= 12)
  {
      buf_status_cb.callback = &retro_audio_buff_status_cb;

      if (!environ_cb(RETRO_ENVIRONMENT_SET_AUDIO_BUFFER_STATUS_CALLBACK,
            &buf_status_cb))
      {
         if (log_cb)
            log_cb(RETRO_LOG_WARN, "Frameskip disabled - frontend does not support audio buffer status monitoring.\n");

         retro_audio_buff_active    = false;
         retro_audio_buff_occupancy = 0;
         retro_audio_buff_underrun  = false;
      }
      else
      log_cb(RETRO_LOG_INFO, "Frameskip Enabled\n");
  }
   else
      environ_cb(RETRO_ENVIRONMENT_SET_AUDIO_BUFFER_STATUS_CALLBACK,NULL);

}

unsigned retro_api_version(void)
{
  return RETRO_API_VERSION;
}


void retro_get_system_info(struct retro_system_info *info)
{
  /* this must match the 'corename' field in mame2003_plus_libretro.info
   * in order for netplay to work. */
  info->library_name = "MAME 2003-Plus";
#ifndef GIT_VERSION
#define GIT_VERSION ""
#endif
  info->library_version = GIT_VERSION;
  info->valid_extensions = "zip";
  info->need_fullpath = true;
  info->block_extract = true;
}

void retro_init (void)
{
  struct retro_log_callback log;
  if (environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log))
    log_cb = log.log;
  else
    log_cb = NULL;

#ifdef LOG_PERFORMANCE
  environ_cb(RETRO_ENVIRONMENT_GET_PERF_INTERFACE, &perf_cb);
#endif

  check_system_specs();
}


static void check_system_specs(void)
{
  #ifndef RETRO_PROFILE
  #define RETRO_PROFILE 10
  #endif
   /* This value can be set in the Makefile, and then modified here before being passed,
    * even on an individual game basis.
    * However, as of June 2021, the libretro performance profile callback is not known
    * to be implemented by any frontends including RetroArch. The RA developers do not
    * have a suggested range of values. We use 10 by convention (copying other cores).
    */
  unsigned level = (unsigned)RETRO_PROFILE;
  environ_cb(RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL, &level);
}


void retro_set_environment(retro_environment_t cb)
{
  struct retro_vfs_interface_info vfs_iface_info;

  environ_cb = cb;

  /* Initialise VFS */
  vfs_iface_info.required_interface_version = 1;
  vfs_iface_info.iface                      = NULL;
  if (environ_cb(RETRO_ENVIRONMENT_GET_VFS_INTERFACE, &vfs_iface_info))
    filestream_vfs_init(&vfs_iface_info);
}


void retro_get_system_av_info(struct retro_system_av_info *info)
{
  mame2003_video_get_geometry(&info->geometry);
  
  info->timing.fps = Machine->drv->frames_per_second;
  info->timing.sample_rate = options.samplerate ;
}


bool retro_load_game(const struct retro_game_info *game)
{
  struct retro_controller_info input_subdevice_ports[MAX_PLAYER_COUNT + 1];

  int   driverIndex    = 0;
  int   port_index;
  char  *driver_lookup = NULL;

  if(string_is_empty(game->path))
  {
    log_cb(RETRO_LOG_ERROR, LOGPRE "Content path is not set. Exiting!\n");
    return false;
  }

  log_cb(RETRO_LOG_INFO, LOGPRE "Full content path %s\n", game->path);
  if(!path_is_valid(game->path))
  {
    log_cb(RETRO_LOG_ERROR, LOGPRE "Content path is not valid. Exiting!");
    return false;
  }
  log_cb(RETRO_LOG_INFO, LOGPRE "Git Version %s\n",GIT_VERSION);
  driver_lookup = strdup(path_basename(game->path));
  path_remove_extension(driver_lookup);

  log_cb(RETRO_LOG_INFO, LOGPRE "Content lookup name: %s\n", driver_lookup);

  for (driverIndex = 0; driverIndex < total_drivers; driverIndex++)
  {
    const struct GameDriver *needle = drivers[driverIndex];

    if ( strcasecmp(driver_lookup, needle->name) == 0 )
    {
      log_cb(RETRO_LOG_INFO, LOGPRE "Driver index counter: %d. Matched game driver: %s\n",  driverIndex, needle->name);
      game_driver = needle;
      options.romset_filename_noext = driver_lookup;
      break;
    }
    if(driverIndex == total_drivers -2) /* we could fix the total drives in drivers c but the it pointless its taken into account here */
    {
      log_cb(RETRO_LOG_ERROR, LOGPRE "Driver index counter: %d. Game driver not found for %s!\n", driverIndex, driver_lookup);
      return false;
    }
  }

  if(!init_game(driverIndex))
    return false;

  set_content_flags();

  options.libretro_content_path = strdup(game->path);
  path_basedir(options.libretro_content_path);

  /* Get system directory from frontend */
  options.libretro_system_path = NULL;
  environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY,&options.libretro_system_path);
  if (options.libretro_system_path == NULL || options.libretro_system_path[0] == '\0')
  {
      log_cb(RETRO_LOG_INFO, LOGPRE "libretro system path not set by frontend, using content path\n");
      options.libretro_system_path = options.libretro_content_path;
  }

  /* Get save directory from frontend */
  options.libretro_save_path = NULL;
  environ_cb(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY,&options.libretro_save_path);
  if (options.libretro_save_path == NULL || options.libretro_save_path[0] == '\0')
  {
      log_cb(RETRO_LOG_INFO,  LOGPRE "libretro save path not set by frontend, using content path\n");
      options.libretro_save_path = options.libretro_content_path;
  }

  /* Remove trailing slashes for specified systems */
  remove_slash(options.libretro_content_path);
  remove_slash(options.libretro_system_path);
  remove_slash(options.libretro_save_path);

  log_cb(RETRO_LOG_INFO, LOGPRE "content path: %s\n", options.libretro_content_path);
  log_cb(RETRO_LOG_INFO, LOGPRE " system path: %s\n", options.libretro_system_path);
  log_cb(RETRO_LOG_INFO, LOGPRE "   save path: %s\n", options.libretro_save_path);


  init_core_options();
  update_variables(true);

  configure_cyclone_mode(driverIndex);

  /* Not all drivers support the maximum number of players. Only send controller info
   * to the frontend for the number of players supported then zero the final record.
   */
  for(port_index = 0; port_index < options.content_flags[CONTENT_CTRL_COUNT]; port_index++)
  {
    input_subdevice_ports[port_index].types       = controllers;
    input_subdevice_ports[port_index].num_types   = IDX_NUMBER_OF_INPUT_TYPES;
  }
  input_subdevice_ports[options.content_flags[CONTENT_CTRL_COUNT]].types       = 0;
  input_subdevice_ports[options.content_flags[CONTENT_CTRL_COUNT]].num_types   = 0;

  environ_cb(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, (void*)input_subdevice_ports);

  if(!run_game(driverIndex))
    return true;

  return false;
}


void retro_reset (void)
{
    machine_reset(); /* use internal core function */
}

/* get pointer axis vector from coord */
int16_t get_pointer_delta(int16_t coord, int16_t *prev_coord)
{
   int16_t delta = 0;
   if (*prev_coord == 0 || coord == 0)
   {
      *prev_coord = coord;
   }
   else
   {
      if (coord != *prev_coord)
      {
         delta = coord - *prev_coord;
         *prev_coord = coord;
      }
   }

   return delta;
}

void retro_run (void)
{
  bool updated = false;
  poll_cb(); /* execute input callback */

  if (retro_running == 0) /* first time through the loop */
  {
    retro_running = 1;
    log_cb(RETRO_LOG_DEBUG, LOGPRE "Entering retro_run() for the first time.\n");
  }

  if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
    update_variables(false);

  if (options.cpu_clock_scale)
  {
    if (cpunum_get_clockscale(0) != options.cpu_clock_scale)
    {
      log_cb(RETRO_LOG_DEBUG, LOGPRE "changing cpu clock scale from %lf to %lf\n",cpunum_get_clockscale(0),options.cpu_clock_scale);
      cpunum_set_clockscale(0, options.cpu_clock_scale);
    }
  }

  mame_frame();
}

void retro_unload_game(void)
{
    mame_done();
    /* do we need to be freeing things here? */

    free(options.romset_filename_noext);
}

void retro_deinit(void)
{
#ifdef LOG_PERFORMANCE
   perf_cb.perf_log();
#endif
}

extern size_t state_get_dump_size(void);

size_t retro_serialize_size(void)
{
    return state_get_dump_size();
}

bool retro_serialize(void *data, size_t size)
{
   int cpunum;
	if(  retro_serialize_size() == size  && size   )
	{
		/* write the save state */
		state_save_save_begin(data);

		/* write tag 0 */
		state_save_set_current_tag(0);
		if(state_save_save_continue())
		{
		    return false;
		}

		/* loop over CPUs */
		for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
		{
			cpuintrf_push_context(cpunum);

			/* make sure banking is set */
			activecpu_reset_banking();

			/* save the CPU data */
			state_save_set_current_tag(cpunum + 1);
			if(state_save_save_continue())
			    return false;

			cpuintrf_pop_context();
		}

		/* finish and close */
		state_save_save_finish();

		return true;
	}

	return false;
}

bool retro_unserialize(const void * data, size_t size)
{
    int cpunum;
	/* if successful, load it */
	if ( (retro_serialize_size() ) && ( data ) && ( size ) && ( !state_save_load_begin((void*)data, size) ) )
	{
        /* read tag 0 */
        state_save_set_current_tag(0);
        if(state_save_load_continue())
            return false;

        /* loop over CPUs */
        for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
        {
            cpuintrf_push_context(cpunum);

            /* make sure banking is set */
            activecpu_reset_banking();

            /* load the CPU data */
            state_save_set_current_tag(cpunum + 1);
            if(state_save_load_continue())
                return false;

            cpuintrf_pop_context();
        }

        /* finish and close */
        state_save_load_finish();


        return true;
	}

	return false;
}

/******************************************************************************

  Sound

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

******************************************************************************/

int osd_start_audio_stream(int stereo)
{
 
  Machine->sample_rate = options.samplerate;

  delta_samples = 0.0f;
  usestereo = stereo ? 1 : 0;

  /* determine the number of samples per frame */
  samples_per_frame = Machine->sample_rate / Machine->drv->frames_per_second;
  orig_samples_per_frame = samples_per_frame;

  if (Machine->sample_rate == 0) return 0;

  samples_buffer = (short *) calloc(samples_per_frame+16, 2 + usestereo * 2);
  if (!usestereo) conversion_buffer = (short *) calloc(samples_per_frame+16, 4);

  return samples_per_frame;
}


int osd_update_audio_stream(INT16 *buffer)
{
	int i,j;
	if ( Machine->sample_rate !=0 && buffer )
	{
		memcpy(samples_buffer, buffer, samples_per_frame * (usestereo ? 4 : 2));
		if (usestereo)
			audio_batch_cb(samples_buffer, samples_per_frame);
		else
		{
			for (i = 0, j = 0; i < samples_per_frame; i++)
			{
				conversion_buffer[j++] = samples_buffer[i];
				conversion_buffer[j++] = samples_buffer[i];
			}
			audio_batch_cb(conversion_buffer,samples_per_frame);
		}


		/*process next frame */

		if ( samples_per_frame  != orig_samples_per_frame ) samples_per_frame = orig_samples_per_frame;

		/* dont drop any sample frames some games like mk will drift with time */

		delta_samples += (Machine->sample_rate / Machine->drv->frames_per_second) - orig_samples_per_frame;
		if ( delta_samples >= 1.0f )
		{

			int integer_delta = (int)delta_samples;
			if (integer_delta <= 16 )
			{
				log_cb(RETRO_LOG_DEBUG,"sound: Delta added value %d added to frame\n",integer_delta);
				samples_per_frame += integer_delta;
			}
			else if(integer_delta >= 16) log_cb(RETRO_LOG_INFO, "sound: Delta not added to samples_per_frame too large integer_delta: %d\n", integer_delta);
			else log_cb(RETRO_LOG_DEBUG,"sound(delta) no conditions met\n");
			delta_samples -= integer_delta;

		}
	}
	return samples_per_frame;
}


void osd_stop_audio_stream(void)
{
}



/******************************************************************************

  Miscellaneous

******************************************************************************/

unsigned retro_get_region (void) {return RETRO_REGION_NTSC;}
void *retro_get_memory_data(unsigned type) {return 0;}
size_t retro_get_memory_size(unsigned type) {return 0;}
bool retro_load_game_special(unsigned game_type, const struct retro_game_info *info, size_t num_info){return false;}
void retro_cheat_reset(void){}
void retro_cheat_set(unsigned unused, bool unused1, const char* unused2){}
void retro_set_video_refresh(retro_video_refresh_t cb) { video_cb = cb; }
void retro_set_audio_sample(retro_audio_sample_t cb) { }
void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) { audio_batch_cb = cb; }
void retro_set_input_poll(retro_input_poll_t cb) { poll_cb = cb; }
void retro_set_input_state(retro_input_state_t cb) { input_cb = cb; }


/******************************************************************************

  RetroPad mapping

******************************************************************************/

  /* Assuming the standard RetroPad layout:
   *
   *   [L2]                                 [R2]
   *   [L]                                   [R]
   *
   *     [^]                               [X]
   *
   * [<]     [>]    [start] [selct]    [Y]     [A]
   *
   *     [v]                               [B]
   *
   *
   * or standard RetroPad fight stick layout:
   *
   *   [start] [selct]
   *                                         [X]  [R]  [L]
   *     [^]                            [Y]
   *
   * [<]     [>]                             [A]  [R2] [L2]
   *                                    [B]
   *     [v]
   *
   *
   *
   * key: [Button code/Street Fighter II move]
   *
   *
   *
   * PAD_CLASSIC
   * ========================
   * Same as FB Alpha's "Classic" mapping.
   * Not sensible for 6 button fighters, but may suit other games.
   *
   * [7/-]                                     [8/-]   |
   * [5/MK]                                    [6/HK]  |
   *                                                   |        [4/WK]  [6/HK]  [5/MK]
   *     [^]                               [4/WK]      |  [3/HP]
   *                                                   |
   * [<]     [>]    [start] [selct]    [3/HP]  [2/MP]  |        [2/MP]  [8/-]   [7/-]
   *                                                   |  [1/LP]
   *     [v]                               [1/LP]      |
   *                                                   |
   *
   *
   * PAD_FIGHTSTICK
   * ========================
   * Uses the fight stick & pad layout popularised by Street Figher IV.
   * Needs an 8+ button controller by default.
   *
   * [8/-]                                     [6/HK]  |
   * [7/-]                                     [3/HP]  |
   *                                                   |        [2/MP]  [3/HP]  [7/-]
   *     [^]                               [2/MP]      |  [1/LP]
   *                                                   |
   * [<]     [>]    [start] [selct]    [1/LP]  [5/MK]  |        [5/MK]  [6/HK]  [8/-]
   *                                                   |  [4/LK]
   *     [v]                               [4/LK]      |
   *                                                   |
   *
   *
   * PAD_6BUTTON
   * ========================
   * Only needs a 6+ button controller by default, doesn't suit 8+ button fight sticks.
   *
   * [7/-]                                      [8/-]  |
   * [3/HP]                                    [6/HK]  |
   *                                                   |        [2/MP]  [6/HK]  [3/HP]
   *     [^]                               [2/MP]      |  [1/LP]
   *                                                   |
   * [<]     [>]    [start] [selct]    [1/LP]  [5/MK]  |        [5/MK]  [8/-]   [7/-]
   *                                                   |  [4/LK]
   *     [v]                               [4/LK]      |
   *                                                   |
   *
   */


/* libretro presents "Player 1", "Player 2", "Player 3", etc while internally using indexed data starting at 0, 1, 2 */
/* The core presents "Player 1", "Player 2," "Player 3", and indexes them via enum values like JOYCODE_1_BUTTON1,        */
/* JOYCODE_2_BUTTON1, JOYCODE_3_BUTTON1 or with #define-d masks IPF_PLAYER1, IPF_PLAYER2, IPF_PLAYER3.               */
/*                                                                                                                   */
/* We are by convention passing "display" value used for mapping to core enums and player # masks to these macros.   */
/* (Display Index - 1) can be used for indexed data structures.                                                      */

void retro_set_controller_port_device(unsigned in_port, unsigned device)
{
  static bool run_update = false;
  if (in_port == (options.content_flags[CONTENT_CTRL_COUNT] - 1) ) run_update = true;

  options.active_control_type[in_port] = device;
  log_cb(RETRO_LOG_DEBUG, LOGPRE "Preparing to connect input    in_port: %i    device: %i\n", in_port, device);

  /* During core init we update and describe the controls when we encounter the last controller port connected
   * to prevent spamming the frontend. On subsequent calls, we update and describe the structures each time.
   */
  if (run_update)
  {
    internal_code_update();     /* update MAME data structures for controls */
    retro_describe_controls();  /* update libretro data structures for controls */
  }
}


void retro_describe_controls(void)
{
  int port_number = 0;
  struct retro_input_descriptor desc[(MAX_PLAYER_COUNT * OSD_INPUT_CODES_PER_PLAYER) +  1]; /* + 1 for the final zeroed record. */
  struct retro_input_descriptor *needle = &desc[0];

  log_cb(RETRO_LOG_DEBUG, LOGPRE "Describing %i controllers supported by the content loaded\n", options.content_flags[CONTENT_CTRL_COUNT]);

  for(port_number = 0; port_number < options.content_flags[CONTENT_CTRL_COUNT]; port_number++)
  {
    unsigned osd_code    = 0;
    unsigned device_code = options.active_control_type[port_number];

    log_cb(RETRO_LOG_DEBUG, LOGPRE "Controller port: %i   device type: %i   parent type: %i\n", port_number, device_code, get_device_parent(device_code));

    for(osd_code = OSD_JOYPAD_B; osd_code < OSD_INPUT_CODES_PER_PLAYER; osd_code++)
    {
      unsigned joycode          = 0;      /* the unique code (including across players) created by the libretro OSD */
      unsigned standard_code    = 0;      /* standard code is the MAME term for the internal input code, associated with a controller */
      unsigned ctrl_ipt_code    = 0;      /* input code connects an input port with standard input code */
      unsigned retro_code       = 0;      /* #define code from the libretro.h input API scheme */
      const char *control_name  = 0;

      joycode = encode_osd_joycode(port_number + 1, osd_code);
      if(joycode == INT_MAX) continue;

      standard_code = oscode_find(joycode, CODE_TYPE_JOYSTICK);
      if(standard_code == CODE_NONE) continue;

      ctrl_ipt_code = get_ctrl_ipt_code(port_number + 1, standard_code) & ~IPF_PLAYERMASK; /* discard the player mask, although later we may want to distinguish control names by player number */
      if(ctrl_ipt_code == CODE_NONE) continue;

      if(ctrl_ipt_code >= IPT_BUTTON1 && ctrl_ipt_code <= IPT_BUTTON10)
        if((ctrl_ipt_code - IPT_BUTTON1 + 1) > options.content_flags[CONTENT_BUTTON_COUNT])
          continue; /* button has a higher index than supported by the driver */

      /* try to get the corresponding ID for this control in libretro.h  */
      /* from the retropad section, or INT_MAX if not valid */
      retro_code = get_retro_code("retropad", osd_code);
      if(retro_code != INT_MAX)
      {
        /* First try to get specific name */
        control_name = game_driver->ctrl_dat->get_name(ctrl_ipt_code);

        if(string_is_empty(control_name))
        {
          switch(retro_code) /* universal default mappings */
          {
            case RETRO_DEVICE_ID_JOYPAD_LEFT:   control_name = "Left";  break;
            case RETRO_DEVICE_ID_JOYPAD_RIGHT:  control_name = "Right"; break;
            case RETRO_DEVICE_ID_JOYPAD_UP:     control_name = "Up";    break;
            case RETRO_DEVICE_ID_JOYPAD_DOWN:   control_name = "Down";  break;
            case RETRO_DEVICE_ID_JOYPAD_SELECT: control_name = "Coin";  break;
            case RETRO_DEVICE_ID_JOYPAD_START:  control_name = "Start"; break;
          }
        }
      }

      if(string_is_empty(control_name))  continue;

      /* With regard to the device number, we refer to the input polling comments in
       * libretro.h which says we "should only poll input based on the base input device
       * types". That seems to be true in here too, because using the result of
       * RETRO_DEVICE_SUBCLASS does not work in RetroArch in April 2021 when passed as part
       * of the descriptions. Therefore, get_device_parent() is used.
       */
      needle->port         = port_number;
      needle->device       = get_device_parent(device_code);
      needle->index        = 0;
      needle->id           = retro_code;
      needle->description  = control_name;
      log_cb(RETRO_LOG_DEBUG, LOGPRE "joycode: %i | standard code: %i | id: %2i | desc: %s\n", joycode, standard_code, needle->id, needle->description);
      needle++;
    }
  }

  /* the extra final record remains zeroed to indicate the end of the description to the frontend */
  /* which specifically looks at the description field to be NULL per libretro.h */
  needle->port        = 0;
  needle->device      = 0;
  needle->index       = 0;
  needle->id          = 0;
  needle->description = NULL;

#if 0
  needle = &desc[0];
  log_cb(RETRO_LOG_DEBUG, LOGPRE "Beginning of description list.\n");
  while(needle->description != NULL)
  {
    log_cb(RETRO_LOG_DEBUG, LOGPRE "Description || port: %i | device: %i | index: %i | id: %i \t| name: %s\n", needle->port, needle->device, needle->index, needle->id, needle->description);
    needle++;
  }
  log_cb(RETRO_LOG_DEBUG, LOGPRE "End of description list.\n");
#endif

  environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc);
}

/* get_device_parent
 * returns the "base" libretro device type, which we may sometimes need
 */
unsigned get_device_parent(unsigned device_id)
{
  if(device_id == RETRO_DEVICE_NONE) return RETRO_DEVICE_NONE;

  switch(device_id)
  {
    case PAD_CLASSIC:
    case PAD_FIGHTSTICK:
    case PAD_8BUTTON:
    case PAD_6BUTTON:
      return RETRO_DEVICE_JOYPAD;
  }

  return INT_MAX;
}

/* get_retro_code
 * converts from OSD_ in mame2003.h to the codes from libretro.h
 * returns INT_MAX if the code is not valid
 */
int get_retro_code(const char* type, unsigned osd_code)
{
  if(strcmp(type, "retropad") == 0)
  {
    switch(osd_code)
    {
      case  OSD_JOYPAD_B:       return RETRO_DEVICE_ID_JOYPAD_B;
      case  OSD_JOYPAD_Y:       return RETRO_DEVICE_ID_JOYPAD_Y;
      case  OSD_JOYPAD_SELECT:  return RETRO_DEVICE_ID_JOYPAD_SELECT;
      case  OSD_JOYPAD_START:   return RETRO_DEVICE_ID_JOYPAD_START;
      case  OSD_JOYPAD_UP:      return RETRO_DEVICE_ID_JOYPAD_UP;
      case  OSD_JOYPAD_DOWN:    return RETRO_DEVICE_ID_JOYPAD_DOWN;
      case  OSD_JOYPAD_LEFT:    return RETRO_DEVICE_ID_JOYPAD_LEFT;
      case  OSD_JOYPAD_RIGHT:   return RETRO_DEVICE_ID_JOYPAD_RIGHT;
      case  OSD_JOYPAD_A:       return RETRO_DEVICE_ID_JOYPAD_A;
      case  OSD_JOYPAD_X:       return RETRO_DEVICE_ID_JOYPAD_X;
      case  OSD_JOYPAD_L:       return RETRO_DEVICE_ID_JOYPAD_L;
      case  OSD_JOYPAD_R:       return RETRO_DEVICE_ID_JOYPAD_R;
      case  OSD_JOYPAD_L2:      return RETRO_DEVICE_ID_JOYPAD_L2;
      case  OSD_JOYPAD_R2:      return RETRO_DEVICE_ID_JOYPAD_R2;
      case  OSD_JOYPAD_L3:      return RETRO_DEVICE_ID_JOYPAD_L3;
      case  OSD_JOYPAD_R3:      return RETRO_DEVICE_ID_JOYPAD_R3;
    }
  }
  else if(strcmp(type, "mouse") == 0)
  {
    switch(osd_code)
    {
      case  OSD_MOUSE_BUTTON_1:  return RETRO_DEVICE_ID_MOUSE_LEFT;
      case  OSD_MOUSE_BUTTON_2:  return RETRO_DEVICE_ID_MOUSE_RIGHT;
      case  OSD_MOUSE_BUTTON_3:  return RETRO_DEVICE_ID_MOUSE_MIDDLE;
      case  OSD_MOUSE_BUTTON_4:  return RETRO_DEVICE_ID_MOUSE_BUTTON_4;
      case  OSD_MOUSE_BUTTON_5:  return RETRO_DEVICE_ID_MOUSE_BUTTON_5;
    }
  }
  else if(strcmp(type, "lightgun") == 0)
  {
    switch(osd_code)
    {
      case  OSD_LIGHTGUN_IS_OFFSCREEN:    return RETRO_DEVICE_ID_LIGHTGUN_IS_OFFSCREEN;
      case  OSD_LIGHTGUN_IS_TRIGGER:      return RETRO_DEVICE_ID_LIGHTGUN_TRIGGER;       /*Status Check*/
      case  OSD_LIGHTGUN_RELOAD:          return RETRO_DEVICE_ID_LIGHTGUN_RELOAD;        /*Forced off-screen shot*/
      case  OSD_LIGHTGUN_AUX_A:           return RETRO_DEVICE_ID_LIGHTGUN_AUX_A;
      case  OSD_LIGHTGUN_AUX_B:           return RETRO_DEVICE_ID_LIGHTGUN_AUX_B;
      case  OSD_LIGHTGUN_START:           return RETRO_DEVICE_ID_LIGHTGUN_START;
      case  OSD_LIGHTGUN_SELECT:          return RETRO_DEVICE_ID_LIGHTGUN_SELECT;
      case  OSD_LIGHTGUN_AUX_C:           return RETRO_DEVICE_ID_LIGHTGUN_AUX_C;
      case  OSD_LIGHTGUN_DPAD_UP:         return RETRO_DEVICE_ID_LIGHTGUN_DPAD_UP;
      case  OSD_LIGHTGUN_DPAD_DOWN:       return RETRO_DEVICE_ID_LIGHTGUN_DPAD_DOWN;
      case  OSD_LIGHTGUN_DPAD_LEFT:       return RETRO_DEVICE_ID_LIGHTGUN_DPAD_LEFT;
      case  OSD_LIGHTGUN_DPAD_RIGHT:      return RETRO_DEVICE_ID_LIGHTGUN_DPAD_RIGHT;
    }
  }
  else
    log_cb(RETRO_LOG_WARN, LOGPRE "get_retro_code() called with invalid type! %s\n", type);

  return INT_MAX; /* no match found */
}

/* surely there is a MAME function equivalent already for PLAYER_CODE_COMPARE
 * and get_ctrl_ipt_code(). but I haven't found it.
 */

#define PLAYER_CODE_COMPARE(PLAYER_NUMBER)                                                        \
  switch(standard_code)                                                                           \
  {                                                                                               \
    case JOYCODE_##PLAYER_NUMBER##_SELECT:         return IPT_COIN##PLAYER_NUMBER;                \
    case JOYCODE_##PLAYER_NUMBER##_START:          return IPT_START##PLAYER_NUMBER;               \
    case JOYCODE_##PLAYER_NUMBER##_UP:             return player_flag | IPT_JOYSTICK_UP;          \
    case JOYCODE_##PLAYER_NUMBER##_DOWN:           return player_flag | IPT_JOYSTICK_DOWN;        \
    case JOYCODE_##PLAYER_NUMBER##_LEFT:           return player_flag | IPT_JOYSTICK_LEFT;        \
    case JOYCODE_##PLAYER_NUMBER##_RIGHT:          return player_flag | IPT_JOYSTICK_RIGHT;       \
    case JOYCODE_##PLAYER_NUMBER##_BUTTON1:        return player_flag | IPT_BUTTON1;              \
    case JOYCODE_##PLAYER_NUMBER##_BUTTON2:        return player_flag | IPT_BUTTON2;              \
    case JOYCODE_##PLAYER_NUMBER##_BUTTON3:        return player_flag | IPT_BUTTON3;              \
    case JOYCODE_##PLAYER_NUMBER##_BUTTON4:        return player_flag | IPT_BUTTON4;              \
    case JOYCODE_##PLAYER_NUMBER##_BUTTON5:        return player_flag | IPT_BUTTON5;              \
    case JOYCODE_##PLAYER_NUMBER##_BUTTON6:        return player_flag | IPT_BUTTON6;              \
    case JOYCODE_##PLAYER_NUMBER##_BUTTON7:        return player_flag | IPT_BUTTON7;              \
    case JOYCODE_##PLAYER_NUMBER##_BUTTON8:        return player_flag | IPT_BUTTON8;              \
    case JOYCODE_##PLAYER_NUMBER##_BUTTON9:        return player_flag | IPT_BUTTON9;              \
    case JOYCODE_##PLAYER_NUMBER##_BUTTON10:       return player_flag | IPT_BUTTON10;             \
    case JOYCODE_##PLAYER_NUMBER##_LEFT_UP:        return player_flag | IPT_JOYSTICKLEFT_UP;      \
    case JOYCODE_##PLAYER_NUMBER##_LEFT_DOWN:      return player_flag | IPT_JOYSTICKLEFT_DOWN;    \
    case JOYCODE_##PLAYER_NUMBER##_LEFT_LEFT:      return player_flag | IPT_JOYSTICKLEFT_LEFT;    \
    case JOYCODE_##PLAYER_NUMBER##_LEFT_RIGHT:     return player_flag | IPT_JOYSTICKLEFT_RIGHT;   \
    case JOYCODE_##PLAYER_NUMBER##_RIGHT_UP:       return player_flag | IPT_JOYSTICKRIGHT_UP;     \
    case JOYCODE_##PLAYER_NUMBER##_RIGHT_DOWN:     return player_flag | IPT_JOYSTICKRIGHT_DOWN;   \
    case JOYCODE_##PLAYER_NUMBER##_RIGHT_LEFT:     return player_flag | IPT_JOYSTICKRIGHT_LEFT;   \
    case JOYCODE_##PLAYER_NUMBER##_RIGHT_RIGHT:    return player_flag | IPT_JOYSTICKRIGHT_RIGHT;  \
    case JOYCODE_MOUSE_##PLAYER_NUMBER##_BUTTON1:  return player_flag | IPT_BUTTON1;              \
    case JOYCODE_MOUSE_##PLAYER_NUMBER##_BUTTON2:  return player_flag | IPT_BUTTON2;              \
    case JOYCODE_MOUSE_##PLAYER_NUMBER##_BUTTON3:  return player_flag | IPT_BUTTON3;              \
    case JOYCODE_MOUSE_##PLAYER_NUMBER##_BUTTON4:  return player_flag | IPT_BUTTON4;              \
    case JOYCODE_MOUSE_##PLAYER_NUMBER##_BUTTON5:  return player_flag | IPT_BUTTON5;              \
    case JOYCODE_MOUSE_##PLAYER_NUMBER##_BUTTON6:  return player_flag | IPT_BUTTON6;              \
    case JOYCODE_GUN_##PLAYER_NUMBER##_BUTTON1:    return player_flag | IPT_BUTTON1;              \
    case JOYCODE_GUN_##PLAYER_NUMBER##_BUTTON2:    return player_flag | IPT_BUTTON2;              \
    case JOYCODE_GUN_##PLAYER_NUMBER##_BUTTON3:    return player_flag | IPT_BUTTON3;              \
    case JOYCODE_GUN_##PLAYER_NUMBER##_BUTTON4:    return player_flag | IPT_BUTTON4;              \
    case JOYCODE_GUN_##PLAYER_NUMBER##_START:      return IPT_START##PLAYER_NUMBER;               \
    case JOYCODE_GUN_##PLAYER_NUMBER##_SELECT:     return IPT_COIN##PLAYER_NUMBER;                \
    case JOYCODE_GUN_##PLAYER_NUMBER##_DPAD_UP:    return player_flag | IPT_JOYSTICK_UP;          \
    case JOYCODE_GUN_##PLAYER_NUMBER##_DPAD_DOWN:  return player_flag | IPT_JOYSTICK_DOWN;        \
    case JOYCODE_GUN_##PLAYER_NUMBER##_DPAD_LEFT:  return player_flag | IPT_JOYSTICK_LEFT;        \
    case JOYCODE_GUN_##PLAYER_NUMBER##_DPAD_RIGHT: return player_flag | IPT_JOYSTICK_RIGHT;       \
  }                                                                                               \

unsigned get_ctrl_ipt_code(unsigned player_number, unsigned standard_code)
{
  int player_flag = 0;

  switch(player_number)
  {
    case 1:  player_flag = IPF_PLAYER1; PLAYER_CODE_COMPARE(1) break;
    case 2:  player_flag = IPF_PLAYER2; PLAYER_CODE_COMPARE(2) break;
    case 3:  player_flag = IPF_PLAYER3; PLAYER_CODE_COMPARE(3) break;
    case 4:  player_flag = IPF_PLAYER4; PLAYER_CODE_COMPARE(4) break;
    case 5:  player_flag = IPF_PLAYER5; PLAYER_CODE_COMPARE(5) break;
    case 6:  player_flag = IPF_PLAYER6; PLAYER_CODE_COMPARE(6) break;
    case 7:  player_flag = IPF_PLAYER7; PLAYER_CODE_COMPARE(7) break;
    case 8:  player_flag = IPF_PLAYER8; PLAYER_CODE_COMPARE(8) break;
    default: player_flag = IPF_PLAYER1; PLAYER_CODE_COMPARE(1) break;
  }

  return CODE_NONE; /* no match was found */
}


/*
 * Each line created by these emitters composes a JoystickInfo struct.
 *
 * struct JoystickInfo
 * {
 *   const char *name;        // OS dependant name; 0 terminates the list
 *   unsigned code;           // OS dependant code
 *   InputCode standardcode;	// CODE_xxx equivalent from list in input.h, or CODE_OTHER if n/a
 * };
 *
 * In the context of MAME 2003+, the 'OS' is the libretro, so we determine the unique codes for
 * our input. When the control mappings are emitted, the input codes are incremented by 1000 * the
 * player number so that simple arithmetic can determine which is the associated player.
 */

#define EMIT_RETROPAD_CLASSIC(DISPLAY_IDX) \
  {"RP"  #DISPLAY_IDX " B",   (DISPLAY_IDX * 1000) + OSD_JOYPAD_B,   JOYCODE_##DISPLAY_IDX##_BUTTON1},  \
  {"RP"  #DISPLAY_IDX " A",   (DISPLAY_IDX * 1000) + OSD_JOYPAD_A,   JOYCODE_##DISPLAY_IDX##_BUTTON2},  \
  {"RP"  #DISPLAY_IDX " Y",   (DISPLAY_IDX * 1000) + OSD_JOYPAD_Y,   JOYCODE_##DISPLAY_IDX##_BUTTON3},  \
  {"RP"  #DISPLAY_IDX " X",   (DISPLAY_IDX * 1000) + OSD_JOYPAD_X,   JOYCODE_##DISPLAY_IDX##_BUTTON4},  \
  {"RP"  #DISPLAY_IDX " L",   (DISPLAY_IDX * 1000) + OSD_JOYPAD_L,   JOYCODE_##DISPLAY_IDX##_BUTTON5},  \
  {"RP"  #DISPLAY_IDX " R",   (DISPLAY_IDX * 1000) + OSD_JOYPAD_R,   JOYCODE_##DISPLAY_IDX##_BUTTON6},  \
  {"RP"  #DISPLAY_IDX " L2",  (DISPLAY_IDX * 1000) + OSD_JOYPAD_L2,  JOYCODE_##DISPLAY_IDX##_BUTTON7},  \
  {"RP"  #DISPLAY_IDX " R2",  (DISPLAY_IDX * 1000) + OSD_JOYPAD_R2,  JOYCODE_##DISPLAY_IDX##_BUTTON8},  \
  {"RP"  #DISPLAY_IDX " L3",  (DISPLAY_IDX * 1000) + OSD_JOYPAD_L3,  JOYCODE_##DISPLAY_IDX##_BUTTON9},  \
  {"RP"  #DISPLAY_IDX " R3",  (DISPLAY_IDX * 1000) + OSD_JOYPAD_R3,  JOYCODE_##DISPLAY_IDX##_BUTTON10}, \
  EMIT_COMMON_CODES(DISPLAY_IDX)

#define EMIT_RETROPAD_FIGHTSTICK(DISPLAY_IDX) \
  {"RP"  #DISPLAY_IDX " Y",   (DISPLAY_IDX * 1000) + OSD_JOYPAD_Y,   JOYCODE_##DISPLAY_IDX##_BUTTON1},  \
  {"RP"  #DISPLAY_IDX " X",   (DISPLAY_IDX * 1000) + OSD_JOYPAD_X,   JOYCODE_##DISPLAY_IDX##_BUTTON2},  \
  {"RP"  #DISPLAY_IDX " R",   (DISPLAY_IDX * 1000) + OSD_JOYPAD_R,   JOYCODE_##DISPLAY_IDX##_BUTTON3},  \
  {"RP"  #DISPLAY_IDX " B",   (DISPLAY_IDX * 1000) + OSD_JOYPAD_B,   JOYCODE_##DISPLAY_IDX##_BUTTON4},  \
  {"RP"  #DISPLAY_IDX " A",   (DISPLAY_IDX * 1000) + OSD_JOYPAD_A,   JOYCODE_##DISPLAY_IDX##_BUTTON5},  \
  {"RP"  #DISPLAY_IDX " R2",  (DISPLAY_IDX * 1000) + OSD_JOYPAD_R2,  JOYCODE_##DISPLAY_IDX##_BUTTON6},  \
  {"RP"  #DISPLAY_IDX " L",   (DISPLAY_IDX * 1000) + OSD_JOYPAD_L,   JOYCODE_##DISPLAY_IDX##_BUTTON7},  \
  {"RP"  #DISPLAY_IDX " L2",  (DISPLAY_IDX * 1000) + OSD_JOYPAD_L2,  JOYCODE_##DISPLAY_IDX##_BUTTON8},  \
  {"RP"  #DISPLAY_IDX " L3",  (DISPLAY_IDX * 1000) + OSD_JOYPAD_L3,  JOYCODE_##DISPLAY_IDX##_BUTTON9},  \
  {"RP"  #DISPLAY_IDX " R3",  (DISPLAY_IDX * 1000) + OSD_JOYPAD_R3,  JOYCODE_##DISPLAY_IDX##_BUTTON10}, \
  EMIT_COMMON_CODES(DISPLAY_IDX)

#define EMIT_RETROPAD_8BUTTON(DISPLAY_IDX) \
  {"RP"  #DISPLAY_IDX " Y",   (DISPLAY_IDX * 1000) + OSD_JOYPAD_Y,   JOYCODE_##DISPLAY_IDX##_BUTTON1},  \
  {"RP"  #DISPLAY_IDX " X",   (DISPLAY_IDX * 1000) + OSD_JOYPAD_X,   JOYCODE_##DISPLAY_IDX##_BUTTON2},  \
  {"RP"  #DISPLAY_IDX " L",   (DISPLAY_IDX * 1000) + OSD_JOYPAD_L,   JOYCODE_##DISPLAY_IDX##_BUTTON3},  \
  {"RP"  #DISPLAY_IDX " B",   (DISPLAY_IDX * 1000) + OSD_JOYPAD_B,   JOYCODE_##DISPLAY_IDX##_BUTTON4},  \
  {"RP"  #DISPLAY_IDX " A",   (DISPLAY_IDX * 1000) + OSD_JOYPAD_A,   JOYCODE_##DISPLAY_IDX##_BUTTON5},  \
  {"RP"  #DISPLAY_IDX " L2",  (DISPLAY_IDX * 1000) + OSD_JOYPAD_L2,  JOYCODE_##DISPLAY_IDX##_BUTTON6},  \
  {"RP"  #DISPLAY_IDX " R",   (DISPLAY_IDX * 1000) + OSD_JOYPAD_R,   JOYCODE_##DISPLAY_IDX##_BUTTON7},  \
  {"RP"  #DISPLAY_IDX " R2",  (DISPLAY_IDX * 1000) + OSD_JOYPAD_R2,  JOYCODE_##DISPLAY_IDX##_BUTTON8},  \
  {"RP"  #DISPLAY_IDX " L3",  (DISPLAY_IDX * 1000) + OSD_JOYPAD_L3,  JOYCODE_##DISPLAY_IDX##_BUTTON9},  \
  {"RP"  #DISPLAY_IDX " R3",  (DISPLAY_IDX * 1000) + OSD_JOYPAD_R3,  JOYCODE_##DISPLAY_IDX##_BUTTON10}, \
  EMIT_COMMON_CODES(DISPLAY_IDX)

#define EMIT_RETROPAD_6BUTTON(DISPLAY_IDX) \
  {"RP"  #DISPLAY_IDX " Y",   (DISPLAY_IDX * 1000) + OSD_JOYPAD_Y,   JOYCODE_##DISPLAY_IDX##_BUTTON1}, \
  {"RP"  #DISPLAY_IDX " X",   (DISPLAY_IDX * 1000) + OSD_JOYPAD_X,   JOYCODE_##DISPLAY_IDX##_BUTTON2}, \
  {"RP"  #DISPLAY_IDX " L",   (DISPLAY_IDX * 1000) + OSD_JOYPAD_L,   JOYCODE_##DISPLAY_IDX##_BUTTON3}, \
  {"RP"  #DISPLAY_IDX " B",   (DISPLAY_IDX * 1000) + OSD_JOYPAD_B,   JOYCODE_##DISPLAY_IDX##_BUTTON4}, \
  {"RP"  #DISPLAY_IDX " A",   (DISPLAY_IDX * 1000) + OSD_JOYPAD_A,   JOYCODE_##DISPLAY_IDX##_BUTTON5}, \
  {"RP"  #DISPLAY_IDX " R",   (DISPLAY_IDX * 1000) + OSD_JOYPAD_R,   JOYCODE_##DISPLAY_IDX##_BUTTON6}, \
  {"RP"  #DISPLAY_IDX " L2",  (DISPLAY_IDX * 1000) + OSD_JOYPAD_L2,  JOYCODE_##DISPLAY_IDX##_BUTTON7}, \
  {"RP"  #DISPLAY_IDX " R2",  (DISPLAY_IDX * 1000) + OSD_JOYPAD_R2,  JOYCODE_##DISPLAY_IDX##_BUTTON8}, \
  {"RP"  #DISPLAY_IDX " L3",  (DISPLAY_IDX * 1000) + OSD_JOYPAD_L3,  JOYCODE_##DISPLAY_IDX##_BUTTON9}, \
  {"RP"  #DISPLAY_IDX " R3",  (DISPLAY_IDX * 1000) + OSD_JOYPAD_R3,  JOYCODE_##DISPLAY_IDX##_BUTTON10},\
  EMIT_COMMON_CODES(DISPLAY_IDX)

/* RetroPad-type input devices: */
/* The dpad, start, select, mouse, and analog axes are the same regardless of layout */
#define EMIT_COMMON_CODES(DISPLAY_IDX) \
\
  {"RP" #DISPLAY_IDX " HAT Left",   (DISPLAY_IDX * 1000) + OSD_JOYPAD_LEFT,  JOYCODE_##DISPLAY_IDX##_LEFT},  \
  {"RP" #DISPLAY_IDX " HAT Right",  (DISPLAY_IDX * 1000) + OSD_JOYPAD_RIGHT, JOYCODE_##DISPLAY_IDX##_RIGHT}, \
  {"RP" #DISPLAY_IDX " HAT Up",     (DISPLAY_IDX * 1000) + OSD_JOYPAD_UP,    JOYCODE_##DISPLAY_IDX##_UP},    \
  {"RP" #DISPLAY_IDX " HAT Down",   (DISPLAY_IDX * 1000) + OSD_JOYPAD_DOWN,  JOYCODE_##DISPLAY_IDX##_DOWN},  \
\
  {"RP" #DISPLAY_IDX " Start",      (DISPLAY_IDX * 1000) + OSD_JOYPAD_START,  JOYCODE_##DISPLAY_IDX##_START},  \
  {"RP" #DISPLAY_IDX " Select",     (DISPLAY_IDX * 1000) + OSD_JOYPAD_SELECT, JOYCODE_##DISPLAY_IDX##_SELECT}, \
\
  {"RP" #DISPLAY_IDX " AXIS 0 X-",  (DISPLAY_IDX * 1000) + OSD_ANALOG_LEFT_NEGATIVE_X,  JOYCODE_##DISPLAY_IDX##_LEFT_LEFT},   \
  {"RP" #DISPLAY_IDX " AXIS 0 X+",  (DISPLAY_IDX * 1000) + OSD_ANALOG_LEFT_POSITIVE_X,  JOYCODE_##DISPLAY_IDX##_LEFT_RIGHT},  \
  {"RP" #DISPLAY_IDX " AXIS 1 Y-",  (DISPLAY_IDX * 1000) + OSD_ANALOG_LEFT_NEGATIVE_Y,  JOYCODE_##DISPLAY_IDX##_LEFT_UP},     \
  {"RP" #DISPLAY_IDX " AXIS 1 Y+",  (DISPLAY_IDX * 1000) + OSD_ANALOG_LEFT_POSITIVE_Y,  JOYCODE_##DISPLAY_IDX##_LEFT_DOWN},   \
  {"RP" #DISPLAY_IDX " AXIS 2 X-",  (DISPLAY_IDX * 1000) + OSD_ANALOG_RIGHT_NEGATIVE_X, JOYCODE_##DISPLAY_IDX##_RIGHT_LEFT},  \
  {"RP" #DISPLAY_IDX " AXIS 2 X+",  (DISPLAY_IDX * 1000) + OSD_ANALOG_RIGHT_POSITIVE_X, JOYCODE_##DISPLAY_IDX##_RIGHT_RIGHT}, \
  {"RP" #DISPLAY_IDX " AXIS 3 Y-",  (DISPLAY_IDX * 1000) + OSD_ANALOG_RIGHT_NEGATIVE_Y, JOYCODE_##DISPLAY_IDX##_RIGHT_UP},    \
  {"RP" #DISPLAY_IDX " AXIS 3 Y+",  (DISPLAY_IDX * 1000) + OSD_ANALOG_RIGHT_POSITIVE_Y, JOYCODE_##DISPLAY_IDX##_RIGHT_DOWN},  \
\
  {"Mouse" #DISPLAY_IDX " Button1", (DISPLAY_IDX * 1000) + OSD_MOUSE_BUTTON_1, JOYCODE_MOUSE_##DISPLAY_IDX##_BUTTON1}, \
  {"Mouse" #DISPLAY_IDX " Button2", (DISPLAY_IDX * 1000) + OSD_MOUSE_BUTTON_2, JOYCODE_MOUSE_##DISPLAY_IDX##_BUTTON2}, \
  {"Mouse" #DISPLAY_IDX " Button3", (DISPLAY_IDX * 1000) + OSD_MOUSE_BUTTON_3, JOYCODE_MOUSE_##DISPLAY_IDX##_BUTTON3}, \
  {"Mouse" #DISPLAY_IDX " Button4", (DISPLAY_IDX * 1000) + OSD_MOUSE_BUTTON_4, JOYCODE_MOUSE_##DISPLAY_IDX##_BUTTON4}, \
  {"Mouse" #DISPLAY_IDX " Button5", (DISPLAY_IDX * 1000) + OSD_MOUSE_BUTTON_5, JOYCODE_MOUSE_##DISPLAY_IDX##_BUTTON5}, \
\
  EMIT_LIGHTGUN(DISPLAY_IDX)

#define EMIT_LIGHTGUN(DISPLAY_IDX) \
  {"Gun" #DISPLAY_IDX " Trigger",     (DISPLAY_IDX * 1000) + OSD_LIGHTGUN_IS_TRIGGER, JOYCODE_GUN_##DISPLAY_IDX##_BUTTON1     }, \
  {"Gun" #DISPLAY_IDX " Aux A",       (DISPLAY_IDX * 1000) + OSD_LIGHTGUN_AUX_A,      JOYCODE_GUN_##DISPLAY_IDX##_BUTTON2     }, \
  {"Gun" #DISPLAY_IDX " Aux B",       (DISPLAY_IDX * 1000) + OSD_LIGHTGUN_AUX_B,      JOYCODE_GUN_##DISPLAY_IDX##_BUTTON3     }, \
  {"Gun" #DISPLAY_IDX " Aux C",       (DISPLAY_IDX * 1000) + OSD_LIGHTGUN_AUX_C,      JOYCODE_GUN_##DISPLAY_IDX##_BUTTON4     }, \
  {"Gun" #DISPLAY_IDX " Start",       (DISPLAY_IDX * 1000) + OSD_LIGHTGUN_START,      JOYCODE_GUN_##DISPLAY_IDX##_START       }, \
  {"Gun" #DISPLAY_IDX " Select",      (DISPLAY_IDX * 1000) + OSD_LIGHTGUN_SELECT,     JOYCODE_GUN_##DISPLAY_IDX##_SELECT      }, \
  {"Gun" #DISPLAY_IDX " DPad Left",   (DISPLAY_IDX * 1000) + OSD_LIGHTGUN_DPAD_LEFT,  JOYCODE_GUN_##DISPLAY_IDX##_DPAD_LEFT   }, \
  {"Gun" #DISPLAY_IDX " DPad Right",  (DISPLAY_IDX * 1000) + OSD_LIGHTGUN_DPAD_RIGHT, JOYCODE_GUN_##DISPLAY_IDX##_DPAD_RIGHT  }, \
  {"Gun" #DISPLAY_IDX " DPad Up",     (DISPLAY_IDX * 1000) + OSD_LIGHTGUN_DPAD_UP,    JOYCODE_GUN_##DISPLAY_IDX##_DPAD_UP     }, \
  {"Gun" #DISPLAY_IDX " DPad Down",   (DISPLAY_IDX * 1000) + OSD_LIGHTGUN_DPAD_DOWN,  JOYCODE_GUN_##DISPLAY_IDX##_DPAD_DOWN   },

#define EMIT_JOYSTICK_OPTIONS(DISPLAY_IDX)      \
  {                                             \
    { EMIT_RETROPAD_CLASSIC(DISPLAY_IDX)    },     \
    { EMIT_RETROPAD_FIGHTSTICK(DISPLAY_IDX) },     \
    { EMIT_RETROPAD_8BUTTON(DISPLAY_IDX)    },     \
    { EMIT_RETROPAD_6BUTTON(DISPLAY_IDX)    },     \
  },

struct JoystickInfo alternate_joystick_maps[MAX_PLAYER_COUNT][IDX_NUMBER_OF_INPUT_TYPES][OSD_INPUT_CODES_PER_PLAYER] =
{
  EMIT_JOYSTICK_OPTIONS(1)
  EMIT_JOYSTICK_OPTIONS(2)
  EMIT_JOYSTICK_OPTIONS(3)
  EMIT_JOYSTICK_OPTIONS(4)
  EMIT_JOYSTICK_OPTIONS(5)
  EMIT_JOYSTICK_OPTIONS(6)
  EMIT_JOYSTICK_OPTIONS(7)
  EMIT_JOYSTICK_OPTIONS(8)
};

/******************************************************************************

  Joystick

******************************************************************************/

struct JoystickInfo mame_joy_map[(MAX_PLAYER_COUNT * OSD_INPUT_CODES_PER_PLAYER) + 1] = { { 0 } }; /* + 1 for final zeroed struct */

const struct JoystickInfo *osd_get_joy_list(void)
{
  unsigned needle      = 0;
  unsigned port_number = 0;

  for(port_number = 0; port_number < MAX_PLAYER_COUNT; port_number++)
  {
    int osd_code;

    for(osd_code = 0; osd_code < OSD_INPUT_CODES_PER_PLAYER; osd_code++)
    {
      int layout_idx   = 0;
      switch(options.active_control_type[port_number])
      {
        case PAD_CLASSIC:      layout_idx = IDX_CLASSIC;      break;
        case PAD_FIGHTSTICK:   layout_idx = IDX_FIGHTSTICK;   break;
        case PAD_8BUTTON:      layout_idx = IDX_8BUTTON;      break;
        case PAD_6BUTTON:      layout_idx = IDX_6BUTTON;      break;
      }
      mame_joy_map[needle] = alternate_joystick_maps[port_number][layout_idx][osd_code];
      if(!string_is_empty(mame_joy_map[needle].name)) needle++;
    }
  }

  /* the extra final record remains zeroed to indicate the end of the description to the frontend */
  mame_joy_map[needle].name         = NULL;
  mame_joy_map[needle].code         = 0;
  mame_joy_map[needle].standardcode = 0;

  return mame_joy_map;
}

/*
 * When the control mappings are emitted, the input codes are incremented by
 * 1000, 2000, 3000, etc as a simple way to indicate the corresponding player index.
 */
int osd_is_joy_pressed(int joycode)
{
  unsigned port          = calc_player_number(joycode) - 1;
  unsigned osd_code      = decode_osd_joycode(joycode);
  unsigned retro_code    = INT_MAX;

  if (!retro_running)                                   return 0; /* input callback has not yet been polled */
  if (options.input_interface == RETRO_DEVICE_KEYBOARD) return 0; /* disregard joystick input */

  /*log_cb(RETRO_LOG_DEBUG, "MAME is polling joysticks -- joycode: %i      player_number: %i      osd_code: %i\n", joycode, player_number, osd_code);*/

  /* standard retropad states */
  retro_code = get_retro_code("retropad", osd_code);
  if (retro_code != INT_MAX)
    return input_cb(port, RETRO_DEVICE_JOYPAD, 0, retro_code);

  /* pointer, mouse, or lightgun states if selected by core option */
  if (options.xy_device != RETRO_DEVICE_NONE)
  {
    retro_code = get_retro_code("mouse", osd_code);
    if (retro_code != INT_MAX)
    {
      if (retro_code == RETRO_DEVICE_ID_MOUSE_LEFT)
      {
        if (input_cb(port, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_PRESSED))
          return 1; /* check if pointer is pressed */
      }
      return input_cb(port, RETRO_DEVICE_MOUSE, 0, retro_code);
    }

    retro_code = get_retro_code("lightgun", osd_code);
    if (retro_code != INT_MAX)
    {
      if (retro_code == RETRO_DEVICE_ID_LIGHTGUN_TRIGGER)
      {
        if (input_cb(port, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_RELOAD))
          return 1; /* lightgun reload hack, report trigger as being pressed */
      }
      return input_cb(port, RETRO_DEVICE_LIGHTGUN, 0, retro_code);
    }
  }

  /* Analog joystick - read as digital button */
  /* If the analog value (normalized for MAME to the range -128, 128) is greater in absolute   */
  /* terms than INPUT_BUTTON_AXIS_THRESHOLD, return it as a binary/digital signal.             */
  if (osd_is_joystick_axis_code(joycode))
  {
    if (osd_code == OSD_ANALOG_LEFT_NEGATIVE_X) {
      if (rescale_analog( input_cb(port, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT,  RETRO_DEVICE_ID_ANALOG_X) ) < -INPUT_BUTTON_AXIS_THRESHOLD)
        return 1; }
    else if (osd_code == OSD_ANALOG_LEFT_POSITIVE_X) {
      if (rescale_analog( input_cb(port, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT,  RETRO_DEVICE_ID_ANALOG_X) ) > INPUT_BUTTON_AXIS_THRESHOLD)
        return 1; }
    else if (osd_code == OSD_ANALOG_LEFT_NEGATIVE_Y) {
      if (rescale_analog( input_cb(port, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT,  RETRO_DEVICE_ID_ANALOG_Y) ) < -INPUT_BUTTON_AXIS_THRESHOLD)
        return 1; }
    else if (osd_code == OSD_ANALOG_LEFT_POSITIVE_Y) {
      if (rescale_analog( input_cb(port, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT,  RETRO_DEVICE_ID_ANALOG_Y) ) > INPUT_BUTTON_AXIS_THRESHOLD)
        return 1; }

    else if (osd_code == OSD_ANALOG_RIGHT_NEGATIVE_X) {
      if (rescale_analog( input_cb(port, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X) ) < -INPUT_BUTTON_AXIS_THRESHOLD)
        return 1; }
    else if (osd_code == OSD_ANALOG_RIGHT_POSITIVE_X) {
      if (rescale_analog( input_cb(port, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X) ) > INPUT_BUTTON_AXIS_THRESHOLD)
        return 1; }
    else if (osd_code == OSD_ANALOG_RIGHT_NEGATIVE_Y) {
      if (rescale_analog( input_cb(port, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y) ) < -INPUT_BUTTON_AXIS_THRESHOLD)
        return 1; }
    else if (osd_code == OSD_ANALOG_RIGHT_POSITIVE_Y) {
      if (rescale_analog( input_cb(port, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y) ) > INPUT_BUTTON_AXIS_THRESHOLD)
        return 1; }
  }

  /* return not pressed */
  return 0;
}


int osd_is_joystick_axis_code(unsigned joycode)
{
  unsigned osd_code = decode_osd_joycode(joycode);

  if(osd_code >= OSD_ANALOG_LEFT_NEGATIVE_X && osd_code <= OSD_ANALOG_RIGHT_POSITIVE_Y)
    return 1;

  return 0;
}

unsigned calc_player_number(unsigned joycode)
{
  return (joycode / 1000);
}

unsigned encode_osd_joycode(unsigned player_number, unsigned osd_code)
{
  if(osd_code >= OSD_INPUT_CODES_PER_PLAYER)
    return INT_MAX;

  return (osd_code + (player_number * 1000));
}

unsigned decode_osd_joycode(unsigned joycode)
{
  return (joycode - (calc_player_number(joycode) * 1000));
}

/******************************************************************************
 * osd_analogjoy_read polls analog joystick axes, and sets the value in the
 * analog_axis[] array.
 * 
 * int player is an array index, starting at 0
*******************************************************************************/
void osd_analogjoy_read(int player, int analog_axis[MAX_ANALOG_AXES], InputCode analogjoy_input[MAX_ANALOG_AXES])
{
  int axis;
  int value;

  for(axis = 0; axis < MAX_ANALOG_AXES; axis++)
  {
    int osd_code;
    value = 0;

    if(analogjoy_input[axis] != CODE_NONE)
    {
      osd_code = decode_osd_joycode(analogjoy_input[axis]);

      if(osd_code == OSD_ANALOG_LEFT_NEGATIVE_X || osd_code == OSD_ANALOG_LEFT_POSITIVE_X)
        value = rescale_analog(input_cb(player, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT,  RETRO_DEVICE_ID_ANALOG_X));

      else if(osd_code == OSD_ANALOG_LEFT_NEGATIVE_Y || osd_code == OSD_ANALOG_LEFT_POSITIVE_Y)
        value = rescale_analog(input_cb(player, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT,  RETRO_DEVICE_ID_ANALOG_Y));

      else if(osd_code == OSD_ANALOG_RIGHT_NEGATIVE_X || osd_code == OSD_ANALOG_RIGHT_POSITIVE_X)
        value = rescale_analog(input_cb(player, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X));

      else if(osd_code == OSD_ANALOG_RIGHT_NEGATIVE_Y || osd_code == OSD_ANALOG_RIGHT_POSITIVE_Y)
        value = rescale_analog(input_cb(player, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y));

      /* opposite when reversing axis mapping */
      if((osd_code % 2) == 0) /* if osd_code is an even number */
        value = -value;

      analog_axis[axis]=value;
    }
  }
}


/******************************************************************************
 *
 * Legacy joystick calibration functions
 *
 * As of March 2021: these MAME functions should not actually be used and will not be invoked
 * as long as needs_calibration always returns 0. The libretro frontend is responsible for
 * providing calibrated position data.
 ******************************************************************************/

/* Joystick calibration routines BW 19981216 */
int osd_joystick_needs_calibration(void) { return 0; }

/* Preprocessing for joystick calibration. Returns 0 on success */
void osd_joystick_start_calibration(void){ }

/* Prepare the next calibration step. Return a description of this step. */
/* (e.g. "move to upper left") */
const char *osd_joystick_calibrate_next(void) { return 0; }

/* Get the actual joystick calibration data for the current position */
void osd_joystick_calibrate(void) { }

/* Postprocessing (e.g. saving joystick data to config) */
void osd_joystick_end_calibration(void) { }



/******************************************************************************

  Trackball, Spinner, Mouse, Pointer, Lightgun

******************************************************************************/

void osd_xy_device_read(int player, int *deltax, int *deltay, const char* type)
{
  /* return zero when no device is set */
  if (options.xy_device == RETRO_DEVICE_NONE) { *deltax = 0; *deltay = 0; return; }

  if (strcmp(type, "relative") == 0)
  {
    if(options.dial_swap_xy)
    {
      *deltax = input_cb(player, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y);
      *deltay = input_cb(player, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X);
    }
    else
    {
      *deltax = input_cb(player, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X);
      *deltay = input_cb(player, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y);
    }
  }

  else if (strcmp(type, "absolute") == 0)
  {
    if (options.xy_device == RETRO_DEVICE_POINTER)
    {
      *deltax = rescale_analog(input_cb(player, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_X));
      *deltay = rescale_analog(input_cb(player, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_Y));
    }

    else if (options.xy_device == RETRO_DEVICE_LIGHTGUN)
    {
      /* simulated lightgun reload hack */
      if(input_cb(player, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_RELOAD) ||
         input_cb(player, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_IS_OFFSCREEN) )
      {
        *deltax = -128;
        *deltay = -128;
        return;
      }
      *deltax = rescale_analog(input_cb(player, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_SCREEN_X));
      *deltay = rescale_analog(input_cb(player, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_SCREEN_Y));
    }

    else /* return zero for relative devices, to avoid tracking issues when analog overrides */
    {
      *deltax = 0;
      *deltay = 0;
    }
  }
}

/******************************************************************************
 * rescale_analog converts between the libretro coordinate system and the
 * MAME OSD coordinate system.
 *
 * RETRO_DEVICE_LIGHTGUN report X/Y coordinates in the range [-0x8000, 0x7fff]
 * in both axes, with zero being center and -0x8000 being out of bounds.
 * RETRO_DEVICE_ANALOG uses the same [-0x8000, 0x7fff] range.
 *
 * For lightguns, the MAME OSD uses delta from the middle of the screen when
 * the lightgun is fired, and 0 when the gun is inactive with a range of
 * -128 to 128. MAME OSD uses this same range for analog joysticks.
 *
 * Therefore we can use a common function to scale input from lightguns and
 * analog controls.
 ******************************************************************************/
int rescale_analog(int libretro_coordinate)
{
  static const float scale_factor = (float)MAME_ANALOG_MAX / LIBRETRO_ANALOG_MAX;

  if (libretro_coordinate == 0 || libretro_coordinate == LIBRETRO_ANALOG_MIN) return 0;

  return round(scale_factor * libretro_coordinate);
}

/******************************************************************************

  Keyboard

******************************************************************************/


const struct KeyboardInfo *osd_get_key_list(void)
{
  return retroKeys;
}

int osd_is_key_pressed(int keycode)
{
  if (!retro_running)                                  return 0; /* input callback has not yet been polled */
  if (options.input_interface == RETRO_DEVICE_JOYPAD)  return 0; /* core option is set to retropad/joystick only */

  if (keycode < RETROK_LAST && keycode >= 0)
    return input_cb(0, RETRO_DEVICE_KEYBOARD, 0, keycode);

  log_cb(RETRO_LOG_WARN, LOGPRE "Invalid OSD keycode received: %i\n", keycode); /* this should not happen when keycodes are properly registered with MAME */
  return 0;
}


int osd_readkey_unicode(int flush)
{
  /* TODO*/
  return 0;
}

/******************************************************************************

  Keymapping

******************************************************************************/


/*  MAME requires that we populate an array of KeyboardInfo structs. The code value
 *  used is up to the OSD, which in this case is the libretro API. libretro.h provides
 *  a set of keycodes suitable for this purpose, so we use those for populating our
 *  KeyboardInfo structs with the help of #define emitters.
 *
 *  struct KeyboardInfo
 *  {
 *    const char *name;       // OS dependant name; 0 terminates the list
 *    unsigned code;          // OS dependant code
 *    InputCode standardcode;	// CODE_xxx equivalent from list below, or CODE_OTHER if n/a
 * };
 *
 * Unassigned keycodes
 *	KEYCODE_OPENBRACE, KEYCODE_CLOSEBRACE, KEYCODE_BACKSLASH2, KEYCODE_STOP, KEYCODE_LWIN,
 *  KEYCODE_RWIN, KEYCODE_DEL_PAD, KEYCODE_PAUSE
 *
 * The format for each systems key constants is RETROK_$(TAG) and KEYCODE_$(TAG)
 * EMIT1(TAG): The tag value is the same between libretro and the core
 * EMIT2(RTAG, MTAG): The tag value is different between the two
 * EXITX(TAG): The core has no equivalent key.
 */

#define EMIT2(RETRO, KEY) {(char*)#RETRO, RETROK_##RETRO, KEYCODE_##KEY}
#define EMIT1(KEY) {(char*)#KEY, RETROK_##KEY, KEYCODE_##KEY}
#define EMITX(KEY) {(char*)#KEY, RETROK_##KEY, KEYCODE_OTHER}

const struct KeyboardInfo retroKeys[] =
{
    EMIT1(BACKSPACE),
    EMIT1(TAB),
    EMITX(CLEAR),

    EMIT1(BACKSPACE),
    EMIT1(TAB),
    EMITX(CLEAR),
    EMIT2(RETURN, ENTER),
    EMITX(PAUSE),
    EMIT2(ESCAPE, ESC),
    EMIT1(SPACE),
    EMITX(EXCLAIM),
    EMIT2(QUOTEDBL, TILDE),
    EMITX(HASH),
    EMITX(DOLLAR),
    EMITX(AMPERSAND),
    EMIT1(QUOTE),
    EMITX(LEFTPAREN),
    EMITX(RIGHTPAREN),
    EMIT1(ASTERISK),
    EMIT2(PLUS, EQUALS),
    EMIT1(COMMA),
    EMIT1(MINUS),
    EMITX(PERIOD),
    EMIT1(SLASH),

    EMIT1(0), EMIT1(1), EMIT1(2), EMIT1(3), EMIT1(4), EMIT1(5), EMIT1(6), EMIT1(7), EMIT1(8), EMIT1(9),

    EMIT1(COLON),
    EMITX(SEMICOLON),
    EMITX(LESS),
    EMITX(EQUALS),
    EMITX(GREATER),
    EMITX(QUESTION),
    EMITX(AT),
    EMITX(LEFTBRACKET),
    EMIT1(BACKSLASH),
    EMITX(RIGHTBRACKET),
    EMITX(CARET),
    EMITX(UNDERSCORE),
    EMITX(BACKQUOTE),

    EMIT2(a, A), EMIT2(b, B), EMIT2(c, C), EMIT2(d, D), EMIT2(e, E), EMIT2(f, F),
    EMIT2(g, G), EMIT2(h, H), EMIT2(i, I), EMIT2(j, J), EMIT2(k, K), EMIT2(l, L),
    EMIT2(m, M), EMIT2(n, N), EMIT2(o, O), EMIT2(p, P), EMIT2(q, Q), EMIT2(r, R),
    EMIT2(s, S), EMIT2(t, T), EMIT2(u, U), EMIT2(v, V), EMIT2(w, W), EMIT2(x, X),
    EMIT2(y, Y), EMIT2(z, Z),

    EMIT2(DELETE, DEL),

    EMIT2(KP0, 0_PAD), EMIT2(KP1, 1_PAD), EMIT2(KP2, 2_PAD), EMIT2(KP3, 3_PAD),
    EMIT2(KP4, 4_PAD), EMIT2(KP5, 5_PAD), EMIT2(KP6, 6_PAD), EMIT2(KP7, 7_PAD),
    EMIT2(KP8, 8_PAD), EMIT2(KP9, 9_PAD),

    EMITX(KP_PERIOD),
    EMIT2(KP_DIVIDE, SLASH_PAD),
    EMITX(KP_MULTIPLY),
    EMIT2(KP_MINUS, MINUS_PAD),
    EMIT2(KP_PLUS, PLUS_PAD),
    EMIT2(KP_ENTER, ENTER_PAD),
    EMITX(KP_EQUALS),

    EMIT1(UP), EMIT1(DOWN), EMIT1(RIGHT), EMIT1(LEFT),
    EMIT1(INSERT), EMIT1(HOME), EMIT1(END), EMIT2(PAGEUP, PGUP), EMIT2(PAGEDOWN, PGDN),

    EMIT1(F1), EMIT1(F2), EMIT1(F3), EMIT1(F4), EMIT1(F5), EMIT1(F6),
    EMIT1(F7), EMIT1(F8), EMIT1(F9), EMIT1(F10), EMIT1(F11), EMIT1(F12),
    EMITX(F13), EMITX(F14), EMITX(F15),

    EMIT1(NUMLOCK),
    EMIT1(CAPSLOCK),
    EMIT2(SCROLLOCK, SCRLOCK),
    EMIT1(RSHIFT), EMIT1(LSHIFT), EMIT2(RCTRL, RCONTROL), EMIT2(LCTRL, LCONTROL), EMIT1(RALT), EMIT1(LALT),
    EMITX(RMETA), EMITX(LMETA), EMITX(LSUPER), EMITX(RSUPER),

    EMITX(MODE),
    EMITX(COMPOSE),

    EMITX(HELP),
    EMIT2(PRINT, PRTSCR),
    EMITX(SYSREQ),
    EMITX(BREAK),
    EMIT1(MENU),
    EMITX(POWER),
    EMITX(EURO),
    EMITX(UNDO),

    {0, 0, 0}
};

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
void osd_customize_inputport_defaults(struct ipd *defaults){}


static void remove_slash (char* temp)
{
  int i;

  for(i=0; temp[i] != '\0'; ++i);

  if( (temp[i-1] == '/' || temp[i-1] == '\\') && (i > 1) )
  {
    temp[i-1] = 0;
    log_cb(RETRO_LOG_DEBUG, LOGPRE "Removed a trailing slash in path: %s\n", temp);
  }
  else
    log_cb(RETRO_LOG_DEBUG, LOGPRE "Trailing slash removal was not necessary path: %s.\n", temp);
}

static void configure_cyclone_mode (int driverIndex)
{
  /* Determine how to use cyclone if available to the platform */

#if (HAS_CYCLONE || HAS_DRZ80)
  int i;
  int use_cyclone = 1;
  int use_drz80 = 1;
  int use_drz80_snd = 1;

  /* cyclone mode core option: 0=disabled, 1=default, 2=Cyclone, 3=DrZ80, 4=Cyclone+DrZ80, 5=DrZ80(snd), 6=Cyclone+DrZ80(snd) */
  switch (options.cyclone_mode)
  {
    case 0:
      use_cyclone = 0;
      use_drz80_snd = 0;
      use_drz80 = 0;
      break;

    case 1:
      for (i=0;i<NUMGAMES;i++)
      {
        /* ASM cores: 0=disabled, 1=Cyclone, 2=DrZ80, 3=Cyclone+DrZ80, 4=DrZ80(snd), 5=Cyclone+DrZ80(snd) */
        if (strcmp(drivers[driverIndex]->name,fe_drivers[i].name)==0)
        {
          switch (fe_drivers[i].cores)
          {
            case 0:
              use_cyclone = 0;
              use_drz80_snd = 0;
              use_drz80 = 0;
              break;
            case 1:
              use_drz80_snd = 0;
              use_drz80 = 0;
              break;
            case 2:
              use_cyclone = 0;
              break;
            case 4:
              use_cyclone = 0;
              use_drz80 = 0;
              break;
            case 5:
              use_drz80 = 0;
              break;
            default:
              break;
          }

          break; /* end for loop */
        }
      }
      break; /* end case 1 */

    case 2:
      use_drz80_snd = 0;
      use_drz80 = 0;
      break;

    case 3:
      use_cyclone = 0;
      break;

    case 5:
      use_cyclone = 0;
      use_drz80 = 0;
      break;

    case 6:
      use_drz80 = 0;
      break;

    default:
      break;
  }

#if (HAS_CYCLONE)
  /* Replace M68000 by CYCLONE */
  if (use_cyclone)
  {
    for (i=0;i<MAX_CPU;i++)
    {
      unsigned int *type=(unsigned int *)&(Machine->drv->cpu[i].cpu_type);

#ifdef NEOMAME
      if (*type==CPU_M68000)
#else
      if (*type==CPU_M68000 || *type==CPU_M68010 )
#endif
      {
        *type=CPU_CYCLONE;
        log_cb(RETRO_LOG_INFO, LOGPRE "Replaced CPU_CYCLONE\n");
      }

      if (!(*type)) break;
    }
  }
#endif

#if (HAS_DRZ80)
  /* Replace Z80 by DRZ80 */
  if (use_drz80)
  {
    for (i=0;i<MAX_CPU;i++)
    {
      unsigned int *type=(unsigned int *)&(Machine->drv->cpu[i].cpu_type);
      if (type==CPU_Z80)
      {
        *type=CPU_DRZ80;
        log_cb(RETRO_LOG_INFO, LOGPRE "Replaced Z80\n");
      }
    }
  }

  /* Replace Z80 with DRZ80 only for sound CPUs */
  if (use_drz80_snd)
  {
    for (i=0;i<MAX_CPU;i++)
    {
      int *type=(int*)&(Machine->drv->cpu[i].cpu_type);
      if (type==CPU_Z80 && Machine->drv->cpu[i].cpu_flags&CPU_AUDIO_CPU)
      {
        *type=CPU_DRZ80;
        log_cb(RETRO_LOG_INFO, LOGPRE "Replaced Z80 sound\n");
      }
    }
  }
#endif

#endif
}
