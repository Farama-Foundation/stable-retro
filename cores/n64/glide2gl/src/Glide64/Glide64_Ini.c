#include <stdint.h>
#include "Glide64_Ini.h"
#include "Glide64_UCode.h"
#include "rdp.h"
#include "Framebuffer_glide64.h"

#include <libretro.h>
#include "../../libretro/libretro_private.h"

extern uint8_t microcode[4096];
extern uint32_t gfx_plugin_accuracy;
extern SETTINGS settings;

extern retro_environment_t environ_cb;

extern void update_variables(bool startup);
extern void glide_set_filtering(unsigned value);

void ReadSettings(void)
{
   struct retro_variable var = { "parallel-n64-screensize", 0 };
   unsigned screen_width = 640;
   unsigned screen_height = 480;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (sscanf(var.value ? var.value : "640x480", "%dx%d", &screen_width, &screen_height) != 2)
      {
         screen_width  = 640;
         screen_height = 480;
      }
   }

   settings.scr_res_x = screen_width;
   settings.scr_res_y = screen_height;
   settings.res_x = 320;
   settings.res_y = 240;

   settings.vsync = 1;

   settings.autodetect_ucode = true;
   settings.ucode = 2;
   settings.fog = 1;
   settings.buff_clear = 1;
   settings.unk_as_red = true;
   settings.unk_clear = false;
}

void ReadSpecialSettings (const char * name)
{
   int smart_read, hires, get_fbinfo, read_always, depth_render, fb_crc_mode,
       read_back_to_screen, cpu_write_hack, optimize_texrect, hires_buf_clear,
       read_alpha, ignore_aux_copy, useless_is_useless;
   uint32_t i, uc_crc;
   bool updated;
   struct retro_variable var;

   fprintf(stderr, "ReadSpecialSettings: %s\n", name);

   /* frame buffer */
   smart_read = 0;
   hires = 0;
   get_fbinfo = 0;
   read_always = 0;
   depth_render = 1;
   fb_crc_mode = 1;
   read_back_to_screen = 0;
   cpu_write_hack = 0;
   optimize_texrect = 1;
   hires_buf_clear = 0;
   read_alpha = 0;
   ignore_aux_copy = 0;
   useless_is_useless = 0;

   updated = false;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
      update_variables(false);
   
   if (strstr(name, (const char *)"DEFAULT"))
   {
      settings.filtering = 0;
      settings.buff_clear = 1;
      settings.swapmode = 1;
      settings.swapmode_retro = false;
      settings.lodmode = 0;

      /* frame buffer */
      settings.alt_tex_size = 0;
      settings.force_microcheck = 0;
      settings.force_quad3d = 0;
      settings.force_calc_sphere = 0;
      settings.depth_bias = 20;
      settings.increase_texrect_edge = 0;
      settings.decrease_fillrect_edge = 0;
      settings.stipple_mode = 2;
      settings.stipple_pattern = 0x3E0F83E0;
      settings.clip_zmin = 0;
      settings.adjust_aspect = 1;
      settings.correct_viewport = 0;
      settings.zmode_compare_less = 0;
      settings.old_style_adither = 0;
      settings.n64_z_scale = 0;
      settings.pal230 = 0;
   }

   settings.hacks = 0;

   // We might want to detect some games by ucode crc, so set
   // up uc_crc here
   uc_crc = 0;

   for (i = 0; i < 3072 >> 2; i++)
      uc_crc += ((uint32_t*)microcode)[i];

   // Glide64 mk2 INI config
   if (strstr(name, (const char *)"1080 SNOWBOARDING"))
   {
      settings.swapmode_retro = true;
      //settings.alt_tex_size = 1;
      //depthmode = 0
      settings.swapmode = 2;
      smart_read = 1;
#ifdef HAVE_HWFBE
      optimize_texrect = 1;
      hires = 1;
#endif
      //fb_clear = 1;
   }
   else if (strstr(name, (const char *)"A Bug's Life"))
   {
      //depthmode = 0
      settings.zmode_compare_less = 1;
   }
   else if (strstr(name, (const char *)"Toy Story 2"))
   {
      settings.zmode_compare_less = 1;
   }
   else if (strstr(name, (const char *)"AERO FIGHTERS ASSAUL"))
   {
      settings.clip_zmin = 1;
   }
   else if (strstr(name, (const char *)"AIDYN CHRONICLES"))
   {
      //depthmode = 1
   }
   else if (strstr(name, (const char *)"All-Star Baseball 20"))
   {
      //force_depth_compare = 1
   }
   else if (
            strstr(name, (const char *)"All-Star Baseball 99")
         || strstr(name, (const char *)"All Star Baseball 99")
         )
   {
      //force_depth_compare = 1
      //depthmode = 1
      settings.buff_clear = 0;
   }
   else if (strstr(name, (const char *)"All-Star Baseball '0"))
   {
      //force_depth_compare = 1
      //depthmode = 0
      smart_read = 1;
#ifdef HAVE_HWFBE
      hires = 1;
#endif
   }
   else if (strstr(name, (const char *)"ARMYMENAIRCOMBAT"))
   {
      settings.increase_texrect_edge = 1;
      //depthmode = 1
   }
   else if (strstr(name, (const char *)"BURABURA POYON"))
   {
      //fix_tex_coord = 1
      //depthmode = 0
   }
   // ;Bakushou Jinsei 64 - Mezease! Resort Ou.
   else if (strstr(name, (const char *)"\xCA\xDE\xB8\xBC\xAE\xB3\xBC\xDE\xDD\xBE\xB2\x36\x34")) // ﾊﾞｸｼｮｳｼﾞﾝｾｲ64
   {
      //fb_info_disable = 1
      //depthmode = 0
   }
   else if (strstr(name, (const char *)"BAKU-BOMBERMAN")
         || strstr(name, (const char *)"BOMBERMAN64E")
         || strstr(name, (const char *)"BOMBERMAN64U"))
   {
      //depthmode = 0
      smart_read = 1;
#ifdef HAVE_HWFBE
      hires = 1;
#endif
   }
   else if (strstr(name, (const char *)"BAKUBOMB2")
         || strstr(name, (const char *)"BOMBERMAN64U2"))
   {
      settings.swapmode_retro = true;
      settings.filtering = 1;
      //depthmode = 0
   }
   else if (strstr(name, (const char *)"BANGAIOH"))
   {
      //depthmode = 1
   }
   else if (
            strstr(name, (const char *)"Banjo-Kazooie")
         || strstr(name, (const char *)"BANJO KAZOOIE 2")
         || strstr(name, (const char *)"BANJO TOOIE")
         )
   {
      settings.swapmode_retro = true;
      settings.filtering = 1;
      //depthmode = 1
      smart_read = 1;
#ifdef HAVE_HWFBE
      hires = 1;
#else
      read_always = 1;
#endif
   }
   else if (strstr(name, (const char *)"BASS HUNTER 64"))
   {
      //fix_tex_coord = 1
      //depthmode = 1
      settings.buff_clear = 0;
      settings.swapmode = 2;
   }
   else if (strstr(name, (const char *)"BATTLEZONE"))
   {
      //force_depth_compare = 1
      //depthmode = 1
   }
   else if (
            strstr(name, (const char *)"BEETLE ADVENTURE JP")
         || strstr(name, (const char *)"Beetle Adventure Rac")
         )
   {
      //wrap_big_tex = 1
      settings.n64_z_scale = 1;
      settings.filtering = 1;
      //depthmode = 1
      smart_read = 1;
#ifdef HAVE_HWFBE
      hires = 1;
#endif
   }
   else if (strstr(name, (const char *)"Bust A Move 3 DX")
         || strstr(name, (const char *)"Bust A Move '99")
         )
   {
      settings.filtering = 2;
      //depthmode = 1
   }
   else if (strstr(name, (const char *)"Bust A Move 2"))
   {
      //fix_tex_coord = 1
      settings.filtering = 2;
      //depthmode = 1
      settings.fog = 0;
   }
   else if (strstr(name, (const char *)"CARMAGEDDON64"))
   {
      //wrap_big_tex = 1
      settings. filtering = 1;
      //depthmode = 1
   }
   else if (strstr(name, (const char *)"HYDRO THUNDER"))
   {
      settings. filtering = 1;
   }
   else if (strstr(name, (const char *)"CENTRE COURT TENNIS"))
   {
      //soft_depth_compare = 1
      //depthmode = 0
   }
   else if (strstr(name, (const char *)"Chameleon Twist2"))
   {
      settings.filtering = 1;
      //depthmode = 0
   }
   else if (strstr(name, (const char *)"extreme_g")
         || strstr(name, (const char *)"extremeg"))
   {
      settings.swapmode_retro = true;
      //depthmode = 0
      smart_read = 1;
#ifdef HAVE_HWFBE
      hires = 1;
#endif
   }
   else if (strstr(name, (const char *)"Forsaken"))
   {
      settings.swapmode_retro = true;
   }
   else if (strstr(name, (const char *)"Extreme G 2"))
   {
      //depthmode = 0
      smart_read = 1;
#ifdef HAVE_HWFBE
      hires = 1;
#endif
      //fb_clear = 1
   }
   else if (strstr(name, (const char *)"MICKEY USA")
         || strstr(name, (const char *)"MICKEY USA PAL")
         )
   {
      settings.swapmode_retro = true;
      settings.alt_tex_size = 1;
      //depthmode = 1
      smart_read = 1;
#ifdef HAVE_HWFBE
      hires = 1;
#endif
      //fb_clear = 1
   }
   else if (strstr(name, (const char *)"MISCHIEF MAKERS")
         || strstr(name, (const char *)"TROUBLE MAKERS"))
   {
      settings.swapmode_retro = true;
      //mischief_tex_hack = 0
      //tex_wrap_hack = 0
      //depthmode = 1
      settings.filtering = 1;
      settings.fog = 0;
   }
      else if (strstr(name, (const char *)"Sin and Punishment"))
   {
      settings.swapmode_retro = true;
      settings.filtering = 1;
	  settings.old_style_adither = 1;
      //depthmode = 1
      smart_read = 1;
#ifdef HAVE_HWFBE
      hires = 1;
#endif
	}
   else if (strstr(name, (const char*)"Tigger's Honey Hunt"))
   {
      settings.zmode_compare_less = 1;
      //depthmode = 0
      settings.buff_clear = 0;
   }
   else if (strstr(name, (const char*)"TOM AND JERRY"))
   {
      settings.depth_bias = 2;
      settings.filtering = 1;
      //depthmode = 0
   }
   else if (strstr(name, (const char*)"SPACE DYNAMITES"))
   {
      settings.force_microcheck = 1;
   }
   else if (strstr(name, (const char*)"SPIDERMAN"))
   {
   }
   else if (strstr(name, (const char*)"STARCRAFT 64"))
   {
      settings.force_microcheck = 1;
      settings.aspectmode = 2;
      //depthmode = 1
   }
   else if (strstr(name, (const char*)"STAR SOLDIER"))
   {
      settings.force_microcheck = 1;
      settings.filtering = 1;
      //depthmode = 1
      settings.swapmode = 0;
   }
   else if (strstr(name, (const char*)"STAR WARS EP1 RACER"))
   {
      settings.swapmode = 2;
   }
   else if (strstr(name, (const char*)"TELEFOOT SOCCER 2000"))
   {
      settings.buff_clear = 0;
   }
   else if (strstr(name, (const char*)"TG RALLY 2"))
   {
      settings.filtering = 1;
      //depthmode = 1
      settings.buff_clear = 0;
      settings.swapmode = 2;
   }
   else if (strstr(name, (const char*)"Tonic Trouble"))
   {
      //depthmode = 0
      cpu_write_hack = 1;
   }
   else if (strstr(name, (const char*)"SUPERROBOTSPIRITS"))
   {
      settings.aspectmode = 2;
   }
   else if (strstr(name, (const char*)"THPS2")
         || strstr(name, (const char*)"THPS3")
         || strstr(name, (const char*)"TONY HAWK PRO SKATER")
         || strstr(name, (const char*)"TONY HAWK SKATEBOARD")
         )
   {
      settings.filtering = 1;
      //depthmode = 0
   }
   else if (strstr(name, (const char*)"TOP GEAR RALLY 2"))
   {
      settings.filtering = 1;
      //depthmode = 1
      settings.buff_clear = 0;
      settings.swapmode = 2;
   }
   else if (strstr(name, (const char*)"TRIPLE PLAY 2000"))
   {
      //wrap_big_tex = 1
      //depthmode = 0
      smart_read = 1;
#ifdef HAVE_HWFBE
      hires = 1;
#endif
   }
   else if (strstr(name, (const char*)"TSUWAMONO64"))
   {
      settings.force_microcheck = 1;
      //depthmode = 0
   }
   else if (strstr(name, (const char *)"TSUMI TO BATSU"))
   {
      settings.swapmode_retro = true;
      settings.filtering = 1;
	  settings.old_style_adither = 1;
      //depthmode = 1
      smart_read = 1;
#ifdef HAVE_HWFBE
      hires = 1;
#endif
      //fb_clear = 1
   }
   else if (strstr(name, (const char *)"MortalKombatTrilogy"))
   {
      settings.filtering = 2;
      //depthmode = 1
   }
   else if (strstr(name, (const char *)"Perfect Dark"))
   {
      settings.swapmode_retro = true;
      useless_is_useless = 1;
      settings.decrease_fillrect_edge = 1;
      settings.filtering = 1;
      //depthmode = 1
      smart_read = 1;
#ifdef HAVE_HWFBE
      optimize_texrect = 0;
      hires = 1;
#endif
      //fb_clear = 1
   }
   else if (strstr(name, (const char *)"Resident Evil II") || strstr(name, (const char *)"BioHazard II"))
   {
      cpu_write_hack = 1;
      settings.adjust_aspect = 0;
      settings.n64_z_scale = 1;
      //fix_tex_coord = 128
      //depthmode = 0
      settings.swapmode = 2;
      smart_read = 1;
#ifdef HAVE_HWFBE
      hires = 1;
#endif
   }
   else if (strstr(name, (const char *)"World Cup 98"))
   {
      //depthmode = 0
      settings.swapmode = 0;
      smart_read = 1;
#ifdef HAVE_HWFBE
      hires = 1;
#endif
   }
   else if (strstr(name, (const char *)"EXCITEBIKE64"))
   {
      //depthmode = 0
      smart_read = 1;
#ifdef HAVE_HWFBE
      hires = 1;
#endif
   }
   else if (strstr(name, (const char *)"\xB4\xB8\xBD\xC4\xD8\xB0\xD1\x47\x32")) // ｴｸｽﾄﾘｰﾑG2
   {
      //;Extreme-G 2
      //depthmode = 0
      smart_read = 0;
#ifdef HAVE_HWFBE
      hires = 1;
#endif
      //fb_clear = 1
   }
   else if (strstr(name, (const char *)"EARTHWORM JIM 3D"))
   {
      //increase_primdepth = 1
      settings.filtering = 1;
      //depthmode = 0
      settings.buff_clear = 0;
   }
   else if (strstr(name, (const char *)"Cruis'n USA"))
   {
      settings.filtering = 1;
      //depthmode = 1
      smart_read = 1;
#ifdef HAVE_HWFBE
      hires = 1;
#endif
      //fb_clear = 1
   }
   else if (strstr(name, (const char *)"CruisnExotica"))
   {
      settings.filtering = 1;
      //depthmode = 1
      settings.buff_clear = 0;
      settings.swapmode = 0;
   }
   else if (strstr(name, (const char *)"custom robo")
         || strstr(name, (const char *)"CUSTOMROBOV2"))
   {
      //depthmode = 0
      smart_read = 1;
#ifdef HAVE_HWFBE
      hires = 1;
#endif
   }
   else if (strstr(name, (const char *)"\xB4\xB2\xBA\xB3\xC9\xBE\xDD\xC4\xB1\xDD\xC4\xDE\xD8\xAD\xB0\xBD")) // ｴｲｺｳﾉｾﾝﾄｱﾝﾄﾞﾘｭｰｽ
   {
      //;Eikou no Saint Andrews
      settings.correct_viewport = 1;
   }
   else if (strstr(name, (const char *)"Eltail"))
   {
      settings.filtering = 2;
      //depthmode = 1
   }
   else if (strstr(name, (const char *)"DeadlyArts"))
   {
      //soft_depth_compare = 1
      //depthmode = 0
      smart_read = 1;
#ifdef HAVE_HWFBE
      hires = 1;
#endif
      settings.clip_zmin = 1;
   }
   else if (strstr(name, (const char *)"Bottom of the 9th"))
   {
      settings.filtering = 1;
      //depthmode = 0;
      smart_read = 1;
#ifdef HAVE_HWFBE
      optimize_texrect = 0;
      hires = 1;
#endif
   }
   else if (strstr(name, (const char *)"BRUNSWICKBOWLING"))
   {
      //depthmode = 0
      settings.buff_clear = 0;
      settings.swapmode = 0;
   }
   else if (strstr(name, (const char *)"CHOPPER ATTACK"))
   {
      settings.filtering = 1;
      //depthmode = 0
   }
   else if (strstr(name, (const char *)"CITY TOUR GP"))
   {
      settings.force_microcheck = 1;
      settings.filtering = 1;
      //depthmode = 1
   }
   else if (strstr(name, (const char *)"Command&Conquer"))
   {
      //fix_tex_coord = 1
      settings.adjust_aspect = 2;
      settings.filtering = 1;
      //depthmode = 1
      settings.fog = 0;
   }
   else if (strstr(name, (const char *)"CONKER BFD"))
   {
      //ignore_previous = 1
      settings.lodmode = 1;
      settings.filtering = 1;
      //depthmode = 0
      smart_read = 1;
#ifdef HAVE_HWFBE
      optimize_texrect = 1;
      hires = 1;
#endif
      //fb_clear = 1
   }
   else if (strstr(name, (const char*)"DARK RIFT"))
   {
      settings.force_microcheck = 1;
   }
   else if (strstr(name, (const char*)"Donald Duck Goin' Qu")
         || strstr(name, (const char*)"Donald Duck Quack At"))
   {
      cpu_write_hack = 1;
      //depthmode = 0
   }
   else if (strstr(name, (const char*)"\xC4\xDE\xD7\xB4\xD3\xDD\x33\x20\xC9\xCB\xDE\xC0\xC9\xCF\xC1\x53\x4F\x53\x21")) // ﾄﾞﾗｴﾓﾝ3 ﾉﾋﾞﾀﾉﾏﾁSOS!
   {
      //;Doraemon 3 - Nobita no Machi SOS! (J)
      settings.clip_zmin = 1;
   }
   else if (strstr(name, (const char*)"DR.MARIO 64"))
   {
      //fix_tex_coord = 256
      //optimize_write = 1
      read_back_to_screen = 1;
      //depthmode = 1
      smart_read = 1;
#ifdef HAVE_HWFBE
      hires = 0;
#endif
   }
   else if (strstr(name, (const char*)"F1 POLE POSITION 64"))
   {
      settings.clip_zmin = 1;
      settings.filtering = 2;
      //depthmode = 1
   }
   else if (strstr(name, (const char*)"HUMAN GRAND PRIX"))
   {
      settings.filtering = 2;
      //depthmode = 0
   }
   else if (strstr(name, (const char*)"F1RacingChampionship"))
   {
      //depthmode = 0
      settings.buff_clear = 0;
      settings.swapmode = 0;
   }
   else if (strstr(name, (const char*)"F1 WORLD GRAND PRIX")
         || strstr(name, (const char*)"F1 WORLD GRAND PRIX2"))
   {
      //soft_depth_compare = 1
      //wrap_big_tex = 1
      //depthmode = 0
      settings.buff_clear = 0;
   }
   else if (strstr(name, (const char*)"\x46\x33\x20\xCC\xB3\xD7\xB2\xC9\xBC\xDA\xDD\x32")) // F3 ﾌｳﾗｲﾉｼﾚﾝ2
   {
      //;Fushigi no Dungeon - Fuurai no Shiren 2 (J)
      settings.decrease_fillrect_edge = 1;
      //depthmode = 0
   }
   else if (strstr(name, (const char*)"G.A.S.P!!Fighters'NE"))
   {
      //soft_depth_compare = 1
      //depthmode = 0
      smart_read = 1;
#ifdef HAVE_HWFBE
      hires = 1;
#endif
      settings.clip_zmin = 1;
   }
   else if (strstr(name, (const char*)"MS. PAC-MAN MM"))
   {
      cpu_write_hack = 1;
      //depthmode = 1
   }
   else if (strstr(name, (const char*)"NBA Courtside 2")
         || strstr(name, (const char*)"NASCAR 2000")
         || strstr(name, (const char*)"NASCAR 99")
         )
   {
      //depthmode = 0
      settings.buff_clear = 0;
      settings.swapmode = 0;
   }
   else if (strstr(name, (const char*)"NBA JAM 2000")
         || strstr(name, (const char*)"NBA JAM 99")
         )
   {
      settings.buff_clear = 0;
   }
   else if (strstr(name, (const char*)"NBA LIVE 2000")
         )
   {
      settings.adjust_aspect = 0;
   }
   else if (strstr(name, (const char*)"NBA Live 99")
         )
   {
      settings.swapmode = 0;
      settings.adjust_aspect = 0;
   }
   else if (strstr(name, (const char*)"NINTAMAGAMEGALLERY64"))
   {
      settings.force_microcheck = 1;
      //depthmode = 0
   }
   else if (strstr(name, (const char*)"NFL QBC 2000")
         || strstr(name, (const char*)"NFL Quarterback Club")
         )
   {
      //wrap_big_tex = 1
      //depthmode = 0
      settings.swapmode = 0;
   }
   else if (strstr(name, (const char*)"GANBAKE GOEMON")
         //|| strstr(name, (const char*)"\xB6\xDE\xDD\xCA\xDE\xDA\x5C\x20\xBA\xDE\xB4\xD3\xDD") */ TODO: illegal characters - find by ucode CRC */ // ｶﾞﾝﾊﾞﾚ¥ ｺﾞｴﾓﾝ
         || strstr(name, (const char*)"MYSTICAL NINJA")
         || strstr(name, (const char*)"MYSTICAL NINJA2 SG")
         )
   {
      //;Ganbare Goemon
      optimize_texrect = 0;
      settings.alt_tex_size = 1;
      settings.filtering = 1;
      //depthmode = 1
      smart_read = 1;
#ifdef HAVE_HWFBE
      hires = 1;
#endif
   }
   else if (strstr(name, (const char*)"GAUNTLET LEGENDS"))
   {
      //depthmode = 1
      settings.swapmode = 2;
   }
   else if (strstr(name, (const char*)"Getter Love!!"))
   {
      settings.zmode_compare_less = 1;
      //texrect_compare_less = 1
      settings.filtering = 2;
      //depthmode = 1
   }
   else if (strstr(name, (const char*)"GOEMON2 DERODERO")
         || strstr(name, (const char*)"GOEMONS GREAT ADV"))
   {
      settings.filtering = 1;
      //depthmode = 1
      smart_read = 1;
#ifdef HAVE_HWFBE
      hires = 1;
#endif
   }
   else if (strstr(name, (const char*)"GOLDEN NUGGET 64"))
   {
      settings.filtering = 2;
      //depthmode = 1
   }
   else if (strstr(name, (const char*)"GT64"))
   {
      settings.force_microcheck = 1;
      settings.filtering = 1;
      //depthmode = 1
   }
   else if (strstr(name, (const char*)"\xCA\xD1\xBD\xC0\xB0\xD3\xC9\xB6\xDE\xC0\xD8\x36\x34") // ﾊﾑｽﾀｰﾓﾉｶﾞﾀﾘ64
         )
   {
      //;Hamster Monogatari 64
      settings.force_microcheck = 1;
      //depthmode = 0
   }
   else if (strstr(name, (const char*)"HARVESTMOON64")
         || strstr(name, (const char*)"\xCE\xDE\xB8\xBC\xDE\xAE\xB3\xD3\xC9\xB6\xDE\xC0\xD8\x32") // ﾎﾞｸｼﾞｮｳﾓﾉｶﾞﾀﾘ2
         )
   {
      //;Bokujou Monogatari 2
      settings.zmode_compare_less = 1;
      //depthmode = 0
      settings.fog = 0;
   }
   else if (strstr(name, (const char*)"MGAH VOL1"))
   {
      settings.force_microcheck = 1;
      //depthmode = 1
      settings.zmode_compare_less = 1;
      smart_read = 1;
   }
   else if (strstr(name, (const char*)"MARIO STORY")
         || strstr(name, (const char*)"PAPER MARIO")
         )
   {
      useless_is_useless = 1;
      hires_buf_clear = 0;
      settings.filtering = 1;
      //depthmode = 1
      settings.swapmode = 2;
      smart_read = 1;
#ifdef HAVE_HWFBE
      optimize_texrect = 0;
      hires = 1;
#endif
      read_alpha = 1;
   }
   else if (strstr(name, (const char*)"Mia Hamm Soccer 64"))
   {
      settings.buff_clear = 0;
   }
   else if (strstr(name, (const char*)"NITRO64"))
   {
      smart_read = 1;
#ifdef HAVE_HWFBE
      hires = 1;
#endif
   }
   else if (strstr(name, (const char*)"NUCLEARSTRIKE64"))
   {
      settings.buff_clear = 0;
   }
   else if (strstr(name, (const char*)"NFL BLITZ")
         || strstr(name, (const char*)"NFL BLITZ 2001")
         || strstr(name, (const char*)"NFL BLITZ SPECIAL ED")
         )
   {
      settings.lodmode = 1;
   }
   else if (strstr(name, (const char*)"Monaco Grand Prix")
         || strstr(name, (const char*)"Monaco GP Racing 2")
         )
   {
      //depthmode = 0
      settings.buff_clear = 0;
   }
   else if (strstr(name, (const char*)"\xD3\xD8\xC0\xBC\xAE\xB3\xB7\xDE\x36\x34")) // ﾓﾘﾀｼｮｳｷﾞ64
   {
      //;Morita Shougi 64
      settings.correct_viewport = 1;
   }
   else if (strstr(name, (const char*)"NEWTETRIS"))
   {
      settings.pal230 = 1;
      //fix_tex_coord = 1
      settings.increase_texrect_edge = 1;
      //depthmode = 0
      settings.fog = 0;
   }
   else if (strstr(name, (const char*)"MLB FEATURING K G JR"))
   {
      read_back_to_screen = 2;
      //depthmode = 1
   }
   else if (strstr(name, (const char*)"HSV ADVENTURE RACING"))
   {
      //wrap_big_tex = 1
      settings.n64_z_scale = 1;
      settings.filtering = 1;
      //depthmode = 1
      smart_read = 1;
#ifdef HAVE_HWFBE
      hires = 1;
#endif
   }
   else if (strstr(name, (const char*)"MarioGolf64"))
   {
      //fb_info_disable = 1
      ignore_aux_copy = 1;
      settings.buff_clear = 0;
      //depthmode = 0
      smart_read = 1;
#ifdef HAVE_HWFBE
      hires = 1;
#endif
      //fb_clear = 1
   }
   else if (strstr(name, (const char*)"Virtual Pool 64"))
   {
      //depthmode = 1
      settings.buff_clear = 0;
   }
   else if (strstr(name, (const char*)"TWINE"))
   {
      settings.filtering = 1;
      //depthmode = 0
   }
   else if (strstr(name, (const char*)"V-RALLY"))
   {
      //fix_tex_coord = 3
      settings.filtering = 1;
      //depthmode = 0
      settings.buff_clear = 0;
      settings.swapmode = 0;
   }
   else if (strstr(name, (const char*)"Waialae Country Club"))
   {
      //wrap_big_tex = 1
      //depthmode = 0
      smart_read = 1;
#ifdef HAVE_HWFBE
      hires = 1;
#endif
   }
   else if (strstr(name, (const char*)"TWISTED EDGE"))
   {
      //depthmode = 1
      smart_read = 1;
#ifdef HAVE_HWFBE
      hires = 1;
#endif
      //fb_clear = 1
   }
   else if (strstr(name, (const char*)"STAR TWINS")
         || strstr(name, (const char*)"JET FORCE GEMINI")
         || strstr(name, (const char*)"J F G DISPLAY")
         )
   {
      read_back_to_screen = 1;
      settings.decrease_fillrect_edge = 1;
      //settings.alt_tex_size = 1;
      //depthmode = 1
      settings.swapmode = 2;
      smart_read = 1;
#ifdef HAVE_HWFBE
      hires = 1;
#endif
   }
   else if (strstr(name, (const char*)"\xC4\xDE\xD7\xB4\xD3\xDD\x20\xD0\xAF\xC2\xC9\xBE\xB2\xDA\xB2\xBE\xB7")) // ﾄﾞﾗｴﾓﾝ ﾐｯﾂﾉｾｲﾚｲｾｷ
   {
      //;Doraemon - Mittsu no Seireiseki (J)
      read_back_to_screen = 1;
      //depthmode = 1
      smart_read = 1;
#ifdef HAVE_HWFBE
      hires = 1;
#endif
   }
   else if (strstr(name, (const char*)"\x48\x45\x49\x57\x41\x20\xCA\xDF\xC1\xDD\xBA\x20\xDC\xB0\xD9\xC4\xDE\x36\x34")) // HEIWA ﾊﾟﾁﾝｺ ﾜｰﾙﾄﾞ64
   {
      //;Heiwa Pachinko World 64
      //depthmode = 0
      settings.fog = 0;
      settings.swapmode = 2;
      smart_read = 1;
#ifdef HAVE_HWFBE
      hires = 1;
#endif
   }
   else if (strstr(name, (const char*)"\xB7\xD7\xAF\xC4\xB6\xB2\xB9\xC2\x20\x36\x34\xC0\xDD\xC3\xB2\xC0\xDE\xDD")) // ｷﾗｯﾄｶｲｹﾂ 64ﾀﾝﾃｲﾀﾞﾝ
   {
      //;Kiratto Kaiketsu! 64 Tanteidan
      settings.filtering = 1;
      //depthmode = 0
      settings.buff_clear = 0;
   }
   else if (strstr(name, (const char*)"\xBD\xB0\xCA\xDF\xB0\xDB\xCE\xDE\xAF\xC4\xC0\xB2\xBE\xDD\x36\x34")) // ｽｰﾊﾟｰﾛﾎﾞｯﾄﾀｲｾﾝ64
   {
      //;Super Robot Taisen 64 (J)
      smart_read = 1;
#ifdef HAVE_HWFBE
      hires = 1;
#endif
   }
   else if (strstr(name, (const char*)"Supercross"))
   {
      //depthmode = 1
      settings.buff_clear = 0;
   }
   else if (strstr(name, (const char*)"Top Gear Overdrive"))
   {
      //fb_info_disable = 1
      //depthmode = 0
      settings.buff_clear = 0;
   }
   else if (strstr(name, (const char*)"\xBD\xBD\xD2\x21\xC0\xB2\xBE\xDD\xCA\xDF\xBD\xDE\xD9\xC0\xDE\xCF")) // ｽｽﾒ!ﾀｲｾﾝﾊﾟｽﾞﾙﾀﾞﾏ
   {
      //;Susume! Taisen Puzzle Dama
      settings.force_microcheck = 1;
      //depthmode = 1
      settings.fog = 0;
      settings.swapmode = 0;
   }
   else if (strstr(name, (const char*)"\xD0\xDD\xC5\xC3\xDE\xC0\xCF\xBA\xDE\xAF\xC1\xDC\xB0\xD9\xC4\xDE")) // ﾐﾝﾅﾃﾞﾀﾏｺﾞｯﾁﾜｰﾙﾄﾞ
   {
      //;Minna de Tamagocchi World / Tamagotchi World 64 (J)
      //depthmode = 0
      settings.fog = 0;
   }
   else if (strstr(name, (const char*)"Taz Express"))
   {
      settings.filtering = 1;
      //depthmode = 0
      settings.buff_clear = 0;
   }
   else if (strstr(name, (const char*)"Top Gear Hyper Bike"))
   {
      //fb_info_disable = 1
      settings.swapmode = 2;
      //depthmode = 0
      smart_read = 1;
#ifdef HAVE_HWFBE
      hires = 1;
#endif
      //fb_clear = 1
   }
   else if (strstr(name, (const char*)"I S S 64"))
   {
      //depthmode = 1
      settings.swapmode = 2;
      settings.old_style_adither = 1;
   }
   else if (strstr(name, (const char*)"I.S.S.2000"))
   {
      //depthmode = 1
      smart_read = 1;
#ifdef HAVE_HWFBE
      hires = 1;
#endif
   }
   else if (strstr(name, (const char*)"ITF 2000")
         || strstr(name, (const char*)"IT&F SUMMERGAMES")
         )
   {
      settings.filtering = 1;
      //depthmode = 1
      smart_read = 1;
#ifdef HAVE_HWFBE
      hires = 1;
#endif
   }
   else if (strstr(name, (const char*)"J_league 1997"))
   {
      //fix_tex_coord = 1
      //depthmode = 1
      settings.swapmode = 0;
   }
#if 0
   // TODO: illegal characters - will have to find this game by ucode CRC
   // later
   else if (strstr(name, (const char*)"\x4A\xD8\xB0\xB8\xDE\x5C\x20\xB2\xDA\xCC\xDE\xDD\xCB\xDE\xB0\xC4\x31\x39\x39\x37")) // Jﾘｰｸﾞ¥ ｲﾚﾌﾞﾝﾋﾞｰﾄ1997
   {
      //;J.League Eleven Beat 1997
      smart_read = 1;
#ifdef HAVE_HWFBE
      hires = 1;
#endif
   }
#endif
   else if (strstr(name, (const char*)"J WORLD SOCCER3"))
   {
      //depthmode = 1
      settings.swapmode = 2;
   }
   else if (strstr(name, (const char*)"KEN GRIFFEY SLUGFEST"))
   {
      read_back_to_screen = 2;
      //depthmode = 1
      settings.swapmode = 0;
      smart_read = 1;
#ifdef HAVE_HWFBE
      hires = 1;
#endif
   }
   else if (strstr(name, (const char*)"MASTERS'98"))
   {
      //wrap_big_tex = 1
      //depthmode = 0
      smart_read = 1;
#ifdef HAVE_HWFBE
      hires = 1;
#endif
   }
   else if (strstr(name, (const char*)"MO WORLD LEAGUE SOCC"))
   {
      settings.buff_clear = 0;
   }
   else if (strstr(name, (const char*)"\xC7\xBC\xC2\xDE\xD8\x36\x34")) // ﾇｼﾂﾞﾘ64
   {
      //; Nushi Tsuri 64 / Nushi Zuri 64
      settings.force_microcheck = 1;
      //wrap_big_tex = 0
      //depthmode = 0
      settings.buff_clear = 0;
   }
   else if (strstr(name, (const char*)"PACHINKO365NICHI"))
   {
      settings.correct_viewport = 1;
   }
   else if (strstr(name, (const char*)"PERFECT STRIKER"))
   {
      //depthmode = 1
      settings.swapmode = 2;
   }
   else if (strstr(name, (const char*)"ROCKETROBOTONWHEELS"))
   {
      settings.clip_zmin = 1;
   }
   else if (strstr(name, (const char*)"SD HIRYU STADIUM"))
   {
      settings.force_microcheck = 1;
      //depthmode = 0
   }
   else if (strstr(name, (const char*)"Shadow of the Empire"))
   {
      settings.swapmode = 2;
   }
   else if (strstr(name, (const char*)"RUSH 2049"))
   {
      //force_texrect_zbuf = 1
      settings.filtering = 1;
      //depthmode = 0
   }
   else if (strstr(name, (const char*)"SCARS"))
   {
      settings.filtering = 1;
      //depthmode = 0
   }
   else if (strstr(name, (const char*)"LEGORacers"))
   {
      cpu_write_hack = 1;
      //depthmode = 1
      settings.buff_clear = 0;
      smart_read = 1;
#ifdef HAVE_HWFBE
      hires = 1;
#endif
      read_alpha = 1;
   }
   else if (strstr(name, (const char*)"Lode Runner 3D"))
   {
      settings.swapmode = 0;
   }
   else if (strstr(name, (const char*)"Parlor PRO 64"))
   {
      settings.force_microcheck = 1;
      settings.filtering = 1;
      //depthmode = 1
   }
   else if (strstr(name, (const char*)"PUZZLE LEAGUE N64")
         || strstr(name, (const char*)"PUZZLE LEAGUE"))
   {
      //PPL = 1
      settings.force_microcheck = 1;
      //fix_tex_coord = 1
      settings.filtering = 2;
      //depthmode = 1
      settings.fog = 0;
      settings.buff_clear = 0;
      smart_read = 1;
#ifdef HAVE_HWFBE
      hires = 0;
#endif
      read_alpha = 1;
   }
   else if (strstr(name, (const char*)"POKEMON SNAP"))
   {
      //depthmode = 1
#ifdef HAVE_HWFBE
      hires = 0;
#else
      read_always = 1;
#endif
      //fb_clear = 1
   }
   else if (strstr(name, (const char*)"POKEMON STADIUM")
         || strstr(name, (const char*)"POKEMON STADIUM G&S")
         )
   {
      //depthmode = 1
      settings.buff_clear = 0;
      smart_read = 1;
#ifdef HAVE_HWFBE
      optimize_texrect = 0;
      hires = 0;
#endif
      read_alpha = 1;
      fb_crc_mode = 2;
   }
   else if (strstr(name, (const char*)"POKEMON STADIUM 2"))
   {
      //depthmode = 1
      settings.buff_clear = 0;
      settings.swapmode = 2;
      smart_read = 1;
#ifdef HAVE_HWFBE
      optimize_texrect = 0;
      hires = 1;
#endif
      read_alpha = 1;
      fb_crc_mode = 2;
   }
   else if (strstr(name, (const char*)"RAINBOW SIX"))
   {
      settings.increase_texrect_edge = 1;
      //depthmode = 1
   }
   else if (strstr(name, (const char*)"RALLY CHALLENGE")
         || strstr(name, (const char*)"Rally'99"))
   {
      settings.filtering = 1;
      //depthmode = 1
      settings.buff_clear = 0;
      smart_read = 1;
#ifdef HAVE_HWFBE
      hires = 1;
#endif
   }
   else if (strstr(name, (const char*)"Rayman 2"))
   {
      //depthmode = 0
      cpu_write_hack = 1;
   }
   else if (strstr(name, (const char*)"quarterback_club_98"))
   {
      hires_buf_clear = 0;
      settings.filtering = 1;
      //depthmode = 1
      settings.swapmode = 0;
      settings.buff_clear = 0;
      smart_read = 1;
#ifdef HAVE_HWFBE
      optimize_texrect = 0;
      hires = 1;
#endif
      read_alpha = 1;
   }
   else if (strstr(name, (const char*)"PowerLeague64"))
   {
      settings.force_quad3d = 1;
   }
   else if (strstr(name, (const char*)"Racing Simulation 2"))
   {
      //depthmode = 0
      settings.buff_clear = 0;
   }
   else if (strstr(name, (const char*)"TOP GEAR RALLY"))
   {
      settings.depth_bias = 64;
      //fillcolor_fix = 1
      //depthmode = 0
   }
   else if (strstr(name, (const char*)"SMASH BROTHERS"))
   {
      settings.swapmode_retro = true;
   }
#if 0
   else if (strstr(name, (const char*)"POLARISSNOCROSS"))
   {
      //fix_tex_coord = 5
      //depthmode = 1
   }
   else if (strstr(name, (const char*)"READY 2 RUMBLE"))
   {
      //fix_tex_coord = 64
      //depthmode = 0
   }
   else if (strstr(name, (const char*)"Ready to Rumble"))
   {
      //fix_tex_coord = 1
      //depthmode = 0
   }
   else if (strstr(name, (const char*)"LT DUCK DODGERS"))
   {
      //wrap_big_tex = 1
      //depthmode = 1
   }
   else if (strstr(name, (const char*)"LET'S SMASH"))
   {
      //soft_depth_compare = 1
      //depthmode = 0
   }
   else if (strstr(name, (const char*)"LCARS - WT_Riker"))
   {
      //depthmode = 1
   }
   else if (strstr(name, (const char*)"RUGRATS IN PARIS"))
   {
      //depthmode = 1
   }
   else if (strstr(name, (const char*)"Shadowman"))
   {
      //depthmode = 0
   }
   else if (strstr(name, (const char*)"J LEAGUE LIVE 64"))
   {
      //wrap_big_tex = 1
      //depthmode = 1
   }
   else if (strstr(name, (const char*)"Iggy's Reckin' Balls"))
   {
      //fix_tex_coord = 512
      //depthmode = 0
   }
   else if (strstr(name, (const char*)"Ultraman Battle JAPA"))
   {
      //depthmode = 0
   }
   else if (strstr(name, (const char*)"D K DISPLAY"))
   {
      //depthmode = 1
      //fb_clear = 1
   }
   else if (strstr(name, (const char*)"MarioParty3"))
   {
      //fix_tex_coord = 1
      //depthmode = 0
   }
   else if (strstr(name, (const char*)"MK_MYTHOLOGIES"))
   {
      //depthmode = 1
   }
   else if (strstr(name, (const char*)"NFL QBC '99"))
   {
      //force_depth_compare = 1
      //wrap_big_tex = 1
      //depthmode = 0
   }
   else if (strstr(name, (const char*)"OgreBattle64"))
   {
      //fb_info_disable = 1
      //force_depth_compare = 1
      //depthmode = 1
   }
   else if (strstr(name, (const char*)"MICROMACHINES64TURBO"))
   {
      //depthmode = 0
   }
   else if (strstr(name, (const char*)"Fighting Force"))
   {
      //depthmode = 1
   }
   else if (strstr(name, (const char *)"D K DISPLAY"))
   {
      //depthmode = 1
      //fb_clear = 1
   }
   else if (strstr(name, (const char *)"DAFFY DUCK STARRING"))
   {
      //depthmode = 1
      //wrap_big_tex = 1
   }
   else if (strstr(name, (const char *)"CyberTiger"))
   {
      //fix_tex_coord = 16
      //depthmode = 0
   }
   else if (strstr(name, (const char *)"F-Zero X") || strstr(name, (const char *)"F-ZERO X"))
   {
      settings.swapmode_retro = true;
      //depthmode = 1
   }
   else if (strstr(name, (const char *)"DERBYSTALLION64"))
   {
      //fix_tex_coord = 1
      //depthmode = 0
   }
   else if (strstr(name, (const char *)"DUKE NUKEM"))
   {
      //increase_primdepth = 1
      //depthmode = 0
   }
   else if (strstr(name, (const char *)"EVANGELION"))
   {
      //depthmode = 1
   }
   else if (strstr(name, (const char *)"Big Mountain 2000"))
   {
      //depthmode = 1
   }
   else if (strstr(name, (const char *)"YAKOUTYUU2"))
   {
      //depthmode = 0
   }
   else if (strstr(name, (const char *)"WRESTLEMANIA 2000"))
   {
      //depthmode = 0
   }
#endif
   else if (strstr(name, (const char *)"Pilot Wings64"))
   {
      settings.swapmode_retro = true;
      settings.depth_bias = 10;
      //depthmode = 1
      settings.buff_clear = 0;
   }
   else if (strstr(name, (const char *)"DRACULA MOKUSHIROKU")
         || strstr(name, (const char *)"DRACULA MOKUSHIROKU2")
         )
   {
      //depthmode = 0
      //fb_clear = 1
      settings.old_style_adither = 1;
   }
   else if (strstr(name, (const char *)"CASTLEVANIA")
         || strstr(name, (const char *)"CASTLEVANIA2"))
   {
      settings.swapmode_retro = true;
	  settings.old_style_adither = 1;
      //depthmode = 0
      //fb_clear = 1
#ifndef HAVE_HWFBE
      read_always = 1;
#endif
   }
   else if (strstr(name, (const char *)"Dual heroes JAPAN")
         || strstr(name, (const char *)"Dual heroes PAL")
         || strstr(name, (const char *)"Dual heroes USA"))
   {
      settings.filtering = 1;
      //depthmode = 0
      settings.swapmode = 0;
   }
   else if (strstr(name, (const char *)"JEREMY MCGRATH SUPER"))
   {
      //depthmode = 0
      settings.swapmode = 0;
   }
   else if (strstr(name, (const char *)"Kirby64"))
   {
      settings.filtering = 1;
      //depthmode = 0
      settings.buff_clear = 0;
      settings.swapmode = 0;
   }
   else if (strstr(name, (const char *)"GOLDENEYE"))
   {
      settings.swapmode_retro = true;
      settings.lodmode = 1;
      settings.depth_bias = 40;
      settings.filtering = 1;
      //depthmode = 0
      smart_read = 1;
#ifdef HAVE_HWFBE
      hires = 1;
#endif
   }
   else if (strstr(name, (const char *)"DONKEY KONG 64"))
   {
      settings.lodmode = 1;
      settings.depth_bias = 64;
      //depthmode = 1
      //fb_clear = 1
      read_always = 1;
   }
   else if (strstr(name, (const char *)"Glover"))
   {
      settings.filtering = 1;
      //depthmode = 0;
   }
   else if (strstr(name, (const char *)"GEX: ENTER THE GECKO")
         || strstr(name, (const char *)"Gex 3 Deep Cover Gec"))
   {
      settings.filtering = 1;
      //depthmode = 0;
   }
   else if (strstr(name, (const char *)"WAVE RACE 64"))
   {
      settings.swapmode_retro = true;
      settings.lodmode = 1;
      settings.pal230 = 1;
   }
   else if (strstr(name, (const char *)"WILD CHOPPERS"))
   {
      settings.filtering = 1;
      //depthmode = 0
   }
   else if (strstr(name, (const char *)"Wipeout 64"))
   {
      settings.filtering = 1;
      //depthmode = 0
      settings.swapmode = 0;
   }
   else if (strstr(name, (const char *)"WONDER PROJECT J2"))
   {
      //depthmode = 0
      settings.buff_clear = 0;
      settings.swapmode = 0;
   }
   else if (strstr(name, (const char *)"Doom64"))
   {
      settings.swapmode_retro = true;
      //fillcolor_fix = 1
      //depthmode = 1
   }
   else if (strstr(name, (const char *)"HEXEN"))
   {
      cpu_write_hack = 1;
      settings.filtering = 1;
      //depthmode = 1
      settings.buff_clear = 0;
      settings.swapmode = 2;
   }
   else if (strstr(name, (const char *)"ZELDA MAJORA'S MASK") || strstr(name, (const char *)"THE MASK OF MUJURA"))
   {
      settings.swapmode_retro = true;
      //wrap_big_tex = 1
      settings.filtering = 1;
      smart_read = 1;
#ifdef HAVE_HWFBE
      hires = 1;
#endif
      //fb_clear = 1
      fb_crc_mode = 0;
   }
   else if (strstr(name, (const char *)"THE LEGEND OF ZELDA") || strstr(name, (const char *)"ZELDA MASTER QUEST"))
   {
      settings.swapmode_retro = true;
      settings.filtering = 1;
      //depthmode = 1
      settings.lodmode = 1;
      smart_read = 1;
#ifdef HAVE_HWFBE
      hires = 1;
#endif
      //fb_clear = 1
      //
      settings.hacks |= hack_OOT;
   }
   else if (strstr(name, (const char*)"Re-Volt"))
   {
      //depthmode = 1
   }
   else if (strstr(name, (const char*)"RIDGE RACER 64"))
   {
      settings.swapmode_retro = true;
      settings.force_calc_sphere = 1;
      //depthmode = 0
      smart_read = 1;
#ifdef HAVE_HWFBE
      hires = 1;
#else
      read_always = 1;
#endif
   }
   else if (strstr(name, (const char*)"ROAD RASH 64"))
   {
      //depthmode = 0
      settings.swapmode = 2;
   }
   else if (strstr(name, (const char*)"Robopon64"))
   {
      //depthmode = 0
      smart_read = 1;
#ifdef HAVE_HWFBE
      hires = 1;
#endif
   }
   else if (strstr(name, (const char*)"RONALDINHO SOCCER"))
   {
      //depthmode = 1
      settings.swapmode = 2;
      settings.old_style_adither = 1;
   }
   else if (strstr(name, (const char*)"RTL WLS2000"))
   {
      settings.buff_clear = 0;
   }
   else if (strstr(name, (const char*)"BIOFREAKS"))
   {
      //depthmode = 0
      settings.buff_clear = 0;
      smart_read = 1;
#ifdef HAVE_HWFBE
      hires = 1;
#endif
   }
   else if (strstr(name, (const char *)"Blast Corps") || strstr(name, (const char *)"Blastdozer"))
   {
      //depthmode = 1
      settings.swapmode = 0;
      smart_read = 1;
#ifdef HAVE_HWFBE
      hires = 1;
#endif
      read_alpha = 1;
   }
   else if (strstr(name, (const char *)"blitz2k"))
   {
      settings.lodmode = 1;
   }
   else if (strstr(name, (const char *)"Body Harvest"))
   {
      //depthmode = 1
      smart_read = 1;
#ifdef HAVE_HWFBE
      hires = 1;
#endif
   }
   else if (strstr(name, (const char *)"Killer Instinct Gold") || strstr(name, (const char *)"KILLER INSTINCT GOLD"))
   {
      settings.swapmode_retro = true;
      settings.filtering = 1;
      //depthmode = 0
      settings.buff_clear = 0;
   }
   else if (strstr(name, (const char*)"KNIFE EDGE"))
   {
      //wrap_big_tex = 1
      settings.filtering = 1;
      //depthmode = 1
   }
   else if (strstr(name, (const char*)"Knockout Kings 2000"))
   {
      //fb_info_disable = 1
      //depthmode = 1
      smart_read = 1;
#ifdef HAVE_HWFBE
      hires = 1;
#endif
      //fb_clear = 1
      read_alpha = 1;
   }
   else if (strstr(name, (const char *)"MACE"))
   {
      settings.swapmode_retro = true;
#if 1
      // Not in original INI - fixes black stripes on big textures
      // TODO: check for regressions
      settings.increase_texrect_edge = 1;
#endif
      //fix_tex_coord = 8
      settings.filtering = 1;
      //depthmode = 1
   }
   else if (strstr(name, (const char *)"Quake"))
   {
      settings.force_microcheck = 1;
      settings.buff_clear = 0;
      settings.swapmode = 2;
   }
   else if (strstr(name, (const char *)"QUAKE II"))
   {
      smart_read = 1;
#ifdef HAVE_HWFBE
      hires = 1;
#endif
   }
   else if (strstr(name, (const char *)"Holy Magic Century"))
   {
      settings.filtering = 2;
      //depthmode = 1
   }
   else if (strstr(name, (const char *)"Quest 64"))
   {
      //depthmode = 1
   }
   else if (strstr(name, (const char*)"Silicon Valley"))
   {
      settings.filtering = 1;
      //depthmode = 0
   }
   else if (strstr(name, (const char*)"SNOWBOARD KIDS2")
         || strstr(name, (const char*)"Snobow Kids 2"))
   {
      settings.swapmode = 0;
      settings.filtering = 1;
   }
   else if (strstr(name, (const char*)"South Park Chef's Lu")
         || strstr(name, (const char*)"South Park: Chef's L"))
   {
      //fix_tex_coord = 4
      settings.filtering = 1;
      //depthmode = 1
      settings.fog = 0;
      settings.buff_clear = 0;
   }
   else if (strstr(name, (const char*)"LAMBORGHINI"))
   {
   }
   else if (strstr(name, (const char*)"MAGICAL TETRIS"))
   {
      settings.force_microcheck = 1;
      //depthmode = 1
      settings.fog = 0;
   }
   else if (strstr(name, (const char *)"MarioParty"))
   {
      settings.clip_zmin = 1;
      //depthmode = 0
      settings.swapmode = 2;
   }
   else if (strstr(name, (const char*)"MarioParty2"))
   {
      //depthmode = 0
      settings.swapmode = 2;
   }
   else if (strstr(name, (const char *)"Mega Man 64")
         || strstr(name, (const char *)"RockMan Dash"))
   {
      settings.increase_texrect_edge = 1;
      //depthmode = 1
      smart_read = 1;
#ifdef HAVE_HWFBE
      hires = 1;
#endif
   }
   else if (strstr(name, (const char *)"TUROK_DINOSAUR_HUNTE"))
   {
      settings.swapmode_retro = true;
      settings.depth_bias = 1;
      settings.lodmode = 1;
   }
   else if (strstr(name, (const char *)"Turok 2"))
   {
      settings.swapmode_retro = true;
   }
   else if (strstr(name, (const char *)"SUPER MARIO 64")
         || strstr(name, (const char *)"SUPERMARIO64"))
   {
      settings.swapmode_retro = true;
      settings.depth_bias = 64;
      settings.lodmode = 1;
      settings.filtering = 1;
      //depthmode = 1
   }
   else if (strstr(name, (const char *)"SM64 Star Road"))
   {
      settings.depth_bias = 1;
      settings.lodmode = 1;
      settings.filtering = 1;
      //depthmode = 1
   }
   else if (strstr(name, (const char *)"SUPERMAN"))
   {
      cpu_write_hack = 1;
   }
   else if (strstr(name, (const char *)"TETRISPHERE"))
   {
      settings.alt_tex_size = 1;
      settings.increase_texrect_edge = 1;
      //depthmode = 1
      smart_read = 1;
#ifdef HAVE_HWFBE
      hires = 1;
#endif
      fb_crc_mode = 2;
   }
   else if (strstr(name, (const char *)"MARIOKART64"))
   {
      settings.swapmode_retro = true;
      settings.depth_bias = 30;
      settings.stipple_mode = 1;
      settings.stipple_pattern = (uint32_t)4286595040UL;
      //depthmode = 1
#ifndef HAVE_HWFBE
      read_always = 1;
#endif
   }
   else if (strstr(name, (const char *)"YOSHI STORY"))
   {
      settings.swapmode_retro = true;
      //fix_tex_coord = 32
      //depthmode = 1
      settings.filtering = 1;
      settings.fog = 0;
   }
   else if (strstr(name, (const char *)"STARFOX64"))
   {
      settings.swapmode_retro = true;
   }
   else
   {
      if (strstr(name, (const char*)"Mini Racers") ||
            uc_crc == UCODE_MINI_RACERS_CRC1 || uc_crc == UCODE_MINI_RACERS_CRC2)
      {
         /* Mini Racers (prototype ROM ) does not have a valid name, so detect by ucode crc */
         settings.force_microcheck = 1;
         settings.buff_clear = 0;
         smart_read = 1;
#ifdef HAVE_HWFBE
         hires = 1;
#endif
         settings.swapmode = 0;
      }
   }

   //detect games which require special hacks
   if (strstr(name, (const char *)"ZELDA") || strstr(name, (const char *)"MASK"))
   {
      settings.hacks |= hack_Zelda;
      settings.flame_corona = 1;
      //settings.flame_corona = (settings.hacks & hack_Zelda) && !fb_depth_render_enabled;
   }
   else if (strstr(name, (const char *)"ROADSTERS TROPHY"))
      settings.hacks |= hack_Zelda;
   else if (strstr(name, (const char *)"Diddy Kong Racing"))
   {
      settings.swapmode_retro = true;
      settings.hacks |= hack_Diddy;
   }
   else if (strstr(name, (const char *)"Tonic Trouble"))
      settings.hacks |= hack_Tonic;
   else if (strstr(name, (const char *)"All") && strstr(name, (const char *)"Star") && strstr(name, (const char *)"Baseball"))
      settings.hacks |= hack_ASB;
   else if (strstr(name, (const char *)"Beetle") || strstr(name, (const char *)"BEETLE") || strstr(name, (const char *)"HSV"))
      settings.hacks |= hack_BAR;
   else if (strstr(name, (const char *)"I S S 64") || strstr(name, (const char *)"J WORLD SOCCER3") || strstr(name, (const char *)"PERFECT STRIKER") || strstr(name, (const char *)"RONALDINHO SOCCER"))
      settings.hacks |= hack_ISS64;
   else if (strstr(name, (const char *)"MARIOKART64"))
      settings.hacks |= hack_MK64;
   else if (strstr(name, (const char *)"NITRO64"))
      settings.hacks |= hack_WCWnitro;
   else if (strstr(name, (const char *)"CHOPPER_ATTACK") || strstr(name, (const char *)"WILD CHOPPERS"))
      settings.hacks |= hack_Chopper;
   else if (strstr(name, (const char *)"Resident Evil II") || strstr(name, (const char *)"BioHazard II"))
      settings.hacks |= hack_RE2;
   else if (strstr(name, (const char *)"YOSHI STORY"))
      settings.hacks |= hack_Yoshi;
   else if (strstr(name, (const char *)"F-Zero X") || strstr(name, (const char *)"F-ZERO X"))
      settings.hacks |= hack_Fzero;
   else if (strstr(name, (const char *)"PAPER MARIO") || strstr(name, (const char *)"MARIO STORY"))
      settings.hacks |= hack_PMario;
   else if (strstr(name, (const char *)"TOP GEAR RALLY 2"))
      settings.hacks |= hack_TGR2;
   else if (strstr(name, (const char *)"TOP GEAR RALLY"))
      settings.hacks |= hack_TGR;
   else if (strstr(name, (const char *)"Top Gear Hyper Bike"))
      settings.hacks |= hack_Hyperbike;
   else if (strstr(name, (const char *)"Killer Instinct Gold") || strstr(name, (const char *)"KILLER INSTINCT GOLD"))
      settings.hacks |= hack_KI;
   else if (strstr(name, (const char *)"Knockout Kings 2000"))
      settings.hacks |= hack_Knockout;
   else if (strstr(name, (const char *)"LEGORacers"))
      settings.hacks |= hack_Lego;
   else if (strstr(name, (const char *)"OgreBattle64"))
      settings.hacks |= hack_Ogre64;
   else if (strstr(name, (const char *)"Pilot Wings64"))
      settings.hacks |= hack_Pilotwings;
   else if (strstr(name, (const char *)"Supercross"))
      settings.hacks |= hack_Supercross;
   else if (strstr(name, (const char *)"STARCRAFT 64"))
      settings.hacks |= hack_Starcraft;
   else if (strstr(name, (const char *)"BANJO KAZOOIE 2") || strstr(name, (const char *)"BANJO TOOIE"))
      settings.hacks |= hack_Banjo2;
   else if (strstr(name, (const char *)"FIFA: RTWC 98") || strstr(name, (const char *)"RoadToWorldCup98"))
      settings.hacks |= hack_Fifa98;
   else if (strstr(name, (const char *)"Mega Man 64") || strstr(name, (const char *)"RockMan Dash"))
      settings.hacks |= hack_Megaman;
   else if (strstr(name, (const char *)"MISCHIEF MAKERS") || strstr(name, (const char *)"TROUBLE MAKERS"))
      settings.hacks |= hack_Makers;
   else if (strstr(name, (const char *)"GOLDENEYE"))
      settings.hacks |= hack_GoldenEye;
   else if (strstr(name, (const char *)"Blast Corps") || strstr(name, (const char *)"Blastdozer"))
      settings.hacks |= hack_Blastcorps;
   else if (strstr(name, (const char *)"PUZZLE LEAGUE"))
      settings.hacks |= hack_PPL;
   else if (strstr(name, (const char *)"WIN BACK") || strstr(name, (const char *)"OPERATION WINBACK"))
      settings.hacks |= hack_Winback;

   switch (gfx_plugin_accuracy)
   {
      case 2: /* HIGH */

         break;
      case 1: /* MEDIUM */
         if (read_always > 0) // turn off read_always
         {
            read_always = 0;
            if (smart_read == 0) // turn on smart_read instead
               smart_read = 1;
         }
         break;
      case 0: /* LOW */
         if (read_always > 0)
            read_always = 0;
         if (smart_read > 0)
            smart_read = 0;
         break; 
   }

   if (settings.n64_z_scale)
      ZLUT_init();

   //frame buffer

   if (optimize_texrect > 0)
      settings.frame_buffer |= fb_optimize_texrect;
   else if (optimize_texrect == 0)
      settings.frame_buffer &= ~fb_optimize_texrect;

   if (ignore_aux_copy > 0)
      settings.frame_buffer |= fb_ignore_aux_copy;
   else if (ignore_aux_copy == 0)
      settings.frame_buffer &= ~fb_ignore_aux_copy;

   if (hires_buf_clear > 0)
      settings.frame_buffer |= fb_hwfbe_buf_clear;
   else if (hires_buf_clear == 0)
      settings.frame_buffer &= ~fb_hwfbe_buf_clear;

   if (read_alpha > 0)
      settings.frame_buffer |= fb_read_alpha;
   else if (read_alpha == 0)
      settings.frame_buffer &= ~fb_read_alpha;
   if (useless_is_useless > 0)
      settings.frame_buffer |= fb_useless_is_useless;
   else
      settings.frame_buffer &= ~fb_useless_is_useless;

   if (fb_crc_mode >= 0)
      settings.fb_crc_mode = fb_crc_mode;

   if (smart_read > 0)
      settings.frame_buffer |= fb_emulation;
   else if (smart_read == 0)
      settings.frame_buffer &= ~fb_emulation;

   if (hires > 0)
      settings.frame_buffer |= fb_hwfbe;
   else if (hires == 0)
      settings.frame_buffer &= ~fb_hwfbe;
   if (read_always > 0)
      settings.frame_buffer |= fb_ref;
   else if (read_always == 0)
      settings.frame_buffer &= ~fb_ref;

   if (read_back_to_screen == 1)
      settings.frame_buffer |= fb_read_back_to_screen;
   else if (read_back_to_screen == 2)
      settings.frame_buffer |= fb_read_back_to_screen2;
   else if (read_back_to_screen == 0)
      settings.frame_buffer &= ~(fb_read_back_to_screen|fb_read_back_to_screen2);

   if (cpu_write_hack > 0)
      settings.frame_buffer |= fb_cpu_write_hack;
   else if (cpu_write_hack == 0)
      settings.frame_buffer &= ~fb_cpu_write_hack;

   if (get_fbinfo > 0)
      settings.frame_buffer |= fb_get_info;
   else if (get_fbinfo == 0)
      settings.frame_buffer &= ~fb_get_info;

   if (depth_render > 0)
      settings.frame_buffer |= fb_depth_render;
   else if (depth_render == 0)
      settings.frame_buffer &= ~fb_depth_render;



   settings.frame_buffer |= fb_motionblur;


   var.key = "mupen64-filtering";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
	  if (strcmp(var.value, "N64 3-point") == 0)
#ifdef DISABLE_3POINT
		 glide_set_filtering(3);
#else
		 glide_set_filtering(1);
#endif
	  else if (strcmp(var.value, "nearest") == 0)
		 glide_set_filtering(2);
	  else if (strcmp(var.value, "bilinear") == 0)
		 glide_set_filtering(3);
   }

   /* this has to be done here. */
   if (strstr(name, "POKEMON STADIUM 2"))
      settings.frame_buffer &= ~fb_emulation;
}

