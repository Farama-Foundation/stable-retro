#ifndef __GPU_UNAI_GPU_ARM_H__
#define __GPU_UNAI_GPU_ARM_H__

#ifdef __cplusplus
extern "C" {
#endif

struct gpu_unai_inner_t;

void tile_driver_st0_asm(void *d, u16 c, u32 cnt, const struct gpu_unai_inner_t *inn);
void tile_driver_st1_asm(void *d, u16 c, u32 cnt, const struct gpu_unai_inner_t *inn);
void tile_driver_st3_asm(void *d, u16 c, u32 cnt, const struct gpu_unai_inner_t *inn);

void sprite_driver_4bpp_asm(void *pPixel, const u8 *pTxt_base,
	u32 count, const struct gpu_unai_inner_t *inn);
void sprite_driver_8bpp_asm(void *pPixel, const u8 *pTxt_base,
	u32 count, const struct gpu_unai_inner_t *inn);
void sprite_driver_16bpp_asm(void *pPixel, const void *pTxt_base,
	u32 count, const struct gpu_unai_inner_t *inn);
void sprite_4bpp_x16_asm(void *d, const void *s, void *pal, int lines);

void sprite_driver_4bpp_l0_std_asm(void *pPixel, const u8 *pTxt_base,
	u32 count, const struct gpu_unai_inner_t *inn);
void sprite_driver_4bpp_l0_st0_asm(void *pPixel, const u8 *pTxt_base,
	u32 count, const struct gpu_unai_inner_t *inn);
void sprite_driver_8bpp_l0_std_asm(void *pPixel, const u8 *pTxt_base,
	u32 count, const struct gpu_unai_inner_t *inn);
void sprite_driver_8bpp_l0_st0_asm(void *pPixel, const u8 *pTxt_base,
	u32 count, const struct gpu_unai_inner_t *inn);

void poly_untex_st0_asm(void *d, const struct gpu_unai_inner_t *inn, int count);
void poly_untex_st1_asm(void *d, const struct gpu_unai_inner_t *inn, int count);
void poly_untex_st3_asm(void *d, const struct gpu_unai_inner_t *inn, int count);
void poly_4bpp_asm       (void *d, const struct gpu_unai_inner_t *inn, int count);
void poly_4bpp_l0_st0_asm(void *d, const struct gpu_unai_inner_t *inn, int count);
void poly_8bpp_asm       (void *d, const struct gpu_unai_inner_t *inn, int count);
void poly_8bpp_l0_st0_asm(void *d, const struct gpu_unai_inner_t *inn, int count);

#ifdef HAVE_ARMV6

void tile_driver_st2_asm(void *d, u16 c, u32 cnt, const struct gpu_unai_inner_t *inn);

void sprite_driver_4bpp_l1_std_asm(void *pPixel, const u8 *pTxt_base,
	u32 count, const struct gpu_unai_inner_t *inn);
void sprite_driver_4bpp_l1_st0_asm(void *pPixel, const u8 *pTxt_base,
	u32 count, const struct gpu_unai_inner_t *inn);
void sprite_driver_4bpp_l1_st1_asm(void *pPixel, const u8 *pTxt_base,
	u32 count, const struct gpu_unai_inner_t *inn);
void sprite_driver_8bpp_l1_std_asm(void *pPixel, const u8 *pTxt_base,
	u32 count, const struct gpu_unai_inner_t *inn);
void sprite_driver_8bpp_l1_st0_asm(void *pPixel, const u8 *pTxt_base,
	u32 count, const struct gpu_unai_inner_t *inn);
void sprite_driver_8bpp_l1_st1_asm(void *pPixel, const u8 *pTxt_base,
	u32 count, const struct gpu_unai_inner_t *inn);

void poly_untex_st2_asm(void *d, const struct gpu_unai_inner_t *inn, int count);
void poly_4bpp_l1_std_asm(void *d, const struct gpu_unai_inner_t *inn, int count);
void poly_4bpp_l1_st0_asm(void *d, const struct gpu_unai_inner_t *inn, int count);
void poly_8bpp_l1_std_asm(void *d, const struct gpu_unai_inner_t *inn, int count);
void poly_8bpp_l1_st0_asm(void *d, const struct gpu_unai_inner_t *inn, int count);

#endif // HAVE_ARMV6

#ifdef __cplusplus
}
#endif

#endif /* __GPU_UNAI_GPU_ARM_H__ */
