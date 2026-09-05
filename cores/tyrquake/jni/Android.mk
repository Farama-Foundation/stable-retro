LOCAL_PATH := $(call my-dir)

CORE_DIR := $(LOCAL_PATH)/..

USE_CODEC_FLAC   := 1
USE_CODEC_VORBIS := 1

include $(CORE_DIR)/Makefile.common

COREFLAGS := -ffast-math -funroll-loops -DINLINE=inline -DNQ_HACK -DQBASEDIR=. -DTYR_VERSION=0.62 -D__LIBRETRO__ -DANDROID $(INCFLAGS)
COREFLAGS += -DUSE_CODEC_WAVE -DUSE_CODEC_VORBIS -DUSE_CODEC_FLAC -DVORBIS_USE_TREMOR
# Silence warnings from the vendored Tremor decoder (clang): its LOOKUP_T macro
# expands to a second 'const' on already-const tables, and sharedbook.c calls
# abs() on a long.  Both are upstream and harmless; suppress rather than edit
# the vendored source.
COREFLAGS += -Wno-duplicate-decl-specifier -Wno-absolute-value

GIT_VERSION := " $(shell git rev-parse --short HEAD || echo unknown)"
ifneq ($(GIT_VERSION)," unknown")
  COREFLAGS += -DGIT_VERSION=\"$(GIT_VERSION)\"
endif

# Float libvorbis built as its own static library.  ndk-build applies
# LOCAL_CFLAGS per module but ignores the per-file target CFLAGS the standalone
# Makefile.common uses, so the fv_* symbol namespacing (-include) and the
# libvorbis include path have to be attached to a dedicated module here.
include $(CLEAR_VARS)
LOCAL_MODULE    := vorbis_fv
LOCAL_SRC_FILES := $(LIBVORBIS_SOURCES)
LOCAL_CFLAGS    := -std=gnu99 $(COREFLAGS) \
                   -include $(LIBVORBIS_DIR)/lib/fvorbis_rename.h \
                   -I$(LIBVORBIS_DIR)/lib -I$(LIBVORBIS_DIR)/include \
                   -Wno-maybe-uninitialized
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := retro
# libvorbis sources are built by the vorbis_fv module above, not here.
LOCAL_SRC_FILES := $(filter-out $(LIBVORBIS_SOURCES),$(SOURCES_C))
LOCAL_CFLAGS    := -std=gnu99 $(COREFLAGS)
LOCAL_STATIC_LIBRARIES := vorbis_fv
LOCAL_LDFLAGS   := -Wl,-version-script=$(CORE_DIR)/common/libretro-link.T

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
  LOCAL_ARM_NEON := true
endif

include $(BUILD_SHARED_LIBRARY)
