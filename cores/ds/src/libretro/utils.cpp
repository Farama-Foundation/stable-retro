#include <algorithm>

#include "utils.h"

int32_t Clamp(int32_t value, int32_t min, int32_t max)
{
   return std::max(min, std::min(max, value));
}

void copy_screen(ScreenLayoutData *data, uint32_t* src, unsigned offset)
{
   if (data->direct_copy)
   {
      memcpy((uint32_t *)data->buffer_ptr + offset, src, data->screen_width * data->screen_height * data->pixel_size);
   } else {
      unsigned y;
      for (y = 0; y < data->screen_height; y++)
      {
         memcpy((uint16_t *)data->buffer_ptr + offset + (y * data->screen_width * data->pixel_size),
            src + (y * data->screen_width), data->screen_width * data->pixel_size);
      }
   }
}

void copy_hybrid_screen(ScreenLayoutData *data, uint32_t* src, ScreenId screen_id)
{
   if (screen_id == ScreenId::Primary)
   {
      unsigned buffer_y, buffer_x;
      unsigned x, y, pixel;
      uint32_t pixel_data;
      unsigned buffer_height = data->screen_height * data->hybrid_ratio;
      unsigned buffer_width = data->screen_width * data->hybrid_ratio;

      for (buffer_y = 0; buffer_y < buffer_height; buffer_y++)
      {
         y = buffer_y / data->hybrid_ratio;
         for (buffer_x = 0; buffer_x < buffer_width; buffer_x++)
         {
            x = buffer_x / data->hybrid_ratio;

            pixel_data = *(uint32_t*)(src + (y * data->screen_width) + x);

            for (pixel = 0; pixel < data->hybrid_ratio; pixel++)
            {
               *(uint32_t *)(data->buffer_ptr + (buffer_y * data->buffer_stride / 2) + pixel * 2 + (buffer_x * 2)) = pixel_data;
            }
         }
      }
   }
   else if (screen_id == ScreenId::Top)
   {
      unsigned y;
      for (y = 0; y < data->screen_height; y++)
      {
         memcpy((uint16_t *)data->buffer_ptr
            // X
            + ((data->screen_width * data->hybrid_ratio * 2) + (data->hybrid_ratio % 2 == 0 ? data->hybrid_ratio : ((data->hybrid_ratio / 2) * 4)))
            // Y
            + (y * data->buffer_stride / 2),
            src + (y * data->screen_width), (data->screen_width) * data->pixel_size);
      }
   }
   else if (screen_id == ScreenId::Bottom)
   {
      unsigned y;
      for (y = 0; y < data->screen_height; y++)
      {
         memcpy((uint16_t *)data->buffer_ptr
            // X
            + ((data->screen_width * data->hybrid_ratio * 2) + (data->hybrid_ratio % 2 == 0 ? data->hybrid_ratio : ((data->hybrid_ratio / 2) * 4)))
            // Y
            + ((y + (data->screen_height * (data->hybrid_ratio - 1))) * data->buffer_stride / 2),
            src + (y * data->screen_width), (data->screen_width) * data->pixel_size);
      }
   }
}

void draw_cursor(ScreenLayoutData *data, int32_t x, int32_t y)
{
   uint32_t* base_offset = (uint32_t*)data->buffer_ptr;

   uint32_t scale = data->displayed_layout == ScreenLayout::HybridBottom ? data->hybrid_ratio : 1;

   uint32_t start_y = Clamp(y - CURSOR_SIZE, 0, data->screen_height) * scale;
   uint32_t end_y = Clamp(y + CURSOR_SIZE, 0, data->screen_height) * scale;

   for (uint32_t y = start_y; y < end_y; y++)
   {
      uint32_t start_x = Clamp(x - CURSOR_SIZE, 0, data->screen_width) * scale;
      uint32_t end_x = Clamp(x + CURSOR_SIZE, 0, data->screen_width) * scale;

      for (uint32_t x = start_x; x < end_x; x++)
      {
         uint32_t* offset = base_offset + ((y + data->touch_offset_y) * data->buffer_width) + ((x + data->touch_offset_x));
         uint32_t pixel = *offset;
         *(uint32_t*)offset = (0xFFFFFF - pixel) | 0xFF000000;
      }
   }
}
