# Drivers, cpus and devices for neogeo subset
SOURCES_C += $(FBNEO_BURN_SND_DIR)/fm.c \
	$(M68K_CPU_DIR)/m68kcpu.c \
	$(M68K_CPU_DIR)/m68kops.c \
	$(FBNEO_BURN_SND_DIR)/ay8910.c \
	$(FBNEO_BURN_SND_DIR)/ymdeltat.c

SOURCES_CXX += $(FBNEO_CPU_DIR)/m68000_intf.cpp \
	$(FBNEO_CPU_DIR)/z80_intf.cpp \
	$(Z80_CPU_DIR)/z80.cpp \
	$(Z80_CPU_DIR)/z80ctc.cpp \
	$(Z80_CPU_DIR)/z80daisy.cpp \
	$(Z80_CPU_DIR)/z80pio.cpp \
	$(FBNEO_BURN_SND_DIR)/burn_ym2203.cpp \
	$(FBNEO_BURN_SND_DIR)/burn_ym2608.cpp \
	$(FBNEO_BURN_SND_DIR)/burn_ym2610.cpp \
	$(FBNEO_BURN_SND_DIR)/burn_ym2612.cpp \
	$(FBNEO_BURN_DEVICES_DIR)/joyprocess.cpp \
	$(FBNEO_BURN_DEVICES_DIR)/resnet.cpp \
	$(NEOGEO_DIR)/d_neogeo.cpp \
	$(NEOGEO_DIR)/neo_decrypt.cpp \
	$(NEOGEO_DIR)/neo_palette.cpp \
	$(NEOGEO_DIR)/neo_run.cpp \
	$(NEOGEO_DIR)/neo_sprite.cpp \
	$(NEOGEO_DIR)/neo_text.cpp \
	$(NEOGEO_DIR)/neo_upd4990a.cpp \
	$(NEOGEO_DIR)/neogeo.cpp

CFLAGS += -DSUBSET=\"$(SUBSET)\" -DNO_CONSOLES_COMPUTERS
CXXFLAGS += -DSUBSET=\"$(SUBSET)\" -DNO_CONSOLES_COMPUTERS
