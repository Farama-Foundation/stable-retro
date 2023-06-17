#include "libretro.h"
#include "libretro_perf.h"
#include "retro_inline.h"
#include "mame2003.h"
#include "palette.h"
#include "fileio.h"
#include "common.h"
#include "mame.h"
#include "usrintrf.h"
#include "driver.h"

#define MAX_LED 8

extern retro_log_printf_t log_cb;
extern retro_environment_t environ_cb;
extern retro_video_refresh_t video_cb;
extern retro_set_led_state_t led_state_cb;

/* Part of libretro's API */
extern int gotFrame;

extern struct RunningMachine *Machine;

/* Output bitmap settings */
struct osd_create_params video_config;
unsigned vis_width, vis_height;
unsigned tate_mode;

/* Output bitmap native conversion/transformation related vars */
unsigned video_conversion_type;
unsigned video_do_bypass;
unsigned video_stride_in, video_stride_out;
bool video_flip_x, video_flip_y, video_swap_xy;
bool video_hw_transpose;
const rgb_t *video_palette;
uint16_t *video_buffer;

/* Possible pixel conversions (see corresponding function far below) */
enum
{
   VCT_PASS8888,
   VCT_PASS1555,
   VCT_PASSPAL,
   VCT_PALTO565
};

/* Retrieve output geometry (i.e. window dimensions) */
void mame2003_video_get_geometry(struct retro_game_geometry *geom)
{
   /* Shorter variable names, for readability */
   unsigned max_w = video_config.width;
   unsigned max_h = video_config.height;
   unsigned vis_w = vis_width > 0 ? vis_width : max_w;
   unsigned vis_h = vis_height > 0 ? vis_height : max_h;

   /* Maximum dimensions must accomodate all image orientations */
   unsigned max_dim = max_w > max_h ? max_w : max_h;
   geom->max_width = geom->max_height = max_dim;

   /* Hardware rotations don't resize the framebuffer, adjust for that */
   geom->base_width = video_hw_transpose ? vis_h : vis_w;
   geom->base_height = video_hw_transpose ? vis_w : vis_h;

   geom->aspect_ratio = video_hw_transpose ? (float)video_config.aspect_y / (float)video_config.aspect_x : (float)video_config.aspect_x / (float)video_config.aspect_y;

}

void mame2003_video_update_visible_area(struct mame_display *display)
{
   struct retro_game_geometry geom = { 0 };

   struct rectangle visible_area = display->game_visible_area;
   vis_width = visible_area.max_x - visible_area.min_x + 1;
   vis_height = visible_area.max_y - visible_area.min_y + 1;

   /* Adjust for native XY swap */
   if (video_swap_xy)
   {
      unsigned temp;
      temp = vis_width; vis_width = vis_height; vis_height = temp;
   }

   /* Update MAME's own UI visible area */
   set_ui_visarea(
      visible_area.min_x, visible_area.min_y,
      visible_area.max_x, visible_area.max_y);

   /* Notify libretro of the change */
   mame2003_video_get_geometry(&geom);
   environ_cb(RETRO_ENVIRONMENT_SET_GEOMETRY, &geom);
}

/* Compute a reverse of a given orientation, accounting for XY swaps */
static unsigned reverse_orientation(unsigned orientation)
{
   int result = orientation;
   if (orientation & ORIENTATION_SWAP_XY)
   {
      result = ORIENTATION_SWAP_XY;
      if (orientation & ORIENTATION_FLIP_X)
         result ^= ORIENTATION_FLIP_Y;
      if (orientation & ORIENTATION_FLIP_Y)
         result ^= ORIENTATION_FLIP_X;
   }
   return result;
}

/* Init video orientation and geometry */
void mame2003_video_init_orientation(void)
{
   unsigned orientation = Machine->gamedrv->flags & ORIENTATION_MASK;
   unsigned rotate_mode;

   rotate_mode = 0; /* Known invalid value */
   tate_mode = options.tate_mode;   /* Acknowledge that the TATE mode is handled */
   video_hw_transpose = false;

   /* test RA if rotation is working if not dont alter the orientation let mame handle it */
   options.ui_orientation = reverse_orientation(orientation);

   if (tate_mode && (orientation & ORIENTATION_SWAP_XY))
      orientation = reverse_orientation(orientation) ^ ROT270;


   if (orientation == ROT0 || orientation == ROT90 || orientation == ROT180 || orientation == ROT270)
   {
      if (environ_cb(RETRO_ENVIRONMENT_SET_ROTATION, &rotate_mode) )
      {
        log_cb(RETRO_LOG_INFO, LOGPRE "RetroArch will perform the rotation.\n");

        rotate_mode = (orientation == ROT270) ? 1 : rotate_mode;
        rotate_mode = (orientation == ROT180) ? 2 : rotate_mode;
        rotate_mode = (orientation == ROT90) ? 3 : rotate_mode;
        if (orientation & ORIENTATION_SWAP_XY) video_hw_transpose = true; /*do this before the rotation reverse*/
        orientation = reverse_orientation(orientation ^ orientation); /* undo mame rotation if retroarch can do it */
        environ_cb(RETRO_ENVIRONMENT_SET_ROTATION, &rotate_mode);
      }

      else
        log_cb(RETRO_LOG_INFO, LOGPRE "This port of RetroArch does not support rotation or it has been disabled. Mame will rotate internally.\n");

   }
   else
     log_cb(RETRO_LOG_INFO, LOGPRE "RetroArch does not support this type of rotation, using mame internal rotation instead.\n");

   tate_mode = options.tate_mode;

   /* Set up native orientation flags that aren't handled by libretro */
   if (orientation & ORIENTATION_SWAP_XY) video_hw_transpose = true; /*dont set this to false if RA changes the flags else vertical games swap the xy*/
   video_flip_x = orientation & ORIENTATION_FLIP_X;
   video_flip_y = orientation & ORIENTATION_FLIP_Y;
   video_swap_xy = orientation & ORIENTATION_SWAP_XY;
   log_cb(RETRO_LOG_DEBUG,"mame internal: video_flip_x:%u video_flip_y:%u video_swap_xy:%u video_hw_transpose:%u\n",video_flip_x,video_flip_y,video_swap_xy,video_hw_transpose);
   Machine->ui_orientation = options.ui_orientation;


}

/* Init video format conversion settings */
void mame2003_video_init_conversion(UINT32 *rgb_components)
{
   unsigned color_mode;

   /* Case I: 16-bit indexed palette */
   if (video_config.depth == 16)
   {
      /* If a 6+ bits per color channel palette is used, do 32-bit XRGB8888 */
      if (video_config.video_attributes & VIDEO_NEEDS_6BITS_PER_GUN)
      {
         video_stride_in = 2; video_stride_out = 4;
         video_conversion_type = VCT_PASSPAL;
         color_mode = RETRO_PIXEL_FORMAT_XRGB8888;
      }
      /* Otherwise 16-bit RGB565 will suffice */
      else
      {
         video_stride_in = 2; video_stride_out = 2;
         video_conversion_type = VCT_PALTO565;
         color_mode = RETRO_PIXEL_FORMAT_RGB565;
      }
   }

   /* Case II: 32-bit XRGB8888, pass it through */
   else if (video_config.depth == 32)
   {
      video_stride_in = 4; video_stride_out = 4;
      video_conversion_type = VCT_PASS8888;
      color_mode = RETRO_PIXEL_FORMAT_XRGB8888;

      /* TODO: figure those out */
      rgb_components[0] = 0x00FF0000;
      rgb_components[1] = 0x0000FF00;
      rgb_components[2] = 0x000000FF;
   }

   /* Case III: 16-bit 0RGB1555, pass it through */
   else if (video_config.depth == 15)
   {
      video_stride_in = 2; video_stride_out = 2;
      video_conversion_type = VCT_PASS1555;
      color_mode = RETRO_PIXEL_FORMAT_0RGB1555;

      /* TODO: figure those out */
      rgb_components[0] = 0x00007C00;
      rgb_components[1] = 0x000003E0;
      rgb_components[2] = 0x0000001F;
   }

   /* Otherwise bail out on unknown video mode */
   else
   {
      if (log_cb)
         log_cb(RETRO_LOG_ERROR, "Unsupported color depth: %u\n", video_config.depth);
      abort();
   }

   environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &color_mode);
}

/* Do a soft reinit to process new video output parameters */
void mame2003_video_reinit(void)
{
   UINT32 rgb_components[3];
   struct osd_create_params old_params = video_config;
   osd_close_display();
   osd_create_display(&old_params, &rgb_components[0]);
}

int osd_create_display(
   const struct osd_create_params *params, UINT32 *rgb_components)
{
   memcpy(&video_config, params, sizeof(video_config));

   mame2003_video_init_orientation();
   mame2003_video_init_conversion(rgb_components);

   /* Check if a framebuffer conversion can be bypassed */
   video_do_bypass =
      !video_flip_x && !video_flip_y && !video_swap_xy &&
      ((video_config.depth == 15) || (video_config.depth == 32));
   /* Allocate an output video buffer, if necessary */
   if (!video_do_bypass)
   {
      video_buffer = malloc(video_config.width * video_config.height * video_stride_out);
      if (!video_buffer)
         return 1;
   }

   return 0;
}

void osd_close_display(void)
{
   free(video_buffer);
   video_buffer = NULL;
}

static INLINE void pix_convert_pass8888(uint32_t *from, uint32_t *to)
{
   *to = *from;
}

static INLINE void pix_convert_pass1555(uint16_t *from, uint16_t *to)
{
   *to = *from;
}

static INLINE void pix_convert_passpal(uint16_t *from, uint32_t *to)
{
   *to = video_palette[*from];
}

static INLINE void pix_convert_palto565(uint16_t *from, uint16_t *to)
{
   const uint32_t color = video_palette[*from];
   *to = (color & 0x00F80000) >> 8 | /* red */
         (color & 0x0000FC00) >> 5 | /* green */
         (color & 0x000000F8) >> 3;  /* blue */
}

static void frame_convert(struct mame_display *display)
{
   int x, y;

   bool flip_x = video_flip_x;
   bool flip_y = video_flip_y;

   struct rectangle visible_area = display->game_visible_area;
   int x0 = visible_area.min_x, y0 = visible_area.min_y;
   int x1 = visible_area.max_x, y1 = visible_area.max_y;
   int w = x1 - x0 + 1, h = y1 - y0 + 1;

   signed pitch = display->game_bitmap->rowpixels;
   char *input = (char*)display->game_bitmap->base;
   char *output = (char*)video_buffer;

   /* Pixel conversion loop macro for best possible inlining, w/o XY swap */
   #define CONVERT_NOSWAP(CONVERT_FUNC, TYPE_IN, TYPE_OUT, FLIP_X, FLIP_Y)\
      {\
         signed skip;\
         TYPE_IN *in = (TYPE_IN*)input;\
         TYPE_OUT *out = (TYPE_OUT*)output;\
         \
         /* Swaps are handled by iterating over input backwards */\
         in += (!FLIP_X ? x0 : x1) + (!FLIP_Y ? y0 * pitch : y1 * pitch);\
         /* After each line, reset the pointer to start, then to next line */\
         skip = (!FLIP_X ? -w : w) + (!FLIP_Y ? pitch : -pitch);\
         \
         for (y = 0; y < h; y++)\
         {\
            if (!FLIP_X)\
               for (x = 0; x < w; x++)\
                  CONVERT_FUNC(in++, out++);\
            else\
               for (x = 0; x < w; x++)\
                  CONVERT_FUNC(in--, out++);\
            in += skip;\
         }\
      }

   /* A much less optimized pixel conversion loop macro, with XY swap */
   #define CONVERT_SWAP(CONVERT_FUNC, TYPE_IN, TYPE_OUT, FLIP_X, FLIP_Y)\
      {\
         TYPE_IN *in = (TYPE_IN*)input;\
         TYPE_OUT *out = (TYPE_OUT*)output;\
         \
         for (x = FLIP_Y ? x1 : x0; FLIP_Y ? x >= x0 : x <= x1; x += FLIP_Y ? -1 : 1)\
            for (y = FLIP_X ? y1 : y0; FLIP_X ? y >= y0 : y <= y1; y += FLIP_X ? -1 : 1)\
               CONVERT_FUNC(&in[y * pitch + x], out++);\
      }

   /* Do a conversion accounting for XY flips */
   #define CONVERT_CHOOSE(CONVERT_MACRO, CONVERT_FUNC, TYPE_IN, TYPE_OUT)\
      {\
         if (!flip_x && !flip_y)\
            CONVERT_MACRO(CONVERT_FUNC, TYPE_IN, TYPE_OUT, false, false)\
         else if (!flip_x && flip_y)\
            CONVERT_MACRO(CONVERT_FUNC, TYPE_IN, TYPE_OUT, false, true)\
         else if (flip_x && !flip_y)\
            CONVERT_MACRO(CONVERT_FUNC, TYPE_IN, TYPE_OUT, true, false)\
         else if (flip_x && flip_y)\
            CONVERT_MACRO(CONVERT_FUNC, TYPE_IN, TYPE_OUT, true, true);\
      }

   /* Do a conversion accounting for XY swap */
   #define CONVERT(CONVERT_FUNC, TYPE_IN, TYPE_OUT)\
      if (!video_swap_xy)\
         CONVERT_CHOOSE(CONVERT_NOSWAP, CONVERT_FUNC, TYPE_IN, TYPE_OUT)\
      else\
         CONVERT_CHOOSE(CONVERT_SWAP, CONVERT_FUNC, TYPE_IN, TYPE_OUT)

   switch (video_conversion_type)
   {
      case VCT_PASS8888:
         CONVERT(pix_convert_pass8888, uint32_t, uint32_t);
         break;
      case VCT_PASS1555:
         CONVERT(pix_convert_pass1555, uint16_t, uint16_t);
         break;
      case VCT_PASSPAL:
         CONVERT(pix_convert_passpal, uint16_t, uint32_t);
         break;
      case VCT_PALTO565:
         CONVERT(pix_convert_palto565, uint16_t, uint16_t);
         break;
   }
}

extern bool retro_audio_buff_underrun;
extern bool retro_audio_buff_active;
extern unsigned retro_audio_buff_occupancy;
extern void (*pause_action)(void);

const int frameskip_table[12][12] =
   { { 0,0,0,0,0,0,0,0,0,0,0,0 },
     { 0,0,0,0,0,0,0,0,0,0,0,1 },
     { 0,0,0,0,0,1,0,0,0,0,0,1 },
     { 0,0,0,1,0,0,0,1,0,0,0,1 },
     { 0,0,1,0,0,1,0,0,1,0,0,1 },
     { 0,1,0,0,1,0,1,0,0,1,0,1 },
     { 0,1,0,1,0,1,0,1,0,1,0,1 },
     { 0,1,0,1,1,0,1,0,1,1,0,1 },
     { 0,1,1,0,1,1,0,1,1,0,1,1 },
     { 0,1,1,1,0,1,1,1,0,1,1,1 },
     { 0,1,1,1,1,1,0,1,1,1,1,1 },
     { 0,1,1,1,1,1,1,1,1,1,1,1 } };

 unsigned frameskip_counter = 0;
int osd_skip_this_frame(void)
{
	static unsigned auto_frameskip_counter = 0;

	bool skip_frame = 0;

	if (pause_action)  return 0;  /* dont skip pause action hack (rendering mame info screens or you wont see them and not know to press a key) */

/*auto frame skip options */
	if(options.frameskip >0 && options.frameskip >= 12)
	{
		if ( retro_audio_buff_active)
		{
			switch ( options.frameskip)
			{
				case 12: /* auto */
					skip_frame = retro_audio_buff_underrun;
				break;
				case 13: /* aggressive */
					skip_frame = (retro_audio_buff_occupancy < 33);
				break;
				case 14: /* max */
					skip_frame = (retro_audio_buff_occupancy < 50);
				break;
				default:
					skip_frame = false;
				break;
}
			if (skip_frame)
			{
				if(auto_frameskip_counter <= 40)
				{
					auto_frameskip_counter++;;
				}
				else
				{
					auto_frameskip_counter = 0;/* control will return 0 at the end */
					skip_frame=0;
				}
			}
		}
	}
	else /*manual frameskip includes disabled check */
	{
		skip_frame = frameskip_table[options.frameskip][frameskip_counter];
	}
	return skip_frame;
}

void osd_update_video_and_audio(struct mame_display *display)
{
   RETRO_PERFORMANCE_INIT(perf_cb, update_video_and_audio);
   RETRO_PERFORMANCE_START(perf_cb, update_video_and_audio);

   if(display->changed_flags &
      ( GAME_BITMAP_CHANGED | GAME_PALETTE_CHANGED
      | GAME_VISIBLE_AREA_CHANGED | VECTOR_PIXELS_CHANGED))
   {
      /* Reinit video output on TATE mode toggle */
      if (options.tate_mode != tate_mode)
      {
         mame2003_video_reinit();
         display->changed_flags |= GAME_VISIBLE_AREA_CHANGED;
         tate_mode = options.tate_mode;
      }

      /* Update the visible area */
      if (display->changed_flags & GAME_VISIBLE_AREA_CHANGED)
         mame2003_video_update_visible_area(display);

      /* Update the palette */
      if (display->changed_flags & GAME_PALETTE_CHANGED)
         video_palette = display->game_palette;

      /* Update the game bitmap */
      if (video_cb)
      {
         if (!osd_skip_this_frame())
         {
            if (video_do_bypass)
            {
               unsigned min_y = display->game_visible_area.min_y;
               unsigned min_x = display->game_visible_area.min_x;
               unsigned pitch = display->game_bitmap->rowpixels * video_stride_out;
               char *base = &((char*)display->game_bitmap->base)[min_y*pitch + min_x*video_stride_out];
               video_cb(base, vis_width, vis_height, pitch);
            }
            else
            {
               frame_convert(display);
               video_cb(video_buffer, vis_width, vis_height, vis_width * video_stride_out);
            }
         }
         else
            video_cb(NULL, vis_width, vis_height, vis_width * video_stride_out);
      }
   }

   /* Update LED indicators state */
   if (led_state_cb && display->changed_flags & LED_STATE_CHANGED)
   {
      static unsigned long prev_led_state = 0;
      unsigned long o = prev_led_state;
      unsigned long n = display->led_state;
      int led;
      for(led=0;led<MAX_LED;led++)
      {
         if((o & 0x01) != (n & 0x01))
         {
            led_state_cb(led,n&0x01);
         }
         o >>= 1; n >>= 1;
      }
      prev_led_state = display->led_state;
   }

   gotFrame = 1;

   frameskip_counter = (frameskip_counter + 1) % 12;
   RETRO_PERFORMANCE_STOP(perf_cb, update_video_and_audio);
}

struct mame_bitmap *osd_override_snapshot(struct mame_bitmap *bitmap, struct rectangle *bounds)
{
   return NULL;
}
