/*
* Glide64 - Glide video plugin for Nintendo 64 emulators.
* Copyright (c) 2002  Dave2001
* Copyright (c) 2003-2009  Sergey 'Gonetz' Lipski
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include "glide.h"
#include "glitchmain.h"
#include "../Glide64/rdp.h"

/* TODO: get rid of glitch_vbo */
/* TODO: try glDrawElements */
/* TODO: investigate triangle degeneration to allow caching GL_TRIANGLE_STRIP */

/* This structure is a truncated version of VERTEX, we use it to lower memory
 * usage and speed up the caching process. */
typedef struct
{
  float    x;
  float    y;
  float    z;
  float    q;
  uint8_t  b;
  uint8_t  g;
  uint8_t  r;
  uint8_t  a;
  float    coord[4];
  float    fog;
} VBufVertex;

#define VERTEX_BUFFER_SIZE 1500

static VBufVertex vbuf_data[VERTEX_BUFFER_SIZE];
static GLenum     vbuf_primitive = GL_TRIANGLES;
static unsigned   vbuf_length    = 0;
static bool       vbuf_use_vbo   = false;
static bool       vbuf_enabled   = false;
static GLuint     vbuf_vbo       = 0;
static size_t     vbuf_vbo_size  = 0;
static bool       vbuf_drawing   = false;

extern retro_environment_t environ_cb;

void vbo_init(void)
{
   struct retro_variable var = { "mupen64-vcache-vbo", 0 };
   vbuf_use_vbo = false;
   vbuf_length = 0;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
      vbuf_use_vbo = (strcmp(var.value, "enabled") == 0);

   if (vbuf_use_vbo)
   {
      glGenBuffers(1, &vbuf_vbo);

      if (!vbuf_vbo)
      {
         log_cb(RETRO_LOG_ERROR, "Failed to create the VBO.");
         vbuf_use_vbo = false;
      }
      else
         log_cb(RETRO_LOG_INFO, "Vertex cache VBO enabled.\n");
   }
}

void vbo_free(void)
{
   if (vbuf_vbo)
      glDeleteBuffers(1, &vbuf_vbo);

   vbuf_vbo      = 0;
   vbuf_vbo_size = 0;

   vbuf_length  = 0;
   vbuf_enabled = false;
   vbuf_drawing = false;
}

void vbo_bind()
{
   if (vbuf_vbo)
      glBindBuffer(GL_ARRAY_BUFFER, vbuf_vbo);
}

void vbo_unbind()
{
   if (vbuf_vbo)
      glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void vbo_buffer_data(void *data, size_t size)
{
   if (vbuf_vbo)
   {
      if (size > vbuf_vbo_size)
      {
         glBufferData(GL_ARRAY_BUFFER, size, data, GL_DYNAMIC_DRAW);

         if (size > VERTEX_BUFFER_SIZE)
            log_cb(RETRO_LOG_INFO, "Extending vertex cache VBO.\n");

         vbuf_vbo_size = size;
      }
      else
         glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
   }
}

void vbo_draw(void)
{
   if (!vbuf_length || vbuf_drawing)
      return;

   /* avoid infinite loop in sgl*BindBuffer */
   vbuf_drawing = true;

   if (vbuf_vbo)
   {
      glBindBuffer(GL_ARRAY_BUFFER, vbuf_vbo);

      glBufferSubData(GL_ARRAY_BUFFER, 0, vbuf_length * sizeof(VBufVertex), vbuf_data);

      glDrawArrays(vbuf_primitive, 0, vbuf_length);
      glBindBuffer(GL_ARRAY_BUFFER, 0);
   }
   else
      glDrawArrays(vbuf_primitive, 0, vbuf_length);

   vbuf_length = 0;
   vbuf_drawing = false;
}

static void vbo_append(GLenum mode, GLsizei count, void *pointers)
{
   if (vbuf_length + count > VERTEX_BUFFER_SIZE)
     vbo_draw();

   /* keep caching triangles as much as possible. */
   if (count == 3 && vbuf_primitive == GL_TRIANGLES)
      mode = GL_TRIANGLES;

   while (count--)
   {
      memcpy(&vbuf_data[vbuf_length++], pointers, sizeof(VBufVertex));
      pointers = (char*)pointers + sizeof(VERTEX);
   }

   vbuf_primitive = mode;

   /* we can't handle anything but triangles so flush it */
   if (mode != GL_TRIANGLES)
      vbo_draw();
}

void vbo_enable(void)
{
   const void *vp   = NULL;
   const void *vc   = NULL;
   const void *tc0  = NULL;
   const void *tc1  = NULL;
   const void *fog  = NULL;
   bool was_drawing = vbuf_drawing;

   if (vbuf_enabled)
      return;

   vbuf_drawing = true;

   if (vbuf_vbo)
   {
      glBindBuffer(GL_ARRAY_BUFFER, vbuf_vbo);
      if (vbuf_vbo_size < VERTEX_BUFFER_SIZE * sizeof(VBufVertex))
         vbo_buffer_data(NULL, VERTEX_BUFFER_SIZE * sizeof(VBufVertex));

      vp  = (void*)offsetof(VBufVertex, x);
      vc  = (void*)offsetof(VBufVertex, b);
      tc0 = (void*)offsetof(VBufVertex, coord[2]);
      tc1 = (void*)offsetof(VBufVertex, coord[0]);
      fog = (void*)offsetof(VBufVertex, fog);
   }
   else
   {
      vp  = &vbuf_data->x;
      vc  = &vbuf_data->b;
      tc0 = &vbuf_data->coord[2];
      tc1 = &vbuf_data->coord[0];
      fog = &vbuf_data->fog;
   }

   glEnableVertexAttribArray(POSITION_ATTR);
   glEnableVertexAttribArray(COLOUR_ATTR);
   glEnableVertexAttribArray(TEXCOORD_0_ATTR);
   glEnableVertexAttribArray(TEXCOORD_1_ATTR);
   glEnableVertexAttribArray(FOG_ATTR);
   glVertexAttribPointer(POSITION_ATTR,   4, GL_FLOAT,         false,  sizeof(VBufVertex), (void*)vp);
   glVertexAttribPointer(COLOUR_ATTR,     4, GL_UNSIGNED_BYTE, true,   sizeof(VBufVertex), (void*)vc);
   glVertexAttribPointer(TEXCOORD_0_ATTR, 2, GL_FLOAT,         false, sizeof(VBufVertex), (void*)tc0);
   glVertexAttribPointer(TEXCOORD_1_ATTR, 2, GL_FLOAT,         false, sizeof(VBufVertex), (void*)tc1);
   glVertexAttribPointer(FOG_ATTR,        1, GL_FLOAT,         false, sizeof(VBufVertex), (void*)fog);

   if (vbuf_vbo)
      glBindBuffer(GL_ARRAY_BUFFER, 0);

   vbuf_enabled = true;

   vbuf_drawing = was_drawing;
}

void vbo_disable(void)
{
   vbo_draw();
   vbuf_enabled = false;
}

void grCullMode( int32_t mode )
{
   switch(mode)
   {
      case GR_CULL_DISABLE:
         glDisable(GL_CULL_FACE);
         break;
      case GR_CULL_NEGATIVE:
         glCullFace(GL_FRONT);
         glEnable(GL_CULL_FACE);
         break;
      case GR_CULL_POSITIVE:
         glCullFace(GL_BACK);
         glEnable(GL_CULL_FACE);
         break;
   }
}

// Depth buffer

void grDepthBufferFunction(GLenum func)
{
   glDepthFunc(func);
}

void grDepthMask(bool mask)
{
   glDepthMask(mask);
}

extern float polygonOffsetFactor;
extern float polygonOffsetUnits;

void grDepthBiasLevel( int32_t level )
{
   if (level)
   {
      glPolygonOffset(polygonOffsetFactor, (float)level * settings.depth_bias * 0.01f);
      glEnable(GL_POLYGON_OFFSET_FILL);
   }
   else
   {
      glPolygonOffset(0,0);
      glDisable(GL_POLYGON_OFFSET_FILL);
   }
}

void grDrawVertexArrayContiguous(uint32_t mode, uint32_t count, void *pointers)
{
   if(need_to_compile)
      compile_shader();

   vbo_enable();
   vbo_append(mode, count, pointers);
}

void init_geometry()
{
   vbo_init();
}

void free_geometry()
{
   vbo_free();
}
