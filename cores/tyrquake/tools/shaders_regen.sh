#!/bin/sh
#
# shaders_regen.sh -- regenerate every SPIR-V bytecode header
# under common/shaders/generated/spv/ from the GLSL source
# files under common/shaders/src/.
#
# This script is OFFLINE tooling.  The tyrquake build never
# runs it: the build sees only the pre-compiled .h files
# under common/shaders/generated/spv/, which are committed to
# the repository alongside the source.  This script is what
# the developer runs after editing a .glsl source to refresh
# the corresponding generated header.
#
# Required tools (developer side, NOT a build dependency):
#   glslang      -- GLSL -> SPIR-V compiler.  Khronos
#                   reference implementation.  Ubuntu apt
#                   package: glslang-tools.  Windows: download
#                   from the LunarG Vulkan SDK or build from
#                   KhronosGroup/glslang.
#   python3      -- for tools/spv2h.py
#
# Optional:
#   spirv-opt    -- SPIR-V optimizer.  Reduces bytecode size
#                   and applies legalization passes.  We pass
#                   -O which is a balanced default.  If not
#                   installed, the script falls back to the
#                   unoptimized glslang output.  Ubuntu apt
#                   package: spirv-tools.
#
# Usage:
#   sh tools/shaders_regen.sh
#
# Add a new shader: drop a .vert or .frag file under
# common/shaders/src/, then append a regen_shader line below.

set -eu

cd "$(dirname "$0")/.."

if ! command -v glslang >/dev/null 2>&1; then
    echo "error: glslang not found in PATH" >&2
    echo "       Ubuntu/Debian: apt-get install glslang-tools" >&2
    exit 1
fi

OPT=""
if command -v spirv-opt >/dev/null 2>&1; then
    OPT="spirv-opt"
fi

regen_shader() {
    # $1 = stage (vert | frag)
    # $2 = base name (e.g. fullscreen_quad)
    # $3 = symbol suffix (e.g. vs / fs)
    stage="$1"
    base="$2"
    suffix="$3"
    src="common/shaders/src/${base}.${stage}"
    spv="common/shaders/generated/spv/${base}_${suffix}.spv"
    hdr="common/shaders/generated/spv/${base}_${suffix}.h"
    sym="spv_${base}_${suffix}"

    mkdir -p "common/shaders/generated/spv"

    echo "regen: ${src} -> ${hdr}"

    case "${stage}" in
        vert) glslang_stage="vert" ;;
        frag) glslang_stage="frag" ;;
        comp) glslang_stage="comp" ;;
        *)
            echo "error: unknown shader stage '${stage}'" >&2
            exit 1
            ;;
    esac

    glslang -V -S "${glslang_stage}" "${src}" -o "${spv}"

    if [ -n "${OPT}" ]; then
        ${OPT} -O "${spv}" -o "${spv}.opt"
        mv "${spv}.opt" "${spv}"
    fi

    python3 tools/spv2h.py "${spv}" "${hdr}" "${sym}"

    # The .spv binary is an intermediate; we commit the .h
    # only.  Remove the binary so a `git status` after running
    # this script is clean (modulo the .h files).
    rm -f "${spv}"
}

# ============================================================
# Add new shaders below.
# ============================================================

regen_shader vert fullscreen_quad  vs
regen_shader frag gradient         fs
regen_shader frag textured         fs
regen_shader frag textured_palette fs
regen_shader comp textured_palette cs
regen_shader vert overlay_quad     vs
regen_shader frag overlay_quad     fs
regen_shader comp particles        cs
regen_shader comp warpscreen       cs
regen_shader comp sprite           cs
regen_shader comp alias            cs
regen_shader comp sky              cs
regen_shader comp turb             cs

echo "done."
