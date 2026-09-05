
/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2010-2011 O. Sezer <sezero@users.sourceforge.net>
Copyright (C) 2010-2014 QuakeSpasm developers

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
/* snd_mix.c -- portable code to mix sounds for snd_dma.c */

#include "common.h"
#include "console.h"
#include "quakedef.h"
#include "sound.h"

/* SIMD vector paths for the no-gather inner loops below.
 *
 * SMIX_SSE2:         enable SSE2 path (x86-64 baseline, opt-in x86)
 * SMIX_NEON:         enable NEON path (AArch64 baseline, opt-in ARMv7)
 * SMIX_NEON_AARCH64: enable NEON paths that win ONLY on AArch64
 *                    (ARMv7 falls back to gcc auto-vectorized scalar)
 *
 * The AArch64-only gate exists for the per-channel paint loop
 * (SND_PaintChannelFrom16): on AArch64 the manual NEON version
 * uses ldp/stp pairs and beats gcc's auto-vec (which uses ld2/st2)
 * by ~3.15x.  On ARMv7 gcc auto-vec already emits VMLA + VLD2/VST2
 * matching our manual code; the manual version measures ~0.9x
 * vs auto-vec (small register-pressure regression on 32-bit ARM
 * with only 16 q-registers), so ARMv7 stays on the scalar path
 * and gets the gcc auto-vec for free.  The blast/saturate loop
 * does win on both ISAs, so it uses the broader SMIX_NEON gate. */
#if defined(__SSE2__) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2) || defined(_M_X64)
#define SMIX_SSE2 1
#include <emmintrin.h>
#endif
#if defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(_M_ARM64)
#define SMIX_NEON 1
#include <arm_neon.h>
#endif
#if defined(__aarch64__) || defined(_M_ARM64)
#define SMIX_NEON_AARCH64 1
#endif

#define	CLAMP(_minval, x, _maxval)		\
	((x) < (_minval) ? (_minval) :		\
	 (x) > (_maxval) ? (_maxval) : (x))

#define	PAINTBUFFER_SIZE	16384
static portable_samplepair_t paintbuffer[PAINTBUFFER_SIZE];
static int *snd_p, snd_linear_count;
short *snd_out;

/* Float output path (see RETRO_ENVIRONMENT_GET_AUDIO_SAMPLE_BATCH_FLOAT in
 * the libretro layer).  When s_float_output is set the paintbuffer is
 * transferred as normalized float [-1,1] into snd_float_buffer instead of the
 * int16 shm->buffer; both point at libretro-layer buffers.  snd_out_f mirrors
 * snd_out for the float blast.  These stay 0/NULL on any frontend that does
 * not negotiate float output, so the deterministic int16 path is unchanged. */
int    s_float_output   = 0;
float *snd_float_buffer = NULL;
static float *snd_out_f;

static int snd_vol;

static void Snd_WriteLinearBlastStereo16 (void)
{
	int		i = 0;
	int		val;

	/* The body is: out[i] = clamp(in[i] >> 8, INT16_MIN, INT16_MAX).
	 * This is a shift-then-saturate-narrow, which both SSE2 and
	 * NEON have as a single packed instruction (PACKSSDW / VQSHRN).
	 * gcc -O3 auto-vectorizes the scalar loop using compare-and-
	 * select on all three ISAs but does NOT recognize the
	 * saturating-narrow primitive, so the manual SIMD wins
	 * everywhere:
	 *   x86_64 SSE2  : 2.55x vs auto-vec scalar
	 *   AArch64 NEON : 1.17x
	 *   ARMv7   NEON : 1.70x
	 * Body: load 8 int32s, arithmetic-shift right 8, saturate-pack
	 * to 8 int16s, store. */
#if defined(SMIX_SSE2)
	if (snd_linear_count >= 8) {
		for (; i + 8 <= snd_linear_count; i += 8) {
			__m128i a = _mm_loadu_si128((const __m128i *)&snd_p[i]);
			__m128i b = _mm_loadu_si128((const __m128i *)&snd_p[i+4]);
			a = _mm_srai_epi32(a, 8);
			b = _mm_srai_epi32(b, 8);
			_mm_storeu_si128((__m128i *)&snd_out[i],
			                 _mm_packs_epi32(a, b));
		}
	}
#elif defined(SMIX_NEON)
	if (snd_linear_count >= 8) {
		for (; i + 8 <= snd_linear_count; i += 8) {
			int32x4_t a = vld1q_s32(&snd_p[i]);
			int32x4_t b = vld1q_s32(&snd_p[i+4]);
			int16x4_t la = vqshrn_n_s32(a, 8);
			int16x4_t lb = vqshrn_n_s32(b, 8);
			vst1q_s16(&snd_out[i], vcombine_s16(la, lb));
		}
	}
#endif

	/* Scalar tail (and full-loop fallback when no SIMD is built). */
	for (; i < snd_linear_count; i += 2)
	{
		val = snd_p[i] >> 8;
		if (val > 0x7fff)
			snd_out[i] = 0x7fff;
		else if (val < (short)0x8000)
			snd_out[i] = (short)0x8000;
		else
			snd_out[i] = val;

		val = snd_p[i+1] >> 8;
		if (val > 0x7fff)
			snd_out[i+1] = 0x7fff;
		else if (val < (short)0x8000)
			snd_out[i+1] = (short)0x8000;
		else
			snd_out[i+1] = val;
	}
}

/* Linearly transfer the contents of paintbuffer[0..count) into
 * the output destination buffer at dst_offset (in stereo
 * frames).  The destination is shm->buffer, which the libretro
 * layer points at a linear int16 buffer sized for one video
 * frame's audio (plus PAINTBUFFER_SIZE headroom for the
 * worst-case low-framerate case).  No ring math, no
 * wraparound.  dst_offset advances across the chunked outer
 * loop in S_PaintChannels so consecutive chunks land at
 * consecutive shm->buffer positions. */
static void S_TransferStereo16 (int dst_offset, int count)
{
	snd_p            = (int*)paintbuffer;
	snd_out          = (short *)shm->buffer + (dst_offset << 1);
	snd_linear_count = count << 1;
	Snd_WriteLinearBlastStereo16();
}

/* Float counterpart of Snd_WriteLinearBlastStereo16.  The paintbuffer
 * accumulator carries 8 fractional bits (the int16 blast does snd_p[i] >> 8),
 * so normalizing by 256*32768 keeps that sub-int16 precision instead of
 * discarding it.  tyrquake's int16 blast hard-saturates (clamp to
 * 0x8000..0x7fff) rather than soft-clipping, so the float blast hard-clamps to
 * [-1,1] to keep float and int16 output tonally identical: the float path only
 * removes the int16 quantization (and the frontend's int16->float widen), it
 * does not reshape the signal. */
#define SND_FLOAT_NORM (1.0f / 8388608.0f)	/* 1 / (256 * 32768) */

static void Snd_WriteLinearBlastStereoFloat (void)
{
	int   i;
	float f;

	for (i = 0; i < snd_linear_count; i++)
	{
		f = (float)snd_p[i] * SND_FLOAT_NORM;
		if (f > 1.0f)
			f = 1.0f;
		else if (f < -1.0f)
			f = -1.0f;
		snd_out_f[i] = f;
	}
}

/* Float counterpart of S_TransferStereo16.  Writes into snd_float_buffer (a
 * libretro-layer float buffer) at the same linear dst_offset the int16 path
 * uses for shm->buffer.  Only reached when float output was negotiated. */
static void S_TransferStereoFloat (int dst_offset, int count)
{
	snd_p            = (int*)paintbuffer;
	snd_out_f        = snd_float_buffer + (dst_offset << 1);
	snd_linear_count = count << 1;
	Snd_WriteLinearBlastStereoFloat();
}

/*
===============================================================================

CHANNEL MIXING

===============================================================================
*/

static void SND_PaintChannelFrom16 (channel_t *ch, sfxcache_t *sc, int count, int paintbufferstart);

void S_PaintChannels (int samples_to_paint)
{
	int		i;
	int		chunk, ltime, count;
	int		painted = 0;	/* samples emitted so far this call */
	channel_t	*ch;
	sfxcache_t	*sc;
	float		vol;

	/* sfxvolume is a user cvar.  NaN propagates to snd_vol
	 * via (int)(NaN * 256), which is UB; on x86 typically
	 * INT_MIN.  Then `data * leftvol` with leftvol ~ INT_MIN/256
	 * overflows the int multiplication used at line 238.  Clamp
	 * the computed product before the cast. */
	vol = sfxvolume.value;
	if (IS_NAN(vol) || vol < 0.0f)
		vol = 0.0f;
	else if (vol > 1.0f)
		vol = 1.0f;
	snd_vol = (int)(vol * 256);

	while (painted < samples_to_paint)
	{
		/* The paintbuffer is a fixed PAINTBUFFER_SIZE scratch
		 * buffer; chunk the per-frame work down to fit.  At
		 * normal framerates one iteration covers the whole
		 * frame; the loop only runs more than once when
		 * samples_per_frame exceeds PAINTBUFFER_SIZE
		 * (degenerate low-framerate paths). */
		chunk = samples_to_paint - painted;
		if (chunk > PAINTBUFFER_SIZE)
			chunk = PAINTBUFFER_SIZE;

		/* clear the paint buffer */
		memset(paintbuffer, 0, chunk * sizeof(portable_samplepair_t));

		/* paint in the channels. */
		ch = channels;
		for (i = 0; i < total_channels; i++, ch++)
		{
			if (!ch->sfx)
				continue;
			/* Defensive: if a channel's sfx pointer is non-NULL
			 * but doesn't point into known_sfx[], a heap stomp
			 * has corrupted it.  Clear and skip rather than
			 * crashing in S_LoadSound/Cache_Check. */
			if (!S_ValidSfx(ch->sfx)) {
				ch->sfx = NULL;
				continue;
			}
			if (!ch->leftvol && !ch->rightvol)
				continue;
			sc = S_LoadSound (ch->sfx);
			if (!sc)
				continue;

			ltime = 0;	/* offset within this chunk */

			while (ltime < chunk)
			{	/* paint up to chunk */
				if (ch->remaining_samples < chunk - ltime)
					count = ch->remaining_samples;
				else
					count = chunk - ltime;

				if (count > 0)
				{
					/* the last param to SND_PaintChannelFrom is the index */
					/* to start painting to in the paintbuffer, usually 0. */
					SND_PaintChannelFrom16(ch, sc, count, ltime);

					ltime += count;
					ch->remaining_samples -= count;
				}

				/* if at end of loop, restart */
				if (ch->remaining_samples <= 0)
				{
					if (sc->loopstart >= 0)
					{
						ch->pos = sc->loopstart;
						ch->remaining_samples = sc->length - ch->pos;
						/* Defensive: a malformed sfxcache_t with
						 * loopstart >= length would yield a non-
						 * positive remaining_samples and re-enter
						 * the loop forever.  Drop the channel. */
						if (ch->remaining_samples <= 0)
						{
							ch->sfx = NULL;
							break;
						}
					}
					else
					{	/* channel just stopped */
						ch->sfx = NULL;
						break;
					}
				}
			}
		}

		/* Hard-limit the SFX mix to 0 dBFS (int16 full scale).  Note: the
		 * upstream QuakeII/QuakeSpasm lineage these comments came from
		 * reduced by a further 6 dB here and relied on a smoothing lowpass
		 * to mask the clipping -- this mixer does neither, so the clamp is a
		 * straight hard clip at 0 dBFS.  A single sound peaks just under the
		 * ceiling (leftvol capped at 255 in SND_PaintChannelFrom16), but
		 * dense simultaneous effects, and effects plus the music summed
		 * below, can still clip here, the same as stock Quake. */
		for (i = 0; i < chunk; i++)
		{
			paintbuffer[i].left = CLAMP(-(32768 << 8), paintbuffer[i].left, 32767 << 8);
			paintbuffer[i].right = CLAMP(-(32768 << 8), paintbuffer[i].right, 32767 << 8);
		}

		if (s_float_output && snd_float_buffer)
		{
			/* Float lane: transfer the (clipped) SFX paintbuffer to the float
			 * output buffer as normalized [-1,1], then sum the float music
			 * FIFO on top and re-clamp.  This keeps the SFX mix integer and
			 * deterministic while the music rides the float codec lane. */
			float *fb = snd_float_buffer + (painted << 1);

			S_TransferStereoFloat(painted, chunk);

			if (s_rawavail > 0)
			{
				int bgm_count = (s_rawavail < chunk) ? s_rawavail : chunk;

				for (i = 0; i < bgm_count; i++)
				{
					int   s = (s_rawhead + i) & (MAX_RAW_SAMPLES - 1);
					float l = fb[i * 2]     + s_rawsamples_f[s].left;
					float r = fb[i * 2 + 1] + s_rawsamples_f[s].right;
					if (l >  1.0f) l =  1.0f; else if (l < -1.0f) l = -1.0f;
					if (r >  1.0f) r =  1.0f; else if (r < -1.0f) r = -1.0f;
					fb[i * 2]     = l;
					fb[i * 2 + 1] = r;
				}

				s_rawhead   = (s_rawhead + bgm_count) & (MAX_RAW_SAMPLES - 1);
				s_rawavail -= bgm_count;
			}
		}
		else
		{
			/* Int lane (unchanged): add the int music FIFO into the paintbuffer,
			 * then transfer to the int16 output buffer. */
			if (s_rawavail > 0)
			{
				/* copy from the streaming sound source */
				int bgm_count = (s_rawavail < chunk) ? s_rawavail : chunk;

				for (i = 0; i < bgm_count; i++)
				{
					int s = (s_rawhead + i) & (MAX_RAW_SAMPLES - 1);
					/* Summed at unity onto the already-clamped SFX; the
					 * final transfer clamps the total, so a loud SFX bed
					 * plus loud music clips at 0 dBFS.  (The "6 dB" the
					 * upstream comment mentioned is not applied here.) */
					paintbuffer[i].left  += s_rawsamples[s].left;
					paintbuffer[i].right += s_rawsamples[s].right;
				}

				s_rawhead   = (s_rawhead + bgm_count) & (MAX_RAW_SAMPLES - 1);
				s_rawavail -= bgm_count;
			}

			S_TransferStereo16(painted, chunk);
		}
		painted += chunk;
	}
}

static void SND_PaintChannelFrom16 (channel_t *ch, sfxcache_t *sc, int count, int paintbufferstart)
{
	int i;
	int leftvol;
	int rightvol;
	signed short *sfx;
	portable_samplepair_t *pb;

	/* SND_Spatialize can produce per-channel volumes up to
	 * roughly 2 * master_vol — its (1 - dist) * rscale factor
	 * tops out near 2.0 for a sound right beside the listener
	 * with master_vol=255 — i.e. ~510. The retired
	 * SND_PaintChannelFrom8 path capped these at 255 explicitly
	 * (snd_scaletable was sized [32][256], so anything above
	 * 255 indexed past the end). The 16-bit path never had
	 * that cap, so once round-12 unified all painting through
	 * here, close full-volume sounds got multiplied by ~2x the
	 * intended scale and saturated the per-sample CLAMP in
	 * S_PaintChannels, audibly clipping. Apply the same cap
	 * here. (Matches QuakeSpasm patch #23,
	 * https://sourceforge.net/p/quakespasm/patches/23/.) */
	if (ch->leftvol  > 255) ch->leftvol  = 255;
	if (ch->rightvol > 255) ch->rightvol = 255;

	leftvol  = (ch->leftvol  * snd_vol) >> 8;
	rightvol = (ch->rightvol * snd_vol) >> 8;
	sfx      = (signed short *)sc->data + ch->pos;
	pb       = &paintbuffer[paintbufferstart];

	i = 0;

	/* SIMD MAC kernel: 8 samples per iter.  Read 8 int16, widen
	 * to two int32x4 lanes, multiply each by leftvol and rightvol
	 * (broadcast scalars), zip into (l,r) stereo layout, add into
	 * paintbuffer.
	 *
	 * Gated to SSE2 and AArch64 NEON only.  On ARMv7 gcc's auto-
	 * vectorizer already emits VLD2/VST2 + VMLA matching the
	 * intrinsic version, and manual NEON measures ~0.91x vs
	 * auto-vec there (register pressure with 16 q-regs); leaving
	 * the scalar loop intact lets gcc auto-vec do its job.
	 *
	 * Measured:
	 *   x86_64 SSE2  : 1.58x vs auto-vec scalar
	 *   AArch64 NEON : 3.15x (beats auto-vec's ld2/st2 by using
	 *                  ldp/stp pairs instead) */
#if defined(SMIX_SSE2)
	if (count >= 8) {
		__m128i vl = _mm_set1_epi16((int16_t)leftvol);
		__m128i vr = _mm_set1_epi16((int16_t)rightvol);
		for (; i + 8 <= count; i += 8) {
			__m128i s  = _mm_loadu_si128((const __m128i *)&sfx[i]);
			__m128i ll = _mm_mullo_epi16(s, vl);
			__m128i lh = _mm_mulhi_epi16(s, vl);
			__m128i rl = _mm_mullo_epi16(s, vr);
			__m128i rh = _mm_mulhi_epi16(s, vr);
			/* PMULLW gives low 16 of int16*int16, PMULHW high 16;
			 * PUNPCK combines to int32 per lane. */
			__m128i Llo = _mm_unpacklo_epi16(ll, lh);
			__m128i Lhi = _mm_unpackhi_epi16(ll, lh);
			__m128i Rlo = _mm_unpacklo_epi16(rl, rh);
			__m128i Rhi = _mm_unpackhi_epi16(rl, rh);
			/* Interleave into stereo (l,r,l,r) layout. */
			__m128i z0 = _mm_unpacklo_epi32(Llo, Rlo);
			__m128i z1 = _mm_unpackhi_epi32(Llo, Rlo);
			__m128i z2 = _mm_unpacklo_epi32(Lhi, Rhi);
			__m128i z3 = _mm_unpackhi_epi32(Lhi, Rhi);
			__m128i p0 = _mm_loadu_si128((const __m128i *)&pb[i].left);
			__m128i p1 = _mm_loadu_si128((const __m128i *)&pb[i+2].left);
			__m128i p2 = _mm_loadu_si128((const __m128i *)&pb[i+4].left);
			__m128i p3 = _mm_loadu_si128((const __m128i *)&pb[i+6].left);
			_mm_storeu_si128((__m128i *)&pb[i].left,
			                 _mm_add_epi32(p0, z0));
			_mm_storeu_si128((__m128i *)&pb[i+2].left,
			                 _mm_add_epi32(p1, z1));
			_mm_storeu_si128((__m128i *)&pb[i+4].left,
			                 _mm_add_epi32(p2, z2));
			_mm_storeu_si128((__m128i *)&pb[i+6].left,
			                 _mm_add_epi32(p3, z3));
		}
	}
#elif defined(SMIX_NEON_AARCH64)
	if (count >= 8) {
		for (; i + 8 <= count; i += 8) {
			int16x8_t s    = vld1q_s16(&sfx[i]);
			int32x4_t s_lo = vmovl_s16(vget_low_s16(s));
			int32x4_t s_hi = vmovl_s16(vget_high_s16(s));
			int32x4_t l_lo = vmulq_n_s32(s_lo, leftvol);
			int32x4_t l_hi = vmulq_n_s32(s_hi, leftvol);
			int32x4_t r_lo = vmulq_n_s32(s_lo, rightvol);
			int32x4_t r_hi = vmulq_n_s32(s_hi, rightvol);
			int32x4_t z0 = vzip1q_s32(l_lo, r_lo);
			int32x4_t z1 = vzip2q_s32(l_lo, r_lo);
			int32x4_t z2 = vzip1q_s32(l_hi, r_hi);
			int32x4_t z3 = vzip2q_s32(l_hi, r_hi);
			int32x4_t p0 = vld1q_s32(&pb[i].left);
			int32x4_t p1 = vld1q_s32(&pb[i+2].left);
			int32x4_t p2 = vld1q_s32(&pb[i+4].left);
			int32x4_t p3 = vld1q_s32(&pb[i+6].left);
			vst1q_s32(&pb[i].left,   vaddq_s32(p0, z0));
			vst1q_s32(&pb[i+2].left, vaddq_s32(p1, z1));
			vst1q_s32(&pb[i+4].left, vaddq_s32(p2, z2));
			vst1q_s32(&pb[i+6].left, vaddq_s32(p3, z3));
		}
	}
#endif

	/* Scalar tail (and full-loop fallback for ARMv7 / no-SIMD
	 * builds; gcc auto-vec turns this into a NEON/SSE2 MAC loop
	 * by itself when it's the only path). */
	for (; i < count; i++)
	{
		int data  = sfx[i];
		int left  = data * leftvol;
		int right = data * rightvol;
		pb[i].left  += left;
		pb[i].right += right;
	}

	ch->pos += count;
}

