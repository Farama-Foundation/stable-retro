#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

enum Aspect
{
   aStretch = 0,
   a43 = 1,
   a169 = 2,
   aAdjust = 3,
   aTotal = 4
};

#ifdef __cplusplus
extern "C" {
#endif

#define config gln64config

#include <boolean.h>

#define BILINEAR_3POINT   0
#define BILINEAR_STANDARD 1


typedef struct
{
    int     version;

    struct
    {
        int width, height;
    } screen;

    struct
    {
        int force, width, height;
    } video;

    struct
    {
        int maxAnisotropy;
        float maxAnisotropyF;
        int enableMipmap;
        uint32_t bilinearMode;
        int useIA;
        int fastCRC;
    } texture;

	struct {
		uint32_t enableNoise;
		uint32_t enableLOD;
		uint32_t enableHWLighting;
		uint32_t enableCustomSettings;
		uint32_t hacks;
	} generalEmulation;


	struct {
		uint32_t enable;
		uint32_t copyToRDRAM;
		uint32_t copyDepthToRDRAM;
		uint32_t copyFromRDRAM;
		uint32_t detectCFB;
		uint32_t N64DepthCompare;
		uint32_t aspect; // 0: stretch ; 1: 4/3 ; 2: 16/9; 3: adjust
		uint32_t validityCheckMethod; // 0: checksum; 1: fill RDRAM
	} frameBufferEmulation;

    int     zHack;

    int     enableNoise;

    int     hackAlpha;

    bool    stretchVideo;
    bool    romPAL;    //is the rom PAL
    char    romName[21];
} gln64Config;

#define hack_Ogre64					(1<<0)  //Ogre Battle 64 background copy
#define hack_noDepthFrameBuffers	(1<<1)  //Do not use depth buffers as texture
#define hack_blurPauseScreen		(1<<2)  //Game copies frame buffer to depth buffer area, CPU blurs it. That image is used as background for pause screen.
#define hack_scoreboard				(1<<3)  //Copy data from RDRAM to auxilary frame buffer. Scoreboard in Mario Tennis.
#define hack_pilotWings				(1<<4)  //Special blend mode for PilotWings.
#define hack_subscreen				(1<<5)  //Fix subscreen delay in Zelda OOT
#define hack_legoRacers				(1<<6)  //LEGO racers course map
#define hack_blastCorps				(1<<7)  //Blast Corps black polygons

extern gln64Config config;

void Config_gln64_LoadConfig(void);
void Config_gln64_LoadRomConfig(unsigned char* header);

#ifdef __cplusplus
}
#endif

#endif
