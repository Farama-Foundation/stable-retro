/*
 * Fullscreen quad vertex shader.
 *
 * Draws a single oversized triangle that covers the screen.
 * The triangle's third vertex is at NDC (3, -1, 0) which is
 * outside the [-1, 1] viewport; the rasterizer clips it.  The
 * net result is the same as a 4-vertex quad but with one
 * fewer vertex shader invocation and no diagonal seam.
 *
 * No vertex buffer is bound; gl_VertexIndex generates the
 * vertex position.  Callers should issue
 * vkCmdDraw(cmd, 3, 1, 0, 0).
 *
 * v_uv is the interpolated [0, 1] screen UV, where (0, 0) is
 * the top-left of the screen and (1, 1) is the bottom-right
 * (matching Vulkan's NDC convention with Y pointing down).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 * Copyright (C) 2026 The libretro team
 */
#version 450

layout(location = 0) out vec2 v_uv;

void main()
{
    /* gl_VertexIndex == 0 -> NDC (-1, -1) -> uv (0, 0) top-left
     * gl_VertexIndex == 1 -> NDC ( 3, -1) -> uv (2, 0) (clipped)
     * gl_VertexIndex == 2 -> NDC (-1,  3) -> uv (0, 2) (clipped)
     *
     * Interpolation across the visible portion of the triangle
     * produces uv in the [0, 1] x [0, 1] range across the screen. */
    vec2 positions[3] = vec2[3](
        vec2(-1.0, -1.0),
        vec2( 3.0, -1.0),
        vec2(-1.0,  3.0)
    );
    vec2 pos    = positions[gl_VertexIndex];
    v_uv        = (pos + 1.0) * 0.5;
    gl_Position = vec4(pos, 0.0, 1.0);
}
