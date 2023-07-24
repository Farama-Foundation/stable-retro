/******************************************************************************/
/* Mednafen Sega Saturn Emulation Module                                      */
/******************************************************************************/
/* gun.cpp - Light Gun Emulation
**  Copyright (C) 2017 Mednafen Team
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

#include <math.h>

#include "common.h"
#include "gun.h"
#include "libretro_settings.h"

IODevice_Gun::IODevice_Gun() : state(0x4C), light_phase(true), light_phase_counter(0x7FFFFFFF)
{

}

IODevice_Gun::~IODevice_Gun()
{

}

void IODevice_Gun::SetCrosshairsColor(uint32 color)
{
	chair_r = (color >> 16) & 0xFF;
	chair_g = (color >>  8) & 0xFF;
	chair_b = (color >>  0) & 0xFF;
}

static void crosshair_plot( MDFN_Surface* surface,
							uint32* lpix,
							int x,
							int y,
							int chair_r,
							int chair_g,
							int chair_b )
{
	int r, g, b;
	int nr, ng, nb;

	surface->DecodeColor( lpix[x], r, g, b );

	//
	nr = (r + chair_r * 3) >> 2;
	ng = (g + chair_g * 3) >> 2;
	nb = (b + chair_b * 3) >> 2;

	if ( (int)((abs(r - nr) - 0x40) & (abs(g - ng) - 0x40) & (abs(b - nb) - 0x40)) < 0 )
	{
		if ( (nr | ng | nb) & 0x80 )
		{
			nr >>= 1;
			ng >>= 1;
			nb >>= 1;
		}
		else
		{
			nr ^= 0x80;
			ng ^= 0x80;
			nb ^= 0x80;
		}
	}

	//
	lpix[x] = MAKECOLOR(nr, ng, nb, 0);
}

void IODevice_Gun::Draw( MDFN_Surface* surface,
						 const MDFN_Rect& drect,
						 const int32* lw,
						 int ifield,
						 float gun_x_scale,
						 float gun_x_offs ) const
{
	switch ( setting_gun_crosshair )
	{

	case SETTING_GUN_CROSSHAIR_OFF:
		return;

	case SETTING_GUN_CROSSHAIR_CROSS:

		for( int oy = -8; oy <= 8; oy++ )
		{
			int32 y = drect.y + (((nom_coord[1] - MDFNGameInfo->mouse_offs_y) + oy) * ((ifield >= 0) ? 2 : 1)) + (ifield == 1);

			if(y < drect.y || (y - drect.y) >= drect.h)
				continue;

			uint32* lpix = surface->pixels + y * surface->pitchinpix;
			int32 cx = floorf(0.5 + (((nom_coord[0] - gun_x_offs) / gun_x_scale) - MDFNGameInfo->mouse_offs_x) * lw[y] / MDFNGameInfo->mouse_scale_x);
			int32 xmin, xmax;

			xmin = drect.x + cx;
			xmax = xmin + ((lw[y] * 2 + MDFNGameInfo->nominal_width) / (MDFNGameInfo->nominal_width * 2)) - 1;

			if(!oy)
			{
				int32 ehw = (lw[y] * 16 + MDFNGameInfo->nominal_width) / (MDFNGameInfo->nominal_width * 2);

				xmin -= ehw;
				xmax += ehw;
			}

			xmin = std::max<int32>(drect.x, xmin);
			xmax = std::min<int32>(drect.x + lw[y] - 1, xmax);

			for( int32 x = xmin; x <= xmax; x++ )
			{
				crosshair_plot( surface, lpix, x, y, chair_r, chair_g, chair_b );
			}
		}

		break;

	case SETTING_GUN_CROSSHAIR_DOT:

		for( int oy = -1; oy <= 1; oy++ )
		{
			int32 y = drect.y + (((nom_coord[1] - MDFNGameInfo->mouse_offs_y) + oy) * ((ifield >= 0) ? 2 : 1)) + (ifield == 1);

			if(y < drect.y || (y - drect.y) >= drect.h)
				continue;

			uint32* lpix = surface->pixels + y * surface->pitchinpix;
			int32 cx = floorf(0.5 + (((nom_coord[0] - gun_x_offs) / gun_x_scale) - MDFNGameInfo->mouse_offs_x) * lw[y] / MDFNGameInfo->mouse_scale_x);
			int32 xmin, xmax;

			xmin = drect.x + cx;
			xmax = xmin + ((lw[y] * 2 + MDFNGameInfo->nominal_width) / (MDFNGameInfo->nominal_width * 2)) - 1;

			int32 ehw = (lw[y] * 2 + MDFNGameInfo->nominal_width) / (MDFNGameInfo->nominal_width * 2);

			xmin -= ehw;
			xmax += ehw;

			xmin = std::max<int32>(drect.x, xmin);
			xmax = std::min<int32>(drect.x + lw[y] - 1, xmax);

			for( int32 x = xmin; x <= xmax; x++ )
			{
				crosshair_plot( surface, lpix, x, y, chair_r, chair_g, chair_b );
			}
		}

		break;

	}; // setting_gun_crosshair
}


void IODevice_Gun::Power(void)
{
 osshot_counter = -1;
 prev_ossb = false;
 //
 state |= 0x40;
}

void IODevice_Gun::TransformInput(uint8* const data, float gun_x_scale, float gun_x_offs) const
{
 int32 tmp = (int16)MDFN_de16lsb(&data[0]);

 tmp = floorf(0.5 + tmp * gun_x_scale + gun_x_offs);
 tmp = std::max<int32>(-32768, std::min<int32>(32767, tmp));

 MDFN_en16lsb(&data[0], tmp);

#if 0
 MDFN_en16lsb(&data[0], rand());
 //MDFN_en16lsb(&data[2], rand());
#endif
}

void IODevice_Gun::UpdateInput(const uint8* data, const int32 time_elapsed)
{
 nom_coord[0] = (int16)MDFN_de16lsb(&data[0]);
 nom_coord[1] = (int16)MDFN_de16lsb(&data[2]);

 state = ((((~(unsigned)data[4]) << 4) & 0x30) | 0x0C) | (state & 0x40);

 //
 //
 //
 const bool cur_ossb = (bool)(data[4] & 0x4);

 if(osshot_counter >= 0)
 {
  const int32 osshot_total = 250000;

  osshot_counter += time_elapsed;
  if(osshot_counter >= osshot_total)
   osshot_counter = -1;
  else
  {
   nom_coord[0] = -16384;
   nom_coord[1] = -16384;

   if(osshot_counter >= osshot_total * 2 / 3)
    state |= 0x10;
   else if(osshot_counter >= osshot_total * 1 / 3)
    state &= ~0x10;
   else
    state |= 0x10;
  }
 }
 else if((prev_ossb ^ cur_ossb) & cur_ossb)
  osshot_counter = 0;

 prev_ossb = cur_ossb;
}

void IODevice_Gun::StateAction(StateMem* sm, const unsigned load, const bool data_only, const char* sname_prefix)
{
 SFORMAT StateRegs[] =
 {
  SFVAR(state),

  SFVAR(light_phase),
  SFVAR(light_phase_counter),
  SFVAR(NextEventTS),

  SFVAR(osshot_counter),
  SFVAR(prev_ossb),

  SFVAR(nom_coord),

  SFEND
 };
 char section_name[64];
 trio_snprintf(section_name, sizeof(section_name), "%s_Gun", sname_prefix);

 if(!MDFNSS_StateAction(sm, load, data_only, StateRegs, section_name, true) && load)
  Power();
 else if(load)
 {
  //state |= 0x40;

 }
}

INLINE void IODevice_Gun::UpdateLight(const sscpu_timestamp_t timestamp)
{
 light_phase_counter -= (timestamp - LastTS);
 LastTS = timestamp;
 if(light_phase_counter <= 0)
 {
  if(!light_phase)
  {
   state &= ~0x40;
   //
   light_phase = true;
   light_phase_counter = 16;
   NextEventTS = timestamp + light_phase_counter;
  }
  else
  {
   state |= 0x40;
   light_phase_counter = 0x7FFFFFFF;
   NextEventTS = SS_EVENT_DISABLED_TS;
  }
 }
}

uint8 IODevice_Gun::UpdateBus(const sscpu_timestamp_t timestamp, const uint8 smpc_out, const uint8 smpc_out_asserted)
{
 UpdateLight(timestamp);
 //
 //
 return ((smpc_out & smpc_out_asserted) | (state &~ smpc_out_asserted)) & 0x7C;
}

void IODevice_Gun::LineHook(const sscpu_timestamp_t timestamp, int32 out_line, int32 div, int32 coord_adj)
{
 UpdateLight(timestamp);
 //
 if(abs((int)((uint32)nom_coord[1] - (uint32)out_line)) <= 1)
 {
  if(nom_coord[0] >= 0 && nom_coord[0] < 21472)
  {
   int32 pd = (nom_coord[0] + coord_adj) * 4 / div;

   if(pd >= 1 /*&& pd <= */)
   {
    state |= 0x40;
    light_phase = false;
    light_phase_counter = pd;
    NextEventTS = timestamp + light_phase_counter;
   }
  }
 }
}
