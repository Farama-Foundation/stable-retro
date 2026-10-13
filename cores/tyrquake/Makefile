DEBUG=0
FRONTEND_SUPPORTS_RGB565=1
TARGET_NAME=tyrquake
STATIC_LINKING=0
GIT_VERSION := " e57bb11"
ifneq ($(GIT_VERSION)," unknown")
	CFLAGS += -DGIT_VERSION=\"$(GIT_VERSION)\"
endif

HAVE_NETWORKING=1
USE_CODEC_WAVE=1
USE_CODEC_FLAC=1
USE_CODEC_VORBIS=1
USE_CODEC_MP3=0
USE_CODEC_OPUS=0
# either mikmod (preferred) or modplug, not both
USE_CODEC_MIKMOD=0
USE_CODEC_MODPLUG=0
USE_CODEC_UMX=0

# which library to use for mp3 decoding: mad or mpg123
MP3LIB=mad
# ogg decoding uses the vendored fixed-point Tremor (libvorbisidec) decoder
# under deps/tremor for deterministic int16 PCM
VORBISLIB=tremor

ifeq ($(platform),)
platform = unix
ifeq ($(shell uname -a),)
   platform = win
else ifneq ($(findstring MINGW,$(shell uname -a)),)
   platform = win
else ifneq ($(findstring Darwin,$(shell uname -a)),)
   platform = osx
	arch = intel
ifeq ($(shell uname -p),powerpc)
	arch = ppc
endif
else ifneq ($(findstring win,$(shell uname -a)),)
   platform = win
endif
endif

# system platform
system_platform = unix
ifeq ($(shell uname -a),)
EXE_EXT = .exe
   system_platform = win
else ifneq ($(findstring Darwin,$(shell uname -a)),)
   system_platform = osx
	arch = intel
ifeq ($(shell uname -p),powerpc)
	arch = ppc
endif
else ifneq ($(findstring MINGW,$(shell uname -a)),)
   system_platform = win
endif

ifneq (,$(findstring msvc,$(platform)))
LIBS :=
else
LIBS := -lm
endif

ifeq ($(STATIC_LINKING),1)
EXT=a

ifeq ($(platform), unix)
PLAT=_unix
endif
endif

SPACE :=
SPACE := $(SPACE) $(SPACE)
BACKSLASH :=
BACKSLASH := \$(BACKSLASH)
filter_out1 = $(filter-out $(firstword $1),$1)
filter_out2 = $(call filter_out1,$(call filter_out1,$1))
unixpath = $(subst \,/,$1)
unixcygpath = /$(subst :,,$(call unixpath,$1))

# UNIX
ifeq ($(platform), unix)
	EXT    ?= so
   TARGET := $(TARGET_NAME)_libretro$(PLAT).$(EXT)
   fpic := -fPIC
   SHARED := -shared -Wl,--version-script=common/libretro-link.T

# Linux (portable library)
else ifeq ($(platform), linux-portable)
	EXT    ?= so
	TARGET := $(TARGET_NAME)_libretro.$(EXT)
   fpic := -fPIC -nostdlib
   SHARED := -shared -Wl,--version-script=common/libretro-link.T
	LIBS =

# OS X
else ifeq ($(platform), osx)
	EXT    ?= dylib
	TARGET := $(TARGET_NAME)_libretro.$(EXT)
   fpic := -fPIC
   SHARED := -dynamiclib -framework CoreFoundation
ifeq ($(arch),ppc)
   CFLAGS += -D__ppc__ -DMSB_FIRST
endif
   OSXVER = `sw_vers -productVersion | cut -d. -f 2`
   OSX_LT_MAVERICKS = `(( $(OSXVER) <= 9)) && echo "YES"`
ifeq ($(OSX_LT_MAVERICKS),YES)
   fpic += -mmacosx-version-min=10.1
endif
   ifeq ($(CROSS_COMPILE),1)
	TARGET_RULE   = -target $(LIBRETRO_APPLE_PLATFORM) -isysroot $(LIBRETRO_APPLE_ISYSROOT)
	CFLAGS   += $(TARGET_RULE)
	LDFLAGS  += $(TARGET_RULE)
   endif

# iOS
else ifneq (,$(findstring ios,$(platform)))
	EXT    ?= dylib
	TARGET := $(TARGET_NAME)_libretro_ios.$(EXT)
   fpic := -fPIC
   SHARED := -dynamiclib -framework CoreFoundation

ifeq ($(IOSSDK),)
   IOSSDK := $(shell xcodebuild -version -sdk iphoneos Path)
endif
ifeq ($(platform), ios-arm64)
   CFLAGS += -DIOS_ARM64
   CC = clang -arch arm64 -isysroot $(IOSSDK) -march=armv8-a -mtune=cortex-a57
else
   CC = clang -arch armv7 -isysroot $(IOSSDK)
endif
   CFLAGS += -DIOS
ifeq ($(platform),$(filter $(platform),ios9 ios-arm64))
	CC     += -miphoneos-version-min=8.0
	CFLAGS += -miphoneos-version-min=8.0
else
	CC     += -miphoneos-version-min=5.0
	CFLAGS += -miphoneos-version-min=5.0
endif

# tvOS 
else ifeq ($(platform), tvos-arm64)
	EXT    ?= dylib
	TARGET := $(TARGET_NAME)_libretro_tvos.$(EXT)
   fpic := -fPIC
   SHARED := -dynamiclib -framework CoreFoundation

ifeq ($(IOSSDK),)
   IOSSDK := $(shell xcodebuild -version -sdk appletvos Path)
endif
   CC = clang -arch arm64 -isysroot $(IOSSDK)
   CXX = clang++ -arch arm64 -isysroot $(IOSSDK)
   CFLAGS += -DIOS -DIOS_ARM64
   CC     += -mappletvos-version-min=11.0
   CFLAGS += -mappletvos-version-min=11.0

# iOS Theos
else ifeq ($(platform), theos_ios)
DEPLOYMENT_IOSVERSION = 5.0
TARGET = iphone:latest:$(DEPLOYMENT_IOSVERSION)
ARCHS = armv7 armv7s
TARGET_IPHONEOS_DEPLOYMENT_VERSION=$(DEPLOYMENT_IOSVERSION)
THEOS_BUILD_DIR := objs
include $(THEOS)/makefiles/common.mk

LIBRARY_NAME = $(TARGET_NAME)_libretro_ios

# QNX
else ifeq ($(platform), qnx)
	EXT    ?= so
   TARGET := $(TARGET_NAME)_libretro_qnx.$(EXT)
   fpic := -fPIC
   SHARED := -shared -Wl,--version-script=common/libretro-link.T
	CC = qcc -Vgcc_ntoarmv7le
	AR = qcc -Vgcc_ntoarmv7le
	CFLAGS += -D__BLACKBERRY_QNX__ -marm -mcpu=cortex-a9 -mfpu=neon -mfloat-abi=softfp

# Lightweight PS3 Homebrew SDK
else ifneq (,$(filter $(platform), ps3 psl1ght))
	TARGET := $(TARGET_NAME)_libretro_$(platform).a
	CFLAGS += -D__ppc__ -DMSB_FIRST -D__PS3__
	STATIC_LINKING = 1
	CC = $(PS3DEV)/ppu/bin/ppu-$(COMMONLV)gcc$(EXE_EXT)
	AR = $(PS3DEV)/ppu/bin/ppu-$(COMMONLV)ar$(EXE_EXT)
	ifeq ($(platform), psl1ght)
		CFLAGS += -D__PSL1GHT__ -I$(PS3DEV)/ppu/include
	endif

# PSP1
else ifeq ($(platform), psp1)
	EXT=a
	TARGET := $(TARGET_NAME)_libretro_$(platform).$(EXT)
	CC = psp-gcc$(EXE_EXT)
	AR = psp-ar$(EXE_EXT)
	CFLAGS += -DPSP -G0 -I$(shell psp-config --pspsdk-path)/include
	STATIC_LINKING = 1

# Vita
else ifeq ($(platform), vita)
	EXT=a
	TARGET := $(TARGET_NAME)_libretro_$(platform).$(EXT)
	CC = arm-vita-eabi-gcc$(EXE_EXT)
	AR = arm-vita-eabi-ar$(EXE_EXT)
	CFLAGS += -DVITA -fno-short-enums -fno-exceptions \
					-fno-unwind-tables -fno-asynchronous-unwind-tables -ftree-vectorize \
					-mfloat-abi=hard -ffast-math -fsingle-precision-constant -funroll-loops
	STATIC_LINKING = 1

# CTR (3DS)
else ifeq ($(platform), ctr)
	EXT=a
	TARGET := $(TARGET_NAME)_libretro_ctr.$(EXT)
	CC = $(DEVKITARM)/bin/arm-none-eabi-gcc$(EXE_EXT)
	AR = $(DEVKITARM)/bin/arm-none-eabi-ar$(EXE_EXT)
	CFLAGS += -DARM11 -D_3DS
	CFLAGS += -march=armv6k -mtune=mpcore -mfloat-abi=hard
	CFLAGS += -Wall -mword-relocations
	CFLAGS += -fomit-frame-pointer -ffast-math
	CFLAGS += -I$(DEVKITPRO)/libctru/include
	STATIC_LINKING = 1

# GCW0
else ifeq ($(platform), gcw0)
        TARGET := $(TARGET_NAME)_libretro.so
        CC = /opt/gcw0-toolchain/usr/bin/mipsel-linux-gcc
        AR = /opt/gcw0-toolchain/usr/bin/mipsel-linux-ar
        fpic := -fPIC
        SHARED := -shared -Wl,--version-script=common/libretro-link.T
        CFLAGS += -DDINGUX -fomit-frame-pointer -ffast-math -march=mips32 -mtune=mips32r2 -mhard-float
        HAVE_NETWORKING=0

# Miyoo
else ifeq ($(platform), miyoo)
        TARGET := $(TARGET_NAME)_libretro.so
        CC = /opt/miyoo/usr/bin/arm-linux-gcc
        AR = /opt/miyoo/usr/bin/arm-linux-ar
        fpic := -fPIC
        SHARED := -shared -Wl,--version-script=common/libretro-link.T
        CFLAGS += -DDINGUX -D_MIYOO -fomit-frame-pointer -ffast-math -mcpu=arm926ej-s
        HAVE_NETWORKING=0

# Raspberry Pi 1
else ifeq ($(platform), rpi1)
        TARGET := $(TARGET_NAME)_libretro.so
        fpic := -fPIC
        SHARED := -shared -Wl,--version-script=common/libretro-link.T
        CFLAGS += -DARM11
        CFLAGS += -marm -march=armv6j -mfloat-abi=hard -mfpu=vfp
        CFLAGS += -fomit-frame-pointer

# Raspberry Pi 2
else ifeq ($(platform), rpi2)
        TARGET := $(TARGET_NAME)_libretro.so
        fpic := -fPIC
        SHARED := -shared -Wl,--version-script=common/libretro-link.T
        CFLAGS += -DARM
        CFLAGS += -marm -mcpu=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard
        CFLAGS += -fomit-frame-pointer
            
# Raspberry Pi 3
else ifeq ($(platform), rpi3)
        TARGET := $(TARGET_NAME)_libretro.so
        fpic := -fPIC
        SHARED := -shared -Wl,--version-script=common/libretro-link.T
        CFLAGS += -DARM
        CFLAGS += -marm -mcpu=cortex-a53 -mfpu=neon-fp-armv8 -mfloat-abi=hard
        CFLAGS += -fomit-frame-pointer

# Raspberry Pi 3 with 64bit kernel & libs
else ifeq ($(platform), rpi3_64)
        TARGET := $(TARGET_NAME)_libretro.so
        fpic := -fPIC
        SHARED := -shared -Wl,--version-script=common/libretro-link.T
        CFLAGS += -DARM
        CFLAGS += -march=armv8-a+crc -mtune=cortex-a53
        CFLAGS += -fomit-frame-pointer

# Raspberry Pi 4 with 64bit kernel & libs
else ifeq ($(platform), rpi4_64)
        TARGET := $(TARGET_NAME)_libretro.so
        fpic := -fPIC
        SHARED := -shared -Wl,--version-script=common/libretro-link.T
        CFLAGS += -DARM
        CFLAGS += -march=armv8-a+crc+simd -mtune=cortex-a72
        CFLAGS += -fomit-frame-pointer

# Classic Platforms ####################
# Platform affix = classic_<ISA>_<µARCH>
# Help at https://modmyclassic.com/comp

# (armv7 a7, hard point, neon based) ### 
# NESC, SNESC, C64 mini 
else ifeq ($(platform), classic_armv7_a7)
	TARGET := $(TARGET_NAME)_libretro.so
	fpic := -fPIC
        SHARED := -shared -Wl,--version-script=common/libretro-link.T  -Wl,--no-undefined
	CFLAGS += -DARM -Ofast \
	#-fwhole-program -fuse-linker-plugin \
	-fdata-sections -ffunction-sections -Wl,--gc-sections \
	-fno-stack-protector -fno-ident -fomit-frame-pointer \
	-falign-functions=1 -falign-jumps=1 -falign-loops=1 \
	-fno-unwind-tables -fno-asynchronous-unwind-tables -fno-unroll-loops \
	-fmerge-all-constants -fno-math-errno \
	-marm -mtune=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard
	ifeq ($(shell echo `$(CC) -dumpversion` "< 4.9" | bc -l), 1)
	  CFLAGS += -march=armv7-a
	else
	  CFLAGS += -march=armv7ve
	  # If gcc is 5.0 or later
	  ifeq ($(shell echo `$(CC) -dumpversion` ">= 5" | bc -l), 1)
	    LDFLAGS += -static-libgcc -static-libstdc++
	  endif
	endif

# (armv8 a35, hard point, neon based) ###
# PS Classic
else ifeq ($(platform), classic_armv8_a35)
	TARGET := $(TARGET_NAME)_libretro.so
	fpic := -fPIC
        SHARED := -shared -Wl,--version-script=common/libretro-link.T  -Wl,--no-undefined
	CFLAGS += -DARM -Ofast \
	#-fwhole-program -fuse-linker-plugin \
	-fdata-sections -ffunction-sections -Wl,--gc-sections \
	-fno-stack-protector -fno-ident -fomit-frame-pointer \
	-falign-functions=1 -falign-jumps=1 -falign-loops=1 \
	-fno-unwind-tables -fno-asynchronous-unwind-tables -fno-unroll-loops \
	-fmerge-all-constants -fno-math-errno \
	-marm -mtune=cortex-a35 -mfpu=neon-fp-armv8 -mfloat-abi=hard
	ifeq ($(shell echo `$(CC) -dumpversion` "< 4.9" | bc -l), 1)
	  CFLAGS += -march=armv8-a
	else
	  CFLAGS += -march=armv8-a
	  # If gcc is 5.0 or later
	  ifeq ($(shell echo `$(CC) -dumpversion` ">= 5" | bc -l), 1)
	    LDFLAGS += -static-libgcc -static-libstdc++
	  endif
	endif
#######################################

# Libxenon
else ifeq ($(platform), xenon)
	EXT=a
	TARGET := $(TARGET_NAME)_libretro_xenon360.$(EXT)
   CC = xenon-gcc$(EXE_EXT)
   AR = xenon-ar$(EXE_EXT)
   CFLAGS += -D__LIBXENON__ -m32 -D__ppc__ -DMSB_FIRST
	STATIC_LINKING = 1

# Nintendo Game Cube
else ifeq ($(platform), ngc)
	EXT=a
   TARGET := $(TARGET_NAME)_libretro_$(platform).$(EXT)
   CC = $(DEVKITPPC)/bin/powerpc-eabi-gcc$(EXE_EXT)
   AR = $(DEVKITPPC)/bin/powerpc-eabi-ar$(EXE_EXT)
   CFLAGS += -DGEKKO -DHW_DOL -mrvl -mcpu=750 -meabi -mhard-float -D__ppc__ -DMSB_FIRST -I$(DEVKITPRO)/libogc/include
   CFLAGS += -U__INT32_TYPE__ -U __UINT32_TYPE__ -D__INT32_TYPE__=int
	STATIC_LINKING = 1
	HAVE_NETWORKING=0

# Nintendo Wii
else ifeq ($(platform), wii)
   TARGET := $(TARGET_NAME)_libretro_$(platform).a
   CC = $(DEVKITPPC)/bin/powerpc-eabi-gcc$(EXE_EXT)
   AR = $(DEVKITPPC)/bin/powerpc-eabi-ar$(EXE_EXT)
   CFLAGS += -DGEKKO -DHW_RVL -mrvl -mcpu=750 -meabi -mhard-float -D__ppc__ -DMSB_FIRST -I$(DEVKITPRO)/libogc/include
   CFLAGS += -U__INT32_TYPE__ -U __UINT32_TYPE__ -D__INT32_TYPE__=int
	STATIC_LINKING = 1
	HAVE_NETWORKING=0

# Nintendo Switch (libnx)
else ifeq ($(platform), libnx)
    include $(DEVKITPRO)/libnx/switch_rules
    EXT=a
    TARGET := $(TARGET_NAME)_libretro_$(platform).$(EXT)
    DEFINES := -DSWITCH=1 -U__linux__ -U__linux -DRARCH_INTERNAL
    CFLAGS	:=	 $(DEFINES) -g -O3 \
                 -fPIE -I$(LIBNX)/include/ -ffunction-sections -fdata-sections -ftls-model=local-exec -Wl,--allow-multiple-definition -specs=$(LIBNX)/switch.specs
    CFLAGS += $(INCDIRS)
    CFLAGS	+=	-D__SWITCH__ -DHAVE_LIBNX -march=armv8-a -mtune=cortex-a57 -mtp=soft
    CXXFLAGS := $(ASFLAGS) $(CFLAGS) -fno-rtti -std=gnu++11
    CFLAGS += -std=gnu11
    STATIC_LINKING = 1

# Nintendo WiiU
else ifeq ($(platform), wiiu)
   TARGET := $(TARGET_NAME)_libretro_$(platform).a
   CC = $(DEVKITPPC)/bin/powerpc-eabi-gcc$(EXE_EXT)
   AR = $(DEVKITPPC)/bin/powerpc-eabi-ar$(EXE_EXT)
   CFLAGS += -DWIIU -mcpu=750 -meabi -mhard-float -D__ppc__ -DMSB_FIRST -I$(DEVKITPRO)/libogc/include
   CFLAGS += -ffunction-sections -fdata-sections -D__wiiu__ -D__wut__
	STATIC_LINKING = 1
	HAVE_NETWORKING=0

# Nintendo Switch (libtransistor)
else ifeq ($(platform), switch)
	EXT=a
        TARGET := $(TARGET_NAME)_libretro_$(platform).$(EXT)
        include $(LIBTRANSISTOR_HOME)/libtransistor.mk
        STATIC_LINKING=1

# ARM
else ifneq (,$(findstring armv,$(platform)))
   TARGET := $(TARGET_NAME)_libretro.so
   fpic := -fPIC
   SHARED := -shared -Wl,--version-script=common/libretro-link.T
ifneq (,$(findstring cortexa8,$(platform)))
   CFLAGS += -marm -mcpu=cortex-a8
else ifneq (,$(findstring cortexa9,$(platform)))
   CFLAGS += -marm -mcpu=cortex-a9
endif
   CFLAGS += -marm
ifneq (,$(findstring neon,$(platform)))
   CFLAGS += -mfpu=neon
   HAVE_NEON = 1
endif
ifneq (,$(findstring softfloat,$(platform)))
   CFLAGS += -mfloat-abi=softfp
else ifneq (,$(findstring hardfloat,$(platform)))
   CFLAGS += -mfloat-abi=hard
endif
   CFLAGS += -DARM

# Emscripten
else ifeq ($(platform), emscripten)
	TARGET := $(TARGET_NAME)_libretro_$(platform).bc
	STATIC_LINKING = 1
	HAVE_NETWORKING=0

# Windows MSVC 2003 Xbox 1
else ifeq ($(platform), xbox1_msvc2003)
TARGET := $(TARGET_NAME)_libretro_xdk1.lib
CC  = CL.exe
CXX  = CL.exe
LD   = lib.exe

export INCLUDE := $(XDK)/xbox/include
export LIB := $(XDK)/xbox/lib
PATH := $(call unixcygpath,$(XDK)/xbox/bin/vc71):$(PATH)
PSS_STYLE :=2
CFLAGS   += -D_XBOX -D_XBOX1
CXXFLAGS += -D_XBOX -D_XBOX1
STATIC_LINKING=1
HAS_GCC := 0
USE_CODEC_FLAC=0
LIBS += ws2_32.lib

# Windows MSVC 2010 Xbox 360
else ifeq ($(platform), xbox360_msvc2010)
TARGET := $(TARGET_NAME)_libretro_xdk360.lib
MSVCBINDIRPREFIX = $(XEDK)/bin/win32
CC  = "$(MSVCBINDIRPREFIX)/cl.exe"
CXX  = "$(MSVCBINDIRPREFIX)/cl.exe"
LD   = "$(MSVCBINDIRPREFIX)/lib.exe"

export INCLUDE := $(XEDK)/include/xbox
export LIB := $(XEDK)/lib/xbox
PSS_STYLE :=2
CFLAGS   += -D_XBOX -D_XBOX360
CXXFLAGS += -D_XBOX -D_XBOX360
STATIC_LINKING=1
HAS_GCC := 0
LIBS += ws2_32.lib

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
LIBS += ws2_32.lib

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
LIBS += ws2_32.lib

# Windows MSVC 2005 x86
else ifeq ($(platform), windows_msvc2005_x86)
	CC  = cl.exe
	CXX = cl.exe

PATH := $(shell IFS=$$'\n'; cygpath "$(VS80COMNTOOLS)../../VC/bin"):$(PATH)
PATH := $(PATH):$(shell IFS=$$'\n'; cygpath "$(VS80COMNTOOLS)../IDE")
INCLUDE := $(shell IFS=$$'\n'; cygpath "$(VS80COMNTOOLS)../../VC/include")
LIB := $(shell IFS=$$'\n'; cygpath -w "$(VS80COMNTOOLS)../../VC/lib")
BIN := $(shell IFS=$$'\n'; cygpath "$(VS80COMNTOOLS)../../VC/bin")

WindowsSdkDir := $(shell reg query "HKLM\SOFTWARE\Microsoft\Microsoft SDKs\Windows\v7.1A" -v "InstallationFolder" | grep -o '[A-Z]:\\.*')
WindowsSdkDir ?= $(shell reg query "HKLM\SOFTWARE\Microsoft\Microsoft SDKs\Windows\v7.0A" -v "InstallationFolder" | grep -o '[A-Z]:\\.*')

WindowsSDKIncludeDir := $(shell cygpath -w "$(WindowsSdkDir)\Include")
WindowsSDKAtlIncludeDir := $(shell cygpath -w "$(WindowsSdkDir)\Include\atl")
WindowsSDKCrtIncludeDir := $(shell cygpath -w "$(WindowsSdkDir)\Include\crt")
WindowsSDKGlIncludeDir := $(shell cygpath -w "$(WindowsSdkDir)\Include\gl")
WindowsSDKMfcIncludeDir := $(shell cygpath -w "$(WindowsSdkDir)\Include\mfc")
WindowsSDKLibDir := $(shell cygpath -w "$(WindowsSdkDir)\Lib")

INCFLAGS_PLATFORM = -I"$(WindowsSDKIncludeDir)"
export INCLUDE := $(INCLUDE);$(WindowsSDKIncludeDir);$(WindowsSDKAtlIncludeDir);$(WindowsSDKCrtIncludeDir);$(WindowsSDKGlIncludeDir);$(WindowsSDKMfcIncludeDir);libretro-common/include/compat/msvc
export LIB := $(LIB);$(WindowsSDKLibDir)
TARGET := $(TARGET_NAME)_libretro.dll
PSS_STYLE :=2
LDFLAGS += -DLL
CFLAGS += -D_CRT_SECURE_NO_DEPRECATE

LIBS += ws2_32.lib

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

export INCLUDE := $(INCLUDE);$(INETSDK)/Include;libretro-common/include/compat/msvc
export LIB := $(LIB);$(WindowsSdkDir);$(INETSDK)/Lib
TARGET := $(TARGET_NAME)_libretro.dll
PSS_STYLE :=2
LDFLAGS += -DLL
CFLAGS += -D_CRT_SECURE_NO_DEPRECATE

LIBS += ws2_32.lib

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
		LIB := $(shell IFS=$$'\n'; cygpath -w "$(LIB)/store")
	endif
    
	export INCLUDE := $(INCLUDE);$(WindowsSDKSharedIncludeDir);$(WindowsSDKUCRTIncludeDir);$(WindowsSDKUMIncludeDir)
	export LIB := $(LIB);$(WindowsSDKUCRTLibDir);$(WindowsSDKUMLibDir)
	TARGET := $(TARGET_NAME)_libretro.dll
	PSS_STYLE :=2
	LDFLAGS += -DLL

USE_CODEC_FLAC=0

LIBS += ws2_32.lib

# Windows
else
	EXT    ?= dll
	TARGET := $(TARGET_NAME)_libretro.$(EXT)
   CC ?= gcc
   fpic :=
   LD_FLAGS :=
   SHARED := -shared -static-libgcc -static-libstdc++
   CFLAGS += -D__WIN32__
   WINSOCKS := -lws2_32
endif

ifeq ($(STATIC_LINKING),1)
SHARED=
fpic=
endif

ifeq ($(DEBUG), 1)
ifneq (,$(findstring msvc,$(platform)))
   CFLAGS   += -Od -Zi -D_DEBUG
   CXXFLAGS += -Od -Zi -D_DEBUG
else
   CFLAGS   += -O0 -g
   CXXFLAGS += -O0 -g
endif
   CFLAGS   += -DDEBUG
   CXXFLAGS += -DDEBUG
else
   CFLAGS   += -O2 -DNDEBUG
   CXXFLAGS += -O2 -DNDEBUG
endif

ifneq (,$(findstring msvc,$(platform)))
ifeq ($(DEBUG),1)
   CFLAGS   += -MTd
   CXXFLAGS += -MTd
else
   CFLAGS   += -MT
   CXXFLAGS += -MT
endif
endif

LDFLAGS += $(LIBS)
CORE_DIR := .

# Per-file -O3 overrides for two TUs whose hot inner loops auto-vectorize
# at -O3 but not at the project default -O2.  Bumping these specific files
# (rather than the whole tree) keeps the build-time and code-size effect
# narrow and avoids the inlining/peeling changes -O3 would impose on the
# rest of the codebase.
#
# snd_mix.c:
#   - paintbuffer clamp loop (per-chunk saturation clip on int32 stereo
#     samples).
#   - BGM stream mix-in loop (paintbuffer[i].left/right += s_rawsamples[]).
#   gcc -O3 auto-vec emits 16-byte SSE/NEON vectors on both; -O2 leaves
#   them scalar.  Combined per-frame saving ~0.8us at 60Hz.
#
# r_alias.c:
#   - R_AliasBlendPoseVerts pose-interpolation lerp inner loop
#     ((pv1->v * blend0 + pv2->v * blend1) >> 22, per alias vertex).
#   Bench at typical numverts=400 shows O2 3.15 ns/vert -> O3 2.22 ns/vert
#   (1.42x).  ~4us/frame at 10 visible alias entities/frame.
#
# Skipped on debug builds; appending -O3 would override the -O0 the debug
# branch sets and defeat -g optimization-off semantics.  No effect on
# platforms whose CFLAGS already pin -O3 (e.g. libnx) -- last -O wins,
# and -O3 -O3 == -O3.
ifneq ($(DEBUG), 1)
$(CORE_DIR)/common/snd_mix.o: CFLAGS += -O3
$(CORE_DIR)/common/r_alias.o: CFLAGS += -O3
endif
ifneq ($(VORBISLIB),tremor)
$(error Invalid VORBISLIB setting (only 'tremor' is supported; the float libvorbis decoder was removed))
endif
ifneq ($(MP3LIB),mpg123)
ifneq ($(MP3LIB),mad)
$(error Invalid MP3LIB setting)
endif
endif
ifeq ($(MP3LIB),mad)
mp3_obj=snd_mp3.o
lib_mp3dec=-lmad
endif
ifeq ($(MP3LIB),mpg123)
mp3_obj=snd_mpg123.o
lib_mp3dec=-lmpg123
endif
ifeq ($(VORBISLIB),tremor)
# Vendored fixed-point Tremor (deps/tremor) is compiled directly into the
# core via Makefile.common; only the -DVORBIS_USE_TREMOR define is needed
# here and no system Vorbis library is linked.
cpp_vorbisdec=-DVORBIS_USE_TREMOR
lib_vorbisdec=
endif

CODECLIBS  :=
ifeq ($(USE_CODEC_WAVE),1)
CFLAGS+= -DUSE_CODEC_WAVE
endif

ifeq ($(USE_CODEC_FLAC),1)
CFLAGS+= -DUSE_CODEC_FLAC
endif

ifeq ($(USE_CODEC_OPUS),1)
# opus and opusfile put their *.h under <includedir>/opus,
# but they include the headers without the opus directory
# prefix and rely on pkg-config. ewww...
CFLAGS+= -DUSE_CODEC_OPUS
CFLAGS+= $(shell pkg-config --cflags opusfile)
CODECLIBS+= $(shell pkg-config --libs   opusfile)
endif
ifeq ($(USE_CODEC_VORBIS),1)
CFLAGS+= -DUSE_CODEC_VORBIS $(cpp_vorbisdec)
CODECLIBS+= $(lib_vorbisdec)
endif
ifeq ($(USE_CODEC_MP3),1)
CFLAGS+= -DUSE_CODEC_MP3
CODECLIBS+= $(lib_mp3dec)
endif
ifeq ($(USE_CODEC_MIKMOD),1)
CFLAGS+= -DUSE_CODEC_MIKMOD
CODECLIBS+= -lmikmod
endif
ifeq ($(USE_CODEC_MODPLUG),1)
CFLAGS+= -DUSE_CODEC_MODPLUG
CODECLIBS+= -lmodplug
endif
ifeq ($(USE_CODEC_UMX),1)
CFLAGS+= -DUSE_CODEC_UMX
endif

LDFLAGS += $(CODECLIBS) 

include Makefile.common

OBJECTS    = $(SOURCES_C:.c=.o)

DEFINES    = -DHAVE_STRINGS_H -DHAVE_STDINT_H -DHAVE_INTTYPES_H -D__LIBRETRO__ -DNQ_HACK -DQBASEDIR=$(CORE_DIR) -DTYR_VERSION=0.62

ifneq (,$(findstring msvc,$(platform)))
DEFINES += -DINLINE="_inline"
else
DEFINES += -DINLINE="inline"
endif

ifeq ($(platform), sncps3)
WARNINGS_DEFINES =
CODE_DEFINES =
else ifeq ($(platform), ios)
WARNINGS_DEFINES =
CODE_DEFINES =
else ifneq (,$(findstring msvc,$(platform)))
WARNINGS_DEFINES =
CODE_DEFINES =
else ifeq ($(platform), emscripten)
WARNINGS_DEFINES = -Wall
CODE_DEFINES = -fomit-frame-pointer
else
WARNINGS_DEFINES = -Wall
CODE_DEFINES = -fomit-frame-pointer
endif

COMMON_DEFINES += $(CODE_DEFINES) $(WARNINGS_DEFINES) $(fpic) $(INCFLAGS) $(INCFLAGS_PLATFORM)

CFLAGS     += $(DEFINES) $(COMMON_DEFINES)

ifeq ($(FRONTEND_SUPPORTS_RGB565), 1)
CFLAGS += -DFRONTEND_SUPPORTS_RGB565
endif

ifeq ($(platform), osx)
ifndef ($(NOUNIVERSAL))
   CFLAGS += $(ARCHFLAGS)
   LFLAGS += $(ARCHFLAGS)
endif
endif

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

ifeq ($(platform), theos_ios)
COMMON_FLAGS := -DIOS $(COMMON_DEFINES) -I$(THEOS_INCLUDE_PATH) -Wno-error
$(LIBRARY_NAME)_CFLAGS += $(COMMON_FLAGS)
${LIBRARY_NAME}_FILES = $(SOURCES_C)
include $(THEOS_MAKE_PATH)/library.mk
else
all: $(TARGET)

$(TARGET): $(OBJECTS)
ifeq ($(STATIC_LINKING), 1)
	$(AR) rcs $@ $(OBJECTS)
else
	$(LD) $(fpic) $(SHARED) $(LINKOUT)$@ $(OBJECTS) $(LDFLAGS) $(WINSOCKS)
endif

%.o: %.c
	$(CC) $(INCFLAGS) $(CFLAGS) -c $(OBJOUT)$@ $<

clean-objs:
	rm -rf $(OBJECTS)

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: clean
endif
