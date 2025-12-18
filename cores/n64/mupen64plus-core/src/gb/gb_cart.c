/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - gb_cart.c                                               *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2015 Bobby Smiles                                       *
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
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "gb_cart.h"

#include "api/m64p_types.h"
#include "api/callbacks.h"
#include "main/util.h"

static int read_gb_cart_normal(struct gb_cart* gb_cart, uint16_t address, uint8_t* data)
{
   uint16_t offset;

   switch(address >> 13)
   {
      case (0x0000 >> 13):
      case (0x2000 >> 13):
      case (0x4000 >> 13):
      case (0x6000 >> 13):
         if (address >= gb_cart->rom_size)
         {
            DebugMessage(M64MSG_WARNING, "Out of bound read to GB ROM %04x", address);
            break;
         }

         memcpy(data, &gb_cart->rom[address], 0x20);
         break;

      case (0xa000 >> 13):
         if (gb_cart->ram == NULL)
         {
            DebugMessage(M64MSG_WARNING, "Trying to read from absent GB RAM %04x", address);
            break;
         }

         offset = address - 0xa000;
         if (offset >= gb_cart->ram_size)
         {
            DebugMessage(M64MSG_WARNING, "Out of bound read from GB RAM %04x", address);
            break;
         }

         memcpy(data, &gb_cart->ram[offset], 0x20);
         break;

      default:
         DebugMessage(M64MSG_WARNING, "Invalid cart read (normal): %04x", address);
   }

   return 0;
}

static int write_gb_cart_normal(struct gb_cart* gb_cart, uint16_t address, const uint8_t* data)
{
   uint16_t offset;

   switch(address >> 13)
   {
      case (0x0000 >> 13):
      case (0x2000 >> 13):
      case (0x4000 >> 13):
      case (0x6000 >> 13):
         DebugMessage(M64MSG_WARNING, "Trying to write to GB ROM %04x", address);
         break;
      case (0xa000 >> 13):
         if (gb_cart->ram == NULL)
         {
            DebugMessage(M64MSG_WARNING, "Trying to write to absent GB RAM %04x", address);
            break;
         }

         offset = address - 0xa000;
         if (offset >= gb_cart->ram_size)
         {
            DebugMessage(M64MSG_WARNING, "Out of bound write to GB RAM %04x", address);
            break;
         }

         memcpy(&gb_cart->ram[offset], data, 0x20);
         break;

      default:
         DebugMessage(M64MSG_WARNING, "Invalid cart write (normal): %04x", address);
   }

   return 0;
}


static int read_gb_cart_mbc1(struct gb_cart* gb_cart, uint16_t address, uint8_t* data)
{
   return 0;
}

static int write_gb_cart_mbc1(struct gb_cart* gb_cart, uint16_t address, const uint8_t* data)
{
   return 0;
}

static int read_gb_cart_mbc2(struct gb_cart* gb_cart, uint16_t address, uint8_t* data)
{
   return 0;
}

static int write_gb_cart_mbc2(struct gb_cart* gb_cart, uint16_t address, const uint8_t* data)
{
   return 0;
}

static void dump_32_bytes(const uint8_t* d)
{
   int i;
   for(i = 0 ; i < 4; ++i, d += 8)
   {
      DebugMessage(M64MSG_WARNING, "%02x %02x %02x %02x %02x %02x %02x %02x",
            d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7]);
   }
}

static int read_gb_cart_mbc3(struct gb_cart* gb_cart, uint16_t address, uint8_t* data)
{
   size_t offset;

   DebugMessage(M64MSG_WARNING, "MBC3 R %04x", address);

   switch(address >> 13)
   {
      case (0x0000 >> 13):
      case (0x2000 >> 13):
         memcpy(data, &gb_cart->rom[address], 0x20);
         DebugMessage(M64MSG_WARNING, "MBC3 read ROM bank 0 (%04x)", address);
         dump_32_bytes(data);
         break;

      case (0x4000 >> 13):
      case (0x6000 >> 13):
         offset = (address - 0x4000) + (gb_cart->rom_bank * 0x4000);
         if (offset < gb_cart->rom_size)
         {
            memcpy(data, &gb_cart->rom[offset], 0x20);
            DebugMessage(M64MSG_WARNING, "MBC3 read ROM bank %d (%08x)", gb_cart->rom_bank, offset);
            dump_32_bytes(data);
         }
         else
         {
            DebugMessage(M64MSG_WARNING, "Out of bound read to GB ROM %08x", offset);
         }
         break;

      case (0xa000 >> 13):
         if (gb_cart->has_rtc && (gb_cart->ram_bank >= 0x08 && gb_cart->ram_bank <= 0x0c))
         {
            /* XXX: implement RTC read */
            DebugMessage(M64MSG_WARNING, "RTC read not implemented !");
            memset(data, 0, 0x20);
         }
         else if (gb_cart->ram != NULL)
         {
            offset = (address - 0xa000) + (gb_cart->ram_bank * 0x2000);
            if (offset < gb_cart->ram_size)
            {
               memcpy(data, &gb_cart->ram[offset], 0x20);
               DebugMessage(M64MSG_WARNING, "MBC3 read RAM bank %d (%08x)", gb_cart->ram_bank, offset);
               dump_32_bytes(data);
            }
            else
            {
               DebugMessage(M64MSG_WARNING, "Out of bound read from GB RAM %08x", offset);
            }
         }
         else
         {
            DebugMessage(M64MSG_WARNING, "Trying to read from absent GB RAM %04x", address);
         }
         break;

      default:
         DebugMessage(M64MSG_WARNING, "Invalid cart read (normal): %04x", address);
   }

   return 0;
}

static int write_gb_cart_mbc3(struct gb_cart* gb_cart, uint16_t address, const uint8_t* data)
{
   uint8_t bank;
   size_t offset;

   DebugMessage(M64MSG_WARNING, "MBC3 W %04x", address);

   switch(address >> 13)
   {
      case (0x0000 >> 13):
         /* XXX: implement RAM enable */
         break;

      case (0x2000 >> 13):
         bank = data[0] & 0x7f;
         gb_cart->rom_bank = (bank == 0) ? 1 : bank;
         DebugMessage(M64MSG_WARNING, "MBC3 set rom bank %02x", gb_cart->rom_bank);
         break;

      case (0x4000 >> 13):
         bank = data[0];
         if (gb_cart->has_rtc && (bank >= 0x8 && bank <= 0xc))
         {
            gb_cart->ram_bank = bank;
         }
         else if (gb_cart->ram != NULL)
         {
            gb_cart->ram_bank = bank & 0x03;
         }
         DebugMessage(M64MSG_WARNING, "MBC3 set ram bank %02x", gb_cart->ram_bank);
         break;

      case (0x6000 >> 13):
         /* XXX: implement timer update */
         DebugMessage(M64MSG_WARNING, "Timer update not implemented !");
         break;

      case (0xa000 >> 13):
         if (gb_cart->has_rtc && (gb_cart->ram_bank >= 0x8 && gb_cart->ram_bank <= 0xc))
         {
            /* XXX: implement RTC write */
            DebugMessage(M64MSG_WARNING, "RTC write not implemented !");
         }
         else if (gb_cart->ram != NULL)
         {
            offset = (address - 0xa000) + (gb_cart->ram_bank * 0x2000);
            if (offset < gb_cart->ram_size)
            {
               memcpy(&gb_cart->ram[offset], data, 0x20);
               DebugMessage(M64MSG_WARNING, "MBC3 write RAM bank %d (%08x)", gb_cart->ram_bank, offset);
            }
            else
            {
               DebugMessage(M64MSG_WARNING, "Out of bound read from GB RAM %08x", offset);
            }
         }
         else
         {
            DebugMessage(M64MSG_WARNING, "Trying to read from absent GB RAM %04x", address);
         }
         break;

      default:
         DebugMessage(M64MSG_WARNING, "Invalid cart read (normal): %04x", address);
   }

   return 0;
}

static int read_gb_cart_mbc4(struct gb_cart* gb_cart, uint16_t address, uint8_t* data)
{
   return 0;
}

static int write_gb_cart_mbc4(struct gb_cart* gb_cart, uint16_t address, const uint8_t* data)
{
   return 0;
}

static int read_gb_cart_mbc5(struct gb_cart* gb_cart, uint16_t address, uint8_t* data)
{
   size_t offset;

   DebugMessage(M64MSG_WARNING, "MBC5 R %04x", address);


   switch(address >> 13)
   {
      case (0x0000 >> 13):
      case (0x2000 >> 13):
         memcpy(data, &gb_cart->rom[address], 0x20);
         DebugMessage(M64MSG_WARNING, "MBC5 read ROM bank 0 (%04x)", address);
         dump_32_bytes(data);
         break;

      case (0x4000 >> 13):
      case (0x6000 >> 13):
         offset = (address - 0x4000) + (gb_cart->rom_bank * 0x4000);
         if (offset < gb_cart->rom_size)
         {
            memcpy(data, &gb_cart->rom[offset], 0x20);
            DebugMessage(M64MSG_WARNING, "MBC5 read ROM bank %d (%08x)", gb_cart->rom_bank, offset);
            dump_32_bytes(data);
         }
         else
         {
            DebugMessage(M64MSG_WARNING, "Out of bound read to GB ROM %08x", offset);
         }
         break;

      case (0xa000 >> 13):
         if (gb_cart->ram != NULL)
         {
            offset = (address - 0xa000) + (gb_cart->ram_bank * 0x2000);
            if (offset < gb_cart->ram_size)
            {
               memcpy(data, &gb_cart->ram[offset], 0x20);
               DebugMessage(M64MSG_WARNING, "MBC5 read RAM bank %d (%08x)", gb_cart->ram_bank, offset);
               dump_32_bytes(data);
            }
            else
            {
               DebugMessage(M64MSG_WARNING, "Out of bound read from GB RAM %08x", offset);
            }
         }
         else
         {
            DebugMessage(M64MSG_WARNING, "Trying to read from absent GB RAM %04x", address);
         }
         break;

      default:
         DebugMessage(M64MSG_WARNING, "Invalid cart read (normal): %04x", address);
   }

   return 0;
}

static int write_gb_cart_mbc5(struct gb_cart* gb_cart, uint16_t address, const uint8_t* data)
{
   size_t offset;

   DebugMessage(M64MSG_WARNING, "MBC5 W %04x", address);

   switch(address >> 13)
   {
      case (0x0000 >> 13):
         /* XXX: implement RAM enable */
         break;

      case (0x2000 >> 13):
         if (address < 0x3000)
         {
            gb_cart->rom_bank &= 0xff00;
            gb_cart->rom_bank |= data[0];
         }
         else
         {
            gb_cart->rom_bank &= 0x00ff;
            gb_cart->rom_bank |= (data[0] & 0x01) << 8;
         }
         DebugMessage(M64MSG_WARNING, "MBC5 set rom bank %04x", gb_cart->rom_bank);
         break;

      case (0x4000 >> 13):
         if (gb_cart->ram != NULL)
         {
            gb_cart->ram_bank = data[0] & 0x0f;
            DebugMessage(M64MSG_WARNING, "MBC5 set ram bank %02x", gb_cart->ram_bank);
         }
         break;


      case (0xa000 >> 13):
         if (gb_cart->ram != NULL)
         {
            offset = (address - 0xa000) + (gb_cart->ram_bank * 0x2000);
            if (offset < gb_cart->ram_size)
            {
               memcpy(&gb_cart->ram[offset], data, 0x20);
               DebugMessage(M64MSG_WARNING, "MBC5 write RAM bank %d (%08x)", gb_cart->ram_bank, offset);
            }
            else
            {
               DebugMessage(M64MSG_WARNING, "Out of bound read from GB RAM %08x", offset);
            }
         }
         else
         {
            DebugMessage(M64MSG_WARNING, "Trying to read from absent GB RAM %04x", address);
         }
         break;

      default:
         DebugMessage(M64MSG_WARNING, "Invalid cart read (normal): %04x", address);
   }

   return 0;
}

static int read_gb_cart_mmm01(struct gb_cart* gb_cart, uint16_t address, uint8_t* data)
{
   return 0;
}

static int write_gb_cart_mmm01(struct gb_cart* gb_cart, uint16_t address, const uint8_t* data)
{
   return 0;
}

static int read_gb_cart_pocket_cam(struct gb_cart* gb_cart, uint16_t address, uint8_t* data)
{
   return 0;
}

static int write_gb_cart_pocket_cam(struct gb_cart* gb_cart, uint16_t address, const uint8_t* data)
{
   return 0;
}

static int read_gb_cart_bandai_tama5(struct gb_cart* gb_cart, uint16_t address, uint8_t* data)
{
   return 0;
}

static int write_gb_cart_bandai_tama5(struct gb_cart* gb_cart, uint16_t address, const uint8_t* data)
{
   return 0;
}

static int read_gb_cart_huc1(struct gb_cart* gb_cart, uint16_t address, uint8_t* data)
{
   return 0;
}

static int write_gb_cart_huc1(struct gb_cart* gb_cart, uint16_t address, const uint8_t* data)
{
   return 0;
}

static int read_gb_cart_huc3(struct gb_cart* gb_cart, uint16_t address, uint8_t* data)
{
   return 0;
}

static int write_gb_cart_huc3(struct gb_cart* gb_cart, uint16_t address, const uint8_t* data)
{
   return 0;
}



enum gbcart_extra_devices
{
   GED_NONE    = 0x00,
   GED_RAM     = 0x01,
   GED_BATTERY = 0x02,
   GED_RTC     = 0x04,
   GED_RUMBLE  = 0x08
};

struct parsed_cart_type
{
   int (*read_gb_cart)(struct gb_cart*,uint16_t,uint8_t*);
   int (*write_gb_cart)(struct gb_cart*,uint16_t,const uint8_t*);
   unsigned int extra_devices;
};

static const struct parsed_cart_type* parse_cart_type(uint8_t cart_type)
{
#define MBC(x) read_gb_cart_ ## x, write_gb_cart_ ## x
   static const struct parsed_cart_type GB_CART_TYPES[] =
   {
      { MBC(normal),          GED_NONE },
      { MBC(mbc1),            GED_NONE },
      { MBC(mbc1),            GED_RAM },
      { MBC(mbc1),            GED_RAM | GED_BATTERY },
      { MBC(mbc2),            GED_NONE },
      { MBC(mbc2),            GED_BATTERY },
      { MBC(normal),          GED_RAM },
      { MBC(normal),          GED_RAM | GED_BATTERY },
      { MBC(mmm01),           GED_NONE },
      { MBC(mmm01),           GED_RAM },
      { MBC(mmm01),           GED_RAM | GED_BATTERY },
      { MBC(mbc3),            GED_BATTERY | GED_RTC },
      { MBC(mbc3),            GED_RAM | GED_BATTERY | GED_RTC },
      { MBC(mbc3),            GED_NONE },
      { MBC(mbc3),            GED_RAM },
      { MBC(mbc3),            GED_RAM | GED_BATTERY },
      { MBC(mbc4),            GED_NONE },
      { MBC(mbc4),            GED_RAM },
      { MBC(mbc4),            GED_RAM | GED_BATTERY },
      { MBC(mbc5),            GED_NONE },
      { MBC(mbc5),            GED_RAM },
      { MBC(mbc5),            GED_RAM | GED_BATTERY },
      { MBC(mbc5),            GED_RUMBLE },
      { MBC(mbc5),            GED_RAM | GED_RUMBLE },
      { MBC(mbc5),            GED_RAM | GED_BATTERY | GED_RUMBLE },
      { MBC(pocket_cam),      GED_NONE },
      { MBC(bandai_tama5),    GED_NONE },
      { MBC(huc3),            GED_NONE },
      { MBC(huc1),            GED_RAM | GED_BATTERY }
   };
#undef MBC


   switch(cart_type)
   {
      case 0x00: return &GB_CART_TYPES[0];
      case 0x01: return &GB_CART_TYPES[1];
      case 0x02: return &GB_CART_TYPES[2];
      case 0x03: return &GB_CART_TYPES[3];
      case 0x05: return &GB_CART_TYPES[4];
      case 0x06: return &GB_CART_TYPES[5];
      case 0x08: return &GB_CART_TYPES[6];
      case 0x09: return &GB_CART_TYPES[7];
      case 0x0b: return &GB_CART_TYPES[8];
      case 0x0c: return &GB_CART_TYPES[9];
      case 0x0d: return &GB_CART_TYPES[10];
      case 0x0f: return &GB_CART_TYPES[11];
      case 0x10: return &GB_CART_TYPES[12];
      case 0x11: return &GB_CART_TYPES[13];
      case 0x12: return &GB_CART_TYPES[14];
      case 0x13: return &GB_CART_TYPES[15];
      case 0x15: return &GB_CART_TYPES[16];
      case 0x16: return &GB_CART_TYPES[17];
      case 0x17: return &GB_CART_TYPES[18];
      case 0x19: return &GB_CART_TYPES[19];
      case 0x1a: return &GB_CART_TYPES[20];
      case 0x1b: return &GB_CART_TYPES[21];
      case 0x1c: return &GB_CART_TYPES[22];
      case 0x1d: return &GB_CART_TYPES[23];
      case 0x1e: return &GB_CART_TYPES[24];
      case 0xfc: return &GB_CART_TYPES[25];
      case 0xfd: return &GB_CART_TYPES[26];
      case 0xfe: return &GB_CART_TYPES[27];
      case 0xff: return &GB_CART_TYPES[28];
      default:   return NULL;
   }
}

int init_gb_cart(struct gb_cart* gb_cart, uint8_t *rom, size_t rom_size)
{
   int err;
   const struct parsed_cart_type* type;
   uint8_t cart_type;
   uint8_t* ram = NULL;
   size_t ram_size = 0;

   /* load GB cart ROM */
   if (rom_size < 0x8000)
   {
      DebugMessage(M64MSG_ERROR, "Invalid GB ROM file size (< 32k)");
      err = -1;
      goto free_rom;
   }

   /* get and parse cart type */
   cart_type = rom[0x147];
   type      = parse_cart_type(cart_type);
   if (type == NULL)
   {
      DebugMessage(M64MSG_ERROR, "Invalid GB cart type (%02x)", cart_type);
      err = -1;
      goto free_rom;
   }

   DebugMessage(M64MSG_INFO, "GB cart type (%02x) %s %s %s %s",
         cart_type,
         (type->extra_devices & GED_RAM)     ? "RAM" : "",
         (type->extra_devices & GED_BATTERY) ? "BATT" : "",
         (type->extra_devices & GED_RTC)     ? "RTC" : "",
         (type->extra_devices & GED_RUMBLE)  ? "RUMBLE" : "");

   /* load ram (if present) */
   if (type->extra_devices & GED_RAM)
   {
      ram_size = 0;
      switch(rom[0x149])
      {
         case 0x01: ram_size =  1*0x800; break;
         case 0x02: ram_size =  4*0x800; break;
         case 0x03: ram_size = 16*0x800; break;
         case 0x04: ram_size = 64*0x800; break;
         case 0x05: ram_size = 32*0x800; break;
      }

      if (ram_size != 0)
      {
         ram = (uint8_t*)malloc(ram_size);
         if (ram == NULL)
         {
            DebugMessage(M64MSG_ERROR, "Cannot allocate enough memory for GB RAM (%d bytes)", ram_size);
            err = -1;
            goto free_rom;
         }

#if 0
         read_from_file("./pkmb.sav", ram, ram_size);
#endif
         DebugMessage(M64MSG_INFO, "Using a %d bytes GB RAM", ram_size);
      }
   }

   /* update gb_cart */
   gb_cart->rom = rom;
   gb_cart->ram = ram;
   gb_cart->rom_size = rom_size;
   gb_cart->ram_size = ram_size;
   gb_cart->rom_bank = 1;
   gb_cart->ram_bank = 0;
   gb_cart->has_rtc = (type->extra_devices & GED_RTC) ? 1 : 0;
   gb_cart->read_gb_cart = type->read_gb_cart;
   gb_cart->write_gb_cart = type->write_gb_cart;
   return 0;

free_rom:
   free(rom);
   return err;
}

void release_gb_cart(struct gb_cart* gb_cart)
{
   if (gb_cart->rom != NULL)
      free(gb_cart->rom);

   if (gb_cart->ram != NULL)
      free(gb_cart->ram);

   memset(gb_cart, 0, sizeof(*gb_cart));
}


int read_gb_cart(struct gb_cart* gb_cart, uint16_t address, uint8_t* data)
{
   return gb_cart->read_gb_cart(gb_cart, address, data);
}

int write_gb_cart(struct gb_cart* gb_cart, uint16_t address, const uint8_t* data)
{
   return gb_cart->write_gb_cart(gb_cart, address, data);
}
