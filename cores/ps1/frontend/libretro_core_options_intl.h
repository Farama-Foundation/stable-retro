#ifndef LIBRETRO_CORE_OPTIONS_INTL_H__
#define LIBRETRO_CORE_OPTIONS_INTL_H__

#if defined(_MSC_VER) && (_MSC_VER >= 1500 && _MSC_VER < 1900)
/* https://support.microsoft.com/en-us/kb/980263 */
#pragma execution_character_set("utf-8")
#pragma warning(disable:4566)
#endif

#include <libretro.h>

/*
 ********************************
 * VERSION: 2.0
 ********************************
 *
 * - 2.0: Add support for core options v2 interface
 * - 1.3: Move translations to libretro_core_options_intl.h
 *        - libretro_core_options_intl.h includes BOM and utf-8
 *          fix for MSVC 2010-2013
 *        - Added HAVE_NO_LANGEXTRA flag to disable translations
 *          on platforms/compilers without BOM support
 * - 1.2: Use core options v1 interface when
 *        RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION is >= 1
 *        (previously required RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION == 1)
 * - 1.1: Support generation of core options v0 retro_core_option_value
 *        arrays containing options with a single value
 * - 1.0: First commit
*/

#ifdef __cplusplus
extern "C" {
#endif

/*
 ********************************
 * Core Option Definitions
 ********************************
*/

/* RETRO_LANGUAGE_JAPANESE */

/* RETRO_LANGUAGE_FRENCH */

/* RETRO_LANGUAGE_SPANISH */

/* RETRO_LANGUAGE_GERMAN */

/* RETRO_LANGUAGE_ITALIAN */

/* RETRO_LANGUAGE_DUTCH */

/* RETRO_LANGUAGE_PORTUGUESE_BRAZIL */

/* RETRO_LANGUAGE_PORTUGUESE_PORTUGAL */

/* RETRO_LANGUAGE_RUSSIAN */

/* RETRO_LANGUAGE_KOREAN */

/* RETRO_LANGUAGE_CHINESE_TRADITIONAL */

/* RETRO_LANGUAGE_CHINESE_SIMPLIFIED */

/* RETRO_LANGUAGE_ESPERANTO */

/* RETRO_LANGUAGE_POLISH */

/* RETRO_LANGUAGE_VIETNAMESE */

/* RETRO_LANGUAGE_ARABIC */

/* RETRO_LANGUAGE_GREEK */

/* RETRO_LANGUAGE_TURKISH */

struct retro_core_option_v2_category option_cats_tr[] = {
   { NULL, NULL, NULL },
};

struct retro_core_option_v2_definition option_defs_tr[] = {
   {
      "pcsx_rearmed_frameskip",
      "Kare Atlama",
      NULL,
      "Görsel pürüzsüzlük pahasına performansı artırmak için ne kadar karenin atlanması gerektiğini seçin.",
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "pcsx_rearmed_bios",
      "BIOS Kullan",
      NULL,
      "Gerçek bios dosyasını (varsa) veya öykünmüş bios'u (HLE) kullanmanızı sağlar. Daha iyi uyumluluk için resmi bios dosyasını kullanmanız önerilir.",
      NULL,
      NULL,
      {
         { "auto", "otomatik" },
         { NULL, NULL },
      },
      "auto",
   },
   {
      "pcsx_rearmed_region",
      "Bölge",
      NULL,
      "Sistemin hangi bölgeden olduğunu seçin. NTSC için 60 Hz, PAL için 50 Hz.",
      NULL,
      NULL,
      {
         { "auto", "otomatik" },
         { NULL, NULL },
      },
      "auto",
   },
   {
      "pcsx_rearmed_memcard2",
      "İkinci Bellek Kartını Etkinleştir (Paylaşılan)",
      NULL,
      "2. Hafıza kartı yuvasını etkinleştirin. Bu hafıza kartı tüm oyunlar arasında paylaşılır.",
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "pcsx_rearmed_multitap1",
      "Multitap 1",
      NULL,
      "Bağlantı noktası 1'deki multitap'ı etkinleştirir / devre dışı bırakır ve izin veren oyunlarda 5 oyuncuya kadar izin verir.",
      NULL,
      NULL,
      {
         { "auto", "otomatik" },
         { NULL, NULL },
      },
      "auto",
   },
   {
      "pcsx_rearmed_multitap2",
      "Multitap 2",
      NULL,
      "Bağlantı noktası 2'deki multitap'ı etkinleştirir/devre dışı bırakır ve izin veren oyunlarda 8 oyuncuya kadar izin verir. Bunun çalışması için Multitap 1'in etkinleştirilmesi gerekir.",
      NULL,
      NULL,
      {
         { "auto", "otomatik" },
         { NULL, NULL },
      },
      "auto",
   },
   {
      "pcsx_rearmed_negcon_deadzone",
      "NegCon Twist Deadzone (Yüzdelik)",
      NULL,
      "Öykünülmüş neGcon kontrolörünün 'büküm' eylemini simüle ederken RetroPad sol analog çubuğunun ölü bölgesini ayarlar. Sürüklenme/istenmeyen girişi ortadan kaldırmak için kullanılır.",
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "pcsx_rearmed_negcon_response",
      "NegCon Twist Response",
      NULL,
      "Öykünülmüş neGcon kontrolörünün 'bükümünü' simule etmek için bir RetroPad sol analog çubuğu kullanırken analog cevabını belirtir.",
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "pcsx_rearmed_vibration",
      "Titreşimi Etkinleştir",
      NULL,
      "Titreşim özelliklerini destekleyen kontrolörler için titreşim geri bildirimini etkinleştirir.",
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "pcsx_rearmed_dithering",
      "Dithering Etkinleştir",
      NULL,
      "Kapalı ise, PSX'in renk bantlarıyla mücadele etmek için uyguladığı renk taklidi düzenini devre dışı bırakır.",
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },

#ifdef NEW_DYNAREC
   {
      "pcsx_rearmed_drc",
      "Dinamik Yeniden Derleyici",
      NULL,
      "Çekirdeğin dinamik yeniden derleyici veya tercüman(daha yavaş) CPU talimatlarını kullanmasını sağlar.",
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "pcsx_rearmed_psxclock",
      "PSX CPU Saat Hızı",
      NULL,
      "Overclock or under-clock the PSX CPU. Try adjusting this if the game is too slow, too fast or hangs."
#if defined(HAVE_PRE_ARMV7) && !defined(_3DS)
      " Default is 50."
#else
      " Default is 57."
#endif
      ,
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },
#endif /* NEW_DYNAREC */

#ifdef GPU_NEON
   {
      "pcsx_rearmed_neon_interlace_enable_v2",
      "Interlacing Mode'u etkinleştir",
      NULL,
      "Sahte tarama çizgileri efektini etkinleştirir.",
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "pcsx_rearmed_neon_enhancement_enable",
      "Geliştirilmiş Çözünürlük",
      NULL,
      "Düşük performans pahasına çift çözünürlükte işler.",
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "pcsx_rearmed_neon_enhancement_no_main",
      "Geliştirilmiş Çözünürlük (Speed Hack)",
      NULL,
      "Geliştirilmiş çözünürlük seçeneği için hız aşırtma(bazı oyunlarda sorun çıkartabilir).",
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },
#endif /* GPU_NEON */

   {
      "pcsx_rearmed_display_internal_fps",
      "Dahili FPS'yi görüntüle",
      NULL,
      "Etkinleştirildiğinde ekranda saniye başına kareyi gösterir.",
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },

   /* GPU PEOPS OPTIONS */
#ifdef GPU_PEOPS
   {
      "pcsx_rearmed_show_gpu_peops_settings",
      "Gelişmiş GPU Ayarlarını Göster",
      NULL,
      "Çeşitli GPU düzeltmelerini etkinleştirin veya devre dışı bırakın. Ayarların etkili olması için core'un yeniden başlatılması gerekebilir. NOT: Bu ayarın etkili olabilmesi için Hızlı Menü’nün değiştirilmesi gerekir.",
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "pcsx_rearmed_gpu_peops_odd_even_bit",
      "(GPU) Odd/Even Bit Hack",
      NULL,
      "Chrono Cross için gerekli.",
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "pcsx_rearmed_gpu_peops_expand_screen_width",
      "(GPU) Ekran Genişliğini Genişlet",
      NULL,
      "Capcom dövüş oyunları",
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "pcsx_rearmed_gpu_peops_ignore_brightness",
      "(GPU) Parlaklık Rengini Yoksay",
      NULL,
      "Lunar Silver Star Story oyunlarında siyah ekran",
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "pcsx_rearmed_gpu_peops_disable_coord_check",
      "(GPU) Koordinat Kontrolünü Devre Dışı Bırak",
      NULL,
      "Uyumluluk modu",
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "pcsx_rearmed_gpu_peops_lazy_screen_update",
      "(GPU) Tembel Ekran Güncellemesi",
      NULL,
      "Pandemonium 2",
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "pcsx_rearmed_gpu_peops_old_frame_skip",
      "(GPU) Eski Çerçeve Atlama",
      NULL,
      "Her ikinci kareyi atla",
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "pcsx_rearmed_gpu_peops_repeated_triangles",
      "(GPU) Tekrarlanan Düz Doku Üçgenleri",
      NULL,
      "Star Wars: Dark Forces için gerekli",
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "pcsx_rearmed_gpu_peops_quads_with_triangles",
      "(GPU) Üçgenler ile Dörtlü Çiz",
      NULL,
      "Daha iyi g renkler, daha kötü dokular",
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "pcsx_rearmed_gpu_peops_fake_busy_state",
      "(GPU) Sahte 'Gpu Meşgul' Konumları",
      NULL,
      "Çizimden sonra meşgul bayraklarını değiştir",
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },
#endif /* GPU_PEOPS */

   {
      "pcsx_rearmed_show_bios_bootlogo",
      "Bios Bootlogo'yu Göster",
      NULL,
      "Etkinleştirildiğinde, başlatırken veya sıfırlarken PlayStation logosunu gösterir. (Bazı oyunları bozabilir).",
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "pcsx_rearmed_spu_reverb",
      "Ses Yankısı",
      NULL,
      "Ses yankı efektini etkinleştirir veya devre dışı bırakır.",
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "pcsx_rearmed_spu_interpolation",
      "Ses Enterpolasyonu",
      NULL,
      NULL,
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "pcsx_rearmed_icache_emulation",
      "ICache Düzeltmleri",
      NULL,
      NULL,
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },

   /* ADVANCED OPTIONS */
   {
      "pcsx_rearmed_noxadecoding",
      "XA Kod Çözme",
      NULL,
      NULL,
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "pcsx_rearmed_nocdaudio",
      "CD Ses",
      NULL,
      NULL,
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },

#ifdef NEW_DYNAREC
   {
      "pcsx_rearmed_nosmccheck",
      "(Speed Hack) SMC Kontrollerini Devre Dışı Bırak",
      NULL,
      "Yükleme sırasında çökmelere neden olabilir, hafıza kartını bozabilir.",
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "pcsx_rearmed_gteregsunneeded",
      "(Speed Hack) GTE'nin Gereksiz Olduğunu Varsayın",
      NULL,
      "Grafiksel bozukluklara neden olabilir.",
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },
   {
      "pcsx_rearmed_nogteflags",
      "(Speed Hack) GTE Bayraklarını Devredışı Bırakın",
      NULL,
      "Grafiksel bozukluklara neden olur.",
      NULL,
      NULL,
      {
         { NULL, NULL },
      },
      NULL
   },
#endif /* NEW_DYNAREC */

   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};

struct retro_core_options_v2 options_tr = {
   option_cats_tr,
   option_defs_tr
};

#ifdef __cplusplus
}
#endif

#endif
