# Vulkan headers (vendored)

This directory holds the C headers required to build tyrquake's Vulkan
renderer backend (`common/backend_vulkan.c`).  They are vendored
verbatim from upstream so that the libretro core has **no build-time
dependency** on a system Vulkan SDK or any external tooling.  Anyone
checking out the repository can build the Vulkan backend with nothing
beyond a C toolchain.

## Required files

When the Vulkan backend is enabled (`make RHI_BACKEND_VULKAN=1`), the
build expects the following layout under `deps/vulkan/`:

```
deps/vulkan/
  include/
    vulkan/
      vulkan.h               (from Khronos -- umbrella header
                              that libretro_vulkan.h includes
                              via `#include <vulkan/vulkan.h>`)
      vulkan_core.h          (from Khronos)
      vk_platform.h          (from Khronos)
    vk_video/                (from Khronos -- video codec
                              structs; included unconditionally
                              by vulkan_core.h even though
                              tyrquake uses no video extensions)
      vulkan_video_codecs_common.h
      vulkan_video_codec_h264std.h
      vulkan_video_codec_h264std_decode.h
      vulkan_video_codec_h264std_encode.h
      vulkan_video_codec_h265std.h
      vulkan_video_codec_h265std_decode.h
      vulkan_video_codec_h265std_encode.h
      vulkan_video_codec_av1std.h
      vulkan_video_codec_av1std_decode.h
    libretro_vulkan.h        (from libretro-common upstream)
  README.md                  (this file)
  LICENSE.Khronos            (Apache-2.0 -- vulkan*.h, vk_platform.h, vk_video/*)
  LICENSE.libretro           (MIT        -- libretro_vulkan.h)
```

`vulkan.h` is a 99-line umbrella that conditionally pulls in
platform-specific extensions (Win32 / Android / Wayland / etc.) gated
behind `VK_USE_PLATFORM_*` macros.  tyrquake defines none of those
macros -- the libretro frontend handles all platform-specific Vulkan
setup -- so in practice vulkan.h reduces to `#include "vk_platform.h"`
plus `#include "vulkan_core.h"`.  It is still vendored so the
`#include <vulkan/vulkan.h>` in `libretro_vulkan.h` resolves cleanly
to upstream without modification.

## Sources

### Khronos headers (vulkan.h, vulkan_core.h, vk_platform.h)

Canonical upstream:
[KhronosGroup/Vulkan-Headers](https://github.com/KhronosGroup/Vulkan-Headers/)

Pinned tag for this vendor drop: **`v1.3.280`** (chosen for broad
driver coverage -- shipping in Mesa 24+, NVIDIA 555+, AMD Adrenalin
24.x).  The tyrquake Vulkan backend only uses Vulkan 1.0 core
features for portability, so the header version is effectively a
ceiling, not a floor; any 1.3.x or 1.2.x release of vulkan_core.h
would work just as well.

To re-vendor or update:

```
git clone --depth 1 --branch v1.3.280 \
    https://github.com/KhronosGroup/Vulkan-Headers.git /tmp/vkh
cp /tmp/vkh/include/vulkan/vulkan.h       deps/vulkan/include/vulkan/
cp /tmp/vkh/include/vulkan/vulkan_core.h  deps/vulkan/include/vulkan/
cp /tmp/vkh/include/vulkan/vk_platform.h  deps/vulkan/include/vulkan/
mkdir -p deps/vulkan/include/vk_video
cp /tmp/vkh/include/vk_video/*.h          deps/vulkan/include/vk_video/
cp /tmp/vkh/LICENSE.md                    deps/vulkan/LICENSE.Khronos
```

License: Apache-2.0 (see `LICENSE.Khronos`).

### libretro Vulkan interface (libretro_vulkan.h)

Canonical upstream:
[libretro-common](https://github.com/libretro/libretro-common/blob/master/include/libretro_vulkan.h)

This header defines the `retro_hw_render_interface_vulkan` struct
that the frontend passes back to the core via
`RETRO_ENVIRONMENT_GET_HW_RENDER_INTERFACE`.  The interface gives the
core a handle to the frontend's `VkInstance`, `VkPhysicalDevice`,
`VkDevice`, queue, and a `vkGetInstanceProcAddr` function pointer
for loading every other Vulkan symbol.

To re-vendor:

```
git clone --depth 1 https://github.com/libretro/libretro-common.git /tmp/lrc
cp /tmp/lrc/include/libretro_vulkan.h     deps/vulkan/include/
```

License: MIT (the license text is embedded at the top of the header
itself; `LICENSE.libretro` carries the same text as a standalone file
for the audit trail).  Earlier libretro projects sometimes describe
their licensing as "zlib" -- the actual license clauses in
libretro_vulkan.h are the MIT permissive-license form, not the zlib
form.

## What we do NOT vendor

* **Platform-specific headers** (vulkan_android.h, vulkan_win32.h,
  vulkan_wayland.h, etc.).  These are included by vulkan.h only when
  the corresponding `VK_USE_PLATFORM_*` macro is defined at compile
  time, which tyrquake never does.  The libretro frontend handles all
  window-system integration; the core never touches a swapchain
  directly.
* **Volk, VMA, vulkan_hpp, glslang, SPIRV-Cross, etc.** -- C++ helpers
  and toolchain dependencies.  tyrquake's Vulkan backend is pure C and
  loads function pointers manually from `vkGetInstanceProcAddr`.

## How the backend uses them

`common/backend_vulkan.c` is the single-TU implementation.  When
`RHI_BACKEND_VULKAN` is defined at compile time, it:

1. Includes `vulkan/vulkan_core.h` for the API types / enums / structs
   / function-pointer typedefs.
2. Includes `libretro_vulkan.h` for the libretro HW interface struct.
3. Caches `vkGetInstanceProcAddr` from the frontend's interface and
   loads every Vulkan entry point it needs into a file-static table.
4. Renders into the per-frame `retro_vulkan_image` the frontend
   negotiates, signalling completion via the frontend-provided
   semaphore.

The backend builds without linking `libvulkan` or any other library;
all Vulkan symbols are resolved at runtime through the loader the
frontend hands us.

## Why we don't pre-compile or bundle a Vulkan loader

A libretro core runs inside a frontend that already has its own
Vulkan loader linked in for its own UI rendering.  The frontend passes
us its `vkGetInstanceProcAddr` so we use the same loader; bringing
our own would conflict and waste memory.  This is the standard
libretro Vulkan model -- see `paraLLEl-RDP`, `beetle-psx-libretro`,
and other RetroArch cores for the same pattern.
