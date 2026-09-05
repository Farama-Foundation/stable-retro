/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
/* wad.c */

#include "common.h"
#include "quakedef.h"
#include "sys.h"
#include "wad.h"

int wad_numlumps;
lumpinfo_t *wad_lumps;
byte *wad_base;

void SwapPic(qpic_t *pic);

/*
==================
W_CleanupName

Lowercases name and pads with spaces and a terminating 0 to the length of
lumpinfo_t->name.
Used so lumpname lookups can proceed rapidly by comparing 4 chars at a time
Space padding is so names can be printed nicely in tables.
Can safely be performed in place.
==================
*/
void W_CleanupName(const char *in, char *out)
{
   int i;
   int c;

   for (i = 0; i < 16; i++)
   {
      c = in[i];
      if (!c)
         break;

      if (c >= 'A' && c <= 'Z')
         c += ('a' - 'A');
      out[i] = c;
   }

   for (; i < 16; i++)
      out[i] = 0;
}



/*
====================
W_LoadWadFile
====================
*/
bool W_LoadWadFile(const char *filename)
{
   lumpinfo_t *lump_p;
   wadinfo_t *header;
   unsigned i;
   int infotableofs;

   wad_base = (byte*)COM_LoadHunkFile(filename);
   if (!wad_base) {
      Sys_Error("%s: couldn't load %s", __func__, filename);
      return false;
   }

   header = (wadinfo_t *)wad_base;

   if (header->identification[0] != 'W'
         || header->identification[1] != 'A'
         || header->identification[2] != 'D'
         || header->identification[3] != '2') {
      Sys_Error("Wad file %s doesn't have WAD2 id", filename);
      return false;
   }

   wad_numlumps = LittleLong(header->numlumps);
   infotableofs = LittleLong(header->infotableofs);

   /* Defensive: numlumps and infotableofs are file-controlled.
    * Without bounds checking, a hostile WAD can make wad_lumps
    * point outside the loaded buffer or make the loop walk far
    * past the lumpinfo array.  com_filesize was set by the
    * underlying COM_LoadFile and reflects the actual byte
    * count loaded into wad_base.
    *
    * wad_numlumps is signed, so reject negatives explicitly --
    * a comparison of a negative int with the positive constant
    * 65536 is signed and would otherwise pass. */
   if (wad_numlumps < 0
       || wad_numlumps > 65536
       || infotableofs < 0
       || infotableofs > com_filesize
       || (long long)infotableofs + (long long)wad_numlumps
              * (long long)sizeof(lumpinfo_t) > (long long)com_filesize) {
      Sys_Error("Wad file %s has corrupt header", filename);
      return false;
   }

   wad_lumps    = (lumpinfo_t *)(wad_base + infotableofs);

   for (i = 0, lump_p = wad_lumps; i < wad_numlumps; i++, lump_p++)
   {
      lump_p->filepos  = LittleLong(lump_p->filepos);
      lump_p->disksize = LittleLong(lump_p->disksize);
      lump_p->size     = LittleLong(lump_p->size);
      /* Per-lump bounds.  Each lump points to a region of the
       * loaded WAD by (filepos, disksize); consumers of
       * W_GetLumpName / W_GetLumpNum receive (wad_base +
       * filepos) and use the data without further range checks
       * (they rely on Hunk_PointerInHunk only for the *base*
       * pointer, not for the lump extent).  Without this check
       * a hostile WAD can make a lump appear to start anywhere
       * within addressable memory, or extend far past the end
       * of the file.
       *
       * disksize 0 is acceptable for label / placeholder lumps,
       * but a negative value or a (filepos + disksize) extending
       * past com_filesize is a corrupt or malicious file. */
      if (lump_p->filepos < 0 || lump_p->disksize < 0
          || (long long)lump_p->filepos + (long long)lump_p->disksize
                 > (long long)com_filesize) {
         Sys_Error("Wad file %s lump %u out of range "
                          "(filepos=%i, disksize=%i, filesize=%i)",
                          filename, i, lump_p->filepos,
                          lump_p->disksize, com_filesize);
         return false;
      }

      W_CleanupName(lump_p->name, lump_p->name);
      if (lump_p->type == TYP_QPIC) {
         qpic_t *qpic;

         /* Need at least the qpic_t header (8 bytes for two
          * int32s) before SwapPic touches it. */
         if (lump_p->disksize < (int)(2 * sizeof(int32_t))) {
            Sys_Error("Wad file %s qpic lump %u too small "
                             "(disksize=%i)", filename, i,
                             lump_p->disksize);
            return false;
         }
         qpic = (qpic_t *)(wad_base + lump_p->filepos);
         SwapPic(qpic);
         /* Validate width/height fit within the lump's
          * disksize.  Same shape as Draw_CachePic's check
          * for standalone .lmp files: a hostile WAD with
          * width = -1 / huge / negative makes downstream
          * Draw_Pic's `memcpy(dest, source, pic->width)`
          * either interpret -1 as size_t SIZE_MAX or walk
          * pic->data[] far past the lump extent.
          * disksize - 8 is the available pixel space. */
         if (qpic->width <= 0 || qpic->height <= 0
             || qpic->width > 4096 || qpic->height > 4096
             || (size_t)qpic->width * (size_t)qpic->height
                  > (size_t)lump_p->disksize - offsetof(qpic_t, data)) {
            Sys_Error("Wad file %s qpic lump %u has bad "
                             "dimensions %dx%d (disksize=%i)",
                             filename, i, qpic->width, qpic->height,
                             lump_p->disksize);
            return false;
         }
      }
   }

   return true;
}


/*
=============
W_GetLumpinfo
=============
*/
lumpinfo_t * W_GetLumpinfo(const char *name)
{
   int i;
   lumpinfo_t *lump_p;
   char clean[16];

   W_CleanupName(name, clean);

   for (lump_p = wad_lumps, i = 0; i < wad_numlumps; i++, lump_p++)
   {
      if (!strcmp(clean, lump_p->name))
         return lump_p;
   }

   Sys_Error("%s: %s not found", __func__, name);
   return NULL;
}

void * W_GetLumpName(const char *name)
{
    lumpinfo_t *lump = W_GetLumpinfo(name);
    return (void *)(wad_base + lump->filepos);
}

void *W_GetLumpNum(int num)
{
   lumpinfo_t *lump;

   /* Off-by-one: num == wad_numlumps would read one past the
    * end of the wad_lumps array. */
   if (num < 0 || num >= wad_numlumps)
      Sys_Error("%s: bad number: %i", __func__, num);

   lump = wad_lumps + num;

   return (void *)(wad_base + lump->filepos);
}

/*
=============================================================================

automatic byte swapping

=============================================================================
*/

void SwapPic(qpic_t *pic)
{
    pic->width  = LittleLong(pic->width);
    pic->height = LittleLong(pic->height);
}
