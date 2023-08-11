#include "rescap.h"

#define FLT_RC_LOWPASS		0
#define FLT_RC_HIGHPASS		1
#define FLT_RC_AC			2
#define FLT_RC_PANNEDLEFT   4
#define FLT_RC_PANNEDRIGHT  8

void filter_rc_update(INT32 num, INT16 *src, INT16 *pSoundBuf, INT32 length);
void filter_rc_set_RC(INT32 num, INT32 type, double R1, double R2, double R3, double C);
void filter_rc_init(INT32 num, INT32 type, double R1, double R2, double R3, double C, INT32 add_signal);
void filter_rc_set_src_gain(INT32 num, double gain);
void filter_rc_set_src_stereo(INT32 num);
void filter_rc_set_route(INT32 num, double nVolume, INT32 nRouteDir);
void filter_rc_set_limit(INT32 num, double nVolume);
void filter_rc_exit();
