# pre68k subset
# Boards using CPUs prior to m68k (pre-16bits ?)
# CPUs to ignore : m68k, sh2, sh3, arm, tms3XXXX, nec VXX
#
# TODO: finish adding all drivers

# Drivers, cpus and devices for pre68k subset
SOURCES_C += $(FBNEO_BURN_SND_DIR)/fm.c \
	$(FBNEO_BURN_SND_DIR)/fmopl.c \
	$(FBNEO_BURN_SND_DIR)/ay8910.c \
	$(FBNEO_BURN_SND_DIR)/ymdeltat.c \
	$(FBNEO_BURN_SND_DIR)/ym2151.c

SOURCES_CXX += $(FBNEO_CPU_DIR)/h6280_intf.cpp \
	$(FBNEO_CPU_DIR)/hd6309_intf.cpp \
	$(FBNEO_CPU_DIR)/m6502_intf.cpp \
	$(FBNEO_CPU_DIR)/m6800_intf.cpp \
	$(FBNEO_CPU_DIR)/m6809_intf.cpp \
	$(FBNEO_CPU_DIR)/s2650_intf.cpp \
	$(FBNEO_CPU_DIR)/z80_intf.cpp \
	$(Z80_CPU_DIR)/z80.cpp \
	$(Z80_CPU_DIR)/z80ctc.cpp \
	$(Z80_CPU_DIR)/z80daisy.cpp \
	$(Z80_CPU_DIR)/z80pio.cpp \
	$(M6502_CPU_DIR)/m6502.cpp \
	$(M6800_CPU_DIR)/m6800.cpp \
	$(M6809_CPU_DIR)/m6809.cpp \
	$(F8_CPU_DIR)/f8.cpp \
	$(H6280_CPU_DIR)/h6280.cpp \
	$(HD6309_CPU_DIR)/hd6309.cpp \
	$(S2650_CPU_DIR)/s2650.cpp \
	$(I8X41_CPU_DIR)/mcs48.cpp \
	$(I8039_CPU_DIR)/i8039.cpp \
	$(I8051_CPU_DIR)/mcs51.cpp \
	$(FBNEO_BURN_DIR)/burn_bitmap.cpp \
	$(FBNEO_BURN_DIR)/burn_pal.cpp \
	$(FBNEO_BURN_DIR)/tilemap_generic.cpp \
	$(FBNEO_BURN_DIR)/tiles_generic.cpp \
	$(FBNEO_BURN_DEVICES_DIR)/earom.cpp \
	$(FBNEO_BURN_DEVICES_DIR)/joyprocess.cpp \
	$(FBNEO_BURN_DEVICES_DIR)/8255ppi.cpp \
	$(FBNEO_BURN_DEVICES_DIR)/i2ceeprom.cpp \
	$(FBNEO_BURN_DEVICES_DIR)/resnet.cpp \
	$(FBNEO_BURN_DEVICES_DIR)/tms9928a.cpp \
	$(FBNEO_BURN_DEVICES_DIR)/watchdog.cpp \
	$(FBNEO_BURN_SND_DIR)/burn_ym2151.cpp \
	$(FBNEO_BURN_SND_DIR)/burn_ym2203.cpp \
	$(FBNEO_BURN_SND_DIR)/burn_ym2608.cpp \
	$(FBNEO_BURN_SND_DIR)/burn_ym2610.cpp \
	$(FBNEO_BURN_SND_DIR)/burn_ym2612.cpp \
	$(FBNEO_BURN_SND_DIR)/burn_ym3526.cpp \
	$(FBNEO_BURN_SND_DIR)/burn_ym3812.cpp \
	$(FBNEO_BURN_SND_DIR)/dac.cpp \
	$(FBNEO_BURN_SND_DIR)/digitalk.cpp \
	$(FBNEO_BURN_SND_DIR)/flt_rc.cpp \
	$(FBNEO_BURN_SND_DIR)/msm5205.cpp \
	$(FBNEO_BURN_SND_DIR)/msm6295.cpp \
	$(FBNEO_BURN_SND_DIR)/pokey.cpp \
	$(FBNEO_BURN_SND_DIR)/samples.cpp \
	$(FBNEO_BURN_SND_DIR)/sn76496.cpp \
	$(ATARI_DIR)/d_akkaarrh.cpp \
	$(ATARI_DIR)/d_missile.cpp \
	$(CHANNELF_DIR)/d_channelf.cpp \
	$(COLECO_DIR)/d_coleco.cpp \
	$(DATAEAST_DIR)/d_actfancr.cpp \
	$(DATAEAST_DIR)/d_brkthru.cpp \
	$(DATAEAST_DIR)/d_bwing.cpp \
	$(DATAEAST_DIR)/d_chanbara.cpp \
	$(DATAEAST_DIR)/d_compgolf.cpp \
	$(DATAEAST_DIR)/d_dec8.cpp \
	$(DATAEAST_DIR)/d_decocass.cpp \
	$(DATAEAST_DIR)/d_kchamp.cpp \
	$(DATAEAST_DIR)/d_liberate.cpp \
	$(DATAEAST_DIR)/d_metlclsh.cpp \
	$(DATAEAST_DIR)/d_pcktgal.cpp \
	$(DATAEAST_DIR)/decobac06.cpp \
	$(DATAEAST_DIR)/d_progolf.cpp \
	$(DATAEAST_DIR)/d_shootout.cpp \
	$(DATAEAST_DIR)/d_sidepckt.cpp \
	$(GALAXIAN_DIR)/d_galaxian.cpp \
	$(GALAXIAN_DIR)/gal_gfx.cpp \
	$(GALAXIAN_DIR)/gal_run.cpp \
	$(GALAXIAN_DIR)/gal_sound.cpp \
	$(GALAXIAN_DIR)/gal_stars.cpp \
	$(IREM_DIR)/d_m52.cpp \
	$(IREM_DIR)/d_m57.cpp \
	$(IREM_DIR)/d_m58.cpp \
	$(IREM_DIR)/d_m62.cpp \
	$(IREM_DIR)/d_m63.cpp \
	$(IREM_DIR)/d_vigilant.cpp \
	$(IREM_DIR)/irem_sound.cpp

CFLAGS += -DSUBSET=\"$(SUBSET)\" -DNO_NEOGEO
CXXFLAGS += -DSUBSET=\"$(SUBSET)\" -DNO_NEOGEO
