#ifndef _UTILS_H
#define _UTILS_H

#include "screenlayout.h"
#include "ARCodeFile.h"

#define CURSOR_SIZE 2 // TODO: Maybe make this adjustable

#if defined(_WIN32)
#define PLATFORM_DIR_SEPERATOR '\\'
#else
#define PLATFORM_DIR_SEPERATOR  '/'
#endif

int32_t Clamp(int32_t value, int32_t min, int32_t max);
void copy_screen(ScreenLayoutData *data, uint32_t* src, unsigned offset);
void copy_hybrid_screen(ScreenLayoutData *data, uint32_t* src, ScreenId screen_id);
void draw_cursor(ScreenLayoutData *data, int32_t x, int32_t y);
namespace AREngine
{
    extern void RunCheat(ARCode& arcode);
}
#endif

