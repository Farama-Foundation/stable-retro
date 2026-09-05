/*
 * Vertex shader for the Phase 4i 2D overlay path.
 *
 * Reads a 2D position (in NDC) and a 2D UV from vertex
 * input.  Writes gl_Position and forwards the UV to the
 * FS for per-pixel interpolation across the quad.
 *
 * Position is in NDC directly (no projection) so the
 * caller controls placement; the per-frame caller fills
 * the vertex buffer with the right NDC corners for each
 * Draw_Pic-equivalent rectangle.  Phase 4j will add a
 * push-constant projection matrix so the caller can pass
 * Quake's 320x200-based screen coordinates directly
 * without manual NDC mapping, but Phase 4i still uses
 * pre-mapped NDC for the static demo quad.
 *
 * UV is in standard 0..1 texture-space coordinates.  The
 * FS samples the palette-indexed test texture at the
 * interpolated UV.
 *
 * Vulkan NDC has y = -1 at the top of the framebuffer
 * and y = +1 at the bottom.  Vulkan UV has v = 0 at the
 * top of the texture and v = 1 at the bottom.  Both are
 * "top-origin" in the same direction, so a quad whose
 * top-left has UV (0, 0) and bottom-right has UV (1, 1)
 * renders the texture with no flip.
 *
 * Layout bindings:
 *   location 0:  vec2  position  (NDC, x in [-1, +1], y in [-1, +1])
 *   location 1:  vec2  uv        (texture-space, [0, 1] x [0, 1])
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 * Copyright (C) 2026 The libretro team
 */
#version 450

layout(location = 0) in vec2 v_pos;
layout(location = 1) in vec2 v_uv;

layout(location = 0) out vec2 f_uv;

void main()
{
    gl_Position = vec4(v_pos, 0.0, 1.0);
    f_uv        = v_uv;
}
