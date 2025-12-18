#ifndef _GLIDE64_MICLWR_H
#define _GLIDE64_MICLWR_H

void ClampTex(uint8_t *tex, uint32_t width,
      uint32_t clamp_to, uint32_t real_width, uint32_t real_height,
      uint32_t size);

void MirrorTex (uint8_t *tex, uint32_t mask, uint32_t max_width,
      uint32_t real_width, uint32_t height, uint32_t size);

#endif
