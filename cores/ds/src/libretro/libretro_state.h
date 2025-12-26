#ifndef _LIBRETRO_STATE_H
#define _LIBRETRO_STATE_H

#include <string>
#include <vector>

#include <libretro.h>

extern struct retro_log_callback logging;

extern retro_audio_sample_batch_t audio_cb;
extern retro_environment_t environ_cb;
extern retro_input_poll_t input_poll_cb;
extern retro_input_state_t input_state_cb;
extern retro_log_printf_t log_cb;
extern retro_video_refresh_t video_cb;

#endif
