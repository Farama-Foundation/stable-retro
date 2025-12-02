#include <stdio.h>
#include <stddef.h>

#include "common.h"

#define WRITE_OFFSET(f, member) \
	fprintf(f, "#define %-50s0x%zx\n", \
		"psx_gpu_" #member "_offset", \
		offsetof(psx_gpu_struct, member));

int main()
{
	FILE *f;

	if (sizeof(f) != 4) {
		fprintf(stderr, "bad pointer size\n");
		return 1;
	}

	f = fopen("psx_gpu_offsets.h", "w");
	if (f == NULL) {
		perror("fopen");
		return 1;
	}
	fputs("#ifndef __P_PSX_GPU_OFFSETS_H__\n", f);
	fputs("#define __P_PSX_GPU_OFFSETS_H__\n\n", f);

	//WRITE_OFFSET(f, test_mask);
	WRITE_OFFSET(f, uvrg);
	WRITE_OFFSET(f, uvrg_dx);
	WRITE_OFFSET(f, uvrg_dy);
	WRITE_OFFSET(f, u_block_span);
	WRITE_OFFSET(f, v_block_span);
	WRITE_OFFSET(f, r_block_span);
	WRITE_OFFSET(f, g_block_span);
	WRITE_OFFSET(f, b_block_span);
	WRITE_OFFSET(f, b);
	WRITE_OFFSET(f, b_dy);
	WRITE_OFFSET(f, triangle_area);
	//WRITE_OFFSET(f, texture_window_settings);
	WRITE_OFFSET(f, current_texture_mask);
	//WRITE_OFFSET(f, viewport_mask);
	WRITE_OFFSET(f, dirty_textures_4bpp_mask);
	WRITE_OFFSET(f, dirty_textures_8bpp_mask);
	WRITE_OFFSET(f, dirty_textures_8bpp_alternate_mask);
	WRITE_OFFSET(f, triangle_color);
	WRITE_OFFSET(f, dither_table);
	//WRITE_OFFSET(f, render_block_handler);
	WRITE_OFFSET(f, texture_page_ptr);
	WRITE_OFFSET(f, texture_page_base);
	WRITE_OFFSET(f, clut_ptr);
	WRITE_OFFSET(f, vram_ptr);
	WRITE_OFFSET(f, vram_out_ptr);
	WRITE_OFFSET(f, uvrgb_phase);
	//WRITE_OFFSET(f, render_state_base);
	//WRITE_OFFSET(f, render_state);
	WRITE_OFFSET(f, num_spans);
	WRITE_OFFSET(f, num_blocks);
	WRITE_OFFSET(f, viewport_start_x);
	WRITE_OFFSET(f, viewport_start_y);
	WRITE_OFFSET(f, viewport_end_x);
	WRITE_OFFSET(f, viewport_end_y);
	WRITE_OFFSET(f, mask_msb);
	WRITE_OFFSET(f, triangle_winding);
	//WRITE_OFFSET(f, display_area_draw_enable);
	WRITE_OFFSET(f, current_texture_page);
	//WRITE_OFFSET(f, last_8bpp_texture_page);
	WRITE_OFFSET(f, texture_mask_width);
	WRITE_OFFSET(f, texture_mask_height);
	//WRITE_OFFSET(f, texture_window_x);
	//WRITE_OFFSET(f, texture_window_y);
	//WRITE_OFFSET(f, primitive_type);
	//WRITE_OFFSET(f, render_mode);
	//WRITE_OFFSET(f, offset_x);
	//WRITE_OFFSET(f, offset_y);
	//WRITE_OFFSET(f, clut_settings);
	//WRITE_OFFSET(f, texture_settings);
	WRITE_OFFSET(f, reciprocal_table_ptr);
	WRITE_OFFSET(f, hacks_active);
	WRITE_OFFSET(f, saved_tmp);
	//WRITE_OFFSET(f, saved_q4_q7);
	WRITE_OFFSET(f, blocks);
	WRITE_OFFSET(f, span_uvrg_offset);
	WRITE_OFFSET(f, span_edge_data);
	WRITE_OFFSET(f, span_b_offset);
	//WRITE_OFFSET(f, texture_4bpp_cache);
	//WRITE_OFFSET(f, texture_8bpp_even_cache);
	//WRITE_OFFSET(f, texture_8bpp_odd_cache);

	fputs("\n#endif /* __P_PSX_GPU_OFFSETS_H__ */\n", f);
	fclose(f);

	return 0;
}
