/*********************************************************************
	common.c
	Generic functions, mostly ROM and graphics related.
*********************************************************************/

#include "driver.h"
#include "png.h"
#include "harddisk.h"
#include "artwork.h"
#include "bootstrap.h"
#include <stdarg.h>
#include <ctype.h>
#include <string/stdstring.h>
#include "libretro-deps/libFLAC/include/FLAC/all.h"
#include "log.h"
/*#define LOG_LOAD */


char *chd_error_text[] =
{
	"CHDERR_NONE",
	"CHDERR_NO_INTERFACE",
	"CHDERR_OUT_OF_MEMORY",
	"CHDERR_INVALID_FILE",
	"CHDERR_INVALID_PARAMETER",
	"CHDERR_INVALID_DATA",
	"CHDERR_FILE_NOT_FOUND",
	"CHDERR_REQUIRES_PARENT",
	"CHDERR_FILE_NOT_WRITEABLE",
	"CHDERR_READ_ERROR",
	"CHDERR_WRITE_ERROR",
	"CHDERR_CODEC_ERROR",
	"CHDERR_INVALID_PARENT",
	"CHDERR_HUNK_OUT_OF_RANGE",
	"CHDERR_DECOMPRESSION_ERROR",
	"CHDERR_COMPRESSION_ERROR",
	"CHDERR_CANT_CREATE_FILE",
	"CHDERR_CANT_VERIFY",
	"CHDERR_NOT_SUPPORTED",
	"CHDERR_METADATA_NOT_FOUND",
	"CHDERR_INVALID_METADATA_SIZE",
	"CHDERR_UNSUPPORTED_VERSION"
};
/***************************************************************************
	Constants
***************************************************************************/

/* VERY IMPORTANT: osd_alloc_bitmap must allocate also a "safety area" 16 pixels wide all */
/* around the bitmap. This is required because, for performance reasons, some graphic */
/* routines don't clip at boundaries of the bitmap. */
#define BITMAP_SAFETY			16

#define MAX_MALLOCS				4096

/***************************************************************************
	Type definitions
***************************************************************************/

struct malloc_info
{
	int tag;
	void *ptr;
};


/***************************************************************************
	FLAC stuff
***************************************************************************/
typedef struct _flac_reader
{
	UINT8* rawdata;
	INT16* write_data;
	int position;
	int length;
	int decoded_size;
	int sample_rate;
	int channels;
	int bits_per_sample;
	int total_samples;
	int write_position;
}flac_reader;


void my_error_callback(const FLAC__StreamDecoder * decoder, FLAC__StreamDecoderErrorStatus status, void * client_data)
{
	printf("FLAC callback error\n");
}

void my_metadata_callback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
{

	flac_reader *flacrd =  (flac_reader*)client_data;

	if (metadata->type==0)
	{
		const FLAC__StreamMetadata_StreamInfo *streaminfo = &(metadata->data.stream_info);

		flacrd->sample_rate = streaminfo->sample_rate;
		flacrd->channels = streaminfo->channels;
		flacrd->bits_per_sample = streaminfo->bits_per_sample;
		flacrd->total_samples = streaminfo->total_samples;
	}
}




FLAC__StreamDecoderReadStatus my_read_callback(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *client_data)
{
	flac_reader *flacrd =  (flac_reader*)client_data;

	if(*bytes > 0)
	{
		if (*bytes <=  flacrd->length)
		{
			memcpy(buffer, flacrd->rawdata+flacrd->position, *bytes);
			flacrd->position+=*bytes;
			flacrd->length-=*bytes;
			return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
		}
		else
		{
			memcpy(buffer, flacrd->rawdata+flacrd->position,  flacrd->length);
		    flacrd->position+= flacrd->length;
			flacrd->length = 0;

			return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
		}
	}
	else
	{
		return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
	}

	if ( flacrd->length==0)
	{
		return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
	}

	return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;

}

FLAC__StreamDecoderWriteStatus my_write_callback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *const buffer[], void *client_data)
{
	int i;
	flac_reader *flacrd =  (flac_reader*)client_data;

	flacrd->decoded_size += frame->header.blocksize;

	for ( i=0;i<frame->header.blocksize;i++)
	{
		flacrd->write_data[i+flacrd->write_position] = buffer[0][i];
	}

	flacrd->write_position +=  frame->header.blocksize;

	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

/***************************************************************************
	Global variables
***************************************************************************/

/* These globals are only kept on a machine basis - LBO 042898 */
unsigned int dispensed_tickets;
unsigned int coins[COIN_COUNTERS];
unsigned int lastcoin[COIN_COUNTERS];
unsigned int coinlockedout[COIN_COUNTERS];

int snapno;

/* malloc tracking */
static struct malloc_info malloc_list[MAX_MALLOCS];
static int malloc_list_index = 0;

/* resource tracking */
int resource_tracking_tag = 0;

/* generic NVRAM */
size_t generic_nvram_size;
data8_t *generic_nvram;

/* disks */
static struct chd_file *disk_handle[4];

/* system BIOS */
static int system_bios;


/***************************************************************************
	Functions
***************************************************************************/

void showdisclaimer(void)   /* MAURY_BEGIN: dichiarazione */
{
	printf("MAME is an emulator: it reproduces, more or less faithfully, the behaviour of\n"
		 "several arcade machines. But hardware is useless without software, so an image\n"
		 "of the ROMs which run on that hardware is required. Such ROMs, like any other\n"
		 "commercial software, are copyrighted material and it is therefore illegal to\n"
		 "use them if you don't own the original arcade machine. Needless to say, ROMs\n"
		 "are not distributed together with MAME. Distribution of MAME together with ROM\n"
		 "images is a violation of copyright law and should be promptly reported to the\n"
		 "authors so that appropriate legal action can be taken.\n\n");
}                           /* MAURY_END: dichiarazione */



/***************************************************************************
	Sample handling code
	This function is different from readroms() because it doesn't fail if
	it doesn't find a file: it will load as many samples as it can find.
***************************************************************************/

#ifdef MSB_FIRST
#define intelLong(x) (((x << 24) | (((unsigned long) x) >> 24) | (( x & 0x0000ff00) << 8) | (( x & 0x00ff0000) >> 8)))
#else
#define intelLong(x) (x)
#endif

/*-------------------------------------------------
	read_wav_sample - read a WAV file as a sample
-------------------------------------------------*/
static struct GameSample *read_wav_sample(mame_file *f, const char *gamename, const char *filename, int filetype, int b_data)
{
	unsigned long offset = 0;
	UINT32 length, rate, filesize, temp32;
	UINT16 bits, temp16;
	char buf[32];
	struct GameSample *result;
	int f_type = 0;

	/* read the core header and make sure it's a proper  file */
	offset += mame_fread(f, buf, 4);

	if (offset < 4)
		return NULL;

	if (memcmp(&buf[0], "RIFF", 4) == 0)
		f_type = 1; /* WAV */
	else if (memcmp(&buf[0], "fLaC", 4) == 0)
		f_type = 2; /* FLAC */
	else
		return NULL; /* No idea what file this is. */

	/* Load WAV file. */
	if(f_type == 1) {
		/* get the total size */
		offset += mame_fread(f, &filesize, 4);
		if (offset < 8)
			return NULL;
		filesize = intelLong(filesize);

		/* read the RIFF file type and make sure it's a WAVE file */
		offset += mame_fread(f, buf, 4);
		if (offset < 12)
			return NULL;
		if (memcmp(&buf[0], "WAVE", 4) != 0)
			return NULL;

		/* seek until we find a format tag */
		while (1)
		{
			offset += mame_fread(f, buf, 4);
			offset += mame_fread(f, &length, 4);
			length = intelLong(length);
			if (memcmp(&buf[0], "fmt ", 4) == 0)
				break;

			/* seek to the next block */
			mame_fseek(f, length, SEEK_CUR);
			offset += length;
			if (offset >= filesize)
				return NULL;
		}

		/* read the format -- make sure it is PCM */
		offset += mame_fread_lsbfirst(f, &temp16, 2);
		if (temp16 != 1)
			return NULL;

		/* number of channels -- only mono is supported */
		offset += mame_fread_lsbfirst(f, &temp16, 2);
		if (temp16 != 1)
			return NULL;

		/* sample rate */
		offset += mame_fread(f, &rate, 4);
		rate = intelLong(rate);

		/* bytes/second and block alignment are ignored */
		offset += mame_fread(f, buf, 6);

		/* bits/sample */
		offset += mame_fread_lsbfirst(f, &bits, 2);
		if (bits != 8 && bits != 16)
			return NULL;

		/* seek past any extra data */
		mame_fseek(f, length - 16, SEEK_CUR);
		offset += length - 16;

		/* seek until we find a data tag */
		while (1)
		{
			offset += mame_fread(f, buf, 4);
			offset += mame_fread(f, &length, 4);
			length = intelLong(length);
			if (memcmp(&buf[0], "data", 4) == 0)
				break;

			/* seek to the next block */
			mame_fseek(f, length, SEEK_CUR);
			offset += length;
			if (offset >= filesize)
				return NULL;
		}

		/* For small samples, lets force them to pre load into memory. */
		if(length <= GAME_SAMPLE_LARGE)
			b_data = 1;

		/* allocate the game sample */
		if(b_data == 1)
			result = auto_malloc(sizeof(struct GameSample) + length);
		else
			result = malloc(sizeof(struct GameSample));

		if (result == NULL)
			return NULL;

		/* fill in the sample data */
		strcpy(result->gamename, gamename);
		strcpy(result->filename, filename);
		result->filetype = filetype;

		result->length = length;
		result->smpfreq = rate;
		result->resolution = bits;

		if(b_data == 1) {
			/* read the data in */
			if (bits == 8)
			{
				mame_fread(f, result->data, length);

				/* convert 8-bit data to signed samples */
				for (temp32 = 0; temp32 < length; temp32++)
					result->data[temp32] ^= 0x80;
			}
			else
			{
				/* 16-bit data is fine as-is */
				mame_fread_lsbfirst(f, result->data, length);
			}

			result->b_decoded = 1;
		}
		else
			result->b_decoded = 0;

		return result;
	}
	else if(f_type == 2) { /* Load FLAC file. */
		int f_length;
    flac_reader flac_file;
    FLAC__StreamDecoder *decoder;

		mame_fseek(f, 0, SEEK_END);
		f_length = mame_ftell(f);
		mame_fseek(f, 0, 0);

		/* For small samples, lets force them to pre load into memory. */
		if (f_length <= GAME_SAMPLE_LARGE)
			b_data = 1;

		flac_file.length = f_length;
		flac_file.position = 0;
		flac_file.decoded_size = 0;

		/* Allocate space for the data. */
		flac_file.rawdata = malloc(f_length);

		/* Read the sample data in. */
		mame_fread(f, flac_file.rawdata, f_length);
		decoder = FLAC__stream_decoder_new();

    if (!decoder) {
			free(flac_file.rawdata);
			return NULL;
		}

		if(FLAC__stream_decoder_init_stream(decoder, my_read_callback,
			NULL, /*my_seek_callback,      // or NULL */
			NULL, /*my_tell_callback,      // or NULL */
			NULL, /*my_length_callback,    // or NULL */
			NULL, /*my_eof_callback,       // or NULL */
			my_write_callback,
			my_metadata_callback, /*my_metadata_callback,  // or NULL */
			my_error_callback,
			(void*)&flac_file) != FLAC__STREAM_DECODER_INIT_STATUS_OK)
				return NULL;

		if (FLAC__stream_decoder_process_until_end_of_metadata(decoder) != FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM) {
			free(flac_file.rawdata);
			FLAC__stream_decoder_delete(decoder);
			return NULL;
		}

		/* only Mono supported */
		if (flac_file.channels != 1) {
			free(flac_file.rawdata);
			FLAC__stream_decoder_delete(decoder);
			return NULL;
		}

		/* only support 16 bit. */
		if (flac_file.bits_per_sample != 16) {
			free(flac_file.rawdata);
			FLAC__stream_decoder_delete(decoder);
			return NULL;
		}

		if (b_data == 1)
			result = auto_malloc(sizeof(struct GameSample) + (flac_file.total_samples * (flac_file.bits_per_sample / 8)));
		else
			result = malloc(sizeof(struct GameSample));

		strcpy(result->gamename, gamename);
		strcpy(result->filename, filename);
		result->filetype = filetype;

		result->smpfreq = flac_file.sample_rate;
		result->length = flac_file.total_samples * (flac_file.bits_per_sample / 8);
		result->resolution = flac_file.bits_per_sample;
		flac_file.write_position = 0;

		if (b_data == 1) {
			flac_file.write_data = result->data;

			if (FLAC__stream_decoder_process_until_end_of_stream (decoder) != FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM) {
				free(flac_file.rawdata);
				FLAC__stream_decoder_delete(decoder);
				return NULL;
			}

			result->b_decoded = 1;
		}
		else
			result->b_decoded = 0;

		if (FLAC__stream_decoder_finish (decoder) != true) {
			free(flac_file.rawdata);
			FLAC__stream_decoder_delete(decoder);
			return NULL;
		}

		FLAC__stream_decoder_delete(decoder);

		free(flac_file.rawdata);

		return result;
	}
	else
		return NULL;
}

/* Handles freeing previous played sample from memory. Helps with the low memory devices which load large sample files. */

void readsample(struct GameSample *SampleInfo, int channel, struct GameSamples *SamplesData, int load)
{
	mame_file *f;
	struct GameSample *SampleFile;

	/* Try opening the file. */
	f = mame_fopen(SampleInfo->gamename,SampleInfo->filename,SampleInfo->filetype,0);

	if (f != 0) {
		char gamename[512];
		char filename[512];
		int filetype = SampleInfo->filetype;

		strcpy(gamename, SampleInfo->gamename);
		strcpy(filename, SampleInfo->filename);

		/* Free up some memory. */
		free(SamplesData->sample[channel]);

		/* Reload or load a sample into memory. */
		SamplesData->sample[channel] = read_wav_sample(f, gamename, filename, filetype, load);

		mame_fclose(f);
	}
}

/*-------------------------------------------------
  readsamples() load all samples
  -------------------------------------------------*/

struct GameSamples *readsamples(const char **samplenames,const char *basename)
/* V.V - avoids samples duplication */
/* if first samplename is *dir, looks for samples into "basename" first, then "dir" */
{
	int i;
	struct GameSamples *samples;
	int skipfirst = 0;
  bool missing_sample = false;

	/* if the user doesn't want to use samples, bail */
	if( !options.use_samples ) return 0;
	if( (!options.use_alt_sound)  &&  (options.content_flags[CONTENT_ALT_SOUND]) ) return 0;

	if (samplenames == 0 || samplenames[0] == 0) return 0;

	if (samplenames[0][0] == '*')
		skipfirst = 1;

	i = 0;
	while (samplenames[i+skipfirst] != 0) i++;

	if (!i) return 0;

	if ((samples = auto_malloc(sizeof(struct GameSamples) + (i-1)*sizeof(struct GameSample))) == 0)
		return 0;

	samples->total = i;
	for (i = 0;i < samples->total;i++)
		samples->sample[i] = 0;

	for (i = 0;i < samples->total;i++)
	{
		mame_file *f;
		int f_type = 0;
		int f_skip = 0;

		if (samplenames[i+skipfirst][0])
		{
			/* Try opening FLAC samples first. */
			if ((f = mame_fopen(basename,samplenames[i+skipfirst],FILETYPE_SAMPLE_FLAC,0)) == 0)
			{
				if (skipfirst) {
					f = mame_fopen(samplenames[0]+1,samplenames[i+skipfirst],FILETYPE_SAMPLE_FLAC,0);
					f_skip = 1;
				}

				/* Fall back to WAV if it exists. */
				if (!f)
				{
					f_type = 1;
					f_skip = 0;

					if ((f = mame_fopen(basename,samplenames[i+skipfirst],FILETYPE_SAMPLE,0)) == 0)
						if (skipfirst) {
							f = mame_fopen(samplenames[0]+1,samplenames[i+skipfirst],FILETYPE_SAMPLE,0);
							f_skip = 1;
						}
				}
			}

			/* Get sample info. Small sample files will pre load into memory at this point. */
			if (f != 0)
			{
				/* Open FLAC. */
				if(f_type == 0) {
					if (f_skip == 1)
						samples->sample[i] = read_wav_sample(f, samplenames[0]+1, samplenames[i+skipfirst], FILETYPE_SAMPLE_FLAC, 0);
					else
						samples->sample[i] = read_wav_sample(f, basename, samplenames[i+skipfirst], FILETYPE_SAMPLE_FLAC, 0);
				}
				else { /* Open WAV. */
					if (f_skip == 1)
						samples->sample[i] = read_wav_sample(f, samplenames[0]+1, samplenames[i+skipfirst], FILETYPE_SAMPLE, 0);
					else
						samples->sample[i] = read_wav_sample(f, basename, samplenames[i+skipfirst], FILETYPE_SAMPLE, 0);
				}

				mame_fclose(f);
			}
			else if (samples->sample[i] == NULL)
			{
				log_cb(RETRO_LOG_WARN, LOGPRE "Missing audio sample: %s\n", samplenames[i+skipfirst]);
				missing_sample = true;
				log_cb(RETRO_LOG_WARN, LOGPRE "Warning: audio sample(s) not found.\n");
				frontend_message_cb("Warning: audio sample(s) not found.", 180);
			}
    }
  }

  return samples;
}



/***************************************************************************

	Memory region code

***************************************************************************/

/*-------------------------------------------------
	memory_region - returns pointer to a memory
	region
-------------------------------------------------*/

unsigned char *memory_region(int num)
{
	int i;

	if (num < MAX_MEMORY_REGIONS)
		return Machine->memory_region[num].base;
	else
	{
		for (i = 0;i < MAX_MEMORY_REGIONS;i++)
		{
			if (Machine->memory_region[i].type == num)
				return Machine->memory_region[i].base;
		}
	}

	return 0;
}


/*-------------------------------------------------
	memory_region_length - returns length of a
	memory region
-------------------------------------------------*/

size_t memory_region_length(int num)
{
	int i;

	if (num < MAX_MEMORY_REGIONS)
		return Machine->memory_region[num].length;
	else
	{
		for (i = 0;i < MAX_MEMORY_REGIONS;i++)
		{
			if (Machine->memory_region[i].type == num)
				return Machine->memory_region[i].length;
		}
	}

	return 0;
}


/*-------------------------------------------------
	new_memory_region - allocates memory for a
	region
-------------------------------------------------*/

int new_memory_region(int num, size_t length, UINT32 flags)
{
    int i;

    if (num < MAX_MEMORY_REGIONS)
    {
        Machine->memory_region[num].length = length;
        Machine->memory_region[num].base = malloc(length);
        return (Machine->memory_region[num].base == NULL) ? 1 : 0;
    }
    else
    {
        for (i = 0;i < MAX_MEMORY_REGIONS;i++)
        {
            if (Machine->memory_region[i].base == NULL)
            {
                Machine->memory_region[i].length = length;
                Machine->memory_region[i].type = num;
                Machine->memory_region[i].flags = flags;
                Machine->memory_region[i].base = malloc(length);
                return (Machine->memory_region[i].base == NULL) ? 1 : 0;
            }
        }
    }
	return 1;
}


/*-------------------------------------------------
	free_memory_region - releases memory for a
	region
-------------------------------------------------*/

void free_memory_region(int num)
{
	int i;

	if (num < MAX_MEMORY_REGIONS)
	{
		free(Machine->memory_region[num].base);
		memset(&Machine->memory_region[num], 0, sizeof(Machine->memory_region[num]));
	}
	else
	{
		for (i = 0;i < MAX_MEMORY_REGIONS;i++)
		{
			if (Machine->memory_region[i].type == num)
			{
				free(Machine->memory_region[i].base);
				memset(&Machine->memory_region[i], 0, sizeof(Machine->memory_region[i]));
				return;
			}
		}
	}
}



/***************************************************************************

	Coin counter code

***************************************************************************/

/*-------------------------------------------------
	coin_counter_w - sets input for coin counter
-------------------------------------------------*/

void coin_counter_w(int num,int on)
{
	if (num >= COIN_COUNTERS) return;
	/* Count it only if the data has changed from 0 to non-zero */
	if (on && (lastcoin[num] == 0))
	{
		coins[num]++;
	}
	lastcoin[num] = on;
}


/*-------------------------------------------------
	coin_lockout_w - locks out one coin input
-------------------------------------------------*/

void coin_lockout_w(int num,int on)
{
	if (num >= COIN_COUNTERS) return;

	coinlockedout[num] = on;
}


/*-------------------------------------------------
	coin_lockout_global_w - locks out all the coin
	inputs
-------------------------------------------------*/

void coin_lockout_global_w(int on)
{
	int i;

	for (i = 0; i < COIN_COUNTERS; i++)
	{
		coin_lockout_w(i,on);
	}
}



/***************************************************************************

	Generic NVRAM code

***************************************************************************/

/*-------------------------------------------------
	nvram_handler_generic_0fill - generic NVRAM
	with a 0 fill
-------------------------------------------------*/

void nvram_handler_generic_0fill(mame_file *file, int read_or_write)
{
	if (read_or_write)
		mame_fwrite(file, generic_nvram, generic_nvram_size);
	else if (file)
		mame_fread(file, generic_nvram, generic_nvram_size);
  else
		memset(generic_nvram, 0, generic_nvram_size);
}


/*-------------------------------------------------
	nvram_handler_generic_1fill - generic NVRAM
	with a 1 fill
-------------------------------------------------*/

void nvram_handler_generic_1fill(mame_file *file, int read_or_write)
{
	if (read_or_write)
		mame_fwrite(file, generic_nvram, generic_nvram_size);
	else if (file)
		mame_fread(file, generic_nvram, generic_nvram_size);
	else
		memset(generic_nvram, 0xff, generic_nvram_size);
}



/***************************************************************************

	Bitmap allocation/freeing code

***************************************************************************/

/*-------------------------------------------------
	bitmap_alloc_core
-------------------------------------------------*/

struct mame_bitmap *bitmap_alloc_core(int width,int height,int depth,int use_auto)
{
	struct mame_bitmap *bitmap;

	/* obsolete kludge: pass in negative depth to prevent orientation swapping */
	if (depth < 0)
		depth = -depth;

	/* verify it's a depth we can handle */
	if (depth != 8 && depth != 15 && depth != 16 && depth != 32)
	{
		log_cb(RETRO_LOG_ERROR, LOGPRE "osd_alloc_bitmap() unknown depth %d\n",depth);
		return NULL;
	}

	/* allocate memory for the bitmap struct */
	bitmap = use_auto ? auto_malloc(sizeof(struct mame_bitmap)) : malloc(sizeof(struct mame_bitmap));
	if (bitmap != NULL)
	{
		int i, rowlen, rdwidth, bitmapsize, linearraysize, pixelsize;
		unsigned char *bm;

		/* initialize the basic parameters */
		bitmap->depth = depth;
		bitmap->width = width;
		bitmap->height = height;

		/* determine pixel size in bytes */
		pixelsize = 1;
		if (depth == 15 || depth == 16)
			pixelsize = 2;
		else if (depth == 32)
			pixelsize = 4;

		/* round the width to a multiple of 8 */
		rdwidth = (width + 7) & ~7;
		rowlen = rdwidth + 2 * BITMAP_SAFETY;
		bitmap->rowpixels = rowlen;

		/* now convert from pixels to bytes */
		rowlen *= pixelsize;
		bitmap->rowbytes = rowlen;

		/* determine total memory for bitmap and line arrays */
		bitmapsize = (height + 2 * BITMAP_SAFETY) * rowlen;
		linearraysize = (height + 2 * BITMAP_SAFETY) * sizeof(unsigned char *);

		/* align to 16 bytes */
		linearraysize = (linearraysize + 15) & ~15;

		/* allocate the bitmap data plus an array of line pointers */
		bitmap->line = use_auto ? auto_malloc(linearraysize + bitmapsize) : malloc(linearraysize + bitmapsize);
		if (bitmap->line == NULL)
		{
			if (!use_auto) free(bitmap);
			return NULL;
		}

		/* clear ALL bitmap, including safety area, to avoid garbage on right */
		bm = (unsigned char *)bitmap->line + linearraysize;
		memset(bm, 0, (height + 2 * BITMAP_SAFETY) * rowlen);

		/* initialize the line pointers */
		for (i = 0; i < height + 2 * BITMAP_SAFETY; i++)
			bitmap->line[i] = &bm[i * rowlen + BITMAP_SAFETY * pixelsize];

		/* adjust for the safety rows */
		bitmap->line += BITMAP_SAFETY;
		bitmap->base = bitmap->line[0];

		/* set the pixel functions */
		set_pixel_functions(bitmap);
	}

	/* return the result */
	return bitmap;
}


/*-------------------------------------------------
	bitmap_alloc - allocate a bitmap at the
	current screen depth
-------------------------------------------------*/

struct mame_bitmap *bitmap_alloc(int width,int height)
{
	return bitmap_alloc_core(width,height,Machine->scrbitmap->depth,0);
}


/*-------------------------------------------------
	bitmap_alloc_depth - allocate a bitmap for a
	specific depth
-------------------------------------------------*/

struct mame_bitmap *bitmap_alloc_depth(int width,int height,int depth)
{
	return bitmap_alloc_core(width,height,depth,0);
}


/*-------------------------------------------------
	bitmap_free - free a bitmap
-------------------------------------------------*/

void bitmap_free(struct mame_bitmap *bitmap)
{
	/* skip if NULL */
	if (!bitmap)
		return;

	/* unadjust for the safety rows */
	bitmap->line -= BITMAP_SAFETY;

	/* free the memory */
	free(bitmap->line);
	free(bitmap);
}



/***************************************************************************

	Resource tracking code

***************************************************************************/

/*-------------------------------------------------
	auto_malloc - allocate auto-freeing memory
-------------------------------------------------*/

void *auto_malloc(size_t size)
{
	void *result = malloc(size);
	if (result)
	{
		struct malloc_info *info;

		/* make sure we have space */
		if (malloc_list_index >= MAX_MALLOCS)
		{
			log_cb(RETRO_LOG_ERROR, LOGPRE  "Out of malloc tracking slots!\n");
			return result;
		}

		/* fill in the current entry */
		info = &malloc_list[malloc_list_index++];
		info->tag = get_resource_tag();
		info->ptr = result;
	}
	return result;
}



/*-------------------------------------------------
	auto_strdup - allocate auto-freeing string
-------------------------------------------------*/

char *auto_strdup(const char *str)
{
	char *new_str = auto_malloc(strlen(str) + 1);
	if (!new_str)
		return NULL;
	strcpy(new_str, str);
	return new_str;
}



/*-------------------------------------------------
	end_resource_tracking - stop tracking
	resources
-------------------------------------------------*/

void auto_free(void)
{
	int tag = get_resource_tag();

	/* start at the end and free everything on the current tag */
	while (malloc_list_index > 0 && malloc_list[malloc_list_index - 1].tag >= tag)
	{
		struct malloc_info *info = &malloc_list[--malloc_list_index];
		free(info->ptr);
	}
}


/*-------------------------------------------------
	bitmap_alloc - allocate a bitmap at the
	current screen depth
-------------------------------------------------*/

struct mame_bitmap *auto_bitmap_alloc(int width,int height)
{
	return bitmap_alloc_core(width,height,Machine->scrbitmap->depth,1);
}


/*-------------------------------------------------
	bitmap_alloc_depth - allocate a bitmap for a
	specific depth
-------------------------------------------------*/

struct mame_bitmap *auto_bitmap_alloc_depth(int width,int height,int depth)
{
	return bitmap_alloc_core(width,height,depth,1);
}


/*-------------------------------------------------
	begin_resource_tracking - start tracking
	resources
-------------------------------------------------*/

void begin_resource_tracking(void)
{
	/* increment the tag counter */
	resource_tracking_tag++;
}


/*-------------------------------------------------
	end_resource_tracking - stop tracking
	resources
-------------------------------------------------*/

void end_resource_tracking(void)
{
	/* call everyone who tracks resources to let them know */
	auto_free();
	timer_free();

	/* decrement the tag counter */
	resource_tracking_tag--;
}

/***************************************************************************

	Hard disk handling

***************************************************************************/

struct chd_file *get_disk_handle(int diskindex)
{
	return disk_handle[diskindex];
}



/***************************************************************************

	ROM loading code

***************************************************************************/

/*-------------------------------------------------
	rom_first_region - return pointer to first ROM
	region
-------------------------------------------------*/

const struct RomModule *rom_first_region(const struct GameDriver *drv)
{
	return drv->rom;
}


/*-------------------------------------------------
	rom_next_region - return pointer to next ROM
	region
-------------------------------------------------*/

const struct RomModule *rom_next_region(const struct RomModule *romp)
{
	romp++;
	while (!ROMENTRY_ISREGIONEND(romp))
		romp++;
	return ROMENTRY_ISEND(romp) ? NULL : romp;
}


/*-------------------------------------------------
	rom_first_file - return pointer to first ROM
	file
-------------------------------------------------*/

const struct RomModule *rom_first_file(const struct RomModule *romp)
{
	romp++;
	while (!ROMENTRY_ISFILE(romp) && !ROMENTRY_ISREGIONEND(romp))
		romp++;
	return ROMENTRY_ISREGIONEND(romp) ? NULL : romp;
}


/*-------------------------------------------------
	rom_next_file - return pointer to next ROM
	file
-------------------------------------------------*/

const struct RomModule *rom_next_file(const struct RomModule *romp)
{
	romp++;
	while (!ROMENTRY_ISFILE(romp) && !ROMENTRY_ISREGIONEND(romp))
		romp++;
	return ROMENTRY_ISREGIONEND(romp) ? NULL : romp;
}


/*-------------------------------------------------
	rom_first_chunk - return pointer to first ROM
	chunk
-------------------------------------------------*/

const struct RomModule *rom_first_chunk(const struct RomModule *romp)
{
	return (ROMENTRY_ISFILE(romp)) ? romp : NULL;
}


/*-------------------------------------------------
	rom_next_chunk - return pointer to next ROM
	chunk
-------------------------------------------------*/

const struct RomModule *rom_next_chunk(const struct RomModule *romp)
{
	romp++;
	return (ROMENTRY_ISCONTINUE(romp)) ? romp : NULL;
}


/*-------------------------------------------------
	debugload - log data to a file
-------------------------------------------------*/



/*-------------------------------------------------
	determine_bios_rom - determine system_bios
	from SystemBios structure and options.bios
-------------------------------------------------*/

int determine_bios_rom(const struct SystemBios *bios)
{
	/* set to default */
	int bios_no = 0;

	/* Not system_bios_0 and options.bios is set  */
	if(bios && (options.bios != NULL))
	{
		/* Test for bios short names */
		while(!BIOSENTRY_ISEND(bios))
		{
			if(strcmp(bios->_name, options.bios) == 0)
			{
				log_cb(RETRO_LOG_INFO, LOGPRE "Using BIOS: %s\n", options.bios);
				bios_no = bios->value;
				break;
			}
			bios++;
		}

		if(string_is_empty(options.bios))
			log_cb(RETRO_LOG_INFO, LOGPRE "No matching BIOS found. Using default system BIOS.");
	}

	return bios_no;
}


/*-------------------------------------------------
	count_roms - counts the total number of ROMs
	that will need to be loaded
-------------------------------------------------*/

static int count_roms(const struct RomModule *romp)
{
	const struct RomModule *region, *rom;
	int count = 0;

	/* determine the correct biosset to load based on options.bios string */
	int this_bios = determine_bios_rom(Machine->gamedrv->bios);

	/* loop over regions, then over files */
	for (region = romp; region; region = rom_next_region(region))
		for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
			if (!ROM_GETBIOSFLAGS(romp) || (ROM_GETBIOSFLAGS(romp) == (this_bios+1))) /* alternate bios sets */
				count++;

	/* return the total count */
	return count;
}


/*-------------------------------------------------
	fill_random - fills an area of memory with
	random data
-------------------------------------------------*/

static void fill_random(UINT8 *base, UINT32 length)
{
	while (length--)
		*base++ = rand();
}


/*-------------------------------------------------
	handle_missing_file - handles error generation
	for missing files
-------------------------------------------------*/

static void handle_missing_file(struct rom_load_data *romdata, const struct RomModule *romp)
{
	/* optional files are okay */
	if (ROM_ISOPTIONAL(romp))
	{
		log_cb(RETRO_LOG_ERROR, LOGPRE  "OPTIONAL %-12s NOT FOUND\n", ROM_GETNAME(romp));
		romdata->warnings++;
	}

	/* no good dumps are okay */
	else if (ROM_NOGOODDUMP(romp))
	{
		log_cb(RETRO_LOG_INFO, LOGPRE "%-12s NOT FOUND (NO GOOD DUMP KNOWN)\n", ROM_GETNAME(romp));
		romdata->warnings++;
	}

	/* anything else is bad */
	else
	{
		log_cb(RETRO_LOG_ERROR, LOGPRE  "%-12s NOT FOUND\n", ROM_GETNAME(romp));
		romdata->errors++;
	}
}

/*-------------------------------------------------
	dump_wrong_and_correct_checksums - dump an
	error message containing the wrong and the
	correct checksums for a given ROM
-------------------------------------------------*/

static void dump_wrong_and_correct_checksums(struct rom_load_data* romdata, const char* hash, const char* acthash)
{
	unsigned i;
	char chksum[256];
	unsigned found_functions;
	unsigned wrong_functions;

	found_functions = hash_data_used_functions(hash) & hash_data_used_functions(acthash);

	hash_data_print(hash, found_functions, chksum);
	log_cb(RETRO_LOG_ERROR, LOGPRE "    EXPECTED: %s\n", chksum);

	/* We dump informations only of the functions for which MAME provided
		a correct checksum. Other functions we might have calculated are
		useless here */
	hash_data_print(acthash, found_functions, chksum);
	log_cb(RETRO_LOG_ERROR, LOGPRE "       FOUND: %s\n", chksum);

	/* For debugging purposes, we check if the checksums available in the
	   driver are correctly specified or not. This can be done by checking
	   the return value of one of the extract functions. Maybe we want to
	   activate this only in debug buils, but many developers only use
	   release builds, so I keep it as is for now. */
	wrong_functions = 0;
	for (i=0;i<HASH_NUM_FUNCTIONS;i++)
		if (hash_data_extract_printable_checksum(hash, 1<<i, chksum) == 2)
			wrong_functions |= 1<<i;

	if (wrong_functions)
	{
		for (i=0;i<HASH_NUM_FUNCTIONS;i++)
			if (wrong_functions & (1<<i))
			{
				sprintf(&romdata->errorbuf[strlen(romdata->errorbuf)],
					"\tInvalid %s checksum treated as 0 (check leading zeros)\n",
					hash_function_name(1<<i));

				romdata->warnings++;
			}
	}
}


/*-------------------------------------------------
	verify_length_and_hash - verify the length
	and hash signatures of a file
-------------------------------------------------*/

static void verify_length_and_hash(struct rom_load_data *romdata, const char *name, UINT32 explength, const char* hash)
{
	UINT32 actlength;
	const char* acthash;

	/* we've already complained if there is no file */
	if (!romdata->file)
		return;

	/* get the length and CRC from the file */
	actlength = mame_fsize(romdata->file);
	acthash = mame_fhash(romdata->file);

	/* verify length */
	if (explength != actlength)
	{
		log_cb(RETRO_LOG_ERROR, LOGPRE "%-12s WRONG LENGTH (expected: %08x found: %08x)\n", name, explength, actlength);
		romdata->warnings++;
	}

	/* If there is no good dump known, write it */
	if (hash_data_has_info(hash, HASH_INFO_NO_DUMP))
	{
			log_cb(RETRO_LOG_ERROR, LOGPRE "%-12s NO GOOD DUMP KNOWN\n", name);
		romdata->warnings++;
	}
	/* verify checksums */
	else if (!hash_data_is_equal(hash, acthash, 0))
	{
		/* otherwise, it's just bad */
		log_cb(RETRO_LOG_ERROR, LOGPRE "%-12s WRONG CHECKSUMS:\n", name);

		dump_wrong_and_correct_checksums(romdata, hash, acthash);

		romdata->warnings++;
	}
	/* If it matches, but it is actually a bad dump, write it */
	else if (hash_data_has_info(hash, HASH_INFO_BAD_DUMP))
	{
		log_cb(RETRO_LOG_ERROR, LOGPRE "%-12s ROM NEEDS REDUMP\n",name);
		romdata->warnings++;
	}
}


/*-------------------------------------------------
	display_rom_load_results - display the final
	results of ROM loading
-------------------------------------------------*/

static int display_rom_load_results(struct rom_load_data *romdata)
{
	int region;
  const char *missing_files = "Required files are missing, the game cannot be run.\n";
  const char *rom_warnings  = "Warnings flagged during ROM loading.\n";

  /* display either an error message or a warning message */
  if (romdata->errors)
  {
    extern int bailing;
    frontend_message_cb(missing_files, 300);
    bailing = 1;
    strcat(romdata->errorbuf, missing_files);
    log_cb(RETRO_LOG_ERROR, LOGPRE "%s", romdata->errorbuf);
  }
  else if (romdata->warnings)
  {
    frontend_message_cb(rom_warnings, 180);
    strcat(romdata->errorbuf, rom_warnings);
    log_cb(RETRO_LOG_WARN, LOGPRE "%s", romdata->errorbuf);
  }
  else
  {
    log_cb(RETRO_LOG_INFO, LOGPRE "Succesfully loaded ROMs.\n");
  }

	/* clean up any regions */
	if (romdata->errors)
		for (region = 0; region < MAX_MEMORY_REGIONS; region++)
			free_memory_region(region);

	/* return true if we had any errors */
	return (romdata->errors != 0);
}


/*-------------------------------------------------
	region_post_process - post-process a region,
	byte swapping and inverting data as necessary
-------------------------------------------------*/

static void region_post_process(struct rom_load_data *romdata, const struct RomModule *regiondata)
{
	int type = ROMREGION_GETTYPE(regiondata);
	int datawidth = ROMREGION_GETWIDTH(regiondata) / 8;
	int littleendian = ROMREGION_ISLITTLEENDIAN(regiondata);
	UINT8 *base;
	int i, j;

	log_cb(RETRO_LOG_DEBUG, LOGPRE "+ datawidth=%d little=%d\n", datawidth, littleendian);

	/* if this is a CPU region, override with the CPU width and endianness */
	if (type >= REGION_CPU1 && type < REGION_CPU1 + MAX_CPU)
	{
		int cputype = Machine->drv->cpu[type - REGION_CPU1].cpu_type;
		if (cputype != 0)
		{
			datawidth = cputype_databus_width(cputype) / 8;
			littleendian = (cputype_endianess(cputype) == CPU_IS_LE);
			log_cb(RETRO_LOG_DEBUG, LOGPRE "+ CPU region #%d: datawidth=%d little=%d\n", type - REGION_CPU1, datawidth, littleendian);
		}
	}

	/* if the region is inverted, do that now */
	if (ROMREGION_ISINVERTED(regiondata))
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "+ Inverting region\n");
		for (i = 0, base = romdata->regionbase; i < romdata->regionlength; i++)
			*base++ ^= 0xff;
	}

	/* swap the endianness if we need to */
#ifdef MSB_FIRST
	if (datawidth > 1 && littleendian)
#else
	if (datawidth > 1 && !littleendian)
#endif
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "+ Byte swapping region\n");
		for (i = 0, base = romdata->regionbase; i < romdata->regionlength; i += datawidth)
		{
			UINT8 temp[8];
			memcpy(temp, base, datawidth);
			for (j = datawidth - 1; j >= 0; j--)
				*base++ = temp[j];
		}
	}
}


/*-------------------------------------------------
	open_rom_file - open a ROM file, searching
	up the parent and loading by checksum
-------------------------------------------------*/

static int open_rom_file(struct rom_load_data *romdata, const struct RomModule *romp)
{
	const struct GameDriver *drv;

	++romdata->romsloaded;

	/* Attempt reading up the chain through the parents. It automatically also
	   attempts any kind of load by checksum supported by the archives. */
	romdata->file = NULL;
	for (drv = Machine->gamedrv; !romdata->file && drv; drv = drv->clone_of)
  {
		if (drv->name && *drv->name)
			romdata->file = mame_fopen_rom(drv->name, ROM_GETNAME(romp), ROM_GETHASHDATA(romp));
  }

	/* return the result */
	return (romdata->file != NULL);
}

/*-------------------------------------------------
	rom_fread - cheesy fread that fills with
	random data for a NULL file
-------------------------------------------------*/

static int rom_fread(struct rom_load_data *romdata, UINT8 *buffer, int length)
{
	/* files just pass through */
	if (romdata->file)
		return mame_fread(romdata->file, buffer, length);

	/* otherwise, fill with randomness */
	else
		fill_random(buffer, length);

	return length;
}


/*-------------------------------------------------
	read_rom_data - read ROM data for a single
	entry
-------------------------------------------------*/

static int read_rom_data(struct rom_load_data *romdata, const struct RomModule *romp)
{
	int datashift = ROM_GETBITSHIFT(romp);
	int datamask = ((1 << ROM_GETBITWIDTH(romp)) - 1) << datashift;
	int numbytes = ROM_GETLENGTH(romp);
	int groupsize = ROM_GETGROUPSIZE(romp);
	int skip = ROM_GETSKIPCOUNT(romp);
	int reversed = ROM_ISREVERSED(romp);
	int numgroups = (numbytes + groupsize - 1) / groupsize;
	UINT8 *base = romdata->regionbase + ROM_GETOFFSET(romp);
	int i;

	log_cb(RETRO_LOG_DEBUG, LOGPRE "Loading ROM data: offs=%X len=%X mask=%02X group=%d skip=%d reverse=%d\n", ROM_GETOFFSET(romp), numbytes, datamask, groupsize, skip, reversed);

	/* make sure the length was an even multiple of the group size */
	if (numbytes % groupsize != 0)
	{
		log_cb(RETRO_LOG_ERROR, LOGPRE "Error in RomModule definition: %s length not an even multiple of group size\n", ROM_GETNAME(romp));
		return -1;
	}

	/* make sure we only fill within the region space */
	if (ROM_GETOFFSET(romp) + numgroups * groupsize + (numgroups - 1) * skip > romdata->regionlength)
	{
		log_cb(RETRO_LOG_ERROR, LOGPRE "Error in RomModule definition: %s out of memory region space\n", ROM_GETNAME(romp));
		return -1;
	}

	/* make sure the length was valid */
	if (numbytes == 0)
	{
		log_cb(RETRO_LOG_ERROR, LOGPRE "Error in RomModule definition: %s has an invalid length\n", ROM_GETNAME(romp));
		return -1;
	}

	/* special case for simple loads */
	if (datamask == 0xff && (groupsize == 1 || !reversed) && skip == 0)
		return rom_fread(romdata, base, numbytes);

	/* chunky reads for complex loads */
	skip += groupsize;
	while (numbytes)
	{
		int evengroupcount = (sizeof(romdata->tempbuf) / groupsize) * groupsize;
		int bytesleft = (numbytes > evengroupcount) ? evengroupcount : numbytes;
		UINT8 *bufptr = romdata->tempbuf;

		/* read as much as we can */
		log_cb(RETRO_LOG_DEBUG, LOGPRE "  Reading %X bytes into buffer\n", bytesleft);
		if (rom_fread(romdata, romdata->tempbuf, bytesleft) != bytesleft)
			return 0;
		numbytes -= bytesleft;

		log_cb(RETRO_LOG_DEBUG, LOGPRE "  Copying to %08X\n", (int)base);

		/* unmasked cases */
		if (datamask == 0xff)
		{
			/* non-grouped data */
			if (groupsize == 1)
				for (i = 0; i < bytesleft; i++, base += skip)
					*base = *bufptr++;

			/* grouped data -- non-reversed case */
			else if (!reversed)
				while (bytesleft)
				{
					for (i = 0; i < groupsize && bytesleft; i++, bytesleft--)
						base[i] = *bufptr++;
					base += skip;
				}

			/* grouped data -- reversed case */
			else
				while (bytesleft)
				{
					for (i = groupsize - 1; i >= 0 && bytesleft; i--, bytesleft--)
						base[i] = *bufptr++;
					base += skip;
				}
		}

		/* masked cases */
		else
		{
			/* non-grouped data */
			if (groupsize == 1)
				for (i = 0; i < bytesleft; i++, base += skip)
					*base = (*base & ~datamask) | ((*bufptr++ << datashift) & datamask);

			/* grouped data -- non-reversed case */
			else if (!reversed)
				while (bytesleft)
				{
					for (i = 0; i < groupsize && bytesleft; i++, bytesleft--)
						base[i] = (base[i] & ~datamask) | ((*bufptr++ << datashift) & datamask);
					base += skip;
				}

			/* grouped data -- reversed case */
			else
				while (bytesleft)
				{
					for (i = groupsize - 1; i >= 0 && bytesleft; i--, bytesleft--)
						base[i] = (base[i] & ~datamask) | ((*bufptr++ << datashift) & datamask);
					base += skip;
				}
		}
	}
	log_cb(RETRO_LOG_INFO, LOGPRE "read_rom_data: All done\n");
	return ROM_GETLENGTH(romp);
}


/*-------------------------------------------------
	fill_rom_data - fill a region of ROM space
-------------------------------------------------*/

static int fill_rom_data(struct rom_load_data *romdata, const struct RomModule *romp)
{
	UINT32 numbytes = ROM_GETLENGTH(romp);
	UINT8 *base = romdata->regionbase + ROM_GETOFFSET(romp);

	/* make sure we fill within the region space */
	if (ROM_GETOFFSET(romp) + numbytes > romdata->regionlength)
	{
		log_cb(RETRO_LOG_ERROR, LOGPRE "Error in RomModule definition: FILL out of memory region space\n");
		return 0;
	}

	/* make sure the length was valid */
	if (numbytes == 0)
	{
		log_cb(RETRO_LOG_ERROR, LOGPRE "Error in RomModule definition: FILL has an invalid length\n");
		return 0;
	}

	/* fill the data (filling value is stored in place of the hashdata) */
	memset(base, (UINT32)ROM_GETHASHDATA(romp) & 0xff, numbytes);
	return 1;
}


/*-------------------------------------------------
	copy_rom_data - copy a region of ROM space
-------------------------------------------------*/

static int copy_rom_data(struct rom_load_data *romdata, const struct RomModule *romp)
{
	UINT8 *base = romdata->regionbase + ROM_GETOFFSET(romp);
	int srcregion = ROM_GETFLAGS(romp) >> 24;
	UINT32 numbytes = ROM_GETLENGTH(romp);
	UINT32 srcoffs = (UINT32)ROM_GETHASHDATA(romp);  /* srcoffset in place of hashdata */
	UINT8 *srcbase;

	/* make sure we copy within the region space */
	if (ROM_GETOFFSET(romp) + numbytes > romdata->regionlength)
	{
		log_cb(RETRO_LOG_ERROR, LOGPRE "Error in RomModule definition: COPY out of target memory region space\n");
		return 0;
	}

	/* make sure the length was valid */
	if (numbytes == 0)
	{
		log_cb(RETRO_LOG_ERROR, LOGPRE "Error in RomModule definition: COPY has an invalid length\n");
		return 0;
	}

	/* make sure the source was valid */
	srcbase = memory_region(srcregion);
	if (!srcbase)
	{
		log_cb(RETRO_LOG_ERROR, LOGPRE "Error in RomModule definition: COPY from an invalid region\n");
		return 0;
	}

	/* make sure we find within the region space */
	if (srcoffs + numbytes > memory_region_length(srcregion))
	{
		log_cb(RETRO_LOG_ERROR, LOGPRE "Error in RomModule definition: COPY out of source memory region space\n");
		return 0;
	}

	/* fill the data */
	memcpy(base, srcbase + srcoffs, numbytes);
	return 1;
}


/*-------------------------------------------------
	process_rom_entries - process all ROM entries
	for a region
-------------------------------------------------*/

static int process_rom_entries(struct rom_load_data *romdata, const struct RomModule *romp)
{
	UINT32 lastflags = 0;
	const struct RomModule *fallback_romp = romp;

	/* loop until we hit the end of this region */
	while (!ROMENTRY_ISREGIONEND(romp))
	{
		/* if this is a continue entry, it's invalid */
		if (ROMENTRY_ISCONTINUE(romp))
		{
			log_cb(RETRO_LOG_ERROR, LOGPRE "Error in RomModule definition: ROM_CONTINUE not preceded by ROM_LOAD\n");
			goto fatalerror;
		}

		/* if this is a reload entry, it's invalid */
		if (ROMENTRY_ISRELOAD(romp))
		{
			log_cb(RETRO_LOG_ERROR, LOGPRE "Error in RomModule definition: ROM_RELOAD not preceded by ROM_LOAD\n");
			goto fatalerror;
		}

		/* handle fills */
		if (ROMENTRY_ISFILL(romp))
		{
			if (!fill_rom_data(romdata, romp++))
				goto fatalerror;
		}

		/* handle copies */
		else if (ROMENTRY_ISCOPY(romp))
		{
			if (!copy_rom_data(romdata, romp++))
				goto fatalerror;
		}

		/* handle files */
		else if (ROMENTRY_ISFILE(romp))
		{
			if (!ROM_GETBIOSFLAGS(romp) || (ROM_GETBIOSFLAGS(romp) == (system_bios+1))) /* alternate bios sets */
			{
				const struct RomModule *baserom = romp;
				int explength = 0;

				/* open the file */
				log_cb(RETRO_LOG_INFO, LOGPRE "Opening ROM file: %s\n", ROM_GETNAME(romp));
				if (!open_rom_file(romdata, romp))
				{
					if (ROM_GETBIOSFLAGS(romp) == (system_bios+1))
					{
						log_cb(RETRO_LOG_WARN, LOGPRE "%s NOT FOUND! Attempt fallback to default bios.\n", ROM_GETNAME(romp));
						if (!open_rom_file(romdata, &fallback_romp[0])) /* try default bios instead */
							handle_missing_file(romdata, romp);
					}
					else
						handle_missing_file(romdata, romp);
				}

				/* loop until we run out of reloads */
				do
				{
					/* loop until we run out of continues */
					do
					{
						struct RomModule modified_romp = *romp++;
						int readresult;

						/* handle flag inheritance */
						if (!ROM_INHERITSFLAGS(&modified_romp))
							lastflags = modified_romp._flags;
						else
							modified_romp._flags = (modified_romp._flags & ~ROM_INHERITEDFLAGS) | lastflags;

						explength += ROM_GETLENGTH(&modified_romp);

						/* attempt to read using the modified entry */
						readresult = read_rom_data(romdata, &modified_romp);
						if (readresult == -1)
							goto fatalerror;
					}
					while (ROMENTRY_ISCONTINUE(romp));

					/* if this was the first use of this file, verify the length and CRC */
					if (baserom)
					{
						log_cb(RETRO_LOG_DEBUG, LOGPRE "Verifying length (%X) and checksums\n", explength);
						verify_length_and_hash(romdata, ROM_GETNAME(baserom), explength, ROM_GETHASHDATA(baserom));
						log_cb(RETRO_LOG_DEBUG, LOGPRE "Length and checksum verify finished\n");
					}

					/* reseek to the start and clear the baserom so we don't reverify */
					if (romdata->file)
						mame_fseek(romdata->file, 0, SEEK_SET);
					baserom = NULL;
					explength = 0;
				}
				while (ROMENTRY_ISRELOAD(romp));

				/* close the file */
				if (romdata->file)
				{
					log_cb(RETRO_LOG_DEBUG, LOGPRE "Closing ROM file\n");
					mame_fclose(romdata->file);
					romdata->file = NULL;
				}
			}
			else
			{
				romp++; /* skip over file */
			}
		}
	}
	return 1;

	/* error case */
fatalerror:
	if (romdata->file)
		mame_fclose(romdata->file);
	romdata->file = NULL;
	return 0;
}



/*-------------------------------------------------
	process_disk_entries - process all disk entries
	for a region
-------------------------------------------------*/

static int process_disk_entries(struct rom_load_data *romdata, const struct RomModule *romp)
{
	/* loop until we hit the end of this region */
	while (!ROMENTRY_ISREGIONEND(romp))
	{
		/* handle files */
		if (ROMENTRY_ISFILE(romp))
		{
			struct chd_file *source, *diff = NULL;
			struct chd_header header;
			char filename[1024], *c;
			char acthash[HASH_BUF_SIZE];
			int err;

			/* make the filename of the source */
			strcpy(filename, ROM_GETNAME(romp));
			c = strrchr(filename, '.');
			if (c)
				strcpy(c, ".chd");
			else
				strcat(filename, ".chd");

			/* first open the source drive */
			log_cb(RETRO_LOG_INFO, LOGPRE "Opening disk image: %s\n", filename);
			source = chd_open(filename, 0, NULL);
			if (!source)
			{
				if (chd_get_last_error() == CHDERR_UNSUPPORTED_VERSION)
					log_cb(RETRO_LOG_ERROR, LOGPRE "%-12s UNSUPPORTED CHD VERSION\n", filename);
				else
					log_cb(RETRO_LOG_ERROR, LOGPRE "%-12s NOT FOUND\n", filename);
				romdata->errors++;
				romp++;
				continue;
			}

			/* get the header and extract the MD5/SHA1 */
			header = *chd_get_header(source);
			hash_data_clear(acthash);
			hash_data_insert_binary_checksum(acthash, HASH_MD5, header.md5);
			hash_data_insert_binary_checksum(acthash, HASH_SHA1, header.sha1);

			/* verify the MD5 */
			if (!hash_data_is_equal(ROM_GETHASHDATA(romp), acthash, 0))
			{
				log_cb(RETRO_LOG_ERROR, LOGPRE "%-12s WRONG CHECKSUMS:\n", filename);
				dump_wrong_and_correct_checksums(romdata, ROM_GETHASHDATA(romp), acthash);
				romdata->warnings++;
			}

			/* if not read-only, make the diff file */
			if (!DISK_ISREADONLY(romp))
			{
				/* make the filename of the diff */
				strcpy(filename, ROM_GETNAME(romp));
				c = strrchr(filename, '.');
				if (c)
					strcpy(c, ".dif");
				else
					strcat(filename, ".dif");

				/* try to open the diff */
				log_cb(RETRO_LOG_INFO, LOGPRE "Opening differencing image: %s\n", filename);
				diff = chd_open(filename, 1, source);
				if (!diff)
				{
					/* didn't work; try creating it instead */
					log_cb(RETRO_LOG_INFO, LOGPRE "Creating differencing image: %s\n", filename);
					err = chd_create(filename, 0, 0, CHDCOMPRESSION_NONE, source);
					if (err != CHDERR_NONE)
					{
						if (chd_get_last_error() == CHDERR_UNSUPPORTED_VERSION)
							log_cb(RETRO_LOG_ERROR, LOGPRE "%-12s UNSUPPORTED CHD VERSION\n", filename);
						else
							log_cb(RETRO_LOG_ERROR, LOGPRE "%-12s: CAN'T CREATE DIFF FILE     Error code %s\n", filename, chd_error_text[err]);
						romdata->errors++;
						romp++;
						continue;
					}

					/* open the newly-created diff file */
					log_cb(RETRO_LOG_INFO, LOGPRE "Opening new differencing image: %s\n", filename);
					diff = chd_open(filename, 1, source);
					if (!diff)
					{
						if (chd_get_last_error() == CHDERR_UNSUPPORTED_VERSION)
							log_cb(RETRO_LOG_ERROR, LOGPRE "%-12s UNSUPPORTED CHD VERSION\n", filename);
						else
							log_cb(RETRO_LOG_ERROR, LOGPRE "%-12s: CAN'T OPEN DIFF FILE  Error code %s\n", filename, chd_error_text[err]);
						romdata->errors++;
						romp++;
						continue;
					}
				}
			}

			/* we're okay, set the handle */
			log_cb(RETRO_LOG_DEBUG, LOGPRE "Assigning to handle %d\n", DISK_GETINDEX(romp));
			disk_handle[DISK_GETINDEX(romp)] = DISK_ISREADONLY(romp) ? source : diff;
			romp++;
		}
	}
	return 1;
}


/*-------------------------------------------------
	rom_load - new, more flexible ROM
	loading system
-------------------------------------------------*/

int rom_load(const struct RomModule *romp)
{
	const struct RomModule *regionlist[REGION_MAX];
	const struct RomModule *region;
	static struct rom_load_data romdata;
	int regnum;

	/* reset the region list */
	for (regnum = 0;regnum < REGION_MAX;regnum++)
		regionlist[regnum] = NULL;

	/* reset the romdata struct */
	memset(&romdata, 0, sizeof(romdata));
	romdata.romstotal = count_roms(romp);

	/* reset the disk list */
	memset(disk_handle, 0, sizeof(disk_handle));

	/* determine the correct biosset to load based on options.bios string */
	system_bios = determine_bios_rom(Machine->gamedrv->bios);

	/* loop until we hit the end */
	for (region = romp, regnum = 0; region; region = rom_next_region(region), regnum++)
	{
		int regiontype = ROMREGION_GETTYPE(region);

		log_cb(RETRO_LOG_DEBUG, LOGPRE "Processing region %02X (length=%X)\n", regiontype, ROMREGION_GETLENGTH(region));

		/* the first entry must be a region */
		if (!ROMENTRY_ISREGION(region))
		{
			log_cb(RETRO_LOG_ERROR, LOGPRE "Error: missing ROM_REGION header\n");
			return 1;
		}

		/* if sound is disabled and it's a sound-only region, skip it */
		if (Machine->sample_rate == 0 && ROMREGION_ISSOUNDONLY(region))
			continue;

		/* allocate memory for the region */
		if (new_memory_region(regiontype, ROMREGION_GETLENGTH(region), ROMREGION_GETFLAGS(region)))
		{
			log_cb(RETRO_LOG_ERROR, LOGPRE "Error: unable to allocate memory for region %d\n", regiontype);
			return 1;
		}

		/* remember the base and length */
		romdata.regionlength = memory_region_length(regiontype);
		romdata.regionbase = memory_region(regiontype);
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Allocated %X bytes @ %08X\n", romdata.regionlength, (int)romdata.regionbase);

		/* clear the region if it's requested */
		if (ROMREGION_ISERASE(region))
			memset(romdata.regionbase, ROMREGION_GETERASEVAL(region), romdata.regionlength);

		/* or if it's sufficiently small (<= 4MB) */
		else if (romdata.regionlength <= 0x400000)
			memset(romdata.regionbase, 0, romdata.regionlength);

#ifdef MAME_DEBUG
		/* if we're debugging, fill region with random data to catch errors */
		else
			fill_random(romdata.regionbase, romdata.regionlength);
#endif

		/* now process the entries in the region */
		if (ROMREGION_ISROMDATA(region))
		{
			if (!process_rom_entries(&romdata, region + 1))
				return 1;
		}
		else if (ROMREGION_ISDISKDATA(region))
		{
			if (!process_disk_entries(&romdata, region + 1))
				return 1;
		}

		/* add this region to the list */
		if (regiontype < REGION_MAX)
			regionlist[regiontype] = region;
	}

	/* post-process the regions */
	for (regnum = 0; regnum < REGION_MAX; regnum++)
		if (regionlist[regnum])
		{
			log_cb(RETRO_LOG_DEBUG, LOGPRE "Post-processing region %02X\n", regnum);
			romdata.regionlength = memory_region_length(regnum);
			romdata.regionbase = memory_region(regnum);
			region_post_process(&romdata, regionlist[regnum]);
		}

	/* display the results and exit */
	return display_rom_load_results(&romdata);
}


/*-------------------------------------------------
	printromlist - print list of ROMs
-------------------------------------------------*/

void printromlist(const struct RomModule *romp,const char *basename)
{
	const struct RomModule *region, *rom, *chunk;
	char buf[512];

	if (!romp) return;

	printf("This is the list of the ROMs required for driver \"%s\".\n"
			"Name              Size       Checksum\n",basename);

	for (region = romp; region; region = rom_next_region(region))
	{
		for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
		{
			const char *name = ROM_GETNAME(rom);
			const char* hash = ROM_GETHASHDATA(rom);
			int length = -1; /* default is for disks! */

			if (ROMREGION_ISROMDATA(region))
			{
				length = 0;
				for (chunk = rom_first_chunk(rom); chunk; chunk = rom_next_chunk(chunk))
					length += ROM_GETLENGTH(chunk);
			}

			log_cb(RETRO_LOG_ERROR, LOGPRE "%-12s ", name);
			if (length >= 0)
				log_cb(RETRO_LOG_ERROR, LOGPRE "%7d",length);
				else
				log_cb(RETRO_LOG_ERROR, LOGPRE "       ");

			if (!hash_data_has_info(hash, HASH_INFO_NO_DUMP))
			{
				if (hash_data_has_info(hash, HASH_INFO_BAD_DUMP))
					log_cb(RETRO_LOG_ERROR, LOGPRE " BAD DUMP");

				hash_data_print(hash, 0, buf);
				log_cb(RETRO_LOG_ERROR, LOGPRE " %s", buf);
			}
			else
				log_cb(RETRO_LOG_ERROR, LOGPRE " NO GOOD DUMP KNOWN");

			log_cb(RETRO_LOG_ERROR, LOGPRE "\n");
		}
	}
}
