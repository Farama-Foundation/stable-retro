#include <stdint.h>
#include <libretro.h>

#include "Glide64_UCode.h"
#include "rdp.h"

extern uint8_t microcode[4096];
extern uint32_t uc_crc;
extern int old_ucode;
extern SETTINGS settings;

extern retro_log_printf_t log_cb;

void microcheck(void)
{
   uint32_t i;
   uc_crc = 0;

   // Check first 3k of ucode, because the last 1k sometimes contains trash
   for (i = 0; i < 3072 >> 2; i++)
      uc_crc += ((uint32_t*)microcode)[i];

   FRDP_E ("crc: %08lx\n", uc_crc);

   if (log_cb)
      log_cb(RETRO_LOG_INFO, "Glide64 ucode = 0x%04x\n", uc_crc);

   old_ucode = settings.ucode;

   if (
         uc_crc == 0x006bd77f
         || uc_crc == 0x07200895
         || uc_crc == UCODE_GOLDENEYE_007
         || uc_crc == UCODE_DUKE_NUKEM_64
         || uc_crc == UCODE_ROBOTECH_CRYSTAL_DREAMS_PROTO
         || uc_crc == UCODE_NBA_SHOWTIME
         || uc_crc == 0xbc03e969
         || uc_crc == 0xd5604971
         || uc_crc == UCODE_MORITA_SHOUGI_64
         || uc_crc == 0xd67c2f8b
         || uc_crc == UCODE_KILLER_INSTINCT_GOLD
         || uc_crc == UCODE_MISCHIEF_MAKERS
         || uc_crc == UCODE_MORTAL_KOMBAT_TRILOGY
         || uc_crc == 0x5182f610
         || uc_crc == UCODE_BLAST_CORPS
         || uc_crc == UCODE_PILOTWINGS_64
         || uc_crc == UCODE_CRUISN_USA
         || uc_crc == UCODE_SUPER_MARIO_64
         || uc_crc == UCODE_TETRISPHERE
         || uc_crc == 0x4165e1fd
         || uc_crc == UCODE_EIKU_NO_SAINT_ANDREWS
         )
         {
            settings.ucode = 0;
            if (log_cb)
               log_cb(RETRO_LOG_INFO, "Microcode 0 - RSP SW 2.0X (Super Mario 64)\n");
         }
   else if (
         uc_crc == UCODE_CLAYFIGHTER_63
         || uc_crc == 0x05777c62
         || uc_crc == 0x057e7c62
         || uc_crc == 0x1118b3e0
         || uc_crc == UCODE_MINI_RACERS_CRC1
         || uc_crc == 0x1de712ff
         || uc_crc == 0x24cd885b
         || uc_crc == 0x26a7879a
         || uc_crc == 0xfb816260
         || uc_crc == 0x2c7975d6
         || uc_crc == 0x2d3fe3f1
         || uc_crc == UCODE_FIGHTING_FORCE_64_CRC1
         || uc_crc == 0x339872a6
         || uc_crc == 0x3ff1a4ca
         || uc_crc == 0x4340ac9b
         || uc_crc == 0x440cfad6
         || uc_crc == 0x4fe6df78
         || uc_crc == 0x5257cd2a
         || uc_crc == UCODE_MORTAL_KOMBAT_MYTHOLOGIES
         || uc_crc == 0x5414030d
         || uc_crc == 0x559ff7d4
         || uc_crc == UCODE_YOSHIS_STORY_CRC2
         || uc_crc == UCODE_IGGY_RECKIN_BALLS
         || uc_crc == 0x6075e9eb
         || uc_crc == UCODE_DEZAEMON3D
         || uc_crc == UCODE_1080_SNOWBOARDING
         || uc_crc == 0x66c0b10a
         || uc_crc == 0x6eaa1da8
         || uc_crc == 0x72a4f34e
         || uc_crc == 0x73999a23
         || uc_crc == 0x7df75834
         || uc_crc == UCODE_DOOM_64
         || uc_crc == UCODE_TUROK_1
         || uc_crc == 0x82f48073
         || uc_crc == UCODE_MINI_RACERS_CRC2
         || uc_crc == 0x841ce10f
         || uc_crc == 0x863e1ca7
         || uc_crc == UCODE_MARIO_KART_64
         || uc_crc == 0x8d5735b2
         || uc_crc == 0x8d5735b3
         || uc_crc == 0x97d1b58a
         || uc_crc == UCODE_QUAKE_64
         || uc_crc == UCODE_WETRIX
         || uc_crc == UCODE_STAR_FOX_64
         || uc_crc == UCODE_FIGHTING_FORCE_64_CRC2
         || uc_crc == 0xb4577b9c
         || uc_crc == 0xbe78677c
         || uc_crc == 0xbed8b069
         || uc_crc == 0xc3704e41
         || uc_crc == UCODE_EXTREME_G
         || uc_crc == 0xc99a4c6c
         || uc_crc == 0xcee7920f
         || uc_crc == 0xd1663234
         || uc_crc == 0xd2a9f59c
         || uc_crc == 0xd41db5f7
         || uc_crc == 0xd57049a5
         || uc_crc == UCODE_TAMIYA_RACING_64_PROTO
         || uc_crc == UCODE_WIPEOUT_64
         || uc_crc == 0xe9231df2
         || uc_crc == 0xec040469
         || uc_crc == UCODE_DUAL_HEROES
         || uc_crc == UCODE_HEXEN_64
         || uc_crc == UCODE_CHAMELEON_TWIST
         || uc_crc == UCODE_BANJO_KAZOOIE
         || uc_crc == UCODE_MACE_THE_DARK_AGE
         || uc_crc == 0xef54ee35
         )
         {
            settings.ucode = 1;
            if (log_cb)
               log_cb(RETRO_LOG_INFO, "Microcode 1 - F3DEX 1.XX (Star Fox 64)\n");
         }
   else if (
         uc_crc == 0x03044b84
         || uc_crc == 0x030f4b84
         || uc_crc == 0x0ff79527
         || uc_crc == UCODE_COMMAND_AND_CONQUER
         || uc_crc == UCODE_KNIFE_EDGE
         || uc_crc == UCODE_EXTREME_G_2
         || uc_crc == UCODE_DONKEY_KONG_64
         || uc_crc == UCODE_TONIC_TROUBLE
         || uc_crc == UCODE_PAPER_MARIO
         || uc_crc == UCODE_ANIMAL_CROSSING
         || uc_crc == UCODE_ZELDA_MAJORAS_MASK
         || uc_crc == UCODE_ZELDA_OOT
         || uc_crc == 0x6124a508
         || uc_crc == 0x630a61fb
         || uc_crc == UCODE_CASTLEVANIA_64
         || uc_crc == UCODE_CASTLEVANIA_64
         || uc_crc == UCODE_KING_HILL_64
         || uc_crc == 0x679e1205
         || uc_crc == 0x6d8f8f8a
         || uc_crc == 0x753be4a5
         || uc_crc == 0xda13ab96
         || uc_crc == 0xe65cb4ad
         || uc_crc == 0xe1290fa2
         || uc_crc == UCODE_HEY_YOU_PIKACHU
         || uc_crc == UCODE_CRUISN_EXOTICA
         || uc_crc == UCODE_STARCRAFT_64
         || uc_crc == 0x2b291027
         || uc_crc == UCODE_POKEMON_SNAP
         || uc_crc == 0x2f7dd1d5
         || uc_crc == UCODE_CRUISN_EXOTICA
         || uc_crc == UCODE_GANBARE_GOEMON_2
         || uc_crc == 0x93d11ffb
         || uc_crc == 0x93d1ff7b
         || uc_crc == UCODE_FZERO_X
         || uc_crc == 0x955117fb
         || uc_crc == UCODE_BIOHAZARD_2
         || uc_crc == 0xa2d0f88e 
         || uc_crc == 0xaa86cb1d
         || uc_crc == 0xaae4a5b9
         || uc_crc == 0xad0a6292
         || uc_crc == 0xad0a6312
         || uc_crc == UCODE_NBA_SHOWTIME
         || uc_crc == 0xba65ea1e
         || uc_crc == UCODE_KIRBY_64_CRYSTAL_SHARDS
         || uc_crc == UCODE_SUPER_SMASH_BROS
         || uc_crc == UCODE_MARIO_TENNIS
         || uc_crc == UCODE_MEGA_MAN_64
         || uc_crc == UCODE_RIDGE_RACER_64
         || uc_crc == UCODE_40WINKS
         || uc_crc == 0xcb8c9b6c
         )
         {
            settings.ucode = 2;
            if (log_cb)
               log_cb(RETRO_LOG_INFO, "Microcode 2 - F3DEX 2.XX (The Legend of Zelda: Ocarina of Time)\n");
         }
   else if (
         uc_crc == UCODE_WAVERACE_64
         )
   {
      settings.ucode = 3;
      if (log_cb)
         log_cb(RETRO_LOG_INFO, "Microcode 3 - F3DEX ? (WaveRace)\n");
   }
   else if (
         uc_crc == UCODE_STAR_WARS_SHADOW_OF_THE_EMPIRE
         )
   {
      settings.ucode = 4;
      if (log_cb)
         log_cb(RETRO_LOG_INFO, "Microcode 4 - RSP SW 2.0D EXT (Star Wars: Shadows of the Empire)\n");
   }
   else if (
         uc_crc == UCODE_JET_FORCE_GEMINI
         || uc_crc == UCODE_MICKEYS_SPEEDWAY_USA
         || uc_crc == UCODE_DIDDY_KONG_RACING
         || uc_crc == 0x63be08b3
         )
   {
      settings.ucode = 5;
      if (log_cb)
         log_cb(RETRO_LOG_INFO, "Microcode 5 - RSP SW 2.0 (Diddy Kong Racing)\n");
   }
   else if (
         uc_crc == 0x1ea9e30f
         || uc_crc == 0x74af0a74
         || uc_crc == 0x794c3e28
         || uc_crc == UCODE_BANGAIOH
         || uc_crc == 0x2b5a89c2
         || uc_crc == UCODE_YOSHIS_STORY_CRC1
         || uc_crc == 0xd20dedbf
         )
   {
      settings.ucode = 6;
      if (log_cb)
         log_cb(RETRO_LOG_INFO, "Microcode 6 - S2DEX 1.XX  (Yoshi's Story - SimCity 2000)\n");
   }
   else if (
         uc_crc == UCODE_PERFECT_DARK
         )
   {
      settings.ucode = 7;
      if (log_cb)
         log_cb(RETRO_LOG_INFO, "Microcode 7 - RSP SW PD (Perfect Dark)\n");
   }
   else if (
         uc_crc == UCODE_CONKERS_BAD_FUR_DAY
         )
   {
      settings.ucode = 8;
      if (log_cb)
         log_cb(RETRO_LOG_INFO, "Microcode 8 - F3DEXBG 2.08 (Conker's Bad Fur Day)\n");
   }
   else if (
         uc_crc == 0x0bf36d36
         )
   {
      settings.ucode = 9;
      if (log_cb)
         log_cb(RETRO_LOG_INFO, "Microcode 9 - Star Wars: Battle for Naboo\n");
   }
   else if (
         uc_crc == 0x1f120bbb
         || uc_crc == 0xf9893f70
         || uc_crc == UCODE_LAST_LEGION_UX
         )
   {
      settings.ucode = 21;
      if (log_cb)
         log_cb(RETRO_LOG_INFO, "Microcode 21 - ???.\n");
   }
   else if (
         uc_crc == 0x0d7bbffb
         || uc_crc == 0x0ff795bf
         || uc_crc == UCODE_STAR_WARS_ROGUE_SQUADRON
         || uc_crc == 0x844b55b5
         || uc_crc == 0x8ec3e124
         || uc_crc == 0xd5c4dc96
         )
   {
      settings.ucode = -1;
      if (log_cb)
         log_cb(RETRO_LOG_INFO, "Microcode -1 - Unknown Microcode.\n");
   }
   else if (
         uc_crc == 0xef54ee35
         )
   {
      settings.ucode = 10;
      if (log_cb)
         log_cb(RETRO_LOG_INFO, "Microcode 10 - F3DTEXA (64 de Hakken).\n");
   }
#if 0
   else if (
         uc_crc == UCODE_ARMORINES_PROJECT
         )
   {
      settings.ucode = 11;
      if (log_cb)
         log_cb(RETRO_LOG_INFO, "Microcode 10 - F3DEX2Acclaim (Turok 2/3/South Park).\n");
   }
#else
   else if (
         uc_crc == UCODE_ARMORINES_PROJECT
         )
   {
      settings.ucode = 2;
      if (log_cb)
         log_cb(RETRO_LOG_INFO, "Microcode 2 - F3DEX 2.XX (The Legend of Zelda: Ocarina of Time)\n");
   }
#endif

   if (log_cb)
      log_cb(RETRO_LOG_INFO, "microcheck: old ucode: %d,  new ucode: %d\n", old_ucode, settings.ucode);
#if 0
   //FIXME/TODO - check if this is/was necessary at all - persp_supported would be set to false here but rdp.Persp_en would be forcibly set to 1
   if (uc_crc == 0x8d5735b2 || uc_crc == 0xb1821ed3 || uc_crc == 0x1118b3e0) //F3DLP.Rej ucode. perspective texture correction is not implemented
   {
   }
#endif
}
