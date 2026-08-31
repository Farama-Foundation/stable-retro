/*
 * Textured fragment shader.
 *
 * Samples a single 2D combined-image-sampler at the interpolated
 * screen-space UV produced by fullscreen_quad.vert.  Phase 4c
 * uses this with a static CPU-generated checkerboard texture to
 * prove the upload + sample + descriptor-bind path works.  In
 * Phase 4d the same shader (or a variant of it) will sample the
 * SW renderer's vid.buffer uploaded as an 8bpp index image, with
 * a palette LUT lookup in place of the direct texture() call.
 *
 * Descriptor set layout:
 *   set 0, binding 0:  sampler2D  u_tex
 *
 * Matches the layout written into the VkDescriptorSetLayout in
 * backend_vulkan.c's create_resources.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 * Copyright (C) 2026 The libretro team
 */
#version 450

layout(location = 0) in  vec2 v_uv;
layout(location = 0) out vec4 fragColor;

layout(set = 0, binding = 0) uniform sampler2D u_tex;

void main()
{
    fragColor = texture(u_tex, v_uv);
}
