/*	hiscore.c
**	generalized high score save/restore support
*/

#include <stdio.h>

#include "driver.h"
#include "hiscore.h"
#include "log.h"
#include "../precompile/hiscore_dat.h"

#define MAX_CONFIG_LINE_SIZE 48
#define HISCORE_SYNC_FILE_DELAY	1000

const char *db_filename = "hiscore.dat"; /* high score definition file */

static struct
{
	int hiscores_have_been_loaded;
	int hs_sync_delay;
	mame_file *hs_file;

	struct mem_range
	{
		UINT32 cpu, addr, num_bytes, start_value, end_value;
		struct mem_range *next;
	} *mem_range;
} state;

/*****************************************************************************/

static void copy_to_memory (int cpu, int addr, const UINT8 *source, int num_bytes)
{
	int i;
	for (i=0; i<num_bytes; i++)
	{
		cpunum_write_byte (cpu, addr+i, source[i]);
	}
}

static void copy_from_memory (int cpu, int addr, UINT8 *dest, int num_bytes)
{
	int i;
	for (i=0; i<num_bytes; i++)
	{
		dest[i] = cpunum_read_byte (cpu, addr+i);
	}
}

/*****************************************************************************/

/*	hexstr2num extracts and returns the value of a hexadecimal field from the
	character buffer pointed to by pString.

	When hexstr2num returns, *pString points to the character following
	the first non-hexadecimal digit, or NULL if an end-of-string marker
	(0x00) is encountered.

*/
static UINT32 hexstr2num (const char **pString)
{
	const char *string = *pString;
	UINT32 result = 0;
	if (string)
	{
		for(;;)
		{
			char c = *string++;
			int digit;

			if (c>='0' && c<='9')
			{
				digit = c-'0';
			}
			else if (c>='a' && c<='f')
			{
				digit = 10+c-'a';
			}
			else if (c>='A' && c<='F')
			{
				digit = 10+c-'A';
			}
			else
			{
				/* not a hexadecimal digit */
				/* safety check for premature EOL */
				if (!c) string = NULL;
				break;
			}
			result = result*16 + digit;
		}
		*pString = string;
	}
	return result;
}

/*	given a line in the hiscore.dat file, determine if it encodes a
	memory range (or a game name).
	For now we assume that CPU number is always a decimal digit, and
	that no game name starts with a decimal digit.
*/
static int is_mem_range (const char *pBuf)
{
	char c;
	for(;;)
	{
		c = *pBuf++;
		if (c == 0) return 0; /* premature EOL */
		if (c == ':') break;
	}
	c = *pBuf; /* character following first ':' */

	return	(c>='0' && c<='9') ||
			(c>='a' && c<='f') ||
			(c>='A' && c<='F');
}

/*	matching_game_name is used to skip over lines until we find <gamename>: */
static int matching_game_name (const char *pBuf, const char *name)
{
	while (*name)
	{
		if (*name++ != *pBuf++) return 0;
	}
	return (*pBuf == ':');
}

/*****************************************************************************/

/* safe_to_load checks the start and end values of each memory range */
static int safe_to_load (void)
{
	struct mem_range *mem_range = state.mem_range;
	while (mem_range)
	{
		if (cpunum_read_byte (mem_range->cpu, mem_range->addr) !=
			mem_range->start_value)
		{
			return 0;
		}
		if (cpunum_read_byte (mem_range->cpu, mem_range->addr + mem_range->num_bytes - 1) !=
			mem_range->end_value)
		{
			return 0;
		}
		mem_range = mem_range->next;
	}
	return 1;
}

/* hs_free disposes of the mem_range linked list */
static void hs_free (void)
{
	struct mem_range *mem_range = state.mem_range;
	while (mem_range)
	{
		struct mem_range *next = mem_range->next;
		free (mem_range);
		mem_range = next;
	}
	state.mem_range = NULL;
}

static void hs_load (void)
{
	mame_file *f = mame_fopen (Machine->gamedrv->name, 0, FILETYPE_HIGHSCORE, 0);
	state.hiscores_have_been_loaded = 1;
	state.hs_sync_delay = HISCORE_SYNC_FILE_DELAY;
    
	if (f)
	{
		struct mem_range *mem_range = state.mem_range;
		log_cb(RETRO_LOG_INFO, LOGPRE "loading %s.hi hiscore memory file...\n", Machine->gamedrv->name);
        
		while (mem_range)
		{
			UINT8 *data = malloc (mem_range->num_bytes);
			if (data)
			{
				/*	this buffer will almost certainly be small
					enough to be dynamically allocated, but let's
					avoid memory trashing just in case
				*/
				if (mem_range->num_bytes == mame_fread (f, data, mem_range->num_bytes))
				{
					copy_to_memory (mem_range->cpu, mem_range->addr, data, mem_range->num_bytes);
				}
				free (data);
			}
			mem_range = mem_range->next;
		}
		mame_fclose(f);
	}
}

static void hs_save (void)
{
	/* bail if the core option has changed and is now disabled */
	if (!options.autosave_hiscore) return;

	if (!state.hs_file)
	{
		state.hs_file = mame_fopen (Machine->gamedrv->name, 0, FILETYPE_HIGHSCORE, 1);
	}

	if (state.hs_file)
	{
		struct mem_range *mem_range = state.mem_range;
		log_cb(RETRO_LOG_INFO, LOGPRE "saving %s.hi hiscore memory file...\n", Machine->gamedrv->name);
		mame_fseek(state.hs_file, 0, SEEK_SET);
		while (mem_range)
		{
			UINT8 *data = malloc (mem_range->num_bytes);
			if (data)
			{
				/*	this buffer will almost certainly be small
					enough to be dynamically allocated, but let's
					avoid memory trashing just in case
				*/
				copy_from_memory (mem_range->cpu, mem_range->addr, data, mem_range->num_bytes);
				mame_fwrite(state.hs_file, data, mem_range->num_bytes);
			}
			mem_range = mem_range->next;
		}
	}
}

/*****************************************************************************/
/* public API */

/* call hs_open once after loading a game */
void hs_open (const char *name)
{   
	char buffer[MAX_CONFIG_LINE_SIZE];
	enum { FIND_NAME, FIND_DATA, FETCH_DATA } mode;
	mame_file *db_file = NULL;

	state.mem_range = NULL;
	mode = FIND_NAME;

	/* Core option to disable hiscore implementation */
	if (!options.autosave_hiscore)
	{
		log_cb(RETRO_LOG_INFO, LOGPRE "hiscore implementation has been disabled via core option\n");
		return;
	}

	db_file = mame_fopen(NULL, db_filename, FILETYPE_HIGHSCORE_DB, 0);

	if(!db_file)
	{
		log_cb(RETRO_LOG_INFO, LOGPRE "hiscore.dat not found: generating new hiscore.dat\n");
		db_file = mame_fopen(NULL, db_filename, FILETYPE_HIGHSCORE_DB, 1);
		mame_fwrite(db_file, hiscoredat_bytes, hiscoredat_length); 
		mame_fclose(db_file);

		db_file = mame_fopen(NULL, db_filename, FILETYPE_HIGHSCORE_DB, 0);
		if(!db_file)
		{
			log_cb(RETRO_LOG_ERROR, LOGPRE "Failure generating hiscore.dat!\n");
			return;
		}
	}

	while (mame_fgets (buffer, MAX_CONFIG_LINE_SIZE, db_file))
	{
		if (mode==FIND_NAME)
		{
			if (matching_game_name (buffer, name))
			{
				mode = FIND_DATA;
				log_cb(RETRO_LOG_INFO, LOGPRE "%s hiscore memory map found in hiscore.dat!\n", name);
			}
		}
		else if (is_mem_range (buffer))
		{
			const char            *pBuf = buffer;
			struct mem_range *mem_range = (struct mem_range*)
				malloc(sizeof(struct mem_range));

			if (mem_range)
			{
				mem_range->cpu = hexstr2num (&pBuf);
				mem_range->addr = hexstr2num (&pBuf);
				mem_range->num_bytes = hexstr2num (&pBuf);
				mem_range->start_value = hexstr2num (&pBuf);
				mem_range->end_value = hexstr2num (&pBuf);

				mem_range->next = NULL;
				{
					struct mem_range *last = state.mem_range;
					while (last && last->next) last = last->next;
					if (last == NULL)
						state.mem_range = mem_range;
					else
						last->next = mem_range;
				}

				mode = FETCH_DATA;
			}
			else
			{
				hs_free();
				break;
			}
		}
		else
		{
			/* line is a game name */
			if (mode == FETCH_DATA)
				break;
		}
	}
	mame_fclose(db_file);
}


/* call hs_init when emulation starts, and when the game is reset */
void hs_init (void)
{
	struct mem_range *mem_range = state.mem_range;
	state.hiscores_have_been_loaded = 0;

	while (mem_range)
	{
		cpunum_write_byte(
			mem_range->cpu,
			mem_range->addr,
			~mem_range->start_value
		);

		cpunum_write_byte(
			mem_range->cpu,
			mem_range->addr + mem_range->num_bytes-1,
			~mem_range->end_value
		);
		mem_range = mem_range->next;
	}
}

/* call hs_update periodically (i.e. once per frame) */
void hs_update (void)
{
	if (state.mem_range)
	{
		if (!state.hiscores_have_been_loaded)
		{
			if (safe_to_load())
				hs_load();
		}
		else if (options.autosave_hiscore == 2)
		{
			if (state.hs_sync_delay-- <= 0)
			{
				hs_save();
				state.hs_sync_delay = HISCORE_SYNC_FILE_DELAY;
			}
		}
	}
}

/* call hs_close when done playing game */
void hs_close (void)
{
	if (state.hiscores_have_been_loaded)
	{
		hs_save();

		if (state.hs_file)
		{
			mame_fclose(state.hs_file);
			state.hs_file = NULL;
		}
	}

	hs_free();
}
