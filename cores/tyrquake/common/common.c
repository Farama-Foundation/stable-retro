/*
Copyright (C) 1996-1997 Id Software, Inc.
Copyright (C) 2002-2009 John Fitzgibbons and others
Copyright (C) 2010-2014 QuakeSpasm developers

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
/* common.c -- misc functions used in client and server */
#include "compat/strl.h"
#include <ctype.h>
#include <limits.h>
#include <retro_dirent.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>

#include <file/file_path.h>

#include "quakedef.h"
#include "host.h"

#include "cmd.h"
#include "common.h"
#include "console.h"
#include "crc.h"
#include "draw.h"
#include "net.h"
#include "shell.h"
#include "sys.h"
#include "zone.h"

#include <streams/file_stream.h>

/* forward declarations */
int64_t rfread(void* buffer,
   size_t elem_size, size_t elem_count, RFILE* stream);
int rfclose(RFILE* stream);
int64_t rftell(RFILE* stream);
char *rfgets(char *buffer, int maxCount, RFILE* stream);
RFILE* rfopen(const char *path, const char *mode);
int64_t rfwrite(void const* buffer,
   size_t elem_size, size_t elem_count, RFILE* stream);
int64_t rfseek(RFILE* stream, int64_t offset, int origin);
int rferror(RFILE* stream);
int rfgetc(RFILE* stream);

#define NUM_SAFE_ARGVS 7

static const char *largv[MAX_NUM_ARGVS + NUM_SAFE_ARGVS + 1];
static const char *argvdummy = " ";

static const char *safeargvs[NUM_SAFE_ARGVS] = {
  "-stdvid", "-nolan", "-nosound", "-nocdaudio", "-nojoy", "-nomouse", "-dibonly"
};

cvar_t registered = { "registered", "0" };
static cvar_t cmdline = { "cmdline", "0", false, true };

static qboolean com_modified;		/* set true if using non-id files */
static int static_registered = 1;	/* only for startup check, then set */

static void COM_InitFilesystem(void);
static void COM_Path_f(void);
static void *SZ_GetSpace(sizebuf_t *buf, int length);

/* if a packfile directory differs from this, it is assumed to be hacked */
#define PAK0_COUNT		339
#define NQ_PAK0_CRC		32981
#define QW_PAK0_CRC		52883

#define CMDLINE_LENGTH	256
static char com_cmdline[CMDLINE_LENGTH];

qboolean standard_quake = true, rogue, hipnotic;

/*

All of Quake's data access is through a hierchal file system, but the contents
of the file system can be transparently merged from several sources.

The "base directory" is the path to the directory holding the quake.exe and
all game directories.  The sys_* files pass this to host_init in
quakeparms_t->basedir.  This can be overridden with the "-basedir" command
line parm to allow code debugging in a different directory.  The base
directory is only used during filesystem initialization.

The "game directory" is the first tree on the search path and directory that
all generated files (savegames, screenshots, demos, config files) will be
saved to.  This can be overridden with the "-game" command line parameter.
The game directory can never be changed while quake is executing.  This is a
precacution against having a malicious server instruct clients to write files
over areas they shouldn't.

*/

/* ============================================================================ */


/* ClearLink is used for new headnodes */
void ClearLink(link_t *l)
{
   l->prev = l->next = l;
}

void RemoveLink(link_t *l)
{
   l->next->prev = l->prev;
   l->prev->next = l->next;
}

void InsertLinkBefore(link_t *l, link_t *before)
{
   l->next = before;
   l->prev = before->prev;
   l->prev->next = l;
   l->next->prev = l;
}

/*
============================================================================

			LIBRARY REPLACEMENT FUNCTIONS

============================================================================
*/

/*
 * Use this function to share static string buffers
 * between different text processing functions.
 * Try to avoid fixed-size intermediate buffers like this if possible
 */
#define COM_STRBUF_LEN 2048
static char *COM_GetStrBuf(void)
{
   static char buffers[4][COM_STRBUF_LEN];
   static unsigned int idx;
   idx = (idx + 1) & 3;
   return buffers[idx];
}

/*
 * Compose dst = a + sep + b, returning the number of bytes that would
 * have been written (excluding the NUL) on success, or -1 on overflow.
 * GCC -Wformat-truncation cannot see that the inputs to the various
 * snprintf("%s/%s", ...) call sites are length-bounded; routing those
 * sites through this helper performs the bound check explicitly,
 * silences the warning, and gives callers a real overflow signal.
 */
int COM_JoinPath(char *dst, size_t dst_size,
                 const char *a, char sep, const char *b)
{
   size_t la = strlen(a);
   size_t lb = strlen(b);
   /* la + 1 (sep) + lb + 1 (NUL) must fit */
   if (la + lb + 2 > dst_size)
   {
      if (dst_size > 0)
         dst[0] = '\0';
      return -1;
   }
   memcpy(dst, a, la);
   dst[la] = sep;
   memcpy(dst + la + 1, b, lb);
   dst[la + 1 + lb] = '\0';
   return (int)(la + 1 + lb);
}

int Q_atoi(const char *str)
{
   /* Use unsigned for accumulation so the left-shift in the hex path
    * and the * 10 in the decimal path do not invoke undefined behavior
    * on signed overflow. The final cast back to int wraps modulo 2^32
    * which is the historical behavior callers expect. */
   unsigned int val;
   int sign;
   int c;

   if (*str == '-')
   {
      sign = -1;
      str++;
   } else
      sign = 1;

   val = 0;

   /* check for hex */
   if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X'))
   {
      str += 2;
      while (1)
      {
         c = *str++;
         if (c >= '0' && c <= '9')
            val = (val << 4) + (unsigned)(c - '0');
         else if (c >= 'a' && c <= 'f')
            val = (val << 4) + (unsigned)(c - 'a' + 10);
         else if (c >= 'A' && c <= 'F')
            val = (val << 4) + (unsigned)(c - 'A' + 10);
         else
            return (int)val * sign;
      }
   }

   /* check for character */
   if (str[0] == '\'')
      return sign * str[1];

   /* assume decimal */
   while (1)
   {
      c = *str++;
      if (c < '0' || c > '9')
         return (int)val * sign;
      val = val * 10u + (unsigned)(c - '0');
   }

   return 0;
}


float Q_atof(const char *str)
{
   double val;
   int sign;
   int c;
   int decimal, total;

   if (*str == '-')
   {
      sign = -1;
      str++;
   }
   else
      sign = 1;

   val = 0;

   /* check for hex */
   if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X'))
   {
      str += 2;
      while (1)
      {
         c = *str++;
         if (c >= '0' && c <= '9')
            val = (val * 16) + c - '0';
         else if (c >= 'a' && c <= 'f')
            val = (val * 16) + c - 'a' + 10;
         else if (c >= 'A' && c <= 'F')
            val = (val * 16) + c - 'A' + 10;
         else
            return val * sign;
      }
   }

   /* check for character */
   if (str[0] == '\'')
      return sign * str[1];

   /* assume decimal */
   decimal = -1;
   total = 0;
   while (1)
   {
      c = *str++;
      if (c == '.')
      {
         decimal = total;
         continue;
      }
      if (c < '0' || c > '9')
         break;
      val = val * 10 + c - '0';
      total++;
   }

   if (decimal == -1)
      return val * sign;

   while (total > decimal)
   {
      val /= 10;
      total--;
   }

   return val * sign;
}

/*
==============================================================================

			MESSAGE IO FUNCTIONS

Handles byte ordering and avoids alignment errors
==============================================================================
*/

/* writing functions */

void MSG_WriteChar(sizebuf_t *sb, int c)
{
   byte *buf = (byte*)SZ_GetSpace(sb, 1);
   buf[0] = c;
}

void MSG_WriteByte(sizebuf_t *sb, int c)
{
   byte *buf = (byte*)SZ_GetSpace(sb, 1);
   buf[0] = c;
}

void MSG_WriteShort(sizebuf_t *sb, int c)
{
   byte *buf = (byte*)SZ_GetSpace(sb, 2);
   buf[0] = c & 0xff;
   buf[1] = c >> 8;
}

void MSG_WriteLong(sizebuf_t *sb, int c)
{
   byte *buf = (byte*)SZ_GetSpace(sb, 4);
   buf[0] = c & 0xff;
   buf[1] = (c >> 8) & 0xff;
   buf[2] = (c >> 16) & 0xff;
   buf[3] = c >> 24;
}

void MSG_WriteFloat(sizebuf_t *sb, float f)
{
   union {
      float f;
      int l;
   } dat;

   dat.f = f;
   dat.l = LittleLong(dat.l);

   SZ_Write(sb, &dat.l, 4);
}

void MSG_WriteString(sizebuf_t *sb, const char *s)
{
   if (!s)
      SZ_Write(sb, "", 1);
   else
      SZ_Write(sb, s, strlen(s) + 1);
}

void MSG_WriteStringvf(sizebuf_t *sb, const char *fmt, va_list ap)
{
   /*
    * FIXME - Kind of ugly to check space first then do getspace
    * afterwards, but we don't know how much we'll need before
    * hand. Update the SZ interface?
    */
   int maxlen = sb->maxsize - sb->cursize;
   int    len;

   /* Defensive: a corrupted sizebuf where cursize exceeds
    * maxsize would make maxlen negative, then the cast to
    * size_t in vsnprintf turns it into an enormous value and
    * overflows sb->data.  Bail rather than scribble. */
   if (maxlen <= 0)
      return;

   len = vsnprintf((char *)sb->data + sb->cursize, maxlen, fmt, ap);

   /* Use SZ_GetSpace to check for overflow */
   if (len >= 0)
      SZ_GetSpace(sb, len + 1);
}

void MSG_WriteStringf(sizebuf_t *sb, const char *fmt, ...)
{
   va_list ap;

   va_start(ap, fmt);
   MSG_WriteStringvf(sb, fmt, ap);
   va_end(ap);
}

void MSG_WriteCoord(sizebuf_t *sb, float f)
{
   /*
    * Co-ords are send as shorts, with the low 3 bits being the fractional
    * component
    */
   MSG_WriteShort(sb, (int)(f * (1 << 3)));
}

void MSG_WriteAngle(sizebuf_t *sb, float f)
{
   MSG_WriteByte(sb, (int)floorf((f * 256 / 360) + 0.5f) & 255);
}

void MSG_WriteAngle16(sizebuf_t *sb, float f)
{
   MSG_WriteShort(sb, (int)floorf((f * 65536 / 360) + 0.5f) & 65535);
}

/*
 * Write the current message length to the start of the buffer (in big
 * endian format) with the control flag set.
 */
void MSG_WriteControlHeader(sizebuf_t *sb)
{
   int c = NETFLAG_CTL | (sb->cursize & NETFLAG_LENGTH_MASK);

   sb->data[0] = c >> 24;
   sb->data[1] = (c >> 16) & 0xff;
   sb->data[2] = (c >> 8) & 0xff;
   sb->data[3] = c & 0xff;
}

/* reading functions */
int msg_readcount;
qboolean msg_badread;

void MSG_BeginReading(void)
{
   msg_readcount = 0;
   msg_badread = false;
}

/* returns -1 and sets msg_badread if no more characters are available */
int MSG_ReadChar(void)
{
   int c;

   if (msg_readcount + 1 > net_message.cursize)
      goto error;

   c = (signed char)net_message.data[msg_readcount];
   msg_readcount++;

   return c;

error:
   msg_badread = true;
   return -1;
}

int MSG_ReadByte(void)
{
   int c;

   if (msg_readcount + 1 > net_message.cursize)
      goto error;

   c = (unsigned char)net_message.data[msg_readcount];
   msg_readcount++;

   return c;

error:
   msg_badread = true;
   return -1;
}

int MSG_ReadShort(void)
{
   int c;

   if (msg_readcount + 2 > net_message.cursize)
      goto error;

   c = (short)(net_message.data[msg_readcount]
         + (net_message.data[msg_readcount + 1] << 8));

   msg_readcount += 2;

   return c;

error:
   msg_badread = true;
   return -1;
}

int MSG_ReadLong(void)
{
   /* Use unsigned accumulation: in the original signed
    * formulation, net_message.data[msg_readcount+3] is
    * promoted to int and then shifted by 24.  When that
    * byte has its high bit set (>= 0x80), the shifted
    * value 0x80000000+ exceeds INT_MAX; left-shifting an
    * int into the sign bit is implementation-defined per
    * C99 6.5.7.  All real compilers wrap to negative
    * correctly, but the unsigned form is well-defined and
    * lets a static analyser verify the read. */
   unsigned int u;

   if (msg_readcount + 4 > net_message.cursize)
      goto error;

   u =  (unsigned)net_message.data[msg_readcount]
      | ((unsigned)net_message.data[msg_readcount + 1] << 8)
      | ((unsigned)net_message.data[msg_readcount + 2] << 16)
      | ((unsigned)net_message.data[msg_readcount + 3] << 24);

   msg_readcount += 4;

   return (int)u;

error:
   msg_badread = true;
   return -1;
}

float MSG_ReadFloat(void)
{
   union {
      byte b[4];
      float f;
      int l;
   } dat;

   /* Match MSG_ReadByte / Short / Long: bounds-check before
    * reading 4 bytes.  Without this, a truncated server
    * message ending right before a float field reads 4
    * bytes past net_message.data[].  Set msg_badread so the
    * outer parse loop sees the failure on its next
    * iteration check; return 0.0f as the safe default. */
   if (msg_readcount + 4 > net_message.cursize)
   {
      msg_badread = true;
      return 0.0f;
   }

   dat.b[0] = net_message.data[msg_readcount];
   dat.b[1] = net_message.data[msg_readcount + 1];
   dat.b[2] = net_message.data[msg_readcount + 2];
   dat.b[3] = net_message.data[msg_readcount + 3];
   msg_readcount += 4;

   dat.l = LittleLong(dat.l);

   /* IEEE-754 NaN and Inf bit patterns are perfectly valid
    * over the wire -- any 4 bytes the server cared to send
    * land here -- but every consumer of this function feeds
    * the result into floating-point math whose downstream is
    * undefined on non-finite input.  The only call site today
    * is cl.mtime[0] (svc_time), which then drives cl.time,
    * the interpolation denominator 'f = cl.mtime[0] -
    * cl.mtime[1]' in CL_RelinkEntities (NaN propagates;
    * Inf - Inf is NaN), ent->msgtime, and is echoed back to
    * the server via MSG_WriteFloat for ping-time accounting.
    * One malformed svc_time packet corrupts the client's
    * notion of game time across every entity and the rest of
    * the connection.
    *
    * Treat non-finite as a malformed packet: trip msg_badread
    * the same way the bounds-check above does, and return
    * 0.0f.  The outer parse loop drops the message. */
   if (IS_NAN(dat.f))
   {
      msg_badread = true;
      return 0.0f;
   }

   return dat.f;
}

char *MSG_ReadString(void)
{
   char *buf = COM_GetStrBuf();
   int len   = 0;

   do
   {
      int c = MSG_ReadChar();
      if (c == -1 || c == 0)
         break;
      buf[len++] = c;
   }while (len < COM_STRBUF_LEN - 1);

   buf[len] = 0;

   return buf;
}

float MSG_ReadCoord(void)
{
   /*
    * Co-ords are send as shorts, with the low 3 bits being the fractional
    * component
    */
   return MSG_ReadShort() * (1.0 / (1 << 3));
}

float MSG_ReadAngle(void)
{
   return MSG_ReadChar() * (360.0 / 256);
}

float MSG_ReadAngle16(void)
{
   return MSG_ReadShort() * (360.0 / 65536);
}

/*
 * Read back the message control header
 * Essentially this is MSG_ReadLong, but big-endian byte order.
 */
int MSG_ReadControlHeader(void)
{
   /* Same unsigned-shift fix as MSG_ReadLong; this reads
    * the 4-byte network-byte-order control header and
    * suffers the same implementation-defined behavior
    * when data[0] has its high bit set. */
   unsigned int u;

   if (msg_readcount + 4 > net_message.cursize)
   {
      msg_badread = true;
      return -1;
   }

   u =  ((unsigned)net_message.data[msg_readcount]     << 24)
      | ((unsigned)net_message.data[msg_readcount + 1] << 16)
      | ((unsigned)net_message.data[msg_readcount + 2] << 8)
      |  (unsigned)net_message.data[msg_readcount + 3];

   msg_readcount += 4;

   return (int)u;
}

/* =========================================================================== */

void SZ_Alloc(sizebuf_t *buf, int startsize)
{
   if (startsize < 256)
      startsize = 256;
   buf->data    = (byte*)Hunk_Alloc(startsize);
   buf->maxsize = startsize;
   buf->cursize = 0;
}

void SZ_Free(sizebuf_t *buf)
{
   /*      Z_Free (buf->data); */
   /*      buf->data = NULL; */
   /*      buf->maxsize = 0; */
   buf->cursize = 0;
}

void SZ_Clear(sizebuf_t *buf)
{
   buf->cursize = 0;
   buf->overflowed = false;
}

static void *SZ_GetSpace(sizebuf_t *buf, int length)
{
   void *data;

   /* Defensive: reject negative or absurd lengths up front.
    * The original signed-int arithmetic in the overflow check
    * below can wrap when length is large enough that
    * cursize + length > INT_MAX, silently passing the check. */
   if (length < 0 || length > buf->maxsize)
   {
      Sys_Error("%s: bad length %d (max %d)",
                __func__, length, buf->maxsize);
   }

   if (buf->cursize + length > buf->maxsize)
   {
      if (!buf->allowoverflow)
         Sys_Error("%s: overflow without allowoverflow set (%d > %d)",
               __func__, buf->cursize + length, buf->maxsize);
      if (length > buf->maxsize)
         Sys_Error("%s: %d is > full buffer size", __func__, length);
      if (developer.value)
         /* Con_Printf may be redirected */
         Sys_Printf("%s: overflow\n", __func__);
      SZ_Clear(buf);
      buf->overflowed = true;
   }
   data = buf->data + buf->cursize;
   buf->cursize += length;

   return data;
}

void SZ_Write(sizebuf_t *buf, const void *data, int length)
{
   memcpy(SZ_GetSpace(buf, length), data, length);
}

void SZ_Print(sizebuf_t *buf, const char *data)
{
   size_t len = strlen(data);

   /* If buf->data has a trailing zero, overwrite it */
   if (!buf->cursize || buf->data[buf->cursize - 1])
      memcpy(SZ_GetSpace(buf, len + 1), data, len + 1);
   else
      memcpy((byte*)SZ_GetSpace(buf, len) - 1, data, len + 1);
}


/* ============================================================================ */


/*
============
COM_SkipPath
============
*/
const char *COM_SkipPath(const char *pathname)
{
   const char *last = pathname;
   while (*pathname)
   {
      if (*pathname == '/')
         last = pathname + 1;
      pathname++;
   }
   return last;
}

/*
============
COM_StripExtension
============
*/
void COM_StripExtension(char *filename)
{
   const char *start = COM_SkipPath(filename);
   char *pos = (char*)strrchr(start, '.');
   if (pos && *pos)
      *pos = 0;
}

/*
============
COM_FileExtension
============
*/
const char *COM_FileExtension(const char *in)
{
   static char exten[8];
   const char *dot;
   int i;

   in = COM_SkipPath(in);
   dot = strrchr(in, '.');
   if (!dot)
      return "";

   dot++;
   for (i = 0; i < sizeof(exten) - 1 && *dot; i++, dot++)
      exten[i] = *dot;
   exten[i] = 0;

   return exten;
}

/*
==================
COM_DefaultExtension
==================
*/
void COM_DefaultExtension(char *path, size_t pathsize, const char *extension)
{
   /* if path doesn't have a .EXT, append extension */
   /* (extension should include the .) */
   const char *src = path + strlen(path) - 1;
   while (*src != '/' && src != path)
   {
      if (*src == '.')
         return;		/* it has an extension */
      src--;
   }
   strlcat(path, extension, pathsize);
}

int COM_CheckExtension(const char *path, const char *extn)
{
   int ret   = 0;
   char *pos = (char*)strrchr(path, '.');

   if (pos)
   {
      if (extn[0] != '.')
         pos++;
      ret = pos && !strcasecmp(pos, extn);
   }

   return ret;
}

/* ============================================================================ */

char com_token[1024];
unsigned com_argc;
const char **com_argv;

/*
==============
COM_Parse

Parse a token out of a string
==============
*/
static const char single_chars[] = "{})(':";

static const char *COM_Parse_(const char *data, qboolean split_single_chars)
{
   int c;
   int len = 0;
   /* com_token is sizeof(com_token) bytes; reserve one for NUL.
    * Without this guard, a long quoted string or unbroken word
    * walks off the end of the static buffer and corrupts whatever
    * follows it in .bss.  This is reachable from any attacker-
    * controlled text the engine parses: BSP entity lump, savegame,
    * config files, Cmd_TokenizeString'd console / network input. */
   const int max = (int)sizeof(com_token) - 1;

   com_token[0] = 0;

   if (!data)
      return NULL;

   /* skip whitespace */
skipwhite:
   while ((c = *data) <= ' ') {
      if (c == 0)
         return NULL;	/* end of file; */
      data++;
   }

   /* skip // comments */
   if (c == '/' && data[1] == '/') {
      while (*data && *data != '\n')
         data++;
      goto skipwhite;
   }
   /* handle quoted strings specially */
   if (c == '\"') {
      data++;
      while (1) {
         c = *data++;
         if (c == '\"' || !c) {
            if (len > max)
               len = max;
            com_token[len] = 0;
            return data;
         }
         if (len < max)
            com_token[len] = c;
         len++;
      }
   }
   /* parse single characters */
   if (split_single_chars && strchr(single_chars, c)) {
      com_token[len] = c;
      len++;
      com_token[len] = 0;
      return data + 1;
   }
   /* parse a regular word */
   do {
      if (len < max)
         com_token[len] = c;
      data++;
      len++;
      c = *data;
      if (split_single_chars && strchr(single_chars, c))
         break;
   } while (c > 32);

   /* If the token overflowed, len may exceed max here.  Clamp the
    * terminator position to the last valid byte so we always end
    * up with a NUL inside the buffer. */
   if (len > max)
      len = max;
   com_token[len] = 0;
   return data;
}

const char *
COM_Parse(const char *data)
{
    return COM_Parse_(data, true);
}

/*
================
COM_CheckParm

Returns the position (1 to argc-1) in the program's argument list
where the given parameter apears, or 0 if not present
================
*/
unsigned COM_CheckParm(const char *parm)
{
   unsigned i;

   for (i = 1; i < com_argc; i++) {
      if (!com_argv[i])
         continue;		/* NEXTSTEP sometimes clears appkit vars. */
      if (!strcmp(parm, com_argv[i]))
         return i;
   }

   return 0;
}

/*
================
COM_CheckRegistered

Looks for the pop.txt file and verifies it.
Sets the "registered" cvar.
Immediately exits out if an alternate game was attempted to be started without
being registered.
================
*/
static void COM_CheckRegistered(void)
{
   RFILE *f;

   Cvar_Set("cmdline", com_cmdline);
   COM_FOpenFile("gfx/pop.lmp", &f);
   if (!f)
      Con_Printf("Playing shareware version.\n");
   else
   {
      rfclose(f);
      Cvar_Set("registered", "1");
      static_registered = 1;
      Con_Printf("Playing registered version.\n");
   }
}


/*
================
COM_InitArgv
================
*/
void COM_InitArgv(int argc, const char **argv)
{
   qboolean safe;
   int i;
   int j, n = 0;
   /* reconstitute the command line for the cmdline externally visible cvar */
   for (j = 0; (j < MAX_NUM_ARGVS) && (j < argc); j++)
   {
      i = 0;
      while ((n < (CMDLINE_LENGTH - 1)) && argv[j][i])
         com_cmdline[n++] = argv[j][i++];

      if (n < (CMDLINE_LENGTH - 1))
         com_cmdline[n++] = ' ';
      else
         break;
   }
   com_cmdline[n] = 0;

   safe = false;

   for (com_argc = 0; (com_argc < MAX_NUM_ARGVS) && (com_argc < argc);
         com_argc++) {
      largv[com_argc] = argv[com_argc];
      if (!strcmp("-safe", argv[com_argc]))
         safe = true;
   }

   if (safe) {
      /* force all the safe-mode switches. Note that we reserved extra space in */
      /* case we need to add these, so we don't need an overflow check */
      for (i = 0; i < NUM_SAFE_ARGVS; i++) {
         largv[com_argc] = safeargvs[i];
         com_argc++;
      }
   }

   largv[com_argc] = argvdummy;
   com_argv = largv;

   /* Reset the mission-pack latches before re-checking argv, so
    * a subsequent retro_load_game on a statically-linked target
    * doesn't carry rogue/hipnotic mode over from the previous
    * session.  These globals are read in dozens of places to
    * alter game behavior; without this reset, switching from a
    * mission pack back to base Quake leaves the game running in
    * the previous pack's mode. */
   rogue          = false;
   hipnotic       = false;
   standard_quake = true;

   if (COM_CheckParm("-rogue")) {
      rogue = true;
      standard_quake = false;
   }

   if (COM_CheckParm("-hipnotic") || COM_CheckParm("-quoth")) {
      hipnotic = true;
      standard_quake = false;
   }
}

/*
================
COM_AddParm

Adds the given string at the end of the current argument list
================
*/

/*
================
COM_Init
================
*/

void COM_Init(void)
{
   Cvar_RegisterVariable(&registered);
   Cvar_RegisterVariable(&cmdline);
   Cmd_AddCommand("path", COM_Path_f);

   COM_InitFilesystem();
   COM_CheckRegistered();
}


/*
============
va

does a varargs printf into a temp buffer, so I don't need to have
varargs versions of all text functions.
============
*/
char *va(const char *format, ...)
{
   va_list argptr;
   int len;
   char *buf = COM_GetStrBuf();

   va_start(argptr, format);
   len = vsnprintf(buf, COM_STRBUF_LEN, format, argptr);
   va_end(argptr);

   if (len > COM_STRBUF_LEN - 1)
      Con_DPrintf("%s: overflow (string truncated)\n", __func__);

   return buf;
}


/*
=============================================================================

QUAKE FILESYSTEM

=============================================================================
*/

int com_filesize;

/* in memory */

typedef struct {
    char name[MAX_QPATH];
    int filepos, filelen;
} packfile_t;

typedef struct pack_s
{
    char filename[MAX_OSPATH];
    int numfiles;
    packfile_t *files;
} pack_t;

/* on disk */
#define MAX_PACKPATH 56
typedef struct
{
    char name[MAX_PACKPATH];
    int filepos, filelen;
} dpackfile_t;

typedef struct
{
    char id[4];
    int dirofs;
    int dirlen;
} dpackheader_t;

char com_gamedir[MAX_OSPATH];
char com_basedir[MAX_OSPATH];
char com_savedir[MAX_OSPATH];

typedef struct searchpath_s {
    char filename[MAX_OSPATH];
    pack_t *pack;		/* only one of filename / pack will be used */
    struct searchpath_s *next;
} searchpath_t;

static searchpath_t *com_searchpaths;

/*
================
COM_filelength
================
*/
static int COM_filelength(RFILE *f)
{
   /* rftell returns int64_t. Use the wide type for the seek/tell
    * roundtrip; the result is narrowed to int at the return because
    * com_filesize and most callers are int. Files larger than INT_MAX
    * (~2 GiB) are not supported by the rest of the engine; clamp
    * defensively rather than overflow silently. */
   int64_t end;
   int64_t pos = rftell(f);

   rfseek(f, 0, SEEK_END);
   end = rftell(f);
   rfseek(f, pos, SEEK_SET);

   if (end > (int64_t)INT_MAX)
      return INT_MAX;
   if (end < 0)
      return -1;
   return (int)end;
}

static int COM_FileOpenRead(const char *path, RFILE **hndl)
{
   RFILE *f = rfopen(path, "rb");

   if (!f)
      goto error;
   *hndl = f;

   return COM_filelength(f);

error:
   *hndl = NULL;
   return -1;
}

/*
============
COM_Path_f

============
*/
static void COM_Path_f(void)
{
   searchpath_t *s;

   Con_Printf("Current search path:\n");
   for (s = com_searchpaths; s; s = s->next)
   {
      if (s->pack)
         Con_Printf("%s (%i files)\n", s->pack->filename,
               s->pack->numfiles);
      else
         Con_Printf("%s\n", s->filename);
   }
}

/*
============
COM_WriteFile

The filename will be prefixed by the current game directory
============
*/
void COM_WriteFile(const char *filename, const void *data, int len)
{
   RFILE *f;
   char name[MAX_OSPATH];

   if (COM_JoinPath(name, sizeof(name), com_gamedir, '/', filename) < 0)
      Sys_Error("Error opening %s: path too long", filename);

   f = rfopen(name, "wb");
   if (!f)
   {
      path_mkdir(com_gamedir);
      f = rfopen(name, "wb");
      if (!f)
         Sys_Error("Error opening %s", filename);
   }
   rfwrite(data, 1, len, f);
   rfclose(f);
}


/*
===========
COM_FOpenFile

Finds the file in the search path.
Sets com_filesize
If the requested file is inside a packfile, a new FILE * will be opened
into the file.
===========
*/
int file_from_pak; /* global indicating file came from pack file */

int COM_FOpenFile(const char *filename, RFILE **file)
{
   searchpath_t *search;
   char path[MAX_OSPATH];
   pack_t *pak;
   int i;

   file_from_pak = 0;

   /* search through the path, one element at a time */
   for (search = com_searchpaths; search; search = search->next)
   {
      /* is the element a pak file? */
      if (search->pack)
      {
         /* look through all the pak file elements */
         pak = search->pack;
         for (i = 0; i < pak->numfiles; i++)
            if (!strcmp(pak->files[i].name, filename))
            {	/* found it! */
               /* open a new file on the pakfile */
               *file = rfopen(pak->filename, "rb");
               if (!*file)
                  Sys_Error("Couldn't reopen %s", pak->filename);
               rfseek(*file, pak->files[i].filepos, SEEK_SET);
               com_filesize  = pak->files[i].filelen;
               file_from_pak = 1;
               return com_filesize;
            }
      } else {
         /* check a file in the directory tree */
         if (!static_registered)
         {
            /* if not a registered version, don't ever go beyond base */
            if (strchr(filename, '/') || strchr(filename, '\\'))
               continue;
         }
         if (COM_JoinPath(path, sizeof(path), search->filename, '/', filename) < 0)
            continue;
         if (!path_is_valid(path))
            continue;

         *file        = rfopen(path, "rb");
         com_filesize = COM_filelength(*file);
         return com_filesize;
      }
   }

   *file        = NULL;
   com_filesize = -1;

   return -1;
}

/*
===========
COM_FileExists

Returns whether the file is found in the quake filesystem.
===========
*/
qboolean COM_FileExists (const char *filename)
{
   searchpath_t *search;
   char path[MAX_OSPATH];
   pack_t *pak;
   int i;

   file_from_pak = 0;

   /* search through the path, one element at a time */
   for (search = com_searchpaths; search; search = search->next)
   {
      /* is the element a pak file? */
      if (search->pack)
      {
         /* look through all the pak file elements */
         pak = search->pack;
         for (i = 0; i < pak->numfiles; i++)
            if (!strcmp(pak->files[i].name, filename))
            {	/* found it! */
               com_filesize = pak->files[i].filelen;
               file_from_pak = 1;
               return true;
            }
      } else {
         /* check a file in the directory tree */
         if (!static_registered)
         {
            /* if not a registered version, don't ever go beyond base */
            if (strchr(filename, '/') || strchr(filename, '\\'))
               continue;
         }
         if (COM_JoinPath(path, sizeof(path), search->filename, '/', filename) < 0)
            continue;
         if (!path_is_valid(path))
            continue;

         return true;
      }
   }

   Sys_Printf("FindFile: can't find %s\n", filename);
   com_filesize = -1;

   return false;
}

static void COM_ScanDirDir(struct stree_root *root, struct RDIR *dir, const char *pfx,
      const char *ext, qboolean stripext)
{
   char *fname;
   int pfx_len = pfx ? strlen(pfx) : 0;
   int ext_len = ext ? strlen(ext) : 0;

   while (retro_readdir(dir))
   {
      if ((!pfx || !strncasecmp(retro_dirent_get_name(dir), pfx, pfx_len)) &&
            (!ext || COM_CheckExtension(retro_dirent_get_name(dir), ext)))
      {
         int len = strlen(retro_dirent_get_name(dir));
         if (ext && stripext)
            len -= ext_len;
         fname = (char*)Z_Malloc(len + 1);
         if (fname)
         {
            strlcpy(fname, retro_dirent_get_name(dir), len + 1);
            STree_InsertAlloc(root, fname, true);
            Z_Free(fname);
         }
      }
   }
}

static void COM_ScanDirPak(struct stree_root *root, pack_t *pak, const char *path,
      const char *pfx, const char *ext, qboolean stripext)
{
   int i, len;
   char *fname;
   int path_len = path ? strlen(path) : 0;
   int pfx_len  = pfx  ? strlen(pfx) : 0;
   int ext_len  = ext  ? strlen(ext) : 0;

   for (i = 0; i < pak->numfiles; i++)
   {
      /* Check the path prefix */
      char *pak_f = pak->files[i].name;

      if (path && path_len)
      {
         if (strncasecmp(pak_f, path, path_len))
            continue;
         if (pak_f[path_len] != '/')
            continue;
         pak_f += path_len + 1;
      }

      /* Don't match sub-directories */
      if (strchr(pak_f, '/'))
         continue;

      /* Check the prefix and extension, if set */
      if (pfx && pfx_len && strncasecmp(pak_f, pfx, pfx_len))
         continue;
      if (ext && ext_len && !COM_CheckExtension(pak_f, ext))
         continue;

      /* Ok, we have a match. Add it */
      len = strlen(pak_f);
      if (ext && stripext)
         len -= ext_len;
      fname = (char*)Z_Malloc(len + 1);

      if (fname)
      {
         strlcpy(fname, pak_f, len + 1);
         STree_InsertAlloc(root, fname, true);
         Z_Free(fname);
      }
   }
}

/*
============
COM_ScanDir

Scan the contents of a the given directory. Any filenames that match
both the given prefix and extension are added to the string tree.
Caller MUST have already called STree_AllocInit()
============
*/
void COM_ScanDir(struct stree_root *root, const char *path, const char *pfx,
      const char *ext, qboolean stripext)
{
   searchpath_t *search;
   char fullpath[MAX_OSPATH];
   struct RDIR *dir;

   for (search = com_searchpaths; search; search = search->next)
   {
      if (search->pack)
         COM_ScanDirPak(root, search->pack, path, pfx, ext, stripext);
      else
      {
         if (COM_JoinPath(fullpath, MAX_OSPATH, search->filename, '/', path) < 0)
            continue;
         dir = retro_opendir(fullpath);

         if (dir)
         {
            COM_ScanDirDir(root, dir, pfx, ext, stripext);
            retro_closedir(dir);
         }
      }
   }
}

/*
============
COM_LoadFile

Filename are reletive to the quake directory.
Optionally return length; set null if you don't want it.
Always appends a 0 byte to the loaded data.
============
*/
static cache_user_t *loadcache;
static byte *loadbuf;
static int loadsize;

static void *COM_LoadFile(const char *path, int usehunk, unsigned long *length)
{
   RFILE *f;
   byte *buf = NULL;			/* quiet compiler warning */
   int len = com_filesize = COM_FOpenFile(path, &f);  /* look for it in the filesystem or pack files */
   if (!f)
      return NULL;

   /* Defensive: the filesize ultimately comes from the PAK
    * directory entry's filelen field, which is untrusted.  A
    * negative or absurdly large value would either underflow
    * the buf[len]=0 store below or overflow the rfread buffer.
    * Cap to a reasonable maximum (256 MB) -- larger than any
    * legitimate single Quake asset, smaller than int wraparound. */
   if (len < 0 || len > 256 * 1024 * 1024) {
      rfclose(f);
      com_filesize = -1;
      return NULL;
   }

   if (length)
      *length = len;

   if (usehunk == 1)
      buf = (byte*)Hunk_Alloc(len + 1);
   else if (usehunk == 2)
      buf = (byte*)Hunk_TempAlloc(len + 1);
   else if (usehunk == 0)
      buf = (byte*)Z_Malloc(len + 1);
   else if (usehunk == 3)
      buf = (byte*)Cache_Alloc(loadcache, len + 1);
   else if (usehunk == 4)
   {
      if (len + 1 > loadsize)
         buf = (byte*)Hunk_TempAlloc(len + 1);
      else
         buf = loadbuf;
   } else
      Sys_Error("%s: bad usehunk", __func__);

   if (!buf)
      Sys_Error("%s: not enough space for %s", __func__, path);

   buf[len] = 0;

   Draw_BeginDisc();
   rfread(buf, 1, len, f);
   rfclose(f);
   Draw_EndDisc();

   return buf;
}

void *COM_LoadHunkFile(const char *path)
{
   return COM_LoadFile(path, 1, NULL);
}

void COM_LoadCacheFile(const char *path, struct cache_user_s *cu)
{
   loadcache = cu;
   COM_LoadFile(path, 3, NULL);
}

/* uses temp hunk if larger than bufsize */
/* length is size of loaded file in bytes */
void *COM_LoadStackFile(const char *path, void *buffer, int bufsize,
      unsigned long *length)
{
   byte *buf;

   loadbuf = (byte *)buffer;
   loadsize = bufsize;
   buf = (byte*)COM_LoadFile(path, 4, length);

   return buf;
}

/*
=================
COM_LoadPackFile

Takes an explicit (not game tree related) path to a pak file.

Loads the header and directory, adding the files at the beginning
of the list so they override previous pack files.
=================
*/
static pack_t *COM_LoadPackFile(const char *packfile)
{
   int mark;
   RFILE *packhandle = NULL;
   dpackheader_t header;
   dpackfile_t *dfiles;
   packfile_t *mfiles;
   pack_t *pack;
   int i, numfiles;
   unsigned short crc;

   if (COM_FileOpenRead(packfile, &packhandle) == -1)
      goto error;

   rfread(&header, 1, sizeof(header), packhandle);
   if (header.id[0] != 'P' || header.id[1] != 'A'
         || header.id[2] != 'C' || header.id[3] != 'K')
      Sys_Error("%s is not a packfile", packfile);

   header.dirofs = LittleLong(header.dirofs);
   header.dirlen = LittleLong(header.dirlen);

   /* Defensive: the on-disk dirofs/dirlen are untrusted.
    * Negative or absurd values produce a negative numfiles,
    * an enormous Hunk_Alloc, or an rfread that overruns dfiles.
    * Bound numfiles to MAX_FILES_IN_PACK (the historical Quake
    * limit, 2048).  Reject malformed values outright rather
    * than trying to recover. */
   if (header.dirlen < 0 || header.dirofs < 0
       || header.dirlen % sizeof(dpackfile_t) != 0
       || header.dirlen / (int)sizeof(dpackfile_t) > 65536) {
      Sys_Error("%s has corrupt directory header", packfile);
   }

   numfiles = header.dirlen / sizeof(dpackfile_t);

   if (numfiles != PAK0_COUNT)
      com_modified = true;	/* not the original file */

   mfiles = (packfile_t*)Hunk_Alloc(numfiles * sizeof(*mfiles));
   mark   = Hunk_LowMark();
   dfiles = Hunk_Alloc(numfiles * sizeof(*dfiles));

   rfseek(packhandle, header.dirofs, SEEK_SET);
   rfread(dfiles, 1, header.dirlen, packhandle);

   /* crc the directory to check for modifications */
   crc = CRC_Block(((byte *)dfiles), header.dirlen);
   if (crc != NQ_PAK0_CRC)
      com_modified = true;

   /* parse the directory */
   for (i = 0; i < numfiles; i++)
   {
      snprintf(mfiles[i].name, sizeof(mfiles[i].name), "%s", dfiles[i].name);
      mfiles[i].filepos = LittleLong(dfiles[i].filepos);
      mfiles[i].filelen = LittleLong(dfiles[i].filelen);
   }

   Hunk_FreeToLowMark(mark);
   pack = (pack_t*)Hunk_Alloc(sizeof(pack_t));

   if (!pack)
      goto error;

   snprintf(pack->filename, sizeof(pack->filename), "%s", packfile);
   pack->numfiles = numfiles;
   pack->files = mfiles;

   Con_Printf("Added packfile %s (%i files)\n", packfile, numfiles);
   rfclose(packhandle);

   return pack;

error:
   if (packhandle)
      rfclose(packhandle);
   return NULL;
}


/*
================
COM_AddGameDirectory

Sets com_gamedir, adds the directory to the head of the path,
then loads and adds pak1.pak pak2.pak ...
================
*/
static void COM_AddGameDirectory(const char *base, const char *dir)
{
   int i;
   searchpath_t *search;
   char pakfile[MAX_OSPATH];
   pack_t *pak = NULL;
#ifdef _WIN32
   const char slash = '\\';
#else
   const char slash = '/';
#endif

   if (!base)
      return;

#ifdef _XBOX360
   if (!strcmp(dir, ""))
      strlcpy(com_gamedir, base, sizeof(com_gamedir));
   else
#endif
      strlcpy(com_gamedir, va("%s%c%s", base, slash, dir), sizeof(com_gamedir));

   /* add the directory to the search path */
   search = (searchpath_t*)Hunk_Alloc(sizeof(searchpath_t));
   strlcpy(search->filename, com_gamedir, sizeof(search->filename));
   search->next = com_searchpaths;
   com_searchpaths = search;

   /* add any pak files in the format pak0.pak pak1.pak, ... */
   {
      /* "pak2147483647.pak" + slash + NUL is at most 19 bytes. */
      size_t gamedir_len = strlen(com_gamedir);
      if (gamedir_len + 19 > sizeof(pakfile))
         return; /* path too long; no PAKs can fit */
   }
   for (i = 0;; i++)
   {
      snprintf(pakfile, sizeof(pakfile), "%s%cpak%i.pak", com_gamedir, slash, i);
      pak = COM_LoadPackFile(pakfile);
      if (!pak)
      {
         /* try uppercase */
         snprintf(pakfile, sizeof(pakfile), "%s%cPAK%i.PAK", com_gamedir, slash, i);
         pak = COM_LoadPackFile(pakfile);

         if (!pak) /* that doesn't work either? then break */
            break;
      }
      search = (searchpath_t*)Hunk_Alloc(sizeof(searchpath_t));
      search->pack = pak;
      search->next = com_searchpaths;
      com_searchpaths = search;
   }
}

/*
================
COM_Gamedir

Sets the gamedir and path to a different directory.

FIXME - if home dir is available, should we create ~/.tyrquake/gamedir ??
================
*/

/*
================
COM_InitFilesystem
================
*/

static void COM_InitFilesystem(void)
{
   int i;
   searchpath_t *search;

   /* Set save directory */
   strlcpy(com_savedir, host_parms.savedir, sizeof(com_savedir));
   
   /* -basedir <path> */
   /* Overrides the system supplied base directory (under id1) */
   i = COM_CheckParm("-basedir");
   strlcpy(com_basedir, host_parms.basedir, sizeof(com_basedir));

   /* start up with id1 by default */
   COM_AddGameDirectory(com_basedir, "id1");

   if (COM_CheckParm("-rogue"))
      COM_AddGameDirectory(com_basedir, "rogue");
   if (COM_CheckParm("-hipnotic"))
      COM_AddGameDirectory(com_basedir, "hipnotic");
   if (COM_CheckParm("-quoth"))
      COM_AddGameDirectory(com_basedir, "quoth");

   /* -game <gamedir> */
   /* Adds basedir/gamedir as an override game */
   i = COM_CheckParm("-game");
   if (i && i < com_argc - 1)
   {
      com_modified = true;
      COM_AddGameDirectory(com_basedir, com_argv[i + 1]);
   }
   
   /* Hack: Add save directory to search path, otherwise 'exec config.cfg' will fail */
   /* (NB: 'host_parms.use_exernal_savedir' is a bit of a kludge, but since basedir */
   /*  changes depending upon the game being loaded and various flags modify the */
   /*  final 'rom' directory, it's the cleanest way to prevent the same directory */
   /*  being added to the search list twice...) */
   if (host_parms.use_exernal_savedir != 0)
   {
		COM_AddGameDirectory(com_savedir, "");
	}
   
   /**/
   /* -path <dir or packfile> [<dir or packfile>] ... */
   /* Fully specifies the exact search path, overriding the generated one */
   /**/
   i = COM_CheckParm("-path");
   if (i) {
      com_modified = true;
      com_searchpaths = NULL;
      while (++i < com_argc) {
         if (!com_argv[i] || com_argv[i][0] == '+'
               || com_argv[i][0] == '-')
            break;

         search = (searchpath_t*)Hunk_Alloc(sizeof(searchpath_t));
         if (!strcmp(COM_FileExtension(com_argv[i]), "pak")) {
            search->pack = COM_LoadPackFile(com_argv[i]);
            if (!search->pack)
               Sys_Error("Couldn't load packfile: %s", com_argv[i]);
         } else
            strlcpy(search->filename, com_argv[i], sizeof(search->filename));
         search->next = com_searchpaths;
         com_searchpaths = search;
      }
   }
}

/* The following FS_*() stdio replacements are necessary if one is
 * to perform non-sequential reads on files reopened on pak files
 * because we need the bookkeeping about file start/end positions.
 * Allocating and filling in the fshandle_t structure is the users'
 * responsibility when the file is initially opened. */

size_t FS_fread(void *ptr, size_t size, size_t nmemb, fshandle_t *fh)
{
	long byte_size;
	long bytes_read;
	size_t nmemb_read;

	if (!fh) {
		errno = EBADF;
		return 0;
	}
	if (!ptr) {
		errno = EFAULT;
		return 0;
	}
	if (!size || !nmemb) {	/* no error, just zero bytes wanted */
		errno = 0;
		return 0;
	}

	byte_size = nmemb * size;
	if (byte_size > fh->length - fh->pos)	/* just read to end */
		byte_size = fh->length - fh->pos;
	bytes_read = rfread(ptr, 1, byte_size, fh->file);
	fh->pos += bytes_read;

	/* fread() must return the number of elements read,
	 * not the total number of bytes. */
	nmemb_read = bytes_read / size;
	/* even if the last member is only read partially
	 * it is counted as a whole in the return value. */
	if (bytes_read % size)
		nmemb_read++;

	return nmemb_read;
}

int FS_fseek(fshandle_t *fh, long offset, int whence)
{
/* I don't care about 64 bit off_t or fseeko() here.
 * the quake/hexen2 file system is 32 bits, anyway. */
	int ret;

	if (!fh) {
		errno = EBADF;
		return -1;
	}

	/* the relative file position shouldn't be smaller
	 * than zero or bigger than the filesize. */
	switch (whence)
	{
	case SEEK_SET:
		break;
	case SEEK_CUR:
		offset += fh->pos;
		break;
	case SEEK_END:
		offset = fh->length + offset;
		break;
	default:
		errno = EINVAL;
		return -1;
	}

	if (offset < 0) {
		errno = EINVAL;
		return -1;
	}

	if (offset > fh->length)	/* just seek to end */
		offset = fh->length;

	ret = rfseek(fh->file, fh->start + offset, SEEK_SET);
	if (ret < 0)
		return ret;

	fh->pos = offset;
	return 0;
}

long FS_ftell(fshandle_t *fh)
{
	if (!fh) {
		errno = EBADF;
		return -1;
	}
	return fh->pos;
}

void FS_rewind(fshandle_t *fh)
{
	if (!fh) return;
	rfseek(fh->file, fh->start, SEEK_SET);
	fh->pos = 0;
}

int FS_feof(fshandle_t *fh)
{
	if (!fh) {
		errno = EBADF;
		return -1;
	}
	if (fh->pos >= fh->length)
		return -1;
	return 0;
}

int FS_ferror(fshandle_t *fh)
{
	if (!fh) {
		errno = EBADF;
		return -1;
	}
	return rferror(fh->file);
}

int FS_fgetc(fshandle_t *fh)
{
	if (!fh) {
		errno = EBADF;
		return EOF;
	}
	if (fh->pos >= fh->length)
		return EOF;
	fh->pos += 1;
	return rfgetc(fh->file);
}

long FS_filelength (fshandle_t *fh)
{
	if (!fh) {
		errno = EBADF;
		return -1;
	}
	return fh->length;
}
