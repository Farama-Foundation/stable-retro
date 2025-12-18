#ifndef VI_H
#define VI_H

#include <stdint.h>
#include <boolean.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    uint32_t width, widthPrev, height, real_height;
    float rwidth, rheight;
    uint32_t lastOrigin;

    uint32_t realWidth, realHeight;
    bool interlaced;
    bool PAL;

    struct{
        uint32_t start, end;
    } display[16];

    uint32_t displayNum;

} VIInfo;

extern VIInfo VI;

void VI_UpdateSize(void);
void VI_UpdateScreen(void);

#ifdef __cplusplus
}
#endif

#endif

