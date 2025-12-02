#ifndef __P_SPU_CONFIG_H__
#define __P_SPU_CONFIG_H__

// user settings

typedef struct
{
 int        iVolume;
 int        iXAPitch;
 int        iUseReverb;
 int        iUseInterpolation;
 int        iTempo;
 int        iUseThread;

 // status
 int        iThreadAvail;
} SPUConfig;

extern SPUConfig spu_config;

#endif /* __P_SPU_CONFIG_H__ */
