#include "libretro-cdrom.h"
#include "../deps/libretro-common/cdrom/cdrom.c"
#if defined(__linux__) && !defined(ANDROID)
//#include <linux/cdrom.h>
#endif

#include "../libpcsxcore/psxcommon.h"
#include "../libpcsxcore/cdrom.h"

//#include "vfs/vfs_implementation.h"
#include "vfs/vfs_implementation_cdrom.h"

static int cdrom_send_command_dummy(const libretro_vfs_implementation_file *stream,
      CDROM_CMD_Direction dir, void *buf, size_t len, unsigned char *cmd, size_t cmd_len,
      unsigned char *sense, size_t sense_len)
{
   return 1;
}

static int cdrom_send_command_once(const libretro_vfs_implementation_file *stream,
      CDROM_CMD_Direction dir, void *buf, size_t len, unsigned char *cmd, size_t cmd_len)
{
   unsigned char sense[CDROM_MAX_SENSE_BYTES] = {0};
   int ret =
#if defined(__linux__) && !defined(ANDROID)
      cdrom_send_command_linux
#elif defined(_WIN32) && !defined(_XBOX)
      cdrom_send_command_win32
#else
      cdrom_send_command_dummy
#endif
         (stream, dir, buf, len, cmd, cmd_len, sense, sizeof(sense));
#ifdef CDROM_DEBUG
   if (ret && sense[2])
      cdrom_print_sense_data(sense, sizeof(sense));
#endif
   (void)cdrom_send_command_dummy;
   return ret;
}

// "extensions" to libretro-common
int cdrom_set_read_speed_x(libretro_vfs_implementation_file *stream, unsigned speed)
{
   // SET CD-ROM SPEED, DA is newer?
   unsigned char cmd1[] = {0xDA, 0, speed - 1, 0, 0, 0, 0, 0, 0, 0, 0, 0};
   unsigned char cmd2[] = {0xBB, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
   int ret;
   ret = cdrom_send_command_once(stream, DIRECTION_NONE, NULL, 0, cmd1, sizeof(cmd1));
   if (ret) {
#if defined(__linux__) && !defined(ANDROID)
      // doesn't work, too late?
      //ret = ioctl(fileno(stream->fp), CDROM_SELECT_SPEED, &speed);
#endif
   }
   if (ret) {
      speed = speed * 2352 * 75 / 1024;
      cmd2[2] = speed >> 8;
      cmd2[3] = speed;
      ret = cdrom_send_command_once(stream, DIRECTION_NONE, NULL, 0, cmd2, sizeof(cmd2));
   }
   return ret;
}

int rcdrom_readSector(void *stream, unsigned int lba, void *b)
{
   unsigned char cmd[] = {0xBE, 0, 0, 0, 0, 0, 0, 0, 1, 0xF8, 0, 0};
   cmd[2] = lba >> 24;
   cmd[3] = lba >> 16;
   cmd[4] = lba >> 8;
   cmd[5] = lba;
   return cdrom_send_command_once(stream, DIRECTION_IN, b, 2352, cmd, sizeof(cmd));
}

void *rcdrom_open(const char *name, u32 *total_lba, u32 *have_subchannel)
{
   void *g_cd_handle = retro_vfs_file_open_impl(name, RETRO_VFS_FILE_ACCESS_READ,
        RETRO_VFS_FILE_ACCESS_HINT_NONE);
   if (!g_cd_handle) {
      SysPrintf("retro_vfs_file_open failed for '%s'\n", name);
      return NULL;
   }
   else {
      int ret = cdrom_set_read_speed_x(g_cd_handle, 4);
      if (ret) SysPrintf("CD speed set failed\n");
      const cdrom_toc_t *toc = retro_vfs_file_get_cdrom_toc();
      const cdrom_track_t *last = &toc->track[toc->num_tracks - 1];
      unsigned int lba = MSF2SECT(last->min, last->sec, last->frame);
      *total_lba = lba + last->track_size;
      *have_subchannel = 0;
      //cdrom_get_current_config_random_readable(acdrom.h);
      //cdrom_get_current_config_multiread(acdrom.h);
      //cdrom_get_current_config_cdread(acdrom.h);
      //cdrom_get_current_config_profiles(acdrom.h);
      return g_cd_handle;
   }
}

void rcdrom_close(void *stream)
{
   retro_vfs_file_close_impl(stream);
}

int rcdrom_getTN(void *stream, u8 *tn)
{
   const cdrom_toc_t *toc = retro_vfs_file_get_cdrom_toc();
   if (toc) {
     tn[0] = 1;
     tn[1] = toc->num_tracks;
     return 0;
   }
   return -1;
}

int rcdrom_getTD(void *stream, u32 total_lba, u8 track, u8 *rt)
{
   const cdrom_toc_t *toc = retro_vfs_file_get_cdrom_toc();
   rt[0] = 0, rt[1] = 2, rt[2] = 0;
   if (track == 0) {
      lba2msf(total_lba + 150, &rt[0], &rt[1], &rt[2]);
   }
   else if (track <= toc->num_tracks) {
      int i = track - 1;
      rt[0] = toc->track[i].min;
      rt[1] = toc->track[i].sec;
      rt[2] = toc->track[i].frame;
   }
   return 0;
}

int rcdrom_getStatus(void *stream, struct CdrStat *stat)
{
   const cdrom_toc_t *toc = retro_vfs_file_get_cdrom_toc();
   stat->Type = toc->track[0].audio ? 2 : 1;
   return 0;
}

int rcdrom_isMediaInserted(void *stream)
{
   return cdrom_is_media_inserted(stream);
}

int rcdrom_readSub(void *stream, unsigned int lba, void *b)
{
   return -1;
}

// vim:sw=3:ts=3:expandtab
