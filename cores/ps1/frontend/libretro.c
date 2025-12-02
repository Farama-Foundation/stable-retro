/*
 * (C) notaz, 2012,2014,2015
 *
 * This work is licensed under the terms of the GNU GPLv2 or later.
 * See the COPYING file in the top-level directory.
 */

#define _GNU_SOURCE 1 // strcasestr
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <assert.h>
#ifdef __MACH__
#include <unistd.h>
#include <sys/syscall.h>
#endif

#include "retro_miscellaneous.h"
#ifdef SWITCH
#include <switch.h>
#endif

#include "../libpcsxcore/misc.h"
#include "../libpcsxcore/psxcounters.h"
#include "../libpcsxcore/psxmem_map.h"
#include "../libpcsxcore/new_dynarec/new_dynarec.h"
#include "../libpcsxcore/cdrom.h"
#include "../libpcsxcore/cdrom-async.h"
#include "../libpcsxcore/cdriso.h"
#include "../libpcsxcore/cheat.h"
#include "../libpcsxcore/r3000a.h"
#include "../libpcsxcore/gpu.h"
#include "../libpcsxcore/database.h"
#include "../plugins/dfsound/out.h"
#include "../plugins/dfsound/spu_config.h"
#include "cspace.h"
#include "main.h"
#include "menu.h"
#include "plugin.h"
#include "plugin_lib.h"
#include "arm_features.h"
#include "revision.h"

#include <libretro.h>
#include "libretro_core_options.h"

#ifdef USE_LIBRETRO_VFS
#include <streams/file_stream_transforms.h>
#endif

#ifdef _3DS
#include <3ds/svc.h>
#include <3ds/services/apt.h>
#include <3ds/allocator/linear.h>
#include "3ds/3ds_utils.h"
#endif

#ifndef MAP_FAILED
#define MAP_FAILED      ((void *)(intptr_t)-1)
#endif

#define PORTS_NUMBER 8

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#define ISHEXDEC ((buf[cursor] >= '0') && (buf[cursor] <= '9')) || ((buf[cursor] >= 'a') && (buf[cursor] <= 'f')) || ((buf[cursor] >= 'A') && (buf[cursor] <= 'F'))

//hack to prevent retroarch freezing when reseting in the menu but not while running with the hot key
static int rebootemu = 0;

static retro_video_refresh_t video_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;
static retro_environment_t environ_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_set_rumble_state_t rumble_cb;
static struct retro_log_callback logging;
static retro_log_printf_t log_cb;

#define LogWarn(fmt, ...) do { \
   if (log_cb) log_cb(RETRO_LOG_WARN, fmt, ##__VA_ARGS__); \
} while (0)
#define LogErr(fmt, ...) do { \
   if (log_cb) log_cb(RETRO_LOG_ERROR, fmt, ##__VA_ARGS__); \
} while (0)

static unsigned msg_interface_version = 0;

static void *vout_buf;
static void *vout_buf_ptr;
static int vout_width = 256, vout_height = 240, vout_pitch_b = 256*2;
static int vout_fb_dirty;
static int psx_w, psx_h;
static bool vout_can_dupe;
static bool found_bios;
static int display_internal_fps;
static bool libretro_supports_bitmasks = false;
static bool libretro_supports_option_categories = false;
static bool show_input_settings = true;
#ifdef GPU_PEOPS
static bool show_advanced_gpu_peops_settings = true;
#endif
#ifdef GPU_UNAI
static bool show_advanced_gpu_unai_settings = true;
#endif
static float mouse_sensitivity = 1.0f;
static unsigned int disk_current_index;

typedef enum
{
   FRAMESKIP_NONE = 0,
   FRAMESKIP_AUTO,
   FRAMESKIP_AUTO_THRESHOLD,
   FRAMESKIP_FIXED_INTERVAL
} frameskip_type_t;

static unsigned frameskip_type                  = FRAMESKIP_NONE;
static unsigned frameskip_threshold             = 0;
static unsigned frameskip_interval              = 0;
static unsigned frameskip_counter               = 0;

static int retro_audio_buff_active              = false;
static unsigned retro_audio_buff_occupancy      = 0;
static int retro_audio_buff_underrun            = false;

static unsigned retro_audio_latency             = 0;
static int update_audio_latency                 = false;

static unsigned int current_width;
static unsigned int current_height;
static enum retro_pixel_format current_fmt;

static int plugins_opened;

#define is_pal_mode Config.PsxType

/* memory card data */
extern char Mcd1Data[MCD_SIZE];
extern char Mcd2Data[MCD_SIZE];
extern char McdDisable[2];

/* PCSX ReARMed core calls and stuff */
int in_type[8] = {
   PSE_PAD_TYPE_NONE, PSE_PAD_TYPE_NONE,
   PSE_PAD_TYPE_NONE, PSE_PAD_TYPE_NONE,
   PSE_PAD_TYPE_NONE, PSE_PAD_TYPE_NONE,
   PSE_PAD_TYPE_NONE, PSE_PAD_TYPE_NONE
};
int in_analog_left[8][2] = { { 127, 127 }, { 127, 127 }, { 127, 127 }, { 127, 127 }, { 127, 127 }, { 127, 127 }, { 127, 127 }, { 127, 127 } };
int in_analog_right[8][2] = { { 127, 127 }, { 127, 127 }, { 127, 127 }, { 127, 127 }, { 127, 127 }, { 127, 127 }, { 127, 127 }, { 127, 127 } };
unsigned short in_keystate[PORTS_NUMBER];
int in_mouse[8][2];
int multitap1 = 0;
int multitap2 = 0;
int in_enable_vibration = 1;
static int in_enable_crosshair[2] = { 0, 0 };
static int in_dualshock_analog_combo = 0;
static bool in_dualshock_toggling = false;

// NegCon adjustment parameters
// > The NegCon 'twist' action is somewhat awkward when mapped
//   to a standard analog stick -> user should be able to tweak
//   response/deadzone for comfort
// > When response is linear, 'additional' deadzone (set here)
//   may be left at zero, since this is normally handled via in-game
//   options menus
// > When response is non-linear, deadzone should be set to match the
//   controller being used (otherwise precision may be lost)
// > negcon_linearity:
//   - 1: Response is linear - recommended when using racing wheel
//        peripherals, not recommended for standard gamepads
//   - 2: Response is quadratic - optimal setting for gamepads
//   - 3: Response is cubic - enables precise fine control, but
//        difficult to use...
#define NEGCON_RANGE 0x7FFF
static int negcon_deadzone = 0;
static int negcon_linearity = 1;

static bool axis_bounds_modifier;

/* PSX max resolution is 640x512, but with enhancement it's 1024x512 */
#ifdef GPU_NEON
#define VOUT_MAX_WIDTH  1024
#else
#define VOUT_MAX_WIDTH  640
#endif
#define VOUT_MAX_HEIGHT 512

//Dummy functions
bool retro_load_game_special(unsigned game_type, const struct retro_game_info *info, size_t num_info) { return false; }
void retro_unload_game(void) {}
static int vout_open(void) { return 0; }
static void vout_close(void) {}
static int snd_init(void) { return 0; }
static void snd_finish(void) {}
static int snd_busy(void) { return 0; }

#define GPU_PEOPS_ODD_EVEN_BIT         (1 << 0)
#define GPU_PEOPS_EXPAND_SCREEN_WIDTH  (1 << 1)
#define GPU_PEOPS_IGNORE_BRIGHTNESS    (1 << 2)
#define GPU_PEOPS_DISABLE_COORD_CHECK  (1 << 3)
#define GPU_PEOPS_LAZY_SCREEN_UPDATE   (1 << 6)
#define GPU_PEOPS_OLD_FRAME_SKIP       (1 << 7)
#define GPU_PEOPS_REPEATED_TRIANGLES   (1 << 8)
#define GPU_PEOPS_QUADS_WITH_TRIANGLES (1 << 9)
#define GPU_PEOPS_FAKE_BUSY_STATE      (1 << 10)

static void init_memcard(char *mcd_data)
{
   unsigned off = 0;
   unsigned i;

   memset(mcd_data, 0, MCD_SIZE);

   mcd_data[off++] = 'M';
   mcd_data[off++] = 'C';
   off += 0x7d;
   mcd_data[off++] = 0x0e;

   for (i = 0; i < 15; i++)
   {
      mcd_data[off++] = 0xa0;
      off += 0x07;
      mcd_data[off++] = 0xff;
      mcd_data[off++] = 0xff;
      off += 0x75;
      mcd_data[off++] = 0xa0;
   }

   for (i = 0; i < 20; i++)
   {
      mcd_data[off++] = 0xff;
      mcd_data[off++] = 0xff;
      mcd_data[off++] = 0xff;
      mcd_data[off++] = 0xff;
      off += 0x04;
      mcd_data[off++] = 0xff;
      mcd_data[off++] = 0xff;
      off += 0x76;
   }
}

static void bgr_to_fb_empty(void *dst, const void *src, int bytes)
{
}

typedef void (bgr_to_fb_func)(void *dst, const void *src, int bytes);
static bgr_to_fb_func *g_bgr_to_fb = bgr_to_fb_empty;

static void set_bgr_to_fb_func(int bgr24)
{
   switch (current_fmt)
   {
   case RETRO_PIXEL_FORMAT_XRGB8888:
      g_bgr_to_fb = bgr24 ? bgr888_to_xrgb8888 : bgr555_to_xrgb8888;
      break;
   case RETRO_PIXEL_FORMAT_RGB565:
      g_bgr_to_fb = bgr24 ? bgr888_to_rgb565 : bgr555_to_rgb565;
      break;
   default:
      LogErr("unsupported current_fmt: %d\n", current_fmt);
      g_bgr_to_fb = bgr_to_fb_empty;
      break;
   }
}

static void set_vout_fb(void)
{
   struct retro_framebuffer fb = { 0 };
   bool ret;

   fb.width          = vout_width;
   fb.height         = vout_height;
   fb.access_flags   = RETRO_MEMORY_ACCESS_WRITE;

   ret = environ_cb(RETRO_ENVIRONMENT_GET_CURRENT_SOFTWARE_FRAMEBUFFER, &fb);
   if (ret && vout_can_dupe &&
       (fb.format == RETRO_PIXEL_FORMAT_RGB565 || fb.format == RETRO_PIXEL_FORMAT_XRGB8888))
   {
      int bytes_pp = (fb.format == RETRO_PIXEL_FORMAT_XRGB8888) ? 4 : 2;
      if (current_fmt != fb.format) {
         LogWarn("fb.format changed: %d->%d\n", current_fmt, fb.format);
         current_fmt = fb.format;
      }
      vout_buf_ptr = fb.data;
      vout_pitch_b = fb.pitch;
      if (fb.pitch != vout_width * bytes_pp)
         LogWarn("got unusual pitch %zd for fmt %d resolution %dx%d\n",
               fb.pitch, fb.format, vout_width, vout_height);
   }
   else
   {
      int bytes_pp = (current_fmt == RETRO_PIXEL_FORMAT_XRGB8888) ? 4 : 2;
      vout_buf_ptr = vout_buf;
      vout_pitch_b = vout_width * bytes_pp;
   }
}

static void vout_set_mode(int w, int h, int raw_w, int raw_h, int bpp)
{
   vout_width = w;
   vout_height = h;
   psx_w = raw_w;
   psx_h = raw_h;

   /* it may seem like we could do RETRO_ENVIRONMENT_SET_PIXEL_FORMAT here to
    * switch to something that can accommodate bgr24 for FMVs, but although it
    * succeeds it doesn't actually change the format at least on Linux, and the
    * docs say only retro_load_game() can do it */

   if (current_width != vout_width || current_height != vout_height)
   {
      current_width = vout_width;
      current_height = vout_height;

      struct retro_system_av_info info;
      retro_get_system_av_info(&info);
      environ_cb(RETRO_ENVIRONMENT_SET_GEOMETRY, &info.geometry);
   }

   set_vout_fb();
   set_bgr_to_fb_func(bpp == 24);
}

// Function to add crosshairs
static void addCrosshair(int port, int crosshair_color, unsigned short *buffer, int bufferStride, int pos_x, int pos_y, int thickness, int size_x, int size_y)
{
   for (port = 0; port < 2; port++) {
      // Draw the horizontal line of the crosshair
      int i, j;
      for (i = pos_y - thickness / 2; i <= pos_y + thickness / 2; i++) {
         for (j = pos_x - size_x / 2; j <= pos_x + size_x / 2; j++) {
            if ((i + vout_height) >= 0 && (i + vout_height) < bufferStride && j >= 0 && j < bufferStride && in_enable_crosshair[port] > 0)
               buffer[i * bufferStride + j] = crosshair_color;
         }
      }

      // Draw the vertical line of the crosshair
      for (i = pos_x - thickness / 2; i <= pos_x + thickness / 2; i++) {
         for (j = pos_y - size_y / 2; j <= pos_y + size_y / 2; j++) {
            if (i >= 0 && i < bufferStride && (j + vout_height) >= 0 && (j + vout_height) < bufferStride && in_enable_crosshair[port] > 0)
               buffer[j * bufferStride + i] = crosshair_color;
         }
      }
   }
}

struct CrosshairInfo {
   int pos_x, pos_y, thickness, size_x, size_y;
};

// Calculate size and position of crosshairs
static void CrosshairDimensions(int port, struct CrosshairInfo *info) {
   int gunx = input_state_cb(port, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_SCREEN_X);
   int guny = input_state_cb(port, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_SCREEN_Y);
   if (gunx == 32767) // Prevent crosshairs from wrapping around right side of screen to left
      info->pos_x = (gunx + 32767.0f) * vout_width / 65534.0f - 0.5f;
   else
      info->pos_x = (gunx + 32767.0f) * vout_width / 65534.0f;
   info->pos_y = (guny + 32767.0f) * vout_height / 65534.0f - vout_height;
   info->thickness = pl_rearmed_cbs.gpu_neon.enhancement_enable ? 4 : 2;
   info->size_x = psx_w * (pl_rearmed_cbs.gpu_neon.enhancement_enable ? 2 : 1) / 40.0f;
   info->size_y = psx_h * (pl_rearmed_cbs.gpu_neon.enhancement_enable ? 2 : 1) * (4.0f / 3.0f) / 40.0f;
}

static void vout_flip(const void *vram_, int vram_ofs, int bgr24,
      int x, int y, int w, int h, int dims_changed)
{
   int bytes_pp = (current_fmt == RETRO_PIXEL_FORMAT_XRGB8888) ? 4 : 2;
   int bytes_pp_s = bgr24 ? 3 : 2;
   bgr_to_fb_func *bgr_to_fb = g_bgr_to_fb;
   unsigned char *dest = vout_buf_ptr;
   const unsigned char *vram = vram_;
   int dstride = vout_pitch_b, h1 = h;
   int enhres = w > psx_w;
   u32 vram_mask = enhres ? ~0 : 0xfffff;
   int port = 0, hwrapped;

   if (vram == NULL || dims_changed || (in_enable_crosshair[0] + in_enable_crosshair[1]) > 0)
   {
      unsigned char *dest2 = dest;
      int h2 = h, ll = vout_width * bytes_pp;
      if (dstride == ll)
         memset(dest2, 0, dstride * vout_height);
      else
         for (; h2-- > 0; dest2 += dstride)
            memset(dest2, 0, ll);
      // blanking
      if (vram == NULL)
         goto out;
   }

   dest += x * bytes_pp + y * dstride;

   for (; h1-- > 0; dest += dstride) {
      bgr_to_fb(dest, vram + vram_ofs, w * bytes_pp_s);
      vram_ofs = (vram_ofs + 2048) & vram_mask;
   }

   hwrapped = (vram_ofs & 2047) + w * bytes_pp_s - 2048;
   if (!enhres && hwrapped > 0) {
      // this is super-rare so just fix-up
      vram_ofs = (vram_ofs - h * 2048) & 0xff800;
      dest -= dstride * h;
      dest += (w - hwrapped / bytes_pp_s) * bytes_pp;
      for (h1 = h; h1-- > 0; dest += dstride) {
         bgr_to_fb(dest, vram + vram_ofs, hwrapped);
         vram_ofs = (vram_ofs + 2048) & 0xfffff;
      }
   }

   if (current_fmt == RETRO_PIXEL_FORMAT_RGB565)
   for (port = 0; port < 2; port++) {
      if (in_enable_crosshair[port] > 0 && (in_type[port] == PSE_PAD_TYPE_GUNCON || in_type[port] == PSE_PAD_TYPE_GUN))
      {
         struct CrosshairInfo crosshairInfo;
         CrosshairDimensions(port, &crosshairInfo);
         addCrosshair(port, in_enable_crosshair[port], (unsigned short *)dest,
               dstride / 2, crosshairInfo.pos_x, crosshairInfo.pos_y,
               crosshairInfo.thickness, crosshairInfo.size_x, crosshairInfo.size_y);
      }
   }

out:
   vout_fb_dirty = 1;
   pl_rearmed_cbs.flip_cnt++;
}

#ifdef _3DS
static u32 mapped_addrs[8];
static u32 mapped_ram, mapped_ram_src;
static void *vram_mem;

// http://3dbrew.org/wiki/Memory_layout#ARM11_User-land_memory_regions
static void *pl_3ds_mmap(unsigned long addr, size_t size,
    enum psxMapTag tag, int *can_retry_addr)
{
   void *ret = MAP_FAILED;
   *can_retry_addr = 0;
   (void)addr;

   if (tag == MAP_TAG_VRAM && vram_mem)
      return vram_mem;

   if (__ctr_svchax) do
   {
      // idea from fbalpha2012_neogeo
      s32 addr = 0x10000000 - 0x1000;
      u32 found_addr = 0;
      MemInfo mem_info;
      PageInfo page_info;
      size_t i;
      int r;

      for (i = 0; i < sizeof(mapped_addrs) / sizeof(mapped_addrs[0]); i++)
         if (mapped_addrs[i] == 0)
            break;
      if (i == sizeof(mapped_addrs) / sizeof(mapped_addrs[0]))
         break;

      size = (size + 0xfff) & ~0xfff;

      while (addr >= 0x08000000)
      {
         if ((r = svcQueryMemory(&mem_info, &page_info, addr)) < 0) {
            LogErr("svcQueryMemory failed: %d\n", r);
            break;
         }

         if (mem_info.state == MEMSTATE_FREE && mem_info.size >= size) {
            found_addr = mem_info.base_addr + mem_info.size - size;
            break;
         }

         addr = mem_info.base_addr - 0x1000;
      }
      if (found_addr == 0) {
         LogErr("no addr space for %u bytes\n", size);
         break;
      }

      // https://libctru.devkitpro.org/svc_8h.html#a8046e9b23b1b209a4e278cb1c19c7a5a
      if ((r = svcControlMemory(&mapped_addrs[i], found_addr, 0, size, MEMOP_ALLOC, MEMPERM_READWRITE)) < 0) {
         LogErr("svcControlMemory failed for %08x %u: %d\n", found_addr, size, r);
         break;
      }
      if (mapped_addrs[i] == 0) // needed?
         mapped_addrs[i] = found_addr;
      ret = (void *)mapped_addrs[i];

      // "round" address helps the dynarec slightly, map ram at 0x13000000
      if (tag == MAP_TAG_RAM && !mapped_ram) {
         u32 target = 0x13000000;
         if ((r = svcControlMemory(&mapped_ram, target, mapped_addrs[i], size, MEMOP_MAP, MEMPERM_READWRITE)) < 0)
            LogErr("could not map ram %08x -> %08x: %d\n", mapped_addrs[i], target, r);
         else {
            mapped_ram_src = mapped_addrs[i];
            mapped_ram = target;
            ret = (void *)mapped_ram;
         }
      }
      memset(ret, 0, size);
      return ret;
   }
   while (0);

   ret = calloc(size, 1);
   return ret ? ret : MAP_FAILED;
}

static void pl_3ds_munmap(void *ptr, size_t size, enum psxMapTag tag)
{
   (void)tag;

   if (ptr && ptr == vram_mem)
      return;

   if (ptr && __ctr_svchax)
   {
      size_t i;
      u32 tmp;

      size = (size + 0xfff) & ~0xfff;

      if (ptr == (void *)mapped_ram) {
         svcControlMemory(&tmp, mapped_ram, mapped_ram_src, size, MEMOP_UNMAP, 0);
         ptr = (void *)mapped_ram_src;
         mapped_ram = mapped_ram_src = 0;
      }
      for (i = 0; i < sizeof(mapped_addrs) / sizeof(mapped_addrs[0]); i++) {
         if (ptr == (void *)mapped_addrs[i]) {
            svcControlMemory(&tmp, mapped_addrs[i], 0, size, MEMOP_FREE, 0);
            mapped_addrs[i] = 0;
            return;
         }
      }
   }

   free(ptr);
}

// debug
static int ctr_get_tlbe_k(u32 ptr)
{
   u32 tlb_base = -1, tlb_ctl = -1, *l1;
   s32 tlb_mask = 0xffffc000;

   asm volatile("mrc p15, 0, %0, c2, c0, 0" : "=r"(tlb_base));
   asm volatile("mrc p15, 0, %0, c2, c0, 2" : "=r"(tlb_ctl));
   tlb_mask >>= tlb_ctl & 7;
   l1 = (u32 *)((tlb_base & tlb_mask) | 0xe0000000);
   return l1[ptr >> 20];
}

static int ctr_get_tlbe(void *ptr)
{
   if (svcConvertVAToPA((void *)0xe0000000, 0) != 0x20000000)
      return -1;
   return svcCustomBackdoor(ctr_get_tlbe_k, ptr, NULL, NULL);
}
#endif

#ifdef HAVE_LIBNX
static void *pl_switch_mmap(unsigned long addr, size_t size,
    enum psxMapTag tag, int *can_retry_addr)
{
   void *ret = MAP_FAILED;
   *can_retry_addr = 0;
   (void)addr;

   // there's svcMapPhysicalMemory() but user logs show it doesn't hand out
   // any desired addresses, so don't even bother
   ret = aligned_alloc(0x1000, size);
   if (!ret)
      return MAP_FAILED;
   memset(ret, 0, size);
   return ret;
}

static void pl_switch_munmap(void *ptr, size_t size, enum psxMapTag tag)
{
   (void)size;
   (void)tag;
   free(ptr);
}
#endif

#ifdef VITA
typedef struct
{
   void *buffer;
   size_t size;
   enum psxMapTag tag;
   int used;
} psx_map_t;

static void *addr = NULL;

static psx_map_t custom_psx_maps[] = {
   { NULL, 0x800000, MAP_TAG_LUTS },
   { NULL, 0x080000, MAP_TAG_OTHER },
   { NULL, 0x010000, MAP_TAG_OTHER },
   { NULL, 0x201000, MAP_TAG_VRAM },
   { NULL, 0x802000, MAP_TAG_VRAM }, // enhanced renderer
   { NULL, 0x210000, MAP_TAG_RAM },
};

static int init_vita_mmap()
{
   void *tmpaddr;
   addr = malloc(64 * 1024 * 1024);
   if (addr == NULL)
      return -1;
   tmpaddr = (void *)(((size_t)addr + 0xFFFFFF) & ~0xFFFFFF);
   custom_psx_maps[0].buffer = tmpaddr + 0x0000000;
   custom_psx_maps[1].buffer = tmpaddr + 0x0800000;
   custom_psx_maps[2].buffer = tmpaddr + 0x0880000;
   custom_psx_maps[3].buffer = tmpaddr + 0x0900000;
   custom_psx_maps[4].buffer = tmpaddr + 0x1000000;
   custom_psx_maps[5].buffer = tmpaddr + 0x2000000;
   memset(tmpaddr, 0, 0x2210000);
#if 0
   int n;
   for(n = 0; n < 5; n++){
   sceClibPrintf("addr reserved %x\n",custom_psx_maps[n].buffer);
   }
#endif
   return 0;
}

static void deinit_vita_mmap()
{
   size_t i;
   for (i = 0; i < sizeof(custom_psx_maps) / sizeof(custom_psx_maps[0]); i++) {
      custom_psx_maps[i].buffer = NULL;
      custom_psx_maps[i].used = 0;
   }
   free(addr);
}

static void *pl_vita_mmap(unsigned long addr, size_t size,
    enum psxMapTag tag, int *can_retry_addr)
{
   void *ret;
   (void)addr;
   *can_retry_addr = 0;

   psx_map_t *custom_map = custom_psx_maps;

   for (; custom_map->size; custom_map++)
   {
      if (custom_map->size == size && custom_map->tag == tag && !custom_map->used)
      {
         custom_map->used = 1;
         return custom_map->buffer;
      }
   }

   ret = calloc(size, 1);
   return ret ? ret : MAP_FAILED;
}

static void pl_vita_munmap(void *ptr, size_t size, enum psxMapTag tag)
{
   (void)tag;

   psx_map_t *custom_map = custom_psx_maps;

   for (; custom_map->size; custom_map++)
   {
      if ((custom_map->buffer == ptr))
      {
         custom_map->used = 0;
         return;
      }
   }

   free(ptr);
}
#endif

static void log_mem_usage(void)
{
#ifdef _3DS
   extern u32 __heap_size, __linear_heap_size, __stacksize__;
   extern char __end__; // 3dsx.ld
   u32 app_memory = *((volatile u32 *)0x1FF80040);
   s64 mem_used = 0;
   if (__ctr_svchax)
      svcGetSystemInfo(&mem_used, 0, 1);

   SysPrintf("mem: %d/%d heap: %d linear: %d/%d stack: %d exe: %d\n",
      (int)mem_used, app_memory, __heap_size, __linear_heap_size - linearSpaceFree(),
      __linear_heap_size, __stacksize__, (int)&__end__ - 0x100000);
#endif
}

static void *pl_mmap(unsigned int size)
{
   return psxMap(0, size, 0, MAP_TAG_VRAM);
}

static void pl_munmap(void *ptr, unsigned int size)
{
   psxUnmap(ptr, size, MAP_TAG_VRAM);
}

struct rearmed_cbs pl_rearmed_cbs = {
   .pl_vout_open     = vout_open,
   .pl_vout_set_mode = vout_set_mode,
   .pl_vout_flip     = vout_flip,
   .pl_vout_close    = vout_close,
   .mmap             = pl_mmap,
   .munmap           = pl_munmap,
   .gpu_state_change = gpu_state_change,
   /* from psxcounters */
   .gpu_hcnt         = &hSyncCount,
   .gpu_frame_count  = &frame_counter,
};

void pl_frame_limit(void)
{
   /* called once per frame, make psxCpu->Execute() above return */
   psxRegs.stop++;
}

void pl_timing_prepare(int is_pal)
{
}

void plat_trigger_vibrate(int pad, int low, int high)
{
   if (!rumble_cb)
      return;

   if (in_enable_vibration)
   {
      rumble_cb(pad, RETRO_RUMBLE_STRONG, high << 8);
      rumble_cb(pad, RETRO_RUMBLE_WEAK, low ? 0xffff : 0x0);
   }
}

//Percentage distance of screen to adjust for Konami Gun
static float KonamiGunAdjustX = 0;
static float KonamiGunAdjustY = 0;

void pl_gun_byte2(int port, unsigned char byte)
{
   int irq_count = 4;
   float justifier_multiplier = 0;
   int justifier_width = psx_w;
   int justifier_height = psx_h;
   int justifier_offscreen = input_state_cb(port, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_IS_OFFSCREEN);
   int justifier_reload = input_state_cb(port, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_RELOAD);

   if (justifier_width == 256)
      justifier_multiplier = is_pal_mode ? .157086f : .158532f;
   else if (justifier_width == 320)
      justifier_multiplier = is_pal_mode ? .196358f : .198166f;
   else if (justifier_width == 384)
      justifier_multiplier = is_pal_mode ? .224409f : .226475f;
   else if (justifier_width == 512)
      justifier_multiplier = is_pal_mode ? .314173f : .317065f;
   else // (justifier_width == 640)
      justifier_multiplier = is_pal_mode ? .392717f : .396332f;

   int gunx = input_state_cb(port, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_SCREEN_X);
   int guny = input_state_cb(port, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_SCREEN_Y);

   //Default offset of +105 for X and -12 for Y is chosen to obtain alignment in Die Hard Trilogy, which has no calibration feature
   int gunx_scaled = ((gunx + 32767.0f) / 65534.0f + KonamiGunAdjustX) * justifier_width / justifier_multiplier + 105.0f;
   int guny_scaled = ((guny + 32767.0f) / 65534.0f + KonamiGunAdjustY) * justifier_height - 12.0f;

   if ((byte & 0x10) && !justifier_offscreen && !justifier_reload)
   {
      psxScheduleIrq10(irq_count, gunx_scaled, guny_scaled);
   }
}

/* sound calls */
static void snd_feed(void *buf, int bytes)
{
   if (audio_batch_cb != NULL)
      audio_batch_cb(buf, bytes / 4);
}

void out_register_libretro(struct out_driver *drv)
{
   drv->name   = "libretro";
   drv->init   = snd_init;
   drv->finish = snd_finish;
   drv->busy   = snd_busy;
   drv->feed   = snd_feed;
}

#define RETRO_DEVICE_PSE_STANDARD         RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD,   0)
#define RETRO_DEVICE_PSE_ANALOG           RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_ANALOG,   0)
#define RETRO_DEVICE_PSE_DUALSHOCK        RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_ANALOG,   1)
#define RETRO_DEVICE_PSE_NEGCON           RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_ANALOG,   2)
#define RETRO_DEVICE_PSE_GUNCON           RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_LIGHTGUN, 0)
#define RETRO_DEVICE_PSE_JUSTIFIER        RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_LIGHTGUN, 1)
#define RETRO_DEVICE_PSE_MOUSE            RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_MOUSE,    0)

static char *get_pse_pad_label[] = {
   "none", "mouse", "negcon", "konami gun", "standard", "analog", "guncon", "dualshock"
};

static const struct retro_controller_description pads[8] =
{
   { "standard",   RETRO_DEVICE_JOYPAD },
   { "analog",     RETRO_DEVICE_PSE_ANALOG },
   { "dualshock",  RETRO_DEVICE_PSE_DUALSHOCK },
   { "negcon",     RETRO_DEVICE_PSE_NEGCON },
   { "guncon",     RETRO_DEVICE_PSE_GUNCON },
   { "konami gun", RETRO_DEVICE_PSE_JUSTIFIER },
   { "mouse",      RETRO_DEVICE_PSE_MOUSE },
   { NULL, 0 },
};

static const struct retro_controller_info ports[9] =
{
   { pads, 7 },
   { pads, 7 },
   { pads, 7 },
   { pads, 7 },
   { pads, 7 },
   { pads, 7 },
   { pads, 7 },
   { pads, 7 },
   { NULL, 0 },
};

/* libretro */

static bool update_option_visibility(void)
{
   struct retro_variable var                       = {0};
   struct retro_core_option_display option_display = {0};
   bool updated                                    = false;
   unsigned i;

   /* If frontend supports core option categories
    * then show/hide core option entries are ignored
    * and no options should be hidden */
   if (libretro_supports_option_categories)
      return false;

   var.key = "pcsx_rearmed_show_input_settings";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      bool show_input_settings_prev =
            show_input_settings;

      show_input_settings = true;
      if (strcmp(var.value, "disabled") == 0)
         show_input_settings = false;

      if (show_input_settings !=
            show_input_settings_prev)
      {
         char input_option[][50] = {
            "pcsx_rearmed_analog_axis_modifier",
            "pcsx_rearmed_vibration",
            "pcsx_rearmed_multitap",
            "pcsx_rearmed_negcon_deadzone",
            "pcsx_rearmed_negcon_response",
            "pcsx_rearmed_input_sensitivity",
            "pcsx_rearmed_crosshair1",
            "pcsx_rearmed_crosshair2",
            "pcsx_rearmed_konamigunadjustx",
            "pcsx_rearmed_konamigunadjusty",
            "pcsx_rearmed_gunconadjustx",
            "pcsx_rearmed_gunconadjusty",
            "pcsx_rearmed_gunconadjustratiox",
            "pcsx_rearmed_gunconadjustratioy"
         };

         option_display.visible = show_input_settings;

         for (i = 0;
              i < (sizeof(input_option) /
                     sizeof(input_option[0]));
              i++)
         {
            option_display.key = input_option[i];
            environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY,
                  &option_display);
         }

         updated = true;
      }
   }
#ifdef GPU_PEOPS
   var.key = "pcsx_rearmed_show_gpu_peops_settings";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      bool show_advanced_gpu_peops_settings_prev =
            show_advanced_gpu_peops_settings;

      show_advanced_gpu_peops_settings = true;
      if (strcmp(var.value, "disabled") == 0)
         show_advanced_gpu_peops_settings = false;

      if (show_advanced_gpu_peops_settings !=
            show_advanced_gpu_peops_settings_prev)
      {
         unsigned i;
         struct retro_core_option_display option_display;
         char gpu_peops_option[][45] = {
            "pcsx_rearmed_gpu_peops_odd_even_bit",
            "pcsx_rearmed_gpu_peops_expand_screen_width",
            "pcsx_rearmed_gpu_peops_ignore_brightness",
            "pcsx_rearmed_gpu_peops_disable_coord_check",
            "pcsx_rearmed_gpu_peops_lazy_screen_update",
            "pcsx_rearmed_gpu_peops_repeated_triangles",
            "pcsx_rearmed_gpu_peops_quads_with_triangles",
            "pcsx_rearmed_gpu_peops_fake_busy_state"
         };

         option_display.visible = show_advanced_gpu_peops_settings;

         for (i = 0;
              i < (sizeof(gpu_peops_option) /
                     sizeof(gpu_peops_option[0]));
              i++)
         {
            option_display.key = gpu_peops_option[i];
            environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY,
                  &option_display);
         }

         updated = true;
      }
   }
#endif
#ifdef GPU_UNAI
   var.key = "pcsx_rearmed_show_gpu_unai_settings";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      bool show_advanced_gpu_unai_settings_prev =
            show_advanced_gpu_unai_settings;

      show_advanced_gpu_unai_settings = true;
      if (strcmp(var.value, "disabled") == 0)
         show_advanced_gpu_unai_settings = false;

      if (show_advanced_gpu_unai_settings !=
            show_advanced_gpu_unai_settings_prev)
      {
         unsigned i;
         struct retro_core_option_display option_display;
         char gpu_unai_option[][40] = {
            "pcsx_rearmed_gpu_unai_blending",
            "pcsx_rearmed_gpu_unai_skipline",
            "pcsx_rearmed_gpu_unai_lighting",
            "pcsx_rearmed_gpu_unai_fast_lighting",
            "pcsx_rearmed_gpu_unai_scale_hires",
         };

         option_display.visible = show_advanced_gpu_unai_settings;

         for (i = 0;
              i < (sizeof(gpu_unai_option) /
                     sizeof(gpu_unai_option[0]));
              i++)
         {
            option_display.key = gpu_unai_option[i];
            environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY,
                  &option_display);
         }

         updated = true;
      }
   }
#endif
   return updated;
}

void retro_set_environment(retro_environment_t cb)
{
   bool option_categories = false;
#ifdef USE_LIBRETRO_VFS
   struct retro_vfs_interface_info vfs_iface_info;
#endif

   environ_cb = cb;

   if (cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging))
      log_cb = logging.log;

   environ_cb(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, (void*)ports);

   /* Set core options
    * An annoyance: retro_set_environment() can be called
    * multiple times, and depending upon the current frontend
    * state various environment callbacks may be disabled.
    * This means the reported 'categories_supported' status
    * may change on subsequent iterations. We therefore have
    * to record whether 'categories_supported' is true on any
    * iteration, and latch the result */
   libretro_set_core_options(environ_cb, &option_categories);
   libretro_supports_option_categories |= option_categories;

   /* If frontend supports core option categories,
    * any show/hide core option entries are unused
    * and should be hidden */
   if (libretro_supports_option_categories)
   {
      struct retro_core_option_display option_display;
      option_display.visible = false;

      option_display.key = "pcsx_rearmed_show_input_settings";
      environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY,
            &option_display);

#ifdef GPU_PEOPS
      option_display.key = "pcsx_rearmed_show_gpu_peops_settings";
      environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY,
            &option_display);
#endif
#ifdef GPU_UNAI
      option_display.key = "pcsx_rearmed_show_gpu_unai_settings";
      environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY,
            &option_display);
#endif
   }
   /* If frontend does not support core option
    * categories, core options may be shown/hidden
    * at runtime. In this case, register 'update
    * display' callback, so frontend can update
    * core options menu without calling retro_run() */
   else
   {
      struct retro_core_options_update_display_callback update_display_cb;
      update_display_cb.callback = update_option_visibility;

      environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_UPDATE_DISPLAY_CALLBACK,
            &update_display_cb);
   }

#ifdef USE_LIBRETRO_VFS
   vfs_iface_info.required_interface_version = 1;
   vfs_iface_info.iface                      = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VFS_INTERFACE, &vfs_iface_info))
	   filestream_vfs_init(&vfs_iface_info);
#endif
}

void retro_set_video_refresh(retro_video_refresh_t cb) { video_cb = cb; }
void retro_set_audio_sample(retro_audio_sample_t cb) { (void)cb; }
void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) { audio_batch_cb = cb; }
void retro_set_input_poll(retro_input_poll_t cb) { input_poll_cb = cb; }
void retro_set_input_state(retro_input_state_t cb) { input_state_cb = cb; }

unsigned retro_api_version(void)
{
   return RETRO_API_VERSION;
}

static void update_multitap(void)
{
   struct retro_variable var = { 0 };

   multitap1 = 0;
   multitap2 = 0;

   var.value = NULL;
   var.key = "pcsx_rearmed_multitap";
   if (environ_cb && (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value))
   {
      if (strcmp(var.value, "port 1") == 0)
         multitap1 = 1;
      else if (strcmp(var.value, "port 2") == 0)
         multitap2 = 1;
      else if (strcmp(var.value, "ports 1 and 2") == 0)
      {
         multitap1 = 1;
         multitap2 = 1;
      }
   }
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
   if (port >= PORTS_NUMBER)
      return;

   switch (device)
   {
   case RETRO_DEVICE_JOYPAD:
   case RETRO_DEVICE_PSE_STANDARD:
      in_type[port] = PSE_PAD_TYPE_STANDARD;
      break;
   case RETRO_DEVICE_PSE_ANALOG:
      in_type[port] = PSE_PAD_TYPE_ANALOGJOY;
      break;
   case RETRO_DEVICE_PSE_DUALSHOCK:
      in_type[port] = PSE_PAD_TYPE_ANALOGPAD;
      break;
   case RETRO_DEVICE_PSE_MOUSE:
      in_type[port] = PSE_PAD_TYPE_MOUSE;
      break;
   case RETRO_DEVICE_PSE_NEGCON:
      in_type[port] = PSE_PAD_TYPE_NEGCON;
      break;
   case RETRO_DEVICE_PSE_GUNCON:
      in_type[port] = PSE_PAD_TYPE_GUNCON;
      break;
   case RETRO_DEVICE_PSE_JUSTIFIER:
      in_type[port] = PSE_PAD_TYPE_GUN;
      break;
   case RETRO_DEVICE_NONE:
   default:
      in_type[port] = PSE_PAD_TYPE_NONE;
      break;
   }

   SysPrintf("port: %u  device: %s\n", port + 1, get_pse_pad_label[in_type[port]]);
}

void retro_get_system_info(struct retro_system_info *info)
{
#ifndef GIT_VERSION
#define GIT_VERSION ""
#endif
   memset(info, 0, sizeof(*info));
   info->library_name     = "PCSX-ReARMed";
   info->library_version  = "r25" GIT_VERSION;
   info->valid_extensions = "bin|cue|img|mdf|pbp|toc|cbn|m3u|chd|iso|exe";
   info->need_fullpath    = true;
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
   unsigned geom_height          = vout_height;
   unsigned geom_width           = vout_width;

   memset(info, 0, sizeof(*info));
   info->timing.fps              = psxGetFps();
   info->timing.sample_rate      = 44100.0;
   info->geometry.base_width     = geom_width;
   info->geometry.base_height    = geom_height;
   info->geometry.max_width      = VOUT_MAX_WIDTH;
   info->geometry.max_height     = VOUT_MAX_HEIGHT;
   info->geometry.aspect_ratio   = 4.0 / 3.0;
}

/* savestates */
size_t retro_serialize_size(void)
{
   // it's currently 4380651-4397047 bytes,
   // but have some reserved for future
   return 0x440000;
}

struct save_fp
{
   char *buf;
   size_t pos;
   int is_write;
};

static void *save_open(const char *name, const char *mode)
{
   struct save_fp *fp;

   if (name == NULL || mode == NULL)
      return NULL;

   fp = malloc(sizeof(*fp));
   if (fp == NULL)
      return NULL;

   fp->buf = (char *)name;
   fp->pos = 0;
   fp->is_write = (mode[0] == 'w' || mode[1] == 'w');

   return fp;
}

static int save_read(void *file, void *buf, u32 len)
{
   struct save_fp *fp = file;
   if (fp == NULL || buf == NULL)
      return -1;

   memcpy(buf, fp->buf + fp->pos, len);
   fp->pos += len;
   return len;
}

static int save_write(void *file, const void *buf, u32 len)
{
   struct save_fp *fp = file;
   if (fp == NULL || buf == NULL)
      return -1;

   memcpy(fp->buf + fp->pos, buf, len);
   fp->pos += len;
   return len;
}

static long save_seek(void *file, long offs, int whence)
{
   struct save_fp *fp = file;
   if (fp == NULL)
      return -1;

   switch (whence)
   {
   case SEEK_CUR:
      fp->pos += offs;
      return fp->pos;
   case SEEK_SET:
      fp->pos = offs;
      return fp->pos;
   default:
      return -1;
   }
}

static void save_close(void *file)
{
   struct save_fp *fp = file;
   size_t r_size = retro_serialize_size();
   if (fp == NULL)
      return;

   if (fp->pos > r_size)
      LogErr("ERROR: save buffer overflow detected\n");
   else if (fp->is_write && fp->pos < r_size)
      // make sure we don't save trash in leftover space
      memset(fp->buf + fp->pos, 0, r_size - fp->pos);
   free(fp);
}

bool retro_serialize(void *data, size_t size)
{
   int ret;
   CdromFrontendId = disk_current_index;
   ret = SaveState(data);
   return ret == 0 ? true : false;
}

static bool disk_set_image_index(unsigned int index);

bool retro_unserialize(const void *data, size_t size)
{
   int ret;
   CdromFrontendId = -1;
   ret = LoadState(data);
   if (ret)
      return false;
   if (CdromFrontendId != -1 && CdromFrontendId != disk_current_index)
      disk_set_image_index(CdromFrontendId);
   return true;
}

/* cheats */
void retro_cheat_reset(void)
{
   ClearAllCheats();
}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
   int ret = -1;
   char *buf;

   // cheat funcs are destructive, need a copy...
   buf = strdup(code);
   if (buf == NULL)
      goto finish;

   //Prepare buffered cheat for PCSX's AddCheat fucntion.
   int cursor = 0;
   int nonhexdec = 0;
   while (buf[cursor])
   {
      if (!(ISHEXDEC))
      {
         if (++nonhexdec % 2)
         {
            buf[cursor] = ' ';
         }
         else
         {
            buf[cursor] = '\n';
         }
      }
      cursor++;
   }

   if (index < NumCheats)
      ret = EditCheat(index, "", buf);
   else
      ret = AddCheat("", buf);

finish:
   if (ret != 0)
      LogErr("Failed to set cheat %#u\n", index);
   else if (index < NumCheats)
      Cheats[index].Enabled = enabled;
   free(buf);
}

// just in case, maybe a win-rt port in the future?
#ifdef _WIN32
#define SLASH '\\'
#else
#define SLASH '/'
#endif

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

/* multidisk support */
static unsigned int disk_initial_index;
static char disk_initial_path[PATH_MAX];
static bool disk_ejected;
static unsigned int disk_count;
static struct disks_state
{
   char *fname;
   char *flabel;
   int internal_index; // for multidisk eboots
} disks[8];

static void get_disk_label(char *disk_label, const char *disk_path, size_t len)
{
   const char *base = NULL;

   if (!disk_path || (*disk_path == '\0'))
      return;

   base = strrchr(disk_path, SLASH);
   if (!base)
      base = disk_path;

   if (*base == SLASH)
      base++;

   strncpy(disk_label, base, len - 1);
   disk_label[len - 1] = '\0';

   char *ext = strrchr(disk_label, '.');
   if (ext)
      *ext = '\0';
}

static void disk_init(void)
{
   size_t i;

   disk_ejected       = false;
   disk_current_index = 0;
   disk_count         = 0;

   for (i = 0; i < sizeof(disks) / sizeof(disks[0]); i++)
   {
      if (disks[i].fname != NULL)
      {
         free(disks[i].fname);
         disks[i].fname = NULL;
      }
      if (disks[i].flabel != NULL)
      {
         free(disks[i].flabel);
         disks[i].flabel = NULL;
      }
      disks[i].internal_index = 0;
   }
}

static bool disk_set_eject_state(bool ejected)
{
   if (ejected != disk_ejected)
      SysPrintf("new eject_state: %d\n", ejected);

   // weird PCSX API...
   SetCdOpenCaseTime(ejected ? -1 : (time(NULL) + 2));
   LidInterrupt();

#ifdef HAVE_CDROM
   if (cdra_is_physical() && ejected != disk_ejected) {
      cdra_stop_thread();
      if (!ejected) {
         // likely the real cd was also changed - rescan
         cdra_close();
         cdra_open();
      }
   }
#endif
   disk_ejected = ejected;
   return true;
}

static bool disk_get_eject_state(void)
{
   /* can't be controlled by emulated software */
   return disk_ejected;
}

static unsigned int disk_get_image_index(void)
{
   return disk_current_index;
}

static bool disk_set_image_index(unsigned int index)
{
   if (index >= sizeof(disks) / sizeof(disks[0]))
      return false;

   CdromId[0] = '\0';
   CdromLabel[0] = '\0';

   if (disks[index].fname == NULL)
   {
      LogErr("missing disk #%u\n", index);
      cdra_shutdown();

      // RetroArch specifies "no disk" with index == count,
      // so don't fail here..
      disk_current_index = index;
      return true;
   }

   LogErr("switching to disk %u: \"%s\" #%d\n", index,
       disks[index].fname, disks[index].internal_index);

   cdrIsoMultidiskSelect = disks[index].internal_index;
   set_cd_image(disks[index].fname);
   if (ReloadCdromPlugin() < 0)
   {
      LogErr("failed to load cdr plugin\n");
      return false;
   }
   if (cdra_open() < 0)
   {
      LogErr("failed to open cdr plugin\n");
      return false;
   }

   if (!disk_ejected)
   {
      disk_set_eject_state(disk_ejected);
   }

   disk_current_index = index;
   return true;
}

static unsigned int disk_get_num_images(void)
{
   return disk_count;
}

static bool disk_replace_image_index(unsigned index,
    const struct retro_game_info *info)
{
   char *old_fname  = NULL;
   char *old_flabel = NULL;
   bool ret         = true;

   if (index >= sizeof(disks) / sizeof(disks[0]))
      return false;

   old_fname  = disks[index].fname;
   old_flabel = disks[index].flabel;

   disks[index].fname          = NULL;
   disks[index].flabel         = NULL;
   disks[index].internal_index = 0;

   if (info != NULL)
   {
      char disk_label[PATH_MAX];
      disk_label[0] = '\0';

      disks[index].fname = strdup(info->path);

      get_disk_label(disk_label, info->path, PATH_MAX);
      disks[index].flabel = strdup(disk_label);

      if (index == disk_current_index)
         ret = disk_set_image_index(index);
   }

   if (old_fname != NULL)
      free(old_fname);

   if (old_flabel != NULL)
      free(old_flabel);

   return ret;
}

static bool disk_add_image_index(void)
{
   if (disk_count >= 8)
      return false;

   disk_count++;
   return true;
}

static bool disk_set_initial_image(unsigned index, const char *path)
{
   if (index >= sizeof(disks) / sizeof(disks[0]))
      return false;

   if (!path || (*path == '\0'))
      return false;

   disk_initial_index = index;

   strncpy(disk_initial_path, path, sizeof(disk_initial_path) - 1);
   disk_initial_path[sizeof(disk_initial_path) - 1] = '\0';

   return true;
}

static bool disk_get_image_path(unsigned index, char *path, size_t len)
{
   const char *fname = NULL;

   if (len < 1)
      return false;

   if (index >= sizeof(disks) / sizeof(disks[0]))
      return false;

   fname = disks[index].fname;

   if (!fname || (*fname == '\0'))
      return false;

   strncpy(path, fname, len - 1);
   path[len - 1] = '\0';

   return true;
}

static bool disk_get_image_label(unsigned index, char *label, size_t len)
{
   const char *flabel = NULL;

   if (len < 1)
      return false;

   if (index >= sizeof(disks) / sizeof(disks[0]))
      return false;

   flabel = disks[index].flabel;

   if (!flabel || (*flabel == '\0'))
      return false;

   strncpy(label, flabel, len - 1);
   label[len - 1] = '\0';

   return true;
}

static struct retro_disk_control_callback disk_control = {
   .set_eject_state     = disk_set_eject_state,
   .get_eject_state     = disk_get_eject_state,
   .get_image_index     = disk_get_image_index,
   .set_image_index     = disk_set_image_index,
   .get_num_images      = disk_get_num_images,
   .replace_image_index = disk_replace_image_index,
   .add_image_index     = disk_add_image_index,
};

static struct retro_disk_control_ext_callback disk_control_ext = {
   .set_eject_state     = disk_set_eject_state,
   .get_eject_state     = disk_get_eject_state,
   .get_image_index     = disk_get_image_index,
   .set_image_index     = disk_set_image_index,
   .get_num_images      = disk_get_num_images,
   .replace_image_index = disk_replace_image_index,
   .add_image_index     = disk_add_image_index,
   .set_initial_image   = disk_set_initial_image,
   .get_image_path      = disk_get_image_path,
   .get_image_label     = disk_get_image_label,
};

static char base_dir[1024];

static bool read_m3u(const char *file)
{
   char line[1024];
   char name[PATH_MAX];
   FILE *fp = fopen(file, "r");
   if (!fp)
      return false;

   while (fgets(line, sizeof(line), fp) && disk_count < sizeof(disks) / sizeof(disks[0]))
   {
      if (line[0] == '#')
         continue;
      char *carrige_return = strchr(line, '\r');
      if (carrige_return)
         *carrige_return = '\0';
      char *newline = strchr(line, '\n');
      if (newline)
         *newline = '\0';

      if (line[0] != '\0')
      {
         char disk_label[PATH_MAX];
         disk_label[0] = '\0';

         snprintf(name, sizeof(name), "%s%c%s", base_dir, SLASH, line);
         disks[disk_count].fname = strdup(name);

         get_disk_label(disk_label, name, PATH_MAX);
         disks[disk_count].flabel = strdup(disk_label);

         disk_count++;
      }
   }

   fclose(fp);
   return (disk_count != 0);
}

static void extract_directory(char *buf, const char *path, size_t size)
{
   char *base;
   strncpy(buf, path, size - 1);
   buf[size - 1] = '\0';

   base = strrchr(buf, '/');
   if (!base)
      base = strrchr(buf, '\\');

   if (base)
      *base = '\0';
   else
   {
      buf[0] = '.';
      buf[1] = '\0';
   }
}

#if defined(__QNX__) || defined(_WIN32)
/* Blackberry QNX doesn't have strcasestr */

/*
 * Find the first occurrence of find in s, ignore case.
 */
char *
strcasestr(const char *s, const char *find)
{
   char c, sc;
   size_t len;

   if ((c = *find++) != 0)
   {
      c = tolower((unsigned char)c);
      len = strlen(find);
      do
      {
         do
         {
            if ((sc = *s++) == 0)
               return (NULL);
         } while ((char)tolower((unsigned char)sc) != c);
      } while (strncasecmp(s, find, len) != 0);
      s--;
   }
   return ((char *)s);
}
#endif

static void set_retro_memmap(void)
{
   uint64_t flags_ram = RETRO_MEMDESC_SYSTEM_RAM;
   struct retro_memory_map retromap = { 0 };
   struct retro_memory_descriptor descs[] = {
      { flags_ram, psxM, 0, 0x00000000, 0x5fe00000, 0, 0x200000 },
      { flags_ram, psxH, 0, 0x1f800000, 0x7ffffc00, 0, 0x000400 },
      // not ram but let the frontend patch it if it wants; should be last
      { flags_ram, psxR, 0, 0x1fc00000, 0x5ff80000, 0, 0x080000 },
   };

   retromap.descriptors = descs;
   retromap.num_descriptors = sizeof(descs) / sizeof(descs[0]);
   if (Config.HLE)
      retromap.num_descriptors--;

   environ_cb(RETRO_ENVIRONMENT_SET_MEMORY_MAPS, &retromap);
}

static void show_notification(const char *msg_str,
      unsigned duration_ms, unsigned priority)
{
   if (msg_interface_version >= 1)
   {
      struct retro_message_ext msg = {
         msg_str,
         duration_ms,
         3,
         RETRO_LOG_WARN,
         RETRO_MESSAGE_TARGET_ALL,
         RETRO_MESSAGE_TYPE_NOTIFICATION,
         -1
      };
      environ_cb(RETRO_ENVIRONMENT_SET_MESSAGE_EXT, &msg);
   }
   else
   {
      struct retro_message msg = {
         msg_str,
         180
      };
      environ_cb(RETRO_ENVIRONMENT_SET_MESSAGE, &msg);
   }
}

static void retro_audio_buff_status_cb(
   bool active, unsigned occupancy, bool underrun_likely)
{
   retro_audio_buff_active    = active;
   retro_audio_buff_occupancy = occupancy;
   retro_audio_buff_underrun  = underrun_likely;
}

static void retro_set_audio_buff_status_cb(void)
{
   if (frameskip_type == FRAMESKIP_NONE)
   {
      environ_cb(RETRO_ENVIRONMENT_SET_AUDIO_BUFFER_STATUS_CALLBACK, NULL);
      retro_audio_latency = 0;
   }
   else
   {
      bool calculate_audio_latency = true;

      if (frameskip_type == FRAMESKIP_FIXED_INTERVAL)
         environ_cb(RETRO_ENVIRONMENT_SET_AUDIO_BUFFER_STATUS_CALLBACK, NULL);
      else
      {
         struct retro_audio_buffer_status_callback buf_status_cb;
         buf_status_cb.callback = retro_audio_buff_status_cb;
         if (!environ_cb(RETRO_ENVIRONMENT_SET_AUDIO_BUFFER_STATUS_CALLBACK,
                         &buf_status_cb))
         {
            retro_audio_buff_active    = false;
            retro_audio_buff_occupancy = 0;
            retro_audio_buff_underrun  = false;
            retro_audio_latency        = 0;
            calculate_audio_latency    = false;
         }
      }

      if (calculate_audio_latency)
      {
         /* Frameskip is enabled - increase frontend
          * audio latency to minimise potential
          * buffer underruns */
         uint32_t frame_time_usec = 1000000.0 / (is_pal_mode ? 50.0 : 60.0);

         /* Set latency to 6x current frame time... */
         retro_audio_latency = (unsigned)(6 * frame_time_usec / 1000);

         /* ...then round up to nearest multiple of 32 */
         retro_audio_latency = (retro_audio_latency + 0x1F) & ~0x1F;
      }
   }

   update_audio_latency = true;
   frameskip_counter    = 0;
}

static void update_variables(bool in_flight);

static int get_bool_variable(const char *key)
{
   struct retro_variable var = { NULL, };

   var.key = key;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "enabled") == 0)
         return 1;
   }
   return 0;
}

bool retro_load_game(const struct retro_game_info *info)
{
   size_t i;
   unsigned int cd_index = 0;
   bool is_m3u, is_exe;
   int ret;

   struct retro_input_descriptor desc[] = {
#define JOYP(port)                                                                                                \
      { port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,   "D-Pad Left" },                              \
      { port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,     "D-Pad Up" },                                \
      { port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,   "D-Pad Down" },                              \
      { port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT,  "D-Pad Right" },                             \
      { port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,      "Cross" },                                   \
      { port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,      "Circle" },                                  \
      { port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,      "Triangle" },                                \
      { port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,      "Square" },                                  \
      { port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,      "L1" },                                      \
      { port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,     "L2" },                                      \
      { port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3,     "L3" },                                      \
      { port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,      "R1" },                                      \
      { port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,     "R2" },                                      \
      { port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3,     "R3" },                                      \
      { port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Select" },                                  \
      { port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,  "Start" },                                   \
      { port, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X,  "Left Analog X" },  \
      { port, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y,  "Left Analog Y" },  \
      { port, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X, "Right Analog X" }, \
      { port, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y, "Right Analog Y" }, \
      { port, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_TRIGGER, "Gun Trigger" },                        \
      { port, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_RELOAD,  "Gun Reload" },                         \
      { port, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_AUX_A,   "Gun Aux A" },                          \
      { port, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_AUX_B,   "Gun Aux B" },                          \
      { port, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_START,   "Gun Start" },
      
      JOYP(0)
      JOYP(1)
      JOYP(2)
      JOYP(3)
      JOYP(4)
      JOYP(5)
      JOYP(6)
      JOYP(7)

      { 0 },
   };

   environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc);

   enum retro_pixel_format fmt = get_bool_variable("pcsx_rearmed_rgb32_output")
      ? RETRO_PIXEL_FORMAT_XRGB8888 : RETRO_PIXEL_FORMAT_RGB565;
   if (environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
      current_fmt = fmt;
   else
      LogErr("SET_PIXEL_FORMAT failed\n");
   SysPrintf("Using PIXEL_FORMAT %d\n", current_fmt);
   set_bgr_to_fb_func(0);

   if (info == NULL || info->path == NULL)
   {
      LogErr("info->path required\n");
      return false;
   }
   is_m3u = (strcasestr(info->path, ".m3u") != NULL);
   is_exe = (strcasestr(info->path, ".exe") != NULL);

   update_variables(false);

   if (plugins_opened)
   {
      ClosePlugins();
      plugins_opened = 0;
   }

   disk_init();

   extract_directory(base_dir, info->path, sizeof(base_dir));

   if (is_m3u)
   {
      if (!read_m3u(info->path))
      {
         LogErr("failed to read m3u file\n");
         return false;
      }
   }
   else
   {
      char disk_label[PATH_MAX];
      disk_label[0] = '\0';

      disk_count = 1;
      disks[0].fname = strdup(info->path);

      get_disk_label(disk_label, info->path, PATH_MAX);
      disks[0].flabel = strdup(disk_label);
   }

   /* If this is an M3U file, attempt to set the
    * initial disk image */
   if (is_m3u && (disk_initial_index > 0) && (disk_initial_index < disk_count))
   {
      const char *fname = disks[disk_initial_index].fname;

      if (fname && (*fname != '\0'))
         if (strcmp(disk_initial_path, fname) == 0)
            cd_index = disk_initial_index;
   }

   set_cd_image(disks[cd_index].fname);
   disk_current_index = cd_index;

   /* have to reload after set_cd_image for correct cdr plugin */
   if (LoadPlugins() == -1)
   {
      LogErr("failed to load plugins\n");
      return false;
   }
   if (!strncmp(info->path, "cdrom:", 6))
   {
#if !defined(HAVE_CDROM) && !defined(USE_LIBRETRO_VFS)
      ReleasePlugins();
      LogErr("%s\n", "Physical CD-ROM support is not compiled in.");
      show_notification("Physical CD-ROM support is not compiled in.", 6000, 3);
      return false;
#endif
   }

   plugins_opened = 1;

   if (OpenPlugins() == -1)
   {
      LogErr("failed to open plugins\n");
      return false;
   }

   /* Handle multi-disk images (i.e. PBP)
    * > Cannot do this until after OpenPlugins() is
    *   called (since this sets the value of
    *   cdrIsoMultidiskCount) */
   if (!is_m3u && (cdrIsoMultidiskCount > 1))
   {
      disk_count = cdrIsoMultidiskCount < 8 ? cdrIsoMultidiskCount : 8;

      /* Small annoyance: We need to change the label
       * of disk 0, so have to clear existing entries */
      if (disks[0].fname != NULL)
         free(disks[0].fname);
      disks[0].fname = NULL;

      if (disks[0].flabel != NULL)
         free(disks[0].flabel);
      disks[0].flabel = NULL;

      for (i = 0; i < sizeof(disks) / sizeof(disks[0]) && i < cdrIsoMultidiskCount; i++)
      {
         char disk_name[PATH_MAX - 16] = { 0 };
         char disk_label[PATH_MAX] = { 0 };

         disks[i].fname = strdup(info->path);

         get_disk_label(disk_name, info->path, sizeof(disk_name));
         snprintf(disk_label, sizeof(disk_label), "%s #%u", disk_name, (unsigned)i + 1);
         disks[i].flabel = strdup(disk_label);

         disks[i].internal_index = i;
      }

      /* This is not an M3U file, so initial disk
       * image has not yet been set - attempt to
       * do so now */
      if ((disk_initial_index > 0) && (disk_initial_index < disk_count))
      {
         const char *fname = disks[disk_initial_index].fname;

         if (fname && (*fname != '\0'))
            if (strcmp(disk_initial_path, fname) == 0)
               cd_index = disk_initial_index;
      }

      if (cd_index > 0)
      {
         CdromId[0] = '\0';
         CdromLabel[0] = '\0';

         cdrIsoMultidiskSelect = disks[cd_index].internal_index;
         disk_current_index = cd_index;
         set_cd_image(disks[cd_index].fname);

         if (ReloadCdromPlugin() < 0)
         {
            LogErr("failed to reload cdr plugins\n");
            return false;
         }
         if (cdra_open() < 0)
         {
            LogErr("failed to open cdr plugin\n");
            return false;
         }
      }
   }

   /* set ports to use "standard controller" initially */
   for (i = 0; i < 8; ++i)
      in_type[i] = PSE_PAD_TYPE_STANDARD;

   if (!is_exe && CheckCdrom() == -1)
   {
      LogErr("unsupported/invalid CD image: %s\n", info->path);
      return false;
   }

   plugin_call_rearmed_cbs();
   SysReset();

   if (is_exe)
      ret = Load(info->path);
   else
      ret = LoadCdrom();
   if (ret != 0)
   {
      LogErr("could not load %s (%d)\n", is_exe ? "exe" : "CD", ret);
      return false;
   }
   emu_on_new_cd(0);

   set_retro_memmap();
   retro_set_audio_buff_status_cb();
   log_mem_usage();

   if (check_unsatisfied_libcrypt())
      show_notification("LibCrypt protected game with missing SBI detected", 3000, 3);
   if (Config.TurboCD)
      show_notification("TurboCD is ON", 700, 2);

   return true;
}

unsigned retro_get_region(void)
{
   return is_pal_mode ? RETRO_REGION_PAL : RETRO_REGION_NTSC;
}

void *retro_get_memory_data(unsigned id)
{
   if (id == RETRO_MEMORY_SAVE_RAM)
      return Mcd1Data;
   else if (id == RETRO_MEMORY_SYSTEM_RAM)
      return psxM;
   else
      return NULL;
}

size_t retro_get_memory_size(unsigned id)
{
   if (id == RETRO_MEMORY_SAVE_RAM)
      return MCD_SIZE;
   else if (id == RETRO_MEMORY_SYSTEM_RAM)
      return 0x200000;
   else
      return 0;
}

void retro_reset(void)
{
   //hack to prevent retroarch freezing when reseting in the menu but not while running with the hot key
   rebootemu = 1;
   //SysReset();
}

static const unsigned short retro_psx_map[] = {
   [RETRO_DEVICE_ID_JOYPAD_B]      = 1 << DKEY_CROSS,
   [RETRO_DEVICE_ID_JOYPAD_Y]      = 1 << DKEY_SQUARE,
   [RETRO_DEVICE_ID_JOYPAD_SELECT] = 1 << DKEY_SELECT,
   [RETRO_DEVICE_ID_JOYPAD_START]  = 1 << DKEY_START,
   [RETRO_DEVICE_ID_JOYPAD_UP]     = 1 << DKEY_UP,
   [RETRO_DEVICE_ID_JOYPAD_DOWN]   = 1 << DKEY_DOWN,
   [RETRO_DEVICE_ID_JOYPAD_LEFT]   = 1 << DKEY_LEFT,
   [RETRO_DEVICE_ID_JOYPAD_RIGHT]  = 1 << DKEY_RIGHT,
   [RETRO_DEVICE_ID_JOYPAD_A]      = 1 << DKEY_CIRCLE,
   [RETRO_DEVICE_ID_JOYPAD_X]      = 1 << DKEY_TRIANGLE,
   [RETRO_DEVICE_ID_JOYPAD_L]      = 1 << DKEY_L1,
   [RETRO_DEVICE_ID_JOYPAD_R]      = 1 << DKEY_R1,
   [RETRO_DEVICE_ID_JOYPAD_L2]     = 1 << DKEY_L2,
   [RETRO_DEVICE_ID_JOYPAD_R2]     = 1 << DKEY_R2,
   [RETRO_DEVICE_ID_JOYPAD_L3]     = 1 << DKEY_L3,
   [RETRO_DEVICE_ID_JOYPAD_R3]     = 1 << DKEY_R3,
};
#define RETRO_PSX_MAP_LEN (sizeof(retro_psx_map) / sizeof(retro_psx_map[0]))

//Percentage distance of screen to adjust for Guncon
static int GunconAdjustX = 0;
static int GunconAdjustY = 0;

//Used when out by a percentage with Guncon
static float GunconAdjustRatioX = 1;
static float GunconAdjustRatioY = 1;

static void update_variables(bool in_flight)
{
   struct retro_variable var;
#ifdef GPU_PEOPS
   // Always enable GPU_PEOPS_OLD_FRAME_SKIP flag
   // (this is set in standalone, with no option
   // to change it)
   int gpu_peops_fix = GPU_PEOPS_OLD_FRAME_SKIP;
#endif
   frameskip_type_t prev_frameskip_type;
   double old_fps = psxGetFps();

   var.value = NULL;
   var.key = "pcsx_rearmed_frameskip_type";

   prev_frameskip_type = frameskip_type;
   frameskip_type = FRAMESKIP_NONE;
   pl_rearmed_cbs.frameskip = 0;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "auto") == 0)
         frameskip_type = FRAMESKIP_AUTO;
      if (strcmp(var.value, "auto_threshold") == 0)
         frameskip_type = FRAMESKIP_AUTO_THRESHOLD;
      if (strcmp(var.value, "fixed_interval") == 0)
         frameskip_type = FRAMESKIP_FIXED_INTERVAL;
   }

   if (frameskip_type != 0)
      pl_rearmed_cbs.frameskip = -1;
   
   var.value = NULL;
   var.key = "pcsx_rearmed_frameskip_threshold";
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
     frameskip_threshold = strtol(var.value, NULL, 10);
   }

   var.value = NULL;
   var.key = "pcsx_rearmed_frameskip_interval";
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
     frameskip_interval = strtol(var.value, NULL, 10);
   }   

   var.value = NULL;
   var.key = "pcsx_rearmed_region";
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      Config.PsxAuto = 0;
      if (strcmp(var.value, "auto") == 0)
         Config.PsxAuto = 1;
      else if (strcmp(var.value, "NTSC") == 0)
         Config.PsxType = 0;
      else if (strcmp(var.value, "PAL") == 0)
         Config.PsxType = 1;
   }

   update_multitap();

   var.value = NULL;
   var.key = "pcsx_rearmed_negcon_deadzone";
   negcon_deadzone = 0;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      negcon_deadzone = (int)(atoi(var.value) * 0.01f * NEGCON_RANGE);
   }

   var.value = NULL;
   var.key = "pcsx_rearmed_negcon_response";
   negcon_linearity = 1;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "quadratic") == 0)
      {
         negcon_linearity = 2;
      }
      else if (strcmp(var.value, "cubic") == 0)
      {
         negcon_linearity = 3;
      }
   }

   var.value = NULL;
   var.key = "pcsx_rearmed_analog_axis_modifier";
   axis_bounds_modifier = true;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "square") == 0)
      {
         axis_bounds_modifier = true;
      }
      else
      {
         axis_bounds_modifier = false;
      }
   }

   var.value = NULL;
   var.key = "pcsx_rearmed_vibration";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "disabled") == 0)
         in_enable_vibration = 0;
      else if (strcmp(var.value, "enabled") == 0)
         in_enable_vibration = 1;
   }

   var.value = NULL;
   var.key = "pcsx_rearmed_analog_combo";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "l1+r1+select") == 0)
         in_dualshock_analog_combo = (1 << RETRO_DEVICE_ID_JOYPAD_L) |
           (1 << RETRO_DEVICE_ID_JOYPAD_R) | (1 << RETRO_DEVICE_ID_JOYPAD_SELECT);
      else if (strcmp(var.value, "l1+r1+start") == 0)
         in_dualshock_analog_combo = (1 << RETRO_DEVICE_ID_JOYPAD_L) |
           (1 << RETRO_DEVICE_ID_JOYPAD_R) | (1 << RETRO_DEVICE_ID_JOYPAD_START);
      else if (strcmp(var.value, "l1+r1+l3") == 0)
         in_dualshock_analog_combo = (1 << RETRO_DEVICE_ID_JOYPAD_L) |
           (1 << RETRO_DEVICE_ID_JOYPAD_R) | (1 << RETRO_DEVICE_ID_JOYPAD_L3);
      else if (strcmp(var.value, "l1+r1+r3") == 0)
         in_dualshock_analog_combo = (1 << RETRO_DEVICE_ID_JOYPAD_L) |
           (1 << RETRO_DEVICE_ID_JOYPAD_R) | (1 << RETRO_DEVICE_ID_JOYPAD_R3);
      else if (strcmp(var.value, "l3+r3") == 0)
         in_dualshock_analog_combo = (1 << RETRO_DEVICE_ID_JOYPAD_L3) |
           (1 << RETRO_DEVICE_ID_JOYPAD_R3);
      else
         in_dualshock_analog_combo = 0;
   }

   var.value = NULL;
   var.key = "pcsx_rearmed_dithering";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "force") == 0)
      {
         pl_rearmed_cbs.dithering = 2;
         pl_rearmed_cbs.gpu_peopsgl.bDrawDither = 1;
      }
      else if (strcmp(var.value, "disabled") == 0)
      {
         pl_rearmed_cbs.dithering = 0;
         pl_rearmed_cbs.gpu_peopsgl.bDrawDither = 0;
      }
      else
      {
         pl_rearmed_cbs.dithering = 1;
         pl_rearmed_cbs.gpu_peopsgl.bDrawDither = 1;
      }
   }

#ifdef GPU_NEON
   var.value = NULL;
   var.key = "pcsx_rearmed_neon_interlace_enable_v2";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "disabled") == 0)
         pl_rearmed_cbs.gpu_neon.allow_interlace = 0;
      else if (strcmp(var.value, "enabled") == 0)
         pl_rearmed_cbs.gpu_neon.allow_interlace = 1;
      else // auto
         pl_rearmed_cbs.gpu_neon.allow_interlace = 2;
   }

   var.value = NULL;
   var.key = "pcsx_rearmed_neon_enhancement_enable";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "disabled") == 0)
         pl_rearmed_cbs.gpu_neon.enhancement_enable = 0;
      else if (strcmp(var.value, "enabled") == 0)
         pl_rearmed_cbs.gpu_neon.enhancement_enable = 1;
   }

   var.value = NULL;
   var.key = "pcsx_rearmed_neon_enhancement_no_main";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "enabled") == 0)
         pl_rearmed_cbs.gpu_neon.enhancement_no_main = 1;
      else
         pl_rearmed_cbs.gpu_neon.enhancement_no_main = 0;
   }

   var.value = NULL;
   var.key = "pcsx_rearmed_neon_enhancement_tex_adj_v2";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "enabled") == 0)
         pl_rearmed_cbs.gpu_neon.enhancement_tex_adj = 1;
      else
         pl_rearmed_cbs.gpu_neon.enhancement_tex_adj = 0;
   }
#endif

   var.value = NULL;
   var.key = "pcsx_rearmed_display_fps_v2";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "extra") == 0)
         display_internal_fps = 2;
      else if (strcmp(var.value, "enabled") == 0)
         display_internal_fps = 1;
      else
         display_internal_fps = 0;
   }

   var.value = NULL;
   var.key = "pcsx_rearmed_cd_turbo";
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "enabled") == 0)
         Config.TurboCD = true;
      else
         Config.TurboCD = false;
   }

#if defined(HAVE_CDROM) || defined(USE_ASYNC_CDROM)
   var.value = NULL;
   var.key = "pcsx_rearmed_cd_readahead";
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      cdra_set_buf_count(strtol(var.value, NULL, 10));
   }
#endif

   //
   // CPU emulation related config
#ifndef DRC_DISABLE
   var.value = NULL;
   var.key = "pcsx_rearmed_drc";

   if (!environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var))
      var.value = "enabled";

   {
      R3000Acpu *prev_cpu = psxCpu;

#ifdef _3DS
      if (!__ctr_svchax)
         Config.Cpu = CPU_INTERPRETER;
      else
#endif
      if (strcmp(var.value, "disabled") == 0)
         Config.Cpu = CPU_INTERPRETER;
      else if (strcmp(var.value, "enabled") == 0)
         Config.Cpu = CPU_DYNAREC;

      psxCpu = (Config.Cpu == CPU_INTERPRETER) ? &psxInt : &psxRec;
      if (psxCpu != prev_cpu)
      {
         prev_cpu->Notify(R3000ACPU_NOTIFY_BEFORE_SAVE, NULL);
         prev_cpu->Shutdown();
         psxCpu->Init();
         psxCpu->Notify(R3000ACPU_NOTIFY_AFTER_LOAD, NULL);
      }
   }
#endif // !DRC_DISABLE

   var.value = NULL;
   var.key = "pcsx_rearmed_psxclock";
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      int psxclock = atoi(var.value);
      if (strcmp(var.value, "auto") == 0 || psxclock == 0)
         Config.cycle_multiplier = CYCLE_MULT_DEFAULT;
      else
         Config.cycle_multiplier = 10000 / psxclock;
   }

#if !defined(DRC_DISABLE) && !defined(LIGHTREC)
#ifdef NDRC_THREAD
   var.value = NULL;
   var.key = "pcsx_rearmed_drc_thread";
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      ndrc_g.hacks &= ~(NDHACK_THREAD_FORCE | NDHACK_THREAD_FORCE_ON);
      if (strcmp(var.value, "disabled") == 0)
         ndrc_g.hacks |= NDHACK_THREAD_FORCE;
      else if (strcmp(var.value, "enabled") == 0)
         ndrc_g.hacks |= NDHACK_THREAD_FORCE | NDHACK_THREAD_FORCE_ON;
      // psxCpu->ApplyConfig(); will start/stop the thread
   }
#endif

   var.value = NULL;
   var.key = "pcsx_rearmed_nosmccheck";
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "enabled") == 0)
         ndrc_g.hacks |= NDHACK_NO_SMC_CHECK;
      else
         ndrc_g.hacks &= ~NDHACK_NO_SMC_CHECK;
   }

   var.value = NULL;
   var.key = "pcsx_rearmed_gteregsunneeded";
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "enabled") == 0)
         ndrc_g.hacks |= NDHACK_GTE_UNNEEDED;
      else
         ndrc_g.hacks &= ~NDHACK_GTE_UNNEEDED;
   }

   var.value = NULL;
   var.key = "pcsx_rearmed_nogteflags";
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "enabled") == 0)
         ndrc_g.hacks |= NDHACK_GTE_NO_FLAGS;
      else
         ndrc_g.hacks &= ~NDHACK_GTE_NO_FLAGS;
   }

   var.value = NULL;
   var.key = "pcsx_rearmed_nocompathacks";
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "enabled") == 0)
         ndrc_g.hacks |= NDHACK_NO_COMPAT_HACKS;
      else
         ndrc_g.hacks &= ~NDHACK_NO_COMPAT_HACKS;
   }
#endif /* !DRC_DISABLE && !LIGHTREC */

   var.value = NULL;
   var.key = "pcsx_rearmed_nostalls";
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "enabled") == 0)
         Config.DisableStalls = 1;
      else
         Config.DisableStalls = 0;
   }

   var.value = NULL;
   var.key = "pcsx_rearmed_icache_emulation";
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "disabled") == 0)
         Config.icache_emulation = 0;
      else if (strcmp(var.value, "enabled") == 0)
         Config.icache_emulation = 1;
   }

   var.value = NULL;
   var.key = "pcsx_rearmed_exception_emulation";
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "enabled") == 0)
         Config.PreciseExceptions = 1;
      else
         Config.PreciseExceptions = 0;
   }

   psxCpu->ApplyConfig();

   // end of CPU emu config
   //

   var.value = NULL;
   var.key = "pcsx_rearmed_spu_reverb";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "disabled") == 0)
         spu_config.iUseReverb = false;
      else if (strcmp(var.value, "enabled") == 0)
         spu_config.iUseReverb = true;
   }

   var.value = NULL;
   var.key = "pcsx_rearmed_spu_interpolation";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "simple") == 0)
         spu_config.iUseInterpolation = 1;
      else if (strcmp(var.value, "gaussian") == 0)
         spu_config.iUseInterpolation = 2;
      else if (strcmp(var.value, "cubic") == 0)
         spu_config.iUseInterpolation = 3;
      else if (strcmp(var.value, "off") == 0)
         spu_config.iUseInterpolation = 0;
   }

#if P_HAVE_PTHREAD
   var.value = NULL;
   var.key = "pcsx_rearmed_spu_thread";
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "enabled") == 0)
         spu_config.iUseThread = 1;
      else
         spu_config.iUseThread = 0;
   }
#endif

   var.value = NULL;
   var.key = "pcsx_rearmed_noxadecoding";
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "disabled") == 0)
         Config.Xa = 1;
      else
         Config.Xa = 0;
   }

   var.value = NULL;
   var.key = "pcsx_rearmed_nocdaudio";
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "disabled") == 0)
         Config.Cdda = 1;
      else
         Config.Cdda = 0;
   }

   var.value = NULL;
   var.key = "pcsx_rearmed_gpu_slow_llists";
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "disabled") == 0)
         Config.GpuListWalking = 0;
      else if (strcmp(var.value, "enabled") == 0)
         Config.GpuListWalking = 1;
      else // auto
         Config.GpuListWalking = -1;
   }

   var.value = NULL;
   var.key = "pcsx_rearmed_fractional_framerate";
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "disabled") == 0)
         Config.FractionalFramerate = 0;
      else if (strcmp(var.value, "enabled") == 0)
         Config.FractionalFramerate = 1;
      else // auto
         Config.FractionalFramerate = -1;
   }

   var.value = NULL;
   var.key = "pcsx_rearmed_screen_centering";
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "game") == 0)
         pl_rearmed_cbs.screen_centering_type = 1;
      else if (strcmp(var.value, "borderless") == 0)
         pl_rearmed_cbs.screen_centering_type = 2;
      else if (strcmp(var.value, "manual") == 0)
         pl_rearmed_cbs.screen_centering_type = 3;
      else // auto
         pl_rearmed_cbs.screen_centering_type = 0;
   }

   var.value = NULL;
   var.key = "pcsx_rearmed_screen_centering_x";
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      pl_rearmed_cbs.screen_centering_x = atoi(var.value);
   }

   var.value = NULL;
   var.key = "pcsx_rearmed_screen_centering_y";
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      pl_rearmed_cbs.screen_centering_y = atoi(var.value);
   }

   var.value = NULL;
   var.key = "pcsx_rearmed_screen_centering_h_adj";
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      pl_rearmed_cbs.screen_centering_h_adj = atoi(var.value);
   }

   var.value = NULL;
   var.key = "pcsx_rearmed_show_overscan";
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "auto") == 0)
         pl_rearmed_cbs.show_overscan = 1;
      else if (strcmp(var.value, "hack") == 0)
         pl_rearmed_cbs.show_overscan = 2;
      else
         pl_rearmed_cbs.show_overscan = 0;
   }

#ifdef THREAD_RENDERING
   var.key = "pcsx_rearmed_gpu_thread_rendering";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "disabled") == 0)
         pl_rearmed_cbs.thread_rendering = THREAD_RENDERING_OFF;
      else if (strcmp(var.value, "sync") == 0)
         pl_rearmed_cbs.thread_rendering = THREAD_RENDERING_SYNC;
      else if (strcmp(var.value, "async") == 0)
         pl_rearmed_cbs.thread_rendering = THREAD_RENDERING_ASYNC;
   }
#endif

#ifdef GPU_PEOPS
   var.value = NULL;
   var.key = "pcsx_rearmed_gpu_peops_odd_even_bit";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "enabled") == 0)
         gpu_peops_fix |= GPU_PEOPS_ODD_EVEN_BIT;
   }

   var.value = NULL;
   var.key = "pcsx_rearmed_gpu_peops_expand_screen_width";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "enabled") == 0)
         gpu_peops_fix |= GPU_PEOPS_EXPAND_SCREEN_WIDTH;
   }

   var.value = NULL;
   var.key = "pcsx_rearmed_gpu_peops_ignore_brightness";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "enabled") == 0)
         gpu_peops_fix |= GPU_PEOPS_IGNORE_BRIGHTNESS;
   }

   var.value = NULL;
   var.key = "pcsx_rearmed_gpu_peops_disable_coord_check";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "enabled") == 0)
         gpu_peops_fix |= GPU_PEOPS_DISABLE_COORD_CHECK;
   }

   var.value = NULL;
   var.key = "pcsx_rearmed_gpu_peops_lazy_screen_update";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "enabled") == 0)
         gpu_peops_fix |= GPU_PEOPS_LAZY_SCREEN_UPDATE;
   }

   var.value = NULL;
   var.key = "pcsx_rearmed_gpu_peops_repeated_triangles";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "enabled") == 0)
         gpu_peops_fix |= GPU_PEOPS_REPEATED_TRIANGLES;
   }

   var.value = NULL;
   var.key = "pcsx_rearmed_gpu_peops_quads_with_triangles";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "enabled") == 0)
         gpu_peops_fix |= GPU_PEOPS_QUADS_WITH_TRIANGLES;
   }

   var.value = NULL;
   var.key = "pcsx_rearmed_gpu_peops_fake_busy_state";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "enabled") == 0)
         gpu_peops_fix |= GPU_PEOPS_FAKE_BUSY_STATE;
   }

   if (pl_rearmed_cbs.gpu_peops.dwActFixes != gpu_peops_fix)
      pl_rearmed_cbs.gpu_peops.dwActFixes = gpu_peops_fix;
#endif

#ifdef GPU_UNAI
   /* Note: This used to be an option, but it only works
    * (correctly) when running high resolution games
    * (480i, 512i) and has been obsoleted by
    * pcsx_rearmed_gpu_unai_scale_hires */
   pl_rearmed_cbs.gpu_unai.ilace_force = 0;

   var.key = "pcsx_rearmed_gpu_unai_old_renderer";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "enabled") == 0)
         pl_rearmed_cbs.gpu_unai.old_renderer = 1;
      else
         pl_rearmed_cbs.gpu_unai.old_renderer = 0;
   }

   var.key = "pcsx_rearmed_gpu_unai_skipline";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "disabled") == 0)
         pl_rearmed_cbs.gpu_unai.ilace_force = 0;
      else if (strcmp(var.value, "enabled") == 0)
         pl_rearmed_cbs.gpu_unai.ilace_force = 1;
   }

   var.key = "pcsx_rearmed_gpu_unai_lighting";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "disabled") == 0)
         pl_rearmed_cbs.gpu_unai.lighting = 0;
      else if (strcmp(var.value, "enabled") == 0)
         pl_rearmed_cbs.gpu_unai.lighting = 1;
   }

   var.key = "pcsx_rearmed_gpu_unai_fast_lighting";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "disabled") == 0)
         pl_rearmed_cbs.gpu_unai.fast_lighting = 0;
      else if (strcmp(var.value, "enabled") == 0)
         pl_rearmed_cbs.gpu_unai.fast_lighting = 1;
   }

   var.key = "pcsx_rearmed_gpu_unai_blending";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "disabled") == 0)
         pl_rearmed_cbs.gpu_unai.blending = 0;
      else if (strcmp(var.value, "enabled") == 0)
         pl_rearmed_cbs.gpu_unai.blending = 1;
   }

   var.key = "pcsx_rearmed_gpu_unai_scale_hires";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "disabled") == 0)
         pl_rearmed_cbs.gpu_unai.scale_hires = 0;
      else if (strcmp(var.value, "enabled") == 0)
         pl_rearmed_cbs.gpu_unai.scale_hires = 1;
   }
#endif // GPU_UNAI

   var.value = NULL;
   var.key = "pcsx_rearmed_crosshair1";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "disabled") == 0)
         in_enable_crosshair[0] = 0;
      else if (strcmp(var.value, "blue") == 0)
         in_enable_crosshair[0] = 0x1F;
      else if (strcmp(var.value, "green") == 0)
         in_enable_crosshair[0] = 0x7E0;
      else if (strcmp(var.value, "red") == 0)
         in_enable_crosshair[0] = 0xF800;
      else if (strcmp(var.value, "white") == 0)
         in_enable_crosshair[0] = 0xFFFF;
   }

   var.value = NULL;
   var.key = "pcsx_rearmed_crosshair2";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "disabled") == 0)
         in_enable_crosshair[1] = 0;
      else if (strcmp(var.value, "blue") == 0)
         in_enable_crosshair[1] = 0x1F;
      else if (strcmp(var.value, "green") == 0)
         in_enable_crosshair[1] = 0x7E0;
      else if (strcmp(var.value, "red") == 0)
         in_enable_crosshair[1] = 0xF800;
      else if (strcmp(var.value, "white") == 0)
         in_enable_crosshair[1] = 0xFFFF;
   }

   //This adjustment process gives the user the ability to manually align the mouse up better
   //with where the shots are in the emulator.

   var.value = NULL;
   var.key = "pcsx_rearmed_konamigunadjustx";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      KonamiGunAdjustX = atof(var.value) / 100.0f;
   }

   var.value = NULL;
   var.key = "pcsx_rearmed_konamigunadjusty";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      KonamiGunAdjustY = atof(var.value) / 100.0f;
   }

   var.value = NULL;
   var.key = "pcsx_rearmed_gunconadjustx";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      GunconAdjustX = atoi(var.value);
   }

   var.value = NULL;
   var.key = "pcsx_rearmed_gunconadjusty";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      GunconAdjustY = atoi(var.value);
   }

   var.value = NULL;
   var.key = "pcsx_rearmed_gunconadjustratiox";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      GunconAdjustRatioX = atof(var.value);
   }

   var.value = NULL;
   var.key = "pcsx_rearmed_gunconadjustratioy";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      GunconAdjustRatioY = atof(var.value);
   }

   var.value = NULL;
   var.key = "pcsx_rearmed_input_sensitivity";
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      mouse_sensitivity = atof(var.value);
   }

#ifdef _3DS
   var.value = NULL;
   var.key = "pcsx_rearmed_3ds_appcputime";
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      APT_SetAppCpuTimeLimit(strtol(var.value, NULL, 10));
   }
#endif

   if (found_bios)
   {
      var.value = NULL;
      var.key = "pcsx_rearmed_show_bios_bootlogo";
      if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
      {
         Config.SlowBoot = 0;
         if (strcmp(var.value, "enabled") == 0)
            Config.SlowBoot = 1;
      }
   }

   if (in_flight)
   {
      // inform core things about possible config changes
      plugin_call_rearmed_cbs();

      if (GPU_open != NULL && GPU_close != NULL)
      {
         GPU_close();
         GPU_open(&gpuDisp, "PCSX", NULL);
      }

      /* Reinitialise frameskipping, if required */
      if (((frameskip_type     != prev_frameskip_type)))
         retro_set_audio_buff_status_cb();

      /* dfinput_activate(); */
   }

   update_option_visibility();

   if (in_flight && old_fps != psxGetFps())
   {
      struct retro_system_av_info info;
      retro_get_system_av_info(&info);
      environ_cb(RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO, &info);
   }
}

// Taken from beetle-psx-libretro
static uint16_t get_analog_button(int16_t ret, retro_input_state_t input_state_cb, int player_index, int id)
{
   // NOTE: Analog buttons were added Nov 2017. Not all front-ends support this
   // feature (or pre-date it) so we need to handle this in a graceful way.

   // First, try and get an analog value using the new libretro API constant
   uint16_t button = input_state_cb(player_index,
       RETRO_DEVICE_ANALOG,
       RETRO_DEVICE_INDEX_ANALOG_BUTTON,
       id);
   button = MIN(button / 128, 255);

   if (button == 0)
   {
      // If we got exactly zero, we're either not pressing the button, or the front-end
      // is not reporting analog values. We need to do a second check using the classic
      // digital API method, to at least get some response - better than nothing.

      // NOTE: If we're really just not holding the button, we're still going to get zero.

      button = (ret & (1 << id)) ? 255 : 0;
   }

   return button;
}

static unsigned char axis_range_modifier(int axis_value, bool is_square)
{
   int modifier_axis_range;

   if (is_square)
      modifier_axis_range = roundf((axis_value >> 8) / 0.785f) + 128;
   else
      modifier_axis_range = (axis_value >> 8) + 128;

   if (modifier_axis_range < 0)
      modifier_axis_range = 0;
   else if (modifier_axis_range > 255)
      modifier_axis_range = 255;

   return modifier_axis_range;
}

static void update_input_guncon(int port, int ret)
{
   //ToDo:
   //Separate pointer and lightgun control types

   //Mouse range is -32767 -> 32767
   //1% is about 655
   //Use the left analog stick field to store the absolute coordinates

   int gunx = input_state_cb(port, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_SCREEN_X);
   int guny = input_state_cb(port, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_SCREEN_Y);

   //Have the Libretro API let /libpcsxcore/plugins.c know when the lightgun is pointed offscreen
   //Offscreen value is chosen to be well out of range of any possible scaling done via core options
   if (input_state_cb(port, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_IS_OFFSCREEN) || input_state_cb(port, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_RELOAD))
   {
      in_analog_left[port][0] = 65536;
      in_analog_left[port][1] = 65536;
   }
   else
   {
      in_analog_left[port][0] = ((gunx * GunconAdjustRatioX) + (GunconAdjustX * 655)) / 64 + 512;
      in_analog_left[port][1] = ((guny * GunconAdjustRatioY) + (GunconAdjustY * 655)) / 64 + 512;
   }
	
   //GUNCON has 3 controls, Trigger,A,B which equal Circle,Start,Cross

   // Trigger
   if (input_state_cb(port, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_TRIGGER) || input_state_cb(port, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_RELOAD))
      in_keystate[port] |= (1 << DKEY_CIRCLE);

   // A
   if (input_state_cb(port, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_AUX_A))
      in_keystate[port] |= (1 << DKEY_START);

   // B
   if (input_state_cb(port, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_AUX_B))
      in_keystate[port] |= (1 << DKEY_CROSS);
	   
}

static void update_input_justifier(int port, int ret)
{
   //ToDo:
   //Separate pointer and lightgun control types

   //RetroArch lightgun range is -32767 -> 32767 on both axes (positive Y is down)

   //JUSTIFIER has 3 controls, Trigger,Special,Start which equal Square,Cross,Start

   // Trigger
   if (input_state_cb(port, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_TRIGGER) || input_state_cb(port, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_RELOAD))
      in_keystate[port] |= (1 << DKEY_SQUARE);

   // Special
   if (input_state_cb(port, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_AUX_A))
      in_keystate[port] |= (1 << DKEY_CROSS);

   // Start
   if (input_state_cb(port, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_START))
      in_keystate[port] |= (1 << DKEY_START);
	   
}

static void update_input_negcon(int port, int ret)
{
   int lsx;
   int rsy;
   int negcon_i_rs;
   int negcon_ii_rs;
   float negcon_twist_amplitude;

   // Query digital inputs
   //
   // > Pad-Up
   if (ret & (1 << RETRO_DEVICE_ID_JOYPAD_UP))
      in_keystate[port] |= (1 << DKEY_UP);
   // > Pad-Right
   if (ret & (1 << RETRO_DEVICE_ID_JOYPAD_RIGHT))
      in_keystate[port] |= (1 << DKEY_RIGHT);
   // > Pad-Down
   if (ret & (1 << RETRO_DEVICE_ID_JOYPAD_DOWN))
      in_keystate[port] |= (1 << DKEY_DOWN);
   // > Pad-Left
   if (ret & (1 << RETRO_DEVICE_ID_JOYPAD_LEFT))
      in_keystate[port] |= (1 << DKEY_LEFT);
   // > Start
   if (ret & (1 << RETRO_DEVICE_ID_JOYPAD_START))
      in_keystate[port] |= (1 << DKEY_START);
   // > neGcon A
   if (ret & (1 << RETRO_DEVICE_ID_JOYPAD_A))
      in_keystate[port] |= (1 << DKEY_CIRCLE);
   // > neGcon B
   if (ret & (1 << RETRO_DEVICE_ID_JOYPAD_X))
      in_keystate[port] |= (1 << DKEY_TRIANGLE);
   // > neGcon R shoulder (digital)
   if (ret & (1 << RETRO_DEVICE_ID_JOYPAD_R))
      in_keystate[port] |= (1 << DKEY_R1);
   // Query analog inputs
   //
   // From studying 'libpcsxcore/plugins.c' and 'frontend/plugin.c':
   // >> pad->leftJoyX  == in_analog_left[port][0]  == NeGcon II
   // >> pad->leftJoyY  == in_analog_left[port][1]  == NeGcon L
   // >> pad->rightJoyX == in_analog_right[port][0] == NeGcon twist
   // >> pad->rightJoyY == in_analog_right[port][1] == NeGcon I
   // So we just have to map in_analog_left/right to more
   // appropriate inputs...
   //
   // > NeGcon twist
   // >> Get raw analog stick value and account for deadzone
   lsx = input_state_cb(port, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X);
   if (lsx > negcon_deadzone)
      lsx = lsx - negcon_deadzone;
   else if (lsx < -negcon_deadzone)
      lsx = lsx + negcon_deadzone;
   else
      lsx = 0;
   // >> Convert to an 'amplitude' [-1.0,1.0] and adjust response
   negcon_twist_amplitude = (float)lsx / (float)(NEGCON_RANGE - negcon_deadzone);
   if (negcon_linearity == 2)
   {
      if (negcon_twist_amplitude < 0.0)
         negcon_twist_amplitude = -(negcon_twist_amplitude * negcon_twist_amplitude);
      else
         negcon_twist_amplitude = negcon_twist_amplitude * negcon_twist_amplitude;
   }
   else if (negcon_linearity == 3)
      negcon_twist_amplitude = negcon_twist_amplitude * negcon_twist_amplitude * negcon_twist_amplitude;
   // >> Convert to final 'in_analog' integer value [0,255]
   in_analog_right[port][0] = MAX(MIN((int)(negcon_twist_amplitude * 128.0f) + 128, 255), 0);
   // > NeGcon I + II
   // >> Handle right analog stick vertical axis mapping...
   //    - Up (-Y) == accelerate == neGcon I
   //    - Down (+Y) == brake == neGcon II
   negcon_i_rs = 0;
   negcon_ii_rs = 0;
   rsy = input_state_cb(port, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y);
   if (rsy >= 0)
   {
      // Account for deadzone
      // (Note: have never encountered a gamepad with significant differences
      // in deadzone between left/right analog sticks, so use the regular 'twist'
      // deadzone here)
      if (rsy > negcon_deadzone)
         rsy = rsy - negcon_deadzone;
      else
         rsy = 0;
      // Convert to 'in_analog' integer value [0,255]
      negcon_ii_rs = MIN((int)(((float)rsy / (float)(NEGCON_RANGE - negcon_deadzone)) * 255.0f), 255);
   }
   else
   {
      if (rsy < -negcon_deadzone)
         rsy = -1 * (rsy + negcon_deadzone);
      else
         rsy = 0;
      negcon_i_rs = MIN((int)(((float)rsy / (float)(NEGCON_RANGE - negcon_deadzone)) * 255.0f), 255);
   }
   // >> NeGcon I
   in_analog_right[port][1] = MAX(
       MAX(
           get_analog_button(ret, input_state_cb, port, RETRO_DEVICE_ID_JOYPAD_R2),
           get_analog_button(ret, input_state_cb, port, RETRO_DEVICE_ID_JOYPAD_B)),
       negcon_i_rs);
   // >> NeGcon II
   in_analog_left[port][0] = MAX(
       MAX(
           get_analog_button(ret, input_state_cb, port, RETRO_DEVICE_ID_JOYPAD_L2),
           get_analog_button(ret, input_state_cb, port, RETRO_DEVICE_ID_JOYPAD_Y)),
       negcon_ii_rs);
   // > NeGcon L
   in_analog_left[port][1] = get_analog_button(ret, input_state_cb, port, RETRO_DEVICE_ID_JOYPAD_L);
}

static void update_input_mouse(int port, int ret)
{
   float raw_x = input_state_cb(port, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X);
   float raw_y = input_state_cb(port, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y);

   int x = (int)roundf(raw_x * mouse_sensitivity);
   int y = (int)roundf(raw_y * mouse_sensitivity);

   if (x > 127) x = 127;
   else if (x < -128) x = -128;

   if (y > 127) y = 127;
   else if (y < -128) y = -128;

   in_mouse[port][0] = x; /* -128..+128 left/right movement, 0 = no movement */
   in_mouse[port][1] = y; /* -128..+128 down/up movement, 0 = no movement    */

   /* left mouse button state */
   if (input_state_cb(port, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_LEFT))
      in_keystate[port] |= 1 << 11;

   /* right mouse button state */
   if (input_state_cb(port, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_RIGHT))
      in_keystate[port] |= 1 << 10;
}

static void update_input(void)
{
   int i;
   int j;

   // reset all keystate, query libretro for keystate
   for (i = 0; i < PORTS_NUMBER; i++)
   {
      int32_t ret = 0;
      int type = in_type[i];

      in_keystate[i] = 0;

      if (type == PSE_PAD_TYPE_NONE)
         continue;

      if (libretro_supports_bitmasks)
      {
         ret = input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_MASK);
         // undo int16 sign extension (why input_state_cb returns int16 in the first place?)
         ret &= (1 << (RETRO_DEVICE_ID_JOYPAD_R3 + 1)) - 1;
      }
      else
      {
         for (j = 0; j < (RETRO_DEVICE_ID_JOYPAD_R3 + 1); j++)
         {
            if (input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, j))
               ret |= (1 << j);
         }
      }

      switch (type)
      {
      case PSE_PAD_TYPE_GUNCON:
         update_input_guncon(i, ret);
         break;
      case PSE_PAD_TYPE_GUN:
         update_input_justifier(i, ret);
         break;
      case PSE_PAD_TYPE_NEGCON:
         update_input_negcon(i, ret);
         break;
      case PSE_PAD_TYPE_MOUSE:
         update_input_mouse(i, ret);
         break;      
      default:
         // dualshock ANALOG toggle?
         if (type == PSE_PAD_TYPE_ANALOGPAD && in_dualshock_analog_combo != 0
             && ret == in_dualshock_analog_combo)
         {
            if (!in_dualshock_toggling)
            {
               int state = padToggleAnalog(i);
               char msg[32];
               snprintf(msg, sizeof(msg), "ANALOG %s", state ? "ON" : "OFF");
               show_notification(msg, 800, 1);
               in_dualshock_toggling = true;
            }
            return;
         }
         in_dualshock_toggling = false;

         // Set digital inputs
         for (j = 0; j < RETRO_PSX_MAP_LEN; j++)
            if (ret & (1 << j))
               in_keystate[i] |= retro_psx_map[j];

         // Query analog inputs
         if (type == PSE_PAD_TYPE_ANALOGJOY || type == PSE_PAD_TYPE_ANALOGPAD)
         {
            in_analog_left[i][0]  = axis_range_modifier(input_state_cb(i, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X), axis_bounds_modifier);
            in_analog_left[i][1]  = axis_range_modifier(input_state_cb(i, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y), axis_bounds_modifier);
            in_analog_right[i][0] = axis_range_modifier(input_state_cb(i, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X), axis_bounds_modifier);
            in_analog_right[i][1] = axis_range_modifier(input_state_cb(i, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y), axis_bounds_modifier);
         }
      }
   }
}

static void print_internal_fps(void)
{
   if (display_internal_fps)
   {
      static u32 fps, frame_count_s;
      static time_t last_time;
      static u32 psx_vsync_count;
      u32 psx_vsync_rate = is_pal_mode ? 50 : 60;
      time_t now;

      psx_vsync_count++;
      frame_count_s++;
      now = time(NULL);
      if (now != last_time)
      {
         fps = frame_count_s;
         frame_count_s = 0;
         last_time = now;
      }

      if (psx_vsync_count >= psx_vsync_rate)
      {
         int pos = 0, cd_count;
         char str[64];

         if (display_internal_fps > 1) {
#if !defined(DRC_DISABLE) && !defined(LIGHTREC)
            if (ndrc_g.did_compile) {
               pos = snprintf(str, sizeof(str), "DRC: %d ", ndrc_g.did_compile);
               ndrc_g.did_compile = 0;
            }
#endif
            cd_count = cdra_get_buf_count();
            if (cd_count) {
               pos += snprintf(str + pos, sizeof(str) - pos, "CD: %2d/%d ",
                     cdra_get_buf_cached_approx(), cd_count);
            }
         }
         snprintf(str + pos, sizeof(str) - pos, "FPS: %2d/%2d",
               pl_rearmed_cbs.flip_cnt, fps);

         pl_rearmed_cbs.flip_cnt = 0;
         psx_vsync_count = 0;

         if (msg_interface_version >= 1)
         {
            struct retro_message_ext msg = {
               str,
               3000,
               1,
               RETRO_LOG_INFO,
               RETRO_MESSAGE_TARGET_OSD,
               RETRO_MESSAGE_TYPE_STATUS,
               -1
            };
            environ_cb(RETRO_ENVIRONMENT_SET_MESSAGE_EXT, &msg);
         }
         else
         {
            struct retro_message msg = {
               str,
               180
            };
            environ_cb(RETRO_ENVIRONMENT_SET_MESSAGE, &msg);
         }
      }
   }
}

void retro_run(void)
{
   //SysReset must be run while core is running,Not in menu (Locks up Retroarch)
   if (rebootemu != 0)
   {
      rebootemu = 0;
      SysReset();
      if (Config.HLE)
         LoadCdrom();
   }

   set_vout_fb();
   print_internal_fps();

   /* Check whether current frame should
    * be skipped */
   pl_rearmed_cbs.fskip_force = 0;
   pl_rearmed_cbs.fskip_dirty = 0;

   if (frameskip_type != FRAMESKIP_NONE)
   {
      bool skip_frame = false;

      switch (frameskip_type)
      {
         case FRAMESKIP_AUTO:
            skip_frame = retro_audio_buff_active && retro_audio_buff_underrun;
            break;
         case FRAMESKIP_AUTO_THRESHOLD:
            skip_frame = retro_audio_buff_active && (retro_audio_buff_occupancy < frameskip_threshold);
            break;
         case FRAMESKIP_FIXED_INTERVAL:
            skip_frame = true;
            break;
         default:
            break;
      }

      if (skip_frame && frameskip_counter < frameskip_interval)
         pl_rearmed_cbs.fskip_force = 1;
   }

   /* If frameskip/timing settings have changed,
    * update frontend audio latency
    * > Can do this before or after the frameskip
    *   check, but doing it after means we at least
    *   retain the current frame's audio output */
   if (update_audio_latency)
   {
      environ_cb(RETRO_ENVIRONMENT_SET_MINIMUM_AUDIO_LATENCY,
            &retro_audio_latency);
      update_audio_latency = false;
   }

   input_poll_cb();

   update_input();

   bool updated = false;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
      update_variables(true);

   psxRegs.stop = 0;
   psxCpu->Execute(&psxRegs);

   if (pl_rearmed_cbs.fskip_dirty == 1) {
      if (frameskip_counter < frameskip_interval)
         frameskip_counter++;
      else if (frameskip_counter >= frameskip_interval || !pl_rearmed_cbs.fskip_force)
         frameskip_counter = 0;
   }

   video_cb((vout_fb_dirty || !vout_can_dupe) ? vout_buf_ptr : NULL,
       vout_width, vout_height, vout_pitch_b);
   vout_fb_dirty = 0;

#ifdef HAVE_CDROM
   int inserted;
   if (cdra_check_eject(&inserted) > 0) {
      bool media_inserted = inserted != 0;
      if (!media_inserted != disk_ejected)
         disk_set_eject_state(!media_inserted);
   }
#endif
}

static bool try_use_bios(const char *path, bool preferred_only)
{
   long size;
   const char *name;
   FILE *fp = fopen(path, "rb");
   if (fp == NULL)
      return false;

   fseek(fp, 0, SEEK_END);
   size = ftell(fp);
   fclose(fp);

   name = strrchr(path, SLASH);
   if (name++ == NULL)
      name = path;

   if (preferred_only && size != 512 * 1024)
      return false;
   if (size != 512 * 1024 && size != 4 * 1024 * 1024)
      return false;
   if (strstr(name, "unirom"))
      return false;
   // jp bios have an addidional region check
   if (preferred_only && (strcasestr(name, "00.") || strcasestr(name, "j.bin")))
      return false;

   snprintf(Config.Bios, sizeof(Config.Bios), "%s", name);
   return true;
}

#ifndef VITA
#include <sys/types.h>
#include <dirent.h>

static bool find_any_bios(const char *dirpath, char *path, size_t path_size)
{
   static const char *substr_pref[] = { "scph", "ps" };
   static const char *substr_alt[] = { "scph", "ps", "openbios" };
   DIR *dir;
   struct dirent *ent;
   bool ret = false;
   size_t i;

   dir = opendir(dirpath);
   if (dir == NULL)
      return false;

   // try to find a "better" bios
   while ((ent = readdir(dir)))
   {
      for (i = 0; i < sizeof(substr_pref) / sizeof(substr_pref[0]); i++)
      {
         const char *substr = substr_pref[i];
         if ((strncasecmp(ent->d_name, substr, strlen(substr)) != 0))
            continue;
         snprintf(path, path_size, "%s%c%s", dirpath, SLASH, ent->d_name);
         ret = try_use_bios(path, true);
         if (ret)
            goto finish;
      }
   }

   // another pass to look for anything fitting, even ps2 bios
   rewinddir(dir);
   while ((ent = readdir(dir)))
   {
      for (i = 0; i < sizeof(substr_alt) / sizeof(substr_alt[0]); i++)
      {
         const char *substr = substr_alt[i];
         if ((strncasecmp(ent->d_name, substr, strlen(substr)) != 0))
            continue;
         snprintf(path, path_size, "%s%c%s", dirpath, SLASH, ent->d_name);
         ret = try_use_bios(path, false);
         if (ret)
            goto finish;
      }
   }


finish:
   closedir(dir);
   return ret;
}
#else
#define find_any_bios(...) false
#endif

static void check_system_specs(void)
{
   unsigned level = 6;
   environ_cb(RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL, &level);
}

static int init_memcards(void)
{
   int ret = 0;
   const char *dir;
   struct retro_variable var = { .key = "pcsx_rearmed_memcard2", .value = NULL };
   static const char CARD2_FILE[] = "pcsx-card2.mcd";

   // Memcard2 will be handled and is re-enabled if needed using core
   // operations.
   // Memcard1 is handled by libretro, doing this will set core to
   // skip file io operations for memcard1 like SaveMcd
   snprintf(Config.Mcd1, sizeof(Config.Mcd1), "none");
   snprintf(Config.Mcd2, sizeof(Config.Mcd2), "none");
   init_memcard(Mcd1Data);
   // Memcard 2 is managed by the emulator on the filesystem,
   // There is no need to initialize Mcd2Data like Mcd1Data.

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      SysPrintf("Memcard 2: %s\n", var.value);
      if (memcmp(var.value, "enabled", 7) == 0)
      {
         if (environ_cb(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &dir) && dir)
         {
            if (strlen(dir) + strlen(CARD2_FILE) + 2 > sizeof(Config.Mcd2))
            {
               LogErr("Path '%s' is too long. Cannot use memcard 2. Use a shorter path.\n", dir);
               ret = -1;
            }
            else
            {
               McdDisable[1] = 0;
               snprintf(Config.Mcd2, sizeof(Config.Mcd2), "%s/%s", dir, CARD2_FILE);
               SysPrintf("Use memcard 2: %s\n", Config.Mcd2);
            }
         }
         else
         {
            LogErr("Could not get save directory! Could not create memcard 2.");
            ret = -1;
         }
      }
   }
   return ret;
}

static void loadPSXBios(void)
{
   const char *dir;
   char path[PATH_MAX];
   unsigned useHLE = 0;

   const char *bios[] = {
      "PSXONPSP660", "psxonpsp660",
      "SCPH101", "scph101",
      "SCPH5501", "scph5501",
      "SCPH7001", "scph7001",
      "SCPH1001", "scph1001"
   };

   struct retro_variable var = {
      .key = "pcsx_rearmed_bios",
      .value = NULL
   };

   found_bios = 0;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (!strcmp(var.value, "HLE"))
         useHLE = 1;
   }

   if (!useHLE)
   {
      if (environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &dir) && dir)
      {
         unsigned i;
         snprintf(Config.BiosDir, sizeof(Config.BiosDir), "%s", dir);

         for (i = 0; i < sizeof(bios) / sizeof(bios[0]); i++)
         {
            snprintf(path, sizeof(path), "%s%c%s.bin", dir, SLASH, bios[i]);
            found_bios = try_use_bios(path, true);
            if (found_bios)
               break;
         }

         if (!found_bios)
            found_bios = find_any_bios(dir, path, sizeof(path));
      }
      if (found_bios)
      {
         SysPrintf("found BIOS file: %s\n", Config.Bios);
      }
   }

   if (!found_bios)
   {
      const char *msg_str;
      unsigned duration;
      if (useHLE)
      {
         msg_str = "BIOS set to \'hle\'";
         SysPrintf("Using HLE BIOS.\n");
         // shorter as the user probably intentionally wants to use HLE
         duration = 700;
      }
      else
      {
         msg_str = "No PlayStation BIOS file found - add for better compatibility";
         SysPrintf("No BIOS files found.\n");
         duration = 3000;
      }
      show_notification(msg_str, duration, 2);
   }
}

void retro_init(void)
{
   unsigned dci_version = 0;
   struct retro_rumble_interface rumble;
   int ret;

   log_mem_usage();

   msg_interface_version = 0;
   environ_cb(RETRO_ENVIRONMENT_GET_MESSAGE_INTERFACE_VERSION, &msg_interface_version);

#if defined(__MACH__) && !defined(TVOS)
   // magic sauce to make the dynarec work on iOS
   syscall(SYS_ptrace, 0 /*PTRACE_TRACEME*/, 0, 0, 0);
#endif

#if defined(_3DS)
   psxMapHook = pl_3ds_mmap;
   psxUnmapHook = pl_3ds_munmap;
#elif defined(HAVE_LIBNX)
   psxMapHook = pl_switch_mmap;
   psxUnmapHook = pl_switch_munmap;
#elif defined(VITA)
   if (init_vita_mmap() < 0)
      abort();
   psxMapHook = pl_vita_mmap;
   psxUnmapHook = pl_vita_munmap;
#endif
   ret = emu_core_preinit();
#ifdef _3DS
   /* emu_core_preinit sets the cpu to dynarec */
   if (!__ctr_svchax)
      Config.Cpu = CPU_INTERPRETER;
#endif
   ret |= init_memcards();

   ret |= emu_core_init();
   if (ret != 0)
   {
      LogErr("PCSX init failed.\n");
      exit(1);
   }

   // alloc enough for RETRO_PIXEL_FORMAT_XRGB8888
   size_t vout_buf_size = VOUT_MAX_WIDTH * VOUT_MAX_HEIGHT * 4;
#ifdef _3DS
   // Place psx vram in linear mem to take advantage of it's supersection mapping.
   // The emu allocs 2x (0x201000 to be exact) but doesn't really need that much,
   // so place vout_buf below to also act as an overdraw guard.
   vram_mem = linearMemAlign(1024*1024 + 4096 + vout_buf_size, 4096);
   if (vram_mem) {
      vout_buf = (char *)vram_mem + 1024*1024 + 4096;
      if (__ctr_svchax)
         SysPrintf("vram: %p PA %08x tlb %08x\n", vram_mem,
               svcConvertVAToPA(vram_mem, 0), ctr_get_tlbe(vram_mem));
   }
#elif defined(_POSIX_C_SOURCE) && (_POSIX_C_SOURCE >= 200112L) && P_HAVE_POSIX_MEMALIGN
   if (posix_memalign(&vout_buf, 16, vout_buf_size) != 0)
      vout_buf = NULL;
   else
      memset(vout_buf, 0, vout_buf_size);
#else
   vout_buf = calloc(vout_buf_size, 1);
#endif
   if (vout_buf == NULL)
   {
      LogErr("OOM for vout_buf.\n");
      // may be able to continue if we get retro_framebuffer access
   }

   vout_buf_ptr = vout_buf;

   loadPSXBios();

   environ_cb(RETRO_ENVIRONMENT_GET_CAN_DUPE, &vout_can_dupe);
   if (!vout_can_dupe)
      LogWarn("CAN_DUPE reports false\n");

   disk_initial_index = 0;
   disk_initial_path[0] = '\0';
   if (environ_cb(RETRO_ENVIRONMENT_GET_DISK_CONTROL_INTERFACE_VERSION, &dci_version) && (dci_version >= 1))
      environ_cb(RETRO_ENVIRONMENT_SET_DISK_CONTROL_EXT_INTERFACE, &disk_control_ext);
   else
      environ_cb(RETRO_ENVIRONMENT_SET_DISK_CONTROL_INTERFACE, &disk_control);

   rumble_cb = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_RUMBLE_INTERFACE, &rumble))
      rumble_cb = rumble.set_rumble_state;

   pl_rearmed_cbs.gpu_peops.dwActFixes = GPU_PEOPS_OLD_FRAME_SKIP;

   SaveFuncs.open = save_open;
   SaveFuncs.read = save_read;
   SaveFuncs.write = save_write;
   SaveFuncs.seek = save_seek;
   SaveFuncs.close = save_close;

   if (environ_cb(RETRO_ENVIRONMENT_GET_INPUT_BITMASKS, NULL))
      libretro_supports_bitmasks = true;

   check_system_specs();
}

void retro_deinit(void)
{
   if (plugins_opened)
   {
      ClosePlugins();
      plugins_opened = 0;
   }
   SysClose();
#ifdef _3DS
   linearFree(vram_mem);
   vram_mem = NULL;
#else
   free(vout_buf);
#endif
   vout_buf = NULL;

#ifdef VITA
   deinit_vita_mmap();
#endif
   libretro_supports_bitmasks = false;
   libretro_supports_option_categories = false;

   show_input_settings = true;
#ifdef GPU_PEOPS
   show_advanced_gpu_peops_settings = true;
#endif
#ifdef GPU_UNAI
   show_advanced_gpu_unai_settings = true;
#endif

   /* Have to reset disks struct, otherwise
    * fnames/flabels will leak memory */
   disk_init();
   frameskip_type             = FRAMESKIP_NONE;
   frameskip_threshold        = 0;
   frameskip_interval         = 0;
   frameskip_counter          = 0;
   retro_audio_buff_active    = false;
   retro_audio_buff_occupancy = 0;
   retro_audio_buff_underrun  = false;
   retro_audio_latency        = 0;
   update_audio_latency       = false;
}

void SysPrintf(const char *fmt, ...)
{
   va_list list;
   char msg[512];

   va_start(list, fmt);
   vsprintf(msg, fmt, list);
   va_end(list);

   if (log_cb)
      log_cb(RETRO_LOG_INFO, "%s", msg);
}

/* Prints debug-level logs */
void SysDLog(const char *fmt, ...)
{
   va_list list;
   char msg[512];

   va_start(list, fmt);
   vsprintf(msg, fmt, list);
   va_end(list);

   if (log_cb)
      log_cb(RETRO_LOG_DEBUG, "%s", msg);
}

// vim:sw=3:ts=3:expandtab
