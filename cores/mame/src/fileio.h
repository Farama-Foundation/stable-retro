/***************************************************************************

	fileio.h

	Core file I/O interface functions and definitions.

***************************************************************************/

#ifndef FILEIO_H
#define FILEIO_H

#include <stdarg.h>
#include "mame2003.h"
#include "hash.h"

#ifdef __cplusplus
extern "C" {
#endif


/* file types */
enum
{
	FILETYPE_RAW = 0,
	FILETYPE_ROM,
	FILETYPE_IMAGE,
	FILETYPE_IMAGE_DIFF,
	FILETYPE_SAMPLE,
	FILETYPE_SAMPLE_FLAC,
	FILETYPE_ARTWORK,
	FILETYPE_NVRAM,
	FILETYPE_HIGHSCORE,
	FILETYPE_HIGHSCORE_DB,
	FILETYPE_CONFIG,
	FILETYPE_MEMCARD,
	FILETYPE_HISTORY,
	FILETYPE_CHEAT,
	FILETYPE_LANGUAGE,
	FILETYPE_CTRLR,
	FILETYPE_XML_DAT,
	FILETYPE_end /* dummy last entry */
};

/* These values are returned by osd_get_path_info */
enum
{
	PATH_NOT_FOUND,
	PATH_IS_FILE,
	PATH_IS_DIRECTORY
};

/* gamename holds the driver name, filename is only used for ROMs and    */
/* samples. If 'write' is not 0, the file is opened for write. Otherwise */
/* it is opened for read. */

typedef struct _mame_file mame_file;

/* Return the number of paths for a given type */
int osd_get_path_count(int pathtype);

struct bin2cFILE {
  const unsigned int length;
  const unsigned char data[]; 
};

/******************************************************************************

 osd_get_path
 Sets char* path to point at a valid path of the type incidated by int pathtype,
 although the path itself does not necessarily exist at this point in the process.

 *****************************************************************************/
 void osd_get_path(int pathtype, char* path);


/* Get information on the existence of a file */
int osd_get_path_info(int pathtype, int pathindex, const char *filename);

/* Attempt to open a file with the given name and mode using the specified path type */
FILE* osd_fopen(int pathtype, int pathindex, const char *filename, const char *mode);

int osd_create_directory(const char *dir);



int mame_faccess(const char *filename, int filetype);
mame_file *mame_fopen(const char *gamename, const char *filename, int filetype, int openforwrite);
mame_file *mame_fopen_rom(const char *gamename, const char *filename, const char* exphash);
UINT32 mame_fread(mame_file *file, void *buffer, UINT32 length);
UINT32 mame_fwrite(mame_file *file, const void *buffer, UINT32 length);
UINT32 mame_fread_swap(mame_file *file, void *buffer, UINT32 length);
UINT32 mame_fwrite_swap(mame_file *file, const void *buffer, UINT32 length);
#ifdef MSB_FIRST
#define mame_fread_msbfirst mame_fread
#define mame_fwrite_msbfirst mame_fwrite
#define mame_fread_lsbfirst mame_fread_swap
#define mame_fwrite_lsbfirst mame_fwrite_swap
#else

#define mame_fread_msbfirst mame_fread_swap
#define mame_fwrite_msbfirst mame_fwrite_swap
#define mame_fread_lsbfirst mame_fread
#define mame_fwrite_lsbfirst mame_fwrite
#endif
int mame_fseek(mame_file *file, INT64 offset, int whence);
void mame_fclose(mame_file *file);
int mame_fchecksum(const char *gamename, const char *filename, unsigned int *length, char* hash);
UINT64 mame_fsize(mame_file *file);
const char *mame_fhash(mame_file *file);
int mame_fgetc(mame_file *file);
int mame_ungetc(int c, mame_file *file);
char *mame_fgets(char *s, int n, mame_file *file);
int mame_feof(mame_file *file);
UINT64 mame_ftell(mame_file *file);

int mame_fputs(mame_file *f, const char *s);
int mame_vfprintf(mame_file *f, const char *fmt, va_list va);


/***************************************************************************
	get_extension_for_filetype
***************************************************************************/

const char *get_extension_for_filetype(int filetype);


/***************************************************************************
	spawn_bootstrap_nvram
  creates a new nvram file for the current romset (as specified in
  options.romset_filename_noext) using bootstrap_nvram as the source.
***************************************************************************/
mame_file *spawn_bootstrap_nvram(unsigned char const *bootstrap_nvram, unsigned nvram_length);

#ifdef __GNUC__
int CLIB_DECL mame_fprintf(mame_file *f, const char *fmt, ...)
      __attribute__ ((format (printf, 2, 3)));
#else
int CLIB_DECL mame_fprintf(mame_file *f, const char *fmt, ...);
#endif /* __GNUC__ */

#ifdef __cplusplus
}
#endif

#endif
