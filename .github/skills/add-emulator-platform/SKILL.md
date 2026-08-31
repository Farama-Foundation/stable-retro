---
name: add-emulator-platform
description: "Add or review a new emulator platform, game system, libretro core, ROM extension mapping, or core build target in Stable Retro. Use for platform ports and core integrations, not ordinary game integrations."
argument-hint: "Describe the platform, libretro core, target operating systems, and available test ROM."
---

# Add an Emulator Platform

Use this workflow for a new emulated system or libretro core. A game integration only adds data under `stable_retro/data/` and should follow `docs/integration.md` instead.

## Gather requirements

Before editing, establish:

- Stable Retro platform identifier and game-directory suffix.
- Core directory slug and expected libretro library basename.
- Upstream repository, pinned revision, license, and redistribution terms.
- ROM extensions, libretro joypad layout, RAM regions, serialization behavior, and BIOS needs.
- Supported host matrix: Linux, Windows, macOS x86_64, and macOS arm64.
- A legally redistributable smoke-test ROM, or a documented manual-test substitute.

Compare one nearby simple platform manifest and one platform with similar rendering, BIOS, or architecture constraints. Do not assume a core's standalone defaults are suitable for deterministic, headless reinforcement learning.

## Implement the platform

1. Vendor the pinned core under `cores/<platform>/`. Preserve upstream licensing and avoid unrelated source edits.
2. Ensure the core's Makefile produces `<core_name>_libretro` with the host shared-library suffix. The top-level `add_core` function expects this exact name.
3. Copy [the manifest template](./assets/platform-manifest.json) to `cores/<platform>.json` and adapt it. The top-level key is the public platform identifier; `lib` is `<core_name>` from CMake.
4. Keep `buttons` and `keybinds` aligned to libretro joypad IDs. Use `null` for unsupported positions and list only valid button names in `actions`.
5. Validate the manifest before compiling:

   ```shell
   python .github/skills/add-emulator-platform/scripts/validate_manifest.py cores/<platform>.json
   ```

6. Add `add_core(<platform> <core_name>)` in `CMakeLists.txt`. Guard it when host support or system libraries are conditional.
7. Add only required deterministic core options to `s_envVariables` in `src/emulator.cpp`. Prefer software rendering unless the core truly requires `ENABLE_HW_RENDER`.
8. Add the platform suffix to `setup.py` only when packaged game integrations use that suffix. Core JSON and shared-library packaging is already generic.
9. Update `tests/emulator.cpp` with manifest loading and an emulator smoke parameter when a redistributable fixture exists. Otherwise add lower-level mapping coverage and record manual load, frame, audio, reset, and serialization evidence.
10. Update `docs/supported_emulators.md` and, when users can import games for the system, the supported ROM types in `docs/integration.md`.

## Validate incrementally

Build the new core first:

```shell
cmake -S . -B . -DCMAKE_DISABLE_FIND_PACKAGE_CapnProto=TRUE
cmake --build . --parallel 2 --target <platform>
```

Then run the native suite:

```shell
CMAKE_BUILD_PARALLEL_LEVEL=2 scripts/test-cpp.sh
```

Run relevant Python tests and repository checks:

```shell
scripts/test-python.sh
python -m pre_commit run --files <changed-files>
```

Use [the completion checklist](./references/checklist.md) before finishing. Clearly report any host platform or hardware path that was not tested.
