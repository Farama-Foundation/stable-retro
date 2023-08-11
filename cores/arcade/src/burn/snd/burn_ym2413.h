// burn_ym2413.h
#include "driver.h"
extern "C" {
 #include "ym2413.h"
}

INT32 BurnYM2413Init(INT32 nClockFrequency);
INT32 BurnYM2413Init(INT32 nClockFrequency, INT32 nAdd);
void BurnYM2413SetRoute(INT32 nIndex, double nVolume, INT32 nRouteDir);
void BurnYM2413Reset(); // inits std. YM2413 instrument table
void BurnYM2413VRC7Reset(); // inits vrc7 instrument table
void BurnYM2413Exit();
extern void (*BurnYM2413Render)(INT16* pSoundBuf, INT32 nSegmentLength);
void BurnYM2413Scan(INT32 nAction, INT32 *pnMin);

#define BurnYM2413Read(a) YM2413Read(0, a)

static inline void BurnYM2413Write(INT32 Address, const UINT8 nValue)
{
#if defined FBNEO_DEBUG
	if (!DebugSnd_YM2413Initted) bprintf(PRINT_ERROR, _T("BurnYM2413Write called without init\n"));
#endif

	YM2413Write(0, Address, nValue);
}

#define BURN_SND_YM2413_YM2413_ROUTE_1		0
#define BURN_SND_YM2413_YM2413_ROUTE_2		1

#define BurnYM2413SetAllRoutes(v, d)							\
	BurnYM2413SetRoute(BURN_SND_YM2413_YM2413_ROUTE_1, v, d);	\
	BurnYM2413SetRoute(BURN_SND_YM2413_YM2413_ROUTE_2, v, d);
