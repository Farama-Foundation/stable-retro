/* decode_stubs.c -- encode-only symbol stubs for the decode-only libvorbis
 * vendoring used by the float audio output path.
 *
 * libvorbis shares vorbis_dsp_clear / _vds_shared_init / vorbis_info_clear and
 * the mapping0/registry export bundles between encode and decode.  Those
 * functions *reference* the encode machinery (envelope.c / psy.c / smallft.c),
 * but every such reference sits behind an encode-only guard: _vds_shared_init
 * runs the FFT/psy/envelope init only under `if(encp)` (decode calls it with
 * encp==0), vorbis_dsp_clear frees them only `if(b->ve)` / `if(b->psy)` (NULL
 * for a decode state), vorbis_info_clear frees psy params only `if(ci->psys)`
 * (0 for decode), and mapping0_forward is never dispatched on the decode path.
 * So these symbols must resolve at link but are never executed when decoding;
 * stubbing them keeps the encoder (and its large masking/setup tables) out of
 * the binary.  Signatures match the internal headers so the TU is C89-clean.
 */
#include "vorbis/codec.h"
#include "codec_internal.h"

void _ve_envelope_init(envelope_lookup *e,vorbis_info *vi) {}
void _ve_envelope_clear(envelope_lookup *e) {}
void _ve_envelope_shift(envelope_lookup *e,long shift) {}
long _ve_envelope_search(vorbis_dsp_state *v) { return 0; }
int _ve_envelope_mark(vorbis_dsp_state *v) { return 0; }
void _vp_psy_init(vorbis_look_psy *p,vorbis_info_psy *vi, vorbis_info_psy_global *gi,int n,long rate) {}
void _vp_psy_clear(vorbis_look_psy *p) {}
void _vp_noisemask(vorbis_look_psy *p, float *logmdct, float *logmask) {}
void _vp_tonemask(vorbis_look_psy *p, float *logfft, float *logmask, float global_specmax, float local_specmax) {}
void _vp_offset_and_mix(vorbis_look_psy *p, float *noise, float *tone, int offset_select, float *logmask, float *mdct, float *logmdct) {}
void _vp_couple_quantize_normalize(int blobno, vorbis_info_psy_global *g, vorbis_look_psy *p, vorbis_info_mapping0 *vi, float **mdct, int **iwork, int *nonzero, int sliding_lowpass, int ch) {}
vorbis_look_psy_global *_vp_global_look(vorbis_info *vi) { return 0; }
void _vp_global_free(vorbis_look_psy_global *look) {}
float _vp_ampmax_decay(float amp,vorbis_dsp_state *vd) { return 0.0f; }
void _vi_psy_free(vorbis_info_psy *i) {}
void drft_init(drft_lookup *l,int n) {}
void drft_clear(drft_lookup *l) {}
void drft_forward(drft_lookup *l,float *data) {}
