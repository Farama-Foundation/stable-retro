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
/* Quake is a trademark of Id Software, Inc., (c) 1996 Id Software, Inc. All */
/* rights reserved. */

#include <string.h>
#include <time.h>
#include <boolean.h>

#include "qtypes.h"
#include "cdaudio.h"
#include "cdaudio_driver.h"
#include "cmd.h"
#include "common.h"
#include "console.h"
#include "sound.h"

#include "client.h"

static byte remap[100];
static qboolean enabled = false;
static qboolean initialized = false;
static qboolean playing = false;
static qboolean wasPlaying = false;
static qboolean playLooping = false;
static qboolean cdValid = false;
static byte maxTrack;
static byte playTrack;
static float cdvolume;

static void
CDAudio_Eject(void)
{
    if (enabled)
	CDDrv_Eject();
}

static void
CDAudio_CloseDoor(void)
{
    if (enabled)
	CDDrv_CloseDoor();
}

static int
CDAudio_GetAudioDiskInfo(void)
{
    int err;

    cdValid = false;
    err = CDDrv_GetMaxTrack(&maxTrack);
    if (!err)
	cdValid = true;

    return err;
}

void
CDAudio_Stop(void)
{
    if (!enabled)
	return;
    if (!playing)
	return;

    CDDrv_Stop();
    wasPlaying = false;
    playing = false;
}

void
CDAudio_Pause(void)
{
    if (!enabled)
	return;
    if (!playing)
	return;

    CDDrv_Pause();
    wasPlaying = playing;
    playing = false;
}

void
CDAudio_Resume(void)
{
    if (!enabled)
	return;
    if (!cdValid)
	return;
    if (!wasPlaying)
	return;

    CDDrv_Resume(playTrack);
    playing = true;
}


int
CDAudio_Play(byte track, qboolean looping)
{
    int err;

    if (!enabled)
	return -1;

    if (!cdValid) {
	CDAudio_GetAudioDiskInfo();
	if (!cdValid)
	    return -1;
    }
    track = remap[track];
    if (track < 1 || track > maxTrack) {
	Con_DPrintf("CDAudio: Bad track number %u.\n", track);
	return -1;
    }
    if (!CDDrv_IsAudioTrack(track)) {
	Con_Printf("CDAudio: track %i is not audio\n", track);
	return -1;
    }
    if (playing) {
	if (playTrack == track)
	    return -1;
	CDAudio_Stop();
    }
    err = CDDrv_PlayTrack(track);
    if (!err) {
	playLooping = looping;
	playTrack = track;
	playing = true;
    }
    if (cdvolume == 0.0)
	CDAudio_Pause();
    return 0;
}

void
CDAudio_Update(void)
{
    static time_t lastchk;

    if (!enabled)
	return;

    if (playing && lastchk < time(NULL)) {
	lastchk = time(NULL) + 2;	/* two seconds between chks */
	if (!CDDrv_IsPlaying(playTrack)) {
	    playing = false;
	    if (playLooping)
		CDAudio_Play(playTrack, true);
	}
    }
}

static void
CD_f(void)
{
   const char *command;
   int n;

   if (Cmd_Argc() < 2)
      return;

   command = Cmd_Argv(1);

   if (strcasecmp(command, "on") == 0) {
      enabled = true;
      return;
   }
   if (strcasecmp(command, "off") == 0) {
      if (playing)
         CDAudio_Stop();
      enabled = false;
      return;
   }
   if (strcasecmp(command, "reset") == 0) {
      enabled = true;
      if (playing)
         CDAudio_Stop();
      for (n = 0; n < 100; n++)
         remap[n] = n;
      CDAudio_GetAudioDiskInfo();
      return;
   }
   if (strcasecmp(command, "remap") == 0)
   {
      int ret = Cmd_Argc() - 2;
      if (ret <= 0) {
         for (n = 1; n < 100; n++)
            if (remap[n] != n)
               Con_Printf("  %u -> %u\n", n, remap[n]);
         return;
      }
      for (n = 1; n <= ret; n++)
         remap[n] = Q_atoi(Cmd_Argv(n + 1));
      return;
   }
   if (strcasecmp(command, "close") == 0) {
      CDAudio_CloseDoor();
      return;
   }
   if (!cdValid) {
      CDAudio_GetAudioDiskInfo();
      if (!cdValid) {
         Con_Printf("No CD in player.\n");
         return;
      }
   }
   if (strcasecmp(command, "play") == 0) {
      CDAudio_Play((byte)Q_atoi(Cmd_Argv(2)), false);
      return;
   }
   if (strcasecmp(command, "loop") == 0) {
      CDAudio_Play((byte)Q_atoi(Cmd_Argv(2)), true);
      return;
   }
   if (strcasecmp(command, "stop") == 0) {
      CDAudio_Stop();
      return;
   }
   if (strcasecmp(command, "pause") == 0) {
      CDAudio_Pause();
      return;
   }
   if (strcasecmp(command, "resume") == 0) {
      CDAudio_Resume();
      return;
   }
   if (strcasecmp(command, "eject") == 0) {
      if (playing)
         CDAudio_Stop();
      CDAudio_Eject();
      cdValid = false;
      return;
   }
   if (strcasecmp(command, "info") == 0) {
      Con_Printf("%u tracks\n", maxTrack);
      if (playing)
         Con_Printf("Currently %s track %u\n",
               playLooping ? "looping" : "playing", playTrack);
      else if (wasPlaying)
         Con_Printf("Paused %s track %u\n",
               playLooping ? "looping" : "playing", playTrack);
      Con_Printf("Volume is %f\n", bgmvolume.value);
      return;
   }
}

int
CDAudio_Init(void)
{
    int i, err;

    /* FIXME - not a valid client state in QW? */
    if (cls.state == ca_dedicated)
	return -1;

    if (COM_CheckParm("-nocdaudio"))
	return -1;

    Cmd_AddCommand("cd", CD_f);
    err = CDDrv_InitDevice();
    if (err)
	return err;

    for (i = 0; i < 100; i++)
	remap[i] = i;
    initialized = true;
    enabled = true;

    Con_Printf("CD Audio Initialized\n");
    if (CDAudio_GetAudioDiskInfo()) {
	Con_Printf("CDAudio_Init: No CD in player.\n");
	cdValid = false;
    }

    return 0;
}

void
CDAudio_Shutdown(void)
{
    if (!initialized)
	return;
    CDAudio_Stop();
    CDDrv_CloseDevice();
    initialized = false;
}
