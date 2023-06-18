LOCAL_PATH := $(call my-dir)

ROOT_DIR := $(LOCAL_PATH)/..
CORE_DIR := $(ROOT_DIR)/src

include $(ROOT_DIR)/Makefile.common

COREFLAGS := $(DEFS) $(COREDEFS) $(CPUDEFS) $(SOUNDDEFS) $(ASMDEFS) $(DBGDEFS) -ffast-math -funroll-loops -DANDROID -DHAVE_ZLIB $(INCFLAGS)

GIT_VERSION := " $(shell git rev-parse --short HEAD || echo unknown)"
ifneq ($(GIT_VERSION)," unknown")
  COREFLAGS += -DGIT_VERSION=\"$(GIT_VERSION)\"
endif

include $(CLEAR_VARS)
LOCAL_MODULE    := retro
LOCAL_SRC_FILES := $(SOURCES_C)
LOCAL_CFLAGS    := -std=gnu90 $(COREFLAGS) -fsigned-char
LOCAL_LDFLAGS   := -Wl,-version-script=$(ROOT_DIR)/link.T

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
  LOCAL_ARM_NEON := true
endif

include $(BUILD_SHARED_LIBRARY)
