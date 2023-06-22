/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - osal_opengl.h                                           *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2013 Richard Goedeken                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#if !defined(OSAL_OPENGL_H)
#define OSAL_OPENGL_H

#if defined(_MSC_VER) && !defined(_XBOX)
#include <windows.h>
#elif defined(_XBOX)
#include <xtl.h>
#endif

#include <glsm/glsmsym.h>

#if defined(HAVE_OPENGLES2) // Desktop GL fix
#define GLSL_VERSION "100"
#else
#define GLSL_VERSION "120"
#endif

// Extension names
#define OSAL_GL_ARB_TEXTURE_ENV_ADD         "GL_texture_env_add"

// Vertex shader params
#define VS_POSITION                         0
#define VS_COLOR                            1
#define VS_TEXCOORD0                        2
#define VS_TEXCOORD1                        3
#define VS_FOG                              4

// Constant substitutions
#ifdef HAVE_OPENGLES2
#define GL_CLAMP                            GL_CLAMP_TO_EDGE
#define GL_MAX_TEXTURE_UNITS                GL_MAX_TEXTURE_IMAGE_UNITS

#define GL_ADD                              0x0104
#define GL_MODULATE                         0x2100
#define GL_INTERPOLATE                      0x8575
#define GL_CONSTANT                         0x8576
#define GL_PREVIOUS                         0x8578
#endif

// No-op substitutions (unavailable in GLES2)
#define glTexEnvi(x,y,z)
#define glTexEnvfv(x,y,z)

#endif // OSAL_OPENGL_H
