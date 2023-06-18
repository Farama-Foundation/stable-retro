/****************************************************************************
 *      datafile.c
 *      History database engine
 *
 *      Token parsing by Neil Bradley
 *      Modifications and higher-level functions by John Butler
 ****************************************************************************/

#include <assert.h>
#include <ctype.h>

#include "osd_cpu.h"
#include "driver.h"
#include "datafile.h"

/****************************************************************************
 *      token parsing constants
 ****************************************************************************/
#define CR      0x0d    /* '\n' and '\r' meanings are swapped in some */
#define LF      0x0a    /*     compilers (e.g., Mac compilers) */

enum
{
        TOKEN_COMMA,
        TOKEN_EQUALS,
        TOKEN_SYMBOL,
        TOKEN_LINEBREAK,
        TOKEN_INVALID=-1
};

#define MAX_TOKEN_LENGTH        256


/****************************************************************************
 *      datafile constants
 ****************************************************************************/
#define MAX_DATAFILE_ENTRIES 5000
#define DATAFILE_TAG '$'

const char *DATAFILE_TAG_KEY = "$info";
const char *DATAFILE_TAG_BIO = "$bio";
const char *DATAFILE_TAG_MAME = "$mame";

const char *history_filename = NULL;
const char *mameinfo_filename = NULL;


/****************************************************************************
 *      private data for parsing functions
 ****************************************************************************/
static mame_file *fp;                                       /* Our file pointer */
static long dwFilePos;                                          /* file position */
static UINT8 bToken[MAX_TOKEN_LENGTH];          /* Our current token */

/* an array of driver name/drivers array index sorted by driver name
   for fast look up by name */
typedef struct
{
    const char *name;
    int index;
} driver_data_type;
static driver_data_type *sorted_drivers = NULL;
static int num_games;


/**************************************************************************
 **************************************************************************
 *
 *              Parsing functions
 *
 **************************************************************************
 **************************************************************************/

/*
 * DriverDataCompareFunc -- compare function for GetGameNameIndex
 */
static int CLIB_DECL DriverDataCompareFunc(const void *arg1,const void *arg2)
{
    return strcmp( ((driver_data_type *)arg1)->name, ((driver_data_type *)arg2)->name );
}

/*
 * GetGameNameIndex -- given a driver name (in lowercase), return
 * its index in the main drivers[] array, or -1 if it's not found.
 */
static int GetGameNameIndex(const char *name)
{
    driver_data_type *driver_index_info;
	driver_data_type key;
	key.name = name;

	if (sorted_drivers == NULL)
	{
		/* initialize array of game names/indices */
		int i;
		num_games = 0;
		while (drivers[num_games] != NULL)
			num_games++;

		sorted_drivers = (driver_data_type *)malloc(sizeof(driver_data_type) * num_games);
		for (i=0;i<num_games;i++)
		{
			sorted_drivers[i].name = drivers[i]->name;
			sorted_drivers[i].index = i;
		}
		qsort(sorted_drivers,num_games,sizeof(driver_data_type),DriverDataCompareFunc);
	}

	/* uses our sorted array of driver names to get the index in log time */
	driver_index_info = bsearch(&key,sorted_drivers,num_games,sizeof(driver_data_type),
								DriverDataCompareFunc);

	if (driver_index_info == NULL)
		return -1;

	return driver_index_info->index;

}

/****************************************************************************
 *      GetNextToken - Pointer to the token string pointer
 *                                 Pointer to position within file
 *
 *      Returns token, or TOKEN_INVALID if at end of file
 ****************************************************************************/
static UINT32 GetNextToken(UINT8 **ppszTokenText, long *pdwPosition)
{
        UINT32 dwLength;                                                /* Length of symbol */
        long dwPos;                                                             /* Temporary position */
        UINT8 *pbTokenPtr = bToken;                             /* Point to the beginning */
        UINT8 bData;                                                    /* Temporary data byte */

        while (1)
        {
                bData = mame_fgetc(fp);                                  /* Get next character */

                /* If we're at the end of the file, bail out */

                if (mame_feof(fp))
                        return(TOKEN_INVALID);

                /* If it's not whitespace, then let's start eating characters */

                if (' ' != bData && '\t' != bData)
                {
                        /* Store away our file position (if given on input) */

                        if (pdwPosition)
                                *pdwPosition = dwFilePos;

                        /* If it's a separator, special case it */

                        if (',' == bData || '=' == bData)
                        {
                                *pbTokenPtr++ = bData;
                                *pbTokenPtr = '\0';
                                ++dwFilePos;

                                if (',' == bData)
                                        return(TOKEN_COMMA);
                                else
                                        return(TOKEN_EQUALS);
                        }

                        /* Otherwise, let's try for a symbol */

                        if (bData > ' ')
                        {
                                dwLength = 0;                   /* Assume we're 0 length to start with */

                                /* Loop until we've hit something we don't understand */

                                while (bData != ',' &&
                                                 bData != '=' &&
                                                 bData != ' ' &&
                                                 bData != '\t' &&
                                                 bData != '\n' &&
                                                 bData != '\r' &&
                                                 mame_feof(fp) == 0)
                                {
                                        ++dwFilePos;
                                        *pbTokenPtr++ = bData;  /* Store our byte */
                                        ++dwLength;
                                        assert(dwLength < MAX_TOKEN_LENGTH);
                                        bData = mame_fgetc(fp);
                                }

                                /* If it's not the end of the file, put the last received byte */
                                /* back. We don't want to touch the file position, though if */
                                /* we're past the end of the file. Otherwise, adjust it. */

                                if (0 == mame_feof(fp))
                                {
                                        mame_ungetc(bData, fp);
                                }

                                /* Null terminate the token */

                                *pbTokenPtr = '\0';

                                /* Connect up the */

                                if (ppszTokenText)
                                        *ppszTokenText = bToken;

                                return(TOKEN_SYMBOL);
                        }

                        /* Not a symbol. Let's see if it's a cr/cr, lf/lf, or cr/lf/cr/lf */
                        /* sequence */

                        if (LF == bData)
                        {
                                /* Unix style perhaps? */

                                bData = mame_fgetc(fp);          /* Peek ahead */
                                mame_ungetc(bData, fp);          /* Force a retrigger if subsequent LF's */

                                if (LF == bData)                /* Two LF's in a row - it's a UNIX hard CR */
                                {
                                        ++dwFilePos;
                                        *pbTokenPtr++ = bData;  /* A real linefeed */
                                        *pbTokenPtr = '\0';
                                        return(TOKEN_LINEBREAK);
                                }

                                /* Otherwise, fall through and keep parsing. */

                        }
                        else
                        if (CR == bData)                /* Carriage return? */
                        {
                                /* Figure out if it's Mac or MSDOS format */

                                ++dwFilePos;
                                bData = mame_fgetc(fp);          /* Peek ahead */

                                /* We don't need to bother with EOF checking. It will be 0xff if */
                                /* it's the end of the file and will be caught by the outer loop. */

                                if (CR == bData)                /* Mac style hard return! */
                                {
                                        /* Do not advance the file pointer in case there are successive */
                                        /* CR/CR sequences */

                                        /* Stuff our character back upstream for successive CR's */

                                        mame_ungetc(bData, fp);

                                        *pbTokenPtr++ = bData;  /* A real carriage return (hard) */
                                        *pbTokenPtr = '\0';
                                        return(TOKEN_LINEBREAK);
                                }
                                else
                                if (LF == bData)        /* MSDOS format! */
                                {
                                        ++dwFilePos;                    /* Our file position to reset to */
                                        dwPos = dwFilePos;              /* Here so we can reposition things */

                                        /* Look for a followup CR/LF */

                                        bData = mame_fgetc(fp);  /* Get the next byte */

                                        if (CR == bData)        /* CR! Good! */
                                        {
                                                bData = mame_fgetc(fp);  /* Get the next byte */

                                                /* We need to do this to pick up subsequent CR/LF sequences */

                                                mame_fseek(fp, dwPos, SEEK_SET);

                                                if (pdwPosition)
                                                        *pdwPosition = dwPos;

                                                if (LF == bData)        /* LF? Good! */
                                                {
                                                        *pbTokenPtr++ = '\r';
                                                        *pbTokenPtr++ = '\n';
                                                        *pbTokenPtr = '\0';

                                                        return(TOKEN_LINEBREAK);
                                                }
                                        }
                                        else
                                        {
                                                --dwFilePos;
                                                mame_ungetc(bData, fp);  /* Put the character back. No good */
                                        }
                                }
                                else
                                {
                                        --dwFilePos;
                                        mame_ungetc(bData, fp);  /* Put the character back. No good */
                                }

                                /* Otherwise, fall through and keep parsing */
                        }
                }

                ++dwFilePos;
        }
}


/****************************************************************************
 *      ParseClose - Closes the existing opened file (if any)
 ****************************************************************************/
static void ParseClose(void)
{
        /* If the file is open, time for fclose. */

        if (fp)
        {
                mame_fclose(fp);
        }

        fp = NULL;
}


/****************************************************************************
 *      ParseOpen - Open up file for reading
 ****************************************************************************/
static UINT8 ParseOpen(const char *pszFilename)
{
   /* Open file up in binary mode */
   fp = mame_fopen (NULL, pszFilename, FILETYPE_HISTORY, 0);

   /* If this is NULL, return FALSE. We can't open it */

   if (!fp)
      return 0;

   /* Otherwise, prepare! */

   dwFilePos = 0;
   return 1;
}


/****************************************************************************
 *      ParseSeek - Move the file position indicator
 ****************************************************************************/
static UINT8 ParseSeek(long offset, int whence)
{
   int result = mame_fseek(fp, offset, whence);

   if (0 == result)
      dwFilePos = mame_ftell(fp);
   return (UINT8)result;
}



/**************************************************************************
 **************************************************************************
 *
 *              Datafile functions
 *
 **************************************************************************
 **************************************************************************/


/**************************************************************************
 *      ci_strcmp - case insensitive string compare
 *
 *      Returns zero if s1 and s2 are equal, ignoring case
 **************************************************************************/
static int ci_strcmp (const char *s1, const char *s2)
{
        int c1, c2;

        while ((c1 = tolower(*s1)) == (c2 = tolower(*s2)))
        {
                if (!c1)
                        return 0;

                s1++;
                s2++;
        }

        return (c1 - c2);
}


/**************************************************************************
 *      ci_strncmp - case insensitive character array compare
 *
 *      Returns zero if the first n characters of s1 and s2 are equal,
 *      ignoring case.
 **************************************************************************/
static int ci_strncmp (const char *s1, const char *s2, int n)
{
        int c1, c2;

        while (n)
        {
                if ((c1 = tolower (*s1)) != (c2 = tolower (*s2)))
                        return (c1 - c2);
                else if (!c1)
                        break;
                --n;

                s1++;
                s2++;
        }
        return 0;
}


/**************************************************************************
 *      index_datafile
 *      Create an index for the records in the currently open datafile.
 *
 *      Returns 0 on error, or the number of index entries created.
 **************************************************************************/
static int index_datafile (struct tDatafileIndex **_index)
{
        struct tDatafileIndex *idx;
        int count = 0;
        UINT32 token = TOKEN_SYMBOL;

        /* rewind file */
        if (ParseSeek (0L, SEEK_SET)) return 0;

        /* allocate index */
        idx = *_index = malloc (MAX_DATAFILE_ENTRIES * sizeof (struct tDatafileIndex));
        if (NULL == idx) return 0;

        /* loop through datafile */
        while ((count < (MAX_DATAFILE_ENTRIES - 1)) && TOKEN_INVALID != token)
        {
                long tell;
                char *s;

                token = GetNextToken ((UINT8 **)&s, &tell);
                if (TOKEN_SYMBOL != token) continue;

                /* DATAFILE_TAG_KEY identifies the driver */
                if (!ci_strncmp (DATAFILE_TAG_KEY, s, strlen (DATAFILE_TAG_KEY)))
                {
                        token = GetNextToken ((UINT8 **)&s, &tell);
                        if (TOKEN_EQUALS == token)
                        {
                                int done = 0;

                                token = GetNextToken ((UINT8 **)&s, &tell);
                                while (!done && TOKEN_SYMBOL == token)
                                {
									int game_index;
									char *p;
									for (p = s; *p; p++)
										*p = tolower(*p);

									game_index = GetGameNameIndex(s);
									if (game_index >= 0)
									{
										idx->driver = drivers[game_index];
										idx->offset = tell;
										idx++;
										count++;
										/* done = 1;  Not done, as we must process other clones in list */

									}
									if (!done)
									{
										token = GetNextToken ((UINT8 **)&s, &tell);

										if (TOKEN_COMMA == token)
											token = GetNextToken ((UINT8 **)&s, &tell);
										else
											done = 1; /* end of key field */
									}
                                }
                        }
                }
        }

        /* mark end of index */
        idx->offset = 0L;
        idx->driver = 0;
        return count;
}


/**************************************************************************
 *      load_datafile_text
 *
 *      Loads text field for a driver into the buffer specified. Specify the
 *      driver, a pointer to the buffer, the buffer size, the index created by
 *      index_datafile(), and the desired text field (e.g., DATAFILE_TAG_BIO).
 *
 *      Returns 0 if successful.
 **************************************************************************/
static int load_datafile_text (const struct GameDriver *drv, char *buffer, int bufsize,
        struct tDatafileIndex *idx, const char *tag)
{
        int     offset = 0;
        int found = 0;
        UINT32  token = TOKEN_SYMBOL;
        UINT32  prev_token = TOKEN_SYMBOL;

        *buffer = '\0';

        /* find driver in datafile index */
        while (idx->driver)
        {

                if (idx->driver == drv) break;

                idx++;
        }
        if (idx->driver == 0) return 1; /* driver not found in index */

        /* seek to correct point in datafile */
        if (ParseSeek (idx->offset, SEEK_SET)) return 1;

        /* read text until buffer is full or end of entry is encountered */
        while (TOKEN_INVALID != token)
        {
                char *s;
                int len;
                long tell;

                token = GetNextToken ((UINT8 **)&s, &tell);
                if (TOKEN_INVALID == token) continue;

                if (found)
                {
                        /* end entry when a tag is encountered */
                        if (TOKEN_SYMBOL == token && DATAFILE_TAG == s[0] && TOKEN_LINEBREAK == prev_token) break;

                        prev_token = token;

                        /* translate platform-specific linebreaks to '\n' */
                        if (TOKEN_LINEBREAK == token)
                                strcpy (s, "\n");

                        /* append a space to words */
                        if (TOKEN_LINEBREAK != token)
                                strcat (s, " ");

                        /* remove extraneous space before commas */
                        if (TOKEN_COMMA == token)
                        {
                                --buffer;
                                --offset;
                                *buffer = '\0';
                        }

                        /* Get length of text to add to the buffer */
                        len = strlen (s);

                        /* Check for buffer overflow */
                        /* For some reason we can get a crash if we try */
                        /* to use the last 30 characters of the buffer  */
                        if ((bufsize - offset) - len <= 45)
                        {
                            strcpy (s, " ...[TRUNCATED]");
                            len = strlen(s);
                            strcpy (buffer, s);
                            buffer += len;
                            offset += len;
                            break;
                        }

                        /* add this word to the buffer */
                        strcpy (buffer, s);
                        buffer += len;
                        offset += len;
                }
                else
                {
                        if (TOKEN_SYMBOL == token)
                        {
                                /* looking for requested tag */
                                if (!ci_strncmp (tag, s, strlen (tag)))
                                        found = 1;
                                else if (!ci_strncmp (DATAFILE_TAG_KEY, s, strlen (DATAFILE_TAG_KEY)))
                                        break; /* error: tag missing */
                        }
                }
        }
        return (!found);
}


/**************************************************************************
 *      load_driver_history
 *      Load history text for the specified driver into the specified buffer.
 *      Combines $bio field of HISTORY.DAT with $mame field of MAMEINFO.DAT.
 *
 *      Returns 0 if successful.
 *
 *      NOTE: For efficiency the indices are never freed (intentional leak).
 **************************************************************************/
int load_driver_history (const struct GameDriver *drv, char *buffer, int bufsize)
{
        static struct tDatafileIndex *hist_idx = 0;
        static struct tDatafileIndex *mame_idx = 0;
        int history = 0, mameinfo = 0;
        int err;

        *buffer = 0;


        if(!history_filename)
                history_filename = "history.dat";

        /* try to open history datafile */
        if (ParseOpen (history_filename))
        {
                /* create index if necessary */
                if (hist_idx)
                        history = 1;
                else
                        history = (index_datafile (&hist_idx) != 0);

                /* load history text */
                if (hist_idx)
                {
                        const struct GameDriver *gdrv;

                        gdrv = drv;
                        do
                        {
                                err = load_datafile_text (gdrv, buffer, bufsize,
                                                                                  hist_idx, DATAFILE_TAG_BIO);
                                gdrv = gdrv->clone_of;
                        } while (err && gdrv);

                        if (err) history = 0;
                }
                ParseClose ();
        }

        if(!mameinfo_filename)
                mameinfo_filename = "mameinfo.dat";

        /* try to open mameinfo datafile */
        if (ParseOpen (mameinfo_filename))
        {
                /* create index if necessary */
                if (mame_idx)
                        mameinfo = 1;
                else
                        mameinfo = (index_datafile (&mame_idx) != 0);

                /* load informational text (append) */
                if (mame_idx)
                {
                        int len = strlen (buffer);
                        const struct GameDriver *gdrv;

                        gdrv = drv;
                        do
                        {
                                err = load_datafile_text (gdrv, buffer+len, bufsize-len,
                                                                                  mame_idx, DATAFILE_TAG_MAME);
                                gdrv = gdrv->clone_of;
                        } while (err && gdrv);

                        if (err) mameinfo = 0;
                }
                ParseClose ();
        }

        return (history == 0 && mameinfo == 0);
}
