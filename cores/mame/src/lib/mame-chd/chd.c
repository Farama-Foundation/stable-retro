/***************************************************************************

	MAME Compressed Hunks of Data file format

***************************************************************************/

#include <chd.h>
#include <retro_inline.h>
#include <utils/md5.h>
#include "mame_sha1.h"
#include <compat/zlib.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>


/*************************************
 *
 *	Debugging
 *
 *************************************/

#define PRINTF_MAX_HUNK				(0)



/*************************************
 *
 *	Constants
 *
 *************************************/

#define MAP_STACK_ENTRIES			512			/* max number of entries to use on the stack */
#define MAP_ENTRY_SIZE				16			/* V3 and later */
#define OLD_MAP_ENTRY_SIZE			8			/* V1-V2 */
#define METADATA_HEADER_SIZE		16			/* metadata header size */
#define CRCMAP_HASH_SIZE			4095		/* number of CRC hashtable entries */

#define MAP_ENTRY_FLAG_TYPE_MASK	0x000f		/* what type of hunk */
#define MAP_ENTRY_FLAG_NO_CRC		0x0010		/* no CRC is present */

#define MAP_ENTRY_TYPE_INVALID		0x0000		/* invalid type */
#define MAP_ENTRY_TYPE_COMPRESSED	0x0001		/* standard compression */
#define MAP_ENTRY_TYPE_UNCOMPRESSED	0x0002		/* uncompressed data */
#define MAP_ENTRY_TYPE_MINI			0x0003		/* mini: use offset as raw data */
#define MAP_ENTRY_TYPE_SELF_HUNK	0x0004		/* same as another hunk in this file */
#define MAP_ENTRY_TYPE_PARENT_HUNK	0x0005		/* same as a hunk in the parent file */

#define CHD_V1_SECTOR_SIZE			512			/* size of a "sector" in the V1 header */

#define COOKIE_VALUE				0xbaadf00d
#define MAX_ZLIB_ALLOCS				64

#define END_OF_LIST_COOKIE			"EndOfListCookie"

#define NO_MATCH					(~0)



/*************************************
 *
 *	Macros
 *
 *************************************/

#define SET_ERROR_AND_CLEANUP(err) do { last_error = (err); goto cleanup; } while (0)

#ifdef _MSC_VER
#define interface interface_
#endif



/*************************************
 *
 *	Type definitions
 *
 *************************************/

struct map_entry
{
	UINT64					offset;			/* offset within the file of the data */
	UINT32					crc;			/* 32-bit CRC of the data */
	UINT16					length;			/* length of the data */
	UINT16					flags;			/* misc flags */
};

struct crcmap_entry
{
	UINT32					hunknum;		/* hunk number */
	struct crcmap_entry *	next;			/* next entry in list */
};

struct metadata_entry
{
	UINT64					offset;			/* offset within the file of the header */
	UINT64					next;			/* offset within the file of the next header */
	UINT64					prev;			/* offset within the file of the previous header */
	UINT32					length;			/* length of the metadata */
	UINT32					metatag;		/* metadata tag */
};

struct chd_file
{
	UINT32					cookie;			/* cookie, should equal COOKIE_VALUE */
	struct chd_file *		next;			/* pointer to next file in the global list */

	struct chd_interface_file *file;		/* handle to the open file */
	struct chd_header 		header;			/* header, extracted from file */

	struct chd_file *		parent;			/* pointer to parent file, or NULL */

	struct map_entry *		map;			/* array of map entries */

	UINT8 *					cache;			/* hunk cache pointer */
	UINT32					cachehunk;		/* index of currently cached hunk */

	UINT8 *					compare;		/* hunk compare pointer */
	UINT32					comparehunk;	/* index of current compare data */

	UINT8 *					compressed;		/* pointer to buffer for compressed data */
	void *					codecdata;		/* opaque pointer to codec data */

	struct crcmap_entry *	crcmap;			/* CRC map entries */
	struct crcmap_entry *	crcfree;		/* free list CRC entries */
	struct crcmap_entry **	crctable;		/* table of CRC entries */

	UINT32					maxhunk;		/* maximum hunk accessed */
};

struct zlib_codec_data
{
	z_stream				inflater;
	z_stream				deflater;
	UINT32 *				allocptr[MAX_ZLIB_ALLOCS];
};



/*************************************
 *
 *	Local variables
 *
 *************************************/

static struct chd_interface interface;
static struct chd_file *first_file;
static int last_error;

static const UINT8 nullmd5[CHD_MD5_BYTES] = { 0 };
static const UINT8 nullsha1[CHD_SHA1_BYTES] = { 0 };



/*************************************
 *
 *	Prototypes
 *
 *************************************/

static int validate_header(const struct chd_header *header);
static int read_hunk_into_memory(struct chd_file *chd, UINT32 hunknum, UINT8 *dest);
static int read_hunk_into_cache(struct chd_file *chd, UINT32 hunknum);
static int write_hunk_from_memory(struct chd_file *chd, UINT32 hunknum, const UINT8 *src);
static int read_header(struct chd_interface_file *file, struct chd_header *header);
static int write_header(struct chd_interface_file *file, const struct chd_header *header);
static int read_hunk_map(struct chd_file *chd);
static void init_crcmap(struct chd_file *chd, int prepopulate);
static void add_to_crcmap(struct chd_file *chd, UINT32 hunknum);
static UINT32 find_matching_hunk(struct chd_file *chd, UINT32 hunknum, UINT32 crc, const UINT8 *rawdata);
static int find_metadata_entry(struct chd_file *chd, UINT32 metatag, UINT32 metaindex, struct metadata_entry *metaentry);

static int init_codec(struct chd_file *chd);
static void free_codec(struct chd_file *chd);

static struct chd_interface_file *multi_open(const char *filename, const char *mode);
static void multi_close(struct chd_interface_file *file);
static UINT32 multi_read(struct chd_interface_file *file, UINT64 offset, UINT32 count, void *buffer);
static UINT32 multi_write(struct chd_interface_file *file, UINT64 offset, UINT32 count, const void *buffer);
static UINT64 multi_length(struct chd_interface_file *file);



/*************************************
 *
 *	Inline helpers
 *
 *************************************/

static INLINE UINT64 get_bigendian_uint64(const UINT8 *base)
{
	return ((UINT64)base[0] << 56) | ((UINT64)base[1] << 48) | ((UINT64)base[2] << 40) | ((UINT64)base[3] << 32) |
			((UINT64)base[4] << 24) | ((UINT64)base[5] << 16) | ((UINT64)base[6] << 8) | (UINT64)base[7];
}

static INLINE void put_bigendian_uint64(UINT8 *base, UINT64 value)
{
	base[0] = value >> 56;
	base[1] = value >> 48;
	base[2] = value >> 40;
	base[3] = value >> 32;
	base[4] = value >> 24;
	base[5] = value >> 16;
	base[6] = value >> 8;
	base[7] = value;
}

static INLINE UINT32 get_bigendian_uint32(const UINT8 *base)
{
	return (base[0] << 24) | (base[1] << 16) | (base[2] << 8) | base[3];
}

static INLINE void put_bigendian_uint32(UINT8 *base, UINT32 value)
{
	base[0] = value >> 24;
	base[1] = value >> 16;
	base[2] = value >> 8;
	base[3] = value;
}

static INLINE UINT16 get_bigendian_uint16(const UINT8 *base)
{
	return (base[0] << 8) | base[1];
}

static INLINE void put_bigendian_uint16(UINT8 *base, UINT16 value)
{
	base[0] = value >> 8;
	base[1] = value;
}

static INLINE void extract_map_entry(const UINT8 *base, struct map_entry *entry)
{
	entry->offset = get_bigendian_uint64(&base[0]);
	entry->crc = get_bigendian_uint32(&base[8]);
	entry->length = get_bigendian_uint16(&base[12]);
	entry->flags = get_bigendian_uint16(&base[14]);
}

static INLINE void assemble_map_entry(UINT8 *base, struct map_entry *entry)
{
	put_bigendian_uint64(&base[0], entry->offset);
	put_bigendian_uint32(&base[8], entry->crc);
	put_bigendian_uint16(&base[12], entry->length);
	put_bigendian_uint16(&base[14], entry->flags);
}

static INLINE void extract_old_map_entry(const UINT8 *base, struct map_entry *entry, UINT32 hunkbytes)
{
	entry->offset = get_bigendian_uint64(&base[0]);
	entry->crc = 0;
	entry->length = entry->offset >> 44;
	entry->flags = MAP_ENTRY_FLAG_NO_CRC | ((entry->length == hunkbytes) ? MAP_ENTRY_TYPE_UNCOMPRESSED : MAP_ENTRY_TYPE_COMPRESSED);
	entry->offset = (entry->offset << 20) >> 20;
}

static INLINE void assemble_old_map_entry(UINT8 *base, struct map_entry *entry)
{
	UINT64 data = entry->offset | ((UINT64)entry->length << 44);
	put_bigendian_uint64(&base[0], data);
}



/*************************************
 *
 *	Interface setup
 *
 *************************************/

void chd_set_interface(struct chd_interface *new_interface)
{
	if (new_interface)
		interface = *new_interface;
	else
		memset(&interface, 0, sizeof(interface));
}



/*************************************
 *
 *	Interface save
 *
 *************************************/

void chd_save_interface(struct chd_interface *interface_save)
{
	*interface_save = interface;
}



/*************************************
 *
 *	Create a new data file
 *
 *************************************/

int chd_create(const char *filename, UINT64 logicalbytes, UINT32 hunkbytes, UINT32 compression, struct chd_file *parent)
{
	UINT8 blank_map_entries[MAP_STACK_ENTRIES * MAP_ENTRY_SIZE];
	int fullchunks, remainder, count;
	struct chd_interface_file *file = NULL;
	struct map_entry mapentry;
	struct chd_header header;
	UINT64 fileoffset;
	int i, j, err;

	last_error = CHDERR_NONE;

	/* punt if no interface */
	if (!interface.open)
		SET_ERROR_AND_CLEANUP(CHDERR_NO_INTERFACE);

	/* verify parameters */
	if (!filename)
		SET_ERROR_AND_CLEANUP(CHDERR_FILE_NOT_FOUND);
	if (compression >= CHDCOMPRESSION_MAX)
		SET_ERROR_AND_CLEANUP(CHDERR_INVALID_PARAMETER);
	if (!parent && (logicalbytes == 0 || hunkbytes == 0))
		SET_ERROR_AND_CLEANUP(CHDERR_INVALID_PARAMETER);

	/* if we have a parent, the sizes come from there */
	if (parent)
	{
		logicalbytes = parent->header.logicalbytes;
		hunkbytes = parent->header.hunkbytes;
	}

	/* if we have a parent, it must be V3 or later */
	if (parent && parent->header.version < 3)
		SET_ERROR_AND_CLEANUP(CHDERR_UNSUPPORTED_VERSION);

	/* build the header */
	header.length = CHD_V3_HEADER_SIZE;
	header.version = CHD_HEADER_VERSION;
	header.flags = CHDFLAGS_IS_WRITEABLE;
	header.compression = compression;
	header.hunkbytes = hunkbytes;
	header.totalhunks = (logicalbytes + hunkbytes - 1) / hunkbytes;
	header.logicalbytes = logicalbytes;
	header.metaoffset = 0;
	memset(&header.md5[0], 0, sizeof(header.md5));
	memset(&header.parentmd5[0], 0, sizeof(header.parentmd5));
	memset(&header.sha1[0], 0, sizeof(header.sha1));
	memset(&header.parentsha1[0], 0, sizeof(header.parentsha1));
	header.obsolete_cylinders = 0;
	header.obsolete_sectors = 0;
	header.obsolete_heads = 0;
	header.obsolete_hunksize = 0;

	/* tweaks if there is a parent */
	if (parent)
	{
		header.flags |= CHDFLAGS_HAS_PARENT;
		memcpy(&header.parentmd5[0], &parent->header.md5[0], sizeof(header.parentmd5));
		memcpy(&header.parentsha1[0], &parent->header.sha1[0], sizeof(header.parentsha1));
	}

	/* validate it */
	err = validate_header(&header);
	if (err != CHDERR_NONE)
		SET_ERROR_AND_CLEANUP(err);

	/* attempt to create the file */
	file = multi_open(filename, "wb");
	if (!file)
		SET_ERROR_AND_CLEANUP(CHDERR_CANT_CREATE_FILE);

	/* write the resulting header */
	err = write_header(file, &header);
	if (err != CHDERR_NONE)
		SET_ERROR_AND_CLEANUP(err);

	/* create a mini hunk of 0's */
	mapentry.offset = 0;
	mapentry.crc = 0;
	mapentry.length = 0;
	mapentry.flags = MAP_ENTRY_TYPE_MINI | MAP_ENTRY_FLAG_NO_CRC;
	for (i = 0; i < MAP_STACK_ENTRIES; i++)
		assemble_map_entry(&blank_map_entries[i * MAP_ENTRY_SIZE], &mapentry);

	/* prepare to write a blank hunk map immediately following */
	fileoffset = header.length;
	fullchunks = header.totalhunks / MAP_STACK_ENTRIES;
	remainder = header.totalhunks % MAP_STACK_ENTRIES;

	/* first write full chunks of blank entries */
	for (i = 0; i < fullchunks; i++)
	{
		/* parent drives need to be mapped through */
		if (parent)
			for (j = 0; j < MAP_STACK_ENTRIES; j++)
			{
				mapentry.offset = i * MAP_STACK_ENTRIES + j;
				mapentry.crc = parent->map[i * MAP_STACK_ENTRIES + j].crc;
				mapentry.flags = MAP_ENTRY_TYPE_PARENT_HUNK;
				assemble_map_entry(&blank_map_entries[j * MAP_ENTRY_SIZE], &mapentry);
			}

		/* write the chunks */
		count = multi_write(file, fileoffset, sizeof(blank_map_entries), blank_map_entries);
		if (count != sizeof(blank_map_entries))
			SET_ERROR_AND_CLEANUP(CHDERR_WRITE_ERROR);
		fileoffset += sizeof(blank_map_entries);
	}

	/* then write the remainder */
	if (remainder)
	{
		/* parent drives need to be mapped through */
		if (parent)
			for (j = 0; j < remainder; j++)
			{
				mapentry.offset = i * MAP_STACK_ENTRIES + j;
				mapentry.crc = parent->map[i * MAP_STACK_ENTRIES + j].crc;
				mapentry.flags = MAP_ENTRY_TYPE_PARENT_HUNK;
				assemble_map_entry(&blank_map_entries[j * MAP_ENTRY_SIZE], &mapentry);
			}

		/* write the chunks */
		count = multi_write(file, fileoffset, remainder * MAP_ENTRY_SIZE, blank_map_entries);
		if (count != remainder * MAP_ENTRY_SIZE)
			SET_ERROR_AND_CLEANUP(CHDERR_WRITE_ERROR);
		fileoffset += remainder * MAP_ENTRY_SIZE;
	}

	/* then write a special end-of-list cookie */
	memcpy(&blank_map_entries[0], END_OF_LIST_COOKIE, MAP_ENTRY_SIZE);
	count = multi_write(file, fileoffset, MAP_ENTRY_SIZE, blank_map_entries);
	if (count != MAP_ENTRY_SIZE)
		SET_ERROR_AND_CLEANUP(CHDERR_WRITE_ERROR);

	/* all done */
	multi_close(file);

	/* if we have a parent, clone the metadata */
	if (parent)
	{
		UINT8 metadata[CHD_MAX_METADATA_SIZE];
		UINT32 metatag, metasize, metaindex;

		/* open the new CHD */
		struct chd_file *newchd = chd_open(filename, 1, parent);
		if (newchd == NULL)
			SET_ERROR_AND_CLEANUP(last_error);

		/* clone the metadata */
		for (metaindex = 0; ; metaindex++)
		{
			metatag = CHDMETATAG_WILDCARD;
			metasize = chd_get_metadata(parent, &metatag, metaindex, metadata, sizeof(metadata));
			if (metasize == 0 || chd_get_last_error() == CHDERR_METADATA_NOT_FOUND)
				break;

			err = chd_set_metadata(newchd, metatag, CHD_METAINDEX_APPEND, metadata, metasize);
			if (err != CHDERR_NONE)
				SET_ERROR_AND_CLEANUP(err);
		}

		/* close the file */
		chd_close(newchd);
	}
	return CHDERR_NONE;

cleanup:
	if (file)
		multi_close(file);
	return last_error;
}



/*************************************
 *
 *	Opening a data file
 *
 *************************************/

struct chd_file *chd_open(const char *filename, int writeable, struct chd_file *parent)
{
	struct chd_file *finalchd;
	struct chd_file chd = { 0 };
	int err;

	last_error = CHDERR_NONE;

	/* punt if no interface */
	if (!interface.open)
		SET_ERROR_AND_CLEANUP(CHDERR_NO_INTERFACE);

	/* verify parameters */
	if (!filename)
		SET_ERROR_AND_CLEANUP(CHDERR_FILE_NOT_FOUND);

	/* punt if invalid parent */
	chd.parent = parent;
	if (chd.parent && chd.parent->cookie != COOKIE_VALUE)
		SET_ERROR_AND_CLEANUP(CHDERR_INVALID_PARAMETER);

	/* first attempt to open the file */
	chd.file = multi_open(filename, writeable ? "rb+" : "rb");
	if (!chd.file)
		SET_ERROR_AND_CLEANUP(CHDERR_FILE_NOT_FOUND);

	/* now attempt to read the header */
	err = read_header(chd.file, &chd.header);
	if (err != CHDERR_NONE)
		SET_ERROR_AND_CLEANUP(err);

	/* validate the header */
	err = validate_header(&chd.header);
	if (err != CHDERR_NONE)
		SET_ERROR_AND_CLEANUP(err);

	/* make sure we don't open a read-only file writeable */
	if (writeable && !(chd.header.flags & CHDFLAGS_IS_WRITEABLE))
		SET_ERROR_AND_CLEANUP(CHDERR_FILE_NOT_WRITEABLE);

	/* also, never open an older version writeable */
	if (writeable && chd.header.version < CHD_HEADER_VERSION)
		SET_ERROR_AND_CLEANUP(CHDERR_UNSUPPORTED_VERSION);

	/* if we need a parent, make sure we have one */
	if (!parent && (chd.header.flags & CHDFLAGS_HAS_PARENT))
		SET_ERROR_AND_CLEANUP(CHDERR_REQUIRES_PARENT);

	/* make sure we have a valid parent */
	if (parent)
	{
		/* check MD5 if it isn't empty */
		if (memcmp(nullmd5, chd.header.parentmd5, sizeof(chd.header.parentmd5)) &&
			memcmp(nullmd5, chd.parent->header.md5, sizeof(chd.parent->header.md5)) &&
			memcmp(chd.parent->header.md5, chd.header.parentmd5, sizeof(chd.header.parentmd5)))
			SET_ERROR_AND_CLEANUP(CHDERR_INVALID_PARENT);

		/* check SHA1 if it isn't empty */
		if (memcmp(nullsha1, chd.header.parentsha1, sizeof(chd.header.parentsha1)) &&
			memcmp(nullsha1, chd.parent->header.sha1, sizeof(chd.parent->header.sha1)) &&
			memcmp(chd.parent->header.sha1, chd.header.parentsha1, sizeof(chd.header.parentsha1)))
			SET_ERROR_AND_CLEANUP(CHDERR_INVALID_PARENT);
	}

	/* now read the hunk map */
	err = read_hunk_map(&chd);
	if (err != CHDERR_NONE)
		SET_ERROR_AND_CLEANUP(err);

	/* allocate and init the hunk cache */
	chd.cache = malloc(chd.header.hunkbytes);
	chd.compare = malloc(chd.header.hunkbytes);
	if (!chd.cache || !chd.compare)
		SET_ERROR_AND_CLEANUP(CHDERR_OUT_OF_MEMORY);
	chd.cachehunk = ~0;
	chd.comparehunk = ~0;

	/* allocate the temporary compressed buffer */
	chd.compressed = malloc(chd.header.hunkbytes);
	if (!chd.compressed)
		SET_ERROR_AND_CLEANUP(CHDERR_OUT_OF_MEMORY);

	/* now init the codec */
	err = init_codec(&chd);
	if (err != CHDERR_NONE)
		SET_ERROR_AND_CLEANUP(err);

	/* okay, now allocate our entry and copy it */
	finalchd = malloc(sizeof(chd));
	if (!finalchd)
		SET_ERROR_AND_CLEANUP(CHDERR_OUT_OF_MEMORY);
	*finalchd = chd;

	/* hook us into the global list */
	finalchd->cookie = COOKIE_VALUE;
	finalchd->next = first_file;
	first_file = finalchd;

	/* all done */
	return finalchd;

cleanup:
	if (chd.codecdata)
		free_codec(&chd);
	if (chd.compressed)
		free(chd.compressed);
	if (chd.compare)
		free(chd.compare);
	if (chd.cache)
		free(chd.cache);
	if (chd.map)
		free(chd.map);
	if (chd.file)
		multi_close(chd.file);
	return NULL;
}



/*************************************
 *
 *	Closing a data file
 *
 *************************************/

void chd_close(struct chd_file *chd)
{
	struct chd_file *curr, *prev;

	/* punt if NULL or invalid */
	if (!chd || chd->cookie != COOKIE_VALUE)
		return;

	/* deinit the codec */
	if (chd->codecdata)
		free_codec(chd);

	/* free the compressed data buffer */
	if (chd->compressed)
		free(chd->compressed);

	/* free the hunk cache and compare data */
	if (chd->compare)
		free(chd->compare);
	if (chd->cache)
		free(chd->cache);

	/* free the hunk map */
	if (chd->map)
		free(chd->map);

	/* free the CRC map */
	if (chd->crcmap)
		free(chd->crcmap);

	/* close the file */
	if (chd->file)
		multi_close(chd->file);

	/* unlink ourselves */
	for (prev = NULL, curr = first_file; curr; prev = curr, curr = curr->next)
		if (curr == chd)
		{
			if (prev)
				prev->next = curr->next;
			else
				first_file = curr->next;
			break;
		}

#if PRINTF_MAX_HUNK
	printf("Max hunk = %d/%d\n", chd->maxhunk, chd->header.totalhunks);
#endif

	/* free our memory */
	free(chd);
}



/*************************************
 *
 *	Closing all open data files
 *
 *************************************/

void chd_close_all(void)
{
	while (first_file)
		chd_close(first_file);
}



/*************************************
 *
 *	Read metadata from a data file
 *
 *************************************/

UINT32 chd_get_metadata(struct chd_file *chd, UINT32 *metatag, UINT32 metaindex, void *outputbuf, UINT32 outputlen)
{
	struct metadata_entry metaentry;
	UINT32 count;

	/* if we didn't find it, just return */
	last_error = find_metadata_entry(chd, *metatag, metaindex, &metaentry);
	if (last_error != CHDERR_NONE)
	{
		/* unless we're an old version and they are requesting hard disk metadata */
		if (chd->header.version < 3 && (*metatag == HARD_DISK_STANDARD_METADATA || *metatag == CHDMETATAG_WILDCARD) && metaindex == 0)
		{
			/* fill in the faux metadata */
			char		faux_metadata[256];
			sprintf(faux_metadata, HARD_DISK_METADATA_FORMAT, chd->header.obsolete_cylinders, chd->header.obsolete_heads, chd->header.obsolete_sectors, chd->header.hunkbytes / chd->header.obsolete_hunksize);

			/* fake it */
			metaentry.length = strlen(faux_metadata) + 1;
			if (outputlen > metaentry.length)
				outputlen = metaentry.length;
			memcpy(outputbuf, faux_metadata, outputlen);

			/* return the length of the data and the tag */
			*metatag = HARD_DISK_STANDARD_METADATA;
			last_error = CHDERR_NONE;
			return metaentry.length;
		}
		return 0;
	}

	/* clamp to the maximum requested size */
	if (outputlen > metaentry.length)
		outputlen = metaentry.length;

	/* read the metadata */
	count = multi_read(chd->file, metaentry.offset + METADATA_HEADER_SIZE, outputlen, outputbuf);
	if (count != outputlen)
		return count;

	/* return the length of the data and the tag */
	*metatag = metaentry.metatag;
	return metaentry.length;
}



/*************************************
 *
 *	Write metadata to a data file
 *
 *************************************/

int chd_set_metadata(struct chd_file *chd, UINT32 metatag, UINT32 metaindex, const void *inputbuf, UINT32 inputlen)
{
	UINT8 raw_meta_header[METADATA_HEADER_SIZE];
	struct metadata_entry metaentry;
	UINT32 count;

	/* if the disk is an old version, punt */
	if (chd->header.version < 3)
		return CHDERR_NOT_SUPPORTED;

	/* if the disk isn't writeable, punt */
	if (!(chd->header.flags & CHDFLAGS_IS_WRITEABLE))
		return CHDERR_FILE_NOT_WRITEABLE;

	/* must be at least 1 byte */
	if (inputlen < 1 || inputlen > CHD_MAX_METADATA_SIZE)
		return CHDERR_INVALID_METADATA_SIZE;

	/* if the entry fits within the previous entry, just overwrite it */
	last_error = (metaindex != CHD_METAINDEX_APPEND) ? find_metadata_entry(chd, metatag, metaindex, &metaentry) : CHDERR_METADATA_NOT_FOUND;
	if (last_error == CHDERR_NONE && inputlen <= metaentry.length)
	{
		count = multi_write(chd->file, metaentry.offset + METADATA_HEADER_SIZE, inputlen, inputbuf);
		if (count != inputlen)
			return last_error = CHDERR_WRITE_ERROR;

		/* if the lengths don't match, we need to update the length in our header */
		if (inputlen != metaentry.length)
		{
			count = multi_read(chd->file, metaentry.offset, sizeof(raw_meta_header), raw_meta_header);
			if (count != sizeof(raw_meta_header))
				return last_error = CHDERR_READ_ERROR;

			put_bigendian_uint32(&raw_meta_header[4], inputlen);
			count = multi_write(chd->file, metaentry.offset, sizeof(raw_meta_header), raw_meta_header);
			if (count != sizeof(raw_meta_header))
				return last_error = CHDERR_WRITE_ERROR;
		}
	}

	/* otherwise, we need to append an entry */
	else
	{
		/* if we already had an entry, unlink it */
		if (last_error == CHDERR_NONE)
		{
			/* if we were the first entry, make the next entry the first */
			if (metaentry.prev == 0)
			{
				chd->header.metaoffset = metaentry.next;
				last_error = write_header(chd->file, &chd->header);
				if (last_error != CHDERR_NONE)
					return last_error;
			}

			/* otherwise, update the link in the previous pointer */
			else
			{
				count = multi_read(chd->file, metaentry.prev, sizeof(raw_meta_header), raw_meta_header);
				if (count != sizeof(raw_meta_header))
					return last_error = CHDERR_READ_ERROR;

				put_bigendian_uint64(&raw_meta_header[8], metaentry.next);
				count = multi_write(chd->file, metaentry.prev, sizeof(raw_meta_header), raw_meta_header);
				if (count != sizeof(raw_meta_header))
					return last_error = CHDERR_WRITE_ERROR;
			}
		}

		/* now build us a new entry */
		put_bigendian_uint32(&raw_meta_header[0], metatag);
		put_bigendian_uint32(&raw_meta_header[4], inputlen);
		put_bigendian_uint64(&raw_meta_header[8], chd->header.metaoffset);

		/* write out the new header */
		metaentry.offset = multi_length(chd->file);
		count = multi_write(chd->file, metaentry.offset, sizeof(raw_meta_header), raw_meta_header);
		if (count != sizeof(raw_meta_header))
			return last_error = CHDERR_WRITE_ERROR;

		/* follow that with the data */
		count = multi_write(chd->file, metaentry.offset + METADATA_HEADER_SIZE, inputlen, inputbuf);
		if (count != inputlen)
			return last_error = CHDERR_WRITE_ERROR;

		/* finally, update the header */
		chd->header.metaoffset = metaentry.offset;
		last_error = write_header(chd->file, &chd->header);
		if (last_error != CHDERR_NONE)
			return last_error;
	}

	return CHDERR_NONE;
}



/*************************************
 *
 *	Reading from a data file
 *
 *************************************/

UINT32 chd_read(struct chd_file *chd, UINT32 hunknum, UINT32 hunkcount, void *buffer)
{
	int err;

	last_error = CHDERR_NONE;

	/* for now, just break down multihunk reads into single hunks */
	if (hunkcount > 1)
	{
		UINT32 total = 0;
		while (hunkcount-- && last_error == CHDERR_NONE)
			total += chd_read(chd, hunknum++, 1, (UINT8 *)buffer + total * chd->header.hunkbytes);
		return total;
	}

	/* punt if NULL or invalid */
	if (!chd || chd->cookie != COOKIE_VALUE)
		SET_ERROR_AND_CLEANUP(CHDERR_INVALID_PARAMETER);

	/* if we're past the end, fail */
	if (hunknum >= chd->header.totalhunks)
		SET_ERROR_AND_CLEANUP(CHDERR_HUNK_OUT_OF_RANGE);

	/* track the max */
	if (hunknum > chd->maxhunk)
		chd->maxhunk = hunknum;

	/* if the hunk is not cached, load and decompress it */
	if (chd->cachehunk != hunknum)
	{
		err = read_hunk_into_cache(chd, hunknum);
		if (err != CHDERR_NONE)
			SET_ERROR_AND_CLEANUP(err);
	}

	/* now copy the data from the cache */
	memcpy(buffer, chd->cache, chd->header.hunkbytes);
	return 1;

cleanup:
	return 0;
}



/*************************************
 *
 *	Writing to a data file
 *
 *************************************/

UINT32 chd_write(struct chd_file *chd, UINT32 hunknum, UINT32 hunkcount, const void *buffer)
{
	int err;

	last_error = CHDERR_NONE;

	/* for now, just break down multihunk writes into single hunks */
	if (hunkcount > 1)
	{
		UINT32 total = 0;
		while (hunkcount-- && last_error == CHDERR_NONE)
			total += chd_write(chd, hunknum++, 1, (const UINT8 *)buffer + total * chd->header.hunkbytes);
		return total;
	}

	/* punt if NULL or invalid */
	if (!chd || chd->cookie != COOKIE_VALUE)
		SET_ERROR_AND_CLEANUP(CHDERR_INVALID_PARAMETER);

	/* if we're past the end, fail */
	if (hunknum >= chd->header.totalhunks)
		SET_ERROR_AND_CLEANUP(CHDERR_HUNK_OUT_OF_RANGE);

	/* track the max */
	if (hunknum > chd->maxhunk)
		chd->maxhunk = hunknum;

	/* then write out the hunk */
	err = write_hunk_from_memory(chd, hunknum, buffer);
	if (err != CHDERR_NONE)
		SET_ERROR_AND_CLEANUP(err);
	return 1;

cleanup:
	return 0;
}



/*************************************
 *
 *	Return last error
 *
 *************************************/

int chd_get_last_error(void)
{
	return last_error;
}



/*************************************
 *
 *	Return pointer to header
 *
 *************************************/

const struct chd_header *chd_get_header(struct chd_file *chd)
{
	/* punt if NULL or invalid */
	if (!chd || chd->cookie != COOKIE_VALUE)
		SET_ERROR_AND_CLEANUP(CHDERR_INVALID_PARAMETER);

	return &chd->header;

cleanup:
	return NULL;
}



/*************************************
 *
 *	Set the header
 *
 *************************************/

int chd_set_header(const char *filename, const struct chd_header *header)
{
	struct chd_interface_file *file = NULL;
	struct chd_header oldheader;
	int err;

	/* punt if no interface */
	if (!interface.open)
		SET_ERROR_AND_CLEANUP(CHDERR_NO_INTERFACE);

	/* punt if NULL or invalid */
	if (!filename || !header)
		SET_ERROR_AND_CLEANUP(CHDERR_INVALID_PARAMETER);

	/* validate the header */
	err = validate_header(header);
	if (err != CHDERR_NONE)
		SET_ERROR_AND_CLEANUP(err);

	/* attempt to open the file */
	file = multi_open(filename, "rb+");
	if (!file)
		SET_ERROR_AND_CLEANUP(CHDERR_FILE_NOT_FOUND);

	/* read the old header */
	err = read_header(file, &oldheader);
	if (err != CHDERR_NONE)
		SET_ERROR_AND_CLEANUP(err);

	/* make sure we're only making valid changes */
	if (header->length != oldheader.length)
		SET_ERROR_AND_CLEANUP(CHDERR_INVALID_PARAMETER);
	if (header->version != oldheader.version)
		SET_ERROR_AND_CLEANUP(CHDERR_INVALID_PARAMETER);
	if (header->compression != oldheader.compression)
		SET_ERROR_AND_CLEANUP(CHDERR_INVALID_PARAMETER);
	if (header->hunkbytes != oldheader.hunkbytes)
		SET_ERROR_AND_CLEANUP(CHDERR_INVALID_PARAMETER);
	if (header->totalhunks != oldheader.totalhunks)
		SET_ERROR_AND_CLEANUP(CHDERR_INVALID_PARAMETER);
	if (header->metaoffset != oldheader.metaoffset)
		SET_ERROR_AND_CLEANUP(CHDERR_INVALID_PARAMETER);
	if (header->obsolete_hunksize != oldheader.obsolete_hunksize)
		SET_ERROR_AND_CLEANUP(CHDERR_INVALID_PARAMETER);

	/* write the new header */
	err = write_header(file, header);
	if (err != CHDERR_NONE)
		SET_ERROR_AND_CLEANUP(err);

	/* close the file and return */
	multi_close(file);
	return CHDERR_NONE;

cleanup:
	if (file)
		multi_close(file);
	return last_error;
}



/*************************************
 *
 *	All-in-one file compressor
 *
 *************************************/

int chd_compress(struct chd_file *chd, const char *rawfile, UINT32 offset, void (*progress)(const char *, ...))
{
	struct chd_interface_file *sourcefile = NULL;
	UINT64 sourceoffset = 0;
	MD5_CTX md5;
	struct sha1_ctx sha;
	clock_t lastupdate;
	int err, hunknum;

	/* punt if no interface */
	if (!interface.open)
		SET_ERROR_AND_CLEANUP(CHDERR_NO_INTERFACE);

	/* verify parameters */
	if (!chd || !rawfile)
		SET_ERROR_AND_CLEANUP(CHDERR_INVALID_PARAMETER);

	/* open the raw file */
	sourcefile = multi_open(rawfile, "rb");
	if (!sourcefile)
		SET_ERROR_AND_CLEANUP(CHDERR_FILE_NOT_FOUND);

	/* mark the CHD writeable and write the updated header */
	chd->header.flags |= CHDFLAGS_IS_WRITEABLE;
	err = write_header(chd->file, &chd->header);
	if (err != CHDERR_NONE)
		SET_ERROR_AND_CLEANUP(err);

	/* create CRC maps for the new CHD and the parent */
	init_crcmap(chd, 0);
	if (chd->parent)
		init_crcmap(chd->parent, 1);

	/* init the MD5/SHA1 computations */
	MD5_Init(&md5);
	sha1_init(&sha);

	/* loop over source hunks until we run out */
	lastupdate = 0;
	for (hunknum = 0; hunknum < chd->header.totalhunks; hunknum++)
	{
		clock_t curtime = clock();
		UINT32 bytestochecksum;
		UINT32 bytesread;

		/* read the data */
		bytesread = multi_read(sourcefile, sourceoffset + offset, chd->header.hunkbytes, chd->cache);
		if (bytesread < chd->header.hunkbytes)
			memset(&chd->cache[bytesread], 0, chd->header.hunkbytes - bytesread);

		/* progress */
		if (curtime - lastupdate > CLOCKS_PER_SEC / 2)
		{
			UINT64 sourcepos = (UINT64)hunknum * chd->header.hunkbytes;
			if (progress && sourcepos)
				(*progress)("Compressing hunk %d/%d... (ratio=%d%%)  \r", hunknum, chd->header.totalhunks, 100 - multi_length(chd->file) * 100 / sourcepos);
			lastupdate = curtime;
		}

		/* update the MD5/SHA1 */
		bytestochecksum = chd->header.hunkbytes;
		if (sourceoffset + chd->header.hunkbytes > chd->header.logicalbytes)
		{
			if (sourceoffset >= chd->header.logicalbytes)
				bytestochecksum = 0;
			else
				bytestochecksum = chd->header.logicalbytes - sourceoffset;
		}
		if (bytestochecksum)
		{
			MD5_Update(&md5, chd->cache, bytestochecksum);
			sha1_update(&sha, bytestochecksum, chd->cache);
		}

		/* write out the hunk */
		err = write_hunk_from_memory(chd, hunknum, chd->cache);
		if (err != CHDERR_NONE)
			SET_ERROR_AND_CLEANUP(err);

		/* update our CRC map */
		if ((chd->map[hunknum].flags & MAP_ENTRY_FLAG_TYPE_MASK) != MAP_ENTRY_TYPE_SELF_HUNK &&
			(chd->map[hunknum].flags & MAP_ENTRY_FLAG_TYPE_MASK) != MAP_ENTRY_TYPE_PARENT_HUNK)
			add_to_crcmap(chd, hunknum);

		/* prepare for the next hunk */
		sourceoffset += chd->header.hunkbytes;
	}

	/* compute the final MD5/SHA1 values */
	MD5_Final(chd->header.md5, &md5);
	sha1_final(&sha);
	sha1_digest(&sha, SHA1_DIGEST_SIZE, chd->header.sha1);

	/* turn off the writeable flag and re-write the header */
	chd->header.flags &= ~CHDFLAGS_IS_WRITEABLE;
	err = write_header(chd->file, &chd->header);
	if (err != CHDERR_NONE)
		SET_ERROR_AND_CLEANUP(err);

	/* final progress update */
	if (progress)
	{
		UINT64 sourcepos = (UINT64)hunknum * chd->header.hunkbytes;
		if (sourcepos)
			(*progress)("Compression complete ... final ratio = %d%%            \n", 100 - multi_length(chd->file) * 100 / sourcepos);
	}

	/* close the file */
	multi_close(sourcefile);
	return CHDERR_NONE;

cleanup:
	if (sourcefile)
		multi_close(sourcefile);
	return last_error;
}



/*************************************
 *
 *	All-in-one file verifier
 *
 *************************************/

int chd_verify(struct chd_file *chd, void (*progress)(const char *, ...), UINT8 actualmd5[CHD_MD5_BYTES], UINT8 actualsha1[CHD_SHA1_BYTES])
{
	MD5_CTX md5;
	struct sha1_ctx sha;
	UINT64 sourceoffset = 0;
	int err, hunknum = 0;
	clock_t lastupdate;

	/* punt if no interface */
	if (!interface.open)
		SET_ERROR_AND_CLEANUP(CHDERR_NO_INTERFACE);

	/* verify parameters */
	if (!chd)
		SET_ERROR_AND_CLEANUP(CHDERR_INVALID_PARAMETER);

	/* if this is a writeable file image, we can't verify */
	if (chd->header.flags & CHDFLAGS_IS_WRITEABLE)
		SET_ERROR_AND_CLEANUP(CHDERR_CANT_VERIFY);

	/* init the MD5/SHA1 computations */
	MD5_Init(&md5);
	sha1_init(&sha);

	/* loop over source hunks until we run out */
	lastupdate = 0;
	for (hunknum = 0; hunknum < chd->header.totalhunks; hunknum++)
	{
		clock_t curtime = clock();
		UINT32 bytestochecksum;

		/* progress */
		if (curtime - lastupdate > CLOCKS_PER_SEC / 2)
		{
			if (progress)
				(*progress)("Verifying hunk %d/%d...\r", hunknum, chd->header.totalhunks);
			lastupdate = curtime;
		}

		/* read the hunk into the cache */
		err = read_hunk_into_cache(chd, hunknum);
		if (err != CHDERR_NONE)
			SET_ERROR_AND_CLEANUP(err);

		/* update the MD5/SHA1 */
		bytestochecksum = chd->header.hunkbytes;
		if (sourceoffset + chd->header.hunkbytes > chd->header.logicalbytes)
		{
			if (sourceoffset >= chd->header.logicalbytes)
				bytestochecksum = 0;
			else
				bytestochecksum = chd->header.logicalbytes - sourceoffset;
		}
		if (bytestochecksum)
		{
			MD5_Update(&md5, chd->cache, bytestochecksum);
			sha1_update(&sha, bytestochecksum, chd->cache);
		}

		/* prepare for the next hunk */
		sourceoffset += chd->header.hunkbytes;
	}

	/* compute the final MD5 */
	MD5_Final(actualmd5, &md5);
	sha1_final(&sha);
	sha1_digest(&sha, SHA1_DIGEST_SIZE, actualsha1);

	/* final progress update */
	if (progress)
		(*progress)("Verification complete                                  \n");
	return CHDERR_NONE;

cleanup:
	return last_error;
}



/*************************************
 *
 *	Validate header data
 *
 *************************************/

static int validate_header(const struct chd_header *header)
{
	/* require a valid version */
	if (header->version == 0 || header->version > CHD_HEADER_VERSION)
		return CHDERR_UNSUPPORTED_VERSION;

	/* require a valid length */
	if ((header->version == 1 && header->length != CHD_V1_HEADER_SIZE) ||
		(header->version == 2 && header->length != CHD_V2_HEADER_SIZE) ||
		(header->version == 3 && header->length != CHD_V3_HEADER_SIZE))
		return CHDERR_INVALID_PARAMETER;

	/* require valid flags */
	if (header->flags & CHDFLAGS_UNDEFINED)
		return CHDERR_INVALID_PARAMETER;

	/* require a valid compression mechanism */
	if (header->compression >= CHDCOMPRESSION_MAX)
		return CHDERR_INVALID_PARAMETER;

	/* require a valid hunksize */
	if (header->hunkbytes == 0 || header->hunkbytes >= 65536)
		return CHDERR_INVALID_PARAMETER;

	/* require a valid hunk count */
	if (header->totalhunks == 0)
		return CHDERR_INVALID_PARAMETER;

	/* require a valid MD5 and/or SHA1 if we're using a parent */
	if ((header->flags & CHDFLAGS_HAS_PARENT) && !memcmp(header->parentmd5, nullmd5, sizeof(nullmd5)) && !memcmp(header->parentsha1, nullsha1, sizeof(nullsha1)))
		return CHDERR_INVALID_PARAMETER;

	/* if we're V3 or later, the obsolete fields must be 0 */
	if (header->version >= 3 &&
		(header->obsolete_cylinders != 0 || header->obsolete_sectors != 0 ||
		 header->obsolete_heads != 0 || header->obsolete_hunksize != 0))
		return CHDERR_INVALID_PARAMETER;

	/* if we're pre-V3, the obsolete fields must NOT be 0 */
	if (header->version < 3 &&
		(header->obsolete_cylinders == 0 || header->obsolete_sectors == 0 ||
		 header->obsolete_heads == 0 || header->obsolete_hunksize == 0))
		return CHDERR_INVALID_PARAMETER;

	return CHDERR_NONE;
}




/*************************************
 *
 *	Hunk read/decompress
 *
 *************************************/

static int read_hunk_into_memory(struct chd_file *chd, UINT32 hunknum, UINT8 *dest)
{
	struct map_entry *entry = &chd->map[hunknum];
	UINT32 bytes;
	int err;

	/* switch off the entry type */
	switch (entry->flags & MAP_ENTRY_FLAG_TYPE_MASK)
	{
		/* compressed data */
		case MAP_ENTRY_TYPE_COMPRESSED:

			/* read it into the decompression buffer */
			bytes = multi_read(chd->file, entry->offset, entry->length, chd->compressed);
			if (bytes != entry->length)
				return CHDERR_READ_ERROR;

			/* now decompress based on the compression method */
			switch (chd->header.compression)
			{
				case CHDCOMPRESSION_ZLIB:
				case CHDCOMPRESSION_ZLIB_PLUS:
				{
					struct zlib_codec_data *codec = chd->codecdata;

					/* reset the decompressor */
					codec->inflater.next_in = chd->compressed;
					codec->inflater.avail_in = entry->length;
					codec->inflater.total_in = 0;
					codec->inflater.next_out = dest;
					codec->inflater.avail_out = chd->header.hunkbytes;
					codec->inflater.total_out = 0;
					err = inflateReset(&codec->inflater);
					if (err != Z_OK)
						return CHDERR_DECOMPRESSION_ERROR;

					/* do it */
					err = inflate(&codec->inflater, Z_FINISH);
					if (codec->inflater.total_out != chd->header.hunkbytes)
						return CHDERR_DECOMPRESSION_ERROR;
					break;
				}
			}
			break;

		/* uncompressed data */
		case MAP_ENTRY_TYPE_UNCOMPRESSED:
			bytes = multi_read(chd->file, entry->offset, chd->header.hunkbytes, dest);
			if (bytes != chd->header.hunkbytes)
				return CHDERR_READ_ERROR;
			break;

		/* mini-compressed data */
		case MAP_ENTRY_TYPE_MINI:
			put_bigendian_uint64(&dest[0], entry->offset);
			for (bytes = 8; bytes < chd->header.hunkbytes; bytes++)
				dest[bytes] = dest[bytes - 8];
			break;

		/* self-referenced data */
		case MAP_ENTRY_TYPE_SELF_HUNK:
			if (chd->cachehunk == entry->offset && dest == chd->cache)
				break;
			return read_hunk_into_memory(chd, entry->offset, dest);

		/* parent-referenced data */
		case MAP_ENTRY_TYPE_PARENT_HUNK:
			err = read_hunk_into_memory(chd->parent, entry->offset, dest);
			if (err != CHDERR_NONE)
				return err;
			break;
	}

	/* validate the CRC if we have one */
	if (!(entry->flags & MAP_ENTRY_FLAG_NO_CRC) && entry->crc != crc32(0, &dest[0], chd->header.hunkbytes))
		return CHDERR_DECOMPRESSION_ERROR;
	return CHDERR_NONE;
}


static int read_hunk_into_cache(struct chd_file *chd, UINT32 hunknum)
{
	int err;

	/* if we're already in the cache, we're done */
	if (chd->cachehunk == hunknum)
		return CHDERR_NONE;
	chd->cachehunk = ~0;

	/* otherwise, read the data */
	err = read_hunk_into_memory(chd, hunknum, chd->cache);
	if (err != CHDERR_NONE)
		return err;

	/* mark the hunk successfully cached in */
	chd->cachehunk = hunknum;
	return CHDERR_NONE;
}



/*************************************
 *
 *	Hunk write/compress
 *
 *************************************/

static int write_hunk_from_memory(struct chd_file *chd, UINT32 hunknum, const UINT8 *src)
{
	struct map_entry *entry = &chd->map[hunknum];
	struct map_entry newentry;
	UINT8 fileentry[MAP_ENTRY_SIZE];
	const void *data = src;
	UINT32 bytes, match;

	/* first compute the CRC */
	newentry.crc = crc32(0, &src[0], chd->header.hunkbytes);

	/* some extra stuff for zlib+ compression */
	if (chd->header.compression == CHDCOMPRESSION_ZLIB_PLUS)
	{
		/* see if we can mini-compress first */
		for (bytes = 8; bytes < chd->header.hunkbytes; bytes++)
			if (src[bytes] != src[bytes - 8])
				break;

		/* if so, we don't need to write any data */
		if (bytes == chd->header.hunkbytes)
		{
			newentry.offset = get_bigendian_uint64(&src[0]);
			newentry.length = 0;
			newentry.flags = MAP_ENTRY_TYPE_MINI;
			goto write_entry;
		}

		/* otherwise, see if we can find a match in the current file */
		match = find_matching_hunk(chd, hunknum, newentry.crc, &src[0]);
		if (match != NO_MATCH)
		{
			newentry.offset = match;
			newentry.length = 0;
			newentry.flags = MAP_ENTRY_TYPE_SELF_HUNK;
			goto write_entry;
		}

		/* if we have a parent, see if we can find a match in there */
		if (chd->header.flags & CHDFLAGS_HAS_PARENT)
		{
			match = find_matching_hunk(chd->parent, ~0, newentry.crc, &src[0]);
			if (match != NO_MATCH)
			{
				newentry.offset = match;
				newentry.length = 0;
				newentry.flags = MAP_ENTRY_TYPE_PARENT_HUNK;
				goto write_entry;
			}
		}
	}

	/* if we get here, we need to compress the data */
	/* first, fill in an uncompressed entry */
	newentry.length = chd->header.hunkbytes;
	newentry.flags = MAP_ENTRY_TYPE_UNCOMPRESSED;

	/* now try compressing the data */
	switch (chd->header.compression)
	{
		case CHDCOMPRESSION_ZLIB:
		case CHDCOMPRESSION_ZLIB_PLUS:
		{
			struct zlib_codec_data *codec = chd->codecdata;
			int err;

			/* reset the decompressor */
			codec->deflater.next_in = (void *)src;
			codec->deflater.avail_in = chd->header.hunkbytes;
			codec->deflater.total_in = 0;
			codec->deflater.next_out = chd->compressed;
			codec->deflater.avail_out = chd->header.hunkbytes;
			codec->deflater.total_out = 0;
			err = deflateReset(&codec->deflater);
			if (err != Z_OK)
				return CHDERR_COMPRESSION_ERROR;

			/* do it */
			err = deflate(&codec->deflater, Z_FINISH);

			/* if we didn't run out of space, override the raw data with compressed */
			if (err == Z_STREAM_END && codec->deflater.total_out < newentry.length)
			{
				data = chd->compressed;
				newentry.length = codec->deflater.total_out;
				newentry.flags = MAP_ENTRY_TYPE_COMPRESSED;
			}
			break;
		}
	}

	/* if the data doesn't fit into the previous entry, make a new one at the eof */
	newentry.offset = entry->offset;
	if (newentry.offset == 0 || newentry.length > entry->length)
		newentry.offset = multi_length(chd->file);

	/* write the data */
	bytes = multi_write(chd->file, newentry.offset, newentry.length, data);
	if (bytes != newentry.length)
		return CHDERR_WRITE_ERROR;

	/* update the entry in memory */
write_entry:
	*entry = newentry;

	/* update the map on file */
	assemble_map_entry(&fileentry[0], &chd->map[hunknum]);
	bytes = multi_write(chd->file, chd->header.length + hunknum * sizeof(fileentry), sizeof(fileentry), &fileentry[0]);
	if (bytes != sizeof(fileentry))
		return CHDERR_WRITE_ERROR;

	return CHDERR_NONE;
}



/*************************************
 *
 *	Header read
 *
 *************************************/

static int read_header(struct chd_interface_file *file, struct chd_header *header)
{
	UINT8 rawheader[CHD_MAX_HEADER_SIZE];
	UINT32 count;

	/* punt if NULL */
	if (!header)
		return CHDERR_INVALID_PARAMETER;

	/* punt if invalid file */
	if (!file)
		return CHDERR_INVALID_FILE;

	/* punt if no interface */
	if (!interface.read)
		return CHDERR_NO_INTERFACE;

	/* seek and read */
	count = multi_read(file, 0, sizeof(rawheader), rawheader);
	if (count != sizeof(rawheader))
		return CHDERR_READ_ERROR;

	/* verify the tag */
	if (strncmp((char *)rawheader, "MComprHD", 8) != 0)
		return CHDERR_INVALID_DATA;

	/* extract the direct data */
	memset(header, 0, sizeof(*header));
	header->length        = get_bigendian_uint32(&rawheader[8]);
	header->version       = get_bigendian_uint32(&rawheader[12]);

	/* make sure it's a version we understand */
	if (header->version == 0 || header->version > CHD_HEADER_VERSION)
		return CHDERR_UNSUPPORTED_VERSION;

	/* make sure the length is expected */
	if ((header->version == 1 && header->length != CHD_V1_HEADER_SIZE) ||
		(header->version == 2 && header->length != CHD_V2_HEADER_SIZE) ||
		(header->version == 3 && header->length != CHD_V3_HEADER_SIZE))
		return CHDERR_INVALID_DATA;

	/* extract the common data */
	header->flags         = get_bigendian_uint32(&rawheader[16]);
	header->compression   = get_bigendian_uint32(&rawheader[20]);
	memcpy(header->md5, &rawheader[44], CHD_MD5_BYTES);
	memcpy(header->parentmd5, &rawheader[60], CHD_MD5_BYTES);

	/* extract the V1/V2-specific data */
	if (header->version < 3)
	{
		int seclen = (header->version == 1) ? CHD_V1_SECTOR_SIZE : get_bigendian_uint32(&rawheader[76]);
		header->obsolete_hunksize  = get_bigendian_uint32(&rawheader[24]);
		header->totalhunks         = get_bigendian_uint32(&rawheader[28]);
		header->obsolete_cylinders = get_bigendian_uint32(&rawheader[32]);
		header->obsolete_heads     = get_bigendian_uint32(&rawheader[36]);
		header->obsolete_sectors   = get_bigendian_uint32(&rawheader[40]);
		header->logicalbytes = (UINT64)header->obsolete_cylinders * (UINT64)header->obsolete_heads * (UINT64)header->obsolete_sectors * (UINT64)seclen;
		header->hunkbytes = seclen * header->obsolete_hunksize;
		header->metaoffset = 0;
	}

	/* extract the V3-specific data */
	else
	{
		header->totalhunks   = get_bigendian_uint32(&rawheader[24]);
		header->logicalbytes = get_bigendian_uint64(&rawheader[28]);
		header->metaoffset   = get_bigendian_uint64(&rawheader[36]);
		header->hunkbytes    = get_bigendian_uint32(&rawheader[76]);
		memcpy(header->sha1, &rawheader[80], CHD_SHA1_BYTES);
		memcpy(header->parentsha1, &rawheader[100], CHD_SHA1_BYTES);
	}

	/* guess it worked */
	return CHDERR_NONE;
}



/*************************************
 *
 *	Header write
 *
 *************************************/

static int write_header(struct chd_interface_file *file, const struct chd_header *header)
{
	UINT8 rawheader[CHD_MAX_HEADER_SIZE];
	UINT32 count;

	/* punt if NULL */
	if (!header)
		return CHDERR_INVALID_PARAMETER;

	/* punt if invalid file */
	if (!file)
		return CHDERR_INVALID_FILE;

	/* punt if no interface */
	if (!interface.write)
		return CHDERR_NO_INTERFACE;

	/* only support writing modern headers */
	if (header->version != 3)
		return CHDERR_INVALID_PARAMETER;

	/* assemble the data */
	memset(rawheader, 0, sizeof(rawheader));
	memcpy(rawheader, "MComprHD", 8);

	put_bigendian_uint32(&rawheader[8],  CHD_V3_HEADER_SIZE);
	put_bigendian_uint32(&rawheader[12], header->version);
	put_bigendian_uint32(&rawheader[16], header->flags);
	put_bigendian_uint32(&rawheader[20], header->compression);
	put_bigendian_uint32(&rawheader[24], header->totalhunks);
	put_bigendian_uint64(&rawheader[28], header->logicalbytes);
	put_bigendian_uint64(&rawheader[36], header->metaoffset);
	memcpy(&rawheader[44], header->md5, CHD_MD5_BYTES);
	memcpy(&rawheader[60], header->parentmd5, CHD_MD5_BYTES);
	put_bigendian_uint32(&rawheader[76], header->hunkbytes);
	memcpy(&rawheader[80], header->sha1, CHD_SHA1_BYTES);
	memcpy(&rawheader[100], header->parentsha1, CHD_SHA1_BYTES);

	/* seek and write */
	count = multi_write(file, 0, CHD_V3_HEADER_SIZE, rawheader);
	if (count != CHD_V3_HEADER_SIZE)
		return CHDERR_WRITE_ERROR;

	return CHDERR_NONE;
}



/*************************************
 *
 *	Read the sector map
 *
 *************************************/

static int read_hunk_map(struct chd_file *chd)
{
	UINT32 entrysize = (chd->header.version < 3) ? OLD_MAP_ENTRY_SIZE : MAP_ENTRY_SIZE;
	UINT8 raw_map_entries[MAP_STACK_ENTRIES * MAP_ENTRY_SIZE];
	UINT8 cookie[MAP_ENTRY_SIZE];
	UINT64 fileoffset;
	UINT32 count;
	int i, err;

	/* first allocate memory */
	chd->map = malloc(sizeof(chd->map[0]) * chd->header.totalhunks);
	if (!chd->map)
		return CHDERR_OUT_OF_MEMORY;

	/* read the map entries in in chunks and extract to the map list */
	fileoffset = chd->header.length;
	for (i = 0; i < chd->header.totalhunks; i += MAP_STACK_ENTRIES)
	{
		/* compute how many entries this time */
		int entries = chd->header.totalhunks - i, j;
		if (entries > MAP_STACK_ENTRIES)
			entries = MAP_STACK_ENTRIES;

		/* read that many */
		count = multi_read(chd->file, fileoffset, entries * entrysize, raw_map_entries);
		if (count != entries * entrysize)
		{
			err = CHDERR_READ_ERROR;
			goto cleanup;
		}
		fileoffset += entries * entrysize;

		/* process that many */
		if (entrysize == MAP_ENTRY_SIZE)
		{
			for (j = 0; j < entries; j++)
				extract_map_entry(&raw_map_entries[j * MAP_ENTRY_SIZE], &chd->map[i + j]);
		}
		else
		{
			for (j = 0; j < entries; j++)
				extract_old_map_entry(&raw_map_entries[j * OLD_MAP_ENTRY_SIZE], &chd->map[i + j], chd->header.hunkbytes);
		}
	}

	/* verify the cookie */
	count = multi_read(chd->file, fileoffset, entrysize, &cookie);
	if (count != entrysize || memcmp(&cookie, END_OF_LIST_COOKIE, entrysize))
	{
		err = CHDERR_INVALID_FILE;
		goto cleanup;
	}
	return CHDERR_NONE;

cleanup:
	if (chd->map)
		free(chd->map);
	chd->map = NULL;
	return err;
}



/*************************************
 *
 *	CRC map initialization
 *
 *************************************/

static void init_crcmap(struct chd_file *chd, int prepopulate)
{
	int i;

	/* if we already have one, bail */
	if (chd->crcmap)
		return;

	/* reset all pointers */
	chd->crcmap = NULL;
	chd->crcfree = NULL;
	chd->crctable = NULL;

	/* allocate a list; one for each hunk */
	chd->crcmap = malloc(chd->header.totalhunks * sizeof(chd->crcmap[0]));
	if (!chd->crcmap)
		return;

	/* allocate a CRC map table */
	chd->crctable = malloc(CRCMAP_HASH_SIZE * sizeof(chd->crctable[0]));
	if (!chd->crctable)
	{
		free(chd->crcmap);
		chd->crcmap = NULL;
		return;
	}

	/* initialize the free list */
	for (i = 0; i < chd->header.totalhunks; i++)
	{
		chd->crcmap[i].next = chd->crcfree;
		chd->crcfree = &chd->crcmap[i];
	}

	/* initialize the table */
	memset(chd->crctable, 0, CRCMAP_HASH_SIZE * sizeof(chd->crctable[0]));

	/* if we're to prepopulate, go for it */
	if (prepopulate)
		for (i = 0; i < chd->header.totalhunks; i++)
			add_to_crcmap(chd, i);
}



/*************************************
 *
 *	CRC map addition
 *
 *************************************/

static void add_to_crcmap(struct chd_file *chd, UINT32 hunknum)
{
	UINT32 hash = chd->map[hunknum].crc % CRCMAP_HASH_SIZE;
	struct crcmap_entry *crcmap;

	/* pull a free entry off the list */
	crcmap = chd->crcfree;
	chd->crcfree = crcmap->next;

	/* set up the entry and link it into the hash table */
	crcmap->hunknum = hunknum;
	crcmap->next = chd->crctable[hash];
	chd->crctable[hash] = crcmap;
}



/*************************************
 *
 *	Matching hunk verifier
 *
 *************************************/

static int is_really_matching_hunk(struct chd_file *chd, UINT32 hunknum, const UINT8 *rawdata)
{
	/* we have a potential match -- better be sure */
	/* read the hunk from disk and compare byte-for-byte */
	if (hunknum != chd->comparehunk)
	{
		chd->comparehunk = ~0;
		if (read_hunk_into_memory(chd, hunknum, chd->compare) == CHDERR_NONE)
			chd->comparehunk = hunknum;
	}
	return (hunknum == chd->comparehunk && !memcmp(rawdata, chd->compare, chd->header.hunkbytes));
}



/*************************************
 *
 *	Matching hunk locater
 *
 *************************************/

static UINT32 find_matching_hunk(struct chd_file *chd, UINT32 hunknum, UINT32 crc, const UINT8 *rawdata)
{
	UINT32 lasthunk = (hunknum < chd->header.totalhunks) ? hunknum : chd->header.totalhunks;
	int curhunk;

	/* if we have a CRC map, use that */
	if (chd->crctable)
	{
		struct crcmap_entry *curentry;
		for (curentry = chd->crctable[crc % CRCMAP_HASH_SIZE]; curentry; curentry = curentry->next)
		{
			curhunk = curentry->hunknum;
			if (chd->map[curhunk].crc == crc && !(chd->map[curhunk].flags & MAP_ENTRY_FLAG_NO_CRC) && is_really_matching_hunk(chd, curhunk, rawdata))
				return curhunk;
		}
		return NO_MATCH;
	}

	/* first see if the last match is a valid one */
	if (chd->comparehunk < chd->header.totalhunks && chd->map[chd->comparehunk].crc == crc && !(chd->map[chd->comparehunk].flags & MAP_ENTRY_FLAG_NO_CRC) &&
		!memcmp(rawdata, chd->compare, chd->header.hunkbytes))
		return chd->comparehunk;

	/* scan through the CHD's hunk map looking for a match */
	for (curhunk = 0; curhunk < lasthunk; curhunk++)
		if (chd->map[curhunk].crc == crc && !(chd->map[curhunk].flags & MAP_ENTRY_FLAG_NO_CRC) && is_really_matching_hunk(chd, curhunk, rawdata))
			return curhunk;

	return NO_MATCH;
}



/*************************************
 *
 *	Internal metadata locator
 *
 *************************************/

static int find_metadata_entry(struct chd_file *chd, UINT32 metatag, UINT32 metaindex, struct metadata_entry *metaentry)
{
	/* start at the beginning */
	metaentry->offset = chd->header.metaoffset;
	metaentry->prev = 0;

	/* loop until we run out of options */
	while (metaentry->offset != 0)
	{
		UINT8	raw_meta_header[METADATA_HEADER_SIZE];
		UINT32	count;

		/* read the raw header */
		count = multi_read(chd->file, metaentry->offset, sizeof(raw_meta_header), raw_meta_header);
		if (count != sizeof(raw_meta_header))
			break;

		/* extract the data */
		metaentry->metatag = get_bigendian_uint32(&raw_meta_header[0]);
		metaentry->length = get_bigendian_uint32(&raw_meta_header[4]);
		metaentry->next = get_bigendian_uint64(&raw_meta_header[8]);

		/* if we got a match, proceed */
		if (metatag == CHDMETATAG_WILDCARD || metaentry->metatag == metatag)
			if (metaindex-- == 0)
				return CHDERR_NONE;

		/* no match, fetch the next link */
		metaentry->prev = metaentry->offset;
		metaentry->offset = metaentry->next;
	}

	/* if we get here, we didn't find it */
	return CHDERR_METADATA_NOT_FOUND;
}



/*************************************
 *
 *	ZLIB memory hooks
 *
 *************************************/

/*
	Because ZLIB allocates and frees memory frequently (once per compression cycle),
	we don't call malloc/free, but instead keep track of our own memory.
*/

static voidpf fast_alloc(voidpf opaque, uInt items, uInt size)
{
	struct zlib_codec_data *data = opaque;
	UINT32 *ptr;
	int i;

	/* compute the size, rounding to the nearest 1k */
	size = (size * items + 0x3ff) & ~0x3ff;

	/* reuse a hunk if we can */
	for (i = 0; i < MAX_ZLIB_ALLOCS; i++)
	{
		ptr = data->allocptr[i];
		if (ptr && size == *ptr)
		{
			/* set the low bit of the size so we don't match next time */
			*ptr |= 1;
			return ptr + 1;
		}
	}

	/* alloc a new one */
	ptr = malloc(size + sizeof(UINT32));
	if (!ptr)
		return NULL;

	/* put it into the list */
	for (i = 0; i < MAX_ZLIB_ALLOCS; i++)
		if (!data->allocptr[i])
		{
			data->allocptr[i] = ptr;
			break;
		}

	/* set the low bit of the size so we don't match next time */
	*ptr = size | 1;
	return ptr + 1;
}


static void fast_free(voidpf opaque, voidpf address)
{
	struct zlib_codec_data *data = opaque;
	UINT32 *ptr = (UINT32 *)address - 1;
	int i;

	/* find the hunk */
	for (i = 0; i < MAX_ZLIB_ALLOCS; i++)
		if (ptr == data->allocptr[i])
		{
			/* clear the low bit of the size to allow matches */
			*ptr &= ~1;
			return;
		}
}



/*************************************
 *
 *	Compression init
 *
 *************************************/

static int init_codec(struct chd_file *chd)
{
	int err = CHDERR_NONE;

	/* now decompress based on the compression method */
	switch (chd->header.compression)
	{
		case CHDCOMPRESSION_NONE:
			/* nothing to do */
			break;

		case CHDCOMPRESSION_ZLIB:
		case CHDCOMPRESSION_ZLIB_PLUS:
		{
			struct zlib_codec_data *data;

			/* allocate memory for the 2 stream buffers */
			chd->codecdata = malloc(sizeof(struct zlib_codec_data));
			if (!chd->codecdata)
				return CHDERR_OUT_OF_MEMORY;

			/* clear the buffers */
			data = chd->codecdata;
			memset(data, 0, sizeof(struct zlib_codec_data));

			/* init the first for decompression and the second for compression */
			data->inflater.next_in = chd->compressed;
			data->inflater.avail_in = 0;
			data->inflater.zalloc = fast_alloc;
			data->inflater.zfree = fast_free;
			data->inflater.opaque = data;
			err = inflateInit2(&data->inflater, -MAX_WBITS);
			if (err == Z_OK)
			{
				data->deflater.next_in = chd->compressed;
				data->deflater.avail_in = 0;
				data->deflater.zalloc = fast_alloc;
				data->deflater.zfree = fast_free;
				data->deflater.opaque = data;
				err = deflateInit2(&data->deflater, Z_BEST_COMPRESSION, Z_DEFLATED, -MAX_WBITS, 8, Z_DEFAULT_STRATEGY);
			}

			/* convert errors */
			if (err == Z_MEM_ERROR)
				err = CHDERR_OUT_OF_MEMORY;
			else if (err != Z_OK)
				err = CHDERR_CODEC_ERROR;
			else
				err = CHDERR_NONE;

			/* handle an error */
			if (err != CHDERR_NONE)
				free(chd->codecdata);
			break;
		}
	}

	/* return the error */
	return err;
}



/*************************************
 *
 *	Compression de-init
 *
 *************************************/

static void free_codec(struct chd_file *chd)
{
	/* now decompress based on the compression method */
	switch (chd->header.compression)
	{
		case CHDCOMPRESSION_NONE:
			/* nothing to do */
			break;

		case CHDCOMPRESSION_ZLIB:
		case CHDCOMPRESSION_ZLIB_PLUS:
		{
			struct zlib_codec_data *data = chd->codecdata;

			/* deinit the streams */
			if (data)
			{
				int i;

				inflateEnd(&data->inflater);
				deflateEnd(&data->deflater);

				/* free our fast memory */
				for (i = 0; i < MAX_ZLIB_ALLOCS; i++)
					if (data->allocptr[i])
						free(data->allocptr[i]);
				free(data);
			}
			break;
		}
	}
}



/*************************************
 *
 *	Multifile routines
 *
 *************************************/

static struct chd_interface_file *multi_open(const char *filename, const char *mode)
{
	return (*interface.open)(filename, mode);
}

static void multi_close(struct chd_interface_file *file)
{
	(*interface.close)(file);
}

static UINT32 multi_read(struct chd_interface_file *file, UINT64 offset, UINT32 count, void *buffer)
{
	return (*interface.read)(file, offset, count, buffer);
}

static UINT32 multi_write(struct chd_interface_file *file, UINT64 offset, UINT32 count, const void *buffer)
{
	return (*interface.write)(file, offset, count, buffer);
}

static UINT64 multi_length(struct chd_interface_file *file)
{
	return (*interface.length)(file);
}
