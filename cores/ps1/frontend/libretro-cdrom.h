#include "cdrom/cdrom.h"

int cdrom_set_read_speed_x(libretro_vfs_implementation_file *stream, unsigned speed);
int cdrom_read_sector(libretro_vfs_implementation_file *stream,
      unsigned int lba, void *b);
