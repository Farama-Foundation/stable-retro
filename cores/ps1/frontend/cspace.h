#ifndef __CSPACE_H__
#define __CSPACE_H__

#ifdef __cplusplus
extern "C"
{
#endif

void bgr555_to_rgb565(void *dst, const void *src, int bytes);
void bgr888_to_rgb888(void *dst, const void *src, int bytes);
void bgr888_to_rgb565(void *dst, const void *src, int bytes);
void rgb888_to_rgb565(void *dst, const void *src, int bytes);

void bgr555_to_rgb565_b(void *dst, const void *src, int bytes,
	int brightness2k); // 0-0x0800

void bgr555_to_xrgb8888(void *dst, const void *src, int bytes);
void bgr888_to_xrgb8888(void *dst, const void *src, int bytes);

void bgr_to_uyvy_init(void);
void rgb565_to_uyvy(void *d, const void *s, int pixels);
void bgr555_to_uyvy(void *d, const void *s, int pixels, int x2);
void bgr888_to_uyvy(void *d, const void *s, int pixels, int x2);

#ifdef __cplusplus
}
#endif

#endif /* __CSPACE_H__ */
