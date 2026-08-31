---
description: "Use when adding or updating an emulator platform, libretro core, core manifest, ROM extension mapping, or platform support documentation."
applyTo: "cores/**,CMakeLists.txt,setup.py,src/emulator.cpp,tests/emulator.cpp,docs/supported_emulators.md"
---

# Emulator Core Guidelines

- Distinguish an emulator platform/core integration from a game integration under `stable_retro/data/`.
- Edit the source manifest at `cores/<platform>.json`; files under `stable_retro/cores/` are generated.
- Keep platform identifiers consistent across the manifest, integration directory suffixes in `setup.py`, tests, and documentation.
- The manifest's `lib` must match the second argument of `add_core(<platform> <core_name>)`.
- Verify every ROM extension is unambiguous because `src/coreinfo.cpp` maps extensions to platforms globally.
- Avoid patching vendored core source when a top-level CMake flag or narrowly scoped adapter change can solve the problem.
- Add a redistributable smoke-test fixture only when its license and provenance are clear.
- Run `CMAKE_BUILD_PARALLEL_LEVEL=2 scripts/test-cpp.sh` and relevant Python tests before completion.
