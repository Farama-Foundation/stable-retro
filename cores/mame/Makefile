TARGET_NAME := mame2003_plus
CORE_DIR    := src
INCLUDE_DRV ?= all

DEBUG         ?= 0
DEBUGGER      ?= 0
SPLIT_UP_LINK ?= 0
ARM           ?= 0 # set to 0 or 1 to indicate ARM or not
CPU_ARCH      ?= 0 # as of November 2018 this flag doesn't seem to be used but is being set to either arm or arm64 for some platforms

LIBS          ?=

HIDE ?= @

ifneq ($(SANITIZER),)
	CFLAGS   := -fsanitize=$(SANITIZER) $(CFLAGS)
	CXXFLAGS := -fsanitize=$(SANITIZER) $(CXXFLAGS)
	LDFLAGS  := -fsanitize=$(SANITIZER) $(LDFLAGS)
endif

GIT_VERSION ?= " $(shell git rev-parse --short HEAD || echo unknown)"
ifneq ($(GIT_VERSION)," unknown")
	CFLAGS += -DGIT_VERSION=\"$(GIT_VERSION)\"
endif

SPACE       :=
SPACE       := $(SPACE) $(SPACE)
BACKSLASH   :=
BACKSLASH   := \$(BACKSLASH)
filter_out1  = $(filter-out $(firstword $1),$1)
filter_out2  = $(call filter_out1,$(call filter_out1,$1))
unixpath     = $(subst \,/,$1)
unixcygpath  = /$(subst :,,$(call unixpath,$1))

ifeq ($(platform),)
	system_platform = unix
	platform = unix
	ifeq ($(shell uname -a),)
		system_platform = win
		platform = win
	else ifneq ($(findstring MINGW,$(shell uname -a)),)
		system_platform = win
		platform = win
	else ifneq ($(findstring Darwin,$(shell uname -a)),)
		system_platform = osx
		platform = osx
	else ifneq ($(findstring win,$(shell uname -a)),)
		system_platform = win
		platform = win
	endif
endif

#Windows and wsl need to have their linking split up due to cmd length limits
ifneq ($(findstring Microsoft,$(shell uname -a)),)
	SPLIT_UP_LINK=1
endif

ifneq ($(findstring MINGW,$(shell uname -a)),)
	SPLIT_UP_LINK=1
endif

ifeq ($(system_platform), win)
	SPLIT_UP_LINK=1
endif


X86_ASM_68000 = # don't use x86 Assembler 68000 engine by default; set to 1 to enable
X86_ASM_68020 = # don't use x86 Assembler 68020 engine by default; set to 1 to enable
X86_MIPS3_DRC = # don't use x86 DRC MIPS3 engine by default;       set to 1 to enable

ifeq ($(ARCH),)
	# no architecture value passed make; try to determine host platform
	UNAME_P = $(shell uname -p)
	ifneq ($(findstring powerpc,$(UNAME_P)),)
		ARCH = ppc
	else ifneq ($(findstring x86_64,$(UNAME_P)),)
		# catch "x86_64" first to avoid 64-bit architecture being caught by our next search for "86"
		# no commands for x86_x64 only at this point
		# we could help compile an x86_64 dynarec here or something like that
	else ifneq ($(findstring 86,$(UNAME_P)),)
		ARCH = x86 # if "86" is found now it must be i386 or i686
	endif
endif

ifeq ($(ARCH), x86)
	X86_MIPS3_DRC = 1
endif


ifeq (,$(findstring msvc,$(platform)))
	LIBS += -lm
endif

ifneq (,$(findstring msvc,$(platform)))
	system_platform = win
endif

# Unix
ifeq ($(platform), unix)
	TARGET = $(TARGET_NAME)_libretro.so
	fpic = -fPIC
	CFLAGS += $(fpic)
	LDFLAGS += $(fpic) -shared -Wl,--version-script=link.T

# Linux Portable
else ifeq ($(platform), linux-portable)
	TARGET = $(TARGET_NAME)_libretro.so
	fpic = -fPIC -nostdlib
	CFLAGS += $(fpic)
	LIBS =
	LDFLAGS += $(fpic) -shared -Wl,--version-script=link.T

# MacOS X
else ifeq ($(platform), osx)
	TARGET = $(TARGET_NAME)_libretro.dylib
	PLATCFLAGS += -Wno-error=implicit-function-declaration
	fpic = -fPIC
	ifeq ($(ARCH),ppc)
		BIGENDIAN = 1
		PLATCFLAGS += -D__ppc__ -D__POWERPC__
	endif
	LDFLAGS += $(fpic) -dynamiclib
	OSXVER = `sw_vers -productVersion | cut -c 4`
	OSX_LT_MAVERICKS = `(( $(OSXVER) <= 9)) && echo "YES"`
	ifeq ($(OSX_LT_MAVERICKS), YES)
		fpic += -mmacosx-version-min=10.1
	endif
	ifeq ($(CROSS_COMPILE),1)
		TARGET_RULE   = -target $(LIBRETRO_APPLE_PLATFORM) -isysroot $(LIBRETRO_APPLE_ISYSROOT)
		CFLAGS   += $(TARGET_RULE)
		CPPFLAGS += $(TARGET_RULE)
		CXXFLAGS += $(TARGET_RULE)
		LDFLAGS  += $(TARGET_RULE)
	endif

# iOS
else ifneq (,$(findstring ios,$(platform)))
	TARGET = $(TARGET_NAME)_libretro_ios.dylib
	fpic = -fPIC
	LDFLAGS += $(fpic) -dynamiclib
	PLATCFLAGS += -D__IOS__ -Wcast-align -Wall -Wno-error=implicit-function-declaration
	MINVERSION :=
	ifeq ($(IOSSDK),)
		IOSSDK := $(shell xcodebuild -version -sdk iphoneos Path)
	endif
	ifeq ($(platform),ios-arm64)
		CC = cc -arch arm64 -isysroot $(IOSSDK)
		LD = cc -arch arm64 -isysroot $(IOSSDK)
	else
		CC = cc -arch armv7 -isysroot $(IOSSDK)
		LD = cc -arch armv7 -isysroot $(IOSSDK)
	endif
	ifeq ($(platform),$(filter $(platform),ios9 ios-arm64))
		MINVERSION = -miphoneos-version-min=8.0
	else
		MINVERSION = -miphoneos-version-min=5.0
	endif
	CFLAGS += $(MINVERSION)
	LDFLAGS += $(MINVERSION)

# tvOS
else ifeq ($(platform), tvos-arm64)
	TARGET = $(TARGET_NAME)_libretro_tvos.dylib
	fpic = -fPIC
	LDFLAGS += $(fpic) -dynamiclib
	PLATCFLAGS += -D__IOS__ -Wcast-align -Wall -Wno-error=implicit-function-declaration
	ifeq ($(IOSSDK),)
		IOSSDK := $(shell xcodebuild -version -sdk appletvos Path)
	endif
	CC = cc -arch arm64 -isysroot $(IOSSDK)

# Raspberry Pi 0
else ifeq ($(platform), rpi0)
	TARGET = $(TARGET_NAME)_libretro.so
	fpic = -fPIC
	CFLAGS += $(fpic)
	LDFLAGS += $(fpic) -shared -Wl,--version-script=link.T
	PLATCFLAGS += -marm -mcpu=arm1176jzf-s -mfpu=vfp -mfloat-abi=hard
	PLATCFLAGS += -fomit-frame-pointer -ffast-math
	CXXFLAGS = $(CFLAGS) -fno-rtti -fno-exceptions
	CPU_ARCH := arm
	ARM = 1

# Raspberry Pi 1
else ifeq ($(platform), rpi1)
	TARGET = $(TARGET_NAME)_libretro.so
	fpic = -fPIC
	CFLAGS += $(fpic)
	LDFLAGS += $(fpic) -shared -Wl,--version-script=link.T
	PLATCFLAGS += -marm -mcpu=arm1176jzf-s -mfpu=vfp -mfloat-abi=hard
	PLATCFLAGS += -fomit-frame-pointer -ffast-math
	CXXFLAGS = $(CFLAGS) -fno-rtti -fno-exceptions
	CPU_ARCH := arm
	ARM = 1

# Raspberry Pi 2
else ifeq ($(platform), rpi2)
	TARGET = $(TARGET_NAME)_libretro.so
	fpic = -fPIC
	CFLAGS += $(fpic)
	LDFLAGS += $(fpic) -shared -Wl,--version-script=link.T
	PLATCFLAGS += -marm -mcpu=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard
	PLATCFLAGS += -fomit-frame-pointer -ffast-math
	CXXFLAGS = $(CFLAGS) -fno-rtti -fno-exceptions
	CPU_ARCH := arm
	ARM = 1

# Raspberry Pi 3
else ifeq ($(platform), rpi3)
	TARGET = $(TARGET_NAME)_libretro.so
	fpic = -fPIC
	CFLAGS += $(fpic)
	LDFLAGS += $(fpic) -shared -Wl,--version-script=link.T
	PLATCFLAGS += -marm -mcpu=cortex-a53 -mfpu=neon-fp-armv8 -mfloat-abi=hard
	PLATCFLAGS += -fomit-frame-pointer -ffast-math
	CXXFLAGS = $(CFLAGS) -fno-rtti -fno-exceptions
	CPU_ARCH := arm
	ARM = 1

# Raspberry Pi 3 (AArch64)
else ifeq ($(platform), rpi3_64)
	TARGET = $(TARGET_NAME)_libretro.so
	fpic = -fPIC
	CFLAGS += $(fpic)
	LDFLAGS += $(fpic) -shared -Wl,--version-script=link.T
	PLATCFLAGS += -mcpu=cortex-a53 -mtune=cortex-a53
	PLATCFLAGS += -fomit-frame-pointer -ffast-math
	CXXFLAGS = $(CFLAGS) -fno-rtti -fno-exceptions
	CPU_ARCH := arm64

# Raspberry Pi 4
else ifeq ($(platform), rpi4)
	TARGET = $(TARGET_NAME)_libretro.so
	fpic = -fPIC
	CFLAGS += $(fpic)
	LDFLAGS += $(fpic) -shared -Wl,--version-script=link.T
	PLATCFLAGS += -marm -mcpu=cortex-a72 -mfpu=neon-fp-armv8 -mfloat-abi=hard
	PLATCFLAGS += -fomit-frame-pointer -ffast-math
	CXXFLAGS = $(CFLAGS) -fno-rtti -fno-exceptions
	CPU_ARCH := arm
	ARM = 1

# Raspberry Pi 4 (AArch64)
else ifeq ($(platform), rpi4_64)
	TARGET = $(TARGET_NAME)_libretro.so
	fpic = -fPIC
	CFLAGS += $(fpic)
	LDFLAGS += $(fpic) -shared -Wl,--version-script=link.T
	PLATCFLAGS += -mcpu=cortex-a72 -mtune=cortex-a72
	PLATCFLAGS += -fomit-frame-pointer -ffast-math
	CXXFLAGS = $(CFLAGS) -fno-rtti -fno-exceptions
	CPU_ARCH := arm64

# Classic Platforms - NESC, SNESC, C64 mini
else ifeq ($(platform), classic_armv7_a7)
	TARGET := $(TARGET_NAME)_libretro.so
	fpic := -fPIC
	LDFLAGS += $(fpic) -shared -Wl,--version-script=link.T
	CFLAGS += -Ofast \
	-flto=4 -fwhole-program -fuse-linker-plugin \
	-fdata-sections -ffunction-sections -Wl,--gc-sections \
	-fno-stack-protector -fno-ident -fomit-frame-pointer \
	-falign-functions=1 -falign-jumps=1 -falign-loops=1 \
	-fno-unwind-tables -fno-asynchronous-unwind-tables -fno-unroll-loops \
	-fmerge-all-constants -fno-math-errno \
	-marm -mtune=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard
	CXXFLAGS += $(CFLAGS)
	CPPFLAGS += $(CFLAGS)
	ASFLAGS += $(CFLAGS)
	HAVE_NEON = 1
	ARCH = arm
	CPU_ARCH := arm
	ARM = 1
	ifeq ($(shell echo `$(CC) -dumpversion` "< 4.9" | bc -l), 1)
		CFLAGS += -march=armv7-a
	else
		CFLAGS += -march=armv7ve
		# If gcc is 5.0 or later
		ifeq ($(shell echo `$(CC) -dumpversion` ">= 5" | bc -l), 1)
			LDFLAGS += -static-libgcc -static-libstdc++
		endif
	endif

# Amlogic S812
else ifeq ($(platform), s812)
	TARGET := $(TARGET_NAME)_libretro.so
	fpic := -fPIC
	LDFLAGS += $(fpic) -shared -Wl,--version-script=link.T
	CFLAGS += -Ofast \
	-flto=4 -fwhole-program -fuse-linker-plugin \
	-fdata-sections -ffunction-sections -Wl,--gc-sections \
	-fno-stack-protector -fno-ident -fomit-frame-pointer \
	-falign-functions=1 -falign-jumps=1 -falign-loops=1 \
	-fno-unwind-tables -fno-asynchronous-unwind-tables -fno-unroll-loops \
	-fmerge-all-constants -fno-math-errno \
	-marm -mtune=cortex-a9 -mfpu=neon-vfpv3 -mfloat-abi=hard
	CXXFLAGS += $(CFLAGS)
	CPPFLAGS += $(CFLAGS)
	ASFLAGS += $(CFLAGS)
	HAVE_NEON = 1
	ARCH = arm
	CPU_ARCH := arm
	ARM = 1

# Playstation Classic
else ifeq ($(platform), classic_armv8_a35)
	TARGET := $(TARGET_NAME)_libretro.so
	fpic := -fPIC
	LDFLAGS += $(fpic) -shared -Wl,--version-script=link.T
	CFLAGS += -Ofast \
	-flto=4 -fwhole-program -fuse-linker-plugin \
	-fdata-sections -ffunction-sections -Wl,--gc-sections \
	-fno-stack-protector -fno-ident -fomit-frame-pointer \
	-falign-functions=1 -falign-jumps=1 -falign-loops=1 \
	-fno-unwind-tables -fno-asynchronous-unwind-tables -fno-unroll-loops \
	-fmerge-all-constants -fno-math-errno \
	-marm -mtune=cortex-a35 -mfpu=neon-fp-armv8 -mfloat-abi=hard
	CXXFLAGS += $(CFLAGS)
	CPPFLAGS += $(CFLAGS)
	ASFLAGS += $(CFLAGS)
	HAVE_NEON = 1
	ARCH = arm
	CPU_ARCH := arm
	ARM = 1
	CFLAGS += -march=armv8-a
	LDFLAGS += -static-libgcc -static-libstdc++

# Generic ARM-hf
else ifeq ($(platform), armhf)
	CFLAGS += $(fpic)
	LDFLAGS += $(fpic) -shared -Wl,--version-script=link.T
	PLATCFLAGS += -march=armv6 -mfloat-abi=hard -mfpu=vfp
	PLATCFLAGS += -fomit-frame-pointer -ffast-math

# Android ARM-v7
else ifeq ($(platform), android-armv7)
	TARGET = $(TARGET_NAME)_libretro_android.so
	CFLAGS += -fPIC
	PLATCFLAGS += -march=armv7-a -mfloat-abi=softfp
	LDFLAGS += -fPIC -shared -Wl,--version-script=link.T
	CC = arm-linux-androideabi-gcc
	AR = arm-linux-androideabi-ar
	LD = arm-linux-androideabi-gcc

# QNX
else ifeq ($(platform), qnx)
	TARGET = $(TARGET_NAME)_libretro_$(platform).so
	CFLAGS += -fPIC
	PLATCFLAGS += -march=armv7-a
	LDFLAGS += -fPIC -shared -Wl,--version-script=link.T
	CC = qcc -Vgcc_ntoarmv7le
	AR = qcc -Vgcc_ntoarmv7le
	LD = QCC -Vgcc_ntoarmv7le

# Nintendo 3DS
else ifeq ($(platform), ctr)
	TARGET = $(TARGET_NAME)_libretro_$(platform).a
	CC = $(DEVKITARM)/bin/arm-none-eabi-gcc$(EXE_EXT)
	CXX = $(DEVKITARM)/bin/arm-none-eabi-g++$(EXE_EXT)
	AR = $(DEVKITARM)/bin/arm-none-eabi-ar$(EXE_EXT)
	PLATCFLAGS += -DARM11 -D_3DS
	PLATCFLAGS += -march=armv6k -mtune=mpcore -mfloat-abi=hard -mfpu=vfp
	PLATCFLAGS += -Wall -mword-relocations
	PLATCFLAGS += -fomit-frame-pointer -ffast-math
	CXXFLAGS = $(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++11
	CPU_ARCH := arm
	STATIC_LINKING = 1

# Nintendo GameCube
else ifeq ($(platform), ngc)
	TARGET := $(TARGET_NAME)_libretro_$(platform).a
	BIGENDIAN = 1
	CC = $(DEVKITPPC)/bin/powerpc-eabi-gcc$(EXE_EXT)
	CXX = $(DEVKITPPC)/bin/powerpc-eabi-g++$(EXE_EXT)
	AR = $(DEVKITPPC)/bin/powerpc-eabi-ar$(EXE_EXT)
	PLATCFLAGS += -DGEKKO -DHW_DOL -mrvl -mcpu=750 -meabi -mhard-float
	PLATCFLAGS += -U__INT32_TYPE__ -U __UINT32_TYPE__ -D__INT32_TYPE__=int
	STATIC_LINKING = 1

# Nintendo Wii
else ifeq ($(platform), wii)
	TARGET = $(TARGET_NAME)_$(INCLUDE_DRV)_libretro_$(platform).a
	BIGENDIAN = 1
	CC = $(DEVKITPPC)/bin/powerpc-eabi-gcc$(EXE_EXT)
	AR = $(DEVKITPPC)/bin/powerpc-eabi-ar$(EXE_EXT)
	PLATCFLAGS += -DGEKKO -mrvl -mcpu=750 -meabi -mhard-float -D__ppc__ -D__POWERPC__
	PLATCFLAGS += -U__INT32_TYPE__ -U __UINT32_TYPE__ -D__INT32_TYPE__=int
	PLATCFLAGS += -D$(INCLUDE_DRV) -DSPLIT_CORE
	STATIC_LINKING = 1
	ZLIB_UNCOMPRESS = 1

# Nintendo WiiU
else ifeq ($(platform), wiiu)
	TARGET = $(TARGET_NAME)_libretro_$(platform).a
	BIGENDIAN = 1
	CC = $(DEVKITPPC)/bin/powerpc-eabi-gcc$(EXE_EXT)
	AR = $(DEVKITPPC)/bin/powerpc-eabi-ar$(EXE_EXT)
	PLATCFLAGS += -DGEKKO -DWIIU -mcpu=750 -meabi -mhard-float -D__ppc__ -D__POWERPC__
	PLATCFLAGS += -U__INT32_TYPE__ -U __UINT32_TYPE__ -D__INT32_TYPE__=int
	STATIC_LINKING = 1

# Nintendo Switch (libnx)
else ifeq ($(platform), libnx)
	include $(DEVKITPRO)/libnx/switch_rules
	EXT=a
	TARGET := $(TARGET_NAME)_libretro_$(platform).$(EXT)
	DEFINES := -DSWITCH=1 -U__linux__ -U__linux -DRARCH_INTERNAL -DHAVE_LIBNX
	CFLAGS := $(DEFINES) -g -O3 -ffast-math -fPIE -I$(LIBNX)/include/ -ffunction-sections -fdata-sections -fcommon -ftls-model=local-exec -Wl,--allow-multiple-definition -specs=$(LIBNX)/switch.specs
	CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++11
	CFLAGS += -std=gnu11
	PLATCFLAGS += -D__SWITCH__ -march=armv8-a -mtune=cortex-a57 -mtp=soft -fPIE
	CPU_ARCH := arm64
	STATIC_LINKING = 1

# Nintendo Switch (libtransistor)
else ifeq ($(platform), switch)
	EXT=a
	TARGET := $(TARGET_NAME)_libretro_$(platform).$(EXT)
	PLATCFLAGS += -D__SWITCH__
	include $(LIBTRANSISTOR_HOME)/libtransistor.mk
	STATIC_LINKING=1

# PS2
else ifeq ($(platform), ps2)
	TARGET := $(TARGET_NAME)_libretro_$(platform).a
	CC = ee-gcc$(EXE_EXT)
	CXX = ee-g++$(EXE_EXT)
	AR = ee-ar$(EXE_EXT)
	PLATCFLAGS := -G0 -Wall -DPS2 -DNO_UNALIGNED_ACCESS -DABGR1555 -DRENDER_GSKIT_PS2 -fsingle-precision-constant
	PLATCFLAGS += -I$(PS2SDK)/ee/include -I$(PS2SDK)/common/include  -I$(PS2DEV)/gsKit/include
	PLATCFLAGS += -O3
	PLATCFLAGS += -DHAVE_NO_LANGEXTRA
	CXXFLAGS += -fno-rtti -fno-exceptions -ffast-math
	STATIC_LINKING = 1

# Lightweight PS3 Homebrew SDK
else ifneq (,$(filter $(platform), ps3 psl1ght))
	TARGET := $(TARGET_NAME)_libretro_$(platform).a
	BIGENDIAN = 1
	CC = $(PS3DEV)/ppu/bin/ppu-$(COMMONLV)gcc$(EXE_EXT)
	AR = $(PS3DEV)/ppu/bin/ppu-$(COMMONLV)ar$(EXE_EXT)
	PLATCFLAGS += -D__PS3__ -D__ppc__ -D__POWERPC__
	ifeq ($(platform), psl1ght)
		PLATFORM_DEFINES += -D__PSL1GHT__
	endif
	STATIC_LINKING = 1
	SPLIT_UP_LINK=1

# PSP
else ifeq ($(platform), psp1)
	TARGET = $(TARGET_NAME)_libretro_$(platform).a
	CC = psp-gcc$(EXE_EXT)
	AR = psp-ar$(EXE_EXT)
	PLATCFLAGS += -DPSP
	CFLAGS += -G0
	STATIC_LINKING = 1

# Vita
else ifeq ($(platform), vita)
	TARGET = $(TARGET_NAME)_libretro_$(platform).a
	CC = arm-vita-eabi-gcc$(EXE_EXT)
	AR = arm-vita-eabi-ar$(EXE_EXT)
	PLATCFLAGS += -DVITA -mthumb
	CFLAGS += -mfloat-abi=hard -fsingle-precision-constant
	CFLAGS += -Wall -mword-relocations
	CFLAGS += -fomit-frame-pointer -ffast-math
	CFLAGS += -fno-unwind-tables -fno-asynchronous-unwind-tables
	CFLAGS +=  -fno-optimize-sibling-calls
	CFLAGS += -ftree-vectorize -funroll-loops -fno-short-enums -fcommon
	CXXFLAGS = $(CFLAGS) -fno-rtti -fno-exceptions
	HAVE_RZLIB := 1
	ARM = 1
	STATIC_LINKING := 1
	USE_CYCLONE := 1
	USE_DRZ80 := 1

# ARMv
else ifneq (,$(findstring armv,$(platform)))
	TARGET = $(TARGET_NAME)_libretro.so
	CFLAGS += -fPIC
	LDFLAGS += -fPIC -shared -Wl,--version-script=link.T

# RS90
else ifeq ($(platform), rs90)
	TARGET := $(TARGET_NAME)_libretro.so
	CC = /opt/rs90-toolchain/usr/bin/mipsel-linux-gcc
	CXX = /opt/rs90-toolchain/usr/bin/mipsel-linux-g++
	AR = /opt/rs90-toolchain/usr/bin/mipsel-linux-ar
	LDFLAGS += -shared -Wl,--version-script=link.T -Wl,-no-undefined
	PLATCFLAGS += -D__GCW0__
	LIBS := -lc -lgcc -lm
	fpic := -fPIC -nostdlib
	CFLAGS += -lm -march=mips32 -mtune=mips32

# GCW0
else ifeq ($(platform), gcw0)
	TARGET := $(TARGET_NAME)_libretro.so
	CC = /opt/gcw0-toolchain/usr/bin/mipsel-linux-gcc
	CXX = /opt/gcw0-toolchain/usr/bin/mipsel-linux-g++
	AR = /opt/gcw0-toolchain/usr/bin/mipsel-linux-ar
	LDFLAGS += -shared -Wl,--version-script=link.T -Wl,-no-undefined
	PLATCFLAGS += -D__GCW0__
	LIBS := -lc -lgcc -lm
	fpic := -fPIC -nostdlib
	CFLAGS += -lm -march=mips32 -mtune=mips32r2 -mhard-float

# RetroFW
else ifeq ($(platform), retrofw)
	TARGET := $(TARGET_NAME)_libretro.so
	CC = /opt/retrofw-toolchain/usr/bin/mipsel-linux-gcc
	CXX = /opt/retrofw-toolchain/usr/bin/mipsel-linux-g++
	AR = /opt/retrofw-toolchain/usr/bin/mipsel-linux-ar
	LDFLAGS += -shared -Wl,--version-script=link.T -Wl,-no-undefined
	PLATCFLAGS += -D__GCW0__
	LIBS := -lc -lgcc -lm
	fpic := -fPIC -nostdlib
	CFLAGS += -lm -march=mips32 -mtune=mips32 -mhard-float

# MIYOO
else ifeq ($(platform), miyoo)
	TARGET := $(TARGET_NAME)_libretro.so
	CC = /opt/miyoo/usr/bin/arm-linux-gcc
	CXX = /opt/miyoo/usr/bin/arm-linux-g++
	AR = /opt/miyoo/usr/bin/arm-linux-ar
	fpic := -fPIC
	LDFLAGS += -shared -Wl,--version-script=link.T -Wl,-no-undefined
	PLATCFLAGS := -DNO_UNALIGNED_ACCESS
	PLATCFLAGS += -fomit-frame-pointer -march=armv5te -mtune=arm926ej-s -ffast-math
	CXXFLAGS += -fno-rtti -fno-exceptions

# Emscripten
else ifeq ($(platform), emscripten)
	TARGET := $(TARGET_NAME)_libretro_$(platform).bc
	HAVE_RZLIB := 1
	STATIC_LINKING := 1
	PLATCFLAGS += -D__EMSCRIPTEN__

# Windows MSVC 2003 Xbox 1
else ifeq ($(platform), xbox1_msvc2003)
	TARGET := $(TARGET_NAME)_libretro_xdk1.lib
	MSVCBINDIRPREFIX = $(XDK)/xbox/bin/vc71
	CC  = "$(MSVCBINDIRPREFIX)/CL.exe"
	CXX = "$(MSVCBINDIRPREFIX)/CL.exe"
	LD  = "$(MSVCBINDIRPREFIX)/lib.exe"
	export INCLUDE := $(XDK)/xbox/include
	export LIB := $(XDK)/xbox/lib
	PSS_STYLE :=2
	CFLAGS   += -D_XBOX -D_XBOX1
	CXXFLAGS += -D_XBOX -D_XBOX1
	STATIC_LINKING=1

# Windows MSVC 2010 Xbox 360
else ifeq ($(platform), xbox360_msvc2010)
	TARGET := $(TARGET_NAME)_libretro_xdk360.lib
	MSVCBINDIRPREFIX = $(XEDK)/bin/win32
	CC   = "$(MSVCBINDIRPREFIX)/cl.exe"
	CXX  = "$(MSVCBINDIRPREFIX)/cl.exe"
	LD   = "$(MSVCBINDIRPREFIX)/lib.exe"
	export INCLUDE := $(XEDK)/include/xbox
	export LIB := $(XEDK)/lib/xbox
	PSS_STYLE :=2
	CFLAGS   += -D_XBOX -D_XBOX360
	CXXFLAGS += -D_XBOX -D_XBOX360
	STATIC_LINKING=1
	BIGENDIAN = 1

# Windows MSVC 2010 x64
else ifeq ($(platform), windows_msvc2010_x64)
	CC  = cl.exe
	CXX = cl.exe
	PATH := $(shell IFS=$$'\n'; cygpath "$(VS100COMNTOOLS)../../VC/bin/amd64"):$(PATH)
	PATH := $(PATH):$(shell IFS=$$'\n'; cygpath "$(VS100COMNTOOLS)../IDE")
	LIB := $(shell IFS=$$'\n'; cygpath "$(VS100COMNTOOLS)../../VC/lib/amd64")
	INCLUDE := $(shell IFS=$$'\n'; cygpath "$(VS100COMNTOOLS)../../VC/include")

	WindowsSdkDir := $(shell reg query "HKLM\SOFTWARE\Microsoft\Microsoft SDKs\Windows\v7.1A" -v "InstallationFolder" | grep -o '[A-Z]:\\.*')
	WindowsSdkDir ?= $(shell reg query "HKLM\SOFTWARE\Microsoft\Microsoft SDKs\Windows\v7.0A" -v "InstallationFolder" | grep -o '[A-Z]:\\.*')

	WindowsSDKIncludeDir := $(shell cygpath -w "$(WindowsSdkDir)\Include")
	WindowsSDKGlIncludeDir := $(shell cygpath -w "$(WindowsSdkDir)\Include\gl")
	WindowsSDKLibDir := $(shell cygpath -w "$(WindowsSdkDir)\Lib\x64")

	INCFLAGS_PLATFORM = -I"$(WindowsSDKIncludeDir)"
	export INCLUDE := $(INCLUDE);$(WindowsSDKIncludeDir);$(WindowsSDKGlIncludeDir)
	export LIB := $(LIB);$(WindowsSDKLibDir)
	TARGET := $(TARGET_NAME)_libretro.dll
	PSS_STYLE :=2
	LDFLAGS += -DLL
	LIBS =

# Windows MSVC 2010 x86
else ifeq ($(platform), windows_msvc2010_x86)
	CC  = cl.exe
	CXX = cl.exe
	PATH := $(shell IFS=$$'\n'; cygpath "$(VS100COMNTOOLS)../../VC/bin"):$(PATH)
	PATH := $(PATH):$(shell IFS=$$'\n'; cygpath "$(VS100COMNTOOLS)../IDE")
	LIB := $(shell IFS=$$'\n'; cygpath -w "$(VS100COMNTOOLS)../../VC/lib")
	INCLUDE := $(shell IFS=$$'\n'; cygpath "$(VS100COMNTOOLS)../../VC/include")

	WindowsSdkDir := $(shell reg query "HKLM\SOFTWARE\Microsoft\Microsoft SDKs\Windows\v7.1A" -v "InstallationFolder" | grep -o '[A-Z]:\\.*')
	WindowsSdkDir ?= $(shell reg query "HKLM\SOFTWARE\Microsoft\Microsoft SDKs\Windows\v7.0A" -v "InstallationFolder" | grep -o '[A-Z]:\\.*')

	WindowsSDKIncludeDir := $(shell cygpath -w "$(WindowsSdkDir)\Include")
	WindowsSDKGlIncludeDir := $(shell cygpath -w "$(WindowsSdkDir)\Include\gl")
	WindowsSDKLibDir := $(shell cygpath -w "$(WindowsSdkDir)\Lib")

	INCFLAGS_PLATFORM = -I"$(WindowsSDKIncludeDir)"
	export INCLUDE := $(INCLUDE);$(WindowsSDKIncludeDir);$(WindowsSDKGlIncludeDir)
	export LIB := $(LIB);$(WindowsSDKLibDir)
	TARGET := $(TARGET_NAME)_libretro.dll
	PSS_STYLE :=2
	LDFLAGS += -DLL
	LIBS =

# Windows MSVC 2003 x86
else ifeq ($(platform), windows_msvc2003_x86)
 	CC  = cl.exe
 	CXX = cl.exe
 	PATH := $(shell IFS=$$'\n'; cygpath "$(VS71COMNTOOLS)../../Vc7/bin"):$(PATH)
 	PATH := $(PATH):$(shell IFS=$$'\n'; cygpath "$(VS71COMNTOOLS)../IDE")
 	INCLUDE := $(shell IFS=$$'\n'; cygpath -w "$(VS71COMNTOOLS)../../Vc7/include")
 	LIB := $(shell IFS=$$'\n'; cygpath -w "$(VS71COMNTOOLS)../../Vc7/lib")
 	BIN := $(shell IFS=$$'\n'; cygpath "$(VS71COMNTOOLS)../../Vc7/bin")
 	WindowsSdkDir := $(INETSDK)
 	export INCLUDE := $(INCLUDE);$(INETSDK)/Include;src/libretro/libretro-common/include/compat/msvc
 	export LIB := $(LIB);$(WindowsSdkDir);$(INETSDK)/Lib
 	TARGET := $(TARGET_NAME)_libretro.dll
 	PSS_STYLE :=2
 	LDFLAGS += -DLL
 	CFLAGS += -D_CRT_SECURE_NO_DEPRECATE
 	LIBS =

# Windows MSVC 2005 x86
else ifeq ($(platform), windows_msvc2005_x86)
 	CC  = cl.exe
 	CXX = cl.exe
 	PATH := $(shell IFS=$$'\n'; cygpath "$(VS80COMNTOOLS)../../VC/bin"):$(PATH)
 	PATH := $(PATH):$(shell IFS=$$'\n'; cygpath "$(VS80COMNTOOLS)../IDE")
 	INCLUDE := $(shell IFS=$$'\n'; cygpath "$(VS80COMNTOOLS)../../VC/include")
 	LIB := $(shell IFS=$$'\n'; cygpath -w "$(VS80COMNTOOLS)../../VC/lib")
 	BIN := $(shell IFS=$$'\n'; cygpath "$(VS80COMNTOOLS)../../VC/bin")

	WindowsSdkDir := $(shell reg query "HKLM\SOFTWARE\Microsoft\MicrosoftSDK\InstalledSDKs\8F9E5EF3-A9A5-491B-A889-C58EFFECE8B3" -v "Install Dir" | grep -o '[A-Z]:\\.*')

	WindowsSDKIncludeDir := $(shell cygpath -w "$(WindowsSdkDir)\Include")
	WindowsSDKAtlIncludeDir := $(shell cygpath -w "$(WindowsSdkDir)\Include\atl")
	WindowsSDKCrtIncludeDir := $(shell cygpath -w "$(WindowsSdkDir)\Include\crt")
	WindowsSDKGlIncludeDir := $(shell cygpath -w "$(WindowsSdkDir)\Include\gl")
	WindowsSDKMfcIncludeDir := $(shell cygpath -w "$(WindowsSdkDir)\Include\mfc")
	WindowsSDKLibDir := $(shell cygpath -w "$(WindowsSdkDir)\Lib")

	export INCLUDE := $(INCLUDE);$(WindowsSDKIncludeDir);$(WindowsSDKAtlIncludeDir);$(WindowsSDKCrtIncludeDir);$(WindowsSDKGlIncludeDir);$(WindowsSDKMfcIncludeDir);src/libretro/libretro-common/include/compat/msvc
	export LIB := $(LIB);$(WindowsSDKLibDir)
 	TARGET := $(TARGET_NAME)_libretro.dll
 	PSS_STYLE :=2
 	LDFLAGS += -DLL
 	CFLAGS += -D_CRT_SECURE_NO_DEPRECATE
 	LIBS =

# Windows MSVC 2017 all architectures
else ifneq (,$(findstring windows_msvc2017,$(platform)))
	NO_GCC := 1
	CFLAGS += -DNOMINMAX
	CXXFLAGS += -DNOMINMAX
	WINDOWS_VERSION = 1
	PlatformSuffix = $(subst windows_msvc2017_,,$(platform))

	ifneq (,$(findstring desktop,$(PlatformSuffix)))
		WinPartition = desktop
		MSVC2017CompileFlags = -DWINAPI_FAMILY=WINAPI_FAMILY_DESKTOP_APP -FS
		LDFLAGS += -MANIFEST -LTCG:incremental -NXCOMPAT -DYNAMICBASE -DEBUG -OPT:REF -INCREMENTAL:NO -SUBSYSTEM:WINDOWS -MANIFESTUAC:"level='asInvoker' uiAccess='false'" -OPT:ICF -ERRORREPORT:PROMPT -NOLOGO -TLBID:1
		LIBS += kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib
	else ifneq (,$(findstring uwp,$(PlatformSuffix)))
		WinPartition = uwp
		MSVC2017CompileFlags = -DWINAPI_FAMILY=WINAPI_FAMILY_APP -D_WINDLL -D_UNICODE -DUNICODE -D__WRL_NO_DEFAULT_LIB__ -EHsc -FS
		LDFLAGS += -APPCONTAINER -NXCOMPAT -DYNAMICBASE -MANIFEST:NO -LTCG -OPT:REF -SUBSYSTEM:CONSOLE -MANIFESTUAC:NO -OPT:ICF -ERRORREPORT:PROMPT -NOLOGO -TLBID:1 -DEBUG:FULL -WINMD:NO
		LIBS += WindowsApp.lib
	endif

	CFLAGS += $(MSVC2017CompileFlags)
	CXXFLAGS += $(MSVC2017CompileFlags)

	TargetArchMoniker = $(subst $(WinPartition)_,,$(PlatformSuffix))

	CC  = cl.exe
	CXX = cl.exe
	LD = link.exe

	reg_query = $(call filter_out2,$(subst $2,,$(shell reg query "$2" -v "$1" 2>nul)))
	fix_path = $(subst $(SPACE),\ ,$(subst \,/,$1))

	ProgramFiles86w := $(shell cmd //c "echo %PROGRAMFILES(x86)%")
	ProgramFiles86 := $(shell cygpath "$(ProgramFiles86w)")

	WindowsSdkDir ?= $(call reg_query,InstallationFolder,HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\Microsoft\Microsoft SDKs\Windows\v10.0)
	WindowsSdkDir ?= $(call reg_query,InstallationFolder,HKEY_CURRENT_USER\SOFTWARE\Wow6432Node\Microsoft\Microsoft SDKs\Windows\v10.0)
	WindowsSdkDir ?= $(call reg_query,InstallationFolder,HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Microsoft SDKs\Windows\v10.0)
	WindowsSdkDir ?= $(call reg_query,InstallationFolder,HKEY_CURRENT_USER\SOFTWARE\Microsoft\Microsoft SDKs\Windows\v10.0)
	WindowsSdkDir := $(WindowsSdkDir)

	WindowsSDKVersion ?= $(firstword $(foreach folder,$(subst $(subst \,/,$(WindowsSdkDir)Include/),,$(wildcard $(call fix_path,$(WindowsSdkDir)Include\*))),$(if $(wildcard $(call fix_path,$(WindowsSdkDir)Include/$(folder)/um/Windows.h)),$(folder),)))$(BACKSLASH)
	WindowsSDKVersion := $(WindowsSDKVersion)

	VsInstallBuildTools = $(ProgramFiles86)/Microsoft Visual Studio/2017/BuildTools
	VsInstallEnterprise = $(ProgramFiles86)/Microsoft Visual Studio/2017/Enterprise
	VsInstallProfessional = $(ProgramFiles86)/Microsoft Visual Studio/2017/Professional
	VsInstallCommunity = $(ProgramFiles86)/Microsoft Visual Studio/2017/Community

	VsInstallRoot ?= $(shell if [ -d "$(VsInstallBuildTools)" ]; then echo "$(VsInstallBuildTools)"; fi)
	ifeq ($(VsInstallRoot), )
		VsInstallRoot = $(shell if [ -d "$(VsInstallEnterprise)" ]; then echo "$(VsInstallEnterprise)"; fi)
	endif
	ifeq ($(VsInstallRoot), )
		VsInstallRoot = $(shell if [ -d "$(VsInstallProfessional)" ]; then echo "$(VsInstallProfessional)"; fi)
	endif
	ifeq ($(VsInstallRoot), )
		VsInstallRoot = $(shell if [ -d "$(VsInstallCommunity)" ]; then echo "$(VsInstallCommunity)"; fi)
	endif
	VsInstallRoot := $(VsInstallRoot)

	VcCompilerToolsVer := $(shell cat "$(VsInstallRoot)/VC/Auxiliary/Build/Microsoft.VCToolsVersion.default.txt" | grep -o '[0-9\.]*')
	VcCompilerToolsDir := $(VsInstallRoot)/VC/Tools/MSVC/$(VcCompilerToolsVer)

	WindowsSDKSharedIncludeDir := $(shell cygpath -w "$(WindowsSdkDir)\Include\$(WindowsSDKVersion)\shared")
	WindowsSDKUCRTIncludeDir := $(shell cygpath -w "$(WindowsSdkDir)\Include\$(WindowsSDKVersion)\ucrt")
	WindowsSDKUMIncludeDir := $(shell cygpath -w "$(WindowsSdkDir)\Include\$(WindowsSDKVersion)\um")
	WindowsSDKUCRTLibDir := $(shell cygpath -w "$(WindowsSdkDir)\Lib\$(WindowsSDKVersion)\ucrt\$(TargetArchMoniker)")
	WindowsSDKUMLibDir := $(shell cygpath -w "$(WindowsSdkDir)\Lib\$(WindowsSDKVersion)\um\$(TargetArchMoniker)")

	# For some reason the HostX86 compiler doesn't like compiling for x64
	# ("no such file" opening a shared library), and vice-versa.
	# Work around it for now by using the strictly x86 compiler for x86, and x64 for x64.
	# NOTE: What about ARM?
	ifneq (,$(findstring x64,$(TargetArchMoniker)))
		VCCompilerToolsBinDir := $(VcCompilerToolsDir)\bin\HostX64
	else
		VCCompilerToolsBinDir := $(VcCompilerToolsDir)\bin\HostX86
	endif

	PATH := $(shell IFS=$$'\n'; cygpath "$(VCCompilerToolsBinDir)/$(TargetArchMoniker)"):$(PATH)
	PATH := $(PATH):$(shell IFS=$$'\n'; cygpath "$(VsInstallRoot)/Common7/IDE")
	INCLUDE := $(shell IFS=$$'\n'; cygpath -w "$(VcCompilerToolsDir)/include")
	LIB := $(shell IFS=$$'\n'; cygpath -w "$(VcCompilerToolsDir)/lib/$(TargetArchMoniker)")
	ifneq (,$(findstring uwp,$(PlatformSuffix)))
		LIB := $(LIB);$(shell IFS=$$'\n'; cygpath -w "$(LIB)/store")
	endif

	export INCLUDE := $(INCLUDE);$(WindowsSDKSharedIncludeDir);$(WindowsSDKUCRTIncludeDir);$(WindowsSDKUMIncludeDir)
	export LIB := $(LIB);$(WindowsSDKUCRTLibDir);$(WindowsSDKUMLibDir)
	TARGET := $(TARGET_NAME)_libretro.dll
	PSS_STYLE :=2
	LDFLAGS += -DLL

# Windows
else
	TARGET := $(TARGET_NAME)_libretro.dll
	CC ?= gcc
	LDFLAGS += -shared -static-libgcc -static-libstdc++ -s -Wl,--version-script=link.T
	CFLAGS += -D__WIN32__
endif


# Architecture-specific flags #############################
ifeq ($(BIGENDIAN), 1)
	PLATCFLAGS += -DMSB_FIRST
endif
# End of architecture-specific flags ######################

# Platform-specific flags #################################
# All Android platforms       #############################
ifneq ($(findstring Android, $(platform)), )
	PLATCFLAGS += -D__ANDROID__
else ifneq ($(findstring android, $(platform)), )
	PLATCFLAGS += -D__ANDROID__
endif

# All MSVC platforms
ifeq (,$(findstring msvc,$(platform)))
	CFLAGS += -D_XOPEN_SOURCE=500 -fomit-frame-pointer -fstrict-aliasing
endif

# All MSVC platforms except the SNC PS3
ifneq ($(platform), sncps3)
	ifeq (,$(findstring msvc,$(platform)))
		CFLAGS += -Wall -Wunused \
		-Wpointer-arith -Wbad-function-cast -Wcast-align -Waggregate-return \
		-Wshadow -Wstrict-prototypes \
		-Wformat-security -Wwrite-strings \
		-Wdisabled-optimization
	endif
endif

# End of platform-specific flags ##########################

# Compiler flags for all platforms and architectures ######

CFLAGS += $(fpic)        # Position-independent code
CFLAGS += -fsigned-char  # Older MAME code assumes signed character type (x86 platform)

CFLAGS += -DFLAC__NO_ASM -DFLAC__NO_DLL
CFLAGS += -DHAVE_INTTYPES_H
CFLAGS += -DHAVE_ICONV
CFLAGS += -DHAVE_LANGINFO_CODESET
CFLAGS += -DHAVE_SOCKLEN_T
CFLAGS += -D_LARGEFILE_SOURCE
CFLAGS += -D_FILE_OFFSET_BITS=64

# Required for RZIP support in cheat.c
CFLAGS += -DHAVE_ZLIB

# In theory, the RETRO_PROFILE could be set to different values for different
# architectures or for special builds to hint to the host system how many
# resources to allocate. In practice, there seems to be no standard way to
# rate performance needs and no point in doing so.
# As of June 2021, the libretro performance profile callback is not known
# to be implemented by any frontends. RetroArch does not use this callback
# and its developers do not have a suggested range of values. We use 10 by
# convention (copying other cores).
RETRO_PROFILE = 10
CFLAGS += -DRETRO_PROFILE=$(RETRO_PROFILE)

# End compiler flags for all platforms and architectures ##


# Disable optimization when debugging #####################
ifeq ($(DEBUG), 1)
	CFLAGS += -O0 -g3
else
	CFLAGS += -O2 -DNDEBUG
endif

ifneq (,$(findstring msvc,$(platform)))
ifeq ($(DEBUG),1)
	CFLAGS += -MTd
else
	CFLAGS += -MT
endif
endif

# include the various .mak files
ifneq (,$(filter $(INCLUDE_DRV),all))
	include Makefile.common
else
	include Makefile.split
endif

# build the targets in different object dirs, since mess changes
# some structures and thus they can't be linked against each other.
DEFS = $(COREDEFINES) -Dasm=__asm__

CFLAGS += $(INCFLAGS) $(INCFLAGS_PLATFORM)

# combine the various definitions to one
CDEFS = $(DEFS) $(COREDEFS) $(CPUDEFS) $(SOUNDDEFS) $(ASMDEFS) $(DBGDEFS)

OBJECTS := $(SOURCES_C:.c=.o) $(SOURCES_ASM:.s=.o)

OBJOUT   = -o
LINKOUT  = -o 

ifneq (,$(findstring msvc,$(platform)))
	OBJOUT = -Fo
	LINKOUT = -out:
ifeq ($(STATIC_LINKING),1)
	LD ?= lib.exe
	STATIC_LINKING=0
else
	LD = link.exe
endif
else
	LD = $(CC)
endif

define NEWLINE


endef

all:	$(TARGET)
$(TARGET): $(OBJECTS)
ifeq ($(STATIC_LINKING),1)
	@echo Archiving $@...
ifeq ($(SPLIT_UP_LINK), 1)
	$(HIDE)$(AR) rcs $@ $(foreach OBJECTS,$(OBJECTS),$(NEWLINE) $(AR) q $@ $(OBJECTS))
else
	$(HIDE)$(AR) rcs $@ $(OBJECTS)
endif
else
	@echo Linking $@...
	@echo platform $(system_platform)
ifeq ($(SPLIT_UP_LINK), 1)
	# Use a temporary file to hold the list of objects, as it can exceed windows shell command limits
	$(HIDE)$(file >$@.in,$(OBJECTS))
	$(HIDE)$(LD) $(LDFLAGS) $(LINKOUT)$@ @$@.in $(LIBS)
	@rm $@.in
else
	$(HIDE)$(LD) $(LDFLAGS) $(LINKOUT)$@ $(OBJECTS) $(LIBS)
endif
endif

CFLAGS += $(PLATCFLAGS) $(CDEFS)

%.o: %.c
	@echo Compiling $<...
	$(HIDE)$(CC) -c $(OBJOUT)$@ $< $(CFLAGS)

%.o: %.s
	$(CC) -c $(OBJOUT)$@ $< $(CFLAGS)

$(OBJ)/%.a:
	@echo Archiving $@...
	@$(RM) $@
	$(HIDE)$(AR) cr $@ $^

clean:
	@echo Cleaning project...
ifeq ($(SPLIT_UP_LINK), 1)
	# Use a temporary file to hold the list of objects, as it can exceed windows shell command limits
	$(HIDE)$(file >$@.in,$(OBJECTS))
	$(HIDE)rm -f @$@.in $(TARGET)
	@rm $@.in
endif
	$(HIDE)rm -f $(OBJECTS) $(TARGET)
