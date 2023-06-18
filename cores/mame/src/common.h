/*********************************************************************

	common.h

	Generic functions, mostly ROM related.

*********************************************************************/

#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include "hash.h"

#ifdef __cplusplus
extern "C" {
#endif


/***************************************************************************

	Type definitions

***************************************************************************/

struct mame_bitmap
{
	int width,height;	/* width and height of the bitmap */
	int depth;			/* bits per pixel */
	void **line;		/* pointers to the start of each line - can be UINT8 **, UINT16 ** or UINT32 ** */

	/* alternate way of accessing the pixels */
	void *base;			/* pointer to pixel (0,0) (adjusted for padding) */
	int rowpixels;		/* pixels per row (including padding) */
	int rowbytes;		/* bytes per row (including padding) */

	/* functions to render in the correct orientation */
	void (*plot)(struct mame_bitmap *bitmap,int x,int y,pen_t pen);
	pen_t (*read)(struct mame_bitmap *bitmap,int x,int y);
	void (*plot_box)(struct mame_bitmap *bitmap,int x,int y,int width,int height,pen_t pen);
};

#define BITMAP_ADDR(bitmap, type, y, x)	\
	((type *)(bitmap)->base + (y) * (bitmap)->rowpixels + (x))

#define BITMAP_ADDR8(bitmap, y, x)	BITMAP_ADDR(bitmap, UINT8, y, x)
#define BITMAP_ADDR16(bitmap, y, x)	BITMAP_ADDR(bitmap, UINT16, y, x)
#define BITMAP_ADDR32(bitmap, y, x)	BITMAP_ADDR(bitmap, UINT32, y, x)

struct RomModule
{
	const char *_name;	/* name of the file to load */
	UINT32 _offset;		/* offset to load it to */
	UINT32 _length;		/* length of the file */
	UINT32 _flags;		/* flags */
	const char *_hashdata; /* hashing informations (checksums) */
};


struct GameSample
{
	int length;
	int smpfreq;
	int resolution;
	int b_decoded;
	int b_h_decoded;
	char gamename[512];
	char filename[512];
	int filetype;
	signed char data[1]; /* extendable */
};


struct SystemBios
{
	int value;			/* value of mask to apply to ROM_BIOSFLAGS is chosen */
	const char *_name;	/* name of the bios, e.g "default","japan" */
	const char *_description;	/* long name of the bios, e.g "Europe MVS (Ver. 2)" */
};


struct rom_load_data
{
	int warnings;				/* warning count during processing */
	int errors;				/* error count during processing */

	int romsloaded;				/* current ROMs loaded count */
	int romstotal;				/* total number of ROMs to read */

	void * file;				/* current file */

	UINT8 *	regionbase;			/* base of current region */
	UINT32 regionlength;			/* length of current region */

	char errorbuf[4096];			/* accumulated errors */
	UINT8 tempbuf[65536];			/* temporary buffer */
};


struct GameSamples
{
	int total;	/* total number of samples */
	struct GameSample *sample[1];	/* extendable */
};
 
#define	GAME_SAMPLE_LARGE		10000000 /* 10MB */

/***************************************************************************

	Constants and macros

***************************************************************************/

enum
{
	REGION_INVALID = 0x80,
	REGION_CPU1,
	REGION_CPU2,
	REGION_CPU3,
	REGION_CPU4,
	REGION_CPU5,
	REGION_CPU6,
	REGION_CPU7,
	REGION_CPU8,
	REGION_GFX1,
	REGION_GFX2,
	REGION_GFX3,
	REGION_GFX4,
	REGION_GFX5,
	REGION_GFX6,
	REGION_GFX7,
	REGION_GFX8,
	REGION_PROMS,
	REGION_SOUND1,
	REGION_SOUND2,
	REGION_SOUND3,
	REGION_SOUND4,
	REGION_SOUND5,
	REGION_SOUND6,
	REGION_SOUND7,
	REGION_SOUND8,
	REGION_USER1,
	REGION_USER2,
	REGION_USER3,
	REGION_USER4,
	REGION_USER5,
	REGION_USER6,
	REGION_USER7,
	REGION_USER8,
	REGION_DISKS,
	audiocrypt,
	REGION_MAX
};

#define BADCRC( crc ) (~(crc))

#define ROMMD5(md5) ("MD5" #md5)



/***************************************************************************

	Core macros for the ROM loading system

***************************************************************************/

/* ----- per-entry constants ----- */
#define ROMENTRYTYPE_REGION			1					/* this entry marks the start of a region */
#define ROMENTRYTYPE_END			2					/* this entry marks the end of a region */
#define ROMENTRYTYPE_RELOAD			3					/* this entry reloads the previous ROM */
#define ROMENTRYTYPE_CONTINUE		4					/* this entry continues loading the previous ROM */
#define ROMENTRYTYPE_FILL			5					/* this entry fills an area with a constant value */
#define ROMENTRYTYPE_COPY			6					/* this entry copies data from another region/offset */
#define ROMENTRYTYPE_COUNT			7

#define ROMENTRY_REGION				((const char *)ROMENTRYTYPE_REGION)
#define ROMENTRY_END				((const char *)ROMENTRYTYPE_END)
#define ROMENTRY_RELOAD				((const char *)ROMENTRYTYPE_RELOAD)
#define ROMENTRY_CONTINUE			((const char *)ROMENTRYTYPE_CONTINUE)
#define ROMENTRY_FILL				((const char *)ROMENTRYTYPE_FILL)
#define ROMENTRY_COPY				((const char *)ROMENTRYTYPE_COPY)

/* ----- per-entry macros ----- */
#define ROMENTRY_GETTYPE(r)			((FPTR)(r)->_name)
#define ROMENTRY_ISSPECIAL(r)		(ROMENTRY_GETTYPE(r) < ROMENTRYTYPE_COUNT)
#define ROMENTRY_ISFILE(r)			(!ROMENTRY_ISSPECIAL(r))
#define ROMENTRY_ISREGION(r)		((r)->_name == ROMENTRY_REGION)
#define ROMENTRY_ISEND(r)			((r)->_name == ROMENTRY_END)
#define ROMENTRY_ISRELOAD(r)		((r)->_name == ROMENTRY_RELOAD)
#define ROMENTRY_ISCONTINUE(r)		((r)->_name == ROMENTRY_CONTINUE)
#define ROMENTRY_ISFILL(r)			((r)->_name == ROMENTRY_FILL)
#define ROMENTRY_ISCOPY(r)			((r)->_name == ROMENTRY_COPY)
#define ROMENTRY_ISREGIONEND(r)		(ROMENTRY_ISREGION(r) || ROMENTRY_ISEND(r))


/* ----- per-region constants ----- */
#define ROMREGION_WIDTHMASK			0x00000003			/* native width of region, as power of 2 */
#define		ROMREGION_8BIT			0x00000000			/*    (non-CPU regions only) */
#define		ROMREGION_16BIT			0x00000001
#define		ROMREGION_32BIT			0x00000002
#define		ROMREGION_64BIT			0x00000003

#define ROMREGION_ENDIANMASK		0x00000004			/* endianness of the region */
#define		ROMREGION_LE			0x00000000			/*    (non-CPU regions only) */
#define		ROMREGION_BE			0x00000004

#define ROMREGION_INVERTMASK		0x00000008			/* invert the bits of the region */
#define		ROMREGION_NOINVERT		0x00000000
#define		ROMREGION_INVERT		0x00000008

#define ROMREGION_DISPOSEMASK		0x00000010			/* dispose of the region after init */
#define		ROMREGION_NODISPOSE		0x00000000
#define		ROMREGION_DISPOSE		0x00000010

#define ROMREGION_SOUNDONLYMASK		0x00000020			/* load only if sound is enabled */
#define		ROMREGION_NONSOUND		0x00000000
#define		ROMREGION_SOUNDONLY		0x00000020

#define ROMREGION_LOADUPPERMASK		0x00000040			/* load into the upper part of CPU space */
#define		ROMREGION_LOADLOWER		0x00000000			/*     (CPU regions only) */
#define		ROMREGION_LOADUPPER		0x00000040

#define ROMREGION_ERASEMASK			0x00000080			/* erase the region before loading */
#define		ROMREGION_NOERASE		0x00000000
#define		ROMREGION_ERASE			0x00000080

#define ROMREGION_ERASEVALMASK		0x0000ff00			/* value to erase the region to */
#define		ROMREGION_ERASEVAL(x)	((((x) & 0xff) << 8) | ROMREGION_ERASE)
#define		ROMREGION_ERASE00		ROMREGION_ERASEVAL(0)
#define		ROMREGION_ERASEFF		ROMREGION_ERASEVAL(0xff)

#define ROMREGION_DATATYPEMASK		0x00010000			/* inherit all flags from previous definition */
#define		ROMREGION_DATATYPEROM	0x00000000
#define		ROMREGION_DATATYPEDISK	0x00010000

/* ----- per-region macros ----- */
#define ROMREGION_GETTYPE(r)		((UINT32)(r)->_hashdata)
#define ROMREGION_GETLENGTH(r)		((r)->_length)
#define ROMREGION_GETFLAGS(r)		((r)->_flags)
#define ROMREGION_GETWIDTH(r)		(8 << (ROMREGION_GETFLAGS(r) & ROMREGION_WIDTHMASK))
#define ROMREGION_ISLITTLEENDIAN(r)	((ROMREGION_GETFLAGS(r) & ROMREGION_ENDIANMASK) == ROMREGION_LE)
#define ROMREGION_ISBIGENDIAN(r)	((ROMREGION_GETFLAGS(r) & ROMREGION_ENDIANMASK) == ROMREGION_BE)
#define ROMREGION_ISINVERTED(r)		((ROMREGION_GETFLAGS(r) & ROMREGION_INVERTMASK) == ROMREGION_INVERT)
#define ROMREGION_ISDISPOSE(r)		((ROMREGION_GETFLAGS(r) & ROMREGION_DISPOSEMASK) == ROMREGION_DISPOSE)
#define ROMREGION_ISSOUNDONLY(r)	((ROMREGION_GETFLAGS(r) & ROMREGION_SOUNDONLYMASK) == ROMREGION_SOUNDONLY)
#define ROMREGION_ISLOADUPPER(r)	((ROMREGION_GETFLAGS(r) & ROMREGION_LOADUPPERMASK) == ROMREGION_LOADUPPER)
#define ROMREGION_ISERASE(r)		((ROMREGION_GETFLAGS(r) & ROMREGION_ERASEMASK) == ROMREGION_ERASE)
#define ROMREGION_GETERASEVAL(r)	((ROMREGION_GETFLAGS(r) & ROMREGION_ERASEVALMASK) >> 8)
#define ROMREGION_GETDATATYPE(r)	(ROMREGION_GETFLAGS(r) & ROMREGION_DATATYPEMASK)
#define ROMREGION_ISROMDATA(r)		(ROMREGION_GETDATATYPE(r) == ROMREGION_DATATYPEROM)
#define ROMREGION_ISDISKDATA(r)		(ROMREGION_GETDATATYPE(r) == ROMREGION_DATATYPEDISK)


/* ----- per-ROM constants ----- */
#define DISK_READONLYMASK			0x00000400			/* is the disk read-only? */
#define		DISK_READWRITE			0x00000000
#define		DISK_READONLY			0x00000400

#define ROM_OPTIONALMASK			0x00000800			/* optional - won't hurt if it's not there */
#define		ROM_REQUIRED			0x00000000
#define		ROM_OPTIONAL			0x00000800

#define ROM_GROUPMASK				0x0000f000			/* load data in groups of this size + 1 */
#define		ROM_GROUPSIZE(n)		((((n) - 1) & 15) << 12)
#define		ROM_GROUPBYTE			ROM_GROUPSIZE(1)
#define		ROM_GROUPWORD			ROM_GROUPSIZE(2)
#define		ROM_GROUPDWORD			ROM_GROUPSIZE(4)

#define ROM_SKIPMASK				0x000f0000			/* skip this many bytes after each group */
#define		ROM_SKIP(n)				(((n) & 15) << 16)
#define		ROM_NOSKIP				ROM_SKIP(0)

#define ROM_REVERSEMASK				0x00100000			/* reverse the byte order within a group */
#define		ROM_NOREVERSE			0x00000000
#define		ROM_REVERSE				0x00100000

#define ROM_BITWIDTHMASK			0x00e00000			/* width of data in bits */
#define		ROM_BITWIDTH(n)			(((n) & 7) << 21)
#define		ROM_NIBBLE				ROM_BITWIDTH(4)
#define		ROM_FULLBYTE			ROM_BITWIDTH(8)

#define ROM_BITSHIFTMASK			0x07000000			/* left-shift count for the bits */
#define		ROM_BITSHIFT(n)			(((n) & 7) << 24)
#define		ROM_NOSHIFT				ROM_BITSHIFT(0)
#define		ROM_SHIFT_NIBBLE_LO		ROM_BITSHIFT(0)
#define		ROM_SHIFT_NIBBLE_HI		ROM_BITSHIFT(4)

#define ROM_INHERITFLAGSMASK		0x08000000			/* inherit all flags from previous definition */
#define		ROM_INHERITFLAGS		0x08000000

#define ROM_BIOSFLAGSMASK			0xf0000000			/* only loaded if value matches global bios value */
#define 	ROM_BIOS(n)				(((n) & 15) << 28)

#define ROM_INHERITEDFLAGS			(ROM_GROUPMASK | ROM_SKIPMASK | ROM_REVERSEMASK | ROM_BITWIDTHMASK | ROM_BITSHIFTMASK | ROM_BIOSFLAGSMASK)

/* ----- per-ROM macros ----- */
#define ROM_GETNAME(r)				((r)->_name)
#define ROM_SAFEGETNAME(r)			(ROMENTRY_ISFILL(r) ? "fill" : ROMENTRY_ISCOPY(r) ? "copy" : ROM_GETNAME(r))
#define ROM_GETOFFSET(r)			((r)->_offset)
#define ROM_GETLENGTH(r)			((r)->_length)
#define ROM_GETFLAGS(r)				((r)->_flags)
#define ROM_GETHASHDATA(r)          ((r)->_hashdata)
#define ROM_ISOPTIONAL(r)			((ROM_GETFLAGS(r) & ROM_OPTIONALMASK) == ROM_OPTIONAL)
#define ROM_GETGROUPSIZE(r)			(((ROM_GETFLAGS(r) & ROM_GROUPMASK) >> 12) + 1)
#define ROM_GETSKIPCOUNT(r)			((ROM_GETFLAGS(r) & ROM_SKIPMASK) >> 16)
#define ROM_ISREVERSED(r)			((ROM_GETFLAGS(r) & ROM_REVERSEMASK) == ROM_REVERSE)
#define ROM_GETBITWIDTH(r)			(((ROM_GETFLAGS(r) & ROM_BITWIDTHMASK) >> 21) + 8 * ((ROM_GETFLAGS(r) & ROM_BITWIDTHMASK) == 0))
#define ROM_GETBITSHIFT(r)			((ROM_GETFLAGS(r) & ROM_BITSHIFTMASK) >> 24)
#define ROM_INHERITSFLAGS(r)		((ROM_GETFLAGS(r) & ROM_INHERITFLAGSMASK) == ROM_INHERITFLAGS)
#define ROM_GETBIOSFLAGS(r)			((ROM_GETFLAGS(r) & ROM_BIOSFLAGSMASK) >> 28)
#define ROM_NOGOODDUMP(r)			(hash_data_has_info((r)->_hashdata, HASH_INFO_NO_DUMP))

/* ----- per-disk macros ----- */
#define DISK_GETINDEX(r)			((r)->_offset)
#define DISK_ISREADONLY(r)			((ROM_GETFLAGS(r) & DISK_READONLYMASK) == DISK_READONLY)



/***************************************************************************

	Derived macros for the ROM loading system

***************************************************************************/

/* ----- start/stop macros ----- */
#define ROM_START(name)								static const struct RomModule rom_##name[] = {
#define ROM_END                                      { ROMENTRY_END, 0, 0, 0, NULL } };

/* ----- ROM region macros ----- */
#define ROM_REGION(length,type,flags)                { ROMENTRY_REGION, 0, length, flags, (const char*)type },
#define ROM_REGION16_LE(length,type,flags)			ROM_REGION(length, type, (flags) | ROMREGION_16BIT | ROMREGION_LE)
#define ROM_REGION16_BE(length,type,flags)			ROM_REGION(length, type, (flags) | ROMREGION_16BIT | ROMREGION_BE)
#define ROM_REGION32_LE(length,type,flags)			ROM_REGION(length, type, (flags) | ROMREGION_32BIT | ROMREGION_LE)
#define ROM_REGION32_BE(length,type,flags)			ROM_REGION(length, type, (flags) | ROMREGION_32BIT | ROMREGION_BE)

/* ----- core ROM loading macros ----- */
#define ROMMD5_LOAD(name,offset,length,hash,flags)   { name, offset, length, flags, hash },
#define ROMX_LOAD(name,offset,length,hash,flags)     { name, offset, length, flags, hash },
#define ROM_LOAD(name,offset,length,hash)            ROMX_LOAD(name, offset, length, hash, 0)
#define ROM_LOAD_OPTIONAL(name,offset,length,hash)   ROMX_LOAD(name, offset, length, hash, ROM_OPTIONAL)
#define ROM_CONTINUE(offset,length)					ROMX_LOAD(ROMENTRY_CONTINUE, offset, length, 0, ROM_INHERITFLAGS)
#define ROM_RELOAD(offset,length)					ROMX_LOAD(ROMENTRY_RELOAD, offset, length, 0, ROM_INHERITFLAGS)
#define ROM_FILL(offset,length,value)                ROM_LOAD(ROMENTRY_FILL, offset, length, (const char*)value)
#define ROM_COPY(rgn,srcoffset,offset,length)        ROMX_LOAD(ROMENTRY_COPY, offset, length, (const char*)srcoffset, (rgn) << 24)

/* ----- nibble loading macros ----- */
#define ROM_LOAD_NIB_HIGH(name,offset,length,hash)   ROMX_LOAD(name, offset, length, hash, ROM_NIBBLE | ROM_SHIFT_NIBBLE_HI)
#define ROM_LOAD_NIB_LOW(name,offset,length,hash)    ROMX_LOAD(name, offset, length, hash, ROM_NIBBLE | ROM_SHIFT_NIBBLE_LO)

/* ----- new-style 16-bit loading macros ----- */
#define ROM_LOAD16_BYTE(name,offset,length,hash)     ROMX_LOAD(name, offset, length, hash, ROM_SKIP(1))
#define ROM_LOAD16_WORD(name,offset,length,hash)     ROM_LOAD(name, offset, length, hash)
#define ROM_LOAD16_WORD_SWAP(name,offset,length,hash)ROMX_LOAD(name, offset, length, hash, ROM_GROUPWORD | ROM_REVERSE)

/* ----- new-style 32-bit loading macros ----- */
#define ROM_LOAD32_BYTE(name,offset,length,hash)     ROMX_LOAD(name, offset, length, hash, ROM_SKIP(3))
#define ROM_LOAD32_WORD(name,offset,length,hash)     ROMX_LOAD(name, offset, length, hash, ROM_GROUPWORD | ROM_SKIP(2))
#define ROM_LOAD32_WORD_SWAP(name,offset,length,hash)ROMX_LOAD(name, offset, length, hash, ROM_GROUPWORD | ROM_REVERSE | ROM_SKIP(2))
#define ROM_LOAD32_DWORD(name,offset,length,hash)    ROMX_LOAD(name, offset, length, hash, ROM_GROUPDWORD)

/* ----- new-style 64-bit loading macros ----- */
#define ROM_LOAD64_BYTE(name,offset,length,hash)        ROMX_LOAD(name, offset, length, hash, ROM_SKIP(7))
#define ROM_LOAD64_WORD(name,offset,length,hash)    ROMX_LOAD(name, offset, length, hash, ROM_GROUPWORD | ROM_SKIP(6))
#define ROM_LOAD64_WORD_SWAP(name,offset,length,hash) ROMX_LOAD(name, offset, length, hash, ROM_GROUPWORD | ROM_REVERSE | ROM_SKIP(6))
#define ROM_LOAD64_DWORD_SWAP(name,offset,length,hash) ROMX_LOAD(name, offset, length, hash, ROM_GROUPDWORD | ROM_REVERSE | ROM_SKIP(4))

/* ----- disk loading macros ----- */
#define DISK_REGION(type)							ROM_REGION(1, type, ROMREGION_DATATYPEDISK)
#define DISK_IMAGE(name,idx,hash)                    ROMMD5_LOAD(name, idx, 0, hash, DISK_READWRITE)
#define DISK_IMAGE_READONLY(name,idx,hash)           ROMMD5_LOAD(name, idx, 0, hash, DISK_READONLY)

/* ----- hash macros ----- */
#define CRC(x)                                       "c:" #x "#"
#define SHA1(x)                                      "s:" #x "#"
#define MD5(x)                                       "m:" #x "#"
#define NO_DUMP                                      "$ND$"
#define BAD_DUMP                                     "$BD$"

/* @@@ FF: Remove this when we use the final SHA1Merger */
#define NOT_DUMPED NO_DUMP
#define BADROM BAD_DUMP

/***************************************************************************

	Derived macros for the alternate BIOS loading system

***************************************************************************/

#define BIOSENTRY_ISEND(b)		((b)->_name == NULL)

/* ----- start/stop macros ----- */
#define SYSTEM_BIOS_START(name)			static const struct SystemBios system_bios_##name[] = {
#define SYSTEM_BIOS_END					{ 0, NULL } };

/* ----- ROM region macros ----- */
#define SYSTEM_BIOS_ADD(value,name,description)		{ (int)value, (const char*)name, (const char*)description },
#define BIOS_DEFAULT			"default"


/***************************************************************************

	Function prototypes

***************************************************************************/

void showdisclaimer(void);

/* helper function that reads samples from disk - this can be used by other */
/* drivers as well (e.g. a sound chip emulator needing drum samples) */
struct GameSamples *readsamples(const char **samplenames,const char *name);
#define freesamples(samps)

/* return a pointer to the specified memory region - num can be either an absolute */
/* number, or one of the REGION_XXX identifiers defined above */
UINT8 *memory_region(int num);
size_t memory_region_length(int num);

/* allocate a new memory region - num can be either an absolute */
/* number, or one of the REGION_XXX identifiers defined above */
int new_memory_region(int num, size_t length, UINT32 flags);
void free_memory_region(int num);

/* common coin counter helpers */
#define COIN_COUNTERS	8	/* total # of coin counters */
void coin_counter_w(int num,int on);
void coin_lockout_w(int num,int on);
void coin_lockout_global_w(int on);  /* Locks out all coin inputs */

/* generic NVRAM handler */
extern size_t generic_nvram_size;
extern data8_t *generic_nvram;
extern void nvram_handler_generic_0fill(mame_file *file, int read_or_write);
extern void nvram_handler_generic_1fill(mame_file *file, int read_or_write);

/* bitmap allocation */
struct mame_bitmap *bitmap_alloc(int width,int height);
struct mame_bitmap *bitmap_alloc_depth(int width,int height,int depth);
void bitmap_free(struct mame_bitmap *bitmap);

/* automatic resource management */
void begin_resource_tracking(void);
void end_resource_tracking(void);
static INLINE int get_resource_tag(void)
{
	extern int resource_tracking_tag;
	return resource_tracking_tag;
}

/* automatically-freeing memory */
void *auto_malloc(size_t size);
char *auto_strdup(const char *str);
struct mame_bitmap *auto_bitmap_alloc(int width,int height);
struct mame_bitmap *auto_bitmap_alloc_depth(int width,int height,int depth);

/* disk handling */
struct chd_file *get_disk_handle(int diskindex);

/* ROM processing */
int rom_load(const struct RomModule *romp);
const struct RomModule *rom_first_region(const struct GameDriver *drv);
const struct RomModule *rom_next_region(const struct RomModule *romp);
const struct RomModule *rom_first_file(const struct RomModule *romp);
const struct RomModule *rom_next_file(const struct RomModule *romp);
const struct RomModule *rom_first_chunk(const struct RomModule *romp);
const struct RomModule *rom_next_chunk(const struct RomModule *romp);

void printromlist(const struct RomModule *romp,const char *name);



/***************************************************************************

	Useful macros to deal with bit shuffling encryptions

***************************************************************************/

#define BIT(x,n) (((x)>>(n))&1)

#define BITSWAP8(val,B7,B6,B5,B4,B3,B2,B1,B0) \
		((BIT(val,B7) << 7) | \
		 (BIT(val,B6) << 6) | \
		 (BIT(val,B5) << 5) | \
		 (BIT(val,B4) << 4) | \
		 (BIT(val,B3) << 3) | \
		 (BIT(val,B2) << 2) | \
		 (BIT(val,B1) << 1) | \
		 (BIT(val,B0) << 0))

#define BITSWAP16(val,B15,B14,B13,B12,B11,B10,B9,B8,B7,B6,B5,B4,B3,B2,B1,B0) \
		((BIT(val,B15) << 15) | \
		 (BIT(val,B14) << 14) | \
		 (BIT(val,B13) << 13) | \
		 (BIT(val,B12) << 12) | \
		 (BIT(val,B11) << 11) | \
		 (BIT(val,B10) << 10) | \
		 (BIT(val, B9) <<  9) | \
		 (BIT(val, B8) <<  8) | \
		 (BIT(val, B7) <<  7) | \
		 (BIT(val, B6) <<  6) | \
		 (BIT(val, B5) <<  5) | \
		 (BIT(val, B4) <<  4) | \
		 (BIT(val, B3) <<  3) | \
		 (BIT(val, B2) <<  2) | \
		 (BIT(val, B1) <<  1) | \
		 (BIT(val, B0) <<  0))

#define BITSWAP24(val,B23,B22,B21,B20,B19,B18,B17,B16,B15,B14,B13,B12,B11,B10,B9,B8,B7,B6,B5,B4,B3,B2,B1,B0) \
		((BIT(val,B23) << 23) | \
		 (BIT(val,B22) << 22) | \
		 (BIT(val,B21) << 21) | \
		 (BIT(val,B20) << 20) | \
		 (BIT(val,B19) << 19) | \
		 (BIT(val,B18) << 18) | \
		 (BIT(val,B17) << 17) | \
		 (BIT(val,B16) << 16) | \
		 (BIT(val,B15) << 15) | \
		 (BIT(val,B14) << 14) | \
		 (BIT(val,B13) << 13) | \
		 (BIT(val,B12) << 12) | \
		 (BIT(val,B11) << 11) | \
		 (BIT(val,B10) << 10) | \
		 (BIT(val, B9) <<  9) | \
		 (BIT(val, B8) <<  8) | \
		 (BIT(val, B7) <<  7) | \
		 (BIT(val, B6) <<  6) | \
		 (BIT(val, B5) <<  5) | \
		 (BIT(val, B4) <<  4) | \
		 (BIT(val, B3) <<  3) | \
		 (BIT(val, B2) <<  2) | \
		 (BIT(val, B1) <<  1) | \
		 (BIT(val, B0) <<  0))
	
#define BITSWAP32(val,B31,B30,B29,B28,B27,B26,B25,B24,B23,B22,B21,B20,B19,B18,B17,B16,B15,B14,B13,B12,B11,B10,B9,B8,B7,B6,B5,B4,B3,B2,B1,B0) \
        ((BIT(val,B31) << 31) | \
         (BIT(val,B30) << 30) | \
         (BIT(val,B29) << 29) | \
         (BIT(val,B28) << 28) | \
         (BIT(val,B27) << 27) | \
         (BIT(val,B26) << 26) | \
         (BIT(val,B25) << 25) | \
         (BIT(val,B24) << 24) | \
         (BIT(val,B23) << 23) | \
         (BIT(val,B22) << 22) | \
         (BIT(val,B21) << 21) | \
         (BIT(val,B20) << 20) | \
         (BIT(val,B19) << 19) | \
         (BIT(val,B18) << 18) | \
         (BIT(val,B17) << 17) | \
         (BIT(val,B16) << 16) | \
         (BIT(val,B15) << 15) | \
         (BIT(val,B14) << 14) | \
         (BIT(val,B13) << 13) | \
         (BIT(val,B12) << 12) | \
         (BIT(val,B11) << 11) | \
         (BIT(val,B10) << 10) | \
         (BIT(val, B9) <<  9) | \
         (BIT(val, B8) <<  8) | \
         (BIT(val, B7) <<  7) | \
         (BIT(val, B6) <<  6) | \
         (BIT(val, B5) <<  5) | \
         (BIT(val, B4) <<  4) | \
         (BIT(val, B3) <<  3) | \
         (BIT(val, B2) <<  2) | \
         (BIT(val, B1) <<  1) | \
         (BIT(val, B0) <<  0))

/* Standard MIN/MAX macros */
#ifndef MIN
#define MIN(x,y)			((x) < (y) ? (x) : (y))
#endif
#ifndef MAX
#define MAX(x,y)			((x) > (y) ? (x) : (y))
#endif


#ifdef __cplusplus
}
#endif

#endif
