/*********************************************************************

	OST sample support

*********************************************************************/

enum
{
  OST_SUPPORT_DISABLED = 0,
  OST_SUPPORT_CONTRA,
  OST_SUPPORT_DDRAGON,
  OST_SUPPORT_FFIGHT,
  OST_SUPPORT_IKARI,
  OST_SUPPORT_MK,
  OST_SUPPORT_MOONWALKER,
  OST_SUPPORT_NBA_JAM,
  OST_SUPPORT_OUTRUN,
  OST_SUPPORT_ROBOCOP,
  OST_SUPPORT_SF1,
  OST_SUPPORT_SF2
};


extern bool ost_support_enabled (int ost);
extern bool (*generate_ost_sound) (int);

extern void install_ost_support (struct InternalMachineDriver *machine, int ost);

#define MDRV_INSTALL_OST_SUPPORT(ost)		\
	install_ost_support(machine, ost);		\


extern void ost_fade_volume (void);
