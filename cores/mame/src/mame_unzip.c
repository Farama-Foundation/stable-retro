#include "unzip.h"
#include "driver.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <compat/zlib.h>

/* public globals */
int	gUnzipQuiet = 0;		/* flag controls error messages */


#define ERROR_CORRUPT "The zipfile seems to be corrupt, please check it"
#define ERROR_FILESYSTEM "Your filesystem seems to be corrupt, please check it"
#define ERROR_UNSUPPORTED "The format of this zipfile is not supported, please recompress it"

#define INFLATE_INPUT_BUFFER_MAX 16384
#ifndef MIN
#define MIN(x,y) ((x)<(y)?(x):(y))
#endif

/* Print a error message */
void errormsg(const char* extmsg, const char* usermsg, const char* zipname) {
	/* Output to the user with no internal detail */
	if (!gUnzipQuiet)
		log_cb(RETRO_LOG_ERROR, LOGPRE "Error in zipfile %s\n%s\n", zipname, usermsg);
	/* Output to log file with all informations */
	log_cb(RETRO_LOG_ERROR, LOGPRE "Error in zipfile %s: %s\n", zipname, extmsg);
}

/* -------------------------------------------------------------------------
   Unzip support
 ------------------------------------------------------------------------- */

/* Use these to avoid structure padding and byte-ordering problems */
static UINT16 read_word (char *buf) {
   unsigned char *ubuf = (unsigned char *) buf;

   return ((UINT16)ubuf[1] << 8) | (UINT16)ubuf[0];
}

/* Use these to avoid structure padding and byte-ordering problems */
static UINT32 read_dword (char *buf) {
   unsigned char *ubuf = (unsigned char *) buf;

   return ((UINT32)ubuf[3] << 24) | ((UINT32)ubuf[2] << 16) | ((UINT32)ubuf[1] << 8) | (UINT32)ubuf[0];
}

/* Locate end-of-central-dir sig in buffer and return offset
   out:
	*offset offset of cent dir start in buffer
   return:
	==0 not found
	!=0 found, *offset valid
*/
static int ecd_find_sig (char *buffer, int buflen, int *offset)
{
	static char ecdsig[] = { 'P', 'K', 0x05, 0x06 };
	int i;
	for (i=buflen-22; i>=0; i--) {
		if (memcmp(buffer+i, ecdsig, 4) == 0) {
			*offset = i;
			return 1;
		}
	}
	return 0;
}

/* Read ecd data in zip structure
   in:
     zip->fp, zip->length zip file
   out:
     zip->ecd, zip->ecd_length ecd data
*/
static int ecd_read(ZIP* zip) {
	char* buf;
	int buf_length = 1024; /* initial buffer length */

	while (1) {
		int offset;

		if (buf_length > zip->length)
			buf_length = zip->length;

		if (fseek(zip->fp, zip->length - buf_length, SEEK_SET) != 0) {
			return -1;
		}

		/* allocate buffer */
		buf = (char*)malloc( buf_length );
		if (!buf) {
			return -1;
		}

		if (fread( buf, 1, buf_length, zip->fp ) != buf_length) {
			free(buf);
			return -1;
		}

		if (ecd_find_sig(buf, buf_length, &offset)) {
			zip->ecd_length = buf_length - offset;

			zip->ecd = (char*)malloc( zip->ecd_length );
			if (!zip->ecd) {
				free(buf);
				return -1;
			}

			memcpy(zip->ecd, buf + offset, zip->ecd_length);

			free(buf);
			return 0;
		}

		free(buf);

		if (buf_length < zip->length) {
			/* double buffer */
			buf_length = 2*buf_length;

			log_cb(RETRO_LOG_ERROR, LOGPRE "Retry reading of zip ecd for %d bytes\n",buf_length);

		} else {
			return -1;
		}
	}
}

/* offsets in end of central directory structure */
#define ZIPESIG		0x00
#define ZIPEDSK		0x04
#define ZIPECEN		0x06
#define ZIPENUM		0x08
#define ZIPECENN	0x0a
#define ZIPECSZ		0x0c
#define ZIPEOFST	0x10
#define ZIPECOML	0x14
#define ZIPECOM		0x16

/* offsets in central directory entry structure */
#define ZIPCENSIG	0x0
#define ZIPCVER		0x4
#define ZIPCOS		0x5
#define	ZIPCVXT		0x6
#define	ZIPCEXOS	0x7
#define ZIPCFLG		0x8
#define ZIPCMTHD	0xa
#define ZIPCTIM		0xc
#define ZIPCDAT		0xe
#define ZIPCCRC		0x10
#define ZIPCSIZ		0x14
#define ZIPCUNC		0x18
#define ZIPCFNL		0x1c
#define ZIPCXTL		0x1e
#define ZIPCCML		0x20
#define ZIPDSK		0x22
#define ZIPINT		0x24
#define ZIPEXT		0x26
#define ZIPOFST		0x2a
#define ZIPCFN		0x2e

/* offsets in local file header structure */
#define ZIPLOCSIG	0x00
#define ZIPVER		0x04
#define ZIPGENFLG	0x06
#define ZIPMTHD		0x08
#define ZIPTIME		0x0a
#define ZIPDATE		0x0c
#define ZIPCRC		0x0e
#define ZIPSIZE		0x12
#define ZIPUNCMP	0x16
#define ZIPFNLN		0x1a
#define ZIPXTRALN	0x1c
#define ZIPNAME		0x1e

/* Opens a zip stream for reading
   return:
     !=0 success, zip stream
     ==0 error
*/
ZIP* openzip(int pathtype, int pathindex, const char* zipfile) {
	/* allocate */
	ZIP* zip = (ZIP*)malloc( sizeof(ZIP) );
	if (!zip) {
		return 0;
	}

	/* open */
	zip->fp = osd_fopen(pathtype, pathindex, zipfile, "rb");
	if (!zip->fp) {
		errormsg ("Opening for reading", ERROR_FILESYSTEM, zipfile);
		free(zip);
		return 0;
	}

	/* go to end */
	if (fseek(zip->fp, 0L, SEEK_END) != 0) {
		errormsg ("Seeking to end", ERROR_FILESYSTEM, zipfile);
		fclose(zip->fp);
		free(zip);
		return 0;
	}

	/* get length */
	zip->length = ftell(zip->fp);
	if (zip->length < 0) {
		errormsg ("Get file size", ERROR_FILESYSTEM, zipfile);
		fclose(zip->fp);
		free(zip);
		return 0;
	}
	if (zip->length == 0) {
		errormsg ("Empty file", ERROR_CORRUPT, zipfile);
		fclose(zip->fp);
		free(zip);
		return 0;
	}

	/* read ecd data */
	if (ecd_read(zip)!=0) {
		errormsg ("Reading ECD (end of central directory)", ERROR_CORRUPT, zipfile);
		fclose(zip->fp);
		free(zip);
		return 0;
	}

	/* compile ecd info */
	zip->end_of_cent_dir_sig = read_dword (zip->ecd+ZIPESIG);
	zip->number_of_this_disk = read_word (zip->ecd+ZIPEDSK);
	zip->number_of_disk_start_cent_dir = read_word (zip->ecd+ZIPECEN);
	zip->total_entries_cent_dir_this_disk = read_word (zip->ecd+ZIPENUM);
	zip->total_entries_cent_dir = read_word (zip->ecd+ZIPECENN);
	zip->size_of_cent_dir = read_dword (zip->ecd+ZIPECSZ);
	zip->offset_to_start_of_cent_dir = read_dword (zip->ecd+ZIPEOFST);
	zip->zipfile_comment_length = read_word (zip->ecd+ZIPECOML);
	zip->zipfile_comment = zip->ecd+ZIPECOM;

	/* verify that we can work with this zipfile (no disk spanning allowed) */
	if ((zip->number_of_this_disk != zip->number_of_disk_start_cent_dir) ||
		(zip->total_entries_cent_dir_this_disk != zip->total_entries_cent_dir) ||
		(zip->total_entries_cent_dir < 1)) {
		errormsg("Cannot span disks", ERROR_UNSUPPORTED, zipfile);
		free(zip->ecd);
		fclose(zip->fp);
		free(zip);
		return 0;
	}

	if (fseek(zip->fp, zip->offset_to_start_of_cent_dir, SEEK_SET)!=0) {
		errormsg ("Seeking to central directory", ERROR_CORRUPT, zipfile);
		free(zip->ecd);
		fclose(zip->fp);
		free(zip);
		return 0;
	}

	/* read from start of central directory */
	zip->cd = (char*)malloc( zip->size_of_cent_dir );
	if (!zip->cd) {
		free(zip->ecd);
		fclose(zip->fp);
		free(zip);
		return 0;
	}

	if (fread(zip->cd, 1, zip->size_of_cent_dir, zip->fp)!=zip->size_of_cent_dir) {
		errormsg ("Reading central directory", ERROR_CORRUPT, zipfile);
		free(zip->cd);
		free(zip->ecd);
		fclose(zip->fp);
		free(zip);
		return 0;
	}

	/* reset ent */
	zip->ent.name = 0;

	/* rewind */
	zip->cd_pos = 0;

	/* file name */
	zip->zip = (char*)malloc(strlen(zipfile)+1);
	if (!zip->zip) {
		free(zip->cd);
		free(zip->ecd);
		fclose(zip->fp);
		free(zip);
		return 0;
	}
	strcpy(zip->zip, zipfile);
	zip->pathtype = pathtype;
	zip->pathindex = pathindex;

	return zip;
}

/* Reads the current entry from a zip stream
   in:
     zip opened zip
   return:
     !=0 success
     ==0 error
*/
struct zipent* readzip(ZIP* zip) {

	/* end of directory */
	if (zip->cd_pos >= zip->size_of_cent_dir)
		return 0;

	/* compile zipent info */
	zip->ent.cent_file_header_sig = read_dword (zip->cd+zip->cd_pos+ZIPCENSIG);
	zip->ent.version_made_by = *(zip->cd+zip->cd_pos+ZIPCVER);
	zip->ent.host_os = *(zip->cd+zip->cd_pos+ZIPCOS);
	zip->ent.version_needed_to_extract = *(zip->cd+zip->cd_pos+ZIPCVXT);
	zip->ent.os_needed_to_extract = *(zip->cd+zip->cd_pos+ZIPCEXOS);
	zip->ent.general_purpose_bit_flag = read_word (zip->cd+zip->cd_pos+ZIPCFLG);
	zip->ent.compression_method = read_word (zip->cd+zip->cd_pos+ZIPCMTHD);
	zip->ent.last_mod_file_time = read_word (zip->cd+zip->cd_pos+ZIPCTIM);
	zip->ent.last_mod_file_date = read_word (zip->cd+zip->cd_pos+ZIPCDAT);
	zip->ent.crc32 = read_dword (zip->cd+zip->cd_pos+ZIPCCRC);
	zip->ent.compressed_size = read_dword (zip->cd+zip->cd_pos+ZIPCSIZ);
	zip->ent.uncompressed_size = read_dword (zip->cd+zip->cd_pos+ZIPCUNC);
	zip->ent.filename_length = read_word (zip->cd+zip->cd_pos+ZIPCFNL);
	zip->ent.extra_field_length = read_word (zip->cd+zip->cd_pos+ZIPCXTL);
	zip->ent.file_comment_length = read_word (zip->cd+zip->cd_pos+ZIPCCML);
	zip->ent.disk_number_start = read_word (zip->cd+zip->cd_pos+ZIPDSK);
	zip->ent.internal_file_attrib = read_word (zip->cd+zip->cd_pos+ZIPINT);
	zip->ent.external_file_attrib = read_dword (zip->cd+zip->cd_pos+ZIPEXT);
	zip->ent.offset_lcl_hdr_frm_frst_disk = read_dword (zip->cd+zip->cd_pos+ZIPOFST);

    /* check to see if filename length is illegally long (past the size of this directory
       entry) */
    if (zip->cd_pos + ZIPCFN + zip->ent.filename_length > zip->size_of_cent_dir)
    {
        errormsg("Invalid filename length in directory", ERROR_CORRUPT,zip->zip);
        return 0;
    }

	/* copy filename */
	free(zip->ent.name);
	zip->ent.name = (char*)malloc(zip->ent.filename_length + 1);
	memcpy(zip->ent.name, zip->cd+zip->cd_pos+ZIPCFN, zip->ent.filename_length);
	zip->ent.name[zip->ent.filename_length] = 0;

	/* skip to next entry in central dir */
	zip->cd_pos += ZIPCFN + zip->ent.filename_length + zip->ent.extra_field_length + zip->ent.file_comment_length;

	return &zip->ent;
}

/* Closes a zip stream */
void closezip(ZIP* zip) {
	/* release all */
	free(zip->ent.name);
	free(zip->cd);
	free(zip->ecd);
	/* only if not suspended */
	if (zip->fp)
		fclose(zip->fp);
	free(zip->zip);
	free(zip);
}

/* Suspend access to a zip file (release file handler)
   in:
      zip opened zip
   note:
     A suspended zip is automatically reopened at first call of
     readuncompressd() or readcompressed() functions
*/
void suspendzip(ZIP* zip) {
	if (zip->fp) {
		fclose(zip->fp);
		zip->fp = 0;
	}
}

/* Revive a suspended zip file (reopen file handler)
   in:
     zip suspended zip
   return:
	zip success
	==0 error (zip must be closed with closezip)
*/
static ZIP* revivezip(ZIP* zip) {
	if (!zip->fp) {
		zip->fp = osd_fopen(zip->pathtype, zip->pathindex, zip->zip, "rb");
		if (!zip->fp) {
			return 0;
		}
	}
	return zip;

}

/* Reset a zip stream to the first entry
   in:
     zip opened zip
   note:
     ZIP file must be opened and not suspended
*/
void rewindzip(ZIP* zip) {
	zip->cd_pos = 0;
}

/* Seek zip->fp to compressed data
   return:
	==0 success
	<0 error
*/
int seekcompresszip(ZIP* zip, struct zipent* ent) {
	char buf[ZIPNAME];
	long offset;

	if (!zip->fp) {
		if (!revivezip(zip))
			return -1;
	}

	if (fseek(zip->fp, ent->offset_lcl_hdr_frm_frst_disk, SEEK_SET)!=0) {
		errormsg ("Seeking to header", ERROR_CORRUPT, zip->zip);
		return -1;
	}

	if (fread(buf, 1, ZIPNAME, zip->fp)!=ZIPNAME) {
		errormsg ("Reading header", ERROR_CORRUPT, zip->zip);
		return -1;
	}

	{
		UINT16 filename_length = read_word (buf+ZIPFNLN);
		UINT16 extra_field_length = read_word (buf+ZIPXTRALN);

		/* calculate offset to data and fseek() there */
		offset = ent->offset_lcl_hdr_frm_frst_disk + ZIPNAME + filename_length + extra_field_length;

		if (fseek(zip->fp, offset, SEEK_SET) != 0) {
			errormsg ("Seeking to compressed data", ERROR_CORRUPT, zip->zip);
			return -1;
		}

	}

	return 0;
}

/* Inflate a file
   in:
   in_file stream to inflate
   in_size size of the compressed data to read
   out_size size of decompressed data
   out:
   out_data buffer for decompressed data
   return:
   ==0 ok

   990525 rewritten for use with zlib MLR
*/
static int inflate_file(FILE* in_file, unsigned in_size, unsigned char* out_data, unsigned out_size)
{
    int err;
	unsigned char* in_buffer;
    z_stream d_stream; /* decompression stream */

    d_stream.zalloc = 0;
    d_stream.zfree = 0;
    d_stream.opaque = 0;

	d_stream.next_in  = 0;
	d_stream.avail_in = 0;
    d_stream.next_out = out_data;
    d_stream.avail_out = out_size;

    err = inflateInit2(&d_stream, -MAX_WBITS);
	/* windowBits is passed < 0 to tell that there is no zlib header.
	 * Note that in this case inflate *requires* an extra "dummy" byte
	 * after the compressed stream in order to complete decompression and
	 * return Z_STREAM_END.
	 */
    if (err != Z_OK)
	{
		log_cb(RETRO_LOG_ERROR, LOGPRE "inflateInit error: %d\n", err);
        return -1;
	}

	in_buffer = (unsigned char*)malloc(INFLATE_INPUT_BUFFER_MAX+1);
	if (!in_buffer)
		return -1;

    for (;;)
	{
		if (in_size <= 0)
		{
			log_cb(RETRO_LOG_ERROR, LOGPRE "inflate error: compressed size too small\n");
			free (in_buffer);
			return -1;
		}
		d_stream.next_in  = in_buffer;
		d_stream.avail_in = fread (in_buffer, 1, MIN(in_size, INFLATE_INPUT_BUFFER_MAX), in_file);
		in_size -= d_stream.avail_in;
		if (in_size == 0)
			d_stream.avail_in++; /* add dummy byte at end of compressed data */

        err = inflate(&d_stream, Z_NO_FLUSH);
        if (err == Z_STREAM_END)
			break;
		if (err != Z_OK)
		{
			log_cb(RETRO_LOG_ERROR, LOGPRE "inflate error: %d\n", err);
			free (in_buffer);
			return -1;
		}
    }

    err = inflateEnd(&d_stream);
	if (err != Z_OK)
	{
		log_cb(RETRO_LOG_ERROR, LOGPRE "inflateEnd error: %d\n", err);
		free (in_buffer);
		return -1;
	}

	free (in_buffer);

	if ((d_stream.avail_out > 0) || (in_size > 0))
	{
		log_cb(RETRO_LOG_ERROR, LOGPRE "zip size mismatch. %i\n", in_size);
		return -1;
	}

	return 0;
}

/* Read compressed data
   out:
	data compressed data read
   return:
	==0 success
	<0 error
*/
int readcompresszip(ZIP* zip, struct zipent* ent, char* data) {
	int err = seekcompresszip(zip,ent);
	if (err!=0)
		return err;

	if (fread(data, 1, ent->compressed_size, zip->fp)!=ent->compressed_size) {
		errormsg ("Reading compressed data", ERROR_CORRUPT, zip->zip);
		return -1;
	}

	return 0;
}

/* Read UNcompressed data
   out:
	data UNcompressed data
   return:
	==0 success
	<0 error
*/
int readuncompresszip(ZIP* zip, struct zipent* ent, char* data) {
	if (ent->compression_method == 0x0000) {
		/* file is not compressed, simply stored */

		/* check if size are equal */
		if (ent->compressed_size != ent->uncompressed_size) {
			errormsg("Wrong uncompressed size in store compression", ERROR_CORRUPT,zip->zip);
			return -3;
		}

		return readcompresszip(zip,ent,data);
	} else if (ent->compression_method == 0x0008) {
		/* file is compressed using "Deflate" method */
		if (ent->version_needed_to_extract > 0x14) {
			errormsg("Version too new", ERROR_UNSUPPORTED,zip->zip);
			return -2;
		}

		if (ent->os_needed_to_extract != 0x00) {
			errormsg("OS not supported", ERROR_UNSUPPORTED,zip->zip);
			return -2;
		}

		if (ent->disk_number_start != zip->number_of_this_disk) {
			errormsg("Cannot span disks", ERROR_UNSUPPORTED,zip->zip);
			return -2;
		}

		/* read compressed data */
		if (seekcompresszip(zip,ent)!=0) {
			return -1;
		}

		/* configure inflate */
		if (inflate_file( zip->fp, ent->compressed_size, (unsigned char*)data, ent->uncompressed_size))
		{
			errormsg("Inflating compressed data", ERROR_CORRUPT, zip->zip);
			return -3;
		}

		return 0;
	} else {
		errormsg("Compression method unsupported", ERROR_UNSUPPORTED, zip->zip);
		return -2;
	}
}

/* -------------------------------------------------------------------------
   Zip cache support
 ------------------------------------------------------------------------- */

/* Use the zip cache */
#define ZIP_CACHE

#ifdef ZIP_CACHE

/* ZIP cache entries */
#define ZIP_CACHE_MAX 5

/* ZIP cache buffer LRU ( Last Recently Used )
     zip_cache_map[0] is the newer
     zip_cache_map[ZIP_CACHE_MAX-1] is the older
*/
static ZIP* zip_cache_map[ZIP_CACHE_MAX];

static ZIP* cache_openzip(int pathtype, int pathindex, const char* zipfile) {
	ZIP* zip;
	unsigned i;

	/* search in the cache buffer */
	for(i=0;i<ZIP_CACHE_MAX;++i) {
		if (zip_cache_map[i] && zip_cache_map[i]->pathtype == pathtype && zip_cache_map[i]->pathindex == pathindex && strcmp(zip_cache_map[i]->zip,zipfile)==0) {
			/* found */
			unsigned j;

/*
			log_cb(RETRO_LOG_ERROR, LOGPRE "Zip cache HIT  for %s\n", zipfile);
*/

			/* reset the zip directory */
			rewindzip( zip_cache_map[i] );

			/* store */
			zip = zip_cache_map[i];

			/* shift */
			for(j=i;j>0;--j)
				zip_cache_map[j] = zip_cache_map[j-1];

			/* set the first entry */
			zip_cache_map[0] = zip;

			return zip_cache_map[0];
		}
	}
	/* not found */

/*
	log_cb(RETRO_LOG_ERROR, LOGPRE "Zip cache FAIL for %s\n", zipfile);
*/

	/* open the zip */
	zip = openzip( pathtype, pathindex, zipfile );
	if (!zip)
		return 0;

	/* close the oldest entry */
	if (zip_cache_map[ZIP_CACHE_MAX-1]) {
		/* close last zip */
		closezip(zip_cache_map[ZIP_CACHE_MAX-1]);
		/* reset the entry */
		zip_cache_map[ZIP_CACHE_MAX-1] = 0;
	}

	/* shift */
	for(i=ZIP_CACHE_MAX-1;i>0;--i)
		zip_cache_map[i] = zip_cache_map[i-1];

	/* set the first entry */
	zip_cache_map[0] = zip;

	return zip_cache_map[0];
}

static void cache_closezip(ZIP* zip) {
	unsigned i;

	/* search in the cache buffer */
	for(i=0;i<ZIP_CACHE_MAX;++i) {
		if (zip_cache_map[i]==zip) {
			/* close zip */
			closezip(zip);

			/* reset cache entry */
			zip_cache_map[i] = 0;
			return;

		}
	}
	/* not found */

	/* close zip */
	closezip(zip);
}

/* CK980415 added to allow osd code to clear zip cache for auditing--each time
   the user opens up an audit for a game we should reread the zip */
void unzip_cache_clear()
{
	unsigned i;

	/* search in the cache buffer for any zip info and clear it */
	for(i=0;i<ZIP_CACHE_MAX;++i) {
		if (zip_cache_map[i] != NULL) {
			/* close zip */
			closezip(zip_cache_map[i]);

			/* reset cache entry */
			zip_cache_map[i] = 0;
/*			return; */

		}
	}
}

#define cache_suspendzip(a) suspendzip(a)

#else

#define cache_openzip(a,b,c) openzip(a,b,c)
#define cache_closezip(a) closezip(a)
#define cache_suspendzip(a) closezip(a)

#define unzip_cache_clear()

#endif

/* -------------------------------------------------------------------------
   Backward MAME compatibility
 ------------------------------------------------------------------------- */

/* Compare two filename
   note:
     don't check directory in zip and ignore case
*/
static int equal_filename(const char* zipfile, const char* file) {
	const char* s1 = file;
	/* start comparison after last / */
	const char* s2 = strrchr(zipfile,'/');
	if (s2)
		++s2;
	else
		s2 = zipfile;
	while (*s1 && toupper(*s1)==toupper(*s2)) {
		++s1;
		++s2;
	}
	return !*s1 && !*s2;
}

/* Pass the path to the zipfile and the name of the file within the zipfile.
   buf will be set to point to the uncompressed image of that zipped file.
   length will be set to the length of the uncompressed data. */
int /* error */ load_zipped_file (int pathtype, int pathindex, const char* zipfile, const char* filename, unsigned char** buf, unsigned int* length) {
	ZIP* zip;
	struct zipent* ent;

	zip = cache_openzip(pathtype, pathindex, zipfile);
	if (!zip)
		return -1;

	while (readzip(zip)) {
		/* NS981003: support for "load by CRC" */
		char crc[9];

		ent = &(zip->ent);

		sprintf(crc,"%08x",ent->crc32);
		if (equal_filename(ent->name, filename) ||
				(ent->crc32 && !strcmp(crc, filename)))
		{
			*length = ent->uncompressed_size;
			*buf = (unsigned char*)malloc( *length );
			if (!*buf) {
				if (!gUnzipQuiet)
					log_cb(RETRO_LOG_ERROR, LOGPRE "load_zipped_file(): Unable to allocate %d bytes of RAM\n",*length);
				cache_closezip(zip);
				return -1;
			}

			if (readuncompresszip(zip, ent, (char*)*buf)!=0) {
				free(*buf);
				cache_closezip(zip);
				return -1;
			}

			cache_suspendzip(zip);
			return 0;
		}
	}

	cache_suspendzip(zip);
	return -1;
}

/*	Pass the path to the zipfile and the name of the file within the zipfile.
	sum will be set to the CRC-32 of that zipped file. */
/*  The caller can preset sum to the expected checksum to enable "load by CRC" */
int /* error */ checksum_zipped_file (int pathtype, int pathindex, const char *zipfile, const char *filename, unsigned int *length, unsigned int *sum) {
	ZIP* zip;
	struct zipent* ent;

	zip = cache_openzip(pathtype, pathindex, zipfile);
	if (!zip)
		return -1;

	while (readzip(zip)) {
		ent = &(zip->ent);

		if (equal_filename(ent->name, filename))
		{
			*length = ent->uncompressed_size;
			*sum = ent->crc32;
			cache_suspendzip(zip);
			return 0;
		}
	}

	cache_suspendzip(zip);

	/* NS981003: support for "load by CRC" */
	zip = cache_openzip(pathtype, pathindex, zipfile);
	if (!zip)
		return -1;

	while (readzip(zip)) {
		ent = &(zip->ent);

		if (*sum && ent->crc32 == *sum)
		{
			*length = ent->uncompressed_size;
			*sum = ent->crc32;
			cache_suspendzip(zip);
			return 0;
		}
	}

	cache_suspendzip(zip);
	return -1;
}
