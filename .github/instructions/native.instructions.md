---
description: "Use when changing C++, CMake, pybind11 bindings, native tests, rendering, or the Qt integration UI."
applyTo: "CMakeLists.txt,src/**/*.cpp,src/**/*.h,tests/*.cpp,tests/CMakeLists.txt"
---

# Native Guidelines

- Compile as C++14 and follow `.clang-format` without reformatting unrelated code.
- Include each standard-library dependency directly instead of relying on transitive includes.
- Keep platform-specific behavior behind the existing CMake options and preprocessor guards.
- Treat `cores/` and `third-party/` as vendored code; change them only when the task specifically requires it.
- Add focused GoogleTest coverage under `tests/` for native behavior changes.
- Run `CMAKE_BUILD_PARALLEL_LEVEL=2 scripts/test-cpp.sh` after native or CMake changes.
- If Python-visible bindings change, also run the relevant tests through `scripts/test-python.sh`.
