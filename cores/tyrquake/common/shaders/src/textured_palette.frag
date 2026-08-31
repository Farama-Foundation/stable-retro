/*
 * Palette-indexed textured fragment shader.
 *
 * Reads an 8bpp palette index from u_index, uses it to look up
 * an RGBA colour in a 256x1 palette texture u_palette, and
 * writes that colour to the output.  The .r channel of u_index
 * is R8_UNORM, returning the byte b in [0, 255] as the float
 * b/255 in [0, 1].  Sampling u_palette at that exact UV with
 * NEAREST filtering picks texel b out of the 256-wide palette:
 *
 *     u = b/255 -> texel index = clamp(floor(u * 256), 0, 255) = b
 *
 * (Verified for b = 0, 1, 127, 128, 254, 255 -- the mapping is
 * exact, no off-by-one.)
 *
 * Alpha is forced to 1.0 so palette indices 0 and 255 -- which
 * VID_SetPalette2 in libretro.c zeros the alpha byte of for
 * SW transparency semantics -- don't bleed the render-pass
 * clear colour through the textured output.
 *
 * Descriptor set layout (matches backend_vulkan.c's
 * create_resources exactly):
 *   set 0, binding 0:  sampler2D  u_index    (R8_UNORM)
 *   set 0, binding 1:  sampler2D  u_palette  (R8G8B8A8_UNORM, 256x1)
 *
 * Both bindings declared FRAGMENT | COMPUTE in the DSL on the
 * C side, so a future compute pipeline can use the same set
 * layout without redeclaring.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 * Copyright (C) 2026 The libretro team
 */
#version 450

layout(location = 0) in  vec2 v_uv;
layout(location = 0) out vec4 fragColor;

layout(set = 0, binding = 0) uniform sampler2D u_index;
layout(set = 0, binding = 1) uniform sampler2D u_palette;

void main()
{
    float idx = texture(u_index, v_uv).r;
    vec3  rgb = texture(u_palette, vec2(idx, 0.5)).rgb;
    fragColor = vec4(rgb, 1.0);
}
