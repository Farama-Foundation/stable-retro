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
/* host.c -- coordinates spawning and killing of local servers */

#include "cdaudio.h"
#include "cmd.h"
#include "console.h"
#include "draw.h"
#include "host.h"
#include "input.h"
#include "keys.h"
#include "menu.h"
#include "model.h"
#include "net.h"
#include "protocol.h"
#include "quakedef.h"
#include "sbar.h"
#include "screen.h"
#include "server.h"
#include "sound.h"
#include "bgmusic.h"
#include "sys.h"
#include "view.h"
#include "wad.h"

#include "r_local.h"
#include "render.h"
#include "perf_timing.h"

#include <streams/file_stream.h>

/* forward declarations */
RFILE* rfopen(const char *path, const char *mode);
int rfclose(RFILE* stream);
int rfprintf(RFILE * stream, const char * format, ...);

/*
 * A server can always be started, even if the system started out as a client
 * to a remote system.
 *
 * A client can NOT be started if the system started as a dedicated server.
 *
 * Memory is cleared/released when a server or client begins, not when they
 * end.
 */

quakeparms_t host_parms;

qboolean host_initialized;	/* true if into command execution */

double host_frametime;
double host_time;
double realtime;		/* without any filtering or bounding */
int host_framecount;

int host_hunklevel;

int minimum_memory;

client_t *host_client;		/* current client */

static jmp_buf host_abort;

byte *host_basepal;
byte *host_colormap;

cvar_t fraglimit = { "fraglimit", "0", false, true };
cvar_t timelimit = { "timelimit", "0", false, true };
cvar_t teamplay = { "teamplay", "0", false, true };

/* QC-visible cvars (read by id1/progs.dat via the
 * cvar() builtin in PF_cvar / PF_cvar_set).  No
 * engine-side reader; do not remove. */
cvar_t samelevel = { "samelevel", "0" };
cvar_t noexit = { "noexit", "0", false, true };

cvar_t developer = { "developer", "0" };

cvar_t skill = { "skill", "1" };	/* 0 - 3 */
cvar_t deathmatch = { "deathmatch", "0" };	/* 0, 1, or 2 */
cvar_t coop = { "coop", "0" };	/* 0 or 1 */

cvar_t pausable = { "pausable", "1" };

cvar_t temp1 = { "temp1", "0" };


/*
================
Host_EndGame
================
*/
void
Host_EndGame(const char *message, ...)
{
    va_list argptr;
    char string[MAX_PRINTMSG];

    va_start(argptr, message);
    vsnprintf(string, sizeof(string), message, argptr);
    va_end(argptr);
    Con_DPrintf("%s: %s\n", __func__, string);

    if (sv.active)
	Host_ShutdownServer(false);

    if (cls.state == ca_dedicated)
	Sys_Error("%s: %s", __func__, string); /* dedicated servers exit */

    if (cls.demonum != -1)
	CL_NextDemo();
    else
	CL_Disconnect();

    longjmp(host_abort, 1);
}

/*
================
Host_Error

This shuts down both the client and server
================
*/
void
Host_Error(const char *error, ...)
{
    va_list argptr;
    char string[MAX_PRINTMSG];
    static qboolean inerror = false;

    if (inerror)
	Sys_Error("%s: recursively entered", __func__);
    inerror = true;

    SCR_EndLoadingPlaque();	/* reenable screen updates */

    va_start(argptr, error);
    vsnprintf(string, sizeof(string), error, argptr);
    va_end(argptr);
    Con_Printf("%s: %s\n", __func__, string);

    if (sv.active)
	Host_ShutdownServer(false);

    if (cls.state == ca_dedicated)
	Sys_Error("%s: %s", __func__, string); /* dedicated servers exit */

    CL_Disconnect();
    cls.demonum = -1;

    inerror = false;

    longjmp(host_abort, 1);
}

/*
================
Host_FindMaxClients
================
*/
static void
Host_FindMaxClients(void)
{
    int i;

    svs.maxclients = 1;

    i = COM_CheckParm("-dedicated");
    if (i) {
	cls.state = ca_dedicated;
	if (i != (com_argc - 1)) {
	    svs.maxclients = Q_atoi(com_argv[i + 1]);
	} else
	    svs.maxclients = 8;
    } else
	cls.state = ca_disconnected;

    i = COM_CheckParm("-listen");
    if (i) {
	if (cls.state == ca_dedicated)
	    Sys_Error("Only one of -dedicated or -listen can be specified");
	if (i != (com_argc - 1))
	    svs.maxclients = Q_atoi(com_argv[i + 1]);
	else
	    svs.maxclients = 8;
    }
    if (svs.maxclients < 1)
	svs.maxclients = 8;
    else if (svs.maxclients > MAX_SCOREBOARD)
	svs.maxclients = MAX_SCOREBOARD;

    svs.maxclientslimit = svs.maxclients;
    if (svs.maxclientslimit < 4)
	svs.maxclientslimit = 4;
    svs.clients = (client_t*)Hunk_Alloc(svs.maxclientslimit * sizeof(client_t));

    if (svs.maxclients > 1)
	Cvar_SetValue("deathmatch", 1.0);
    else
	Cvar_SetValue("deathmatch", 0.0);
}


/*
=======================
Host_InitLocal
======================
*/
static void
Host_InitLocal(void)
{
    Host_InitCommands();

    Cvar_RegisterVariable(&fraglimit);
    Cvar_RegisterVariable(&timelimit);
    Cvar_RegisterVariable(&teamplay);
    Cvar_RegisterVariable(&samelevel);
    Cvar_RegisterVariable(&noexit);
    Cvar_RegisterVariable(&skill);
    Cvar_RegisterVariable(&deathmatch);
    Cvar_RegisterVariable(&coop);

    Cvar_RegisterVariable(&pausable);

    Cvar_RegisterVariable(&temp1);

    Cvar_RegisterVariable(&developer);
    if (COM_CheckParm("-developer"))
	Cvar_SetValue("developer", 1);

    Host_FindMaxClients();

    host_time = 1.0;		/* so a think at time 0 won't get called */
}


/*
===============
Host_WriteConfiguration

Writes key bindings and archived cvars to config.cfg
===============
*/
static void
Host_WriteConfiguration(void)
{
    RFILE *f;

/* dedicated servers initialize the host but don't parse and set the */
/* config.cfg cvars */
    if (host_initialized & !isDedicated) {
	f = rfopen(va("%s/config.cfg", com_savedir), "w");
	if (!f) {
	    Con_Printf("Couldn't write config.cfg.\n");
	    return;
	}

	Key_WriteBindings(f);
	Cvar_WriteVariables(f);

	/* Save the mlook state (rarely used as an actual key binding) */
	if (in_mlook.state & 1)
	    rfprintf(f, "+mlook\n");

	rfclose(f);
    }
}


/*
=================
SV_ClientPrintf

Sends text across to be displayed
FIXME: make this just a stuffed echo?
=================
*/
void
SV_ClientPrintf(const char *fmt, ...)
{
    va_list argptr;

    MSG_WriteByte(&host_client->message, svc_print);
    va_start(argptr, fmt);
    MSG_WriteStringvf(&host_client->message, fmt, argptr);
    va_end(argptr);
}

/*
=================
SV_BroadcastPrintf

Sends text to all active clients
=================
*/
void
SV_BroadcastPrintf(const char *fmt, ...)
{
    va_list argptr;
    int i;

    for (i = 0; i < svs.maxclients; i++)
	if (svs.clients[i].active && svs.clients[i].spawned) {
	    MSG_WriteByte(&svs.clients[i].message, svc_print);
	    va_start(argptr, fmt);
	    MSG_WriteStringvf(&svs.clients[i].message, fmt, argptr);
	    va_end(argptr);
	}
}

/*
=================
Host_ClientCommands

Send text over to the client to be executed
=================
*/
void
Host_ClientCommands(const char *fmt, ...)
{
    va_list argptr;

    MSG_WriteByte(&host_client->message, svc_stufftext);
    va_start(argptr, fmt);
    MSG_WriteStringvf(&host_client->message, fmt, argptr);
    va_end(argptr);
}

/*
=====================
SV_DropClient

Called when the player is getting totally kicked off the host
if (crash = true), don't bother sending signofs
=====================
*/
void
SV_DropClient(qboolean crash)
{
    int saveSelf;
    int i;
    client_t *client;

    if (!crash) {
	/* send any final messages (don't check for errors) */
	if (NET_CanSendMessage(host_client->netconnection)) {
	    MSG_WriteByte(&host_client->message, svc_disconnect);
	    NET_SendMessage(host_client->netconnection,
			    &host_client->message);
	}

	if (host_client->edict && host_client->spawned) {
	    /* call the prog function for removing a client */
	    /* this will set the body to a dead frame, among other things */
	    saveSelf = pr_global_struct->self;
	    pr_global_struct->self = EDICT_TO_PROG(host_client->edict);
	    PR_ExecuteProgram(pr_global_struct->ClientDisconnect);
	    pr_global_struct->self = saveSelf;
	}

	Sys_Printf("Client %s removed\n", host_client->name);
    }
/* break the net connection */
    NET_Close(host_client->netconnection);
    host_client->netconnection = NULL;

/* free the client (the body stays around) */
    host_client->active = false;
    host_client->name[0] = 0;
    host_client->old_frags = -999999;
    net_activeconnections--;

/* send notification to all clients */
    for (i = 0, client = svs.clients; i < svs.maxclients; i++, client++) {
	if (!client->active)
	    continue;
	MSG_WriteByte(&client->message, svc_updatename);
	MSG_WriteByte(&client->message, host_client - svs.clients);
	MSG_WriteString(&client->message, "");
	MSG_WriteByte(&client->message, svc_updatefrags);
	MSG_WriteByte(&client->message, host_client - svs.clients);
	MSG_WriteShort(&client->message, 0);
	MSG_WriteByte(&client->message, svc_updatecolors);
	MSG_WriteByte(&client->message, host_client - svs.clients);
	MSG_WriteByte(&client->message, 0);
    }
}

/*
==================
Host_ShutdownServer

This only happens at the end of a game, not between levels
==================
*/
void
Host_ShutdownServer(qboolean crash)
{
    int i;
    int count;
    sizebuf_t buf;
    byte message[4];

    if (!sv.active)
	return;

    sv.active = false;

/* stop all client sounds immediately */
    if (cls.state >= ca_connected)
	CL_Disconnect();

/* flush any pending messages - like the score!!!
 * Bounded by an iteration cap rather than a 3-second
 * wall-clock deadline.  In SP play with the loopback
 * driver count is 0 from the start (NET_CanSendMessage
 * always returns true) and the loop never actually
 * iterates.  For a real-network session with an
 * unresponsive client, 30000 spins is plenty for the
 * driver to either drain or fail; the historical
 * 3-second timeout served the same emergency role. */
    {
	int iters = 0;
	do {
	    count = 0;
	    for (i = 0, host_client = svs.clients; i < svs.maxclients;
		 i++, host_client++) {
		if (host_client->active && host_client->message.cursize) {
		    if (NET_CanSendMessage(host_client->netconnection)) {
			NET_SendMessage(host_client->netconnection,
					&host_client->message);
			SZ_Clear(&host_client->message);
		    } else {
			NET_GetMessage(host_client->netconnection);
			count++;
		    }
		}
	    }
	    if (++iters > 30000)
		break;
	} while (count);
    }

/* make sure all the clients know we're disconnecting */
    buf.data = message;
    buf.maxsize = 4;
    buf.cursize = 0;
    MSG_WriteByte(&buf, svc_disconnect);
    count = NET_SendToAll(&buf, 5);
    if (count)
	Con_Printf("%s: NET_SendToAll failed for %u clients\n", __func__,
		   count);

    for (i = 0, host_client = svs.clients; i < svs.maxclients;
	 i++, host_client++)
	if (host_client->active)
	    SV_DropClient(crash);

/**/
/* clear structures */
/**/
    memset(&sv, 0, sizeof(sv));
    memset(svs.clients, 0, svs.maxclientslimit * sizeof(client_t));
}


/*
================
Host_ClearMemory

This clears all the memory used by both the client and server, but does
not reinitialize anything.
================
*/
void
Host_ClearMemory(void)
{
    Con_DPrintf("Clearing memory\n");
    D_FlushCaches();
    Mod_ClearAll();
    if (host_hunklevel)
	Hunk_FreeToLowMark(host_hunklevel);

    cls.signon = 0;
    memset(&sv, 0, sizeof(sv));
    memset(&cl, 0, sizeof(cl));
}


/* ============================================================================ */


/*
===================
Host_FilterTime
===================
*/
static void Host_FilterTime(float time)
{
    static double oldrealtime; /* last frame run */
    realtime += time;

    host_frametime = realtime - oldrealtime;
    oldrealtime    = realtime;

    /* don't allow really long or short frames -- this
     * is a defence against degenerate values from
     * Host_Frame's caller (libretro retro_run passes
     * 1.0/framerate, well within these bounds for any
     * sane framerate setting). */
    if (host_frametime > 0.1)
	host_frametime = 0.1;
    if (host_frametime < 0.001)
	host_frametime = 0.001;
}


/*
===================
Host_GetConsoleCommands

Add them exactly as if they had been typed at the console
===================
*/
static void
Host_GetConsoleCommands(void)
{
    char *cmd;

    while (1) {
	cmd = Sys_ConsoleInput();
	if (!cmd)
	    break;
	Cbuf_AddText("%s", cmd);
    }
}


/*
==================
Host_ServerFrame

==================
*/
void
Host_ServerFrame(void)
{
    /* run the world state */
    pr_global_struct->frametime = host_frametime;

    /* set the time and clear the general datagram */
    SV_ClearDatagram();

    /* check for new clients */
    SV_CheckForNewClients();

    /* read client messages */
    SV_RunClients();

    /*
     * Move things around and think. Always pause in single player if in
     * console or menus
     */
    if (!sv.paused && (svs.maxclients > 1 || key_dest == key_game))
	SV_Physics();

    /* send all messages to the clients */
    SV_SendClientMessages();
}


/*
==================
Host_Frame

Runs all active servers
==================
*/
void Host_Frame(float time)
{
   /* something bad happened, or the server disconnected */
   if (setjmp(host_abort))
      return;

   /* keep the random time dependent */
   rand();

   /*
    * Decide the simulation time. Don't run too fast, or packets will flood
    * out.
    */
   Host_FilterTime(time);

   /* Age the console notify-line elapsed counters by the same delta
    * Host_FilterTime just folded into realtime, so notification lines
    * age in real time regardless of whether we're connected to a
    * server. */
   Con_AgeNotifyTimes(host_frametime);
   SCR_AgeLoadingPlaque(host_frametime);

   /* get new key events */
   Sys_SendKeyEvents();

   /* allow mice or other external controllers to add commands */
   IN_Commands();

   /* process console commands */
   Cbuf_Execute();

   NET_Poll();

   /* if running the server locally, make intentions now */
   if (sv.active)
      CL_SendCmd();

   /* ------------------- */
   /**/
   /* server operations */
   /**/
   /* ------------------- */

   /* check for commands typed to the host */
   Host_GetConsoleCommands();

   if (sv.active)
      Host_ServerFrame();

   /* ------------------- */
   /**/
   /* client operations */
   /**/
   /* ------------------- */

   /*
    * if running the server remotely, send intentions now after the incoming
    * messages have been read
    */
   if (!sv.active)
      CL_SendCmd();

   host_time += host_frametime;

   /* fetch results from server */
   if (cls.state >= ca_connected)
      CL_ReadFromServer();

   SCR_UpdateScreen();
   perf_timing_section_begin(PERF_SECTION_HOST_PT);
   CL_RunParticles();
   perf_timing_section_end(PERF_SECTION_HOST_PT);

   host_framecount++;
}


/*
====================
Host_Init
====================
*/

extern int host_fullbrights;

bool
Host_Init(quakeparms_t *parms)
{
    if (standard_quake)
	minimum_memory = MINIMUM_MEMORY;
    else
	minimum_memory = MINIMUM_MEMORY_LEVELPAK;

    if (COM_CheckParm("-minmemory"))
	parms->memsize = minimum_memory;

    host_parms = *parms;

    if (parms->memsize < minimum_memory) {
       Sys_Error("Only %4.1f megs of memory reported, can't execute game",
             parms->memsize / (float)0x100000);
       return false;
    }

    com_argc = parms->argc;
    com_argv = parms->argv;

    Memory_Init(parms->membase, parms->memsize);
    Cbuf_Init();
    Cmd_Init();
    V_Init();
    Chase_Init();
    COM_Init();
    Host_InitLocal();
    if (!W_LoadWadFile("gfx.wad"))
       return false;

    Key_Init();
    Con_Init();
    M_Init();
    PR_Init();
    Mod_Init(R_ModelLoader());
    NET_Init();
    SV_Init();

    Con_Printf("Exe: " __TIME__ " " __DATE__ "\n");
    Con_Printf("%4.1f megabyte heap\n", parms->memsize / (1024 * 1024.0));

    R_InitTextures();		/* needed even for dedicated servers */

    if (cls.state != ca_dedicated) {
	host_basepal = (byte*)COM_LoadHunkFile("gfx/palette.lmp");
	if (!host_basepal) {
	    Sys_Error("Couldn't load gfx/palette.lmp");
	    return false;
	}
	/* palette.lmp is exactly 256 RGB triples = 768 bytes.
	 * Consumers (V_UpdatePalette, the 24bit-table builder
	 * d_8to24table, every renderer that reads
	 * host_basepal[i*3+{0,1,2}] for i in [0,256)) walk
	 * 768 bytes unconditionally; a truncated file lets
	 * them OOB-read into adjacent hunk memory. */
	if (com_filesize != 768) {
	    Sys_Error("gfx/palette.lmp has bad size %d (expected 768)",
	                     com_filesize);
	    return false;
	}

	host_colormap = (byte*)COM_LoadHunkFile("gfx/colormap.lmp");
	if (!host_colormap) {
	    Sys_Error("Couldn't load gfx/colormap.lmp");
	    return false;
	}
	/* colormap.lmp is exactly 64 shade-rows * 256 colors
	 * + 1 trailing fullbrights byte = 16385 bytes.  The
	 * fullbrights read at host_colormap[16384] right
	 * below would OOB on a truncated file; the renderer
	 * indexes vid.colormap[(light & 0xFF00) + pix] up to
	 * 64 * 256 - 1 = 16383, also requiring the full
	 * extent. */
	if (com_filesize != 16385) {
	    Sys_Error("gfx/colormap.lmp has bad size %d (expected 16385)",
	                     com_filesize);
	    return false;
	}


   /* Always derive host_fullbrights from the colormap.  This is the
    * fullbright-pixel cutoff used by both the grayscale rasterizer
    * (via colormap row indexing) and the RGB rasterizers (via the
    * `if (*lptex < host_fullbrights)` check).  Setting it
    * unconditionally lets r_coloredlight be toggled at runtime
    * without leaving the rasterizers with a bogus value of 0. */
   host_fullbrights = 256 - host_colormap[16384];

	VID_Init(host_basepal);

	Draw_Init();
	SCR_Init();
	R_Init();

	S_Init();
	CDAudio_Init();
	BGM_Init();

	Sbar_Init();
	CL_Init();

	IN_Init();
    }

    Hunk_Alloc(0);
    host_hunklevel = Hunk_LowMark();

    host_initialized = true;
    Sys_Printf("========Quake Initialized=========\n");

    /* In case exec of quake.rc fails */
    if (!setjmp(host_abort)) {
	Cbuf_InsertText("exec quake.rc\n");
	Cbuf_Execute();
    }

    return true;
}


/*
===============
Host_Shutdown

FIXME: this is a callback from Sys_Quit and Sys_Error.  It would be better
to run quit through here before the final handoff to the sys code.
===============
*/
void
Host_Shutdown(void)
{
    /* Recursion guard: Sys_Error during shutdown re-enters Sys_Quit.
     * Reset to false at the end so a subsequent libretro deinit/init
     * cycle (statically-linked targets) actually runs the shutdown
     * chain again, rather than no-oping and leaking audio/video/
     * network resources from the previous session. */
    static qboolean isdown = false;

    if (isdown)
	return;
    isdown = true;

    /* keep Con_Printf from trying to update the screen */
    scr_disabled_for_loading = true;

    Host_WriteConfiguration();

    CDAudio_Shutdown();
    NET_Shutdown();
    BGM_Shutdown();
    S_Shutdown();
    IN_Shutdown();

    if (cls.state != ca_dedicated)
	VID_Shutdown();

    Mod_Shutdown();
    Cmd_Shutdown();
    Cvar_Shutdown();

    isdown = false;
}
