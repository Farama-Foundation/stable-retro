--------------------
 TyrQuake (libretro)
--------------------

A libretro core for Quake and its mission packs, based on TyrQuake 0.62
by Kevin Shanahan (aka. Tyrann) -- http://disenchant.net

TyrQuake is a conservative branch of the original Quake source code:
it keeps the look and feel of the id Software originals while fixing
bugs and adding carefully-scoped enhancements. This libretro port turns
it into a core that runs inside RetroArch and other libretro frontends,
with renderer, video, audio and input features layered on top of the
faithful base engine.


Loading content
---------------
- Load a .pak file (e.g. id1/pak0.pak) from a Quake data directory.
  The full directory layout around the pak is used, so custom content
  in the same game directory is picked up as usual.
- Mission packs are detected automatically from the content path:
    * "hipnotic" -> Scourge of Armagon  (-hipnotic)
    * "rogue"    -> Dissolution of Eternity (-rogue)
    * "quoth"    -> Quoth (-quoth)
- Any other (non-id1) game directory is launched as a mod via -game,
  so total conversions and standalone mods work by loading their pak
  from inside their own directory.
- Shareware, registered and mod paks are all supported.


Renderer backends
-----------------
- Software renderer: the classic 8-bit palettized span rasterizer,
  extended well beyond the original (see "Rendering features" below).
  Output is RGB565.
- Vulkan hardware renderer (optional, RHI_HAVE_VULKAN=1 builds): uses
  the libretro Vulkan HW context. Presentation runs as a GPU compute
  dispatch (palette LUT expansion on the GPU) rather than a classic
  graphics pipeline.
- Compute rendering option: on HW backends with compute support, the
  3D view is rendered by a GPU port of Quake's software rasterizer
  (spans, affine texturing, surface cache, alias edge stepping) --
  pixel-identical to the software renderer but offloaded to the GPU,
  which is much faster at high internal resolutions. When disabled,
  the HW backend uses traditional hardware rasterization instead.
- "Auto" renderer selection picks the best hardware backend the
  frontend offers and falls back to software when no HW context is
  available.


Rendering features
------------------
All of these are available in the software renderer and exposed in the
in-game Options -> Video menu (as well as via console cvars):

- Internal resolutions from 320x200 up to 1920x1200 (45 presets),
  with sensible per-platform defaults (e.g. 400x240 on 3DS).
- Dither filtering (ordered-dither texture sampling for a smoothed
  look without leaving the palette pipeline).
- Colored lighting: Half-Life / Quake II-style RGB lighting in the
  software renderer, including support for .lit sidecar files
  (with header and size validation), plus a light-dither option.
- Phong shading on models, tri-state: Off / Phong / Phong + specular.
- Transparent liquids: independent alpha controls for water, lava,
  slime and teleporters, plus an underwater liquid-blend overlay.
- Polygon subdivision: runtime tessellation of alias (model) meshes,
  up to 3 passes, to smooth out low-poly silhouettes.
- Smooth animation and smooth movement: keyframe (r_lerpmodels) and
  transform (r_lerpmove) interpolation for fluid entity motion at
  high framerates.
- Entity shadows.
- Water warp with adjustable intensity (r_waterwarp_scale).
- Level-of-detail (mip) control.
- Aspect-ratio correction.
- UI scaling (scr_uiscale): integer scaling of the 320x200-native
  HUD, menus and console at high resolutions.
- Gamma, brightness and contrast controls.
- Crosshair options.
- Persistent gibs option (Options -> Game).
- Chase camera / third-person view with camera-type selection and a
  first-person toggle in the menu.


Timing
------
- Runs at fixed frametimes for consistent physics and demo playback.
- Framerate is selectable from 10 to 600 fps, or "Auto", which
  queries the frontend's target refresh rate and matches it.
  (Values above 72 fps can expose original-engine timing quirks.)


Audio
-----
- Sound mixed at a selectable output rate: 32 / 44.1 / 48 / 96 kHz,
  or "Auto", which matches the frontend's reported target sample rate
  to bypass the frontend resampler entirely (lower latency, no extra
  filtering).
- Float PCM output is negotiated with frontends that support it, with
  a transparent fallback to the standard 16-bit integer batch path.
- Music-stream decode is float-based with bit-identical int16 results
  across platforms and FPUs.


Soundtrack / background music
-----------------------------
- CD-audio replacement: rips placed in a "music" directory inside the
  game directory (music/track02.<ext>, etc.) play automatically as the
  level soundtrack.
- WAV, FLAC and Ogg Vorbis are enabled by default; MP3, Opus, and
  tracker formats (MikMod, ModPlug, UMX) are available as build-time
  options.
- Mod-supplied tracks take priority over id1 tracks of the same
  number, and file format is resolved per-track.
- "music" console command for playing tracks manually.


Input
-----
Four selectable input device types:

- Gamepad Classic: original-style layout with a run-mode toggle.
- Gamepad Classic Alt: alternative classic layout.
- Gamepad Modern: dual-analog FPS layout (move + look on sticks).
- Keyboard + Mouse: full keyboard support via the libretro keyboard
  callback plus relative mouse look with five mouse buttons
  (including wheel up/down). The console is reachable on ` / ~ / '.

Additional input features:

- Configurable analog stick deadzone (0-30%).
- Invert Y axis option.
- Rumble support: strong motor feedback scaled by damage taken, weak
  motor pulses on item pickup.
- Fully rebindable controls through the in-game Customize Controls
  menu.


Multiplayer
-----------
- NetQuake UDP networking on platforms built with HAVE_NETWORKING=1:
  LAN games, server search and direct connect through the in-game
  Multiplayer menu, with cooperative and deathmatch game options
  (frag limits, time limits, skill, episode selection, including the
  mission-pack episode lists).
- Local (loopback) games on all platforms.


Saves and memory
----------------
- Native Quake save/load system; savegames are written to the
  frontend's save directory. (Libretro savestates are not currently
  implemented.)
- The engine heap (hunk) is sized automatically from the memory the
  frontend reports as available (a quarter of free RAM, capped at
  256 MB and never below the tuned per-platform default), so large
  modern maps and mods load on capable systems while low-memory
  platforms keep a conservative footprint.


Platforms
---------
Builds for a wide range of libretro targets, including Windows
(MinGW and MSVC toolchains as old as 2003), Linux, macOS, iOS/tvOS,
Android, Emscripten (web), Nintendo Switch, Wii, Wii U, GameCube,
3DS, PlayStation Vita, PSP, PS3, Xbox / Xbox 360, Raspberry Pi and
various embedded handhelds (GCW-Zero, Miyoo, RetroFW/Dingux).


Building
--------
    make

produces tyrquake_libretro for the host platform. Cross builds use
the usual libretro conventions, e.g.:

    make platform=vita
    make platform=libnx

Optional build flags:

    RHI_HAVE_VULKAN=1     enable the Vulkan renderer backend
                          (requires vendored Vulkan headers, see
                          deps/vulkan/README.md)
    HAVE_NETWORKING=0/1   toggle UDP multiplayer support
    USE_CODEC_MP3=1       enable MP3 soundtrack support
    USE_CODEC_OPUS=1      enable Opus soundtrack support
    USE_CODEC_MIKMOD=1    enable MikMod tracker music
    USE_CODEC_MODPLUG=1   enable ModPlug tracker music
    USE_CODEC_UMX=1       enable UMX tracker music


Credits and license
-------------------
- Quake is Copyright (C) 1996-1997 Id Software, Inc.
- TyrQuake is developed by Kevin Shanahan (aka. Tyrann),
  http://disenchant.net
- libretro port by the libretro team and contributors.

Distributed under the GNU General Public License, version 2 or later;
see LICENSE.txt. The original id Software readme is preserved as
readme-id.txt.
