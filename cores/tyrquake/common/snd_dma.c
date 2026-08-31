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

/*
 * snd_dma.c -- main control for any streaming sound output device
 */

#include "compat/strl.h"
#include "bspfile.h"
#include "client.h"
#include "cmd.h"
#include "common.h"
#include "console.h"
#include "input.h"
#include "model.h"
#include "quakedef.h"
#include "sound.h"
#include "snd_codec.h"
#include "bgmusic.h"
#include "sys.h"

#include "host.h"

/* FIXME - reorder to remove forward decls? */
static void S_Play(void);
static void S_PlayVol(void);
static void S_StopAllSoundsC(void);

/*
 * Internal sound data & structures
 */

channel_t channels[MAX_CHANNELS];
int total_channels;

int snd_blocked = 0;
static qboolean snd_ambient = 1;
static qboolean snd_initialized = false;

/* pointer should go away (JC?) */
volatile dma_t *shm = 0;
static dma_t sn;

static vec3_t listener_origin;
static vec3_t listener_forward;
static vec3_t listener_right;
static vec3_t listener_up;
static vec_t sound_nominal_clip_dist = 1000.0;

#define	MAX_SFX 512
static sfx_t *known_sfx;	/* hunk allocated [MAX_SFX] */
static int num_sfx;

/*
 * ==================
 * S_ValidSfx
 *
 * Returns true iff sfx points to an aligned slot inside the
 * known_sfx[] array.  Same defense pattern as R_ValidParticle
 * and R_ValidEfrag -- known_sfx is hunk-allocated once at
 * startup and never moves, so any sfx_t* not falling on a
 * proper slot inside [known_sfx, known_sfx + num_sfx) is
 * garbage from a stomp in unrelated subsystem.
 *
 * S_LoadSound/S_PaintChannels/S_StartSound previously trusted
 * any non-NULL sfx -- the channels[].sfx field had no guard
 * against stale or wild pointers.  Crash signature: SIGSEGV
 * deep in Cache_Check (c->data deref) called from S_LoadSound
 * called from S_PaintChannels with ch->sfx pointing into the
 * malloc'd heap somewhere outside the known_sfx range.
 * ==================
 */
int
S_ValidSfx(const sfx_t *sfx)
{
    const char *base = (const char *)known_sfx;
    const char *end  = base + (size_t)num_sfx * sizeof(sfx_t);
    const char *q    = (const char *)sfx;
    if (!sfx || !known_sfx || num_sfx <= 0)
	return 0;
    if (q < base || q >= end)
	return 0;
    if (((size_t)(q - base) % sizeof(sfx_t)) != 0)
	return 0;
    return 1;
}

int s_rawhead;
int s_rawavail;
portable_samplepair_t	s_rawsamples[MAX_RAW_SAMPLES];
float_samplepair_t	s_rawsamples_f[MAX_RAW_SAMPLES];

static sfx_t *ambient_sfx[NUM_AMBIENTS];

static int sound_started = 0;

cvar_t bgmvolume = { "bgmvolume", "1", true };
cvar_t sfxvolume = { "volume", "0.7", true };

static cvar_t precache = { "precache", "1" };
static cvar_t ambient_level = { "ambient_level", "0.3" };
static cvar_t ambient_fade = { "ambient_fade", "100" };

/*
 * ================
 * S_Startup
 * ================
 */

void
S_Startup(void)
{
   if (!snd_initialized)
      return;

   if (!SNDDMA_Init(&sn))
   {
      sound_started = 0;
      return;
   }
   sound_started = 1;
}


/*
 * ================
 * S_Init
 * ================
 */
void
S_Init(void)
{
    Con_Printf("\nSound Initialization\n");

    Cmd_AddCommand("play", S_Play);
    Cmd_AddCommand("playvol", S_PlayVol);
    Cmd_AddCommand("stopsound", S_StopAllSoundsC);

    Cvar_RegisterVariable(&sfxvolume);
    Cvar_RegisterVariable(&precache);
    Cvar_RegisterVariable(&bgmvolume);
    Cvar_RegisterVariable(&ambient_level);
    Cvar_RegisterVariable(&ambient_fade);

    snd_initialized = true;

    S_Startup();

    known_sfx = (sfx_t*)Hunk_Alloc(MAX_SFX * sizeof(sfx_t));
    num_sfx = 0;

    if (sound_started)
       Con_Printf("Sound sampling rate: %i\n", shm->speed);

    ambient_sfx[AMBIENT_WATER] = S_PrecacheSound("ambience/water1.wav");
    ambient_sfx[AMBIENT_SKY] = S_PrecacheSound("ambience/wind2.wav");

    S_CodecInit();

    S_StopAllSounds(true);
}


/*
 * Shutdown sound engine
 */
void
S_Shutdown(void)
{
   if (!sound_started)
      return;

   /* Clear sound_started first so any reader that observes
    * the new sound_started==0 won't proceed into the mixer
    * and try to dereference the just-cleared shm. */
   sound_started = 0;
   shm = 0;
}

/*
 * ==================
 * S_FindName
 * ==================
 */
static sfx_t *
S_FindName(const char *name)
{
    int i;
    sfx_t *sfx;

    if (!name)
	Sys_Error("%s: NULL", __func__);
    if (strlen(name) >= MAX_QPATH)
	Sys_Error("%s: name too long: %s", __func__, name);

    /* see if already loaded */
    for (i = 0; i < num_sfx; i++)
	if (!strcmp(known_sfx[i].name, name))
	    return &known_sfx[i];

    if (num_sfx == MAX_SFX)
	Sys_Error("%s: out of sfx_t", __func__);

    sfx = &known_sfx[i];
    strlcpy(sfx->name, name, sizeof(sfx->name));

    num_sfx++;

    return sfx;
}


/*
 * ==================
 * S_TouchSound
 * ==================
 */
void
S_TouchSound(const char *name)
{
    sfx_t *sfx;

    if (!sound_started)
	return;

    sfx = S_FindName(name);
    Cache_Check(&sfx->cache);
}

/*
 * ==================
 * S_PrecacheSound
 * ==================
 */
sfx_t *
S_PrecacheSound(const char *name)
{
    sfx_t *sfx;

    if (!sound_started)
	return NULL;

    sfx = S_FindName(name);

    /* cache it in */
    if (precache.value)
	S_LoadSound(sfx);

    return sfx;
}


/*
 * =================
 * SND_PickChannel
 * =================
 */
static channel_t *
SND_PickChannel(int entnum, int entchannel)
{
    int i;
    int life_left;
    channel_t *channel;
    channel_t *first_to_die = NULL;

    /* Check for replacement sound, or find the best one to replace */
    life_left = 0x7fffffff;
    for (i = NUM_AMBIENTS; i < NUM_AMBIENTS + MAX_DYNAMIC_CHANNELS; i++) {
	channel = &channels[i];
	/*
	 * - channel 0 never overrides
	 * - allways override sound from same entity
	 */
	if (entchannel != 0 && channel->entnum == entnum &&
	    (channel->entchannel == entchannel || entchannel == -1)) {
	    first_to_die = channel;
	    break;
	}
	/* don't let monster sounds override player sounds */
	if (channel->entnum == cl.viewentity
	    && entnum != cl.viewentity && channel->sfx)
	    continue;
	if (channel->remaining_samples < life_left) {
	    life_left = channel->remaining_samples;
	    first_to_die = channel;
	}
    }
    if (first_to_die && first_to_die->sfx)
	first_to_die->sfx = NULL;

    return first_to_die;
}

/*
 * =================
 * SND_Spatialize
 *=================
 */
static void
SND_Spatialize(channel_t *ch)
{
    vec_t dot;
    vec_t dist;
    vec_t lscale, rscale, scale;
    vec3_t source_vec;

    /* anything coming from the view entity will allways be full volume */
    if (ch->entnum == cl.viewentity) {
	ch->leftvol = ch->master_vol;
	ch->rightvol = ch->master_vol;
	return;
    }

    /* calculate stereo seperation and distance attenuation */
    VectorSubtract(ch->origin, listener_origin, source_vec);
    dist = VectorNormalize(source_vec) * ch->dist_mult;

    dot = DotProduct(listener_right, source_vec);
    rscale = 1.0 + dot;
    lscale = 1.0 - dot;

    /* add in distance effect */
    scale = (1.0 - dist) * rscale;
    ch->rightvol = (int)(ch->master_vol * scale);
    if (ch->rightvol < 0)
	ch->rightvol = 0;

    scale = (1.0 - dist) * lscale;
    ch->leftvol = (int)(ch->master_vol * scale);
    if (ch->leftvol < 0)
	ch->leftvol = 0;
}


void
S_StartSound(int entnum, int entchannel, sfx_t *sfx, vec3_t origin,
	     float fvol, float attenuation)
{
    channel_t *target_chan, *check;
    sfxcache_t *sc;
    int vol;
    int ch_idx;
    int skip;

    if (!sound_started)
	return;
    if (!sfx)
	return;
    /* Defensive: sfx may have been read from a stale or
     * out-of-bounds index in cl.sound_precache[].  Reject
     * pointers that don't refer to a real known_sfx[] slot
     * before storing into channels[].sfx, where it would
     * later crash S_PaintChannels. */
    if (!S_ValidSfx(sfx))
	return;

    vol = fvol * 255;

    /* pick a channel to play on */
    target_chan = SND_PickChannel(entnum, entchannel);
    if (!target_chan)
	return;

    /* spatialize */
    memset(target_chan, 0, sizeof(*target_chan));
    VectorCopy(origin, target_chan->origin);
    target_chan->dist_mult = attenuation / sound_nominal_clip_dist;
    target_chan->master_vol = vol;
    target_chan->entnum = entnum;
    target_chan->entchannel = entchannel;
    SND_Spatialize(target_chan);

    if (!target_chan->leftvol && !target_chan->rightvol)
	return;			/* not audible at all */

    /* new channel */
    sc = S_LoadSound(sfx);
    if (!sc) {
	target_chan->sfx = NULL;
	return;			/* couldn't load the sound's data */
    }

    target_chan->sfx = sfx;
    target_chan->pos = 0.0;
    target_chan->remaining_samples = sc->length;

    /*
     * if an identical sound has also been started this frame, offset the pos
     * a bit to keep it from just making the first one louder
     */
    check = &channels[NUM_AMBIENTS];
    for (ch_idx = NUM_AMBIENTS; ch_idx < NUM_AMBIENTS + MAX_DYNAMIC_CHANNELS;
	 ch_idx++, check++) {
	if (check == target_chan)
	    continue;
	if (check->sfx == sfx && !check->pos) {
	    skip = rand() % (int)(0.1 * shm->speed);
	    if (skip >= target_chan->remaining_samples)
		skip = target_chan->remaining_samples - 1;
	    target_chan->pos += skip;
	    target_chan->remaining_samples -= skip;
	    break;
	}
    }
}

void
S_StopSound(int entnum, int entchannel)
{
    int i;

    for (i = 0; i < MAX_DYNAMIC_CHANNELS; i++) {
	if (channels[i].entnum == entnum
	    && channels[i].entchannel == entchannel) {
	    channels[i].remaining_samples = 0;
	    channels[i].sfx = NULL;
	    return;
	}
    }
}

void
S_StopAllSounds(qboolean clear)
{
    int i;

    if (!sound_started)
	return;

    /* no statics */
    total_channels = MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS;

    for (i = 0; i < MAX_CHANNELS; i++)
	if (channels[i].sfx)
	    channels[i].sfx = NULL;

    memset(channels, 0, MAX_CHANNELS * sizeof(channel_t));
    if (clear)
	S_ClearBuffer();
}

static void
S_StopAllSoundsC(void)
{
    S_StopAllSounds(true);
}

void S_ClearBuffer(void)
{
   if (!sound_started || !shm)
      return;

   memset(shm->buffer, 0, AUDIO_BUFFER_SIZE * 16 / 8);
}

/*
===================
S_RawSamplesFloat

Float counterpart of S_RawSamples' integer body, used when float output was
negotiated.  Appends into the float FIFO (s_rawsamples_f) as normalized
[-1,1] * volume.  Unlike the int path (which truncates to the nearest source
sample), the float lane linearly interpolates between adjacent source samples,
since float makes the interpolation exact and music here is not part of the
deterministic SFX mix.  When the source and output rates match (scale == 1.0)
the fraction is 0 and each output equals its source sample, so that common
case is bit-identical to the old nearest path.  Linear interpolation removes
the zipper/repeat artifacts of fractional and up-sampling rate conversion
(e.g. 44.1 kHz music into a 48 kHz core); integer down-sampling still aliases
and would want a decimating FIR, which is deliberately left out of scope here.

Source width selects how each sample is read (and folded into the volume
scale): 4 -> IEEE-754 float in [-1,1]; 2 -> signed 16-bit * 1/32768;
1 -> unsigned 8-bit (x-128) * 1/128.
===================
*/
static float S_FetchFloat (const byte *data, int width, int k, float v)
{
	if (width == 2)
		return (float)(((const short *)data)[k]) * v;
	if (width == 1)
		return (float)((int)data[k] - 128) * v;
	return ((const float *)data)[k] * v;	/* width == 4 */
}

static void S_RawSamplesFloat (int samples, float scale, int width,
		int nchannels, byte *data, float volume)
{
	int   i, idx, n, dst;
	float v, pos, frac;

	/* fold the per-width normalization into the volume scale */
	if (width == 2)
		v = volume * (1.0f / 32768.0f);
	else if (width == 1)
		v = volume * (1.0f / 128.0f);
	else
		v = volume;			/* width == 4 */

	for (i = 0; ; i++)
	{
		pos = i * scale;
		idx = (int)pos;
		if (idx >= samples || s_rawavail >= MAX_RAW_SAMPLES)
			break;
		frac = pos - (float)idx;
		/* clamp the upper tap at the buffer end (per-call boundary; the
		 * last fraction holds rather than interpolating past 'samples') */
		n = (idx + 1 < samples) ? (idx + 1) : idx;

		dst = (s_rawhead + s_rawavail) & (MAX_RAW_SAMPLES - 1);
		s_rawavail++;

		if (nchannels == 2)
		{
			float l0 = S_FetchFloat(data, width, idx * 2,     v);
			float l1 = S_FetchFloat(data, width, n   * 2,     v);
			float r0 = S_FetchFloat(data, width, idx * 2 + 1, v);
			float r1 = S_FetchFloat(data, width, n   * 2 + 1, v);
			s_rawsamples_f[dst].left  = l0 + (l1 - l0) * frac;
			s_rawsamples_f[dst].right = r0 + (r1 - r0) * frac;
		}
		else
		{
			float s0 = S_FetchFloat(data, width, idx, v);
			float s1 = S_FetchFloat(data, width, n,   v);
			s_rawsamples_f[dst].left  =
			s_rawsamples_f[dst].right = s0 + (s1 - s0) * frac;
		}
	}
}

/*
===================
S_RawSamples		(from QuakeII)

Streaming music support. Byte swapping
of data must be handled by the codec.
Expects data in signed 16 bit, or unsigned
8 bit format.
===================
*/
void S_RawSamples (int samples, int rate, int width, int nchannels, byte *data, float volume)
{
	int i;
	int src, dst;
	float scale;
	int intVolume;

	/* The per-format inner loops below exit on 'src = i*scale;
	 * if (src >= samples) break;'. If scale ends up as 0 (rate
	 * <= 0, which the WAV streaming parser now rejects, or
	 * shm->speed somehow 0 from an early call), src stays at 0
	 * forever and the loop spins indefinitely. Reject the bad
	 * input here so no future codec gap reintroduces the hang. */
	if (rate <= 0 || !shm || shm->speed <= 0)
		return;

	/* The 8-bit-width branches below also do 'intVolume *= 256'
	 * before the multiply, so a tiny pathological 'volume' can
	 * still wrap intVolume in those branches; clamp on the float
	 * side before the cast so the int math is bounded. NaN takes
	 * the IS_NAN branch (NaN<0 and NaN>1 are both false otherwise)
	 * for the same reason BGM_Update's clamp had to be widened.
	 * The bgmvolume / sfxvolume cvar clamps already handle their
	 * paths upstream, but S_RawSamples is exposed in sound.h and
	 * a future caller could hand in any float. */
	if (IS_NAN(volume) || volume < 0.0f)
		volume = 0.0f;
	else if (volume > 1.0f)
		volume = 1.0f;

	scale = (float) rate / shm->speed;

	/* Float output lane: when float output was negotiated, append into the
	 * float FIFO (normalized [-1,1] * volume) and return.  The integer body
	 * below is the int16 output path, left byte-for-byte unchanged. */
	if (s_float_output)
	{
		S_RawSamplesFloat(samples, scale, width, nchannels, data, volume);
		return;
	}

	intVolume = (int) (256 * volume);

	/* All four format branches below share the same FIFO append
	 * pattern: write at (s_rawhead + s_rawavail) & MASK, bump
	 * s_rawavail.  The 's_rawavail >= MAX_RAW_SAMPLES' break
	 * stops us from overwriting unread samples (the FIFO is
	 * full); BGM_UpdateStream keeps s_rawavail at or near
	 * MAX_RAW_SAMPLES per frame, so the fill-into-empty-slot
	 * pattern is the steady state. */
	if (nchannels == 2 && width == 2)
	{
		for (i = 0; ; i++)
		{
			src = i * scale;
			if (src >= samples)
				break;
			if (s_rawavail >= MAX_RAW_SAMPLES)
				break;
			dst = (s_rawhead + s_rawavail) & (MAX_RAW_SAMPLES - 1);
			s_rawavail++;
			s_rawsamples [dst].left = ((short *) data)[src * 2] * intVolume;
			s_rawsamples [dst].right = ((short *) data)[src * 2 + 1] * intVolume;
		}
	}
	else if (nchannels == 1 && width == 2)
	{
		for (i = 0; ; i++)
		{
			src = i * scale;
			if (src >= samples)
				break;
			if (s_rawavail >= MAX_RAW_SAMPLES)
				break;
			dst = (s_rawhead + s_rawavail) & (MAX_RAW_SAMPLES - 1);
			s_rawavail++;
			s_rawsamples [dst].left = ((short *) data)[src] * intVolume;
			s_rawsamples [dst].right = ((short *) data)[src] * intVolume;
		}
	}
	else if (nchannels == 2 && width == 1)
	{
		intVolume *= 256;

		for (i = 0; ; i++)
		{
			src = i * scale;
			if (src >= samples)
				break;
			if (s_rawavail >= MAX_RAW_SAMPLES)
				break;
			dst = (s_rawhead + s_rawavail) & (MAX_RAW_SAMPLES - 1);
			s_rawavail++;
			s_rawsamples [dst].left = (((byte *) data)[src * 2] - 128) * intVolume;
			s_rawsamples [dst].right = (((byte *) data)[src * 2 + 1] - 128) * intVolume;
		}
	}
	else if (nchannels == 1 && width == 1)
	{
		intVolume *= 256;

		for (i = 0; ; i++)
		{
			src = i * scale;
			if (src >= samples)
				break;
			if (s_rawavail >= MAX_RAW_SAMPLES)
				break;
			dst = (s_rawhead + s_rawavail) & (MAX_RAW_SAMPLES - 1);
			s_rawavail++;
			s_rawsamples [dst].left = (((byte *) data)[src] - 128) * intVolume;
			s_rawsamples [dst].right = (((byte *) data)[src] - 128) * intVolume;
		}
	}
}


/*
 * =================
 * S_StaticSound
 * =================
 */
void
S_StaticSound(sfx_t *sfx, vec3_t origin, float vol, float attenuation)
{
    channel_t *ss;
    sfxcache_t *sc;

    if (!sfx)
	return;
    /* Defensive: see S_StartSound. */
    if (!S_ValidSfx(sfx))
	return;
    if (total_channels == MAX_CHANNELS) {
	Con_Printf("total_channels == MAX_CHANNELS\n");
	return;
    }

    sc = S_LoadSound(sfx);
    if (!sc)
	return;

    if (sc->loopstart == -1) {
	Con_Printf("Sound %s not looped\n", sfx->name);
	return;
    }

    ss = &channels[total_channels];
    total_channels++;

    ss->sfx = sfx;
    VectorCopy(origin, ss->origin);
    ss->master_vol = vol;
    ss->dist_mult = (attenuation / 64) / sound_nominal_clip_dist;
    ss->remaining_samples = sc->length;

    SND_Spatialize(ss);
}


/*
 * ===================
 * S_UpdateAmbientSounds
 * ===================
 */
static void
S_UpdateAmbientSounds(void)
{
   mleaf_t *leaf;
   int ambient_channel;
   /* Persistent float accumulator for the per-channel fade ramp.
    * The channel_t::master_vol field is int, so accumulating
    * directly into it discards the fractional part of
    * host_frametime * ambient_fade.value every frame. With the
    * default ambient_fade=100 that increment is below 1.0 above
    * ~100 fps, so on a fast frametime the int field stays at 0
    * forever and the leaf-driven ambients never become audible.
    * Keep the running ramp here in float and only project to the
    * int field for the rest of the mixer to consume. */
   static float ambient_vol_accum[NUM_AMBIENTS];

   if (!snd_ambient)
      return;

   /* calc ambient sound levels */
   if (!cl.worldmodel)
      return;

   leaf = Mod_PointInLeaf(cl.worldmodel, listener_origin);

   if (!leaf || !ambient_level.value)
   {
      for (ambient_channel = 0; ambient_channel < NUM_AMBIENTS;
            ambient_channel++) {
         channels[ambient_channel].sfx = NULL;
         ambient_vol_accum[ambient_channel] = 0.0f;
      }
      return;
   }

   for (ambient_channel = 0; ambient_channel < NUM_AMBIENTS;
         ambient_channel++)
   {
      float target;
      float accum;
      channel_t *chan = &channels[ambient_channel];
      chan->sfx = ambient_sfx[ambient_channel];

      target = ambient_level.value * leaf->ambient_sound_level[ambient_channel];
      if (target < 8)
         target = 0;

      accum = ambient_vol_accum[ambient_channel];
      if (accum < target) {
         accum += host_frametime * ambient_fade.value;
         if (accum > target)
            accum = target;
      } else if (accum > target) {
         accum -= host_frametime * ambient_fade.value;
         if (accum < target)
            accum = target;
      }
      /* ambient_level / ambient_fade are user-settable cvars
       * and can be set to NaN ("ambient_level nan") or to
       * extreme values.  target / accum then propagate NaN
       * or values outside int range, and (int)accum is UB
       * (typically INT_MIN on x86; INT_MAX or 0 on some
       * ARM).  The downstream master_vol cap at
       * SND_PaintChannelFrom16 only clamps the upper end --
       * a negative-from-UB master_vol would underflow into
       * the painting multiply.  Clamp to [0, 255] in float
       * before casting.  IS_NAN catches NaN/Inf too. */
      if (IS_NAN(accum) || accum < 0.0f)
         accum = 0.0f;
      else if (accum > 255.0f)
         accum = 255.0f;
      ambient_vol_accum[ambient_channel] = accum;

      chan->master_vol = (int)accum;
      chan->leftvol    = chan->master_vol;
      chan->rightvol   = chan->master_vol;
   }
}

static void S_Update_(void)
{
   int frame_samps;

   if (!sound_started || (snd_blocked > 0)) {
      /* Engine isn't mixing this frame -- but
       * audio_callback in libretro.c will still push
       * the contents of shm->buffer.  Zero it so we
       * push silence instead of stale samples. */
      if (shm && shm->buffer && shm->samples_per_frame > 0)
         memset(shm->buffer, 0,
                shm->samples_per_frame * sizeof(int16_t) * 2);
      return;
   }

   /* Paint exactly one video frame's worth of audio.
    * If the libretro layer hasn't told us yet
    * (samples_per_frame == 0 before the first
    * SNDDMA_Init), fall back to ~0.1s -- harmless
    * first-tick warmup.
    *
    * No paintedtime, no monotonic counter, no chop.
    * Channel lifetimes are countdown semantics in
    * channel->remaining_samples (decremented inside
    * S_PaintChannels by the count actually painted);
    * the BGM ring is a head/avail FIFO maintained by
    * S_RawSamples (write) and S_PaintChannels (read).
    * Both stay bounded by their respective limits
    * (sc->length and MAX_RAW_SAMPLES). */
   frame_samps = shm->samples_per_frame;
   if (frame_samps <= 0)
      frame_samps = shm->speed / 10;

   S_PaintChannels(frame_samps);
}

/*
 * ============
 * S_Update
 *
 * Called once each time through the main loop
 * ============
 */
void S_Update(vec3_t origin, vec3_t forward, vec3_t right, vec3_t up)
{
   int i, j;
   channel_t *ch;
   channel_t *combine;

   if (!sound_started || (snd_blocked > 0))
      return;

   VectorCopy(origin, listener_origin);
   VectorCopy(forward, listener_forward);
   VectorCopy(right, listener_right);
   VectorCopy(up, listener_up);

   /* update general area ambient sound sources */
   S_UpdateAmbientSounds();

   combine = NULL;

   /* update spatialization for static and dynamic sounds */
   ch = channels + NUM_AMBIENTS;
   for (i = NUM_AMBIENTS; i < total_channels; i++, ch++) {
      if (!ch->sfx)
         continue;
      SND_Spatialize(ch);	/* respatialize channel */
      if (!ch->leftvol && !ch->rightvol)
         continue;

      /*
       * try to combine static sounds with a previous channel of the same
       * sound effect so we don't mix five torches every frame
       */
      if (i >= MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS) {
         /* see if it can just use the last one */
         if (combine && combine->sfx == ch->sfx) {
            combine->leftvol += ch->leftvol;
            combine->rightvol += ch->rightvol;
            ch->leftvol = ch->rightvol = 0;
            continue;
         }
         /* search for one */
         combine = channels + MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS;
         for (j = MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS; j < i;
               j++, combine++)
            if (combine->sfx == ch->sfx)
               break;

         if (j == total_channels) {
            combine = NULL;
         } else {
            if (combine != ch) {
               combine->leftvol += ch->leftvol;
               combine->rightvol += ch->rightvol;
               ch->leftvol = ch->rightvol = 0;
            }
            continue;
         }
      }
   }

   /* mix some sound */
   S_Update_();
}



/*
===============================================================================

console functions

===============================================================================
*/

static void S_Play(void)
{
   static int hash = 345;
   char name[256];
   int i = 1;

   while (i < Cmd_Argc())
   {
      sfx_t *sfx;

      if (!strrchr(Cmd_Argv(i), '.'))
      {
         strlcpy(name, Cmd_Argv(i), sizeof(name));
         strlcat(name, ".wav", sizeof(name));
      }
      else
         strlcpy(name, Cmd_Argv(i), sizeof(name));
      sfx = S_PrecacheSound(name);
      S_StartSound(hash++, 0, sfx, listener_origin, 1.0, 1.0);
      i++;
   }
}

static void S_PlayVol(void)
{
   static int hash = 543;
   char name[256];
   int i = 1;

   while (i < Cmd_Argc())
   {
      float vol;
      sfx_t *sfx;

      if (!strrchr(Cmd_Argv(i), '.'))
      {
         strlcpy(name, Cmd_Argv(i), sizeof(name));
         strlcat(name, ".wav", sizeof(name));
      }
      else
         strlcpy(name, Cmd_Argv(i), sizeof(name));
      sfx = S_PrecacheSound(name);
      vol = Q_atof(Cmd_Argv(i + 1));
      S_StartSound(hash++, 0, sfx, listener_origin, vol, 1.0);
      i += 2;
   }
}

void S_LocalSound(const char *sound)
{
   sfx_t *sfx;

   if (!sound_started)
      return;

   sfx = S_PrecacheSound(sound);
   if (!sfx) {
      Con_Printf("%s: can't cache %s\n", __func__, sound);
      return;
   }
   S_StartSound(cl.viewentity, -1, sfx, vec3_origin, 1, 1);
}

void S_BeginPrecaching(void)
{
}


void S_EndPrecaching(void)
{
}
