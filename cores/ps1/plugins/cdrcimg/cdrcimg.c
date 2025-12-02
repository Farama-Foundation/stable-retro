/*
 * (C) Gražvydas "notaz" Ignotas, 2010
 *
 * This work is licensed under the terms of any of these licenses
 * (at your option):
 *  - GNU GPL, version 2 or later.
 *  - GNU LGPL, version 2.1 or later.
 * See the COPYING file in the top-level directory.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <zlib.h>
#if !defined(_WIN32) && !defined(NO_DYLIB)
#include <dlfcn.h>
#endif

#include "cdrcimg.h"

#undef CALLBACK
#define CALLBACK

#define PFX "cdrcimg: "
#define err(f, ...) fprintf(stderr, PFX f, ##__VA_ARGS__)

#define CD_FRAMESIZE_RAW 2352

enum {
	CDRC_ZLIB,
	CDRC_ZLIB2,
	CDRC_BZ,
};

static const char *cd_fname;
static unsigned int *cd_index_table;
static unsigned int  cd_index_len;
static unsigned int  cd_sectors_per_blk;
static int cd_compression;
static FILE *cd_file;

static int (*pBZ2_bzBuffToBuffDecompress)(char *dest, unsigned int *destLen, char *source,
		unsigned int sourceLen, int small, int verbosity);

static struct {
	unsigned char raw[16][CD_FRAMESIZE_RAW];
	unsigned char compressed[CD_FRAMESIZE_RAW * 16 + 100];
} *cdbuffer;
static int current_block, current_sect_in_blk;

struct CdrStat;
extern long CALLBACK CDR__getStatus(struct CdrStat *stat);

struct CdrStat
{
	unsigned int Type;
	unsigned int Status;
	unsigned char Time[3]; // current playing time
};

struct trackinfo {
	enum {DATA, CDDA} type;
	char start[3];		// MSF-format
	char length[3];		// MSF-format
};

#define MAXTRACKS 100 /* How many tracks can a CD hold? */

static int numtracks = 0;

#define btoi(b)           ((b) / 16 * 10 + (b) % 16) /* BCD to u_char */
#define MSF2SECT(m, s, f) (((m) * 60 + (s) - 2) * 75 + (f))

// return Starting and Ending Track
// buffer:
//  byte 0 - start track
//  byte 1 - end track
static long CDRgetTN(unsigned char *buffer)
{
	buffer[0] = 1;
	buffer[1] = numtracks > 0 ? numtracks : 1;

	return 0;
}

// return Track Time
// buffer:
//  byte 0 - frame
//  byte 1 - second
//  byte 2 - minute
static long CDRgetTD(unsigned char track, unsigned char *buffer)
{
	buffer[2] = 0;
	buffer[1] = 2;
	buffer[0] = 0;

	return 0;
}

static int uncompress2_pcsx(void *out, unsigned long *out_size, void *in, unsigned long in_size)
{
	static z_stream z;
	int ret = 0;

	if (z.zalloc == NULL) {
		// XXX: one-time leak here..
		z.next_in = Z_NULL;
		z.avail_in = 0;
		z.zalloc = Z_NULL;
		z.zfree = Z_NULL;
		z.opaque = Z_NULL;
		ret = inflateInit2(&z, -15);
	}
	else
		ret = inflateReset(&z);
	if (ret != Z_OK)
		return ret;

	z.next_in = in;
	z.avail_in = in_size;
	z.next_out = out;
	z.avail_out = *out_size;

	ret = inflate(&z, Z_NO_FLUSH);
	//inflateEnd(&z);

	*out_size -= z.avail_out;
	return ret == 1 ? 0 : ret;
}

// read track
// time: byte 0 - minute; byte 1 - second; byte 2 - frame
// uses bcd format
static long CDRreadTrack(unsigned char *time)
{
	unsigned int start_byte, size;
	unsigned long cdbuffer_size;
	int ret, sector, block;

	if (cd_file == NULL)
		return -1;

	sector = MSF2SECT(btoi(time[0]), btoi(time[1]), btoi(time[2]));

	// avoid division if possible
	switch (cd_sectors_per_blk) {
	case 1:
		block = sector;
		current_sect_in_blk = 0;
		break;
	case 10:
		block = sector / 10;
		current_sect_in_blk = sector % 10;
		break;
	case 16:
		block = sector >> 4;
		current_sect_in_blk = sector & 15;
		break;
	default:
		err("unhandled cd_sectors_per_blk: %d\n", cd_sectors_per_blk);
		return -1;
	}

	if (block == current_block) {
		// it's already there, nothing to do
		//printf("hit sect %d\n", sector);
		return 0;
	}

	if (sector >= cd_index_len * cd_sectors_per_blk) {
		err("sector %d is past track end\n", sector);
		return -1;
	}

	start_byte = cd_index_table[block];
	if (fseek(cd_file, start_byte, SEEK_SET) != 0) {
		err("seek error for block %d at %x: ",
			block, start_byte);
		perror(NULL);
		return -1;
	}

	size = cd_index_table[block + 1] - start_byte;
	if (size > sizeof(cdbuffer->compressed)) {
		err("block %d is too large: %u\n", block, size);
		return -1;
	}

	if (fread(cdbuffer->compressed, 1, size, cd_file) != size) {
		err("read error for block %d at %x: ", block, start_byte);
		perror(NULL);
		return -1;
	}

	cdbuffer_size = sizeof(cdbuffer->raw[0]) * cd_sectors_per_blk;
	switch (cd_compression) {
	case CDRC_ZLIB:
		ret = uncompress(cdbuffer->raw[0], &cdbuffer_size, cdbuffer->compressed, size);
		break;
	case CDRC_ZLIB2:
		ret = uncompress2_pcsx(cdbuffer->raw[0], &cdbuffer_size, cdbuffer->compressed, size);
		break;
	case CDRC_BZ:
		ret = pBZ2_bzBuffToBuffDecompress((char *)cdbuffer->raw, (unsigned int *)&cdbuffer_size,
			(char *)cdbuffer->compressed, size, 0, 0);
		break;
	default:
		err("bad cd_compression: %d\n", cd_compression);
		return -1;
	}

	if (ret != 0) {
		err("uncompress failed with %d for block %d, sector %d\n",
			ret, block, sector);
		return -1;
	}
	if (cdbuffer_size != sizeof(cdbuffer->raw[0]) * cd_sectors_per_blk)
		err("cdbuffer_size: %lu != %d, sector %d\n", cdbuffer_size,
			(int)sizeof(cdbuffer->raw[0]) * cd_sectors_per_blk, sector);

	// done at last!
	current_block = block;
	return 0;
}

// return read track
static unsigned char *CDRgetBuffer(void)
{
	return cdbuffer->raw[current_sect_in_blk] + 12;
}

// plays cdda audio
// sector: byte 0 - minute; byte 1 - second; byte 2 - frame
// does NOT uses bcd format
static long CDRplay(unsigned char *time)
{
	return 0;
}

// stops cdda audio
static long CDRstop(void)
{
	return 0;
}

// gets subchannel data
static unsigned char* CDRgetBufferSub(void)
{
	return NULL;
}

static long CDRgetStatus(struct CdrStat *stat) {
	CDR__getStatus(stat);

	stat->Type = 0x01;

	return 0;
}

static long CDRclose(void)
{
	if (cd_file != NULL) {
		fclose(cd_file);
		cd_file = NULL;
	}
	if (cd_index_table != NULL) {
		free(cd_index_table);
		cd_index_table = NULL;
	}
	return 0;
}

static long CDRshutdown(void)
{
	return CDRclose();
}

static long CDRinit(void)
{
	if (cdbuffer == NULL) {
		cdbuffer = malloc(sizeof(*cdbuffer));
		if (cdbuffer == NULL) {
			err("OOM\n");
			return -1;
		}
	}
#if !defined(_WIN32) && !defined(NO_DYLIB)
	if (pBZ2_bzBuffToBuffDecompress == NULL) {
		void *h = dlopen("/usr/lib/libbz2.so.1", RTLD_LAZY);
		if (h == NULL)
			h = dlopen("./lib/libbz2.so.1", RTLD_LAZY);
		if (h != NULL) {
			pBZ2_bzBuffToBuffDecompress = dlsym(h, "BZ2_bzBuffToBuffDecompress");
			if (pBZ2_bzBuffToBuffDecompress == NULL) {
				err("dlsym bz2: %s", dlerror());
				dlclose(h);
			}
		}
	}
#endif
	return 0;
}

static long handle_eboot(void)
{
	struct {
		unsigned int sig;
		unsigned int dontcare[8];
		unsigned int psar_offs;
	} pbp_hdr;
	struct {
		unsigned int offset;
		unsigned int size;
		unsigned int dontcare[6];
	} index_entry;
	char psar_sig[9];
	unsigned int cdimg_base;
	int i, ret;
	FILE *f;

	f = fopen(cd_fname, "rb");
	if (f == NULL) {
		err("missing file: %s: ", cd_fname);
		perror(NULL);
		return -1;
	}

	ret = fread(&pbp_hdr, 1, sizeof(pbp_hdr), f);
	if (ret != sizeof(pbp_hdr)) {
		err("failed to read pbp\n");
		goto fail_io;
	}

	ret = fseek(f, pbp_hdr.psar_offs, SEEK_SET);
	if (ret != 0) {
		err("failed to seek to %x\n", pbp_hdr.psar_offs);
		goto fail_io;
	}

	ret = fread(psar_sig, 1, sizeof(psar_sig), f);
	if (ret != sizeof(psar_sig)) {
		err("failed to read psar_sig\n");
		goto fail_io;
	}

	psar_sig[8] = 0;
	if (strcmp(psar_sig, "PSISOIMG") != 0) {
		err("bad psar_sig: %s\n", psar_sig);
		goto fail_io;
	}

	// seek to ISO index
	ret = fseek(f, 0x4000 - sizeof(psar_sig), SEEK_CUR);
	if (ret != 0) {
		err("failed to seek to ISO index\n");
		goto fail_io;
	}

	cd_index_len = (0x100000 - 0x4000) / sizeof(index_entry);
	cd_index_table = malloc((cd_index_len + 1) * sizeof(cd_index_table[0]));
	if (cd_index_table == NULL)
		goto fail_io;

	cdimg_base = pbp_hdr.psar_offs + 0x100000;
	for (i = 0; i < cd_index_len; i++) {
		ret = fread(&index_entry, 1, sizeof(index_entry), f);
		if (ret != sizeof(index_entry)) {
			err("failed to read index_entry #%d\n", i);
			goto fail_index;
		}

		if (index_entry.size == 0)
			break;

		cd_index_table[i] = cdimg_base + index_entry.offset;
	}
	cd_index_table[i] = cdimg_base + index_entry.offset + index_entry.size;

	cd_compression = CDRC_ZLIB2;
	cd_sectors_per_blk = 16;
	cd_file = f;

	printf(PFX "Loaded EBOOT CD Image: %s.\n", cd_fname);
	return 0;

fail_index:
	free(cd_index_table);
	cd_index_table = NULL;
fail_io:
	fclose(f);
	return -1;
}

// This function is invoked by the front-end when opening an ISO
// file for playback
static long CDRopen(void)
{
	union {
		struct {
			unsigned int offset;
			unsigned short size;
		} __attribute__((packed)) ztab_entry;
		struct {
			unsigned int offset;
			unsigned short size;
			unsigned int dontcare;
		} __attribute__((packed)) znxtab_entry;
		unsigned int bztab_entry;
	} u;
	int tabentry_size;
	char table_fname[256];
	long table_size;
	int i, ret;
	char *ext;
	FILE *f = NULL;

	if (cd_file != NULL)
		return 0; // it's already open

	numtracks = 0;
	current_block = -1;
	current_sect_in_blk = 0;

	if (cd_fname == NULL)
		return -1;

	ext = strrchr(cd_fname, '.');
	if (ext == NULL)
		return -1;

	if (strcasecmp(ext, ".pbp") == 0) {
		return handle_eboot();
	}

	// pocketiso stuff
	else if (strcasecmp(ext, ".z") == 0) {
		cd_compression = CDRC_ZLIB;
		tabentry_size = sizeof(u.ztab_entry);
		snprintf(table_fname, sizeof(table_fname), "%s.table", cd_fname);
	}
	else if (strcasecmp(ext, ".znx") == 0) {
		cd_compression = CDRC_ZLIB;
		tabentry_size = sizeof(u.znxtab_entry);
		snprintf(table_fname, sizeof(table_fname), "%s.table", cd_fname);
	}
	else if (strcasecmp(ext, ".bz") == 0) {
		if (pBZ2_bzBuffToBuffDecompress == NULL) {
			err("libbz2 unavailable for .bz2 handling\n");
			return -1;
		}
		cd_compression = CDRC_BZ;
		tabentry_size = sizeof(u.bztab_entry);
		snprintf(table_fname, sizeof(table_fname), "%s.index", cd_fname);
	}
	else {
		err("unhandled extension: %s\n", ext);
		return -1;
	}

	f = fopen(table_fname, "rb");
	if (f == NULL) {
		err("missing file: %s: ", table_fname);
		perror(NULL);
		return -1;
	}

	ret = fseek(f, 0, SEEK_END);
	if (ret != 0) {
		err("failed to seek\n");
		goto fail_table_io;
	}
	table_size = ftell(f);
	fseek(f, 0, SEEK_SET);

	if (table_size > 4 * 1024 * 1024) {
		err(".table too large\n");
		goto fail_table_io;
	}

	cd_index_len = table_size / tabentry_size;

	cd_index_table = malloc((cd_index_len + 1) * sizeof(cd_index_table[0]));
	if (cd_index_table == NULL)
		goto fail_table_io;

	switch (cd_compression) {
	case CDRC_ZLIB:
		// a Z.table file is binary where each element represents
		// one compressed frame.  
		//    4 bytes: the offset of the frame in the .Z file
		//    2 bytes: the length of the compressed frame
		// .znx.table has 4 additional bytes (xa header??)
		u.znxtab_entry.dontcare = 0;
		for (i = 0; i < cd_index_len; i++) {
			ret = fread(&u, 1, tabentry_size, f);
			if (ret != tabentry_size) {
				err(".table read failed on entry %d/%d\n", i, cd_index_len);
				goto fail_table_io_read;
			}
			cd_index_table[i] = u.ztab_entry.offset;
			//if (u.znxtab_entry.dontcare != 0)
			//	printf("znx %08x!\n", u.znxtab_entry.dontcare);
		}
		// fake entry, so that we know last compressed block size
		cd_index_table[i] = u.ztab_entry.offset + u.ztab_entry.size;
		cd_sectors_per_blk = 1;
		break;
	case CDRC_BZ:
		// the .BZ.table file is arranged so that one entry represents
		// 10 compressed frames. Each element is a 4 byte unsigned integer
		// representing the offset in the .BZ file. Last entry is the size
		// of the compressed file.
		for (i = 0; i < cd_index_len; i++) {
			ret = fread(&u.bztab_entry, 1, sizeof(u.bztab_entry), f);
			if (ret != sizeof(u.bztab_entry)) {
				err(".table read failed on entry %d/%d\n", i, cd_index_len);
				goto fail_table_io_read;
			}
			cd_index_table[i] = u.bztab_entry;
		}
		cd_sectors_per_blk = 10;
		break;
	}

	cd_file = fopen(cd_fname, "rb");
	if (cd_file == NULL) {
		err("failed to open: %s: ", table_fname);
		perror(NULL);
		goto fail_img;
	}
	fclose(f);

	printf(PFX "Loaded compressed CD Image: %s.\n", cd_fname);

	return 0;

fail_img:
fail_table_io_read:
	free(cd_index_table);
	cd_index_table = NULL;
fail_table_io:
	fclose(f);
	return -1;
}

#define FUNC(n) { #n, n }

static const struct {
	const char *name;
	void *func;
} plugin_funcs[] = {
	/* CDR */
	FUNC(CDRinit),
	FUNC(CDRshutdown),
	FUNC(CDRopen),
	FUNC(CDRclose),
	FUNC(CDRgetTN),
	FUNC(CDRgetTD),
	FUNC(CDRreadTrack),
	FUNC(CDRgetBuffer),
	FUNC(CDRgetBufferSub),
	FUNC(CDRplay),
	FUNC(CDRstop),
	FUNC(CDRgetStatus),
};

void cdrcimg_set_fname(const char *fname)
{
	cd_fname = fname;
}

void *cdrcimg_get_sym(const char *sym)
{
	int i;
	for (i = 0; i < sizeof(plugin_funcs) / sizeof(plugin_funcs[0]); i++)
		if (strcmp(plugin_funcs[i].name, sym) == 0)
			return plugin_funcs[i].func;
	return NULL;
}

