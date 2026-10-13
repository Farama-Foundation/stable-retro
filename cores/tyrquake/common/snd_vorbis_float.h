/*
 * snd_vorbis_float.h
 *
 * Opaque wrappers around the vendored, symbol-namespaced float libvorbis
 * (deps/libvorbis), used by snd_vorbis.c only when float audio output was
 * negotiated; the default int16 path keeps using Tremor.
 *
 * No libvorbis types appear here, so snd_vorbis.c can call these without
 * including <vorbis/...> (whose ov_* and vorbis_* symbols collide with Tremor).
 * The handle and the file handle are opaque void *.
 */

#ifndef SND_VORBIS_FLOAT_H
#define SND_VORBIS_FLOAT_H

/* fh is a fshandle_t * (opaque here).  On success returns an opaque decode
 * handle and writes the stream rate/channels; returns NULL on failure. */
void *FVorbis_Open(void *fh, const char *name, int *out_rate, int *out_channels);

/* Decode up to max_floats interleaved float samples ([-1,1]) into out.
 * Returns floats written, 0 at EOF, or a negative error. */
int FVorbis_ReadFloat(void *handle, float *out, int max_floats);

/* Seek to start; returns 0 on success. */
int FVorbis_Rewind(void *handle);

/* Close and free the handle. */
void FVorbis_Close(void *handle);

#endif /* SND_VORBIS_FLOAT_H */
