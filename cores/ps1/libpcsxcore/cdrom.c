/***************************************************************************
 *   Copyright (C) 2007 Ryan Schultz, PCSX-df Team, PCSX team              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.           *
 ***************************************************************************/

/*
* Handles all CD-ROM registers and functions.
*/

#include <stdalign.h>
#include <assert.h>
#include "cdrom.h"
#include "cdrom-async.h"
#include "misc.h"
#include "ppf.h"
#include "psxdma.h"
#include "psxevents.h"
#include "arm_features.h"

/* logging */
#if 0
#define CDR_LOG SysPrintf
#else
#define CDR_LOG(...)
#endif
#if 0
#define CDR_LOG_I SysPrintf
#else
#define CDR_LOG_I(fmt, ...) \
	log_unhandled("%u cdrom: " fmt, psxRegs.cycle, ##__VA_ARGS__)
#endif
#if 0
#define CDR_LOG_IO SysPrintf
#else
#define CDR_LOG_IO(...)
#endif
//#define CDR_LOG_CMD_IRQ

static struct {
	// unused members maintain savesate compatibility
	unsigned char unused0;
	unsigned char unused1;
	unsigned char IrqMask;
	unsigned char unused2;
	unsigned char Ctrl;
	unsigned char IrqStat;

	unsigned char StatP;

	unsigned char Transfer[DATA_SIZE];
	struct {
		unsigned char Track;
		unsigned char Index;
		unsigned char Relative[3];
		unsigned char Absolute[3];
	} subq;
	unsigned char TrackChanged;
	unsigned char ReportDelay;
	unsigned char PhysCdPropagations;
	unsigned short sectorsRead;
	unsigned int  freeze_ver;

	unsigned char Prev[4];
	unsigned char Param[8];
	unsigned char Result[16];

	unsigned char ParamC;
	unsigned char ParamP;
	unsigned char ResultC;
	unsigned char ResultP;
	unsigned char ResultReady;
	unsigned char Cmd;
	unsigned char SubqForwardSectors;
	unsigned char SetlocPending;
	u32 Reading;

	unsigned char ResultTN[6];
	unsigned char ResultTD[4];
	unsigned char SetSectorPlay[4];
	unsigned char SetSectorEnd[4];
	unsigned char SetSector[4];
	unsigned char Track;
	boolean Play, Muted;
	int CurTrack;
	unsigned char Mode;
	unsigned char FileChannelSelected;
	unsigned char CurFile, CurChannel;
	int FilterFile, FilterChannel;
	unsigned char LocL[8];
	int unused4;

	xa_decode_t Xa;

	u16 FifoOffset;
	u16 FifoSize;

	u16 CmdInProgress;
	u8 Irq1Pending;
	u8 AdpcmActive;
	u32 LastReadSeekCycles;

	u8 RetryDetected;

	u8 DriveState; // enum drive_state
	u8 FastForward;
	u8 FastBackward;
	u8 errorRetryhack;

	u8 AttenuatorLeftToLeft, AttenuatorLeftToRight;
	u8 AttenuatorRightToRight, AttenuatorRightToLeft;
	u8 AttenuatorLeftToLeftT, AttenuatorLeftToRightT;
	u8 AttenuatorRightToRightT, AttenuatorRightToLeftT;
} cdr;
alignas(64) static s16 read_buf[CD_FRAMESIZE_RAW_ALIGNED / 2];

struct SubQ {
	char res0[12];
	unsigned char ControlAndADR;
	unsigned char TrackNumber;
	unsigned char IndexNumber;
	unsigned char TrackRelativeAddress[3];
	unsigned char Filler;
	unsigned char AbsoluteAddress[3];
	unsigned char CRC[2];
	char res1[72];
};

/* CD-ROM magic numbers */
#define CdlSync        0  /* nocash documentation : "Uh, actually, returns error code 40h = Invalid Command...?" */
#define CdlNop         1
#define CdlSetloc      2
#define CdlPlay        3
#define CdlForward     4
#define CdlBackward    5
#define CdlReadN       6
#define CdlStandby     7
#define CdlStop        8
#define CdlPause       9
#define CdlReset       10
#define CdlMute        11
#define CdlDemute      12
#define CdlSetfilter   13
#define CdlSetmode     14
#define CdlGetparam    15
#define CdlGetlocL     16
#define CdlGetlocP     17
#define CdlReadT       18
#define CdlGetTN       19
#define CdlGetTD       20
#define CdlSeekL       21
#define CdlSeekP       22
#define CdlSetclock    23
#define CdlGetclock    24
#define CdlTest        25
#define CdlID          26
#define CdlReadS       27
#define CdlInit        28
#define CdlGetQ        29
#define CdlReadToc     30

#ifdef CDR_LOG_CMD_IRQ
static const char * const CmdName[0x100] = {
    "CdlSync",     "CdlNop",       "CdlSetloc",  "CdlPlay",
    "CdlForward",  "CdlBackward",  "CdlReadN",   "CdlStandby",
    "CdlStop",     "CdlPause",     "CdlReset",    "CdlMute",
    "CdlDemute",   "CdlSetfilter", "CdlSetmode", "CdlGetparam",
    "CdlGetlocL",  "CdlGetlocP",   "CdlReadT",   "CdlGetTN",
    "CdlGetTD",    "CdlSeekL",     "CdlSeekP",   "CdlSetclock",
    "CdlGetclock", "CdlTest",      "CdlID",      "CdlReadS",
    "CdlInit",     NULL,           "CDlReadToc", NULL
};
#endif

unsigned char Test04[] = { 0 };
unsigned char Test05[] = { 0 };
unsigned char Test20[] = { 0x98, 0x06, 0x10, 0xC3 };
unsigned char Test22[] = { 0x66, 0x6F, 0x72, 0x20, 0x45, 0x75, 0x72, 0x6F };
unsigned char Test23[] = { 0x43, 0x58, 0x44, 0x32, 0x39 ,0x34, 0x30, 0x51 };

// cdr.IrqStat:
#define NoIntr		0
#define DataReady	1
#define Complete	2
#define Acknowledge	3
#define DataEnd		4
#define DiskError	5

/* Modes flags */
#define MODE_SPEED       (1<<7) // 0x80
#define MODE_STRSND      (1<<6) // 0x40 ADPCM on/off
#define MODE_SIZE_2340   (1<<5) // 0x20
#define MODE_SIZE_2328   (1<<4) // 0x10
#define MODE_SIZE_2048   (0<<4) // 0x00
#define MODE_SF          (1<<3) // 0x08 channel on/off
#define MODE_REPORT      (1<<2) // 0x04
#define MODE_AUTOPAUSE   (1<<1) // 0x02
#define MODE_CDDA        (1<<0) // 0x01

/* Status flags */
#define STATUS_PLAY      (1<<7) // 0x80
#define STATUS_SEEK      (1<<6) // 0x40
#define STATUS_READ      (1<<5) // 0x20
#define STATUS_SHELLOPEN (1<<4) // 0x10
#define STATUS_UNKNOWN3  (1<<3) // 0x08
#define STATUS_SEEKERROR (1<<2) // 0x04
#define STATUS_ROTATING  (1<<1) // 0x02
#define STATUS_ERROR     (1<<0) // 0x01

/* Errors */
#define ERROR_NOTREADY   (1<<7) // 0x80
#define ERROR_INVALIDCMD (1<<6) // 0x40
#define ERROR_BAD_ARGNUM (1<<5) // 0x20
#define ERROR_BAD_ARGVAL (1<<4) // 0x10
#define ERROR_SHELLOPEN  (1<<3) // 0x08

// 1x = 75 sectors per second
// PSXCLK = 1 sec in the ps
// so (PSXCLK / 75) = cdr read time (linuzappz)
#define cdReadTime (PSXCLK / 75)

#define LOCL_INVALID 0xff
#define SUBQ_FORWARD_SECTORS 2u

enum drive_state {
	DRIVESTATE_STANDBY = 0, // different from paused
	DRIVESTATE_LID_OPEN,
	DRIVESTATE_RESCAN_CD,
	DRIVESTATE_PREPARE_CD,
	DRIVESTATE_STOPPED,
	DRIVESTATE_PAUSED,
	DRIVESTATE_PLAY_READ,
	DRIVESTATE_SEEK,
};

static struct CdrStat cdr_stat;

static unsigned int msf2sec(const u8 *msf) {
	return ((msf[0] * 60 + msf[1]) * 75) + msf[2];
}

// cdrPlayReadInterrupt
#define CDRPLAYREAD_INT(eCycle, isFirst) { \
	u32 e_ = eCycle; \
	psxRegs.interrupt |= (1 << PSXINT_CDREAD); \
	if (isFirst) \
		psxRegs.intCycle[PSXINT_CDREAD].sCycle = psxRegs.cycle; \
	else \
		psxRegs.intCycle[PSXINT_CDREAD].sCycle += psxRegs.intCycle[PSXINT_CDREAD].cycle; \
	psxRegs.intCycle[PSXINT_CDREAD].cycle = e_; \
	set_event_raw_abs(PSXINT_CDREAD, psxRegs.intCycle[PSXINT_CDREAD].sCycle + e_); \
}

#define StopReading() { \
	cdr.Reading = 0; \
	psxRegs.interrupt &= ~(1 << PSXINT_CDREAD); \
}

#define StopCdda() { \
	cdr.Play = FALSE; \
	cdr.FastForward = 0; \
	cdr.FastBackward = 0; \
}

#define SetPlaySeekRead(x, f) { \
	x &= ~(STATUS_PLAY | STATUS_SEEK | STATUS_READ); \
	x |= f; \
}

#define SetResultSize_(size) { \
	cdr.ResultP = 0; \
	cdr.ResultC = size; \
	cdr.ResultReady = 1; \
}

#define SetResultSize(size) { \
	if (cdr.ResultP < cdr.ResultC) \
		CDR_LOG_I("overwriting result, len=%u\n", cdr.ResultC); \
	SetResultSize_(size); \
}

static void setIrq(u8 irq, int log_cmd)
{
	u8 old = cdr.IrqStat & cdr.IrqMask ? 1 : 0;
	u8 new_ = irq & cdr.IrqMask ? 1 : 0;

	cdr.IrqStat = irq;
	if ((old ^ new_) & new_)
		psxHu32ref(0x1070) |= SWAP32((u32)0x4);

#ifdef CDR_LOG_CMD_IRQ
	if (cdr.IrqStat)
	{
		int i;
		CDR_LOG_I("CDR IRQ=%d cmd %02x irqstat %02x: ",
			!!(cdr.IrqStat & cdr.IrqMask), log_cmd, cdr.IrqStat);
		for (i = 0; i < cdr.ResultC; i++)
			SysPrintf("%02x ", cdr.Result[i]);
		SysPrintf("\n");
	}
#endif
}

// timing used in this function was taken from tests on real hardware
// (yes it's slow, but you probably don't want to modify it)
void cdrLidSeekInterrupt(void)
{
	CDR_LOG_I("%s cdr.DriveState=%d\n", __func__, cdr.DriveState);

	switch (cdr.DriveState) {
	default:
	case DRIVESTATE_STANDBY:
		StopCdda();
		//StopReading();
		SetPlaySeekRead(cdr.StatP, 0);

		if (cdra_getStatus(&cdr_stat) == -1)
			return;

		if (cdr_stat.Status & STATUS_SHELLOPEN)
		{
			memset(cdr.Prev, 0xff, sizeof(cdr.Prev));
			cdr.DriveState = DRIVESTATE_LID_OPEN;
			set_event(PSXINT_CDRLID, 0x800);
		}
		break;

	case DRIVESTATE_LID_OPEN:
		if (cdra_getStatus(&cdr_stat) != 0)
			cdr_stat.Status &= ~STATUS_SHELLOPEN;

		// 02, 12, 10
		if (!(cdr.StatP & STATUS_SHELLOPEN)) {
			int was_reading = cdr.Reading;
			StopReading();
			SetPlaySeekRead(cdr.StatP, 0);
			cdr.StatP |= STATUS_SHELLOPEN;
			memset(cdr.Prev, 0xff, sizeof(cdr.Prev));

			// IIRC this sometimes doesn't happen on real hw
			// (when lots of commands are sent?)
			SetResultSize(2);
			cdr.Result[0] = cdr.StatP | STATUS_SEEKERROR;
			cdr.Result[1] = ERROR_SHELLOPEN;
			if (cdr.CmdInProgress || was_reading) {
				psxRegs.interrupt &= ~(1 << PSXINT_CDR);
				cdr.CmdInProgress = 0;
				cdr.Result[0] = cdr.StatP | STATUS_ERROR;
				cdr.Result[1] = ERROR_NOTREADY;
			}
			setIrq(DiskError, 0x1006);

			set_event(PSXINT_CDRLID, cdReadTime * 30);
			break;
		}
		else if (cdr.StatP & STATUS_ROTATING) {
			cdr.StatP &= ~STATUS_ROTATING;
		}
		else if (!(cdr_stat.Status & STATUS_SHELLOPEN)) {
			// closed now
			CheckCdrom();

			// cdr.StatP STATUS_SHELLOPEN is "sticky"
			// and is only cleared by CdlNop

			cdr.DriveState = DRIVESTATE_RESCAN_CD;
			set_event(PSXINT_CDRLID, cdReadTime * 105);
			break;
		}

		// recheck for close
		set_event(PSXINT_CDRLID, cdReadTime * 3);
		break;

	case DRIVESTATE_RESCAN_CD:
		cdr.StatP |= STATUS_ROTATING;
		cdr.DriveState = DRIVESTATE_PREPARE_CD;

		// this is very long on real hardware, over 6 seconds
		// make it a bit faster here...
		set_event(PSXINT_CDRLID, cdReadTime * 150);
		break;

	case DRIVESTATE_PREPARE_CD:
		if (cdr.StatP & STATUS_SEEK) {
			SetPlaySeekRead(cdr.StatP, 0);
			cdr.DriveState = DRIVESTATE_STANDBY;
		}
		else {
			SetPlaySeekRead(cdr.StatP, STATUS_SEEK);
			set_event(PSXINT_CDRLID, cdReadTime * 26);
		}
		break;
	}
}

static void Find_CurTrack(const u8 *time)
{
	int current, sect;

	current = msf2sec(time);

	for (cdr.CurTrack = 1; cdr.CurTrack < cdr.ResultTN[1]; cdr.CurTrack++) {
		cdra_getTD(cdr.CurTrack + 1, cdr.ResultTD);
		sect = msf2sec(cdr.ResultTD);
		if (sect - current >= 150)
			break;
	}
}

static void generate_subq(const u8 *time)
{
	unsigned char start[3], next[3];
	unsigned int this_s, start_s, next_s, pregap;
	int relative_s;

	cdra_getTD(cdr.CurTrack, start);
	if (cdr.CurTrack + 1 <= cdr.ResultTN[1]) {
		pregap = 150;
		cdra_getTD(cdr.CurTrack + 1, next);
	}
	else {
		// last track - cd size
		pregap = 0;
		memcpy(next, cdr.SetSectorEnd, 3);
	}

	this_s = msf2sec(time);
	start_s = msf2sec(start);
	next_s = msf2sec(next);

	cdr.TrackChanged = FALSE;

	if (next_s - this_s < pregap) {
		cdr.TrackChanged = TRUE;
		cdr.CurTrack++;
		start_s = next_s;
	}

	cdr.subq.Index = 1;

	relative_s = this_s - start_s;
	if (relative_s < 0) {
		cdr.subq.Index = 0;
		relative_s = -relative_s;
	}
	lba2msf(relative_s, &cdr.subq.Relative[0],
		&cdr.subq.Relative[1], &cdr.subq.Relative[2]);

	cdr.subq.Track = itob(cdr.CurTrack);
	cdr.subq.Relative[0] = itob(cdr.subq.Relative[0]);
	cdr.subq.Relative[1] = itob(cdr.subq.Relative[1]);
	cdr.subq.Relative[2] = itob(cdr.subq.Relative[2]);
	cdr.subq.Absolute[0] = itob(time[0]);
	cdr.subq.Absolute[1] = itob(time[1]);
	cdr.subq.Absolute[2] = itob(time[2]);
}

static int ReadTrack(const u8 *time)
{
	int ret;

	CDR_LOG("ReadTrack *** %02d:%02d:%02d\n", tmp[0], tmp[1], tmp[2]);

	if (memcmp(cdr.Prev, time, 3) == 0)
		return 1;

	ret = cdra_readTrack(time);
	if (ret == 0)
		memcpy(cdr.Prev, time, 3);
	return ret == 0;
}

static void UpdateSubq(const u8 *time)
{
	int ret = -1, s = MSF2SECT(time[0], time[1], time[2]);
	struct SubQ subq;
	u16 crc;

	if (CheckSBI(s))
		return;

	if (cdr.CurTrack == 1)
		ret = cdra_readSub(time, &subq);
	if (ret == 0) {
		crc = calcCrc((u8 *)&subq + 12, 10);
		if (crc == (((u16)subq.CRC[0] << 8) | subq.CRC[1])) {
			cdr.subq.Track = subq.TrackNumber;
			cdr.subq.Index = subq.IndexNumber;
			memcpy(cdr.subq.Relative, subq.TrackRelativeAddress, 3);
			memcpy(cdr.subq.Absolute, subq.AbsoluteAddress, 3);
		}
		else {
			CDR_LOG_I("subq bad crc @%02d:%02d:%02d\n",
				time[0], time[1], time[2]);
		}
	}
	else {
		generate_subq(time);
	}

	CDR_LOG(" -> %02x,%02x %02x:%02x:%02x %02x:%02x:%02x\n",
		cdr.subq.Track, cdr.subq.Index,
		cdr.subq.Relative[0], cdr.subq.Relative[1], cdr.subq.Relative[2],
		cdr.subq.Absolute[0], cdr.subq.Absolute[1], cdr.subq.Absolute[2]);
}

static void cdrPlayInterrupt_Autopause()
{
	u32 abs_lev_max = 0;
	boolean abs_lev_chselect;
	u32 i;

	if ((cdr.Mode & MODE_AUTOPAUSE) && cdr.TrackChanged) {
		CDR_LOG_I("autopause\n");

		SetResultSize(1);
		cdr.Result[0] = cdr.StatP;
		setIrq(DataEnd, 0x1000); // 0x1000 just for logging purposes

		StopCdda();
		SetPlaySeekRead(cdr.StatP, 0);
		cdr.DriveState = DRIVESTATE_PAUSED;
	}
	else if ((cdr.Mode & MODE_REPORT) && !cdr.ReportDelay &&
		 ((cdr.subq.Absolute[2] & 0x0f) == 0 || cdr.FastForward || cdr.FastBackward))
	{
		SetResultSize(8);
		cdr.Result[0] = cdr.StatP;
		cdr.Result[1] = cdr.subq.Track;
		cdr.Result[2] = cdr.subq.Index;
		
		abs_lev_chselect = cdr.subq.Absolute[1] & 0x01;
		
		/* 8 is a hack. For accuracy, it should be 588. */
		for (i = 0; i < 8; i++)
		{
			abs_lev_max = MAX_VALUE(abs_lev_max, abs(read_buf[i * 2 + abs_lev_chselect]));
		}
		abs_lev_max = MIN_VALUE(abs_lev_max, 32767);
		abs_lev_max |= abs_lev_chselect << 15;

		if (cdr.subq.Absolute[2] & 0x10) {
			cdr.Result[3] = cdr.subq.Relative[0];
			cdr.Result[4] = cdr.subq.Relative[1] | 0x80;
			cdr.Result[5] = cdr.subq.Relative[2];
		}
		else {
			cdr.Result[3] = cdr.subq.Absolute[0];
			cdr.Result[4] = cdr.subq.Absolute[1];
			cdr.Result[5] = cdr.subq.Absolute[2];
		}
		cdr.Result[6] = abs_lev_max >> 0;
		cdr.Result[7] = abs_lev_max >> 8;

		setIrq(DataReady, 0x1001);
	}

	if (cdr.ReportDelay)
		cdr.ReportDelay--;
}

static boolean canDoTurbo(void)
{
	u32 c = psxRegs.cycle;
	return Config.TurboCD && !cdr.RetryDetected && !cdr.AdpcmActive
		//&& c - psxRegs.intCycle[PSXINT_SPUDMA].sCycle > (u32)cdReadTime * 2
		&& c - psxRegs.intCycle[PSXINT_MDECOUTDMA].sCycle > (u32)cdReadTime * 16;
}

static int cdrSeekTime(unsigned char *target)
{
	int diff = abs((int)msf2sec(cdr.SetSectorPlay) - (int)msf2sec(target));
	int seekTime = diff * (cdReadTime / 2000);
	int cyclesSinceRS = psxRegs.cycle - cdr.LastReadSeekCycles;
	seekTime = MAX_VALUE(seekTime, 20000);

	// sled seek?
	if (diff >= 7200)
		seekTime = PSXCLK / 7 + diff * 64;
	// add *something* as rotation time until the target sector
	if (cyclesSinceRS >= cdReadTime)
		seekTime += (8 - ((cyclesSinceRS >> 18) & 7)) * (cdReadTime / 2);

	// Transformers Beast Wars Transmetals does Setloc(x),SeekL,Setloc(x),ReadN
	// and then wants some slack time
	if (cdr.DriveState == DRIVESTATE_PAUSED || cyclesSinceRS < cdReadTime *3/2)
		seekTime += cdReadTime;

	//seekTime = MIN_VALUE(seekTime, PSXCLK * 2 / 3);
	CDR_LOG("seek: %02d:%02d:%02d %.2f %.2f (%.2f) st %d di %d\n",
		target[0], target[1], target[2], (float)seekTime / PSXCLK,
		(float)seekTime / cdReadTime, (float)cyclesSinceRS / cdReadTime,
		cdr.DriveState, diff);
	return seekTime;
}

static u32 cdrAlignTimingHack(u32 cycles)
{
	/*
	 * timing hack for T'ai Fu - Wrath of the Tiger:
	 * The game has a bug where it issues some cdc commands from a low priority
	 * vint handler, however there is a higher priority default bios handler
	 * that acks the vint irq and returns, so game's handler is not reached
	 * (see bios irq handler chains at e004 and the game's irq handling func
	 * at 80036810). For the game to work, vint has to arrive after the bios
	 * vint handler rejects some other irq (of which only cd and rcnt2 are
	 * active), but before the game's handler loop reads I_STAT. The time
	 * window for this is quite small (~1k cycles of so). Apparently this
	 * somehow happens naturally on the real hardware.
	 *
	 * Note: always enforcing this breaks other games like Crash PAL version
	 * (inputs get dropped because bios handler doesn't see interrupts).
	 */
	u32 vint_rel = rcnts[3].cycleStart + 63000 - psxRegs.cycle;
	while ((s32)(vint_rel - cycles) < 0)
		vint_rel += PSXCLK / 60;
	return vint_rel;
}

static void cdrUpdateTransferBuf(const u8 *buf);
static void cdrReadInterrupt(void);
static void cdrPrepCdda(s16 *buf, int samples);

static void msfiAdd(u8 *msfi, u32 count)
{
	assert(count < 75);
	msfi[2] += count;
	if (msfi[2] >= 75) {
		msfi[2] -= 75;
		msfi[1]++;
		if (msfi[1] == 60) {
			msfi[1] = 0;
			msfi[0]++;
		}
	}
}

static void msfiSub(u8 *msfi, u32 count)
{
	assert(count < 75);
	msfi[2] -= count;
	if ((s8)msfi[2] < 0) {
		msfi[2] += 75;
		msfi[1]--;
		if ((s8)msfi[1] < 0) {
			msfi[1] = 60;
			msfi[0]--;
		}
	}
}

static int msfiEq(const u8 *a, const u8 *b)
{
	return a[0] == b[0] && a[1] == b[1] && a[2] == b[2];
}

void cdrPlayReadInterrupt(void)
{
	// this works but causes instability for timing sensitive games
#if 0
	int hit = cdra_prefetch(cdr.SetSectorPlay[0], cdr.SetSectorPlay[1], cdr.SetSectorPlay[2]);
	if (!hit && cdr.PhysCdPropagations < 75/2) {
		// this propagates the real cdrom delays to the emulated game
		CDRPLAYREAD_INT(cdReadTime / 2, 0);
		cdr.PhysCdPropagations++;
		return;
	}
#endif
	cdr.LastReadSeekCycles = psxRegs.cycle;

	if (cdr.Reading) {
		cdrReadInterrupt();
		return;
	}

	if (!cdr.Play) return;

	CDR_LOG("CDDA - %02d:%02d:%02d m %02x\n",
		cdr.SetSectorPlay[0], cdr.SetSectorPlay[1], cdr.SetSectorPlay[2], cdr.Mode);

	cdr.DriveState = DRIVESTATE_PLAY_READ;
	SetPlaySeekRead(cdr.StatP, STATUS_PLAY);
	if (memcmp(cdr.SetSectorPlay, cdr.SetSectorEnd, 3) == 0) {
		CDR_LOG_I("end stop\n");
		StopCdda();
		SetPlaySeekRead(cdr.StatP, 0);
		cdr.TrackChanged = TRUE;
		cdr.DriveState = DRIVESTATE_PAUSED;
	}
	else {
		cdra_readCDDA(cdr.SetSectorPlay, read_buf);
	}

	if (!cdr.IrqStat && (cdr.Mode & (MODE_AUTOPAUSE|MODE_REPORT)))
		cdrPlayInterrupt_Autopause();

	if (cdr.Play && !Config.Cdda) {
		cdrPrepCdda(read_buf, CD_FRAMESIZE_RAW / 4);
		SPU_playCDDAchannel(read_buf, CD_FRAMESIZE_RAW, psxRegs.cycle, 0);
	}

	msfiAdd(cdr.SetSectorPlay, 1);
	cdra_prefetch(cdr.SetSectorPlay[0], cdr.SetSectorPlay[1], cdr.SetSectorPlay[2]);

	// update for CdlGetlocP/autopause
	generate_subq(cdr.SetSectorPlay);

	CDRPLAYREAD_INT(cdReadTime, 0);

	// stop propagation since it breaks streaming
	cdr.PhysCdPropagations = 0xff;
}

static void softReset(void)
{
	cdra_getStatus(&cdr_stat);
	if (cdr_stat.Status & STATUS_SHELLOPEN) {
		cdr.DriveState = DRIVESTATE_LID_OPEN;
		cdr.StatP = STATUS_SHELLOPEN;
	}
	else if (CdromId[0] == '\0') {
		cdr.DriveState = DRIVESTATE_STOPPED;
		cdr.StatP = 0;
	}
	else {
		cdr.DriveState = DRIVESTATE_STANDBY;
		cdr.StatP = STATUS_ROTATING;
	}

	cdr.FifoOffset = DATA_SIZE; // fifo empty
	cdr.LocL[0] = LOCL_INVALID;
	cdr.Mode = MODE_SIZE_2340;
	cdr.Muted = FALSE;
	SPU_setCDvol(cdr.AttenuatorLeftToLeft, cdr.AttenuatorLeftToRight,
		cdr.AttenuatorRightToLeft, cdr.AttenuatorRightToRight, psxRegs.cycle);
}

#define CMD_PART2           0x100
#define CMD_WHILE_NOT_READY 0x200

void cdrInterrupt(void) {
	int start_rotating = 0;
	int error = 0;
	u32 cycles, seekTime = 0;
	u32 second_resp_time = 0;
	const void *buf;
	u8 ParamC;
	int read_ok;
	u16 not_ready = 0;
	u8 IrqStat = Acknowledge;
	u8 DriveStateOld;
	u16 Cmd;
	int i;

	if (cdr.IrqStat) {
		CDR_LOG_I("cmd %02x with irqstat %x\n",
			cdr.CmdInProgress, cdr.IrqStat);
		return;
	}
	if (cdr.Irq1Pending) {
		// hand out the "newest" sector, according to nocash
		cdrUpdateTransferBuf(cdra_getBuffer());
		CDR_LOG_I("%x:%02x:%02x loaded on ack, cmd=%02x res=%02x\n",
			cdr.Transfer[0], cdr.Transfer[1], cdr.Transfer[2],
			cdr.CmdInProgress, cdr.Irq1Pending);
		SetResultSize(1);
		cdr.Result[0] = cdr.Irq1Pending;
		cdr.Irq1Pending = 0;
		setIrq((cdr.Irq1Pending & STATUS_ERROR) ? DiskError : DataReady, 0x1003);
		return;
	}

	// default response
	SetResultSize(1);
	cdr.Result[0] = cdr.StatP;

	Cmd = cdr.CmdInProgress;
	cdr.CmdInProgress = 0;
	ParamC = cdr.ParamC;

	if (Cmd < 0x100) {
		cdr.Ctrl &= ~0x80;
		cdr.ParamC = 0;
		cdr.Cmd = 0;
	}

	switch (cdr.DriveState) {
	case DRIVESTATE_PREPARE_CD:
		if (Cmd > 2) {
			// Syphon filter 2 expects commands to work shortly after it sees
			// STATUS_ROTATING, so give up trying to emulate the startup seq
			cdr.DriveState = DRIVESTATE_STANDBY;
			cdr.StatP &= ~STATUS_SEEK;
			psxRegs.interrupt &= ~(1 << PSXINT_CDRLID);
			break;
		}
		// fallthrough
	case DRIVESTATE_LID_OPEN:
	case DRIVESTATE_RESCAN_CD:
		// no disk or busy with the initial scan, allowed cmds are limited
		not_ready = CMD_WHILE_NOT_READY;
		break;
	}

	switch (Cmd | not_ready) {
		case CdlNop:
		case CdlNop + CMD_WHILE_NOT_READY:
			if (cdr.DriveState != DRIVESTATE_LID_OPEN)
				cdr.StatP &= ~STATUS_SHELLOPEN;
			break;

		case CdlSetloc:
		case CdlSetloc + CMD_WHILE_NOT_READY: // apparently?
			if (cdr.StatP & STATUS_SHELLOPEN)
				// wrong? Driver2 vs Amerzone
				goto set_error;

			// MM must be BCD, SS must be BCD and <0x60, FF must be BCD and <0x75
			if (((cdr.Param[0] & 0x0F) > 0x09) || (cdr.Param[0] > 0x99) || ((cdr.Param[1] & 0x0F) > 0x09) || (cdr.Param[1] >= 0x60) || ((cdr.Param[2] & 0x0F) > 0x09) || (cdr.Param[2] >= 0x75))
			{
				CDR_LOG_I("Invalid/out of range seek to %02X:%02X:%02X\n", cdr.Param[0], cdr.Param[1], cdr.Param[2]);
				if (++cdr.errorRetryhack > 100)
					break;
				error = ERROR_BAD_ARGNUM;
				goto set_error;
			}
			else
			{
				u8 set_loc[3];
				for (i = 0; i < 3; i++)
					set_loc[i] = btoi(cdr.Param[i]);
				if ((msfiEq(cdr.SetSector, set_loc)) //|| msfiEq(cdr.Param, cdr.Transfer))
						&& !cdr.SetlocPending)
					cdr.RetryDetected++;
				else
					cdr.RetryDetected = 0;
				memcpy(cdr.SetSector, set_loc, 3);
				cdr.SetSector[3] = 0;
				cdr.SetlocPending = 1;
				cdr.errorRetryhack = 0;
			}
			break;

		do_CdlPlay:
		case CdlPlay:
			StopCdda();
			StopReading();

			cdr.FastBackward = 0;
			cdr.FastForward = 0;

			// BIOS CD Player
			// - Pause player, hit Track 01/02/../xx (Setloc issued!!)

			if (ParamC != 0 && cdr.Param[0] != 0) {
				int track = btoi( cdr.Param[0] );

				if (track <= cdr.ResultTN[1])
					cdr.CurTrack = track;

				CDR_LOG("PLAY track %d\n", cdr.CurTrack);

				if (cdra_getTD(cdr.CurTrack, cdr.ResultTD) != -1) {
					seekTime = cdrSeekTime(cdr.ResultTD);
					memcpy(cdr.SetSectorPlay, cdr.ResultTD, 3);
				}
			}
			else if (cdr.SetlocPending) {
				seekTime = cdrSeekTime(cdr.SetSector);
				memcpy(cdr.SetSectorPlay, cdr.SetSector, 4);
			}
			else {
				CDR_LOG("PLAY Resume @ %d:%d:%d\n",
					cdr.SetSectorPlay[0], cdr.SetSectorPlay[1], cdr.SetSectorPlay[2]);
			}
			cdr.SetlocPending = 0;

			/*
			Rayman: detect track changes
			- fixes logo freeze

			Twisted Metal 2: skip PREGAP + starting accurate SubQ
			- plays tracks without retry play

			Wild 9: skip PREGAP + starting accurate SubQ
			- plays tracks without retry play
			*/
			Find_CurTrack(cdr.SetSectorPlay);
			generate_subq(cdr.SetSectorPlay);
			cdr.LocL[0] = LOCL_INVALID;
			cdr.SubqForwardSectors = 1;
			cdr.TrackChanged = FALSE;
			cdr.FileChannelSelected = 0;
			cdr.AdpcmActive = 0;
			cdr.ReportDelay = 60;
			cdr.sectorsRead = 0;

			SetPlaySeekRead(cdr.StatP, STATUS_SEEK | STATUS_ROTATING);
			
			// BIOS player - set flag again
			cdr.Play = TRUE;
			cdr.DriveState = DRIVESTATE_PLAY_READ;
			cdr.PhysCdPropagations = 0;

			CDRPLAYREAD_INT(cdReadTime + seekTime, 1);
			start_rotating = 1;
			break;

		case CdlForward:
			// TODO: error 80 if stopped
			IrqStat = Complete;

			// GameShark CD Player: Calls 2x + Play 2x
			cdr.FastForward = 1;
			cdr.FastBackward = 0;
			break;

		case CdlBackward:
			IrqStat = Complete;

			// GameShark CD Player: Calls 2x + Play 2x
			cdr.FastBackward = 1;
			cdr.FastForward = 0;
			break;

		case CdlStandby:
			if (cdr.DriveState != DRIVESTATE_STOPPED) {
				error = ERROR_BAD_ARGNUM;
				goto set_error;
			}
			second_resp_time = cdReadTime * 125 / 2;
			start_rotating = 1;
			break;

		case CdlStandby + CMD_PART2:
			IrqStat = Complete;
			break;

		case CdlStop:
			if (cdr.Play) {
				// grab time for current track
				cdra_getTD(cdr.CurTrack, cdr.ResultTD);
				memcpy(cdr.SetSectorPlay, cdr.ResultTD, 3);
			}

			StopCdda();
			StopReading();
			SetPlaySeekRead(cdr.StatP, 0);
			cdr.StatP &= ~STATUS_ROTATING;
			cdr.LocL[0] = LOCL_INVALID;

			second_resp_time = 0x800;
			if (cdr.DriveState != DRIVESTATE_STOPPED)
				second_resp_time = cdReadTime * 30 / 2;

			cdr.DriveState = DRIVESTATE_STOPPED;
			break;

		case CdlStop + CMD_PART2:
			IrqStat = Complete;
			break;

		case CdlPause:
			if (cdr.AdpcmActive) {
				cdr.AdpcmActive = 0;
				cdr.Xa.nsamples = 0;
				SPU_playADPCMchannel(&cdr.Xa, psxRegs.cycle, 1); // flush adpcm
			}
			StopCdda();
			StopReading();

			// how the drive maintains the position while paused is quite
			// complicated, this is the minimum to make "Bedlam" happy
			msfiSub(cdr.SetSectorPlay, MIN_VALUE(cdr.sectorsRead, 4));
			cdr.sectorsRead = 0;

			/*
			Gundam Battle Assault 2
			Hokuto no Ken 2
			InuYasha - Feudal Fairy Tale
			Dance Dance Revolution Konamix
			Digimon Rumble Arena
			...
			*/
			if (!(cdr.StatP & (STATUS_PLAY | STATUS_READ)))
			{
				second_resp_time = 7000;
			}
			else
			{
				second_resp_time = 2100011;
				// a hack to try to avoid weird cmd vs irq1 races causing games to retry
				second_resp_time += (cdr.RetryDetected & 15) * 100001;
			}
			SetPlaySeekRead(cdr.StatP, 0);
			DriveStateOld = cdr.DriveState;
			cdr.DriveState = DRIVESTATE_PAUSED;
			if (DriveStateOld == DRIVESTATE_SEEK) {
				// According to Duckstation this fails, but the
				// exact conditions and effects are not clear.
				// Moto Racer World Tour seems to rely on this.
				// For now assume pause works anyway, just errors out.
				error = ERROR_NOTREADY;
				goto set_error;
			}
			break;

		case CdlPause + CMD_PART2:
			IrqStat = Complete;
			break;

		case CdlReset:
		case CdlReset + CMD_WHILE_NOT_READY:
			// note: nocash and Duckstation calls this 'Init', but
			// the official SDK calls it 'Reset', and so do we
			StopCdda();
			StopReading();
			softReset();
			second_resp_time = not_ready ? 70000 : 4100000;
			start_rotating = 1;
			break;

		case CdlReset + CMD_PART2:
		case CdlReset + CMD_PART2 + CMD_WHILE_NOT_READY:
			IrqStat = Complete;
			break;

		case CdlMute:
			cdr.Muted = TRUE;
			SPU_setCDvol(0, 0, 0, 0, psxRegs.cycle);
			break;

		case CdlDemute:
			cdr.Muted = FALSE;
			SPU_setCDvol(cdr.AttenuatorLeftToLeft, cdr.AttenuatorLeftToRight,
				cdr.AttenuatorRightToLeft, cdr.AttenuatorRightToRight, psxRegs.cycle);
			break;

		case CdlSetfilter:
			cdr.FilterFile = cdr.Param[0];
			cdr.FilterChannel = cdr.Param[1];
			cdr.FileChannelSelected = 0;
			break;

		case CdlSetmode:
		case CdlSetmode + CMD_WHILE_NOT_READY:
			CDR_LOG("cdrWrite1() Log: Setmode %x\n", cdr.Param[0]);
			cdr.Mode = cdr.Param[0];
			break;

		case CdlGetparam:
		case CdlGetparam + CMD_WHILE_NOT_READY:
			/* Gameblabla : According to mednafen, Result size should be 5 and done this way. */
			SetResultSize_(5);
			cdr.Result[1] = cdr.Mode;
			cdr.Result[2] = 0;
			cdr.Result[3] = cdr.FilterFile;
			cdr.Result[4] = cdr.FilterChannel;
			break;

		case CdlGetlocL:
			if (cdr.LocL[0] == LOCL_INVALID) {
				error = 0x80;
				goto set_error;
			}
			SetResultSize_(8);
			memcpy(cdr.Result, cdr.LocL, 8);
			break;

		case CdlGetlocP:
			SetResultSize_(8);
			memcpy(&cdr.Result, &cdr.subq, 8);
			break;

		case CdlReadT: // SetSession?
			// really long
			second_resp_time = cdReadTime * 290 / 4;
			start_rotating = 1;
			break;

		case CdlReadT + CMD_PART2:
			IrqStat = Complete;
			break;

		case CdlGetTN:
			if (cdra_getTN(cdr.ResultTN) != 0) {
				assert(0);
			}
			SetResultSize_(3);
			cdr.Result[1] = itob(cdr.ResultTN[0]);
			cdr.Result[2] = itob(cdr.ResultTN[1]);
			break;

		case CdlGetTD:
			cdr.Track = btoi(cdr.Param[0]);
			if (cdra_getTD(cdr.Track, cdr.ResultTD) != 0) {
				error = ERROR_BAD_ARGVAL;
				goto set_error;
			}
			SetResultSize_(3);
			cdr.Result[1] = itob(cdr.ResultTD[0]);
			cdr.Result[2] = itob(cdr.ResultTD[1]);
			// no sector number
			//cdr.Result[3] = itob(cdr.ResultTD[2]);
			break;

		case CdlSeekL:
		case CdlSeekP:
			StopCdda();
			StopReading();
			SetPlaySeekRead(cdr.StatP, STATUS_SEEK | STATUS_ROTATING);

			if (!canDoTurbo())
				seekTime = cdrSeekTime(cdr.SetSector);
			memcpy(cdr.SetSectorPlay, cdr.SetSector, 4);
			cdr.DriveState = DRIVESTATE_SEEK;
			cdra_prefetch(cdr.SetSectorPlay[0], cdr.SetSectorPlay[1],
					cdr.SetSectorPlay[2]);
			/*
			Crusaders of Might and Magic = 0.5x-4x
			- fix cutscene speech start

			Eggs of Steel = 2x-?
			- fix new game

			Medievil = ?-4x
			- fix cutscene speech

			Rockman X5 = 0.5-4x
			- fix capcom logo
			*/
			second_resp_time = cdReadTime + seekTime;
			start_rotating = 1;
			break;

		case CdlSeekL + CMD_PART2:
		case CdlSeekP + CMD_PART2:
			SetPlaySeekRead(cdr.StatP, 0);
			cdr.Result[0] = cdr.StatP;
			IrqStat = Complete;

			Find_CurTrack(cdr.SetSectorPlay);
			read_ok = ReadTrack(cdr.SetSectorPlay);
			if (read_ok && (buf = cdra_getBuffer()))
				memcpy(cdr.LocL, buf, 8);
			UpdateSubq(cdr.SetSectorPlay);
			cdr.DriveState = DRIVESTATE_STANDBY;
			cdr.TrackChanged = FALSE;
			cdr.LastReadSeekCycles = psxRegs.cycle;
			break;

		case CdlTest:
		case CdlTest + CMD_WHILE_NOT_READY:
			switch (cdr.Param[0]) {
				case 0x20: // System Controller ROM Version
					SetResultSize_(4);
					memcpy(cdr.Result, Test20, 4);
					break;
				case 0x22:
					SetResultSize_(8);
					memcpy(cdr.Result, Test22, 4);
					break;
				case 0x23: case 0x24:
					SetResultSize_(8);
					memcpy(cdr.Result, Test23, 4);
					break;
			}
			break;

		case CdlID:
			second_resp_time = 20480;
			break;

		case CdlID + CMD_PART2:
			SetResultSize_(8);
			cdr.Result[0] = cdr.StatP;
			cdr.Result[1] = 0;
			cdr.Result[2] = 0;
			cdr.Result[3] = 0;

			// 0x10 - audio | 0x40 - disk missing | 0x80 - unlicensed
			if (cdra_getStatus(&cdr_stat) != 0 || cdr_stat.Type == 0 || cdr_stat.Type == 0xff) {
				cdr.Result[1] = 0xc0;
			}
			else {
				if (cdr_stat.Type == 2)
					cdr.Result[1] |= 0x10;
				if (CdromId[0] == '\0')
					cdr.Result[1] |= 0x80;
			}
			cdr.Result[0] |= (cdr.Result[1] >> 4) & 0x08;
			CDR_LOG_I("CdlID: %02x %02x %02x %02x\n", cdr.Result[0],
				cdr.Result[1], cdr.Result[2], cdr.Result[3]);

			/* This adds the string "PCSX" in Playstation bios boot screen */
			memcpy((char *)&cdr.Result[4], "PCSX", 4);
			IrqStat = Complete;
			break;

		case CdlInit:
		case CdlInit + CMD_WHILE_NOT_READY:
			StopCdda();
			StopReading();
			SetPlaySeekRead(cdr.StatP, 0);
			// yes, it really sets STATUS_SHELLOPEN
			cdr.StatP |= STATUS_SHELLOPEN;
			cdr.DriveState = DRIVESTATE_RESCAN_CD;
			set_event(PSXINT_CDRLID, 20480);
			start_rotating = 1;
			break;

		case CdlGetQ:
		case CdlGetQ + CMD_WHILE_NOT_READY:
			break;

		case CdlReadToc:
		case CdlReadToc + CMD_WHILE_NOT_READY:
			cdr.LocL[0] = LOCL_INVALID;
			second_resp_time = cdReadTime * 180 / 4;
			start_rotating = 1;
			break;

		case CdlReadToc + CMD_PART2:
		case CdlReadToc + CMD_PART2 + CMD_WHILE_NOT_READY:
			IrqStat = Complete;
			break;

		case CdlReadN:
		case CdlReadS:
			if (cdr.Reading && !cdr.SetlocPending)
				break;

			Find_CurTrack(cdr.SetlocPending ? cdr.SetSector : cdr.SetSectorPlay);

			if ((cdr.Mode & MODE_CDDA) && cdr.CurTrack > 1)
				// Read* acts as play for cdda tracks in cdda mode
				goto do_CdlPlay;

			StopCdda();
			if (cdr.SetlocPending) {
				seekTime = cdrSeekTime(cdr.SetSector);
				memcpy(cdr.SetSectorPlay, cdr.SetSector, 4);
				cdr.SetlocPending = 0;
			}
			cdr.Reading = 1;
			cdr.FileChannelSelected = 0;
			cdr.AdpcmActive = 0;

			// Fighting Force 2 - update subq time immediately
			// - fixes new game
			UpdateSubq(cdr.SetSectorPlay);
			cdr.LocL[0] = LOCL_INVALID;
			cdr.SubqForwardSectors = 1;
			cdr.sectorsRead = 0;
			cdr.DriveState = DRIVESTATE_SEEK;
			cdr.PhysCdPropagations = 0;
			cdra_prefetch(cdr.SetSectorPlay[0], cdr.SetSectorPlay[1],
					cdr.SetSectorPlay[2]);

			cycles = (cdr.Mode & MODE_SPEED) ? cdReadTime : cdReadTime * 2;
			cycles += seekTime;
			if (Config.hacks.cdr_read_timing)
				cycles = cdrAlignTimingHack(cycles);
			else if (canDoTurbo())
				cycles = cdReadTime / 2;
			CDRPLAYREAD_INT(cycles, 1);

			SetPlaySeekRead(cdr.StatP, STATUS_SEEK);
			start_rotating = 1;
			break;

		case CdlSync:
		default:
			error = ERROR_INVALIDCMD;
			// FALLTHROUGH

		set_error:
			SetResultSize_(2);
			cdr.Result[0] = cdr.StatP | STATUS_ERROR;
			cdr.Result[1] = not_ready ? ERROR_NOTREADY : error;
			IrqStat = DiskError;
			CDR_LOG_I("cmd %02x error %02x\n", Cmd, cdr.Result[1]);
			break;
	}

	if (cdr.DriveState == DRIVESTATE_STOPPED && start_rotating) {
		cdr.DriveState = DRIVESTATE_STANDBY;
		cdr.StatP |= STATUS_ROTATING;
	}

	if (second_resp_time) {
		cdr.CmdInProgress = Cmd | 0x100;
		set_event(PSXINT_CDR, second_resp_time);
	}
	else if (cdr.Cmd && cdr.Cmd != (Cmd & 0xff)) {
		cdr.CmdInProgress = cdr.Cmd;
		CDR_LOG_I("cmd %02x came before %02x finished\n", cdr.Cmd, Cmd);
	}

	setIrq(IrqStat, Cmd);
}

static void cdrPrepCdda(s16 *buf, int samples)
{
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	int i;
	for (i = 0; i < samples; i++) {
		buf[i * 2 + 0] = SWAP16(buf[i * 2 + 0]);
		buf[i * 2 + 1] = SWAP16(buf[i * 2 + 1]);
	}
#endif
}

static void cdrReadInterruptSetResult(unsigned char result)
{
	if (cdr.IrqStat) {
		CDR_LOG_I("%d:%02d:%02d irq miss, cmd=%02x irqstat=%02x\n",
			cdr.SetSectorPlay[0], cdr.SetSectorPlay[1], cdr.SetSectorPlay[2],
			cdr.CmdInProgress, cdr.IrqStat);
		cdr.Irq1Pending = result;
		// F1 2000 timing hack :(
		// compensate for some csum func @80014380 taking too long
		if (!cdr.AdpcmActive)
			psxRegs.intCycle[PSXINT_CDREAD].sCycle += cdReadTime / 10;
		return;
	}
	SetResultSize(1);
	cdr.Result[0] = result;
	setIrq((result & STATUS_ERROR) ? DiskError : DataReady, 0x1004);
}

static void cdrUpdateTransferBuf(const u8 *buf)
{
	if (!buf)
		return;
	memcpy(cdr.Transfer, buf, DATA_SIZE);
	CheckPPFCache(cdr.Transfer, cdr.Prev[0], cdr.Prev[1], cdr.Prev[2]);
	CDR_LOG("cdr.Transfer  %02x:%02x:%02x\n",
		cdr.Transfer[0], cdr.Transfer[1], cdr.Transfer[2]);
	if (cdr.FifoOffset < 2048 + 12)
		CDR_LOG("FifoOffset(1) %d/%d\n", cdr.FifoOffset, cdr.FifoSize);
}

static void cdrReadInterrupt(void)
{
	const struct { u8 file, chan, mode, coding; } *subhdr;
	const u8 *buf = NULL;
	int deliver_data = 1;
	u8 subqPos[3];
	int read_ok;
	int is_start;

	memcpy(subqPos, cdr.SetSectorPlay, sizeof(subqPos));
	msfiAdd(subqPos, cdr.SubqForwardSectors);
	UpdateSubq(subqPos);
	if (cdr.SubqForwardSectors < SUBQ_FORWARD_SECTORS) {
		cdr.SubqForwardSectors++;
		CDRPLAYREAD_INT((cdr.Mode & MODE_SPEED) ? (cdReadTime / 2) : cdReadTime, 0);
		return;
	}

	// note: CdlGetlocL should work as soon as STATUS_READ is indicated
	SetPlaySeekRead(cdr.StatP, STATUS_READ | STATUS_ROTATING);
	cdr.DriveState = DRIVESTATE_PLAY_READ;
	cdr.sectorsRead++;

	read_ok = ReadTrack(cdr.SetSectorPlay);
	if (read_ok)
		buf = cdra_getBuffer();
	if (buf == NULL)
		read_ok = 0;

	if (!read_ok) {
		CDR_LOG_I("cdrReadInterrupt() Log: err\n");
		cdrReadInterruptSetResult(cdr.StatP | STATUS_ERROR);
		cdr.DriveState = DRIVESTATE_PAUSED; // ?
		return;
	}
	memcpy(cdr.LocL, buf, 8);

	if (!cdr.IrqStat && !cdr.Irq1Pending)
		cdrUpdateTransferBuf(buf);

	subhdr = (void *)(buf + 4);
	do {
		// try to process as adpcm
		if (!(cdr.Mode & MODE_STRSND))
			break;
		if (buf[3] != 2 || (subhdr->mode & 0x44) != 0x44) // or 0x64?
			break;
		CDR_LOG("f=%d m=%d %d,%3d | %d,%2d | %d,%2d\n", !!(cdr.Mode & MODE_SF), cdr.Muted,
			subhdr->file, subhdr->chan, cdr.CurFile, cdr.CurChannel, cdr.FilterFile, cdr.FilterChannel);
		if ((cdr.Mode & MODE_SF) && (subhdr->file != cdr.FilterFile || subhdr->chan != cdr.FilterChannel))
			break;
		if (subhdr->chan & 0x80) { // ?
			if (subhdr->chan != 0xff)
				log_unhandled("adpcm %d:%d\n", subhdr->file, subhdr->chan);
			break;
		}
		if (!cdr.FileChannelSelected) {
			cdr.CurFile = subhdr->file;
			cdr.CurChannel = subhdr->chan;
			cdr.FileChannelSelected = 1;
		}
		else if (subhdr->file != cdr.CurFile || subhdr->chan != cdr.CurChannel)
			break;

		// accepted as adpcm
		deliver_data = 0;

		if (Config.Xa)
			break;
		is_start = !cdr.AdpcmActive;
		cdr.AdpcmActive = !xa_decode_sector(&cdr.Xa, buf + 4, is_start);
		if (cdr.AdpcmActive)
			SPU_playADPCMchannel(&cdr.Xa, psxRegs.cycle, is_start);
	} while (0);

	if ((cdr.Mode & MODE_SF) && (subhdr->mode & 0x44) == 0x44) // according to nocash
		deliver_data = 0;
	if (buf[3] != 1 && buf[3] != 2) { // according to duckstation
		deliver_data = 0;
		CDR_LOG_I("%x:%02x:%02x mode %02x ignored\n",
			buf[0], buf[1], buf[2], buf[3]);
	}

	/*
	Croc 2: $40 - only FORM1 (*)
	Judge Dredd: $C8 - only FORM1 (*)
	Sim Theme Park - no adpcm at all (zero)
	*/

	if (deliver_data)
		cdrReadInterruptSetResult(cdr.StatP);

	msfiAdd(cdr.SetSectorPlay, 1);
	cdra_prefetch(cdr.SetSectorPlay[0], cdr.SetSectorPlay[1], cdr.SetSectorPlay[2]);

	CDRPLAYREAD_INT((cdr.Mode & MODE_SPEED) ? (cdReadTime / 2) : cdReadTime, 0);
}

/*
cdrRead0:
	bit 0,1 - reg index
	bit 2 - adpcm active
	bit 3 - 1 parameter fifo empty
	bit 4 - 1 parameter fifo not full
	bit 5 - 1 response fifo not empty
	bit 6 - 1 data fifo not empty
	bit 7 - 1 command being processed
*/

unsigned char cdrRead0(void) {
	cdr.Ctrl &= ~0x64;
	cdr.Ctrl |= cdr.AdpcmActive << 2;
	cdr.Ctrl |= cdr.ResultReady << 5;
	cdr.Ctrl |= ((signed int)(cdr.FifoOffset - cdr.FifoSize) >> 31) & 0x40;

	cdr.Ctrl |= 0x18;

	CDR_LOG_IO("cdr r0.sta: %02x\n", cdr.Ctrl);

	return psxHu8(0x1800) = cdr.Ctrl;
}

void cdrWrite0(unsigned char rt) {
	CDR_LOG_IO("cdr w0.x.idx: %02x\n", rt);

	cdr.Ctrl = (rt & 3) | (cdr.Ctrl & ~3);
}

unsigned char cdrRead1(void) {
	if ((cdr.ResultP & 0xf) < cdr.ResultC)
		psxHu8(0x1801) = cdr.Result[cdr.ResultP & 0xf];
	else
		psxHu8(0x1801) = 0;
	cdr.ResultP++;
	if (cdr.ResultP == cdr.ResultC)
		cdr.ResultReady = 0;

	CDR_LOG_IO("cdr r1.x.rsp: %02x #%u\n", psxHu8(0x1801), cdr.ResultP - 1);

	return psxHu8(0x1801);
}

void cdrWrite1(unsigned char rt) {
	const char *rnames[] = { "0.cmd", "1.smd", "2.smc", "3.arr" }; (void)rnames;
	CDR_LOG_IO("cdr w1.%s: %02x\n", rnames[cdr.Ctrl & 3], rt);

	switch (cdr.Ctrl & 3) {
	case 0:
		break;
	case 3:
		cdr.AttenuatorRightToRightT = rt;
		return;
	default:
		return;
	}

#ifdef CDR_LOG_CMD_IRQ
	CDR_LOG_I("CD1 write: %x (%s)", rt, CmdName[rt]);
	if (cdr.ParamC) {
		int i;
		SysPrintf(" Param[%d] = {", cdr.ParamC);
		for (i = 0; i < cdr.ParamC; i++)
			SysPrintf(" %x,", cdr.Param[i]);
		SysPrintf("}");
	}
	SysPrintf(" @%08x\n", psxRegs.pc);
#endif

	cdr.ResultReady = 0;
	cdr.Ctrl |= 0x80;

	if (!cdr.CmdInProgress) {
		cdr.CmdInProgress = rt;
		// should be something like 12k + controller delays
		set_event(PSXINT_CDR, 5000);
	}
	else {
		CDR_LOG_I("cmd while busy: %02x, prev %02x, busy %02x\n",
			rt, cdr.Cmd, cdr.CmdInProgress);
		if (cdr.CmdInProgress < 0x100) // no pending 2nd response
			cdr.CmdInProgress = rt;
	}

	cdr.Cmd = rt;
}

unsigned char cdrRead2(void) {
	unsigned char ret = cdr.Transfer[0x920];

	if (cdr.FifoOffset < cdr.FifoSize)
		ret = cdr.Transfer[cdr.FifoOffset++];
	else
		CDR_LOG_I("read empty fifo (%d)\n", cdr.FifoSize);

	CDR_LOG_IO("cdr r2.x.dat: %02x\n", ret);
	return ret;
}

void cdrWrite2(unsigned char rt) {
	const char *rnames[] = { "0.prm", "1.ien", "2.all", "3.arl" }; (void)rnames;
	CDR_LOG_IO("cdr w2.%s: %02x\n", rnames[cdr.Ctrl & 3], rt);

	switch (cdr.Ctrl & 3) {
	case 0:
		if (cdr.ParamC < 8) // FIXME: size and wrapping
			cdr.Param[cdr.ParamC++] = rt;
		return;
	case 1:
		cdr.IrqMask = rt;
		setIrq(cdr.IrqStat, 0x1005);
		return;
	case 2:
		cdr.AttenuatorLeftToLeftT = rt;
		return;
	case 3:
		cdr.AttenuatorRightToLeftT = rt;
		return;
	}
}

unsigned char cdrRead3(void) {
	if (cdr.Ctrl & 0x1)
		psxHu8(0x1803) = cdr.IrqStat | 0xE0;
	else
		psxHu8(0x1803) = cdr.IrqMask | 0xE0;

	CDR_LOG_IO("cdr r3.%d.%s: %02x\n", cdr.Ctrl & 3,
		(cdr.Ctrl & 1) ? "ifl" : "ien", psxHu8(0x1803));
	return psxHu8(0x1803);
}

void cdrWrite3(unsigned char rt) {
	const char *rnames[] = { "0.req", "1.ifl", "2.alr", "3.ava" }; (void)rnames;
	u8 ll, lr, rl, rr;
	CDR_LOG_IO("cdr w3.%s: %02x\n", rnames[cdr.Ctrl & 3], rt);

	switch (cdr.Ctrl & 3) {
	case 0:
		break; // transfer
	case 1:
		if (cdr.IrqStat & rt) {
			u32 nextCycle = psxRegs.intCycle[PSXINT_CDR].sCycle
				+ psxRegs.intCycle[PSXINT_CDR].cycle;
			int pending = psxRegs.interrupt & (1 << PSXINT_CDR);
#ifdef CDR_LOG_CMD_IRQ
			CDR_LOG_I("ack %02x (w=%02x p=%d,%x,%x,%d)\n",
				cdr.IrqStat & rt, rt, !!pending, cdr.CmdInProgress,
				cdr.Irq1Pending, nextCycle - psxRegs.cycle);
#endif
			// note: Croc, Shadow Tower (more) vs Discworld Noir (<993)
			if (!pending && (cdr.CmdInProgress || cdr.Irq1Pending))
			{
				s32 c = 2048;
				if (cdr.CmdInProgress) {
					c = 2048 - (psxRegs.cycle - nextCycle);
					c = MAX_VALUE(c, 512);
				}
				set_event(PSXINT_CDR, c);
			}
		}
		cdr.IrqStat &= ~rt;

		if (rt & 0x40)
			cdr.ParamC = 0;
		return;
	case 2:
		cdr.AttenuatorLeftToRightT = rt;
		return;
	case 3:
		if (rt & 0x01)
			log_unhandled("Mute ADPCM?\n");
		if (rt & 0x20) {
			ll = cdr.AttenuatorLeftToLeftT; lr = cdr.AttenuatorLeftToRightT;
			rl = cdr.AttenuatorRightToLeftT; rr = cdr.AttenuatorRightToRightT;
			if (ll == cdr.AttenuatorLeftToLeft &&
			    lr == cdr.AttenuatorLeftToRight &&
			    rl == cdr.AttenuatorRightToLeft &&
			    rr == cdr.AttenuatorRightToRight)
				return;
			cdr.AttenuatorLeftToLeft = ll; cdr.AttenuatorLeftToRight = lr;
			cdr.AttenuatorRightToLeft = rl; cdr.AttenuatorRightToRight = rr;
			CDR_LOG_I("CD-XA Volume: %02x %02x | %02x %02x\n", ll, lr, rl, rr);
			SPU_setCDvol(ll, lr, rl, rr, psxRegs.cycle);
		}
		return;
	}

	// test: Viewpoint
	if ((rt & 0x80) && cdr.FifoOffset < cdr.FifoSize) {
		CDR_LOG("cdrom: FifoOffset(2) %d/%d\n", cdr.FifoOffset, cdr.FifoSize);
	}
	else if (rt & 0x80) {
		switch (cdr.Mode & (MODE_SIZE_2328|MODE_SIZE_2340)) {
			case MODE_SIZE_2328:
			case 0x00:
				cdr.FifoOffset = 12;
				cdr.FifoSize = 2048 + 12;
				break;

			case MODE_SIZE_2340:
			default:
				cdr.FifoOffset = 0;
				cdr.FifoSize = 2340;
				break;
		}
	}
	else if (!(rt & 0xc0))
		cdr.FifoOffset = DATA_SIZE; // fifo empty
}

void psxDma3(u32 madr, u32 bcr, u32 chcr) {
	u32 cdsize, max_words, cycles;
	int size;
	u8 *ptr;

#if 0
	CDR_LOG_I("psxDma3() Log: *** DMA 3 *** %x addr = %x size = %x", chcr, madr, bcr);
	if (cdr.FifoOffset == 0) {
		ptr = cdr.Transfer;
		SysPrintf(" %02x:%02x:%02x", ptr[0], ptr[1], ptr[2]);
	}
	SysPrintf("\n");
#endif

	switch (chcr & 0x71000000) {
		case 0x11000000:
			madr &= ~3;
			ptr = getDmaRam(madr, &max_words);
			if (ptr == INVALID_PTR) {
				CDR_LOG_I("psxDma3() Log: *** DMA 3 *** NULL Pointer!\n");
				break;
			}

			cdsize = (((bcr - 1) & 0xffff) + 1) * 4;

			/*
			GS CDX: Enhancement CD crash
			- Setloc 0:0:0
			- CdlPlay
			- Spams DMA3 and gets buffer overrun
			*/
			size = DATA_SIZE - cdr.FifoOffset;
			if (size > cdsize)
				size = cdsize;
			if (size > max_words * 4)
				size = max_words * 4;
			if (size > 0)
			{
				memcpy(ptr, cdr.Transfer + cdr.FifoOffset, size);
				cdr.FifoOffset += size;
			}
			if (size < cdsize) {
				CDR_LOG_I("cdrom: dma3 %d/%d\n", size, cdsize);
				memset(ptr + size, cdr.Transfer[0x920], cdsize - size);
			}
			psxCpu->Clear(madr, cdsize / 4);

			cycles = (cdsize / 4) * 24;
			set_event(PSXINT_CDRDMA, cycles);

			HW_DMA3_CHCR &= SWAPu32(~0x10000000);
			if (chcr & 0x100) {
				HW_DMA3_MADR = SWAPu32(madr + cdsize);
				HW_DMA3_BCR &= SWAPu32(0xffff0000);
			}
			else {
				// halted
				psxRegs.cycle += cycles - 20;
			}
			if (canDoTurbo() && cdr.Reading && cdr.FifoOffset >= 2048)
				CDRPLAYREAD_INT(cycles + 4096, 1);
			return;

		default:
			CDR_LOG_I("psxDma3() Log: Unknown cddma %x\n", chcr);
			break;
	}

	HW_DMA3_CHCR &= SWAP32(~0x01000000);
	DMA_INTERRUPT(3);
}

void cdrDmaInterrupt(void)
{
	if (HW_DMA3_CHCR & SWAP32(0x01000000))
	{
		HW_DMA3_CHCR &= SWAP32(~0x01000000);
		DMA_INTERRUPT(3);
	}
}

static void getCdInfo(void)
{
	cdra_getTN(cdr.ResultTN);
	cdra_getTD(0, cdr.SetSectorEnd);
}

void cdrReset() {
	memset(&cdr, 0, sizeof(cdr));
	cdr.CurTrack = 1;
	cdr.FilterFile = 0;
	cdr.FilterChannel = 0;
	cdr.IrqMask = 0x1f;
	cdr.IrqStat = NoIntr;

	// BIOS player - default values
	cdr.AttenuatorLeftToLeft = 0x80;
	cdr.AttenuatorLeftToRight = 0x00;
	cdr.AttenuatorRightToLeft = 0x00;
	cdr.AttenuatorRightToRight = 0x80;

	softReset();
	getCdInfo();
}

int cdrFreeze(void *f, int Mode) {
	u32 tmp;
	u8 tmpp[3];

	cdr.freeze_ver = 0x63647203;
	gzfreeze(&cdr, sizeof(cdr));
	
	if (Mode == 1) {
		cdr.ParamP = cdr.ParamC;
		tmp = cdr.FifoOffset;
	}

	gzfreeze(&tmp, sizeof(tmp));

	if (Mode == 0) {
		u8 ll = 0, lr = 0, rl = 0, rr = 0;
		getCdInfo();

		cdr.FifoOffset = tmp < DATA_SIZE ? tmp : DATA_SIZE;
		cdr.FifoSize = (cdr.Mode & MODE_SIZE_2340) ? 2340 : 2048 + 12;
		if (cdr.SubqForwardSectors > SUBQ_FORWARD_SECTORS)
			cdr.SubqForwardSectors = SUBQ_FORWARD_SECTORS;

		// read right sub data
		memcpy(tmpp, cdr.Prev, sizeof(tmpp));
		if (cdr.freeze_ver < 0x63647203) {
			tmpp[0] = btoi(tmpp[0]);
			tmpp[1] = btoi(tmpp[1]);
			tmpp[2] = btoi(tmpp[2]);
		}
		cdr.Prev[0] = 0xff;
		if (tmpp[0] != 0xff)
			ReadTrack(tmpp);

		if (cdr.Play) {
			if (cdr.freeze_ver < 0x63647202)
				memcpy(cdr.SetSectorPlay, cdr.SetSector, 3);

			Find_CurTrack(cdr.SetSectorPlay);
		}
		if (!cdr.Muted)
			ll = cdr.AttenuatorLeftToLeft, lr = cdr.AttenuatorLeftToRight,
			rl = cdr.AttenuatorRightToLeft, rr = cdr.AttenuatorRightToRight;
		SPU_setCDvol(ll, lr, rl, rr, psxRegs.cycle);
	}

	return 0;
}

void LidInterrupt(void) {
	getCdInfo();
	cdrLidSeekInterrupt();
}
