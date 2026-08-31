/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#ifndef SOUND_H
#define SOUND_H

#include "cvar.h"
#include "mathlib.h"
#include "qtypes.h"
#include "zone.h"

#include "quakedef.h"

/* Linear audio output buffer size, in int16
 * samples (= 2 per stereo frame).  Sized for the
 * worst-case (smallest framerate, largest sample
 * rate) one-frame chunk: 48000 / 10 fps = 4800
 * stereo frames = 9600 int16.  Rounded up to the
 * next power of 2 for slop.  PAINTBUFFER_SIZE in
 * snd_mix.c is independently 16384 portable_
 * samplepair_t (per-frame mixing scratch); both
 * sizes need to comfortably hold one video frame's
 * worth of audio. */
#define AUDIO_BUFFER_SIZE 16384

/* sound.h -- client sound i/o functions */

/* FIXME - QW defines these in protocol.h, which is better? */
#define DEFAULT_SOUND_PACKET_VOLUME 255
#define DEFAULT_SOUND_PACKET_ATTENUATION 1.0

typedef struct {
    int left;
    int right;
} portable_samplepair_t;

typedef struct sfx_s {
    char name[MAX_QPATH];
    cache_user_t cache;
} sfx_t;

typedef struct {
    int length;
    int loopstart;
    int speed;
    int width;
    int stereo;
    byte data[1];		/* variable sized */
} sfxcache_t;

typedef struct {
    int speed;			/* sample rate (44100 / 22050 / 48000) */
    int samples_per_frame;	/* stereo frames per video frame; the
				 * libretro layer sets this to
				 * (speed / framerate) so the mixer
				 * paints exactly one video frame's
				 * worth of audio per S_Update. */
    unsigned char *buffer;
} dma_t;

typedef struct {
    sfx_t *sfx;			/* sfx number */
    int leftvol;		/* 0-255 volume */
    int rightvol;		/* 0-255 volume */
    int remaining_samples;	/* WAS: int end (= paintedtime + length).
				 * Countdown: samples left to mix from
				 * this channel before the sound either
				 * loops (sc->loopstart >= 0) or ends
				 * (ch->sfx = NULL).  Decremented in
				 * S_PaintChannels by the per-channel
				 * count actually painted each frame.
				 * The old absolute-paintedtime model
				 * carried a monotonic counter that
				 * walked toward INT_MAX over hours of
				 * playback; the countdown model has no
				 * such drift -- it's the same change
				 * we already made for dlight die and
				 * the entity lerp time pairs. */
    int pos;			/* sample position in sfx */
    int looping;		/* where to loop, -1 = no looping */
    int entnum;			/* to allow overriding a specific sound */
    int entchannel;		/**/
    vec3_t origin;		/* origin of sound effect */
    vec_t dist_mult;		/* distance multiplier (attenuation/clipK) */
    int master_vol;		/* 0-255 master volume */
} channel_t;

#define WAV_FORMAT_PCM	1

typedef struct
{
	int	rate;
	int	width;
	int	channels;
	int	loopstart;
	int	samples;
	int	dataofs;		/* chunk starts this many bytes from file start	*/
} wavinfo_t;

void S_Init(void);
void S_Startup(void);
void S_Shutdown(void);
void S_StartSound(int entnum, int entchannel, sfx_t *sfx,
		  vec3_t origin, float fvol, float attenuation);
void S_StaticSound(sfx_t *sfx, vec3_t origin, float vol, float attenuation);
void S_StopSound(int entnum, int entchannel);
void S_StopAllSounds(qboolean clear);
void S_ClearBuffer(void);
void S_Update(vec3_t origin, vec3_t v_forward, vec3_t v_right, vec3_t v_up);

sfx_t *S_PrecacheSound(const char *sample);
void S_TouchSound(const char *sample);
void S_BeginPrecaching(void);
void S_EndPrecaching(void);

/*
 * Returns true iff sfx points to an aligned slot inside the
 * known_sfx[] array.  Use as a guard at every call site that
 * dereferences a sfx_t pointer of uncertain provenance --
 * heap stomps elsewhere have been observed to leave bogus
 * non-NULL sfx pointers on the channels[] array, which then
 * crash deep in S_LoadSound / Cache_Check.
 */
int S_ValidSfx(const sfx_t *sfx);
void S_PaintChannels(int endtime);
void S_InitPaintChannels(void);

/* music stream support */
void S_RawSamples(int samples, int rate, int width, int nchannels, byte * data, float volume);
				/* Expects data in signed 16 bit, or unsigned 8 bit format. */

/* initializes cycling through a DMA buffer and returns information on it */
qboolean SNDDMA_Init(dma_t *dma);

/* gets the current DMA position */

/* ==================================================================== */
/* User-setable variables */
/* ==================================================================== */

#define	MAX_CHANNELS		512
#define	MAX_DYNAMIC_CHANNELS	128

extern channel_t channels[MAX_CHANNELS];

/* Channel layout (matches snd_dma.c):
 *   0 to NUM_AMBIENTS-1                                  = ambients (water, sky, slime, lava)
 *   NUM_AMBIENTS to NUM_AMBIENTS+MAX_DYNAMIC_CHANNELS-1  = dynamic entity sounds
 *   NUM_AMBIENTS+MAX_DYNAMIC_CHANNELS to total_channels  = static sounds
 */

extern int total_channels;

extern volatile dma_t *shm;

/* BGM ring-buffer cursor.  Replaces the old 'paintedtime'
 * absolute-clock pair with a head/count FIFO:
 *
 *   s_rawhead   = next read position in s_rawsamples[]
 *   s_rawavail  = number of queued stereo sample pairs
 *
 * Writer (S_RawSamples) appends at (s_rawhead + s_rawavail)
 * & (MAX_RAW_SAMPLES - 1) and bumps s_rawavail.
 * Reader (S_PaintChannels BGM mix) consumes from s_rawhead
 * and decrements s_rawavail.
 *
 * The previous model used 'paintedtime' as the absolute
 * sample-pair counter shared by both the channel mixer and
 * the BGM ring math.  In this libretro fork the audio path
 * is deterministic -- one S_Update_ call per video frame
 * paints exactly samples_per_frame stereo pairs and ships
 * them straight to audio_batch_cb -- so a monotonic clock
 * served no purpose except to provide a shared modulo for
 * the s_rawsamples ring buffer index, at the cost of a
 * threshold-chop branch every frame to keep it out of
 * INT_MAX wrap territory (~13.5h at 44.1 kHz).  The FIFO
 * pair carries the same information with bounded values
 * and no special-case cycling. */
extern int s_rawhead;
extern int s_rawavail;

extern cvar_t bgmvolume;
extern cvar_t sfxvolume;

extern int snd_blocked;

#define	MAX_RAW_SAMPLES	8192
extern	portable_samplepair_t	s_rawsamples[MAX_RAW_SAMPLES];

/* Float counterpart of s_rawsamples, used for the streaming music (BGM) FIFO
 * when float output was negotiated.  S_RawSamples fills this (normalized
 * [-1,1] * volume) instead of the int s_rawsamples, and S_PaintChannels sums
 * it on the float side of the output transfer.  The FIFO indices s_rawhead /
 * s_rawavail are shared with the int path (only one format is live per game). */
typedef struct { float left, right; } float_samplepair_t;
extern	float_samplepair_t	s_rawsamples_f[MAX_RAW_SAMPLES];

/* Float audio output state, owned by snd_mix.c and driven by the libretro
 * layer's RETRO_ENVIRONMENT_GET_AUDIO_SAMPLE_BATCH_FLOAT negotiation.  When
 * s_float_output is set the engine transfers the paintbuffer as normalized
 * float [-1,1] into snd_float_buffer; otherwise the int16 path is used. */
extern int    s_float_output;
extern float *snd_float_buffer;

void S_LocalSound(const char *s);
sfxcache_t *S_LoadSound(sfx_t *s);

wavinfo_t *GetWavinfo (const char *name, byte *wav, int wavlength);

void S_AmbientOff(void);
void S_AmbientOn(void);

#endif /* SOUND_H */
