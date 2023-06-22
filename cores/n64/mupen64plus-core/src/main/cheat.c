/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - cheat.c                                                 *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2009 Richard Goedeken                                   *
 *   Copyright (C) 2008 Okaygo                                             *
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

// gameshark and xploder64 reference: http://doc.kodewerx.net/hacking_n64.html 

#include "api/m64p_types.h"
#include "api/callbacks.h"
#include "api/config.h"

#include "memory/memory.h"
#include "cheat.h"
#include "main.h"
#include "device.h"
#include "rom.h"
#include "list.h"
#include "eventloop.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Local definitions */
#define CHEAT_CODE_MAGIC_VALUE 0xDEAD0000
#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

typedef struct cheat_code
{
    unsigned int address;
    int value;
    int old_value;
    struct list_head list;
} cheat_code_t;

typedef struct cheat
{
    char *name;
    int enabled;
    int was_enabled;
    struct list_head cheat_codes;
    struct list_head list;
} cheat_t;

/* Local variables */
static LIST_HEAD(active_cheats);
extern unsigned int frame_dupe;

/* Private functions */
static uint16_t read_address_16bit(unsigned int address)
{
    return *(uint16_t*)(((uint8_t*)g_dev.ri.rdram.dram + ((address & 0xFFFFFF)^S16)));
}

static uint8_t read_address_8bit(unsigned int address)
{
    return *(unsigned char *)(((unsigned char*)g_dev.ri.rdram.dram + ((address & 0xFFFFFF)^S8)));
}

static void update_address_16bit(unsigned int address, unsigned short new_value)
{
    *(uint16_t *)(((uint8_t*)g_dev.ri.rdram.dram + ((address & 0xFFFFFF)^S16))) = new_value;
}

static void update_address_8bit(unsigned int address, unsigned char new_value)
{
     *(uint8_t *)(((uint8_t*)g_dev.ri.rdram.dram + ((address & 0xFFFFFF)^S8))) = new_value;
}

static int address_equal_to_8bit(unsigned int address, unsigned char value)
{
    unsigned char value_read;
    value_read = *(unsigned char *)(((unsigned char*)g_dev.ri.rdram.dram + ((address & 0xFFFFFF)^S8)));
    return value_read == value;
}

static int address_equal_to_16bit(unsigned int address, unsigned short value)
{
    unsigned short value_read;
    value_read = *(unsigned short *)(((unsigned char*)g_dev.ri.rdram.dram + ((address & 0xFFFFFF)^S16)));
    return value_read == value;
}

// individual application - returns 0 if we are supposed to skip the next cheat
// (only really used on conditional codes)
static int execute_cheat(unsigned int address, unsigned short value, int *old_value)
{
   switch (address & 0xFF000000)
   {
      case 0x80000000:
      case 0x88000000:
      case 0xA0000000:
      case 0xA8000000:
      case 0xF0000000:
         // if pointer to old value is valid and uninitialized, write current value to it
         if(old_value && (*old_value == CHEAT_CODE_MAGIC_VALUE))
            *old_value = (int) read_address_8bit(address);
         update_address_8bit(address,(unsigned char) value);
         return 1;
      case 0x81000000:
      case 0x89000000:
      case 0xA1000000:
      case 0xA9000000:
      case 0xF1000000:
         // if pointer to old value is valid and uninitialized, write current value to it
         if(old_value && (*old_value == CHEAT_CODE_MAGIC_VALUE))
            *old_value = (int) read_address_16bit(address);
         update_address_16bit(address,value);
         return 1;
      case 0xD0000000:
      case 0xD8000000:
         return address_equal_to_8bit(address,(unsigned char) value);
      case 0xD1000000:
      case 0xD9000000:
         return address_equal_to_16bit(address,value);
      case 0xD2000000:
      case 0xDB000000:
         return !(address_equal_to_8bit(address,(unsigned char) value));
      case 0xD3000000:
      case 0xDA000000:
         return !(address_equal_to_16bit(address,value));
      case 0xEE000000:
         // most likely, this doesnt do anything.
         execute_cheat(0xF1000318, 0x0040, NULL);
         execute_cheat(0xF100031A, 0x0000, NULL);
         return 1;
   }

   return 1;
}

static cheat_t *find_or_create_cheat(const char *name)
{
    cheat_t *cheat;
    int found = 0;

    list_for_each_entry_t(cheat, &active_cheats, cheat_t, list)
    {
        if (strcmp(cheat->name, name) == 0)
        {
            found = 1;
            break;
        }
    }

    if (found)
    {
        /* delete any pre-existing cheat codes */
        cheat_code_t *code, *safe;

        list_for_each_entry_safe_t(code, safe, &cheat->cheat_codes, cheat_code_t, list)
        {
             list_del(&code->list);
             free(code);
        }

        cheat->enabled = 0;
        cheat->was_enabled = 0;
    }
    else
    {
        cheat = malloc(sizeof(*cheat));
        cheat->name = strdup(name);
        cheat->enabled = 0;
        cheat->was_enabled = 0;
        INIT_LIST_HEAD(&cheat->cheat_codes);
        list_add_tail(&cheat->list, &active_cheats);
    }

    return cheat;
}


// public functions
void cheat_init(void)
{
}

void cheat_uninit(void)
{
}

void cheat_apply_cheats(int entry)
{
    cheat_t *cheat;
    cheat_code_t *code;
    int skip;
    int execute_next;


#if 0
   if ((
         strncmp((char *)ROM_HEADER.Name, (const char *)"BANJO KAZOOIE 2", 15) == 0
         || strncmp((char *)ROM_HEADER.Name, (const char *)"BANJO TOOIE", 11) == 0
         ) && entry == ENTRY_VI)
   {
      execute_cheat(0x8107913C, 0x0000, NULL);
      execute_cheat(0x8107913E, 0x0000, NULL);
   }
#endif

    // If game is Pokemon Snap, apply controller fix
    if (strncmp((char *)ROM_HEADER.Name, "POKEMON SNAP", 12) == 0 && entry == ENTRY_VI)
    {
       if (sl(ROM_HEADER.CRC1) == 0xCA12B547 && sl(ROM_HEADER.CRC2) == 0x71FA4EE4) {
          // Pokemon Snap (U)
          execute_cheat(0xD1382D1C, 0x0002, NULL);
          execute_cheat(0x80382D0F, 0x0000, NULL);
       }
       else if (sl(ROM_HEADER.CRC1) == 0x7BB18D40 && sl(ROM_HEADER.CRC2) == 0x83138559) {
          // Pokemon Snap (A)
          execute_cheat(0xD1382D1C, 0x0002, NULL);
          execute_cheat(0x80382D0F, 0x0000, NULL);
       }
       else if (sl(ROM_HEADER.CRC1) == 0x39119872 && sl(ROM_HEADER.CRC2) == 0x07722E9F) {
          // Pokemon Snap Station (U)
          execute_cheat(0xD1382D1C, 0x0002, NULL);
          execute_cheat(0x80382D0F, 0x0000, NULL);
       }
       else if (sl(ROM_HEADER.CRC1) == 0xEC0F690D && sl(ROM_HEADER.CRC2) == 0x32A7438C) {
          // Pokemon Snap (J) (V1.0)
          execute_cheat(0xD136D22C, 0x802A, NULL);
          execute_cheat(0x8036D21F, 0x0000, NULL);
       }
       else if (sl(ROM_HEADER.CRC1) == 0xE0044E9E && sl(ROM_HEADER.CRC2) == 0xCD659D0D) {
          // Pokemon Snap (J) (V1.1)
          execute_cheat(0xD136D22C, 0x802A, NULL);
          execute_cheat(0x8036D21F, 0x0000, NULL);
       }
       else if (sl(ROM_HEADER.CRC1) == 0x5753720D && sl(ROM_HEADER.CRC2) == 0x2A8A884D) {
          // Pokemon Snap (G)
          execute_cheat(0xD1381BDC, 0x802C, NULL);
          execute_cheat(0x80381BCF, 0x0000, NULL);
       }
       else {
          // Pokemon Snap (E) + (F) + (I) + (S)
          execute_cheat(0xD1381BFC, 0x802C, NULL);
          execute_cheat(0x80381BEF, 0x0000, NULL);
       }
    }
    else if (!strcmp((char *)ROM_HEADER.Name, "DONKEY KONG 64"))
    {
       switch (ROM_HEADER.destination_code)
       {
          case 'J': /* Japan */
             execute_cheat(0x806170A2, 0x0000, NULL);
             break;
          case 'A': /* Japan / USA */
          case 'E': /* USA */
             execute_cheat(0x80619632, 0x0000, NULL);
             break;
          case 'D': /* Germany */
          case 'F': /* France */
          case 'I': /* Italy */
          case 'S': /* Spain */
          case 0x50:
          case 0x58:
          case 0x20:
          case 0x21:
          case 0x38:
          case 0x70:
             execute_cheat(0x806128E2, 0x0000, NULL);
             break;
          default:
             break;

       }
    }
    
    if (list_empty(&active_cheats))
        return;

    list_for_each_entry_t(cheat, &active_cheats, cheat_t, list)
    {
        if (cheat->enabled)
        {
            cheat->was_enabled = 1;
            switch(entry)
            {
                case ENTRY_BOOT:
                    list_for_each_entry_t(code, &cheat->cheat_codes, cheat_code_t, list)
                    {
                        // code should only be written once at boot time
                        if((code->address & 0xF0000000) == 0xF0000000)
                            execute_cheat(code->address, code->value, &code->old_value);
                    }
                    break;
                case ENTRY_VI:
                    skip = 0;
                    execute_next = 0;
                    list_for_each_entry_t(code, &cheat->cheat_codes, cheat_code_t, list)
                    {
                        if (skip)
                        {
                            skip = 0;
                            continue;
                        }
                        if (execute_next)
                        {
                            execute_next = 0;

                            // if code needs GS button pressed, don't save old value
                            if(((code->address & 0xFF000000) == 0xD8000000 ||
                                (code->address & 0xFF000000) == 0xD9000000 ||
                                (code->address & 0xFF000000) == 0xDA000000 ||
                                (code->address & 0xFF000000) == 0xDB000000))
                               execute_cheat(code->address, code->value, NULL);
                            else
                               execute_cheat(code->address, code->value, &code->old_value);

                            continue;
                        }
                        // conditional cheat codes
                        if((code->address & 0xF0000000) == 0xD0000000)
                        {
                            // if code needs GS button pressed and it's not, skip it
                            if(((code->address & 0xFF000000) == 0xD8000000 ||
                                (code->address & 0xFF000000) == 0xD9000000 ||
                                (code->address & 0xFF000000) == 0xDA000000 ||
                                (code->address & 0xFF000000) == 0xDB000000) &&
                               !event_gameshark_active())
                            {
                                // skip next code
                                skip = 1;
                                continue;
                            }

                            if (execute_cheat(code->address, code->value, NULL))
                            {
                                // if condition true, execute next cheat code
                                execute_next = 1;
                            }
                            else
                            {
                                // if condition false, skip next code
                                skip = 1;
                                continue;
                            }
                        }
                        // GS button triggers cheat code
                        else if((code->address & 0xFF000000) == 0x88000000 ||
                                (code->address & 0xFF000000) == 0x89000000 ||
                                (code->address & 0xFF000000) == 0xA8000000 ||
                                (code->address & 0xFF000000) == 0xA9000000)
                        {
                            if(event_gameshark_active())
                                execute_cheat(code->address, code->value, NULL);
                        }
                        // normal cheat code
                        else
                        {
                            // exclude boot-time cheat codes
                            if((code->address & 0xF0000000) != 0xF0000000)
                                execute_cheat(code->address, code->value, &code->old_value);
                        }
                    }
                    break;
                default:
                    break;
            }
        }
        // if cheat was enabled, but is now disabled, restore old memory values
        else if (cheat->was_enabled)
        {
            cheat->was_enabled = 0;
            switch(entry)
            {
                case ENTRY_VI:
                    list_for_each_entry_t(code, &cheat->cheat_codes, cheat_code_t, list) {
                        // set memory back to old value and clear saved copy of old value
                        if(code->old_value != CHEAT_CODE_MAGIC_VALUE)
                        {
                            execute_cheat(code->address, code->old_value, NULL);
                            code->old_value = CHEAT_CODE_MAGIC_VALUE;
                        }
                    }
                    break;
                default:
                    break;
            }
        }
    }
}


void cheat_delete_all(void)
{
    cheat_t *cheat, *safe_cheat;
    cheat_code_t *code, *safe_code;

    if (list_empty(&active_cheats))
        return;

    list_for_each_entry_safe_t(cheat, safe_cheat, &active_cheats, cheat_t, list)
    {
        free(cheat->name);

        list_for_each_entry_safe_t(code, safe_code, &cheat->cheat_codes, cheat_code_t, list)
        {
            list_del(&code->list);
            free(code);
        }
        list_del(&cheat->list);
        free(cheat);
    }
}

int cheat_set_enabled(const char *name, int enabled)
{
    cheat_t *cheat = NULL;

    if (list_empty(&active_cheats))
        return 0;

    list_for_each_entry_t(cheat, &active_cheats, cheat_t, list)
    {
        if (strcmp(name, cheat->name) == 0)
        {
            cheat->enabled = enabled;
            return 1;
        }
    }

    return 0;
}

int cheat_add_new(const char *name, m64p_cheat_code *code_list, int num_codes)
{
    cheat_t *cheat;
    int i, j;

    /* create a new cheat function or erase the codes in an existing cheat function */
    cheat = find_or_create_cheat(name);
    if (cheat == NULL)
        return 0;

    cheat->enabled = 1; /* default for new cheats is enabled */

    for (i = 0; i < num_codes; i++)
    {
        /* if this is a 'patch' code, convert it and dump out all of the individual codes */
        if ((code_list[i].address & 0xFFFF0000) == 0x50000000 && i < num_codes - 1)
        {
           int code_count = ((code_list[i].address & 0xFF00) >> 8);
           int incr_addr  = code_list[i].address & 0xFF;
           int incr_value = code_list[i].value;
           int cur_addr   = code_list[i+1].address;
           int cur_value  = code_list[i+1].value;

           i += 1;

           for (j = 0; j < code_count; j++)
           {
              cheat_code_t *code = malloc(sizeof(*code));
              code->address = cur_addr;
              code->value = cur_value;
              code->old_value = CHEAT_CODE_MAGIC_VALUE;
              list_add_tail(&code->list, &cheat->cheat_codes);
              cur_addr += incr_addr;
              cur_value += incr_value;
           }
        }
        else
        {
           /* just a normal code */
           cheat_code_t *code = malloc(sizeof(*code));

           code->address   = code_list[i].address;
           code->value     = code_list[i].value;
           code->old_value = CHEAT_CODE_MAGIC_VALUE;

           list_add_tail(&code->list, &cheat->cheat_codes);
        }
    }

    return 1;
}



