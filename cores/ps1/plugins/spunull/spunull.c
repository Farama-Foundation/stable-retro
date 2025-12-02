/////////////////////////////////////////////////////////

#define PSE_LT_SPU                  4
#define PSE_SPU_ERR_SUCCESS         0
#define PSE_SPU_ERR                 -60
#define PSE_SPU_ERR_NOTCONFIGURED   PSE_SPU_ERR -1
#define PSE_SPU_ERR_INIT            PSE_SPU_ERR -2

/////////////////////////////////////////////////////////
// main emu calls:
// 0. Get type/name/version
// 1. Init
// 2. SetCallbacks
// 3. SetConfigFile
// 4. Open
// 5. Dma/register/xa calls...
// 6. Close
// 7. Shutdown
/////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xa.h"
#include "register.h"
// some ms windows compatibility define
#undef CALLBACK
#define CALLBACK

////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////

const unsigned char version  = 1;
const unsigned char revision = 1;
const unsigned char build    = 1;
static char * libraryName    = "Pete's Null Audio Driver";
static char * libraryInfo    = "Pete's Null Audio Driver V1.1\nCoded by Pete Bernert\n"; 

////////////////////////////////////////////////////////////////////////

unsigned short  regArea[10000];                        // psx buffer
unsigned short  spuMem[256*1024];
unsigned char * spuMemC;
unsigned char * pSpuIrq=0;

unsigned short spuCtrl, spuStat, spuIrq=0;             // some vars to store psx reg infos
unsigned long  spuAddr=0xffffffff;                     // address into spu mem
char *         pConfigFile=0;

////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////

void (CALLBACK *irqCallback)(void)=0;                   // func of main emu, called on spu irq
void (CALLBACK *cddavCallback)(short, short)=0;

////////////////////////////////////////////////////////////////////////
// CODE AREA
////////////////////////////////////////////////////////////////////////

void CALLBACK SPUwriteRegister(unsigned long reg, unsigned short val, unsigned int cycles)
{
 unsigned long r=reg&0xfff;
 regArea[(r-0xc00)>>1] = val;

 if(r>=0x0c00 && r<0x0d80)
  {
   //int ch=(r>>4)-0xc0;
   switch(r&0x0f)
    {
     //------------------------------------------------// l volume
     case 0:                    
       //SetVolumeL(ch,val);
       return;
     //------------------------------------------------// r volume
     case 2:                                           
       //SetVolumeR(ch,val);
       return;
     //------------------------------------------------// pitch
     case 4:                                           
       //SetPitch(ch,val);
       return;
     //------------------------------------------------// start
     case 6:
       //s_chan[ch].pStart=spuMemC+((unsigned long) val<<3);
       return;
     //------------------------------------------------// adsr level 
     case 8:
       return;
     //------------------------------------------------// adsr rate 
     case 10:
      return;
     //------------------------------------------------// adsr volume
     case 12:
       return;
     //------------------------------------------------// loop adr
     case 14:                                          
       return;
     //------------------------------------------------//
    }
   return;
  }

 switch(r)
   {
    //-------------------------------------------------//
    case H_SPUaddr:
        spuAddr = (unsigned long) val<<3;
        return;
    //-------------------------------------------------//
    case H_SPUdata:
        spuMem[spuAddr>>1] = val;
        spuAddr+=2;
        if(spuAddr>0x7ffff) spuAddr=0;
        return;
    //-------------------------------------------------//
    case H_SPUctrl:
        spuCtrl=val;
        return;
    //-------------------------------------------------//
    case H_SPUstat:
        spuStat=val & 0xf800;
        return;
    //-------------------------------------------------//
    case H_SPUirqAddr:
        spuIrq = val;
        pSpuIrq=spuMemC+((unsigned long) val<<3);
        return;
    //-------------------------------------------------//
    case H_SPUon1:
        //SoundOn(0,16,val);
        return;
    //-------------------------------------------------//
    case H_SPUon2:
        //SoundOn(16,24,val);
        return;
    //-------------------------------------------------//
    case H_SPUoff1:
        //SoundOff(0,16,val);
        return;
    //-------------------------------------------------//
    case H_SPUoff2:
        //SoundOff(16,24,val);
        return;
    //-------------------------------------------------//
    case H_CDLeft:
        if(cddavCallback) cddavCallback(0,val);
        return;
    case H_CDRight:
        if(cddavCallback) cddavCallback(1,val);
        return;
    //-------------------------------------------------//
    case H_FMod1:
        //FModOn(0,16,val);
        return;
    //-------------------------------------------------//
    case H_FMod2:
        //FModOn(16,24,val);
        return;
    //-------------------------------------------------//
    case H_Noise1:
        //NoiseOn(0,16,val);
        return;
    //-------------------------------------------------//
    case H_Noise2:
        //NoiseOn(16,24,val);
        return;
    //-------------------------------------------------//
    case H_RVBon1:
        //ReverbOn(0,16,val);
        return;
    //-------------------------------------------------//
    case H_RVBon2:
        //ReverbOn(16,24,val);
        return;
    //-------------------------------------------------//
    case H_Reverb:
        return;
   }
}

////////////////////////////////////////////////////////////////////////

unsigned short CALLBACK SPUreadRegister(unsigned long reg, unsigned int cycles)
{
 unsigned long r=reg&0xfff;

 if(r>=0x0c00 && r<0x0d80)
  {
   switch(r&0x0f)
    {
     case 12:                                          // adsr vol
      {
       //int ch=(r>>4)-0xc0;
       static unsigned short adsr_dummy_vol=0;
       adsr_dummy_vol=!adsr_dummy_vol;
       return adsr_dummy_vol;
      }

     case 14:                                          // return curr loop adr
      {
       //int ch=(r>>4)-0xc0;        
       return 0;
      }
    }
  }

 switch(r)
  {
    case H_SPUctrl:
        return spuCtrl;

    case H_SPUstat:
        return spuStat;
        
    case H_SPUaddr:
        return (unsigned short)(spuAddr>>3);

    case H_SPUdata:
      {
       unsigned short s=spuMem[spuAddr>>1];
       spuAddr+=2;
       if(spuAddr>0x7ffff) spuAddr=0;
       return s;
      }

    case H_SPUirqAddr:
        return spuIrq;
  }
 return regArea[(r-0xc00)>>1];
}
 
////////////////////////////////////////////////////////////////////////

unsigned short CALLBACK SPUreadDMA(void)
{
 unsigned short s=spuMem[spuAddr>>1];
 spuAddr+=2;
 if(spuAddr>0x7ffff) spuAddr=0;
 return s;
}

////////////////////////////////////////////////////////////////////////

void CALLBACK SPUwriteDMA(unsigned short val)
{
 spuMem[spuAddr>>1] = val;                             // spu addr got by writeregister
 spuAddr+=2;                                           // inc spu addr
 if(spuAddr>0x7ffff) spuAddr=0;                        // wrap
}

////////////////////////////////////////////////////////////////////////

void CALLBACK SPUwriteDMAMem(unsigned short * pusPSXMem,int iSize,unsigned int cycles)
{
 int i;
 for(i=0;i<iSize;i++)
  {
   spuMem[spuAddr>>1] = *pusPSXMem++;                  // spu addr got by writeregister
   spuAddr+=2;                                         // inc spu addr
   if(spuAddr>0x7ffff) spuAddr=0;                      // wrap
  }
}

////////////////////////////////////////////////////////////////////////

void CALLBACK SPUreadDMAMem(unsigned short * pusPSXMem,int iSize,unsigned int cycles)
{
 int i;
 for(i=0;i<iSize;i++)
  {
   *pusPSXMem++=spuMem[spuAddr>>1];                    // spu addr got by writeregister
   spuAddr+=2;                                         // inc spu addr
   if(spuAddr>0x7ffff) spuAddr=0;                      // wrap
  }
}

////////////////////////////////////////////////////////////////////////
// XA AUDIO
////////////////////////////////////////////////////////////////////////

void CALLBACK SPUplayADPCMchannel(xa_decode_t *xap, unsigned int cycles, int is_start)
{
}

void CALLBACK SPUsetCDvol(unsigned char ll, unsigned char lr,
  unsigned char rl, unsigned char rr, unsigned int cycle)
{
}

////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
// INIT/EXIT STUFF
////////////////////////////////////////////////////////////////////////

long CALLBACK SPUinit(void)
{
 spuMemC=(unsigned char *)spuMem;                      // just small setup
 return 0;
}

////////////////////////////////////////////////////////////////////////

int bSPUIsOpen=0;

long CALLBACK SPUopen(void)
{
 if(bSPUIsOpen) return 0;

 bSPUIsOpen=1;

 //if(pConfigFile) ReadConfigFile(pConfigFile);

 return PSE_SPU_ERR_SUCCESS;        
}

////////////////////////////////////////////////////////////////////////

void SPUsetConfigFile(char * pCfg)
{
 pConfigFile=pCfg;
}

////////////////////////////////////////////////////////////////////////

long CALLBACK SPUclose(void)
{
 if(!bSPUIsOpen) return 0;
 bSPUIsOpen=0;
 return 0;
}

////////////////////////////////////////////////////////////////////////

long CALLBACK SPUshutdown(void)
{
 return 0;
}

////////////////////////////////////////////////////////////////////////
// MISC STUFF
////////////////////////////////////////////////////////////////////////

long CALLBACK SPUtest(void)
{
 return 0;
}

void SPUasync(unsigned int cycle, unsigned int flags)
{
}

int SPUplayCDDAchannel(short *pcm, int nbytes, unsigned int cycle, int is_start)
{
 return -1;
}

////////////////////////////////////////////////////////////////////////
// this functions will be called once, 
// passes a callback that should be called on SPU-IRQ

void CALLBACK SPUregisterCallback(void (CALLBACK *callback)(void))
{
 irqCallback = callback;
}

void CALLBACK SPUregisterCDDAVolume(void (CALLBACK *CDDAVcallback)(short, short))
{
 cddavCallback = CDDAVcallback;
}

////////////////////////////////////////////////////////////////////////

char * CALLBACK PSEgetLibName(void)
{
 return libraryName;
}

////////////////////////////////////////////////////////////////////////

unsigned long CALLBACK PSEgetLibType(void)
{
 return  PSE_LT_SPU;
}

////////////////////////////////////////////////////////////////////////

unsigned long CALLBACK PSEgetLibVersion(void)
{
 return version<<16|revision<<8|build;
}


char * SPUgetLibInfos(void)
{
 return libraryInfo;
}

////////////////////////////////////////////////////////////////////////

typedef struct
{
 char          szSPUName[8];
 unsigned long ulFreezeVersion;
 unsigned long ulFreezeSize;
 unsigned char cSPUPort[0x200];
 unsigned char cSPURam[0x80000];
 xa_decode_t   xaS;     
} SPUFreeze_t;

typedef struct
{
 unsigned long Future[256];

} SPUNULLFreeze_t;

////////////////////////////////////////////////////////////////////////

long CALLBACK SPUfreeze(unsigned long ulFreezeMode,SPUFreeze_t * pF,unsigned int cycles)
{
 int i;

 if(!pF) return 0;

 if(ulFreezeMode)
  {
   if(ulFreezeMode==1)
    memset(pF,0,sizeof(SPUFreeze_t)+sizeof(SPUNULLFreeze_t));

   strcpy(pF->szSPUName,"PBNUL");
   pF->ulFreezeVersion=1;
   pF->ulFreezeSize=sizeof(SPUFreeze_t)+sizeof(SPUNULLFreeze_t);

   if(ulFreezeMode==2) return 1;

   memcpy(pF->cSPURam,spuMem,0x80000);
   memcpy(pF->cSPUPort,regArea,0x200);
   // dummy:
   memset(&pF->xaS,0,sizeof(xa_decode_t));
   return 1;
  }

 if(ulFreezeMode!=0) return 0;

 memcpy(spuMem,pF->cSPURam,0x80000);
 memcpy(regArea,pF->cSPUPort,0x200);

 for(i=0;i<0x100;i++)
  {
   if(i!=H_SPUon1-0xc00 && i!=H_SPUon2-0xc00)
    SPUwriteRegister(0x1f801c00+i*2,regArea[i],cycles);
  }
 SPUwriteRegister(H_SPUon1,regArea[(H_SPUon1-0xc00)/2],cycles);
 SPUwriteRegister(H_SPUon2,regArea[(H_SPUon2-0xc00)/2],cycles);

 return 1;
}


//////////////////////////////////////////////////////////////////////// 
//////////////////////////////////////////////////////////////////////// 
//////////////////////////////////////////////////////////////////////// 
// UNUSED WINDOWS FUNCS... YOU SHOULDN'T USE THEM IN LINUX

long CALLBACK SPUconfigure(void)
{
 return 0;
}

void CALLBACK SPUabout(void)
{
}

//////////////////////////////////////////////////////////////////////// 
//////////////////////////////////////////////////////////////////////// 
//////////////////////////////////////////////////////////////////////// 
// OLD PSEMU 1 FUNCS... YOU SHOULDN'T USE THEM

unsigned short CALLBACK SPUgetOne(unsigned long val) 
{ 
 if(spuAddr!=0xffffffff) 
  { 
   return SPUreadDMA(); 
  } 
 if(val>=512*1024) val=512*1024-1; 
 return spuMem[val>>1]; 
} 
 
void CALLBACK SPUputOne(unsigned long val,unsigned short data) 
{ 
 if(spuAddr!=0xffffffff) 
  { 
   SPUwriteDMA(data); 
   return; 
  } 
 if(val>=512*1024) val=512*1024-1; 
 spuMem[val>>1] = data; 
} 
 
void CALLBACK SPUplaySample(unsigned char ch) 
{ 
} 
 
void CALLBACK SPUsetAddr(unsigned char ch, unsigned short waddr) 
{ 
 //s_chan[ch].pStart=spuMemC+((unsigned long) waddr<<3); 
} 
 
void CALLBACK SPUsetPitch(unsigned char ch, unsigned short pitch) 
{ 
 //SetPitch(ch,pitch); 
} 
 
void CALLBACK SPUsetVolumeL(unsigned char ch, short vol) 
{ 
 //SetVolumeL(ch,vol); 
} 
 
void CALLBACK SPUsetVolumeR(unsigned char ch, short vol) 
{ 
 //SetVolumeR(ch,vol); 
}                
 
void CALLBACK SPUstartChannels1(unsigned short channels) 
{ 
 //SoundOn(0,16,channels); 
} 
 
void CALLBACK SPUstartChannels2(unsigned short channels) 
{ 
 //SoundOn(16,24,channels); 
} 
 
void CALLBACK SPUstopChannels1(unsigned short channels) 
{ 
 //SoundOff(0,16,channels); 
} 
 
void CALLBACK SPUstopChannels2(unsigned short channels) 
{ 
 //SoundOff(16,24,channels); 
} 

void CALLBACK SPUregisterScheduleCb(void (CALLBACK *callback)(unsigned int))
{
}
