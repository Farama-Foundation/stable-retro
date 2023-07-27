/* Mednafen - Multi-system Emulator
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <string.h>

#include <boolean.h>
#include <map>

#include "mednafen.h"
#include "general.h"
#include "state.h"
#include <compat/msvc.h>

#define RLSB MDFNSTATE_RLSB	//0x80000000

int32_t smem_read(StateMem *st, void *buffer, uint32_t len)
{
   if ((len + st->loc) > st->len)
      return 0;

   memcpy(buffer, st->data + st->loc, len);
   st->loc += len;

   return(len);
}

int32_t smem_write(StateMem *st, void *buffer, uint32_t len)
{
   if ((len + st->loc) > st->malloced)
   {
      uint32_t newsize = (st->malloced >= 32768) ? st->malloced : (st->initial_malloc ? st->initial_malloc : 32768);

      while(newsize < (len + st->loc))
         newsize *= 2;

      // Don't realloc data_frontend memory
      if (st->data == st->data_frontend && st->data != NULL)
      {
         st->data = (uint8_t *)malloc(newsize);
         memcpy(st->data, st->data_frontend, st->malloced);
      }
      else
         st->data = (uint8_t *)realloc(st->data, newsize);

      st->malloced = newsize;
   }
   memcpy(st->data + st->loc, buffer, len);
   st->loc += len;

   if (st->loc > st->len)
      st->len = st->loc;

   return(len);
}

int32_t smem_putc(StateMem *st, int value)
{
   uint8_t tmpval = value;
   if(smem_write(st, &tmpval, 1) != 1)
      return(-1);
   return(1);
}

int32_t smem_seek(StateMem *st, uint32_t offset, int whence)
{
   switch(whence)
   {
      case SEEK_SET: st->loc = offset; break;
      case SEEK_END: st->loc = st->len - offset; break;
      case SEEK_CUR: st->loc += offset; break;
   }

   if(st->loc > st->len)
   {
      st->loc = st->len;
      return(-1);
   }

   if(st->loc < 0)
   {
      st->loc = 0;
      return(-1);
   }

   return(0);
}

int smem_write32le(StateMem *st, uint32_t b)
{
   uint8_t s[4];
   s[0]=b;
   s[1]=b>>8;
   s[2]=b>>16;
   s[3]=b>>24;
   return((smem_write(st, s, 4)<4)?0:4);
}

int smem_read32le(StateMem *st, uint32_t *b)
{
   uint8_t s[4];

   if(smem_read(st, s, 4) < 4)
      return(0);

   *b = s[0] | (s[1] << 8) | (s[2] << 16) | (s[3] << 24);

   return(4);
}

static void SubWrite(StateMem *st, const SFORMAT *sf)
{
   while(sf->size || sf->name)	// Size can sometimes be zero, so also check for the text name.  These two should both be zero only at the end of a struct.
   {
      if(!sf->size || !sf->data)
      {
         sf++;
         continue;
      }

      if(sf->size == ~0U)		/* Link to another struct.	*/
      {
         SubWrite(st, (const SFORMAT *)sf->data);

         sf++;
         continue;
      }

      int32_t bytesize = sf->size;
      uintptr_t p = (uintptr_t)sf->data;
      uint32 repcount = sf->repcount;
      const size_t repstride = sf->repstride;
      char nameo[1 + 255];
      const int slen = strlen(sf->name);

      memcpy(&nameo[1], sf->name, slen);
      nameo[0] = slen;

      smem_write(st, nameo, 1 + nameo[0]);
      smem_write32le(st, bytesize * (repcount + 1));

	do
	{
		// Special case for the evil bool type, to convert bool to 1-byte elements.
		if(!sf->type)
		{
			for(int32 bool_monster = 0; bool_monster < bytesize; bool_monster++)
			{
				uint8 tmp_bool = ((bool *)p)[bool_monster];
				smem_write(st, &tmp_bool, 1);
			}
		}
		else
		{
			smem_write(st, (void*)p, bytesize);
		}
	} while(p += repstride, repcount--);

      sf++;
   }
}

struct compare_cstr
{
 bool operator()(const char *s1, const char *s2) const
 {
  return(strcmp(s1, s2) < 0);
 }
};

typedef std::map<const char *, const SFORMAT *, compare_cstr> SFMap_t;

static void MakeSFMap(const SFORMAT *sf, SFMap_t &sfmap)
{
 while(sf->size || sf->name) // Size can sometimes be zero, so also check for the text name.  These two should both be zero only at the end of a struct.
 {
  if(!sf->size || !sf->data)
  {
   sf++;
   continue;
  }

  if(sf->size == ~0U)            /* Link to another SFORMAT structure. */
   MakeSFMap((const SFORMAT *)sf->data, sfmap);
  else
  {
   assert(sf->name);

   if(sfmap.find(sf->name) != sfmap.end())
    log_cb( RETRO_LOG_WARN, "Duplicate save state variable in internal emulator structures(CLUB THE PROGRAMMERS WITH BREADSTICKS): %s\n", sf->name);

   sfmap[sf->name] = sf;
  }

  sf++;
 }
}

static int ReadStateChunk(StateMem *st, const SFORMAT *sf, uint32 size)
{
	SFMap_t sfmap;

	MakeSFMap(sf, sfmap);

	int temp = st->loc;

	while (st->loc < (temp + size))
	{
		int where_are_we = st->loc;

		uint32_t recorded_size;	// In bytes
		uint8_t toa[1 + 256];	// Don't change to char unless cast toa[0] to unsigned to smem_read() and other places.

		if(smem_read(st, toa, 1) != 1)
			return(0);

		if(smem_read(st, toa + 1, toa[0]) != toa[0])
			return 0;

		toa[1 + toa[0]] = 0;

		smem_read32le(st, &recorded_size);

		SFMap_t::iterator sfmit;

		sfmit = sfmap.find((char *)toa + 1);

		if(sfmit != sfmap.end())
		{
			const SFORMAT *tmp = sfmit->second;

			if(recorded_size != tmp->size * (1 + tmp->repcount))
			{
				if(smem_seek(st, recorded_size, SEEK_CUR) < 0)
					return(0);
			}
			else
			{
				const auto type            = tmp->type;
				const uint32 expected_size = tmp->size;	// In bytes
				uintptr_t p                = (uintptr_t)tmp->data;
				uint32 repcount            = tmp->repcount;
				const size_t repstride     = tmp->repstride;

				do
				{
					smem_read(st, (void*)p, expected_size);

					if(!type)
					{
						// Converting downwards is necessary for the case of sizeof(bool) > 1
						for(int32 bool_monster = expected_size - 1; bool_monster >= 0; bool_monster--)
							((bool *)p)[bool_monster] = ((uint8 *)p)[bool_monster];
					}
				} while(p += repstride, repcount--);
			}
		}
		else
		{
			if(smem_seek(st, recorded_size, SEEK_CUR) < 0)
				return(0);
		}
	}

	return 1;
}

static int WriteStateChunk(StateMem *st, const char *sname, SFORMAT *sf)
{
	int32_t data_start_pos;
	int32_t end_pos;

	uint8_t sname_tmp[32];
	size_t sname_len = strlen(sname);

	memset(sname_tmp, 0, sizeof(sname_tmp));
        memcpy((char *)sname_tmp, sname, (sname_len < 32) ? sname_len : 32);

	smem_write(st, sname_tmp, 32);

	smem_write32le(st, 0);                // We'll come back and write this later.

	data_start_pos = st->loc;

	SubWrite(st, sf);

	end_pos = st->loc;

	smem_seek(st, data_start_pos - 4, SEEK_SET);
	smem_write32le(st, end_pos - data_start_pos);
	smem_seek(st, end_pos, SEEK_SET);

	return(end_pos - data_start_pos);
}

/* This function is called by the game driver(NES, GB, GBA) to save a state. */
static int MDFNSS_StateAction_internal( void *st_p, int load, int data_only, SSDescriptor *section)
{
	StateMem *st = (StateMem*)st_p;

	if ( load )
	{
		char sname[32];

		int found = 0;
		uint32_t tmp_size;
		uint32_t total = 0;

		while ( smem_read(st, (uint8_t *)sname, 32 ) == 32 )
		{
			int where_are_we = st->loc - 32;

			if(smem_read32le(st, &tmp_size) != 4)
				return(0);

			total += tmp_size + 32 + 4;

			// Yay, we found the section
			if ( !strncmp(sname, section->name, 32 ) )
			{
				if(!ReadStateChunk(st, section->sf, tmp_size))
					return(0);

				found = 1;
				break;
			}
			else
			{
				if ( smem_seek(st, tmp_size, SEEK_CUR ) < 0 )
					return(0);
			}
		}

		if ( smem_seek(st, -total, SEEK_CUR) < 0 )
			return(0);

		if( !found && !section->optional ) // Not found.  We are sad!
			return(0);
	}
	else
	{
		// Write all the chunks.
		if ( !WriteStateChunk(st, section->name, section->sf ) )
			return(0);
	}

	return(1);
}

int MDFNSS_StateAction(void *st_p, int load, int data_only, SFORMAT *sf, const char *name, bool optional)
{
   SSDescriptor love;
   StateMem *st  = (StateMem*)st_p;

   love.sf       = sf;
   love.name     = name;
   love.optional = optional;

   return(MDFNSS_StateAction_internal(st, load, 0, &love));
}

extern int LibRetro_StateAction( StateMem* sm, const unsigned load);

int MDFNSS_SaveSM(void *st_p, uint32_t ver, const void*, const void*, const void*)
{
	int success;
	uint8_t header[32];
	StateMem *st = (StateMem*)st_p;
	static const char *header_magic = "MDFNSVST";
	int neowidth = 0, neoheight = 0;

	// Write header.
	memset( header, 0, sizeof(header) );
	memcpy( header, header_magic, 8 );
	MDFN_en32lsb(header + 16, ver);
	MDFN_en32lsb(header + 24, neowidth);
	MDFN_en32lsb(header + 28, neoheight);
	smem_write(st, header, 32);

	// Call out to main save state function.
	success = LibRetro_StateAction( st, 0 /*SAVE*/);

	// Circle back and fill in the file size.
	uint32_t sizy = st->loc;
	smem_seek(st, 16 + 4, SEEK_SET);
	smem_write32le(st, sizy);

	// Success!
	return success;
}

int MDFNSS_LoadSM(void *st_p, uint32_t ver)
{
	uint8_t header[32];
	uint32_t stateversion;
	StateMem *st = (StateMem*)st_p;

	smem_read( st, header, 32 );

	// Invalid header?
	if ( memcmp( header, "MDFNSVST", 8 ) )
		return(0);

	// Unsupported state version?
	stateversion = MDFN_de32lsb( header + 16 );
	if ( stateversion < 0x900 )
		return(0);

	// Call out to main save state function.
	return LibRetro_StateAction( st, stateversion /*LOAD*/);
}
