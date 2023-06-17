/*
    HuC6280 sound chip emulator
    by Charles MacDonald
    E-mail: cgfm2@hotmail.com
    WWW: http://cgfm2.emuviews.com

    Thanks to:

    - Paul Clifford for his PSG documentation.
    - Richard Bannister for the TGEmu-specific sound updating code.
    - http://www.uspto.gov for the PSG patents.
    - All contributors to the tghack-list.

    Changes:

    (03/30/2003)
    - Removed TGEmu specific code and added support functions for MAME.
    - Modified setup code to handle multiple chips with different clock and
      volume settings.

    Missing features / things to do:

    - Add LFO support. But do any games actually use it?

    - Add shared index for waveform playback and sample writes. Almost every
      game will reset the index prior to playback so this isn't an issue.

    - While the noise emulation is complete, the data for the pseudo-random
      bitstream is calculated by rand() and is not a representation of what
      the actual hardware does.

    For some background on Hudson Soft's C62 chipset:

    - http://www.hudsonsoft.net/ww/about/about.html
    - http://www.hudson.co.jp/corp/eng/coinfo/history.html

    Legal information:

    Copyright (C) 2002, 2003  Charles MacDonald

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "driver.h"
#include "state.h"
#include "c6280.h"

/* Local function prototypes */
void c6280_init(int which, double clk, double rate, double volume);
void c6280_write(int which, int offset, int data);
void c6280_update(int which, INT16 **buffer, int length);

static int stream[MAX_C6280];
c6280_t c6280[MAX_C6280];


void c6280_init(int which, double clk, double rate, double volume)
{
    c6280_t *p = &c6280[which];
    int i;
    double step;

    /* Loudest volume level for table */
    double level = (65535 * (volume / 100.0)) / 6.0 / 32.0;

    /* Clear context */
    memset(p, 0, sizeof(c6280_t));

    /* Make waveform frequency table */
    for(i = 0; i < 4096; i += 1)
    {
        step = ((clk / rate) * 4096) / (i+1);
        p->wave_freq_tab[(1 + i) & 0xFFF] = (UINT32)step;
    }

    /* Make noise frequency table */
    for(i = 0; i < 32; i += 1)
    {
        step = ((clk / rate) * 32) / (i+1);
        p->noise_freq_tab[i] = (UINT32)step;
    }

    /* Make volume table */
    /* PSG has 48dB volume range spread over 32 steps */
    step = 48.0 / 32.0;
    for(i = 0; i < 31; i++)
    {
        p->volume_table[i] = (UINT16)level;
        level /= pow(10.0, step / 20.0);
    }
    p->volume_table[31] = 0;
}


void c6280_write(int which, int offset, int data)
{
    c6280_t *p = &c6280[which];
    t_channel *q = &p->channel[p->select];

    /* Update stream */
    stream_update(stream[which], 0);

    switch(offset & 0x0F)
    {
        case 0x00: /* Channel select */
            p->select = data & 0x07;
            break;

        case 0x01: /* Global balance */
            p->balance  = data;
            break;

        case 0x02: /* Channel frequency (LSB) */
            q->frequency = (q->frequency & 0x0F00) | data;
            q->frequency &= 0x0FFF;
            break;

        case 0x03: /* Channel frequency (MSB) */
            q->frequency = (q->frequency & 0x00FF) | (data << 8);
            q->frequency &= 0x0FFF;
            break;

        case 0x04: /* Channel control (key-on, DDA mode, volume) */

            /* 1-to-0 transition of DDA bit resets waveform index */
            if((q->control & 0x40) && ((data & 0x40) == 0))
            {
                q->index = 0;
            }
            q->control = data;
            break;

        case 0x05: /* Channel balance */
            q->balance = data;
            break;

        case 0x06: /* Channel waveform data */

            switch(q->control & 0xC0)
            {
                case 0x00:
                    q->waveform[q->index & 0x1F] = data & 0x1F;
                    q->index = (q->index + 1) & 0x1F;
                    break;

                case 0x40:
                    break;

                case 0x80:
                    q->waveform[q->index & 0x1F] = data & 0x1F;
                    q->index = (q->index + 1) & 0x1F;
                    break;

                case 0xC0:
                    q->dda = data & 0x1F;
                    break;
            }

            break;

        case 0x07: /* Noise control (enable, frequency) */
            q->noise_control = data;
            break;

        case 0x08: /* LFO frequency */
            p->lfo_frequency = data;
            break;

        case 0x09: /* LFO control (enable, mode) */
            p->lfo_control = data;
            break;

        default:
            break;
    }
}


void c6280_update(int num, INT16 **buffer, int length)
{
    static int scale_tab[] = {
        0x00, 0x03, 0x05, 0x07, 0x09, 0x0B, 0x0D, 0x0F,
        0x10, 0x13, 0x15, 0x17, 0x19, 0x1B, 0x1D, 0x1F
    };
    int ch;
    int i;
    c6280_t *p = &c6280[num];

    int lmal = (p->balance >> 4) & 0x0F;
    int rmal = (p->balance >> 0) & 0x0F;
    int vll, vlr;

    lmal = scale_tab[lmal];
    rmal = scale_tab[rmal];

    /* Clear buffer */
    for(i = 0; i < length; i++)
    {
        buffer[0][i] = 0;
        buffer[1][i] = 0;
    }

    for(ch = 0; ch < 6; ch++)
    {
        /* Only look at enabled channels */
        if(p->channel[ch].control & 0x80)
        {
            int lal = (p->channel[ch].balance >> 4) & 0x0F;
            int ral = (p->channel[ch].balance >> 0) & 0x0F;
            int al  = p->channel[ch].control & 0x1F;

            lal = scale_tab[lal];
            ral = scale_tab[ral];

            /* Calculate volume just as the patent says */
            vll = (0x1F - lal) + (0x1F - al) + (0x1F - lmal);
            if(vll > 0x1F) vll = 0x1F;

            vlr = (0x1F - ral) + (0x1F - al) + (0x1F - rmal);
            if(vlr > 0x1F) vlr = 0x1F;

            vll = p->volume_table[vll];
            vlr = p->volume_table[vlr];

            /* Check channel mode */
            if((ch >= 4) && (p->channel[ch].noise_control & 0x80))
            {
                /* Noise mode */
                UINT32 step = p->noise_freq_tab[(p->channel[ch].noise_control & 0x1F) ^ 0x1F];
                for(i = 0; i < length; i += 1)
                {
                    static int data = 0;
                    p->channel[ch].noise_counter += step;
                    if(p->channel[ch].noise_counter >= 0x800)
                    {
                        data = (rand() & 1) ? 0x1F : 0;
                    }
                    p->channel[ch].noise_counter &= 0x7FF;
                    buffer[0][i] += (INT16)(vll * (data - 16));
                    buffer[1][i] += (INT16)(vlr * (data - 16));
                }
            }
            else
            if(p->channel[ch].control & 0x40)
            {
                /* DDA mode */
                for(i = 0; i < length; i++)
                {
                    buffer[0][i] += (INT16)(vll * (p->channel[ch].dda - 16));
                    buffer[1][i] += (INT16)(vlr * (p->channel[ch].dda - 16));
                }
            }
            else
            {
                /* Waveform mode */
                UINT32 step = p->wave_freq_tab[p->channel[ch].frequency];
                for(i = 0; i < length; i += 1)
                {
                    int offset;
                    INT16 data;
                    offset = (p->channel[ch].counter >> 12) & 0x1F;
                    p->channel[ch].counter += step;
                    p->channel[ch].counter &= 0x1FFFF;
                    data = p->channel[ch].waveform[offset];
                    buffer[0][i] += (INT16)(vll * (data - 16));
                    buffer[1][i] += (INT16)(vlr * (data - 16));
                }
            }
        }
    }
}


/*--------------------------------------------------------------------------*/
/* MAME specific code                                                       */
/*--------------------------------------------------------------------------*/

int c6280_sh_start(const struct MachineSound *msound)
{
    const struct C6280_interface *intf = msound->sound_interface;
	char buf[2][64];
    const char *name[2];
    int volume[2];
	int i;

    for (i = 0; i < intf->num; i++)
    {
        /* Initialize PSG emulator */
        c6280_init(i, intf->clock[i], Machine->sample_rate, intf->volume[i]&0xff);

        /* Set up device name */
        sprintf(buf[0], "HuC6280 #%d", i);
        sprintf(buf[1], "HuC6280 #%d", i);
        name[0] = buf[0];
        name[1] = buf[1];

        /* Set up volume */
		volume[0] = MIXER(intf->volume[i], MIXER_PAN_LEFT);
		volume[1] = MIXER(intf->volume[i], MIXER_PAN_RIGHT);

        /* Create stereo stream */
        stream[i] = stream_init_multi(2, name, volume, Machine->sample_rate, i, c6280_update);
        if(stream[i] == -1)
            return 1;
    }

    return 0;
}

void c6280_sh_stop(void)
{
}

WRITE_HANDLER( C6280_0_w ) {  c6280_write(0,offset,data); }
WRITE_HANDLER( C6280_1_w ) {  c6280_write(1,offset,data); }
