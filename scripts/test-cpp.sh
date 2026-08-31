#!/usr/bin/env bash

set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")/.."

if [[ -n "${CMAKE_BUILD_PARALLEL_LEVEL:-}" ]]; then
	jobs="${CMAKE_BUILD_PARALLEL_LEVEL}"
elif command -v nproc >/dev/null 2>&1; then
	jobs="$(nproc)"
else
	jobs="2"
fi

cmake -S . -B . -DCMAKE_DISABLE_FIND_PACKAGE_CapnProto=TRUE "$@"
cmake --build . --parallel "${jobs}"
cmake --build . --parallel "${jobs}" --target build-tests
ctest --test-dir . --output-on-failure -R '^(data|emulator|memory-overlay|memory|script|search)$'
