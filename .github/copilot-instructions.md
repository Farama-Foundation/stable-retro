# Stable Retro Repository Instructions

Stable Retro is an actively developed Gymnasium-compatible fork of OpenAI Gym Retro. It combines a Python API, a C++14 extension, and vendored libretro emulator cores.

## Repository map

- `stable_retro/` is the primary Python package. `retro/` is a compatibility shim; keep both import paths working.
- `src/` contains the native runtime, pybind11 bindings, and optional Qt integration UI.
- `cores/<platform>/` contains vendored emulator source. Avoid broad cleanup or formatting there.
- `cores/*.json` contains the source platform manifests consumed by `src/coreinfo.cpp`.
- `stable_retro/cores/` contains generated build outputs. Do not edit its JSON, version stamps, or shared libraries directly.
- `tests/test_python/` contains Python tests. `tests/*.cpp` contains native tests and small test ROM fixtures are under `tests/roms/`.
- `docs/` is the Sphinx documentation source.

## Environment and build

- Supported Python versions are 3.10 through 3.14. Do not require a newer Python runtime.
- On Debian/Ubuntu, install `cmake capnproto zlib1g-dev build-essential pkg-config libzip-dev libbz2-dev xvfb python3-opengl libgl1-mesa-dev libglu1-mesa-dev`.
- Install the package and development tools with `python -m pip install -e '.[dev]'`.
- The tested native workflow uses an in-source CMake build because native tests resolve cores and ROM fixtures relative to the source tree.
- Pass optional native settings through `STABLE_RETRO_CMAKE_ARGS` or `CMAKE_ARGS`, for example `CMAKE_ARGS='-DBUILD_N64=OFF' python -m pip install -e .`.

## Validation

- Python-only changes: run `scripts/test-python.sh`, optionally followed by a specific test path.
- C++, CMake, core, or binding changes: run `CMAKE_BUILD_PARALLEL_LEVEL=2 scripts/test-cpp.sh`.
- Formatting and repository checks: run `python -m pre_commit run --files <changed-files>`. Use `--all-files` before a broad pull request.
- Documentation changes: install `docs/requirements.txt`, then run `sphinx-build -b dirhtml -v docs _build`.
- Start with the narrowest relevant test, then run the full command required by the changed boundary.

## Change rules

- Keep changes focused and preserve existing public APIs unless the task explicitly changes one.
- Do not commit generated CMake files, compiled objects, core shared libraries, or version-stamp files.
- Do not add copyrighted commercial ROMs. Only use redistributable fixtures with clear provenance.
- A new emulator platform normally requires a vendored libretro core, `cores/<platform>.json`, an `add_core` entry in `CMakeLists.txt`, packaging updates in `setup.py`, emulator smoke coverage, and documentation updates.
- Core-specific runtime options belong in `src/emulator.cpp`. Add only options needed for deterministic headless execution, serialization, input, or rendering.
- Work with existing uncommitted changes. Never clean or revert unrelated files as part of a task.
