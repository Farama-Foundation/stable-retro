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

#ifndef __PLUGINS_H__
#define __PLUGINS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "psxcommon.h"
#include "psemu_plugin_defs.h"

//#define ENABLE_SIO1API 1

typedef long (CALLBACK *GPUopen)(unsigned long *, char *, char *);
typedef long (CALLBACK *SPUopen)(void);
typedef long (CALLBACK *SIO1open)(unsigned long *);

#include "spu.h"
#include "gpu.h"
#include "decode_xa.h"

int LoadPlugins();
void ReleasePlugins();
int OpenPlugins();
void ClosePlugins();
int ReloadCdromPlugin();

typedef unsigned long (CALLBACK* PSEgetLibType)(void);
typedef unsigned long (CALLBACK* PSEgetLibVersion)(void);
typedef char *(CALLBACK* PSEgetLibName)(void);

// GPU Functions
typedef long (CALLBACK* GPUinit)(void);
typedef long (CALLBACK* GPUshutdown)(void);
typedef long (CALLBACK* GPUclose)(void);
typedef void (CALLBACK* GPUwriteStatus)(uint32_t);
typedef void (CALLBACK* GPUwriteData)(uint32_t);
typedef void (CALLBACK* GPUwriteDataMem)(uint32_t *, int);
typedef uint32_t (CALLBACK* GPUreadStatus)(void);
typedef uint32_t (CALLBACK* GPUreadData)(void);
typedef void (CALLBACK* GPUreadDataMem)(uint32_t *, int);
typedef long (CALLBACK* GPUdmaChain)(uint32_t *, uint32_t, uint32_t *, int32_t *);
typedef void (CALLBACK* GPUupdateLace)(void);
typedef long (CALLBACK* GPUfreeze)(uint32_t, GPUFreeze_t *);
typedef void (CALLBACK* GPUvBlank)(int, int);
typedef void (CALLBACK* GPUgetScreenInfo)(int *, int *);

// GPU function pointers
extern GPUupdateLace    GPU_updateLace;
extern GPUinit          GPU_init;
extern GPUshutdown      GPU_shutdown; 
extern GPUopen          GPU_open;
extern GPUclose         GPU_close;
extern GPUreadStatus    GPU_readStatus;
extern GPUreadData      GPU_readData;
extern GPUreadDataMem   GPU_readDataMem;
extern GPUwriteStatus   GPU_writeStatus; 
extern GPUwriteData     GPU_writeData;
extern GPUwriteDataMem  GPU_writeDataMem;
extern GPUdmaChain      GPU_dmaChain;
extern GPUfreeze        GPU_freeze;
extern GPUvBlank        GPU_vBlank;
extern GPUgetScreenInfo GPU_getScreenInfo;

// CD-ROM
struct CdrStat {
	uint32_t Type; // DATA, CDDA
	uint32_t Status; // same as cdr.StatP
	unsigned char Time_[3]; // unused
};

int CDR__getStatus(struct CdrStat *stat);

// SPU Functions
typedef long (CALLBACK* SPUinit)(void);				
typedef long (CALLBACK* SPUshutdown)(void);	
typedef long (CALLBACK* SPUclose)(void);			
typedef void (CALLBACK* SPUwriteRegister)(unsigned long, unsigned short, unsigned int);
typedef unsigned short (CALLBACK* SPUreadRegister)(unsigned long, unsigned int);
typedef void (CALLBACK* SPUwriteDMAMem)(unsigned short *, int, unsigned int);
typedef void (CALLBACK* SPUreadDMAMem)(unsigned short *, int, unsigned int);
typedef void (CALLBACK* SPUplayADPCMchannel)(xa_decode_t *, unsigned int, int);
typedef void (CALLBACK* SPUregisterCallback)(void (CALLBACK *callback)(int));
typedef void (CALLBACK* SPUregisterScheduleCb)(void (CALLBACK *callback)(unsigned int cycles_after));
typedef struct {
	unsigned char PluginName[8];
	uint32_t PluginVersion;
	uint32_t Size;
} SPUFreezeHdr_t;
typedef struct SPUFreeze {
	unsigned char PluginName[8];
	uint32_t PluginVersion;
	uint32_t Size;
	unsigned char SPUPorts[0x200];
	unsigned char SPURam[0x80000];
	xa_decode_t xa;
	unsigned char *unused;
} SPUFreeze_t;
typedef long (CALLBACK* SPUfreeze)(unsigned int, struct SPUFreeze *, unsigned int);
typedef void (CALLBACK* SPUasync)(unsigned int, unsigned int);
typedef int  (CALLBACK* SPUplayCDDAchannel)(short *, int, unsigned int, int);
typedef void (CALLBACK* SPUsetCDvol)(unsigned char, unsigned char, unsigned char, unsigned char, unsigned int);

// SPU function pointers
extern SPUinit             SPU_init;
extern SPUshutdown         SPU_shutdown;
extern SPUopen             SPU_open;
extern SPUclose            SPU_close;
extern SPUwriteRegister    SPU_writeRegister;
extern SPUreadRegister     SPU_readRegister;
extern SPUwriteDMAMem      SPU_writeDMAMem;
extern SPUreadDMAMem       SPU_readDMAMem;
extern SPUplayADPCMchannel SPU_playADPCMchannel;
extern SPUfreeze           SPU_freeze;
extern SPUregisterCallback SPU_registerCallback;
extern SPUregisterScheduleCb SPU_registerScheduleCb;
extern SPUasync            SPU_async;
extern SPUplayCDDAchannel  SPU_playCDDAchannel;
extern SPUsetCDvol         SPU_setCDvol;

// PAD Functions
long PAD1_readPort(PadDataS *);
unsigned char PAD1_startPoll(int);
unsigned char PAD1_poll(unsigned char, int *);

long PAD2_readPort(PadDataS *);
unsigned char PAD2_startPoll(int);
unsigned char PAD2_poll(unsigned char, int *);

#ifdef ENABLE_SIO1API

// SIO1 Functions (link cable)
typedef long (CALLBACK* SIO1init)(void);
typedef long (CALLBACK* SIO1shutdown)(void);
typedef long (CALLBACK* SIO1close)(void);
typedef long (CALLBACK* SIO1configure)(void);
typedef long (CALLBACK* SIO1test)(void);
typedef void (CALLBACK* SIO1about)(void);
typedef void (CALLBACK* SIO1pause)(void);
typedef void (CALLBACK* SIO1resume)(void);
typedef long (CALLBACK* SIO1keypressed)(int);
typedef void (CALLBACK* SIO1writeData8)(unsigned char);
typedef void (CALLBACK* SIO1writeData16)(unsigned short);
typedef void (CALLBACK* SIO1writeData32)(unsigned long);
typedef void (CALLBACK* SIO1writeStat16)(unsigned short);
typedef void (CALLBACK* SIO1writeStat32)(unsigned long);
typedef void (CALLBACK* SIO1writeMode16)(unsigned short);
typedef void (CALLBACK* SIO1writeMode32)(unsigned long);
typedef void (CALLBACK* SIO1writeCtrl16)(unsigned short);
typedef void (CALLBACK* SIO1writeCtrl32)(unsigned long);
typedef void (CALLBACK* SIO1writeBaud16)(unsigned short);
typedef void (CALLBACK* SIO1writeBaud32)(unsigned long);
typedef unsigned char (CALLBACK* SIO1readData8)(void);
typedef unsigned short (CALLBACK* SIO1readData16)(void);
typedef unsigned long (CALLBACK* SIO1readData32)(void);
typedef unsigned short (CALLBACK* SIO1readStat16)(void);
typedef unsigned long (CALLBACK* SIO1readStat32)(void);
typedef unsigned short (CALLBACK* SIO1readMode16)(void);
typedef unsigned long (CALLBACK* SIO1readMode32)(void);
typedef unsigned short (CALLBACK* SIO1readCtrl16)(void);
typedef unsigned long (CALLBACK* SIO1readCtrl32)(void);
typedef unsigned short (CALLBACK* SIO1readBaud16)(void);
typedef unsigned long (CALLBACK* SIO1readBaud32)(void);
typedef void (CALLBACK* SIO1registerCallback)(void (CALLBACK *callback)(void));

// SIO1 function pointers 
extern SIO1init               SIO1_init;
extern SIO1shutdown           SIO1_shutdown;
extern SIO1open               SIO1_open;
extern SIO1close              SIO1_close; 
extern SIO1test               SIO1_test;
extern SIO1configure          SIO1_configure;
extern SIO1about              SIO1_about;
extern SIO1pause              SIO1_pause;
extern SIO1resume             SIO1_resume;
extern SIO1keypressed         SIO1_keypressed;
extern SIO1writeData8         SIO1_writeData8;
extern SIO1writeData16        SIO1_writeData16;
extern SIO1writeData32        SIO1_writeData32;
extern SIO1writeStat16        SIO1_writeStat16;
extern SIO1writeStat32        SIO1_writeStat32;
extern SIO1writeMode16        SIO1_writeMode16;
extern SIO1writeMode32        SIO1_writeMode32;
extern SIO1writeCtrl16        SIO1_writeCtrl16;
extern SIO1writeCtrl32        SIO1_writeCtrl32;
extern SIO1writeBaud16        SIO1_writeBaud16;
extern SIO1writeBaud32        SIO1_writeBaud32;
extern SIO1readData8          SIO1_readData8;
extern SIO1readData16         SIO1_readData16;
extern SIO1readData32         SIO1_readData32;
extern SIO1readStat16         SIO1_readStat16;
extern SIO1readStat32         SIO1_readStat32;
extern SIO1readMode16         SIO1_readMode16;
extern SIO1readMode32         SIO1_readMode32;
extern SIO1readCtrl16         SIO1_readCtrl16;
extern SIO1readCtrl32         SIO1_readCtrl32;
extern SIO1readBaud16         SIO1_readBaud16;
extern SIO1readBaud32         SIO1_readBaud32;
extern SIO1registerCallback   SIO1_registerCallback;

#endif

void SetIsoFile(const char *filename);
const char *GetIsoFile(void);
boolean UsingIso(void);
void SetCdOpenCaseTime(s64 time);

int padFreeze(void *f, int Mode);
int padToggleAnalog(unsigned int index);

extern void pl_gun_byte2(int port, unsigned char byte);
extern void plat_trigger_vibrate(int pad, int low, int high);

#ifdef __cplusplus
}
#endif
#endif
