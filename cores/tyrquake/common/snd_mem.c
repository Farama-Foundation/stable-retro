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
/* snd_mem.c: sound caching */

#include "compat/strl.h"

#include "common.h"
#include "console.h"
#include "quakedef.h"
#include "sound.h"
#include "sys.h"

/*
================
ResampleSfx
================
*/
/*
 * Decode/resample WAV data into the cache. Output is always 16-bit
 * signed mono, matching the libretro frontend's S16 stereo output
 * stage (the second channel is duplicated from the first at mix time).
 *
 * Input is 8-bit unsigned PCM (the Quake-original format) or 16-bit
 * signed PCM (mods, replacement packs). 8-bit samples are widened by
 * subtracting the bias and shifting left 8, giving the equivalent
 * signed-16 representation without changing the perceived volume.
 */
static void
ResampleSfx(sfx_t *sfx, int inrate, int inwidth, const byte *data)
{
   int outcount;
   int srcsample;
   int srccount;
   float stepscale;
   int i;
   int sample, samplefrac, fracstep;
   sfxcache_t *sc = (sfxcache_t*)Cache_Check(&sfx->cache);
   if (!sc)
      return;

   stepscale  = (float)inrate / shm->speed;	/* this is usually 0.5, 1, or 2 */
   srccount   = sc->length;			/* source sample count (for interp bound) */
   outcount   = sc->length / stepscale;
   sc->length = outcount;
   if (sc->loopstart != -1)
      sc->loopstart = sc->loopstart / stepscale;

   sc->speed  = shm->speed;
   sc->width  = 2;	/* cache is always 16-bit signed mono now */
   sc->stereo = 0;

   if (stepscale == 1)
   {
      /* fast no-resample case */
      if (inwidth == 1)
      {
         for (i = 0; i < outcount; i++)
            ((short *)sc->data)[i] =
               ((int)((unsigned char)data[i]) - 128) << 8;
      }
      else
      {
         for (i = 0; i < outcount; i++)
            ((short *)sc->data)[i] =
               LittleShort(((const short *)data)[i]);
      }
      return;
   }

   /* general resampling case: fixed-point linear interpolation between
    * adjacent source samples.  samplefrac carries 8 fractional bits
    * (fracstep = stepscale * 256), so 'samplefrac & 255' is the position
    * between srcsample and the next -- previously discarded (the old path
    * truncated to srcsample, i.e. nearest-neighbour).  This is all integer
    * math, so the cached samples stay bit-identical on every platform/FPU
    * and the SFX mix remains deterministic for runahead/netplay.  At integer
    * source positions (frac == 0, e.g. any down-sampling ratio) the result is
    * identical to the old nearest output; only fractional positions change. */
   samplefrac = 0;
   fracstep   = stepscale * 256;
   for (i = 0; i < outcount; i++)
   {
      int frac = samplefrac & 255;
      int s0, s1;
      srcsample = samplefrac >> 8;
      samplefrac += fracstep;

      if (inwidth == 2)
      {
         s0 = LittleShort(((const short *)data)[srcsample]);
         s1 = (srcsample + 1 < srccount)
            ? LittleShort(((const short *)data)[srcsample + 1]) : s0;
      }
      else
      {
         s0 = ((int)((unsigned char)data[srcsample]) - 128) << 8;
         s1 = (srcsample + 1 < srccount)
            ? (((int)((unsigned char)data[srcsample + 1]) - 128) << 8) : s0;
      }

      /* s0,s1 are in signed 16-bit range, so the blend stays in range too;
       * (s1 - s0) * frac fits comfortably in int (max ~65535 * 255). */
      sample = s0 + (((s1 - s0) * frac) >> 8);
      ((short *)sc->data)[i] = (short)sample;
   }
}

/* ============================================================================= */

/*
==============
S_LoadSound
==============
*/
sfxcache_t *
S_LoadSound(sfx_t *s)
{
    char namebuffer[256];
    byte *data;
    wavinfo_t *info;
    int len;
    float stepscale;
    byte stackbuf[1024];	/* avoid dirtying the cache heap */
    sfxcache_t *sc;

    /* Defensive: sfx pointers reach this function via channels[].sfx,
     * which can be corrupted by a heap stomp from unrelated
     * code.  Bail before dereferencing s->cache or s->name if
     * s isn't a valid known_sfx[] slot. */
    if (!S_ValidSfx(s))
	return NULL;

    /* see if still in memory */
    sc = (sfxcache_t*)Cache_Check(&s->cache);
    if (sc)
	return sc;

    /* load it in */
    strlcpy(namebuffer, "sound/", sizeof(namebuffer));
    strlcat(namebuffer, s->name, sizeof(namebuffer));

    data = (byte*)COM_LoadStackFile(namebuffer, stackbuf, sizeof(stackbuf), NULL);

    if (!data) {
	Con_Printf("Couldn't load %s\n", namebuffer);
	return NULL;
    }

    info = GetWavinfo(s->name, data, com_filesize);
    /* GetWavinfo sets info->width = 0 to signal a malformed
     * file (truncated header, bad sample format, OOB data
     * chunk extent etc.).  Don't try to ResampleSfx in that
     * case -- info->dataofs / info->samples may be unset or
     * unsafe. */
    if (info->width == 0)
	return NULL;
    if (info->channels != 1) {
	Con_Printf("%s is a stereo sample\n", s->name);
	return NULL;
    }

    stepscale = (float)info->rate / shm->speed;
    len = info->samples / stepscale;

    /* cache is always allocated as 16-bit signed mono regardless of
     * input width — see ResampleSfx. */
    len = len * 2;

    sc = (sfxcache_t*)Cache_Alloc(&s->cache, len + sizeof(sfxcache_t));
    if (!sc)
	return NULL;

    sc->length = info->samples;
    sc->loopstart = info->loopstart;
    /* sc->speed/width/stereo are written by ResampleSfx using the
     * output (cache) format, not the input format. */

    ResampleSfx(s, info->rate, info->width, data + info->dataofs);

    return sc;
}



/*
===============================================================================

WAV loading

===============================================================================
*/


static const byte *data_p;
static const byte *iff_end;
static const byte *last_chunk;
static const byte *iff_data;
static int iff_chunk_len;


static short GetLittleShort(void)
{
    short val = 0;

    val = *data_p;
    val = val + (*(data_p + 1) << 8);
    data_p += 2;
    return val;
}

static int GetLittleLong(void)
{
    int val = 0;

    val = *data_p;
    val = val + (*(data_p + 1) << 8);
    val = val + (*(data_p + 2) << 16);
    val = val + (*(data_p + 3) << 24);
    data_p += 4;
    return val;
}

static void
FindNextChunk(const char *name, const char *filename)
{
   while (1)
   {
      /* Need at least 8 bytes for a chunk */
      if (last_chunk + 8 >= iff_end)
      {
         data_p = NULL;
         return;
      }

      data_p = last_chunk + 4;
      iff_chunk_len = GetLittleLong();
      if (iff_chunk_len < 0 || iff_chunk_len > iff_end - data_p) {
         Con_DPrintf("Bad \"%s\" chunk length (%d) in wav file %s\n",
               name, iff_chunk_len, filename);
         data_p = NULL;
         return;
      }
      last_chunk = data_p + ((iff_chunk_len + 1) & ~1);
      data_p -= 8;
      if (!strncmp((const char *)data_p, name, 4))
         return;
   }
}

static void
FindChunk(const char *name, const char *filename)
{
    last_chunk = iff_data;
    FindNextChunk(name, filename);
}

/*
============
GetWavinfo
============
*/

wavinfo_t *GetWavinfo (const char *name, byte *wav, int wavlength)
{
   static wavinfo_t info;
   int format;
   int samples;

   memset(&info, 0, sizeof(info));

   if (!wav)
      return &info;

   iff_data = wav;
   iff_end  = wav + wavlength;

   /* find "RIFF" chunk */
   FindChunk("RIFF", name);
   /* RIFF chunk header is 8 bytes (id + length); the form type
    * "WAVE" occupies the next 4 bytes (offset 8..11 from chunk
    * start).  Need 12 bytes total before strncmp can safely
    * read at data_p + 8. */
   if (!(data_p && data_p + 12 <= iff_end
         && !strncmp((char *)data_p + 8, "WAVE", 4)))
   {
      Con_Printf("Missing RIFF/WAVE chunks\n");
      return &info;
   }
   /* get "fmt " chunk */
   iff_data = data_p + 12;
   /* DumpChunks (); */

   FindChunk("fmt ", name);
   if (!data_p) {
      Con_Printf("Missing fmt chunk\n");
      return &info;
   }
   /* fmt chunk: skip 8 byte chunk header, then need at least
    * 14 bytes of payload (format + channels + rate + 4 bytes
    * skipped + 2 bytes for bits-per-sample). */
   if (data_p + 8 + 14 > iff_end) {
      Con_Printf("Truncated fmt chunk in %s\n", name);
      return &info;
   }
   data_p += 8;
   format = GetLittleShort();
   if (format != 1) {
      Con_Printf("Microsoft PCM format only\n");
      return &info;
   }

   info.channels = GetLittleShort();
   info.rate = GetLittleLong();
   data_p += 4 + 2;
   info.width = GetLittleShort() / 8;

   /* PCM bits-per-sample is conventionally 8 or 16, giving
    * width 1 or 2.  A 0 width would cause a divide-by-zero
    * below at samples = data_size / width; a huge width has
    * no defined meaning and would produce zero samples for
    * any reasonable data chunk.  Reject either case. */
   if (info.width != 1 && info.width != 2) {
      Con_Printf("%s: bad sample width %d (expected 1 or 2)\n",
                 name, info.width);
      info.width = 0;	/* mark invalid */
      return &info;
   }
   if (info.channels < 1 || info.channels > 2) {
      Con_Printf("%s: bad channel count %d\n", name, info.channels);
      info.width = 0;
      return &info;
   }
   if (info.rate <= 0) {
      Con_Printf("%s: bad sample rate %d\n", name, info.rate);
      info.width = 0;
      return &info;
   }

   /* get cue chunk */
   FindChunk("cue ", name);
   if (data_p)
   {
      /* The cue chunk parser below walks 32 bytes ahead and
       * then optionally another 24+ for the LIST mark.  Bound
       * check before each step. */
      if (data_p + 32 + 4 > iff_end) {
         info.loopstart = -1;
      } else {
         data_p += 32;
         info.loopstart = GetLittleLong();

         /* if the next chunk is a LIST chunk, look for a cue length marker */
         FindNextChunk("LIST", name);
         if (data_p)
         {
            /* this is not a proper parse, but it works with cooledit... */
            if (data_p + 32 <= iff_end
                && !strncmp((char *)data_p + 28, "mark", 4))
            {
               int i;
               data_p += 24;
               i = GetLittleLong();	/* samples in loop */
               info.samples = info.loopstart + i;
            }
         }
      }
   } else
      info.loopstart = -1;

   /* find data chunk */
   FindChunk("data", name);
   if (!data_p)
   {
      Con_Printf("Missing data chunk\n");
      return &info;
   }

   /* data chunk: skip 4-byte ID, read 4-byte length. */
   if (data_p + 8 > iff_end) {
      Con_Printf("Truncated data chunk in %s\n", name);
      info.width = 0;
      return &info;
   }
   data_p += 4;
   samples = GetLittleLong() / info.width;

   if (info.samples)
   {
      if (samples < info.samples)
         Sys_Error("Sound %s has a bad loop length", name);
   }
   else
      info.samples = samples;

   /* The data chunk's payload starts at data_p and is
    * info.samples * info.width bytes.  ResampleSfx and any
    * looping playback walk that range; require it to lie
    * fully within the file allocation. */
   if (info.samples < 0 || info.samples > (iff_end - data_p) / info.width) {
      Con_Printf("%s: data chunk samples (%d) extend past file end\n",
                 name, info.samples);
      info.width = 0;
      return &info;
   }

   info.dataofs = data_p - wav;

   return &info;
}
