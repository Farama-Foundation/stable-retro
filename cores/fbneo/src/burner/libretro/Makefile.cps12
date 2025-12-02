# Drivers, cpus and devices for cps1+cps2 subset
SOURCES_C += $(FBNEO_BURN_SND_DIR)/fm.c \
	$(M68K_CPU_DIR)/m68kcpu.c \
	$(M68K_CPU_DIR)/m68kops.c \
	$(FBNEO_BURN_SND_DIR)/ay8910.c \
	$(FBNEO_BURN_SND_DIR)/ymdeltat.c \
	$(FBNEO_BURN_SND_DIR)/ym2151.c

SOURCES_CXX += $(FBNEO_CPU_DIR)/m68000_intf.cpp \
	$(FBNEO_CPU_DIR)/z80_intf.cpp \
	$(Z80_CPU_DIR)/z80.cpp \
	$(Z80_CPU_DIR)/z80ctc.cpp \
	$(Z80_CPU_DIR)/z80daisy.cpp \
	$(Z80_CPU_DIR)/z80pio.cpp \
	$(FBNEO_BURN_SND_DIR)/burn_ym2151.cpp \
	$(FBNEO_BURN_SND_DIR)/burn_ym2203.cpp \
	$(FBNEO_BURN_SND_DIR)/burn_ym2608.cpp \
	$(FBNEO_BURN_SND_DIR)/burn_ym2610.cpp \
	$(FBNEO_BURN_SND_DIR)/burn_ym2612.cpp \
	$(FBNEO_BURN_SND_DIR)/msm5205.cpp \
	$(FBNEO_BURN_SND_DIR)/msm6295.cpp \
	$(FBNEO_BURN_SND_DIR)/samples.cpp \
	$(FBNEO_BURN_DEVICES_DIR)/eeprom.cpp \
	$(FBNEO_BURN_DEVICES_DIR)/joyprocess.cpp \
	$(FBNEO_BURN_DEVICES_DIR)/timekpr.cpp \
	$(CAPCOM_DIR)/cps.cpp \
	$(CAPCOM_DIR)/cps_config.cpp \
	$(CAPCOM_DIR)/cps_draw.cpp \
	$(CAPCOM_DIR)/cps_mem.cpp \
	$(CAPCOM_DIR)/cps_obj.cpp \
	$(CAPCOM_DIR)/cps_pal.cpp \
	$(CAPCOM_DIR)/cps_run.cpp \
	$(CAPCOM_DIR)/cps_rw.cpp \
	$(CAPCOM_DIR)/cps_scr.cpp \
	$(CAPCOM_DIR)/cps2_crpt.cpp \
	$(CAPCOM_DIR)/cpsr.cpp \
	$(CAPCOM_DIR)/cpsrd.cpp \
	$(CAPCOM_DIR)/cpst.cpp \
	$(CAPCOM_DIR)/ctv.cpp \
	$(CAPCOM_DIR)/d_cps1.cpp \
	$(CAPCOM_DIR)/d_cps2.cpp \
	$(CAPCOM_DIR)/fcrash_snd.cpp \
	$(CAPCOM_DIR)/kabuki.cpp \
	$(CAPCOM_DIR)/ps.cpp \
	$(CAPCOM_DIR)/ps_m.cpp \
	$(CAPCOM_DIR)/ps_z.cpp \
	$(CAPCOM_DIR)/qs.cpp \
	$(CAPCOM_DIR)/qs_c.cpp \
	$(CAPCOM_DIR)/qs_z.cpp \
	$(CAPCOM_DIR)/sf2mdt_snd.cpp

CFLAGS += -DSUBSET=\"$(SUBSET)\" -DNO_NEOGEO -DNO_CONSOLES_COMPUTERS
CXXFLAGS += -DSUBSET=\"$(SUBSET)\" -DNO_NEOGEO -DNO_CONSOLES_COMPUTERS
