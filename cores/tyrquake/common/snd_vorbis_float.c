/*
 * snd_vorbis_float.c
 *
 * Float Vorbis decode path, used when float audio output was negotiated
 * (RETRO_ENVIRONMENT_GET_AUDIO_SAMPLE_BATCH_FLOAT).  Decodes straight to
 * 32-bit float via libvorbis' ov_read_float, preserving the full Vorbis
 * precision that the default fixed-point Tremor int16 path truncates to 16
 * bits.  The int16 output path is unchanged and keeps using Tremor.
 *
 * The vendored libvorbis (deps/libvorbis) is symbol-namespaced to fv_* (see
 * fvorbis_rename.h, force-included into its TUs) so it links alongside Tremor,
 * which exports the same ov_* and vorbis_* names.  This wrapper calls into it, so
 * the handful of ov_* entry points used below are #defined to their fv_*
 * names just before the libvorbis header is pulled in -- after the engine
 * headers, so no engine symbol is affected and libvorbis' generic-named
 * internal headers never reach this TU's include path.
 *
 * vorbisfile.h is included by a relative path rather than <vorbis/vorbisfile.h>
 * so this TU needs no -I of its own: it pulls in vorbisfile.h directly, which
 * includes "codec.h" relatively, which needs only <ogg/ogg.h> (already on the
 * path for Tremor/libogg).  This keeps the ndk-build (which doesn't apply the
 * per-file include flags from Makefile.common) able to find the header.
 */

#include "quakedef.h"
#include "common.h"
#include "console.h"

#include "snd_vorbis_float.h"

#define ov_open_callbacks	fv_ov_open_callbacks
#define ov_seekable		fv_ov_seekable
#define ov_streams		fv_ov_streams
#define ov_info			fv_ov_info
#define ov_read_float		fv_ov_read_float
#define ov_time_seek		fv_ov_time_seek
#define ov_clear		fv_ov_clear
/* vorbisfile.h defines several unused static OV_CALLBACKS_* convenience
 * structs at file scope; suppress the unused-variable noise for the header
 * only (this TU stays at full -Wall otherwise).  GCC/Clang-only; MSVC, which
 * does not warn on unused file-scope statics, skips the pragma. */
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif
#include "../deps/libvorbis/include/vorbis/vorbisfile.h"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

/* I/O callbacks driven by the engine's FS_* helpers (same shape as the Tremor
 * callbacks in snd_vorbis.c).  ov_callbacks is a plain struct type, unaffected
 * by the symbol rename. */
static int fovc_fclose(void *f)
{
	(void)f;
	return 0; /* the stream's file handle is closed by S_CodecUtilClose */
}

static int fovc_fseek(void *f, ogg_int64_t off, int whence)
{
	if (f == NULL)
		return -1;
	return FS_fseek((fshandle_t *)f, (long)off, whence);
}

static ov_callbacks fovc_qfs =
{
	(size_t (*)(void *, size_t, size_t, void *))	FS_fread,
	(int (*)(void *, ogg_int64_t, int))		fovc_fseek,
	(int (*)(void *))				fovc_fclose,
	(long (*)(void *))				FS_ftell
};

void *FVorbis_Open(void *fhv, const char *name, int *out_rate, int *out_channels)
{
	fshandle_t     *fh = (fshandle_t *)fhv;
	OggVorbis_File *vf;
	vorbis_info    *vi;
	int             res;

	vf = (OggVorbis_File *)malloc(sizeof(OggVorbis_File));
	if (!vf)
		return NULL;

	res = ov_open_callbacks(fh, vf, NULL, 0, fovc_qfs);
	if (res != 0)
	{
		Con_Printf("%s is not a valid Ogg Vorbis file (error %i).\n", name, res);
		free(vf);
		return NULL;
	}

	if (!ov_seekable(vf))
	{
		Con_Printf("Stream %s not seekable.\n", name);
		ov_clear(vf);
		free(vf);
		return NULL;
	}

	if (ov_streams(vf) != 1)
	{
		Con_Printf("More than one stream in %s.\n", name);
		ov_clear(vf);
		free(vf);
		return NULL;
	}

	vi = ov_info(vf, 0);
	if (!vi || (vi->channels != 1 && vi->channels != 2))
	{
		Con_Printf("Unsupported Ogg Vorbis stream %s.\n", name);
		ov_clear(vf);
		free(vf);
		return NULL;
	}

	*out_rate     = vi->rate;
	*out_channels = vi->channels;
	return vf;
}

int FVorbis_ReadFloat(void *handle, float *out, int max_floats)
{
	OggVorbis_File *vf = (OggVorbis_File *)handle;
	vorbis_info    *vi;
	int             section;
	int             channels;
	int             total = 0;

	vi = ov_info(vf, -1);
	if (!vi)
		return -1;
	channels = vi->channels;
	if (channels < 1)
		return -1;

	/* ov_read_float hands back per-channel (non-interleaved) float buffers
	 * already in [-1,1]; interleave them into the caller's buffer. */
	while (total + channels <= max_floats)
	{
		float **pcm;
		long    got = ov_read_float(vf, &pcm,
				(max_floats - total) / channels, &section);

		if (got == 0)
			break;			/* EOF */
		if (got < 0)
			return total ? total : (int)got;

		{
			int i, c;
			for (i = 0; i < got; i++)
				for (c = 0; c < channels; c++)
					out[total++] = pcm[c][i];
		}
	}

	return total;
}

int FVorbis_Rewind(void *handle)
{
	/* libvorbis ov_time_seek takes seconds as a double (Tremor takes
	 * milliseconds as int64, handled on the Tremor side). */
	return ov_time_seek((OggVorbis_File *)handle, 0.0);
}

void FVorbis_Close(void *handle)
{
	OggVorbis_File *vf = (OggVorbis_File *)handle;
	if (!vf)
		return;
	ov_clear(vf);
	free(vf);
}
