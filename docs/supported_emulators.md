# Supported Emulators

This page lists the emulator cores that Stable-Retro can build, and whether they are supported on each platform.

Legend: ✓ supported, — not supported.

| Emulator core | Linux | Windows | Apple |
| --- | --- | --- | --- |
| Snes9x (SNES) | ✓ | ✓ | ✓ |
| Genesis Plus GX (Genesis / SMS / Sega CD) | ✓ | ✓ | ✓ |
| FCEUmm (NES) | ✓ | ✓ | ✓ |
| Stella (Atari 2600) | ✓ | ✓ | ✓ |
| Gambatte (Game Boy) | ✓ | ✓ | ✓* |
| mGBA (GBA) | ✓ | ✓ | ✓ |
| Mednafen PCE Fast (PC Engine) | ✓ | ✓ | ✓ |
| PicoDrive (Sega 32X) | ✓ | ✓ | ✓ |
| Beetle Saturn (Saturn) | ✓ | ✓ | ✓ |
| melonDS (Nintendo DS) | ✓ | ✓ | ✓ |
| FBNeo (Arcade) | ✓ | ✓ | — |
| Parallel N64 (Nintendo 64) | ✓† | ✓† | — |
| Flycast (Dreamcast) | ✓‡ | — | — |

[Full list of supported Arcade machines here](https://emulation.gametechwiki.com/index.php/FinalBurn_Neo)

\* On Apple Silicon (arm64), Gambatte (GB) is skipped by default in the CMake build.

† Built by default when `BUILD_N64=ON` and OpenGL headers are available. If headers are missing, the build skips the N64 core.

‡ Only available when hardware rendering is enabled (`ENABLE_HW_RENDER=ON`). Hardware rendering support is currently Linux-only in this project.
