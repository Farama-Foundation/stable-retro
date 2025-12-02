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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02111-1307 USA.           *
 ***************************************************************************/

/*
* Plugin library callback/access functions.
*/

#include "plugins.h"
#include "cdriso.h"
#include "cdrom-async.h"
#include "psxcounters.h"

static char IsoFile[MAXPATHLEN] = "";
static s64 cdOpenCaseTime = 0;

GPUupdateLace         GPU_updateLace;
GPUinit               GPU_init;
GPUshutdown           GPU_shutdown;
GPUopen               GPU_open;
GPUclose              GPU_close;
GPUreadStatus         GPU_readStatus;
GPUreadData           GPU_readData;
GPUreadDataMem        GPU_readDataMem;
GPUwriteStatus        GPU_writeStatus;
GPUwriteData          GPU_writeData;
GPUwriteDataMem       GPU_writeDataMem;
GPUdmaChain           GPU_dmaChain;
GPUfreeze             GPU_freeze;
GPUvBlank             GPU_vBlank;
GPUgetScreenInfo      GPU_getScreenInfo;

SPUinit               SPU_init;
SPUshutdown           SPU_shutdown;
SPUopen               SPU_open;
SPUclose              SPU_close;
SPUwriteRegister      SPU_writeRegister;
SPUreadRegister       SPU_readRegister;
SPUwriteDMAMem        SPU_writeDMAMem;
SPUreadDMAMem         SPU_readDMAMem;
SPUplayADPCMchannel   SPU_playADPCMchannel;
SPUfreeze             SPU_freeze;
SPUregisterCallback   SPU_registerCallback;
SPUregisterScheduleCb SPU_registerScheduleCb;
SPUasync              SPU_async;
SPUplayCDDAchannel    SPU_playCDDAchannel;
SPUsetCDvol           SPU_setCDvol;

#ifdef ENABLE_SIO1API

SIO1init              SIO1_init;
SIO1shutdown          SIO1_shutdown;
SIO1open              SIO1_open;
SIO1close             SIO1_close;
SIO1test              SIO1_test;
SIO1configure         SIO1_configure;
SIO1about             SIO1_about;
SIO1pause             SIO1_pause;
SIO1resume            SIO1_resume;
SIO1keypressed        SIO1_keypressed;
SIO1writeData8        SIO1_writeData8;
SIO1writeData16       SIO1_writeData16;
SIO1writeData32       SIO1_writeData32;
SIO1writeStat16       SIO1_writeStat16;
SIO1writeStat32       SIO1_writeStat32;
SIO1writeMode16       SIO1_writeMode16;
SIO1writeMode32       SIO1_writeMode32;
SIO1writeCtrl16       SIO1_writeCtrl16;
SIO1writeCtrl32       SIO1_writeCtrl32;
SIO1writeBaud16       SIO1_writeBaud16;
SIO1writeBaud32       SIO1_writeBaud32;
SIO1readData8         SIO1_readData8;
SIO1readData16        SIO1_readData16;
SIO1readData32        SIO1_readData32;
SIO1readStat16        SIO1_readStat16;
SIO1readStat32        SIO1_readStat32;
SIO1readMode16        SIO1_readMode16;
SIO1readMode32        SIO1_readMode32;
SIO1readCtrl16        SIO1_readCtrl16;
SIO1readCtrl32        SIO1_readCtrl32;
SIO1readBaud16        SIO1_readBaud16;
SIO1readBaud32        SIO1_readBaud32;
SIO1registerCallback  SIO1_registerCallback;

#endif

static const char *err;

#define CheckErr(func) { \
	err = SysLibError(); \
	if (err != NULL) { SysMessage(_("Error loading %s: %s"), func, err); return -1; } \
}

#define LoadSym(dest, src, name, checkerr) { \
	dest = (src)SysLoadSym(drv, name); \
	if (checkerr) { CheckErr(name); } \
}

void *hGPUDriver = NULL;

static void CALLBACK GPU__vBlank(int val) {}
static void CALLBACK GPU__getScreenInfo(int *y, int *base_hres) {}

#define LoadGpuSym1(dest, name) \
	LoadSym(GPU_##dest, GPU##dest, name, TRUE);

#define LoadGpuSym0(dest, name) \
	LoadSym(GPU_##dest, GPU##dest, name, FALSE); \
	if (GPU_##dest == NULL) GPU_##dest = (GPU##dest) GPU__##dest;

#define LoadGpuSymN(dest, name) \
	LoadSym(GPU_##dest, GPU##dest, name, FALSE);

static int LoadGPUplugin(const char *GPUdll) {
	void *drv;

	hGPUDriver = SysLoadLibrary(GPUdll);
	if (hGPUDriver == NULL) {
		SysMessage (_("Could not load GPU plugin %s!"), GPUdll); return -1;
	}
	drv = hGPUDriver;
	LoadGpuSym1(init, "GPUinit");
	LoadGpuSym1(shutdown, "GPUshutdown");
	LoadGpuSym1(open, "GPUopen");
	LoadGpuSym1(close, "GPUclose");
	LoadGpuSym1(readData, "GPUreadData");
	LoadGpuSym1(readDataMem, "GPUreadDataMem");
	LoadGpuSym1(readStatus, "GPUreadStatus");
	LoadGpuSym1(writeData, "GPUwriteData");
	LoadGpuSym1(writeDataMem, "GPUwriteDataMem");
	LoadGpuSym1(writeStatus, "GPUwriteStatus");
	LoadGpuSym1(dmaChain, "GPUdmaChain");
	LoadGpuSym1(updateLace, "GPUupdateLace");
	LoadGpuSym1(freeze, "GPUfreeze");
	LoadGpuSym0(vBlank, "GPUvBlank");
	LoadGpuSym0(getScreenInfo, "GPUgetScreenInfo");

	return 0;
}

int CDR__getStatus(struct CdrStat *stat) {
	if (cdOpenCaseTime < 0 || cdOpenCaseTime > (s64)time(NULL))
		stat->Status = 0x10;
	else
		stat->Status = 0;

	return 0;
}

static void *hSPUDriver = NULL;
static void CALLBACK SPU__registerScheduleCb(void (CALLBACK *cb)(unsigned int)) {}
static void CALLBACK SPU__setCDvol(unsigned char ll, unsigned char lr,
		unsigned char rl, unsigned char rr, unsigned int cycle) {}

#define LoadSpuSym1(dest, name) \
	LoadSym(SPU_##dest, SPU##dest, name, TRUE);

#define LoadSpuSym0(dest, name) \
	LoadSym(SPU_##dest, SPU##dest, name, FALSE); \
	if (SPU_##dest == NULL) SPU_##dest = SPU__##dest;

#define LoadSpuSymN(dest, name) \
	LoadSym(SPU_##dest, SPU##dest, name, FALSE);

static int LoadSPUplugin(const char *SPUdll) {
	void *drv;

	hSPUDriver = SysLoadLibrary(SPUdll);
	if (hSPUDriver == NULL) {
		SysMessage (_("Could not load SPU plugin %s!"), SPUdll); return -1;
	}
	drv = hSPUDriver;
	LoadSpuSym1(init, "SPUinit");
	LoadSpuSym1(shutdown, "SPUshutdown");
	LoadSpuSym1(open, "SPUopen");
	LoadSpuSym1(close, "SPUclose");
	LoadSpuSym1(writeRegister, "SPUwriteRegister");
	LoadSpuSym1(readRegister, "SPUreadRegister");
	LoadSpuSym1(writeDMAMem, "SPUwriteDMAMem");
	LoadSpuSym1(readDMAMem, "SPUreadDMAMem");
	LoadSpuSym1(playADPCMchannel, "SPUplayADPCMchannel");
	LoadSpuSym1(freeze, "SPUfreeze");
	LoadSpuSym1(registerCallback, "SPUregisterCallback");
	LoadSpuSym0(registerScheduleCb, "SPUregisterScheduleCb");
	LoadSpuSymN(async, "SPUasync");
	LoadSpuSymN(playCDDAchannel, "SPUplayCDDAchannel");
	LoadSpuSym0(setCDvol, "SPUsetCDvol");

	return 0;
}

extern int in_type[8];

// Pad information, keystate, mode, config mode, vibration
static PadDataS pads[8];

static int reqPos, respSize;

static unsigned char buf[256];

static unsigned char stdpar[8] = { 0x41, 0x5a, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

//response for request 44, 45, 46, 47, 4C, 4D
static const u8 resp45[8]    = {0xF3, 0x5A, 0x01, 0x02, 0x00, 0x02, 0x01, 0x00};
static const u8 resp46_00[8] = {0xF3, 0x5A, 0x00, 0x00, 0x01, 0x02, 0x00, 0x0A};
static const u8 resp46_01[8] = {0xF3, 0x5A, 0x00, 0x00, 0x01, 0x01, 0x01, 0x14};
static const u8 resp47[8]    = {0xF3, 0x5A, 0x00, 0x00, 0x02, 0x00, 0x01, 0x00};
static const u8 resp4C_00[8] = {0xF3, 0x5A, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00};
static const u8 resp4C_01[8] = {0xF3, 0x5A, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00};

//fixed reponse of request number 41, 48, 49, 4A, 4B, 4E, 4F
static const u8 resp40[8] = {0xF3, 0x5A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const u8 resp41[8] = {0xF3, 0x5A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const u8 resp43[8] = {0xF3, 0x5A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const u8 resp44[8] = {0xF3, 0x5A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const u8 resp49[8] = {0xF3, 0x5A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const u8 resp4A[8] = {0xF3, 0x5A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const u8 resp4B[8] = {0xF3, 0x5A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const u8 resp4E[8] = {0xF3, 0x5A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const u8 resp4F[8] = {0xF3, 0x5A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// Resquest of psx core
enum {
	// REQUEST
	// first call of this request for the pad, the pad is configured as an digital pad.
	// 0x0X, 0x42, 0x0Y, 0xZZ, 0xAA, 0x00, 0x00, 0x00, 0x00
	// X pad number (used for the multitap, first request response 0x00, 0x80, 0x5A, (8 bytes pad A), (8 bytes pad B), (8 bytes pad C), (8 bytes pad D)
	// Y if 1 : psx request the full length response for the multitap, 3 bytes header and 4 block of 8 bytes per pad
	// Y if 0 : psx request a pad key state
	// ZZ rumble small motor 00-> OFF, 01 -> ON
	// AA rumble large motor speed 0x00 -> 0xFF
	// RESPONSE
	// header 3 Bytes
	// 0x00
	// PadId -> 0x41 for digital pas, 0x73 for analog pad
	// 0x5A mode has not change (no press on analog button on the center of pad), 0x00 the analog button have been pressed and the mode switch
	// 6 Bytes for keystates
	CMD_READ_DATA_AND_VIBRATE = 0x42,

	// REQUEST
	// Header
	// 0x0N, 0x43, 0x00, XX, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	// XX = 00 -> Normal mode : Seconde bytes of response = padId
	// XX = 01 -> Configuration mode : Seconde bytes of response = 0xF3
	// RESPONSE
	// enter in config mode example :
	// req : 01 43 00 01 00 00 00 00 00 00
	// res : 00 41 5A buttons state, analog states
	// exit config mode :
	// req : 01 43 00 00 00 00 00 00 00 00
	// res : 00 F3 5A buttons state, analog states
	CMD_CONFIG_MODE = 0x43,

	// Set led State
	// REQUEST
	// 0x0N, 0x44, 0x00, VAL, SEL, 0x00, 0x00, 0x00, 0x00
	// If sel = 2 then
	// VAL = 00 -> OFF
	// VAL = 01 -> ON
	// RESPONSE
	// 0x00, 0xF3, 0x5A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	CMD_SET_MODE_AND_LOCK = 0x44,

	// Get Analog Led state
	// REQUEST
	// 0x0N, 0x45, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	// RESPONSE
	// 0x00, 0xF3, 0x5A, 0x01, 0x02, VAL, 0x02, 0x01, 0x00
	// VAL = 00 Led OFF
	// VAL = 01 Led ON
	CMD_QUERY_MODEL_AND_MODE = 0x45,

	//Get Variable A
	// REQUEST
	// 0x0N, 0x46, 0x00, 0xXX, 0x00, 0x00, 0x00, 0x00, 0x00
	// RESPONSE
	// XX=00
	// 0x00, 0xF3, 0x5A, 0x00, 0x00, 0x01, 0x02, 0x00, 0x0A
	// XX=01
	// 0x00, 0xF3, 0x5A, 0x00, 0x00, 0x01, 0x01, 0x01, 0x14
	CMD_QUERY_ACT = 0x46,

	// REQUEST
	// 0x0N, 0x47, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	// RESPONSE
	// 0x00, 0xF3, 0x5A, 0x00, 0x00, 0x02, 0x00, 0x01, 0x00
	CMD_QUERY_COMB = 0x47,

	// REQUEST
	// 0x0N, 0x4C, 0x00, 0xXX, 0x00, 0x00, 0x00, 0x00, 0x00
	// RESPONSE
	// XX = 0
	// 0x00, 0xF3, 0x5A, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00
	// XX = 1
	// 0x00, 0xF3, 0x5A, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00
	CMD_QUERY_MODE = 0x4C,

	// REQUEST
	// 0x0N, 0x4D, 0x00, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
	// RESPONSE
	// 0x00, 0xF3, 0x5A, old value or
	// AA = 01 unlock large motor (and swap VAL1 and VAL2)
	// BB = 01 unlock large motor (default)
	// CC, DD, EE, FF = all FF -> unlock small motor
	//
	// default repsonse for analog pad with 2 motor : 0x00 0xF3 0x5A 0x00 0x01 0xFF 0xFF 0xFF 0xFF
	//
	CMD_VIBRATION_TOGGLE = 0x4D,
	REQ40 = 0x40,
	REQ41 = 0x41,
	REQ49 = 0x49,
	REQ4A = 0x4A,
	REQ4B = 0x4B,
	REQ4E = 0x4E,
	REQ4F = 0x4F
};


static void initBufForRequest(int padIndex, char value) {
	if (pads[padIndex].ds.configMode) {
		buf[0] = 0xf3; buf[1] = 0x5a;
		respSize = 8;
	}
	else if (value != 0x42 && value != 0x43) {
		respSize = 1;
		return;
	}

	if ((u32)(frame_counter - pads[padIndex].ds.lastUseFrame) > 2*60u
	    && pads[padIndex].ds.configModeUsed
	    && !Config.hacks.dualshock_init_analog)
	{
		//SysPrintf("Pad reset\n");
		pads[padIndex].ds.padMode = 0; // according to nocash
		pads[padIndex].ds.autoAnalogTried = 0;
	}
	else if (pads[padIndex].ds.padMode == 0 && value == CMD_READ_DATA_AND_VIBRATE
		 && pads[padIndex].ds.configModeUsed
		 && !pads[padIndex].ds.configMode
		 && !pads[padIndex].ds.userToggled)
	{
		if (pads[padIndex].ds.autoAnalogTried == 16) {
			// auto-enable for convenience
			SysPrintf("Auto-enabling dualshock analog mode.\n");
			pads[padIndex].ds.padMode = 1;
			pads[padIndex].ds.autoAnalogTried = 255;
		}
		else if (pads[padIndex].ds.autoAnalogTried < 16)
			pads[padIndex].ds.autoAnalogTried++;
	}
	pads[padIndex].ds.lastUseFrame = frame_counter;

	switch (value) {
		// keystate already in buffer, set by PADstartPoll_()
		//case CMD_READ_DATA_AND_VIBRATE :
		//	break;
		case CMD_CONFIG_MODE :
			if (pads[padIndex].ds.configMode) {
				memcpy(buf, resp43, 8);
				break;
			}
			// else not in config mode, pad keystate return
			break;
		case CMD_SET_MODE_AND_LOCK :
			memcpy(buf, resp44, 8);
			break;
		case CMD_QUERY_MODEL_AND_MODE :
			memcpy(buf, resp45, 8);
			buf[4] = pads[padIndex].ds.padMode;
			break;
		case CMD_QUERY_ACT :
			memcpy(buf, resp46_00, 8);
			break;
		case CMD_QUERY_COMB :
			memcpy(buf, resp47, 8);
			break;
		case CMD_QUERY_MODE :
			memcpy(buf, resp4C_00, 8);
			break;
		case CMD_VIBRATION_TOGGLE: // 4d
			memcpy(buf + 2, pads[padIndex].ds.cmd4dConfig, 6);
			break;
		case REQ40 :
			memcpy(buf, resp40, 8);
			break;
		case REQ41 :
			memcpy(buf, resp41, 8);
			break;
		case REQ49 :
			memcpy(buf, resp49, 8);
			break;
		case REQ4A :
			memcpy(buf, resp4A, 8);
			break;
		case REQ4B :
			memcpy(buf, resp4B, 8);
			break;
		case REQ4E :
			memcpy(buf, resp4E, 8);
			break;
		case REQ4F :
			memcpy(buf, resp4F, 8);
			break;
	}
}

static void reqIndex2Treatment(int padIndex, u8 value) {
	switch (pads[padIndex].txData[0]) {
		case CMD_CONFIG_MODE :
			//0x43
			if (value == 0) {
				pads[padIndex].ds.configMode = 0;
			} else if (value == 1) {
				pads[padIndex].ds.configMode = 1;
				pads[padIndex].ds.configModeUsed = 1;
			}
			break;
		case CMD_SET_MODE_AND_LOCK :
			//0x44 store the led state for change mode if the next value = 0x02
			//0x01 analog ON
			//0x00 analog OFF
			if ((value & ~1) == 0)
				pads[padIndex].ds.padMode = value;
			break;
		case CMD_QUERY_ACT :
			//0x46
			if (value == 1) {
				memcpy(buf, resp46_01, 8);
			}
			break;
		case CMD_QUERY_MODE :
			if (value == 1) {
				memcpy(buf, resp4C_01, 8);
			}
			break;
	}
}

static void ds_update_vibrate(int padIndex) {
	PadDataS *pad = &pads[padIndex];
	if (pad->ds.configModeUsed) {
		pad->Vib[0] = (pad->Vib[0] == 1) ? 1 : 0;
	}
	else {
		// compat mode
		pad->Vib[0] = (pad->Vib[0] & 0xc0) == 0x40 && (pad->Vib[1] & 1);
		pad->Vib[1] = 0;
	}
	if (pad->Vib[0] != pad->VibF[0] || pad->Vib[1] != pad->VibF[1]) {
		//value is different update Value and call libretro for vibration
		pad->VibF[0] = pad->Vib[0];
		pad->VibF[1] = pad->Vib[1];
		plat_trigger_vibrate(padIndex, pad->VibF[0], pad->VibF[1]);
		//printf("vib%i %02x %02x\n", padIndex, pad->VibF[0], pad->VibF[1]);
	}
}

static void log_pad(int port, int pos)
{
#if 0
	if (port == 0 && pos == respSize - 1) {
		int i;
		for (i = 0; i < respSize; i++)
			printf("%02x ", pads[port].txData[i]);
		printf("|");
		for (i = 0; i < respSize; i++)
			printf(" %02x", buf[i]);
		printf("\n");
	}
#endif
}

static void adjust_analog(unsigned char *b)
{
	// ff8 hates 0x80 for whatever reason (broken in 2d area menus),
	// or is this caused by something else we do wrong??
	// Also S.C.A.R.S. treats 0x7f as turning left.
	if (b[6] == 0x7f || b[6] == 0x80)
		b[6] = 0x81;
}

// Build response for 0x42 request Pad in port
static void PADstartPoll_(PadDataS *pad) {
	switch (pad->controllerType) {
		case PSE_PAD_TYPE_MOUSE:
			stdpar[0] = 0x12;
			stdpar[1] = 0x5a;
			stdpar[2] = pad->buttonStatus & 0xff;
			stdpar[3] = pad->buttonStatus >> 8;
			stdpar[4] = pad->moveX;
			stdpar[5] = pad->moveY;
			memcpy(buf, stdpar, 6);
			respSize = 6;
			break;
		case PSE_PAD_TYPE_NEGCON: // npc101/npc104(slph00001/slph00069)
			stdpar[0] = 0x23;
			stdpar[1] = 0x5a;
			stdpar[2] = pad->buttonStatus & 0xff;
			stdpar[3] = pad->buttonStatus >> 8;
			stdpar[4] = pad->rightJoyX;
			stdpar[5] = pad->rightJoyY;
			stdpar[6] = pad->leftJoyX;
			stdpar[7] = pad->leftJoyY;
			memcpy(buf, stdpar, 8);
			respSize = 8;
			break;
		case PSE_PAD_TYPE_GUNCON: // GUNCON - gun controller SLPH-00034 from Namco
			stdpar[0] = 0x63;
			stdpar[1] = 0x5a;
			stdpar[2] = pad->buttonStatus & 0xff;
			stdpar[3] = pad->buttonStatus >> 8;

			int absX = pad->absoluteX; // 0-1023
			int absY = pad->absoluteY;

			if (absX == 65536 || absY == 65536) {
				stdpar[4] = 0x01;
				stdpar[5] = 0x00;
				stdpar[6] = 0x0A;
				stdpar[7] = 0x00;
			}
			else {
				int y_ofs = 0, yres = 240;
				GPU_getScreenInfo(&y_ofs, &yres);
				int y_top = (Config.PsxType ? 0x30 : 0x19) + y_ofs;
				int w = Config.PsxType ? 385 : 378;
				int x = 0x40 + (w * absX >> 10);
				int y = y_top + (yres * absY >> 10);
				//printf("%3d %3d %4x %4x\n", absX, absY, x, y);

				stdpar[4] = x;
				stdpar[5] = x >> 8;
				stdpar[6] = y;
				stdpar[7] = y >> 8;
			}

			memcpy(buf, stdpar, 8);
			respSize = 8;
			break;
		case PSE_PAD_TYPE_GUN: // GUN CONTROLLER - gun controller SLPH-00014 from Konami
			stdpar[0] = 0x31;
			stdpar[1] = 0x5a;
			stdpar[2] = pad->buttonStatus & 0xff;
			stdpar[3] = pad->buttonStatus >> 8;
			memcpy(buf, stdpar, 4);
			respSize = 4;
			break;
		case PSE_PAD_TYPE_ANALOGPAD: // scph1150
			if (pad->ds.padMode == 0)
				goto standard;
			stdpar[0] = 0x73;
			stdpar[1] = 0x5a;
			stdpar[2] = pad->buttonStatus & 0xff;
			stdpar[3] = pad->buttonStatus >> 8;
			stdpar[4] = pad->rightJoyX;
			stdpar[5] = pad->rightJoyY;
			stdpar[6] = pad->leftJoyX;
			stdpar[7] = pad->leftJoyY;
			adjust_analog(stdpar);
			memcpy(buf, stdpar, 8);
			respSize = 8;
			break;
		case PSE_PAD_TYPE_ANALOGJOY: // scph1110
			stdpar[0] = 0x53;
			stdpar[1] = 0x5a;
			stdpar[2] = pad->buttonStatus & 0xff;
			stdpar[3] = pad->buttonStatus >> 8;
			stdpar[4] = pad->rightJoyX;
			stdpar[5] = pad->rightJoyY;
			stdpar[6] = pad->leftJoyX;
			stdpar[7] = pad->leftJoyY;
			adjust_analog(stdpar);
			memcpy(buf, stdpar, 8);
			respSize = 8;
			break;
		case PSE_PAD_TYPE_STANDARD:
		standard:
			stdpar[0] = 0x41;
			stdpar[1] = 0x5a;
			stdpar[2] = pad->buttonStatus & 0xff;
			stdpar[3] = pad->buttonStatus >> 8;
			memcpy(buf, stdpar, 4);
			respSize = 4;
			break;
		default:
			respSize = 0;
			break;
	}
}

static void PADpoll_dualshock(int port, unsigned char value, int pos)
{
	switch (pos) {
		case 0:
			initBufForRequest(port, value);
			break;
		case 2:
			reqIndex2Treatment(port, value);
			break;
		case 7:
			if (pads[port].txData[0] == CMD_VIBRATION_TOGGLE)
				memcpy(pads[port].ds.cmd4dConfig, pads[port].txData + 2, 6);
			break;
	}

	if (pads[port].txData[0] == CMD_READ_DATA_AND_VIBRATE
	    && !pads[port].ds.configModeUsed && 2 <= pos && pos < 4)
	{
		// "compat" single motor mode
		pads[port].Vib[pos - 2] = value;
	}
	else if (pads[port].txData[0] == CMD_READ_DATA_AND_VIBRATE
		 && 2 <= pos && pos < 8)
	{
		// 0 - weak motor, 1 - strong motor
		int dev = pads[port].ds.cmd4dConfig[pos - 2];
		if (dev < 2)
			pads[port].Vib[dev] = value;
	}
	if (pos == respSize - 1)
		ds_update_vibrate(port);
}

static unsigned char PADpoll_(int port, unsigned char value, int pos, int *more_data) {
	if (pos == 0 && value != 0x42 && in_type[port] != PSE_PAD_TYPE_ANALOGPAD)
		respSize = 1;

	switch (in_type[port]) {
		case PSE_PAD_TYPE_ANALOGPAD:
			PADpoll_dualshock(port, value, pos);
			break;
		case PSE_PAD_TYPE_GUN:
			if (pos == 2)
				pl_gun_byte2(port, value);
			break;
	}

	*more_data = pos < respSize - 1;
	if (pos >= respSize)
		return 0xff; // no response/HiZ

	log_pad(port, pos);
	return buf[pos];
}

// response: 0x80, 0x5A, 8 bytes each for ports A, B, C, D
static unsigned char PADpollMultitap(int port, unsigned char value, int pos, int *more_data) {
	unsigned int devByte, dev;
	int unused = 0;

	if (pos == 0) {
		*more_data = (value == 0x42);
		return 0x80;
	}
	*more_data = pos < 34 - 1;
	if (pos == 1)
		return 0x5a;
	if (pos >= 34)
		return 0xff;

	devByte = pos - 2;
	dev = devByte / 8;
	if (devByte % 8 == 0)
		PADstartPoll_(&pads[port + dev]);
	return PADpoll_(port + dev, value, devByte % 8, &unused);
}

static unsigned char PADpollMain(int port, unsigned char value, int *more_data) {
	unsigned char ret;
	int pos = reqPos++;

	if (pos < sizeof(pads[port].txData))
		pads[port].txData[pos] = value;
	if (!pads[port].portMultitap || !pads[port].multitapLongModeEnabled)
		ret = PADpoll_(port, value, pos, more_data);
	else
		ret = PADpollMultitap(port, value, pos, more_data);
	return ret;

}

// refresh the button state on port 1.
// int pad is not needed.
unsigned char PAD1_startPoll(int unused) {
	int i;

	reqPos = 0;
	pads[0].requestPadIndex = 0;
	PAD1_readPort(&pads[0]);

	pads[0].multitapLongModeEnabled = 0;
	if (pads[0].portMultitap)
		pads[0].multitapLongModeEnabled = pads[0].txData[1] & 1;

	if (!pads[0].portMultitap || !pads[0].multitapLongModeEnabled) {
		PADstartPoll_(&pads[0]);
	} else {
		// a multitap is plugged and enabled: refresh pads 1-3
		for (i = 1; i < 4; i++) {
			pads[i].requestPadIndex = i;
			PAD1_readPort(&pads[i]);
		}
	}
	return 0xff;
}

unsigned char PAD1_poll(unsigned char value, int *more_data) {
	return PADpollMain(0, value, more_data);
}


unsigned char PAD2_startPoll(int pad) {
	int pad_index = pads[0].portMultitap ? 4 : 1;
	int i;

	reqPos = 0;
	pads[pad_index].requestPadIndex = pad_index;
	PAD2_readPort(&pads[pad_index]);

	pads[pad_index].multitapLongModeEnabled = 0;
	if (pads[pad_index].portMultitap)
		pads[pad_index].multitapLongModeEnabled = pads[pad_index].txData[1] & 1;

	if (!pads[pad_index].portMultitap || !pads[pad_index].multitapLongModeEnabled) {
		PADstartPoll_(&pads[pad_index]);
	} else {
		for (i = 1; i < 4; i++) {
			pads[pad_index + i].requestPadIndex = pad_index + i;
			PAD2_readPort(&pads[pad_index + i]);
		}
	}
	return 0xff;
}

unsigned char PAD2_poll(unsigned char value, int *more_data) {
	return PADpollMain(pads[0].portMultitap ? 4 : 1, value, more_data);
}

static void PAD_init(void) {
	size_t p;

	memset(pads, 0, sizeof(pads));
	for (p = 0; p < sizeof(pads) / sizeof(pads[0]); p++) {
		memset(pads[p].ds.cmd4dConfig, 0xff, sizeof(pads[p].ds.cmd4dConfig));
	}
}

int padFreeze(void *f, int Mode) {
	size_t i;

	for (i = 0; i < sizeof(pads) / sizeof(pads[0]); i++) {
		pads[i].saveSize = sizeof(pads[i]);
		gzfreeze(&pads[i], sizeof(pads[i]));
		if (Mode == 0 && pads[i].saveSize != sizeof(pads[i]))
			SaveFuncs.seek(f, pads[i].saveSize - sizeof(pads[i]), SEEK_CUR);
	}

	return 0;
}

int padToggleAnalog(unsigned int index)
{
	int r = -1;

	if (index < sizeof(pads) / sizeof(pads[0])) {
		r = (pads[index].ds.padMode ^= 1);
		pads[index].ds.userToggled = 1;
	}
	return r;
}

#ifdef ENABLE_SIO1API

void *hSIO1Driver = NULL;

long CALLBACK SIO1__init(void) { return 0; }
long CALLBACK SIO1__shutdown(void) { return 0; }
long CALLBACK SIO1__open(void) { return 0; }
long CALLBACK SIO1__close(void) { return 0; }
long CALLBACK SIO1__configure(void) { return 0; }
long CALLBACK SIO1__test(void) { return 0; }
void CALLBACK SIO1__about(void) {}
void CALLBACK SIO1__pause(void) {}
void CALLBACK SIO1__resume(void) {}
long CALLBACK SIO1__keypressed(int key) { return 0; }
void CALLBACK SIO1__writeData8(unsigned char val) {}
void CALLBACK SIO1__writeData16(unsigned short val) {}
void CALLBACK SIO1__writeData32(unsigned long val) {}
void CALLBACK SIO1__writeStat16(unsigned short val) {}
void CALLBACK SIO1__writeStat32(unsigned long val) {}
void CALLBACK SIO1__writeMode16(unsigned short val) {}
void CALLBACK SIO1__writeMode32(unsigned long val) {}
void CALLBACK SIO1__writeCtrl16(unsigned short val) {}
void CALLBACK SIO1__writeCtrl32(unsigned long val) {}
void CALLBACK SIO1__writeBaud16(unsigned short val) {}
void CALLBACK SIO1__writeBaud32(unsigned long val) {}
unsigned char CALLBACK SIO1__readData8(void) { return 0; }
unsigned short CALLBACK SIO1__readData16(void) { return 0; }
unsigned long CALLBACK SIO1__readData32(void) { return 0; }
unsigned short CALLBACK SIO1__readStat16(void) { return 0; }
unsigned long CALLBACK SIO1__readStat32(void) { return 0; }
unsigned short CALLBACK SIO1__readMode16(void) { return 0; }
unsigned long CALLBACK SIO1__readMode32(void) { return 0; }
unsigned short CALLBACK SIO1__readCtrl16(void) { return 0; }
unsigned long CALLBACK SIO1__readCtrl32(void) { return 0; }
unsigned short CALLBACK SIO1__readBaud16(void) { return 0; }
unsigned long CALLBACK SIO1__readBaud32(void) { return 0; }
void CALLBACK SIO1__registerCallback(void (CALLBACK *callback)(void)) {};

void CALLBACK SIO1irq(void) {
	psxHu32ref(0x1070) |= SWAPu32(0x100);
}

#define LoadSio1Sym1(dest, name) \
	LoadSym(SIO1_##dest, SIO1##dest, name, TRUE);

#define LoadSio1SymN(dest, name) \
	LoadSym(SIO1_##dest, SIO1##dest, name, FALSE);

#define LoadSio1Sym0(dest, name) \
	LoadSym(SIO1_##dest, SIO1##dest, name, FALSE); \
	if (SIO1_##dest == NULL) SIO1_##dest = (SIO1##dest) SIO1__##dest;

static int LoadSIO1plugin(const char *SIO1dll) {
	void *drv;

	hSIO1Driver = SysLoadLibrary(SIO1dll);
	if (hSIO1Driver == NULL) {
		SysMessage (_("Could not load SIO1 plugin %s!"), SIO1dll); return -1;
	}
	drv = hSIO1Driver;

	LoadSio1Sym0(init, "SIO1init");
	LoadSio1Sym0(shutdown, "SIO1shutdown");
	LoadSio1Sym0(open, "SIO1open");
	LoadSio1Sym0(close, "SIO1close");
	LoadSio1Sym0(pause, "SIO1pause");
	LoadSio1Sym0(resume, "SIO1resume");
	LoadSio1Sym0(keypressed, "SIO1keypressed");
	LoadSio1Sym0(configure, "SIO1configure");
	LoadSio1Sym0(test, "SIO1test");
	LoadSio1Sym0(about, "SIO1about");
	LoadSio1Sym0(writeData8, "SIO1writeData8");
	LoadSio1Sym0(writeData16, "SIO1writeData16");
	LoadSio1Sym0(writeData32, "SIO1writeData32");
	LoadSio1Sym0(writeStat16, "SIO1writeStat16");
	LoadSio1Sym0(writeStat32, "SIO1writeStat32");
	LoadSio1Sym0(writeMode16, "SIO1writeMode16");
	LoadSio1Sym0(writeMode32, "SIO1writeMode32");
	LoadSio1Sym0(writeCtrl16, "SIO1writeCtrl16");
	LoadSio1Sym0(writeCtrl32, "SIO1writeCtrl32");
	LoadSio1Sym0(writeBaud16, "SIO1writeBaud16");
	LoadSio1Sym0(writeBaud32, "SIO1writeBaud32");
	LoadSio1Sym0(readData16, "SIO1readData16");
	LoadSio1Sym0(readData32, "SIO1readData32");
	LoadSio1Sym0(readStat16, "SIO1readStat16");
	LoadSio1Sym0(readStat32, "SIO1readStat32");
	LoadSio1Sym0(readMode16, "SIO1readMode16");
	LoadSio1Sym0(readMode32, "SIO1readMode32");
	LoadSio1Sym0(readCtrl16, "SIO1readCtrl16");
	LoadSio1Sym0(readCtrl32, "SIO1readCtrl32");
	LoadSio1Sym0(readBaud16, "SIO1readBaud16");
	LoadSio1Sym0(readBaud32, "SIO1readBaud32");
	LoadSio1Sym0(registerCallback, "SIO1registerCallback");

	return 0;
}

#endif

int LoadPlugins() {
	char Plugin[MAXPATHLEN * 2];
	int ret;

	ReleasePlugins();
	SysLibError();

	sprintf(Plugin, "%s/%s", Config.PluginsDir, Config.Gpu);
	if (LoadGPUplugin(Plugin) == -1) return -1;

	sprintf(Plugin, "%s/%s", Config.PluginsDir, Config.Spu);
	if (LoadSPUplugin(Plugin) == -1) return -1;

#ifdef ENABLE_SIO1API
	sprintf(Plugin, "%s/%s", Config.PluginsDir, Config.Sio1);
	if (LoadSIO1plugin(Plugin) == -1) return -1;
#endif

	ret = cdra_init();
	if (ret < 0) { SysMessage (_("Error initializing CD-ROM plugin: %d"), ret); return -1; }
	ret = GPU_init();
	if (ret < 0) { SysMessage (_("Error initializing GPU plugin: %d"), ret); return -1; }
	ret = SPU_init();
	if (ret < 0) { SysMessage (_("Error initializing SPU plugin: %d"), ret); return -1; }
	PAD_init();

#ifdef ENABLE_SIO1API
	ret = SIO1_init();
	if (ret < 0) { SysMessage (_("Error initializing SIO1 plugin: %d"), ret); return -1; }
#endif

	SysPrintf(_("Plugins loaded.\n"));
	return 0;
}

void ReleasePlugins() {
	cdra_shutdown();
	if (hGPUDriver != NULL) GPU_shutdown();
	if (hSPUDriver != NULL) SPU_shutdown();

	if (hGPUDriver != NULL) { SysCloseLibrary(hGPUDriver); hGPUDriver = NULL; }
	if (hSPUDriver != NULL) { SysCloseLibrary(hSPUDriver); hSPUDriver = NULL; }

#ifdef ENABLE_SIO1API
	if (hSIO1Driver != NULL) {
		SIO1_shutdown();
		SysCloseLibrary(hSIO1Driver);
		hSIO1Driver = NULL;
	}
#endif
}

// for CD swap
int ReloadCdromPlugin()
{
	cdra_shutdown();
	return cdra_init();
}

void SetIsoFile(const char *filename) {
	if (filename == NULL) {
		IsoFile[0] = '\0';
		return;
	}
	strncpy(IsoFile, filename, MAXPATHLEN - 1);
}

const char *GetIsoFile(void) {
	return IsoFile;
}

boolean UsingIso(void) {
	return (IsoFile[0] != '\0');
}

void SetCdOpenCaseTime(s64 time) {
	cdOpenCaseTime = time;
}
