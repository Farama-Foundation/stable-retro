# Supported Emulators

This page lists the emulator cores that Stable-Retro can build, and whether they are supported on each platform.

Legend: ✓ supported, — not supported, experimental support is marked separately.

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
| TyrQuake (Quake) | Experimental | Experimental | Experimental |

[Full list of supported Arcade machines here](https://emulation.gametechwiki.com/index.php/FinalBurn_Neo)

\* On Apple Silicon (arm64), Gambatte (GB) is skipped by default in the CMake build.

† Built by default when `BUILD_N64=ON` and OpenGL headers are available. If headers are missing, the build skips the N64 core.

‡ Only available when hardware rendering is enabled (`ENABLE_HW_RENDER=ON`). Hardware rendering support is currently Linux-only in this project.

## TyrQuake limitations

TyrQuake loads Quake data from a `.pak` file. Select `id1/pak0.pak`; the core uses the surrounding directory to find the registered data, mission packs, and mods. Quake game data is not included with Stable Retro.

Stable Retro exposes a compact 136-byte little-endian memory map instead of TyrQuake's engine heap. Useful signed 32-bit values include health at byte 0, multiplayer frags at byte 4, ammo at byte 12, armor at byte 16, total secrets at byte 44, total monsters at byte 48, found secrets at byte 52, killed monsters at byte 56, intermission state at byte 128, and completed level time at byte 132. Intermission state 1 identifies a normal level ending, while 2 and 3 identify a finale or cutscene. Quake has no single-player score; `frags` is its multiplayer score.

TyrQuake does not implement libretro save states. The `Quake-Quake-v0` integration instead uses TyrQuake's native new-game reset and a fixed startup warmup, giving Python environments reproducible episode resets from the beginning of Quake. The Integration UI can search the compact statistics block, but it cannot create level-specific `.state` starting points. Keep Quake game integrations in the `experimental` integration tier.
