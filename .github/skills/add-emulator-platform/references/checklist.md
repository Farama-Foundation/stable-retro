# Emulator Platform Completion Checklist

## Core and licensing

- The vendored source is pinned to a known upstream revision.
- Upstream license files and required notices are present.
- No commercial ROM, BIOS, key, firmware, or copyrighted test asset was added without redistribution permission.
- Patches to vendored source are minimal and documented.

## Manifest and runtime

- `cores/<platform>.json` passes `validate_manifest.py`.
- Platform identifier, core library basename, and integration directory suffix are consistent.
- ROM extensions do not override an existing platform mapping.
- Input labels and action combinations work for every exposed player/control.
- RAM mapping, overlays, data types, and endianness were verified where applicable.
- Load, frame output, audio, reset, unload, and serialization were exercised.
- BIOS requirements and missing-BIOS errors are documented.

## Build and packaging

- The focused CMake core target builds on each claimed host platform.
- Conditional host, architecture, rendering, and dependency checks fail or skip clearly.
- Wheel or editable-install output contains the manifest and core shared library.
- Generated files under `stable_retro/cores/` are not committed.

## Tests and docs

- Automated smoke coverage uses only redistributable fixtures with documented provenance.
- `scripts/test-cpp.sh` and relevant Python tests pass.
- Pre-commit checks pass for every changed first-party file.
- Supported emulator, ROM extension, BIOS, and installation documentation is current.
- Untested hosts and manual verification are stated in the pull request.
