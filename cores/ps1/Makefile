# Makefile for PCSX ReARMed

# default stuff goes here, so that config can override
TARGET ?= pcsx
CFLAGS += -Wall -Iinclude -ffast-math

DEBUG ?= 0
DEBUG_SYMS ?= 0
ASSERTS ?= 0
HAVE_CHD ?= 1
ifneq ($(DEBUG)$(DEBUG_SYMS), 00)
CFLAGS += -ggdb
endif
ifneq ($(DEBUG), 1)
CFLAGS += -O3
ifneq ($(ASSERTS), 1)
CFLAGS += -DNDEBUG
endif
endif
ifeq ($(DEBUG_ASAN), 1)
CFLAGS += -fsanitize=address
LDFLAGS += -fsanitize=address
#LDFLAGS += -static-libasan
endif
ifeq ($(DEBUG_UBSAN), 1)
CFLAGS += -fsanitize=undefined -fno-sanitize=shift-base
LDFLAGS += -fsanitize=undefined
endif
#DRC_DBG = 1
#PCNT = 1

# Suppress minor warnings for dependencies
deps/%: CFLAGS += -Wno-unused -Wno-unused-function

all: config.mak target_ plugins_

ifndef NO_CONFIG_MAK
ifneq ($(wildcard config.mak),)
config.mak: ./configure
	@echo $@ is out-of-date, running configure
	@sed -n "/.*Configured with/s/[^:]*: //p" $@ | sh
include config.mak
else
config.mak:
	@echo "Please run ./configure before running make!"
	@exit 1
endif
else # NO_CONFIG_MAK
config.mak:
endif

-include Makefile.local

CC_LINK ?= $(CC)
CC_AS ?= $(CC)
LDFLAGS += $(MAIN_LDFLAGS)
#EXTRA_LDFLAGS ?= -Wl,-Map=$@.map # not on some linkers
LDLIBS += $(MAIN_LDLIBS)
ifdef PCNT
CFLAGS += -DPCNT
endif

ifneq ($(NO_FSECTIONS), 1)
CFLAGS += -ffunction-sections -fdata-sections
ifeq ($(GNU_LINKER),1)
FSECTIONS_LDFLAGS ?= -Wl,--gc-sections
LDFLAGS += $(FSECTIONS_LDFLAGS)
endif
endif # NO_FSECTIONS
CFLAGS += -DP_HAVE_MMAP=$(if $(NO_MMAP),0,1) \
	  -DP_HAVE_PTHREAD=$(if $(NO_PTHREAD),0,1) \
	  -DP_HAVE_POSIX_MEMALIGN=$(if $(NO_POSIX_MEMALIGN),0,1) \
	  -DDISABLE_MEM_LUTS=0
CXXFLAGS += $(CFLAGS)

# core
OBJS += libpcsxcore/cdriso.o libpcsxcore/cdrom.o libpcsxcore/cdrom-async.o \
	libpcsxcore/cheat.o libpcsxcore/database.o \
	libpcsxcore/decode_xa.o libpcsxcore/mdec.o \
	libpcsxcore/misc.o libpcsxcore/plugins.o libpcsxcore/ppf.o libpcsxcore/psxbios.o \
	libpcsxcore/psxcommon.o libpcsxcore/psxcounters.o libpcsxcore/psxdma.o \
	libpcsxcore/psxhw.o libpcsxcore/psxinterpreter.o libpcsxcore/psxmem.o \
	libpcsxcore/psxevents.o libpcsxcore/r3000a.o \
	libpcsxcore/sio.o libpcsxcore/spu.o libpcsxcore/gpu.o
OBJS += libpcsxcore/gte.o libpcsxcore/gte_nf.o libpcsxcore/gte_divider.o
#OBJS += libpcsxcore/debug.o libpcsxcore/socket.o libpcsxcore/disr3000a.o

ifeq ($(WANT_ZLIB),1)
ZLIB_DIR = deps/libchdr/deps/zlib-1.3.1
CFLAGS += -I$(ZLIB_DIR)
OBJS += $(ZLIB_DIR)/adler32.o \
        $(ZLIB_DIR)/compress.o \
        $(ZLIB_DIR)/crc32.o \
        $(ZLIB_DIR)/deflate.o \
        $(ZLIB_DIR)/gzclose.o \
        $(ZLIB_DIR)/gzlib.o \
        $(ZLIB_DIR)/gzread.o \
        $(ZLIB_DIR)/gzwrite.o \
        $(ZLIB_DIR)/infback.o \
        $(ZLIB_DIR)/inffast.o \
        $(ZLIB_DIR)/inflate.o \
        $(ZLIB_DIR)/inftrees.o \
        $(ZLIB_DIR)/trees.o \
        $(ZLIB_DIR)/uncompr.o \
        $(ZLIB_DIR)/zutil.o
$(ZLIB_DIR)/%.o: CFLAGS += -DHAVE_UNISTD_H
endif

ifeq "$(ARCH)" "arm"
OBJS += libpcsxcore/gte_arm.o
endif
ifeq "$(HAVE_NEON_ASM)" "1"
OBJS += libpcsxcore/gte_neon.o
endif
libpcsxcore/psxbios.o: CFLAGS += -Wno-nonnull

ifeq ($(MMAP_WIN32),1)
CFLAGS += -Iinclude/mman -Ideps/mman
OBJS += deps/mman/mman.o
endif
ifeq "$(USE_ASYNC_CDROM)" "1"
libpcsxcore/cdrom-async.o: CFLAGS += -DUSE_ASYNC_CDROM
frontend/libretro.o: CFLAGS += -DUSE_ASYNC_CDROM
frontend/menu.o: CFLAGS += -DUSE_ASYNC_CDROM
USE_RTHREADS := 1
endif

# dynarec
ifeq "$(DYNAREC)" "lightrec"
CFLAGS += -Ideps/lightning/include -Ideps/lightrec -Iinclude/lightning -Iinclude/lightrec \
		  -DLIGHTREC -DLIGHTREC_STATIC
ifeq ($(LIGHTREC_DEBUG),1)
deps/lightrec/%.o: CFLAGS += -DLOG_LEVEL=DEBUG_L
libpcsxcore/lightrec/plugin.o: CFLAGS += -DLIGHTREC_DEBUG=1
frontend/main.o: CFLAGS += -DLIGHTREC_DEBUG=1
deps/lightning/%.o: CFLAGS += -DDISASSEMBLER=1 -DBINUTILS_2_38=1 -DBINUTILS_2_29=1 \
	-DHAVE_DISASSEMBLE_INIT_FOR_TARGET=1 -DPACKAGE_VERSION=1
LDFLAGS += -lopcodes -lbfd
endif
LIGHTREC_CUSTOM_MAP ?= 0
LIGHTREC_CUSTOM_MAP_OBJ ?= libpcsxcore/lightrec/mem.o
LIGHTREC_THREADED_COMPILER ?= 0
LIGHTREC_CODE_INV ?= 0
CFLAGS += -DLIGHTREC_CUSTOM_MAP=$(LIGHTREC_CUSTOM_MAP) \
	  -DLIGHTREC_CODE_INV=$(LIGHTREC_CODE_INV) \
	  -DLIGHTREC_ENABLE_THREADED_COMPILER=$(LIGHTREC_THREADED_COMPILER) \
	  -DLIGHTREC_ENABLE_DISASSEMBLER=$(or $(LIGHTREC_DEBUG),0) \
	  -DLIGHTREC_NO_DEBUG=$(if $(LIGHTREC_DEBUG),0,1)
ifeq ($(LIGHTREC_CUSTOM_MAP),1)
LDLIBS += -lrt
OBJS += $(LIGHTREC_CUSTOM_MAP_OBJ)
endif
ifeq ($(NEED_SYSCONF),1)
OBJS += libpcsxcore/lightrec/sysconf.o
endif
ifeq ($(LIGHTREC_THREADED_COMPILER),1)
OBJS += deps/lightrec/recompiler.o \
	deps/lightrec/reaper.o
endif
OBJS += deps/lightrec/tlsf/tlsf.o
OBJS += libpcsxcore/lightrec/plugin.o
OBJS += deps/lightning/lib/jit_disasm.o \
		deps/lightning/lib/jit_memory.o \
		deps/lightning/lib/jit_names.o \
		deps/lightning/lib/jit_note.o \
		deps/lightning/lib/jit_print.o \
		deps/lightning/lib/jit_size.o \
		deps/lightning/lib/lightning.o \
		deps/lightrec/blockcache.o \
		deps/lightrec/constprop.o \
		deps/lightrec/disassembler.o \
		deps/lightrec/emitter.o \
		deps/lightrec/interpreter.o \
		deps/lightrec/lightrec.o \
		deps/lightrec/memmanager.o \
		deps/lightrec/optimizer.o \
		deps/lightrec/regcache.o
deps/lightning/%.o: CFLAGS += -DHAVE_MMAP=P_HAVE_MMAP
deps/lightning/%: CFLAGS += -Wno-uninitialized
deps/lightrec/%: CFLAGS += -Wno-uninitialized
libpcsxcore/lightrec/mem.o: CFLAGS += -D_GNU_SOURCE
ifeq ($(MMAP_WIN32),1)
deps/lightning/lib/lightning.o: CFLAGS += -Dmprotect=_mprotect # deps/mman
deps/lightning/lib/jit_print.o: CFLAGS += -w
endif
else ifeq "$(DYNAREC)" "ari64"
OBJS += libpcsxcore/new_dynarec/new_dynarec.o
OBJS += libpcsxcore/new_dynarec/pcsxmem.o
 ifeq "$(ARCH)" "arm"
 OBJS += libpcsxcore/new_dynarec/linkage_arm.o
 libpcsxcore/new_dynarec/new_dynarec.o: libpcsxcore/new_dynarec/assem_arm.c
 else ifneq (,$(findstring $(ARCH),aarch64 arm64))
 OBJS += libpcsxcore/new_dynarec/linkage_arm64.o
 libpcsxcore/new_dynarec/new_dynarec.o: libpcsxcore/new_dynarec/assem_arm64.c
 else
 $(error no dynarec support for architecture $(ARCH))
 endif
 ifeq "$(NDRC_THREAD)" "1"
 libpcsxcore/new_dynarec/new_dynarec.o: CFLAGS += -DNDRC_THREAD
 libpcsxcore/new_dynarec/emu_if.o: CFLAGS += -DNDRC_THREAD
 frontend/libretro.o: CFLAGS += -DNDRC_THREAD
 USE_RTHREADS := 1
 endif
else
CFLAGS += -DDRC_DISABLE
endif
OBJS += libpcsxcore/new_dynarec/emu_if.o
libpcsxcore/new_dynarec/new_dynarec.o: libpcsxcore/new_dynarec/pcsxmem_inline.c
ifdef DRC_DBG
libpcsxcore/new_dynarec/emu_if.o: CFLAGS += -D_FILE_OFFSET_BITS=64
CFLAGS += -DDRC_DBG
endif
ifeq "$(BASE_ADDR_DYNAMIC)" "1"
libpcsxcore/new_dynarec/%.o: CFLAGS += -DBASE_ADDR_DYNAMIC=1
endif

# spu
OBJS += plugins/dfsound/dma.o plugins/dfsound/freeze.o \
	plugins/dfsound/registers.o plugins/dfsound/spu.o \
	plugins/dfsound/out.o plugins/dfsound/nullsnd.o
plugins/dfsound/spu.o: plugins/dfsound/adsr.c plugins/dfsound/reverb.c \
	plugins/dfsound/xa.c
ifeq "$(ARCH)" "arm"
OBJS += plugins/dfsound/arm_utils.o
endif
ifeq "$(HAVE_C64_TOOLS)" "1"
plugins/dfsound/%.o: CFLAGS += -DC64X_DSP -DWANT_THREAD_CODE
plugins/dfsound/spu.o: plugins/dfsound/spu_c64x.c
frontend/menu.o: CFLAGS += -DC64X_DSP
endif
ifneq ($(findstring oss,$(SOUND_DRIVERS)),)
plugins/dfsound/out.o: CFLAGS += -DHAVE_OSS
OBJS += plugins/dfsound/oss.o
endif
ifneq ($(findstring alsa,$(SOUND_DRIVERS)),)
plugins/dfsound/out.o: CFLAGS += -DHAVE_ALSA
OBJS += plugins/dfsound/alsa.o
LDLIBS += -lasound
endif
ifneq ($(findstring sdl,$(SOUND_DRIVERS)),)
plugins/dfsound/out.o: CFLAGS += -DHAVE_SDL
OBJS += plugins/dfsound/sdl.o
endif
ifneq ($(findstring pulseaudio,$(SOUND_DRIVERS)),)
plugins/dfsound/out.o: CFLAGS += -DHAVE_PULSE
OBJS += plugins/dfsound/pulseaudio.o
endif
ifneq ($(findstring libretro,$(SOUND_DRIVERS)),)
plugins/dfsound/out.o: CFLAGS += -DHAVE_LIBRETRO
endif

# supported gpu list in menu
ifeq "$(HAVE_NEON_GPU)" "1"
frontend/menu.o: CFLAGS += -DGPU_NEON
endif
ifeq "$(HAVE_GLES)" "1"
frontend/menu.o: CFLAGS += -DHAVE_GLES
endif

# builtin gpu
OBJS += plugins/gpulib/gpu.o plugins/gpulib/vout_pl.o plugins/gpulib/prim.o
ifeq "$(BUILTIN_GPU)" "neon"
CFLAGS += -DGPU_NEON
OBJS += plugins/gpu_neon/psx_gpu_if.o
plugins/gpu_neon/psx_gpu_if.o: CFLAGS += -DNEON_BUILD -DTEXTURE_CACHE_4BPP -DTEXTURE_CACHE_8BPP
plugins/gpu_neon/psx_gpu_if.o: plugins/gpu_neon/psx_gpu/*.c
frontend/menu.o frontend/plugin_lib.o: CFLAGS += -DBUILTIN_GPU_NEON
 ifeq "$(HAVE_NEON_ASM)" "1"
 OBJS += plugins/gpu_neon/psx_gpu/psx_gpu_arm_neon.o
 else
 OBJS += plugins/gpu_neon/psx_gpu/psx_gpu_simd.o
 plugins/gpu_neon/psx_gpu_if.o: CFLAGS += -DSIMD_BUILD
 plugins/gpu_neon/psx_gpu/psx_gpu_simd.o: CFLAGS += -DSIMD_BUILD
 endif
endif
ifeq "$(BUILTIN_GPU)" "peops"
CFLAGS += -DGPU_PEOPS
# note: code is not safe for strict-aliasing? (Castlevania problems)
plugins/dfxvideo/gpulib_if.o: CFLAGS += -fno-strict-aliasing
plugins/dfxvideo/gpulib_if.o: plugins/dfxvideo/prim.c plugins/dfxvideo/soft.c
frontend/menu.o frontend/plugin_lib.o: CFLAGS += -DBUILTIN_GPU_PEOPS
OBJS += plugins/dfxvideo/gpulib_if.o
ifeq "$(THREAD_RENDERING)" "1"
CFLAGS += -DTHREAD_RENDERING
OBJS += plugins/gpulib/gpulib_thread_if.o
endif
endif

ifeq "$(BUILTIN_GPU)" "unai"
CFLAGS += -DGPU_UNAI
CFLAGS += -DUSE_GPULIB=1
OBJS += plugins/gpu_unai/gpulib_if.o
ifeq "$(ARCH)" "arm"
OBJS += plugins/gpu_unai/gpu_arm.o
endif
ifeq "$(THREAD_RENDERING)" "1"
CFLAGS += -DTHREAD_RENDERING
OBJS += plugins/gpulib/gpulib_thread_if.o
endif
ifneq "$(GPU_UNAI_NO_OLD)" "1"
OBJS += plugins/gpu_unai/old/if.o
else
CFLAGS += -DGPU_UNAI_NO_OLD
endif
plugins/gpu_unai/gpulib_if.o: plugins/gpu_unai/*.h
plugins/gpu_unai/gpulib_if.o: CFLAGS += -DREARMED -DUSE_GPULIB=1
frontend/menu.o frontend/plugin_lib.o: CFLAGS += -DBUILTIN_GPU_UNAI
ifneq ($(DEBUG), 1)
plugins/gpu_unai/gpulib_if.o \
plugins/gpu_unai/old/if.o: CFLAGS += -O3
endif
CC_LINK = $(CXX)
endif

# libchdr
ifeq "$(HAVE_CHD)" "1"
LCHDR = deps/libchdr
LCHDR_LZMA = $(LCHDR)/deps/lzma-24.05
LCHDR_ZSTD = $(LCHDR)/deps/zstd-1.5.6/lib
OBJS += $(LCHDR)/src/libchdr_bitstream.o
OBJS += $(LCHDR)/src/libchdr_cdrom.o
OBJS += $(LCHDR)/src/libchdr_chd.o
OBJS += $(LCHDR)/src/libchdr_flac.o
OBJS += $(LCHDR)/src/libchdr_huffman.o
$(LCHDR)/src/%.o: CFLAGS += -Wno-unused -Wno-maybe-uninitialized -std=gnu11
OBJS += $(LCHDR_LZMA)/src/Alloc.o
OBJS += $(LCHDR_LZMA)/src/CpuArch.o
OBJS += $(LCHDR_LZMA)/src/Delta.o
OBJS += $(LCHDR_LZMA)/src/LzFind.o
OBJS += $(LCHDR_LZMA)/src/LzmaDec.o
OBJS += $(LCHDR_LZMA)/src/LzmaEnc.o
OBJS += $(LCHDR_LZMA)/src/Sort.o
$(LCHDR_LZMA)/src/%.o: CFLAGS += -Wno-unused -DZ7_ST -I$(LCHDR_LZMA)/include
$(LCHDR)/src/%.o: CFLAGS += -I$(LCHDR_LZMA)/include
OBJS += $(LCHDR_ZSTD)/common/entropy_common.o
OBJS += $(LCHDR_ZSTD)/common/error_private.o
OBJS += $(LCHDR_ZSTD)/common/fse_decompress.o
OBJS += $(LCHDR_ZSTD)/common/xxhash.o
OBJS += $(LCHDR_ZSTD)/common/zstd_common.o
OBJS += $(LCHDR_ZSTD)/decompress/huf_decompress.o
OBJS += $(LCHDR_ZSTD)/decompress/huf_decompress_amd64.o
OBJS += $(LCHDR_ZSTD)/decompress/zstd_ddict.o
OBJS += $(LCHDR_ZSTD)/decompress/zstd_decompress_block.o
OBJS += $(LCHDR_ZSTD)/decompress/zstd_decompress.o
$(LCHDR_ZSTD)/common/%.o \
$(LCHDR_ZSTD)/decompress/%.o: CFLAGS += -I$(LCHDR_ZSTD)
$(LCHDR)/src/%.o: CFLAGS += -I$(LCHDR_ZSTD)
libpcsxcore/cdriso.o: CFLAGS += -Wno-unused-function
CFLAGS += -DHAVE_CHD -I$(LCHDR)/include
endif

# frontend/gui
OBJS += frontend/cspace.o
ifeq "$(HAVE_NEON_ASM)" "1"
OBJS += frontend/cspace_neon.o
frontend/cspace.o: CFLAGS += -DHAVE_bgr555_to_rgb565 -DHAVE_bgr888_to_x
else
ifeq "$(ARCH)" "arm"
OBJS += frontend/cspace_arm.o
frontend/cspace.o: CFLAGS += -DHAVE_bgr555_to_rgb565
endif
endif

ifeq "$(PLATFORM)" "generic"
OBJS += frontend/libpicofe/in_sdl.o
#OBJS += frontend/libpicofe/plat_sdl.o
OBJS += frontend/libpicofe/plat_dummy.o
OBJS += frontend/plat_sdl.o
frontend/plat_sdl.o frontend/libpicofe/plat_sdl.o: CFLAGS += -DSDL_OVERLAY_2X
frontend/menu.o: CFLAGS += -DSDL_OVERLAY_2X -DMENU_SHOW_VARSCALER=1
ifeq "$(HAVE_EVDEV)" "1"
OBJS += frontend/libpicofe/linux/in_evdev.o
endif
ifeq "$(HAVE_GLES)" "1"
OBJS += frontend/libpicofe/gl.o frontend/libpicofe/gl_platform.o
OBJS += frontend/libpicofe/gl_loader.o
#LDLIBS += $(LDLIBS_GLES) # loaded dynamically now
frontend/libpicofe/plat_sdl.o: CFLAGS += -DHAVE_GLES $(CFLAGS_GLES)
frontend/libpicofe/gl_platform.o: CFLAGS += -DHAVE_GLES $(CFLAGS_GLES)
frontend/libpicofe/gl.o: CFLAGS += -DHAVE_GLES $(CFLAGS_GLES)
frontend/plat_sdl.o: CFLAGS += -DHAVE_GLES $(CFLAGS_GLES)
endif
USE_PLUGIN_LIB = 1
USE_FRONTEND = 1
endif
ifeq "$(PLATFORM)" "pandora"
OBJS += frontend/libpicofe/pandora/plat.o
OBJS += frontend/libpicofe/linux/fbdev.o frontend/libpicofe/linux/xenv.o
OBJS += frontend/libpicofe/linux/in_evdev.o
OBJS += frontend/plat_pandora.o frontend/plat_omap.o
frontend/main.o frontend/menu.o: CFLAGS += -include frontend/pandora/ui_feat.h
frontend/main.o frontend/plugin_lib.o: CFLAGS += -DPANDORA
frontend/libpicofe/linux/plat.o: CFLAGS += -DPANDORA
USE_PLUGIN_LIB = 1
USE_FRONTEND = 1
CFLAGS += -gdwarf-3
endif
ifeq "$(PLATFORM)" "caanoo"
OBJS += frontend/libpicofe/gp2x/in_gp2x.o frontend/warm/warm.o
OBJS += frontend/libpicofe/gp2x/soc_pollux.o
OBJS += frontend/libpicofe/linux/in_evdev.o
OBJS += frontend/plat_pollux.o frontend/in_tsbutton.o frontend/blit320.o
frontend/main.o frontend/menu.o: CFLAGS += -include frontend/320240/ui_gp2x.h
USE_PLUGIN_LIB = 1
USE_FRONTEND = 1
endif
ifeq "$(PLATFORM)" "maemo"
OBJS += maemo/hildon.o maemo/main.o maemo/maemo_xkb.o frontend/pl_gun_ts.o
maemo/%.o: maemo/%.c
USE_PLUGIN_LIB = 1
LDFLAGS += $(shell pkg-config --libs hildon-1 libpulse)
CFLAGS += $(shell pkg-config --cflags hildon-1) -DHAVE_TSLIB
CFLAGS += `pkg-config --cflags glib-2.0 libosso dbus-1 hildon-fm-2`
LDFLAGS += `pkg-config --libs glib-2.0 libosso dbus-1 hildon-fm-2`
endif
ifeq "$(PLATFORM)" "libretro"
ifneq "$(HAVE_PHYSICAL_CDROM)$(USE_LIBRETRO_VFS)" "00"
OBJS += deps/libretro-common/compat/compat_strl.o
OBJS += deps/libretro-common/file/file_path.o
OBJS += deps/libretro-common/string/stdstring.o
OBJS += deps/libretro-common/vfs/vfs_implementation.o
endif
ifeq "$(HAVE_PHYSICAL_CDROM)" "1"
OBJS += frontend/libretro-cdrom.o
OBJS += deps/libretro-common/lists/string_list.o
OBJS += deps/libretro-common/memmap/memalign.o
OBJS += deps/libretro-common/vfs/vfs_implementation_cdrom.o
CFLAGS += -DHAVE_CDROM
endif
ifeq "$(USE_LIBRETRO_VFS)" "1"
OBJS += deps/libretro-common/compat/compat_posix_string.o
OBJS += deps/libretro-common/compat/fopen_utf8.o
OBJS += deps/libretro-common/encodings/encoding_utf.o
OBJS += deps/libretro-common/streams/file_stream.o
OBJS += deps/libretro-common/streams/file_stream_transforms.o
OBJS += deps/libretro-common/time/rtime.o
CFLAGS += -DUSE_LIBRETRO_VFS
endif
OBJS += frontend/libretro.o
CFLAGS += -DHAVE_LIBRETRO
INC_LIBRETRO_COMMON := 1

endif # $(PLATFORM) == "libretro"

ifeq "$(USE_RTHREADS)" "1"
OBJS += frontend/libretro-rthreads.o
OBJS += deps/libretro-common/features/features_cpu.o
frontend/main.o: CFLAGS += -DHAVE_RTHREADS
INC_LIBRETRO_COMMON := 1
endif
ifeq "$(INC_LIBRETRO_COMMON)" "1"
CFLAGS += -Ideps/libretro-common/include
endif

ifeq "$(USE_PLUGIN_LIB)" "1"
OBJS += frontend/plugin_lib.o
OBJS += frontend/libpicofe/linux/plat.o
OBJS += frontend/libpicofe/readpng.o frontend/libpicofe/fonts.o
frontend/libpicofe/linux/plat.o: CFLAGS += -DNO_HOME_DIR
ifeq "$(HAVE_NEON_ASM)" "1"
OBJS += frontend/libpicofe/arm/neon_scale2x.o
OBJS += frontend/libpicofe/arm/neon_eagle2x.o
frontend/libpicofe/arm/neon_scale2x.o: CFLAGS += -DDO_BGR_TO_RGB
frontend/libpicofe/arm/neon_eagle2x.o: CFLAGS += -DDO_BGR_TO_RGB
endif
endif
ifeq "$(USE_FRONTEND)" "1"
OBJS += frontend/menu.o
OBJS += frontend/libpicofe/input.o
frontend/libpicofe/input.o: CFLAGS += -Wno-array-bounds
frontend/menu.o: frontend/libpicofe/menu.c
ifeq "$(HAVE_TSLIB)" "1"
frontend/%.o: CFLAGS += -DHAVE_TSLIB
OBJS += frontend/pl_gun_ts.o
endif
else
CFLAGS += -DNO_FRONTEND
endif

# misc
OBJS += frontend/main.o frontend/plugin.o
frontend/main.o libpcsxcore/misc.o: CFLAGS += -DBUILTIN_GPU=$(BUILTIN_GPU)

frontend/menu.o frontend/main.o: include/revision.h
frontend/plat_sdl.o frontend/libretro.o: include/revision.h
libpcsxcore/misc.o: include/revision.h

CFLAGS += $(CFLAGS_LAST)

frontend/libpicofe/%.c:
	@echo "libpicofe module is missing, please run:"
	@echo "git submodule init && git submodule update"
	@exit 1

libpcsxcore/gte_nf.o: libpcsxcore/gte.c
	$(CC) -c -o $@ $^ $(CFLAGS) -DFLAGLESS

include/revision.h: FORCE
	@(git describe --always || echo) | sed -e 's/.*/#define REV "\0"/' > $@_
	@diff -q $@_ $@ > /dev/null 2>&1 || cp $@_ $@
	@rm $@_

%.o: %.S
	$(CC_AS) $(CFLAGS) -c $^ -o $@


target_: $(TARGET)

$(TARGET): $(OBJS)
ifeq ($(PARTIAL_LINKING), 1)
	$(LD) -o $(basename $(TARGET))1.o -r --gc-sections $(addprefix -u ,$(shell cat frontend/libretro-extern)) $(addprefix -u ,$(EXTRA_EXTERN_SYMS)) $^
	$(OBJCOPY) --keep-global-symbols=frontend/libretro-extern $(addprefix -G ,$(EXTRA_EXTERN_SYMS)) $(basename $(TARGET))1.o $(basename $(TARGET)).o
	$(AR) rcs $@ $(basename $(TARGET)).o
else ifeq ($(STATIC_LINKING), 1)
	$(AR) rcs $@ $^
else
	$(CC_LINK) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS) $(EXTRA_LDFLAGS)
endif

clean: $(PLAT_CLEAN) clean_plugins
	$(RM) $(TARGET) *.o $(OBJS) $(TARGET).map include/revision.h

ifneq ($(PLUGINS),)
plugins_: $(PLUGINS)

plugins/gpulib/gpulib.$(ARCH).a:
	$(MAKE) -C plugins/gpulib/

$(PLUGINS): plugins/gpulib/gpulib.$(ARCH).a
	$(MAKE) -C $(dir $@)

clean_plugins:
	$(MAKE) -C plugins/gpulib/ clean
	for dir in $(PLUGINS) ; do \
		$(MAKE) -C $$(dirname $$dir) clean; done
else
plugins_:
clean_plugins:
endif

.PHONY: all clean target_ plugins_ clean_plugins FORCE

ifneq "$(PLATFORM)" "pandora"
ifdef CPATH
$(warning warning: CPATH is defined)
endif
endif

# ----------- release -----------

VER ?= $(shell git describe --always HEAD)

ifeq "$(PLATFORM)" "generic"
OUT = pcsx_rearmed_$(VER)

rel: pcsx $(PLUGINS) \
		frontend/pandora/skin readme.txt COPYING
	rm -rf $(OUT)
	mkdir -p $(OUT)/plugins
	mkdir -p $(OUT)/bios
	cp -r $^ $(OUT)/
	-mv $(OUT)/*.so* $(OUT)/plugins/
	zip -9 -r $(OUT).zip $(OUT)
endif

ifeq "$(PLATFORM)" "pandora"
PND_MAKE ?= $(HOME)/dev/pnd/src/pandora-libraries/testdata/scripts/pnd_make.sh

rel: pcsx plugins/dfsound/pcsxr_spu_area3.out $(PLUGINS) \
		frontend/pandora/pcsx.sh frontend/pandora/pcsx.pxml.templ frontend/pandora/pcsx.png \
		frontend/pandora/picorestore frontend/pandora/skin readme.txt COPYING
	rm -rf out
	mkdir -p out/plugins
	cp -r $^ out/
	sed -e 's/%PR%/$(VER)/g' out/pcsx.pxml.templ > out/pcsx.pxml
	rm out/pcsx.pxml.templ
	-mv out/*.so out/plugins/
	$(PND_MAKE) -p pcsx_rearmed_$(VER).pnd -d out -x out/pcsx.pxml -i frontend/pandora/pcsx.png -c
endif

ifeq "$(PLATFORM)" "caanoo"
PLAT_CLEAN = caanoo_clean

caanoo_clean:
	$(RM) frontend/320240/pollux_set

rel: pcsx $(PLUGINS) \
		frontend/320240/caanoo.gpe frontend/320240/pcsx26.png \
		frontend/320240/pcsxb.png frontend/320240/skin \
		frontend/warm/bin/warm_2.6.24.ko frontend/320240/pollux_set \
		frontend/320240/pcsx_rearmed.ini frontend/320240/haptic_w.cfg \
		frontend/320240/haptic_s.cfg \
		readme.txt COPYING
	rm -rf out
	mkdir -p out/pcsx_rearmed/plugins
	cp -r $^ out/pcsx_rearmed/
	-mv out/pcsx_rearmed/*.so out/pcsx_rearmed/plugins/
	mv out/pcsx_rearmed/caanoo.gpe out/pcsx_rearmed/pcsx.gpe
	mv out/pcsx_rearmed/pcsx_rearmed.ini out/
	mkdir out/pcsx_rearmed/lib/
	cp ./lib/libbz2.so.1 out/pcsx_rearmed/lib/
	mkdir out/pcsx_rearmed/bios/
	cd out && zip -9 -r ../pcsx_rearmed_$(VER)_caanoo.zip *
endif
