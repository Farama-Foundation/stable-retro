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

#include "compat/strl.h"

#include "client.h"
#include "cmd.h"
#include "console.h"
#include "host.h"
#include "net.h"
#include "protocol.h"
#include "quakedef.h"
#include "sys.h"
#include "zone.h"

#include <streams/file_stream.h>

/* forward declarations */
int rfclose(RFILE* stream);
int64_t rfread(void* buffer,
   size_t elem_size, size_t elem_count, RFILE* stream);
int64_t rfflush(RFILE * stream);
int64_t rfwrite(void const* buffer,
   size_t elem_size, size_t elem_count, RFILE* stream);
int rfprintf(RFILE * stream, const char * format, ...);
int rfgetc(RFILE* stream);

static void CL_FinishTimeDemo(void);

/*
==============================================================================

DEMO CODE

When a demo is playing back, all NET_SendMessages are skipped, and
NET_GetMessages are read from the demo file.

Whenever cl.time gets past the last received message, another message is
read from the demo file.
==============================================================================
*/

/*
==============
CL_StopPlayback

Called when a demo file runs out, or the user starts a game
==============
*/
void
CL_StopPlayback(void)
{
    if (!cls.demoplayback)
	return;

    rfclose(cls.demofile);
    cls.demoplayback = false;
    cls.demofile     = NULL;
    cls.state        = ca_disconnected;

    if (cls.timedemo)
	CL_FinishTimeDemo();
}

/*
====================
CL_GetMessage

Handles recording and playback of demos, on top of NET_ code
====================
*/
int
CL_GetMessage(void)
{
   int r;

   if (cls.demoplayback)
   {
      int i;
      float f;

      /* decide if it is time to grab the next message */
      /* allways grab until fully connected */
      if (cls.state == ca_active)
      {
         if (cls.timedemo)
         {
            if (host_framecount == cls.td_lastframe)
               return 0;	/* allready read this frame's message */

            cls.td_lastframe = host_framecount;

            /* if this is the second frame, grab the real td_starttime */
            /* so the bogus time on the first frame doesn't count */
            if (host_framecount == cls.td_startframe + 1)
               cls.td_starttime = realtime;
         }
         else if (cl.time <= cl.mtime[0])
         {
            /* don't need another message yet */
            return 0;
         }
      }
      /* get the next message */
      rfread(&net_message.cursize, 4, 1, cls.demofile);
      VectorCopy(cl.mviewangles[0], cl.mviewangles[1]);

      for (i = 0; i < 3; i++) {
         rfread(&f, 4, 1, cls.demofile);
         cl.mviewangles[0][i] = LittleFloat(f);
         /* Demo viewangles are loaded raw from the file (bit
          * pattern preserved). A malformed or hostile demo can
          * land NaN/Inf here, which then flows through
          * CL_RelinkEntities's lerp:
          *   d = cl.mviewangles[0][j] - cl.mviewangles[1][j];
          *   cl.viewangles[j] = cl.mviewangles[1][j] + frac * d;
          * NaN propagates into cl.viewangles, feeds AngleVectors
          * and the screen transforms, and the rasterizer takes
          * non-finite inputs the way the MSG_ReadFloat audit
          * already documented. Clamp to 0 -- losing one frame
          * of view-angle continuity from a broken demo is the
          * least-bad recovery. */
         if (IS_NAN(cl.mviewangles[0][i]))
            cl.mviewangles[0][i] = 0.0f;
      }

      net_message.cursize = LittleLong(net_message.cursize);

      if (net_message.cursize > MAX_MSGLEN)
         Sys_Error("Demo message > MAX_MSGLEN");
      /* Defensive: a corrupt or hostile demo can put a negative
       * cursize on the wire.  Without this check, rfread is
       * called with size = (size_t)cursize -- a negative int
       * cast to size_t becomes a huge value and overflows
       * net_message.data[].  The upper bound above already
       * caught absurd positive values; mirror that for negatives. */
      if (net_message.cursize < 0) {
         CL_StopPlayback();
         return 0;
      }
      r = rfread(net_message.data, net_message.cursize, 1, cls.demofile);
      if (r != 1) {
         CL_StopPlayback();
         return 0;
      }

      return 1;
   }

   while (1)
   {
      r = NET_GetMessage(cls.netcon);

      if (r != 1 && r != 2)
         return r;

      /* discard nop keepalive message */
      if (net_message.cursize == 1 && net_message.data[0] == svc_nop)
         Con_Printf("<-- server to client keepalive\n");
      else
         break;
   }

   return r;
}

/*
====================
CL_PlayDemo_f

play [demoname]
====================
*/
void
CL_PlayDemo_f(void)
{
    char name[256];
    int c;
    qboolean neg = false;

    if (cmd_source != src_command)
	return;

    if (Cmd_Argc() != 2) {
	Con_Printf("play <demoname> : plays a demo\n");
	return;
    }
/**/
/* disconnect from server */
/**/
    CL_Disconnect();

/**/
/* open the demo file */
/**/
    strlcpy(name, Cmd_Argv(1), sizeof(name));
    COM_DefaultExtension(name, sizeof(name), ".dem");

    Con_Printf("Playing demo from %s.\n", name);
    COM_FOpenFile(name, &cls.demofile);
    if (!cls.demofile) {
	Con_Printf("ERROR: couldn't open.\n");
	cls.demonum = -1;	/* stop demo loop */
	return;
    }

    cls.demoplayback = true;
    cls.state = ca_connected;
    cls.forcetrack = 0;

    /* Defensive: bound the header parse.  The original loop has
     * no EOF check (rfgetc returns -1, which is not '\n' so the
     * loop spins forever on a truncated file) and no iteration
     * cap (a hostile demo of solid digits could overflow
     * forcetrack into UB and stall the loader).  Cap the read at
     * a generous fixed count and treat EOF / control characters
     * as terminators. */
    {
	int iter;
	for (iter = 0; iter < 32; iter++) {
	    c = rfgetc(cls.demofile);
	    if (c == EOF || c == '\n' || c == '\0')
		break;
	    if (c == '-') {
		neg = true;
	    } else if (c >= '0' && c <= '9') {
		/* Saturate rather than overflow on absurd input. */
		if (cls.forcetrack < 100000)
		    cls.forcetrack = cls.forcetrack * 10 + (c - '0');
	    }
	    /* silently ignore other characters */
	}
    }

    if (neg)
	cls.forcetrack = -cls.forcetrack;
}

struct stree_root *
CL_Demo_Arg_f(const char *arg)
{
    struct stree_root *root;

    root = (struct stree_root*)Z_Malloc(sizeof(struct stree_root));
    if (root)
    {
    root->entries = 0;
    root->maxlen = 0;
    root->minlen = -1;
    /* root->root = NULL; */
    root->stack = NULL;
	STree_AllocInit();
	COM_ScanDir(root, "", arg, ".dem", true);
    }

    return root;
}

/*
====================
CL_FinishTimeDemo

====================
*/
static void CL_FinishTimeDemo(void)
{
    int frames;
    float time;

    cls.timedemo = false;

/* the first frame didn't count */
    frames = (host_framecount - cls.td_startframe) - 1;
    time = realtime - cls.td_starttime;
    if (!time)
	time = 1;
    Con_Printf("%i frames %5.1f seconds %5.1f fps\n", frames, time,
	       frames / time);
}

/*
====================
CL_TimeDemo_f

timedemo [demoname]
====================
*/
void
CL_TimeDemo_f(void)
{
    if (cmd_source != src_command)
	return;

    if (Cmd_Argc() != 2) {
	Con_Printf("timedemo <demoname> : gets demo speeds\n");
	return;
    }

    CL_PlayDemo_f();

/* cls.td_starttime will be grabbed at the second frame of the demo, so */
/* all the loading time doesn't get counted */

    cls.timedemo = true;
    cls.td_startframe = host_framecount;
    cls.td_lastframe = -1;	/* get a new message this frame */
}
