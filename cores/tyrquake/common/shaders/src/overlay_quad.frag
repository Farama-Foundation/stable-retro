/*
 * Fragment shader for the Phase 4i 2D overlay path
 * (Phase 4l: gains an early discard on Quake's
 * transparent-key palette index, 255).
 *
 * Samples an R8_UNORM palette-index texture at the
 * interpolated UV, then looks the index up in the same
 * 256x1 RGBA8 palette texture the compute palette LUT
 * uses (Phase 4f vk_palette_texture, propagated through
 * d_8to24table_shifted to track damage / quad / under-
 * water shifts).  Phase 4l: index 255 = TRANSPARENT_COLOR
 * (d_iface.h) -- the byte Quake's Draw_TransPic uses as
 * its transparency key.  The same shader handles
 * Draw_Pic and Draw_TransPic; Draw_Pic content
 * legitimately can't contain index 255 because Quake's
 * palette index 255 is reserved as the bright-magenta
 * transparency key by convention (no Quake opaque
 * artwork uses it as a visible colour), so the
 * unconditional discard is safe for both code paths.
 *
 * Output alpha is 1.0; the overlay pipeline's blend
 * stage is still enabled with SRC_ALPHA / ONE_MINUS_
 * SRC_ALPHA but with alpha = 1.0 the result is "src
 * replaces dst", which is what Quake's HUD pics want.
 *
 * Sampler binding details:
 *   set = 0, binding = 0:  R8_UNORM    index     (per-quad texture)
 *   set = 0, binding = 1:  R8G8B8A8_UNORM palette  (256x1, shared)
 *
 * Index sampling uses NEAREST filtering so Quake's
 * pixel-perfect 2D look survives any upscaling the
 * frontend does, AND so the discard test sees the exact
 * R8_UNORM-decoded value (255 / 255 = 1.0 with no
 * filter smear).  Palette sampling also uses NEAREST.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 * Copyright (C) 2026 The libretro team
 */
#version 450

layout(set = 0, binding = 0) uniform sampler2D u_index;
layout(set = 0, binding = 1) uniform sampler2D u_palette;

layout(location = 0) in  vec2 f_uv;
layout(location = 0) out vec4 out_color;

void main()
{
    float idx = texture(u_index, f_uv).r;

    /* TRANSPARENT_COLOR (== 255) maps to exactly 1.0 in
     * R8_UNORM; the threshold sits halfway between
     * 254 / 255 (~0.99608) and 255 / 255 (1.0). */
    if (idx > 254.5 / 255.0)
        discard;

    vec3 rgb = texture(u_palette, vec2(idx, 0.5)).rgb;
    out_color = vec4(rgb, 1.0);
}
