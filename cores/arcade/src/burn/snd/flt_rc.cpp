// Based on MAME driver by Derrick Renaud, Couriersud

#include "burnint.h"
#include "flt_rc.h"

#include <math.h>

#define FLT_RC_NUM      16

struct flt_rc_info
{
	INT32 type;
	double R1;
	double R2;
	double R3;
	double C;

	struct {
		INT32 k;
		INT32 memory;
		INT32 type;
	} state;

	double src_gain;
	double gain;
	INT16 limit;
	INT32 src_stereo;
	INT32 output_dir;
	INT32 add_signal;
};

static struct flt_rc_info flt_rc_table[FLT_RC_NUM];

static INT32 num_filters;

#define FLT_CLIP(A) ((A) < -ptr->limit ? -ptr->limit : (A) > ptr->limit ? ptr->limit : (A))

void filter_rc_update(INT32 num, INT16 *src, INT16 *pSoundBuf, INT32 length)
{
#if defined FBNEO_DEBUG
	if (!DebugSnd_FilterRCInitted) bprintf(PRINT_ERROR, _T("filter_rc_update called without init\n"));
	if (num > num_filters) bprintf(PRINT_ERROR, _T("filter_rc_update called with invalid num %i\n"), num);
#endif

	struct flt_rc_info *ptr;

	ptr = &flt_rc_table[num];

	INT32 memory = ptr->state.memory;
	INT16 value;

	switch (ptr->state.type) {
		case FLT_RC_LOWPASS: {
			while (length--) {
				if (ptr->state.k == 0x10000) {
					memory = (INT32)(*src++ * ptr->src_gain); // filter disabled
				} else {
					memory += (((INT32)((*src++ * ptr->src_gain)) - memory) * ptr->state.k) / 0x10000; // enabled
				}

				if (ptr->src_stereo) src++;

				INT32 nLeftSample = 0, nRightSample = 0;

				if ((ptr->output_dir & BURN_SND_ROUTE_LEFT) == BURN_SND_ROUTE_LEFT) {
					nLeftSample += (INT32)(memory * ptr->gain);
				}

				if ((ptr->output_dir & BURN_SND_ROUTE_RIGHT) == BURN_SND_ROUTE_RIGHT) {
					nRightSample += (INT32)(memory * ptr->gain);
				}

				if (ptr->output_dir & (FLT_RC_PANNEDLEFT + FLT_RC_PANNEDRIGHT)) { // panned slightly right or left (used in gyruss)
					nLeftSample  += (INT32)(memory * ((ptr->output_dir & FLT_RC_PANNEDRIGHT) ? (ptr->gain / 3) : (ptr->gain)));
					nRightSample += (INT32)(memory * ((ptr->output_dir & FLT_RC_PANNEDLEFT ) ? (ptr->gain / 3) : (ptr->gain)));
				}

				nLeftSample = FLT_CLIP(nLeftSample);
				nRightSample = FLT_CLIP(nRightSample);

				if (ptr->add_signal) {
					pSoundBuf[0] = BURN_SND_CLIP(pSoundBuf[0] + nLeftSample);
					pSoundBuf[1] = BURN_SND_CLIP(pSoundBuf[1] + nRightSample);
				} else {
					pSoundBuf[0] = nLeftSample;
					pSoundBuf[1] = nRightSample;
				}
				pSoundBuf += 2;
			}
			break;
		}

		case FLT_RC_HIGHPASS:
		case FLT_RC_AC: {
			while (length--) {
				if (ptr->state.k == 0x0) {
					value = (INT32)(*src * ptr->src_gain); // filter disabled
				} else {
					value = (INT32)(*src * ptr->src_gain) - memory; // enabled
				}

				INT32 nLeftSample = 0, nRightSample = 0;

				if ((ptr->output_dir & BURN_SND_ROUTE_LEFT) == BURN_SND_ROUTE_LEFT) {
					nLeftSample += (INT32)(value * ptr->gain);
				}

				if ((ptr->output_dir & BURN_SND_ROUTE_RIGHT) == BURN_SND_ROUTE_RIGHT) {
					nRightSample += (INT32)(value * ptr->gain);
				}

				if (ptr->output_dir & (FLT_RC_PANNEDLEFT + FLT_RC_PANNEDRIGHT)) { // panned slightly right or left (used in gyruss)
					nLeftSample  += (INT32)(value * ((ptr->output_dir & FLT_RC_PANNEDRIGHT) ? (ptr->gain / 3) : (ptr->gain)));
					nRightSample += (INT32)(value * ((ptr->output_dir & FLT_RC_PANNEDLEFT ) ? (ptr->gain / 3) : (ptr->gain)));
				}

				nLeftSample = FLT_CLIP(nLeftSample);
				nRightSample = FLT_CLIP(nRightSample);

				if (ptr->add_signal) {
					pSoundBuf[0] = BURN_SND_CLIP(pSoundBuf[0] + nLeftSample);
					pSoundBuf[1] = BURN_SND_CLIP(pSoundBuf[1] + nRightSample);
				} else {
					pSoundBuf[0] = nLeftSample;
					pSoundBuf[1] = nRightSample;
				}
				pSoundBuf += 2;
				memory += (((INT32)(*src++ * ptr->src_gain) - memory) * ptr->state.k) / 0x10000;
				if (ptr->src_stereo) src++;
			}
			break;
		}
	}

	ptr->state.memory = memory;
}

static void set_RC_info(INT32 num, INT32 type, double R1, double R2, double R3, double C)
{
	double Req = 0.00;

	struct flt_rc_info *ptr;

	ptr = &flt_rc_table[num];

	ptr->state.type = type;

	switch (ptr->state.type) {
		case FLT_RC_LOWPASS: {
			if (C == 0.0) {
				/* filter disabled */
				ptr->state.k = 0x10000;
				return;
			}
			Req = (R1 * (R2 + R3)) / (R1 + R2 + R3);
			break;
		}

		case FLT_RC_HIGHPASS:
		case FLT_RC_AC: {
			if (C == 0.0) {
				/* filter disabled */
				ptr->state.k = 0x0;
				ptr->state.memory = 0x0;
				return;
			}
			Req = R1;
			break;
		}

		default:
			bprintf(PRINT_IMPORTANT, _T("filter_rc_setRC: Wrong filter type %d\n"), ptr->state.type);
	}

	/* Cut Frequency = 1/(2*Pi*Req*C) */
	/* k = (1-(EXP(-TIMEDELTA/RC)))    */
	ptr->state.k = (INT32)(0x10000 - 0x10000 * (exp(-1 / (Req * C) / nBurnSoundRate)));
}

void filter_rc_set_RC(INT32 num, INT32 type, double R1, double R2, double R3, double C)
{
#if defined FBNEO_DEBUG
	if (!DebugSnd_FilterRCInitted) bprintf(PRINT_ERROR, _T("filter_rc_set_RC called without init\n"));
	if (num > num_filters) bprintf(PRINT_ERROR, _T("filter_rc_set_RC called with invalid num %i\n"), num);
#endif

	set_RC_info(num, type, R1, R2, R3, C);
}

void filter_rc_init(INT32 num, INT32 type, double R1, double R2, double R3, double C, INT32 add_signal)
{
#if defined FBNEO_DEBUG
	if (num >= FLT_RC_NUM) bprintf (PRINT_ERROR, _T("filter_rc_init called for too many chips (%d)! Change FLT_RC_NUM (%d)!\n"), num, FLT_RC_NUM);
#endif

	DebugSnd_FilterRCInitted = 1;

	num_filters = num + 1;

	set_RC_info(num, type, R1, R2, R3, C);

	struct flt_rc_info *ptr;

	ptr = &flt_rc_table[num];

	ptr->src_gain = 1.00;
	ptr->src_stereo = 0; // mostly used with ay8910 mono input, so default to off for stereo
	ptr->gain = 1.00;
	ptr->limit = 0x7fff;
	ptr->output_dir = BURN_SND_ROUTE_BOTH;
	ptr->add_signal = add_signal;
}

void filter_rc_set_src_gain(INT32 num, double gain)
{
#if defined FBNEO_DEBUG
	if (!DebugSnd_FilterRCInitted) bprintf(PRINT_ERROR, _T("filter_rc_set_src_gain called without init\n"));
	if (num > num_filters) bprintf(PRINT_ERROR, _T("filter_rc_set_src_gain called with invalid num %i\n"), num);
#endif

	struct flt_rc_info *ptr;

	ptr = &flt_rc_table[num];

	ptr->src_gain = gain;
}

void filter_rc_set_src_stereo(INT32 num)
{ // allows for processing a mono (but stereo) stream
#if defined FBNEO_DEBUG
	if (!DebugSnd_FilterRCInitted) bprintf(PRINT_ERROR, _T("filter_rc_set_src_stereo called without init\n"));
	if (num > num_filters) bprintf(PRINT_ERROR, _T("filter_rc_set_src_stereo called with invalid num %i\n"), num);
#endif

	struct flt_rc_info *ptr;

	ptr = &flt_rc_table[num];

	ptr->src_stereo = 1;
}

void filter_rc_set_route(INT32 num, double nVolume, INT32 nRouteDir)
{
#if defined FBNEO_DEBUG
	if (!DebugSnd_FilterRCInitted) bprintf(PRINT_ERROR, _T("filter_rc_set_route called without init\n"));
	if (num > num_filters) bprintf(PRINT_ERROR, _T("filter_rc_set_route called with invalid num %i\n"), num);
#endif

	struct flt_rc_info *ptr;

	ptr = &flt_rc_table[num];

	ptr->gain = nVolume;
	ptr->output_dir = nRouteDir;
}

void filter_rc_set_limit(INT32 num, double nVolume)
{
#if defined FBNEO_DEBUG
	if (!DebugSnd_FilterRCInitted) bprintf(PRINT_ERROR, _T("filter_rc_set_limit called without init\n"));
	if (num > num_filters) bprintf(PRINT_ERROR, _T("filter_rc_set_limit called with invalid num %i\n"), num);
#endif

	struct flt_rc_info *ptr;

	ptr = &flt_rc_table[num];

	ptr->limit = (INT16)((double)nVolume * 0x7fff);
	//bprintf(0, _T("limiter is +-%X. (%f)\n"), ptr->limit, nVolume);
}

void filter_rc_exit()
{
#if defined FBNEO_DEBUG
	if (!DebugSnd_FilterRCInitted) bprintf(PRINT_ERROR, _T("filter_rc_exit called without init\n"));
#endif

	if (!DebugSnd_FilterRCInitted) return;

	for (INT32 i = 0; i < FLT_RC_NUM; i++) {
		struct flt_rc_info *ptr;

		ptr = &flt_rc_table[i];

		memset(ptr, 0, sizeof(flt_rc_info));
	}

	num_filters = 0;

	DebugSnd_FilterRCInitted = 0;
}
