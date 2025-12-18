/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - cic.c                                                   *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2014 Bobby Smiles                                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "cic.h"

#include "api/m64p_types.h"
#include "api/callbacks.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

void init_cic_using_ipl3(struct cic* cic, const void* ipl3)
{
    size_t i;
    uint64_t crc = 0;

    static const struct cic cics[] =
    {
        { CIC_X101, 0x3f },
        { CIC_X102, 0x3f },
        { CIC_X103, 0x78 },
        { CIC_X105, 0x91 },
        { CIC_X106, 0x85 },
        { CIC_5167, 0xdd },
        { CIC_8303, 0xdd },
        { CIC_USDD, 0xde },
        { CIC_DVDD, 0xdd }
    };

    for (i = 0; i < 0xfc0/4; i++)
        crc += ((uint32_t*)ipl3)[i];

    switch(crc)
    {
       default:
          DebugMessage(M64MSG_WARNING,
                "Unknown CIC type (%08x)! using CIC 6102.", crc);
       case 0x000000D057C85244LL: /* CIC_X102 */
          i = 1;
          break;
       case 0x000000D0027FDF31LL: /* CIC_X101 */
       case 0x000000CFFB631223LL: /* CIC_X101 */
          i = 0;
          break;
       case 0x000000D6497E414BLL: /* CIC_X103 */
          i = 2;
          break;
       case 0x0000011A49F60E96LL: /* CIC_X105 */
          i = 3;
          break;
       case 0x000000D6D5BE5580LL: /* CIC_X106 */
          i = 4;
          break;
       case UINT64_C(0x000001053BC19870): /* CIC_5167 */
          i = 5;
          break;
       case UINT64_C(0x000000D2E53EF008): /* CIC_8303 */
          i = 6;
          break;
       case UINT64_C(0x000000D2E53E5DDA): /* CIC_USDD */
          i = 7;
          break;
       case UINT64_C(0x000000D2E53EF39F): /* CIC_DVDD */
          i = 8;
          break;
    }

    memcpy(cic, &cics[i], sizeof(*cic));
}
