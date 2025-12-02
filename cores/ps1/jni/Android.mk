LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

$(shell cd "$(LOCAL_PATH)" && ((git describe --always || echo) | sed -e 's/.*/#define REV "\0"/' > ../include/revision.h_))
$(shell cd "$(LOCAL_PATH)" && (diff -q ../include/revision.h_ ../include/revision.h > /dev/null 2>&1 || cp ../include/revision.h_ ../include/revision.h))
$(shell cd "$(LOCAL_PATH)" && (rm ../include/revision.h_))

USE_LIBRETRO_VFS ?= 0
USE_ASYNC_CDROM ?= 1
USE_RTHREADS ?= 0
NDRC_THREAD ?= 1

ROOT_DIR     := $(LOCAL_PATH)/..
CORE_DIR     := $(ROOT_DIR)/libpcsxcore
SPU_DIR      := $(ROOT_DIR)/plugins/dfsound
GPU_DIR      := $(ROOT_DIR)/plugins/gpulib
CDR_DIR      := $(ROOT_DIR)/plugins/cdrcimg
FRONTEND_DIR := $(ROOT_DIR)/frontend
NEON_DIR     := $(ROOT_DIR)/plugins/gpu_neon
UNAI_DIR     := $(ROOT_DIR)/plugins/gpu_unai
PEOPS_DIR    := $(ROOT_DIR)/plugins/dfxvideo
DYNAREC_DIR  := $(ROOT_DIR)/libpcsxcore/new_dynarec
DEPS_DIR     := $(ROOT_DIR)/deps
LIBRETRO_COMMON := $(DEPS_DIR)/libretro-common
EXTRA_INCLUDES :=
COREFLAGS    :=
SOURCES_ASM  :=

# core
SOURCES_C := $(CORE_DIR)/cdriso.c \
             $(CORE_DIR)/cdrom.c \
             $(CORE_DIR)/cdrom-async.c \
             $(CORE_DIR)/cheat.c \
             $(CORE_DIR)/database.c \
             $(CORE_DIR)/decode_xa.c \
             $(CORE_DIR)/mdec.c \
             $(CORE_DIR)/misc.c \
             $(CORE_DIR)/plugins.c \
             $(CORE_DIR)/ppf.c \
             $(CORE_DIR)/psxbios.c \
             $(CORE_DIR)/psxcommon.c \
             $(CORE_DIR)/psxcounters.c \
             $(CORE_DIR)/psxdma.c \
             $(CORE_DIR)/psxevents.c \
             $(CORE_DIR)/psxhw.c \
             $(CORE_DIR)/psxinterpreter.c \
             $(CORE_DIR)/psxmem.c \
             $(CORE_DIR)/r3000a.c \
             $(CORE_DIR)/sio.c \
             $(CORE_DIR)/spu.c \
             $(CORE_DIR)/gpu.c \
             $(CORE_DIR)/gte.c \
             $(CORE_DIR)/gte_nf.c \
             $(CORE_DIR)/gte_divider.c

# spu
SOURCES_C += $(SPU_DIR)/dma.c \
             $(SPU_DIR)/freeze.c \
             $(SPU_DIR)/registers.c \
             $(SPU_DIR)/spu.c \
             $(SPU_DIR)/out.c \
             $(SPU_DIR)/nullsnd.c

# gpu
SOURCES_C += $(GPU_DIR)/gpu.c \
             $(GPU_DIR)/prim.c \
             $(GPU_DIR)/vout_pl.c

# cdrcimg
SOURCES_C += $(CDR_DIR)/cdrcimg.c

# frontend
SOURCES_C += $(FRONTEND_DIR)/main.c \
             $(FRONTEND_DIR)/plugin.c \
             $(FRONTEND_DIR)/cspace.c \
             $(FRONTEND_DIR)/libretro.c

# libchdr
LCHDR = $(DEPS_DIR)/libchdr
LCHDR_LZMA = $(LCHDR)/deps/lzma-24.05
LCHDR_ZSTD = $(LCHDR)/deps/zstd-1.5.6/lib
SOURCES_C += \
	     $(LCHDR)/src/libchdr_bitstream.c \
	     $(LCHDR)/src/libchdr_cdrom.c \
	     $(LCHDR)/src/libchdr_chd.c \
	     $(LCHDR)/src/libchdr_flac.c \
	     $(LCHDR)/src/libchdr_huffman.c \
	     $(LCHDR_LZMA)/src/Alloc.c \
	     $(LCHDR_LZMA)/src/CpuArch.c \
	     $(LCHDR_LZMA)/src/Delta.c \
	     $(LCHDR_LZMA)/src/LzFind.c \
	     $(LCHDR_LZMA)/src/LzmaDec.c \
	     $(LCHDR_LZMA)/src/LzmaEnc.c \
	     $(LCHDR_LZMA)/src/Sort.c \
	     $(LCHDR_ZSTD)/common/entropy_common.c \
	     $(LCHDR_ZSTD)/common/error_private.c \
	     $(LCHDR_ZSTD)/common/fse_decompress.c \
	     $(LCHDR_ZSTD)/common/xxhash.c \
	     $(LCHDR_ZSTD)/common/zstd_common.c \
	     $(LCHDR_ZSTD)/decompress/huf_decompress.c \
	     $(LCHDR_ZSTD)/decompress/zstd_ddict.c \
	     $(LCHDR_ZSTD)/decompress/zstd_decompress_block.c \
	     $(LCHDR_ZSTD)/decompress/zstd_decompress.c
EXTRA_INCLUDES += $(LCHDR)/include $(LCHDR_LZMA)/include $(LCHDR_ZSTD)
COREFLAGS += -DHAVE_CHD -DZ7_ST -DZSTD_DISABLE_ASM
ifeq (,$(call gte,$(APP_PLATFORM_LEVEL),18))
ifneq ($(TARGET_ARCH_ABI),arm64-v8a)
# HACK
COREFLAGS += -Dgetauxval=0*
endif
endif

COREFLAGS += -ffast-math -DHAVE_LIBRETRO -DNO_FRONTEND -DANDROID -DREARMED
COREFLAGS += -DP_HAVE_MMAP=1 -DP_HAVE_PTHREAD=1 -DP_HAVE_POSIX_MEMALIGN=1

ifeq ($(USE_LIBRETRO_VFS),1)
SOURCES_C += \
             $(LIBRETRO_COMMON)/compat/compat_posix_string.c \
             $(LIBRETRO_COMMON)/compat/fopen_utf8.c \
             $(LIBRETRO_COMMON)/encodings/compat_strl.c \
             $(LIBRETRO_COMMON)/encodings/encoding_utf.c \
             $(LIBRETRO_COMMON)/file/file_path.c \
             $(LIBRETRO_COMMON)/streams/file_stream.c \
             $(LIBRETRO_COMMON)/streams/file_stream_transforms.c \
             $(LIBRETRO_COMMON)/string/stdstring.c \
             $(LIBRETRO_COMMON)/time/rtime.c \
             $(LIBRETRO_COMMON)/vfs/vfs_implementation.c
COREFLAGS += -DUSE_LIBRETRO_VFS
endif
EXTRA_INCLUDES += $(LIBRETRO_COMMON)/include

USE_RTHREADS=0
HAVE_ARI64=0
HAVE_LIGHTREC=0
LIGHTREC_CUSTOM_MAP=0
LIGHTREC_THREADED_COMPILER=0
HAVE_GPU_NEON=0
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
  HAVE_ARI64=1
  HAVE_GPU_NEON=1
else ifeq ($(TARGET_ARCH_ABI),armeabi)
  HAVE_ARI64=1
else ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
  HAVE_ARI64=1
  HAVE_GPU_NEON=1
else ifeq ($(TARGET_ARCH_ABI),x86_64)
  HAVE_LIGHTREC=1
  HAVE_GPU_NEON=1
else ifeq ($(TARGET_ARCH_ABI),x86)
  HAVE_LIGHTREC=1
  HAVE_GPU_NEON=1
else
  COREFLAGS   += -DDRC_DISABLE
endif
  COREFLAGS   += -DLIGHTREC_CUSTOM_MAP=$(LIGHTREC_CUSTOM_MAP)
  COREFLAGS   += -DLIGHTREC_ENABLE_THREADED_COMPILER=$(LIGHTREC_THREADED_COMPILER)
  COREFLAGS   += -DLIGHTREC_ENABLE_DISASSEMBLER=$(or $(LIGHTREC_DEBUG),0)
  COREFLAGS   += -DLIGHTREC_NO_DEBUG=$(if $(LIGHTREC_DEBUG),0,1)

ifeq ($(HAVE_ARI64),1)
  SOURCES_C   += $(DYNAREC_DIR)/new_dynarec.c \
                 $(DYNAREC_DIR)/pcsxmem.c
  ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
    SOURCES_ASM += $(DYNAREC_DIR)/linkage_arm64.S
  else
    SOURCES_ASM += $(CORE_DIR)/gte_arm.S \
                   $(SPU_DIR)/arm_utils.S \
                   $(DYNAREC_DIR)/linkage_arm.S
  endif
  ifeq ($(NDRC_THREAD),1)
  COREFLAGS   += -DNDRC_THREAD
  USE_RTHREADS := 1
  endif
endif
  SOURCES_C   += $(DYNAREC_DIR)/emu_if.c

ifeq ($(HAVE_LIGHTREC),1)
  COREFLAGS   += -DLIGHTREC -DLIGHTREC_STATIC -DLIGHTREC_CODE_INV=0
  EXTRA_INCLUDES += $(DEPS_DIR)/lightning/include \
		    $(DEPS_DIR)/lightrec \
		    $(DEPS_DIR)/lightrec/tlsf \
		    $(ROOT_DIR)/include/lightning \
		    $(ROOT_DIR)/include/lightrec
  SOURCES_C   += $(DEPS_DIR)/lightrec/blockcache.c \
					  $(DEPS_DIR)/lightrec/constprop.c \
					  $(DEPS_DIR)/lightrec/disassembler.c \
					  $(DEPS_DIR)/lightrec/emitter.c \
					  $(DEPS_DIR)/lightrec/interpreter.c \
					  $(DEPS_DIR)/lightrec/lightrec.c \
					  $(DEPS_DIR)/lightrec/memmanager.c \
					  $(DEPS_DIR)/lightrec/optimizer.c \
					  $(DEPS_DIR)/lightrec/regcache.c \
					  $(DEPS_DIR)/lightrec/recompiler.c \
					  $(DEPS_DIR)/lightrec/reaper.c \
					  $(DEPS_DIR)/lightrec/tlsf/tlsf.c
  SOURCES_C   += $(DEPS_DIR)/lightning/lib/jit_disasm.c \
					  $(DEPS_DIR)/lightning/lib/jit_memory.c \
					  $(DEPS_DIR)/lightning/lib/jit_names.c \
					  $(DEPS_DIR)/lightning/lib/jit_note.c \
					  $(DEPS_DIR)/lightning/lib/jit_print.c \
					  $(DEPS_DIR)/lightning/lib/jit_size.c \
					  $(DEPS_DIR)/lightning/lib/lightning.c
  SOURCES_C   += $(CORE_DIR)/lightrec/plugin.c
ifeq ($(LIGHTREC_CUSTOM_MAP),1)
  SOURCES_C   += $(CORE_DIR)/lightrec/mem.c
endif
endif


ifeq ($(HAVE_GPU_NEON),1)
  COREFLAGS   += -DNEON_BUILD -DTEXTURE_CACHE_4BPP -DTEXTURE_CACHE_8BPP -DGPU_NEON
  ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
    SOURCES_ASM += $(NEON_DIR)/psx_gpu/psx_gpu_arm_neon.S
  else
    COREFLAGS   += -DSIMD_BUILD
    SOURCES_C   += $(NEON_DIR)/psx_gpu/psx_gpu_simd.c
  endif
  SOURCES_C   += $(NEON_DIR)/psx_gpu_if.c
else ifeq ($(TARGET_ARCH_ABI),armeabi)
  COREFLAGS   += -DUSE_GPULIB=1 -DGPU_UNAI
  COREFLAGS   += -DHAVE_bgr555_to_rgb565
  SOURCES_ASM += $(UNAI_DIR)/gpu_arm.S \
                 $(FRONTEND_DIR)/cspace_arm.S
  SOURCES_C += $(UNAI_DIR)/gpulib_if.cpp
else
  COREFLAGS += -fno-strict-aliasing -DGPU_PEOPS
  SOURCES_C += $(PEOPS_DIR)/gpulib_if.c
endif

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
  COREFLAGS   += -DHAVE_bgr555_to_rgb565 -DHAVE_bgr888_to_x
  SOURCES_ASM += $(CORE_DIR)/gte_neon.S \
                 $(FRONTEND_DIR)/cspace_neon.S
endif

ifeq ($(USE_ASYNC_CDROM),1)
COREFLAGS += -DUSE_ASYNC_CDROM
USE_RTHREADS := 1
endif
ifeq ($(USE_RTHREADS),1)
SOURCES_C += \
             $(FRONTEND_DIR)/libretro-rthreads.c \
             $(LIBRETRO_COMMON)/features/features_cpu.c
COREFLAGS += -DHAVE_RTHREADS
endif

GIT_VERSION := " $(shell git rev-parse --short HEAD || echo unknown)"
ifneq ($(GIT_VERSION)," unknown")
  COREFLAGS += -DGIT_VERSION=\"$(GIT_VERSION)\"
endif

LOCAL_MODULE        := retro
LOCAL_SRC_FILES     := $(SOURCES_C) $(SOURCES_ASM)
LOCAL_CFLAGS        := $(COREFLAGS)
LOCAL_C_INCLUDES    := $(ROOT_DIR)/include
LOCAL_C_INCLUDES    += $(DEPS_DIR)/crypto
LOCAL_C_INCLUDES    += $(EXTRA_INCLUDES)
LOCAL_LDFLAGS       := -Wl,-version-script=$(FRONTEND_DIR)/libretro-version-script
LOCAL_LDFLAGS       += -Wl,--script=$(FRONTEND_DIR)/libretro-extern.T
LOCAL_LDFLAGS       += -Wl,--gc-sections
LOCAL_LDLIBS        := -lz -llog
LOCAL_ARM_MODE      := arm

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
  LOCAL_ARM_NEON  := true
endif

include $(BUILD_SHARED_LIBRARY)
