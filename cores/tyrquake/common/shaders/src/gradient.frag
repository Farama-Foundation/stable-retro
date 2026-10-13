/*
 * Gradient fragment shader.
 *
 * Outputs an RGB gradient derived from the interpolated
 * screen-space UV (red increases left-to-right, green increases
 * top-to-bottom, blue is a fixed 0.5).  Useful as the first
 * sanity check for the Vulkan backend's graphics pipeline:
 * proves that
 *   - SPIR-V shader modules load and link
 *   - the render pass + framebuffer + pipeline state objects
 *     are correctly set up
 *   - vertex shader interpolation reaches the fragment stage
 *   - the fragment stage's output reaches the render target
 *
 * Will be replaced in Phase 4b+ by shaders that sample
 * textures (the SW renderer's framebuffer first, then real
 * world geometry).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 * Copyright (C) 2026 The libretro team
 */
#version 450

layout(location = 0) in  vec2 v_uv;
layout(location = 0) out vec4 fragColor;

void main()
{
    fragColor = vec4(v_uv.x, v_uv.y, 0.5, 1.0);
}
