/******************************************************************************/
/* Mednafen - Multi-system Emulator                                           */
/******************************************************************************/
/* CDAccess_CHD.cpp:
**  Copyright (C) 2017 Romain Tisserand
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software Foundation, Inc.,
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "../mednafen.h"
#include "../general.h"

#include <stdio.h>

#include "CDAccess_CHD.h"

// Disk-image(rip) track/sector formats
enum
{
  DI_FORMAT_AUDIO = 0x00,
  DI_FORMAT_MODE1 = 0x01,
  DI_FORMAT_MODE1_RAW = 0x02,
  DI_FORMAT_MODE2 = 0x03,
  DI_FORMAT_MODE2_FORM1 = 0x04,
  DI_FORMAT_MODE2_FORM2 = 0x05,
  DI_FORMAT_MODE2_RAW = 0x06,
  DI_FORMAT_CDI_RAW = 0x07,
  _DI_FORMAT_COUNT
};

static const int32_t DI_Size_Table[8] =
{
  2352, // Audio
  2048, // MODE1
  2352, // MODE1 RAW
  2336, // MODE2
  2048, // MODE2 Form 1
  2324, // MODE2 Form 2
  2352, // MODE2 RAW
  2352  // CD-I RAW
};

CDAccess_CHD::CDAccess_CHD(const std::string &path, bool image_memcache) : NumTracks(0), total_sectors(0)
{
  Load(path, image_memcache);
}

bool CDAccess_CHD::Load(const std::string &path, bool image_memcache)
{
  chd_error err = chd_open(path.c_str(), CHD_OPEN_READ, NULL, &chd);
  if (err != CHDERR_NONE)
  {
    log_cb(RETRO_LOG_ERROR, "Failed to load CHD image: %s", path.c_str());
    return false;
  }

  if (image_memcache)
  {
    err = chd_precache(chd);

    if (err != CHDERR_NONE)
    {
      log_cb(RETRO_LOG_ERROR, "Failed to pre-cache CHD image: %s", path.c_str());
      return false;
    }
  }

  /* allocate storage for sector reads */
  const chd_header *head = chd_get_header(chd);
  hunkmem = (uint8_t *)malloc(head->hunkbytes);
  oldhunk = -1;

  log_cb(RETRO_LOG_INFO, "chd_load '%s' hunkbytes=%d\n", path.c_str(), head->hunkbytes);

  int plba = -150;
  int numsectors = 0;
  int chd_offset = 0;
  while (1)
  {
    int tkid = 0, frames = 0, pad = 0, pregap = 0, postgap = 0;
    char type[64], subtype[32], pgtype[32], pgsub[32];
    char tmp[512];

    err = chd_get_metadata(chd, CDROM_TRACK_METADATA2_TAG, NumTracks, tmp, sizeof(tmp), NULL, NULL, NULL);
    if (err == CHDERR_NONE)
    {
      sscanf(tmp, CDROM_TRACK_METADATA2_FORMAT, &tkid, type, subtype, &frames, &pregap, pgtype, pgsub, &postgap);
    }
    else
    {
      /* try to read the old v3/v4 metadata tag */
      err = chd_get_metadata(chd, CDROM_TRACK_METADATA_TAG,
                             NumTracks, tmp, sizeof(tmp), NULL, NULL,
                             NULL);
      if (err == CHDERR_NONE)
      {
        sscanf(tmp, CDROM_TRACK_METADATA_FORMAT, &tkid, type, subtype,
               &frames);
      }
      else
      {
        /* if there's no valid metadata, this is the end of the TOC */
        break;
      }
    }

    if (strcmp(type, "MODE1") && strcmp(type, "MODE1_RAW") && strcmp(type, "MODE2_RAW") &&
        strcmp(type, "AUDIO"))
    {
      log_cb(RETRO_LOG_ERROR, "chd_parse track type %s unsupported\n", type);
      return 0;
    }

    if (strcmp(subtype, "NONE"))
    {
      log_cb(RETRO_LOG_ERROR, "chd_parse track subtype %s unsupported\n", subtype);
      return 0;
    }

    /* add track */
    NumTracks++;
    toc.tracks[NumTracks].adr = 1;
    toc.tracks[NumTracks].control = strcmp(type, "AUDIO") == 0 ? 0 : 4;
    toc.tracks[NumTracks].valid = true;

    Tracks[NumTracks].pregap = (NumTracks == 1) ? 150 : 0;
    Tracks[NumTracks].pregap_dv = pregap;
    plba += Tracks[NumTracks].pregap + Tracks[NumTracks].pregap_dv;
    Tracks[NumTracks].LBA = toc.tracks[NumTracks].lba = plba;
    Tracks[NumTracks].postgap = postgap;
    Tracks[NumTracks].chd_offset = chd_offset;
    Tracks[NumTracks].sectors = frames - Tracks[NumTracks].pregap_dv;
    Tracks[NumTracks].SubchannelMode = 0;
    Tracks[NumTracks].index[0] = -1;
    Tracks[NumTracks].index[1] = 0;
    for (int32_t i = 2; i < 100; i++)
      Tracks[NumTracks].index[i] = -1;

    toc.tracks[NumTracks].lba = plba;

    if (strcmp(type, "AUDIO") == 0)
    {
      Tracks[NumTracks].DIFormat = DI_FORMAT_AUDIO;
      Tracks[NumTracks].RawAudioMSBFirst = 1;
    }
    else if (strcmp(type, "MODE1_RAW") == 0)
      Tracks[NumTracks].DIFormat = DI_FORMAT_MODE1_RAW;
    else if (strcmp(type, "MODE2_RAW") == 0)
      Tracks[NumTracks].DIFormat = DI_FORMAT_MODE2_RAW;
    else if (strcmp(type, "MODE1") == 0)
      Tracks[NumTracks].DIFormat = DI_FORMAT_MODE1;

    Tracks[NumTracks].subq_control = strcmp(type, "AUDIO") == 0 ? 0 : 4;

    //log_cb(RETRO_LOG_INFO, "chd_parse '%s' track=%d lba=%d, pregap=%d pregap_dv=%d postgap=%d sectors=%d\n", tmp, NumTracks, Tracks[NumTracks].LBA, Tracks[NumTracks].pregap, Tracks[NumTracks].pregap_dv, Tracks[NumTracks].postgap, Tracks[NumTracks].sectors);

    plba += frames - Tracks[NumTracks].pregap_dv;
    plba += Tracks[NumTracks].postgap;

    // tracks are padded to a 4-frame boundary in chds, calculate the
    // next track's offset to generate correct block addresses
    if (frames % CD_TRACK_PADDING > 0)
      chd_offset += (frames + (CD_TRACK_PADDING - frames % CD_TRACK_PADDING)) - frames;

    numsectors += frames;
    toc.first_track = 1;
    toc.last_track = NumTracks;
  }

  FirstTrack = 1;
  LastTrack = NumTracks;
  total_sectors = numsectors;
  //log_cb(RETRO_LOG_INFO, "chd total_sectors '%d'\n", total_sectors);

  /* add track */
  toc.tracks[100].adr = 1;
  toc.tracks[100].control = 0;
  toc.tracks[100].lba = numsectors; // HACK
  toc.tracks[100].valid = true;

  //
  // Adjust indexes for MakeSubPQ()
  //
  for (int x = FirstTrack; x < (FirstTrack + NumTracks); x++)
  {
    const int32_t base = Tracks[x].index[1];
    for (int32_t i = 0; i < 100; i++)
    {
      if (i == 0 || Tracks[x].index[i] == -1)
        Tracks[x].index[i] = INT32_MAX;
      else
        Tracks[x].index[i] = Tracks[x].LBA + (Tracks[x].index[i] - base);

      assert(Tracks[x].index[i] >= 0);
    }
  }

  return true;
}

CDAccess_CHD::~CDAccess_CHD()
{
  if (chd != NULL)
    chd_close(chd);

  if (hunkmem)
    free(hunkmem);
}

bool CDAccess_CHD::Read_Raw_Sector(uint8_t *buf, int32_t lba)
{
  uint8_t SimuQ[0xC];
  int32_t track;
  CHDFILE_TRACK_INFO *ct;

  //
  // Leadout synthesis
  //
  if (lba >= total_sectors)
  {
    uint8_t data_synth_mode = 0x01; // Default for DISC_TYPE_CDDA_OR_M1, would be 0x02 for DISC_TYPE_CD_XA

    switch (Tracks[LastTrack].DIFormat)
    {
    case DI_FORMAT_AUDIO:
      break;

    case DI_FORMAT_MODE1_RAW:
    case DI_FORMAT_MODE1:
      data_synth_mode = 0x01;
      break;

    case DI_FORMAT_MODE2_RAW:
    case DI_FORMAT_MODE2_FORM1:
    case DI_FORMAT_MODE2_FORM2:
    case DI_FORMAT_MODE2:
    case DI_FORMAT_CDI_RAW:
      data_synth_mode = 0x02;
      break;
    }

    synth_leadout_sector_lba(data_synth_mode, toc, lba, buf);
    return true;
  }

  memset(buf + 2352, 0, 96);
  track = MakeSubPQ(lba, buf + 2352);
  subq_deinterleave(buf + 2352, SimuQ);

  ct = &Tracks[track];

  //
  // Handle pregap and postgap reading
  //
  if (lba < (ct->LBA - ct->pregap_dv) || lba >= (ct->LBA + ct->sectors))
  {
    int32_t pg_offset = lba - ct->LBA;
    CHDFILE_TRACK_INFO *et = ct;

    if (pg_offset < -150)
    {
      if ((Tracks[track].subq_control & SUBQ_CTRLF_DATA) && (FirstTrack < track) && !(Tracks[track - 1].subq_control & SUBQ_CTRLF_DATA))
        et = &Tracks[track - 1];
    }

    memset(buf, 0, 2352);
    switch (et->DIFormat)
    {
    case DI_FORMAT_AUDIO:
      break;

    case DI_FORMAT_MODE1_RAW:
    case DI_FORMAT_MODE1:
      encode_mode1_sector(lba + 150, buf);
      break;

    case DI_FORMAT_MODE2_RAW:
    case DI_FORMAT_MODE2_FORM1:
    case DI_FORMAT_MODE2_FORM2:
    case DI_FORMAT_MODE2:
    case DI_FORMAT_CDI_RAW:
      buf[12 + 6] = 0x20;
      buf[12 + 10] = 0x20;
      encode_mode2_form2_sector(lba + 150, buf);
      // TODO: Zero out optional(?) checksum bytes?
      break;
    }
  }
  else
  {
    const chd_header *head = chd_get_header(chd);
    int cad                = lba + ct->chd_offset;
    int hunkid             = (cad * CD_FRAME_SIZE) / head->hunkbytes;
    int hunkofs            = (cad * CD_FRAME_SIZE) % head->hunkbytes;
    int err                = CHDERR_NONE;

    /* each hunk holds ~8 sectors, optimize when reading contiguous sectors */
    if (hunkid != oldhunk)
    {
      err = chd_read(chd, hunkid, hunkmem);
      if (err == CHDERR_NONE)
        oldhunk = hunkid;
    }

    if (ct->DIFormat == DI_FORMAT_MODE1 || ct->DIFormat == DI_FORMAT_MODE2) {
        memcpy(buf + 16, hunkmem + hunkofs, DI_Size_Table[ct->DIFormat]);
    } else {
        memcpy(buf, hunkmem + hunkofs, DI_Size_Table[ct->DIFormat]);
    }

    switch(ct->DIFormat)
    {
      case DI_FORMAT_AUDIO:
        if (ct->RawAudioMSBFirst)
          Endian_A16_Swap(buf, 588 * 2);
        break;

      case DI_FORMAT_MODE1:
        encode_mode1_sector(lba + 150, buf);
        break;

      case DI_FORMAT_MODE2:
        encode_mode2_sector(lba + 150, buf);
        break;
    }

  } // end if audible part of audio track read.

  return true;
}

//
// Note: this function makes use of the current contents(as in |=) in SubPWBuf.
//
int32_t CDAccess_CHD::MakeSubPQ(int32_t lba, uint8_t *SubPWBuf) const
{
  uint8_t buf[0xC];
  int32_t track;
  uint32_t lba_relative;
  uint32_t ma, sa, fa;
  uint32_t m, s, f;
  uint8_t pause_or = 0x00;
  bool track_found = false;

  for (track = FirstTrack; track < (FirstTrack + NumTracks); track++)
  {
    if (lba >= (Tracks[track].LBA - Tracks[track].pregap_dv - Tracks[track].pregap) && lba < (Tracks[track].LBA + Tracks[track].sectors + Tracks[track].postgap))
    {
      track_found = true;
      break;
    }
  }

  if (!track_found)
    throw(MDFN_Error(0, "Could not find track for sector %u!", lba));

  if (lba < Tracks[track].LBA)
    lba_relative = Tracks[track].LBA - 1 - lba;
  else
    lba_relative = lba - Tracks[track].LBA;

  f = (lba_relative % 75);
  s = ((lba_relative / 75) % 60);
  m = (lba_relative / 75 / 60);

  fa = (lba + 150) % 75;
  sa = ((lba + 150) / 75) % 60;
  ma = ((lba + 150) / 75 / 60);

  uint8_t adr = 0x1; // Q channel data encodes position
  uint8_t control = Tracks[track].subq_control;

  // Handle pause(D7 of interleaved subchannel byte) bit, should be set to 1 when in pregap or postgap.
  if ((lba < Tracks[track].LBA) || (lba >= Tracks[track].LBA + Tracks[track].sectors))
    pause_or = 0x80;

  // Handle pregap between audio->data track
  {
    int32_t pg_offset = (int32_t)lba - Tracks[track].LBA;

    // If we're more than 2 seconds(150 sectors) from the real "start" of the track/INDEX 01, and the track is a data track,
    // and the preceding track is an audio track, encode it as audio(by taking the SubQ control field from the preceding track).
    //
    // TODO: Look into how we're supposed to handle subq control field in the four combinations of track types(data/audio).
    //
    if (pg_offset < -150)
    {
      if ((Tracks[track].subq_control & SUBQ_CTRLF_DATA) && (FirstTrack < track) && !(Tracks[track - 1].subq_control & SUBQ_CTRLF_DATA))
        control = Tracks[track - 1].subq_control;
    }
  }

  memset(buf, 0, 0xC);
  buf[0] = (adr << 0) | (control << 4);
  buf[1] = U8_to_BCD(track);

  // Index
  //if(lba < Tracks[track].LBA) // Index is 00 in pregap
  // buf[2] = U8_to_BCD(0x00);
  //else
  // buf[2] = U8_to_BCD(0x01);
  {
    int index = 0;

    for (int32_t i = 0; i < 100; i++)
    {
      if (lba >= Tracks[track].index[i])
        index = i;
    }
    buf[2] = U8_to_BCD(index);
  }

  // Track relative MSF address
  buf[3] = U8_to_BCD(m);
  buf[4] = U8_to_BCD(s);
  buf[5] = U8_to_BCD(f);

  buf[6] = 0; // Zerroooo

  // Absolute MSF address
  buf[7] = U8_to_BCD(ma);
  buf[8] = U8_to_BCD(sa);
  buf[9] = U8_to_BCD(fa);

  subq_generate_checksum(buf);

  for (int i = 0; i < 96; i++)
    SubPWBuf[i] |= (((buf[i >> 3] >> (7 - (i & 0x7))) & 1) ? 0x40 : 0x00) | pause_or;

  return track;
}

bool CDAccess_CHD::Fast_Read_Raw_PW_TSRE(uint8_t *pwbuf, int32_t lba)
{
  int32_t track;

  if (lba >= total_sectors)
  {
    subpw_synth_leadout_lba(toc, lba, pwbuf);
    return (true);
  }

  memset(pwbuf, 0, 96);
  track = MakeSubPQ(lba, pwbuf);

  //
  // If TOC+BIN has embedded subchannel data, we can't fast-read(synthesize) it...
  //
  if (Tracks[track].SubchannelMode && lba >= (Tracks[track].LBA - Tracks[track].pregap_dv) && (lba < Tracks[track].LBA + Tracks[track].sectors))
    return (false);

  return (true);
}

bool CDAccess_CHD::Read_TOC(TOC *toc)
{
  *toc = this->toc;
  return true;
}
